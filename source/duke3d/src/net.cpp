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

#include "duke3d.h"
#include "game.h"
#include "gamedef.h"
#include "net.h"
#include "premap.h"
#include "savegame.h"
#include "input.h"

#include "enet/enet.h"
#include "lz4.h"
#include "crc32.h"

ENetHost *g_netServer = NULL;
ENetHost *g_netClient = NULL;
ENetPeer *g_netClientPeer = NULL;
enet_uint16 g_netPort = 23513;
int32_t g_netDisconnect = 0;
char g_netPassword[32];
int32_t g_netPlayersWaiting = 0;
#ifndef NETCODE_DISABLE
int32_t g_networkMode = NET_CLIENT;
#endif
int32_t g_netIndex = 2;
newgame_t pendingnewgame;

#ifdef NETCODE_DISABLE
void faketimerhandler(void) {}
#else

static char recbuf[180];
static int32_t g_chatPlayer = -1;

// sprites of these statnums are synced to clients by the server
int16_t g_netStatnums[] =
{
    STAT_PROJECTILE,
    //STAT_PLAYER,
    STAT_STANDABLE,
    //STAT_ACTIVATOR,
    //STAT_TRANSPORT,
    //STAT_EFFECTOR,
    STAT_ACTOR,
    STAT_ZOMBIEACTOR,
    STAT_MISC,
    STAT_DEFAULT,
    STAT_NETALLOC,
    MAXSTATUS
};

#pragma pack(push,1)
uint32_t g_netMapRevision = 0;
netmapstate_t g_mapStartState;
netmapstate_t *g_mapStateHistory[NET_REVISIONS];
uint8_t tempnetbuf[sizeof(netmapstate_t) + 400];
netmapdiff_t tempMapDiff;
#pragma pack(pop)
#define tempnetbufsize sizeof(tempnetbuf)

static void P_RemovePlayer(int32_t p)
{
    // server obviously can't leave the game, and index 0 shows up for disconnect events from
    // players that haven't gotten far enough into the connection process to get a player ID

    if (p == 0) return;

    g_player[p].playerquitflag = 0;

    Bsprintf(recbuf,"%s^00 is history!",g_player[p].user_name);
    G_AddUserQuote(recbuf);

    if (numplayers == 1)
        S_PlaySound(GENERIC_AMBIENCE17);

    if (g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        if (screenpeek == p)
            screenpeek = myconnectindex;

        pub = NUMPAGES;
        pus = NUMPAGES;
        G_UpdateScreenArea();

        P_QuickKill(g_player[p].ps);

        if (voting == p)
        {
            for (p=0; p<MAXPLAYERS; p++)
            {
                g_player[p].vote = 0;
                g_player[p].gotvote = 0;
            }
            voting = -1;
        }

        Bstrcpy(apStrings[QUOTE_RESERVED2],recbuf);
        g_player[myconnectindex].ps->ftq = QUOTE_RESERVED2;
        g_player[myconnectindex].ps->fta = 180;
    }
}

// sync a connecting player up with the current game state
void Net_SyncPlayer(ENetEvent *event)
{
    int32_t i, j;

    if (numplayers + g_netPlayersWaiting >= MAXPLAYERS)
    {
        enet_peer_disconnect_later(event->peer, DISC_SERVER_FULL);
        initprintf("Refused peer; server full.\n");
        return;
    }

    g_netPlayersWaiting++;

    S_PlaySound(DUKE_GETWEAPON2);

    // open a new slot if necessary and save off the resulting slot # for future reference
    for (TRAVERSE_CONNECT(i))
    {
        if (g_player[i].playerquitflag == 0)
        {
            break;
        }
    }

    if (i == -1)
    {
        i = g_mostConcurrentPlayers++;
    }

    event->peer->data = (void *)(intptr_t)i;

    g_player[i].netsynctime = totalclock;
    g_player[i].playerquitflag = 1;
    //g_player[i].revision = g_netMapRevision;

    for (j=0; j<g_mostConcurrentPlayers-1; j++)
    {
        connectpoint2[j] = j+1;
    }

    connectpoint2[g_mostConcurrentPlayers-1] = -1;

    G_MaybeAllocPlayer(i);

    g_netPlayersWaiting--;
    ++numplayers;
    ++ud.multimode;
    Net_SendNewPlayer(i);
    Net_SendPlayerIndex(i, event->peer);
    Net_SendClientInfo();
    Net_SendUserMapName();
    Net_SendNewGame(0, event->peer);
}

void Net_SpawnPlayer(int32_t player)
{
    int32_t j = 0;
    packbuf[j++] = PACKET_PLAYER_SPAWN;
    packbuf[j++] = player;

    Bmemcpy(&packbuf[j], &g_player[player].ps->pos.x, sizeof(vec3_t) * 2);
    j += sizeof(vec3_t) * 2;

    packbuf[j++] = 0;

    enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, j, ENET_PACKET_FLAG_RELIABLE));
}

static void display_betascreen(void)
{
    rotatesprite_fs(160<<16,100<<16,65536,0,BETASCREEN,0,0,2+8+64+BGSTRETCH);

    rotatesprite_fs(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8);
    rotatesprite_fs(160<<16,(129)<<16,30<<11,0,THREEDEE,0,0,2+8);
    if (PLUTOPAK)   // JBF 20030804
        rotatesprite_fs(160<<16,(151)<<16,30<<11,0,PLUTOPAKSPRITE+1,0,0,2+8);
}

void faketimerhandler(void)
{
    if (g_netServer==NULL && g_netClient==NULL)
        return;

    enet_host_service(g_netServer ? g_netServer : g_netClient, NULL, 0);
}

void Net_WaitForServer(void)
{
    int32_t server_ready = g_player[0].pingcnt;

    if (numplayers < 2 || g_netServer) return;

    P_SetGamePalette(g_player[myconnectindex].ps, TITLEPAL, 8+2+1);

    do
    {
        if (quitevent || keystatus[1]) G_GameExit("");

        if (G_FPSLimit())
        {
            display_betascreen();
            gametext_center_shade(170, "Waiting for server", 14);
        }

        // XXX: this looks like something that should be rate limited...
        packbuf[0] = PACKET_PLAYER_PING;
        packbuf[1] = myconnectindex;

        if (g_netClientPeer)
            enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(packbuf, 2, ENET_PACKET_FLAG_RELIABLE));

        G_HandleAsync();

        if (g_player[0].pingcnt > server_ready)
        {
            P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 8+2+1);
            return;
        }
    }
    while (1);
}

void Net_ResetPrediction(void)
{
}

////////////////////////////////////////////////////////////////////////////////
// Connect/Disconnect

void Net_Connect(const char *srvaddr)
{
    ENetAddress address;
    ENetEvent event;
    char *addrstr = NULL;
    int32_t i;

    char *oursrvaddr = Xstrdup(srvaddr);

    Net_Disconnect();

    g_netClient = enet_host_create(NULL, 1, CHAN_MAX, 0, 0);

    if (g_netClient == NULL)
    {
        initprintf("An error occurred while trying to create an ENet client host.\n");
        return;
    }

    addrstr = strtok(oursrvaddr, ":");
    enet_address_set_host(&address, addrstr);
    addrstr = strtok(NULL, ":");
    address.port = addrstr==NULL ? g_netPort : Batoi(addrstr);

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
            initprintf("Connection to %s:%d succeeded.\n", oursrvaddr, address.port);
            Bfree(oursrvaddr);
            return;
        }
        else
        {
            /* Either the 5 seconds are up or a disconnect event was */
            /* received. Reset the peer in the event the 5 seconds   */
            /* had run out without any significant event.            */
            enet_peer_reset(g_netClientPeer);
            initprintf("Connection to %s:%d failed.\n", oursrvaddr, address.port);
        }
        initprintf(i ? "Retrying...\n" : "Giving up connection attempt.\n");
    }

    Bfree(oursrvaddr);
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
                numplayers = g_mostConcurrentPlayers = ud.multimode = 1;
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
        int32_t i;
        ENetEvent event;

        for (i=0; i<(signed)g_netServer->peerCount; i++)
            enet_peer_disconnect_later(&g_netServer->peers[i], DISC_SERVER_QUIT);

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

