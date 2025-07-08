#pragma once

#include "Graphics/Application/Scene.h"
#include "Graphics/Core/CADModel.h"
#include "Graphics/Core/DrawLiDARPointCloud.h"
#include "Graphics/Core/DrawAABB.h"
#include "Graphics/Core/DrawPath.h"
#include "Graphics/Core/DrawRay3D.h"
#include "Graphics/Core/LiDARSimulation.h"
#include "Graphics/Core/Model3D.h"
#include "Graphics/Core/ShaderList.h"

/**
*	@file LiDARScene.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 07/12/2019
*/

/**
*	@brief Generic scene for LiDAR simulation.
*/
class LiDARScene: public Scene
{
protected:
	// [Simulation]
	bool						_releaseMemory;				//!< Release memory after BVH initialization
	LiDARSimulation*			_LiDAR;						//!< Sensor wrapping

	// [Models]
	DrawLiDARPointCloud*		_drawPointCloud;			//!< Wrapping of point cloud renderer (uniform rendering)
	VAO*						_LiDARBeamVAO;				//!< 
	LiDARPointCloud*			_pointCloud;				//!< 
	std::vector<DrawRay3D*>		_ray;						//!< Emitted rays

	CADModel*					_aerialLiDAR;				//!< Aerial sensor model
	CADModel*					_terrestrialLiDAR;			//!< Terrestrial sensor model

	// [GUI Support]
	Texture*					_aerialViewTexture;			//!< Aerial rendering of scene
	DrawPath*					_lidarPath;					//!< Path drawn by the user

protected:
	/**
	*	@brief Renders an aerial view of the scene.
	*/
	void acquireAerialView();
	
	/**
	*	@brief Computes the new model matrix to render the LiDAR model, either it is aerial or terrestrial.
	*/
	void computeLiDARModelMatrix(std::vector<mat4>* matrix, LiDARParameters* LiDARParams);

	/**
	*	@brief Loads objects non-dependant of group construction. 
	*/
	void loadModelsCore(Group3D::StaticGPUData* staticGPUData);

	/**
	*	@brief Loads the models necessary to render the scene.
	*/
	virtual void loadModels();

	/**
	*	@return False if any point cloud or wireframe scene must be rendered.
	*/
	bool needToApplyAmbientOcclusion(RenderingParameters* rendParams);

	/**
	*	@brief Renders the LiDAR model which represents the position of such sensor.
	*/
	void renderLiDARModel(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams, const bool shadows = false);

