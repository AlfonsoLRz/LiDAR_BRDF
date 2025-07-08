#pragma once

#include "Geometry/3D/TriangleMesh.h"
#include "Graphics/Core/LiDARPointCloud.h"
#include "Graphics/Core/Model3D.h"
#include "Graphics/Core/VAO.h"

/**
*	@file Draw3DPointCloud.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 31/08/2019
*/

/**
*	@brief Renders a point cloud in a 3d space.
*/
class DrawLiDARPointCloud: public Model3D
{
protected:
	LiDARPointCloud*		_pointCloud;		//!< Collection of points with any distribution
	vec2					_minMaxIntensity;	//!<

protected:
	/**
	*	@brief Initializes VAO buffer in GPU.
	*/
	virtual void setVAOData();

public:
	/**
	*	@brief Constructor.
	*/
	DrawLiDARPointCloud(LiDARPointCloud* pointCloud, const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Destructor.
	*/
	~DrawLiDARPointCloud();

	/**
	*	@return Minimum and maximum intensity.
	*/
	vec2 getMinMaxIntensity() { return _minMaxIntensity; }

	/**
	*	@brief Computes the point cloud data and sends it to GPU.
	*	@return Success of operation.
	*/
	virtual bool load(const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Updates our GPU buffer from the point cloud pointer. 
	*/
	void updateVAO();
};

