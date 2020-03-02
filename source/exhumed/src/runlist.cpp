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

BEGIN_PS_NS

//#define kFuncMax	0x260000 // the number 38 stored in the high word of an int
#define kFuncMax		39
#define kMaxRunStack	200


short RunCount = -1;
short nRadialSpr = -1;
short nStackCount = 0;
short word_966BE = 0;
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
    FuncPlayer,
    FuncBullet,
    FuncSpider,
    FuncCreatureChunk,
    FuncMummy,
    FuncGrenade,
    FuncAnim,
    FuncSnake,
    FuncFish,
    FuncLion,
    FuncBubble,
    FuncLava,
    FuncLavaLimb,
    FuncObject,
    FuncRex,
    FuncSet,
    FuncQueen,
    FuncQueenHead,
    FuncRoach,
    FuncQueenEgg,
    FuncWasp,
    FuncTrap,
    FuncFishLimb,
    FuncRa,
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

int runlist_HeadRun()
{
    int nRun = runlist_GrabRun();

    RunData[nRun]._4 = -1;
    RunData[nRun]._6 = -1;

    return nRun;
}

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

void runlist_SubRunRec(int RunPtr)
{
    assert(RunPtr >= 0 && RunPtr < kMaxRuns);

    RunData[RunPtr].nMoves = -totalmoves;
}

void runlist_SendMessageToRunRec(int nRun, int nMessage, int nDamage)
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
    aiFunctions[nFunc](nMessage, nDamage, nRun);
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
        I_Error("PopMoveRun() called inappropriately\n");
        exit(-1);
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

