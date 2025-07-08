#define BRDF_IDEAL_SPECULAR		0
#define BRDF_IDEAL_DIFFUSE		1
#define BRDF_MINNAERT			2
#define BRDF_BLINN_PHONG		3
#define BRDF_COOK_TORRANCE		4
#define BRDF_WARD_ANISOTROPIC	5
#define BRDF_OREN_NAYAR			6
#define BRDF_ZOHDI              7

#define SQUARE(x) (x * x)
#define ATAN2(x, y) (mix(PI / 2.0f - atan(x, y), atan(y, x), float(abs(x) > abs(y))))
#define VECTOR_H(v, l) (normalize(v + l))

#define BLINN_PHONG_ROUGHNESS_MULTIPLIER 100.0f

// ------------- IDEAL SPECULAR -------------

float idealSpecular(const uint index, const vec2 KdKs, const RayGPUData ray, const MaterialGPUData material, const vec3 reflectDirection)
{
    const vec3 perfectReflection    = normalize(reflect(rayCollision[index].normal, ray.previousDirection));
    const float dotReflectReceiver  = abs(dot(perfectReflection, reflectDirection));
    
    if (dotReflectReceiver < EPSILON) return KdKs.y * 1.0f;

    return .0f;
}

// ------------- IDEAL DIFFUSE -------------

float idealDiffuse(const uint index, const vec2 KdKs, const RayGPUData ray, const MaterialGPUData material, const vec3 reflectDirection)
{
    return KdKs.x * cos(rayCollision[index].angle);
}

// ------------- MINNAERT -------------

#define MINNAERT_K 1.5f

float minnaert(const uint index, const vec2 KdKs, const RayGPUData ray, const MaterialGPUData material, const vec3 reflectDirection)
{
    const float dotReflection = dot(rayCollision[index].normal, reflectDirection);

    return KdKs.x * pow(cos(rayCollision[index].angle) * dotReflection, MINNAERT_K - 1);
}

// ------------- BLINN PHONG -------------

float blinnPhong(const uint index, const vec2 KdKs, const RayGPUData ray, const MaterialGPUData material, const vec3 reflectDirection)
{
    return KdKs.x * cos(rayCollision[index].angle) + KdKs.y * pow(dot(rayCollision[index].normal, VECTOR_H(reflectDirection, -ray.previousDirection)), material.roughness * BLINN_PHONG_ROUGHNESS_MULTIPLIER);
}

// ------------- COOK TORRANCE ------------

float CT_normalDistribution(const float dotNH, const float roughness)
{   
    const float dotNHSquared = SQUARE(dotNH);
    const float squaredRoughness = SQUARE(roughness);

    return (1.0f / (squaredRoughness * dotNHSquared * dotNHSquared)) * exp((dotNHSquared - 1.0f) / (squaredRoughness * dotNHSquared));
}

float CT_geometrySchlickGGX(const vec3 normal, const vec3 direction, const float roughness)
{   
    float k = SQUARE(roughness) / 2.0f;
    float dotNV = dot(normal, direction);

    return dotNV / (dotNV * (1.0f - k) + k);
}

float CT_geometrySmith(const vec3 normal, const vec3 rayDirection, const vec3 reflectDirection, const vec3 h, const float roughness)
{   
    float dotNL = dot(normal, rayDirection);
	float dotNV = dot(normal, reflectDirection);
    float dotNH = dot(normal, h);
	float dotHV = dot(h, reflectDirection);

	float G1 = (2.0 * dotNH * dotNV) / dotHV;
	float G2 = (2.0 * dotNH * dotNL) / dotHV;
	float G = min(1.0f, min(G1, G2));

    return G;
}

float CT_fresnelSchlick(const float cosTheta, const vec3 F0Reflectivity)
{
    const vec3 fresnel = F0Reflectivity + (1.0f - F0Reflectivity) * pow(1.0f - cosTheta, 5.0f);

    return max(fresnel.x, max(fresnel.y, fresnel.z));
}

float cookTorrance(const uint index, const vec2 KdKs, const RayGPUData ray, const MaterialGPUData material, const vec3 reflectDirection)
{   
    const vec3 h                = VECTOR_H(reflectDirection, -ray.previousDirection);
    const float dotNH           = dot(h, rayCollision[index].normal);
    const float roughness       = 1.0f - material.roughness;

    const float normalDist      = CT_normalDistribution(dotNH, material.roughness);
    const float geometryFunct   = CT_geometrySmith(rayCollision[index].normal, -ray.previousDirection, reflectDirection, h, roughness);
    const float fresnelF        = CT_fresnelSchlick(dotNH, material.reflectivity);
    const float cosAngle        = cos(rayCollision[index].angle);
    const float dotReflection   = dot(rayCollision[index].normal, reflectDirection);

    return KdKs.x * cos(rayCollision[index].angle) + KdKs.y / PI + KdKs.y * (fresnelF * geometryFunct * normalDist) / (PI * cosAngle * dotReflection);
}

