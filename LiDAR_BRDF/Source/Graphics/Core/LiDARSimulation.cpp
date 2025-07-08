#include "stdafx.h"
#include "LiDARSimulation.h"

#include "Geometry/3D/Intersections3D.h"
#include "Geometry/Animation/CatmullRom.h"
#include "Graphics/Application/Renderer.h"
#include "Graphics/Application/TerrainParameters.h"
#include "Graphics/Application/TerrainScene.h"
#include "Graphics/Core/AerialEllipticalBuilder.h"
#include "Graphics/Core/AerialLinearBuilder.h"
#include "Graphics/Core/AerialZigZagBuilder.h"
#include "Graphics/Core/ShaderList.h"
#include "Graphics/Core/TerrestrialSphericalBuilder.h"
#include "Graphics/Core/VAO.h"
#include "Utilities/ChronoUtilities.h"
#include "Utilities/FileManagement.h"
#include "Utilities/RandomUtilities.h"

/// [Initialization of static attributes]

LiDARParameters				LiDARSimulation::LIDAR_PARAMS;
PointCloudParameters		LiDARSimulation::POINT_CLOUD_PARAMS;

const float					LiDARSimulation::NOISE_TEXTURE_FREQUENCY = 10.0f;
const unsigned				LiDARSimulation::NOISE_TEXTURE_SIZE = 5e6;
const GLuint				LiDARSimulation::RAY_MEMORY_BOUNDARY = 10e6;
const float					LiDARSimulation::RAY_OVERFLOW = 10000.0f;
const GLuint				LiDARSimulation::SHADER_UINT_MAX = 0xFFFFFFF;

const RayBuilderApplicators LiDARSimulation::RAY_BUILDER_APPLICATOR = LiDARSimulation::getRayBuildApplicators();

/// [Public methods]

LiDARSimulation::LiDARSimulation(Group3D* scene) :
	_scene(scene), _LiDARRaysVAO(nullptr), _numRays(0),
	_emptyModelComponent(nullptr), _groupGPUData(nullptr), _hermiteSSBO(-1),
	_LiDARMaterialsSSBO(-1), _returnThresholdSSBO(-1), _whiteNoiseSSBO(-1),
	_brdfSSBO(-1), _collisionSSBO(-1), _counterSSBO(-1), _newCounterSSBO(-1), 
	_triangleCollisionSSBO(-1)
{
	Renderer* renderer = Renderer::getInstance();
	
	_emptyModelComponent = new Model3D::ModelComponent(nullptr);
	_pointCloud = new LiDARPointCloud();

	_isForestScene = dynamic_cast<TerrainScene*>(renderer->getCurrentScene()) != nullptr;
	_scene->registerModelComponent(_emptyModelComponent);
}

LiDARSimulation::~LiDARSimulation()
{
	delete _emptyModelComponent;
	delete _LiDARRaysVAO;
	delete _pointCloud;
}

void LiDARSimulation::launchSimulation(bool instantiateRaysVAO)
{
	if ((LIDAR_PARAMS._LiDARType == LiDARParameters::TERRESTRIAL_SPHERICAL && !LIDAR_PARAMS._tlsUseManualPath && _tlsPositions.empty()) || LIDAR_PARAMS._LiDARType != LiDARParameters::TERRESTRIAL_SPHERICAL)
	{
		this->launchSingleSimulation(instantiateRaysVAO);
	}
	else
	{
		vec3 orig = LIDAR_PARAMS._tlsPosition;
		std::vector<vec3>* positions;
		this->getTLSPath(positions);
		this->launchMultipleSimulations(*positions);

		LIDAR_PARAMS._tlsPosition = orig;
		if (LIDAR_PARAMS._tlsUseManualPath) delete positions;
	}
}

