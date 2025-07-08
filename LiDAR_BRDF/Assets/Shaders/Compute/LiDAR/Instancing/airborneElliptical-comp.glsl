#version 450

#extension GL_ARB_compute_variable_group_size: enable
layout (local_size_variable) in;

#define HEIGHT_NOISE_OFFSET 0xAC987
#define X_NOISE_OFFSET 0xDC967
#define Z_NOISE_OFFSET 0x1C943
#define PULSE_NOISE_OFFSET uvec2(0x66565, 0x23456)
#define RAY_NOISE_OFFSET uvec3(0xFF245, 0x23456, 0xFFFF289)
#define UP_VECTOR vec3(.0f, -1.0f, .0f)
	
#include <Assets/Shaders/Compute/Templates/computeAxes.glsl>
#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>
#include <Assets/Shaders/Compute/Templates/random.glsl>
#include <Assets/Shaders/Compute/Templates/rotation.glsl>

layout (std430, binding = 0) buffer RayBuffer			{ RayGPUData rayData[]; };
layout (std430, binding = 1) buffer WaypointBuffer		{ vec4 waypoints[]; };
layout (std430, binding = 2) buffer NoiseBuffer			{ float noiseBuffer[]; };

uniform float	ellipseRadius;
uniform float	ellipseScale;
uniform float	heightJittering;
uniform float	heightRadius;
uniform float	incrementRadians;
uniform uint	noiseBufferSize;
uniform uint	numPulses;
uniform uint	numThreads;
uniform uint	pathLength;
uniform float	pulseIncrement;
uniform float	pulseRadius;
uniform float	rayJittering;
uniform uint	raysPulse;
uniform uint	threadOffset;


float getUniformRandomValue(const uint index, const uint offset)
{
	return noiseBuffer[(index + offset) % noiseBufferSize];
}

void main()
{
	uint index = gl_GlobalInvocationID.x;
	if (index >= numThreads) return;
	index += threadOffset;			// Iterative generation

	const uint pathID = index / (pathLength - 1);
	const uint pulseID = index % (pathLength - 1);
	const uint waypointID = pathID * pathLength + pulseID + 1;
	const uint baseIndex = index * raysPulse;

	vec3 spherePosition, sensorPosition, normalizedDirection = normalize(waypoints[waypointID].xyz - waypoints[waypointID - 1].xyz);
	const float angle = incrementRadians * pulseID;

	spherePosition = vec3(sin(angle), .0f, cos(angle)) * ellipseRadius;
	spherePosition.x *= ellipseScale;
	spherePosition += vec3(getUniformRandomValue(index, RAY_NOISE_OFFSET.x) * rayJittering,
						   -heightRadius + getUniformRandomValue(index, RAY_NOISE_OFFSET.y) * rayJittering,
						   getUniformRandomValue(index, RAY_NOISE_OFFSET.z) * rayJittering);
	sensorPosition = waypoints[waypointID].xyz + 
					vec3(.0f, getUniformRandomValue(index, HEIGHT_NOISE_OFFSET) * heightJittering, .0f);

	rayData[baseIndex].origin = sensorPosition;
	rayData[baseIndex].destination = sensorPosition + spherePosition;
	rayData[baseIndex].direction = normalize(rayData[baseIndex].destination - rayData[baseIndex].origin);

	{
		vec3 u, v, pulseNoise;
		getRadiusAxes(rayData[baseIndex].direction, UP_VECTOR, u, v);

		for (int ray = 1; ray < raysPulse; ++ray)
		{
			pulseNoise = getUniformRandomValue(index, PULSE_NOISE_OFFSET.x + ray) * pulseRadius * u + getUniformRandomValue(index, PULSE_NOISE_OFFSET.y + ray) * pulseRadius * v;
			rayData[baseIndex + ray].origin = rayData[baseIndex].origin + pulseNoise;
			rayData[baseIndex + ray].destination = rayData[baseIndex].destination + pulseNoise;
			rayData[baseIndex + ray].direction = normalize(rayData[baseIndex + ray].destination - rayData[baseIndex + ray].origin);
		}
	}
}