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
char tempnetbuf[sizeof(netmapstate_t) + 400];
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
            for (j=0; j<10; j++)
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
    int32_t i;
    // int32_t x; // used in commented code below
    intptr_t j;
    char phealth[MAXPLAYERS];

    if (save == NULL)
    {
        return;
    }

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
        for (j=0; j<10; j++)
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
    {
        if (g_player[i].playerquitflag == 0)
        {
            break;
        }
    }

    if (i == -1)
    {
        i = playerswhenstarted++;
    }

    event->peer->data = (void *)(intptr_t)i;

    g_player[i].netsynctime = totalclock;
    g_player[i].playerquitflag = 1;
    g_player[i].revision = g_netMapRevision;

    for (j=0; j<playerswhenstarted-1; j++)
    {
        connectpoint2[j] = j+1;
    }

    connectpoint2[playerswhenstarted-1] = -1;

    if (!g_player[i].ps)
    {
        g_player[i].ps = (DukePlayer_t *) Bcalloc(1, sizeof(DukePlayer_t));
    }

    if (!g_player[i].sync)
    {
        g_player[i].sync = (input_t *) Bcalloc(1, sizeof(input_t));
    }

    ++numplayers;
    ++ud.multimode;
    Net_SendNewPlayer(i);
    Net_SendPlayerIndex(i, event->peer);
    Net_SendClientInfo();
    Net_SendUserMapName();
    Net_SendClientSync(event, i);
}

static void display_betascreen(void)
{
    rotatesprite_fs(0,0,65536,0,BETASCREEN,0,0,2+8+16+64);

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

////////////////////////////////////////////////////////////////////////////////
// Connect/Disconnect

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
    }
}

void Net_ReceiveDisconnect(ENetEvent *event)
{
    g_netDisconnect = 1;
    numplayers = playerswhenstarted = ud.multimode = 1;
    myconnectindex = screenpeek = 0;
    G_BackToMenu();

    switch (event->data)
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
}

////////////////////////////////////////////////////////////////////////////////
// Packet Handlers

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

void Net_HandleClientPackets(void)
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
            if (event.channelID == CHAN_SYNC)
            {
                Net_ReceiveClientSync(&event);
            }
            else
            {
                Net_ParseServerPacket(&event);
            }
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
        break;

    case PACKET_AUTH:
        Net_RecieveChallenge(pbuf, packbufleng, event);
        break;

    case PACKET_REQUEST_GAMESTATE:

        if (g_netServer && g_player[0].ps->gm & MODE_GAME)
        {
            Net_SendNewGame(0, event->peer);
        }

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
    int32_t i, j, l;
    input_t *nsyn;

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

        if (*(uint32_t *)&pbuf[1] != g_multiMapState[0]->revision)
        {
            initprintf("base revision mismatch, expected %d and found %d\n", g_player[myconnectindex].revision, *(uint32_t *)&pbuf[1]);
            return;
        }

        Net_ReceiveMapUpdate(pbuf, packbufleng);

        break;

    case PACKET_NEW_GAME:
        Net_ReceiveNewGame(event);
        break;

    case PACKET_VERSION:
        Net_RecieveVersion(pbuf, packbufleng);
        break;

    case PACKET_NUM_PLAYERS:
        Net_RecieveNewPlayer(event->packet->data, event->packet->dataLength);
        break;

    case PACKET_PLAYER_INDEX:
        Net_RecievePlayerIndex(event->packet->data, event->packet->dataLength);
        break;

    case PACKET_PLAYER_DISCONNECTED:
        if ((g_player[myconnectindex].ps->gm & MODE_GAME))
            P_RemovePlayer(pbuf[1]);
        numplayers = pbuf[2];
        ud.multimode = pbuf[3];
        playerswhenstarted = pbuf[4];
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
        ticrandomseed = *(int32_t *)&pbuf[4];
        P_FragPlayer(pbuf[1]);
        break;

    default:
        Net_ParsePacketCommon(pbuf, packbufleng, 1);
        break;
    }
}

