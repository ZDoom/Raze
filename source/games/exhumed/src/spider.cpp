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
#include <assert.h>

BEGIN_PS_NS

static actionSeq SpiderSeq[] = {
    {16, 0},
    {8,  0},
    {32, 0},
    {24, 0},
    {0,  0},
    {40, 1},
    {41, 1}
};


DExhumedActor* BuildSpider(DExhumedActor* spp, int x, int y, int z, short nSector, int nAngle)
{
    spritetype* sp;
    if (spp == nullptr)
    {
        spp = insertActor(nSector, 99);
        sp = &spp->s();
    }
    else
    {
        ChangeActorStat(spp, 99);
        sp = &spp->s();

        x = sp->x;
        y = sp->y;
        z = sector[sp->sectnum].floorz;
        nAngle = sp->ang;
    }

    sp->x = x;
    sp->y = y;
    sp->z = z;
    sp->cstat = 0x101;
    sp->shade = -12;
    sp->clipdist = 15;
    sp->xvel = 0;
    sp->yvel = 0;
    sp->zvel = 0;
    sp->xrepeat = 40;
    sp->yrepeat = 40;
    sp->pal = sector[sp->sectnum].ceilingpal;
    sp->xoffset = 0;
    sp->yoffset = 0;
    sp->ang = nAngle;
    sp->picnum = 1;
    sp->hitag = 0;
    sp->lotag = runlist_HeadRun() + 1;
    sp->extra = -1;

    //	GrabTimeSlot(3);

    spp->nAction = 0;
    spp->nFrame = 0;
    spp->pTarget = nullptr;
    spp->nHealth = 160;
    spp->nPhase = Counters[kCountSpider]++;

    sp->owner = runlist_AddRunRec(sp->lotag - 1, spp, 0xC0000);

    spp->nRun = runlist_AddRunRec(NewRun, spp, 0xC0000);

    nCreaturesTotal++;

    return spp;
}

