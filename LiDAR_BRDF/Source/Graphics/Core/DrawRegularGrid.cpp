#include "stdafx.h"
#include "DrawRegularGrid.h"

#include "Graphics/Application/TextureList.h"
#include "Graphics/Core/OpenGLUtilities.h"

/// [Static attributes]

const float DrawRegularGrid::TERRAIN_OFFSET = -0.08f;

/// [Public methods]

DrawRegularGrid::DrawRegularGrid(Terrain* terrain, RegularGrid* grid) : Model3D(), _regularGrid(grid), _terrain(terrain), _colorMap(nullptr), _regularGridTexture(nullptr)
{
}

DrawRegularGrid::~DrawRegularGrid()
{
	delete _regularGridTexture;
}

void DrawRegularGrid::drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	shader->setUniform("width", _terrain->getSize().x);
	shader->setUniform("depth", _terrain->getSize().y);
	shader->setUniform("terrainDisp", Terrain::getTerrainParameters()->_terrainMaxHeight + TERRAIN_OFFSET);
	shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * _terrain->getModelMatrix());
	
	{
		unsigned textureIndex = 0;
		
		_terrain->getHeightMap()->applyTexture(shader, textureIndex++, "heightMap");
		_regularGridTexture->applyTexture(shader, textureIndex++, "densityMap");
		_colorMap->applyTexture(shader, textureIndex++, "colorMap");
	}

	_modelComp[0]->_vao->drawObject(RendEnum::IBO_TRIANGLE_MESH, GL_TRIANGLES, _modelComp[0]->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH]);
}

bool DrawRegularGrid::load(const mat4& modelMatrix)
{
	if (!_loaded)
	{
		const unsigned numSubdivisions = _regularGrid->getNumSubdivisions();

		_modelComp[0]->_vao = Primitives::getNormalizedMeshVAO(numSubdivisions, _modelComp[0]->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH]);
		_colorMap = TextureList::getInstance()->getTexture(CGAppEnum::TEXTURE_LIDAR_HEIGHT);
		_regularGridTexture = _regularGrid->getDensityTexture();
		
		_loaded = true;
	}
	
	return _loaded;
}
