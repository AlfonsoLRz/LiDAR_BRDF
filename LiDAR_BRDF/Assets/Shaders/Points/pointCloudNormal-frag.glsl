#version 450

in vec3 normal;

layout (location = 0) out vec4 fColor;

void main() {
	// Rounded points
	vec2 centerPointv = gl_PointCoord - 0.5f;
	if (dot(centerPointv, centerPointv) > 0.25f)		// Vector * vector = square module => we avoid square root
	{
		discard;										// Discarded because distance to center is bigger than 0.5
	}

	fColor = vec4(normal, 1.0f);
}