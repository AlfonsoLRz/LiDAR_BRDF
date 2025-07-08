#include "stdafx.h"
#include "RayBuilder.h"

#include "Geometry/Animation/BezierCurve.h"
#include "Geometry/Animation/CatmullRom.h"

/// [Static attributes]

const vec3		RayBuilder::AERIAL_UP_VECTOR = vec3(.0f, -1.0f, .0f);
const float		RayBuilder::BOUNDARY_OFFSET = .0f;
const GLuint	RayBuilder::NOISE_BUFFER_SIZE = 5e5;
const vec3		RayBuilder::TERRESTRIAL_UP_VECTOR = vec3(.0f, 1.0f, .0f);


/// [Public methods]

void RayBuilder::freeContext()
{
	delete _parameters;
	_parameters = nullptr;
}

void RayBuilder::resetPendingRays(LiDARParameters* LiDARParams)
{
	_parameters->_leftRays = _parameters->_numRays * LiDARParams->_raysPulse;
	_parameters->_currentNumRays = std::min(_parameters->_allowedRaysIteration * LiDARParams->_raysPulse, _parameters->_leftRays);
}

/// [Protected methods]

void RayBuilder::addPulseRadius(std::vector<Model3D::RayGPUData>& rays, unsigned baseIndex, const vec3& up, const int numRaysPulse, const float radius)
{
	vec3 u, v, noise;
	this->getRadiusAxes(rays[baseIndex]._direction, u, v, up);

	for (int ray = 0; ray < numRaysPulse - 1; ++ray)
	{
		noise = float(RandomUtilities::getUniformRandomValue()) * radius * u + float(RandomUtilities::getUniformRandomValue()) * radius * v;	
		rays[baseIndex + ray + 1] = Model3D::RayGPUData(rays[baseIndex]._origin + noise, rays[baseIndex]._destination + noise);
	}
}

void RayBuilder::addPulseRadius(std::vector<Model3D::RayGPUData>& rays, Model3D::RayGPUData& ray, const vec3& up, const int numRaysPulse, const float radius)
{
	vec3 u, v, noise;
	this->getRadiusAxes(ray._direction, u, v, up);

	for (int rayIdx = 0; rayIdx < numRaysPulse - 1; ++rayIdx)
	{
		noise = float(RandomUtilities::getUniformRandomValue()) * radius * u + float(RandomUtilities::getUniformRandomValue()) * radius * v;
		rays.push_back(Model3D::RayGPUData(ray._origin + noise, ray._destination + noise));
	}
}

GLuint RayBuilder::buildNoiseBuffer()
{
	std::vector<float> noiseOutput(NOISE_BUFFER_SIZE);
	RandomUtilities::initializeUniformDistribution(-1.0f, 1.0f);

	for (unsigned index = 0; index < noiseOutput.size(); ++index)
	{
		noiseOutput[index] = RandomUtilities::getUniformRandomValue();
	}

	return ComputeShader::setReadBuffer(noiseOutput, GL_STATIC_DRAW);
}

std::vector<vec2> RayBuilder::douglasPecker(const std::vector<vec2>& points, float epsilon)
{
	std::vector<vec2> resultList;
	float maxDistance = 0;
	unsigned index = 0, endIndex = points.size() - 1;

	for (int i = 1; i < endIndex; ++i)
	{
		float distance = perpendicularDistance(points[i], points[0], points[endIndex]);
		
		if (distance > maxDistance) {
			index = i;
			maxDistance = distance;
		}
	}
	
	// If max distance is greater than epsilon, recursively simplify
	if (maxDistance > epsilon)
	{
		std::vector<vec2> points1(points.begin(), points.begin() + index + 1);
		std::vector<vec2> points2(points.begin() + index, points.end());

		std::vector<vec2> result1 = douglasPecker(points1, epsilon);
		std::vector<vec2> result2 = douglasPecker(points2, epsilon);

		resultList.insert(resultList.end(), result1.begin(), result1.end());
		resultList.insert(resultList.end(), result2.begin() + 1, result2.end());
	}
	else
	{
		resultList.clear();
		resultList.push_back(points[0]);
		resultList.push_back(points[endIndex]);
	}

	return resultList;
}

