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

#include "cheats.h"
#include "cmdline.h"
#include "demo.h"  // g_firstDemoFile[]
#include "duke3d.h"
#include "menus.h"
#include "osdcmds.h"
#include "savegame.h"
#include "sbar.h"
#include "mapinfo.h"

BEGIN_RR_NS

struct osdcmd_cheatsinfo osdcmd_cheatsinfo_stat = { -1, 0, 0 };

static int osdcmd_map(osdcmdptr_t parm)
{
    char filename[BMAX_PATH];

    const int32_t wildcardp = parm->numparms==1 &&
        (Bstrchr(parm->parms[0], '*') != NULL);

    if (parm->numparms != 1 || wildcardp)
    {
        return OSDCMD_SHOWHELP;
    }

    maybe_append_ext(filename, sizeof(filename), parm->parms[0], ".map");

    if (!fileSystem.FileExists(filename))
    {
        OSD_Printf(OSD_ERROR "map: file \"%s\" not found.\n", filename);
        return OSDCMD_OK;
    }

    boardfilename[0] = '/';
    boardfilename[1] = 0;
    strcat(boardfilename, filename);

    if (numplayers > 1)
    {
        return OSDCMD_OK;
    }

    osdcmd_cheatsinfo_stat.cheatnum = -1;
    ud.m_volume_number = 0;
    m_level_number = 7;

    ud.m_monsters_off = ud.monsters_off = 0;

    ud.m_respawn_items = 0;
    ud.m_respawn_inventory = 0;

    ud.multimode = 1;

    if (g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        G_NewGame(ud.m_volume_number, m_level_number, ud.m_player_skill);
        g_player[myconnectindex].ps->gm = MODE_RESTART;
    }
    else G_NewGame_EnterLevel();

    return OSDCMD_OK;
}

// demo <demonum or demofn> [<prof>]
//
// To profile a demo ("timedemo mode"), <prof> can be given in the range 0-8,
// which will start to replay it as fast as possible, rendering <prof> frames
// for each gametic.
//
// Notes:
//  * The demos should be recorded with demorec_diffs set to 0, so that the
//    game state updates are actually computed.
//  * Currently, the profiling can only be aborted on SDL 1.2 builds by
//    pressing any key.
//  * With <prof> greater than 1, interpolation should be calculated properly,
//    though this has not been verified by looking at the frames.
//  * When testing whether a change in the source has an effect on performance,
//    the variance of the run times MUST be taken into account (that is, the
//    replaying must be performed multiple times for the old and new versions,
//    etc.)
static int osdcmd_demo(osdcmdptr_t parm)
{
    if (numplayers > 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    if (g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        OSD_Printf("demo: Must not be in a game.\n");
        return OSDCMD_OK;
    }

    if (parm->numparms != 1 && parm->numparms != 2)
        return OSDCMD_SHOWHELP;

    {
        int32_t prof = parm->numparms==2 ? Batoi(parm->parms[1]) : -1;

        Demo_SetFirst(parm->parms[0]);
        Demo_PlayFirst(clamp(prof, -1, 8)+1, 0);
    }

    return OSDCMD_OK;
}

static int osdcmd_activatecheat(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
        osdcmd_cheatsinfo_stat.cheatnum = Batoi(parm->parms[0]);
    else
        OSD_Printf("activatecheat: Not in a single-player game.\n");

    return OSDCMD_OK;
}

static int osdcmd_god(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_CORNHOLIO;
    else
        OSD_Printf("god: Not in a single-player game.\n");

    return OSDCMD_OK;
}

static int osdcmd_noclip(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_CLIP;
    }
    else
    {
        OSD_Printf("noclip: Not in a single-player game.\n");
    }

    return OSDCMD_OK;
}

static int osdcmd_restartsound(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    S_SoundShutdown();

    S_SoundStartup();

    FX_StopAllSounds();
    S_ClearSoundLocks();

    return OSDCMD_OK;
}

int osdcmd_restartmap(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    if (g_player[myconnectindex].ps->gm & MODE_GAME && ud.multimode == 1)
        g_player[myconnectindex].ps->gm = MODE_RESTART;

    return OSDCMD_OK;
}