// ------------- WARD ANISOTROPIC -------------

#define ALPHA vec2(.15f, .75f)

float wardAnisotropic(const uint index, const vec2 KdKs, const RayGPUData ray, const MaterialGPUData material, const vec3 reflectDirection)
{
    const vec3 h            = VECTOR_H(reflectDirection, -ray.previousDirection);
	const vec3 binormal     = normalize(cross(rayCollision[index].tangent, rayCollision[index].normal));
    const float dotLN       = clamp(dot(-ray.previousDirection, rayCollision[index].normal), .0f, 1.0f);
	const float dotNV       = clamp(dot(rayCollision[index].normal, reflectDirection), .0f, 1.0f);
	const float dotHX       = dot(h, rayCollision[index].tangent);
	const float dotHY       = dot(h, binormal);	
    const float dotHN       = dot(h, rayCollision[index].normal);
	    
	const vec2 alphaTerm    = vec2(dotHX / ALPHA.x, dotHY / ALPHA.y);
	const float expFactor   = -2.0f * ((SQUARE(alphaTerm.x) + SQUARE(alphaTerm.y)) / (1.0f + dotHN));

	return KdKs.x * cos(rayCollision[index].angle) + KdKs.y * (1.0f / (4.0f * PI * ALPHA.x * ALPHA.y * sqrt(max(dotLN * dotNV, .1f)))) * exp(expFactor);
}

// ------------- OREN NAYAR -------------

float getOrenNayarA(const float roughness)
{
    const float roughnessSquared = SQUARE(roughness);
    return 1.0f - 0.5f * (roughnessSquared / (roughnessSquared + .33f));
}

float getOrenNayarB(const float roughness)
{
    const float roughnessSquared = SQUARE(roughness);
    return .45f * (roughnessSquared / (roughnessSquared + .09f));
}

float orenNayar(const uint index, const vec2 KdKs, const RayGPUData ray, const MaterialGPUData material, const vec3 reflectDirection)
{
	const float dotLN   = clamp(dot(-ray.previousDirection, rayCollision[index].normal), .0f, 1.0f);
	const float dotNV   = clamp(dot(rayCollision[index].normal, reflectDirection), .0f, 1.0f);
    const float lnBeta  = acos(dotLN);
	const float nvAlpha = acos(dotNV);
    const float roughness = material.roughness;

    return KdKs.x * (getOrenNayarA(roughness) + getOrenNayarB(roughness) * max(0, dotNV - dotLN) * sin(max(nvAlpha, lnBeta)) * cos(min(nvAlpha, lnBeta)));
}

float reflectIrradiance(const uint index, const vec2 KdKs, const RayGPUData ray)
{
    const MaterialGPUData material = materialData[meshData[rayCollision[index].modelCompID].materialID];
	const uint brdfModel = material.brdfModel;
	const vec3 reflectVector = normalize(-ray.previousDirection);
    float irradiance = .0f;

	switch (brdfModel) {
    case BRDF_IDEAL_SPECULAR:
        irradiance = idealSpecular(index, KdKs, ray, material, reflectVector);
        break;

    case BRDF_IDEAL_DIFFUSE:
        irradiance = idealDiffuse(index, KdKs, ray, material, reflectVector);
        break;

    case BRDF_MINNAERT:
        irradiance = minnaert(index, KdKs, ray, material, reflectVector);
        break;

    case BRDF_BLINN_PHONG:
        irradiance = blinnPhong(index, KdKs, ray, material, reflectVector);
        break;

    case BRDF_COOK_TORRANCE:
        irradiance = cookTorrance(index, KdKs, ray, material, reflectVector);
        break;

    case BRDF_WARD_ANISOTROPIC:
        irradiance = wardAnisotropic(index, KdKs, ray, material, reflectVector);
        break;

    case BRDF_OREN_NAYAR:
        irradiance = orenNayar(index, KdKs, ray, material, reflectVector);
        break;
    }

    return clamp(irradiance, .0f, 1.0f);
}