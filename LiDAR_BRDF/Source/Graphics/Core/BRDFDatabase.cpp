#include "stdafx.h"
#include "BRDFDatabase.h"

#include "bsdf/powitacq.h"
#include "Utilities/RandomUtilities.h"

const std::string BRDFDatabase::BINARY_DATABASE = "database.bin";
const std::string BRDFDatabase::BINARY_MATERIAL_EXTENSION = "spec.bsdf";
const std::string BRDFDatabase::SAMPLE_BRDF_OUT = ".out";
const unsigned BRDFDatabase::THETA_SAMPLES = 90;
const unsigned BRDFDatabase::PHI_SAMPLES = 360;

// [Public methods]

BRDFDatabase::BRDFDatabase(const std::string& folder, bool useBinary)
{
	if (!std::filesystem::exists(folder + BINARY_DATABASE))
	{
		for (const auto& file : std::filesystem::directory_iterator(folder))
		{
			std::string fileName = file.path().filename().string();

			if (fileName.find(BINARY_MATERIAL_EXTENSION) != std::string::npos && fileName.find(".txt") == std::string::npos)
			{
				std::cout << "Loading material " << fileName << " ..." << std::endl;
				_material.push_back(std::unique_ptr<BRDFMaterial>(this->sampleBSDF(file.path().string(), file.path().filename().replace_extension().string())));
				_materialId[file.path().filename().replace_extension().string()] = _material.size() - 1;
				this->saveSampledBRDF(folder + file.path().filename().replace_extension().string(), _material[_material.size() - 1].get());
			}
		}

		this->saveBinary(folder + BINARY_DATABASE);
	}
	else
	{
		this->loadBinary(folder + BINARY_DATABASE);
	}
}

BRDFDatabase::~BRDFDatabase()
{
}

int BRDFDatabase::lookUpMaterial(const std::string& name)
{
	return _materialId[name];
}

void BRDFDatabase::lookUpMaterial(unsigned materialId, float wl, std::vector<float>& reflectance)
{
	if (materialId < _material.size())
	{
		BRDFMaterial* material = _material[materialId].get();

		unsigned wlIndex = this->findWavelengthIndex(wl), index;
		//reflectance.resize(PHI_SAMPLES * (THETA_SAMPLES + 1));

		for (int phi = 0; phi < PHI_SAMPLES; ++phi)
		{
			for (int theta = 0; theta <= THETA_SAMPLES; ++theta)
			{
				index = phi * (THETA_SAMPLES + 1) + theta;
				float f_phi = phi / static_cast<float>(PHI_SAMPLES) * 2.0f * M_PI;
				float f_theta = (1.0f - theta / static_cast<float>(THETA_SAMPLES)) * M_PI / 2.0f;
				vec3 wi_wo = vec3(glm::cos(f_phi), -glm::sin(f_phi), glm::sin(f_theta));

				reflectance.push_back(material->_reflectance[index * _wavelengths.size() + wlIndex]);
			}
		}

		while (glm::epsilonNotEqual(static_cast<float>(fmod(reflectance.size(), 4)), .0f, glm::epsilon<float>()))
		{
			reflectance.push_back(.0f);
		}
	}
}

// [Protected methods]

unsigned BRDFDatabase::findWavelengthIndex(float wl)
{
	float minDistance = FLT_MAX;

	for (int wlIndex = 0; wlIndex < _wavelengths.size(); ++wlIndex)
	{
		if (glm::distance(_wavelengths[wlIndex], wl) < minDistance)
		{
			minDistance = glm::distance(_wavelengths[wlIndex], wl);
		}
		else
		{
			return wlIndex - 1;
		}
	}

	return _wavelengths.size() - 1;
}

bool BRDFDatabase::loadBinary(const std::string& filename)
{
	std::ifstream fin(filename, std::ios::in | std::ios::binary);
	if (!fin.is_open())
	{
		return true;
	}

	size_t numMaterials, stringSize, numSamples, numWavelengths;

	fin.read((char*)&numWavelengths, sizeof(size_t));
	this->_wavelengths.resize(numWavelengths);
	fin.read((char*)this->_wavelengths.data(), numWavelengths * sizeof(float));

	fin.read((char*)&numMaterials, sizeof(size_t));
	while (_material.size() < numMaterials)
	{
		BRDFMaterial* material = new BRDFMaterial;

		fin.read((char*)&stringSize, sizeof(size_t));
		material->_name.resize(stringSize);
		fin.read((char*)material->_name.data(), stringSize * sizeof(char));

		std::cout << "Loading material " << material->_name << BINARY_MATERIAL_EXTENSION << " ..." << std::endl;

		fin.read((char*)&numSamples, sizeof(size_t));
		material->_reflectance.resize(numSamples);
		fin.read((char*)material->_reflectance.data(), numSamples * sizeof(float));

		_material.push_back(std::unique_ptr<BRDFMaterial>(material));
		_materialId[material->_name] = _material.size() - 1;
	}

	fin.close();

	return true;
}

