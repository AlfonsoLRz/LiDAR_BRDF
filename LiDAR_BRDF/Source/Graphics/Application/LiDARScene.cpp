#include "stdafx.h"
#include "LiDARScene.h"

#include "Geometry/3D/Intersections3D.h"
#include "Graphics/Application/Renderer.h"
#include "Graphics/Application/TextureList.h"
#include "Graphics/Core/OpenGLUtilities.h"
#include "Graphics/Core/RenderingShader.h"
#include "Graphics/Core/ShaderList.h"
#include "Interface/Window.h"

/// [Public methods]

LiDARScene::LiDARScene(): Scene(),
	_terrestrialLiDAR(nullptr), _aerialLiDAR(nullptr), _drawPointCloud(nullptr), _lidarPath(new DrawPath), _pointCloud(nullptr), 
	_releaseMemory(true), _LiDAR(nullptr), _aerialViewTexture(nullptr), _LiDARBeamVAO(nullptr)
{
}

LiDARScene::~LiDARScene()
{
	delete _terrestrialLiDAR;
	delete _aerialLiDAR;
	delete _drawPointCloud;
	delete _LiDAR;
	delete _aerialViewTexture;
	delete _LiDARBeamVAO;

	for (DrawRay3D* ray : _ray)
	{
		delete ray;
	}
}

void LiDARScene::clearSimulation()
{
	_pointCloud->archive();
	_drawPointCloud->updateVAO();

	for (Model3D::ModelComponent* modelComponent : *_sceneGroup->getRegisteredModelComponents())
	{
		modelComponent->clearLiDARPoints();
	}
}

void LiDARScene::load()
{	
	Scene::load();

	if (_sceneGroup) 
		this->acquireAerialView();
}

void LiDARScene::launchSimulation()
{
	_LiDAR->launchSimulation(true);
	_drawPointCloud->updateVAO();
}

void LiDARScene::render(const mat4& mModel, RenderingParameters* rendParams)
{
	Camera* activeCamera = _cameraManager->getActiveCamera();

	this->drawAsTriangles4Shadows(mModel, rendParams);

	if (rendParams->_ambientOcclusion && this->needToApplyAmbientOcclusion(rendParams))
	{
		_ssaoFBO->bindMultisamplingFBO();
		this->renderScene(mModel, rendParams);
		_ssaoFBO->writeGBuffer(0);

		_ssaoFBO->bindGBufferFBO(1);
		if (rendParams->_showTriangleMesh) this->drawAsTriangles4Position(mModel, rendParams);

		_ssaoFBO->bindGBufferFBO(2);
		if (rendParams->_showTriangleMesh) this->drawAsTriangles4Normal(mModel, rendParams);

		_ssaoFBO->bindSSAOFBO();
		this->drawSSAOScene();

		this->bindDefaultFramebuffer(rendParams);
		this->composeScene();
	}
	else
	{
		this->bindDefaultFramebuffer(rendParams);

		this->renderScene(mModel, rendParams);
	}
}

void LiDARScene::render4Reflection(Camera* camera, const mat4& mModel, RenderingParameters* rendParams)
{
	this->drawAsTriangles(camera, mModel, rendParams);
}

void LiDARScene::updatePath()
{
	auto LiDARParams = _LiDAR->getLiDARParams();

	if (LiDARParams->_LiDARType == LiDARParams->TERRESTRIAL_SPHERICAL)
		_lidarPath->updateInterpolation(LiDARParams->_tlsManualPath, LiDARParams->_tlsPosition.y, LiDARParams->_tlsManualPathCanvasSize, _sceneGroup->getAABB());
	else
	{
		if (!LiDARParams->_alsManualPath.empty())
			_lidarPath->updateInterpolation(LiDARParams->_alsManualPath, LiDARParams->_alsPosition.y, LiDARParams->_alsManualPathCanvasSize, _sceneGroup->getAABB());
		else
		{
			_lidarPath->updateInterpolation(_LiDAR->getAerialPath());
		}
	}

	_lidarPath->load();
}

/// [Protected methods]

