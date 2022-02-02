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

void BuildRex(DExhumedActor* pActor, int x, int y, int z, sectortype* pSector, int nAngle, int nChannel)
{
    if (pActor == nullptr)
    {
        pActor = insertActor(pSector, 119);
    }
    else
    {
        x = pActor->int_pos().X;
        y = pActor->int_pos().Y;
        z = pActor->sector()->int_floorz();
        nAngle = pActor->spr.ang;

        ChangeActorStat(pActor, 119);
    }

    pActor->set_int_pos({ x, y, z });
    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pActor->spr.clipdist = 80;
    pActor->spr.shade = -12;
    pActor->spr.xrepeat = 64;
    pActor->spr.yrepeat = 64;
    pActor->spr.picnum = 1;
    pActor->spr.pal = pActor->sector()->ceilingpal;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.ang = nAngle;
    pActor->spr.xvel = 0;
    pActor->spr.yvel = 0;
    pActor->spr.zvel = 0;
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

    // this isn't stored anywhere.
    runlist_AddRunRec(NewRun, pActor, 0x180000);

    nCreaturesTotal++;
}

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
                pActor->spr.xvel = 0;
                pActor->spr.yvel = 0;
                pActor->spr.zvel = 0;
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

void AIRex::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    int nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqRex] + RexSeq[nAction].a, pActor->nFrame, RexSeq[nAction].b);
    return;
}

void AIRex::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    int nAction = pActor->nAction;

    bool bVal = false;

    Gravity(pActor);

    int nSeq = SeqOffsets[kSeqRex] + RexSeq[nAction].a;

    pActor->spr.picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

    int ecx = 2;

    if (nAction != 2) {
        ecx = 1;
    }

    // moves the mouth open and closed as it's idle?
    while (--ecx != -1)
    {
        seq_MoveSequence(pActor, nSeq, pActor->nFrame);

        pActor->nFrame++;
        if (pActor->nFrame >= SeqSize[nSeq])
        {
            pActor->nFrame = 0;
            bVal = true;
        }
    }

    int nFlag = FrameFlag[SeqBase[nSeq] + pActor->nFrame];

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
                    auto nAngle = pActor->spr.ang; // make backup of this variable
                    pActor->pTarget = FindPlayer(pActor, 60);
                    pActor->spr.ang = nAngle;
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

                pActor->spr.xvel = bcos(pActor->spr.ang, -2);
                pActor->spr.yvel = bsin(pActor->spr.ang, -2);

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
                pActor->spr.xvel = 0;
                pActor->spr.yvel = 0;
                return;
            }
            else
            {
                if (((PlotCourseToSprite(pActor, pTarget) >> 8) >= 60) || pActor->nCount > 0)
                {
                    int nAngle = pActor->spr.ang & 0xFFF8;
                    pActor->spr.xvel = bcos(nAngle, -2);
                    pActor->spr.yvel = bsin(nAngle, -2);
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
            pActor->spr.ang = (pActor->spr.ang + 256) & kAngleMask;
            pActor->spr.xvel = bcos(pActor->spr.ang, -2);
            pActor->spr.yvel = bsin(pActor->spr.ang, -2);
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

            pActor->spr.xvel = bcos(pActor->spr.ang, -1);
            pActor->spr.yvel = bsin(pActor->spr.ang, -1);

            auto nMov = MoveCreatureWithCaution(pActor);

            switch (nMov.type)
            {
            case kHitWall:
            {
                SetQuake(pActor, 25);
                pActor->nCount = 60;

                pActor->spr.ang = (pActor->spr.ang + 256) & kAngleMask;
                pActor->spr.xvel = bcos(pActor->spr.ang, -2);
                pActor->spr.yvel = bsin(pActor->spr.ang, -2);
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
                    int nAngle = pActor->spr.ang;

                    runlist_DamageEnemy(nMov.actor(), pActor, 15);

                    int xVel = bcos(nAngle) * 15;
                    int yVel = bsin(nAngle) * 15;

                    if (pHitActor->spr.statnum == 100)
                    {
                        auto nPlayer = GetPlayerFromActor(nMov.actor());
                        PlayerList[nPlayer].nDamage.X += (xVel << 4);
                        PlayerList[nPlayer].nDamage.Y += (yVel << 4);
                        pHitActor->spr.zvel = -3584;
                    }
                    else
                    {
                        pHitActor->spr.xvel += (xVel >> 3);
                        pHitActor->spr.yvel += (yVel >> 3);
                        pHitActor->spr.zvel = -2880;
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
            if (PlotCourseToSprite(pActor, pTarget) < 768)
            {
                if (nFlag & 0x80)
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
            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;
        }
    }
    return;
}


END_PS_NS
