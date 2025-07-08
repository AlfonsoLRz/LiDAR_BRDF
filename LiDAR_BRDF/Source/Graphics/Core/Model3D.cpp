#include "stdafx.h"
#include "Model3D.h"

#include "Graphics/Application/LiDARParameters.h"
#include "Graphics/Application/Renderer.h"
#include "Graphics/Core/FBOScreenshot.h"
#include "Graphics/Core/Group3D.h"
#include "Graphics/Core/OpenGLUtilities.h"
#include "Graphics/Core/ShaderList.h"
#include "Graphics/Core/VAO.h"
#include "Utilities/ChronoUtilities.h"
#include "Utilities/FileManagement.h"
#include "Interface/Window.h"


// [Static variables initialization]

const GLuint Model3D::RESTART_PRIMITIVE_INDEX = 0xFFFFFFFF;

std::unordered_map<unsigned, vec3> Model3D::_asprsGroupColor = Model3D::getASPRSCodeColor();
std::unordered_map<unsigned, vec3> Model3D::_groupColor;
std::unordered_map<std::string, unsigned> Model3D::_groupId;
std::unordered_map<unsigned, std::string> Model3D::_groupName;

GLuint Model3D::_ssaoKernelTextureID = -1;
GLuint Model3D::_ssaoNoiseTextureID = -1;
GLuint Model3D::_shadowTextureID = -1;


/// [Static methods

void Model3D::bindSSAOTextures(RenderingShader* shader)
{
	shader->setUniform("texKernels", 3);
	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, _ssaoKernelTextureID);

	shader->setUniform("texNoise", 4);
	glActiveTexture(GL_TEXTURE0 + 4);
	glBindTexture(GL_TEXTURE_2D, _ssaoNoiseTextureID);
}

void Model3D::buildSSAONoiseKernels()
{
	// Uniform distribution
	std::uniform_real_distribution<GLfloat> randomFloats(0.0f, 1.0f);
	std::default_random_engine generator;

	// Sample kernels
	std::vector<vec3> ssaoKernel;
	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);

		// Scale samples s.t. they are more aligned to the center of kernel
		float scale = float(i) / 64.0f;
		scale = glm::mix(0.1f, 1.0f, 1.0f - scale * scale);
		sample *= scale;

		ssaoKernel.push_back(sample);
	}

	// Noise texture
	std::vector<glm::vec3> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);	// Rotate around z-axis (in tangent space)
		ssaoNoise.push_back(noise);
	}

	// Compose textures
	glGenTextures(1, &_ssaoKernelTextureID);
	glBindTexture(GL_TEXTURE_2D, _ssaoKernelTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 64, 1, 0, GL_RGB, GL_FLOAT, &ssaoKernel[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glGenTextures(1, &_ssaoNoiseTextureID);
	glBindTexture(GL_TEXTURE_2D, _ssaoNoiseTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void Model3D::buildShadowOffsetTexture()
{
	auto jitter = []() -> float
		{
			return ((float)rand() / RAND_MAX) - 0.5f;
		};

	const int size = 64;
	const int samplesU = 8, samplesV = 8;
	const int samples = samplesU * samplesV;
	const int bufSize = size * size * samples * 2;
	const float twoPI = M_PI * 2.0F;
	float* data = new float[bufSize];

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			for (int k = 0; k < samples; k += 2) {
				int x1, y1, x2, y2;

				x1 = k % (samplesU);
				y1 = (samples - 1 - k) / samplesU;
				x2 = (k + 1) % samplesU;
				y2 = (samples - 1 - k - 1) / samplesU;

				vec4 v;
				// Center on grid and jitter
				v.x = (x1 + 0.5f) + jitter();
				v.y = (y1 + 0.5f) + jitter();
				v.z = (x2 + 0.5f) + jitter();
				v.w = (y2 + 0.5f) + jitter();
				// Scale between 0 and 1
				v.x /= samplesU;
				v.y /= samplesV;
				v.z /= samplesU;
				v.w /= samplesV;

				// Warp to disk
				int cell = ((k / 2) * size * size + j * size + i) * 4;
				data[cell + 0] = sqrtf(v.y) * cosf(twoPI * v.x);
				data[cell + 1] = sqrtf(v.y) * sinf(twoPI * v.x);
				data[cell + 2] = sqrtf(v.w) * cosf(twoPI * v.z);
				data[cell + 3] = sqrtf(v.w) * sinf(twoPI * v.z);
			}
		}
	}

	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &_shadowTextureID);
	glBindTexture(GL_TEXTURE_3D, _shadowTextureID);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, size, size, samples / 2, 0, GL_RGBA, GL_FLOAT, data);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	delete[] data;
}

