#pragma once

#include "DataStructures/RegularGrid.h"
#include "Graphics/Application/TerrainParameters.h"
#include "Graphics/Core/CADModel.h"
#include "Graphics/Core/Grass.h"
#include "Graphics/Core/Model3D.h"
#include "Graphics/Core/Terrain.h"

/**
*	@file Forest.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 08/25/2020
*/

/**
*	@brief Set of trees divided into canopy and trunk.
*/
class Forest: public Model3D
{
protected:
	enum ForestModelType: uint8_t
	{
		TRUNK, CANOPY, NUM_MODEL_TYPES
	};

protected:
	std::vector<CADModel*>	_forestModel[NUM_MODEL_TYPES];					//!< Trunk and canopy models which are later instantiated multiple times
	std::vector<GLuint>		_numVertices[NUM_MODEL_TYPES];					//!< Number of vertices of each canopy/trunk model
	std::vector<GLuint>		_numTrees;										//!< Number of instantiated trees

	TerrainConfiguration*	_terrainConfiguration;							//!<

protected:
	/**
	*	@brief Generates the position, rotation and scaling vectors for every tree.
	*/
	void computeTreeMap();

	/**
	*	@brief Indicates those properties which are neccessary for both model components either for rendering and LiDAR simulation.
	*/
	void defineModelComponentProperties();

	/**
	*	@brief Retrieves some valid seeds where trees can be instantiated.
	*/
	void generateRandomPositions(std::vector<vec4>& randomPositions);

	/**
	*	@brief Generates the geometry and topology of the whole set of trees, this way we can intersect them with rays.
	*/
	void generateTreesGeometryTopology(const GLuint positionBufferID, const GLuint rotationBufferID, const GLuint scaleBufferID, const unsigned numTrees, CADModel* model, Model3D::ModelComponent* refModelComp);

	/**
	*	@brief Loads all the type of models which could be instantiated.
	*/
	void loadForestModels();

public:
	/**
	*	@brief Constructor of a plane of any length and subdivisions.
	*/
	Forest(TerrainConfiguration* terrainConfiguration, const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Deleted copy constructor.
	*/
	Forest(const Forest& forest) = delete;

	/**
	*	@brief Destructor.
	*/
	~Forest();

	/**
	*	@brief Renders the trees as a set of triangles.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the model as a set of triangles whose color depends on their semantic group.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTrianglesWithGroup(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, bool ASPRS = true);

	/**
	*	@brief Renders the trees as a set of triangles with no texture as we only want to retrieve the depth.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders those points captured from a LiDAR sensor corresponding to leaves as RGB points.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawCapturedPointsLeaves(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the points of trunks captured by a sensor (coloured).
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawCapturedPointsTrunks(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Loads the plane data into GPU.
	*	@return Success of operation.
	*/
	virtual bool load(const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Deleted assignment operator overriding.
	*	@param vegetation Grass from where we need to copy attributes.
	*/
	Forest& operator=(const Forest& forest) = delete;
};