static int osdcmd_vidmode(osdcmdptr_t parm)
{
    int32_t newbpp = ScreenBPP, newwidth = ScreenWidth,
            newheight = ScreenHeight, newfs = ScreenMode;
    int32_t tmp;

    if (parm->numparms < 1 || parm->numparms > 4) return OSDCMD_SHOWHELP;

    switch (parm->numparms)
    {
    case 1: // bpp switch
        tmp = Batol(parm->parms[0]);
        if (!(tmp==8 || tmp==16 || tmp==32))
            return OSDCMD_SHOWHELP;
        newbpp = tmp;
        break;
    case 2: // res switch
        newwidth = Batol(parm->parms[0]);
        newheight = Batol(parm->parms[1]);
        break;
    case 3: // res & bpp switch
    case 4:
        newwidth = Batol(parm->parms[0]);
        newheight = Batol(parm->parms[1]);
        tmp = Batol(parm->parms[2]);
        if (!(tmp==8 || tmp==16 || tmp==32))
            return OSDCMD_SHOWHELP;
        newbpp = tmp;
        if (parm->numparms == 4)
            newfs = (Batol(parm->parms[3]) != 0);
        break;
    }

    if (videoSetGameMode(newfs,newwidth,newheight,newbpp,upscalefactor))
    {
        initprintf("vidmode: Mode change failed!\n");
        if (videoSetGameMode(ScreenMode, ScreenWidth, ScreenHeight, ScreenBPP, upscalefactor))
            G_GameExit("vidmode: Reset failed!\n");
    }
    ScreenBPP = newbpp;
    ScreenWidth = newwidth;
    ScreenHeight = newheight;
    ScreenMode = newfs;
    onvideomodechange(ScreenBPP>8);
    G_UpdateScreenArea();
    return OSDCMD_OK;
}

static int osdcmd_spawn(osdcmdptr_t parm)
{
    int32_t picnum = 0;
    uint16_t cstat=0;
    char pal=0;
    int16_t ang=0;
    int16_t set=0, idx;
    vec3_t vect;

    if (numplayers > 1 || !(g_player[myconnectindex].ps->gm & MODE_GAME))
    {
        OSD_Printf("spawn: Can't spawn sprites in multiplayer games or demos\n");
        return OSDCMD_OK;
    }

    switch (parm->numparms)
    {
    case 7: // x,y,z
        vect.x = Batol(parm->parms[4]);
        vect.y = Batol(parm->parms[5]);
        vect.z = Batol(parm->parms[6]);
        set |= 8;
        fallthrough__;
    case 4: // ang
        ang = Batol(parm->parms[3]) & 2047;
        set |= 4;
        fallthrough__;
    case 3: // cstat
        cstat = (uint16_t)Batol(parm->parms[2]);
        set |= 2;
        fallthrough__;
    case 2: // pal
        pal = (uint8_t)Batol(parm->parms[1]);
        set |= 1;
        fallthrough__;
    case 1: // tile number
        if (isdigit(parm->parms[0][0]))
        {
            picnum = Batol(parm->parms[0]);
        }
        else
        {
            int32_t i;
            int32_t j;

            for (j=0; j<2; j++)
            {
                for (i=0; i<g_labelCnt; i++)
                {
                    if ((j == 0 && !Bstrcmp(label+(i<<6),     parm->parms[0])) ||
                        (j == 1 && !Bstrcasecmp(label+(i<<6), parm->parms[0])))
                    {
                        picnum = labelcode[i];
                        break;
                    }
                }

                if (i < g_labelCnt)
                    break;
            }
            if (i==g_labelCnt)
            {
                OSD_Printf("spawn: Invalid tile label given\n");
                return OSDCMD_OK;
            }
        }

        if ((uint32_t)picnum >= MAXUSERTILES)
        {
            OSD_Printf("spawn: Invalid tile number\n");
            return OSDCMD_OK;
        }
        break;

    default:
        return OSDCMD_SHOWHELP;
    }

    idx = A_Spawn(g_player[myconnectindex].ps->i, picnum);
    if (set & 1) sprite[idx].pal = (uint8_t)pal;
    if (set & 2) sprite[idx].cstat = (int16_t)cstat;
    if (set & 4) sprite[idx].ang = ang;
    if (set & 8)
    {
        if (setsprite(idx, &vect) < 0)
        {
            OSD_Printf("spawn: Sprite can't be spawned into null space\n");
            A_DeleteSprite(idx);
        }
    }

    return OSDCMD_OK;
}


static int osdcmd_crosshaircolor(osdcmdptr_t parm)
{
    if (parm->numparms != 3)
    {
        OSD_Printf("crosshaircolor: r:%d g:%d b:%d\n",CrosshairColors.r,CrosshairColors.g,CrosshairColors.b);
        return OSDCMD_SHOWHELP;
    }

    uint8_t const r = Batol(parm->parms[0]);
    uint8_t const g = Batol(parm->parms[1]);
    uint8_t const b = Batol(parm->parms[2]);

    G_SetCrosshairColor(r,g,b);

	OSD_Printf("%s\n", parm->raw);

    return OSDCMD_OK;
}

