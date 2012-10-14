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
#include "savegame.h"
#include "input.h"

#include "enet/enet.h"
#include "quicklz.h"
#include "crc32.h"
#include "xdelta3.h"

/*
this should be lower than the MTU size by at least the size of the UDP and ENet headers
or else fragmentation will occur
*/
#define SYNCPACKETSIZE 1344

uint32_t g_netMapRevision = 0;
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

#pragma pack(push,1)
netmapstate_t *g_multiMapState[MAXPLAYERS];
netmapstate_t *g_pendingMapState[MAXPLAYERS];
netmapstate_t *g_multiMapRevisions[NET_REVISIONS];
netmapstate_t *streamoutput = NULL;
#pragma pack(pop)

static void alloc_multimapstate(int32_t i)
{
    if (g_multiMapState[i] == NULL)
    {
        g_multiMapState[i] = Bcalloc(1, sizeof(netmapstate_t));
        if (g_multiMapState[i] == NULL)
            G_GameExit("OUT OF MEMORY in alloc_multimapstate");
    }

    if (g_pendingMapState[i] == NULL)
    {
        g_pendingMapState[i] = Bcalloc(1, sizeof(netmapstate_t));
        if (g_pendingMapState[i] == NULL)
            G_GameExit("OUT OF MEMORY in alloc_multimapstate");
    }
}

void Net_SaveMapState(netmapstate_t *save)
{
    if (save != NULL)
    {
        int32_t i;
        intptr_t j;

        save->revision = g_netMapRevision;
        Bmemcpy(&save->numwalls,&numwalls,sizeof(numwalls));
        Bmemcpy(&save->wall[0],&wall[0],sizeof(walltype)*MAXWALLS);
        Bmemcpy(&save->numsectors,&numsectors,sizeof(numsectors));
        Bmemcpy(&save->sector[0],&sector[0],sizeof(sectortype)*MAXSECTORS);
        Bmemcpy(&save->sprite[0],&sprite[0],sizeof(spritetype)*MAXSPRITES);
        Bmemcpy(&save->spriteext[0],&spriteext[0],sizeof(spriteext_t)*MAXSPRITES);
        Bmemcpy(&save->headspritesect[0],&headspritesect[0],sizeof(headspritesect));
        Bmemcpy(&save->prevspritesect[0],&prevspritesect[0],sizeof(prevspritesect));
        Bmemcpy(&save->nextspritesect[0],&nextspritesect[0],sizeof(nextspritesect));
        Bmemcpy(&save->headspritestat[0],&headspritestat[0],sizeof(headspritestat));
        Bmemcpy(&save->prevspritestat[0],&prevspritestat[0],sizeof(prevspritestat));
        Bmemcpy(&save->nextspritestat[0],&nextspritestat[0],sizeof(nextspritestat));


        for (i=MAXSPRITES-1; i>=0; i--)
            save->scriptptrs[i] = 0;


        Bmemcpy(&save->actor[0],&actor[0],sizeof(netactor_t)*MAXSPRITES);

        for (i=MAXSPRITES-1; i>=0; i--)
            for (j=0;j<10;j++)
                save->actor[i].t_data[j] = actor[i].t_data[j];



        Bmemcpy(&save->g_numCyclers,&g_numCyclers,sizeof(g_numCyclers));
        Bmemcpy(&save->cyclers[0][0],&cyclers[0][0],sizeof(cyclers));
        Bmemcpy(&save->g_playerSpawnPoints[0],&g_playerSpawnPoints[0],sizeof(g_playerSpawnPoints));
        Bmemcpy(&save->g_numAnimWalls,&g_numAnimWalls,sizeof(g_numAnimWalls));
        Bmemcpy(&save->SpriteDeletionQueue[0],&SpriteDeletionQueue[0],sizeof(SpriteDeletionQueue));
        Bmemcpy(&save->g_spriteDeleteQueuePos,&g_spriteDeleteQueuePos,sizeof(g_spriteDeleteQueuePos));
        Bmemcpy(&save->animwall[0],&animwall[0],sizeof(animwall));
        Bmemcpy(&save->msx[0],&msx[0],sizeof(msx));
        Bmemcpy(&save->msy[0],&msy[0],sizeof(msy));
        Bmemcpy(&save->g_mirrorWall[0],&g_mirrorWall[0],sizeof(g_mirrorWall));
        Bmemcpy(&save->g_mirrorSector[0],&g_mirrorSector[0],sizeof(g_mirrorSector));
        Bmemcpy(&save->g_mirrorCount,&g_mirrorCount,sizeof(g_mirrorCount));
/*        Bmemcpy(&save->animategoal[0],&animategoal[0],sizeof(animategoal));
        Bmemcpy(&save->animatevel[0],&animatevel[0],sizeof(animatevel));
        Bmemcpy(&save->g_animateCount,&g_animateCount,sizeof(g_animateCount));
        Bmemcpy(&save->animatesect[0],&animatesect[0],sizeof(animatesect));


        G_Util_PtrToIdx(animateptr, g_animateCount, sector, P2I_FWD);
        Bmemcpy(&save->animateptr[0],&animateptr[0],sizeof(animateptr));
        G_Util_PtrToIdx(animateptr, g_animateCount, sector, P2I_BACK);
*/

        Bmemcpy(&save->g_numPlayerSprites,&g_numPlayerSprites,sizeof(g_numPlayerSprites));
        Bmemcpy(&save->g_earthquakeTime,&g_earthquakeTime,sizeof(g_earthquakeTime));
//        Bmemcpy(&save->lockclock,&lockclock,sizeof(lockclock));
        Bmemcpy(&save->randomseed,&randomseed,sizeof(randomseed));
        Bmemcpy(&save->g_globalRandom,&g_globalRandom,sizeof(g_globalRandom));

/*
        for (i=g_gameVarCount-1; i>=0; i--)
        {
            if (aGameVars[i].dwFlags & GAMEVAR_NORESET) continue;
            if (aGameVars[i].dwFlags & GAMEVAR_PERPLAYER)
            {
                if (!save->vars[i])
                    save->vars[i] = Bcalloc(MAXPLAYERS,sizeof(intptr_t));
                Bmemcpy(&save->vars[i][0],&aGameVars[i].val.plValues[0],sizeof(intptr_t) * MAXPLAYERS);
            }
            else if (aGameVars[i].dwFlags & GAMEVAR_PERACTOR)
            {
                if (!save->vars[i])
                    save->vars[i] = Bcalloc(MAXSPRITES,sizeof(intptr_t));
                Bmemcpy(&save->vars[i][0],&aGameVars[i].val.plValues[0],sizeof(intptr_t) * MAXSPRITES);
            }
            else save->vars[i] = (intptr_t *)aGameVars[i].val.lValue;
        }
*/

//        ototalclock = totalclock;

        save->crc = crc32once((uint8_t *)save, offsetof(netmapstate_t, crc));
    }
}

