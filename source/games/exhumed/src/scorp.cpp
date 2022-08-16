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
#include "sound.h"
#include <assert.h>

BEGIN_PS_NS

static actionSeq ScorpSeq[] = {
    {0, 0},
    {8, 0},
    {29, 0},
    {19, 0},
    {45, 1},
    {46, 1},
    {47, 1},
    {48, 1},
    {50, 1},
    {53, 1}
};

void BuildScorp(DExhumedActor* pActor, int x, int y, int z, sectortype* pSector, int nAngle, int nChannel)
{
    if (pActor == nullptr)
    {
        pActor = insertActor(pSector, 122);
    }
    else
    {
        ChangeActorStat(pActor, 122);

        x = pActor->int_pos().X;
        y = pActor->int_pos().Y;
        z = pActor->sector()->int_floorz();
        nAngle = pActor->int_ang();
    }

	pActor->set_int_pos({ x, y, z });
    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pActor->spr.clipdist = 70;
    pActor->spr.shade = -12;
    pActor->spr.xrepeat = 80;
    pActor->spr.yrepeat = 80;
    pActor->spr.picnum = 1;
    pActor->spr.pal = pActor->sector()->ceilingpal;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->set_int_ang(nAngle);
    pActor->spr.xvel = 0;
    pActor->spr.yvel = 0;
    pActor->spr.zvel = 0;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.extra = -1;
    pActor->spr.hitag = 0;

    //	GrabTimeSlot(3);

    pActor->nHealth = 20000;
    pActor->nFrame = 0;
    pActor->nAction = 0;
    pActor->pTarget = nullptr;
    pActor->nCount = 0;
    pActor->nIndex2 = 1;
	pActor->nPhase = Counters[kCountScorp]++;

    pActor->nChannel = nChannel;

    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x220000);
    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x220000);

    nCreaturesTotal++;
}

void AIScorp::Draw(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

    int nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqScorp] + ScorpSeq[nAction].a, pActor->nFrame, ScorpSeq[nAction].b);
}

void AIScorp::RadialDamage(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

    ev->nDamage = runlist_CheckRadialDamage(pActor);
    if (ev->nDamage) Damage(ev);
}


void AIScorp::Damage(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

    DExhumedActor* pTarget = nullptr;

    if (pActor->nHealth <= 0) {
        return;
    }

    pActor->nHealth -= dmgAdjust(ev->nDamage);

    if (pActor->nHealth <= 0)
    {
        pActor->nHealth = 0;
        pActor->nAction = 4;
        pActor->nFrame = 0;
        pActor->nCount = 10;

        pActor->spr.xvel = 0;
        pActor->spr.yvel = 0;
        pActor->spr.zvel = 0;
        pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;

        nCreaturesKilled++;
        return;
    }
    else
    {
        pTarget = ev->pOtherActor;

        if (pTarget)
        {
            if (pActor->spr.statnum == 100 || (pActor->spr.statnum < 199 && !RandomSize(5)))
            {
                pActor->pTarget = pTarget;
            }
        }

        if (!RandomSize(5))
        {
            pActor->nAction = RandomSize(2) + 4;
            pActor->nFrame = 0;
            return;
        }

        if (RandomSize(2)) {
            return;
        }

        D3PlayFX(StaticSound[kSound41], pActor);
        Effect(ev, pTarget, 0);
    }
}

