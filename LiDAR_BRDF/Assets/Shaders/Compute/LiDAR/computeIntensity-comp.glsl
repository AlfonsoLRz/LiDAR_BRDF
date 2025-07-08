#define WATER_DIFFUSE		vec3(0.45f, 0.48f, 0.5f)
#define WATER_REFRACTIVE	1.33f

float getAttenuation(float distance)
{
	return pow(10.0f, -2.0f * distance * atmosphericAttenuation / 10000.0f);
}

float computeIntensity(const uint index, const float brdfFactor, const RayGPUData ray)
{
	const float distance			= rayCollision[index].distance;
	const float squaredDistance		= distance * distance;
	const float pulsePower			= ray.power * rayCollision[index].numIntersectedRays;
	const float squaredDiameter		= sensorDiameter * sensorDiameter;
	const float atmFactor			= getAttenuation(distance);

	return (pulsePower * squaredDiameter * brdfFactor * reflectanceWeight * atmFactor * systemAttenuation) / (4.0f * squaredDistance);
}

float computeBathymetricIntensity(const uint index, const float brdfFactor, const RayGPUData ray)
{
	const float maxDiffuseWater = max(WATER_DIFFUSE.x, max(WATER_DIFFUSE.y, WATER_DIFFUSE.z));
	const float distance		= rayCollision[index].distance;
	const float receiverArea	= PI * (sensorDiameter / 2.0f) * (sensorDiameter / 2.0f);
	const float altitude		= ray.startingPoint.y - waterHeight;
	const float depth			= waterHeight - rayCollision[index].point.y;
	const vec3 transmitDir		= normalize(rayCollision[rayCollision[index].previousCollision].point - ray.startingPoint);
	const float transmitCosine	= dot(transmitDir, vec3(.0f, -1.0f, .0f));
	const float denom			= WATER_REFRACTIVE * altitude + depth;
	const float waterAngle		= acos(dot(vec3(.0f, -1.0f, .0f), ray.direction));
	const float hypotenuse		= depth / cos(waterAngle);
	const float sinus			= sin(waterAngle) * hypotenuse;
	const float atmFactor		= getAttenuation(distance);

	float intensity				= (ray.power * brdfFactor * receiverArea * transmitCosine * transmitCosine * reflectanceWeight * atmFactor * 100.0f) / (PI * denom * denom);
	intensity					*= exp(-2.0f * maxDiffuseWater * depth * hypotenuse / sinus);

	return intensity;
}