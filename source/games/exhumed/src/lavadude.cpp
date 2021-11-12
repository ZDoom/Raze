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
    auto pSprite = &pActor->s();
    int nSector =pSprite->sectnum;

    auto pLimbActor = insertActor(nSector, 118);
	auto pLimbSprite = &pLimbActor->s();

    pLimbSprite->x = pSprite->x;
    pLimbSprite->y = pSprite->y;
    pLimbSprite->z = pSprite->z - RandomLong() % ebx;
    pLimbSprite->cstat = 0;
    pLimbSprite->shade = -127;
    pLimbSprite->pal = 1;
    pLimbSprite->xvel = (RandomSize(5) - 16) << 8;
    pLimbSprite->yvel = (RandomSize(5) - 16) << 8;
    pLimbSprite->zvel = 2560 - (RandomSize(5) << 8);
    pLimbSprite->xoffset = 0;
    pLimbSprite->yoffset = 0;
    pLimbSprite->xrepeat = 90;
    pLimbSprite->yrepeat = 90;
    pLimbSprite->picnum = (move & 3) % 3;
    pLimbSprite->hitag = 0;
    pLimbSprite->lotag = runlist_HeadRun() + 1;
    pLimbSprite->clipdist = 0;

//	GrabTimeSlot(3);

    pLimbSprite->extra = -1;
    pLimbSprite->owner = runlist_AddRunRec(pLimbSprite->lotag - 1, pLimbActor, 0x160000);
    pLimbSprite->hitag = runlist_AddRunRec(NewRun, pLimbActor, 0x160000);

    return pLimbActor;
}

void AILavaDudeLimb::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    auto pSprite = &pActor->s();

    pSprite->shade += 3;

    auto coll = movesprite(pActor, pSprite->xvel << 12, pSprite->yvel << 12, pSprite->zvel, 2560, -2560, CLIPMASK1);

    if (coll.type || pSprite->shade > 100)
    {
        pSprite->xvel = 0;
        pSprite->yvel = 0;
        pSprite->zvel = 0;

        runlist_DoSubRunRec(pSprite->owner);
        runlist_FreeRun(pSprite->lotag - 1);
        runlist_SubRunRec(pSprite->hitag);

        DeleteActor(pActor);
    }
}

void AILavaDudeLimb::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    seq_PlotSequence(ev->nParam, (SeqOffsets[kSeqLavag] + 30) + pActor->s().picnum, 0, 1);
}


void BuildLava(DExhumedActor* pActor, int x, int y, int, int nSector, short nAngle, int nChannel)
{
    spritetype* pSprite;
    if (pActor == nullptr)
    {
        pActor = insertActor(nSector, 118);
        pSprite = &pActor->s();
    }
    else
    {
        pSprite = &pActor->s();
        nSector = pSprite->sectnum;
        nAngle = pSprite->ang;
        x = pSprite->x;
        y = pSprite->y;

        ChangeActorStat(pActor, 118);
    }

    pSprite->x = x;
    pSprite->y = y;
    pSprite->z = sector[nSector].floorz;
    pSprite->cstat = 0x8000;
    pSprite->xrepeat = 200;
    pSprite->yrepeat = 200;
    pSprite->shade = -12;
    pSprite->pal = 0;
    pSprite->clipdist = 127;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->picnum = seq_GetSeqPicnum(kSeqLavag, LavadudeSeq[3].a, 0);
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->ang = nAngle;
    pSprite->hitag = 0;
    pSprite->lotag = runlist_HeadRun() + 1;

//	GrabTimeSlot(3);

    pSprite->extra = -1;

    pActor->nAction = 0;
    pActor->nHealth = 4000;
    pActor->pTarget = nullptr;
    pActor->nCount = nChannel;
    pActor->nFrame = 0;
    pActor->nPhase = Counters[kCountLava]++;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, pActor, 0x150000);
    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x150000);

    nCreaturesTotal++;
}

