//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#include "ns.h"
#include "engine.h"
#include "aistuff.h"
#include "exhumed.h"
#include "view.h"
#include "sound.h"
#include "names.h"
#include "sequence.h"
#include "player.h"
#include "mapinfo.h"
#include <string.h>
#include <assert.h>

BEGIN_PS_NS

#define kMaxBobs		200
#define kMaxDrips		50
#define kMaxMoveSects	50
#define kMaxObjects		128
#define kMaxWallFace	4096
#define kMaxSlideData	128
#define kMaxPoints		1024
#define kMaxTraps		40
#define kMaxTrails		20
#define kMaxTrailPoints	100


static short ObjectSeq[] = {
    46, -1, 72, -1
};

static short ObjectStatnum[] = {
    kStatExplodeTrigger, kStatExplodeTarget, 98, kStatDestructibleSprite
};

struct Trail
{
    short field_0;
    short field_2;
    short field_4;
    short pad;
};

struct TrailPoint
{
    int x;
    int y;
};

struct Bob
{
    short nSector;
    char field_2;
    char field_3;
    int z;
};

struct Drip
{
    short nSprite;
    short field_2;
};

// 56 bytes
struct Elev
{
    short field_0;
    short nChannel;
    short nSector;
    int field_6;
    int field_A;
    short nCountZOffsets; // count of items in zOffsets
    short nCurZOffset;
    int zOffsets[8]; // different Z offsets
    short field_32;
    short nSprite;
    short field_36;
};

// 16 bytes
struct MoveSect
{
    short nSector;
    short nTrail;
    short nTrailPoint;
    short field_6;
    short field_8; // nSector?
    int field_10;
    short field_14; // nChannel?
};

struct Object
{
    short field_0;
    short nHealth;
    short field_4;
    short nSprite;
    short field_8;
    short field_10;
    short field_12;
};

struct wallFace
{
    short nChannel;
    short nWall;
    short field_4;
    short field_6[8];
};

// TODO - rename
struct slideData2
{
    short nChannel;
    short field_2;
    short field_4;
    short field_6;
    short field_8;
    uint8_t field_A[6]; // pad?
};

struct slideData
{
    int field_0;
    int field_4;
    int field_8;
    int field_C;
    int x1;
    int y1;
    int x2;
    int y2;
    int field_20;
    int field_24;
    int field_28;
    int field_2C;
    int field_30;
    int field_34;
    int field_38;
    int field_3C;
};

struct Point
{
    short field_0;
    short field_2;
    short field_4;
    short field_6;
    short field_8;
    short field_A;
    short field_C;
    short field_E;
};

struct Trap
{
    short field_0;
    short nSprite;
    short nType;
    short field_6;
    short field_8;
    short field_A;
    short field_C;
    short field_E;
};

Trap sTrap[kMaxTraps];

Bob sBob[kMaxBobs];
Trail sTrail[kMaxTrails];
TrailPoint sTrailPoint[kMaxTrailPoints];
Elev Elevator[kMaxElevs];
Object ObjectList[kMaxObjects];
MoveSect sMoveSect[kMaxMoveSects];
slideData SlideData[kMaxSlideData];
short sMoveDir[kMaxMoveSects];
wallFace WallFace[kMaxWallFace];
slideData2 SlideData2[kMaxSlideData];
Point PointList[kMaxPoints];

short nTrapInterval[kMaxTraps];

short sBobID[kMaxBobs];

short PointCount;
short PointFree[kMaxPoints];

short SlideCount = 0;
short SlideFree[kMaxSlides];

char nTrailPointVal[kMaxTrailPoints];
short nTrailPointPrev[kMaxTrailPoints];
short nTrailPointNext[kMaxTrailPoints];

Drip sDrip[kMaxDrips];

short ElevCount = -1;

short WallFaceCount = -1;

int lFinaleStart;

short nTrailPoints;

short nEnergyBlocks;
short nMoveSects;
short nFinaleStage;
short nTrails;
short nTraps;
short nFinaleSpr;
short ObjectCount;
short nDrips;

short nBobs = 0;
short nDronePitch = 0;
short nSmokeSparks = 0;

// done
void InitObjects()
{
    ObjectCount = 0;
    nTraps = 0;
    nDrips = 0;
    nBobs = 0;
    nTrails = 0;
    nTrailPoints = 0;
    nMoveSects = 0;

    memset(sTrail, -1, sizeof(sTrail));

    nEnergyBlocks = 0;
    nDronePitch = 0;
    nFinaleStage = 0;
    lFinaleStart = 0;
    nSmokeSparks = 0;
}

// done
void InitElev()
{
    ElevCount = kMaxElevs;
}

// done
int BuildWallSprite(int nSector)
{
    int nWall = sector[nSector].wallptr;

    int x = wall[nWall].x;
    int y = wall[nWall].y;

    int x2 = wall[nWall + 1].x;
    int y2 = wall[nWall + 1].y;

    int nSprite = insertsprite(nSector, 401);

    sprite[nSprite].x = (x + x2) / 2;
    sprite[nSprite].y = (y + y2) / 2;
    sprite[nSprite].z = (sector[nSector].floorz + sector[nSector].ceilingz) / 2;
    sprite[nSprite].cstat = 0x8000;

    return nSprite;
}

// done
short FindWallSprites(short nSector)
{
    int var_24 = 0x7FFFFFFF;
    int ecx    = 0x7FFFFFFF;

    int nWall = sector[nSector].wallptr;
    int nWallCount = sector[nSector].wallnum;

    int esi = 0x80000002;
    int edi = 0x80000002;

    int i;

    for (i = 0; i < nWallCount; i++)
    {
        if (wall[nWall].x < var_24) {
            var_24 = wall[nWall].x;
        }

        if (esi < wall[nWall].x) {
            esi = wall[nWall].x;
        }

        if (ecx > wall[nWall].y) {
            ecx = wall[nWall].y;
        }

        if (edi < wall[nWall].y) {
            edi = wall[nWall].y;
        }

        nWall++;
    }

    ecx -= 5;
    esi += 5;
    edi += 5;
    var_24 -= 5;

    int nSprite = -1;

    for (i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].lotag == 0)
        {
            if ((sprite[i].cstat & 0x50) == 80)
            {
                int var_28 = sprite[i].x;
                int ebx = sprite[i].y;

                if ((var_28 >= var_24) && (esi >= var_28) && (ebx >= ecx) && (ebx <= edi))
                {
                    sprite[i].owner = nSprite;
                    nSprite = i;
                }
            }
        }
    }

    if (nSprite < 0)
    {
        nSprite = insertsprite(nSector, 401);

        sprite[nSprite].x = (var_24 + esi) / 2;
        sprite[nSprite].y = (ecx + edi) / 2;
        sprite[nSprite].z = sector[nSector].floorz;
        sprite[nSprite].cstat = 0x8000;
        sprite[nSprite].owner = -1;
        sprite[nSprite].lotag = 0;
        sprite[nSprite].hitag = 0;
    }

    return nSprite;
}

int BuildElevF(int nChannel, int nSector, int nWallSprite, int arg_4, int arg_5, int nCount, ...)
{
    assert(ElevCount > 0);

    if (ElevCount <= 0) {
        return -1;
    }

    ElevCount--;

    Elevator[ElevCount].field_0 = 2;
    Elevator[ElevCount].field_6 = arg_4;
    Elevator[ElevCount].field_32 = -1;
    Elevator[ElevCount].field_A = arg_5;
    Elevator[ElevCount].nChannel = nChannel;
    Elevator[ElevCount].nSector = nSector;
    Elevator[ElevCount].nCountZOffsets = 0;
    Elevator[ElevCount].nCurZOffset = 0;
    Elevator[ElevCount].field_36 = 0;

    if (nWallSprite < 0) {
        nWallSprite = BuildWallSprite(nSector);
    }

    Elevator[ElevCount].nSprite = nWallSprite;

    va_list zlist;
    va_start(zlist, nCount);

    while (1)
    {
        if (Elevator[ElevCount].nCountZOffsets >= nCount) {
            return ElevCount;
        }

        int nVal = Elevator[ElevCount].nCountZOffsets;

        Elevator[ElevCount].nCountZOffsets++;

        Elevator[ElevCount].zOffsets[nVal] = va_arg(zlist, int);
    }
    va_end(zlist);

    return ElevCount;
}

