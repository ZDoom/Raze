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
#include "exhumed.h"
#include "engine.h"
#include "sequence.h"
#include "player.h"
#include "sound.h"
#include "names.h"
#include <assert.h>

BEGIN_PS_NS

enum
{
    kMaxQueens = 1,
    kMaxEggs = 10,
    kMaxTails = 7
};

short QueenCount = 0;

static actionSeq QueenSeq[] = {
    {0,  0},
    {0,  0},
    {9,  0},
    {36, 0},
    {18, 0},
    {27, 0},
    {45, 0},
    {45, 0},
    {54, 1},
    {53, 1},
    {55, 1}
};

static actionSeq HeadSeq[] = {
    {56, 1},
    {65, 0},
    {65, 0},
    {65, 0},
    {65, 0},
    {65, 0},
    {74, 0},
    {82, 0},
    {90, 0}
};

static actionSeq EggSeq[] = {
    {19, 1},
    {18, 1},
    {0,  0},
    {9,  0},
    {23, 1},
};

struct Queen
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nTarget;
    short field_A;
    short field_C;
    short field_10;
    short field_12;
};

struct Egg
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nRunPtr;
    short nTarget;
    short field_C;
    short field_E;
};

struct Head
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short field_8;
    short nTarget;
    short field_C;
    short tails;
};

FreeListArray<Egg, kMaxEggs> QueenEgg;

int nQHead = 0;

short nHeadVel;
short nVelShift;

short tailspr[kMaxTails];

short QueenChan[kMaxQueens];

Queen QueenList[kMaxQueens];
Head QueenHead;

int MoveQX[25];
int MoveQY[25];
int MoveQZ[25];
short MoveQS[25];
short MoveQA[25];

FSerializer& Serialize(FSerializer& arc, const char* keyname, Queen& w, Queen* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("health", w.nHealth)
            ("frame", w.nFrame)
            ("action", w.nAction)
            ("sprite", w.nSprite)
            ("target", w.nTarget)
            ("ata", w.field_A)
            ("atc", w.field_C)
            ("at10", w.field_10)
            ("at12", w.field_12)
            .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, Egg& w, Egg* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("health", w.nHealth)
            ("frame", w.nFrame)
            ("action", w.nAction)
            ("sprite", w.nSprite)
            ("target", w.nTarget)
            ("runptr", w.nRunPtr)
            ("atc", w.field_C)
            ("ate", w.field_E)
            .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, Head& w, Head* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("health", w.nHealth)
            ("frame", w.nFrame)
            ("action", w.nAction)
            ("sprite", w.nSprite)
            ("target", w.nTarget)
            ("at8", w.field_8)
            ("atc", w.field_C)
            ("tails", w.tails)
            .EndObject();
    }
    return arc;
}

void SerializeQueen(FSerializer& arc)
{
    if (arc.BeginObject("queen"))
    {
        arc("count", QueenCount);
        if (QueenCount == 0) // only save the rest if we got a queen. There can only be one.
        {
            for (int i = 0; i < kMaxEggs; i++)
            {
                QueenEgg[i].nRunPtr = -1;
            }
            arc("qhead", nQHead)
                ("headvel", nHeadVel)
                ("velshift", nVelShift)
                ("head", QueenHead)
                .Array("tailspr", tailspr, countof(tailspr))
                ("queenchan", QueenChan[0])
                ("queen", QueenList[0])
                ("eggs", QueenEgg)
                .Array("moveqx", MoveQX, countof(MoveQX))
                .Array("moveqy", MoveQY, countof(MoveQY))
                .Array("moveqz", MoveQZ, countof(MoveQZ))
                .Array("moveqa", MoveQA, countof(MoveQA))
                .Array("moveqs", MoveQS, countof(MoveQS));
        }
        arc.EndObject();
    }
}

void InitQueens()
{
    QueenCount = 1;
    QueenEgg.Clear();
    for (int i = 0; i < kMaxEggs; i++)
    {
        QueenEgg[i].nRunPtr = -1;
    }
}

int GrabEgg()
{
    auto egg = QueenEgg.Get();
    if (egg == -1) return -1;
    return egg;
}

void BlowChunks(int nSprite)
{
    for (int i = 0; i < 4; i++)
    {
        BuildCreatureChunk(nSprite, seq_GetSeqPicnum(16, i + 41, 0));
    }
}

void DestroyEgg(short nEgg)
{
    short nSprite = QueenEgg[nEgg].nSprite;
    auto pSprite = &sprite[nSprite];

    if (QueenEgg[nEgg].nAction != 4)
    {
        BuildAnim(nullptr, 34, 0, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, pSprite->xrepeat, 4);
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            BuildCreatureChunk(nSprite, seq_GetSeqPicnum(kSeqQueenEgg, (i % 2) + 24, 0));
        }
    }

    runlist_DoSubRunRec(pSprite->owner);
    runlist_DoSubRunRec(pSprite->lotag - 1);
    runlist_SubRunRec(QueenEgg[nEgg].nRunPtr);

    QueenEgg[nEgg].nRunPtr = -1;

    mydeletesprite(nSprite);
    QueenEgg.Release(nEgg);
}

