#version 450

layout(points) in;
layout(line_strip, max_vertices = 256) out;

#include <Assets/Shaders/Compute/Templates/constraints.glsl>

uniform vec3	aabbMax, aabbMin;
uniform vec3	beamNormal;
uniform float	coneRadius;
uniform vec3	LiDARPosition;
uniform mat4	mModelViewProj;
uniform uint	subdivisions;
uniform vec2	maxDistance;


bool equal(vec3 a, vec3 b)
{
	return (a.x - b.x < EPSILON&& a.y - b.y < EPSILON&& a.z - b.z < EPSILON);
}

void main()
{
	vec3 u = vec3(1.0f, .0f, .0f), v = vec3(.0f, .0f, 1.0f);
	vec4 conePoint;
	float angle = .0f;
	bool finish = false;
	const vec4 origin = gl_in[0].gl_Position;
	const float alphaIncrement = 2.0f * PI / subdivisions;
	float coneRadius = maxDistance.x;

	while (!finish)
	{
		gl_Position = mModelViewProj * vec4(LiDARPosition + u * cos(angle) * coneRadius - v * sin(angle) * coneRadius, 1.0f);
		EmitVertex();

		gl_Position = mModelViewProj * vec4(LiDARPosition + u * cos(angle + alphaIncrement) * coneRadius - v * sin(angle + alphaIncrement) * coneRadius, 1.0f);
		EmitVertex();

		EndPrimitive();

		finish = angle > 2.0 * PI;
		angle += alphaIncrement;
	}

	angle = .0f; finish = false;
	coneRadius = maxDistance.y;

	while (!finish)
	{
		gl_Position = mModelViewProj * vec4(LiDARPosition + u * cos(angle) * coneRadius - v * sin(angle) * coneRadius, 1.0f);
		EmitVertex();

		gl_Position = mModelViewProj * vec4(LiDARPosition + u * cos(angle + alphaIncrement) * coneRadius - v * sin(angle + alphaIncrement) * coneRadius, 1.0f);
		EmitVertex();

		EndPrimitive();

		finish = angle > 2.0 * PI;
		angle += alphaIncrement;
	}
}