std::vector<Interpolation*> LiDARSimulation::getAerialPath()
{
	float width;
	return RAY_BUILDER_APPLICATOR[LiDARParameters::TERRESTRIAL_SPHERICAL]->getAirbonePaths(&LIDAR_PARAMS, RAY_BUILDER_APPLICATOR[LiDARParameters::TERRESTRIAL_SPHERICAL]->getNumSteps(&LIDAR_PARAMS, _scene->getAABB(), width), 
																					_scene->getAABB(), LIDAR_PARAMS._alsPosition.y);
}

/// [Static methods]

std::vector<std::unique_ptr<RayBuilder>> LiDARSimulation::getRayBuildApplicators()
{
	std::vector<std::unique_ptr<RayBuilder>> applicator(LiDARParameters::NUM_RAY_BUILDERS);

	applicator[LiDARParameters::TERRESTRIAL_SPHERICAL].reset(new TerrestrialSphericalBuilder());
	applicator[LiDARParameters::AERIAL_LINEAR].reset(new AerialLinearBuilder());
	applicator[LiDARParameters::AERIAL_ZIGZAG].reset(new AerialZigZagBuilder());
	applicator[LiDARParameters::AERIAL_ELLIPTICAL].reset(new AerialEllipticalBuilder());

	return applicator;
}

/// [Protected methods]

void LiDARSimulation::appendLiDARData(std::vector<Model3D::TriangleCollisionGPUData>* collisions)
{
	Model3D::ModelComponent* modelComponent = nullptr;
	
	_pointCloud->pushCollisions(*collisions, _scene->getRegisteredModelComponents());				// Append results to previous LiDAR point clouds

	for (Model3D::TriangleCollisionGPUData& collision : *collisions)
	{
		modelComponent = _scene->getModelComponent(collision._modelCompID);
		modelComponent->pushBackCapturedPoints(&collision);
		_modelCompMap.insert(modelComponent);
	}

	for (auto& modelIt : _modelCompMap)
	{
		modelIt->loadLiDARPoints();
	}
}

unsigned LiDARSimulation::buildWhiteNoiseTexture(const unsigned size)
{
	std::vector<float> noiseOutput(size);
	const unsigned seed	= rand();
	RandomUtilities::initializeUniformDistribution(.0f, 1.0f);
	
	for (unsigned index = 0; index < noiseOutput.size(); ++index)
	{
		noiseOutput[index] = RandomUtilities::getUniformRandomValue();
	}

	return ComputeShader::setReadBuffer(noiseOutput, GL_STATIC_DRAW);
}

void LiDARSimulation::defineSceneUniforms(ComputeShader* LiDARShader)
{
	//if (_isForestScene) LiDARShader->setUniform("waterHeight", _terrainConfiguration->_terrainParameters._waterHeight);
}

void LiDARSimulation::filterPath(std::vector<vec3>& path)
{
	for (int i = 1; i < path.size(); ++i)
	{
		if (glm::all(glm::epsilonEqual(path[i], path[i - 1], glm::epsilon<float>())))
		{
			path.erase(path.begin() + i);
			--i;
		}
	}
}

float LiDARSimulation::getAtmosphericAttenuation()
{
	// Terrestrial LiDAR
	if (LIDAR_PARAMS._LiDARType == LiDARParameters::RayBuild::TERRESTRIAL_SPHERICAL)
	{
		return LIDAR_PARAMS._tlsAtmosphericClearness * (LIDAR_PARAMS.TLS_MAX_ATMOSPHERE_ATTENUATION - LIDAR_PARAMS.TLS_MIN_ATMOSPHERE_ATTENUATION) + LIDAR_PARAMS.TLS_MIN_ATMOSPHERE_ATTENUATION;
	}

	// Aerial LiDAR
	const float flyingHeight = LIDAR_PARAMS._alsPosition.y - _scene->getAABB().min().y;
	const float referenceHeight = 1000.0f, attenuationReferenceHeight = LIDAR_PARAMS.ALS_MAX_ATMOSPHERE_ATTENUATION;
	const float newAttenuation = attenuationReferenceHeight * flyingHeight / referenceHeight;
	const float attenuationDifference = std::abs(LIDAR_PARAMS.ALS_MAX_ATMOSPHERE_ATTENUATION - newAttenuation);

	return attenuationReferenceHeight + (attenuationDifference);
}