void Net_ParsePacketCommon(uint8_t *pbuf, int32_t packbufleng, int32_t serverpacketp)
{
    int32_t i, j;

    switch (pbuf[0])
    {
    case PACKET_MESSAGE:
        Net_ReceiveMessage(pbuf, packbufleng);
        break;

    case PACKET_CLIENT_INFO:
        Net_ReceiveClientInfo(pbuf, packbufleng, serverpacketp);
        break;

    case PACKET_RTS:
        if (rts_numlumps == 0) break;

        if (ud.config.SoundToggle == 0 || ud.lockout == 1 || ud.config.FXDevice < 0 || !(ud.config.VoiceToggle & 4))
            break;

        FX_PlayAuto3D((char *)RTS_GetSound(pbuf[1]-1),RTS_SoundLength(pbuf[1]-1),0,0,0,255,-pbuf[1]);
        g_RTSPlaying = 7;
        break;

    case PACKET_USER_MAP:
        Net_ReceiveUserMapName(pbuf, packbufleng);
        break;

    case PACKET_MAP_VOTE:
        Net_RecieveMapVote(pbuf);
        break;

    case PACKET_MAP_VOTE_INITIATE: // call map vote
        Net_RecieveMapVoteInitiate(pbuf);
        break;

    case PACKET_MAP_VOTE_CANCEL: // cancel map vote
        Net_RecieveMapVoteCancel(pbuf);
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Version Packets

void Net_SendVersion(ENetPeer *client)
{
    if (!g_netServer)
    {
        return;
    }

    buf[0] = PACKET_VERSION;
    buf[1] = BYTEVERSION;
    // XXX: s_buildDate is outdated and useless; uint8 is not enough :/
    buf[2] = (uint8_t)atoi(s_buildDate);
    buf[3] = myconnectindex;

    enet_peer_send(client, CHAN_GAMESTATE, enet_packet_create(&buf[0], 4, ENET_PACKET_FLAG_RELIABLE));
}

void Net_RecieveVersion(uint8_t *pbuf, int32_t packbufleng)
{
    if (pbuf[1] != BYTEVERSION || pbuf[2] != (uint8_t)atoi(s_buildDate))
    {
        initprintf("Server protocol is version %d.%d, expecting %d.%d\n",
                   pbuf[1], pbuf[2], BYTEVERSION, (uint8_t)atoi(s_buildDate));
        initprintf("Server version mismatch!  You cannot play Duke with different versions!\n");
        g_netDisconnect = 1;
        return;
    }

    Net_SendChallenge();
}

////////////////////////////////////////////////////////////////////////////////
// Challenge Packets

// sends a simple crc32 of the current password, verified by the server before the connection can continue
void Net_SendChallenge(ENetPeer *client)
{
    int32_t l = 1;

    if (!g_netClientPeer)
    {
        return;
    }

    buf[0] = PACKET_AUTH;
    *(uint32_t *)&buf[1] = crc32once((uint8_t *)g_netPassword, Bstrlen(g_netPassword));
    l += sizeof(int32_t);

    buf[l++] = myconnectindex;

    enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&buf[0], l, ENET_PACKET_FLAG_RELIABLE));
}

void Net_RecieveChallenge(uint8_t *pbuf, int32_t packbufleng, ENetEvent *event)
{
    uint32_t crc = *(uint32_t *)&pbuf[1];

    if (crc == crc32once((uint8_t *)g_netPassword, Bstrlen(g_netPassword)))
    {
        Net_SyncPlayer(event);
    }
    else
    {
        enet_peer_disconnect_later(event->peer, DISC_BAD_PASSWORD);
        initprintf("Bad password from client.\n");
    }
}

////////////////////////////////////////////////////////////////////////////////
// Num Players Packets

void Net_SendNewPlayer(int32_t newplayerindex)
{
    packbuf[0] = PACKET_NUM_PLAYERS;
    packbuf[1] = numplayers;
    packbuf[2] = playerswhenstarted;
    packbuf[3] = ud.multimode;
    packbuf[4] = newplayerindex;
    packbuf[5] = g_networkMode;
    packbuf[6] = myconnectindex;
    enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, 7, ENET_PACKET_FLAG_RELIABLE));
}

