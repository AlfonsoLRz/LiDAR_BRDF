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

#define SHINY_DISTANCE_WEIGHT	1.0f / 200.0f
#define SHINY_MODEL_WEIGHT		1.0f / 80.0f
#define SHINY_INDIVIDUAL_ERROR	1.0f / 100.0f

#define DISTANCE_NOISE_OFFSET	0x456823
#define HORIZONTAL_AXIS_OFFSET  uvec2(0x45623, 0x7652FA)
#define LOSS_NOISE_OFFSET		0x45632
#define MODEL_COMP_NOISE_OFFSET 0xAC987
#define POINT_NOISE_OFFSET		0xAC666
#define TERRAIN_NOISE_OFFSET	uvec2(0x56789, 0x65432)

layout(std430, binding = 0) buffer VertexBuffer				{ VertexGPUData				vertexData[]; };
layout(std430, binding = 1) buffer FaceBuffer				{ FaceGPUData				faceData[]; };
layout(std430, binding = 2) buffer MeshDataBuffer			{ MeshGPUData				meshData[]; };
layout(std430, binding = 3) buffer MaterialBuffer			{ MaterialGPUData			materialData[]; };
layout(std430, binding = 4) buffer RayBuffer				{ RayGPUData				rayData[]; };
layout(std430, binding = 5) buffer CollisionBuffer			{ TriangleCollisionGPUData  rayCollision[]; };
layout(std430, binding = 6) buffer NoiseBuffer				{ float						noiseBuffer[]; };
layout(std430, binding = 7) buffer FinalCollisionBuffer		{ TriangleCollisionGPUData	compactCollision[]; };
layout(std430, binding = 8) buffer CountBuffer				{ uint						numCollisions; };

uniform uint		bathymetric;
uniform	uint		inducedTerrainError;
uniform float		maxDistance;
uniform vec2		maxDistanceBoundary;
uniform uint		maxReturns;
uniform uint		noiseBufferSize;
uniform uint		numPulses;				
uniform uint		numRaysPulse;
uniform float		pulseRadius;
uniform vec3		sensorNormal;
uniform uint		shinySurfaceError;

// Loss Frequency
uniform float		lossAddCoefficient, lossMultCoefficient, lossPower, lossThreshold;


bool areTriangleContiguous(uint mesh1, uint mesh2, uint triangle1, uint triangle2)
{
	uvec3 indices1 = faceData[triangle1].vertices, indices2 = faceData[triangle2].vertices;
	
	return (mesh1 == mesh2) && 
		   ((indices1.x == indices2.x || indices1.x == indices2.y || indices1.x == indices2.z) ||
		    (indices1.y == indices2.x || indices1.y == indices2.y || indices1.y == indices2.z) ||
		    (indices1.z == indices2.x || indices1.z == indices2.y || indices1.z == indices2.z));
}

vec3 computeRayDirection(uint rayIndex, uint collisionIndex, bool isSurfaceWater)
{
	const uint materialID				= meshData[rayCollision[collisionIndex].modelCompID].materialID;
	vec3 refraction						= normalize(refract(rayData[rayIndex].direction, rayCollision[collisionIndex].normal, materialData[materialID].refractiveIndex));
	rayData[rayIndex].previousDirection = rayData[rayIndex].direction;

	if (isSurfaceWater) return refraction;

	return rayData[rayIndex].direction;
}

vec3 getBarycentricCoordinates(vec3 position, in VertexGPUData vertex1, in VertexGPUData vertex2, in VertexGPUData vertex3)
{
	const vec3 v0		= vertex2.position - vertex1.position, v1 = vertex3.position - vertex1.position, v2 = position - vertex1.position;
	const float d00		= dot(v0, v0);
	const float d01		= dot(v0, v1);
	const float d11		= dot(v1, v1);
	const float d20		= dot(v2, v0);
	const float d21		= dot(v2, v1);
	const float denom	= d00 * d11 - d01 * d01;
	const float v		= (d11 * d20 - d01 * d21) / denom;
	const float w		= (d00 * d21 - d01 * d20) / denom;
	const float u		= 1.0f - v - w;

	return vec3(u, v, w);
}

float getLossThreshold(const float ks)
{
	if (ks < lossThreshold) return .0f;

	return lossMultCoefficient * pow(ks + lossAddCoefficient, lossPower);
}