void DestroyAllEggs()
{
    for (int i = 0; i < kMaxEggs; i++)
    {
        if (QueenEgg[i].nRunPtr > -1)
        {
            DestroyEgg(i);
        }
    }
}

void SetHeadVel(short nSprite)
{
    auto pSprite = &sprite[nSprite];
    short nAngle = pSprite->ang;

    pSprite->xvel = bcos(nAngle, nVelShift);
    pSprite->yvel = bsin(nAngle, nVelShift);
}

int QueenAngleChase(short nSprite, short nSprite2, int val1, int val2)
{
    short nAngle;

    spritetype* pSprite = &sprite[nSprite];
    if (nSprite2 < 0)
    {
        pSprite->zvel = 0;
        nAngle = pSprite->ang;
    }
    else
    {
        spritetype* pSprite2 = &sprite[nSprite2];
        int nTileY = (tileHeight(pSprite2->picnum) * pSprite2->yrepeat) * 2;

        int nMyAngle = GetMyAngle(pSprite2->x - pSprite->x, pSprite2->y - pSprite->y);

        int edx = ((pSprite2->z - nTileY) - pSprite->z) >> 8;

        uint32_t xDiff = abs(pSprite2->x - pSprite->x);
        uint32_t yDiff = abs(pSprite2->y - pSprite->y);

        uint32_t sqrtVal = xDiff * xDiff + yDiff * yDiff;

        if (sqrtVal > INT_MAX)
        {
            DPrintf(DMSG_WARNING, "%s %d: overflow\n", __func__, __LINE__);
            sqrtVal = INT_MAX;
        }

        int nSqrt = ksqrt(sqrtVal);

        int var_14 = GetMyAngle(nSqrt, edx);

        int nAngDelta = AngleDelta(pSprite->ang, nMyAngle, 1024);

        if (abs(nAngDelta) > 127)
        {
            val1 /= abs(nAngDelta >> 7);
            if (val1 < 256)
                val1 = 256;
        }

        if (abs(nAngDelta) > val2)
        {
            if (nAngDelta < 0)
                nAngDelta = -val2;
            else
                nAngDelta = val2;
        }

        nAngle = (nAngDelta + pSprite->ang) & kAngleMask;

        pSprite->zvel = (AngleDelta(pSprite->zvel, var_14, 24) + pSprite->zvel) & kAngleMask;
    }

    pSprite->ang = nAngle;

    int da = pSprite->zvel;
    int x = abs(bcos(da));

    int v26 = x * ((val1 * bcos(nAngle)) >> 14);
    int v27 = x * ((val1 * bsin(nAngle)) >> 14);

    uint32_t xDiff = abs((int32_t)(v26 >> 8));
    uint32_t yDiff = abs((int32_t)(v27 >> 8));

    uint32_t sqrtNum = xDiff * xDiff + yDiff * yDiff;

    if (sqrtNum > INT_MAX)
    {
        DPrintf(DMSG_WARNING, "%s %d: overflow\n", __func__, __LINE__);
        sqrtNum = INT_MAX;
    }

    int nSqrt = ksqrt(sqrtNum) * bsin(da);

    return movesprite(nSprite, v26 >> 2, v27 >> 2, bsin(bobangle, -5) + (nSqrt >> 13), 0, 0, CLIPMASK1);
}

int DestroyTailPart()
{
    if (!QueenHead.tails) {
        return 0;
    }

    short nSprite = tailspr[--QueenHead.tails];

    BlowChunks(nSprite);
    BuildExplosion(&exhumedActors[nSprite]);

    for (int i = 0; i < 5; i++)
    {
        short nHeight = GetSpriteHeight(nSprite);
        BuildLavaLimb(&exhumedActors[nSprite], i, nHeight);
    }

    mydeletesprite(nSprite);
    return 1;
}

void BuildTail()
{
    short nSprite = QueenHead.nSprite;
    auto pSprite = &sprite[nSprite];

    int x = pSprite->x;
    int y = pSprite->x;
    int z = pSprite->x;
    short nSector = pSprite->sectnum;

    int i;

    for (i = 0; i < kMaxTails; i++)
    {
        short nTailSprite = insertsprite(nSector, 121);
        auto pTailSprite = &sprite[nTailSprite];
        tailspr[i] = nTailSprite;

        if (nTailSprite < 0) {
            I_Error("Can't create queen's tail!\n");
        }

        pTailSprite->lotag = runlist_HeadRun() + 1;
        pTailSprite->owner = runlist_AddRunRec(sprite[nTailSprite].lotag - 1, (i + 1), 0x1B0000);
        pTailSprite->shade = -12;
        pTailSprite->x = x;
        pTailSprite->y = y;
        pTailSprite->hitag = 0;
        pTailSprite->cstat = 0;
        pTailSprite->clipdist = 100;
        pTailSprite->xrepeat = 80;
        pTailSprite->yrepeat = 80;
        pTailSprite->picnum = 1;
        pTailSprite->pal = sector[sprite[nTailSprite].sectnum].ceilingpal;
        pTailSprite->xoffset = 0;
        pTailSprite->yoffset = 0;
        pTailSprite->z = z;
        pTailSprite->extra = -1;
    }

    for (i = 0; i < 24 + 1; i++)
    {
        MoveQX[i] = x;
        MoveQZ[i] = z;
        MoveQY[i] = y;
        assert(nSector >= 0 && nSector < kMaxSectors);
        MoveQS[i] = nSector;
    }

    nQHead = 0;
    QueenHead.tails = 7;
}

