#include "stdafx.h"
#include "ForestEditorNode.h"

#include <bitset>
#include "Graphics/Core/Terrain.h"

/// [Static methods]

const char* ForestEditorNode::_forestNodeName[] = { "Scene Root", "Simplex Noise", "Terrain", "Forest", "Grass", "Water", "Watcher", "Transmission Tower" };

ForestEditorNode::NodeImagePathMap	ForestEditorNode::_nodeImagePath;
ForestEditorNode::NodeApplicatorMap ForestEditorNode::_nodeApplicator;
ForestEditorNode::TextureMap		ForestEditorNode::_nodeImage;
ForestEditorNode::SceneDescription	ForestEditorNode::_sceneDescription;

const uvec2		ForestEditorNode::MAX_TERRAIN_SUBDIVISIONS = uvec2(1000, 1000);
const uvec2		ForestEditorNode::MIN_TERRAIN_SUBDIVISIONS = uvec2(100, 100);
const vec2		ForestEditorNode::MAX_TERRAIN_SIZE = vec2(300.0f, 300.0f);
const vec2		ForestEditorNode::MIN_TERRAIN_SIZE = vec2(5.0f, 5.0f);
const float		ForestEditorNode::MAX_TERRAIN_DISPLACEMENT = 10.0f;
const float		ForestEditorNode::MIN_TERRAIN_DISPLACEMENT = .1f;
const float		ForestEditorNode::MAX_TERRAIN_HEIGHT = 50.0f;
const float		ForestEditorNode::MIN_TERRAIN_HEIGHT = 1.0f;

const float		ForestEditorNode::MAX_NOISE_FREQUENCY = 0.01f;
const float		ForestEditorNode::MIN_NOISE_FREQUENCY = 0.0001f;
const float		ForestEditorNode::MAX_NOISE_GAIN = 0.2;
const float		ForestEditorNode::MIN_NOISE_GAIN = 1.0f;
const float		ForestEditorNode::MAX_NOISE_LACUNARITY = 1.5f;
const float		ForestEditorNode::MIN_NOISE_LACUNARITY = 2.6f;
const GLuint	ForestEditorNode::MAX_NOISE_OCTAVES = 10;
const GLuint	ForestEditorNode::MIN_NOISE_OCTAVES = 2;
const GLuint	ForestEditorNode::MAX_MAP_SIZE = 800;
const GLuint	ForestEditorNode::MIN_MAP_SIZE = 100;

const float		ForestEditorNode::MAX_TEXTURE_SCALE = 50.0f;
const float		ForestEditorNode::MIN_TEXTURE_SCALE = .2f;

const float		ForestEditorNode::MAX_EROSION_FACTOR = 10.0f;
const float		ForestEditorNode::MIN_EROSION_FACTOR = .01f;
const float		ForestEditorNode::MAX_EROSION_PARTICLES = 1000;
const float		ForestEditorNode::MIN_EROSION_PARTICLES = 500000;
const float		ForestEditorNode::MAX_EROSION_LIFETIME = 100;
const float		ForestEditorNode::MIN_EROSION_LIFETIME = 1;
const GLuint	ForestEditorNode::MAX_EROSION_BRUSH_RADIUS = 10;
const GLuint	ForestEditorNode::MIN_EROSION_BRUSH_RADIUS = 1;

const uvec2		ForestEditorNode::MAX_WATER_SUBDIVISIONS = uvec2(500, 500);
const uvec2		ForestEditorNode::MIN_WATER_SUBDIVISIONS = uvec2(100, 100);
const float		ForestEditorNode::MAX_WATER_HEIGHT = 0.0f;
const float		ForestEditorNode::MIN_WATER_HEIGHT = -10.0f;
const float		ForestEditorNode::MAX_WATER_VARIANCE = .2f;
const float		ForestEditorNode::MIN_WATER_VARIANCE = .0f;
const GLuint	ForestEditorNode::MAX_NUM_LAKES = 10;
const GLuint	ForestEditorNode::MIN_NUM_LAKES = 0;

const GLuint	ForestEditorNode::MAX_TREES = 3000;
const GLuint	ForestEditorNode::MIN_TREES = 0;
const float		ForestEditorNode::MAX_TREE_SCALE = .4f;
const float		ForestEditorNode::MIN_TREE_SCALE = .01f;

