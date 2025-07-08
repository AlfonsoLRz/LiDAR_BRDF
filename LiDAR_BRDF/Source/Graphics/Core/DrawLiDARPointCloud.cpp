#include "stdafx.h"
#include "DrawLiDARPointCloud.h"

/// [Public methods]

DrawLiDARPointCloud::DrawLiDARPointCloud(LiDARPointCloud* pointCloud, const mat4& modelMatrix) :
	Model3D(modelMatrix, 1), _pointCloud(pointCloud), _minMaxIntensity(FLT_MAX, FLT_MIN)
{
	for (float intensity : _pointCloud->_intensity)
	{
		_minMaxIntensity = vec2(std::min(_minMaxIntensity.x, intensity), std::max(_minMaxIntensity.y, intensity));
	}
}

DrawLiDARPointCloud::~DrawLiDARPointCloud()
{
}

bool DrawLiDARPointCloud::load(const mat4& modelMatrix)
{
	if (!_loaded && _pointCloud)
	{
		this->setVAOData();

		return _loaded = true;
	}

	return false;
}

void DrawLiDARPointCloud::updateVAO()
{
	ModelComponent* modelComponent = _modelComp[0];
	modelComponent->_pointCloud.resize(_pointCloud->getNumPoints());
	std::iota(modelComponent->_pointCloud.begin(), modelComponent->_pointCloud.end(), 0);
	modelComponent->_topologyIndicesLength[RendEnum::IBO_POINT_CLOUD] = _pointCloud->getNumPoints();
	
	modelComponent->_vao->setVBOData(RendEnum::VBO_POSITION, _pointCloud->_points);
	modelComponent->_vao->setVBOData(RendEnum::VBO_RETURN_NUMBER, _pointCloud->_returnNumber);
	modelComponent->_vao->setVBOData(RendEnum::VBO_RETURN_DIVISION, _pointCloud->_returnPercent);
	modelComponent->_vao->setVBOData(RendEnum::VBO_INTENSITY, _pointCloud->_intensity);
	modelComponent->_vao->setVBOData(RendEnum::VBO_NORMAL, _pointCloud->_normal);
	modelComponent->_vao->setVBOData(RendEnum::VBO_SCAN_ANGLE_RANK, _pointCloud->_scanAngleRank);
	modelComponent->_vao->setVBOData(RendEnum::VBO_SCAN_ANGLE_DIRECTION, _pointCloud->_scanDirection);
	modelComponent->_vao->setVBOData(RendEnum::VBO_GPS_TIME, _pointCloud->_gpsTime);
	modelComponent->_vao->setIBOData(RendEnum::IBO_POINT_CLOUD, modelComponent->_pointCloud);

	_minMaxIntensity = vec2(FLT_MAX, FLT_MIN);
	for (float intensity : _pointCloud->_intensity)
	{
		_minMaxIntensity = vec2(std::min(_minMaxIntensity.x, intensity), std::max(_minMaxIntensity.y, intensity));
	}
}

/// [Protected methods]

void DrawLiDARPointCloud::setVAOData()
{
	ModelComponent* modelComponent = _modelComp[0];
	modelComponent->_vao = new VAO(false);
	modelComponent->_vao->defineIntensityVBO();
	modelComponent->_vao->defineReturnNumberVBO();
	modelComponent->_vao->defineReturnPercentageVBO();
	modelComponent->_vao->defineScanAngleRankVBO();
	modelComponent->_vao->defineScanDirectionVBO();
	modelComponent->_vao->defineGPSTimeVBO();

	this->updateVAO();
}