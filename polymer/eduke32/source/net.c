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
#define SYNCPACKETSIZE 1366

ENetHost *g_netServer = NULL;
ENetHost *g_netClient = NULL;
ENetPeer *g_netClientPeer = NULL;
int32_t g_netPort = 23513;
int32_t g_netDisconnect = 0;
char g_netPassword[32];
int32_t g_netSync = 0;
int32_t g_netPlayersWaiting = 0;
int32_t g_netServerMode = 0;

static char recbuf[180];
static int32_t g_chatPlayer = -1;

// sprites of these statnums are synced to clients by the server
int8_t g_netStatnums[8] = { STAT_PROJECTILE, STAT_STANDABLE, STAT_ACTIVATOR, STAT_TRANSPORT,
                            STAT_EFFECTOR, STAT_ACTOR, STAT_ZOMBIEACTOR, STAT_MISC
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
    address.port = atoi((addrstr = strtok(NULL, ":")) == NULL ? "23513" : addrstr);

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
    buf[1] = BYTEVERSION;
    buf[2] = (uint8_t)atoi(s_buildDate);
    buf[3] = myconnectindex;

    enet_peer_send(client, CHAN_GAMESTATE, enet_packet_create(&buf[0], 4, ENET_PACKET_FLAG_RELIABLE));
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
    uint32_t crc = crc32once((uint8_t *)g_netPassword, Bstrlen(g_netPassword));

    if (!g_netClientPeer) return;

    buf[0] = PACKET_AUTH;
    *(uint32_t *)&buf[1] = crc;
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

        Bstrcpy(ScriptQuotes[116],buf);
        g_player[myconnectindex].ps->ftq = 116;
        g_player[myconnectindex].ps->fta = 180;
    }
}

// sync a connecting player up with the current game state
void Net_SyncPlayer(ENetEvent *event)
{
    int32_t i, j;

    g_netPlayersWaiting++;

    S_PlaySound(DUKE_GETWEAPON2);

    TRAVERSE_CONNECT(i)
    if (g_player[i].playerquitflag == 0)
        break;

    // open a new slot if necessary
    event->peer->data = (void *)((intptr_t)(i = (i == -1 ? playerswhenstarted++ : i)));

    g_player[i].netsynctime = totalclock;
    g_player[i].playerquitflag = 1;

    for (j=0; j<playerswhenstarted-1; j++) connectpoint2[j] = j+1;
    connectpoint2[playerswhenstarted-1] = -1;

    TRAVERSE_CONNECT(j)
    {
        if (!g_player[j].ps) g_player[j].ps = (DukePlayer_t *) Bcalloc(1, sizeof(DukePlayer_t));
        if (!g_player[j].sync) g_player[j].sync = (input_t *) Bcalloc(1, sizeof(input_t));
    }

    packbuf[0] = PACKET_NUM_PLAYERS;
    packbuf[1] = ++numplayers;
    packbuf[2] = playerswhenstarted;
    packbuf[3] = ++ud.multimode;
    packbuf[4] = i;
    packbuf[5] = myconnectindex;
    enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, 6, ENET_PACKET_FLAG_RELIABLE));

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
                // all of these packets are SYNCPACKETSIZE
                do
                {
                    enet_peer_send(event->peer, CHAN_SYNC,
                                   enet_packet_create((char *)(buf)+qlz_size_compressed(buf)-j, SYNCPACKETSIZE, ENET_PACKET_FLAG_RELIABLE));
                    j -= SYNCPACKETSIZE;
                }
                while (j >= SYNCPACKETSIZE);

                // ...except for this one.  A non-SYNCPACKETSIZE packet on CHAN_SYNC doubles as the signal that the transfer is done.
                enet_peer_send(event->peer, CHAN_SYNC,
                               enet_packet_create((char *)(buf)+qlz_size_compressed(buf)-j, j, ENET_PACKET_FLAG_RELIABLE));
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
    int16_t sect, statnum, osect, ostatnum, jj, opicnum, j = 0;
#ifdef POLYMER
    int16_t lightid = -1;
    _prlight *mylight = NULL;
