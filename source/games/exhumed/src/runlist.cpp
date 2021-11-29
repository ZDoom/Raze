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
#include "aistuff.h"
#include "player.h"
#include "sound.h"
#include <assert.h>

BEGIN_PS_NS

enum
{
	kFuncMax		= 39,
	kMaxRunStack	= 200
};


DExhumedActor* pRadialActor;
short nStackCount = 0;
short word_966BE = 0;
short ChannelList = -1;
short ChannelLast = -1;

int nDamageRadius;
int nRadialDamage;
short RunChain;
short NewRun;

int sRunStack[kMaxRunStack];
RunChannel sRunChannels[kMaxChannels];
FreeListArray<RunStruct, kMaxRuns> RunData;


FSerializer& Serialize(FSerializer& arc, const char* keyname, RunStruct& w, RunStruct* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("ref", w.nAIType)
            ("val", w.nObjIndex)
            ("actor", w.pObjActor)
            ("_4", w.next)
            ("_6", w.prev)
            .EndObject();
    }
    return arc;
}

void SerializeRunList(FSerializer& arc)
{
    if (arc.BeginObject("runlist"))
    {
        arc("data", RunData)
            ("stackcount", nStackCount)
            ("w966be", word_966BE)
            ("list", ChannelList)
            ("last", ChannelLast)
            ("damageradius", nDamageRadius)
            ("radialdamage", nRadialDamage)
            ("runchain", RunChain)
            ("newrun", NewRun)
            .Array("runstack", sRunStack, nStackCount)
            .Array("runchannels", &sRunChannels[0].a, 4 * kMaxChannels) // save this in a more compact form than an array of structs.
            .EndObject();
    }
}

AIElev aiElev;
AISWReady aiSwReady;
AISWPause aiSwPause;
AISWStepOn aiSwStepOn;
AISWNotOnPause aiSwNotOnPause;
AISWPressSector aiSwPressSector;
AISWPressWall aiSwPressWall;
AIWallFace aiWallFace;
AISlide aiSlide;
AIAnubis aiAnubis;
AIPlayer aiPlayer;
AIBullet aiBullet;
AISpider aiSpider;
AICreatureChunk aiCreatureChunk;
AIMummy aiMummy;
AIGrenade aiGrenade;
AIAnim aiAnim;
AISnake aiSnake;
AIFish aiFish;
AILion aiLion;
AIBubble aiBubble;
AILavaDude aiLava;
AILavaDudeLimb aiLavaLimb;
AIObject aiObject;
AIRex aiRex;
AISet aiSet;
AIQueen aiQueen;
AIQueenHead aiQueenHead;
AIRoach aiRoach;
AIQueenEgg aiQueenEgg;
AIWasp aiWasp;
AITrap aiTrap;
AIFishLimb aiFishLimb;
AIRa aiRa;
AIScorp aiScorp;
AISoul aiSoul;
AIRat aiRat;
AIEnergyBlock aiEnergyBlock;
AISpark aiSpark;


ExhumedAI* ais[kFuncMax] =
{
    &aiElev,
    &aiSwReady,
    &aiSwPause,
    &aiSwStepOn,
    &aiSwNotOnPause,
    &aiSwPressSector,
    &aiSwPressWall,
    &aiWallFace,
    &aiSlide,
    &aiAnubis,
    &aiPlayer,
    &aiBullet,
    &aiSpider,
    &aiCreatureChunk,
    &aiMummy,
    &aiGrenade,
    &aiAnim,
    &aiSnake,
    &aiFish,
    &aiLion,
    &aiBubble,
    &aiLava,
    &aiLavaLimb,
    &aiObject,
    &aiRex,
    &aiSet,
    &aiQueen,
    &aiQueenHead,
    &aiRoach,
    &aiQueenEgg,
    &aiWasp,
    &aiTrap,
    &aiFishLimb,
    &aiRa,
    &aiScorp,
    &aiSoul,
    &aiRat,
    &aiEnergyBlock,
    &aiSpark,
};


int runlist_GrabRun()
{
    return RunData.Get();
}