int BuildElevC(int arg1, int nChannel, int nSector, int nWallSprite, int arg5, int arg6, int nCount, ...)
{
    int edi = arg5;

    assert(ElevCount > 0);
    if (ElevCount <= 0) {
        return -1;
    }

    ElevCount--;

    Elevator[ElevCount].field_0 = arg1;

    if (arg1 & 4)
    {
        edi = arg5 / 2;
    }

    Elevator[ElevCount].field_6 = edi;
    Elevator[ElevCount].nCountZOffsets = 0;
    Elevator[ElevCount].field_36 = 0;
    Elevator[ElevCount].nCurZOffset = 0;
    Elevator[ElevCount].field_A = arg6;
    Elevator[ElevCount].field_32 = -1;
    Elevator[ElevCount].nChannel = nChannel;
    Elevator[ElevCount].nSector = nSector;

    if (nWallSprite < 0) {
        nWallSprite = BuildWallSprite(nSector);
    }

    Elevator[ElevCount].nSprite = nWallSprite;

    va_list zlist;
    va_start(zlist, nCount);

    while (1)
    {
        if (Elevator[ElevCount].nCountZOffsets >= nCount) {
            return ElevCount;
        }

        int nVal = Elevator[ElevCount].nCountZOffsets;

        Elevator[ElevCount].nCountZOffsets++;

        Elevator[ElevCount].zOffsets[nVal] = va_arg(zlist, int);
    }
    va_end(zlist);

    return ElevCount;
}

// TODO - tidy me up
// RENAME param A - not always Z
// Confirmed 100% correct with original .exe
int LongSeek(int *pZVal, int a2, int a3, int a4)
{
    int v4; // edx@1
    int v5; // ebx@2

    v4 = a2 - *pZVal;

    if (v4 < 0)
    {
        v5 = -a3;
        if (v5 > v4)
            v4 = v5;
        (*pZVal) += v4;
    }

    if (v4 > 0)
    {
        if (a4 < v4)
            v4 = a4;
        (*pZVal) += v4;
    }

    return v4;
}

// done
int CheckSectorSprites(short nSector, int nVal)
{
    int b = 0;

    if (nVal)
    {
        short nSprite = headspritesect[nSector];

        int nZDiff = sector[nSector].floorz - sector[nSector].ceilingz;

        while (nSprite != -1)
        {
            if ((sprite[nSprite].cstat & 0x101) && (nZDiff < GetSpriteHeight(nSprite)))
            {
                if (nVal != 1) {
                    return 1;
                }

                b = 1;

                runlist_DamageEnemy(nSprite, -1, 5);

                if (sprite[nSprite].statnum == 100 && PlayerList[GetPlayerFromSprite(nSprite)].nHealth <= 0)
                {
                    PlayFXAtXYZ(StaticSound[kSoundJonFDie],
                        sprite[nSprite].x,
                        sprite[nSprite].y,
                        sprite[nSprite].z,
                        sprite[nSprite].sectnum | 0x4000);
                }
            }
            nSprite = nextspritesect[nSprite];
        }
    }
    else
    {
        for (int i = headspritesect[nSector]; i != -1; i = nextspritesect[i])
        {
            if (sprite[i].cstat & 0x101) {
                return 1;
            }
        }
        b = 0;
    }

    return b;
}

// done
void MoveSectorSprites(int nSector, int z)
{
    int nSprite = headspritesect[nSector];

    while (nSprite != -1)
    {
        if (sprite[nSprite].statnum != 200) {
            sprite[nSprite].z += z;
        }

        nSprite = nextspritesect[nSprite];
    }
}

void StartElevSound(short nSprite, int nVal)
{
    short nSound;

    if (nVal & 2) {
        nSound = nElevSound;
    }
    else {
        nSound = nStoneSound;
    }

    D3PlayFX(StaticSound[nSound], nSprite);
}

void FuncElev(int a, int, int nRun)
{
    short nElev = RunData[nRun].nVal;
    assert(nElev >= 0 && nElev < kMaxElevs);

    short nChannel = Elevator[nElev].nChannel;
    short var_18 = Elevator[nElev].field_0;

    assert(nChannel >= 0 && nChannel < kMaxChannels);

    int nMessage = a & 0x7F0000;

    if (nMessage < 0x10000) {
        return;
    }

//	int var_24 = var_18 & 0x10; // floor based?

    switch (nMessage)
    {
        default:
        {
            return;
        }

        case 0x10000:
        {
//			short ax = var_18 & 8;
            short dx = sRunChannels[nChannel].c;

            int edi = 999; // FIXME CHECKME - this isn't default set to anything in the ASM that I can see - if ax is 0 and var_24 is 0, this will never be set to a known value otherwise!

            if (var_18 & 0x8)
            {
                if (dx) {
                    edi = 1;
                }
                else {
                    edi = 0;
                }
            }
            else
            {
                // loc_20D48:
                if (var_18 & 0x10) // was var_24
                {
                    if (Elevator[nElev].field_32 < 0)
                    {
                        Elevator[nElev].field_32 = runlist_AddRunRec(NewRun, RunData[nRun].nMoves);
                        StartElevSound(Elevator[nElev].nSprite, var_18);

                        edi = 1;
                    }
                }
                else
                {
                    if (dx < 0) {
                        edi = 0;
                    }
                    else
                    {
                        if (dx == Elevator[nElev].nCurZOffset || dx >= Elevator[nElev].nCountZOffsets)
                        {
                            Elevator[nElev].field_36 = dx;
                            edi = 1;
                        }
                        else
                        {
                            Elevator[nElev].nCurZOffset = sRunChannels[nChannel].c;
                            edi = 1;
                        }
                    }
                }
            }

            assert(edi != 999);

            // loc_20DF9:
            if (edi)
            {
                if (Elevator[nElev].field_32 < 0)
                {
                    Elevator[nElev].field_32 = runlist_AddRunRec(NewRun, RunData[nRun].nMoves);

                    StartElevSound(Elevator[nElev].nSprite, var_18);
                }
            }
            else
            {
                //loc_20E4E:
                if (Elevator[nElev].field_32 >= 0)
                {
                    runlist_SubRunRec(Elevator[nElev].field_32);
                    Elevator[nElev].field_32 = -1;
                }
            }

            return;
        }

        case 0x20000:
        {
            short nSector = Elevator[nElev].nSector;
            short di = Elevator[nElev].nSprite;

            int ebp = 0; // initialise to *something*

            if (var_18 & 0x2)
            {
                int nZOffset = Elevator[nElev].nCurZOffset;
                int nZVal = Elevator[nElev].zOffsets[nZOffset];

                short nSectorB = nSector;

                int nVal = LongSeek((int*)&sector[nSector].floorz, nZVal, Elevator[nElev].field_6, Elevator[nElev].field_A);
                ebp = nVal;

                if (!nVal)
                {
                    if (var_18 & 0x10)
                    {
                        Elevator[nElev].nCurZOffset ^= 1;
                        StartElevSound(di, var_18);
                    }
                    else
                    {
                        StopSpriteSound(di);
                        runlist_SubRunRec(nRun);
                        Elevator[nElev].field_32 = -1;
                        runlist_ReadyChannel(nChannel);

                        D3PlayFX(StaticSound[nStopSound], Elevator[nElev].nSprite);
                    }
                }
                else
                {
                    assert(nSector == nSectorB);
                    MoveSectorSprites(nSector, nVal);

                    if (nVal < 0 && CheckSectorSprites(nSector, 2))
                    {
                        runlist_ChangeChannel(nChannel, sRunChannels[nChannel].c == 0);
                        return;
                    }
                }
            }
            else
            {
                // loc_20FC3:
                int ceilZ = sector[nSector].ceilingz;
                sectortype *var_28 = &sector[nSector];

                int nZOffset = Elevator[nElev].nCurZOffset;
                int zVal = Elevator[nElev].zOffsets[nZOffset];

                int nVal = LongSeek(&ceilZ, zVal, Elevator[nElev].field_6, Elevator[nElev].field_A);
                ebp = nVal;

                if (!nVal)
                {
                    if (var_18 & 0x10)
                    {
                        Elevator[nElev].nCurZOffset ^= 1;

                        StartElevSound(Elevator[nElev].nSprite, var_18);
                    }
                    else
                    {
                        runlist_SubRunRec(nRun);
                        Elevator[nElev].field_32 = -1;
                        StopSpriteSound(Elevator[nElev].nSprite);
                        D3PlayFX(StaticSound[nStopSound], Elevator[nElev].nSprite);
                        runlist_ReadyChannel(nChannel);
                    }

                    return;
                }
                else if (nVal > 0)
                {
                    if (ceilZ == zVal)
                    {
                        if (var_18 & 0x4) {
                            SetQuake(di, 30);
                        }

                        PlayFXAtXYZ(StaticSound[kSound26], sprite[di].x, sprite[di].y, sprite[di].z, sprite[di].sectnum);
                    }

                    if (var_18 & 0x4)
                    {
                        if (CheckSectorSprites(nSector, 1)) {
                            return;
                        }
                    }
                    else
                    {
                        if (CheckSectorSprites(nSector, 0))
                        {
                            runlist_ChangeChannel(nChannel, sRunChannels[nChannel].c == 0);
                            return;
                        }
                    }
                }

                var_28->ceilingz = ceilZ;
            }

            // maybe this doesn't go here?
            while (di != -1)
            {
                sprite[di].z += ebp;
                di = sprite[di].owner;
            }

            return;
        }
    }
}

