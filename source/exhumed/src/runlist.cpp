
#include "exhumed.h"
#include "engine.h"
#include "runlist.h"
#include "player.h"
#include "trigdat.h"
#include "move.h"
#include "random.h"
#include "mummy.h"
#include "fish.h"
#include "lion.h"
#include "rex.h"
#include "set.h"
#include "rat.h"
#include "wasp.h"
#include "anubis.h"
#include "snake.h"
#include "scorp.h"
#include "ra.h"
#include "spider.h"
#include "bullet.h"
#include "queen.h"
#include "roach.h"
#include "bubbles.h"
#include "lavadude.h"
#include "grenade.h"
#include "object.h"
#include "switch.h"
#include "anims.h"
#include "sound.h"
#include "init.h"
#include "lighting.h"
#include <assert.h>

//#define kFuncMax	0x260000 // the number 38 stored in the high word of an int
#define kFuncMax		39
#define kMaxRunStack	200


short RunCount   = -1;
short nRadialSpr = -1;
short nStackCount =  0;
short word_966BE =  0;
short ChannelList = -1;
short ChannelLast = -1;

int nRadialOwner;
int nDamageRadius;
int nRadialDamage;
short RunChain;
short NewRun;

int sRunStack[kMaxRunStack];
short RunFree[kMaxRuns];
RunChannel sRunChannels[kMaxChannels];


RunStruct RunData[kMaxRuns];

short word_96760 = 0;

/* variables 
  Name:  _sRunStack
  Name:  _RunFree
  Name:  _channel
  Name:  _RunData
  Name:  _nRadialOwner
  Name:  _nDamageRadius
  Name:  _nRadialDamage
  Name:  _RunCount
  Name:  _nRadialSpr
  Name:  _nStackCount
*/

AiFunc aiFunctions[kFuncMax] = {
    FuncElev,
    FuncSwReady,
    FuncSwPause,
    FuncSwStepOn,
    FuncSwNotOnPause,
    FuncSwPressSector,
    FuncSwPressWall,
    FuncWallFace,
    FuncSlide,
    FuncAnubis,
    FuncPlayer, // 10
    FuncBullet,
    FuncSpider,
    FuncCreatureChunk,
    FuncMummy,
    FuncGrenade,
    FuncAnim,
    FuncSnake,
    FuncFish,
    FuncLion,
    FuncBubble, // 20
    FuncLava,
    FuncLavaLimb,
    FuncObject,
    FuncRex,
    FuncSet,
    FuncQueen,
    FuncQueenHead,
    FuncRoach,
    FuncQueenEgg,
    FuncWasp, // 30
    FuncTrap,
    FuncFishLimb,
    FuncRa, // 33
    FuncScorp,
    FuncSoul,
    FuncRat,
    FuncEnergyBlock,
    FuncSpark,
};



int runlist_GrabRun()
{
    assert(RunCount > 0 && RunCount <= kMaxRuns);

    RunCount--;

    return RunFree[RunCount];
}

int runlist_FreeRun(int nRun)
{
    assert(RunCount >= 0 && RunCount < kMaxRuns);
    assert(nRun >= 0 && nRun < kMaxRuns);

    RunFree[RunCount] = nRun;
    RunCount++;

    RunData[nRun]._6 = -1;
    RunData[nRun].nMoves = -1;
    RunData[nRun]._4 = RunData[nRun]._6;

    return 1;
}

// done
int runlist_HeadRun()
{
    int nRun = runlist_GrabRun();

    RunData[nRun]._4 = -1;
    RunData[nRun]._6 = -1;

    return nRun;
}

// sub 4
void runlist_InitRun()
{
    int i;

    RunCount = kMaxRuns;
    nStackCount = 0;

    for (i = 0; i < kMaxRuns; i++)
    {
        RunData[i].nMoves = -1;
        RunData[i]._6 = -1;
        RunFree[i] = i;
        RunData[i]._4 = -1;
    }

    int nRun = runlist_HeadRun();
    RunChain = nRun;
    NewRun = nRun;

    for (i = 0; i < kMaxPlayers; i++) {
        PlayerList[i].nRun = -1;
    }

    nRadialSpr = -1;
}

