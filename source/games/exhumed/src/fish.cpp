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



void BuildFishLimb(DExhumedActor* pActor, int anim)
{
    auto pChunkActor = insertActor(pActor->sector(), 99);

    pChunkActor->nCount = anim + 40;
    pChunkActor->nFrame = RandomSize(3) % SeqSize[SeqOffsets[kSeqFish] + anim + 40];

    pChunkActor->set_int_pos(pActor->int_pos());
    pChunkActor->spr.cstat = 0;
    pChunkActor->spr.shade = -12;
    pChunkActor->spr.pal = 0;
    pChunkActor->spr.xvel = (RandomSize(5) - 16) << 8;
    pChunkActor->spr.yvel = (RandomSize(5) - 16) << 8;
    pChunkActor->spr.xrepeat = 64;
    pChunkActor->spr.yrepeat = 64;
    pChunkActor->spr.xoffset = 0;
    pChunkActor->spr.yoffset = 0;
    pChunkActor->spr.zvel = (-(RandomByte() + 512)) * 2;

    seq_GetSeqPicnum(kSeqFish, pChunkActor->nCount, 0);

    pChunkActor->spr.picnum = anim;
    pChunkActor->spr.lotag = runlist_HeadRun() + 1;
    pChunkActor->spr.clipdist = 0;

//	GrabTimeSlot(3);

    pChunkActor->spr.extra = -1;
    pChunkActor->spr.intowner = runlist_AddRunRec(pChunkActor->spr.lotag - 1, pChunkActor, 0x200000);
    pChunkActor->spr.hitag = runlist_AddRunRec(NewRun, pChunkActor, 0x200000);
}

void BuildBlood(int x, int y, int z, sectortype* pSector)
{
    BuildAnim(nullptr, kSeqFish, 36, x, y, z, pSector, 75, 128);
}

void AIFishLimb::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    int nSeq = SeqOffsets[kSeqFish] + pActor->nCount;

    pActor->spr.picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

    Gravity(pActor);

    pActor->nFrame++;

    if (pActor->nFrame >= SeqSize[nSeq])
    {
        pActor->nFrame = 0;
        if (RandomBit()) {
            BuildBlood(pActor->int_pos().X, pActor->int_pos().Y, pActor->int_pos().Z, pActor->sector());
        }
    }

    int FloorZ = pActor->sector()->int_floorz();

    if (FloorZ <= pActor->int_pos().Z)
    {
        pActor->add_int_z(256);

        if ((pActor->int_pos().Z - FloorZ) > 25600)
        {
            pActor->spr.zvel = 0;
            runlist_DoSubRunRec(pActor->spr.intowner);
            runlist_FreeRun(pActor->spr.lotag - 1);
            runlist_SubRunRec(pActor->spr.hitag);
            DeleteActor(pActor);
        }
        else if ((pActor->int_pos().Z - FloorZ) > 0)
        {
            pActor->spr.zvel = 1024;
        }
    }
    else
    {
        auto coll = movesprite(pActor, pActor->spr.xvel << 8, pActor->spr.yvel << 8, pActor->spr.zvel, 2560, -2560, CLIPMASK1);
        if (coll.type != kHitNone)
        {
            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;
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


void BuildFish(DExhumedActor* pActor, int x, int y, int z, sectortype* pSector, int nAngle)
{
    if (pActor == nullptr)
    {
        pActor = insertActor(pSector, 103);
    }
    else
    {
        x = pActor->int_pos().X;
        y = pActor->int_pos().Y;
        z = pActor->int_pos().Z;
        nAngle = pActor->spr.ang;
        ChangeActorStat(pActor, 103);
    }

    pActor->set_int_pos({ x, y, z });
    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pActor->spr.shade = -12;
    pActor->spr.clipdist = 80;
    pActor->spr.xrepeat = 40;
    pActor->spr.yrepeat = 40;
    pActor->spr.pal = pActor->sector()->ceilingpal;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.picnum = seq_GetSeqPicnum(kSeqFish, FishSeq[0].a, 0);
    pActor->spr.xvel = 0;
    pActor->spr.yvel = 0;
    pActor->spr.zvel = 0;
    pActor->spr.ang = nAngle;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.hitag = 0;
    pActor->spr.extra = -1;

//	GrabTimeSlot(3);

    pActor->nAction = 0;
    pActor->nHealth = 200;
    pActor->pTarget = nullptr;
    pActor->nCount = 60;
    pActor->nFrame = 0;

    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x120000);
    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x120000);

    nCreaturesTotal++;
}