// done
void InitWallFace()
{
    WallFaceCount = kMaxWallFace;
}

int BuildWallFace(short nChannel, short nWall, int nCount, ...)
{
    if (WallFaceCount <= 0) {
        I_Error("Too many wall faces!\n");
    }

    WallFaceCount--;

    WallFace[WallFaceCount].field_4 = 0;
    WallFace[WallFaceCount].nWall = nWall;
    WallFace[WallFaceCount].nChannel = nChannel;

    if (nCount > 8) {
        nCount = 8;
    }

    va_list piclist;
    va_start(piclist, nCount);

    while (WallFace[WallFaceCount].field_4 < nCount)
    {
        int i = WallFace[WallFaceCount].field_4;
        WallFace[WallFaceCount].field_4++;

        WallFace[WallFaceCount].field_6[i] = (short)va_arg(piclist, int);
    }
    va_end(piclist);

    return WallFaceCount | 0x70000;
}

void FuncWallFace(int a, int, int nRun)
{
    int nWallFace = RunData[nRun].nVal;
    assert(nWallFace >= 0 && nWallFace < kMaxWallFace);

    short nChannel = WallFace[nWallFace].nChannel;

    if ((a & 0x7F0000) != 0x10000)
        return;

    short si = sRunChannels[nChannel].c;

    if ((si <= WallFace[nWallFace].field_4) && (si >= 0))
    {
        wall[WallFace[nWallFace].nWall].picnum = WallFace[nWallFace].field_6[si];
    }
}

// done
void InitPoint()
{
    PointCount = 0;

    for (int i = 0; i < kMaxPoints; i++) {
        PointFree[i] = i;
    }
}

// done
int GrabPoint()
{
    return PointFree[PointCount++];
}

// done
void InitSlide()
{
    SlideCount = kMaxSlides;

    for (int i = 0; i < kMaxSlides; i++) {
        SlideFree[i] = i;
    }
}

int IdentifySector(int nVal)
{
    for (int i = 0; i < numsectors; i++)
    {
        for (int j = 0; ; j++)
        {
            if (j < sector[i].wallnum)
            {
                int nWall = sector[i].wallptr;
                if (nWall + j == nVal)
                    return i;
            }
            else {
                break;
            }
        }
    }

    return -1;
}

int BuildSlide(int nChannel, int nStartWall, int ebx, int ecx, int nWall2, int nWall3, int nWall4)
{
    if (SlideCount <= 0) {
        I_Error("Too many slides!\n");
        return -1;
    }

    SlideCount--;

    int nSlide = SlideCount;

    short nSector = IdentifySector(nStartWall);

    SlideData2[nSlide].field_4 = -1;
    SlideData2[nSlide].nChannel = nChannel;
    SlideData2[nSlide].field_2 = -1;

    int nPoint = GrabPoint();

    SlideData2[nSlide].field_2 = nPoint;

    PointList[nPoint].field_E = -1;
    PointList[nPoint].field_0 = nSector;

    short startwall = sector[nSector].wallptr;
    short endwall = startwall + sector[nSector].wallnum;

    for (int nWall = startwall; nWall < endwall; nWall++)
    {
        short ax = SlideData2[nSlide].field_2;

        if (ax >= 0)
        {
            while (ax >= 0)
            {
                if (wall[nWall].nextsector == PointList[ax].field_0) {
                    break;
                }

                ax = PointList[ax].field_E;
            }
        }
        else
        {
            if (wall[nWall].nextsector >= 0)
            {
                nPoint = GrabPoint();

                PointList[nPoint].field_E = SlideData2[nSlide].field_2;
                PointList[nPoint].field_0 = wall[nWall].nextsector;

                SlideData2[nSlide].field_2 = nPoint;
            }
        }
    }

    SlideData[nSlide].field_0 = nStartWall;
    SlideData[nSlide].field_4 = ebx;
    SlideData[nSlide].field_8 = nWall2;
    SlideData[nSlide].field_C = nWall3;

    SlideData[nSlide].x1 = wall[nStartWall].x;
    SlideData[nSlide].y1 = wall[nStartWall].y;

    SlideData[nSlide].x2 = wall[nWall2].x;
    SlideData[nSlide].y2 = wall[nWall2].y;

    SlideData[nSlide].field_20 = wall[ebx].x;
    SlideData[nSlide].field_24 = wall[ebx].y;

    SlideData[nSlide].field_28 = wall[nWall3].x;
    SlideData[nSlide].field_2C = wall[nWall3].y;

    SlideData[nSlide].field_30 = wall[ecx].x;
    SlideData[nSlide].field_34 = wall[ecx].y;

    SlideData[nSlide].field_38 = wall[nWall4].x;
    SlideData[nSlide].field_3C = wall[nWall4].y;

    int nSprite = insertsprite(nSector, 899);

    SlideData2[nSlide].field_6 = nSprite;
    sprite[nSprite].cstat = 0x8000;
    sprite[nSprite].x = wall[nStartWall].x;
    sprite[nSprite].y = wall[nStartWall].y;
    sprite[nSprite].z = sector[nSector].floorz;

    SlideData2[nSlide].field_8 = 0;

    return nSlide | 0x80000;
}

void FuncSlide(int a, int, int nRun)
{
    int nSlide = RunData[nRun].nVal;
    assert(nSlide >= 0 && nSlide < kMaxSlides);

    short nChannel = SlideData2[nSlide].nChannel;

    int nMessage = a & 0x7F0000;

    int ebp = 0;

    short cx = sRunChannels[nChannel].c;

    switch (nMessage)
    {
        case 0x10000:
        {
            if (SlideData2[nSlide].field_4 >= 0)
            {
                runlist_SubRunRec(SlideData2[nSlide].field_4);
                SlideData2[nSlide].field_4 = -1;
            }

            if (sRunChannels[nChannel].c && sRunChannels[nChannel].c != 1) {
                return;
            }

            SlideData2[nSlide].field_4 = runlist_AddRunRec(NewRun, RunData[nRun].nMoves);

            if (SlideData2[nSlide].field_8 != sRunChannels[nChannel].c)
            {
                D3PlayFX(StaticSound[kSound23], SlideData2[nSlide].field_6);
                SlideData2[nSlide].field_8 = sRunChannels[nChannel].c;
            }

            return;
        }

        case 0x20000:
        {
            int clipmask = ebp + 1; // RENAME

            if (cx == 1)
            {
                short nWall = SlideData[nSlide].field_4;
                int x = wall[nWall].x;
                int y = wall[nWall].y;

                int nSeekA = LongSeek(&x, SlideData[nSlide].field_30, 20, 20);
                int var_34 = nSeekA;
                int var_20 = nSeekA;

                int nSeekB = LongSeek(&y, SlideData[nSlide].field_34, 20, 20);
                int var_2C = nSeekB;
                int var_24 = nSeekB;

                dragpoint(SlideData[nSlide].field_4, x, y, 0);
                movesprite(SlideData2[nSlide].field_6, var_34 << 14, var_2C << 14, 0, 0, 0, CLIPMASK1);

                if (var_34 == 0)
                {
                    if (var_2C == 0)
                    {
                        ebp = clipmask;
                    }
                }

                nWall = SlideData[nSlide].field_0;

                y = wall[nWall].y + var_24;
                x = wall[nWall].x + var_20;

                dragpoint(SlideData[nSlide].field_0, x, y, 0);

                nWall = SlideData[nSlide].field_C;

                x = wall[nWall].x;
                y = wall[nWall].y;

                int nSeekC = LongSeek(&x, SlideData[nSlide].field_38, 20, 20);
                int var_30 = nSeekC;
                var_20 = nSeekC;

                int nSeekD = LongSeek(&y, SlideData[nSlide].field_3C, 20, 20);
                int edi = nSeekD;
                var_24 = nSeekD;

                dragpoint(SlideData[nSlide].field_C, x, y, 0);

                if (var_30 == 0 && edi == 0) {
                    ebp++;
                }

                nWall = SlideData[nSlide].field_8;

                x = wall[nWall].x + var_20;
                y = wall[nWall].y + var_24;

                dragpoint(SlideData[nSlide].field_8, x, y, 0);
            }
            else if (cx == 0) // right branch
            {
                short nWall = SlideData[nSlide].field_0;
                int x = wall[nWall].x;
                int y = wall[nWall].y;

                int nSeekA = LongSeek(&x, SlideData[nSlide].x1, 20, 20);
                int edi = nSeekA;
                int var_1C = nSeekA;

                int nSeekB = LongSeek(&y, SlideData[nSlide].y1, 20, 20);
                int ecx = nSeekB;
                int var_28 = nSeekB;

                dragpoint(SlideData[nSlide].field_0, x, y, 0);

                if (edi == 0 && ecx == 0) {
                    ebp = clipmask;
                }

                nWall = SlideData[nSlide].field_4;

                y = wall[nWall].y + var_28;
                x = wall[nWall].x + var_1C;

                dragpoint(SlideData[nSlide].field_4, x, y, 0);

                nWall = SlideData[nSlide].field_8;

                x = wall[nWall].x;
                y = wall[nWall].y;

                int nSeekC = LongSeek(&x, SlideData[nSlide].x2, 20, 20);
                edi = nSeekC;
                var_1C = nSeekC;

                int nSeekD = LongSeek(&y, SlideData[nSlide].y2, 20, 20);
                ecx = nSeekD;
                var_28 = nSeekD;

                dragpoint(SlideData[nSlide].field_8, x, y, 0);

                if (edi == 0 && ecx == 0) {
                    ebp++;
                }

                nWall = SlideData[nSlide].field_C;

                y = wall[nWall].y + var_28;
                x = wall[nWall].x + var_1C;

                dragpoint(SlideData[nSlide].field_C, x, y, 0);
            }

            // loc_21A51:
            if (ebp >= 2)
            {
                runlist_SubRunRec(SlideData2[nSlide].field_4);

                SlideData2[nSlide].field_4 = -1;
                D3PlayFX(StaticSound[nStopSound], SlideData2[nSlide].field_6);

                runlist_ReadyChannel(nChannel);
            }

            return;
        }
    }
}

