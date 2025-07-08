#pragma once

/**
*	@file MaterialDatabase.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 09/02/2021

*/

#include "Graphics/Core/BRDFDatabase.h"
#include "Graphics/Core/Image.h"
#include "Graphics/Core/Texture.h"
#include "spline/spline.h"
#include "Utilities/Singleton.h"

#define LIDAR_DATA_DELIMITER ' '

/**
*	@brief Wraps the LiDAR capture parameters.
*/
class MaterialDatabase : public Singleton<MaterialDatabase>
{
protected:
	friend class Singleton<MaterialDatabase>;

public:
	// 
	enum LiDARMaterialList
	{
		ALUMINIUM, COPPER, GOLD, IRON, SILVER,			
		AIR, WATER, STONE, WOOD, LEAF,
		FABRIC, CRYSTAL_GLASS, PLASTIC,
		DIAMOND, GEMS,
		NUM_MATERIALS
	};

	//
	inline static const std::string LiDARMaterial_STR[NUM_MATERIALS] = 
	{
		"ALUMINIUM", "COPPER", "GOLD", "IRON", "SILVER",
		"AIR", "WATER", "STONE", "WOOD", "LEAF",
		"FABRIC", "CRYSTAL_GLASS", "PLASTIC",
		"DIAMOND", "GEMS"
	};

	/**
	*	@brief
	*/
	struct LiDARMaterial
	{
		inline static unsigned _globalIdentifier = 0;

		unsigned		_brdf;
		unsigned		_identifier;
		float			_roughness;
		tk::spline		_refractiveIndex;

		/**
		*	@brief Constructor of a material relevant for LiDAR sensing.
		*/
		LiDARMaterial() : _identifier(_globalIdentifier++), _brdf(0), _refractiveIndex(), _roughness(.0f)
		{
		}

		/**
		*	@return Refractive index for such wavelength on the fitted spline function. 
		*/
		float getRefractiveIndex(const float wavelength)
		{
			if (_refractiveIndex.num_points())
				return _refractiveIndex(wavelength);
			else
				return .0f;
		}
	};

	struct LiDARMaterialGPUData
	{
		float				_refractiveIndex;
		float				_roughness;
		vec2				_padding1;
	};

protected:
	const static std::string BRDF_DATABASE_FILE;
	const static std::string LIDAR_MATERIAL_FOLDER;
	const static std::string REFLECTIVITY_FILE;
	const static std::string REFLECTIVITY_FOLDER;
	const static std::string REFRACTIVE_INDEX_FOLDER;
	const static std::string ROUGHNESS_FOLDER;
	const static std::string ROUGHNESS_FILE;

protected:
	BRDFDatabase									_brdfDatabase;
	std::unordered_map<std::string, LiDARMaterial*> _LiDARMaterial;
	std::unordered_map<unsigned, std::string>		_LiDARMaterialID;

protected:
	/**
	*	@brief Protected constructor.
	*/
	MaterialDatabase();

	/**
	*	@return Material related to such name or a new material if doesn't exist yet. 
	*/
	LiDARMaterial* getMaterial(const std::string& name);
	
	/**
	*	@brief
	*/
	void loadRefractiveIndicesMap();

	/**
	*	@brief
	*/
	void loadReflectivityMap();

	/**
	*	@brief
	*/
	void loadRoughnessMap();

	/**
	*	@brief  
	*/
	void readRefractiveIndexFile(const std::string& materialName, const std::string& filePath);

public:
	/**
	*	@brief Destructor. 
	*/
	~MaterialDatabase();

	/**
	*	@brief 
	*/
	void exportRefractiveSpline(const std::string& rootPath);

	/**
	*	@return Name of material.
	*/
	std::string getMaterialName(const unsigned id) { return _LiDARMaterialID[id]; }
	
	// --------- Getters ----------

	/**
	*	@return Identifier of material whose name is equal to the one given in parameters, if any.
	*/
	unsigned getMaterialID(const std::string& name);

	/**
	*	@return Identifier of material corresponding to a enum value from the material list, if any.
	*/
	unsigned getMaterialID(const LiDARMaterialList material);

	/**
	*	@brief Feeds a vector with simplified description of materials for GPU simulation. 
	*/
	void getMaterialGPUArray(const float waveLength, std::vector<LiDARMaterialGPUData>& material, std::vector<float>& brdf);
};