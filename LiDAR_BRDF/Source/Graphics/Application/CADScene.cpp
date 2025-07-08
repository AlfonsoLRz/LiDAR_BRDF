#include "stdafx.h"
#include "CADScene.h"

#include <filesystem>
#include <regex>
#include "Graphics/Application/TextureList.h"
#include "Graphics/Core/CADModel.h"
#include "Graphics/Core/Light.h"
#include "Graphics/Core/OpenGLUtilities.h"

/// Initialization of static attributes
std::string CADScene::SCENE_ROOT_FOLDER = "Assets/Scene/";
std::string CADScene::SCENE_SETTINGS_FOLDER = "Assets/Scene/Settings/";

const std::string CADScene::SCENE_CAMERA_FILE = "Camera.txt";
const std::string CADScene::SCENE_LIGHTS_FILE = "Lights.txt";

// [Public methods]

CADScene::CADScene()
{
}

CADScene::~CADScene()
{
}

void CADScene::render(const mat4& mModel, RenderingParameters* rendParams)
{
	LiDARScene::render(mModel, rendParams);
}

void CADScene::setRootScene(const std::string& name)
{
	std::string sceneName = name;
	if (name.empty())
	{
		sceneName = "Conference";
	}
	sceneName += "/";

	SCENE_ROOT_FOLDER += sceneName;
	SCENE_SETTINGS_FOLDER += sceneName;
}

// [Protected methods]

bool CADScene::isExtensionReadable(const std::string& filename)
{
	return filename.find(CADModel::OBJ_EXTENSION) != std::string::npos;
}

void CADScene::loadDefaultCamera(Camera* camera)
{
	camera->setPosition(_sceneGroup->getAABB().center() + _sceneGroup->getAABB().extent() * 1.5f);
	camera->setLookAt(_sceneGroup->getAABB().center());
}

void CADScene::loadDefaultLights()
{
	Light* pointLight_01 = new Light();
	pointLight_01->setLightType(Light::POINT_LIGHT);
	pointLight_01->setPosition(vec3(1.64f, 2.0f, -0.12f));
	pointLight_01->setId(vec3(0.35f));
	pointLight_01->setIs(vec3(0.0f));

	_lights.push_back(std::unique_ptr<Light>(pointLight_01));

	Light* pointLight_02 = new Light();
	pointLight_02->setLightType(Light::POINT_LIGHT);
	pointLight_02->setPosition(vec3(-2.86f, 2.0f, -0.13f));
	pointLight_02->setId(vec3(0.35f));
	pointLight_02->setIs(vec3(0.0f));

	_lights.push_back(std::unique_ptr<Light>(pointLight_02));

	Light* sunLight = new Light();
	Camera* camera = sunLight->getCamera();
	ShadowMap* shadowMap = sunLight->getShadowMap();
	camera->setBottomLeftCorner(vec2(-7.0f, -7.0f));
	shadowMap->modifySize(4096, 4096);
	sunLight->setLightType(Light::DIRECTIONAL_LIGHT);
	sunLight->setPosition(vec3(.0f, 3.0f, -5.0f));
	sunLight->setDirection(vec3(-0.1, -0.8f, 1.0f));
	sunLight->setId(vec3(0.5f));
	sunLight->setIs(vec3(0.0f));
	sunLight->castShadows(true);
	sunLight->setShadowIntensity(0.2f, 1.0f);
	sunLight->setBlurFilterSize(5);

	_lights.push_back(std::unique_ptr<Light>(sunLight));

	Light* fillLight = new Light();
	fillLight->setLightType(Light::DIRECTIONAL_LIGHT);
	fillLight->setDirection(vec3(-1.0f, 1.0f, 0.0f));
	fillLight->setId(vec3(0.1f));
	fillLight->setIs(vec3(0.0f));

	_lights.push_back(std::unique_ptr<Light>(fillLight));

	Light* rimLight = new Light();
	rimLight->setLightType(Light::RIM_LIGHT);
	rimLight->setIa(vec3(0.015f, 0.015f, 0.05f));

	_lights.push_back(std::unique_ptr<Light>(rimLight));
}