void Net_RecieveNewPlayer(uint8_t *pbuf, int32_t packbufleng)
{
    int32_t i;

    numplayers = pbuf[1];
    playerswhenstarted = pbuf[2];
    ud.multimode = pbuf[3];
    if (pbuf[4]) // ID of new player
    {
        g_player[pbuf[4]].playerquitflag = 1;

        if (!g_player[pbuf[4]].ps)
        {
            g_player[pbuf[4]].ps = (DukePlayer_t *) Bcalloc(1,sizeof(DukePlayer_t));
        }
        if (!g_player[pbuf[4]].sync)
        {
            g_player[pbuf[4]].sync = (input_t *) Bcalloc(1,sizeof(input_t));
        }
    }

    if (pbuf[5] == NET_DEDICATED_SERVER)
    {
        g_networkMode = NET_DEDICATED_CLIENT;
    }

    for (i=0; i<playerswhenstarted-1; i++)
    {
        connectpoint2[i] = i+1;
    }

    connectpoint2[playerswhenstarted-1] = -1;

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

void Net_RecievePlayerIndex(uint8_t *pbuf, int32_t packbufleng)
{
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
        g_player[myconnectindex].user_name[l] = Btoupper(szPlayerName[l]);

    if (numplayers < 2) return;

    buf[0] = PACKET_CLIENT_INFO;
    l = 1;

    //null terminated player name to send
    for (i=0; szPlayerName[i]; i++)
    {
        buf[l++] = Btoupper(szPlayerName[i]);
    }
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
    {
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&buf[0], l, ENET_PACKET_FLAG_RELIABLE));
    }
    else if (g_netServer)
    {
        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(&buf[0], l, ENET_PACKET_FLAG_RELIABLE));
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
// Client Sync Packets

void Net_SendClientSync(ENetEvent *event, int32_t player)
{
    int32_t j;
    int32_t i = player;

    if ((g_player[0].ps->gm & MODE_GAME) == 0)
    {
        return;
    }

    alloc_multimapstate(i);

    sprite[g_player[i].ps->i].cstat = 32768;
    g_player[i].ps->runspeed = g_playerFriction;
    g_player[i].ps->last_extra = sprite[g_player[i].ps->i].extra = g_player[i].ps->max_player_health = g_maxPlayerHealth;

    Net_SaveMapState(g_multiMapState[i]);
    if ((j = qlz_compress((char *)g_multiMapState[i], tempnetbuf, sizeof(netmapstate_t), state_compress)))
    {
        enet_peer_send(event->peer, CHAN_SYNC, enet_packet_create(tempnetbuf, j, ENET_PACKET_FLAG_RELIABLE));
        enet_host_service(g_netServer, NULL, 0);

        initprintf("Compressed %u bytes to %u\n", (uint32_t)sizeof(netmapstate_t), (uint32_t)qlz_size_compressed(tempnetbuf));
    }
    else
    {
        initprintf("Error compressing map state for transfer!\n");
    }
}

