/*
** hw_bunchdrawer.cpp
**
**---------------------------------------------------------------------------
** Copyright 2008-2021 Christoph Oelckers
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
*/ 

#include "hw_drawinfo.h"
#include "hw_bunchdrawer.h"
#include "hw_clipper.h"
#include "hw_clock.h"
#include "hw_drawstructs.h"
#include "automap.h"
#include "gamefuncs.h"



//==========================================================================
//
//
//
//==========================================================================

void BunchDrawer::Init(HWDrawInfo *_di, Clipper* c, vec2_t& view)
{
	di = _di;
	clipper = c;
	viewx = view.x * (1/ 16.f);
	viewy = view.y * -(1/ 16.f);
	StartScene();
	clipper->SetViewpoint(view);
	for (int i = 0; i < numwalls; i++)
	{
		// Precalculate the clip angles to avoid doing this repeatedly during level traversal.
		// Reverse the orientation so that startangle and endangle are properly ordered.
		wall[i].clipangle = clipper->PointToAngle(wall[i].pos);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void BunchDrawer::StartScene()
{
	LastBunch = 0;
	StartTime = I_msTime();
	Bunches.Clear();
	CompareData.Clear();
	gotsector.Zero();
}

//==========================================================================
//
//
//
//==========================================================================

void BunchDrawer::StartBunch(int sectnum, int linenum, binangle startan, binangle endan)
{
	FBunch* bunch = &Bunches[LastBunch = Bunches.Reserve(1)];

	bunch->sectnum = sectnum;
	bunch->startline = bunch->endline = linenum;
	bunch->startangle = startan;
	bunch->endangle = endan;
}

//==========================================================================
//
//
//
//==========================================================================

void BunchDrawer::AddLineToBunch(int line, binangle newan)
{
	Bunches[LastBunch].endline++;
	Bunches[LastBunch].endangle = newan;
}

//==========================================================================
//
//
//
//==========================================================================

void BunchDrawer::DeleteBunch(int index)
{
	Bunches[index] = Bunches.Last();
	Bunches.Pop();
}

bool BunchDrawer::CheckClip(walltype* wal)
{
#ifdef _DEBUG
	if (wal - wall == 843 || wal - wall == 847)
	{
		int a = 0;
	}
#endif



	auto pt2 = &wall[wal->point2];
	sectortype* backsector = &sector[wal->nextsector];
	sectortype* frontsector = &sector[wall[wal->nextwall].nextsector];

	// if one plane is sky on both sides, the line must not clip.
	if (frontsector->ceilingstat & backsector->ceilingstat & CSTAT_SECTOR_SKY) return false;
	if (frontsector->floorstat & backsector->floorstat & CSTAT_SECTOR_SKY) return false;

	float bs_floorheight1;
	float bs_floorheight2;
	float bs_ceilingheight1;
	float bs_ceilingheight2;
	float fs_floorheight1;
	float fs_floorheight2;
	float fs_ceilingheight1;
	float fs_ceilingheight2;

	// Mirrors and horizons always block the view
	//if (linedef->special==Line_Mirror || linedef->special==Line_Horizon) return true;

	PlanesAtPoint(frontsector, wal->x, wal->y, &fs_ceilingheight1, &fs_floorheight1);
	PlanesAtPoint(frontsector, pt2->x, pt2->y, &fs_ceilingheight2, &fs_floorheight2);

	PlanesAtPoint(backsector, wal->x, wal->y, &bs_ceilingheight1, &bs_floorheight1);
	PlanesAtPoint(backsector, pt2->x, pt2->y, &bs_ceilingheight2, &bs_floorheight2);

	// now check for closed sectors! No idea if we really need the sky checks. We'll see.
	if (bs_ceilingheight1 <= fs_floorheight1 && bs_ceilingheight2 <= fs_floorheight2)
	{
		// backsector's ceiling is below frontsector's floor.
		return true;
	}

	if (fs_ceilingheight1 <= bs_floorheight1 && fs_ceilingheight2 <= bs_floorheight2) 
	{
		// backsector's floor is above frontsector's ceiling
		return true;
	}

	if (bs_ceilingheight1 <= bs_floorheight1 && bs_ceilingheight2 <= bs_floorheight2) 
	{
		// backsector is closed
		return true;
	}

	return false;
}

//==========================================================================
//
// ClipLine
// Clips the given segment
//
//==========================================================================

int BunchDrawer::ClipLine(int line)
{
	auto wal = &wall[line];

	auto startAngle = wal->clipangle;
	auto endAngle = wall[wal->point2].clipangle;

	// Back side, i.e. backface culling	- read: endAngle >= startAngle!
	if (startAngle.asbam() - endAngle.asbam() < ANGLE_180)
	{
		return CL_Skip;
	}

	if (!clipper->SafeCheckRange(startAngle, endAngle))
	{
		return CL_Skip;
	}

	if (wal->nextwall == -1 || (wal->cstat & CSTAT_WALL_1WAY) || CheckClip(wal))
	{
		// one-sided
		clipper->SafeAddClipRange(startAngle, endAngle);
		return CL_Draw;
	}
	else
	{
		return CL_Draw | CL_Pass;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void BunchDrawer::ProcessBunch(int bnch)
{
	FBunch* bunch = &Bunches[bnch];

	ClipWall.Clock();
	for (int i = bunch->startline; i <= bunch->endline; i++)
	{
		int clipped = ClipLine(i);

		if (clipped & CL_Draw)
		{
			show2dwall.Set(i);

			//if (gl_render_walls)
			{
				SetupWall.Clock();

				HWWall hwwall;
				//Printf("Rendering wall %d\n", i);
				hwwall.Process(di, &wall[i], &sector[bunch->sectnum], wall[i].nextsector<0? nullptr : &sector[wall[i].nextsector]);
				rendered_lines++;

				SetupWall.Unclock();
			}
		}

		if (clipped & CL_Pass)
		{
			ClipWall.Unclock();
			ProcessSector(wall[i].nextsector);
			ClipWall.Clock();
		}
	}
	ClipWall.Unclock();
}

//==========================================================================
//
// 
//
//==========================================================================

int BunchDrawer::WallInFront(int wall1, int wall2)
{
	double x1s = WallStartX(wall1);
	double y1s = WallStartY(wall1);
	double x1e = WallEndX(wall1);
	double y1e = WallEndY(wall1);
	double x2s = WallStartX(wall2);
	double y2s = WallStartY(wall2);
	double x2e = WallEndX(wall2);
	double y2e = WallEndY(wall2);

	double dx = x1e - x1s;
	double dy = y1e - y1s;

	double t1 = PointOnLineSide(x2s, y2s, x1s, y1s, dx, dy);
	double t2 = PointOnLineSide(x2e, y2e, x1s, y1s, dx, dy);
	if (t1 == 0)
	{
		if (t2 == 0) return(-1);
		t1 = t2;
	}
	if (t2 == 0) t2 = t1;

	if ((t1 * t2) >= 0)
	{
		t2 = PointOnLineSide(viewx, viewy, x1s, y1s, dx, dy);
		return((t2 * t1) < 0);
	}

	dx = x2e - x2s;
	dy = y2e - y2s;
	t1 = PointOnLineSide(x1s, y1s, x2s, y2s, dx, dy);
	t2 = PointOnLineSide(x1e, y1e, x2s, y2s, dx, dy);
	if (t1 == 0)
	{
		if (t2 == 0) return(-1);
		t1 = t2;
	}
	if (t2 == 0) t2 = t1;
	if ((t1 * t2) >= 0)
	{
		t2 = PointOnLineSide(viewx, viewy, x2s, y2s, dx, dy);
		return((t2 * t1) >= 0);
	}
	return(-2);
}

//==========================================================================
//
// This is a bit more complicated than it looks because angles can wrap
// around so we can only compare angle differences.
//
// Rules:
// 1. Any bunch can span at most 180°.
// 2. 2 bunches can never overlap at both ends
// 3. if there is an overlap one of the 2 starting points must be in the
//    overlapping area.
//
//==========================================================================

int BunchDrawer::BunchInFront(FBunch* b1, FBunch* b2)
{
	binangle anglecheck, endang;

	if (b2->startangle.asbam() - b1->startangle.asbam() < b1->endangle.asbam() - b1->startangle.asbam())
	{
		// we have an overlap at b2->startangle
		anglecheck = b2->startangle - b1->startangle;

		// Find the wall in b1 that overlaps b2->startangle
		for (int i = b1->startline; i <= b1->endline; i++)
		{
			endang = wall[wall[i].point2].clipangle - b1->startangle;
			if (endang.asbam() > anglecheck.asbam())
			{
				// found a line
				int ret = WallInFront(b2->startline, i);
				return ret;
			}
		}
	}
	else if (b1->startangle.asbam() - b2->startangle.asbam() < b2->endangle.asbam() - b2->startangle.asbam())
	{
		// we have an overlap at b1->startangle
		anglecheck = b1->startangle - b2->startangle;

		// Find the wall in b2 that overlaps b1->startangle
		for (int i = b2->startline; i <= b2->endline; i++)
		{
			endang = wall[wall[i].point2].clipangle - b2->startangle;
			if (endang.asbam() > anglecheck.asbam())
			{
				// found a line
				int ret = WallInFront(i, b1->startline);
				return ret;
			}
		}
	}
	// we have no overlap
	return -1;
}

//==========================================================================
//
//
//
//==========================================================================

int BunchDrawer::FindClosestBunch()
{
	int closest = 0;              //Almost works, but not quite :(

	CompareData.Clear();
	for (unsigned i = 1; i < Bunches.Size(); i++)
	{
		switch (BunchInFront(&Bunches[i], &Bunches[closest]))
		{
		case 0:		// i is in front
			closest = i;
			continue;

		case 1:	// i is behind
			continue;

		default:		// can't determine
			CompareData.Push(i);	// mark for later comparison
			continue;
		}
	}

	// we need to do a second pass to see how the marked bunches relate to the currently closest one.
	for (unsigned i = 0; i < CompareData.Size(); i++)
	{
		switch (BunchInFront(&Bunches[CompareData[i]], &Bunches[closest]))
		{
		case 0:		// is in front
			closest = CompareData[i];
			CompareData[i] = CompareData.Last();
			CompareData.Pop();
			i = 0;	// we need to recheck everything that's still marked.
			continue;

		case 1:	// is behind
			CompareData[i] = CompareData.Last();
			CompareData.Pop();
			i--;
			continue;

		default:
			continue;

		}
	}
	return closest;
}

//==========================================================================
//
//
//
//==========================================================================

void BunchDrawer::ProcessSector(int sectnum)
{
	if (gotsector[sectnum]) return;
	gotsector.Set(sectnum);

	Bsp.Clock();

	auto sect = &sector[sectnum];
	bool inbunch;
	binangle startangle;

	SetupFlat.Clock();
	HWFlat flat;
	flat.ProcessSector(di, &sector[sectnum]);
	SetupFlat.Unclock();

	//Todo: process subsectors
	inbunch = false;
	for (int i = 0; i < sect->wallnum; i++)
	{
		auto thiswall = &wall[sect->wallptr + i];

#ifdef _DEBUG
		// For displaying positions in debugger
		DVector2 start = { WallStartX(thiswall), WallStartY(thiswall) };
		DVector2 end = { WallStartX(thiswall->point2), WallStartY(thiswall->point2) };
#endif
		binangle ang1 = thiswall->clipangle;
		binangle ang2 = wall[thiswall->point2].clipangle;

		if (ang1.asbam() - ang2.asbam() < ANGLE_180)
		{
			// Backside
			inbunch = false;
		}
		else if (!clipper->SafeCheckRange(ang1, ang2))
		{
			// is it visible?
			inbunch = false;
		}
		else if (!inbunch || ang2.asbam() - startangle.asbam() >= ANGLE_180)
		{
			// don't let a bunch span more than 180° to avoid problems.
			// This limitation ensures that the combined range of 2
			// bunches will always be less than 360° which simplifies
			// the distance comparison code because it prevents a 
			// situation where 2 bunches may overlap at both ends.

			startangle = ang1;
			StartBunch(sectnum, sect->wallptr + i, ang1, ang2);
			inbunch = true;
		}
		else
		{
			AddLineToBunch(sect->wallptr + i, ang2);
		}
		if (thiswall->point2 != sect->wallptr + i + 1) inbunch = false;
	}
	Bsp.Unclock();
}

//==========================================================================
//
//
//
//==========================================================================

void BunchDrawer::RenderScene(int viewsector)
{
	ProcessSector(viewsector);
	while (Bunches.Size() > 0)
	{
		int closest = FindClosestBunch();
		ProcessBunch(closest);
		DeleteBunch(closest);
	}
}