void BuildQueenEgg(short nQueen, int nVal)
{
    int nEgg = GrabEgg();
    if (nEgg < 0) {
        return;
    }

    short nSprite = QueenList[nQueen].nSprite;
    auto pSprite = &sprite[nSprite];

    int x = pSprite->x;
    int y = pSprite->y;
    short nSector = pSprite->sectnum;
    int nFloorZ = sector[nSector].floorz;
    short nAngle = pSprite->ang;

    int nSprite2 = insertsprite(nSector, 121);
    assert(nSprite2 >= 0 && nSprite2 < kMaxSprites);
    auto pSprite2 = &sprite[nSprite2];

    pSprite2->x = x;
    pSprite2->y = y;
    pSprite2->z = nFloorZ;
    pSprite2->pal = 0;
    pSprite2->clipdist = 50;
    pSprite2->xoffset = 0;
    pSprite2->yoffset = 0;
    pSprite2->shade = -12;
    pSprite2->picnum = 1;
    pSprite2->ang = (RandomSize(9) + (nAngle - 256)) & kAngleMask;
    pSprite2->backuppos();

    if (!nVal)
    {
        pSprite2->xrepeat = 30;
        pSprite2->yrepeat = 30;
        pSprite2->xvel = bcos(pSprite2->ang);
        pSprite2->yvel = bsin(pSprite2->ang);
        pSprite2->zvel = -6000;
        pSprite2->cstat = 0;
    }
    else
    {
        pSprite2->xrepeat = 60;
        pSprite2->yrepeat = 60;
        pSprite2->xvel = 0;
        pSprite2->yvel = 0;
        pSprite2->zvel = -2000;
        pSprite2->cstat = 0x101;
    }

    pSprite2->lotag = runlist_HeadRun() + 1;
    pSprite2->extra = -1;
    pSprite2->hitag = 0;

    GrabTimeSlot(3);

    QueenEgg[nEgg].nHealth = 200;
    QueenEgg[nEgg].nFrame = 0;
    QueenEgg[nEgg].nSprite = nSprite2;
    QueenEgg[nEgg].field_E = nVal;
    QueenEgg[nEgg].nTarget = QueenList[nQueen].nTarget;

    if (nVal)
    {
        nVal = 4;
        QueenEgg[nEgg].field_C = 200;
    }

    QueenEgg[nEgg].nAction = nVal;

    pSprite2->owner = runlist_AddRunRec(pSprite2->lotag - 1, nEgg, 0x1D0000);
    QueenEgg[nEgg].nRunPtr = runlist_AddRunRec(NewRun, nEgg, 0x1D0000);
}

void AIQueenEgg::Tick(RunListEvent* ev)
{
    short nEgg = RunData[ev->nRun].nObjIndex;
    Egg* pEgg = &QueenEgg[nEgg];
    short nSprite = pEgg->nSprite;
    auto pSprite = &sprite[nSprite];
    short nAction = pEgg->nAction;

    short nTarget;

    bool bVal = false;

    if (pEgg->nHealth <= 0)
    {
        DestroyEgg(nEgg);
        return;
    }

    if (nAction == 0 || nAction == 4) {
        Gravity(nSprite);
    }

    short nSeq = SeqOffsets[kSeqQueenEgg] + EggSeq[nAction].a;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pEgg->nFrame);

    if (nAction != 4)
    {
        seq_MoveSequence(nSprite, nSeq, pEgg->nFrame);

        pEgg->nFrame++;
        if (pEgg->nFrame >= SeqSize[nSeq])
        {
            pEgg->nFrame = 0;
            bVal = true;
        }

        nTarget = UpdateEnemy(&pEgg->nTarget);
        pEgg->nTarget = nTarget;

        if (nTarget >= 0 && (sprite[nTarget].cstat & 0x101) == 0)
        {
            pEgg->nTarget = -1;
            pEgg->nAction = 0;
        }
        else
        {
            nTarget = FindPlayer(-nSprite, 1000);
            pEgg->nTarget = nTarget;
        }
    }

    switch (nAction)
    {
    case 0:
    {
        int nMov = MoveCreature(nSprite);
        if (!nMov) {
            break;
        }

        if (nMov & 0x20000)
        {
            if (!RandomSize(1))
            {
                pEgg->nAction = 1;
                pEgg->nFrame = 0;
            }
            else
            {
                DestroyEgg(nEgg);
            }
        }
        else
        {
            short nAngle;

            switch (nMov & 0xC000)
            {
            default:
                return;
            case 0x8000:
                nAngle = GetWallNormal(nMov & 0x3FFF);
                break;
            case 0xC000:
                nAngle = sprite[nMov & 0x3FFF].ang;
                break;
            }

            pSprite->ang = nAngle;
            pSprite->xvel = bcos(nAngle, -1);
            pSprite->yvel = bsin(nAngle, -1);
        }

        break;
    }

    case 1:
    {
        if (bVal)
        {
            pEgg->nAction = 3;
            pSprite->cstat = 0x101;
        }
        break;
    }

    case 2:
    case 3:
    {
        int nMov = QueenAngleChase(nSprite, nTarget, nHeadVel, 64);

        switch (nMov & 0xC000)
        {
        case 0xC000:
            if (sprite[nMov & 0x3FFF].statnum != 121)
            {
                runlist_DamageEnemy(nMov & 0x3FFF, nSprite, 5);
            }
            fallthrough__;
        case 0x8000:
            pSprite->ang += (RandomSize(9) + 768);
            pSprite->ang &= kAngleMask;
            pSprite->xvel = bcos(pSprite->ang, -3);
            pSprite->yvel = bsin(pSprite->ang, -3);
            pSprite->zvel = -RandomSize(5);
            break;
        }

        return;
    }

    case 4:
    {
        int nMov = MoveCreature(nSprite);

        if (nMov & 0x20000)
        {
            pSprite->zvel = -(pSprite->zvel - 256);
            if (pSprite->zvel < -512)
            {
                pSprite->zvel = 0;
            }
        }

        pEgg->field_C--;
        if (pEgg->field_C <= 0)
        {
            auto pWaspSprite = BuildWasp(nullptr, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, pSprite->ang, true);
            pSprite->z = pWaspSprite->s().z;

            DestroyEgg(nEgg);
        }
        break;
    }
    }
}