const GLuint	ForestEditorNode::MAX_MODELS = 5;
const GLuint	ForestEditorNode::MIN_MODELS = 0;
const float		ForestEditorNode::MAX_MODEL_RADIUS = 10.0f;
const float		ForestEditorNode::MIN_MODEL_RADIUS = .0f;

const GLuint	ForestEditorNode::MAX_GRASS_SEEDS = 3000000;
const GLuint	ForestEditorNode::MIN_GRASS_SEEDS = 0;
const float		ForestEditorNode::MAX_GRASS_SCALE = .01f;
const float		ForestEditorNode::MIN_GRASS_SCALE = .001f;
const float		ForestEditorNode::MAX_SLOPE_WEIGHT = 30.0f;
const float		ForestEditorNode::MIN_SLOPE_WEIGHT = .1f;

const float		ForestEditorNode::MAX_NORMALIZED_VALUE = 1.0f;
const float		ForestEditorNode::MIN_NORMALIZED_VALUE = .0f;

/// [Public methods]

ForestEditorNode::ForestEditorNode(ForestNodeType type) : _nodeType(type)
{
	TerrainParameters* terrainParameters = Terrain::getTerrainParameters();

	_sceneDescription = *terrainParameters;
}

ForestEditorNode::~ForestEditorNode()
{
}

void ForestEditorNode::drawNode()
{
	_nodeApplicator[_nodeType]->drawNode(this);
}

void ForestEditorNode::updateLinks(LinkMap& links)
{
	_nodeApplicator[_nodeType]->updateLinks(links);
}

void ForestEditorNode::initializeTextures()
{
	ForestEditorNode::buildImagePathMap();
	ForestEditorNode::buildApplicatorMap();
	ForestEditorNode::loadTextures();
}

bool ForestEditorNode::saveDescriptionToBinary(ForestEditorNode::LinkMap& linkMap)
{
	std::ofstream fout(TerrainParameters::BINARY_LOCATION, std::ios::out | std::ios::binary);
	if (!fout.is_open()) return false;

	SceneDescription description = _sceneDescription;
	ForestEditorNode::buildSceneDescriptionFromEditor(linkMap, description);
	fout.write((char*)&description, sizeof(SceneDescription));
	fout.close();

	return true;
}

/// [Protected methods]

void ForestEditorNode::buildApplicatorMap()
{
	_nodeApplicator = ForestEditorNode::NodeApplicatorMap(ForestNodeType::NUM_NODE_TYPES);

	_nodeApplicator[ForestNodeType::SCENE_ROOT].reset(new SceneRootNodeApplicator);
	_nodeApplicator[ForestNodeType::NOISE_SIMPLEX].reset(new SimplexNodeApplicator);
	_nodeApplicator[ForestNodeType::TERRAIN_SURFACE].reset(new TerrainNodeApplicator);
	_nodeApplicator[ForestNodeType::WATER].reset(new WaterNodeApplicator);
	_nodeApplicator[ForestNodeType::FOREST].reset(new ForestNodeApplicator);
	_nodeApplicator[ForestNodeType::GRASS].reset(new GrassNodeApplicator);
	_nodeApplicator[ForestNodeType::WATCHER_MODEL].reset(new WatcherNodeApplicator);
	_nodeApplicator[ForestNodeType::TRANSMISSION_TOWER_MODEL].reset(new TransmissionTowerNodeApplicator);
}

void ForestEditorNode::buildImagePathMap()
{
	_nodeImagePath = ForestEditorNode::NodeImagePathMap(ForestNodeType::NUM_NODE_TYPES);

	_nodeImagePath[ForestNodeType::SCENE_ROOT]					= "";
	_nodeImagePath[ForestNodeType::NOISE_SIMPLEX]				= "Assets/Images/SimplexTerrain-LowRes.png";
	_nodeImagePath[ForestNodeType::TERRAIN_SURFACE]				= "Assets/Images/Terrain-LowRes.png";
	_nodeImagePath[ForestNodeType::WATER]						= "Assets/Images/Water-LowRes.png";
	_nodeImagePath[ForestNodeType::FOREST]						= "Assets/Images/Trees-LowRes.png";
	_nodeImagePath[ForestNodeType::GRASS]						= "Assets/Images/Grass-LowRes.png";
	_nodeImagePath[ForestNodeType::WATCHER_MODEL]				= "Assets/Images/Watcher-LowRes.png";
	_nodeImagePath[ForestNodeType::TRANSMISSION_TOWER_MODEL]	= "Assets/Images/TransmissionTower-LowRes.png";
}

