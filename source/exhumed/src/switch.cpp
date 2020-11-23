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

short LinkCount = -1;
short SwitchCount = -1;

int8_t LinkMap[kMaxLinks][8];

struct Switch
{
    short field_0;
    short field_2;
    short nChannel;
    short nLink;
    short field_8;
    short nSector;
    short field_C;
    short nWall;
    short field_10;
    uint16_t field_12;
    short field_14;
    char pad[10];
};

Switch SwitchData[kMaxSwitches];

static SavegameHelper sghswitch("switch",
    SV(LinkCount),
    SV(SwitchCount),
    SA(LinkMap),
    SA(SwitchData),
    nullptr);

void InitLink()
{
    LinkCount = kMaxLinks;
}

int BuildLink(int nCount, ...)
{
    if (LinkCount <= 0) {
        return -1;
    }

    va_list list;
    va_start(list, nCount);

    LinkCount--;

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

        LinkMap[LinkCount][i] = (int8_t)ebx;
    }
    va_end(list);

    return LinkCount;
}

void InitSwitch()
{
    SwitchCount = kMaxSwitches;
    memset(SwitchData, 0, sizeof(SwitchData));
}

int BuildSwReady(int nChannel, short nLink)
{
    if (SwitchCount <= 0 || nLink < 0) {
        I_Error("Too many switch readys!\n");
        return -1;
    }

    SwitchCount--;
    SwitchData[SwitchCount].nChannel = nChannel;
    SwitchData[SwitchCount].nLink = nLink;

    return SwitchCount | 0x10000;
}

void FuncSwReady(int a, int, int nRun)
{
    short nSwitch = RunData[nRun].nVal;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    int nMessage = a & 0x7F0000;

    short nChannel = SwitchData[nSwitch].nChannel;
    short nLink = SwitchData[nSwitch].nLink;

    switch (nMessage)
    {
        case 0x10000:
            return;

        case 0x30000:
        {
            assert(sRunChannels[nChannel].c < 8);
            int8_t nVal = LinkMap[nLink][sRunChannels[nChannel].c];
            if (nVal >= 0) {
                runlist_ChangeChannel(nChannel, nVal);
            }

            break;
        }

        default:
            return;
    }
}

int BuildSwPause(int nChannel, int nLink, int ebx)
{
    for (int i = kMaxSwitches - 1; i >= SwitchCount; i--)
    {
        if (SwitchData[i].nChannel == nChannel && SwitchData[i].field_2 != 0) {
            return i | 0x20000;
        }
    }

    if (SwitchCount <= 0 || nLink < 0) {
        I_Error("Too many switches!\n");
        return -1;
    }

    SwitchCount--;

    SwitchData[SwitchCount].nChannel = nChannel;
    SwitchData[SwitchCount].nLink = nLink;
    SwitchData[SwitchCount].field_2 = ebx;
    SwitchData[SwitchCount].field_8 = -1;

    return SwitchCount | 0x20000;
}

void FuncSwPause(int a, int, int nRun)
{
    short nSwitch = RunData[nRun].nVal;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    int nMessage = a & 0x7F0000;

    short nChannel = SwitchData[nSwitch].nChannel;
    short nLink = SwitchData[nSwitch].nLink;

    switch (nMessage)
    {
        default:
            return;

        case 0x10000:
        {
            if (SwitchData[nSwitch].field_8 >= 0)
            {
                runlist_SubRunRec(SwitchData[nSwitch].field_8);
                SwitchData[nSwitch].field_8 = -1;
            }

            return;
        }

        case 0x20000:
        {
            SwitchData[nSwitch].field_0--;
            if (SwitchData[nSwitch].field_0 <= 0)
            {
                runlist_SubRunRec(SwitchData[nSwitch].field_8);
                SwitchData[nSwitch].field_8 = -1;

                assert(sRunChannels[nChannel].c < 8);
                assert(nLink < 1024);

                runlist_ChangeChannel(nChannel, LinkMap[nLink][sRunChannels[nChannel].c]);
            }

            return;
        }

        case 0x30000:
        {
            assert(sRunChannels[nChannel].c < 8);

            if (LinkMap[nLink][sRunChannels[nChannel].c] < 0) {
                return;
            }

            if (SwitchData[nSwitch].field_8 >= 0) {
                return;
            }

            SwitchData[nSwitch].field_8 = runlist_AddRunRec(NewRun, RunData[nRun].nMoves);

            int eax;

            if (SwitchData[nSwitch].field_2 <= 0)
            {
                eax = 100;
            }
            else
            {
                eax = SwitchData[nSwitch].field_2;
            }

            SwitchData[nSwitch].field_0 = eax;
            return;
        }
    }
}

