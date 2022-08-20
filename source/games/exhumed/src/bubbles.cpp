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
#include "exhumed.h"
#include "engine.h"
#include "sequence.h"
#include "mapinfo.h"
#include <assert.h>

BEGIN_PS_NS

void DestroyBubble(DExhumedActor* pActor)
{
    runlist_DoSubRunRec(pActor->spr.lotag - 1); 
    runlist_DoSubRunRec(pActor->spr.intowner);
    runlist_SubRunRec(pActor->nRun);
    DeleteActor(pActor);
}

DExhumedActor* BuildBubble(const DVector3& pos, sectortype* pSector)
{
    int nSize = RandomSize(3);
    if (nSize > 4) {
        nSize -= 4;
    }

    auto pActor = insertActor(pSector, 402);

	pActor->spr.pos = pos;
    pActor->spr.cstat = 0;
    pActor->spr.shade = -32;
    pActor->spr.pal = 0;
    pActor->spr.clipdist = 5;
    pActor->spr.xrepeat = 40;
    pActor->spr.yrepeat = 40;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.picnum = 1;
    pActor->set_int_ang(inita);
    pActor->spr.xvel = 0;
    pActor->spr.yvel = 0;
    pActor->spr.zvel = -1200;
    pActor->spr.hitag = -1;
    pActor->spr.extra = -1;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->backuppos();

//	GrabTimeSlot(3);

    pActor->nFrame = 0;
    pActor->nIndex = SeqOffsets[kSeqBubble] + nSize;

    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x140000);

    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x140000);
    return pActor;
}

void AIBubble::Tick(RunListEvent* ev) 
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    int nSeq = pActor->nIndex;

    seq_MoveSequence(pActor, nSeq, pActor->nFrame);

    pActor->nFrame++;

    if (pActor->nFrame >= SeqSize[nSeq]) {
        pActor->nFrame = 0;
    }

    pActor->add_int_z(pActor->spr.zvel);

    auto pSector = pActor->sector();

    if (pActor->spr.pos.Z <= pSector->ceilingz)
    {
        auto pSectAbove = pSector->pAbove;

        if (pActor->spr.hitag > -1 && pSectAbove != nullptr) {
            BuildAnim(nullptr, 70, 0, DVector3(pActor->spr.pos.XY(), pSectAbove->floorz), pSectAbove, 64, 0);
        }

        DestroyBubble(pActor);
    }
}

void AIBubble::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    seq_PlotSequence(ev->nParam, pActor->nIndex, pActor->nFrame, 1);
    ev->pTSprite->ownerActor = nullptr;
}


void DoBubbleMachines()
{
    ExhumedStatIterator it(kStatBubbleMachine);
    while (auto pActor = it.Next())
    {
        pActor->nCount--;

        if (pActor->nCount <= 0)
        {
            pActor->nCount = (RandomWord() % pActor->nFrame) + 30;

            BuildBubble(pActor->spr.pos, pActor->sector());
        }
    }
}

void BuildBubbleMachine(DExhumedActor* pActor)
{
    pActor->nFrame = 75;
    pActor->nCount = pActor->nFrame;

    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    ChangeActorStat(pActor, kStatBubbleMachine);
}

void DoBubbles(int nPlayer)
{
    sectortype* pSector;

    auto pos = WheresMyMouth(nPlayer, &pSector);

    auto pActor = BuildBubble(pos, pSector);
    pActor->spr.hitag = nPlayer;
}
END_PS_NS