void Net_ReceiveDisconnect(ENetEvent *event)
{
    g_netDisconnect = 1;
    numplayers = g_mostConcurrentPlayers = ud.multimode = 1;
    myconnectindex = screenpeek = 0;
    G_BackToMenu();

    switch (event->data)
    {
    case DISC_BAD_PASSWORD:
        initprintf("Bad password.\n");
        return;
    case DISC_VERSION_MISMATCH:
        initprintf("Version mismatch.\n");
        return;
    case DISC_INVALID:
        initprintf("Invalid data detected.\n");
        return;
    case DISC_SERVER_QUIT:
        initprintf("The server is quitting.\n");
        return;
    case DISC_SERVER_FULL:
        initprintf("The server is full.\n");
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
}

////////////////////////////////////////////////////////////////////////////////
// Packet Handlers

#endif

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
        Net_HandleClientPackets();
    }
    else if (g_netClient)
    {
        Net_HandleServerPackets();
    }
}

#ifndef NETCODE_DISABLE

void Net_HandleClientPackets(void)
{
    ENetEvent event;

    // pull events from the wire into the packet queue without dispatching them, once per Net_GetPackets() call
    enet_host_service(g_netServer, NULL, 0);

    // dispatch any pending events from the local packet queue
    while (enet_host_check_events(g_netServer, &event) > 0)
    {
        const intptr_t playeridx = (intptr_t)event.peer->data;

        if (playeridx < 0 || playeridx >= MAXPLAYERS)
        {
            enet_peer_disconnect_later(event.peer, DISC_INVALID);
            buildprint("Invalid player id (", playeridx, ") from client.\n");
            continue;
        }

        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
        {
            char ipaddr[32];

            enet_address_get_host_ip(&event.peer->address, ipaddr, sizeof(ipaddr));

            initprintf("A new client connected from %s:%u.\n", ipaddr, event.peer->address.port);

            Net_SendAcknowledge(event.peer);
            break;
        }

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
            // we set the state to disconnected so enet_host_broadcast
            // doesn't send the player back his own packets
            if ((event.channelID == CHAN_GAMESTATE && event.packet->data[0] > PACKET_BROADCAST)
                    || event.channelID == CHAN_CHAT)
            {
                const ENetPacket *pak = event.packet;

                event.peer->state = ENET_PEER_STATE_DISCONNECTED;
                enet_host_broadcast(g_netServer, event.channelID,
                                    enet_packet_create(pak->data, pak->dataLength, pak->flags&ENET_PACKET_FLAG_RELIABLE));
                event.peer->state = ENET_PEER_STATE_CONNECTED;
            }

            enet_packet_destroy(event.packet);
            g_player[playeridx].ping = (event.peer->lastRoundTripTime + event.peer->roundTripTime)/2;
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            numplayers--;
            ud.multimode--;

            P_RemovePlayer(playeridx);

            packbuf[0] = PACKET_PLAYER_DISCONNECTED;
            packbuf[1] = playeridx;
            packbuf[2] = numplayers;
            packbuf[3] = ud.multimode;
            packbuf[4] = g_mostConcurrentPlayers;
            packbuf[5] = myconnectindex;

            enet_host_broadcast(g_netServer, CHAN_GAMESTATE,
                                enet_packet_create(packbuf, 6, ENET_PACKET_FLAG_RELIABLE));

            initprintf("%s disconnected.\n", g_player[playeridx].user_name);
            event.peer->data = NULL;
            break;

        default:
            break;
        }
    }
}

void Net_HandleServerPackets(void)
{
    ENetEvent event;

    enet_host_service(g_netClient, NULL, 0);

    while (enet_host_check_events(g_netClient, &event) > 0)
    {
        if (event.type == ENET_EVENT_TYPE_DISCONNECT)
        {
            Net_ReceiveDisconnect(&event);
        }
        else if (event.type == ENET_EVENT_TYPE_RECEIVE)
        {
            Net_ParseServerPacket(&event);
        }

        enet_packet_destroy(event.packet);
    }
}

void Net_ParseClientPacket(ENetEvent *event)
{
    uint8_t *pbuf = event->packet->data;
    int32_t packbufleng = event->packet->dataLength;
    int16_t j;
    int32_t other = pbuf[--packbufleng];

#if 0
    initprintf("Received Packet: type: %d : len %d\n", pbuf[0], packbufleng);
#endif
    switch (pbuf[0])
    {
    case PACKET_SLAVE_TO_MASTER:  //[1] (receive slave sync buffer)
        Net_ReceiveClientUpdate(event);
        break;

    case PACKET_PLAYER_READY:

        if (other == 0)
        {
            break;
        }

        j = g_player[other].ps->i;
        Bmemcpy(g_player[other].ps, g_player[0].ps, sizeof(DukePlayer_t));

        g_player[other].ps->i = j;
        changespritestat(j, STAT_PLAYER);

        g_player[other].ps->last_extra = sprite[g_player[other].ps->i].extra = g_player[other].ps->max_player_health;
        sprite[g_player[other].ps->i].cstat = 1+256;
        actor[g_player[other].ps->i].t_data[2] = actor[g_player[other].ps->i].t_data[3] = actor[g_player[other].ps->i].t_data[4] = 0;

        P_ResetPlayer(other);
        Net_SpawnPlayer(other);

        break;

    case PACKET_PLAYER_PING:
        if (g_player[myconnectindex].ps->gm & MODE_GAME)
        {
            packbuf[0] = PACKET_PLAYER_PING;
            packbuf[1] = myconnectindex;
            enet_peer_send(event->peer, CHAN_GAMESTATE, enet_packet_create(packbuf, 2, ENET_PACKET_FLAG_RELIABLE));
        }
        g_player[other].pingcnt++;
        break;

    case PACKET_AUTH:
        Net_ReceiveChallenge(pbuf, packbufleng, event);
        break;

    default:
        Net_ParsePacketCommon(pbuf, packbufleng, 0);
        break;
    }
}

void Net_ParseServerPacket(ENetEvent *event)
{
    uint8_t *pbuf = event->packet->data;
    int32_t packbufleng = event->packet->dataLength;
   // input_t *nsyn;

    --packbufleng;  //    int32_t other = pbuf[--packbufleng];

#if 0
    initprintf("Received Packet: type: %d : len %d\n", pbuf[0], packbufleng);
#endif
    switch (pbuf[0])
    {
    case PACKET_MASTER_TO_SLAVE:

        if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
        {
            return;
        }

        Net_ReceiveServerUpdate(event);
        break;

    case PACKET_MAP_STREAM:

        if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
            return;

        Net_ReceiveMapUpdate(event);

        break;

    case PACKET_NEW_GAME:
        Net_ReceiveNewGame(event);
        break;

    case PACKET_ACK:
        Net_ReceiveAcknowledge(pbuf, packbufleng);
        break;

    case PACKET_NUM_PLAYERS:
        Net_ReceiveNewPlayer(event->packet->data, event->packet->dataLength);
        break;

    case PACKET_PLAYER_INDEX:
        Net_ReceivePlayerIndex(event->packet->data, event->packet->dataLength);
        break;

    case PACKET_PLAYER_DISCONNECTED:
        if ((g_player[myconnectindex].ps->gm & MODE_GAME))
            P_RemovePlayer(pbuf[1]);
        numplayers = pbuf[2];
        ud.multimode = pbuf[3];
        g_mostConcurrentPlayers = pbuf[4];
        break;

    case PACKET_PLAYER_SPAWN:
        if (!(g_player[myconnectindex].ps->gm & MODE_GAME)) break;

        P_ResetPlayer(pbuf[1]);
        Bmemcpy(&g_player[pbuf[1]].ps->pos.x, &pbuf[2], sizeof(vec3_t) * 2);
        Bmemcpy(&sprite[g_player[pbuf[1]].ps->i], &pbuf[2], sizeof(vec3_t));
        break;

    case PACKET_PLAYER_PING:
        g_player[0].pingcnt++;
        return;

    case PACKET_FRAG:
        if (!(g_player[myconnectindex].ps->gm & MODE_GAME)) break;
        g_player[pbuf[1]].ps->frag_ps = pbuf[2];
        actor[g_player[pbuf[1]].ps->i].picnum = pbuf[3];
        ticrandomseed = B_UNBUF32(&pbuf[4]);
        P_FragPlayer(pbuf[1]);
        break;

    default:
        Net_ParsePacketCommon(pbuf, packbufleng, 1);
        break;
    }
}

