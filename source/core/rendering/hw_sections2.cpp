/*
** hw_sections.cpp
** For decoupling the renderer from internal Build structures
**
**---------------------------------------------------------------------------
** Copyright 2021 Christoph Oelckers
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
** The sole reason for existence of this file is that Build's sector setup
** does not allow for easy splitting of sectors, either for having disjoint parts
** or requiring partial rendering. So we need to add a superstructure
** where we can shuffle around the map content without disturbing the original
** order...
**
*/

#include "build.h"
#include "hw_sections2.h"
#include "memarena.h"
#include "c_cvars.h"

CVAR(Bool, hw_sectiondebug, false, 0)
TMap<int, bool> bugged;

static FMemArena sectionArena(102400);
TArray<Section2*> sections2;
TArrayView<TArrayView<Section2*>> sections2PerSector;

struct loopcollect
{
	TArray<TArray<int>> loops;
	int bugged = 0;
};

struct sectionbuild
{
	int bugged = 0;
	int wallcount = 0;
	TArray<TArray<int>> loops;
};

struct sectionbuildsector
{
	int sectnum;
	TArray<sectionbuild> sections;
};

static bool cmpLess(int a, int b)
{
	return a < b;
}

static bool cmpGreater(int a, int b)
{
	return a > b;
}

static int sgn(int v)
{
	return (v > 0) - (v < 0);
}

static int dist(const vec2_t& a, const vec2_t& b)
{
	// We only need to know if it's 1 or higher, so this is enough.
	return abs(a.x - b.x) + abs(a.y - b.y);
}


using cmp = bool(*)(int, int);

//==========================================================================
//
// This will also be needed by the triangulator because it faces the same problems with eliminating linedef overlaps.
//
//==========================================================================

void StripLoop(TArray<vec2_t>& points)
{
	for (int p = 0; p < (int)points.Size(); p++)
	{
		unsigned prev = p == 0 ? points.Size() - 1 : p - 1;
		unsigned next = p == points.Size() - 1 ? 0 : p + 1;

		if (points[next] == points[prev]) // if the two neighboring points are equal, this one dos not contribute to the sector's area.
		{
			if (next == 0)
			{
				points.Delete(0);
				points.Pop();
			}
			else
			{
				points.Delete(p, 2);
				p--;
			}
			if (p > 0) p--; // backtrack one point more to ensure we can check the newly formed connection as well.
		}
		else if ((points[prev].x == points[p].x && points[next].x == points[p].x && sgn(points[next].y - points[p].y) == sgn(points[prev].y - points[p].y)) ||
			(points[prev].y == points[p].y && points[next].y == points[p].y && sgn(points[next].x - points[p].x) == sgn(points[prev].x - points[p].x)) ||
			dist(points[prev], points[next]) <= 1) // if the two points are extremely close together, we may also ignore the intermediate point.
		{
			// both connections exit the point into the same direction. Here it is sufficient to just delete it so that the neighboring ones connect directly.
			points.Delete(p);
			p--;
			if (p > 0) p--; // backtrack one point more to ensure we can check the newly formed connection as well.
		}
		// Todo: check the non-orthogonal case of the above, too. Duke E2L7's sector 130 is such a case.
	}
}


//==========================================================================
//
//
//
//==========================================================================

int GetWindingOrder(TArray<vec2_t>& poly, cmp comp1 = cmpLess, cmp comp2 = cmpGreater)
{
	int n = poly.Size();
	int minx = poly[0].x;
	int miny = poly[0].y;
	int m = 0;

	for (int i = 0; i < n; i++) 
	{
		if ((comp1(poly[i].y, miny)) || ((poly[i].y == miny) && (comp2(poly[i].x, minx))))
		{
			m = i;
			minx = poly[m].x;
			miny = poly[m].y;
		}
	}

	int64_t a[2], b[2], c[2];

	int m1 = (m + n - 1) % n;
	int m2 = (m + 1) % n;

	a[0] = poly[m1].x;
	b[0] = poly[m].x;
	c[0] = poly[m2].x;

	a[1] = poly[m1].y;
	b[1] = poly[m].y;
	c[1] = poly[m2].y;

	auto area =
		a[0] * b[1] - a[1] * b[0] +
		a[1] * c[0] - a[0] * c[1] +
		b[0] * c[1] - c[0] * b[1];

	return (area > 0) - (area < 0); 
}

