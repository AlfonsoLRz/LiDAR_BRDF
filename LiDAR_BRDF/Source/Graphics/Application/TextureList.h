#pragma once

#include "Graphics/Application/GraphicsAppEnumerations.h"
#include "Graphics/Core/Texture.h"
#include "Utilities/Singleton.h"

/**
*	@file TextureList.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 07/19/2019
*/

/**
*	@brief Database of built-in application textures.
*/
class TextureList: public Singleton<TextureList>
{
	friend class Singleton<TextureList>;

protected:
	static std::unordered_map<uint16_t, std::string> TEXTURE_PATH;						//!< Location of every texture which is not defined by a single color

private:
	struct ColorHash {
		std::size_t operator()(const vec4& color) const
		{
			glm::ivec4 iColor = color * 256.0f;
			return static_cast<size_t>(iColor.x + iColor.y * 256 + iColor.z * 512 + iColor.w + 1024);
		}
	};

	struct ColorEqual {
		bool operator()(const vec4& c1, const vec4& c2) const
		{
			return glm::distance(c1, c2) < glm::epsilon<float>();
		}
	};

protected:
	std::unordered_map<vec4, std::unique_ptr<Texture>, ColorHash, ColorEqual>	_colorTexture;			//!< Location of every texture which is not defined by a single color
	std::unordered_map<std::string, std::unique_ptr<Texture>>					_pathTexture;			//!< Location of every texture which is not defined by a single color
	std::vector<Texture*>														_predefinedTexture;		//!< Only queried textures are initialized

protected:
	/**
	*	@brief Default constructor.
	*/
	TextureList();

	/**
	*	@brief Defines the composition of each texture.
	*/
	void loadTextures();

public:
	/**
	*	@return Texture defined by the index material (TextureNames).
	*/
	Texture* getTexture(const CGAppEnum::TextureNames texture);

	/**
	*	@return Texture defined by a color.
	*/
	Texture* getTexture(const vec4& color);

	/**
	*	@return Texture defined by the texture's path.
	*/
	Texture* getTexture(const std::string& path);
};

