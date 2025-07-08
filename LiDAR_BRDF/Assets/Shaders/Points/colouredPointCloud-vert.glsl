#version 450

// ********** STRUCT ************

struct VertexData
{
	vec3 position;
	vec3 normal;
	vec4 shadowCoord;

	vec4 kad;
	vec4 ks;
};

// ********** PARAMETERS & VARIABLES ***********

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTextCoord;


// ------------- Point parameters ---------------
uniform float pointSize;

// ------------- Matrices -----------
uniform mat4 mModelView;
uniform mat4 mModelViewProj;
uniform mat4 mShadow;

// ------------- Light types ----------------
subroutine void lightType(const VertexData vData);
subroutine uniform lightType lightUniform;

// ---- Geometry ----
uniform vec3 lightPosition;
uniform vec3 lightDirection;

// ---- Colors ----
uniform vec3 Ia;
uniform vec3 Id;
uniform vec3 Is;

// ---- Spot light ----
uniform float cosUmbra, cosPenumbra;      // Ranged angle where light just fades out
uniform float exponentS;

// ---- Lighting attenuation ----
subroutine float attenuationType(const float distance);
subroutine uniform attenuationType attenuationUniform;

// Basic model
uniform float c1, c2, c3;

// Ranged distance model
uniform float minDistance, maxDistance;

// Pixar model
uniform float fMax;
uniform float distC, fC;
uniform float exponentSE;
uniform float k0, k1;

// ------------- Materials ---------------
uniform sampler2D texKadSampler;
uniform sampler2D texKsSampler;
uniform float materialScattering;				// Substitutes ambient lighting
uniform float shininess;

subroutine vec4 semiTransparentType(const vec4 color);
subroutine uniform semiTransparentType semiTransparentUniform;

uniform sampler2D texSemiTransparentSampler;


// Vertex data
out vec4 shadowCoord;

// Color related
out float attenuation;
out vec3 diffuseColor;
out vec3 specularColor;


// ********* FUNCTIONS ************

// ----------- Attenuation ------------

// Computes the attenuation which must be applied to the fragment color
// Distance is the length of the vector which goes from the fragment to the
// lighting position

subroutine(attenuationType)
float basicAttenuation(const float distance)
{
	return min(1.0f / (c1 + c2 * distance + c3 * pow(distance, 2)), 1.0f);
}

subroutine(attenuationType)
float rangedAttenuation(const float distance)
{
	return clamp((maxDistance - distance) / (maxDistance - minDistance), 0.0f, 1.0f);
}

subroutine(attenuationType)
float pixarAttenuation(const float distance)
{
	float attenuation;

	if (distance <= distC)
	{
		attenuation = fMax * exp(k0 * pow(distance / distC, -k1));
	}
	else
	{
		attenuation = fC * pow(distC / distance, exponentSE);
	}

	return attenuation;
}

// ------------- Lighting --------------

// Computes the diffuse term with lighting wrapping, if active
vec3 getDiffuse(const vec3 kad, const float dotLN)   // TODO
{
	return Id * kad * max((dotLN + materialScattering) / (1 + materialScattering), 0.0f);
}

// Computes the specular term with halfway vector
vec3 getSpecular(const vec3 ks, const float dotHN)
{
	return Is * ks * pow(max(dotHN, 0.0f), shininess);
}

// Computes the color related to any light source. Receives the attenuation variables from shadows
void setDiffuseAndSpecular(const VertexData vData)
{
	const vec3 n = normalize(vData.normal);
	const vec3 l = normalize(lightPosition - vData.position);
	const vec3 v = normalize(-vData.position);
	const vec3 h = normalize(v + l);						// Halfway vector

	const float dotLN = clamp(dot(l, n), -1.0f, 1.0f);      // Prevents Nan values from acos
	const float dotHN = dot(h, n);

	diffuseColor = getDiffuse(vData.kad.rgb, dotLN);
	specularColor = getSpecular(vData.ks.rgb, dotHN);
}