void Net_ParsePacketCommon(uint8_t *pbuf, int32_t packbufleng, int32_t serverpacketp)
{
    switch (pbuf[0])
    {
    case PACKET_MESSAGE:
        Net_ReceiveMessage(pbuf, packbufleng);
        break;

    case PACKET_CLIENT_INFO:
        Net_ReceiveClientInfo(pbuf, packbufleng, serverpacketp);
        break;

    case PACKET_RTS:
        G_StartRTS(pbuf[1], 0);
        break;

    case PACKET_USER_MAP:
        Net_ReceiveUserMapName(pbuf, packbufleng);
        break;

    case PACKET_MAP_VOTE:
        Net_ReceiveMapVote(pbuf);
        break;

    case PACKET_MAP_VOTE_INITIATE: // call map vote
        Net_ReceiveMapVoteInitiate(pbuf);
        break;

    case PACKET_MAP_VOTE_CANCEL: // cancel map vote
        Net_ReceiveMapVoteCancel(pbuf);
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Acknowledgement Packets

void Net_SendAcknowledge(ENetPeer *client)
{
    if (!g_netServer)
        return;

    tempnetbuf[0] = PACKET_ACK;
    tempnetbuf[1] = myconnectindex;

    enet_peer_send(client, CHAN_GAMESTATE, enet_packet_create(&tempnetbuf[0], 2, ENET_PACKET_FLAG_RELIABLE));
}

void Net_ReceiveAcknowledge(uint8_t *pbuf, int32_t packbufleng)
{
    UNREFERENCED_PARAMETER(pbuf); // remove when this variable is used
    UNREFERENCED_PARAMETER(packbufleng); // remove when this variable is used

    Net_SendChallenge();
}

////////////////////////////////////////////////////////////////////////////////
// Challenge Packets

// sends the version and a simple crc32 of the current password, all verified by the server before the connection can continue
void Net_SendChallenge()
{
    if (!g_netClientPeer)
    {
        return;
    }

    tempnetbuf[0] = PACKET_AUTH;
    B_BUF16(&tempnetbuf[1], BYTEVERSION);
    B_BUF16(&tempnetbuf[3], NETVERSION);
    B_BUF32(&tempnetbuf[5], Bcrc32((uint8_t *)g_netPassword, Bstrlen(g_netPassword), 0));
    tempnetbuf[9] = myconnectindex;

    enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&tempnetbuf[0], 10, ENET_PACKET_FLAG_RELIABLE));
}

void Net_ReceiveChallenge(uint8_t *pbuf, int32_t packbufleng, ENetEvent *event)
{
    const uint16_t byteVersion = B_UNBUF16(&pbuf[1]);
    const uint16_t netVersion = B_UNBUF16(&pbuf[3]);
    const uint32_t crc = B_UNBUF32(&pbuf[5]);

    UNREFERENCED_PARAMETER(packbufleng); // remove when this variable is used

    if (byteVersion != BYTEVERSION || netVersion != NETVERSION)
    {
        enet_peer_disconnect_later(event->peer, DISC_VERSION_MISMATCH);
        initprintf("Bad client protocol: version %u.%u\n", byteVersion, netVersion);
        return;
    }
    if (crc != Bcrc32((uint8_t *)g_netPassword, Bstrlen(g_netPassword), 0))
    {
        enet_peer_disconnect_later(event->peer, DISC_BAD_PASSWORD);
        initprintf("Bad password from client.\n");
        return;
    }

    Net_SyncPlayer(event);
}

////////////////////////////////////////////////////////////////////////////////
// Num Players Packets

void Net_SendNewPlayer(int32_t newplayerindex)
{
    packbuf[0] = PACKET_NUM_PLAYERS;
    packbuf[1] = numplayers;
    packbuf[2] = g_mostConcurrentPlayers;
    packbuf[3] = ud.multimode;
    packbuf[4] = newplayerindex;
    packbuf[5] = g_networkMode;
    packbuf[6] = myconnectindex;
    enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, 7, ENET_PACKET_FLAG_RELIABLE));
}

void Net_ReceiveNewPlayer(uint8_t *pbuf, int32_t packbufleng)
{
    int32_t i;

    UNREFERENCED_PARAMETER(packbufleng); // remove when this variable is used

    numplayers = pbuf[1];
    g_mostConcurrentPlayers = pbuf[2];
    ud.multimode = pbuf[3];
    if (pbuf[4]) // ID of new player
    {
        g_player[pbuf[4]].playerquitflag = 1;

        if (!g_player[pbuf[4]].ps)
        {
            g_player[pbuf[4]].ps = (DukePlayer_t *) Xcalloc(1,sizeof(DukePlayer_t));
        }
        if (!g_player[pbuf[4]].inputBits)
        {
            g_player[pbuf[4]].inputBits = (input_t *) Xcalloc(1,sizeof(input_t));
        }
    }

#ifndef NETCODE_DISABLE
    if (pbuf[5] == NET_DEDICATED_SERVER)
    {
        g_networkMode = NET_DEDICATED_CLIENT;
    }
#endif

    for (i=0; i<g_mostConcurrentPlayers-1; i++)
    {
        connectpoint2[i] = i+1;
    }

    connectpoint2[g_mostConcurrentPlayers-1] = -1;

    S_PlaySound(DUKE_GETWEAPON2);

    // myconnectindex is 0 until we get PACKET_PLAYER_INDEX
    if (myconnectindex != 0)
    {
        Net_SendClientInfo();
    }
}

////////////////////////////////////////////////////////////////////////////////
// Player Index Packets

void Net_SendPlayerIndex(int32_t index, ENetPeer *peer)
{
    packbuf[0] = PACKET_PLAYER_INDEX;
    packbuf[1] = index;
    packbuf[2] = myconnectindex;
    enet_peer_send(peer, CHAN_GAMESTATE, enet_packet_create(packbuf, 3, ENET_PACKET_FLAG_RELIABLE));
}

void Net_ReceivePlayerIndex(uint8_t *pbuf, int32_t packbufleng)
{
    UNREFERENCED_PARAMETER(packbufleng); // remove when this variable is used

    myconnectindex = pbuf[1];
    g_player[myconnectindex].playerquitflag = 1;
    Net_SendClientInfo();
}

////////////////////////////////////////////////////////////////////////////////
// Client Info Packets

void Net_SendClientInfo(void)
{
    int32_t i,l;

    for (l=0; (unsigned)l<sizeof(szPlayerName)-1; l++)
        g_player[myconnectindex].user_name[l] = szPlayerName[l];

    if (numplayers < 2) return;

    tempnetbuf[0] = PACKET_CLIENT_INFO;
    l = 1;

    //null terminated player name to send
    for (i=0; szPlayerName[i]; i++)
    {
        tempnetbuf[l++] = szPlayerName[i];
    }
    tempnetbuf[l++] = 0;

    tempnetbuf[l++] = g_player[myconnectindex].ps->aim_mode = ud.mouseaiming;
    tempnetbuf[l++] = g_player[myconnectindex].ps->auto_aim = ud.config.AutoAim;
    tempnetbuf[l++] = g_player[myconnectindex].ps->weaponswitch = ud.weaponswitch;
    tempnetbuf[l++] = g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = ud.color;

    tempnetbuf[l++] = g_player[myconnectindex].pteam = ud.team;

    for (i=0; i<10; i++)
    {
        g_player[myconnectindex].wchoice[i] = g_player[0].wchoice[i];
        tempnetbuf[l++] = (uint8_t)g_player[0].wchoice[i];
    }

    tempnetbuf[l++] = myconnectindex;

    if (g_netClient)
    {
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&tempnetbuf[0], l, ENET_PACKET_FLAG_RELIABLE));
    }
    else if (g_netServer)
    {
        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(&tempnetbuf[0], l, ENET_PACKET_FLAG_RELIABLE));
    }
}

