#include "stdafx.h"
#include "Renderer.h"

#include "Graphics/Application/CADScene.h"
#include "Interface/Window.h"
#include "Utilities/FileManagement.h"

// [Static attributes]

const std::string Renderer::SCENE_CONFIGURATION_FILE = "Settings/SceneConfig.txt";
const std::string Renderer::STR_LINE_COMMENT = "#";

/// [Protected methods]

Renderer::Renderer() :
	_screenshotFBO(nullptr),
	_state(std::unique_ptr<RenderingParameters>(new RenderingParameters()))
{
}

void Renderer::createScene(const uint8_t sceneType, std::vector<std::string>& additionalInformation)
{
	switch (sceneType)
	{
		case CGAppEnum::CAD_SCENE: 
			_scene.reset(new CADScene());
			if (!additionalInformation.empty())
				CADScene::setRootScene(additionalInformation[0]);

			break;
	}
}

void Renderer::readSceneIndex(uint8_t& sceneIndex, std::vector<std::string>& additionalInformation)
{
	std::string fileLine;		
	std::vector<float> floatTokens;
	std::vector<std::string> strTokens;

	std::ifstream fin(SCENE_CONFIGURATION_FILE, std::ios::in);
	if (fin.is_open())
	{
		while (std::getline(fin, fileLine))
		{
			FileManagement::clearTokens(strTokens, floatTokens);
			FileManagement::readTokens(fileLine, ' ', strTokens, floatTokens);

			if (!strTokens.empty())
			{
				if (!strTokens[0]._Starts_with(STR_LINE_COMMENT))
				{
					if (strTokens[0]._Equal("CAD"))
					{
						sceneIndex = CGAppEnum::CAD_SCENE;
					}
					else if (strTokens[0]._Equal("Terrain"))
					{
						sceneIndex = CGAppEnum::TERRAIN_SCENE;
					}

					if (strTokens.size() >= 2)
						additionalInformation.insert(additionalInformation.begin(), strTokens.begin() + 1, strTokens.end());
				}
			}
		}
		
		sceneIndex = glm::clamp(sceneIndex, static_cast<uint8_t>(CGAppEnum::TERRAIN_SCENE), static_cast<uint8_t>(CGAppEnum::CAD_SCENE));
		fin.close();
	}
}

/// [Public methods]

Renderer::~Renderer()
{
}

void Renderer::prepareOpenGL(const uint16_t width, const uint16_t height)
{
	glClearColor(_state->_backgroundColor.x, _state->_backgroundColor.y, _state->_backgroundColor.z, 1.0f);

	glEnable(GL_DEPTH_TEST);							// Depth is taking into account when drawing and only points with LESS depth are taken into account
	glDepthFunc(GL_LESS);

	glEnable(GL_BLEND);									// Multipass rendering

	glEnable(GL_MULTISAMPLE);							// Antialiasing

	glEnable(GL_POINT_SPRITE);
	glEnable(GL_PROGRAM_POINT_SIZE);					// Point clouds of different point sizes

	glCullFace(GL_FRONT);								// Necessary for shadow mapping, even tho we need an enable order before it

	glEnable(GL_PRIMITIVE_RESTART);						// Index which marks different primitives
	glPrimitiveRestartIndex(Model3D::RESTART_PRIMITIVE_INDEX);

	ComputeShader::initializeMaxGroupSize();			// Once the context is ready we can query for maximum work group size
	Model3D::buildShadowOffsetTexture();				// Alternative shadow technique
	Model3D::buildSSAONoiseKernels();					// Ambient occlusion samples

	// [Framebuffers]

	_screenshotFBO = std::unique_ptr<FBOScreenshot>(new FBOScreenshot(width, height));

	// [State]

	_state->_viewportSize = ivec2(width, height);

	// [Scenes]

	uint8_t sceneIndex = UINT8_MAX;
	std::vector<std::string> additionalInformation;

	this->readSceneIndex(sceneIndex, additionalInformation);
	this->createScene(sceneIndex, additionalInformation);

	if (!_scene.get())
		throw std::runtime_error("No scene could be generated, please configure the scene file.");
	_scene->load();
}

void Renderer::render()
{
	_scene->render(mat4(1.0f), _state.get());
}

bool Renderer::getScreenshot(const std::string& filename)
{
	const ivec2 size = _state->_viewportSize;
	const ivec2 newSize = ivec2(_state->_viewportSize.x * _state->_screenshotMultiplier, _state->_viewportSize.y * _state->_screenshotMultiplier);

	_scene->modifyNextFramebufferID(_screenshotFBO->getIdentifier());
	this->resize(newSize.x, newSize.y);
	Window::getInstance()->changedSize(newSize.x, newSize.y);

	this->render();
	const bool success = _screenshotFBO->saveImage(filename);

	_scene->modifyNextFramebufferID(0);
	this->resize(size.x, size.y);
	Window::getInstance()->changedSize(size.x, size.y);

	return success;
}

void Renderer::resize(const uint16_t width, const uint16_t height)
{
	// Viewport state
	_state->_viewportSize = ivec2(width, height);

	// OpenGL
	glViewport(0, 0, width, height);

	// Scenes
	_scene->modifySize(width, height);

	// FBO
	_screenshotFBO->modifySize(width, height);
}