void IdleFish(DExhumedActor* pActor, int edx)
{
    pActor->spr.ang += (256 - RandomSize(9)) + 1024;
    pActor->spr.ang &= kAngleMask;

    pActor->spr.xvel = bcos(pActor->spr.ang, -8);
    pActor->spr.yvel = bsin(pActor->spr.ang, -8);

    pActor->nAction = 0;
    pActor->nFrame = 0;

    pActor->spr.zvel = RandomSize(9);

    if (!edx)
    {
        if (RandomBit()) {
            pActor->spr.zvel = -pActor->spr.zvel;
        }
    }
    else if (edx < 0)
    {
        pActor->spr.zvel = -pActor->spr.zvel;
    }
}

void DestroyFish(DExhumedActor* pActor)
{
    runlist_DoSubRunRec(pActor->spr.intowner);
    runlist_FreeRun(pActor->spr.lotag - 1);
    runlist_SubRunRec(pActor->nRun);
    DeleteActor(pActor);
}



void AIFish::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (pActor == nullptr) return;
    int nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqFish] + FishSeq[nAction].a, pActor->nFrame, FishSeq[nAction].b);
    ev->pTSprite->ownerActor = nullptr;
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

    pActor->nHealth -= dmgAdjust(ev->nDamage);
    if (pActor->nHealth <= 0)
    {
        pActor->nHealth = 0;
        nCreaturesKilled++;

        pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;

        if (!ev->isRadialEvent())
        {
            for (int i = 0; i < 3; i++)
            {
                BuildFishLimb(pActor, i);
            }

            PlayFXAtXYZ(StaticSound[kSound40], pActor->int_pos().X, pActor->int_pos().Y, pActor->int_pos().Z);
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
        if (pTarget && pTarget->spr.statnum < 199)
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

    int nAction = pActor->nAction;

    if (!(pActor->sector()->Flag & kSectUnderwater))
    {
        Gravity(pActor);
    }

    int nSeq = SeqOffsets[kSeqFish] + FishSeq[nAction].a;

    pActor->spr.picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

    seq_MoveSequence(pActor, nSeq, pActor->nFrame);

    pActor->nFrame++;
    if (pActor->nFrame >= SeqSize[nSeq]) {
        pActor->nFrame = 0;
    }

    DExhumedActor* pTargetActor = pActor->pTarget;

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

                int nAngle = GetMyAngle(pTargetActor->int_pos().X - pActor->int_pos().X, pTargetActor->int_pos().Z - pActor->int_pos().Z);
                pActor->spr.zvel = bsin(nAngle, -5);

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

            int z = abs(pTargetActor->int_pos().Z - pActor->int_pos().Z);

            if (z <= nHeight)
            {
                pActor->spr.xvel = bcos(pActor->spr.ang, -5) - bcos(pActor->spr.ang, -7);
                pActor->spr.yvel = bsin(pActor->spr.ang, -5) - bsin(pActor->spr.ang, -7);
            }
            else
            {
                pActor->spr.xvel = 0;
                pActor->spr.yvel = 0;
            }

            pActor->spr.zvel = (pTargetActor->int_pos().Z - pActor->int_pos().Z) >> 3;
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

    int x = pActor->int_pos().X;
    int y = pActor->int_pos().Y;
    int z = pActor->int_pos().Z;
    auto pSector =pActor->sector();

    // loc_2EF54
    Collision coll = movesprite(pActor, pActor->spr.xvel << 13, pActor->spr.yvel << 13, pActor->spr.zvel << 2, 0, 0, CLIPMASK0);

    if (!(pActor->sector()->Flag & kSectUnderwater))
    {
        ChangeActorSect(pActor, pSector);
        pActor->set_int_pos({ x, y, z });

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

                auto pHitAct = coll.actor();
                if (pHitAct->spr.statnum == 100)
                {
                    pActor->pTarget = coll.actor();
                    pActor->spr.ang = GetMyAngle(pHitAct->int_pos().X - pActor->int_pos().X, pHitAct->int_pos().Y - pActor->int_pos().Y);

                    if (nAction != 3)
                    {
                        pActor->nAction = 3;
                        pActor->nFrame = 0;
                    }

                    if (!pActor->nFrame)
                    {
                        runlist_DamageEnemy(coll.actor(), pActor, 2);
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
