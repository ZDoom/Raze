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

void BuildAnubis(DExhumedActor* ap, int x, int y, int z, int nSector, int nAngle, uint8_t bIsDrummer)
{
    spritetype* sp;
    if (ap == nullptr)
    {
        ap = insertActor(nSector, 101);
        sp = &ap->s();
    }
    else
    {
        ChangeActorStat(ap, 101);
        sp = &ap->s();

        x = sp->x;
        y = sp->y;
        z = sector[sp->sectnum].floorz;
        nAngle = sp->ang;
    }

    sp->x = x;
    sp->y = y;
    sp->z = z;
    sp->cstat = 0x101;
    sp->xoffset = 0;
    sp->shade = -12;
    sp->yoffset = 0;
    sp->picnum = 1;
    sp->pal = sector[sp->sectnum].ceilingpal;
    sp->clipdist = 60;
    sp->ang = nAngle;
    sp->xrepeat = 40;
    sp->yrepeat = 40;
    sp->xvel = 0;
    sp->yvel = 0;
    sp->zvel = 0;
    sp->hitag = 0;
    sp->lotag = runlist_HeadRun() + 1;
    sp->extra = -1;

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

    sp->owner = runlist_AddRunRec(sp->lotag - 1, ap, 0x90000);

    runlist_AddRunRec(NewRun, ap, 0x90000);
    nCreaturesTotal++;
}

void AIAnubis::Tick(RunListEvent* ev)
{
    auto ap = ev->pObjActor;
    auto sp = &ap->s();
    int nAction = ap->nAction;

    bool bVal = false;

    if (nAction < 11) {
        Gravity(ap);
    }

    short nSeq = SeqOffsets[kSeqAnubis] + AnubisSeq[nAction].a;

    seq_MoveSequence(ap, nSeq, ap->nFrame);

    sp->picnum = seq_GetSeqPicnum2(nSeq, ap->nFrame);

    ap->nFrame++;
    if (ap->nFrame >= SeqSize[nSeq])
    {
        ap->nFrame = 0;
        bVal = true;
    }

    auto pTarget = ap->pTarget;

    short nFrame = SeqBase[nSeq] + ap->nFrame;
    short nFlag = FrameFlag[nFrame];

    Collision move(0);

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

                sp->xvel = bcos(sp->ang, -2);
                sp->yvel = bsin(sp->ang, -2);
            }
        }
        return;
    }
    case 1:
    {
        if ((ap->nPhase & 0x1F) == (totalmoves & 0x1F) && pTarget)
        {
            PlotCourseToSprite(ap, pTarget);

            int nAngle = sp->ang & 0xFFF8;
            sp->xvel = bcos(nAngle, -2);
            sp->yvel = bsin(nAngle, -2);
        }

        switch (move.type)
        {
        case kHitSprite:
        {
            if (move.actor == pTarget)
            {
                int nAng = getangle(pTarget->s().x - sp->x, pTarget->s().y - sp->y);
                int nAngDiff = AngleDiff(sp->ang, nAng);

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
            sp->ang = (sp->ang + 256) & kAngleMask;
            sp->xvel = bcos(sp->ang, -2);
            sp->yvel = bsin(sp->ang, -2);
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
                    if (cansee(sp->x, sp->y, sp->z - GetActorHeight(ap), sp->sectnum,
                        pTarget->s().x, pTarget->s().y, pTarget->s().z - GetActorHeight(pTarget), pTarget->s().sectnum))
                    {
                        sp->xvel = 0;
                        sp->yvel = 0;
                        sp->ang = GetMyAngle(pTarget->s().x - sp->x, pTarget->s().y - sp->y);

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

            sp->xvel = bcos(sp->ang, -2);
            sp->yvel = bsin(sp->ang, -2);
            ap->nFrame = 0;
        }
        else
        {
            // loc_25718:
            if (nFlag & 0x80)
            {
                BuildBullet(ap, 8, -1, sp->ang, pTarget, 1);
            }
        }

        return;
    }
    case 4:
    case 5:
    {
        sp->xvel = 0;
        sp->yvel = 0;

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

            sp->xvel = 0;
            sp->yvel = 0;
        }
        return;
    }
    case 13:
    case 14:
    {
        sp->cstat &= 0xFEFE;
        return;
    }

    default:
        return;
    }

    // loc_2564C:
    if (nAction && pTarget != nullptr)
    {
        if (!(pTarget->s().cstat & 0x101))
        {
            ap->nAction = 0;
            ap->nFrame = 0;
            ap->nCount = 100;
            ap->pTarget = nullptr;

            sp->xvel = 0;
            sp->yvel = 0;
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
    auto sp = &ap->s();
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
            auto pTarget = &ev->pOtherActor->s();

            if (pTarget->statnum == 100 || pTarget->statnum < 199)
            {
                if (!RandomSize(5)) {
                    ap->pTarget = ev->pOtherActor;
                }
            }

            if (RandomSize(1))
            {
                if (nAction >= 6 && nAction <= 10)
                {
                    auto pDrumActor = insertActor(sp->sectnum, kStatAnubisDrum);
                    auto pDrumSprite = &pDrumActor->s();

                    pDrumSprite->x = sp->x;
                    pDrumSprite->y = sp->y;
                    pDrumSprite->z = sector[pDrumSprite->sectnum].floorz;
                    pDrumSprite->xrepeat = 40;
                    pDrumSprite->yrepeat = 40;
                    pDrumSprite->shade = -64;

                    BuildObject(pDrumActor, 2, 0);
                }

                ap->pTarget = ev->pOtherActor;
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
            sp->xvel = 0;
            sp->yvel = 0;
            sp->zvel = 0;
            sp->z = sector[sp->sectnum].floorz;
            sp->cstat &= 0xFEFE;

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
