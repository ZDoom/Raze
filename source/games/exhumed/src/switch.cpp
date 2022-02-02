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
#include "player.h"
#include "sound.h"
#include <string.h>
#include <assert.h>

BEGIN_PS_NS

enum { kMaxSwitches = 1024 };

int SwitchCount = -1;

struct Link
{
    int8_t v[8];
};

struct Switch
{
    walltype* pWall;
    sectortype* pSector;
    int nChannel;
    int nLink;
    int16_t nWaitTimer;
    int16_t nWait;
    int16_t nRunPtr;
    int16_t nRun2;

    int16_t nRun3;
    uint16_t nKeyMask;
};

TArray<Link> LinkMap;
Switch SwitchData[kMaxSwitches];

FSerializer& Serialize(FSerializer& arc, const char* keyname, Link& w, Link* def)
{
    arc.Array(keyname, w.v, 8);
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, Switch& w, Switch* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("waittimer", w.nWaitTimer)
            ("wait", w.nWait)
            ("channel", w.nChannel)
            ("link", w.nLink)
            ("runptr", w.nRunPtr)
            ("sector", w.pSector)
            ("run2", w.nRun2)
            ("wall", w.pWall)
            ("run3", w.nRun3)
            ("keymask", w.nKeyMask)
            .EndObject();
    }
    return arc;
}

void SerializeSwitch(FSerializer& arc)
{
    arc("switchcount", SwitchCount)
        .Array("switch", SwitchData + SwitchCount, kMaxSwitches - SwitchCount)
        ("linkmap", LinkMap);
}

void InitLink()
{
    LinkMap.Clear();
}

int BuildLink(int nCount, ...)
{
    va_list list;
    va_start(list, nCount);

    unsigned LinkCount = LinkMap.Reserve(1);

    for (int i = 0; i < 8; i++)
    {
        int ebx;

        if (i >= nCount)
        {
            ebx = -1;
        }
        else
        {
            ebx = va_arg(list, int);
        }

        LinkMap[LinkCount].v[i] = (int8_t)ebx;
    }
    va_end(list);

    return LinkCount;
}

void InitSwitch()
{
    SwitchCount = kMaxSwitches;
    memset(SwitchData, 0, sizeof(SwitchData));
}

std::pair<int, int> BuildSwReady(int nChannel, int nLink)
{
    if (SwitchCount <= 0 || nLink < 0) {
        I_Error("Too many switch readys!\n");
    }

    SwitchCount--;
    SwitchData[SwitchCount].nChannel = nChannel;
    SwitchData[SwitchCount].nLink = nLink;

    return { SwitchCount, 0x10000 };
}

void AISWReady::Process(RunListEvent* ev)
{
    int nSwitch = RunData[ev->nRun].nObjIndex;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    int nChannel = SwitchData[nSwitch].nChannel;
    int nLink = SwitchData[nSwitch].nLink;

    assert(sRunChannels[nChannel].c < 8);
    int8_t nVal = LinkMap[nLink].v[sRunChannels[nChannel].c];
    if (nVal >= 0) {
        runlist_ChangeChannel(nChannel, nVal);
    }
}

std::pair<int, int> BuildSwPause(int nChannel, int nLink, int ebx)
{
    for (int i = kMaxSwitches - 1; i >= SwitchCount; i--)
    {
        if (SwitchData[i].nChannel == nChannel && SwitchData[i].nWait != 0) {
            return { i, 0x20000 };
        }
    }

    if (SwitchCount <= 0 || nLink < 0) {
        I_Error("Too many switches!\n");
    }

    SwitchCount--;

    SwitchData[SwitchCount].nChannel = nChannel;
    SwitchData[SwitchCount].nLink = nLink;
    SwitchData[SwitchCount].nWait = ebx;
    SwitchData[SwitchCount].nRunPtr = -1;

    return { SwitchCount, 0x20000 };
}

