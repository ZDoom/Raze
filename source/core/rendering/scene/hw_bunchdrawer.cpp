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
#include "hw_portal.h"
#include "gamestruct.h"
#include "hw_voxels.h"
#include "mapinfo.h"
#include "gamecontrol.h"
#include "hw_sections.h"
#include "coreactor.h"

//#define DEBUG_CLIPPER
//==========================================================================
//
//
//
//==========================================================================

void BunchDrawer::Init(HWDrawInfo *_di, Clipper* c, vec2_t& view, angle_t a1, angle_t a2)
{
	ang1 = a1;
	ang2 = a2;
	angrange = ang2 - ang1;
	di = _di;
	clipper = c;
	viewx = view.X * (1/ 16.f);
	viewy = view.Y * -(1/ 16.f);
	viewz = (float)di->Viewpoint.Pos.Z;
	iview = view;
	StartScene();

	gcosang = g_cosbam(di->Viewpoint.RotAngle);
	gsinang = g_sinbam(di->Viewpoint.RotAngle);

	for (auto& w : wall)
	{
		// Precalculate the clip angles to avoid doing this repeatedly during level traversal.
		auto vv = w.wall_int_pos() - view;
		w.clipangle = RAD2BAM(atan2(vv.Y, vv.X));
	}
	memset(sectionstartang.Data(), -1, sectionstartang.Size() * sizeof(sectionstartang[0]));
	memset(sectionendang.Data(), -1, sectionendang.Size() * sizeof(sectionendang[0]));
	//blockwall.Resize(wall.Size());
}

//==========================================================================
//
//
//
//==========================================================================

void BunchDrawer::StartScene()
{
	unsigned numsections = sections.Size();
	LastBunch = 0;
	StartTime = I_msTime();
	Bunches.Clear();
	CompareData.Clear();
	gotsector.Resize(sector.Size());
	gotsector.Zero();
	gotsection2.Resize(numsections);
	gotsection2.Zero();
	gotwall.Resize(wall.Size());
	gotwall.Zero();
	sectionstartang.Resize(numsections);
	sectionendang.Resize(numsections);
	//blockwall.Zero();
}

//==========================================================================
//
//
//
//==========================================================================

bool BunchDrawer::StartBunch(int sectnum, int linenum, angle_t startan, angle_t endan, bool portal)
{
	FBunch* bunch = &Bunches[LastBunch = Bunches.Reserve(1)];

	bunch->sectornum = sectnum;
	bunch->startline = bunch->endline = linenum;
	bunch->startangle = startan;
	bunch->endangle = endan;
	bunch->portal = portal;
	assert(bunch->endangle >= bunch->startangle);
	return bunch->endangle != angrange;
}

//==========================================================================
//
//
//
//==========================================================================