void AIQueenEgg::RadialDamage(RunListEvent* ev)
{
    short nEgg = RunData[ev->nRun].nObjIndex;
    Egg* pEgg = &QueenEgg[nEgg];
    short nSprite = pEgg->nSprite;
    auto pSprite = &sprite[nSprite];

    if (sprite[nRadialSpr].statnum != 121 && (pSprite->cstat & 0x101) != 0)
    {
        int nDamage = runlist_CheckRadialDamage(nSprite);

        pEgg->nHealth -= nDamage;
    }
}

void AIQueenEgg::Damage(RunListEvent* ev)
{
    short nEgg = RunData[ev->nRun].nObjIndex;
    Egg* pEgg = &QueenEgg[nEgg];

    if (ev->nDamage != 0 && pEgg->nHealth > 0)
    {
        pEgg->nHealth -= ev->nDamage;

        if (pEgg->nHealth <= 0)
            DestroyEgg(nEgg);
    }
}

void AIQueenEgg::Draw(RunListEvent* ev)
{
    short nEgg = RunData[ev->nRun].nObjIndex;
    Egg* pEgg = &QueenEgg[nEgg];
    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqQueenEgg] + EggSeq[pEgg->nAction].a, pEgg->nFrame, EggSeq[pEgg->nAction].b);
}


