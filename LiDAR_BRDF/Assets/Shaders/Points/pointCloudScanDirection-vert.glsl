#version 450

layout (location = 0) in vec4 vPosition;
layout (location = 8) in vec3 vScanDirection;

uniform mat4	mModelViewProj;
uniform float	pointSize;

out vec3 scanDirection;

void main() {
	scanDirection = (normalize(vScanDirection) + vec3(1.0f)) / 2.0f;

	gl_PointSize = pointSize;
	gl_Position = mModelViewProj * vPosition;
}