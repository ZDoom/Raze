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
int gNetFifoTail = 0;
int gNetFifoHead[8];
int gPredictTail = 0;
InputPacket gFifoInput[256][8];

void netResetToSinglePlayer(void)
{
    myconnectindex = connecthead = 0;
    gNetPlayers = numplayers = 1;
    connectpoint2[0] = -1;
    gGameOptions.nGameType = 0;
    UpdateNetworkMenus();
}

void netReset(void)
{
    lastTic = -1;
    gPredictTail = 0;
    gNetFifoTail = 0;
    memset(gNetFifoHead, 0, sizeof(gNetFifoHead));
}

void netBroadcastPlayerInfo(int nPlayer)
{
    PROFILE *pProfile = &gProfile[nPlayer];
    strcpy(pProfile->name, playername);
    pProfile->skill = gSkill;
    pProfile->nAutoAim = cl_autoaim;
    pProfile->nWeaponSwitch = cl_weaponswitch;
}

void netGetInput(void)
{
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
        if (gNetFifoHead[myconnectindex]-200 > gNetFifoHead[p])
            return;
    InputPacket &input = gFifoInput[gNetFifoHead[myconnectindex]&255][myconnectindex];
    input = gNetInput;
    gNetFifoHead[myconnectindex]++;
}

void netInitialize(bool bConsole)
{
    netReset();
    netResetToSinglePlayer();
}


END_BLD_NS
