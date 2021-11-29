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


SectionLine sectionLines[MAXWALLS + (MAXWALLS >> 2)];
Section sections[MAXSECTORS + (MAXSECTORS >> 2)];
TArray<int> sectionspersector[MAXSECTORS];	// reverse map, mainly for the automap
int numsections;
int numsectionlines;

void hw_SplitSector(int sector, int startpos, int endpos);

TArray<int> splits;


void hw_BuildSections()
{
	for (int i = 0; i < numsectors; i++)
	{
		// Fix maps which do not set their wallptr to the first wall. Lo Wang In Time's map 11 is such a case.
		int wp = sector[i].wallptr;
		while  (wp > 0 && wall[wp - 1].nextwall >= 0 && wall[wall[wp - 1].nextwall].nextsector == i)
		{
			sector[i].wallptr--;
			sector[i].wallnum++;
			wp--;
		}
		sections[i].sector = i;
		sections[i].lines.Resize(sector[i].wallnum);
		for (int j = 0; j < sector[i].wallnum; j++) sections[i].lines[j] = sector[i].wallptr + j;
		sectionspersector[i].Resize(1);
		sectionspersector[i][0] = i;
	}

	// Initial setup just creates a 1:1 mapping of walls to section lines and sectors to sections.
	numsectionlines = numwalls;
	numsections = numsectors;
	for (int i = 0; i < numwalls; i++)
	{
		sectionLines[i].startpoint = sectionLines[i].wall = i;
		sectionLines[i].endpoint = wall[i].point2;
		sectionLines[i].partner = wall[i].nextwall;
		sectionLines[i].section = wall[i].sector;
		sectionLines[i].partnersection = wall[i].nextsector;
		sectionLines[i].point2index = wall[i].point2 - sector[wall[i].sector].wallptr;
	}

	for (unsigned i = 0; i < splits.Size(); i += 3)
		hw_SplitSector(splits[i], splits[i + 1], splits[i + 2]);
}


static void SplitSection(int section, int start, int end)
{
	// note: to do this, the sector's lines must be well ordered and there must only be one outline and no holes.
	// This also can only apply a single split to a given sector.
	int firstsection = numsections++;
	int secondsection = numsections++;

	auto& sect = sections[section];
	Section* sect1 = &sections[firstsection];
	Section* sect2 = &sections[secondsection];
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
			newline.section = int16_t(sect1 - sections);
			if (i != sect1->lines.Size() - 1) newline.point2index = thisline + 1 - firstnewline;
			else newline.point2index = 0;
			assert(newline.point2index >= 0);

			// relink the partner
			auto& partnerline = sectionLines[newline.partner];
			partnerline.partner = thisline;
			partnerline.partnersection = newline.section;
			thisline++;
		}
		else
		{
			splitline1 = thisline++;
			sectionLines[splitline1].wall = -1;
			sectionLines[splitline1].section = int16_t(sect1 - sections);
			sectionLines[splitline1].partnersection = int16_t(sect2 - sections);
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
			newline.section = int16_t(sect2 - sections);
			if (i != sect2->lines.Size() - 1) newline.point2index = thisline + 1 - firstnewline;
			else newline.point2index = 0;
			assert(newline.point2index >= 0);

			// relink the partner
			auto& partnerline = sectionLines[newline.partner];
			partnerline.partner = thisline;
			partnerline.partnersection = newline.section;
			thisline++;
		}
		else
		{
			splitline2 = thisline++;
			sectionLines[splitline2].wall = -1;
			sectionLines[splitline2].section = int16_t(sect2 - sections);
			sectionLines[splitline2].partnersection = int16_t(sect1 - sections);
			sectionLines[splitline2].startpoint = end;
			sectionLines[splitline2].endpoint = start;
			sectionLines[splitline2].point2index = splitline2 + 1 - firstnewline;
		}
	}
	sectionLines[splitline1].partner = splitline2;
	sectionLines[splitline2].partner = splitline1;

	sectionspersector[sect.sector].Resize(2);
	sectionspersector[sect.sector][0] = int16_t(sect1 - sections);
	sectionspersector[sect.sector][1] = int16_t(sect2 - sections);
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
		for (int aline : sections[sect].lines)
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