int BuildTrap(int nSprite, int edx, int ebx, int ecx)
{
    int var_14 = edx;
    int var_18 = ebx;
    int var_10 = ecx;

    if (nTraps >= kMaxTraps) {
        I_Error("Too many traps!\n");
    }

    short nTrap = nTraps;
    nTraps++;

    changespritestat(nSprite, 0);

    sprite[nSprite].cstat = 0x8000;
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;
    sprite[nSprite].extra = -1;

    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].hitag = runlist_AddRunRec(NewRun, nTrap | 0x1F0000);
    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nTrap | 0x1F0000);

//	GrabTimeSlot(3);

    sTrap[nTrap].nSprite = nSprite;
    sTrap[nTrap].nType = (var_14 == 0) + 14;
    sTrap[nTrap].field_0 = -1;

    nTrapInterval[nTrap] = 64 - (2 * var_10);
    if (nTrapInterval[nTrap] < 5) {
        nTrapInterval[nTrap] = 5;
    }

    sTrap[nTrap].field_C = 0;
    sTrap[nTrap].field_A = 0;

    if (var_18 == -1) {
        return nTrap | 0x1F0000;
    }

    sTrap[nTrap].field_6 = -1;
    sTrap[nTrap].field_8 = -1;

    short nSector = sprite[nSprite].sectnum;
    short nWall = sector[nSector].wallptr;

    int i = 0;

    while (1)
    {
        if (sector[nSector].wallnum >= i) {
            return nTrap | 0x1F0000;
        }

        if (var_18 == wall[nWall].hitag)
        {
            if (sTrap[nTrap].field_6 != -1)
            {
                sTrap[nTrap].field_8 = nWall;
                sTrap[nTrap].field_C = wall[nWall].picnum;
                return nTrap | 0x1F0000;
            }
            else
            {
                sTrap[nTrap].field_6 = nWall;
                sTrap[nTrap].field_A = wall[nWall].picnum;
            }
        }

        ecx++;
        nWall++;
    }
}

void FuncTrap(int a, int, int nRun)
{
    short nTrap = RunData[nRun].nVal;
    short nSprite = sTrap[nTrap].nSprite;

    int nMessage = a & 0x7F0000;

    switch (nMessage)
    {
        case 0x10000:
        {
            short nChannel = a & 0x3FFF;

            if (sRunChannels[nChannel].c > 0)
            {
                sTrap[nTrap].field_0 = 12;
            }
            else
            {
                sTrap[nTrap].field_0 = -1;
            }

            return;
        }

        case 0x20000:
        {
            if (sTrap[nTrap].field_0 >= 0)
            {
                sTrap[nTrap].field_0--;
                if (sTrap[nTrap].field_0 > 10) {
                    return;
                }

                short nType = sTrap[nTrap].nType;

                if (sTrap[nTrap].field_0 == 0)
                {
                    sTrap[nTrap].field_0 = nTrapInterval[nTrap];

                    if (nType == 14)
                    {
                        short nWall = sTrap[nTrap].field_6;
                        if (nWall > -1)
                        {
                            wall[nWall].picnum = sTrap[nTrap].field_A;
                        }

                        nWall = sTrap[nTrap].field_8;
                        if (nWall > -1)
                        {
                            wall[nWall].picnum = sTrap[nTrap].field_C;
                        }
                    }
                }
                else
                {
                    // loc_21D92:
                    if (sTrap[nTrap].field_0 != 5) {
                        return;
                    }

                    int nBullet = BuildBullet(nSprite, nType, 0, 0, 0, sprite[nSprite].ang, 0, 1);
                    if (nBullet > -1)
                    {
                        short nBulletSprite = nBullet & 0xFFFF; // isolate the sprite index (disregard top 16 bits)
                        assert(nBulletSprite >= 0);

                        if (nType == 15)
                        {
                            sprite[nBulletSprite].ang = (sprite[nBulletSprite].ang - 512) & kAngleMask;
                            D3PlayFX(StaticSound[kSound32], nSprite);
                        }
                        else
                        {
                            sprite[nBulletSprite].clipdist = 50;

                            short nWall = sTrap[nTrap].field_6;
                            if (nWall > -1)
                            {
                                wall[nWall].picnum = sTrap[nTrap].field_A + 1;
                            }

                            nWall = sTrap[nTrap].field_8;
                            if (nWall > -1)
                            {
                                wall[nWall].picnum = sTrap[nTrap].field_C + 1;
                            }

                            D3PlayFX(StaticSound[kSound36], nSprite);
                        }
                    }
                }
            }

            return;
        }

        case 0x30000:
        case 0x90000:
        case 0x80000:
        case 0xA0000:
            return;

        default:
            DebugOut("unknown msg %d for trap\n", a & 0x7F0000);
            return;
    }
}

int BuildArrow(int nSprite, int nVal)
{
    return BuildTrap(nSprite, 0, -1, nVal);
}

int BuildFireBall(int nSprite, int a, int b)
{
    return BuildTrap(nSprite, 1, a, b);
}

int BuildSpark(int nSprite, int nVal)
{
    int var_14 = insertsprite(sprite[nSprite].sectnum, 0);

    if (var_14 < 0) {
        return -1;
    }

    assert(var_14 < kMaxSprites);

    sprite[var_14].x = sprite[nSprite].x;
    sprite[var_14].y = sprite[nSprite].y;
    sprite[var_14].cstat = 0;
    sprite[var_14].shade = -127;
    sprite[var_14].pal = 1;
    sprite[var_14].xoffset = 0;
    sprite[var_14].yoffset = 0;
    sprite[var_14].xrepeat = 50;
    sprite[var_14].yrepeat = 50;

    if (nVal >= 2)
    {
        sprite[var_14].picnum = kEnergy2;
        nSmokeSparks++;

        if (nVal == 3)
        {
            sprite[var_14].xrepeat = 120;
            sprite[var_14].yrepeat = 120;
        }
        else
        {
            sprite[var_14].xrepeat = sprite[nSprite].xrepeat + 15;
            sprite[var_14].yrepeat = sprite[nSprite].xrepeat + 15;
        }
    }
    else
    {
        int nAngle = (sprite[nSprite].ang + 256) - RandomSize(9);

        if (nVal)
        {
            sprite[var_14].xvel = Cos(nAngle) >> 5;
            sprite[var_14].yvel = Sin(nAngle) >> 5;
        }
        else
        {
            sprite[var_14].xvel = Cos(nAngle) >> 6;
            sprite[var_14].yvel = Sin(nAngle) >> 6;
        }

        sprite[var_14].zvel = -(RandomSize(4) << 7);
        sprite[var_14].picnum = kTile985 + nVal;
    }

    sprite[var_14].z = sprite[nSprite].z;
    sprite[var_14].lotag = runlist_HeadRun() + 1;
    sprite[var_14].clipdist = 1;
    sprite[var_14].hitag = 0;

//	GrabTimeSlot(3);

    sprite[var_14].extra = -1;
    sprite[var_14].owner = runlist_AddRunRec(sprite[var_14].lotag - 1, var_14 | 0x260000);
    sprite[var_14].hitag = runlist_AddRunRec(NewRun, var_14 | 0x260000);

    return var_14;
}