void FuncQueenEgg(int nObject, int nMessage, int nDamage, int nRun)
{
    AIQueenEgg ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void BuildQueenHead(short nQueen)
{
    short nSprite = QueenList[nQueen].nSprite;
    auto pSprite = &sprite[nSprite];

    int x = pSprite->x;
    int y = pSprite->y;
    short nAngle = pSprite->ang;
    short nSector = pSprite->sectnum;
    int z = sector[nSector].floorz;

    int nSprite2 = insertsprite(nSector, 121);
    assert(nSprite2 >= 0 && nSprite2 < kMaxSprites);
    auto pSprite2 = &sprite[nSprite2];

    pSprite2->x = x;
    pSprite2->y = y;
    pSprite2->z = z;
    pSprite2->clipdist = 70;
    pSprite2->xrepeat = 80;
    pSprite2->yrepeat = 80;
    pSprite2->cstat = 0;
    pSprite2->picnum = 1;
    pSprite2->shade = -12;
    pSprite2->pal = 0;
    pSprite2->xoffset = 0;
    pSprite2->yoffset = 0;
    pSprite2->ang = nAngle;

    nVelShift = 2;
    SetHeadVel(nSprite2);

    pSprite2->zvel = -8192;
    pSprite2->lotag = runlist_HeadRun() + 1;
    pSprite2->hitag = 0;
    pSprite2->extra = -1;

    GrabTimeSlot(3);

    QueenHead.nHealth = 800;
    QueenHead.nAction = 0;
    QueenHead.nTarget = QueenList[nQueen].nTarget;
    QueenHead.nFrame = 0;
    QueenHead.nSprite = nSprite2;
    QueenHead.field_C = 0;

    pSprite2->owner = runlist_AddRunRec(pSprite2->lotag - 1, 0, 0x1B0000);

    QueenHead.field_8 = runlist_AddRunRec(NewRun, 0, 0x1B0000);
    QueenHead.tails = 0;
}

void AIQueenHead::Tick(RunListEvent* ev)
{
    short nHead = RunData[ev->nRun].nObjIndex;

    short nSprite = QueenHead.nSprite;
    auto pSprite = &sprite[nSprite];

    int nSector = pSprite->sectnum;
    assert(nSector >= 0 && nSector < kMaxSectors);

    short nAction = QueenHead.nAction;
    short nTarget, nHd;
    int var_14 = 0;

    if (nAction == 0) {
        Gravity(nSprite);
    }

    short nSeq = SeqOffsets[kSeqQueen] + HeadSeq[QueenHead.nAction].a;

    seq_MoveSequence(nSprite, nSeq, QueenHead.nFrame);

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, QueenHead.nFrame);

    QueenHead.nFrame++;
    if (QueenHead.nFrame >= SeqSize[nSeq])
    {
        QueenHead.nFrame = 0;
        var_14 = 1;
    }

    nTarget = QueenHead.nTarget;

    if (nTarget > -1)
    {
        if (!(sprite[nTarget].cstat & 0x101))
        {
            nTarget = -1;
            QueenHead.nTarget = nTarget;
        }
    }
    else
    {
        nTarget = FindPlayer(nSprite, 1000);
        QueenHead.nTarget = nTarget;
    }

    switch (nAction)
    {
    case 0:
        if (QueenHead.field_C > 0)
        {
            QueenHead.field_C--;
            if (QueenHead.field_C == 0)
            {
                BuildTail();

                QueenHead.nAction = 6;
                nHeadVel = 800;
                pSprite->cstat = 0x101;
            }
            else if (QueenHead.field_C < 60)
            {
                pSprite->shade--;
            }
        }
        else
        {
            int nMov = MoveCreature(nSprite);

            // original BUG - this line doesn't exist in original code?
            short nNewAng = pSprite->ang;

            switch (nMov & 0xFC000)
            {
            default:
                return;
            case 0xC000:
                nNewAng = sprite[nMov & 0x3FFF].ang;
                break;
            case 0x8000:
                nNewAng = GetWallNormal(nMov & 0x3FFF);
                break;
            case 0x20000:
                pSprite->zvel = -(pSprite->zvel >> 1);

                if (pSprite->zvel > -256)
                {
                    nVelShift = 100;
                    pSprite->zvel = 0;
                }
                break;
            }

            // original BUG - var_18 isn't being set if the check above == 0x20000 ?
            pSprite->ang = nNewAng;
            nVelShift++;

            if (nVelShift < 5)
            {
                SetHeadVel(nSprite);
            }
            else
            {
                pSprite->xvel = 0;
                pSprite->yvel = 0;

                if (pSprite->zvel == 0)
                {
                    QueenHead.field_C = 120;
                }
            }
        }

        break;

    case 6:
        if (var_14)
        {
            QueenHead.nAction = 1;
            QueenHead.nFrame = 0;
            break;
        }
        fallthrough__;

    case 1:
        if ((sprite[nTarget].z - 51200) > pSprite->z)
        {
            QueenHead.nAction = 4;
            QueenHead.nFrame = 0;
        }
        else
        {
            pSprite->z -= 2048;
            goto __MOVEQS;
        }
        break;

    case 4:
    case 7:
    case 8:
        if (var_14)
        {
            int nRnd = RandomSize(2);

            if (nRnd == 0)
            {
                QueenHead.nAction = 4;
            }
            else if (nRnd == 1)
            {
                QueenHead.nAction = 7;
            }
            else
            {
                QueenHead.nAction = 8;
            }
        }

        if (nTarget > -1)
        {
            int nMov = QueenAngleChase(nSprite, nTarget, nHeadVel, 64);

            switch (nMov & 0xC000)
            {
            case 0x8000:
                break;
            case 0xC000:
                if ((nMov & 0x3FFF) == nTarget)
                {
                    runlist_DamageEnemy(nTarget, nSprite, 10);
                    D3PlayFX(StaticSound[kSoundQTail] | 0x2000, nSprite);

                    pSprite->ang += RandomSize(9) + 768;
                    pSprite->ang &= kAngleMask;

                    pSprite->zvel = (-20) - RandomSize(6);

                    SetHeadVel(nSprite);
                }
                break;
            }
        }

        // switch break. MoveQS stuff?
    __MOVEQS:
        MoveQX[nQHead] = pSprite->x;
        MoveQY[nQHead] = pSprite->y;
        MoveQZ[nQHead] = pSprite->z;
        assert(pSprite->sectnum >= 0 && pSprite->sectnum < kMaxSectors);
        MoveQS[nQHead] = pSprite->sectnum;
        MoveQA[nQHead] = pSprite->ang;

        nHd = nQHead;

        for (int i = 0; i < QueenHead.tails; i++)
        {
            nHd -= 3;
            if (nHd < 0) {
                nHd += (24 + 1); // TODO - enum/define for these
                //assert(nHd < 24 && nHd >= 0);
            }

            int var_20 = MoveQS[nHd];
            short nTSprite = tailspr[i];

            if (var_20 != sprite[nTSprite].sectnum)
            {
                assert(var_20 >= 0 && var_20 < kMaxSectors);
                mychangespritesect(nTSprite, var_20);
            }

            sprite[nTSprite].x = MoveQX[nHd];
            sprite[nTSprite].y = MoveQY[nHd];
            sprite[nTSprite].z = MoveQZ[nHd];
            sprite[nTSprite].ang = MoveQA[nHd];
        }

        nQHead++;
        if (nQHead >= 25)
        {
            nQHead = 0;
        }

        break;

    case 5:
        QueenHead.field_C--;
        if (QueenHead.field_C <= 0)
        {
            QueenHead.field_C = 3;

            if (QueenHead.tails--)
            {
                if (QueenHead.tails >= 15 || QueenHead.tails < 10)
                {
                    int x = pSprite->x;
                    int y = pSprite->y;
                    int z = pSprite->z;
                    short nSector = pSprite->sectnum;
                    int nAngle = RandomSize(11) & kAngleMask;

                    pSprite->xrepeat = 127 - QueenHead.tails;
                    pSprite->yrepeat = 127 - QueenHead.tails;

                    pSprite->cstat = 0x8000;

                    // DEMO-TODO: in disassembly angle was used without masking and thus causing OOB issue.
                    // This behavior probably would be needed emulated for demo compatibility
                    int dx = bcos(nAngle, 10);
                    int dy = bsin(nAngle, 10);
                    int dz = (RandomSize(5) - RandomSize(5)) << 7;

                    movesprite(nSprite, dx, dy, dz, 0, 0, CLIPMASK1);

                    BlowChunks(nSprite);
                    BuildExplosion(&exhumedActors[nSprite]);

                    mychangespritesect(nSprite, nSector);

                    pSprite->x = x;
                    pSprite->y = y;
                    pSprite->z = z;

                    if (QueenHead.tails < 10) {
                        for (int i = (10 - QueenHead.tails) * 2; i > 0; i--)
                        {
                            BuildLavaLimb(&exhumedActors[nSprite], i, GetSpriteHeight(nSprite));
                        }
                    }
                }
            }
            else
            {
                BuildExplosion(&exhumedActors[nSprite]);

                int i;

                for (i = 0; i < 10; i++)
                {
                    BlowChunks(nSprite);
                }

                for (i = 0; i < 20; i++)
                {
                    BuildLavaLimb(&exhumedActors[nSprite], i, GetSpriteHeight(nSprite));
                }

                runlist_SubRunRec(pSprite->owner);
                runlist_SubRunRec(QueenHead.field_8);
                mydeletesprite(nSprite);
                runlist_ChangeChannel(QueenChan[0], 1);
            }
        }
        break;
    }
}

