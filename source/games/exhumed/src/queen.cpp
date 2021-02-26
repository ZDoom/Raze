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
	kMaxQueens	= 1,
	kMaxEggs	= 10,
	kMaxTails	= 7
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
    short field_8;
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
            ("at8", w.field_8)
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
        QueenEgg[i].field_8 = -1;
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

    if (QueenEgg[nEgg].nAction != 4)
    {
        BuildAnim(-1, 34, 0, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum, sprite[nSprite].xrepeat, 4);
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            BuildCreatureChunk(nSprite, seq_GetSeqPicnum(kSeqQueenEgg, (i % 2) + 24, 0));
        }
    }

    runlist_DoSubRunRec(sprite[nSprite].owner);
    runlist_DoSubRunRec(sprite[nSprite].lotag - 1);
    runlist_SubRunRec(QueenEgg[nEgg].field_8);

    QueenEgg[nEgg].field_8 = -1;

    mydeletesprite(nSprite);
    QueenEgg.Release(nEgg);
}

void DestroyAllEggs()
{
    for (int i = 0; i < kMaxEggs; i++)
    {
        if (QueenEgg[i].field_8 > -1)
        {
            DestroyEgg(i);
        }
    }
}

void SetHeadVel(short nSprite)
{
    short nAngle = sprite[nSprite].ang;

    sprite[nSprite].xvel = bcos(nAngle, nVelShift);
    sprite[nSprite].yvel = bsin(nAngle, nVelShift);
}

