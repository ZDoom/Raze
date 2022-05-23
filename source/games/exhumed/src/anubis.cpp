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
#include "exhumed.h"
#include "aistuff.h"
#include "engine.h"
#include "sequence.h"
#include "sound.h"
#include <assert.h>

BEGIN_PS_NS

static const actionSeq AnubisSeq[] = {
    { 0, 0 },
    { 8, 0 },
    { 16, 0 },
    { 24, 0 },
    { 32, 0 },
    { -1, 0 },
    { 46, 1 },
    { 46, 1 },
    { 47, 1 },
    { 49, 1 },
    { 49, 1 },
    { 40, 1 },
    { 42, 1 },
    { 41, 1 },
    { 43, 1 },
};

void BuildAnubis(DExhumedActor* ap, int x, int y, int z, sectortype* pSector, int nAngle, uint8_t bIsDrummer)
{
    if (ap == nullptr)
    {
        ap = insertActor(pSector, 101);
    }
    else
    {
        ChangeActorStat(ap, 101);

        x = ap->spr.pos.X;
        y = ap->spr.pos.Y;
        z = ap->sector()->floorz;
        nAngle = ap->spr.ang;
    }

    ap->spr.pos.X = x;
    ap->spr.pos.Y = y;
    ap->spr.pos.Z = z;
    ap->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    ap->spr.xoffset = 0;
    ap->spr.shade = -12;
    ap->spr.yoffset = 0;
    ap->spr.picnum = 1;
    ap->spr.pal = ap->sector()->ceilingpal;
    ap->spr.clipdist = 60;
    ap->spr.ang = nAngle;
    ap->spr.xrepeat = 40;
    ap->spr.yrepeat = 40;
    ap->spr.xvel = 0;
    ap->spr.yvel = 0;
    ap->spr.zvel = 0;
    ap->spr.hitag = 0;
    ap->spr.lotag = runlist_HeadRun() + 1;
    ap->spr.extra = -1;

//	GrabTimeSlot(3);

    if (bIsDrummer)
    {
        auto& nAnubisDrum = Counters[kCountAnubisDrum];
        ap->nAction = nAnubisDrum + 6;
        nAnubisDrum++;

        if (nAnubisDrum >= 5) {
            nAnubisDrum = 0;
        }
    }
    else
    {
        ap->nAction = 0;
    }

    ap->nPhase = Counters[kCountAnubis]++;
    ap->nHealth = 540;
    ap->nFrame  = 0;
    ap->pTarget = nullptr;
    ap->nCount = 0;

    ap->spr.intowner = runlist_AddRunRec(ap->spr.lotag - 1, ap, 0x90000);

    runlist_AddRunRec(NewRun, ap, 0x90000);
    nCreaturesTotal++;
}

