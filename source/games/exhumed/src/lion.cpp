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


void BuildLion(DExhumedActor* pActor, const DVector3& pos, sectortype* pSector, int nAngle)
{
    if (pActor == nullptr)
    {
        pActor = insertActor(pSector, 104);
		pActor->spr.pos = pos;
    }
    else
    {
        ChangeActorStat(pActor, 104);
        pActor->spr.pos.Z = pActor->sector()->floorz;
        nAngle = pActor->int_ang();
    }

    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pActor->spr.clipdist = 60;
    pActor->spr.shade = -12;
    pActor->spr.xrepeat = 40;
    pActor->spr.yrepeat = 40;
    pActor->spr.picnum = 1;
    pActor->spr.pal = pActor->sector()->ceilingpal;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->set_int_ang(nAngle);
    pActor->spr.xvel = 0;
    pActor->spr.yvel = 0;
    pActor->spr.zvel = 0;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.hitag = 0;
    pActor->spr.extra = -1;

//	GrabTimeSlot(3);

    pActor->nAction = 0;
    pActor->nHealth = 500;
    pActor->nFrame = 0;
    pActor->pTarget = nullptr;
    pActor->nCount = 0;
    pActor->nPhase = Counters[kCountLion]++;

    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x130000);

    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x130000);

    nCreaturesTotal++;
}