int QueenAngleChase(short nSprite, short nSprite2, int val1, int val2)
{
    short nAngle;

    spritetype *pSprite = &sprite[nSprite];
    if (nSprite2 < 0)
    {
        pSprite->zvel = 0;
        nAngle = pSprite->ang;
    }
    else
    {
        spritetype *pSprite2 = &sprite[nSprite2];
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
            val1 /= abs(nAngDelta>>7);
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

        nAngle = (nAngDelta + sprite[nSprite].ang) & kAngleMask;

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
    BuildExplosion(nSprite);

    for (int i = 0; i < 5; i++)
    {
        short nHeight = GetSpriteHeight(nSprite);
        BuildLavaLimb(nSprite, i, nHeight);
    }

    mydeletesprite(nSprite);
    return 1;
}

void BuildTail()
{
    short nSprite = QueenHead.nSprite;

    int x = sprite[nSprite].x;
    int y = sprite[nSprite].x;
    int z = sprite[nSprite].x;
    short nSector = sprite[nSprite].sectnum;

    int i;

    for (i = 0; i < kMaxTails; i++)
    {
        short nTailSprite = insertsprite(nSector, 121);
        tailspr[i] = nTailSprite;

        if (nTailSprite < 0) {
            I_Error("Can't create queen's tail!\n");
        }

        sprite[nTailSprite].lotag = runlist_HeadRun() + 1;
        sprite[nTailSprite].owner = runlist_AddRunRec(sprite[nTailSprite].lotag - 1, (i + 1) | 0x1B0000);
        sprite[nTailSprite].shade = -12;
        sprite[nTailSprite].x = x;
        sprite[nTailSprite].y = y;
        sprite[nTailSprite].hitag = 0;
        sprite[nTailSprite].cstat = 0;
        sprite[nTailSprite].clipdist = 100;
        sprite[nTailSprite].xrepeat = 80;
        sprite[nTailSprite].yrepeat = 80;
        sprite[nTailSprite].picnum = 1;
        sprite[nTailSprite].pal = sector[sprite[nTailSprite].sectnum].ceilingpal;
        sprite[nTailSprite].xoffset = 0;
        sprite[nTailSprite].yoffset = 0;
        sprite[nTailSprite].z = z;
        sprite[nTailSprite].extra = -1;
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

int BuildQueenEgg(short nQueen, int nVal)
{
    int nEgg = GrabEgg();
    if (nEgg < 0) {
        return -1;
    }

    short nSprite = QueenList[nQueen].nSprite;

    int x = sprite[nSprite].x;
    int y = sprite[nSprite].y;
    short nSector = sprite[nSprite].sectnum;
    int nFloorZ = sector[nSector].floorz;
    short nAngle = sprite[nSprite].ang;

    int nSprite2 = insertsprite(nSector, 121);
    assert(nSprite2 >= 0 && nSprite2 < kMaxSprites);

    sprite[nSprite2].x = x;
    sprite[nSprite2].y = y;
    sprite[nSprite2].z = nFloorZ;
    sprite[nSprite2].pal = 0;
    sprite[nSprite2].clipdist = 50;
    sprite[nSprite2].xoffset = 0;
    sprite[nSprite2].yoffset = 0;
    sprite[nSprite2].shade = -12;
    sprite[nSprite2].picnum = 1;
    sprite[nSprite2].ang = (RandomSize(9) + (nAngle - 256)) & kAngleMask;
    sprite[nSprite2].backuppos();

    if (!nVal)
    {
        sprite[nSprite2].xrepeat = 30;
        sprite[nSprite2].yrepeat = 30;
        sprite[nSprite2].xvel = bcos(sprite[nSprite2].ang);
        sprite[nSprite2].yvel = bsin(sprite[nSprite2].ang);
        sprite[nSprite2].zvel = -6000;
        sprite[nSprite2].cstat = 0;
    }
    else
    {
        sprite[nSprite2].xrepeat = 60;
        sprite[nSprite2].yrepeat = 60;
        sprite[nSprite2].xvel = 0;
        sprite[nSprite2].yvel = 0;
        sprite[nSprite2].zvel = -2000;
        sprite[nSprite2].cstat = 0x101;
    }

    sprite[nSprite2].lotag = runlist_HeadRun() + 1;
    sprite[nSprite2].extra = -1;
    sprite[nSprite2].hitag = 0;

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

    sprite[nSprite2].owner = runlist_AddRunRec(sprite[nSprite2].lotag - 1, nEgg | 0x1D0000);
    QueenEgg[nEgg].field_8 = runlist_AddRunRec(NewRun, nEgg | 0x1D0000);

    return 0;
}

void FuncQueenEgg(int a, int nDamage, int nRun)
{
    short nEgg = RunData[nRun].nVal;

    Egg *pEgg = &QueenEgg[nEgg];
    short nSprite = pEgg->nSprite;
    short nAction = pEgg->nAction;

    short nTarget;

    bool bVal = false;

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        default:
        {
            DebugOut("unknown msg %d for Queenhead\n", nMessage);
            break;
        }

        case 0x20000:
        {
            if (pEgg->nHealth <= 0)
            {
                DestroyEgg(nEgg);
                return;
            }

            if (nAction == 0 || nAction == 4) {
                Gravity(nSprite);
            }

            short nSeq = SeqOffsets[kSeqQueenEgg] + EggSeq[nAction].a;

            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, pEgg->nFrame);

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

                        sprite[nSprite].ang = nAngle;
                        sprite[nSprite].xvel = bcos(nAngle, -1);
                        sprite[nSprite].yvel = bsin(nAngle, -1);
                    }

                    break;
                }

                case 1:
                {
                    if (bVal)
                    {
                        pEgg->nAction = 3;
                        sprite[nSprite].cstat = 0x101;
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
                        sprite[nSprite].ang += (RandomSize(9) + 768);
                        sprite[nSprite].ang &= kAngleMask;
                        sprite[nSprite].xvel = bcos(sprite[nSprite].ang, -3);
                        sprite[nSprite].yvel = bsin(sprite[nSprite].ang, -3);
                        sprite[nSprite].zvel = -RandomSize(5);
                        break;
                    }

                    return;
                }

                case 4:
                {
                    int nMov = MoveCreature(nSprite);

                    if (nMov & 0x20000)
                    {
                        sprite[nSprite].zvel = -(sprite[nSprite].zvel - 256);
                        if (sprite[nSprite].zvel < -512)
                        {
                            sprite[nSprite].zvel = 0;
                        }
                    }

                    pEgg->field_C--;
                    if (pEgg->field_C <= 0)
                    {
                        short nWaspSprite = BuildWasp(-2, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum, sprite[nSprite].ang);
                        sprite[nSprite].z = sprite[nWaspSprite].z;

                        DestroyEgg(nEgg);
                    }
                    break;
                }
            }
            break;
        }

        case 0xA0000:
        {
            if (sprite[nRadialSpr].statnum != 121 && (sprite[nSprite].cstat & 0x101) != 0)
            {
                nDamage = runlist_CheckRadialDamage(nSprite);

                pEgg->nHealth -= nDamage;
            }
            break;
        }

        case 0x80000:
        {
            if (nDamage != 0 && pEgg->nHealth > 0)
            {
                QueenEgg[nEgg].nHealth -= nDamage;

                if (QueenEgg[nEgg].nHealth <= 0)
                    DestroyEgg(nEgg);
            }
            break;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, SeqOffsets[kSeqQueenEgg] + EggSeq[nAction].a, pEgg->nFrame, EggSeq[nAction].b);
            break;
        }
        }
}

