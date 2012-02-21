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

#include "duke3d.h"
#include "game.h"
#include "gamedef.h"
#include "net.h"
#include "premap.h"

#include "enet/enet.h"
#include "quicklz.h"
#include "crc32.h"

/*
this should be lower than the MTU size by at least the size of the UDP and ENet headers
or else fragmentation will occur
*/
#define SYNCPACKETSIZE 1344

ENetHost *g_netServer = NULL;
ENetHost *g_netClient = NULL;
ENetPeer *g_netClientPeer = NULL;
int32_t g_netPort = 23513;
int32_t g_netDisconnect = 0;
char g_netPassword[32];
int32_t g_netSync = 0;
int32_t g_netPlayersWaiting = 0;
int32_t g_networkMode = NET_CLIENT;

static char recbuf[180];
static int32_t g_chatPlayer = -1;

// sprites of these statnums are synced to clients by the server
int16_t g_netStatnums[10] = { STAT_PROJECTILE, STAT_PLAYER, STAT_STANDABLE, STAT_ACTIVATOR, STAT_TRANSPORT,
                             STAT_EFFECTOR, STAT_ACTOR, STAT_ZOMBIEACTOR, STAT_MISC, MAXSTATUS
                           };

int32_t lastupdate[MAXSPRITES];
int32_t lastsectupdate[MAXSECTORS];
int32_t lastwallupdate[MAXWALLS];
mapstate_t *g_multiMapState = NULL;
static int32_t spritecrc[MAXSPRITES];
static int32_t sectcrc[MAXSECTORS];
static int32_t wallcrc[MAXWALLS];

void Net_Connect(const char *srvaddr)
{
    ENetAddress address;
    ENetEvent event;
    char *addrstr = NULL;
    int32_t i;

    Net_Disconnect();

    g_netClient = enet_host_create(NULL, 1, CHAN_MAX, 0, 0);

    if (g_netClient == NULL)
    {
        initprintf("An error occurred while trying to create an ENet client host.\n");
        return;
    }

    addrstr = strtok((char *)srvaddr, ":");
    enet_address_set_host(&address, addrstr);
    address.port = Batoi((addrstr = strtok(NULL, ":")) == NULL ? "23513" : addrstr);

    g_netClientPeer = enet_host_connect(g_netClient, &address, CHAN_MAX, 0);

    if (g_netClientPeer == NULL)
    {
        initprintf("No available peers for initiating an ENet connection.\n");
        return;
    }

    for (i=4; i>0; i--)
    {
        /* Wait up to 5 seconds for the connection attempt to succeed. */
        if (enet_host_service(g_netClient, & event, 5000) > 0 &&
                event.type == ENET_EVENT_TYPE_CONNECT)
        {
            initprintf("Connection to %s:%d succeeded.\n", (char *)srvaddr, address.port);
            return;
        }
        else
        {
            /* Either the 5 seconds are up or a disconnect event was */
            /* received. Reset the peer in the event the 5 seconds   */
            /* had run out without any significant event.            */
            enet_peer_reset(g_netClientPeer);
            initprintf("Connection to %s:%d failed.\n",(char *)srvaddr,address.port);
        }
        initprintf(i ? "Retrying...\n" : "Giving up connection attempt.\n");
    }

    Net_Disconnect();
}

void Net_Disconnect(void)
{
    if (g_netClient)
    {
        ENetEvent event;

        if (g_netClientPeer)
            enet_peer_disconnect_later(g_netClientPeer, 0);

        while (enet_host_service(g_netClient, & event, 3000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
            case ENET_EVENT_TYPE_NONE:
            case ENET_EVENT_TYPE_RECEIVE:
                if (event.packet)
                    enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                numplayers = playerswhenstarted = ud.multimode = 1;
                myconnectindex = screenpeek = 0;
                G_BackToMenu();
                break;
            }
        }

        enet_peer_reset(g_netClientPeer);
        g_netClientPeer = NULL;
        enet_host_destroy(g_netClient);
        g_netClient = NULL;
        return;
    }

    if (g_netServer)
    {
        ENetPeer *currentPeer;
        ENetEvent event;

        for (currentPeer = g_netServer -> peers;
                currentPeer < & g_netServer -> peers [g_netServer -> peerCount];
                ++ currentPeer)
        {
            enet_peer_disconnect_later(currentPeer, 0);
        }

        while (enet_host_service(g_netServer, & event, 3000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
            case ENET_EVENT_TYPE_NONE:
            case ENET_EVENT_TYPE_RECEIVE:
            case ENET_EVENT_TYPE_DISCONNECT:
                if (event.packet)
                    enet_packet_destroy(event.packet);
                break;
            }
        }
        enet_host_destroy(g_netServer);
        g_netServer = NULL;
    }
}

static void Net_SendVersion(ENetPeer *client)
{
    if (!g_netServer) return;

    buf[0] = PACKET_VERSION;
    buf[1] = BYTEVERSION>>16;
    buf[2] = BYTEVERSION&255;
    buf[3] = (uint8_t)atoi(s_buildDate);
    buf[4] = myconnectindex;

    enet_peer_send(client, CHAN_GAMESTATE, enet_packet_create(&buf[0], 5, ENET_PACKET_FLAG_RELIABLE));
}

void Net_SendClientInfo(void)
{
    int32_t i,l;

    for (l=0; (unsigned)l<sizeof(szPlayerName)-1; l++)
        g_player[myconnectindex].user_name[l] = Btoupper(szPlayerName[l]);

    if (numplayers < 2) return;

    buf[0] = PACKET_CLIENT_INFO;
    l = 1;

    //null terminated player name to send
    for (i=0; szPlayerName[i]; i++) buf[l++] = Btoupper(szPlayerName[i]);
    buf[l++] = 0;

    buf[l++] = g_player[myconnectindex].ps->aim_mode = ud.mouseaiming;
    buf[l++] = g_player[myconnectindex].ps->auto_aim = ud.config.AutoAim;
    buf[l++] = g_player[myconnectindex].ps->weaponswitch = ud.weaponswitch;
    buf[l++] = g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = ud.color;

    buf[l++] = g_player[myconnectindex].pteam = ud.team;

    for (i=0; i<10; i++)
    {
        g_player[myconnectindex].wchoice[i] = g_player[0].wchoice[i];
        buf[l++] = (uint8_t)g_player[0].wchoice[i];
    }

    buf[l++] = myconnectindex;

    if (g_netClient)
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&buf[0], l, ENET_PACKET_FLAG_RELIABLE));
    else if (g_netServer)
        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(&buf[0], l, ENET_PACKET_FLAG_RELIABLE));
}

void Net_SendUserMapName(void)
{
    int32_t j;

    if (numplayers < 2)
        return;

    packbuf[0] = PACKET_USER_MAP;
    packbuf[1] = 0;

    Bcorrectfilename(boardfilename,0);

    j = Bstrlen(boardfilename);
    boardfilename[j++] = 0;
    Bstrcat(packbuf+1,boardfilename);

    packbuf[j++] = myconnectindex;

    if (g_netClient)
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(packbuf, j, ENET_PACKET_FLAG_RELIABLE));
    else if (g_netServer)
        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, j, ENET_PACKET_FLAG_RELIABLE));
}

// FIXME: change all of the game starting support code to be fully server controlled
void Net_NewGame(int32_t volume, int32_t level)
{
    packbuf[0] = PACKET_NEW_GAME;
    packbuf[1] = ud.m_level_number = level;
    packbuf[2] = ud.m_volume_number = volume;
    packbuf[3] = ud.m_player_skill+1;
    packbuf[4] = ud.m_monsters_off;
    packbuf[5] = ud.m_respawn_monsters;
    packbuf[6] = ud.m_respawn_items;
    packbuf[7] = ud.m_respawn_inventory;
    packbuf[8] = ud.m_coop;
    packbuf[9] = ud.m_marker;
    packbuf[10] = ud.m_ffire;
    packbuf[11] = ud.m_noexits;
    packbuf[12] = myconnectindex;

    if (g_netClient)
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(packbuf, 13, ENET_PACKET_FLAG_RELIABLE));
    else if (g_netServer)
        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, 13, ENET_PACKET_FLAG_RELIABLE));
}

// sends a simple crc32 of the current password, verified by the server before the connection can continue
static void Net_SendChallenge(void)
{
    int32_t l = 1;

    if (!g_netClientPeer) return;

    buf[0] = PACKET_AUTH;
    *(uint32_t *)&buf[1] = crc32once((uint8_t *)g_netPassword, Bstrlen(g_netPassword));
    l += sizeof(int32_t);

    buf[l++] = myconnectindex;

    enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&buf[0], l, ENET_PACKET_FLAG_RELIABLE));
}

static void P_RemovePlayer(int32_t i)
{
    // server obviously can't leave the game, and index 0 shows up for disconnect events from
    // players that haven't gotten far enough into the connection process to get a player ID

    if (i == 0) return;

    g_player[i].playerquitflag = 0;

    Bsprintf(buf,"%s^00 is history!",g_player[i].user_name);
    G_AddUserQuote(buf);

    if (numplayers == 1)
        S_PlaySound(GENERIC_AMBIENCE17);

    if (g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        if (screenpeek == i)
            screenpeek = myconnectindex;

        pub = NUMPAGES;
        pus = NUMPAGES;
        G_UpdateScreenArea();

        P_QuickKill(g_player[i].ps);

        if (voting == i)
        {
            for (i=0; i<MAXPLAYERS; i++)
            {
                g_player[i].vote = 0;
                g_player[i].gotvote = 0;
            }
            voting = -1;
        }

        Bstrcpy(ScriptQuotes[QUOTE_RESERVED2],buf);
        g_player[myconnectindex].ps->ftq = QUOTE_RESERVED2;
        g_player[myconnectindex].ps->fta = 180;
    }
}

// sync a connecting player up with the current game state
void Net_SyncPlayer(ENetEvent *event)
{
    int32_t i, j;

    g_netPlayersWaiting++;

    S_PlaySound(DUKE_GETWEAPON2);

    // open a new slot if necessary and save off the resulting slot # for future reference
    for (TRAVERSE_CONNECT(i)) if (g_player[i].playerquitflag == 0) break;
    event->peer->data = (void *)((intptr_t)(i = (i == -1 ? playerswhenstarted++ : i)));

    g_player[i].netsynctime = totalclock;
    g_player[i].playerquitflag = 1;

    for (j=0; j<playerswhenstarted-1; j++) connectpoint2[j] = j+1;
    connectpoint2[playerswhenstarted-1] = -1;

//    for (TRAVERSE_CONNECT(j))
//    {
        if (!g_player[i].ps) g_player[i].ps = (DukePlayer_t *) Bcalloc(1, sizeof(DukePlayer_t));
        if (!g_player[i].sync) g_player[i].sync = (input_t *) Bcalloc(1, sizeof(input_t));
//    }

    packbuf[0] = PACKET_NUM_PLAYERS;
    packbuf[1] = ++numplayers;
    packbuf[2] = playerswhenstarted;
    packbuf[3] = ++ud.multimode;
    packbuf[4] = i;
    packbuf[5] = g_networkMode;
    packbuf[6] = myconnectindex;
    enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, 7, ENET_PACKET_FLAG_RELIABLE));

    packbuf[0] = PACKET_PLAYER_INDEX;
    packbuf[1] = i;
    packbuf[2] = myconnectindex;
    enet_peer_send(event->peer, CHAN_GAMESTATE, enet_packet_create(packbuf, 3, ENET_PACKET_FLAG_RELIABLE));

    Net_SendClientInfo();
    Net_SendUserMapName();

    if (g_player[0].ps->gm & MODE_GAME)
    {
        if (g_multiMapState == NULL) g_multiMapState = (mapstate_t *)Bcalloc(1, sizeof(mapstate_t));
        if (g_multiMapState)
        {
            char *buf = (char *)Bmalloc(sizeof(mapstate_t)<<1);

            sprite[g_player[i].ps->i].cstat = 32768;
            g_player[i].ps->runspeed = g_playerFriction;
            g_player[i].ps->last_extra = sprite[g_player[i].ps->i].extra = g_player[i].ps->max_player_health = g_maxPlayerHealth;

            G_SaveMapState(g_multiMapState);
            if ((j = qlz_compress((char *)g_multiMapState, buf, sizeof(mapstate_t), state_compress)))
            {
                size_t csize = qlz_size_compressed(buf);

                // all of these packets are SYNCPACKETSIZE
                do
                {
                    enet_peer_send(event->peer, CHAN_SYNC,
                                   enet_packet_create((char *)(buf)+csize-j, SYNCPACKETSIZE, ENET_PACKET_FLAG_RELIABLE));
                    j -= SYNCPACKETSIZE;
                    enet_host_service(g_netServer, NULL, 0);
                }
                while (j >= SYNCPACKETSIZE);

                // ...except for this one.  A non-SYNCPACKETSIZE packet on CHAN_SYNC doubles as the signal that the transfer is done.
                enet_peer_send(event->peer, CHAN_SYNC,
                               enet_packet_create((char *)(buf)+csize-j, j, ENET_PACKET_FLAG_RELIABLE));
                enet_host_service(g_netServer, NULL, 0);
            }
            else
                initprintf("Error compressing map state for transfer!\n");

            Bfree(buf);
            Bfree(g_multiMapState);
            g_multiMapState = NULL;
        }
    }
}