void Net_ReceiveClientInfo(uint8_t *pbuf, int32_t packbufleng, int32_t fromserver)
{
    uint32_t i, j;
    int32_t other = pbuf[packbufleng];

    for (i=1; pbuf[i]; i++)
    {
        g_player[other].user_name[i-1] = pbuf[i];
    }

    g_player[other].user_name[i-1] = 0;
    i++;

    g_player[other].ps->aim_mode = pbuf[i++];
    g_player[other].ps->auto_aim = pbuf[i++];
    g_player[other].ps->weaponswitch = pbuf[i++];
    g_player[other].ps->palookup = g_player[other].pcolor = pbuf[i++];
    g_player[other].pteam = pbuf[i++];

    for (j=i; i-j<10; i++)
    {
        g_player[other].wchoice[i-j] = pbuf[i];
    }

    if (fromserver)
    {
        g_player[other].playerquitflag = 1;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Map Name Packets

void Net_SendUserMapName(void)
{
    int32_t j;

    if (numplayers < 2)
        return;

    packbuf[0] = PACKET_USER_MAP;

    Bcorrectfilename(boardfilename,0);

    // user map name is sent with a NUL at the end
    j = Bstrlen(boardfilename)+1;
    Bmemcpy(&packbuf[1], boardfilename, j);
    j++;

    packbuf[j++] = myconnectindex;

    if (g_netClient)
    {
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(packbuf, j, ENET_PACKET_FLAG_RELIABLE));
    }
    else if (g_netServer)
    {
        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, j, ENET_PACKET_FLAG_RELIABLE));
    }
}

void Net_ReceiveUserMapName(uint8_t *pbuf, int32_t packbufleng)
{
    int32_t i;

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
}

////////////////////////////////////////////////////////////////////////////////
// Map Update Packets

netmapstate_t *Net_GetRevision(uint8_t revision, uint8_t cancreate)
{
    assert(revision < NET_REVISIONS);

    if (revision == 0)
    {
        return &g_mapStartState;
    }

    if (cancreate && g_mapStateHistory[revision] == NULL)
    {
        g_mapStateHistory[revision] = (netmapstate_t *) Xcalloc(1, sizeof(netmapstate_t));
        Bmemset(g_mapStateHistory[revision], 0, sizeof(netmapstate_t));
    }

    return g_mapStateHistory[revision];
}

void Net_SendMapUpdate(void)
{
    int32_t pi;
   // uint32_t numdiff = 0;
    uint32_t diffsize = 0;
    uint32_t packetsize = 0;
   // uint32_t prevMapDiff = g_netMapRevision;

    if (!g_netServer || numplayers < 2)
    {
        return;
    }

    g_netMapRevision++;
    if (g_netMapRevision >= NET_REVISIONS)
    {
        g_netMapRevision = 1;
    }

    Net_SaveMapState(Net_GetRevision(g_netMapRevision, 1));

    // I use this to test server diffs without requiring a client.
    //Net_FillMapDiff(prevMapDiff, g_netMapRevision);

    for (pi = 0; pi < (signed) g_netServer->peerCount; pi++)
    {
        ENetPeer *const currentPeer = &g_netServer->peers[pi];
        const intptr_t playeridx = (intptr_t) currentPeer->data;

        if (playeridx < 0 || playeridx >= MAXPLAYERS)
        {
            continue;
        }

        if (currentPeer->state != ENET_PEER_STATE_CONNECTED || !g_player[playeridx].playerquitflag)
        {
            continue;
        }

        if (g_player[playeridx].revision == g_netMapRevision)
        {
            continue;
        }

        Net_FillMapDiff(g_player[playeridx].revision, g_netMapRevision);

        diffsize = 4 * sizeof(uint32_t);
        diffsize += tempMapDiff.numActors * sizeof(netactor_t);
        diffsize += tempMapDiff.numToDelete * sizeof(int32_t);

        packetsize = LZ4_compress_default((const char*)&tempMapDiff, (char*)&tempnetbuf[5], diffsize, tempnetbufsize - 5);

        if (packetsize == 0)
            return;

        // apply header
        tempnetbuf[0] = PACKET_MAP_STREAM;

        // apply uncompressed size
        B_BUF32(&tempnetbuf[1], diffsize);

        packetsize += 5;

        //initprintf("update packet size: %d - revision (%d->%d) - num actors: %d\n", packetsize, g_player[playeridx].revision, g_netMapRevision, tempMapDiff.numActors);

        enet_peer_send(currentPeer, CHAN_GAMESTATE, enet_packet_create(tempnetbuf, packetsize, ENET_PACKET_FLAG_RELIABLE));
    }
}

void Net_ReceiveMapUpdate(ENetEvent *event)
{
    const uint8_t *pktBuf = (uint8_t *) event->packet->data;
    uint32_t diffsize = B_UNBUF32(&pktBuf[1]);
    LZ4_decompress_safe((const char*)&pktBuf[5], (char*)&tempMapDiff, diffsize, sizeof(netmapdiff_t));

    Net_RestoreMapState();
    //initprintf("Update packet size: %d - num actors: %d\n", event->packet->dataLength, tempMapDiff.numActors);
}

////////////////////////////////////////////////////////////////////////////////
// Map State

static int Net_CompareActors(const void *actor1, const void *actor2)
{
    return ((netactor_t const *)actor1)->netIndex - ((netactor_t const *)actor2)->netIndex;
}

void Net_SaveMapState(netmapstate_t *save)
{
    int32_t i;
    int32_t statIndex;

    if (save == NULL)
    {
        return;
    }

    save->numActors = 0;

    for (statIndex = 0; g_netStatnums[statIndex] != MAXSTATUS; ++statIndex)
    {
        i = headspritestat[g_netStatnums[statIndex]];
        for (; i >= 0; i = nextspritestat[i])
        {
            if (save->numActors >= NETMAXACTORS)
            {
                break;
            }

            if (Net_IsRelevantSprite(i) && sprite[i].statnum != STAT_NETALLOC)
            {
                netactor_t *tempActor = &save->actor[save->numActors];
                Net_CopyToNet(i, tempActor);
                save->numActors++;
            }
        }
    }

    qsort(save->actor, save->numActors, sizeof(netactor_t), &Net_CompareActors);
}

void Net_FillMapDiff(uint32_t fromRevision, uint32_t toRevision)
{
    uint32_t fromIndex = 0;
    uint32_t toIndex = 0;
    netmapstate_t *fromState;
    netmapstate_t *toState;
    int32_t *deleteBuf;
    netactor_t *actorBuf;

    // First check to see if the tempMapDiff is already filled with the diff we want
    if (tempMapDiff.fromRevision == fromRevision && tempMapDiff.toRevision == toRevision)
    {
        return;
    }

    tempMapDiff.fromRevision = fromRevision;
    tempMapDiff.toRevision = toRevision;
    tempMapDiff.numActors = 0;
    tempMapDiff.numToDelete = 0;

    actorBuf = (netactor_t *) tempMapDiff.data;
    deleteBuf = (int32_t *) tempnetbuf;

    fromState = Net_GetRevision(fromRevision, 0);
    toState = Net_GetRevision(toRevision, 0);

    assert(fromState != NULL);
    assert(toState != NULL);

    while (fromIndex < fromState->numActors || toIndex < toState->numActors)
    {
        const int32_t fromNet = fromState->actor[fromIndex].netIndex;
        const int32_t toNet = toState->actor[toIndex].netIndex;
        const int32_t fromValid = fromIndex < fromState->numActors;
        const int32_t toValid = toIndex < toState->numActors;
        if (toValid && (!fromValid || fromNet > toNet))
        {
            //initprintf("This actor is new: %d - %d\n", toState->actor[toIndex].netIndex, toState->actor[toIndex].sprite.picnum);

            // Add the "to" data. It's a new actor.
            memcpy(&actorBuf[tempMapDiff.numActors], &toState->actor[toIndex], sizeof(netactor_t));
            tempMapDiff.numActors++;
            toIndex++;
        }
        else if (fromValid && (!toValid || toNet > fromNet))
        {
            //initprintf("This actor is deleted: %d - %d\n", fromState->actor[fromIndex].netIndex, fromState->actor[fromIndex].sprite.picnum);

            // Add the "fromNet" to the list of deleted actors
            deleteBuf[tempMapDiff.numToDelete] = fromState->actor[fromIndex].netIndex;
            tempMapDiff.numToDelete++;
            fromIndex++;
        }
        else //if (fromNet == toNet)
        {
            assert(fromNet == toNet);

            if (Net_ActorsAreDifferent(&fromState->actor[fromIndex], &toState->actor[toIndex]))
            {
                //initprintf("This actor is different: %d - %d\n", toState->actor[toIndex].netIndex, toState->actor[toIndex].sprite.picnum);

                // Add the "to" data. It's changed.
                memcpy(&actorBuf[tempMapDiff.numActors], &toState->actor[toIndex], sizeof(netactor_t));
                tempMapDiff.numActors++;
            }

            fromIndex++;
            toIndex++;
        }
    }

    if (tempMapDiff.numToDelete > 0)
    {
        memcpy(&actorBuf[tempMapDiff.numActors], deleteBuf, tempMapDiff.numToDelete * sizeof(int32_t));
    }
}