void AILavaDude::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    short nAction = pActor->nAction;
    short nSeq = LavadudeSeq[nAction].a + SeqOffsets[kSeqLavag];

    seq_PlotSequence(ev->nParam, nSeq, pActor->nFrame, LavadudeSeq[nAction].b);
    ev->pTSprite->owner = -1;
    return;
}

void AILavaDude::Damage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    short nAction = pActor->nAction;
    auto pSprite = &pActor->s();

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

        pSprite->cstat &= 0xFEFE;
    }
    else
    {
        auto pTarget = ev->pOtherActor;

        if (pTarget)
        {
            if (pTarget->s().statnum < 199)
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
                pSprite->cstat = 0;
            }
        }

        BuildLavaLimb(pActor, totalmoves, 64000);
    }
}

void AILavaDude::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    short nAction = pActor->nAction;
    short nSeq = LavadudeSeq[nAction].a + SeqOffsets[kSeqLavag];

    auto pSprite = &pActor->s();

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);
    int var_38 = pActor->nFrame;

    short nFlag = FrameFlag[SeqBase[nSeq] + var_38];

    int var_1C;

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

    auto pTarget = pActor->pTarget;

    if (pTarget && nAction < 4)
    {
        if (!(pTarget->s().cstat & 0x101) || pTarget->s().sectnum >= 1024)
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

            pSprite->xvel = bcos(pSprite->ang);
            pSprite->yvel = bsin(pSprite->ang);

            if (pTarget && !RandomSize(1))
            {
                pActor->pTarget = pTarget;
                pActor->nAction = 2;
                pSprite->cstat = 0x101;
                pActor->nFrame = 0;
                break;
            }
        }

        int x = pSprite->x;
        int y = pSprite->y;
        int z = pSprite->z;
        int nSector =pSprite->sectnum;

        auto coll = movesprite(pActor, pSprite->xvel << 8, pSprite->yvel << 8, 0, 0, 0, CLIPMASK0);

        if (nSector != pSprite->sectnum)
        {
            ChangeActorSect(pActor, nSector);
            pSprite->x = x;
            pSprite->y = y;
            pSprite->z = z;

            pSprite->ang = (pSprite->ang + ((RandomWord() & 0x3FF) + 1024)) & kAngleMask;
            pSprite->xvel = bcos(pSprite->ang);
            pSprite->yvel = bsin(pSprite->ang);
            break;
        }

        if (coll.type == kHitNone) {
            break;
        }

        if (coll.type == kHitWall)
        {
            pSprite->ang = (pSprite->ang + ((RandomWord() & 0x3FF) + 1024)) & kAngleMask;
            pSprite->xvel = bcos(pSprite->ang);
            pSprite->yvel = bsin(pSprite->ang);
            break;
        }
        else if (coll.type == kHitSprite)
        {
            if (coll.actor == pTarget)
            {
                int nAng = getangle(pTarget->s().x - pSprite->x, pTarget->s().y - pSprite->y);
                if (AngleDiff(pSprite->ang, nAng) < 64)
                {
                    pActor->nAction = 2;
                    pActor->nFrame = 0;
                    pSprite->cstat = 0x101;
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

            pSprite->cstat |= 0x101;
        }

        break;
    }

    case 3:
    {
        if ((nFlag & 0x80) && pTarget)
        {
            int nHeight = GetActorHeight(pActor);
            GetUpAngle(pActor, -64000, pTarget, (-(nHeight >> 1)));

            BuildBullet(pActor, 10, -1, pSprite->ang, pTarget, 1);
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
            pSprite->cstat &= 0xFEFE;
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

            runlist_DoSubRunRec(pSprite->owner);
            runlist_FreeRun(pSprite->lotag - 1);
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
            pSprite->cstat = 0x8000;
        }
        break;
    }
    }

    // loc_31521:
    pSprite->pal = 1;
}

END_PS_NS