void LiDARSimulation::getTLSPath(std::vector<vec3>*& tlsPath)
{
	if (LIDAR_PARAMS._tlsUseManualPath)
	{
		AABB aabb = _scene->getAABB();
		vec2 transformedPoint;
		vec2 canvasSize = LIDAR_PARAMS._tlsManualPathCanvasSize;
		vec2 sceneSize = vec2(aabb.size().x, aabb.size().z);
		vec2 sceneMinPoint = vec2(aabb.min().x, aabb.min().z);
		std::vector<vec4> waypoints;
		std::vector<vec2> tlsPath_2d = LIDAR_PARAMS._tlsManualPath;

		RayBuilder::removeRedundantPoints(tlsPath_2d);
		tlsPath_2d = RayBuilder::douglasPecker(tlsPath_2d, LIDAR_PARAMS._douglasPeckerEpsilon);

		tlsPath = new std::vector<vec3>();
		for (vec2& pos : tlsPath_2d)
		{
			transformedPoint = pos * sceneSize / canvasSize + sceneMinPoint;
			tlsPath->push_back(vec3(transformedPoint.x, LIDAR_PARAMS._tlsPosition.y, transformedPoint.y));
		}
	}
	else
	{
		tlsPath = &_tlsPositions;
	}
}

void LiDARSimulation::instantiateRaysVAO(std::vector<Model3D::RayGPUData>& rayArray)
{
	if (!_LiDARRaysVAO) _LiDARRaysVAO = new VAO();

	std::vector<vec4> rayPosition;
	std::vector<GLuint>	rayIndices;
	
	for (unsigned ray = 0; ray < rayArray.size(); ++ray)
	{
		rayPosition.push_back(vec4(rayArray[ray]._origin, 1.0f));
		rayPosition.push_back(vec4(rayArray[ray]._origin + rayArray[ray]._direction * RAY_OVERFLOW, 1.0f));

		rayIndices.push_back(ray * 2);
		rayIndices.push_back(ray * 2 + 1);
		rayIndices.push_back(Model3D::RESTART_PRIMITIVE_INDEX);
	}

	_numRays = rayArray.size();
	_LiDARRaysVAO->setVBOData(RendEnum::VBO_POSITION, rayPosition);
	_LiDARRaysVAO->setIBOData(RendEnum::IBOTypes::IBO_WIREFRAME, rayIndices);
}