void AILion::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    int nAction = pActor->nAction;

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

    int nAction = pActor->nAction;

    if (ev->nDamage && pActor->nHealth > 0)
    {
        pActor->nHealth -= dmgAdjust(ev->nDamage);
        if (pActor->nHealth <= 0)
        {
            // R.I.P.
            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;
            pActor->spr.zvel = 0;
            pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;

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
                if (pTarget->spr.statnum < 199) {
                    pActor->pTarget = pTarget;
                }

                if (nAction != 6)
                {
                    if (RandomSize(8) <= (pActor->nHealth >> 2))
                    {
                        pActor->nAction = 4;
                        pActor->spr.xvel = 0;
                        pActor->spr.yvel = 0;
                    }
                    else if (RandomSize(1))
                    {
                        PlotCourseToSprite(pActor, pTarget);
                        pActor->nAction = 5;
                        pActor->nCount = RandomSize(3);
                        pActor->set_int_ang((pActor->int_ang() - (RandomSize(1) << 8)) + (RandomSize(1) << 8)); // NOTE: no angle mask in original code
                    }
                    else
                    {
                        pActor->nAction = 8;
                        pActor->spr.xvel = 0;
                        pActor->spr.yvel = 0;
                        pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
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

    int nAction = pActor->nAction;

    bool bVal = false;


    if (nAction != 7) {
        Gravity(pActor);
    }

    int nSeq = SeqOffsets[kSeqLion] + LionSeq[nAction].a;

    pActor->spr.picnum = seq_GetSeqPicnum2(nSeq, pActor->nFrame);

    seq_MoveSequence(pActor, nSeq, pActor->nFrame);

    pActor->nFrame++;
    if (pActor->nFrame >= SeqSize[nSeq])
    {
        pActor->nFrame = 0;
        bVal = true;
    }

    int nFlag = FrameFlag[SeqBase[nSeq] + pActor->nFrame];
    DExhumedActor* pTarget = pActor->pTarget;

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

                    pActor->spr.xvel = bcos(pActor->int_ang(), -1);
                    pActor->spr.yvel = bsin(pActor->int_ang(), -1);
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
                    pActor->set_int_ang(RandomWord() & kAngleMask);
                    pActor->spr.xvel = bcos(pActor->int_ang(), -1);
                    pActor->spr.yvel = bsin(pActor->int_ang(), -1);
                }
                else
                {
                    pActor->spr.xvel = 0;
                    pActor->spr.yvel = 0;
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

            int nAng = pActor->int_ang() & 0xFFF8;

            if (pActor->spr.cstat & CSTAT_SPRITE_INVISIBLE)
            {
                pActor->spr.xvel = bcos(nAng, 1);
                pActor->spr.yvel = bsin(nAng, 1);
            }
            else
            {
                pActor->spr.xvel = bcos(nAng, -1);
                pActor->spr.yvel = bsin(nAng, -1);
            }
        }

        if (nMov.type == kHitWall)
        {
            // loc_378FA:
            pActor->set_int_ang((pActor->int_ang() + 256) & kAngleMask);
            pActor->spr.xvel = bcos(pActor->int_ang(), -1);
            pActor->spr.yvel = bsin(pActor->int_ang(), -1);
            break;
        }
        else if (nMov.type == kHitSprite)
        {
            if (nMov.actor() == pTarget)
            {
                if (pActor->spr.cstat & CSTAT_SPRITE_INVISIBLE)
                {
                    pActor->nAction = 9;
                    pActor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
                    pActor->spr.xvel = 0;
                    pActor->spr.yvel = 0;
                }
                else
                {
					auto nAngDiff = AngleDiff(pActor->spr.angle, VecToAngle(pTarget->spr.pos - pActor->spr.pos));
					if (nAngDiff < 64)
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
                pActor->set_int_ang((pActor->int_ang() + 256) & kAngleMask);
                pActor->spr.xvel = bcos(pActor->int_ang(), -1);
                pActor->spr.yvel = bsin(pActor->int_ang(), -1);
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
            pActor->spr.xvel >>= 1;
            pActor->spr.yvel >>= 1;
        }

        return;
    }

    case 5: // Jump away when damaged
    {
        pActor->nCount--;
        if (pActor->nCount <= 0)
        {
            pActor->spr.zvel = -4000;
            pActor->nCount = 0;

            int x = pActor->int_pos().X;
            int y = pActor->int_pos().Y;
            int z = pActor->int_pos().Z - (GetActorHeight(pActor) >> 1);

            int nCheckDist = 0x7FFFFFFF;

            int nAngle = pActor->int_ang();
            int nScanAngle = (pActor->int_ang() - 512) & kAngleMask;

            for (int i = 0; i < 5; i++)
            {
                HitInfo hit{};

                hitscan(vec3_t( x, y, z ), pActor->sector(), { bcos(nScanAngle), bsin(nScanAngle), 0 }, hit, CLIPMASK1);

                if (hit.hitWall)
                {
                    int theX = abs(hit.int_hitpos().X - x);
                    int theY = abs(hit.int_hitpos().Y - y);

                    if ((theX + theY) < nCheckDist)
                    {
                        nCheckDist = theX;
                        nAngle = nScanAngle;
                    }
                }

                nScanAngle += 256;
                nScanAngle &= kAngleMask;
            }

            pActor->set_int_ang(nAngle);

            pActor->nAction = 6;
            pActor->spr.xvel = bcos(pActor->int_ang()) - bcos(pActor->int_ang(), -3);
            pActor->spr.yvel = bsin(pActor->int_ang()) - bsin(pActor->int_ang(), -3);
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
            pActor->set_int_ang((GetWallNormal(nMov.hitWall) + 1024) & kAngleMask);
            pActor->nCount = RandomSize(4);
            return;
        }
        else if (nMov.type == kHitSprite)
        {
            if (nMov.actor() == pTarget)
            {
				auto nAngDiff = AngleDiff(pActor->spr.angle, VecToAngle(pTarget->spr.pos - pActor->spr.pos));
				if (nAngDiff < 64)
                {
                    pActor->nAction = 3;
                    pActor->nFrame = 0;
                }
            }
            else
            {
                // loc_378FA:
                pActor->set_int_ang((pActor->int_ang() + 256) & kAngleMask);
                pActor->spr.xvel = bcos(pActor->int_ang(), -1);
                pActor->spr.yvel = bsin(pActor->int_ang(), -1);
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
                pActor->set_int_ang((RandomSize(9) + (pActor->int_ang() + 768)) & kAngleMask);
            }

            pActor->spr.zvel = -1000;

            pActor->nAction = 6;
            pActor->spr.xvel = bcos(pActor->int_ang()) - bcos(pActor->int_ang(), -3);
            pActor->spr.yvel = bsin(pActor->int_ang()) - bsin(pActor->int_ang(), -3);
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
            pActor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
        }
        return;
    }

    case 9:
    {
        if (bVal)
        {
            pActor->nFrame = 0;
            pActor->nAction = 2;
            pActor->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
        }
        return;
    }

    case 10:
    case 11:
    {
        if (bVal)
        {
            runlist_SubRunRec(pActor->spr.intowner);
            runlist_SubRunRec(pActor->nRun);
            pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
        }
        return;
    }
    }

    // loc_379AD: ?
    if (nAction != 1 && pTarget != nullptr)
    {
        if (!(pTarget->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
        {
            pActor->nAction = 1;
            pActor->nFrame = 0;
            pActor->nCount = 100;
            pActor->pTarget = nullptr;
            pActor->spr.xvel = 0;
            pActor->spr.yvel = 0;
        }
    }
}

END_PS_NS
