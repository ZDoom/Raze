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

static const int8_t ObjectSeq[] = {
    46, -1, 72, -1
};

static const int16_t ObjectStatnum[] = {
    kStatExplodeTrigger, kStatExplodeTarget, 98, kStatDestructibleSprite
};

struct Trail
{
    int16_t nPoint;
    int16_t nVal;
    int16_t nPoint2;
};

struct TrailPoint
{
    int x;
    int y;
    uint8_t nTrailPointVal;
    int16_t nTrailPointPrev;
    int16_t nTrailPointNext;

};

struct Bob
{
    sectortype* pSector;
    int z;
    uint8_t nPhase;
    uint8_t field_3;
    uint16_t sBobID;
};

struct Drip
{
    TObjPtr<DExhumedActor*> pActor;
    int16_t nCount;
};

// 56 bytes
struct Elev
{
    TObjPtr<DExhumedActor*> pActor;
    sectortype* pSector;
    int16_t nFlags;
    int16_t nChannel;
    int nParam1;
    int nParam2;
    int16_t nCountZOffsets; // count of items in zOffsets
    int16_t nCurZOffset;
    int zOffsets[8]; // different Z offsets
    int16_t nRunRec;
};

// 16 bytes
struct MoveSect
{
    sectortype* pSector;
    sectortype* pCurSector;
    int field_10;
    int16_t nTrail;
    int16_t nTrailPoint;
    int16_t nFlags;
    int16_t nChannel; // nChannel?
    int16_t sMoveDir;

};

struct wallFace
{
    walltype* pWall;
    int16_t nChannel;
    int16_t count;
    int16_t piclist[8];
};

struct slideData
{
    TObjPtr<DExhumedActor*> pActor;
    int16_t nChannel;
    int16_t nStart;
    int16_t nRunRec;
    int16_t nRunC;

    walltype* pStartWall;
    walltype* pWall1;
    walltype* pWall2;
    walltype* pWall3;
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
    sectortype* pSector;
    int16_t nNext;
};

struct Trap
{
    TObjPtr<DExhumedActor*> pActor;
    walltype* pWall1;
    walltype* pWall2;