void ForestEditorNode::buildSceneDescriptionFromEditor(ForestEditorNode::LinkMap& linkMap, SceneDescription& newDescription)
{
	TerrainParameters* terrainParameters = Terrain::getTerrainParameters();

	newDescription._terrainSubdivisions = isLinkCreated(SCENE_ROOT, TERRAIN_SURFACE, linkMap) ? _sceneDescription._terrainSubdivisions : terrainParameters->_terrainSubdivisions;
	newDescription._terrainSize = isLinkCreated(SCENE_ROOT, TERRAIN_SURFACE, linkMap) ? _sceneDescription._terrainSize : terrainParameters->_terrainSize;
	newDescription._terrainMaxHeight = isLinkCreated(SCENE_ROOT, TERRAIN_SURFACE, linkMap) ? _sceneDescription._terrainMaxHeight : terrainParameters->_terrainMaxHeight;

	newDescription._frequency = isLinkCreated(SCENE_ROOT, TERRAIN_SURFACE, linkMap) && isLinkCreated(TERRAIN_SURFACE, NOISE_SIMPLEX, linkMap) ? _sceneDescription._frequency : terrainParameters->_frequency;
	newDescription._gain = isLinkCreated(SCENE_ROOT, TERRAIN_SURFACE, linkMap) && isLinkCreated(TERRAIN_SURFACE, NOISE_SIMPLEX, linkMap) ? _sceneDescription._gain : terrainParameters->_gain;
	newDescription._lacunarity = isLinkCreated(SCENE_ROOT, TERRAIN_SURFACE, linkMap) && isLinkCreated(TERRAIN_SURFACE, NOISE_SIMPLEX, linkMap) ? _sceneDescription._lacunarity : terrainParameters->_lacunarity;
	newDescription._octaves = isLinkCreated(SCENE_ROOT, TERRAIN_SURFACE, linkMap) && isLinkCreated(TERRAIN_SURFACE, NOISE_SIMPLEX, linkMap) ? _sceneDescription._octaves : terrainParameters->_octaves;

	newDescription._numGrassSeeds = isLinkCreated(SCENE_ROOT, GRASS, linkMap) ? _sceneDescription._numGrassSeeds : terrainParameters->_numGrassSeeds;

	newDescription._waterSubdivisions = isLinkCreated(SCENE_ROOT, WATER, linkMap) ? _sceneDescription._waterSubdivisions : terrainParameters->_waterSubdivisions;
	newDescription._waterHeight = isLinkCreated(SCENE_ROOT, WATER, linkMap) ? _sceneDescription._waterHeight : terrainParameters->_waterHeight;
	newDescription._numLakes = isLinkCreated(SCENE_ROOT, WATER, linkMap) ? _sceneDescription._numLakes : terrainParameters->_numLakes > 0;

	newDescription._numWatchers = isLinkCreated(SCENE_ROOT, WATCHER_MODEL, linkMap) ? _sceneDescription._numWatchers : terrainParameters->_numWatchers;

	newDescription._numTransmissionTowers = isLinkCreated(SCENE_ROOT, TRANSMISSION_TOWER_MODEL, linkMap) ? _sceneDescription._numTransmissionTowers : terrainParameters->_numTransmissionTowers > 0;

	for (int modelIdx = 0; modelIdx < terrainParameters->NUM_TREES; ++modelIdx)
	{
		newDescription._numTrees[modelIdx] = isLinkCreated(SCENE_ROOT, FOREST, linkMap) ? _sceneDescription._numTrees[modelIdx] : terrainParameters->_numTrees[modelIdx];
	}
}

bool ForestEditorNode::eraseLink(ForestNodeType nodeType1, ForestNodeType nodeType2, LinkMap& linkMap)
{
	int outputPin = TerrainNodeApplicator::getAttributeId(nodeType1, nodeType2, true);
	auto it = linkMap.begin();

	while (it != linkMap.end())
	{
		if (it->first == outputPin) 
		{
			it = linkMap.erase(it); return true;
		}
		else
		{
			++it;
		}
	}

	return false;
}

