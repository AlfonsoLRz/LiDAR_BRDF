#pragma once

#include "Geometry/3D/TriangleMesh.h"
#include "Graphics/Application/GraphicsAppEnumerations.h"
#include "Graphics/Application/LiDARParameters.h"
#include "Graphics/Core/Camera.h"
#include "Graphics/Core/ColorUtilities.h"
#include "Graphics/Core/GraphicsCoreEnumerations.h"
#include "Graphics/Core/Material.h"
#include "Graphics/Core/MaterialDatabase.h"
#include "Graphics/Core/ShaderProgram.h"
#include "objloader/OBJ_Loader.h"

class FBOScreenshot;
class Group3D;
class TriangleMesh;
class VAO;

/**
*	@file Model3D.h
*	@authors Alfonso L�pez Ruiz (alr00048@red.ujaen.es)
*	@date 07/11/2019
*/

/**
*	@brief Base class for any drawable 3D model.
*/
class Model3D
{
public:
	class ModelComponent;

public:
	/**
	*	@brief Struct which wraps all that information for a mesh vertex.
	*/
	struct VertexGPUData
	{
		vec3		_position;
		float		_ks;

		vec3		_normal;
		float		_shininess;

		vec2		_textCoord;
		vec2		_padding3;

		vec3		_tangent;
		float		_padding4;

		vec4		_kad;
	};

	/**
	*	@brief Struct which wraps a mesh data.
	*/
	struct FaceGPUData
	{
		uvec3		_vertices;
		unsigned	_modelCompID;							//!< ID of model component where the face belongs to

		vec3		_minPoint;								//!< Bounding box corner
		float		_padding1;

		vec3		_maxPoint;	
		float		_padding2;

		vec3		_normal;								//!< Accelerates LiDAR intersections 
		float		_padding3;
	};

	struct MeshGPUData
	{
		unsigned	_numVertices;
		unsigned	_startIndex;
		float		_opacity;
		float		_shininess;

		vec2		_reflectance;
		unsigned	_surfaceType;
		unsigned	_materialID;
	};

	struct BVHCluster
	{
		vec3		_minPoint;
		unsigned	_prevIndex1;

		vec3		_maxPoint;
		unsigned	_prevIndex2;

		unsigned	_faceIndex;
		vec3		_padding;

		mat4 getScaleMatrix()
		{
			vec3 size = _maxPoint - _minPoint;

			return glm::scale(mat4(1.0f), size);
		}

		mat4 getTranslationMatrix()
		{
			vec3 center = (_maxPoint + _minPoint) / 2.0f;

			return glm::translate(mat4(1.0f), center);
		}
	};

	struct RayGPUData
	{
		vec3		_origin;
		float		_gpsTime;

		vec3		_destination;
		float		_power;

		vec3		_direction;
		unsigned	_returnNumber;

		vec3		_startingPoint;
		unsigned	_lastCollisionIndex;

		vec3		_previousDirection;
		unsigned	_continueRay;

		/**
		*	@brief Default constructor. 
		*/
		RayGPUData() :
			_origin(.0f), _destination(.0f), _direction(.0f), _previousDirection(.0f), _startingPoint(.0f),
			_power(1.0f), _returnNumber(0), _lastCollisionIndex(UINT_MAX), _continueRay(0), _gpsTime(.0f) {}

		/**
		*	@brief Base constructor for any ray.
		*/
		RayGPUData(const vec3& orig, const vec3& dest) : RayGPUData()
		{
			_origin = orig;
			_destination = dest;
			_direction = glm::normalize(dest - orig);
			_startingPoint = orig;
		}

		/**
		*	@return Point for ray in a certain parametric value.
		*/
		vec3 getPoint(float t) {
			return _origin + _direction * t;
		}
	};

	struct TriangleCollisionGPUData
	{
		vec3		_point;
		int			_faceIndex;

		vec3		_normal;
		float		_distance;				//!< Depends on distance from sensor eye to intersection point

		vec2		_textCoord;
		unsigned	_modelCompID;			//!< ID of model component where the face belongs to	
		unsigned	_returnNumber;

