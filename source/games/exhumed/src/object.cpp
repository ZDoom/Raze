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
    char nTrailPointVal;
    short nTrailPointPrev;
    short nTrailPointNext;

};

struct Bob
{
    short nSector;
    char field_2;
    char field_3;
    int z;
    short sBobID;

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
    short sMoveDir;

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

struct slideData
{
    short nChannel;
    short field_2a;
    short field_4a;
    short nSprite;
    short field_8a;

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
    short nTrapInterval;

};


TArray<Point> PointList;
TArray<Trap> sTrap;
TArray<Bob> sBob;
TArray<Trail> sTrail;
TArray<TrailPoint> sTrailPoint;
TArray<Elev> Elevator;
TArray<Object> ObjectList;
TArray<MoveSect> sMoveSect;
TArray<slideData> SlideData;
TArray<wallFace> WallFace;
TArray<Drip> sDrip;

int lFinaleStart;

short nEnergyBlocks;
short nFinaleStage;
short nFinaleSpr;

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
		arc("sprite", w.nSprite)
			("at2", w.field_2)
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
			("sprite", w.nSprite)
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
FSerializer& Serialize(FSerializer& arc, const char* keyname, Object& w, Object* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("at0", w.field_0)
			("health", w.nHealth)
			("at4", w.field_4)
			("at8", w.field_8)
			("sprite", w.nSprite)
			("at8", w.field_8)
			("at10", w.field_10)
			("at12", w.field_12)
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
			("at20", w.field_20)
			("at24", w.field_24)
			("at28", w.field_28)
			("at2c", w.field_2C)
			("at30", w.field_30)
			("at34", w.field_34)
			("at38", w.field_38)
			("at3c", w.field_3C)
			("channel", w.nChannel)
			("at2a", w.field_2a)
			("at4a", w.field_4a)
			("at6a", w.nSprite)
			("at8a", w.field_8a)
			.EndObject();
	}
	return arc;
}
FSerializer& Serialize(FSerializer& arc, const char* keyname, Point& w, Point* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("at0", w.field_0)
			("at2", w.field_2)
			("at4", w.field_4)
			("at6", w.field_6)
			("at8", w.field_8)
			("ata", w.field_A)
			("atc", w.field_C)
			("ate", w.field_E)
			.EndObject();
	}
	return arc;
}
FSerializer& Serialize(FSerializer& arc, const char* keyname, Trap& w, Trap* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("at0", w.field_0)
			("sprite", w.nSprite)
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
			("energyblocks", nEnergyBlocks)
			("finalestage", nFinaleStage)
			("finalespr", nFinaleSpr)
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

    nEnergyBlocks = 0;
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
int BuildWallSprite(int nSector)
{
    int nWall = sector[nSector].wallptr;

    int x = wall[nWall].x;
    int y = wall[nWall].y;

    int x2 = wall[nWall + 1].x;
    int y2 = wall[nWall + 1].y;

    int nSprite = insertsprite(nSector, 401);
	auto pSprite = &sprite[nSprite];

    pSprite->x = (x + x2) / 2;
    pSprite->y = (y + y2) / 2;
    pSprite->z = (sector[nSector].floorz + sector[nSector].ceilingz) / 2;
    pSprite->cstat = 0x8000;

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
		auto pSprite = &sprite[nSprite];

        pSprite->x = (var_24 + esi) / 2;
        pSprite->y = (ecx + edi) / 2;
        pSprite->z = sector[nSector].floorz;
        pSprite->cstat = 0x8000;
        pSprite->owner = -1;
        pSprite->lotag = 0;
        pSprite->hitag = 0;
    }

    return nSprite;
}

