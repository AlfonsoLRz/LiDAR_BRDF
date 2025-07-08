#version 450

#extension GL_ARB_compute_variable_group_size: enable
layout (local_size_variable) in;
								
#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>
#include <Assets/Shaders/Compute/Templates/random.glsl>

#define TERRAIN_MASK		1 << 0
#define WATER_MASK			1 << 1

#define HORIZONTAL_TERRAIN_ERROR_W		1.0f / 1000.0f
#define VERTICAL_TERRAIN_ERROR_HEIGHT_W 1e-4
#define VERTICAL_TERRAIN_ERROR_ANGLE_W	.5f

#define SHINY_DISTANCE_WEIGHT	1.0f / 10.0f
#define SHINY_MODEL_WEIGHT		1.0f / 50.0f
#define SHINY_INDIVIDUAL_ERROR	1.0f / 60.0f

#define DISTANCE_NOISE_OFFSET	0x456823
#define HORIZONTAL_AXIS_OFFSET  uvec2(0x45623, 0x7652FA)
#define MODEL_COMP_NOISE_OFFSET 0xAC987
#define POINT_NOISE_OFFSET		0xAC666
#define TERRAIN_NOISE_OFFSET	uvec2(0x56789, 0x65432)


layout (std430, binding = 0) buffer VertexBuffer	{ VertexGPUData				vertexData[]; };
layout (std430, binding = 1) buffer FaceBuffer		{ FaceGPUData				faceData[]; };
layout (std430, binding = 2) buffer MeshDataBuffer	{ MeshGPUData				meshData[]; };
layout (std430, binding = 3) buffer MaterialBuffer	{ MaterialGPUData			materialData[]; };
layout (std430, binding = 4) buffer RayBuffer		{ RayGPUData				rayData[]; };
layout (std430, binding = 5) buffer TemporalBuffer	{ TriangleCollisionGPUData	rayCollision[]; };
layout (std430, binding = 6) buffer NoiseBuffer		{ float						noiseBuffer[]; };

uniform	uint		inducedTerrainError;
uniform float		maxDistance;
uniform vec2		maxDistanceBoundary;
uniform uint		maxReturns;
uniform uint		noiseBufferSize;
uniform uint		numPulses;
uniform uint		numRaysPulse;
uniform uint		shinySurfaceError;
uniform uint		waveLength;


// Calculates new direction for ray once it has collided
vec3 computeRayDirection(uint rayIndex, uint collisionIndex, bool isSurfaceWater)
{
	const uint materialID	= meshData[rayCollision[collisionIndex].modelCompID].materialID;
	vec3 refraction			= normalize(refract(rayData[rayIndex].direction, rayCollision[collisionIndex].normal, materialData[materialID].refractiveIndex));

	return rayData[rayIndex].direction * (1.0f - float(isSurfaceWater)) + refraction * float(isSurfaceWater);
}

float getWhiteNoise(const uint index, const uint offset)
{
	return noiseBuffer[(index + offset) % noiseBufferSize];
}

vec3 translateShinySurface(uint rayIndex, uint collisionIndex)
{
	const float materialShininess	= meshData[rayCollision[collisionIndex].modelCompID].shininess;
	const float modelCompRandom		= getWhiteNoise(rayCollision[collisionIndex].modelCompID, MODEL_COMP_NOISE_OFFSET) * SHINY_MODEL_WEIGHT;
	const float pointRandom			= getWhiteNoise(rayIndex, POINT_NOISE_OFFSET) * SHINY_INDIVIDUAL_ERROR;
	const float isShiny				= float(materialShininess > EPSILON);

	return rayData[rayIndex].direction * materialShininess * rayCollision[collisionIndex].distance * SHINY_DISTANCE_WEIGHT + rayData[rayIndex].direction * (modelCompRandom + pointRandom) * isShiny;
}

vec3 translateTerrain(uint rayIndex, uint collisionIndex)
{
	const vec3 position			= rayCollision[collisionIndex].point;
	const float height			= rayData[rayIndex].origin.y - position.y;
	const float angle			= rayCollision[collisionIndex].angle;
	const float verticalNoise	= getWhiteNoise(rayIndex, TERRAIN_NOISE_OFFSET.x);
	const float horizontalNoise = getWhiteNoise(rayIndex, TERRAIN_NOISE_OFFSET.y);
	const float verticalError	= verticalNoise * (VERTICAL_TERRAIN_ERROR_HEIGHT_W * height + VERTICAL_TERRAIN_ERROR_ANGLE_W * angle);
	const float horizontalError = horizontalNoise * HORIZONTAL_TERRAIN_ERROR_W * height;
	const vec3 horizontalAxis	= vec3(getWhiteNoise(rayIndex, HORIZONTAL_AXIS_OFFSET.x), .0f, getWhiteNoise(rayIndex, HORIZONTAL_AXIS_OFFSET.y));

	return vec3(.0f, 1.0f, .0f) * verticalError + horizontalAxis * horizontalError;
}


void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numPulses) return;

	uint collisionIndex = index * numRaysPulse, rayIndex = rayCollision[collisionIndex].rayIndex;
	bool continueRay	= false, validCollision = false;
	bool bathymetric	= false, isWater = false, isTerrain = false, exceedReturns = false;
	float distanceNoise, noisyMaxRange;
	
	if (rayIndex != UINT_MAX)
	{
		// Can the ray follow its path to impact another surface? ----------
		isWater			= (meshData[rayCollision[collisionIndex].modelCompID].surface & WATER_MASK) != 0;
		isTerrain		= (meshData[rayCollision[collisionIndex].modelCompID].surface & TERRAIN_MASK) != 0;
		exceedReturns	= (rayData[rayIndex].returnNumber + 1) >= maxReturns;
		continueRay		= !exceedReturns;
		distanceNoise	= getWhiteNoise(index, DISTANCE_NOISE_OFFSET);
		noisyMaxRange	= maxDistance + distanceNoise * (maxDistanceBoundary.y - maxDistanceBoundary.x) + maxDistanceBoundary.x;
		validCollision	= rayCollision[collisionIndex].distance < noisyMaxRange;

		if (validCollision)
		{
			rayCollision[collisionIndex].intensity			= rayData[rayCollision[collisionIndex].rayIndex].power;
			if (shinySurfaceError > EPSILON)				rayCollision[collisionIndex].point += translateShinySurface(rayIndex, collisionIndex);
			if (inducedTerrainError > EPSILON && isTerrain)	rayCollision[collisionIndex].point += translateTerrain(rayIndex, collisionIndex);

			for (int ray = 0; ray < numRaysPulse; ++ray)
			{
				rayData[collisionIndex + ray].returnNumber += 1;
			}
		}
		else
		{
			rayCollision[collisionIndex].faceIndex = UINT_MAX;
		}

		for (int ray = 0; ray < numRaysPulse; ++ray)
		{
			if (continueRay && rayData[collisionIndex + ray].power > EPSILON && rayData[collisionIndex + ray].lastCollisionIndex == UINT_MAX)
			{
				rayData[collisionIndex + ray].direction			= computeRayDirection(collisionIndex + ray, collisionIndex, isWater);
				rayData[collisionIndex + ray].destination		= rayData[collisionIndex + ray].origin + rayData[collisionIndex + ray].direction;
				rayData[collisionIndex + ray].refractiveIndex	= materialData[meshData[rayCollision[collisionIndex].modelCompID].materialID].refractiveIndex;
			}
			else
			{
				rayData[collisionIndex + ray].power = .0f;
			}
		}
	}
}