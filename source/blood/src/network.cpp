//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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
#include "ns.h"	// Must come before everything else!

#include "build.h"
#include "mmulti.h"
#include "pragmas.h"
#include "compat.h"
#include "controls.h"
#include "globals.h"
#include "network.h"
#include "player.h"
#include "seq.h"
#include "sound.h"
#include "view.h"
#include "menu.h"
#include "gamestate.h"

BEGIN_BLD_NS

MapRecord *gStartNewGame = 0;
PACKETMODE gPacketMode = PACKETMODE_1;
ClockTicks gNetFifoClock = 0;
int gNetFifoTail = 0;
int gNetFifoHead[8];
int gPredictTail = 0;
int gNetFifoMasterTail = 0;
GINPUT gFifoInput[256][8];
int myMinLag[8];
int otherMinLag = 0;
int myMaxLag = 0;
int gSendCheckTail = 0;
int gCheckTail = 0;
int gInitialNetPlayers = 0;
int gBufferJitter = 1;
int gPlayerReady[8];
bool bNoResend = true;
bool gRobust = false;
bool bOutOfSync = false;
bool ready2send = false;

NETWORKMODE gNetMode = NETWORK_NONE;
char gNetAddress[32];
// PORT-TODO: Use different port?
int gNetPort = kNetDefaultPort;

const short word_1328AC = 0x214;

void netResetToSinglePlayer(void)
{
    myconnectindex = connecthead = 0;
    gInitialNetPlayers = gNetPlayers = numplayers = 1;
    connectpoint2[0] = -1;
    gGameOptions.nGameType = 0;
    gNetMode = NETWORK_NONE;
    UpdateNetworkMenus();
}

void netReset(void)
{
    gNetFifoClock = gFrameClock = totalclock = 0;
    gNetFifoMasterTail = 0;
    gPredictTail = 0;
    gNetFifoTail = 0;
    memset(gNetFifoHead, 0, sizeof(gNetFifoHead));
    memset(myMinLag, 0, sizeof(myMinLag));
    otherMinLag = 0;
    myMaxLag = 0;
    gSendCheckTail = 0;
    gCheckTail = 0;
    bOutOfSync = 0;
    gBufferJitter = 1;
}

void netBroadcastPlayerInfo(int nPlayer)
{
    PROFILE *pProfile = &gProfile[nPlayer];
    strcpy(pProfile->name, playername);
    pProfile->skill = gSkill;
    pProfile->nAutoAim = cl_autoaim;
    pProfile->nWeaponSwitch = cl_weaponswitch;
    if (numplayers < 2)
        return;
}

void netGetInput(void)
{
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
        if (gNetFifoHead[myconnectindex]-200 > gNetFifoHead[p])
            return;
    GINPUT &input = gFifoInput[gNetFifoHead[myconnectindex]&255][myconnectindex];
    input = gNetInput;
    gNetFifoHead[myconnectindex]++;
    if (gGameOptions.nGameType == 0 || numplayers == 1)
    {
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            if (p != myconnectindex)
            {
                GINPUT *pInput1 = &gFifoInput[(gNetFifoHead[p]-1)&255][p];
                GINPUT *pInput2 = &gFifoInput[gNetFifoHead[p]&255][p];
                memcpy(pInput2, pInput1, sizeof(GINPUT));
                gNetFifoHead[p]++;
            }
        }
        return;
    }
}

void netInitialize(bool bConsole)
{
    memset(gPlayerReady, 0, sizeof(gPlayerReady));
    netReset();
    netResetToSinglePlayer();
}

void netDeinitialize(void)
{
}

void netPlayerQuit(int nPlayer)
{
}

END_BLD_NS