void LiDARSimulation::launchMultipleSimulations(std::vector<vec3>& positions)
{
	PipelineMetrics globalMetrics;
	unsigned iteration = 0;
	long long rayBuildTime = 0;
	AABB aabb = _scene->getAABB();
	GLuint raySSBO, numRays, totalRays = 0;
	std::vector<Model3D::TriangleCollisionGPUData> collisions;

	// Initialize variables and buffers
	RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->initializeContext(&LIDAR_PARAMS, aabb);
	this->prepareLiDARData(RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->getRayMaxCapacity());
	this->prepareMaterialData((LIDAR_PARAMS._wavelength[0] + LIDAR_PARAMS._wavelength[1]) / 2.0f);

	ChronoUtilities::getDuration();			// Clean chrono

	for (int idx = 0; idx < positions.size(); ++idx)
	{
		PipelineMetrics localMetrics;
		LIDAR_PARAMS._tlsPosition = positions[idx];
		LIDAR_PARAMS._tlsDirection = vec3(.0f);
		if (idx < positions.size() - 1)
		{
			LIDAR_PARAMS._tlsDirection = positions[idx + 1] - positions[idx];
			LIDAR_PARAMS._tlsDirection.y = .0f;
		}

		// Solve intersections in GPU
		RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->resetPendingRays(&LIDAR_PARAMS);
		while (RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->arePendingRays())
		{
			collisions.clear();

			{
				localMetrics.initChrono();

				RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->buildRays(&LIDAR_PARAMS, aabb);
				glFinish();

				localMetrics.measureStage(PipelineMetrics::RAY_BUILDING);
			}

			{
				RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->getRaySSBO(raySSBO, numRays);
				localMetrics.add(this->solveRayIntersection(raySSBO, numRays, collisions, true));
				totalRays += numRays;

				this->appendLiDARData(&collisions);
			}
		}

		globalMetrics.add(localMetrics);

		std::cout << "Iteration: " << ++iteration << "/" << positions.size() << std::endl;
		std::cout << "Number of Points: " << _pointCloud->getNumPoints() << std::endl;
		std::cout << "Ray Building Time: " << localMetrics.getStageTime(PipelineMetrics::RAY_BUILDING) << " microseconds for " << totalRays << " rays." << std::endl;
		std::cout << "LiDAR Response Time: " << localMetrics.getGlobalTime(PipelineMetrics::PREPARE) << " microseconds." << std::endl;

		std::string wlStr = "Results/Paths/TLS/" + std::to_string(iteration) + ".ply";
		_pointCloud->writePLY(wlStr, _scene, false);
		_pointCloud->archive();
	}

	std::cout << "Number of rays: " << totalRays << std::endl;
	std::cout << globalMetrics << std::endl;
	std::cout << "Average Ray Building: " << globalMetrics.getGlobalTime(PipelineMetrics::RAY_BUILDING, PipelineMetrics::RAY_BUILDING) / float(LIDAR_PARAMS._numExecs) << std::endl;
	std::cout << "Average Simulation: " << globalMetrics.getGlobalTime(PipelineMetrics::PREPARE) / float(LIDAR_PARAMS._numExecs) << std::endl;

	this->releaseLiDARData();
	RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->freeContext();

	if (POINT_CLOUD_PARAMS._savePointCloud)
	{
		_pointCloud->writePLY(std::string(POINT_CLOUD_PARAMS._filenameBuffer), _scene, true);
	}
}