bool ForestEditorNode::isLinkCreated(ForestNodeType nodeType1, ForestNodeType nodeType2, LinkMap& linkMap)
{
	int outputPin = TerrainNodeApplicator::getAttributeId(nodeType1, nodeType2, true);

	for (auto& link: linkMap)
	{
		if (link.first == outputPin) return true;
	}

	return false;
}

void ForestEditorNode::loadTextures()
{
	_nodeImage = ForestEditorNode::TextureMap(ForestNodeType::NUM_NODE_TYPES);
	
	for (int nodeIdx = 0; nodeIdx < NUM_NODE_TYPES; ++nodeIdx)
	{
		if (!_nodeImagePath[nodeIdx].empty())
		{
			Image image(ForestEditorNode::_nodeImagePath[nodeIdx]);
			image.flipImageVertically();
			_nodeImage[nodeIdx].reset(new Texture(&image, GL_CLAMP, GL_CLAMP, GL_LINEAR, GL_LINEAR));
		}
	}
}

/// [Forest Node Applicator]

// Base class

void ForestEditorNode::ForestEditorNodeApplicator::leaveSpace(const unsigned space)
{
	for (int i = 0; i < space; ++i)
	{
		ImGui::Spacing();
	}
}

// Root

void ForestEditorNode::SceneRootNodeApplicator::drawNode(ForestEditorNode* node)
{
	imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar, IM_COL32(210, 0, 0, 255));
	imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarSelected, IM_COL32(255, 0, 0, 255));
	imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarHovered, IM_COL32(255, 0, 0, 255));
	
	imnodes::BeginNode(node->_nodeType);
	
	imnodes::BeginNodeTitleBar();
	ImGui::TextUnformatted(ForestEditorNode::_forestNodeName[node->_nodeType]);
	imnodes::EndNodeTitleBar();

	{
		imnodes::BeginOutputAttribute(getAttributeId(node->_nodeType, TERRAIN_SURFACE, true));
		ImGui::Text("Terrain");
		imnodes::EndOutputAttribute();

		imnodes::BeginOutputAttribute(getAttributeId(node->_nodeType, WATER, true));
		ImGui::Text("Water");
		imnodes::EndOutputAttribute();

		imnodes::BeginOutputAttribute(getAttributeId(node->_nodeType, FOREST, true));
		ImGui::Text("Forest");
		imnodes::EndOutputAttribute();

		imnodes::BeginOutputAttribute(getAttributeId(node->_nodeType, GRASS, true));
		ImGui::Text("Grass");
		imnodes::EndOutputAttribute();

		imnodes::BeginOutputAttribute(getAttributeId(node->_nodeType, WATCHER_MODEL, true));
		ImGui::Text("Watcher");
		imnodes::EndOutputAttribute();

		imnodes::BeginOutputAttribute(getAttributeId(node->_nodeType, TRANSMISSION_TOWER_MODEL, true));
		ImGui::Text("Transmission Tower");
		imnodes::EndOutputAttribute();

	}

	imnodes::EndNode();

	imnodes::PopColorStyle();
	imnodes::PopColorStyle();
	imnodes::PopColorStyle();
}

int ForestEditorNode::ForestEditorNodeApplicator::getAttributeId(const unsigned nodeType, const unsigned pinType, const bool output)
{
	int nodeId = nodeType * NUM_NODE_TYPES * 2;
	nodeId += pinType + NUM_NODE_TYPES * int(output);

	return nodeId;
}

void ForestEditorNode::SceneRootNodeApplicator::updateLinks(LinkMap& links)
{
	int outputPin, outputPinCopy, startPin, startPinCopy;
	
	for (int outputNodeIdx = TERRAIN_SURFACE; outputNodeIdx < NUM_NODE_TYPES; ++outputNodeIdx)
	{
		outputPin = outputPinCopy = getAttributeId(SCENE_ROOT, outputNodeIdx, true);
		startPin = startPinCopy = getAttributeId(outputNodeIdx, SCENE_ROOT, false);
		
		if (imnodes::IsLinkCreated(&outputPin, &startPin) && (outputPin == outputPinCopy && startPin == startPinCopy))
		{
			links.push_back(std::make_pair(outputPin, startPin));
		}
	}
}

// Simplex Noise

