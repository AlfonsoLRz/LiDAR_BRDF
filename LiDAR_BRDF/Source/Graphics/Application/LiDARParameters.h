#pragma once

/**
*	@file LiDARParameters.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 08/13/2020
*/

typedef std::vector<vec2> LiDARPath;
typedef std::vector<vec4> BouncePath;

/**
*	@brief Wraps the LiDAR capture parameters.
*/
struct LiDARParameters
{
public:
	struct RangeResolution
	{
		vec2 _rangeInterval;
		int  _resolution;

		/**
		*	@brief Constructor of a LiDAR scanning interval.
		*/
		RangeResolution(const vec2& range, int resolution = 10) : _rangeInterval(range), _resolution(resolution) {}
	};

public:
	inline static const float		MAX_NORMALIZED_VALUE = 1.0f;				//!<
	inline static const float		MIN_NORMALIZED_VALUE = .0f;					//!<
	
	// xLS
	inline static const unsigned	MAX_NUMBER_OF_RETURNS = 5;					//!< Maximum number of returns of an unique ray
	inline static const unsigned	MIN_NUMBER_OF_RETURNS = 1;					//!< Minimum number of returns of an unique ray
	inline static const float		MAX_PEAK_POWER = 150.0f;					//!<
	inline static const float		MIN_PEAK_POWER = 10.0f;						//!<
	inline static const float		MAX_PULSE_RADIUS = 0.01f;					//!<
	inline static const float		MIN_PULSE_RADIUS = glm::epsilon<float>();	//!<
	inline static const int			MAX_RAYS_PULSE = 50;						//!<
	inline static const int			MIN_RAYS_PULSE = 1;							//!<
	inline static const float		MAX_RANGE = 300.0f;							//!<
	inline static const float		MIN_RANGE = .1f;
	inline static const float		MAX_SENSOR_DIAMETER = 1.0f;					//!<
	inline static const float		MIN_SENSOR_DIAMETER = 0.05f;				//!<
	inline static const float		MAX_REFLECTANCE_WEIGHT = 300.0f;			//!<
	inline static const float		MIN_REFLECTANCE_WEIGHT = 0.0001f;			//!<
	inline static const float		MAX_CHANNEL_DISTANCE = 0.05f;				//!<
	inline static const float		MIN_CHANNEL_DISTANCE = .0f;					//!<
	inline static const float		MAX_CHANNEL_ANGLE_INC = 20.0f;				//!<
	inline static const float		MIN_CHANNEL_ANGLE_INC = .0f;				//!<
	inline static const float		MAX_OUTLIER_THRESHOLD = 1.0f;				//!<
	inline static const float		MIN_OUTLIER_THRESHOLD = 0.0f;				//!<
		
	// TLS
	inline static const float		TLS_MAX_ANGLE = 360.0f;						//!<
	inline static const float		TLS_MIN_ANGLE = .0f;						//!<
	inline static const float		TLS_MAX_FOV_HORIZONTAL = 360.0f;			//!<
	inline static const float		TLS_MIN_FOV_HORIZONTAL = 180.0f;			//!<
	inline static const float		TLS_MAX_FOV_VERTICAL = 180.0f;				//!<
	inline static const float		TLS_MIN_FOV_VERTICAL = 15.0f;				//!<
	inline static const int			TLS_MAX_RESOLUTION_HORIZONTAL = 8000;		//!<
	inline static const int			TLS_MIN_RESOLUTION_HORIZONTAL = 20;			//!<
	inline static const int			TLS_MAX_RESOLUTION_VERTICAL = 6000;			//!<
	inline static const int			TLS_MIN_RESOLUTION_VERTICAL = 10;			//!<
	inline static const float		TLS_MAX_AXIS_JITTERING = 1.0f / 200.0f;		//!<
	inline static const float		TLS_MIN_AXIS_JITTERING = .0f;				//!<
	inline static const float		TLS_MAX_ANGLE_JITTERING = 1.0f / 10.0f;	//!<
	inline static const float		TLS_MIN_ANGLE_JITTERING = .0f;				//!<
	inline static const float		TLS_MAX_ATMOSPHERE_ATTENUATION = 3.9f;		//!<
	inline static const float		TLS_MIN_ATMOSPHERE_ATTENUATION = 0.2f;		//!<

