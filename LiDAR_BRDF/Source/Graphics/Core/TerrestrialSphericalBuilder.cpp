#include "stdafx.h"
#include "TerrestrialSphericalBuilder.h"

#include "Graphics/Core/ComputeShader.h"
#include "Graphics/Core/ShaderList.h"

/// [Static attributes]

/// [Public methods]

void TerrestrialSphericalBuilder::buildRays(LiDARParameters* LiDARParams, AABB& sceneAABB)
{	
	TLSParameters* parameters = dynamic_cast<TLSParameters*>(_parameters);

	if (LiDARParams->_gpuInstantiation)
	{
		this->buildRaysGPU(parameters, LiDARParams, sceneAABB);
	}
	else
	{
		this->buildRaysCPU(parameters, LiDARParams, sceneAABB);
	}
}

unsigned TerrestrialSphericalBuilder::getNumSimulatedRays(LiDARParameters* LiDARParams, BuildingParameters* buildingParams)
{
	if (LiDARParams->_useSimulationTime)
	{
		return buildingParams->_leftRays * LiDARParams->_scanFrequencyHz * LiDARParams->_simulationTime;
	}
	else
	{
		return buildingParams->_leftRays;
	}
}

void TerrestrialSphericalBuilder::initializeContext(LiDARParameters* LiDARParams, AABB& sceneAABB)
{
	_parameters = this->buildParameters(LiDARParams, sceneAABB);
}

void TerrestrialSphericalBuilder::resetPendingRays(LiDARParameters* LiDARParams)
{
	RayBuilder::resetPendingRays(LiDARParams);

	_parameters->_leftRays = this->getNumSimulatedRays(LiDARParams, _parameters);
}

/// [Protected methods]

RayBuilder::TLSParameters* TerrestrialSphericalBuilder::buildParameters(LiDARParameters* LiDARParams, AABB& sceneAABB)
{
	TLSParameters* params = new TLSParameters;

	params->_numChannels	= LiDARParameters::Channels_UINT[LiDARParams->_channels];
	params->_verticalRes	= this->getVerticalResolution(LiDARParams, params);
	params->_numRays		= LiDARParams->_tlsResolutionHorizontal * params->_verticalRes;
	params->_startRadians	= LiDARParams->_tlsMiddleAngleHorizontal * M_PI / 180.0f;
	params->_fovRadians		= vec2(LiDARParams->_tlsFOVHorizontal * M_PI / 180.0f, LiDARParams->_tlsFOVVertical * M_PI / 180.0f);
	params->_fovRadians.y	+= params->_fovRadians.y / params->_verticalRes;
	params->_incrementRadians = params->_fovRadians / vec2(LiDARParams->_tlsResolutionHorizontal * params->_verticalRes, params->_verticalRes);
	params->_upVector		= TERRESTRIAL_UP_VECTOR;
	params->_startingAngleVertical = LiDARParams->_tlsMiddleAngleVertical * M_PI / 180.0f - params->_fovRadians.y / 2.0f;
	params->_timePulse		= (1.0f / LiDARParams->_alsScanFrequency) / static_cast<float>(params->_numRays);

	// State params
	this->precalculateVerticalAngles(LiDARParams, params);
	params->_numThreads = params->_numRays;
	RayBuilder::initializeContext(LiDARParams, params);

	if (LiDARParams->_gpuInstantiation)
	{
		std::vector<vec3> channelPosition;
		std::vector<vec4> channelPositionv4;
		this->getSensorPosition(channelPosition, params->_numChannels, LiDARParams->_tlsPosition);
		for (vec3& channelPos : channelPosition) channelPositionv4.push_back(vec4(channelPos, 1.0f));

		params->_channelBuffer = ComputeShader::setReadBuffer(channelPositionv4, GL_STATIC_DRAW);
		params->_vAngleBuffer = ComputeShader::setReadBuffer(params->_verticalAngleIncrement, GL_STATIC_DRAW);
	}

	return params;
}

void TerrestrialSphericalBuilder::buildRaysGPU(TLSParameters* parameters, LiDARParameters* LiDARParams, AABB& sceneAABB)
{
	if (parameters->_leftRays > 0)
	{
		ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::TERRESTRIAL_SPHERICAL_LIDAR);

		// PREPARE LiDAR FLOW
		unsigned threadOffset = (parameters->_numRays * LiDARParams->_raysPulse - parameters->_leftRays) / LiDARParams->_raysPulse;
		parameters->_currentNumRays = std::min(parameters->_allowedRaysIteration, parameters->_leftRays);

		shader->bindBuffers(std::vector<GLuint> { parameters->_channelBuffer, parameters->_rayBuffer, parameters->_noiseBuffer, parameters->_vAngleBuffer });
		shader->use();
		shader->setUniform("advance", LiDARParams->_tlsDirection / vec3(parameters->_numRays, 1.0f, parameters->_numRays));
		shader->setUniform("angleJittering", LiDARParams->_tlsAngleJittering);
		//shader->setUniform("axisJittering", LiDARParams->_tlsAxisJittering);
		shader->setUniform("fovRadians", parameters->_fovRadians);
		shader->setUniform("incrementRadians", parameters->_incrementRadians);
		shader->setUniform("noiseBufferSize", NOISE_BUFFER_SIZE);
		shader->setUniform("numThreads", parameters->_currentNumRays);
		shader->setUniform("numChannels", parameters->_numChannels);
		shader->setUniform("position", LiDARParams->_tlsPosition);
		shader->setUniform("pulseRadius", LiDARParams->_pulseRadius);
		shader->setUniform("raysPulse", unsigned(LiDARParams->_raysPulse));
		shader->setUniform("startRadians", parameters->_startRadians);
		shader->setUniform("threadOffset", threadOffset);
		shader->setUniform("timePulse", parameters->_timePulse);
		shader->setUniform("verticalRes", parameters->_verticalRes);
		shader->execute(parameters->_numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		parameters->_leftRays -= parameters->_currentNumRays;
	}
}

