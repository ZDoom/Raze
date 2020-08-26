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
#pragma once
#include "compat.h"
#include "build.h"
#include "controls.h"
#include "mapinfo.h"

BEGIN_BLD_NS

enum PACKETMODE {
    PACKETMODE_0 = 0,
    PACKETMODE_1,
    PACKETMODE_2,
    PACKETMODE_3,
};

enum NETWORKMODE {
    NETWORK_NONE = 0,
    NETWORK_SERVER,
    NETWORK_CLIENT
};

#define kNetDefaultPort 23513

extern MapRecord *gStartNewGame;
extern PACKETMODE gPacketMode;
extern int gNetFifoTail;
extern int gNetFifoHead[8];
extern int gPredictTail;
extern int gNetFifoMasterTail;
extern InputPacket gFifoInput[256][8];
extern int myMinLag[8];
extern int otherMinLag;
extern int myMaxLag;
extern int gSendCheckTail;
extern int gCheckTail;
extern int gInitialNetPlayers;
extern int gBufferJitter;
extern int gPlayerReady[8];
extern bool bNoResend;
extern bool gRobust;
extern bool bOutOfSync;
extern bool ready2send;
extern NETWORKMODE gNetMode;
extern char gNetAddress[32];
extern int gNetPort;


void netReset(void);
void netResetToSinglePlayer(void);
inline void netWaitForEveryone(char a1) {}
inline void netGetPackets(void) {}
inline void CalcGameChecksum(void) {}
inline void netBroadcastPlayerLogoff(int nPlayer) {}
inline void netBroadcastMyLogoff(bool bRestart) {}
void netInitialize(bool bConsole);
void netBroadcastPlayerInfo(int nPlayer);
inline void netCheckSync(void) {}
void netMasterUpdate(void);
void netGetInput(void);
void netPlayerQuit(int nPlayer);
void netDeinitialize(void);

END_BLD_NS
