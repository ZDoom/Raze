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
#include "engine.h"
#include "aistuff.h"
#include "sequence.h"
#include "exhumed.h"
#include "sound.h"
#include <assert.h>

BEGIN_PS_NS

static actionSeq LavadudeSeq[] = {
    {0, 1},
    {0, 1},
    {1, 0},
    {10, 0},
    {19, 0},
    {28, 1},
    {29, 1},
    {33, 0},
    {42, 1}
};

DExhumedActor* BuildLavaLimb(DExhumedActor* pActor, int move, int ebx)
{
    auto pLimbActor = insertActor(pActor->sector(), 118);

    pLimbActor->spr.pos = pActor->spr.pos.plusZ((-RandomLong() % ebx) * zmaptoworld);
    pLimbActor->spr.cstat = 0;
    pLimbActor->spr.shade = -127;
    pLimbActor->spr.pal = 1;
    pLimbActor->vel.X = ((RandomSize(5) - 16) << 4);
    pLimbActor->vel.Y = ((RandomSize(5) - 16) << 4);
    pLimbActor->vel.Z = 10 - RandomSize(5);
    pLimbActor->spr.xoffset = 0;
    pLimbActor->spr.yoffset = 0;
    pLimbActor->spr.xrepeat = 90;
    pLimbActor->spr.yrepeat = 90;
    pLimbActor->spr.picnum = (move & 3) % 3;
    pLimbActor->spr.hitag = 0;
    pLimbActor->spr.lotag = runlist_HeadRun() + 1;
    pLimbActor->set_const_clipdist(0);

//	GrabTimeSlot(3);

    pLimbActor->spr.extra = -1;
    pLimbActor->spr.intowner = runlist_AddRunRec(pLimbActor->spr.lotag - 1, pLimbActor, 0x160000);
    pLimbActor->spr.hitag = runlist_AddRunRec(NewRun, pLimbActor, 0x160000);

    return pLimbActor;
}

void AILavaDudeLimb::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    pActor->spr.shade += 3;

    auto coll = movesprite(pActor, pActor->vel, 4096., 2560, -2560, CLIPMASK1);

    if (coll.type || pActor->spr.shade > 100)
    {
        pActor->vel.X = 0;
        pActor->vel.Y = 0;
        pActor->vel.Z = 0;

        runlist_DoSubRunRec(pActor->spr.intowner);
        runlist_FreeRun(pActor->spr.lotag - 1);
        runlist_SubRunRec(pActor->spr.hitag);

        DeleteActor(pActor);
    }
}

void AILavaDudeLimb::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    seq_PlotSequence(ev->nParam, (SeqOffsets[kSeqLavag] + 30) + pActor->spr.picnum, 0, 1);
}


void BuildLava(DExhumedActor* pActor, const DVector3& pos, sectortype* pSector, DAngle nAngle, int nChannel)
{
    if (pActor == nullptr)
    {
        pActor = insertActor(pSector, 118);
		pActor->spr.pos = pos;
    }
    else
    {
        pSector = pActor->sector();
        nAngle = pActor->spr.angle;
		pActor->spr.pos.Z = pSector->floorz;

        ChangeActorStat(pActor, 118);
    }

    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    pActor->spr.xrepeat = 200;
    pActor->spr.yrepeat = 200;
    pActor->spr.shade = -12;
    pActor->spr.pal = 0;
    pActor->set_const_clipdist(127);
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.picnum = seq_GetSeqPicnum(kSeqLavag, LavadudeSeq[3].a, 0);
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
    pActor->spr.angle = nAngle;
    pActor->spr.hitag = 0;
    pActor->spr.lotag = runlist_HeadRun() + 1;

//	GrabTimeSlot(3);

    pActor->spr.extra = -1;

    pActor->nAction = 0;
    pActor->nHealth = 4000;
    pActor->pTarget = nullptr;
    pActor->nCount = nChannel;
    pActor->nFrame = 0;
    pActor->nPhase = Counters[kCountLava]++;

    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x150000);
    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x150000);

    nCreaturesTotal++;
}

void AILavaDude::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    int nAction = pActor->nAction;
    int nSeq = LavadudeSeq[nAction].a + SeqOffsets[kSeqLavag];

    seq_PlotSequence(ev->nParam, nSeq, pActor->nFrame, LavadudeSeq[nAction].b);
    ev->pTSprite->ownerActor = nullptr;
    return;
}

void AILavaDude::Damage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    int nAction = pActor->nAction;

    if (!ev->nDamage) 
    {
        return;
    }

    pActor->nHealth -= dmgAdjust(ev->nDamage, 3);

    if (pActor->nHealth <= 0)
    {
        pActor->nHealth = 0;
        pActor->nAction = 5;
        pActor->nFrame = 0;

        nCreaturesKilled++;

        pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
    }
    else
    {
        auto pTarget = ev->pOtherActor;

        if (pTarget)
        {
            if (pTarget->spr.statnum < 199)
            {
                pActor->pTarget = pTarget;
            }
        }

        if (nAction == 3)
        {
            if (!RandomSize(2))
            {
                pActor->nAction = 4;
                pActor->nFrame = 0;
                pActor->spr.cstat = 0;
            }
        }

        BuildLavaLimb(pActor, totalmoves, 64000);
    }
}

