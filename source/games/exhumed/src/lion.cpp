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
#include "sound.h"
#include <assert.h>

BEGIN_PS_NS

static actionSeq LionSeq[] = {
    {54, 1},
    {18, 0},
    {0,  0},
    {10, 0},
    {44, 0},
    {18, 0},
    {26, 0},
    {34, 0},
    {8,  1},
    {9,  1},
    {52, 1},
    {53, 1}
};


void BuildLion(DExhumedActor* pActor, int x, int y, int z, int nSector, short nAngle)
{
    spritetype* pSprite;
    if (pActor == nullptr)
    {
        pActor = insertActor(nSector, 104);
		pSprite = &pActor->s();
    }
    else
    {
        ChangeActorStat(pActor, 104);
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
    pSprite->clipdist = 60;
    pSprite->shade = -12;
    pSprite->xrepeat = 40;
    pSprite->yrepeat = 40;
    pSprite->picnum = 1;
    pSprite->pal = pSprite->sector()->ceilingpal;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->ang = nAngle;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->hitag = 0;
    pSprite->extra = -1;

//	GrabTimeSlot(3);

    pActor->nAction = 0;
    pActor->nHealth = 500;
    pActor->nFrame = 0;
    pActor->pTarget = nullptr;
    pActor->nCount = 0;
    pActor->nPhase = Counters[kCountLion]++;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, pActor, 0x130000);

    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x130000);

    nCreaturesTotal++;
}

void AILion::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    short nAction = pActor->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[kSeqLion] + LionSeq[nAction].a, pActor->nFrame, LionSeq[nAction].b);
}

void AILion::RadialDamage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    ev->nDamage = runlist_CheckRadialDamage(pActor);
    Damage(ev);
}

void AILion::Damage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    auto pSprite = &pActor->s();
    short nAction = pActor->nAction;

    if (ev->nDamage && pActor->nHealth > 0)
    {
        pActor->nHealth -= dmgAdjust(ev->nDamage);
        if (pActor->nHealth <= 0)
        {
            // R.I.P.
            pSprite->xvel = 0;
            pSprite->yvel = 0;
            pSprite->zvel = 0;
            pSprite->cstat &= 0xFEFE;

            pActor->nHealth = 0;

            nCreaturesKilled++;

            if (nAction < 10)
            {
                DropMagic(pActor);

                if (ev->isRadialEvent())
                {
                    pActor->nAction = 11;
                }
                else
                {
                    pActor->nAction = 10;
                }

                pActor->nFrame = 0;
                return;
            }
        }
        else
        {
            auto pTarget = ev->pOtherActor;

            if (pTarget)
            {
                if (pTarget->s().statnum < 199) {
                    pActor->pTarget = pTarget;
                }

                if (nAction != 6)
                {
                    if (RandomSize(8) <= (pActor->nHealth >> 2))
                    {
                        pActor->nAction = 4;
                        pSprite->xvel = 0;
                        pSprite->yvel = 0;
                    }
                    else if (RandomSize(1))
                    {
                        PlotCourseToSprite(pActor, pTarget);
                        pActor->nAction = 5;
                        pActor->nCount = RandomSize(3);
                        pSprite->ang = (pSprite->ang - (RandomSize(1) << 8)) + (RandomSize(1) << 8); // NOTE: no angle mask in original code
                    }
                    else
                    {
                        pActor->nAction = 8;
                        pSprite->xvel = 0;
                        pSprite->yvel = 0;
                        pSprite->cstat &= 0xFEFE;
                    }

                    pActor->nFrame = 0;
                }
            }
        }
    }
}

