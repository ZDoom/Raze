/*
** nodebuild_utility.cpp
**
** Miscellaneous node builder utility functions.
**
**---------------------------------------------------------------------------
** Copyright 2002-2006 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
** 4. When not used as part of ZDoom or a ZDoom derivative, this code will be
**    covered by the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or (at
**    your option) any later version.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include <stdlib.h>
#ifdef _MSC_VER
#include <malloc.h>
#endif
#include <string.h>
#include "nodebuild.h"
#include "printf.h"
#include "m_fixed.h"
#include "m_bbox.h"

#if 0
#define D(x) x
#else
#define D(x) do{}while(0)
#endif

#if 0
#define P(x) x
#else
#define P(x) do{}while(0)
#endif

angle_t FNodeBuilder::PointToAngle (fixed_t x, fixed_t y)
{
	const double rad2bam = double(1<<30) / M_PI;
	double ang = g_atan2 (double(y), double(x));
	// Convert to signed first since negative double to unsigned is undefined.
	return angle_t(int(ang * rad2bam)) << 1;
}

void FNodeBuilder::FindUsedVertices (vertex_t *oldverts, int max)
{
	int *map = new int[max];
	int i;
	FPrivVert newvert;

	memset (&map[0], -1, sizeof(int)*max);

	for (i = 0; i < Level.NumLines; ++i)
	{
		ptrdiff_t v1 = Level.Lines[i].v1 - oldverts;
		ptrdiff_t v2 = Level.Lines[i].v2 - oldverts;

		if (map[v1] == -1)
		{
			newvert.x = oldverts[v1].fixX();
			newvert.y = oldverts[v1].fixY();
			map[v1] = VertexMap->SelectVertexExact (newvert);
		}
		if (map[v2] == -1)
		{
			newvert.x = oldverts[v2].fixX();
			newvert.y = oldverts[v2].fixY();
			map[v2] = VertexMap->SelectVertexExact (newvert);
		}

		Level.Lines[i].v1 = (vertex_t *)(size_t)map[v1];
		Level.Lines[i].v2 = (vertex_t *)(size_t)map[v2];
	}
	OldVertexTable = map;
}

// Retrieves the original vertex -> current vertex table.
// Doing so prevents the node builder from freeing it.

const int *FNodeBuilder::GetOldVertexTable()
{
	int *table = OldVertexTable;
	OldVertexTable = NULL;
	return table;
}

// For every sidedef in the map, create a corresponding seg.

void FNodeBuilder::MakeSegsFromSides ()
{
	int i, j;

	for (i = 0; i < Level.NumLines; ++i)
	{
		if (Level.Lines[i].sidedef[0] != NULL)
		{
			CreateSeg (i, 0);
		}
		else
		{
			Printf ("Linedef %d does not have a front side.\n", i);
		}

		if (Level.Lines[i].sidedef[1] != NULL)
		{
			j = CreateSeg (i, 1);
			if (Level.Lines[i].sidedef[0] != NULL)
			{
				Segs[j-1].partner = j;
				Segs[j].partner = j-1;
			}
		}
	}
}

int FNodeBuilder::CreateSeg (int linenum, int sidenum)
{
	FPrivSeg seg;
	int segnum;

	seg.next = UINT_MAX;
	seg.loopnum = 0;
	seg.partner = UINT_MAX;
	seg.hashnext = NULL;
	seg.planefront = false;
	seg.planenum = UINT_MAX;
	seg.storedseg = UINT_MAX;

	if (sidenum == 0)
	{ // front
		seg.frontsector = Level.Lines[linenum].frontsector;
		seg.backsector = Level.Lines[linenum].backsector;
		seg.v1 = (int)(size_t)Level.Lines[linenum].v1;
		seg.v2 = (int)(size_t)Level.Lines[linenum].v2;
	}
	else
	{ // back
		seg.frontsector = Level.Lines[linenum].backsector;
		seg.backsector = Level.Lines[linenum].frontsector;
		seg.v2 = (int)(size_t)Level.Lines[linenum].v1;
		seg.v1 = (int)(size_t)Level.Lines[linenum].v2;
	}
	seg.linedef = linenum;
	side_t *sd = Level.Lines[linenum].sidedef[sidenum];
	seg.sidedef = sd != NULL? sd->Index() : int(NO_SIDE);
	seg.nextforvert = Vertices[seg.v1].segs;
	seg.nextforvert2 = Vertices[seg.v2].segs2;

	segnum = (int)Segs.Push (seg);
	Vertices[seg.v1].segs = segnum;
	Vertices[seg.v2].segs2 = segnum;
	D(Printf(PRINT_LOG, "Seg %4d: From line %d, side %s (%5d,%5d)-(%5d,%5d)  [%08x,%08x]-[%08x,%08x]\n", segnum, linenum, sidenum ? "back " : "front",
		Vertices[seg.v1].x>>16, Vertices[seg.v1].y>>16, Vertices[seg.v2].x>>16, Vertices[seg.v2].y>>16,
		Vertices[seg.v1].x, Vertices[seg.v1].y, Vertices[seg.v2].x, Vertices[seg.v2].y));

	return segnum;
}

// For every seg, create FPrivSegs and FPrivVerts.

void FNodeBuilder::AddSegs(seg_t *segs, int numsegs)
{
	assert(numsegs > 0);

	for (int i = 0; i < numsegs; ++i)
	{
		FPrivSeg seg;
		FPrivVert vert;
		int segnum;

		seg.next = UINT_MAX;
		seg.loopnum = 0;
		seg.partner = UINT_MAX;
		seg.hashnext = NULL;
		seg.planefront = false;
		seg.planenum = UINT_MAX;
		seg.storedseg = UINT_MAX;

		seg.frontsector = segs[i].frontsector;
		seg.backsector = segs[i].backsector;
		vert.x = segs[i].v1->fixX();
		vert.y = segs[i].v1->fixY();
		seg.v1 = VertexMap->SelectVertexExact(vert);
		vert.x = segs[i].v2->fixX();
		vert.y = segs[i].v2->fixY();
		seg.v2 = VertexMap->SelectVertexExact(vert);
		seg.linedef = segs[i].linedef->Index();
		seg.sidedef = segs[i].sidedef != NULL ? segs[i].sidedef->Index() : int(NO_SIDE);
		seg.nextforvert = Vertices[seg.v1].segs;
		seg.nextforvert2 = Vertices[seg.v2].segs2;

		segnum = (int)Segs.Push(seg);
		Vertices[seg.v1].segs = segnum;
		Vertices[seg.v2].segs2 = segnum;
	}
}



// Group colinear segs together so that only one seg per line needs to be checked
// by SelectSplitter().

void FNodeBuilder::GroupSegPlanes ()
{
	const int bucketbits = 12;
	FPrivSeg *buckets[1<<bucketbits] = { 0 };
	int i, planenum;

	for (i = 0; i < (int)Segs.Size(); ++i)
	{
		FPrivSeg *seg = &Segs[i];
		seg->next = i+1;
		seg->hashnext = NULL;
	}

	Segs[Segs.Size()-1].next = UINT_MAX;

	for (i = planenum = 0; i < (int)Segs.Size(); ++i)
	{
		FPrivSeg *seg = &Segs[i];
		fixed_t x1 = Vertices[seg->v1].x;
		fixed_t y1 = Vertices[seg->v1].y;
		fixed_t x2 = Vertices[seg->v2].x;
		fixed_t y2 = Vertices[seg->v2].y;
		angle_t ang = PointToAngle (x2 - x1, y2 - y1);

		if (ang >= 1u<<31)
			ang += 1u<<31;

		FPrivSeg *check = buckets[ang >>= 31-bucketbits];

		while (check != NULL)
		{
			fixed_t cx1 = Vertices[check->v1].x;
			fixed_t cy1 = Vertices[check->v1].y;
			fixed_t cdx = Vertices[check->v2].x - cx1;
			fixed_t cdy = Vertices[check->v2].y - cy1;
			if (PointOnSide (x1, y1, cx1, cy1, cdx, cdy) == 0 &&
				PointOnSide (x2, y2, cx1, cy1, cdx, cdy) == 0)
			{
				break;
			}
			check = check->hashnext;
		}
		if (check != NULL)
		{
			seg->planenum = check->planenum;
			const FSimpleLine *line = &Planes[seg->planenum];
			if (line->dx != 0)
			{
				if ((line->dx > 0 && x2 > x1) || (line->dx < 0 && x2 < x1))
				{
					seg->planefront = true;
				}
				else
				{
					seg->planefront = false;
				}
			}
			else
			{
				if ((line->dy > 0 && y2 > y1) || (line->dy < 0 && y2 < y1))
				{
					seg->planefront = true;
				}
				else
				{
					seg->planefront = false;
				}
			}
		}
		else
		{
			seg->hashnext = buckets[ang];
			buckets[ang] = seg;
			seg->planenum = planenum++;
			seg->planefront = true;

			FSimpleLine pline = { Vertices[seg->v1].x,
								  Vertices[seg->v1].y,
								  Vertices[seg->v2].x - Vertices[seg->v1].x,
								  Vertices[seg->v2].y - Vertices[seg->v1].y };
			Planes.Push (pline);
		}
	}

	D(Printf ("%d planes from %d segs\n", planenum, Segs.Size()));

	PlaneChecked.Reserve ((planenum + 7) / 8);
}

// Just create one plane per seg. Should be good enough for mini BSPs.
void FNodeBuilder::GroupSegPlanesSimple()
{
	Planes.Resize(Segs.Size());
	for (int i = 0; i < (int)Segs.Size(); ++i)
	{
		FPrivSeg *seg = &Segs[i];
		FSimpleLine *pline = &Planes[i];
		seg->next = i+1;
		seg->hashnext = NULL;
		seg->planenum = i;
		seg->planefront = true;
		pline->x = Vertices[seg->v1].x;
		pline->y = Vertices[seg->v1].y;
		pline->dx = Vertices[seg->v2].x - Vertices[seg->v1].x;
		pline->dy = Vertices[seg->v2].y - Vertices[seg->v1].y;
	}
	Segs.Last().next = UINT_MAX;
	PlaneChecked.Reserve((Segs.Size() + 7) / 8);
}

void FNodeBuilder::AddSegToBBox (fixed_t bbox[4], const FPrivSeg *seg)
{
	FPrivVert *v1 = &Vertices[seg->v1];
	FPrivVert *v2 = &Vertices[seg->v2];

	if (v1->x < bbox[BOXLEFT])		bbox[BOXLEFT] = v1->x;
	if (v1->x > bbox[BOXRIGHT])		bbox[BOXRIGHT] = v1->x;
	if (v1->y < bbox[BOXBOTTOM])	bbox[BOXBOTTOM] = v1->y;
	if (v1->y > bbox[BOXTOP])		bbox[BOXTOP] = v1->y;

	if (v2->x < bbox[BOXLEFT])		bbox[BOXLEFT] = v2->x;
	if (v2->x > bbox[BOXRIGHT])		bbox[BOXRIGHT] = v2->x;
	if (v2->y < bbox[BOXBOTTOM])	bbox[BOXBOTTOM] = v2->y;
	if (v2->y > bbox[BOXTOP])		bbox[BOXTOP] = v2->y;
}

void FNodeBuilder::FLevel::FindMapBounds()
{
	double minx, maxx, miny, maxy;

	minx = maxx = Vertices[0].fX();
	miny = maxy = Vertices[0].fY();

	for (int i = 1; i < NumVertices; ++i)
	{
		vertex_t* v = &Vertices[i];
		if (v->fX() < minx) minx = v->fX();
		else if (v->fX() > maxx) maxx = v->fX();
		if (v->fY() < miny) miny = v->fY();
		else if (v->fY() > maxy) maxy = v->fY();
	}

	MinX = FLOAT2FIXED(minx);
	MinY = FLOAT2FIXED(miny);
	MaxX = FLOAT2FIXED(maxx);
	MaxY = FLOAT2FIXED(maxy);
}

FNodeBuilder::IVertexMap::~IVertexMap()
{
}

FNodeBuilder::FVertexMap::FVertexMap (FNodeBuilder &builder,
	fixed_t minx, fixed_t miny, fixed_t maxx, fixed_t maxy)
	: MyBuilder(builder)
{
	MinX = minx;
	MinY = miny;
	BlocksWide = int(((double(maxx) - minx + 1) + (BLOCK_SIZE - 1)) / +BLOCK_SIZE);
	BlocksTall = int(((double(maxy) - miny + 1) + (BLOCK_SIZE - 1)) / +BLOCK_SIZE);
	MaxX = MinX + fixed64_t(BlocksWide) * BLOCK_SIZE - 1;
	MaxY = MinY + fixed64_t(BlocksTall) * BLOCK_SIZE - 1;
	VertexGrid = new TArray<int>[BlocksWide * BlocksTall];
}

FNodeBuilder::FVertexMap::~FVertexMap ()
{
	delete[] VertexGrid;
}

int FNodeBuilder::FVertexMap::SelectVertexExact (FNodeBuilder::FPrivVert &vert)
{
	TArray<int> &block = VertexGrid[GetBlock (vert.x, vert.y)];
	FPrivVert *vertices = &MyBuilder.Vertices[0];
	unsigned int i;

	for (i = 0; i < block.Size(); ++i)
	{
		if (vertices[block[i]].x == vert.x && vertices[block[i]].y == vert.y)
		{
			return block[i];
		}
	}

	// Not present: add it!
	return InsertVertex (vert);
}

int FNodeBuilder::FVertexMap::SelectVertexClose (FNodeBuilder::FPrivVert &vert)
{
	TArray<int> &block = VertexGrid[GetBlock (vert.x, vert.y)];
	FPrivVert *vertices = &MyBuilder.Vertices[0];
	unsigned int i;

	for (i = 0; i < block.Size(); ++i)
	{
#if VERTEX_EPSILON <= 1
		if (vertices[block[i]].x == vert.x && vertices[block[i]].y == vert.y)
#else
		if (abs(vertices[block[i]].x - vert.x) < VERTEX_EPSILON &&
			abs(vertices[block[i]].y - vert.y) < VERTEX_EPSILON)
#endif
		{
			return block[i];
		}
	}

	// Not present: add it!
	return InsertVertex (vert);
}

int FNodeBuilder::FVertexMap::InsertVertex (FNodeBuilder::FPrivVert &vert)
{
	int vertnum;

	vert.segs = UINT_MAX;
	vert.segs2 = UINT_MAX;
	vertnum = (int)MyBuilder.Vertices.Push (vert);

	// If a vertex is near a block boundary, then it will be inserted on
	// both sides of the boundary so that SelectVertexClose can find
	// it by checking in only one block.
	fixed64_t minx = max(MinX, fixed64_t(vert.x) - VERTEX_EPSILON);
	fixed64_t maxx = min(MaxX, fixed64_t(vert.x) + VERTEX_EPSILON);
	fixed64_t miny = max(MinY, fixed64_t(vert.y) - VERTEX_EPSILON);
	fixed64_t maxy = min(MaxY, fixed64_t(vert.y) + VERTEX_EPSILON);

	int blk[4] =
	{
		GetBlock (minx, miny),
		GetBlock (maxx, miny),
		GetBlock (minx, maxy),
		GetBlock (maxx, maxy)
	};
	unsigned int blkcount[4] =
	{
		VertexGrid[blk[0]].Size(),
		VertexGrid[blk[1]].Size(),
		VertexGrid[blk[2]].Size(),
		VertexGrid[blk[3]].Size()
	};
	for (int i = 0; i < 4; ++i)
	{
		if (VertexGrid[blk[i]].Size() == blkcount[i])
		{
			VertexGrid[blk[i]].Push (vertnum);
		}
	}

	return vertnum;
}

FNodeBuilder::FVertexMapSimple::FVertexMapSimple(FNodeBuilder &builder)
	: MyBuilder(builder)
{
}

int FNodeBuilder::FVertexMapSimple::SelectVertexExact(FNodeBuilder::FPrivVert &vert)
{
	FPrivVert *verts = &MyBuilder.Vertices[0];
	unsigned int stop = MyBuilder.Vertices.Size();

	for (unsigned int i = 0; i < stop; ++i)
	{
		if (verts[i].x == vert.x && verts[i].y == vert.y)
		{
			return i;
		}
	}
	// Not present: add it!
	return InsertVertex(vert);
}

int FNodeBuilder::FVertexMapSimple::SelectVertexClose(FNodeBuilder::FPrivVert &vert)
{
	FPrivVert *verts = &MyBuilder.Vertices[0];
	unsigned int stop = MyBuilder.Vertices.Size();

	for (unsigned int i = 0; i < stop; ++i)
	{
#if VERTEX_EPSILON <= 1
		if (verts[i].x == vert.x && verts[i].y == y)
#else
		if (abs(verts[i].x - vert.x) < VERTEX_EPSILON &&
			abs(verts[i].y - vert.y) < VERTEX_EPSILON)
#endif
		{
			return i;
		}
	}
	// Not present: add it!
	return InsertVertex (vert);
}

int FNodeBuilder::FVertexMapSimple::InsertVertex (FNodeBuilder::FPrivVert &vert)
{
	vert.segs = UINT_MAX;
	vert.segs2 = UINT_MAX;
	return (int)MyBuilder.Vertices.Push (vert);
}
