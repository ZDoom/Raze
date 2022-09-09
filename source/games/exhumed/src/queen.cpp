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
    TObjPtr<DExhumedActor*> pActor;
    TObjPtr<DExhumedActor*> pTarget;
    int16_t nHealth;
    int16_t nFrame;
    int16_t nAction;
    int16_t nAction2;
    int16_t nIndex;
    int16_t nIndex2;
    int16_t nChannel;
};

struct Egg
{
    TObjPtr<DExhumedActor*> pActor;
    TObjPtr<DExhumedActor*> pTarget;
    int16_t nHealth;
    int16_t nFrame;
    int16_t nAction;
    int16_t nRun;
    int16_t nCounter;
};

struct Head
{
    TObjPtr<DExhumedActor*> pActor;
    TObjPtr<DExhumedActor*> pTarget;
    int16_t nHealth;
    int16_t nFrame;
    int16_t nAction;
    int16_t nRun;
    int16_t nIndex;
    int16_t nIndex2;
    int16_t nChannel;
};

FreeListArray<Egg, kMaxEggs> QueenEgg;

int QueenCount = 0;
int nQHead = 0;
int nHeadVel;
int nVelShift;

TObjPtr<DExhumedActor*> tailspr[kMaxTails];


Queen QueenList[kMaxQueens];
Head QueenHead;

DVector3 MoveQP[25];
sectortype* MoveQS[25];
DAngle MoveQA[25];


size_t MarkQueen()
{
    GC::Mark(QueenList[0].pActor);
    GC::Mark(QueenList[0].pTarget);
    GC::Mark(QueenHead.pActor);
    GC::Mark(QueenHead.pTarget);
    for (int i = 0; i < kMaxEggs; i++)
    {
        GC::Mark(QueenEgg[i].pActor);
        GC::Mark(QueenEgg[i].pTarget);
    }
    return 4 + 2 * kMaxEggs;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, Queen& w, Queen* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("health", w.nHealth)
            ("frame", w.nFrame)
            ("action", w.nAction)
            ("sprite", w.pActor)
            ("target", w.pTarget)
            ("ata", w.nAction2)
            ("atc", w.nIndex)
            ("at10", w.nIndex2)
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
            ("sprite", w.pActor)
            ("target", w.pTarget)
            ("runptr", w.nRun)
            ("atc", w.nCounter)
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
            ("sprite", w.pActor)
            ("target", w.pTarget)
            ("at8", w.nRun)
            ("atc", w.nIndex)
            ("tails", w.nIndex2)
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
                QueenEgg[i].nRun = -1;
            }
            arc("qhead", nQHead)
                ("headvel", nHeadVel)
                ("velshift", nVelShift)
                ("head", QueenHead)
                .Array("tailspr", tailspr, countof(tailspr))
                ("queen", QueenList[0])
                ("eggs", QueenEgg)
                .Array("moveqv", MoveQP, countof(MoveQP))
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
        QueenEgg[i].nRun = -1;
    }
}

int GrabEgg()
{
    auto egg = QueenEgg.Get();
    if (egg == -1) return -1;
    return egg;
}

void BlowChunks(DExhumedActor* pActor)
{
    for (int i = 0; i < 4; i++)
    {
        BuildCreatureChunk(pActor, seq_GetSeqPicnum(16, i + 41, 0));
    }
}

void DestroyEgg(int nEgg)
{
    DExhumedActor* pActor = QueenEgg[nEgg].pActor;
    if (!pActor) return;

    if (QueenEgg[nEgg].nAction != 4)
    {
        BuildAnim(nullptr, 34, 0, pActor->spr.pos, pActor->sector(), pActor->spr.xrepeat, 4);
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            BuildCreatureChunk(pActor, seq_GetSeqPicnum(kSeqQueenEgg, (i % 2) + 24, 0));
        }
    }

    runlist_DoSubRunRec(pActor->spr.intowner);
    runlist_DoSubRunRec(pActor->spr.lotag - 1);
    runlist_SubRunRec(QueenEgg[nEgg].nRun);

    QueenEgg[nEgg].nRun = -1;

    DeleteActor(pActor);
    QueenEgg.Release(nEgg);
}