		vec3		_rayDirection;
		unsigned	_rayIndex;

		unsigned	_numReturns;
		float		_angle;
		float		_intensity;
		unsigned	_previousCollision;

		vec2		_padding1;
		unsigned	_numIntersectedRays;
		float		_gpsTime;
	};

	struct ModelComponentDescription
	{
		char		_modelName[60];

		// Material
		char		_materialName[60];
		vec3		_kd, _ks;
		float		_ns;
		char		_mapKd[200], _mapKs[200], _mapShininess[200];

		/**
		*	@brief Default constructor for the initialization of a model component.
		*/
		ModelComponentDescription() 
		{
			std::string emptyString = "";

			_kd = vec3(.0f);
			_ks = vec3(.0f);
			_ns = .0f;

			strcpy(_modelName, emptyString.c_str());
			strcpy(_materialName, emptyString.c_str());
			strcpy(_mapKd, emptyString.c_str());
			strcpy(_mapKs, emptyString.c_str());
			strcpy(_mapShininess, emptyString.c_str());
		}

		/**
		*	@brief Construction of description from the loading process.
		*/
		ModelComponentDescription(objl::Mesh* mesh)
		{
			if (!mesh->MeshName.empty())
			{
				strcpy(_modelName, mesh->MeshName.c_str());
			}
			else if (!mesh->MeshMaterial.name.empty())
			{
				strcpy(_modelName, mesh->MeshMaterial.name.c_str());
			}

			objl::Material* material = &mesh->MeshMaterial;

			_kd = vec3(material->Kd.X, material->Kd.Y, material->Kd.Z);
			_ks = vec3(material->Ks.X, material->Ks.Y, material->Ks.Z);
			_ns = 1.0f - material->Ns / 1000.0f;

			strcpy(_materialName, material->name.c_str());
			strcpy(_mapKd, material->map_Kd.c_str());
			strcpy(_mapKs, material->map_Ks.c_str());
			strcpy(_mapShininess, material->map_Ns.c_str());
		}

		/**
		*	@brief Assignment operator overriding.
		*/
		ModelComponentDescription& operator=(const ModelComponentDescription& description)
		{
			_kd = description._kd;
			_ks = description._ks;
			_ns = description._ns;

			strcpy(_modelName, description._modelName);
			strcpy(_materialName, description._materialName);
			strcpy(_mapKd, description._mapKd);
			strcpy(_mapKs, description._mapKs);

			return *this;
		}
	};

public:
	// [Rendering]
	const static GLuint		RESTART_PRIMITIVE_INDEX;		//!< Index which marks the end of a primitive

protected:
	static std::unordered_map<unsigned, vec3>			_asprsGroupColor;
	static std::unordered_map<unsigned, vec3>			_groupColor;
	static std::unordered_map<std::string, unsigned>	_groupId;
	static std::unordered_map<unsigned, std::string>	_groupName;

	// Textures which are initialized just once
	static GLuint					_ssaoKernelTextureID;
	static GLuint					_ssaoNoiseTextureID;
	static GLuint					_shadowTextureID;

protected:
	bool							_loaded;				//!< Mark to know if model has already been loaded

	// [Geometry]
	std::vector<ModelComponent*>	_modelComp;				//!< One for each part of the mode, i.e. a revolution object with two bases and a body

	// [Matrices]
	glm::mat4						_modelMatrix;			//!< World transformation

public:
	/**
	*	@brief Prints the identifiers of the previous defined custom classes.
	*/
	static void printCustomClassesIDs();

protected:
	/**
	*	@brief Renders and save an image with the help of a framebuffer.
	*/
	static void captureTexture(FBOScreenshot* fbo, const std::vector<vec4>& pixels, const uvec2& dimension, const std::string& filename);

	/**
	*	@brief Computes the tangents for a model component with an already loaded geometry and topology.
	*/
	virtual void computeTangents(ModelComponent* modelComp);

	/**
	*	@brief
	*/
	virtual void generatePointCloud();

	/**
	*	@brief
	*/
	virtual void generateWireframe();

