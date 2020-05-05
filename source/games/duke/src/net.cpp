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
#include "ns.h"	// Must come before everything else!

#ifndef NETWORK_DISABLE
#include "enet.h"
#endif

#include "duke3d.h"
#include "game.h"
#include "gamedef.h"
#include "net.h"
#include "premap.h"
#include "savegame.h"

#include "m_crc32.h"
#include "mapinfo.h"

BEGIN_DUKE_NS

#define TIMERUPDATESIZ 32

ENetHost *g_netServer = NULL;
ENetHost *g_netClient = NULL;
ENetPeer *g_netClientPeer = NULL;
ENetPeer* g_netPlayerPeer[MAXPLAYERS];
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


// sync a connecting player up with the current game state
void Net_SyncPlayer(ENetEvent *event)
{
    int32_t i, j;

    if (numplayers + g_netPlayersWaiting >= MAXPLAYERS)
    {
        enet_peer_disconnect_later(event->peer, DISC_SERVER_FULL);
        Printf("Refused peer; server full.\n");
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

    //g_player[i].netsynctime = totalclock;
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
   // Net_SendNewGame(0, event->peer);
}

//void Net_SpawnPlayer(int32_t player)
//{
//    int32_t j = 0;
//    packbuf[j++] = PACKET_PLAYER_SPAWN;
//    packbuf[j++] = player;
//
//    Bmemcpy(&packbuf[j], &g_player[player].ps->pos.x, sizeof(vec3_t) * 2);
//    j += sizeof(vec3_t) * 2;
//
//    packbuf[j++] = 0;
//
//    enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, j, ENET_PACKET_FLAG_RELIABLE));
//}

static void display_betascreen(void)
{
    rotatesprite_fs(160<<16,100<<16,65536,0,TILE_BETASCREEN,0,0,2+8+64+BGSTRETCH, nullptr, TITLEPAL);

    rotatesprite_fs(160<<16,(104)<<16,60<<10,0,TILE_DUKENUKEM,0,0,2+8, nullptr, TITLEPAL);
    rotatesprite_fs(160<<16,(129)<<16,30<<11,0,TILE_THREEDEE,0,0,2+8, nullptr, TITLEPAL);
    if (PLUTOPAK)   // JBF 20030804
        rotatesprite_fs(160<<16,(151)<<16,30<<11,0,TILE_PLUTOPAKSPRITE+1,0,0,2+8, nullptr, TITLEPAL);
}

void faketimerhandler(void)
{
    if (g_netServer==NULL && g_netClient==NULL)
        return;

    enet_host_service(g_netServer ? g_netServer : g_netClient, NULL, 0);

    //Net_Update();
}

void Net_WaitForEverybody(void)
{
    if (numplayers < 2) return;

    packbuf[0] = PACKET_TYPE_PLAYER_READY;
    g_player[myconnectindex].playerreadyflag++;

    // if we're a peer or slave, not a master
    if ((g_networkBroadcastMode == 1) || (!g_networkBroadcastMode && (myconnectindex != connecthead)))
        for (int TRAVERSE_CONNECT(i))
    {
        if (i != myconnectindex) Net_SendPacket(i, packbuf, 1);
        if ((!g_networkBroadcastMode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
    }


    do
    {
        if (G_FPSLimit())
        {
            display_betascreen();
            gametext_center_shade(170, "Waiting for players", 14);
            videoNextPage();
        };

        G_HandleAsync();
        Net_GetPackets();

        int i;
        for (TRAVERSE_CONNECT(i))
        {
            if (g_player[i].playerreadyflag < g_player[myconnectindex].playerreadyflag) break;
            if ((!g_networkBroadcastMode) && (myconnectindex != connecthead))
            {
                // we're a slave
                i = -1; break;
            }
            //slaves in M/S mode only wait for master
        }
        if (i < 0)
        {
            // master sends ready packet once it hears from all slaves
            if (!g_networkBroadcastMode && myconnectindex == connecthead)
                for (int TRAVERSE_CONNECT(i))
            {
                packbuf[0] = PACKET_TYPE_PLAYER_READY;
                if (i != myconnectindex) Net_SendPacket(i, packbuf, 1);
            }

            P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);
            return;
        }
    }
    while (1);
}

void Net_ResetPrediction(void)
{
    mypos = omypos = g_player[myconnectindex].ps->pos;
    myvel = { 0, 0, 0 };
    myang = omyang = g_player[myconnectindex].ps->q16ang;
    myhoriz = omyhoriz = g_player[myconnectindex].ps->q16horiz;
    myhorizoff = omyhorizoff = g_player[myconnectindex].ps->q16horizoff;
    mycursectnum = g_player[myconnectindex].ps->cursectnum;
    myjumpingcounter = g_player[myconnectindex].ps->jumping_counter;
    myjumpingtoggle = g_player[myconnectindex].ps->jumping_toggle;
    myonground = g_player[myconnectindex].ps->on_ground;
    myhardlanding = g_player[myconnectindex].ps->hard_landing;
    myreturntocenter = g_player[myconnectindex].ps->return_to_center;
    my_moto_speed = g_player[myconnectindex].ps->moto_speed;
    my_not_on_water = g_player[myconnectindex].ps->not_on_water;
    my_moto_on_ground = g_player[myconnectindex].ps->moto_on_ground;
    my_moto_do_bump = g_player[myconnectindex].ps->moto_do_bump;
    my_moto_bump_fast = g_player[myconnectindex].ps->moto_bump_fast;
    my_moto_on_oil = g_player[myconnectindex].ps->moto_on_oil;
    my_moto_on_mud = g_player[myconnectindex].ps->moto_on_mud;
    my_moto_bump = g_player[myconnectindex].ps->moto_do_bump;
    my_moto_bump_target = g_player[myconnectindex].ps->moto_bump_target;
    my_moto_turb = g_player[myconnectindex].ps->moto_turb;
    my_stairs = g_player[myconnectindex].ps->stairs;
}

void Net_DoPrediction(void)
{
    DukePlayer_t *const pPlayer = g_player[myconnectindex].ps;
    spritetype *const   pSprite = &sprite[pPlayer->i];

    input_t  *const pInput = &inputfifo[predictfifoplc&(MOVEFIFOSIZ-1)][myconnectindex];

    int16_t backcstat = pSprite->cstat;
    pSprite->cstat &= ~257;

    uint32_t playerBits = pInput->bits;

    if (RRRA)
    {
        if (pPlayer->on_motorcycle && pSprite->extra > 0)
        {
            int var64, var68, var6c, var74, var7c;
            if (my_moto_speed < 0)
                my_moto_speed = 0;
            if (TEST_SYNC_KEY(playerBits, SK_CROUCH))
            {
                var64 = 1;
                playerBits &= ~(1<<SK_CROUCH);
            }
            else
                var64 = 0;

            if (TEST_SYNC_KEY(playerBits, SK_JUMP))
            {
                var68 = 1;
                playerBits &= ~(1<< SK_JUMP);
            }
            else
            {
                var68 = 0;
            }
            if (TEST_SYNC_KEY(playerBits, SK_AIM_UP))
            {
                var6c = 1;
                playerBits &= ~(1<<SK_AIM_UP);
            }
            else
                var6c = 0;
            if (TEST_SYNC_KEY(playerBits, SK_AIM_DOWN))
            {
                var74 = 1;
                playerBits &= ~(1<<SK_AIM_DOWN);
            }
            else
            {
                var74 = 0;
            }
            if (TEST_SYNC_KEY(playerBits, SK_LOOK_LEFT))
            {
                var7c = 1;
                playerBits &= ~(1<<SK_LOOK_LEFT);
            }
            else
            {
                var7c = 0;
            }
            if (myonground == 1)
            {
                if (var64 && my_moto_speed > 0)
                {
                    if (my_moto_on_oil)
                        my_moto_speed -= 2;
                    else
                        my_moto_speed -= 4;
                    if (my_moto_speed < 0)
                        my_moto_speed = 0;
                    my_moto_bump_target = -30;
                    my_moto_do_bump = 1;
                }
                else if (var68 && !var64)
                {
                    if (my_moto_speed < 40)
                    {
                        my_moto_bump_target = 70;
                        my_moto_bump_fast = 1;
                    }
                    my_moto_speed += 2;
                    if (my_moto_speed > 120)
                        my_moto_speed = 120;
                    if (!my_not_on_water)
                        if (my_moto_speed > 80)
                            my_moto_speed = 80;
                }
                else if (my_moto_speed > 0)
                    my_moto_speed--;
                if (my_moto_do_bump && (!var64 || my_moto_speed == 0))
                {
                    my_moto_bump_target = 0;
                    my_moto_do_bump = 0;
                }
                if (var6c && my_moto_speed <= 0 && !var64)
                {
                    int var88;
                    my_moto_speed = -15;
                    var88 = var7c;
                    var7c = var74;
                    var74 = var88;
                }
            }
            if (my_moto_speed != 0 && myonground == 1)
            {
                if (!my_moto_bump)
                    if ((g_globalRandom&3) == 2)
                        my_moto_bump_target = (my_moto_speed>>4)*((randomseed&7)-4);
            }
            if (my_moto_turb)
            {
                if (my_moto_turb <= 1)
                {
                    myhoriz = F16(100);
                    my_moto_turb = 0;
                    my_moto_bump = 0;
                }
                else
                {
                    myhoriz = F16(100+(g_globalRandom&15)-7);
                    my_moto_turb--;
                }
            }
            else if (my_moto_bump_target > my_moto_bump)
            {
                if (my_moto_bump_fast)
                    my_moto_bump += 6;
                else
                    my_moto_bump++;
                if (my_moto_bump_target < my_moto_bump)
                    my_moto_bump = my_moto_bump_target;
                myhoriz = F16(100+my_moto_bump/3);
            }
            else if (my_moto_bump_target < my_moto_bump)
            {
                if (my_moto_bump_fast)
                    my_moto_bump -= 6;
                else
                    my_moto_bump--;
                if (my_moto_bump_target > my_moto_bump)
                    my_moto_bump = my_moto_bump_target;
                myhoriz = F16(100+my_moto_bump/3);
            }
            else
            {
                my_moto_bump_fast = 0;
            }
            if (my_moto_speed >= 20 && myonground == 1 && (var74 || var7c))
            {
                short var8c, var90, var94, var98;
                var8c = my_moto_speed;
                var90 = fix16_to_int(myang);
                if (var74)
                    var94 = -10;
                else
                    var94 = 10;
                if (var94 < 0)
                    var98 = 350;
                else
                    var98 = -350;
                if (my_moto_on_mud || my_moto_on_oil || !my_not_on_water)
                {
                    if (my_moto_on_oil)
                        var8c <<= 3;
                    else
                        var8c <<= 2;
                    if (my_moto_do_bump)
                    {
                        myvel.x += (var8c>>5)*(sintable[(var94*-51+var90+512)&2047]<<4);
                        myvel.y += (var8c>>5)*(sintable[(var94*-51+var90)&2047]<<4);
                        myang = F16((var90-(var98>>2))&2047);
                    }
                    else
                    {
                        myvel.x += (var8c>>7)*(sintable[(var94*-51+var90+512)&2047]<<4);
                        myvel.y += (var8c>>7)*(sintable[(var94*-51+var90)&2047]<<4);
                        myang = F16((var90-(var98>>6))&2047);
                    }
                    my_moto_on_mud = 0;
                    my_moto_on_oil = 0;
                }
                else
                {
                    if (my_moto_do_bump)
                    {
                        myvel.x += (var8c >> 5)*(sintable[(var94*-51 + var90 + 512) & 2047] << 4);
                        myvel.y += (var8c>>5)*(sintable[(var94*-51+var90)&2047]<<4);
                        myang = F16((var90-(var98>>4))&2047);
                    }
                    else
                    {
                        myvel.x += (var8c >> 7)*(sintable[(var94*-51 + var90 + 512) & 2047] << 4);
                        myvel.y += (var8c>>7)*(sintable[(var94*-51+var90)&2047]<<4);
                        myang = F16((var90-(var98>>7))&2047);
                    }
                }
            }
            else if (my_moto_speed >= 20 && myonground == 1 && (my_moto_on_mud || my_moto_on_oil))
            {
                short var9c, vara0;
                var9c = my_moto_speed;
                vara0 = fix16_to_int(myang);
                if (my_moto_on_oil)
                    var9c *= 10;
                else
                    var9c *= 5;
                myvel.x += (var9c>>7)*(sintable[(vara0+512)&2047]<<4);
                myvel.y += (var9c>>7)*(sintable[(vara0)&2047]<<4);
            }
            my_moto_on_mud = 0;
            my_moto_on_oil = 0;
        }
        else if (pPlayer->on_boat && pSprite->extra > 0)
        {
            int vara8, varac, varb0, varb4, varbc, varc4;
            if (my_moto_speed < 0)
                my_moto_speed = 0;
            if (TEST_SYNC_KEY(playerBits, SK_CROUCH) && TEST_SYNC_KEY(playerBits, SK_JUMP))
            {
                vara8 = 1;
                varac = 0;
                playerBits &= ~(1<<SK_JUMP);
                varb0 = 0;
                playerBits &= ~(1<<SK_CROUCH);
            }
            else
                vara8 = 0;
            if (TEST_SYNC_KEY(playerBits, SK_JUMP))
            {
                varac = 1;
                playerBits &= ~(1<<SK_JUMP);
            }
            else
            {
                varac = 0;
            }
            if (TEST_SYNC_KEY(playerBits, SK_CROUCH))
            {
                varb0 = 1;
                playerBits &= ~(1<<SK_CROUCH);
            }
            else
                varb0 = 0;
            if (TEST_SYNC_KEY(playerBits, SK_AIM_UP))
            {
                varb4 = 1;
                playerBits &= ~(1<<SK_AIM_UP);
            }
            else varb4 = 0;
            if (TEST_SYNC_KEY(playerBits, SK_AIM_DOWN))
            {
                varbc = 1;
                playerBits &= ~(1<<SK_AIM_DOWN);
            }
            else
            {
                varbc = 0;
            }
            if (TEST_SYNC_KEY(playerBits, SK_LOOK_LEFT))
            {
                varc4 = 1;
                playerBits &= ~(1<< SK_LOOK_LEFT);
            }
            else
            {
                varc4 = 0;
            }
            if (myonground == 1)
            {
                if (vara8)
                {
                    if (my_moto_speed <= 25)
                    {
                        my_moto_speed++;
                    }
                    else
                    {
                        my_moto_speed -= 2;
                        if (my_moto_speed < 0)
                            my_moto_speed = 0;
                        my_moto_bump_target = 30;
                        my_moto_do_bump = 1;
                    }
                }
                else if (varb0 && my_moto_speed > 0)
                {
                    my_moto_speed -= 2;
                    if (my_moto_speed < 0)
                        my_moto_speed = 0;
                    my_moto_bump_target = 30;
                    my_moto_do_bump = 1;
                }
                else if (varac)
                {
                    if (my_moto_speed < 40)
                        if (!my_not_on_water)
                        {
                            my_moto_bump_target = -30;
                            my_moto_bump_fast = 1;
                        }
                    my_moto_speed++;
                    if (my_moto_speed > 120)
                        my_moto_speed = 120;
                }
                else if (my_moto_speed > 0)
                    my_moto_speed--;
                if (my_moto_do_bump && (!varb0 || my_moto_speed == 0))
                {
                    my_moto_bump_target = 0;
                    my_moto_do_bump = 0;
                }
                if (varb4 && my_moto_speed == 0 && !varb0)
                {
                    int vard0;
                    if (!my_not_on_water)
                        my_moto_speed = -25;
                    else
                        my_moto_speed = -20;
                    vard0 = varc4;
                    varc4 = varbc;
                    varbc = vard0;
                }
            }
            if (my_moto_speed != 0 && myonground == 1)
            {
                if (!my_moto_bump)
                    if ((g_globalRandom & 15) == 14)
                        my_moto_bump_target = (my_moto_speed>>4)*((randomseed&3)-2);
            }
            if (my_moto_turb)
            {
                if (my_moto_turb <= 1)
                {
                    myhoriz = F16(100);
                    my_moto_turb = 0;
                    my_moto_bump_target = 0;
                    my_moto_bump = 0;
                }
                else
                {
                    myhoriz = F16(100+((g_globalRandom&15)-7));
                    my_moto_turb--;
                }
            }
            else if (my_moto_bump_target > my_moto_bump)
            {
                if (my_moto_bump_fast)
                    my_moto_bump += 6;
                else
                    my_moto_bump++;
                if (my_moto_bump_target < my_moto_bump)
                    my_moto_bump = my_moto_bump_target;
                myhoriz = F16(100+my_moto_bump/3);
            }
            else if (my_moto_bump_target < my_moto_bump)
            {
                if (my_moto_bump_fast)
                    my_moto_bump -= 6;
                else
                    my_moto_bump--;
                if (my_moto_bump_target > my_moto_bump)
                    my_moto_bump = my_moto_bump_target;
                myhoriz = F16(100+my_moto_bump/3);
            }
            else
            {
                my_moto_bump_target = 0;
                my_moto_bump_fast = 0;
            }
            if (my_moto_speed > 0 && myonground == 1 && (varbc || varc4))
            {
                short vard4, vard8, vardc, vare0;
                vard4 = my_moto_speed;
                vard8 = fix16_to_int(myang);
                if (varbc)
                    vardc = -10;
                else
                    vardc = 10;
                if (vardc < 0)
                    vare0 = 350;
                else
                    vare0 = -350;
                vard4 <<= 2;
                if (my_moto_do_bump)
                {
                    myvel.x += (vard4>>6)*(sintable[(vardc*-51+vard8+512)&2047]<<4);
                    myvel.y += (vard4>>6)*(sintable[(vardc*-51+vard8)&2047]<<4);
                    myang = F16((vard8-(vare0>>5))&2047);
                }
                else
                {
                    myvel.x += (vard4>>7)*(sintable[(vardc*-51+vard8+512)&2047]<<4);
                    myvel.y += (vard4>>7)*(sintable[(vardc*-51+vard8)&2047]<<4);
                    myang = F16((vard8-(vare0>>6))&2047);
                }
            }
            if (my_not_on_water)
                if (my_moto_speed > 50)
                    my_moto_speed -= (my_moto_speed>>1);
        }
    }

    int sectorLotag = sector[mycursectnum].lotag;
    int myclipdist = 64;
    int spriteNum = 0;

    if (RR)
    {
        if (sectorLotag == 867)
        {
            int spriteNum = headspritesect[mycursectnum];
            while (spriteNum >= 0)
            {
                int const nextSprite = nextspritesect[spriteNum];
                if (sprite[spriteNum].picnum == TILE_RRTILE380)
                    if (sprite[spriteNum].z - ZOFFSET3 < mypos.z)
                        sectorLotag = 2;
                spriteNum = nextSprite;
            }
        }

        if (sectorLotag == 848 && sector[mycursectnum].floorpicnum == TILE_WATERTILE2)
            sectorLotag = 1;

        if (sectorLotag == 857)
            myclipdist = 1;
        else
            myclipdist = 64;
    }

    uint8_t spritebridge = 0;

    int stepHeight, centerHoriz;
    int16_t   ceilingBunch, floorBunch;

    if (ud.clipping == 0 && (mycursectnum < 0 || mycursectnum >= MAXSECTORS || sector[mycursectnum].floorpicnum == TILE_MIRROR))
    {
        mypos.x = omypos.x;
        mypos.y = omypos.y;
    }
    else
    {
        omypos.x = mypos.x;
        omypos.y = mypos.y;
    }

    omyhoriz = myhoriz;
    omyhorizoff = myhorizoff;
    omypos.z = mypos.z;
    omyang = myang;

    int32_t floorZ, ceilZ, highZhit, lowZhit;
    if (!RR || myclipdist == 64)
        getzrange(&mypos, mycursectnum, &ceilZ, &highZhit, &floorZ, &lowZhit, 163, CLIPMASK0);
    else
        getzrange(&mypos, mycursectnum, &ceilZ, &highZhit, &floorZ, &lowZhit, 1, CLIPMASK0);
    
    int truecz, truefz;
#ifdef YAX_ENABLE
    getzsofslope_player(mycursectnum, mypos.x, mypos.y, &truecz, &truefz);
#else
    getzsofslope(psect, mypos.x, mypos.y, &truecz, &truefz);
#endif
    int const trueFloorZ    = truefz;
    int const trueFloorDist = klabs(mypos.z - trueFloorZ);

    if ((lowZhit & 49152) == 16384 && sectorLotag == 1 && trueFloorDist > PHEIGHT + ZOFFSET2)
        sectorLotag = 0;
    
    // calculates automatic view angle for playing without a mouse
    if (pPlayer->aim_mode == 0 && myonground && sectorLotag != ST_2_UNDERWATER
        && (sector[mycursectnum].floorstat & 2))
    {
        vec2_t const adjustedPlayer = { mypos.x + (sintable[(fix16_to_int(myang) + 512) & 2047] >> 5),
                                        mypos.y + (sintable[fix16_to_int(myang) & 2047] >> 5) };
        int16_t curSectNum = mycursectnum;

        updatesector(adjustedPlayer.x, adjustedPlayer.y, &curSectNum);

        if (curSectNum >= 0)
        {
            int const slopeZ = getflorzofslope(mycursectnum, adjustedPlayer.x, adjustedPlayer.y);
            if ((mycursectnum == curSectNum) ||
                (klabs(getflorzofslope(curSectNum, adjustedPlayer.x, adjustedPlayer.y) - slopeZ) <= ZOFFSET6))
                myhorizoff += fix16_from_int(mulscale16(trueFloorZ - slopeZ, 160));
        }
    }
    if (myhorizoff > 0)
    {
        myhorizoff -= ((myhorizoff >> 3) + fix16_one);
        myhorizoff = max(myhorizoff, 0);
    }
    else if (myhorizoff < 0)
    {
        myhorizoff += (((-myhorizoff) >> 3) + fix16_one);
        myhorizoff = min(myhorizoff, 0);
    }

    if (highZhit >= 0 && (highZhit&49152) == 49152)
    {
        highZhit &= (MAXSPRITES-1);

        if (sprite[highZhit].statnum == STAT_ACTOR && sprite[highZhit].extra >= 0)
        {
            highZhit = 0;
            ceilZ    = truecz;
        }
        if (RR)
        {
            if (sprite[highZhit].picnum == TILE_RRTILE3587)
            {
                if (!my_stairs)
                {
                    my_stairs = 10;
                    if (TEST_SYNC_KEY(playerBits, SK_JUMP) && (!RRRA || !pPlayer->on_motorcycle))
                    {
                        highZhit = 0;
                        ceilZ = pPlayer->truecz;
                    }
                }
                else
                    my_stairs--;
            }
        }
    }

    if (lowZhit >= 0 && (lowZhit&49152) == 49152)
    {
        int spriteNum = lowZhit&(MAXSPRITES-1);

        if ((sprite[spriteNum].cstat&33) == 33)
        {
            sectorLotag             = 0;
            spritebridge            = 1;
            //pPlayer->sbs            = spriteNum;
        }
        else if (!RRRA)
            goto check_enemy_sprite;

        if (RRRA)
        {
            if (pPlayer->on_motorcycle)
            {
                if (A_CheckEnemySprite(&sprite[spriteNum]))
                {
                    my_moto_speed -= my_moto_speed >> 4;
                }
            }
            if (pPlayer->on_boat)
            {
                if (A_CheckEnemySprite(&sprite[spriteNum]))
                {
                    my_moto_speed -= my_moto_speed >> 4;
                }
            }
            else
            {
check_enemy_sprite:
                if (A_CheckEnemySprite(&sprite[spriteNum]) && sprite[spriteNum].xrepeat > 24
                     && klabs(pSprite->z - sprite[spriteNum].z) < (84 << 8))
                {
                    // TX: I think this is what makes the player slide off enemies... might
                    // be a good sprite flag to add later.
                    // Helix: there's also SLIDE_ABOVE_ENEMY.
                    int spriteAng = getangle(sprite[spriteNum].x - mypos.x,
                                                sprite[spriteNum].y - mypos.y);
                    myvel.x -= sintable[(spriteAng + 512) & 2047] << 4;
                    myvel.y -= sintable[spriteAng & 2047] << 4;
                }
            }
        }
        if (RR)
        {
            if (sprite[spriteNum].picnum == TILE_RRTILE3587)
            {
                if (!my_stairs)
                {
                    my_stairs = 10;
                    if (TEST_SYNC_KEY(playerBits, SK_CROUCH) && (!RRRA || !pPlayer->on_motorcycle))
                    {
                        ceilZ = sprite[spriteNum].z;
                        highZhit = 0;
                        floorZ = sprite[spriteNum].z + ZOFFSET6;
                    }
                }
                else
                    my_stairs--;
            }
        }
    }
    
    int       velocityModifier = TICSPERFRAME;
    int       floorZOffset     = 40;
    int const playerShrunk     = (pSprite->yrepeat < (RR ? 8 : 32));

    if (pSprite->extra <= 0)
    {
        if (sectorLotag == ST_2_UNDERWATER)
        {
            if (pPlayer->on_warping_sector == 0)
            {
                if (klabs(mypos.z - floorZ) >(PHEIGHT>>1))
                    mypos.z += 348;
            }
            clipmove(&mypos, &mycursectnum,
                0, 0, 164, (4L<<8), (4L<<8), CLIPMASK0);
        }

        updatesector(mypos.x, mypos.y, &mycursectnum);
        pushmove(&mypos, &mycursectnum, 128L, (4L<<8), (20L<<8), CLIPMASK0);

        myhoriz = F16(100);
        myhorizoff = 0;

        goto ENDFAKEPROCESSINPUT;
    }

    if (pPlayer->on_crane >= 0)
        goto FAKEHORIZONLY;

    if (pPlayer->one_eighty_count < 0)
        myang += F16(128);

    if (sectorLotag == ST_2_UNDERWATER)
    {
        myjumpingcounter = 0;

        if (TEST_SYNC_KEY(playerBits, SK_JUMP))
        {
            myvel.z = max(min(-348, myvel.z - 348), -(256 * 6));
        }
        else if (TEST_SYNC_KEY(playerBits, SK_CROUCH))
        {
            myvel.z = min(max(348, myvel.z + 348), (256 * 6));
        }
        else
        {
            // normal view
            if (myvel.z < 0)
                myvel.z = min(0, myvel.z + 256);

            if (myvel.z > 0)
                myvel.z = max(0, myvel.z - 256);
        }

        if (myvel.z > 2048)
            myvel.z >>= 1;

        mypos.z += myvel.z;

        if (mypos.z > (floorZ-(15<<8)))
            mypos.z += ((floorZ-(15<<8))-mypos.z)>>1;

        if (mypos.z < ceilZ+ZOFFSET6)
        {
            mypos.z = ceilZ+ZOFFSET6;
            myvel.z = 0;
        }
    }
    else if (!RR && pPlayer->jetpack_on)
    {
        myonground = 0;
        myjumpingcounter = 0;
        myhardlanding = 0;

        if (pPlayer->jetpack_on < 11)
            mypos.z -= (pPlayer->jetpack_on<<7); //Goin up

        int const zAdjust = playerShrunk ? 512 : 2048;

        if (TEST_SYNC_KEY(playerBits, SK_JUMP))  // jumping, flying up
        {
            mypos.z -= zAdjust;
        }

        if (TEST_SYNC_KEY(playerBits, SK_CROUCH))  // crouching, flying down
        {
            mypos.z += zAdjust;
        }
        
        int const Zdiff = (playerShrunk == 0 && (sectorLotag == 0 || sectorLotag == ST_2_UNDERWATER)) ? 32 : 16;

        if (mypos.z > (floorZ - (Zdiff << 8)))
            mypos.z += ((floorZ - (Zdiff << 8)) - mypos.z) >> 1;

        if (mypos.z < (ceilZ + (18 << 8)))
            mypos.z = ceilZ + (18 << 8);
    }
    else
    {
        if (sectorLotag == ST_1_ABOVE_WATER && spritebridge == 0)
        {
            floorZOffset = playerShrunk ? 12 : 34;
        }
        if (mypos.z < (floorZ-(floorZOffset<<8)))  //falling
        {
            if ((!TEST_SYNC_KEY(playerBits, SK_JUMP) && !TEST_SYNC_KEY(playerBits, SK_CROUCH)) && myonground &&
                (sector[mycursectnum].floorstat & 2) && mypos.z >= (floorZ - (floorZOffset << 8) - ZOFFSET2))
                mypos.z = floorZ - (floorZOffset << 8);
            else
            {
                myonground = 0;
                
                if (RRRA && (pPlayer->on_motorcycle || pPlayer->on_boat) && floorZ - (floorZOffset << 9) > mypos.z)
                {
                    if (my_moto_on_ground)
                    {
                        my_moto_bump_target = 80;
                        my_moto_bump_fast = 1;
                        myvel.z -= g_spriteGravity*(my_moto_speed>>4);
                        my_moto_on_ground = 0;
                    }
                    else
                    {
                        myvel.z += g_spriteGravity-80+(120-my_moto_speed);
                    }
                }
                else
                    myvel.z += (g_spriteGravity + 80);

                if (myvel.z >= (4096 + 2048))
                    myvel.z = (4096 + 2048);

                if ((mypos.z + myvel.z) >= (floorZ - (floorZOffset << 8)) && mycursectnum >= 0)  // hit the ground
                {
                    if (sector[mycursectnum].lotag != ST_1_ABOVE_WATER)
                    {
                        if (RRRA)
                            my_moto_on_ground = 1;
                    }
                }
            }
        }

        else
        {
            if ((sectorLotag != ST_1_ABOVE_WATER && sectorLotag != ST_2_UNDERWATER) &&
                (myonground == 0 && myvel.z > (6144 >> 1)))
                myhardlanding = myvel.z>>10;
            myonground = 1;

            if (floorZOffset == 40)
            {
                //Smooth on the ground
                int Zdiff = ((floorZ - (floorZOffset << 8)) - mypos.z) >> 1;

                if (klabs(Zdiff) < 256)
                    Zdiff = 0;
                
                mypos.z += ((klabs(Zdiff) >= 256) ? (((floorZ - (floorZOffset << 8)) - mypos.z) >> 1) : 0);
                //myz += k; // ((fz-(i<<8))-myz)>>1;
                myvel.z -= 768; // 412;
                if (myvel.z < 0)
                    myvel.z = 0;
            }
            else if (myjumpingcounter == 0)
            {
                mypos.z += ((floorZ - (floorZOffset << 7)) - mypos.z) >> 1;  // Smooth on the water

                if (pPlayer->on_warping_sector == 0 && mypos.z > floorZ - ZOFFSET2)
                {
                    mypos.z = floorZ - ZOFFSET2;
                    myvel.z >>= 1;
                }
            }

            if (TEST_SYNC_KEY(playerBits, SK_CROUCH) && (!RRRA || !pPlayer->on_motorcycle))
                mypos.z += (2048+768);

            if (!TEST_SYNC_KEY(playerBits, SK_JUMP) && (!RRRA || !pPlayer->on_motorcycle) && myjumpingtoggle == 1)
                myjumpingtoggle = 0;
            else if (TEST_SYNC_KEY(playerBits, SK_JUMP) && (!RRRA || !pPlayer->on_motorcycle) && myjumpingtoggle == 0)
            {
                if (myjumpingcounter == 0)
                    if ((floorZ-ceilZ) > (56<<8))
                    {
                        myjumpingcounter = 1;
                        myjumpingtoggle = 1;
                    }
            }
            if (!RR && myjumpingcounter && !TEST_SYNC_KEY(playerBits, SK_JUMP))
                myjumpingcounter = 0;
        }

        if (myjumpingcounter)
        {
            if (!TEST_SYNC_KEY(playerBits, SK_JUMP) && (!RRRA || !pPlayer->on_motorcycle) && myjumpingtoggle == 1)
                myjumpingtoggle = 0;

            if (myjumpingcounter < (RR ? 768 : (1024+256)))
            {
                if (sectorLotag == ST_1_ABOVE_WATER && myjumpingcounter > 768)
                {
                    myjumpingcounter = 0;
                    myvel.z = -512;
                }
                else
                {
                    myvel.z -= (sintable[(2048-128+myjumpingcounter)&2047])/12;
                    myjumpingcounter += 180;
                    myonground = 0;
                }
            }
            else
            {
                myjumpingcounter = 0;
                myvel.z = 0;
            }
        }

        mypos.z += myvel.z;

        if (mypos.z < (ceilZ+ZOFFSET6))
        {
            myjumpingcounter = 0;
            if (myvel.z < 0)
                myvel.x = myvel.y = 0;
            myvel.z = 128;
            mypos.z = ceilZ+ZOFFSET6;
        }

    }

    if (pPlayer->fist_incs || pPlayer->transporter_hold > 2 || myhardlanding || pPlayer->access_incs > 0 ||
        pPlayer->knee_incs > 0 || (!RR && pPlayer->curr_weapon == TRIPBOMB_WEAPON &&
                                   pPlayer->kickback_pic > 1 && pPlayer->kickback_pic < 4))
    {
        velocityModifier = 0;
        myvel.x   = 0;
        myvel.y   = 0;
    }
    else if (pInput->q16avel)          //p->ang += syncangvel * constant
    {
        fix16_t const inputAng  = pInput->q16avel;

        myang += (sectorLotag == ST_2_UNDERWATER) ? fix16_mul(inputAng - (inputAng >> 3), fix16_from_int(ksgn(velocityModifier)))
                                                            : fix16_mul(inputAng, fix16_from_int(ksgn(velocityModifier)));
        
        myang &= 0x7FFFFFF;
    }

    if (myvel.x || myvel.y || pInput->fvel || pInput->svel)
    {
        if (RRRA)
        {
            if (spritebridge == 0 && myonground)
            {
                if (sectorLotag == ST_1_ABOVE_WATER)
                    my_not_on_water = 0;
                else if (pPlayer->on_boat)
                {
                    if (sectorLotag == 1234)
                        my_not_on_water = 0;
                    else
                        my_not_on_water = 1;
                }
                else
                    my_not_on_water = 1;
            }
        }
        if (pPlayer->jetpack_on == 0 && pPlayer->inv_amount[GET_STEROIDS] > 0 && pPlayer->inv_amount[GET_STEROIDS] < 400)
            velocityModifier <<= 1;

        myvel.x += ((pInput->fvel * velocityModifier) << 6);
        myvel.y += ((pInput->svel * velocityModifier) << 6);

        int playerSpeedReduction = 0;
        
        if (!RRRA && myonground && (TEST_SYNC_KEY(playerBits, SK_CROUCH)
         || (pPlayer->kickback_pic > 10 && pPlayer->curr_weapon == KNEE_WEAPON)))
            playerSpeedReduction = 0x2000;
        else if (sectorLotag == ST_2_UNDERWATER)
            playerSpeedReduction = 0x1400;

        myvel.x = mulscale16(myvel.x, pPlayer->runspeed - playerSpeedReduction);
        myvel.y = mulscale16(myvel.y, pPlayer->runspeed - playerSpeedReduction);

        if (RR)
        {
            if (RRRA)
            {
                if (sector[mycursectnum].floorpicnum == TILE_RRTILE7888)
                {
                    if (pPlayer->on_motorcycle && myonground)
                        my_moto_on_oil = 1;
                }
                else if (sector[mycursectnum].floorpicnum == TILE_RRTILE7889)
                {
                    if (pPlayer->on_motorcycle)
                    {
                        if (myonground)
                            my_moto_on_mud = 1;
                    }
                    else if (pPlayer->inv_amount[GET_BOOTS] <= 0)
                    {
                        myvel.x = mulscale16(myvel.x, pPlayer->runspeed);
                        myvel.y = mulscale16(myvel.y, pPlayer->runspeed);
                    }
                }
            }
            if (sector[mycursectnum].floorpicnum == TILE_RRTILE3073 || sector[mycursectnum].floorpicnum == TILE_RRTILE2702)
            {
                if (RRRA && pPlayer->on_motorcycle)
                {
                    if (myonground)
                    {
                        myvel.x = mulscale16(myvel.x, pPlayer->runspeed-0x1800);
                        myvel.y = mulscale16(myvel.y, pPlayer->runspeed-0x1800);
                    }
                }
                else if (pPlayer->inv_amount[GET_BOOTS] <= 0)
                {
                    myvel.x = mulscale16(myvel.x, pPlayer->runspeed-0x1800);
                    myvel.y = mulscale16(myvel.y, pPlayer->runspeed-0x1800);
                }
            }
        }

        if (klabs(myvel.x) < 2048 && klabs(myvel.y) < 2048)
            myvel.x = myvel.y = 0;

        if (playerShrunk)
        {
            myvel.x = mulscale16(myvel.x, pPlayer->runspeed - (pPlayer->runspeed >> 1) + (pPlayer->runspeed >> 2));
            myvel.y = mulscale16(myvel.y, pPlayer->runspeed - (pPlayer->runspeed >> 1) + (pPlayer->runspeed >> 2));
        }
    }

FAKEHORIZONLY:;
    stepHeight = (sectorLotag == ST_1_ABOVE_WATER || spritebridge == 1) ? pPlayer->autostep_sbw : pPlayer->autostep;
    
#ifdef YAX_ENABLE
    if (mycursectnum >= 0)
        yax_getbunches(mycursectnum, &ceilingBunch, &floorBunch);

    // This updatesectorz conflicts with Duke3D's way of teleporting through water,
    // so make it a bit conditional... OTOH, this way we have an ugly z jump when
    // changing from above water to underwater

    if ((mycursectnum >= 0 && !(sector[mycursectnum].lotag == ST_1_ABOVE_WATER && myonground && floorBunch >= 0))
        && ((floorBunch >= 0 && !(sector[mycursectnum].floorstat & 512))
            || (ceilingBunch >= 0 && !(sector[mycursectnum].ceilingstat & 512))))
    {
        mycursectnum += MAXSECTORS;  // skip initial z check, restored by updatesectorz
        updatesectorz(mypos.x, mypos.y, mypos.z, &mycursectnum);
    }
#endif
    clipmove(&mypos, &mycursectnum, myvel.x, myvel.y, 164, (4L << 8), stepHeight, CLIPMASK0);
    pushmove(&mypos, &mycursectnum, 164, (4L << 8), (4L << 8), CLIPMASK0);

    // This makes the player view lower when shrunk.  NOTE that it can get the
    // view below the sector floor (and does, when on the ground).
    if (pPlayer->jetpack_on == 0 && sectorLotag != ST_2_UNDERWATER && sectorLotag != ST_1_ABOVE_WATER && playerShrunk)
        mypos.z += ZOFFSET5;

    if (RR)
    {
        if ((spriteNum & 49152) == 32768)
        {
            int const wallNum = spriteNum&(MAXWALLS-1);
            if (RRRA && pPlayer->on_motorcycle)
            {
                int16_t var108, var10c;
                var108 = getangle(wall[wall[wallNum].point2].x-wall[wallNum].x,wall[wall[wallNum].point2].y-wall[wallNum].y);
                var10c = klabs(fix16_to_int(myang)-var108);
                if (var10c >= 441 && var10c <= 581)
                {
                    my_moto_speed = 0;
                }
                else if (var10c >= 311 && var10c <= 711)
                {
                    my_moto_speed -= (my_moto_speed>>1)+(my_moto_speed>>2);
                }
                else if (var10c >= 111 && var10c <= 911)
                {
                    my_moto_speed -= (my_moto_speed>>1);
                }
                else
                {
                    my_moto_speed -= (my_moto_speed>>3);
                }
            }
            else if (RRRA && pPlayer->on_boat)
            {
                short var114, var118;
                var114 = getangle(wall[wall[wallNum].point2].x-wall[wallNum].x,wall[wall[wallNum].point2].y-wall[wallNum].y);
                var118 = klabs(fix16_to_int(myang)-var114);
                if (var118 >= 441 && var118 <= 581)
                {
                    my_moto_speed = ((my_moto_speed>>1)+(my_moto_speed>>2))>>2;
                }
                else if (var118 >= 311 && var118 <= 711)
                {
                    my_moto_speed -= ((my_moto_speed>>1)+(my_moto_speed>>2))>>3;
                }
                else if (var118 >= 111 && var118 <= 911)
                {
                    my_moto_speed -= (my_moto_speed>>4);
                }
                else
                {
                    my_moto_speed -= (my_moto_speed>>6);
                }
            }
        }
        else if ((spriteNum & 49152) == 49152)
        {
            spriteNum &= (MAXSPRITES-1);
                
            if (RRRA && pPlayer->on_motorcycle)
            {
                if (A_CheckEnemySprite(&sprite[spriteNum]) || sprite[spriteNum].picnum == TILE_APLAYER)
                {
                    my_moto_speed -= my_moto_speed>>2;
                    my_moto_turb = 6;
                }
            }
            else if (RRRA && pPlayer->on_boat)
            {
                if (A_CheckEnemySprite(&sprite[spriteNum]) || sprite[spriteNum].picnum == TILE_APLAYER)
                {
                    my_moto_speed -= my_moto_speed>>2;
                    my_moto_turb = 6;
                }
            }
        }
    }

    centerHoriz = 0;
    if (TEST_SYNC_KEY(playerBits, SK_CENTER_VIEW) || myhardlanding)
        myreturntocenter = 9;

    if (TEST_SYNC_KEY(playerBits, SK_LOOK_UP))
    {
        myreturntocenter = 9;
        myhoriz += fix16_from_int(12<<(int)(TEST_SYNC_KEY(playerBits, SK_RUN)));
        centerHoriz++;
    }
    else if (TEST_SYNC_KEY(playerBits, SK_LOOK_DOWN))
    {
        myreturntocenter = 9;
        myhoriz -= fix16_from_int(12<<(int)(TEST_SYNC_KEY(playerBits, SK_RUN)));
        centerHoriz++;
    }
    else if (TEST_SYNC_KEY(playerBits, SK_AIM_UP))
    {
        myhoriz += fix16_from_int(6<<(int)(TEST_SYNC_KEY(playerBits, SK_RUN)));
        centerHoriz++;
    }
    else if (TEST_SYNC_KEY(playerBits, SK_AIM_DOWN))
    {
        myhoriz -= fix16_from_int(6<<(int)(TEST_SYNC_KEY(playerBits, SK_RUN)));
        centerHoriz++;
    }

    if (RR && pPlayer->recoil && pPlayer->kickback_pic == 0)
    {
        int delta = pPlayer->recoil >> 1;
        if (!delta) delta++;
        myhoriz -= F16(delta);
    }
    if (myreturntocenter > 0 && !TEST_SYNC_KEY(playerBits, SK_LOOK_UP) && !TEST_SYNC_KEY(playerBits, SK_LOOK_DOWN))
    {
        myreturntocenter--;
        myhoriz += F16(33)-fix16_div(myhoriz, F16(3));
        centerHoriz++;
    }
    if (myhardlanding > 0)
    {
        myhardlanding--;
        myhoriz -= fix16_from_int(myhardlanding<<4);
    }

    myhoriz = fix16_clamp(myhoriz + pInput->q16horz, F16(HORIZ_MIN), F16(HORIZ_MAX));

    if (centerHoriz)
    {
        if (myhoriz > F16(95) && myhoriz < F16(105)) myhoriz = F16(100);
        if (myhorizoff > F16(-5) && myhorizoff < F16(5)) myhorizoff = 0;
    }

    if (pPlayer->knee_incs > 0)
    {
        myhoriz -= F16(48);
        myreturntocenter = 9;
    }


ENDFAKEPROCESSINPUT:

    myposbak[predictfifoplc&(MOVEFIFOSIZ-1)] = mypos;
    myangbak[predictfifoplc&(MOVEFIFOSIZ-1)] = myang;
    myhorizbak[predictfifoplc&(MOVEFIFOSIZ-1)] = myhoriz;
    predictfifoplc++;

    pSprite->cstat = backcstat;
}

void Net_CorrectPrediction(void)
{
    if (numplayers < 2)
        return;

    int i = ((movefifoplc-1)&(MOVEFIFOSIZ-1));
    DukePlayer_t *p = g_player[myconnectindex].ps;

    if (!Bmemcmp(&p->pos, &myposbak[i], sizeof(vec3_t))
        && p->q16horiz == myhorizbak[i] && p->q16ang == myangbak[i]) return;

    mypos = p->pos; omypos = p->opos, myvel = p->vel;
    myang = p->q16ang; omyang = p->oq16ang;
    mycursectnum = p->cursectnum;
    myhoriz = p->q16horiz; omyhoriz = p->oq16horiz;
    myhorizoff = p->q16horizoff; omyhorizoff = p->oq16horizoff;
    myjumpingcounter = p->jumping_counter;
    myjumpingtoggle = p->jumping_toggle;
    myonground = p->on_ground;
    myhardlanding = p->hard_landing;
    myreturntocenter = p->return_to_center;
    my_moto_speed = p->moto_speed;
    my_not_on_water = p->not_on_water;
    my_moto_on_ground = p->moto_on_ground;
    my_moto_do_bump = p->moto_do_bump;
    my_moto_bump_fast = p->moto_bump_fast;
    my_moto_on_oil = p->moto_on_oil;
    my_moto_on_mud = p->moto_on_mud;
    my_moto_bump = p->moto_do_bump;
    my_moto_bump_target = p->moto_bump_target;
    my_moto_turb = p->moto_turb;
    my_stairs = p->stairs;

    predictfifoplc = movefifoplc;
    while (predictfifoplc < g_player[myconnectindex].movefifoend)
        Net_DoPrediction();
}

int g_numSyncBytes = 1;
char g_szfirstSyncMsg[MAXSYNCBYTES][60];
int g_foundSyncError = 0;

char syncstat[MAXSYNCBYTES];
int syncvaltail, syncvaltottail;

static int crctable[256];
#define updatecrc(dcrc,xz) (dcrc = (crctable[((dcrc)>>8)^((xz)&255)]^((dcrc)<<8)))

void initsynccrc(void)
{
    int i, j, k, a;

    for (j=0;j<256;j++)     //Calculate CRC table
    {
        k = (j<<8); a = 0;
        for (i=7;i>=0;i--)
        {
            if (((k^a)&0x8000) > 0)
                a = ((a<<1)&65535) ^ 0x1021;   //0x1021 = genpoly
            else
                a = ((a<<1)&65535);
            k = ((k<<1)&65535);
        }
        crctable[j] = (a&65535);
    }
}

char Net_PlayerSync(void)
{
    uint16_t crc = 0;
    DukePlayer_t *pp;

    for(int TRAVERSE_CONNECT(i))
    {
        pp = g_player[i].ps;
        updatecrc(crc, pp->pos.x & 255);
        updatecrc(crc, pp->pos.y & 255);
        updatecrc(crc, pp->pos.z & 255);
        updatecrc(crc, pp->q16ang & 255);
    }

    return ((char) crc & 255);
}

char Net_PlayerSync2(void)
{
    int nextj;
    uint16_t crc = 0;
    DukePlayer_t *pp;
    spritetype *spr;

    for (int TRAVERSE_CONNECT(i))
    {
        pp = g_player[i].ps;

        updatecrc(crc, pp->q16horiz & 255);
        updatecrc(crc, sprite[pp->i].extra & 255);
        updatecrc(crc, pp->bobcounter & 255);
    }

    for (int TRAVERSE_SPRITE_STAT(headspritestat[STAT_PLAYER], j, nextj))
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    return ((char) crc & 255);
}

char Net_ActorSync(void)
{
    uint16_t crc = 0;
    int nextj;
    spritetype *spr;

    for (int TRAVERSE_SPRITE_STAT(headspritestat[STAT_ACTOR], j, nextj))
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->lotag) & 255);
        updatecrc(crc, (spr->hitag) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    for (int TRAVERSE_SPRITE_STAT(headspritestat[STAT_ZOMBIEACTOR], j, nextj))
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->lotag) & 255);
        updatecrc(crc, (spr->hitag) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    return ((char) crc & 255);
}