void DestroyAllEggs()
{
    for (int i = 0; i < kMaxEggs; i++)
    {
        if (QueenEgg[i].nRun > -1)
        {
            DestroyEgg(i);
        }
    }
}

void SetHeadVel(DExhumedActor* pActor)
{
	pActor->vel.XY() = pActor->spr.angle.ToVector() * 1024 * (1 << nVelShift);
}

Collision QueenAngleChase(DExhumedActor* pActor, DExhumedActor* pActor2, int val1, int val2)
{
    int nAngle;

    if (pActor2 == nullptr)
    {
        pActor->angle2 = 0;
        nAngle = pActor->int_ang();
    }
    else
    {
        int nTileY = (tileHeight(pActor2->spr.picnum) * pActor2->spr.yrepeat) * 2;

		auto vect = pActor2->spr.pos.XY() - pActor->spr.pos.XY();
        int nMyAngle = getangle(vect);

        int edx = ((pActor2->int_pos().Z - nTileY) - pActor->int_pos().Z) >> 8;

        int nSqrt = int(vect.Length() * worldtoint);

        int var_14 = getangle(nSqrt, edx);

        int nAngDelta = AngleDelta(pActor->int_ang(), nMyAngle, 1024);

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

        nAngle = (nAngDelta + pActor->int_ang()) & kAngleMask;

        pActor->angle2 = (AngleDelta(pActor->angle2, var_14, 24) + pActor->angle2) & kAngleMask;
    }

    pActor->set_int_ang(nAngle);

    int da = pActor->angle2;
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

    return movesprite(pActor, v26 >> 2, v27 >> 2, bsin(bobangle, -5) + (nSqrt >> 13), 0, 0, CLIPMASK1);
}

int DestroyTailPart()
{
    if (!QueenHead.nIndex2) {
        return 0;
    }

    DExhumedActor* pActor = tailspr[--QueenHead.nIndex2];
    if (!pActor) return 0;

    BlowChunks(pActor);
    BuildExplosion(pActor);

    for (int i = 0; i < 5; i++)
    {
        int nHeight = GetActorHeight(pActor);
        BuildLavaLimb(pActor, i, nHeight);
    }

    DeleteActor(pActor);
    return 1;
}

void BuildTail()
{
    auto head = QueenHead.pActor;

	auto pos = head->spr.pos;
    auto pSector =head->sector();

    int i;

    for (i = 0; i < kMaxTails; i++)
    {
        auto pTailActor = insertActor(pSector, 121);
        tailspr[i] = pTailActor;

        pTailActor->spr.lotag = runlist_HeadRun() + 1;
        pTailActor->spr.intowner = runlist_AddRunRec(pTailActor->spr.lotag - 1, (i + 1), 0x1B0000);
        pTailActor->spr.shade = -12;
        pTailActor->spr.hitag = 0;
        pTailActor->spr.cstat = 0;
        pTailActor->set_const_clipdist(100);
        pTailActor->spr.xrepeat = 80;
        pTailActor->spr.yrepeat = 80;
        pTailActor->spr.picnum = 1;
        pTailActor->spr.pal = pTailActor->sector()->ceilingpal;
        pTailActor->spr.xoffset = 0;
        pTailActor->spr.yoffset = 0;
		pTailActor->spr.pos = pos;
        pTailActor->spr.extra = -1;
    }

    for (i = 0; i < 24 + 1; i++)
    {
        MoveQP[i] = pos;
        assert(pSector);
        MoveQS[i] = pSector;
    }

    nQHead = 0;
    QueenHead.nIndex2 = 7;
}

