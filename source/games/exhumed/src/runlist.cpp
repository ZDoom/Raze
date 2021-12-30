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
int nStackCount = 0;
int word_966BE = 0;
int ChannelList = -1;
int ChannelLast = -1;

int nDamageRadius;
int nRadialDamage;
int RunChain;
int NewRun;

int sRunStack[kMaxRunStack];
RunChannel sRunChannels[kMaxChannels];
FreeListArray<RunStruct, kMaxRuns> RunData;


size_t MarkRunlist()
{
    for (unsigned i = 0; i < kMaxRuns; i++) // only way to catch everything. :(
    {
        GC::Mark(RunData[i].pObjActor);
    }
    return kMaxRuns;
}


FSerializer& Serialize(FSerializer& arc, const char* keyname, RunStruct& w, RunStruct* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("ref", w.nAIType)
            ("val", w.nObjIndex)
            ("actor", w.pObjActor)
            ("next", w.next)
            ("prev", w.prev)
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
    int nextPtr = RunChain;

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

void runlist_ChangeChannel(int eax, int nVal)
{
    if (sRunChannels[eax].b < 0)
    {
        int nChannel = ChannelList;
        ChannelList = eax;
        sRunChannels[eax].b = nChannel;
    }

    sRunChannels[eax].c = nVal;
    sRunChannels[eax].d |= 2;
}

void runlist_ReadyChannel(int eax)
{
    if (sRunChannels[eax].b < 0)
    {
        int nChannel = ChannelList;
        ChannelList = eax;
        sRunChannels[eax].b = nChannel;
    }

    sRunChannels[eax].d |= 1;
}

void runlist_ProcessChannels()
{
    int v0;
    int v1;
    int v5;
    int b;
    int d;

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

}

int runlist_FindChannel(int ax)
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

void runlist_ProcessSectorTag(sectortype* pSector, int nLotag, int nHitag)
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
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->ceilingz, -1, -1);


            int nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwPress = BuildSwPressSector(nChannel, BuildLink(1, 1), pSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwPress.first, nSwPress.second);

            auto nSwPause = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSwPause.first, nSwPause.second);
            return;
        }

        case 2: // Floor Doom door
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->ceilingz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwPress = BuildSwPressSector(nChannel, BuildLink(1, 1), pSector, keyMask);

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
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz + 1, -1, -1);
			if (nextSectorP == nullptr) break;

            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 6: // Touchplate floor lower, single
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), 400, 400, 2, nextSectorP->floorz, pSector->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(2, 1, 1), pSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            pSector->setfloorz(nextSectorP->floorz);
            return;
        }

        case 7: // Touchplate floor lower, multiple
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), pSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            auto nSwitch2 = BuildSwNotOnPause(nChannel, BuildLink(2, -1, 0), pSector, 8);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch2.first, nSwitch2.second);
            return;
        }

        case 8: // Permanent floor lower
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 9: // Switch activated lift down
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 150);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 10: // Touchplate Floor Raise
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), pSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            auto nSwitch2 = BuildSwNotOnPause(nChannel, BuildLink(2, -1, 0), pSector, 8);

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

            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, -1);
            if (nextSectorP != nullptr) {
                zVal = nextSectorP->floorz;
            }

            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, zVal);

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

            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, -1);
            if (nextSectorP != nullptr) {
                zVal = nextSectorP->floorz;
            }

            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, zVal);

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

            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
            if (nextSectorP != nullptr) {
                zVal = nextSectorP->floorz;
            }

            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, zVal);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwReady(nChannel, BuildLink(2, 1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 15: // Sector raise/lower
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 16: // Stuttering noise (floor makes noise)
        {
            int nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), 200, nSpeed * 100, 2, pSector->ceilingz, pSector->floorz - 8);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), pSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            auto nSwitch2 = BuildSwReady(nChannel, BuildLink(2, -1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch2.first, nSwitch2.second);
            return;
        }

        case 17: // Reserved?
        {
            int nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), 200, nSpeed * 100, 2, pSector->ceilingz, pSector->floorz - 8);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwReady(nChannel, BuildLink(2, -1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 18: // Raises floor AND lowers ceiling
        {
            int ebx = ((pSector->floorz - pSector->ceilingz) / 2) + pSector->ceilingz;

            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), 200, nSpeed * 100, 2, pSector->floorz, ebx);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            int ebx2 = (((pSector->floorz - pSector->ceilingz) / 2) + pSector->ceilingz) - 8;

            int nElev2 = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), 200, nSpeed * 100, 2, pSector->ceilingz, ebx2);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev2, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), pSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 21: // Touchplate
        {
            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(2, 1, 1), pSector);

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

            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
            if (nextSectorP) {
                zVal = nextSectorP->floorz;
            }

            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), 32767, 200, 2, pSector->floorz, zVal);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), nSpeed * 60);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 24: // Ceiling door, channel trigger only
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 25:
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 300);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 26:
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 450);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 27:
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 600);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 28:
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 900);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 31: // Touchplate
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), 0x7FFF, 0x7FFF, 2, pSector->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), pSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 32:
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), 0x7FFF, 0x7FFF, 2, pSector->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 33: // Ceiling Crusher
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(20, nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, nextSectorP->ceilingz, pSector->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 34: // Triggerable Ceiling Crusher(Inactive)
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(28, nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, nextSectorP->ceilingz, pSector->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 35: // Destructible sector
        case 36:
        {
            nEnergyTowers++;

            auto nEnergyBlock = BuildEnergyBlock(pSector);

            if (nLotag == 36) {
                pFinaleSpr = nEnergyBlock;
            }

            return;
        }

        case 37:
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 39: // Touchplate
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), 0x7FFF, 0x7FFF, 2, pSector->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), pSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            auto nSwitch2 = BuildSwNotOnPause(nChannel, BuildLink(2, -1, 0), pSector, 8);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch2.first, nSwitch2.second);
            return;
        }

        case 40: // Moving sector(follows waypoints)
        {
            AddMovingSector(pSector, nLotag, nHitag % 1000, 2);
            return;
        }

        case 41: // Moving sector(follows waypoints)
        {
            AddMovingSector(pSector, nLotag, nHitag % 1000, 18);
            return;
        }

        case 42: // Moving sector(follows waypoints)
        {
            AddMovingSector(pSector, nLotag, nHitag % 1000, 58);
            return;
        }

        case 43: // Moving sector(follows waypoints)
        {
            AddMovingSector(pSector, nLotag, nHitag % 1000, 122);
            return;
        }

        case 44: // Moving sector(follows waypoints)
        {
            AddMovingSector(pSector, nLotag, nHitag % 1000, 90);
            return;
        }

        case 45: // Pushbox sector
        {
            CreatePushBlock(pSector);
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

            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->ceilingz, -1, 1);
            if (nextSectorP != nullptr) {
                zVal = nextSectorP->ceilingz;
            }

            int nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), 200, nSpeed * 100, 2, pSector->ceilingz, zVal);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 49:
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), 200, nSpeed * 100, 2, pSector->ceilingz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 50: // Floor lower / raise
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->ceilingz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), 0x7FFF, 200, 2, nextSectorP->floorz, pSector->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 51:
        {
            int edx = ((pSector->floorz - pSector->ceilingz) / 2) + pSector->ceilingz;

            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), 200, nSpeed * 100, 2, pSector->floorz, edx);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            int eax = (((pSector->floorz - pSector->ceilingz) / 2) + pSector->ceilingz) - 8;

            nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), 200, nSpeed * 100, 2, pSector->ceilingz, eax);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwReady(nChannel, BuildLink(2, 1, 0));

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 52:
        {
            int eax = ((pSector->floorz - pSector->ceilingz) / 2) + pSector->ceilingz;

            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, eax, pSector->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            eax = ((pSector->floorz - pSector->ceilingz) / 2) + pSector->ceilingz;

            nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, eax, pSector->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), pSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 53:
        {
            int eax = ((pSector->floorz - pSector->ceilingz) / 2) + pSector->ceilingz;

            int nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, eax, pSector->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            eax = ((pSector->floorz - pSector->ceilingz) / 2) + pSector->ceilingz;

            nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, eax, pSector->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPause(nChannel, BuildLink(2, -1, 0), 150);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 54:
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), pSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 55:
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), pSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 56:
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 57:
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->ceilingz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 58:
        {
            auto nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), pSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            // Fall through to case 62
            [[fallthrough]];
        }
        case 63: // Ceiling door, kill trigger (Enemy death triggers door)
        {
            if (nLotag == 63) {
                nEnergyChan = nChannel;
            }

            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->ceilingz, -1, -1);


            int nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 59:
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(1, 1), pSector);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            auto nSwitch2 = BuildSwNotOnPause(nChannel, BuildLink(1, 1), pSector, 60);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch2.first, nSwitch2.second);
            return;
        }

        case 61:
        {
            zListB[0] = pSector->floorz;
            int var_1C = 1;

            while (1)
            {
                auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, -1);
                if (nextSectorP == nullptr || var_1C >= 8) {
                    break;
                }

                zListB[var_1C] = nextSectorP->floorz;

                var_1C++;
            }

            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, var_1C,
                zListB[0], zListB[1], zListB[2], zListB[3], zListB[4], zListB[5], zListB[6], zListB[7]);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 62:
        {
            zListA[0] = pSector->floorz;
            int var_20 = 1;

            while (1)
            {
                auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
                if (nextSectorP == nullptr || var_20 >= 8) {
                    break;
                }

                zListA[var_20] = nextSectorP->floorz;

                var_20++;
            }

            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, var_20,
                zListA[0], zListA[1], zListA[2], zListA[3], zListA[4], zListA[5], zListA[6], zListA[7]);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 64:
        {
            auto nSwitch = BuildSwStepOn(nChannel, BuildLink(2, 0, 0), pSector);
            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 68:
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->floorz, 1, 1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevF(nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, pSector->floorz, nextSectorP->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 70:
        case 71:
        {
            auto nextSectorP = safenextsectorneighborzptr(pSector, pSector->ceilingz, -1, -1);
			if (nextSectorP == nullptr) break;


            int nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, (int)pSector->floorz, (int)nextSectorP->ceilingz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);

            auto nSwitch = BuildSwPressSector(nChannel, BuildLink(1, 1), pSector, keyMask);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);

            auto nSwitch2 = BuildSwPause(nChannel, BuildLink(2, -1, 0), 60);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch2.first, nSwitch2.second);
            return;
        }

        case 75:
        {
            int nElev = BuildElevC(0, nChannel, pSector, FindWallSprites(pSector), nSpeed * 100, nSpeed * 100, 2, (int)pSector->ceilingz, (int)pSector->floorz);

            runlist_AddRunRec(sRunChannels[nChannel].a, nElev, 0);
            return;
        }

        case 80:
        {
            pSector->Flag |= 0x8000;
            return;
        }
    }
}