void TerrestrialSphericalBuilder::buildRaysCPU(TLSParameters* parameters, LiDARParameters* LiDARParams, AABB& sceneAABB)
{
	float horizontalAngle = -parameters->_fovRadians.x / 2.0f + parameters->_startRadians, horizontalTmp;
	unsigned verticalResChannel = unsigned(std::floor(parameters->_verticalRes / parameters->_numChannels));

	std::vector<Model3D::RayGPUData> rays (LiDARParams->_tlsResolutionHorizontal * parameters->_verticalRes * LiDARParams->_raysPulse);
	std::vector<vec3> channelPosition;
	this->getSensorPosition(channelPosition, parameters->_numChannels, LiDARParams->_tlsPosition);

	RandomUtilities::initializeUniformDistribution(-1.0f, 1.0f);

	#pragma omp parallel for
	for (int horizontalIdx = 0; horizontalIdx < LiDARParams->_tlsResolutionHorizontal; ++horizontalIdx)
	{
		for (int verticalIdx = 0; verticalIdx < parameters->_verticalRes; ++verticalIdx)
		{
			unsigned channel	= glm::clamp(verticalIdx / verticalResChannel, unsigned(0), parameters->_numChannels - 1);
			float verticalAngle = parameters->_verticalAngleIncrement[verticalIdx];

			horizontalTmp		= horizontalAngle + parameters->_incrementRadians.x * float(horizontalIdx * parameters->_verticalRes) + parameters->_incrementRadians.x * verticalIdx;
			vec3 spherePosition = vec3(std::cos(horizontalTmp), 0.0f, -std::sin(horizontalTmp));
			vec3 rotationAxis	= vec3(spherePosition.z, 0.0f, -spherePosition.x);
			vec3 noise			= vec3(RandomUtilities::getUniformRandomValue(), RandomUtilities::getUniformRandomValue(), RandomUtilities::getUniformRandomValue()) * LiDARParams->_tlsAxisJittering;
			mat4 noiseRotation	= (LiDARParams->_tlsAngleJittering > glm::epsilon<float>()/* && LiDARParams->_tlsAxisJittering > glm::epsilon<float>()*/) ? 
								   glm::rotate(mat4(1.0f), float(RandomUtilities::getUniformRandomValue() * LiDARParams->_tlsAngleJittering), noise) : mat4(1.0f);
			vec3 destination	= vec3(noiseRotation * glm::rotate(mat4(1.0f), verticalAngle, rotationAxis) * vec4(spherePosition, 1.0f));

			unsigned baseIndex	= horizontalIdx * parameters->_verticalRes * LiDARParams->_raysPulse + verticalIdx * LiDARParams->_raysPulse;
			rays[baseIndex]		= Model3D::RayGPUData(LiDARParams->_tlsPosition + channelPosition[channel], LiDARParams->_tlsPosition + channelPosition[channel] + destination);

			this->addPulseRadius(rays, baseIndex, parameters->_upVector, LiDARParams->_raysPulse, LiDARParams->_pulseRadius);
		}
	}

	parameters->_currentNumRays = parameters->_numRays * LiDARParams->_raysPulse;
	parameters->_leftRays = 0;
	parameters->_rayBuffer = ComputeShader::setReadBuffer(rays, GL_DYNAMIC_DRAW);
}

void TerrestrialSphericalBuilder::getSensorPosition(std::vector<vec3>& sensor, const unsigned numChannels, const vec3& origin)
{
	unsigned currentChannel = 0;

	while (currentChannel < numChannels)
	{
		sensor.push_back(vec3(.0f));
		++currentChannel;
	}
}

int TerrestrialSphericalBuilder::getVerticalResolution(LiDARParameters* LiDARParams, TLSParameters* tlsParams)
{
	if (LiDARParams->_tlsUniformVerticalResolution)
	{
		return LiDARParams->_tlsResolutionVertical;
	}

	int verticalRes = 0;
	for (LiDARParameters::RangeResolution& range : LiDARParams->_tlsRangeResolution)
	{
		verticalRes += range._resolution;
	}

	return verticalRes;
}

void TerrestrialSphericalBuilder::precalculateVerticalAngles(LiDARParameters* LiDARParams, TLSParameters* tlsParams)
{
	float angle = tlsParams->_startingAngleVertical;

	tlsParams->_verticalAngleIncrement.clear();

	if (LiDARParams->_tlsUniformVerticalResolution)
	{
		for (int verticalIdx = 0; verticalIdx < tlsParams->_verticalRes; ++verticalIdx)
		{
			angle = tlsParams->_startingAngleVertical + tlsParams->_incrementRadians.y * verticalIdx;
			tlsParams->_verticalAngleIncrement.push_back(angle);
			angle += tlsParams->_incrementRadians.y;
		}
	}
	else
	{
		for (LiDARParameters::RangeResolution& range : LiDARParams->_tlsRangeResolution)
		{
			float increment = glm::radians((range._rangeInterval.y - range._rangeInterval.x) / range._resolution);

			for (int divIdx = 0; divIdx < range._resolution; ++divIdx)
			{
				tlsParams->_verticalAngleIncrement.push_back(angle);
				angle += increment;
			}
		}
	}
}
