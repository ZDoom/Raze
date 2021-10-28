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


void BuildMummy(DExhumedActor* pActor, int x, int y, int z, int nSector, int nAngle)
{
    spritetype* pSprite;
    if (pActor == nullptr)
    {
        pActor = insertActor(nSector, 102);
        pSprite = &pActor->s();
    }
    else
    {
        pSprite = &pActor->s();
        x = pSprite->x;
        y = pSprite->y;
        z = pSprite->z;
        nAngle = pSprite->ang;

        ChangeActorStat(pActor, 102);
    }

    pSprite->x = x;
    pSprite->y = y;
    pSprite->z = z;
    pSprite->cstat = 0x101;
    pSprite->shade = -12;
    pSprite->clipdist = 32;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->xrepeat = 42;
    pSprite->yrepeat = 42;
    pSprite->pal = sector[pSprite->sectnum].ceilingpal;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->ang = nAngle;
    pSprite->picnum = 1;
    pSprite->hitag = 0;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->extra = -1;

//	GrabTimeSlot(3);

    pActor->nAction = 0;
    pActor->nHealth = 640;
    pActor->nFrame = 0;
    pActor->pTarget = nullptr;
    pActor->nCount = 0;
    pActor->nPhase = Counters[kCountMummy]++;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, pActor, 0xE0000);

    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0xE0000);

    nCreaturesTotal++;
}

void CheckMummyRevive(DExhumedActor* pActor)
{
	auto pSprite = &pActor->s();

    ExhumedStatIterator it(102);
    while (auto pOther = it.Next())
    {
        if (pOther != pActor)
        {
            if (pOther->nAction != 5) {
                continue;
            }
            auto pSprite2 = &pOther->s();

            int x = abs(pSprite2->x - pSprite->x) >> 8;
            int y = abs(pSprite2->y - pSprite->y) >> 8;

            if (x <= 20 && y <= 20)
            {
                if (cansee(pSprite->x, pSprite->y, pSprite->z - 8192, pSprite->sectnum,
                          pSprite2->x, pSprite2->y, pSprite2->z - 8192, pSprite2->sectnum))
                {
                    pSprite2->cstat = 0;
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

    auto pTarget = UpdateEnemy(&pActor->pTarget);

    auto pSprite = &pActor->s();
    short nAction = pActor->nAction;

    Gravity(pActor);

    int nSeq = SeqOffsets[kSeqMummy] + MummySeq[nAction].a;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

    short nFrame = SeqBase[nSeq] + pActor->nFrame;
    short nFrameFlag = FrameFlag[nFrame];

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
        if ((!pTarget->s().cstat) && nAction)
        {
            pActor->nAction = 0;
            pActor->nFrame = 0;
            pSprite->xvel = 0;
            pSprite->yvel = 0;
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
            pSprite->cstat = 0x101;

            if (pTarget == nullptr)
            {
                auto pTarget = FindPlayer(pActor, 100);
                if (pTarget != nullptr)
                {
                    D3PlayFX(StaticSound[kSound7], pActor);
                    pActor->nFrame = 0;
                    pActor->pTarget = pTarget;
                    pActor->nAction = 1;
                    pActor->nCount = 90;

                    pSprite->xvel = bcos(pSprite->ang, -2);
                    pSprite->yvel = bsin(pSprite->ang, -2);
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
            pSprite->cstat = 0x101;

            PlotCourseToSprite(pActor, pTarget);

            if (pActor->nAction == 1)
            {
                if (RandomBit() && pTarget)
                {
                    if (cansee(pSprite->x, pSprite->y, pSprite->z - GetActorHeight(pActor), pSprite->sectnum,
                        pTarget->s().x, pTarget->s().y, pTarget->s().z - GetActorHeight(pTarget), pTarget->s().sectnum))
                    {
                        pActor->nAction = 3;
                        pActor->nFrame = 0;

                        pSprite->xvel = 0;
                        pSprite->yvel = 0;
                        return;
                    }
                }
            }
        }

        // loc_2B5A8
        if (!pActor->nFrame)
        {
            pSprite->xvel = bcos(pSprite->ang, -1);
            pSprite->yvel = bsin(pSprite->ang, -1);
        }

        if (pSprite->xvel || pSprite->yvel)
        {
            if (pSprite->xvel > 0)
            {
                pSprite->xvel -= 1024;
                if (pSprite->xvel < 0) {
                    pSprite->xvel = 0;
                }
            }
            else if (pSprite->xvel < 0)
            {
                pSprite->xvel += 1024;
                if (pSprite->xvel > 0) {
                    pSprite->xvel = 0;
                }
            }

            if (pSprite->yvel > 0)
            {
                pSprite->yvel -= 1024;
                if (pSprite->yvel < 0) {
                    pSprite->yvel = 0;
                }
            }
            else if (pSprite->yvel < 0)
            {
                pSprite->yvel += 1024;
                if (pSprite->yvel > 0) {
                    pSprite->yvel = 0;
                }
            }
        }

        switch (nMov.type)
        {
        case kHitWall:
        {
            pSprite->ang = (pSprite->ang + ((RandomWord() & 0x3FF) + 1024)) & kAngleMask;
            pSprite->xvel = bcos(pSprite->ang, -2);
            pSprite->yvel = bsin(pSprite->ang, -2);
            return;
        }

        case kHitSprite:
        {
            if (nMov.actor == pTarget)
            {
                int nAngle = getangle(pTarget->s().x - pSprite->x, pTarget->s().y - pSprite->y);
                if (AngleDiff(pSprite->ang, nAngle) < 64)
                {
                    pActor->nAction = 2;
                    pActor->nFrame = 0;

                    pSprite->xvel = 0;
                    pSprite->yvel = 0;
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
            auto pBullet = BuildBullet(pActor, 9, -15360, pSprite->ang, pTarget, 1);
            CheckMummyRevive(pActor);

            if (pBullet)
            {
                if (!RandomSize(3))
                {
                    SetBulletEnemy(pBullet->nPhase, pTarget);
                    pBullet->s().pal = 5;
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
            pSprite->cstat = 0x101;

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
            pSprite->xvel >>= 1;
            pSprite->yvel >>= 1;
        }

        if (bVal)
        {
            pSprite->xvel = 0;
            pSprite->yvel = 0;
            pSprite->cstat = 0x101;

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
    short nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqMummy] + MummySeq[nAction].a, pActor->nFrame, MummySeq[nAction].b);
    return;
}

void AIMummy::RadialDamage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    auto pSprite = &pActor->s();

    if (pActor->nHealth <= 0)
        return;

    ev->nDamage = runlist_CheckRadialDamage(pActor);
    Damage(ev);
}

void AIMummy::Damage(RunListEvent* ev) 
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    auto pSprite = &pActor->s();

    if (ev->nDamage <= 0)
        return;

    if (pActor->nHealth <= 0) {
        return;
    }

    pActor->nHealth -= dmgAdjust(ev->nDamage);

    if (pActor->nHealth <= 0)
    {
        pActor->nHealth = 0;
        pSprite->cstat &= 0xFEFE;
        nCreaturesKilled++;

        DropMagic(pActor);

        pActor->nFrame = 0;
        pActor->nAction = 4;

        pSprite->xvel = 0;
        pSprite->yvel = 0;
        pSprite->zvel = 0;
        pSprite->z = sector[pSprite->sectnum].floorz;
    }
    else
    {
        if (!RandomSize(2))
        {
            pActor->nAction = 7;
            pActor->nFrame = 0;

            pSprite->xvel = 0;
            pSprite->yvel = 0;
        }
    }

    return;
}

END_PS_NS
