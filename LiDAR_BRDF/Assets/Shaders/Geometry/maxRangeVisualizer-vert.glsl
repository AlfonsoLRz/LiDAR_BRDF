#version 450

layout(location = 0) in vec4 vPosition;

// Matrices
uniform vec3 LiDARPosition;
uniform mat4 mModelViewProj;

void main()
{
	gl_PointSize = 10.0f;
	gl_Position = mModelViewProj * vec4(LiDARPosition, 1.0f);
}