#pragma once

#include "Graphics/Application/LiDARScene.h"
#include "Graphics/Core/FBOScreenshot.h"
#include "Graphics/Core/Model3D.h"
#include "Graphics/Core/Texture.h"

class ReflectiveObject: public Model3D
{
public:
	enum RefractiveMaterial : uint8_t { AIR, WATER, ICE, GLASS, DIAMOND };

protected:
	static const float	CUBE_MAP_SIZE;
	static std::unordered_map<uint8_t, float> REFRACTIVE_INDEX;
	static bool			_render;

protected:
	ModelComponent* _copyModelComp;				//!< Model component to be copied during load
	Model3D*		_copyModel;

	// Reflection support
	Camera*			_camera;
	Texture*		_environmentMapping;
	FBOScreenshot*	_fbo;
	uint8_t			_refractiveMaterial;

	// Rendering 
	float			_baseColorWeight;
	bool			_capturedEnvironment;
	float			_reflectionWeight;
	LiDARScene*		_scene;

protected:
	/**
	*	@brief Generates geometry via GPU.
	*/
	void generateGeometryTopology(const mat4& modelMatrix);

public:
	/**
	*	@brief Reflective object constructor. Depends on an external Model component.
	*	@param modelMatrix First model transformation.
	*/
	ReflectiveObject(ModelComponent* modelComp, LiDARScene* scene, const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Reflective object constructor. Depends on an external Model component.
	*	@param modelMatrix First model transformation.
	*/
	ReflectiveObject(Model3D* model, LiDARScene* scene);

	/**
	*	@brief Deleted copy constructor.
	*	@param model Model to copy attributes.
	*/
	ReflectiveObject(const ReflectiveObject& model) = delete;

	/**
	*	@brief Destructor.
	*/
	virtual ~ReflectiveObject();

	/**
	*	@brief Creates the cube map for the following renders.
	*/
	void captureEnvironment(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@return True if environment map has already been obtained.
	*/
	bool capturedScene() { return _capturedEnvironment; }

	/**
	*	@brief Renders the model as a set of lines.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsLines(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the model as a set of points.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the model as a set of triangles.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the model as a set of triangles with reflection.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsReflectiveTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, Camera* camera, const GLuint lightIndex = 0);

	/**
	*	@brief Renders the model as a set of triangles with no textures as we only want to color fragments taking into account its depth.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTriangles4Fog(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the model as a set of triangles with no texture as we only want to retrieve the depth.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the points captured by a sensor (coloured).
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawCapturedPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the points captured by a sensor (coloured).
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawReflectiveCapturedPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, Camera* camera, const GLuint lightIndex = 0);

	/**
	*	@brief Renders the points captured by a sensor. Colors depend on the group they belong to.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawCapturedPointsWithGroups(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, bool ASPRS = true);

	/**
	*	@brief Loads the model data from file.
	*	@return Success of operation.
	*/
	virtual bool load(const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Assignment operator overriding.
	*	@param model Model to copy attributes.
	*/
	ReflectiveObject& operator=(const ReflectiveObject& model) = delete;

	/**
	*	@brief Modifies the weight of base color vs environment color.
	*/
	void setBaseColorWeight(const float weight) { _baseColorWeight = weight; }

	/**
	*	@brief Modifies the weight of reflection vs refraction.
	*/
	void setReflectiveWeight(const float weight) { _reflectionWeight = weight; }

	/**
	*	@brief Modifies the refractive index.
	*/
	void setRefractiveMaterial(const RefractiveMaterial material) { _refractiveMaterial = material; }

	/**
	*	@brief Allows or forbids rendering.
	*/
	void setRender(bool render) { _render = render; }
};