int BuildSwStepOn(int nChannel, int nLink, int nSector)
{
    if (SwitchCount <= 0 || nLink < 0 || nSector < 0)
        I_Error("Too many switches!\n");

    int nSwitch = --SwitchCount;

    SwitchData[nSwitch].nChannel = nChannel;
    SwitchData[nSwitch].nLink = nLink;
    SwitchData[nSwitch].nSector = nSector;
    SwitchData[nSwitch].field_C = -1;

    return nSwitch | 0x30000;
}

void FuncSwStepOn(int a, int, int nRun)
{
    short nSwitch = RunData[nRun].nVal;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    short nLink = SwitchData[nSwitch].nLink;
    short nChannel = SwitchData[nSwitch].nChannel;
    short nSector = SwitchData[nSwitch].nSector;

    assert(sRunChannels[nChannel].c < 8);

    int8_t var_14 = LinkMap[nLink][sRunChannels[nChannel].c];

    int nMessage = a & 0x7F0000;

    switch (nMessage)
    {
        default:
            return;

        case 0x10000:
        {
            if (SwitchData[nSwitch].field_C >= 0)
            {
                runlist_SubRunRec(SwitchData[nSwitch].field_C);
                SwitchData[nSwitch].field_C = -1;
            }

            if (var_14 >= 0)
            {
                SwitchData[nSwitch].field_C = runlist_AddRunRec(sector[nSector].lotag - 1, RunData[nRun].nMoves);
            }

            return;
        }

        case 0x50000:
        {
            if (var_14 != sRunChannels[nChannel].c)
            {
                short nWall = sector[nSector].wallptr;
                PlayFXAtXYZ(StaticSound[nSwitchSound], wall[nWall].x, wall[nWall].y, sector[nSector].floorz, nSector);

                assert(sRunChannels[nChannel].c < 8);

                runlist_ChangeChannel(nChannel, LinkMap[nLink][sRunChannels[nChannel].c]);
            }
        }

        return;
    }

}

int BuildSwNotOnPause(int nChannel, int nLink, int nSector, int ecx)
{
    if (SwitchCount <= 0 || nLink < 0 || nSector < 0)
        I_Error("Too many switches!\n");

    int nSwitch = --SwitchCount;

    SwitchData[nSwitch].nChannel = nChannel;
    SwitchData[nSwitch].nLink    = nLink;
    SwitchData[nSwitch].field_2  = ecx;
    SwitchData[nSwitch].nSector  = nSector;
    SwitchData[nSwitch].field_8  = -1;
    SwitchData[nSwitch].field_C  = -1;

    return nSwitch | 0x40000;
}

void FuncSwNotOnPause(int a, int, int nRun)
{
    short nSwitch = RunData[nRun].nVal;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    int nMessage = a & 0x7F0000;

    short nChannel = SwitchData[nSwitch].nChannel;
    short nLink = SwitchData[nSwitch].nLink;

    switch (nMessage)
    {
        default:
            return;

        case 0x10000:
        {
            if (SwitchData[nSwitch].field_C >= 0)
            {
                runlist_SubRunRec(SwitchData[nSwitch].field_C);
                SwitchData[nSwitch].field_C = -1;
            }

            if (SwitchData[nSwitch].field_8 >= 0)
            {
                runlist_SubRunRec(SwitchData[nSwitch].field_8);
                SwitchData[nSwitch].field_8 = -1;
            }

            return;
        }

        case 0x20000:
        {
            SwitchData[nSwitch].field_0 -= 4;
            if (SwitchData[nSwitch].field_0 <= 0)
            {
                assert(sRunChannels[nChannel].c < 8);

                runlist_ChangeChannel(nChannel, LinkMap[nLink][sRunChannels[nChannel].c]);
            }

            return;
        }

        case 0x30000:
        {
            assert(sRunChannels[nChannel].c < 8);

            if (LinkMap[nLink][sRunChannels[nChannel].c] >= 0)
            {
                if (SwitchData[nSwitch].field_8 < 0)
                {
                    SwitchData[nSwitch].field_8 = runlist_AddRunRec(NewRun, RunData[nRun].nMoves);

                    short nSector = SwitchData[nSwitch].nSector;

                    SwitchData[nSwitch].field_0 = SwitchData[nSwitch].field_2;
                    SwitchData[nSwitch].field_C = runlist_AddRunRec(sector[nSector].lotag - 1, RunData[nRun].nMoves);
                }
            }

            return;
        }

        case 0x50000:
        {
            SwitchData[nSwitch].field_0 = SwitchData[nSwitch].field_2;
            return;
        }
    }
}