std::map<std::string, vec3>* Model3D::getASPRSClasses()
{
	std::map<std::string, vec3>* map = new std::map<std::string, vec3>();

	for (int i = 0; i < LiDARParameters::NUM_CLASSES; ++i)
	{
		const vec3 rgb = _asprsGroupColor[i];
		(*map)[LiDARParameters::ASPRSClass_STR[i]] = ColorUtilities::RGBtoHSV(rgb.x, rgb.y, rgb.z);
	}

	return map;
}

/// [Public methods]

Model3D::Model3D(const glm::mat4& modelMatrix, unsigned numComponents) :
	_loaded(false), _modelMatrix(modelMatrix), _modelComp(numComponents)
{
	for (int i = 0; i < numComponents; ++i)
	{
		_modelComp[i] = new ModelComponent(this);
	}
}

Model3D::~Model3D()
{
	for (auto& it : _modelComp)
	{
		delete it;
	}

	glDeleteTextures(1, &_ssaoKernelTextureID);
	glDeleteTextures(1, &_ssaoNoiseTextureID);
	glDeleteTextures(1, &_shadowTextureID);
}

void Model3D::drawAsLines(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	for (ModelComponent* modelComp : _modelComp) this->renderLines(shader, shaderType, matrix, modelComp, GL_LINES);
}

void Model3D::drawAsPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	for (ModelComponent* modelComp : _modelComp) this->renderPoints(shader, shaderType, matrix, modelComp, GL_POINTS);
}

void Model3D::drawAsTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	for (ModelComponent* modelComp : _modelComp) this->renderTriangles(shader, shaderType, matrix, modelComp, GL_TRIANGLES);
}

void Model3D::drawAsTrianglesWithGroup(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, bool ASPRS)
{
	for (ModelComponent* modelComp : _modelComp) this->renderTrianglesWithGroup(shader, shaderType, matrix, modelComp, GL_TRIANGLES, ASPRS);
}

void Model3D::drawAsReflectiveTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, Camera* camera, const GLuint lightIndex)
{
}

void Model3D::drawAsTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	for (ModelComponent* modelComp : _modelComp) this->renderTriangles4Shadows(shader, shaderType, matrix, modelComp, GL_TRIANGLES);
}

void Model3D::drawCapturedPointsInstance(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	for (ModelComponent* modelComp : _modelComp)
	{
		shader->setUniform("vColor", ColorUtilities::HSVtoRGB(ColorUtilities::getHueValue(modelComp->_id), .9f, .9f));

		this->renderCapturedPoints(shader, shaderType, matrix, modelComp, GL_POINTS);
	}
}

void Model3D::drawCapturedPointsRGB(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	for (ModelComponent* modelComp : _modelComp) this->renderCapturedPointsRGB(shader, shaderType, matrix, modelComp, GL_POINTS);
}

void Model3D::drawCapturedPointsWithGroups(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, bool ASPRS)
{
	for (ModelComponent* modelComp : _modelComp) this->renderCapturedPointsWithGroup(shader, shaderType, matrix, modelComp, GL_POINTS, ASPRS);
}

void Model3D::drawReflectiveCapturedPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, Camera* camera, const GLuint lightIndex)
{
}

void Model3D::getASPRSSemanticGroup(unsigned modelCompID, int& lastSemanticGroup, bool& found)
{
	for (ModelComponent* modelComp : _modelComp)
	{
		if (modelComp->_id == modelCompID)
		{
			found = true;

			if (modelComp->_asprsSemanticGroup >= 0)
			{
				lastSemanticGroup = modelComp->_asprsSemanticGroup;
			}

			break;
		}
	}
}

