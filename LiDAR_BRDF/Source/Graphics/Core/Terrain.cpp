 #include "stdafx.h"
#include "Terrain.h"

#include "Geometry/3D/Intersections3D.h"
#include "Graphics/Application/TextureList.h"
#include "Graphics/Core/OpenGLUtilities.h"
#include "Graphics/Core/ShaderList.h"
#include "Interface/Window.h"
#include "Utilities/RandomUtilities.h"

/// [Public methods]

Terrain::Terrain(Group3D* sceneGroup, TerrainConfiguration* terrainConfiguration, const mat4& modelMatrix):
	_terrainParameters(&terrainConfiguration->_terrainParameters),
	_terrainConfiguration(terrainConfiguration),
	PlanarSurface(
		terrainConfiguration->_terrainParameters._terrainSize.x, terrainConfiguration->_terrainParameters._terrainSize.y, 
		terrainConfiguration->_terrainParameters._terrainSubdivisions.x, terrainConfiguration->_terrainParameters._terrainSubdivisions.y, 
		1.0f, 1.0f, modelMatrix),
	_material(nullptr), _minMax(.0f), _regularGrid(terrainConfiguration->_grid), _sceneGroup(sceneGroup), _heightMapFloat(nullptr)
{
	TextureList* textureList = TextureList::getInstance();
	this->_clayTexture = textureList->getTexture(CGAppEnum::TEXTURE_CLAY_ALBEDO);
	this->_snowTexture = textureList->getTexture(CGAppEnum::TEXTURE_SNOW_ALBEDO);
	this->_stoneTexture = textureList->getTexture(CGAppEnum::TEXTURE_STONE_ALBEDO);
}

Terrain::~Terrain()
{
	delete[] _heightMapFloat;
	delete _material;
}

void Terrain::detectLakes(std::vector<Lake>& lakes)
{
	std::vector<vec3> localMinima;

	// Retrieve local minima points
	for (int x = 0; x < _terrainParameters->_mapSize; ++x)
	{
		for (int y = 0; y < _terrainParameters->_mapSize; ++y)
		{
			if (this->isLocalMinima(x, y)) localMinima.push_back(vec3(x, _heightMapFloat[y * _terrainParameters->_mapSize + x], y));
		}
	}

	bool* analyzed = new bool[_terrainParameters->_mapSize * _terrainParameters->_mapSize];
	RandomUtilities::initializeUniformDistribution(-1.0f, 1.0f);

	for (int pointIdx = 0; pointIdx < localMinima.size(); ++pointIdx)
	{
		Lake lake;
		std::queue<vec3> pointQueue;
		pointQueue.push(localMinima[pointIdx]);
		lake.pushPoint(localMinima[pointIdx]);
		std::fill(analyzed, analyzed + _terrainParameters->_mapSize * _terrainParameters->_mapSize, false);

		lake._maxObservedHeight = _terrainParameters->_waterHeight + RandomUtilities::getUniformRandomValue() * _terrainParameters->_waterHeightVariance;

		while (!pointQueue.empty())
		{
			vec3 expandNeighbor = pointQueue.front();
			pointQueue.pop();

			this->expandPoint(expandNeighbor, pointQueue, lake, analyzed);
		}

		lakes.push_back(std::move(lake));
	}

	delete[] analyzed;

	for (int lakeIdx = 0; lakeIdx < lakes.size(); ++lakeIdx)
	{
		lakes[lakeIdx].computeAABB();
	}
	std::sort(lakes.begin(), lakes.end(), [](const Lake& left, const Lake& right) { return left._aabb.area() > right._aabb.area();  });
}

void Terrain::drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	// Apply terrain texturing parameters
	_clayTexture->applyTexture(shader, TerrainParameters::TEXTURE_START_INDEX, "texGroundSampler");
	_snowTexture->applyTexture(shader, TerrainParameters::TEXTURE_START_INDEX + 1, "texSnowSampler");
	_stoneTexture->applyTexture(shader, TerrainParameters::TEXTURE_START_INDEX + 2, "texRockSampler");

	shader->setUniform("groundTextureScale", _terrainParameters->_grassTextureScale);
	shader->setUniform("heightBoundaries", _minMax);
	shader->setUniform("snowTextureScale", _terrainParameters->_snowTextureScale);
	shader->setUniform("rockTextureScale", _terrainParameters->_rockTextureScale);
	shader->setUniform("rockWeightFactor", _terrainParameters->_rockWeightFactor);

	this->renderTriangles(shader, shaderType, matrix, _modelComp[0], GL_TRIANGLE_STRIP);
}

