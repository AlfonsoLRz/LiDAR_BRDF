#version 450

#extension GL_ARB_compute_variable_group_size: enable
layout (local_size_variable) in;
								
#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>

layout (std430, binding = 0) buffer RayBuffer	{ RayGPUData ray[]; };

uniform uint		numRays;
uniform uint		numRaysPulse;
uniform float		peakPower;

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numRays) return;

	// Ray initialization
	ray[index].returnNumber			= 0;
	//ray[index].refractiveIndex		= 1.0f;
	ray[index].power				= peakPower / float(numRaysPulse);
	ray[index].startingPoint		= ray[index].origin;
	ray[index].lastCollisionIndex	= UINT_MAX;
	ray[index].continueRay			= 1;
	ray[index].previousDirection	= ray[index].direction;
}