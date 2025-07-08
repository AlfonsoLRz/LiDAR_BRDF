#include "stdafx.h"
#include "DrawPath.h"

// [Public methods]

DrawPath::DrawPath(const mat4& modelMatrix): Model3D(modelMatrix)
{
	this->setVAOData();
}

DrawPath::~DrawPath()
{
	for (CatmullRom* interpolation: _interpolation)
		delete interpolation;
	_interpolation.clear();
}

void DrawPath::drawAsLines(RenderingShader* shader, const RendEnum::RendShaderTypes shaderType, std::vector<mat4>& matrix)
{
	VAO* vao = _modelComp[0]->_vao;

	if (vao)
	{
		this->setShaderUniforms(shader, shaderType, matrix);

		vao->drawObject(RendEnum::IBO_WIREFRAME, GL_LINE_STRIP, _modelComp[0]->_topologyIndicesLength[RendEnum::IBO_WIREFRAME]);
	}
}

bool DrawPath::load(const mat4& modelMatrix)
{
	if (_interpolation.empty()) return false;

	unsigned accumulatedPoints = 0;
	Model3D::ModelComponent* modelComp = _modelComp[0];
	modelComp->releaseMemory();

	for (CatmullRom* interpolation : _interpolation)
	{
		std::vector<vec4> points;
		interpolation->getPoints(points, .01f);

		for (int idx = 0; idx < points.size(); ++idx)
		{
			modelComp->_geometry.push_back(Model3D::VertexGPUData{ points[idx]});
			modelComp->_wireframe.push_back(accumulatedPoints + idx);
		}

		accumulatedPoints += points.size();
		modelComp->_wireframe.push_back(RESTART_PRIMITIVE_INDEX);
	}

	this->generatePointCloud();
	this->updateVAOData();

	return true;
}

void DrawPath::updateInterpolation(std::vector<vec2>& path, float height, const vec2& canvasSize, const AABB& aabb)
{
	for (CatmullRom* interpolation : _interpolation)
		delete interpolation;
	_interpolation.clear();

	std::vector<float> timeKey (path.size());
	std::vector<vec4> points;
	vec2 sceneSize		= vec2(aabb.size().x, aabb.size().z);
	vec2 sceneMinPoint	= vec2(aabb.min().x, aabb.min().z);

	for (int idx = 0; idx < path.size(); ++idx)
	{
		vec2 newPoint = path[idx] * sceneSize / canvasSize + sceneMinPoint;
		points.push_back(vec4(newPoint.x, height, newPoint.y, 1.0f));
	}

	CatmullRom* interpolation = new CatmullRom(points);
	for (int timeKeyIdx = 0; timeKeyIdx < path.size(); ++timeKeyIdx)
	{
		timeKey[timeKeyIdx] = timeKeyIdx / float(path.size());
	}
	interpolation->setTimeKey(timeKey);
	_interpolation.push_back(interpolation);
}

void DrawPath::updateInterpolation(const std::vector<Interpolation*>& paths)
{
	for (CatmullRom* interpolation : _interpolation)
		delete interpolation;
	_interpolation.clear();

	for (Interpolation* interpolation: paths)
	{
		std::vector<vec4> points;
		BouncePath* bouncePath = interpolation->getWaypoints();

		for (const vec4& point: *bouncePath)
			points.push_back(point);

		std::vector<float> timeKey(points.size());
		CatmullRom* interpolation = new CatmullRom(points);

		for (int timeKeyIdx = 0; timeKeyIdx < points.size(); ++timeKeyIdx)
		{
			timeKey[timeKeyIdx] = timeKeyIdx / float(points.size());
		}
		interpolation->setTimeKey(timeKey);
		_interpolation.push_back(interpolation);
	}
}
