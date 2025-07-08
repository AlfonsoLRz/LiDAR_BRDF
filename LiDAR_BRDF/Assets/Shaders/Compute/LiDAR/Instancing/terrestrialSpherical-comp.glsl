#version 450

#extension GL_ARB_compute_variable_group_size: enable
layout (local_size_variable) in;

#define AXIS_NOISE_OFFSET uvec3(0xFF245, 0x23456, 0xFFFF28)
#define ANGLE_NOISE_OFFSET 0xAC987
#define PULSE_NOISE_OFFSET uvec2(0x66565, 0x23456)
#define UP_VECTOR vec3(.0f, 1.0f, .0f)
		
#include <Assets/Shaders/Compute/Templates/computeAxes.glsl>
#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>
#include <Assets/Shaders/Compute/Templates/random.glsl>
#include <Assets/Shaders/Compute/Templates/rotation.glsl>

layout (std430, binding = 0) buffer ChannelPosBuffer	{ vec4 channelPosition[]; };
layout (std430, binding = 1) buffer RayBuffer			{ RayGPUData rayData[]; };
layout (std430, binding = 2) buffer NoiseBuffer			{ float noiseBuffer[]; };
layout (std430, binding = 3) buffer VerticalAngleBuffer { float verticalAngleIncrement[]; };

uniform vec3	advance;
uniform float	angleJittering;
uniform float	axisJittering;
uniform float	channelSpacing;
uniform vec2	fovRadians;
uniform vec2	incrementRadians;
uniform uint	noiseBufferSize;
uniform uint	numChannels;
uniform uint	numThreads;
uniform vec3	position;
uniform float	pulseRadius;
uniform uint	raysPulse;
uniform float	startRadians;
uniform float	startingAngleVertical;
uniform uint	threadOffset;
uniform float	timePulse;
uniform int		verticalRes;


float getUniformRandomValue(const uint index, const uint offset)
{
	return noiseBuffer[(index + offset) % noiseBufferSize];
}

void main()
{
	uint index = gl_GlobalInvocationID.x;
	if (index >= numThreads) return;

	const uint baseIndex		= index * raysPulse;
	index += threadOffset;			// Iterative generation

	const uint horizontalID			= index / (verticalRes);
	const uint verticalID			= index % verticalRes;
	const uint verticalResChannel	= uint(floor(verticalRes / numChannels));
	const uint channelID			= clamp(verticalID / verticalResChannel, 0, numChannels - 1);

	const float verticalAngle	= verticalAngleIncrement[verticalID];
	const float horizontalAngle = -fovRadians.x / 2.0f + startRadians + incrementRadians.x * float(horizontalID * verticalRes) + incrementRadians.x * verticalID;
	const vec3 spherePosition	= vec3(cos(horizontalAngle), 0.0f, -sin(horizontalAngle));
	const vec3 rotationAxis		= vec3(spherePosition.z, 0.0f, -spherePosition.x);
	const vec3 noise			= vec3(getUniformRandomValue(index, AXIS_NOISE_OFFSET.x),
									   getUniformRandomValue(index, AXIS_NOISE_OFFSET.y),
									   getUniformRandomValue(index, AXIS_NOISE_OFFSET.z));
	const vec3 destination		= vec3(rotation3d(noise, getUniformRandomValue(index, ANGLE_NOISE_OFFSET) * angleJittering) * rotation3d(rotationAxis, verticalAngle) * vec4(spherePosition, 1.0f));

	rayData[baseIndex].origin		= position + advance * index + vec3(.0f, channelPosition[channelID].y, .0f);
	rayData[baseIndex].destination	= rayData[baseIndex].origin + destination;
	rayData[baseIndex].direction	= normalize(rayData[baseIndex].destination - rayData[baseIndex].origin);
	rayData[baseIndex].gpsTime		= timePulse * (horizontalID * verticalRes + verticalID);

	{
		vec3 u, v, pulseNoise;
		getRadiusAxes(rayData[baseIndex].direction, UP_VECTOR, u, v);

		for (int ray = 1; ray < raysPulse; ++ray)
		{
			pulseNoise = getUniformRandomValue(index, PULSE_NOISE_OFFSET.x + ray) * pulseRadius * u + getUniformRandomValue(index, PULSE_NOISE_OFFSET.y + ray) * pulseRadius * v;
			rayData[baseIndex + ray].origin			= rayData[baseIndex].origin;
			rayData[baseIndex + ray].destination	= rayData[baseIndex].destination + pulseNoise;
			rayData[baseIndex + ray].direction		= normalize(rayData[baseIndex + ray].destination - rayData[baseIndex + ray].origin);
			rayData[baseIndex + ray].gpsTime		= rayData[baseIndex].gpsTime;
		}
	}
}