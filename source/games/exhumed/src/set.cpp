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

void BuildSet(DExhumedActor* pActor, int x, int y, int z, int nSector, int nAngle, int nChannel)
{
	spritetype* pSprite;
    if (pActor == nullptr)
    {
        pActor = insertActor(nSector, 120);
        pSprite = &pActor->s();
    }
    else
    {
        ChangeActorStat(pActor, 120);
		pSprite = &pActor->s();
        x = pSprite->x;
        y = pSprite->y;
        z = pSprite->sector()->floorz;
        nAngle = pSprite->ang;
    }

    pSprite->x = x;
    pSprite->y = y;
    pSprite->z = z;
    pSprite->cstat = 0x101;
    pSprite->shade = -12;
    pSprite->clipdist = 110;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->xrepeat = 87;
    pSprite->yrepeat = 96;
    pSprite->pal = pSprite->sector()->ceilingpal;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->ang = nAngle;
    pSprite->picnum = 1;
    pSprite->hitag = 0;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->extra = -1;

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

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, pActor, 0x190000);

    // this isn't stored anywhere.
    runlist_AddRunRec(NewRun, pActor, 0x190000);

    nCreaturesTotal++;
}

void BuildSoul(DExhumedActor* pSet)
{
    auto pSetSprite = &pSet->s();
    auto pActor = insertActor(pSetSprite->sectnum, 0);
    auto pSprite = &pActor->s();

    pSprite->cstat = 0x8000;
    pSprite->shade = -127;
    pSprite->xrepeat = 1;
    pSprite->yrepeat = 1;
    pSprite->pal = 0;
    pSprite->clipdist = 5;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->picnum = seq_GetSeqPicnum(kSeqSet, 75, 0);
    pSprite->ang = RandomSize(11);
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = (-256) - RandomSize(10);
    pSprite->x = pSetSprite->x;
    pSprite->y = pSetSprite->y;

    int nSector =pSprite->sectnum;
    pSprite->z = (RandomSize(8) << 8) + 8192 + sector[nSector].ceilingz - GetActorHeight(pActor);

    //pSprite->hitag = nSet;
	pActor->pTarget = pSet;
	pActor->nPhase = Counters[kCountSoul]++;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->extra = 0;

    //	GrabTimeSlot(3);

    pSprite->owner = runlist_AddRunRec(NewRun, pActor, 0x230000);
}

void AISoul::Tick(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

    auto pSprite = &pActor->s();

    seq_MoveSequence(pActor, SeqOffsets[kSeqSet] + 75, 0);

    if (pSprite->xrepeat < 32)
    {
        pSprite->xrepeat++;
        pSprite->yrepeat++;
    }

    pSprite->extra += (pActor->nPhase & 0x0F) + 5;
    pSprite->extra &= kAngleMask;

    int nVel = bcos(pSprite->extra, -7);

	auto coll = movesprite(pActor, bcos(pSprite->ang) * nVel, bsin(pSprite->ang) * nVel, pSprite->zvel, 5120, 0, CLIPMASK0);
    if (coll.exbits & 0x10000)
    {
		auto pSet = pActor->pTarget;
		if (!pSet) return;
        auto pSetSprite = &pSet->s();

        pSprite->cstat = 0;
        pSprite->yrepeat = 1;
        pSprite->xrepeat = 1;
        pSprite->x = pSetSprite->x;
        pSprite->y = pSetSprite->y;
        pSprite->z = pSetSprite->z - (GetActorHeight(pSet) >> 1);
        ChangeActorSect(pActor, pSetSprite->sectnum);
        return;
    }
}


void AISet::RadialDamage(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;
    short nAction = pActor->nAction;

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

	short nAction = pActor->nAction;
    auto pSprite = &pActor->s();

    if (ev->nDamage && pActor->nHealth > 0)
    {
        if (nAction != 1)
        {
            pActor->nHealth -= dmgAdjust(ev->nDamage);
        }

        if (pActor->nHealth <= 0)
        {
            pSprite->xvel = 0;
            pSprite->yvel = 0;
            pSprite->zvel = 0;
            pSprite->cstat &= 0xFEFE;

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
    short nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqSet] + SetSeq[nAction].a, pActor->nFrame, SetSeq[nAction].b);
    return;
}