int BuildQueenHead(short nQueen)
{
    short nSprite = QueenList[nQueen].nSprite;

    int x = sprite[nSprite].x;
    int y = sprite[nSprite].y;
    short nAngle = sprite[nSprite].ang;
    short nSector = sprite[nSprite].sectnum;
    int z = sector[nSector].floorz;

    int nSprite2 = insertsprite(nSector, 121);
    assert(nSprite2 >= 0 && nSprite2 < kMaxSprites);

    sprite[nSprite2].x = x;
    sprite[nSprite2].y = y;
    sprite[nSprite2].z = z;
    sprite[nSprite2].clipdist = 70;
    sprite[nSprite2].xrepeat = 80;
    sprite[nSprite2].yrepeat = 80;
    sprite[nSprite2].cstat = 0;
    sprite[nSprite2].picnum = 1;
    sprite[nSprite2].shade = -12;
    sprite[nSprite2].pal = 0;
    sprite[nSprite2].xoffset = 0;
    sprite[nSprite2].yoffset = 0;
    sprite[nSprite2].ang = nAngle;

    nVelShift = 2;
    SetHeadVel(nSprite2);

    sprite[nSprite2].zvel = -8192;
    sprite[nSprite2].lotag = runlist_HeadRun() + 1;
    sprite[nSprite2].hitag = 0;
    sprite[nSprite2].extra = -1;

    GrabTimeSlot(3);

    QueenHead.nHealth = 800;
    QueenHead.nAction = 0;
    QueenHead.nTarget = QueenList[nQueen].nTarget;
    QueenHead.nFrame = 0;
    QueenHead.nSprite = nSprite2;
    QueenHead.field_C = 0;

    sprite[nSprite2].owner = runlist_AddRunRec(sprite[nSprite2].lotag - 1, 0x1B0000);

    QueenHead.field_8 = runlist_AddRunRec(NewRun, 0x1B0000);
    QueenHead.tails = 0;

    return 0;
}

void FuncQueenHead(int a, int nDamage, int nRun)
{
    short nHead = RunData[nRun].nVal;

    short nSprite = QueenHead.nSprite;
    int nSector = sprite[nSprite].sectnum;
    assert(nSector >= 0 && nSector < kMaxSectors);

    short nAction = QueenHead.nAction;

    short nTarget, nHd;

    int var_14 = 0;

    int nMessage = a & 0x7F0000;

    switch (nMessage)
    {
        case 0x20000:
        {
            if (nAction == 0) {
                Gravity(nSprite);
            }

            short nSeq = SeqOffsets[kSeqQueen] + HeadSeq[QueenHead.nAction].a;

            seq_MoveSequence(nSprite, nSeq, QueenHead.nFrame);

            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, QueenHead.nFrame);

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
                            sprite[nSprite].cstat = 0x101;
                        }
                        else if (QueenHead.field_C < 60)
                        {
                            sprite[nSprite].shade--;
                        }
                    }
                    else
                    {
                        int nMov = MoveCreature(nSprite);

                        // original BUG - this line doesn't exist in original code?
                        short nNewAng = sprite[nSprite].ang;

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
                            sprite[nSprite].zvel = -(sprite[nSprite].zvel >> 1);

                            if (sprite[nSprite].zvel > -256)
                            {
                                nVelShift = 100;
                                sprite[nSprite].zvel = 0;
                            }
                            break;
                        }

                        // original BUG - var_18 isn't being set if the check above == 0x20000 ?
                        sprite[nSprite].ang = nNewAng;
                        nVelShift++;

                        if (nVelShift < 5)
                        {
                            SetHeadVel(nSprite);
                        }
                        else
                        {
                            sprite[nSprite].xvel = 0;
                            sprite[nSprite].yvel = 0;

                            if (sprite[nSprite].zvel == 0)
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
                    if ((sprite[nTarget].z - 51200) > sprite[nSprite].z)
                    {
                        QueenHead.nAction = 4;
                        QueenHead.nFrame = 0;
                    }
                    else
                    {
                        sprite[nSprite].z -= 2048;
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

                                sprite[nSprite].ang += RandomSize(9) + 768;
                                sprite[nSprite].ang &= kAngleMask;

                                sprite[nSprite].zvel = (-20) - RandomSize(6);

                                SetHeadVel(nSprite);
                            }
                            break;
                        }
                    }

                    // switch break. MoveQS stuff?