void LiDARScene::acquireAerialView()
{
	float increasingFactor = 5.0f;
	AABB aabb = _sceneGroup->getAABB();
	float widthHeightFactor = aabb.size().x / aabb.size().z;
	ivec2 windowSize(4096, 4096 * 1.0f / widthHeightFactor);

	Camera* topCamera = new Camera(windowSize.x, windowSize.y);
	float posYdiff = aabb.extent().y * increasingFactor - aabb.size().y / 2.0f;
	topCamera->setCameraType(Camera::ORTHO_PROJ);
	topCamera->setPosition(aabb.center() + vec3(.0f, aabb.extent().y * increasingFactor, .0f));
	topCamera->setLookAt(aabb.center());
	topCamera->setUp(vec3(.0f, .0f, -1.0f));
	topCamera->setZFar(aabb.size().y * increasingFactor * 2.0f);
	topCamera->setBottomLeftCorner(vec2(-aabb.extent().x, -aabb.extent().z));

	FBOScreenshot* fboScreenshot = new FBOScreenshot(windowSize.x, windowSize.y);
	_cameraManager->insertCamera(topCamera);
	_cameraManager->setActiveCamera(1);

	// Render in FBO
	{
		Renderer* renderer = Renderer::getInstance();
		Window* window = Window::getInstance();
		auto rendParams = renderer->getRenderingParameters();
		const ivec2 size = rendParams->_viewportSize;

		this->modifyNextFramebufferID(fboScreenshot->getIdentifier());
		renderer->resize(windowSize.x, windowSize.y);
		window->changedSize(windowSize.x, windowSize.y);

		this->render(mat4(1.0f), rendParams);
		auto image = fboScreenshot->getImage();
		image->flipImageVertically();
		_aerialViewTexture = new Texture(image);
		fboScreenshot->saveImage("Top.png");

		this->modifyNextFramebufferID(0);
		renderer->resize(size.x, size.y);
		window->changedSize(size.x, size.y);
	}

	_cameraManager->setActiveCamera(0);
	_cameraManager->deleteCamera(1);
	delete fboScreenshot;
}

void LiDARScene::computeLiDARModelMatrix(std::vector<mat4>* matrix, LiDARParameters* LiDARParams)
{
	if (LiDARParams->isLiDARTerrestrial())
	{
		(*matrix)[RendEnum::MODEL_MATRIX] = glm::translate(mat4(1.0f), LiDARParams->_tlsPosition);
	}
	else
	{
		(*matrix)[RendEnum::MODEL_MATRIX] = glm::translate(mat4(1.0f), LiDARParams->_alsPosition) * matrix->at(RendEnum::MODEL_MATRIX);
	}
}

void LiDARScene::loadModelsCore(Group3D::StaticGPUData* staticGPUData)
{
	MaterialList* materialList = MaterialList::getInstance();

	_LiDAR = new LiDARSimulation(_sceneGroup);
	_LiDAR->setGroupGPUData(staticGPUData);
	_pointCloud = _LiDAR->getLiDARPointCloud();
	_drawPointCloud = new DrawLiDARPointCloud(_pointCloud);
	_drawPointCloud->load();

	// [Simulation model] Cannot collide with rays
	_terrestrialLiDAR = new CADModel("Assets/Models/Simulation/GGGJSphere", "Assets/Models/Simulation/", true);
	_terrestrialLiDAR->load();
	_terrestrialLiDAR->setModelMatrix(glm::scale(mat4(1.0f), vec3(10.0f)));
	//_LiDAR->getLiDARParams()->_tlsPosition = _sceneGroup->getAABB().center();

	_aerialLiDAR = new CADModel("Assets/Models/Simulation/GGGJSphere", "Assets/Textures/Simulation/", true);
	_aerialLiDAR->load();

	// LiDAR beam VAO
	std::vector<vec4> position{ vec4(.0f, .0f, .0f, 1.0f) };
	std::vector<unsigned> indices{ 0 };
	_LiDARBeamVAO = new VAO();
	_LiDARBeamVAO->setVBOData(RendEnum::VBO_POSITION, position);
	_LiDARBeamVAO->setIBOData(RendEnum::IBO_POINT_CLOUD, indices);	
}

void LiDARScene::loadModels()
{
	if (_sceneGroup != nullptr)
	{
		_sceneGroup->load();
		_sceneGroup->registerScene();
		Group3D::StaticGPUData* staticGPUData = _sceneGroup->generateBVH(true);

		this->loadModelsCore(staticGPUData);
	}
}