	/**
	*	@brief Builds a hash table which links every ASPRS class with a color.
	*/
	static std::unordered_map<unsigned, vec3> getASPRSCodeColor();

	/**
	*	@brief Sets the uniform variables for each shader.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	void setShaderUniforms(ShaderProgram* shader, const RendEnum::RendShaderTypes shaderType, const std::vector<mat4>& matrix);

	/**
	*	@brief Creates the VAO with geometry & topology.
	*/
	virtual void setVAOData();

	/**
	*	@brief Updates the VAO with geometry & topology.
	*/
	virtual void updateVAOData();

protected:
	/// Rendering methods

	/**
	*	@brief Renders a component as a set of lines.
	*	@param modelComp Component where the VAO is located.
	*	@param primitive Primitive we need to use to render the VAO.
	*/
	virtual void renderLines(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive);

	/**
	*	@brief Renders a component as a set of lines.
	*	@param modelComp Component where the VAO is located.
	*	@param primitive Primitive we need to use to render the VAO.
	*/
	virtual void renderPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive);

	/**
	*	@brief Renders a component as a set of triangles.
	*	@param modelComp Component where the VAO is located.
	*	@param primitive Primitive we need to use to render the VAO.
	*/
	virtual void renderTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive);

	/**
	*	@brief Renders LiDAR result as a set of points.
	*	@param modelComp Component where the VAO is located.
	*	@param primitive Primitive we need to use to render the VAO.
	*/
	virtual void renderTrianglesWithGroup(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive, bool ASPRS);

	/**
	*	@brief Renders a component as a set of lines.
	*	@param modelComp Component where the VAO is located.
	*	@param primitive Primitive we need to use to render the VAO.
	*/
	virtual void renderTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive);

	/**
	*	@brief Renders LiDAR result as a set of points.
	*	@param modelComp Component where the VAO is located.
	*	@param primitive Primitive we need to use to render the VAO.
	*/
	virtual void renderCapturedPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive);

	/**
	*	@brief Renders LiDAR result as a set of points.
	*	@param modelComp Component where the VAO is located.
	*	@param primitive Primitive we need to use to render the VAO.
	*/
	virtual void renderCapturedPointsRGB(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive);

	/**
	*	@brief Renders LiDAR result as a set of points.
	*	@param modelComp Component where the VAO is located.
	*	@param primitive Primitive we need to use to render the VAO.
	*/
	virtual void renderCapturedPointsWithGroup(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, ModelComponent* modelComp, const GLuint primitive, bool ASPRS);