void CADScene::loadCameras()
{
	ivec2 canvasSize = _window->getSize();
	Camera* camera = new Camera(canvasSize[0], canvasSize[1]);

	if (!this->readCameraFromSettings(camera))
	{
		this->loadDefaultCamera(camera);
	}

	_cameraManager->insertCamera(camera);
}

void CADScene::loadLights()
{
	if (!this->readLightsFromSettings())
	{
		this->loadDefaultLights();
	}

	Scene::loadLights();
}

void CADScene::loadModels()
{
	MaterialList* materialList = MaterialList::getInstance();

	_sceneGroup = new Group3D();
	_sceneGroup->setMaterial(materialList->getMaterial(CGAppEnum::MATERIAL_CAD_WHITE));

	const unsigned rootFolderLength = SCENE_ROOT_FOLDER.length();
	const std::string textureFolder = SCENE_ROOT_FOLDER;

	for (auto& assetFile : std::filesystem::recursive_directory_iterator(SCENE_ROOT_FOLDER))
	{
		if (!assetFile.is_directory() && this->isExtensionReadable(assetFile.path().generic_string()))
		{
			std::string modelPath = assetFile.path().generic_string();
			const size_t extensionDotIndex_01 = modelPath.find_last_of('.');

			CADModel* model = new CADModel(modelPath.substr(0, extensionDotIndex_01), textureFolder, true);
			_sceneGroup->addComponent(model);
		}
	}

	LiDARScene::loadModels();
}

bool CADScene::readCameraFromSettings(Camera* camera)
{
	const std::string filename = SCENE_SETTINGS_FOLDER + SCENE_CAMERA_FILE;
	std::string currentLine, lineHeader;	
	std::stringstream line;
	std::ifstream inputStream;	
	std::vector<float> floatValues;
	std::vector<std::string> strValues;

	//inputStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	inputStream.open(filename.c_str());

	if (inputStream.fail()) return false;

	while (!(inputStream >> std::ws).eof())
	{
		std::getline(inputStream, currentLine);

		line.clear();
		line.str(currentLine);
		std::getline(line, lineHeader, ' ');

		if (lineHeader.find(COMMENT_CHAR) == std::string::npos)
		{
			floatValues.clear();
			FileManagement::readTokens(currentLine, ' ', strValues, floatValues);

			if (floatValues.size() == 3 && lineHeader._Starts_with(CAMERA_POS_HEADER))
			{
				camera->setPosition(vec3(floatValues[0], floatValues[1], floatValues[2]));
			}
			else if (floatValues.size() == 3 && lineHeader._Starts_with(CAMERA_LOOKAT_HEADER))
			{
				camera->setLookAt(vec3(floatValues[0], floatValues[1], floatValues[2]));
			}
			else if (!floatValues.empty() && lineHeader._Starts_with(CAMERA_FOV_X_HEADER))
			{
				camera->setFovX(floatValues[0] * M_PI / 180.0f);
			}
			else if (!floatValues.empty() && lineHeader._Starts_with(CAMERA_FOV_Y_HEADER))
			{
				camera->setFovY(floatValues[0] * M_PI / 180.0f);
			}
		}
	}

	inputStream.close();	

	return true;
}

