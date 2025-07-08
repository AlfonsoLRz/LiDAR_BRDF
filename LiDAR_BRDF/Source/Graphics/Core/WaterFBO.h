#pragma once

#include "Graphics/Core/FBO.h"

/**
*	@file Texture.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 07/18/2019
*/

/**
*	@brief Wraps an image which takes part in a material.
*/
class WaterFBO: public FBO
{
protected:
	const static vec3 SKY_COLOR;							//!< Default color of texture 

protected:
	GLuint		_colorBuffer;								//!< Refraction or reflection buffers
	GLuint		_depthTexture;								//!< Identifier on GPU of depth texture for main FBO

public:
	/**
	*	@brief Constructor.
	*	@param width Width of textures and framebuffers.
	*	@param height Height of textures and framebuffers.
	*/
	WaterFBO(const uint16_t width, const uint16_t height);

	/**
	*	@brief Destructor.
	*/
	virtual ~WaterFBO();

	/**
	*	@brief Binds the framebuffer so it gets active.
	*/
	virtual bool bindFBO();

	/**
	*	@return Identifier of color buffer.
	*/
	GLuint getColorBuffer() { return _colorBuffer; }

	/**
	*	@brief Modifies the size specified in textures related to framebuffer.
	*	@param width New width.
	*	@param height New height.
	*/
	virtual void modifySize(const uint16_t width, const uint16_t height);
};