void ForestEditorNode::SimplexNodeApplicator::drawNode(ForestEditorNode* node)
{
	imnodes::BeginNode(node->_nodeType);

	imnodes::BeginNodeTitleBar();
	ImGui::TextUnformatted(ForestEditorNode::_forestNodeName[node->_nodeType]);
	imnodes::EndNodeTitleBar();

	ImGui::Image((void*)_nodeImage[node->_nodeType]->getID(), ImVec2(150, 83));

	ImGui::PushItemWidth(80.0f);
	{
		this->leaveSpace(1);

		imnodes::BeginStaticAttribute(0);

		ImGui::SliderScalar("Frequency", ImGuiDataType_Float, &_sceneDescription._frequency, &MIN_NOISE_FREQUENCY, &MAX_NOISE_FREQUENCY);
		ImGui::SliderScalar("Gain", ImGuiDataType_Float, &_sceneDescription._gain, &MIN_NOISE_GAIN, &MAX_NOISE_GAIN);
		ImGui::SliderScalar("Lacunarity", ImGuiDataType_Float, &_sceneDescription._lacunarity, &MIN_NOISE_LACUNARITY, &MAX_NOISE_LACUNARITY);
		ImGui::SliderScalar("Octaves", ImGuiDataType_U16, &_sceneDescription._octaves, &MIN_NOISE_OCTAVES, &MAX_NOISE_OCTAVES);
		ImGui::SliderScalar("Map Size", ImGuiDataType_U16, &_sceneDescription._mapSize, &MIN_MAP_SIZE, &MAX_MAP_SIZE);

		imnodes::EndStaticAttribute();

		imnodes::BeginInputAttribute(getAttributeId(node->_nodeType, TERRAIN_SURFACE, false));
		imnodes::EndInputAttribute();
	}

	imnodes::EndNode();
}

// Terrain

