#pragma once

#include "Geometry/3D/AABB.h"
#include "Geometry/Animation/Interpolation.h"
#include "Geometry/Animation/LinearInterpolation.h"
#include "Graphics/Core/Model3D.h"
#include "Utilities/RandomUtilities.h"

#define APPLY_PULSE_RADIUS
#define EXPORT_PATHS true

/**
*	@file RayBuilder.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 02/02/2021
*/

/**
*	@brief Base class for any ray builder which instantiates those rays to be thrown during a LiDAR simulation
*/
class RayBuilder
{
	friend class LiDARSimulation;
protected:
	const static vec3	AERIAL_UP_VECTOR;			//!<
	const static float	BOUNDARY_OFFSET;			//!< Offset for terrain boundaries when throwing rays
	const static GLuint	NOISE_BUFFER_SIZE;			//!<
	const static vec3	TERRESTRIAL_UP_VECTOR;		//!<

protected:
	struct BuildingParameters
	{
		unsigned	_allowedRaysIteration;
		unsigned	_currentNumRays;
		unsigned	_leftRays;
		unsigned	_numRays;
		unsigned	_numThreads;
		unsigned	_minSize;
		unsigned	_numGroups;
		float		_timePulse;

		// SSBOs
		GLuint		_rayBuffer;
		GLuint		_noiseBuffer;

		/**
		*	@brief Constructor.
		*/
		BuildingParameters() { _rayBuffer = UINT_MAX; _noiseBuffer = UINT_MAX; }

		/**
		*	@brief Destructor.
		*/
		virtual ~BuildingParameters()
		{
			glDeleteBuffers(1, &_rayBuffer);
			glDeleteBuffers(1, &_noiseBuffer);
		}
	};

	struct TLSParameters: public BuildingParameters
	{
		float				_channelSpacing;
		vec2				_fovRadians, _reducedFovRadians;
		vec2				_incrementRadians;
		unsigned			_numChannels;
		unsigned			_numRays;
		float				_startRadians;
		float				_startingAngleVertical;
		vec3				_upVector;
		std::vector<float>	_verticalAngleIncrement;			//!< Increment for each new vertical angle subdivision
		int					_verticalRes;

		// SSBOs
		GLuint				_channelBuffer;
		GLuint				_vAngleBuffer;

		/**
		*	@brief Constructor.
		*/
		TLSParameters() { _channelBuffer = UINT_MAX; _vAngleBuffer = UINT_MAX; }

		/**
		*	@brief Destructor.
		*/
		virtual ~TLSParameters()
		{
			glDeleteBuffers(1, &_channelBuffer);
			glDeleteBuffers(1, &_vAngleBuffer);
		}
	};

	struct ALSParameters: public BuildingParameters
	{
		float		_advancePulse;
		float		_advanceScan;
		float		_advanceScan_t;
		float		_ellipseRadius;
		float		_ellipseScale;
		float		_fovRadians;
		float		_heightRadius;
		float		_incrementRadians;
		float		_metersSec;
		unsigned	_numPulsesScan;
		unsigned	_numPulses;
		unsigned	_numScans;
		unsigned	_pathLength;
		unsigned	_pulsesSec;
		unsigned	_numSteps;
		unsigned	_scansSec;
		float		_startRadians;
		vec3		_upVector;

		// SSBOs
		GLuint		_waypointBuffer;

		/**
		*	@brief Constructor.
		*/
		ALSParameters() { _waypointBuffer = UINT_MAX; }

		/**
		*	@brief Destructor.
		*/
		virtual ~ALSParameters()
		{
			glDeleteBuffers(1, &_waypointBuffer);
		}
	};

protected:
	BuildingParameters* _parameters;

protected:
	/**
	*	@brief
	*/
	void addPulseRadius(std::vector<Model3D::RayGPUData>& rays, unsigned baseIndex, const vec3& up, const int numRaysPulse, const float radius);

	/**
	*	@brief
	*/
	void addPulseRadius(std::vector<Model3D::RayGPUData>& rays, Model3D::RayGPUData& ray, const vec3& up, const int numRaysPulse, const float radius);
	
	/**
	*	@brief Builds a white noise texture.
	*/
	GLuint buildNoiseBuffer();
	
	/**
	*	@brief Simplifies a vector of points to avoid user's noise.
	*/
	static std::vector<vec2> douglasPecker(const std::vector<vec2>& points, float epsilon);

	/**
	*	@brief Exports an user-defined path.
	*/
	bool exportPath(const LiDARPath& path, const std::string& filename);
	
	/**
	*	@brief Generates numRays rays in a range. Uniformely distributed rays.
	*/
	float generateRandomNumber(const float range, const float min_val);

	/**
	*	@return Array of paths which indicates the trajectory of our airbone device. 
	*/
	std::vector<Interpolation*> getAirbonePaths(LiDARParameters* LiDARParams, const unsigned numSteps, const AABB& aabb, const float alsHeight);

	/**
	*	@return Number of steps which are needed to cover the whole scene. 
	*/
	unsigned getNumSteps(LiDARParameters* LiDARParams, const AABB& aabb, float& width);

	/**
	*	@brief Radius of LiDAR sensor taking into account its field of view angle.
	*/
	float getRadius(const float fov, const float height);

	/**
	*	@brief Retrieve X and Y axis of a ray, i.e. n axis.
	*/
	void getRadiusAxes(const vec3& n, vec3& u, vec3& v, const vec3& up);

	/**
	*	@brief Initializes parameters in common for ALS and TLS.
	*/
	void initializeContext(LiDARParameters* LiDARParams, BuildingParameters* params);

	/**
	*	@brief Computes the perpendicular distance between point1 and the segment given by point2 and point3. 
	*/
	static float perpendicularDistance(const vec2& point1, const vec2& point2, const vec2& point3);

	/**
	*	@brief 
	*/
	static void removeRedundantPoints(std::vector<vec2>& points);

	/**
	*	@brief  
	*/
	void retrievePath(std::vector<Interpolation*> paths, std::vector<vec4>& waypoints, const float tIncrement);

public:
	/**
	*	@brief Builds those rays which are thrown from the LiDAR sensor according to the class instance.
	*/
	virtual void buildRays(LiDARParameters* LiDARParams, AABB& sceneAABB) = 0;

	/**
	*	@brief Deletes allocated content.
	*/
	virtual void freeContext();

	/**
	*	@brief Initializes the context regarding memory allocation and parameter computation.
	*/
	virtual void initializeContext(LiDARParameters* LiDARParams, AABB& sceneAABB) = 0;

	// ---- Getters ----

	/**
	*	@return True if there are still rays to be thrown.
	*/
	bool arePendingRays() { return _parameters->_leftRays > 0; } 

	/**
	*	@return Current number of rays.
	*/
	unsigned getCurrentNumRays() { return _parameters->_currentNumRays; }

	/**
	*	@return Number of rays that will be simulated.
	*/
	virtual unsigned getNumSimulatedRays(LiDARParameters* LiDARParams, BuildingParameters* buildingParams);

	/**
	*	@return Maximum capacity of a collision buffer.
	*/
	unsigned getRayMaxCapacity();

	/**
	*	@brief Assigns the ray SSBO and its current size.
	*/
	void getRaySSBO(GLuint& ssbo, GLuint& size) { ssbo = _parameters->_rayBuffer; size = _parameters->_currentNumRays; }

	/**
	*	@return AABB offset for paths.
	*/
	static float getSceneOffset() { return BOUNDARY_OFFSET; }

	/**
	*	@brief Reset ray count for simulations during a path.
	*/
	virtual void resetPendingRays(LiDARParameters* LiDARParams);
};

