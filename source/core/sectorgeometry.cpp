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
#include "hw_sections.h"
#include "tesselator.h"

SectionGeometry sectionGeometry;

//==========================================================================
//
//
//
//==========================================================================

static FVector3 CalcNormal(sectortype* sector, int plane)
{
	FVector3 pt[3];

	if (plane == 0 && !(sector->floorstat & CSTAT_SECTOR_SLOPE)) return { 0.f, 1.f, 0.f };
	if (plane == 1 && !(sector->ceilingstat & CSTAT_SECTOR_SLOPE)) return { 0.f, -1.f, 0.f };


	auto wal = sector->firstWall();
	auto wal2 = wal->point2Wall();

	pt[0] = { (float)WallStartX(wal), 0.f, (float)WallStartY(wal)};
	pt[1] = { (float)WallStartX(wal2), 0.f, (float)WallStartY(wal2)};
	PlanesAtPoint(sector, wal->pos.X, wal->pos.Y, plane ? &pt[0].Z : nullptr, plane? nullptr : &pt[0].Y);
	PlanesAtPoint(sector, wal2->pos.X, wal2->pos.Y, plane ? &pt[1].Z : nullptr, plane ? nullptr : &pt[1].Y);

	if (pt[0].X == pt[1].X)
	{
		if (pt[0].Z == pt[1].Z) return { 0.f, plane ? -1.f : 1.f, 0.f };
		pt[2].X = pt[0].X + 4;
		pt[2].Z = pt[0].Z;
	}
	else
	{
		pt[2].X = pt[0].X;
		pt[2].Z = pt[0].Z + 4;
	}
	PlanesAtPoint(sector, pt[2].X, -pt[2].Z, plane ? &pt[2].Y : nullptr, plane ? nullptr : &pt[2].Y);

	auto normal = ((pt[2] - pt[0]) ^ (pt[1] - pt[0])).Unit();
	if ((normal.Y < 0 && !plane) || (normal.Y > 0 && plane)) return -normal;
	return normal;
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
	float ix1;
	float iy1;
	float ix2;
	float iy2;
	float sinalign, cosalign;
	FGameTexture* tex;
	float xpanning, ypanning;
	float xscaled, yscaled;
	FVector2 offset;

public:

	UVCalculator(sectortype* sec, int plane, FGameTexture* tx, const FVector2& off)
	{
		float xpan, ypan;

		sect = sec;
		tex = tx;
		myplane = plane;
		offset = off;

		auto firstwall = sec->firstWall();
		ix1 = firstwall->pos.X;
		iy1 = firstwall->pos.Y;
		ix2 = firstwall->point2Wall()->pos.X;
		iy2 = firstwall->point2Wall()->pos.Y;

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

		DVector2 dv = { (ix2 - ix1), -(iy2 - iy1) };
		auto vang = dv.Angle() - 90.;

		cosalign = float(vang.Cos());
		sinalign = float(vang.Sin());

		int pow2width = 1 << sizeToBits((int)tx->GetDisplayWidth());
		int pow2height = 1 << sizeToBits((int)tx->GetDisplayHeight());

		xpanning = xpan / 256.f;
		ypanning = ypan / 256.f;

		float scalefactor = (stat & CSTAT_SECTOR_TEXHALF) ? 0.5f : 1.f;

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

		xscaled = scalefactor * pow2width;
		yscaled = scalefactor * pow2height;
	}

	FVector2 GetUV(float x, float y, float z)
	{
		float tv, tu;

		if (stat & CSTAT_SECTOR_ALIGN)
		{
			float dx = (x - ix1);
			float dy = (y - iy1);

			tu = -(dx * sinalign + dy * cosalign);
			tv = (dx * cosalign - dy * sinalign);

			if (stat & CSTAT_SECTOR_SLOPE)
			{
				float dz = (z - z1);
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


enum class ETriangulateResult
{
	Ok = 0,
	Failed = 1,		// unable to triangulate
	Invalid = 2,	// input data invalid.
};

//==========================================================================
//
// Convert the outline to render coordinates.
//
//==========================================================================

static int OutlineToFloat(Outline& outl, FOutline& polygon)
{
	polygon.resize(outl.Size());
	unsigned count = 0;
	for (unsigned i = 0; i < outl.Size(); i++)
	{
		polygon[i].resize(outl[i].Size());
		count += outl[i].Size();
		for (unsigned j = 0; j < outl[i].Size(); j++)
		{
			float X = RenderX(outl[i][j].X);
			float Y = RenderY(outl[i][j].Y);
			if (fabs(X) > 32768.f || fabs(Y) > 32768.f)
			{
				// If we get here there's some fuckery going around with the coordinates. Let's better abort and wait for things to realign.
				// Do not try alternative methods if this happens.
				return -1;
			}
			polygon[i][j] = { X, Y };
			}
		}
	return count;
}

//==========================================================================
//
// Try to triangulate a given outline with Earcut.
//
//==========================================================================

ETriangulateResult TriangulateOutlineEarcut(const FOutline& polygon, int count, TArray<FVector2>& points, TArray<int>& indicesOut)
{
	// Sections are already validated so we can assume that the data is well defined here.

	auto indices = mapbox::earcut(polygon);
	size_t numtriangles = count + (polygon.size() - 1) * 2 - 2; // accout for the extra connections needed to turn the polygon into s single loop.
	if (indices.size() < numtriangles * 3)
	{
		// this means that full triangulation failed.
		return ETriangulateResult::Failed;
	}
	points.Resize(count);
	int p = 0;
	for (size_t a = 0; a < polygon.size(); a++)
	{
		for (auto& pt : polygon[a])
		{
			points[p++] = { pt.first, pt.second };
		}
	}
	indicesOut.Resize((unsigned)indices.size());
	for (unsigned i = 0; i < indices.size(); i++)
	{
		indicesOut[i] = indices[i];
	}
	return ETriangulateResult::Ok;
}

//==========================================================================
//
// Try to triangulate a given outline with libtess2.
//
//==========================================================================
FMemArena tessArena(100000);

ETriangulateResult TriangulateOutlineLibtess(const FOutline& polygon, int count, TArray<FVector2>& points, TArray<int>& indicesOut)
{
	tessArena.FreeAll();

	auto poolAlloc = [](void* userData, unsigned int size) -> void*
	{
		FMemArena* pool = (FMemArena*)userData;
		return pool->Alloc(size);
	};

	auto poolFree = [](void*, void*)
	{
	};

	TESSalloc ma{};
	ma.memalloc = poolAlloc;
	ma.memfree = poolFree;
	ma.userData = (void*)&tessArena;
	ma.extraVertices = 256; // realloc not provided, allow 256 extra vertices.

	auto tess = tessNewTess(&ma);
	if (!tess)
		return ETriangulateResult::Failed;

	tessSetOption(tess, TESS_CONSTRAINED_DELAUNAY_TRIANGULATION, 0);
	tessSetOption(tess, TESS_REVERSE_CONTOURS, 1);

	// Add contours.
	for (auto& loop : polygon)
		tessAddContour(tess, 2, &loop.data()->first, (int)sizeof(*loop.data()), (int)loop.size());

	int result = tessTesselate(tess, TESS_WINDING_POSITIVE, TESS_POLYGONS, 3, 2, nullptr);
	if (!result)
	{
		tessDeleteTess(tess);
		return ETriangulateResult::Failed;
	}

	const float* verts = tessGetVertices(tess);
	const int* vinds = tessGetVertexIndices(tess);
	const int* elems = tessGetElements(tess);
	const int nverts = tessGetVertexCount(tess);
	const int nelems = tessGetElementCount(tess) * 3;	// an 'element' here is a full triangle, not a single vertex like in OpenGL...

	points.Resize(nverts);
	indicesOut.Resize(nelems);
	for (int i = 0; i < nverts; i++)
	{
		points[i] = { verts[i * 2], verts[i * 2 + 1] };
	}
	for (int i = 0; i < nelems; i++)
	{
		indicesOut[i] = elems[i];
	}
	return ETriangulateResult::Ok;
}

//==========================================================================
//
//
//
//==========================================================================

bool SectionGeometry::ValidateSection(Section* section, int plane)
{
	auto sec = &sector[section->sector];
	auto& sdata = data[section->index];

	auto compare = &sdata.compare[plane];
	if (plane == 0)
	{
		if (sec->floorheinum == compare->floorheinum &&
			sec->floorpicnum == compare->floorpicnum &&
			((sec->floorstat ^ compare->floorstat) & (CSTAT_SECTOR_ALIGN | CSTAT_SECTOR_YFLIP | CSTAT_SECTOR_XFLIP | CSTAT_SECTOR_TEXHALF | CSTAT_SECTOR_SWAPXY)) == 0 &&
			sec->floorxpan_ == compare->floorxpan_ &&
			sec->floorypan_ == compare->floorypan_ &&
			sec->firstWall()->pos == sdata.poscompare[0] &&
			sec->firstWall()->point2Wall()->pos == sdata.poscompare2[0] &&
			!(section->dirty & EDirty::FloorDirty) && sdata.planes[plane].vertices.Size() ) return true;

		section->dirty &= ~EDirty::FloorDirty;
	}
	else
	{
		if (sec->ceilingheinum == compare->ceilingheinum &&
			sec->ceilingpicnum == compare->ceilingpicnum &&
			((sec->ceilingstat ^ compare->ceilingstat) & (CSTAT_SECTOR_ALIGN | CSTAT_SECTOR_YFLIP | CSTAT_SECTOR_XFLIP | CSTAT_SECTOR_TEXHALF | CSTAT_SECTOR_SWAPXY)) == 0 &&
			sec->ceilingxpan_ == compare->ceilingxpan_ &&
			sec->ceilingypan_ == compare->ceilingypan_ &&
			sec->firstWall()->pos == sdata.poscompare[1] &&
			sec->firstWall()->point2Wall()->pos == sdata.poscompare2[1] &&
			!(section->dirty & EDirty::CeilingDirty) && sdata.planes[1].vertices.Size()) return true;

		section->dirty &= ~EDirty::CeilingDirty;
	}
	compare->copy(sec);
	sdata.poscompare[plane] = sec->firstWall()->pos;
	sdata.poscompare2[plane] = sec->firstWall()->point2Wall()->pos;
	return false;
}


//==========================================================================
//
//
//
//==========================================================================

bool SectionGeometry::CreateMesh(Section* section)
{
	auto outline = BuildOutline(section);
	FOutline foutline;
	int count = OutlineToFloat(outline, foutline);
	if (count == -1) return false; // gotta wait...
	TArray<FVector2> meshVertices;
	TArray<int> meshIndices;
	ETriangulateResult result = ETriangulateResult::Failed;

	auto& sdata = data[section->index];

	if (!(section->flags & NoEarcut))
	{
		result = TriangulateOutlineEarcut(foutline, count, sdata.meshVertices, sdata.meshIndices);
	}
	if (result == ETriangulateResult::Failed && !(section->geomflags & NoLibtess))
	{
		section->geomflags |= NoEarcut;
		result = TriangulateOutlineLibtess(foutline, count, sdata.meshVertices, sdata.meshIndices);
	}

	sdata.planes[0].vertices.Clear();
	sdata.planes[1].vertices.Clear();
	section->dirty &= ~EDirty::GeometryDirty;
	section->dirty |= EDirty::FloorDirty | EDirty::CeilingDirty;
	return true;
}

//==========================================================================
//
// assumes that the geometry has already been validated.
//
//==========================================================================

void SectionGeometry::CreatePlaneMesh(Section* section, int plane, const FVector2& offset)
{
	auto sectorp = &sector[section->sector];
	// calculate the rest.
	auto texture = tileGetTexture(plane ? sectorp->ceilingpicnum : sectorp->floorpicnum);
	auto& sdata = data[section->index];
	auto& entry = sdata.planes[plane];
	int fz = sectorp->floorz, cz = sectorp->ceilingz;
	sectorp->setfloorz(0, true);
	sectorp->setceilingz(0, true);

	UVCalculator uvcalc(sectorp, plane, texture, offset);

	entry.vertices.Resize(sdata.meshVertices.Size());
	entry.texcoords.Resize(entry.vertices.Size());

	for (unsigned i = 0; i < entry.vertices.Size(); i++)
	{
		auto& org = sdata.meshVertices[i];
		auto& pt = entry.vertices[i];
		auto& tc = entry.texcoords[i];

		pt.X = org.X; pt.Y = org.Y;
		PlanesAtPoint(sectorp, pt.X, -pt.Y, plane ? &pt.Z : nullptr, !plane ? &pt.Z : nullptr);
		tc = uvcalc.GetUV(pt.X, -pt.Y, pt.Z);
	}
	sectorp->setfloorz(fz, true);
	sectorp->setceilingz(cz, true);
	entry.normal = CalcNormal(sectorp, plane);
}

//==========================================================================
//
//
//
//==========================================================================

void SectionGeometry::MarkDirty(sectortype* sector)
{
	for (auto section : sectionsPerSector[sectnum(sector)])
	{
		sections[section].dirty = sector->dirty;
	}
	sector->dirty = 0;
}

//==========================================================================
//
//
//
//==========================================================================

SectionGeometryPlane* SectionGeometry::get(Section* section, int plane, const FVector2& offset, TArray<int>** pIndices)
{
	if (!section || section->index >= data.Size()) return nullptr;
	auto sectp = &sector[section->sector];
	if (sectp->dirty) MarkDirty(sectp);
	if (section->dirty & EDirty::GeometryDirty)
	{
		bool res = CreateMesh(section);
		if (!res)
		{
			section->dirty &= ~EDirty::GeometryDirty;	// sector is in an invalid state, so pretend the old setup is still valid. Happens in some SW maps.
		}
	}
	if (!ValidateSection(section, plane))
	{
		CreatePlaneMesh(section, plane, offset);
	}
	*pIndices = &data[section->index].meshIndices;
	return &data[section->index].planes[plane];
}