bool Terrain::load(const mat4& modelMatrix)
{
	if (!_loaded && !_terrainConfiguration->_terrainParameters._heightMapFile.empty())
	{
		this->initializeHeightMap();
		this->createNormalMap();
		this->createMaterial();
		this->generateGeometryTopology(modelMatrix);
		this->setVAOData();
		this->initializeLakes();

		_modelComp[0]->setMaterial(MaterialDatabase::STONE);
		_loaded = true;
	}

	return _loaded;
}

void Terrain::retrieveColorsGPU()
{
	Model3D::retrieveColorsGPU();

	_modelComp[0]->_material->setTexture(Texture::KAD_TEXTURE, nullptr);
}

/// [Protected methods]

void Terrain::buildBasementMesh(const unsigned startIndex)
{
	ModelComponent* modelComponent = _modelComp[0];
	const unsigned geometrySize = modelComponent->_geometry.size();
	const unsigned indices[] = { 0, _tilingH, startIndex - 1 - _tilingH, startIndex - 1 };

	for (int pointIdx = 0; pointIdx < 4; ++pointIdx)
	{
		VertexGPUData vertex = modelComponent->_geometry[indices[pointIdx]];
		vertex._position.y = std::min(_terrainParameters->_terrainExtrusion, vertex._position.y);
		vertex._normal = vec3(.0f, -1.0f, .0f);
		vertex._tangent = vec3(1.0f, .0f, .0f);
		vertex._textCoord = vec2(-FLT_MAX);

		modelComponent->_geometry.push_back(vertex);
	}

	modelComponent->_triangleMesh.insert(modelComponent->_triangleMesh.end(), { geometrySize, geometrySize + 1, geometrySize + 2, geometrySize + 3, Model3D::RESTART_PRIMITIVE_INDEX });
}

void Terrain::buildLateralMeshes(const unsigned size, const unsigned startIndex, const unsigned jump, const vec3& normal, const unsigned avoidAdvance)
{
	ModelComponent* modelComponent = this->_modelComp[0];
	const unsigned initialVertexNumber = modelComponent->_geometry.size();

	for (unsigned i = 0; i <= size; ++i)
	{
		VertexGPUData vertex = modelComponent->_geometry[i * avoidAdvance + startIndex + i * jump];
		vertex._position.y = std::min(_terrainParameters->_terrainExtrusion, vertex._position.y);
		vertex._normal = normal;
		vertex._tangent = vec3(.0f, -1.0f, .0f);
		vertex._textCoord = vec2(-FLT_MAX);
		modelComponent->_geometry[i * avoidAdvance + startIndex + i * jump]._textCoord = vec2(-FLT_MAX);

		modelComponent->_geometry.push_back(vertex);
	}

	for (int i = 0; i <= size; ++i)
	{
		modelComponent->_triangleMesh.insert(modelComponent->_triangleMesh.end(), { i * avoidAdvance + startIndex + i * jump, initialVertexNumber + i });
	}

	modelComponent->_triangleMesh.insert(modelComponent->_triangleMesh.end(), { Model3D::RESTART_PRIMITIVE_INDEX });
}

void Terrain::createMaterial()
{
	TextureList* textureList = TextureList::getInstance();

	_material = new Material();
	_material->setTexture(Texture::KAD_TEXTURE, textureList->getTexture(CGAppEnum::TEXTURE_STONE_ALBEDO));
	_material->setTexture(Texture::KS_TEXTURE, textureList->getTexture(CGAppEnum::TEXTURE_BLACK));
	_material->setTexture(Texture::DISPLACEMENT_MAPPING_TEXTURE, _terrainConfiguration->_heightMap);
	_material->setTexture(Texture::BUMP_MAPPING_TEXTURE, _terrainConfiguration->_normalMap);
	_material->setDisplacementFactor(_terrainParameters->_terrainMaxHeight);
	_material->setShininess(500.0f);

	this->setMaterial(_material);
}

