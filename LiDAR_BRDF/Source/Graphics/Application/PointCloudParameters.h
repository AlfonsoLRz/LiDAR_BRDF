#pragma once

/**
*	@file PointCloudParameters.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 02/11/2020
*/

/**
*	@brief Wraps the parameters of a point cloud file (PLY).
*/
struct PointCloudParameters
{
public:
	bool		_asynchronousWrite;					//!<
	char		_filenameBuffer[32];				//!< File where point cloud is saved
	bool		_savePointCloud;					//!< Save point cloud after simulation or not

public:
	/**
	*	@brief Default constructor.
	*/
	PointCloudParameters() :
		_asynchronousWrite(true),
		_filenameBuffer("PointCloud.ply"),
		_savePointCloud(true)
	{
	}

	/**
	*	@brief Destructor.
	*/
	~PointCloudParameters()
	{
	}
};