BRDFDatabase::BRDFMaterial* BRDFDatabase::sampleBSDF(const std::string& filename, const std::string& materialName)
{
	BRDFMaterial* material = new BRDFMaterial;
	powitacq::BRDF brdf(filename);
	bool savedDebug = false;

	auto wl = brdf.wavelengths();
	if (_wavelengths.size() != wl.size())
		_wavelengths = std::vector(std::begin(wl), std::end(wl));
	material->_name = materialName;
	material->_reflectance.resize(PHI_SAMPLES * (THETA_SAMPLES + 1) * wl.size());

	std::vector<float> spectrum90(wl.size(), .0f);
	
	// Sample wi and wo
	//#pragma omp parallel for
	for (int phi = 0; phi < PHI_SAMPLES; ++phi)
	{
		for (int theta = 0; theta <= THETA_SAMPLES; ++theta)
		{
			float f_phi = phi / static_cast<float>(PHI_SAMPLES) * 2.0f * M_PI;
			float f_theta = (theta / static_cast<float>(THETA_SAMPLES)) * M_PI / 2.0f;
			powitacq::Vector3f wi_wo = powitacq::Vector3f(glm::cos(f_phi), -glm::sin(f_phi), glm::sin(f_theta));

			auto spectrum = brdf.eval(wi_wo, wi_wo);
			std::ranges::copy(spectrum, material->_reflectance.begin() + (phi * (THETA_SAMPLES + 1) + theta) * wl.size());

			if (glm::abs(glm::sin(f_theta) - 1.0) < glm::epsilon<float>())
			{
				for (int i = 0; i < wl.size(); ++i)
				{
					spectrum90[i] += spectrum[i];
				}
			}

		}
	}

	// Write to file in C:\Users\alfon\Pictures\Assets - Artículos\LiDAR (Elsevier)\helios-plusplus-win\assets\spectra
	std::ofstream file("C:\\helios-plusplus-win\\assets\\spectra\\" + materialName + ".txt");

	if (file.is_open() && file.good())
	{
		/**
		 * Write metadata:
		 *
			First Column:  X
			Second Column:  Y
			X Units:  Wavelength (micrometers)
			Y Units:  Reflectance (percent)
			First X Value: 0.3
			Last X Value: 12.5
			Number of X Values: 536
			Additional Information:  None
		**/
		file << "Measurement:  Directional (10 Degree) Hemispherical Reflectance" << std::endl;
		file << "First Column:  X" << std::endl;
		file << "Second Column:  Y" << std::endl;
		file << "X Units:  Wavelength (micrometers)" << std::endl;
		file << "Y Units:  Reflectance (percent)" << std::endl;
		file << "First X Value: " << _wavelengths.front() / 1000.0f << std::endl;
		file << "Last X Value: " << _wavelengths.back() / 1000.0f << std::endl;
		file << "Number of X Values: " << _wavelengths.size() << std::endl;
		file << "Additional Information:  None\n" << std::endl;

		for (int i = 0; i < wl.size(); ++i)
		{
			file << wl[i] / 1000.0f << "\t" << spectrum90[i] << std::endl;
		}
		file.close();
		savedDebug = true;
	}

#if DEBUG
		this->writeSample(filename + "_raw_normalized.txt", material);
#endif

	return material;
}

bool BRDFDatabase::saveBinary(const std::string& filename)
{
	std::ofstream fout(filename, std::ios::out | std::ios::binary);
	if (!fout.is_open())
	{
		return false;
	}

	size_t stringSize, numSamples;
	const size_t numMaterials = _material.size(), numWavelengths = _wavelengths.size();

	fout.write((char*)&numWavelengths, sizeof(size_t));
	fout.write(reinterpret_cast<char*>(this->_wavelengths.data()), numWavelengths * sizeof(float));

	fout.write((char*)&numMaterials, sizeof(size_t));
	for (const std::unique_ptr<BRDFMaterial>& material: _material)
	{
		stringSize = material->_name.size();
		fout.write(reinterpret_cast<char*>(&stringSize), sizeof(size_t));
		material->_name.resize(stringSize);
		fout.write(material->_name.data(), stringSize * sizeof(char));

		numSamples = material->_reflectance.size();
		fout.write(reinterpret_cast<char*>(&numSamples), sizeof(size_t));
		material->_reflectance.resize(numSamples);
		fout.write(reinterpret_cast<char*>(material->_reflectance.data()), numSamples * sizeof(float));
	}

	fout.close();

	return true;
}

bool BRDFDatabase::saveSampledBRDF(const std::string& filename, BRDFMaterial* material)
{
	std::ofstream fout(filename + SAMPLE_BRDF_OUT, std::ios::out | std::ios::binary);
	if (!fout.is_open())
	{
		return false;
	}

	vec3 size = vec3(THETA_SAMPLES, PHI_SAMPLES, _wavelengths.size());
	std::vector<float> reflectance (material->_reflectance.begin(), material->_reflectance.end());

	fout.write((char*)&size, sizeof(vec3));
	fout.write((char*)_wavelengths.data(), _wavelengths.size() * sizeof(float));
	fout.write((char*)reflectance.data(), reflectance.size() * sizeof(float));
	fout.close();

	return true;
}

void BRDFDatabase::writeSample(const std::string& filename, BRDFMaterial* material)
{
	std::ofstream file(filename);

	if (file.is_open() && file.good())
	{
		const unsigned randomPhi = RandomUtilities::getUniformRandomValue() * (PHI_SAMPLES - 1);
		const unsigned randomWavelength = RandomUtilities::getUniformRandomValue() * _wavelengths.size();
		float f_phi = randomPhi / static_cast<float>(PHI_SAMPLES) * 2.0f * M_PI, f_theta, reflectance;

		for (int i = 0; i <= THETA_SAMPLES; ++i)
		{
			f_theta = (i / static_cast<float>(THETA_SAMPLES)) * M_PI / 2.0f;
			vec3 wi_wo = vec3(glm::cos(f_phi), -glm::sin(f_phi), glm::sin(f_theta));
			reflectance = material->_reflectance[(randomPhi * (THETA_SAMPLES + 1) + i) * _wavelengths.size() + randomWavelength];

			if (!i)
				file << reflectance << std::endl;
			else
				file << reflectance / glm::dot(glm::normalize(vec3(std::cos(f_theta), .0f, glm::sin(f_theta))), vec3(.0f, .0f, 1.0f)) << std::endl;
		}

		file.close();
	}
}
