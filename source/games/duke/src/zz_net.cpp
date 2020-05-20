//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
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

#ifndef NETWORK_DISABLE
#include "enet.h"
#endif

#include "duke3d.h"
#include "game.h"
#include "gamedef.h"
#include "net.h"
#include "premap.h"
#include "savegame.h"

#include "m_crc32.h"
#include "mapinfo.h"

BEGIN_DUKE_NS

#define TIMERUPDATESIZ 32

ENetHost *g_netServer = NULL;
ENetHost *g_netClient = NULL;
ENetPeer *g_netClientPeer = NULL;
ENetPeer* g_netPlayerPeer[MAXPLAYERS];
enet_uint16 g_netPort = 23513;
int32_t g_netDisconnect = 0;
char g_netPassword[32];
int32_t g_netPlayersWaiting = 0;
int32_t g_netIndex = 2;
newgame_t pendingnewgame;

void faketimerhandler(void) {}

void Net_GetPackets(void)
{
    if (g_netDisconnect)
    {
        g_netDisconnect = 0;

        if (g_gameQuit)
            G_GameExit(" ");

        return;
    }
}

END_DUKE_NS
