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
#include <assert.h>

BEGIN_PS_NS

static actionSeq RoachSeq[] = {
    {24, 0},
    {0,  0},
    {0,  0},
    {16, 0},
    {8,  0},
    {32, 1},
    {42, 1}
};

struct Roach
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nTarget;
    short nRun;
    short nCount;
    short nIndex;
};

// TODO - make nType a bool?
void BuildRoach(int nType, DExhumedActor* pActor, int x, int y, int z, int nSector, int angle)
{
    spritetype* pSprite;
    if (pActor == nullptr)
    {
        pActor = insertActor(nSector, 105);
        pSprite = &pActor->s();
    }
    else
    {
        ChangeActorStat(pActor, 105);
        pSprite = &pActor->s();
        x = pSprite->x;
        y = pSprite->y;
        z = pSprite->sector()->floorz;
        angle = pSprite->ang;
    }

    pSprite->x = x;
    pSprite->y = y;
    pSprite->z = z;
    pSprite->cstat = 0x101;
    pSprite->shade = -12;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->picnum = 1;
    pSprite->pal = pSprite->sector()->ceilingpal;
    pSprite->clipdist = 60;
    pSprite->ang = angle;
    pSprite->xrepeat = 40;
    pSprite->yrepeat = 40;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->hitag = 0;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->extra = -1;

    //	GrabTimeSlot(3);

    if (nType)
    {
        pActor->nAction = 0;
    }
    else
    {
        pActor->nAction = 1;
    }

    pActor->nFrame = 0;
    pActor->nCount = 0;
    pActor->pTarget = nullptr;
    pActor->nHealth = 600;
	pActor->nPhase = Counters[kCountRoach]++;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, pActor, 0x1C0000);
    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x1C0000);

    nCreaturesTotal++;
}

void GoRoach(spritetype* pSprite)
{
    pSprite->xvel = bcos(pSprite->ang, -1) - bcos(pSprite->ang, -3);
    pSprite->yvel = bsin(pSprite->ang, -1) - bsin(pSprite->ang, -3);
}

void AIRoach::Draw(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;
    short nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, RoachSeq[nAction].a + SeqOffsets[kSeqRoach], pActor->nFrame, RoachSeq[nAction].b);
    return;
}

void AIRoach::RadialDamage(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;
 
    ev->nDamage = runlist_CheckRadialDamage(pActor);
    Damage(ev);
}

void AIRoach::Damage(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

	auto pSprite = &pActor->s();
    short nAction = pActor->nAction;

    if (ev->nDamage)
    {
        if (pActor->nHealth <= 0) {
            return;
        }

        pActor->nHealth -= dmgAdjust(ev->nDamage);
        if (pActor->nHealth <= 0)
        {
            pSprite->xvel = 0;
            pSprite->yvel = 0;
            pSprite->zvel = 0;
            pSprite->cstat &= 0xFEFE;

            pActor->nHealth = 0;

            if (nAction < 5)
            {
                DropMagic(pActor);
                pActor->nAction = 5;
                pActor->nFrame = 0;
            }

            nCreaturesKilled++; // NOTE: This was incrementing in original code. Bug?
        }
        else
        {
            auto pSprite2 = ev->pOtherActor;
            if (pSprite2)
            {
                if (pSprite2->s().statnum < 199) {
                    pActor->pTarget = pSprite2;
                }

                if (nAction == 0)
                {
                    pActor->nAction = 2;
                    GoRoach(pSprite);
                    pActor->nFrame = 0;
                }
                else
                {
                    if (!RandomSize(4))
                    {
                        pActor->nAction = 4;
                        pActor->nFrame = 0;
                    }
                }
            }
        }
    }
}

