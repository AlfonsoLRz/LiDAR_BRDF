#pragma once

#include "stdafx.h"
#include "Graphics/Application/GraphicsAppEnumerations.h"
#include "Utilities/FileManagement.h"

/**
*	@file TerrainParameters.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 13/02/2021
*/

/**
*	@brief Wraps those parameters which defines the terrain aspect.
*/
struct TerrainParameters
{
public:
	inline static const std::string BINARY_LOCATION = "Assets/Scene/TerrainEnvironment/BinaryDescription.bin";
	inline static std::vector<CGAppEnum::MaterialNames>	CANOPY_MATERIALS = { CGAppEnum::MATERIAL_LEAVES_01, CGAppEnum::MATERIAL_LEAVES_01, CGAppEnum::MATERIAL_LEAVES_01 };
	inline static const GLuint							NUM_TREES = 3;
	inline static GLuint								TEXTURE_START_INDEX = 15;
	inline static std::vector<bool>						TREE_HAS_LEAVES = { true, true, true };
	inline static std::vector<std::string>				TREE_PATH = { "Assets/Models/Trees/Tree01/", "Assets/Models/Trees/Tree02/", "Assets/Models/Trees/Tree03/" };
	inline static std::vector<CGAppEnum::MaterialNames>	TRUNK_MATERIALS = { CGAppEnum::MATERIAL_TRUNK_01, CGAppEnum::MATERIAL_TRUNK_01, CGAppEnum::MATERIAL_TRUNK_01 };

protected:
	// Loading configuration file
	inline static const char LINE_COMMENT_CHAR = '#';

public:
	// Terrain
	vec2		_regularGridSubdivisions;
	unsigned	_mapSize;
	float		_terrainExtrusion;
	float		_terrainMaxHeight;
	vec2		_terrainSize;
	uvec2		_terrainSubdivisions;

	// Water
	GLuint		_numLakes;
	float		_waterHeight;
	float		_waterHeightVariance;
	GLuint		_waterSubdivisions;

	// DEM
	std::string _heightMapFile;
	float		_normalMapDepth;

	// Terrain Rendering
	float		_grassTextureScale;
	float		_snowTextureScale;
	float		_rockTextureScale;
	float		_rockWeightFactor;
	GLuint		_textureStartIndex;

	// Grass
	vec2		_grassHeightThreshold;
	float		_grassHeightWeight;
	GLuint		_numGrassSeeds;
	float		_grassSlopeWeight;
	GLuint		_grassTextureSize;
	float		_grassThreshold;
	vec2		_grassScale;

	// Trees
	float		_maxTreeScale[NUM_TREES];
	float		_minTreeScale[NUM_TREES];
	GLuint		_numTrees[NUM_TREES];
	float		_treeVerticalOffset;

	// Terrain Models
	unsigned	_modelNumParticles;
	unsigned	_modelSaturationLevel;
	unsigned	_numTransmissionTowers;
	unsigned	_numWatchers;
	unsigned	_transmissionTowerRadius;
	unsigned	_watcherRadius;

	// ---- Methods -----

	/**
	*	@brief Assigns retrieved values to a new variable.
	*/
	static void assignFloatValues(std::vector<float>& source, float* target, int size);

	/**
	*	@brief Assigns retrieved values to a new variable.
	*/
	static void assignFloatValues(std::vector<float>& source, GLuint* target, int size);

	/**
	*	@return Number of trees that can be instanced (globally, instead of for each type). 
	*/
	GLuint getNumTrees();

	/**
	*	@brief Loads default configuration file.
	*/
	bool loadConfiguration(const std::string& filename);

	/**
	*	@brief Loads binary description of terrain, if exists, onto these parameters. 
	*/
	void loadTerrainParameters();
};

inline void TerrainParameters::assignFloatValues(std::vector<float>& source, float* target, int size)
{
	if (source.size() < size) return;

	for (int idx = 0; idx < size; ++idx)
	{
		target[idx] = source[idx];
	}
}

inline void TerrainParameters::assignFloatValues(std::vector<float>& source, GLuint* target, int size)
{
	if (source.size() < size) return;

	for (int idx = 0; idx < size; ++idx)
	{
		target[idx] = source[idx];
	}
}

inline GLuint TerrainParameters::getNumTrees()
{
	GLuint sum = 0;
	for (GLuint count : _numTrees) sum += count;

	return sum;
}

