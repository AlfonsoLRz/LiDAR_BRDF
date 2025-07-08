#include "stdafx.h"
#include "TextureList.h"

// [Static members initialization]

std::unordered_map<uint16_t, std::string> TextureList::TEXTURE_PATH
{
	// TERRAIN_SCENE
	{CGAppEnum::TEXTURE_CLAY_ALBEDO, "Assets/Textures/Floor/Clay/Albedo.png"},
	{CGAppEnum::TEXTURE_CLAY_DISP, "Assets/Textures/Floor/Clay/Displacement.png"},
	{CGAppEnum::TEXTURE_CLAY_NORMAL, "Assets/Textures/Floor/Clay/Normal.png"},
	{CGAppEnum::TEXTURE_FIRE_TOWER_ALBEDO, "Assets/Textures/Buildings/Tower/Albedo.png"},
	{CGAppEnum::TEXTURE_FIRE_TOWER_NORMAL, "Assets/Textures/Buildings/Tower/Normal.png"},
	{CGAppEnum::TEXTURE_GRASS_ALBEDO, "Assets/Textures/Floor/Grass/Albedo.png"},
	{CGAppEnum::TEXTURE_GRASS_DISP, "Assets/Textures/Floor/Grass/Displacement.png"},
	{CGAppEnum::TEXTURE_GRASS_NORMAL, "Assets/Textures/Floor/Grass/Normal.png"},
	{CGAppEnum::TEXTURE_LEAVES_01_ALBEDO, "Assets/Textures/Trees/Leaf_Albedo.png"},
	{CGAppEnum::TEXTURE_LEAVES_01_ALPHA, "Assets/Textures/Trees/Leaf_Alpha.png"},
	{CGAppEnum::TEXTURE_LEAVES_01_NORMAL, "Assets/Textures/Trees/Leaf_Normal.png"},
	{CGAppEnum::TEXTURE_SNOW_ALBEDO, "Assets/Textures/Floor/Snow/Albedo.png"},
	{CGAppEnum::TEXTURE_STONE_ALBEDO, "Assets/Textures/Floor/Stone/Albedo.png"},
	{CGAppEnum::TEXTURE_STONE_DISP, "Assets/Textures/Floor/Stone/Displacement.png"},
	{CGAppEnum::TEXTURE_STONE_NORMAL, "Assets/Textures/Floor/Stone/Normal.png"},
	{CGAppEnum::TEXTURE_TRANSMISSION_TOWER_ALBEDO, "Assets/Textures/Buildings/TransmissionTower/MetalAlbedo.png"},
	{CGAppEnum::TEXTURE_TRANSMISSION_TOWER_NORMAL, "Assets/Textures/Buildings/TransmissionTower/MetalNormal.png"},
	{CGAppEnum::TEXTURE_TRANSMISSION_TOWER_SPECULAR, "Assets/Textures/Buildings/TransmissionTower/MetalRoughness.png"},
	{CGAppEnum::TEXTURE_TRUNK_01_ALBEDO, "Assets/Textures/Trees/Trunk_Albedo.png"},
	{CGAppEnum::TEXTURE_TRUNK_01_NORMAL, "Assets/Textures/Trees/Trunk_Normal.png"},
	{CGAppEnum::TEXTURE_TRUNK_01_SPECULAR, "Assets/Textures/Trees/Trunk_Specular.png"},
	{CGAppEnum::TEXTURE_WATER_DUDV_MAP, "Assets/Textures/Water/DuDvMap.png"},
	{CGAppEnum::TEXTURE_WATER_DUDV_NORMAL_MAP, "Assets/Textures/Water/DuDvNormalMap.png"},

	// LiDAR
	{CGAppEnum::TEXTURE_CANISTER_ALBEDO, "Assets/Textures/Simulation/Canister/Albedo.png"},
	{CGAppEnum::TEXTURE_CANISTER_NORMAL, "Assets/Textures/Simulation/Canister/Normal.png"},
	{CGAppEnum::TEXTURE_CANISTER_SPECULAR, "Assets/Textures/Simulation/Canister/Specular.png"},
	{CGAppEnum::TEXTURE_DRONE_ALBEDO, "Assets/Textures/Simulation/Drone_v2/Albedo.png"},
	{CGAppEnum::TEXTURE_DRONE_NORMAL, "Assets/Textures/Simulation/Drone_v2/Normal.png"},
	{CGAppEnum::TEXTURE_DRONE_SPECULAR, "Assets/Textures/Simulation/Drone_v2/Specular.png"},
	{CGAppEnum::TEXTURE_LIDAR_HEIGHT, "Assets/Textures/LiDAR/HeightMap.png"},
	{CGAppEnum::TEXTURE_RETURN_NUMBER, "Assets/Textures/LiDAR/ReturnNumber.png"},
};