bool RayBuilder::exportPath(const LiDARPath& path, const std::string& filename)
{
	std::ofstream file(filename);
	if (file.is_open())
	{
		for (const vec2& point : path)
		{
			file << point.x << "," << point.y << "\n";
		}

		file.close();

		return true;
	}

	return false;
}

float RayBuilder::generateRandomNumber(const float range, const float min_val)
{
	return RandomUtilities::getUniformRandomValue()* range + min_val;
}

std::vector<Interpolation*> RayBuilder::getAirbonePaths(LiDARParameters* LiDARParams, const unsigned numSteps, const AABB& aabb, const float alsHeight)
{
	std::vector<Interpolation*> paths;

	if (!LiDARParams->_alsUseManualPath)
	{
		const float depthDivision = (aabb.size().z + BOUNDARY_OFFSET * 2.0f) / (numSteps + 1);

		for (int stepIdx = 0; stepIdx < numSteps && stepIdx < LiDARParams->_alsMaxSceneSweeps; ++stepIdx)
		{
			std::vector<vec4> waypoints;
			waypoints.push_back(vec4(aabb.min().x - BOUNDARY_OFFSET, alsHeight, depthDivision * (stepIdx + 1) + aabb.min().z - BOUNDARY_OFFSET, 1.0f));
			waypoints.push_back(vec4(aabb.max().x + BOUNDARY_OFFSET, alsHeight, depthDivision * (stepIdx + 1) + aabb.min().z - BOUNDARY_OFFSET, 1.0f));

			LinearInterpolation* path = new LinearInterpolation(waypoints);
			paths.push_back(path);
		}
	}
	else
	{
#if EXPORT_PATHS
		const std::string pathFolder = "../Python/PathRenderer/Paths/";
		this->exportPath(LiDARParams->_alsManualPath, pathFolder + "Original.txt");
#endif
		vec2 transformedPoint;
		vec2 canvasSize = LiDARParams->_alsManualPathCanvasSize;
		vec2 sceneSize = vec2(aabb.size().x + BOUNDARY_OFFSET * 2.0f, aabb.size().z + BOUNDARY_OFFSET * 2.0f);
		vec2 sceneMinPoint = vec2(aabb.min().x - BOUNDARY_OFFSET, aabb.min().z - BOUNDARY_OFFSET);
		std::vector<vec4> waypoints;
		this->removeRedundantPoints(LiDARParams->_alsManualPath);
		LiDARPath simplifiedPath = this->douglasPecker(LiDARParams->_alsManualPath, LiDARParams->_douglasPeckerEpsilon);

#if EXPORT_PATHS
		this->exportPath(simplifiedPath, pathFolder + "Douglas-Pecker.txt");
#endif

		for (vec2& pathPoint : simplifiedPath)
		{
			transformedPoint = pathPoint * sceneSize / canvasSize + sceneMinPoint;
			waypoints.push_back(vec4(transformedPoint.x, alsHeight, transformedPoint.y, 1.0f));
		}

		if (!waypoints.empty())
		{
			std::vector<float> timeKey (waypoints.size());

			for (int timeKeyIdx = 0; timeKeyIdx < waypoints.size(); ++timeKeyIdx)
			{
				timeKey[timeKeyIdx] = timeKeyIdx / float(waypoints.size());
			}
			
			Interpolation* interp;
			LinearInterpolation* linearTnterp = nullptr;
			if (LiDARParams->_useCatmullRom)
			{
				CatmullRom* catmullRom = new CatmullRom(waypoints);
				catmullRom->setTimeKey(timeKey);
				interp = catmullRom;
			}
			else
			{
				interp = linearTnterp = new LinearInterpolation(waypoints);
			}
			paths.push_back(interp);

#if EXPORT_PATHS
			LiDARPath points;
			float t = .0f;
			bool finished = false;

			while (t <= 1.0f)
			{
				auto catmullPosition = interp->getPosition(t, finished);
				points.push_back(vec2(catmullPosition.x, catmullPosition.z));
				t += 0.0001f;
			}

			this->exportPath(points, pathFolder + "CatmullRom.txt");
			if (linearTnterp) linearTnterp->resetIndex();
#endif
		}
	}

	return paths;
}

unsigned RayBuilder::getNumSimulatedRays(LiDARParameters* LiDARParams, BuildingParameters* buildingParams)
{
	return buildingParams->_numRays;
}