void BuildQueenEgg(int nQueen, int nVal)
{
    int nEgg = GrabEgg();
    if (nEgg < 0) {
        return;
    }

    DExhumedActor* pActor = QueenList[nQueen].pActor;
    if (!pActor) return;

    auto pSector =pActor->sector();

    auto pActor2 = insertActor(pSector, 121);

	pActor2->spr.pos = DVector3(pActor->spr.pos.XY(), pSector->floorz);
    pActor2->spr.pal = 0;
    pActor2->set_const_clipdist(50);
    pActor2->spr.xoffset = 0;
    pActor2->spr.yoffset = 0;
    pActor2->spr.shade = -12;
    pActor2->spr.picnum = 1;
	pActor2->spr.angle = pActor->spr.angle + RandomAngle9() - DAngle45;
    pActor2->backuppos();

    if (!nVal)
    {
        pActor2->spr.xrepeat = 30;
        pActor2->spr.yrepeat = 30;
		pActor2->vel.XY() = pActor2->spr.angle.ToVector() * 1024;
        pActor2->vel.Z = -6000 / 256.;
        pActor2->spr.cstat = 0;
    }
    else
    {
        pActor2->spr.xrepeat = 60;
        pActor2->spr.yrepeat = 60;
        pActor2->vel.X = 0;
        pActor2->vel.Y = 0;
        pActor2->vel.Z = -2000 / 256.;
        pActor2->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    }

    pActor2->spr.lotag = runlist_HeadRun() + 1;
    pActor2->spr.extra = -1;
    pActor2->spr.hitag = 0;

    GrabTimeSlot(3);

    QueenEgg[nEgg].nHealth = 200;
    QueenEgg[nEgg].nFrame = 0;
    QueenEgg[nEgg].pActor = pActor2;
    QueenEgg[nEgg].pTarget = QueenList[nQueen].pTarget;

    if (nVal)
    {
        nVal = 4;
        QueenEgg[nEgg].nCounter = 200;
    }

    QueenEgg[nEgg].nAction = nVal;

    pActor2->spr.intowner = runlist_AddRunRec(pActor2->spr.lotag - 1, nEgg, 0x1D0000);
    QueenEgg[nEgg].nRun = runlist_AddRunRec(NewRun, nEgg, 0x1D0000);
}

void AIQueenEgg::Tick(RunListEvent* ev)
{
    int nEgg = RunData[ev->nRun].nObjIndex;
    Egg* pEgg = &QueenEgg[nEgg];
    DExhumedActor* pActor = pEgg->pActor;
    if (!pActor) return;
    int nAction = pEgg->nAction;

    DExhumedActor* pTarget = nullptr;

    bool bVal = false;

    if (pEgg->nHealth <= 0)
    {
        DestroyEgg(nEgg);
        return;
    }

    if (nAction == 0 || nAction == 4) {
        Gravity(pActor);
    }

    int nSeq = SeqOffsets[kSeqQueenEgg] + EggSeq[nAction].a;

    pActor->spr.picnum = seq_GetSeqPicnum2(nSeq, pEgg->nFrame);

    if (nAction != 4)
    {
        seq_MoveSequence(pActor, nSeq, pEgg->nFrame);

        pEgg->nFrame++;
        if (pEgg->nFrame >= SeqSize[nSeq])
        {
            pEgg->nFrame = 0;
            bVal = true;
        }

        DExhumedActor* enemy = pEgg->pActor;
        pTarget = UpdateEnemy(&enemy);
        pEgg->pActor = enemy;
        pEgg->pTarget = pTarget;

        if (pTarget && (pTarget->spr.cstat & CSTAT_SPRITE_BLOCK_ALL) == 0)
        {
            pEgg->pTarget = nullptr;
            pEgg->nAction = 0;
        }
        else
        {
            pTarget = FindPlayer(pActor, 1000, true);
            pEgg->pTarget = pTarget;
        }
    }

    switch (nAction)
    {
    case 0:
    {
        auto nMov = MoveCreature(pActor);

        if (nMov.exbits & kHitAux2)
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
            int nAngle;

            switch (nMov.type)
            {
            default:
                return;
            case kHitWall:
                nAngle = GetWallNormal(nMov.hitWall);
                break;
            case kHitSprite:
                nAngle = nMov.actor()->int_ang();
                break;
            }

            pActor->set_int_ang(nAngle);
			pActor->vel.XY() = pActor->spr.angle.ToVector() * 512;
        }

        break;
    }

    case 1:
    {
        if (bVal)
        {
            pEgg->nAction = 3;
            pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
        }
        break;
    }

    case 2:
    case 3:
    {
        auto nMov = QueenAngleChase(pActor, pTarget, nHeadVel, 64);

        switch (nMov.type)
        {
        case kHitSprite:
            if (nMov.actor()->spr.statnum != 121)
            {
                runlist_DamageEnemy(nMov.actor(), pActor, 5);
            }
            [[fallthrough]];
        case kHitWall:
            pActor->spr.angle = DAngle45 + DAngle90 + RandomAngle9();
            pActor->VelFromAngle(-3);
            pActor->vel.Z = (-RandomSize(5)) / 256.;
            break;
        }

        return;
    }

    case 4:
    {
        auto nMov = MoveCreature(pActor);

        if (nMov.exbits & kHitAux2)
        {
            pActor->vel.Z = -(pActor->vel.Z - 1);
            if (pActor->vel.Z < -2)
            {
                pActor->vel.Z = 0;
            }
        }

        pEgg->nCounter--;
        if (pEgg->nCounter <= 0)
        {
            auto pWaspSprite = BuildWasp(nullptr, pActor->spr.pos, pActor->sector(), pActor->spr.angle, true);
            pActor->spr.pos.Z = pWaspSprite->spr.pos.Z;

            DestroyEgg(nEgg);
        }
        break;
    }
    }
}

