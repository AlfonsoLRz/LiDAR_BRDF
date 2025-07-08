#pragma once

#include "Geometry/Animation/LinearInterpolation.h"
#include "Graphics/Application/LiDARParameters.h"
#include "Graphics/Application/PointCloudParameters.h"
#include "Graphics/Core/Group3D.h"
#include "Graphics/Core/LiDARPointCloud.h"
#include "Graphics/Core/MaterialDatabase.h"
#include "Graphics/Core/Model3D.h"
#include "Graphics/Core/RayBuilder.h"
#include "Utilities/Histogram.h"
#include "Utilities/PipelineMetrics.h"

/**
*	@file LiDARSimulation.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 03/09/2019
*/

#define ITERATE_BY_COUNT false
#define SAVE_OVERLEAF_CONTENT false

typedef std::unordered_set<Model3D::ModelComponent*> ModelComponentSet;
typedef std::vector<std::unique_ptr<RayBuilder>> RayBuilderApplicators;

/**
*	@brief Obtains a point cloud from any scene as a LiDAR sensor does.
*/
class LiDARSimulation
{
protected:
	const static float					NOISE_TEXTURE_FREQUENCY;				//!< Frequency of texture clustering
	const static unsigned				NOISE_TEXTURE_SIZE;						//!< Size of noise textures
	const static GLuint					RAY_MEMORY_BOUNDARY;					//!< Maximum number of rays per LiDAR iteration
	const static float					RAY_OVERFLOW;							//!< Exceeds ray destination point to make it looks like an infinite ray
	const static GLuint					SHADER_UINT_MAX;						//!< Infinite value for compute shaders

	static LiDARParameters				LIDAR_PARAMS;							//!<
	static PointCloudParameters			POINT_CLOUD_PARAMS;						//!< 

	// [Strategy pattern]
	const static RayBuilderApplicators	RAY_BUILDER_APPLICATOR;					//!< Applicators for each model of ray builder

protected:
	// [State]
	ModelComponentSet				_modelCompMap;								//!<
	std::vector<vec3>				_tlsPositions;								//!< Set of TLS positions to automatize data colleciton within an environment

	// [Results]
	LiDARPointCloud*				_pointCloud;								//!<

	// [Scene]
	Model3D::ModelComponent*		_emptyModelComponent;						//!< Empty model component for outlier values
	Group3D::StaticGPUData*			_groupGPUData;								//!<
	bool							_isForestScene;								//!<
	Group3D*						_scene;										//!<

	// [Rendering]
	VAO*							_LiDARRaysVAO;								//!<
	unsigned						_numRays;									//!<

	// [Temporary data]
	unsigned						_brdfSSBO;									//!<
	unsigned						_collisionSSBO;								//!<
	unsigned						_counterSSBO;								//!<
	unsigned						_hermiteSSBO;								//!<
	unsigned						_LiDARMaterialsSSBO;						//!<
	unsigned						_newCounterSSBO;							//!<
	unsigned						_returnThresholdSSBO;						//!<
	unsigned						_triangleCollisionSSBO;						//!<
	unsigned						_whiteNoiseSSBO;							//!<

protected:
	/**
	*	@return Array with applicators for each attenuation type.
	*/
	static std::vector<std::unique_ptr<RayBuilder>> getRayBuildApplicators();

protected:
	/**
	*	@brief Appends new LiDAR data to point cloud and loads it into GPU. 
	*/
	void appendLiDARData(std::vector<Model3D::TriangleCollisionGPUData>* collisions);

	/**
	*	@brief Creates a new noise texture to sample random values from a uniform distribution.
	*/
	unsigned buildWhiteNoiseTexture(const unsigned size);

	/**
	*	@brief Defines those uniform variables which depend on the type of scene and its parameters.
	*/
	void defineSceneUniforms(ComputeShader* LiDARShader);

	/**
	*	@brief 
	*/
	void filterPath(std::vector<vec3>& path);

	/**
	*	@return Attenuator of atmospheric conditions for radar equation.  
	*/
	float getAtmosphericAttenuation();

