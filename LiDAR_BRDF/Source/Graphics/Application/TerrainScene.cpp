#include "stdafx.h"
#include "TerrainScene.h"

#include "Graphics/Application/TextureList.h"
#include "Interface/Window.h"

/// [Public methods]

TerrainScene::~TerrainScene()
{
	delete _forest;
	delete _terrain;

	for (Water* water : _water) delete water;
	for (TerrainModel* fireTower : _fireTower) delete fireTower;
	for (TerrainModel* transmissionTower : _transmissionTower) delete transmissionTower;
}

void TerrainScene::loadTerrainConfiguration(const std::string& filename)
{
	_terrainConfiguration._terrainParameters.loadConfiguration(filename);
}

void TerrainScene::render(const mat4& mModel, RenderingParameters* rendParams)
{
	LiDARScene::render(mModel, rendParams);
}

/// [Protected methods]

void TerrainScene::drawAsTriangles(Camera* camera, const mat4& mModel, RenderingParameters* rendParams)
{
	for (Water* water: _water) water->updateAnimation();

	this->generateWaterTextures(mModel, rendParams);

	LiDARParameters* lidarParams = LiDARSimulation::getLiDARParams();
	RenderingShader* terrainShader = ShaderList::getInstance()->getRenderingShader(RendEnum::TERRAIN_SHADER),
				   * normalShader = ShaderList::getInstance()->getRenderingShader(RendEnum::TRIANGLE_MESH_SHADER),
				   * waterShader = ShaderList::getInstance()->getRenderingShader(RendEnum::WATER_LAKE_SHADER),
				   * grassShader = ShaderList::getInstance()->getRenderingShader(RendEnum::GRASS_SHADER),
				   * treeShader = ShaderList::getInstance()->getRenderingShader(RendEnum::TREE_TRIANGLE_MESH_SHADER);
	std::vector<mat4> matrix(RendEnum::numMatricesTypes());
	const mat4 bias = glm::translate(mat4(1.0f), vec3(0.5f)) * glm::scale(mat4(1.0f), vec3(0.5f));

	matrix[RendEnum::MODEL_MATRIX] = mModel;
	matrix[RendEnum::VIEW_MATRIX] = camera->getViewMatrix();
	matrix[RendEnum::VIEW_PROJ_MATRIX] = camera->getViewProjMatrix();

	glDepthFunc(GL_LEQUAL);

	for (unsigned int i = 0; i < _lights.size(); ++i)											// Multipass rendering
	{
		if (i == 0)
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		}

		if (_lights[i]->shouldCastShadows())
		{
			matrix[RendEnum::BIAS_VIEW_PROJ_MATRIX] = bias * _lights[i]->getCamera()->getViewProjMatrix();
		}

		// TERRAIN
		terrainShader->use();
		terrainShader->setUniform("materialScattering", rendParams->_materialScattering);				// Ambient lighting

		_lights[i]->applyLight(terrainShader, matrix[RendEnum::VIEW_MATRIX]);
		_lights[i]->applyShadowMapTexture(terrainShader, false);
		terrainShader->applyActiveSubroutines();
		terrainShader->setUniform("snowHeightFactor", rendParams->_snowHeightFactor);
		terrainShader->setUniform("snowHeightThreshold", rendParams->_snowHeightThreshold);
		terrainShader->setUniform("snowSlopeFactor", rendParams->_snowSlopeFactor);

		_terrain->drawAsTriangles(terrainShader, RendEnum::TERRAIN_SHADER, matrix);

		// FOREST
		treeShader->use();
		treeShader->setUniform("materialScattering", rendParams->_materialScattering);		

		_lights[i]->applyLight(treeShader, matrix[RendEnum::VIEW_MATRIX]);
		_lights[i]->applyShadowMapTexture(treeShader, false);
		treeShader->applyActiveSubroutines();

		_forest->drawAsTriangles(treeShader, RendEnum::TREE_TRIANGLE_MESH_SHADER, matrix);

		// GRASS
		grassShader->use();
		grassShader->setUniform("materialScattering", rendParams->_materialScattering);

		_lights[i]->applyLight(grassShader, matrix[RendEnum::VIEW_MATRIX]);
		_lights[i]->applyShadowMapTexture(grassShader, false);
		grassShader->applyActiveSubroutines();

		_grass->drawAsTriangles(grassShader, RendEnum::GRASS_SHADER, matrix);

		if (i == 0)
		{
			// LAKES
			waterShader->use();
			waterShader->setUniform("materialScattering", rendParams->_materialScattering);			

			_lights[i]->applyLight(waterShader, matrix[RendEnum::VIEW_MATRIX]);
			_lights[i]->applyShadowMapTexture(waterShader, false);
			waterShader->applyActiveSubroutines();

			for (Water* water : _water) water->drawAsTriangles(waterShader, RendEnum::WATER_LAKE_SHADER, matrix);
		}

		normalShader->use();
		normalShader->setUniform("materialScattering", rendParams->_materialScattering);	

		_lights[i]->applyLight(normalShader, matrix[RendEnum::VIEW_MATRIX]);
		_lights[i]->applyShadowMapTexture(normalShader);
		normalShader->applyActiveSubroutines();

		// BUILDINGS
		for (TerrainModel* fireTower : _fireTower)					fireTower->drawAsTriangles(normalShader, RendEnum::TRIANGLE_MESH_SHADER, matrix);
		for (TerrainModel* transmissionTower : _transmissionTower)	transmissionTower->drawAsTriangles(normalShader, RendEnum::TRIANGLE_MESH_SHADER, matrix);

		//// LiDAR model
		this->renderLiDARModel(normalShader, RendEnum::TRIANGLE_MESH_SHADER, &matrix, rendParams, false);
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);											// Back to initial state
	glDepthFunc(GL_LESS);
}

