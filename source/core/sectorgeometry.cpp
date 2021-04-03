/*
** sectorgeometry.cpp
**
** caches the triangle meshes used for rendering sector planes.
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
**
*/  

#include "sectorgeometry.h"
#include "build.h"
#include "gamefuncs.h"
#include "texturemanager.h"
#include "earcut.hpp"
#include "nodebuilder/nodebuild.h"

SectorGeometry sectorGeometry;

//==========================================================================
//
// CalcPlane fixme - this should be stored in the sector, not be recalculated each frame.
//
//==========================================================================

static FVector3 CalcNormal(sectortype* sector, int plane)
{
	FVector3 pt[3];

	auto wal = &wall[sector->wallptr];
	auto wal2 = &wall[wal->point2];

	pt[0] = { (float)WallStartX(wal), (float)WallStartY(wal), 0 };
	pt[1] = { (float)WallEndX(wal), (float)WallEndY(wal), 0 };
	PlanesAtPoint(sector, wal->x, wal->y, plane ? &pt[0].Z : nullptr, plane? nullptr : &pt[0].Z);
	PlanesAtPoint(sector, wal2->x, wal2->y, plane ? &pt[1].Z : nullptr, plane ? nullptr : &pt[1].Z);

	if (pt[0].X == pt[1].X)
	{
		if (pt[0].Y == pt[1].Y) return { 0.f, 0.f, plane ? -1.f : 1.f };
		pt[2].X = pt[0].X + 4;
		pt[2].Y = pt[0].Y;
	}
	else
	{
		pt[2].X = pt[0].X;
		pt[2].Y = pt[0].Y + 4;
	}
	PlanesAtPoint(sector, pt[2].X * 16, pt[2].Y * 16, plane ? &pt[2].Z : nullptr, plane ? nullptr : &pt[2].Z);

	auto normal = (pt[2] - pt[0]) ^ (pt[1] - pt[0]);

	if ((pt[2].Z < 0 && !plane) || (pt[2].Z > 0 && plane)) return -pt[2];
	return pt[2];
}

//==========================================================================
//
// The math used here to calculate texture positioning was derived from
// Polymer but required several fixes for correctness. 
//
//==========================================================================
class UVCalculator
{
	sectortype* sect;
	int myplane;
	int stat;
	float z1;
	int ix1;
	int iy1;
	int ix2;
	int iy2;
	float sinalign, cosalign;
	FGameTexture* tex;
	float xpanning, ypanning;
	float xscaled, yscaled;
	FVector2 offset;

public:

	// Moved in from pragmas.h
	UVCalculator(sectortype* sec, int plane, FGameTexture* tx, const FVector2& off)
	{
		float xpan, ypan;

		sect = sec;
		tex = tx;
		myplane = plane;
		offset = off;

		auto firstwall = &wall[sec->wallptr];
		ix1 = firstwall->x;
		iy1 = firstwall->y;
		ix2 = wall[firstwall->point2].x;
		iy2 = wall[firstwall->point2].y;

		if (plane == 0)
		{
			stat = sec->floorstat;
			xpan = sec->floorxpan_;
			ypan = sec->floorypan_;
			PlanesAtPoint(sec, ix1, iy1, nullptr, &z1);
		}
		else
		{
			stat = sec->ceilingstat;
			xpan = sec->ceilingxpan_;
			ypan = sec->ceilingypan_;
			PlanesAtPoint(sec, ix1, iy1, &z1, nullptr);
		}

		DVector2 dv = { double(ix2 - ix1), -double(iy2 - iy1) };
		auto vang = dv.Angle() - 90.;

		cosalign = vang.Cos();
		sinalign = vang.Sin();

		int pow2width = 1 << sizeToBits((int)tx->GetDisplayWidth());
		if (pow2width < (int)tx->GetDisplayWidth()) pow2width *= 2;

		int pow2height = 1 << sizeToBits((int)tx->GetDisplayHeight());
		if (pow2height < (int)tx->GetDisplayHeight()) pow2height *= 2;

		xpanning = pow2width * xpan / (256.f * tx->GetDisplayWidth());
		ypanning = pow2height * ypan / (256.f * tx->GetDisplayHeight());

		float scalefactor = (stat & CSTAT_SECTOR_TEXHALF) ? 8.0f : 16.0f;

		if ((stat & (CSTAT_SECTOR_SLOPE | CSTAT_SECTOR_ALIGN)) == (CSTAT_SECTOR_ALIGN))
		{
			// This is necessary to adjust for some imprecisions in the math.
			// To calculate the inverse Build performs an integer division with significant loss of precision
			// that can cause the texture to be shifted by multiple pixels.
			// The code below calculates the amount of this deviation so that it can be added back to the formula.
			int len = ksqrt(uhypsq(ix2 - ix1, iy2 - iy1));
			if (len != 0)
			{
				int i = 1048576 / len;
				scalefactor *= 1048576.f / (i * len);
			}
		}

		xscaled = scalefactor * (int)tx->GetDisplayWidth();
		yscaled = scalefactor * (int)tx->GetDisplayHeight();
	}