void AIQueenHead::RadialDamage(RunListEvent* ev)
{
    short nHead = RunData[ev->nRun].nObjIndex;

    short nSprite = QueenHead.nSprite;
    auto pSprite = &sprite[nSprite];

    if (sprite[nRadialSpr].statnum != 121 && (pSprite->cstat & 0x101) != 0)
    {

        ev->nDamage = runlist_CheckRadialDamage(nSprite);
        if (ev->nDamage) Damage(ev);
    }
}

void AIQueenHead::Damage(RunListEvent* ev)
{
    short nHead = RunData[ev->nRun].nObjIndex;

    short nSprite = QueenHead.nSprite;
    auto pSprite = &sprite[nSprite];

    if (QueenHead.nHealth > 0 && ev->nDamage != 0)
    {
        QueenHead.nHealth -= ev->nDamage;

        if (!RandomSize(4))
        {
            QueenHead.nTarget = ev->nParam;
            QueenHead.nAction = 7;
            QueenHead.nFrame = 0;
        }

        if (QueenHead.nHealth <= 0)
        {
            if (DestroyTailPart())
            {
                QueenHead.nHealth = 200;
                nHeadVel += 100;
            }
            else
            {
                QueenHead.nAction = 5;
                QueenHead.nFrame = 0;
                QueenHead.field_C = 0;
                QueenHead.tails = 80;
                pSprite->cstat = 0;
            }
        }
    }
}

void AIQueenHead::Draw(RunListEvent* ev)
{
    short nHead = RunData[ev->nRun].nObjIndex;
    short nAction = QueenHead.nAction;

    short nSeq = SeqOffsets[kSeqQueen];

    int edx;

    if (nHead == 0)
    {
        edx = HeadSeq[nAction].b;
        nSeq += HeadSeq[nAction].a;
    }
    else
    {
        edx = 1;
        nSeq += 73;
    }

    seq_PlotSequence(ev->nParam, nSeq, QueenHead.nFrame, edx);
}


void FuncQueenHead(int nObject, int nMessage, int nDamage, int nRun)
{
    AIQueenHead ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void BuildQueen(int nSprite, int x, int y, int z, int nSector, int nAngle, int nChannel)
{
    QueenCount--;

    short nQueen = QueenCount;
    if (nQueen < 0) {
        return;
    }
    auto pSprite = &sprite[nSprite];

    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 121);
        pSprite = &sprite[nSprite];

    }
    else
    {
        changespritestat(nSprite, 121);
        x = pSprite->x;
        y = pSprite->y;
        z = sector[pSprite->sectnum].floorz;
        nAngle = pSprite->ang;
    }

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    pSprite->x = x;
    pSprite->y = y;
    pSprite->z = z;
    pSprite->cstat = 0x101;
    pSprite->pal = 0;
    pSprite->shade = -12;
    pSprite->clipdist = 100;
    pSprite->xrepeat = 80;
    pSprite->yrepeat = 80;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->picnum = 1;
    pSprite->ang = nAngle;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->extra = -1;
    pSprite->hitag = 0;

    GrabTimeSlot(3);

    QueenList[nQueen].nAction = 0;
    QueenList[nQueen].nHealth = 4000;
    QueenList[nQueen].nFrame = 0;
    QueenList[nQueen].nSprite = nSprite;
    QueenList[nQueen].nTarget = -1;
    QueenList[nQueen].field_A = 0;
    QueenList[nQueen].field_10 = 5;
    QueenList[nQueen].field_C = 0;

    QueenChan[nQueen] = nChannel;

    nHeadVel = 800;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, nQueen, 0x1A0000);

    runlist_AddRunRec(NewRun, nQueen, 0x1A0000);

    nCreaturesTotal++;
}