	/**
	*	@brief Renders other data structures related to a LiDAR scene.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	void renderOtherStructures(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief This method gathers all the point cloud renderings.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	void renderPointsClouds(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders the scene without any post-processing efect.
	*/
	void renderScene(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders the scenario as a point cloud.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	void renderPointCloud(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders the scenario as a wireframe mesh.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	void renderWireframe(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders the triangle mesh scenario, whether it is rendered with real colors or semantic concepts.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	void renderTriangleMesh(const mat4& mModel, RenderingParameters* rendParams);


	// -------------- Point clouds -----------------

	/**
	*	@brief
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void renderGPSTimePointCloud(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders all those points resulted from scene geometry and rays intersections. Color depends on point height.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void renderHeightPointCloud(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders points from LiDAR scanning, coloured using their model component.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void renderInstancePointCloud(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders all those points resulted from scene geometry and rays intersections. Color depends on intensity captured by LiDAR.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void renderIntensityPointCloud(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief 
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void renderNormalPointCloud(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders all those points resulted from scene geometry and rays intersections. Color depends on point return number.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void renderReturnNumberPointCloud(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders all those points resulted from scene geometry and rays intersections. Color depends on RGB shading.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void renderRGBPointCloud(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void renderScanAngleRankPointCloud(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void renderScanDirectionPointCloud(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders all those points resulted from scene geometry and rays intersections. Color depends on the group the object belongs to.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void renderSemanticPointCloud(const mat4& mModel, RenderingParameters* rendParams, bool ASPRS);

	/**
	*	@brief Renders all those points resulted from scene geometry and rays intersections. Uniform color and faster rendering.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void renderUniformPointCloud(const mat4& mModel, RenderingParameters* rendParams);

	// --------------- Other secondary structures ----------------------
	
	/**
	*	@brief Renders BVH tree built by room group.
	*/
	void renderBVH(const mat4& model, RenderingParameters* rendParams);

	/**
	*	@brief Shows a LiDAR beam as a cone to show its divergence. 
	*/
	void renderLiDARBeam(const mat4& model, RenderingParameters* rendParams);

	/**
	*	@brief Shows the maximum range of a LiDAR sensor. 
	*/
	void renderLiDARMaximumRange(const mat4& model, RenderingParameters* rendParams);

	/**
	*	@brief Renders the LiDAR path which should follow the drone.
	*/
	virtual void renderLiDARPath(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Renders the ray which are produced from LiDAR simulation.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	void renderRays(const mat4& mModel, RenderingParameters* rendParams);

	// -------------- Draw scene -------------

	/**
	*	@brief Decides which objects are going to be rendered as RGB points from LiDAR simulation.
	*	@param shader Rendering shader which is drawing the scene.
	*	@param shaderType Unique ID of "shader".
	*	@param matrix Vector of matrices, including view, projection, etc.
	*	@param rendParams Parameters which indicates how the scene is rendered.
	*/
	virtual void drawSceneAsRGBCapturedPoints(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams);

	/**
	*	@brief Decides which objects are going to be rendered as a triangle mesh.
	*	@param shader Rendering shader which is drawing the scene.
	*	@param shaderType Unique ID of "shader".
	*	@param matrix Vector of matrices, including view, projection, etc.
	*	@param rendParams Parameters which indicates how the scene is rendered.
	*/
	virtual void drawSceneAsTriangles(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams);

	/**
	*	@brief Decides which objects are going to be rendered as a triangle mesh. Only the normal is calculated for each fragment.
	*	@param shader Rendering shader which is drawing the scene.
	*	@param shaderType Unique ID of "shader".
	*	@param matrix Vector of matrices, including view, projection, etc.
	*	@param rendParams Parameters which indicates how the scene is rendered.
	*/
	virtual void drawSceneAsTriangles4Normal(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams);

	/**
	*	@brief Decides which objects are going to be rendered as a triangle mesh. Only the position is calculated for each fragment.
	*	@param shader Rendering shader which is drawing the scene.
	*	@param shaderType Unique ID of "shader".
	*	@param matrix Vector of matrices, including view, projection, etc.
	*	@param rendParams Parameters which indicates how the scene is rendered.
	*/
	virtual void drawSceneAsTriangles4Position(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams);

public:
	/**
	*	@brief Default constructor.
	*/
	LiDARScene();

	/**
	*	@brief Destructor. Frees memory allocated for 3d models.
	*/
	virtual ~LiDARScene();

	/**
	*	@brief Clears current simulation state, if any.
	*/
	virtual void clearSimulation();

	/**
	*	@brief Loads the elements from the scene: lights, cameras, models, etc.
	*/
	virtual void load();

	/**
	*	@brief Loads all those rendering elements which depends on the simulation process. Therefore, the simulation must be executed before this point.
	*/
	virtual void launchSimulation();

	/**
	*	@brief Draws the scene as the rendering parameters specifies.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void render(const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Draws the scene as the rendering parameters specifies.
	*	@param mModel Additional model matrix to be applied over the initial model matrix.
	*	@param rendParams Rendering parameters to be taken into account.
	*/
	virtual void render4Reflection(Camera* camera, const mat4& mModel, RenderingParameters* rendParams);

	/**
	*	@brief Generates the VAO for the LiDAR path defined by the user.
	*/
	void updatePath();

	// ------------ Getters ---------------

	/**
	*	@return Aerial rendering of current scene.
	*/
	Texture* getAerialView() { return _aerialViewTexture; }
	
	/**
	*	@return Entity which wraps the LIDAR simulation.
	*/
	LiDARSimulation* getLiDARSensor() { return _LiDAR; }

	/**
	*	@return Group which wraps the scene content.
	*/
	virtual Group3D* getRenderingGroup() { return _sceneGroup; }

	/**
	*	@return Axis aligned bounding box which wraps the complete scene.
	*/
	AABB getAABB() { return _sceneGroup->getAABB(); }

	/**
	*	@return Pointer of vector with all registered model components.
	*/
	std::vector<Model3D::ModelComponent*>* getModelComponents() { return _sceneGroup->getRegisteredModelComponents(); }
};

