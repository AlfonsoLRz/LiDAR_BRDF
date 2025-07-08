#define TERRAIN_MASK 1 << 0
#define WATER_MASK 1 << 1
#define VEGETATION_MASK 1 << 2

// Checks collision between a ray and BVH built from scene geometry & topology
bool checkCollision(const uint index, inout RayGPUData ray, inout uint globalIndex, out bool isCollisionValid)
{
	// No collision by default
	collision[index].faceIndex	= UINT_MAX;
	collision[index].distance	= UINT_MAX;

	// Initialize stack
	bool	collided			= false;
	int		currentIndex		= 0;
	uint	toExplore[100];

	toExplore[currentIndex] = numClusters - 1;			// First node to explore: root

	while (currentIndex >= 0)
	{
		BVHCluster cluster = clusterData[toExplore[currentIndex]];

		if (rayAABBIntersection(ray, cluster.minPoint, cluster.maxPoint, collision[index].distance))
		{
			if (cluster.faceIndex != UINT_MAX)
			{
				if (rayTriangleIntersection(index, faceData[cluster.faceIndex], ray))
				{
					collision[index].faceIndex		= cluster.faceIndex;								// Collision point is already set
					collision[index].modelCompID	= faceData[cluster.faceIndex].modelCompID;
					collision[index].returnNumber	= ray.returnNumber;
					collided						= true;
				}
			}
			else
			{
				toExplore[currentIndex]		= cluster.prevIndex1;
				toExplore[++currentIndex]	= cluster.prevIndex2;
				++currentIndex;										// Prevent --currentIndex instead of branching
			}
		}

		--currentIndex;
	}
	
	if (collided)
	{
		globalIndex	= atomicAdd(numCollisions, 1);
		collision[index].intensity = computeIntensity(index, ray);

		faceCollision[globalIndex] = collision[index];
	}

	return collided;
}