void ForestEditorNode::TerrainNodeApplicator::drawNode(ForestEditorNode* node)
{
	imnodes::BeginNode(node->_nodeType);

	imnodes::BeginNodeTitleBar();
	ImGui::TextUnformatted(ForestEditorNode::_forestNodeName[node->_nodeType]);
	imnodes::EndNodeTitleBar();

	ImGui::Image((void*)_nodeImage[node->_nodeType]->getID(), ImVec2(200, 84));

	ImGui::PushItemWidth(80.0f);
	{
		this->leaveSpace(1);

		imnodes::BeginStaticAttribute(0);

		ImGui::SliderScalar("Horizontal Subdiv.", ImGuiDataType_U16, &_sceneDescription._terrainSubdivisions.x, &MIN_TERRAIN_SUBDIVISIONS.x, &MAX_TERRAIN_SUBDIVISIONS.x);
		ImGui::SliderScalar("Vertical Subdiv.", ImGuiDataType_U16, &_sceneDescription._terrainSubdivisions.y, &MIN_TERRAIN_SUBDIVISIONS.y, &MAX_TERRAIN_SUBDIVISIONS.y);
		ImGui::SliderScalar("Extrusion", ImGuiDataType_Float, &_sceneDescription._terrainMaxHeight, &MIN_TERRAIN_DISPLACEMENT, &MAX_TERRAIN_DISPLACEMENT);
		ImGui::SliderScalar("Size (x)", ImGuiDataType_Float, &_sceneDescription._terrainSize.x, &MIN_TERRAIN_SIZE, &MAX_TERRAIN_SIZE);
		ImGui::SliderScalar("Size (y)", ImGuiDataType_Float, &_sceneDescription._terrainSize.y, &MIN_TERRAIN_SIZE, &MAX_TERRAIN_SIZE);

		ImGui::Spacing();

		if (ImGui::TreeNode("Texturing"))
		{
			ImGui::SliderScalar("Grass Texture Scale", ImGuiDataType_Float, &_sceneDescription._grassTextureScale, &MIN_TEXTURE_SCALE, &MAX_TEXTURE_SCALE);
			ImGui::SliderScalar("Rock Texture Scale", ImGuiDataType_Float, &_sceneDescription._rockTextureScale, &MIN_TEXTURE_SCALE, &MAX_TEXTURE_SCALE);
			ImGui::SliderScalar("Snow Texture Scale", ImGuiDataType_Float, &_sceneDescription._snowTextureScale, &MIN_TEXTURE_SCALE, &MAX_TEXTURE_SCALE);
			ImGui::SliderScalar("Rock Weight", ImGuiDataType_Float, &_sceneDescription._rockWeightFactor, &MIN_NORMALIZED_VALUE, &MAX_NORMALIZED_VALUE);

			ImGui::TreePop();
		}

		ImGui::Spacing();

		if (ImGui::TreeNode("Erosion"))
		{
			ImGui::SliderScalar("Deposit Speed", ImGuiDataType_Float, &_sceneDescription._depositSpeed, &MIN_EROSION_FACTOR, &MAX_EROSION_FACTOR);
			ImGui::SliderScalar("Erode Speed", ImGuiDataType_Float, &_sceneDescription._erodeSpeed, &MIN_EROSION_FACTOR, &MAX_EROSION_FACTOR);
			ImGui::SliderScalar("Erosion Brush Radius", ImGuiDataType_U16, &_sceneDescription._erosionBrushRadius, &MIN_EROSION_BRUSH_RADIUS, &MAX_EROSION_BRUSH_RADIUS);
			ImGui::SliderScalar("Particles", ImGuiDataType_Float, &_sceneDescription._numErosionIterations, &MIN_EROSION_PARTICLES, &MAX_EROSION_PARTICLES);
			ImGui::SliderScalar("Evaporate Speed", ImGuiDataType_Float, &_sceneDescription._evaporateSpeed, &MIN_EROSION_FACTOR, &MAX_EROSION_FACTOR);
			ImGui::SliderScalar("Gravity", ImGuiDataType_Float, &_sceneDescription._gravity, &MIN_EROSION_FACTOR, &MAX_EROSION_FACTOR);
			ImGui::SliderScalar("Inertia", ImGuiDataType_Float, &_sceneDescription._inertia, &MIN_EROSION_FACTOR, &MAX_EROSION_FACTOR);
			ImGui::SliderScalar("Max. Lifetime", ImGuiDataType_Float, &_sceneDescription._maxLifetime, &MIN_EROSION_LIFETIME, &MAX_EROSION_LIFETIME);
			ImGui::SliderScalar("Min. Sediment Capacity", ImGuiDataType_Float, &_sceneDescription._minSedimentCapacity, &MIN_EROSION_FACTOR, &MAX_EROSION_FACTOR);
			ImGui::SliderScalar("Sediment Cap. Factor", ImGuiDataType_Float, &_sceneDescription._sedimentCapacityFactor, &MIN_EROSION_FACTOR, &MAX_EROSION_FACTOR);
			ImGui::SliderScalar("Start Speed", ImGuiDataType_Float, &_sceneDescription._startSpeed, &MIN_EROSION_FACTOR, &MAX_EROSION_FACTOR);
			ImGui::SliderScalar("Start Water", ImGuiDataType_Float, &_sceneDescription._startWater, &MIN_EROSION_FACTOR, &MAX_EROSION_FACTOR);

			ImGui::TreePop();
		}

		imnodes::EndStaticAttribute();

		imnodes::BeginInputAttribute(getAttributeId(node->_nodeType, SCENE_ROOT, false));
		imnodes::EndInputAttribute();

		imnodes::BeginOutputAttribute(getAttributeId(node->_nodeType, NOISE_SIMPLEX, true));
		
		ImGui::Text("Noise");
		
		imnodes::EndOutputAttribute();
	}

	imnodes::EndNode();
}

void ForestEditorNode::TerrainNodeApplicator::updateLinks(LinkMap& links)
{
	int outputPin, outputPinCopy, startPin, startPinCopy;

	for (int outputNodeIdx = NOISE_SIMPLEX; outputNodeIdx < TERRAIN_SURFACE; ++outputNodeIdx)
	{
		startPin = startPinCopy = getAttributeId(outputNodeIdx, TERRAIN_SURFACE, false);
		outputPin = outputPinCopy = getAttributeId(TERRAIN_SURFACE, outputNodeIdx, true);

		if (imnodes::IsLinkCreated(&outputPin, &startPin) && startPin == startPinCopy)
		{
			links.push_back(std::make_pair(outputPin, startPin));
		}
	}
}

// Water