std::map<std::string, vec3>* Model3D::getCustomClasses()
{
	std::map<std::string, vec3>* map = new std::map<std::string, vec3>();

	for (auto pair : _groupName)
	{
		const vec3 rgb = _groupColor[pair.first];
		(*map)[pair.second] = ColorUtilities::RGBtoHSV(rgb.x, rgb.y, rgb.z);
	}

	return map;
}

std::string Model3D::getCustomGroupName(Group3D* group, int modelCompID)
{
	if (modelCompID < 0) return "Unlabeled";

	auto modelComp = group->getModelComponent(modelCompID);
	return _groupName[modelComp->_semanticGroup];
}

void Model3D::getSemanticGroup(unsigned modelCompID, int& lastSemanticGroup, bool& found)
{
	for (ModelComponent* modelComp : _modelComp)
	{
		if (modelComp->_id == modelCompID)
		{
			found = true;

			if (modelComp->_semanticGroup >= 0)
			{
				lastSemanticGroup = modelComp->_semanticGroup;
			}

			break;
		}
	}
}

void Model3D::retrieveColorsGPU()
{
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::RETRIEVE_COLORS);

	for (ModelComponent* modelComponent : _modelComp)
	{
		const unsigned size = modelComponent->_geometry.size();
		const unsigned numGroups = ComputeShader::getNumGroups(size);
		const GLuint geometrySSBO = ComputeShader::setReadBuffer(modelComponent->_geometry, GL_DYNAMIC_DRAW);
		Material* material = modelComponent->_material;

		shader->use();
		shader->bindBuffers(std::vector<GLuint>{geometrySSBO});
		shader->setUniform("size", size);

		if (material)
		{
			material->applyTexture(shader, Texture::KAD_TEXTURE);
			material->applyTexture(shader, Texture::KS_TEXTURE);
			material->applyTexture(shader, Texture::NS_TEXTURE);

			if (material->applyTexture(shader, Texture::SEMI_TRANSPARENT_TEXTURE))
			{
				shader->setSubroutineUniform(GL_COMPUTE_SHADER, "alphaUniform", "alphaColor");
			}
			else
			{
				shader->setSubroutineUniform(GL_COMPUTE_SHADER, "alphaUniform", "noAlphaColor");
			}
		}

		shader->applyActiveSubroutines();
		shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		Model3D::VertexGPUData* pVertices = ComputeShader::readData(geometrySSBO, Model3D::VertexGPUData());
		modelComponent->_geometry = std::vector<Model3D::VertexGPUData>(pVertices, pVertices + size);

		glDeleteBuffers(1, &geometrySSBO);
	}
}

void Model3D::registerModelComponentGroup(Group3D* group)
{
	for (ModelComponent* modelComp : _modelComp)
	{
		group->registerModelComponent(modelComp);
	}
}

void Model3D::saveCustomClasses()
{
	std::ofstream file("Results/custom_classes.txt");
	if (!file.is_open() || !file.good()) return;

	for (auto& pair : _groupName)
	{
		file << std::to_string(pair.first) << "," << pair.second << std::endl;
	}

	file.close();
}

void Model3D::setName(const std::string& name)
{
	if (_modelComp.size() > 1)
	{
		unsigned index = 0;

		for (ModelComponent* modelComp : _modelComp)
		{
			modelComp->setName(name + " (" + std::to_string(++index) + ")");
		}
	}
	else if (!_modelComp.empty())
	{
		_modelComp[0]->setName(name);
	}
}

void Model3D::setMaterial(Material* material, unsigned slot)
{
	if (slot >= _modelComp.size())
	{
		return;
	}

	_modelComp[slot]->_material = material;
}

void Model3D::setMaterial(std::vector<Material*> material)
{
	int index = 0;

	while (index < _modelComp.size() && index < material.size())
	{
		_modelComp[index]->_material = material[index++];
	}
}

void Model3D::setMaterial(MaterialDatabase::LiDARMaterialList material, unsigned slot)
{
	if (slot >= _modelComp.size())
	{
		return;
	}

	_modelComp[slot]->setMaterial(material);
}

void Model3D::setMaterial(MaterialDatabase::LiDARMaterialList material)
{
	for (ModelComponent* modelComp : _modelComp)
	{
		modelComp->setMaterial(material);
	}
}