void FuncSpark(int a, int, int nRun)
{
    int nSprite = RunData[nRun].nVal;
    assert(nSprite >= 0 && nSprite < kMaxSprites);

    int nMessage = a & 0x7F0000;

    if (nMessage != 0x20000) {
        return;
    }

    sprite[nSprite].shade += 3;
    sprite[nSprite].xrepeat -= 2;

    if (sprite[nSprite].xrepeat >= 4 && sprite[nSprite].shade <= 100)
    {
        sprite[nSprite].yrepeat -= 2;

        // calling BuildSpark() with 2nd parameter as '1' will set kTile986
        if (sprite[nSprite].picnum == kTile986 && (sprite[nSprite].xrepeat & 2))
        {
            BuildSpark(nSprite, 2);
        }

        if (sprite[nSprite].picnum >= kTile3000) {
            return;
        }

        sprite[nSprite].zvel += 128;

        int nMov = movesprite(nSprite, sprite[nSprite].xvel << 12, sprite[nSprite].yvel << 12, sprite[nSprite].zvel, 2560, -2560, CLIPMASK1);
        if (!nMov) {
            return;
        }

        if (sprite[nSprite].zvel <= 0) {
            return;
        }
    }

    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;

    if (sprite[nSprite].picnum > kTile3000) {
        nSmokeSparks--;
    }

    runlist_DoSubRunRec(sprite[nSprite].owner);
    runlist_FreeRun(sprite[nSprite].lotag - 1);
    runlist_SubRunRec(sprite[nSprite].hitag);
    mydeletesprite(nSprite);
}

void DimLights()
{
    static short word_96786 = 0;

    word_96786 = word_96786 == 0;
    if (word_96786 == 0)
        return;

    for (int i = 0; i < numsectors; i++)
    {
        if (sector[i].ceilingshade < 100)
            sector[i].ceilingshade++;

        if (sector[i].floorshade < 100)
            sector[i].floorshade++;

        short startwall = sector[i].wallptr;
        short endwall = startwall + sector[i].wallnum;

        for (int nWall = startwall; nWall < endwall; nWall++)
        {
            if (wall[nWall].shade < 100)
                wall[nWall].shade++;
        }
    }
}

void DoFinale()
{
    static int dword_96788 = 0;
    static int nextstage = 0;

    if (!lFinaleStart)
        return;

    dword_96788++;

    if (dword_96788 < 90)
    {
        if (!(dword_96788 & 2))
        {
            int nAng = RandomSize(11);
            sprite[nFinaleSpr].ang = nAng;
            BuildSpark(nFinaleSpr, 1);
        }

        if (!RandomSize(2))
        {
            PlayFX2(StaticSound[kSound78] | 0x2000, nFinaleSpr);

            for (int i = 0; i < nTotalPlayers; i++) {
                nQuake[i] = 1280;
            }
        }
    }
    else
    {
        DimLights();

        if (nDronePitch <= -2400)
        {
            if (nFinaleStage < 2)
            {
                if (nFinaleStage == 1)
                {
                    StopLocalSound();
                    PlayLocalSound(StaticSound[kSound76], 0);
                    nextstage = leveltime*4 + 120;
                    nFinaleStage++;
                }
            }
            else if (nFinaleStage <= 2)
            {
                if (leveltime*4 >= nextstage)
                {
                    PlayLocalSound(StaticSound[kSound77], 0);
                    nFinaleStage++;
                    nextstage = leveltime*4 + 360;
                }
            }
            else if (nFinaleStage == 3 && leveltime*4 >= nextstage)
            {
                LevelFinished();
            }
        }
        else
        {
            nDronePitch -= 128;
            BendAmbientSound();
            nFinaleStage = 1;
        }
    }
}

int BuildEnergyBlock(short nSector)
{
    short startwall = sector[nSector].wallptr;
    short nWalls = sector[nSector].wallnum;

    int x = 0;
    int y = 0;

    for (int i = 0; i < nWalls; i++)
    {
        x += wall[startwall + i].x;
        y += wall[startwall + i].y;

        wall[startwall + i].picnum = kClockSymbol16;
        wall[startwall + i].pal = 0;
        wall[startwall + i].shade = 50;
    }

    int xAvg = x / nWalls;
    int yAvg = y / nWalls;

    int nSprite = insertsprite(nSector, 406);

    short nextsector = wall[startwall].nextsector;

    sprite[nSprite].x = xAvg;
    sprite[nSprite].y = yAvg;

    sector[nSector].extra = nSprite;

//	GrabTimeSlot(3);

    sprite[nSprite].z = sector[nextsector].floorz;

    // CHECKME - name of this variable?
    int nRepeat = (sprite[nSprite].z - sector[nSector].floorz) >> 8;
    if (nRepeat > 255) {
        nRepeat = 255;
    }

    sprite[nSprite].xrepeat = nRepeat;
    sprite[nSprite].cstat = 0x8000;
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;
    sprite[nSprite].extra = -1;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].hitag = 0;
    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nSprite | 0x250000);

    nEnergyBlocks++;

    return nSprite | 0x250000;
}

// TODO - tidy
void KillCreatures()
{
    signed int v0;
    signed int v1;
    int i;

    v0 = 99;
    v1 = 99;

    while (1)
    {
        if (v0 != 100)
        {
            for (i = headspritestat[v1]; i != -1; i = nextspritestat[i])
            {
                runlist_DamageEnemy(i, -1, 1600);
            }
        }
        ++v0;
        ++v1;

        if (v0 > 107) {
            return;
        }
    }
}

void ExplodeEnergyBlock(int nSprite)
{
    short nSector = sprite[nSprite].sectnum;

    short startwall = sector[nSector].wallptr;
    short nWalls = sector[nSector].wallnum;

    int i;

    for (i = 0; i < nWalls; i++)
    {
        short nextwall = wall[startwall + i].nextwall;

        if (wall[nextwall].pal >= 4) {
            wall[nextwall].pal = 7;
        }
        else {
            wall[nextwall].pal = 0;
        }

        wall[nextwall].shade = 50;
    }

    if (sector[nSector].floorpal >= 4) {
        sector[nSector].floorpal = 7;
    }
    else {
        sector[nSector].floorpal = 0;
    }

    sector[nSector].floorshade = 50;
    sector[nSector].extra = -1;
    sector[nSector].floorz = sprite[nSprite].z;

    sprite[nSprite].z = (sprite[nSprite].z + sector[nSector].floorz) / 2;

    BuildSpark(nSprite, 3);

    sprite[nSprite].cstat = 0;
    sprite[nSprite].xrepeat = 100;

    PlayFX2(StaticSound[kSound78], nSprite);

    sprite[nSprite].xrepeat = 0;

    nEnergyTowers--;

    for (i = 0; i < 20; i++)
    {
        sprite[nSprite].ang = RandomSize(11);
        BuildSpark(nSprite, 1); // shoot out blue orbs
    }

    TintPalette(64, 64, 64);

    if (nEnergyTowers == 1)
    {
        runlist_ChangeChannel(nEnergyChan, nEnergyTowers);
        StatusMessage(1000, "TAKE OUT THE CONTROL CENTER!");
    }
    else if (nEnergyTowers != 0)
    {
        StatusMessage(500, "%d TOWERS REMAINING", nEnergyTowers);
    }
    else
    {
        nFinaleSpr = nSprite;
        lFinaleStart = leveltime*4;

        if (!lFinaleStart) {
            lFinaleStart = lFinaleStart + 1;
        }

        for (i = 0; i < numsectors; i++)
        {
            if (sector[i].ceilingpal == 1) {
                sector[i].ceilingpal = 0;
            }

            if (sector[i].floorpal == 1) {
                sector[i].floorpal = 0;
            }

            short startwall = sector[i].wallptr;
            short endwall = startwall + sector[i].wallnum;

            for (int nWall = startwall; nWall < endwall; nWall++)
            {
                if (wall[nWall].pal == 1) {
                    wall[nWall].pal = 0;
                }
            }
        }

        KillCreatures();
    }

    changespritestat(nSprite, 0);
}

