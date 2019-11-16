
#include "exhumed.h"
#include "aistuff.h"
#include "engine.h"
#include "queen.h"
#include "move.h"
#include "sequence.h"
#include "runlist.h"
#include "random.h"
#include "wasp.h"
#include "trigdat.h"
#include "anims.h"
#include "player.h"
#include "sound.h"
#include "names.h"
#include <assert.h>

#define kMaxQueens	1
#define kMaxEggs	10
#define kMaxTails	7

short QueenCount = 0;

static actionSeq ActionSeq[] = {
    { 0, 0 },
    { 0, 0 },
    { 9, 0 },
    { 36, 0 },
    { 18, 0 },
    { 27, 0 },
    { 45, 0 },
    { 45, 0 },
    { 54, 1 },
    { 53, 1 },
    { 55, 1 }
};

static actionSeq HeadSeq[] = {
    { 56, 1 },
    { 65, 0 },
    { 65, 0 },
    { 65, 0 },
    { 65, 0 },
    { 65, 0 },
    { 74, 0 },
    { 82, 0 },
    { 90, 0 }
};

static actionSeq EggSeq[] = {
    { 19, 1 },
    { 18, 1 },
    { 0, 0 },
    { 9, 0 },
    { 23, 1 },
};

int nQHead = 0;

short nEggsFree;
short nHeadVel;
short nVelShift;

short tailspr[kMaxTails];
short nEggFree[kMaxEggs];

short QueenChan[kMaxQueens];




struct Queen
{
    short nHealth;
    short field_2;
    short nAction;
    short nSprite;
    short nTarget;
    short field_A;
    short field_C;
    short pad;
    short field_10;
    short field_12;
};

struct Egg
{
    short nHealth;
    short field_2;
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
    short field_2;
    short nAction;
    short nSprite;
    short field_8;
    short nTarget;
    short field_C;
    short field_E;
};

Egg QueenEgg[kMaxEggs];
Queen QueenList[kMaxQueens];
Head QueenHead;

int MoveQX[25];
int MoveQY[25];
int MoveQZ[25];
short MoveQS[25];
short MoveQA[25];


void InitQueens()
{
    QueenCount = 1;

    for (int i = 0; i < kMaxEggs; i++)
    {
        nEggFree[i] = i;
        QueenEgg[i].field_8 = -1;
    }
}

int GrabEgg()
{
    if (!nEggsFree) {
        return -1;
    }

    nEggsFree--;
    return nEggFree[nEggsFree];
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

    if (QueenEgg[nEgg].nAction == 4)
    {
        for (int i = 0; i < 4; i++)
        {
            BuildCreatureChunk(nSprite, seq_GetSeqPicnum(kSeqQueenEgg, (i % 2) + 24, 0));
        }
    }
    else
    {
        BuildAnim(-1, 34, 0, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum, sprite[nSprite].xrepeat, 4);
    }

    runlist_DoSubRunRec(sprite[nSprite].owner);
    runlist_DoSubRunRec(sprite[nSprite].lotag - 1);
    runlist_SubRunRec(QueenEgg[nEgg].field_8);

    QueenEgg[nEgg].field_8 = -1;

    mydeletesprite(nSprite);

    nEggFree[nEggsFree] = nEgg;
    nEggsFree++;
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

    if (nVelShift < 0)
    {
        sprite[nSprite].xvel = Sin(nAngle + 512) << (int8_t)(-nVelShift);
        sprite[nSprite].yvel = Sin(nAngle) << (int8_t)(-nVelShift);
    }
    else
    {
        sprite[nSprite].xvel = Sin(nAngle + 512) >> (int8_t)(nVelShift);
        sprite[nSprite].yvel = Sin(nAngle) >> (int8_t)(nVelShift);
    }
}

