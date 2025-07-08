#version 450

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec3 vNormal;

uniform mat4 mModelViewProj;
uniform float pointSize;

out vec3 normal;

void main() {
	normal = (vNormal + vec3(1.0f)) / 2.0f;
	gl_PointSize = pointSize;
	gl_Position = mModelViewProj * vPosition;
}