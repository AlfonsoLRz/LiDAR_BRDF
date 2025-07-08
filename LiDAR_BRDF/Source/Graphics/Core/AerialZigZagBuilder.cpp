#include "stdafx.h"
#include "AerialZigZagBuilder.h"

#include "Graphics/Core/ShaderList.h"

/// [Static attributes]

/// [Public methods]

void AerialZigZagBuilder::buildRays(LiDARParameters* LiDARParams, AABB& sceneAABB)
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

void AerialZigZagBuilder::initializeContext(LiDARParameters* LiDARParams, AABB& sceneAABB)
{
	_parameters = this->buildParameters(LiDARParams, sceneAABB);
}

/// [Protected methods]

RayBuilder::ALSParameters* AerialZigZagBuilder::buildParameters(LiDARParameters* LiDARParams, AABB& sceneAABB)
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
	params->_advancePulse	= params->_metersSec / params->_pulsesSec;
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

void AerialZigZagBuilder::buildRaysCPU(ALSParameters* parameters, LiDARParameters* LiDARParams, AABB& sceneAABB)
{
	// Jittering initialization
	RandomUtilities::initializeUniformDistribution(-1.0f, 1.0f);

	// Retrieve waypoints to emulate platform movement
	std::vector<vec4> waypoints;
	std::vector<Interpolation*> airbonePaths = this->getAirbonePaths(LiDARParams, parameters->_numSteps, sceneAABB, LiDARParams->_alsPosition.y);
	this->retrievePath(airbonePaths, waypoints, parameters->_advanceScan_t);

	// Iterate through each interpolation path
	float zigZagSign = 1.0f;
	unsigned pathLength = waypoints.size() / airbonePaths.size();		// Each interpolation produces the same number of points
	unsigned raysPerPathTimestamp = (pathLength - 1) * parameters->_numPulsesScan * LiDARParams->_raysPulse;
	std::vector<Model3D::RayGPUData> rays(airbonePaths.size() * raysPerPathTimestamp);

	for (int pathIdx = 0; pathIdx < airbonePaths.size(); ++pathIdx)
	{
		unsigned pointOffset = pathLength * pathIdx;

		for (int point = pointOffset + 1; point < pointOffset + pathLength; ++point)
		{
			unsigned baseIndex = pathIdx * raysPerPathTimestamp + (point - 1) * parameters->_numPulsesScan * LiDARParams->_raysPulse;
			this->throwRays(parameters, LiDARParams, rays, waypoints[point], parameters->_startRadians, parameters->_fovRadians, parameters->_numPulsesScan, zigZagSign, parameters->_advancePulse, baseIndex);
			zigZagSign *= -1.0f;
		}
	}

	parameters->_leftRays -= rays.size();

	for (Interpolation* interpolation : airbonePaths)
	{
		delete interpolation;
	}

	parameters->_rayBuffer = ComputeShader::setReadBuffer(rays, GL_DYNAMIC_DRAW);
}

void AerialZigZagBuilder::buildRaysGPU(ALSParameters* parameters, LiDARParameters* LiDARParams, AABB& sceneAABB)
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
		shader->setUniform("zigzag", GLuint(1));
		shader->execute(parameters->_numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		parameters->_leftRays -= parameters->_currentNumRays;
	}
}

void AerialZigZagBuilder::throwRays(ALSParameters* parameters, LiDARParameters* LiDARParams, std::vector<Model3D::RayGPUData>& rays, const vec3& LiDARPosition, const float startAngle, const float fov,
								    const unsigned numPulses, const float zigZagSign, const float advancePulse, unsigned baseIndex)
{
	vec3 spherePosition, sensorPosition;
	float baseAngle = zigZagSign * startAngle, angleIncrement = fov / numPulses;

	//#pragma omp parallel for
	for (int rayIdx = 0; rayIdx < numPulses; ++rayIdx)
	{
		float angle = baseAngle + zigZagSign * angleIncrement * rayIdx;

		spherePosition = vec3(RandomUtilities::getUniformRandomValue() * LiDARParams->_alsRayJittering, 
							  -std::cos(angle) + RandomUtilities::getUniformRandomValue() * LiDARParams->_alsRayJittering, 
							  -std::sin(angle) + RandomUtilities::getUniformRandomValue() * LiDARParams->_alsRayJittering);
		sensorPosition = LiDARPosition + vec3(advancePulse * rayIdx, RandomUtilities::getUniformRandomValue() * LiDARParams->_alsHeightJittering, .0f);

		rays[baseIndex + rayIdx * LiDARParams->_raysPulse] = Model3D::RayGPUData(sensorPosition, sensorPosition + spherePosition);
		this->addPulseRadius(rays, baseIndex + rayIdx * LiDARParams->_raysPulse, parameters->_upVector, LiDARParams->_raysPulse, LiDARParams->_pulseRadius);
	}
}