    int16_t nState;
    int16_t nType;
    int16_t nPicnum1;
    int16_t nPicnum2;
    int16_t nTrapInterval;

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
TObjPtr<DExhumedActor*> pFinaleSpr;

size_t MarkObjects()
{
    GC::Mark(pFinaleSpr);
    for (auto& d : sDrip) GC::Mark(d.pActor);
    for (auto& d : sTrap) GC::Mark(d.pActor);
    for (auto& d : Elevator) GC::Mark(d.pActor);
    for (auto& d : SlideData) GC::Mark(d.pActor);
    GC::MarkArray(EnergyBlocks);
    return 1 + sDrip.Size() + sTrap.Size() + Elevator.Size() + SlideData.Size();
}

int lFinaleStart;

int nFinaleStage;

int nDronePitch = 0;
int nSmokeSparks = 0;

FSerializer& Serialize(FSerializer& arc, const char* keyname, Trail& w, Trail* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at0", w.nPoint)
            ("at2", w.nVal)
            ("at4", w.nPoint2)
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
        arc("sector", w.pSector)
            ("at2", w.nPhase)
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
        arc("at0", w.nFlags)
            ("channel", w.nChannel)
            ("sector", w.pSector)
            ("at6", w.nParam1)
            ("ata", w.nParam2)
            ("countz", w.nCountZOffsets)
            ("curz", w.nCurZOffset)
            .Array("zofs", w.zOffsets, 8)
            ("at32", w.nRunRec)
            ("sprite", w.pActor)
            .EndObject();
    }
    return arc;
}
FSerializer& Serialize(FSerializer& arc, const char* keyname, MoveSect& w, MoveSect* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("sector", w.pSector)
            ("trail", w.nTrail)
            ("trailpoint", w.nTrailPoint)
            ("at6", w.nFlags)
            ("at8", w.pCurSector)
            ("at10", w.field_10)
            ("at14", w.nChannel)
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
            ("wall", w.pWall)
            ("at4", w.count)
            .Array("at6", w.piclist, 8)
            .EndObject();
    }
    return arc;
}
FSerializer& Serialize(FSerializer& arc, const char* keyname, slideData& w, slideData* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at0", w.pStartWall)
            ("at4", w.pWall1)
            ("at8", w.pWall2)
            ("atc", w.pWall3)
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
            ("at4a", w.nRunRec)
            ("at6a", w.pActor)
            ("at8a", w.nRunC)
            .EndObject();
    }
    return arc;
}
FSerializer& Serialize(FSerializer& arc, const char* keyname, Point& w, Point* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at0", w.pSector)
            ("ate", w.nNext)
            .EndObject();
    }
    return arc;
}
FSerializer& Serialize(FSerializer& arc, const char* keyname, Trap& w, Trap* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at0", w.nState)
            ("sprite", w.pActor)
            ("type", w.nType)
            ("wall1", w.pWall1)
            ("wall2", w.pWall2)
            ("pic1", w.nPicnum1)
            ("pic2", w.nPicnum2)
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
DExhumedActor* BuildWallSprite(sectortype* pSector)
{
    auto wal = pSector->firstWall();

    auto pActor = insertActor(pSector, 401);

	pActor->spr.pos = DVector3(wal->fcenter(), (pSector->floorz + pSector->ceilingz) * 0.5);
    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;

    return pActor;
 }

// done
DExhumedActor* FindWallSprites(sectortype* pSector)
{
    int var_24 = 0x7FFFFFFF;
    int ecx = 0x7FFFFFFF;

    int esi = 0x80000002;
    int edi = 0x80000002;

	for (auto& wal : wallsofsector(pSector))
    {
        if (wal.wall_int_pos().X < var_24) {
            var_24 = wal.wall_int_pos().X;
        }

        if (esi < wal.wall_int_pos().X) {
            esi = wal.wall_int_pos().X;
        }

        if (ecx > wal.wall_int_pos().Y) {
            ecx = wal.wall_int_pos().Y;
        }

        if (edi < wal.wall_int_pos().Y) {
            edi = wal.wall_int_pos().Y;
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
        if (actor->spr.lotag == 0)
        {
            if ((actor->spr.cstat & (CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_ONE_SIDE)) == (CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_ONE_SIDE))
            {
                int var_28 = actor->int_pos().X;
                int ebx = actor->int_pos().Y;

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
        pAct = insertActor(pSector, 401);

        pAct->set_int_pos({ (var_24 + esi) / 2, (ecx + edi) / 2, pSector->int_floorz() });
        pAct->spr.cstat = CSTAT_SPRITE_INVISIBLE;
        pAct->spr.intowner = -1;
        pAct->spr.lotag = 0;
        pAct->spr.hitag = 0;
    }

    return pAct;
}

int BuildElevF(int nChannel, sectortype* pSector, DExhumedActor* nWallSprite, int arg_4, int arg_5, int nCount, ...)
{
    auto ElevCount = Elevator.Reserve(1);

    Elevator[ElevCount].nFlags = 2;
    Elevator[ElevCount].nParam1 = arg_4;
    Elevator[ElevCount].nRunRec = -1;
    Elevator[ElevCount].nParam2 = arg_5;
    Elevator[ElevCount].nChannel = nChannel;
    Elevator[ElevCount].pSector = pSector;
    Elevator[ElevCount].nCountZOffsets = 0;
    Elevator[ElevCount].nCurZOffset = 0;

    if (nWallSprite == nullptr) {
        nWallSprite = BuildWallSprite(pSector);
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

int BuildElevC(int arg1, int nChannel, sectortype* pSector, DExhumedActor* nWallSprite, int arg5, int arg6, int nCount, ...)
{
    int edi = arg5;

    auto ElevCount = Elevator.Reserve(1);

    Elevator[ElevCount].nFlags = arg1;

    if (arg1 & 4)
    {
        edi = arg5 / 2;
    }

    Elevator[ElevCount].nParam1 = edi;
    Elevator[ElevCount].nCountZOffsets = 0;
    Elevator[ElevCount].nCurZOffset = 0;
    Elevator[ElevCount].nParam2 = arg6;
    Elevator[ElevCount].nRunRec = -1;
    Elevator[ElevCount].nChannel = nChannel;
    Elevator[ElevCount].pSector = pSector;

    if (nWallSprite == nullptr) {
        nWallSprite = BuildWallSprite(pSector);
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
int CheckSectorSprites(sectortype* pSector, int nVal)
{
    int b = 0;

    if (nVal)
    {
        int nZDiff = pSector->int_floorz() - pSector->int_ceilingz();

        ExhumedSectIterator it(pSector);
        while (auto pActor= it.Next())
        {
            if ((pActor->spr.cstat & CSTAT_SPRITE_BLOCK_ALL) && (nZDiff < GetActorHeight(pActor)))
            {
                if (nVal != 1) {
                    return 1;
                }

                b = 1;

                runlist_DamageEnemy(pActor, nullptr, 5);

                if (pActor->spr.statnum == 100 && PlayerList[GetPlayerFromActor(pActor)].nHealth <= 0)
                {
                    PlayFXAtXYZ(StaticSound[kSoundJonFDie],
                        pActor->spr.pos,
                        CHANF_NONE, 0x4000);
                }
            }
        }
    }
    else
    {
        ExhumedSectIterator it(pSector);
        while (auto pActor = it.Next())
        {
            if (pActor->spr.cstat & CSTAT_SPRITE_BLOCK_ALL) {
                return 1;
            }
        }
        b = 0;
    }

    return b;
}

// done
void MoveSectorSprites(sectortype* pSector, double z)
{
    double newz = pSector->floorz;
    double oldz = newz - z;
    double minz = min(newz, oldz);
    double maxz = max(newz, oldz);
    ExhumedSectIterator it(pSector);
    while (auto pActor = it.Next())
    {
        int actz = pActor->spr.pos.Z;
        if ((pActor->spr.statnum != 200 && actz >= minz && actz <= maxz) || pActor->spr.statnum >= 900)
        {
			pActor->spr.pos.Z = newz;
        }
    }
}

void StartElevSound(DExhumedActor* pActor, int nVal)
{
    int nSound;

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
    int nElev = RunData[nRun].nObjIndex;
    assert(nElev >= 0 && nElev < (int)Elevator.Size());

    int nChannel = Elevator[nElev].nChannel;
    int nFlags = Elevator[nElev].nFlags;

    assert(nChannel >= 0 && nChannel < kMaxChannels);

    //			int ax = var_18 & 8;
    int dx = sRunChannels[nChannel].c;

    int edi = 999; // FIXME CHECKME - this isn't default set to anything in the ASM that I can see - if ax is 0 and var_24 is 0, this will never be set to a known value otherwise!

    if (nFlags & 0x8)
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
        if (nFlags & 0x10) // was var_24
        {
            if (Elevator[nElev].nRunRec < 0)
            {
                Elevator[nElev].nRunRec = runlist_AddRunRec(NewRun, &RunData[nRun]);
                StartElevSound(Elevator[nElev].pActor, nFlags);

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
        if (Elevator[nElev].nRunRec < 0)
        {
            Elevator[nElev].nRunRec = runlist_AddRunRec(NewRun, &RunData[nRun]);

            StartElevSound(Elevator[nElev].pActor, nFlags);
        }
    }
    else
    {
        //loc_20E4E:
        if (Elevator[nElev].nRunRec >= 0)
        {
            runlist_SubRunRec(Elevator[nElev].nRunRec);
            Elevator[nElev].nRunRec = -1;
        }
    }
}

void AIElev::Tick(RunListEvent* ev)
{
    int nRun = ev->nRun;
    int nElev = RunData[nRun].nObjIndex;
    assert(nElev >= 0 && nElev < (int)Elevator.Size());

    int nChannel = Elevator[nElev].nChannel;
    int var_18 = Elevator[nElev].nFlags;

    assert(nChannel >= 0 && nChannel < kMaxChannels);

    auto pSector =Elevator[nElev].pSector;
    DExhumedActor* pElevSpr = Elevator[nElev].pActor;

    int ebp = 0; // initialise to *something*

    if (var_18 & 0x2)
    {
        int nZOffset = Elevator[nElev].nCurZOffset;
        int nZVal = Elevator[nElev].zOffsets[nZOffset];

        StartInterpolation(pSector, Interp_Sect_Floorz);
        int fz = pSector->int_floorz();
        int nVal = LongSeek(&fz, nZVal, Elevator[nElev].nParam1, Elevator[nElev].nParam2);
        pSector->set_int_floorz(fz);
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
                Elevator[nElev].nRunRec = -1;
                runlist_ReadyChannel(nChannel);

                D3PlayFX(StaticSound[nStopSound], Elevator[nElev].pActor);
            }
        }
        else
        {
            MoveSectorSprites(pSector, nVal * inttoworld);

            if (nVal < 0 && CheckSectorSprites(pSector, 2))
            {
                runlist_ChangeChannel(nChannel, sRunChannels[nChannel].c == 0);
                return;
            }
        }
    }
    else
    {
        // loc_20FC3:
        int ceilZ = pSector->int_ceilingz();

        int nZOffset = Elevator[nElev].nCurZOffset;
        int zVal = Elevator[nElev].zOffsets[nZOffset];

        StartInterpolation(pSector, Interp_Sect_Ceilingz);
        int nVal = LongSeek(&ceilZ, zVal, Elevator[nElev].nParam1, Elevator[nElev].nParam2);
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
                Elevator[nElev].nRunRec = -1;
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

                PlayFXAtXYZ(StaticSound[kSound26], pElevSpr->spr.pos);
            }

            if (var_18 & 0x4)
            {
                if (CheckSectorSprites(pSector, 1)) {
                    return;
                }
            }
            else
            {
                if (CheckSectorSprites(pSector, 0))
                {
                    runlist_ChangeChannel(nChannel, sRunChannels[nChannel].c == 0);
                    return;
                }
            }
        }

        StartInterpolation(pSector, Interp_Sect_Ceilingz);
        pSector->set_int_ceilingz(ceilZ);
    }

    // maybe this doesn't go here?
    while (pElevSpr)
    {
        pElevSpr->add_int_z(ebp);
        pElevSpr = pElevSpr->pTarget;
    }
}


// done
void InitWallFace()
{
    WallFace.Clear();
}

int BuildWallFace(int nChannel, walltype* pWall, int nCount, ...)
{
    auto WallFaceCount = WallFace.Reserve(1);

    WallFace[WallFaceCount].count = 0;
    WallFace[WallFaceCount].pWall = pWall;
    WallFace[WallFaceCount].nChannel = nChannel;

    if (nCount > 8) {
        nCount = 8;
    }

    va_list piclist;
    va_start(piclist, nCount);

    while (WallFace[WallFaceCount].count < nCount)
    {
        int i = WallFace[WallFaceCount].count;
        WallFace[WallFaceCount].count++;

        WallFace[WallFaceCount].piclist[i] = (int16_t)va_arg(piclist, int);
    }
    va_end(piclist);

    return WallFaceCount;
}

void AIWallFace::ProcessChannel(RunListEvent* ev)
{
    int nWallFace = RunData[ev->nRun].nObjIndex;
    assert(nWallFace >= 0 && nWallFace < (int)WallFace.Size());

    int16_t nChannel = WallFace[nWallFace].nChannel;

    int16_t si = sRunChannels[nChannel].c;

    if ((si <= WallFace[nWallFace].count) && (si >= 0))
    {
        WallFace[nWallFace].pWall->picnum = WallFace[nWallFace].piclist[si];
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

int BuildSlide(int nChannel, walltype* pStartWall, walltype* pWall1, walltype* p2ndLastWall, walltype* pWall2, walltype* pWall3, walltype* pWall4)
{
    auto nSlide = SlideData.Reserve(1);

    auto pSector = pStartWall->sectorp();

    SlideData[nSlide].nRunRec = -1;
    SlideData[nSlide].nChannel = nChannel;
    SlideData[nSlide].nStart = -1;

    int nPoint = GrabPoint();

    SlideData[nSlide].nStart = nPoint;

    PointList[nPoint].nNext = -1;
    PointList[nPoint].pSector = pSector;

    for(auto& wal : wallsofsector(pSector))
    {
        int ax = SlideData[nSlide].nStart;

        if (ax >= 0)
        {
            while (ax >= 0)
            {
                if (wal.nextSector() == PointList[ax].pSector) {
                    break;
                }

                ax = PointList[ax].nNext;
            }
        }
        else
        {
            if (wal.twoSided())
            {
                nPoint = GrabPoint();

                PointList[nPoint].nNext = SlideData[nSlide].nStart;
                PointList[nPoint].pSector = wal.nextSector();

                SlideData[nSlide].nStart = nPoint;
            }
        }
    }

    SlideData[nSlide].pStartWall = pStartWall;
    SlideData[nSlide].pWall1 = pWall1;
    SlideData[nSlide].pWall2 = pWall2;
    SlideData[nSlide].pWall3 = pWall3;

    SlideData[nSlide].x1 = pStartWall->wall_int_pos().X;
    SlideData[nSlide].y1 = pStartWall->wall_int_pos().Y;

    SlideData[nSlide].x2 = pWall2->wall_int_pos().X;
    SlideData[nSlide].y2 = pWall2->wall_int_pos().Y;

    SlideData[nSlide].x3 = pWall1->wall_int_pos().X;
    SlideData[nSlide].y3 = pWall1->wall_int_pos().Y;

    SlideData[nSlide].x4 = pWall3->wall_int_pos().X;
    SlideData[nSlide].y4 = pWall3->wall_int_pos().Y;

    SlideData[nSlide].x5 = p2ndLastWall->wall_int_pos().X;
    SlideData[nSlide].y5 = p2ndLastWall->wall_int_pos().Y;

    SlideData[nSlide].x6 = pWall4->wall_int_pos().X;
    SlideData[nSlide].y6 = pWall4->wall_int_pos().Y;

    StartInterpolation(pStartWall, Interp_Wall_X);
    StartInterpolation(pStartWall, Interp_Wall_Y);

    StartInterpolation(pWall1, Interp_Wall_X);
    StartInterpolation(pWall1, Interp_Wall_Y);

    StartInterpolation(pWall2, Interp_Wall_X);
    StartInterpolation(pWall2, Interp_Wall_Y);

    StartInterpolation(pWall3, Interp_Wall_X);
    StartInterpolation(pWall3, Interp_Wall_Y);


    auto pActor = insertActor(pSector, 899);

    SlideData[nSlide].pActor = pActor;
    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    pActor->spr.pos = DVector3(pStartWall->pos, pSector->floorz);
    pActor->backuppos();

    SlideData[nSlide].nRunC = 0;

    return nSlide;
}

void AISlide::ProcessChannel(RunListEvent* ev)
{
    int nRun = ev->nRun;
    int nSlide = RunData[nRun].nObjIndex;
    assert(nSlide >= 0 && nSlide < (int)SlideData.Size());

    int nChannel = SlideData[nSlide].nChannel;

    if (SlideData[nSlide].nRunRec >= 0)
    {
        runlist_SubRunRec(SlideData[nSlide].nRunRec);
        SlideData[nSlide].nRunRec = -1;
    }

    if (sRunChannels[nChannel].c && sRunChannels[nChannel].c != 1) {
        return;
    }

    SlideData[nSlide].nRunRec = runlist_AddRunRec(NewRun, &RunData[nRun]);

    if (SlideData[nSlide].nRunC != sRunChannels[nChannel].c)
    {
        D3PlayFX(StaticSound[kSound23], SlideData[nSlide].pActor);
        SlideData[nSlide].nRunC = sRunChannels[nChannel].c;
    }
}

void AISlide::Tick(RunListEvent* ev)
{
    int nRun = ev->nRun;
    int nSlide = RunData[nRun].nObjIndex;
    assert(nSlide >= 0 && nSlide < (int)SlideData.Size());

    int nChannel = SlideData[nSlide].nChannel;
    int ebp = 0;

    int cx = sRunChannels[nChannel].c;

    int clipmask = ebp + 1; // RENAME

    if (cx == 1)
    {
        auto pWall = SlideData[nSlide].pWall1;
        int x = pWall->wall_int_pos().X;
        int y = pWall->wall_int_pos().Y;

        int nSeekA = LongSeek(&x, SlideData[nSlide].x5, 20, 20);
        int var_34 = nSeekA;
        int var_20 = nSeekA;

        int nSeekB = LongSeek(&y, SlideData[nSlide].y5, 20, 20);
        int var_2C = nSeekB;
        int var_24 = nSeekB;

        dragpoint(SlideData[nSlide].pWall1, x, y);
        movesprite(SlideData[nSlide].pActor, var_34 << 14, var_2C << 14, 0, 0, 0, CLIPMASK1);

        if (var_34 == 0)
        {
            if (var_2C == 0)
            {
                ebp = clipmask;
            }
        }

        pWall = SlideData[nSlide].pStartWall;

        y = pWall->wall_int_pos().Y + var_24;
        x = pWall->wall_int_pos().X + var_20;

        dragpoint(SlideData[nSlide].pStartWall, x, y);

        pWall = SlideData[nSlide].pWall3;

        x = pWall->wall_int_pos().X;
        y = pWall->wall_int_pos().Y;

        int nSeekC = LongSeek(&x, SlideData[nSlide].x6, 20, 20);
        int var_30 = nSeekC;
        var_20 = nSeekC;

        int nSeekD = LongSeek(&y, SlideData[nSlide].y6, 20, 20);
        int edi = nSeekD;
        var_24 = nSeekD;

        dragpoint(SlideData[nSlide].pWall3, x, y);

        if (var_30 == 0 && edi == 0) {
            ebp++;
        }

        pWall = SlideData[nSlide].pWall2;

        x = pWall->wall_int_pos().X + var_20;
        y = pWall->wall_int_pos().Y + var_24;

        dragpoint(SlideData[nSlide].pWall2, x, y);
    }
    else if (cx == 0) // right branch
    {
        auto pWall = SlideData[nSlide].pStartWall;
        int x = pWall->wall_int_pos().X;
        int y = pWall->wall_int_pos().Y;

        int nSeekA = LongSeek(&x, SlideData[nSlide].x1, 20, 20);
        int edi = nSeekA;
        int var_1C = nSeekA;

        int nSeekB = LongSeek(&y, SlideData[nSlide].y1, 20, 20);
        int ecx = nSeekB;
        int var_28 = nSeekB;

        dragpoint(SlideData[nSlide].pStartWall, x, y);

        if (edi == 0 && ecx == 0) {
            ebp = clipmask;
        }

        pWall = SlideData[nSlide].pWall1;

        y = pWall->wall_int_pos().Y + var_28;
        x = pWall->wall_int_pos().X + var_1C;

        dragpoint(SlideData[nSlide].pWall1, x, y);

        pWall = SlideData[nSlide].pWall2;

        x = pWall->wall_int_pos().X;
        y = pWall->wall_int_pos().Y;

        int nSeekC = LongSeek(&x, SlideData[nSlide].x2, 20, 20);
        edi = nSeekC;
        var_1C = nSeekC;

        int nSeekD = LongSeek(&y, SlideData[nSlide].y2, 20, 20);
        ecx = nSeekD;
        var_28 = nSeekD;

        dragpoint(SlideData[nSlide].pWall2, x, y);

        if (edi == 0 && ecx == 0) {
            ebp++;
        }

        pWall = SlideData[nSlide].pWall3;

        y = pWall->wall_int_pos().Y + var_28;
        x = pWall->wall_int_pos().X + var_1C;

        dragpoint(SlideData[nSlide].pWall3, x, y);
    }

    // loc_21A51:
    if (ebp >= 2)
    {
        runlist_SubRunRec(SlideData[nSlide].nRunRec);

        SlideData[nSlide].nRunRec = -1;
        D3PlayFX(StaticSound[nStopSound], SlideData[nSlide].pActor);

        runlist_ReadyChannel(nChannel);
    }
}

int BuildTrap(DExhumedActor* pActor, int edx, int ebx, int ecx)
{
    int var_14 = edx;
    int var_18 = ebx;
    int var_10 = ecx;

    int nTrap = sTrap.Reserve(1);
    sTrap[nTrap] = {};

    ChangeActorStat(pActor, 0);

    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    pActor->spr.xvel = 0;
    pActor->spr.yvel = 0;
    pActor->spr.zvel = 0;
    pActor->spr.extra = -1;

    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.hitag = runlist_AddRunRec(NewRun, nTrap, 0x1F0000);
    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, nTrap, 0x1F0000);

    //	GrabTimeSlot(3);

    sTrap[nTrap].pActor = pActor;
    sTrap[nTrap].nType = (var_14 == 0) + 14;
    sTrap[nTrap].nState = -1;

    sTrap[nTrap].nTrapInterval = 64 - (2 * var_10);
    if (sTrap[nTrap].nTrapInterval < 5) {
        sTrap[nTrap].nTrapInterval = 5;
    }

    sTrap[nTrap].nPicnum2 = 0;
    sTrap[nTrap].nPicnum1 = 0;

    if (var_18 == -1) {
        return nTrap;
    }

    auto pSector = pActor->sector();

    for(auto& wal : wallsofsector(pSector))
    {
        if (var_18 == wal.hitag)
        {
            if (sTrap[nTrap].pWall1 != nullptr)
            {
                sTrap[nTrap].pWall2 = &wal;
                sTrap[nTrap].nPicnum2 = wal.picnum;
                break;
            }
            else
            {
                sTrap[nTrap].pWall1 = &wal;
                sTrap[nTrap].nPicnum1 = wal.picnum;
            }
        }
    }
    pActor->backuppos();
    return nTrap;
}

void AITrap::ProcessChannel(RunListEvent* ev)
{
    int nChannel = ev->nParam & 0x3FFF;
    int nTrap = RunData[ev->nRun].nObjIndex;

    if (sRunChannels[nChannel].c > 0)
    {
        sTrap[nTrap].nState = 12;
    }
    else
    {
        sTrap[nTrap].nState = -1;
    }
}

void AITrap::Tick(RunListEvent* ev)
{
    int nTrap = RunData[ev->nRun].nObjIndex;
    DExhumedActor* pActor = sTrap[nTrap].pActor;
    if (!pActor) return;

    if (sTrap[nTrap].nState >= 0)
    {
        sTrap[nTrap].nState--;
        if (sTrap[nTrap].nState > 10) {
            return;
        }

        int nType = sTrap[nTrap].nType;

        if (sTrap[nTrap].nState == 0)
        {
            sTrap[nTrap].nState = sTrap[nTrap].nTrapInterval;

            if (nType == 14)
            {
                auto pWall = sTrap[nTrap].pWall1;
                if (pWall)
                {
                    pWall->picnum = sTrap[nTrap].nPicnum1;
                }

                pWall = sTrap[nTrap].pWall1;
                if (pWall)
                {
                    pWall->picnum = sTrap[nTrap].nPicnum2;
                }
            }
        }
        else
        {
            // loc_21D92:
            if (sTrap[nTrap].nState != 5) {
                return;
            }

            auto pBullet = BuildBullet(pActor, nType, 0, pActor->int_ang(), nullptr, 1);
            if (pBullet)
            {
                if (nType == 15)
                {
                    pBullet->set_int_ang((pBullet->int_ang() - 512) & kAngleMask);
                    D3PlayFX(StaticSound[kSound32], pBullet);
                }
                else
                {
                    pBullet->spr.clipdist = 50;

                    auto pWall = sTrap[nTrap].pWall1;
                    if (pWall)
                    {
                        pWall->picnum = sTrap[nTrap].nPicnum1 + 1;
                    }

                    pWall = sTrap[nTrap].pWall2;
                    if (pWall)
                    {
                        pWall->picnum = sTrap[nTrap].nPicnum2;
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
    auto pSpark = insertActor(pActor->sector(), 0);

    pSpark->copyXY(pActor);
    pSpark->spr.cstat = 0;
    pSpark->spr.shade = -127;
    pSpark->spr.pal = 1;
    pSpark->spr.xoffset = 0;
    pSpark->spr.yoffset = 0;
    pSpark->spr.xrepeat = 50;
    pSpark->spr.yrepeat = 50;

    if (nVal >= 2)
    {
        pSpark->spr.picnum = kEnergy2;
        nSmokeSparks++;

        if (nVal == 3)
        {
            pSpark->spr.xrepeat = 120;
            pSpark->spr.yrepeat = 120;
        }
        else
        {
            pSpark->spr.xrepeat = pActor->spr.xrepeat + 15;
            pSpark->spr.yrepeat = pActor->spr.xrepeat + 15;
        }
    }
    else
    {
        int nAngle = (pActor->int_ang() + 256) - RandomSize(9);

        if (nVal)
        {
            pSpark->spr.xvel = bcos(nAngle, -5);
            pSpark->spr.yvel = bsin(nAngle, -5);
        }
        else
        {
            pSpark->spr.xvel = bcos(nAngle, -6);
            pSpark->spr.yvel = bsin(nAngle, -6);
        }

        pSpark->spr.zvel = -(RandomSize(4) << 7);
        pSpark->spr.picnum = kTile985 + nVal;
    }

    pSpark->spr.pos.Z = pActor->spr.pos.Z;
    pSpark->spr.lotag = runlist_HeadRun() + 1;
    pSpark->spr.clipdist = 1;
    pSpark->spr.hitag = 0;
    pSpark->backuppos();

    //	GrabTimeSlot(3);

    pSpark->spr.extra = -1;
    pSpark->spr.intowner = runlist_AddRunRec(pSpark->spr.lotag - 1, pSpark, 0x260000);
    pSpark->spr.hitag = runlist_AddRunRec(NewRun, pSpark, 0x260000);

    return pSpark;
}

void AISpark::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    pActor->spr.shade += 3;
    pActor->spr.xrepeat -= 2;

    if (pActor->spr.xrepeat >= 4 && pActor->spr.shade <= 100)
    {
        pActor->spr.yrepeat -= 2;

        // calling BuildSpark() with 2nd parameter as '1' will set kTile986
        if (pActor->spr.picnum == kTile986 && (pActor->spr.xrepeat & 2))
        {
            BuildSpark(pActor, 2);
        }

        if (pActor->spr.picnum >= kTile3000) {
            return;
        }

        pActor->spr.zvel += 128;

        auto nMov = movesprite(pActor, pActor->spr.xvel << 12, pActor->spr.yvel << 12, pActor->spr.zvel, 2560, -2560, CLIPMASK1);
        if (!nMov.type && !nMov.exbits) {
            return;
        }

        if (pActor->spr.zvel <= 0) {
            return;
        }
    }

    pActor->spr.xvel = 0;
    pActor->spr.yvel = 0;
    pActor->spr.zvel = 0;

    if (pActor->spr.picnum > kTile3000) {
        nSmokeSparks--;
    }

    runlist_DoSubRunRec(pActor->spr.intowner);
    runlist_FreeRun(pActor->spr.lotag - 1);
    runlist_SubRunRec(pActor->spr.hitag);
    DeleteActor(pActor);
}


void DimLights()
{
    static int word_96786 = 0;

    word_96786 = word_96786 == 0;
    if (word_96786 == 0)
        return;

    for (auto&sect: sector)
    {
        if (sect.ceilingshade < 100)
            sect.ceilingshade++;

        if (sect.floorshade < 100)
            sect.floorshade++;

        for (auto& wal : wallsofsector(&sect))
        {
            if (wal.shade < 100)
                wal.shade++;
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
            pFinaleSpr->set_int_ang(nAng);
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

DExhumedActor* BuildEnergyBlock(sectortype* pSector)
{
    int x = 0;
    int y = 0;

	for(auto& wal : wallsofsector(pSector))
    {
        x += wal.wall_int_pos().X;
        y += wal.wall_int_pos().Y;

        wal.picnum = kClockSymbol16;
        wal.pal = 0;
        wal.shade = 50;
    }

    int xAvg = x / pSector->wallnum;
    int yAvg = y / pSector->wallnum;

    auto pActor = insertActor(pSector, 406);

	pActor->set_int_xy(xAvg, yAvg);

    pSector->extra = (int16_t)EnergyBlocks.Push(pActor);

    //	GrabTimeSlot(3);

    pActor->spr.pos.Z = pSector->firstWall()->nextSector()->floorz;

    // CHECKME - name of this variable?
    int nRepeat = (pActor->int_pos().Z - pSector->int_floorz()) >> 8;
    if (nRepeat > 255) {
        nRepeat = 255;
    }

    pActor->spr.xrepeat = nRepeat;
    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    pActor->spr.xvel = 0;
    pActor->spr.yvel = 0;
    pActor->spr.zvel = 0;
    pActor->spr.extra = -1;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.hitag = 0;
    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x250000);
    pActor->backuppos();

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
	auto pSector = pActor->sector();

	for(auto& wal : wallsofsector(pSector))
    {
		if (!wal.twoSided()) continue;
		auto nextwal = wal.nextWall();

        if (nextwal->pal >= 4) {
            nextwal->pal = 7;
        }
        else {
            nextwal->pal = 0;
        }

        nextwal->shade = 50;
    }

    if (pSector->floorpal >= 4) {
        pSector->floorpal = 7;
    }
    else {
        pSector->floorpal = 0;
    }

    pSector->floorshade = 50;
    pSector->extra = -1;
    pSector->set_int_floorz(pActor->int_pos().Z);

    pActor->spr.pos.Z = (pActor->spr.pos.Z + pSector->floorz) * 0.5;

    BuildSpark(pActor, 3);

    pActor->spr.cstat = 0;
    pActor->spr.xrepeat = 100;

    PlayFX2(StaticSound[kSound78], pActor);

    pActor->spr.xrepeat = 0;

    nEnergyTowers--;

    for (int i = 0; i < 20; i++)
    {
        pActor->set_int_ang(RandomSize(11));
        BuildSpark(pActor, 1); // shoot out blue orbs
    }

    TintPalette(64, 64, 64);

    if (nEnergyTowers == 1)
    {
        runlist_ChangeChannel(nEnergyChan, nEnergyTowers);
        StatusMessage(1000, GStrings("TXT_EX_TAKEOUT"));
    }
    else if (nEnergyTowers != 0)
    {
		FString msg = GStrings("TXT_EX_TOWERSREMAIN");
		msg.Substitute("%d", std::to_string(nEnergyTowers).c_str());
        StatusMessage(500, msg.GetChars());
    }
    else
    {
        pFinaleSpr = pActor;
        lFinaleStart = PlayClock;

        if (!lFinaleStart) {
            lFinaleStart = lFinaleStart + 1;
        }

		for(auto& sect: sector)
        {
            if (sect.ceilingpal == 1) {
                sect.ceilingpal = 0;
            }

            if (sect.floorpal == 1) {
                sect.floorpal = 0;
            }

			for (auto& wal : wallsofsector(&sect))
             {
                if (wal.pal == 1) {
                    wal.pal = 0;
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

    ev->nDamage >>= 2;
    if (ev->nDamage <= 0) {
        return;
    }

    if (ev->nDamage < pActor->spr.xrepeat)
    {
        pActor->spr.xrepeat -= ev->nDamage;

        auto pActor2 = insertActor(lasthitsect, 0);

        pActor2->set_int_ang(ev->nParam);
        pActor2->spr.pos = lasthit;

        BuildSpark(pActor2, 0); // shoot out blue orb when damaged
        DeleteActor(pActor2);
    }
    else
    {
        pActor->spr.xrepeat = 0; // using xrepeat to store health
        ExplodeEnergyBlock(pActor);
    }
}

void AIEnergyBlock::RadialDamage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    auto pSector =pActor->sector();

    if (pSector->extra == -1) {
        return;
    }

    double nFloorZ = pSector->floorz;

    pSector->setfloorz(pActor->spr.pos.Z);
	pActor->spr.pos.Z--;

    ev->nDamage = runlist_CheckRadialDamage(pActor);

    // restore previous values
    pSector->set_int_floorz(nFloorZ);
	pActor->spr.pos.Z++;

    if (ev->nDamage <= 0) {
        return;
    }

    // fall through to case 0x80000
    Damage(ev);
}


DExhumedActor* BuildObject(DExhumedActor* pActor, int nOjectType, int nHitag)
{
    ChangeActorStat(pActor, ObjectStatnum[nOjectType]);

    // 0x7FFD to ensure set as blocking ('B' and 'H') sprite and also disable translucency and set not invisible
    pActor->spr.cstat = (pActor->spr.cstat | CSTAT_SPRITE_BLOCK_ALL) & ~(CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_INVISIBLE);
    pActor->spr.xvel = 0;
    pActor->spr.yvel = 0;
    pActor->spr.zvel = 0;
    pActor->spr.extra = -1;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.hitag = 0;
    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x170000);

    //	GrabTimeSlot(3);
    pActor->nPhase = ObjectList.Push(pActor);
    if (pActor->spr.statnum == kStatDestructibleSprite) {
        pActor->nHealth = 4;
    }
    else {
        pActor->nHealth = 120;
    }

    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x170000);

    int nSeq = ObjectSeq[nOjectType];

    if (nSeq > -1)
    {
        pActor->nIndex = SeqOffsets[nSeq];

        if (!nOjectType) // if not Explosion Trigger (e.g. Exploding Fire Cauldron)
        {
            pActor->nFrame = RandomSize(4) % (SeqSize[pActor->nIndex] - 1);
        }

        auto  pActor2 = insertActor(pActor->sector(), 0);
        pActor->pTarget = pActor2;
        pActor->nIndex2 = -1;

        pActor2->spr.cstat = CSTAT_SPRITE_INVISIBLE;
		pActor2->spr.pos = pActor->spr.pos;
	}
    else
    {
        pActor->nFrame = 0;
        pActor->nIndex = -1;

        if (pActor->spr.statnum == kStatDestructibleSprite) {
            pActor->nIndex2 = -1;
        }
        else {
            pActor->nIndex2 = -nHitag;
        }
    }
    pActor->backuppos();

    return pActor;
}

// in-game destructable wall mounted screen
void ExplodeScreen(DExhumedActor* pActor)
{
    pActor->spr.pos.Z -= GetActorHeightF(pActor) * 0.5;

    for (int i = 0; i < 30; i++) {
        BuildSpark(pActor, 0); // shoot out blue orbs
    }

    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    PlayFX2(StaticSound[kSound78], pActor);
}

void AIObject::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    int nStat = pActor->spr.statnum;
    int bx = pActor->nIndex;

    if (nStat == 97 || (!(pActor->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))) {
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

        pActor->spr.picnum = seq_GetSeqPicnum2(bx, pActor->nFrame);
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
            auto nMov = movesprite(pActor, pActor->spr.xvel << 6, pActor->spr.yvel << 6, pActor->spr.zvel, 0, 0, CLIPMASK0);

            if (pActor->spr.statnum == kStatExplodeTrigger) {
                pActor->spr.pal = 1;
            }

            if (nMov.exbits & kHitAux2)
            {
                pActor->spr.xvel -= pActor->spr.xvel >> 3;
                pActor->spr.yvel -= pActor->spr.yvel >> 3;
            }

            if (nMov.type == kHitSprite)
            {
                pActor->spr.yvel = 0;
                pActor->spr.xvel = 0;
            }
        }

        return;
    }
    else
    {
        int var_18;

        // red branch
        if ((nStat == kStatExplodeTarget) || (pActor->spr.pos.Z < pActor->sector()->floorz))
        {
            var_18 = 36;
        }
        else
        {
            var_18 = 34;
        }

        AddFlash(pActor->sector(), pActor->spr.pos, 128);
        BuildAnim(nullptr, var_18, 0, DVector3(pActor->spr.pos.XY(), pActor->sector()->floorz), pActor->sector(), 240, 4);

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
            runlist_SubRunRec(pActor->spr.intowner);
            runlist_SubRunRec(pActor->nRun);

            DeleteActor(pActor);
            return;
        }
        else
        {
            StartRegenerate(pActor);
            pActor->nHealth = 120;

            pActor->spr.pos = pActor->pTarget->spr.pos;
            ChangeActorSect(pActor, pActor->pTarget->sector());
            return;
        }
    }
}

void AIObject::Damage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    int nStat = pActor->spr.statnum;

    if (nStat >= 150 || pActor->nHealth <= 0) {
        return;
    }

    if (nStat == 98)
    {
        D3PlayFX((StaticSound[kSound47] | 0x2000) | (RandomSize(2) << 9), pActor);
        return;
    }

    pActor->nHealth -= (int16_t)ev->nDamage;
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

    int nStat = pActor->spr.statnum;

    if (pActor->nHealth > 0 && pActor->spr.cstat & CSTAT_SPRITE_BLOCK_ALL
        && (nStat != kStatExplodeTarget
            || ev->pRadialActor->spr.statnum == 201
            || (nRadialBullet != 3 && nRadialBullet > -1)
            || ev->pRadialActor->spr.statnum == kStatExplodeTrigger))
    {
        int nDamage = runlist_CheckRadialDamage(pActor);
        if (nDamage <= 0) {
            return;
        }

        if (pActor->spr.statnum != kStatAnubisDrum) {
            pActor->nHealth -= nDamage;
        }

        if (pActor->spr.statnum == kStatExplodeTarget)
        {
            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;
            pActor->spr.zvel = 0;
        }
        else if (pActor->spr.statnum != kStatAnubisDrum)
        {
            pActor->spr.xvel >>= 1;
            pActor->spr.yvel >>= 1;
            pActor->spr.zvel >>= 1;
        }

        if (pActor->nHealth > 0) {
            return;
        }

        if (pActor->spr.statnum == kStatExplodeTarget)
        {
            pActor->nHealth = -1;
            int ax = pActor->nIndex2;

            if (ax < 0 || ObjectList[ax]->nHealth <= 0) {
                return;
            }

            ObjectList[ax]->nHealth = -1;
        }
        else if (pActor->spr.statnum == kStatDestructibleSprite)
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
    auto nDrips = sDrip.Reserve(1);
    sDrip[nDrips].pActor = pActor;
    sDrip[nDrips].nCount = RandomSize(8) + 90;
    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
}

void DoDrips()
{
    for (unsigned i = 0; i < sDrip.Size(); i++)
    {
        sDrip[i].nCount--;
        if (sDrip[i].nCount <= 0)
        {
            DExhumedActor* pActor = sDrip[i].pActor;
            if (!pActor) continue;
            int nSeqOffset = SeqOffsets[kSeqDrips];

            if (!(pActor->sector()->Flag & kSectLava)) {
                nSeqOffset++;
            }

            seq_MoveSequence(pActor, nSeqOffset, RandomSize(2) % SeqSize[nSeqOffset]);

            sDrip[i].nCount = RandomSize(8) + 90;
        }
    }

    for (unsigned i = 0; i < sBob.Size(); i++)
    {
        sBob[i].nPhase += 4;

        int edx = bsin(sBob[i].nPhase << 3, -4);
        auto pSector =sBob[i].pSector;

        if (sBob[i].field_3)
        {
            pSector->set_int_ceilingz(edx + sBob[i].z);
        }
        else
        {
            double nFloorZ = pSector->floorz;
            pSector->set_int_floorz(edx + sBob[i].z);
            MoveSectorSprites(pSector, pSector->floorz - nFloorZ);
        }
    }
}

void SnapBobs(sectortype* pSectorA, sectortype* pSectorB)
{
    int select1 = -1;
    int select2 = select1;
    int esi;

    for (unsigned i = 0; i < sBob.Size(); i++)
    {
        auto pSector = sBob[i].pSector;

        if (pSector != pSectorA)
        {
            if (pSectorB != pSector)
                continue;

            esi = select2;
            select1 = i;
        }
        else
        {
            esi = select1;
            select2 = i;
        }

        if (esi != -1) {
            break;
        }
    }

    if (select1 <= -1) {
        return;
    }

    if (select2 <= -1) {
        return;
    }

    sBob[select1].nPhase = sBob[select2].nPhase;
}

void AddSectorBob(sectortype* pSector, int nHitag, int bx)
{
    auto nBobs = sBob.Reserve(1);
    sBob[nBobs].field_3 = bx;

    int z;

    if (bx == 0) {
        z = pSector->int_floorz();
    }
    else {
        z = pSector->int_ceilingz();
    }

    sBob[nBobs].z = z;
    sBob[nBobs].nPhase = nHitag << 4;
    sBob[nBobs].sBobID = nHitag;

    sBob[nBobs].pSector = pSector;
    StartInterpolation(pSector, bx == 0 ? Interp_Sect_Floorz : Interp_Sect_Ceilingz);

    pSector->Flag |= 0x0010;
}

int FindTrail(int nVal)
{
    for (unsigned i = 0; i < sTrail.Size(); i++)
    {
        if (sTrail[i].nVal == nVal)
            return i;
    }

    auto nTrails = sTrail.Reserve(1);
    sTrail[nTrails].nVal = nVal;
    sTrail[nTrails].nPoint = -1;
    sTrail[nTrails].nPoint2 = -1;
    return nTrails;
}

// ok ?
void ProcessTrailSprite(DExhumedActor* pActor, int nLotag, int nHitag)
{
    auto nPoint = sTrailPoint.Reserve(1);

    sTrailPoint[nPoint].x = pActor->int_pos().X;
    sTrailPoint[nPoint].y = pActor->int_pos().Y;

    int nTrail = FindTrail(nHitag);

    int var_14 = nLotag - 900;

    sTrailPoint[nPoint].nTrailPointVal = var_14;

    int field0 = sTrail[nTrail].nPoint;

    if (field0 == -1)
    {
        sTrail[nTrail].nPoint = nPoint;
        sTrail[nTrail].nPoint2 = nPoint;

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

                if (field0 == sTrail[nTrail].nPoint) {
                    sTrail[nTrail].nPoint = nPoint;
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
            sTrail[nTrail].nPoint2 = nPoint;
        }
    }

    DeleteActor(pActor);
}

// ok?
void AddMovingSector(sectortype* pSector, int edx, int ebx, int ecx)
{
    CreatePushBlock(pSector);
    setsectinterpolate(pSector);

    int nTrail = FindTrail(ebx);


    auto nMoveSects = sMoveSect.Reserve(1);
    MoveSect* pMoveSect = &sMoveSect[nMoveSects];

    pMoveSect->sMoveDir = 1;
    pMoveSect->nTrail = nTrail;
    pMoveSect->nTrailPoint = -1;
    pMoveSect->pCurSector = nullptr;
    pMoveSect->nFlags = ecx;
    pMoveSect->field_10 = (edx / 1000) + 1;
    pMoveSect->pSector = pSector;

    if (ecx & 8)
    {
        pMoveSect->nChannel = runlist_AllocChannel(ebx % 1000);
    }
    else
    {
        pMoveSect->nChannel = -1;
    }

    pSector->floorstat |= CSTAT_SECTOR_ALIGN;
}

void DoMovingSects()
{
    for (unsigned i = 0; i < sMoveSect.Size(); i++)
    {
        if (sMoveSect[i].pSector == nullptr) {
            continue;
        }

        if (sMoveSect[i].nChannel != -1 && !sRunChannels[sMoveSect[i].nChannel].c) {
            continue;
        }

        auto pSector =sMoveSect[i].pSector;
        int nBlock = pSector->extra;

        BlockInfo* pBlockInfo = &sBlockInfo[nBlock];

        if (sMoveSect[i].nTrailPoint == -1)
        {
            if (sMoveSect[i].nFlags & 0x20)
            {
                runlist_ChangeChannel(sMoveSect[i].nChannel, 0);
            }

            int ax;

            if (sMoveSect[i].nFlags & 0x10)
            {
                sMoveSect[i].sMoveDir = -sMoveSect[i].sMoveDir;
                if (sMoveSect[i].sMoveDir > 0)
                {
                    ax = sTrail[sMoveSect[i].nTrail].nPoint;
                }
                else
                {
                    ax = sTrail[sMoveSect[i].nTrail].nPoint2;
                }
            }
            else
            {
                ax = sTrail[sMoveSect[i].nTrail].nPoint;
            }

            sMoveSect[i].nTrailPoint = ax;
        }

        int nTrail = sMoveSect[i].nTrailPoint;
        //		TrailPoint *pTrail = &sTrailPoint[nTrail];

                // loc_23872:
        int nAngle = getangle(sTrailPoint[nTrail].x - pBlockInfo->x, sTrailPoint[nTrail].y - pBlockInfo->y);

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
        if (sMoveSect[i].pCurSector != nullptr)
        {
            MoveSector(sMoveSect[i].pCurSector, -1, &nXVel, &nYVel);
        }

        int var_2C = nXVel;
        int var_30 = nYVel;

        MoveSector(pSector, -1, &nXVel, &nYVel);

        if (nXVel != var_2C || nYVel != var_30)
        {
            MoveSector(sMoveSect[i].pCurSector, -1, &var_2C, &var_30);
            MoveSector(sMoveSect[i].pCurSector, -1, &nXVel, &nYVel);
        }
    }
}

void PostProcess()
{
    for (unsigned i = 0; i < sMoveSect.Size(); i++)
    {
        int nTrail = sMoveSect[i].nTrail;
        sMoveSect[i].nTrailPoint = sTrail[nTrail].nPoint;

        if (sMoveSect[i].nFlags & 0x40) {
            runlist_ChangeChannel(sMoveSect[i].nChannel, 1);
        }

        auto pSector =sMoveSect[i].pSector;

        if (pSector->Flag & kSectUnderwater)
        {
            pSector->ceilingstat |= CSTAT_SECTOR_ALIGN;
            pSector->floorstat &= ~(CSTAT_SECTOR_EXHUMED_BIT1 | CSTAT_SECTOR_EXHUMED_BIT2);

            for (unsigned j = 0; j < sMoveSect.Size(); j++)
            {
                if (j != i && sMoveSect[i].nTrail == sMoveSect[j].nTrail)
                {
                    sMoveSect[j].pCurSector = sMoveSect[i].pSector;

                    SnapSectors(sMoveSect[j].pSector, sMoveSect[i].pSector, 0);
                    sMoveSect[i].pSector = nullptr;
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
                        SnapSectors(sBob[i].pSector, sBob[j].pSector, 0);
                    }
                }
            }
        }
    }

    if (!(currentLevel->gameflags & LEVEL_EX_COUNTDOWN))
    {
        for (auto& sect: sector)
        {
            int var_20 = 30000;

            if (sect.Speed && sect.Depth && !(sect.Flag & kSectLava))
            {
                sect.pSoundSect = &sect;
                sect.Sound = StaticSound[kSound43];
            }
            else
            {
                for (auto& sectj: sector)
                {
                    // loc_23CA6:

                    if (&sect != &sectj && sectj.Speed && !(sect.Flag & kSectLava))
                    {
						int xVal = abs(sect.firstWall()->wall_int_pos().X - sectj.firstWall()->wall_int_pos().X);
						int yVal = abs(sect.firstWall()->wall_int_pos().Y - sectj.firstWall()->wall_int_pos().Y);

                        if (xVal < 15000 && yVal < 15000 && (xVal + yVal < var_20))
                        {
                            var_20 = xVal + yVal;
                            sect.pSoundSect = &sectj;
                            sect.Sound = StaticSound[kSound43];
                        }
                    }
                }
            }
        }
    }
    else // nMap == kMap20)
    {
        for(auto& sect: sector)
        {
            sect.pSoundSect = &sect;
            sect.Sound = StaticSound[kSound62];

            for(auto& wal : wallsofsector(&sect))
            {
                if (wal.picnum == kTile3603)
                {
                    wal.pal = 1;
                    auto pActor = insertActor(&sect, 407);
                    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                }
            }
        }

        ExhumedSpriteIterator it;
        while (auto act = it.Next())
        {
            if (act->spr.statnum < kMaxStatus && act->spr.picnum == kTile3603)
            {
                ChangeActorStat(act, 407);
                act->spr.pal = 1;
            }
        }
    }

    for (unsigned i = 0; i < ObjectList.Size(); i++)
    {
        auto pObjectActor = ObjectList[i];

        if (pObjectActor->spr.statnum == kStatExplodeTarget)
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

                    if (i != j && ObjectList[j]->spr.statnum == kStatExplodeTarget && edi == ObjectList[j]->nIndex2)
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