void AISWPause::ProcessChannel(RunListEvent* ev)
{
    int nSwitch = RunData[ev->nRun].nObjIndex;
    if (SwitchData[nSwitch].nRunPtr >= 0)
    {
        runlist_SubRunRec(SwitchData[nSwitch].nRunPtr);
        SwitchData[nSwitch].nRunPtr = -1;
    }
}

void AISWPause::Tick(RunListEvent* ev)
{
    int nSwitch = RunData[ev->nRun].nObjIndex;
    int nChannel = SwitchData[nSwitch].nChannel;
    int nLink = SwitchData[nSwitch].nLink;

    SwitchData[nSwitch].nWaitTimer--;
    if (SwitchData[nSwitch].nWaitTimer <= 0)
    {
        runlist_SubRunRec(SwitchData[nSwitch].nRunPtr);
        SwitchData[nSwitch].nRunPtr = -1;

        assert(sRunChannels[nChannel].c < 8);
        assert(nLink < 1024);

        runlist_ChangeChannel(nChannel, LinkMap[nLink].v[sRunChannels[nChannel].c]);
    }
}

void AISWPause::Process(RunListEvent* ev)
{
    int nSwitch = RunData[ev->nRun].nObjIndex;
    int nChannel = SwitchData[nSwitch].nChannel;
    int nLink = SwitchData[nSwitch].nLink;
    assert(sRunChannels[nChannel].c < 8);

    if (LinkMap[nLink].v[sRunChannels[nChannel].c] < 0) {
        return;
    }

    if (SwitchData[nSwitch].nRunPtr >= 0) {
        return;
    }

    SwitchData[nSwitch].nRunPtr = runlist_AddRunRec(NewRun, &RunData[ev->nRun]);

    int eax;

    if (SwitchData[nSwitch].nWait <= 0)
    {
        eax = 100;
    }
    else
    {
        eax = SwitchData[nSwitch].nWait;
    }

    SwitchData[nSwitch].nWaitTimer = eax;
}

std::pair<int, int> BuildSwStepOn(int nChannel, int nLink, sectortype* pSector)
{
    if (SwitchCount <= 0 || nLink < 0 || pSector == nullptr)
        I_Error("Too many switches!\n");

    int nSwitch = --SwitchCount;

    SwitchData[nSwitch].nChannel = nChannel;
    SwitchData[nSwitch].nLink = nLink;
    SwitchData[nSwitch].pSector = pSector;
    SwitchData[nSwitch].nRun2 = -1;

    return { nSwitch , 0x30000 };
}

void AISWStepOn::ProcessChannel(RunListEvent* ev)
{
    int nSwitch = RunData[ev->nRun].nObjIndex;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    int nLink = SwitchData[nSwitch].nLink;
    int nChannel = SwitchData[nSwitch].nChannel;
    auto pSector =SwitchData[nSwitch].pSector;

    assert(sRunChannels[nChannel].c < 8);

    int8_t var_14 = LinkMap[nLink].v[sRunChannels[nChannel].c];

    if (SwitchData[nSwitch].nRun2 >= 0)
    {
        runlist_SubRunRec(SwitchData[nSwitch].nRun2);
        SwitchData[nSwitch].nRun2 = -1;
    }

    if (var_14 >= 0)
    {
        SwitchData[nSwitch].nRun2 = runlist_AddRunRec(pSector->lotag - 1, &RunData[ev->nRun]);
    }
}

void AISWStepOn::TouchFloor(RunListEvent* ev)
{
    int nSwitch = RunData[ev->nRun].nObjIndex;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    int nLink = SwitchData[nSwitch].nLink;
    int nChannel = SwitchData[nSwitch].nChannel;
    auto pSector = SwitchData[nSwitch].pSector;

    assert(sRunChannels[nChannel].c < 8);

    int8_t var_14 = LinkMap[nLink].v[sRunChannels[nChannel].c];

    if (var_14 != sRunChannels[nChannel].c)
    {
        auto pWall = pSector->firstWall();
        PlayFXAtXYZ(StaticSound[nSwitchSound], pWall->wall_int_pos().X, pWall->wall_int_pos().Y, pSector->int_floorz());

        assert(sRunChannels[nChannel].c < 8);

        runlist_ChangeChannel(nChannel, LinkMap[nLink].v[sRunChannels[nChannel].c]);
    }
}