	// ALS
	inline static const float		ALS_MAX_HEIGHT = 40.0f;						//!<
	inline static const float		ALS_MIN_HEIGHT = -10.0f;					//!<
	inline static const float		ALS_MAX_FOV = 170.0f;						//!<
	inline static const float		ALS_MIN_FOV = 5.0f;							//!<
	inline static const float		ALS_MAX_SPEED = 20.0f;						//!<		
	inline static const float		ALS_MIN_SPEED = 0.001f;						//!<
	inline static const int			ALS_MAX_PULSE_FREQUENCY = 10000;			//!<
	inline static const int			ALS_MIN_PULSE_FREQUENCY = 5;				//!<
	inline static const int			ALS_MAX_SCAN_FREQUENCY = 100;				//!<
	inline static const int			ALS_MIN_SCAN_FREQUENCY = 1;					//!<
	inline static const float		ALS_MAX_ELLIPTICAL_SCALE = 1.0f;			//!<
	inline static const float		ALS_MIN_ELLIPTICAL_SCALE = 0.1f;			//!<
	inline static const float		ALS_MAX_HEIGHT_JITTERING = 1.0f / 40.0f;	//!<
	inline static const float		ALS_MIN_HEIGHT_JITTERING = .0f;				//!<
	inline static const float		ALS_MAX_RAY_JITTERING = 1.0f / 100.0f;		//!<
	inline static const float		ALS_MIN_RAY_JITTERING = .0f;				//!<
	inline static const int			ALS_MAX_SCENE_SWEEPS = 200;					//!<
	inline static const int			ALS_MIN_SCENE_SWEEPS = 1;					//!<
	inline static const float		ALS_MAX_ATMOSPHERE_ATTENUATION = 0.22f;		//!<
	inline static const float		ALS_MIN_ATMOSPHERE_ATTENUATION = 0.14f;		//!<

public:
	// --------- Ray Construction ---------
	enum RayBuild : uint8_t {
		TERRESTRIAL_SPHERICAL, AERIAL_LINEAR, AERIAL_ZIGZAG, AERIAL_ELLIPTICAL, NUM_RAY_BUILDERS
	};

	inline static const char* RayBuilder_STR[NUM_RAY_BUILDERS] = { "Terrestrial (Spherical)", "Aerial (Linear)", "Aerial (Zig Zag)",
																   "Aerial (Elliptical)" };

	// --------- LiDAR Specifications ---------
	enum LiDARSpecifications : uint8_t {
		CUSTOM, HDL64E, Pandar64, HDL32E, Puck, PuckLite, PuckHiRes, UltraPuck, AlphaPrime, Zenmuse_L1, NUM_SPECIFICATIONS
	};

	inline static const char* LiDARSpecs_STR[NUM_SPECIFICATIONS] = { "None", "HDL-64E", "Pandar64", "HDL-32E", "Puck", "Puck Lite", "Puck Hi-Res", "Ultra Puck", "Alpha Prime", "Zenmuse L1" };

	// -------- Sensor Channels

	enum Channels : uint8_t {
		CH_1, CH_8, CH_16, CH_32, CH_64, CH_128, NUM_DISTINCT_CHANNELS
	};

	inline static const char*	Channels_STR[NUM_DISTINCT_CHANNELS] = { "1", "8", "16", "32", "64", "128" };
	inline static unsigned		Channels_UINT[NUM_DISTINCT_CHANNELS] = { 1, 8, 16, 32, 64, 128 };

	// ----------- Classes ------------
	enum ASPRSClass : uint8_t {
		CREATED, UNCLASSIFIED, GROUND, LOW_VEGETATION, MEDIUM_VEGETATION, HIGH_VEGETATION, BUILDING, NOISE,
		RESERVED_01, WATER, RAIL, ROAD_SURFACE, RESERVED_02, WIRE_GUARD, WIRE_CONDUCTOR, TRANSMISSION_TOWER, WIRE,
		BRIDGE_DECK, NIGH_NOISE,
		NUM_CLASSES
	};

	inline static const std::string ASPRSClass_STR[NUM_CLASSES] = {
		"CREATED", "UNCLASSIFIED", "GROUND", "LOW_VEGETATION", "MEDIUM_VEGETATION", "HIGH_VEGETATION", "BUILDING", "NOISE",
		"RESERVED_01", "WATER", "RAIL", "ROAD_SURFACE", "RESERVED_02", "WIRE_GUARD", "WIRE_CONDUCTOR", "TRANSMISSION_TOWER", "WIRE",
		"BRIDGE_DECK", "NIGH_NOISE"
	};

