/*
** gl_vertexmap.cpp
** Vertex management for precise wall rendering.
**
**---------------------------------------------------------------------------
** Copyright 2021-2022 Christoph Oelckers
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
**
*/

#include "maptypes.h"
#include "memarena.h"
#include "gamefuncs.h"
#include "hw_vertexmap.h"

extern FMemArena sectionArena; // allocate from the same arena as the sections as the data here has the same lifetime.

TArray<int> vertexMap;	// maps walls to the vertex data.
TArray<vertex_t> vertices;
TArray<TArrayView<int>> verticespersector;


void CreateVertexMap()
{
	BitArray processed(wall.Size());
	processed.Zero();
	TArray<int> walls;
	TArray<int> sectors;
	TArray<int> countpersector(sector.Size(), true);

	vertices.Clear();
	vertexMap.Resize(wall.Size());
	verticespersector.Resize(sector.Size());

	for (auto& c : countpersector) c = 0;
	for (auto& c : vertexMap) c = -1;
	for (unsigned i = 0; i < wall.Size(); i++)
	{
		if (processed[i]) continue;
		walls.Clear();
		sectors.Clear();

		vertexscan(&wall[i], [&](walltype* wal)
			{
				int w = wallnum(wal);
				if (processed[w]) return;	// broken wall setups can trigger this.
				walls.Push(w);
				processed.Set(w);
				if (!sectors.Contains(wal->sector))
				{
					sectors.Push(wal->sector);
					countpersector[wal->sector]++;
				}
			});

		unsigned index = vertices.Reserve(1);
		auto newvert = &vertices.Last();

		newvert->masterwall = walls[0];
		newvert->viewangle = 0;
		newvert->angletime = 0;
		newvert->dirty = true;
		newvert->numheights = 0;

		for (auto w : walls)
		{
			vertexMap[w] = index;
		}

		// allocate all data within this struct from the arena to simplify memory management.
		auto sect = (int*)sectionArena.Alloc(sectors.Size() * sizeof(int));
		newvert->sectors.Set(sect, sectors.Size());
		memcpy(sect, sectors.Data(), sectors.Size() * sizeof(int));

		auto wals = (int*)sectionArena.Alloc(walls.Size() * sizeof(int));
		newvert->walls.Set(wals, walls.Size());
		memcpy(wals, walls.Data(), walls.Size() * sizeof(int));

		// 2x number of sectors is currently the upper bound for the number of associated heights.
		newvert->heightlist = (float*)sectionArena.Alloc(sectors.Size() * sizeof(float));

		// create the inverse map to assign vertices to sectors. This is needed by the dirty marking code.
		for (unsigned ii = 0; ii < sector.Size(); ii++)
		{
			auto sdata = (int*)sectionArena.Alloc(countpersector[ii] * sizeof(int));
			verticespersector[ii].Set(sdata, countpersector[ii]);
			countpersector[ii] = 0;
		}
		for (unsigned ii = 0; ii < vertices.Size(); ii++)
		{
			for (auto sec : vertices[ii].sectors)
			{
				verticespersector[sec][countpersector[sec]++] = ii;
			}
		}
	}
#if 0
	for (unsigned i = 0; i < vertices.Size(); i++)
	{
		Printf("Vertex %d at (%2.3f, %2.3f)\n", i, wall[vertices[i].masterwall].pos.X / 16., wall[vertices[i].masterwall].pos.Y / -16.);
		Printf("    Walls: ");
		for (auto wal : vertices[i].walls) Printf("%d ", wal);
		Printf("\n");
		Printf("    Sectors: ");
		for (auto wal : vertices[i].sectors) Printf("%d ", wal);
		Printf("\n");
	}
#endif
}

//==========================================================================
//
// 
//
//==========================================================================

void MarkVerticesForSector(int sector)
{
	for (auto vert : verticespersector[sector])
	{
		vertices[vert].dirty = true;
	}
}

//==========================================================================
//
// Recalculate all heights affecting this vertex.
//
//==========================================================================

void vertex_t::RecalcVertexHeights()
{
	numheights = 0;
	dirty = false;
	if (sectors.Size() == 1) return;	// no need to bother
	for (auto& sect : sectors)
	{
		float heights[2];

		auto point = wall[masterwall].pos;
		PlanesAtPoint(&sector[sect], point.X, point.Y, &heights[0], &heights[1]);
		for(auto height : heights)
		{
			int k;
			for ( k = 0; k < numheights; k++)
			{
				if (height == heightlist[k]) break;
				if (height < heightlist[k])
				{
					memmove(&heightlist[k + 1], &heightlist[k], sizeof(float) * (numheights - k));
					heightlist[k] = height;
					numheights++;
					break;
				}
			}
			if (k == numheights) heightlist[numheights++] = height;
		}
	}
	if (numheights <= 2) numheights = 0;	// is not in need of any special attention
}