__MOVEQS:
                    MoveQX[nQHead] = sprite[nSprite].x;
                    MoveQY[nQHead] = sprite[nSprite].y;
                    MoveQZ[nQHead] = sprite[nSprite].z;
                    assert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors);
                    MoveQS[nQHead] = sprite[nSprite].sectnum;
                    MoveQA[nQHead] = sprite[nSprite].ang;

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
                                int x = sprite[nSprite].x;
                                int y = sprite[nSprite].y;
                                int z = sprite[nSprite].z;
                                short nSector = sprite[nSprite].sectnum;
                                int nAngle = RandomSize(11) & kAngleMask;

                                sprite[nSprite].xrepeat = 127 - QueenHead.tails;
                                sprite[nSprite].yrepeat = 127 - QueenHead.tails;

                                sprite[nSprite].cstat = 0x8000;

                                // DEMO-TODO: in disassembly angle was used without masking and thus causing OOB issue.
                                // This behavior probably would be needed emulated for demo compatibility
                                int dx = bcos(nAngle, 10);
                                int dy = bsin(nAngle, 10);
                                int dz = (RandomSize(5) - RandomSize(5)) << 7;

                                movesprite(nSprite, dx, dy, dz, 0, 0, CLIPMASK1);

                                BlowChunks(nSprite);
                                BuildExplosion(nSprite);

                                mychangespritesect(nSprite, nSector);

                                sprite[nSprite].x = x;
                                sprite[nSprite].y = y;
                                sprite[nSprite].z = z;

                                if (QueenHead.tails < 10) {
                                    for (int i = (10 - QueenHead.tails) * 2; i > 0; i--)
                                    {
                                        BuildLavaLimb(nSprite, i, GetSpriteHeight(nSprite));
                                    }
                                }
                            }
                        }
                        else
                        {
                            BuildExplosion(nSprite);

                            int i;

                            for (i = 0; i < 10; i++)
                            {
                                BlowChunks(nSprite);
                            }

                            for (i = 0; i < 20; i++)
                            {
                                BuildLavaLimb(nSprite, i, GetSpriteHeight(nSprite));
                            }

                            runlist_SubRunRec(sprite[nSprite].owner);
                            runlist_SubRunRec(QueenHead.field_8);
                            mydeletesprite(nSprite);
                            runlist_ChangeChannel(QueenChan[0], 1);
                        }
                    }
                    break;
            }
            break;
        }

        case 0xA0000:
            if (sprite[nRadialSpr].statnum != 121 && (sprite[nSprite].cstat & 0x101) != 0)
            {
                nDamage = runlist_CheckRadialDamage(nSprite);
                if (!nDamage)
                    break;
            }
            else
                break;
            // fall through to case 0x80000
            fallthrough__;

        case 0x80000:
            if (QueenHead.nHealth > 0 && nDamage != 0)
            {
                QueenHead.nHealth -= nDamage;

                if (!RandomSize(4))
                {
                    QueenHead.nTarget = a & 0xFFFF;
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
                        sprite[nSprite].cstat = 0;
                    }
                }
            }
            break;

        case 0x90000:
        {
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

            seq_PlotSequence(a & 0xFFFF, nSeq, QueenHead.nFrame, edx);
            break;
        }

        default:
        {
            DebugOut("unknown msg %d for Queenhead\n", a & 0x7F0000);
            break;
        }
    }
}

int BuildQueen(int nSprite, int x, int y, int z, int nSector, int nAngle, int nChannel)
{
    QueenCount--;

    short nQueen = QueenCount;
    if (nQueen < 0) {
        return -1;
    }

    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 121);
    }
    else
    {
        changespritestat(nSprite, 121);
        x = sprite[nSprite].x;
        y = sprite[nSprite].y;
        z = sector[sprite[nSprite].sectnum].floorz;
        nAngle = sprite[nSprite].ang;
    }

    assert(nSprite >= 0 && nSprite < kMaxSprites);

    sprite[nSprite].x = x;
    sprite[nSprite].y = y;
    sprite[nSprite].z = z;
    sprite[nSprite].cstat = 0x101;
    sprite[nSprite].pal = 0;
    sprite[nSprite].shade = -12;
    sprite[nSprite].clipdist = 100;
    sprite[nSprite].xrepeat = 80;
    sprite[nSprite].yrepeat = 80;
    sprite[nSprite].xoffset = 0;
    sprite[nSprite].yoffset = 0;
    sprite[nSprite].picnum = 1;
    sprite[nSprite].ang = nAngle;
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].extra = -1;
    sprite[nSprite].hitag = 0;

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

    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nQueen | 0x1A0000);

    runlist_AddRunRec(NewRun, nQueen | 0x1A0000);

    nCreaturesTotal++;

    return nQueen | 0x1A0000;
}

void SetQueenSpeed(short nSprite, int nSpeed)
{
    sprite[nSprite].xvel = bcos(sprite[nSprite].ang, -(2 - nSpeed));
    sprite[nSprite].yvel = bsin(sprite[nSprite].ang, -(2 - nSpeed));
}

