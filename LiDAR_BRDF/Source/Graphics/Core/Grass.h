#pragma once

#include "Graphics/Application/TerrainConfiguration.h"
#include "Graphics/Core/Model3D.h"

class Terrain;

/**
*	@file Grass.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 07/02/2020
*/

/**
*	@brief Wraps the procedural vegetation which is generated over a terrain.
*/
class Grass: public Model3D
{
protected:
	TerrainConfiguration* _terrainConfiguration;

protected:
	/**
	*	@brief Generates a map which indicates the probability of placing vegetation in the terrain.
	*/
	void createVegetationMap();

	/**
	*	@brief Retrieves some valid seeds where trees can be instantiated.
	*/
	void generateRandomPositions(std::vector<vec4>& randomPositions);

	/**
	*	@brief Computes geometry & topology of grass.
	*/
	void generateVegetationModel();

	/**
	*	@brief
	*/
	virtual void generateWireframe();

public:
	/**
	*	@brief Constructor of a plane of any length and subdivisions.
	*/
	Grass(TerrainConfiguration* terrainConfiguration, const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Deleted copy constructor.
	*/
	Grass(const Grass& vegetation) = delete;

	/**
	*	@brief Destructor.
	*/
	~Grass();

	/**
	*	@brief Renders the model as a set of triangles.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the model as a set of triangles with no texture as we only want to retrieve the depth.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Loads the plane data into GPU.
	*	@return Success of operation.
	*/
	virtual bool load(const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Deleted assignment operator overriding.
	*	@param vegetation Vegetation from where we need to copy attributes.
	*/
	Grass& operator=(const Grass& vegetation) = delete;
};