bool LiDARScene::needToApplyAmbientOcclusion(RenderingParameters* rendParams)
{
	return true/*!((rendParams->_showLidarPointCloud && _pointCloud->getNumPoints()) || rendParams->_visualizationMode == CGAppEnum::VIS_POINTS || rendParams->_visualizationMode == CGAppEnum::VIS_LINES)*/;
}

void LiDARScene::renderLiDARModel(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams, const bool shadows)
{
	if (rendParams->_showLidarModel)
	{
		LiDARParameters* LiDARParams = LiDARSimulation::getLiDARParams();
		mat4 mModel = matrix->at(RendEnum::MODEL_MATRIX);

		this->computeLiDARModelMatrix(matrix, LiDARParams);

		if (LiDARParams->isLiDARTerrestrial())
		{
			if (_LiDAR->getTLSPositions()->empty())
			{
				if (!shadows) _terrestrialLiDAR->drawAsTriangles(shader, shaderType, *matrix);
				else _terrestrialLiDAR->drawAsTriangles4Shadows(shader, shaderType, *matrix);
			}
			else
			{
				for (vec3& position : *_LiDAR->getTLSPositions())
				{
					(*matrix)[RendEnum::MODEL_MATRIX] = glm::translate(mat4(1.0f), position) * glm::scale(mat4(1.0f), vec3(5.0f));

					if (!shadows) _terrestrialLiDAR->drawAsTriangles(shader, shaderType, *matrix);
					else _terrestrialLiDAR->drawAsTriangles4Shadows(shader, shaderType, *matrix);
				}
			}
		}
		else
		{
			if (!shadows) _aerialLiDAR->drawAsTriangles(shader, shaderType, *matrix);
			else _aerialLiDAR->drawAsTriangles4Shadows(shader, shaderType, *matrix);
		}

		{
			(*matrix)[RendEnum::MODEL_MATRIX] = mModel;
		}
	}
}

void LiDARScene::renderOtherStructures(const mat4& mModel, RenderingParameters* rendParams)
{
	if (rendParams->_showBVH) this->renderBVH(mModel, rendParams);
	if (rendParams->_showLidarRays) this->renderRays(mModel, rendParams);
	if (rendParams->_showLiDARPath) this->renderLiDARPath(mModel, rendParams);
	if (rendParams->_showLidarBeam) this->renderLiDARBeam(mModel, rendParams);
	if (rendParams->_showLidarMaxRange) this->renderLiDARMaximumRange(mModel, rendParams);
}

void LiDARScene::renderPointsClouds(const mat4& mModel, RenderingParameters* rendParams)
{
	if (rendParams->_showLidarPointCloud)
	{
		switch (rendParams->_pointCloudType)
		{
		case RenderingParameters::PointCloudType::RGB:
			this->renderRGBPointCloud(mModel, rendParams);
			break;

		case RenderingParameters::PointCloudType::ASPRS_SEMANTIC:
			this->renderSemanticPointCloud(mModel, rendParams, true);
			break;

		case RenderingParameters::PointCloudType::CUSTOM_SEMANTIC:
			this->renderSemanticPointCloud(mModel, rendParams, false);
			break;

		case RenderingParameters::PointCloudType::HEIGHT:
			this->renderHeightPointCloud(mModel, rendParams);
			break;

		case RenderingParameters::PointCloudType::INTENSITY:
			this->renderIntensityPointCloud(mModel, rendParams);
			break;

		case RenderingParameters::PointCloudType::RETURN_NUMBER:
			this->renderReturnNumberPointCloud(mModel, rendParams);
			break;

		case RenderingParameters::PointCloudType::NORMAL:
			this->renderNormalPointCloud(mModel, rendParams);
			break;

		case RenderingParameters::PointCloudType::SCAN_ANGLE_RANK:
			this->renderScanAngleRankPointCloud(mModel, rendParams);
			break;

		case RenderingParameters::PointCloudType::SCAN_DIRECTION:
			this->renderScanDirectionPointCloud(mModel, rendParams);
			break;

		case RenderingParameters::PointCloudType::GPS_TIME:
			this->renderGPSTimePointCloud(mModel, rendParams);
			break;

		case RenderingParameters::PointCloudType::INSTANCE:
			this->renderInstancePointCloud(mModel, rendParams);
			break;

		case RenderingParameters::PointCloudType::UNIFORM:
			this->renderUniformPointCloud(mModel, rendParams);
			break;
		}
	}
}

