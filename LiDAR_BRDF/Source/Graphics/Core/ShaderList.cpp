#include "stdafx.h"
#include "ShaderList.h"

// [Static members initialization]

std::unordered_map<uint8_t, std::string> ShaderList::COMP_SHADER_SOURCE{
		{RendEnum::ADD_OUTLIER_SHADER, "Assets/Shaders/Compute/LiDAR/addOutlier"},
		{RendEnum::AERIAL_ELLIPTICAL_LIDAR, "Assets/Shaders/Compute/LiDAR/Instancing/airborneElliptical"},
		{RendEnum::AERIAL_LINEAR_ZIGZAG_LIDAR, "Assets/Shaders/Compute/LiDAR/Instancing/airborneLinearZigzag"},
		{RendEnum::BIT_MASK_RADIX_SORT, "Assets/Shaders/Compute/RadixSort/bitMask-radixSort"},
		{RendEnum::BUILD_CLUSTER_BUFFER, "Assets/Shaders/Compute/BVHGeneration/buildClusterBuffer"},
		{RendEnum::CLUSTER_MERGING, "Assets/Shaders/Compute/BVHGeneration/clusterMerging"},
		{RendEnum::COMPUTE_BEZIER_CURVE, "Assets/Shaders/Compute/Interpolations/buildBezierCurve"},
		{RendEnum::COMPUTE_POINT_COLOR, "Assets/Shaders/Compute/LiDAR/computeColor"},
		{RendEnum::COMPUTE_FACE_AABB, "Assets/Shaders/Compute/Model/computeFaceAABB"},
		{RendEnum::COMPUTE_GROUP_AABB, "Assets/Shaders/Compute/Group/computeGroupAABB"},
		{RendEnum::COMPUTE_BUILDING_POSITION, "Assets/Shaders/Compute/Terrain/computeBuildingPosition"},
		{RendEnum::COMPUTE_MORTON_CODES, "Assets/Shaders/Compute/BVHGeneration/computeMortonCodes"},
		{RendEnum::COMPUTE_TANGENTS_1, "Assets/Shaders/Compute/Model/computeTangents_1"},
		{RendEnum::COMPUTE_TANGENTS_2, "Assets/Shaders/Compute/Model/computeTangents_2"},
		{RendEnum::COMPUTE_TERRAIN_NORMALS, "Assets/Shaders/Compute/Terrain/computeNormals"},
		{RendEnum::COMPUTE_TREE_PROPERTIES, "Assets/Shaders/Compute/Terrain/computeTreeProperties"},
		{RendEnum::DOWN_SWEEP_PREFIX_SCAN, "Assets/Shaders/Compute/PrefixScan/downSweep-prefixScan"},
		{RendEnum::END_LOOP_COMPUTATIONS, "Assets/Shaders/Compute/BVHGeneration/endLoopComputations"},
		{RendEnum::ERODE_TERRAIN, "Assets/Shaders/Compute/Terrain/terrainErosion"},
		{RendEnum::FIND_BEST_NEIGHBOR, "Assets/Shaders/Compute/BVHGeneration/findBestNeighbor"},
		{RendEnum::FIND_BVH_COLLISION, "Assets/Shaders/Compute/LiDAR/findBVHCollision"},
		{RendEnum::GENERATE_TREE_GEOMETRY_TOPOLOGY, "Assets/Shaders/Compute/Terrain/generateTreeGeometryTopology"},
		{RendEnum::GENERATE_VEGETATION, "Assets/Shaders/Compute/Terrain/generateVegetation"},
		{RendEnum::GENERATE_VEGETATION_MAP, "Assets/Shaders/Compute/Terrain/genVegetationMap"},
		{RendEnum::MODEL_APPLY_MODEL_MATRIX, "Assets/Shaders/Compute/Model/modelApplyModelMatrix"},
		{RendEnum::MODEL_MESH_GENERATION, "Assets/Shaders/Compute/Model/modelMeshGeneration"},
		{RendEnum::RAY_GEOMETRY_INTERSECTION, "Assets/Shaders/Compute/LiDAR/LiDARSensor"},
		{RendEnum::PLANAR_SURFACE_GENERATION, "Assets/Shaders/Compute/PlanarSurface/planarSurfaceGeometryTopology"},
		{RendEnum::PLANAR_SURFACE_TOPOLOGY, "Assets/Shaders/Compute/PlanarSurface/planarSurfaceFaces"},
		{RendEnum::PREPARE_LIDAR_DATA, "Assets/Shaders/Compute/LiDAR/prepareData"},
		{RendEnum::REALLOCATE_CLUSTERS, "Assets/Shaders/Compute/BVHGeneration/reallocateClusters"},
		{RendEnum::REALLOCATE_RADIX_SORT, "Assets/Shaders/Compute/RadixSort/reallocateIndices-radixSort"},
		{RendEnum::REDUCE_COLLISIONS, "Assets/Shaders/Compute/LiDAR/reduceCollisions"},
		{RendEnum::REDUCE_PREFIX_SCAN, "Assets/Shaders/Compute/PrefixScan/reduce-prefixScan"},
		{RendEnum::RESET_BUFFER_INDEX, "Assets/Shaders/Compute/Generic/resetBufferIndex"},
		{RendEnum::RESET_LAST_POSITION_PREFIX_SCAN, "Assets/Shaders/Compute/PrefixScan/resetLastPosition-prefixScan"},
		{RendEnum::RESET_LIDAR_DATA, "Assets/Shaders/Compute/LiDAR/resetData"},
		{RendEnum::RETRIEVE_COLORS, "Assets/Shaders/Compute/Model/retrieveColors"},
		{RendEnum::TERRAIN_FACES_TOPOLOGY, "Assets/Shaders/Compute/Terrain/terrainFaces"},
		{RendEnum::TERRAIN_GEOMETRY_TOPOLOGY, "Assets/Shaders/Compute/Terrain/terrainGeometryTopology"},
		{RendEnum::TERRESTRIAL_SPHERICAL_LIDAR, "Assets/Shaders/Compute/LiDAR/Instancing/terrestrialSpherical"},
		{RendEnum::UPDATE_COLLISION_RETURNS, "Assets/Shaders/Compute/LiDAR/updateReturns"},
		{RendEnum::UPDATE_LIDAR_DATA, "Assets/Shaders/Compute/LiDAR/updateData"}
};

