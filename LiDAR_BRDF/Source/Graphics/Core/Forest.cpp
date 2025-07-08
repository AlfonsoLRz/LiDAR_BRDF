#include "stdafx.h"
#include "Forest.h"

#include "DataStructures/RegularGrid.h"
#include "Graphics/Application/MaterialList.h"
#include "Graphics/Core/Group3D.h"
#include "Graphics/Core/OpenGLUtilities.h"
#include "Graphics/Core/ShaderList.h"
#include "Utilities/ChronoUtilities.h"
#include "Utilities/RandomUtilities.h"

/// Initialization of static attributes

/// Public methods

Forest::Forest(TerrainConfiguration* terrainConfiguration, const mat4& modelMatrix) :
	Model3D(modelMatrix, 2), _terrainConfiguration(terrainConfiguration)
{
}

Forest::~Forest()
{
	for (int modelType = TRUNK; modelType < NUM_MODEL_TYPES; ++modelType)
	{
		for (CADModel* canopy : _forestModel[modelType])
		{
			delete canopy;
		}
	}
}

void Forest::drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	ModelComponent* modelComp;

	for (int tree = 0; tree < _forestModel[TRUNK].size(); ++tree)
	{
		if (!_numTrees[tree]) continue;

		this->setShaderUniforms(shader, shaderType, matrix);

		if (_modelComp[0]->_enabled)
		{
			modelComp = _forestModel[TRUNK][tree]->getModelComponent(0);
			modelComp->_material->applyMaterial(shader);
			modelComp->_vao->drawObject(RendEnum::IBO_TRIANGLE_MESH, GL_TRIANGLES, modelComp->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH], _numTrees[tree]);
		}

		if (_forestModel[CANOPY][tree] && _modelComp[1]->_enabled)
		{
			modelComp = _forestModel[CANOPY][tree]->getModelComponent(0);
			modelComp->_material->applyMaterial(shader);
			modelComp->_vao->drawObject(RendEnum::IBO_TRIANGLE_MESH, GL_TRIANGLES, modelComp->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH], _numTrees[tree]);
		}
	}
}

void Forest::drawAsTrianglesWithGroup(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, bool ASPRS)
{
	if (shaderType != RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_GROUP_SHADER) return;
	
	ModelComponent* modelComp;

	for (int tree = 0; tree < _forestModel[TRUNK].size(); ++tree)
	{
		if (!_numTrees[tree]) continue;

		this->setShaderUniforms(shader, shaderType, matrix);

		if (_modelComp[0]->_enabled)
		{
			modelComp = _forestModel[TRUNK][tree]->getModelComponent(0);
			if (modelComp->_material) modelComp->_material->applyMaterial4UniformColor(shader);
			if (!ASPRS && _modelComp[0]->_semanticGroup != -1) shader->setUniform("kadColor", _groupColor[_modelComp[0]->_semanticGroup]);
			else if (ASPRS && _modelComp[0]->_asprsSemanticGroup != -1) shader->setUniform("kadColor", _asprsGroupColor[_modelComp[0]->_asprsSemanticGroup]);

			modelComp->_vao->drawObject(RendEnum::IBO_TRIANGLE_MESH, GL_TRIANGLES, modelComp->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH], _numTrees[tree]);
		}

		if (_forestModel[CANOPY][tree] && _modelComp[1]->_enabled)
		{
			modelComp = _forestModel[CANOPY][tree]->getModelComponent(0);
			if (modelComp->_material) modelComp->_material->applyMaterial4UniformColor(shader);
			if (!ASPRS && _modelComp[1]->_semanticGroup != -1) shader->setUniform("kadColor", _groupColor[_modelComp[1]->_semanticGroup]);
			else if (ASPRS && _modelComp[1]->_asprsSemanticGroup != -1) shader->setUniform("kadColor", _asprsGroupColor[_modelComp[1]->_asprsSemanticGroup]);
			
			modelComp->_vao->drawObject(RendEnum::IBO_TRIANGLE_MESH, GL_TRIANGLES, modelComp->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH], _numTrees[tree]);
		}
	}
}