extern void Gv_RefreshPointers(void);

void Net_RestoreMapState(netmapstate_t *save)
{
    if (save != NULL)
    {
        int32_t i;
        // int32_t x; // used in commented code below
        intptr_t j;
        char phealth[MAXPLAYERS];

        initprintf("restoring revision %d\n", save->revision);
        Bassert(save->crc == crc32once((uint8_t *)save, offsetof(netmapstate_t, crc)));

        for (i=0; i<playerswhenstarted; i++)
            phealth[i] = sprite[g_player[i].ps->i].extra;

        pub = NUMPAGES;
        pus = NUMPAGES;
        G_UpdateScreenArea();

        Bmemcpy(&numwalls,&save->numwalls,sizeof(numwalls));
        Bmemcpy(&wall[0],&save->wall[0],sizeof(walltype)*MAXWALLS);
        Bmemcpy(&numsectors,&save->numsectors,sizeof(numsectors));
        Bmemcpy(&sector[0],&save->sector[0],sizeof(sectortype)*MAXSECTORS);
        Bmemcpy(&sprite[0],&save->sprite[0],sizeof(spritetype)*MAXSPRITES);
        Bmemcpy(&spriteext[0],&save->spriteext[0],sizeof(spriteext_t)*MAXSPRITES);

        Bmemcpy(&headspritesect[0],&save->headspritesect[0],sizeof(headspritesect));
        Bmemcpy(&prevspritesect[0],&save->prevspritesect[0],sizeof(prevspritesect));
        Bmemcpy(&nextspritesect[0],&save->nextspritesect[0],sizeof(nextspritesect));
        Bmemcpy(&headspritestat[0],&save->headspritestat[0],sizeof(headspritestat));
        Bmemcpy(&prevspritestat[0],&save->prevspritestat[0],sizeof(prevspritestat));
        Bmemcpy(&nextspritestat[0],&save->nextspritestat[0],sizeof(nextspritestat));

        Bmemcpy(&actor[0],&save->actor[0],sizeof(netactor_t)*MAXSPRITES);


        for (i=MAXSPRITES-1; i>=0; i--)
            for (j=0;j<10;j++)
                actor[i].t_data[j] = save->actor[i].t_data[j];



        Bmemcpy(&g_numCyclers,&save->g_numCyclers,sizeof(g_numCyclers));
        Bmemcpy(&cyclers[0][0],&save->cyclers[0][0],sizeof(cyclers));
        Bmemcpy(&g_playerSpawnPoints[0],&save->g_playerSpawnPoints[0],sizeof(g_playerSpawnPoints));
        Bmemcpy(&g_numAnimWalls,&save->g_numAnimWalls,sizeof(g_numAnimWalls));
        Bmemcpy(&SpriteDeletionQueue[0],&save->SpriteDeletionQueue[0],sizeof(SpriteDeletionQueue));
        Bmemcpy(&g_spriteDeleteQueuePos,&save->g_spriteDeleteQueuePos,sizeof(g_spriteDeleteQueuePos));
        Bmemcpy(&animwall[0],&save->animwall[0],sizeof(animwall));
        Bmemcpy(&msx[0],&save->msx[0],sizeof(msx));
        Bmemcpy(&msy[0],&save->msy[0],sizeof(msy));
        Bmemcpy(&g_mirrorWall[0],&save->g_mirrorWall[0],sizeof(g_mirrorWall));
        Bmemcpy(&g_mirrorSector[0],&save->g_mirrorSector[0],sizeof(g_mirrorSector));
        Bmemcpy(&g_mirrorCount,&save->g_mirrorCount,sizeof(g_mirrorCount));
        /*

        Bmemcpy(&animategoal[0],&save->animategoal[0],sizeof(animategoal));
        Bmemcpy(&animatevel[0],&save->animatevel[0],sizeof(animatevel));
        Bmemcpy(&g_animateCount,&save->g_animateCount,sizeof(g_animateCount));
        Bmemcpy(&animatesect[0],&save->animatesect[0],sizeof(animatesect));

        Bmemcpy(&animateptr[0],&save->animateptr[0],sizeof(animateptr));
        G_Util_PtrToIdx(animateptr, g_animateCount, sector, P2I_BACK);
*/

        Bmemcpy(&g_numPlayerSprites,&save->g_numPlayerSprites,sizeof(g_numPlayerSprites));
        Bmemcpy(&g_earthquakeTime,&save->g_earthquakeTime,sizeof(g_earthquakeTime));
//        Bmemcpy(&lockclock,&save->lockclock,sizeof(lockclock));
        Bmemcpy(&randomseed,&save->randomseed,sizeof(randomseed));
        Bmemcpy(&g_globalRandom,&save->g_globalRandom,sizeof(g_globalRandom));

/*
        for (i=g_gameVarCount-1; i>=0; i--)
        {
            if (aGameVars[i].dwFlags & GAMEVAR_NORESET) continue;
            if (aGameVars[i].dwFlags & GAMEVAR_PERPLAYER)
            {
                if (!save->vars[i]) continue;
                Bmemcpy(&aGameVars[i].val.plValues[0],&save->vars[i][0],sizeof(intptr_t) * MAXPLAYERS);
            }
            else if (aGameVars[i].dwFlags & GAMEVAR_PERACTOR)
            {
                if (!save->vars[i]) continue;
                Bmemcpy(&aGameVars[i].val.plValues[0],&save->vars[i][0],sizeof(intptr_t) * MAXSPRITES);
            }
            else aGameVars[i].val.lValue = (intptr_t)save->vars[i];
        }
*/

        Gv_RefreshPointers();

        for (i=0; i<playerswhenstarted; i++)
            sprite[g_player[i].ps->i].extra = phealth[i];

        if (g_player[myconnectindex].ps->over_shoulder_on != 0)
        {
            g_cameraDistance = 0;
            g_cameraClock = 0;
            g_player[myconnectindex].ps->over_shoulder_on = 1;
        }

        screenpeek = myconnectindex;

/*
        if (ud.lockout == 0)
        {
            for (x=g_numAnimWalls-1; x>=0; x--)
                if (wall[animwall[x].wallnum].extra >= 0)
                    wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
        }
        else
        {
            for (x=g_numAnimWalls-1; x>=0; x--)
                switch (DynamicTileMap[wall[animwall[x].wallnum].picnum])
            {
                case FEMPIC1__STATIC:
                    wall[animwall[x].wallnum].picnum = BLANKSCREEN;
                    break;
                case FEMPIC2__STATIC:
                case FEMPIC3__STATIC:
                    wall[animwall[x].wallnum].picnum = SCREENBREAK6;
                    break;
            }
        }
*/

        G_ResetInterpolations();

//        Net_ResetPrediction();

//        G_ClearFIFO();
//        G_ResetTimers();
        initprintf("Net_RestoreMapState(): restored revision %u\n",save->revision);
    }
}


