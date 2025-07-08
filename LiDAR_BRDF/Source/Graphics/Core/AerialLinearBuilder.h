#pragma once

#include "Graphics/Core/RayBuilder.h"

/**
*	@file AerialLinearBuilder.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 02/02/2021
*/

/**
*	@brief
*/
class AerialLinearBuilder : public RayBuilder
{
protected:
	/**
	*	@brief Builds those parameters useful for building rays in ALS station.
	*/
	ALSParameters* buildParameters(LiDARParameters* LiDARParams, AABB& sceneAABB);
	
	/**
	*	@brief Builds rays to be launched in CPU.
	*/
	virtual void buildRaysCPU(ALSParameters* parameters, LiDARParameters* LiDARParams, AABB& sceneAABB);
	
	/**
	*	@brief Builds rays to be launched in GPU.
	*/
	virtual void buildRaysGPU(ALSParameters* parameters, LiDARParameters* LiDARParams, AABB& sceneAABB);
	
	/**
	*	@brief  
	*/
	void throwRays(ALSParameters* parameters, LiDARParameters* LiDARParams, std::vector<Model3D::RayGPUData>& rays, const vec3& LiDARPosition, const vec3& LiDARDirection, 
				   const float startAngle, const float fov, const unsigned numPulses, unsigned baseIndex);

public:
	/**
	*	@brief Builds those rays which are thrown from the LiDAR sensor according to the class instance.
	*/
	virtual void buildRays(LiDARParameters* LiDARParams, AABB& sceneAABB);

	/**
	*	@brief Initializes the context regarding memory allocation and parameter computation.
	*/
	virtual void initializeContext(LiDARParameters* LiDARParams, AABB& sceneAABB);
};

