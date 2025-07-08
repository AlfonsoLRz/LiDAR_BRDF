#version 450

#extension GL_ARB_compute_variable_group_size: enable
layout (local_size_variable) in;
								
#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>
#include <Assets/Shaders/Compute/Templates/random.glsl>

#define WATER_MASK			1 << 1

layout (std430, binding = 0) buffer VertexBuffer	{ VertexGPUData				vertexData[]; };
layout (std430, binding = 1) buffer FaceBuffer		{ FaceGPUData				faceData[]; };
layout (std430, binding = 2) buffer MeshDataBuffer	{ MeshGPUData				meshData[]; };
layout (std430, binding = 3) buffer MaterialBuffer	{ MaterialGPUData			materialData[]; };
layout (std430, binding = 4) buffer RayBuffer		{ RayGPUData				rayData[]; };
layout (std430, binding = 5) buffer CollisionBuffer	{ TriangleCollisionGPUData	rayCollision[]; };
layout (std430, binding = 6) buffer CountBuffer		{ uint						numCollisions; };
layout (std430, binding = 7) buffer BRDFBuffer		{ float						brdfData[]; };
layout (std430, binding = 8) buffer HermiteBuffer	{ float						hermiteTensor[]; };

uniform float		atmosphericAttenuation;
uniform uint		bathymetric;
uniform uint		numRaysPulse;
uniform float		reflectanceWeight;
uniform float		sensorDiameter;
uniform float		systemAttenuation;
uniform float		waterHeight;

#include <Assets/Shaders/Compute/LiDAR/computeIntensity-comp.glsl>

float getRawInterpolation(uint materialID, float x, float y)
{
	return brdfData[materialID * 32760 + (int(x) % 360) * 91 + int(y)];
}

float getLinearInterpolation(uint materialID, float x, float y)
{
	float x_i, y_i, x_f = modf(x, x_i), y_f = modf(y, y_i);
	int x0 = int(x_i), y0 = int(y_i), x1 = (x0 + 1) % 360, y1 = clamp(y0 + 1, 0, 89);

	return	brdfData[materialID * 32760 + x0 * 91 + y0] * (1.0f - x_f) * (1.0f - y_f) +
			brdfData[materialID * 32760 + x1 * 91 + y0] * x_f * (1.0f - y_f) +
			brdfData[materialID * 32760 + x0 * 91 + y1] * (1.0f - x_f) * y_f +
			brdfData[materialID * 32760 + x1 * 91 + y1] * x_f * y_f;
}

float getHermiteInterpolation(uint materialID, float x, float y)
{
	float x_i, y_i, x_f = modf(x, x_i), y_f = modf(y, y_i);
	int x0 = int(x_i - 1) % 360, x1 = (x0 + 1) % 360, x2 = (x1 + 1) % 360, x3 = (x2 + 1) % 360;
	int y0 = clamp(int(y_i - 1), 0, 90), y1 = clamp(y0 + 1, 0, 90), y2 = clamp(y1 + 1, 0, 90), y3 = clamp(y2 + 1, 0, 90);

	float rx0 = brdfData[materialID * 32760 + x0 * 91 + y0], rx1 = brdfData[materialID * 32760 + x1 * 91 + y0],
		rx2 = brdfData[materialID * 32760 + x2 * 91 + y0], rx3 = brdfData[materialID * 32760 + x3 * 91 + y0];
	float ry0 = brdfData[materialID * 32760 + x0 * 91 + y0], ry1 = brdfData[materialID * 32760 + x0 * 91 + y1],
		ry2 = brdfData[materialID * 32760 + x0 * 91 + y2], ry3 = brdfData[materialID * 32760 + x0 * 91 + y3];

	float ax = rx0 * hermiteTensor[0] + rx1 * hermiteTensor[1] + rx2 * hermiteTensor[2] + rx3 * hermiteTensor[3];
	float bx = rx0 * hermiteTensor[4] + rx1 * hermiteTensor[5] + rx2 * hermiteTensor[6] + rx3 * hermiteTensor[7];
	float cx = rx0 * hermiteTensor[8] + rx1 * hermiteTensor[9] + rx2 * hermiteTensor[10] + rx3 * hermiteTensor[11];
	float dx = rx0 * hermiteTensor[12] + rx1 * hermiteTensor[13] + rx2 * hermiteTensor[14] + rx3 * hermiteTensor[15];

	float ay = ry0 * hermiteTensor[0] + ry1 * hermiteTensor[1] + ry2 * hermiteTensor[2] + ry3 * hermiteTensor[3];
	float by = ry0 * hermiteTensor[4] + ry1 * hermiteTensor[5] + ry2 * hermiteTensor[6] + ry3 * hermiteTensor[7];
	float cy = ry0 * hermiteTensor[8] + ry1 * hermiteTensor[9] + ry2 * hermiteTensor[10] + ry3 * hermiteTensor[11];
	float dy = ry0 * hermiteTensor[12] + ry1 * hermiteTensor[13] + ry2 * hermiteTensor[14] + ry3 * hermiteTensor[15];

	return (x_f * (x_f * (x_f * ax + bx) + cx) + dx) + (y_f * (y_f * (y_f * ay + by) + cy) + dy);
}

float reflectIrradiance(const uint index, const vec2 KdKs, const RayGPUData ray)
{
	const vec3 N = normalize(rayCollision[index].normal);
	const vec3 L = normalize(ray.origin - rayCollision[index].point);
	const float y_dot = abs(dot(L, N));

	uint materialID = meshData[rayCollision[index].modelCompID].materialID;
	float y = (abs(dot(L, N)) * PI / 2.0f) * 180.0f / PI, x = ((atan(L.z, L.x) + PI / 2.0f) * 2.0f) * 180.0f / PI;
	
	return clamp(getHermiteInterpolation(materialID, x, y), .0f, 1.0f);
}

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numCollisions) return;

	const float brdfFactor			= reflectIrradiance(index, vec2(1.0f), rayData[rayCollision[index].rayIndex]);
	const uint previousCollision	= rayCollision[index].previousCollision;

	if (previousCollision != UINT_MAX && (meshData[rayCollision[previousCollision].modelCompID].surface & WATER_MASK) != 0 && bathymetric == 1)
	{
		rayCollision[index].intensity = computeBathymetricIntensity(index, brdfFactor, rayData[rayCollision[index].rayIndex]);
	}
	else
	{
		rayCollision[index].intensity = computeIntensity(index, brdfFactor, rayData[rayCollision[index].rayIndex]);
	}
}