void AISet::Tick(RunListEvent* ev)
{
	auto pActor = ev->pObjActor;
	if (!pActor) return;

	short nAction = pActor->nAction;
    auto pSprite = &pActor->s();

    bool bVal = false;

    Gravity(pActor);

    short nSeq = SeqOffsets[kSeqSet] + SetSeq[pActor->nAction].a;
    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);
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

    short nFlag = FrameFlag[SeqBase[nSeq] + pActor->nFrame];
    auto pTarget = pActor->pTarget;

    if (pTarget && nAction < 10)
    {
        if (!(pTarget->s().cstat & 0x101))
        {
            pActor->pTarget = nullptr;
            pActor->nAction = 0;
            pActor->nFrame = 0;
            pTarget = nullptr;
        }
    }

    auto nMov = MoveCreature(pActor);

	static_assert(sizeof(pSprite->sectnum) != 4);
	int sectnum = pSprite->sectnum;
    pushmove(&pSprite->pos, &sectnum, pSprite->clipdist << 2, 5120, -5120, CLIPMASK0);
	pSprite->sectnum = sectnum;

    if (pSprite->zvel > 4000)
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

                pSprite->xvel = bcos(pSprite->ang, -1);
                pSprite->yvel = bsin(pSprite->ang, -1);
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

            pSprite->xvel = 0;
            pSprite->yvel = 0;

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
                    pSprite->xvel = 0;
                    pSprite->yvel = 0;
                    return;
                }
                case 1:
                {
                    PlotCourseToSprite(pActor, pTarget);

                    pActor->nAction = 6;
                    pActor->nFrame = 0;
                    pActor->nRun = 5;
                    pSprite->xvel = 0;
                    pSprite->yvel = 0;
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
            int nAngle = pSprite->ang & 0xFFF8;
            pSprite->xvel = bcos(nAngle, -1);
            pSprite->yvel = bsin(nAngle, -1);

            if (pActor->nIndex2)
            {
                pSprite->xvel *= 2;
                pSprite->yvel *= 2;
            }

            if (nMov.type == kHitWall)
            {
                int nWall = nMov.index;
                int nSector = wall[nWall].nextsector;

                if (nSector >= 0)
                {
                    if ((pSprite->z - sector[nSector].floorz) < 55000)
                    {
                        if (pSprite->z > sector[nSector].ceilingz)
                        {
                            pActor->nIndex = 1;
                            pActor->nAction = 7;
                            pActor->nFrame = 0;
                            pSprite->xvel = 0;
                            pSprite->yvel = 0;
                            return;
                        }
                    }
                }

                pSprite->ang = (pSprite->ang + 256) & kAngleMask;
                pSprite->xvel = bcos(pSprite->ang, -1);
                pSprite->yvel = bsin(pSprite->ang, -1);
                break;
            }
            else if (nMov.type == kHitSprite)
            {
                if (pTarget == nMov.actor)
                {
                    int nAng = getangle(pTarget->s().x - pSprite->x, pTarget->s().y - pSprite->y);
                    if (AngleDiff(pSprite->ang, nAng) < 64)
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
                    pSprite->xvel = 0;
                    pSprite->yvel = 0;
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
            auto pBullet = BuildBullet(pActor, 11, -1, pSprite->ang, pTarget, 1);
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
                pSprite->zvel = -10000;
            }
            else
            {
                pSprite->zvel = -(PlotCourseToSprite(pActor, pTarget));
            }

            pActor->nAction = 8;
            pActor->nFrame = 0;

            pSprite->xvel = bcos(pSprite->ang);
            pSprite->yvel = bsin(pSprite->ang);
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
        pSprite->xvel >>= 1;
        pSprite->yvel >>= 1;

        if (bVal)
        {
            pSprite->xvel = 0;
            pSprite->yvel = 0;

            PlotCourseToSprite(pActor, pTarget);

            pActor->nAction = 6;
            pActor->nFrame = 0;
            pActor->nRun = 5;

            pSprite->xvel = 0;
            pSprite->yvel = 0;
        }
        return;
    }

    case 10:
    {
        if (nFlag & 0x80)
        {
            pSprite->z -= GetActorHeight(pActor);
            BuildCreatureChunk(pActor, seq_GetSeqPicnum(kSeqSet, 76, 0));
            pSprite->z += GetActorHeight(pActor);
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
        pSprite->cstat &= 0xFEFE;
        return;
    }
    }

    // loc_33AE3: ?
    if (nAction)
    {
        if (pTarget)
        {
            if (!(pTarget->s().cstat & 0x101))
            {
                pActor->nAction = 0;
                pActor->nFrame = 0;
                pActor->nCount = 100;
                pActor->pTarget = nullptr;
                pSprite->xvel = 0;
                pSprite->yvel = 0;
            }
        }
    }

    return;
}

END_PS_NS
