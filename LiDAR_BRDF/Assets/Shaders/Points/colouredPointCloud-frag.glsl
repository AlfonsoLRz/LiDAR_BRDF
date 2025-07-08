#version 450

// ********** PARAMETERS & VARIABLES ***********

// ------------ Constraints ------------
const float EPSILON = 0.0000001f;

// ------------ Vertex data --------------
in vec4 shadowCoord;

// ------------ Color --------------
in float attenuation;
in vec3 diffuseColor;
in vec3 specularColor;


// ---------------- Shadows ---------------

subroutine void depthTextureType(out float shadowDiffuseFactor, out float shadowSpecFactor);
subroutine uniform depthTextureType depthTextureUniform;

uniform float shadowMaxIntensity, shadowMinIntensity;						// Color range
uniform float shadowRadius;
uniform sampler3D texOffset;
uniform sampler2DShadow texShadowMapSampler;


layout (location = 0) out vec4 fColor;


// ----- Shadows -------

// Computes the fragment attenuation related to shadowing. For that purpose we must query a depth texture

subroutine(depthTextureType)
void shadow(out float shadowDiffuseFactor, out float shadowSpecFactor)
{
	ivec3 offsetCoord;
	vec3 offsetTexSize = vec3(64, 64, 32);
	offsetCoord.xy = ivec2(mod(gl_FragCoord.xy, offsetTexSize.xy));

	float sum = 0.0;
	int samplesDiv2 = int(offsetTexSize.z);
	vec4 sc = shadowCoord;
	shadowDiffuseFactor = 0.0f;

	for (int i = 0; i < 4; i++) {
		offsetCoord.z = i;

		vec4 offsets = texelFetch(texOffset, offsetCoord, 0) * shadowRadius * shadowCoord.w;
		sc.xy = shadowCoord.xy + offsets.xy;
		sum += textureProj(texShadowMapSampler, sc);
		sc.xy = shadowCoord.xy + offsets.zw;
		sum += textureProj(texShadowMapSampler, sc);
	}

	shadowDiffuseFactor = sum / 8.0f;
	if (shadowDiffuseFactor != 1.0f && shadowDiffuseFactor != 0.0f)
	{
		for (int i = 4; i < samplesDiv2; i++) {
			offsetCoord.z = i;

			vec4 offsets = texelFetch(texOffset, offsetCoord, 0) * shadowRadius * shadowCoord.w;
			sc.xy = shadowCoord.xy + offsets.xy;
			sum += textureProj(texShadowMapSampler, sc);
			sc.xy = shadowCoord.xy + offsets.zw;
			sum += textureProj(texShadowMapSampler, sc);
		}

		shadowDiffuseFactor = sum / float(samplesDiv2 * 2.0f);
	}

	shadowDiffuseFactor = shadowDiffuseFactor * (shadowMaxIntensity - shadowMinIntensity) + shadowMinIntensity;
	shadowSpecFactor = 1.0f;
}

subroutine(depthTextureType)
void noShadow(out float shadowDiffuseFactor, out float shadowSpecFactor)
{
	shadowDiffuseFactor = 1.0f;					// Color gets no attenuation
	shadowSpecFactor = 1.0f;
}


void main()
{
	// Rounded points
	vec2 centerPointv = gl_PointCoord - 0.5f;
	if (dot(centerPointv, centerPointv) > 0.25f)		// Vector * vector = square module => we avoid square root
	{
		discard;										// Discarded because distance to center is bigger than 0.5
	}
	
	float shadowDiffuseFactor, shadowSpecFactor;
	depthTextureUniform(shadowDiffuseFactor, shadowSpecFactor);
	fColor = vec4(attenuation * shadowDiffuseFactor * (diffuseColor + specularColor * shadowSpecFactor), 1.0f);
}