void TerrainScene::drawAsTrianglesWithGroups(const mat4& mModel, RenderingParameters* rendParams)
{
	Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
	RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::TRIANGLE_MESH_GROUP_SHADER),
				   * multiInstanceShader = ShaderList::getInstance()->getRenderingShader(RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_GROUP_SHADER);
	std::vector<mat4> matrix(RendEnum::numMatricesTypes());
	const mat4 bias = glm::translate(mat4(1.0f), vec3(0.5f)) * glm::scale(mat4(1.0f), vec3(0.5f));						// Proj: [-1, 1] => with bias: [0, 1]

	{
		matrix[RendEnum::MODEL_MATRIX] = mModel;
		matrix[RendEnum::VIEW_MATRIX] = activeCamera->getViewMatrix();
		matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

		glDepthFunc(GL_LEQUAL);
	}

	{
		for (unsigned int i = 0; i < _lights.size(); ++i)																// Multipass rendering
		{
			if (i == 0)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			else
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			}

			if (_lights[i]->shouldCastShadows())
			{
				matrix[RendEnum::BIAS_VIEW_PROJ_MATRIX] = bias * _lights[i]->getCamera()->getViewProjMatrix();
			}

			shader->use();
			shader->setUniform("materialScattering", rendParams->_materialScattering);	
			_lights[i]->applyLight(shader, matrix[RendEnum::VIEW_MATRIX]);
			_lights[i]->applyShadowMapTexture(shader);
			shader->applyActiveSubroutines();

			_sceneGroup->drawAsTrianglesWithGroup(shader, RendEnum::TRIANGLE_MESH_GROUP_SHADER, matrix, rendParams->_semanticRenderingConcept == RenderingParameters::ASPRS_CONCEPT);

			multiInstanceShader->use();
			multiInstanceShader->setUniform("materialScattering", rendParams->_materialScattering);
			_lights[i]->applyLight(multiInstanceShader, matrix[RendEnum::VIEW_MATRIX]);
			_lights[i]->applyShadowMapTexture(multiInstanceShader);
			multiInstanceShader->applyActiveSubroutines();

			_forest->drawAsTrianglesWithGroup(multiInstanceShader, RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_GROUP_SHADER, matrix, rendParams->_semanticRenderingConcept == RenderingParameters::ASPRS_CONCEPT);
		}
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);					// Back to initial state
	glDepthFunc(GL_LESS);
}