void Terrain::createNormalMap()
{
	const ivec2 canvasSize = Window::getInstance()->getSize();
	VAO* quadVAO = Primitives::getQuadVAO();
	FBOScreenshot* normalMapFBO = new FBOScreenshot(_terrainParameters->_mapSize, _terrainParameters->_mapSize);

	{
		glBindFramebuffer(GL_FRAMEBUFFER, normalMapFBO->getIdentifier());
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, _terrainParameters->_mapSize, _terrainParameters->_mapSize);

		RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::NORMAL_MAP_SHADER);
		shader->use();
		shader->setUniform("extrusion", _terrainParameters->_normalMapDepth);
		shader->setUniform("width", _terrainParameters->_mapSize);
		shader->setUniform("height", _terrainParameters->_mapSize);
		_terrainConfiguration->_heightMap->applyTexture(shader, 0, "texHeightSampler");
		shader->applyActiveSubroutines();

		quadVAO->drawObject(RendEnum::IBO_TRIANGLE_MESH, GL_TRIANGLES, 2 * 4);
	}

	{
		//normalMapFBO->saveImage("Normal.png");
		Image* image = normalMapFBO->getImage();
		_terrainConfiguration->_normalMap
			= new Texture(image, _terrainParameters->_mapSize, _terrainParameters->_mapSize, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		// Go back to initial state
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, canvasSize.x, canvasSize.y);

		delete image;
		delete normalMapFBO;
	}
}

void Terrain::expandPoint(const vec3& currentPoint, std::queue<vec3>& pointQueue, Lake& lake, bool* analyzed)
{
	int x = int(currentPoint.x), y = int(currentPoint.z);
	int min_x = glm::clamp(x - 1, 0, int(_terrainParameters->_mapSize)), min_y = glm::clamp(y - 1, 0, int(_terrainParameters->_mapSize));
	int max_x = glm::clamp(x + 1, 0, int(_terrainParameters->_mapSize - 1)), max_y = glm::clamp(y + 1, 0, int(_terrainParameters->_mapSize - 1));

	for (int x_idx = min_x; x_idx <= max_x; ++x_idx)
	{
		for (int y_idx = min_y; y_idx <= max_y; ++y_idx)
		{
			if (!analyzed[y_idx * _terrainParameters->_mapSize + x_idx])
			{
				if (_heightMapFloat[y_idx * _terrainParameters->_mapSize + x_idx] <= lake._maxObservedHeight)
				{
					analyzed[y_idx * _terrainParameters->_mapSize + x_idx] = true;
					pointQueue.push(vec3(x_idx, _heightMapFloat[y_idx * _terrainParameters->_mapSize + x_idx], y_idx));
					lake.pushPoint(vec3(x_idx, _heightMapFloat[y_idx * _terrainParameters->_mapSize + x_idx], y_idx));
				}
			}
		}
	}
}

void Terrain::extrudeTerrain()
{
	const unsigned vertexIndex = _modelComp[0]->_geometry.size();

	this->buildLateralMeshes(_tilingH, 0, 0, vec3(.0f, 1.0f, .0f));
	this->buildLateralMeshes(_tilingH, (_tilingV + 1) * _tilingH, 0, vec3(.0f, 1.0f, 0.0f));
	this->buildLateralMeshes(_tilingV, 0, _tilingH + 1, vec3(.0f, 1.0f, .0f), 0);
	this->buildLateralMeshes(_tilingV, _tilingH, _tilingH + 1, vec3(.0f, 1.0f, .0f), 0);
	this->buildBasementMesh(vertexIndex);
}