void Forest::drawAsTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	ModelComponent* modelComp;

	for (int tree = 0; tree < _forestModel[TRUNK].size(); ++tree)
	{
		if (!_numTrees[tree]) continue;

		this->setShaderUniforms(shader, shaderType, matrix);

		if (_modelComp[0]->_enabled)
		{
			modelComp = _forestModel[TRUNK][tree]->getModelComponent(0);
			modelComp->_material->applyAlphaTexture(shader);
			modelComp->_vao->drawObject(RendEnum::IBO_TRIANGLE_MESH, GL_TRIANGLES, modelComp->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH], _numTrees[tree]);
		}

		if (_forestModel[CANOPY][tree] && _modelComp[1]->_enabled)
		{
			modelComp = _forestModel[CANOPY][tree]->getModelComponent(0);
			modelComp->_material->applyAlphaTexture(shader);
			modelComp->_vao->drawObject(RendEnum::IBO_TRIANGLE_MESH, GL_TRIANGLES, modelComp->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH], _numTrees[tree]);
		}
	}
}

void Forest::drawCapturedPointsLeaves(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	this->renderCapturedPoints(shader, shaderType, matrix, _modelComp[1], GL_POINTS);
}

void Forest::drawCapturedPointsTrunks(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	this->renderCapturedPoints(shader, shaderType, matrix, _modelComp[0], GL_POINTS);
}

bool Forest::load(const mat4& modelMatrix)
{
	if (!_loaded)
	{
		this->loadForestModels();
		this->computeTreeMap();
		//this->generatePointCloud();
		//this->generateWireframe();
		this->setVAOData();
		
		_loaded = true;
	}

	return _loaded;
}

/// Protected methods