int32_t Net_UnpackSprite(int32_t i, uint8_t *pbuf)
{
    int16_t sect = sprite[i].sectnum, statnum = sprite[i].statnum;
    int16_t opicnum, j = 0;
#ifdef POLYMER
    int16_t lightid = -1;
    _prlight *mylight = NULL;
#endif

    uint32_t flags = *(uint32_t *)&pbuf[j];
    j += sizeof(uint32_t);

    opicnum = sprite[i].picnum;

    if (flags & NET_SPRITE_X)
    {
        sprite[i].x = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }

    if (flags & NET_SPRITE_Y)
    {
        sprite[i].y = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }

    if (flags & NET_SPRITE_Z)
    {
        sprite[i].z = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }

    if (flags & NET_SPRITE_SHADE)
    {
        sprite[i].shade = *(int8_t *)&pbuf[j];
        j += sizeof(int8_t);
    }

    if (flags & NET_SPRITE_PAL)
    {
        sprite[i].pal = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_SPRITE_CLIPDIST)
    {
        sprite[i].clipdist = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_SPRITE_XREPEAT)
    {
        sprite[i].xrepeat = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_SPRITE_YREPEAT)
    {
        sprite[i].yrepeat = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_SPRITE_XOFFSET)
    {
        sprite[i].xoffset = *(int8_t *)&pbuf[j];
        j += sizeof(int8_t);
    }

    if (flags & NET_SPRITE_YOFFSET)
    {
        sprite[i].yoffset = *(int8_t *)&pbuf[j];
        j += sizeof(int8_t);
    }

    if (flags & NET_SPRITE_SECTNUM)
    {
        sect = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SPRITE_STATNUM)
    {
        statnum = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SPRITE_ANG)
    {
        sprite[i].ang = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SPRITE_OWNER)
    {
        sprite[i].owner = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SPRITE_XVEL)
    {
        sprite[i].xvel = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SPRITE_YVEL)
    {
        sprite[i].yvel = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SPRITE_ZVEL)
    {
        sprite[i].zvel = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SPRITE_LOTAG)
    {
        sprite[i].lotag = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SPRITE_HITAG)
    {
        sprite[i].hitag = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SPRITE_EXTRA)
    {
        sprite[i].extra = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SPRITE_CSTAT)
    {
        sprite[i].cstat = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SPRITE_PICNUM)
    {
        sprite[i].picnum = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (sect == MAXSECTORS || statnum == MAXSTATUS)
    {
//        j += sizeof(netactor_t);
        if (sprite[i].sectnum != MAXSECTORS && sprite[i].statnum != MAXSTATUS)
            deletesprite(i);
        return j;
    }

    // doesn't exist on the client yet
    if (sprite[i].statnum == MAXSTATUS || sprite[i].sectnum == MAXSECTORS)
    {
        int16_t sprs[MAXSPRITES], z = 0;
        while ((sprs[z++] = insertsprite(sect, statnum)) != i);
        z--;
        while (z--) deletesprite(sprs[z]);
    }
    else
    {
        if (sect != sprite[i].sectnum) changespritesect(i, sect);
        if (statnum != sprite[i].statnum) changespritestat(i, statnum);
    }
#ifdef POLYMER
    if (sprite[i].picnum == opicnum)
    {
        mylight = actor[i].lightptr;
        lightid = actor[i].lightId;
    }
    else if (getrendermode() == 4 && actor[i].lightptr != NULL)
    {
        polymer_deletelight(actor[i].lightId);
        actor[i].lightId = -1;
        actor[i].lightptr = NULL;
    }
#endif

    /*initprintf("updating sprite %d (%d)\n",i,sprite[i].picnum);*/

    flags = *(uint32_t *)&pbuf[j];
    j += sizeof(uint32_t);

    if (flags & NET_ACTOR_T1)
    {
        actor[i].t_data[0] = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }

    if (flags & NET_ACTOR_T2)
    {
        actor[i].t_data[1] = *(int32_t *)&pbuf[j];
#if !defined SAMESIZE_ACTOR_T
        if (flags & NET_ACTOR_PTR1)
            actor[i].t_data[1] += (intptr_t)&script[0];
#endif
        j += sizeof(int32_t);
    }

    if (flags & NET_ACTOR_T3)
    {
        actor[i].t_data[2] = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }

    if (flags & NET_ACTOR_T4)
    {
        actor[i].t_data[3] = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }

    if (flags & NET_ACTOR_T5)
    {
        actor[i].t_data[4] = *(int32_t *)&pbuf[j];
#if !defined SAMESIZE_ACTOR_T
        if (flags & NET_ACTOR_PTR2)
            actor[i].t_data[4] += (intptr_t)&script[0];
#endif
        j += sizeof(int32_t);
    }

    if (flags & NET_ACTOR_T6)
    {
        actor[i].t_data[5] = *(int32_t *)&pbuf[j];
#if !defined SAMESIZE_ACTOR_T
        if (flags & NET_ACTOR_PTR3)
            actor[i].t_data[5] += (intptr_t)&script[0];
#endif
        j += sizeof(int32_t);
    }

    if (flags & NET_ACTOR_T7)
    {
        actor[i].t_data[6] = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }

    if (flags & NET_ACTOR_T8)
    {
        actor[i].t_data[7] = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }

    if (flags & NET_ACTOR_T9)
    {
        actor[i].t_data[8] = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }

    if (flags & NET_ACTOR_T10)
    {
        actor[i].t_data[9] = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }

    if (flags & NET_ACTOR_PICNUM)
    {
        actor[i].picnum = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_ACTOR_ANG)
    {
        actor[i].ang = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_ACTOR_EXTRA)
    {
        actor[i].extra = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_ACTOR_OWNER)
    {
        actor[i].owner = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_ACTOR_MOVFLAG)
    {
        actor[i].movflag = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_ACTOR_TEMPANG)
    {
        actor[i].tempang = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_ACTOR_TIMETOSLEEP)
    {
        actor[i].timetosleep = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_ACTOR_FLAGS)
    {
        actor[i].flags = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }

#ifdef POLYMER
    actor[i].lightptr = mylight;
    actor[i].lightId = lightid;
#endif

    actor[i].flags &= ~SPRITE_NULL;

    do
    {
        int16_t var_id = *(int16_t *)&pbuf[j];

        j += sizeof(int16_t);

        if (var_id == MAXGAMEVARS) break;

        if (aGameVars[var_id].val.plValues)
            aGameVars[var_id].val.plValues[i] = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }
    while (1);

    return j;
}

int32_t Net_PackSprite(int32_t i, uint8_t *pbuf)
{
    int32_t j = 0;
    uint32_t *flags;
    static spritetype netsprite[MAXSPRITES];
    static netactor_t netactor[MAXSPRITES];

    if (lastupdate[i] && !Bmemcmp(&sprite[i], &netsprite[i], sizeof(spritetype)))
        return 0;

    *(int16_t *)&pbuf[j] = i;
    j += sizeof(int16_t);

    flags = (uint32_t *)&pbuf[j];
    *flags = 0;
    j += sizeof(uint32_t);

    if (sprite[i].sectnum == MAXSECTORS || sprite[i].statnum == MAXSTATUS)
    {
        *flags = NET_SPRITE_SECTNUM;
        *(int16_t *)&pbuf[j] = MAXSECTORS;
        j += sizeof(int16_t);

        Bmemcpy(&netsprite[i], &sprite[i], sizeof(spritetype));
        return j;
    }

    if (!lastupdate[i] || sprite[i].x != netsprite[i].x)
    {
        *flags |= NET_SPRITE_X;
        *(int32_t *)&pbuf[j] = sprite[i].x;
        j += sizeof(int32_t);
    }

    if (!lastupdate[i] || sprite[i].y != netsprite[i].y)
    {
        *flags |= NET_SPRITE_Y;
        *(int32_t *)&pbuf[j] = sprite[i].y;
        j += sizeof(int32_t);
    }

    if (!lastupdate[i] || sprite[i].z != netsprite[i].z)
    {
        *flags |= NET_SPRITE_Z;
        *(int32_t *)&pbuf[j] = sprite[i].z;
        j += sizeof(int32_t);
    }

    if (!lastupdate[i] || sprite[i].shade != netsprite[i].shade)
    {
        *flags |= NET_SPRITE_SHADE;
        *(int8_t *)&pbuf[j] = sprite[i].shade;
        j += sizeof(int8_t);
    }

    if (!lastupdate[i] || sprite[i].pal != netsprite[i].pal)
    {
        *flags |= NET_SPRITE_PAL;
        *(uint8_t *)&pbuf[j] = sprite[i].pal;
        j += sizeof(uint8_t);
    }

    if (!lastupdate[i] || sprite[i].clipdist != netsprite[i].clipdist)
    {
        *flags |= NET_SPRITE_CLIPDIST;
        *(uint8_t *)&pbuf[j] = sprite[i].clipdist;
        j += sizeof(uint8_t);
    }

    if (!lastupdate[i] || sprite[i].xrepeat != netsprite[i].xrepeat)
    {
        *flags |= NET_SPRITE_XREPEAT;
        *(uint8_t *)&pbuf[j] = sprite[i].xrepeat;
        j += sizeof(uint8_t);
    }

    if (!lastupdate[i] || sprite[i].yrepeat != netsprite[i].yrepeat)
    {
        *flags |= NET_SPRITE_YREPEAT;
        *(uint8_t *)&pbuf[j] = sprite[i].yrepeat;
        j += sizeof(uint8_t);
    }

    if (!lastupdate[i] || sprite[i].xoffset != netsprite[i].xoffset)
    {
        *flags |= NET_SPRITE_XOFFSET;
        *(int8_t *)&pbuf[j] = sprite[i].xoffset;
        j += sizeof(int8_t);
    }

    if (!lastupdate[i] || sprite[i].yoffset != netsprite[i].yoffset)
    {
        *flags |= NET_SPRITE_YOFFSET;
        *(int8_t *)&pbuf[j] = sprite[i].yoffset;
        j += sizeof(int8_t);
    }

    if (!lastupdate[i] || sprite[i].sectnum != netsprite[i].sectnum || sprite[i].sectnum == MAXSECTORS)
    {
        *flags |= NET_SPRITE_SECTNUM;
        *(int16_t *)&pbuf[j] = sprite[i].sectnum;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || sprite[i].statnum != netsprite[i].statnum || sprite[i].statnum == MAXSTATUS)
    {
        *flags |= NET_SPRITE_STATNUM;
        *(int16_t *)&pbuf[j] = sprite[i].statnum;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || sprite[i].ang != netsprite[i].ang)
    {
        *flags |= NET_SPRITE_ANG;
        *(int16_t *)&pbuf[j] = sprite[i].ang;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || sprite[i].owner != netsprite[i].owner)
    {
        *flags |= NET_SPRITE_OWNER;
        *(int16_t *)&pbuf[j] = sprite[i].owner;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || sprite[i].xvel != netsprite[i].xvel)
    {
        *flags |= NET_SPRITE_XVEL;
        *(int16_t *)&pbuf[j] = sprite[i].xvel;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || sprite[i].yvel != netsprite[i].yvel)
    {
        *flags |= NET_SPRITE_YVEL;
        *(int16_t *)&pbuf[j] = sprite[i].yvel;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || sprite[i].zvel != netsprite[i].zvel)
    {
        *flags |= NET_SPRITE_ZVEL;
        *(int16_t *)&pbuf[j] = sprite[i].zvel;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || sprite[i].lotag != netsprite[i].lotag)
    {
        *flags |= NET_SPRITE_LOTAG;
        *(int16_t *)&pbuf[j] = sprite[i].lotag;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || sprite[i].hitag != netsprite[i].hitag)
    {
        *flags |= NET_SPRITE_HITAG;
        *(int16_t *)&pbuf[j] = sprite[i].hitag;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || sprite[i].extra != netsprite[i].extra)
    {
        *flags |= NET_SPRITE_EXTRA;
        *(int16_t *)&pbuf[j] = sprite[i].extra;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || sprite[i].cstat != netsprite[i].cstat)
    {
        *flags |= NET_SPRITE_CSTAT;
        *(int16_t *)&pbuf[j] = sprite[i].cstat;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || sprite[i].picnum != netsprite[i].picnum)
    {
        *flags |= NET_SPRITE_PICNUM;
        *(int16_t *)&pbuf[j] = sprite[i].picnum;
        j += sizeof(int16_t);
    }

    if (lastupdate[i])
        Bmemcpy(&netsprite[i], &sprite[i], sizeof(spritetype));

    flags = (uint32_t *)&pbuf[j];
    *flags = 0;
    j += sizeof(uint32_t);

    if (!lastupdate[i] || actor[i].t_data[0] != netactor[i].t_data[0])
    {
        *flags |= NET_ACTOR_T1;
        *(int32_t *)&pbuf[j] = actor[i].t_data[0];
        j += sizeof(int32_t);
    }

    if (!lastupdate[i] || actor[i].t_data[1] != netactor[i].t_data[1])
    {
        *flags |= NET_ACTOR_T2;
#if !defined SAMESIZE_ACTOR_T
        if (T2 >= (intptr_t)&script[0] && T2 < (intptr_t)(&script[g_scriptSize]))
        {
            *flags |= NET_ACTOR_PTR1;
            *(int32_t *)&pbuf[j] = (int32_t)(actor[i].t_data[1] - (intptr_t)&script[0]);
        }
        else
#endif
            *(int32_t *)&pbuf[j] = actor[i].t_data[1];
        j += sizeof(int32_t);
    }

    if (!lastupdate[i] || actor[i].t_data[2] != netactor[i].t_data[2])
    {
        *flags |= NET_ACTOR_T3;
        *(int32_t *)&pbuf[j] = actor[i].t_data[2];
        j += sizeof(int32_t);
    }

    if (!lastupdate[i] || actor[i].t_data[3] != netactor[i].t_data[3])
    {
        *flags |= NET_ACTOR_T4;
        *(int32_t *)&pbuf[j] = actor[i].t_data[3];
        j += sizeof(int32_t);
    }

    if (!lastupdate[i] || actor[i].t_data[4] != netactor[i].t_data[4])
    {
        *flags |= NET_ACTOR_T5;
#if !defined SAMESIZE_ACTOR_T
        if (T5 >= (intptr_t)&script[0] && T5 < (intptr_t)(&script[g_scriptSize]))
        {
            *flags |= NET_ACTOR_PTR2;
            *(int32_t *)&pbuf[j] = (int32_t)(actor[i].t_data[4] - (intptr_t)&script[0]);
        }
        else
#endif
            *(int32_t *)&pbuf[j] = actor[i].t_data[4];
        j += sizeof(int32_t);
    }

    if (!lastupdate[i] || actor[i].t_data[5] != netactor[i].t_data[5])
    {
        *flags |= NET_ACTOR_T6;
#if !defined SAMESIZE_ACTOR_T
        if (T6 >= (intptr_t)&script[0] && T6 < (intptr_t)(&script[g_scriptSize]))
        {
            *flags |= NET_ACTOR_PTR3;
            *(int32_t *)&pbuf[j] = (int32_t)(actor[i].t_data[5] - (intptr_t)&script[0]);
        }
        else
#endif
            *(int32_t *)&pbuf[j] = actor[i].t_data[5];
        j += sizeof(int32_t);
    }

    if (!lastupdate[i] || actor[i].t_data[6] != netactor[i].t_data[6])
    {
        *flags |= NET_ACTOR_T7;
        *(int32_t *)&pbuf[j] = actor[i].t_data[6];
        j += sizeof(int32_t);
    }

    if (!lastupdate[i] || actor[i].t_data[7] != netactor[i].t_data[7])
    {
        *flags |= NET_ACTOR_T8;
        *(int32_t *)&pbuf[j] = actor[i].t_data[7];
        j += sizeof(int32_t);
    }

    if (!lastupdate[i] || actor[i].t_data[8] != netactor[i].t_data[8])
    {
        *flags |= NET_ACTOR_T9;
        *(int32_t *)&pbuf[j] = actor[i].t_data[8];
        j += sizeof(int32_t);
    }

    if (!lastupdate[i] || actor[i].t_data[9] != netactor[i].t_data[9])
    {
        *flags |= NET_ACTOR_T10;
        *(int32_t *)&pbuf[j] = actor[i].t_data[9];
        j += sizeof(int32_t);
    }

    if (!lastupdate[i] || actor[i].picnum != netactor[i].picnum)
    {
        *flags |= NET_ACTOR_PICNUM;
        *(int16_t *)&pbuf[j] = actor[i].picnum;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || actor[i].ang != netactor[i].ang)
    {
        *flags |= NET_ACTOR_ANG;
        *(int16_t *)&pbuf[j] = actor[i].ang;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || actor[i].extra != netactor[i].extra)
    {
        *flags |= NET_ACTOR_EXTRA;
        *(int16_t *)&pbuf[j] = actor[i].extra;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || actor[i].owner!= netactor[i].owner)
    {
        *flags |= NET_ACTOR_OWNER;
        *(int16_t *)&pbuf[j] = actor[i].owner;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || actor[i].movflag != netactor[i].movflag)
    {
        *flags |= NET_ACTOR_MOVFLAG;
        *(int16_t *)&pbuf[j] = actor[i].movflag;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || actor[i].tempang != netactor[i].tempang)
    {
        *flags |= NET_ACTOR_TEMPANG;
        *(int16_t *)&pbuf[j] = actor[i].tempang;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || actor[i].timetosleep != netactor[i].timetosleep)
    {
        *flags |= NET_ACTOR_TIMETOSLEEP;
        *(int16_t *)&pbuf[j] = actor[i].timetosleep;
        j += sizeof(int16_t);
    }

    if (!lastupdate[i] || actor[i].flags != netactor[i].flags)
    {
        *flags |= NET_ACTOR_FLAGS;
        *(int32_t *)&pbuf[j] = actor[i].flags;
        j += sizeof(int32_t);
    }

    if (lastupdate[i])
        Bmemcpy(&netactor[i], &actor[i], sizeof(netactor_t));

    if (*flags == 0)
        return 0;

    {
        int16_t ii=g_gameVarCount-1;

        for (; ii>=0; ii--)
        {
            if ((aGameVars[ii].dwFlags & (GAMEVAR_PERACTOR|GAMEVAR_NOMULTI)) == GAMEVAR_PERACTOR && aGameVars[ii].val.plValues)
            {
                if (aGameVars[ii].val.plValues[i] != aGameVars[ii].lDefault)
                {
                    *(int16_t *)&pbuf[j] = ii;
                    j += sizeof(int16_t);
                    *(int32_t *)&pbuf[j] = aGameVars[ii].val.plValues[i];
                    j += sizeof(int32_t);
                }
            }
        }

        *(int16_t *)&pbuf[j] = MAXGAMEVARS;
        j += sizeof(int16_t);
    }

    return j;
}

int32_t Net_UnpackSect(int32_t i, uint8_t *pbuf)
{
    int32_t j = 0;
    uint32_t flags = *(uint32_t *)&pbuf[j];

    j += sizeof(uint32_t);

    if (flags & NET_SECTOR_WALLPTR)
    {
        sector[i].wallptr = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SECTOR_WALLNUM)
    {
        sector[i].wallnum = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SECTOR_CEILINGZ)
    {
        sector[i].ceilingz = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }

    if (flags & NET_SECTOR_FLOORZ)
    {
        sector[i].floorz = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }

    if (flags & NET_SECTOR_CEILINGSTAT)
    {
        sector[i].ceilingstat = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SECTOR_FLOORSTAT)
    {
        sector[i].floorstat = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SECTOR_CEILINGPIC)
    {
        sector[i].ceilingpicnum = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SECTOR_CEILINGSLOPE)
    {
        sector[i].ceilingheinum = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SECTOR_CEILINGSHADE)
    {
        sector[i].ceilingshade = *(int8_t *)&pbuf[j];
        j += sizeof(int8_t);
    }

    if (flags & NET_SECTOR_CEILINGPAL)
    {
        sector[i].ceilingpal = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_SECTOR_CEILINGXPAN)
    {
        sector[i].ceilingxpanning = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_SECTOR_CEILINGYPAN)
    {
        sector[i].ceilingypanning = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_SECTOR_FLOORPIC)
    {
        sector[i].floorpicnum = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SECTOR_FLOORSLOPE)
    {
        sector[i].floorheinum = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SECTOR_FLOORSHADE)
    {
        sector[i].floorshade = *(int8_t *)&pbuf[j];
        j += sizeof(int8_t);
    }

    if (flags & NET_SECTOR_FLOORPAL)
    {
        sector[i].floorpal = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_SECTOR_FLOORXPAN)
    {
        sector[i].floorxpanning = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_SECTOR_FLOORYPAN)
    {
        sector[i].floorypanning = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_SECTOR_VISIBILITY)
    {
        sector[i].visibility = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_SECTOR_LOTAG)
    {
        sector[i].lotag = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SECTOR_HITAG)
    {
        sector[i].hitag = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_SECTOR_EXTRA)
    {
        sector[i].extra = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    return j;
}

int32_t Net_PackSect(int32_t i, uint8_t *pbuf)
{
    int32_t j = 0;
    uint32_t *flags;
    static sectortype netsect[MAXSECTORS];

    if (lastsectupdate[i] && !Bmemcmp(&sector[i], &netsect[i], sizeof(sectortype)))
        return 0;

    *(int16_t *)&pbuf[j] = i;
    j += sizeof(int16_t);

    flags = (uint32_t *)&pbuf[j];
    *flags = 0;
    j += sizeof(uint32_t);

    if (!lastsectupdate[i] || sector[i].wallptr != netsect[i].wallptr)
    {
        *flags |= NET_SECTOR_WALLPTR;
        *(int16_t *)&pbuf[j] = sector[i].wallptr;
        j += sizeof(int16_t);
    }

    if (!lastsectupdate[i] || sector[i].wallnum != netsect[i].wallnum)
    {
        *flags |= NET_SECTOR_WALLNUM;
        *(int16_t *)&pbuf[j] = sector[i].wallnum;
        j += sizeof(int16_t);
    }

    if (!lastsectupdate[i] || sector[i].ceilingz != netsect[i].ceilingz)
    {
        *flags |= NET_SECTOR_CEILINGZ;
        *(int32_t *)&pbuf[j] = sector[i].ceilingz;
        j += sizeof(int32_t);
    }

    if (!lastsectupdate[i] || sector[i].floorz != netsect[i].floorz)
    {
        *flags |= NET_SECTOR_FLOORZ;
        *(int32_t *)&pbuf[j] = sector[i].floorz;
        j += sizeof(int32_t);
    }

    if (!lastsectupdate[i] || sector[i].ceilingstat != netsect[i].ceilingstat)
    {
        *flags |= NET_SECTOR_CEILINGSTAT;
        *(int16_t *)&pbuf[j] = sector[i].ceilingstat;
        j += sizeof(int16_t);
    }

    if (!lastsectupdate[i] || sector[i].floorstat != netsect[i].floorstat)
    {
        *flags |= NET_SECTOR_FLOORSTAT;
        *(int16_t *)&pbuf[j] = sector[i].floorstat;
        j += sizeof(int16_t);
    }

    if (!lastsectupdate[i] || sector[i].ceilingpicnum != netsect[i].ceilingpicnum)
    {
        *flags |= NET_SECTOR_CEILINGPIC;
        *(int16_t *)&pbuf[j] = sector[i].ceilingpicnum;
        j += sizeof(int16_t);
    }

    if (!lastsectupdate[i] || sector[i].ceilingheinum != netsect[i].ceilingheinum)
    {
        *flags |= NET_SECTOR_CEILINGSLOPE;
        *(int16_t *)&pbuf[j] = sector[i].ceilingheinum;
        j += sizeof(int16_t);
    }

    if (!lastsectupdate[i] || sector[i].ceilingshade != netsect[i].ceilingshade)
    {
        *flags |= NET_SECTOR_CEILINGSHADE;
        *(int8_t *)&pbuf[j] = sector[i].ceilingshade;
        j += sizeof(int8_t);
    }

    if (!lastsectupdate[i] || sector[i].ceilingpal != netsect[i].ceilingpal)
    {
        *flags |= NET_SECTOR_CEILINGPAL;
        *(uint8_t *)&pbuf[j] = sector[i].ceilingpal;
        j += sizeof(uint8_t);
    }

    if (!lastsectupdate[i] || sector[i].ceilingxpanning != netsect[i].ceilingxpanning)
    {
        *flags |= NET_SECTOR_CEILINGXPAN;
        *(uint8_t *)&pbuf[j] = sector[i].ceilingxpanning;
        j += sizeof(uint8_t);
    }

    if (!lastsectupdate[i] || sector[i].ceilingypanning != netsect[i].ceilingypanning)
    {
        *flags |= NET_SECTOR_CEILINGYPAN;
        *(uint8_t *)&pbuf[j] = sector[i].ceilingypanning;
        j += sizeof(uint8_t);
    }

    if (!lastsectupdate[i] || sector[i].floorpicnum != netsect[i].floorpicnum)
    {
        *flags |= NET_SECTOR_FLOORPIC;
        *(int16_t *)&pbuf[j] = sector[i].floorpicnum;
        j += sizeof(int16_t);
    }

    if (!lastsectupdate[i] || sector[i].floorheinum != netsect[i].floorheinum)
    {
        *flags |= NET_SECTOR_FLOORSLOPE;
        *(int16_t *)&pbuf[j] = sector[i].floorheinum;
        j += sizeof(int16_t);
    }

    if (!lastsectupdate[i] || sector[i].floorshade != netsect[i].floorshade)
    {
        *flags |= NET_SECTOR_FLOORSHADE;
        *(int8_t *)&pbuf[j] = sector[i].floorshade;
        j += sizeof(int8_t);
    }

    if (!lastsectupdate[i] || sector[i].floorpal != netsect[i].floorpal)
    {
        *flags |= NET_SECTOR_FLOORPAL;
        *(uint8_t *)&pbuf[j] = sector[i].floorpal;
        j += sizeof(uint8_t);
    }

    if (!lastsectupdate[i] || sector[i].floorxpanning != netsect[i].floorxpanning)
    {
        *flags |= NET_SECTOR_FLOORXPAN;
        *(uint8_t *)&pbuf[j] = sector[i].floorxpanning;
        j += sizeof(uint8_t);
    }

    if (!lastsectupdate[i] || sector[i].floorypanning != netsect[i].floorypanning)
    {
        *flags |= NET_SECTOR_FLOORYPAN;
        *(uint8_t *)&pbuf[j] = sector[i].floorypanning;
        j += sizeof(uint8_t);
    }

    if (!lastsectupdate[i] || sector[i].visibility != netsect[i].visibility)
    {
        *flags |= NET_SECTOR_VISIBILITY;
        *(uint8_t *)&pbuf[j] = sector[i].visibility;
        j += sizeof(uint8_t);
    }

    if (!lastsectupdate[i] || sector[i].lotag != netsect[i].lotag)
    {
        *flags |= NET_SECTOR_LOTAG;
        *(int16_t *)&pbuf[j] = sector[i].lotag;
        j += sizeof(int16_t);
    }

    if (!lastsectupdate[i] || sector[i].hitag != netsect[i].hitag)
    {
        *flags |= NET_SECTOR_HITAG;
        *(int16_t *)&pbuf[j] = sector[i].hitag;
        j += sizeof(int16_t);
    }

    if (!lastsectupdate[i] || sector[i].extra != netsect[i].extra)
    {
        *flags |= NET_SECTOR_EXTRA;
        *(int16_t *)&pbuf[j] = sector[i].extra;
        j += sizeof(int16_t);
    }

    if (lastsectupdate[i])
        Bmemcpy(&netsect[i], &sector[i], sizeof(sectortype));

    return *flags ? j : 0;
}


int32_t Net_UnpackWall(int32_t i, uint8_t *pbuf)
{
    int32_t j = 0;
    uint32_t flags = *(uint32_t *)&pbuf[j];

    j += sizeof(uint32_t);

    if (flags & NET_WALL_X)
    {
        wall[i].x = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }

    if (flags & NET_WALL_Y)
    {
        wall[i].y = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
    }

    if (flags & NET_WALL_POINT2)
    {
        wall[i].point2 = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_WALL_NEXTWALL)
    {
        wall[i].nextwall = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_WALL_NEXTSECTOR)
    {
        wall[i].nextsector = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_WALL_CSTAT)
    {
        wall[i].cstat = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_WALL_PICNUM)
    {
        wall[i].picnum = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_WALL_OVERPICNUM)
    {
        wall[i].overpicnum = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_WALL_SHADE)
    {
        wall[i].shade = *(int8_t *)&pbuf[j];
        j += sizeof(int8_t);
    }

    if (flags & NET_WALL_PAL)
    {
        wall[i].pal = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_WALL_XREPEAT)
    {
        wall[i].xrepeat = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_WALL_YREPEAT)
    {
        wall[i].yrepeat = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_WALL_XPANNING)
    {
        wall[i].xpanning = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_WALL_YPANNING)
    {
        wall[i].ypanning = *(uint8_t *)&pbuf[j];
        j += sizeof(uint8_t);
    }

    if (flags & NET_WALL_LOTAG)
    {
        wall[i].lotag = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_WALL_HITAG)
    {
        wall[i].hitag = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    if (flags & NET_WALL_EXTRA)
    {
        wall[i].extra = *(int16_t *)&pbuf[j];
        j += sizeof(int16_t);
    }

    return j;
}

int32_t Net_PackWall(int32_t i, uint8_t *pbuf)
{
    int32_t j = 0;
    uint32_t *flags;
    static walltype netwall[MAXWALLS];

    if (lastwallupdate[i] && !Bmemcmp(&wall[i], &netwall[i], sizeof(walltype)))
        return 0;

    *(int16_t *)&pbuf[j] = i;
    j += sizeof(int16_t);

    flags = (uint32_t *)&pbuf[j];
    *flags = 0;
    j += sizeof(uint32_t);

    if (!lastwallupdate[i] || wall[i].x != netwall[i].x)
    {
        *flags |= NET_WALL_X;
        *(int32_t *)&pbuf[j] = wall[i].x;
        j += sizeof(int32_t);
    }

    if (!lastwallupdate[i] || wall[i].y != netwall[i].y)
    {
        *flags |= NET_WALL_Y;
        *(int32_t *)&pbuf[j] = wall[i].y;
        j += sizeof(int32_t);
    }

    if (!lastwallupdate[i] || wall[i].point2 != netwall[i].point2)
    {
        *flags |= NET_WALL_POINT2;
        *(int16_t *)&pbuf[j] = wall[i].point2;
        j += sizeof(int16_t);
    }

    if (!lastwallupdate[i] || wall[i].nextwall != netwall[i].nextwall)
    {
        *flags |= NET_WALL_NEXTWALL;
        *(int16_t *)&pbuf[j] = wall[i].nextwall;
        j += sizeof(int16_t);
    }

    if (!lastwallupdate[i] || wall[i].nextsector != netwall[i].nextsector)
    {
        *flags |= NET_WALL_NEXTSECTOR;
        *(int16_t *)&pbuf[j] = wall[i].nextsector;
        j += sizeof(int16_t);
    }

    if (!lastwallupdate[i] || wall[i].cstat != netwall[i].cstat)
    {
        *flags |= NET_WALL_CSTAT;
        *(int16_t *)&pbuf[j] = wall[i].cstat;
        j += sizeof(int16_t);
    }

    if (!lastwallupdate[i] || wall[i].picnum != netwall[i].picnum)
    {
        *flags |= NET_WALL_PICNUM;
        *(int16_t *)&pbuf[j] = wall[i].picnum;
        j += sizeof(int16_t);
    }

    if (!lastwallupdate[i] || wall[i].overpicnum != netwall[i].overpicnum)
    {
        *flags |= NET_WALL_OVERPICNUM;
        *(int16_t *)&pbuf[j] = wall[i].overpicnum;
        j += sizeof(int16_t);
    }

    if (!lastwallupdate[i] || wall[i].shade != netwall[i].shade)
    {
        *flags |= NET_WALL_SHADE;
        *(int8_t *)&pbuf[j] = wall[i].shade;
        j += sizeof(int8_t);
    }

    if (!lastwallupdate[i] || wall[i].pal != netwall[i].pal)
    {
        *flags |= NET_WALL_PAL;
        *(uint8_t *)&pbuf[j] = wall[i].pal;
        j += sizeof(uint8_t);
    }

    if (!lastwallupdate[i] || wall[i].xrepeat != netwall[i].xrepeat)
    {
        *flags |= NET_WALL_XREPEAT;
        *(uint8_t *)&pbuf[j] = wall[i].xrepeat;
        j += sizeof(uint8_t);
    }

    if (!lastwallupdate[i] || wall[i].yrepeat != netwall[i].yrepeat)
    {
        *flags |= NET_WALL_YREPEAT;
        *(uint8_t *)&pbuf[j] = wall[i].yrepeat;
        j += sizeof(uint8_t);
    }

    if (!lastwallupdate[i] || wall[i].xpanning != netwall[i].xpanning)
    {
        *flags |= NET_WALL_XPANNING;
        *(uint8_t *)&pbuf[j] = wall[i].xpanning;
        j += sizeof(uint8_t);
    }

    if (!lastwallupdate[i] || wall[i].ypanning != netwall[i].ypanning)
    {
        *flags |= NET_WALL_YPANNING;
        *(uint8_t *)&pbuf[j] = wall[i].ypanning;
        j += sizeof(uint8_t);
    }

    if (!lastwallupdate[i] || wall[i].lotag != netwall[i].lotag)
    {
        *flags |= NET_WALL_LOTAG;
        *(int16_t *)&pbuf[j] = wall[i].lotag;
        j += sizeof(int16_t);
    }

    if (!lastwallupdate[i] || wall[i].hitag != netwall[i].hitag)
    {
        *flags |= NET_WALL_HITAG;
        *(int16_t *)&pbuf[j] = wall[i].hitag;
        j += sizeof(int16_t);
    }

    if (!lastwallupdate[i] || wall[i].extra != netwall[i].extra)
    {
        *flags |= NET_WALL_EXTRA;
        *(int16_t *)&pbuf[j] = wall[i].extra;
        j += sizeof(int16_t);
    }

    if (lastwallupdate[i])
        Bmemcpy(&netwall[i], &wall[i], sizeof(walltype));

    return *flags ? j : 0;
}


void Net_ParseServerPacket(ENetEvent *event)
{
    uint8_t *pbuf = event->packet->data;
    int32_t packbufleng = event->packet->dataLength;
    int32_t i, j, l;
    int32_t other = pbuf[--packbufleng];
    input_t *nsyn;

#if 0
    initprintf("RECEIVED PACKET: type: %d : len %d\n", pbuf[0], packbufleng);
#endif
    switch (pbuf[0])
    {
    case PACKET_MASTER_TO_SLAVE:  //[0] (receive master sync buffer)
        if (!(g_player[myconnectindex].ps->gm & MODE_GAME) || g_netSync) return;

        j = 0;

        packbufleng = qlz_size_decompressed((char *)&pbuf[1]);
        pbuf = (uint8_t *)Bcalloc(1, packbufleng+1);
        packbufleng = qlz_decompress((char *)&event->packet->data[1], (char *)(pbuf), state_decompress);

        ticrandomseed = *(int32_t *)&pbuf[j];
        j += sizeof(int32_t);
        ud.pause_on = pbuf[j++];

        for (TRAVERSE_CONNECT(i))
        {
            g_player[i].ps->dead_flag = *(int16_t *)&pbuf[j];
            j += sizeof(int16_t);

            g_player[i].playerquitflag = pbuf[j++];

            if (g_player[i].playerquitflag == 0) continue;

            /*            if (i == myconnectindex && !g_player[i].ps->dead_flag)
                        {
                            j += offsetof(input_t, filler) +
                                 (sizeof(vec3_t) * 3) + // position and velocity
                                 (sizeof(int16_t) * 3); // ang and horiz
                            goto process;
                        }
                        */

            nsyn = (input_t *)&inputfifo[0][0];

            if (i != myconnectindex || g_player[i].ps->dead_flag)
                Bmemcpy(&nsyn[i], &pbuf[j], offsetof(input_t, filler));

            j += offsetof(input_t, filler);

//                Bmemcpy(&g_player[i].ps->opos.x, &g_player[i].ps->pos.x, sizeof(vec3_t));

//            Bmemcpy(&g_player[i].ps->pos.x, &pbuf[j], sizeof(vec3_t) * 2);

            Bmemcpy(&sprite[g_player[i].ps->i], &pbuf[j], sizeof(vec3_t));
            sprite[g_player[i].ps->i].z += PHEIGHT;
            j += sizeof(vec3_t) * 2;

            Bmemcpy(&g_player[i].ps->vel.x, &pbuf[j], sizeof(vec3_t));
            j += sizeof(vec3_t);

            if (i != myconnectindex)
            {
                g_player[i].ps->oang = g_player[i].ps->ang;
                g_player[i].ps->ang = sprite[g_player[i].ps->i].ang = *(int16_t *)&pbuf[j];
                j += sizeof(int16_t);

                Bmemcpy(&g_player[i].ps->ohoriz, &g_player[i].ps->horiz, sizeof(int16_t) * 2);
                Bmemcpy(&g_player[i].ps->horiz, &pbuf[j], sizeof(int16_t) * 2);
                j += sizeof(int16_t) * 2;
            }
            else j += sizeof(int16_t) * 3;



//process:
            g_player[i].ps->gotweapon = *(uint16_t *)&pbuf[j];
            j += sizeof(uint16_t);

            Bmemcpy(&g_player[i].ps->ammo_amount[0], &pbuf[j], sizeof(g_player[i].ps->ammo_amount));
            j += sizeof(g_player[i].ps->ammo_amount);

            Bmemcpy(&g_player[i].ps->inv_amount[0], &pbuf[j], sizeof(g_player[i].ps->inv_amount));
            j += sizeof(g_player[i].ps->inv_amount);

            Bmemcpy(g_player[i].frags, &pbuf[j], sizeof(g_player[i].frags));
            j += sizeof(g_player[i].frags);

            sprite[g_player[i].ps->i].extra = (uint8_t)pbuf[j++];

            sprite[g_player[i].ps->i].cstat = *(int16_t *)&pbuf[j];
            j += sizeof(int16_t);

            g_player[i].ps->kickback_pic = (uint8_t)pbuf[j++];

            actor[g_player[i].ps->i].owner = *(int16_t *)&pbuf[j];
            j += sizeof(int16_t);

            actor[g_player[i].ps->i].picnum = *(int16_t *)&pbuf[j];
            j += sizeof(int16_t);

            g_player[i].ps->curr_weapon = (uint8_t)pbuf[j++];
            g_player[i].ps->last_weapon = (int8_t)pbuf[j++];
            g_player[i].ps->wantweaponfire = (int8_t)pbuf[j++];
            g_player[i].ps->weapon_pos = (int8_t)pbuf[j++];
            g_player[i].ps->frag_ps = (uint8_t)pbuf[j++];

            g_player[i].ps->frag = (uint8_t)pbuf[j++];

            g_player[i].ps->fraggedself = (uint8_t)pbuf[j++];

            g_player[i].ps->last_extra = (uint8_t)pbuf[j++];

            g_player[i].ping = *(int16_t *)&pbuf[j];
            j += sizeof(int16_t);

            g_player[i].ps->newowner = *(int16_t *)&pbuf[j];
            j += sizeof(int16_t);

            if (g_player[i].ps->newowner == -1 && g_player[i].ps->cursectnum >= 0 && g_player[i].ps->cursectnum < numsectors)
            {
                updatesectorz(g_player[i].ps->pos.x, g_player[i].ps->pos.y, g_player[i].ps->pos.z,
                    &g_player[i].ps->cursectnum);
                changespritesect(g_player[i].ps->i, g_player[i].ps->cursectnum);
            }

            sprite[g_player[i].ps->i].pal = (uint8_t)pbuf[j++];


            l = i;

            i = g_player[l].ps->i;

            {
#if defined SAMESIZE_ACTOR_T
                j++;
                Bmemcpy(&T5, &pbuf[j], sizeof(T5));
                j += sizeof(T5);
#else
                int16_t jj = j++;
                int32_t oa = (T5 >= (intptr_t)&script[0] && T5 < (intptr_t)&script[g_scriptSize]) ? T5-(intptr_t)&script[0] : T5;

                Bmemcpy(&T5, &pbuf[j], sizeof(T5));
                j += sizeof(T5);

                if (oa != T5) T3 = T4 = 0;
                if (pbuf[jj] & 2) T5 += (intptr_t)&script[0];
#endif
            }

            do
            {
                int16_t var_id = *(int16_t *)&pbuf[j];
                j += sizeof(int16_t);

                if (var_id == MAXGAMEVARS) break;

                aGameVars[var_id].val.plValues[i] = *(int32_t *)&pbuf[j];
                j += sizeof(int32_t);
            }
            while (1);

            i = l;

            do
            {
                int16_t var_id = *(int16_t *)&pbuf[j];
                j += sizeof(int16_t);

                if (var_id == MAXGAMEVARS) break;

                aGameVars[var_id].val.plValues[i] = *(int32_t *)&pbuf[j];
                j += sizeof(int32_t);
            }
            while (1);
        }

        {
            // sprite/sector/wall updates tacked on to the end of the packet

            l = *(int16_t *)&pbuf[j];
            j += sizeof(int16_t);

//            if (l) initprintf("unpacking %d sprites\n", l);

            while (l--)
            {
                int32_t spriteid = *(int16_t *)&pbuf[j];
                j += sizeof(int16_t);

/*
                initprintf("unpacking sprite %d at %d/%d\n", spriteid, j, packbufleng);
                initprintf("flags: %d\n", *(uint32_t *)&pbuf[j]);
*/
                j += Net_UnpackSprite(spriteid, &pbuf[j]);
            }
        }

        Bfree(pbuf);

        break;

    case PACKET_MAP_STREAM:
        if (!(g_player[myconnectindex].ps->gm & MODE_GAME) || g_netSync)
            return;

        j = 0;

        packbufleng = qlz_size_decompressed((char *)&pbuf[1]);
        pbuf = (uint8_t *)Bmalloc(packbufleng);
        packbufleng = qlz_decompress((char *)&event->packet->data[1], (char *)(pbuf), state_decompress);

        l = *(uint16_t *)&pbuf[j];
        j += sizeof(uint16_t);

//        if (l) initprintf("unpacking %d sprites\n", l);

        while (l--)
        {
            int32_t spriteid = *(int16_t *)&pbuf[j];
            j += sizeof(int16_t);
//            initprintf("unpacking msprite %d at %d/%d\n", spriteid, j, packbufleng);
            j += Net_UnpackSprite(spriteid, &pbuf[j]);
        }

        l = *(uint16_t *)&pbuf[j];
        j += sizeof(uint16_t);

        while (l--)
        {
            int16_t secid = *(int16_t *)&pbuf[j];
            j += sizeof(int16_t);

            j += Net_UnpackSect(secid, &pbuf[j]);
        }

        l = *(uint16_t *)&pbuf[j];
        j += sizeof(uint16_t);
        while (l--)
        {
            int16_t wallid = *(int16_t *)&pbuf[j];
            j += sizeof(int16_t);

            j += Net_UnpackWall(wallid, &pbuf[j]);

            // we call dragpoint() to make sure the nextwall position gets updated too
            dragpoint(wallid, wall[wallid].x, wall[wallid].y);
        }

        Bfree(pbuf);

        break;


    case PACKET_MESSAGE:
        Bstrncpy(recbuf, (char *)pbuf+2, packbufleng-2);
        recbuf[packbufleng-2] = 0;

        G_AddUserQuote(recbuf);
        S_PlaySound(EXITMENUSOUND);

        pus = pub = NUMPAGES;

        break;

    case PACKET_NEW_GAME:
        if ((vote_map + vote_episode + voting) != -3)
            G_AddUserQuote("VOTE SUCCEEDED");

        ud.m_level_number = ud.level_number = pbuf[1];
        ud.m_volume_number = ud.volume_number = pbuf[2];
        ud.m_player_skill = ud.player_skill = pbuf[3];
        ud.m_monsters_off = ud.monsters_off = pbuf[4];
        ud.m_respawn_monsters = ud.respawn_monsters = pbuf[5];
        ud.m_respawn_items = ud.respawn_items = pbuf[6];
        ud.m_respawn_inventory = ud.respawn_inventory = pbuf[7];
        ud.m_coop = pbuf[8];
        ud.m_marker = ud.marker = pbuf[9];
        ud.m_ffire = ud.ffire = pbuf[10];
        ud.m_noexits = ud.noexits = pbuf[11];

        for (TRAVERSE_CONNECT(i))
        {
            P_ResetWeapons(i);
            P_ResetInventory(i);
        }

        G_NewGame(ud.volume_number,ud.level_number,ud.player_skill);
        ud.coop = ud.m_coop;

        if (G_EnterLevel(MODE_GAME))
        {
            G_BackToMenu();
            break;
        }

        if (g_netSync)
        {
            packbuf[0] = PACKET_PLAYER_READY;
            packbuf[1] = myconnectindex;

            if (g_netClientPeer)
                enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(packbuf, 2, ENET_PACKET_FLAG_RELIABLE));

            g_netSync = 0;
        }

        break;

    case PACKET_VERSION:
        if (pbuf[1] != BYTEVERSION>>16 || pbuf[2] != (BYTEVERSION&255) || pbuf[3] != (uint8_t)atoi(s_buildDate))
        {
            initprintf("Server protocol is version %d.%d, expecting %d.%d\n",
                       ((pbuf[1]<<16)|pbuf[2]), pbuf[3], BYTEVERSION, (uint8_t)atoi(s_buildDate));
            initprintf("Server version mismatch!  You cannot play Duke with different versions!\n");
            g_netDisconnect = 1;
            return;
        }
        Net_SendChallenge();
        break;

    case PACKET_NUM_PLAYERS:
        numplayers = pbuf[1];
        playerswhenstarted = pbuf[2];
        ud.multimode = pbuf[3];
        if (pbuf[4]) // ID of new player
        {
            g_player[pbuf[4]].playerquitflag = 1;

            if (!g_player[pbuf[4]].ps) g_player[pbuf[4]].ps = (DukePlayer_t *) Bcalloc(1,sizeof(DukePlayer_t));
            if (!g_player[pbuf[4]].sync) g_player[pbuf[4]].sync = (input_t *) Bcalloc(1,sizeof(input_t));
        }

        if (pbuf[5] == NET_DEDICATED_SERVER)
            g_networkMode = NET_DEDICATED_CLIENT;

        for (i=0; i<playerswhenstarted-1; i++) connectpoint2[i] = i+1;
        connectpoint2[playerswhenstarted-1] = -1;

        S_PlaySound(DUKE_GETWEAPON2);

        // myconnectindex is 0 until we get PACKET_PLAYER_INDEX
        if (myconnectindex != 0)
            Net_SendClientInfo();
        break;

        // receive client player index from server
    case PACKET_PLAYER_INDEX:
        myconnectindex = pbuf[1];
        g_player[myconnectindex].playerquitflag = 1;
        Net_SendClientInfo();
        break;

    case PACKET_PLAYER_DISCONNECTED:
        if ((g_player[myconnectindex].ps->gm & MODE_GAME) && !g_netSync)
            P_RemovePlayer(pbuf[1]);
        numplayers = pbuf[2];
        ud.multimode = pbuf[3];
        playerswhenstarted = pbuf[4];
        break;

    case PACKET_PLAYER_SPAWN:
        if (!(g_player[myconnectindex].ps->gm & MODE_GAME) || g_netSync) break;

        P_ResetPlayer(pbuf[1]);
        Bmemcpy(&g_player[pbuf[1]].ps->pos.x, &pbuf[2], sizeof(vec3_t) * 2);
        Bmemcpy(&sprite[g_player[pbuf[1]].ps->i], &pbuf[2], sizeof(vec3_t));
        break;

    case PACKET_PLAYER_PING:
        g_player[0].pingcnt++;
        return;

    case PACKET_FRAG:
        if (!(g_player[myconnectindex].ps->gm & MODE_GAME) || g_netSync) break;
        g_player[pbuf[1]].ps->frag_ps = pbuf[2];
        actor[g_player[pbuf[1]].ps->i].picnum = pbuf[3];
        ticrandomseed = *(int32_t *)&pbuf[4];
        P_FragPlayer(pbuf[1]);
        break;

    case PACKET_CLIENT_INFO:
        for (i=1; pbuf[i]; i++)
            g_player[other].user_name[i-1] = pbuf[i];
        g_player[other].user_name[i-1] = 0;
        i++;

        g_player[other].ps->aim_mode = pbuf[i++];
        g_player[other].ps->auto_aim = pbuf[i++];
        g_player[other].ps->weaponswitch = pbuf[i++];
        g_player[other].ps->palookup = g_player[other].pcolor = pbuf[i++];
        g_player[other].pteam = pbuf[i++];

        j = i;
        for (; i-j<10; i++) g_player[other].wchoice[i-j] = pbuf[i];

        g_player[other].playerquitflag = 1;

        break;


    case PACKET_RTS:
        if (rts_numlumps == 0) break;

        if (ud.config.SoundToggle == 0 || ud.lockout == 1 || ud.config.FXDevice < 0 || !(ud.config.VoiceToggle & 4))
            break;
        FX_PlayAuto3D((char *)RTS_GetSound(pbuf[1]-1),RTS_SoundLength(pbuf[1]-1),0,0,0,255,-pbuf[1]);
        g_RTSPlaying = 7;

        break;

    case PACKET_USER_MAP:
        Bstrcpy(boardfilename,(char *)pbuf+1);
        boardfilename[packbufleng-1] = 0;
        Bcorrectfilename(boardfilename,0);
        if (boardfilename[0] != 0)
        {
            if ((i = kopen4loadfrommod(boardfilename,0)) < 0)
            {
                Bmemset(boardfilename,0,sizeof(boardfilename));
                Net_SendUserMapName();
            }
            else kclose(i);
        }

        if (ud.m_level_number == 7 && ud.m_volume_number == 0 && boardfilename[0] == 0)
            ud.m_level_number = 0;

        break;

    case PACKET_MAP_VOTE:
        if (voting == myconnectindex && g_player[(uint8_t)pbuf[1]].gotvote == 0)
        {
            g_player[(uint8_t)pbuf[1]].gotvote = 1;
            g_player[(uint8_t)pbuf[1]].vote = pbuf[2];
            Bsprintf(tempbuf,"CONFIRMED VOTE FROM %s",g_player[(uint8_t)pbuf[1]].user_name);
            G_AddUserQuote(tempbuf);
        }
        break;

    case PACKET_MAP_VOTE_INITIATE: // call map vote
        voting = pbuf[1];
        vote_episode = pbuf[2];
        vote_map = pbuf[3];

        Bsprintf(tempbuf,"%s^00 HAS CALLED A VOTE TO CHANGE MAP TO %s (E%dL%d)",
                 g_player[(uint8_t)pbuf[1]].user_name,
                 MapInfo[(uint8_t)(pbuf[2]*MAXLEVELS + pbuf[3])].name,
                 pbuf[2]+1,pbuf[3]+1);
        G_AddUserQuote(tempbuf);

        Bsprintf(tempbuf,"PRESS F1 TO ACCEPT, F2 TO DECLINE");
        G_AddUserQuote(tempbuf);

        for (i=MAXPLAYERS-1; i>=0; i--)
        {
            g_player[i].vote = 0;
            g_player[i].gotvote = 0;
        }
        g_player[voting].gotvote = g_player[voting].vote = 1;
        break;

    case PACKET_MAP_VOTE_CANCEL: // cancel map vote
        if (voting == pbuf[1])
        {
            voting = -1;
            i = 0;
            for (j=MAXPLAYERS-1; j>=0; j--)
                i += g_player[j].gotvote;

            if (i != numplayers)
                Bsprintf(tempbuf,"%s^00 HAS CANCELED THE VOTE",g_player[(uint8_t)pbuf[1]].user_name);
            else Bsprintf(tempbuf,"VOTE FAILED");
            for (i=MAXPLAYERS-1; i>=0; i--)
            {
                g_player[i].vote = 0;
                g_player[i].gotvote = 0;
            }
            G_AddUserQuote(tempbuf);
        }
        break;
    }
}

void Net_ParseClientPacket(ENetEvent *event)
{
    uint8_t *pbuf = event->packet->data;
    int32_t packbufleng = event->packet->dataLength;
    int16_t i, j;
    int32_t other = pbuf[--packbufleng];
    input_t *nsyn;

#if 0
    initprintf("RECEIVED PACKET: type: %d : len %d\n", pbuf[0], packbufleng);
#endif
    switch (pbuf[0])
    {
    case PACKET_SLAVE_TO_MASTER:  //[1] (receive slave sync buffer)
        j = 0;

        packbufleng = qlz_size_decompressed((char *)&pbuf[1]);
        pbuf = (uint8_t *)Bcalloc(1, packbufleng+1);
        packbufleng = qlz_decompress((char *)&event->packet->data[1], (char *)(pbuf), state_decompress);

        nsyn = (input_t *)&inputfifo[0][0];

        Bmemcpy(&nsyn[other], &pbuf[j], sizeof(input_t));

        j += offsetof(input_t, filler);

        // anyone the server thinks is dead can go fuck themselves
        if (g_player[other].ps->dead_flag)
        {
            Bfree(pbuf);
            break;
        }
        else
            g_player[other].playerquitflag = 1;

//            Bmemcpy(&g_player[other].ps->opos.x, &g_player[other].ps->pos.x, sizeof(vec3_t));
//        Bmemcpy(&g_player[other].ps->pos.x, &pbuf[j], sizeof(vec3_t) * 2);
//        updatesectorz(g_player[other].ps->pos.x, g_player[other].ps->pos.y, g_player[other].ps->pos.z,
//                      &g_player[other].ps->cursectnum);
//        Bmemcpy(&sprite[g_player[other].ps->i], &pbuf[j], sizeof(vec3_t));
//        sprite[g_player[other].ps->i].z += PHEIGHT;
//        changespritesect(g_player[other].ps->i, g_player[other].ps->cursectnum);
//        Bmemcpy(&g_player[other].ps->npos.x, &pbuf[j], sizeof(vec3_t));
        j += sizeof(vec3_t) * 2;

//        Bmemcpy(&g_player[other].ps->vel.x, &pbuf[j], sizeof(vec3_t));
        j += sizeof(vec3_t);

        g_player[other].ps->oang = g_player[other].ps->ang;
        Bmemcpy(&g_player[other].ps->ang, &pbuf[j], sizeof(int16_t));
        Bmemcpy(&sprite[g_player[other].ps->i].ang, &pbuf[j], sizeof(int16_t));
        j += sizeof(int16_t);

        Bmemcpy(&g_player[other].ps->ohoriz, &g_player[other].ps->horiz, sizeof(int16_t) * 2);
        Bmemcpy(&g_player[other].ps->horiz, &pbuf[j], sizeof(int16_t) * 2);
        j += sizeof(int16_t) * 2;

        Bfree(pbuf);
        break;

    case PACKET_PLAYER_READY:
        j = g_player[other].ps->i;
        Bmemcpy(g_player[other].ps, g_player[0].ps, sizeof(DukePlayer_t));

        g_player[other].ps->i = j;
        changespritestat(j, STAT_PLAYER);

        g_player[other].ps->last_extra = sprite[g_player[other].ps->i].extra = g_player[other].ps->max_player_health;
        sprite[g_player[other].ps->i].cstat = 1+256;
        actor[g_player[other].ps->i].t_data[2] = actor[g_player[other].ps->i].t_data[3] = actor[g_player[other].ps->i].t_data[4] = 0;

        P_ResetPlayer(other);

        j = 0;
        packbuf[j++] = PACKET_PLAYER_SPAWN;
        packbuf[j++] = other;

        Bmemcpy(&packbuf[j], &g_player[other].ps->pos.x, sizeof(vec3_t) * 2);
        j += sizeof(vec3_t) * 2;

        packbuf[j++] = 0;

        enet_host_broadcast(g_netServer, CHAN_GAMESTATE , enet_packet_create(packbuf, j, ENET_PACKET_FLAG_RELIABLE));

        // a player connecting is a good time to mark things as needing to be updated
        // we invalidate everything that has changed since we started sending the snapshot of the map to the new player

        {
            int32_t zz, i, nexti;

            for (zz = 0; (unsigned)zz < (sizeof(g_netStatnums)/sizeof(g_netStatnums[0])); zz++)
                for (TRAVERSE_SPRITE_STAT(headspritestat[g_netStatnums[zz]], i, nexti))
                {
                    if (lastupdate[i] >= g_player[other].netsynctime)
                        lastupdate[i] = 0;
                }
        }

        for (i=numwalls-1; i>=0; i--)
            if (lastwallupdate[i] >= g_player[other].netsynctime)
                lastwallupdate[i] = 0;

        for (i=numsectors-1; i>=0; i--)
            if (lastsectupdate[i] >= g_player[other].netsynctime)
                lastsectupdate[i] = 0;

        break;


    case PACKET_PLAYER_PING:
        if (g_player[myconnectindex].ps->gm & MODE_GAME)
        {
            packbuf[0] = PACKET_PLAYER_PING;
            packbuf[1] = myconnectindex;
            enet_peer_send(event->peer, CHAN_GAMESTATE, enet_packet_create(packbuf, 2, ENET_PACKET_FLAG_RELIABLE));
        }
        g_player[other].pingcnt++;
        return;

    case PACKET_MESSAGE:
        Bstrncpy(recbuf, (char *)pbuf+2, packbufleng-2);
        recbuf[packbufleng-2] = 0;

        G_AddUserQuote(recbuf);
        S_PlaySound(EXITMENUSOUND);

        pus = pub = NUMPAGES;
        break;

    case PACKET_NEW_GAME:
        if ((vote_map + vote_episode + voting) != -3)
            G_AddUserQuote("VOTE SUCCEEDED");

        ud.m_level_number = ud.level_number = pbuf[1];
        ud.m_volume_number = ud.volume_number = pbuf[2];
        ud.m_player_skill = ud.player_skill = pbuf[3];
        ud.m_monsters_off = ud.monsters_off = pbuf[4];
        ud.m_respawn_monsters = ud.respawn_monsters = pbuf[5];
        ud.m_respawn_items = ud.respawn_items = pbuf[6];
        ud.m_respawn_inventory = ud.respawn_inventory = pbuf[7];
        ud.m_coop = pbuf[8];
        ud.m_marker = ud.marker = pbuf[9];
        ud.m_ffire = ud.ffire = pbuf[10];
        ud.m_noexits = ud.noexits = pbuf[11];

        for (TRAVERSE_CONNECT(i))
        {
            P_ResetWeapons(i);
            P_ResetInventory(i);
        }

        G_NewGame(ud.volume_number,ud.level_number,ud.player_skill);
        ud.coop = ud.m_coop;

        if (G_EnterLevel(MODE_GAME)) G_BackToMenu();
        break;

    case PACKET_AUTH:
    {
        uint32_t crc = *(uint32_t *)&pbuf[1];

        if (crc == crc32once((uint8_t *)g_netPassword, Bstrlen(g_netPassword)))
            Net_SyncPlayer(event);
        else
        {
            enet_peer_disconnect_later(event->peer, DISC_BAD_PASSWORD);
            initprintf("Bad password from client.\n");
        }
    }
    break;

    case PACKET_CLIENT_INFO:
        for (i=1; pbuf[i]; i++)
            g_player[other].user_name[i-1] = pbuf[i];
        g_player[other].user_name[i-1] = 0;
        i++;

        g_player[other].ps->aim_mode = pbuf[i++];
        g_player[other].ps->auto_aim = pbuf[i++];
        g_player[other].ps->weaponswitch = pbuf[i++];
        g_player[other].ps->palookup = g_player[other].pcolor = pbuf[i++];
        g_player[other].pteam = pbuf[i++];

        for (j=i; i-j<10; i++) g_player[other].wchoice[i-j] = pbuf[i];
        break;

    case PACKET_RTS:
        if (rts_numlumps == 0) break;

        if (ud.config.SoundToggle == 0 || ud.lockout == 1 || ud.config.FXDevice < 0 || !(ud.config.VoiceToggle & 4))
            break;

        FX_PlayAuto3D((char *)RTS_GetSound(pbuf[1]-1),RTS_SoundLength(pbuf[1]-1),0,0,0,255,-pbuf[1]);
        g_RTSPlaying = 7;
        break;

    case PACKET_USER_MAP:
        Bstrcpy(boardfilename,(char *)pbuf+1);
        boardfilename[packbufleng-1] = 0;
        Bcorrectfilename(boardfilename,0);
        if (boardfilename[0] != 0)
        {
            if ((i = kopen4loadfrommod(boardfilename,0)) < 0)
            {
                Bmemset(boardfilename,0,sizeof(boardfilename));
                Net_SendUserMapName();
            }
            else kclose(i);
        }

        if (ud.m_level_number == 7 && ud.m_volume_number == 0 && boardfilename[0] == 0)
            ud.m_level_number = 0;
        break;

    case PACKET_MAP_VOTE:
        if (voting == myconnectindex && g_player[(uint8_t)pbuf[1]].gotvote == 0)
        {
            g_player[(uint8_t)pbuf[1]].gotvote = 1;
            g_player[(uint8_t)pbuf[1]].vote = pbuf[2];
            Bsprintf(tempbuf,"CONFIRMED VOTE FROM %s",g_player[(uint8_t)pbuf[1]].user_name);
            G_AddUserQuote(tempbuf);
        }
        break;

    case PACKET_MAP_VOTE_INITIATE:
        voting = pbuf[1];
        vote_episode = pbuf[2];
        vote_map = pbuf[3];

        Bsprintf(tempbuf,"%s^00 HAS CALLED A VOTE TO CHANGE MAP TO %s (E%dL%d)",
                 g_player[(uint8_t)pbuf[1]].user_name,
                 MapInfo[(uint8_t)(pbuf[2]*MAXLEVELS + pbuf[3])].name,
                 pbuf[2]+1,pbuf[3]+1);
        G_AddUserQuote(tempbuf);

        Bsprintf(tempbuf,"PRESS F1 TO ACCEPT, F2 TO DECLINE");
        G_AddUserQuote(tempbuf);

        for (i=MAXPLAYERS-1; i>=0; i--)
        {
            g_player[i].vote = 0;
            g_player[i].gotvote = 0;
        }
        g_player[voting].gotvote = g_player[voting].vote = 1;
        break;

    case PACKET_MAP_VOTE_CANCEL:
        if (voting == pbuf[1])
        {
            voting = -1;
            i = 0;
            for (j=MAXPLAYERS-1; j>=0; j--)
                i += g_player[j].gotvote;

            if (i != numplayers)
                Bsprintf(tempbuf,"%s^00 HAS CANCELED THE VOTE",g_player[(uint8_t)pbuf[1]].user_name);
            else Bsprintf(tempbuf,"VOTE FAILED");
            for (i=MAXPLAYERS-1; i>=0; i--)
            {
                g_player[i].vote = 0;
                g_player[i].gotvote = 0;
            }
            G_AddUserQuote(tempbuf);
        }
        break;

    case PACKET_REQUEST_GAMESTATE:
        if (g_netServer && g_player[0].ps->gm & MODE_GAME)
        {
            packbuf[0] = PACKET_NEW_GAME;
            packbuf[1] = ud.level_number;
            packbuf[2] = ud.volume_number;
            packbuf[3] = ud.player_skill+1;
            packbuf[4] = ud.monsters_off;
            packbuf[5] = ud.respawn_monsters;
            packbuf[6] = ud.respawn_items;
            packbuf[7] = ud.respawn_inventory;
            packbuf[8] = ud.coop;
            packbuf[9] = ud.marker;
            packbuf[10] = ud.ffire;
            packbuf[11] = ud.noexits;
            packbuf[12] = myconnectindex;

            enet_peer_send(event->peer, CHAN_GAMESTATE, enet_packet_create(packbuf, 13, ENET_PACKET_FLAG_RELIABLE));

            g_netPlayersWaiting--;
        }
        break;
    }
}


void Net_GetPackets(void)
{
    sampletimer();
    MUSIC_Update();
    S_Update();

    G_HandleSpecialKeys();

    if (g_netDisconnect)
    {
        Net_Disconnect();
        g_netDisconnect = 0;

        if (g_gameQuit)
            G_GameExit(" ");

        return;
    }

    if (g_netServer)
    {
        ENetEvent event;

        // pull events from the wire into the packet queue without dispatching them, once per Net_GetPackets() call
        enet_host_service(g_netServer, NULL, 0);

        // dispatch any pending events from the local packet queue
        while (enet_host_check_events(g_netServer, &event) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
            {
                char ipaddr[32];

                enet_address_get_host_ip(&event.peer->address, ipaddr, sizeof(ipaddr));

                initprintf("A new client connected from %s:%u.\n",
                           ipaddr, event.peer -> address.port);
            }
            Net_SendVersion(event.peer);
            break;

            case ENET_EVENT_TYPE_RECEIVE:
                /*
                initprintf ("A packet of length %u containing %s was received from player %d on channel %u.\n",
                event.packet -> dataLength,
                event.packet -> data,
                event.peer -> data,
                event.channelID);
                */
                Net_ParseClientPacket(&event);
                // broadcast takes care of enet_packet_destroy itself
                // we set the state to disconnected so enet_host_broadcast doesn't send the player back his own packets
                if ((event.channelID == CHAN_GAMESTATE && event.packet->data[0] > PACKET_BROADCAST) || event.channelID == CHAN_CHAT)
                {
                    event.peer->state = ENET_PEER_STATE_DISCONNECTED;
                    enet_host_broadcast(g_netServer, event.channelID,
                                        enet_packet_create(event.packet->data, event.packet->dataLength, event.packet->flags & ENET_PACKET_FLAG_RELIABLE));
                    event.peer->state = ENET_PEER_STATE_CONNECTED;
                }

                enet_packet_destroy(event.packet);
                g_player[(intptr_t)event.peer->data].ping = (event.peer->lastRoundTripTime + event.peer->roundTripTime)/2;
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                numplayers--;
                ud.multimode--;

                P_RemovePlayer((intptr_t)event.peer->data);

                packbuf[0] = PACKET_PLAYER_DISCONNECTED;
                packbuf[1] = (intptr_t)event.peer->data;
                packbuf[2] = numplayers;
                packbuf[3] = ud.multimode;
                packbuf[4] = playerswhenstarted;
                packbuf[5] = myconnectindex;

                enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, 6, ENET_PACKET_FLAG_RELIABLE));

                initprintf("%s disconnected.\n", g_player[(intptr_t)event.peer->data].user_name);
                event.peer->data = NULL;
                break;

            default:
                break;
            }
        }
    }
    else if (g_netClient)
    {
        ENetEvent event;
        size_t datasiz = 0;

        do
        {
            enet_host_service(g_netClient, NULL, 0);

            while (enet_host_check_events(g_netClient, &event) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_RECEIVE:

                    /*
                    initprintf("A packet of length %u was received from player %d on channel %u.\n",
                    event.packet -> dataLength,
                    event.peer -> data,
                    event.channelID);
                    */

                    // mapstate transfer from the server... all packets but the last are SYNCPACKETSIZE
                    if (event.channelID == CHAN_SYNC)
                    {
                        static uint8_t *buf = NULL;

                        if (buf == NULL)
                        {
                            g_netSync = 1;
                            buf = (uint8_t *)Bcalloc(1, sizeof(mapstate_t)<<1);
                        }

                        g_multiMapState = (mapstate_t *)Brealloc(g_multiMapState, sizeof(mapstate_t));

                        rotatesprite_fs(0,0,65536L,0,BETASCREEN,0,0,2+8+16+64);

                        rotatesprite_fs(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8);
                        rotatesprite_fs(160<<16,(129)<<16,30<<11,0,THREEDEE,0,0,2+8);
                        if (PLUTOPAK)   // JBF 20030804
                            rotatesprite_fs(160<<16,(151)<<16,30<<11,0,PLUTOPAKSPRITE+1,0,0,2+8);

                        if (buf && event.packet->dataLength == SYNCPACKETSIZE)
                        {
                            char tbuf[64];
                            Bmemcpy((uint8_t *)(buf)+datasiz, event.packet->data, event.packet->dataLength);
                            datasiz += SYNCPACKETSIZE;

                            Bsprintf(tbuf, "RECEIVED %d BYTES\n", (int32_t)datasiz);
                            gametext(160,190,tbuf,14,2);
                        }
                        // last packet of mapstate sequence
                        else if (buf)
                        {
                            Bmemcpy((uint8_t *)(buf)+datasiz, event.packet->data, event.packet->dataLength);
                            datasiz = 0;
                            //                        g_netSync = 0;

                            if (qlz_size_decompressed((const char *)buf) == sizeof(mapstate_t))
                            {
                                qlz_decompress((const char *)buf, g_multiMapState, state_decompress);
                                Bfree(buf);
                                buf = NULL;

                                packbuf[0] = PACKET_REQUEST_GAMESTATE;
                                packbuf[1] = myconnectindex;
                                enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&packbuf[0], 2, ENET_PACKET_FLAG_RELIABLE));

                                gametext(160,190,"TRANSFER COMPLETE",14,2);
                            }
                            else
                            {
                                initprintf("Invalid map state from server!\n");
                                Bfree(buf);
                                buf = NULL;
                                g_netDisconnect = 1;
                                g_netSync = 0;

                                gametext(160,190,"TRANSFER ERROR",14,2);
                            }
                        }
                        else
                        {
                            initprintf("Error allocating buffer for map state!\n");
                            g_netDisconnect = 1;
                            g_netSync = 0;

                            gametext(160,190,"TRANSFER ERROR",14,2);
                        }

                        nextpage();
                    }
                    else Net_ParseServerPacket(&event);

                    enet_packet_destroy(event.packet);
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    g_netDisconnect = 1;
                    numplayers = playerswhenstarted = ud.multimode = 1;
                    myconnectindex = screenpeek = 0;
                    G_BackToMenu();
                    switch (event.data)
                    {
                    case DISC_BAD_PASSWORD:
                        initprintf("Bad password.\n");
                        return;
                    case DISC_KICKED:
                        initprintf("You have been kicked from the server.\n");
                        return;
                    case DISC_BANNED:
                        initprintf("You are banned from this server.\n");
                        return;
                    default:
                        initprintf("Disconnected.\n");
                        return;
                    }
                default:
                    break;
                }
            }
        }
        while (datasiz);
    }
}