void AIQueenEgg::RadialDamage(RunListEvent* ev)
{
    int nEgg = RunData[ev->nRun].nObjIndex;
    Egg* pEgg = &QueenEgg[nEgg];
    DExhumedActor* pActor = pEgg->pActor;
    if (!pActor) return;

    if (ev->pRadialActor->spr.statnum != 121 && (pActor->spr.cstat & CSTAT_SPRITE_BLOCK_ALL) != 0)
    {
        int nDamage = runlist_CheckRadialDamage(pActor);

        pEgg->nHealth -= nDamage;
    }
}

void AIQueenEgg::Damage(RunListEvent* ev)
{
    int nEgg = RunData[ev->nRun].nObjIndex;
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
    int nEgg = RunData[ev->nRun].nObjIndex;
    Egg* pEgg = &QueenEgg[nEgg];
    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqQueenEgg] + EggSeq[pEgg->nAction].a, pEgg->nFrame, EggSeq[pEgg->nAction].b);
}

void BuildQueenHead(int nQueen)
{
    DExhumedActor* pActor = QueenList[nQueen].pActor;
    if (!pActor) return;

    int nAngle = pActor->int_ang();
    auto pSector = pActor->sector();

    auto pActor2 = insertActor(pSector, 121);

	pActor2->spr.pos.XY() = pActor->spr.pos.XY();
	pActor2->spr.pos.Z = pSector->floorz;
    pActor2->set_const_clipdist(70);
    pActor2->spr.xrepeat = 80;
    pActor2->spr.yrepeat = 80;
    pActor2->spr.cstat = 0;
    pActor2->spr.picnum = 1;
    pActor2->spr.shade = -12;
    pActor2->spr.pal = 0;
    pActor2->spr.xoffset = 0;
    pActor2->spr.yoffset = 0;
    pActor2->set_int_ang(nAngle);

    nVelShift = 2;
    SetHeadVel(pActor2);

    pActor2->vel.Z = -32;
    pActor2->spr.lotag = runlist_HeadRun() + 1;
    pActor2->spr.hitag = 0;
    pActor2->spr.extra = -1;

    GrabTimeSlot(3);

    QueenHead.nHealth = 800;
    QueenHead.nAction = 0;
    QueenHead.pTarget = QueenList[nQueen].pTarget;
    QueenHead.nFrame = 0;
    QueenHead.pActor = pActor2;
    QueenHead.nIndex = 0;
    QueenHead.nChannel = QueenList[nQueen].nChannel;

    pActor2->spr.intowner = runlist_AddRunRec(pActor2->spr.lotag - 1, 0, 0x1B0000);

    QueenHead.nRun = runlist_AddRunRec(NewRun, 0, 0x1B0000);
    QueenHead.nIndex2 = 0;
}

