/*
** hw_sectiona.cpp
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
** where we can shuffle around some content without disturbing the original
** order...
**
*/


#include "hw_sections.h"
#include "sectorgeometry.h"
#include "gamefuncs.h"


FMemArena tempsectionArena(102400);
TArray<SectionLine> sectionLines;
TArray<Section> Sections;
TArray<TArray<int>> sectionspersector;	// reverse map, mainly for the automap
int numsectionlines;

void hw_SplitSector(int sector, int startpos, int endpos);

TArray<int> splits;


void hw_BuildSections()
{
	Sections.Resize(numsectors);
	memset(Sections.Data(), 0, numsectors * sizeof(Section));
	sectionspersector.Resize(numsectors);
	sectionLines.Resize(numwalls * 5 / 4); // cannot reallocate, unfortunately.
	numsectionlines = numwalls;
	for (int i = 0; i < numsectors; i++)
	{
		Sections[i].sector = i;
		auto lines = (int*)tempsectionArena.Alloc(sector[i].wallnum * sizeof(int));
		Sections[i].lines.Set(lines, sector[i].wallnum);
		for (int j = 0; j < sector[i].wallnum; j++) Sections[i].lines[j] = sector[i].wallptr + j;
		sectionspersector[i].Resize(1);
		sectionspersector[i][0] = i;
	}

	// Initial setup just creates a 1:1 mapping of walls to section lines and sectors to sections.
	numsectionlines = numwalls;
	for (int i = 0; i < numwalls; i++)
	{
		auto& wal = wall[i];
		sectionLines[i].startpoint = sectionLines[i].wall = i;
		sectionLines[i].endpoint = wal.point2;
		sectionLines[i].partner = wal.nextwall;
		sectionLines[i].section = wal.sector;
		sectionLines[i].partnersection = wal.nextsector;
		sectionLines[i].point2index = 0;
		if (wal.sector == -1)
			Printf("Warning: Wall %d without a sector!\n", wall.IndexOf(&wal));
		else 
			sectionLines[i].point2index = wal.point2 - wal.sectorp()->wallptr;
	}

	for (unsigned i = 0; i < splits.Size(); i += 3)
		hw_SplitSector(splits[i], splits[i + 1], splits[i + 2]);
}


