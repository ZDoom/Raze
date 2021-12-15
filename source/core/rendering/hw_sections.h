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

	vec2_t v1() const { return ::wall[startpoint].pos; }
	vec2_t v2() const { return ::wall[endpoint].pos; }
	walltype* wallp() const { return &::wall[wall]; }
	SectionLine* partnerLine() const;

};
extern TArray<SectionLine> sectionLines;

inline SectionLine* SectionLine::partnerLine() const
{
	return partner == -1 ? nullptr : &sectionLines[partner];
}

struct Section
{
	int sector;
	// this is the whole point of sections - instead of just having a start index and count, we have an explicit list of lines that's a lot easier to change when needed.
	TArray<int> lines;	
};

extern TArray<Section> Sections;
extern TArray<TArray<int>> sectionspersector;	// reverse map, mainly for the automap


void hw_BuildSections();
void hw_SetSplitSector(int sector, int startpos, int endpos);
void hw_ClearSplitSector();
