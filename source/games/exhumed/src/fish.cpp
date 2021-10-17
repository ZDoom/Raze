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
#include "aistuff.h"
#include "engine.h"
#include "sequence.h"
#include "exhumed.h"
#include "sound.h"
#include <assert.h>

BEGIN_PS_NS

static actionSeq FishSeq[] = {
    {8, 0},
    {8, 0},
    {0, 0},
    {24, 0},
    {8, 0},
    {32, 1},
    {33, 1},
    {34, 1},
    {35, 1},
    {39, 1}
};


struct Fish
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nTarget;
    short nCount;
    short nRun;
};

struct Chunk
{
    short nSprite;
    short nIndex;
    short nSeqIndex;
};

TArray<Fish> FishList;
TArray<Chunk> FishChunk;

FSerializer& Serialize(FSerializer& arc, const char* keyname, Fish& w, Fish* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("health", w.nHealth)
            ("frame", w.nFrame)
            ("action", w.nAction)
            ("sprite", w.nSprite)
            ("target", w.nTarget)
            ("run", w.nRun)
            ("count", w.nCount)
            .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, Chunk& w, Chunk* def)
{
    if (arc.BeginObject(keyname))
    {
        arc ("sprite", w.nSprite)
            ("index", w.nIndex)
            ("seqindex", w.nSeqIndex)
            .EndObject();
    }
    return arc;
}

void SerializeFish(FSerializer& arc)
{
    arc("fish", FishList)
        ("fishchunk", FishChunk);
}

void InitFishes()
{
    FishList.Clear();
    FishChunk.Clear();
}

void BuildFishLimb(short nFish, short edx)
{
    short nSprite = FishList[nFish].nSprite;
	auto pSprite = &sprite[nSprite];

    int nFree = FishChunk.Reserve(1);

    int nSprite2 = insertsprite(pSprite->sectnum, 99);
    assert(nSprite2 >= 0 && nSprite2 < kMaxSprites);
	auto pSprite2 = &sprite[nSprite2];

    FishChunk[nFree].nSprite = nSprite2;
    FishChunk[nFree].nSeqIndex = edx + 40;
    FishChunk[nFree].nIndex = RandomSize(3) % SeqSize[SeqOffsets[kSeqFish] + edx + 40];

    pSprite2->x = pSprite->x;
    pSprite2->y = pSprite->y;
    pSprite2->z = pSprite->z;
    pSprite2->cstat = 0;
    pSprite2->shade = -12;
    pSprite2->pal = 0;
    pSprite2->xvel = (RandomSize(5) - 16) << 8;
    pSprite2->yvel = (RandomSize(5) - 16) << 8;
    pSprite2->xrepeat = 64;
    pSprite2->yrepeat = 64;
    pSprite2->xoffset = 0;
    pSprite2->yoffset = 0;
    pSprite2->zvel = (-(RandomByte() + 512)) * 2;

    seq_GetSeqPicnum(kSeqFish, FishChunk[nFree].nSeqIndex, 0);

    pSprite2->picnum = edx;
    pSprite2->lotag = runlist_HeadRun() + 1;
    pSprite2->clipdist = 0;

//	GrabTimeSlot(3);

    pSprite2->extra = -1;
    pSprite2->owner = runlist_AddRunRec(pSprite2->lotag - 1, nFree, 0x200000);
    pSprite2->hitag = runlist_AddRunRec(NewRun, nFree, 0x200000);
}

void BuildBlood(int x, int y, int z, short nSector)
{
    BuildAnim(-1, kSeqFish, 36, x, y, z, nSector, 75, 128);
}

void AIFishLimb::Tick(RunListEvent* ev)
{
    short nFish = RunData[ev->nRun].nObjIndex;
    short nSprite = FishChunk[nFish].nSprite;
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    auto pSprite = &sprite[nSprite];

    int nSeq = SeqOffsets[kSeqFish] + FishChunk[nFish].nSeqIndex;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, FishChunk[nFish].nIndex);

    Gravity(nSprite);

    FishChunk[nFish].nIndex++;

    if (FishChunk[nFish].nIndex >= SeqSize[nSeq])
    {
        FishChunk[nFish].nIndex = 0;
        if (RandomBit()) {
            BuildBlood(pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum);
        }
    }

    int FloorZ = sector[pSprite->sectnum].floorz;

    if (FloorZ <= pSprite->z)
    {
        pSprite->z += 256;

        if ((pSprite->z - FloorZ) > 25600)
        {
            pSprite->zvel = 0;
            runlist_DoSubRunRec(pSprite->owner);
            runlist_FreeRun(pSprite->lotag - 1);
            runlist_SubRunRec(pSprite->hitag);
            mydeletesprite(nSprite);
        }
        else if ((pSprite->z - FloorZ) > 0)
        {
            pSprite->zvel = 1024;
        }

        return;
    }
    else
    {
        if (movesprite(nSprite, pSprite->xvel << 8, pSprite->yvel << 8, pSprite->zvel, 2560, -2560, CLIPMASK1))
        {
            pSprite->xvel = 0;
            pSprite->yvel = 0;
        }
    }

}