int QueenAngleChase(short nSprite, short nSprite2, int val1, int val2)
{
    short nAngle;

    if (nSprite2 < 0)
    {
        sprite[nSprite].zvel = 0;
        nAngle = sprite[nSprite].ang;
    }
    else
    {
        int nTileY = (tilesiz[sprite[nSprite2].picnum].y * sprite[nSprite2].yrepeat) * 2;

        int nMyAngle = GetMyAngle(sprite[nSprite2].x - sprite[nSprite].x, sprite[nSprite2].y - sprite[nSprite].y);

        int edx = ((sprite[nSprite2].z - nTileY) - sprite[nSprite].z) >> 8;

        int x = sprite[nSprite2].x - sprite[nSprite].x;
        int y = sprite[nSprite2].y - sprite[nSprite].y;

        int nSqrt = ksqrt(x * x + y * y);

        int var_14 = GetMyAngle(nSqrt, edx);

        int nAngDelta = AngleDelta(sprite[nSprite].ang, nMyAngle, 1024);
        int nAngDeltaB = nAngDelta; // edx
        int nAngDeltaC = nAngDelta; // edi

        if (nAngDelta < 0) {
            nAngDelta = -nAngDelta;
        }

        if (nAngDelta > 127)
        {
            nAngDelta = nAngDeltaB >> 7;
            if (nAngDelta < 0) {
                nAngDelta = -nAngDelta;
            }

            val1 = val1 / nAngDelta;

            if (val1 < 256) {
                val1 = 256;
            }
        }

        // restore the value of nAngDelta
        nAngDelta = nAngDeltaC;

        if (nAngDelta < 0) {
            nAngDelta = -nAngDelta;
        }

        if (nAngDelta > val2)
        {
            if (nAngDeltaC >= 0)
            {
                nAngDeltaC = val2;
            }
            else
            {
                nAngDeltaC = -val2;
            }
        }

        nAngle = (nAngDeltaC + sprite[nSprite].ang) & kAngleMask;

        sprite[nSprite].zvel = (AngleDelta(sprite[nSprite].zvel, var_14, 24) + sprite[nSprite].zvel) & kAngleMask;
    }

    sprite[nSprite].ang = nAngle;

    int x = Sin(sprite[nSprite].zvel + 512);

    if (x < 0) {
        x = -x;
    }

    int v26 = x * ((val1 * Sin(nAngle + 512)) >> 14);
    int v27 = x * ((val1 * Sin(nAngle)) >> 14);

    int nSqrt = ksqrt(((v26 >> 8) * (v26 >> 8)) + ((v27 >> 8) * (v27 >> 8)));

    return movesprite(nSprite, v26 >> 2, v27 >> 2, (Sin(bobangle) >> 5) + ((nSqrt * Sin(sprite[nSprite].zvel)) >> 13), 0, 0, CLIPMASK1);
}