void Net_ClientMove(void)
{
    int32_t j;
    input_t *nsyn = (input_t *)&inputfifo[0][myconnectindex];

    packbuf[0] = PACKET_SLAVE_TO_MASTER;
    j = 1;

    Bmemcpy(&packbuf[j], &nsyn[0], offsetof(input_t, filler));
    j += offsetof(input_t, filler);

    Bmemcpy(&packbuf[j], &g_player[myconnectindex].ps->pos.x, sizeof(vec3_t) * 2);
    j += sizeof(vec3_t) * 2;

    Bmemcpy(&packbuf[j], &g_player[myconnectindex].ps->vel.x, sizeof(vec3_t));
    j += sizeof(vec3_t);

    *(int16_t *)&packbuf[j] = g_player[myconnectindex].ps->ang;
    j += sizeof(int16_t);

    Bmemcpy(&packbuf[j], &g_player[myconnectindex].ps->horiz, sizeof(int16_t) * 2);
    j += sizeof(int16_t) * 2;

    {
        char buf[1024];

        j = qlz_compress((char *)(packbuf)+1, (char *)buf, j, state_compress);
        Bmemcpy((char *)(packbuf)+1, (char *)buf, j);
        j++;
    }

    packbuf[j++] = myconnectindex;

    enet_peer_send(g_netClientPeer, CHAN_MOVE, enet_packet_create(packbuf, j, 0));

}

