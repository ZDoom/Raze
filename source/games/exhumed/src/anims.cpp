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


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

/*
    Use when deleting an ignited actor to check if any actors reference it. 
    Will remove the actor's anim loop flag and set the source (the ignited actor's) actor target to null.
    FuncAnim() will then delete the actor on next call for this actor.

    Without this, the actor will hold reference to an actor which will prevent the GC from deleting it properly.

    Specifically needed for IgniteSprite() anims which can become orphaned from the source actor (e.g a bullet)
    when the bullet actor is deleted.
*/

void UnlinkIgnitedAnim(DExhumedActor* pActor)
{
    ExhumedStatIterator it(kStatIgnited);
    while (auto itActor = it.Next())
    {
        if (pActor == itActor->pTarget)
        {
            itActor->nAction &= ~kAnimLoop;
            itActor->pTarget = nullptr;
        }
    }
}

void DestroyAnim(DExhumedActor* pActor)
{
    if (pActor)
    {
        StopActorSound(pActor);
        runlist_SubRunRec(pActor->nRun);
        runlist_DoSubRunRec(pActor->spr.extra);
        runlist_FreeRun(pActor->spr.lotag - 1);
        DeleteActor(pActor);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DExhumedActor* BuildAnim(DExhumedActor* pActor, const FName seqFile, int seqIndex, const DVector3& pos, sectortype* pSector, double nScale, int nFlag)
{
    if (pActor == nullptr)
        pActor = insertActor(pSector, 500);

    if (nFlag & 4)
    {
        pActor->spr.pal = 4;
        pActor->spr.shade = -64;
    }
    else
    {
        pActor->spr.pal = 0;
        pActor->spr.shade = -12;
    }

    // CHECKME - where is hitag set otherwise?
    if (pActor->spr.statnum < 900)
        pActor->spr.hitag = -1;

    if (nFlag & 0x80)
        pActor->spr.cstat |= CSTAT_SPRITE_TRANSLUCENT; // set transluscence

	pActor->spr.pos = pos;
    pActor->spr.cstat = 0;
    pActor->clipdist = 2.5;
	pActor->spr.scale = DVector2(nScale, nScale);
    pActor->spr.picnum = 1;
    pActor->spr.Angles.Yaw = nullAngle;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.intowner = -1;
    pActor->spr.extra = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x100000);
    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x100000);
    pActor->nFlags = nFlag;
    pActor->nFrame = 0;
    pActor->nSeqFile = seqFile;
    pActor->nSeqIndex = seqIndex;
    pActor->pTarget = nullptr;
    pActor->nPhase = ITEM_MAGIC;
    pActor->backuppos();

    return pActor;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIAnim::Tick(RunListEvent* ev)
{
    const auto pActor = ev->pObjActor;
    if (!pActor || pActor->nSeqFile == NAME_None) return;

    const auto animSeq = getSequence(pActor->nSeqFile, pActor->nSeqIndex);
    const int nFrame = pActor->nFrame;

    if (!(pActor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
    {
        animSeq->frames[nFrame].playSound(pActor);
    }

    if (pActor->spr.statnum == kStatIgnited)
    {
        DExhumedActor* pIgniter = pActor->pTarget;

        if (pIgniter)
        {
			pActor->spr.pos = pIgniter->spr.pos;

            if (pIgniter->sector() != pActor->sector())
            {
                if (!pIgniter->sector())
                {
                    DestroyAnim(pActor);
                    return;
                }
                else
                {
                    ChangeActorSect(pActor, pIgniter->sector());
                }
            }

            if (!nFrame)
            {
                if (pIgniter->spr.cstat != CSTAT_SPRITE_INVISIBLE)
                {
                    int hitag2 = pIgniter->spr.hitag;
                    pIgniter->spr.hitag--;

                    if (hitag2 >= 15)
                    {
                        runlist_DamageEnemy(pIgniter, nullptr, (pIgniter->spr.hitag - 14) * 2);

                        if (pIgniter->spr.shade < 100)
                        {
                            pIgniter->spr.pal = 0;
                            pIgniter->spr.shade++;
                        }

                        if (!(pIgniter->spr.cstat & CSTAT_SPRITE_BLOCK_ALL)) // was 101 (decimal), GDX had 0x101 which appears to be correct.
                        {
		                    DestroyAnim(pActor);
                            return;
                        }
                    }
                    else
                    {
                        pIgniter->spr.hitag = 1;
	                    DestroyAnim(pActor);
                    }
                }
                else
                {
                    pIgniter->spr.hitag = 1;
                    DestroyAnim(pActor);
                }
            }
        }
    }

    pActor->nFrame++;
    if (pActor->nFrame >= animSeq->frames.Size())
    {
        if (pActor->nFlags & kAnimLoop)
        {
            pActor->nFrame = 0;
        }
        else if (pActor->nSeqFile == FName("magic2") && pActor->nSeqIndex == 0)
        {
            pActor->nFrame = 0;
            pActor->nSeqFile = "items";
            pActor->nSeqIndex = 21;
            pActor->nFlags |= kAnimLoop;
            pActor->spr.cstat |= CSTAT_SPRITE_TRANSLUCENT;
        }
        else if (pActor->nSeqFile == FName("items") && pActor->nSeqIndex == 12)
        {
            pActor->nFrame = 0;
            pActor->nSeqIndex++;
            pActor->nFlags |= kAnimLoop;
        }
        else
        {
            DestroyAnim(pActor);
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIAnim::Draw(RunListEvent* ev)
{
    const auto pActor = ev->pObjActor;
    if (!pActor || pActor->nSeqFile == NAME_None) return;

    seq_PlotSequence(ev->nParam, pActor->nSeqFile, pActor->nSeqIndex, pActor->nFrame, 0x101);
    ev->pTSprite->ownerActor = nullptr;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void BuildExplosion(DExhumedActor* pActor)
{
    const auto pSector = pActor->sector();

    FName seqFile = "grenpow";

    if (pSector->Flag & kSectUnderwater)
    {
        seqFile = "grenbubb";
    }
    else if (pActor->spr.pos.Z == pSector->floorz)
    {
        seqFile = "grenboom";
    }

    BuildAnim(nullptr, seqFile, 0, pActor->spr.pos, pSector, pActor->spr.scale.X, 4);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void BuildSplash(DExhumedActor* pActor, sectortype* pSector)
{
    int nSound;
	double nScale;

    if (pActor->spr.statnum != 200)
    {
		const double rep = pActor->spr.scale.X;
        nScale = rep + RandomFloat(rep);
        nSound = kSound0;
    }
    else
    {
        nScale = 0.3125;
        nSound = kSound1;
    }

    const int bIsLava = pSector->Flag & kSectLava;

    FName seqFile;
    int nFlag;

    if (bIsLava)
    {
        seqFile = "lsplash";
        nFlag = 4;
    }
    else
    {
        seqFile = "splash";
        nFlag = 0;
    }

	const auto pSpawned = BuildAnim(nullptr, seqFile, 0, DVector3(pActor->spr.pos.XY(), pSector->floorz), pSector, nScale, nFlag);

    if (!bIsLava)
    {
        D3PlayFX(StaticSound[nSound] | 0xa00, pSpawned);
    }
}
END_PS_NS