void Net_RestoreMapState()
{
    int32_t i;
    uint32_t j;
    int32_t *deleteBuf;
    netactor_t *actorBuf;

    // Make sure we're using a diff from our current revision
    if (tempMapDiff.fromRevision != g_player[myconnectindex].revision)
    {
        return;
    }

    g_player[myconnectindex].revision = tempMapDiff.toRevision;

    actorBuf = (netactor_t *) tempMapDiff.data;

    for (j = 0; j < tempMapDiff.numActors; ++j)
    {
        i = actorBuf[j].netIndex;
        /*
        if (Net_IsRelevantSprite(i))
        {
            initprintf("This actor is different: %d - %d\n", i, actorBuf[j].sprite.picnum);
        }
        else
        {
            initprintf("This actor is new: %d - %d\n", i, actorBuf[j].sprite.picnum);
        }
        */
        Net_CopyFromNet(i, &actorBuf[j]);
    }

    deleteBuf = (int32_t *) &actorBuf[tempMapDiff.numActors];
    for (j = 0; j < tempMapDiff.numToDelete; ++j)
    {
        //initprintf("This actor is deleted: %d - %d\n", deleteBuf[j], sprite[deleteBuf[j]].picnum);
        A_DeleteSprite(deleteBuf[j]);
    }
}

void Net_CopyToNet(int32_t i, netactor_t *netactor)
{
    netactor->netIndex = i;
    netactor->picnum = actor[i].picnum;
    netactor->ang = actor[i].ang;
    netactor->extra = actor[i].extra;
    netactor->owner = actor[i].owner;
    netactor->movflag = actor[i].movflag;
    netactor->tempang = actor[i].tempang;
    netactor->timetosleep = actor[i].timetosleep;
    netactor->flags = actor[i].flags;
    netactor->floorz = actor[i].floorz;
    netactor->lastv.x = actor[i].lastv.x;
    netactor->lastv.y = actor[i].lastv.y;
    netactor->lasttransport = actor[i].lasttransport;
    netactor->actorstayput = actor[i].actorstayput;
    netactor->cgg = actor[i].cgg;
    netactor->owner = actor[i].owner;

    Bmemcpy(netactor->t_data, actor[i].t_data, 10 * sizeof(int32_t));

    Bmemcpy(&netactor->sprite, &sprite[i], sizeof(spritetype));
}

void Net_CopyFromNet(int32_t i, netactor_t *netactor)
{
    if (netactor->sprite.statnum == STAT_NETALLOC)
    {
        // Do nothing if it's going to be deleted
        return;
    }
    else if (sprite[i].statnum == STAT_NETALLOC)
    {
        changespritestat(i, netactor->sprite.statnum);
        do_insertsprite_at_headofsect(i, netactor->sprite.sectnum);
    }
    else
    {
        // These functions already check to see if the sprite already has the stat/sect value. No need to do it twice.
        changespritestat(i, netactor->sprite.statnum);
        changespritesect(i, netactor->sprite.sectnum);
    }

    actor[i].picnum = netactor->picnum;
    actor[i].ang = netactor->ang;
    actor[i].extra = netactor->extra;
    actor[i].owner = netactor->owner;
    actor[i].movflag = netactor->movflag;
    actor[i].tempang = netactor->tempang;
    actor[i].timetosleep = netactor->timetosleep;
    actor[i].flags = netactor->flags;
    actor[i].floorz = netactor->floorz;
    actor[i].lastv.x = netactor->lastv.x;
    actor[i].lastv.y = netactor->lastv.y;
    actor[i].lasttransport = netactor->lasttransport;
    actor[i].actorstayput = netactor->actorstayput;
    actor[i].cgg = netactor->cgg;
    actor[i].owner = netactor->owner;

    Bmemcpy(actor[i].t_data, netactor->t_data, 10 * sizeof(int32_t));

    Bmemcpy(&sprite[i], &netactor->sprite, sizeof(spritetype));
}

int32_t Net_ActorsAreDifferent(netactor_t *actor1, netactor_t *actor2)
{
    int32_t allDiff;
    int32_t finalDiff;
    int32_t nonStandableDiff;

    allDiff =
        actor1->picnum			!= actor2->picnum ||
        actor1->ang				!= actor2->ang ||
        actor1->extra			!= actor2->extra ||
        actor1->owner			!= actor2->owner ||
        actor1->movflag			!= actor2->movflag ||
        actor1->tempang			!= actor2->tempang ||
        //actor1->timetosleep	!= actor2->timetosleep ||
        actor1->flags			!= actor2->flags ||
        actor1->floorz			!= actor2->floorz ||
        actor1->lastv.x			!= actor2->lastv.x ||
        actor1->lastv.y			!= actor2->lastv.y ||
        actor1->lasttransport	!= actor2->lasttransport ||
        actor1->actorstayput	!= actor2->actorstayput ||
        //actor1->cgg			!= actor2->cgg ||

        actor1->sprite.owner	!= actor2->sprite.owner ||
        actor1->sprite.statnum	!= actor2->sprite.statnum ||
        actor1->sprite.sectnum	!= actor2->sprite.sectnum ||
        actor1->sprite.picnum	!= actor2->sprite.picnum ||
        //actor1->sprite.shade	!= actor2->sprite.shade ||
        actor1->sprite.xrepeat	!= actor2->sprite.xrepeat ||
        actor1->sprite.yrepeat	!= actor2->sprite.yrepeat;// ||
        //actor1->sprite.ang	!= actor2->sprite.ang ||

    nonStandableDiff =
        actor1->sprite.x		!= actor2->sprite.x ||
        actor1->sprite.y		!= actor2->sprite.y ||
        actor1->sprite.z		!= actor2->sprite.z ||
        actor1->sprite.xvel		!= actor2->sprite.xvel ||
        actor1->sprite.yvel		!= actor2->sprite.yvel ||
        actor1->sprite.zvel		!= actor2->sprite.zvel;

    finalDiff = allDiff || (actor1->sprite.statnum != STAT_STANDABLE && nonStandableDiff);

    return finalDiff;
}

int32_t Net_IsRelevantSprite(int32_t i)
{
    if (g_netServer == NULL && g_netClient == NULL)
    {
        return 0;
    }
    else if (i < 0 || i >= MAXSPRITES)
    {
        return 0;
    }
    else
    {
        return Net_IsRelevantStat(sprite[i].statnum);
    }
}

int32_t Net_IsRelevantStat(int32_t stat)
{
    int32_t statIndex;

    if (g_netServer == NULL && g_netClient == NULL)
    {
        return 0;
    }

    for (statIndex = 0; g_netStatnums[statIndex] != MAXSTATUS; ++statIndex)
    {
        if (g_netStatnums[statIndex] == stat)
        {
            return 1;
        }
    }

    return 0;
}

