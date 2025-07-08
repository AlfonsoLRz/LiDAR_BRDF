#include "stdafx.h"
#include "PipelineMetrics.h"

#include "Graphics/Core/Group3D.h"
#include "Utilities/FileManagement.h"

// [Public methods]

PipelineMetrics::PipelineMetrics(): _numCollisions(0)
{
	for (int stageIdx = 0; stageIdx < NUM_STAGES; ++stageIdx)
	{
		_stageTime[stageIdx] = 0;
	}
}

void PipelineMetrics::add(const PipelineMetrics& measurements)
{
	for (int stageIdx = 0; stageIdx < NUM_STAGES; ++stageIdx)
	{
		_stageTime[stageIdx] += measurements._stageTime[stageIdx];
	}

	this->addFrame(measurements);
}

void PipelineMetrics::addCollisions(std::vector<Model3D::TriangleCollisionGPUData>* collisions, Group3D* scene)
{
	std::string className;

	for (const Model3D::TriangleCollisionGPUData& collision : *collisions)
	{
#if USE_MATERIAL_NAME
		if (collision._modelCompID >= 0) className = MaterialDatabase::getInstance()->getMaterialName(scene->getModelComponent(collision._modelCompID)->_materialID);
#else
		className = Model3D::getCustomGroupName(scene, collision._modelCompID);
#endif

		if (_classCount.find(className) != _classCount.end())
		{
			_classCount[className] += 1;
		}
		else
		{
			_classCount[className] = 0;
		}
	}

	_frameCollisions.push_back(collisions->size());
}

void PipelineMetrics::clearClassStorage()
{
	_classCount.clear();
}

void PipelineMetrics::divide(float division)
{
	division = glm::max(division, 1.0f);	// Avoid zero division

	for (int stageIdx = 0; stageIdx < NUM_STAGES; ++stageIdx)
	{
		_stageTime[stageIdx] /= division;
	}
}

void PipelineMetrics::exportCollisionClass()
{
	std::string className;
	std::map<std::string, unsigned>	classCount;				
	std::multimap<unsigned, std::string> countClass;					

	// Order results
	unsigned numPoints = 0;
	for (auto& pair : _classCount)
	{
		className = pair.first;
		className.erase(std::remove(className.begin(), className.end(), ' '), className.end());
		countClass.insert(std::make_pair(pair.second, className));
		classCount.insert(std::make_pair(className, pair.second));
		numPoints += pair.second;
	}

	std::cout << "Number of Classes: " << _classCount.size() << std::endl;
	std::cout << "Number of Points: " << numPoints << std::endl;

	// Print symbolic x coords
	std::string symbolicXCoords = "\nsymbolic x coords \t= {\n\t";
	std::string pointsClass = "{";

#if ITERATE_BY_COUNT
	auto itSymbolic = countClass.rbegin();
	while (itSymbolic != countClass.rend())
	{
		symbolicXCoords += itSymbolic->second + ", ";
		pointsClass += "(" + itSymbolic->second + ", " + std::to_string(itSymbolic->first) + ") ";

		++itSymbolic;
	}
#else
	auto itSymbolic = classCount.begin();
	while (itSymbolic != classCount.end())
	{
		symbolicXCoords += itSymbolic->first + ", ";
		pointsClass += "(" + itSymbolic->first + ", " + std::to_string(itSymbolic->second) + ") ";

		++itSymbolic;
	}
#endif
	symbolicXCoords += "\t\n},";
	pointsClass += "};";

	FileManagement::writeString(CLASS_COUNT_FILENAME, symbolicXCoords + "\n" + pointsClass);
}

void PipelineMetrics::exportFrameCollisions()
{
	std::string frameCollisions = "{\n\t";

	for (int idx = 0; idx < _frameCollisions.size(); ++idx)
	{
		frameCollisions += "(" + std::to_string(idx + 1) + ", " + std::to_string(_frameCollisions[idx]) + ") ";
	}

	frameCollisions += "\n};";
	FileManagement::writeString(FRAME_COLLISION_FILENAME, frameCollisions);
}

void PipelineMetrics::exportFrameTime()
{
	//std::string frameResponseTime = "{\n\t";

	//for (int idx = 0; idx < _frameResponseTime.size(); ++idx)
	//{
	//	frameResponseTime += "(" + std::to_string(idx + 1) + ", " + std::to_string(_frameResponseTime[idx]) + ") ";
	//}

	//frameResponseTime += "\n};";
	std::string frameResponseTime;

	for (int idx = 0; idx < _frameResponseTime.size(); ++idx)
	{
		frameResponseTime += std::to_string(_frameResponseTime[idx]);
	}

	FileManagement::writeString(FRAME_RESPONSE_TIME_FILENAME, frameResponseTime);
}

long long PipelineMetrics::getGlobalTime(PipelineMetrics::LiDARStage stageInit, PipelineMetrics::LiDARStage stageFinal) const
{
	long long globalTime = 0;

	for (int stageIdx = stageInit; stageIdx <= glm::min(int(stageFinal), int(WRITE)); ++stageIdx)
	{
		globalTime += _stageTime[stageIdx];
	}

	return globalTime;
}

float PipelineMetrics::getStd(PipelineMetrics::LiDARStage initStage, PipelineMetrics::LiDARStage endStage) const
{
	float std = .0f, dist;

	for (const PipelineMetrics& metrics : _frameMetrics)
	{
		long long measuredTime = metrics.getGlobalTime(initStage, endStage);
		dist = glm::abs(measuredTime - getMean(initStage, endStage));
		std += dist * dist;
	}

	return std::sqrt(std / static_cast<float>(_frameMetrics.size()));
}

void PipelineMetrics::measureStage(LiDARStage stage, ChronoUtilities::TimeUnit unit)
{
#if STOP_OPENGL_PIPELINE
	glFinish();
#endif

#if ENABLE_INIT_CHRONO 
	_stageTime[stage] += ChronoUtilities::getDuration(unit);
#endif
}

std::ostream& operator<<(std::ostream& os, const PipelineMetrics& pm)
{
	auto globalTime = pm.getGlobalTime();

	for (int stageIdx = 0; stageIdx < PipelineMetrics::NUM_STAGES; ++stageIdx)
	{
		os << PipelineMetrics::STAGE_TITLE[stageIdx] << ": " << float(pm._stageTime[stageIdx]) / globalTime << " | " << float(pm._stageTime[stageIdx]) / float(pm._frameResponseTime.size()) << " +- " << pm.getStd(static_cast<PipelineMetrics::LiDARStage>(stageIdx), (static_cast<PipelineMetrics::LiDARStage>(stageIdx))) << "\n";
	}

	return os;
}

// [Protected methods]

void PipelineMetrics::addFrame(const PipelineMetrics& metrics)
{
	_frameResponseTime.push_back(metrics.getGlobalTime());
	_frameMetrics.push_back(metrics);
}

float PipelineMetrics::getMean(PipelineMetrics::LiDARStage initStage, PipelineMetrics::LiDARStage endStage) const
{
	return this->getGlobalTime(initStage, endStage) / static_cast<float>(_frameResponseTime.size());
}
