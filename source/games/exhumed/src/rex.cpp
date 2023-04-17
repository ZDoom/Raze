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
#include "sound.h"
#include "player.h"
#include <assert.h>

BEGIN_PS_NS

static actionSeq RexSeq[] = {
    {29, 0},
    {0,  0},
    {0,  0},
    {37, 0},
    {9,  0},
    {18, 0},
    {27, 1},
    {28, 1}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void BuildRex(DExhumedActor* pActor, const DVector3& pos, sectortype* pSector, DAngle nAngle, int nChannel)
{
    if (pActor == nullptr)
    {
        pActor = insertActor(pSector, 119);
		pActor->spr.pos = pos;
	}
	else
	{
		nAngle = pActor->spr.Angles.Yaw;
		pActor->spr.pos.Z = pActor->sector()->floorz;
        ChangeActorStat(pActor, 119);
    }

    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pActor->clipdist = 20;
    pActor->spr.shade = -12;
    pActor->spr.scale = DVector2(1, 1);
    pActor->spr.picnum = 1;
    pActor->spr.pal = pActor->sector()->ceilingpal;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.Angles.Yaw = nAngle;
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.extra = -1;
    pActor->spr.hitag = 0;

    GrabTimeSlot(3);

    pActor->nAction = 0;
    pActor->nHealth = 4000;
    pActor->nFrame = 0;
    pActor->pTarget = nullptr;
    pActor->nCount = 0;
    pActor->nPhase = Counters[kCountRex]++;

    pActor->nRun = nChannel;

    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x180000);

    pActor->nSeqFile = "rex";

    // this isn't stored anywhere.
    runlist_AddRunRec(NewRun, pActor, 0x180000);

    nCreaturesTotal++;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIRex::RadialDamage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    int nAction = pActor->nAction;

    if (nAction == 5)
    {
        ev->nDamage = runlist_CheckRadialDamage(pActor);
    }
    Damage(ev);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIRex::Damage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    int nAction = pActor->nAction;

    if (ev->nDamage)
    {
        auto pTarget = ev->pOtherActor;
        if (pTarget && pTarget->spr.statnum == 100)
        {
            pActor->pTarget = pTarget;
        }

        if (pActor->nAction == 5 && pActor->nHealth > 0)
        {
            pActor->nHealth -= dmgAdjust(ev->nDamage);

            if (pActor->nHealth <= 0)
            {
                pActor->vel.X = 0;
                pActor->vel.Y = 0;
                pActor->vel.Z = 0;
                pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;

                pActor->nHealth = 0;

                nCreaturesKilled++;

                if (nAction < 6)
                {
                    pActor->nAction = 6;
                    pActor->nFrame = 0;
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

void AIRex::Draw(RunListEvent* ev)
{
    if (const auto pActor = ev->pObjActor)
    {
        const auto rexSeq = &RexSeq[pActor->nAction];
        seq_PlotSequence(ev->nParam, pActor->nSeqFile, rexSeq->nSeqId, pActor->nFrame, rexSeq->nFlags);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIRex::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    int nAction = pActor->nAction;

    bool bVal = false;

    Gravity(pActor);

    const auto& rexSeq = getSequence(pActor->nSeqFile, RexSeq[nAction].nSeqId);
    const auto& seqFrame = rexSeq.frames[pActor->nFrame];

    pActor->spr.setspritetexture(seqFrame.getFirstTexID());

    int ecx = 2;

    if (nAction != 2) {
        ecx = 1;
    }

    // moves the mouth open and closed as it's idle?
    while (--ecx != -1)
    {
        seqFrame.playSound(pActor);

        pActor->nFrame++;
        if (pActor->nFrame >= rexSeq.frames.Size())
        {
            pActor->nFrame = 0;
            bVal = true;
        }
    }

    DExhumedActor* pTarget = pActor->pTarget;

    switch (nAction)
    {
    default:
        return;

    case 0:
    {
        if (!pActor->nCount)
        {
            if ((pActor->nPhase & 0x1F) == (totalmoves & 0x1F))
            {
                if (pTarget == nullptr)
                {
                    auto nAngle = pActor->spr.Angles.Yaw;
                    pActor->pTarget = FindPlayer(pActor, 60);
                    pActor->spr.Angles.Yaw = nAngle;
                }
                else
                {
                    pActor->nCount = 60;
                }
            }
        }
        else
        {
            pActor->nCount--;
            if (pActor->nCount <= 0)
            {
                pActor->nAction = 1;
                pActor->nFrame = 0;

                pActor->VelFromAngle(-2);

                D3PlayFX(StaticSound[kSound48], pActor);

                pActor->nCount = 30;
            }
        }

        return;
    }

    case 1:
    {
        if (pActor->nCount > 0)
        {
            pActor->nCount--;
        }

        if ((pActor->nPhase & 0x0F) == (totalmoves & 0x0F))
        {
            if (!RandomSize(1))
            {
                pActor->nAction = 5;
                pActor->nFrame = 0;
                pActor->vel.X = 0;
                pActor->vel.Y = 0;
                return;
            }
            else
            {
                if ((PlotCourseToSprite(pActor, pTarget) >= 60*16) || pActor->nCount > 0)
                {
					pActor->vel.XY() = pActor->spr.Angles.Yaw.ToVector() * 256;
                }
                else
                {
                    pActor->nAction = 2;
                    pActor->nCount = 240;
                    D3PlayFX(StaticSound[kSound48], pActor);
                    pActor->nFrame = 0;
                    return;
                }
            }
        }

        auto nMov = MoveCreatureWithCaution(pActor);

        switch (nMov.type)
        {
        case kHitSprite:
        {
            if (nMov.actor() == pTarget)
            {
                PlotCourseToSprite(pActor, pTarget);
                pActor->nAction = 4;
                pActor->nFrame = 0;
                break;
            }
            [[fallthrough]];
        }
        case kHitWall:
        {
            pActor->spr.Angles.Yaw += DAngle45;
            pActor->VelFromAngle(-2);
            pActor->nAction = 1;
            pActor->nFrame = 0;
            nAction = 1;
            break;
        }
        }

        break;
    }

    case 2:
    {
        pActor->nCount--;
        if (pActor->nCount > 0)
        {
            PlotCourseToSprite(pActor, pTarget);

            pActor->VelFromAngle(-1);

            auto nMov = MoveCreatureWithCaution(pActor);

            switch (nMov.type)
            {
            case kHitWall:
            {
                SetQuake(pActor, 25);
                pActor->nCount = 60;

                pActor->spr.Angles.Yaw += DAngle45;
                pActor->VelFromAngle(-2);
                pActor->nAction = 1;
                pActor->nFrame = 0;
                nAction = 1;
                break;
            }
            case kHitSprite:
            {
                pActor->nAction = 3;
                pActor->nFrame = 0;

                auto pHitActor = nMov.actor();

                if (pHitActor->spr.statnum && pHitActor->spr.statnum < 107)
                {
                    runlist_DamageEnemy(nMov.actor(), pActor, 15);

					auto vel = pActor->spr.Angles.Yaw.ToVector() * 1024 * 15;

                    if (pHitActor->spr.statnum == 100)
                    {
                        auto nPlayer = GetPlayerFromActor(nMov.actor());
                        PlayerList[nPlayer].nThrust += vel / 4096;
                        pHitActor->vel.Z = -14;
                    }
                    else
                    {
						pHitActor->vel.XY() = vel / 8.;
                        pHitActor->vel.Z = 11.25;
                    }
                }

                pActor->nCount >>= 2;
                return;
            }
            }
        }
        else
        {
            pActor->nAction = 1;
            pActor->nFrame = 0;
            pActor->nCount = 90;
        }

        return;
    }

    case 3:
    {
        if (bVal)
        {
            pActor->nAction = 2;
        }
        return;
    }

    case 4:
    {
        if (pTarget != nullptr)
        {
            if (PlotCourseToSprite(pActor, pTarget) < 48)
            {
                if (seqFrame.flags & 0x80)
                {
                    runlist_DamageEnemy(pTarget, pActor, 15);
                }

                break;
            }
        }

        pActor->nAction = 1;
        break;
    }

    case 5:
    {
        if (bVal)
        {
            pActor->nAction = 1;
            pActor->nCount = 15;
        }
        return;
    }

    case 6:
    {
        if (bVal)
        {
            pActor->nAction = 7;
            pActor->nFrame = 0;
            runlist_ChangeChannel(pActor->nRun, 1);
        }
        return;
    }

    case 7:
    {
        pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
        return;
    }
    }

    // break-ed
    if (nAction > 0)
    {
        if ((pTarget != nullptr) && (!(pTarget->spr.cstat & CSTAT_SPRITE_BLOCK_ALL)))
        {
            pActor->nAction = 0;
            pActor->nFrame = 0;
            pActor->nCount = 0;
            pActor->pTarget = nullptr;
            pActor->vel.X = 0;
            pActor->vel.Y = 0;
        }
    }
    return;
}


END_PS_NS