char Net_WeaponSync(void)
{
    uint16_t crc = 0;
    int nextj;
    spritetype *spr;

    for (int TRAVERSE_SPRITE_STAT(headspritestat[STAT_PROJECTILE], j, nextj))
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    return ((char) crc & 255);
}

char Net_MapSync(void)
{
    uint16_t crc = 0;
    int nextj;
    spritetype *spr;
    walltype *wal;
    sectortype *sect;

    for (int TRAVERSE_SPRITE_STAT(headspritestat[STAT_EFFECTOR], j, nextj))
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
        updatecrc(crc, (spr->lotag) & 255);
        updatecrc(crc, (spr->hitag) & 255);
    }

    for (int j=numwalls;j>=0;j--)
    {
        wal = &wall[j];
        updatecrc(crc, (wal->x) & 255);
        updatecrc(crc, (wal->y) & 255);
    }

    for (int j=numsectors;j>=0;j--)
    {
        sect = &sector[j];
        updatecrc(crc, (sect->floorz) & 255);
        updatecrc(crc, (sect->ceilingz) & 255);
    }

    return ((char) crc & 255);
}

char Net_RandomSync(void)
{
    unsigned short crc = 0;

    updatecrc(crc, randomseed & 255);
    updatecrc(crc, (randomseed >> 8) & 255);
    updatecrc(crc, g_globalRandom & 255);
    updatecrc(crc, (g_globalRandom >> 8) & 255);

    if (g_numSyncBytes == 1)
    {
        updatecrc(crc,Net_PlayerSync() & 255);
        updatecrc(crc,Net_PlayerSync2() & 255);
        updatecrc(crc,Net_WeaponSync() & 255);
        updatecrc(crc,Net_ActorSync() & 255);
        updatecrc(crc,Net_MapSync() & 255);
    }

    return ((char) crc & 255);
}