int runlist_FreeRun(int nRun)
{
    assert(nRun >= 0 && nRun < kMaxRuns);

    RunData[nRun].prev = -1;
    RunData[nRun].nAIType = -1;
    RunData[nRun].nObjIndex = -1;
    RunData[nRun].pObjActor = nullptr;
    RunData[nRun].next = RunData[nRun].prev;
    RunData.Release(nRun);
    return 1;
}

int runlist_HeadRun()
{
    int nRun = runlist_GrabRun();

    RunData[nRun].next = -1;
    RunData[nRun].prev = -1;

    return nRun;
}

void runlist_InitRun()
{
    int i;

    RunData.Clear();
    nStackCount = 0;

    for (i = 0; i < kMaxRuns; i++)
    {
        RunData[i].nAIType = -1;
        RunData[i].pObjActor = nullptr;
        RunData[i].nObjIndex = -1;
        RunData[i].prev = -1;
        RunData[i].next = -1;
    }

    int nRun = runlist_HeadRun();
    RunChain = nRun;
    NewRun = nRun;

    for (i = 0; i < kMaxPlayers; i++) {
        PlayerList[i].nRun = -1;
    }

    pRadialActor = nullptr;
}

void runlist_UnlinkRun(int nRun)
{
    if (!(nRun >= 0 && nRun < kMaxRuns)) return;

    if (nRun == RunChain)
    {
        RunChain = RunData[nRun].next;
        return;
    }

    if (RunData[nRun].prev >= 0)
    {
        RunData[RunData[nRun].prev].next = RunData[nRun].next;
    }

    if (RunData[nRun].next >= 0)
    {
        RunData[RunData[nRun].next].prev = RunData[nRun].prev;
    }
}

void runlist_InsertRun(int RunLst, int RunNum)
{
    if (!(RunLst >= 0 && RunLst < kMaxRuns)) return;
    if (!(RunNum >= 0 && RunNum < kMaxRuns)) return;

    RunData[RunNum].prev = RunLst;
    int val = RunData[RunLst].next;
    RunData[RunNum].next = val;

    if (val >= 0) {
        RunData[val].prev = RunNum;
    }

    RunData[RunLst].next = RunNum;
}

int runlist_AddRunRec(int nIndex, int nObject, int nAIType)
{
    int nRun = runlist_GrabRun();

    RunData[nRun].nAIType = nAIType;
    RunData[nRun].nObjIndex = nObject;
    RunData[nRun].pObjActor = nullptr;

    runlist_InsertRun(nIndex, nRun);
    return nRun;
}

int runlist_AddRunRec(int nIndex, DExhumedActor* pObject, int nAIType)
{
    int nRun = runlist_GrabRun();

    RunData[nRun].nAIType = nAIType;
    RunData[nRun].nObjIndex = -1;
    RunData[nRun].pObjActor = pObject;

    runlist_InsertRun(nIndex, nRun);
    return nRun;
}

int runlist_AddRunRec(int nIndex, RunStruct* other)
{
    int nRun = runlist_GrabRun();

    RunData[nRun].nAIType = other->nAIType;
    RunData[nRun].nObjIndex = other->nObjIndex;
    RunData[nRun].pObjActor = other->pObjActor;

    runlist_InsertRun(nIndex, nRun);
    return nRun;
}


void runlist_DoSubRunRec(int RunPtr)
{
    if (!(RunPtr >= 0 && RunPtr < kMaxRuns)) return;

    runlist_UnlinkRun(RunPtr);
    runlist_FreeRun(RunPtr);
}

void runlist_CleanRunRecs()
{
    int nextPtr = RunChain;

    if (nextPtr >= 0)
    {
        assert(nextPtr < kMaxRuns);
        nextPtr = RunData[nextPtr].next;
    }

    while (nextPtr >= 0)
    {
        int runPtr = nextPtr;
        assert(runPtr < kMaxRuns);

        int val = RunData[runPtr].nAIType; // >> 16;
        nextPtr = RunData[runPtr].next;

        if (val < 0) {
            runlist_DoSubRunRec(runPtr);
        }
    }
}