inline bool TerrainParameters::loadConfiguration(const std::string& filename)
{
	std::string currentLine, lineHeader;
	std::stringstream line;
	std::ifstream inputStream;
	std::vector<float> floatValues;
	std::vector<std::string> strValues;

	inputStream.open(filename.c_str());
	if (inputStream.fail()) return false;

	while (!(inputStream >> std::ws).eof())
	{
		std::getline(inputStream, currentLine);

		line.clear();
		line.str(currentLine);
		std::getline(line, lineHeader, ' ');

		if (lineHeader.find(LINE_COMMENT_CHAR) == std::string::npos)
		{
			floatValues.clear();
			strValues.clear();
			FileManagement::readTokens(currentLine, ' ', strValues, floatValues);

			if (lineHeader._Starts_with("MAP_SIZE"))
			{
				assignFloatValues(floatValues, &_mapSize, 1);
			}
			else if (lineHeader._Starts_with("REGULAR_GRID_SUBDIVISIONS"))
			{
				assignFloatValues(floatValues, &_regularGridSubdivisions[0], 2);
			}
			else if (lineHeader._Starts_with("TERRAIN_EXTRUSION"))
			{
				assignFloatValues(floatValues, &_terrainExtrusion, 1);
			}
			else if (lineHeader._Starts_with("TERRAIN_MAX_HEIGHT"))
			{
				assignFloatValues(floatValues, &_terrainMaxHeight, 1);
			}
			else if (lineHeader._Starts_with("TERRAIN_SIZE"))
			{
				assignFloatValues(floatValues, &_terrainSize[0], 1);
				_terrainSize[1] = _terrainSize[0];
			}
			else if (lineHeader._Starts_with("TERRAIN_SUBDIVISIONS"))
			{
				assignFloatValues(floatValues, &_terrainSubdivisions[0], 2);
			}
			else if (lineHeader._Starts_with("MAX_NUM_LAKES"))
			{
				assignFloatValues(floatValues, &_numLakes, 1);
			}
			else if (lineHeader._Starts_with("WATER_HEIGHT"))
			{
				assignFloatValues(floatValues, &_waterHeight, 1);
			}
			else if (lineHeader._Starts_with("WATER_VARIANCE"))
			{
				assignFloatValues(floatValues, &_waterHeightVariance, 1);
			}
			else if (lineHeader._Starts_with("WATER_SUBDIVISIONS"))
			{
				assignFloatValues(floatValues, &_waterSubdivisions, 1);
			}
			else if (lineHeader._Starts_with("GRASS_TEXTURE_SCALE"))
			{
				assignFloatValues(floatValues, &_grassTextureScale, 1);
			}
			else if (lineHeader._Starts_with("SNOW_TEXTURE_SCALE"))
			{
				assignFloatValues(floatValues, &_snowTextureScale, 1);
			}
			else if (lineHeader._Starts_with("ROCK_TEXTURE_SCALE"))
			{
				assignFloatValues(floatValues, &_rockTextureScale, 1);
			}
			else if (lineHeader._Starts_with("ROCK_WEIGHT_FACTOR"))
			{
				assignFloatValues(floatValues, &_rockWeightFactor, 1);
			}
			else if (lineHeader._Starts_with("GRASS_HEIGHT_THRESHOLD"))
			{
				assignFloatValues(floatValues, &_grassHeightThreshold[0], 2);
			}
			else if (lineHeader._Starts_with("GRASS_HEIGHT_WEIGHT"))
			{
				assignFloatValues(floatValues, &_grassHeightWeight, 1);
			}
			else if (lineHeader._Starts_with("NUM_GRASS_SEEDS"))
			{
				assignFloatValues(floatValues, &_numGrassSeeds, 1);
			}
			else if (lineHeader._Starts_with("GRASS_SLOPE_WEIGHT"))
			{
				assignFloatValues(floatValues, &_grassSlopeWeight, 1);
			}
			else if (lineHeader._Starts_with("GRASS_TEXTURE_SIZE"))
			{
				assignFloatValues(floatValues, &_grassTextureSize, 1);
			}
			else if (lineHeader._Starts_with("GRASS_THRESHOLD"))
			{
				assignFloatValues(floatValues, &_grassThreshold, 1);
			}
			else if (lineHeader._Starts_with("GRASS_SCALE"))
			{
				assignFloatValues(floatValues, &_grassScale[0], 2);
			}
			else if (lineHeader._Starts_with("MAX_TREE_SCALE"))
			{
				assignFloatValues(floatValues, &_maxTreeScale[0], NUM_TREES);
			}
			else if (lineHeader._Starts_with("MIN_TREE_SCALE"))
			{
				assignFloatValues(floatValues, &_minTreeScale[0], NUM_TREES);
			}
			else if (lineHeader._Starts_with("TREE_VERTICAL_OFFSET"))
			{
				assignFloatValues(floatValues, &_treeVerticalOffset, 1);
			}
			else if (lineHeader._Starts_with("NUM_TREES"))
			{
				assignFloatValues(floatValues, &_numTrees[0], NUM_TREES);
			}
			else if (lineHeader._Starts_with("NUM_WATCHER"))
			{
				assignFloatValues(floatValues, &_numWatchers, 1);
			}
			else if (lineHeader._Starts_with("NUM_TRANSMISSION_TOWER"))
			{
				assignFloatValues(floatValues, &_numTransmissionTowers, 1);
			}
			else if (lineHeader._Starts_with("MODEL_NUM_PARTICLES"))
			{
				assignFloatValues(floatValues, &_modelNumParticles, 1);
			}
			else if (lineHeader._Starts_with("TRANSMISSION_TOWER_RADIUS"))
			{
				assignFloatValues(floatValues, &_transmissionTowerRadius, 1);
			}
			else if (lineHeader._Starts_with("WATCHER_RADIUS"))
			{
				assignFloatValues(floatValues, &_watcherRadius, 1);
			}
			else if (lineHeader._Starts_with("MODEL_SATURATION_LEVEL"))
			{
				assignFloatValues(floatValues, &_modelSaturationLevel, 1);
			}
			else if (lineHeader._Starts_with("HEIGHTMAP_FILE"))
			{
				_heightMapFile = strValues[1];
			}
			else if (lineHeader._Starts_with("NORMAL_MAP_DEPTH"))
			{
				assignFloatValues(floatValues, &_normalMapDepth, 1);
			}
		}
	}

	inputStream.close();

	return true;
}

inline void TerrainParameters::loadTerrainParameters()
{
	std::ifstream fin(BINARY_LOCATION, std::ios::in | std::ios::binary);
	if (!fin.is_open()) return;

	TerrainParameters parameters;
	fin.read((char*)&parameters, sizeof(TerrainParameters));
	fin.close();

	// Modify static parameters according to binary description
	*this = parameters;
}

