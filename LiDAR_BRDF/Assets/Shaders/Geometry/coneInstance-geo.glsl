#version 450

layout(points) in;
layout(line_strip, max_vertices = 200) out;

#include <Assets/Shaders/Compute/Templates/constraints.glsl>

uniform vec3	aabbMax, aabbMin;
uniform vec3	beamNormal;
uniform float	coneRadius;
uniform vec3	LiDARPosition;
uniform mat4	mModelViewProj;
uniform uint	subdivisions;


bool equal(vec3 a, vec3 b)
{
	return (a.x - b.x < EPSILON&& a.y - b.y < EPSILON&& a.z - b.z < EPSILON);
}

// Find the intersection of a line from v0 to v1 and an axis-aligned bounding box http://www.youtube.com/watch?v=USjbg5QXk3g
float rayAABBIntersection(vec3 ro, vec3 rd)
{
	float low = -UINT_MAX;
	float high = UINT_MAX;

	for (int i = 0; i < 3; ++i) {
		float dimLow = (aabbMin[i] - ro[i]) / rd[i];
		float dimHigh = (aabbMax[i] - ro[i]) / rd[i];

		if (dimLow > dimHigh) {
			float tmp = dimLow;
			dimLow = dimHigh;
			dimHigh = tmp;
		}

		if (dimHigh < low || dimLow > high) {
			return UINT_MAX;
		}

		if (dimLow > low) low = dimLow;
		if (dimHigh < high) high = dimHigh;
	}

	return low > high ? UINT_MAX : high;
}

void main()
{
	// Ray-AABB collision test
	float rayDistance = rayAABBIntersection(LiDARPosition, normalize(beamNormal));

	if (rayDistance != UINT_MAX)
	{
		vec3 u, v;
		vec4 conePoint;
		float angle = .0f;
		bool finish = false;
		const vec4 origin = gl_in[0].gl_Position;
		const vec3 up = vec3(.0f, -1.0, .0f); // TODO
		const vec3 n = normalize(beamNormal);
		const float alphaIncrement = 2.0f * PI / subdivisions;

		if (equal(n, -up))		// x axis: UP x n is 0 as both vectors are parallel. Since up and n are normalized we can check if they are equal (with epsilon checkup)
		{
			u = normalize(cross(vec3(0.0f, 0.0f, -1.0f), n));
		}
		else if (equal(n, up))
		{
			u = normalize(cross(vec3(0.0f, 0.0f, 1.0f), n));
		}
		else
		{
			u = normalize(cross(up, n));
		}
		v = normalize(cross(n, u));					// y axis

		while (!finish)
		{
			conePoint = mModelViewProj * vec4(LiDARPosition + n * rayDistance + u * cos(angle) * coneRadius - v * sin(angle) * coneRadius, 1.0f);

			gl_Position = origin;
			EmitVertex();

			gl_Position = conePoint;
			EmitVertex();

			EndPrimitive();

			gl_Position = conePoint;
			EmitVertex();

			gl_Position = mModelViewProj * vec4(LiDARPosition + n * rayDistance + u * cos(angle + alphaIncrement) * coneRadius - v * sin(angle + alphaIncrement) * coneRadius, 1.0f);
			EmitVertex();

			EndPrimitive();

			finish = angle > 2.0 * PI;
			angle += alphaIncrement;
		}
	}
}