int32_t Net_InsertSprite(int32_t sect, int32_t stat)
{
    int32_t i = headspritestat[STAT_NETALLOC];

    // This means that we've run out of server-side actors
    if (i == -1)
    {
        return -1;
    }

    changespritestat(i, stat);
    do_insertsprite_at_headofsect(i, sect);

    return i;
}

void Net_DeleteSprite(int32_t spritenum)
{
    if (sprite[spritenum].statnum == STAT_NETALLOC)
    {
        return;
    }

    changespritestat(spritenum, STAT_NETALLOC);
    do_deletespritesect(spritenum);
    sprite[spritenum].sectnum = MAXSECTORS;
}

extern void Gv_RefreshPointers(void);

////////////////////////////////////////////////////////////////////////////////
// Player Updates

void Net_FillPlayerUpdate(playerupdate_t *update, int32_t player)
{
    update->playerindex = player;

    update->pos = g_player[player].ps->pos;
    update->opos = g_player[player].ps->opos;
    update->vel = g_player[player].ps->vel;
    update->ang = g_player[player].ps->ang;
    update->horiz = g_player[player].ps->horiz;
    update->horizoff = g_player[player].ps->horizoff;
    update->ping = g_player[player].ping;
    update->deadflag = g_player[player].ps->dead_flag;
    update->playerquitflag = g_player[player].playerquitflag;
}

void Net_ExtractPlayerUpdate(playerupdate_t *update, int32_t type)
{
    const int32_t playerindex = update->playerindex;

    if (playerindex != myconnectindex)
    {
        g_player[playerindex].ps->pos = update->pos;
        g_player[playerindex].ps->opos = update->opos;
        g_player[playerindex].ps->vel = update->vel;
        g_player[playerindex].ps->ang = update->ang;
        g_player[playerindex].ps->horiz = update->horiz;
        g_player[playerindex].ps->horizoff = update->horizoff;
    }

    if (type == PACKET_MASTER_TO_SLAVE)
    {
        g_player[playerindex].ping = update->ping;
        g_player[playerindex].ps->dead_flag = update->deadflag;
        g_player[playerindex].playerquitflag = update->playerquitflag;
    }

    //updatesectorz(g_player[other].ps->pos.x, g_player[other].ps->pos.y, g_player[other].ps->pos.z, &g_player[other].ps->cursectnum);
    //changespritesect(g_player[other].ps->i, g_player[other].ps->cursectnum);
}

////////////////////////////////////////////////////////////////////////////////
// Server Update Packets

#pragma pack(push,1)
typedef struct
{
    uint8_t header;
    uint8_t numplayers;
    input_t nsyn;
    int32_t seed;
    int16_t pause_on;
} serverupdate_t;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct
{
    uint8_t extra;
    int16_t cstat;
    uint16_t owner;
    uint16_t picnum;
    uint16_t gotweapon;
    uint8_t kickback_pic;
    uint8_t frags[MAXPLAYERS];
    int16_t inv_amount[GET_MAX];
    int16_t ammo_amount[MAX_WEAPONS];

    uint8_t curr_weapon;
    uint8_t last_weapon;
    uint8_t wantweaponfire;
    uint8_t weapon_pos;
    uint8_t frag_ps;
    uint8_t frag;
    uint8_t fraggedself;
    uint8_t last_extra;
    uint8_t pal;
    uint16_t ping;
    uint16_t newowner;

    playerupdate_t player;
} serverplayerupdate_t;
#pragma pack(pop)

void Net_SendServerUpdates(void)
{
    int16_t i;
    uint8_t *updatebuf;
    serverupdate_t serverupdate;
    serverplayerupdate_t playerupdate;
    input_t *osyn = (input_t *)&inputfifo[1][0];
    input_t *nsyn = (input_t *)&inputfifo[0][0];

    ticrandomseed = randomseed;

    if (g_netServer)
    {
        Bmemcpy(&osyn[0], &nsyn[0], sizeof(input_t));
    }

    if (!g_netServer || numplayers < 2)
    {
        return;
    }

    serverupdate.header = PACKET_MASTER_TO_SLAVE;
    serverupdate.seed = ticrandomseed;
    serverupdate.nsyn = *nsyn;
    serverupdate.pause_on = ud.pause_on;

    serverupdate.numplayers = 0;
    updatebuf = tempnetbuf + sizeof(serverupdate_t);

    for (TRAVERSE_CONNECT(i))
    {
        if (g_player[i].playerquitflag == 0)
        {
            continue;
        }

        Net_FillPlayerUpdate(&playerupdate.player, i);

        playerupdate.gotweapon = g_player[i].ps->gotweapon;
        playerupdate.extra = sprite[g_player[i].ps->i].extra;
        playerupdate.cstat = sprite[g_player[i].ps->i].cstat;
        playerupdate.owner = actor[g_player[i].ps->i].owner;
        playerupdate.picnum = actor[g_player[i].ps->i].picnum;
        playerupdate.kickback_pic = g_player[i].ps->kickback_pic;
        Bmemcpy(playerupdate.frags, g_player[i].frags, sizeof(playerupdate.frags));
        Bmemcpy(playerupdate.inv_amount, g_player[i].ps->inv_amount, sizeof(playerupdate.inv_amount));
        Bmemcpy(playerupdate.ammo_amount, g_player[i].ps->ammo_amount, sizeof(playerupdate.ammo_amount));

        playerupdate.curr_weapon = g_player[i].ps->curr_weapon;
        playerupdate.last_weapon = g_player[i].ps->last_weapon;
        playerupdate.wantweaponfire = g_player[i].ps->wantweaponfire;
        playerupdate.weapon_pos = g_player[i].ps->weapon_pos;
        playerupdate.frag_ps = g_player[i].ps->frag_ps;
        playerupdate.frag = g_player[i].ps->frag;
        playerupdate.fraggedself = g_player[i].ps->fraggedself;
        playerupdate.last_extra = g_player[i].ps->last_extra;
        playerupdate.ping = g_player[i].ping;
        playerupdate.newowner = g_player[i].ps->newowner;
        playerupdate.pal = sprite[g_player[i].ps->i].pal;

        Bmemcpy(updatebuf, &playerupdate, sizeof(serverplayerupdate_t));
        updatebuf += sizeof(serverplayerupdate_t);
        serverupdate.numplayers++;
    }

    if (serverupdate.numplayers == 0)
    {
        return;
    }

    Bmemcpy(tempnetbuf, &serverupdate, sizeof(serverupdate_t));

    enet_host_broadcast(g_netServer, CHAN_MOVE, enet_packet_create(tempnetbuf, sizeof(serverupdate_t) + (serverupdate.numplayers * sizeof(serverplayerupdate_t)), 0));
}