void AIAnubis::Tick(RunListEvent* ev)
{
    auto ap = ev->pObjActor;
    int nAction = ap->nAction;

    bool bVal = false;

    if (nAction < 11) {
        Gravity(ap);
    }

    int nSeq = SeqOffsets[kSeqAnubis] + AnubisSeq[nAction].a;

    seq_MoveSequence(ap, nSeq, ap->nFrame);

    ap->spr.picnum = seq_GetSeqPicnum2(nSeq, ap->nFrame);

    ap->nFrame++;
    if (ap->nFrame >= SeqSize[nSeq])
    {
        ap->nFrame = 0;
        bVal = true;
    }

    DExhumedActor* pTarget = ap->pTarget;

    int nFrame = SeqBase[nSeq] + ap->nFrame;
    int nFlag = FrameFlag[nFrame];

    Collision move;
    move.setNone();

    if (nAction > 0 && nAction < 11) {
        move = MoveCreatureWithCaution(ap);
    }

    switch (nAction)
    {
    case 0:
    {
        if ((ap->nPhase & 0x1F) == (totalmoves & 0x1F))
        {
            if (pTarget == nullptr) {
                pTarget = FindPlayer(ap, 100);
            }

            if (pTarget)
            {
                D3PlayFX(StaticSound[kSound8], ap);
                ap->nAction = 1;
                ap->nFrame = 0;
                ap->pTarget = pTarget;

                ap->spr.xvel = bcos(ap->spr.ang, -2);
                ap->spr.yvel = bsin(ap->spr.ang, -2);
            }
        }
        return;
    }
    case 1:
    {
        if ((ap->nPhase & 0x1F) == (totalmoves & 0x1F) && pTarget)
        {
            PlotCourseToSprite(ap, pTarget);

            int nAngle = ap->spr.ang & 0xFFF8;
            ap->spr.xvel = bcos(nAngle, -2);
            ap->spr.yvel = bsin(nAngle, -2);
        }

        switch (move.type)
        {
        case kHitSprite:
        {
            if (move.actor() == pTarget)
            {
                int nAng = getangle(pTarget->spr.pos.X - ap->spr.pos.X, pTarget->spr.pos.Y - ap->spr.pos.Y);
                int nAngDiff = AngleDiff(ap->spr.ang, nAng);

                if (nAngDiff < 64)
                {
                    ap->nAction = 2;
                    ap->nFrame = 0;
                }
                break;
            }
            // else we fall through to 0x8000
            [[fallthrough]];
        }
        case kHitWall:
        {
            ap->spr.ang = (ap->spr.ang + 256) & kAngleMask;
            ap->spr.xvel = bcos(ap->spr.ang, -2);
            ap->spr.yvel = bsin(ap->spr.ang, -2);
            break;
        }

        default:
        {
            if (ap->nCount)
            {
                ap->nCount--;
            }
            else
            {
                ap->nCount = 60;

                if (pTarget != nullptr) // NOTE: nTarget can be -1. this check wasn't in original code. TODO: demo compatiblity?
                {
                    if (cansee(ap->spr.pos.X, ap->spr.pos.Y, ap->spr.pos.Z - GetActorHeight(ap), ap->sector(),
                        pTarget->spr.pos.X, pTarget->spr.pos.Y, pTarget->spr.pos.Z - GetActorHeight(pTarget), pTarget->sector()))
                    {
                        ap->spr.xvel = 0;
                        ap->spr.yvel = 0;
                        ap->spr.ang = GetMyAngle(pTarget->spr.pos.X - ap->spr.pos.X, pTarget->spr.pos.Y - ap->spr.pos.Y);

                        ap->nAction = 3;
                        ap->nFrame = 0;
                    }
                }
            }
            break;
        }
        }
        break;
    }
    case 2:
    {
        if (pTarget == nullptr)
        {
            ap->nAction = 0;
            ap->nCount = 50;
        }
        else
        {
            if (PlotCourseToSprite(ap, pTarget) >= 768)
            {
                ap->nAction = 1;
            }
            else
            {
                if (nFlag & 0x80)
                {
                    runlist_DamageEnemy(pTarget, ap, 7);
                }
            }
        }

        break;
    }
    case 3:
    {
        if (bVal)
        {
            ap->nAction = 1;

            ap->spr.xvel = bcos(ap->spr.ang, -2);
            ap->spr.yvel = bsin(ap->spr.ang, -2);
            ap->nFrame = 0;
        }
        else
        {
            // loc_25718:
            if (nFlag & 0x80)
            {
                BuildBullet(ap, 8, -1, ap->spr.ang, pTarget, 1);
            }
        }

        return;
    }
    case 4:
    case 5:
    {
        ap->spr.xvel = 0;
        ap->spr.yvel = 0;

        if (bVal)
        {
            ap->nAction = 1;
            ap->nFrame = 0;
        }
        return;
    }
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    {
        if (bVal)
        {
            ap->nAction = (RandomSize(3) % 5) + 6;
            ap->nFrame = 0;
        }
        return;
    }
    case 11:
    case 12:
    {
        if (bVal)
        {
            ap->nAction = nAction + 2;
            ap->nFrame = 0;

            ap->spr.xvel = 0;
            ap->spr.yvel = 0;
        }
        return;
    }
    case 13:
    case 14:
    {
        ap->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
        return;
    }

    default:
        return;
    }

    // loc_2564C:
    if (nAction && pTarget != nullptr)
    {
        if (!(pTarget->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
        {
            ap->nAction = 0;
            ap->nFrame = 0;
            ap->nCount = 100;
            ap->pTarget = nullptr;

            ap->spr.xvel = 0;
            ap->spr.yvel = 0;
        }
    }
}

void AIAnubis::Draw(RunListEvent* ev)
{
    auto ap = ev->pObjActor;
    if (!ap) return;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqAnubis] + AnubisSeq[ap->nAction].a, ap->nFrame, AnubisSeq[ap->nAction].b);
}

void AIAnubis::RadialDamage(RunListEvent* ev)
{
    auto ap = ev->pObjActor;
    if (!ap) return;
    if (ap->nAction < 11) 
	{
    	ev->nDamage = runlist_CheckRadialDamage(ap);
	    Damage(ev);
	}
}

void AIAnubis::Damage(RunListEvent* ev)
{
    auto ap = ev->pObjActor;
    if (!ap) return;
    int nAction = ap->nAction;
    int nDamage = ev->nDamage;

    if (nDamage)
    {
        if (ap->nHealth <= 0)
            return;

        ap->nHealth -= dmgAdjust(nDamage);

        if (ap->nHealth > 0)
        {
            // loc_258D6:
            if (ev->pOtherActor == nullptr) {
                return;
            }
            auto statnum = ev->pOtherActor->spr.statnum;

            if (statnum == 100 || statnum < 199)
            {
                if (!RandomSize(5)) {
                    ap->pTarget = ev->pOtherActor;
                }
            }

            if (RandomSize(1))
            {
                if (nAction >= 6 && nAction <= 10)
                {
                    auto pDrumActor = insertActor(ap->sector(), kStatAnubisDrum);

                    pDrumActor->spr.pos.X = ap->spr.pos.X;
                    pDrumActor->spr.pos.Y = ap->spr.pos.Y;
                    pDrumActor->spr.pos.Z = pDrumActor->sector()->floorz;
                    pDrumActor->spr.xrepeat = 40;
                    pDrumActor->spr.yrepeat = 40;
                    pDrumActor->spr.shade = -64;

                    BuildObject(pDrumActor, 2, 0);
                }

                ap->nAction = 4;
                ap->nFrame = 0;
            }
            else
            {
                // loc_259B5:
                D3PlayFX(StaticSound[kSound39], ap);
            }
        }
        else
        {
            // he ded.
            ap->spr.xvel = 0;
            ap->spr.yvel = 0;
            ap->spr.zvel = 0;
            ap->spr.pos.Z = ap->sector()->floorz;
            ap->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;

            ap->nHealth = 0;

            nCreaturesKilled++;

            if (nAction < 11)
            {
                DropMagic(ap);
                ap->nAction = int(ev->isRadialEvent()) + 11;
                ap->nFrame = 0;
            }
        }
    }
}


END_PS_NS