void LiDARScene::renderGPSTimePointCloud(const mat4& mModel, RenderingParameters* rendParams)
{
	if (_drawPointCloud)
	{
		Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
		RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::POINT_CLOUD_GPS_TIME_SHADER);
		AABB aabb = _pointCloud->getAABB();

		std::vector<mat4> matrix(RendEnum::numMatricesTypes());

		matrix[RendEnum::MODEL_MATRIX] = mModel;
		matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

		shader->use();
		shader->applyActiveSubroutines();
		shader->setUniform("maxGPSTime", _LiDAR->getMaximumGPSTime());

		_drawPointCloud->drawAsPoints(shader, RendEnum::POINT_CLOUD_GPS_TIME_SHADER, matrix);
	}
}

void LiDARScene::renderHeightPointCloud(const mat4& mModel, RenderingParameters* rendParams)
{
	if (_drawPointCloud)
	{
		Camera* activeCamera		= _cameraManager->getActiveCamera(); if (!activeCamera) return;
		RenderingShader* shader		= ShaderList::getInstance()->getRenderingShader(rendParams->_grayscaleHeight ? RendEnum::POINT_CLOUD_GRAYSCALE_HEIGHT_SHADER : RendEnum::POINT_CLOUD_HEIGHT_SHADER);
		Material* heightMaterial	= MaterialList::getInstance()->getMaterial(CGAppEnum::MATERIAL_LIDAR_HEIGHT);
		AABB aabb = _pointCloud->getAABB();

		std::vector<mat4> matrix(RendEnum::numMatricesTypes());

		matrix[RendEnum::MODEL_MATRIX] = mModel;
		matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

		shader->use();
		if (!rendParams->_grayscaleHeight) heightMaterial->applyMaterial(shader);
		shader->setUniform("heightBoundaries", rendParams->_heightBoundaries);
		shader->setUniform("maxSceneHeight", aabb.max().y);
		shader->setUniform("minSceneHeight", aabb.min().y);
		shader->applyActiveSubroutines();

		_drawPointCloud->drawAsPoints(shader, RendEnum::POINT_CLOUD_HEIGHT_SHADER, matrix);
	}
}

void LiDARScene::renderInstancePointCloud(const mat4& mModel, RenderingParameters* rendParams)
{
	Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
	RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::POINT_CLOUD_SHADER);
	std::vector<mat4> matrix(RendEnum::numMatricesTypes());

	matrix[RendEnum::MODEL_MATRIX] = mModel;
	matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

	shader->use();

	_sceneGroup->drawCapturedPointsInstance(shader, RendEnum::POINT_CLOUD_SHADER, matrix);
}

void LiDARScene::renderIntensityPointCloud(const mat4& mModel, RenderingParameters* rendParams)
{
	if (_drawPointCloud)
	{
		Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
		RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::INTENSITY_POINT_CLOUD_SHADER);
		Material* heightMaterial = MaterialList::getInstance()->getMaterial(CGAppEnum::MATERIAL_LIDAR_HEIGHT);
		std::vector<mat4> matrix(RendEnum::numMatricesTypes());

		matrix[RendEnum::MODEL_MATRIX] = mModel;
		matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

		shader->use();
		shader->applyActiveSubroutines();
		shader->setUniform("addition", rendParams->_intensityAddition);
		shader->setUniform("multiply", rendParams->_intensityMultiplier);
		shader->setUniform("hdr", GLuint(rendParams->_intensityHDR));
		shader->setUniform("exposure", rendParams->_intensityExposure);
		shader->setUniform("gamma", rendParams->_intensityGamma);
		if (!rendParams->_grayscaleIntensity)
		{
			heightMaterial->applyMaterial(shader);
			shader->setUniform("maxReflectance", _drawPointCloud->getMinMaxIntensity().y);
			shader->setUniform("minReflectance", _drawPointCloud->getMinMaxIntensity().x);
		}
		else
		{
			shader->setUniform("maxReflectance", -1.0f);
			shader->setUniform("minReflectance", -1.0f);
		}

		_drawPointCloud->drawAsPoints(shader, RendEnum::INTENSITY_POINT_CLOUD_SHADER, matrix);
	}
}