int GetWindingOrder(TArray<int>& poly)
{
	// This is more complicated than it should be due to how doors are designed in Build. 
	// Overlapping and backtracking lines are quite common and need to be removed from the data before determining the winding order.
	TArray<vec2_t> points(poly.Size(), true);
	int i = 0;
	for (auto& index : poly)
	{
		points[i++] = wall[index].pos;
	}
	StripLoop(points);
	if (points.Size() == 0) return 1; // Sector has no dimension. We must accept this as valid here.
	int order = GetWindingOrder(points);
	if (order == 0)
	{
		// this may be a diagonal overlap, so try one other corner, too.
		order = GetWindingOrder(points, cmpGreater, cmpLess);
	}
	// if (order == 0) do a pedantic check - this is hopefully not needed ever.
	return order;
}

//==========================================================================
//
//
//
//==========================================================================

static void CollectLoops(TArray<loopcollect>& sectors)
{
	BitArray visited;
	visited.Resize(numwalls);
	visited.Zero();

	TArray<int> thisloop;
	
	int count = 0;
	for (int i = 0; i < numsectors; i++)
	{
		int first = sector[i].wallptr;
		int last = first + sector[i].wallnum;
		sectors.Reserve(1);
		sectors.Last().bugged = 0;
		
		for (int w = first; w < last; w++)
		{
			if (visited[w]) continue;
			thisloop.Clear();
			thisloop.Push(w);
			visited.Set(w);
			
			for (int ww = wall[w].point2; ww != w; ww = wall[ww].point2)
			{
				if (ww < first || ww >= last)
				{
					Printf("Found wall %d outside sector %d in a loop\n", ww, i);
					sectors.Last().bugged = ESEctionFlag::Unclosed;
					bugged.Insert(i, true);
					break;
				}
				if (visited[ww])
				{
					// quick check for the only known cause of this in proper maps: 
					// RRRA E1L3 and SW $yamato have a wall duplicate where the duplicate's index is the original's + 1. These can just be deleted here and be ignored.
					if (ww > 1 && wall[ww-1].x == wall[ww-2].x && wall[ww-1].y == wall[ww-2].y && wall[ww-1].point2 == wall[ww-2].point2 && wall[ww - 1].point2 == ww)
					{
						thisloop.Clear();
						break;
					}
					Printf("found already visited wall %d\nLinked by:", ww);
					bugged.Insert(i, true);
					for (int i = 0; i < numwalls; i++)
					{
						if (wall[i].point2 == ww)
							Printf(" %d,", i);
					}
					Printf("\n");
					sectors.Last().bugged = ESEctionFlag::Unclosed;
					break;
				}
				thisloop.Push(ww);
				visited.Set(ww);
			}
			if (thisloop.Size() > 0)
			{
				count++;
				int o = GetWindingOrder(thisloop);
				if (o == 0)
				{
					Printf("Unable to determine winding order of loop in sector %d!\n", i);
					bugged.Insert(i, true);
				}

				thisloop.Push(o);
				sectors.Last().loops.Push(std::move(thisloop));
			}
		}
	}
}

//==========================================================================
//
// checks if a point is within a given section
//
// Completely redone based on outside information.
// The math in here is based on this article: https://wrf.ecse.rpi.edu/Research/Short_Notes/pnpoly.html
// Copyright (c) 1970-2003, Wm. Randolph Franklin , licensed under BSD 3-clause
// but was transformed to avoid the division it contained and to properly pick the vertices of Build walls.
//
// (not used in-game because it is not 100% identical to Build's original check and causing issues in SW.)
// 
//==========================================================================

static int insideLoop(int vertex, TArray<int>& loop)
{
	auto pt = wall[vertex].pos;
	for (int i = 0; i < 2; i++)
	{
		// to reliably detect walls where vertices lie directly on outer walls, we must test the wall's center as well.
		// SW: Wanton Destrcution's $bath.map, sector 601 is an example for that.
		if (i == 1) pt += wall[vertex].delta() / 2; 
		bool c = false;
		for (unsigned i = 0; i < loop.Size() - 1; i++)
		{
			auto& wal = wall[loop[i]];
			auto& pt1 = wal.pos;
			auto& pt2 = wal.point2Wall()->pos;

			if ((pt1.y >pt.y) != (pt2.y > pt.y)) // skip if both are on the same side.
			{
				// use 64 bit values to avoid overflows in the multiplications below.
				int64_t deltatx = int64_t(pt.x) - pt1.x;
				int64_t deltaty = int64_t(pt.y) - pt1.y;
				int64_t deltax = int64_t(pt2.x) - pt1.x;
				int64_t deltay = int64_t(pt2.y) - pt1.y;
				//if (x < deltax * (deltaty) / deltay + pt1.x)
				// reformatted to avoid the division - for nagative deltay the sign needs to be flipped to give the correct result.
				int64_t result = ((deltay * deltatx - deltax * deltaty) ^ deltay);
				if (result < 0)
					c = !c;
			}
		}
		if (i == 1 || c == 1) return int(c);
	}
	return -1;
}

