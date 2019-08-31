
#include "bubbles.h"
#include "runlist.h"
#include "exhumed.h"
#include "random.h"
#include "engine.h"
#include "sequence.h"
#include "move.h"
#include "init.h"
#include "runlist.h"
#include "init.h"
#include "anims.h"
#include <assert.h>

#define kMaxBubbles		200
#define kMaxMachines	125

struct Bubble
{
    short _0;
    short _2;
    short nSprite;
    short _6;
};

struct machine
{
    short _0;
    short nSprite;
    short _4;
};

short BubbleCount = 0;

short nFreeCount;
short nMachineCount;

uint8_t nBubblesFree[kMaxBubbles];
machine Machine[kMaxMachines];
Bubble BubbleList[kMaxBubbles];


void InitBubbles()
{
    BubbleCount = 0;
    nMachineCount = 0;

    for (int i = 0; i < kMaxBubbles; i++) {
        nBubblesFree[i] = i;
    }

    nFreeCount = 0;
}

void DestroyBubble(short nBubble)
{
    short nSprite = BubbleList[nBubble].nSprite;

    runlist_DoSubRunRec(sprite[nSprite].lotag - 1);
    runlist_DoSubRunRec(sprite[nSprite].owner);
    runlist_SubRunRec(BubbleList[nBubble]._6);

    mydeletesprite(nSprite);

    nBubblesFree[nFreeCount] = nBubble;

    nFreeCount++;
}

short GetBubbleSprite(short nBubble)
{
    return BubbleList[nBubble].nSprite;
}

int BuildBubble(int x, int y, int z, short nSector)
{
    int nSize = RandomSize(3);
    if (nSize > 4) {
        nSize -= 4;
    }

    if (nFreeCount <= 0) {
        return -1;
    }

    nFreeCount--;

    uint8_t nBubble = nBubblesFree[nFreeCount];

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

//	GrabTimeSlot(3);

    BubbleList[nBubble].nSprite = nSprite;
    BubbleList[nBubble]._0 = 0;
    BubbleList[nBubble]._2 = SeqOffsets[kSeqBubble] + nSize;
    
    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nBubble | 0x140000);

    BubbleList[nBubble]._6 = runlist_AddRunRec(NewRun, nBubble | 0x140000);
    return nBubble | 0x140000;
}

void FuncBubble(int a, int b, int nRun)
{
    short nBubble = RunData[nRun].nVal;
    assert(nBubble >= 0 && nBubble < kMaxBubbles);

    short nSprite = BubbleList[nBubble].nSprite;
    short dx = BubbleList[nBubble]._2;

    int nMessage = a & 0x7F0000;

    switch (nMessage)
    {
        case 0x20000:
        {
            seq_MoveSequence(nSprite, dx, BubbleList[nBubble]._0);

            BubbleList[nBubble]._0++;

            if (BubbleList[nBubble]._0 >= SeqSize[dx]) {
                BubbleList[nBubble]._0 = 0;
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
            seq_PlotSequence(a & 0xFFFF, dx, BubbleList[nBubble]._0, 1);
            tsprite[a & 0xFFFF].owner = -1;
            return;
        }

        case 0x80000:
        case 0xA0000:
            return;

        default:
            DebugOut("unknown msg %d for Bubble\n", nMessage);
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
        bail2dos("too many bubble machines in level %d\n", levelnew);
    }

    Machine[nMachineCount]._4 = 75;
    Machine[nMachineCount].nSprite = nSprite;
    Machine[nMachineCount]._0 = Machine[nMachineCount]._4;
    nMachineCount++;

    sprite[nSprite].cstat = 0x8000u;
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