	FVector2 GetUV(int x, int y, float z)
	{
		float tv, tu;

		if (stat & CSTAT_SECTOR_ALIGN)
		{
			float dx = (float)(x - ix1);
			float dy = (float)(y - iy1);

			tu = -(dx * sinalign + dy * cosalign);
			tv = (dx * cosalign - dy * sinalign);

			if (stat & CSTAT_SECTOR_SLOPE)
			{
				float dz = (z - z1) * 16;
				float newtv = sqrt(tv * tv + dz * dz);
				tv = tv < 0 ? -newtv : newtv;
			}
		}
		else 
		{
			tu = x - offset.X;
			tv = -y - offset.Y;
		}

		if (stat & CSTAT_SECTOR_SWAPXY)
			std::swap(tu, tv);

		if (stat & CSTAT_SECTOR_XFLIP) tu = -tu;
		if (stat & CSTAT_SECTOR_YFLIP) tv = -tv;



		return { tu / xscaled + xpanning, tv / yscaled + ypanning };

	}
};


//==========================================================================
//
//
//
//==========================================================================

bool SectorGeometry::MakeVertices(unsigned int secnum, int plane, const FVector2& offset)
{
	auto sec = &sector[secnum];
	int numvertices = sec->wallnum;
	
	TArray<FVector3> points(numvertices, true);
	using Point = std::pair<float, float>;
	std::vector<std::vector<Point>> polygon;
	std::vector<Point>* curPoly;

	polygon.resize(1);
	curPoly = &polygon.back();
	FixedBitArray<MAXWALLSB> done;

	int fz = sec->floorz, cz = sec->ceilingz;

	int vertstoadd = numvertices;

	done.Zero();
	while (vertstoadd > 0)
	{
		int start = 0;
		while (done[start] && start < numvertices) start++;
		int s = start;
		if (start < numvertices)
		{
			while (!done[start])
			{
				auto wallp = &wall[sec->wallptr + start];
				float X = WallStartX(wallp);
				float Y = WallStartY(wallp);
				if (fabs(X) > 32768. || fabs(Y) > 32768.)
				{
					// If we get here there's some fuckery going around with the coordinates. Let's better abort and wait for things to realign.
					// Do not try alternative methods if this happens.
					return true;
				}
				curPoly->push_back(std::make_pair(X, Y));
				done.Set(start);
				vertstoadd--;
				start = wallp->point2 - sec->wallptr;
			}
			polygon.resize(polygon.size() + 1);
			curPoly = &polygon.back();
			if (start != s) return false; // means the sector is badly defined. RRRA'S E1L3 triggers this.
		}
	}
	// Now make sure that the outer boundary is the first polygon by picking a point that's as much to the outside as possible.
	int outer = 0;
	float minx = FLT_MAX;
	float miny = FLT_MAX;
	for (size_t a = 0; a < polygon.size(); a++)
	{
		for (auto& pt : polygon[a])
		{
			if (pt.first < minx || (pt.first == minx && pt.second < miny))
			{
				minx = pt.first;
				miny = pt.second;
				outer = a;
			}
		}
	}
	if (outer != 0) std::swap(polygon[0], polygon[outer]);
	auto indices = mapbox::earcut(polygon);
	if (indices.size() < 3 * (sec->wallnum - 2))
	{
		// this means that full triangulation failed.
		return false;
	}
	sec->floorz = sec->ceilingz = 0;

	int p = 0;
	for (size_t a = 0; a < polygon.size(); a++)
	{
		for (auto& pt : polygon[a])
		{
			float planez;
			PlanesAtPoint(sec, (pt.first * 16), (pt.second * -16), plane ? &planez : nullptr, !plane ? &planez : nullptr);
			FVector3 point = { pt.first, pt.second, planez };
			points[p++] = point;
		}
	}
	
	auto& entry = data[secnum].planes[plane];
	entry.vertices.Resize(indices.size());
	entry.texcoords.Resize(indices.size());
	entry.normal = CalcNormal(sec, plane);

	auto texture = tileGetTexture(plane ? sec->ceilingpicnum : sec->floorpicnum);

	UVCalculator uvcalc(sec, plane, texture, offset);
	
	for(unsigned i = 0; i < entry.vertices.Size(); i++)
	{
		auto& pt = points[indices[i]];
		entry.vertices[i] = pt;
		entry.texcoords[i] = uvcalc.GetUV(int(pt.X * 16), int(pt.Y * -16), pt.Z);
	}

	sec->floorz = fz;
	sec->ceilingz = cz;
	return true;
}