void runlist_SubRunRec(int RunPtr)
{
    if (!(RunPtr >= 0 && RunPtr < kMaxRuns)) return;

    RunData[RunPtr].nAIType = -totalmoves;
}

void runlist_SendMessage(int nRun, int nObject, void(ExhumedAI::* func)(RunListEvent*), RunListEvent* ev)
{
    int nFunc = RunData[nRun].nAIType >> 16;

    if (nFunc < 0 || nFunc >= (int)countof(ais)) {
        return;
    }

    RunListEvent defev;
    if (!ev)
    {
        defev = {};
        ev = &defev;
    }
    ev->nObjIndex = RunData[nRun].nObjIndex;
    ev->pObjActor = RunData[nRun].pObjActor;
    ev->nParam = nObject;
    ev->nRun = nRun;


    (ais[nFunc]->*func)(ev);
}

void runlist_ExplodeSignalRun()
{
    short nextPtr = RunChain;

    if (nextPtr >= 0)
    {
        assert(nextPtr < kMaxRuns);
        nextPtr = RunData[nextPtr].next;
    }

    // LOOP
    while (nextPtr >= 0)
    {
        int runPtr = nextPtr;
        assert(runPtr < kMaxRuns);

        nextPtr = RunData[runPtr].next;

        if (RunData[runPtr].nObjIndex >= 0 || RunData[runPtr].pObjActor)
        {
            RunListEvent ev{};
            ev.nMessage = 1;
            ev.nRadialDamage = nRadialDamage;
            ev.nDamageRadius = nDamageRadius;
            ev.pRadialActor = pRadialActor;
            runlist_SendMessage(runPtr, 0, &ExhumedAI::RadialDamage, &ev);
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

void runlist_SignalRun(int NxtPtr, int edx, void(ExhumedAI::* func)(RunListEvent*), RunListEvent* ev)
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
            NxtPtr = RunData[NxtPtr].next;
        }

        while (NxtPtr >= 0)
        {
            int RunPtr = NxtPtr;

            if (RunPtr >= 0)
            {
                assert(RunPtr < kMaxRuns);
                NxtPtr = RunData[RunPtr].next;

                if (RunData[RunPtr].nObjIndex >= 0 || RunData[RunPtr].pObjActor) {
                    runlist_SendMessage(RunPtr, edx, func, ev);
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
                runlist_SignalRun(sRunChannels[ChannelList].a, ChannelList, &ExhumedAI::ProcessChannel);
            }

            if (d & 1)
            {
                sRunChannels[ChannelList].d ^= 1;
                runlist_SignalRun(sRunChannels[ChannelList].a, 0, &ExhumedAI::Process);
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
            runlist_SignalRun(sRunChannels[nChannel].a, ChannelList, &ExhumedAI::ProcessChannel);
        }

        if (d & 1)
        {
            sRunChannels[nChannel].d = d ^ 1;
            runlist_SignalRun(sRunChannels[nChannel].a, 0, &ExhumedAI::Process);
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
    runlist_SignalRun(RunChain, 0, &ExhumedAI::Tick);
}

void runlist_ProcessSectorTag(int nSector, int nLotag, int nHitag)
{
    auto sectp = &sector[nSector];
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
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->ceilingz, -1, -1);
            

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwPress = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwPress.first, nSwPress.second);

            auto nSwPause = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwPause.first, nSwPause.second);
            return;
        }

        case 2: // Floor Doom door
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
            

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->ceilingz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwPress = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwPress.first, nSwPress.second);

            auto nSwPause = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwPause.first, nSwPause.second);
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
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz + 1, -1, -1);
			if (nextSectorP == nullptr) break;

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 6: // Touchplate floor lower, single
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 400, 400, 2, nextSectorP->floorz, sectp->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(2, 1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            sectp->floorz = nextSectorP->floorz;
            return;
        }

        case 7: // Touchplate floor lower, multiple
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            auto nSwitch2 = BuildSwNotOnPause(nChannel, BuildLink(2, -1, 0), nSector, 8);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch2.first, nSwitch2.second);
            return;
        }

        case 8: // Permanent floor lower
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 9: // Switch activated lift down
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 150);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 10: // Touchplate Floor Raise
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            auto nSwitch2 = BuildSwNotOnPause(nChannel, BuildLink(2, -1, 0), nSector, 8);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch2.first, nSwitch2.second);
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

            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, -1);
            if (nextSectorP != nullptr) {
                zVal = nextSectorP->floorz;
            }

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, zVal);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
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

            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, -1);
            if (nextSectorP != nullptr) {
                zVal = nextSectorP->floorz;
            }

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, zVal);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 150);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
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

            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
            if (nextSectorP != nullptr) {
                zVal = nextSectorP->floorz;
            }

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, zVal);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwReady(nChannel, BuildLink(2, 1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 15: // Sector raise/lower
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 16: // Stuttering noise (floor makes noise)
        {
            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sectp->ceilingz, sectp->floorz - 8);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            auto nSwitch2 = BuildSwReady(nChannel, BuildLink(2, -1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch2.first, nSwitch2.second);
            return;
        }

        case 17: // Reserved?
        {
            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sectp->ceilingz, sectp->floorz - 8);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwReady(nChannel, BuildLink(2, -1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 18: // Raises floor AND lowers ceiling
        {
            int ebx = ((sectp->floorz - sectp->ceilingz) / 2) + sectp->ceilingz;

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sectp->floorz, ebx);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            int ebx2 = (((sectp->floorz - sectp->ceilingz) / 2) + sectp->ceilingz) - 8;

            int nElev2 = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sectp->ceilingz, ebx2);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev2, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 21: // Touchplate
        {
            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(2, 1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
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

            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
            if (nextSectorP) {
                zVal = nextSectorP->floorz;
            }

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 32767, 200, 2, sectp->floorz, zVal);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), nSpeed * 60);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 24: // Ceiling door, channel trigger only
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 25:
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 300);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 26:
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 450);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 27:
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 600);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 28:
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 900);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 31: // Touchplate
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 0x7FFF, 0x7FFF, 2, sectp->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 32:
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 0x7FFF, 0x7FFF, 2, sectp->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 33: // Ceiling Crusher
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(20, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, nextSectorP->ceilingz, sectp->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 34: // Triggerable Ceiling Crusher(Inactive)
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(28, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, nextSectorP->ceilingz, sectp->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 35: // Destructible sector
        case 36:
        {
            nEnergyTowers++;

            auto nEnergyBlock = BuildEnergyBlock(nSector);

            if (nLotag == 36) {
                pFinaleSpr = nEnergyBlock;
            }

            return;
        }

        case 37:
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 39: // Touchplate
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 0x7FFF, 0x7FFF, 2, sectp->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            auto nSwitch2 = BuildSwNotOnPause(nChannel, BuildLink(2, -1, 0), nSector, 8);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch2.first, nSwitch2.second);
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

            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->ceilingz, -1, 1);
            if (nextSectorP != nullptr) {
                zVal = nextSectorP->ceilingz;
            }

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sectp->ceilingz, zVal);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 49:
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sectp->ceilingz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 50: // Floor lower / raise
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->ceilingz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 0x7FFF, 200, 2, nextSectorP->floorz, sectp->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 51:
        {
            int edx = ((sectp->floorz - sectp->ceilingz) / 2) + sectp->ceilingz;

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sectp->floorz, edx);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            int eax = (((sectp->floorz - sectp->ceilingz) / 2) + sectp->ceilingz) - 8;

            nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), 200, nSpeed * 100, 2, sectp->ceilingz, eax);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwReady(nChannel, BuildLink(2, 1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 52:
        {
            int eax = ((sectp->floorz - sectp->ceilingz) / 2) + sectp->ceilingz;

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, eax, sectp->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            eax = ((sectp->floorz - sectp->ceilingz) / 2) + sectp->ceilingz;

            nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, eax, sectp->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 53:
        {
            int eax = ((sectp->floorz - sectp->ceilingz) / 2) + sectp->ceilingz;

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, eax, sectp->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            eax = ((sectp->floorz - sectp->ceilingz) / 2) + sectp->ceilingz;

            nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, eax, sectp->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 150);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 54:
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 55:
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 56:
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 57:
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->ceilingz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 58:
        {
            auto nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            // Fall through to case 62
            fallthrough__;
        }
        case 63: // Ceiling door, kill trigger (Enemy death triggers door)
        {
            if (nLotag == 63) {
                nEnergyChan = nChannel;
            }

            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->ceilingz, -1, -1);
            

            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 59:
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), nSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            auto nSwitch2 = BuildSwNotOnPause(nChannel, BuildLink(1, 1), nSector, 60);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch2.first, nSwitch2.second);
            return;
        }

        case 61:
        {
            zListB[0] = sectp->floorz;
            int var_1C = 1;

            while (1)
            {
                auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, -1);
                if (nextSectorP == nullptr || var_1C >= 8) {
                    break;
                }

                zListB[var_1C] = nextSectorP->floorz;

                var_1C++;
            }

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, var_1C,
                zListB[0], zListB[1], zListB[2], zListB[3], zListB[4], zListB[5], zListB[6], zListB[7]);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 62:
        {
            zListA[0] = sectp->floorz;
            int var_20 = 1;

            while (1)
            {
                auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
                if (nextSectorP == nullptr || var_20 >= 8) {
                    break;
                }

                zListA[var_20] = nextSectorP->floorz;

                var_20++;
            }

            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, var_20,
                zListA[0], zListA[1], zListA[2], zListA[3], zListA[4], zListA[5], zListA[6], zListA[7]);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 64:
        {
            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(2, 0, 0), nSector);
            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 68:
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, sectp->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 70:
        case 71:
        {
            auto nextSectorP = nextsectorneighborzptr(nSector, sectp->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, (int)sectp->floorz, (int)nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), nSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            auto nSwitch2 = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch2.first, nSwitch2.second);
            return;
        }

        case 75:
        {
            int nElev = BuildElevC(0, nChannel, nSector, FindWallSprites(nSector), nSpeed * 100, nSpeed * 100, 2, (int)sectp->ceilingz, (int)sectp->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
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
            runlist_AddRunRec(sRunChannels[nChannel].a, nWallFace,  0x70000);

            auto nSwitch = BuildSwPressWall(nChannel, BuildLink(2, nEffectTag, 0), nWall);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 6:
        {
            auto nSwitch = BuildSwPressWall(nChannel, BuildLink(2, 1, 0), nWall);
            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 7: // Regular switch
        {
            int nWallFace = BuildWallFace(nChannel, nWall, 2, wall[nWall].picnum, wall[nWall].picnum + 1);
            runlist_AddRunRec(sRunChannels[nChannel].a, nWallFace,  0x70000);

            auto nSwitch = BuildSwPressWall(nChannel, BuildLink(1, 1), nWall);
            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 8: // Reverse switch
        {
            int nWallFace = BuildWallFace(nChannel, nWall, 2, wall[nWall].picnum, wall[nWall].picnum + 1);
            runlist_AddRunRec(sRunChannels[nChannel].a, nWallFace,  0x70000);

            auto nSwitch = BuildSwPressWall(nChannel, BuildLink(2, -1, 0), nWall);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 9: // Invisible switch
        {
            auto nSwitch = BuildSwPressWall(nChannel, BuildLink(2, 1, 1), nWall);
            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 10:
        {
            auto nSwitch = BuildSwPressWall(nChannel, BuildLink(2, -1, 0), nWall);
            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
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

            runlist_AddRunRec(sRunChannels[nChannel].a, nSlide, 0x80000);
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

int runlist_CheckRadialDamage(DExhumedActor* pActor)
{
	auto pSprite = &pActor->s();
    auto pRadialSpr = &pRadialActor->s();

    if (pActor == pRadialActor) {
        return 0;
    }

    if (!(pSprite->cstat & 0x101)) {
        return 0;
    }

    if (pSprite->statnum >= kMaxStatus || pRadialSpr->statnum >= kMaxStatus) {
        return 0;
    }

    if (pSprite->statnum != 100 && pActor == pRadialActor->pTarget) {
        return 0;
    }

    int x = (pSprite->x - pRadialSpr->x) >> 8;
    int y = (pSprite->y - pRadialSpr->y) >> 8;
    int z = (pSprite->z - pRadialSpr->z) >> 12;

    if (abs(x) > nDamageRadius) {
        return 0;
    }

    if (abs(y) > nDamageRadius) {
        return 0;
    }

    if (abs(z) > nDamageRadius) {
        return 0;
    }

    int edi = 0;

    uint32_t xDiff = abs(x);
    uint32_t yDiff = abs(y);

    uint32_t sqrtNum = xDiff * xDiff + yDiff * yDiff;

    if (sqrtNum > INT_MAX)
    {
        DPrintf(DMSG_WARNING, "%s %d: overflow\n", __func__, __LINE__);
        sqrtNum = INT_MAX;
    }

    int nDist = ksqrt(sqrtNum);

    if (nDist < nDamageRadius)
    {
        uint16_t nCStat = pSprite->cstat;
        pSprite->cstat = 0x101;

        if (((kStatExplodeTarget - pSprite->statnum) <= 1) ||
            cansee(pRadialSpr->x,
                pRadialSpr->y,
                pRadialSpr->z - 512,
                pRadialSpr->sectnum,
                pSprite->x,
                pSprite->y,
                pSprite->z - 8192,
                pSprite->sectnum))
        {
            edi = (nRadialDamage * (nDamageRadius - nDist)) / nDamageRadius;

            if (edi < 0) {
                edi = 0;
            }
            else if (edi > 20)
            {
                int nAngle = GetMyAngle(x, y);

                pSprite->xvel += (edi * bcos(nAngle)) >> 3;
                pSprite->yvel += (edi * bsin(nAngle)) >> 3;
                pSprite->zvel -= edi * 24;

                if (pSprite->zvel < -3584) {
                    pSprite->zvel = -3584;
                }
            }
        }

        pSprite->cstat = nCStat;
    }

    if (edi > 0x7FFF) {
        edi = 0x7FFF;
    }

    return edi;
}

void runlist_RadialDamageEnemy(DExhumedActor* pActor, short nDamage, short nRadius)
{
    if (!nRadius) {
        return;
    }

    if (pRadialActor == nullptr)
    {
        nRadialDamage = nDamage * 4;
        nDamageRadius = nRadius;
        pRadialActor = pActor;

        runlist_ExplodeSignalRun();

        pRadialActor = nullptr;
    }
}

void runlist_DamageEnemy(DExhumedActor* pActor, DExhumedActor* pActor2, short nDamage)
{
	auto pSprite = &pActor->s();

    if (pSprite->statnum >= kMaxStatus) {
        return;
    }

    short nRun = pSprite->owner;
    if (nRun <= -1) {
        return;
    }

    short nPreCreaturesKilled = nCreaturesKilled;

    RunListEvent ev{};
    ev.pOtherActor = pActor2;
    ev.nDamage = nDamage * 4;
    runlist_SendMessage(nRun, -1, &ExhumedAI::Damage, &ev);

    // is there now one less creature? (has one died)
    if (nPreCreaturesKilled < nCreaturesKilled && pActor2 != nullptr)
    {
        if (pActor2->s().statnum != 100) {
            return;
        }

        int nPlayer = GetPlayerFromActor(pActor2);
        PlayerList[nPlayer].nTauntTimer--;

        if (PlayerList[nPlayer].nTauntTimer <= 0)
        {
            // Do a taunt
            auto pPlayerActor = PlayerList[nPlayer].Actor();
            int nSector = pPlayerActor->s().sectnum;

            if (!(SectFlag[nSector] & kSectUnderwater))
            {
                int ebx = 0x4000;

                if (nPlayer == nLocalPlayer) {
                    ebx = 0x6000;
                }

                D3PlayFX(StaticSound[kSoundTauntStart + (RandomSize(3) % 5)], PlayerList[nPlayer].pDoppleSprite, ebx);
            }

            PlayerList[nPlayer].nTauntTimer = RandomSize(3) + 3;
        }
    }
}

END_PS_NS
