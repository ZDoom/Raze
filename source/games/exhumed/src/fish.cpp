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
#include "sequence.h"
#include "exhumed.h"
#include "sound.h"
#include <assert.h>

BEGIN_PS_NS

static actionSeq FishSeq[] = {
    {8, 0},
    {8, 0},
    {0, 0},
    {24, 0},
    {8, 0},
    {32, 1},
    {33, 1},
    {34, 1},
    {35, 1},
    {39, 1}
};



void BuildFishLimb(DExhumedActor* pActor, short anim)
{
	auto pSprite = &pActor->s();

    auto pChunkActor = insertActor(pSprite->sectnum, 99);
	auto pSprite2 = &pChunkActor->s();

    pChunkActor->nCount = anim + 40;
    pChunkActor->nFrame = RandomSize(3) % SeqSize[SeqOffsets[kSeqFish] + anim + 40];

    pSprite2->x = pSprite->x;
    pSprite2->y = pSprite->y;
    pSprite2->z = pSprite->z;
    pSprite2->cstat = 0;
    pSprite2->shade = -12;
    pSprite2->pal = 0;
    pSprite2->xvel = (RandomSize(5) - 16) << 8;
    pSprite2->yvel = (RandomSize(5) - 16) << 8;
    pSprite2->xrepeat = 64;
    pSprite2->yrepeat = 64;
    pSprite2->xoffset = 0;
    pSprite2->yoffset = 0;
    pSprite2->zvel = (-(RandomByte() + 512)) * 2;

    seq_GetSeqPicnum(kSeqFish, pChunkActor->nCount, 0);

    pSprite2->picnum = anim;
    pSprite2->lotag = runlist_HeadRun() + 1;
    pSprite2->clipdist = 0;

//	GrabTimeSlot(3);

    pSprite2->extra = -1;
    pSprite2->owner = runlist_AddRunRec(pSprite2->lotag - 1, pChunkActor, 0x200000);
    pSprite2->hitag = runlist_AddRunRec(NewRun, pChunkActor, 0x200000);
}

void BuildBlood(int x, int y, int z, int nSector)
{
    BuildAnim(nullptr, kSeqFish, 36, x, y, z, nSector, 75, 128);
}

void AIFishLimb::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    auto pSprite = &pActor->s();

    int nSeq = SeqOffsets[kSeqFish] + pActor->nCount;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

    Gravity(pActor);

    pActor->nFrame++;

    if (pActor->nFrame >= SeqSize[nSeq])
    {
        pActor->nFrame = 0;
        if (RandomBit()) {
            BuildBlood(pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum);
        }
    }

    int FloorZ = pSprite->sector()->floorz;

    if (FloorZ <= pSprite->z)
    {
        pSprite->z += 256;

        if ((pSprite->z - FloorZ) > 25600)
        {
            pSprite->zvel = 0;
            runlist_DoSubRunRec(pSprite->owner);
            runlist_FreeRun(pSprite->lotag - 1);
            runlist_SubRunRec(pSprite->hitag);
            DeleteActor(pActor);
        }
        else if ((pSprite->z - FloorZ) > 0)
        {
            pSprite->zvel = 1024;
        }
    }
    else
    {
        auto coll = movesprite(pActor, pSprite->xvel << 8, pSprite->yvel << 8, pSprite->zvel, 2560, -2560, CLIPMASK1);
        if (coll.type != kHitNone)
        {
            pSprite->xvel = 0;
            pSprite->yvel = 0;
        }
    }

}

void AIFishLimb::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (pActor == nullptr) return;
    int nSeq = SeqOffsets[kSeqFish] + pActor->nCount;
    seq_PlotSequence(ev->nParam, nSeq, pActor->nFrame, 1);
}