void TerrainScene::drawAsTriangles4Position(const mat4& mModel, RenderingParameters* rendParams)
{
	Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
	RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::TRIANGLE_MESH_POSITION_SHADER),
				   * multiInstanceShader = ShaderList::getInstance()->getRenderingShader(RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_POSITION_SHADER);
	std::vector<mat4> matrix(RendEnum::numMatricesTypes());

	{
		matrix[RendEnum::MODEL_MATRIX] = mModel;
		matrix[RendEnum::VIEW_MATRIX] = activeCamera->getViewMatrix();
		matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

		shader->use();

		for (Water* water : _water) water->drawAsTriangles4Shadows(shader, RendEnum::TRIANGLE_MESH_POSITION_SHADER, matrix);
		_terrain->drawAsTriangles4Shadows(shader, RendEnum::TRIANGLE_MESH_POSITION_SHADER, matrix);
		_grass->drawAsTriangles4Shadows(shader, RendEnum::TRIANGLE_MESH_POSITION_SHADER, matrix);
		for (TerrainModel* fireTower : _fireTower)					fireTower->drawAsTriangles4Shadows(shader, RendEnum::TRIANGLE_MESH_POSITION_SHADER, matrix);
		for (TerrainModel* transmissionTower : _transmissionTower)	transmissionTower->drawAsTriangles4Shadows(shader, RendEnum::TRIANGLE_MESH_POSITION_SHADER, matrix);
		this->renderLiDARModel(shader, RendEnum::TRIANGLE_MESH_POSITION_SHADER, &matrix, rendParams, true);

		multiInstanceShader->use();

		_forest->drawAsTriangles4Shadows(multiInstanceShader, RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_POSITION_SHADER, matrix);
	}
}

void TerrainScene::drawAsTriangles4Normal(const mat4& mModel, RenderingParameters* rendParams)
{
	Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
	RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::TRIANGLE_MESH_NORMAL_SHADER),
		           * multiInstanceShader = ShaderList::getInstance()->getRenderingShader(RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_NORMAL_SHADER);
	std::vector<mat4> matrix(RendEnum::numMatricesTypes());

	{
		matrix[RendEnum::MODEL_MATRIX] = mModel;
		matrix[RendEnum::VIEW_MATRIX] = activeCamera->getViewMatrix();
		matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

		shader->use();

		for (Water* water : _water) water->drawAsTriangles4Shadows(shader, RendEnum::TRIANGLE_MESH_NORMAL_SHADER, matrix);
		_terrain->drawAsTriangles4Shadows(shader, RendEnum::TRIANGLE_MESH_NORMAL_SHADER, matrix);
		_grass->drawAsTriangles4Shadows(shader, RendEnum::TRIANGLE_MESH_NORMAL_SHADER, matrix);
		for (TerrainModel* fireTower : _fireTower)					fireTower->drawAsTriangles4Shadows(shader, RendEnum::TRIANGLE_MESH_NORMAL_SHADER, matrix);
		for (TerrainModel* transmissionTower : _transmissionTower)	transmissionTower->drawAsTriangles4Shadows(shader, RendEnum::TRIANGLE_MESH_NORMAL_SHADER, matrix);
		this->renderLiDARModel(shader, RendEnum::TRIANGLE_MESH_NORMAL_SHADER, &matrix, rendParams, true);

		multiInstanceShader->use();

		_forest->drawAsTriangles4Shadows(multiInstanceShader, RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_NORMAL_SHADER, matrix);
	}
}

