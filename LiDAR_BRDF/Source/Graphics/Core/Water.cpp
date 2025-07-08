#include "stdafx.h"
#include "Water.h"

#include "Graphics/Application/MaterialList.h"
#include "Graphics/Application/TerrainScene.h"
#include "Graphics/Application/TextureList.h"
#include "Graphics/Core/OpenGLUtilities.h"
#include "Interface/Window.h"

/// Public methods

Water::Water(const float width, const float depth, const uint16_t tilingH, const uint16_t tilingV, const mat4& modelMatrix) :
	PlanarSurface(width, depth, tilingH, tilingV, 1.0f, 1.0f, modelMatrix), _t(.0f)
{
	_dudvMap = TextureList::getInstance()->getTexture(CGAppEnum::TEXTURE_WATER_DUDV_MAP);
	_terrainFBO = new WaterFBO(512, 512);
}

Water::~Water()
{
	delete _terrainFBO;
}

void Water::bindFrameBuffer(bool defaultFramebuffer)
{
	if (!defaultFramebuffer)
	{
		_canvasSize = Window::getInstance()->getSize();

		glClear(GL_COLOR_BUFFER_BIT);
		_terrainFBO->bindFBO();
		glClear(GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, _terrainFBO->getSize().x, _terrainFBO->getSize().y);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, _canvasSize.x, _canvasSize.y);
	}
}

void Water::drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	this->definedShaderUniforms(shader);
	this->renderTriangles(shader, shaderType, matrix, _modelComp[0], GL_TRIANGLE_STRIP);
}

void Water::drawCapturedPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	this->definedShaderUniforms(shader);
	this->renderCapturedPoints(shader, shaderType, matrix, _modelComp[0], GL_POINTS);
}

bool Water::load(const mat4& modelMatrix)
{
	if (!_loaded)
	{
		PlanarSurface::load(modelMatrix);

		this->_modelComp[0]->_material = MaterialList::getInstance()->getMaterial(CGAppEnum::MATERIAL_WATER);
		this->_modelComp[0]->setMaterial(MaterialDatabase::WATER);

		_loaded = true;
	}

	return false;
}

void Water::updateAnimation()
{
	_t += ANIMATION_LAMBDA;
}

/// [Protected methods]

void Water::definedShaderUniforms(RenderingShader* shader)
{
	const GLint textureStartIndex = 10;
	
	{
		shader->setUniform("texTerrain", textureStartIndex);
		glActiveTexture(GL_TEXTURE0 + textureStartIndex);
		glBindTexture(GL_TEXTURE_2D, _terrainFBO->getColorBuffer());
	}

	_dudvMap->applyTexture(shader, textureStartIndex + 1, "texReflectionNoise");
	shader->setUniform("t", _t);
}