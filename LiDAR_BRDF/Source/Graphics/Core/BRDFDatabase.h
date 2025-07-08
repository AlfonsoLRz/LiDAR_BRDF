#pragma once

#define DEBUG true
#define POWITACQ_IMPLEMENTATION

class BRDFDatabase
{
protected:
	struct BRDFMaterial
	{
		std::string			_name;
		std::vector<float>	_reflectance;
	};

protected:
	const static std::string BINARY_DATABASE;
	const static std::string BINARY_MATERIAL_EXTENSION;
	const static std::string SAMPLE_BRDF_OUT;
	const static unsigned THETA_SAMPLES, PHI_SAMPLES;

	std::vector<std::unique_ptr<BRDFMaterial>>		_material;
	std::unordered_map<std::string, int>			_materialId;
	std::vector<float>								_wavelengths;

protected:
	/**
	*	@brief
	*/
	unsigned findWavelengthIndex(float wl);

	/**
	*	@brief Loads a binary database.
	*/
	bool loadBinary(const std::string& filename);

	/**
	*	@brief 
	*/
	BRDFMaterial* sampleBSDF(const std::string& filename, const std::string& materialName);

	/**
	*	@brief Writes a binary database.
	*/
	bool saveBinary(const std::string& filename);

	/**
	*	@brief Saves the sampled reflectance to be read in Python.
	*/
	bool saveSampledBRDF(const std::string& filename, BRDFMaterial* material);

	/**
	*	@brief 
	*/
	void writeSample(const std::string& filename, BRDFMaterial* material);

public:
	/**
	*	@brief Constructor for a database located in 'filename'.
	*/
	BRDFDatabase(const std::string& folder, bool useBinary = true);

	/**
	*	@brief Destructor.
	*/
	virtual ~BRDFDatabase();

	/**
	*	@return Index of material in the database, if exists. -1 otherwise.
	*/
	int lookUpMaterial(const std::string& name);

	/**
	*	@return Index of material in the database, if exists. -1 otherwise.
	*/
	void lookUpMaterial(unsigned id, float wl, std::vector<float>& reflectance);

	/**
	*	@return Number of materials.
	*/
	unsigned numMaterials() { return _material.size(); }
};