void Net_Connect(const char *srvaddr)
{
    ENetAddress address;
    ENetEvent event;
    char *addrstr = NULL;
    int32_t i;

    char *oursrvaddr = Bstrdup(srvaddr);
    if (!oursrvaddr) G_GameExit("OUT OF MEMORY");

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
        int32_t i;
        ENetEvent event;

        for (i=0; i<(signed)g_netServer->peerCount; i++)
            enet_peer_disconnect_later(&g_netServer->peers[i], 0);

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
        Bfree(streamoutput);
    }
}

static void Net_SendVersion(ENetPeer *client)
{
    if (!g_netServer) return;

    buf[0] = PACKET_VERSION;
    buf[1] = BYTEVERSION;
    // XXX: s_buildDate is outdated and useless; uint8 is not enough :/
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

    Bcorrectfilename(boardfilename,0);

    // user map name is sent with a NUL at the end
    j = Bstrlen(boardfilename)+1;
    Bmemcpy(&packbuf[1], boardfilename, j);
    j++;

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

static void P_RemovePlayer(int32_t p)
{
    // server obviously can't leave the game, and index 0 shows up for disconnect events from
    // players that haven't gotten far enough into the connection process to get a player ID

    if (p == 0) return;

    g_player[p].playerquitflag = 0;

    Bsprintf(buf,"%s^00 is history!",g_player[p].user_name);
    G_AddUserQuote(buf);

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
    for (TRAVERSE_CONNECT(i))
        if (g_player[i].playerquitflag == 0)
            break;
    if (i == -1)
        i = playerswhenstarted++;
    event->peer->data = (void *)(intptr_t)i;

    g_player[i].netsynctime = totalclock;
    g_player[i].playerquitflag = 1;
    g_player[i].revision = g_netMapRevision;

    for (j=0; j<playerswhenstarted-1; j++) connectpoint2[j] = j+1;
    connectpoint2[playerswhenstarted-1] = -1;

    if (!g_player[i].ps) g_player[i].ps = (DukePlayer_t *) Bcalloc(1, sizeof(DukePlayer_t));
    if (!g_player[i].sync) g_player[i].sync = (input_t *) Bcalloc(1, sizeof(input_t));

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
        alloc_multimapstate(i);

        {
            char *buf = (char *)Bmalloc(sizeof(netmapstate_t)+512);

            sprite[g_player[i].ps->i].cstat = 32768;
            g_player[i].ps->runspeed = g_playerFriction;
            g_player[i].ps->last_extra = sprite[g_player[i].ps->i].extra = g_player[i].ps->max_player_health = g_maxPlayerHealth;

            Net_SaveMapState(g_multiMapState[i]);
            if ((j = qlz_compress((char *)g_multiMapState[i], buf, sizeof(netmapstate_t), state_compress)))
            {
                size_t csize = qlz_size_compressed(buf);

                // all of these packets are SYNCPACKETSIZE
                do
                {
                    enet_peer_send(event->peer, CHAN_SYNC,
                        enet_packet_create(buf+csize-j, SYNCPACKETSIZE, ENET_PACKET_FLAG_RELIABLE));
                    j -= SYNCPACKETSIZE;
                    enet_host_service(g_netServer, NULL, 0);
                }
                while (j >= SYNCPACKETSIZE);

                // ...except for this one.  A non-SYNCPACKETSIZE packet on
                // CHAN_SYNC doubles as the signal that the transfer is done.
                enet_peer_send(event->peer, CHAN_SYNC,
                    enet_packet_create(buf+csize-j, j, ENET_PACKET_FLAG_RELIABLE));
                enet_host_service(g_netServer, NULL, 0);

                initprintf("Compressed %u bytes to %u\n", (uint32_t)sizeof(netmapstate_t), (uint32_t)qlz_size_compressed(buf));
            }
            else
                initprintf("Error compressing map state for transfer!\n");

            Bfree(buf);
        }
    }
}

