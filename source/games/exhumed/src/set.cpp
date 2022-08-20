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
#include <assert.h>

BEGIN_PS_NS

static actionSeq SetSeq[] = {
    {0, 0},
    {77, 1},
    {78, 1},
    {0, 0},
    {9, 0},
    {63, 0},
    {45, 0},
    {18, 0},
    {27, 0},
    {36, 0},
    {72, 1},
    {74, 1}
};

void BuildSet(DExhumedActor* pActor, const DVector3& pos, sectortype* pSector, int nAngle, int nChannel)
{
    if (pActor == nullptr)
    {
        pActor = insertActor(pSector, 120);
		pActor->spr.pos = pos;
    }
    else
    {
        ChangeActorStat(pActor, 120);
		pActor->spr.pos.Z = pActor->sector()->floorz;
        nAngle = pActor->int_ang();
    }

    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pActor->spr.shade = -12;
    pActor->spr.clipdist = 110;
    pActor->spr.xvel = 0;
    pActor->spr.yvel = 0;
    pActor->spr.zvel = 0;
    pActor->spr.xrepeat = 87;
    pActor->spr.yrepeat = 96;
    pActor->spr.pal = pActor->sector()->ceilingpal;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->set_int_ang(nAngle);
    pActor->spr.picnum = 1;
    pActor->spr.hitag = 0;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.extra = -1;

    //	GrabTimeSlot(3);

    pActor->nAction = 1;
    pActor->nHealth = 8000;
    pActor->nFrame = 0;
    pActor->pTarget = nullptr;
    pActor->nCount = 90;
    pActor->nIndex = 0;
    pActor->nIndex2 = 0;
	pActor->nPhase = Counters[kCountSet]++;

    pActor->nChannel = nChannel;

    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x190000);

    // this isn't stored anywhere.
    runlist_AddRunRec(NewRun, pActor, 0x190000);

    nCreaturesTotal++;
}

void BuildSoul(DExhumedActor* pSet)
{
    auto pActor = insertActor(pSet->sector(), 0);

    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    pActor->spr.shade = -127;
    pActor->spr.xrepeat = 1;
    pActor->spr.yrepeat = 1;
    pActor->spr.pal = 0;
    pActor->spr.clipdist = 5;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.picnum = seq_GetSeqPicnum(kSeqSet, 75, 0);
    pActor->set_int_ang(RandomSize(11));
    pActor->spr.xvel = 0;
    pActor->spr.yvel = 0;
    pActor->spr.zvel = (-256) - RandomSize(10);
    pActor->spr.pos = DVector3(pSet->spr.pos.XY(), RandomSize(8) + 32 + pActor->sector()->ceilingz - GetActorHeightF(pActor));

    //pActor->spr.hitag = nSet;
	pActor->pTarget = pSet;
	pActor->nPhase = Counters[kCountSoul]++;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.extra = 0;

    //	GrabTimeSlot(3);

    pActor->spr.intowner = runlist_AddRunRec(NewRun, pActor, 0x230000);
}

void AISoul::Tick(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

    seq_MoveSequence(pActor, SeqOffsets[kSeqSet] + 75, 0);

    if (pActor->spr.xrepeat < 32)
    {
        pActor->spr.xrepeat++;
        pActor->spr.yrepeat++;
    }

    pActor->spr.extra += (pActor->nPhase & 0x0F) + 5;
    pActor->spr.extra &= kAngleMask;

    int nVel = bcos(pActor->spr.extra, -7);

	auto coll = movesprite(pActor, bcos(pActor->int_ang()) * nVel, bsin(pActor->int_ang()) * nVel, pActor->spr.zvel, 5120, 0, CLIPMASK0);
    if (coll.exbits & 0x10000)
    {
		DExhumedActor* pSet = pActor->pTarget;
		if (!pSet) return;

        pActor->spr.cstat = 0;
        pActor->spr.yrepeat = 1;
        pActor->spr.xrepeat = 1;
        pActor->spr.pos = pSet->spr.pos.plusZ(-GetActorHeightF(pSet) * 0.5);
        ChangeActorSect(pActor, pSet->sector());
        return;
    }
}


void AISet::RadialDamage(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;
    int nAction = pActor->nAction;

    if (nAction == 5)
    {
        ev->nDamage = runlist_CheckRadialDamage(pActor);
        // fall through to case 0x80000
    }
    Damage(ev);
}

