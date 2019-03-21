//-------------------------------------------------------------------------
/*
Copyright (C) 2017 EDuke32 developers and contributors

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

// As of Nov. of 2018, the version of gcc that the Makefile is using is too old for [[maybe_unused]],
// but __attribute__ is GNU C only.
#ifdef __GNUC__
#define EDUKE32_UNUSED __attribute__((unused))
#else
#define EDUKE32_UNUSED
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
extern int32_t        g_networkMode;
extern int32_t        g_netIndex;

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
    PACKET_WORLD_UPDATE, //[75]

    // any packet with an ID higher than PACKET_BROADCAST is rebroadcast by server
    // so hacked clients can't create fake server packets and get the server to
    // send them to everyone
    // newer versions of the netcode also make this determination based on which
    // channel the packet was broadcast on

    PACKET_BROADCAST,
    PACKET_NEW_GAME, // [S->C]
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


//[75]

typedef struct netWall_s
{
    uint16_t netIndex;

    int32_t	x,
        y,
        point2,
        nextwall,
        nextsector,

        cstat,
        picnum,
        overpicnum,
        shade,
        pal,

        xrepeat,
        yrepeat,
        xpanning,
        ypanning,
        lotag,

        hitag,
        extra;

} netWall_t;

// sector struct with all 32 bit entries
typedef struct netSector_s
{
    uint16_t netIndex;

    int32_t	wallptr,
        wallnum,
        ceilingz,
        floorz,
        ceilingstat,

        floorstat,
        ceilingpicnum,
        ceilingheinum,
        ceilingshade,
        ceilingpal,

        ceilingxpanning,
        ceilingypanning,
        floorpicnum,
        floorheinum,
        floorshade,

        floorpal,
        floorxpanning,
        floorypanning,
        visibility,
        fogpal,

        lotag,
        hitag,
        extra;
} netSector_t;

const uint64_t cSnapshotMemUsage = NET_REVISIONS *	(
                                                            (sizeof(netWall_t) * MAXWALLS)
                                                        +   (sizeof(netSector_t)  * MAXSECTORS)
                                                        +   (sizeof(netactor_t) * MAXSPRITES)
                                                    );


#pragma pack(push,1)
typedef struct netmapstate_s
{
    uint32_t revisionNumber;
    int32_t maxActorIndex;
    netactor_t actor[MAXSPRITES];
    netWall_t wall[MAXWALLS];
    netSector_t sector[MAXSECTORS];

} netmapstate_t;

#pragma pack(pop)

#pragma pack(push,1)
typedef struct playerupdate_s
{
    vec3_t pos;
    vec3_t opos;
    vec3_t vel;

    fix16_t q16ang;
    fix16_t q16horiz;
    fix16_t q16horizoff;

    int16_t ping;
    int16_t playerindex;
    int16_t deadflag;
    int16_t playerquitflag;
} playerupdate_t;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct newgame_s
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

// Packet Handlers
#endif
void    Net_GetPackets(void);
#ifndef NETCODE_DISABLE

void    Net_SendClientInfo(void);

void    Net_SendUserMapName(void);

void    Net_SendMapUpdate(void);

int32_t Net_InsertSprite(int32_t sect, int32_t stat);
void    Net_DeleteSprite(int32_t spritenum);

void    Net_SendServerUpdates(void);

void    Net_SendClientUpdate(void);

void    Net_SendMessage(void);

void    Net_StartNewGame();
void    Net_NotifyNewGame();
void    Net_SendNewGame(int32_t frommenu, ENetPeer *peer);

void    Net_FillNewGame(newgame_t *newgame, int32_t frommenu);

void    Net_SendMapVoteInitiate(void);

void    Net_SendMapVote(int32_t votefor);

void    Net_SendMapVoteCancel(int32_t failed);

void    Net_StoreClientState(void);

//////////

void    Net_ResetPrediction(void);
void    Net_SpawnPlayer(int32_t player);
void    Net_WaitForServer(void);
void    faketimerhandler(void);

void Net_InitMapStateHistory();
void Net_AddWorldToInitialSnapshot();

// Debugging
int32_t Dbg_PacketSent(enum DukePacket_t iPacketType);

void DumpMapStateHistory();

void Net_WaitForInitialSnapshot();

#else

// note: don't include faketimerhandler in this

#define Net_Connect(...) ((void)0)

#define Net_SendClientInfo(...) ((void)0)

#define Net_SendUserMapName(...) ((void)0)

#define Net_SendClientSync(...) ((void)0)
#define Net_ReceiveClientSync(...) ((void)0)

#define Net_SendMapUpdates(...) ((void)0)
#define Net_ReceiveMapUpdate(...) ((void)0)

#define Net_SendServerUpdates(...) ((void)0)

#define Net_SendClientUpdate(...) ((void)0)

#define Net_SendMessage(...) ((void)0)

#define Net_StartNewGame(...) ((void)0)
#define Net_SendNewGame(...) ((void)0)

#define Net_FillNewGame(...) ((void)0)

#define Net_SendMapVoteInitiate(...) ((void)0)

#define Net_SendMapVote(...) ((void)0)

#define Net_SendMapVoteCancel(...) ((void)0)

#define Net_ResetPrediction(...) ((void)0)
#define Net_RestoreMapState(...) ((void)0)
#define Net_WaitForServer(...) ((void)0)

#define Net_InsertSprite(...) ((void)0)
#define Net_DeleteSprite(...) ((void)0)

#define Net_NotifyNewGame(...) ((void)0)

#define Net_WaitForInitialSnapshot(...) ((void)0)
#define Net_SendMapUpdate(...) ((void)0)
#define Net_StoreClientState(...) ((void)0)
#define Net_InitMapStateHistory(...) ((void)0)
#define Net_AddWorldToInitialSnapshot(...) ((void)0)
#define DumpMapStateHistory(...) ((void)0)




#endif

#endif // netplay_h_