void Net_UpdateClients(void)
{
    input_t *osyn = (input_t *)&inputfifo[1][0];
    input_t *nsyn = (input_t *)&inputfifo[0][0];
    int16_t i, nexti, k = 0, l;
    int32_t j;

    if (!g_netServer || numplayers < 2)
    {
        ticrandomseed = randomseed;
        if (g_netServer)
            Bmemcpy(&osyn[0], &nsyn[0], sizeof(input_t));
        return;
    }

    packbuf[0] = PACKET_MASTER_TO_SLAVE;
    j = 1;

    *(int32_t *)&packbuf[j] = ticrandomseed = randomseed;
    j += sizeof(int32_t);
    packbuf[j++] = ud.pause_on;

    for (TRAVERSE_CONNECT(i))
    {
        Bmemcpy(&osyn[i], &nsyn[i], offsetof(input_t, filler));

        *(int16_t *)&packbuf[j] = g_player[i].ps->dead_flag;
        j += sizeof(int16_t);

        packbuf[j++] = g_player[i].playerquitflag;

        if (g_player[i].playerquitflag == 0) continue;

        Bmemcpy(&packbuf[j], &nsyn[i], offsetof(input_t, filler));
        j += offsetof(input_t, filler);

        Bmemcpy(&packbuf[j], &g_player[i].ps->pos.x, sizeof(vec3_t) * 2);
        j += sizeof(vec3_t) * 2;

        Bmemcpy(&packbuf[j], &g_player[i].ps->vel.x, sizeof(vec3_t));
        j += sizeof(vec3_t);

        *(int16_t *)&packbuf[j] = g_player[i].ps->ang;
        j += sizeof(int16_t);

        Bmemcpy(&packbuf[j], &g_player[i].ps->horiz, sizeof(int16_t) * 2);
        j += sizeof(int16_t) * 2;

        *(uint16_t *)&packbuf[j] = g_player[i].ps->gotweapon;
        j += sizeof(uint16_t);

        Bmemcpy(&packbuf[j], &g_player[i].ps->ammo_amount[0], sizeof(g_player[i].ps->ammo_amount));
        j += sizeof(g_player[i].ps->ammo_amount);

        Bmemcpy(&packbuf[j], &g_player[i].ps->inv_amount[0], sizeof(g_player[i].ps->inv_amount));
        j += sizeof(g_player[i].ps->inv_amount);

        Bmemcpy(&packbuf[j], g_player[i].frags, sizeof(g_player[i].frags));
        j += sizeof(g_player[i].frags);

        packbuf[j++] = (uint8_t) sprite[g_player[i].ps->i].extra;

        *(int16_t *)&packbuf[j] = sprite[g_player[i].ps->i].cstat;
        j += sizeof(int16_t);

        packbuf[j++] = (uint8_t) g_player[i].ps->kickback_pic;

        *(int16_t *)&packbuf[j] = actor[g_player[i].ps->i].owner;
        j += sizeof(int16_t);

        *(int16_t *)&packbuf[j] = actor[g_player[i].ps->i].picnum;
        j += sizeof(int16_t);

        packbuf[j++] = (uint8_t) g_player[i].ps->curr_weapon;
        packbuf[j++] = (int8_t) g_player[i].ps->last_weapon;
        packbuf[j++] = (int8_t) g_player[i].ps->wantweaponfire;
        packbuf[j++] = (int8_t) g_player[i].ps->weapon_pos;
        packbuf[j++] = (uint8_t) g_player[i].ps->frag_ps;

        packbuf[j++] = g_player[i].ps->frag;
        packbuf[j++] = g_player[i].ps->fraggedself;
        packbuf[j++] = g_player[i].ps->last_extra;

        *(int16_t *)&packbuf[j] = g_player[i].ping;
        j += sizeof(int16_t);

        *(int16_t *)&packbuf[j] = g_player[i].ps->newowner;
        j += sizeof(int16_t);

        packbuf[j++] = sprite[g_player[i].ps->i].pal;

        l = i;
        i = g_player[l].ps->i;

        {
            int32_t jj;
#if !defined SAMESIZE_ACTOR_T
            int32_t oa;
#endif
            packbuf[(jj = j++)] = 0;

#if !defined SAMESIZE_ACTOR_T
            if (T5 >= (intptr_t)&script[0] && T5 < (intptr_t)(&script[g_scriptSize]))
            {
                packbuf[jj] |= 2;
                T5 -= (intptr_t)&script[0];
            }

            oa = T5;
#endif
            Bmemcpy(&packbuf[j], &T5, sizeof(T5));
            j += sizeof(T5);

#if !defined SAMESIZE_ACTOR_T
            if (oa != T5) T3 = T4 = 0;

            if (packbuf[jj] & 2) T5 += (intptr_t)&script[0];
#endif
        }

        {
            int16_t ii=g_gameVarCount-1, kk = 0;

            for (; ii>=0 && j <= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)); ii--)
            {
                if ((aGameVars[ii].dwFlags & (GAMEVAR_PERACTOR|GAMEVAR_NOMULTI)) == GAMEVAR_PERACTOR && aGameVars[ii].val.plValues)
                {
                    if (aGameVars[ii].val.plValues[i] != aGameVars[ii].lDefault)
                    {
                        *(int16_t *)&packbuf[j] = ii;
                        j += sizeof(int16_t);
                        *(int32_t *)&packbuf[j] = aGameVars[ii].val.plValues[i];
                        j += sizeof(int32_t);
                        kk++;

                        if (j >= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)))
                            break;
                    }
                }
            }
            *(int16_t *)&packbuf[j] = MAXGAMEVARS;
            j += sizeof(int16_t);
        }

        i = l;

        {
            int16_t ii=g_gameVarCount-1, kk = 0;

            for (; ii>=0 && j <= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)); ii--)
            {
                if ((aGameVars[ii].dwFlags & (GAMEVAR_PERPLAYER|GAMEVAR_NOMULTI)) == GAMEVAR_PERPLAYER && aGameVars[ii].val.plValues)
                {
                    if (aGameVars[ii].val.plValues[i] != aGameVars[ii].lDefault)
                    {
                        *(int16_t *)&packbuf[j] = ii;
                        j += sizeof(int16_t);
                        *(int32_t *)&packbuf[j] = aGameVars[ii].val.plValues[i];
                        j += sizeof(int32_t);
                        kk++;

                        if (j >= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)))
                            break;
                    }
                }
            }
            *(int16_t *)&packbuf[j] = MAXGAMEVARS;
            j += sizeof(int16_t);
        }
    }

    k = 0;

    {
        int32_t zz, zj;

        packbuf[(zj = j)] = 0;
        j += sizeof(int16_t);

        for (zz = 0; (unsigned)zz < (sizeof(g_netStatnums)/sizeof(g_netStatnums[0])) && j <= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)); zz++)
            for (TRAVERSE_SPRITE_STAT(headspritestat[g_netStatnums[zz]], i, nexti))
            {
                // only send newly spawned sprites
                if (!lastupdate[i] && sprite[i].statnum != MAXSTATUS)
                {
                    int32_t ii;

                    j += (ii = Net_PackSprite(i, (uint8_t *)&packbuf[j]));
                    if (ii) k++;

                    lastupdate[i] = totalclock;

                    if (j >= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)))
                        break;
                }
            }

        *(int16_t *)&packbuf[zj] = k;
        j += sizeof(int16_t);
    }

    {
        char buf[PACKBUF_SIZE];

        if (j >= PACKBUF_SIZE)
        {
            initprintf("Global packet buffer overflow! Size of packet: %i\n", j);
        }

        j = qlz_compress((char *)(packbuf)+1, (char *)buf, j, state_compress);
        Bmemcpy((char *)(packbuf)+1, (char *)buf, j);
        j++;
    }

    packbuf[j++] = myconnectindex;

    enet_host_broadcast(g_netServer, CHAN_MOVE, enet_packet_create(packbuf, j, 0));
}