void AISpider::Tick(RunListEvent* ev)
{
    auto spp = ev->pObjActor;
    if (!spp) return;

    auto sp = &spp->s();
    short nAction = spp->nAction;

    int nVel = 6;

    if (spp->nHealth)
    {
        if (sp->cstat & 8)
        {
            sp->z = sector[sp->sectnum].ceilingz + GetActorHeight(spp);
        }
        else
        {
            Gravity(spp);
        }
    }

    int nSeq = SeqOffsets[kSeqSpider] + SpiderSeq[nAction].a;

    sp->picnum = seq_GetSeqPicnum2(nSeq, spp->nFrame);

    seq_MoveSequence(spp, nSeq, spp->nFrame);

    int nFrameFlag = FrameFlag[SeqBase[nSeq] + spp->nFrame];

    spp->nFrame++;
    if (spp->nFrame >= SeqSize[nSeq]) {
        spp->nFrame = 0;
    }

    auto pTarget = spp->pTarget;

    if (pTarget == nullptr || pTarget->s().cstat & 0x101)
    {
        switch (nAction)
        {
        default:
            return;

        case 0:
        {
            if ((spp->nPhase & 0x1F) == (totalmoves & 0x1F))
            {
                if (pTarget == nullptr) {
                    pTarget = FindPlayer(spp, 100);
                }

                if (pTarget)
                {
                    spp->nAction = 1;
                    spp->nFrame = 0;
                    spp->pTarget = pTarget;

                    sp->xvel = bcos(sp->ang);
                    sp->yvel = bsin(sp->ang);
                    return;
                }
            }

            break;
        }
        case 1:
        {
            if (pTarget) {
                nVel++;
            }
            goto case_3;
            break;
        }
        case 4:
        {
            if (!spp->nFrame)
            {
                spp->nFrame = 0;
                spp->nAction = 1;
            }
            [[fallthrough]];
        }
        case 3:
        {
        case_3:
            short nSector = sp->sectnum;

            if (sp->cstat & 8)
            {
                sp->zvel = 0;
                sp->z = sector[nSector].ceilingz + (tileHeight(sp->picnum) << 5);

                if (sector[nSector].ceilingstat & 1)
                {
                    sp->cstat ^= 8;
                    sp->zvel = 1;

                    spp->nAction = 3;
                    spp->nFrame = 0;
                }
            }

            if ((totalmoves & 0x1F) == (spp->nPhase & 0x1F))
            {
                PlotCourseToSprite(spp, pTarget);

                if (RandomSize(3))
                {
                    sp->xvel = bcos(sp->ang);
                    sp->yvel = bsin(sp->ang);
                }
                else
                {
                    sp->xvel = 0;
                    sp->yvel = 0;
                }

                if (spp->nAction == 1 && RandomBit())
                {
                    if (sp->cstat & 8)
                    {
                        sp->cstat ^= 8;
                        sp->zvel = 1;
                        sp->z = sector[nSector].ceilingz + GetActorHeight(spp);
                    }
                    else
                    {
                        sp->zvel = -5120;
                    }

                    spp->nAction = 3;
                    spp->nFrame = 0;

                    if (!RandomSize(3)) {
                        D3PlayFX(StaticSound[kSound29], spp);
                    }
                }
            }
            break;
        }
        case 5:
        {
            if (!spp->nFrame)
            {
                runlist_DoSubRunRec(sp->owner);
                runlist_FreeRun(sp->lotag - 1);
                runlist_SubRunRec(spp->nRun);
                sp->cstat = 0x8000;
                DeleteActor(spp);
            }
            return;
        }
        case 2:
        {
            if (pTarget)
            {
                if (nFrameFlag & 0x80)
                {
                    runlist_DamageEnemy(pTarget, spp, 3);
                    D3PlayFX(StaticSound[kSound38], spp);
                }

                if (PlotCourseToSprite(spp, pTarget) < 1024) {
                    return;
                }

                spp->nAction = 1;
            }
            else
            {
                spp->nAction = 0;
                sp->xvel = 0;
                sp->yvel = 0;
            }

            spp->nFrame = 0;
            break;
        }
        }
    }
    else
    {
        spp->pTarget = nullptr;
        spp->nAction = 0;
        spp->nFrame = 0;

        sp->xvel = 0;
        sp->yvel = 0;
    }

    auto nMov = movesprite(spp, sp->xvel << nVel, sp->yvel << nVel, sp->zvel, 1280, -1280, CLIPMASK0);

    if (nMov.type == kHitNone && nMov.exbits == 0)
        return;

    if (nMov.exbits & kHitAux1
        && sp->zvel < 0
        && hiHit.type != kHitSprite
        && !((sector[sp->sectnum].ceilingstat) & 1))
    {
        sp->cstat |= 8;
        sp->z = GetActorHeight(spp) + sector[sp->sectnum].ceilingz;
        sp->zvel = 0;

        spp->nAction = 1;
        spp->nFrame = 0;
        return;
    }
    else
    {
        switch (nMov.type)
        {
        case kHitWall:
        {
            sp->ang = (sp->ang + 256) & 0x7EF;
            sp->xvel = bcos(sp->ang);
            sp->yvel = bsin(sp->ang);
            return;
        }
        case kHitSprite:
        {
            if (nMov.actor == pTarget)
            {
                int nAng = getangle(pTarget->s().x - sp->x, pTarget->s().y - sp->y);
                if (AngleDiff(sp->ang, nAng) < 64)
                {
                    spp->nAction = 2;
                    spp->nFrame = 0;
                }
            }
            return;
        }
        default:
            break;
        }

        if (spp->nAction == 3)
        {
            spp->nAction = 1;
            spp->nFrame = 0;
        }
        return;
    }

    return;
}

void AISpider::Draw(RunListEvent* ev)
{
    auto spp = ev->pObjActor;
    if (!spp) return;

    short nAction = spp->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqSpider] + SpiderSeq[nAction].a, spp->nFrame, SpiderSeq[nAction].b);
}

void AISpider::RadialDamage(RunListEvent* ev)
{
    auto spp = ev->pObjActor;
    if (!spp) return;

    if (spp->nHealth <= 0)
        return;

    ev->nDamage = runlist_CheckRadialDamage(spp);
    Damage(ev);
}

void AISpider::Damage(RunListEvent* ev)
{
    auto spp = ev->pObjActor;
    if (!spp) return;
    auto sp = &spp->s();

    if (!ev->nDamage)
        return;

    DExhumedActor* pTarget = ev->pOtherActor;

    spp->nHealth -= dmgAdjust(ev->nDamage);
    if (spp->nHealth > 0)
    {
        /*
        NOTE:
            nTarget check was added, but should we return if it's invalid instead
            or should code below (action set, b set) happen?
            Other AI doesn't show consistency in this regard (see Scorpion code)
        */
        if (pTarget && pTarget->s().statnum == 100)
        {
            spp->pTarget = pTarget;
        }

        spp->nAction = 4;
        spp->nFrame = 0;
    }
    else
    {
        // creature is dead, make some chunks
        spp->nHealth = 0;
        spp->nAction = 5;
        spp->nFrame = 0;

        sp->cstat &= 0xFEFE;

        nCreaturesKilled++;

        for (int i = 0; i < 7; i++)
        {
            BuildCreatureChunk(spp, seq_GetSeqPicnum(kSeqSpider, i + 41, 0));
        }
    }
}

END_PS_NS