void BuildFish(DExhumedActor* pActor, int x, int y, int z, int nSector, int nAngle)
{
	spritetype* pSprite;

    if (pActor == nullptr)
    {
        pActor = insertActor(nSector, 103);
        pSprite = &pActor->s();
    }
    else
    {
        pSprite = &pActor->s();
        x = pSprite->x;
        y = pSprite->y;
        z = pSprite->z;
        nAngle = pSprite->ang;
        ChangeActorStat(pActor, 103);
    }

    pSprite->x = x;
    pSprite->y = y;
    pSprite->z = z;
    pSprite->cstat = 0x101;
    pSprite->shade = -12;
    pSprite->clipdist = 80;
    pSprite->xrepeat = 40;
    pSprite->yrepeat = 40;
    pSprite->pal = pSprite->sector()->ceilingpal;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->picnum = seq_GetSeqPicnum(kSeqFish, FishSeq[0].a, 0);
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->ang = nAngle;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->hitag = 0;
    pSprite->extra = -1;

//	GrabTimeSlot(3);

    pActor->nAction = 0;
    pActor->nHealth = 200;
    pActor->pTarget = nullptr;
    pActor->nCount = 60;
    pActor->nFrame = 0;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, pActor, 0x120000);
    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x120000);

    nCreaturesTotal++;
}

void IdleFish(DExhumedActor* pActor, short edx)
{
    auto pSprite = &pActor->s();

    pSprite->ang += (256 - RandomSize(9)) + 1024;
    pSprite->ang &= kAngleMask;

    pSprite->xvel = bcos(pSprite->ang, -8);
    pSprite->yvel = bsin(pSprite->ang, -8);

    pActor->nAction = 0;
    pActor->nFrame = 0;

    pSprite->zvel = RandomSize(9);

    if (!edx)
    {
        if (RandomBit()) {
            pSprite->zvel = -pSprite->zvel;
        }
    }
    else if (edx < 0)
    {
        pSprite->zvel = -pSprite->zvel;
    }
}

void DestroyFish(DExhumedActor* pActor)
{
    auto pSprite = &pActor->s();

    runlist_DoSubRunRec(pSprite->owner);
    runlist_FreeRun(pSprite->lotag - 1);
    runlist_SubRunRec(pActor->nRun);
    DeleteActor(pActor);
}



void AIFish::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (pActor == nullptr) return;
    short nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqFish] + FishSeq[nAction].a, pActor->nFrame, FishSeq[nAction].b);
    ev->pTSprite->owner = -1;
    return;
}

void AIFish::RadialDamage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (pActor == nullptr) return;

    if (pActor->nHealth <= 0) {
        return;
    }
    else
    {
        ev->nDamage = runlist_CheckRadialDamage(pActor);
        if (!ev->nDamage) {
            return;
        }

        pActor->nCount = 10;
    }
    // fall through
    Damage(ev);
}

void AIFish::Damage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;

    if (!ev->nDamage || !pActor) {
        return;
    }
    auto pSprite = &pActor->s();
    short nAction = pActor->nAction;

    pActor->nHealth -= dmgAdjust(ev->nDamage);
    if (pActor->nHealth <= 0)
    {
        pActor->nHealth = 0;
        nCreaturesKilled++;

        pSprite->cstat &= 0xFEFE;

        if (!ev->isRadialEvent())
        {
            for (int i = 0; i < 3; i++)
            {
                BuildFishLimb(pActor, i);
            }

            PlayFXAtXYZ(StaticSound[kSound40], pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum);
            DestroyFish(pActor);
        }
        else
        {
            pActor->nAction = 9;
            pActor->nFrame = 0;
        }

        return;
    }
    else
    {
        auto pTarget = ev->pOtherActor;
        if (pTarget && pTarget->s().statnum < 199)
        {
            pActor->pTarget = pTarget;
        }

        pActor->nAction = 4;
        pActor->nFrame = 0;
        pActor->nCount += 10;
    }
}