static void SplitSection(int section, int start, int end)
{
#if 0 // disabled until refactoring. This code is a mess and needs to be redone.

	// note: to do this, the sector's lines must be well ordered and there must only be one outline and no holes.
	// This also can only apply a single split to a given sector.
	int firstsection = Sections.Reserve(2);
	int secondsection = firstsection+1;

	auto& sect = Sections[section];
	Section* sect1 = &Sections[firstsection];
	Section* sect2 = &Sections[secondsection];
	sect1->sector = sect.sector;
	sect2->sector = sect.sector;
	sect1->lines.Clear();
	sect2->lines.Clear();
	for (int aline : sect.lines)
	{
		int line = sectionLines[aline].wall;
		if (line < start || line >= end)
		{
			sect1->lines.Push(aline);
		}
		if (line == start)
		{
			sect1->lines.Push(-1);
			sect2->lines.Push(-1);
		}
		if (line >= start && line < end)
		{
			sect2->lines.Push(aline);
		}
	}

	int firstnewline = numsectionlines;
	int thisline = numsectionlines;
	int splitline1 = 0, splitline2 = 0;
	//numsectionlines += sect1->lines.Size() + 1;
	for (unsigned i = 0; i < sect1->lines.Size(); i++)//  auto& sline : sect1->lines)
	{
		int sline = sect1->lines[i];
		sect1->lines[i] = thisline;
		if (sline != -1)
		{
			SectionLine& newline = sectionLines[thisline];
			newline = sectionLines[sline];
			newline.section = Sections.IndexOf(sect1);
			if (i != sect1->lines.Size() - 1) newline.point2index = thisline + 1 - firstnewline;
			else newline.point2index = 0;
			assert(newline.point2index >= 0);

			// relink the partner
			if (newline.partner >= 0)
			{
				auto& partnerline = sectionLines[newline.partner];
				partnerline.partner = thisline;
				partnerline.partnersection = newline.section;
			}
			thisline++;
		}
		else
		{
			splitline1 = thisline++;
			sectionLines[splitline1].wall = -1;
			sectionLines[splitline1].section = Sections.IndexOf(sect1);
			sectionLines[splitline1].partnersection = Sections.IndexOf(sect2);
			sectionLines[splitline1].startpoint = start;
			sectionLines[splitline1].endpoint = end;
			sectionLines[splitline1].point2index = splitline1 + 1 - firstnewline;
		}
	}

	firstnewline = thisline;
	for (unsigned i = 0; i < sect2->lines.Size(); i++)//  auto& sline : sect1->lines)
	{
		int sline = sect2->lines[i];
		sect2->lines[i] = thisline;
		if (sline != -1)
		{
			SectionLine& newline = sectionLines[thisline];
			newline = sectionLines[sline];
			newline.section = Sections.IndexOf(sect1);
			if (i != sect2->lines.Size() - 1) newline.point2index = thisline + 1 - firstnewline;
			else newline.point2index = 0;
			assert(newline.point2index >= 0);

			// relink the partner
			if (newline.partner >= 0)
			{
				auto& partnerline = sectionLines[newline.partner];
				partnerline.partner = thisline;
				partnerline.partnersection = newline.section;
			}
			thisline++;
		}
		else
		{
			splitline2 = thisline++;
			sectionLines[splitline2].wall = -1;
			sectionLines[splitline2].section = Sections.IndexOf(sect2);
			sectionLines[splitline2].partnersection = Sections.IndexOf(sect1);
			sectionLines[splitline2].startpoint = end;
			sectionLines[splitline2].endpoint = start;
			sectionLines[splitline2].point2index = splitline2 + 1 - firstnewline;
		}
	}
	sectionLines[splitline1].partner = splitline2;
	sectionLines[splitline2].partner = splitline1;

	sectionspersector[sect.sector].Resize(2);
	sectionspersector[sect.sector][0] = Sections.IndexOf(sect1);
	sectionspersector[sect.sector][1] = Sections.IndexOf(sect2);
#endif
}

void hw_SplitSector(int sectnum, int start, int end)
{
	int wallstart = sector[sectnum].wallptr;
	int wallend = wallstart + sector[sectnum].wallnum;
	if (start < wallstart || start >= wallend || end < wallstart || end >= wallend || end < start) return;

	for (unsigned i = 0; i < sectionspersector[sectnum].Size(); i++)
	{
		int sect = sectionspersector[sectnum][i];
		bool foundstart = false, foundend = false;
		for (int aline : Sections[sect].lines)
		{
			int line = sectionLines[aline].wall;
			if (line == start) foundstart = true;
			if (line == end) foundend = true;
		}
		if (foundstart && foundend)
		{
			sectionspersector[sectnum].Delete(i);
			SplitSection(sect, start, end);
			return;
		}
	}
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

// this got dumped here to move it out of the way.
static FVector3 CalcNormal(sectortype* sector, int plane)
{
	return { 0,0,0 };
}

class UVCalculator1
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

	UVCalculator1(sectortype* sec, int plane, FGameTexture* tx, const FVector2& off)
	{
		float xpan, ypan;

		sect = sec;
		tex = tx;
		myplane = plane;
		offset = off;

		auto firstwall = sec->firstWall();
		ix1 = firstwall->x;
		iy1 = firstwall->y;
		ix2 = firstwall->point2Wall()->x;
		iy2 = firstwall->point2Wall()->y;

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

		cosalign = float(vang.Cos());
		sinalign = float(vang.Sin());

		int pow2width = 1 << sizeToBits((int)tx->GetDisplayWidth());
		int pow2height = 1 << sizeToBits((int)tx->GetDisplayHeight());

		xpanning = xpan / 256.f;
		ypanning = ypan / 256.f;

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

		xscaled = scalefactor * pow2width;
		yscaled = scalefactor * pow2height;
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

