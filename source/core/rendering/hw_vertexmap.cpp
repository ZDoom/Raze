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

#include "basics.h"
#include "maptypes.h"
#include "memarena.h"
#include "gamefuncs.h"
#include "hw_vertexmap.h"

extern FMemArena sectionArena; // allocate from the same arena as the section as the data here has the same lifetime.

TArray<int> vertexMap;	// maps walls to the vertex data.
TArray<vertex_t> vertices;


void CreateVertexMap()
{
	BitArray processed(wall.Size());
	processed.Zero();
	TArray<int> walls;
	TArray<int> sectors;

	vertices.Clear();
	vertexMap.Resize(wall.Size());

	for (unsigned i = 0; i < wall.Size(); i++)
	{
		if (processed[i]) continue;
		walls.Clear();
		sectors.Clear();

		vertexscan(&wall[i], [&](walltype* wal)
			{
				int w = wallnum(wal);
				walls.Push(w);
				processed.Set(w);
				if (!sectors.Contains(wal->sector))
					sectors.Push(wal->sector);
			});

		vertices.Reserve(1);
		auto newvert = &vertices.Last();

		newvert->masterwall = walls[0];
		newvert->viewangle = 0;
		newvert->angletime = 0;
		newvert->dirty = true;
		newvert->numheights = 0;

		// allocate all data within this struct from the arena to simplify memory management.
		auto sect = (int*)sectionArena.Alloc(sectors.Size() * sizeof(int));
		newvert->sectors.Set(sect, sectors.Size());
		memcpy(sect, sectors.Data(), sectors.Size() * sizeof(int));

		auto wals = (int*)sectionArena.Alloc(walls.Size() * sizeof(int));
		newvert->walls.Set(wals, walls.Size());
		memcpy(wals, walls.Data(), walls.Size() * sizeof(int));

		// 2x number of sectors is currently the upper bound for the number of associated heights.
		newvert->heightlist = (float*)sectionArena.Alloc(sectors.Size() * sizeof(float));
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
	dirty = false;
}


