#version 450

layout (location = 0) in vec4 vPosition;
layout (location = 9) in float gpsTime;

uniform float	maxGPSTime;
uniform mat4	mModelViewProj;
uniform float	pointSize;

out float normalizedGPSTime;

void main() {
	normalizedGPSTime = gpsTime / maxGPSTime;

	gl_PointSize = pointSize;
	gl_Position = mModelViewProj * vPosition;
}