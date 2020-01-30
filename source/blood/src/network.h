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

extern char packet[576];
extern bool gStartNewGame;
extern PACKETMODE gPacketMode;
extern ClockTicks gNetFifoClock;
extern int gNetFifoTail;
extern int gNetFifoHead[8];
extern int gPredictTail;
extern int gNetFifoMasterTail;
extern GINPUT gFifoInput[256][8];
extern int myMinLag[8];
extern int otherMinLag;
extern int myMaxLag;
extern unsigned int gChecksum[4];
extern unsigned int gCheckFifo[256][8][4];
extern int gCheckHead[8];
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


struct PKT_STARTGAME {
    short version;
    char gameType, difficulty, monsterSettings, weaponSettings, itemSettings, respawnSettings;
    char episodeId, levelId;
    int unk;
    char userMap, userMapName[13];
    int weaponsV10x;
    bool bFriendlyFire;
    bool bKeepKeysOnRespawn;
};

extern PKT_STARTGAME gPacketStartGame;


inline void PutPacketByte(char *&p, int a2)
{
    //dassert(p - packet + 1 < sizeof(packet));
    *p++ = a2;
}

inline void PutPacketWord(char *&p, int a2)
{
    //dassert(p - packet + 2 < sizeof(packet));
    *(short*)p = a2;
    p += 2;
}

inline void PutPacketDWord(char *&p, int a2)
{
    //dassert(p - packet + 4 < sizeof(packet));
    *(int*)p = a2;
    p += 4;
}

inline void PutPacketBuffer(char *&p, const void *pBuffer, int size)
{
    //dassert(p + size < packet + sizeof(packet));
    memcpy(p, pBuffer, size);
    p += size;
}

inline char GetPacketByte(char *&p)
{
    return *p++;
}

inline short GetPacketWord(char *&p)
{
    short t = *(short*)p;
    p += 2;
    return t;
}

inline int GetPacketDWord(char *&p)
{
    int t = *(int*)p;
    p += 4;
    return t;
}

inline void GetPacketBuffer(char *&p, void *pBuffer, int size)
{
    //dassert(p + size < packet + sizeof(packet));
    memcpy(pBuffer, p, size);
    p += size;
}

void sub_79760(void);
void netResetToSinglePlayer(void);
void netBroadcastMessage(int nPlayer, const char *pzMessage);
void netWaitForEveryone(char a1);
void sub_7AC28(const char *pzString);
void netGetPackets(void);
void netBroadcastTaunt(int nPlayer, int nTaunt);
void CalcGameChecksum(void);
void netBroadcastPlayerLogoff(int nPlayer);
void netBroadcastMyLogoff(bool bRestart);
void netInitialize(bool bConsole);
void netBroadcastPlayerInfo(int nPlayer);
void netCheckSync(void);
void netMasterUpdate(void);
void netGetInput(void);
void netPlayerQuit(int nPlayer);
void netUpdate(void);
void netDeinitialize(void);
void netBroadcastNewGame(void);

END_BLD_NS
