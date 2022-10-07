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
    DVector2 pos;
    uint8_t nTrailPointVal;
    int16_t nTrailPointPrev;
    int16_t nTrailPointNext;

};

struct Bob
{
    sectortype* pSector;
    double Z;
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
    int nSpeed1;
    int nSpeed2;
    int16_t nCountZOffsets; // count of items in zOffsets
    int16_t nCurZOffset;
    double zOffsets[8]; // different Z offsets
    int16_t nRunRec;
};

// 16 bytes
struct MoveSect
{
    sectortype* pSector;
    sectortype* pCurSector;
    int nMoveDist;
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
    DVector2 pos[6];
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
        arc("pos", w.pos)
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
            ("z", w.Z)
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
            ("at6", w.nSpeed1)
            ("ata", w.nSpeed2)
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
            ("flags", w.nFlags)
            ("cursector", w.pCurSector)
            ("movedist", w.nMoveDist)
            ("channel", w.nChannel)
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
            .Array("pos", w.pos, 6)
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DExhumedActor* BuildWallSprite(sectortype* pSector)
{
    auto wal = pSector->firstWall();

    auto pActor = insertActor(pSector, 401);

	pActor->spr.pos = DVector3(wal->center(), (pSector->floorz + pSector->ceilingz) * 0.5);
    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;

    return pActor;
 }

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DExhumedActor* FindWallSprites(sectortype* pSector)
{
    double min_x = DBL_MAX;
    double min_y = DBL_MAX;

    double max_x = -DBL_MAX;
    double max_y = -DBL_MAX;

	for (auto& wal : wallsofsector(pSector))
    {
        if (wal.pos.X < min_x) {
            min_x = wal.pos.X;
        }

        if (max_x < wal.pos.X) {
            max_x = wal.pos.X;
        }

        if (min_y > wal.pos.Y) {
            min_y = wal.pos.Y;
        }

        if (max_y < wal.pos.Y) {
            max_y = wal.pos.Y;
        }
    }

    min_y -= 5./16;
    max_x += 5./16;
    max_y += 5./16;
    min_x -= 5./16;

    DExhumedActor* pAct = nullptr;

    ExhumedSpriteIterator it;
    while (auto actor = it.Next())
    {
        if (actor->spr.lotag == 0)
        {
            if ((actor->spr.cstat & (CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_ONE_SIDE)) == (CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_ONE_SIDE))
            {
                double act_x = actor->spr.pos.X;
                double act_y = actor->spr.pos.Y;

                if ((act_x >= min_x) && (max_x >= act_x) && (act_y >= min_y) && (act_y <= max_y))
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

        pAct->spr.pos = { (min_x + max_x) / 2, (min_y + max_y) / 2, pSector->floorz };
        pAct->spr.cstat = CSTAT_SPRITE_INVISIBLE;
        pAct->spr.intowner = -1;
        pAct->spr.lotag = 0;
        pAct->spr.hitag = 0;
    }

    return pAct;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int BuildElevF(int nChannel, sectortype* pSector, DExhumedActor* nWallSprite, int arg_4, int arg_5, int nCount, ...)
{
    auto ElevCount = Elevator.Reserve(1);

    Elevator[ElevCount].nFlags = 2;
    Elevator[ElevCount].nSpeed1 = arg_4;
    Elevator[ElevCount].nRunRec = -1;
    Elevator[ElevCount].nSpeed2 = arg_5;
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

        Elevator[ElevCount].zOffsets[nVal] = va_arg(zlist, double);
    }
    va_end(zlist);

    return ElevCount;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int BuildElevC(int arg1, int nChannel, sectortype* pSector, DExhumedActor* nWallSprite, int speed1, int speed2, int nCount, ...)
{
    int edi = speed1;

    auto ElevCount = Elevator.Reserve(1);

    Elevator[ElevCount].nFlags = arg1;

    if (arg1 & 4)
    {
        edi = speed1 / 2;
    }

    Elevator[ElevCount].nSpeed1 = edi;
    Elevator[ElevCount].nCountZOffsets = 0;
    Elevator[ElevCount].nCurZOffset = 0;
    Elevator[ElevCount].nSpeed2 = speed2;
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

        Elevator[ElevCount].zOffsets[nVal] = va_arg(zlist, double);
    }
    va_end(zlist);

    return ElevCount;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static double LongSeek(double* pZVal, double a2, double a3, double a4)
{
    double v4 = a2 - *pZVal;

    if (v4 < 0)
    {
        if (-a3 > v4)
            v4 = -a3;
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

static inline DVector2 LongSeek(DVector2& pZVals, const DVector2& a2, double a3, double a4)
{
    return DVector2(LongSeek(&pZVals.X, a2.X, a3, a4), LongSeek(&pZVals.Y, a2.Y, a3, a4));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int CheckSectorSprites(sectortype* pSector, int nVal)
{
    int b = 0;

    if (nVal)
    {
        double nZDiff = pSector->floorz - pSector->ceilingz;

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void MoveSectorSprites(sectortype* pSector, double z)
{
    double newz = pSector->floorz;
    double oldz = newz - z;
    double minz = min(newz, oldz);
    double maxz = max(newz, oldz);
    ExhumedSectIterator it(pSector);
    while (auto pActor = it.Next())
    {
        double actz = pActor->spr.pos.Z;
        if ((pActor->spr.statnum != 200 && actz >= minz && actz <= maxz) || pActor->spr.statnum >= 900)
        {
			pActor->spr.pos.Z = newz;
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

    double move = 0; // initialise to *something*

    if (var_18 & 0x2)
    {
        int nZOffset = Elevator[nElev].nCurZOffset;
        double nZVal = Elevator[nElev].zOffsets[nZOffset];

        StartInterpolation(pSector, Interp_Sect_Floorz);
        double fz = pSector->floorz;
        double nVal = LongSeek(&fz, nZVal, Elevator[nElev].nSpeed1 / 256., Elevator[nElev].nSpeed2 / 256.);
        pSector->floorz = fz;
        move = nVal;

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
            MoveSectorSprites(pSector, nVal);

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
        double ceilZ = pSector->ceilingz;

        int nZOffset = Elevator[nElev].nCurZOffset;
        double zVal = Elevator[nElev].zOffsets[nZOffset];

        StartInterpolation(pSector, Interp_Sect_Ceilingz);
        double nVal = LongSeek(&ceilZ, zVal, Elevator[nElev].nSpeed1 / 256., Elevator[nElev].nSpeed2 / 256.);
        move = nVal;

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
        pSector->ceilingz = ceilZ;
    }

    // maybe this doesn't go here?
    while (pElevSpr)
    {
        pElevSpr->spr.pos.Z += move;
        pElevSpr = pElevSpr->pTarget;
    }
}


void InitWallFace()
{
    WallFace.Clear();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitPoint()
{
    PointList.Clear();
}

int GrabPoint()
{
    return PointList.Reserve(1);
}

void InitSlide()
{
    SlideData.Clear();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

    SlideData[nSlide].pos[0] = pStartWall->pos;
    SlideData[nSlide].pos[1] = pWall2->pos;
    SlideData[nSlide].pos[2] = pWall1->pos;
    SlideData[nSlide].pos[3] = pWall3->pos;
    SlideData[nSlide].pos[4] = p2ndLastWall->pos;
    SlideData[nSlide].pos[5] = pWall4->pos;

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AISlide::Tick(RunListEvent* ev)
{
    int nRun = ev->nRun;
    int nSlide = RunData[nRun].nObjIndex;
    assert(nSlide >= 0 && nSlide < (int)SlideData.Size());

    int nChannel = SlideData[nSlide].nChannel;
    int clipstate = 0;

    int cx = sRunChannels[nChannel].c;

    int clipmask = clipstate + 1; // RENAME

    if (cx == 1)
    {
        auto pWall = SlideData[nSlide].pWall1;
        auto wlPos = pWall->pos;
        auto nSeek = LongSeek(wlPos, SlideData[nSlide].pos[4], 1.25, 1.25);
        dragpoint(pWall, wlPos);
        movesprite(SlideData[nSlide].pActor, nSeek, 0, 0, CLIPMASK1);

        if (nSeek.isZero())
        {
             clipstate = clipmask;
        }

        pWall = SlideData[nSlide].pStartWall;
        wlPos = pWall->pos + nSeek;
        dragpoint(pWall, wlPos);

        pWall = SlideData[nSlide].pWall3;
        wlPos = pWall->pos;
        nSeek = LongSeek(wlPos, SlideData[nSlide].pos[5], 1.25, 1.25);
        dragpoint(pWall, wlPos);

        if (nSeek.isZero())
        {
            clipstate++;
        }

        pWall = SlideData[nSlide].pWall2;
        wlPos = pWall->pos + nSeek;
        dragpoint(pWall, wlPos);
    }
    else if (cx == 0) // right branch
    {
        auto pWall = SlideData[nSlide].pStartWall;
        auto wlPos = pWall->pos;
        auto nSeek = LongSeek(wlPos, SlideData[nSlide].pos[0], 1.25, 1.25);
        dragpoint(pWall, wlPos);

        if (nSeek.isZero())
        {
            clipstate = clipmask;
        }

        pWall = SlideData[nSlide].pWall1;
        wlPos = pWall->pos + nSeek;
        dragpoint(pWall, wlPos);

        pWall = SlideData[nSlide].pWall2;
        wlPos = pWall->pos;
        nSeek = LongSeek(wlPos, SlideData[nSlide].pos[1], 1.25, 1.25);
        dragpoint(pWall, wlPos);

        if (nSeek.isZero())
        {
            clipstate++;
        }

        pWall = SlideData[nSlide].pWall3;
        wlPos = pWall->pos + nSeek;
        dragpoint(pWall, wlPos);
    }

    // loc_21A51:
    if (clipstate >= 2)
    {
        runlist_SubRunRec(SlideData[nSlide].nRunRec);

        SlideData[nSlide].nRunRec = -1;
        D3PlayFX(StaticSound[nStopSound], SlideData[nSlide].pActor);

        runlist_ReadyChannel(nChannel);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int BuildTrap(DExhumedActor* pActor, int edx, int ebx, int ecx)
{
    int var_14 = edx;
    int var_18 = ebx;
    int var_10 = ecx;

    int nTrap = sTrap.Reserve(1);
    sTrap[nTrap] = {};

    ChangeActorStat(pActor, 0);

    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

            auto pBullet = BuildBullet(pActor, nType, 0, pActor->spr.angle, nullptr, 1);
            if (pBullet)
            {
                if (nType == 15)
                {
                    pBullet->spr.angle -= DAngle90;
                    D3PlayFX(StaticSound[kSound32], pBullet);
                }
                else
                {
					pBullet->clipdist = 12.5;

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int BuildArrow(DExhumedActor* nSprite, int nVal)
{
    return BuildTrap(nSprite, 0, -1, nVal);
}

int BuildFireBall(DExhumedActor* nSprite, int a, int b)
{
    return BuildTrap(nSprite, 1, a, b);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DExhumedActor* BuildSpark(DExhumedActor* pActor, int nVal)
{
    auto pSpark = insertActor(pActor->sector(), 0);

    pSpark->spr.pos.XY() = pActor->spr.pos.XY();
    pSpark->spr.cstat = 0;
    pSpark->spr.shade = -127;
    pSpark->spr.pal = 1;
    pSpark->spr.xoffset = 0;
    pSpark->spr.yoffset = 0;
    pSpark->spr.SetScale(0.78125, 0.78125);

    if (nVal >= 2)
    {
        pSpark->spr.picnum = kEnergy2;
        nSmokeSparks++;

        if (nVal == 3)
        {
			pSpark->spr.SetScale(1.875, 1.875);
        }
        else
        {
			pSpark->spr.scale = pActor->spr.scale + DVector2(0.234375, 0.234375);
        }
    }
    else
    {
        auto nAngle = pActor->spr.angle + DAngle22_5 - RandomAngle9();

        if (nVal)
        {
			pSpark->vel.XY() = nAngle.ToVector() * 32;
        }
        else
        {
			pSpark->vel.XY() = nAngle.ToVector() * 16;
        }

        pSpark->vel.Z = -RandomSize(4) * 0.5;
        pSpark->spr.picnum = kTile985 + nVal;
    }

    pSpark->spr.pos.Z = pActor->spr.pos.Z;
    pSpark->spr.lotag = runlist_HeadRun() + 1;
	pSpark->clipdist = 0.25;
    pSpark->spr.hitag = 0;
    pSpark->backuppos();

    //	GrabTimeSlot(3);

    pSpark->spr.extra = -1;
    pSpark->spr.intowner = runlist_AddRunRec(pSpark->spr.lotag - 1, pSpark, 0x260000);
    pSpark->spr.hitag = runlist_AddRunRec(NewRun, pSpark, 0x260000);

    return pSpark;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AISpark::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    pActor->spr.shade += 3;
	pActor->spr.scale.X += (-0.03125);

    if (pActor->spr.scale.X >= 0.0625 && pActor->spr.shade <= 100)
    {
		pActor->spr.scale.Y += (-0.03125);

        // calling BuildSpark() with 2nd parameter as '1' will set kTile986
        if (pActor->spr.picnum == kTile986 && int((pActor->spr.scale.X * INV_REPEAT_SCALE)) & 2) // hack alert
        {
            BuildSpark(pActor, 2);
        }

        if (pActor->spr.picnum >= kTile3000) {
            return;
        }

        pActor->vel.Z += 0.5;

        auto nMov = movespritevel(pActor, pActor->vel, 16., -10, CLIPMASK1);
        if (!nMov.type && !nMov.exbits) {
            return;
        }

        if (pActor->vel.Z <= 0) {
            return;
        }
    }

    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;

    if (pActor->spr.picnum > kTile3000) {
        nSmokeSparks--;
    }

    runlist_DoSubRunRec(pActor->spr.intowner);
    runlist_FreeRun(pActor->spr.lotag - 1);
    runlist_SubRunRec(pActor->spr.hitag);
    DeleteActor(pActor);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
            pFinaleSpr->spr.angle = RandomAngle();
            BuildSpark(pFinaleSpr, 1);
        }

        if (!RandomSize(2))
        {
            PlayFX2(StaticSound[kSound78] | 0x2000, pFinaleSpr);

            for (int i = 0; i < nTotalPlayers; i++) {
                nQuake[i] = 5.;
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DExhumedActor* BuildEnergyBlock(sectortype* pSector)
{
	DVector2 apos(0, 0);

	for(auto& wal : wallsofsector(pSector))
    {
		apos += wal.pos;
		
        wal.picnum = kClockSymbol16;
        wal.pal = 0;
        wal.shade = 50;
    }

    auto pActor = insertActor(pSector, 406);

	pActor->spr.pos.XY() = apos / pSector->wallnum;

    pSector->extra = (int16_t)EnergyBlocks.Push(pActor);

    //	GrabTimeSlot(3);

    pActor->spr.pos.Z = pSector->firstWall()->nextSector()->floorz;

    // CHECKME - name of this variable?
    int nRepeat = int(pActor->spr.pos.Z - pSector->floorz);
    if (nRepeat > 255) {
        nRepeat = 255;
    }

    pActor->spr.intangle = nRepeat;
    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
    pActor->spr.extra = -1;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.hitag = 0;
    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x250000);
    pActor->backuppos();

    return pActor;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
    pSector->floorz = pActor->spr.pos.Z;

    pActor->spr.pos.Z = (pActor->spr.pos.Z + pSector->floorz) * 0.5;

    BuildSpark(pActor, 3);

    pActor->spr.cstat = 0;
    pActor->spr.intangle = 100;

    PlayFX2(StaticSound[kSound78], pActor);

    pActor->spr.intangle = 0;

    nEnergyTowers--;

    for (int i = 0; i < 20; i++)
    {
        pActor->spr.angle = RandomAngle();
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIEnergyBlock::Damage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    ev->nDamage >>= 2;
    if (ev->nDamage <= 0) {
        return;
    }

    if (ev->nDamage < pActor->spr.intangle)
    {
        pActor->spr.intangle -= ev->nDamage;

        auto pActor2 = insertActor(lasthitsect, 0);

        pActor2->spr.angle = mapangle(ev->nParam);
        pActor2->spr.pos = lasthit;

        BuildSpark(pActor2, 0); // shoot out blue orb when damaged
        DeleteActor(pActor2);
    }
    else
    {
        pActor->spr.intangle = 0; // using intangle to store health
        ExplodeEnergyBlock(pActor);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
    pSector->floorz = nFloorZ;
	pActor->spr.pos.Z++;

    if (ev->nDamage <= 0) {
        return;
    }

    // fall through to case 0x80000
    Damage(ev);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DExhumedActor* BuildObject(DExhumedActor* pActor, int nOjectType, int nHitag)
{
    ChangeActorStat(pActor, ObjectStatnum[nOjectType]);

    // 0x7FFD to ensure set as blocking ('B' and 'H') sprite and also disable translucency and set not invisible
    pActor->spr.cstat = (pActor->spr.cstat | CSTAT_SPRITE_BLOCK_ALL) & ~(CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_INVISIBLE);
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
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

//---------------------------------------------------------------------------
//
// in-game destructable wall mounted screen
//
//---------------------------------------------------------------------------

void ExplodeScreen(DExhumedActor* pActor)
{
    pActor->spr.pos.Z -= GetActorHeight(pActor) * 0.5;

    for (int i = 0; i < 30; i++) {
        BuildSpark(pActor, 0); // shoot out blue orbs
    }

    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    PlayFX2(StaticSound[kSound78], pActor);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
            auto nMov = movespritevel(pActor, pActor->vel, 0.25, 0, CLIPMASK0);

            if (pActor->spr.statnum == kStatExplodeTrigger) {
                pActor->spr.pal = 1;
            }

            if (nMov.exbits & kHitAux2)
            {
				pActor->vel.XY() *= 0.875;
            }

            if (nMov.type == kHitSprite)
            {
                pActor->vel.Y = 0;
                pActor->vel.X = 0;
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
        BuildAnim(nullptr, var_18, 0, DVector3(pActor->spr.pos.XY(), pActor->sector()->floorz), pActor->sector(), 3.75, 4);

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
            pActor->ZeroVelocity();
        }
        else if (pActor->spr.statnum != kStatAnubisDrum)
        {
            pActor->vel *= 0.5;
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

        double amount = BobVal(sBob[i].nPhase << 3) * 4.;
        auto pSector =sBob[i].pSector;

        if (sBob[i].field_3)
        {
            pSector->setceilingz(amount + sBob[i].Z);
        }
        else
        {
            double nFloorZ = pSector->floorz;
            pSector->setfloorz(amount + sBob[i].Z);
            MoveSectorSprites(pSector, pSector->floorz - nFloorZ);
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AddSectorBob(sectortype* pSector, int nHitag, int bx)
{
    auto nBobs = sBob.Reserve(1);
    sBob[nBobs].field_3 = bx;

    double Z;

    if (bx == 0) {
        Z = pSector->floorz;
    }
    else {
        Z = pSector->ceilingz;
    }

    sBob[nBobs].Z = Z;
    sBob[nBobs].nPhase = nHitag << 4;
    sBob[nBobs].sBobID = nHitag;

    sBob[nBobs].pSector = pSector;
    StartInterpolation(pSector, bx == 0 ? Interp_Sect_Floorz : Interp_Sect_Ceilingz);

    pSector->Flag |= 0x0010;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ProcessTrailSprite(DExhumedActor* pActor, int nLotag, int nHitag)
{
    auto nPoint = sTrailPoint.Reserve(1);

    sTrailPoint[nPoint].pos = pActor->spr.pos;

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AddMovingSector(sectortype* pSector, int lotag, int hitag, int flags)
{
    CreatePushBlock(pSector);
    setsectinterpolate(pSector);

    int nTrail = FindTrail(hitag);


    auto nMoveSects = sMoveSect.Reserve(1);
    MoveSect* pMoveSect = &sMoveSect[nMoveSects];

    pMoveSect->sMoveDir = 1;
    pMoveSect->nTrail = nTrail;
    pMoveSect->nTrailPoint = -1;
    pMoveSect->pCurSector = nullptr;
    pMoveSect->nFlags = flags;
    pMoveSect->nMoveDist = (lotag / 1000) + 1;
    pMoveSect->pSector = pSector;

    if (flags & 8)
    {
        pMoveSect->nChannel = runlist_AllocChannel(hitag % 1000);
    }
    else
    {
        pMoveSect->nChannel = -1;
    }

    pSector->floorstat |= CSTAT_SECTOR_ALIGN;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

        auto nAngle = (sTrailPoint[nTrail].pos - pBlockInfo->pos).Angle();

        auto vel = nAngle.ToVector() * sMoveSect[i].nMoveDist;
        auto delta = sTrailPoint[nTrail].pos - pBlockInfo->pos;


        if (abs(vel.X) > abs(delta.X) || abs(vel.Y) > abs(delta.Y))
        {
            vel = delta;

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
            MoveSector(sMoveSect[i].pCurSector, -minAngle, vel);
        }
        auto ovel = vel;

        MoveSector(pSector, -minAngle, vel);

        if (sMoveSect[i].pCurSector != nullptr && vel != ovel)
        {
            MoveSector(sMoveSect[i].pCurSector, -minAngle, ovel);
            MoveSector(sMoveSect[i].pCurSector, -minAngle, vel);
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
            double maxval = 300000;

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
						double xVal = abs(sect.firstWall()->pos.X - sectj.firstWall()->pos.X);
						double yVal = abs(sect.firstWall()->pos.Y - sectj.firstWall()->pos.Y);

                        if (xVal < 15000/16. && yVal < 15000/16. && (xVal + yVal < maxval))
                        {
                            maxval = xVal + yVal;
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

