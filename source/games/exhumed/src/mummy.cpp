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
#include "sequence.h"
#include "sound.h"
#include "exhumed.h"
#include <assert.h>
#include "engine.h"

BEGIN_PS_NS

static actionSeq MummySeq[] = {
    {8, 0},
    {0, 0},
    {16, 0},
    {24, 0},
    {32, 1},
    {40, 1},
    {48, 1},
    {50, 0}
};


void BuildMummy(DExhumedActor* pActor, int x, int y, int z, sectortype* pSector, int nAngle)
{
    if (pActor == nullptr)
    {
        pActor = insertActor(pSector, 102);
    }
    else
    {
        x = pActor->int_pos().X;
        y = pActor->int_pos().Y;
        z = pActor->int_pos().Z;
        nAngle = pActor->spr.ang;

        ChangeActorStat(pActor, 102);
    }

    pActor->set_int_pos({ x, y, z });
    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pActor->spr.shade = -12;
    pActor->spr.clipdist = 32;
    pActor->spr.xvel = 0;
    pActor->spr.yvel = 0;
    pActor->spr.zvel = 0;
    pActor->spr.xrepeat = 42;
    pActor->spr.yrepeat = 42;
    pActor->spr.pal = pActor->sector()->ceilingpal;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.ang = nAngle;
    pActor->spr.picnum = 1;
    pActor->spr.hitag = 0;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.extra = -1;

//	GrabTimeSlot(3);

    pActor->nAction = 0;
    pActor->nHealth = 640;
    pActor->nFrame = 0;
    pActor->pTarget = nullptr;
    pActor->nCount = 0;
    pActor->nPhase = Counters[kCountMummy]++;

    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0xE0000);

    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0xE0000);

    nCreaturesTotal++;
}

void CheckMummyRevive(DExhumedActor* pActor)
{
    ExhumedStatIterator it(102);
    while (auto pOther = it.Next())
    {
        if (pOther != pActor)
        {
            if (pOther->nAction != 5) {
                continue;
            }
            int x = abs(pOther->int_pos().X - pActor->int_pos().X) >> 8;
            int y = abs(pOther->int_pos().Y - pActor->int_pos().Y) >> 8;

            if (x <= 20 && y <= 20)
            {
                if (cansee(pActor->int_pos().X, pActor->int_pos().Y, pActor->int_pos().Z - 8192, pActor->sector(),
                          pOther->int_pos().X, pOther->int_pos().Y, pOther->int_pos().Z - 8192, pOther->sector()))
                {
                    pOther->spr.cstat = 0;
                    pOther->nAction = 6;
                    pOther->nFrame = 0;
                }
            }
        }
    }
}

