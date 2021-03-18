#pragma once

#include "tarray.h"
#include "basics.h"

struct HWDrawInfo;
class Clipper;

struct FBunch
{
    int sectnum;
    int startline;
    int endline;
    angle_t startangle; // in pseudo angles for the clipper
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
    FixedBitArray<MAXSECTORS> gotsector;

private:

    enum
    {
        CL_Skip = 0,
        CL_Draw = 1,
        CL_Pass = 2,
    };


    void StartScene();
    void StartBunch(int sectnum, int linenum, angle_t startan, angle_t endan);
    void AddLineToBunch(int line, int newan);
    void DeleteBunch(int index);
    bool CheckClip(walltype* wal);
    int ClipLine(int line);
    void ProcessBunch(int bnch);
    int WallInFront(int wall1, int wall2);
    int BunchInFront(FBunch* b1, FBunch* b2);
    int FindClosestBunch();
    void ProcessSector(int sectnum);

public:
    void Init(HWDrawInfo* _di, Clipper* c, vec2_t& view);
    void RenderScene(int viewsector);
};