int runlist_UnlinkRun(int nRun)
{
    assert(nRun >= 0 && nRun < kMaxRuns);

    if (nRun == RunChain)
    {
        RunChain = RunData[nRun]._4;
        return nRun;
    }

    if (RunData[nRun]._6 >= 0)
    {
        RunData[RunData[nRun]._6]._4 = RunData[nRun]._4;
    }

    if (RunData[nRun]._4 >= 0)
    {
        RunData[RunData[nRun]._4]._6 = RunData[nRun]._6;
    }

    return nRun;
}

// done ?
int runlist_InsertRun(int RunLst, int RunNum)
{
    assert(RunLst >= 0 && RunLst < kMaxRuns);
    assert(RunNum >= 0 && RunNum < kMaxRuns);

    RunData[RunNum]._6 = RunLst;
    int val = RunData[RunLst]._4;
    RunData[RunNum]._4 = val;

    if (val >= 0) {
        RunData[val]._6 = RunNum;
    }

    RunData[RunLst]._4 = RunNum;
    return RunNum;
}

// done
int runlist_AddRunRec(int a, int b)
{
    int nRun = runlist_GrabRun();

    RunData[nRun].nMoves = b; // TODO - split this into the two shorts?

    runlist_InsertRun(a, nRun);
    return nRun;
}

void runlist_DoSubRunRec(int RunPtr)
{
    assert(RunPtr >= 0 && RunPtr < kMaxRuns);

    runlist_UnlinkRun(RunPtr);
    runlist_FreeRun(RunPtr);
}

void runlist_CleanRunRecs()
{
    int nextPtr = RunChain;

    if (nextPtr >= 0)
    {
        assert(nextPtr < kMaxRuns);
        nextPtr = RunData[nextPtr]._4;
    }

    while (nextPtr >= 0)
    {
        int runPtr = nextPtr;
        assert(runPtr < kMaxRuns);

        int val = RunData[runPtr].nRef; // >> 16;
        nextPtr = RunData[runPtr]._4;

        if (val < 0) {
            runlist_DoSubRunRec(runPtr);
        }
    }
}

// done
void runlist_SubRunRec(int RunPtr)
{
    assert(RunPtr >= 0 && RunPtr < kMaxRuns);

    RunData[RunPtr].nMoves = -totalmoves;
}

void runlist_SendMessageToRunRec(int nRun, int edx, int nDamage)
{
    int nFunc = RunData[nRun].nRef;// >> 16;

    if (nFunc < 0) {
        return;
    }

    assert(nFunc >= 0 && nFunc <= kFuncMax);

    if (nFunc > kFuncMax) {
        return;
    }

    assert(nFunc < kFuncMax); // REMOVE
    
    // do function pointer call here.
    aiFunctions[nFunc](edx, nDamage, nRun);
}

void runlist_ExplodeSignalRun()
{
    short nextPtr = RunChain;

    if (nextPtr >= 0)
    {
        assert(nextPtr < kMaxRuns);
        nextPtr = RunData[nextPtr]._4;
    }

    // LOOP
    while (nextPtr >= 0)
    {
        int runPtr = nextPtr;
        assert(runPtr < kMaxRuns);

        int val = RunData[runPtr].nMoves;
        nextPtr = RunData[runPtr]._4;

        if (val >= 0)
        {
            runlist_SendMessageToRunRec(runPtr, 0xA0000, 0);
        }
    }
}

void runlist_PushMoveRun(int eax)
{
    if (nStackCount < kMaxRunStack)
    {
        sRunStack[nStackCount] = eax;
        nStackCount++;
    }
}

int runlist_PopMoveRun()
{
    if (nStackCount <= 0) {
        bail2dos("PopMoveRun() called inappropriately\n");
    }

    nStackCount--;
    return sRunStack[nStackCount];
}

void runlist_SignalRun(int NxtPtr, int edx)
{
    if (NxtPtr == RunChain && word_966BE != 0) {
        runlist_PushMoveRun(edx);
        return;
    }

    while (1)
    {
        word_966BE = 1;

        if (NxtPtr >= 0)
        {
            assert(NxtPtr < kMaxRuns);
            NxtPtr = RunData[NxtPtr]._4;
        }

        while (NxtPtr >= 0)
        {
            int RunPtr = NxtPtr;

            if (RunPtr >= 0)
            {
                assert(RunPtr < kMaxRuns);
                int val = RunData[RunPtr].nMoves;
                NxtPtr = RunData[RunPtr]._4;

                if (val >= 0) {
                    runlist_SendMessageToRunRec(RunPtr, edx, 0);
                }
            }
        }

        word_966BE = 0;
        if (nStackCount == 0) {
            return;
        }

        edx = runlist_PopMoveRun();
        NxtPtr = RunChain;
    }
}