int DestroyTailPart()
{
    if (!QueenHead.field_E) {
        return 0;
    }

    QueenHead.field_E--;
    int edx = QueenHead.field_E;

    short nSprite = tailspr[edx];

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
            bail2dos("Can't create queen's tail!\n");
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
    QueenHead.field_E = 7;
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

    if (nVal)
    {
        sprite[nSprite2].xrepeat = 60;
        sprite[nSprite2].yrepeat = 60;
        sprite[nSprite2].xvel = 0;
        sprite[nSprite2].yvel = 0;
        sprite[nSprite2].zvel = -2000;
        sprite[nSprite2].cstat = 0x101;
    }
    else
    {
        sprite[nSprite2].xrepeat = 30;
        sprite[nSprite2].yrepeat = 30;
        sprite[nSprite2].xvel = Sin(sprite[nSprite2].ang + 512);
        sprite[nSprite2].yvel = Sin(sprite[nSprite2].ang);
        sprite[nSprite2].zvel = -6000;
        sprite[nSprite2].cstat = 0;
    }

    sprite[nSprite2].lotag = runlist_HeadRun() + 1;
    sprite[nSprite2].extra = -1;
    sprite[nSprite2].hitag = 0;

    GrabTimeSlot(3);

    QueenEgg[nEgg].nHealth = 200;
    QueenEgg[nEgg].field_2 = 0;
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

    int var_14 = 0;

    short nSprite = QueenEgg[nEgg].nSprite;
    short nAction = QueenEgg[nEgg].nAction;

    short nTarget;

    int nMessage = a & 0x7F0000;

    switch (nMessage)
    {
        default:
        {
            DebugOut("unknown msg %d for Queenhead\n", a & 0x7F0000);
            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, SeqOffsets[kSeqQueenEgg] + EggSeq[nAction].a, QueenEgg[nEgg].field_2, EggSeq[nAction].b);
            return;
        }

        case 0xA0000:
        {
            if (sprite[nRadialSpr].statnum == 121) {
                return;
            }

            if (!(sprite[nSprite].cstat & 0x101)) {
                return;
            }

            nDamage = runlist_CheckRadialDamage(nSprite);

            QueenEgg[nEgg].nHealth -= nDamage;
            return;
        }

        case 0x80000:
        {
            if (!nDamage) {
                return;
            }

            if (QueenEgg[nEgg].nHealth <= 0) {
                return;
            }

            QueenEgg[nEgg].nHealth -= nDamage;

            if (QueenEgg[nEgg].nHealth > 0) {
                return;
            }

            DestroyEgg(nEgg);
            return;
        }

        case 0x20000:
        {
            if (QueenEgg[nEgg].nHealth <= 0)
            {
                DestroyEgg(nEgg);
                return;
            }

            if (!nAction || nAction == 4) {
                Gravity(nSprite);
            }

            short nSeq = SeqOffsets[kSeqQueenEgg] + EggSeq[nAction].a;

            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, QueenEgg[nEgg].field_2);

            if (nAction != 4)
            {
                seq_MoveSequence(nSprite, nSeq, QueenEgg[nEgg].field_2);

                QueenEgg[nEgg].field_2++;
                if (QueenEgg[nEgg].field_2 >= SeqSize[nSeq])
                {
                    QueenEgg[nEgg].field_2 = 0;
                    var_14 = 1;
                }

                nTarget = UpdateEnemy(&QueenEgg[nEgg].nTarget);
                QueenEgg[nEgg].nTarget = nTarget;

                if (nTarget < 0 || (sprite[nTarget].cstat & 0x101))
                {
                    nTarget = FindPlayer(-nSprite, 1000);
                    QueenEgg[nEgg].nTarget = nTarget;
                }
                else
                {
                    QueenEgg[nEgg].nTarget = -1;
                    QueenEgg[nEgg].nAction = 0;
                }
            }

            switch (nAction)
            {
                default:
                    return;

                case 0:
                {
                    int nMov = MoveCreature(nSprite);
                    if (!nMov) {
                        return;
                    }

                    if (nMov != 0x20000)
                    {
                        short nAngle;

                        if ((nMov & 0xC000) == 0x8000)
                        {
                            nAngle = GetWallNormal(nMov & 0x3FFF);
                        }
                        else if ((nMov & 0xC000) == 0xC000)
                        {
                            nAngle = sprite[nMov & 0x3FFF].ang;
                        }
                        else {
                            return;
                        }

                        sprite[nSprite].ang = nAngle;
                        sprite[nSprite].xvel = Sin(nAngle + 512) >> 1;
                        sprite[nSprite].yvel = Sin(nAngle) >> 1;
                    }
                    else
                    {
                        if (!RandomSize(1))
                        {
                            QueenEgg[nEgg].nAction = 1;
                            QueenEgg[nEgg].field_2 = 0;
                        }
                        else
                        {
                            DestroyEgg(nEgg);
                        }
                    }

                    return;
                }

                case 1:
                {
                    if (!var_14) {
                        return;
                    }

                    QueenEgg[nEgg].nAction = 3;

                    sprite[nSprite].cstat = 0x101;
                    return;
                }

                case 2:
                case 3:
                {
                    int nMov = QueenAngleChase(nSprite, nTarget, nHeadVel, 64);

                    if ((nMov & 0xC000) == 0x8000)
                    {
                        sprite[nSprite].ang += (RandomSize(9) + 768);
                        sprite[nSprite].ang &= kAngleMask;
                        sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512) >> 3;
                        sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 3;
                        sprite[nSprite].zvel = -RandomSize(5);
                    }
                    else if ((nMov & 0xC000) == 0xC000)
                    {
                        if (sprite[nMov & 0x3FFF].statnum != 121)
                        {
                            runlist_DamageEnemy(nMov & 0x3FFF, nSprite, 5);
                        }

                        sprite[nSprite].ang += (RandomSize(9) + 768);
                        sprite[nSprite].ang &= kAngleMask;
                        sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512) >> 3;
                        sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 3;
                        sprite[nSprite].zvel = -RandomSize(5);
                    }

                    return;
                }

                case 4:
                {
                    int nMov = MoveCreature(nSprite);

                    if (nMov == 0x20000)
                    {
                        sprite[nSprite].zvel = -(sprite[nSprite].zvel - 256);
                        if (sprite[nSprite].zvel < -512)
                        {
                            sprite[nSprite].zvel = 0;
                        }
                    }

                    QueenEgg[nEgg].field_C--;
                    if (QueenEgg[nEgg].field_C > 0) {
                        return;
                    }

                    short nWaspSprite = BuildWasp(-2, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum, sprite[nSprite].ang);
                    sprite[nSprite].z = sprite[nWaspSprite].z;

                    DestroyEgg(nEgg);
                    return;
                }
            }
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
    QueenHead.field_2 = 0;
    QueenHead.nSprite = nSprite2;
    int nSector2 = sprite[QueenHead.nSprite].sectnum;
    assert(nSector2 >= 0 && nSector2 < kMaxSectors);

    QueenHead.field_C = 0;

    sprite[nSprite2].owner = runlist_AddRunRec(sprite[nSprite2].lotag - 1, 0x1B0000);

    QueenHead.field_8 = runlist_AddRunRec(NewRun, 0x1B0000);
    QueenHead.field_E = 0;

    return 0;
}