const char *SyncNames[] =
{
    "Net_CheckRandomSync",
    "Net_CheckPlayerSync",
    "Net_CheckPlayerSync2",
    "Net_CheckWeaponSync",
    "Net_CheckActorSync",
    "Net_CheckMapSync",
    NULL
};

static char(*SyncFunc[MAXSYNCBYTES + 1])(void) =
{
    Net_RandomSync,
    Net_PlayerSync,
    Net_PlayerSync2,
    Net_WeaponSync,
    Net_ActorSync,
    Net_MapSync,
    NULL
};

void Net_GetSyncStat(void)
{
    int i;
    playerdata_t *pp = &g_player[myconnectindex];
    unsigned int val;
    static unsigned int count;

    if (numplayers < 2)
        return;

    for (i = 0; SyncFunc[i]; i++)
    {
        pp->syncval[pp->syncvalhead & (SYNCFIFOSIZ - 1)][i] = (*SyncFunc[i])();
    }

    val = pp->syncval[pp->syncvalhead & (SYNCFIFOSIZ - 1)][0];
    count += val;

    pp->syncvalhead++;
}

////////////////////////////////////////////////////////////////////////
//
// Sync Message print
//
////////////////////////////////////////////////////////////////////////


void Net_DisplaySyncMsg(void)
{
    int i, j;
    static unsigned int moveCount = 0;
    extern unsigned int g_moveThingsCount;

//    if (!SyncPrintMode)
//        return;

    if (numplayers < 2)
        return;

    for (i = 0; i < g_numSyncBytes; i++)
    {
        // syncstat is NON 0 - out of sync
        if (syncstat[i] != 0)
        {
            if (g_numSyncBytes > 1)
            {
                Printf(PRINT_NOTIFY, "Out Of Sync - %s\n", SyncNames[i]);
            }

            if (!g_foundSyncError && g_szfirstSyncMsg[i][0] == '\0')
            {
                // g_foundSyncError one so test all of them and then never test again
                g_foundSyncError = 1;

                // save off loop count
                moveCount = g_moveThingsCount;

                for (j = 0; j < g_numSyncBytes; j++)
                {
                    if (syncstat[j] != 0 && g_szfirstSyncMsg[j][0] == '\0')
                    {
                        Printf(PRINT_NOTIFY, "Out Of Sync (%s) - Please restart game\n", SyncNames[j]);
                    }
                }
            }
        }
    }

    // print out the g_szfirstSyncMsg message you got
    for (i = 0; i < g_numSyncBytes; i++)
    {
        if (g_szfirstSyncMsg[i][0] != '\0')
        {
            if (g_numSyncBytes > 1)
            {
                Printf(PRINT_NOTIFY, "FIRST %s - moveCount %d\n", g_szfirstSyncMsg[i],moveCount);
            }
            else
            {
                Printf(PRINT_NOTIFY, "%s\n", g_szfirstSyncMsg[i]);
            }
        }
    }
}