void Net_StreamLevel(void)
{
    int16_t i, nexti;
    int32_t j = 0;
    int32_t zz, zj, k = 0, l;

    if (!g_netServer || numplayers < 2)
        return;

    packbuf[j++] = PACKET_MAP_STREAM;

    *(uint16_t *)&packbuf[(zj = j)] = 0;
    j += sizeof(uint16_t);

    for (zz = 0; (unsigned)zz < (sizeof(g_netStatnums)/sizeof(g_netStatnums[0])) && j <= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)); zz++)
        for (TRAVERSE_SPRITE_STAT(headspritestat[g_netStatnums[zz]], i, nexti))
        {
            // only send STAT_MISC sprites at spawn time and let the client handle it from there
            if (totalclock > (lastupdate[i] + TICRATE) && sprite[i].statnum != STAT_MISC)
            {
                l = crc32once((uint8_t *)&sprite[i], sizeof(spritetype));

                if (!lastupdate[i] || spritecrc[i] != l)
                {
                    int32_t ii;
                    /*initprintf("updating sprite %d (%d)\n",i,sprite[i].picnum);*/
                    spritecrc[i] = l;

                    j += (ii = Net_PackSprite(i, (uint8_t *)&packbuf[j]));
                    if (ii) k++;

                    lastupdate[i] = totalclock;

                    if (j >= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)))
                        break;
                }
            }
        }

    *(uint16_t *)&packbuf[zj] = k;
    k = 0;

    *(uint16_t *)&packbuf[(zj = j)] = 0;
    j += sizeof(uint16_t);

    for (i = numsectors-1; i >= 0 && j <= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)); i--)
    {
        if (totalclock > (lastsectupdate[i] + TICRATE))
        {
            l = crc32once((uint8_t *)&sector[i], sizeof(sectortype));

            if (sectcrc[i] != l)
            {
                sectcrc[i] = l;
                lastsectupdate[i] = totalclock;
                j += Net_PackSect(i, (uint8_t *)&packbuf[j]);
                k++;

                if (j >= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)))
                    break;
            }
        }
    }
    *(uint16_t *)&packbuf[zj] = k;
    k = 0;

    *(uint16_t *)&packbuf[(zj = j)] = 0;
    j += sizeof(uint16_t);

    for (i = numwalls-1; i >= 0 && j <= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)); i--)
    {
        if (totalclock > (lastwallupdate[i] + TICRATE))
        {
            l = crc32once((uint8_t *)&wall[i], sizeof(walltype));

            if (wallcrc[i] != l)
            {
                wallcrc[i] = l;
                lastwallupdate[i] = totalclock;
                j += Net_PackWall(i, (uint8_t *)&packbuf[j]);
                k++;

                if (j >= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)))
                    break;
            }
        }
    }
    *(uint16_t *)&packbuf[zj] = k;

    {
        char buf[PACKBUF_SIZE];

        if (j >= PACKBUF_SIZE)
            initprintf("Global packet buffer overflow! Size of packet: %i\n", j);

        j = qlz_compress((char *)(packbuf)+1, (char *)buf, j, state_compress);
        Bmemcpy((char *)(packbuf)+1, (char *)buf, j);
        j++;
    }

    packbuf[j++] = myconnectindex;

    enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, j, ENET_PACKET_FLAG_RELIABLE));
}

