#pragma once

#include "Geometry/3D/AABB.h"
#include "Graphics/Core/Model3D.h"
#include "Utilities/FileManagement.h"

#define SAVE_INTENSITY_CLASS false

/**
*	@file LiDARPointCloud.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 05/02/2021
*/

typedef std::unordered_map<int, std::vector<float>> IntensityClass;

/**
*	@brief Draws a regular grid as a saturation texture and a set of lines.
*/
class LiDARPointCloud
{
protected:
	friend class DrawLiDARPointCloud;

protected:
	std::vector<vec4>		_points;					//!< 3D space
	unsigned				_numPoints;					//!< 

	// [LiDAR data]
	std::vector<vec3>		_normal;					//!< 
	std::vector<vec2>		_textCoord;					//!< 
	std::vector<GLuint>		_modelComponent;			//!< 	
	std::vector<float>		_returnNumber;				//!< 
	std::vector<float>		_returnPercent;				//!< 
	std::vector<float>		_intensity;					//!<
	IntensityClass			_intensityClass;			//!<
	std::vector<float>		_scanAngleRank;				//!< 
	std::vector<vec3>		_scanDirection;				//!<
	std::vector<float>		_gpsTime;					//!< 
	float					_maxGPSTime;				//!<
	
	// [Metadata]
	AABB					_aabb;						//!< Boundaries

protected:
	/**
	*	@brief 
	*/
	bool writeBinary();

	/**
	* 
	*/
	bool writePLYThreaded(const std::string& filename, Group3D* scene);

public:
	/**
	*	@brif Default constructor. 
	*/
	LiDARPointCloud();

	/**
	*	@brief Destructor. 
	*/
	~LiDARPointCloud();

	/**
	*	@brief  
	*/
	bool archive();

	/**
	*	@brief  
	*/
	void pushCollisions(std::vector<Model3D::TriangleCollisionGPUData>& collisions, std::vector<Model3D::ModelComponent*>* modelComponents);

	/**
	*	@brief  
	*/
	bool writePLY(const std::string& filename, Group3D* scene, bool asynchronous = true);

	// ----------- Getters -------------

	/**
	*	@return Axis-aligned bounding-box of the point cloud. 
	*/
	AABB getAABB() { return _aabb; }

	/**
	*	@return Maximum observed GPS time.
	*/
	float getMaxGPSTime() { return _maxGPSTime; }

	/**
	*	@return Number of points. 
	*/
	unsigned getNumPoints() { return _points.size(); }
};