void  Net_AddSyncInfoToPacket(int *j)
{
    int sb;
    int count = 0;

    // sync testing
    while (g_player[myconnectindex].syncvalhead != syncvaltail && count++ < 4)
    {
        for (sb = 0; sb < g_numSyncBytes; sb++)
            packbuf[(*j)++] = g_player[myconnectindex].syncval[syncvaltail & (SYNCFIFOSIZ - 1)][sb];

        syncvaltail++;
    }
}

void Net_GetSyncInfoFromPacket(uint8_t *packbuf, int packbufleng, int *j, int otherconnectindex)
{
    int sb;
    extern int syncvaltail, syncvaltottail;
    playerdata_t *ppo = &g_player[otherconnectindex];
    char found = 0;

    // have had problems with this routine crashing when players quit
    // games.

    // if ready2send is not set then don't try to get sync info

    if (!ready2send)
        return;

    // Suspect that its trying to traverse the connect list
    // for a player that does not exist.  This tries to take care of that

    for (int TRAVERSE_CONNECT(i))
    {
        if (otherconnectindex == i)
            found = 1;
    }

    if (!found)
        return;

    // sync testing
    //while ((*j) != packbufleng) // changed this on Kens suggestion
    while ((*j) < packbufleng)
    {
        for (sb = 0; sb < g_numSyncBytes; sb++)
        {
            ppo->syncval[ppo->syncvalhead & (SYNCFIFOSIZ - 1)][sb] = packbuf[(*j)++];
        }
        ppo->syncvalhead++;
    }

    // update syncstat
    // if any of the syncstat vars is non-0 then there is a problem
    for (int TRAVERSE_CONNECT(i))
    {
        if (g_player[i].syncvalhead == syncvaltottail)
            return;
    }

    //for (sb = 0; sb < g_numSyncBytes; sb++)
    //    syncstat[sb] = 0;

    while (1)
    {
        for (int i = connectpoint2[connecthead]; i >= 0; i = connectpoint2[i])
        {
            for (sb = 0; sb < g_numSyncBytes; sb++)
            {
                if (g_player[i].connected && (g_player[i].syncval[syncvaltottail & (SYNCFIFOSIZ - 1)][sb] != g_player[connecthead].syncval[syncvaltottail & (SYNCFIFOSIZ - 1)][sb]))
                {
                    syncstat[sb] = 1;
                }
            }
        }

        syncvaltottail++;

        for (int TRAVERSE_CONNECT(i))
        {
            if (g_player[i].syncvalhead == syncvaltottail)
                return;
        }
    }
}