void FuncEnergyBlock(int a, int nDamage, int nRun)
{
    short nSprite = RunData[nRun].nVal;

    int nMessage = a & 0x7F0000;

    switch (nMessage)
    {
        case 0x20000:
        case 0x30000:
        case 0x90000:
        {
            return;
        }

        case 0xA0000:
        {
            short nSector = sprite[nSprite].sectnum;

            if (sector[nSector].extra == -1) {
                return;
            }

            int nFloorZ = sector[nSector].floorz;

            sector[nSector].floorz = sprite[nSprite].z;
            sprite[nSprite].z -= 256;

            nDamage = runlist_CheckRadialDamage(nSprite);

            // restore previous values
            sector[nSector].floorz = nFloorZ;
            sprite[nSprite].z += 256;

            if (nDamage <= 0) {
                return;
            }

            // fall through to case 0x80000
            fallthrough__;
        }

        case 0x80000:
        {
            nDamage >>= 2;
            if (nDamage <= 0) {
                return;
            }

            if (nDamage < sprite[nSprite].xrepeat)
            {
                sprite[nSprite].xrepeat -= nDamage;

                int nSprite2 = insertsprite(lasthitsect, 0);

                sprite[nSprite2].ang = a & 0xFFFF;
                sprite[nSprite2].x = lasthitx;
                sprite[nSprite2].y = lasthity;
                sprite[nSprite2].z = lasthitz;

                BuildSpark(nSprite2, 0); // shoot out blue orb when damaged
                mydeletesprite(nSprite2);
            }
            else
            {
                sprite[nSprite].xrepeat = 0; // using xrepeat to store health
                ExplodeEnergyBlock(nSprite);
            }

            return;
        }
    }
}

int BuildObject(short nSprite, int nOjectType, int nHitag)
{
    if (ObjectCount >= kMaxObjects) {
        I_Error("Too many objects!\n");
    }

    short nObject = ObjectCount;
    ObjectCount++;

    changespritestat(nSprite, ObjectStatnum[nOjectType]);

    // 0x7FFD to ensure set as blocking ('B' and 'H') sprite and also disable translucency and set not invisible
    sprite[nSprite].cstat = (sprite[nSprite].cstat | 0x101) & 0x7FFD;
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;
    sprite[nSprite].extra = -1;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].hitag = 0;
    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nObject | 0x170000);

//	GrabTimeSlot(3);

    if (sprite[nSprite].statnum == kStatDestructibleSprite) {
        ObjectList[nObject].nHealth = 4;
    }
    else {
        ObjectList[nObject].nHealth = 120;
    }

    ObjectList[nObject].nSprite = nSprite;
    ObjectList[nObject].field_4 = runlist_AddRunRec(NewRun, nObject | 0x170000);

    short nSeq = ObjectSeq[nOjectType];

    if (nSeq > -1)
    {
        ObjectList[nObject].field_8 = SeqOffsets[nSeq];

        if (!nOjectType) // if not Explosion Trigger (e.g. Exploding Fire Cauldron)
        {
            ObjectList[nObject].field_0 = RandomSize(4) % (SeqSize[ObjectList[nObject].field_8] - 1);
        }

        int nSprite2 = insertsprite(sprite[nSprite].sectnum, 0);
        ObjectList[nObject].field_10 = nSprite2;

        sprite[nSprite2].cstat = 0x8000;
        sprite[nSprite2].x = sprite[nSprite].x;
        sprite[nSprite2].y = sprite[nSprite].y;
        sprite[nSprite2].z = sprite[nSprite].z;
    }
    else
    {
        ObjectList[nObject].field_0 = 0;
        ObjectList[nObject].field_8 = -1;

        if (sprite[nSprite].statnum == kStatDestructibleSprite) {
            ObjectList[nObject].field_10 = -1;
        }
        else {
            ObjectList[nObject].field_10 = -nHitag;
        }
    }

    return nObject | 0x170000;
}

// in-game destructable wall mounted screen
void ExplodeScreen(short nSprite)
{
    sprite[nSprite].z -= GetSpriteHeight(nSprite) / 2;

    for (int i = 0; i < 30; i++) {
        BuildSpark(nSprite, 0); // shoot out blue orbs
    }

    sprite[nSprite].cstat = 0x8000;
    PlayFX2(StaticSound[kSound78], nSprite);
}

void FuncObject(int a, int b, int nRun)
{
    short nObject = RunData[nRun].nVal;

    short nSprite = ObjectList[nObject].nSprite;
    short nStat = sprite[nSprite].statnum;
    short bx = ObjectList[nObject].field_8;

    int nMessage = a & 0x7F0000;

    switch (nMessage)
    {
        default:
        {
            DebugOut("unknown msg %d for Object\n", a & 0x7F0000);
            return;
        }

        case 0x30000:
            return;

        case 0x80000:
        {
            if (nStat >= 150 || ObjectList[nObject].nHealth <= 0) {
                return;
            }

            if (nStat == 98)
            {
                D3PlayFX((StaticSound[kSound47] | 0x2000) | (RandomSize(2) << 9), nSprite);
                return;
            }

            ObjectList[nObject].nHealth -= (short)b;
            if (ObjectList[nObject].nHealth > 0) {
                return;
            }

            if (nStat == kStatDestructibleSprite)
            {
                ExplodeScreen(nSprite);
            }
            else
            {
                ObjectList[nObject].nHealth = -(RandomSize(3) + 1);
            }

            return;
        }

        case 0x90000:
        {
            if (bx > -1)
            {
                seq_PlotSequence(a & 0xFFFF, bx, ObjectList[nObject].field_0, 1);
            }
            return;
        }

        case 0xA0000:
        {
            if (ObjectList[nObject].nHealth > 0 && sprite[nSprite].cstat & 0x101
                && (nStat != kStatExplodeTarget
                    || sprite[nRadialSpr].statnum == 201
                    || (nRadialBullet != 3 && nRadialBullet > -1)
                    || sprite[nRadialSpr].statnum == kStatExplodeTrigger))
            {
                int nDamage = runlist_CheckRadialDamage(nSprite);
                if (nDamage <= 0) {
                    return;
                }

                if (sprite[nSprite].statnum != kStatAnubisDrum) {
                    ObjectList[nObject].nHealth -= nDamage;
                }

                if (sprite[nSprite].statnum == kStatExplodeTarget)
                {
                    sprite[nSprite].xvel = 0;
                    sprite[nSprite].yvel = 0;
                    sprite[nSprite].zvel = 0;
                }
                else if (sprite[nSprite].statnum != kStatAnubisDrum)
                {
                    sprite[nSprite].xvel >>= 1;
                    sprite[nSprite].yvel >>= 1;
                    sprite[nSprite].zvel >>= 1;
                }

                if (ObjectList[nObject].nHealth > 0) {
                    return;
                }

                if (sprite[nSprite].statnum == kStatExplodeTarget)
                {
                    ObjectList[nObject].nHealth = -1;
                    short ax = ObjectList[nObject].field_10;

                    if (ax < 0 || ObjectList[ax].nHealth <= 0) {
                        return;
                    }

                    ObjectList[ax].nHealth = -1;
                }
                else if (sprite[nSprite].statnum == kStatDestructibleSprite)
                {
                    ObjectList[nObject].nHealth = 0;

                    ExplodeScreen(nSprite);
                }
                else
                {
                    ObjectList[nObject].nHealth = -(RandomSize(4) + 1);
                }
            }

            return;
        }

        case 0x20000:
        {
            if (nStat == 97 || (!(sprite[nSprite].cstat & 0x101))) {
                return;
            }

            if (nStat != kStatExplodeTarget) {
                Gravity(nSprite);
            }

            // do animation
            if (bx != -1)
            {
                ObjectList[nObject].field_0++;
                if (ObjectList[nObject].field_0 >= SeqSize[bx]) {
                    ObjectList[nObject].field_0 = 0;
                }

                sprite[nSprite].picnum = seq_GetSeqPicnum2(bx, ObjectList[nObject].field_0);
            }

            if (ObjectList[nObject].nHealth >= 0) {
                goto FUNCOBJECT_GOTO;
            }

            ObjectList[nObject].nHealth++;

            if (ObjectList[nObject].nHealth)
            {
FUNCOBJECT_GOTO:
                if (nStat != kStatExplodeTarget)
                {
                    int nMov = movesprite(nSprite, sprite[nSprite].xvel << 6, sprite[nSprite].yvel << 6, sprite[nSprite].zvel, 0, 0, CLIPMASK0);

                    if (sprite[nSprite].statnum == kStatExplodeTrigger) {
                        sprite[nSprite].pal = 1;
                    }

                    if (nMov & 0x20000)
                    {
                        sprite[nSprite].xvel -= sprite[nSprite].xvel >> 3;
                        sprite[nSprite].yvel -= sprite[nSprite].yvel >> 3;
                    }

                    if (((nMov & 0xC000) > 0x8000) && ((nMov & 0xC000) == 0xC000))
                    {
                        sprite[nSprite].yvel = 0;
                        sprite[nSprite].xvel = 0;
                    }
                }

                return;
            }
            else
            {
                int var_18;

                // red branch
                if ((nStat == kStatExplodeTarget) || (sprite[nSprite].z < sector[sprite[nSprite].sectnum].floorz))
                {
                    var_18 = 36;
                }
                else
                {
                    var_18 = 34;
                }

                AddFlash(sprite[nSprite].sectnum, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, 128);
                BuildAnim(-1, var_18, 0, sprite[nSprite].x, sprite[nSprite].y, sector[sprite[nSprite].sectnum].floorz, sprite[nSprite].sectnum, 240, 4);

//				int edi = nSprite | 0x4000;

                if (nStat == kStatExplodeTrigger)
                {
                    for (int i = 4; i < 8; i++) {
                        BuildCreatureChunk(nSprite | 0x4000, seq_GetSeqPicnum(kSeqFirePot, (i >> 2) + 1, 0));
                    }

                    runlist_RadialDamageEnemy(nSprite, 200, 20);
                }
                else if (nStat == kStatExplodeTarget)
                {
                    for (int i = 0; i < 8; i++) {
                        BuildCreatureChunk(nSprite | 0x4000, seq_GetSeqPicnum(kSeqFirePot, (i >> 1) + 3, 0));
                    }
                }

                if (currentLevel->levelNumber <= 20 || nStat != kStatExplodeTrigger)
                {
                    runlist_SubRunRec(sprite[nSprite].owner);
                    runlist_SubRunRec(ObjectList[nObject].field_4);

                    mydeletesprite(nSprite);
                    return;
                }
                else
                {
                    StartRegenerate(nSprite);
                    ObjectList[nObject].nHealth = 120;

                    sprite[nSprite].x = sprite[ObjectList[nObject].field_10].x;
                    sprite[nSprite].y = sprite[ObjectList[nObject].field_10].y;
                    sprite[nSprite].z = sprite[ObjectList[nObject].field_10].z;

                    mychangespritesect(nSprite, sprite[ObjectList[nObject].field_10].sectnum);
                    return;
                }
            }
        }
    }
}