static int osdcmd_give(osdcmdptr_t parm)
{
    int32_t i;

    if (numplayers != 1 || (g_player[myconnectindex].ps->gm & MODE_GAME) == 0 ||
            g_player[myconnectindex].ps->dead_flag != 0)
    {
        OSD_Printf("give: Cannot give while dead or not in a single-player game.\n");
        return OSDCMD_OK;
    }

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    if (!Bstrcasecmp(parm->parms[0], "all"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_STUFF;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "health"))
    {
        sprite[g_player[myconnectindex].ps->i].extra = g_player[myconnectindex].ps->max_player_health<<1;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "weapons"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_WEAPONS;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "ammo"))
    {
        for (i=MAX_WEAPONS-(VOLUMEONE?6:1)-1; i>=PISTOL_WEAPON; i--)
            P_AddAmmo(g_player[myconnectindex].ps,i,g_player[myconnectindex].ps->max_ammo_amount[i]);
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "armor"))
    {
        g_player[myconnectindex].ps->inv_amount[GET_SHIELD] = 100;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "keys"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_KEYS;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "inventory"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_INVENTORY;
        return OSDCMD_OK;
    }
    return OSDCMD_SHOWHELP;
}

void onvideomodechange(int32_t newmode)
{
    uint8_t palid;

    // XXX?
    if (!newmode || g_player[screenpeek].ps->palette < BASEPALCOUNT)
        palid = g_player[screenpeek].ps->palette;
    else
        palid = BASEPAL;

#ifdef POLYMER
    if (videoGetRenderMode() == REND_POLYMER)
    {
        int32_t i = 0;

        while (i < MAXSPRITES)
        {
            if (actor[i].lightptr)
            {
                polymer_deletelight(actor[i].lightId);
                actor[i].lightptr = NULL;
                actor[i].lightId = -1;
            }
            i++;
        }
    }
#endif

    videoSetPalette(0, palid, 0);
    g_restorePalette = -1;
    g_crosshairSum = -1;
}


#if !defined NETCODE_DISABLE
static int osdcmd_disconnect(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    // NUKE-TODO:
    if (g_player[myconnectindex].ps->gm&MODE_MENU)
        g_netDisconnect = 1;
    return OSDCMD_OK;
}

static int osdcmd_connect(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    Net_Connect(parm->parms[0]);
    G_BackToMenu();
    return OSDCMD_OK;
}

static int osdcmd_password(osdcmdptr_t parm)
{
    if (parm->numparms < 1)
    {
        Bmemset(g_netPassword, 0, sizeof(g_netPassword));
        return OSDCMD_OK;
    }
    Bstrncpy(g_netPassword, (parm->raw) + 9, sizeof(g_netPassword)-1);

    return OSDCMD_OK;
}

static int osdcmd_listplayers(osdcmdptr_t parm)
{
    ENetPeer *currentPeer;
    char ipaddr[32];

    if (parm && parm->numparms != 0)
        return OSDCMD_SHOWHELP;

    if (!g_netServer)
    {
        initprintf("You are not the server.\n");
        return OSDCMD_OK;
    }

    initprintf("Connected clients:\n");

    for (currentPeer = g_netServer -> peers;
            currentPeer < & g_netServer -> peers [g_netServer -> peerCount];
            ++ currentPeer)
    {
        if (currentPeer -> state != ENET_PEER_STATE_CONNECTED)
            continue;

        enet_address_get_host_ip(&currentPeer->address, ipaddr, sizeof(ipaddr));
        initprintf("%x %s %s\n", currentPeer->address.host, ipaddr,
                   g_player[(intptr_t)currentPeer->data].user_name);
    }

    return OSDCMD_OK;
}

#if 0
static int osdcmd_kick(osdcmdptr_t parm)
{
    ENetPeer *currentPeer;
    uint32_t hexaddr;

    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    if (!g_netServer)
    {
        initprintf("You are not the server.\n");
        return OSDCMD_OK;
    }

    for (currentPeer = g_netServer -> peers;
            currentPeer < & g_netServer -> peers [g_netServer -> peerCount];
            ++ currentPeer)
    {
        if (currentPeer -> state != ENET_PEER_STATE_CONNECTED)
            continue;

        sscanf(parm->parms[0],"%" SCNx32 "", &hexaddr);

        if (currentPeer->address.host == hexaddr)
        {
            initprintf("Kicking %x (%s)\n", currentPeer->address.host,
                       g_player[(intptr_t)currentPeer->data].user_name);
            enet_peer_disconnect(currentPeer, DISC_KICKED);
            return OSDCMD_OK;
        }
    }

    initprintf("Player %s not found!\n", parm->parms[0]);
    osdcmd_listplayers(NULL);

    return OSDCMD_OK;
}