	inline static const std::unordered_map<std::string, ASPRSClass> ASPRSClassString = {
		{ ASPRSClass_STR[0], ASPRSClass::CREATED }, { ASPRSClass_STR[1], ASPRSClass::UNCLASSIFIED }, { ASPRSClass_STR[2], ASPRSClass::GROUND },
		{ ASPRSClass_STR[3], ASPRSClass::LOW_VEGETATION }, { ASPRSClass_STR[4], ASPRSClass::MEDIUM_VEGETATION }, { ASPRSClass_STR[5], ASPRSClass::HIGH_VEGETATION },
		{ ASPRSClass_STR[6], ASPRSClass::BUILDING }, { ASPRSClass_STR[7], ASPRSClass::NOISE }, { ASPRSClass_STR[8], ASPRSClass::RESERVED_01 },
		{ ASPRSClass_STR[9], ASPRSClass::WATER }, { ASPRSClass_STR[10], ASPRSClass::RAIL }, { ASPRSClass_STR[11], ASPRSClass::ROAD_SURFACE },
		{ ASPRSClass_STR[12], ASPRSClass::RESERVED_02 }, { ASPRSClass_STR[13], ASPRSClass::WIRE_GUARD }, { ASPRSClass_STR[14], ASPRSClass::WIRE_CONDUCTOR },
		{ ASPRSClass_STR[15], ASPRSClass::TRANSMISSION_TOWER }, { ASPRSClass_STR[16], ASPRSClass::WIRE }, { ASPRSClass_STR[17], ASPRSClass::BRIDGE_DECK },
		{ ASPRSClass_STR[18], ASPRSClass::NIGH_NOISE }
	};

	const float LIGHT_SPEED_MS = 299792458.0f;				//!< Defines maximum distance (along with maximum travelling time)

public:
	// Type of LiDAR
	int			_LiDARType;									//!< Terrestrial, aerial
	int			_LiDARSpecs;								//!< Model whose specifications these parameters are supposed to follow

	// Computational parameters
	bool		_gpuInstantiation;							//!<
	int			_numExecs;									//!< Number of repetitions for a LiDAR simulation
	
	// Global parameters
	int			_channels;									//!< Number of simultaneous channels
	bool		_discardFirstExecution;						//!< Omit first execution for measuring latency
	float		_douglasPeckerEpsilon;						//!< Filter of user-defined paths
	float		_hermiteT;									//!< Blending factor of Hermite interpolations
	bool		_includeOutliers;							//!< Includes anomalies in the point cloud
	bool		_includeShinySurfaceError;					//!< Includes error from shiny surfaces as there exists several rebounds which distorts the return signal
	bool		_includeTerrainInducedError;				//!< Applies horizontal and vertical errors due to terrain slope and airbone height
	float		_maxRange;									//!< Maximum range of beams
	vec2		_maxRangeSoftBoundary;						//!< Boundaries from max. range where rays may not collide
	unsigned    _maxReturns;								//!< Current maximum value of returns of a same ray (global maximum is defined in the constraints)
	vec2		_outlierRange;								//!< Range of t values, from 0 to 1, where outliers can be instantiated
	float		_outlierThreshold;							//!< As outlier simulation uses a noise texture, over which values do we considerate a displacement
	float		_peakPower;									//!< Initial power of each ray launched by LiDAR sensor (watts)
	float		_pulseRadius;								//!< Radius of a LiDAR pulse
	int			_rayDivisor;								//!< Boundary for maximum number of rays contained in the GPU
	int			_raysPulse;									//!< Number of rays for a discretized pulse
	float		_reflectanceWeight;							//!< Weight of material reflectance in intensity equation
	BouncePath	_returnThreshold;							//!< Percentage of success for every return level (first should have a higher chance)
	float		_sensorDiameter;							//!< Diameter in meters
	float		_simulationTime;							//!<
	float		_scanFrequencyHz;							//!<
	float		_systemAttenuation;							//!<
	bool		_useCatmullRom;								//!< Interpolates path through Catmull or linear
	bool		_useSimulationTime;							//!<
	ivec2		_wavelength;								//!< Wavelength in nanometers

