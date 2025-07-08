#pragma once

#include "DataStructures/RegularGrid.h"
#include "Graphics/Application/TerrainParameters.h"
#include "Graphics/Core/Texture.h"


class TerrainConfiguration
{
public:
	Texture* _heightMap, * _normalMap, * _vegetationMap;
	RegularGrid* _grid;
	TerrainParameters	_terrainParameters;

public:
	TerrainConfiguration() : _heightMap(nullptr), _normalMap(nullptr), _vegetationMap(nullptr) {}

	virtual ~TerrainConfiguration()
	{
		delete _heightMap;
		delete _normalMap;
		delete _vegetationMap;
		delete _grid;
	}
};