void FuncQueen(int a, int nDamage, int nRun)
{
    short nQueen = RunData[nRun].nVal;
    assert(nQueen >= 0 && nQueen < kMaxQueens);

    short nSprite = QueenList[nQueen].nSprite;
    short nAction = QueenList[nQueen].nAction;
    short si = QueenList[nQueen].field_A;
    short nTarget = QueenList[nQueen].nTarget;

    bool bVal = false;

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {

        default:
        {
            DebugOut("unknown msg %d for Queen\n", nMessage);
            break;
        }

        case 0x20000:
        {
            if (si < 3) {
                Gravity(nSprite);
            }

            short nSeq = SeqOffsets[kSeqQueen] + QueenSeq[nAction].a;

            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, QueenList[nQueen].nFrame);

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
                    if (!(sprite[nSprite].cstat & 0x101))
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
                        QueenList[nQueen].nFrame  = 0;
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
                                sprite[nSprite].xvel = 0;
                                sprite[nSprite].yvel = 0;
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
                                if (WaspCount() < 100)
                                {
                                    QueenList[nQueen].nAction = 6;
                                    QueenList[nQueen].nFrame  = 0;
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
                            sprite[nSprite].ang += 256;
                            sprite[nSprite].ang &= kAngleMask;

                            SetQueenSpeed(nSprite, si);
                            break;
                    }

                    // loc_35BD2
                    if (nAction && nTarget != -1)
                    {
                        if (!(sprite[nTarget].cstat & 0x101))
                        {
                            QueenList[nQueen].nAction = 0;
                            QueenList[nQueen].nFrame  = 0;
                            QueenList[nQueen].field_C = 100;
                            QueenList[nQueen].nTarget = -1;

                            sprite[nSprite].xvel = 0;
                            sprite[nSprite].yvel = 0;
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
                                BuildBullet(nSprite, 12, 0, 0, -1, sprite[nSprite].ang, nTarget + 10000, 1);
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
                        QueenList[nQueen].nFrame  = 0;
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
                                sprite[nSprite].cstat = 0;

                                for (int i = 0; i < 20; i++)
                                {
                                    short nChunkSprite = BuildCreatureChunk(nSprite, seq_GetSeqPicnum(kSeqQueen, 57, 0)) & 0xFFFF;

                                    sprite[nChunkSprite].picnum = kTile3117 + (i % 3);
                                    sprite[nChunkSprite].xrepeat = 100;
                                    sprite[nChunkSprite].yrepeat = 100;
                                }

                                short nChunkSprite = BuildCreatureChunk(nSprite, seq_GetSeqPicnum(kSeqQueen, 57, 0));

                                sprite[nChunkSprite].picnum = kTile3126;
                                sprite[nChunkSprite].yrepeat = 100;
                                sprite[nChunkSprite].xrepeat = 100;

                                PlayFXAtXYZ(
                                    StaticSound[kSound40],
                                    sprite[nSprite].x,
                                    sprite[nSprite].y,
                                    sprite[nSprite].z,
                                    sprite[nSprite].sectnum);

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
                    sprite[nSprite].cstat &= 0xFEFE;
                    break;
                }
            }
            break;
        }

        case 0xA0000:
        {
            if (sprite[nRadialSpr].statnum != 121 && (sprite[nSprite].cstat & 0x101) != 0)
            {
                nDamage = runlist_CheckRadialDamage(nSprite);

                if (!nDamage) {
                    break;
                }
            }
            else
                break;

            fallthrough__;
        } // fall through to case 0x80000

        case 0x80000:
        {
            if (QueenList[nQueen].nHealth > 0)
            {
                QueenList[nQueen].nHealth -= nDamage;

                if (QueenList[nQueen].nHealth <= 0)
                {
                    sprite[nSprite].xvel = 0;
                    sprite[nSprite].yvel = 0;
                    sprite[nSprite].zvel = 0;

                    QueenList[nQueen].field_A++;

                    switch (QueenList[nQueen].field_A)
                    {
                    case 1:
                        QueenList[nQueen].nHealth = 4000;
                        QueenList[nQueen].nAction = 7;

                        BuildAnim(-1, 36, 0, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z - 7680, sprite[nSprite].sectnum, sprite[nSprite].xrepeat, 4);
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
                        QueenList[nQueen].nFrame  = 0;
                    }
                }
            }
            break;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, SeqOffsets[kSeqQueen] + QueenSeq[nAction].a, QueenList[nQueen].nFrame, QueenSeq[nAction].b);
            break;
        }
        }
}
END_PS_NS
