#pragma once

#include "build.h"

struct SectionLine
{
	int16_t section;
	int16_t partnersection;
	int16_t startpoint;
	int16_t endpoint;
	int16_t wall;
	int16_t partner;
	int16_t point2index;
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