float getWhiteNoise(const uint index, const uint offset)
{
	return noiseBuffer[(index + offset) % noiseBufferSize];
}

void invalidateRay(uint index)
{
	rayData[index].continueRay = 0;
	rayData[index].lastCollisionIndex = UINT_MAX;
}

vec3 translateShinySurface(uint rayIndex, uint collisionIndex, float shininessFactor)
{
	const float modelCompRandom		= getWhiteNoise(rayCollision[collisionIndex].modelCompID, MODEL_COMP_NOISE_OFFSET) * SHINY_MODEL_WEIGHT;
	const float pointRandom			= getWhiteNoise(rayIndex, POINT_NOISE_OFFSET) * SHINY_INDIVIDUAL_ERROR;
	const float isShiny				= float(shininessFactor > EPSILON);

	return rayData[rayIndex].direction * shininessFactor * shininessFactor * rayCollision[collisionIndex].distance * SHINY_DISTANCE_WEIGHT + rayData[rayIndex].direction * (modelCompRandom + pointRandom) * shininessFactor;
}

vec3 translateTerrain(uint rayIndex, uint collisionIndex)
{
	const vec3 position			= rayCollision[collisionIndex].point;
	const float height			= rayData[rayIndex].startingPoint.y - position.y;
	const float angle			= rayCollision[collisionIndex].angle;
	const float verticalNoise	= getWhiteNoise(rayIndex, TERRAIN_NOISE_OFFSET.x);
	const float horizontalNoise = getWhiteNoise(rayIndex, TERRAIN_NOISE_OFFSET.y);
	const float verticalError	= verticalNoise * (VERTICAL_TERRAIN_ERROR_HEIGHT_W * height + VERTICAL_TERRAIN_ERROR_ANGLE_W * angle);
	const float horizontalError = horizontalNoise * HORIZONTAL_TERRAIN_ERROR_W * height;
	const vec3 horizontalAxis	= vec3(getWhiteNoise(rayIndex, HORIZONTAL_AXIS_OFFSET.x), .0f, getWhiteNoise(rayIndex, HORIZONTAL_AXIS_OFFSET.y));

	return vec3(.0f, 1.0f, .0f) * verticalError + horizontalAxis * horizontalError;
}

