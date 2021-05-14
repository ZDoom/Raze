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
    bool portal;
    binangle startangle;
    binangle endangle;
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
    vec2_t iview;
    float gcosang, gsinang;
    FixedBitArray<MAXSECTORS> gotsector;
    FixedBitArray<MAXSECTORS*5/4> gotsection2;
    FixedBitArray<MAXWALLS> gotwall;
    FixedBitArray<MAXWALLS> blockwall;
    binangle ang1, ang2, angrange;

    int sectionstartang[MAXSECTORS*5/4], sectionendang[MAXSECTORS*5/4];

private:

    enum
    {
        CL_Skip = 0,
        CL_Draw = 1,
        CL_Pass = 2,
    };

    binangle ClipAngle(int wal) { return wall[wal].clipangle - ang1; }
    void StartScene();
    bool StartBunch(int sectnum, int linenum, binangle startan, binangle endan, bool portal);
    bool AddLineToBunch(int line, binangle newan);
    void DeleteBunch(int index);
    bool CheckClip(walltype* wal);
    int ClipLine(int line, bool portal);
    void ProcessBunch(int bnch);
    int WallInFront(int wall1, int wall2);
    int BunchInFront(FBunch* b1, FBunch* b2);
    int FindClosestBunch();
    void ProcessSection(int sectnum, bool portal);

public:
    void Init(HWDrawInfo* _di, Clipper* c, vec2_t& view, binangle a1, binangle a2);
    void RenderScene(const int* viewsectors, unsigned sectcount, bool portal);
    const FixedBitArray<MAXSECTORS>& GotSector() const { return gotsector; }
};
