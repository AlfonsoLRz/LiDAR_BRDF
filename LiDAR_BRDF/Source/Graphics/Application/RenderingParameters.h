#pragma once

#include "Graphics/Application/GraphicsAppEnumerations.h"

/**
*	@file RenderingParameters.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 07/20/2019
*/

/**
*	@brief Wraps the rendering conditions of the application.
*/
struct RenderingParameters
{
public:
	enum SemanticRendering : int
	{
		CUSTOM_CONCEPT = 0,
		ASPRS_CONCEPT = 1
	};

	enum PointCloudType : int {
		UNIFORM = 0,
		ASPRS_SEMANTIC = 1,
		CUSTOM_SEMANTIC = 2,
		HEIGHT = 3,
		INTENSITY = 4,
		RETURN_NUMBER = 5,
		INSTANCE = 6,
		NORMAL = 7,
		SCAN_ANGLE_RANK = 8,
		SCAN_DIRECTION = 9,
		GPS_TIME = 10,
		RGB = 11
	};

public:
	// Application
	vec3							_backgroundColor;						//!< Clear color
	ivec2							_viewportSize;							//!< Viewport size (!= window)

	// Lighting
	float							_materialScattering;					//!< Ambient light substitute
	float							_occlusionMinIntensity;					//!< Mininum value for occlusion factor (max is 1 => no occlusion)

	// Screenshot
	char							_screenshotFilenameBuffer[32];			//!< Location of screenshot
	float							_screenshotAlpha;						//!<
	float							_screenshotMultiplier;					//!< Multiplier of current size of GLFW window

	// Rendering type
	int								_visualizationMode;						//!< Only triangle mesh is defined here
	
	// Point cloud	
	float							_lidarPointSize;						//!< Size of points in a cloud
	vec3							_lidarPointCloudColor;					//!< Color of uniform point cloud
	vec3							_scenePointCloudColor;					//!< Color of point cloud which shows all the vertices

	// Wireframe
	vec3							_bvhWireframeColor;						//!< Color of BVH structure 
	vec3							_wireframeColor;						//!< Color of lines in wireframe rendering

	// Triangle mesh
	bool							_ambientOcclusion;						//!< Boolean value to enable/disable occlusion
	bool							_renderSemanticConcept;					//!< Boolean value to indicate if rendering semantic concepts is needed
	int								_semanticRenderingConcept;				//!< ASPRS / Custom semantic concepts (selector)

	// What to see		
	float							_bvhNodesPercentage;					//!< Percentage of BVH nodes to be rendered (lower percentage means smaller nodes will be rendered)
	bool							_grayscaleHeight;						//!< Render point cloud with grayscale color depending on height of points
	bool							_grayscaleIntensity;					//!<
	vec2							_heightBoundaries;						//!<
	float							_intensityAddition;						//!<
	float							_intensityExposure;						//!<
	float							_intensityGamma;						//!<
	bool							_intensityHDR;							//!<
	float							_intensityMultiplier;					//!<
	unsigned						_lastReturnRendered;					//!< Last return to be rendered
	vec3							_lidarBeamNormal;						//!< Normal for displaying a LiDAR beams and its divergence
	float							_lineWidth;								//!<
	int								_numLiDARRBeamSubdivisions;				//!< Number of LiDAR subdivisions while showing a cone
	int								_numLiDARRangeSubdivisions;				//!< Number of subdivisions for showing the maximum range of a LiDAR
	int								_pointCloudType;						//!< ID of the point cloud type which must be rendered
	float							_raysPercentage;						//!< Percentage of rays to be rendered
	float							_returnDivisionThreshold;				//!< Threshold for division numReturn / maxReturn
	bool							_showBVH;								//!< Render BVH data structure
	bool							_showLiDARPath;							//!< Show custom path drawn by the user
	bool							_showLidarBeam;							//!< Display a laser beam to show footprint
	bool							_showLidarMaxRange;						//!< Shows the maximum range of a LiDAR sensor as a circle
	bool							_showLidarModel;						//!< Render a model which simulates the LiDAR
	bool							_showLidarRays;							//!< Render thrown rays
	bool							_showLidarPointCloud;					//!< Allows to render the select type of LiDAR point cloud
	bool							_showTerrainRegularGrid;				//!< Shows a grid with the saturation level of a regular grid
	bool							_showTriangleMesh;						//!< Render original scene

	// Real-time terrain rendering
	float							_snowHeightFactor;						//!<
	float							_snowHeightThreshold;					//!<
	float							_snowSlopeFactor;						//!<

public:
	/**
	*	@brief Default constructor.
	*/
	RenderingParameters() :
		_viewportSize(1.0f, 1.0f),

		_backgroundColor(0.4f, 0.4f, 0.4f),

		_materialScattering(3.0f),

		_screenshotFilenameBuffer("Screenshot.png"),
		_screenshotAlpha(.0f),
		_screenshotMultiplier(3.0f),

		_visualizationMode(CGAppEnum::VIS_TRIANGLES),

		_lidarPointSize(2.0f),
		_lidarPointCloudColor(1.0f, 1.0f, .0f),
		_scenePointCloudColor(1.0f, .0f, .0f),

		_bvhWireframeColor(1.0f, 1.0f, .0f),
		_wireframeColor(0.0f),

		_ambientOcclusion(true),
		_renderSemanticConcept(false),
		_semanticRenderingConcept(ASPRS_CONCEPT),

		_bvhNodesPercentage(1.0f),
		_grayscaleHeight(false),
		_grayscaleIntensity(false),
		_heightBoundaries(.0f, 1.0f),
		_intensityHDR(false),
		_intensityAddition(.0f),
		_intensityExposure(1.0f),
		_intensityGamma(2.2f),
		_intensityMultiplier(1.0f),
		_lastReturnRendered(1),
		_lidarBeamNormal(.0f, -1.0f, .0f),
		_lineWidth(5.0f),
		_numLiDARRBeamSubdivisions(30),
		_numLiDARRangeSubdivisions(30),
		_pointCloudType(PointCloudType::ASPRS_SEMANTIC),
		_raysPercentage(.0f),
		_returnDivisionThreshold(.0f),
		_showBVH(false),
		_showLiDARPath(false),
		_showLidarBeam(false),
		_showLidarMaxRange(false),
		_showLidarModel(false),
		_showLidarPointCloud(true),
		_showLidarRays(true),
		_showTerrainRegularGrid(false),
		_showTriangleMesh(true),

		_snowHeightFactor(68.0f),			
		_snowHeightThreshold(1.0f),
		_snowSlopeFactor(20.0f)
	{
	}
};