void LiDARSimulation::launchSingleSimulation(bool instantiateRaysVAO)
{
	PipelineMetrics globalMetrics;
	AABB aabb = _scene->getAABB();
	GLuint raySSBO, numRays, totalRays = 0, numExecs = LIDAR_PARAMS._numExecs + int(LIDAR_PARAMS._discardFirstExecution), iteration = 0;
	long long time = 0;
	std::vector<Model3D::TriangleCollisionGPUData> collisions;

	// Initialize variables and buffers
	RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->initializeContext(&LIDAR_PARAMS, aabb);
	this->prepareLiDARData(RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->getRayMaxCapacity());
	glFinish();

	ChronoUtilities::getDuration();			// Clean chrono

	for (int wl = LIDAR_PARAMS._wavelength[0]; wl <= LIDAR_PARAMS._wavelength[1]; wl += 1)
	{
		this->prepareMaterialData(wl);
		_pointCloud->archive();

		for (int execIdx = 0; execIdx < numExecs; ++execIdx)
		{
			if (execIdx > 0 || numExecs == 1)
			{
				totalRays = 0;
				PipelineMetrics localMetrics;
				RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->resetPendingRays(&LIDAR_PARAMS);

				while (RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->arePendingRays())
				{
					collisions.clear();

					{
						localMetrics.initChrono();

						RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->buildRays(&LIDAR_PARAMS, aabb);

						localMetrics.measureStage(PipelineMetrics::RAY_BUILDING);
					}

					{
						RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->getRaySSBO(raySSBO, numRays);
						localMetrics.add(this->solveRayIntersection(raySSBO, numRays, collisions, wl, execIdx == 0));
						totalRays += numRays;

						this->appendLiDARData(&collisions);
					}

					//std::string wlStr = "Results/Paths/ALS/" + std::to_string(iteration) + ".ply";
					//_pointCloud->writePLY(wlStr, _scene, false);
					//_pointCloud->archive();

					++iteration;
				}

				globalMetrics.add(localMetrics);

				std::cout << "Number of points: " << _pointCloud->getNumPoints() << std::endl;
				std::cout << "Number of rays: " << totalRays << std::endl;
			}
			else
			{
				RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->resetPendingRays(&LIDAR_PARAMS);

				while (RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->arePendingRays())
				{
					RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->buildRays(&LIDAR_PARAMS, aabb);
					RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->getRaySSBO(raySSBO, numRays);
					this->solveRayIntersection(raySSBO, numRays, collisions, wl, true);

					this->appendLiDARData(&collisions);
				}
			}
		}

		std::cout << globalMetrics << std::endl;
		std::cout << "Average Ray Building: " << globalMetrics.getGlobalTime(PipelineMetrics::RAY_BUILDING, PipelineMetrics::RAY_BUILDING) / float(LIDAR_PARAMS._numExecs) / 1000.0f << " +- " << globalMetrics.getStd(PipelineMetrics::RAY_BUILDING, PipelineMetrics::RAY_BUILDING) / 1000.0f << " ms" << std::endl;
		std::cout << "Average Simulation: " << globalMetrics.getGlobalTime(PipelineMetrics::PREPARE_ATTRIBUTES) / float(LIDAR_PARAMS._numExecs) / 1000.0f << " +- " << globalMetrics.getStd() / 1000.0f << " ms" << std::endl;

		if (POINT_CLOUD_PARAMS._savePointCloud)
		{
			std::string pointCloudStr = std::string(POINT_CLOUD_PARAMS._filenameBuffer);
			const unsigned pointIndex = pointCloudStr.find(".");

			if (pointIndex != std::string::npos)
			{
				pointCloudStr = pointCloudStr.substr(0, pointIndex);
			}

			if (LIDAR_PARAMS._wavelength[0] != LIDAR_PARAMS._wavelength[1])
			{
				pointCloudStr += "_" + std::to_string(wl);
			}

			if (pointCloudStr.find(".") == std::string::npos)
			{
				pointCloudStr += ".ply";

			}

			_pointCloud->writePLY(pointCloudStr, _scene, false);
		}

		this->releaseMaterialData();
	}

	this->releaseLiDARData();
	RAY_BUILDER_APPLICATOR[LIDAR_PARAMS._LiDARType]->freeContext();
}

void LiDARSimulation::prepareLiDARData(GLuint numRays)
{
	_whiteNoiseSSBO	= this->buildWhiteNoiseTexture(NOISE_TEXTURE_SIZE);

	{
		std::vector<float> returnThreshold(LIDAR_PARAMS.MAX_NUMBER_OF_RETURNS);
		for (int returnIdx = 0; returnIdx < returnThreshold.size(); ++returnIdx)
		{
			returnThreshold[returnIdx] = LIDAR_PARAMS._returnThreshold[returnIdx].y;
		}

		_returnThresholdSSBO = ComputeShader::setReadBuffer(returnThreshold, GL_STATIC_DRAW);
	}

	std::vector<float> hermiteCoefficients{
		-LIDAR_PARAMS._hermiteT, 2.0f - LIDAR_PARAMS._hermiteT, LIDAR_PARAMS._hermiteT - 2.0f, LIDAR_PARAMS._hermiteT,
		2.0f * LIDAR_PARAMS._hermiteT, LIDAR_PARAMS._hermiteT - 3.0f, 3.0f - 2.0f * LIDAR_PARAMS._hermiteT, -LIDAR_PARAMS._hermiteT,
		-LIDAR_PARAMS._hermiteT, 0.0f, LIDAR_PARAMS._hermiteT, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f
	};

	{
		_triangleCollisionSSBO	= ComputeShader::setWriteBuffer(Model3D::TriangleCollisionGPUData(), numRays * LIDAR_PARAMS._maxReturns, GL_DYNAMIC_DRAW);
		_collisionSSBO			= ComputeShader::setWriteBuffer(Model3D::TriangleCollisionGPUData(), numRays, GL_DYNAMIC_DRAW);
		_hermiteSSBO			= ComputeShader::setReadBuffer(hermiteCoefficients, GL_STATIC_DRAW);
		_counterSSBO			= ComputeShader::setWriteBuffer(GLuint(), 1, GL_DYNAMIC_DRAW);
		_newCounterSSBO			= ComputeShader::setWriteBuffer(GLuint(), 1, GL_DYNAMIC_DRAW);
	}
}