static int32_t NewGameCommon(uint8_t *pbuf)
{
    int32_t i;

    if ((vote_map + vote_episode + voting) != -3)
        G_AddUserQuote("Vote Succeeded");

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
        g_player[i].revision = 0;
    }

    G_NewGame(ud.volume_number,ud.level_number,ud.player_skill);
    ud.coop = ud.m_coop;

    g_netMapRevision = 0;

    if (G_EnterLevel(MODE_GAME))
    {
        G_BackToMenu();
        return 1;
    }

    return 0;
}

static void ParsePacketCommon(uint8_t *pbuf, int32_t packbufleng, int32_t serverpacketp)
{
    int32_t i, j;
    int32_t other = pbuf[packbufleng];

    switch (pbuf[0])
    {
    case PACKET_MESSAGE:
        Bstrncpy(recbuf, (char *)pbuf+2, packbufleng-2);
        recbuf[packbufleng-2] = 0;

        G_AddUserQuote(recbuf);
        S_PlaySound(EXITMENUSOUND);

        pus = pub = NUMPAGES;
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

        if (serverpacketp)
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
            Bsprintf(tempbuf,"Confirmed vote from %s",g_player[(uint8_t)pbuf[1]].user_name);
            G_AddUserQuote(tempbuf);
        }
        break;

    case PACKET_MAP_VOTE_INITIATE: // call map vote
        voting = pbuf[1];
        vote_episode = pbuf[2];
        vote_map = pbuf[3];

        Bsprintf(tempbuf,"%s^00 has called a vote to change map to %s (E%dL%d)",
                 g_player[(uint8_t)pbuf[1]].user_name,
                 MapInfo[(uint8_t)(pbuf[2]*MAXLEVELS + pbuf[3])].name,
                 pbuf[2]+1,pbuf[3]+1);
        G_AddUserQuote(tempbuf);

        Bsprintf(tempbuf,"Press F1 to Accept, F2 to Decline");
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
                Bsprintf(tempbuf,"%s^00 has canceled the vote",g_player[(uint8_t)pbuf[1]].user_name);
            else Bsprintf(tempbuf,"Vote Failed");
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


void Net_ParseServerPacket(ENetEvent *event)
{
    uint8_t *pbuf = event->packet->data;
    int32_t packbufleng = event->packet->dataLength;
    int32_t i, j, l;
    input_t *nsyn;

    --packbufleng;  //    int32_t other = pbuf[--packbufleng];

#if 0
    initprintf("Received Packet: type: %d : len %d\n", pbuf[0], packbufleng);
#endif
    switch (pbuf[0])
    {
    case PACKET_MASTER_TO_SLAVE:  //[0] (receive master sync buffer)
        if (!(g_player[myconnectindex].ps->gm & MODE_GAME) || g_netSync) return;

        j = 0;

        packbufleng = qlz_size_decompressed((char *)&pbuf[1]);
        pbuf = (uint8_t *)Bcalloc(1, packbufleng+512);
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

            Bmemcpy(&g_player[i].ps->pos.x, &pbuf[j], sizeof(vec3_t) * 2);

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
                int32_t oa;

                j++;
                oa = T5;

                T5 = *(int32_t *)&pbuf[j];
                j += sizeof(int32_t);

                if (oa != T5) T3 = T4 = 0;
            }

            do
            {
                uint16_t var_id = *(uint16_t *)&pbuf[j];
                j += sizeof(int16_t);

                if (var_id == MAXGAMEVARS) break;

                aGameVars[var_id].val.plValues[i] = *(int32_t *)&pbuf[j];
                j += sizeof(int32_t);
            }
            while (1);

            i = l;

            do
            {
                uint16_t var_id = *(uint16_t *)&pbuf[j];
                j += sizeof(int16_t);

                if (var_id == MAXGAMEVARS) break;

                aGameVars[var_id].val.plValues[i] = *(int32_t *)&pbuf[j];
                j += sizeof(int32_t);
            }
            while (1);
        }

        Bfree(pbuf);

        break;

    case PACKET_MAP_STREAM:
        if (!(g_player[myconnectindex].ps->gm & MODE_GAME) || g_netSync)
            return;

        if (*(uint32_t *)&pbuf[1] != g_multiMapState[0]->revision)
        {
            initprintf("base revision mismatch, expected %d and found %d\n", g_player[myconnectindex].revision, *(uint32_t *)&pbuf[1]);
            return;
        }

        if (!streamoutput)
            streamoutput = (netmapstate_t *)Bcalloc(1, sizeof(netmapstate_t));

        {
            usize_t osize = 0;
            int ret;

            packbufleng = qlz_size_decompressed((char *)&pbuf[5]);
            pbuf = (uint8_t *)Bmalloc(packbufleng<<1);
            packbufleng = qlz_decompress((char *)&event->packet->data[5], (char *)(pbuf), state_decompress);

            initprintf("packbufleng: %d\n", packbufleng);

            ret = xd3_decode_memory((const uint8_t *)pbuf, packbufleng,
                (const uint8_t *)g_multiMapState[0], sizeof(netmapstate_t),
                (uint8_t *)streamoutput, &osize, sizeof(netmapstate_t), XD3_COMPLEVEL_1|XD3_NOCOMPRESS);

            if (ret)
            {
                initprintf("xdelta3 returned %d\n", ret);
                Bfree(pbuf);
                break;
            }

            if (sizeof(netmapstate_t) != osize)
                initprintf("decompressed data size mismatch!\n");
            else
            {
                Net_RestoreMapState(streamoutput);
                g_player[myconnectindex].revision = streamoutput->revision;
                Bmemcpy(g_multiMapState[0], streamoutput, sizeof(netmapstate_t));
            }

            Bfree(pbuf);
        }

        break;

    case PACKET_NEW_GAME:
        if (NewGameCommon(pbuf))
            break;

        if (g_netSync)
        {
            Net_RestoreMapState(g_multiMapState[0]);
            
            packbuf[0] = PACKET_PLAYER_READY;
            packbuf[1] = myconnectindex;

            if (g_netClientPeer)
                enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(packbuf, 2, ENET_PACKET_FLAG_RELIABLE));

            g_netSync = 0;

            g_player[myconnectindex].ps->gm = MODE_GAME;
            ready2send = 1;
        }
        break;

    case PACKET_VERSION:
        if (pbuf[1] != BYTEVERSION || pbuf[2] != (uint8_t)atoi(s_buildDate))
        {
            initprintf("Server protocol is version %d.%d, expecting %d.%d\n",
                       pbuf[1], pbuf[2], BYTEVERSION, (uint8_t)atoi(s_buildDate));
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

    default:
        ParsePacketCommon(pbuf, packbufleng, 1);
        break;
    }
}

