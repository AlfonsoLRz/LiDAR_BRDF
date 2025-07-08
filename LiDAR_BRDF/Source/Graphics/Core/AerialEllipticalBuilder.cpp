#include "stdafx.h"
#include "AerialEllipticalBuilder.h"

#include "Graphics/Core/ShaderList.h"


/// [Static attributes]

/// [Public methods]

void AerialEllipticalBuilder::buildRays(LiDARParameters* LiDARParams, AABB& sceneAABB)
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

void AerialEllipticalBuilder::initializeContext(LiDARParameters* LiDARParams, AABB& sceneAABB)
{
	_parameters = this->buildParameters(LiDARParams, sceneAABB);
}

/// [Protected methods]

RayBuilder::ALSParameters* AerialEllipticalBuilder::buildParameters(LiDARParameters* LiDARParams, AABB& sceneAABB)
{
	ALSParameters* params = new ALSParameters;
	float width;

	params->_numSteps = this->getNumSteps(LiDARParams, sceneAABB, width);
	params->_scansSec = LiDARParams->_alsScanFrequency;
	params->_pulsesSec = LiDARParams->_alsPulseFrequency;
	params->_metersSec = LiDARParams->_alsSpeed;
	params->_numPulsesScan = params->_pulsesSec / params->_scansSec;
	params->_numScans = (sceneAABB.size().x + BOUNDARY_OFFSET * 2.0f) / params->_metersSec * params->_scansSec;
	params->_numPulses = (sceneAABB.size().x + BOUNDARY_OFFSET * 2.0f) / params->_metersSec * params->_pulsesSec;
	params->_heightRadius = 1.0f;
	params->_ellipseRadius = this->getRadius(LiDARParams->_alsFOVHorizontal, params->_heightRadius);
	params->_ellipseScale = LiDARParams->_alsFOVVertical;//LiDARParams->_alsFOVVertical * width / LiDARParams->_alsFOVHorizontal / 2.0f;
	params->_advancePulse = 1.0f / params->_numPulses;
	params->_incrementRadians = 2.0f * M_PI / (params->_numPulses / params->_numScans);
	params->_upVector = AERIAL_UP_VECTOR;

	// State params
	std::vector<vec4> waypoints;
	std::vector<Interpolation*> airbonePaths = this->getAirbonePaths(LiDARParams, params->_numSteps, sceneAABB, LiDARParams->_alsPosition.y);
	this->retrievePath(airbonePaths, waypoints, params->_advancePulse);

	params->_numThreads = waypoints.size() - airbonePaths.size();
	params->_pathLength = waypoints.size() / airbonePaths.size();		// Each interpolation produces the same number of points
	RayBuilder::initializeContext(LiDARParams, params);

	if (LiDARParams->_gpuInstantiation)
	{
		params->_waypointBuffer = ComputeShader::setReadBuffer(waypoints, GL_STATIC_DRAW);
	}

	return params;
}

void AerialEllipticalBuilder::buildRaysCPU(ALSParameters* parameters, LiDARParameters* LiDARParams, AABB& sceneAABB)
{
	// Jittering initialization
	RandomUtilities::initializeUniformDistribution(-1.0f, 1.0f);

	// Retrieve waypoints to emulate platform movement
	std::vector<vec4> waypoints;
	std::vector<Interpolation*> airbonePaths = this->getAirbonePaths(LiDARParams, parameters->_numSteps, sceneAABB, LiDARParams->_alsPosition.y);
	this->retrievePath(airbonePaths, waypoints, parameters->_advancePulse);

	// Iterate through each interpolation path
	unsigned pathLength = waypoints.size() / airbonePaths.size();		// Each interpolation produces the same number of points
	unsigned raysPerPathTimestamp = (pathLength - 1) * parameters->_numPulsesScan * LiDARParams->_raysPulse;
	std::vector<Model3D::RayGPUData> rays(airbonePaths.size() * raysPerPathTimestamp);

	for (int pathIdx = 0; pathIdx < airbonePaths.size(); ++pathIdx)
	{
		float baseAngle = RandomUtilities::getUniformRandomValue();
		vec4 spherePosition, sensorPosition;
		unsigned pointOffset = pathLength * pathIdx;

		#pragma omp parallel for
		for (int point = pointOffset + 1; point < pointOffset + pathLength; ++point)
		{
			unsigned baseIndex = pathIdx * raysPerPathTimestamp + (point - 1) * parameters->_numPulsesScan * LiDARParams->_raysPulse;
			float angle = baseAngle + (point - pointOffset - 1) * parameters->_incrementRadians;

			spherePosition = vec4(sin(angle), .0f, cos(angle), .0f) * parameters->_ellipseRadius;
			spherePosition.x *= parameters->_ellipseScale;
			spherePosition.x += RandomUtilities::getUniformRandomValue() * LiDARParams->_alsRayJittering;
			spherePosition.y = -parameters->_heightRadius + RandomUtilities::getUniformRandomValue() * LiDARParams->_alsRayJittering;
			spherePosition.z += RandomUtilities::getUniformRandomValue() * LiDARParams->_alsRayJittering;
			sensorPosition = waypoints[point] + vec4(.0f, RandomUtilities::getUniformRandomValue() * LiDARParams->_alsHeightJittering, .0f, .0f);
			
			rays[baseIndex] = Model3D::RayGPUData(sensorPosition, sensorPosition + spherePosition);

			this->addPulseRadius(rays, baseIndex, parameters->_upVector, LiDARParams->_raysPulse, LiDARParams->_pulseRadius);
		}
	}

	parameters->_leftRays -= rays.size();

	for (Interpolation* interpolation : airbonePaths)
	{
		delete interpolation;
	}

	parameters->_rayBuffer = ComputeShader::setReadBuffer(rays, GL_DYNAMIC_DRAW);
}

void AerialEllipticalBuilder::buildRaysGPU(ALSParameters* parameters, LiDARParameters* LiDARParams, AABB& sceneAABB)
{
	if (parameters->_leftRays > 0)
	{
		ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::AERIAL_ELLIPTICAL_LIDAR);

		// PREPARE LiDAR FLOW
		unsigned threadOffset = (parameters->_numRays * LiDARParams->_raysPulse - parameters->_leftRays) / LiDARParams->_raysPulse;
		parameters->_currentNumRays = std::min(parameters->_allowedRaysIteration, parameters->_leftRays);

		shader->bindBuffers(std::vector<GLuint> { parameters->_rayBuffer, parameters->_waypointBuffer, parameters->_noiseBuffer });
		shader->use();
		shader->setUniform("ellipseRadius", parameters->_ellipseRadius);
		shader->setUniform("ellipseScale", parameters->_ellipseScale);
		shader->setUniform("heightJittering", LiDARParams->_alsHeightJittering);
		shader->setUniform("heightRadius", parameters->_heightRadius);
		shader->setUniform("incrementRadians", parameters->_incrementRadians);
		shader->setUniform("noiseBufferSize", NOISE_BUFFER_SIZE);
		shader->setUniform("numPulses", parameters->_numPulsesScan);
		shader->setUniform("numThreads", parameters->_currentNumRays);
		shader->setUniform("pathLength", parameters->_pathLength);
		shader->setUniform("pulseRadius", LiDARParams->_pulseRadius);
		shader->setUniform("rayJittering", LiDARParams->_alsRayJittering);
		shader->setUniform("raysPulse", unsigned(LiDARParams->_raysPulse));
		shader->setUniform("threadOffset", threadOffset);
		shader->execute(parameters->_numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		parameters->_leftRays -= parameters->_currentNumRays;
	}
}