void Net_ClearFIFO(void)
{
    int i = 0;

    syncvaltail = 0L;
    syncvaltottail = 0L;
    memset(&syncstat, 0, sizeof(syncstat));
    memset(&g_szfirstSyncMsg, 0, sizeof(g_szfirstSyncMsg));
    g_foundSyncError = 0;

    bufferjitter = 1;
    mymaxlag = otherminlag = 0;
    movefifoplc = movefifosendplc = predictfifoplc = 0;
    avgfvel = avgsvel = avgavel = avghorz = avgbits = avgextbits = 0;

    for (; i < MAXPLAYERS; i++)
    {
        Bmemset(&g_player[i].movefifoend, 0, sizeof(g_player[i].movefifoend));
        Bmemset(&g_player[i].syncvalhead, 0, sizeof(g_player[i].syncvalhead));
        Bmemset(&g_player[i].myminlag, 0, sizeof(g_player[i].myminlag));
    }
}

void Net_GetInput(void)
{
    input_t* osyn, * nsyn;

    if (numplayers > 1)
        Net_GetPackets();

    if (g_player[myconnectindex].movefifoend - movefifoplc >= 100 || System_WantGuiCapture())
        return;

    if (RRRA && g_player[myconnectindex].ps->on_motorcycle)
        P_GetInputMotorcycle(myconnectindex);
    else if (RRRA && g_player[myconnectindex].ps->on_boat)
        P_GetInputBoat(myconnectindex);
    else if (DEER)
        P_DHGetInput(myconnectindex);
    else
        P_GetInput(myconnectindex);

    avgfvel += localInput.fvel;
    avgsvel += localInput.svel;
    avgavel += localInput.q16avel;
    avghorz += localInput.q16horz;
    avgbits |= localInput.bits;
    avgextbits |= localInput.extbits;
    
    if (g_player[myconnectindex].movefifoend&(g_movesPerPacket-1))
    {
        memcpy(&inputfifo[g_player[myconnectindex].movefifoend & (MOVEFIFOSIZ - 1)][myconnectindex],
            &inputfifo[(g_player[myconnectindex].movefifoend - 1) & (MOVEFIFOSIZ - 1)][myconnectindex], sizeof(input_t));
        g_player[myconnectindex].movefifoend++;
        return;
    }
    nsyn = &inputfifo[g_player[myconnectindex].movefifoend&(MOVEFIFOSIZ-1)][myconnectindex];
    nsyn[0].fvel = avgfvel/g_movesPerPacket;
    nsyn[0].svel = avgsvel/g_movesPerPacket;
    nsyn[0].q16avel = avgavel/g_movesPerPacket;
    nsyn[0].q16horz = avghorz/g_movesPerPacket;
    nsyn[0].bits = avgbits;
    nsyn[0].extbits = avgextbits;
    avgfvel = avgsvel = avgavel = avghorz = avgbits = avgextbits = 0;
    g_player[myconnectindex].movefifoend++;


    for (int TRAVERSE_CONNECT(i))
    if (i != myconnectindex)
    {
        int k = (g_player[myconnectindex].movefifoend-1)-g_player[i].movefifoend;
        g_player[i].myminlag = min(g_player[i].myminlag,k);
        mymaxlag = max(mymaxlag,k);
    }

    if (((g_player[myconnectindex].movefifoend-1)&(TIMERUPDATESIZ-1)) == 0)
    {
        int i = mymaxlag-bufferjitter; mymaxlag = 0;
        if (i > 0) bufferjitter += ((3+i)>>2);
        else if (i < 0) bufferjitter -= ((1-i)>>2);
    }

    if (g_networkBroadcastMode == 1)
    {
        packbuf[0] = SERVER_GENERATED_BROADCAST;
        if ((g_player[myconnectindex].movefifoend-1) == 0) packbuf[0] = PACKET_TYPE_BROADCAST;
        int j = 1;

        //Fix timers and buffer/jitter value
        if (((g_player[myconnectindex].movefifoend-1)&(TIMERUPDATESIZ-1)) == 0)
        {
            if (myconnectindex != connecthead)
            {
                int i = g_player[connecthead].myminlag-otherminlag;
                if (klabs(i) > 8) i >>= 1;
                else if (klabs(i) > 2) i = ksgn(i);
                else i = 0;

                totalclock -= TICSPERFRAME*i;
                g_player[connecthead].myminlag -= i; otherminlag += i;
            }

            if (myconnectindex == connecthead)
                for(int i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                    packbuf[j++] = min(max(g_player[i].myminlag,-128),127);

            for(int i=connecthead;i>=0;i=connectpoint2[i])
                g_player[i].myminlag = 0x7fffffff;
        }

        osyn = (input_t *)&inputfifo[(g_player[myconnectindex].movefifoend-2)&(MOVEFIFOSIZ-1)][myconnectindex];
        nsyn = (input_t *)&inputfifo[(g_player[myconnectindex].movefifoend-1)&(MOVEFIFOSIZ-1)][myconnectindex];

        int k = j;
        packbuf[j++] = 0;
        packbuf[j++] = 0;

        if (nsyn[0].fvel != osyn[0].fvel)
        {
            packbuf[j++] = (char)nsyn[0].fvel;
            packbuf[j++] = (char)(nsyn[0].fvel>>8);
            packbuf[k] |= 1;
        }
        if (nsyn[0].svel != osyn[0].svel)
        {
            packbuf[j++] = (char)nsyn[0].svel;
            packbuf[j++] = (char)(nsyn[0].svel>>8);
            packbuf[k] |= 2;
        }
        if (nsyn[0].q16avel != osyn[0].q16avel)
        {
            packbuf[j++] = (char)((nsyn[0].q16avel) & 255);
            packbuf[j++] = (char)((nsyn[0].q16avel >> 8) & 255);
            packbuf[j++] = (char)((nsyn[0].q16avel >> 16) & 255);
            packbuf[j++] = (char)((nsyn[0].q16avel >> 24) & 255);
            packbuf[k] |= 4;
        }

        if ((nsyn[0].bits^osyn[0].bits)&0x000000ff) packbuf[j++] = (nsyn[0].bits&255), packbuf[k] |= 8;
        if ((nsyn[0].bits^osyn[0].bits)&0x0000ff00) packbuf[j++] = ((nsyn[0].bits>>8)&255), packbuf[k] |= 16;
        if ((nsyn[0].bits^osyn[0].bits)&0x00ff0000) packbuf[j++] = ((nsyn[0].bits>>16)&255), packbuf[k] |= 32;
        if ((nsyn[0].bits^osyn[0].bits)&0xff000000) packbuf[j++] = ((nsyn[0].bits>>24)&255), packbuf[k] |= 64;

        if (nsyn[0].q16horz != osyn[0].q16horz)
        {
            packbuf[j++] = (char)((nsyn[0].q16horz) & 255);
            packbuf[j++] = (char)((nsyn[0].q16horz >> 8) & 255);
            packbuf[j++] = (char)((nsyn[0].q16horz >> 16) & 255);
            packbuf[j++] = (char)((nsyn[0].q16horz >> 24) & 255);
            packbuf[k] |= 128;
        }
//        k++;
        packbuf[++k] = 0;
        if (nsyn[0].extbits != osyn[0].extbits) packbuf[j++] = nsyn[0].extbits, packbuf[k] |= 1;
        /*        if ((nsyn[0].extbits^osyn[0].extbits)&0x000000ff) packbuf[j++] = (nsyn[0].extbits&255), packbuf[k] |= 1;
                if ((nsyn[0].extbits^osyn[0].extbits)&0x0000ff00) packbuf[j++] = ((nsyn[0].extbits>>8)&255), packbuf[k] |= 2;
                if ((nsyn[0].extbits^osyn[0].extbits)&0x00ff0000) packbuf[j++] = ((nsyn[0].extbits>>16)&255), packbuf[k] |= 4;
                if ((nsyn[0].extbits^osyn[0].extbits)&0xff000000) packbuf[j++] = ((nsyn[0].extbits>>24)&255), packbuf[k] |= 8; */

        /*        while (g_player[myconnectindex].syncvalhead != syncvaltail)
                {
                    packbuf[j++] = g_player[myconnectindex].syncval[syncvaltail&(MOVEFIFOSIZ-1)];
                    syncvaltail++;
                } */

        Net_AddSyncInfoToPacket(&j);

        for (int TRAVERSE_CONNECT(i))
        if (i != myconnectindex)
            Net_SendPacket(i,packbuf,j);

        return;
    }
    if (myconnectindex != connecthead)   //Slave
    {
        //Fix timers and buffer/jitter value
        if (((g_player[myconnectindex].movefifoend-1)&(TIMERUPDATESIZ-1)) == 0)
        {
            int i = g_player[connecthead].myminlag - otherminlag;
            if (klabs(i) > 2)
            {
                if (klabs(i) > 8)
                {
                    if (i < 0)
                        i++;
                    i >>= 1;
                }
                else
                {
                    if (i < 0)
                        i = -1;
                    if (i > 0)
                        i = 1;
                }
                totalclock -= TICSPERFRAME * i;
                otherminlag += i;
            }

            for (int TRAVERSE_CONNECT(i))
            {
                g_player[i].myminlag = 0x7fffffff;
            }
        }

        packbuf[0] = PACKET_TYPE_SLAVE_TO_MASTER;
        packbuf[1] = 0;
        packbuf[2] = 0;
        int j = 3;

        osyn = (input_t *)&inputfifo[(g_player[myconnectindex].movefifoend-2)&(MOVEFIFOSIZ-1)][myconnectindex];
        nsyn = (input_t *)&inputfifo[(g_player[myconnectindex].movefifoend-1)&(MOVEFIFOSIZ-1)][myconnectindex];

        if (nsyn[0].fvel != osyn[0].fvel)
        {
            packbuf[j++] = (char)nsyn[0].fvel;
            packbuf[j++] = (char)(nsyn[0].fvel>>8);
            packbuf[1] |= 1;
        }
        if (nsyn[0].svel != osyn[0].svel)
        {
            packbuf[j++] = (char)nsyn[0].svel;
            packbuf[j++] = (char)(nsyn[0].svel>>8);
            packbuf[1] |= 2;
        }
        if (nsyn[0].q16avel != osyn[0].q16avel)
        {
            packbuf[j++] = (char)((nsyn[0].q16avel) & 255);
            packbuf[j++] = (char)((nsyn[0].q16avel >> 8) & 255);
            packbuf[j++] = (char)((nsyn[0].q16avel >> 16) & 255);
            packbuf[j++] = (char)((nsyn[0].q16avel >> 24) & 255);
            packbuf[1] |= 4;
        }

        if ((nsyn[0].bits^osyn[0].bits)&0x000000ff) packbuf[j++] = (nsyn[0].bits&255), packbuf[1] |= 8;
        if ((nsyn[0].bits^osyn[0].bits)&0x0000ff00) packbuf[j++] = ((nsyn[0].bits>>8)&255), packbuf[1] |= 16;
        if ((nsyn[0].bits^osyn[0].bits)&0x00ff0000) packbuf[j++] = ((nsyn[0].bits>>16)&255), packbuf[1] |= 32;
        if ((nsyn[0].bits^osyn[0].bits)&0xff000000) packbuf[j++] = ((nsyn[0].bits>>24)&255), packbuf[1] |= 64;

        if (nsyn[0].q16horz != osyn[0].q16horz)
        {
            packbuf[j++] = (char)((nsyn[0].q16horz) & 255);
            packbuf[j++] = (char)((nsyn[0].q16horz >> 8) & 255);
            packbuf[j++] = (char)((nsyn[0].q16horz >> 16) & 255);
            packbuf[j++] = (char)((nsyn[0].q16horz >> 24) & 255);
            packbuf[1] |= 128;
        }
        packbuf[2] = 0;
        if (nsyn[0].extbits != osyn[0].extbits) packbuf[j++] = nsyn[0].extbits, packbuf[2] |= 1;
        /*        if ((nsyn[0].extbits^osyn[0].extbits)&0x000000ff) packbuf[j++] = (nsyn[0].extbits&255), packbuf[2] |= 1;
                if ((nsyn[0].extbits^osyn[0].extbits)&0x0000ff00) packbuf[j++] = ((nsyn[0].extbits>>8)&255), packbuf[2] |= 2;
                if ((nsyn[0].extbits^osyn[0].extbits)&0x00ff0000) packbuf[j++] = ((nsyn[0].extbits>>16)&255), packbuf[2] |= 4;
                if ((nsyn[0].extbits^osyn[0].extbits)&0xff000000) packbuf[j++] = ((nsyn[0].extbits>>24)&255), packbuf[2] |= 8; */

        /*        while (g_player[myconnectindex].syncvalhead != syncvaltail)
                {
                    packbuf[j++] = g_player[myconnectindex].syncval[syncvaltail&(MOVEFIFOSIZ-1)];
                    syncvaltail++;
                } */
        Net_AddSyncInfoToPacket(&j);

        Net_SendPacket(connecthead,packbuf,j);
        return;
    }

    //This allows allow packet resends
    for (int TRAVERSE_CONNECT(i))
    if (g_player[i].movefifoend <= movefifosendplc)
    {
        packbuf[0] = PACKET_TYPE_NULL_PACKET;
        for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
            Net_SendPacket(i,packbuf,1);
        return;
    }

    while (1)  //Master
    {
        for (int TRAVERSE_CONNECT(i))
        if (g_player[i].playerquitflag && (g_player[i].movefifoend <= movefifosendplc)) return;

        osyn = (input_t *)&inputfifo[(movefifosendplc-1)&(MOVEFIFOSIZ-1)][0];
        nsyn = (input_t *)&inputfifo[(movefifosendplc)&(MOVEFIFOSIZ-1)][0];

        //MASTER -> SLAVE packet
        packbuf[0] = PACKET_TYPE_MASTER_TO_SLAVE;
        int j = 1;

        //Fix timers and buffer/jitter value
        if ((movefifosendplc&(TIMERUPDATESIZ-1)) == 0)
        {
            for (int i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                if (g_player[i].playerquitflag)
                    packbuf[j++] = min(max(g_player[i].myminlag,-128),127);

            for (int TRAVERSE_CONNECT(i))
            g_player[i].myminlag = 0x7fffffff;
        }

        int k = j;
        for (int TRAVERSE_CONNECT(i))
        j += g_player[i].playerquitflag + g_player[i].playerquitflag;
        for (int TRAVERSE_CONNECT(i))
        {
            if (g_player[i].playerquitflag == 0) continue;

            packbuf[k] = 0;
            if (nsyn[i].fvel != osyn[i].fvel)
            {
                packbuf[j++] = (char)nsyn[i].fvel;
                packbuf[j++] = (char)(nsyn[i].fvel>>8);
                packbuf[k] |= 1;
            }
            if (nsyn[i].svel != osyn[i].svel)
            {
                packbuf[j++] = (char)nsyn[i].svel;
                packbuf[j++] = (char)(nsyn[i].svel>>8);
                packbuf[k] |= 2;
            }
            if (nsyn[i].q16avel != osyn[i].q16avel)
            {
                packbuf[j++] = (char)((nsyn[i].q16avel) & 255);
                packbuf[j++] = (char)((nsyn[i].q16avel >> 8) & 255);
                packbuf[j++] = (char)((nsyn[i].q16avel >> 16) & 255);
                packbuf[j++] = (char)((nsyn[i].q16avel >> 24) & 255);
                packbuf[k] |= 4;
            }

            if ((nsyn[i].bits^osyn[i].bits)&0x000000ff) packbuf[j++] = (nsyn[i].bits&255), packbuf[k] |= 8;
            if ((nsyn[i].bits^osyn[i].bits)&0x0000ff00) packbuf[j++] = ((nsyn[i].bits>>8)&255), packbuf[k] |= 16;
            if ((nsyn[i].bits^osyn[i].bits)&0x00ff0000) packbuf[j++] = ((nsyn[i].bits>>16)&255), packbuf[k] |= 32;
            if ((nsyn[i].bits^osyn[i].bits)&0xff000000) packbuf[j++] = ((nsyn[i].bits>>24)&255), packbuf[k] |= 64;

            if (nsyn[i].q16horz != osyn[i].q16horz)
            {
                packbuf[j++] = (char)((nsyn[i].q16horz) & 255);
                packbuf[j++] = (char)((nsyn[i].q16horz >> 8) & 255);
                packbuf[j++] = (char)((nsyn[i].q16horz >> 16) & 255);
                packbuf[j++] = (char)((nsyn[i].q16horz >> 24) & 255);
                packbuf[k] |= 128;
            }
            k++;
            packbuf[k] = 0;
            if (nsyn[i].extbits != osyn[i].extbits) packbuf[j++] = nsyn[i].extbits, packbuf[k] |= 1;
            /*
            if ((nsyn[i].extbits^osyn[i].extbits)&0x000000ff) packbuf[j++] = (nsyn[i].extbits&255), packbuf[k] |= 1;
            if ((nsyn[i].extbits^osyn[i].extbits)&0x0000ff00) packbuf[j++] = ((nsyn[i].extbits>>8)&255), packbuf[k] |= 2;
            if ((nsyn[i].extbits^osyn[i].extbits)&0x00ff0000) packbuf[j++] = ((nsyn[i].extbits>>16)&255), packbuf[k] |= 4;
            if ((nsyn[i].extbits^osyn[i].extbits)&0xff000000) packbuf[j++] = ((nsyn[i].extbits>>24)&255), packbuf[k] |= 8; */
            k++;
        }

        /*        while (g_player[myconnectindex].syncvalhead != syncvaltail)
                {
                    packbuf[j++] = g_player[myconnectindex].syncval[syncvaltail&(MOVEFIFOSIZ-1)];
                    syncvaltail++;
                } */
        Net_AddSyncInfoToPacket(&j);

        for (int i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
            if (g_player[i].playerquitflag)
            {
                Net_SendPacket(i,packbuf,j);
                if (TEST_SYNC_KEY(nsyn[i].bits,SK_GAMEQUIT))
                    g_player[i].playerquitflag = 0;
            }

        movefifosendplc += g_movesPerPacket;
    }

}

void Net_ParsePacket(uint8_t *packbuf, int packbufleng)
{
    int i, j, k, l;
    int other = *packbuf++;
    packbufleng--;

    input_t *osyn, *nsyn;
    
    //if (numplayers < 2) return;
    //while ((packbufleng = mmulti_getpacket(&other,packbuf)) > 0)
    //{
        //lastpackettime = totalclock;
#if 0
    Printf("RECEIVED PACKET: type: %d : len %d\n", packbuf[0], packbufleng);
#endif
    switch (packbuf[0])
    {
    case PACKET_TYPE_MASTER_TO_SLAVE:  //[0] (receive master sync buffer)
        j = 1;

        if ((g_player[other].movefifoend&(TIMERUPDATESIZ-1)) == 0)
            for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
            {
                if (g_player[i].playerquitflag == 0) continue;
                if (i == myconnectindex)
                    otherminlag = (int)((signed char)packbuf[j]);
                j++;
            }

        osyn = (input_t *)&inputfifo[(g_player[connecthead].movefifoend-1)&(MOVEFIFOSIZ-1)][0];
        nsyn = (input_t *)&inputfifo[(g_player[connecthead].movefifoend)&(MOVEFIFOSIZ-1)][0];

        k = j;
        for(TRAVERSE_CONNECT(i))
        j += g_player[i].playerquitflag+g_player[i].playerquitflag;
        for(TRAVERSE_CONNECT(i))
        {
            if (g_player[i].playerquitflag == 0) continue;

            l = packbuf[k]+(int)(packbuf[k+1]<<8);
            k += 2;

            if (i == myconnectindex)
            {
                //j += ((l&1)<<1)+(l&2)+((l&4)>>2)+((l&8)>>3)+((l&16)>>4)+((l&32)>>5)+((l&64)>>6)+((l&128)>>7)+((l&256)>>8)/*+((l&512)>>9)+((l&1024)>>10)+((l&2048)>>11)*/;

                if (l & 1) j += 2;
                if (l & 2) j += 2;
                if (l & 4) j += 4;
                if (l & 8) j++;
                if (l & 16) j++;
                if (l & 32) j++;
                if (l & 64) j++;
                if (l & 128) j += 4;
                if (l & 256) j++;

                continue;
            }

            memcpy(&nsyn[i],&osyn[i],sizeof(input_t));
            if (l&1)   nsyn[i].fvel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
            if (l&2)   nsyn[i].svel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
            if (l&4)
            {
                nsyn[i].q16avel = (fix16_t)packbuf[j];
                nsyn[i].q16avel += (fix16_t)packbuf[j + 1] << 8;
                nsyn[i].q16avel += (fix16_t)packbuf[j + 2] << 16;
                nsyn[i].q16avel += (fix16_t)packbuf[j + 3] << 24;
                j += 4;
            }
            if (l&8)   nsyn[i].bits = ((nsyn[i].bits&0xffffff00)|((int)packbuf[j++]));
            if (l&16)  nsyn[i].bits = ((nsyn[i].bits&0xffff00ff)|((int)packbuf[j++])<<8);
            if (l&32)  nsyn[i].bits = ((nsyn[i].bits&0xff00ffff)|((int)packbuf[j++])<<16);
            if (l&64)  nsyn[i].bits = ((nsyn[i].bits&0x00ffffff)|((int)packbuf[j++])<<24);
            if (l&128)
            {
                nsyn[i].q16horz = (fix16_t)packbuf[j];
                nsyn[i].q16horz += (fix16_t)packbuf[j + 1] << 8;
                nsyn[i].q16horz += (fix16_t)packbuf[j + 2] << 16;
                nsyn[i].q16horz += (fix16_t)packbuf[j + 3] << 24;
                j += 4;
            }
            if (l&256)  nsyn[i].extbits = (unsigned char)packbuf[j++];
            /*                if (l&256)  nsyn[i].extbits = ((nsyn[i].extbits&0xffffff00)|((int)packbuf[j++]));
                            if (l&512)  nsyn[i].extbits = ((nsyn[i].extbits&0xffff00ff)|((int)packbuf[j++])<<8);
                            if (l&1024) nsyn[i].extbits = ((nsyn[i].extbits&0xff00ffff)|((int)packbuf[j++])<<16);
                            if (l&2048) nsyn[i].extbits = ((nsyn[i].extbits&0x00ffffff)|((int)packbuf[j++])<<24); */

            if (TEST_SYNC_KEY(nsyn[i].bits,SK_GAMEQUIT)) g_player[i].playerquitflag = 0;
            g_player[i].movefifoend++;
        }

        Net_GetSyncInfoFromPacket(packbuf, packbufleng, &j, other);

        for (TRAVERSE_CONNECT(i))
        if (i != myconnectindex)
            for (j=g_movesPerPacket-1;j>=1;j--)
            {
                memcpy(&inputfifo[g_player[i].movefifoend&(MOVEFIFOSIZ-1)][i], &nsyn[i],sizeof(input_t));
                g_player[i].movefifoend++;
            }

        movefifosendplc += g_movesPerPacket;

        break;
    case PACKET_TYPE_SLAVE_TO_MASTER:  //[1] (receive slave sync buffer)
        j = 3;
        k = packbuf[1] + (int)(packbuf[2]<<8);

        osyn = (input_t *)&inputfifo[(g_player[other].movefifoend-1)&(MOVEFIFOSIZ-1)][0];
        nsyn = (input_t *)&inputfifo[(g_player[other].movefifoend)&(MOVEFIFOSIZ-1)][0];

        memcpy(&nsyn[other], &osyn[other], sizeof(input_t));
        if (k&1)   nsyn[other].fvel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
        if (k&2)   nsyn[other].svel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
        if (k&4)
        {
            nsyn[other].q16avel = (fix16_t)packbuf[j];
            nsyn[other].q16avel += (fix16_t)packbuf[j + 1] << 8;
            nsyn[other].q16avel += (fix16_t)packbuf[j + 2] << 16;
            nsyn[other].q16avel += (fix16_t)packbuf[j + 3] << 24;
            j += 4;
        }
        if (k&8)   nsyn[other].bits = ((nsyn[other].bits&0xffffff00)|((int)packbuf[j++]));
        if (k&16)  nsyn[other].bits = ((nsyn[other].bits&0xffff00ff)|((int)packbuf[j++])<<8);
        if (k&32)  nsyn[other].bits = ((nsyn[other].bits&0xff00ffff)|((int)packbuf[j++])<<16);
        if (k&64)  nsyn[other].bits = ((nsyn[other].bits&0x00ffffff)|((int)packbuf[j++])<<24);
        if (k&128)
        {
            nsyn[other].q16horz = (fix16_t)packbuf[j];
            nsyn[other].q16horz += (fix16_t)packbuf[j + 1] << 8;
            nsyn[other].q16horz += (fix16_t)packbuf[j + 2] << 16;
            nsyn[other].q16horz += (fix16_t)packbuf[j + 3] << 24;
            j += 4;
        }
        if (k&256) nsyn[other].extbits = (unsigned char)packbuf[j++];
        /*            if (k&256)  nsyn[other].extbits = ((nsyn[other].extbits&0xffffff00)|((int)packbuf[j++]));
                    if (k&512)  nsyn[other].extbits = ((nsyn[other].extbits&0xffff00ff)|((int)packbuf[j++])<<8);
                    if (k&1024) nsyn[other].extbits = ((nsyn[other].extbits&0xff00ffff)|((int)packbuf[j++])<<16);
                    if (k&2048) nsyn[other].extbits = ((nsyn[other].extbits&0x00ffffff)|((int)packbuf[j++])<<24); */
        g_player[other].movefifoend++;

        /*            while (j != packbufleng)
                    {
                        g_player[other].syncval[g_player[other].syncvalhead&(MOVEFIFOSIZ-1)] = packbuf[j++];
                        g_player[other].syncvalhead++;
                    } */
        Net_GetSyncInfoFromPacket(packbuf, packbufleng, &j, other);

        for (i=g_movesPerPacket-1;i>=1;i--)
        {
            memcpy(&inputfifo[g_player[other].movefifoend&(MOVEFIFOSIZ-1)][other], &nsyn[other], sizeof(input_t));
            g_player[other].movefifoend++;
        }

        break;

    case PACKET_TYPE_BROADCAST:
        g_player[other].movefifoend = movefifoplc = movefifosendplc = predictfifoplc = 0;
        g_player[other].syncvalhead = syncvaltottail = 0L;
    case SERVER_GENERATED_BROADCAST:
        j = 1;

        if ((g_player[other].movefifoend&(TIMERUPDATESIZ-1)) == 0)
            if (other == connecthead)
                for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                {
                    if (i == myconnectindex)
                        otherminlag = (int)((signed char)packbuf[j]);
                    j++;
                }

        osyn = (input_t *)&inputfifo[(g_player[other].movefifoend-1)&(MOVEFIFOSIZ-1)][0];
        nsyn = (input_t *)&inputfifo[(g_player[other].movefifoend)&(MOVEFIFOSIZ-1)][0];

        memcpy(&nsyn[other], &osyn[other], sizeof(input_t));
        k = packbuf[j] + (int)(packbuf[j+1]<<8);
        j += 2;

        if (k&1)   nsyn[other].fvel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
        if (k&2)   nsyn[other].svel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
        if (k&4)
        {
            nsyn[other].q16avel = (fix16_t)packbuf[j];
            nsyn[other].q16avel += (fix16_t)packbuf[j + 1] << 8;
            nsyn[other].q16avel += (fix16_t)packbuf[j + 2] << 16;
            nsyn[other].q16avel += (fix16_t)packbuf[j + 3] << 24;
            j += 4;
        }
        if (k&8)   nsyn[other].bits = ((nsyn[other].bits&0xffffff00)|((int)packbuf[j++]));
        if (k&16)  nsyn[other].bits = ((nsyn[other].bits&0xffff00ff)|((int)packbuf[j++])<<8);
        if (k&32)  nsyn[other].bits = ((nsyn[other].bits&0xff00ffff)|((int)packbuf[j++])<<16);
        if (k&64)  nsyn[other].bits = ((nsyn[other].bits&0x00ffffff)|((int)packbuf[j++])<<24);
        if (k&128)
        {
            nsyn[other].q16horz = (fix16_t)packbuf[j];
            nsyn[other].q16horz += (fix16_t)packbuf[j + 1] << 8;
            nsyn[other].q16horz += (fix16_t)packbuf[j + 2] << 16;
            nsyn[other].q16horz += (fix16_t)packbuf[j + 3] << 24;
            j += 4;
        }

        if (k&256) nsyn[other].extbits = (unsigned char)packbuf[j++];
        /*            if (k&256)  nsyn[other].extbits = ((nsyn[other].extbits&0xffffff00)|((int)packbuf[j++]));
                    if (k&512)  nsyn[other].extbits = ((nsyn[other].extbits&0xffff00ff)|((int)packbuf[j++])<<8);
                    if (k&1024) nsyn[other].extbits = ((nsyn[other].extbits&0xff00ffff)|((int)packbuf[j++])<<16);
                    if (k&2048) nsyn[other].extbits = ((nsyn[other].extbits&0x00ffffff)|((int)packbuf[j++])<<24); */
        g_player[other].movefifoend++;

        for (i=g_movesPerPacket-1;i>=1;i--)
        {
            memcpy(&inputfifo[g_player[other].movefifoend&(MOVEFIFOSIZ-1)][other], &nsyn[other], sizeof(input_t));
            g_player[other].movefifoend++;
        }

        /*
                    while (j < packbufleng)
                    {
                        g_player[other].syncval[g_player[other].syncvalhead&(MOVEFIFOSIZ-1)] = packbuf[j++];
                        g_player[other].syncvalhead++;
                    }
                    */
        Net_GetSyncInfoFromPacket(packbuf, packbufleng, &j, other);

        if (j > packbufleng)
            Printf("INVALID GAME PACKET!!! (packet %d, %d too many bytes (%d %d))\n",packbuf[0],j-packbufleng,packbufleng,k);

        break;
    case PACKET_TYPE_NULL_PACKET:
        break;

    case PACKET_TYPE_PLAYER_READY:
        if (g_player[other].playerreadyflag == 0)
            Printf("Player %d is ready\n", other);
        g_player[other].playerreadyflag++;
        return;
    //case PACKET_TYPE_QUIT:
    //    G_GameExit(" ");
    //    break;

    }
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
        Printf("An error occurred while trying to create an ENet client host.\n");
        return;
    }

    addrstr = strtok(oursrvaddr, ":");
    enet_address_set_host(&address, addrstr);
    addrstr = strtok(NULL, ":");
    address.port = addrstr==NULL ? g_netPort : Batoi(addrstr);

    g_netClientPeer = enet_host_connect(g_netClient, &address, CHAN_MAX, 0);

    if (g_netClientPeer == NULL)
    {
        Printf("No available peers for initiating an ENet connection.\n");
        return;
    }

    for (i=4; i>0; i--)
    {
        /* Wait up to 5 seconds for the connection attempt to succeed. */
        if (enet_host_service(g_netClient, & event, 5000) > 0 &&
                event.type == ENET_EVENT_TYPE_CONNECT)
        {
            Printf("Connection to %s:%d succeeded.\n", oursrvaddr, address.port);
            Xfree(oursrvaddr);
            return;
        }
        else
        {
            /* Either the 5 seconds are up or a disconnect event was */
            /* received. Reset the peer in the event the 5 seconds   */
            /* had run out without any significant event.            */
            enet_peer_reset(g_netClientPeer);
            Printf("Connection to %s:%d failed.\n", oursrvaddr, address.port);
        }
        Printf(i ? "Retrying...\n" : "Giving up connection attempt.\n");
    }

    Xfree(oursrvaddr);
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
					
			default:
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
			default:
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
    case DISC_GAME_STARTED:
        Printf("Game already started.\n");
        return;
    case DISC_BAD_PASSWORD:
        Printf("Bad password.\n");
        return;
    case DISC_VERSION_MISMATCH:
        Printf("Version mismatch.\n");
        return;
    case DISC_INVALID:
        Printf("Invalid data detected.\n");
        return;
    case DISC_SERVER_QUIT:
        Printf("The server is quitting.\n");
        return;
    case DISC_SERVER_FULL:
        Printf("The server is full.\n");
        return;
    case DISC_KICKED:
        Printf("You have been kicked from the server.\n");
        return;
    case DISC_BANNED:
        Printf("You are banned from this server.\n");
        return;
    default:
        Printf("Disconnected.\n");
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Packet Handlers

#endif

void Net_GetPackets(void)
{
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

static void P_RemovePlayer(int32_t p);

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

            Printf("A new client connected from %s:%u.\n", ipaddr, event.peer->address.port);

            Net_SendAcknowledge(event.peer);
            break;
        }

        case ENET_EVENT_TYPE_RECEIVE:
            /*
            Printf ("A packet of length %u containing %s was received from player %d on channel %u.\n",
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
            //g_player[playeridx].ping = (event.peer->lastRoundTripTime + event.peer->roundTripTime)/2;
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

            Printf("%s disconnected.\n", g_player[playeridx].user_name);
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
    switch (event->channelID)
    {
    case CHAN_REROUTE:
    {
        char *packet = (char*)event->packet->data;
        int dest = *packet++;
        int source = *packet++;
        int size = event->packet->dataLength-2;
        if (g_netPlayerPeer[dest])
        {
            ENetPacket *netpacket = enet_packet_create(NULL, size+1, ENET_PACKET_FLAG_RELIABLE);
            char *buffer = (char*)netpacket->data;
            *buffer++ = source;
            Bmemcpy(buffer, packet, size);
            enet_peer_send(g_netPlayerPeer[dest], CHAN_GAMESTATE, netpacket);
            enet_host_service(g_netServer, NULL, 0);
        }
        break;
    }
    case CHAN_GAME:
        Net_ParsePacket(event->packet->data, event->packet->dataLength);
        break;
    default:
    {
        uint8_t *pbuf = event->packet->data;
        int32_t packbufleng = event->packet->dataLength;
        int32_t other = pbuf[--packbufleng];
        switch (pbuf[0])
        {
        //case PACKET_SLAVE_TO_MASTER:  //[1] (receive slave sync buffer)
        //    Net_ReceiveClientUpdate(event);
        //    break;

        //case PACKET_PLAYER_READY:
        //
        //    if (other == 0)
        //    {
        //        break;
        //    }
        //
        //    j = g_player[other].ps->i;
        //    Bmemcpy(g_player[other].ps, g_player[0].ps, sizeof(DukePlayer_t));
        //
        //    g_player[other].ps->i = j;
        //    changespritestat(j, STAT_PLAYER);
        //
        //    g_player[other].ps->last_extra = sprite[g_player[other].ps->i].extra = g_player[other].ps->max_player_health;
        //    sprite[g_player[other].ps->i].cstat = 1+256;
        //    actor[g_player[other].ps->i].t_data[2] = actor[g_player[other].ps->i].t_data[3] = actor[g_player[other].ps->i].t_data[4] = 0;
        //
        //    P_ResetPlayer(other);
        //    Net_SpawnPlayer(other);
        //
        //    break;

        //case PACKET_PLAYER_PING:
        //    if (g_player[myconnectindex].ps->gm & MODE_GAME)
        //    {
        //        packbuf[0] = PACKET_PLAYER_PING;
        //        packbuf[1] = myconnectindex;
        //        enet_peer_send(event->peer, CHAN_GAMESTATE, enet_packet_create(packbuf, 2, ENET_PACKET_FLAG_RELIABLE));
        //    }
        //    g_player[other].pingcnt++;
        //    break;

        case PACKET_AUTH:
            Net_ReceiveChallenge(pbuf, packbufleng, event);
            break;

        default:
            Net_ParsePacketCommon(pbuf, packbufleng, 0);
            break;
        }
        break;
    }
    }

#if 0
    Printf("Received Packet: type: %d : len %d\n", pbuf[0], packbufleng);
#endif
}

void Net_ParseServerPacket(ENetEvent *event)
{
    if (event->channelID == CHAN_GAME)
    {
        Net_ParsePacket(event->packet->data, event->packet->dataLength);
        return;
    }
    uint8_t *pbuf = event->packet->data;
    int32_t packbufleng = event->packet->dataLength;
   // input_t *nsyn;

    --packbufleng;  //    int32_t other = pbuf[--packbufleng];

#if 0
    Printf("Received Packet: type: %d : len %d\n", pbuf[0], packbufleng);
#endif
    switch (pbuf[0])
    {

    //case PACKET_MAP_STREAM:
    //
    //    if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
    //        return;
    //
    //    Net_ReceiveMapUpdate(event);
    //
    //    break;

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
        //if ((g_player[myconnectindex].ps->gm & MODE_GAME))
        P_RemovePlayer(pbuf[1]);
        numplayers = pbuf[2];
        ud.multimode = pbuf[3];
        g_mostConcurrentPlayers = pbuf[4];
        break;

    //case PACKET_PLAYER_SPAWN:
    //    if (!(g_player[myconnectindex].ps->gm & MODE_GAME)) break;
    //
    //    P_ResetPlayer(pbuf[1]);
    //    Bmemcpy(&g_player[pbuf[1]].ps->pos.x, &pbuf[2], sizeof(vec3_t) * 2);
    //    Bmemcpy(&sprite[g_player[pbuf[1]].ps->i], &pbuf[2], sizeof(vec3_t));
    //    break;

    //case PACKET_PLAYER_PING:
    //    g_player[0].pingcnt++;
    //    return;

    //case PACKET_FRAG:
    //    if (!(g_player[myconnectindex].ps->gm & MODE_GAME)) break;
    //    g_player[pbuf[1]].ps->frag_ps = pbuf[2];
    //    actor[g_player[pbuf[1]].ps->i].picnum = pbuf[3];
    //    ticrandomseed = B_UNBUF32(&pbuf[4]);
    //    P_FragPlayer(pbuf[1]);
    //    break;

    default:
        Net_ParsePacketCommon(pbuf, packbufleng, 1);
        break;
    }
}

void Net_SendPacket(int dest, uint8_t *pbuf, int32_t packbufleng)
{
    if (g_networkMode == NET_SERVER)
    {
        if (!g_netServer)
            return;
        if (g_netPlayerPeer[dest] != NULL)
        {
            ENetPacket* packet = enet_packet_create(NULL, packbufleng+1, ENET_PACKET_FLAG_RELIABLE);
            uint8_t* buffer = packet->data;
            *buffer++ = myconnectindex;
            Bmemcpy(buffer, pbuf, packbufleng);
            enet_peer_send(g_netPlayerPeer[dest], CHAN_GAME, packet);
            enet_host_service(g_netServer, NULL, 0);
        }
    }
    if (g_networkMode == NET_CLIENT)
    {
        if (!g_netClient)
            return;
        if (dest == 0)
        {
            ENetPacket *packet = enet_packet_create(NULL, packbufleng+1, ENET_PACKET_FLAG_RELIABLE);
            uint8_t* buffer = packet->data;
            *buffer++ = myconnectindex;
            Bmemcpy(buffer, pbuf, packbufleng);
            enet_peer_send(g_netClientPeer, CHAN_GAME, packet);
        }
        else
        {
            ENetPacket * packet = enet_packet_create(NULL, packbufleng+2, ENET_PACKET_FLAG_RELIABLE);
            uint8_t* buffer = packet->data;
            *buffer++ = dest;
            *buffer++ = myconnectindex;
            Bmemcpy(buffer, pbuf, packbufleng);
            enet_peer_send(g_netClientPeer, CHAN_REROUTE, packet);
        }
        enet_host_service(g_netClient, NULL, 0);
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

    tempbuf[0] = PACKET_ACK;
    tempbuf[1] = myconnectindex;

    enet_peer_send(client, CHAN_GAMESTATE, enet_packet_create(&tempbuf[0], 2, ENET_PACKET_FLAG_RELIABLE));
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

    tempbuf[0] = PACKET_AUTH;
    B_BUF16(&tempbuf[1], BYTEVERSION);
    B_BUF16(&tempbuf[3], NETVERSION);
    B_BUF32(&tempbuf[5], Bcrc32((uint8_t *)g_netPassword, Bstrlen(g_netPassword), 0));
    tempbuf[9] = myconnectindex;

    enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&tempbuf[0], 10, ENET_PACKET_FLAG_RELIABLE));
}

void Net_ReceiveChallenge(uint8_t *pbuf, int32_t packbufleng, ENetEvent *event)
{
    const uint16_t byteVersion = B_UNBUF16(&pbuf[1]);
    const uint16_t netVersion = B_UNBUF16(&pbuf[3]);
    const uint32_t crc = B_UNBUF32(&pbuf[5]);

    UNREFERENCED_PARAMETER(packbufleng); // remove when this variable is used

    if (g_player[myconnectindex].ps->gm&MODE_GAME)
    {
        enet_peer_disconnect_later(event->peer, DISC_GAME_STARTED);
        Printf("Client attempted to connect to started game\n");
        return;
    }

    if (byteVersion != BYTEVERSION || netVersion != NETVERSION)
    {
        enet_peer_disconnect_later(event->peer, DISC_VERSION_MISMATCH);
        Printf("Bad client protocol: version %u.%u\n", byteVersion, netVersion);
        return;
    }
    if (crc != Bcrc32((uint8_t *)g_netPassword, Bstrlen(g_netPassword), 0))
    {
        enet_peer_disconnect_later(event->peer, DISC_BAD_PASSWORD);
        Printf("Bad password from client.\n");
        return;
    }

    Net_SyncPlayer(event);
}

////////////////////////////////////////////////////////////////////////////////
// Num Players Packets

static void P_RemovePlayer(int32_t p)
{
    // server obviously can't leave the game, and index 0 shows up for disconnect events from
    // players that haven't gotten far enough into the connection process to get a player ID

    if (p <= 0) return;

    g_player[p].playerquitflag = 0;

    Bsprintf(recbuf,"%s^00 is history!",g_player[p].user_name);
    G_AddUserQuote(recbuf);

    if (numplayers == 1)
        S_PlaySound(GENERIC_AMBIENCE17);
}

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
        if (!g_player[pbuf[4]].input)
        {
            g_player[pbuf[4]].input = (input_t *) Xcalloc(1,sizeof(input_t));
        }
    }

//#ifndef NETCODE_DISABLE
//    if (pbuf[5] == NET_DEDICATED_SERVER)
//    {
//        g_networkMode = NET_DEDICATED_CLIENT;
//    }
//#endif

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
    g_netPlayerPeer[index] = peer;
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

	strncpy(g_player[myconnectindex].user_name, playername, 32);

    if (numplayers < 2) return;

    tempbuf[0] = PACKET_CLIENT_INFO;
    l = 1;

    //null terminated player name to send
	strncpy(tempbuf + l, playername, 32);
	l += 32;
	tempbuf[l++] = 0;

    tempbuf[l++] = g_player[myconnectindex].ps->aim_mode = in_mousemode;
    tempbuf[l++] = g_player[myconnectindex].ps->auto_aim = cl_autoaim;
    tempbuf[l++] = g_player[myconnectindex].ps->weaponswitch = cl_weaponswitch;
    tempbuf[l++] = g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = G_CheckPlayerColor(playercolor);

    tempbuf[l++] = g_player[myconnectindex].pteam = playerteam;

    for (i=0; i<10; i++)
    {
        g_player[myconnectindex].wchoice[i] = g_player[0].wchoice[i];
        tempbuf[l++] = (uint8_t)g_player[0].wchoice[i];
    }

    tempbuf[l++] = myconnectindex;

    if (g_netClient)
    {
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&tempbuf[0], l, ENET_PACKET_FLAG_RELIABLE));
    }
    else if (g_netServer)
    {
        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(&tempbuf[0], l, ENET_PACKET_FLAG_RELIABLE));
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
    Bstrcpy(boardfilename,(char *)pbuf+1);
    boardfilename[packbufleng-1] = 0;
    if (boardfilename[0] != 0)
    {
        if (fileSystem.FileExists(boardfilename))
        {
            Bmemset(boardfilename,0,sizeof(boardfilename));
            Net_SendUserMapName();
        }
    }

    if (m_level_number == 7 && ud.m_volume_number == 0 && boardfilename[0] == 0)
        m_level_number = 0;
}

void Net_SendMessage(void)
{
#if 0
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

            if (inputState.keyBufferWaiting())
            {
                i = inputState.keyGetChar();

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

                inputState.ClearKeyStatus(sc_1);
                inputState.ClearKeyStatus(sc_2);
                inputState.ClearKeyStatus(sc_3);
                inputState.ClearKeyStatus(sc_4);
                inputState.ClearKeyStatus(sc_5);
                inputState.ClearKeyStatus(sc_6);
                inputState.ClearKeyStatus(sc_7);
                inputState.ClearKeyStatus(sc_8);
                inputState.ClearKeyStatus(sc_A);
                inputState.ClearKeyStatus(sc_Escape);
                inputState.ClearKeyStatus(sc_Enter);
            }
        }
    }
#endif
}

void Net_ReceiveMessage(uint8_t *pbuf, int32_t packbufleng)
{
    Bstrncpy(recbuf, (char *)pbuf+2, packbufleng-2);
    recbuf[packbufleng-2] = 0;

    G_AddUserQuote(recbuf);
    S_PlaySound(EXITMENUSOUND, CHAN_AUTO, CHANF_UI);

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
        //g_player[i].revision = 0;
    }

    Net_ExtractNewGame(&pendingnewgame, 0);
    G_NewGame(ud.volume_number,ud.level_number,ud.player_skill);
    ud.coop = m_coop;

    //g_netMapRevision = 0;

    if (G_EnterLevel(MODE_GAME))
    {
        G_BackToMenu();
    }
}

void Net_NotifyNewGame()
{
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

    //packbuf[0] = PACKET_PLAYER_READY;
    //packbuf[1] = myconnectindex;
    //
    //if (g_netClientPeer)
    //{
    //    enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(packbuf, 2, ENET_PACKET_FLAG_RELIABLE));
    //}
    //
    //g_player[myconnectindex].ps->gm = MODE_GAME;
    //ready2send = 1;
}

void Net_FillNewGame(newgame_t *newgame, int32_t frommenu)
{
    if (frommenu)
    {
        newgame->level_number = m_level_number;
        newgame->volume_number = ud.m_volume_number;
        newgame->player_skill = ud.m_player_skill;
        newgame->monsters_off = ud.m_monsters_off;
        newgame->respawn_monsters = ud.m_respawn_monsters;
        newgame->respawn_items = ud.m_respawn_items;
        newgame->respawn_inventory = ud.m_respawn_inventory;
        newgame->ffire = m_ffire;
        newgame->noexits = m_noexits;
        newgame->coop = m_coop;
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
    m_level_number = newgame->level_number;
    ud.m_volume_number = newgame->volume_number;
    ud.m_player_skill = newgame->player_skill;
    ud.m_monsters_off = newgame->monsters_off;
    ud.m_respawn_monsters = newgame->respawn_monsters;
    ud.m_respawn_items = newgame->respawn_items;
    ud.m_respawn_inventory = newgame->respawn_inventory;
    m_ffire = newgame->ffire;
    m_noexits = newgame->noexits;
    m_coop = newgame->coop;

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
             mapList[(uint8_t)(vote_episode*MAXLEVELS + vote_map)].DisplayName(),
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

void Net_SendTaunt(int ridiculeNum)
{
	tempbuf[0] = PACKET_MESSAGE;
	tempbuf[1] = 255;
	tempbuf[2] = 0;
	Bstrcat(tempbuf + 2, *CombatMacros[ridiculeNum - 1]);

	ridiculeNum = 2 + strlen(*CombatMacros[ridiculeNum - 1]);

	tempbuf[ridiculeNum++] = myconnectindex;

	if (g_netClient)
		enet_peer_send(g_netClientPeer, CHAN_CHAT, enet_packet_create(&tempbuf[0], ridiculeNum, 0));
	else if (g_netServer)
		enet_host_broadcast(g_netServer, CHAN_CHAT, enet_packet_create(&tempbuf[0], ridiculeNum, 0));

}

void Net_SendRTS(int ridiculeNum)
{
	if ((g_netServer || ud.multimode > 1))
	{
		tempbuf[0] = PACKET_RTS;
		tempbuf[1] = ridiculeNum;
		tempbuf[2] = myconnectindex;

		if (g_netClient)
			enet_peer_send(g_netClientPeer, CHAN_CHAT, enet_packet_create(&tempbuf[0], 3, 0));
		else if (g_netServer)
			enet_host_broadcast(g_netServer, CHAN_CHAT, enet_packet_create(&tempbuf[0], 3, 0));
	}
}

void Net_InitNetwork()
{
    if (g_networkMode == NET_SERVER/* || g_networkMode == NET_DEDICATED_SERVER*/)
    {
        ENetAddress address = { ENET_HOST_ANY, g_netPort };
        g_netServer = enet_host_create(&address, MAXPLAYERS, CHAN_MAX, 0, 0);

        if (g_netServer == NULL)
            Printf("An error occurred while trying to create an ENet server host.\n");
        else Printf("Multiplayer server initialized\n");
    }
}

void Net_PrintLag(FString& output)
{
    // lag meter
    if (g_netClientPeer)
    {
        output.AppendFormat("%d +- %d ms\n", (g_netClientPeer->lastRoundTripTime + g_netClientPeer->roundTripTime) / 2,
            (g_netClientPeer->lastRoundTripTimeVariance + g_netClientPeer->roundTripTimeVariance) / 2);
    }
}

int osdcmd_listplayers(CCmdFuncPtr parm)
{
    ENetPeer* currentPeer;
    char ipaddr[32];

    if (parm && parm->numparms != 0)
        return OSDCMD_SHOWHELP;

    if (!g_netServer)
    {
        Printf("You are not the server.\n");
        return OSDCMD_OK;
    }

    Printf("Connected clients:\n");

    for (currentPeer = g_netServer->peers;
        currentPeer < &g_netServer->peers[g_netServer->peerCount];
        ++currentPeer)
    {
        if (currentPeer->state != ENET_PEER_STATE_CONNECTED)
            continue;

        enet_address_get_host_ip(&currentPeer->address, ipaddr, sizeof(ipaddr));
        Printf("%s %s\n", ipaddr,
            g_player[(intptr_t)currentPeer->data].user_name);
    }

    return OSDCMD_OK;
}

#if 0
static int osdcmd_kick(CCmdFuncPtr parm)
{
    ENetPeer* currentPeer;
    uint32_t hexaddr;

    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    if (!g_netServer)
    {
        Printf("You are not the server.\n");
        return OSDCMD_OK;
    }

    for (currentPeer = g_netServer->peers;
        currentPeer < &g_netServer->peers[g_netServer->peerCount];
        ++currentPeer)
    {
        if (currentPeer->state != ENET_PEER_STATE_CONNECTED)
            continue;

        sscanf(parm->parms[0], "%" SCNx32 "", &hexaddr);

        if (currentPeer->address.host == hexaddr)
        {
            Printf("Kicking %x (%s)\n", currentPeer->address.host,
                g_player[(intptr_t)currentPeer->data].user_name);
            enet_peer_disconnect(currentPeer, DISC_KICKED);
            return OSDCMD_OK;
        }
    }

    Printf("Player %s not found!\n", parm->parms[0]);
    osdcmd_listplayers(NULL);

    return OSDCMD_OK;
}

static int osdcmd_kickban(CCmdFuncPtr parm)
{
    ENetPeer* currentPeer;
    uint32_t hexaddr;

    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    if (!g_netServer)
    {
        Printf("You are not the server.\n");
        return OSDCMD_OK;
    }

    for (currentPeer = g_netServer->peers;
        currentPeer < &g_netServer->peers[g_netServer->peerCount];
        ++currentPeer)
    {
        if (currentPeer->state != ENET_PEER_STATE_CONNECTED)
            continue;

        sscanf(parm->parms[0], "%" SCNx32 "", &hexaddr);

        // TODO: implement banning logic

        if (currentPeer->address.host == hexaddr)
        {
            char ipaddr[32];

            enet_address_get_host_ip(&currentPeer->address, ipaddr, sizeof(ipaddr));
            Printf("Host %s is now banned.\n", ipaddr);
            Printf("Kicking %x (%s)\n", currentPeer->address.host,
                g_player[(intptr_t)currentPeer->data].user_name);
            enet_peer_disconnect(currentPeer, DISC_BANNED);
            return OSDCMD_OK;
        }
    }

    Printf("Player %s not found!\n", parm->parms[0]);
    osdcmd_listplayers(NULL);

    return OSDCMD_OK;
}
#endif


#endif  // !defined NETCODE_DISABLE

END_DUKE_NS