// Computes the color from a light source, including diffuse and specular terms, as well as 
// ambient if necessary (ambient light). The result can be attenuated taking into account any
// model for such effect

subroutine(lightType)
void ambientLight(const VertexData vData)
{
	diffuseColor = Ia * vData.kad.rgb;
	specularColor = vec3(0.0f);
	attenuation = 1.0f;
}

subroutine(lightType)
void pointLight(const VertexData vData)
{
	const float distance = distance(lightPosition, vData.position);

	setDiffuseAndSpecular(vData);
	attenuation = attenuationUniform(distance);
}

subroutine(lightType)
void directionalLight(const VertexData vData)
{
	const vec3 n = normalize(vData.normal);
	const vec3 l = normalize(-lightDirection);
	const vec3 v = normalize(-vData.position);
	const vec3 h = normalize(v + l);						// Halfway vector

	const float dotLN = clamp(dot(l, n), -1.0f, 1.0f);      // Prevents Nan values from acos
	const float dotHN = dot(h, n);

	diffuseColor = getDiffuse(vData.kad.rgb, dotLN);
	specularColor = getSpecular(vData.ks.rgb, dotHN);

	attenuation = 1.0f;
}

subroutine(lightType)
void spotLight(const VertexData vData)
{
	const vec3 n = normalize(vData.normal);
	const vec3 l = normalize(lightPosition - vData.position);
	const vec3 v = normalize(-vData.position);
	const vec3 d = normalize(lightDirection);
	const vec3 h = normalize(v + l);						// Halfway vector

	const float dotLN = clamp(dot(l, n), -1.0f, 1.0f);      // Prevents Nan values from acos
	const float dotHN = dot(h, n);

	diffuseColor = getDiffuse(vData.kad.rgb, dotLN);
	specularColor = getSpecular(vData.ks.rgb, dotHN);

	const float distance = distance(lightPosition, vData.position);

	// Radial attenuation
	float sf = 0.0f;
	const float dotLD = dot(-l, d);

	if (dotLD >= cosPenumbra) 
	{
		sf = 1.0f;
	}
	else if (dotLD > cosUmbra) 
	{
		sf = pow((dotLD - cosUmbra) / (cosPenumbra - cosUmbra), exponentS);		
	}

	attenuation = sf * attenuationUniform(distance);
}

subroutine(lightType)
void rimLight(const VertexData vData)
{
	const vec3 n = normalize(vData.normal);
	const vec3 v = normalize(-vData.position);
	const float vdn = 1.0f - max(dot(v, n), 0.0f);

	diffuseColor = vdn * Ia;
	specularColor = vec3(0.0f);
	attenuation = 1.0f;
}

// --------------- Materials ---------------

// ----- Diffuse & specular -----

// Obtains color from diffuse texture
vec4 getKad()
{
	return texture(texKadSampler, vTextCoord);
}

// Obtains color from specular texture
vec4 getKs()
{
	return texture(texKsSampler, vTextCoord);
}

// ----- Semitransparent -----

// Combines current fragment color with a semi-transparent texture, if any

subroutine(semiTransparentType)
vec4 semiTransparentTexture(const vec4 color)
{
	const vec4 semiTransparent = texture(texSemiTransparentSampler, vTextCoord);

	return vec4(mix(color.xyz, semiTransparent.xyz, semiTransparent.w), color.w);
}

subroutine(semiTransparentType)
vec4 noSemiTransparentTexture(const vec4 color)
{
	return color;
}


void main()
{
	VertexData vData;

	// Geometry
	vData.position = vec3(mModelView *  vPosition);
	vData.normal = vec3(mModelView * vec4(vNormal, 0.0f));			// Needed for TBN matrix
	vData.shadowCoord = mShadow * vPosition;
	
	gl_Position = mModelViewProj * vPosition;
	gl_PointSize = pointSize;

	// Color
	vData.kad = semiTransparentUniform(getKad());
	vData.ks = getKs();

	lightUniform(vData);
	shadowCoord = vData.shadowCoord;
}
