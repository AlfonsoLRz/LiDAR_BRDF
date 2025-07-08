#version 450

#extension GL_ARB_compute_variable_group_size: enable
layout (local_size_variable) in;								

layout (std430, binding = 0) buffer WaypointBuffer	{ vec4 waypoints[]; };
layout (std430, binding = 1) buffer ResultBuffer	{ vec4 point[]; };

uniform uint numWaypoints;
uniform uint numParametricSplits;

vec4 getBezierPoint(const vec4 p1, const vec4 p2, const vec4 p3, t)
{
	float minus_t = 1.0f - t;

	return minus_t * minus_t * minus_t * p0 + 3.0f * minus_t * minus_t * t * p1 + 3.0f * minus_t * t * t * p2 + t * t * t * p3;
}

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= numParametricSplits)
	{
		return;
	}

	const float t = float(index) / numParametricSplits;
	point[index] = vec4(.0f);

	for (int pointIdx = 0; pointIdx < numWaypoints; ++pointIdx)
	{
		point[index] += float(nChooseK(numWaypoints - 1, pointIdx) * pow(1.0f - t, numWaypoints - 1 - pointIdx) * pow(t, pointIdx)) * waypoints[pointIdx];
	}

	point[index] = vec4(point[index].xyz, 1.0f);
}