void Terrain::generateGeometryTopology(const mat4& modelMatrix)
{
	// Geometry constraints
	const float width_2 = _width / 2.0f, depth_2 = _depth / 2.0f;
	const float tileWidth = _width / _tilingH, tileDepth = _depth / _tilingV;

	// Topology constraints
	const unsigned numVertices = (_tilingH + 1) * (_tilingV + 1);
	const unsigned numTriangles = _tilingH * _tilingV * 2;
	const unsigned numIndices = _tilingH * (_tilingV + 1) * 2 + _tilingH;

	unsigned numGroups = 128;
	const GLuint workGroupSize_x = ComputeShader::getWorkGroupSize(numGroups, _tilingH + 1);
	const GLuint workGroupSize_y = ComputeShader::getWorkGroupSize(numGroups, _tilingV + 1);

	// FIRST PHASE: create mesh geometry and topology
	ModelComponent* modelComp = _modelComp[0];
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::TERRAIN_GEOMETRY_TOPOLOGY);

	GLuint modelBufferID, meshBufferID, rawMeshBufferID;
	modelBufferID = ComputeShader::setWriteBuffer(Model3D::VertexGPUData(), numVertices, GL_DYNAMIC_DRAW);
	meshBufferID = ComputeShader::setWriteBuffer(Model3D::FaceGPUData(), numTriangles, GL_DYNAMIC_DRAW);
	rawMeshBufferID = ComputeShader::setWriteBuffer(GLuint(), numIndices);

	shader->bindBuffers(std::vector<GLuint> { modelBufferID, rawMeshBufferID });
	shader->use();
	shader->setUniform("mModel", modelMatrix * _modelMatrix);
	shader->setUniform("gridSize", vec2(_tilingH, _tilingV));
	shader->setUniform("gridDim_2", vec2(width_2, depth_2));
	shader->setUniform("tileDim", vec2(tileWidth, tileDepth));
	shader->setUniform("maxTextCoord", vec2(_maxTextValH, _maxTextValV));
	shader->setUniform("restartPrimitiveIndex", Model3D::RESTART_PRIMITIVE_INDEX);
	if (modelComp->_material) modelComp->_material->applyMaterial4ComputeShader(shader, true);
	shader->execute(numGroups, numGroups, 1, workGroupSize_x, workGroupSize_y, 1);

	// SECOND PHASE: compute topology once geometry is fully computed
	numGroups = ComputeShader::getNumGroups(numTriangles / 2);
	shader = ShaderList::getInstance()->getComputeShader(RendEnum::TERRAIN_FACES_TOPOLOGY);

	shader->bindBuffers(std::vector<GLuint> { modelBufferID, meshBufferID });
	shader->use();
	shader->setUniform("gridSize", uvec2(_tilingH, _tilingV));
	shader->setUniform("numFaces", numTriangles);
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	VertexGPUData* modelData = shader->readData(modelBufferID, VertexGPUData());
	modelComp->_geometry.resize(numVertices);
	std::copy(modelData, modelData + numVertices, modelComp->_geometry.begin());

	FaceGPUData* faceData = shader->readData(meshBufferID, FaceGPUData());					// Fully computed after second phase
	modelComp->_topology = std::move(std::vector<FaceGPUData>(faceData, faceData + numTriangles));

	GLuint* meshData = shader->readData(rawMeshBufferID, GLuint());
	modelComp->_triangleMesh = std::move(std::vector<GLuint>(meshData, meshData + numIndices));

	this->_minMax = vec2(.0f, _terrainParameters->_terrainMaxHeight);

	glDeleteBuffers(1, &modelBufferID);
	glDeleteBuffers(1, &meshBufferID);
	glDeleteBuffers(1, &rawMeshBufferID);

	this->extrudeTerrain();
}

AABB Terrain::getDimensions()
{
	vec2 halfSize = _terrainParameters->_terrainSize / 2.0f;
	return AABB(vec3(-halfSize.x, .0f, -halfSize.y), vec3(halfSize.x, .0f, halfSize.y));
}

