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

short nMagicSeq = -1;
short nPreMagicSeq  = -1;
short nSavePointSeq = -1;
//FreeListArray<Anim, kMaxAnims> AnimList;


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
		auto pSprite = &pActor->s();
        StopActorSound(pActor);
        runlist_SubRunRec(pActor->nRun);
        runlist_DoSubRunRec(pSprite->extra);
        runlist_FreeRun(pSprite->lotag - 1);
        DeleteActor(pActor);
    }
}

DExhumedActor* BuildAnim(DExhumedActor* pActor, int val, int val2, int x, int y, int z, int nSector, int nRepeat, int nFlag)
{
    if (pActor == nullptr) {
        pActor = insertActor(nSector, 500);
    }
	auto pSprite = &pActor->s();

    pSprite->x = x;
    pSprite->y = y;
    pSprite->z = z;
    pSprite->cstat = 0;

    if (nFlag & 4)
    {
        pSprite->pal = 4;
        pSprite->shade = -64;
    }
    else
    {
        pSprite->pal = 0;
        pSprite->shade = -12;
    }

    pSprite->clipdist = 10;
    pSprite->xrepeat = nRepeat;
    pSprite->yrepeat = nRepeat;
    pSprite->picnum = 1;
    pSprite->ang = 0;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->backuppos();

    // CHECKME - where is hitag set otherwise?
    if (pSprite->statnum < 900) {
        pSprite->hitag = -1;
    }

    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->owner = -1;
    pSprite->extra = runlist_AddRunRec(pSprite->lotag - 1, pActor, 0x100000);

    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x100000);
    pActor->nAction = nFlag;
    pActor->nIndex = 0;
    pActor->nIndex2 = SeqOffsets[val] + val2;
    pActor->pTarget = nullptr;
    pActor->nDamage = pActor->nRun;
    pActor->nPhase = ITEM_MAGIC;

    if (nFlag & 0x80) {
        pSprite->cstat |= 0x2; // set transluscence
    }

    return pActor;
}

void AIAnim::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    short nIndex2 = pActor->nIndex2;
    auto pSprite = &pActor->s();

    short nIndex = pActor->nIndex;

    if (!(pSprite->cstat & 0x8000))
    {
        seq_MoveSequence(pActor, nIndex2, nIndex);
    }

    if (pSprite->statnum == kStatIgnited)
    {
        auto pIgniter = pActor->pTarget;

        if (pIgniter)
        {
            auto pSpriteB = &pIgniter->s(); 
            pSprite->x = pSpriteB->x;
            pSprite->y = pSpriteB->y;
            pSprite->z = pSpriteB->z;

            if (pSpriteB->sectnum != pSprite->sectnum)
            {
                if (pSpriteB->sectnum < 0 || pSpriteB->sectnum >= kMaxSectors)
                {
                    DestroyAnim(pActor);
                    return;
                }
                else
                {
                    ChangeActorSect(pActor, pSpriteB->sectnum);
                }
            }

            if (!nIndex)
            {
                if (pSpriteB->cstat != 0x8000)
                {
                    short hitag2 = pSpriteB->hitag;
                    pSpriteB->hitag--;

                    if (hitag2 >= 15)
                    {
                        runlist_DamageEnemy(pIgniter, nullptr, (pSpriteB->hitag - 14) * 2);

                        if (pSpriteB->shade < 100)
                        {
                            pSpriteB->pal = 0;
                            pSpriteB->shade++;
                        }

                        if (!(pSpriteB->cstat & 101))
                        {
		                    DestroyAnim(pActor);
                            return;
                        }
                    }
                    else
                    {
                        pSpriteB->hitag = 1;
	                    DestroyAnim(pActor);
                    }
                }
                else
                {
                    pSpriteB->hitag = 1;
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
            pSprite->cstat |= 2;
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
    short nIndex2 = pActor->nIndex2;

    seq_PlotSequence(ev->nParam, nIndex2, pActor->nIndex, 0x101);
    ev->pTSprite->owner = -1;
}

void BuildExplosion(DExhumedActor* pActor)
{
    auto pSprite = &pActor->s();
 
    int nSector = pSprite->sectnum;

    int edx = 36;

    if (SectFlag[nSector] & kSectUnderwater)
    {
        edx = 75;
    }
    else if (pSprite->z == pSprite->sector()->floorz)
    {
        edx = 34;
    }

    BuildAnim(nullptr, edx, 0, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, pSprite->xrepeat, 4);
}

void BuildSplash(DExhumedActor* actor, int nSector)
{
    auto pSprite = &actor->s();
    int nRepeat, nSound;

    if (pSprite->statnum != 200)
    {
        nRepeat = pSprite->xrepeat + (RandomWord() % pSprite->xrepeat);
        nSound = kSound0;
    }
    else
    {
        nRepeat = 20;
        nSound = kSound1;
    }

    int bIsLava = SectFlag[nSector] & kSectLava;

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

    auto pActor = BuildAnim(nullptr, edx, 0, pSprite->x, pSprite->y, sector[nSector].floorz, nSector, nRepeat, nFlag);

    if (!bIsLava)
    {
        D3PlayFX(StaticSound[nSound] | 0xa00, pActor);
    }
}
END_PS_NS