void Model3D::setSemanticGroup(const LiDARParameters::ASPRSClass groupName)
{
	for (int modelComp = 0; modelComp < _modelComp.size(); ++modelComp)
	{
		this->setSemanticGroup(groupName, modelComp);
	}
}

void Model3D::setSemanticGroup(const LiDARParameters::ASPRSClass groupName, unsigned modelCompIndex)
{
	if (modelCompIndex > _modelComp.size()) return;

	_modelComp[modelCompIndex]->setASPRSSemanticGroup(groupName);
}

void Model3D::setSemanticGroup(const std::string& groupName)
{
	for (int modelComp = 0; modelComp < _modelComp.size(); ++modelComp)
	{
		this->setSemanticGroup(groupName, modelComp);
	}
}

void Model3D::setSemanticGroup(const std::string& groupName, unsigned modelCompIndex)
{
	if (modelCompIndex > _modelComp.size()) return;

	_modelComp[modelCompIndex]->setCustomSemanticGroup(groupName);
}

/// [Protected methods]

void Model3D::captureTexture(FBOScreenshot* fbo, const std::vector<vec4>& pixels, const uvec2& dimension, const std::string& filename)
{
	float* imageData = new float[pixels.size() * 4];
	for (int i = 0; i < pixels.size(); ++i)
	{
		imageData[i * 4] = pixels[i].x;
		imageData[i * 4 + 1] = pixels[i].y;
		imageData[i * 4 + 2] = pixels[i].z;
		imageData[i * 4 + 3] = 1.0f;
	}

	Texture* tex = new Texture(imageData, dimension.x, dimension.y, GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST, true);
	const ivec2 canvasSize = Window::getInstance()->getSize();
	VAO* quadVAO = Primitives::getQuadVAO();

	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo->getIdentifier());
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, fbo->getSize().x, fbo->getSize().y);

		RenderingShader* shader = ShaderList::getInstance()->getRenderingShader(RendEnum::DEBUG_QUAD_SHADER);
		shader->use();
		tex->applyTexture(shader, 0, "texSampler");
		shader->applyActiveSubroutines();

		quadVAO->drawObject(RendEnum::IBO_TRIANGLE_MESH, GL_TRIANGLES, 2 * 4);
	}

	{
		fbo->saveImage(filename);

		// Go back to initial state
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, canvasSize.x, canvasSize.y);
	}

	delete tex;
}

void Model3D::computeTangents(ModelComponent* modelComp)
{
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::COMPUTE_TANGENTS_1);
	const int numVertices = modelComp->_geometry.size(), numTriangles = modelComp->_topology.size();
	int numGroups = ComputeShader::getNumGroups(numTriangles);

	GLuint geometryBufferID, meshBufferID, outBufferID;
	geometryBufferID = ComputeShader::setReadBuffer(modelComp->_geometry);
	meshBufferID = ComputeShader::setReadBuffer(modelComp->_topology);
	outBufferID = ComputeShader::setWriteBuffer(vec4(), numVertices);

	shader->bindBuffers(std::vector<GLuint> { geometryBufferID, meshBufferID, outBufferID });
	shader->use();
	shader->setUniform("numTriangles", numTriangles);
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	shader = ShaderList::getInstance()->getComputeShader(RendEnum::COMPUTE_TANGENTS_2);
	numGroups = ComputeShader::getNumGroups(numVertices);

	shader->bindBuffers(std::vector<GLuint> { geometryBufferID, outBufferID });
	shader->use();
	shader->setUniform("numVertices", numVertices);
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	VertexGPUData* data = ComputeShader::readData(geometryBufferID, VertexGPUData());
	modelComp->_geometry = std::move(std::vector<VertexGPUData>(data, data + numVertices));

	glDeleteBuffers(1, &geometryBufferID);
	glDeleteBuffers(1, &meshBufferID);
	glDeleteBuffers(1, &outBufferID);
}

void Model3D::generatePointCloud()
{
	for (ModelComponent* modelComp : _modelComp)
	{
		modelComp->_pointCloud.resize(modelComp->_geometry.size());
		std::iota(modelComp->_pointCloud.begin(), modelComp->_pointCloud.end(), 0);
	}
}

