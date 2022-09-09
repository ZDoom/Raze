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

int nMagicSeq = -1;
int nPreMagicSeq  = -1;
int nSavePointSeq = -1;


void SerializeAnim(FSerializer& arc)
{
    if (arc.BeginObject("anims"))
    {
        arc("magic", nMagicSeq)
            ("premagic", nPreMagicSeq)
            ("savepoint", nSavePointSeq)
            .EndObject();
    }
}

void InitAnims()
{
    nMagicSeq     = SeqOffsets[kSeqItems] + 21;
    nPreMagicSeq  = SeqOffsets[kSeqMagic2];
    nSavePointSeq = SeqOffsets[kSeqItems] + 12;
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

DExhumedActor* BuildAnim(DExhumedActor* pActor, int val, int val2, const DVector3& pos, sectortype* pSector, int nRepeat, int nFlag)
{
    if (pActor == nullptr) {
        pActor = insertActor(pSector, 500);
    }
	pActor->spr.pos = pos;
    pActor->spr.cstat = 0;

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

    pActor->set_const_clipdist(10);
    pActor->spr.xrepeat = nRepeat;
    pActor->spr.yrepeat = nRepeat;
    pActor->spr.picnum = 1;
    pActor->spr.angle = nullAngle;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
    pActor->backuppos();

    // CHECKME - where is hitag set otherwise?
    if (pActor->spr.statnum < 900) {
        pActor->spr.hitag = -1;
    }

    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.intowner = -1;
    pActor->spr.extra = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x100000);

    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x100000);
    pActor->nAction = nFlag;
    pActor->nIndex = 0;
    pActor->nIndex2 = SeqOffsets[val] + val2;
    pActor->pTarget = nullptr;
    pActor->nDamage = pActor->nRun;
    pActor->nPhase = ITEM_MAGIC;

    if (nFlag & 0x80) {
        pActor->spr.cstat |= CSTAT_SPRITE_TRANSLUCENT; // set transluscence
    }

    return pActor;
}

void AIAnim::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    int nIndex2 = pActor->nIndex2;
    int nIndex = pActor->nIndex;

    if (!(pActor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
    {
        seq_MoveSequence(pActor, nIndex2, nIndex);
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

            if (!nIndex)
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

    pActor->nIndex++;
    if (pActor->nIndex >= SeqSize[nIndex2])
    {
        if (pActor->nAction & 0x10)
        {
            pActor->nIndex = 0;
        }
        else if (nIndex2 == nPreMagicSeq)
        {
            pActor->nIndex = 0;
            pActor->nIndex2 = nMagicSeq;
            pActor->nAction |= 0x10;
            pActor->spr.cstat |= CSTAT_SPRITE_TRANSLUCENT;
        }
        else if (nIndex2 == nSavePointSeq)
        {
            pActor->nIndex = 0;
            pActor->nIndex2++;
            pActor->nAction |= 0x10;
        }
        else
        {
            DestroyAnim(pActor);
        }
    }
}

void AIAnim::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;
    int nIndex2 = pActor->nIndex2;

    seq_PlotSequence(ev->nParam, nIndex2, pActor->nIndex, 0x101);
    ev->pTSprite->ownerActor = nullptr;
}

void BuildExplosion(DExhumedActor* pActor)
{
    auto pSector = pActor->sector();

    int edx = 36;

    if (pSector->Flag & kSectUnderwater)
    {
        edx = 75;
    }
    else if (pActor->spr.pos.Z == pActor->sector()->floorz)
    {
        edx = 34;
    }

    BuildAnim(nullptr, edx, 0, pActor->spr.pos, pActor->sector(), pActor->spr.xrepeat, 4);
}

void BuildSplash(DExhumedActor* pActor, sectortype* pSector)
{
    int nRepeat, nSound;

    if (pActor->spr.statnum != 200)
    {
        nRepeat = pActor->spr.xrepeat + (RandomWord() % pActor->spr.xrepeat);
        nSound = kSound0;
    }
    else
    {
        nRepeat = 20;
        nSound = kSound1;
    }

    int bIsLava = pSector->Flag & kSectLava;

    int edx, nFlag;

    if (bIsLava)
    {
        edx = 43;
        nFlag = 4;
    }
    else
    {
        edx = 35;
        nFlag = 0;
    }

	auto pSpawned = BuildAnim(nullptr, edx, 0, DVector3(pActor->spr.pos.XY(), pSector->floorz), pSector, nRepeat, nFlag);

    if (!bIsLava)
    {
        D3PlayFX(StaticSound[nSound] | 0xa00, pSpawned);
    }
}
END_PS_NS
