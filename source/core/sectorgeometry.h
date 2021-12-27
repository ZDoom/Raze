#pragma once

#include "tarray.h"
#include "vectors.h"
#include "build.h"

struct Section;

struct SectorGeometryPlane
{
	TArray<FVector3> vertices;
	TArray<FVector2> texcoords;
	FVector3 normal{};
};

enum GeomFlags
{
	NoEarcut = 1,
	NoLibtess = 2,
	FloorDone = 4,
	CeilingDone = 8,
};

using SectionGeometryPlane = SectorGeometryPlane;

struct sectortypelight
{
	float ceilingxpan_;
	float ceilingypan_;
	float floorxpan_;
	float floorypan_;

	ESectorFlags ceilingstat;
	ESectorFlags floorstat;
	int16_t ceilingpicnum;
	int16_t ceilingheinum;
	int16_t floorpicnum;
	int16_t floorheinum;

	void copy(sectortype* sec)
	{
		ceilingxpan_ = sec->ceilingxpan_;
		ceilingypan_ = sec->ceilingypan_;
		floorxpan_ = sec->floorxpan_;
		floorypan_ = sec->floorypan_;
		ceilingstat = sec->ceilingstat;
		floorstat = sec->floorstat;
		ceilingheinum = sec->ceilingheinum;
		floorheinum = sec->floorheinum;
		ceilingpicnum = sec->ceilingpicnum;
		floorpicnum = sec->floorpicnum;
	}
};

struct SectionGeometryData
{
	SectorGeometryPlane planes[2];
	TArray<FVector2> meshVertices;	// flat vertices. Stored separately so that plane changes won't require completely new triangulation.
	TArray<int> meshIndices;
	sectortypelight compare[2] = {};
	vec2_t poscompare[2] = {};
	vec2_t poscompare2[2] = {};
	FVector3 normal[2];
};

class SectionGeometry
{
	TArray<SectionGeometryData> data;

	bool ValidateSection(Section* section, int plane);
	void MarkDirty(sectortype* sector);
	bool CreateMesh(Section* section);
	void CreatePlaneMesh(Section* section, int plane, const FVector2& offset);


public:
	SectionGeometryPlane* get(Section* section, int plane, const FVector2& offset, TArray<int>** pIndices);

	void SetSize(unsigned sectcount)
	{
		data.Clear(); // delete old content
		data.Resize(sectcount);
	}
};

extern SectionGeometry sectionGeometry;

				