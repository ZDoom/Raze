#pragma once

#include "tarray.h"
#include "basics.h"

struct HWDrawInfo;
class Clipper;

struct FBunch
{
	int sectornum;
	int startline;
	int endline;
	bool portal;
	angle_t startangle;
	angle_t endangle;
};

class BunchDrawer
{
	HWDrawInfo *di;
	Clipper *clipper;
	int LastBunch;
	int StartTime;
	TArray<FBunch> Bunches;
	TArray<int> CompareData;
	double viewx, viewy;
	float gcosang, gsinang;
	BitArray gotsector;
	BitArray gotsection2;
	BitArray gotwall;
	BitArray blockwall;
	angle_t ang1, ang2, angrange;
	float viewz;

	TArray<int> sectionstartang, sectionendang;

private:

	enum
	{
		CL_Skip = 0,
		CL_Draw = 1,
		CL_Pass = 2,
	};

	angle_t ClipAngle(int wal) { return wall[wal].clipangle - ang1; }
	void StartScene();
	bool StartBunch(int sectnum, int linenum, angle_t startan, angle_t endan, bool portal);
	bool AddLineToBunch(int line, angle_t newan);
	void DeleteBunch(int index);
	bool CheckClip(walltype* wal, float* topclip, float* bottomclip);
	int ClipLine(int line, bool portal);
	void ProcessBunch(int bnch);
	int WallInFront(int wall1, int wall2);
	int ColinearBunchInFront(FBunch* b1, FBunch* b2);
	int BunchInFront(FBunch* b1, FBunch* b2);
	int FindClosestBunch();
	void ProcessSection(int sectnum, bool portal);

public:
	void Init(HWDrawInfo* _di, Clipper* c, vec2_t& view, angle_t a1, angle_t a2);
	void RenderScene(const int* viewsectors, unsigned sectcount, bool portal);
	const BitArray& GotSector() const { return gotsector; }
};