void Model3D::generateWireframe()
{
	std::unordered_map<int, std::unordered_set<int>> segmentIncluded;
	auto canInsert = [&](int index1, int index2) -> bool
		{
			std::unordered_map<int, std::unordered_set<int>>::iterator it;

			if ((it = segmentIncluded.find(index1)) != segmentIncluded.end())
			{
				if (it->second.find(index2) != it->second.end())
				{
					return false;
				}
			}

			if ((it = segmentIncluded.find(index2)) != segmentIncluded.end())
			{
				if (it->second.find(index1) != it->second.end())
				{
					return false;
				}
			}

			return true;
		};

	for (ModelComponent* modelComp : _modelComp)
	{
		const unsigned triangleMeshSize = modelComp->_triangleMesh.size();

		for (int i = 0; i < triangleMeshSize; i += 3)
		{
			for (int j = i; (j <= (i + 1)) && (j < triangleMeshSize - 1); ++j)
			{
				if (canInsert(modelComp->_triangleMesh[j], modelComp->_triangleMesh[j + 1]))
				{
					modelComp->_wireframe.push_back(modelComp->_triangleMesh[j]);
					modelComp->_wireframe.push_back(modelComp->_triangleMesh[j + 1]);
					modelComp->_wireframe.push_back(Model3D::RESTART_PRIMITIVE_INDEX);
				}
			}
		}
	}
}

std::unordered_map<unsigned, vec3> Model3D::getASPRSCodeColor()
{
	std::unordered_map<unsigned, vec3> table;

	for (int i = 0; i < LiDARParameters::ASPRSClass::NUM_CLASSES; ++i)
	{
		table[i] = ColorUtilities::HSVtoRGB(ColorUtilities::getHueValue(i), .99f, .99f);
	}

	return table;
}

void Model3D::printCustomClassesIDs()
{
	std::cout << "--------------------" << std::endl;

	for (std::pair<unsigned, std::string> classID : _groupName)
	{
		std::cout << classID.first << ": " << classID.second << std::endl;
	}

	std::cout << "--------------------" << std::endl;
}