void AIFishLimb::Draw(RunListEvent* ev)
{
    short nFish = RunData[ev->nRun].nObjIndex;
    int nSeq = SeqOffsets[kSeqFish] + FishChunk[nFish].nSeqIndex;
    seq_PlotSequence(ev->nParam, nSeq, FishChunk[nFish].nIndex, 1);
}


void  FuncFishLimb(int nObject, int nMessage, int nDamage, int nRun)
{
    AIFishLimb ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void BuildFish(int nSprite, int x, int y, int z, int nSector, int nAngle)
{
    int nFish = FishList.Reserve(1);
	auto pSprite = &sprite[nSprite];

    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 103);
		pSprite = &sprite[nSprite];
    }
    else
    {
        x = pSprite->x;
        y = pSprite->y;
        z = pSprite->z;
        nAngle = pSprite->ang;
        changespritestat(nSprite, 103);
    }

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    pSprite->x = x;
    pSprite->y = y;
    pSprite->z = z;
    pSprite->cstat = 0x101;
    pSprite->shade = -12;
    pSprite->clipdist = 80;
    pSprite->xrepeat = 40;
    pSprite->yrepeat = 40;
    pSprite->pal = sector[pSprite->sectnum].ceilingpal;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->picnum = seq_GetSeqPicnum(kSeqFish, FishSeq[0].a, 0);
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->ang = nAngle;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->hitag = 0;
    pSprite->extra = -1;

//	GrabTimeSlot(3);

    FishList[nFish].nAction = 0;
    FishList[nFish].nHealth = 200;
    FishList[nFish].nSprite = nSprite;
    FishList[nFish].nTarget = -1;
    FishList[nFish].nCount = 60;
    FishList[nFish].nFrame = 0;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, nFish, 0x120000);
    FishList[nFish].nRun = runlist_AddRunRec(NewRun, nFish, 0x120000);

    nCreaturesTotal++;
}

void IdleFish(short nFish, short edx)
{
    short nSprite = FishList[nFish].nSprite;
	auto pSprite = &sprite[nSprite];

    pSprite->ang += (256 - RandomSize(9)) + 1024;
    pSprite->ang &= kAngleMask;

    pSprite->xvel = bcos(pSprite->ang, -8);
    pSprite->yvel = bsin(pSprite->ang, -8);

    FishList[nFish].nAction = 0;
    FishList[nFish].nFrame = 0;

    pSprite->zvel = RandomSize(9);

    if (!edx)
    {
        if (RandomBit()) {
            pSprite->zvel = -pSprite->zvel;
        }
    }
    else if (edx < 0)
    {
        pSprite->zvel = -pSprite->zvel;
    }
}

void DestroyFish(short nFish)
{
    short nSprite = FishList[nFish].nSprite;
	auto pSprite = &sprite[nSprite];

    runlist_DoSubRunRec(pSprite->owner);
    runlist_FreeRun(pSprite->lotag - 1);
    runlist_SubRunRec(FishList[nFish].nRun);
    mydeletesprite(nSprite);
}



void AIFish::Draw(RunListEvent* ev)
{
    short nFish = RunData[ev->nRun].nObjIndex;
    assert(nFish >= 0 && nFish < (int)FishList.Size());
    short nAction = FishList[nFish].nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqFish] + FishSeq[nAction].a, FishList[nFish].nFrame, FishSeq[nAction].b);
    ev->pTSprite->owner = -1;
    return;
}

void AIFish::RadialDamage(RunListEvent* ev)
{
    short nFish = RunData[ev->nRun].nObjIndex;
    short nSprite = FishList[nFish].nSprite;

    if (FishList[nFish].nHealth <= 0) {
        return;
    }
    else
    {
        ev->nDamage = runlist_CheckRadialDamage(nSprite);
        if (!ev->nDamage) {
            return;
        }

        FishList[nFish].nCount = 10;
    }
    // fall through
    Damage(ev);
}

void AIFish::Damage(RunListEvent* ev)
{
    short nFish = RunData[ev->nRun].nObjIndex;
    assert(nFish >= 0 && nFish < (int)FishList.Size());
    short nAction = FishList[nFish].nAction;
    short nSprite = FishList[nFish].nSprite;
    auto pSprite = &sprite[nSprite];

    if (!ev->nDamage) {
        return;
    }

    FishList[nFish].nHealth -= dmgAdjust(ev->nDamage);
    if (FishList[nFish].nHealth <= 0)
    {
        FishList[nFish].nHealth = 0;
        nCreaturesKilled++;

        pSprite->cstat &= 0xFEFE;

        if (ev->nMessage == EMessageType::Damage)
        {
            for (int i = 0; i < 3; i++)
            {
                BuildFishLimb(nFish, i);
            }

            PlayFXAtXYZ(StaticSound[kSound40], pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum);
            DestroyFish(nFish);
        }
        else
        {
            FishList[nFish].nAction = 9;
            FishList[nFish].nFrame = 0;
        }

        return;
    }
    else
    {
        short nTarget = ev->nParam;
        if (nTarget >= 0 && sprite[nTarget].statnum < 199)
        {
            FishList[nFish].nTarget = nTarget;
        }

        FishList[nFish].nAction = 4;
        FishList[nFish].nFrame = 0;
        FishList[nFish].nCount += 10;
    }
}

