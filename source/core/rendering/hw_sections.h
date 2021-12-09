#pragma once

#include "build.h"

struct SectionLine
{
	int section;
	int partnersection;
	int startpoint;
	int endpoint;
	int wall;
	int partner;
	int point2index;
};

struct Section
{
	int sector;
	// this is the whole point of sections - instead of just having a start index and count, we have an explicit list of lines that's a lot easier to change when needed.
	TArray<int> lines;	
};

extern TArray<SectionLine> sectionLines;
extern TArray<Section> Sections;
extern TArray<TArray<int>> sectionspersector;	// reverse map, mainly for the automap


void hw_BuildSections();
void hw_SetSplitSector(int sector, int startpos, int endpos);
void hw_ClearSplitSector();
