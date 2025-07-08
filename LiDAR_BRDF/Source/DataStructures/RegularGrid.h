#pragma once

#include "Graphics/Core/Image.h"
#include "Graphics/Core/Texture.h"

/**
*	@file RegularGrid.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 09/02/2020
*/

/**
*	@brief Data structure which helps us to locate models on a terrain.
*/
class RegularGrid
{
protected:
	// Any cell from the regular grid
	struct GridCell
	{
		int _numPoints;								//!< We don't save the points but the number of points

		GridCell() { _numPoints = 0; }
	};

protected:
	std::vector<std::vector<GridCell>> _cells;				//!< Matrix of cells

	vec2			_cellSize;								//!< Number of cells
	int				_globalNumPoints;						//!< Number of points in the complete grid 
	vec2			_maxPoint, _minPoint;					//!< Location of regular grid
	ivec2			_numDivs;								//!< Number of subdivisions of space between mininum and maximum point

protected:
	/**
	*	@brief Grid cell where pos should be placed.
	*/
	ivec2 getPositionIndex(const vec2& pos);

public:
	/**
	*	@brief Constructor which specifies the area and the number of divisions of such area.
	*/
	RegularGrid(const vec2& min, const vec2& max, const uvec2 numDivs);

	/**
	*	@brief Invalid copy constructor.
	*/
	RegularGrid(const RegularGrid& regulargrid) = delete;

	/**
	*	@brief Destructor.
	*/
	virtual ~RegularGrid();

	/**
	*	@return Texture where color depends on the number of points.
	*/
	Texture* getDensityTexture();

	/**
	*	@return Number of subdivisions for both axes.
	*/
	ivec2 getNumSubdivisions() { return _numDivs; }

	/**
	*	@brief Inserts a new point in the grid.
	*	@param addition By default it's 1.
	*/
	void insertPoint(const vec2& pos, unsigned addition = 1);

	/**
	*	@return True if the number of points where pos is located is over threshold.
	*/
	bool isSaturated(const vec2& pos, const unsigned threshold);

	/**
	*	@brief Overloads cells inside an squared radius from pos.
	*/
	void saturatePoint_Square(const vec2& pos, const unsigned radius, const unsigned addition);

	/**
	*	@brief Overloads cells inside a radius from pos by adding 'numPoints' points which adds a quantity equal to addition.
	*/
	void saturatePoint_Circular(const vec2& pos, unsigned radius, const unsigned addition);
};

