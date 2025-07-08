#include "stdafx.h"
#include "Grass.h"

#include "Graphics/Application/MaterialList.h"
#include "Graphics/Application/TextureList.h"
#include "Graphics/Core/OpenGLUtilities.h"
#include "Graphics/Core/ShaderList.h"
#include "Graphics/Core/Terrain.h"
#include "Utilities/ChronoUtilities.h"
#include "Utilities/RandomUtilities.h"

/// [Public methods]

Grass::Grass(TerrainConfiguration* terrainConfiguration, const mat4& modelMatrix) :
	Model3D(modelMatrix, 1), _terrainConfiguration(terrainConfiguration)
{
}

Grass::~Grass()
{
}

void Grass::drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	this->renderTriangles(shader, shaderType, matrix, _modelComp[0], GL_TRIANGLE_STRIP);
}

void Grass::drawAsTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	this->renderTriangles4Shadows(shader, shaderType, matrix, _modelComp[0], GL_TRIANGLE_STRIP);
}

bool Grass::load(const mat4& modelMatrix)
{
	if (!_loaded)
	{
		this->createVegetationMap();
		this->generateVegetationModel();
		//this->generatePointCloud();
		//this->generateWireframe();
		this->setVAOData();

		this->_modelComp[0]->_material = MaterialList::getInstance()->getMaterial(CGAppEnum::MATERIAL_WEED_PLANT);
		this->_modelComp[0]->setMaterial(MaterialDatabase::LEAF);

		return _loaded = true;
	}

	return false;
}

/// [Protected methods]

void Grass::createVegetationMap()
{
	TerrainParameters* terrainParameters = &_terrainConfiguration->_terrainParameters;
	const unsigned textureSize = terrainParameters->_grassTextureSize;
	const unsigned numPixels = textureSize * textureSize;
	const unsigned numGroups = 32;
	const GLuint workGroupSize_x = ComputeShader::getWorkGroupSize(numGroups, textureSize);
	const GLuint workGroupSize_y = ComputeShader::getWorkGroupSize(numGroups, textureSize);

	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::GENERATE_VEGETATION_MAP);
	Texture* terrainHeightMap = _terrainConfiguration->_heightMap;
	Texture* terrainNormalMap = _terrainConfiguration->_normalMap;

	// Define buffers
	GLuint vegetationMapID = ComputeShader::setWriteBuffer(vec4(), textureSize * textureSize, GL_STREAM_DRAW);

	shader->bindBuffers(std::vector<GLuint> { vegetationMapID });
	shader->use();
	shader->setUniform("globalSize", textureSize);
	shader->setUniform("maxHeight", terrainParameters->_grassHeightThreshold.y);
	shader->setUniform("minHeight", terrainParameters->_grassHeightThreshold.x);
	shader->setUniform("heightWeight", terrainParameters->_grassHeightWeight);
	shader->setUniform("slopeWeight", terrainParameters->_grassSlopeWeight);
	terrainHeightMap->applyTexture(shader, 0, "heightMap");
	terrainNormalMap->applyTexture(shader, 1, "normalMap");
	shader->execute(numGroups, numGroups, 1, workGroupSize_x, workGroupSize_y, 1);

	vec4* pixels = shader->readData(vegetationMapID, vec4());
	float* imageData = new float[textureSize * textureSize * 4];

	for (int i = 0; i < textureSize * textureSize; ++i)
	{
		imageData[i * 4] = pixels[i].x;
		imageData[i * 4 + 1] = pixels[i].y;
		imageData[i * 4 + 2] = pixels[i].z;
		imageData[i * 4 + 3] = 1.0f;
	}

	_terrainConfiguration->_vegetationMap = new Texture(imageData, textureSize, textureSize, GL_REPEAT, GL_REPEAT, GL_MIPMAP, GL_MIPMAP, true);
}

void Grass::generateRandomPositions(std::vector<vec4>& randomPositions)
{
	RandomUtilities::initializeUniformDistribution(.0f, 1.0f);

	for (int tree = 0; tree < _terrainConfiguration->_terrainParameters._numGrassSeeds; ++tree)
	{
		vec2 position = vec2(RandomUtilities::getUniformRandomValue(), RandomUtilities::getUniformRandomValue());
		randomPositions.push_back(vec4(position, .0f, .0f));
	}
}