	// TLS
	LiDARPath   _tlsManualPath;								//!< Manual path defined by the user through the GUI
	vec2		_tlsManualPathCanvasSize;					//!< Size of canvas where manual path was drawn
	bool		_tlsUseManualPath;							//!< Apply user-defined path during TLS scan
	vec3		_tlsPosition;								//!< Origin of rays for TLS
	vec3		_tlsDirection;								//!<
	float		_tlsFOVVertical;							//!< Field of view for both horizontal and vertical field
	float		_tlsFOVHorizontal;							//!< Idem
	int			_tlsResolutionVertical;						//!< Resolution of vertical view for TLS sensor
	int			_tlsResolutionHorizontal;					//!< Resolution of horizontal view for TLS sensor
	float		_tlsMiddleAngleHorizontal;					//!< Angle just in the middle of horizontal FOV
	float		_tlsMiddleAngleVertical;					//!< Starting angle for vertical scans
	float		_tlsAxisJittering;							//!<
	float		_tlsAngleJittering;							//!<
	float		_tlsAtmosphericClearness;					//!<
	bool		_tlsUniformVerticalResolution;				//!< Enables non-uniform resolution, e.g. for Pandar64
	std::vector<RangeResolution> _tlsRangeResolution;		//!< Array of spatial intervals with different scanning resolution

	// ALS
	vec3		_alsPosition;								//!< Position for rendering purposes
	vec3		_alsNormal;									//!< Normal for rendering purposes
	LiDARPath   _alsManualPath;								//!< Aerial path defined by the user through the GUI
	vec2		_alsManualPathCanvasSize;					//!< Size of canvas where manual path was drawn
	bool		_alsUseManualPath;							//!< Use path drawn by the user instead of computer-defined paths
	float		_alsFOVHorizontal;							//!< Field of view of ALS (horizontal)
	float		_alsFOVVertical;							//!< Field of view of ALS (vertical)
	float		_alsSpeed;									//!< Meters/second of airbone device
	int			_alsScanFrequency;							//!< Number of scans per seconds
	int			_alsPulseFrequency;							//!< Number of pulses per second
	float		_alsHeightJittering;						//!< Noise for airbone height during path
	float		_alsRayJittering;							//!< Noise for rays direction
	int			_alsMaxSceneSweeps;							//!< Maximum of paths for capturing an scene
	float		_alsOverlapping;							//!<

	// Loss function		
	float		_multCoefficient, _addCoefficient;			//!<
	float		_zeroThreshold;								//!<
	float		_lossPower;									//!<

public:
	LiDARParameters() :
		_LiDARType(RayBuild::TERRESTRIAL_SPHERICAL),
		_LiDARSpecs(LiDARSpecifications::CUSTOM),
		_gpuInstantiation(true),
		_channels(Channels::CH_16),
		_discardFirstExecution(false),
		_douglasPeckerEpsilon(3.0f),
		_hermiteT(.5f),
		_includeOutliers(false),
		_includeShinySurfaceError(true),
		_includeTerrainInducedError(false),
		_maxRange(200.0f),
		_maxRangeSoftBoundary(-10.0f, 3.0f),
		_maxReturns(1),
		_numExecs(1),
		_outlierRange(.0f, 1.0f),
		_outlierThreshold(0.8f),
		_peakPower(65.0f),
		_pulseRadius(.001f),
		_rayDivisor(15),
		_raysPulse(10),
		_reflectanceWeight(1.0f),
		_scanFrequencyHz(50.0f),
		_sensorDiameter(0.215f),
		_simulationTime(1.0f),
		_systemAttenuation(1.0f),
		_useCatmullRom(true),
		_useSimulationTime(false),
		_wavelength(1064, 1064),

		// TLS
		_tlsUseManualPath(false),
		//_tlsPosition(3.0f, -79.0f, 0.0f),
		//_tlsPosition(3.14f, 1.09f, 1.5f),				// Nissan Rogue
		//_tlsPosition(15.5f, 1.3f, 7.87f),
		//_tlsPosition(-2.59168f, -1.66768f, -121.981f),
		//_tlsPosition(187.563f, -78.8403f, -0.62701f),
		_tlsPosition(-1.2f, 1.5f, 1.5f),				// Conference
		//_tlsPosition(2.0f, 0.7f, 2.14f),				// Bedroom
		//_tlsPosition(4.0f, -2.0f, .0f),					// Teapot
		//_tlsPosition(5.8005f, -0.665068f, -34.6425f),		// Suburb
		_tlsFOVVertical(150.0f),
		_tlsFOVHorizontal(240.0f),
		_tlsResolutionVertical(64),
		_tlsResolutionHorizontal(360),
		_tlsMiddleAngleHorizontal(0.0f),
		_tlsMiddleAngleVertical(.0f),
		_tlsAxisJittering(1.0f / 10000.0f),
		_tlsAngleJittering(1.0f / 10000.0f),
		_tlsAtmosphericClearness(1.0f),
		_tlsUniformVerticalResolution(true),