static int osdcmd_kickban(osdcmdptr_t parm)
{
    ENetPeer *currentPeer;
    uint32_t hexaddr;

    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    if (!g_netServer)
    {
        initprintf("You are not the server.\n");
        return OSDCMD_OK;
    }

    for (currentPeer = g_netServer -> peers;
            currentPeer < & g_netServer -> peers [g_netServer -> peerCount];
            ++ currentPeer)
    {
        if (currentPeer -> state != ENET_PEER_STATE_CONNECTED)
            continue;

        sscanf(parm->parms[0],"%" SCNx32 "", &hexaddr);

        // TODO: implement banning logic

        if (currentPeer->address.host == hexaddr)
        {
            char ipaddr[32];

            enet_address_get_host_ip(&currentPeer->address, ipaddr, sizeof(ipaddr));
            initprintf("Host %s is now banned.\n", ipaddr);
            initprintf("Kicking %x (%s)\n", currentPeer->address.host,
                       g_player[(intptr_t)currentPeer->data].user_name);
            enet_peer_disconnect(currentPeer, DISC_BANNED);
            return OSDCMD_OK;
        }
    }

    initprintf("Player %s not found!\n", parm->parms[0]);
    osdcmd_listplayers(NULL);

    return OSDCMD_OK;
}
#endif
#endif

static int osdcmd_printtimes(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    char buf[32];
    int32_t maxlen = 0;
    int32_t haveac=0;

    for (int i=0; i<MAXTILES; i++)
        if (g_actorCalls[i])
        {
            if (!haveac)
            {
                haveac = 1;
                OSD_Printf("\nactor times: tile, total calls, total time [ms], {min,mean,max} time/call [us]\n");
            }

            buf[0] = 0;

            for (int ii=0; ii<g_labelCnt; ii++)
            {
                if (labelcode[ii] == i && labeltype[ii] & LABEL_ACTOR)
                {
                    Bstrcpy(buf, label+(ii<<6));
                    break;
                }
            }

            if (!buf[0]) Bsprintf(buf, "%d", i);

            OSD_Printf("%17s, %8d, %9.3f, %9.3f, %9.3f, %9.3f,\n",
                buf, g_actorCalls[i], g_actorTotalMs[i],
                1000*g_actorMinMs[i],
                1000*g_actorTotalMs[i]/g_actorCalls[i],
                1000*g_actorMaxMs[i]);
        }

    return OSDCMD_OK;
}


int32_t registerosdcommands(void)
{

    if (!VOLUMEONE)
    {
        OSD_RegisterFunction("map","map <mapfile>: loads the given user map", osdcmd_map);
        OSD_RegisterFunction("demo","demo <demofile or demonum>: starts the given demo", osdcmd_demo);
    }

    OSD_RegisterFunction("crosshaircolor","crosshaircolor: changes the crosshair color", osdcmd_crosshaircolor);

#if !defined NETCODE_DISABLE
    OSD_RegisterFunction("connect","connect: connects to a multiplayer game", osdcmd_connect);
    OSD_RegisterFunction("disconnect","disconnect: disconnects from the local multiplayer game", osdcmd_disconnect);
#endif

    OSD_RegisterFunction("give","give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item", osdcmd_give);
    OSD_RegisterFunction("god","god: toggles god mode", osdcmd_god);
    OSD_RegisterFunction("activatecheat","activatecheat <id>: activates a cheat code", osdcmd_activatecheat);

#ifdef DEBUGGINGAIDS
    OSD_RegisterFunction("inittimer","debug", osdcmd_inittimer);
#endif
#if !defined NETCODE_DISABLE
#if 0
    OSD_RegisterFunction("kick","kick <id>: kicks a multiplayer client.  See listplayers.", osdcmd_kick);
    OSD_RegisterFunction("kickban","kickban <id>: kicks a multiplayer client and prevents them from reconnecting.  See listplayers.", osdcmd_kickban);
#endif

    OSD_RegisterFunction("listplayers","listplayers: lists currently connected multiplayer clients", osdcmd_listplayers);
#endif
    OSD_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);

#if !defined NETCODE_DISABLE
    OSD_RegisterFunction("password","password: sets multiplayer game password", osdcmd_password);
#endif

    OSD_RegisterFunction("printtimes", "printtimes: prints VM timing statistics", osdcmd_printtimes);

    OSD_RegisterFunction("restartmap", "restartmap: restarts the current map", osdcmd_restartmap);
    OSD_RegisterFunction("restartsound","restartsound: reinitializes the sound system",osdcmd_restartsound);

    OSD_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",osdcmd_spawn);

    OSD_RegisterFunction("vidmode","vidmode <xdim> <ydim> <bpp> <fullscreen>: change the video mode",osdcmd_vidmode);

    return 0;
}

END_RR_NS
