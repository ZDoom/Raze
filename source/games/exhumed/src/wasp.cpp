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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DExhumedActor* BuildWasp(DExhumedActor* pActor, const DVector3& pos, sectortype* pSector, DAngle nAngle, bool bEggWasp)
{
    if (pActor == nullptr)
    {
        pActor = insertActor(pSector, 107);

		pActor->spr.pos = pos;
    }
    else
    {
        nAngle = pActor->spr.Angles.Yaw;
        ChangeActorStat(pActor, 107);
    }

    pActor->spr.shade = -12;
    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pActor->spr.pal = pActor->sector()->ceilingpal;
	pActor->clipdist = 17.5;

    if (bEggWasp)
    {
        pActor->spr.scale = DVector2(0.3125, 0.3125);
    }
    else
    {
        pActor->spr.scale = DVector2(0.78125, 0.78125);
    }

    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    setvalidpic(pActor);
    pActor->spr.Angles.Yaw = nAngle;
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
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

    pActor->nSeqFile = "wasp";

    Level.addKillCount();
    return pActor;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIWasp::Draw(RunListEvent* ev)
{
    if (const auto pActor = ev->pObjActor)
    {
        const auto waspSeq = &WaspSeq[pActor->nAction];
        seq_PlotSequence(ev->nParam, pActor->nSeqFile, waspSeq->nSeqId, pActor->nFrame, waspSeq->nFlags);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIWasp::RadialDamage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    if (!(pActor->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
        return;

    ev->nDamage = runlist_CheckRadialDamage(pActor);
    Damage(ev);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
            pActor->nAction = !RandomSize(4) ? 3 : 1;
			pActor->spr.Angles.Yaw += DAngle45 + DAngle90 + RandomAngle9();
            pActor->norm_ang();

            pActor->nVel = 3000;

            pActor->vel.Z = -1.25 - RandomSize(6) / 256.;
        }
        else
        {
            // Wasp is dead
            pActor->nAction = 4;
            pActor->nFrame = 0;

            pActor->spr.cstat = 0;
            pActor->spr.Angles.Yaw += DAngle180;

            pActor->VelFromAngle();

            pActor->vel.Z = 2;

            Level.addKill(-1);
        }
        pActor->nFrame = 0;
    }
    return;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIWasp::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    int nAction = pActor->nAction;

    DExhumedActor* pTarget = nullptr;

    bool bVal = false;

    const auto waspSeq = getSequence(pActor->nSeqFile, WaspSeq[nAction].nSeqId);
    const auto& seqFrame = waspSeq->frames[pActor->nFrame];

    pActor->spr.setspritetexture(seqFrame.getFirstChunkTexture());

    seqFrame.playSound(pActor);

    pActor->nFrame++;
    if (pActor->nFrame >= waspSeq->frames.Size())
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
            pActor->nFrame = 0;
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
        pActor->vel.Z = BobVal(pActor->nAngle) * 4;

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
                pActor->pitch = nullAngle;
                pActor->vel.Z = 0;
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
            pActor->nFrame = 0;
            pActor->nCount = RandomSize(6);
            return;
        }

        auto nChaseVal = AngleChase(pActor, pTarget, pActor->nVel, 0, DAngle22_5 / 8);

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
                pActor->vel.X = 0;
                pActor->vel.Y = 0;
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
			pActor->spr.Angles.Yaw += DAngle45 + DAngle90 + RandomAngle9();
            pActor->vel.Z = ((-20) - RandomSize(6)) / 256.;

            pActor->nAction = 1;
            pActor->nFrame = 0;
            pActor->nVel = 3000;
        }
        return;
    }
    case 4:
    {
        auto nMove = MoveCreature(pActor);

        //if (nMove.type != kHitNone) // The code messed up the return value so this check always was true.
        {
            pActor->vel.X = 0;
            pActor->vel.Y = 0;
            pActor->vel.Z = 4;
            pActor->nAction = 5;
            pActor->nFrame = 0;
        }

        return;
    }
    case 5:
    {
        auto pSector =pActor->sector();

        pActor->spr.pos.Z += pActor->vel.Z;

        if (pActor->spr.pos.Z >= pSector->floorz)
        {
            if (pSector->pBelow != nullptr)
            {
                BuildSplash(pActor, pSector);
                pActor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
            }

            pActor->vel.X = 0;
            pActor->vel.Y = 0;
            pActor->vel.Z = 0;
            pActor->nAction = 6;
            pActor->nFrame = 0;
            runlist_SubRunRec(pActor->nRun);
        }

        return;
    }
    }
}

END_PS_NS
