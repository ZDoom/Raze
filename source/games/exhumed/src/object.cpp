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
#include "interpolate.h"
#include "mapinfo.h"
#include <string.h>
#include <assert.h>

BEGIN_PS_NS

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
};

struct TrailPoint
{
    int x;
    int y;
    uint8_t nTrailPointVal;
    short nTrailPointPrev;
    short nTrailPointNext;

};

struct Bob
{
    int nSector;
    uint8_t field_2;
    uint8_t field_3;
    int z;
    short sBobID;

};

struct Drip
{
    DExhumedActor* pActor;
    short nCount;
};

// 56 bytes
struct Elev
{
    DExhumedActor* pActor;
    short field_0;
    short nChannel;
    int nSector;
    int field_6;
    int field_A;
    short nCountZOffsets; // count of items in zOffsets
    short nCurZOffset;
    int zOffsets[8]; // different Z offsets
    short field_32;
    short field_36;
};

// 16 bytes
struct MoveSect
{
    int nSector;
    short nTrail;
    short nTrailPoint;
    short field_6;
    short field_8; // nSector?
    int field_10;
    short field_14; // nChannel?
    short sMoveDir;

};

struct wallFace
{
    short nChannel;
    int nWall;
    short field_4;
    short field_6[8];
};

struct slideData
{
    DExhumedActor* pActor;
    short nChannel;
    short nStart;
    short field_4a;
    short field_8a;

    int field_0;
    int field_4;
    int field_8;
    int field_C;
    int x1;
    int y1;
    int x2;
    int y2;
    int x3;
    int y3;
    int x4;
    int y4;
    int x5;
    int y5;
    int x6;
    int y6;
};

struct Point
{
    int nSector;
    short field_2;
    short field_4;
    short field_6;
    short field_8;
    short field_A;
    short field_C;
    short nNext;
};

struct Trap
{
    DExhumedActor* pActor;

    short field_0;
    short nType;
    int field_6;
    int field_8; // wallnum
    short field_A;
    short field_C;
    short field_E;
    short nTrapInterval;

};


TArray<Point> PointList;
TArray<Trap> sTrap;
TArray<Bob> sBob;
TArray<Trail> sTrail;
TArray<TrailPoint> sTrailPoint;
TArray<Elev> Elevator;
TArray<DExhumedActor*> ObjectList;
TArray<MoveSect> sMoveSect;
TArray<slideData> SlideData;
TArray<wallFace> WallFace;
TArray<Drip> sDrip;
TArray<DExhumedActor*> EnergyBlocks;

int lFinaleStart;

short nFinaleStage;
DExhumedActor* pFinaleSpr;

short nDronePitch = 0;
short nSmokeSparks = 0;

FSerializer& Serialize(FSerializer& arc, const char* keyname, Trail& w, Trail* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at0", w.field_0)
            ("at2", w.field_2)
            ("at4", w.field_4)
            .EndObject();
    }
    return arc;
}
FSerializer& Serialize(FSerializer& arc, const char* keyname, TrailPoint& w, TrailPoint* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("x", w.x)
            ("y", w.y)
            ("val", w.nTrailPointVal)
            ("next", w.nTrailPointNext)
            ("prev", w.nTrailPointPrev)
            .EndObject();
    }
    return arc;
}
FSerializer& Serialize(FSerializer& arc, const char* keyname, Bob& w, Bob* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("sector", w.nSector)
            ("at2", w.field_2)
            ("at3", w.field_3)
            ("z", w.z)
            ("id", w.sBobID)
            .EndObject();
    }
    return arc;
}
FSerializer& Serialize(FSerializer& arc, const char* keyname, Drip& w, Drip* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("sprite", w.pActor)
            ("at2", w.nCount)
            .EndObject();
    }
    return arc;
}
FSerializer& Serialize(FSerializer& arc, const char* keyname, Elev& w, Elev* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at0", w.field_0)
            ("channel", w.nChannel)
            ("sector", w.nSector)
            ("at6", w.field_6)
            ("ata", w.field_A)
            ("countz", w.nCountZOffsets)
            ("curz", w.nCurZOffset)
            .Array("zofs", w.zOffsets, 8)
            ("at32", w.field_32)
            ("sprite", w.pActor)
            ("at36", w.field_36)
            .EndObject();
    }
    return arc;
}
FSerializer& Serialize(FSerializer& arc, const char* keyname, MoveSect& w, MoveSect* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("sector", w.nSector)
            ("trail", w.nTrail)
            ("trailpoint", w.nTrailPoint)
            ("at6", w.field_6)
            ("at8", w.field_8)
            ("at10", w.field_10)
            ("at14", w.field_14)
            ("movedir", w.sMoveDir)
            .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, wallFace& w, wallFace* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("channel", w.nChannel)
            ("wall", w.nWall)
            ("at4", w.field_4)
            .Array("at6", w.field_6, 8)
            .EndObject();
    }
    return arc;
}
FSerializer& Serialize(FSerializer& arc, const char* keyname, slideData& w, slideData* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at0", w.field_0)
            ("at4", w.field_4)
            ("at8", w.field_8)
            ("atc", w.field_C)
            ("x1", w.x1)
            ("y1", w.y1)
            ("x2", w.x2)
            ("y2", w.y2)
            ("at20", w.x3)
            ("at24", w.y3)
            ("at28", w.x4)
            ("at2c", w.y4)
            ("at30", w.x5)
            ("at34", w.y5)
            ("at38", w.x6)
            ("at3c", w.y6)
            ("channel", w.nChannel)
            ("at2a", w.nStart)
            ("at4a", w.field_4a)
            ("at6a", w.pActor)
            ("at8a", w.field_8a)
            .EndObject();
    }
    return arc;
}
FSerializer& Serialize(FSerializer& arc, const char* keyname, Point& w, Point* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at0", w.nSector)
            ("at2", w.field_2)
            ("at4", w.field_4)
            ("at6", w.field_6)
            ("at8", w.field_8)
            ("ata", w.field_A)
            ("atc", w.field_C)
            ("ate", w.nNext)
            .EndObject();
    }
    return arc;
}
FSerializer& Serialize(FSerializer& arc, const char* keyname, Trap& w, Trap* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at0", w.field_0)
            ("sprite", w.pActor)
            ("type", w.nType)
            ("at6", w.field_6)
            ("at8", w.field_8)
            ("ata", w.field_A)
            ("atc", w.field_C)
            ("ate", w.field_E)
            ("interval", w.nTrapInterval)
            .EndObject();
    }
    return arc;
}