bool validateCollision(const uint collisionIndex, const uint rayIndex)
{
	const bool isWater			= (meshData[rayCollision[collisionIndex].modelCompID].surface & WATER_MASK) != 0;
	const bool isTerrain		= (meshData[rayCollision[collisionIndex].modelCompID].surface & TERRAIN_MASK) != 0;
	const bool exceedReturns	= (rayData[rayIndex].returnNumber + 1) >= maxReturns;
	const float distanceNoise	= getWhiteNoise(rayIndex, DISTANCE_NOISE_OFFSET);
	const float noisyMaxRange	= maxDistance + distanceNoise * (maxDistanceBoundary.y - maxDistanceBoundary.x) + maxDistanceBoundary.x;
	VertexGPUData vertex		= vertexData[faceData[rayCollision[collisionIndex].faceIndex].vertices.x + meshData[rayCollision[collisionIndex].modelCompID].startIndex];
	const float shininessFactor = clamp(pow(vertex.ks, vertex.ns) * materialData[meshData[rayCollision[collisionIndex].modelCompID].materialID].roughness, .0f, 1.0f);
	const float lossThreshold	= getLossThreshold(shininessFactor);
	const bool isReturnLost		= getWhiteNoise(rayIndex, LOSS_NOISE_OFFSET) <= lossThreshold && bathymetric != 1;
	const bool validCollision	= rayCollision[collisionIndex].distance < noisyMaxRange && (!isWater || (isWater && rayCollision[collisionIndex].previousCollision == UINT_MAX)) && !isReturnLost;

	if (validCollision)
	{
		const uint finalIndex = atomicAdd(numCollisions, 1);

		if (shinySurfaceError > EPSILON)				rayCollision[collisionIndex].point += translateShinySurface(rayIndex, collisionIndex, 1.0f - shininessFactor);
		if (inducedTerrainError > EPSILON && isTerrain)	rayCollision[collisionIndex].point += translateTerrain(rayIndex, collisionIndex);

		compactCollision[finalIndex] = rayCollision[collisionIndex];

		for (int ray = 0; ray < numRaysPulse; ++ray)
		{
			if (!exceedReturns && (rayData[collisionIndex + ray].continueRay == 1 || (rayData[collisionIndex + ray].lastCollisionIndex != UINT_MAX && isWater && bathymetric == 1)))
			{
				rayData[collisionIndex + ray].continueRay		= 1;
				rayData[collisionIndex + ray].origin			= float(!isWater) * rayData[collisionIndex + ray].origin + float(isWater) * (rayCollision[collisionIndex + ray].point + rayData[collisionIndex + ray].direction * 0.0001f);
				rayData[collisionIndex + ray].direction			= computeRayDirection(collisionIndex + ray, collisionIndex, isWater);
			}
			else
			{
				rayData[collisionIndex + ray].continueRay = 0;
			}

			rayData[collisionIndex + ray].returnNumber		+= 1;
			rayData[collisionIndex + ray].lastCollisionIndex = finalIndex;
		}
	}

	return validCollision;
}

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numPulses)
	{
		return;
	}

	uint rayOffset			= index * numRaysPulse;
	uint minCollisionIndex	= UINT_MAX;
	uint collisionIndex		= 0;
	float minDistance		= UINT_MAX;

	for (int ray = 0; ray < numRaysPulse; ++ray)
	{
		collisionIndex = rayOffset + ray;

		if (rayCollision[collisionIndex].faceIndex != UINT_MAX && rayCollision[collisionIndex].distance < minDistance /**&& dot(-rayData[collisionIndex].direction, rayCollision[collisionIndex].normal) > .0f**/)
		{
			minCollisionIndex = collisionIndex;
			minDistance = rayCollision[collisionIndex].distance;
		}
	}

	if (minCollisionIndex != UINT_MAX)
	{
		float footprint = distance(rayData[minCollisionIndex].startingPoint, rayCollision[minCollisionIndex].point) * pulseRadius;
		float allowedRadius = 2.0f * footprint * (2.0f - abs(dot(rayCollision[minCollisionIndex].normal, -rayData[minCollisionIndex].direction)));
		uint lastCollisionIndex = rayData[minCollisionIndex].lastCollisionIndex;
		rayCollision[minCollisionIndex].numIntersectedRays = 0;

		for (int ray = 0; ray < numRaysPulse; ++ray)
		{
			collisionIndex = rayOffset + ray;

			if (rayCollision[collisionIndex].faceIndex != UINT_MAX)
			{
				uint isSameCollision = uint(distance(rayCollision[minCollisionIndex].point, rayCollision[collisionIndex].point) < allowedRadius
										   || rayCollision[minCollisionIndex].faceIndex == rayCollision[collisionIndex].faceIndex
									       || areTriangleContiguous(rayCollision[minCollisionIndex].modelCompID, rayCollision[collisionIndex].modelCompID, rayCollision[minCollisionIndex].faceIndex, rayCollision[collisionIndex].faceIndex));
				rayData[collisionIndex].continueRay			= 1 - isSameCollision;
				rayData[collisionIndex].lastCollisionIndex	= collisionIndex;
				rayCollision[minCollisionIndex].numIntersectedRays += isSameCollision;
			}
			else
			{
				invalidateRay(collisionIndex);
			}
		}

		vec3 normalizedDirection = normalize(-rayData[minCollisionIndex].direction);

		rayCollision[rayOffset]						= rayCollision[minCollisionIndex];
		rayCollision[rayOffset].previousCollision	= lastCollisionIndex;
		rayCollision[rayOffset].rayIndex			= minCollisionIndex;
		rayCollision[rayOffset].returnNumber		= rayData[minCollisionIndex].returnNumber;
		rayCollision[rayOffset].angle				= clamp(acos(dot(normalizedDirection * sensorNormal, normalizedDirection)), -PI / 2.0f, PI / 2.0f) / (PI / 2.0f) * 90.0f;
		rayCollision[rayOffset].distance			= distance(rayData[minCollisionIndex].startingPoint, rayCollision[rayOffset].point);
		rayCollision[rayOffset].gpsTime				= rayData[minCollisionIndex].gpsTime + (rayCollision[rayOffset].distance * 2.0f) / 299792458.0f;
		
		validateCollision(rayOffset, minCollisionIndex);
	}
	else
	{
		for (int ray = 0; ray < numRaysPulse; ++ray)
		{
			invalidateRay(rayOffset + ray);
		}
	}
}