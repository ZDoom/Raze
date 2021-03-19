#pragma once

#include "tarray.h"
#include "vectors.h"
#include "build.h"

struct SectorGeometryPlane
{
	TArray<FVector3> vertices;
	TArray<FVector2> texcoords;
	FVector3 normal{};
};

struct SectorGeometryData
{
	SectorGeometryPlane planes[2];
	sectortype compare{};
};

class SectorGeometry
{
	TArray<SectorGeometryData> data;

	void ValidateSector(unsigned sectnum, int plane);
	void MakeVertices(unsigned sectnum, int plane);

public:
	SectorGeometryPlane* get(unsigned sectnum, int plane)
	{
		if (sectnum >= data.Size()) return nullptr;
		ValidateSector(sectnum, plane);
		return &data[sectnum].planes[plane];
	}

	void SetSize(unsigned sectcount)
	{
		data.Clear(); // delete old content
		data.Resize(sectcount);
	}
};

extern SectorGeometry sectorGeometry;

		