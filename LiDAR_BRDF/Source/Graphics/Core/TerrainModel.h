#pragma once

#include "DataStructures/RegularGrid.h"
#include "Graphics/Application/TerrainConfiguration.h"
#include "Graphics/Core/CADModel.h"
#include "Graphics/Core/Grass.h"
#include "Graphics/Core/Model3D.h"
#include "Graphics/Core/Terrain.h"
#include "Utilities/RandomUtilities.h"

/**
*	@file TerrainModel.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 13/01/2021
*/

/**
*	@brief Model to be placed over the terrain.
*/
class TerrainModel: public CADModel
{		
protected:
	const static float		TERRAIN_OFFSET;					//!< Boundaries to generate particles, so that they cannot be generated on the ends of the terrain
	const static float		RANDOM_ROTATION;				//!< 

protected:
	float					_elevation;						//!< Its used to rectify the position of the model in the Y axis
	unsigned				_radius;						//!< Radius where we check the occupance in the regular grid
	TerrainConfiguration*	_terrainConfiguration;			//!<

protected:
	/**
	*	@brief Computes the raw mesh buffer (a buffer of indices for rendering purposes).
	*/
	void computeMeshData(ModelComponent* modelComp);

	/**
	*	@brief Removes faces that exceeds the terrain boundaries.
	*/
	void filterFaces(const AABB& terrainAABB);

	/**
	*	@brief Generates geometry via GPU.
	*/
	void generateGeometryTopology(ModelComponent* modelComp, const mat4& modelMatrix);

	/**
	*	@brief Generates some seeds where the model could be instantiated.
	*/
	void generateRandomPositions(std::vector<vec4>& randomPositions);

	/**
	*	@return Index of that value with maximum goodness value. Therefore, it is the most appropiate position for instantiating a model in the terrain.
	*/
	unsigned getMaximumGoodness(std::vector<vec4>& goodness);

	/**
	*	@return Best suited position to place the model.
	*/
	vec4 getPosition(vec2& selectedRandomPos);

	/**
	*	@return Uniform random value.
	*/
	static float getRandomRotation() { RandomUtilities::initializeUniformDistribution(.0f, 1.0f); return RandomUtilities::getUniformRandomValue(); }

public:
	/**
	*	@brief Constructor from a terrain object.
	*/
	TerrainModel(const std::string& filename, const std::string& textureFolder, TerrainConfiguration* terrainConfiguration, const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Deleted copy constructor.
	*/
	TerrainModel(const TerrainModel& forest) = delete;

	/**
	*	@brief Destructor.
	*/
	virtual ~TerrainModel();

	/**
	*	@brief Loads the plane data into GPU.
	*	@return Success of operation.
	*/
	virtual bool load(const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Deleted assignment operator overriding.
	*	@param vegetation Vegetation from where we need to copy attributes.
	*/
	TerrainModel& operator=(const TerrainModel& forest) = delete;

	/**
	*	@brief  
	*/
	void setElevation(const float elevation) { _elevation = elevation; }

	/**
	*	@brief Modifies the radius which is used to look for a suitable place.
	*/
	void setRadius(const unsigned radius) { _radius = radius; }
};

