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
#include "osdcmds.h"
#include "menus.h"
#include "osdfuncs.h"
#include "demo.h"  // g_firstDemoFile[]
#include "cheats.h"
#include "sbar.h"

#ifdef LUNATIC
# include "lunatic_game.h"
#endif

#ifdef EDUKE32_TOUCH_DEVICES
#include "in_android.h"
#endif

struct osdcmd_cheatsinfo osdcmd_cheatsinfo_stat;
float r_ambientlight = 1.0, r_ambientlightrecip = 1.0;

uint32_t cl_cheatmask;

static inline int32_t osdcmd_quit(osdfuncparm_t const * const UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    OSD_ShowDisplay(0);
    G_GameQuit();
    return OSDCMD_OK;
}

static int32_t osdcmd_changelevel(osdfuncparm_t const * const parm)
{
    int32_t volume=0,level;
    char *p;

    if (!VOLUMEONE)
    {
        if (parm->numparms != 2) return OSDCMD_SHOWHELP;

        volume = strtol(parm->parms[0], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
        level = strtol(parm->parms[1], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
    }
    else
    {
        if (parm->numparms != 1) return OSDCMD_SHOWHELP;

        level = strtol(parm->parms[0], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
    }

    if (volume < 0) return OSDCMD_SHOWHELP;
    if (level < 0) return OSDCMD_SHOWHELP;

    if (!VOLUMEONE)
    {
        if (volume > g_volumeCnt)
        {
            OSD_Printf("changelevel: invalid volume number (range 1-%d)\n",g_volumeCnt);
            return OSDCMD_OK;
        }
    }

    if (level > MAXLEVELS || g_mapInfo[volume *MAXLEVELS+level].filename == NULL)
    {
        OSD_Printf("changelevel: invalid level number\n");
        return OSDCMD_SHOWHELP;
    }

    if (numplayers > 1)
    {
        /*
        if (g_netServer)
            Net_NewGame(volume,level);
        else if (voting == -1)
        {
            ud.m_volume_number = volume;
            ud.m_level_number = level;

            if (g_player[myconnectindex].ps->i)
            {
                int32_t i;

                for (i=0; i<MAXPLAYERS; i++)
                {
                    g_player[i].vote = 0;
                    g_player[i].gotvote = 0;
                }

                g_player[myconnectindex].vote = g_player[myconnectindex].gotvote = 1;

                voting = myconnectindex;

                tempbuf[0] = PACKET_MAP_VOTE_INITIATE;
                tempbuf[1] = myconnectindex;
                tempbuf[2] = ud.m_volume_number;
                tempbuf[3] = ud.m_level_number;

                enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(tempbuf, 4, ENET_PACKET_FLAG_RELIABLE));
            }
            if ((g_gametypeFlags[ud.m_coop] & GAMETYPE_PLAYERSFRIENDLY) && !(g_gametypeFlags[ud.m_coop] & GAMETYPE_TDM))
                ud.m_noexits = 0;

            M_OpenMenu(myconnectindex);
            Menu_Change(MENU_NETWAITVOTES);
        }
        */
        return OSDCMD_OK;
    }
    if (g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        // in-game behave like a cheat
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_SCOTTY;
        osdcmd_cheatsinfo_stat.volume   = volume;
        osdcmd_cheatsinfo_stat.level    = level;
    }
    else
    {
        // out-of-game behave like a menu command
        osdcmd_cheatsinfo_stat.cheatnum = -1;

        ud.m_volume_number     = volume;
        ud.m_level_number      = level;

        ud.m_monsters_off      = 0;
        ud.monsters_off        = 0;

        ud.m_respawn_items     = 0;
        ud.m_respawn_inventory = 0;

        ud.multimode           = 1;

        G_NewGame_EnterLevel();
    }

    return OSDCMD_OK;
}

static int32_t osdcmd_map(osdfuncparm_t const * const parm)
{
    int32_t i;
    char filename[BMAX_PATH];

    const int32_t wildcardp = parm->numparms==1 &&
        (Bstrchr(parm->parms[0], '*') != NULL);

    if (parm->numparms != 1 || wildcardp)
    {
        CACHE1D_FIND_REC *r;
        fnlist_t fnlist = FNLIST_INITIALIZER;
        int32_t maxwidth = 0;

        if (wildcardp)
            maybe_append_ext(filename, sizeof(filename), parm->parms[0], ".map");
        else
            Bstrcpy(filename, "*.MAP");

        fnlist_getnames(&fnlist, "/", filename, -1, 0);

        for (r=fnlist.findfiles; r; r=r->next)
            maxwidth = max((unsigned)maxwidth, Bstrlen(r->name));

        if (maxwidth > 0)
        {
            int32_t x = 0, count = 0;
            maxwidth += 3;
            OSD_Printf(OSDTEXT_RED "Map listing:\n");
            for (r=fnlist.findfiles; r; r=r->next)
            {
                OSD_Printf("%-*s",maxwidth,r->name);
                x += maxwidth;
                count++;
                if (x > OSD_GetCols() - maxwidth)
                {
                    x = 0;
                    OSD_Printf("\n");
                }
            }
            if (x) OSD_Printf("\n");
            OSD_Printf(OSDTEXT_RED "Found %d maps\n", fnlist.numfiles);
        }

        fnlist_clearnames(&fnlist);

        return OSDCMD_SHOWHELP;
    }

    maybe_append_ext(filename, sizeof(filename), parm->parms[0], ".map");

    if ((i = kopen4loadfrommod(filename,0)) < 0)
    {
        OSD_Printf(OSD_ERROR "map: file \"%s\" not found.\n", filename);
        return OSDCMD_OK;
    }
    kclose(i);

    boardfilename[0] = '/';
    boardfilename[1] = 0;
    strcat(boardfilename, filename);

    if (numplayers > 1)
    {
        /*
        if (g_netServer)
        {
            Net_SendUserMapName();
            ud.m_volume_number = 0;
            ud.m_level_number = 7;
            Net_NewGame(ud.m_volume_number, ud.m_level_number);
        }
        else if (voting == -1)
        {
            Net_SendUserMapName();

            ud.m_volume_number = 0;
            ud.m_level_number = 7;

            if (g_player[myconnectindex].ps->i)
            {
                int32_t i;

                for (i=0; i<MAXPLAYERS; i++)
                {
                    g_player[i].vote = 0;
                    g_player[i].gotvote = 0;
                }

                g_player[myconnectindex].vote = g_player[myconnectindex].gotvote = 1;
                voting = myconnectindex;

                tempbuf[0] = PACKET_MAP_VOTE_INITIATE;
                tempbuf[1] = myconnectindex;
                tempbuf[2] = ud.m_volume_number;
                tempbuf[3] = ud.m_level_number;

                enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(tempbuf, 4, ENET_PACKET_FLAG_RELIABLE));
            }
            if ((g_gametypeFlags[ud.m_coop] & GAMETYPE_PLAYERSFRIENDLY) && !(g_gametypeFlags[ud.m_coop] & GAMETYPE_TDM))
                ud.m_noexits = 0;

            M_OpenMenu(myconnectindex);
            Menu_Change(MENU_NETWAITVOTES);
        }
        */
        return OSDCMD_OK;
    }

    osdcmd_cheatsinfo_stat.cheatnum = -1;
    ud.m_volume_number = 0;
    ud.m_level_number = 7;

    ud.m_monsters_off = ud.monsters_off = 0;

    ud.m_respawn_items = 0;
    ud.m_respawn_inventory = 0;

    ud.multimode = 1;

    G_NewGame_EnterLevel();

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
static int32_t osdcmd_demo(osdfuncparm_t const * const parm)
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

static int32_t osdcmd_activatecheat(osdfuncparm_t const * const parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
        osdcmd_cheatsinfo_stat.cheatnum = Batoi(parm->parms[0]);
    else
        OSD_Printf("activatecheat: Not in a single-player game.\n");

    return OSDCMD_OK;
}

static int32_t osdcmd_god(osdfuncparm_t const * const UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_CORNHOLIO;
    else
        OSD_Printf("god: Not in a single-player game.\n");

    return OSDCMD_OK;
}

static int32_t osdcmd_noclip(osdfuncparm_t const * const UNUSED(parm))
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

static int32_t osdcmd_restartsound(osdfuncparm_t const * const UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    S_SoundShutdown();
    S_MusicShutdown();

    S_MusicStartup();
    S_SoundStartup();

    FX_StopAllSounds();
    S_ClearSoundLocks();

    if (ud.config.MusicToggle)
        S_RestartMusic();

    return OSDCMD_OK;
}

static int32_t osdcmd_music(osdfuncparm_t const * const parm)
{
    if (parm->numparms == 1)
    {
        int32_t sel = G_GetMusicIdx(parm->parms[0]);

        if (sel == -1)
            return OSDCMD_SHOWHELP;

        if (sel == -2)
        {
            OSD_Printf("%s is not a valid episode/level number pair\n", parm->parms[0]);
            return OSDCMD_OK;
        }

        if (g_mapInfo[sel].musicfn != NULL)
        {
            g_musicIndex = sel;
            G_StartMusic();
        }
        else
        {
            OSD_Printf("No music defined for %s\n", parm->parms[0]);
        }

        return OSDCMD_OK;
    }

    return OSDCMD_SHOWHELP;
}

int32_t osdcmd_restartvid(osdfuncparm_t const * const UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    resetvideomode();
    if (setgamemode(ud.config.ScreenMode,ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP))
        G_GameExit("restartvid: Reset failed...\n");
    onvideomodechange(ud.config.ScreenBPP>8);
    G_UpdateScreenArea();

    return OSDCMD_OK;
}

int32_t osdcmd_restartmap(osdfuncparm_t const * const UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    if (g_player[myconnectindex].ps->gm & MODE_GAME && ud.multimode == 1)
        g_player[myconnectindex].ps->gm = MODE_RESTART;

    return OSDCMD_OK;
}

static int32_t osdcmd_vidmode(osdfuncparm_t const * const parm)
{
    int32_t newbpp = ud.config.ScreenBPP, newwidth = ud.config.ScreenWidth,
            newheight = ud.config.ScreenHeight, newfs = ud.config.ScreenMode;
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

    if (setgamemode(newfs,newwidth,newheight,newbpp))
    {
        initprintf("vidmode: Mode change failed!\n");
        if (setgamemode(ud.config.ScreenMode, ud.config.ScreenWidth, ud.config.ScreenHeight, ud.config.ScreenBPP))
            G_GameExit("vidmode: Reset failed!\n");
    }
    ud.config.ScreenBPP = newbpp;
    ud.config.ScreenWidth = newwidth;
    ud.config.ScreenHeight = newheight;
    ud.config.ScreenMode = newfs;
    onvideomodechange(ud.config.ScreenBPP>8);
    G_UpdateScreenArea();
    return OSDCMD_OK;
}

#ifdef LUNATIC
// Returns: INT32_MIN if no such CON label, its value else.
LUNATIC_CB int32_t (*El_GetLabelValue)(const char *label);
#endif

static int32_t osdcmd_spawn(osdfuncparm_t const * const parm)
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
#ifdef LUNATIC
            i = g_labelCnt;
            picnum = El_GetLabelValue(parm->parms[0]);
            if (picnum != INT32_MIN)
                i = !i;
#else
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
#endif
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

#if !defined LUNATIC
static int32_t osdcmd_setvar(osdfuncparm_t const * const parm)
{
    int32_t i, varval;
    char varname[256];

    if (parm->numparms != 2) return OSDCMD_SHOWHELP;

    if (numplayers > 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    strcpy(varname,parm->parms[1]);
    varval = Batol(varname);
    i = hash_find(&h_gamevars,varname);
    if (i >= 0)
        varval=Gv_GetVar(i, g_player[screenpeek].ps->i, screenpeek);

    strcpy(varname,parm->parms[0]);
    i = hash_find(&h_gamevars,varname);
    if (i >= 0)
        Gv_SetVar(i, varval, g_player[screenpeek].ps->i, screenpeek);
    return OSDCMD_OK;
}

static int32_t osdcmd_addlogvar(osdfuncparm_t const * const parm)
{
    int32_t i;
    char varname[256];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    if (numplayers > 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    strcpy(varname,parm->parms[0]);
    i = hash_find(&h_gamevars,varname);
    if (i >= 0)
        OSD_Printf("%s = %d\n", varname, Gv_GetVar(i, g_player[screenpeek].ps->i, screenpeek));
    return OSDCMD_OK;
}

static int32_t osdcmd_setactorvar(osdfuncparm_t const * const parm)
{
    int32_t i, varval, ID;
    char varname[256];

    if (parm->numparms != 3) return OSDCMD_SHOWHELP;

    if (numplayers > 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    ID=Batol(parm->parms[0]);
    if (ID>=MAXSPRITES)
    {
        OSD_Printf("Invalid sprite ID\n");
        return OSDCMD_OK;
    }

    varval = Batol(parm->parms[2]);
    strcpy(varname,parm->parms[2]);
    varval = Batol(varname);
    i = hash_find(&h_gamevars,varname);
    if (i >= 0)
        varval=Gv_GetVar(i, g_player[screenpeek].ps->i, screenpeek);

    strcpy(varname,parm->parms[1]);
    i = hash_find(&h_gamevars,varname);
    if (i >= 0)
        Gv_SetVar(i, varval, ID, -1);
    return OSDCMD_OK;
}
#else
static int32_t osdcmd_lua(osdfuncparm_t const * const parm)
{
    // Should be used like
    // lua "lua code..."
    // (the quotes making the whole string passed as one argument)

    int32_t ret;

    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    if (!L_IsInitialized(&g_ElState))
    {
        OSD_Printf("Lua state is not initialized.\n");
        return OSDCMD_OK;
    }

    // TODO: "=<expr>" as shorthand for "print(<expr>)", like in the
    //  stand-alone Lua interpreter?
    // TODO: reserve some table to explicitly store stuff on the top level, for
    //  debugging convenience?

    // For the 'lua' OSD command, don't make errors appear on-screen:
    el_addNewErrors = 0;
    ret = L_RunString(&g_ElState, parm->parms[0], -1, "console");
    el_addNewErrors = 1;

    if (ret != 0)
        OSD_Printf("Error running the Lua code (error code %d)\n", ret);

    return OSDCMD_OK;
}
#endif

static int32_t osdcmd_addpath(osdfuncparm_t const * const parm)
{
    char pathname[BMAX_PATH];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    strcpy(pathname,parm->parms[0]);
    addsearchpath(pathname);
    return OSDCMD_OK;
}

static int32_t osdcmd_initgroupfile(osdfuncparm_t const * const parm)
{
    char file[BMAX_PATH];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    strcpy(file,parm->parms[0]);
    initgroupfile(file);
    return OSDCMD_OK;
}

static int32_t osdcmd_cmenu(osdfuncparm_t const * const parm)
{
    if (parm->numparms != 1) return OSDCMD_SHOWHELP;
    if (numplayers > 1)
    {
        OSD_Printf("cmenu: disallowed in multiplayer\n");
        return OSDCMD_OK;
    }
    else
    {
        Menu_Change(Batol(parm->parms[0]));
    }

    return OSDCMD_OK;
}

extern void G_SetCrosshairColor(int32_t r, int32_t g, int32_t b);
extern palette_t CrosshairColors;

static int32_t osdcmd_crosshaircolor(osdfuncparm_t const * const parm)
{
    int32_t r, g, b;

    if (parm->numparms != 3)
    {
        OSD_Printf("crosshaircolor: r:%d g:%d b:%d\n",CrosshairColors.r,CrosshairColors.g,CrosshairColors.b);
        return OSDCMD_SHOWHELP;
    }
    r = Batol(parm->parms[0]);
    g = Batol(parm->parms[1]);
    b = Batol(parm->parms[2]);
    G_SetCrosshairColor(r,g,b);

    if (!OSD_ParsingScript())
        OSD_Printf("%s\n", parm->raw);

    return OSDCMD_OK;
}

static int32_t osdcmd_give(osdfuncparm_t const * const parm)
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
    if (getrendermode() == REND_POLYMER)
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

    setbrightness(ud.brightness>>2, palid, 0);
    g_restorePalette = -1;
    g_crosshairSum = -1;
}

static int32_t osdcmd_name(osdfuncparm_t const * const parm)
{
    char namebuf[32];

    if (parm->numparms != 1)
    {
        OSD_Printf("\"name\" is \"%s\"\n",szPlayerName);
        return OSDCMD_SHOWHELP;
    }

    Bstrcpy(tempbuf,parm->parms[0]);

    while (Bstrlen(OSD_StripColors(namebuf,tempbuf)) > 10)
        tempbuf[Bstrlen(tempbuf)-1] = '\0';

    Bstrncpy(szPlayerName,tempbuf,sizeof(szPlayerName)-1);
    szPlayerName[sizeof(szPlayerName)-1] = '\0';

    OSD_Printf("name %s\n",szPlayerName);

    Net_SendClientInfo();

    return OSDCMD_OK;
}


static int32_t osdcmd_button(osdfuncparm_t const * const parm)
{
    char const *p = parm->name+9;  // skip "gamefunc_"
//    if (g_player[myconnectindex].ps->gm == MODE_GAME) // only trigger these if in game
    CONTROL_OSDInput[CONFIG_FunctionNameToNum(p)] = 1; // FIXME
    return OSDCMD_OK;
}

const keydef_t ConsoleKeys[]=
{
    { "Escape", 0x1 },
    { "1", 0x2 },
    { "2", 0x3 },
    { "3", 0x4 },
    { "4", 0x5 },
    { "5", 0x6 },
    { "6", 0x7 },
    { "7", 0x8 },
    { "8", 0x9 },
    { "9", 0xa },
    { "0", 0xb },
    { "-", 0xc },
    { "=", 0xd },
    { "BakSpc", 0xe },
    { "Tab", 0xf },
    { "Q", 0x10 },
    { "W", 0x11 },
    { "E", 0x12 },
    { "R", 0x13 },
    { "T", 0x14 },
    { "Y", 0x15 },
    { "U", 0x16 },
    { "I", 0x17 },
    { "O", 0x18 },
    { "P", 0x19 },
    { "[", 0x1a },
    { "]", 0x1b },
    { "Enter", 0x1c },
    { "LCtrl", 0x1d },
    { "A", 0x1e },
    { "S", 0x1f },
    { "D", 0x20 },
    { "F", 0x21 },
    { "G", 0x22 },
    { "H", 0x23 },
    { "J", 0x24 },
    { "K", 0x25 },
    { "L", 0x26 },
    { "SemiColon", 0x27 },
    { "'", 0x28 },
    { "Tilde", 0x29 },
    { "LShift", 0x2a },
    { "Backslash", 0x2b },
    { "Z", 0x2c },
    { "X", 0x2d },
    { "C", 0x2e },
    { "V", 0x2f },
    { "B", 0x30 },
    { "N", 0x31 },
    { "M", 0x32 },
    { ",", 0x33 },
    { ".", 0x34 },
    { "/", 0x35 },
    { "RShift", 0x36 },
    { "Kpad*", 0x37 },
    { "LAlt", 0x38 },
    { "Space", 0x39 },
    { "CapLck", 0x3a },
    { "F1", 0x3b },
    { "F2", 0x3c },
    { "F3", 0x3d },
    { "F4", 0x3e },
    { "F5", 0x3f },
    { "F6", 0x40 },
    { "F7", 0x41 },
    { "F8", 0x42 },
    { "F9", 0x43 },
    { "F10", 0x44 },
    { "NumLck", 0x45 },
    { "ScrLck", 0x46 },
    { "Kpad7", 0x47 },
    { "Kpad8", 0x48 },
    { "Kpad9", 0x49 },
    { "Kpad-", 0x4a },
    { "Kpad4", 0x4b },
    { "Kpad5", 0x4c },
    { "Kpad6", 0x4d },
    { "Kpad+", 0x4e },
    { "Kpad1", 0x4f },
    { "Kpad2", 0x50 },
    { "Kpad3", 0x51 },
    { "Kpad0", 0x52 },
    { "Kpad.", 0x53 },
    { "F11", 0x57 },
    { "F12", 0x58 },
    { "KpdEnt", 0x9c },
    { "RCtrl", 0x9d },
    { "Kpad/", 0xb5 },
    { "RAlt", 0xb8 },
    { "PrtScn", 0xb7 },
    { "Pause", 0xc5 },
    { "Home", 0xc7 },
    { "Up", 0xc8 },
    { "PgUp", 0xc9 },
    { "Left", 0xcb },
    { "Right", 0xcd },
    { "End", 0xcf },
    { "Down", 0xd0 },
    { "PgDn", 0xd1 },
    { "Insert", 0xd2 },
    { "Delete", 0xd3 },

    {0,0}
};

const char *const ConsoleButtons[] =
{
    "mouse1", "mouse2", "mouse3", "mouse4", "mwheelup",
    "mwheeldn", "mouse5", "mouse6", "mouse7", "mouse8"
};

static int32_t osdcmd_bind(osdfuncparm_t const * const parm)
{
    int32_t i, j, repeat;

    if (parm->numparms==1 && !Bstrcasecmp(parm->parms[0],"showkeys"))
    {
        for (i=0; ConsoleKeys[i].name; i++)
            OSD_Printf("%s\n",ConsoleKeys[i].name);
        for (i=0; i<MAXMOUSEBUTTONS; i++)
            OSD_Printf("%s\n",ConsoleButtons[i]);
        return OSDCMD_OK;
    }

    if (parm->numparms==0)
    {
        int32_t j=0;

        OSD_Printf("Current key bindings:\n");

        for (i=0; i<MAXBOUNDKEYS+MAXMOUSEBUTTONS; i++)
            if (CONTROL_KeyIsBound(i))
            {
                j++;
                OSD_Printf("%-9s %s\"%s\"\n", CONTROL_KeyBinds[i].key, CONTROL_KeyBinds[i].repeat?"":"norepeat ",
                           CONTROL_KeyBinds[i].cmdstr);
            }

        if (j == 0)
            OSD_Printf("No binds found.\n");

        return OSDCMD_OK;
    }

    for (i=0; ConsoleKeys[i].name; i++)
        if (!Bstrcasecmp(parm->parms[0],ConsoleKeys[i].name))
            break;

    if (!ConsoleKeys[i].name)
    {
        for (i=0; i<MAXMOUSEBUTTONS; i++)
            if (!Bstrcasecmp(parm->parms[0],ConsoleButtons[i]))
                break;
        if (i >= MAXMOUSEBUTTONS)
            return OSDCMD_SHOWHELP;

        if (parm->numparms < 2)
        {
            if (CONTROL_KeyBinds[MAXBOUNDKEYS + i].cmdstr && CONTROL_KeyBinds[MAXBOUNDKEYS + i ].key)
                OSD_Printf("%-9s %s\"%s\"\n", ConsoleButtons[i], CONTROL_KeyBinds[MAXBOUNDKEYS + i].repeat?"":"norepeat ",
                CONTROL_KeyBinds[MAXBOUNDKEYS + i].cmdstr);
            else OSD_Printf("%s is unbound\n", ConsoleButtons[i]);
            return OSDCMD_OK;
        }

        j = 1;

        repeat = 1;
        if (parm->numparms >= 2 && !Bstrcasecmp(parm->parms[j],"norepeat"))
        {
            repeat = 0;
            j++;
        }

        Bstrcpy(tempbuf,parm->parms[j++]);
        for (; j<parm->numparms; j++)
        {
            Bstrcat(tempbuf," ");
            Bstrcat(tempbuf,parm->parms[j++]);
        }

        CONTROL_BindMouse(i, tempbuf, repeat, ConsoleButtons[i]);

        if (!OSD_ParsingScript())
            OSD_Printf("%s\n",parm->raw);
        return OSDCMD_OK;
    }

    if (parm->numparms < 2)
    {
        if (CONTROL_KeyIsBound(ConsoleKeys[i].id))
            OSD_Printf("%-9s %s\"%s\"\n", ConsoleKeys[i].name, CONTROL_KeyBinds[ConsoleKeys[i].id].repeat?"":"norepeat ",
                       CONTROL_KeyBinds[ConsoleKeys[i].id].cmdstr);
        else OSD_Printf("%s is unbound\n", ConsoleKeys[i].name);

        return OSDCMD_OK;
    }

    j = 1;

    repeat = 1;
    if (parm->numparms >= 2 && !Bstrcasecmp(parm->parms[j],"norepeat"))
    {
        repeat = 0;
        j++;
    }

    Bstrcpy(tempbuf,parm->parms[j++]);
    for (; j<parm->numparms; j++)
    {
        Bstrcat(tempbuf," ");
        Bstrcat(tempbuf,parm->parms[j++]);
    }

    CONTROL_BindKey(ConsoleKeys[i].id, tempbuf, repeat, ConsoleKeys[i].name);

    {
        char *cp = tempbuf;

        // Populate the keyboard config menu based on the bind.
        // Take care of processing one-to-many bindings properly, too.
        while ((cp = Bstrstr(cp, "gamefunc_")))
        {
            char *semi;

            cp += 9;  // skip the "gamefunc_"

            semi = Bstrchr(cp, ';');
            if (semi)
                *semi = 0;

            j = CONFIG_FunctionNameToNum(cp);

            if (semi)
                cp = semi+1;

            if (j != -1)
            {
                ud.config.KeyboardKeys[j][1] = ud.config.KeyboardKeys[j][0];
                ud.config.KeyboardKeys[j][0] = ConsoleKeys[i].id;
//            CONTROL_MapKey(j, ConsoleKeys[i].id, ud.config.KeyboardKeys[j][0]);

                if (j == gamefunc_Show_Console)
                    OSD_CaptureKey(ConsoleKeys[i].id);
            }
        }
    }

    if (!OSD_ParsingScript())
        OSD_Printf("%s\n",parm->raw);

    return OSDCMD_OK;
}

static int32_t osdcmd_unbindall(osdfuncparm_t const * const UNUSED(parm))
{
    int32_t i;

    UNREFERENCED_CONST_PARAMETER(parm);

    for (i=0; i<MAXBOUNDKEYS; i++)
        CONTROL_FreeKeyBind(i);

    for (i=0; i<MAXMOUSEBUTTONS; i++)
        CONTROL_FreeMouseBind(i);

    for (i=0; i<NUMGAMEFUNCTIONS; i++)
    {
        ud.config.KeyboardKeys[i][0] = ud.config.KeyboardKeys[i][1] = 0xff;
//        CONTROL_MapKey(i, ud.config.KeyboardKeys[i][0], ud.config.KeyboardKeys[i][1]);
    }

    if (!OSD_ParsingScript())
        OSD_Printf("unbound all controls\n");

    return OSDCMD_OK;
}

static int32_t osdcmd_unbind(osdfuncparm_t const * const parm)
{
    int32_t i;

    if (parm->numparms < 1) return OSDCMD_SHOWHELP;

    for (i=0; ConsoleKeys[i].name; i++)
        if (!Bstrcasecmp(parm->parms[0],ConsoleKeys[i].name))
            break;

    if (!ConsoleKeys[i].name)
    {
        for (i=0; i<MAXMOUSEBUTTONS; i++)
            if (!Bstrcasecmp(parm->parms[0],ConsoleButtons[i]))
                break;

        if (i >= MAXMOUSEBUTTONS)
            return OSDCMD_SHOWHELP;

        CONTROL_FreeMouseBind(i);

        OSD_Printf("unbound %s\n",ConsoleButtons[i]);

        return OSDCMD_OK;
    }

    CONTROL_FreeKeyBind(ConsoleKeys[i].id);

    OSD_Printf("unbound key %s\n",ConsoleKeys[i].name);

    return OSDCMD_OK;
}

static int32_t osdcmd_quicksave(osdfuncparm_t const * const UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
        OSD_Printf("quicksave: not in a game.\n");
    else g_doQuickSave = 1;
    return OSDCMD_OK;
}

static int32_t osdcmd_quickload(osdfuncparm_t const * const UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
        OSD_Printf("quickload: not in a game.\n");
    else g_doQuickSave = 2;
    return OSDCMD_OK;
}

static int32_t osdcmd_screenshot(osdfuncparm_t const * const parm)
{
//    KB_ClearKeysDown();
#ifndef EDUKE32_STANDALONE
    static const char *fn = "duke0000.png";
#else
    static const char *fn = "capt0000.png";
#endif

    if (parm->numparms == 1 && !Bstrcasecmp(parm->parms[0], "tga"))
        screencapture_tga(fn, 0);
    else screencapture(fn, 0);

    return OSDCMD_OK;
}

#if 0
static int32_t osdcmd_savestate(osdfuncparm_t const * const UNUSED(parm))
{
    UNREFERENCED_PARAMETER(parm);
    G_SaveMapState();
    return OSDCMD_OK;
}

static int32_t osdcmd_restorestate(osdfuncparm_t const * const UNUSED(parm))
{
    UNREFERENCED_PARAMETER(parm);
    G_RestoreMapState();
    return OSDCMD_OK;
}
#endif

#ifdef DEBUGGINGAIDS
static int32_t osdcmd_inittimer(osdfuncparm_t const * const parm)
{
    if (parm->numparms != 1)
    {
        OSD_Printf("%dHz timer\n",g_timerTicsPerSecond);
        return OSDCMD_SHOWHELP;
    }

    G_InitTimer(Batol(parm->parms[0]));

    OSD_Printf("%s\n",parm->raw);
    return OSDCMD_OK;
}
#endif

static int32_t osdcmd_disconnect(osdfuncparm_t const * const UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    g_netDisconnect = 1;
    return OSDCMD_OK;
}

static int32_t osdcmd_connect(osdfuncparm_t const * const parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    Net_Connect(parm->parms[0]);
    G_BackToMenu();
    return OSDCMD_OK;
}

static int32_t osdcmd_password(osdfuncparm_t const * const parm)
{
    if (parm->numparms < 1)
    {
        Bmemset(g_netPassword, 0, sizeof(g_netPassword));
        return OSDCMD_OK;
    }
    Bstrncpy(g_netPassword, (parm->raw) + 9, sizeof(g_netPassword)-1);

    return OSDCMD_OK;
}

#if !defined NETCODE_DISABLE
static int32_t osdcmd_listplayers(osdfuncparm_t const * const parm)
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

static int32_t osdcmd_kick(osdfuncparm_t const * const parm)
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

static int32_t osdcmd_kickban(osdfuncparm_t const * const parm)
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

static int32_t osdcmd_cvar_set_game(osdfuncparm_t const * const parm)
{
    int32_t r = osdcmd_cvar_set(parm);

    if (r != OSDCMD_OK) return r;

    if (!Bstrcasecmp(parm->name, "r_size"))
    {
        ud.statusbarmode = (ud.screen_size < 8);
        G_UpdateScreenArea();
    }
    else if (!Bstrcasecmp(parm->name, "r_maxfps"))
    {
        if (r_maxfps != 0) r_maxfps = clamp(r_maxfps, 30, 1000);
        g_frameDelay = r_maxfps ? (getu64tickspersec()/r_maxfps) : 0;
    }
    else if (!Bstrcasecmp(parm->name, "r_ambientlight"))
    {
        if (r_ambientlight == 0)
            r_ambientlightrecip = 256.f;
        else r_ambientlightrecip = 1.f/r_ambientlight;
    }
    else if (!Bstrcasecmp(parm->name, "in_mouse"))
    {
        CONTROL_MouseEnabled = (ud.config.UseMouse && CONTROL_MousePresent);
    }
    else if (!Bstrcasecmp(parm->name, "in_joystick"))
    {
        CONTROL_JoystickEnabled = (ud.config.UseJoystick && CONTROL_JoyPresent);
    }
    else if (!Bstrcasecmp(parm->name, "vid_gamma"))
    {
        ud.brightness = GAMMA_CALC;
        ud.brightness <<= 2;
        setbrightness(ud.brightness>>2,g_player[myconnectindex].ps->palette,0);
    }
    else if (!Bstrcasecmp(parm->name, "vid_brightness") || !Bstrcasecmp(parm->name, "vid_contrast"))
    {
        setbrightness(ud.brightness>>2,g_player[myconnectindex].ps->palette,0);
    }
    else if (!Bstrcasecmp(parm->name, "hud_scale")
             || !Bstrcasecmp(parm->name, "hud_statusbarmode")
             || !Bstrcasecmp(parm->name, "r_rotatespritenowidescreen"))
    {
        G_UpdateScreenArea();
    }
    else if (!Bstrcasecmp(parm->name, "skill"))
    {
        if (numplayers > 1)
            return r;

        ud.player_skill = ud.m_player_skill;
    }
    else if (!Bstrcasecmp(parm->name, "color"))
    {
        G_CheckPlayerColor((int32_t *)&ud.color,-1);
        g_player[0].ps->palookup = g_player[0].pcolor = ud.color;
    }
    else if (!Bstrcasecmp(parm->name, "osdscale"))
    {
        osdrscale = 1.f/osdscale;

        if (xdim && ydim)
            OSD_ResizeDisplay(xdim, ydim);
    }
    else if (!Bstrcasecmp(parm->name, "wchoice"))
    {
        if (parm->numparms == 1)
        {
            if (g_forceWeaponChoice) // rewrite ud.wchoice because osdcmd_cvar_set already changed it
            {
                int j = 0;

                while (j < 10)
                {
                    ud.wchoice[j] = g_player[myconnectindex].wchoice[j] + '0';
                    j++;
                }

                ud.wchoice[j] = 0;
            }
            else
            {
                char const *c = parm->parms[0];

                if (*c)
                {
                    int j = 0;

                    while (*c && j < 10)
                    {
                        g_player[myconnectindex].wchoice[j] = *c - '0';
                        c++;
                        j++;
                    }

                    while (j < 10)
                    {
                        if (j == 9)
                            g_player[myconnectindex].wchoice[9] = 1;
                        else
                            g_player[myconnectindex].wchoice[j] = 2;

                        j++;
                    }
                }
            }

            g_forceWeaponChoice = 0;
        }

        /*    Net_SendClientInfo();*/
    }

    return r;
}

static int32_t osdcmd_cvar_set_multi(osdfuncparm_t const * const parm)
{
    int32_t r = osdcmd_cvar_set_game(parm);

    if (r != OSDCMD_OK) return r;

    G_UpdatePlayerFromMenu();

    return r;
}

int32_t registerosdcommands(void)
{
    uint32_t i;

    static osdcvardata_t cvars_game[] =
    {
        { "crosshair", "enable/disable crosshair", (void *)&ud.crosshair, CVAR_BOOL, 0, 1 },

        { "cl_autoaim", "enable/disable weapon autoaim", (void *)&ud.config.AutoAim, CVAR_INT|CVAR_MULTI, 0, 3 },
        { "cl_automsg", "enable/disable automatically sending messages to all players", (void *)&ud.automsg, CVAR_BOOL, 0, 1 },
        { "cl_autorun", "enable/disable autorun", (void *)&ud.auto_run, CVAR_BOOL, 0, 1 },
        { "cl_autovote", "enable/disable automatic voting", (void *)&ud.autovote, CVAR_INT, 0, 2 },

        { "cl_cheatmask", "configure what cheats show in the cheats menu", (void *)&cl_cheatmask, CVAR_UINT, 0, ~0 },

        { "cl_obituaries", "enable/disable multiplayer death messages", (void *)&ud.obituaries, CVAR_BOOL, 0, 1 },
        { "cl_democams", "enable/disable demo playback cameras", (void *)&ud.democams, CVAR_BOOL, 0, 1 },

        { "cl_idplayers", "enable/disable name display when aiming at opponents", (void *)&ud.idplayers, CVAR_BOOL, 0, 1 },

        { "cl_runmode", "enable/disable modernized run key operation", (void *)&ud.runkey_mode, CVAR_BOOL, 0, 1 },

        { "cl_showcoords", "show your position in the game world", (void *)&ud.coords, CVAR_INT, 0,
#ifdef USE_OPENGL
          2
#else
          1
#endif
        },

        { "cl_viewbob", "enable/disable player head bobbing", (void *)&ud.viewbob, CVAR_BOOL, 0, 1 },

        { "cl_weaponsway", "enable/disable player weapon swaying", (void *)&ud.weaponsway, CVAR_BOOL, 0, 1 },
        { "cl_weaponswitch", "enable/disable auto weapon switching", (void *)&ud.weaponswitch, CVAR_INT|CVAR_MULTI, 0, 7 },

        { "color", "changes player palette", (void *)&ud.color, CVAR_INT|CVAR_MULTI, 0, MAXPALOOKUPS-1 },

        { "crosshairscale","changes the size of the crosshair", (void *)&ud.crosshairscale, CVAR_INT, 10, 100 },

        { "demorec_diffs","enable/disable diff recording in demos",(void *)&demorec_diffs_cvar, CVAR_BOOL, 0, 1 },
        { "demorec_force","enable/disable forced demo recording",(void *)&demorec_force_cvar, CVAR_BOOL|CVAR_NOSAVE, 0, 1 },
        {
            "demorec_difftics","sets game tic interval after which a diff is recorded",
            (void *)&demorec_difftics_cvar, CVAR_INT, 2, 60*REALGAMETICSPERSEC
        },
        { "demorec_diffcompress","Compression method for diffs. (0: none, 1: KSLZW)",(void *)&demorec_diffcompress_cvar, CVAR_INT, 0, 1 },
        { "demorec_synccompress","Compression method for input. (0: none, 1: KSLZW)",(void *)&demorec_synccompress_cvar, CVAR_INT, 0, 1 },
        { "demorec_seeds","enable/disable recording of random seed for later sync checking",(void *)&demorec_seeds_cvar, CVAR_BOOL, 0, 1 },
        { "demoplay_diffs","enable/disable application of diffs in demo playback",(void *)&demoplay_diffs, CVAR_BOOL, 0, 1 },
        { "demoplay_showsync","enable/disable display of sync status",(void *)&demoplay_showsync, CVAR_BOOL, 0, 1 },

        { "hud_althud", "enable/disable alternate mini-hud", (void *)&ud.althud, CVAR_INT, 0, 2 },
        { "hud_bgstretch", "enable/disable background image stretching in wide resolutions", (void *)&ud.bgstretch, CVAR_BOOL, 0, 1 },
        { "hud_messagetime", "length of time to display multiplayer chat messages", (void *)&ud.msgdisptime, CVAR_INT, 0, 3600 },
        { "hud_numbertile", "first tile in alt hud number set", (void *)&althud_numbertile, CVAR_INT, 0, MAXUSERTILES-10 },
        { "hud_numberpal", "pal for alt hud numbers", (void *)&althud_numberpal, CVAR_INT, 0, MAXPALOOKUPS-1 },
        { "hud_shadows", "enable/disable althud shadows", (void *)&althud_shadows, CVAR_BOOL, 0, 1 },
        { "hud_flashing", "enable/disable althud flashing", (void *)&althud_flashing, CVAR_BOOL, 0, 1 },
        { "hud_glowingquotes", "enable/disable \"glowing\" quote text", (void *)&hud_glowingquotes, CVAR_BOOL, 0, 1 },
        { "hud_scale","changes the hud scale", (void *)&ud.statusbarscale, CVAR_INT|CVAR_FUNCPTR, 36, 100 },
        { "hud_showmapname", "enable/disable map name display on load", (void *)&hud_showmapname, CVAR_BOOL, 0, 1 },
        { "hud_stats", "enable/disable level statistics display", (void *)&ud.levelstats, CVAR_BOOL, 0, 1 },
        { "hud_textscale", "sets multiplayer chat message size", (void *)&ud.textscale, CVAR_INT, 100, 400 },
        { "hud_weaponscale","changes the weapon scale", (void *)&ud.weaponscale, CVAR_INT, 10, 100 },
        { "hud_statusbarmode", "change overlay mode of status bar", (void *)&ud.statusbarmode, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },

#ifdef EDUKE32_TOUCH_DEVICES
        { "hud_hidestick", "hide the touch input stick", (void *)&droidinput.hideStick, CVAR_BOOL, 0, 1 },
#endif

        { "in_joystick","enables input from the joystick if it is present",(void *)&ud.config.UseJoystick, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "in_mouse","enables input from the mouse if it is present",(void *)&ud.config.UseMouse, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },

        { "in_aimmode", "0:toggle, 1:hold to aim", (void *)&ud.mouseaiming, CVAR_BOOL, 0, 1 },
        {
            "in_mousebias", "emulates the original mouse code's weighting of input towards whichever axis is moving the most at any given time",
            (void *)&ud.config.MouseBias, CVAR_INT, 0, 32
        },
        { "in_mousedeadzone", "amount of mouse movement to filter out", (void *)&ud.config.MouseDeadZone, CVAR_INT, 0, 512 },
        { "in_mouseflip", "invert vertical mouse movement", (void *)&ud.mouseflip, CVAR_BOOL, 0, 1 },
        { "in_mousemode", "toggles vertical mouse view", (void *)&g_myAimMode, CVAR_BOOL, 0, 1 },
        { "in_mousesmoothing", "enable/disable mouse input smoothing", (void *)&ud.config.SmoothInput, CVAR_BOOL, 0, 1 },

        { "mus_enabled", "enables/disables music", (void *)&ud.config.MusicToggle, CVAR_BOOL, 0, 1 },
        { "mus_volume", "controls music volume", (void *)&ud.config.MusicVolume, CVAR_INT, 0, 255 },

        { "osdhightile", "enable/disable hires art replacements for console text", (void *)&osdhightile, CVAR_BOOL, 0, 1 },
        { "osdscale", "adjust console text size", (void *)&osdscale, CVAR_FLOAT|CVAR_FUNCPTR, 1, 4 },

        { "r_camrefreshdelay", "minimum delay between security camera sprite updates, 120 = 1 second", (void *)&ud.camera_time, CVAR_INT, 1, 240 },
        { "r_drawweapon", "enable/disable weapon drawing", (void *)&ud.drawweapon, CVAR_INT, 0, 2 },
        { "r_showfps", "show the frame rate counter", (void *)&ud.showfps, CVAR_INT, 0, 2 },
        { "r_shadows", "enable/disable sprite and model shadows", (void *)&ud.shadows, CVAR_BOOL, 0, 1 },
        { "r_size", "change size of viewable area", (void *)&ud.screen_size, CVAR_INT|CVAR_FUNCPTR, 0, 64 },
        { "r_rotatespritenowidescreen", "pass bit 1024 to all CON rotatesprite calls", (void *)&g_rotatespriteNoWidescreen, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "r_pixeldoubling", "enable/disable pixel doubling in the software renderer", (void *) &ud.detail, CVAR_BOOL, 0, 1 },
        { "r_precache", "enable/disable the pre-level caching routine", (void *)&ud.config.useprecache, CVAR_BOOL, 0, 1 },

        { "r_ambientlight", "sets the global map light level",(void *)&r_ambientlight, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "r_maxfps", "limit the frame rate",(void *)&r_maxfps, CVAR_INT|CVAR_FUNCPTR, 0, 1000 },

        { "sensitivity","changes the mouse sensitivity", (void *)&CONTROL_MouseSensitivity, CVAR_FLOAT|CVAR_FUNCPTR, 0, 25 },

        { "skill","changes the game skill setting", (void *)&ud.m_player_skill, CVAR_INT|CVAR_FUNCPTR/*|CVAR_NOMULTI*/, 0, 5 },

        { "snd_ambience", "enables/disables ambient sounds", (void *)&ud.config.AmbienceToggle, CVAR_BOOL, 0, 1 },
        { "snd_enabled", "enables/disables sound effects", (void *)&ud.config.SoundToggle, CVAR_BOOL, 0, 1 },
        { "snd_mastervolume", "master volume for sound system", (void *)&ud.config.MasterVolume, CVAR_INT, 0, 255 },
        { "snd_fxvolume", "volume of sound effects", (void *)&ud.config.FXVolume, CVAR_INT, 1, 255 },
        { "snd_mixrate", "sound mixing rate", (void *)&ud.config.MixRate, CVAR_INT, 0, 48000 },
        { "snd_numchannels", "the number of sound channels", (void *)&ud.config.NumChannels, CVAR_INT, 0, 2 },
        { "snd_numvoices", "the number of concurrent sounds", (void *)&ud.config.NumVoices, CVAR_INT, 0, 256 },
        { "snd_reversestereo", "reverses the stereo channels", (void *)&ud.config.ReverseStereo, CVAR_BOOL, 0, 1 },
        { "snd_speech", "enables/disables player speech", (void *)&ud.config.VoiceToggle, CVAR_INT, 0, 5 },

        { "team","change team in multiplayer", (void *)&ud.team, CVAR_INT|CVAR_MULTI, 0, 3 },

#ifdef EDUKE32_TOUCH_DEVICES
        { "touch_sens_move_x","touch input sensitivity for moving forward/back", (void *)&droidinput.forward_sens, CVAR_FLOAT, 1, 9 },
        { "touch_sens_move_y","touch input sensitivity for strafing", (void *)&droidinput.strafe_sens, CVAR_FLOAT, 1, 9 },
        { "touch_sens_look_x", "touch input sensitivity for turning left/right", (void *) &droidinput.yaw_sens, CVAR_FLOAT, 1, 9 },
        { "touch_sens_look_y", "touch input sensitivity for looking up/down", (void *) &droidinput.pitch_sens, CVAR_FLOAT, 1, 9 },
        { "touch_invert", "invert look up/down touch input", (void *) &droidinput.invertLook, CVAR_INT, 0, 1 },
#endif

        { "vid_gamma","adjusts gamma component of gamma ramp",(void *)&vid_gamma, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "vid_contrast","adjusts contrast component of gamma ramp",(void *)&vid_contrast, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "vid_brightness","adjusts brightness component of gamma ramp",(void *)&vid_brightness, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "wchoice","sets weapon autoselection order", (void *)ud.wchoice, CVAR_STRING|CVAR_FUNCPTR, 0, MAX_WEAPONS },
    };

    osdcmd_cheatsinfo_stat.cheatnum = -1;

    for (i=0; i<ARRAY_SIZE(cvars_game); i++)
    {
        switch (cvars_game[i].flags & (CVAR_FUNCPTR|CVAR_MULTI))
        {
        case CVAR_FUNCPTR:
            OSD_RegisterCvar(&cvars_game[i], osdcmd_cvar_set_game); break;
        case CVAR_MULTI:
        case CVAR_FUNCPTR|CVAR_MULTI:
            OSD_RegisterCvar(&cvars_game[i], osdcmd_cvar_set_multi); break;
        default:
            OSD_RegisterCvar(&cvars_game[i], osdcmd_cvar_set); break;
        }
    }

    if (VOLUMEONE)
        OSD_RegisterFunction("changelevel","changelevel <level>: warps to the given level", osdcmd_changelevel);
    else
    {
        OSD_RegisterFunction("changelevel","changelevel <volume> <level>: warps to the given level", osdcmd_changelevel);
        OSD_RegisterFunction("map","map <mapfile>: loads the given user map", osdcmd_map);
        OSD_RegisterFunction("demo","demo <demofile or demonum>: starts the given demo", osdcmd_demo);
    }

    OSD_RegisterFunction("addpath","addpath <path>: adds path to game filesystem", osdcmd_addpath);
    OSD_RegisterFunction("bind","bind <key> <string>: associates a keypress with a string of console input. Type \"bind showkeys\" for a list of keys and \"listsymbols\" for a list of valid console commands.", osdcmd_bind);
    OSD_RegisterFunction("cmenu","cmenu <#>: jumps to menu", osdcmd_cmenu);
    OSD_RegisterFunction("crosshaircolor","crosshaircolor: changes the crosshair color", osdcmd_crosshaircolor);

    OSD_RegisterFunction("connect","connect: connects to a multiplayer game", osdcmd_connect);
    OSD_RegisterFunction("disconnect","disconnect: disconnects from the local multiplayer game", osdcmd_disconnect);

    for (i=0; i<NUMGAMEFUNCTIONS; i++)
    {
        char *t;
        int32_t j;

        if (gamefunctions[i][0] == '\0')
            continue;

//        if (!Bstrcmp(gamefunctions[i],"Show_Console")) continue;

        Bsprintf(tempbuf,"gamefunc_%s",gamefunctions[i]);
        t = Xstrdup(tempbuf);
        for (j=Bstrlen(t); j>=0; j--)
            t[j] = Btolower(t[j]);
        Bstrcat(tempbuf,": game button");
        OSD_RegisterFunction(t, Xstrdup(tempbuf), osdcmd_button);
    }

    OSD_RegisterFunction("give","give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item", osdcmd_give);
    OSD_RegisterFunction("god","god: toggles god mode", osdcmd_god);
    OSD_RegisterFunction("activatecheat","activatecheat <id>: activates a cheat code", osdcmd_activatecheat);

    OSD_RegisterFunction("initgroupfile","initgroupfile <path>: adds a grp file into the game filesystem", osdcmd_initgroupfile);
#ifdef DEBUGGINGAIDS
    OSD_RegisterFunction("inittimer","debug", osdcmd_inittimer);
#endif
#if !defined NETCODE_DISABLE
    OSD_RegisterFunction("kick","kick <id>: kicks a multiplayer client.  See listplayers.", osdcmd_kick);
    OSD_RegisterFunction("kickban","kickban <id>: kicks a multiplayer client and prevents them from reconnecting.  See listplayers.", osdcmd_kickban);

    OSD_RegisterFunction("listplayers","listplayers: lists currently connected multiplayer clients", osdcmd_listplayers);
#endif
    OSD_RegisterFunction("music","music E<ep>L<lev>: change music", osdcmd_music);
    OSD_RegisterFunction("name","name: change your multiplayer nickname", osdcmd_name);
    OSD_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);

    OSD_RegisterFunction("password","password: sets multiplayer game password", osdcmd_password);

    OSD_RegisterFunction("quicksave","quicksave: performs a quick save", osdcmd_quicksave);
    OSD_RegisterFunction("quickload","quickload: performs a quick load", osdcmd_quickload);
    OSD_RegisterFunction("quit","quit: exits the game immediately", osdcmd_quit);
    OSD_RegisterFunction("exit","exit: exits the game immediately", osdcmd_quit);

    OSD_RegisterFunction("restartmap", "restartmap: restarts the current map", osdcmd_restartmap);
    OSD_RegisterFunction("restartsound","restartsound: reinitializes the sound system",osdcmd_restartsound);
    OSD_RegisterFunction("restartvid","restartvid: reinitializes the video mode",osdcmd_restartvid);
#if !defined LUNATIC
    OSD_RegisterFunction("addlogvar","addlogvar <gamevar>: prints the value of a gamevar", osdcmd_addlogvar);
    OSD_RegisterFunction("setvar","setvar <gamevar> <value>: sets the value of a gamevar", osdcmd_setvar);
    OSD_RegisterFunction("setvarvar","setvarvar <gamevar1> <gamevar2>: sets the value of <gamevar1> to <gamevar2>", osdcmd_setvar);
    OSD_RegisterFunction("setactorvar","setactorvar <actor#> <gamevar> <value>: sets the value of <actor#>'s <gamevar> to <value>", osdcmd_setactorvar);
#else
    OSD_RegisterFunction("lua", "lua \"Lua code...\": runs Lunatic code", osdcmd_lua);
#endif
    OSD_RegisterFunction("screenshot","screenshot [format]: takes a screenshot.", osdcmd_screenshot);

    OSD_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",osdcmd_spawn);

    OSD_RegisterFunction("unbind","unbind <key>: unbinds a key", osdcmd_unbind);
    OSD_RegisterFunction("unbindall","unbindall: unbinds all keys", osdcmd_unbindall);

    OSD_RegisterFunction("vidmode","vidmode <xdim> <ydim> <bpp> <fullscreen>: change the video mode",osdcmd_vidmode);
#ifdef USE_OPENGL
    baselayer_osdcmd_vidmode_func = osdcmd_vidmode;
#endif
    return 0;
}

