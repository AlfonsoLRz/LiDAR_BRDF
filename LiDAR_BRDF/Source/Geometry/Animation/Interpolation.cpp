#include "stdafx.h"
#include "Interpolation.h"


// [Public methods]

Interpolation::Interpolation(std::vector<vec4>& waypoints) : _waypoints(waypoints)
{
}

Interpolation::Interpolation(std::vector<vec2>& waypoints, float height) 
{
	for (vec2& point : waypoints)
	{
		_waypoints.push_back(vec4(point.x, height, point.y, 1.0f));
	}
}

Interpolation::~Interpolation()
{
}

void Interpolation::exportPoints()
{
	unsigned index = 1;
	
	for(vec4& point: _waypoints)
	{
		std::cout << "path[" << index++ << "] = vec2(" << point.x << ", " << point.z << ");" << std::endl;
	}
}

void Interpolation::getPoints(std::vector<vec4>& points, float t)
{
	points.clear();

	float currT = 0;
	bool finished;

	while (currT < 1.0f)
	{
		points.push_back(this->getPosition(currT, finished));
		currT += t;
	}
}

float Interpolation::getLineLength(std::vector<vec4>& points)
{
	float length = 0.0f;

	for (unsigned i = 1; i < points.size(); ++i)
	{
		length += glm::distance(points[i], points[i - 1]);
	}

	return length;
}