void Net_ReceiveClientSync(ENetEvent *event)
{
    const char *pbuf = (const char *) event->packet->data;
    if (qlz_size_decompressed(pbuf) == sizeof(netmapstate_t))
    {
        alloc_multimapstate(0);
        qlz_decompress((const char *)pbuf, g_multiMapState[0], state_decompress);

        packbuf[0] = PACKET_REQUEST_GAMESTATE;
        packbuf[1] = myconnectindex;
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&packbuf[0], 2, ENET_PACKET_FLAG_RELIABLE));

        gametext(160,190,"Transfer Complete",14,2);
    }
    else
    {
        initprintf("Invalid map state from server!  Decompressed to %u bytes, expected %u.\n",
                   (uint32_t)qlz_size_decompressed((char *)buf), (uint32_t)sizeof(netmapstate_t));

        g_netDisconnect = 1;

        gametext(160,190,"Transfer Error",14,2);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Map Update Packets

char g_netCompressBuf[PACKBUF_SIZE+400];

void Net_SendMapUpdate(void)
{
    int32_t pi;
    int32_t siz = 0;
    netmapstate_t *currentstate;
    usize_t osize = sizeof(netmapstate_t);

    if (!g_netServer || numplayers < 2)
        return;

    currentstate = g_multiMapRevisions[g_netMapRevision % NET_REVISIONS];

    if (currentstate == NULL)
    {
        currentstate = (netmapstate_t *) Bcalloc(1, sizeof(netmapstate_t));
        g_multiMapRevisions[g_netMapRevision % NET_REVISIONS] = currentstate;
    }

    Net_SaveMapState(currentstate);

    for (pi=0; pi<(signed)g_netServer->peerCount; pi++)
    {
        ENetPeer *const currentPeer = &g_netServer->peers[pi];
        const intptr_t playeridx = (intptr_t)currentPeer->data;

        if (currentPeer->state != ENET_PEER_STATE_CONNECTED || !g_player[playeridx].playerquitflag)
        {
            continue;
        }

        if (g_player[playeridx].revision == g_netMapRevision)
        {
            continue;
        }

        packbuf[0] = PACKET_MAP_STREAM;
        *(uint32_t *)&packbuf[1] = g_player[playeridx].revision; // base revision for this update

        xd3_encode_memory((const uint8_t *) currentstate, sizeof(netmapstate_t),
                          (const uint8_t *) g_multiMapState[playeridx], sizeof(netmapstate_t),
                          (uint8_t *)tempnetbuf, &osize, sizeof(netmapstate_t), XD3_COMPLEVEL_1|XD3_NOCOMPRESS);

        Bmemcpy(g_pendingMapState[playeridx], currentstate, sizeof(netmapstate_t));

        siz = qlz_compress((char *)tempnetbuf, packbuf+1+sizeof(uint32_t), osize, state_compress);

        siz++; // Because of the packet tag
        siz += sizeof(uint32_t); // Because of the revision

        packbuf[siz++] = myconnectindex;

        initprintf("final packet size: %d\n", siz);

        enet_peer_send(currentPeer, CHAN_GAMESTATE, enet_packet_create(packbuf, siz, ENET_PACKET_FLAG_RELIABLE));
    }

    g_netMapRevision++;
}

void Net_ReceiveMapUpdate(uint8_t *pbuf, int32_t packbufleng)
{
    int ret;
    usize_t osize = 0;
    netmapstate_t *receivedstate;

    packbufleng = qlz_size_decompressed((char *)&pbuf[5]);
    pbuf = (uint8_t *)Bmalloc(packbufleng<<1);
    packbufleng = qlz_decompress((char *)&pbuf[4], (char *)(pbuf), state_decompress);

    initprintf("packbufleng: %d\n", packbufleng);

    ret = xd3_decode_memory((const uint8_t *)pbuf, packbufleng,
                            (const uint8_t *)g_multiMapState[0], sizeof(netmapstate_t),
                            (uint8_t *)tempnetbuf, &osize, sizeof(netmapstate_t), XD3_COMPLEVEL_1|XD3_NOCOMPRESS);

    if (ret)
    {
        initprintf("xdelta3 returned %d\n", ret);
        Bfree(pbuf);
        return;
    }

    if (sizeof(netmapstate_t) != osize)
    {
        initprintf("decompressed data size mismatch!\n");
    }
    else
    {
        receivedstate = (netmapstate_t *) tempnetbuf;
        Net_RestoreMapState(receivedstate);
        g_player[myconnectindex].revision = receivedstate->revision;
        Bmemcpy(g_multiMapState[0], receivedstate, sizeof(netmapstate_t));
    }

    Bfree(pbuf);
}

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

void Net_ExtractPlayerUpdate(playerupdate_t *update)
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

    g_player[playerindex].ping = update->ping;
    g_player[playerindex].ps->dead_flag = update->deadflag;
    g_player[playerindex].playerquitflag = update->playerquitflag;

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
    char *updatebuf;
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

    if ((ticrandomseed % 5) == 0)
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

    /*

    for (TRAVERSE_CONNECT(i))
    {

        l = i;
        i = g_player[l].ps->i;

        {
            packbuf[siz++] = 0;

            *(int32_t *)&packbuf[siz] = T5;

            siz += sizeof(int32_t);
        }

        {
            int16_t ii=g_gameVarCount-1;

            for (; ii>=0 && siz <= (SYNCPACKETSIZE-(SYNCPACKETSIZE>>3)); ii--) // SYNCPACKETSIZE-(SYNCPACKETSIZE>>3) == 1176
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

        siz = qlz_compress(packbuf+1, compressbuf, siz, state_compress);

        if (siz >= PACKBUF_SIZE-1)
        {
            initprintf("Packet buffer overflow! Size of compressed MOVE payload: %d\n", siz);
            return;
        }

        Bmemcpy(packbuf+1, compressbuf, siz);
        siz++;
    }

    packbuf[siz++] = myconnectindex;

    enet_host_broadcast(g_netServer, CHAN_MOVE, enet_packet_create(packbuf, siz, 0));
    */
}