void Net_ParseClientPacket(ENetEvent *event)
{
    uint8_t *pbuf = event->packet->data;
    int32_t packbufleng = event->packet->dataLength;
    int16_t j;
    int32_t other = pbuf[--packbufleng];
    input_t *nsyn;

#if 0
    initprintf("Received Packet: type: %d : len %d\n", pbuf[0], packbufleng);
#endif
    switch (pbuf[0])
    {
    case PACKET_SLAVE_TO_MASTER:  //[1] (receive slave sync buffer)
    {
        const int32_t playeridx = (int32_t)(intptr_t)event->peer->data;
        uint32_t rev;

        j = 0;

        packbufleng = qlz_size_decompressed((char *)&pbuf[1]);
        pbuf = (uint8_t *)Bcalloc(1, packbufleng+512);
        packbufleng = qlz_decompress((const char *)&event->packet->data[1], (char *)(pbuf), state_decompress);

        nsyn = (input_t *)&inputfifo[0][0];

        alloc_multimapstate(playeridx);

        rev = *(uint32_t *)&pbuf[j];

        if (g_player[playeridx].revision != rev)
        {
            g_player[playeridx].ready = 1;
            g_player[playeridx].revision = rev;
            if (rev != g_pendingMapState[playeridx]->revision)
                initprintf("wtf? expected confirmation of revision %d from player %d, got revision %d\n",
                g_pendingMapState[playeridx]->revision, playeridx, rev);
            else
            {
                    Bmemcpy(g_multiMapState[playeridx], g_pendingMapState[playeridx], sizeof(netmapstate_t));
                    initprintf("player %d now at base revision %d\n", playeridx, g_player[playeridx].revision);
            }
        }

        j += sizeof(uint32_t);

        // XXX: g_multiMapRevisions[...] is NULL when started like
        // (peer 1)$ eduke32 -server
        // (peer 2)$ eduke32 -connect localhost
        // (peer 1 starts some map)
//         Bmemcpy(g_multiMapState[playeridx], g_multiMapRevisions[g_player[other].revision&(NET_REVISIONS-1)], sizeof(netmapstate_t));

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
    }

    case PACKET_PLAYER_READY:
        j = g_player[other].ps->i;
        Bmemcpy(g_player[other].ps, g_player[0].ps, sizeof(DukePlayer_t));

        g_player[other].ps->i = j;
        changespritestat(j, STAT_PLAYER);

        g_player[other].ps->last_extra = sprite[g_player[other].ps->i].extra = g_player[other].ps->max_player_health;
        sprite[g_player[other].ps->i].cstat = 1+256;
        actor[g_player[other].ps->i].t_data[2] = actor[g_player[other].ps->i].t_data[3] = actor[g_player[other].ps->i].t_data[4] = 0;

        P_ResetPlayer(other);

        g_player[other].ready = 1;

        j = 0;
        packbuf[j++] = PACKET_PLAYER_SPAWN;
        packbuf[j++] = other;

        Bmemcpy(&packbuf[j], &g_player[other].ps->pos.x, sizeof(vec3_t) * 2);
        j += sizeof(vec3_t) * 2;

        packbuf[j++] = 0;

        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, j, ENET_PACKET_FLAG_RELIABLE));

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

    case PACKET_NEW_GAME:
        NewGameCommon(pbuf);
        return;

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

    default:
        ParsePacketCommon(pbuf, packbufleng, 0);
        break;
    }
}