void AIMummy::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    DExhumedActor* targ = pActor->pTarget;
    auto pTarget = UpdateEnemy(&targ);
    pActor->pTarget = targ;

    int nAction = pActor->nAction;

    Gravity(pActor);

    int nSeq = SeqOffsets[kSeqMummy] + MummySeq[nAction].a;

    pActor->spr.picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

    int nFrame = SeqBase[nSeq] + pActor->nFrame;
    int nFrameFlag = FrameFlag[nFrame];

    seq_MoveSequence(pActor, nSeq, pActor->nFrame);

    bool bVal = false;

    pActor->nFrame++;
    if (pActor->nFrame >= SeqSize[nSeq])
    {
        pActor->nFrame = 0;

        bVal = true;
    }

    if (pTarget != nullptr && nAction < 4)
    {
        if ((!pTarget->spr.cstat) && nAction)
        {
            pActor->nAction = 0;
            pActor->nFrame = 0;
            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;
        }
    }

    auto nMov = MoveCreatureWithCaution(pActor);

    if (nAction > 7)
        return;

    switch (nAction)
    {
    case 0:
    {
        if ((pActor->nPhase & 0x1F) == (totalmoves & 0x1F))
        {
            pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;

            if (pTarget == nullptr)
            {
                pTarget = FindPlayer(pActor, 100);
                if (pTarget != nullptr)
                {
                    D3PlayFX(StaticSound[kSound7], pActor);
                    pActor->nFrame = 0;
                    pActor->pTarget = pTarget;
                    pActor->nAction = 1;
                    pActor->nCount = 90;

                    pActor->spr.xvel = bcos(pActor->spr.ang, -2);
                    pActor->spr.yvel = bsin(pActor->spr.ang, -2);
                }
            }
        }
        return;
    }

    case 1:
    {
        if (pActor->nCount > 0)
        {
            pActor->nCount--;
        }

        if ((pActor->nPhase & 0x1F) == (totalmoves & 0x1F))
        {
            pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;

            PlotCourseToSprite(pActor, pTarget);

            if (pActor->nAction == 1)
            {
                if (RandomBit() && pTarget)
                {
                    if (cansee(pActor->int_pos().X, pActor->int_pos().Y, pActor->int_pos().Z - GetActorHeight(pActor), pActor->sector(),
                        pTarget->int_pos().X, pTarget->int_pos().Y, pTarget->int_pos().Z - GetActorHeight(pTarget), pTarget->sector()))
                    {
                        pActor->nAction = 3;
                        pActor->nFrame = 0;

                        pActor->spr.xvel = 0;
                        pActor->spr.yvel = 0;
                        return;
                    }
                }
            }
        }

        // loc_2B5A8
        if (!pActor->nFrame)
        {
            pActor->spr.xvel = bcos(pActor->spr.ang, -1);
            pActor->spr.yvel = bsin(pActor->spr.ang, -1);
        }

        if (pActor->spr.xvel || pActor->spr.yvel)
        {
            if (pActor->spr.xvel > 0)
            {
                pActor->spr.xvel -= 1024;
                if (pActor->spr.xvel < 0) {
                    pActor->spr.xvel = 0;
                }
            }
            else if (pActor->spr.xvel < 0)
            {
                pActor->spr.xvel += 1024;
                if (pActor->spr.xvel > 0) {
                    pActor->spr.xvel = 0;
                }
            }

            if (pActor->spr.yvel > 0)
            {
                pActor->spr.yvel -= 1024;
                if (pActor->spr.yvel < 0) {
                    pActor->spr.yvel = 0;
                }
            }
            else if (pActor->spr.yvel < 0)
            {
                pActor->spr.yvel += 1024;
                if (pActor->spr.yvel > 0) {
                    pActor->spr.yvel = 0;
                }
            }
        }

        switch (nMov.type)
        {
        case kHitWall:
        {
            pActor->spr.ang = (pActor->spr.ang + ((RandomWord() & 0x3FF) + 1024)) & kAngleMask;
            pActor->spr.xvel = bcos(pActor->spr.ang, -2);
            pActor->spr.yvel = bsin(pActor->spr.ang, -2);
            return;
        }

        case kHitSprite:
        {
            if (nMov.actor() == pTarget)
            {
                int nAngle = getangle(pTarget->int_pos().X - pActor->int_pos().X, pTarget->int_pos().Y - pActor->int_pos().Y);
                if (AngleDiff(pActor->spr.ang, nAngle) < 64)
                {
                    pActor->nAction = 2;
                    pActor->nFrame = 0;

                    pActor->spr.xvel = 0;
                    pActor->spr.yvel = 0;
                }
            }
            return;
        }
        }

        break;
    }

    case 2:
    {
        if (pTarget == nullptr)
        {
            pActor->nAction = 0;
            pActor->nFrame = 0;
        }
        else
        {
            if (PlotCourseToSprite(pActor, pTarget) >= 1024)
            {
                pActor->nAction = 1;
                pActor->nFrame = 0;
            }
            else if (nFrameFlag & 0x80)
            {
                runlist_DamageEnemy(pTarget, pActor, 5);
            }
        }
        return;
    }

    case 3:
    {
        if (bVal)
        {
            pActor->nFrame = 0;
            pActor->nAction = 0;
            pActor->nCount = 100;
            pActor->pTarget = nullptr;
            return;
        }
        else if (nFrameFlag & 0x80)
        {
            SetQuake(pActor, 100);

            // low 16 bits of returned var contains the sprite index, the high 16 the bullet number
            auto pBullet = BuildBullet(pActor, 9, -15360, pActor->spr.ang, pTarget, 1);
            CheckMummyRevive(pActor);

            if (pBullet)
            {
                if (!RandomSize(3))
                {
                    SetBulletEnemy(pBullet->nPhase, pTarget);
                    pBullet->spr.pal = 5;
                }
            }
        }
        return;
    }

    case 4:
    {
        if (bVal)
        {
            pActor->nFrame = 0;
            pActor->nAction = 5;
        }
        return;
    }

    case 5:
    {
        pActor->nFrame = 0;
        return;
    }

    case 6:
    {
        if (bVal)
        {
            pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;

            pActor->nAction = 0;
            pActor->nHealth = 300;
            pActor->pTarget = nullptr;

            nCreaturesTotal++;
        }
        return;
    }

    case 7:
    {
        if (nMov.exbits)
        {
            pActor->spr.xvel >>= 1;
            pActor->spr.yvel >>= 1;
        }

        if (bVal)
        {
            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;
            pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;

            pActor->nAction = 0;
            pActor->nFrame = 0;
            pActor->pTarget = nullptr;
        }

        return;
    }
    }
}

void AIMummy::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    int nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqMummy] + MummySeq[nAction].a, pActor->nFrame, MummySeq[nAction].b);
    return;
}

void AIMummy::RadialDamage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    if (pActor->nHealth <= 0)
        return;

    ev->nDamage = runlist_CheckRadialDamage(pActor);
    Damage(ev);
}

void AIMummy::Damage(RunListEvent* ev) 
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    if (ev->nDamage <= 0)
        return;

    if (pActor->nHealth <= 0) {
        return;
    }

    pActor->nHealth -= dmgAdjust(ev->nDamage);

    if (pActor->nHealth <= 0)
    {
        pActor->nHealth = 0;
        pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
        nCreaturesKilled++;

        DropMagic(pActor);

        pActor->nFrame = 0;
        pActor->nAction = 4;

        pActor->spr.xvel = 0;
        pActor->spr.yvel = 0;
        pActor->spr.zvel = 0;
        pActor->set_int_z(pActor->sector()->int_floorz());
    }
    else
    {
        if (!RandomSize(2))
        {
            pActor->nAction = 7;
            pActor->nFrame = 0;

            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;
        }
    }

    return;
}

END_PS_NS
