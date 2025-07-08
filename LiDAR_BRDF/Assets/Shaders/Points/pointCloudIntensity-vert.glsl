#version 450

layout (location = 0) in vec4 vPosition;
layout (location = 4) in float vReflectance;

uniform mat4	mModelViewProj;
uniform float	pointSize;

out float reflectance;

void main() {
	reflectance = vReflectance;

	gl_PointSize = pointSize;
	gl_Position = mModelViewProj * vPosition;
}