void runlist_ProcessWallTag(walltype* pWall, int nLotag, int nHitag)
{
	auto& wal = *pWall;
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
            int nWallFace = BuildWallFace(nChannel, pWall, 2, pWall->picnum, pWall->picnum + 1);
            runlist_AddRunRec(sRunChannels[nChannel].a, nWallFace,  0x70000);

            auto nSwitch = BuildSwPressWall(nChannel, BuildLink(2, nEffectTag, 0), pWall);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 6:
        {
            auto nSwitch = BuildSwPressWall(nChannel, BuildLink(2, 1, 0), pWall);
            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 7: // Regular switch
        {
            int nWallFace = BuildWallFace(nChannel, pWall, 2, pWall->picnum, pWall->picnum + 1);
            runlist_AddRunRec(sRunChannels[nChannel].a, nWallFace,  0x70000);

            auto nSwitch = BuildSwPressWall(nChannel, BuildLink(1, 1), pWall);
            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 8: // Reverse switch
        {
            int nWallFace = BuildWallFace(nChannel, pWall, 2, pWall->picnum, pWall->picnum + 1);
            runlist_AddRunRec(sRunChannels[nChannel].a, nWallFace,  0x70000);

            auto nSwitch = BuildSwPressWall(nChannel, BuildLink(2, -1, 0), pWall);

            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 9: // Invisible switch
        {
            auto nSwitch = BuildSwPressWall(nChannel, BuildLink(2, 1, 1), pWall);
            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 10:
        {
            auto nSwitch = BuildSwPressWall(nChannel, BuildLink(2, -1, 0), pWall);
            runlist_AddRunRec(sRunChannels[nChannel].a,nSwitch.first, nSwitch.second);
            return;
        }

        case 12: // Twin star trek door
        case 14:
        case 16:
        case 19:
        case 20:
        {
            walltype* pLastWall = nullptr;
            walltype* p2ndLastWall = nullptr;

            auto pStart = pWall;

            while (1)
            {
                pWall = pWall->point2Wall(); // get the next (right side) wall point

                if (pStart == pWall) { // we've looped back around
                    break;
                }

                p2ndLastWall = pLastWall;
                pLastWall = pWall;
            }

            auto pWall2 = pStart->point2Wall();
            auto pWall3 = pWall2->point2Wall();
            auto pWall4 = pWall3->point2Wall();

            int nSlide = BuildSlide(nChannel, pStart, pLastWall, p2ndLastWall, pWall2, pWall3, pWall4);

            runlist_AddRunRec(sRunChannels[nChannel].a, nSlide, 0x80000);
            return;
        }

        case 24: // Waterfall
        {
            AddFlow(pWall, nPanSpeed, 3);
            return;
        }

        case 25: // Inverse waterfall
        {
            AddFlow(pWall, nPanSpeed, 2);
            return;
        }
    }
}

int runlist_CheckRadialDamage(DExhumedActor* pActor)
{
    if (pActor == pRadialActor) {
        return 0;
    }

    if (!(pActor->spr.cstat & CSTAT_SPRITE_BLOCK_ALL)) {
        return 0;
    }

    if (pActor->spr.statnum >= kMaxStatus || pRadialActor->spr.statnum >= kMaxStatus) {
        return 0;
    }

    if (pActor->spr.statnum != 100 && pActor == pRadialActor->pTarget) {
        return 0;
    }

    int x = (pActor->spr.pos.X - pRadialActor->spr.pos.X) >> 8;
    int y = (pActor->spr.pos.Y - pRadialActor->spr.pos.Y) >> 8;
    int z = (pActor->spr.pos.Z - pRadialActor->spr.pos.Z) >> 12;

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
        auto nCStat = pActor->spr.cstat;
        pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;

        if (((kStatExplodeTarget - pActor->spr.statnum) <= 1) ||
            cansee(pRadialActor->spr.pos.X,
                pRadialActor->spr.pos.Y,
                pRadialActor->spr.pos.Z - 512,
                pRadialActor->sector(),
                pActor->spr.pos.X,
                pActor->spr.pos.Y,
                pActor->spr.pos.Z - 8192,
                pActor->sector()))
        {
            edi = (nRadialDamage * (nDamageRadius - nDist)) / nDamageRadius;

            if (edi < 0) {
                edi = 0;
            }
            else if (edi > 20)
            {
                int nAngle = GetMyAngle(x, y);

                pActor->spr.xvel += (edi * bcos(nAngle)) >> 3;
                pActor->spr.yvel += (edi * bsin(nAngle)) >> 3;
                pActor->spr.zvel -= edi * 24;

                if (pActor->spr.zvel < -3584) {
                    pActor->spr.zvel = -3584;
                }
            }
        }

        pActor->spr.cstat = nCStat;
    }

    if (edi > 0x7FFF) {
        edi = 0x7FFF;
    }

    return edi;
}

void runlist_RadialDamageEnemy(DExhumedActor* pActor, int nDamage, int nRadius)
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

void runlist_DamageEnemy(DExhumedActor* pActor, DExhumedActor* pActor2, int nDamage)
{
	if (pActor->spr.statnum >= kMaxStatus) {
        return;
    }

    int nRun = pActor->spr.owner;
    if (nRun <= -1) {
        return;
    }

    int nPreCreaturesKilled = nCreaturesKilled;

    RunListEvent ev{};
    ev.pOtherActor = pActor2;
    ev.nDamage = nDamage * 4;
    runlist_SendMessage(nRun, -1, &ExhumedAI::Damage, &ev);

    // is there now one less creature? (has one died)
    if (nPreCreaturesKilled < nCreaturesKilled && pActor2 != nullptr)
    {
        if (pActor2->spr.statnum != 100) {
            return;
        }

        int nPlayer = GetPlayerFromActor(pActor2);
        PlayerList[nPlayer].nTauntTimer--;

        if (PlayerList[nPlayer].nTauntTimer <= 0)
        {
            // Do a taunt
            auto pPlayerActor = PlayerList[nPlayer].pActor;
            auto pSector = pPlayerActor->sector();

            if (!(pSector->Flag & kSectUnderwater))
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