void runlist_ChangeChannel(int eax, short nVal)
{
    if (sRunChannels[eax].b < 0)
    {
        short nChannel = ChannelList;
        ChannelList = eax;
        sRunChannels[eax].b = nChannel;
    }

    sRunChannels[eax].c = nVal;
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
    short v0;
    short v1;
    int v5;
    short b;
    short d;

    do
    {
        v0 = -1;
        v1 = -1;

        while (ChannelList >= 0)
        {
            b = sRunChannels[ChannelList].b;
            d = sRunChannels[ChannelList].d;

            if (d & 2)
            {
                sRunChannels[ChannelList].d = d ^ 2;
                runlist_SignalRun(sRunChannels[ChannelList].a, ChannelList | 0x10000);
            }

            if (d & 1)
            {
                sRunChannels[ChannelList].d ^= 1;
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

void runlist_ProcessSectorTag(int nSector, int nLotag, int nHitag)
{
    int zListA[8];
    int zListB[8];

    int nChannel = runlist_AllocChannel(nHitag % 1000);
    assert(nChannel >= 0 && nChannel < kMaxChannels);

    int keyMask = (nHitag / 1000) << 12;
    int nSpeed = nLotag / 1000;

    if (!nSpeed) {
        nSpeed = 1;
    }

    nSpeed <<= 2;

    int nEffectTag = (nLotag % 1000);

    switch (nEffectTag)
    {
        case 1: // Ceiling Doom door
        {
            /*
                This function searches z-coordinates of neighboring sectors to find the
                closest (next) ceiling starting at the given z-coordinate (thez).
            */
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwPress = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwPress);

            int nSwPause = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwPause);
            return;
        }

        case 2: // Floor Doom door
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].ceilingz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwPress = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwPress);

            int nSwPause = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwPause);
            return;
        }

        case 3:
        case 4:
        case 19:
        case 20:
        case 22:
        case 29:
        case 30:
        case 46:
        case 47:
        case 60:
        case 65:
        case 66:
        case 67:
        case 69:
        case 72:
        case 73:
        case 74:
        case 76:
        case 77:
        case 78:
        case 79:
        case 81:
        case 82:
        case 83:
        case 84:
        case 85:
        {
            return;
        }

        case 5: // Permanent floor raise
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz + 1, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 6: // Touchplate floor lower, single
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

        case 7: // Touchplate floor lower, multiple
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            int nSwitch2 = BuildSwNotOnPause(nChannel, BuildLink(2, -1, 0), nSector, 8);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch2);
            return;
        }

        case 8: // Permanent floor lower
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 9: // Switch activated lift down
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 150);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 10: // Touchplate Floor Raise
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            int nSwitch2 = BuildSwNotOnPause(nChannel, BuildLink(2, -1, 0), nSector, 8);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch2);
            return;
        }

        case 11: // Permanent floor raise
        case 14: // Sector raise / lower
        case 38: // Sector raise / lower
        {
            /*
                fix for original behaviour - nextSector could be -1 the and game would do an invalid memory read
                when getting the floorz for nextSector. Here, we assume 0 and only set the correct value if nextSector
                is valid.
            */
            int zVal = 0;

            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, -1);
            if (nextSector >= 0) {
                zVal = sector[nextSector].floorz;
            }

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, zVal);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 12: // Switch activated lift up
        {
            /*
                fix for original behaviour - nextSector could be -1 the and game would do an invalid memory read
                when getting the floorz for nextSector. Here, we assume 0 and only set the correct value if nextSector
                is valid.
            */
            int zVal = 0;

            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, -1);
            if (nextSector >= 0) {
                zVal = sector[nextSector].floorz;
            }

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, zVal);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 150);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 13: // Bobbing floor
        {
            /*
                fix for original behaviour - nextSector could be -1 the and game would do an invalid memory read
                when getting the floorz for nextSector. Here, we assume 0 and only set the correct value if nextSector
                is valid.
            */
            int zVal = 0;

            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            if (nextSector >= 0) {
                zVal = sector[nextSector].floorz;
            }

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, zVal);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwReady(nChannel, BuildLink(2, 1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 15: // Sector raise/lower
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 16: // Stuttering noise (floor makes noise)
        {
            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sector[nSector].ceilingz, sector[nSector].floorz - 8);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            int nSwitch2 = BuildSwReady(nChannel, BuildLink(2, -1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch2);
            return;
        }

        case 17: // Reserved?
        {
            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sector[nSector].ceilingz, sector[nSector].floorz - 8);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwReady(nChannel, BuildLink(2, -1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 18: // Raises floor AND lowers ceiling
        {
            int ebx = ((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz;

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sector[nSector].floorz, ebx);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int ebx2 = (((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz) - 8;

            int nElev2 = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sector[nSector].ceilingz, ebx2);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev2);

            int nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 21: // Touchplate
        {
            int nSwitch = BuildSwStepOn(nChannel, BuildLink(2, 1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 23: // Floor raise, Sychronize
        {
            /*
                fix for original behaviour - nextSector could be -1 the and game would do an invalid memory read
                when getting the floorz for nextSector. Here, we assume 0 and only set the correct value if nextSector
                is valid.
            */
            int zVal = 0;

            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            if (nextSector >= 0) {
                zVal = sector[nextSector].floorz;
            }

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 32767, 200, 2, sector[nSector].floorz, zVal);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), nSpeed * 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 24: // Ceiling door, channel trigger only
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 25:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 300);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 26:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 450);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 27:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 600);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 28:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 900);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 31: // Touchplate
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 0x7FFF, 0x7FFF, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 32:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 0x7FFF, 0x7FFF, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 33: // Ceiling Crusher
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(20, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nextSector].ceilingz, sector[nSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 34: // Triggerable Ceiling Crusher(Inactive)
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(28, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nextSector].ceilingz, sector[nSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 35: // Destructible sector
        case 36:
        {
            nEnergyTowers++;

            int nEnergyBlock = BuildEnergyBlock(nSector);

            if (nLotag == 36) {
                nFinaleSpr = nEnergyBlock;
            }

            return;
        }

        case 37:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 39: // Touchplate
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

        case 40: // Moving sector(follows waypoints)
        {
            AddMovingSector(nSector, nLotag, nHitag % 1000, 2);
            return;
        }

        case 41: // Moving sector(follows waypoints)
        {
            AddMovingSector(nSector, nLotag, nHitag % 1000, 18);
            return;
        }

        case 42: // Moving sector(follows waypoints)
        {
            AddMovingSector(nSector, nLotag, nHitag % 1000, 58);
            return;
        }

        case 43: // Moving sector(follows waypoints)
        {
            AddMovingSector(nSector, nLotag, nHitag % 1000, 122);
            return;
        }

        case 44: // Moving sector(follows waypoints)
        {
            AddMovingSector(nSector, nLotag, nHitag % 1000, 90);
            return;
        }

        case 45: // Pushbox sector
        {
            CreatePushBlock(nSector);
            return;
        }

        case 48: // Ceiling lower
        {
            /*
                fix for original behaviour - nextSector could be -1 the and game would do an invalid memory read
                when getting the floorz for nextSector. Here, we assume 0 and only set the correct value if nextSector
                is valid.
            */
            int zVal = 0;

            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, 1);
            if (nextSector >= 0) {
                zVal = sector[nextSector].ceilingz;
            }

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sector[nSector].ceilingz, zVal);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 49:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sector[nSector].ceilingz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 50: // Floor lower / raise
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 0x7FFF, 200, 2, sector[nextSector].floorz, sector[nSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 51:
        {
            int edx = ((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz;

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sector[nSector].floorz, edx);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int eax = (((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz) - 8;

            nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sector[nSector].ceilingz, eax);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwReady(nChannel, BuildLink(2, 1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 52:
        {
            int eax = ((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz;

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, eax, sector[nSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            eax = ((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz;

            nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, eax, sector[nSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 53:
        {
            int eax = ((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz;

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, eax, sector[nSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            eax = ((sector[nSector].floorz - sector[nSector].ceilingz) / 2) + sector[nSector].ceilingz;

            nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, eax, sector[nSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 150);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 54:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 55:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 56:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 57:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].ceilingz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 58:
        {
            int nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            // Fall through to case 62
            fallthrough__;
        }
        case 63: // Ceiling door, kill trigger (Enemy death triggers door)
        {
            if (nLotag == 63) {
                nEnergyChan = nChannel;
            }

            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 59:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            int nSwitch2 = BuildSwNotOnPause(nChannel, BuildLink(1, 1), nSector, 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch2);
            return;
        }

        case 61:
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

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, var_1C,
                zListB[0], zListB[1], zListB[2], zListB[3], zListB[4], zListB[5], zListB[6], zListB[7]);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 62:
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

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, var_20,
                zListA[0], zListA[1], zListA[2], zListA[3], zListA[4], zListA[5], zListA[6], zListA[7]);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 64:
        {
            int nSwitch = BuildSwStepOn(nChannel, BuildLink(2, 0, 0), nSector);
            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);
            return;
        }

        case 68:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].floorz, 1, 1);
            assert(nextSector > -1);

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sector[nSector].floorz, sector[nextSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 70:
        case 71:
        {
            short nextSector = nextsectorneighborz(nSector, sector[nSector].ceilingz, -1, -1);
            assert(nextSector > -1);

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, (int)sector[nSector].floorz, (int)sector[nextSector].ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);

            int nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch);

            int nSwitch2 = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwitch2);
            return;
        }

        case 75:
        {
            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, (int)sector[nSector].ceilingz, (int)sector[nSector].floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev);
            return;
        }

        case 80:
        {
            SectFlag[nSector] |= 0x8000;
            return;
        }
    }
}

void runlist_ProcessWallTag(int nWall, short nLotag, short nHitag)
{
    int nChannel = runlist_AllocChannel(nHitag % 1000);
    assert(nChannel >= 0 && nChannel < kMaxChannels);

    int nPanSpeed = nLotag / 1000;
    if (!nPanSpeed) {
        nPanSpeed = 1;
    }

    nPanSpeed <<= 2;

    int nEffectTag = nLotag % 1000;

    switch (nEffectTag)
    {
        default:
            return;

        case 1:
        {
            int nWallFace = BuildWallFace(nChannel, nWall, 2, wall[nWall].picnum, wall[nWall].picnum + 1);
            runlist_AddRunRec(sRunChannels[nChannel].a, nWallFace);

            int nSwitch = BuildSwPressWall(nChannel, BuildLink(2, nEffectTag, 0), nWall);

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

        case 8: // Reverse switch
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

        case 12: // Twin star trek door
        case 14:
        case 16:
        case 19:
        case 20:
        {
            int nLastWall = 0;
            int n2ndLastWall = 0;

            short nStart = nWall;

            while (1)
            {
                nWall = wall[nWall].point2; // get the next (right side) wall point

                if (nStart == nWall) { // we've looped back around
                    break;
                }

                n2ndLastWall = nLastWall;
                nLastWall = nWall;
            }

            short nWall2 = wall[nStart].point2;
            short nWall3 = wall[nWall2].point2;
            short nWall4 = wall[nWall3].point2;

            int nSlide = BuildSlide(nChannel, nStart, nLastWall, n2ndLastWall, nWall2, nWall3, nWall4);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSlide);
            return;
        }

        case 24: // Waterfall
        {
            AddFlow(nWall, nPanSpeed, 3);
            return;
        }

        case 25: // Inverse waterfall
        {
            AddFlow(nWall, nPanSpeed, 2);
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

    if (klabs(x) > nDamageRadius) {
        return 0;
    }

    if (klabs(y) > nDamageRadius) {
        return 0;
    }

    if (klabs(z) > nDamageRadius) {
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

                sprite[nSprite].xvel += (edi * Cos(nAngle)) >> 3;
                sprite[nSprite].yvel += (edi * Sin(nAngle)) >> 3;
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

    if (nRadialSpr == -1)
    {
        nRadialDamage = nDamage * 4;
        nDamageRadius = nRadius;
        nRadialSpr = nSprite;
        nRadialOwner = sprite[nSprite].owner;

        runlist_ExplodeSignalRun();

        nRadialSpr = -1;
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

    short nPreCreaturesLeft = nCreaturesLeft;

    runlist_SendMessageToRunRec(nRun, (nSprite2 & 0xFFFF) | 0x80000, nDamage * 4);

    // is there now one less creature? (has one died)
    if (nPreCreaturesLeft > nCreaturesLeft&& nSprite2 > -1)
    {
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
}

static SavegameHelper sgh("runlist",
    SV(RunCount),
    SV(nRadialSpr),
    SV(nStackCount),
    SV(word_966BE),
    SV(ChannelList),
    SV(ChannelLast),
    SV(nRadialOwner),
    SV(nDamageRadius),
    SV(nRadialDamage),
    SV(RunChain),
    SV(NewRun),
    SA(sRunStack),
    SA(RunFree),
    SA(sRunChannels),
    SA(RunData),
    SV(word_96760),
    nullptr);

END_PS_NS