void Forest::computeTreeMap()
{
	std::vector<vec4> randomPositions;
	Texture* vegetationMap = _terrainConfiguration->_vegetationMap, *heightMap = _terrainConfiguration->_heightMap;
	TerrainParameters* terrainParameters = &_terrainConfiguration->_terrainParameters;
	unsigned offset = 0, trunkGeometryOffset = 0, canopyGeometryOffset = 0, instantiatedTrees;
	const vec2 terrainSize = _terrainConfiguration->_terrainParameters._terrainSize;

	this->generateRandomPositions(randomPositions);

	// GPU Buffers
	const GLuint numTrees				= terrainParameters->getNumTrees();
	const GLuint randomPosBufferID		= ComputeShader::setReadBuffer(randomPositions, GL_STATIC_DRAW);
	const GLuint positionDataBufferID	= ComputeShader::setWriteBuffer(vec4(), numTrees, GL_STREAM_DRAW);
	const GLuint scaleBufferID			= ComputeShader::setWriteBuffer(vec4(), numTrees, GL_STREAM_DRAW);
	const GLuint rotationBufferID		= ComputeShader::setWriteBuffer(vec4(), numTrees, GL_STREAM_DRAW);
	const GLuint instantiatedTreesID	= ComputeShader::setReadBuffer(&instantiatedTrees, 1, GL_DYNAMIC_DRAW);
	ComputeShader* shader				= ShaderList::getInstance()->getComputeShader(RendEnum::COMPUTE_TREE_PROPERTIES);
	
	for (int tree = 0; tree < terrainParameters->NUM_TREES; ++tree)
	{
		if (!terrainParameters->_numTrees[tree])
		{
			_numTrees.push_back(0);
			continue;
		}

		instantiatedTrees = 0;

		const int numGroups = ComputeShader::getNumGroups(terrainParameters->_numTrees[tree]);
		ComputeShader::updateReadBuffer(instantiatedTreesID, &instantiatedTrees, 1, GL_DYNAMIC_DRAW);

		shader->bindBuffers(std::vector<GLuint> { positionDataBufferID, scaleBufferID, rotationBufferID, randomPosBufferID, instantiatedTreesID });
		shader->use();
		shader->setUniform("instanceBoundary", .5f);
		shader->setUniform("numTrees", terrainParameters->_numTrees[tree]);
		shader->setUniform("offset", offset);
		shader->setUniform("maxScale", terrainParameters->_maxTreeScale[tree]);
		shader->setUniform("minScale", terrainParameters->_minTreeScale[tree]);
		shader->setUniform("modelMatrix", _modelMatrix);
		shader->setUniform("width", terrainSize.x);
		shader->setUniform("depth", terrainSize.y);
		shader->setUniform("terrainDisp", terrainParameters->_terrainMaxHeight);
		vegetationMap->applyTexture(shader, 0, "vegetationMap");
		heightMap->applyTexture(shader, 1, "heightMap");
		shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		instantiatedTrees = *ComputeShader::readData(instantiatedTreesID, GLuint());
		_numTrees.push_back(instantiatedTrees);

		vec4* positionData = shader->readData(positionDataBufferID, vec4());
		std::vector<vec4> posVector = std::move(std::vector<vec4>(positionData, positionData + instantiatedTrees));

		vec4* scaleData = shader->readData(scaleBufferID, vec4());
		std::vector<vec4> scaleVector = std::move(std::vector<vec4>(scaleData, scaleData + instantiatedTrees));

		vec4* rotationData = shader->readData(rotationBufferID, vec4());
		std::vector<vec4> rotationVector = std::move(std::vector<vec4>(rotationData, rotationData + instantiatedTrees));
		
		_forestModel[TRUNK][tree]->getModelComponent(0)->_vao->setVBOData(RendEnum::VBO_OFFSET, posVector, GL_STATIC_DRAW);
		_forestModel[TRUNK][tree]->getModelComponent(0)->_vao->setVBOData(RendEnum::VBO_SCALE, scaleVector, GL_STATIC_DRAW);
		_forestModel[TRUNK][tree]->getModelComponent(0)->_vao->setVBOData(RendEnum::VBO_ROTATION, rotationVector, GL_STATIC_DRAW);

		this->generateTreesGeometryTopology(positionDataBufferID, rotationBufferID, scaleBufferID, instantiatedTrees, _forestModel[TRUNK][tree], _modelComp[0]);

		if (_forestModel[CANOPY][tree])
		{
			_forestModel[CANOPY][tree]->getModelComponent(0)->_vao->setVBOData(RendEnum::VBO_OFFSET, posVector, GL_STATIC_DRAW);
			_forestModel[CANOPY][tree]->getModelComponent(0)->_vao->setVBOData(RendEnum::VBO_SCALE, scaleVector, GL_STATIC_DRAW);
			_forestModel[CANOPY][tree]->getModelComponent(0)->_vao->setVBOData(RendEnum::VBO_ROTATION, rotationVector, GL_STATIC_DRAW);

			this->generateTreesGeometryTopology(positionDataBufferID, rotationBufferID, scaleBufferID, instantiatedTrees, _forestModel[CANOPY][tree], _modelComp[1]);
		}

		offset += terrainParameters->_numTrees[tree];
	}

	GLuint buffers[] = { positionDataBufferID, scaleBufferID, rotationBufferID, instantiatedTreesID };
	glDeleteBuffers(4, buffers);

	this->defineModelComponentProperties();
}

void Forest::defineModelComponentProperties()
{
	MaterialList* materialList = MaterialList::getInstance();

	this->_modelComp[0]->_material = materialList->getMaterial(CGAppEnum::MATERIAL_TRUNK_01);
	this->_modelComp[1]->_material = materialList->getMaterial(CGAppEnum::MATERIAL_LEAVES_01);

	this->_modelComp[0]->setMaterial(MaterialDatabase::WOOD);
	this->_modelComp[1]->setMaterial(MaterialDatabase::LEAF);

	this->setName("Trunks");
	this->setName("Canopy");

	this->setSemanticGroup("Trunk", 0);
	this->setSemanticGroup("Canopy", 1);

	this->setSemanticGroup(LiDARParameters::HIGH_VEGETATION);
}

void Forest::generateRandomPositions(std::vector<vec4>& randomPositions)
{
	RandomUtilities::initializeUniformDistribution(.0f, 1.0f);
	
	for (int tree = 0; tree < _terrainConfiguration->_terrainParameters.getNumTrees(); ++tree)
	{
		vec2 position = vec2(RandomUtilities::getUniformRandomValue(), RandomUtilities::getUniformRandomValue());

		if (!_terrainConfiguration->_grid->isSaturated(position, .0f))
		{
			randomPositions.push_back(vec4(position, .0f, .0f));
			_terrainConfiguration->_grid->insertPoint(position, 1);
		}
	}
}