void faketimerhandler(void)
{
    if (((uintptr_t)g_netServer|(uintptr_t)g_netClient) == (uintptr_t)NULL) return;

    enet_host_service(g_netServer ? g_netServer : g_netClient, NULL, 0);
}

void Net_EnterMessage(void)
{
    int16_t hitstate, i, j, l;

    if (g_player[myconnectindex].ps->gm&MODE_SENDTOWHOM)
    {
        if (g_chatPlayer != -1 || ud.multimode < 3)
        {
            tempbuf[0] = PACKET_MESSAGE;
            tempbuf[2] = 0;
            recbuf[0]  = 0;

            if (ud.multimode < 3)
                g_chatPlayer = 2;

            if (typebuf[0] == '/' && Btoupper(typebuf[1]) == 'M' && Btoupper(typebuf[2]) == 'E')
            {
                Bstrcat(recbuf,"* ");
                i = 3, j = Bstrlen(typebuf);
                Bstrcpy(tempbuf,typebuf);
                while (i < j)
                {
                    typebuf[i-3] = tempbuf[i];
                    i++;
                }
                typebuf[i-3] = '\0';
                Bstrcat(recbuf,g_player[myconnectindex].user_name);
            }
            else
            {
                Bstrcat(recbuf,g_player[myconnectindex].user_name);
                Bstrcat(recbuf,": ");
            }

            Bstrcat(recbuf,"^00");
            Bstrcat(recbuf,typebuf);
            j = Bstrlen(recbuf);
            recbuf[j] = 0;
            Bstrcat(tempbuf+2,recbuf);

            if (g_chatPlayer >= ud.multimode)
            {
                tempbuf[1] = 255;
                tempbuf[j+2] = myconnectindex;
                j++;
                if (g_netServer) enet_host_broadcast(g_netServer, CHAN_CHAT, enet_packet_create(tempbuf, j+2, 0));
                else if (g_netClient) enet_peer_send(g_netClientPeer, CHAN_CHAT, enet_packet_create(tempbuf, j+2, 0));
                G_AddUserQuote(recbuf);
                quotebot += 8;
                l = G_GameTextLen(USERQUOTE_LEFTOFFSET,OSD_StripColors(tempbuf,recbuf));
                while (l > (ud.config.ScreenWidth - USERQUOTE_RIGHTOFFSET))
                {
                    l -= (ud.config.ScreenWidth - USERQUOTE_RIGHTOFFSET);
                    quotebot += 8;
                }
                quotebotgoal = quotebot;
            }
            g_chatPlayer = -1;
            g_player[myconnectindex].ps->gm &= ~(MODE_TYPE|MODE_SENDTOWHOM);
        }
        else if (g_chatPlayer == -1)
        {
            j = 50;
            gametext(320>>1,j,"SEND MESSAGE TO...",0,2+8+16);
            j += 8;
            for (TRAVERSE_CONNECT(i))
            {
                if (i == myconnectindex)
                {
                    minitextshade((320>>1)-40+1,j+1,"A/ENTER - ALL",26,0,2+8+16);
                    minitext((320>>1)-40,j,"A/ENTER - ALL",0,2+8+16);
                    j += 7;
                }
                else
                {
                    Bsprintf(buf,"      %d - %s",i+1,g_player[i].user_name);
                    minitextshade((320>>1)-40-6+1,j+1,buf,26,0,2+8+16);
                    minitext((320>>1)-40-6,j,buf,0,2+8+16);
                    j += 7;
                }
            }
            minitextshade((320>>1)-40-4+1,j+1,"    ESC - Abort",26,0,2+8+16);
            minitext((320>>1)-40-4,j,"    ESC - Abort",0,2+8+16);
            j += 7;

            if (ud.screen_size > 0) j = 200-45;
            else j = 200-8;
            mpgametext(j,typebuf,0,2+8+16);

            if (KB_KeyWaiting())
            {
                i = KB_GetCh();

                if (i == 'A' || i == 'a' || i == 13)
                    g_chatPlayer = ud.multimode;
                else if (i >= '1' || i <= (ud.multimode + '1'))
                    g_chatPlayer = i - '1';
                else
                {
                    g_chatPlayer = ud.multimode;
                    if (i == 27)
                    {
                        g_player[myconnectindex].ps->gm &= ~(MODE_TYPE|MODE_SENDTOWHOM);
                        g_chatPlayer = -1;
                    }
                    else
                        typebuf[0] = 0;
                }

                KB_ClearKeyDown(sc_1);
                KB_ClearKeyDown(sc_2);
                KB_ClearKeyDown(sc_3);
                KB_ClearKeyDown(sc_4);
                KB_ClearKeyDown(sc_5);
                KB_ClearKeyDown(sc_6);
                KB_ClearKeyDown(sc_7);
                KB_ClearKeyDown(sc_8);
                KB_ClearKeyDown(sc_A);
                KB_ClearKeyDown(sc_Escape);
                KB_ClearKeyDown(sc_Enter);
            }
        }
    }
    else
    {
        if (ud.screen_size > 1) j = 200-45;
        else j = 200-8;
        if (xdim >= 640 && ydim >= 480)
            j = scale(j,ydim,200);
        hitstate = Net_EnterText(320>>1,j,typebuf,120,1);

        if (hitstate == 1)
        {
            KB_ClearKeyDown(sc_Enter);
            if (Bstrlen(typebuf) == 0)
            {
                g_player[myconnectindex].ps->gm &= ~(MODE_TYPE|MODE_SENDTOWHOM);
                return;
            }
            if (ud.automsg)
            {
                if (SHIFTS_IS_PRESSED) g_chatPlayer = -1;
                else g_chatPlayer = ud.multimode;
            }
            g_player[myconnectindex].ps->gm |= MODE_SENDTOWHOM;
        }
        else if (hitstate == -1)
            g_player[myconnectindex].ps->gm &= ~(MODE_TYPE|MODE_SENDTOWHOM);
        else pub = NUMPAGES;
    }
}