void Model3D::setShaderUniforms(ShaderProgram* shader, const RendEnum::RendShaderTypes shaderType, const std::vector<mat4>& matrix)
{
	RenderingParameters* rendParams = Renderer::getInstance()->getRenderingParameters();

	switch (shaderType)
	{
	case RendEnum::TRIANGLE_MESH_SHADER:
	case RendEnum::TRIANGLE_MESH_GROUP_SHADER:
	case RendEnum::REFLECTIVE_TRIANGLE_MESH_SHADER:
	case RendEnum::POINT_CLOUD_COLOUR_SHADER:
		shader->setUniform("mModelView", matrix[RendEnum::VIEW_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("mShadow", matrix[RendEnum::BIAS_VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);

		// ---- Alternative shadow technique
		shader->setUniform("texOffset", 10);
		glActiveTexture(GL_TEXTURE0 + 10);
		glBindTexture(GL_TEXTURE_3D, _shadowTextureID);

		break;

	case RendEnum::TERRAIN_SHADER:
	case RendEnum::WATER_LAKE_SHADER:
	case RendEnum::GRASS_SHADER:
	case RendEnum::TREE_TRIANGLE_MESH_SHADER:
		shader->setUniform("mModelView", matrix[RendEnum::VIEW_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("mShadow", matrix[RendEnum::BIAS_VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);

		break;

	case RendEnum::POINT_CLOUD_SHADER:
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("pointSize", rendParams->_lidarPointSize);
		shader->setUniform("vColor", rendParams->_scenePointCloudColor);

		break;

	case RendEnum::INTENSITY_POINT_CLOUD_SHADER:
	case RendEnum::POINT_CLOUD_HEIGHT_SHADER:
	case RendEnum::POINT_CLOUD_NORMAL_SHADER:
	case RendEnum::POINT_CLOUD_SCAN_ANGLE_RANK_SHADER:
	case RendEnum::POINT_CLOUD_SCAN_DIRECTION_SHADER:
	case RendEnum::POINT_CLOUD_RETURN_NUMBER_SHADER:
	case RendEnum::POINT_CLOUD_GPS_TIME_SHADER:
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("pointSize", rendParams->_lidarPointSize);

		break;

	case RendEnum::WIREFRAME_SHADER:
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("vColor", rendParams->_wireframeColor);

		break;

	case RendEnum::BVH_SHADER:
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("vColor", rendParams->_bvhWireframeColor);

		break;

	case RendEnum::TRIANGLE_MESH_NORMAL_SHADER:
	case RendEnum::TRIANGLE_MESH_POSITION_SHADER:
	case RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_NORMAL_SHADER:
	case RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_POSITION_SHADER:
	case RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_GROUP_SHADER:
		shader->setUniform("mModelView", matrix[RendEnum::VIEW_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);

		break;

	case RendEnum::MULTI_INSTANCE_SHADOWS_SHADER:
	case RendEnum::SHADOWS_SHADER:
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);

		break;
	}
}

void Model3D::setVAOData()
{
	for (int i = 0; i < _modelComp.size(); ++i)
	{
		VAO* vao = new VAO(true);
		ModelComponent* modelComp = _modelComp[i];

		modelComp->_topologyIndicesLength[RendEnum::IBO_POINT_CLOUD] = modelComp->_pointCloud.size();
		modelComp->_topologyIndicesLength[RendEnum::IBO_WIREFRAME] = modelComp->_wireframe.size();
		modelComp->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH] = modelComp->_triangleMesh.size();

		vao->setVBOData(modelComp->_geometry);
		vao->setIBOData(RendEnum::IBO_POINT_CLOUD, modelComp->_pointCloud);
		vao->setIBOData(RendEnum::IBO_WIREFRAME, modelComp->_wireframe);
		vao->setIBOData(RendEnum::IBO_TRIANGLE_MESH, modelComp->_triangleMesh);

		modelComp->_vao = vao;
	}
}

void Model3D::updateVAOData()
{
	for (int i = 0; i < _modelComp.size(); ++i)
	{
		ModelComponent* modelComp = _modelComp[i];
		VAO* vao = modelComp->_vao;

		if (vao)
		{
			modelComp->_topologyIndicesLength[RendEnum::IBO_POINT_CLOUD] = modelComp->_pointCloud.size();
			modelComp->_topologyIndicesLength[RendEnum::IBO_WIREFRAME] = modelComp->_wireframe.size();
			modelComp->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH] = modelComp->_triangleMesh.size();

			vao->setVBOData(modelComp->_geometry);
			vao->setIBOData(RendEnum::IBO_POINT_CLOUD, modelComp->_pointCloud);
			vao->setIBOData(RendEnum::IBO_WIREFRAME, modelComp->_wireframe);
			vao->setIBOData(RendEnum::IBO_TRIANGLE_MESH, modelComp->_triangleMesh);
		}
	}
}

/// [Rendering protected methods]

void Model3D::renderLines(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive)
{
	VAO* vao = modelComp->_vao;

	if (vao && modelComp->_enabled)
	{
		this->setShaderUniforms(shader, shaderType, matrix);

		vao->drawObject(RendEnum::IBO_WIREFRAME, primitive, GLuint(modelComp->_topologyIndicesLength[RendEnum::IBO_WIREFRAME]));
	}
}

void Model3D::renderPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive)
{
	VAO* vao = modelComp->_vao;

	if (vao && modelComp->_enabled)
	{
		this->setShaderUniforms(shader, shaderType, matrix);

		vao->drawObject(RendEnum::IBO_POINT_CLOUD, primitive, GLuint(modelComp->_topologyIndicesLength[RendEnum::IBO_POINT_CLOUD]));
	}
}

void Model3D::renderTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive)
{
	VAO* vao = modelComp->_vao;
	Material* material = modelComp->_material;

	if (vao && modelComp->_enabled)
	{
		this->setShaderUniforms(shader, shaderType, matrix);

		if (material)
		{
			material->applyMaterial(shader);
		}

		vao->drawObject(RendEnum::IBO_TRIANGLE_MESH, primitive, GLuint(modelComp->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH]));
	}
}

void Model3D::renderTrianglesWithGroup(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive, bool ASPRS)
{
	if (modelComp->_vao && modelComp->_enabled)
	{
		this->setShaderUniforms(shader, shaderType, matrix);

		if (modelComp->_material)
		{
			modelComp->_material->applyMaterial4UniformColor(shader);
		}

		if (!ASPRS && modelComp->_semanticGroup != -1) shader->setUniform("kadColor", _groupColor[modelComp->_semanticGroup]);
		else if (ASPRS && modelComp->_asprsSemanticGroup != -1) shader->setUniform("kadColor", _asprsGroupColor[modelComp->_asprsSemanticGroup]);

		modelComp->_vao->drawObject(RendEnum::IBO_TRIANGLE_MESH, primitive, GLuint(modelComp->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH]));
	}
}