void LiDARScene::renderNormalPointCloud(const mat4& mModel, RenderingParameters* rendParams)
{
	if (_drawPointCloud)
	{
		Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
		RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::POINT_CLOUD_NORMAL_SHADER);
		AABB aabb = _pointCloud->getAABB();

		std::vector<mat4> matrix(RendEnum::numMatricesTypes());

		matrix[RendEnum::MODEL_MATRIX] = mModel;
		matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

		shader->use();
		shader->applyActiveSubroutines();

		_drawPointCloud->drawAsPoints(shader, RendEnum::POINT_CLOUD_NORMAL_SHADER, matrix);
	}
}

void LiDARScene::renderReturnNumberPointCloud(const mat4& mModel, RenderingParameters* rendParams)
{
	if (_drawPointCloud)
	{
		Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
		RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::POINT_CLOUD_RETURN_NUMBER_SHADER);
		Texture* texture = TextureList::getInstance()->getTexture(CGAppEnum::TEXTURE_RETURN_NUMBER);
		std::vector<mat4> matrix(RendEnum::numMatricesTypes());

		matrix[RendEnum::MODEL_MATRIX] = mModel;
		matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

		shader->use();
		shader->setUniform("percentageBoundary", rendParams->_returnDivisionThreshold);
		shader->setUniform("lastRenderedReturn", rendParams->_lastReturnRendered - 1);
		texture->applyTexture(shader, 0, "texColorSampler");

		_drawPointCloud->drawAsPoints(shader, RendEnum::POINT_CLOUD_RETURN_NUMBER_SHADER, matrix);
	}
}

void LiDARScene::renderRGBPointCloud(const mat4& mModel, RenderingParameters* rendParams)
{
	if (_drawPointCloud)
	{
		Camera* camera = _cameraManager->getActiveCamera(); if (!camera) return;
		RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::POINT_CLOUD_COLOUR_SHADER);
		std::vector<mat4> matrix(RendEnum::numMatricesTypes());
		const mat4 bias = glm::translate(mat4(1.0f), vec3(0.5f)) * glm::scale(mat4(1.0f), vec3(0.5f));						// Proj: [-1, 1] => with bias: [0, 1]

		{
			matrix[RendEnum::MODEL_MATRIX] = mModel;
			matrix[RendEnum::VIEW_MATRIX] = camera->getViewMatrix();
			matrix[RendEnum::VIEW_PROJ_MATRIX] = camera->getViewProjMatrix();

			glDepthFunc(GL_LEQUAL);

			shader->use();
			shader->setUniform("materialScattering", rendParams->_materialScattering);										// Ambient lighting
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

				_lights[i]->applyLight4ColouredPoints(shader, matrix[RendEnum::VIEW_MATRIX]);
				_lights[i]->applyShadowMapTexture(shader);
				shader->applyActiveSubroutines();

				this->drawSceneAsRGBCapturedPoints(shader, RendEnum::POINT_CLOUD_COLOUR_SHADER, &matrix, rendParams);
			}
		}

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);					// Back to initial state
		glDepthFunc(GL_LESS);
	}
}

void LiDARScene::renderScanAngleRankPointCloud(const mat4& mModel, RenderingParameters* rendParams)
{
	if (_drawPointCloud)
	{
		Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
		RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::POINT_CLOUD_SCAN_ANGLE_RANK_SHADER);
		std::vector<mat4> matrix(RendEnum::numMatricesTypes());

		matrix[RendEnum::MODEL_MATRIX] = mModel;
		matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

		shader->use();

		_drawPointCloud->drawAsPoints(shader, RendEnum::POINT_CLOUD_SCAN_ANGLE_RANK_SHADER, matrix);
	}
}