unsigned RayBuilder::getRayMaxCapacity()
{
	return _parameters->_minSize;
}

unsigned RayBuilder::getNumSteps(LiDARParameters* LiDARParams, const AABB& aabb, float& width)
{
	// Worst scenario: height of scene is the minimum Y value of AABB
	float maximumHeight = LiDARParams->_alsPosition.y - aabb.max().y;
	width = std::abs(this->getRadius(LiDARParams->_alsFOVHorizontal, maximumHeight) * 2.0f);

	return std::ceil((aabb.size().z + BOUNDARY_OFFSET * 2.0f) / (width - width * LiDARParams->_alsOverlapping));
}

float RayBuilder::getRadius(const float fov, const float height)
{
	float LiDARFov_2 = fov / 2.0f * M_PI / 180.0f;
	float radius = std::tan(LiDARFov_2) * height;

	return radius;
}

void RayBuilder::getRadiusAxes(const vec3& n, vec3& u, vec3& v, const vec3& up)
{
	u = glm::normalize(glm::cross(up, n));
	v = glm::normalize(glm::cross(n, u));
}

void RayBuilder::initializeContext(LiDARParameters* LiDARParams, BuildingParameters* params)
{
	params->_allowedRaysIteration	= std::ceil(ComputeShader::getAllowedNumberOfInstances(Model3D::TriangleCollisionGPUData()) / float(LiDARParams->_raysPulse) / float(glm::max(GLuint(LiDARParams->_rayDivisor), LiDARParams->_maxReturns))) - 1;
	params->_numRays				= params->_numThreads;
	params->_minSize				= std::min(params->_allowedRaysIteration, params->_numRays) * LiDARParams->_raysPulse;
	params->_numGroups				= ComputeShader::getNumGroups(params->_minSize / LiDARParams->_raysPulse);
	params->_leftRays				= params->_numRays * LiDARParams->_raysPulse;
	params->_currentNumRays			= std::min(params->_allowedRaysIteration * LiDARParams->_raysPulse, params->_leftRays);

	if (LiDARParams->_gpuInstantiation)
	{
		params->_rayBuffer = ComputeShader::setWriteBuffer(Model3D::RayGPUData(vec3(.0f), vec3(.0f)), params->_minSize);
		params->_noiseBuffer = RayBuilder::buildNoiseBuffer();
	}
}

float RayBuilder::perpendicularDistance(const vec2& point1, const vec2& point2, const vec2& point3)
{
	double dx = point3.x - point2.x;
	double dy = point3.y - point2.y;

	//Normalise
	double mag = pow(pow(dx, 2.0) + pow(dy, 2.0), 0.5);
	if (mag > 0.0)
	{
		dx /= mag; dy /= mag;
	}

	double pvx = point1.x - point2.x;
	double pvy = point1.y - point2.y;

	//Get dot product (project pv onto normalized direction)
	double pvdot = dx * pvx + dy * pvy;

	//Scale line direction vector
	double dsx = pvdot * dx;
	double dsy = pvdot * dy;

	//Subtract this from pv
	double ax = pvx - dsx;
	double ay = pvy - dsy;

	return pow(pow(ax, 2.0) + pow(ay, 2.0), 0.5);
}

void RayBuilder::removeRedundantPoints(std::vector<vec2>& points)
{
	for (int i = 1; i < points.size(); ++i)
	{
		if (glm::all(glm::epsilonEqual(points[i], points[i - 1], glm::epsilon<float>())))
		{
			points.erase(points.begin() + i);
			--i;
		}
	}
}

void RayBuilder::retrievePath(std::vector<Interpolation*> paths, std::vector<vec4>& waypoints, const float tIncrement)
{
	for (Interpolation* interpolation: paths)
	{
		float t = (RandomUtilities::getUniformRandomValue() + 1.0f) / 2.0f * tIncrement / 10.0f;
		bool isPathOver = false;
		vec4 direction, point;

		while (!isPathOver)
		{
			point = interpolation->getPosition(t, isPathOver);
			direction = waypoints.empty() ? point : glm::normalize(point - waypoints[waypoints.size() - 1]);

			if (glm::length(direction) > glm::epsilon<float>())
			{
				waypoints.push_back(point);
			}

			t += tIncrement;
			isPathOver |= t > 1.0f;
		}
	}
}
