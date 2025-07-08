#include "stdafx.h"
#include "AerialLinearBuilder.h"

#include <glm/gtx/matrix_transform_2d.hpp>
#include "Graphics/Core/ShaderList.h"

/// [Static attributes]

/// [Public methods]

void AerialLinearBuilder::buildRays(LiDARParameters* LiDARParams, AABB& sceneAABB)
{
	ALSParameters* parameters = dynamic_cast<ALSParameters*>(_parameters);

	if (LiDARParams->_gpuInstantiation)
	{
		this->buildRaysGPU(parameters, LiDARParams, sceneAABB);
	}
	else
	{
		this->buildRaysCPU(parameters, LiDARParams, sceneAABB);
	}
}

void AerialLinearBuilder::initializeContext(LiDARParameters* LiDARParams, AABB& sceneAABB)
{
	_parameters = this->buildParameters(LiDARParams, sceneAABB);
}

/// [Protected methods]

RayBuilder::ALSParameters* AerialLinearBuilder::buildParameters(LiDARParameters* LiDARParams, AABB& sceneAABB)
{
	ALSParameters* params = new ALSParameters;
	float width;

	params->_numSteps		= this->getNumSteps(LiDARParams, sceneAABB, width);
	params->_scansSec		= LiDARParams->_alsScanFrequency;
	params->_pulsesSec		= LiDARParams->_alsPulseFrequency;
	params->_metersSec		= LiDARParams->_alsSpeed;
	params->_numPulsesScan	= params->_pulsesSec / params->_scansSec;
	params->_advanceScan	= params->_metersSec / params->_scansSec;
	params->_advanceScan_t	= params->_advanceScan / (sceneAABB.size().x + BOUNDARY_OFFSET * 2.0f);
	params->_advancePulse	= 1.0f / params->_pulsesSec;
	params->_fovRadians		= LiDARParams->_alsFOVHorizontal * M_PI / 180.0f;
	params->_startRadians	= -params->_fovRadians / 2.0f + 2.0f * M_PI;
	params->_incrementRadians = params->_fovRadians / params->_numPulsesScan;
	params->_upVector		= AERIAL_UP_VECTOR;

	// State params
	std::vector<vec4> waypoints;
	std::vector<Interpolation*> airbonePaths = this->getAirbonePaths(LiDARParams, params->_numSteps, sceneAABB, LiDARParams->_alsPosition.y);
	this->retrievePath(airbonePaths, waypoints, params->_advanceScan_t);

	params->_numThreads = (waypoints.size() - airbonePaths.size()) * params->_numPulsesScan;
	params->_pathLength = waypoints.size() / airbonePaths.size();		// Each interpolation produces the same number of points
	RayBuilder::initializeContext(LiDARParams, params);

	if (LiDARParams->_gpuInstantiation)
	{
		params->_waypointBuffer = ComputeShader::setReadBuffer(waypoints, GL_STATIC_DRAW);
	}

	return params;
}

void AerialLinearBuilder::buildRaysCPU(ALSParameters* parameters, LiDARParameters* LiDARParams, AABB& sceneAABB)
{
	// Jittering initialization
	RandomUtilities::initializeUniformDistribution(-1.0f, 1.0f);

	// Retrieve waypoints to emulate platform movement
	std::vector<vec4> waypoints;
	std::vector<Interpolation*> airbonePaths = this->getAirbonePaths(LiDARParams, parameters->_numSteps, sceneAABB, LiDARParams->_alsPosition.y);
	this->retrievePath(airbonePaths, waypoints, parameters->_advanceScan_t);

	// Iterate through each interpolation path
	unsigned pathLength = waypoints.size() / airbonePaths.size();		// Each interpolation produces the same number of points
	unsigned raysPerPathTimestamp = (pathLength - 1) * parameters->_numPulsesScan * LiDARParams->_raysPulse;
	std::vector<Model3D::RayGPUData> rays(airbonePaths.size() * raysPerPathTimestamp * LiDARParams->_raysPulse);

	for (int pathIdx = 0; pathIdx < airbonePaths.size(); ++pathIdx)
	{
		unsigned pointOffset = pathLength * pathIdx;

		for (int point = pointOffset + 1; point < pointOffset + pathLength; ++point)
		{
			unsigned baseIndex = pathIdx * raysPerPathTimestamp + (point - 1) * parameters->_numPulsesScan * LiDARParams->_raysPulse;
			this->throwRays(parameters, LiDARParams, rays, waypoints[point], glm::normalize(waypoints[point] - waypoints[point - 1]), parameters->_startRadians, parameters->_fovRadians, parameters->_numPulsesScan, baseIndex);
		}
	}

	parameters->_leftRays -= std::min(size_t(parameters->_leftRays), rays.size());

	for (Interpolation* interpolation : airbonePaths)
	{
		delete interpolation;
	}

	parameters->_rayBuffer = ComputeShader::setReadBuffer(rays, GL_DYNAMIC_DRAW);
}

