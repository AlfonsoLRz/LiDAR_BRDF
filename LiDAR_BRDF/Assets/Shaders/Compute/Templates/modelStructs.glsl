struct VertexGPUData
{
	vec3	position;
	float	ks;
	vec3	normal;
	float	ns;
	vec2	textCoord;
	vec3	tangent;
	vec4	kad;
};

struct FaceGPUData
{
	uvec3	vertices;
	uint	modelCompID;

	vec3	minPoint;
	vec3	maxPoint;
	vec3    normal;
};

struct MeshGPUData
{
	uint	numVertices;
	uint	startIndex;
	float	opacity;
	float	shininess;

	vec2	reflectance;
	uint	surface;
	uint	materialID;
};

struct MaterialGPUData
{
	float	refractiveIndex;
	float	roughness;
	vec2	padding;
};

struct BVHCluster
{
	vec3	minPoint;
	uint	prevIndex1;

	vec3	maxPoint;
	uint	prevIndex2;

	uint	faceIndex;
};

struct RayGPUData 
{
	vec3	origin;
	float	gpsTime;

	vec3	destination;
	float	power;

	vec3	direction;
	uint	returnNumber;

	vec3	startingPoint;
	uint	lastCollisionIndex;

	vec3	previousDirection;
	uint	continueRay;
};

struct TriangleCollisionGPUData
{
	vec3	point;
	uint	faceIndex;

	vec3	normal;
	float	distance;

	vec2	textCoord;
	uint	modelCompID;	
	uint	returnNumber;

	vec3	tangent;
	uint	rayIndex;

	uint	numReturns;
	float	angle;
	float	intensity;
	uint	previousCollision;

	vec2	padding1;
	uint	numIntersectedRays;
	float	gpsTime;
};