#endif

    osect = sprite[i].sectnum;
    ostatnum = sprite[i].statnum;
    opicnum = sprite[i].picnum;

    Bmemcpy(&sprite[i], &pbuf[j], sizeof(spritetype));
    j += sizeof(spritetype);

    sect = sprite[i].sectnum;
    statnum = sprite[i].statnum;

    if (sect == MAXSECTORS || statnum == MAXSTATUS)
    {
        j += sizeof(netactor_t);
        deletesprite(i);
        return j;
    }

    sprite[i].sectnum = osect;
    sprite[i].statnum = ostatnum;

    // doesn't exist on the client yet
    if (ostatnum == MAXSTATUS || osect == MAXSECTORS)
    {
        int16_t sprs[MAXSPRITES], z = 0;
        while ((sprs[z++] = insertsprite(sect, statnum)) != i);
        z--;
        while (z--) deletesprite(sprs[z]);
    }
    else
    {
        if (sect != osect) changespritesect(i, sect);
        if (statnum != ostatnum) changespritestat(i, statnum);
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

    jj = j++;

    Bmemcpy(&actor[i], &pbuf[j], sizeof(netactor_t));
    j += sizeof(netactor_t);

    actor[i].projectile = &SpriteProjectile[i];
#ifdef POLYMER
    actor[i].lightptr = mylight;
    actor[i].lightId = lightid;
#endif

    actor[i].flags &= ~SPRITE_NULL;

    if (pbuf[jj] & 1) T2 += (intptr_t)&script[0];
    if (pbuf[jj] & 2) T5 += (intptr_t)&script[0];
    if (pbuf[jj] & 4) T6 += (intptr_t)&script[0];

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
    int32_t j = 0, jj;

    *(int16_t *)&pbuf[j] = i;
    j += sizeof(int16_t);
    Bmemcpy(&pbuf[j], &sprite[i], sizeof(spritetype));
    j += sizeof(spritetype);

    pbuf[(jj = j++)] = 0;

    if (T2 >= (intptr_t)&script[0] && T2 < (intptr_t)(&script[g_scriptSize]))
    {
        pbuf[jj] |= 1;
        T2 -= (intptr_t)&script[0];
    }

    if (T5 >= (intptr_t)&script[0] && T5 < (intptr_t)(&script[g_scriptSize]))
    {
        pbuf[jj] |= 2;
        T5 -= (intptr_t)&script[0];
    }

    if (T6 >= (intptr_t)&script[0] && T6 < (intptr_t)(&script[g_scriptSize]))
    {
        pbuf[jj] |= 4;
        T6 -= (intptr_t)&script[0];
    }

    Bmemcpy(&pbuf[j], &actor[i], sizeof(netactor_t));
    j += sizeof(netactor_t);

    if (pbuf[jj] & 1) T2 += (intptr_t)&script[0];
    if (pbuf[jj] & 2) T5 += (intptr_t)&script[0];
    if (pbuf[jj] & 4) T6 += (intptr_t)&script[0];

    {
        int16_t ii=g_gameVarCount-1, kk = 0;

        for (; ii>=0 && kk <= 64; ii--)
        {
            if ((aGameVars[ii].dwFlags & (GAMEVAR_PERACTOR|GAMEVAR_NOMULTI)) == GAMEVAR_PERACTOR && aGameVars[ii].val.plValues)
            {
                if (aGameVars[ii].val.plValues[i] != aGameVars[ii].lDefault)
                {
                    *(int16_t *)&pbuf[j] = ii;
                    j += sizeof(int16_t);
                    *(int32_t *)&pbuf[j] = aGameVars[ii].val.plValues[i];
                    j += sizeof(int32_t);
                    kk++;
                }
            }
        }
        *(int16_t *)&pbuf[j] = MAXGAMEVARS;
        j += sizeof(int16_t);
    }

    return j;
}

