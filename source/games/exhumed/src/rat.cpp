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
    pActor->VelFromAngle(-2);
}

void BuildRat(DExhumedActor* pActor, const DVector3& pos, sectortype* pSector, DAngle nAngle)
{
	if (pActor == nullptr)
	{
		pActor = insertActor(pSector, 108);
		pActor->spr.pos = pos;
	}
	else
	{
		ChangeActorStat(pActor, 108);
		pActor->spr.pos.Z = pActor->sector()->floorz;
		nAngle = pActor->spr.angle;
	}

    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pActor->spr.shade = -12;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.picnum = 1;
    pActor->spr.pal = pActor->sector()->ceilingpal;
    pActor->set_const_clipdist(30);
    pActor->spr.angle = nAngle;
    pActor->spr.xrepeat = 50;
    pActor->spr.yrepeat = 50;
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.hitag = 0;
    pActor->spr.extra = -1;

    if (nAngle.Degrees() >= 0) {
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

	if (nChunkTotal)
    {
        DExhumedActor* pActor2 = nChunkSprite[RandomSize(7) % nChunkTotal];
		if (pActor2 != nullptr)
		{
            if (cansee(pActor->spr.pos.plusZ(pSector->ceilingz * 0.5), pSector, pActor2->spr.pos, pActor2->sector())) {
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
            if (cansee(pActor->spr.pos, pSector, pActor2->spr.pos, pActor2->sector())) {
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
        pActor->vel.X = 0;
        pActor->vel.Y = 0;
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
	constexpr double CHECK_DIST = 50/16.;
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

		auto delta = pActor->spr.pos.XY() - pTarget->spr.pos.XY();

        if (abs(delta.X) > CHECK_DIST || abs(delta.Y) >= CHECK_DIST)
        {
            pActor->nAction = 2;
            pActor->nFrame = 0;
            pActor->pTarget = nullptr;

            pActor->vel.X = 0;
            pActor->vel.Y = 0;
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

            pActor->vel.X = 0;
            pActor->vel.Y = 0;
        }

        MoveCreature(pActor);

		auto delta = pActor->spr.pos.XY() - pTarget->spr.pos.XY();

		if (abs(delta.X) > CHECK_DIST || abs(delta.Y) >= CHECK_DIST)
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

        pActor->vel.X = 0;
        pActor->vel.Y = 0;
        return;
    }
    case 2:
    {
        if (pActor->vel.X != 0 || pActor->vel.Y != 0 || pActor->vel.Z != 0) {
            MoveCreature(pActor);
        }

        pActor->nCount--;
        if (pActor->nCount <= 0)
        {
            pActor->pTarget = FindFood(pActor);

            if (pActor->pTarget == nullptr)
            {
                pActor->nCount = RandomSize(6);
                if (pActor->vel.X != 0 || pActor->vel.Y != 0)
                {
                    pActor->vel.X = 0;
                    pActor->vel.Y = 0;
                    return;
                }

                pActor->set_int_ang(RandomSize(11));
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
