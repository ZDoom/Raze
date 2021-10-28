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

void BuildRex(DExhumedActor* pActor, int x, int y, int z, short nSector, short nAngle, int nChannel)
{
    spritetype* pSprite;
    if (pActor == nullptr)
    {
        pActor = insertActor(nSector, 119);
        pSprite = &pActor->s();
    }
    else
    {
        pSprite = &pActor->s();
        x = pSprite->x;
        y = pSprite->y;
        z = sector[pSprite->sectnum].floorz;
        nAngle = pSprite->ang;

        ChangeActorStat(pActor, 119);
    }

    pSprite->x = x;
    pSprite->y = y;
    pSprite->z = z;
    pSprite->cstat = 0x101;
    pSprite->clipdist = 80;
    pSprite->shade = -12;
    pSprite->xrepeat = 64;
    pSprite->yrepeat = 64;
    pSprite->picnum = 1;
    pSprite->pal = sector[pSprite->sectnum].ceilingpal;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->ang = nAngle;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->extra = -1;
    pSprite->hitag = 0;

    GrabTimeSlot(3);

    pActor->nAction = 0;
    pActor->nHealth = 4000;
    pActor->nFrame = 0;
    pActor->pTarget = nullptr;
    pActor->nCount = 0;
    pActor->nPhase = Counters[kCountRex]++;

    pActor->nRun = nChannel;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, pActor, 0x180000);

    // this isn't stored anywhere.
    runlist_AddRunRec(NewRun, pActor, 0x180000);

    nCreaturesTotal++;
}

void AIRex::RadialDamage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    short nAction = pActor->nAction;

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

    short nAction = pActor->nAction;
    auto pSprite = &pActor->s();

    if (ev->nDamage)
    {
        auto pTarget = ev->pOtherActor;
        if (pTarget && pTarget->s().statnum == 100)
        {
            pActor->pTarget = pTarget;
        }

        if (pActor->nAction == 5 && pActor->nHealth > 0)
        {
            pActor->nHealth -= dmgAdjust(ev->nDamage);

            if (pActor->nHealth <= 0)
            {
                pSprite->xvel = 0;
                pSprite->yvel = 0;
                pSprite->zvel = 0;
                pSprite->cstat &= 0xFEFE;

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

    short nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqRex] + RexSeq[nAction].a, pActor->nFrame, RexSeq[nAction].b);
    return;
}

void AIRex::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    short nAction = pActor->nAction;
    auto pSprite = &pActor->s();

    bool bVal = false;

    Gravity(pActor);

    int nSeq = SeqOffsets[kSeqRex] + RexSeq[nAction].a;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

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

    auto pTarget = pActor->pTarget;

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
                    short nAngle = pSprite->ang; // make backup of this variable
                    pActor->pTarget = FindPlayer(pActor, 60);
                    pSprite->ang = nAngle;
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

                pSprite->xvel = bcos(pSprite->ang, -2);
                pSprite->yvel = bsin(pSprite->ang, -2);

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
                pSprite->xvel = 0;
                pSprite->yvel = 0;
                return;
            }
            else
            {
                if (((PlotCourseToSprite(pActor, pTarget) >> 8) >= 60) || pActor->nCount > 0)
                {
                    int nAngle = pSprite->ang & 0xFFF8;
                    pSprite->xvel = bcos(nAngle, -2);
                    pSprite->yvel = bsin(nAngle, -2);
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
            if (nMov.actor == pTarget)
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
            pSprite->ang = (pSprite->ang + 256) & kAngleMask;
            pSprite->xvel = bcos(pSprite->ang, -2);
            pSprite->yvel = bsin(pSprite->ang, -2);
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

            pSprite->xvel = bcos(pSprite->ang, -1);
            pSprite->yvel = bsin(pSprite->ang, -1);

            auto nMov = MoveCreatureWithCaution(pActor);

            switch (nMov.type)
            {
            case kHitWall:
            {
                SetQuake(pActor, 25);
                pActor->nCount = 60;

                pSprite->ang = (pSprite->ang + 256) & kAngleMask;
                pSprite->xvel = bcos(pSprite->ang, -2);
                pSprite->yvel = bsin(pSprite->ang, -2);
                pActor->nAction = 1;
                pActor->nFrame = 0;
                nAction = 1;
                break;
            }
            case kHitSprite:
            {
                pActor->nAction = 3;
                pActor->nFrame = 0;

                auto pSprite2 = &nMov.actor->s();

                if (pSprite2->statnum && pSprite2->statnum < 107)
                {
                    short nAngle = pSprite->ang;

                    runlist_DamageEnemy(nMov.actor, pActor, 15);

                    int xVel = bcos(nAngle) * 15;
                    int yVel = bsin(nAngle) * 15;

                    if (pSprite2->statnum == 100)
                    {
                        auto nPlayer = GetPlayerFromActor(nMov.actor);
                        PlayerList[nPlayer].nXDamage += (xVel << 4);
                        PlayerList[nPlayer].nYDamage += (yVel << 4);
                        pSprite2->zvel = -3584;
                    }
                    else
                    {
                        pSprite2->xvel += (xVel >> 3);
                        pSprite2->yvel += (yVel >> 3);
                        pSprite2->zvel = -2880;
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
        pSprite->cstat &= 0xFEFE;
        return;
    }
    }

    // break-ed
    if (nAction > 0)
    {
        if ((pTarget != nullptr) && (!(pTarget->s().cstat & 0x101)))
        {
            pActor->nAction = 0;
            pActor->nFrame = 0;
            pActor->nCount = 0;
            pActor->pTarget = nullptr;
            pSprite->xvel = 0;
            pSprite->yvel = 0;
        }
    }
    return;
}


END_PS_NS