std::unordered_map<uint8_t, std::string> ShaderList::REND_SHADER_SOURCE{
		{RendEnum::BLUR_SHADER, "Assets/Shaders/Filters/blur"},
		{RendEnum::BLUR_SSAO_SHADER, "Assets/Shaders/2D/blurSSAOShader"},
		{RendEnum::BVH_SHADER, "Assets/Shaders/Lines/bvh"},
		{RendEnum::DEBUG_QUAD_SHADER, "Assets/Shaders/Triangles/debugQuad"},
		{RendEnum::GEOMETRY_CONE_SHADER, "Assets/Shaders/Geometry/coneInstance"},
		{RendEnum::GEOMETRY_RANGE_SHADER, "Assets/Shaders/Geometry/maxRangeVisualizer"},
		{RendEnum::GRASS_SHADER, "Assets/Shaders/Triangles/grassTriangleMesh"},
		{RendEnum::INTENSITY_POINT_CLOUD_SHADER, "Assets/Shaders/Points/pointCloudIntensity"},
		{RendEnum::MULTI_INSTANCE_SHADOWS_SHADER, "Assets/Shaders/Triangles/multiInstanceShadowsShader"},
		{RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_POSITION_SHADER, "Assets/Shaders/Triangles/multiInstanceTriangleMeshPosition"},
		{RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_NORMAL_SHADER, "Assets/Shaders/Triangles/multiInstanceTriangleMeshNormal"},
		{RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_GROUP_SHADER, "Assets/Shaders/Triangles/multiInstanceTriangleMeshGroup"},
		{RendEnum::NORMAL_MAP_SHADER, "Assets/Shaders/Filters/normalMapShader"},
		{RendEnum::POINT_CLOUD_SHADER, "Assets/Shaders/Points/pointCloud"},
		{RendEnum::POINT_CLOUD_COLOUR_SHADER, "Assets/Shaders/Points/colouredPointCloud"},
		{RendEnum::POINT_CLOUD_HEIGHT_SHADER, "Assets/Shaders/Points/pointCloudHeight"},
		{RendEnum::POINT_CLOUD_GRAYSCALE_HEIGHT_SHADER, "Assets/Shaders/Points/pointCloudHeightGrayscale"},
		{RendEnum::POINT_CLOUD_NORMAL_SHADER, "Assets/Shaders/Points/pointCloudNormal"},
		{RendEnum::POINT_CLOUD_RETURN_NUMBER_SHADER, "Assets/Shaders/Points/pointCloudReturnNumber"},
		{RendEnum::POINT_CLOUD_SCAN_ANGLE_RANK_SHADER, "Assets/Shaders/Points/pointCloudScanAngle"},
		{RendEnum::POINT_CLOUD_SCAN_DIRECTION_SHADER, "Assets/Shaders/Points/pointCloudScanDirection"},
		{RendEnum::POINT_CLOUD_GPS_TIME_SHADER, "Assets/Shaders/Points/pointCloudGPSTime"},
		{RendEnum::TERRAIN_REGULAR_GRID_SHADER, "Assets/Shaders/Triangles/regularGrid"},
		{RendEnum::TERRAIN_SHADER, "Assets/Shaders/Triangles/terrainShader"},
		{RendEnum::TREE_TRIANGLE_MESH_SHADER, "Assets/Shaders/Triangles/treeTriangleMesh"},
		{RendEnum::TRIANGLE_MESH_SHADER, "Assets/Shaders/Triangles/triangleMesh"},
		{RendEnum::TRIANGLE_MESH_GROUP_SHADER, "Assets/Shaders/Triangles/triangleMeshGroup"},
		{RendEnum::TRIANGLE_MESH_NORMAL_SHADER, "Assets/Shaders/Triangles/triangleMeshNormal"},
		{RendEnum::TRIANGLE_MESH_POSITION_SHADER, "Assets/Shaders/Triangles/triangleMeshPosition"},
		{RendEnum::WIREFRAME_SHADER, "Assets/Shaders/Lines/wireframe"},
		{RendEnum::REFLECTIVE_TRIANGLE_MESH_SHADER, "Assets/Shaders/Triangles/reflectiveTriangleMesh"},
		{RendEnum::SHADOWS_SHADER, "Assets/Shaders/Triangles/shadowsShader"},
		{RendEnum::SSAO_SHADER, "Assets/Shaders/2D/ssaoShader"},
		{RendEnum::WATER_LAKE_SHADER, "Assets/Shaders/Triangles/waterLake"}
};

std::vector<std::unique_ptr<ComputeShader>> ShaderList::_computeShader(RendEnum::numComputeShaderTypes());
std::vector<std::unique_ptr<RenderingShader>> ShaderList::_renderingShader(RendEnum::numRenderingShaderTypes());

/// [Protected methods]

ShaderList::ShaderList()
{
}

/// [Public methods]

ComputeShader* ShaderList::getComputeShader(const RendEnum::CompShaderTypes shader)
{
	const int shaderID = shader;

	if (!_computeShader[shader].get())
	{
		ComputeShader* shader = new ComputeShader();
		shader->createShaderProgram(COMP_SHADER_SOURCE.at(shaderID).c_str());

		_computeShader[shaderID].reset(shader);
	}

	return _computeShader[shaderID].get();
}

RenderingShader* ShaderList::getRenderingShader(const RendEnum::RendShaderTypes shader)
{
	const int shaderID = shader;

	if (!_renderingShader[shader].get())
	{
		RenderingShader* shader = new RenderingShader();
		shader->createShaderProgram(REND_SHADER_SOURCE.at(shaderID).c_str());

		_renderingShader[shaderID].reset(shader);
	}

	return _renderingShader[shader].get();
}