		// ALS
		_alsPosition(.0f, 30.0f, .0f),
		_alsNormal(.0f, .0f, 1.0f),
		_alsFOVHorizontal(70.0f),
		_alsFOVVertical(4.5f),
		_alsSpeed(0.089f),
		_alsScanFrequency(4),
		_alsPulseFrequency(1000),
		_alsManualPathCanvasSize(.0f),
		_alsUseManualPath(false),
		_alsOverlapping(0.5f),
		_alsHeightJittering(1.0f / 200.0f),
		_alsRayJittering(1.0f / 300.0f),
		_alsMaxSceneSweeps(ALS_MAX_SCENE_SWEEPS),

		// Loss function
		//_multCoefficient(1.0f),
		//_addCoefficient(0.8f),
		//_zeroThreshold(.0f),
		//_lossPower(3.5f)

		_multCoefficient(5.0f),
		_addCoefficient(-0.4f),
		_zeroThreshold(.5f),
		_lossPower(4.815f)
	{
		for (int returnIdx = 0; returnIdx < MAX_NUMBER_OF_RETURNS; ++returnIdx)
		{
			_returnThreshold.push_back(vec4(returnIdx + 1.0f, 1.0f - 0.02f - 0.05f * returnIdx, .0f, 1.0f));
		}
	}

	~LiDARParameters()
	{
	}

	/// Setters

	/**
	*	@brief Builds the specs for different sensor models.  
	*/
	void buildSpecifications();

	/// Getters

	/**
	*	@return Number of simultaneous channels. 
	*/
	unsigned getNumberOfChannels() { return Channels_UINT[_channels]; }

	/**
	*	@return Wavelength of sensor in nanometers. 
	*/
	ivec2 getWaveLength() { return _wavelength; }

	/**
	*	@return True if LiDAR ray builder is a terrestrial choice.
	*/
	bool isLiDARTerrestrial() { return _LiDARType == TERRESTRIAL_SPHERICAL; }
};