void SetQueenSpeed(short nSprite, int nSpeed)
{
    auto pSprite = &sprite[nSprite];

    pSprite->xvel = bcos(pSprite->ang, -(2 - nSpeed));
    pSprite->yvel = bsin(pSprite->ang, -(2 - nSpeed));
}

void AIQueen::Tick(RunListEvent* ev)
{
    short nQueen = RunData[ev->nRun].nObjIndex;
    assert(nQueen >= 0 && nQueen < kMaxQueens);

    short nSprite = QueenList[nQueen].nSprite;
    auto pSprite = &sprite[nSprite];
    short nAction = QueenList[nQueen].nAction;
    short si = QueenList[nQueen].field_A;
    short nTarget = QueenList[nQueen].nTarget;

    bool bVal = false;

    if (si < 3) {
        Gravity(nSprite);
    }

    short nSeq = SeqOffsets[kSeqQueen] + QueenSeq[nAction].a;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, QueenList[nQueen].nFrame);

    seq_MoveSequence(nSprite, nSeq, QueenList[nQueen].nFrame);

    QueenList[nQueen].nFrame++;
    if (QueenList[nQueen].nFrame >= SeqSize[nSeq])
    {
        QueenList[nQueen].nFrame = 0;
        bVal = true;
    }

    short nFlag = FrameFlag[SeqBase[nSeq] + QueenList[nQueen].nFrame];

    if (nTarget > -1)
    {
        if (nAction < 7)
        {
            if (!(pSprite->cstat & 0x101))
            {
                nTarget = -1;
                QueenList[nQueen].nTarget = -1;
                QueenList[nQueen].nAction = 0;
            }
        }
    }
    switch (nAction)
    {
    case 0:
    {
        if (nTarget < 0)
        {
            nTarget = FindPlayer(nSprite, 60);
        }

        if (nTarget >= 0)
        {
            QueenList[nQueen].nAction = QueenList[nQueen].field_A + 1;
            QueenList[nQueen].nFrame = 0;
            QueenList[nQueen].nTarget = nTarget;
            QueenList[nQueen].field_C = RandomSize(7);

            SetQueenSpeed(nSprite, si);
        }
        break;
    }

    case 6:
    {
        if (bVal)
        {
            BuildQueenEgg(nQueen, 1);
            QueenList[nQueen].nAction = 3;
            QueenList[nQueen].field_C = RandomSize(6) + 60;
        }

        break;
    }

    case 1:
    case 2:
    case 3:
    {
        QueenList[nQueen].field_C--;

        if ((nQueen & 0x1F) == (totalmoves & 0x1F))
        {
            if (si < 2)
            {
                if (QueenList[nQueen].field_C <= 0)
                {
                    QueenList[nQueen].nFrame = 0;
                    pSprite->xvel = 0;
                    pSprite->yvel = 0;
                    QueenList[nQueen].nAction = si + 4;
                    QueenList[nQueen].field_C = RandomSize(6) + 30;
                    break;
                }
                else
                {
                    if (QueenList[nQueen].field_10 < 5)
                    {
                        QueenList[nQueen].field_10++;
                    }

                    // then to PLOTSPRITE
                }
            }
            else
            {
                if (QueenList[nQueen].field_C <= 0)
                {
                    if (Counters[kCountWasp] < 100)
                    {
                        QueenList[nQueen].nAction = 6;
                        QueenList[nQueen].nFrame = 0;
                        break;
                    }
                    else
                    {
                        QueenList[nQueen].field_C = 30000;
                        // then to PLOTSPRITE
                    }
                }
            }

            // loc_35B4B
            PlotCourseToSprite(nSprite, nTarget);
            SetQueenSpeed(nSprite, si);
        }

        int nMov = MoveCreatureWithCaution(nSprite);

        switch (nMov & 0xC000)
        {
        case 0xC000:
            if ((si == 2) && ((nMov & 0x3FFF) == nTarget))
            {
                runlist_DamageEnemy(nTarget, nSprite, 5);
                break;
            }
            fallthrough__;
        case 0x8000:
            pSprite->ang += 256;
            pSprite->ang &= kAngleMask;

            SetQueenSpeed(nSprite, si);
            break;
        }

        // loc_35BD2
        if (nAction && nTarget != -1)
        {
            if (!(sprite[nTarget].cstat & 0x101))
            {
                QueenList[nQueen].nAction = 0;
                QueenList[nQueen].nFrame = 0;
                QueenList[nQueen].field_C = 100;
                QueenList[nQueen].nTarget = -1;

                pSprite->xvel = 0;
                pSprite->yvel = 0;
            }
        }

        break;
    }

    case 4:
    case 5:
    {
        if (bVal && QueenList[nQueen].field_10 <= 0)
        {
            QueenList[nQueen].nAction = 0;
            QueenList[nQueen].field_C = 15;
        }
        else
        {
            if (nFlag & 0x80)
            {
                QueenList[nQueen].field_10--;

                PlotCourseToSprite(nSprite, nTarget);

                if (!si)
                {
                    BuildBullet(nSprite, 12, 0, 0, -1, pSprite->ang, nTarget + 10000, 1);
                }
                else
                {
                    BuildQueenEgg(nQueen, 0);
                }
            }
        }

        break;
    }

    case 7:
    {
        if (bVal)
        {
            QueenList[nQueen].nAction = 0;
            QueenList[nQueen].nFrame = 0;
        }

        break;
    }

    case 8:
    case 9:
    {
        if (bVal)
        {
            if (nAction == 9)
            {
                QueenList[nQueen].field_C--;
                if (QueenList[nQueen].field_C <= 0)
                {
                    pSprite->cstat = 0;

                    for (int i = 0; i < 20; i++)
                    {
                        short nChunkSprite = BuildCreatureChunk(nSprite, seq_GetSeqPicnum(kSeqQueen, 57, 0)) & 0xFFFF;

                        sprite[nChunkSprite].picnum = kQueenChunk + (i % 3);
                        sprite[nChunkSprite].xrepeat = 100;
                        sprite[nChunkSprite].yrepeat = 100;
                    }

                    short nChunkSprite = BuildCreatureChunk(nSprite, seq_GetSeqPicnum(kSeqQueen, 57, 0));

                    sprite[nChunkSprite].picnum = kTile3126;
                    sprite[nChunkSprite].yrepeat = 100;
                    sprite[nChunkSprite].xrepeat = 100;

                    PlayFXAtXYZ(
                        StaticSound[kSound40],
                        pSprite->x,
                        pSprite->y,
                        pSprite->z,
                        pSprite->sectnum);

                    BuildQueenHead(nQueen);

                    QueenList[nQueen].nAction++;
                }
            }
            else
                QueenList[nQueen].nAction++;
        }

        break;
    }

    case 10:
    {
        pSprite->cstat &= 0xFEFE;
        break;
    }
    }
}