void Net_ParseServerPacket(ENetEvent *event)
{
    uint8_t *pbuf = event->packet->data;
    int32_t packbufleng = event->packet->dataLength;
    int16_t i, j, l;
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

        TRAVERSE_CONNECT(i)
        {
            g_player[i].ps->dead_flag = *(int16_t *)&pbuf[j];
            j += sizeof(int16_t);

            g_player[i].playerquitflag = pbuf[j++];

            if (g_player[i].playerquitflag == 0) continue;

            if (i == myconnectindex && !g_player[i].ps->dead_flag)
            {
                j += offsetof(input_t, filler) +
                     (sizeof(vec3_t) * 3) + // position and velocity
                     (sizeof(int16_t) * 3); // ang and horiz
                goto process;
            }

            nsyn = (input_t *)&inputfifo[0][0];

            Bmemcpy(&nsyn[i], &pbuf[j], offsetof(input_t, filler));
            j += offsetof(input_t, filler);

//                Bmemcpy(&g_player[i].ps->opos.x, &g_player[i].ps->pos.x, sizeof(vec3_t));

            Bmemcpy(&g_player[i].ps->pos.x, &pbuf[j], sizeof(vec3_t) * 2);
            if (g_player[i].ps->cursectnum >= 0 && g_player[i].ps->cursectnum < numsectors)
            {
                updatesectorz(g_player[i].ps->pos.x, g_player[i].ps->pos.y, g_player[i].ps->pos.z,
                              &g_player[i].ps->cursectnum);
                changespritesect(g_player[i].ps->i, g_player[i].ps->cursectnum);
            }
            Bmemcpy(&sprite[g_player[i].ps->i], &pbuf[j], sizeof(vec3_t));
            sprite[g_player[i].ps->i].z += PHEIGHT;
            j += sizeof(vec3_t) * 2;

            Bmemcpy(&g_player[i].ps->posvel.x, &pbuf[j], sizeof(vec3_t));
            j += sizeof(vec3_t);

            g_player[i].ps->oang = g_player[i].ps->ang;
            g_player[i].ps->ang = sprite[g_player[i].ps->i].ang = *(int16_t *)&pbuf[j];
            j += sizeof(int16_t);

            Bmemcpy(&g_player[i].ps->ohoriz, &g_player[i].ps->horiz, sizeof(int16_t) * 2);
            Bmemcpy(&g_player[i].ps->horiz, &pbuf[j], sizeof(int16_t) * 2);
            j += sizeof(int16_t) * 2;

process:
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

            sprite[g_player[i].ps->i].pal = (uint8_t)pbuf[j++];

            l = i;

            i = g_player[l].ps->i;

            {
                int16_t jj = j++;
                int32_t oa = (T5 >= (intptr_t)&script[0] && T5 < (intptr_t)&script[g_scriptSize]) ? T5-(intptr_t)&script[0] : T5;

                Bmemcpy(&T5, &pbuf[j], sizeof(T5));
                j += sizeof(T5);

                if (oa != T5) T3 = T4 = 0;
                if (pbuf[jj] & 2) T5 += (intptr_t)&script[0];
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

            l = pbuf[j++];
            while (l--)
            {
                int32_t i = *(int16_t *)&pbuf[j];
                j += sizeof(int16_t);

                j += Net_UnpackSprite(i, &pbuf[j]);
            }
        }

        Bfree(pbuf);

        break;

    case PACKET_MAP_STREAM:
        if (!(g_player[myconnectindex].ps->gm & MODE_GAME) || g_netSync) return;

        j = 0;

        packbufleng = qlz_size_decompressed((char *)&pbuf[1]);
        pbuf = (uint8_t *)Bcalloc(1, packbufleng+1);
        packbufleng = qlz_decompress((char *)&event->packet->data[1], (char *)(pbuf), state_decompress);

        l = pbuf[j++];
        while (l--)
        {
            int32_t i = *(int16_t *)&pbuf[j];
            j += sizeof(int16_t);

            j += Net_UnpackSprite(i, &pbuf[j]);
        }

        l = pbuf[j++];
        while (l--)
        {
            int16_t secid = *(int16_t *)&pbuf[j];

            Bmemcpy(&sector[secid], &pbuf[j + sizeof(int16_t)], sizeof(sectortype));
            j += sizeof(int16_t) + sizeof(sectortype);
        }

        l = pbuf[j++];
        while (l--)
        {
            int16_t wallid = *(int16_t *)&pbuf[j];

            Bmemcpy(&wall[wallid], &pbuf[j + sizeof(int16_t)], sizeof(walltype));
            j += sizeof(int16_t) + sizeof(walltype);
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

        TRAVERSE_CONNECT(i)
        {
            P_ResetWeapons(i);
            P_ResetInventory(i);
        }

        G_NewGame(ud.volume_number,ud.level_number,ud.player_skill);
        ud.coop = ud.m_coop;

        if (G_EnterLevel(MODE_GAME)) G_BackToMenu();
        break;

    case PACKET_VERSION:
        if (pbuf[1] != BYTEVERSION || pbuf[2] != (uint8_t)atoi(s_buildDate))
        {
            Bsprintf(tempbuf, "Server protocol is version %d.%d, expecting %d.%d\n",
                     pbuf[1], pbuf[2], BYTEVERSION, (uint8_t)atoi(s_buildDate));
            initprintf(tempbuf);
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
        break;

    case PACKET_PLAYER_READY:
        g_player[0].playerreadyflag++;
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
        Bmemcpy(&g_player[other].ps->pos.x, &pbuf[j], sizeof(vec3_t) * 2);
        updatesectorz(g_player[other].ps->pos.x, g_player[other].ps->pos.y, g_player[other].ps->pos.z,
                      &g_player[other].ps->cursectnum);
        Bmemcpy(&sprite[g_player[other].ps->i], &pbuf[j], sizeof(vec3_t));
        sprite[g_player[other].ps->i].z += PHEIGHT;
        changespritesect(g_player[other].ps->i, g_player[other].ps->cursectnum);
        j += sizeof(vec3_t) * 2;

        Bmemcpy(&g_player[other].ps->posvel.x, &pbuf[j], sizeof(vec3_t));
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
        if (g_player[myconnectindex].ps->gm & MODE_GAME)
        {
            packbuf[0] = PACKET_PLAYER_READY;
            packbuf[1] = myconnectindex;
            enet_peer_send(event->peer, CHAN_GAMESTATE, enet_packet_create(packbuf, 2, ENET_PACKET_FLAG_RELIABLE));
        }
        g_player[other].playerreadyflag++;
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

        TRAVERSE_CONNECT(i)
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

            j = g_player[other].ps->i;
            Bmemcpy(g_player[other].ps, g_player[0].ps, sizeof(DukePlayer_t));

            g_player[other].ps->i = j;
            changespritestat(j, STAT_PLAYER);

            P_ResetStatus(other);
            P_ResetWeapons(other);
            P_ResetInventory(other);

            g_player[other].ps->last_extra = sprite[g_player[other].ps->i].extra = g_player[other].ps->max_player_health;
            sprite[g_player[other].ps->i].cstat = 1+256;
            actor[g_player[other].ps->i].t_data[2] = actor[g_player[other].ps->i].t_data[3] = actor[g_player[other].ps->i].t_data[4] = 0;

            g_netPlayersWaiting--;

            // a player connecting is a good time to mark things as needing to be updated
            // we invalidate everything that has changed since we started sending the snapshot of the map to the new player

            {
                int32_t zz, i, nexti;

                for (zz = 0; (unsigned)zz < (sizeof(g_netStatnums)/sizeof(g_netStatnums[0])); zz++)
                    TRAVERSE_SPRITE_STAT(headspritestat[g_netStatnums[zz]], i, nexti)
                {
                    if (lastupdate[i] >= g_player[other].netsynctime)
                        spritecrc[i] = 0xdeadbeef;
                }
            }

            for (i=numwalls-1; i>=0; i--)
                if (lastwallupdate[i] >= g_player[other].netsynctime)
                    wallcrc[i] = 0xdeadbeef;

            for (i=numsectors-1; i>=0; i--)
                if (lastsectupdate[i] >= g_player[other].netsynctime)
                    sectcrc[i] = 0xdeadbeef;
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
                    static int32_t datasiz = 0;
                    static uint8_t *buf = NULL;

                    if (buf == NULL)
                    {
                        datasiz = 0;
                        g_netSync = 1;
                        buf = (uint8_t *)Bcalloc(1, sizeof(mapstate_t)<<1);
                    }

                    g_multiMapState = (mapstate_t *)Brealloc(g_multiMapState, sizeof(mapstate_t));

                    if (buf && event.packet->dataLength == SYNCPACKETSIZE)
                    {
                        Bmemcpy((uint8_t *)(buf)+datasiz, event.packet->data, event.packet->dataLength);
                        datasiz += SYNCPACKETSIZE;
                    }
                    // last packet of mapstate sequence
                    else if (buf)
                    {
                        Bmemcpy((uint8_t *)(buf)+datasiz, event.packet->data, event.packet->dataLength);
                        datasiz = 0;
                        g_netSync = 0;

                        if (qlz_size_decompressed((const char *)buf) == sizeof(mapstate_t))
                        {
                            qlz_decompress((const char *)buf, g_multiMapState, state_decompress);
                            Bfree(buf);
                            buf = NULL;

                            packbuf[0] = PACKET_REQUEST_GAMESTATE;
                            packbuf[1] = myconnectindex;
                            enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&packbuf[0], 2, ENET_PACKET_FLAG_RELIABLE));
                        }
                        else
                        {
                            initprintf("Invalid map state from server!\n");
                            Bfree(buf);
                            buf = NULL;
                            g_netDisconnect = 1;
                        }
                    }
                    else
                    {
                        initprintf("Error allocating buffer for map state!\n");
                        g_netDisconnect = 1;
                    }
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
                    initprintf("You have been banned from this server.\n");
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

    TRAVERSE_CONNECT(i)
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

        Bmemcpy(&packbuf[j], &g_player[i].ps->posvel.x, sizeof(vec3_t));
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

        packbuf[j++] = sprite[g_player[i].ps->i].pal;

        l = i;
        i = g_player[l].ps->i;

        {
            int32_t jj, oa;

            packbuf[(jj = j++)] = 0;

            if (T5 >= (intptr_t)&script[0] && T5 < (intptr_t)(&script[g_scriptSize]))
            {
                packbuf[jj] |= 2;
                T5 -= (intptr_t)&script[0];
            }

            oa = T5;

            Bmemcpy(&packbuf[j], &T5, sizeof(T5));
            j += sizeof(T5);

            if (oa != T5) T3 = T4 = 0;

            if (packbuf[jj] & 2) T5 += (intptr_t)&script[0];
        }

        {
            int16_t ii=g_gameVarCount-1, kk = 0;

            for (; ii>=0 && kk <= 64; ii--)
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
                    }
                }
            }
            *(int16_t *)&packbuf[j] = MAXGAMEVARS;
            j += sizeof(int16_t);
        }

        i = l;

        {
            int16_t ii=g_gameVarCount-1, kk = 0;

            for (; ii>=0 && kk <= 64; ii--)
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

        packbuf[(zj = j++)] = 0;

        for (zz = 0; (unsigned)zz < (sizeof(g_netStatnums)/sizeof(g_netStatnums[0])) /*&& k <= 3*/; zz++)
            TRAVERSE_SPRITE_STAT(headspritestat[g_netStatnums[zz]], i, nexti)
        {
            // only send newly spawned sprites
            if (!lastupdate[i])
            {
                lastupdate[i] = totalclock;

                j += Net_PackSprite(i, (uint8_t *)&packbuf[j]);
                k++;
            }
        }

        packbuf[zj] = k;
        j++;
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
    int16_t i, nexti, k = 0, l;
    int32_t j;
    int32_t zz, zj;

    if (!g_netServer || numplayers < 2)
        return;

    packbuf[0] = PACKET_MAP_STREAM;
    j = 1;

    packbuf[(zj = j++)] = 0;

    for (zz = 0; (unsigned)zz < (sizeof(g_netStatnums)/sizeof(g_netStatnums[0])) /*&& k <= 3*/; zz++)
        TRAVERSE_SPRITE_STAT(headspritestat[g_netStatnums[zz]], i, nexti)
    {
        // only send STAT_MISC sprites at spawn time and let the client handle it from there
        if (totalclock > (lastupdate[i] + TICRATE) && sprite[i].statnum != STAT_MISC)
        {
            l = crc32once((uint8_t *)&sprite[i], sizeof(spritetype));

            if (!lastupdate[i] || spritecrc[i] != l)
            {
                /*initprintf("updating sprite %d (%d)\n",i,sprite[i].picnum);*/
                spritecrc[i] = l;
                lastupdate[i] = totalclock;

                j += Net_PackSprite(i, (uint8_t *)&packbuf[j]);
                k++;
            }
        }
    }

    packbuf[zj] = k;
    k = 0;

    packbuf[(zj = j++)] = 0;
    for (i = numsectors-1; i >= 0 && k <= 6; i--)
    {
        if (totalclock > (lastsectupdate[i] + TICRATE))
        {
            l = crc32once((uint8_t *)&sector[i], sizeof(sectortype));

            if (sectcrc[i] != l)
            {
                sectcrc[i] = l;
                lastsectupdate[i] = totalclock;
                *(int16_t *)&packbuf[j] = i;
                j += sizeof(int16_t);
                Bmemcpy(&packbuf[j], &sector[i], sizeof(sectortype));
                j += sizeof(sectortype);
                k++;
            }
        }
    }
    packbuf[zj] = k;
    k = 0;

    packbuf[(zj = j++)] = 0;
    for (i = numwalls-1; i >= 0 && k <= 6; i--)
    {
        if (totalclock > (lastwallupdate[i] + TICRATE))
        {
            l = crc32once((uint8_t *)&wall[i], sizeof(walltype));

            if (wallcrc[i] != l)
            {
                wallcrc[i] = l;
                lastwallupdate[i] = totalclock;
                *(int16_t *)&packbuf[j] = i;
                j += sizeof(int16_t);
                Bmemcpy(&packbuf[j], &wall[i], sizeof(walltype));
                j += sizeof(walltype);
                k++;
            }
        }
    }
    packbuf[zj] = k;

    j++;

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

    enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, j, ENET_PACKET_FLAG_RELIABLE));
}