void LiDARScene::renderScanDirectionPointCloud(const mat4& mModel, RenderingParameters* rendParams)
{
	if (_drawPointCloud)
	{
		Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
		RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::POINT_CLOUD_SCAN_DIRECTION_SHADER);

		std::vector<mat4> matrix(RendEnum::numMatricesTypes());

		matrix[RendEnum::MODEL_MATRIX] = mModel;
		matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

		shader->use();
		shader->applyActiveSubroutines();

		_drawPointCloud->drawAsPoints(shader, RendEnum::POINT_CLOUD_SCAN_DIRECTION_SHADER, matrix);
	}
}

void LiDARScene::renderSemanticPointCloud(const mat4& mModel, RenderingParameters* rendParams, bool ASPRS)
{
	Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
	RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::POINT_CLOUD_SHADER);
	std::vector<mat4> matrix(RendEnum::numMatricesTypes());

	matrix[RendEnum::MODEL_MATRIX] = mModel;
	matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

	shader->use();

	_sceneGroup->drawCapturedPointsWithGroups(shader, RendEnum::POINT_CLOUD_SHADER, matrix, ASPRS);
}

void LiDARScene::renderUniformPointCloud(const mat4& mModel, RenderingParameters* rendParams)
{
	if (_drawPointCloud)
	{
		Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
		RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::POINT_CLOUD_SHADER);
		std::vector<mat4> matrix(RendEnum::numMatricesTypes());

		matrix[RendEnum::MODEL_MATRIX] = mModel;
		matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

		shader->use();
		shader->applyActiveSubroutines();

		_drawPointCloud->drawAsPoints(shader, RendEnum::POINT_CLOUD_SHADER, matrix);
	}
}

void LiDARScene::renderScene(const mat4& mModel, RenderingParameters* rendParams)
{
	int visualizationMode = rendParams->_visualizationMode;

	switch (visualizationMode)
	{
	case CGAppEnum::VIS_POINTS:
		this->renderPointCloud(mModel, rendParams);
		break;
	case CGAppEnum::VIS_LINES:
		this->renderWireframe(mModel, rendParams);
		break;
	case CGAppEnum::VIS_TRIANGLES:
		this->renderTriangleMesh(mModel, rendParams);
		break;
	case CGAppEnum::VIS_ALL_TOGETHER:
		this->renderTriangleMesh(mModel, rendParams);
		this->renderWireframe(mModel, rendParams);
		this->renderPointCloud(mModel, rendParams);
		break;
	}

	this->renderPointsClouds(mModel, rendParams);
	this->renderOtherStructures(mModel, rendParams);
}

void LiDARScene::renderPointCloud(const mat4& mModel, RenderingParameters* rendParams)
{
	this->drawAsPoints(mModel, rendParams);
}

void LiDARScene::renderWireframe(const mat4& mModel, RenderingParameters* rendParams)
{
	this->drawAsLines(mModel, rendParams);
}

void LiDARScene::renderTriangleMesh(const mat4& mModel, RenderingParameters* rendParams)
{
	if (rendParams->_showTriangleMesh)
	{
		if (rendParams->_renderSemanticConcept)
		{
			this->drawAsTrianglesWithGroups(mModel, rendParams);
		}
		else
		{
			LiDARScene::drawAsTriangles(mModel, rendParams);
		}
	}
}

// ---------------------------- Other structures -----------------------------

void LiDARScene::renderBVH(const mat4& model, RenderingParameters* rendParams)
{
	Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
	std::vector<mat4> matrix(RendEnum::numMatricesTypes());

	matrix[RendEnum::MODEL_MATRIX] = model;
	matrix[RendEnum::VIEW_MATRIX] = activeCamera->getViewMatrix();
	matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

	// BVH rendering
	{
		RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::WIREFRAME_SHADER);
		shader->use();
		shader->applyActiveSubroutines();

		_sceneGroup->drawBVH(shader, RendEnum::WIREFRAME_SHADER, matrix);
	}
}