bool BunchDrawer::AddLineToBunch(int line, angle_t newan)
{
	Bunches[LastBunch].endline++;
	assert(newan > Bunches[LastBunch].endangle);
	Bunches[LastBunch].endangle = newan;
	assert(Bunches[LastBunch].endangle > Bunches[LastBunch].startangle);
	return Bunches[LastBunch].endangle != angrange;
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

bool BunchDrawer::CheckClip(walltype* wal, float* topclip, float* bottomclip)
{
	auto pt2 = wal->point2Wall();
	sectortype* backsector = wal->nextSector();
	sectortype* frontsector = wal->sectorp();

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

	PlanesAtPoint(frontsector, wal->pos.X, wal->pos.Y, &fs_ceilingheight1, &fs_floorheight1);
	PlanesAtPoint(frontsector, pt2->pos.X, pt2->pos.Y, &fs_ceilingheight2, &fs_floorheight2);

	PlanesAtPoint(backsector, wal->pos.X, wal->pos.Y, &bs_ceilingheight1, &bs_floorheight1);
	PlanesAtPoint(backsector, pt2->pos.X, pt2->pos.Y, &bs_ceilingheight2, &bs_floorheight2);

	*bottomclip = max(min(bs_floorheight1, bs_floorheight2), min(fs_floorheight1, fs_floorheight2));

	// if one plane is sky on both sides, the line must not clip.
	if (frontsector->ceilingstat & backsector->ceilingstat & CSTAT_SECTOR_SKY)
	{
		// save some processing with outside areas - no need to add to the clipper if back sector is higher.
		/*if (fs_ceilingheight1 <= bs_floorheight1 && fs_ceilingheight2 <= bs_floorheight2)*/ *bottomclip = -FLT_MAX;
		*topclip = FLT_MAX;
		return false;
	}
	*topclip = min(max(bs_ceilingheight1, bs_ceilingheight2), max(fs_ceilingheight1, fs_ceilingheight2));

	if (frontsector->floorstat & backsector->floorstat & CSTAT_SECTOR_SKY)
	{
		*bottomclip = -FLT_MAX;
		return false;
	}

	// now check for closed sectors.
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

int BunchDrawer::ClipLine(int aline, bool portal)
{
	auto cline = &sectionLines[aline];
	int section = cline->section;
	int line = cline->wall;

	auto startAngleBam = ClipAngle(cline->startpoint);
	auto endAngleBam = ClipAngle(cline->endpoint);

	// Back side, i.e. backface culling	- read: endAngle <= startAngle!
	if (startAngleBam - endAngleBam < ANGLE_180)
	{
		return CL_Skip;
	}
	//if (line >= 0 && blockwall[line]) return CL_Draw;

	// convert to clipper coordinates and clamp to valid range.
	int startAngle = startAngleBam;
	int endAngle = endAngleBam;
	if (startAngle < 0) startAngle = 0;
	if (endAngle < 0 || endAngle > (int)angrange) endAngle = angrange;

	// since these values are derived from previous calls of this function they cannot be out of range.
	int sectStartAngle = sectionstartang[section];
	auto sectEndAngle = sectionendang[section];

	// check against the maximum possible viewing range of the sector.
	// Todo: check if this is sufficient or if we really have to do a more costly check against the single visible segments.
	// Note: These walls may be excluded from the clipper, but not from being drawn!
	// if sectors got dragged around there may be overlaps which this code does not handle well do it may not run on such sectors.
	bool dontclip = false;
	if (sectStartAngle != -1 && !(sector[sections[section].sector].exflags & SECTOREX_DRAGGED)) 
	{
		if ((sectStartAngle > endAngle || sectEndAngle < startAngle))
		{
			dontclip = true;
		}
		else
		{
			if (sectStartAngle > startAngle) startAngle = sectStartAngle;
			if (sectEndAngle < endAngle) endAngle = sectEndAngle;
			if (endAngle <= startAngle) return CL_Skip; // can this even happen?
		}
	}

	if (!portal && !clipper->IsRangeVisible(startAngle, endAngle))
	{
		return CL_Skip;
	}

	if (line < 0)
		return CL_Pass;

	float topclip = 0, bottomclip = 0;
	if (cline->partner == -1 || (wall[line].cstat & CSTAT_WALL_1WAY) || CheckClip(&wall[line], &topclip, &bottomclip))
	{
		// one-sided
		if (!portal && !dontclip && !(sector[sections[section].sector].exflags & SECTOREX_DONTCLIP))
		{
			clipper->AddClipRange(startAngle, endAngle);
			//Printf("\nWall %d from %2.3f - %2.3f (blocking)\n", line, bamang(startAngle).asdeg(), bamang(endAngle).asdeg());
			//clipper->DumpClipper();
		}
		return CL_Draw;
	}
	else
	{
		if (portal) clipper->RemoveClipRange(startAngle, endAngle);
		else
		{
			if ((topclip < FLT_MAX || bottomclip > -FLT_MAX) && !dontclip)
			{
				clipper->AddWindowRange(startAngle, endAngle, topclip, bottomclip, viewz);
				//Printf("\nWall %d from %2.3f - %2.3f, (%2.3f, %2.3f) (passing)\n", line, bamang(startAngle).asdeg(), bamang(endAngle).asdeg(), topclip, bottomclip);
				//clipper->DumpClipper();
			}
		}

		// set potentially visible viewing range for this line's back sector.
		int nsection = cline->partnersection;
		if (sectionstartang[nsection] == -1)
		{
			sectionstartang[nsection] = startAngle;
			sectionendang[nsection] = endAngle;
		}
		else
		{
			if (startAngle < sectionstartang[nsection]) sectionstartang[nsection] = startAngle;
			if (endAngle > sectionendang[nsection]) sectionendang[nsection] = endAngle;
		}

		return dontclip? CL_Draw : CL_Draw | CL_Pass;
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
	int start = bunch->startline;
	int end = bunch->endline;

	ClipWall.Clock();
	for (int i = start; i <= end; i++)
	{
		bunch = &Bunches[bnch];	// re-get the pointer in case of reallocation.
		int clipped = ClipLine(i, bunch->portal);

		if (clipped & CL_Draw)
		{
			int ww = sectionLines[i].wall;
			if (ww != -1)
			{
				show2dwall.Set(ww);

				if (!gotwall[i])
				{
					gotwall.Set(i);
					ClipWall.Unclock();
					Bsp.Unclock();
					SetupWall.Clock();

					HWWall hwwall;
					hwwall.Process(di, &wall[ww], &sector[bunch->sectornum], wall[ww].nextsector < 0 ? nullptr : &sector[wall[ww].nextsector]);

					SetupWall.Unclock();
					Bsp.Clock();
					ClipWall.Clock();
				}
			}
		}

		if (clipped & CL_Pass)
		{
			ClipWall.Unclock();
			ProcessSection(sectionLines[i].partnersection, false);
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

int BunchDrawer::WallInFront(int line1, int line2)
{
	int wall1s = sectionLines[line1].startpoint;
	int wall1e = sectionLines[line1].endpoint;
	int wall2s = sectionLines[line2].startpoint;
	int wall2e = sectionLines[line2].endpoint;

	double x1s = WallStartX(wall1s);
	double y1s = WallStartY(wall1s);
	double x1e = WallStartX(wall1e);
	double y1e = WallStartY(wall1e);
	double x2s = WallStartX(wall2s);
	double y2s = WallStartY(wall2s);
	double x2e = WallStartX(wall2e);
	double y2e = WallStartY(wall2e);

retry:
	double dx1 = x1e - x1s;
	double dy1 = y1e - y1s;

	double t1 = PointOnLineSide(x2s, y2s, x1s, y1s, dx1, dy1);
	double t2 = PointOnLineSide(x2e, y2e, x1s, y1s, dx1, dy1);
	if (t1 == 0)
	{
		if (t2 == 0) return -1;
		t1 = t2;
	}
	if (t2 == 0) t2 = t1;

	if ((t1 * t2) >= 0)
	{
		t2 = PointOnLineSide(viewx, viewy, x1s, y1s, dx1, dy1);
		return (t2 * t1) <= 0;
	}

	double dx2 = x2e - x2s;
	double dy2 = y2e - y2s;
	double t3 = PointOnLineSide(x1s, y1s, x2s, y2s, dx2, dy2);
	double t4 = PointOnLineSide(x1e, y1e, x2s, y2s, dx2, dy2);
	if (t3 == 0)
	{
		if (t4 == 0) return -1;
		t3 = t4;
	}
	if (t4 == 0) t4 = t3;
	if ((t3 * t4) >= 0)
	{
		t4 = PointOnLineSide(viewx, viewy, x2s, y2s, dx2, dy2);
		return (t4 * t3) > 0;
	}

	// If we got here the walls intersect. Most of the time this is just a tiny sliver intruding into the other wall.
	// If that is the case we can ignore that sliver and pretend it is completely on the other side.

	const double max_dist = 3;
	const double side_threshold = (max_dist * max_dist) / (16. * 16.);	// we are operating in render coordinate space but want 3 map units tolerance.

	double d1 = SquareDistToLine(x2s, y2s, x1s, y1s, x1e, y1e);
	if (d1 < side_threshold) t1 = t2; 
	double d2 = SquareDistToLine(x2e, y2e, x1s, y1s, x1e, y1e);
	if (d2 < side_threshold) t2 = t1;
	if ((fabs(d1) < side_threshold) ^ (fabs(d2) < side_threshold)) // only acceptable if only one end of the wall got adjusted.
	{
		t2 = PointOnLineSide(viewx, viewy, x1s, y1s, dx1, dy1);
		return((t2 * t1) <= 0);
	}

	double d3 = SquareDistToLine(x1s, y1s, x2s, y2s, x2e, y2e);
	if (d3 < side_threshold) t1 = t2;
	double d4 = SquareDistToLine(x1e, y1e, x2s, y2s, x2e, y2e);
	if (d4 < side_threshold) t2 = t1;
	if ((fabs(d3) < side_threshold) ^ (fabs(d4) < side_threshold)) // only acceptable if only one end of the wall got adjusted.
	{
		t2 = PointOnLineSide(viewx, viewy, x2s, y2s, dx2, dy2);
		return((t2 * t1) <= 0);
	}

	// let's try some last ditch effort here: compare the longer sections of the two walls from the intersection point.
	// Only do this if the distance of the smaller one is not too large.

	const double max_overlap = 2 * 2;

	if (max(min(d1, d2), min(d3, d4)) < max_overlap)
	{
		// if one of the walls is too short, let colinearBunchInFront decide. This case normally only happens with doors where this will yield the correct result.
		if ((d1 < max_overlap && d2 < max_overlap) || (d3 < max_overlap && d4 < max_overlap))
			return -1;

		DVector2 intersect;
		SquareDistToWall(x1s, -y1s, &wall[line2], &intersect);
		intersect.Y = -intersect.Y;

		if (d3 < max_overlap)
		{
			x1s = intersect.X;
			y1s = intersect.Y;
		}
		else
		{
			x1e = intersect.X;
			y1e = intersect.Y;
		}

		if (d1 < max_overlap)
		{
			x2s = intersect.X;
			y2s = intersect.Y;
		}
		else
		{
			x2e = intersect.X;
			y2e = intersect.Y;
		}
		goto retry;
	}

	return -2;
}

//==========================================================================
//
// Rules:
// 1. Any bunch can span at most 180°.
// 2. 2 bunches can never overlap at both ends
// 3. if there is an overlap one of the 2 starting points must be in the
//    overlapping area.
//
//==========================================================================

int BunchDrawer::ColinearBunchInFront(FBunch* b1, FBunch* b2)
{
	// Unable to determine the order. The only option left is to see if the sectors within the bunch can be ordered.
	for (int i = b1->startline; i <= b1->endline; i++)
	{
		int wall1s = sectionLines[i].wall;
		if (wall1s == -1) continue;
		int sect1 = wall[wall1s].sector;
		int nsect1 = wall[wall1s].nextsector;
		for (int j = b2->startline; j <= b2->endline; j++)
		{
			int wall2s = sectionLines[j].wall;
			if (wall2s == -1) continue;
			int sect2 = wall[wall2s].sector;
			int nsect2 = wall[wall2s].nextsector;
			if (sect1 == nsect2) return 1; // bunch 2 is in front
			if (sect2 == nsect1) return 0; // bunch 1 is in front
		}
	}
	return -1;
}

int BunchDrawer::BunchInFront(FBunch* b1, FBunch* b2)
{
	angle_t anglecheck, endang;
	bool colinear = false;

	if (b2->startangle >= b1->startangle && b2->startangle < b1->endangle)
	{
		// we have an overlap at b2->startangle
		anglecheck = b2->startangle;

		// Find the wall in b1 that overlaps b2->startangle
		for (int i = b1->startline; i <= b1->endline; i++)
		{
			endang = ClipAngle(sectionLines[i].endpoint);
			if (endang > anglecheck)
			{
				// found a line
				int ret = WallInFront(b2->startline, i);
				if (ret == -1)
				{
					ret = ColinearBunchInFront(b1, b2);
					if (ret == -1)
					{
						colinear = true;
						continue;
					}
				}
				return ret;
			}
		}
	}
	else if (b1->startangle >= b2->startangle && b1->startangle < b2->endangle)
	{
		// we have an overlap at b1->startangle
		anglecheck = b1->startangle;

		// Find the wall in b2 that overlaps b1->startangle
		for (int i = b2->startline; i <= b2->endline; i++)
		{
			endang = ClipAngle(sectionLines[i].endpoint);
			if (endang > anglecheck)
			{
				// found a line
				int ret = WallInFront(i, b1->startline);
				if (ret == -1)
				{
					ret = ColinearBunchInFront(b1, b2);
					if (ret == -1)
					{
						colinear = true;
						continue;
					}
				}
				return ret;
			}
		}
	}
	if (colinear)
	{
		return -2;
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
			i = -1;	// we need to recheck everything that's still marked. -1 because this will get incremented before being used.
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
	/*
	int nsection = sectionLines[Bunches[closest].startline].section;
	Printf("\n=====================================\npicked bunch starting at sector %d, wall %d - Range at (%2.3f - %2.3f)\n",
		sections[nsection].sector, Bunches[closest].startline,
		bamang(sectionstartang[nsection]).asdeg(), bamang(sectionendang[nsection]).asdeg());
	*/
	return closest;
}

//==========================================================================
//
//
//
//==========================================================================

void BunchDrawer::ProcessSection(int sectionnum, bool portal)
{
	if (gotsection2[sectionnum]) return;
	gotsection2.Set(sectionnum);

	bool inbunch;


	int sectnum = sections[sectionnum].sector;
	if (!gotsector[sectnum])
	{
		Bsp.Unclock();
		SetupSprite.Clock();
		gotsector.Set(sectnum);
		CoreSectIterator it(sectnum);
		while (auto actor = it.Next())
		{
			if ((actor->spr.cstat & CSTAT_SPRITE_INVISIBLE) || actor->spr.xrepeat == 0 || actor->spr.yrepeat == 0) // skip invisible sprites
				continue;

			int sx = actor->int_pos().X - iview.X, sy = actor->int_pos().Y - int(iview.Y);

			// this checks if the sprite is it behind the camera, which will not work if the pitch is high enough to necessitate a FOV of more than 180°.
			//if ((actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) || (hw_models && tile2model[actor->spr.picnum].modelid >= 0) || ((sx * gcosang) + (sy * gsinang) > 0)) 
			{
				if ((actor->spr.cstat & (CSTAT_SPRITE_ONE_SIDE | CSTAT_SPRITE_ALIGNMENT_MASK)) != (CSTAT_SPRITE_ONE_SIDE | CSTAT_SPRITE_ALIGNMENT_WALL) ||
					(r_voxels && tiletovox[actor->spr.picnum] >= 0 && voxmodels[tiletovox[actor->spr.picnum]]) ||
					(r_voxels && gi->Voxelize(actor->spr.picnum) > -1) ||
					DMulScale(bcos(actor->int_ang()), -sx, bsin(actor->int_ang()), -sy, 6) > 0)
					if (!renderAddTsprite(di->tsprites, actor))
						break;
			}
		}
		SetupSprite.Unclock();
		Bsp.Clock();
	}

	if (automapping)
		show2dsector.Set(sectnum);

	Bsp.Unclock();
	SetupFlat.Clock();
	HWFlat flat;
	flat.ProcessSector(di, &sector[sectnum], sectionnum);
	SetupFlat.Unclock();
	Bsp.Clock();

	//Todo: process subsectors
	inbunch = false;

	auto section = &sections[sectionnum];
	for (unsigned i = 0; i < section->lines.Size(); i++)
	{
		auto thisline = &sectionLines[section->lines[i]];

		angle_t walang1 = ClipAngle(thisline->startpoint);
		angle_t walang2 = ClipAngle(thisline->endpoint);

		// outside the visible area or seen from the backside.
		if ((walang1 > angrange && walang2 > angrange && walang1 < walang2) ||
			(walang1 - walang2 < ANGLE_180))
		{
			inbunch = false;
		}
		else
		{
			if (walang1 >= angrange) { walang1 = 0; inbunch = false; }
			if (walang2 >= angrange) walang2 = angrange;
			if (section->lines[i] >= (int)wall.Size()) inbunch = false;
			if (!inbunch)
			{
				//Printf("Starting bunch, Sector %d\n\tWall %d\n", section->sector, section->lines[i]);
				inbunch = StartBunch(sectnum, section->lines[i], walang1, walang2, portal);
			}
			else
			{
				//Printf("\tWall %d\n", section->lines[i]);
				inbunch = AddLineToBunch(section->lines[i], walang2);
			}
		}
		if (thisline->endpoint != section->lines[i] + 1) inbunch = false;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void BunchDrawer::RenderScene(const int* viewsectors, unsigned sectcount, bool portal)
{
	//Printf("----------------------------------------- \nstart at sector %d, z = %2.3f\n", viewsectors[0], viewz);
	auto process = [&]()
	{
		clipper->Clear(ang1);

		for (unsigned i = 0; i < sectcount; i++)
		{
			for (auto j : sectionsPerSector[viewsectors[i]])
			{
				sectionstartang[j] = 0;
				sectionendang[j] = int(angrange);
			}
		}
		for (unsigned i = 0; i < sectcount; i++)
		{
			for (auto j : sectionsPerSector[viewsectors[i]])
			{
				ProcessSection(j, portal);
			}
		}
		while (Bunches.Size() > 0)
		{
			int closest = FindClosestBunch();
			ProcessBunch(closest);
			DeleteBunch(closest);
		}
	};

	Bsp.Clock();
	if (ang1 != 0 || ang2 != 0)
	{
		process();
	}
	else
	{
		// with a 360° field of view we need to split the scene into two halves. 
		// The BunchInFront check can fail with angles that may wrap around.
		auto rotang = di->Viewpoint.RotAngle;
		ang1 = rotang - ANGLE_90;
		ang2 = rotang + ANGLE_90 - 1;
		angrange = ang2 - ang1;
		process();
		gotsection2.Zero();
		ang1 = rotang + ANGLE_90;
		ang2 = rotang - ANGLE_90 - 1;
		angrange = ang2 - ang1;
		process();
	}
	Bsp.Unclock();
}