void faketimerhandler(void)
{
    if (g_netServer)
        enet_host_service(g_netServer, NULL, 0);
    else if (g_netClient)
        enet_host_service(g_netClient, NULL, 0);
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
            TRAVERSE_CONNECT(i)
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
    int32_t server_ready = g_player[0].playerreadyflag;

    if (numplayers < 2 || g_netServer) return;

    if ((g_netServer || ud.multimode > 1))
    {
        P_SetGamePalette(g_player[myconnectindex].ps, titlepal, 11);
        rotatesprite(0,0,65536L,0,BETASCREEN,0,0,2+8+16+64,0,0,xdim-1,ydim-1);

        rotatesprite(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);
        rotatesprite(160<<16,(129)<<16,30<<11,0,THREEDEE,0,0,2+8,0,0,xdim-1,ydim-1);
        if (PLUTOPAK)   // JBF 20030804
            rotatesprite(160<<16,(151)<<16,30<<11,0,PLUTOPAKSPRITE+1,0,0,2+8,0,0,xdim-1,ydim-1);

        gametext(160,190,"WAITING FOR SERVER",14,2);
        nextpage();
    }

    do
    {
        if (quitevent || keystatus[1]) G_GameExit("");

        packbuf[0] = PACKET_PLAYER_READY;
        packbuf[1] = myconnectindex;

        if (g_netClientPeer)
            enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(packbuf, 2, ENET_PACKET_FLAG_RELIABLE));

        handleevents();
        Net_GetPackets();

        if (g_player[0].playerreadyflag > server_ready)
        {
            P_SetGamePalette(g_player[myconnectindex].ps, palette, 11);
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

