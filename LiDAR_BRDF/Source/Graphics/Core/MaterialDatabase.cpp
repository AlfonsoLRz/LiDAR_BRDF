#include "stdafx.h"
#include "MaterialDatabase.h"

#include <filesystem>
#include "Graphics/Core/Model3D.h"
#include <regex>
#include "Utilities/FileManagement.h"

/// [Initialization of static attributes]

const std::string MaterialDatabase::BRDF_DATABASE_FILE = "Assets/BRDF/brdfs_rgl_18/";
const std::string MaterialDatabase::LIDAR_MATERIAL_FOLDER = "Assets/LiDAR/";
const std::string MaterialDatabase::REFLECTIVITY_FOLDER = "Reflectivity/";
const std::string MaterialDatabase::REFRACTIVE_INDEX_FOLDER = "RefractiveIndex/";
const std::string MaterialDatabase::ROUGHNESS_FOLDER = "Roughness/";
const std::string MaterialDatabase::REFLECTIVITY_FILE = "Reflectivity.txt";
const std::string MaterialDatabase::ROUGHNESS_FILE = "Roughness.txt";

/// [Protected methods]

MaterialDatabase::MaterialDatabase(): _brdfDatabase(BRDF_DATABASE_FILE)
{
	this->loadReflectivityMap();
	this->loadRefractiveIndicesMap();
	this->loadRoughnessMap();

	this->exportRefractiveSpline("../Python/PathRenderer/MaterialSpline/");
}

MaterialDatabase::LiDARMaterial* MaterialDatabase::getMaterial(const std::string& name)
{
	LiDARMaterial* material = nullptr;
	auto it = _LiDARMaterial.find(name);
	
	if (it == _LiDARMaterial.end())
	{
		material = new LiDARMaterial();
		_LiDARMaterial[name] = material;
		_LiDARMaterialID[material->_identifier] = name;
	}
	else
	{
		material = it->second;
	}
	
	return material;
}

void MaterialDatabase::loadRefractiveIndicesMap()
{
	const std::string folder = LIDAR_MATERIAL_FOLDER + REFRACTIVE_INDEX_FOLDER;
	const unsigned rootFolderLength = folder.length();
	std::string filePath, materialName;

	for (auto& assetFile : std::filesystem::recursive_directory_iterator(folder))
	{
		if (!assetFile.is_directory())
		{
			filePath = assetFile.path().generic_string();
			const size_t extensionDotIndex = filePath.find_last_of(".");
			const size_t folderBarIndex = filePath.find_last_of("/");
			materialName = filePath.substr(folderBarIndex + 1, extensionDotIndex - folderBarIndex - 1);
			
			this->readRefractiveIndexFile(materialName, filePath);
		}
	}
}

void MaterialDatabase::loadReflectivityMap()
{
	const std::string filename = LIDAR_MATERIAL_FOLDER + REFLECTIVITY_FOLDER + REFLECTIVITY_FILE;
	std::string line;
	std::ifstream in;
	std::vector<std::string> stringTokens;
	std::vector<float> floatTokens;

	in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	in.open(filename);

	while (!(in >> std::ws).eof())
	{
		std::getline(in, line);

		FileManagement::clearTokens(stringTokens, floatTokens);
		FileManagement::readTokens(line, LIDAR_DATA_DELIMITER, stringTokens, floatTokens);

		if (stringTokens.size() == 2)
		{
			// Clean material name
			std::string materialName = stringTokens[1];
			materialName.erase(remove(materialName.begin(), materialName.end(), '\t'), materialName.end());

			MaterialDatabase::LiDARMaterial* material = this->getMaterial(stringTokens[0]);
			material->_brdf = _brdfDatabase.lookUpMaterial(materialName + "_spec");
		}
	}

	in.close();
}

