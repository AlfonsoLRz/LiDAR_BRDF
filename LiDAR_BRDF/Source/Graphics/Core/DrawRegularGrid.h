#pragma once

#include "DataStructures/RegularGrid.h"
#include "Graphics/Core/Model3D.h"
#include "Graphics/Core/Terrain.h"

/**
*	@file DrawGrid.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 02/02/2021
*/

/**
*	@brief Draws a regular grid as a saturation texture and a set of lines.
*/
class DrawRegularGrid: public Model3D
{
protected:
	const static float TERRAIN_OFFSET;		//!< Displacement to adjust texture to terrain

protected:
	Texture*		_colorMap;				//!< Color of each fragment according to its saturation level
	RegularGrid*	_regularGrid;			//!< Data structure whose texture we need to render
	Texture*		_regularGridTexture;	//!< Texture with saturation levels over the regular grid
	Terrain*		_terrain;				//!< Displacement of regular grid mesh	

public:
	/**
	*	@brief Constructor from a regular grid. 
	*/
	DrawRegularGrid(Terrain* terrain, RegularGrid* grid);

	/**
	*	@brief Destructor.
	*/
	virtual ~DrawRegularGrid();

	/**
	*	@brief Renders the model as a set of triangles.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Computes the model data and sends it to GPU.
	*	@param modelMatrix Model matrix to be applied while generating geometry.
	*	@return Success of operation.
	*/
	virtual bool load(const mat4& modelMatrix = mat4(1.0f));
};