static void display_betascreen(void)
{
    rotatesprite_fs(0,0,65536,0,BETASCREEN,0,0,2+8+16+64);

    rotatesprite_fs(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8);
    rotatesprite_fs(160<<16,(129)<<16,30<<11,0,THREEDEE,0,0,2+8);
    if (PLUTOPAK)   // JBF 20030804
        rotatesprite_fs(160<<16,(151)<<16,30<<11,0,PLUTOPAKSPRITE+1,0,0,2+8);
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
            const intptr_t playeridx = (intptr_t)event.peer->data;

            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
            {
                char ipaddr[32];

                enet_address_get_host_ip(&event.peer->address, ipaddr, sizeof(ipaddr));

                initprintf("A new client connected from %s:%u.\n", ipaddr, event.peer->address.port);

                Net_SendVersion(event.peer);
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
                packbuf[4] = playerswhenstarted;
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
                            buf = (uint8_t *)Bcalloc(1, sizeof(netmapstate_t)+512);
                            if (buf == NULL)  // TODO: G_GameExit should send a "bye" packet?
                                G_GameExit("OUT OF MEMORY in Net_GetPackets:sync");
                        }

                        alloc_multimapstate(0);

                        display_betascreen();

                        Bassert(datasiz+event.packet->dataLength <= sizeof(netmapstate_t)+512);

                        if (event.packet->dataLength == SYNCPACKETSIZE)
                        {
                            char tbuf[64];

                            Bmemcpy(buf+datasiz, event.packet->data, SYNCPACKETSIZE);
                            datasiz += SYNCPACKETSIZE;

                            Bsprintf(tbuf, "Received %d bytes\n", (int32_t)datasiz);
                            gametext(160,190,tbuf,14,2);
                        }
                        else
                        {
                            // last packet of mapstate sequence

                            Bmemcpy(buf+datasiz, event.packet->data, event.packet->dataLength);
                            datasiz = 0;

                            if (qlz_size_decompressed((const char *)buf) == sizeof(netmapstate_t))
                            {
                                qlz_decompress((const char *)buf, g_multiMapState[0], state_decompress);

                                packbuf[0] = PACKET_REQUEST_GAMESTATE;
                                packbuf[1] = myconnectindex;
                                enet_peer_send(g_netClientPeer, CHAN_GAMESTATE,
                                    enet_packet_create(&packbuf[0], 2, ENET_PACKET_FLAG_RELIABLE));

                                gametext(160,190,"Transfer Complete",14,2);
                            }
                            else
                            {
                                initprintf("Invalid map state from server!  Decompressed to %u bytes, expected %u.\n",
                                    (uint32_t)qlz_size_decompressed((char *)buf), (uint32_t)sizeof(netmapstate_t));

                                g_netDisconnect = 1;
                                g_netSync = 0;

                                gametext(160,190,"Transfer Error",14,2);
                            }

                            Bfree(buf);
                            buf = NULL;
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
    int32_t siz = 1;
    input_t *nsyn = (input_t *)&inputfifo[0][myconnectindex];

    packbuf[0] = PACKET_SLAVE_TO_MASTER;

    *(uint32_t *)&packbuf[siz] = g_player[myconnectindex].revision;
    siz += sizeof(uint32_t);

    Bmemcpy(&packbuf[siz], &nsyn[0], offsetof(input_t, filler));
    siz += offsetof(input_t, filler);

    Bmemcpy(&packbuf[siz], &g_player[myconnectindex].ps->pos.x, sizeof(vec3_t) * 2);
    siz += sizeof(vec3_t) * 2;

    Bmemcpy(&packbuf[siz], &g_player[myconnectindex].ps->vel.x, sizeof(vec3_t));
    siz += sizeof(vec3_t);

    *(int16_t *)&packbuf[siz] = g_player[myconnectindex].ps->ang;
    siz += sizeof(int16_t);

    Bmemcpy(&packbuf[siz], &g_player[myconnectindex].ps->horiz, sizeof(int16_t) * 2);
    siz += sizeof(int16_t) * 2;

    {
        char buf[1024];

        Bassert((signed)sizeof(buf) >= siz+400);
        siz = qlz_compress(packbuf+1, buf, siz, state_compress);
        Bmemcpy(packbuf+1, buf, siz);
        siz++;
    }

    packbuf[siz++] = myconnectindex;

    enet_peer_send(g_netClientPeer, CHAN_MOVE, enet_packet_create(packbuf, siz, 0));

}

char g_netCompressBuf[PACKBUF_SIZE+400];

void Net_UpdateClients(void)
{
    input_t *osyn = (input_t *)&inputfifo[1][0];
    input_t *nsyn = (input_t *)&inputfifo[0][0];
    int16_t i, l;
    int32_t siz = 1;

    if (!g_netServer || numplayers < 2)
    {
        ticrandomseed = randomseed;
        if (g_netServer)
            Bmemcpy(&osyn[0], &nsyn[0], sizeof(input_t));
        return;
    }

    packbuf[0] = PACKET_MASTER_TO_SLAVE;

    *(int32_t *)&packbuf[siz] = ticrandomseed = randomseed;
    siz += sizeof(int32_t);
    packbuf[siz++] = ud.pause_on;

    for (TRAVERSE_CONNECT(i))
    {
        Bmemcpy(&osyn[i], &nsyn[i], offsetof(input_t, filler));

        *(int16_t *)&packbuf[siz] = g_player[i].ps->dead_flag;
        siz += sizeof(int16_t);

        packbuf[siz++] = g_player[i].playerquitflag;

        if (g_player[i].playerquitflag == 0) continue;

        Bmemcpy(&packbuf[siz], &nsyn[i], offsetof(input_t, filler));
        siz += offsetof(input_t, filler);

        Bmemcpy(&packbuf[siz], &g_player[i].ps->pos.x, sizeof(vec3_t) * 2);
        siz += sizeof(vec3_t) * 2;

        Bmemcpy(&packbuf[siz], &g_player[i].ps->vel.x, sizeof(vec3_t));
        siz += sizeof(vec3_t);

        *(int16_t *)&packbuf[siz] = g_player[i].ps->ang;
        siz += sizeof(int16_t);

        Bmemcpy(&packbuf[siz], &g_player[i].ps->horiz, sizeof(int16_t) * 2);
        siz += sizeof(int16_t) * 2;

        *(uint16_t *)&packbuf[siz] = g_player[i].ps->gotweapon;
        siz += sizeof(uint16_t);

        Bmemcpy(&packbuf[siz], &g_player[i].ps->ammo_amount[0], sizeof(g_player[i].ps->ammo_amount));
        siz += sizeof(g_player[i].ps->ammo_amount);

        Bmemcpy(&packbuf[siz], &g_player[i].ps->inv_amount[0], sizeof(g_player[i].ps->inv_amount));
        siz += sizeof(g_player[i].ps->inv_amount);

        Bmemcpy(&packbuf[siz], g_player[i].frags, sizeof(g_player[i].frags));
        siz += sizeof(g_player[i].frags);

        packbuf[siz++] = (uint8_t) sprite[g_player[i].ps->i].extra;

        *(int16_t *)&packbuf[siz] = sprite[g_player[i].ps->i].cstat;
        siz += sizeof(int16_t);

        packbuf[siz++] = (uint8_t) g_player[i].ps->kickback_pic;

        *(int16_t *)&packbuf[siz] = actor[g_player[i].ps->i].owner;
        siz += sizeof(int16_t);

        *(int16_t *)&packbuf[siz] = actor[g_player[i].ps->i].picnum;
        siz += sizeof(int16_t);

        packbuf[siz++] = (uint8_t) g_player[i].ps->curr_weapon;
        packbuf[siz++] = (int8_t) g_player[i].ps->last_weapon;
        packbuf[siz++] = (int8_t) g_player[i].ps->wantweaponfire;
        packbuf[siz++] = (int8_t) g_player[i].ps->weapon_pos;
        packbuf[siz++] = (uint8_t) g_player[i].ps->frag_ps;

        packbuf[siz++] = g_player[i].ps->frag;
        packbuf[siz++] = g_player[i].ps->fraggedself;
        packbuf[siz++] = g_player[i].ps->last_extra;

        *(int16_t *)&packbuf[siz] = g_player[i].ping;
        siz += sizeof(int16_t);

        *(int16_t *)&packbuf[siz] = g_player[i].ps->newowner;
        siz += sizeof(int16_t);

        packbuf[siz++] = sprite[g_player[i].ps->i].pal;

        l = i;
        i = g_player[l].ps->i;

        {
            packbuf[siz++] = 0;

            *(int32_t *)&packbuf[siz] = T5;

            siz += sizeof(int32_t);
        }

        {
            int16_t ii=g_gameVarCount-1;

            for (; ii>=0 && siz <= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)); ii--)
            {
                if ((aGameVars[ii].dwFlags & (GAMEVAR_PERACTOR|GAMEVAR_NOMULTI)) == GAMEVAR_PERACTOR && aGameVars[ii].val.plValues &&
                    aGameVars[ii].val.plValues[i] != aGameVars[ii].lDefault)
                {
                    *(int16_t *)&packbuf[siz] = ii;
                    siz += sizeof(int16_t);
                    *(int32_t *)&packbuf[siz] = aGameVars[ii].val.plValues[i];
                    siz += sizeof(int32_t);

                    if (siz >= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)))
                        break;
                }
            }
            *(int16_t *)&packbuf[siz] = MAXGAMEVARS;
            siz += sizeof(int16_t);
        }

        i = l;

        {
            int16_t ii=g_gameVarCount-1;

            for (; ii>=0 && siz <= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)); ii--)
            {
                if ((aGameVars[ii].dwFlags & (GAMEVAR_PERPLAYER|GAMEVAR_NOMULTI)) == GAMEVAR_PERPLAYER && aGameVars[ii].val.plValues &&
                    aGameVars[ii].val.plValues[i] != aGameVars[ii].lDefault)
                {
                    *(int16_t *)&packbuf[siz] = ii;
                    siz += sizeof(int16_t);
                    *(int32_t *)&packbuf[siz] = aGameVars[ii].val.plValues[i];
                    siz += sizeof(int32_t);

                    if (siz >= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)))
                        break;
                }
            }
            *(int16_t *)&packbuf[siz] = MAXGAMEVARS;
            siz += sizeof(int16_t);
        }
    }

    {
#if 1
        // We're screwed anyway if this fails:
        Bassert(siz <= PACKBUF_SIZE);
#else
        if (siz >= PACKBUF_SIZE)
        {
            initprintf("Global packet buffer overflow! Size of packet: %i\n", siz);
            return;
        }
#endif
        siz = qlz_compress(packbuf+1, g_netCompressBuf, siz, state_compress);

        if (siz >= PACKBUF_SIZE-1)
        {
            initprintf("Packet buffer overflow! Size of compressed MOVE payload: %d\n", siz);
            return;
        }

        Bmemcpy(packbuf+1, g_netCompressBuf, siz);
        siz++;
    }

    packbuf[siz++] = myconnectindex;

    enet_host_broadcast(g_netServer, CHAN_MOVE, enet_packet_create(packbuf, siz, 0));
}

