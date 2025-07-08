#version 450

#extension GL_ARB_compute_variable_group_size: enable
layout (local_size_variable) in;

#define DISTANCE_NOISE_OFFSET 0xFCBA23
#define NOISE_OFFSET 0x234578
								
#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>
#include <Assets/Shaders/Compute/Templates/random.glsl>

layout (std430, binding = 0) buffer RayBuffer		{ RayGPUData				rayData[]; };
layout (std430, binding = 1) buffer CollisionBuffer	{ TriangleCollisionGPUData	faceCollision[]; };
layout (std430, binding = 2) buffer NoiseBuffer		{ float						noiseBuffer[]; };
layout (std430, binding = 3) buffer CountBuffer		{ uint						numCollisions; };
layout (std430, binding = 4) buffer NewCountBuffer	{ uint						newCollisions; };

uniform uint		noiseBufferSize;
uniform vec2		noiseRange;
uniform float		noiseThreshold;
uniform uint		nullModelCompID;
uniform uint		numPulses;

#define NORMALIZE_NOISE(n) (n * (noiseRange.y - noiseRange.x) + noiseRange.x)

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= newCollisions) return;

	const uint rayIndex = faceCollision[numCollisions - newCollisions + index].rayIndex;

	if (faceCollision[index].rayIndex != UINT_MAX)
	{
		const vec3 rayDirection		= normalize(rayData[rayIndex].previousDirection);
		const vec3 collisionPoint	= faceCollision[numCollisions - newCollisions + index].point;
		const float maxDistance		= distance(rayData[rayIndex].startingPoint, collisionPoint);		// Max. parametric t
		const float noise			= noiseBuffer[(index + NOISE_OFFSET) % noiseBufferSize];

		if ((noise * 2.0f - 1.0f) > noiseThreshold)
		{
			const uint finalIndex	= atomicAdd(numCollisions, 1);
			const float noiseDist	= noiseBuffer[(index + DISTANCE_NOISE_OFFSET) % noiseBufferSize] * 1.5f;
			const float distance	= NORMALIZE_NOISE(noiseDist) * maxDistance;

			faceCollision[finalIndex].point				= rayData[rayIndex].startingPoint + rayDirection * distance;
			faceCollision[finalIndex].faceIndex			= UINT_MAX;
			faceCollision[finalIndex].normal			= faceCollision[finalIndex].tangent = vec3(.0f);
			faceCollision[finalIndex].textCoord			= vec2(.0f);
			faceCollision[finalIndex].modelCompID		= nullModelCompID;
			faceCollision[finalIndex].returnNumber		= 0;
			faceCollision[finalIndex].distance			= distance;
			faceCollision[finalIndex].angle				= .0f;
			faceCollision[finalIndex].numReturns		= 1;
			faceCollision[finalIndex].intensity			= .0f;
			faceCollision[finalIndex].previousCollision = UINT_MAX;
			faceCollision[finalIndex].rayIndex			= UINT_MAX;
		}
	}
}