//==========================================================================
//
// Use ZDoom's node builder if the simple approach fails.
// This will create something usable in the vast majority of cases, 
// even if the result is less efficient.
//
//==========================================================================

bool SectorGeometry::MakeVertices2(unsigned int secnum, int plane, const FVector2& offset)
{

	// Convert our sector into something the node builder understands
	auto sect = &sector[secnum];
	TArray<vertex_t> vertexes(sect->wallnum, true);
	TArray<line_t> lines(sect->wallnum, true);
	TArray<side_t> sides(sect->wallnum, true);
	for (int i = 0; i < sect->wallnum; i++)
	{
		auto wal = &wall[sect->wallptr + i];
		vertexes[i].p = { wal->x * (1 / 16.), wal->y * (1 / -16.) };
		lines[i].backsector = nullptr;
		lines[i].frontsector = sect;
		lines[i].linenum = i;
		lines[i].sidedef[0] = &sides[i];
		lines[i].sidedef[1] = nullptr;
		lines[i].v1 = &vertexes[i];
		lines[i].v2 = &vertexes[wal->point2 - sect->wallptr];

		sides[i].sidenum = i;
		sides[i].sector = sect;
	}


	FNodeBuilder::FLevel leveldata =
	{
		&vertexes[0], (int)vertexes.Size(),
		&sides[0], (int)sides.Size(),
		&lines[0], (int)lines.Size(),
		0, 0, 0, 0
	};
	leveldata.FindMapBounds();
	FNodeBuilder builder(leveldata);

	FLevelLocals Level;
	builder.Extract(Level);

	// Now turn the generated subsectors into triangle meshes

	auto& entry = data[secnum].planes[plane];
	entry.vertices.Clear();
	entry.texcoords.Clear();

	int fz = sect->floorz, cz = sect->ceilingz;
	sect->floorz = sect->ceilingz = 0;

	for (auto& sub : Level.subsectors)
	{
		auto v0 = sub.firstline->v1;
		for (unsigned i = 1; i < sub.numlines-1; i++)
		{
			auto v1 = sub.firstline[i].v1;
			auto v2 = sub.firstline[i].v2;

			entry.vertices.Push({ (float)v0->fX(), (float)v0->fY(), 0 });
			entry.vertices.Push({ (float)v1->fX(), (float)v1->fY(), 0 });
			entry.vertices.Push({ (float)v2->fX(), (float)v2->fY(), 0 });

		}
	}

	// calculate the rest.
	auto texture = tileGetTexture(plane ? sect->ceilingpicnum : sect->floorpicnum);

	UVCalculator uvcalc(sect, plane, texture, offset);

	entry.texcoords.Resize(entry.vertices.Size());
	for (unsigned i = 0; i < entry.vertices.Size(); i++)
	{
		auto& pt = entry.vertices[i];

		float planez;
		PlanesAtPoint(sect, (pt.X * 16), (pt.Y * -16), plane ? &planez : nullptr, !plane ? &planez : nullptr);
		entry.vertices[i].Z = planez;
		entry.texcoords[i] = uvcalc.GetUV(int(pt.X * 16.), int(pt.Y * -16.), pt.Z);
	}
	entry.normal = CalcNormal(sect, plane);
	sect->floorz = fz;
	sect->ceilingz = cz;
	return true;
}