void LiDARSimulation::prepareMaterialData(GLuint wavelength)
{
	std::vector<float> brdfData;
	std::vector<MaterialDatabase::LiDARMaterialGPUData> materials;

	MaterialDatabase::getInstance()->getMaterialGPUArray(wavelength, materials, brdfData);

	_LiDARMaterialsSSBO = ComputeShader::setReadBuffer(materials, GL_STATIC_DRAW);
	_brdfSSBO = ComputeShader::setReadBuffer(brdfData, GL_STATIC_DRAW);
}

void LiDARSimulation::releaseLiDARData()
{	
	glDeleteBuffers(1, &_returnThresholdSSBO);
	glDeleteBuffers(1, &_whiteNoiseSSBO);
	glDeleteBuffers(1, &_counterSSBO);
	glDeleteBuffers(1, &_triangleCollisionSSBO);
	glDeleteBuffers(1, &_collisionSSBO);
	glDeleteBuffers(1, &_hermiteSSBO);
}

void LiDARSimulation::releaseMaterialData()
{
	glDeleteBuffers(1, &_brdfSSBO);
	glDeleteBuffers(1, &_LiDARMaterialsSSBO);
}

PipelineMetrics LiDARSimulation::solveRayIntersection(GLuint raySSBO, GLuint numRays, std::vector<Model3D::TriangleCollisionGPUData>& collisions, int wl, bool readData)
{
	PipelineMetrics		pipelineMetrics;

	ComputeShader*		prepareDataShader		= ShaderList::getInstance()->getComputeShader(RendEnum::PREPARE_LIDAR_DATA);
	ComputeShader*		findBVHCollisionShader	= ShaderList::getInstance()->getComputeShader(RendEnum::FIND_BVH_COLLISION);
	ComputeShader*		reduceCollisionsShader	= ShaderList::getInstance()->getComputeShader(RendEnum::REDUCE_COLLISIONS);
	ComputeShader*		computeColorShader		= ShaderList::getInstance()->getComputeShader(RendEnum::COMPUTE_POINT_COLOR);
	ComputeShader*		updateReturnsShader		= ShaderList::getInstance()->getComputeShader(RendEnum::UPDATE_COLLISION_RETURNS);
	ComputeShader*		addOutliersShader		= ShaderList::getInstance()->getComputeShader(RendEnum::ADD_OUTLIER_SHADER);

	LiDARParameters*	lidarParams				= LiDARSimulation::getLiDARParams();
	unsigned			numCollisions			= 0;
	unsigned			previousCollisions		= 0;
	unsigned			newCollisions			= 0;
	unsigned			idReturn				= 0;
	unsigned			bathymetric				= unsigned(wl < 533) && lidarParams->_LiDARType != LiDARParameters::TERRESTRIAL_SPHERICAL;
	const unsigned		clusterSize				= _groupGPUData->_numTriangles * 2 - 1;
	const unsigned		numGroups				= ComputeShader::getNumGroups(numRays);
	const unsigned		numGroupsPulse			= ComputeShader::getNumGroups(numRays / lidarParams->_raysPulse);
	unsigned			currentNumRays			= numRays;
	unsigned			actualNumRays			= numRays / lidarParams->_raysPulse;

	{
		pipelineMetrics.initChrono();

		numCollisions = newCollisions = previousCollisions = 0;
		ComputeShader::updateReadBuffer(_counterSSBO, &numCollisions, 1, GL_DYNAMIC_DRAW);

		pipelineMetrics.measureStage(PipelineMetrics::WRITE);

		// 1. Reset LiDAR data and prepare LiDAR rays
		pipelineMetrics.initChrono();

		prepareDataShader->use();
		prepareDataShader->bindBuffers(std::vector<GLuint>{ raySSBO });
		prepareDataShader->setUniform("numRays", currentNumRays);
		prepareDataShader->setUniform("numRaysPulse", GLuint(lidarParams->_raysPulse));
		prepareDataShader->setUniform("peakPower", LIDAR_PARAMS._peakPower);
		prepareDataShader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		pipelineMetrics.measureStage(PipelineMetrics::PREPARE);

		do
		{
			// 2. Find collision of each ray with BVH
			pipelineMetrics.initChrono();

			findBVHCollisionShader->use();
			findBVHCollisionShader->bindBuffers(std::vector<GLuint>{
					_groupGPUData->_clusterSSBO, _groupGPUData->_groupGeometrySSBO, _groupGPUData->_groupTopologySSBO,
					_groupGPUData->_groupMeshSSBO, raySSBO, _collisionSSBO
			});
			findBVHCollisionShader->setUniform("numClusters", clusterSize);
			findBVHCollisionShader->setUniform("numRays", currentNumRays);
			findBVHCollisionShader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

			pipelineMetrics.measureStage(PipelineMetrics::FIND_COLLISION);

			// 3. Reduce collisions, as some of them may are generated by the same pulse
			pipelineMetrics.initChrono();

			reduceCollisionsShader->use();
			reduceCollisionsShader->bindBuffers(std::vector<GLuint>{
					_groupGPUData->_groupGeometrySSBO, _groupGPUData->_groupTopologySSBO,
					_groupGPUData->_groupMeshSSBO, _LiDARMaterialsSSBO, raySSBO,
					_collisionSSBO, _whiteNoiseSSBO, _triangleCollisionSSBO, _counterSSBO
			});
			reduceCollisionsShader->setUniform("bathymetric", bathymetric);
			reduceCollisionsShader->setUniform("inducedTerrainError", unsigned(LIDAR_PARAMS._includeTerrainInducedError));
			reduceCollisionsShader->setUniform("lossAddCoefficient", LIDAR_PARAMS._addCoefficient);
			reduceCollisionsShader->setUniform("lossMultCoefficient", LIDAR_PARAMS._multCoefficient);
			reduceCollisionsShader->setUniform("lossPower", LIDAR_PARAMS._lossPower);
			reduceCollisionsShader->setUniform("lossThreshold", LIDAR_PARAMS._zeroThreshold);
			reduceCollisionsShader->setUniform("maxDistance", LIDAR_PARAMS._maxRange);
			reduceCollisionsShader->setUniform("maxDistanceBoundary", LIDAR_PARAMS._maxRangeSoftBoundary);
			reduceCollisionsShader->setUniform("maxReturns", LIDAR_PARAMS._maxReturns);
			reduceCollisionsShader->setUniform("noiseBufferSize", NOISE_TEXTURE_SIZE);
			reduceCollisionsShader->setUniform("numPulses", actualNumRays);
			reduceCollisionsShader->setUniform("numRaysPulse", GLuint(lidarParams->_raysPulse));
			reduceCollisionsShader->setUniform("pulseRadius", lidarParams->_pulseRadius);
			reduceCollisionsShader->setUniform("sensorNormal", (LIDAR_PARAMS._LiDARType == LiDARParameters::TERRESTRIAL_SPHERICAL) ? vec3(1.0f, .0f, 1.0f) : vec3(1.0f, 1.0f, .0f));
			reduceCollisionsShader->setUniform("shinySurfaceError", unsigned(LIDAR_PARAMS._includeShinySurfaceError));
			reduceCollisionsShader->execute(numGroupsPulse, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

			pipelineMetrics.measureStage(PipelineMetrics::REDUCE);

			pipelineMetrics.initChrono();

			numCollisions = *ComputeShader::readData(_counterSSBO, unsigned());
			newCollisions = numCollisions - previousCollisions;
			previousCollisions = numCollisions;

			pipelineMetrics.measureStage(PipelineMetrics::READ);

			pipelineMetrics.initChrono();

			ComputeShader::updateReadBuffer(_newCounterSSBO, &newCollisions, 1, GL_DYNAMIC_DRAW);

			pipelineMetrics.measureStage(PipelineMetrics::WRITE);

			// 4. Include outliers
			if (LIDAR_PARAMS._includeOutliers)
			{
				pipelineMetrics.initChrono();

				addOutliersShader->use();
				addOutliersShader->bindBuffers(std::vector<GLuint>{ raySSBO, _triangleCollisionSSBO, _whiteNoiseSSBO, _counterSSBO, _newCounterSSBO });
				addOutliersShader->setUniform("noiseBufferSize", NOISE_TEXTURE_SIZE);
				addOutliersShader->setUniform("noiseRange", LIDAR_PARAMS._outlierRange);
				addOutliersShader->setUniform("noiseThreshold", LIDAR_PARAMS._outlierThreshold);
				addOutliersShader->setUniform("nullModelCompID", _emptyModelComponent->_id);
				addOutliersShader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

				pipelineMetrics.measureStage(PipelineMetrics::OUTLIERS);

				previousCollisions = *ComputeShader::readData(_counterSSBO, unsigned());
			}
		} while (newCollisions > 1 && (++idReturn) < LIDAR_PARAMS._maxReturns);

		// 5. Compute colors of valid collisions
		pipelineMetrics.initChrono();

		computeColorShader->use();
		computeColorShader->bindBuffers(std::vector<GLuint>{
			_groupGPUData->_groupGeometrySSBO, _groupGPUData->_groupTopologySSBO,
				_groupGPUData->_groupMeshSSBO, _LiDARMaterialsSSBO, raySSBO, _triangleCollisionSSBO, _counterSSBO, _brdfSSBO, _hermiteSSBO
		});
		this->defineSceneUniforms(computeColorShader);
		computeColorShader->setUniform("atmosphericAttenuation", this->getAtmosphericAttenuation());
		computeColorShader->setUniform("bathymetric", bathymetric);
		computeColorShader->setUniform("reflectanceWeight", LIDAR_PARAMS._reflectanceWeight * (bathymetric == 1 ? .5f : 1.0f));
		computeColorShader->setUniform("sensorDiameter", LIDAR_PARAMS._sensorDiameter);
		computeColorShader->setUniform("systemAttenuation", LIDAR_PARAMS._systemAttenuation);
		computeColorShader->setUniform("waterHeight", 1.0f);		// To check
		computeColorShader->execute(numGroupsPulse, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		pipelineMetrics.measureStage(PipelineMetrics::INTENSITY);

		// 6. Update returns
		pipelineMetrics.initChrono();

		updateReturnsShader->use();
		updateReturnsShader->bindBuffers(std::vector<GLuint>{ raySSBO, _triangleCollisionSSBO, _counterSSBO });
		updateReturnsShader->execute(numGroupsPulse, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		pipelineMetrics.measureStage(PipelineMetrics::RETURNS);

		if (readData && numCollisions > 0)
		{
			// Dump GPU data into CPU
			pipelineMetrics.initChrono();

			numCollisions = *ComputeShader::readData(_counterSSBO, unsigned());
			Model3D::TriangleCollisionGPUData* pCollision = ComputeShader::readData(_triangleCollisionSSBO, Model3D::TriangleCollisionGPUData());

			pipelineMetrics.measureStage(PipelineMetrics::READ);

			collisions.insert(collisions.end(), pCollision, pCollision + numCollisions);
		}
	}

	return pipelineMetrics;
}