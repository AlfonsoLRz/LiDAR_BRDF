#version 450

in float angle;

layout(location = 0) out vec4 fColor;

void main() {
	fColor = vec4(vec3((angle + 90.0f) / 180.0f), 1.0f);
}