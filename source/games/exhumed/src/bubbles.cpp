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
    short nRun;
};

struct machine
{
    short _0;
    short nSprite;
    short _4;
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
            ("run", w.nRun)
            ("sprite", w.nSprite)
            .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, machine& w, machine* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at0", w._0)
            ("at4", w._4)
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
    runlist_SubRunRec(BubbleList[nBubble].nRun);

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

    BubbleList[nBubble].nRun = runlist_AddRunRec(NewRun, nBubble, 0x140000);
    return nBubble | 0x140000;
}

void FuncBubble(int a, int, int nRun)
{
    short nBubble = RunData[nRun].nVal;
    assert(nBubble >= 0 && nBubble < kMaxBubbles);

    short nSprite = BubbleList[nBubble].nSprite;
    short nSeq = BubbleList[nBubble].nSeq;
	auto pSprite = &sprite[nSprite];

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        case 0x20000:
        {
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
                    BuildAnim(-1, 70, 0, pSprite->x, pSprite->y, sector[nSectAbove].floorz, nSectAbove, 64, 0);
                }

                DestroyBubble(nBubble);
            }

            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, nSeq, BubbleList[nBubble].nFrame, 1);
            mytsprite[a & 0xFFFF].owner = -1;
            return;
        }

        case 0x80000:
        case 0xA0000:
            return;

        default:
            Printf("unknown msg %d for Bubble\n", nMessage);
            return;
    }
}

void DoBubbleMachines()
{
    for (int i = 0; i < nMachineCount; i++)
    {
        Machine[i]._0--;

        if (Machine[i]._0 <= 0)
        {
            Machine[i]._0 = (RandomWord() % Machine[i]._4) + 30;

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

    Machine[nMachineCount]._4 = 75;
    Machine[nMachineCount].nSprite = nSprite;
    Machine[nMachineCount]._0 = Machine[nMachineCount]._4;
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
