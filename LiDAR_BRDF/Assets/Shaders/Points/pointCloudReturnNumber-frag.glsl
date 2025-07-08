#version 450

#define EPSILON 0.000001f
#define	MAX_RETURNS 10
#define OFFSET 1.0 / MAX_RETURNS / 2.0f								// Put texture coordinate in the middle of a new color, instead of borders

in float			returnNumber;
in float			returnDivision;

uniform float		percentageBoundary;
uniform uint		lastRenderedReturn;
uniform sampler2D	texColorSampler;

layout (location = 0) out vec4 fColor;


void main() {
	// Rounded points
	vec2 centerPointv = gl_PointCoord - 0.5f;
	if (dot(centerPointv, centerPointv) > 0.25f)		// Vector * vector = square module => we avoid square root
	{
		discard;										// Discarded because distance to center is bigger than 0.5
	}

	if (returnNumber < lastRenderedReturn || returnDivision < percentageBoundary - EPSILON)
	{
		discard;
	}
			
	const vec3 color = texture(texColorSampler, vec2(returnNumber / MAX_RETURNS + OFFSET, .5f)).rgb;

	fColor = vec4(color, 1.0f);
}