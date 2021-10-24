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


enum
{
	kMaxBubbles		= 200,
	kMaxMachines	= 125
};

struct Bubble
{
    short nFrame;
    short nSeq;
    short nSprite;
    short nRunIndex;
};

struct machine
{
    short nCount;
    short nSprite;
    short nFrame;
};

short nMachineCount;
machine Machine[kMaxMachines];

FreeListArray<Bubble, kMaxBubbles> BubbleList;

FSerializer& Serialize(FSerializer& arc, const char* keyname, Bubble& w, Bubble* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("seq", w.nSeq)
            ("frame", w.nFrame)
            ("run", w.nRunIndex)
            ("sprite", w.nSprite)
            .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, machine& w, machine* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at0", w.nCount)
            ("at4", w.nFrame)
            ("sprite", w.nSprite)
            .EndObject();
    }
    return arc;
}

void SerializeBubbles(FSerializer& arc)
{
    if (arc.BeginObject("bubbles"))
    {
        arc ("machinecount", nMachineCount)
            ("list", BubbleList)
            .Array("machines", Machine, nMachineCount)
            .EndObject();
    }
}

void InitBubbles()
{
    nMachineCount = 0;
    BubbleList.Clear();
}

void DestroyBubble(short nBubble)
{
    short nSprite = BubbleList[nBubble].nSprite;
	auto pSprite = &sprite[nSprite];

    runlist_DoSubRunRec(pSprite->lotag - 1);
    runlist_DoSubRunRec(pSprite->owner);
    runlist_SubRunRec(BubbleList[nBubble].nRunIndex);

    mydeletesprite(nSprite);

    BubbleList.Release(nBubble);
}

short GetBubbleSprite(int nBubble)
{
    return BubbleList[nBubble & 0xffff].nSprite;
}

int BuildBubble(int x, int y, int z, short nSector)
{
    int nSize = RandomSize(3);
    if (nSize > 4) {
        nSize -= 4;
    }

    int nBubble = BubbleList.Get();
    if (nBubble < 0) {
        return -1;
    }

    int nSprite = insertsprite(nSector, 402);
    assert(nSprite >= 0 && nSprite < kMaxSprites);
	auto pSprite = &sprite[nSprite];

    pSprite->x = x;
    pSprite->y = y;
    pSprite->z = z;
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

    BubbleList[nBubble].nSprite = nSprite;
    BubbleList[nBubble].nFrame = 0;
    BubbleList[nBubble].nSeq = SeqOffsets[kSeqBubble] + nSize;

    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, nBubble, 0x140000);

    BubbleList[nBubble].nRunIndex = runlist_AddRunRec(NewRun, nBubble, 0x140000);
    return nBubble | 0x140000;
}

void AIBubble::Tick(RunListEvent* ev) 
{
    short nBubble = RunData[ev->nRun].nObjIndex;
    assert(nBubble >= 0 && nBubble < kMaxBubbles);

    short nSprite = BubbleList[nBubble].nSprite;
    short nSeq = BubbleList[nBubble].nSeq;
    auto pSprite = &sprite[nSprite];

    seq_MoveSequence(nSprite, nSeq, BubbleList[nBubble].nFrame);

    BubbleList[nBubble].nFrame++;

    if (BubbleList[nBubble].nFrame >= SeqSize[nSeq]) {
        BubbleList[nBubble].nFrame = 0;
    }

    pSprite->z += pSprite->zvel;

    short nSector = pSprite->sectnum;

    if (pSprite->z <= sector[nSector].ceilingz)
    {
        short nSectAbove = SectAbove[nSector];

        if (pSprite->hitag > -1 && nSectAbove != -1) {
            BuildAnim(nullptr, 70, 0, pSprite->x, pSprite->y, sector[nSectAbove].floorz, nSectAbove, 64, 0);
        }

        DestroyBubble(nBubble);
    }
}

void AIBubble::Draw(RunListEvent* ev)
{
    short nBubble = RunData[ev->nRun].nObjIndex;
    assert(nBubble >= 0 && nBubble < kMaxBubbles);

    seq_PlotSequence(ev->nParam, BubbleList[nBubble].nSeq, BubbleList[nBubble].nFrame, 1);
    ev->pTSprite->owner = -1;
}

void  FuncBubble(int nObject, int nMessage, int nDamage, int nRun)
{
    AIBubble ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void DoBubbleMachines()
{
    for (int i = 0; i < nMachineCount; i++)
    {
        Machine[i].nCount--;

        if (Machine[i].nCount <= 0)
        {
            Machine[i].nCount = (RandomWord() % Machine[i].nFrame) + 30;

            int nSprite = Machine[i].nSprite;
			auto pSprite = &sprite[nSprite];
            BuildBubble(pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum);
        }
    }
}

void BuildBubbleMachine(int nSprite)
{
    if (nMachineCount >= kMaxMachines) {
        I_Error("too many bubble machines in level %s\n", currentLevel->labelName.GetChars());
        exit(-1);
    }

    Machine[nMachineCount].nFrame = 75;
    Machine[nMachineCount].nSprite = nSprite;
    Machine[nMachineCount].nCount = Machine[nMachineCount].nFrame;
    nMachineCount++;

	auto pSprite = &sprite[nSprite];
    pSprite->cstat = 0x8000;
}

void DoBubbles(int nPlayer)
{
    int x, y, z;
    short nSector;

    WheresMyMouth(nPlayer, &x, &y, &z, &nSector);

    int nBubble = BuildBubble(x, y, z, nSector);
    int nSprite = GetBubbleSprite(nBubble);
	auto pSprite = &sprite[nSprite];

    pSprite->hitag = nPlayer;
}
END_PS_NS