void Net_ReceiveServerUpdate(ENetEvent *event)
{
    int32_t i;
    uint8_t *updatebuf;
   // int8_t numupdates;
    serverupdate_t serverupdate;
    serverplayerupdate_t playerupdate;

    if (((event->packet->dataLength - sizeof(serverupdate_t)) % sizeof(serverplayerupdate_t)) != 0)
    {
        return;
    }

    updatebuf = (uint8_t *) event->packet->data;
    Bmemcpy(&serverupdate, updatebuf, sizeof(serverupdate_t));
    updatebuf += sizeof(serverupdate_t);
    inputfifo[0][0] = serverupdate.nsyn;
    ud.pause_on = serverupdate.pause_on;

    ticrandomseed = serverupdate.seed;

    for (i = 0; i < serverupdate.numplayers; ++i)
    {
        Bmemcpy(&playerupdate, updatebuf, sizeof(serverplayerupdate_t));
        updatebuf += sizeof(serverplayerupdate_t);

        Net_ExtractPlayerUpdate(&playerupdate.player, PACKET_MASTER_TO_SLAVE);

        g_player[i].ps->gotweapon = playerupdate.gotweapon;
        sprite[g_player[i].ps->i].extra = playerupdate.extra;
        sprite[g_player[i].ps->i].cstat = playerupdate.cstat;
        //actor[g_player[i].ps->i].owner = playerupdate.owner; // This makes baby jesus cry
        actor[g_player[i].ps->i].picnum = playerupdate.picnum;
        g_player[i].ps->kickback_pic = playerupdate.kickback_pic;
        Bmemcpy(g_player[i].frags, playerupdate.frags, sizeof(playerupdate.frags));
        Bmemcpy(g_player[i].ps->inv_amount, playerupdate.inv_amount, sizeof(playerupdate.inv_amount));
        Bmemcpy(g_player[i].ps->ammo_amount, playerupdate.ammo_amount, sizeof(playerupdate.ammo_amount));

        g_player[i].ps->curr_weapon = playerupdate.curr_weapon;
        g_player[i].ps->last_weapon = playerupdate.last_weapon;
        g_player[i].ps->wantweaponfire = playerupdate.wantweaponfire;
        g_player[i].ps->weapon_pos = playerupdate.weapon_pos;
        g_player[i].ps->frag_ps = playerupdate.frag_ps;
        g_player[i].ps->frag = playerupdate.frag;
        g_player[i].ps->fraggedself = playerupdate.fraggedself;
        g_player[i].ps->last_extra = playerupdate.last_extra;
        g_player[i].ping = playerupdate.ping;
        sprite[g_player[i].ps->i].pal = playerupdate.pal;
        g_player[i].ps->newowner = playerupdate.newowner;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Client Update Packets

#pragma pack(push,1)
typedef struct
{
    uint8_t header;
    uint32_t revision;
    input_t nsyn;
    playerupdate_t player;
} clientupdate_t;
#pragma pack(pop)

void Net_SendClientUpdate(void)
{
    clientupdate_t update;
    update.header = PACKET_SLAVE_TO_MASTER;
    update.revision = g_player[myconnectindex].revision;
    update.nsyn = inputfifo[0][myconnectindex];

    Net_FillPlayerUpdate(&update.player, myconnectindex);

    enet_peer_send(g_netClientPeer, CHAN_MOVE, enet_packet_create(&update, sizeof(clientupdate_t), 0));
}

void Net_ReceiveClientUpdate(ENetEvent *event)
{
    int32_t playeridx;
    clientupdate_t update;

    if (event->packet->dataLength != sizeof(clientupdate_t))
    {
        return;
    }

    Bmemcpy(&update, (char *) event->packet->data, sizeof(clientupdate_t));

    playeridx = (int32_t)(intptr_t) event->peer->data;

    if (playeridx < 0 || playeridx >= MAXPLAYERS)
    {
        return;
    }

    g_player[playeridx].revision = update.revision;
    inputfifo[0][playeridx] = update.nsyn;

    Net_ExtractPlayerUpdate(&update.player, PACKET_SLAVE_TO_MASTER);
}

////////////////////////////////////////////////////////////////////////////////
// Message Packets

void Net_SendMessage(void)
{

    if (g_player[myconnectindex].ps->gm&MODE_SENDTOWHOM)
    {
        int32_t i, j;
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
            }
            g_chatPlayer = -1;
            g_player[myconnectindex].ps->gm &= ~(MODE_TYPE|MODE_SENDTOWHOM);
        }
        else if (g_chatPlayer == -1)
        {
            j = 50;
            gametext_center(j, "Send message to...");
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
                    Bsprintf(recbuf,"      %d - %s",i+1,g_player[i].user_name);
                    minitextshade((320>>1)-40-6+1,j+1,recbuf,26,0,2+8+16);
                    minitext((320>>1)-40-6,j,recbuf,0,2+8+16);
                    j += 7;
                }
            }
            minitextshade((320>>1)-40-4+1,j+1,"    ESC - Abort",26,0,2+8+16);
            minitext((320>>1)-40-4,j,"    ESC - Abort",0,2+8+16);
            j += 7;

            mpgametext(mpgametext_x, ud.screen_size > 0 ? (200-45)<<16 : (200-8)<<16, typebuf, 0, 0, 0, 0);

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
#define MAXCHATLENGTH 120
        EDUKE32_STATIC_ASSERT(MAXCHATLENGTH < TYPEBUFSIZE);
        int32_t const hitstate = I_EnterText(typebuf, MAXCHATLENGTH, 0);

        int32_t const y = ud.screen_size > 1 ? (200-58)<<16 : (200-35)<<16;

        int32_t const width = mpgametextsize(typebuf, TEXT_LITERALESCAPE).x;
        int32_t const fullwidth = width + textsc((tilesiz[SPINNINGNUKEICON].x<<15)+(2<<16));
        int32_t const text_x = fullwidth >= (320<<16) ? (320<<16) - fullwidth : mpgametext_x;
        mpgametext(text_x, y, typebuf, 1, 2|8|16|ROTATESPRITE_FULL16, 0, TEXT_YCENTER|TEXT_LITERALESCAPE);
        int32_t const cursor_x = text_x + width + textsc((tilesiz[SPINNINGNUKEICON].x<<14)+(1<<16));
        rotatesprite_fs(cursor_x, y, textsc(32768), 0, SPINNINGNUKEICON+((totalclock>>3)%7), 4-(sintable[(totalclock<<4)&2047]>>11), 0, 2|8);

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

void Net_ReceiveMessage(uint8_t *pbuf, int32_t packbufleng)
{
    Bstrncpy(recbuf, (char *)pbuf+2, packbufleng-2);
    recbuf[packbufleng-2] = 0;

    G_AddUserQuote(recbuf);
    S_PlaySound(EXITMENUSOUND);

    pus = pub = NUMPAGES;
}

////////////////////////////////////////////////////////////////////////////////
// New Game Packets

void Net_StartNewGame()
{
    int32_t i;

    for (TRAVERSE_CONNECT(i))
    {
        P_ResetWeapons(i);
        P_ResetInventory(i);
        g_player[i].revision = 0;
    }

    Net_ExtractNewGame(&pendingnewgame, 0);
    G_NewGame(ud.volume_number,ud.level_number,ud.player_skill);
    ud.coop = ud.m_coop;

    g_netMapRevision = 0;

    if (G_EnterLevel(MODE_GAME))
    {
        G_BackToMenu();
    }
}

void Net_NotifyNewGame()
{
    int32_t i;
    int32_t statIndex;
    int32_t numSprites = 0;
    int32_t numSpritesToNetAlloc = 0;

    if (!g_netServer && !g_netClient)
    {
        return;
    }

    // Grab the total number of sprites at level load
    for (statIndex = 0; statIndex < MAXSTATUS; ++statIndex)
    {
        i = headspritestat[statIndex];
        for (; i >= 0; i = nextspritestat[i])
        {
            numSprites++;
        }
    }

    // Take half of the leftover sprites and allocate them for the network's nefarious purposes.
    numSpritesToNetAlloc = (MAXSPRITES - numSprites) / 2;
    for (i = 0; i < numSpritesToNetAlloc; ++i)
    {
        int32_t newSprite = insertspritestat(STAT_NETALLOC);
        sprite[newSprite].sectnum = MAXSECTORS;
        Numsprites++;
    }

    Net_SaveMapState(&g_mapStartState);
}

void Net_SendNewGame(int32_t frommenu, ENetPeer *peer)
{
    newgame_t newgame;

    newgame.header = PACKET_NEW_GAME;
    Net_FillNewGame(&newgame, frommenu);

    if (peer)
    {
        enet_peer_send(peer, CHAN_GAMESTATE, enet_packet_create(&newgame, sizeof(newgame_t), ENET_PACKET_FLAG_RELIABLE));
    }
    else
    {
        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(&newgame, sizeof(newgame_t), ENET_PACKET_FLAG_RELIABLE));
    }
}

void Net_ReceiveNewGame(ENetEvent *event)
{
    if ((vote_map + vote_episode + voting) != -3)
        G_AddUserQuote("Vote Succeeded");

    Bmemcpy(&pendingnewgame, event->packet->data, sizeof(newgame_t));
    Net_StartNewGame();

    packbuf[0] = PACKET_PLAYER_READY;
    packbuf[1] = myconnectindex;

    if (g_netClientPeer)
    {
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(packbuf, 2, ENET_PACKET_FLAG_RELIABLE));
    }

    g_player[myconnectindex].ps->gm = MODE_GAME;
    ready2send = 1;
}