void AIFish::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (pActor == nullptr) return;
    auto pSprite = &pActor->s();

    short nAction = pActor->nAction;

    if (!(SectFlag[pSprite->sectnum] & kSectUnderwater))
    {
        Gravity(pActor);
    }

    short nSeq = SeqOffsets[kSeqFish] + FishSeq[nAction].a;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

    seq_MoveSequence(pActor, nSeq, pActor->nFrame);

    pActor->nFrame++;
    if (pActor->nFrame >= SeqSize[nSeq]) {
        pActor->nFrame = 0;
    }

    auto pTargetActor = pActor->pTarget;

    switch (nAction)
    {
    default:
        return;

    case 0:
    {
        pActor->nCount--;
        if (pActor->nCount <= 0)
        {
            pTargetActor = FindPlayer(pActor, 60);
            if (pTargetActor)
            {
                pActor->pTarget = pTargetActor;
                pActor->nAction = 2;
                pActor->nFrame = 0;

                int nAngle = GetMyAngle(pTargetActor->s().x - pSprite->x, pTargetActor->s().z - pSprite->z);
                pSprite->zvel = bsin(nAngle, -5);

                pActor->nCount = RandomSize(6) + 90;
            }
            else
            {
                IdleFish(pActor, 0);
            }
        }

        break;
    }

    case 1:
        return;

    case 2:
    case 3:
    {
        pActor->nCount--;
        if (pActor->nCount <= 0)
        {
            IdleFish(pActor, 0);
            return;
        }
        else
        {
            PlotCourseToSprite(pActor, pTargetActor);
            int nHeight = GetActorHeight(pActor) >> 1;

            int z = abs(pTargetActor->s().z - pSprite->z);

            if (z <= nHeight)
            {
                pSprite->xvel = bcos(pSprite->ang, -5) - bcos(pSprite->ang, -7);
                pSprite->yvel = bsin(pSprite->ang, -5) - bsin(pSprite->ang, -7);
            }
            else
            {
                pSprite->xvel = 0;
                pSprite->yvel = 0;
            }

            pSprite->zvel = (pTargetActor->s().z - pSprite->z) >> 3;
        }
        break;
    }

    case 4:
    {
        if (pActor->nFrame == 0)
        {
            IdleFish(pActor, 0);
        }
        return;
    }

    case 8:
    {
        return;
    }

    case 9:
    {
        if (pActor->nFrame == 0)
        {
            DestroyFish(pActor);
        }
        return;
    }
    }

    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;
    int nSector =pSprite->sectnum;

    // loc_2EF54
    Collision coll = movesprite(pActor, pSprite->xvel << 13, pSprite->yvel << 13, pSprite->zvel << 2, 0, 0, CLIPMASK0);

    if (!(SectFlag[pSprite->sectnum] & kSectUnderwater))
    {
        ChangeActorSect(pActor, nSector);
        pSprite->x = x;
        pSprite->y = y;
        pSprite->z = z;

        IdleFish(pActor, 0);
        return;
    }
    else
    {
        if (nAction >= 5) {
            return;
        }

        if (coll.type == kHitNone)
        {
            if (nAction == 3)
            {
                pActor->nAction = 2;
                pActor->nFrame = 0;
            }
            return;
        }

        if (!coll.exbits)
        {
            if (coll.type == kHitWall)
            {
                IdleFish(pActor, 0);
            }
            else if (coll.type == kHitSprite)
            {

                auto pHitSpr = &coll.actor->s();
                if (pHitSpr->statnum == 100)
                {
                    pActor->pTarget = coll.actor;
                    pSprite->ang = GetMyAngle(pHitSpr->x - pSprite->x, pHitSpr->y - pSprite->y);

                    if (nAction != 3)
                    {
                        pActor->nAction = 3;
                        pActor->nFrame = 0;
                    }

                    if (!pActor->nFrame)
                    {
                        runlist_DamageEnemy(coll.actor, pActor, 2);
                    }
                }
            }
        }
        else if (coll.exbits & kHitAux2)
        {
            IdleFish(pActor, -1);
        }
        else
        {
            IdleFish(pActor, 1);
        }
    }
}

END_PS_NS