void runlist_InitChan()
{
    ChannelList = -1;
    ChannelLast = -1;

    for (int i = 0; i < kMaxChannels; i++)
    {
        sRunChannels[i].c = -1;
        sRunChannels[i].a = runlist_HeadRun();
        sRunChannels[i].b = -1;
        sRunChannels[i].d = 0;
    }
}

void runlist_ChangeChannel(int eax, short dx)
{
    if (sRunChannels[eax].b < 0)
    {
        short nChannel = ChannelList;
        ChannelList = eax;
        sRunChannels[eax].b = nChannel;
    }

    sRunChannels[eax].c = dx;
    sRunChannels[eax].d |= 2;
}

void runlist_ReadyChannel(short eax)
{
    if (sRunChannels[eax].b < 0)
    {
        short nChannel = ChannelList;
        ChannelList = eax;
        sRunChannels[eax].b = nChannel;
    }

    sRunChannels[eax].d |= 1;
}

void runlist_ProcessChannels()
{
#if 1
    short v0; // di@1
    short v1; // si@1
    short *v2; // ebx@3
    short v3; // cx@3
    short v4; // cx@5
    int v5; // eax@11
    int result; // eax@13
    short b; // [sp+0h] [bp-1Ch]@3
    short d;

    do
    {
        v0 = -1;
        v1 = -1;

        while (ChannelList >= 0)
        {
            b = sRunChannels[ChannelList].b;
            d = sRunChannels[ChannelList].d;
            //v3 = v2[3];
            //b = v2[1];

            if (d & 2)
            {
                sRunChannels[ChannelList].d = d ^ 2;
                //v2[3] = v3 ^ 2;
                runlist_SignalRun(sRunChannels[ChannelList].a, ChannelList | 0x10000);
            }

//			v4 = v3 & 1;
            if (d & 1)
            {
                sRunChannels[ChannelList].d ^= 1;
//				*((_BYTE *)v2 + offsetof(RunChannel, d)) ^= 1u;
                runlist_SignalRun(sRunChannels[ChannelList].a, 0x30000);
            }

            if (sRunChannels[ChannelList].d)
            {
                if (v1 == -1)
                {
                    v1 = ChannelList;
                    v0 = ChannelList;
                }
                else
                {
                    v5 = v0;
                    v0 = ChannelList;
                    sRunChannels[v5].b = ChannelList;
                }
            }
            else
            {
                sRunChannels[ChannelList].b = -1;
            }
            ChannelList = b;
        }
        result = v1;
        ChannelList = v1;
    } while (v1 != -1);

#else
    int edi = -1;
    int esi = edi;

    while (1)
    {
        short nChannel = ChannelList;
        if (nChannel < 0)
        {
            ChannelList = esi;
            if (esi != -1)
            {
                edi = -1;
                esi = edi;
                continue;
            }
            else {
                return;
            }
        }

        short b = sRunChannels[nChannel].b;
        short d = sRunChannels[nChannel].d;

        if (d & 2)
        {
            sRunChannels[nChannel].d = d ^ 2;
            runlist_SignalRun(sRunChannels[nChannel].a, ChannelList | 0x10000);
        }

        if (d & 1)
        {
            sRunChannels[nChannel].d = d ^ 1;
            runlist_SignalRun(sRunChannels[nChannel].a, 0x30000);
        }

        if (sRunChannels[nChannel].d == 0)
        {
            sRunChannels[ChannelList].b = -1;
        }
        else
        {
            if (esi == -1)
            {
                esi = ChannelList;
                edi = esi;
            }
            else
            {
                sRunChannels[edi].b = ChannelList;
                edi = ChannelList;
            }
        }

        ChannelList = b;
    }
#endif
}

int runlist_FindChannel(short ax)
{
    for (int i = 0; i < kMaxChannels; i++)
    {
        if (sRunChannels[i].c == -1)
        {
            sRunChannels[i].c = ax;
            return i;
        }
    }

    return -1;
}

int runlist_AllocChannel(int a)
{
    if (a)
    {
        for (int i = 0; i < kMaxChannels; i++)
        {
            if (sRunChannels[i].c == a) {
                return i;
            }
        }
    }

    return runlist_FindChannel(a);
}

