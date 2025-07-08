#pragma once

#include "Graphics/Core/RayBuilder.h"

/**
*	@file TerrestrialSphericalBuilder.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 02/02/2021
*/

/**
*	@brief
*/
class TerrestrialSphericalBuilder : public RayBuilder
{
protected:
	const static float AXIS_JITTERING_WEIGHT;			//!< Noise for rotation axis
	const static float ANGLE_JITTERING_WEIGHT;			//!< Noise for amount of rotation

protected:
	/**
	*	@brief Builds those parameters useful for building rays in TLS station. 
	*/
	TLSParameters* buildParameters(LiDARParameters* LiDARParams, AABB& sceneAABB);
	
	/**
	*	@brief Builds rays to be launched in GPU.
	*/
	virtual void buildRaysCPU(TLSParameters* parameters, LiDARParameters* LiDARParams, AABB& sceneAABB);
	
	/**
	*	@brief Builds rays to be launched in GPU.
	*/
	virtual void buildRaysGPU(TLSParameters* parameters, LiDARParameters* LiDARParams, AABB& sceneAABB);
	
	/**
	*	@brief Calculate multiple channels positions, so that each one launch their rays from a different height. 
	*/
	void getSensorPosition(std::vector<vec3>& sensor, const unsigned numChannels, const vec3& origin);

	/**
	*	@return Vertical resolution, depending on the uniformity of vertical subdivision.
	*/
	int getVerticalResolution(LiDARParameters* LiDARParams, TLSParameters* tlsParams);

	/**
	*	@brief Precalculates the vertical angle of each subdivision.
	*/
	void precalculateVerticalAngles(LiDARParameters* LiDARParams, TLSParameters* tlsParams);

public:
	/**
	*	@brief Builds those rays which are thrown from the LiDAR sensor according to the class instance.
	*/
	virtual void buildRays(LiDARParameters* LiDARParams, AABB& sceneAABB);

	/**
	*	@return Number of rays that will be simulated.
	*/
	virtual unsigned getNumSimulatedRays(LiDARParameters* LiDARParams, BuildingParameters* buildingParams);

	/**
	*	@brief Initializes the context regarding memory allocation and parameter computation.
	*/
	virtual void initializeContext(LiDARParameters* LiDARParams, AABB& sceneAABB);

	/**
	*	@brief Reset ray count for simulations during a path.
	*/
	virtual void resetPendingRays(LiDARParameters* LiDARParams);
};

