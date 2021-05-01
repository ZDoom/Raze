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

    runlist_DoSubRunRec(sprite[nSprite].lotag - 1);
    runlist_DoSubRunRec(sprite[nSprite].owner);
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

    sprite[nSprite].x = x;
    sprite[nSprite].y = y;
    sprite[nSprite].z = z;
    sprite[nSprite].cstat = 0;
    sprite[nSprite].shade = -32;
    sprite[nSprite].pal = 0;
    sprite[nSprite].clipdist = 5;
    sprite[nSprite].xrepeat = 40;
    sprite[nSprite].yrepeat = 40;
    sprite[nSprite].xoffset = 0;
    sprite[nSprite].yoffset = 0;
    sprite[nSprite].picnum = 1;
    sprite[nSprite].ang = inita;
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = -1200;
    sprite[nSprite].hitag = -1;
    sprite[nSprite].extra = -1;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].backuppos();

//	GrabTimeSlot(3);

    BubbleList[nBubble].nSprite = nSprite;
    BubbleList[nBubble].nFrame = 0;
    BubbleList[nBubble].nSeq = SeqOffsets[kSeqBubble] + nSize;

    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nBubble | 0x140000);

    BubbleList[nBubble].nRun = runlist_AddRunRec(NewRun, nBubble | 0x140000);
    return nBubble | 0x140000;
}

void FuncBubble(int a, int, int nRun)
{
    short nBubble = RunData[nRun].nVal;
    assert(nBubble >= 0 && nBubble < kMaxBubbles);

    short nSprite = BubbleList[nBubble].nSprite;
    short nSeq = BubbleList[nBubble].nSeq;

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

            sprite[nSprite].z += sprite[nSprite].zvel;

            short nSector = sprite[nSprite].sectnum;

            if (sprite[nSprite].z <= sector[nSector].ceilingz)
            {
                short nSectAbove = SectAbove[nSector];

                if (sprite[nSprite].hitag > -1 && nSectAbove != -1) {
                    BuildAnim(-1, 70, 0, sprite[nSprite].x, sprite[nSprite].y, sector[nSectAbove].floorz, nSectAbove, 64, 0);
                }

                DestroyBubble(nBubble);
            }

            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, nSeq, BubbleList[nBubble].nFrame, 1);
            tsprite[a & 0xFFFF].owner = -1;
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
            BuildBubble(sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum);
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

    sprite[nSprite].cstat = 0x8000;
}

void DoBubbles(int nPlayer)
{
    int x, y, z;
    short nSector;

    WheresMyMouth(nPlayer, &x, &y, &z, &nSector);

    int nBubble = BuildBubble(x, y, z, nSector);
    int nSprite = GetBubbleSprite(nBubble);

    sprite[nSprite].hitag = nPlayer;
}
END_PS_NS