void TerrainScene::drawAsTriangles4Shadows(const mat4& mModel, RenderingParameters* rendParams)
{
	Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
	RenderingShader* normalShader = ShaderList::getInstance()->getRenderingShader(RendEnum::SHADOWS_SHADER),
				   * treeShader = ShaderList::getInstance()->getRenderingShader(RendEnum::MULTI_INSTANCE_SHADOWS_SHADER);
	const ivec2 canvasSize = rendParams->_viewportSize;
	std::vector<mat4> matrix(RendEnum::numMatricesTypes());

	{
		matrix[RendEnum::MODEL_MATRIX] = mModel;

		glEnable(GL_CULL_FACE);
	}

	for (unsigned int i = 0; i < _lights.size(); ++i)
	{
		Light* light = _lights[i].get();

		if (light->shouldCastShadows() && _computeShadowMap[i])
		{
			ShadowMap* shadowMap = light->getShadowMap();
			const ivec2 size = shadowMap->getSize();

			{
				shadowMap->bindFBO();
				glClear(GL_DEPTH_BUFFER_BIT);
				glViewport(0, 0, size.x, size.y);
				normalShader->applyActiveSubroutines();
			}

			matrix[RendEnum::VIEW_PROJ_MATRIX] = light->getCamera()->getViewProjMatrix();

			normalShader->use();
			for (Water* water : _water) water->drawAsTriangles4Shadows(normalShader, RendEnum::SHADOWS_SHADER, matrix);
			_terrain->drawAsTriangles4Shadows(normalShader, RendEnum::SHADOWS_SHADER, matrix);
			_grass->drawAsTriangles4Shadows(normalShader, RendEnum::SHADOWS_SHADER, matrix);
			for (TerrainModel* fireTower : _fireTower)					fireTower->drawAsTriangles4Shadows(normalShader, RendEnum::SHADOWS_SHADER, matrix);
			for (TerrainModel* transmissionTower : _transmissionTower)	transmissionTower->drawAsTriangles4Shadows(normalShader, RendEnum::SHADOWS_SHADER, matrix);

			{
				glDisable(GL_CULL_FACE);
				
				treeShader->use();
				_forest->drawAsTriangles4Shadows(treeShader, RendEnum::MULTI_INSTANCE_SHADOWS_SHADER, matrix);

				glEnable(GL_CULL_FACE);
			}

		/*	_computeShadowMap[i] = false;*/
		}
	}

	glViewport(0, 0, canvasSize.x, canvasSize.y);
	glDisable(GL_CULL_FACE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TerrainScene::drawTerrain(const mat4& mModel, RenderingParameters* rendParams)
{
	Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
	RenderingShader* terrainShader = ShaderList::getInstance()->getRenderingShader(RendEnum::TERRAIN_SHADER),
				   * grassShader = ShaderList::getInstance()->getRenderingShader(RendEnum::GRASS_SHADER),
				   * treeShader = ShaderList::getInstance()->getRenderingShader(RendEnum::TREE_TRIANGLE_MESH_SHADER),
		           * normalShader = ShaderList::getInstance()->getRenderingShader(RendEnum::TRIANGLE_MESH_SHADER);
	std::vector<mat4> matrix(RendEnum::numMatricesTypes());
	const mat4 bias = glm::translate(mat4(1.0f), vec3(0.5f)) * glm::scale(mat4(1.0f), vec3(0.5f));
	const int numLights = _lights.size();

	matrix[RendEnum::MODEL_MATRIX] = mModel;
	matrix[RendEnum::VIEW_MATRIX] = activeCamera->getViewMatrix();
	matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

	glDepthFunc(GL_LEQUAL);

	for (unsigned int i = 0; i < numLights; ++i)														// Multipass rendering
	{
		if (i == 0)
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		}

		if (_lights[i]->shouldCastShadows())
		{
			matrix[RendEnum::BIAS_VIEW_PROJ_MATRIX] = bias * _lights[i]->getCamera()->getViewProjMatrix();
		}

		// TERRAIN
		terrainShader->use();
		terrainShader->setUniform("materialScattering", rendParams->_materialScattering);				// Ambient lighting

		_lights[i]->applyLight(terrainShader, matrix[RendEnum::VIEW_MATRIX]);
		_lights[i]->applyShadowMapTexture(terrainShader, false);
		terrainShader->applyActiveSubroutines();

		_terrain->drawAsTriangles(terrainShader, RendEnum::TERRAIN_SHADER, matrix);

		// FOREST
		treeShader->use();
		treeShader->setUniform("materialScattering", rendParams->_materialScattering);			

		_lights[i]->applyLight(treeShader, matrix[RendEnum::VIEW_MATRIX]);
		_lights[i]->applyShadowMapTexture(treeShader, false);
		treeShader->applyActiveSubroutines();

		_forest->drawAsTriangles(treeShader, RendEnum::TREE_TRIANGLE_MESH_SHADER, matrix);

		// BUILDINGS
		normalShader->use();
		normalShader->setUniform("materialScattering", rendParams->_materialScattering);			

		_lights[i]->applyLight(normalShader, matrix[RendEnum::VIEW_MATRIX]);
		_lights[i]->applyShadowMapTexture(normalShader);
		normalShader->applyActiveSubroutines();

		for (TerrainModel* fireTower : _fireTower)					fireTower->drawAsTriangles(normalShader, RendEnum::TRIANGLE_MESH_SHADER, matrix);
		for (TerrainModel* transmissionTower : _transmissionTower)	transmissionTower->drawAsTriangles(normalShader, RendEnum::TRIANGLE_MESH_SHADER, matrix);
		_grass->drawAsTriangles(normalShader, RendEnum::TRIANGLE_MESH_SHADER, matrix);
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);											// Back to initial state
	glDepthFunc(GL_LESS);
}

void TerrainScene::generateWaterTextures(const mat4& mModel, RenderingParameters* rendParams)
{
	// Read currently bounded framebuffer
	GLint fboID = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fboID);

	for (Water* water : _water)
	{
		water->bindFrameBuffer(false);
		this->drawTerrain(mModel, rendParams);
		water->bindFrameBuffer(true);
	}

	// If SSAO was active => restore its FBO
	if (fboID)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	}
}

