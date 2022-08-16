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
#include "exhumed.h"
#include "sequence.h"
#include <assert.h>

BEGIN_PS_NS

static actionSeq WaspSeq[] = {
    {0,  0},
    {0,  0},
    {9,  0},
    {18, 0},
    {27, 1},
    {28, 1},
    {29, 1}
};

void SetWaspVel(DExhumedActor* pActor)
{
    pActor->spr.xvel = bcos(pActor->int_ang());
    pActor->spr.yvel = bsin(pActor->int_ang());
}

DExhumedActor* BuildWasp(DExhumedActor* pActor, int x, int y, int z, sectortype* pSector, int nAngle, bool bEggWasp)
{
    if (pActor == nullptr)
    {
        pActor = insertActor(pSector, 107);

        pActor->set_int_pos({ x, y, z });
    }
    else
    {
        nAngle = pActor->int_ang();
        ChangeActorStat(pActor, 107);
    }

    pActor->spr.shade = -12;
    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pActor->spr.pal = pActor->sector()->ceilingpal;
    pActor->spr.clipdist = 70;

    if (bEggWasp)
    {
        pActor->spr.xrepeat = 20;
        pActor->spr.yrepeat = 20;
    }
    else
    {
        pActor->spr.xrepeat = 50;
        pActor->spr.yrepeat = 50;
    }

    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.picnum = 1;
    pActor->set_int_ang(nAngle);
    pActor->spr.xvel = 0;
    pActor->spr.yvel = 0;
    pActor->spr.zvel = 0;
    pActor->spr.hitag = 0;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.extra = -1;

    //	GrabTimeSlot(3);

    pActor->nAction = 0;
    pActor->nFrame = 0;
    pActor->pTarget = nullptr;
    pActor->nHealth = 800;
    pActor->nDamage = 10;
    pActor->nPhase = Counters[kCountWasp]++;

    if (bEggWasp)
    {
        pActor->nCount = 60;
        pActor->nDamage /= 2;
    }
    else
    {
        pActor->nCount = RandomSize(5);
    }

    pActor->nAngle = 0;
    pActor->nVel = 0;
    pActor->nAngle2 = RandomSize(7) + 127;

    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x1E0000);

    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x1E0000);

    nCreaturesTotal++;
    return pActor;
}

void AIWasp::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    int nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqWasp] + WaspSeq[nAction].a, pActor->nFrame, WaspSeq[nAction].b);
    return;
}

void AIWasp::RadialDamage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    if (!(pActor->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
        return;

    ev->nDamage = runlist_CheckRadialDamage(pActor);
    Damage(ev);
}

void AIWasp::Damage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    if (!ev->nDamage) {
        return;
    }

    if (pActor->nHealth > 0)
    {
        pActor->nHealth -= dmgAdjust(ev->nDamage, 3);

        if (pActor->nHealth > 0)
        {
            if (!RandomSize(4))
            {
                pActor->nAction = 3;
                pActor->nFrame = 0;
            }

            pActor->nAction = 1;
            pActor->add_int_ang(RandomSize(9) + 768);
            pActor->norm_ang();

            pActor->nVel = 3000;

            pActor->spr.zvel = (-20) - RandomSize(6);
        }
        else
        {
            // Wasp is dead
            pActor->nAction = 4;
            pActor->nFrame = 0;

            pActor->spr.cstat = 0;
            pActor->set_int_ang((pActor->int_ang() + 1024) & kAngleMask);

            SetWaspVel(pActor);

            pActor->spr.zvel = 512;

            nCreaturesKilled++;
        }
    }
    return;
}

void AIWasp::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    int nAction = pActor->nAction;

    DExhumedActor* pTarget = nullptr;

    bool bVal = false;

    int nSeq = SeqOffsets[kSeqWasp] + WaspSeq[nAction].a;

    pActor->spr.picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

    seq_MoveSequence(pActor, nSeq, pActor->nFrame);

    pActor->nFrame++;
    if (pActor->nFrame >= SeqSize[nSeq])
    {
        pActor->nFrame = 0;
        bVal = true;
    }

    if (pActor->nHealth > 0)
    {
        pTarget = pActor->pTarget;

        if (pTarget && (!(pTarget->spr.cstat & CSTAT_SPRITE_BLOCK_ALL) || (pTarget->sector()->Flag & kSectUnderwater)))
        {
            // goto pink
            pActor->pTarget = nullptr;
            pActor->nAction = 0;
            pActor->nCount = RandomSize(6);
            return;
        }
    }

    switch (nAction)
    {
    default:
        return;

    case 0:
    {
        pActor->spr.zvel = bsin(pActor->nAngle, -4);

        pActor->nAngle += pActor->nAngle2;
        pActor->nAngle &= kAngleMask;

        MoveCreature(pActor);

        if (pTarget)
        {
            pActor->nCount--;
            if (pActor->nCount > 0)
            {
                PlotCourseToSprite(pActor, pTarget);
            }
            else
            {
                pActor->spr.zvel = 0;
                pActor->nAction = 1;
                pActor->nFrame = 0;
                pActor->nVel = 1500;
                pActor->nCount = RandomSize(5) + 60;
            }
        }
        else
        {
            if ((pActor->nPhase & 0x1F) == (totalmoves & 0x1F)) {
                pActor->pTarget = FindPlayer(pActor, 60);
            }
        }

        return;
    }

    case 1:
    {
        pActor->nCount--;

        if (pActor->nCount <= 0)
        {
            pActor->nAction = 0;
            pActor->nCount = RandomSize(6);
            return;
        }

        auto nChaseVal = AngleChase(pActor, pTarget, pActor->nVel, 0, 16);

        switch (nChaseVal.type)
        {
        default:
            return;

        case kHitWall:
        {
            return;
        }

        case kHitSprite:
        {
            if (nChaseVal.actor() == pTarget)
            {
                pActor->spr.xvel = 0;
                pActor->spr.yvel = 0;
                runlist_DamageEnemy(pTarget, pActor, pActor->nDamage);
                pActor->nAction = 2;
                pActor->nFrame = 0;
            }
            return;
        }
        }

        return;
    }

    case 2:
    case 3:
    {
        if (bVal)
        {
            pActor->add_int_ang(RandomSize(9) + 768);
            pActor->norm_ang();
            pActor->spr.zvel = (-20) - RandomSize(6);

            pActor->nAction = 1;
            pActor->nVel = 3000;
        }
        return;
    }
    case 4:
    {
        auto nMove = MoveCreature(pActor);

        //if (nMove.type != kHitNone) // The code messed up the return value so this check always was true.
        {
            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;
            pActor->spr.zvel = 1024;
            pActor->nAction = 5;
            pActor->nFrame = 0;
        }

        return;
    }
    case 5:
    {
        auto pSector =pActor->sector();

        pActor->add_int_z(pActor->spr.zvel);

        if (pActor->int_pos().Z >= pSector->int_floorz())
        {
            if (pSector->pBelow != nullptr)
            {
                BuildSplash(pActor, pSector);
                pActor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
            }

            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;
            pActor->spr.zvel = 0;
            pActor->nAction = 6;
            pActor->nFrame = 0;
            runlist_SubRunRec(pActor->nRun);
        }

        return;
    }
    }
}

END_PS_NS