void BuildDrip(int nSprite)
{
    if (nDrips >= kMaxDrips) {
        I_Error("Too many drips!\n");
    }

    sDrip[nDrips].nSprite = nSprite;
    sDrip[nDrips].field_2 = RandomSize(8) + 90;

    nDrips++;

    sprite[nSprite].cstat = 0x8000u;
}

void DoDrips()
{
    int i;

    for (i = 0; i < nDrips; i++)
    {
        sDrip[i].field_2--;
        if (sDrip[i].field_2 <= 0)
        {
            short nSprite = sDrip[i].nSprite;

            short nSeqOffset = SeqOffsets[kSeqDrips];

            if (!(SectFlag[sprite[nSprite].sectnum] & kSectLava)) {
                nSeqOffset++;
            }

            seq_MoveSequence(nSprite, nSeqOffset, RandomSize(2) % SeqSize[nSeqOffset]);

            sDrip[i].field_2 = RandomSize(8) + 90;
        }
    }

    for (i = 0; i < nBobs; i++)
    {
        sBob[i].field_2 += 4;

        int edx = Sin(sBob[i].field_2 << 3) >> 4;
        short nSector = sBob[i].nSector;

        if (sBob[i].field_3)
        {
            sector[nSector].ceilingz = edx + sBob[i].z;
        }
        else
        {
            int nFloorZ = sector[nSector].floorz;

            sector[nSector].floorz = edx + sBob[i].z;

            MoveSectorSprites(nSector, sector[nSector].floorz - nFloorZ);
        }
    }
}

void SnapBobs(short nSectorA, short nSectorB)
{
    int ecx = -1;
    int ebx = ecx;
//	int var_14 = nSector;
//	int edi = edx;

    for (int i = 0; i < nBobs; i++)
    {
        int esi = sBob[i].nSector;

        if (esi != nSectorA)
        {
            if (nSectorB != esi)
                continue;

            esi = ebx;
            ecx = i;
        }
        else
        {
            esi = ecx;
            ebx = i;
        }

        if (esi != -1) {
            break;
        }
    }

    if (ecx <= -1) {
        return;
    }

    if (ebx <= -1) {
        return;
    }

    sBob[ecx].field_2 = sBob[ebx].field_2;
}

void AddSectorBob(int nSector, int nHitag, int bx)
{
    if (nBobs >= kMaxBobs) {
        I_Error("Too many bobs!\n");
    }

    sBob[nBobs].field_3 = bx;

    int z;

    if (bx == 0) {
        z = sector[nSector].floorz;
    }
    else {
        z = sector[nSector].ceilingz;
    }

    sBob[nBobs].z = z;
    sBob[nBobs].field_2 = nHitag << 4;
    sBobID[nBobs] = nHitag;

    sBob[nBobs].nSector = nSector;

    SectFlag[nSector] |= 0x0010;

    nBobs++;
}

// Confirmed 100% correct with original .exe
int FindTrail(int nVal)
{
    for (int i = 0; i < nTrails; i++)
    {
        if (sTrail[i].field_2 == nVal)
            return i;
    }

    sTrail[nTrails].field_2 = nVal;
    sTrail[nTrails].field_0 = -1;

    return nTrails++;
}

// ok ?
void ProcessTrailSprite(int nSprite, int nLotag, int nHitag)
{
    if (nTrailPoints >= 100) {
        I_Error("Too many trail point sprites (900-949)... increase MAX_TRAILPOINTS\n");
    }

    short nPoint = nTrailPoints;
    nTrailPoints++;

    sTrailPoint[nPoint].x = sprite[nSprite].x;
    sTrailPoint[nPoint].y = sprite[nSprite].y;

    int nTrail = FindTrail(nHitag);

    int var_14 = nLotag - 900;

    nTrailPointVal[nPoint] = var_14;

    int field0 = sTrail[nTrail].field_0;

    if (field0 == -1)
    {
        sTrail[nTrail].field_0 = nPoint;
        sTrail[nTrail].field_4 = nPoint;

        nTrailPointNext[nPoint] = -1;
        nTrailPointPrev[nPoint] = -1;
    }
    else
    {
        int ecx = -1;

        while (field0 != -1)
        {
            if (nTrailPointVal[field0] > var_14)
            {
                nTrailPointPrev[nPoint] = nTrailPointPrev[field0];
                nTrailPointPrev[field0] = nPoint;
                nTrailPointNext[nPoint] = field0;

                if (field0 == sTrail[nTrail].field_0) {
                    sTrail[nTrail].field_0 = nPoint;
                }

                break;
            }

            ecx = field0;
            field0 = nTrailPointNext[field0];
        }

        if (field0 == -1)
        {
            nTrailPointNext[ecx] = nPoint;
            nTrailPointPrev[nPoint] = ecx;
            nTrailPointNext[nPoint] = -1;
            sTrail[nTrail].field_4 = nPoint;
        }
    }

    mydeletesprite(nSprite);
}

// ok?
void AddMovingSector(int nSector, int edx, int ebx, int ecx)
{
    if (nMoveSects >= kMaxMoveSects) {
        I_Error("Too many moving sectors\n");
    }

    CreatePushBlock(nSector);

    short nTrail = FindTrail(ebx);

    sMoveDir[nMoveSects] = 1;

    MoveSect *pMoveSect = &sMoveSect[nMoveSects];
    nMoveSects++;

    pMoveSect->nTrail = nTrail;
    pMoveSect->nTrailPoint = -1;
    pMoveSect->field_8 = -1;
    pMoveSect->field_6 = ecx;
    pMoveSect->field_10 = (edx / 1000) + 1;
    pMoveSect->nSector = nSector;

    if (ecx & 8)
    {
        pMoveSect->field_14 = runlist_AllocChannel(ebx % 1000);
    }
    else
    {
        pMoveSect->field_14 = -1;
    }

    sector[nSector].floorstat |= 0x40;
}