void Net_FillNewGame(newgame_t *newgame, int32_t frommenu)
{
    if (frommenu)
    {
        newgame->level_number = ud.m_level_number;
        newgame->volume_number = ud.m_volume_number;
        newgame->player_skill = ud.m_player_skill;
        newgame->monsters_off = ud.m_monsters_off;
        newgame->respawn_monsters = ud.m_respawn_monsters;
        newgame->respawn_items = ud.m_respawn_items;
        newgame->respawn_inventory = ud.m_respawn_inventory;
        newgame->ffire = ud.m_ffire;
        newgame->noexits = ud.m_noexits;
        newgame->coop = ud.m_coop;
    }
    else
    {
        newgame->level_number = ud.level_number;
        newgame->volume_number = ud.volume_number;
        newgame->player_skill = ud.player_skill;
        newgame->monsters_off = ud.monsters_off;
        newgame->respawn_monsters = ud.respawn_monsters;
        newgame->respawn_items = ud.respawn_items;
        newgame->respawn_inventory = ud.respawn_inventory;
        newgame->ffire = ud.ffire;
        newgame->noexits = ud.noexits;
        newgame->coop = ud.coop;
    }
}

void Net_ExtractNewGame(newgame_t *newgame, int32_t menuonly)
{
    ud.m_level_number = newgame->level_number;
    ud.m_volume_number = newgame->volume_number;
    ud.m_player_skill = newgame->player_skill;
    ud.m_monsters_off = newgame->monsters_off;
    ud.m_respawn_monsters = newgame->respawn_monsters;
    ud.m_respawn_items = newgame->respawn_items;
    ud.m_respawn_inventory = newgame->respawn_inventory;
    ud.m_ffire = newgame->ffire;
    ud.m_noexits = newgame->noexits;
    ud.m_coop = newgame->coop;

    if (!menuonly)
    {
        ud.level_number = newgame->level_number;
        ud.volume_number = newgame->volume_number;
        ud.player_skill = newgame->player_skill;
        ud.monsters_off = newgame->monsters_off;
        ud.respawn_monsters = newgame->respawn_monsters;
        ud.respawn_monsters = newgame->respawn_items;
        ud.respawn_inventory = newgame->respawn_inventory;
        ud.ffire = newgame->ffire;
        ud.noexits = newgame->noexits;
        ud.coop = newgame->coop;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Map Vote Packets

void Net_SendMapVoteInitiate(void)
{
    newgame_t newgame;

    if (!g_netClient)
    {
        return;
    }

    voting = myconnectindex;

    newgame.header = PACKET_MAP_VOTE_INITIATE;
    newgame.connection = myconnectindex;
    Net_FillNewGame(&newgame, 1);

    enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&newgame, sizeof(newgame_t), ENET_PACKET_FLAG_RELIABLE));
}

void Net_ReceiveMapVoteInitiate(uint8_t *pbuf)
{
    int32_t i;

    Bmemcpy(&pendingnewgame, pbuf, sizeof(newgame_t));
    Net_ExtractNewGame(&pendingnewgame, 1);

    voting = pendingnewgame.connection;
    vote_episode = pendingnewgame.volume_number;
    vote_map = pendingnewgame.level_number;

    Bsprintf(tempbuf,"%s^00 has called a vote to change map to %s (E%dL%d)",
             g_player[voting].user_name,
             g_mapInfo[(uint8_t)(vote_episode*MAXLEVELS + vote_map)].name,
             vote_episode+1,vote_map+1);
    G_AddUserQuote(tempbuf);

    Bsprintf(tempbuf,"Press F1 to Accept, F2 to Decline");
    G_AddUserQuote(tempbuf);

    for (i=MAXPLAYERS-1; i>=0; i--)
    {
        g_player[i].vote = 0;
        g_player[i].gotvote = 0;
    }

    g_player[voting].gotvote = g_player[voting].vote = 1;
}

void Net_SendMapVote(int32_t votefor)
{
    voting = -1;
    g_player[myconnectindex].gotvote = 1;
    g_player[myconnectindex].vote = votefor;

    tempbuf[0] = PACKET_MAP_VOTE;
    tempbuf[1] = myconnectindex;
    tempbuf[2] = votefor;
    tempbuf[3] = myconnectindex;

    if (g_netClient)
    {
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(tempbuf, 4, ENET_PACKET_FLAG_RELIABLE));
    }
    else if (g_netServer)
    {
        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(tempbuf, 4, ENET_PACKET_FLAG_RELIABLE));
    }

    Net_CheckForEnoughVotes();
}

void Net_ReceiveMapVote(uint8_t *pbuf)
{
    if (voting == myconnectindex && g_player[(uint8_t)pbuf[1]].gotvote == 0)
    {
        Bsprintf(tempbuf,"Confirmed vote from %s",g_player[(uint8_t)pbuf[1]].user_name);
        G_AddUserQuote(tempbuf);
    }

    if (!g_netServer)
    {
        return;
    }

    g_player[(uint8_t)pbuf[1]].gotvote = 1;
    g_player[(uint8_t)pbuf[1]].vote = pbuf[2];
    Net_CheckForEnoughVotes();
}

void Net_CheckForEnoughVotes()
{
    int32_t i;
    int32_t requiredvotes;
    int32_t numfor, numagainst;

    // Only the server can decide map changes
    if (!g_netServer || numplayers <= 1)
    {
        return;
    }

    // If there are just two players, both of them deserve a vote
    if (numplayers == 2)
    {
        requiredvotes = 2;
    }
    else
    {
        // If more than two players, we need at least 50% of the players to vote
        // Which means that if there's an odd number of players, we'll need slightly more than 50% of the vote.
        requiredvotes = numplayers / 2;
        if (numplayers % 2 == 1)
        {
            requiredvotes++;
        }
    }

    numfor = numagainst = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (g_player[i].gotvote)
        {
            if (g_player[i].vote)
            {
                numfor++;
            }
            else
            {
                numagainst++;
            }
        }
    }

    if (numfor >= requiredvotes)
    {
        Net_StartNewGame();
        Net_SendNewGame(1, NULL);
    }
    else if (numagainst >= requiredvotes || (numfor + numagainst) == numplayers)
    {
        Net_SendMapVoteCancel(1);
    }
}

// If failed is true, that means the vote lost. Otherwise it was cancelled by the client who initiated it.
void Net_SendMapVoteCancel(int32_t failed)
{
    // Only the server or the client that initiated the vote can cancel the vote
    if (g_netClient && voting != myconnectindex)
    {
        return;
    }

    tempbuf[0] = PACKET_MAP_VOTE_CANCEL;
    tempbuf[1] = myconnectindex;

    // If we're forwarding a cancelled message, change the connection index to the one who cancelled it.
    if (g_netServer && !failed)
    {
        tempbuf[1] = voting;
    }

    voting = -1;

    if (g_netClient)
    {
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(tempbuf, 2, ENET_PACKET_FLAG_RELIABLE));
    }
    else if (g_netServer)
    {
        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(tempbuf, 2, ENET_PACKET_FLAG_RELIABLE));
    }
}

void Net_ReceiveMapVoteCancel(uint8_t *pbuf)
{
   // int32_t numvotes;

    // Ignore if we're not voting
    if (voting == -1)
    {
        return;
    }

    // Ignore cancellations from clients that did not initiate the map vote
    if (voting != pbuf[1] && voting != myconnectindex)
    {
        return;
    }

    if (voting == myconnectindex || voting != pbuf[1])
    {
        Bsprintf(tempbuf,"Vote Failed");
    }
    else if (voting == pbuf[1])
    {
        Bsprintf(tempbuf,"%s^00 has canceled the vote",g_player[voting].user_name);
    }

    G_AddUserQuote(tempbuf);

    if (g_netServer)
    {
        Net_SendMapVoteCancel(0);
    }

    voting = -1;
}

#endif  // !defined NETCODE_DISABLE
