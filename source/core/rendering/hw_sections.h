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
	TArray<int16_t> lines;	
};

// giving 25% more may be a bit high as normally this should be small numbers only.
extern SectionLine sectionLines[MAXWALLS + (MAXWALLS >> 2)];
extern Section sections[MAXSECTORS + (MAXSECTORS >> 2)];
extern TArray<int> sectionspersector[MAXSECTORS];	// reverse map, mainly for the automap
extern int numsections;
extern int numsectionlines;


void hw_BuildSections();
void hw_SetSplitSector(int sector, int startpos, int endpos);
void hw_ClearSplitSector();