void SerializeObjects(FSerializer& arc)
{
    if (arc.BeginObject("objects"))
    {
        arc("points", PointList)
            ("traps", sTrap)
            ("bob", sBob)
            ("trails", sTrail)
            ("trailpoints", sTrailPoint)
            ("elevators", Elevator)
            ("objects", ObjectList)
            ("movesect", sMoveSect)
            ("slides", SlideData)
            ("wallface", WallFace)
            ("drips", sDrip)
            ("finalestart", lFinaleStart)
            ("energyblocks", EnergyBlocks)
            ("finalestage", nFinaleStage)
            ("finalespr", pFinaleSpr)
            ("dronepitch", nDronePitch)
            ("smokesparks", nSmokeSparks)
            .EndObject();
    }
}

// done
void InitObjects()
{
    sTrap.Clear();
    sBob.Clear();
    sTrail.Clear();
    sTrailPoint.Clear();
    ObjectList.Clear();
    sMoveSect.Clear();
    sDrip.Clear();
    EnergyBlocks.Clear();

    nDronePitch = 0;
    nFinaleStage = 0;
    lFinaleStart = 0;
    nSmokeSparks = 0;
}

// done
void InitElev()
{
    Elevator.Clear();
}

// done
DExhumedActor* BuildWallSprite(int nSector)
{
    auto wal = sector[nSector].firstWall();

    auto pActor = insertActor(nSector, 401);
    auto pSprite = &pActor->s();

	pSprite->pos.vec2 = wal->center();
    pSprite->z = (sector[nSector].floorz + sector[nSector].ceilingz) / 2;
    pSprite->cstat = 0x8000;

    return pActor;
 }

// done
DExhumedActor* FindWallSprites(int nSector)
{
    int var_24 = 0x7FFFFFFF;
    int ecx = 0x7FFFFFFF;

    int esi = 0x80000002;
    int edi = 0x80000002;

	for (auto& wal : wallsofsector(nSector))
    {
        if (wal.x < var_24) {
            var_24 = wal.x;
        }

        if (esi < wal.x) {
            esi = wal.x;
        }

        if (ecx > wal.y) {
            ecx = wal.y;
        }

        if (edi < wal.y) {
            edi = wal.y;
        }
    }

    ecx -= 5;
    esi += 5;
    edi += 5;
    var_24 -= 5;

    DExhumedActor* pAct = nullptr;

    ExhumedSpriteIterator it;
    while (auto actor = it.Next())
    {
        auto spr = &actor->s();
        if (spr->lotag == 0)
        {
            if ((spr->cstat & 0x50) == 80)
            {
                int var_28 = spr->x;
                int ebx = spr->y;

                if ((var_28 >= var_24) && (esi >= var_28) && (ebx >= ecx) && (ebx <= edi))
                {
                    actor->pTarget = pAct;
                    pAct = actor;
                }
            }
        }
    }

    if (pAct == nullptr)
    {
        pAct = insertActor(nSector, 401);
        auto pSprite = &pAct->s();

        pSprite->x = (var_24 + esi) / 2;
        pSprite->y = (ecx + edi) / 2;
        pSprite->z = sector[nSector].floorz;
        pSprite->cstat = 0x8000;
        pSprite->owner = -1;
        pSprite->lotag = 0;
        pSprite->hitag = 0;
    }

    return pAct;
}