void AISet::Damage(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

	int nAction = pActor->nAction;

    if (ev->nDamage && pActor->nHealth > 0)
    {
        if (nAction != 1)
        {
            pActor->nHealth -= dmgAdjust(ev->nDamage);
        }

        if (pActor->nHealth <= 0)
        {
            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;
            pActor->spr.zvel = 0;
            pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;

            pActor->nHealth = 0;

            nCreaturesKilled++;

            if (nAction < 10)
            {
                pActor->nFrame = 0;
                pActor->nAction = 10;
            }
        }
        else if (nAction == 1)
        {
            pActor->nAction = 2;
            pActor->nFrame = 0;
        }
    }
}

void AISet::Draw(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;
    int nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqSet] + SetSeq[nAction].a, pActor->nFrame, SetSeq[nAction].b);
    return;
}

void AISet::Tick(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

	int nAction = pActor->nAction;

    bool bVal = false;

    Gravity(pActor);

    int nSeq = SeqOffsets[kSeqSet] + SetSeq[pActor->nAction].a;
    pActor->spr.picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);
    seq_MoveSequence(pActor, nSeq, pActor->nFrame);

    if (nAction == 3)
    {
        if (pActor->nIndex2) {
            pActor->nFrame++;
        }
    }

    pActor->nFrame++;
    if (pActor->nFrame >= SeqSize[nSeq])
    {
        pActor->nFrame = 0;
        bVal = true;
    }

    int nFlag = FrameFlag[SeqBase[nSeq] + pActor->nFrame];
    DExhumedActor* pTarget = pActor->pTarget;

    if (pTarget && nAction < 10)
    {
        if (!(pTarget->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
        {
            pActor->pTarget = nullptr;
            pActor->nAction = 0;
            pActor->nFrame = 0;
            pTarget = nullptr;
        }
    }

    auto nMov = MoveCreature(pActor);

	auto sect = pActor->sector();
    pushmove(pActor, &sect, pActor->spr.clipdist << 2, 5120, -5120, CLIPMASK0);
    pActor->setsector(sect);

    if (pActor->spr.zvel > 4000)
    {
        if (nMov.exbits & kHitAux2)
        {
            SetQuake(pActor, 100);
        }
    }

    switch (nAction)
    {
    default:
        return;

    case 0:
    {
        if ((pActor->nPhase & 0x1F) == (totalmoves & 0x1F))
        {
            if (pTarget == nullptr)
            {
                pTarget = FindPlayer(pActor, 1000);
            }

            if (pTarget)
            {
                pActor->nAction = 3;
                pActor->nFrame = 0;
                pActor->pTarget = pTarget;

                pActor->spr.xvel = bcos(pActor->int_ang(), -1);
                pActor->spr.yvel = bsin(pActor->int_ang(), -1);
            }
        }

        return;
    }

    case 1:
    {
        if (FindPlayer(pActor, 1000))
        {
            pActor->nCount--;
            if (pActor->nCount <= 0)
            {
                pActor->nAction = 2;
                pActor->nFrame = 0;
            }
        }

        return;
    }

    case 2:
    {
        if (bVal)
        {
            pActor->nAction = 7;
            pActor->nIndex = 0;
            pActor->nFrame = 0;

            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;

            pActor->pTarget = FindPlayer(pActor, 1000);
        }
        return;
    }

    case 3:
    {
        if (pTarget != nullptr)
        {
            if ((nFlag & 0x10) && (nMov.exbits & kHitAux2))
            {
                SetQuake(pActor, 100);
            }

            int nCourse = PlotCourseToSprite(pActor, pTarget);

            if ((pActor->nPhase & 0x1F) == (totalmoves & 0x1F))
            {
                int nRand = RandomSize(3);

                switch (nRand)
                {
                case 0:
                case 2:
                {
                    pActor->nIndex = 0;
                    pActor->nAction = 7;
                    pActor->nFrame = 0;
                    pActor->spr.xvel = 0;
                    pActor->spr.yvel = 0;
                    return;
                }
                case 1:
                {
                    PlotCourseToSprite(pActor, pTarget);

                    pActor->nAction = 6;
                    pActor->nFrame = 0;
                    pActor->nRun = 5;
                    pActor->spr.xvel = 0;
                    pActor->spr.yvel = 0;
                    return;
                }
                default:
                {
                    if (nCourse <= 100)
                    {
                        pActor->nIndex2 = 0;
                    }
                    else
                    {
                        pActor->nIndex2 = 1;
                    }
                    break;
                }
                }
            }

            // loc_338E2
            int nAngle = pActor->int_ang() & 0xFFF8;
            pActor->spr.xvel = bcos(nAngle, -1);
            pActor->spr.yvel = bsin(nAngle, -1);

            if (pActor->nIndex2)
            {
                pActor->spr.xvel *= 2;
                pActor->spr.yvel *= 2;
            }

            if (nMov.type == kHitWall)
            {
                auto pSector = nMov.hitWall->nextSector();

                if (pSector)
                {
                    if ((pActor->spr.pos.Z - pSector->floorz) < (55000/256.))
                    {
                        if (pActor->spr.pos.Z > pSector->ceilingz)
                        {
                            pActor->nIndex = 1;
                            pActor->nAction = 7;
                            pActor->nFrame = 0;
                            pActor->spr.xvel = 0;
                            pActor->spr.yvel = 0;
                            return;
                        }
                    }
                }

                pActor->set_int_ang((pActor->int_ang() + 256) & kAngleMask);
                pActor->spr.xvel = bcos(pActor->int_ang(), -1);
                pActor->spr.yvel = bsin(pActor->int_ang(), -1);
                break;
            }
            else if (nMov.type == kHitSprite)
            {
                if (pTarget == nMov.actor())
                {
					auto nAngDiff = AngleDiff(pActor->spr.angle, VecToAngle(pTarget->spr.pos - pActor->spr.pos));
					if (nAngDiff < 64)
                   {
                        pActor->nAction = 4;
                        pActor->nFrame = 0;
                    }
                    break;
                }
                else
                {
                    pActor->nIndex = 1;
                    pActor->nAction = 7;
                    pActor->nFrame = 0;
                    pActor->spr.xvel = 0;
                    pActor->spr.yvel = 0;
                    return;
                }
            }

            break;
        }
        else
        {
            pActor->nAction = 0;
            pActor->nFrame = 0;
            return;
        }
    }

    case 4:
    {
        if (pTarget == nullptr)
        {
            pActor->nAction = 0;
            pActor->nCount = 50;
        }
        else
        {
            if (PlotCourseToSprite(pActor, pTarget) >= 768)
            {
                pActor->nAction = 3;
            }
            else if (nFlag & 0x80)
            {
                runlist_DamageEnemy(pTarget, pActor, 5);
            }
        }

        break;
    }

    case 5:
    {
        if (bVal)
        {
            pActor->nAction = 0;
            pActor->nCount = 15;
        }
        return;
    }

    case 6:
    {
        if (nFlag & 0x80)
        {
            auto pBullet = BuildBullet(pActor, 11, -1, pActor->int_ang(), pTarget, 1);
            if (pBullet)
				SetBulletEnemy(pBullet->nPhase, pTarget);

            pActor->nRun--;
            if (pActor->nRun <= 0 || !RandomBit())
            {
                pActor->nAction = 0;
                pActor->nFrame = 0;
            }
        }
        return;
    }

    case 7:
    {
        if (bVal)
        {
            if (pActor->nIndex)
            {
                pActor->spr.zvel = -10000;
            }
            else
            {
                pActor->spr.zvel = -(PlotCourseToSprite(pActor, pTarget));
            }

            pActor->nAction = 8;
            pActor->nFrame = 0;

            pActor->spr.xvel = bcos(pActor->int_ang());
            pActor->spr.yvel = bsin(pActor->int_ang());
        }
        return;
    }

    case 8:
    {
        if (bVal)
        {
            pActor->nFrame = SeqSize[nSeq] - 1;
        }

        if (nMov.exbits & kHitAux2)
        {
            SetQuake(pActor, 200);
            pActor->nAction = 9;
            pActor->nFrame = 0;
        }
        return;
    }

    case 9:
    {
        pActor->spr.xvel >>= 1;
        pActor->spr.yvel >>= 1;

        if (bVal)
        {
            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;

            PlotCourseToSprite(pActor, pTarget);

            pActor->nAction = 6;
            pActor->nFrame = 0;
            pActor->nRun = 5;

            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;
        }
        return;
    }

    case 10:
    {
        if (nFlag & 0x80)
        {
            pActor->spr.pos.Z -= GetActorHeightF(pActor);
            BuildCreatureChunk(pActor, seq_GetSeqPicnum(kSeqSet, 76, 0));
			pActor->spr.pos.Z += GetActorHeightF(pActor);
        }

        if (bVal)
        {
            pActor->nAction = 11;
            pActor->nFrame = 0;

            runlist_ChangeChannel(pActor->nChannel, 1);

            for (int i = 0; i < 20; i++)
            {
                BuildSoul(pActor);
            }
        }
        return;
    }

    case 11:
    {
        pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
        return;
    }
    }

    // loc_33AE3: ?
    if (nAction)
    {
        if (pTarget)
        {
            if (!(pTarget->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
            {
                pActor->nAction = 0;
                pActor->nFrame = 0;
                pActor->nCount = 100;
                pActor->pTarget = nullptr;
                pActor->spr.xvel = 0;
                pActor->spr.yvel = 0;
            }
        }
    }

    return;
}

END_PS_NS
