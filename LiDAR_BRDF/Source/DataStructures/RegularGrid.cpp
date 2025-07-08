#include "stdafx.h"
#include "RegularGrid.h"

/// Public methods

RegularGrid::RegularGrid(const vec2& min, const vec2& max, const uvec2 numDivs) :
	_globalNumPoints(0), _minPoint(min), _maxPoint(max), _numDivs(numDivs)
{
	_cellSize = vec2((max.x - min.x) / float(numDivs.x), (max.y - min.y) / float(numDivs.y));
	_cells.insert(_cells.begin(), numDivs.x, std::vector<GridCell>(numDivs.y));
}

RegularGrid::~RegularGrid()
{
}

Texture* RegularGrid::getDensityTexture()
{
	float densityPixel = .0f;
	float* density = new float[_numDivs.x * _numDivs.y];			// 1D array to build a 2D image

	for (int i = 0; i < _numDivs.x; ++i)
	{
		for (int j = 0; j < _numDivs.y; ++j)
		{
			densityPixel = float(_cells[i][j]._numPoints) / _globalNumPoints;
			density[i * _numDivs.x + j] = densityPixel;
		}
	}

	Texture* texture = new Texture(density, _numDivs.x, _numDivs.y, GL_CLAMP, GL_CLAMP, GL_LINEAR, GL_LINEAR);
	delete[] density;
	
	return texture;
}

void RegularGrid::insertPoint(const vec2& pos, unsigned addition)
{
	ivec2 index = getPositionIndex(pos);

	_cells[index.x][index.y]._numPoints += addition;
	_globalNumPoints = std::max(_globalNumPoints, _cells[index.x][index.y]._numPoints);							// Addition could be negative
}

bool RegularGrid::isSaturated(const vec2& pos, const unsigned threshold)
{
	ivec2 index = getPositionIndex(pos);

	return _cells[index.x][index.y]._numPoints > threshold;
}

void RegularGrid::saturatePoint_Square(const vec2& pos, const unsigned radius, const unsigned addition)
{
	ivec2 index = getPositionIndex(pos);
	ivec2 startIndex = glm::clamp(index - ivec2(radius), ivec2(0), ivec2(_numDivs - 1));
	ivec2 endIndex = glm::clamp(index + ivec2(radius), ivec2(0), ivec2(_numDivs - 1));

	for (int x = startIndex.x; x <= endIndex.x; ++x)
	{
		for (int y = startIndex.y; y <= endIndex.y; ++y)
		{
			_cells[x][y]._numPoints += addition;
			_globalNumPoints = std::max(_globalNumPoints, _cells[x][y]._numPoints);
		}
	}
}

void RegularGrid::saturatePoint_Circular(const vec2& pos, unsigned radius, const unsigned addition)
{
	ivec2 index = getPositionIndex(pos);
	ivec2 startIndex = glm::clamp(index - ivec2(radius), ivec2(0), ivec2(_numDivs) - 1);
	ivec2 endIndex = glm::clamp(index + ivec2(radius), ivec2(0), ivec2(_numDivs) - 1);

	for (int x = startIndex.x; x <= endIndex.x; ++x)
	{
		for (int y = startIndex.y; y <= endIndex.y; ++y)
		{
			ivec2 dir = ivec2(x, y) - index;

			if ((dir.x * dir.x + dir.y * dir.y) <= radius * radius)
			{
				_cells[x][y]._numPoints += addition;
				_globalNumPoints = std::max(_globalNumPoints, _cells[x][y]._numPoints);
			}
		}
	}
}

/// Protected methods	

ivec2 RegularGrid::getPositionIndex(const vec2& pos)
{
	int i = (pos.x - _minPoint.x) / _cellSize.x, j = (pos.y - _minPoint.y) / _cellSize.y;

	return ivec2(glm::clamp(i, 0, _numDivs.x - 1), glm::clamp(j, 0, _numDivs.y - 1));			// Could be outside, nothing guarantees a valid scenario
}