#pragma once

#include "FBOScreenshot.h"
#include "PlanarSurface.h"
#include "WaterFBO.h"

class TerrainScene;

/**
*	@file Water.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 07/17/2020
*/

#define ANIMATION_LAMBDA 9e-4

/**
*	@brief Wraps lake-like water.
*/
class Water: public PlanarSurface
{
protected:
	ivec2				_canvasSize;					//!< Recovers previous state
	WaterFBO*			_terrainFBO;					//!< Terrain texture

	// Animation
	Texture*			_dudvMap;						//!< Distortion texture
	float				_t;								//!< [0, infinity]; controls the movement in the texture to simulate time

protected:
	/**
	*	@brief Defines those variables which are necessary for shaders. 
	*/
	void definedShaderUniforms(RenderingShader* shader);

public:
	/**
	*	@brief Constructor of an ocean which is adjustable by means of world size and number of subdivisions.
	*	@param width Ocean width.
	*	@param height Ocean depth.
	*	@param tiling Number of horizontal subdivisions.
	*	@param modelMatrix Transformation which must be applied to the ocean to render it.
	*/
	Water(const float width = 1.0f, const float depth = 1.0f, const uint16_t tilingH = 1, const uint16_t tilingV = 1, const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Deleted copy constructor.
	*/
	Water(const Water& ocean) = delete;

	/**
	*	@brief Destructor.
	*/
	~Water();
	
	/**
	*	@brief Bind the default framebuffer or the FBO dedicated to retrieve the reflection texture.
	*/
	void bindFrameBuffer(const bool defaultFramebuffer = true);

	/**
	*	@brief Renders the model as a set of triangles.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the points captured by a sensor (coloured).
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawCapturedPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Loads the plane data into GPU.
	*	@return Success of operation.
	*/
	virtual bool load(const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Deleted assignment operator overriding.
	*	@paramocean Water object from where we need to copy attributes.
	*/
	Water& operator=(const Water& ocean) = delete;

	/**
	*	@brief Updates the attribute t to modify the textures.
	*/
	void updateAnimation();
};

