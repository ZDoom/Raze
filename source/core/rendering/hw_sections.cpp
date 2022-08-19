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
#include "hw_sections.h"
#include "memarena.h"
#include "c_cvars.h"

void CreateVertexMap();

FMemArena sectionArena(102400);

TMap<int, bool> bugged;

TArray<SectionLine> sectionLines;
TArray<Section> sections;
TArrayView<TArrayView<int>> sectionsPerSector;
TArray<int> splits;

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

static bool cmpLess(double a, double b)
{
	return a < b;
}

static bool cmpGreater(double a, double b)
{
	return a > b;
}

static int sgn(double v)
{
	// Don't try to be smart here - the compiler won't like it.
	return (v > 0)? 1: (v < 0)? -1 : 0;
}

static int dist(const DVector2& a, const DVector2& b)
{
	// We only need to know if it's 1 or higher, so this is enough.
	return fabs(a.X - b.X) + fabs(a.Y - b.Y);
}


using cmp = bool(*)(double, double);

//==========================================================================
//
// This will also be needed by the triangulator because it faces the same problems with eliminating linedef overlaps.
//
//==========================================================================

void StripLoop(TArray<DVector2>& points)
{
	for (int p = 0; p < (int)points.Size(); p++)
	{
		unsigned prev = p == 0 ? points.Size() - 1 : p - 1;
		unsigned next = unsigned(p) == points.Size() - 1 ? 0 : p + 1;

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
		else if ((points[prev].X == points[p].X && points[next].X == points[p].X && sgn(points[next].Y - points[p].Y) == sgn(points[prev].Y - points[p].Y)) ||
			(points[prev].Y == points[p].Y && points[next].Y == points[p].Y && sgn(points[next].X - points[p].X) == sgn(points[prev].X - points[p].X)) ||
			dist(points[prev], points[next]) <= 1/256.) // if the two points are extremely close together, we may also ignore the intermediate point.
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

int GetWindingOrder(TArray<DVector2>& poly, cmp comp1 = cmpLess, cmp comp2 = cmpGreater)
{
	int n = poly.Size();
	double minx = poly[0].X;
	double miny = poly[0].Y;
	int m = 0;

	for (int i = 0; i < n; i++) 
	{
		if ((comp1(poly[i].Y, miny)) || ((poly[i].Y == miny) && (comp2(poly[i].X, minx))))
		{
			m = i;
			minx = poly[m].X;
			miny = poly[m].Y;
		}
	}

	int64_t a[2], b[2], c[2];

	int m1 = (m + n - 1) % n;
	int m2 = (m + 1) % n;

	a[0] = poly[m1].X;
	b[0] = poly[m].X;
	c[0] = poly[m2].X;

	a[1] = poly[m1].Y;
	b[1] = poly[m].Y;
	c[1] = poly[m2].Y;

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
	TArray<DVector2> points(poly.Size(), true);
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
	visited.Resize(wall.Size());
	visited.Zero();

	TArray<int> thisloop;

	int count = 0;
	for (unsigned i = 0; i < sector.Size(); i++)
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
					sectors.Last().bugged = ESectionFlag::Unclosed;
					bugged.Insert(i, true);
					break;
				}
				if (visited[ww])
				{
					// quick check for the only known cause of this in proper maps: 
					// RRRA E1L3 and SW $yamato have a wall duplicate where the duplicate's index is the original's + 1. These can just be deleted here and be ignored.
					if (ww > 1 && wall[ww-1].pos == wall[ww-2].pos && wall[ww-1].point2 == wall[ww-2].point2 && wall[ww - 1].point2 == ww)
					{
						thisloop.Clear();
						break;
					}
					Printf("found already visited wall %d\nLinked by:", ww);
					bugged.Insert(i, true);
					for (unsigned walnum = 0; walnum < wall.Size(); walnum++)
					{
						if (wall[walnum].point2 == ww)
							Printf(" %d,", walnum);
					}
					Printf("\n");
					sectors.Last().bugged = ESectionFlag::Unclosed;
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
					//Printf("Unable to determine winding order of loop in sector %d!\n", i);
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
	auto pt = wall[vertex].wall_int_pos();
	for (int i = 0; i < 2; i++)
	{
		// to reliably detect walls where vertices lie directly on outer walls, we must test the wall's center as well.
		// SW: Wanton Destrcution's $bath.map, sector 601 is an example for that.
		if (i == 1) pt += wall[vertex].delta() / 2; 
		bool c = false;
		for (unsigned ii = 0; ii < loop.Size() - 1; ii++)
		{
			auto& wal = wall[loop[ii]];
			const auto pt1 = wal.wall_int_pos();
			const auto pt2 = wal.point2Wall()->wall_int_pos();

			if ((pt1.Y >pt.Y) != (pt2.Y > pt.Y)) // skip if both are on the same side.
			{
				// use 64 bit values to avoid overflows in the multiplications below.
				int64_t deltatx = int64_t(pt.X) - pt1.X;
				int64_t deltaty = int64_t(pt.Y) - pt1.Y;
				int64_t deltax = int64_t(pt2.X) - pt1.X;
				int64_t deltay = int64_t(pt2.Y) - pt1.Y;
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
	for (unsigned i = 0; i < sector.Size(); i++)
	{
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
				builder.sections.Last().bugged = ESectionFlag::BadWinding; // Todo: Use flags for bugginess.
				//Printf("Sector %d has wrong winding order\n", i);
				bugged.Insert(i, true);
			}
			continue;
		}
		if (!collect[i].bugged)	// only try to build a proper set of sections if the sector is not malformed. Otherwise just make a single one of everything.
		{
			unsigned wind1count = 0;
			unsigned windnegcount = 0;
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
								//Printf("Bad winding order for loops in sector %d\n", i);
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
							//Printf("Nested loops found in sector %d, comparing loops starting at %d and %d\n", i, sectloops[a][0], sectloops[b][0]);
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
							auto& iloop = sectloops[c];
							builder.sections.Last().wallcount += iloop.Size() - 1;
							builder.sections.Last().loops.Push(std::move(iloop));
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
					builder.sections.Last().bugged = ESectionFlag::Dumped;	// this will most likely require use of the node builder to triangulate anyway.
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
// handle explicit sector splits while we still have simple index arrays.
// This operates on the generated sections
//
//==========================================================================

static void TrySplitLoop(sectionbuildsector& builder, int firstwall, int lastwall)
{
	for (unsigned s = 0; s < builder.sections.Size(); s++)
	{
		auto& section = builder.sections[s];
		if (section.loops.Size() > 1)
		{
			// Must have one loop to split a section. Should this ever be needed for sections with holes the loops need to be joined before running this.
			Printf("Unable to split sector %d between walls %d and %d\n", builder.sectnum, firstwall, lastwall);
			return;
		}
		auto& loop = section.loops[0];
		unsigned i1 = loop.Find(firstwall);
		unsigned i2 = loop.Find(lastwall);
		if (i1 >= loop.Size() || i2 >= loop.Size()) continue;
		if (i2 < i1) std::swap(i1, i2);
		TArray<int> newloop1;
		TArray<int> newloop2;
		auto it = loop.begin();
		auto end = loop.end() - 1;
		while (it != end && *it != firstwall) newloop1.Push(*it++);
		newloop1.Push(-firstwall);
		while (it != end && *it != lastwall) newloop2.Push(*it++);
		newloop2.Push(-lastwall);
		while (it != end) newloop1.Push(*it++);
		section.wallcount = newloop1.Size();
		newloop1.Push(loop.Last());
		section.loops[0] = std::move(newloop1);
		builder.sections.Reserve(1);
		auto& newsect = builder.sections.Last();
		newsect.bugged = false;
		newsect.wallcount = newloop2.Size();
		newloop2.Push(loop.Last());
		newsect.loops.Resize(1);
		newsect.loops[0] = std::move(newloop2);
		break;
	}
}

static void SplitLoops(TArray<sectionbuildsector>& builders)
{
	for (unsigned i = 0; i < splits.Size(); i += 3)
	{
		int sector = splits[0];
		TrySplitLoop(builders[sector], splits[i + 1], splits[i + 2]);
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
	TArray<int> splitwalls;

	// Allocate all Section walls.
	sectionLines.Resize(wall.Size() + splits.Size() * 2 / 3);

	for (unsigned i = 0; i < splits.Size(); i++)
	{
		if (i % 3) splitwalls.Push(splits[i]);
	}

	unsigned nextwall = wall.Size();
	for (unsigned i = 0; i < wall.Size(); i++)
	{
		sectionLines[i].startpoint = i;
		sectionLines[i].endpoint = wall[i].point2;
		sectionLines[i].wall = i;
		sectionLines[i].partner = wall[i].nextwall;
	}
	for (unsigned i = wall.Size(); i < sectionLines.Size(); i++)
	{
		unsigned pair = (i - wall.Size());
		sectionLines[i].startpoint = splitwalls[pair];
		sectionLines[i].endpoint = splitwalls[pair ^ 1];
		sectionLines[i].wall = -1;
		sectionLines[i].partner = wall.Size() + (pair ^ 1);
	}

	unsigned count = 0;
	// allocate as much as possible from the arena here.
	size_t size = sizeof(*sectionsPerSector.Data()) * sector.Size();
	auto data = sectionArena.Calloc(size);
	sectionsPerSector.Set(static_cast<decltype(sectionsPerSector.Data())>(data), sector.Size());

	for (unsigned i = 0; i < sector.Size(); i++)
	{
		auto& builder = builders[i];
		count += builder.sections.Size();

		size = sizeof(int) * builder.sections.Size();
		data = sectionArena.Calloc(size);
		sectionsPerSector[i].Set(static_cast<int* >(data), builder.sections.Size()); // although this may need reallocation, it is too small to warrant single allocations for each sector.
	}
	sections.Resize(count); // this we cannot put into the arena because its size may change.
	memset(sections.Data(), 0, count * sizeof(*sections.Data()));

	// now fill in the data

	int cursection = 0;
	for (unsigned i = 0; i < sector.Size(); i++)
	{
		auto& builder = builders[i];
		for (unsigned j = 0; j < builder.sections.Size(); j++)
		{
			auto section = &sections[cursection];
			auto& srcsect = builder.sections[j];
			sectionsPerSector[i][j] = cursection;
			section->sector = i;
			section->index = cursection++;

			int sectwalls = srcsect.wallcount;
			auto walls = (int*)sectionArena.Calloc(sectwalls * sizeof(int));
			section->lines.Set(walls, sectwalls);

			unsigned srcloops = srcsect.loops.Size();
			auto loops = (Section2Loop*)sectionArena.Calloc(srcloops * sizeof(Section2Loop));
			section->loops.Set(loops, srcloops);

			int curwall = 0;
			for (unsigned ii = 0; ii < srcloops; ii++)
			{
				auto& srcloop = srcsect.loops[ii];
				auto& loop = section->loops[ii];
				unsigned numsectionwalls = srcloop.Size() - 1;
				auto wallarray = (int*)sectionArena.Calloc(numsectionwalls * sizeof(int));
				loop.walls.Set(wallarray, numsectionwalls);
				for (unsigned w = 0; w < numsectionwalls; w++)
				{
					int wall_i = srcloop[w];
					if (wall_i >= 0)
					{
						auto wal = &sectionLines[wall_i];
						section->lines[curwall++] = loop.walls[w] = wall_i;
						wal->section = section->index;
					}
					else
					{
						wall_i = (int)wall.Size() + splitwalls.Find(-wall_i);
						auto wal = &sectionLines[wall_i];
						section->lines[curwall++] = loop.walls[w] = wall_i;
						wal->section = section->index;
					}
				}
			}
		}
	}
	// Can only do this after completing everything else.
	for (auto& line : sectionLines)
	{
		line.partnersection = line.partner == -1 ? -1 : sectionLines[line.partner].section;
	}
	sectionLines.ShrinkToFit();
	sections.ShrinkToFit();
}

//==========================================================================
//
//
//
//==========================================================================

void hw_CreateSections()
{
	bugged.Clear();
	sectionArena.FreeAll();
	sections.Clear();
	sectionLines.Clear();
	TArray<loopcollect> collect;
	CollectLoops(collect);

	TArray<sectionbuildsector> builders(sector.Size(), true);
	GroupData(collect, builders);
	SplitLoops(builders);

	ConstructSections(builders);
	CreateVertexMap();
}

//==========================================================================
//
// Create a set of vertex loops from a given session
//
//==========================================================================

Outline BuildOutline(Section* section)
{
	Outline output(section->loops.Size(), true);
	for (unsigned i = 0; i < section->loops.Size(); i++)
	{
		output[i].Resize(section->loops[i].walls.Size());
		for (unsigned j = 0; j < section->loops[i].walls.Size(); j++)
		{
			output[i][j] = sectionLines[section->loops[i].walls[j]].v1();
		}
		StripLoop(output[i]);
	}
	return output;
}


void hw_SetSplitSector(int sectnum, int start, int end)
{
	splits.Push(sectnum);
	splits.Push(start);
	splits.Push(end);
}

void hw_ClearSplitSector()
{
	splits.Clear();
}