/// [Protected methods]

TextureList::TextureList()
	: _predefinedTexture(CGAppEnum::numAvailableTextures())
{
	this->loadTextures();
}

void TextureList::loadTextures()
{
	// Solution to increase flexibility of texture instantiation: define here only those textures which doesn't come from images in file system or use different parameters for
	// magnification / mignification or wrapping S / T 

	_predefinedTexture[CGAppEnum::TEXTURE_BLACK]				= this->getTexture(vec4(0.0f, 0.0f, 0.0f, 1.0f));
	_predefinedTexture[CGAppEnum::TEXTURE_BLUE]					= this->getTexture(vec4(0.35f, 0.35f, 1.0f, 1.0f));
	_predefinedTexture[CGAppEnum::TEXTURE_BLUE_SKY]				= this->getTexture(vec4(0.04f, 0.7f, 0.94f, 1.0f));
	_predefinedTexture[CGAppEnum::TEXTURE_GRAY_64]				= this->getTexture(vec4(0.25f, 0.25f, 0.25f, 1.0f));
	_predefinedTexture[CGAppEnum::TEXTURE_GRAY_127]				= this->getTexture(vec4(0.5f, 0.5f, 0.5f, 1.0f));
	_predefinedTexture[CGAppEnum::TEXTURE_GRAY_192]				= this->getTexture(vec4(0.75f, 0.75f, 0.75f, 1.0f));
	_predefinedTexture[CGAppEnum::TEXTURE_GRAY_240]				= this->getTexture(vec4(0.94f, 0.94f, 0.94f, 1.0f));
	_predefinedTexture[CGAppEnum::TEXTURE_GOLD]					= this->getTexture(vec4(0.751f, 0.606f, 0.226f, 1.0f));
	_predefinedTexture[CGAppEnum::TEXTURE_GREEN]				= this->getTexture(vec4(0.0f, 1.0f, 0.0f, 1.0f));
	_predefinedTexture[CGAppEnum::TEXTURE_RED]					= this->getTexture(vec4(1.0f, 0.0f, 0.0f, 1.0f));
	_predefinedTexture[CGAppEnum::TEXTURE_WATER_LAKE_ALBEDO]	= this->getTexture(vec4(0.45f, 0.48f, 0.5f, 0.46f));
	_predefinedTexture[CGAppEnum::TEXTURE_WEED_PLANT_ALBEDO]	= this->getTexture(vec4(0.2f, 0.42f, 0.1f, 1.0f));
	_predefinedTexture[CGAppEnum::TEXTURE_WHITE]				= this->getTexture(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	_predefinedTexture[CGAppEnum::TEXTURE_YELLOW]				= this->getTexture(vec4(1.0f, 1.0f, 0.0f, 1.0f));
}

/// [Public methods]

Texture* TextureList::getTexture(const CGAppEnum::TextureNames texture)
{
	if (!_predefinedTexture[texture])
	{
		if (TEXTURE_PATH.find(texture) == TEXTURE_PATH.end())
		{
			return nullptr;
		}

		Texture* text = this->getTexture(TEXTURE_PATH[texture]);
		_predefinedTexture[texture] = text;
	}

	return _predefinedTexture[texture];
}

Texture* TextureList::getTexture(const vec4& color)
{
	Texture* texture = nullptr;
	auto colorIt = _colorTexture.find(color);

	if (colorIt == _colorTexture.end())
	{
		texture = new Texture(color);
		_colorTexture[color] = std::unique_ptr<Texture>(texture);
	}
	else
	{
		texture = colorIt->second.get();
	}

	return texture;
}

Texture* TextureList::getTexture(const std::string& path)
{
	Texture* texture = nullptr;
	auto colorIt = _pathTexture.find(path);

	if (colorIt == _pathTexture.end())
	{
		texture = new Texture(path);
		_pathTexture[path] = std::unique_ptr<Texture>(texture);
	}
	else
	{
		texture = colorIt->second.get();
	}

	return texture;
}