void Net_WaitForServer(void)
{
    int32_t server_ready = g_player[0].pingcnt;

    if (numplayers < 2 || g_netServer) return;

    P_SetGamePalette(g_player[myconnectindex].ps, TITLEPAL, 8+2/*+1*/);

    do
    {
        if (quitevent || keystatus[1]) G_GameExit("");

        rotatesprite_fs(0,0,65536L,0,BETASCREEN,0,0,2+8+16+64);

        rotatesprite_fs(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8);
        rotatesprite_fs(160<<16,(129)<<16,30<<11,0,THREEDEE,0,0,2+8);
        if (PLUTOPAK)   // JBF 20030804
            rotatesprite_fs(160<<16,(151)<<16,30<<11,0,PLUTOPAKSPRITE+1,0,0,2+8);

        gametext(160,170,"WAITING FOR SERVER",14,2);
        nextpage();

        packbuf[0] = PACKET_PLAYER_PING;
        packbuf[1] = myconnectindex;

        if (g_netClientPeer)
            enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(packbuf, 2, ENET_PACKET_FLAG_RELIABLE));

        handleevents();
        Net_GetPackets();

        if (g_player[0].pingcnt > server_ready)
        {
            P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 8+2/*+1*/);
            return;
        }
    }
    while (1);
}

void Net_ResetPrediction(void)
{
    Bmemcpy(&my, &g_player[myconnectindex].ps, sizeof(vec3_t));
    Bmemcpy(&omy, &g_player[myconnectindex].ps, sizeof(vec3_t));
    Bmemset(&myvel, 0, sizeof(vec3_t));

    myang = omyang = g_player[myconnectindex].ps->ang;
    myhoriz = omyhoriz = g_player[myconnectindex].ps->horiz;
    myhorizoff = omyhorizoff = g_player[myconnectindex].ps->horizoff;
    mycursectnum = g_player[myconnectindex].ps->cursectnum;
    myjumpingcounter = g_player[myconnectindex].ps->jumping_counter;
    myjumpingtoggle = g_player[myconnectindex].ps->jumping_toggle;
    myonground = g_player[myconnectindex].ps->on_ground;
    myhardlanding = g_player[myconnectindex].ps->hard_landing;
    myreturntocenter = g_player[myconnectindex].ps->return_to_center;
}

