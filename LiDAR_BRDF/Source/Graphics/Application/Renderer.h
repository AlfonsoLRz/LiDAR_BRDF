#pragma once

#include "Graphics/Application/CameraManager.h"
#include "Graphics/Application/GraphicsAppEnumerations.h"
#include "Graphics/Application/LiDARScene.h"
#include "Graphics/Application/RenderingParameters.h"
#include "Graphics/Application/Scene.h"
#include "Graphics/Core/BRDFDatabase.h"
#include "Graphics/Core/Camera.h"
#include "Graphics/Core/FBOScreenshot.h"
#include "Graphics/Core/Model3D.h"
#include "Graphics/Core/RenderingShader.h"
#include "Utilities/Singleton.h"

/**
*	@file InputManager.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 07/11/2019
*/

/**
*	@brief Manages the scenes and graphic application state.
*/
class Renderer: public Singleton<Renderer>
{
	friend class Singleton<Renderer>;

public:
	const static std::string STR_LINE_COMMENT;

protected:
	const static std::string SCENE_CONFIGURATION_FILE;
	const static std::string TERRAIN_STORE_FOLDER;

protected:
	// [Rendering]
	std::unique_ptr<FBOScreenshot>				_screenshotFBO;			//!< Framebuffer which allows us to capture the scene (and save it) at higher resolution
	std::unique_ptr<LiDARScene>					_scene;					//!< Scene manager
	std::unique_ptr<RenderingParameters>		_state;					//!< Properties of rendering

protected:
	/**
	*	@brief Default constructor with the only purpose of initializing properly the attributes.
	*/
	Renderer();

	/**
	*	@brief Creates an scene taking into account the type from the arguments.
	*	@param sceneType Index-name of scene type.
	*/
	void createScene(const uint8_t sceneType, std::vector<std::string>& additionalInformation);

	/**
	*	@brief Reads the index of the scene which needs to be loaded at first.
	*/
	void readSceneIndex(uint8_t& sceneIndex, std::vector<std::string>& additionalInformation);

public:
	/**
	*	@brief Destructor. Frees resources.
	*/
	virtual ~Renderer();

	/**
	*	@brief Initializes the OpenGL variables at the current context.
	*	@param width Canvas initial width.
	*	@param height Canvas initial height.
	*/
	void prepareOpenGL(const uint16_t width, const uint16_t height);

	/**
	*	@brief Draws the currently active scene.
	*/
	void render();

	// [Scene data]

	/**
	*	@return Current camera that allows us to capture the scene.
	*/
	Camera* getActiveCamera()  const { return _scene->getCameraManager()->getActiveCamera(); }

	/**
	*	@return Color of background under the scene.
	*/
	vec3 getBackgroundColor()  const { return _state->_backgroundColor; }

	/**
	*	@return Currently active scene.
	*/
	LiDARScene* getCurrentScene() { return _scene.get(); }

	/**
	*	@return Wrapping class for rendering parameters (unique instance per renderer).
	*/
	RenderingParameters* getRenderingParameters() { return _state.get(); }

	/**
	*	@brief Renders the scene into an FBO.
	*	@param filename Path of file system where the image needs to be saved.
	*/
	bool getScreenshot(const std::string& filename);

	// [Events]

	/**
	*	@brief Resize event from canvas view.
	*	@param width New canvas width.
	*	@param height New canvas height.
	*/
	void resize(const uint16_t width, const uint16_t height);
};

