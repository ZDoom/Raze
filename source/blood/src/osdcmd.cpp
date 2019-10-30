//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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

#include "build.h"
#include "baselayer.h"
#include "keyboard.h"
#include "control.h"
#include "osd.h"
#include "compat.h"
#include "mmulti.h"
#include "sndcards.h"
#include "common_game.h"
#include "config.h"
#include "blood.h"
#include "demo.h"
#include "gamemenu.h"
#include "globals.h"
#include "levels.h"
#include "messages.h"
#include "network.h"
#include "osdcmds.h"
#include "screen.h"
#include "sound.h"
#include "sfx.h"
#include "view.h"

BEGIN_BLD_NS

static inline int osdcmd_quit(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    OSD_ShowDisplay(0);
    QuitGame();
    return OSDCMD_OK;
}

static int osdcmd_changelevel(osdcmdptr_t parm)
{
    int32_t volume,level;
    char *p;

    if (parm->numparms != 2) return OSDCMD_SHOWHELP;

    volume = strtol(parm->parms[0], &p, 10) - 1;
    if (p[0]) return OSDCMD_SHOWHELP;
    level = strtol(parm->parms[1], &p, 10) - 1;
    if (p[0]) return OSDCMD_SHOWHELP;

    if (volume < 0) return OSDCMD_SHOWHELP;
    if (level < 0) return OSDCMD_SHOWHELP;

    if (volume >= 6)
    {
        OSD_Printf("changelevel: invalid volume number (range 1-%d)\n",6);
        return OSDCMD_OK;
    }

    if (level >= gEpisodeInfo[volume].nLevels)
    {
        OSD_Printf("changelevel: invalid level number\n");
        return OSDCMD_SHOWHELP;
    }

    if (gDemo.at1)
        gDemo.StopPlayback();

    if (numplayers > 1)
    {
        gPacketStartGame.episodeId = volume;
        gPacketStartGame.levelId = level;
        netBroadcastNewGame();
        gStartNewGame = 1;
        gGameMenuMgr.Deactivate();
        return OSDCMD_OK;
    }
    levelSetupOptions(volume, level);
    StartLevel(&gGameOptions);
    viewResizeView(gViewSize);
    gGameMenuMgr.Deactivate();

    return OSDCMD_OK;
}

static int osdcmd_map(osdcmdptr_t parm)
{
    char filename[BMAX_PATH];

    strcpy(filename, parm->parms[0]);
    ChangeExtension(filename, "");

    if (!gSysRes.Lookup(filename, "MAP"))
    {
        OSD_Printf(OSD_ERROR "map: file \"%s\" not found.\n", filename);
        return OSDCMD_OK;
    }

    if (gDemo.at1)
        gDemo.StopPlayback();

    levelAddUserMap(filename);

    if (numplayers > 1)
    {
        gPacketStartGame.episodeId = gGameOptions.nEpisode;
        gPacketStartGame.levelId = gGameOptions.nLevel;
        netBroadcastNewGame();
        gStartNewGame = 1;
        gGameMenuMgr.Deactivate();
        return OSDCMD_OK;
    }
    levelSetupOptions(gGameOptions.nEpisode, gGameOptions.nLevel);
    StartLevel(&gGameOptions);
    viewResizeView(gViewSize);
    gGameMenuMgr.Deactivate();

    return OSDCMD_OK;
}

