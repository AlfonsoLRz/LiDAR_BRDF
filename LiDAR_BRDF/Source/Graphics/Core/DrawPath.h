#pragma once

#include "Geometry/Animation/CatmullRom.h"
#include "Geometry/Animation/Interpolation.h"
#include "Graphics/Core/Model3D.h"

/**
*	@file LiDARScene.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 07/12/2019
*/

/**
*	@brief Generic scene for LiDAR simulation.
*/
class DrawPath: public Model3D
{
protected:
	std::vector<CatmullRom*> _interpolation;

public:
	/**
	*	@brief Constructor.
	*	@param modelMatrix First model transformation.
	*/
	DrawPath(const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Destructor.
	*/
	~DrawPath();

	/**
	*	@brief Renders the AABB as a set of lines.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsLines(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Computes the AABB data and sends it to GPU.
	*	@return Success of operation.
	*/
	virtual bool load(const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Updates the path.
	*/
	void updateInterpolation(std::vector<vec2>& path, float height, const vec2& canvasSize, const AABB& aabb);

	/**
	*	@brief Updates the path.
	*/
	void updateInterpolation(const std::vector<Interpolation*>& paths);
};