void AIFish::Tick(RunListEvent* ev)
{
    short nFish = RunData[ev->nRun].nObjIndex;
    assert(nFish >= 0 && nFish < (int)FishList.Size());
    short nAction = FishList[nFish].nAction;
    short nSprite = FishList[nFish].nSprite;
    auto pSprite = &sprite[nSprite];

    if (!(SectFlag[pSprite->sectnum] & kSectUnderwater))
    {
        Gravity(nSprite);
    }

    short nSeq = SeqOffsets[kSeqFish] + FishSeq[nAction].a;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, FishList[nFish].nFrame);

    seq_MoveSequence(nSprite, nSeq, FishList[nFish].nFrame);

    FishList[nFish].nFrame++;
    if (FishList[nFish].nFrame >= SeqSize[nSeq]) {
        FishList[nFish].nFrame = 0;
    }

    short nTarget = FishList[nFish].nTarget;

    switch (nAction)
    {
    default:
        return;

    case 0:
    {
        FishList[nFish].nCount--;
        if (FishList[nFish].nCount <= 0)
        {
            nTarget = FindPlayer(nSprite, 60);
            if (nTarget >= 0)
            {
                FishList[nFish].nTarget = nTarget;
                FishList[nFish].nAction = 2;
                FishList[nFish].nFrame = 0;

                int nAngle = GetMyAngle(sprite[nTarget].x - pSprite->x, sprite[nTarget].z - pSprite->z);
                pSprite->zvel = bsin(nAngle, -5);

                FishList[nFish].nCount = RandomSize(6) + 90;
            }
            else
            {
                IdleFish(nFish, 0);
            }
        }

        break;
    }

    case 1:
        return;

    case 2:
    case 3:
    {
        FishList[nFish].nCount--;
        if (FishList[nFish].nCount <= 0)
        {
            IdleFish(nFish, 0);
            return;
        }
        else
        {
            PlotCourseToSprite(nSprite, nTarget);
            int nHeight = GetSpriteHeight(nSprite) >> 1;

            int z = abs(sprite[nTarget].z - pSprite->z);

            if (z <= nHeight)
            {
                pSprite->xvel = bcos(pSprite->ang, -5) - bcos(pSprite->ang, -7);
                pSprite->yvel = bsin(pSprite->ang, -5) - bsin(pSprite->ang, -7);
            }
            else
            {
                pSprite->xvel = 0;
                pSprite->yvel = 0;
            }

            pSprite->zvel = (sprite[nTarget].z - pSprite->z) >> 3;
        }
        break;
    }

    case 4:
    {
        if (FishList[nFish].nFrame == 0)
        {
            IdleFish(nFish, 0);
        }
        return;
    }

    case 8:
    {
        return;
    }

    case 9:
    {
        if (FishList[nFish].nFrame == 0)
        {
            DestroyFish(nFish);
        }
        return;
    }
    }

    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;
    short nSector = pSprite->sectnum;

    // loc_2EF54
    int nMov = movesprite(nSprite, pSprite->xvel << 13, pSprite->yvel << 13, pSprite->zvel << 2, 0, 0, CLIPMASK0);

    if (!(SectFlag[pSprite->sectnum] & kSectUnderwater))
    {
        mychangespritesect(nSprite, nSector);
        pSprite->x = x;
        pSprite->y = y;
        pSprite->z = z;

        IdleFish(nFish, 0);
        return;
    }
    else
    {
        if (nAction >= 5) {
            return;
        }

        if (!nMov)
        {
            if (nAction == 3)
            {
                FishList[nFish].nAction = 2;
                FishList[nFish].nFrame = 0;
            }
            return;
        }

        if ((nMov & 0x30000) == 0)
        {
            if ((nMov & 0xC000) == 0x8000)
            {
                IdleFish(nFish, 0);
            }
            else if ((nMov & 0xC000) == 0xC000)
            {
                if (sprite[nMov & 0x3FFF].statnum == 100)
                {
                    FishList[nFish].nTarget = nMov & 0x3FFF;
                    pSprite->ang = GetMyAngle(sprite[nTarget].x - pSprite->x, sprite[nTarget].y - pSprite->y);

                    if (nAction != 3)
                    {
                        FishList[nFish].nAction = 3;
                        FishList[nFish].nFrame = 0;
                    }

                    if (!FishList[nFish].nFrame)
                    {
                        runlist_DamageEnemy(nTarget, nSprite, 2);
                    }
                }
            }
        }
        else if (nMov & 0x20000)
        {
            IdleFish(nFish, -1);
        }
        else
        {
            IdleFish(nFish, 1);
        }
    }
}


void  FuncFish(int nObject, int nMessage, int nDamage, int nRun)
{
    AIFish ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

END_PS_NS