void MaterialDatabase::loadRoughnessMap()
{
	const std::string filename = LIDAR_MATERIAL_FOLDER + ROUGHNESS_FOLDER + ROUGHNESS_FILE;
	std::string line;
	std::ifstream in;
	std::vector<std::string> stringTokens;
	std::vector<float> floatTokens;

	in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	in.open(filename);

	while (!(in >> std::ws).eof())
	{
		std::getline(in, line);

		FileManagement::clearTokens(stringTokens, floatTokens);
		FileManagement::readTokens(line, LIDAR_DATA_DELIMITER, stringTokens, floatTokens);

		if (floatTokens.size() && stringTokens.size())
		{
			MaterialDatabase::LiDARMaterial* material = this->getMaterial(stringTokens[0]);
			material->_roughness = floatTokens[0];
		}
	}

	in.close();
}

void MaterialDatabase::readRefractiveIndexFile(const std::string& materialName, const std::string& filePath)
{
	const char DELIMITER = '\t';
	std::string line;
	std::ifstream in;
	std::vector<std::string> stringTokens;
	std::vector<float> floatTokens;

	std::vector<double> waveLength, refractiveIndex;
	float waveLengthUnit = 1000.0f;

	in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	in.open(filePath);

	while (!(in >> std::ws).eof())
	{
		std::getline(in, line);

		FileManagement::clearTokens(stringTokens, floatTokens);
		FileManagement::readTokens(line, DELIMITER, stringTokens, floatTokens);

		if (stringTokens.size() && stringTokens[0].find("nm") != std::string::npos)		// Nanometers instead of micro
		{
			waveLengthUnit = 1.0f;
		}
		
		if (stringTokens.size() >= 2 && stringTokens[1] == "k")							// Extinction coefficient 
		{
			break;
		}

		if (floatTokens.size())
		{
			if (waveLength.empty() && (floatTokens[0] * waveLengthUnit) > 2000.0f)
				waveLengthUnit = 100.0f;

			waveLength.push_back(floatTokens[0] * waveLengthUnit);						// From micrometers to nanometers
			refractiveIndex.push_back(floatTokens[1]);
		}
	}

	{
		LiDARMaterial* material = this->getMaterial(materialName);
		material->_refractiveIndex.set_points(waveLength, refractiveIndex);
	}

	in.close();
}

/// [Public methods]

MaterialDatabase::~MaterialDatabase()
{
	auto it = _LiDARMaterial.begin();

	while (it != _LiDARMaterial.end())
	{
		delete it->second;
		++it;
	}
}

void MaterialDatabase::exportRefractiveSpline(const std::string& rootPath)
{
	const ivec2 bounds(0, 1500);

	for (auto& mapMaterial : _LiDARMaterial)
	{
		std::ofstream file(rootPath + mapMaterial.first + ".txt");
		if (file.is_open())
		{
			float t = bounds.x;
			while (t < bounds.y)
			{
				file << t << "," << mapMaterial.second->getRefractiveIndex(t) << "\n";
				t += 1.0f;
			}

			file.close();
		}
	}
}

unsigned MaterialDatabase::getMaterialID(const std::string& name)
{
	auto it = _LiDARMaterial.find(name);

	if (it != _LiDARMaterial.end())
	{
		return it->second->_identifier;
	}

	return 0;				// Assign first material
}

unsigned MaterialDatabase::getMaterialID(const LiDARMaterialList material)
{
	return getMaterialID(LiDARMaterial_STR[material]);
}

void MaterialDatabase::getMaterialGPUArray(const float waveLength, std::vector<LiDARMaterialGPUData>& material, std::vector<float>& brdf)
{
	material.clear(); material.resize(_LiDARMaterial.size());
	
	for (auto& pair : _LiDARMaterialID)
	{
		auto it = _LiDARMaterial.find(pair.second);
		if (it != _LiDARMaterial.end())
		{
			material[it->second->_identifier]._refractiveIndex = it->second->getRefractiveIndex(waveLength);
			material[it->second->_identifier]._roughness = it->second->_roughness;
			_brdfDatabase.lookUpMaterial(it->second->_brdf, waveLength, brdf);
		}
	}
}