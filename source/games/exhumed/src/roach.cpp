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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void BuildRoach(int nType, DExhumedActor* pActor, const DVector3& pos, sectortype* pSector, DAngle angle)
{
	if (pActor == nullptr)
	{
		pActor = insertActor(pSector, 105);
		pActor->spr.pos = pos;
	}
	else
	{
		ChangeActorStat(pActor, 105);
		pActor->spr.pos.Z = pActor->sector()->floorz;
		angle = pActor->spr.Angles.Yaw;
	}

    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pActor->spr.shade = -12;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    setvalidpic(pActor);
    pActor->spr.pal = pActor->sector()->ceilingpal;
	pActor->clipdist = 15;
    pActor->spr.Angles.Yaw = angle;
    pActor->spr.scale = DVector2(0.625, 0.625);
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
    pActor->spr.hitag = 0;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.extra = -1;

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

    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x1C0000);
    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x1C0000);

    pActor->nSeqFile = "roach";

    Level.addKillCount();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GoRoach(DExhumedActor* pActor)
{
	pActor->vel.SetXY(pActor->spr.Angles.Yaw.ToVector() * (512 - 128));
}

void AIRoach::Draw(RunListEvent* ev)
{
	if (const auto pActor = ev->pObjActor)
    {
        const auto roachSeq = &RoachSeq[pActor->nAction];
        seq_PlotSequence(ev->nParam, pActor->nSeqFile, roachSeq->nSeqId, pActor->nFrame, roachSeq->nFlags);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIRoach::RadialDamage(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

    ev->nDamage = runlist_CheckRadialDamage(pActor);
    Damage(ev);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIRoach::Damage(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

    int nAction = pActor->nAction;

    if (ev->nDamage)
    {
        if (pActor->nHealth <= 0) {
            return;
        }

        pActor->nHealth -= dmgAdjust(ev->nDamage);
        if (pActor->nHealth <= 0)
        {
            pActor->vel.X = 0;
            pActor->vel.Y = 0;
            pActor->vel.Z = 0;
            pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;

            pActor->nHealth = 0;

            if (nAction < 5)
            {
                DropMagic(pActor);
                pActor->nAction = 5;
                pActor->nFrame = 0;
            }

            Level.addKill(-1); // NOTE: This was incrementing in original code. Bug?
        }
        else
        {
            auto pSprite2 = ev->pOtherActor;
            if (pSprite2)
            {
                if (pSprite2->spr.statnum < 199) {
                    pActor->pTarget = pSprite2;
                }

                if (nAction == 0)
                {
                    pActor->nAction = 2;
                    GoRoach(pActor);
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIRoach::Tick(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

    int nAction = pActor->nAction;
    bool bVal = false;

    Gravity(pActor);

    const auto roachSeq = getSequence(pActor->nSeqFile, RoachSeq[nAction].nSeqId);
    const auto& seqFrame = roachSeq->frames[pActor->nFrame];

    pActor->spr.setspritetexture(seqFrame.getFirstChunkTexture());
    seqFrame.playSound(pActor);

    pActor->nFrame++;
    if (pActor->nFrame >= roachSeq->frames.Size())
    {
        bVal = true;
        pActor->nFrame = 0;
    }

    DExhumedActor* pTarget = pActor->pTarget;

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
            pTarget = FindPlayer(pActor, 50);
            if (pTarget)
            {
                pActor->nAction = 2;
                pActor->nFrame = 0;
                pActor->pTarget = pTarget;
                GoRoach(pActor);
            }
        }

        return;
    }

    case 1:
    {
        // partly the same as case 0.
        if (((pActor->nPhase & 0xF) == (totalmoves & 0xF)) && pTarget == nullptr)
        {
            pTarget = FindPlayer(pActor, 100);
            if (pTarget)
            {
                pActor->nAction = 2;
                pActor->nFrame = 0;
                pActor->pTarget = pTarget;
                GoRoach(pActor);
            }
        }

        return;
    }

    case 2:
    {
        if ((totalmoves & 0xF) == (pActor->nPhase & 0xF))
        {
            PlotCourseToSprite(pActor, pTarget);
            GoRoach(pActor);
        }

        auto nMov = MoveCreatureWithCaution(pActor);

        if (nMov.type == kHitSprite)
        {
            if (nMov.actor() == pTarget)
            {
                // repeated below
                pActor->nIndex = RandomSize(2) + 1;
                pActor->nAction = 3;

                pActor->vel.X = 0;
                pActor->vel.Y = 0;
                pActor->spr.Angles.Yaw = (pTarget->spr.pos - pActor->spr.pos).Angle();

                pActor->nFrame = 0;
            }
            else
            {
                pActor->spr.Angles.Yaw += DAngle45;
                GoRoach(pActor);
            }
        }
        else if (nMov.type == kHitWall)
        {
            pActor->spr.Angles.Yaw += DAngle45;
            GoRoach(pActor);
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

                pActor->vel.X = 0;
                pActor->vel.Y = 0;
				pActor->spr.Angles.Yaw = (pTarget->spr.pos - pActor->spr.pos).Angle();

                pActor->nFrame = 0;
            }
        }

        if (pTarget && !(pTarget->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
        {
            pActor->nAction = 1;
            pActor->nFrame = 0;
            pActor->nCount = 100;
            pActor->pTarget = nullptr;
            pActor->vel.X = 0;
            pActor->vel.Y = 0;
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
                GoRoach(pActor);
                pActor->nFrame = 0;
                pActor->nCount = RandomSize(7);
            }
        }
        else
        {
            if (seqFrame.flags & 0x80)
            {
                BuildBullet(pActor, 13, INT_MAX, pActor->spr.Angles.Yaw, pTarget, 1);
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
            pActor->spr.cstat = 0;
            pActor->nAction = 6;
            pActor->nFrame = 0;
        }

        return;
    }
    }
}

END_PS_NS
