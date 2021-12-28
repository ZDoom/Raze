#pragma once

#include "basics.h"
#include "tarray.h"


struct vertex_t
{
	int masterwall;
	angle_t viewangle;	// precalculated angle for clipping
	int angletime;		// recalculation time for view angle
	bool dirty;			// something has changed and needs to be recalculated
	int numheights;
	TArrayView<int> sectors;
	TArrayView<int> walls;
	float * heightlist;

	void RecalcVertexHeights();
};

extern TArray<int> vertexMap;	// maps walls to the vertex data.
extern TArray<vertex_t> vertices;
