#version 450

layout (location = 0) in vec4	vPosition;
layout (location = 5) in float	vReturnNumber;
layout (location = 6) in float	vReturnDivision;

uniform mat4	mModelViewProj;
uniform float	pointSize;	

out float		returnNumber;
out float		returnDivision;

void main() {
	returnNumber	= vReturnNumber;
	returnDivision	= vReturnDivision;

	gl_PointSize	= pointSize;
	gl_Position		= mModelViewProj * vPosition;
}