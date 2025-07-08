#version 450

in float reflectance;

layout (location = 0) out vec4 fColor;
layout (location = 1) out vec4 brightColor;

#define A 0.15f
#define B 0.50f
#define C 0.10f
#define D 0.20f
#define E 0.02f
#define F 0.30f
#define W 11.2f
#define WHITE ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F

uniform float	addition;
uniform float	multiply;

// Tone mapping
uniform uint	hdr;
uniform float	gamma;
uniform float	exposure;

// Boundaries
uniform sampler2D	texKadSampler;
uniform float	minReflectance, maxReflectance;

void main() {
	// Rounded points
	vec2 centerPointv = gl_PointCoord - 0.5f;
	if (dot(centerPointv, centerPointv) > 0.25f)		// Vector * vector = square module => we avoid square root
	{
		discard;										// Discarded because distance to center is bigger than 0.5
	}

	float expandedIntensity = reflectance * multiply + addition;

	// Uncharted 2 tone mapping
	if (hdr == 1)
	{
		expandedIntensity *= exposure;
		expandedIntensity = ((expandedIntensity * (A * expandedIntensity + C * B) + D * E) / (expandedIntensity * (A * expandedIntensity + B) + D * F)) - E / F;
		expandedIntensity /= WHITE;
		expandedIntensity = pow(expandedIntensity, 1.0f / gamma);
	}

	vec3 normIntensity = vec3(expandedIntensity);

	if (minReflectance > -1.0f)
		normIntensity = texture(texKadSampler, vec2(.5f, (expandedIntensity - minReflectance) / (maxReflectance - minReflectance))).rgb;

	fColor = vec4(normIntensity, 1.0f);
	brightColor = vec4(0.0f);
}