void TerrainScene::loadCameras()
{
	AABB aabb = _sceneGroup->getAABB();
	ivec2 canvasSize = Window::getInstance()->getSize();
	Camera* camera = new Camera(canvasSize[0], canvasSize[1]);
	camera->setPosition(aabb.center() - vec3(.0f, -aabb.extent().y * 3.0f, aabb.extent().z * .5f));
	camera->setLookAt(aabb.center());

	_cameraManager->insertCamera(camera);
}

void TerrainScene::loadLights()
{
	Light* keyLight = new Light();
	Camera* camera = keyLight->getCamera();
	ShadowMap* shadowMap = keyLight->getShadowMap();
	keyLight->setLightType(Light::DIRECTIONAL_LIGHT);
	keyLight->setPosition(vec3(15.0f, 20.0f, 0.0f));
	keyLight->setDirection(vec3(-1.0f, -1.0f, 0.0f));
	keyLight->setId(vec3(1.12f, 1.12f, 1.2f));
	keyLight->setIs(vec3(0.4f));
	keyLight->castShadows(true);
	keyLight->setShadowIntensity(0.7f, 1.0f);
	keyLight->setBlurFilterSize(5);

	camera->setBottomLeftCorner(vec2(-18.0f, -18.0f));
	camera->setPosition(vec3(25.0f, 20.0f, 0.0f));
	camera->setLookAt(vec3(7.0f, 9.0f, 0.0f));
	shadowMap->modifySize(8192, 8192);

	_lights.push_back(std::unique_ptr<Light>(keyLight));

	Light* rimLight = new Light();
	rimLight->setLightType(Light::RIM_LIGHT);
	rimLight->setIa(vec3(0.05f, 0.02f, 0.1f));

	_lights.push_back(std::unique_ptr<Light>(rimLight));

	Light* fillLight = new Light();
	fillLight->setLightType(Light::DIRECTIONAL_LIGHT);
	fillLight->setDirection(vec3(1.0f, 0.3f, 0.0f));
	fillLight->setId(vec3(0.4f));
	fillLight->setIs(vec3(0.0f));

	_lights.push_back(std::unique_ptr<Light>(fillLight));

	_computeShadowMap = std::vector<bool>(_lights.size());
	for (unsigned int i = 0; i < _computeShadowMap.size(); ++i)
	{
		_computeShadowMap[i] = true;
	}
}