static int osdcmd_demo(osdcmdptr_t parm)
{
    if (numplayers > 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    //if (g_player[myconnectindex].ps->gm & MODE_GAME)
    //{
    //    OSD_Printf("demo: Must not be in a game.\n");
    //    return OSDCMD_OK;
    //}

    if (parm->numparms != 1/* && parm->numparms != 2*/)
        return OSDCMD_SHOWHELP;

    gDemo.SetupPlayback(parm->parms[0]);
    gGameStarted = 0;
    gDemo.Playback();

    return OSDCMD_OK;
}

int osdcmd_restartvid(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    videoResetMode();
    if (videoSetGameMode(ScreenMode,ScreenWidth,ScreenHeight,ScreenBPP,0))
        ThrowError("restartvid: Reset failed...\n");
    onvideomodechange(ScreenBPP>8);
    viewResizeView(gViewSize);

    return OSDCMD_OK;
}

static int osdcmd_music(osdcmdptr_t parm)
{
    char buffer[128];
    if (parm->numparms == 1)
    {
        int32_t sel = levelGetMusicIdx(parm->parms[0]);

        if (sel == -1)
            return OSDCMD_SHOWHELP;

        if (sel == -2)
        {
            OSD_Printf("%s is not a valid episode/level number pair\n", parm->parms[0]);
            return OSDCMD_OK;
        }

        int nEpisode = sel/kMaxLevels;
        int nLevel = sel%kMaxLevels;

        if (!levelTryPlayMusic(nEpisode, nLevel))
        {
            if (mus_redbook)
                snprintf(buffer, sizeof(buffer), "Playing %i track", gEpisodeInfo[nEpisode].at28[nLevel].ate0);
            else
                snprintf(buffer, sizeof(buffer), "Playing %s", gEpisodeInfo[nEpisode].at28[nLevel].atd0);
            viewSetMessage(buffer);
        }
        else
        {
            OSD_Printf("No music defined for %s\n", parm->parms[0]);
        }

        return OSDCMD_OK;
    }

    return OSDCMD_SHOWHELP;
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
            ThrowError("vidmode: Reset failed!\n");
    }
    ScreenBPP = newbpp;
    ScreenWidth = newwidth;
    ScreenHeight = newheight;
    ScreenMode = newfs;
    onvideomodechange(ScreenBPP>8);
    viewResizeView(gViewSize);
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

    g_isAlterDefaultCrosshair = true;
    viewSetCrosshairColor(r,g,b);

    if (!OSD_ParsingScript())
        OSD_Printf("%s\n", parm->raw);

    return OSDCMD_OK;
}

static int osdcmd_resetcrosshair(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    g_isAlterDefaultCrosshair = false;
    viewResetCrosshairToDefault();

    return OSDCMD_OK;
}

static int osdcmd_give(osdcmdptr_t parm)
{
    if (numplayers != 1 || !gGameStarted || gMe->pXSprite->health == 0)
    {
        OSD_Printf("give: Cannot give while dead or not in a single-player game.\n");
        return OSDCMD_OK;
    }

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    if (!Bstrcasecmp(parm->parms[0], "all"))
    {
        SetWeapons(true);
        SetAmmo(true);
        SetToys(true);
        SetArmor(true);
        SetKeys(true);
        gCheatMgr.m_bPlayerCheated = true;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "health"))
    {
        actHealDude(gMe->pXSprite, 200, 200);
        gCheatMgr.m_bPlayerCheated = true;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "weapons"))
    {
        SetWeapons(true);
        gCheatMgr.m_bPlayerCheated = true;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "ammo"))
    {
        SetAmmo(true);
        gCheatMgr.m_bPlayerCheated = true;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "armor"))
    {
        SetArmor(true);
        gCheatMgr.m_bPlayerCheated = true;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "keys"))
    {
        SetKeys(true);
        gCheatMgr.m_bPlayerCheated = true;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "inventory"))
    {
        SetToys(true);
        gCheatMgr.m_bPlayerCheated = true;
        return OSDCMD_OK;
    }
    return OSDCMD_SHOWHELP;
}

static int osdcmd_god(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    if (numplayers == 1 && gGameStarted)
    {
        SetGodMode(!gMe->godMode);
        gCheatMgr.m_bPlayerCheated = true;
    }
    else
        OSD_Printf("god: Not in a single-player game.\n");

    return OSDCMD_OK;
}