int BuildElevF(int nChannel, int nSector, int nWallSprite, int arg_4, int arg_5, int nCount, ...)
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
        int nZDiff = sector[nSector].floorz - sector[nSector].ceilingz;

        int nSprite;
        SectIterator it(nSector);
        while ((nSprite = it.NextIndex()) >= 0)
        {
			auto pSprite = &sprite[nSprite];
            if ((pSprite->cstat & 0x101) && (nZDiff < GetSpriteHeight(nSprite)))
            {
                if (nVal != 1) {
                    return 1;
                }

                b = 1;

                runlist_DamageEnemy(nSprite, -1, 5);

                if (pSprite->statnum == 100 && PlayerList[GetPlayerFromSprite(nSprite)].nHealth <= 0)
                {
                    PlayFXAtXYZ(StaticSound[kSoundJonFDie],
                        pSprite->x,
                        pSprite->y,
                        pSprite->z,
                        pSprite->sectnum | 0x4000);
                }
            }
        }
    }
    else
    {
        int i;
        SectIterator it(nSector);
        while ((i = it.NextIndex()) >= 0)
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
    int newz = sector[nSector].floorz;
    int oldz = newz - z;
    int minz = std::min(newz, oldz);
    int maxz = std::max(newz, oldz);
    int nSprite;
    SectIterator it(nSector);
    while ((nSprite = it.NextIndex()) >= 0)
    {
		auto pSprite = &sprite[nSprite];
        int z = pSprite->z;
        if ((pSprite->statnum != 200 && z >= minz && z <= maxz) || pSprite->statnum >= 900)
        {
            pSprite->z = newz;
        }
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
    assert(nElev >= 0 && nElev < (int)Elevator.Size());

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

                StartInterpolation(nSector, Interp_Sect_Floorz);
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
                sectortype *cursect = &sector[nSector];

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

                StartInterpolation(nSector, Interp_Sect_Ceilingz);
                cursect->ceilingz = ceilZ;
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
    WallFace.Clear();
}

int BuildWallFace(short nChannel, short nWall, int nCount, ...)
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

    return WallFaceCount | 0x70000;
}

void FuncWallFace(int a, int, int nRun)
{
    int nWallFace = RunData[nRun].nVal;
    assert(nWallFace >= 0 && nWallFace < (int)WallFace.Size());

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

    short nSector = IdentifySector(nStartWall);

    SlideData[nSlide].field_4a = -1;
    SlideData[nSlide].nChannel = nChannel;
    SlideData[nSlide].field_2a = -1;

    int nPoint = GrabPoint();

    SlideData[nSlide].field_2a = nPoint;

    PointList[nPoint].field_E = -1;
    PointList[nPoint].field_0 = nSector;

    short startwall = sector[nSector].wallptr;
    short endwall = startwall + sector[nSector].wallnum;

    for (int nWall = startwall; nWall < endwall; nWall++)
    {
        short ax = SlideData[nSlide].field_2a;

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

                PointList[nPoint].field_E = SlideData[nSlide].field_2a;
                PointList[nPoint].field_0 = wall[nWall].nextsector;

                SlideData[nSlide].field_2a = nPoint;
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

    SlideData[nSlide].field_20 = wall[nWall1].x;
    SlideData[nSlide].field_24 = wall[nWall1].y;

    SlideData[nSlide].field_28 = wall[nWall3].x;
    SlideData[nSlide].field_2C = wall[nWall3].y;

    SlideData[nSlide].field_30 = wall[ecx].x;
    SlideData[nSlide].field_34 = wall[ecx].y;

    SlideData[nSlide].field_38 = wall[nWall4].x;
    SlideData[nSlide].field_3C = wall[nWall4].y;

    StartInterpolation(nStartWall, Interp_Wall_X);
    StartInterpolation(nStartWall, Interp_Wall_Y);

    StartInterpolation(nWall1, Interp_Wall_X);
    StartInterpolation(nWall1, Interp_Wall_Y);

    StartInterpolation(nWall2, Interp_Wall_X);
    StartInterpolation(nWall2, Interp_Wall_Y);

    StartInterpolation(nWall3, Interp_Wall_X);
    StartInterpolation(nWall3, Interp_Wall_Y);


    int nSprite = insertsprite(nSector, 899);
	auto pSprite = &sprite[nSprite];

    SlideData[nSlide].nSprite = nSprite;
    pSprite->cstat = 0x8000;
    pSprite->x = wall[nStartWall].x;
    pSprite->y = wall[nStartWall].y;
    pSprite->z = sector[nSector].floorz;
    pSprite->backuppos();

    SlideData[nSlide].field_8a = 0;

    return nSlide | 0x80000;
}

void FuncSlide(int a, int, int nRun)
{
    int nSlide = RunData[nRun].nVal;
    assert(nSlide >= 0 && nSlide < (int)SlideData.Size());

    short nChannel = SlideData[nSlide].nChannel;

    int nMessage = a & 0x7F0000;

    int ebp = 0;

    short cx = sRunChannels[nChannel].c;

    switch (nMessage)
    {
        case 0x10000:
        {
            if (SlideData[nSlide].field_4a >= 0)
            {
                runlist_SubRunRec(SlideData[nSlide].field_4a);
                SlideData[nSlide].field_4a = -1;
            }

            if (sRunChannels[nChannel].c && sRunChannels[nChannel].c != 1) {
                return;
            }

            SlideData[nSlide].field_4a = runlist_AddRunRec(NewRun, RunData[nRun].nMoves);

            if (SlideData[nSlide].field_8a != sRunChannels[nChannel].c)
            {
                D3PlayFX(StaticSound[kSound23], SlideData[nSlide].nSprite);
                SlideData[nSlide].field_8a = sRunChannels[nChannel].c;
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
                movesprite(SlideData[nSlide].nSprite, var_34 << 14, var_2C << 14, 0, 0, 0, CLIPMASK1);

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
                runlist_SubRunRec(SlideData[nSlide].field_4a);

                SlideData[nSlide].field_4a = -1;
                D3PlayFX(StaticSound[nStopSound], SlideData[nSlide].nSprite);

                runlist_ReadyChannel(nChannel);
            }

            return;
        }
    }
}

int BuildTrap(int nSprite, int edx, int ebx, int ecx)
{
	auto pSprite = &sprite[nSprite];
    int var_14 = edx;
    int var_18 = ebx;
    int var_10 = ecx;

    int nTrap = sTrap.Reserve(1);

    changespritestat(nSprite, 0);

    pSprite->cstat = 0x8000;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->extra = -1;

    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->hitag = runlist_AddRunRec(NewRun, nTrap | 0x1F0000);
    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, nTrap | 0x1F0000);

//	GrabTimeSlot(3);

    sTrap[nTrap].nSprite = nSprite;
    sTrap[nTrap].nType = (var_14 == 0) + 14;
    sTrap[nTrap].field_0 = -1;

    sTrap[nTrap].nTrapInterval = 64 - (2 * var_10);
    if (sTrap[nTrap].nTrapInterval < 5) {
        sTrap[nTrap].nTrapInterval = 5;
    }

    sTrap[nTrap].field_C = 0;
    sTrap[nTrap].field_A = 0;

    if (var_18 == -1) {
        return nTrap | 0x1F0000;
    }

    sTrap[nTrap].field_6 = -1;
    sTrap[nTrap].field_8 = -1;

    short nSector = pSprite->sectnum;
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
    pSprite->backuppos();

}

