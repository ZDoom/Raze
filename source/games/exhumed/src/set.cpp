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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void BuildSet(DExhumedActor* pActor, const DVector3& pos, sectortype* pSector, DAngle nAngle, int nChannel)
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
        nAngle = pActor->spr.Angles.Yaw;
    }

    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pActor->spr.shade = -12;
	pActor->clipdist = 27.5;
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
	pActor->spr.scale = DVector2(1.359375, 1.5);
    pActor->spr.pal = pActor->sector()->ceilingpal;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.Angles.Yaw = nAngle;
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

    pActor->nSeqFile = "set";

    // this isn't stored anywhere.
    runlist_AddRunRec(NewRun, pActor, 0x190000);

    nCreaturesTotal++;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void BuildSoul(DExhumedActor* pSet)
{
    auto pActor = insertActor(pSet->sector(), 0);

    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    pActor->spr.shade = -127;
    pActor->spr.scale = DVector2(REPEAT_SCALE, REPEAT_SCALE);
    pActor->spr.pal = 0;
	pActor->clipdist = 1.25;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->nSeqFile = "set";
    pActor->spr.picnum = getSequence(pActor->nSeqFile, 75).getFirstPicnum();
    pActor->spr.Angles.Yaw = RandomAngle();
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = -1 - RandomSize(10) / 256.;
    pActor->spr.pos = DVector3(pSet->spr.pos.XY(), RandomSize(8) + 32 + pActor->sector()->ceilingz - GetActorHeight(pActor));

    //pActor->spr.hitag = nSet;
	pActor->pTarget = pSet;
	pActor->nPhase = Counters[kCountSoul]++;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.extra = 0;

    //	GrabTimeSlot(3);

    pActor->spr.intowner = runlist_AddRunRec(NewRun, pActor, 0x230000);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AISoul::Tick(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

    playFrameSound(pActor, getSequence("set", 75).frames[0]);

    if (pActor->spr.scale.X < 0.5)
    {
        pActor->spr.scale.X += (REPEAT_SCALE);
		pActor->spr.scale.Y += (REPEAT_SCALE);
    }

    pActor->spr.extra += (pActor->nPhase & 0x0F) + 5;
    pActor->spr.extra &= kAngleMask;

    double nVel = mapangle(pActor->spr.extra).Cos();

    auto vect = pActor->spr.Angles.Yaw.ToVector() * nVel * 8;
	auto coll = movesprite(pActor,vect, pActor->vel.Z, 0, CLIPMASK0);
    if (coll.exbits & 0x10000)
    {
		DExhumedActor* pSet = pActor->pTarget;
		if (!pSet) return;

        pActor->spr.cstat = 0;
		pActor->spr.scale = DVector2(REPEAT_SCALE, REPEAT_SCALE);
        pActor->spr.pos = pSet->spr.pos.plusZ(-GetActorHeight(pSet) * 0.5);
        ChangeActorSect(pActor, pSet->sector());
        return;
    }
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
            pActor->vel.X = 0;
            pActor->vel.Y = 0;
            pActor->vel.Z = 0;
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AISet::Draw(RunListEvent* ev)
{
	if (const auto pActor = ev->pObjActor)
    {
        const auto setSeq = &SetSeq[pActor->nAction];
        seq_PlotSequence(ev->nParam, pActor->nSeqFile, setSeq->nSeqId, pActor->nFrame, setSeq->nFlags);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AISet::Tick(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

	int nAction = pActor->nAction;

    bool bVal = false;

    Gravity(pActor);

    const auto& setSeq = getSequence(pActor->nSeqFile, SetSeq[nAction].nSeqId);
    const auto& seqFrame = setSeq.frames[pActor->nFrame];

    pActor->spr.picnum = seqFrame.getFirstPicnum();
    playFrameSound(pActor, seqFrame);

    if (nAction == 3)
    {
        if (pActor->nIndex2) {
            pActor->nFrame++;
        }
    }

    pActor->nFrame++;
    if (pActor->nFrame >= setSeq.frames.Size())
    {
        pActor->nFrame = 0;
        bVal = true;
    }

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
    pushmove(pActor->spr.pos, &sect, pActor->clipdist, 20, -20, CLIPMASK0);
    pActor->setsector(sect);

    if (pActor->vel.Z > 4000/256.)
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

                pActor->VelFromAngle(-1);
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

            pActor->vel.X = 0;
            pActor->vel.Y = 0;

            pActor->pTarget = FindPlayer(pActor, 1000);
        }
        return;
    }

    case 3:
    {
        if (pTarget != nullptr)
        {
            if ((seqFrame.flags & 0x10) && (nMov.exbits & kHitAux2))
            {
                SetQuake(pActor, 100);
            }

            double nCourse = PlotCourseToSprite(pActor, pTarget);

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
                    pActor->vel.X = 0;
                    pActor->vel.Y = 0;
                    return;
                }
                case 1:
                {
                    PlotCourseToSprite(pActor, pTarget);

                    pActor->nAction = 6;
                    pActor->nFrame = 0;
                    pActor->nRun = 5;
                    pActor->vel.X = 0;
                    pActor->vel.Y = 0;
                    return;
                }
                default:
                {
                    if (nCourse <= 100/16.)
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
			pActor->vel.XY() = pActor->spr.Angles.Yaw.ToVector() * 512;

            if (pActor->nIndex2)
            {
                pActor->vel.X *= 2;
                pActor->vel.Y *= 2;
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
                            pActor->vel.X = 0;
                            pActor->vel.Y = 0;
                            return;
                        }
                    }
                }

                pActor->spr.Angles.Yaw += DAngle45;
                pActor->VelFromAngle(-1);
                break;
            }
            else if (nMov.type == kHitSprite)
            {
                if (pTarget == nMov.actor())
                {
                    auto nAngDiff = absangle(pActor->spr.Angles.Yaw, (pTarget->spr.pos - pActor->spr.pos).Angle());
                    if (nAngDiff < DAngle22_5 / 2)
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
                    pActor->vel.X = 0;
                    pActor->vel.Y = 0;
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
            if (PlotCourseToSprite(pActor, pTarget) >= 48)
            {
                pActor->nAction = 3;
            }
            else if (seqFrame.flags & 0x80)
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
        if (seqFrame.flags & 0x80)
        {
            auto pBullet = BuildBullet(pActor, 11, INT_MAX, pActor->spr.Angles.Yaw, pTarget, 1);
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
                pActor->vel.Z = -10000 / 256.;
            }
            else
            {
                pActor->vel.Z = -(PlotCourseToSprite(pActor, pTarget)) / 16.;
            }

            pActor->nAction = 8;
            pActor->nFrame = 0;

            pActor->VelFromAngle();
        }
        return;
    }

    case 8:
    {
        if (bVal)
        {
            pActor->nFrame = setSeq.frames.Size() - 1;
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
        pActor->vel.X *= 0.5;
        pActor->vel.Y *= 0.5;

        if (bVal)
        {
            pActor->vel.X = 0;
            pActor->vel.Y = 0;

            PlotCourseToSprite(pActor, pTarget);

            pActor->nAction = 6;
            pActor->nFrame = 0;
            pActor->nRun = 5;

            pActor->vel.X = 0;
            pActor->vel.Y = 0;
        }
        return;
    }

    case 10:
    {
        if (seqFrame.flags & 0x80)
        {
            pActor->spr.pos.Z -= GetActorHeight(pActor);
            BuildCreatureChunk(pActor, getSequence("set", 76).getFirstPicnum());
			pActor->spr.pos.Z += GetActorHeight(pActor);
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
                pActor->vel.X = 0;
                pActor->vel.Y = 0;
            }
        }
    }

    return;
}

END_PS_NS