void AerialLinearBuilder::buildRaysGPU(ALSParameters* parameters, LiDARParameters* LiDARParams, AABB& sceneAABB)
{
	if (parameters->_leftRays > 0)
	{
		ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::AERIAL_LINEAR_ZIGZAG_LIDAR);

		// PREPARE LiDAR FLOW
		unsigned threadOffset = (parameters->_numRays * LiDARParams->_raysPulse - parameters->_leftRays) / LiDARParams->_raysPulse;
		parameters->_currentNumRays = std::min(parameters->_allowedRaysIteration, parameters->_leftRays);

		shader->bindBuffers(std::vector<GLuint> { parameters->_rayBuffer, parameters->_waypointBuffer, parameters->_noiseBuffer });
		shader->use();
		shader->setUniform("heightJittering", LiDARParams->_alsHeightJittering);
		shader->setUniform("incrementRadians", parameters->_incrementRadians);
		shader->setUniform("noiseBufferSize", NOISE_BUFFER_SIZE);
		shader->setUniform("numPulses", parameters->_numPulsesScan);
		shader->setUniform("numThreads", parameters->_currentNumRays);
		shader->setUniform("pathLength", parameters->_pathLength);
		shader->setUniform("pulseRadius", LiDARParams->_pulseRadius);
		shader->setUniform("rayJittering", LiDARParams->_alsRayJittering);
		shader->setUniform("raysPulse", unsigned(LiDARParams->_raysPulse));
		shader->setUniform("startRadians", parameters->_startRadians);
		shader->setUniform("threadOffset", threadOffset);
		shader->setUniform("zigzag", GLuint(0));
		shader->execute(parameters->_numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		parameters->_leftRays -= parameters->_currentNumRays;
	}
}

void AerialLinearBuilder::throwRays(ALSParameters* parameters, LiDARParameters* LiDARParams, std::vector<Model3D::RayGPUData>& rays, const vec3& LiDARPosition, const vec3& LiDARDirection,
									const float startAngle, const float fov, const unsigned numPulses, unsigned baseIndex)
{
	vec3 normalizedDirection = glm::normalize(LiDARDirection);
	vec3 rotateAxis = vec3(-normalizedDirection.z, 0.0f, normalizedDirection.x);

	#pragma omp parallel for
	for (int rayIdx = 0; rayIdx < numPulses; ++rayIdx)
	{
		float angle = parameters->_incrementRadians * rayIdx + parameters->_startRadians;
		vec3 spherePosition = rotateAxis * -std::sin(angle);
		spherePosition.x += RandomUtilities::getUniformRandomValue() * LiDARParams->_alsRayJittering;
		spherePosition.y = -std::cos(angle) + RandomUtilities::getUniformRandomValue() * LiDARParams->_alsRayJittering;
		spherePosition.z += RandomUtilities::getUniformRandomValue() * LiDARParams->_alsRayJittering;
		vec3 sensorPosition = LiDARPosition + vec3(.0f, RandomUtilities::getUniformRandomValue() * LiDARParams->_alsHeightJittering, .0f);
		
		rays[baseIndex + rayIdx * LiDARParams->_raysPulse] = Model3D::RayGPUData(sensorPosition, sensorPosition + spherePosition);
		this->addPulseRadius(rays, baseIndex + rayIdx * LiDARParams->_raysPulse, parameters->_upVector, LiDARParams->_raysPulse, LiDARParams->_pulseRadius);
	}
}