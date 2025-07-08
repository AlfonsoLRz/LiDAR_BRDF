#include "stdafx.h"
#include "LiDARPointCloud.h"

#include "Utilities/Histogram.h"
#include "Utilities/PipelineMetrics.h"

/// [Public methods]

LiDARPointCloud::LiDARPointCloud() : _numPoints(.0f), _maxGPSTime(.0f)
{
}

LiDARPointCloud::~LiDARPointCloud()
{
}

bool LiDARPointCloud::archive()
{
	_points.clear();
	_normal.clear();
	_textCoord.clear();
	_intensity.clear();
	_modelComponent.clear();
	_returnNumber.clear();
	_returnPercent.clear();
	_scanAngleRank.clear();
	_scanDirection.clear();
	_maxGPSTime = .0;

	return true;
}

void LiDARPointCloud::pushCollisions(std::vector<Model3D::TriangleCollisionGPUData>& collisions, std::vector<Model3D::ModelComponent*>* modelComponents)
{
	for (Model3D::TriangleCollisionGPUData& collision : collisions)
	{
		_points.push_back(vec4(collision._point, 1.0f));
		_normal.push_back(collision._normal);
		_textCoord.push_back(collision._textCoord);
		_intensity.push_back(collision._intensity);
		_modelComponent.push_back(collision._modelCompID);
		_returnNumber.push_back(collision._returnNumber);
		_returnPercent.push_back((collision._returnNumber + 1) / float(collision._numReturns));
		_scanAngleRank.push_back(collision._angle);
		_scanDirection.push_back(collision._rayDirection);
		_gpsTime.push_back(collision._gpsTime);

		_aabb.update(collision._point);
		_maxGPSTime = std::max(_maxGPSTime, _gpsTime[_gpsTime.size() - 1]);
	}
}

bool LiDARPointCloud::writePLY(const std::string& filename, Group3D* scene, bool asynchronous)
{
	if (asynchronous)
	{
		// Save point cloud in a different thread
		std::thread writeImageThread(&LiDARPointCloud::writePLYThreaded, this, filename, scene);
		writeImageThread.detach();

		return true;
	}
	else
	{
		return writePLYThreaded(filename, scene);
	}
}

/// [Protected methods]

bool LiDARPointCloud::writeBinary()
{
	return false;
}