public:
	// ------------- Static methods ----------------

	/**
	*	@brief Binds the ambient occlusion textures into the rendering shader.
	*/
	static void bindSSAOTextures(RenderingShader* shader);

	/**
	*	@brief Builds a texture with random samples for the ambient occlusion technique.
	*/
	static void buildSSAONoiseKernels();

	/**
	*	@brief Builds the shadow offset map for an alternative shadowing technique.
	*/
	static void buildShadowOffsetTexture();

	/**
	*	@return A map in dynamic memory which links ASPRS classes and their colors.
	*/
	static std::map<std::string, vec3>* getASPRSClasses();

	// ------------------------------------------------

	/**
	*	@brief Model 3D constructor.
	*	@param modelMatrix Transformation of model in world.
	*	@param numComponents Number of component which compose the model.
	*/
	Model3D(const mat4& modelMatrix = mat4(1.0f), unsigned numComponents = 1);

	/**
	*	@brief Copy constructor.
	*/
	Model3D(const Model3D& model) = delete;

	/**
	*	@brief Destructor.
	*/
	virtual ~Model3D();

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
	*	@brief Renders the model as a set of triangles whose color depends on their semantic group.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTrianglesWithGroup(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, bool ASPRS = true);

	/**
	*	@brief Renders the model as a set of triangles with reflection.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsReflectiveTriangles(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, Camera* camera, const GLuint lightIndex = 0);

	/**
	*	@brief Renders the model as a set of triangles with no textures (except disp. mapping) as we only want to retrieve the depth.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawAsTriangles4Shadows(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the points captured by a sensor coloured by their component ID.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawCapturedPointsInstance(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the points captured by a camera coupled to LiDAR.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawCapturedPointsRGB(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix);

	/**
	*	@brief Renders the points captured by a sensor. Colors depend on the group they belong to.
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawCapturedPointsWithGroups(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, bool ASPRS = true);

	/**
	*	@brief Renders the points captured by a sensor (coloured).
	*	@param shader Shader which will draw the model.
	*	@param shaderType Index of shader at the list, so we can identify what type of uniform variables must be declared.
	*	@param matrix Vector of matrices which can be applied in the rendering process.
	*/
	virtual void drawReflectiveCapturedPoints(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix, Camera* camera, const GLuint lightIndex = 0);

	/**
	*	@brief Computes the model data and sends it to GPU.
	*	@param modelMatrix Model matrix to be applied while generating geometry.
	*	@return Success of operation.
	*/
	virtual bool load(const mat4& modelMatrix = mat4(1.0f)) = 0;

	/**
	*	@brief Assignment operator overriding.
	*/
	Model3D& operator=(const Model3D& model) = delete;

	/**
	*	@brief Retrieve diffuse and specular colors from textures in compute shader.
	*/
	virtual void retrieveColorsGPU();

	/**
	*	@brief Assings identifier for each model component.
	*/
	virtual void registerModelComponentGroup(Group3D* group);

	/**
	*	@brief Saves custom classes in the results folder.
	*/
	static void saveCustomClasses();

	// ------------- Getters ----------------

	/**
	*	@brief Retrieves the id of the ASPRS semantic group linked to a model component identified by 'modelCompID'.
	*/
	virtual void getASPRSSemanticGroup(unsigned modelCompID, int& lastSemanticGroup, bool& found);

	/**
	*	@return A map in dynamic memory which links custom classes and their colors.
	*/
	static std::map<std::string, vec3>* getCustomClasses();

	/**
	*	@return Name of class linked to a model comp identifier, if exists. Otherwise, 'Unlabeled' is returned.
	*/
	static std::string getCustomGroupName(Group3D* group, int modelCompID);

	/**
	*	@return Model component located at the specified index.
	*/
	ModelComponent* getModelComponent(unsigned index) { return _modelComp[index]; }

	/**
	*	@return Model transformation matrix.
	*/
	mat4 getModelMatrix() { return _modelMatrix; }

	/**
	*	@brief Retrieves the id of the semantic group linked to a model component identified by 'modelCompID'.
	*/
	virtual void getSemanticGroup(unsigned modelCompID, int& lastSemanticGroup, bool& found);

	// -------------- Setters ------------------

	/**
	*	@brief Modifies the name which identifies the model component at compIndex.
	*/
	void setName(const std::string& name);

	/**
	*	@brief Establish the material as the color source for triangles.
	*	@param slot Component for which the material should be assigned.
	*/
	void setMaterial(Material* material, unsigned slot = 0);

	/**
	*	@brief Establish the material as the color source for triangles.
	*/
	void setMaterial(std::vector<Material*> material);

	/**
	*	@brief Establish the material which interacts with LiDAR sensor.
	*	@param slot Component for which the material should be assigned.
	*/
	void setMaterial(MaterialDatabase::LiDARMaterialList material, unsigned slot);

	/**
	*	@brief Establish the material which interacts with LiDAR sensor.
	*/
	void setMaterial(MaterialDatabase::LiDARMaterialList material);

	/**
	*	@brief Assigns a new matrix for model transformation.
	*/
	void setModelMatrix(const mat4& modelMatrix) { _modelMatrix = modelMatrix; }

	/**
	*	@brief Gives the ASPRS group name where the object belongs to.
	*/
	void setSemanticGroup(const LiDARParameters::ASPRSClass groupName);

	/**
	*	@brief Gives the ASPRS group name where the object belongs to.
	*/
	void setSemanticGroup(const LiDARParameters::ASPRSClass groupName, unsigned modelCompIndex);

	/**
	*	@brief Gives the group name where the object belongs to.
	*/
	void setSemanticGroup(const std::string& groupName);

	/**
	*	@brief Gives the group name where the object belongs to.
	*/
	void setSemanticGroup(const std::string& groupName, unsigned modelCompIndex);
};

