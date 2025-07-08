#include "stdafx.h"
#include "WaterFBO.h"

// Initialization of static attributes
const vec3 WaterFBO::SKY_COLOR = vec3(.66f, .73f, .9f);


WaterFBO::WaterFBO(const uint16_t width, const uint16_t height):
	FBO(width, height)
{
	// Color buffers for scene reflection and refraction textures
	glGenFramebuffers(1, &_id);
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
	glGenTextures(1, &_colorBuffer);

	glBindTexture(GL_TEXTURE_2D, _colorBuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, _size.x, _size.y, 0, GL_RGB, GL_FLOAT, nullptr);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorBuffer, 0);

	// Depth buffer
	glGenRenderbuffers(1, &_depthTexture);
	glBindRenderbuffer(GL_RENDERBUFFER, _depthTexture);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _size.x, _size.y);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthTexture);

	// Tell OpenGL we're using two color attachments to render the scene
	unsigned int attachments = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &attachments);

	_success = this->checkFBOstate();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

WaterFBO::~WaterFBO()
{
	glDeleteTextures(1, &_colorBuffer);
	glDeleteRenderbuffers(1, &_depthTexture);
}

bool WaterFBO::bindFBO()
{
	if (!_success)
	{
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, _id);
	glClearTexImage(_colorBuffer, 0, GL_RGB, GL_FLOAT, &SKY_COLOR[0]);

	return true;
}

void WaterFBO::modifySize(const uint16_t width, const uint16_t height)
{
	FBO::modifySize(width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, _id);
	glBindTexture(GL_TEXTURE_2D, _colorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, _size.x, _size.y, 0, GL_RGB, GL_FLOAT, nullptr);			// Using size because it's already modified

	glBindRenderbuffer(GL_RENDERBUFFER, _depthTexture);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _size.x, _size.y);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