void FuncQueenHead(int a, int nDamage, int nRun)
{
    short nHead = RunData[nRun].nVal;

    short nSprite = QueenHead.nSprite;
    int nSector = sprite[nSprite].sectnum;
    assert(nSector >= 0 && nSector < kMaxSectors);

    short nAction = QueenHead.nAction;

    short nTarget;

    int var_14 = 0;

    int nMessage = a & 0x7F0000;

    switch (nMessage)
    {
        default:
        {
            DebugOut("unknown msg %d for Queenhead\n", a & 0x7F0000);
            return;
        }

        case 0x90000:
        {
            short nSeq = SeqOffsets[kSeqQueen];

            int edx;

            if (nHead)
            {
                edx = 1;
                nSeq += 73;
            }
            else
            {
                edx = HeadSeq[nAction].b;
                nSeq += HeadSeq[nAction].a;
            }

            seq_PlotSequence(a & 0xFFFF, nSeq, QueenHead.field_2, edx);
            return;
        }

        case 0xA0000:
        {
            if (sprite[nRadialSpr].statnum == 121) {
                return;
            }

            if (!(sprite[nSprite].cstat & 0x101)) {
                return;
            }

            nDamage = runlist_CheckRadialDamage(nSprite);
            if (!nDamage) {
                return;
            }
            // fall through to case 0x80000
        }

        case 0x80000:
        {
            if (QueenHead.nHealth <= 0) {
                return;
            }

            if (!nDamage) {
                return;
            }

            QueenHead.nHealth -= nDamage;

            if (!RandomSize(4))
            {
                QueenHead.nTarget = a & 0xFFFF;
                QueenHead.nAction = 7;
                QueenHead.field_2 = 0;
            }

            if (QueenHead.nHealth > 0) {
                return;
            }

            if (DestroyTailPart())
            {
                QueenHead.nHealth = 10; // 200;
                nHeadVel += 100;
                return;
            }
            else
            {
                QueenHead.nAction = 5;
                QueenHead.field_2 = 0;
                QueenHead.field_C = 0;
                QueenHead.field_E = 80;
                sprite[nSprite].cstat = 0;
                return;
            }
        }

        case 0x20000:
        {
            if (nAction == 0) {
                Gravity(nSprite);
            }

            short nSeq = SeqOffsets[kSeqQueen] + HeadSeq[QueenHead.nAction].a;

            seq_MoveSequence(nSprite, nSeq, QueenHead.field_2);

            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, QueenHead.field_2);

            QueenHead.field_2++;
            if (QueenHead.field_2 >= SeqSize[nSeq])
            {
                QueenHead.field_2 = 0;
                var_14 = 1;
            }

            nTarget = QueenHead.nTarget;

            if (nTarget <= -1)
            {
                nTarget = FindPlayer(nSprite, 1000);
                QueenHead.nTarget = nTarget;
            }
            else
            {
                if (!(sprite[nTarget].cstat & 0x101))
                {
                    nTarget = -1;
                    QueenHead.nTarget = nTarget;
                }
            }

            switch (nAction)
            {
                default:
                    return;

                case 0: 
                {
                    if (QueenHead.field_C > 0)
                    {
                        QueenHead.field_C--;
                        if (QueenHead.field_C == 0)
                        {
                            BuildTail();

                            QueenHead.nAction = 6;
                            nHeadVel = 800;
                            sprite[nSprite].cstat = 0x101;
                            return;
                        }

                        if (QueenHead.field_C < 60) {
                            sprite[nSprite].shade--;
                        }

                        return;
                    }
                    else
                    {
                        int nMov = MoveCreature(nSprite);

                        // original BUG - this line doesn't exist in original code?
                        short nNewAng = sprite[nSprite].ang;

                        if ((nMov & 0xFC000) == 0xC000)
                        {
                            nNewAng = sprite[nMov & 0x3FFF].ang;
                        }
                        else if ((nMov & 0xFC000) == 0x8000)
                        {
                            nNewAng = GetWallNormal(nMov & 0x3FFF);
                        }
                        else if ((nMov & 0xFC000) == 0x20000)
                        {
                            sprite[nSprite].zvel = -(sprite[nSprite].zvel >> 1);

                            if (sprite[nSprite].zvel > -256)
                            {
                                nVelShift = 100;
                                sprite[nSprite].zvel = 0;
                            }
                        }
                        else
                        {
                            return;
                        }

                        // original BUG - var_18 isn't being set if the check above == 0x20000 ?
                        sprite[nSprite].ang = nNewAng;
                        nVelShift++;

                        if (nVelShift < 5)
                        {
                            SetHeadVel(nSprite);
                            return;
                        }
                        else
                        {
                            sprite[nSprite].xvel = 0;
                            sprite[nSprite].yvel = 0;

                            if (sprite[nSprite].zvel == 0)
                            {
                                QueenHead.field_C = 120;
                            }

                            return;
                        }
                    }

                    return;
                }

                case 1:
                {
                    if ((sprite[nTarget].z - 51200) <= sprite[nSprite].z)
                    {
                        sprite[nSprite].z -= 2048;
                        break;
                    }
                    else
                    {
                        QueenHead.nAction = 4;
                        QueenHead.field_2 = 0;
                    }

                    return;
                }

                case 2:
                case 3:
                {
                    return;
                }

                case 4:
                case 7:
                case 8:
                {
                    if (var_14)
                    {
                        int nRnd = RandomSize(2);

                        if (nRnd == 0)
                        {
                            QueenHead.nAction = 4;
                        }
                        else
                        {
                            if (nRnd == 1) {
                                QueenHead.nAction = 7;
                            }
                            else {
                                QueenHead.nAction = 8;
                            }
                        }
                    }

                    if (nTarget > -1)
                    {
                        int nMov = QueenAngleChase(nSprite, nTarget, nHeadVel, 64);

                        if ((nMov & 0xC000) == 0x8000) {
                            break;
                        }

                        if ((nMov & 0xC000) == 0xC000)
                        {
                            if ((nMov & 0x3FFF) == nTarget)
                            {
                                runlist_DamageEnemy(nTarget, nSprite, 10);
                                D3PlayFX((StaticSound[kSoundQTail] | 0x2000) & 0xFFFF, nSprite);

                                sprite[nSprite].ang += RandomSize(9) + 768;
                                sprite[nSprite].ang &= kAngleMask;

                                sprite[nSprite].zvel = (-20) - RandomSize(6);

                                SetHeadVel(nSprite);
                            }
                        }
                    }

                    break;
                }

                case 5:
                {
                    QueenHead.field_C--;
                    if (QueenHead.field_C > 0) {
                        return;
                    }

                    short ax = QueenHead.field_E;
                    
                    QueenHead.field_C = 3;
                    QueenHead.field_E--;

                    if (ax == 0)
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
                    else
                    {
                        if (QueenHead.field_E >= 15 || QueenHead.field_E < 10)
                        {
                            int x = sprite[nSprite].x;
                            int y = sprite[nSprite].y;
                            int z = sprite[nSprite].z;
                            short nSector = sprite[nSprite].sectnum;

                            sprite[nSprite].xrepeat = 127 - QueenHead.field_E;
                            sprite[nSprite].yrepeat = 127 - QueenHead.field_E;
                            
                            sprite[nSprite].cstat = 0x8000;

                            // DEMO-TODO: in disassembly angle was used without masking and thus causing OOB issue.
                            // This behavior probably would be needed emulated for demo compatibility
                            // int dx = sintable[RandomSize(11) & kAngleMask) + 512] << 10;
                            int dx = Sin((RandomSize(11) & kAngleMask) + 512) << 10;
                            int dy = Sin(RandomSize(11) & kAngleMask) << 10;
                            int dz = (RandomSize(5) - RandomSize(5)) << 7;

                            int nMov = movesprite(nSprite, dx, dy, dz, 0, 0, CLIPMASK1);

                            BlowChunks(nSprite);
                            BuildExplosion(nSprite);

                            mychangespritesect(nSprite, nSector);

                            sprite[nSprite].x = x;
                            sprite[nSprite].y = y;
                            sprite[nSprite].z = z;

                            if (QueenHead.field_E >= 10) {
                                return;
                            }

                            int ecx = (10 - QueenHead.field_E) * 2;

                            while (ecx > 0)
                            {
                                BuildLavaLimb(nSprite, ecx, GetSpriteHeight(nSprite));
                                ecx--;
                            }
                        }
                    }

                    return;
                }

                case 6:
                {
                    if (var_14 != 0)
                    {
                        QueenHead.nAction = 1;
                        QueenHead.field_2 = 0;
                        return;
                    }
                    else
                    {
                        if ((sprite[nTarget].z - 51200) > sprite[nSprite].z)
                        {
                            QueenHead.nAction = 4;
                            QueenHead.field_2 = 0;
                            return;
                        }
                        else
                        {
                            sprite[nSprite].z -= 2048;
                        }
                    }

                    break;
                }
            }

            // switch break. MoveQS stuff?
            MoveQX[nQHead] = sprite[nSprite].x;
            MoveQY[nQHead] = sprite[nSprite].y;
            MoveQZ[nQHead] = sprite[nSprite].z;
            assert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors);
            MoveQS[nQHead] = sprite[nSprite].sectnum;
            MoveQA[nQHead] = sprite[nSprite].ang;

            short nHd = nQHead;

            for (int i = 0; i < QueenHead.field_E; i++)
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
                    mychangespritesect(nSprite, var_20);
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

            return;
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
    QueenList[nQueen].field_2 = 0;
    QueenList[nQueen].nSprite = nSprite;
    QueenList[nQueen].nTarget = -1;
    QueenList[nQueen].field_A = 0;
    QueenList[nQueen].field_10 = 5;
    QueenList[nQueen].field_C = 0;

    QueenChan[nQueen] = nChannel;

    nHeadVel = 800;

    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nQueen | 0x1A0000);

    runlist_AddRunRec(NewRun, nQueen | 0x1A0000);

    nCreaturesLeft++;

    return nQueen | 0x1A0000;
}