void LiDARScene::renderLiDARBeam(const mat4& model, RenderingParameters* rendParams)
{
	Camera* activeCamera	= _cameraManager->getActiveCamera(); if (!activeCamera) return;
	auto LiDARParams		= _LiDAR->getLiDARParams();
	std::vector<mat4> matrix(RendEnum::numMatricesTypes());

	matrix[RendEnum::MODEL_MATRIX] = model;
	matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

	{
		RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::GEOMETRY_CONE_SHADER);
		shader->use();
		shader->setUniform("aabbMax", _sceneGroup->getAABB().max());
		shader->setUniform("aabbMin", _sceneGroup->getAABB().min());
		shader->setUniform("beamNormal", rendParams->_lidarBeamNormal);
		shader->setUniform("coneRadius", LiDARParams->_pulseRadius);
		shader->setUniform("LiDARPosition", LiDARParams->isLiDARTerrestrial() ? LiDARParams->_tlsPosition : LiDARParams->_alsPosition);
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("subdivisions", GLuint(rendParams->_numLiDARRBeamSubdivisions));

		_LiDARBeamVAO->drawObject(RendEnum::IBO_POINT_CLOUD, GL_POINTS, 1);
	}
}

void LiDARScene::renderLiDARMaximumRange(const mat4& model, RenderingParameters* rendParams)
{
	Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
	auto LiDARParams = _LiDAR->getLiDARParams();
	std::vector<mat4> matrix(RendEnum::numMatricesTypes());

	matrix[RendEnum::MODEL_MATRIX] = model;
	matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

	{
		RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::GEOMETRY_RANGE_SHADER);
		shader->use();
		shader->setUniform("LiDARPosition", LiDARParams->isLiDARTerrestrial() ? LiDARParams->_tlsPosition : LiDARParams->_alsPosition);
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("subdivisions", GLuint(rendParams->_numLiDARRangeSubdivisions));
		shader->setUniform("maxDistance", vec2(LiDARParams->_maxRange) + LiDARParams->_maxRangeSoftBoundary);

		_LiDARBeamVAO->drawObject(RendEnum::IBO_POINT_CLOUD, GL_POINTS, 1);
	}
}

void LiDARScene::renderLiDARPath(const mat4& mModel, RenderingParameters* rendParams)
{
	LiDARParameters* lidarParams = LiDARSimulation::getLiDARParams();
	Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
	RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::WIREFRAME_SHADER);
	std::vector<mat4> matrix(RendEnum::numMatricesTypes());

	matrix[RendEnum::MODEL_MATRIX] = mModel;
	matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

	shader->use();
	shader->applyActiveSubroutines();

	glLineWidth(rendParams->_lineWidth);
	_lidarPath->drawAsLines(shader, RendEnum::WIREFRAME_SHADER, matrix);
}

void LiDARScene::renderRays(const mat4& mModel, RenderingParameters* rendParams)
{
	VAO* vao = _LiDAR->getRayVAO();
	if (vao == nullptr) return;

	Camera* activeCamera = _cameraManager->getActiveCamera(); if (!activeCamera) return;
	RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::WIREFRAME_SHADER);
	std::vector<mat4> matrix(RendEnum::numMatricesTypes());

	matrix[RendEnum::MODEL_MATRIX] = mModel;
	matrix[RendEnum::VIEW_MATRIX] = activeCamera->getViewMatrix();
	matrix[RendEnum::VIEW_PROJ_MATRIX] = activeCamera->getViewProjMatrix();

	shader->use();
	shader->applyActiveSubroutines();
	shader->setUniform("vColor", vec3(1.0f, 1.0f, .0f));
	shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);

	vao->drawObject(RendEnum::IBOTypes::IBO_WIREFRAME, GL_LINES, rendParams->_raysPercentage * _LiDAR->getNumRays() * 3);
}

// ------------------------- Draw scene ------------------------------

void LiDARScene::drawSceneAsRGBCapturedPoints(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	_sceneGroup->drawCapturedPointsRGB(shader, shaderType, *matrix);
}

void LiDARScene::drawSceneAsTriangles(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	Scene::drawSceneAsTriangles(shader, shaderType, matrix, rendParams);

	this->renderLiDARModel(shader, shaderType, matrix, rendParams);
}

void LiDARScene::drawSceneAsTriangles4Normal(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	Scene::drawSceneAsTriangles4Normal(shader, shaderType, matrix, rendParams);

	this->renderLiDARModel(shader, shaderType, matrix, rendParams, true);
}

void LiDARScene::drawSceneAsTriangles4Position(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	Scene::drawSceneAsTriangles4Position(shader, shaderType, matrix, rendParams);

	this->renderLiDARModel(shader, shaderType, matrix, rendParams, true);
}