void Forest::generateTreesGeometryTopology(const GLuint positionBufferID, const GLuint rotationBufferID, const GLuint scaleBufferID, const unsigned numTrees, CADModel* model, Model3D::ModelComponent* refModelComp)
{
	// Expected geometry and topology
	ModelComponent* modelComp = model->getModelComponent(0);
	const unsigned numVertices = modelComp->_geometry.size(), numFaces = modelComp->_topology.size();
	const int numGroups = ComputeShader::getNumGroups(numTrees);

	const GLuint modelVertexBufferID = ComputeShader::setReadBuffer(model->getModelComponent(0)->_geometry, GL_STATIC_DRAW);
	const GLuint modelFaceBufferID = ComputeShader::setReadBuffer(model->getModelComponent(0)->_topology, GL_STATIC_DRAW);
	const GLuint geometryBufferID = ComputeShader::setWriteBuffer(VertexGPUData(), numVertices * numTrees, GL_DYNAMIC_DRAW);
	const GLuint faceBufferID = ComputeShader::setWriteBuffer(FaceGPUData(), numFaces * numTrees, GL_DYNAMIC_DRAW);

	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::GENERATE_TREE_GEOMETRY_TOPOLOGY);
	shader->use();
	shader->bindBuffers(std::vector<GLuint> { geometryBufferID, faceBufferID, modelVertexBufferID, modelFaceBufferID, positionBufferID, rotationBufferID, scaleBufferID });
	shader->setUniform("numTrees", numTrees);
	shader->setUniform("modelGeomSize", numVertices);
	shader->setUniform("modelFaceSize", numFaces);
	shader->setUniform("globalOffset", (GLuint) refModelComp->_geometry.size());
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	VertexGPUData* vertexData = shader->readData(geometryBufferID, VertexGPUData());
	std::vector<VertexGPUData> geometry = std::move(std::vector<VertexGPUData>(vertexData, vertexData + numVertices * numTrees));
	refModelComp->_geometry.insert(refModelComp->_geometry.end(), geometry.begin(), geometry.end());

	FaceGPUData* faceData = shader->readData(faceBufferID, FaceGPUData());
	std::vector<FaceGPUData> topology = std::move(std::vector<FaceGPUData>(faceData, faceData + numFaces * numTrees));
	refModelComp->_topology.insert(refModelComp->_topology.end(), topology.begin(), topology.end());

	GLuint buffers[] = { modelVertexBufferID, modelFaceBufferID, geometryBufferID, faceBufferID };
	glDeleteBuffers(4, buffers);
}

void Forest::loadForestModels()
{	
	CADModel* canopy, *trunk;
	VAO* vao;
	MaterialList* materialList = MaterialList::getInstance();

	for (int i = 0; i < TerrainParameters::TREE_HAS_LEAVES.size(); ++i)
	{
		trunk = new CADModel(TerrainParameters::TREE_PATH[i] + "Trunk", "", true);
		trunk->load();
		trunk->setMaterial(materialList->getMaterial(TerrainParameters::TRUNK_MATERIALS[i]));
		
		_forestModel[TRUNK].push_back(trunk);
		_numVertices[TRUNK].push_back(trunk->getModelComponent(0)->_geometry.size());

		vao = trunk->getModelComponent(0)->_vao;
		vao->defineOffsetVBO();
		vao->defineScaleVBO();
		vao->defineRotationVBO();

		if (TerrainParameters::TREE_HAS_LEAVES[i])
		{
			canopy = new CADModel(TerrainParameters::TREE_PATH[i] + "Leaves", "", true);
			canopy->load();
			canopy->setMaterial(materialList->getMaterial(TerrainParameters::CANOPY_MATERIALS[i]));
			
			_forestModel[CANOPY].push_back(canopy);
			_numVertices[CANOPY].push_back(canopy->getModelComponent(0)->_geometry.size());

			vao = canopy->getModelComponent(0)->_vao;
			vao->defineOffsetVBO();
			vao->defineScaleVBO();
			vao->defineRotationVBO();
		}
		else
		{
			_forestModel[CANOPY].push_back(nullptr);
			_numVertices[CANOPY].push_back(0);
		}
	}
}