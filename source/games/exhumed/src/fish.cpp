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

	pChunkActor->spr.pos = pActor->spr.pos;
    pChunkActor->spr.cstat = 0;
    pChunkActor->spr.shade = -12;
    pChunkActor->spr.pal = 0;
    pChunkActor->set_int_xvel((RandomSize(5) - 16) << 8);
    pChunkActor->set_int_yvel((RandomSize(5) - 16) << 8);
    pChunkActor->spr.xrepeat = 64;
    pChunkActor->spr.yrepeat = 64;
    pChunkActor->spr.xoffset = 0;
    pChunkActor->spr.yoffset = 0;
    pChunkActor->set_int_zvel((-(RandomByte() + 512)) * 2);

    seq_GetSeqPicnum(kSeqFish, pChunkActor->nCount, 0);

    pChunkActor->spr.picnum = anim;
    pChunkActor->spr.lotag = runlist_HeadRun() + 1;
    pChunkActor->spr.clipdist = 0;

//	GrabTimeSlot(3);

    pChunkActor->spr.extra = -1;
    pChunkActor->spr.intowner = runlist_AddRunRec(pChunkActor->spr.lotag - 1, pChunkActor, 0x200000);
    pChunkActor->spr.hitag = runlist_AddRunRec(NewRun, pChunkActor, 0x200000);
}

void BuildBlood(const DVector3& pos, sectortype* pSector)
{
    BuildAnim(nullptr, kSeqFish, 36, pos, pSector, 75, 128);
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
            BuildBlood(pActor->spr.pos, pActor->sector());
        }
    }

    double FloorZ = pActor->sector()->floorz;

    if (FloorZ <= pActor->spr.pos.Z)
    {
		pActor->spr.pos.Z++;

        if ((pActor->spr.pos.Z - FloorZ) > 100)
        {
            pActor->vel.Z = 0;
            runlist_DoSubRunRec(pActor->spr.intowner);
            runlist_FreeRun(pActor->spr.lotag - 1);
            runlist_SubRunRec(pActor->spr.hitag);
            DeleteActor(pActor);
        }
        else if ((pActor->spr.pos.Z - FloorZ) > 0)
        {
            pActor->vel.Z = 4;
        }
    }
    else
    {
        auto coll = movesprite(pActor, pActor->int_xvel() << 8, pActor->int_yvel() << 8, pActor->int_zvel(), 2560, -2560, CLIPMASK1);
        if (coll.type != kHitNone)
        {
            pActor->vel.X = 0;
            pActor->vel.Y = 0;
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


void BuildFish(DExhumedActor* pActor, const DVector3& pos, sectortype* pSector, DAngle nAngle)
{
    if (pActor == nullptr)
    {
        pActor = insertActor(pSector, 103);
		pActor->spr.pos = pos;
    }
    else
    {
        nAngle = pActor->spr.angle;
        ChangeActorStat(pActor, 103);
    }

    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pActor->spr.shade = -12;
    pActor->spr.clipdist = 80;
    pActor->spr.xrepeat = 40;
    pActor->spr.yrepeat = 40;
    pActor->spr.pal = pActor->sector()->ceilingpal;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.picnum = seq_GetSeqPicnum(kSeqFish, FishSeq[0].a, 0);
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
    pActor->spr.angle = nAngle;
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
    pActor->add_int_ang((256 - RandomSize(9)) + 1024);
    pActor->norm_ang();

    pActor->VelFromAngle(-8);

    pActor->nAction = 0;
    pActor->nFrame = 0;

    pActor->set_int_zvel(RandomSize(9));

    if (!edx)
    {
        if (RandomBit()) {
            pActor->vel.Z = -pActor->vel.Z;
        }
    }
    else if (edx < 0)
    {
        pActor->vel.Z = -pActor->vel.Z;
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

            PlayFXAtXYZ(StaticSound[kSound40], pActor->spr.pos);
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

                int nAngle = getangle(pTargetActor->spr.pos - pActor->spr.pos);
                pActor->set_int_zvel(bsin(nAngle, -5));

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
            double nHeight = GetActorHeightF(pActor) * 0.5;
            double z = fabs(pTargetActor->spr.pos.Z - pActor->spr.pos.Z);

            if (z <= nHeight)
            {
                pActor->set_int_xvel(bcos(pActor->int_ang(), -5) - bcos(pActor->int_ang(), -7));
                pActor->set_int_yvel(bsin(pActor->int_ang(), -5) - bsin(pActor->int_ang(), -7));
            }
            else
            {
                pActor->vel.X = 0;
                pActor->vel.Y = 0;
            }

            pActor->set_int_zvel(int((pTargetActor->spr.pos.Z - pActor->spr.pos.Z) * zworldtoint / 8));
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

	auto pos = pActor->spr.pos;
    auto pSector =pActor->sector();

    // loc_2EF54
    Collision coll = movesprite(pActor, pActor->int_xvel() << 13, pActor->int_yvel() << 13, pActor->int_zvel() << 2, 0, 0, CLIPMASK0);

    if (!(pActor->sector()->Flag & kSectUnderwater))
    {
        ChangeActorSect(pActor, pSector);
        pActor->spr.pos = pos;

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
                    pActor->spr.angle = VecToAngle(pHitAct->spr.pos - pActor->spr.pos);

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