void TerrainScene::loadModels()
{
	_sceneGroup = new Group3D();
	_terrainConfiguration._grid = new RegularGrid(vec2(.0f), vec2(1.0f), _terrainConfiguration._terrainParameters._regularGridSubdivisions);

	{
		MaterialList* materialList = MaterialList::getInstance();

		// Base terrain
		_terrain = new Terrain(_sceneGroup, &_terrainConfiguration, mat4(1.0f));
		_terrain->setSemanticGroup("Terrain");
		_terrain->setSemanticGroup(LiDARParameters::GROUND);
		_terrain->setName("Terrain");
		_sceneGroup->addComponent(_terrain);

		// Grass
		_grass = new Grass(&_terrainConfiguration, _terrain->getModelMatrix());
		_grass->setSemanticGroup("Low vegetation");
		_grass->setSemanticGroup(LiDARParameters::LOW_VEGETATION);
		_grass->setName("Grass");
		_sceneGroup->addComponent(_grass);

		// Buildings
		for (int idx = 0; idx < _terrainConfiguration._terrainParameters._numWatchers; ++idx)
		{
			TerrainModel* fireTower = new TerrainModel("Assets/Models/Buildings/Tower/Tower", "Assets/Textures/Buildings/Tower/", &_terrainConfiguration, glm::scale(mat4(1.0f), vec3(0.6f)));
			fireTower->setMaterial(materialList->getMaterial(CGAppEnum::MATERIAL_FIRE_TOWER));
			fireTower->setRadius(_terrainConfiguration._terrainParameters._watcherRadius);
			_fireTower.push_back(fireTower);

			_sceneGroup->addComponent(fireTower);
		}

		for (int idx = 0; idx < _terrainConfiguration._terrainParameters._numTransmissionTowers; ++idx)
		{
			TerrainModel* transmissionTower = new TerrainModel("Assets/Models/Buildings/TransmissionTower/T300kv", "Assets/Models/Buildings/TransmissionTower/", &_terrainConfiguration, glm::scale(mat4(1.0f), vec3(0.43f)));
			transmissionTower->setElevation(.0f);
			transmissionTower->setRadius(_terrainConfiguration._terrainParameters._transmissionTowerRadius);
			_transmissionTower.push_back(transmissionTower);

			_sceneGroup->addComponent(transmissionTower);
		}

		// Forest
		_forest = new Forest(&_terrainConfiguration, _terrain->getModelMatrix());
		_sceneGroup->addComponent(_forest);
	}

	{
		_sceneGroup->load();
		_water = _terrain->getWaterModels();

		// Assign attributes to those model component whose objects are unknown from the beginning
		{
			for (TerrainModel* fireTower: _fireTower)
			{
				fireTower->setSemanticGroup("Building");
				fireTower->setSemanticGroup(LiDARParameters::BUILDING);
				fireTower->setName("Fire Tower");
				fireTower->setMaterial(MaterialDatabase::WOOD);
			}

			for (TerrainModel* transmissionTower : _transmissionTower)
			{
				transmissionTower->setSemanticGroup("Building");
				transmissionTower->setSemanticGroup(LiDARParameters::TRANSMISSION_TOWER);
				transmissionTower->setName("Transmission Tower");
				transmissionTower->setMaterial(MaterialDatabase::IRON);
			}
		}

		_sceneGroup->registerScene();
		Group3D::StaticGPUData* staticGPUData = _sceneGroup->generateBVH(false);

		this->loadModelsCore(staticGPUData);
	}
}

// ------- Render -------

void TerrainScene::drawSceneAsLines(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	LiDARScene::drawSceneAsLines(shader, shaderType, matrix, rendParams);
}