void runlist_ExecObjects()
{
    runlist_ProcessChannels();
    runlist_SignalRun(RunChain, 0x20000);
}

void runlist_ProcessSectorTag(int nSector, int lotag, int hitag)
{
    int zListA[8];
    int zListB[8];

    int _lotag = lotag;
    int nChannel = runlist_AllocChannel(hitag % 1000);
    assert(nChannel >= 0); // REMOVE

    int var_2C = 1000;
    int var_24 = (hitag / 1000) << 12;
    int var_18 = lotag / 1000;

    if (!var_18) {
        var_18 = 1;
    }

    var_18 <<= 2;

    _lotag = (lotag % 1000);
    int eax = _lotag - 1;

    switch (eax)
    {
        case 0: // Ceiling Doom door
        {
            /* 
                This function searches z-coordinates of neighboring sectors to find the
                closest (next) ceiling starting at the given z-coordinate (thez).
            */
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwPress = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, var_24);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwPress);

            int nSwPause = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwPause);
            return;
        }

        case 1: // Floor Doom door
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].ceilingz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwPress = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, var_24);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwPress);

            int nSwPause = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwPause);
            return;
        }

        case 2:
        case 3:
        case 18:
        case 19:
        case 21:
        case 28:
        case 29:
        case 45:
        case 46:
        case 59:
        case 64:
        case 65:
        case 66:
        case 68:
        case 71:
        case 72:
        case 73:
        case 75:
        case 76:
        case 77:
        case 78:
        case 80:
        case 81:
        case 82:
        case 83:
        case 84:
        {
            return;
        }

        case 4: // Permanent floor raise
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz + 1, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 5: // Touchplate floor lower, single
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 400, 400, 2, sector[nextSector].floorz, sector[nSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwStepOn(nChannel, BuildLink(2, 1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            sector[nSector].floorz = sector[nextSector].floorz;
            return;
        }

        case 6: // Touchplate floor lower, multiple
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            int nSwitch2 = BuildSwNotOnPause(nChannel, BuildLink(2, -1, 0), nSector, 8);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch2);
            return;
        }

        case 7: // Permanent floor lower
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 8: // Switch activated lift down
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 150);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 9: // Touchplate Floor Raise
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            int nSwitch2 = BuildSwNotOnPause(nChannel, BuildLink(2, -1, 0), nSector, 8);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch2);
            return;
        }

        case 10: // Permanent floor raise
        case 13: // Sector raise / lower
        case 37: // Sector raise / lower
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 11: // Switch activated lift up
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 150);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 12: // Bobbing floor
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwReady(nChannel, BuildLink(2, 1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 14: // Sector raise/lower
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 15: // Stuttering noise (floor makes noise)
        {
            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, var_18 * 100, 2, sector[nSector].ceilingz, sector[nSector].floorz - 8);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            int nSwitch2 = BuildSwReady(nChannel, BuildLink(2, -1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch2);
            return;
        }

        case 16: // Reserved?
        {
            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, var_18 * 100, 2, sector[nSector].ceilingz, sector[nSector].floorz - 8);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwReady(nChannel, BuildLink(2, -1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 17: // Raises floor AND lowers ceiling
        {
            int ebx = ((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz;

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 200, var_18 * 100, 2, sector[nSector].floorz, ebx);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int ebx2 = (((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz) - 8;

            int nElev2 = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, var_18 * 100, 2, sector[nSector].ceilingz, ebx2);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev2);

            int nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 20: // Touchplate
        {
            int nSwitch = BuildSwStepOn(nChannel, BuildLink(2, 1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 22: // Floor raise, Sychronize
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 32767, 200, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), var_18 * 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 23: // Ceiling door, channel trigger only
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 24:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 300);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 25:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 450);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 26:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 600);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 27:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 900);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 30: // Touchplate
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 0x7FFF, 0x7FFF, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 31:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 0x7FFF, 0x7FFF, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 32: // Ceiling Crusher
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(20, nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nextSector].ceilingz, sector[nSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 33: // Triggerable Ceiling Crusher(Inactive)
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(28, nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nextSector].ceilingz, sector[nSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 34: // Destructible sector
        case 35:
        {
            nEnergyTowers++;

            int nEnergyBlock = BuildEnergyBlock(nSector);

            if (_lotag == 36) {
                nFinaleSpr = nEnergyBlock;
            }

            return;
        }

        case 36:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 38: // Touchplate
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 0x7FFF, 0x7FFF, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            int nSwitch2 = BuildSwNotOnPause(nChannel, BuildLink(2, -1, 0), nSector, 8);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch2);
            return;
        }

        case 39: // Moving sector(follows waypoints)
        {
            AddMovingSector(nSector, lotag, hitag % 1000, 2);
            return;
        }

        case 40: // Moving sector(follows waypoints)
        {
            AddMovingSector(nSector, lotag, hitag % 1000, 18);
            return;
        }

        case 41: // Moving sector(follows waypoints)
        {
            AddMovingSector(nSector, lotag, hitag % 1000, 58);
            return;
        }

        case 42: // Moving sector(follows waypoints)
        {
            AddMovingSector(nSector, lotag, hitag % 1000, 122);
            return;
        }

        case 43: // Moving sector(follows waypoints)
        {
            AddMovingSector(nSector, lotag, hitag % 1000, 90);
            return;
        }

        case 44: // Pushbox sector
        {
            CreatePushBlock(nSector);
            return;
        }

        case 47: // Ceiling lower
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, var_18 * 100, 2, sector[nSector].ceilingz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 48:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, var_18 * 100, 2, sector[nSector].ceilingz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 49: // Floor lower / raise
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 0x7FFF, 200, 2, sector[nextSector].floorz, sector[nSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 50:
        {
            int edx = ((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz;

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 200, var_18 * 100, 2, sector[nSector].floorz, edx);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int eax = (((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz) - 8;

            nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, var_18 * 100, 2, sector[nSector].ceilingz, eax);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwReady(nChannel, BuildLink(2, 1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 51:
        {
            int eax = ((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz;

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, eax, sector[nSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            eax = ((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz;

            nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, eax, sector[nSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, var_24);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 52:
        {
            int eax = ((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz;

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, eax, sector[nSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            eax = ((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz;

            nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, eax, sector[nSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 150);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 53:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, var_24);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 54:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, var_24);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 55:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 56:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].ceilingz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 57:
        {
            int nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, var_24);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            // Fall through to case 62
        }
        case 62:
        {
            if (_lotag == 63) {
                nEnergyChan = nChannel;
            }

            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 58:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            int nSwitch2 = BuildSwNotOnPause(nChannel, BuildLink(1, 1), nSector, 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch2);
            return;
        }

        case 60:
        {
            zListB[0] = sector[nSector].floorz;
            int var_1C = 1;

            while (1)
            {
                short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, -1);
                if (nextSector < 0 || var_1C >= 8) {
                    break;
                }

                zListB[var_1C] = sector[nextSector].floorz;

                var_1C++;
            }

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, var_1C,
                zListB[0], zListB[1], zListB[2], zListB[3], zListB[4], zListB[5], zListB[6], zListB[7]);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            return;
        }

        case 61:
        {
            zListA[0] = sector[nSector].floorz;
            int var_20 = 1;

            while (1)
            {
                short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
                if (nextSector < 0 || var_20 >= 8) {
                    break;
                }

                zListA[var_20] = sector[nextSector].floorz;

                var_20++;
            }

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, var_20,
                zListA[0], zListA[1], zListA[2], zListA[3], zListA[4], zListA[5], zListA[6], zListA[7]);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            return;
        }

        case 63:
        {
            int nSwitch = BuildSwStepOn(nChannel, BuildLink(2, 0, 0), nSector);
            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            return;
        }

        case 67:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            return;
        }

        case 69:
        case 70:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, (int)sector[nSector].floorz, (int)sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, var_24);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            int nSwitch2 = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch2);

            return;
        }

        case 74:
        {
            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), var_18 * 100, var_18 * 100, 2, (int)sector[nSector].ceilingz, (int)sector[nSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            return;
        }

        case 79:
        {
            SectFlag[nSector] |= 0x8000;
            return;
        }
    }
}

void runlist_ProcessWallTag(int nWall, short lotag, short hitag)
{
    int nChannel = runlist_AllocChannel(hitag % 1000);
    assert(nChannel >= 0 && nChannel < kMaxChannels);

    int var_18 = 0; // TODO - FIXME CHECKME. This doesn't seem to be initialised in the ASM?
    int var_28;

    int var_20 = 0; // TODO - FIXME CHECKME. This doesn't seem to be initialised in the ASM?
    int var_34;

    int var_14 = 0; // TODO - FIXME CHECKME. This doesn't seem to be initialised in the ASM?
    int var_24;

    int var_38 = 0; // TODO - FIXME CHECKME. This doesn't seem to be initialised in the ASM?
    int var_2C; 

    int eax = lotag / 1000;
    if (!eax) {
        eax = 1;
    }

    int edx = lotag % 1000;
    int edi = edx;
    eax <<= 2;

    switch (edx)
    {
        case 1:
        {
            int nWallFace = BuildWallFace(nChannel, nWall, 2, wall[nWall].picnum, wall[nWall].picnum + 1);
            runlist_AddRunRec(sRunChannels[nChannel].a, nWallFace);

            int nSwitch = BuildSwPressWall(nChannel, BuildLink(2, edi, 0), nWall);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 6:
        {
            int nSwitch = BuildSwPressWall(nChannel, BuildLink(2, 1, 0), nWall);
            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 7: // Regular switch
        {
            int nWallFace = BuildWallFace(nChannel, nWall, 2, wall[nWall].picnum, wall[nWall].picnum + 1);
            runlist_AddRunRec(sRunChannels[nChannel].a, nWallFace);

            int nSwitch = BuildSwPressWall(nChannel, BuildLink(1, 1), nWall);
            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 8:
        {
            int nWallFace = BuildWallFace(nChannel, nWall, 2, wall[nWall].picnum, wall[nWall].picnum + 1);
            runlist_AddRunRec(sRunChannels[nChannel].a, nWallFace);

            int nSwitch = BuildSwPressWall(nChannel, BuildLink(2, -1, 0), nWall);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 9: // Invisible switch
        {
            int nSwitch = BuildSwPressWall(nChannel, BuildLink(2, 1, 1), nWall);
            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 10:
        {
            int nSwitch = BuildSwPressWall(nChannel, BuildLink(2, -1, 0), nWall);
            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 12:
        {
            short nStart = nWall;

            while (1)
            {
                nWall = wall[nWall].point2;

                if (nStart == nWall) {
                    break;
                }
                
                var_28 = var_18;
                var_18 = nWall;
            }

            short nWall2 = wall[nStart].point2;
            short nWall3 = wall[nWall2].point2;
            short nWall4 = wall[nWall3].point2;

            int nSlide = BuildSlide(nChannel, nStart, var_18, var_28, nWall2, nWall3, nWall4);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSlide);
            return;
        }

        case 14:
        {
            short nStart = nWall;

            while (1)
            {
                nWall = wall[nWall].point2;

                if (nStart == nWall) {
                    break;
                }

                var_34 = var_20;
                var_20 = nWall;
            }

            short nWall2 = wall[nStart].point2;
            short nWall3 = wall[nWall2].point2;
            short nWall4 = wall[nWall3].point2;

            int nSlide = BuildSlide(nChannel, nStart, var_20, var_34, nWall2, nWall3, nWall4);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSlide);
            return;
        }

        case 16:
        {
            short nStart = nWall;

            while (1)
            {
                nWall = wall[nWall].point2;

                if (nStart == nWall) {
                    break;
                }

                var_24 = var_14;
                var_14 = nWall;
            }

            short nWall2 = wall[nStart].point2;
            short nWall3 = wall[nWall2].point2;
            short nWall4 = wall[nWall3].point2;

            int nSlide = BuildSlide(nChannel, nStart, var_14, var_24, nWall2, nWall3, nWall4);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSlide);
            return;
        }

        case 19:
        {
            short nStart = nWall;
            
            while (1)
            {
                nWall = wall[nWall].point2;

                if (nStart == nWall) {
                    break;
                }

                var_2C = var_38;
                var_38 = nWall;
            }

            short nWall2 = wall[nStart].point2;
            short nWall3 = wall[nWall2].point2;
            short nWall4 = wall[nWall3].point2;

            int nSlide = BuildSlide(nChannel, nStart, var_38, var_2C, nWall2, nWall3, nWall4);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSlide);
            return;
        }

        case 21:
        {
            return;
        }

        case 24:
        {
            AddFlow(nWall, eax, 3);
            return;
        }

        case 25:
        {
            AddFlow(nWall, eax, 2);
            return;
        }
    }
}

int runlist_CheckRadialDamage(short nSprite)
{
    if (nSprite == nRadialSpr) {
        return 0;
    }

    if (!(sprite[nSprite].cstat & 0x101)) {
        return 0;
    }

    if (sprite[nSprite].statnum >= kMaxStatus || sprite[nRadialSpr].statnum >= kMaxStatus) {
        return 0;
    }

    if (sprite[nSprite].statnum != 100 && nSprite == nRadialOwner) {
        return 0;
    }

    int x = (sprite[nSprite].x - sprite[nRadialSpr].x) >> 8;
    int y = (sprite[nSprite].y - sprite[nRadialSpr].y) >> 8;
    int z = (sprite[nSprite].z - sprite[nRadialSpr].z) >> 12;

    if (x < 0) {
        x = -x;
    }

    if (x > nDamageRadius) {
        return 0;
    }

    if (y < 0) {
        y = -y;
    }

    if (y > nDamageRadius) {
        return 0;
    }

    if (z < 0) {
        z = -z;
    }

    if (z > nDamageRadius) {
        return 0;
    }

    int edi = 0;

    int nDist = ksqrt(x * x + y * y);

    if (nDist < nDamageRadius)
    {
        uint16_t nCStat = sprite[nSprite].cstat;
        sprite[nSprite].cstat = 0x101;

        if (((kStatExplodeTarget - sprite[nSprite].statnum) <= 1) ||
            cansee(sprite[nRadialSpr].x,
                sprite[nRadialSpr].y,
                sprite[nRadialSpr].z - 512,
                sprite[nRadialSpr].sectnum,
                sprite[nSprite].x,
                sprite[nSprite].y,
                sprite[nSprite].z - 8192,
                sprite[nSprite].sectnum))
        {
            edi = (nRadialDamage * (nDamageRadius - nDist)) / nDamageRadius;

            if (edi < 0) {
                edi = 0;
            }
            else if (edi > 20)
            {
                int nAngle = GetMyAngle(x, y);
                sprite[nSprite].xvel += (short)((edi * Sin(nAngle + 512)) >> 3);
                sprite[nSprite].yvel += (short)((edi * Sin(nAngle)) >> 3);
                sprite[nSprite].zvel -= edi * 24;

                if (sprite[nSprite].zvel < -3584) {
                    sprite[nSprite].zvel = -3584;
                }
            }
        }

        sprite[nSprite].cstat = nCStat;
    }

    if (edi > 0x7FFF) {
        edi = 0x7FFF;
    }

    return edi;
}

void runlist_RadialDamageEnemy(short nSprite, short nDamage, short nRadius)
{
    if (!nRadius) {
        return;
    }

    word_96760++;

    if (nRadialSpr == -1)
    {
        nRadialDamage = nDamage * 4;
        nDamageRadius = nRadius;
        nRadialSpr = nSprite;
        nRadialOwner = sprite[nSprite].owner;

        runlist_ExplodeSignalRun();

        nRadialSpr = -1;
        word_96760--;
    }
}

void runlist_DamageEnemy(int nSprite, int nSprite2, short nDamage)
{
    if (sprite[nSprite].statnum >= kMaxStatus) {
        return;
    }

    short nRun = sprite[nSprite].owner;

    if (nRun <= -1) {
        return;
    }

    runlist_SendMessageToRunRec(nRun, (nSprite2 & 0xFFFF) | 0x80000, nDamage * 4);

    if (nCreaturesLeft <= 0) {
        return;
    }

    if (sprite[nSprite2].statnum != 100) {
        return;
    }

    short nPlayer = GetPlayerFromSprite(nSprite2);
    nTauntTimer[nPlayer]--;

    if (nTauntTimer[nPlayer] <= 0)
    {
        // Do a taunt
        int nPlayerSprite = PlayerList[nPlayer].nSprite;
        int nSector = sprite[nPlayerSprite].sectnum;

        if (!(SectFlag[nSector] & kSectUnderwater))
        {
            int ebx = 0x4000;

            if (nPlayer == nLocalPlayer) {
                ebx = 0x6000;
            }

            int nDopSprite = nDoppleSprite[nPlayer];
            D3PlayFX(StaticSound[kSoundTauntStart + (RandomSize(3) % 5)], nDopSprite | ebx);
        }

        nTauntTimer[nPlayer] = RandomSize(3) + 3;
    }
}
