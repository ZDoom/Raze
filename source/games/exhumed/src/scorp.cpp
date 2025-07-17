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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void BuildScorp(DExhumedActor* pActor, const DVector3& pos, sectortype* pSector, DAngle nAngle, int nChannel)
{
	if (pActor == nullptr)
	{
		pActor = insertActor(pSector, 122);
		pActor->spr.pos = pos;
	}
	else
	{
		ChangeActorStat(pActor, 122);
		pActor->spr.pos.Z = pActor->sector()->floorz;
		nAngle = pActor->spr.Angles.Yaw;
	}

    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
	pActor->clipdist = 17.5;
    pActor->spr.shade = -12;
	pActor->spr.scale = DVector2(1.25, 1.25);
    setvalidpic(pActor);
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

    pActor->nSeqFile = "scorp";

    Level.addKillCount();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIScorp::Draw(RunListEvent* ev)
{
	if (const auto pActor = ev->pObjActor)
    {
        const auto scorpSeq = &ScorpSeq[pActor->nAction];
        seq_PlotSequence(ev->nParam, pActor->nSeqFile, scorpSeq->nSeqId, pActor->nFrame, scorpSeq->nFlags);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIScorp::RadialDamage(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

    ev->nDamage = runlist_CheckRadialDamage(pActor);
    if (ev->nDamage) Damage(ev);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

        pActor->vel.X = 0;
        pActor->vel.Y = 0;
        pActor->vel.Z = 0;
        pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;

        Level.addKill(-1);
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

    const auto scorpSeq = getSequence(pActor->nSeqFile, ScorpSeq[nAction].nSeqId);
    const auto& seqFrame = scorpSeq->frames[pActor->nFrame];

    pActor->spr.setspritetexture(seqFrame.getFirstChunkTexture());
    seqFrame.playSound(pActor);

    pActor->nFrame++;

    if (pActor->nFrame >= scorpSeq->frames.Size())
    {
        pActor->nFrame = 0;
        bVal = true;
    }

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
                    pActor->VelFromAngle();

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
                    auto nAngDiff = absangle(pActor->spr.Angles.Yaw, (pTarget->spr.pos - pActor->spr.pos).Angle());
                    if (nAngDiff < DAngle22_5 / 2)
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
            pActor->nFrame = 0;
            pActor->nCount = 5;
        }
        else
        {
            if (PlotCourseToSprite(pActor, pTarget) >= 48)
            {
                pActor->nAction = 1;
                pActor->nFrame = 0;
            }
            else if (seqFrame.flags & 0x80)
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

                pActor->VelFromAngle();

                pActor->nFrame = 0;
                return;
            }
        }

        if (!(seqFrame.flags & 0x80)) {
            return;
        }

        auto nBulletSprite = BuildBullet(pActor, 16, INT_MAX, pActor->spr.Angles.Yaw, pTarget, 1);
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
        pActor->nFrame = 0;

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

        auto pSpiderActor = BuildSpider(nullptr, pActor->spr.pos, pActor->sector(), pActor->spr.Angles.Yaw);
        if (pSpiderActor)
        {
            pSpiderActor->spr.Angles.Yaw = RandomAngle();

            int nVel = RandomSize(5) + 1;

			pSpiderActor->vel.SetXY(pSpiderActor->spr.Angles.Yaw.ToVector() * 4 * nVel);
            pSpiderActor->vel.Z = -(RandomSize(5) + 3);
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIScorp::Effect(RunListEvent* ev, DExhumedActor* pTarget, int mode)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

    int nAction = pActor->nAction;

    if (mode == 0)
    {
        PlotCourseToSprite(pActor, pTarget);
        pActor->spr.Angles.Yaw += mapangle(RandomSize(7) - 63);
        pActor->norm_ang();

        pActor->VelFromAngle();
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

            if (cansee(pActor->spr.pos.plusZ(-GetActorHeight(pActor)), pActor->sector(),
                pTarget->spr.pos.plusZ(-GetActorHeight(pTarget)), pTarget->sector()))
            {
                pActor->vel.X = 0;
                pActor->vel.Y = 0;
				pActor->spr.Angles.Yaw = (pTarget->spr.pos - pActor->spr.pos).Angle();

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

        pActor->vel.X = 0;
        pActor->vel.Y = 0;
    }
}


END_PS_NS