	/**
	*	@brief Builds a TLS path depending on if it is marked as manual or not.
	*/
	void getTLSPath(std::vector<vec3>*& tlsPath);

	/**
	*	@brief Creates a new VAO which allows us to render the complete set of rays in a more efficient way.
	*/
	void instantiateRaysVAO(std::vector<Model3D::RayGPUData>& rayArray);

	/**
	*	@brief Launchs multiple LiDAR simulations.
	*/
	void launchMultipleSimulations(std::vector<vec3>& positions);

	/**
	*	@brief Launchs a single LiDAR simulation.
	*/
	void launchSingleSimulation(bool instantiateRaysVAO);

	/**
	*	@brief Prepares temporary data for LiDAR simulation. 
	*/
	void prepareLiDARData(GLuint numRays);

	/**
	*	@brief Prepares material-related data for LiDAR simulation.
	*/
	void prepareMaterialData(GLuint wavelength);

	/**
	*	@brief Releases memory from temporary data. 
	*/
	void releaseLiDARData();

	/**
	*	@brief Releases memory from material-related data.
	*/
	void releaseMaterialData();

	/**
	*	@brief Gets ray intersections with clusters in BVH.
	*	@param rayArray Vector of arrays which needs to be tried.
	*/
	PipelineMetrics solveRayIntersection(GLuint raySSBO, GLuint numRays, std::vector<Model3D::TriangleCollisionGPUData>& collisions, int wl, bool readData = true);

public:
	/**
	*	@brief Main constructor.
	*	@param scene Group of objects to be recognized with sensor.
	*/
	LiDARSimulation(Group3D* scene);

	/**
	*	@brief Copy constructor removal.
	*/
	LiDARSimulation(const LiDARSimulation& orig) = delete;

	/**
	*	@brief Destructor.
	*/
	virtual ~LiDARSimulation();

	/**
	*	@brief Casts the rays and computes the intersection of each one with the scene.
	*	@param instantiateRayVAO True if ray VAO must be initialized for future rendering.
	*	@return True if there are new results from LiDAR sensor.
	*/
	void launchSimulation(bool instantiateRayVAO = true);

	/**
	*	@brief Assignment operator removal.
	*/
	LiDARSimulation& operator=(const LiDARSimulation& orig) = delete;

	// ------------- Getters -----------------
	
	/**
	*	@return Linear interpolation with points which must be followed by aerial LiDAR.
	*/
	std::vector<Interpolation*> getAerialPath();

	/**
	*	@return Static instance of LiDAR parameters.
	*/
	static LiDARParameters* getLiDARParams() { return &LIDAR_PARAMS; }

	/**
	*	@return Point cloud with accumulated LiDAR results. 
	*/
	LiDARPointCloud* getLiDARPointCloud() { return _pointCloud; }

	/**
	*	@return Maximum detected GPS time.
	*/
	float getMaximumGPSTime() { return _pointCloud->getMaxGPSTime(); }

	/**
	*	@return Number of rays which have been casted.
	*/
	unsigned getNumRays() { return _numRays; }

	/**
	*	@return Static instance of point cloud parameters.
	*/
	static PointCloudParameters* getPointCloudParams() { return &POINT_CLOUD_PARAMS; }

	/**
	*	@return VAO which allows us to render the LiDAR rays.
	*/
	VAO* getRayVAO() { return _LiDARRaysVAO; }

	/**
	*	@return TLS positions to automatize several LIDAR simulations.
	*/
	std::vector<vec3>* getTLSPositions() { return &_tlsPositions; }

	// ------------ Setters -------------

	/**
	*	@brief Modifies the instance which wraps the GPU data of a group. 
	*/
	void setGroupGPUData(Group3D::StaticGPUData* groupGPUData) { _groupGPUData = groupGPUData; }

	/**
	*	@brief Modifies the set of TLS positions to launch LiDAR simulations.
	*/
	void setTLSPositions(std::vector<vec3>& tlsPositions) { _tlsPositions = tlsPositions; }
};