void Net_StreamLevel(void)
{
    int32_t pi;
    int32_t siz = 0;
    usize_t osize = sizeof(netmapstate_t);

    if (!g_netServer || numplayers < 2)
        return;

    if (!streamoutput)
        streamoutput = (netmapstate_t *)Bcalloc(1, sizeof(netmapstate_t));

    for (pi=0; pi<(signed)g_netServer->peerCount; pi++)
    {
        ENetPeer *const currentPeer = &g_netServer->peers[pi];
        const intptr_t playeridx = (intptr_t)currentPeer->data;

        if (currentPeer->state != ENET_PEER_STATE_CONNECTED || !g_player[playeridx].playerquitflag || !g_player[playeridx].ready)
            continue;

        g_player[playeridx].ready = 0;

        packbuf[0] = PACKET_MAP_STREAM;
        *(uint32_t *)&packbuf[1] = g_player[playeridx].revision; // base revision for this update

        if (g_multiMapRevisions[g_netMapRevision&(NET_REVISIONS-1)] == NULL)
            g_multiMapRevisions[g_netMapRevision&(NET_REVISIONS-1)] = (netmapstate_t *)Bcalloc(1, sizeof(netmapstate_t));

        Net_SaveMapState(g_multiMapRevisions[g_netMapRevision&(NET_REVISIONS-1)]);

        xd3_encode_memory((const uint8_t *)g_multiMapRevisions[g_netMapRevision&(NET_REVISIONS-1)], sizeof(netmapstate_t),
            (const uint8_t *)g_multiMapState[playeridx], sizeof(netmapstate_t),
            (uint8_t *)streamoutput, &osize, sizeof(netmapstate_t), XD3_COMPLEVEL_1|XD3_NOCOMPRESS);

        Bmemcpy(g_pendingMapState[playeridx], g_multiMapRevisions[g_netMapRevision&(NET_REVISIONS-1)], sizeof(netmapstate_t));

        initprintf("upd %u->%u: ", g_player[playeridx].revision, g_netMapRevision);

        {
            if (osize >= PACKBUF_SIZE)
            {
                // XXX: this currently happens when e.g. switching levels
                initprintf("Packet buffer overflow! GAMESTATE payload size "
                           "after diff before compress: %u\n", osize);
                return;
            }

            siz = qlz_compress((char *)streamoutput, g_netCompressBuf, osize, state_compress);

            if (siz >= PACKBUF_SIZE-1)
            {
                initprintf("Global packet buffer overflow! GAMESTATE payload size "
                           "after diff and compress: %d\n", siz);
                return;
            }

            Bmemcpy(packbuf+1+sizeof(uint32_t), g_netCompressBuf, siz);
            siz++;
            siz += sizeof(uint32_t);
        }

        packbuf[siz++] = myconnectindex;

        initprintf("final packet size: %d\n", siz);

        enet_peer_send(currentPeer, CHAN_GAMESTATE, enet_packet_create(packbuf, siz, ENET_PACKET_FLAG_RELIABLE));
    }

    g_netMapRevision++;
}

void faketimerhandler(void)
{
    if (g_netServer==NULL && g_netClient==NULL)
        return;

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
            gametext(320>>1,j,"Send message to...",0,2+8+16);
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

    P_SetGamePalette(g_player[myconnectindex].ps, TITLEPAL, 8+2+1);

    do
    {
        if (quitevent || keystatus[1]) G_GameExit("");

        display_betascreen();

        gametext(160,170,"Waiting for server",14,2);
        nextpage();

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

