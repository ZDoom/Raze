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

#ifndef netplay_h_
#define netplay_h_

#ifdef _WIN32
// include this before enet does
# define NEED_WINSOCK2_H
# include "windows_inc.h"
#endif

#include "enet/enet.h"

// net packet specification/compatibility version
#define NETVERSION    1

extern ENetHost       *g_netClient;
extern ENetHost       *g_netServer;
extern ENetPeer       *g_netClientPeer;
extern char           g_netPassword[32];
extern int32_t        g_netDisconnect;
extern int32_t        g_netPlayersWaiting;
extern enet_uint16    g_netPort;
#ifndef NETCODE_DISABLE
extern int32_t        g_networkMode;
#else
#define g_networkMode 0
#endif
extern int32_t        g_netIndex;
extern int32_t        lastsectupdate[MAXSECTORS];
extern int32_t        lastupdate[MAXSPRITES];
extern int32_t        lastwallupdate[MAXWALLS];
extern int16_t		  g_netStatnums[];

#define NET_REVISIONS 64

enum netchan_t
{
    CHAN_MOVE,      // unreliable movement packets
    CHAN_GAMESTATE, // gamestate changes... frags, respawns, player names, etc
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
    PACKET_ACK,
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
    DISC_VERSION_MISMATCH,
    DISC_INVALID,
    DISC_SERVER_QUIT,
    DISC_SERVER_FULL,
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

#define NETMAXACTORS 1024

#pragma pack(push,1)
typedef struct
{

    uint32_t numActors;
    netactor_t actor[NETMAXACTORS];

} netmapstate_t;

typedef struct
{

    uint32_t numActors;
    uint32_t numToDelete;
    uint32_t fromRevision;
    uint32_t toRevision;
    char data[MAXSPRITES *sizeof(netactor_t)];

} netmapdiff_t;

extern netmapstate_t  *g_multiMapState[MAXPLAYERS];
extern netmapstate_t  *g_multiMapRevisions[NET_REVISIONS];
#pragma pack(pop)

#pragma pack(push,1)
typedef struct
{
    vec3_t pos;
    vec3_t opos;
    vec3_t vel;
    int16_t ang;
    int16_t horiz;
    int16_t horizoff;
    int16_t ping;
    int16_t playerindex;
    int16_t deadflag;
    int16_t playerquitflag;
} playerupdate_t;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct
{
    int8_t header;
    int8_t connection;
    int8_t level_number;
    int8_t volume_number;
    int8_t player_skill;
    int8_t monsters_off;
    int8_t respawn_monsters;
    int8_t respawn_items;
    int8_t respawn_inventory;
    int8_t marker;
    int8_t ffire;
    int8_t noexits;
    int8_t coop;
} newgame_t;
#pragma pack(pop)

extern newgame_t pendingnewgame;

#ifndef NETCODE_DISABLE

// Connect/Disconnect
void    Net_Connect(const char *srvaddr);
void    Net_Disconnect(void);
void    Net_ReceiveDisconnect(ENetEvent *event);

// Packet Handlers
#endif
void    Net_GetPackets(void);
#ifndef NETCODE_DISABLE
void    Net_HandleServerPackets(void);
void    Net_HandleClientPackets(void);
void    Net_ParseClientPacket(ENetEvent *event);
void    Net_ParseServerPacket(ENetEvent *event);
void    Net_ParsePacketCommon(uint8_t *pbuf, int32_t packbufleng, int32_t serverpacketp);

void    Net_SendAcknowledge(ENetPeer *client);
void    Net_ReceiveAcknowledge(uint8_t *pbuf, int32_t packbufleng);

void    Net_SendChallenge();
void    Net_ReceiveChallenge(uint8_t *pbuf, int32_t packbufleng, ENetEvent *event);

void    Net_SendNewPlayer(int32_t newplayerindex);
void    Net_ReceiveNewPlayer(uint8_t *pbuf, int32_t packbufleng);

void    Net_SendPlayerIndex(int32_t index, ENetPeer *peer);
void    Net_ReceivePlayerIndex(uint8_t *pbuf, int32_t packbufleng);

void    Net_SendClientInfo(void);
void    Net_ReceiveClientInfo(uint8_t *pbuf, int32_t packbufleng, int32_t fromserver);

void    Net_SendUserMapName(void);
void    Net_ReceiveUserMapName(uint8_t *pbuf, int32_t packbufleng);

netmapstate_t *Net_GetRevision(uint8_t revision, uint8_t cancreate);

void    Net_SendMapUpdate(void);
void    Net_ReceiveMapUpdate(ENetEvent *event);

void    Net_FillMapDiff(uint32_t fromRevision, uint32_t toRevision);
void	Net_SaveMapState(netmapstate_t *save);
void    Net_RestoreMapState();

void    Net_CopyToNet(int32_t i, netactor_t *netactor);
void    Net_CopyFromNet(int32_t i, netactor_t *netactor);
int32_t Net_ActorsAreDifferent(netactor_t *actor1, netactor_t *actor2);
int32_t Net_IsRelevantSprite(int32_t i);
int32_t Net_IsRelevantStat(int32_t stat);
int32_t Net_InsertSprite(int32_t sect, int32_t stat);
void    Net_DeleteSprite(int32_t spritenum);

void    Net_FillPlayerUpdate(playerupdate_t *update, int32_t player);
void    Net_ExtractPlayerUpdate(playerupdate_t *update, int32_t type);

void    Net_SendServerUpdates(void);
void    Net_ReceiveServerUpdate(ENetEvent *event);

void    Net_SendClientUpdate(void);
void    Net_ReceiveClientUpdate(ENetEvent *event);

void    Net_SendMessage(void);
void    Net_ReceiveMessage(uint8_t *pbuf, int32_t packbufleng);

void    Net_StartNewGame();
void    Net_NotifyNewGame();
void    Net_SendNewGame(int32_t frommenu, ENetPeer *peer);
void    Net_ReceiveNewGame(ENetEvent *event);

void    Net_FillNewGame(newgame_t *newgame, int32_t frommenu);
void    Net_ExtractNewGame(newgame_t *newgame, int32_t menuonly);

void    Net_SendMapVoteInitiate(void);
void    Net_ReceiveMapVoteInitiate(uint8_t *pbuf);

void    Net_SendMapVote(int32_t votefor);
void    Net_ReceiveMapVote(uint8_t *pbuf);
void    Net_CheckForEnoughVotes();

void    Net_SendMapVoteCancel(int32_t failed);
void    Net_ReceiveMapVoteCancel(uint8_t *pbuf);

//////////

void    Net_ResetPrediction(void);
void    Net_SpawnPlayer(int32_t player);
void    Net_SyncPlayer(ENetEvent *event);
void    Net_WaitForServer(void);
void    faketimerhandler(void);

#else

/* NETCODE_ENABLE is not defined */

// Connect/Disconnect
#define Net_Connect(...) ((void)0)
#define Net_Disconnect(...) ((void)0)
#define Net_ReceiveDisconnect(...) ((void)0)

// Packet Handlers
#define Net_HandleServerPackets(...) ((void)0)
#define Net_HandleClientPackets(...) ((void)0)
#define Net_ParseClientPacket(...) ((void)0)
#define Net_ParseServerPacket(...) ((void)0)
#define Net_ParsePacketCommon(...) ((void)0)

#define Net_SendAcknowledge(...) ((void)0)
#define Net_ReceiveAcknowledge(...) ((void)0)

#define Net_SendChallenge(...) ((void)0)
#define Net_ReceiveChallenge(...) ((void)0)

#define Net_SendNewPlayer(...) ((void)0)
#define Net_ReceiveNewPlayer(...) ((void)0)

#define Net_SendPlayerIndex(...) ((void)0)
#define Net_ReceivePlayerIndex(...) ((void)0)

#define Net_SendClientInfo(...) ((void)0)
#define Net_ReceiveClientInfo(...) ((void)0)

#define Net_SendUserMapName(...) ((void)0)
#define Net_ReceiveUserMapName(...) ((void)0)

#define Net_SendClientSync(...) ((void)0)
#define Net_ReceiveClientSync(...) ((void)0)

#define Net_SendMapUpdate(...) ((void)0)
#define Net_ReceiveMapUpdate(...) ((void)0)

#define Net_FillPlayerUpdate(...) ((void)0)
#define Net_ExtractPlayerUpdate(...) ((void)0)

#define Net_SendServerUpdates(...) ((void)0)
#define Net_ReceiveServerUpdate(...) ((void)0)

#define Net_SendClientUpdate(...) ((void)0)
#define Net_ReceiveClientUpdate(...) ((void)0)

#define Net_SendMessage(...) ((void)0)
#define Net_ReceiveMessage(...) ((void)0)

#define Net_StartNewGame(...) ((void)0)
#define Net_SendNewGame(...) ((void)0)
#define Net_ReceiveNewGame(...) ((void)0)

#define Net_FillNewGame(...) ((void)0)
#define Net_ExtractNewGame(...) ((void)0)

#define Net_SendMapVoteInitiate(...) ((void)0)
#define Net_ReceiveMapVoteInitiate(...) ((void)0)

#define Net_SendMapVote(...) ((void)0)
#define Net_ReceiveMapVote(...) ((void)0)
#define Net_CheckForEnoughVotes(...) ((void)0)

#define Net_SendMapVoteCancel(...) ((void)0)
#define Net_ReceiveMapVoteCancel(...) ((void)0)

//////////

#define Net_ResetPrediction(...) ((void)0)
#define Net_RestoreMapState(...) ((void)0)
#define Net_SyncPlayer(...) ((void)0)
#define Net_WaitForServer(...) ((void)0)

#define Net_ActorsAreDifferent(...) 0
#define Net_IsRelevantSprite(...) 0
#define Net_IsRelevantStat(...) 0
#define Net_InsertSprite(...) 0
#define Net_DeleteSprite(...) ((void)0)

#define Net_NotifyNewGame(...) ((void)0)

#endif

#endif // netplay_h_