void Grass::generateVegetationModel()
{
	// Random positions
	TerrainParameters* terrainParameters = &_terrainConfiguration->_terrainParameters;
	unsigned numRandomPositions = terrainParameters->_numGrassSeeds, numPlants = 0;
	std::vector<vec4> randomPositions;

	this->generateRandomPositions(randomPositions);

	// Execute compute shader to build geometry and topology of vegetation 
	const int numGroups = ComputeShader::getNumGroups(numRandomPositions);
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::GENERATE_VEGETATION);

	GLuint randomPosBufferID, vertexBufferID, rawMeshBufferID, faceBufferID, numPlantsBufferID;
	randomPosBufferID = ComputeShader::setReadBuffer(randomPositions, GL_STATIC_DRAW);
	vertexBufferID = ComputeShader::setWriteBuffer(Model3D::VertexGPUData(), numRandomPositions * 7 * 3, GL_DYNAMIC_DRAW);
	rawMeshBufferID = ComputeShader::setWriteBuffer(unsigned(), numRandomPositions * 7 * 3 + numRandomPositions, GL_DYNAMIC_DRAW);
	faceBufferID = ComputeShader::setWriteBuffer(Model3D::FaceGPUData(), numRandomPositions * 7, GL_DYNAMIC_DRAW);
	numPlantsBufferID = ComputeShader::setReadBuffer(&numPlants, 1, GL_DYNAMIC_DRAW);

	shader->bindBuffers(std::vector<GLuint> { randomPosBufferID, vertexBufferID, rawMeshBufferID, faceBufferID, numPlantsBufferID });
	shader->use();
	shader->setUniform("numTriangles", numRandomPositions);
	shader->setUniform("minScale", terrainParameters->_grassScale.x);
	shader->setUniform("maxScale", terrainParameters->_grassScale.y);
	shader->setUniform("vegetationThreshold", terrainParameters->_grassThreshold);
	shader->setUniform("restartPrimitiveIndex", Model3D::RESTART_PRIMITIVE_INDEX);
	shader->setUniform("modelMatrix", _modelMatrix);
	shader->setUniform("width", _terrainConfiguration->_terrainParameters._terrainSize.x);
	shader->setUniform("depth", _terrainConfiguration->_terrainParameters._terrainSize.y);
	shader->setUniform("terrainDisp", terrainParameters->_terrainMaxHeight);
	_terrainConfiguration->_vegetationMap->applyTexture(shader, 0, "vegetationMap");
	_terrainConfiguration->_heightMap->applyTexture(shader, 1, "heightMap");
	_terrainConfiguration->_normalMap->applyTexture(shader, 2, "normalMap");
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	// Retrieve geometry & topology
	ModelComponent* modelComp = _modelComp[0];
	numPlants = *shader->readData(numPlantsBufferID, unsigned());

	VertexGPUData* modelData = shader->readData(vertexBufferID, VertexGPUData());
	modelComp->_geometry = std::move(std::vector<VertexGPUData>(modelData, modelData + numPlants * 7));

	FaceGPUData* faceData = shader->readData(faceBufferID, FaceGPUData());
	modelComp->_topology = std::move(std::vector<FaceGPUData>(faceData, faceData + numPlants * 5));

	GLuint* meshData = shader->readData(rawMeshBufferID, GLuint());
	modelComp->_triangleMesh = std::move(std::vector<GLuint>(meshData, meshData + numPlants * 8));

	// Free resources
	std::vector<GLuint> bufferIndices{ randomPosBufferID, vertexBufferID, rawMeshBufferID, faceBufferID, numPlantsBufferID };
	glDeleteBuffers(1, bufferIndices.data());
}

void Grass::generateWireframe()
{
	for (ModelComponent* modelComp : _modelComp)
	{
		const unsigned triangleMeshSize = modelComp->_triangleMesh.size();

		for (int i = 0; i < triangleMeshSize; i += 8)
		{
			for (int j = i; j < (i + 5) && j < triangleMeshSize; ++j)
			{
				modelComp->_wireframe.push_back(modelComp->_triangleMesh[j]);
				modelComp->_wireframe.push_back(modelComp->_triangleMesh[j + 1]);
				modelComp->_wireframe.push_back(Model3D::RESTART_PRIMITIVE_INDEX);
				
				modelComp->_wireframe.push_back(modelComp->_triangleMesh[j + 1]);
				modelComp->_wireframe.push_back(modelComp->_triangleMesh[j + 2]);
				modelComp->_wireframe.push_back(Model3D::RESTART_PRIMITIVE_INDEX);
				
				modelComp->_wireframe.push_back(modelComp->_triangleMesh[j]);
				modelComp->_wireframe.push_back(modelComp->_triangleMesh[j + 2]);
				modelComp->_wireframe.push_back(Model3D::RESTART_PRIMITIVE_INDEX);
			}
		}
	}
}