void AIQueen::RadialDamage(RunListEvent* ev)
{
    short nQueen = RunData[ev->nRun].nObjIndex;
    assert(nQueen >= 0 && nQueen < kMaxQueens);
    short nSprite = QueenList[nQueen].nSprite;
    auto pSprite = &sprite[nSprite];

    if (sprite[nRadialSpr].statnum != 121 && (pSprite->cstat & 0x101) != 0)
    {
        ev->nDamage = runlist_CheckRadialDamage(nSprite);
        if (ev->nDamage) Damage(ev);
    }
}

void AIQueen::Damage(RunListEvent* ev)
{
    short nQueen = RunData[ev->nRun].nObjIndex;
    assert(nQueen >= 0 && nQueen < kMaxQueens);

    short nSprite = QueenList[nQueen].nSprite;
    auto pSprite = &sprite[nSprite];
    short si = QueenList[nQueen].field_A;

    if (QueenList[nQueen].nHealth > 0)
    {
        QueenList[nQueen].nHealth -= dmgAdjust(ev->nDamage);

        if (QueenList[nQueen].nHealth <= 0)
        {
            pSprite->xvel = 0;
            pSprite->yvel = 0;
            pSprite->zvel = 0;

            QueenList[nQueen].field_A++;

            switch (QueenList[nQueen].field_A)
            {
            case 1:
                QueenList[nQueen].nHealth = 4000;
                QueenList[nQueen].nAction = 7;

                BuildAnim(nullptr, 36, 0, pSprite->x, pSprite->y, pSprite->z - 7680, pSprite->sectnum, pSprite->xrepeat, 4);
                break;
            case 2:
                QueenList[nQueen].nHealth = 4000;
                QueenList[nQueen].nAction = 7;

                DestroyAllEggs();
                break;
            case 3:
                QueenList[nQueen].nAction = 8;
                QueenList[nQueen].nHealth = 0;
                QueenList[nQueen].field_C = 5;

                nCreaturesKilled++;
                break;
            }

            QueenList[nQueen].nFrame = 0;
        }
        else
        {
            if (si > 0 && !RandomSize(4))
            {
                QueenList[nQueen].nAction = 7;
                QueenList[nQueen].nFrame = 0;
            }
        }
    }
}

void AIQueen::Draw(RunListEvent* ev)
{
    short nQueen = RunData[ev->nRun].nObjIndex;
    assert(nQueen >= 0 && nQueen < kMaxQueens);
    short nAction = QueenList[nQueen].nAction;
    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqQueen] + QueenSeq[nAction].a, QueenList[nQueen].nFrame, QueenSeq[nAction].b);
}

void FuncQueen(int nObject, int nMessage, int nDamage, int nRun)
{
    AIQueen ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

END_PS_NS
