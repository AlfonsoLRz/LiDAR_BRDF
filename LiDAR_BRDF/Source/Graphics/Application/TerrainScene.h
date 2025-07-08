#pragma once

#include "DataStructures/RegularGrid.h"
#include "Graphics/Application/LiDARScene.h"
#include "Graphics/Application/TerrainConfiguration.h"
#include "Graphics/Core/Forest.h"
#include "Graphics/Core/Grass.h"
#include "Graphics/Core/PlanarSurface.h"
#include "Graphics/Core/Terrain.h"
#include "Graphics/Core/TerrainModel.h"
#include "Graphics/Core/Water.h"

/**
*	@file TerrainScene.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 17/02/2020
*/

/**
*	@brief Scenario of a terrain for aerial LiDARs.
*/
class TerrainScene: public LiDARScene
{
protected:
	TerrainConfiguration		_terrainConfiguration;			

	// [[Models]]
	std::vector<TerrainModel*>	_fireTower;						//!< Model to be instantiated in a suitable random point
	Forest*						_forest;						//!< Vector of trees
	Grass*						_grass;							//!< Low vegetation		
	Terrain*					_terrain;						//!< Base of this map
	std::vector<TerrainModel*>	_transmissionTower;				//!< Same than fire tower
	std::vector<Water*>			_water;							//!< Lakes

protected:
	/**
	*	@brief Renders the model as a set of triangles.
	*	@brief
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void drawAsTriangles(Camera* camera, const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders the scene as a set of triangles where their colors depends on the semantic group which they belong to.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void drawAsTrianglesWithGroups(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders the scene as shadowed triangles.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void drawAsTriangles4Position(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders the scene as shadowed triangles.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void drawAsTriangles4Normal(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Draws the terrain to be captured by reflection/refraction, therefore water must not be rendered.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void drawTerrain(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders the model as a set of triangles with no textures, except those which change the mesh shape (disp. mapping).
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void drawAsTriangles4Shadows(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders the water textures (reflection & refraction).
	*/
	void generateWaterTextures(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Loads the cameras where the scene is seen from.
	*/
	virtual void loadCameras();

	/**
	*	@brief Loads the lights which affects the scene.
	*/
	virtual void loadLights();

	/**
	*	@brief Builds the scene.
	*/
	virtual void loadModels();

	// -------- Render --------

	/**
	*	@brief Decides which objects are going to be rendered as a wireframe mesh.
	*	@param shader Rendering shader which is drawing the scene.
	*	@param shaderType Unique ID of "shader".
	*	@param matrix Vector of matrices, including view, projection, etc.
	*	@param rendParams Parameters which indicates how the scene is rendered.
	*/
	virtual void drawSceneAsLines(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams);

public:
	/**
	*	@brief Destructor.
	*/
	virtual ~TerrainScene();

	/**
	*	@brief 
	*/
	void loadTerrainConfiguration(const std::string& filename);

	/**
	*	@brief Draws the scene as the rendering parameters specifies.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void render(const mat4& mModel, RenderingParameters* rendParams);
};

