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
#include "player.h"

BEGIN_PS_NS

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DestroyBubble(DExhumedActor* pActor)
{
    runlist_DoSubRunRec(pActor->spr.lotag - 1); 
    runlist_DoSubRunRec(pActor->spr.intowner);
    runlist_SubRunRec(pActor->nRun);
    DeleteActor(pActor);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static DExhumedActor* BuildBubble(const DVector3& pos, sectortype* pSector, const int nPlayer = nLocalPlayer)
{
    int nSize = RandomSize(3);
    if (nSize > 4)
        nSize -= 4;

    // Was inita global previously.
    const auto nAngle = PlayerList[nPlayer].GetActor()->spr.Angles.Yaw;
    const auto pActor = insertActor(pSector, 402);

	pActor->spr.pos = pos;
    pActor->spr.cstat = 0;
    pActor->spr.shade = -32;
    pActor->spr.pal = 0;
	pActor->clipdist = 1.25;
    pActor->spr.scale = DVector2(0.625, 0.625);
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    setvalidpic(pActor);
    pActor->spr.Angles.Yaw = nAngle;
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = -1200 / 256.;
    pActor->spr.hitag = -1;
    pActor->spr.extra = -1;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->backuppos();

//	GrabTimeSlot(3);

    pActor->nFrame = 0;
    pActor->nSeqFile = "bubble";
    pActor->nSeqIndex = nSize;
    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x140000);
    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x140000);

    return pActor;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIBubble::Tick(RunListEvent* ev)
{
    const auto pActor = ev->pObjActor;
    if (!pActor) return;

    const auto bubbSeq = getSequence(pActor->nSeqFile, pActor->nSeqIndex);

    bubbSeq->frames[pActor->nFrame].playSound(pActor);

    pActor->nFrame++;

    if (pActor->nFrame >= bubbSeq->frames.Size())
        pActor->nFrame = 0;

    pActor->spr.pos.Z = pActor->vel.Z;

    const auto pSector = pActor->sector();

    if (pActor->spr.pos.Z <= pSector->ceilingz)
    {
        auto pSectAbove = pSector->pAbove;

        if (pActor->spr.hitag > -1 && pSectAbove != nullptr)
            BuildAnim(nullptr, "seebubbl", 0, DVector3(pActor->spr.pos.XY(), pSectAbove->floorz), pSectAbove, 1., 0);

        DestroyBubble(pActor);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIBubble::Draw(RunListEvent* ev)
{
    if (const auto pActor = ev->pObjActor)
    {
        seq_PlotSequence(ev->nParam, pActor->nSeqFile, pActor->nSeqIndex, pActor->nFrame, 1);
        ev->pTSprite->ownerActor = nullptr;
    }
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoBubbleMachines()
{
    ExhumedStatIterator it(kStatBubbleMachine);
    while (const auto itActor = it.Next())
    {
        itActor->nCount--;

        if (itActor->nCount <= 0)
        {
            itActor->nCount = (RandomWord() % itActor->nFrame) + 30;
            BuildBubble(itActor->spr.pos, itActor->sector());
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void BuildBubbleMachine(DExhumedActor* pActor)
{
    pActor->nFrame = 75;
    pActor->nCount = pActor->nFrame;
    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    ChangeActorStat(pActor, kStatBubbleMachine);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoBubbles(int nPlayer)
{
    sectortype* pSector;
    const auto pos = WheresMyMouth(nPlayer, &pSector);
    const auto pActor = BuildBubble(pos, pSector, nPlayer);
    pActor->spr.hitag = nPlayer;
}
END_PS_NS
