#version 450

#extension GL_ARB_compute_variable_group_size: enable
layout (local_size_variable) in;
								
#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>

layout (std430, binding = 0) buffer RayBuffer		{ RayGPUData				rayData[]; };
layout (std430, binding = 1) buffer CollisionBuffer	{ TriangleCollisionGPUData	faceCollision[]; };
layout (std430, binding = 2) buffer CountBuffer		{ uint						numCollisions; };

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numCollisions) return;

	const uint numReturns = rayData[faceCollision[index].rayIndex].returnNumber;
	uint collisionIndex = index;

	while (collisionIndex != UINT_MAX)
	{
		faceCollision[collisionIndex].numReturns = numReturns;
		collisionIndex = faceCollision[collisionIndex].previousCollision;
	}
}