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

struct ENetHost;
struct ENetPeer;
struct ENetEvent;
struct ENetPacket;

BEGIN_DUKE_NS


// net packet specification/compatibility version
#define NETVERSION    1

extern ENetHost       *g_netClient;
extern ENetHost       *g_netServer;
extern ENetPeer       *g_netClientPeer;
extern char           g_netPassword[32];
extern int32_t        g_netDisconnect;
extern int32_t        g_netPlayersWaiting;
extern uint16_t    g_netPort;
#define g_networkMode 0
extern int32_t        g_netIndex;
extern int32_t        lastsectupdate[MAXSECTORS];
extern int32_t        lastupdate[MAXSPRITES];
extern int32_t        lastwallupdate[MAXWALLS];
extern int16_t		  g_netStatnums[];

#define NET_REVISIONS 64

enum netchan_t
{
    CHAN_REROUTE,
    CHAN_GAME,
    //CHAN_MOVE,      // unreliable movement packets
    CHAN_GAMESTATE, // gamestate changes... frags, respawns, player names, etc
    CHAN_CHAT,      // chat and RTS
    //CHAN_MISC,      // whatever else
    CHAN_MAX
};

enum ServicePacket_t
{
    SERVICEPACKET_TYPE_SENDTOID,
};

enum DukePacket_t
{
    PACKET_TYPE_MASTER_TO_SLAVE,
    PACKET_TYPE_SLAVE_TO_MASTER,
    PACKET_TYPE_BROADCAST,
    SERVER_GENERATED_BROADCAST,
    //PACKET_TYPE_VERSION,

    /* don't change anything above this line */

    //PACKET_TYPE_MESSAGE,
    //
    //PACKET_TYPE_NEW_GAME,
    //PACKET_TYPE_RTS,
    //PACKET_TYPE_MENU_LEVEL_QUIT,
    //PACKET_TYPE_WEAPON_CHOICE,
    //PACKET_TYPE_PLAYER_OPTIONS,
    //PACKET_TYPE_PLAYER_NAME,
    //PACKET_TYPE_INIT_SETTINGS,
    //
    //PACKET_TYPE_USER_MAP,
    //
    //PACKET_TYPE_MAP_VOTE,
    //PACKET_TYPE_MAP_VOTE_INITIATE,
    //PACKET_TYPE_MAP_VOTE_CANCEL,
    //
    //PACKET_TYPE_LOAD_GAME,
    PACKET_TYPE_NULL_PACKET,
    PACKET_TYPE_PLAYER_READY,
    //PACKET_TYPE_FRAGLIMIT_CHANGED,
    //PACKET_TYPE_EOL,
    //PACKET_TYPE_QUIT = 255, // should match mmulti I think


    //PACKET_MASTER_TO_SLAVE,
    //PACKET_SLAVE_TO_MASTER,

    PACKET_NUM_PLAYERS,
    PACKET_PLAYER_INDEX,
    PACKET_PLAYER_DISCONNECTED,
    //PACKET_PLAYER_SPAWN,
    //PACKET_FRAG,
    PACKET_ACK,
    PACKET_AUTH,
    //PACKET_PLAYER_PING,
    //PACKET_PLAYER_READY,
    //PACKET_MAP_STREAM,

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
    DISC_GAME_STARTED,
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
    //NET_DEDICATED_CLIENT, // client on dedicated server
    //NET_DEDICATED_SERVER
};

#define MAXSYNCBYTES 16
#define SYNCFIFOSIZ 1024

// TENSW: on really bad network connections, the sync FIFO queue can overflow if it is the
// same size as the move fifo.
#if MOVEFIFOSIZ >= SYNCFIFOSIZ
#error "MOVEFIFOSIZ is greater than or equal to SYNCFIFOSIZ!"
#endif

extern char syncstat[MAXSYNCBYTES];
extern char g_szfirstSyncMsg[MAXSYNCBYTES][60];

extern int g_numSyncBytes;
extern int g_foundSyncError;
extern int syncvaltail, syncvaltottail;

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

void    Net_ClearFIFO(void);
void    Net_GetInput(void);
void    Net_GetPackets(void);
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
#define Net_DoPrediction(...) ((void)0)
#define Net_CorrectPrediction(...) ((void)0)
#define Net_RestoreMapState(...) ((void)0)
#define Net_SyncPlayer(...) ((void)0)
#define Net_WaitForServer(...) ((void)0)

#define Net_ActorsAreDifferent(...) 0
#define Net_IsRelevantSprite(...) 0
#define Net_IsRelevantStat(...) 0
#define Net_InsertSprite(...) 0
#define Net_DeleteSprite(...) ((void)0)

#define Net_NotifyNewGame(...) ((void)0)

#define Net_SendTaunt(...) ((void)0)
#define Net_SendRTS(...) ((void)0)

#define Net_WaitForEverybody(...) ((void)0)
#define initsynccrc(...) ((void)0)
#define Net_GetSyncStat(...) ((void)0)
#define Net_DisplaySyncMsg(...) ((void)0)
#define Net_ClearFIFO(...) ((void)0)
#define Net_GetInput(...) ((void)0)

END_DUKE_NS

#endif // netplay_h_