int BuildElevF(int nChannel, int nSector, DExhumedActor* nWallSprite, int arg_4, int arg_5, int nCount, ...)
{
    auto ElevCount = Elevator.Reserve(1);

    Elevator[ElevCount].field_0 = 2;
    Elevator[ElevCount].field_6 = arg_4;
    Elevator[ElevCount].field_32 = -1;
    Elevator[ElevCount].field_A = arg_5;
    Elevator[ElevCount].nChannel = nChannel;
    Elevator[ElevCount].nSector = nSector;
    Elevator[ElevCount].nCountZOffsets = 0;
    Elevator[ElevCount].nCurZOffset = 0;
    Elevator[ElevCount].field_36 = 0;

    if (nWallSprite == nullptr) {
        nWallSprite = BuildWallSprite(nSector);
    }

    Elevator[ElevCount].pActor = nWallSprite;

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

int BuildElevC(int arg1, int nChannel, int nSector, DExhumedActor* nWallSprite, int arg5, int arg6, int nCount, ...)
{
    int edi = arg5;

    auto ElevCount = Elevator.Reserve(1);

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

    if (nWallSprite == nullptr) {
        nWallSprite = BuildWallSprite(nSector);
    }

    Elevator[ElevCount].pActor = nWallSprite;

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
int LongSeek(int* pZVal, int a2, int a3, int a4)
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
int CheckSectorSprites(int nSector, int nVal)
{
    int b = 0;

    if (nVal)
    {
        int nZDiff = sector[nSector].floorz - sector[nSector].ceilingz;

        ExhumedSectIterator it(nSector);
        while (auto pActor= it.Next())
        {
            auto pSprite = &pActor->s();
            if ((pSprite->cstat & 0x101) && (nZDiff < GetActorHeight(pActor)))
            {
                if (nVal != 1) {
                    return 1;
                }

                b = 1;

                runlist_DamageEnemy(pActor, nullptr, 5);

                if (pSprite->statnum == 100 && PlayerList[GetPlayerFromActor(pActor)].nHealth <= 0)
                {
                    PlayFXAtXYZ(StaticSound[kSoundJonFDie],
                        pSprite->x,
                        pSprite->y,
                        pSprite->z,
                        pSprite->sectnum, CHANF_NONE, 0x4000);
                }
            }
        }
    }
    else
    {
        ExhumedSectIterator it(nSector);
        while (auto pActor = it.Next())
        {
            if (pActor->s().cstat & 0x101) {
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
    int newz = sector[nSector].floorz;
    int oldz = newz - z;
    int minz = min(newz, oldz);
    int maxz = max(newz, oldz);
    ExhumedSectIterator it(nSector);
    while (auto pActor = it.Next())
    {
        auto pSprite = &pActor->s();
        int z = pSprite->z;
        if ((pSprite->statnum != 200 && z >= minz && z <= maxz) || pSprite->statnum >= 900)
        {
            pSprite->z = newz;
        }
    }
}

void StartElevSound(DExhumedActor* pActor, int nVal)
{
    short nSound;

    if (nVal & 2) {
        nSound = nElevSound;
    }
    else {
        nSound = nStoneSound;
    }

    D3PlayFX(StaticSound[nSound], pActor);
}

void AIElev::ProcessChannel(RunListEvent* ev)
{
    int nRun = ev->nRun;
    short nElev = RunData[nRun].nObjIndex;
    assert(nElev >= 0 && nElev < (int)Elevator.Size());

    short nChannel = Elevator[nElev].nChannel;
    short var_18 = Elevator[nElev].field_0;

    assert(nChannel >= 0 && nChannel < kMaxChannels);

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
                Elevator[nElev].field_32 = runlist_AddRunRec(NewRun, &RunData[nRun]);
                StartElevSound(Elevator[nElev].pActor, var_18);

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
            Elevator[nElev].field_32 = runlist_AddRunRec(NewRun, &RunData[nRun]);

            StartElevSound(Elevator[nElev].pActor, var_18);
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
}

void AIElev::Tick(RunListEvent* ev)
{
    int nRun = ev->nRun;
    short nElev = RunData[nRun].nObjIndex;
    assert(nElev >= 0 && nElev < (int)Elevator.Size());

    short nChannel = Elevator[nElev].nChannel;
    short var_18 = Elevator[nElev].field_0;

    assert(nChannel >= 0 && nChannel < kMaxChannels);

    int nSector =Elevator[nElev].nSector;
    auto pElevSpr = Elevator[nElev].pActor;

    int ebp = 0; // initialise to *something*

    if (var_18 & 0x2)
    {
        int nZOffset = Elevator[nElev].nCurZOffset;
        int nZVal = Elevator[nElev].zOffsets[nZOffset];

        short nSectorB = nSector;

        StartInterpolation(nSector, Interp_Sect_Floorz);
        int nVal = LongSeek((int*)&sector[nSector].floorz, nZVal, Elevator[nElev].field_6, Elevator[nElev].field_A);
        ebp = nVal;

        if (!nVal)
        {
            if (var_18 & 0x10)
            {
                Elevator[nElev].nCurZOffset ^= 1;
                StartElevSound(pElevSpr, var_18);
            }
            else
            {
                StopActorSound(pElevSpr);
                runlist_SubRunRec(nRun);
                Elevator[nElev].field_32 = -1;
                runlist_ReadyChannel(nChannel);

                D3PlayFX(StaticSound[nStopSound], Elevator[nElev].pActor);
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
        sectortype* cursect = &sector[nSector];

        int nZOffset = Elevator[nElev].nCurZOffset;
        int zVal = Elevator[nElev].zOffsets[nZOffset];

        StartInterpolation(nSector, Interp_Sect_Ceilingz);
        int nVal = LongSeek(&ceilZ, zVal, Elevator[nElev].field_6, Elevator[nElev].field_A);
        ebp = nVal;

        if (!nVal)
        {
            if (var_18 & 0x10)
            {
                Elevator[nElev].nCurZOffset ^= 1;

                StartElevSound(Elevator[nElev].pActor, var_18);
            }
            else
            {
                runlist_SubRunRec(nRun);
                Elevator[nElev].field_32 = -1;
                StopActorSound(Elevator[nElev].pActor);
                D3PlayFX(StaticSound[nStopSound], Elevator[nElev].pActor);
                runlist_ReadyChannel(nChannel);
            }

            return;
        }
        else if (nVal > 0)
        {
            if (ceilZ == zVal)
            {
                if (var_18 & 0x4) {
                    SetQuake(pElevSpr, 30);
                }

                auto sp = &pElevSpr->s();
                PlayFXAtXYZ(StaticSound[kSound26], sp->x, sp->y, sp->z, sp->sectnum);
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

        StartInterpolation(nSector, Interp_Sect_Ceilingz);
        cursect->ceilingz = ceilZ;
    }

    // maybe this doesn't go here?
    while (pElevSpr)
    {
        pElevSpr->s().z += ebp;
        pElevSpr = pElevSpr->pTarget;
    }
}


// done
void InitWallFace()
{
    WallFace.Clear();
}

int BuildWallFace(short nChannel, int nWall, int nCount, ...)
{
    auto WallFaceCount = WallFace.Reserve(1);

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

    return WallFaceCount;
}

void AIWallFace::ProcessChannel(RunListEvent* ev)
{
    int nWallFace = RunData[ev->nRun].nObjIndex;
    assert(nWallFace >= 0 && nWallFace < (int)WallFace.Size());

    short nChannel = WallFace[nWallFace].nChannel;

    short si = sRunChannels[nChannel].c;

    if ((si <= WallFace[nWallFace].field_4) && (si >= 0))
    {
        wall[WallFace[nWallFace].nWall].picnum = WallFace[nWallFace].field_6[si];
    }
}

// done
void InitPoint()
{
    PointList.Clear();
}

// done
int GrabPoint()
{
    return PointList.Reserve(1);
}

// done
void InitSlide()
{
    SlideData.Clear();
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

int BuildSlide(int nChannel, int nStartWall, int nWall1, int ecx, int nWall2, int nWall3, int nWall4)
{
    auto nSlide = SlideData.Reserve(1);

    int nSector =IdentifySector(nStartWall);

    SlideData[nSlide].field_4a = -1;
    SlideData[nSlide].nChannel = nChannel;
    SlideData[nSlide].nStart = -1;

    int nPoint = GrabPoint();

    SlideData[nSlide].nStart = nPoint;

    PointList[nPoint].nNext = -1;
    PointList[nPoint].nSector = nSector;

    int startwall = sector[nSector].wallptr;
    int endwall = startwall + sector[nSector].wallnum;

    for (int nWall = startwall; nWall < endwall; nWall++)
    {
        short ax = SlideData[nSlide].nStart;

        if (ax >= 0)
        {
            while (ax >= 0)
            {
                if (wall[nWall].nextsector == PointList[ax].nSector) {
                    break;
                }

                ax = PointList[ax].nNext;
            }
        }
        else
        {
            if (wall[nWall].nextsector >= 0)
            {
                nPoint = GrabPoint();

                PointList[nPoint].nNext = SlideData[nSlide].nStart;
                PointList[nPoint].nSector = wall[nWall].nextsector;

                SlideData[nSlide].nStart = nPoint;
            }
        }
    }

    SlideData[nSlide].field_0 = nStartWall;
    SlideData[nSlide].field_4 = nWall1;
    SlideData[nSlide].field_8 = nWall2;
    SlideData[nSlide].field_C = nWall3;

    SlideData[nSlide].x1 = wall[nStartWall].x;
    SlideData[nSlide].y1 = wall[nStartWall].y;

    SlideData[nSlide].x2 = wall[nWall2].x;
    SlideData[nSlide].y2 = wall[nWall2].y;

    SlideData[nSlide].x3 = wall[nWall1].x;
    SlideData[nSlide].y3 = wall[nWall1].y;

    SlideData[nSlide].x4 = wall[nWall3].x;
    SlideData[nSlide].y4 = wall[nWall3].y;

    SlideData[nSlide].x5 = wall[ecx].x;
    SlideData[nSlide].y5 = wall[ecx].y;

    SlideData[nSlide].x6 = wall[nWall4].x;
    SlideData[nSlide].y6 = wall[nWall4].y;

    StartInterpolation(nStartWall, Interp_Wall_X);
    StartInterpolation(nStartWall, Interp_Wall_Y);

    StartInterpolation(nWall1, Interp_Wall_X);
    StartInterpolation(nWall1, Interp_Wall_Y);

    StartInterpolation(nWall2, Interp_Wall_X);
    StartInterpolation(nWall2, Interp_Wall_Y);

    StartInterpolation(nWall3, Interp_Wall_X);
    StartInterpolation(nWall3, Interp_Wall_Y);


    auto pActor = insertActor(nSector, 899);
    auto pSprite = &pActor->s();

    SlideData[nSlide].pActor = pActor;
    pSprite->cstat = 0x8000;
    pSprite->x = wall[nStartWall].x;
    pSprite->y = wall[nStartWall].y;
    pSprite->z = sector[nSector].floorz;
    pSprite->backuppos();

    SlideData[nSlide].field_8a = 0;

    return nSlide;
}

void AISlide::ProcessChannel(RunListEvent* ev)
{
    int nRun = ev->nRun;
    int nSlide = RunData[nRun].nObjIndex;
    assert(nSlide >= 0 && nSlide < (int)SlideData.Size());

    short nChannel = SlideData[nSlide].nChannel;

    if (SlideData[nSlide].field_4a >= 0)
    {
        runlist_SubRunRec(SlideData[nSlide].field_4a);
        SlideData[nSlide].field_4a = -1;
    }

    if (sRunChannels[nChannel].c && sRunChannels[nChannel].c != 1) {
        return;
    }

    SlideData[nSlide].field_4a = runlist_AddRunRec(NewRun, &RunData[nRun]);

    if (SlideData[nSlide].field_8a != sRunChannels[nChannel].c)
    {
        D3PlayFX(StaticSound[kSound23], SlideData[nSlide].pActor);
        SlideData[nSlide].field_8a = sRunChannels[nChannel].c;
    }
}

void AISlide::Tick(RunListEvent* ev)
{
    int nRun = ev->nRun;
    int nSlide = RunData[nRun].nObjIndex;
    assert(nSlide >= 0 && nSlide < (int)SlideData.Size());

    short nChannel = SlideData[nSlide].nChannel;
    int ebp = 0;

    short cx = sRunChannels[nChannel].c;

    int clipmask = ebp + 1; // RENAME

    if (cx == 1)
    {
        int nWall = SlideData[nSlide].field_4;
        int x = wall[nWall].x;
        int y = wall[nWall].y;

        int nSeekA = LongSeek(&x, SlideData[nSlide].x5, 20, 20);
        int var_34 = nSeekA;
        int var_20 = nSeekA;

        int nSeekB = LongSeek(&y, SlideData[nSlide].y5, 20, 20);
        int var_2C = nSeekB;
        int var_24 = nSeekB;

        dragpoint(SlideData[nSlide].field_4, x, y);
        movesprite(SlideData[nSlide].pActor, var_34 << 14, var_2C << 14, 0, 0, 0, CLIPMASK1);

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

        dragpoint(SlideData[nSlide].field_0, x, y);

        nWall = SlideData[nSlide].field_C;

        x = wall[nWall].x;
        y = wall[nWall].y;

        int nSeekC = LongSeek(&x, SlideData[nSlide].x6, 20, 20);
        int var_30 = nSeekC;
        var_20 = nSeekC;

        int nSeekD = LongSeek(&y, SlideData[nSlide].y6, 20, 20);
        int edi = nSeekD;
        var_24 = nSeekD;

        dragpoint(SlideData[nSlide].field_C, x, y);

        if (var_30 == 0 && edi == 0) {
            ebp++;
        }

        nWall = SlideData[nSlide].field_8;

        x = wall[nWall].x + var_20;
        y = wall[nWall].y + var_24;

        dragpoint(SlideData[nSlide].field_8, x, y);
    }
    else if (cx == 0) // right branch
    {
        int nWall = SlideData[nSlide].field_0;
        int x = wall[nWall].x;
        int y = wall[nWall].y;

        int nSeekA = LongSeek(&x, SlideData[nSlide].x1, 20, 20);
        int edi = nSeekA;
        int var_1C = nSeekA;

        int nSeekB = LongSeek(&y, SlideData[nSlide].y1, 20, 20);
        int ecx = nSeekB;
        int var_28 = nSeekB;

        dragpoint(SlideData[nSlide].field_0, x, y);

        if (edi == 0 && ecx == 0) {
            ebp = clipmask;
        }

        nWall = SlideData[nSlide].field_4;

        y = wall[nWall].y + var_28;
        x = wall[nWall].x + var_1C;

        dragpoint(SlideData[nSlide].field_4, x, y);

        nWall = SlideData[nSlide].field_8;

        x = wall[nWall].x;
        y = wall[nWall].y;

        int nSeekC = LongSeek(&x, SlideData[nSlide].x2, 20, 20);
        edi = nSeekC;
        var_1C = nSeekC;

        int nSeekD = LongSeek(&y, SlideData[nSlide].y2, 20, 20);
        ecx = nSeekD;
        var_28 = nSeekD;

        dragpoint(SlideData[nSlide].field_8, x, y);

        if (edi == 0 && ecx == 0) {
            ebp++;
        }

        nWall = SlideData[nSlide].field_C;

        y = wall[nWall].y + var_28;
        x = wall[nWall].x + var_1C;

        dragpoint(SlideData[nSlide].field_C, x, y);
    }

    // loc_21A51:
    if (ebp >= 2)
    {
        runlist_SubRunRec(SlideData[nSlide].field_4a);

        SlideData[nSlide].field_4a = -1;
        D3PlayFX(StaticSound[nStopSound], SlideData[nSlide].pActor);

        runlist_ReadyChannel(nChannel);
    }
}

int BuildTrap(DExhumedActor* pActor, int edx, int ebx, int ecx)
{
    auto pSprite = &pActor->s();
    int var_14 = edx;
    int var_18 = ebx;
    int var_10 = ecx;

    int nTrap = sTrap.Reserve(1);

    ChangeActorStat(pActor, 0);

    pSprite->cstat = 0x8000;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->extra = -1;

    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->hitag = runlist_AddRunRec(NewRun, nTrap, 0x1F0000);
    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, nTrap, 0x1F0000);

    //	GrabTimeSlot(3);

    sTrap[nTrap].pActor = pActor;
    sTrap[nTrap].nType = (var_14 == 0) + 14;
    sTrap[nTrap].field_0 = -1;

    sTrap[nTrap].nTrapInterval = 64 - (2 * var_10);
    if (sTrap[nTrap].nTrapInterval < 5) {
        sTrap[nTrap].nTrapInterval = 5;
    }

    sTrap[nTrap].field_C = 0;
    sTrap[nTrap].field_A = 0;

    if (var_18 == -1) {
        return nTrap;
    }

    sTrap[nTrap].field_6 = -1;
    sTrap[nTrap].field_8 = -1;

    int nSector =pSprite->sectnum;
    int nWall = sector[nSector].wallptr;

    int i = 0;

    while (1)
    {
        if (sector[nSector].wallnum >= i) {
            break;
        }

        if (var_18 == wall[nWall].hitag)
        {
            if (sTrap[nTrap].field_6 != -1)
            {
                sTrap[nTrap].field_8 = nWall;
                sTrap[nTrap].field_C = wall[nWall].picnum;
                break;
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
    pSprite->backuppos();
    return nTrap;
}

void AITrap::ProcessChannel(RunListEvent* ev)
{
    short nChannel = ev->nParam & 0x3FFF;
    short nTrap = RunData[ev->nRun].nObjIndex;

    if (sRunChannels[nChannel].c > 0)
    {
        sTrap[nTrap].field_0 = 12;
    }
    else
    {
        sTrap[nTrap].field_0 = -1;
    }
}

void AITrap::Tick(RunListEvent* ev)
{
    short nTrap = RunData[ev->nRun].nObjIndex;
    auto pActor = sTrap[nTrap].pActor;
    auto pSprite = &pActor->s();

    if (sTrap[nTrap].field_0 >= 0)
    {
        sTrap[nTrap].field_0--;
        if (sTrap[nTrap].field_0 > 10) {
            return;
        }

        short nType = sTrap[nTrap].nType;

        if (sTrap[nTrap].field_0 == 0)
        {
            sTrap[nTrap].field_0 = sTrap[nTrap].nTrapInterval;

            if (nType == 14)
            {
                int nWall = sTrap[nTrap].field_6;
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

            auto pBullet = BuildBullet(pActor, nType, 0, pSprite->ang, nullptr, 1);
            if (pBullet)
            {
                if (nType == 15)
                {
                    pBullet->s().ang = (pBullet->s().ang - 512) & kAngleMask;
                    D3PlayFX(StaticSound[kSound32], pBullet);
                }
                else
                {
                    pBullet->s().clipdist = 50;

                    int nWall = sTrap[nTrap].field_6;
                    if (nWall > -1)
                    {
                        wall[nWall].picnum = sTrap[nTrap].field_A + 1;
                    }

                    nWall = sTrap[nTrap].field_8;
                    if (nWall > -1)
                    {
                        wall[nWall].picnum = sTrap[nTrap].field_C + 1;
                    }

                    D3PlayFX(StaticSound[kSound36], pBullet);
                }
            }
        }
    }
}

int BuildArrow(DExhumedActor* nSprite, int nVal)
{
    return BuildTrap(nSprite, 0, -1, nVal);
}

int BuildFireBall(DExhumedActor* nSprite, int a, int b)
{
    return BuildTrap(nSprite, 1, a, b);
}

DExhumedActor* BuildSpark(DExhumedActor* pActor, int nVal)
{
    auto pSprite = &pActor->s();
    auto pSpark = insertActor(pSprite->sectnum, 0);

    auto spr = &pSpark->s();

    spr->x = pSprite->x;
    spr->y = pSprite->y;
    spr->cstat = 0;
    spr->shade = -127;
    spr->pal = 1;
    spr->xoffset = 0;
    spr->yoffset = 0;
    spr->xrepeat = 50;
    spr->yrepeat = 50;

    if (nVal >= 2)
    {
        spr->picnum = kEnergy2;
        nSmokeSparks++;

        if (nVal == 3)
        {
            spr->xrepeat = 120;
            spr->yrepeat = 120;
        }
        else
        {
            spr->xrepeat = pSprite->xrepeat + 15;
            spr->yrepeat = pSprite->xrepeat + 15;
        }
    }
    else
    {
        int nAngle = (pSprite->ang + 256) - RandomSize(9);

        if (nVal)
        {
            spr->xvel = bcos(nAngle, -5);
            spr->yvel = bsin(nAngle, -5);
        }
        else
        {
            spr->xvel = bcos(nAngle, -6);
            spr->yvel = bsin(nAngle, -6);
        }

        spr->zvel = -(RandomSize(4) << 7);
        spr->picnum = kTile985 + nVal;
    }

    spr->z = pSprite->z;
    spr->lotag = runlist_HeadRun() + 1;
    spr->clipdist = 1;
    spr->hitag = 0;
    spr->backuppos();

    //	GrabTimeSlot(3);

    spr->extra = -1;
    spr->owner = runlist_AddRunRec(spr->lotag - 1, pSpark, 0x260000);
    spr->hitag = runlist_AddRunRec(NewRun, pSpark, 0x260000);

    return pSpark;
}

void AISpark::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    auto pSprite = &pActor->s();

    pSprite->shade += 3;
    pSprite->xrepeat -= 2;

    if (pSprite->xrepeat >= 4 && pSprite->shade <= 100)
    {
        pSprite->yrepeat -= 2;

        // calling BuildSpark() with 2nd parameter as '1' will set kTile986
        if (pSprite->picnum == kTile986 && (pSprite->xrepeat & 2))
        {
            BuildSpark(pActor, 2);
        }

        if (pSprite->picnum >= kTile3000) {
            return;
        }

        pSprite->zvel += 128;

        auto nMov = movesprite(pActor, pSprite->xvel << 12, pSprite->yvel << 12, pSprite->zvel, 2560, -2560, CLIPMASK1);
        if (!nMov.type && !nMov.exbits) {
            return;
        }

        if (pSprite->zvel <= 0) {
            return;
        }
    }

    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;

    if (pSprite->picnum > kTile3000) {
        nSmokeSparks--;
    }

    runlist_DoSubRunRec(pSprite->owner);
    runlist_FreeRun(pSprite->lotag - 1);
    runlist_SubRunRec(pSprite->hitag);
    DeleteActor(pActor);
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

        int startwall = sector[i].wallptr;
        int endwall = startwall + sector[i].wallnum;

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
            pFinaleSpr->s().ang = nAng;
            BuildSpark(pFinaleSpr, 1);
        }

        if (!RandomSize(2))
        {
            PlayFX2(StaticSound[kSound78] | 0x2000, pFinaleSpr);

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
                    nextstage = PlayClock + 120;
                    nFinaleStage++;
                }
            }
            else if (nFinaleStage <= 2)
            {
                if (PlayClock >= nextstage)
                {
                    PlayLocalSound(StaticSound[kSound77], 0);
                    nFinaleStage++;
                    nextstage = PlayClock + 360;
                }
            }
            else if (nFinaleStage == 3 && PlayClock >= nextstage)
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

DExhumedActor* BuildEnergyBlock(int nSector)
{
    int startwall = sector[nSector].wallptr;
    int nWalls = sector[nSector].wallnum;

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

    auto pActor = insertActor(nSector, 406);
    auto spr = &pActor->s();


    short nextsector = wall[startwall].nextsector;

    spr->x = xAvg;
    spr->y = yAvg;

    sector[nSector].extra = (int16_t)EnergyBlocks.Push(pActor);

    //	GrabTimeSlot(3);

    spr->z = sector[nextsector].floorz;

    // CHECKME - name of this variable?
    int nRepeat = (spr->z - sector[nSector].floorz) >> 8;
    if (nRepeat > 255) {
        nRepeat = 255;
    }

    spr->xrepeat = nRepeat;
    spr->cstat = 0x8000;
    spr->xvel = 0;
    spr->yvel = 0;
    spr->zvel = 0;
    spr->extra = -1;
    spr->lotag = runlist_HeadRun() + 1;
    spr->hitag = 0;
    spr->owner = runlist_AddRunRec(spr->lotag - 1, pActor, 0x250000);
    spr->backuppos();

    return pActor;
}

// TODO - tidy
void KillCreatures()
{
    signed int v0;
    signed int v1;

    v0 = 99;
    v1 = 99;

    while (1)
    {
        if (v0 != 100)
        {
            ExhumedStatIterator it(v1);
            while (auto i = it.Next())
            {
                runlist_DamageEnemy(i, nullptr, 1600);
            }
        }
        ++v0;
        ++v1;

        if (v0 > 107) {
            return;
        }
    }
}

void ExplodeEnergyBlock(DExhumedActor* pActor)
{
    auto pSprite = &pActor->s();

    int nSector =pSprite->sectnum;

    int startwall = sector[nSector].wallptr;
    int nWalls = sector[nSector].wallnum;

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
    sector[nSector].floorz = pSprite->z;

    pSprite->z = (pSprite->z + sector[nSector].floorz) / 2;

    BuildSpark(pActor, 3);

    pSprite->cstat = 0;
    pSprite->xrepeat = 100;

    PlayFX2(StaticSound[kSound78], pActor);

    pSprite->xrepeat = 0;

    nEnergyTowers--;

    for (i = 0; i < 20; i++)
    {
        pSprite->ang = RandomSize(11);
        BuildSpark(pActor, 1); // shoot out blue orbs
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
        pFinaleSpr = pActor;
        lFinaleStart = PlayClock;

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

            int startwall = sector[i].wallptr;
            int endwall = startwall + sector[i].wallnum;

            for (int nWall = startwall; nWall < endwall; nWall++)
            {
                if (wall[nWall].pal == 1) {
                    wall[nWall].pal = 0;
                }
            }
        }

        KillCreatures();
    }

    ChangeActorStat(pActor, 0);
}

void AIEnergyBlock::Damage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    auto spr = &pActor->s();

    ev->nDamage >>= 2;
    if (ev->nDamage <= 0) {
        return;
    }

    if (ev->nDamage < spr->xrepeat)
    {
        spr->xrepeat -= ev->nDamage;

        auto pActor2 = insertActor(lasthitsect, 0);
        auto pSprite2 = &pActor2->s();

        pSprite2->ang = ev->nParam;
        pSprite2->x = lasthitx;
        pSprite2->y = lasthity;
        pSprite2->z = lasthitz;

        BuildSpark(pActor2, 0); // shoot out blue orb when damaged
        DeleteActor(pActor2);
    }
    else
    {
        spr->xrepeat = 0; // using xrepeat to store health
        ExplodeEnergyBlock(pActor);
    }
}

void AIEnergyBlock::RadialDamage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    auto spr = &pActor->s();

    int nSector =spr->sectnum;

    if (sector[nSector].extra == -1) {
        return;
    }

    int nFloorZ = sector[nSector].floorz;

    sector[nSector].floorz = spr->z;
    spr->z -= 256;

    ev->nDamage = runlist_CheckRadialDamage(pActor);

    // restore previous values
    sector[nSector].floorz = nFloorZ;
    spr->z += 256;

    if (ev->nDamage <= 0) {
        return;
    }

    // fall through to case 0x80000
    Damage(ev);
}


DExhumedActor* BuildObject(DExhumedActor* pActor, int nOjectType, int nHitag)
{
    auto spr = &pActor->s();

    ChangeActorStat(pActor, ObjectStatnum[nOjectType]);

    // 0x7FFD to ensure set as blocking ('B' and 'H') sprite and also disable translucency and set not invisible
    spr->cstat = (spr->cstat | 0x101) & 0x7FFD;
    spr->xvel = 0;
    spr->yvel = 0;
    spr->zvel = 0;
    spr->extra = -1;
    spr->lotag = runlist_HeadRun() + 1;
    spr->hitag = 0;
    spr->owner = runlist_AddRunRec(spr->lotag - 1, pActor, 0x170000);

    //	GrabTimeSlot(3);
    pActor->nPhase = ObjectList.Push(pActor);
    if (spr->statnum == kStatDestructibleSprite) {
        pActor->nHealth = 4;
    }
    else {
        pActor->nHealth = 120;
    }

    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x170000);

    short nSeq = ObjectSeq[nOjectType];

    if (nSeq > -1)
    {
        pActor->nIndex = SeqOffsets[nSeq];

        if (!nOjectType) // if not Explosion Trigger (e.g. Exploding Fire Cauldron)
        {
            pActor->nFrame = RandomSize(4) % (SeqSize[pActor->nIndex] - 1);
        }

        auto  pActor2 = insertActor(spr->sectnum, 0);
        auto pSprite2 = &pActor2->s();
        pActor->pTarget = pActor2;
        pActor->nIndex2 = -1;

        pSprite2->cstat = 0x8000;
        pSprite2->x = spr->x;
        pSprite2->y = spr->y;
        pSprite2->z = spr->z;
    }
    else
    {
        pActor->nFrame = 0;
        pActor->nIndex = -1;

        if (spr->statnum == kStatDestructibleSprite) {
            pActor->nIndex2 = -1;
        }
        else {
            pActor->nIndex2 = -nHitag;
        }
    }
    spr->backuppos();

    return pActor;
}

// in-game destructable wall mounted screen
void ExplodeScreen(DExhumedActor* pActor)
{
    auto pSprite = &pActor->s();
    pSprite->z -= GetActorHeight(pActor) / 2;

    for (int i = 0; i < 30; i++) {
        BuildSpark(pActor, 0); // shoot out blue orbs
    }

    pSprite->cstat = 0x8000;
    PlayFX2(StaticSound[kSound78], pActor);
}

void AIObject::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    auto pSprite = &pActor->s();
    short nStat = pSprite->statnum;
    int bx = pActor->nIndex;

    if (nStat == 97 || (!(pSprite->cstat & 0x101))) {
        return;
    }

    if (nStat != kStatExplodeTarget) {
        Gravity(pActor);
    }

    // do animation
    if (bx != -1)
    {
        pActor->nFrame++;
        if (pActor->nFrame >= SeqSize[bx]) {
            pActor->nFrame = 0;
        }

        pSprite->picnum = seq_GetSeqPicnum2(bx, pActor->nFrame);
    }

    if (pActor->nHealth >= 0) {
        goto FUNCOBJECT_GOTO;
    }

    pActor->nHealth++;

    if (pActor->nHealth)
    {
    FUNCOBJECT_GOTO:
        if (nStat != kStatExplodeTarget)
        {
            auto nMov = movesprite(pActor, pSprite->xvel << 6, pSprite->yvel << 6, pSprite->zvel, 0, 0, CLIPMASK0);

            if (pSprite->statnum == kStatExplodeTrigger) {
                pSprite->pal = 1;
            }

            if (nMov.exbits & kHitAux2)
            {
                pSprite->xvel -= pSprite->xvel >> 3;
                pSprite->yvel -= pSprite->yvel >> 3;
            }

            if (nMov.type == kHitSprite)
            {
                pSprite->yvel = 0;
                pSprite->xvel = 0;
            }
        }

        return;
    }
    else
    {
        int var_18;

        // red branch
        if ((nStat == kStatExplodeTarget) || (pSprite->z < pSprite->sector()->floorz))
        {
            var_18 = 36;
        }
        else
        {
            var_18 = 34;
        }

        AddFlash(pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 128);
        BuildAnim(nullptr, var_18, 0, pSprite->x, pSprite->y, pSprite->sector()->floorz, pSprite->sectnum, 240, 4);

        //				int edi = nSprite | 0x4000;

        if (nStat == kStatExplodeTrigger)
        {
            for (int i = 4; i < 8; i++) {
                BuildCreatureChunk(pActor, seq_GetSeqPicnum(kSeqFirePot, (i >> 2) + 1, 0), true);
            }

            runlist_RadialDamageEnemy(pActor, 200, 20);
        }
        else if (nStat == kStatExplodeTarget)
        {
            for (int i = 0; i < 8; i++) {
                BuildCreatureChunk(pActor, seq_GetSeqPicnum(kSeqFirePot, (i >> 1) + 3, 0), true);
            }
        }

        if (!(currentLevel->gameflags & LEVEL_EX_MULTI) || nStat != kStatExplodeTrigger)
        {
            runlist_SubRunRec(pSprite->owner);
            runlist_SubRunRec(pActor->nRun);

            DeleteActor(pActor);
            return;
        }
        else
        {
            StartRegenerate(pActor);
            pActor->nHealth = 120;

            auto pTargSpr = &pActor->pTarget->s();
            pSprite->x = pTargSpr->x;
            pSprite->y = pTargSpr->y;
            pSprite->z = pTargSpr->z;

            ChangeActorSect(pActor, pTargSpr->sectnum);
            return;
        }
    }
}

void AIObject::Damage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    auto pSprite = &pActor->s();
    int nStat = pSprite->statnum;

    if (nStat >= 150 || pActor->nHealth <= 0) {
        return;
    }

    if (nStat == 98)
    {
        D3PlayFX((StaticSound[kSound47] | 0x2000) | (RandomSize(2) << 9), pActor);
        return;
    }

    pActor->nHealth -= (short)ev->nDamage;
    if (pActor->nHealth > 0) {
        return;
    }

    if (nStat == kStatDestructibleSprite)
    {
        ExplodeScreen(pActor);
    }
    else
    {
        pActor->nHealth = -(RandomSize(3) + 1);
    }
}

void AIObject::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    int bx = pActor->nIndex;

    if (bx > -1)
    {
        seq_PlotSequence(ev->nParam, bx, pActor->nFrame, 1);
    }
    return;
}

void AIObject::RadialDamage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    auto pSprite = &pActor->s();
    short nStat = pSprite->statnum;

    if (pActor->nHealth > 0 && pSprite->cstat & 0x101
        && (nStat != kStatExplodeTarget
            || ev->pRadialActor->s().statnum == 201
            || (nRadialBullet != 3 && nRadialBullet > -1)
            || ev->pRadialActor->s().statnum == kStatExplodeTrigger))
    {
        int nDamage = runlist_CheckRadialDamage(pActor);
        if (nDamage <= 0) {
            return;
        }

        if (pSprite->statnum != kStatAnubisDrum) {
            pActor->nHealth -= nDamage;
        }

        if (pSprite->statnum == kStatExplodeTarget)
        {
            pSprite->xvel = 0;
            pSprite->yvel = 0;
            pSprite->zvel = 0;
        }
        else if (pSprite->statnum != kStatAnubisDrum)
        {
            pSprite->xvel >>= 1;
            pSprite->yvel >>= 1;
            pSprite->zvel >>= 1;
        }

        if (pActor->nHealth > 0) {
            return;
        }

        if (pSprite->statnum == kStatExplodeTarget)
        {
            pActor->nHealth = -1;
            short ax = pActor->nIndex2;

            if (ax < 0 || ObjectList[ax]->nHealth <= 0) {
                return;
            }

            ObjectList[ax]->nHealth = -1;
        }
        else if (pSprite->statnum == kStatDestructibleSprite)
        {
            pActor->nHealth = 0;

            ExplodeScreen(pActor);
        }
        else
        {
            pActor->nHealth = -(RandomSize(4) + 1);
        }
    }
}

void BuildDrip(DExhumedActor* pActor)
{
    auto pSprite = &pActor->s();
    auto nDrips = sDrip.Reserve(1);
    sDrip[nDrips].pActor = pActor;
    sDrip[nDrips].nCount = RandomSize(8) + 90;
    pSprite->cstat = 0x8000u;
}

void DoDrips()
{
    for (unsigned i = 0; i < sDrip.Size(); i++)
    {
        sDrip[i].nCount--;
        if (sDrip[i].nCount <= 0)
        {
            auto pActor = sDrip[i].pActor;
            auto pSprite = &pActor->s();

            short nSeqOffset = SeqOffsets[kSeqDrips];

            if (!(SectFlag[pSprite->sectnum] & kSectLava)) {
                nSeqOffset++;
            }

            seq_MoveSequence(pActor, nSeqOffset, RandomSize(2) % SeqSize[nSeqOffset]);

            sDrip[i].nCount = RandomSize(8) + 90;
        }
    }

    for (unsigned i = 0; i < sBob.Size(); i++)
    {
        sBob[i].field_2 += 4;

        int edx = bsin(sBob[i].field_2 << 3, -4);
        int nSector =sBob[i].nSector;

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

void SnapBobs(int nSectorA, int nSectorB)
{
    int ecx = -1;
    int ebx = ecx;
    //	int var_14 = nSector;
    //	int edi = edx;

    for (unsigned i = 0; i < sBob.Size(); i++)
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
    auto nBobs = sBob.Reserve(1);
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
    sBob[nBobs].sBobID = nHitag;

    sBob[nBobs].nSector = nSector;
    StartInterpolation(nSector, bx == 0 ? Interp_Sect_Floorz : Interp_Sect_Ceilingz);

    SectFlag[nSector] |= 0x0010;
}

int FindTrail(int nVal)
{
    for (unsigned i = 0; i < sTrail.Size(); i++)
    {
        if (sTrail[i].field_2 == nVal)
            return i;
    }

    auto nTrails = sTrail.Reserve(1);
    sTrail[nTrails].field_2 = nVal;
    sTrail[nTrails].field_0 = -1;
    sTrail[nTrails].field_4 = -1;
    return nTrails;
}

// ok ?
void ProcessTrailSprite(DExhumedActor* pActor, int nLotag, int nHitag)
{
    auto pSprite = &pActor->s();
    auto nPoint = sTrailPoint.Reserve(1);

    sTrailPoint[nPoint].x = pSprite->x;
    sTrailPoint[nPoint].y = pSprite->y;

    int nTrail = FindTrail(nHitag);

    int var_14 = nLotag - 900;

    sTrailPoint[nPoint].nTrailPointVal = var_14;

    int field0 = sTrail[nTrail].field_0;

    if (field0 == -1)
    {
        sTrail[nTrail].field_0 = nPoint;
        sTrail[nTrail].field_4 = nPoint;

        sTrailPoint[nPoint].nTrailPointNext = -1;
        sTrailPoint[nPoint].nTrailPointPrev = -1;
    }
    else
    {
        int ecx = -1;

        while (field0 != -1)
        {
            if (sTrailPoint[field0].nTrailPointVal > var_14)
            {
                sTrailPoint[nPoint].nTrailPointPrev = sTrailPoint[field0].nTrailPointPrev;
                sTrailPoint[field0].nTrailPointPrev = nPoint;
                sTrailPoint[nPoint].nTrailPointNext = field0;

                if (field0 == sTrail[nTrail].field_0) {
                    sTrail[nTrail].field_0 = nPoint;
                }

                break;
            }

            ecx = field0;
            field0 = sTrailPoint[field0].nTrailPointNext;
        }

        if (field0 == -1)
        {
            sTrailPoint[ecx].nTrailPointNext = nPoint;
            sTrailPoint[nPoint].nTrailPointPrev = ecx;
            sTrailPoint[nPoint].nTrailPointNext = -1;
            sTrail[nTrail].field_4 = nPoint;
        }
    }

    DeleteActor(pActor);
}

// ok?
void AddMovingSector(int nSector, int edx, int ebx, int ecx)
{
    CreatePushBlock(nSector);
    setsectinterpolate(nSector);

    short nTrail = FindTrail(ebx);


    auto nMoveSects = sMoveSect.Reserve(1);
    MoveSect* pMoveSect = &sMoveSect[nMoveSects];

    pMoveSect->sMoveDir = 1;
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
    for (unsigned i = 0; i < sMoveSect.Size(); i++)
    {
        if (sMoveSect[i].nSector == -1) {
            continue;
        }

        if (sMoveSect[i].field_14 != -1 && !sRunChannels[sMoveSect[i].field_14].c) {
            continue;
        }

        int nSector =sMoveSect[i].nSector;
        short nBlock = sector[nSector].extra;

        BlockInfo* pBlockInfo = &sBlockInfo[nBlock];

        if (sMoveSect[i].nTrailPoint == -1)
        {
            if (sMoveSect[i].field_6 & 0x20)
            {
                runlist_ChangeChannel(sMoveSect[i].field_14, 0);
            }

            short ax;

            if (sMoveSect[i].field_6 & 0x10)
            {
                sMoveSect[i].sMoveDir = -sMoveSect[i].sMoveDir;
                if (sMoveSect[i].sMoveDir > 0)
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

        int nXVel = bcos(nAngle, 4) * sMoveSect[i].field_10;
        int nYVel = bsin(nAngle, 4) * sMoveSect[i].field_10;

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

                if (sMoveSect[i].sMoveDir > 0)
                {
                    sMoveSect[i].nTrailPoint = sTrailPoint[sMoveSect[i].nTrailPoint].nTrailPointNext;
                }
                else
                {
                    sMoveSect[i].nTrailPoint = sTrailPoint[sMoveSect[i].nTrailPoint].nTrailPointPrev;
                }
            }
        }
        else
        {
            // repeat of code from loc_23908
            nYVel = ecx;
            nXVel = ebx;

            if (sMoveSect[i].sMoveDir > 0)
            {
                sMoveSect[i].nTrailPoint = sTrailPoint[sMoveSect[i].nTrailPoint].nTrailPointNext;
            }
            else
            {
                sMoveSect[i].nTrailPoint = sTrailPoint[sMoveSect[i].nTrailPoint].nTrailPointPrev;
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

    for (unsigned i = 0; i < sMoveSect.Size(); i++)
    {
        int nTrail = sMoveSect[i].nTrail;
        sMoveSect[i].nTrailPoint = sTrail[nTrail].field_0;

        if (sMoveSect[i].field_6 & 0x40) {
            runlist_ChangeChannel(sMoveSect[i].field_14, 1);
        }

        int nSector =sMoveSect[i].nSector;

        if (SectFlag[nSector] & kSectUnderwater)
        {
            sector[nSector].ceilingstat |= 0x40;
            sector[nSector].floorstat &= 0xBFFF;

            for (unsigned j = 0; j < sMoveSect.Size(); j++)
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

    for (unsigned i = 0; i < sBob.Size(); i++)
    {
        if (sBob[i].field_3 == 0)
        {
            int bobID = sBob[i].sBobID;

            for (unsigned j = 0; j < sBob.Size(); j++)
            {
                if (j != i)
                {
                    if (sBob[i].field_3 != 0 && sBob[j].sBobID == bobID) {
                        SnapSectors(i, j, 0);
                    }
                }
            }
        }
    }

    if (!(currentLevel->gameflags & LEVEL_EX_COUNTDOWN))
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
						int xVal = abs(sector[i].firstWall()->x - sector[j].firstWall()->x);
						int yVal = abs(sector[i].firstWall()->y - sector[j].firstWall()->y);

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
                    auto pActor = insertActor(i, 407);
                    pActor->s().cstat = 0x8000;
                }

                nWall++;
            }
        }

        ExhumedLinearSpriteIterator it;
        while (auto act = it.Next())
        {
            auto spr = &act->s();
            if (spr->statnum < kMaxStatus && spr->picnum == kTile3603)
            {
                ChangeActorStat(act, 407);
                spr->pal = 1;
            }
        }
    }

    for (unsigned i = 0; i < ObjectList.Size(); i++)
    {
        auto pObjectActor = ObjectList[i];

        if (pObjectActor->s().statnum == kStatExplodeTarget)
        {
            if (!pObjectActor->nIndex2) {
                pObjectActor->nIndex2 = -1;
            }
            else
            {
                int edi = pObjectActor->nIndex2;
                pObjectActor->nIndex2 = -1;

                for (unsigned j = 0; j < ObjectList.Size(); j++)
                {

                    if (i != j && ObjectList[j]->s().statnum == kStatExplodeTarget && edi == ObjectList[j]->nIndex2)
                    {
                        pObjectActor->nIndex2 = j;
                        ObjectList[j]->nIndex2 = i;
                    }
                }
            }
        }
    }
}

END_PS_NS