int BuildSwPressSector(int nChannel, int nLink, int nSector, int keyMask)
{
    if (SwitchCount <= 0 || nLink < 0 || nSector < 0)
        I_Error("Too many switches!\n");

    int nSwitch = --SwitchCount;

    SwitchData[nSwitch].nChannel = nChannel;
    SwitchData[nSwitch].nLink = nLink;
    SwitchData[nSwitch].nSector = nSector;
    SwitchData[nSwitch].field_12 = keyMask;
    SwitchData[nSwitch].field_C = -1;

    return nSwitch | 0x50000;
}

void FuncSwPressSector(int a, int, int nRun)
{
    short nSwitch = RunData[nRun].nVal;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    int nMessage = a & 0x7F0000;

    short nChannel = SwitchData[nSwitch].nChannel;
    short nLink = SwitchData[nSwitch].nLink;
    short nPlayer = a & 0xFFFF;

    switch (nMessage)
    {
        default:
            return;

        case 0x10000:
        {
            if (SwitchData[nSwitch].field_C >= 0)
            {
                runlist_SubRunRec(SwitchData[nSwitch].field_C);
                SwitchData[nSwitch].field_C = -1;
            }

            assert(sRunChannels[nChannel].c < 8);

            if (LinkMap[nLink][sRunChannels[nChannel].c] < 0) {
                return;
            }

            short nSector = SwitchData[nSwitch].nSector;

            SwitchData[nSwitch].field_C = runlist_AddRunRec(sector[nSector].lotag - 1, RunData[nRun].nMoves);
            return;
        }

        case 0x40000:
        {
            if ((PlayerList[nPlayer].keys & SwitchData[nSwitch].field_12) == SwitchData[nSwitch].field_12)
            {
                runlist_ChangeChannel(nChannel, LinkMap[nLink][sRunChannels[nChannel].c]);
            }
            else
            {
                if (SwitchData[nSwitch].field_12)
                {
                    short nSprite = PlayerList[nPlayer].nSprite;
                    PlayFXAtXYZ(StaticSound[nSwitchSound], sprite[nSprite].x, sprite[nSprite].y, 0, sprite[nSprite].sectnum, CHANF_LISTENERZ);

                    StatusMessage(300, "YOU NEED THE KEY FOR THIS DOOR");
                }
            }
        }
    }
}

int BuildSwPressWall(short nChannel, short nLink, short nWall)
{
    if (SwitchCount <= 0 || nLink < 0 || nWall < 0) {
        I_Error("Too many switches!\n");
    }

    SwitchCount--;

    SwitchData[SwitchCount].nChannel = nChannel;
    SwitchData[SwitchCount].nLink = nLink;
    SwitchData[SwitchCount].nWall = nWall;
    SwitchData[SwitchCount].field_10 = -1;
    SwitchData[SwitchCount].field_14 = 0;

    return SwitchCount | 0x60000;
}

void FuncSwPressWall(int a, int, int nRun)
{
    short nSwitch = RunData[nRun].nVal;
    assert(nSwitch >= 0 && nSwitch < kMaxSwitches);

    short nChannel = SwitchData[nSwitch].nChannel;
    short nLink = SwitchData[nSwitch].nLink;

    // TEMP
//	assert(nLink < 1024);
//	assert(nChannel < 8);

    int nMessage = a & 0x7F0000;

    switch (nMessage)
    {
        default:
            return;

        case 0x30000:
        {
            if (SwitchData[nSwitch].field_10 >= 0)
            {
                runlist_SubRunRec(SwitchData[nSwitch].field_10);
                SwitchData[nSwitch].field_10 = -1;
            }

            if (LinkMap[nLink][sRunChannels[nChannel].c] >= 0)
            {
                short nWall = SwitchData[nSwitch].nWall;
                SwitchData[nSwitch].field_10 = runlist_AddRunRec(wall[nWall].lotag - 1, RunData[nRun].nMoves);
            }

            return;
        }

        case 0x40000:
        {
            int8_t cx = LinkMap[nLink][sRunChannels[nChannel].c];

            runlist_ChangeChannel(nChannel, LinkMap[nLink][sRunChannels[nChannel].c]);

            if (cx < 0 || LinkMap[nLink][cx] < 0)
            {
                runlist_SubRunRec(SwitchData[nSwitch].field_10);
                SwitchData[nSwitch].field_10 = -1;
            }

            short nWall = SwitchData[nSwitch].nWall;
            short nSector = SwitchData[nSwitch].nSector; // CHECKME - where is this set??

            PlayFXAtXYZ(StaticSound[nSwitchSound], wall[nWall].x, wall[nWall].y, 0, nSector, CHANF_LISTENERZ);

            return;
        }
    }
}
END_PS_NS
