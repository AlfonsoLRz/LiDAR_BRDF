#version 450

in float normalizedGPSTime;

layout(location = 0) out vec4 fColor;

void main() {
	fColor = vec4(vec3(normalizedGPSTime), 1.0f);
}