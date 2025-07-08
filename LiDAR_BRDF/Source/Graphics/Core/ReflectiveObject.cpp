#include "stdafx.h"
#include "ReflectiveObject.h"

#include "Graphics/Core/OpenGLUtilities.h"
#include "Graphics/Core/ShaderList.h"
#include "Graphics/Core/VAO.h"
#include "Interface/Window.h"

/// Initialization of static attributes
const float ReflectiveObject::CUBE_MAP_SIZE = 512;
std::unordered_map<uint8_t, float> ReflectiveObject::REFRACTIVE_INDEX {
	{ ReflectiveObject::RefractiveMaterial::AIR, 1.0f },
	{ ReflectiveObject::RefractiveMaterial::WATER, 1.33f },
	{ ReflectiveObject::RefractiveMaterial::ICE, 1.309f },
	{ ReflectiveObject::RefractiveMaterial::GLASS, 1.52f },
	{ ReflectiveObject::RefractiveMaterial::DIAMOND, 2.42f },
};

bool ReflectiveObject::_render = true;

/// Public methods

ReflectiveObject::ReflectiveObject(ModelComponent* modelComp, LiDARScene* scene, const mat4& modelMatrix)
	: Model3D(modelMatrix, 1), _baseColorWeight(.5f), _capturedEnvironment(false), _copyModelComp(modelComp), _environmentMapping(nullptr), _refractiveMaterial(GLASS), _reflectionWeight(.5f), _scene(scene)
{
	_camera = new Camera(CUBE_MAP_SIZE, CUBE_MAP_SIZE);
	_camera->setFovX(90.0f * glm::pi<float>() / 180.0f);
	_camera->setFovY(90.0f * glm::pi<float>() / 180.0f);
	_camera->setPosition(vec3(modelMatrix * vec4(.0f, .0f, .0f, 1.0f)));
	_fbo = new FBOScreenshot(CUBE_MAP_SIZE, CUBE_MAP_SIZE);
}

ReflectiveObject::ReflectiveObject(Model3D* model, LiDARScene* scene)
	: ReflectiveObject((ModelComponent*) nullptr, scene, model->getModelMatrix())
{
	_copyModel = model;
}

ReflectiveObject::~ReflectiveObject()
{
	delete _camera;
	delete _copyModelComp;
	delete _copyModel;
	delete _environmentMapping;
	delete _fbo;
}

void ReflectiveObject::captureEnvironment(const mat4& mModel, RenderingParameters* rendParams)
{
	_render = false;

	{
		std::vector<vec3> directions = { 
			vec3(-1.0f, .0f, .0f),
			vec3(1.0f, .0f, .0f),
			vec3(.0f, 1.0f, .0f),
			vec3(.0f, -1.0f, .0f),
			vec3(.0f, .0f, 1.0f),
			vec3(.0f, .0f, -1.0f),				// Back
		};

		std::vector<Image*> images;
		const ivec2 canvasSize = Window::getInstance()->getSize();
		//int i = 0;

		for (vec3& dir : directions)
		{
			_camera->setLookAt(_camera->getEye() + dir);
			if (dir.y < .0f) _camera->setUp(vec3(.0f, .0f, 1.0f));
			else if (dir.y > .0f) _camera->setUp(vec3(.0f, .0f, -1.0f));
			else _camera->setUp(vec3(.0f, 1.0f, .0f));

			{
				_fbo->bindFBO();
				glClearColor(rendParams->_backgroundColor.x, rendParams->_backgroundColor.y, rendParams->_backgroundColor.z, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glViewport(0, 0, CUBE_MAP_SIZE, CUBE_MAP_SIZE);

				_scene->render4Reflection(_camera, mModel, rendParams);
			}

			{
				Image* image = _fbo->getImage();
				image->flipImageVertically();
				images.push_back(image);
				//_fbo->saveImage("Prueba_Refl" + std::to_string(i++) + ".png");
			}
		}

		// Go back to initial state
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, canvasSize.x, canvasSize.y);

		_environmentMapping = new Texture(images, CUBE_MAP_SIZE, CUBE_MAP_SIZE);

		for (Image* image : images)
		{
			delete image;
		}
	}

	_capturedEnvironment = _render = true;
}

void ReflectiveObject::drawAsLines(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	this->renderLines(shader, shaderType, matrix, _modelComp[0], GL_LINES);
}

void ReflectiveObject::drawAsPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	this->renderPoints(shader, shaderType, matrix, _modelComp[0], GL_POINTS);
}

void ReflectiveObject::drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
}

void ReflectiveObject::drawAsReflectiveTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, Camera* camera, const GLuint lightIndex)
{
	if (_render)
	{
		shader->setUniform("cameraPosition", camera->getEye());
		shader->setUniform("refractiveIndex", REFRACTIVE_INDEX[_refractiveMaterial]);
		shader->setUniform("baseColorWeight", _baseColorWeight);
		shader->setUniform("reflectiveWeight", _reflectionWeight);
		shader->setSubroutineUniform(GL_FRAGMENT_SHADER, "mixingUniform", (lightIndex > 0) ? "baseColor" : "mixColor");
		_environmentMapping->applyCubeMap(shader);

		this->renderTriangles(shader, shaderType, matrix, _modelComp[0], GL_TRIANGLES);
	}
}

void ReflectiveObject::drawAsTriangles4Fog(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
}

void ReflectiveObject::drawAsTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	this->renderTriangles4Shadows(shader, shaderType, matrix, _modelComp[0], GL_TRIANGLES);
}

