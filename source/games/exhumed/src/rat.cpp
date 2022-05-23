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
#include "view.h"
#include "exhumed.h"
#include <assert.h>

BEGIN_PS_NS

int nMinChunk;
int nPlayerPic;
int nMaxChunk;

static actionSeq RatSeq[] = {
    {0, 1},
    {1, 0},
    {1, 0},
    {9, 1},
    {0, 1}
};

void SerializeRat(FSerializer& arc)
{
    if (arc.BeginObject("rat"))
    {
        arc("minchunk", nMinChunk)
            ("maxchunk", nMaxChunk)
            ("playerpic", nPlayerPic)
            .EndObject();
    }
}

void InitRats()
{
    nMinChunk = 9999;
    nMaxChunk = -1;

    for (int i = 122; i <= 131; i++)
    {
        int nPic = seq_GetSeqPicnum(kSeqJoe, i, 0);

        if (nPic < nMinChunk)
            nMinChunk = nPic;

        if (nPic > nMaxChunk)
            nMaxChunk = nPic;
    }

    nPlayerPic = seq_GetSeqPicnum(kSeqJoe, 120, 0);
}

void SetRatVel(DExhumedActor* pActor)
{
    pActor->spr.xvel = bcos(pActor->spr.ang, -2);
    pActor->spr.yvel = bsin(pActor->spr.ang, -2);
}

void BuildRat(DExhumedActor* pActor, int x, int y, int z, sectortype* pSector, int nAngle)
{
    if (pActor == nullptr)
    {
        pActor = insertActor(pSector, 108);
    }
    else
    {
        x = pActor->spr.pos.X;
        y = pActor->spr.pos.Y;
        z = pActor->spr.pos.Z;
        nAngle = pActor->spr.ang;

        ChangeActorStat(pActor, 108);
    }

    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pActor->spr.shade = -12;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.picnum = 1;
    pActor->spr.pal = pActor->sector()->ceilingpal;
    pActor->spr.clipdist = 30;
    pActor->spr.ang = nAngle;
    pActor->spr.xrepeat = 50;
    pActor->spr.yrepeat = 50;
    pActor->spr.xvel = 0;
    pActor->spr.yvel = 0;
    pActor->spr.zvel = 0;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.hitag = 0;
    pActor->spr.extra = -1;

    if (nAngle >= 0) {
        pActor->nAction = 2;
    }
    else {
        pActor->nAction = 4;
    }

    pActor->nFrame = 0;
    pActor->pTarget = nullptr;
    pActor->nCount = RandomSize(5);
    pActor->nPhase = RandomSize(3);

    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x240000);

    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x240000);
}

DExhumedActor* FindFood(DExhumedActor* pActor)
{
    auto pSector = pActor->sector();
    int x = pActor->spr.pos.X;
    int y = pActor->spr.pos.Y;
    int z = pActor->spr.pos.Z;

    int z2 = (z + pSector->ceilingz) / 2;

    if (nChunkTotal)
    {
        DExhumedActor* pActor2 = nChunkSprite[RandomSize(7) % nChunkTotal];
		if (pActor2 != nullptr)
		{
            if (cansee(x, y, z2, pSector, pActor2->spr.pos.X, pActor2->spr.pos.Y, pActor2->spr.pos.Z, pActor2->sector())) {
                return pActor2;
            }
        }
    }

    if (!nBodyTotal) {
        return nullptr;
    }

    DExhumedActor* pActor2 = nBodySprite[RandomSize(7) % nBodyTotal];
    if (pActor2 != nullptr)
    {
        if (nPlayerPic == pActor2->spr.picnum)
        {
            if (cansee(x, y, z, pSector, pActor2->spr.pos.X, pActor2->spr.pos.Y, pActor2->spr.pos.Z, pActor2->sector())) {
                return pActor2;
            }
        }
    }

    return nullptr;
}

void AIRat::RadialDamage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    ev->nDamage = runlist_CheckRadialDamage(pActor);
    Damage(ev);
}

