#version 450

layout (location = 0) in vec4 vPosition;
layout (location = 7) in float vAngle;

uniform mat4	mModelViewProj;
uniform float	pointSize;

out float angle;

void main() {
	angle = vAngle;

	gl_PointSize = pointSize;
	gl_Position = mModelViewProj * vPosition;
}