void AIScorp::Tick(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

    int nAction = pActor->nAction;
    bool bVal = false;

    DExhumedActor* pTarget = nullptr;

    if (pActor->nHealth) {
        Gravity(pActor);
    }

    int nSeq = SeqOffsets[kSeqScorp] + ScorpSeq[nAction].a;

    pActor->spr.picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);
    seq_MoveSequence(pActor, nSeq, pActor->nFrame);

    pActor->nFrame++;

    if (pActor->nFrame >= SeqSize[nSeq])
    {
        pActor->nFrame = 0;
        bVal = true;
    }

    int nFlag = FrameFlag[SeqBase[nSeq] + pActor->nFrame];
    pTarget = pActor->pTarget;

    switch (nAction)
    {
    default:
        return;

    case 0:
    {
        if (pActor->nCount > 0)
        {
            pActor->nCount--;
            return;
        }

        if ((pActor->nPhase & 0x1F) == (totalmoves & 0x1F))
        {
            if (pTarget == nullptr)
            {
                pTarget = FindPlayer(pActor, 500);

                if (pTarget)
                {
                    D3PlayFX(StaticSound[kSound41], pActor);

                    pActor->nFrame = 0;
                    pActor->spr.xvel = bcos(pActor->int_ang());
                    pActor->spr.yvel = bsin(pActor->int_ang());

                    pActor->nAction = 1;
                    pActor->pTarget = pTarget;
                }
            }
        }

        return;
    }

    case 1:
    {
        pActor->nIndex2--;

        if (pActor->nIndex2 <= 0)
        {
            pActor->nIndex2 = RandomSize(5);
            Effect(ev, pTarget, 0);
        }
        else
        {
            auto nMov = MoveCreatureWithCaution(pActor);
            if (nMov.type == kHitSprite)
            {
                if (pTarget == nMov.actor())
                {
                    int nAngle = getangle(pTarget->int_pos().X - pActor->int_pos().X, pTarget->int_pos().Y - pActor->int_pos().Y);
                    if (AngleDiff(pActor->int_ang(), nAngle) < 64)
                    {
                        pActor->nAction = 2;
                        pActor->nFrame = 0;
                    }
                    Effect(ev, pTarget, 2);
                }
                else
                {
                    Effect(ev, pTarget, 0);
                }
                return;
            }
            else if (nMov.type == kHitWall)
            {
                Effect(ev, pTarget, 0);
            }
            else
            {
                Effect(ev, pTarget, 1);
            }
        }
        return;
    }

    case 2:
    {
        if (pTarget == nullptr)
        {
            pActor->nAction = 0;
            pActor->nCount = 5;
        }
        else
        {
            if (PlotCourseToSprite(pActor, pTarget) >= 768)
            {
                pActor->nAction = 1;
            }
            else if (nFlag & 0x80)
            {
                runlist_DamageEnemy(pTarget, pActor, 7);
            }
        }
        Effect(ev, pTarget, 2);
        return;
    }

    case 3:
    {
        if (bVal)
        {
            pActor->nIndex--;
            if (pActor->nIndex <= 0)
            {
                pActor->nAction = 1;

                pActor->spr.xvel = bcos(pActor->int_ang());
                pActor->spr.yvel = bsin(pActor->int_ang());

                pActor->nFrame = 0;
                return;
            }
        }

        if (!(nFlag & 0x80)) {
            return;
        }

        auto nBulletSprite = BuildBullet(pActor, 16, -1, pActor->int_ang(), pTarget, 1);
        if (nBulletSprite)
        {
            PlotCourseToSprite(nBulletSprite, pTarget);
        }

        return;
    }

    case 4:
    case 5:
    case 6:
    case 7:
    {
        if (!bVal) {
            return;
        }

        if (pActor->nHealth > 0)
        {
            pActor->nAction = 1;
            pActor->nFrame = 0;
            pActor->nCount = 0;
            return;
        }

        pActor->nCount--;
        if (pActor->nCount <= 0)
        {
            pActor->nAction = 8;
        }
        else
        {
            pActor->nAction = RandomBit() + 6;
        }

        return;
    }

    case 8:
    {
        if (bVal)
        {
            pActor->nAction++; // set to 9
            pActor->nFrame = 0;

            runlist_ChangeChannel(pActor->nChannel, 1);
            return;
        }

        auto pSpiderActor = BuildSpider(nullptr, pActor->int_pos().X, pActor->int_pos().Y, pActor->int_pos().Z, pActor->sector(), pActor->int_ang());
        if (pSpiderActor)
        {
            pSpiderActor->set_int_ang(RandomSize(11));

            int nVel = RandomSize(5) + 1;

            pSpiderActor->spr.xvel = bcos(pSpiderActor->int_ang(), -8) * nVel;
            pSpiderActor->spr.yvel = bsin(pSpiderActor->int_ang(), -8) * nVel;
            pSpiderActor->spr.zvel = (-(RandomSize(5) + 3)) << 8;
        }

        return;
    }

    case 9:
    {
        pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;

        if (bVal)
        {
            runlist_SubRunRec(pActor->nRun);
            runlist_DoSubRunRec(pActor->spr.intowner);
            runlist_FreeRun(pActor->spr.lotag - 1);

            DeleteActor(pActor);
        }

        return;
    }
    }
}

void AIScorp::Effect(RunListEvent* ev, DExhumedActor* pTarget, int mode)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

    int nAction = pActor->nAction;

    if (mode == 0)
    {
        PlotCourseToSprite(pActor, pTarget);
        pActor->add_int_ang(RandomSize(7) - 63);
        pActor->norm_ang();

        pActor->spr.xvel = bcos(pActor->int_ang());
        pActor->spr.yvel = bsin(pActor->int_ang());
    }
    if (mode <= 1)
    {
        if (pActor->nCount)
        {
            pActor->nCount--;
        }
        else
        {
            pActor->nCount = 45;

            if (cansee(pActor->int_pos().X, pActor->int_pos().Y, pActor->int_pos().Z - GetActorHeight(pActor), pActor->sector(),
                pTarget->int_pos().X, pTarget->int_pos().Y, pTarget->int_pos().Z - GetActorHeight(pTarget), pTarget->sector()))
            {
                pActor->spr.xvel = 0;
                pActor->spr.yvel = 0;
                pActor->set_int_ang(GetMyAngle(pTarget->int_pos().X - pActor->int_pos().X, pTarget->int_pos().Y - pActor->int_pos().Y));

                pActor->nIndex = RandomSize(2) + RandomSize(3);

                if (!pActor->nIndex) {
                    pActor->nCount = RandomSize(5);
                }
                else
                {
                    pActor->nAction = 3;
                    pActor->nFrame = 0;
                }
            }
        }
    }

    if (!nAction || pTarget == nullptr) {
        return;
    }

    if (!(pTarget->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
    {
        pActor->nAction = 0;
        pActor->nFrame = 0;
        pActor->nCount = 30;
        pActor->pTarget = nullptr;

        pActor->spr.xvel = 0;
        pActor->spr.yvel = 0;
    }
}


END_PS_NS