static int osdcmd_noclip(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    if (numplayers == 1 && gGameStarted)
    {
        SetClipMode(!gNoClip);
        gCheatMgr.m_bPlayerCheated = true;
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
    sfxTerm();
    sndTerm();

    sndInit();
    sfxInit();

    if (MusicEnabled() && (gGameStarted || gDemo.at1))
        sndPlaySong(gGameOptions.zLevelSong, true);

    return OSDCMD_OK;
}

void onvideomodechange(int32_t newmode)
{
    UNREFERENCED_PARAMETER(newmode);
#if 0
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
#endif
    if (newmode)
        scrResetPalette();
    UpdateDacs(gLastPal, false);
}

static int osdcmd_unbound(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
        return OSDCMD_OK;

    int const gameFunc = CONFIG_FunctionNameToNum(parm->parms[0]);

    if (gameFunc != -1)
        KeyboardKeys[gameFunc][0] = 0;

    return OSDCMD_OK;
}

static int osdcmd_quicksave(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    if (!gGameStarted || gDemo.at1 || gGameMenuMgr.m_bActive)
        OSD_Printf("quicksave: not in a game.\n");
    else gDoQuickSave = 1;
    return OSDCMD_OK;
}

static int osdcmd_quickload(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    if (!gGameStarted || gDemo.at1 || gGameMenuMgr.m_bActive)
        OSD_Printf("quickload: not in a game.\n");
    else gDoQuickSave = 2;
    return OSDCMD_OK;
}

static int osdcmd_screenshot(osdcmdptr_t parm)
{
    static const char *fn = "blud0000.png";

    if (parm->numparms == 1 && !Bstrcasecmp(parm->parms[0], "tga"))
        videoCaptureScreenTGA(fn, 0);
    else videoCaptureScreen(fn, 0);

    return OSDCMD_OK;
}

#if 0
static int osdcmd_savestate(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_PARAMETER(parm);
    G_SaveMapState();
    return OSDCMD_OK;
}

static int osdcmd_restorestate(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_PARAMETER(parm);
    G_RestoreMapState();
    return OSDCMD_OK;
}
#endif

#if 0
#ifdef DEBUGGINGAIDS
static int osdcmd_inittimer(osdcmdptr_t parm)
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
#endif

int32_t registerosdcommands(void)
{
    OSD_RegisterFunction("changelevel","changelevel <volume> <level>: warps to the given level", osdcmd_changelevel);
    OSD_RegisterFunction("map","map <mapfile>: loads the given user map", osdcmd_map);
    OSD_RegisterFunction("demo","demo <demofile or demonum>: starts the given demo", osdcmd_demo);
//    }
//
    OSD_RegisterFunction("bind",R"(bind <key> <string>: associates a keypress with a string of console input. Type "bind showkeys" for a list of keys and "listsymbols" for a list of valid console commands.)", osdcmd_bind);
//    OSD_RegisterFunction("cmenu","cmenu <#>: jumps to menu", osdcmd_cmenu);
    OSD_RegisterFunction("crosshaircolor","crosshaircolor: changes the crosshair color", osdcmd_crosshaircolor);
    OSD_RegisterFunction("crosshairreset", "crosshairreset: restores the original crosshair", osdcmd_resetcrosshair);

    OSD_RegisterFunction("give","give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item", osdcmd_give);
    OSD_RegisterFunction("god","god: toggles god mode", osdcmd_god);
//    OSD_RegisterFunction("activatecheat","activatecheat <id>: activates a cheat code", osdcmd_activatecheat);
//
//    OSD_RegisterFunction("initgroupfile","initgroupfile <path>: adds a grp file into the game filesystem", osdcmd_initgroupfile);
//#ifdef DEBUGGINGAIDS
//    OSD_RegisterFunction("inittimer","debug", osdcmd_inittimer);
//#endif
//#if !defined NETCODE_DISABLE
//    OSD_RegisterFunction("kick","kick <id>: kicks a multiplayer client.  See listplayers.", osdcmd_kick);
//    OSD_RegisterFunction("kickban","kickban <id>: kicks a multiplayer client and prevents them from reconnecting.  See listplayers.", osdcmd_kickban);
//
//    OSD_RegisterFunction("listplayers","listplayers: lists currently connected multiplayer clients", osdcmd_listplayers);
//#endif
    OSD_RegisterFunction("music","music E<ep>L<lev>: change music", osdcmd_music);
//
//#if !defined NETCODE_DISABLE
//    OSD_RegisterFunction("name","name: change your multiplayer nickname", osdcmd_name);
//#endif
//
    OSD_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);
//
//#if !defined NETCODE_DISABLE
//    OSD_RegisterFunction("password","password: sets multiplayer game password", osdcmd_password);
//#endif
//
//    OSD_RegisterFunction("printtimes", "printtimes: prints VM timing statistics", osdcmd_printtimes);
//
//    OSD_RegisterFunction("purgesaves", "purgesaves: deletes obsolete and unreadable save files", osdcmd_purgesaves);
//
    OSD_RegisterFunction("quicksave","quicksave: performs a quick save", osdcmd_quicksave);
    OSD_RegisterFunction("quickload","quickload: performs a quick load", osdcmd_quickload);
    OSD_RegisterFunction("quit","quit: exits the game immediately", osdcmd_quit);
    OSD_RegisterFunction("exit","exit: exits the game immediately", osdcmd_quit);
//
//    OSD_RegisterFunction("restartmap", "restartmap: restarts the current map", osdcmd_restartmap);
    OSD_RegisterFunction("restartsound","restartsound: reinitializes the sound system",osdcmd_restartsound);
    OSD_RegisterFunction("restartvid","restartvid: reinitializes the video mode",osdcmd_restartvid);
//#if !defined LUNATIC
//    OSD_RegisterFunction("addlogvar","addlogvar <gamevar>: prints the value of a gamevar", osdcmd_addlogvar);
//    OSD_RegisterFunction("setvar","setvar <gamevar> <value>: sets the value of a gamevar", osdcmd_setvar);
//    OSD_RegisterFunction("setvarvar","setvarvar <gamevar1> <gamevar2>: sets the value of <gamevar1> to <gamevar2>", osdcmd_setvar);
//    OSD_RegisterFunction("setactorvar","setactorvar <actor#> <gamevar> <value>: sets the value of <actor#>'s <gamevar> to <value>", osdcmd_setactorvar);
//#else
//    OSD_RegisterFunction("lua", "lua \"Lua code...\": runs Lunatic code", osdcmd_lua);
//#endif
    OSD_RegisterFunction("screenshot","screenshot [format]: takes a screenshot.", osdcmd_screenshot);
//
//    OSD_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",osdcmd_spawn);

    OSD_RegisterFunction("unbind","unbind <key>: unbinds a key", osdcmd_unbind);
    OSD_RegisterFunction("unbindall","unbindall: unbinds all keys", osdcmd_unbindall);
    OSD_RegisterFunction("unbound", NULL, osdcmd_unbound);

    OSD_RegisterFunction("vidmode","vidmode <xdim> <ydim> <bpp> <fullscreen>: change the video mode",osdcmd_vidmode);
#ifdef USE_OPENGL
    baselayer_osdcmd_vidmode_func = osdcmd_vidmode;
#endif
//
//#ifndef NETCODE_DISABLE
//    OSD_RegisterFunction("dumpmapstates", "Dumps current snapshots to CL/Srv_MapStates.bin", osdcmd_dumpmapstate);
//    OSD_RegisterFunction("playerinfo", "Prints information about the current player", osdcmd_playerinfo);
//#endif

    return 0;
}

void GAME_onshowosd(int shown)
{
    // G_UpdateScreenArea();

    mouseLockToWindow((!shown) + 2);

    //osdshown = shown;

    // XXX: it's weird to fake a keypress like this.
//    if (numplayers == 1 && ((shown && !ud.pause_on) || (!shown && ud.pause_on)))
//        	inputState.SetKeyStatus(sc_Escape, 1);

}

void GAME_clearbackground(int numcols, int numrows)
{
    COMMON_clearbackground(numcols, numrows);
}

END_BLD_NS