static int insideLoop(TArray<int>& check, TArray<int>& loop)
{
	for (unsigned v = 0; v < check.Size() - 1; v++)
	{
		if (insideLoop(check[v], loop) == 1)
		{
			return true;
		}
	}
	return false;
}

//==========================================================================
//
//
//
//==========================================================================

static void GroupData(TArray<loopcollect>& collect, TArray<sectionbuildsector>& builders)
{
	for (int i = 0; i < numsectors; i++)
	{
		if (i == 250)
		{
			int a = 0;
		}

		auto& builder = builders[i];
		builder.sectnum = i;
		auto& sectloops = collect[i].loops;

		// Handle the two easy cases  explicitly so that they can be done without running into more complex checks
		if (sectloops.Size() == 1)
		{
			// we got one loop - do this quickly without any checks.
			auto& loop = sectloops[0];
			builder.sections.Reserve(1);
			int last = loop.Last();
			builder.sections.Last().wallcount = loop.Size() - 1;
			builder.sections.Last().loops.Push(std::move(loop));
			builder.sections.Last().bugged = collect[i].bugged;
			if (last != 1)
			{
				builder.sections.Last().bugged = ESEctionFlag::BadWinding; // Todo: Use flags for bugginess.
				Printf("Sector %d has wrong winding order\n", i);
				bugged.Insert(i, true);
			}
			continue;
		}
		if (!collect[i].bugged)	// only try to build a proper set of sections if the sector is not malformed. Otherwise just make a single one of everything.
		{
			int wind1count = 0;
			int windnegcount = 0;
			int posplace = -1;
			for (unsigned l = 0; l < sectloops.Size(); l++)
			{
				auto& loop = sectloops[l];
				if (loop.Last() == 1)
				{
					wind1count++;
					posplace = l;
				}
				else if (loop.Last() == -1) windnegcount++;
			}
			// Check for one outer loop with multiple inner loops. This is also fairly common and quickly found.
			if (wind1count == 1 && windnegcount == sectloops.Size() - 1)
			{
				if (posplace > 0) sectloops[0].Swap(sectloops[posplace]);
				unsigned insidecount = 0;
				for (unsigned l = 1; l < sectloops.Size(); l++)
				{
					if (insideLoop(sectloops[l], sectloops[0])) insidecount++;
				}
				if (insidecount == sectloops.Size() - 1)
				{
					builder.sections.Reserve(1);
					builder.sections.Last().wallcount = 0;
					builder.sections.Last().bugged = 0;
					for (auto& loop : sectloops)
					{
						builder.sections.Last().wallcount += loop.Size() - 1;
						builder.sections.Last().loops.Push(std::move(loop));
					}
					continue;
				}
			}
			// Check for multiple outer loops with no inner loops. Less frequent, but still a regular occurence.
			if (wind1count == sectloops.Size() && windnegcount == 0)
			{
				for (auto& loop1 : sectloops) for (auto& loop2 : sectloops)
				{
					if (&loop1 != &loop2 && insideLoop(loop1, loop2))
					{
						goto nope; // just get out of here.
					}
				}
				for (auto& loop : sectloops)
				{
					builder.sections.Reserve(1);
					builder.sections.Last().bugged = 0;
					builder.sections.Last().wallcount = loop.Size() - 1;
					builder.sections.Last().loops.Push(std::move(loop));
				}
				continue;
			}
		nope:;

			// Now try the case where we got multiple sections where some have holes.
			// For that, first build a map to see which sectors lie inside others.
			TArray<int> inside(sectloops.Size(), true);
			TArray<TArray<int>> outside(sectloops.Size(), true);
			for (auto& in : inside) in = -1;
			for (unsigned a = 0; a < sectloops.Size(); a++)
			{
				for (unsigned b = 0; b < sectloops.Size(); b++)
				{
					if (b != a && insideLoop(sectloops[a], sectloops[b]))
					{
						if (inside[a] == -1)
						{
							if (sectloops[a].Last() != -1 || sectloops[b].Last() != 1)
							{
								Printf("Bad winding order for loops in sector %d\n", i);
								bugged.Insert(i, true);
								inside[a] = inside[b] = -2; // invalidate both loops
							}
							else
							{
								inside[a] = b;
								outside[b].Push(a);
							}
						}
						else
						{
							Printf("Nested loops found in sector %d, comparing loops starting at %d and %d\n", i, sectloops[a][0], sectloops[b][0]);
							bugged.Insert(i, true);
							if (inside[a] != -2)
							{
								inside[inside[a]] = -2;
							}
							inside[a] = inside[b] = -2;
						}
					}
				}
			}
			// Now write out the proper sections we were able to find.
			for (unsigned a = 0; a < sectloops.Size(); a++)
			{
				if (inside[a] == -1 && sectloops[a].Size() > 0 && sectloops[a].Last() == 1)
				{
					auto& loop = sectloops[a];
					builder.sections.Reserve(1);
					builder.sections.Last().bugged = -1; // debug only - remove once checked!!!
					builder.sections.Last().wallcount = loop.Size() - 1;
					builder.sections.Last().loops.Push(std::move(loop));
					for (auto c: outside[a])
					{
						if (inside[c] == a)
						{
							auto& loop = sectloops[c];
							builder.sections.Last().wallcount += loop.Size() - 1;
							builder.sections.Last().loops.Push(std::move(loop));
							inside[c] = -1;
						}
					}
				}
			}
		}

		// Whatever gets here is in a shape where any guesswork is futile. Just dump it into a single section and don't think about it any further.
		bool tossit = false;
		for (unsigned a = 0; a < sectloops.Size(); a++)
		{
			if (sectloops[a].Size() > 0)
			{
				if (!tossit) // Have we created our dumping section yet? If no, do so now and print a warning.
				{
					tossit = true;
					Printf("Potential problem at sector %d with %d loops\n", i, sectloops.Size());
					bugged.Insert(i, true);
					builder.sections.Reserve(1);
					builder.sections.Last().bugged = ESEctionFlag::Dumped;	// this will most likely require use of the node builder to triangulate anyway.
				}
				auto& loop = sectloops[a];
				builder.sections.Last().wallcount += loop.Size() - 1;
				builder.sections.Last().loops.Push(std::move(loop));
			}
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

static void ConstructSections(TArray<sectionbuildsector>& builders)
{
	// count all sections and allocate the global buffers.

	// Allocate all Section walls.
	TArray<Section2Wall*> wallmap(numwalls, true);
	for (int i = 0; i < numwalls; i++)
	{
		wallmap[i] = (Section2Wall*)sectionArena.Calloc(sizeof(Section2Wall));
	}
	for (int i = 0; i < numwalls; i++)
	{
		wallmap[i]->v1 = &wall[i].pos;
		wallmap[i]->v2 = &wall[i].point2Wall()->pos;
		wallmap[i]->wall = &wall[i];
		wallmap[i]->backside = validWallIndex(wall[i].nextwall)? wallmap[wall[i].nextwall] : nullptr;
	}

	unsigned count = 0;
	// allocate as much as possible from the arena here.
	size_t size = sizeof(*sections2PerSector.Data()) * numsectors;
	auto data = sectionArena.Calloc(size);
	sections2PerSector.Set(static_cast<decltype(sections2PerSector.Data())>(data), numsectors);

	for (int i = 0; i < numsectors; i++)
	{
		auto& builder = builders[i];
		count += builder.sections.Size();

		size = sizeof(Section2*) * builder.sections.Size();
		data = sectionArena.Calloc(size);
		sections2PerSector[i].Set(static_cast<Section2** >(data), builder.sections.Size()); // although this may need reallocation, it is too small to warrant single allocations for each sector.
	}
	sections2.Resize(count); // this we cannot put into the arena because its size may change.
	memset(sections2.Data(), 0, count * sizeof(*sections2.Data()));

	// now fill in the data

	int cursection = 0;
	for (int i = 0; i < numsectors; i++)
	{
		auto& builder = builders[i];
		for (unsigned j = 0; j < builder.sections.Size(); j++)
		{
			auto section = (Section2*)sectionArena.Calloc(sizeof(Section2));
			auto& srcsect = builder.sections[j];
			sections2[cursection] = section;
			sections2PerSector[i][j] = section;
			section->sector = &sector[i];
			section->index = cursection++;

			int sectwalls = srcsect.wallcount;
			auto walls = (Section2Wall**)sectionArena.Calloc(sectwalls * sizeof(Section2Wall*));
			section->walls.Set(walls, sectwalls);

			unsigned srcloops = srcsect.loops.Size();
			auto loops = (Section2Loop*)sectionArena.Calloc(srcloops * sizeof(Section2Loop));
			section->loops.Set(loops, srcloops);

			int curwall = 0;
			for (unsigned i = 0; i < srcloops; i++)
			{
				auto& srcloop = srcsect.loops[i];
				auto& loop = section->loops[i];
				unsigned numwalls = srcloop.Size() - 1;
				auto walls = (Section2Wall**)sectionArena.Calloc(numwalls * sizeof(Section2Wall*));
				loop.walls.Set(walls, numwalls);
				for (unsigned w = 0; w < numwalls; w++)
				{
					auto wal = wallmap[srcloop[w]];
					section->walls[curwall++] = loop.walls[w] = wal;
					wal->frontsection = section;
					// backsection will be filled in when everything is done.
				}
			}
		}
	}
	for (auto& wal : wallmap)
		if (wal->backside) wal->backsection = wal->backside->frontsection;
}

//==========================================================================
//
//
//
//==========================================================================

void hw_CreateSections2()
{
	bugged.Clear();
	sectionArena.FreeAll();
	sections2.Reset();
	TArray<loopcollect> collect;
	CollectLoops(collect);

	TArray<sectionbuildsector> builders(numsectors, true);
	GroupData(collect, builders);

	ConstructSections(builders);

	if (hw_sectiondebug)
	{
		for (int i = 0; i < numsectors; i++)
		{
			//if (sections2PerSector[i][0]->flags == 0 && !bugged.CheckKey(i)) continue;	
			Printf(PRINT_LOG, "Sector %d, %d walls, %d sections\n", i, sector[i].wallnum, sections2PerSector[i].Size());
			for (auto& section : sections2PerSector[i])
			{
				Printf(PRINT_LOG, "\tSection %d, %d loops, flags = %d\n", section->index, section->loops.Size(), section->flags);
				for (auto& loop : section->loops)
				{
					Printf(PRINT_LOG, "\t\tLoop, %d walls\n", loop.walls.Size());
					for (auto& wall : loop.walls)
					{
						Printf(PRINT_LOG, "\t\t\tWall %d, (%d, %d) -> (%d, %d)", ::wall.IndexOf(wall->wall), wall->v1->x / 16, wall->v1->y / -16, wall->v2->x / 16, wall->v2->y / -16);
						if (wall->wall->nextwall == -1) Printf(PRINT_LOG, "one-sided\n");
						else
						{
							Printf(PRINT_LOG, " next wall = %d, next sector = %d", wall->wall->nextwall, wall->wall->nextsector);
							if (wall->wall->nextWall()->nextWall() != wall->wall) Printf(PRINT_LOG, " unreachable");
							Printf(PRINT_LOG, "\n");
						}
					}
				}
			}
		}
	}
}

//==========================================================================
//
// Create a set of vertex loops from a given session
//
//==========================================================================

Outline BuildOutline(Section2* section)
{
	Outline output(section->loops.Size(), true);
	for (unsigned i = 0; i < section->loops.Size(); i++)
	{
		output[i].Resize(section->loops[i].walls.Size());
		for (unsigned j = 0; j < section->loops[i].walls.Size(); j++)
		{
			output[i][j] = *section->loops[i].walls[j]->v1;
		}
		StripLoop(output[i]);
	}
	return output;
}


#include "c_dispatch.h"
#include "engineerrors.h"

CCMD(sectiontest)
{
	TMap<FString, bool> processed;
	for (int i = 0; i < fileSystem.GetNumEntries(); i++)
	{
		FString f = FString(fileSystem.GetFileFullName(i)).MakeUpper();
		if (f.IndexOf(".MAP") == f.Len() - 4)
		{
			if (processed.CheckKey(f)) continue;
			processed.Insert(f, true);
			Printf(PRINT_LOG, "Checking %s\n--------------------------\n", f.GetChars());
			try
			{
				loadMapBackup(f);
			}
			catch (const CRecoverableError& error)
			{
				Printf(PRINT_LOG, "Unable to load map: %s\n", error.what());
			}
			Printf(PRINT_LOG, "\n\n");
			I_DebugPrint(f);
			I_DebugPrint("\n");
		}
	}
}