void ForestEditorNode::WaterNodeApplicator::drawNode(ForestEditorNode* node)
{
	imnodes::BeginNode(node->_nodeType);

	imnodes::BeginNodeTitleBar();
	ImGui::TextUnformatted(ForestEditorNode::_forestNodeName[node->_nodeType]);
	imnodes::EndNodeTitleBar();

	ImGui::Image((void*)_nodeImage[node->_nodeType]->getID(), ImVec2(200, 110));

	ImGui::PushItemWidth(80.0f);
	{
		this->leaveSpace(1);

		imnodes::BeginStaticAttribute(0);

		ImGui::SliderScalar("Subdiv./m", ImGuiDataType_U16, &_sceneDescription._waterSubdivisions, &MIN_WATER_SUBDIVISIONS.x, &MAX_WATER_SUBDIVISIONS.x);
		ImGui::SliderScalar("Ref. Height", ImGuiDataType_Float, &_sceneDescription._waterHeight, &MIN_WATER_HEIGHT, &MAX_WATER_HEIGHT);
		ImGui::SliderScalar("Height Variance", ImGuiDataType_Float, &_sceneDescription._waterHeightVariance, &MIN_WATER_HEIGHT, &MAX_WATER_HEIGHT);
		ImGui::SliderScalar("Max. Num. Lakes", ImGuiDataType_U16, &_sceneDescription._numLakes, &MIN_NUM_LAKES, &MAX_NUM_LAKES);

		imnodes::EndStaticAttribute();

		imnodes::BeginInputAttribute(getAttributeId(node->_nodeType, SCENE_ROOT, false));
		imnodes::EndInputAttribute();
	}

	imnodes::EndNode();
}

// Forest

void ForestEditorNode::ForestNodeApplicator::drawNode(ForestEditorNode* node)
{
	imnodes::BeginNode(node->_nodeType);

	imnodes::BeginNodeTitleBar();
	ImGui::TextUnformatted(ForestEditorNode::_forestNodeName[node->_nodeType]);
	imnodes::EndNodeTitleBar();

	ImGui::Image((void*)_nodeImage[node->_nodeType]->getID(), ImVec2(160, 154));

	ImGui::PushItemWidth(80.0f);
	{
		this->leaveSpace(1);

		imnodes::BeginStaticAttribute(0);

		ImGui::SliderScalar("Num. Trees (1)", ImGuiDataType_U16, &_sceneDescription._numTrees[0], &MIN_TREES, &MAX_TREES);
		ImGui::SliderScalar("Num. Trees (2)", ImGuiDataType_U16, &_sceneDescription._numTrees[1], &MIN_TREES, &MAX_TREES);
		ImGui::SliderScalar("Num. Trees (3)", ImGuiDataType_U16, &_sceneDescription._numTrees[2], &MIN_TREES, &MAX_TREES);
		ImGui::SliderScalar("Min. Scale (1)", ImGuiDataType_Float, &_sceneDescription._minTreeScale[0], &MIN_TREE_SCALE, &_sceneDescription._maxTreeScale[0]);
		ImGui::SliderScalar("Min. Scale (2)", ImGuiDataType_Float, &_sceneDescription._minTreeScale[1], &MIN_TREE_SCALE, &_sceneDescription._maxTreeScale[1]);
		ImGui::SliderScalar("Min. Scale (3)", ImGuiDataType_Float, &_sceneDescription._minTreeScale[2], &MIN_TREE_SCALE, &_sceneDescription._maxTreeScale[2]);
		ImGui::SliderScalar("Max. Scale (1)", ImGuiDataType_Float, &_sceneDescription._maxTreeScale[0], &_sceneDescription._minTreeScale[0], &MAX_TREE_SCALE);
		ImGui::SliderScalar("Max. Scale (2)", ImGuiDataType_Float, &_sceneDescription._maxTreeScale[1], &_sceneDescription._minTreeScale[1], &MAX_TREE_SCALE);
		ImGui::SliderScalar("Max. Scale (3)", ImGuiDataType_Float, &_sceneDescription._maxTreeScale[2], &_sceneDescription._minTreeScale[2], &MAX_TREE_SCALE);

		imnodes::EndStaticAttribute();

		imnodes::BeginInputAttribute(getAttributeId(node->_nodeType, SCENE_ROOT, false));
		imnodes::EndInputAttribute();
	}

	imnodes::EndNode();
}

// Grass

