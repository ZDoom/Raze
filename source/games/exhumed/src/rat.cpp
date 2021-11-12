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

short nMinChunk;
short nPlayerPic;
short nMaxChunk;

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

void SetRatVel(spritetype* pSprite)
{
    pSprite->xvel = bcos(pSprite->ang, -2);
    pSprite->yvel = bsin(pSprite->ang, -2);
}

void BuildRat(DExhumedActor* pActor, int x, int y, int z, int nSector, int nAngle)
{
    spritetype* pSprite;
    if (pActor == nullptr)
    {
        pActor = insertActor(nSector, 108);
        pSprite = &pActor->s();
    }
    else
    {
        pSprite = &pActor->s();
        x = pSprite->x;
        y = pSprite->y;
        z = pSprite->z;
        nAngle = pSprite->ang;

        ChangeActorStat(pActor, 108);
    }

    pSprite->cstat = 0x101;
    pSprite->shade = -12;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->picnum = 1;
    pSprite->pal = pSprite->sector()->ceilingpal;
    pSprite->clipdist = 30;
    pSprite->ang = nAngle;
    pSprite->xrepeat = 50;
    pSprite->yrepeat = 50;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->hitag = 0;
    pSprite->extra = -1;

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

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, pActor, 0x240000);

    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x240000);
}

DExhumedActor* FindFood(DExhumedActor* pActor)
{
    auto pSprite = &pActor->s();
    int nSector = pSprite->sectnum;
    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;

    int z2 = (z + sector[nSector].ceilingz) / 2;

    if (nChunkTotal)
    {
        auto pActor2 = nChunkSprite[RandomSize(7) % nChunkTotal];
		if (pActor2 != nullptr)
		{
			auto pSprite2 = &pActor2->s();
            if (cansee(x, y, z2, nSector, pSprite2->x, pSprite2->y, pSprite2->z, pSprite2->sectnum)) {
                return pActor2;
            }
        }
    }

    if (!nBodyTotal) {
        return nullptr;
    }

    auto pActor2 = nBodySprite[RandomSize(7) % nBodyTotal];
    if (pActor2 != nullptr)
    {
		auto pSprite2 = &pActor2->s();
        if (nPlayerPic == pSprite2->picnum)
        {
            if (cansee(x, y, z, nSector, pSprite2->x, pSprite2->y, pSprite2->z, pSprite2->sectnum)) {
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
    auto pSprite = &pActor->s();

    if (ev->nDamage)
    {
        pSprite->cstat = 0;
        pSprite->xvel = 0;
        pSprite->yvel = 0;
        pActor->nAction = 3;
        pActor->nFrame = 0;
    }
}

void AIRat::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    short nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqRat] + RatSeq[nAction].a, pActor->nFrame, RatSeq[nAction].b);
}


void AIRat::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    short nAction = pActor->nAction;
    auto pSprite = &pActor->s();

    bool bVal = false;

    int nSeq = SeqOffsets[kSeqRat] + RatSeq[nAction].a;
    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

    seq_MoveSequence(pActor, nSeq, pActor->nFrame);

    pActor->nFrame++;
    if (pActor->nFrame >= SeqSize[nSeq])
    {
        bVal = true;
        pActor->nFrame = 0;
    }

    auto pTarget = pActor->pTarget;

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

        int xVal = abs(pSprite->x - pTarget->s().x);
        int yVal = abs(pSprite->y - pTarget->s().y);

        if (xVal > 50 || yVal > 50)
        {
            pActor->nAction = 2;
            pActor->nFrame = 0;
            pActor->pTarget = nullptr;

            pSprite->xvel = 0;
            pSprite->yvel = 0;
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
            SetRatVel(pSprite);

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

            pSprite->xvel = 0;
            pSprite->yvel = 0;
        }

        MoveCreature(pActor);

        int xVal = abs(pSprite->x - pTarget->s().x);
        int yVal = abs(pSprite->y - pTarget->s().y);

        if (xVal >= 50 || yVal >= 50)
        {
            pActor->nCount--;
            if (pActor->nCount < 0)
            {
                PlotCourseToSprite(pActor, pTarget);
                SetRatVel(pSprite);

                pActor->nCount = 32;
            }

            return;
        }

        pActor->nAction = 0;
        pActor->nFrame = 0;
        pActor->nPhase = RandomSize(3);

        pSprite->xvel = 0;
        pSprite->yvel = 0;
        return;
    }
    case 2:
    {
        if (pSprite->xvel || pSprite->yvel || pSprite->zvel) {
            MoveCreature(pActor);
        }

        pActor->nCount--;
        if (pActor->nCount <= 0)
        {
            pActor->pTarget = FindFood(pActor);

            if (pActor->pTarget == nullptr)
            {
                pActor->nCount = RandomSize(6);
                if (pSprite->xvel || pSprite->yvel)
                {
                    pSprite->xvel = 0;
                    pSprite->yvel = 0;
                    return;
                }

                pSprite->ang = RandomSize(11);
                SetRatVel(pSprite);
                return;
            }
            else
            {
                PlotCourseToSprite(pActor, pActor->pTarget);
                SetRatVel(pSprite);
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
            runlist_DoSubRunRec(pSprite->owner);
            runlist_FreeRun(pSprite->lotag - 1);
            runlist_SubRunRec(pActor->nRun);

            pSprite->cstat = 0x8000;
            DeleteActor(pActor);
        }
        return;
    }
    }
}

END_PS_NS
