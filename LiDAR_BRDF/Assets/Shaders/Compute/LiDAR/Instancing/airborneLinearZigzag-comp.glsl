#version 450

#extension GL_ARB_compute_variable_group_size: enable
layout (local_size_variable) in;

#define HEIGHT_NOISE_OFFSET 0xAC987
#define PULSE_NOISE_OFFSET uvec2(0x66565, 0x23456)
#define RAY_NOISE_OFFSET uvec3(0xFF245, 0x23456, 0xFFFF289)
#define UP_VECTOR vec3(.0, -1.0f, .0f)
	
#include <Assets/Shaders/Compute/Templates/computeAxes.glsl>
#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>
#include <Assets/Shaders/Compute/Templates/random.glsl>
#include <Assets/Shaders/Compute/Templates/rotation.glsl>

layout (std430, binding = 0) buffer RayBuffer			{ RayGPUData rayData[]; };
layout (std430, binding = 1) buffer WaypointBuffer		{ vec4 waypoints[]; };
layout (std430, binding = 2) buffer NoiseBuffer			{ float noiseBuffer[]; };

uniform float	heightJittering;
uniform float	incrementRadians;
uniform uint	noiseBufferSize;
uniform uint	numPulses;
uniform uint	numThreads;
uniform uint	offset;
uniform uint	pathLength;
uniform float	pulseRadius;
uniform uint	raysPulse;
uniform float	rayJittering;
uniform float	startRadians;
uniform uint	threadOffset;
uniform uint	zigzag;


float getUniformRandomValue(const uint index, const uint offset)
{
	return noiseBuffer[(index + offset) % noiseBufferSize];
}

void main()
{
	uint index = gl_GlobalInvocationID.x;
	if (index >= numThreads) return;

	const uint baseIndex = index * raysPulse;
	index += threadOffset;			// Iterative generation

	const uint pathID = index / ((pathLength - 1) * numPulses);
	const uint scanID = index / numPulses;
	const uint waypointID = scanID % (pathLength - 1) + 1 + pathID * pathLength;
	const uint pulseID = index % numPulses;
	const float zigZagSign = zigzag == 1 ? float(scanID % 2 == 0) * 2.0f - 1.0f : 1;

	vec3 spherePosition, sensorPosition, waypointDirection = waypoints[waypointID].xyz - waypoints[waypointID - 1].xyz;
	const vec3 normalizedWaypointDirection = normalize(waypointDirection);
	const vec3 rotationAxis = vec3(-normalizedWaypointDirection.z, 0.0f, normalizedWaypointDirection.x);
	const float angle = zigZagSign * startRadians + zigZagSign * pulseID * incrementRadians;

	spherePosition = rotationAxis * -sin(angle);
	spherePosition += vec3(getUniformRandomValue(index, RAY_NOISE_OFFSET.x) * rayJittering,
						   -cos(angle) + getUniformRandomValue(index, RAY_NOISE_OFFSET.y) * rayJittering,
						   getUniformRandomValue(index, RAY_NOISE_OFFSET.z) * rayJittering);
	sensorPosition = waypoints[waypointID].xyz + vec3(.0f, getUniformRandomValue(index, HEIGHT_NOISE_OFFSET) * heightJittering, .0f) + waypointDirection / numPulses * pulseID;

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