void AIQueenHead::Tick(RunListEvent* ev)
{
    DExhumedActor* pActor = QueenHead.pActor;
    if (!pActor) return;

    int nAction = QueenHead.nAction;
    int nHd;
    int var_14 = 0;

    if (nAction == 0) {
        Gravity(pActor);
    }

    int nSeq = SeqOffsets[kSeqQueen] + HeadSeq[QueenHead.nAction].a;

    seq_MoveSequence(pActor, nSeq, QueenHead.nFrame);

    pActor->spr.picnum = seq_GetSeqPicnum2(nSeq, QueenHead.nFrame);

    QueenHead.nFrame++;
    if (QueenHead.nFrame >= SeqSize[nSeq])
    {
        QueenHead.nFrame = 0;
        var_14 = 1;
    }

    DExhumedActor* pTarget = QueenHead.pTarget;

    if (pTarget)
    {
        if (!(pTarget->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
        {
            pTarget = nullptr;
            QueenHead.pTarget = pTarget;
        }
    }
    else
    {
        pTarget = FindPlayer(pActor, 1000);
        QueenHead.pTarget = pTarget;
    }

    switch (nAction)
    {
    case 0:
        if (QueenHead.nIndex > 0)
        {
            QueenHead.nIndex--;
            if (QueenHead.nIndex == 0)
            {
                BuildTail();

                QueenHead.nAction = 6;
                nHeadVel = 800;
                pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
            }
            else if (QueenHead.nIndex < 60)
            {
                pActor->spr.shade--;
            }
        }
        else
        {
            auto nMov = MoveCreature(pActor);

            // original BUG - this line doesn't exist in original code?
            int nNewAng = pActor->int_ang();

            if (nMov.exbits == 0)
            {
                if (nMov.type == kHitSprite) nNewAng = nMov.actor()->int_ang();
                else if (nMov.type == kHitWall) nNewAng = GetWallNormal(nMov.hitWall);
            }
            else if (nMov.exbits == kHitAux2)
            {
				pActor->vel.Z *= -0.5;

                if (pActor->vel.Z > -1)
                {
                    nVelShift = 100;
                    pActor->vel.Z = 0;
                }
            }

            // original BUG - var_18 isn't being set if the check above == 0x20000 ?
            pActor->set_int_ang(nNewAng);
            nVelShift++;

            if (nVelShift < 5)
            {
                SetHeadVel(pActor);
            }
            else
            {
                pActor->vel.X = 0;
                pActor->vel.Y = 0;

                if (pActor->vel.Z == 0)
                {
                    QueenHead.nIndex = 120;
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
        [[fallthrough]];

    case 1:
        if ((pTarget->spr.pos.Z - 200) > pActor->spr.pos.Z)
        {
            QueenHead.nAction = 4;
            QueenHead.nFrame = 0;
        }
        else
        {
			pActor->spr.pos.Z -= 8;
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

        if (pTarget)
        {
            auto nMov = QueenAngleChase(pActor, pTarget, nHeadVel, 64);

            if (nMov.type == kHitSprite)
            {
                if (nMov.actor() == pTarget)
                {
                    runlist_DamageEnemy(pTarget, pActor, 10);
                    D3PlayFX(StaticSound[kSoundQTail] | 0x2000, pActor);

					pActor->spr.angle += DAngle45 + DAngle90 + RandomAngle9();
                    pActor->norm_ang();

                    pActor->vel.Z = ((-20) - RandomSize(6)) / 256.;

                    SetHeadVel(pActor);
                }
            }
        }

        // switch break. MoveQS stuff?
    __MOVEQS:
        MoveQP[nQHead] = pActor->spr.pos;
        assert(pActor->sector());
        MoveQS[nQHead] = pActor->sector();
        MoveQA[nQHead] = pActor->spr.angle;

        nHd = nQHead;

        for (int i = 0; i < QueenHead.nIndex2; i++)
        {
            nHd -= 3;
            if (nHd < 0) {
                nHd += (24 + 1); // TODO - enum/define for these
                //assert(nHd < 24 && nHd >= 0);
            }

            auto headSect = MoveQS[nHd];
            DExhumedActor* pTActor = tailspr[i];
            if (pTActor)
            {
                if (headSect != pTActor->sector())
                {
                    assert(headSect);
                    ChangeActorSect(pTActor, headSect);
                }

                pTActor->spr.pos = MoveQP[nHd];
                pTActor->spr.angle = MoveQA[nHd];
            }
        }

        nQHead++;
        if (nQHead >= 25)
        {
            nQHead = 0;
        }

        break;

    case 5:
        QueenHead.nIndex--;
        if (QueenHead.nIndex <= 0)
        {
            QueenHead.nIndex = 3;

            if (QueenHead.nIndex2--)
            {
                if (QueenHead.nIndex2 >= 15 || QueenHead.nIndex2 < 10)
                {
					auto pos = pActor->spr.pos;
                    auto pSector =pActor->sector();
                    int nAngle = RandomSize(11) & kAngleMask;

                    pActor->spr.xrepeat = 127 - QueenHead.nIndex2;
                    pActor->spr.yrepeat = 127 - QueenHead.nIndex2;

                    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;

                    // DEMO-TODO: in disassembly angle was used without masking and thus causing OOB issue.
                    // This behavior probably would be needed emulated for demo compatibility
                    int dx = bcos(nAngle, 10);
                    int dy = bsin(nAngle, 10);
                    int dz = (RandomSize(5) - RandomSize(5)) << 7;

                    movesprite(pActor, dx, dy, dz, 0, 0, CLIPMASK1);

                    BlowChunks(pActor);
                    BuildExplosion(pActor);

                    ChangeActorSect(pActor, pSector);

					pActor->spr.pos = pos;

                    if (QueenHead.nIndex2 < 10) {
                        for (int i = (10 - QueenHead.nIndex2) * 2; i > 0; i--)
                        {
                            BuildLavaLimb(pActor, i, GetActorHeight(pActor));
                        }
                    }
                }
            }
            else
            {
                BuildExplosion(pActor);

                int i;

                for (i = 0; i < 10; i++)
                {
                    BlowChunks(pActor);
                }

                for (i = 0; i < 20; i++)
                {
                    BuildLavaLimb(pActor, i, GetActorHeight(pActor));
                }

                runlist_SubRunRec(pActor->spr.intowner);
                runlist_SubRunRec(QueenHead.nRun);
                DeleteActor(pActor);
                runlist_ChangeChannel(QueenHead.nChannel, 1);
            }
        }
        break;
    }
}

void AIQueenHead::RadialDamage(RunListEvent* ev)
{
    if (ev->pRadialActor->spr.statnum != 121 && (QueenHead.pActor->spr.cstat & CSTAT_SPRITE_BLOCK_ALL) != 0)
    {
        ev->nDamage = runlist_CheckRadialDamage(QueenHead.pActor);
        if (ev->nDamage) Damage(ev);
    }
}

void AIQueenHead::Damage(RunListEvent* ev)
{
    DExhumedActor* pActor = QueenHead.pActor;
    if (!pActor) return;

    if (QueenHead.nHealth > 0 && ev->nDamage != 0)
    {
        QueenHead.nHealth -= ev->nDamage;

        if (!RandomSize(4))
        {
            QueenHead.pTarget = ev->pOtherActor;
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
                QueenHead.nIndex = 0;
                QueenHead.nIndex2 = 80;
                pActor->spr.cstat = 0;
            }
        }
    }
}

void AIQueenHead::Draw(RunListEvent* ev)
{
    int nHead = RunData[ev->nRun].nObjIndex;
    int nAction = QueenHead.nAction;

    int nSeq = SeqOffsets[kSeqQueen];

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

void BuildQueen(DExhumedActor* pActor, const DVector3& pos, sectortype* pSector, DAngle nAngle, int nChannel)
{
    QueenCount--;

    int nQueen = QueenCount;
    if (nQueen < 0) {
        return;
    }

    if (pActor == nullptr)
    {
        pActor = insertActor(pSector, 121);
		pActor->spr.pos = pos;
	}
	else
	{
		ChangeActorStat(pActor, 121);
		pActor->spr.pos.Z = pActor->sector()->floorz;
        nAngle = pActor->spr.angle;
    }

    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pActor->spr.pal = 0;
    pActor->spr.shade = -12;
    pActor->set_const_clipdist(100);
    pActor->spr.xrepeat = 80;
    pActor->spr.yrepeat = 80;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.picnum = 1;
    pActor->spr.angle = nAngle;
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.extra = -1;
    pActor->spr.hitag = 0;

    GrabTimeSlot(3);

    QueenList[nQueen].nAction = 0;
    QueenList[nQueen].nHealth = 4000;
    QueenList[nQueen].nFrame = 0;
    QueenList[nQueen].pActor = pActor;
    QueenList[nQueen].pTarget = nullptr;
    QueenList[nQueen].nAction2 = 0;
    QueenList[nQueen].nIndex2 = 5;
    QueenList[nQueen].nIndex = 0;
    QueenList[nQueen].nChannel = nChannel;

    nHeadVel = 800;

    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, nQueen, 0x1A0000);

    runlist_AddRunRec(NewRun, nQueen, 0x1A0000);

    nCreaturesTotal++;
}

void SetQueenSpeed(DExhumedActor* pActor, int nSpeed)
{
    pActor->VelFromAngle(-(2 - nSpeed));
}

void AIQueen::Tick(RunListEvent* ev)
{
    int nQueen = RunData[ev->nRun].nObjIndex;
    assert(nQueen >= 0 && nQueen < kMaxQueens);

    DExhumedActor* pActor = QueenList[nQueen].pActor;
    if (!pActor) return;
    int nAction = QueenList[nQueen].nAction;
    int si = QueenList[nQueen].nAction2;
    DExhumedActor* pTarget = QueenList[nQueen].pTarget;

    bool bVal = false;

    if (si < 3) {
        Gravity(pActor);
    }

    int nSeq = SeqOffsets[kSeqQueen] + QueenSeq[nAction].a;

    pActor->spr.picnum = seq_GetSeqPicnum2(nSeq, QueenList[nQueen].nFrame);

    seq_MoveSequence(pActor, nSeq, QueenList[nQueen].nFrame);

    QueenList[nQueen].nFrame++;
    if (QueenList[nQueen].nFrame >= SeqSize[nSeq])
    {
        QueenList[nQueen].nFrame = 0;
        bVal = true;
    }

    int nFlag = FrameFlag[SeqBase[nSeq] + QueenList[nQueen].nFrame];

    if (pActor != nullptr)
    {
        if (nAction < 7)
        {
            if (!(pActor->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
            {
                pTarget = nullptr;
                QueenList[nQueen].pTarget = nullptr;
                QueenList[nQueen].nAction = 0;
            }
        }
    }
    switch (nAction)
    {
    case 0:
    {
        if (pTarget == nullptr)
        {
            pTarget = FindPlayer(pActor, 60);
        }

        if (pTarget)
        {
            QueenList[nQueen].nAction = QueenList[nQueen].nAction2 + 1;
            QueenList[nQueen].nFrame = 0;
            QueenList[nQueen].pTarget = pTarget;
            QueenList[nQueen].nIndex = RandomSize(7);

            SetQueenSpeed(pActor, si);
        }
        break;
    }

    case 6:
    {
        if (bVal)
        {
            BuildQueenEgg(nQueen, 1);
            QueenList[nQueen].nAction = 3;
            QueenList[nQueen].nIndex = RandomSize(6) + 60;
        }

        break;
    }

    case 1:
    case 2:
    case 3:
    {
        QueenList[nQueen].nIndex--;

        if ((nQueen & 0x1F) == (totalmoves & 0x1F))
        {
            if (si < 2)
            {
                if (QueenList[nQueen].nIndex <= 0)
                {
                    QueenList[nQueen].nFrame = 0;
                    pActor->vel.X = 0;
                    pActor->vel.Y = 0;
                    QueenList[nQueen].nAction = si + 4;
                    QueenList[nQueen].nIndex = RandomSize(6) + 30;
                    break;
                }
                else
                {
                    if (QueenList[nQueen].nIndex2 < 5)
                    {
                        QueenList[nQueen].nIndex2++;
                    }

                    // then to PLOTSPRITE
                }
            }
            else
            {
                if (QueenList[nQueen].nIndex <= 0)
                {
                    if (Counters[kCountWasp] < 100)
                    {
                        QueenList[nQueen].nAction = 6;
                        QueenList[nQueen].nFrame = 0;
                        break;
                    }
                    else
                    {
                        QueenList[nQueen].nIndex = 30000;
                        // then to PLOTSPRITE
                    }
                }
            }

            // loc_35B4B
            PlotCourseToSprite(pActor, pTarget);
            SetQueenSpeed(pActor, si);
        }

        auto nMov = MoveCreatureWithCaution(pActor);

        switch (nMov.type)
        {
        case kHitSprite:
            if ((si == 2) && (nMov.actor() == pTarget))
            {
                runlist_DamageEnemy(pTarget, pActor, 5);
                break;
            }
            [[fallthrough]];
        case 0x8000:
			pActor->spr.angle += DAngle45;
            pActor->norm_ang();

            SetQueenSpeed(pActor, si);
            break;
        }

        // loc_35BD2
        if (nAction && pTarget != nullptr)
        {
            if (!(pTarget->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
            {
                QueenList[nQueen].nAction = 0;
                QueenList[nQueen].nFrame = 0;
                QueenList[nQueen].nIndex = 100;
                QueenList[nQueen].pTarget = nullptr;

                pActor->vel.X = 0;
                pActor->vel.Y = 0;
            }
        }

        break;
    }

    case 4:
    case 5:
    {
        if (bVal && QueenList[nQueen].nIndex2 <= 0)
        {
            QueenList[nQueen].nAction = 0;
            QueenList[nQueen].nIndex = 15;
        }
        else
        {
            if (nFlag & 0x80)
            {
                QueenList[nQueen].nIndex2--;

                PlotCourseToSprite(pActor, pTarget);

                if (!si)
                {
                    BuildBullet(pActor, 12, -1, pActor->spr.angle, pTarget, 1);
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
                QueenList[nQueen].nIndex--;
                if (QueenList[nQueen].nIndex <= 0)
                {
                    pActor->spr.cstat = 0;

                    for (int i = 0; i < 20; i++)
                    {
                        auto pChunkActor = BuildCreatureChunk(pActor, seq_GetSeqPicnum(kSeqQueen, 57, 0));

                        pChunkActor->spr.picnum = kQueenChunk + (i % 3);
                        pChunkActor->spr.xrepeat = 100;
                        pChunkActor->spr.yrepeat = 100;
                    }

                    auto pChunkActor = BuildCreatureChunk(pActor, seq_GetSeqPicnum(kSeqQueen, 57, 0));

                    pChunkActor->spr.picnum = kTile3126;
                    pChunkActor->spr.yrepeat = 100;
                    pChunkActor->spr.xrepeat = 100;

                    PlayFXAtXYZ(
                        StaticSound[kSound40],
                        pActor->spr.pos);

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
        pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
        break;
    }
    }
}

void AIQueen::RadialDamage(RunListEvent* ev)
{
    int nQueen = RunData[ev->nRun].nObjIndex;
    assert(nQueen >= 0 && nQueen < kMaxQueens);
    DExhumedActor* pActor = QueenList[nQueen].pActor;
    if (!pActor) return;

    if (ev->pRadialActor->spr.statnum != 121 && (pActor->spr.cstat & CSTAT_SPRITE_BLOCK_ALL) != 0)
    {
        ev->nDamage = runlist_CheckRadialDamage(pActor);
        if (ev->nDamage) Damage(ev);
    }
}

void AIQueen::Damage(RunListEvent* ev)
{
    int nQueen = RunData[ev->nRun].nObjIndex;
    assert(nQueen >= 0 && nQueen < kMaxQueens);

    DExhumedActor* pActor = QueenList[nQueen].pActor;
    if (!pActor) return;
    int si = QueenList[nQueen].nAction2;

    if (QueenList[nQueen].nHealth > 0)
    {
        QueenList[nQueen].nHealth -= dmgAdjust(ev->nDamage);

        if (QueenList[nQueen].nHealth <= 0)
        {
            pActor->vel.X = 0;
            pActor->vel.Y = 0;
            pActor->vel.Z = 0;

            QueenList[nQueen].nAction2++;

            switch (QueenList[nQueen].nAction2)
            {
            case 1:
                QueenList[nQueen].nHealth = 4000;
                QueenList[nQueen].nAction = 7;

                BuildAnim(nullptr, 36, 0, pActor->spr.pos.plusZ(-30), pActor->sector(), pActor->spr.xrepeat, 4);
                break;
            case 2:
                QueenList[nQueen].nHealth = 4000;
                QueenList[nQueen].nAction = 7;

                DestroyAllEggs();
                break;
            case 3:
                QueenList[nQueen].nAction = 8;
                QueenList[nQueen].nHealth = 0;
                QueenList[nQueen].nIndex = 5;

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
    int nQueen = RunData[ev->nRun].nObjIndex;
    assert(nQueen >= 0 && nQueen < kMaxQueens);
    int nAction = QueenList[nQueen].nAction;
    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqQueen] + QueenSeq[nAction].a, QueenList[nQueen].nFrame, QueenSeq[nAction].b);
}

END_PS_NS