void SetQueenSpeed(short nSprite, int nSpeed)
{
    sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512) >> (2 - nSpeed);
    sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> (2 - nSpeed);
}

void FuncQueen(int a, int nDamage, int nRun)
{
    int var_18 = 0;

    short nQueen = RunData[nRun].nVal;
    assert(nQueen >= 0 && nQueen < kMaxQueens);

    int nMessage = a & 0x7F0000;

    short nSprite = QueenList[nQueen].nSprite;
    short nAction = QueenList[nQueen].nAction;
    short si = QueenList[nQueen].field_A;
    short nTarget = QueenList[nQueen].nTarget;

    switch (nMessage)
    {
        default:
        {
            DebugOut("unknown msg %d for Queen\n", a & 0x7F0000);
            return;
        }

        case 0xA0000:
        {
            if (sprite[nRadialSpr].statnum == 121) {
                return;
            }

            if (!(sprite[nSprite].cstat & 0x101)) {
                return;
            }

            nDamage = runlist_CheckRadialDamage(nSprite);

            if (!nDamage) {
                return;
            }
        } // fall through to case 0x80000

        case 0x80000:
        {
            if (QueenList[nQueen].nHealth <= 0) {
                return;
            }

            QueenList[nQueen].nHealth -= nDamage;

            if (QueenList[nQueen].nHealth > 0)
            {
                if (si <= 0) {
                    return;
                }

                if (RandomSize(4)) {
                    return;
                }

                QueenList[nQueen].nAction = 7;
                QueenList[nQueen].field_2 = 0;
                return;
            }
            else
            {
                sprite[nSprite].xvel = 0;
                sprite[nSprite].yvel = 0;
                sprite[nSprite].zvel = 0;

                QueenList[nQueen].field_A++;

                if (QueenList[nQueen].field_A == 1)
                {
                    QueenList[nQueen].nHealth = 4000;
                    QueenList[nQueen].nAction = 7;

                    BuildAnim(-1, 36, 0, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z - 7680, sprite[nSprite].sectnum, sprite[nSprite].xrepeat, 4);
                }
                else if (QueenList[nQueen].field_A == 2)
                {
                    QueenList[nQueen].nHealth = 4000;
                    QueenList[nQueen].nAction = 7;

                    DestroyAllEggs();
                }
                else if (QueenList[nQueen].field_A == 3)
                {
                    QueenList[nQueen].nAction = 8;
                    QueenList[nQueen].nHealth = 0;
                    QueenList[nQueen].field_C = 5;

                    nCreaturesLeft--;
                }

                QueenList[nQueen].field_2 = 0;
            }

            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, SeqOffsets[kSeqQueen] + ActionSeq[nAction].a, QueenList[nQueen].field_2, ActionSeq[nAction].b);
            return;
        }

        case 0x20000:
        {
            if (si < 3) {
                Gravity(nSprite);
            }

            short nSeq = SeqOffsets[kSeqQueen] + ActionSeq[nAction].a;

            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, QueenList[nQueen].field_2);

            seq_MoveSequence(nSprite, nSeq, QueenList[nQueen].field_2);

            QueenList[nQueen].field_2++;
            if (QueenList[nQueen].field_2 >= SeqSize[nSeq])
            {
                QueenList[nQueen].field_2 = 0;
                var_18 = 1;
            }

            short nFlag = FrameFlag[SeqBase[nSeq] + QueenList[nQueen].field_2];

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

            if (nAction > 10) {
                return;
            }

            switch (nAction)
            {
                case 0:
                {
                    if (nTarget < 0)
                    {
                        nTarget = FindPlayer(nSprite, 60);
                    }

                    if (nTarget < 0) {
                        return;
                    }

                    QueenList[nQueen].nAction = QueenList[nQueen].field_A + 1;
                    QueenList[nQueen].field_2 = 0;
                    QueenList[nQueen].nTarget = nTarget;
                    QueenList[nQueen].field_C = RandomSize(7);

                    SetQueenSpeed(nSprite, si);

                    return;
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
                            if (QueenList[nQueen].field_C > 0)
                            {
                                if (QueenList[nQueen].field_10 < 5)
                                {
                                    QueenList[nQueen].field_10++;
                                }

                                // then to PLOTSPRITE
                            }
                            else
                            {
                                QueenList[nQueen].field_2 = 0;
                                sprite[nSprite].xvel = 0;
                                sprite[nSprite].yvel = 0;
                                QueenList[nQueen].nAction = si + 4;
                                QueenList[nQueen].field_C = RandomSize(6) + 30;
                                return;
                            }
                        }
                        else
                        {
                            if (QueenList[nQueen].field_C <= 0)
                            {
                                if (nWaspCount < 100)
                                {
                                    QueenList[nQueen].nAction = 6;
                                    QueenList[nQueen].field_2 = 0;
                                    return;
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

                    if ((nMov & 0xC000) == 0xC000)
                    {
                        if ((si == 2) && ((nMov & 0x3FFF) == nTarget))
                        {
                            runlist_DamageEnemy(nTarget, nSprite, 5);
                        }
                        else
                        {
                            sprite[nSprite].ang += 256;
                            sprite[nSprite].ang &= kAngleMask;

                            SetQueenSpeed(nSprite, si);
                        }
                    }
                    else if ((nMov & 0xC000) == 0x8000)
                    {
                        sprite[nSprite].ang += 256;
                        sprite[nSprite].ang &= kAngleMask;

                        SetQueenSpeed(nSprite, si);
                    }

                    // loc_35BD2
                    if (nAction && nTarget != -1)
                    {
                        if (!(sprite[nTarget].cstat & 0x101))
                        {
                            QueenList[nQueen].nAction = 0;
                            QueenList[nQueen].field_2 = 0;
                            QueenList[nQueen].field_C = 100;
                            QueenList[nQueen].nTarget = -1;

                            sprite[nSprite].xvel = 0;
                            sprite[nSprite].yvel = 0;
                            return;
                        }
                    }

                    return;
                }

                case 4:
                case 5:
                {
                    if (var_18 && QueenList[nQueen].field_10 <= 0)
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

                            if (si)
                            {
                                BuildQueenEgg(nQueen, 0);
                            }
                            else
                            {
                                BuildBullet(nSprite, 12, 0, 0, -1, sprite[nSprite].ang, nTarget + 10000, 1);
                            }
                        }
                    }

                    return;
                }

                case 6:
                {
                    if (var_18)
                    {
                        BuildQueenEgg(nQueen, 1);
                        QueenList[nQueen].nAction = 3;
                        QueenList[nQueen].field_C = RandomSize(6) + 60;
                    }

                    return;
                }

                case 7:
                {
                    if (var_18)
                    {
                        QueenList[nQueen].nAction = 0;
                        QueenList[nQueen].field_2 = 0;
                    }

                    return;
                }

                case 8:
                case 9:
                {
                    if (var_18)
                    {
                        if (nAction != 9)
                        {
                            QueenList[nQueen].nAction++;
                            return;
                        }

                        QueenList[nQueen].field_C--;
                        if (QueenList[nQueen].field_C <= 0)
                        {
                            sprite[nSprite].cstat = 0;

                            for (int i = 0; i < 20; i++)
                            {
                                short nChunkSprite = BuildCreatureChunk(nSprite, seq_GetSeqPicnum(kSeqQueen, 57, 0)) & 0xFFFF;

                                sprite[nChunkSprite].picnum = kTile3117 + (i % 3);
                                sprite[nChunkSprite].yrepeat = 100;
                                sprite[nChunkSprite].xrepeat = 100;
                            }

                            short nChunkSprite = BuildCreatureChunk(nSprite, seq_GetSeqPicnum(kSeqQueen, 57, 0));

                            sprite[nChunkSprite].picnum = kTile3126;
                            sprite[nChunkSprite].yrepeat = 100;
                            sprite[nChunkSprite].xrepeat = 100;
                            PlayFXAtXYZ(
                                StaticSound[40],
                                sprite[nSprite].x,
                                sprite[nSprite].y,
                                sprite[nSprite].z,
                                sprite[nSprite].sectnum);
                            BuildQueenHead(nQueen);

                            QueenList[nQueen].nAction++;
                        }
                    }

                    return;
                }

                case 10:
                {
                    sprite[nSprite].cstat &= 0xFEFE;
                    return;
                }
            }
        }
    }
}