void ReflectiveObject::drawCapturedPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	if (_render)
	{
		this->renderCapturedPoints(shader, shaderType, matrix, _modelComp[0], GL_POINTS);
	}
}

void ReflectiveObject::drawReflectiveCapturedPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, Camera* camera, const GLuint lightIndex)
{
	shader->setUniform("cameraPosition", camera->getEye());
	shader->setUniform("refractiveIndex", REFRACTIVE_INDEX[_refractiveMaterial]);
	shader->setUniform("baseColorWeight", _baseColorWeight);
	shader->setUniform("reflectiveWeight", _reflectionWeight);
	shader->setSubroutineUniform(GL_FRAGMENT_SHADER, "mixingUniform", (lightIndex > 0) ? "baseColor" : "mixColor");
	_environmentMapping->applyCubeMap(shader);

	this->renderCapturedPoints(shader, shaderType, matrix, _modelComp[0], GL_POINTS);
}

void ReflectiveObject::drawCapturedPointsWithGroups(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, bool ASPRS)
{
	this->renderCapturedPointsWithGroup(shader, shaderType, matrix, _modelComp[0], GL_POINTS, ASPRS);
}

bool ReflectiveObject::load(const mat4& modelMatrix)
{
	if (!_loaded && _copyModelComp)
	{
		_modelComp[0]->_geometry = std::move(_copyModelComp->_geometry);
		_modelComp[0]->_topology = std::move(_copyModelComp->_topology);
		_modelComp[0]->_triangleMesh = std::move(_copyModelComp->_triangleMesh);
		_modelComp[0]->_wireframe = std::move(_copyModelComp->_wireframe);
		_modelComp[0]->_topologyIndicesLength[RendEnum::IBO_WIREFRAME] = _modelComp[0]->_wireframe.size();
		_modelComp[0]->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH] = _modelComp[0]->_triangleMesh.size();

		this->generateGeometryTopology(modelMatrix);

		_modelComp[0]->_vao = new VAO(true);
		_modelComp[0]->_vao->setVBOData(_modelComp[0]->_geometry);
		_modelComp[0]->_vao->setIBOData(RendEnum::IBO_WIREFRAME, _modelComp[0]->_wireframe);
		_modelComp[0]->_vao->setIBOData(RendEnum::IBO_TRIANGLE_MESH, _modelComp[0]->_triangleMesh);

		return _loaded = true;
	}
	else if (!_loaded && _copyModel)
	{
		_copyModel->load(modelMatrix);

		ModelComponent* modelComp = _copyModel->getModelComponent(0);

		_modelComp[0]->_geometry = std::move(modelComp->_geometry);
		_modelComp[0]->_topology = std::move(modelComp->_topology);
		_modelComp[0]->_triangleMesh = std::move(modelComp->_triangleMesh);
		_modelComp[0]->_wireframe = std::move(modelComp->_wireframe);
		_modelComp[0]->_topologyIndicesLength[RendEnum::IBO_WIREFRAME] = _modelComp[0]->_wireframe.size();
		_modelComp[0]->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH] = _modelComp[0]->_triangleMesh.size();

		_modelComp[0]->_vao = new VAO(true);
		_modelComp[0]->_vao->setVBOData(_modelComp[0]->_geometry);
		_modelComp[0]->_vao->setIBOData(RendEnum::IBO_WIREFRAME, _modelComp[0]->_wireframe);
		_modelComp[0]->_vao->setIBOData(RendEnum::IBO_TRIANGLE_MESH, _modelComp[0]->_triangleMesh);

		return _loaded = true;
	}

	return false;
}

/// Protected methods

void ReflectiveObject::generateGeometryTopology(const mat4& modelMatrix)
{
	ModelComponent* modelComp = _modelComp[0];

	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::MODEL_APPLY_MODEL_MATRIX);
	const int numVertices = modelComp->_geometry.size(), numFaces = modelComp->_topology.size();
	const int numGroups_v = ComputeShader::getNumGroups(numVertices), numGroups_f = ComputeShader::getNumGroups(numFaces);

	GLuint modelBufferID, faceBufferID;
	modelBufferID = ComputeShader::setReadBuffer(modelComp->_geometry);
	faceBufferID = ComputeShader::setReadBuffer(modelComp->_topology);

	// Apply model matrix to geometry
	shader->bindBuffers(std::vector<GLuint> { modelBufferID });
	shader->use();
	shader->setUniform("mModel", modelMatrix * _modelMatrix);
	shader->setUniform("size", numVertices);
	if (modelComp->_material) modelComp->_material->applyMaterial4ComputeShader(shader);
	shader->execute(numGroups_v, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	// Compute AABB and normals for faces
	shader = ShaderList::getInstance()->getComputeShader(RendEnum::COMPUTE_FACE_AABB);

	shader->bindBuffers(std::vector<GLuint> { modelBufferID, faceBufferID });
	shader->use();
	shader->setUniform("numFaces", numFaces);
	shader->execute(numGroups_f, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	VertexGPUData* vData = shader->readData(modelBufferID, VertexGPUData());
	modelComp->_geometry = std::move(std::vector<VertexGPUData>(vData, vData + numVertices));

	FaceGPUData* fData = shader->readData(faceBufferID, FaceGPUData());
	modelComp->_topology = std::move(std::vector<FaceGPUData>(fData, fData + numFaces));

	glDeleteBuffers(1, &modelBufferID);
}