bool LiDARPointCloud::writePLYThreaded(const std::string& filename, Group3D* scene)
{
	std::filebuf fileBufferBinary;
	fileBufferBinary.open(filename, std::ios::out | std::ios::binary);
	std::ostream outstreamBinary(&fileBufferBinary);
	if (outstreamBinary.fail()) throw std::runtime_error("Failed to open " + filename + "...");

	tinyply::PlyFile pointCloud;

	// Fill vectors
	std::vector<vec3> position, normal, semanticGroupColor, asprsSemanticGroupColor, scanDirection;
	std::vector<vec2> textureCoord;
	std::vector<float> intensity, returnPercent, scanAngleRank;
	std::vector<uvec2> returns;
	std::vector<uint8_t> semanticGroups, asprsSemanticGroups;

	std::unordered_map<unsigned, int> modelSemanticGroup, modelASPRSGroup;
	unsigned semanticGroup, asprsSemanticGroup;
	unsigned modelComponentID;
	const unsigned numPoints = this->getNumPoints();

	// Reserve space to avoid asking to avoid allocating more than once
	position.reserve(numPoints);
	normal.reserve(numPoints);
	semanticGroupColor.reserve(numPoints);
	asprsSemanticGroupColor.reserve(numPoints);
	semanticGroups.reserve(numPoints);
	asprsSemanticGroups.reserve(numPoints);
	textureCoord.reserve(numPoints);
	intensity.reserve(numPoints);
	returnPercent.reserve(numPoints);
	returns.reserve(numPoints);
	scanDirection.reserve(numPoints);
	scanAngleRank.reserve(numPoints);

	for (int pointIdx = 0; pointIdx < numPoints; ++pointIdx)
	{
		modelComponentID = _modelComponent[pointIdx];
		semanticGroup = scene->getModelComponent(modelComponentID)->_semanticGroup;
		asprsSemanticGroup = scene->getModelComponent(modelComponentID)->_asprsSemanticGroup;

		if (semanticGroup < 0)
		{
			auto it = modelSemanticGroup.find(semanticGroup);

			if (it != modelSemanticGroup.end())
			{
				semanticGroup = modelSemanticGroup[modelComponentID];
			}
			else
			{
				bool found = false;
				int semanticGroup_x = -1;

				scene->getSemanticGroup(modelComponentID, semanticGroup_x, found);
				modelSemanticGroup[modelComponentID] = semanticGroup = semanticGroup_x;
			}
		}

		if (asprsSemanticGroup < 0)
		{
			auto it = modelASPRSGroup.find(asprsSemanticGroup);

			if (it != modelASPRSGroup.end())
			{
				asprsSemanticGroup = modelASPRSGroup[modelComponentID];
			}
			else
			{
				bool found = false;
				int semanticGroup_y = -1;

				scene->getASPRSSemanticGroup(modelComponentID, semanticGroup_y, found);
				modelASPRSGroup[modelComponentID] = asprsSemanticGroup = semanticGroup_y;
			}
		}

		position.push_back(_points[pointIdx]);
		normal.push_back(_normal[pointIdx]);
		textureCoord.push_back(_textCoord[pointIdx]);
		intensity.push_back(_intensity[pointIdx]);
		returns.push_back(uvec2(_returnNumber[pointIdx], unsigned(_returnNumber[pointIdx] * _returnPercent[pointIdx])));
		returnPercent.push_back(_returnPercent[pointIdx]);
		semanticGroups.push_back(semanticGroup);
		asprsSemanticGroups.push_back(asprsSemanticGroup);
		semanticGroupColor.push_back(scene->getSemanticColor(semanticGroup));
		asprsSemanticGroupColor.push_back(scene->getASPRSColor(asprsSemanticGroup));
		scanAngleRank.push_back(_scanAngleRank[pointIdx]);
		scanDirection.push_back(_scanDirection[pointIdx]);
	}

	pointCloud.add_properties_to_element("LiDAR", { "x", "y", "z" }, tinyply::Type::FLOAT32, position.size(), reinterpret_cast<uint8_t*>(position.data()), tinyply::Type::INVALID, 0);
	pointCloud.add_properties_to_element("LiDAR", { "nx", "ny", "nz" }, tinyply::Type::FLOAT32, normal.size(), reinterpret_cast<uint8_t*>(normal.data()), tinyply::Type::INVALID, 0);
	pointCloud.add_properties_to_element("LiDAR", { "u", "v" }, tinyply::Type::FLOAT32, textureCoord.size(), reinterpret_cast<uint8_t*>(textureCoord.data()), tinyply::Type::INVALID, 0);
	pointCloud.add_properties_to_element("LiDAR", { "intensity" }, tinyply::Type::FLOAT32, intensity.size(), reinterpret_cast<uint8_t*>(intensity.data()), tinyply::Type::INVALID, 0);
	pointCloud.add_properties_to_element("LiDAR", { "scan_rank" }, tinyply::Type::FLOAT32, scanAngleRank.size(), reinterpret_cast<uint8_t*>(scanAngleRank.data()), tinyply::Type::INVALID, 0);
	pointCloud.add_properties_to_element("LiDAR", { "scan_direction_x", "scan_direction_y", "scan_direction_z" }, tinyply::Type::FLOAT32, scanDirection.size(), reinterpret_cast<uint8_t*>(scanDirection.data()), tinyply::Type::INVALID, 0);
	pointCloud.add_properties_to_element("LiDAR", { "returnNumber", "numReturns" }, tinyply::Type::UINT8, returns.size(), reinterpret_cast<uint8_t*>(returns.data()), tinyply::Type::INVALID, 0);
	pointCloud.add_properties_to_element("LiDAR", { "returnPercent" }, tinyply::Type::FLOAT32, returnPercent.size(), reinterpret_cast<uint8_t*>(returnPercent.data()), tinyply::Type::INVALID, 0);
	pointCloud.add_properties_to_element("LiDAR", { "semanticGroup" }, tinyply::Type::UINT8, semanticGroups.size(), reinterpret_cast<uint8_t*>(semanticGroups.data()), tinyply::Type::INVALID, 0);
	pointCloud.add_properties_to_element("LiDAR", { "asprsSemanticGroup" }, tinyply::Type::UINT8, asprsSemanticGroups.size(), reinterpret_cast<uint8_t*>(asprsSemanticGroups.data()), tinyply::Type::INVALID, 0);
	pointCloud.add_properties_to_element("LiDAR", { "semanticGroup_red", "semanticGroup_green", "semanticGroup_blue" }, tinyply::Type::FLOAT32, semanticGroupColor.size(), reinterpret_cast<uint8_t*>(semanticGroupColor.data()), tinyply::Type::INVALID, 0);
	pointCloud.add_properties_to_element("LiDAR", { "asprsSemanticGroup_red", "asprsSemanticGroup_green", "asprsSemanticGroup_blue" }, tinyply::Type::FLOAT32, asprsSemanticGroupColor.size(), reinterpret_cast<uint8_t*>(asprsSemanticGroupColor.data()), tinyply::Type::INVALID, 0);

	// Write a binary file
	pointCloud.write(outstreamBinary, true);

	return true;
}