void AILavaDude::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    int nAction = pActor->nAction;
    int nSeq = LavadudeSeq[nAction].a + SeqOffsets[kSeqLavag];

    pActor->spr.picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);
    int var_38 = pActor->nFrame;

    int nFlag = FrameFlag[SeqBase[nSeq] + var_38];

    int var_1C = 0;

    if (nAction)
    {
        seq_MoveSequence(pActor, nSeq, var_38);

        pActor->nFrame++;
        if (pActor->nFrame >= SeqSize[nSeq])
        {
            var_1C = 1;
            pActor->nFrame = 0;
        }
        else
        {
            var_1C = 0;
        }
    }

    DExhumedActor* pTarget = pActor->pTarget;

    if (pTarget && nAction < 4)
    {
        if (!(pTarget->spr.cstat & CSTAT_SPRITE_BLOCK_ALL) || pTarget->spr.statnum == MAXSTATUS)
        {
            pTarget = nullptr;
            pActor->pTarget = nullptr;
        }
    }

    switch (nAction)
    {
    case 0:
    {
        if ((pActor->nPhase & 0x1F) == (totalmoves & 0x1F))
        {
            if (pTarget == nullptr)
            {
                pTarget = FindPlayer(pActor, 76800);
            }

            PlotCourseToSprite(pActor, pTarget);

            pActor->VelFromAngle();

            if (pTarget && !RandomSize(1))
            {
                pActor->pTarget = pTarget;
                pActor->nAction = 2;
                pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
                pActor->nFrame = 0;
                break;
            }
        }

		auto pos = pActor->spr.pos;
        auto pSector =pActor->sector();

        auto coll = movesprite(pActor, DVector3(pActor->vel.XY(), 0), 256., 0, 0, CLIPMASK0);

        if (pSector != pActor->sector())
        {
            ChangeActorSect(pActor, pSector);
			pActor->spr.pos = pos;

            pActor->set_int_ang((pActor->int_ang() + ((RandomWord() & 0x3FF) + 1024)) & kAngleMask);
            pActor->VelFromAngle();
            break;
        }

        if (coll.type == kHitNone) {
            break;
        }

        if (coll.type == kHitWall)
        {
            pActor->set_int_ang((pActor->int_ang() + ((RandomWord() & 0x3FF) + 1024)) & kAngleMask);
            pActor->VelFromAngle();
            break;
        }
        else if (coll.type == kHitSprite)
        {
            if (coll.actor() == pTarget)
            {
				auto nAngDiff = AngleDiff(pActor->spr.angle, VecToAngle(pTarget->spr.pos - pActor->spr.pos));
				if (nAngDiff < 64)
                {
                    pActor->nAction = 2;
                    pActor->nFrame = 0;
                    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
                    break;
                }
            }
        }

        break;
    }

    case 1:
    case 6:
    {
        break;
    }

    case 2:
    {
        if (var_1C)
        {
            pActor->nAction = 3;
            pActor->nFrame = 0;

            PlotCourseToSprite(pActor, pTarget);

            pActor->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
        }

        break;
    }

    case 3:
    {
        if ((nFlag & 0x80) && pTarget)
        {
             BuildBullet(pActor, 10, -1, pActor->spr.angle, pTarget, 1);
        }
        else if (var_1C)
        {
            PlotCourseToSprite(pActor, pTarget);
            pActor->nAction = 7;
            pActor->nFrame = 0;
        }

        break;
    }

    case 4:
    {
        if (var_1C)
        {
            pActor->nAction = 7;
            pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
        }

        break;
    }

    case 5:
    {
        if (nFlag & 0x40)
        {
            auto pLimbSprite = BuildLavaLimb(pActor, pActor->nFrame, 64000);
            D3PlayFX(StaticSound[kSound26], pLimbSprite);
        }

        if (pActor->nFrame)
        {
            if (nFlag & 0x80)
            {
                int ecx = 0;
                do
                {
                    BuildLavaLimb(pActor, ecx, 64000);
                    ecx++;
                } while (ecx < 20);
                runlist_ChangeChannel(pActor->nCount, 1);
            }
        }
        else
        {
            int ecx = 0;

            do
            {
                BuildLavaLimb(pActor, ecx, 256);
                ecx++;
            } while (ecx < 30);

            runlist_DoSubRunRec(pActor->spr.intowner);
            runlist_FreeRun(pActor->spr.lotag - 1);
            runlist_SubRunRec(pActor->nRun);
            DeleteActor(pActor);
        }

        break;
    }

    case 7:
    {
        if (var_1C)
        {
            pActor->nAction = 8;
            pActor->nFrame = 0;
        }
        break;
    }

    case 8:
    {
        if (var_1C)
        {
            pActor->nAction = 0;
            pActor->nFrame = 0;
            pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
        }
        break;
    }
    }

    // loc_31521:
    pActor->spr.pal = 1;
}

END_PS_NS