bool CADScene::readLightsFromSettings()
{
	// File management
	const std::string filename = SCENE_SETTINGS_FOLDER + SCENE_LIGHTS_FILE;
	std::string currentLine, lineHeader;
	std::stringstream line;
	std::ifstream inputStream;

	Light* light = nullptr;
	vec3 vec3val;
	vec2 vec2val;
	float floatval;
	std::string stringval;

	//inputStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	inputStream.open(filename.c_str());

	if (inputStream.fail()) return false;

	while (!(inputStream >> std::ws).eof())
	{
		std::getline(inputStream, currentLine);

		line.clear();
		line.str(currentLine);
		std::getline(line, lineHeader, '\t');

		if (lineHeader.empty())
		{
			std::getline(line, lineHeader, ' ');
		}

		if (lineHeader.find(COMMENT_CHAR) == std::string::npos)
		{
			if (lineHeader == NEW_LIGHT)
			{
				if (light) _lights.push_back(std::unique_ptr<Light>(light));

				light = new Light();
			}
			else if (light)
			{
				if (lineHeader.find(LIGHT_POSITION) != std::string::npos)
				{
					for (int i = 0; i < 3; ++i) { line >> vec3val[i]; line.ignore(); }

					light->setPosition(vec3val);
				}
				else if (lineHeader.find(LIGHT_DIRECTION) != std::string::npos)
				{
					for (int i = 0; i < 3; ++i) { line >> vec3val[i]; line.ignore(); }

					light->setDirection(vec3val);
				}
				else if (lineHeader.find(LIGHT_TYPE) != std::string::npos)
				{
					line >> stringval;

					Light::LightModels type = Light::stringToLightModel(stringval);
					light->setLightType(type);
				}
				else if (lineHeader.find(AMBIENT_INTENSITY) != std::string::npos)
				{
					for (int i = 0; i < 3; ++i) { line >> vec3val[i]; line.ignore(); }

					light->setIa(vec3val);
				}
				else if (lineHeader.find(DIFFUSE_INTENSITY) != std::string::npos)
				{
					for (int i = 0; i < 3; ++i) { line >> vec3val[i]; line.ignore(); }

					light->setId(vec3val);
				}
				else if (lineHeader.find(SPECULAR_INTENSITY) != std::string::npos)
				{
					for (int i = 0; i < 3; ++i) { line >> vec3val[i]; line.ignore(); }

					light->setIs(vec3val);
				}
				else if (lineHeader.find(SHADOW_MAP_SIZE) != std::string::npos)
				{
					for (int i = 0; i < 2; ++i) { line >> vec2val[i]; line.ignore(); }

					light->getShadowMap()->modifySize(vec2val.x, vec2val.y);
				}
				else if (lineHeader.find(BLUR_SHADOW_SIZE) != std::string::npos)
				{
					line >> floatval;

					light->setBlurFilterSize(floatval);
				}
				else if (lineHeader.find(ORTHO_SIZE) != std::string::npos)
				{
					for (int i = 0; i < 2; ++i) { line >> vec2val[i]; line.ignore(); }

					light->getCamera()->setBottomLeftCorner(vec2val);
				}
				else if (lineHeader.find(SHADOW_INTENSITY) != std::string::npos)
				{
					for (int i = 0; i < 2; ++i) { line >> vec2val[i]; line.ignore(); }

					light->setShadowIntensity(vec2val.x, vec2val.y);
				}
				else if (lineHeader.find(CAST_SHADOWS) != std::string::npos)
				{
					line >> stringval;

					light->castShadows(stringval == "true" || stringval == "True");
				}
				else if (lineHeader.find(SHADOW_CAMERA_ANGLE_X) != std::string::npos)
				{
					line >> floatval;

					light->getCamera()->setFovX(floatval);
				}
				else if (lineHeader.find(SHADOW_CAMERA_ANGLE_Y) != std::string::npos)
				{
					line >> floatval;

					light->getCamera()->setFovY(floatval);
				}
				else if (lineHeader.find(SHADOW_CAMERA_RASPECT) != std::string::npos)
				{
					for (int i = 0; i < 2; ++i) { line >> vec2val[i]; line.ignore(); }

					light->getCamera()->setRaspect(vec2val.x, vec2val.y);
				}
				else if (lineHeader.find(SHADOW_RADIUS) != std::string::npos)
				{
					line >> floatval;

					light->setShadowRadius(floatval);
				}
				else if (lineHeader.find(SHADOW_CAMERA_ZFAR) != std::string::npos)
				{
					line >> floatval;

					light->getCamera()->setZFar(floatval);
				}
			}
		}
	}

	if (light) _lights.push_back(std::unique_ptr<Light>(light));

	inputStream.close();

	return true;
}