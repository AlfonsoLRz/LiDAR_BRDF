#pragma once

#include "Graphics/Core/Model3D.h"
#include "Utilities/ChronoUtilities.h"

/**
*	@file PipelineMetrics.h
*	@authors Alfonso López Ruiz (allopezr@ujaen.es)
*	@date 31/01/2022
*/

#define ENABLE_INIT_CHRONO true
#define STOP_OPENGL_PIPELINE true
#define USE_MATERIAL_NAME true

/**
*	@brief Metrics for the LiDAR pipeline and manuscript reports.
*/
class PipelineMetrics
{
public:
	enum LiDARStage
	{
		PREPARE_ATTRIBUTES, RAY_BUILDING, PREPARE, FIND_COLLISION, REDUCE, INTENSITY, OUTLIERS, RETURNS, READ, WRITE, NUM_STAGES
	};

protected:
	const static inline std::string CLASS_COUNT_FILENAME = "Results/ClassCount.txt";
	const static inline std::string FRAME_COLLISION_FILENAME = "Results/FrameCollisions.txt";
	const static inline std::string FRAME_RESPONSE_TIME_FILENAME = "Results/frame_time.txt";
	const static inline std::string STAGE_TITLE[NUM_STAGES] = { "Prepare Attributes", "Ray Building", "Prepare", "Find Collision", "Reduce", "Intensity", "Outliers", "Returns", "Read", "Write" };

	std::map<std::string, unsigned>			_classCount;					//!<
	std::vector<long>						_frameCollisions;				//!<
	std::vector<long long>					_frameResponseTime;				//!<
	std::vector<PipelineMetrics>			_frameMetrics;					//!<
	long									_numCollisions;					//!<
	long long								_stageTime[NUM_STAGES];			//!< Time recorded per stage

protected:
	/**
	*	@brief Adds a new frame with its corresponding response time.
	*/
	void addFrame(const PipelineMetrics& metrics);

	/**
	*	@brief Average response time for a single stage.
	*/
	float getMean(PipelineMetrics::LiDARStage initStage, PipelineMetrics::LiDARStage endStage) const;

public:
	/**
	*	@brief Constructor.
	*/
	PipelineMetrics();

	/**
	*	@brief Merges two pipeline metrics.
	*/
	void add(const PipelineMetrics& measurements);

	/**
	*	@brief Add collision to current class count.
	*/
	void addCollisions(std::vector<Model3D::TriangleCollisionGPUData>* collisions, Group3D* scene);

	/**
	*	@brief Clears current storage concerning class count.
	*/
	void clearClassStorage();

	/**
	*	@brief Divides the metrics between a given factor.
	*/
	void divide(float division);

	/**
	*	@brief Exports the number of points per class.
	*/
	void exportCollisionClass();

	/**
	*	@brief Exports the number of collisions of each frame.
	*/
	void exportFrameCollisions();

	/**
	*	@brief Exports the response time of each frame.
	*/
	void exportFrameTime();

	/**
	*	@return Global measured time for all stages.
	*/
	long long getGlobalTime(PipelineMetrics::LiDARStage stageInit = PipelineMetrics::PREPARE_ATTRIBUTES, PipelineMetrics::LiDARStage stageFinal = PipelineMetrics::WRITE) const;

	/**
	*	@return Response time for an specific stage.
	*/
	long long getStageTime(PipelineMetrics::LiDARStage stage) { return _stageTime[stage]; }

	/**
	*	@brief
	*/
	float getStd(PipelineMetrics::LiDARStage initStage = PipelineMetrics::PREPARE_ATTRIBUTES, PipelineMetrics::LiDARStage endStage = PipelineMetrics::WRITE) const;

	/**
	*	@brief Encapsulates the chrono initialization.
	*/
	void initChrono() { 
		#if ENABLE_INIT_CHRONO 
			ChronoUtilities::initChrono();
		#endif
	}

	/**
	*	@brief Asks the chrono for the time measured since the last init call.
	*/
	void measureStage(LiDARStage stage, ChronoUtilities::TimeUnit unit = ChronoUtilities::MICROSECONDS);

	/**
	*	@brief Overriding console message.
	*/
	friend std::ostream& operator<<(std::ostream& os, const PipelineMetrics& dt);
};

