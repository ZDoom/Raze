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


void hw_BuildSections()
{
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

	for (int i = 0; i < numsectors; i++)
	{
		sections[i].sector = i;
		sections[i].lines.Resize(sector[i].wallnum);
		for (int j = 0; j < sector[i].wallnum; j++) sections[i].lines[j] = sector[i].wallptr + j;
		sectionspersector[i].Resize(1);
		sectionspersector[i][0] = i;
	}
}