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
	auto pSprite = &pActor->s();

    runlist_DoSubRunRec(pSprite->lotag - 1); 
    runlist_DoSubRunRec(pSprite->owner);
    runlist_SubRunRec(pActor->nRun);
    DeleteActor(pActor);
}

DExhumedActor* BuildBubble(vec3_t pos, short nSector)
{
    int nSize = RandomSize(3);
    if (nSize > 4) {
        nSize -= 4;
    }

    auto pActor = insertActor(nSector, 402);
	auto pSprite = &pActor->s();

    pSprite->pos = pos;
    pSprite->cstat = 0;
    pSprite->shade = -32;
    pSprite->pal = 0;
    pSprite->clipdist = 5;
    pSprite->xrepeat = 40;
    pSprite->yrepeat = 40;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->picnum = 1;
    pSprite->ang = inita;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = -1200;
    pSprite->hitag = -1;
    pSprite->extra = -1;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->backuppos();

//	GrabTimeSlot(3);

    pActor->nFrame = 0;
    pActor->nIndex = SeqOffsets[kSeqBubble] + nSize;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, pActor, 0x140000);

    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x140000);
    return pActor;
}

void AIBubble::Tick(RunListEvent* ev) 
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    short nSeq = pActor->nIndex;
    auto pSprite = &pActor->s();

    seq_MoveSequence(pActor, nSeq, pActor->nFrame);

    pActor->nFrame++;

    if (pActor->nFrame >= SeqSize[nSeq]) {
        pActor->nFrame = 0;
    }

    pSprite->z += pSprite->zvel;

    short nSector = pSprite->sectnum;

    if (pSprite->z <= sector[nSector].ceilingz)
    {
        short nSectAbove = SectAbove[nSector];

        if (pSprite->hitag > -1 && nSectAbove != -1) {
            BuildAnim(nullptr, 70, 0, pSprite->x, pSprite->y, sector[nSectAbove].floorz, nSectAbove, 64, 0);
        }

        DestroyBubble(pActor);
    }
}

void AIBubble::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    seq_PlotSequence(ev->nParam, pActor->nIndex, pActor->nFrame, 1);
    ev->pTSprite->owner = -1;
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

			auto pSprite = &pActor->s();
            BuildBubble(pSprite->pos, pSprite->sectnum);
        }
    }
}

void BuildBubbleMachine(DExhumedActor* pActor)
{
    pActor->nFrame = 75;
    pActor->nCount = pActor->nFrame;

	auto pSprite = &pActor->s();
    pSprite->cstat = 0x8000;
    ChangeActorStat(pActor, kStatBubbleMachine);
}

void DoBubbles(int nPlayer)
{
    vec3_t pos;
    short nSector;

    WheresMyMouth(nPlayer, &pos, &nSector);

    auto pActor = BuildBubble(pos, nSector);
    pActor->s().hitag = nPlayer;
}
END_PS_NS