//==========================================================================
//
//
//
//==========================================================================

void SectorGeometry::ValidateSector(unsigned int secnum, int plane, const FVector2& offset)
{
	auto sec = &sector[secnum];
	auto compare = &data[secnum].compare[plane];
	if (plane == 0)
	{
		if (sec->floorheinum == compare->floorheinum &&
			sec->floorpicnum == compare->floorpicnum &&
			((sec->floorstat ^ compare->floorstat) & (CSTAT_SECTOR_ALIGN | CSTAT_SECTOR_YFLIP | CSTAT_SECTOR_XFLIP | CSTAT_SECTOR_TEXHALF | CSTAT_SECTOR_SWAPXY)) == 0 &&
			sec->floorxpan_ == compare->floorxpan_ &&
			sec->floorypan_ == compare->floorypan_ &&
			wall[sec->wallptr].pos == data[secnum].poscompare[0] &&
			wall[wall[sec->wallptr].point2].pos == data[secnum].poscompare2[0] &&
			!(sec->dirty & 1) && data[secnum].planes[plane].vertices.Size() ) return;

		sec->dirty &= ~1;
	}
	else
	{
		if (sec->ceilingheinum == compare->ceilingheinum &&
			sec->ceilingpicnum == compare->ceilingpicnum &&
			((sec->ceilingstat ^ compare->ceilingstat) & (CSTAT_SECTOR_ALIGN | CSTAT_SECTOR_YFLIP | CSTAT_SECTOR_XFLIP | CSTAT_SECTOR_TEXHALF | CSTAT_SECTOR_SWAPXY)) == 0 &&
			sec->ceilingxpan_ == compare->ceilingxpan_ &&
			sec->ceilingypan_ == compare->ceilingypan_ &&
			wall[sec->wallptr].pos == data[secnum].poscompare[1] &&
			wall[wall[sec->wallptr].point2].pos == data[secnum].poscompare2[1] &&
			!(sec->dirty & 2) && data[secnum].planes[1].vertices.Size()) return;

		sec->dirty &= ~2;
	}
	*compare = *sec;
	data[secnum].poscompare[plane] = wall[sec->wallptr].pos;
	data[secnum].poscompare2[plane] = wall[wall[sec->wallptr].point2].pos;
	if (data[secnum].degenerate || !MakeVertices(secnum, plane, offset))
	{
		data[secnum].degenerate = true;
		//Printf(TEXTCOLOR_YELLOW "Normal triangulation failed for sector %d. Retrying with alternative approach\n", secnum);
		MakeVertices2(secnum, plane, offset);
	}
}
