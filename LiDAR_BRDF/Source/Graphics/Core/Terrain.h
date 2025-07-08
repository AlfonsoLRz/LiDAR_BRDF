#pragma once

#include "DataStructures/RegularGrid.h"
#include "Graphics/Application/TerrainConfiguration.h"
#include "Graphics/Core/FBOScreenshot.h"
#include "Graphics/Core/Model3D.h"
#include "Graphics/Core/PlanarSurface.h"
#include "Graphics/Core/Water.h"

/**
*	@file Terrain.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 17/02/2020
*/

/**
*	@brief Wraps the generation of a procedural terrain.
*/
class Terrain : public PlanarSurface
{
public:
	struct Lake
	{
		AABB				_aabb;
		float				_maxObservedHeight;
		std::vector<vec3>	_points;

		/**
		*	@brief Default constructor.
		*/
		Lake() : _maxObservedHeight(FLT_MIN) {}

		/**
		*	@brief Computes the bounding box of the selected points.
		*/
		inline void computeAABB()
		{
			for (vec3& point : _points) _aabb.update(point);
		}
		
		/**
		*	@brief Inserts a new point.
		*/
		inline void pushPoint(const vec3& point)
		{
			_points.push_back(point);
		}
	};

protected:
	float*					_heightMapFloat;			//!< 
	vec2					_minMax;					//!< Boundaries of terrain (height)

	// Textures for rendering
	Material*				_material;					//!<
	Texture*				_clayTexture;				//!<
	Texture*				_snowTexture;				//!<
	Texture*				_stoneTexture;				//!<

	// Secondary models
	std::vector<Water*>		_lakes;						//!< 
	RegularGrid*			_regularGrid;				//!<

	Group3D*				_sceneGroup;				//!<

	TerrainConfiguration*	_terrainConfiguration;
	TerrainParameters*		_terrainParameters;			//!<

protected:
	/**
	*	@brief Builds basement of extrusion.
	*/
	void buildBasementMesh(const unsigned startIndex);

	/**
	*	@brief Builds lateral panels of extrusion.
	*/
	void buildLateralMeshes(const unsigned size, const unsigned startIndex, const unsigned jump, const vec3& normal, const unsigned avoidAdvance = 1);

	/**
	*	@brief Generates a new material with height and normal map.
	*/
	void createMaterial();

	/**
	*	@brief Generates normal map derived from height map.
	*/
	void createNormalMap();

	/**
	*	@brief Expands current point whether neighbors present a close height and are not already analyzed.
	*/
	void expandPoint(const vec3& currentPoint, std::queue<vec3>& pointQueue, Lake& lake, bool* analyzed);

	/**
	*	@brief Builds lateral meshes for a terrain.
	*/
	void extrudeTerrain();

	/**
	*	@brief Computes each vertex attributes.
	*	@param modelMatrix Transformation which must be applied to a plane model to render it.
	*/
	virtual void generateGeometryTopology(const mat4& modelMatrix);

	/**
	*	@brief Generates a noise map with simplex method.
	*/
	void initializeHeightMap();

	/**
	*	@brief Initialize lake models.
	*/
	void initializeLakes();

	/**
	*	@return True if x, y corresponds to a local minima point within the height map.
	*/
	bool isLocalMinima(int x, int y);

public:
	/**
	*	@brief Constructor of a terrain of any length and subdivisions.
	*	@param modelMatrix Transformation which must be applied to a plane model to render it.
	*/
	Terrain(Group3D* sceneGroup, TerrainConfiguration* terrainConfiguration, const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Deleted copy constructor.
	*/
	Terrain(const Terrain& plane) = delete;

	/**
	*	@brief Destructor.
	*/
	~Terrain();

	/**
	*	@brief 
	*/
	void detectLakes(std::vector<Lake>& lakes);

	/**
	*	@brief Renders the terrain as a set of triangles.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Loads the terrain data into GPU.
	*	@return Success of operation.
	*/
	virtual bool load(const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Deleted assignment operator overriding.
	*	@param plane PlanarSurface from where we need to copy attributes.
	*/
	Terrain& operator=(const Terrain& plane) = delete;

	/**
	*	@brief Retrieve diffuse and specular colors from textures in compute shader.
	*/
	virtual void retrieveColorsGPU();

	//  ---- Getters -----

	/**
	*	@return Dimensions of terrain.
	*/
	AABB getDimensions();

	/**
	*	@return Buffer of instantiated water models.
	*/
	std::vector<Water*> getWaterModels() { return _lakes; }
};

