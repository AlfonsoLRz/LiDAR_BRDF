#include "stdafx.h"
#include "TerrainModel.h"

#include "Graphics/Application/MaterialList.h"
#include "Graphics/Core/ShaderList.h"

// Initialization of static attributes
const float TerrainModel::TERRAIN_OFFSET = .05f;
const float TerrainModel::RANDOM_ROTATION = getRandomRotation();


/// Public methods

TerrainModel::TerrainModel(const std::string& filename, const std::string& textureFolder, TerrainConfiguration* terrainConfiguration, const mat4& modelMatrix) :
	CADModel(filename, textureFolder, false),
	_terrainConfiguration(terrainConfiguration),
	_elevation(	.0f), _radius(1.0f)
{
	this->_modelMatrix = modelMatrix;
}

TerrainModel::~TerrainModel()
{
}

bool TerrainModel::load(const mat4& modelMatrix)
{
	if (!_loaded)
	{	
		CADModel::load();

		vec2		selectedPosition;
		const vec4	position = this->getPosition(selectedPosition);
		const mat4	mMatrix = modelMatrix * glm::translate(mat4(1.0f), vec3(position)) * glm::rotate(mat4(1.0f), RANDOM_ROTATION * 2.0f * glm::pi<float>(), vec3(.0f, 1.0f, .0f));

		_terrainConfiguration->_grid->saturatePoint_Square(selectedPosition, _radius, _terrainConfiguration->_terrainParameters._modelSaturationLevel);

		// Modify model component accordingly to our model matrix
		for (ModelComponent* modelComp: _modelComp)
		{
			this->generateGeometryTopology(modelComp, mMatrix);
			
			delete modelComp->_vao;
			this->setVAOData(modelComp);
		}

		_loaded = true;
	}

	return _loaded;
}

/// Protected methods

void TerrainModel::computeMeshData(Model3D::ModelComponent* modelComp)
{
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::MODEL_MESH_GENERATION);
	const int arraySize = modelComp->_topology.size();
	const int numGroups = ComputeShader::getNumGroups(arraySize);

	GLuint modelBufferID, meshBufferID, outBufferID, faceIncrementID;
	modelBufferID	= ComputeShader::setReadBuffer(modelComp->_geometry);
	meshBufferID	= ComputeShader::setReadBuffer(modelComp->_topology);
	outBufferID		= ComputeShader::setWriteBuffer(GLuint(), arraySize * 4);
	faceIncrementID = ComputeShader::setWriteBuffer(GLuint(), 1);

	shader->bindBuffers(std::vector<GLuint> { modelBufferID, meshBufferID, outBufferID, faceIncrementID });
	shader->use();
	shader->setUniform("restartPrimitiveIndex", Model3D::RESTART_PRIMITIVE_INDEX);
	shader->setUniform("size", arraySize);
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	FaceGPUData* faceData = shader->readData(meshBufferID, FaceGPUData());
	GLuint* rawMeshData = shader->readData(outBufferID, GLuint());
	modelComp->_topology = std::move(std::vector<FaceGPUData>(faceData, faceData + arraySize));
	modelComp->_triangleMesh = std::move(std::vector<GLuint>(rawMeshData, rawMeshData + arraySize * 4));

	glDeleteBuffers(1, &modelBufferID);
	glDeleteBuffers(1, &meshBufferID);
	glDeleteBuffers(1, &outBufferID);
}

void TerrainModel::filterFaces(const AABB& terrainAABB)
{
	vec3 terrainMaxPoint = terrainAABB.max(), terrainMinPoint = terrainAABB.min();

	auto insideAABB = [=](const vec3& vertex) -> bool
	{
		return (vertex.x > terrainMinPoint.x) && (vertex.x < terrainMaxPoint.x) && (vertex.z > terrainMinPoint.z) && (vertex.z < terrainMaxPoint.z);
	};
	
	for (ModelComponent* modelComp : _modelComp)
	{	
		auto faceIt = modelComp->_topology.begin();
		while(faceIt != modelComp->_topology.end())
		{
			if (!insideAABB(modelComp->_geometry[faceIt->_vertices.x]._position) || !insideAABB(modelComp->_geometry[faceIt->_vertices.y]._position) || !insideAABB(modelComp->_geometry[faceIt->_vertices.z]._position))
			{
				faceIt = modelComp->_topology.erase(faceIt);
			}
			else
			{
				++faceIt;
			}
		}
	}
}