void Model3D::renderTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive)
{
	VAO* vao = modelComp->_vao;

	if (vao && modelComp->_enabled)
	{
		this->setShaderUniforms(shader, shaderType, matrix);

		vao->drawObject(RendEnum::IBO_TRIANGLE_MESH, primitive, GLuint(modelComp->_topologyIndicesLength[RendEnum::IBO_TRIANGLE_MESH]));
	}
}

void Model3D::renderCapturedPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive)
{
	RenderingParameters* rendParams = Renderer::getInstance()->getRenderingParameters();
	VAO* vao = modelComp->_vaoLiDAR;

	if (vao)
	{
		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("pointSize", rendParams->_lidarPointSize);

		vao->drawObject(RendEnum::IBO_POINT_CLOUD, primitive, GLuint(modelComp->_lidarPointPosition.size()));
	}
}

void Model3D::renderCapturedPointsRGB(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive)
{
	VAO* vao = modelComp->_vaoLiDAR;
	Material* material = modelComp->_material;
	RenderingParameters* rendParams = Renderer::getInstance()->getRenderingParameters();

	if (vao && modelComp->_enabled)
	{
		this->setShaderUniforms(shader, shaderType, matrix);
		shader->setUniform("pointSize", rendParams->_lidarPointSize);

		if (material)
		{
			material->applyMaterial4ColouredPoints(shader);
		}

		vao->drawObject(RendEnum::IBO_POINT_CLOUD, primitive, GLuint(modelComp->_lidarPointPosition.size()));
	}
}

void Model3D::renderCapturedPointsWithGroup(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive, bool ASPRS)
{
	VAO* vao = modelComp->_vaoLiDAR;

	if (vao)
	{
		RenderingParameters* rendParams = Renderer::getInstance()->getRenderingParameters();

		shader->setUniform("mModelViewProj", matrix[RendEnum::VIEW_PROJ_MATRIX] * matrix[RendEnum::MODEL_MATRIX]);
		shader->setUniform("pointSize", rendParams->_lidarPointSize);

		if (!ASPRS && modelComp->_semanticGroup != -1) shader->setUniform("vColor", _groupColor[modelComp->_semanticGroup]);
		else if (ASPRS && modelComp->_asprsSemanticGroup != -1) shader->setUniform("vColor", _asprsGroupColor[modelComp->_asprsSemanticGroup]);

		vao->drawObject(RendEnum::IBO_POINT_CLOUD, primitive, GLuint(modelComp->_lidarPointPosition.size()));
	}
}

/// MODEL COMPONENT SUBCLASS

/// [Public methods]

Model3D::ModelComponent::ModelComponent(Model3D* root) :
	_root(root), _id(-1), _enabled(true), _material(nullptr), _semanticGroup(-1), _asprsSemanticGroup(-1), _materialID(0),
	_topologyIndicesLength(RendEnum::numIBOTypes()), _vao(nullptr), _vaoLiDAR(nullptr)
{
}

Model3D::ModelComponent::~ModelComponent()
{
	delete _vao;
	delete _vaoLiDAR;
}

void Model3D::ModelComponent::assignModelCompIDFaces()
{
	for (FaceGPUData& data : _topology)
	{
		data._modelCompID = _id;
	}
}

void Model3D::ModelComponent::buildPointCloudTopology()
{
	_pointCloud.resize(_geometry.size());
	std::iota(_pointCloud.begin(), _pointCloud.end(), 0);
}

