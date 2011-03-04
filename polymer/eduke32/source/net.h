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

#define NET_SECTOR_WALLPTR          0x00000001
#define NET_SECTOR_WALLNUM          0x00000002
#define NET_SECTOR_CEILINGZ         0x00000004
#define NET_SECTOR_FLOORZ           0x00000008
#define NET_SECTOR_CEILINGSTAT      0x00000010
#define NET_SECTOR_FLOORSTAT        0x00000020
#define NET_SECTOR_CEILINGPIC       0x00000040
#define NET_SECTOR_CEILINGSLOPE     0x00000080
#define NET_SECTOR_CEILINGSHADE     0x00000100
#define NET_SECTOR_CEILINGPAL       0x00000200
#define NET_SECTOR_CEILINGXPAN      0x00000400
#define NET_SECTOR_CEILINGYPAN      0x00000800
#define NET_SECTOR_FLOORPIC         0x00001000
#define NET_SECTOR_FLOORSLOPE       0x00002000
#define NET_SECTOR_FLOORSHADE       0x00004000
#define NET_SECTOR_FLOORPAL         0x00008000
#define NET_SECTOR_FLOORXPAN        0x00010000
#define NET_SECTOR_FLOORYPAN        0x00020000
#define NET_SECTOR_VISIBILITY       0x00040000
#define NET_SECTOR_LOTAG            0x00080000
#define NET_SECTOR_HITAG            0x00100000
#define NET_SECTOR_EXTRA            0x00200000

#define NET_WALL_X                  0x00000001
#define NET_WALL_Y                  0x00000002
#define NET_WALL_POINT2             0x00000004
#define NET_WALL_NEXTWALL           0x00000008
#define NET_WALL_NEXTSECTOR         0x00000010
#define NET_WALL_CSTAT              0x00000020
#define NET_WALL_PICNUM             0x00000040
#define NET_WALL_OVERPICNUM         0x00000080
#define NET_WALL_SHADE              0x00000100
#define NET_WALL_PAL                0x00000200
#define NET_WALL_XREPEAT            0x00000400
#define NET_WALL_YREPEAT            0x00000800
#define NET_WALL_XPANNING           0x00001000
#define NET_WALL_YPANNING           0x00002000
#define NET_WALL_LOTAG              0x00004000
#define NET_WALL_HITAG              0x00008000
#define NET_WALL_EXTRA              0x00010000

#define NET_SPRITE_X                0x00000001
#define NET_SPRITE_Y                0x00000002
#define NET_SPRITE_Z                0x00000004
#define NET_SPRITE_SHADE            0x00000008
#define NET_SPRITE_PAL              0x00000010
#define NET_SPRITE_CLIPDIST         0x00000020
#define NET_SPRITE_XREPEAT          0x00000040
#define NET_SPRITE_YREPEAT          0x00000080
#define NET_SPRITE_XOFFSET          0x00000100
#define NET_SPRITE_YOFFSET          0x00000200
#define NET_SPRITE_SECTNUM          0x00000400
#define NET_SPRITE_STATNUM          0x00000800
#define NET_SPRITE_ANG              0x00001000
#define NET_SPRITE_OWNER            0x00002000
#define NET_SPRITE_XVEL             0x00004000
#define NET_SPRITE_YVEL             0x00008000
#define NET_SPRITE_ZVEL             0x00010000
#define NET_SPRITE_LOTAG            0x00020000
#define NET_SPRITE_HITAG            0x00040000
#define NET_SPRITE_EXTRA            0x00080000
#define NET_SPRITE_CSTAT            0x00100000
#define NET_SPRITE_PICNUM           0x00200000

#define NET_ACTOR_T1                0x00000001
#define NET_ACTOR_T2                0x00000002
#define NET_ACTOR_T3                0x00000004
#define NET_ACTOR_T4                0x00000008
#define NET_ACTOR_T5                0x00000010
#define NET_ACTOR_T6                0x00000020
#define NET_ACTOR_T7                0x00000040
#define NET_ACTOR_T8                0x00000080
#define NET_ACTOR_T9                0x00000100
#define NET_ACTOR_T10               0x00000200
#define NET_ACTOR_PICNUM            0x00000400
#define NET_ACTOR_ANG               0x00000800
#define NET_ACTOR_EXTRA             0x00001000
#define NET_ACTOR_OWNER             0x00002000
#define NET_ACTOR_MOVFLAG           0x00004000
#define NET_ACTOR_TEMPANG           0x00008000
#define NET_ACTOR_TIMETOSLEEP       0x00010000
#define NET_ACTOR_FLAGS             0x00020000
#define NET_ACTOR_PTR1              0x00040000
#define NET_ACTOR_PTR2              0x00080000
#define NET_ACTOR_PTR3              0x00100000

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

extern ENetHost     *g_netClient;
extern ENetHost     *g_netServer;
extern ENetPeer     *g_netClientPeer;
extern char         g_netPassword[32];
extern int32_t      g_netDisconnect;
extern int32_t      g_netPlayersWaiting;
extern int32_t      g_netPort;
extern int32_t      g_networkMode;
extern int32_t      g_netSync;
extern int32_t      lastsectupdate[MAXSECTORS];
extern int32_t      lastupdate[MAXSPRITES];
extern int32_t      lastwallupdate[MAXWALLS];
extern int16_t       g_netStatnums[10];
extern mapstate_t   *g_multiMapState;

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
void    Net_SendClientInfo(void);
void    Net_SendUserMapName(void);
void    Net_StreamLevel(void);
void    Net_SyncPlayer(ENetEvent *event);
void    Net_UpdateClients(void);
void    Net_WaitForServer(void);
void    faketimerhandler(void);
#endif