void AIRoach::Tick(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

	auto pSprite = &pActor->s();
    short nAction = pActor->nAction;
    bool bVal = false;

    Gravity(pActor);

    int nSeq = SeqOffsets[kSeqRoach] + RoachSeq[pActor->nAction].a;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);
    seq_MoveSequence(pActor, nSeq, pActor->nFrame);

    pActor->nFrame++;
    if (pActor->nFrame >= SeqSize[nSeq])
    {
        bVal = true;
        pActor->nFrame = 0;
    }

    int nFlag = FrameFlag[SeqBase[nSeq] + pActor->nFrame];
    auto pTarget = pActor->pTarget;

    if (nAction > 5) {
        return;
    }

    switch (nAction)
    {
    case 0:
    {
        if (pActor->nFrame == 1)
        {
            pActor->nCount--;
            if (pActor->nCount <= 0)
            {
                pActor->nCount = RandomSize(6);
            }
            else
            {
                pActor->nFrame = 0;
            }
        }

        if (((pActor->nPhase & 0xF) == (totalmoves & 0xF)) && pTarget == nullptr)
        {
            auto pTarget = FindPlayer(pActor, 50);
            if (pTarget)
            {
                pActor->nAction = 2;
                pActor->nFrame = 0;
                pActor->pTarget = pTarget;
                GoRoach(pSprite);
            }
        }

        return;
    }

    case 1:
    {
        // partly the same as case 0.
        if (((pActor->nPhase & 0xF) == (totalmoves & 0xF)) && pTarget == nullptr)
        {
            auto pTarget = FindPlayer(pActor, 100);
            if (pTarget)
            {
                pActor->nAction = 2;
                pActor->nFrame = 0;
                pActor->pTarget = pTarget;
                GoRoach(pSprite);
            }
        }

        return;
    }

    case 2:
    {
        if ((totalmoves & 0xF) == (pActor->nPhase & 0xF))
        {
            PlotCourseToSprite(pActor, pTarget);
            GoRoach(pSprite);
        }

        auto nMov = MoveCreatureWithCaution(pActor);

        if (nMov.type == kHitSprite)
        {
            if (nMov.actor == pTarget)
            {
                // repeated below
                pActor->nIndex = RandomSize(2) + 1;
                pActor->nAction = 3;

                pSprite->xvel = 0;
                pSprite->yvel = 0;
                pSprite->ang = GetMyAngle(pTarget->s().x - pSprite->x, pTarget->s().y - pSprite->y);

                pActor->nFrame = 0;
            }
            else
            {
                pSprite->ang = (pSprite->ang + 256) & kAngleMask;
                GoRoach(pSprite);
            }
        }
        else if (nMov.type == kHitWall)
        {
            pSprite->ang = (pSprite->ang + 256) & kAngleMask;
            GoRoach(pSprite);
        }
        else
        {
            if (pActor->nCount != 0)
            {
                pActor->nCount--;
            }
            else
            {
                // same as above
                pActor->nIndex = RandomSize(2) + 1;
                pActor->nAction = 3;

                pSprite->xvel = 0;
                pSprite->yvel = 0;
                pSprite->ang = GetMyAngle(pTarget->s().x - pSprite->x, pTarget->s().y - pSprite->y);

                pActor->nFrame = 0;
            }
        }

        if (pTarget && !(pTarget->s().cstat & 0x101))
        {
            pActor->nAction = 1;
            pActor->nFrame = 0;
            pActor->nCount = 100;
            pActor->pTarget = nullptr;
            pSprite->xvel = 0;
            pSprite->yvel = 0;
        }

        return;
    }

    case 3:
    {
        if (bVal)
        {
            pActor->nIndex--;
            if (pActor->nIndex <= 0)
            {
                pActor->nAction = 2;
                GoRoach(pSprite);
                pActor->nFrame = 0;
                pActor->nCount = RandomSize(7);
            }
        }
        else
        {
            if (nFlag & 0x80)
            {
                BuildBullet(pActor, 13, -1, pSprite->ang, pTarget, 1);
            }
        }

        return;
    }

    case 4:
    {
        if (bVal)
        {
            pActor->nAction = 2;
            pActor->nFrame = 0;
        }

        return;
    }

    case 5:
    {
        if (bVal)
        {
            pSprite->cstat = 0;
            pActor->nAction = 6;
            pActor->nFrame = 0;
        }

        return;
    }
    }
}

END_PS_NS
