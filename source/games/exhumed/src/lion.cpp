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


void BuildLion(DExhumedActor* pActor, const DVector3& pos, sectortype* pSector, DAngle nAngle)
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
        nAngle = pActor->spr.Angles.Yaw;
    }

    pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
	pActor->clipdist = 15;
    pActor->spr.shade = -12;
    pActor->spr.scale = DVector2(0.625, 0.625);
    pActor->spr.picnum = 1;
    pActor->spr.pal = pActor->sector()->ceilingpal;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.Angles.Yaw = nAngle;
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
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

    pActor->nSeqFile = "lion";

    nCreaturesTotal++;
}

void AILion::Draw(RunListEvent* ev)
{
    if (const auto pActor = ev->pObjActor)
    {
        const auto lionSeq = &LionSeq[pActor->nAction];
        seq_PlotSequence(ev->nParam, pActor->nSeqFile, lionSeq->nSeqId, pActor->nFrame, lionSeq->nFlags);
    }
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
            pActor->vel.X = 0;
            pActor->vel.Y = 0;
            pActor->vel.Z = 0;
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
                        pActor->vel.X = 0;
                        pActor->vel.Y = 0;
                    }
                    else if (RandomSize(1))
                    {
                        PlotCourseToSprite(pActor, pTarget);
                        pActor->nAction = 5;
                        pActor->nCount = RandomSize(3);
                        pActor->spr.Angles.Yaw += mapangle((- (RandomSize(1) << 8)) + (RandomSize(1) << 8)); // NOTE: no angle mask in original code
                    }
                    else
                    {
                        pActor->nAction = 8;
                        pActor->vel.X = 0;
                        pActor->vel.Y = 0;
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

    const auto& lionSeq = getSequence(pActor->nSeqFile, LionSeq[nAction].nSeqId);
    const auto& seqFrame = lionSeq.frames[pActor->nFrame];

    pActor->spr.picnum = seqFrame.getFirstPicnum();

    playFrameSound(pActor, seqFrame);

    pActor->nFrame++;
    if (pActor->nFrame >= lionSeq.frames.Size())
    {
        pActor->nFrame = 0;
        bVal = true;
    }

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

                    pActor->VelFromAngle(-1);
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
                    pActor->spr.Angles.Yaw = RandomAngle();
                    pActor->VelFromAngle(-1);
                }
                else
                {
                    pActor->vel.X = 0;
                    pActor->vel.Y = 0;
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

            if (pActor->spr.cstat & CSTAT_SPRITE_INVISIBLE)
            {
				pActor->vel.XY() = pActor->spr.Angles.Yaw.ToVector() * 2048;
            }
            else
            {
				pActor->vel.XY() = pActor->spr.Angles.Yaw.ToVector() * 512;
            }
        }

        if (nMov.type == kHitWall)
        {
            // loc_378FA:
            pActor->spr.Angles.Yaw += DAngle45;
            pActor->VelFromAngle(-1);
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
                    pActor->vel.X = 0;
                    pActor->vel.Y = 0;
                }
                else
                {
                    auto nAngDiff = absangle(pActor->spr.Angles.Yaw, (pTarget->spr.pos - pActor->spr.pos).Angle());
                    if (nAngDiff < DAngle22_5 / 2)
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
                pActor->spr.Angles.Yaw += DAngle45;
                pActor->VelFromAngle(-1);
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
            if (PlotCourseToSprite(pActor, pTarget) >= 48)
            {
                pActor->nAction = 2;
            }
            else if (seqFrame.flags & 0x80)
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
            pActor->vel.X *= 0.5;
            pActor->vel.Y *= 0.5;
        }

        return;
    }

    case 5: // Jump away when damaged
    {
        pActor->nCount--;
        if (pActor->nCount <= 0)
        {
            pActor->vel.Z = -4000 / 256.;
            pActor->nCount = 0;

            double nCheckDist = 0x7FFFFFFF;

            DAngle nAngle = pActor->spr.Angles.Yaw;
            DAngle nScanAngle = (nAngle - DAngle90).Normalized360();

            for (int i = 0; i < 5; i++)
            {
                HitInfo hit{};

                hitscan(pActor->spr.pos.plusZ(-GetActorHeight(pActor) * 0.5), pActor->sector(), DVector3(nScanAngle.ToVector() * 1024, 0), hit, CLIPMASK1);

                if (hit.hitWall)
                {
                    double theX = abs(hit.hitpos.X - pActor->spr.pos.X);
                    double theY = abs(hit.hitpos.Y - pActor->spr.pos.Y);

                    if ((theX + theY) < nCheckDist)
                    {
                        nCheckDist = theX;
                        nAngle = nScanAngle;
                    }
                }

                nScanAngle += DAngle45;
            }

            pActor->spr.Angles.Yaw = nAngle;

            pActor->nAction = 6;
			pActor->vel.XY() = pActor->spr.Angles.Yaw.ToVector() * (1024 - 128);
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
            pActor->spr.Angles.Yaw = (nMov.hitWall->normalAngle() + DAngle180).Normalized360();
            pActor->nCount = RandomSize(4);
            return;
        }
        else if (nMov.type == kHitSprite)
        {
            if (nMov.actor() == pTarget)
            {
                auto nAngDiff = absangle(pActor->spr.Angles.Yaw, (pTarget->spr.pos - pActor->spr.pos).Angle());
                if (nAngDiff < DAngle22_5 / 2)
                {
                    pActor->nAction = 3;
                    pActor->nFrame = 0;
                }
            }
            else
            {
                // loc_378FA:
                pActor->spr.Angles.Yaw += DAngle45;
                pActor->VelFromAngle(-1);
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
                pActor->spr.Angles.Yaw += RandomAngle9() + DAngle45 + DAngle90;
            }

            pActor->vel.Z = -1000 / 256.;

            pActor->nAction = 6;
			pActor->vel.XY() = pActor->spr.Angles.Yaw.ToVector() * (1024 - 128);
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
            pActor->vel.X = 0;
            pActor->vel.Y = 0;
        }
    }
}

END_PS_NS