inline void LiDARParameters::buildSpecifications()
{
	if (_LiDARSpecs == LiDARSpecifications::HDL64E)
	{
		_LiDARType = TERRESTRIAL_SPHERICAL;
		_channels = Channels::CH_64;
		_maxRange = 120.0f;
		_peakPower = 60.0f;
		_sensorDiameter = 0.215f;
		_tlsFOVHorizontal = 360.0f;
		_tlsFOVVertical = 26.9f;
		_tlsMiddleAngleVertical = -11.45f;
		_tlsResolutionHorizontal = 4500;
		_tlsResolutionVertical = 64;
		_maxReturns = 2;
		_tlsUniformVerticalResolution = true;
		_scanFrequencyHz = 10;
		_tlsAngleJittering = 0.002;
	}
	else if (_LiDARSpecs == LiDARSpecifications::Pandar64)
	{
		_LiDARType = TERRESTRIAL_SPHERICAL;
		_channels = Channels::CH_1;
		_maxRange = 200.0f;
		_peakPower = 60.0f;
		_sensorDiameter = 0.116f;
		_tlsFOVHorizontal = 360.0f;
		_tlsFOVVertical = 40.0f;
		_tlsMiddleAngleVertical = -5.0f;
		_tlsResolutionHorizontal = 1800;
		_tlsResolutionVertical = 63;
		_maxReturns = 2;
		_tlsUniformVerticalResolution = false;
		_tlsRangeResolution = {
			RangeResolution(vec2(-25.0f, -19.0f), 1), RangeResolution(vec2(-19.0f, -14.0f), 1), RangeResolution(vec2(-14.0f, -6.0f), 8), RangeResolution(vec2(-6.0f, 2.0f), 48), 
			RangeResolution(vec2(2.0f, 3.0f), 1), RangeResolution(vec2(3.0f, 5.0f), 1), RangeResolution(vec2(5.0f, 11.0f), 2),  RangeResolution(vec2(11.0f, 15.0f), 1),
		};
		_scanFrequencyHz = 10;
	}
	else if (_LiDARSpecs == LiDARSpecifications::HDL32E)
	{
		_LiDARType = TERRESTRIAL_SPHERICAL;
		_channels = Channels::CH_32;
		_maxRange = 100.0f;
		_peakPower = 12.0f;
		_sensorDiameter = 0.085f;
		_tlsFOVHorizontal = 360.0f;
		_tlsFOVVertical = 41.34f;
		_tlsMiddleAngleVertical = -9.92f;
		_tlsResolutionHorizontal = 3600;
		_tlsResolutionVertical = 31;
		_tlsUniformVerticalResolution = true;
		_maxReturns = 2;
		_scanFrequencyHz = 10;
	}
	else if (_LiDARSpecs == LiDARSpecifications::Puck)
	{
		_LiDARType = TERRESTRIAL_SPHERICAL;
		_channels = Channels::CH_16;
		_maxRange = 100.0f;
		_peakPower = 8.0f;
		_sensorDiameter = 0.103f;
		_tlsFOVHorizontal = 360.0f;
		_tlsFOVVertical = 30.0f;
		_tlsMiddleAngleVertical = .0f;
		_tlsResolutionHorizontal = 3600;
		_tlsResolutionVertical = 15;
		_tlsUniformVerticalResolution = true;
		_maxReturns = 2;
		_scanFrequencyHz = 20;
	}
	else if (_LiDARSpecs == LiDARSpecifications::PuckLite)
	{
		_LiDARType = TERRESTRIAL_SPHERICAL;
		_channels = Channels::CH_16;
		_maxRange = 100.0f;
		_peakPower = 8.0f;
		_sensorDiameter = 0.103f;
		_tlsMiddleAngleVertical = .0f;
		_tlsFOVHorizontal = 360.0f;
		_tlsFOVVertical = 30.0f;
		_tlsResolutionHorizontal = 3600;
		_tlsResolutionVertical = 15;
		_tlsUniformVerticalResolution = true;
		_maxReturns = 2;
		_scanFrequencyHz = 20;
	}
	else if (_LiDARSpecs == LiDARSpecifications::PuckHiRes)
	{
		_LiDARType = TERRESTRIAL_SPHERICAL;
		_channels = Channels::CH_16;
		_maxRange = 100.0f;
		_peakPower = 8.0f;
		_sensorDiameter = 0.103f;
		_tlsMiddleAngleVertical = .0f;
		_tlsFOVHorizontal = 360.0f;
		_tlsFOVVertical = 20.0f;
		_tlsResolutionHorizontal = 3600;
		_tlsResolutionVertical = 15;
		_tlsUniformVerticalResolution = true;
		_maxReturns = 2;
		_scanFrequencyHz = 10;
	}
	else if (_LiDARSpecs == LiDARSpecifications::UltraPuck)
	{
		_LiDARType = TERRESTRIAL_SPHERICAL;
		_channels = Channels::CH_32;
		_maxRange = 200.0f;
		_peakPower = 10.0f;
		_sensorDiameter = 0.103f;
		_tlsMiddleAngleVertical = .0f;
		_tlsFOVHorizontal = 360.0f;
		_tlsFOVVertical = 40.0f;
		_tlsResolutionHorizontal = 3600;
		_tlsResolutionVertical = 122;
		_tlsUniformVerticalResolution = true;
		_maxReturns = 2;
		_scanFrequencyHz = 20;
	}
	else if (_LiDARSpecs == LiDARSpecifications::AlphaPrime)
	{
		_LiDARType = TERRESTRIAL_SPHERICAL;
		_channels = Channels::CH_128;
		_maxRange = 300.0f;
		_peakPower = 22.0f;
		_sensorDiameter = 0.1655f;
		_tlsMiddleAngleVertical = -5.0f;
		_tlsFOVHorizontal = 360.0f;
		_tlsFOVVertical = 40.0f;
		_tlsResolutionHorizontal = 3600;
		_tlsResolutionVertical = 364;
		_tlsUniformVerticalResolution = true;
		_maxReturns = 2;
		_scanFrequencyHz = 20;
	}
	else if (_LiDARSpecs == LiDARSpecifications::Zenmuse_L1)
	{
		_LiDARType = AERIAL_ZIGZAG;
		_channels = Channels::CH_1;
		_alsFOVHorizontal = 70.4f;
		_alsFOVVertical = 4.5f;
		_alsScanFrequency = 10.0f;
		_alsPulseFrequency = 490.0f * 10.0f;
		_maxReturns = 3;
		_tlsUniformVerticalResolution = true;
	}
}