void AILion::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    auto pSprite = &pActor->s();
    short nAction = pActor->nAction;

    bool bVal = false;


    if (nAction != 7) {
        Gravity(pActor);
    }

    short nSeq = SeqOffsets[kSeqLion] + LionSeq[nAction].a;

    pSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

    seq_MoveSequence(pActor, nSeq, pActor->nFrame);

    pActor->nFrame++;
    if (pActor->nFrame >= SeqSize[nSeq])
    {
        pActor->nFrame = 0;
        bVal = true;
    }

    short nFlag = FrameFlag[SeqBase[nSeq] + pActor->nFrame];
    auto pTarget = pActor->pTarget;

    auto nMov = MoveCreatureWithCaution(pActor);

    switch (nAction)
    {
    default:
        return;

    case 0:
    case 1:
    {
        if ((pActor->nPhase & 0x1F) == (totalmoves & 0x1F))
        {
            if (pTarget == nullptr)
            {
                pTarget = FindPlayer(pActor, 40);
                if (pTarget)
                {
                    D3PlayFX(StaticSound[kSound24], pActor);
                    pActor->nAction = 2;
                    pActor->nFrame = 0;

                    pSprite->xvel = bcos(pSprite->ang, -1);
                    pSprite->yvel = bsin(pSprite->ang, -1);
                    pActor->pTarget = pTarget;
                    return;
                }
            }
        }

        if (nAction && !easy())
        {
            pActor->nCount--;
            if (pActor->nCount <= 0)
            {
                if (RandomBit())
                {
                    pSprite->ang = RandomWord() & kAngleMask;
                    pSprite->xvel = bcos(pSprite->ang, -1);
                    pSprite->yvel = bsin(pSprite->ang, -1);
                }
                else
                {
                    pSprite->xvel = 0;
                    pSprite->yvel = 0;
                }

                pActor->nCount = 100;
            }
        }

        return;
    }

    case 2:
    {
        if ((totalmoves & 0x1F) == (pActor->nPhase & 0x1F))
        {
            PlotCourseToSprite(pActor, pTarget);

            int nAng = pSprite->ang & 0xFFF8;

            if (pSprite->cstat & 0x8000)
            {
                pSprite->xvel = bcos(nAng, 1);
                pSprite->yvel = bsin(nAng, 1);
            }
            else
            {
                pSprite->xvel = bcos(nAng, -1);
                pSprite->yvel = bsin(nAng, -1);
            }
        }

        if (nMov.type == kHitWall)
        {
            // loc_378FA:
            pSprite->ang = (pSprite->ang + 256) & kAngleMask;
            pSprite->xvel = bcos(pSprite->ang, -1);
            pSprite->yvel = bsin(pSprite->ang, -1);
            break;
        }
        else if (nMov.type == kHitSprite)
        {
            if (nMov.actor == pTarget)
            {
                if (pSprite->cstat & 0x8000)
                {
                    pActor->nAction = 9;
                    pSprite->cstat &= 0x7FFF;
                    pSprite->xvel = 0;
                    pSprite->yvel = 0;
                }
                else
                {
                    int nAng = getangle(pTarget->s().x - pSprite->x, pTarget->s().y - pSprite->y);

                    if (AngleDiff(pSprite->ang, nAng) < 64)
                    {
                        pActor->nAction = 3;
                    }
                }

                pActor->nFrame = 0;
                break;
            }
            else
            {
                // loc_378FA:
                pSprite->ang = (pSprite->ang + 256) & kAngleMask;
                pSprite->xvel = bcos(pSprite->ang, -1);
                pSprite->yvel = bsin(pSprite->ang, -1);
                break;
            }
        }

        break;
    }

    case 3:
    {
        if (pTarget == nullptr)
        {
            pActor->nAction = 1;
            pActor->nCount = 50;
        }
        else
        {
            if (PlotCourseToSprite(pActor, pTarget) >= 768)
            {
                pActor->nAction = 2;
            }
            else if (nFlag & 0x80)
            {
                runlist_DamageEnemy(pTarget, pActor, 10);
            }
        }

        break;
    }

    case 4:
    {
        if (bVal)
        {
            pActor->nAction = 2;
            pActor->nFrame = 0;
        }

        if (nMov.exbits & kHitAux2)
        {
            pSprite->xvel >>= 1;
            pSprite->yvel >>= 1;
        }

        return;
    }

    case 5: // Jump away when damaged
    {
        pActor->nCount--;
        if (pActor->nCount <= 0)
        {
            pSprite->zvel = -4000;
            pActor->nCount = 0;

            int x = pSprite->x;
            int y = pSprite->y;
            int z = pSprite->z - (GetActorHeight(pActor) >> 1);

            int nCheckDist = 0x7FFFFFFF;

            short nAngle = pSprite->ang;
            short nScanAngle = (pSprite->ang - 512) & kAngleMask;

            for (int i = 0; i < 5; i++)
            {
                short hitwall;
                int hitx, hity;
                vec3_t startPos = { x, y, z };
                hitdata_t hitData;

                hitscan(&startPos, pSprite->sectnum, bcos(nScanAngle), bsin(nScanAngle), 0, &hitData, CLIPMASK1);

                hitx = hitData.pos.x;
                hity = hitData.pos.y;
                hitwall = hitData.wall;

                if (hitwall > -1)
                {
                    int theX = abs(hitx - x);
                    int theY = abs(hity - y);

                    if ((theX + theY) < nCheckDist)
                    {
                        nCheckDist = theX;
                        nAngle = nScanAngle;
                    }
                }

                nScanAngle += 256;
                nScanAngle &= kAngleMask;
            }

            pSprite->ang = nAngle;

            pActor->nAction = 6;
            pSprite->xvel = bcos(pSprite->ang) - bcos(pSprite->ang, -3);
            pSprite->yvel = bsin(pSprite->ang) - bsin(pSprite->ang, -3);
            D3PlayFX(StaticSound[kSound24], pActor);
        }

        return;
    }

    case 6:
    {
        if (nMov.exbits)
        {
            pActor->nAction = 2;
            pActor->nFrame = 0;
            return;
        }

        if (nMov.type == kHitWall)
        {
            pActor->nAction = 7;
            pSprite->ang = (GetWallNormal(nMov.index) + 1024) & kAngleMask;
            pActor->nCount = RandomSize(4);
            return;
        }
        else if (nMov.type == kHitSprite)
        {
            if (nMov.actor == pTarget)
            {
                int nAng = getangle(pTarget->s().x - pSprite->x, pTarget->s().y - pSprite->y);
                if (AngleDiff(pSprite->ang, nAng) < 64)
                {
                    pActor->nAction = 3;
                    pActor->nFrame = 0;
                }
            }
            else
            {
                // loc_378FA:
                pSprite->ang = (pSprite->ang + 256) & kAngleMask;
                pSprite->xvel = bcos(pSprite->ang, -1);
                pSprite->yvel = bsin(pSprite->ang, -1);
                break;
            }
        }

        return;
    }

    case 7:
    {
        pActor->nCount--;

        if (pActor->nCount <= 0)
        {
            pActor->nCount = 0;
            if (pTarget)
            {
                PlotCourseToSprite(pActor, pTarget);
            }
            else
            {
                pSprite->ang = (RandomSize(9) + (pSprite->ang + 768)) & kAngleMask;
            }

            pSprite->zvel = -1000;

            pActor->nAction = 6;
            pSprite->xvel = bcos(pSprite->ang) - bcos(pSprite->ang, -3);
            pSprite->yvel = bsin(pSprite->ang) - bsin(pSprite->ang, -3);
            D3PlayFX(StaticSound[kSound24], pActor);
        }

        return;
    }

    case 8:
    {
        if (bVal)
        {
            pActor->nAction = 2;
            pActor->nFrame = 0;
            pSprite->cstat |= 0x8000;
        }
        return;
    }

    case 9:
    {
        if (bVal)
        {
            pActor->nFrame = 0;
            pActor->nAction = 2;
            pSprite->cstat |= 0x101;
        }
        return;
    }

    case 10:
    case 11:
    {
        if (bVal)
        {
            runlist_SubRunRec(pSprite->owner);
            runlist_SubRunRec(pActor->nRun);
            pSprite->cstat = 0x8000;
        }
        return;
    }
    }

    // loc_379AD: ?
    if (nAction != 1 && pTarget != nullptr)
    {
        if (!(pTarget->s().cstat & 0x101))
        {
            pActor->nAction = 1;
            pActor->nFrame = 0;
            pActor->nCount = 100;
            pActor->pTarget = nullptr;
            pSprite->xvel = 0;
            pSprite->yvel = 0;
        }
    }
}

END_PS_NS