void Terrain::initializeHeightMap()
{
	Image* image = new Image(_terrainParameters->_heightMapFile);
	auto bits = image->bits();
	int height = image->getHeight(), width = image->getWidth(), depth = image->getDepth();
	_terrainParameters->_mapSize = std::min(height, width);

	_heightMapFloat = new float[_terrainParameters->_mapSize * _terrainParameters->_mapSize];
	for (int row = 0; row < _terrainParameters->_mapSize; ++row)
	{
		for (int col = 0; col < _terrainParameters->_mapSize; ++col)
		{
			unsigned index = row * _terrainParameters->_mapSize * depth + col * depth;
			_heightMapFloat[row * _terrainParameters->_mapSize + col] = bits[index] / 255.0f;
		}
	}

	_terrainConfiguration->_heightMap = new Texture(_heightMapFloat, _terrainParameters->_mapSize, _terrainParameters->_mapSize, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	delete image;
}

void Terrain::initializeLakes()
{
	if (!_terrainParameters->_numLakes) return;

	std::vector<Lake> lakes;
	this->detectLakes(lakes);

	float scaleFactor = _terrainParameters->_terrainSize.x / float(_terrainParameters->_mapSize);
	auto transformFunc = [=](AABB& aabb) -> AABB
	{
		vec3 aabbSize = aabb.size();
		vec3 newAABBSize = aabbSize * scaleFactor;
		vec3 aabbCenter = vec3(aabb.center().x, .0f, _terrainParameters->_mapSize - aabb.center().z);
		vec3 center = aabbCenter * scaleFactor - vec3(_terrainParameters->_terrainSize.x / 2.0f, .0f, _terrainParameters->_terrainSize.y / 2.0f);
		center.y = aabb.max().y * _terrainParameters->_terrainMaxHeight;

		return AABB(center - newAABBSize / 2.0f, center + newAABBSize / 2.0f);
	};

	auto transformPointFunc = [=](const vec3& point) -> vec3
	{
		vec3 newPoint = point * scaleFactor;
		newPoint /= _terrainParameters->_terrainSize.x;

		return newPoint;
	};

	unsigned numInstantiatedLakes = 0;
	bool collidesPreviousAABB;
	std::vector<AABB> instantiatedAABBs;

	for (int idx = 0; numInstantiatedLakes < _terrainParameters->_numLakes && idx < lakes.size(); ++idx)
	{
		collidesPreviousAABB = false;
		AABB aabb (lakes[idx]._aabb.min() - vec3(1.0f, .0f, 1.0f), lakes[idx]._aabb.max() + vec3(1.0f, .0f, 1.0f));
		AABB newAABB = transformFunc(aabb);
		vec3 newOrigin = transformPointFunc(lakes[idx]._points[0]);

		if (_regularGrid->isSaturated(vec2(newOrigin.x, newOrigin.z), _terrainParameters->_modelSaturationLevel / 2) || newAABB.area() < 1.0f) continue;
		for (AABB& aabb : instantiatedAABBs) 
		{
			if (Intersections3D::intersect(aabb, newAABB)) collidesPreviousAABB = true;
		}
		if (collidesPreviousAABB) continue;

		uvec2 waterTiling (_terrainParameters->_waterSubdivisions * newAABB.size().x, _terrainParameters->_waterSubdivisions * newAABB.size().z);
		Water* water = new Water(newAABB.size().x, newAABB.size().z, waterTiling.x, waterTiling.y, glm::translate(mat4(1.0f), newAABB.center()));
		water->setSemanticGroup("Water");
		water->setSemanticGroup(LiDARParameters::WATER);
		water->setName("Water");
		water->load();
		_sceneGroup->addComponent(water);
		_lakes.push_back(water);
		++numInstantiatedLakes;
		instantiatedAABBs.push_back(newAABB);

		for (vec3& point : lakes[idx]._points)
		{
			vec3 newPoint = transformPointFunc(point);
			_regularGrid->saturatePoint_Square(vec2(newPoint.x, newPoint.z), 1.0f, _terrainParameters->_modelSaturationLevel);
		}
	}
}

bool Terrain::isLocalMinima(int x, int y)
{
	unsigned numHigherPoints = 0;
	int min_x = glm::clamp(x - 1, 0, int(_terrainParameters->_mapSize)), min_y = glm::clamp(y - 1, 0, int(_terrainParameters->_mapSize));
	int max_x = glm::clamp(x + 1, 0, int(_terrainParameters->_mapSize - 1)), max_y = glm::clamp(y + 1, 0, int(_terrainParameters->_mapSize - 1));

	for (int x_idx = min_x; x_idx <= max_x; ++x_idx)
	{
		for (int y_idx = min_y; y_idx <= max_y; ++y_idx)
		{
			numHigherPoints += (_heightMapFloat[y * _terrainParameters->_mapSize + x] - _heightMapFloat[y_idx * _terrainParameters->_mapSize + x_idx]) <= .0001f;
		}
	}

	return numHigherPoints > 8;
}