std::pair<int, int> BuildSwNotOnPause(int nChannel, int nLink, sectortype* pSector, int ecx)
{
    if (SwitchCount <= 0 || nLink < 0 || pSector == nullptr)
        I_Error("Too many switches!\n");

    int nSwitch = --SwitchCount;

    SwitchData[nSwitch].nChannel = nChannel;
    SwitchData[nSwitch].nLink = nLink;
    SwitchData[nSwitch].nWait = ecx;
    SwitchData[nSwitch].pSector = pSector;
    SwitchData[nSwitch].nRunPtr = -1;
    SwitchData[nSwitch].nRun2 = -1;

    return { nSwitch, 0x40000 };
}

void AISWNotOnPause::ProcessChannel(RunListEvent* ev)
{
    int nSwitch = RunData[ev->nRun].nObjIndex;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    if (SwitchData[nSwitch].nRun2 >= 0)
    {
        runlist_SubRunRec(SwitchData[nSwitch].nRun2);
        SwitchData[nSwitch].nRun2 = -1;
    }

    if (SwitchData[nSwitch].nRunPtr >= 0)
    {
        runlist_SubRunRec(SwitchData[nSwitch].nRunPtr);
        SwitchData[nSwitch].nRunPtr = -1;
    }

    return;
}

void AISWNotOnPause::Tick(RunListEvent* ev)
{
    int nSwitch = RunData[ev->nRun].nObjIndex;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    int nChannel = SwitchData[nSwitch].nChannel;
    int nLink = SwitchData[nSwitch].nLink;

    SwitchData[nSwitch].nWaitTimer -= 4;
    if (SwitchData[nSwitch].nWaitTimer <= 0)
    {
        assert(sRunChannels[nChannel].c < 8);

        runlist_ChangeChannel(nChannel, LinkMap[nLink].v[sRunChannels[nChannel].c]);
    }
}

void AISWNotOnPause::Process(RunListEvent* ev)
{
    int nSwitch = RunData[ev->nRun].nObjIndex;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    int nChannel = SwitchData[nSwitch].nChannel;
    int nLink = SwitchData[nSwitch].nLink;

    assert(sRunChannels[nChannel].c < 8);

    if (LinkMap[nLink].v[sRunChannels[nChannel].c] >= 0)
    {
        if (SwitchData[nSwitch].nRunPtr < 0)
        {
            SwitchData[nSwitch].nRunPtr = runlist_AddRunRec(NewRun, &RunData[ev->nRun]);

            auto pSector = SwitchData[nSwitch].pSector;

            SwitchData[nSwitch].nWaitTimer = SwitchData[nSwitch].nWait;
            SwitchData[nSwitch].nRun2 = runlist_AddRunRec(pSector->lotag - 1, &RunData[ev->nRun]);
        }
    }
}

void AISWNotOnPause::TouchFloor(RunListEvent* ev)
{
    int nSwitch = RunData[ev->nRun].nObjIndex;

    SwitchData[nSwitch].nWaitTimer = SwitchData[nSwitch].nWait;
    return;
}

std::pair<int, int> BuildSwPressSector(int nChannel, int nLink, sectortype* pSector, int keyMask)
{
    if (SwitchCount <= 0 || nLink < 0 || pSector == nullptr)
        I_Error("Too many switches!\n");

    int nSwitch = --SwitchCount;

    SwitchData[nSwitch].nChannel = nChannel;
    SwitchData[nSwitch].nLink = nLink;
    SwitchData[nSwitch].pSector = pSector;
    SwitchData[nSwitch].nKeyMask = keyMask;
    SwitchData[nSwitch].nRun2 = -1;

    return { nSwitch, 0x50000 };
}