void Model3D::ModelComponent::buildWireframeTopology()
{
	std::unordered_map<int, std::unordered_set<int>> includedEdges;				// Already included edges

	auto isEdgeIncluded = [&](int index1, int index2) -> bool
		{
			std::unordered_map<int, std::unordered_set<int>>::iterator it;

			if ((it = includedEdges.find(index1)) != includedEdges.end())			// p2, p1 and p1, p2 are considered to be the same edge
			{
				if (it->second.find(index2) != it->second.end())					// Already included
				{
					return true;
				}
			}

			if ((it = includedEdges.find(index2)) != includedEdges.end())
			{
				if (it->second.find(index1) != it->second.end())					// Already included
				{
					return true;
				}
			}

			return false;
		};

	for (unsigned int i = 0; i < _triangleMesh.size(); i += 4)
	{
		_wireframe.push_back(_triangleMesh[i]);
		_wireframe.push_back(_triangleMesh[i + 1]);
		_wireframe.push_back(RESTART_PRIMITIVE_INDEX);

		_wireframe.push_back(_triangleMesh[i + 1]);
		_wireframe.push_back(_triangleMesh[i + 2]);
		_wireframe.push_back(RESTART_PRIMITIVE_INDEX);

		_wireframe.push_back(_triangleMesh[i]);
		_wireframe.push_back(_triangleMesh[i + 2]);
		_wireframe.push_back(RESTART_PRIMITIVE_INDEX);
	}
}

void Model3D::ModelComponent::clearLiDARPoints()
{
	_lidarPointPosition.clear();
	_lidarPointTextureCoordinates.clear();
}

void Model3D::ModelComponent::loadLiDARPoints()
{
	if (!_vaoLiDAR)
	{
		_vaoLiDAR = new VAO();
	}

	std::vector<GLuint> pointCloud(_lidarPointPosition.size());
	std::iota(pointCloud.begin(), pointCloud.end(), 0);

	_vaoLiDAR->setVBOData(RendEnum::VBO_POSITION, _lidarPointPosition);
	_vaoLiDAR->setVBOData(RendEnum::VBO_NORMAL, _lidarPointNormal);
	_vaoLiDAR->setVBOData(RendEnum::VBO_TEXT_COORD, _lidarPointTextureCoordinates);
	_vaoLiDAR->setIBOData(RendEnum::IBO_POINT_CLOUD, pointCloud);
}

void Model3D::ModelComponent::pushBackCapturedPoints(TriangleCollisionGPUData* collision)
{
	_lidarPointPosition.push_back(vec4(collision->_point, 1.0f));
	_lidarPointNormal.push_back(collision->_normal);
	_lidarPointTextureCoordinates.push_back(collision->_textCoord);
}

void Model3D::ModelComponent::releaseMemory()
{
	std::vector<VertexGPUData>().swap(_geometry);
	std::vector<FaceGPUData>().swap(_topology);
	std::vector<GLuint>().swap(_pointCloud);
	std::vector<GLuint>().swap(_wireframe);
	std::vector<GLuint>().swap(_triangleMesh);
}

void Model3D::ModelComponent::setCustomSemanticGroup(const std::string groupName)
{
	auto nameIt = Model3D::_groupId.find(groupName);
	int id;

	if (nameIt == Model3D::_groupId.end())
	{
		id = Model3D::_groupId.size();

		Model3D::_groupId[groupName] = id;
		Model3D::_groupName[id] = groupName;
		Model3D::_groupColor[id] = ColorUtilities::HSVtoRGB(ColorUtilities::getHueValue(id), .99f, .99f);
	}
	else
	{
		id = nameIt->second;
	}

	_semanticGroup = id;
}

void Model3D::ModelComponent::setMaterial(const unsigned materialID)
{
	this->_materialID = materialID;
}

void Model3D::ModelComponent::setMaterial(const MaterialDatabase::LiDARMaterialList material)
{
	MaterialDatabase* materialDB = MaterialDatabase::getInstance();
	this->_materialID = materialDB->getMaterialID(material);
}

/// [Protected methods]

vec3 Model3D::ModelComponent::getGroupColor()
{
	if (_semanticGroup == -1) return vec3(.0f);

	return _groupColor[_semanticGroup];
}