void AIRat::Damage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    if (ev->nDamage)
    {
        pActor->spr.cstat = 0;
        pActor->spr.xvel = 0;
        pActor->spr.yvel = 0;
        pActor->nAction = 3;
        pActor->nFrame = 0;
    }
}

void AIRat::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    int nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqRat] + RatSeq[nAction].a, pActor->nFrame, RatSeq[nAction].b);
}


void AIRat::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    int nAction = pActor->nAction;

    bool bVal = false;

    int nSeq = SeqOffsets[kSeqRat] + RatSeq[nAction].a;
    pActor->spr.picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

    seq_MoveSequence(pActor, nSeq, pActor->nFrame);

    pActor->nFrame++;
    if (pActor->nFrame >= SeqSize[nSeq])
    {
        bVal = true;
        pActor->nFrame = 0;
    }

    DExhumedActor* pTarget = pActor->pTarget;

    Gravity(pActor);

    switch (nAction)
    {
    default:
        return;

    case 0:
    {
        pActor->nCount--;
        if (pActor->nCount > 0) {
            return;
        }

        int xVal = abs(pActor->spr.pos.X - pTarget->spr.pos.X);
        int yVal = abs(pActor->spr.pos.Y - pTarget->spr.pos.Y);

        if (xVal > 50 || yVal > 50)
        {
            pActor->nAction = 2;
            pActor->nFrame = 0;
            pActor->pTarget = nullptr;

            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;
            return;
        }

        pActor->nFrame ^= 1;
        pActor->nCount = RandomSize(5) + 4;
        pActor->nPhase--;

        if (pActor->nPhase <= 0)
        {
            auto pFoodSprite = FindFood(pActor);
            if (pFoodSprite == nullptr) {
                return;
            }

            pActor->pTarget = pFoodSprite;

            PlotCourseToSprite(pActor, pFoodSprite);
            SetRatVel(pActor);

            pActor->nAction = 1;
            pActor->nPhase = 900;
            pActor->nFrame = 0;
        }

        return;
    }
    case 1:
    {
        pActor->nPhase--;

        if (pActor->nPhase <= 0)
        {
            pActor->nAction = 2;
            pActor->nFrame = 0;
            pActor->pTarget = nullptr;

            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;
        }

        MoveCreature(pActor);

        int xVal = abs(pActor->spr.pos.X - pTarget->spr.pos.X);
        int yVal = abs(pActor->spr.pos.Y - pTarget->spr.pos.Y);

        if (xVal >= 50 || yVal >= 50)
        {
            pActor->nCount--;
            if (pActor->nCount < 0)
            {
                PlotCourseToSprite(pActor, pTarget);
                SetRatVel(pActor);

                pActor->nCount = 32;
            }

            return;
        }

        pActor->nAction = 0;
        pActor->nFrame = 0;
        pActor->nPhase = RandomSize(3);

        pActor->spr.xvel = 0;
        pActor->spr.yvel = 0;
        return;
    }
    case 2:
    {
        if (pActor->spr.xvel || pActor->spr.yvel || pActor->spr.zvel) {
            MoveCreature(pActor);
        }

        pActor->nCount--;
        if (pActor->nCount <= 0)
        {
            pActor->pTarget = FindFood(pActor);

            if (pActor->pTarget == nullptr)
            {
                pActor->nCount = RandomSize(6);
                if (pActor->spr.xvel || pActor->spr.yvel)
                {
                    pActor->spr.xvel = 0;
                    pActor->spr.yvel = 0;
                    return;
                }

                pActor->spr.ang = RandomSize(11);
                SetRatVel(pActor);
                return;
            }
            else
            {
                PlotCourseToSprite(pActor, pActor->pTarget);
                SetRatVel(pActor);
                pActor->nAction = 1;
                pActor->nPhase = 900;
                pActor->nFrame = 0;
                return;
            }
        }

        return;
    }
    case 3:
    {
        if (bVal)
        {
            runlist_DoSubRunRec(pActor->spr.intowner);
            runlist_FreeRun(pActor->spr.lotag - 1);
            runlist_SubRunRec(pActor->nRun);

            pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
            DeleteActor(pActor);
        }
        return;
    }
    }
}

END_PS_NS