void Net_ReceiveServerUpdate(ENetEvent *event)
{
    int32_t i;
    char *updatebuf;
    int8_t numupdates;
    serverupdate_t serverupdate;
    serverplayerupdate_t playerupdate;

    if (((event->packet->dataLength - sizeof(serverupdate_t)) % sizeof(serverplayerupdate_t)) != 0)
    {
        return;
    }

    updatebuf = (char *) event->packet->data;
    Bmemcpy(&serverupdate, updatebuf, sizeof(serverupdate_t));
    updatebuf += sizeof(serverupdate_t);
    inputfifo[0][0] = serverupdate.nsyn;
    ud.pause_on = serverupdate.pause_on;

    ticrandomseed = serverupdate.seed;

    for (i = 0; i < serverupdate.numplayers; ++i)
    {
        Bmemcpy(&playerupdate, updatebuf, sizeof(serverplayerupdate_t));
        updatebuf += sizeof(serverplayerupdate_t);

        Net_ExtractPlayerUpdate(&playerupdate.player);

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

        /*
        if (g_player[i].ps->newowner == -1 && g_player[i].ps->cursectnum >= 0 && g_player[i].ps->cursectnum < numsectors)
        {
        	updatesectorz(g_player[i].ps->pos.x, g_player[i].ps->pos.y, g_player[i].ps->pos.z,
        	&g_player[i].ps->cursectnum);
        	changespritesect(g_player[i].ps->i, g_player[i].ps->cursectnum);
        }
        */
    }

    /*
    for (TRAVERSE_CONNECT(i))
    {
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
    */
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

    g_player[playeridx].revision = update.revision;
    inputfifo[0][playeridx] = update.nsyn;

    Net_ExtractPlayerUpdate(&update.player);
}

////////////////////////////////////////////////////////////////////////////////
// Message Packets

void Net_SendMessage(void)
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

    g_netPlayersWaiting--;
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
    int32_t i;
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

void Net_RecieveMapVoteInitiate(uint8_t *pbuf)
{
    int32_t i;

    Bmemcpy(&pendingnewgame, pbuf, sizeof(newgame_t));
    Net_ExtractNewGame(&pendingnewgame, 1);

    voting = pendingnewgame.connection;
    vote_episode = pendingnewgame.volume_number;
    vote_map = pendingnewgame.level_number;

    Bsprintf(tempbuf,"%s^00 has called a vote to change map to %s (E%dL%d)",
             g_player[voting].user_name,
             MapInfo[(uint8_t)(vote_episode*MAXLEVELS + vote_map)].name,
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

void Net_RecieveMapVote(uint8_t *pbuf)
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
    int32_t i;

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

void Net_RecieveMapVoteCancel(uint8_t *pbuf)
{
    int32_t i, numvotes;

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