void ForestEditorNode::GrassNodeApplicator::drawNode(ForestEditorNode* node)
{
	imnodes::BeginNode(node->_nodeType);

	imnodes::BeginNodeTitleBar();
	ImGui::TextUnformatted(ForestEditorNode::_forestNodeName[node->_nodeType]);
	imnodes::EndNodeTitleBar();

	ImGui::Image((void*)_nodeImage[node->_nodeType]->getID(), ImVec2(160, 100));

	ImGui::PushItemWidth(80.0f);
	{
		this->leaveSpace(1);

		imnodes::BeginStaticAttribute(0);

		ImGui::SliderScalar("Num. Seeds", ImGuiDataType_U32, &_sceneDescription._numGrassSeeds, &MIN_GRASS_SEEDS, &MAX_GRASS_SEEDS);
		ImGui::SliderScalar("Scale", ImGuiDataType_Float, &_sceneDescription._grassScale, &MIN_GRASS_SCALE, &MAX_GRASS_SCALE);
		ImGui::SliderScalar("Height Weight", ImGuiDataType_Float, &_sceneDescription._grassHeightWeight, &MIN_NORMALIZED_VALUE, &MAX_NORMALIZED_VALUE);
		ImGui::SliderScalar("Slope Weight", ImGuiDataType_Float, &_sceneDescription._grassSlopeWeight, &MIN_SLOPE_WEIGHT, &MAX_SLOPE_WEIGHT);
		ImGui::SliderFloat2("Height Interval", &_sceneDescription._grassHeightThreshold[0], MIN_NORMALIZED_VALUE, MAX_NORMALIZED_VALUE);
		ImGui::SliderScalar("Threshold", ImGuiDataType_Float, &_sceneDescription._grassThreshold, &MIN_GRASS_SCALE, &MAX_GRASS_SCALE);

		imnodes::EndStaticAttribute();

		imnodes::BeginInputAttribute(getAttributeId(node->_nodeType, SCENE_ROOT, false));
		imnodes::EndInputAttribute();
	}

	imnodes::EndNode();
}

// Watcher

void ForestEditorNode::WatcherNodeApplicator::drawNode(ForestEditorNode* node)
{
	imnodes::BeginNode(node->_nodeType);

	imnodes::BeginNodeTitleBar();
	ImGui::TextUnformatted(ForestEditorNode::_forestNodeName[node->_nodeType]);
	imnodes::EndNodeTitleBar();

	ImGui::Image((void*)_nodeImage[node->_nodeType]->getID(), ImVec2(120, 140));

	ImGui::PushItemWidth(80.0f);
	{
		this->leaveSpace(1);

		imnodes::BeginStaticAttribute(0);

		ImGui::SliderScalar("Num. Models", ImGuiDataType_U16, &_sceneDescription._numWatchers, &MIN_MODELS, &MAX_MODELS);
		ImGui::SliderScalar("Saturation Radius", ImGuiDataType_Float, &_sceneDescription._watcherRadius, &MIN_MODEL_RADIUS, &MAX_MODEL_RADIUS);

		imnodes::EndStaticAttribute();

		imnodes::BeginInputAttribute(getAttributeId(node->_nodeType, SCENE_ROOT, false));
		imnodes::EndInputAttribute();
	}

	imnodes::EndNode();
}

// Transmission tower

void ForestEditorNode::TransmissionTowerNodeApplicator::drawNode(ForestEditorNode* node)
{
	imnodes::BeginNode(node->_nodeType);

	imnodes::BeginNodeTitleBar();
	ImGui::TextUnformatted(ForestEditorNode::_forestNodeName[node->_nodeType]);
	imnodes::EndNodeTitleBar();

	ImGui::Image((void*)_nodeImage[node->_nodeType]->getID(), ImVec2(120, 154));

	ImGui::PushItemWidth(80.0f);
	{
		this->leaveSpace(1);

		imnodes::BeginStaticAttribute(0);

		ImGui::SliderScalar("Num. Models", ImGuiDataType_U16, &_sceneDescription._numTransmissionTowers, &MIN_MODELS, &MAX_MODELS);
		ImGui::SliderScalar("Saturation Radius", ImGuiDataType_Float, &_sceneDescription._transmissionTowerRadius, &MIN_MODEL_RADIUS, &MAX_MODEL_RADIUS);

		imnodes::EndStaticAttribute();

		imnodes::BeginInputAttribute(getAttributeId(node->_nodeType, SCENE_ROOT, false));
		imnodes::EndInputAttribute();
	}

	imnodes::EndNode();
}