class Model3D::ModelComponent
{
public:
	// [Access variables]	
	Model3D*					_root;										//!<
	unsigned					_id;										//!<

	// [Topology]
	std::vector<GLuint>			_pointCloud;								//!<
	std::vector<GLuint>			_triangleMesh;								//!<
	std::vector<GLuint>			_wireframe;									//!<
	std::vector<unsigned>		_topologyIndicesLength;						//!<

	// [LiDAR geometry]
	std::vector<vec4>			_lidarPointPosition;						//!<
	std::vector<vec3>			_lidarPointNormal;							//!<
	std::vector<vec2>			_lidarPointTextureCoordinates;				//!<

	// [GPU Data]
	std::vector<VertexGPUData>	_geometry;									//!<
	std::vector<FaceGPUData>	_topology;									//!<
				
	// [GPU storage]
	VAO*						_vao;										//!<
	VAO*						_vaoLiDAR;									//!<
			
	// [Additional info]
	bool						_enabled;
	Material*					_material;									//!< As many vector as material types for different shaders
	unsigned					_materialID;								//!< LiDAR material for simulation reflections
	ModelComponentDescription	_modelDescription;							//!<
	std::string					_name;										//!< Display name for GUI windows
	int							_semanticGroup, _asprsSemanticGroup;		//!< Identifier of group where the object belongs to

protected:
	/**
	*	@return Color which identifies the group where the object belongs to.
	*/
	vec3 getGroupColor();

public:
	/**
	*	@brief Default constructor.
	*	@param root Model where this component is located.
	*/
	ModelComponent(Model3D* root);
	
	/**
	*	@brief Deleted copy constructor.
	*/
	ModelComponent(const ModelComponent& modelComp) = delete;

	/**
	*	@brief Destructor.
	*/
	~ModelComponent();

	/**
	*	@brief Assigns the identifier to each face of loaded topology.
	*/
	void assignModelCompIDFaces();

	/**
	*	@brief Builds an array with those indices which compose the point cloud topology.
	*/
	virtual void buildPointCloudTopology();

	/**
	*	@brief Builds an array with those indices which compose the wireframe topology.
	*/
	virtual void buildWireframeTopology();

	/**
	*	@brief Empties those structures related to LiDAR sensor.
	*/
	void clearLiDARPoints();

	/**
	*	@brief Loads those points captured by a LiDAR sensor into a VAO buffer.
	*/
	void loadLiDARPoints();

	/**
	*	@brief Assignment operator overriding.
	*/
	ModelComponent& operator=(const ModelComponent& orig) = delete;

	/**
	*	@brief Adds a new point captured by a sensor as well as its associated geometry for rendering.
	*/
	void pushBackCapturedPoints(TriangleCollisionGPUData* collision);

	/**
	*	@brief Clear geometry and topology arrays to free memory linked to process.
	*/
	virtual void releaseMemory();

	// --------------- Setters -------------------

	/**
	*	@brief Modifies the semantic group of the component from the ASPRS standard.
	*/
	void setASPRSSemanticGroup(const LiDARParameters::ASPRSClass groupName) { _asprsSemanticGroup = groupName; }

	/**
	*	@brief Modifies the semantic group of the component. This semantic concept is customizable.
	*/
	void setCustomSemanticGroup(const std::string groupName);

	/**
	*	@brief Modifies the identifier of the material to be used during simulation. 
	*/
	void setMaterial(const unsigned materialID);

	/**
	*	@brief Modifies the identifier of the material to be used during simulation.
	*/
	void setMaterial(const MaterialDatabase::LiDARMaterialList material);

	/**
	*	@brief Assigns a name to the model component, so it can be identified in the GUI.
	*/
	void setName(const std::string& name) { _name = name; }
};