void DoMovingSects()
{
    for (int i = 0; i < nMoveSects; i++)
    {
        if (sMoveSect[i].nSector == -1) {
            continue;
        }

        if (sMoveSect[i].field_14 != -1 && !sRunChannels[sMoveSect[i].field_14].c) {
            continue;
        }

        short nSector = sMoveSect[i].nSector;
        short nBlock = sector[nSector].extra;

        BlockInfo *pBlockInfo = &sBlockInfo[nBlock];

        if (sMoveSect[i].nTrailPoint == -1)
        {
            if (sMoveSect[i].field_6 & 0x20)
            {
                runlist_ChangeChannel(sMoveSect[i].field_14, 0);
            }

            short ax;

            if (sMoveSect[i].field_6 & 0x10)
            {
                sMoveDir[i] = -sMoveDir[i];
                if (sMoveDir[i] > 0)
                {
                    ax = sTrail[sMoveSect[i].nTrail].field_0;
                }
                else
                {
                    ax = sTrail[sMoveSect[i].nTrail].field_4;
                }
            }
            else
            {
                ax = sTrail[sMoveSect[i].nTrail].field_0;
            }

            sMoveSect[i].nTrailPoint = ax;
        }

        short nTrail = sMoveSect[i].nTrailPoint;
//		TrailPoint *pTrail = &sTrailPoint[nTrail];

        // loc_23872:
        int nAngle = GetMyAngle(sTrailPoint[nTrail].x - pBlockInfo->x, sTrailPoint[nTrail].y - pBlockInfo->y);

        int nXVel = (Sin(nAngle + 512) << 4) * sMoveSect[i].field_10;
        int nYVel = (Sin(nAngle) << 4) * sMoveSect[i].field_10;

        int ebx = (sTrailPoint[nTrail].x - pBlockInfo->x) << 14;

        int eax = nXVel;
        if (eax < 0) {
            eax = -eax;
        }

        int edx = eax;
        eax = ebx;

        int ecx = (sTrailPoint[nTrail].y - pBlockInfo->y) << 14;

        if (eax < 0) {
            eax = -eax;
        }

        // loc_238EC:
        if (edx <= eax)
        {
            eax = nYVel;
            if (eax < 0) {
                eax = -eax;
            }

            edx = eax;
            eax = ecx;

            if (eax < 0) {
                eax = -eax;
            }

            if (edx > eax)
            {
                // loc_23908:
                nYVel = ecx;
                nXVel = ebx;

                if (sMoveDir[i] > 0)
                {
                    sMoveSect[i].nTrailPoint = nTrailPointNext[sMoveSect[i].nTrailPoint];
                }
                else
                {
                    sMoveSect[i].nTrailPoint = nTrailPointPrev[sMoveSect[i].nTrailPoint];
                }
            }
        }
        else
        {
            // repeat of code from loc_23908
            nYVel = ecx;
            nXVel = ebx;

            if (sMoveDir[i] > 0)
            {
                sMoveSect[i].nTrailPoint = nTrailPointNext[sMoveSect[i].nTrailPoint];
            }
            else
            {
                sMoveSect[i].nTrailPoint = nTrailPointPrev[sMoveSect[i].nTrailPoint];
            }
        }

        // loc_2393A:
        if (sMoveSect[i].field_8 != -1)
        {
            MoveSector(sMoveSect[i].field_8, -1, &nXVel, &nYVel);
        }

        int var_2C = nXVel;
        int var_30 = nYVel;

        MoveSector(nSector, -1, &nXVel, &nYVel);

        if (nXVel != var_2C || nYVel != var_30)
        {
            MoveSector(sMoveSect[i].field_8, -1, &var_2C, &var_30);
            MoveSector(sMoveSect[i].field_8, -1, &nXVel, &nYVel);
        }
    }
}

void PostProcess()
{
    int i, j;

    for (i = 0; i < nMoveSects; i++)
    {
        int nTrail = sMoveSect[i].nTrail;
        sMoveSect[i].nTrailPoint = sTrail[nTrail].field_0;

        if (sMoveSect[i].field_6 & 0x40) {
            runlist_ChangeChannel(sMoveSect[i].field_14, 1);
        }

        short nSector = sMoveSect[i].nSector;

        if (SectFlag[nSector] & kSectUnderwater)
        {
            sector[nSector].ceilingstat |= 0x40;
            sector[nSector].floorstat &= 0xBFFF;

            for (j = 0; j < nMoveSects; j++)
            {
                if (j != i && sMoveSect[i].nTrail == sMoveSect[j].nTrail)
                {
                    sMoveSect[j].field_8 = sMoveSect[i].nSector;

                    SnapSectors(sMoveSect[j].nSector, sMoveSect[i].nSector, 0);
                    sMoveSect[i].nSector = -1;
                }
            }
        }
    }

    for (i = 0; i < nBobs; i++)
    {
        if (sBob[i].field_3 == 0)
        {
            int bobID = sBobID[i];

            for (j = 0; j < nBobs; j++)
            {
                if (j != i)
                {
                    if (sBob[i].field_3 != 0 && sBobID[j] == bobID) {
                        SnapSectors(i, j, 0);
                    }
                }
            }
        }
    }

    if (currentLevel->levelNumber != kMap20)
    {
        // esi is i
        for (i = 0; i < numsectors; i++)
        {
            int var_20 = 30000;

            if (SectSpeed[i] && SectDepth[i] && !(SectFlag[i] & kSectLava))
            {
                SectSoundSect[i] = i;
                SectSound[i] = StaticSound[kSound43];
            }
            else
            {
                // ebp and ecx are j
                for (j = 0; j < numsectors; j++)
                {
                    // loc_23CA6:

                    if (i != j && SectSpeed[j] && !(SectFlag[i] & kSectLava))
                    {
                        int xVal = wall[sector[i].wallptr].x - wall[sector[j].wallptr].x;
                        if (xVal < 0) {
                            xVal = -xVal;
                        }

                        int yVal = wall[sector[i].wallptr].x - wall[sector[j].wallptr].x;
                        if (yVal < 0) {
                            yVal = -yVal;
                        }

                        if (xVal < 15000 && yVal < 15000 && (xVal + yVal < var_20))
                        {
                            var_20 = xVal + yVal;
                            SectSoundSect[i] = j;
                            SectSound[i] = StaticSound[kSound43];
                        }
                    }
                }
            }
        }
    }
    else // nMap == kMap20)
    {
        //		int var_24 = 0;
        int ebp = 0;

        for (i = 0; i < numsectors; i++)
        {
            SectSoundSect[i] = i;
            SectSound[i] = StaticSound[kSound62];

            int startwall = sector[ebp].wallptr;
            int endwall = sector[ebp].wallptr + sector[ebp].wallnum;

            int nWall = startwall;

            while (nWall < endwall)
            {
                if (wall[nWall].picnum == kTile3603)
                {
                    wall[nWall].pal = 1;
                    int nSprite = insertsprite(i, 407);
                    sprite[nSprite].cstat = 0x8000;
                }

                nWall++;
            }
        }

        for (i = 0; i < kMaxSprites; i++)
        {
            if (sprite[i].statnum < kMaxStatus && sprite[i].picnum == kTile3603)
            {
                changespritestat(i, 407);
                sprite[i].pal = 1;
            }
        }
    }

    // esi is i
    for (i = 0; i < ObjectCount; i++)
    {
        int nObjectSprite = ObjectList[i].nSprite;

        if (sprite[nObjectSprite].statnum == kStatExplodeTarget)
        {
            if (!ObjectList[i].field_10) {
                ObjectList[i].field_10 = -1;
            }
            else
            {
                int edi = ObjectList[i].field_10;
                ObjectList[i].field_10 = -1;

                // ecx, ebx is j
                for (j = 0; j < ObjectCount; j++)
                {
                    if (i != j && sprite[ObjectList[j].nSprite].statnum == kStatExplodeTarget && edi == ObjectList[j].field_10)
                    {
                        ObjectList[i].field_10 = j;
                        ObjectList[j].field_10 = i;
                    }
                }
            }
        }
    }
}


static SavegameHelper sghobj("objects",
    SA(sTrap),
    SA(sBob),
    SA(sTrail),
    SA(sTrailPoint),
    SA(Elevator),
    SA(ObjectList),
    SA(sMoveSect),
    SA(SlideData),
    SA(sMoveDir),
    SA(WallFace),
    SA(SlideData2),
    SA(PointList),
    SA(nTrapInterval),
    SA(sBobID),
    SA(PointFree),
    SA(SlideFree),
    SA(nTrailPointVal),
    SA(nTrailPointPrev),
    SA(nTrailPointNext),
    SA(sDrip),
    SV(PointCount),
    SV(SlideCount),
    SV(ElevCount),
    SV(WallFaceCount),
    SV(lFinaleStart),
    SV(nTrailPoints),
    SV(nEnergyBlocks),
    SV(nMoveSects),
    SV(nFinaleStage),
    SV(nTrails),
    SV(nTraps),
    SV(nFinaleSpr),
    SV(ObjectCount),
    SV(nDrips),
    SV(nBobs),
    SV(nDronePitch),
    SV(nSmokeSparks),
    nullptr);

END_PS_NS