void TerrainModel::generateGeometryTopology(Model3D::ModelComponent* modelComp, const mat4& modelMatrix)
{
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::MODEL_APPLY_MODEL_MATRIX);
	const int arraySize = modelComp->_geometry.size();
	const int numGroups = ComputeShader::getNumGroups(arraySize);
	GLuint modelBufferID = ComputeShader::setReadBuffer(modelComp->_geometry);

	shader->bindBuffers(std::vector<GLuint> { modelBufferID});
	shader->use();
	shader->setUniform("mModel", modelMatrix * _modelMatrix);
	shader->setUniform("size", arraySize);
	if (modelComp->_material) modelComp->_material->applyMaterial4ComputeShader(shader);
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	VertexGPUData* data = shader->readData(modelBufferID, VertexGPUData());
	modelComp->_geometry = std::move(std::vector<VertexGPUData>(data, data + arraySize));

	//this->filterFaces(_terrain->getDimensions());
	this->computeTangents(modelComp);
	this->computeMeshData(modelComp);

	glDeleteBuffers(1, &modelBufferID);
}

void TerrainModel::generateRandomPositions(std::vector<vec4>& randomPositions)
{
	RandomUtilities::initializeUniformDistribution(.0f + TERRAIN_OFFSET, 1.0f - TERRAIN_OFFSET);

	for (int pos = 0; pos < _terrainConfiguration->_terrainParameters._modelNumParticles; ++pos)
	{
		const vec2 position = vec2(RandomUtilities::getUniformRandomValue(), RandomUtilities::getUniformRandomValue());

		if (!_terrainConfiguration->_grid->isSaturated(position, .0f))
		{
			randomPositions.push_back(vec4(position, .0f, .0f));
		}
	}
}

unsigned TerrainModel::getMaximumGoodness(std::vector<vec4>& goodness)
{
	unsigned selectedIndex = 0;
	float maxGoodness = goodness[0].x;

	for (int i = 1; i < goodness.size(); ++i)
	{
		if (goodness[i].x > maxGoodness)
		{
			selectedIndex = i;
			maxGoodness = goodness[i].x;
		}
	}

	return selectedIndex;
}

vec4 TerrainModel::getPosition(vec2& selectedRandomPos)
{
	Texture* heightMap = _terrainConfiguration->_heightMap, *normalMap = _terrainConfiguration->_normalMap, *vegetationMap = _terrainConfiguration->_vegetationMap;
	const vec2 terrainSize = _terrainConfiguration->_terrainParameters._terrainSize;
	std::vector<vec4> randomPositions;
	unsigned selectedIndex;
	unsigned modelNumParticles = _terrainConfiguration->_terrainParameters._modelNumParticles;

	this->generateRandomPositions(randomPositions);

	// Evaluate the goodness of each seed so that the model is placed in the most flat place 
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::COMPUTE_BUILDING_POSITION);
	const int numGroups = ComputeShader::getNumGroups(modelNumParticles);

	vec4* goodnessBuffer = (vec4*)calloc(modelNumParticles, sizeof(vec4));				// Initialized with 0
	const GLuint randomPosBufferID = ComputeShader::setReadBuffer(randomPositions, GL_STATIC_DRAW);
	const GLuint positionBufferID = ComputeShader::setWriteBuffer(vec4(), modelNumParticles, GL_STREAM_DRAW);
	const GLuint goodnessBufferID = ComputeShader::setReadBuffer(goodnessBuffer, modelNumParticles, GL_DYNAMIC_DRAW);
	free(goodnessBuffer);

	shader->bindBuffers(std::vector<GLuint> { randomPosBufferID, positionBufferID, goodnessBufferID });
	shader->use();
	shader->setUniform("numPositions", modelNumParticles);
	shader->setUniform("modelMatrix", this->getModelMatrix());
	shader->setUniform("width", terrainSize.x);
	shader->setUniform("depth", terrainSize.y);
	shader->setUniform("terrainDisp", _terrainConfiguration->_terrainParameters._terrainMaxHeight);
	shader->setUniform("instanceBoundary", .61f);
	shader->setUniform("radius", _radius);
	heightMap->applyTexture(shader, 0, "heightMap");
	normalMap->applyTexture(shader, 1, "normalMap");
	vegetationMap->applyTexture(shader, 2, "vegetationMap");
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	vec4* positionData = shader->readData(positionBufferID, vec4());
	std::vector<vec4> position = std::move(std::vector<vec4>(positionData, positionData + modelNumParticles));

	vec4* goodnessData = shader->readData(goodnessBufferID, vec4());
	std::vector<vec4> goodness = std::move(std::vector<vec4>(goodnessData, goodnessData + modelNumParticles));

	selectedRandomPos = randomPositions[selectedIndex = this->getMaximumGoodness(goodness)];

	GLuint buffers[] = { randomPosBufferID, positionBufferID, goodnessBufferID };
	glDeleteBuffers(3, buffers);

	return position[selectedIndex] + vec4(.0f, _elevation, .0f, .0f);
}