void FuncTrap(int a, int, int nRun)
{
    short nTrap = RunData[nRun].nVal;
    short nSprite = sTrap[nTrap].nSprite;
    auto pSprite = &sprite[nSprite];

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
                    sTrap[nTrap].field_0 = sTrap[nTrap].nTrapInterval;

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

                    int nBullet = BuildBullet(nSprite, nType, 0, 0, 0, pSprite->ang, 0, 1);
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
	auto pSprite = &sprite[nSprite];
    int var_14 = insertsprite(pSprite->sectnum, 0);

    if (var_14 < 0) {
        return -1;
    }
    auto spr = &sprite[var_14];

    assert(var_14 < kMaxSprites);

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
    spr->owner = runlist_AddRunRec(spr->lotag - 1, var_14 | 0x260000);
    spr->hitag = runlist_AddRunRec(NewRun, var_14 | 0x260000);

    return var_14;
}

void FuncSpark(int a, int, int nRun)
{
    int nSprite = RunData[nRun].nVal;
	auto pSprite = &sprite[nSprite];

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    int nMessage = a & 0x7F0000;

    if (nMessage != 0x20000) {
        return;
    }

    pSprite->shade += 3;
    pSprite->xrepeat -= 2;

    if (pSprite->xrepeat >= 4 && pSprite->shade <= 100)
    {
        pSprite->yrepeat -= 2;

        // calling BuildSpark() with 2nd parameter as '1' will set kTile986
        if (pSprite->picnum == kTile986 && (pSprite->xrepeat & 2))
        {
            BuildSpark(nSprite, 2);
        }

        if (pSprite->picnum >= kTile3000) {
            return;
        }

        pSprite->zvel += 128;

        int nMov = movesprite(nSprite, pSprite->xvel << 12, pSprite->yvel << 12, pSprite->zvel, 2560, -2560, CLIPMASK1);
        if (!nMov) {
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

    int const nSprite = insertsprite(nSector, 406);
    auto spr = &sprite[nSprite];


    short nextsector = wall[startwall].nextsector;

    spr->x = xAvg;
    spr->y = yAvg;

    sector[nSector].extra = nSprite;

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
    spr->owner = runlist_AddRunRec(spr->lotag - 1, nSprite | 0x250000);
    spr->backuppos();

    nEnergyBlocks++;

    return nSprite | 0x250000;
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
            int i;
            StatIterator it(v1);
            while ((i = it.NextIndex()) >= 0)
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
	auto pSprite = &sprite[nSprite];

    short nSector = pSprite->sectnum;

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
    sector[nSector].floorz = pSprite->z;

    pSprite->z = (pSprite->z + sector[nSector].floorz) / 2;

    BuildSpark(nSprite, 3);

    pSprite->cstat = 0;
    pSprite->xrepeat = 100;

    PlayFX2(StaticSound[kSound78], nSprite);

    pSprite->xrepeat = 0;

    nEnergyTowers--;

    for (i = 0; i < 20; i++)
    {
        pSprite->ang = RandomSize(11);
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
    int const nSprite = RunData[nRun].nVal;
    auto spr = &sprite[nSprite];

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
            short nSector = spr->sectnum;

            if (sector[nSector].extra == -1) {
                return;
            }

            int nFloorZ = sector[nSector].floorz;

            sector[nSector].floorz = spr->z;
            spr->z -= 256;

            nDamage = runlist_CheckRadialDamage(nSprite);

            // restore previous values
            sector[nSector].floorz = nFloorZ;
            spr->z += 256;

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

            if (nDamage < spr->xrepeat)
            {
                spr->xrepeat -= nDamage;

                int nSprite2 = insertsprite(lasthitsect, 0);
                auto pSprite2 = &sprite[nSprite2];

                pSprite2->ang = a & 0xFFFF;
                pSprite2->x = lasthitx;
                pSprite2->y = lasthity;
                pSprite2->z = lasthitz;

                BuildSpark(nSprite2, 0); // shoot out blue orb when damaged
                mydeletesprite(nSprite2);
            }
            else
            {
                spr->xrepeat = 0; // using xrepeat to store health
                ExplodeEnergyBlock(nSprite);
            }

            return;
        }
    }
}

int BuildObject(int const nSprite, int nOjectType, int nHitag)
{
    auto spr = &sprite[nSprite];

    auto nObject = ObjectList.Reserve(1);

    changespritestat(nSprite, ObjectStatnum[nOjectType]);

    // 0x7FFD to ensure set as blocking ('B' and 'H') sprite and also disable translucency and set not invisible
    spr->cstat = (spr->cstat | 0x101) & 0x7FFD;
    spr->xvel = 0;
    spr->yvel = 0;
    spr->zvel = 0;
    spr->extra = -1;
    spr->lotag = runlist_HeadRun() + 1;
    spr->hitag = 0;
    spr->owner = runlist_AddRunRec(spr->lotag - 1, nObject | 0x170000);

//	GrabTimeSlot(3);

    if (spr->statnum == kStatDestructibleSprite) {
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

        int nSprite2 = insertsprite(spr->sectnum, 0);
        auto pSprite2 = &sprite[nSprite2];
        ObjectList[nObject].field_10 = nSprite2;

        pSprite2->cstat = 0x8000;
        pSprite2->x = spr->x;
        pSprite2->y = spr->y;
        pSprite2->z = spr->z;
    }
    else
    {
        ObjectList[nObject].field_0 = 0;
        ObjectList[nObject].field_8 = -1;

        if (spr->statnum == kStatDestructibleSprite) {
            ObjectList[nObject].field_10 = -1;
        }
        else {
            ObjectList[nObject].field_10 = -nHitag;
        }
    }
    spr->backuppos();

    return nObject | 0x170000;
}

// in-game destructable wall mounted screen
void ExplodeScreen(short nSprite)
{
	auto pSprite = &sprite[nSprite];
    pSprite->z -= GetSpriteHeight(nSprite) / 2;

    for (int i = 0; i < 30; i++) {
        BuildSpark(nSprite, 0); // shoot out blue orbs
    }

    pSprite->cstat = 0x8000;
    PlayFX2(StaticSound[kSound78], nSprite);
}

void FuncObject(int a, int b, int nRun)
{
    short nObject = RunData[nRun].nVal;

    short nSprite = ObjectList[nObject].nSprite;
	auto pSprite = &sprite[nSprite];
    short nStat = pSprite->statnum;
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
            if (ObjectList[nObject].nHealth > 0 && pSprite->cstat & 0x101
                && (nStat != kStatExplodeTarget
                    || sprite[nRadialSpr].statnum == 201
                    || (nRadialBullet != 3 && nRadialBullet > -1)
                    || sprite[nRadialSpr].statnum == kStatExplodeTrigger))
            {
                int nDamage = runlist_CheckRadialDamage(nSprite);
                if (nDamage <= 0) {
                    return;
                }

                if (pSprite->statnum != kStatAnubisDrum) {
                    ObjectList[nObject].nHealth -= nDamage;
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

                if (ObjectList[nObject].nHealth > 0) {
                    return;
                }

                if (pSprite->statnum == kStatExplodeTarget)
                {
                    ObjectList[nObject].nHealth = -1;
                    short ax = ObjectList[nObject].field_10;

                    if (ax < 0 || ObjectList[ax].nHealth <= 0) {
                        return;
                    }

                    ObjectList[ax].nHealth = -1;
                }
                else if (pSprite->statnum == kStatDestructibleSprite)
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
            if (nStat == 97 || (!(pSprite->cstat & 0x101))) {
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

                pSprite->picnum = seq_GetSeqPicnum2(bx, ObjectList[nObject].field_0);
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
                    int nMov = movesprite(nSprite, pSprite->xvel << 6, pSprite->yvel << 6, pSprite->zvel, 0, 0, CLIPMASK0);

                    if (pSprite->statnum == kStatExplodeTrigger) {
                        pSprite->pal = 1;
                    }

                    if (nMov & 0x20000)
                    {
                        pSprite->xvel -= pSprite->xvel >> 3;
                        pSprite->yvel -= pSprite->yvel >> 3;
                    }

                    if (((nMov & 0xC000) > 0x8000) && ((nMov & 0xC000) == 0xC000))
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
                if ((nStat == kStatExplodeTarget) || (pSprite->z < sector[pSprite->sectnum].floorz))
                {
                    var_18 = 36;
                }
                else
                {
                    var_18 = 34;
                }

                AddFlash(pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 128);
                BuildAnim(-1, var_18, 0, pSprite->x, pSprite->y, sector[pSprite->sectnum].floorz, pSprite->sectnum, 240, 4);

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

                if (!(currentLevel->gameflags & LEVEL_EX_MULTI) || nStat != kStatExplodeTrigger)
                {
                    runlist_SubRunRec(pSprite->owner);
                    runlist_SubRunRec(ObjectList[nObject].field_4);

                    mydeletesprite(nSprite);
                    return;
                }
                else
                {
                    StartRegenerate(nSprite);
                    ObjectList[nObject].nHealth = 120;

                    pSprite->x = sprite[ObjectList[nObject].field_10].x;
                    pSprite->y = sprite[ObjectList[nObject].field_10].y;
                    pSprite->z = sprite[ObjectList[nObject].field_10].z;

                    mychangespritesect(nSprite, sprite[ObjectList[nObject].field_10].sectnum);
                    return;
                }
            }
        }
    }
}

void BuildDrip(int nSprite)
{
	auto pSprite = &sprite[nSprite];
    auto nDrips = sDrip.Reserve(1);
    sDrip[nDrips].nSprite = nSprite;
    sDrip[nDrips].field_2 = RandomSize(8) + 90;
    pSprite->cstat = 0x8000u;
}

void DoDrips()
{
    for (unsigned i = 0; i < sDrip.Size(); i++)
    {
        sDrip[i].field_2--;
        if (sDrip[i].field_2 <= 0)
        {
            short nSprite = sDrip[i].nSprite;
			auto pSprite = &sprite[nSprite];

            short nSeqOffset = SeqOffsets[kSeqDrips];

            if (!(SectFlag[pSprite->sectnum] & kSectLava)) {
                nSeqOffset++;
            }

            seq_MoveSequence(nSprite, nSeqOffset, RandomSize(2) % SeqSize[nSeqOffset]);

            sDrip[i].field_2 = RandomSize(8) + 90;
        }
    }

    for (unsigned i = 0; i < sBob.Size(); i++)
    {
        sBob[i].field_2 += 4;

        int edx = bsin(sBob[i].field_2 << 3, -4);
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
    StartInterpolation(nSector, bx == 0? Interp_Sect_Floorz : Interp_Sect_Ceilingz);

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
void ProcessTrailSprite(int nSprite, int nLotag, int nHitag)
{
	auto pSprite = &sprite[nSprite];
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

    mydeletesprite(nSprite);
}

// ok?
void AddMovingSector(int nSector, int edx, int ebx, int ecx)
{
    CreatePushBlock(nSector);
    setsectinterpolate(nSector);

    short nTrail = FindTrail(ebx);


    auto nMoveSects = sMoveSect.Reserve(1);
    MoveSect *pMoveSect = &sMoveSect[nMoveSects];

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
    int i,j;

    for (unsigned i = 0; i < sMoveSect.Size(); i++)
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
					auto pSprite = &sprite[nSprite];
                    pSprite->cstat = 0x8000;
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

    for (unsigned i = 0; i < ObjectList.Size(); i++)
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

                for (unsigned j = 0; j < ObjectList.Size(); j++)
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

END_PS_NS

