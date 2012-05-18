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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#ifndef __netplay_h__
#define __netplay_h__

#include "enet/enet.h"

#define NET_REVISIONS 64

enum netchan_t
{
    CHAN_MOVE,      // unreliable movement packets
    CHAN_GAMESTATE, // gamestate changes... frags, respawns, player names, etc
    CHAN_SYNC,      // client join sync packets
    CHAN_CHAT,      // chat and RTS
    CHAN_MISC,      // whatever else
    CHAN_MAX
};

enum DukePacket_t
{
    PACKET_MASTER_TO_SLAVE,
    PACKET_SLAVE_TO_MASTER,

    PACKET_NUM_PLAYERS,
    PACKET_PLAYER_INDEX,
    PACKET_PLAYER_DISCONNECTED,
    PACKET_PLAYER_SPAWN,
    PACKET_FRAG,
    PACKET_REQUEST_GAMESTATE,
    PACKET_VERSION,
    PACKET_AUTH,
    PACKET_PLAYER_PING,
    PACKET_PLAYER_READY,
    PACKET_MAP_STREAM,

    // any packet with an ID higher than PACKET_BROADCAST is rebroadcast by server
    // so hacked clients can't create fake server packets and get the server to
    // send them to everyone
    // newer versions of the netcode also make this determination based on which
    // channel the packet was broadcast on

    PACKET_BROADCAST,
    PACKET_NEW_GAME,
    PACKET_RTS,
    PACKET_CLIENT_INFO,
    PACKET_MESSAGE,
    PACKET_USER_MAP,

    PACKET_MAP_VOTE,
    PACKET_MAP_VOTE_INITIATE,
    PACKET_MAP_VOTE_CANCEL,
};

enum netdisconnect_t
{
    DISC_BAD_PASSWORD = 1,
    DISC_KICKED,
    DISC_BANNED
};

enum netmode_t
{
    NET_CLIENT = 0,
    NET_SERVER,
    NET_DEDICATED_CLIENT, // client on dedicated server
    NET_DEDICATED_SERVER
};

#pragma pack(push,1)
typedef struct {
    int32_t revision;
    int32_t animategoal[MAXANIMATES], animatevel[MAXANIMATES], g_animateCount;
    int32_t animateptr[MAXANIMATES];
//    int32_t lockclock;
    int32_t msx[2048], msy[2048];
    int32_t randomseed, g_globalRandom;

    int16_t SpriteDeletionQueue[1024],g_spriteDeleteQueuePos;
    int16_t animatesect[MAXANIMATES];
    int16_t cyclers[MAXCYCLERS][6];
    int16_t g_mirrorWall[64], g_mirrorSector[64], g_mirrorCount;
    int16_t g_numAnimWalls;
    int16_t g_numCyclers;
    int16_t headspritesect[MAXSECTORS+1];
    int16_t headspritestat[MAXSTATUS+1];
    int16_t nextspritesect[MAXSPRITES];
    int16_t nextspritestat[MAXSPRITES];
    int16_t numsectors;
    int16_t numwalls;
    int16_t prevspritesect[MAXSPRITES];
    int16_t prevspritestat[MAXSPRITES];

    uint8_t g_earthquakeTime;
    uint8_t g_numPlayerSprites;
    uint8_t scriptptrs[MAXSPRITES];

    netactor_t actor[MAXSPRITES];
    playerspawn_t g_playerSpawnPoints[MAXPLAYERS];
    animwalltype animwall[MAXANIMWALLS];
    sectortype sector[MAXSECTORS];
    spriteext_t spriteext[MAXSPRITES];
    spritetype sprite[MAXSPRITES];
    walltype wall[MAXWALLS];
    uint32_t crc;
} netmapstate_t;

extern netmapstate_t  *g_multiMapState[MAXPLAYERS];
extern netmapstate_t  *g_multiMapRevisions[NET_REVISIONS];
#pragma pack(pop)

extern uint32_t        g_netMapRevision;
extern ENetHost       *g_netClient;
extern ENetHost       *g_netServer;
extern ENetPeer       *g_netClientPeer;
extern char           g_netPassword[32];
extern int32_t        g_netDisconnect;
extern int32_t        g_netPlayersWaiting;
extern int32_t        g_netPort;
extern int32_t        g_networkMode;
extern int32_t        g_netSync;
extern int32_t        lastsectupdate[MAXSECTORS];
extern int32_t        lastupdate[MAXSPRITES];
extern int32_t        lastwallupdate[MAXWALLS];
extern int16_t        g_netStatnums[10];


int32_t Net_PackSprite(int32_t i,uint8_t *pbuf);
int32_t Net_UnpackSprite(int32_t i,uint8_t *pbuf);
void    Net_ClientMove(void);
void    Net_Connect(const char *srvaddr);
void    Net_Disconnect(void);
void    Net_EnterMessage(void);
void    Net_GetPackets(void);
void    Net_NewGame(int32_t volume,int32_t level);
void    Net_ParseClientPacket(ENetEvent *event);
void    Net_ParseServerPacket(ENetEvent *event);
void    Net_ResetPrediction(void);
void    Net_RestoreMapState(netmapstate_t *save);
void    Net_SendClientInfo(void);
void    Net_SendUserMapName(void);
void    Net_StreamLevel(void);
void    Net_SyncPlayer(ENetEvent *event);
void    Net_UpdateClients(void);
void    Net_WaitForServer(void);
void    faketimerhandler(void);
#endif