void AISWPressSector::ProcessChannel(RunListEvent* ev)
{
    int nSwitch = RunData[ev->nRun].nObjIndex;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    int nChannel = SwitchData[nSwitch].nChannel;
    int nLink = SwitchData[nSwitch].nLink;

    if (SwitchData[nSwitch].nRun2 >= 0)
    {
        runlist_SubRunRec(SwitchData[nSwitch].nRun2);
        SwitchData[nSwitch].nRun2 = -1;
    }

    assert(sRunChannels[nChannel].c < 8);

    if (LinkMap[nLink].v[sRunChannels[nChannel].c] < 0) {
        return;
    }

    auto pSector = SwitchData[nSwitch].pSector;

    SwitchData[nSwitch].nRun2 = runlist_AddRunRec(pSector->lotag - 1, &RunData[ev->nRun]);
}

void AISWPressSector::Use(RunListEvent* ev)
{
    int nSwitch = RunData[ev->nRun].nObjIndex;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    int nChannel = SwitchData[nSwitch].nChannel;
    int nLink = SwitchData[nSwitch].nLink;
    int nPlayer = ev->nParam;

    if ((PlayerList[nPlayer].keys & SwitchData[nSwitch].nKeyMask) == SwitchData[nSwitch].nKeyMask)
    {
        runlist_ChangeChannel(nChannel, LinkMap[nLink].v[sRunChannels[nChannel].c]);
    }
    else
    {
        if (SwitchData[nSwitch].nKeyMask)
        {
            auto& pos = PlayerList[nPlayer].pActor->int_pos();
            PlayFXAtXYZ(StaticSound[nSwitchSound], pos.X, pos.Y, 0, CHANF_LISTENERZ);

            StatusMessage(300, GStrings("TXT_EX_NEEDKEY"));
        }
    }

}

std::pair<int, int> BuildSwPressWall(int nChannel, int nLink, walltype* pWall)
{
    if (SwitchCount <= 0 || nLink < 0 || !pWall) {
        I_Error("Too many switches!\n");
    }

    SwitchCount--;

    SwitchData[SwitchCount].nChannel = nChannel;
    SwitchData[SwitchCount].nLink = nLink;
    SwitchData[SwitchCount].pWall = pWall;
    SwitchData[SwitchCount].nRun3 = -1;
    SwitchData[SwitchCount].pSector = nullptr;

    return { SwitchCount, 0x60000 };
}

void AISWPressWall::Process(RunListEvent* ev)
{
    int nSwitch = RunData[ev->nRun].nObjIndex;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    int nChannel = SwitchData[nSwitch].nChannel;
    int nLink = SwitchData[nSwitch].nLink;

    if (SwitchData[nSwitch].nRun3 >= 0)
    {
        runlist_SubRunRec(SwitchData[nSwitch].nRun3);
        SwitchData[nSwitch].nRun3 = -1;
    }

    if (LinkMap[nLink].v[sRunChannels[nChannel].c] >= 0)
    {
        auto pWall = SwitchData[nSwitch].pWall;
        SwitchData[nSwitch].nRun3 = runlist_AddRunRec(pWall->lotag - 1, &RunData[ev->nRun]);
    }
}

void AISWPressWall::Use(RunListEvent* ev)
{
    int nSwitch = RunData[ev->nRun].nObjIndex;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    int nChannel = SwitchData[nSwitch].nChannel;
    int nLink = SwitchData[nSwitch].nLink;

    int8_t cx = LinkMap[nLink].v[sRunChannels[nChannel].c];

    runlist_ChangeChannel(nChannel, LinkMap[nLink].v[sRunChannels[nChannel].c]);

    if (cx < 0 || LinkMap[nLink].v[cx] < 0)
    {
        runlist_SubRunRec(SwitchData[nSwitch].nRun3);
        SwitchData[nSwitch].nRun3 = -1;
    }

    auto pWall = SwitchData[nSwitch].pWall;
    auto pSector = SwitchData[nSwitch].pSector;

    PlayFXAtXYZ(StaticSound[nSwitchSound], pWall->wall_int_pos().X, pWall->wall_int_pos().Y, 0, CHANF_LISTENERZ);
}

END_PS_NS
