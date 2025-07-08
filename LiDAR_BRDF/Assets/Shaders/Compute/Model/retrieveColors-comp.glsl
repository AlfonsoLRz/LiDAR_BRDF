#version 450

#extension GL_ARB_compute_variable_group_size: enable
layout (local_size_variable) in;

#include <Assets/Shaders/Compute/Templates/constraints.glsl>
#include <Assets/Shaders/Compute/Templates/modelStructs.glsl>

#define CUTOFF -1.0f

layout (std430, binding = 0) buffer InputBufferA	{ VertexGPUData vertexData[]; };

uniform uint		size;
uniform sampler2D	texKadSampler;
uniform sampler2D	texKsSampler, texNsSampler;
uniform sampler2D	texSemiTransparentSampler;

subroutine float alphaType(vec2 textCoord);
subroutine uniform alphaType alphaUniform;

subroutine(alphaType)
float alphaColor(vec2 textCoord)
{
	return float(texture(texSemiTransparentSampler, textCoord).x > 1.0f - EPSILON);
}

subroutine(alphaType)
float noAlphaColor(vec2 textCoord)
{
	return 1.0f;
}

void main()
{
	uint index = gl_GlobalInvocationID.x;
	if (index >= size) return;

	const vec4 rgba			= texture(texKadSampler, vertexData[index].textCoord);
	const float alpha		= min(alphaUniform(vertexData[index].textCoord), float(rgba.w > CUTOFF));

	vertexData[index].kad	= vec4(rgba.xyz, alpha);
	//vertexData[index].ks	= texture(texKsSampler, vertexData[index].textCoord).r;
	//vertexData[index].ns	= texture(texNsSampler, vertexData[index].textCoord).r;
	vertexData[index].ks	= 1.0f;
	vertexData[index].ns	= 1.0f;
}