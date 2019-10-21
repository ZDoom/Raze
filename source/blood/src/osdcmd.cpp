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
        gSysRes.FNAddFiles(&fnlist, filename);

        for (r=fnlist.findfiles; r; r=r->next)
            maxwidth = max<int>(maxwidth, Bstrlen(r->name));

        if (maxwidth > 0)
        {
            int32_t x = 0;
            maxwidth += 3;
            OSD_Printf(OSDTEXT_RED "Map listing:\n");
            for (r=fnlist.findfiles; r; r=r->next)
            {
                OSD_Printf("%-*s",maxwidth,r->name);
                x += maxwidth;
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
    if (videoSetGameMode(gSetup.fullscreen,gSetup.xdim,gSetup.ydim,gSetup.bpp,0))
        ThrowError("restartvid: Reset failed...\n");
    onvideomodechange(gSetup.bpp>8);
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
            if (CDAudioToggle)
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
    int32_t newbpp = gSetup.bpp, newwidth = gSetup.xdim,
            newheight = gSetup.ydim, newfs = gSetup.fullscreen;
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
        if (videoSetGameMode(gSetup.fullscreen, gSetup.xdim, gSetup.ydim, gSetup.bpp, upscalefactor))
            ThrowError("vidmode: Reset failed!\n");
    }
    gSetup.bpp = newbpp;
    gSetup.xdim = newwidth;
    gSetup.ydim = newheight;
    gSetup.fullscreen = newfs;
    onvideomodechange(gSetup.bpp>8);
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
        SetGodMode(!gMe->at31a);
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

    if (MusicToggle && (gGameStarted || gDemo.at1))
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

    videoSetPalette(ud.brightness>>2, palid, 0);
    g_restorePalette = -1;
#endif
    if (newmode)
        scrResetPalette();
    UpdateDacs(gLastPal, false);
}

static int osdcmd_button(osdcmdptr_t parm)
{
    static char const s_gamefunc_[] = "gamefunc_";
    int constexpr strlen_gamefunc_  = ARRAY_SIZE(s_gamefunc_) - 1;

    char const *p = parm->name + strlen_gamefunc_;

//    if (g_player[myconnectindex].ps->gm == MODE_GAME) // only trigger these if in game
    if (gInputMode == kInputGame)
        CONTROL_ButtonFlags[CONFIG_FunctionNameToNum(p)] = 1; // FIXME

    return OSDCMD_OK;
}

const char *const ConsoleButtons[] =
{
    "mouse1", "mouse2", "mouse3", "mouse4", "mwheelup",
    "mwheeldn", "mouse5", "mouse6", "mouse7", "mouse8"
};

static int osdcmd_bind(osdcmdptr_t parm)
{
    char buffer[256];
    if (parm->numparms==1 && !Bstrcasecmp(parm->parms[0],"showkeys"))
    {
        for (auto & s : sctokeylut)
            OSD_Printf("%s\n", s.key);
        for (auto ConsoleButton : ConsoleButtons)
            OSD_Printf("%s\n",ConsoleButton);
        return OSDCMD_OK;
    }

    if (parm->numparms==0)
    {
        int j=0;

        OSD_Printf("Current key bindings:\n");

        for (int i=0; i<MAXBOUNDKEYS+MAXMOUSEBUTTONS; i++)
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

    int i, j, repeat;

    for (i=0; i < ARRAY_SSIZE(sctokeylut); i++)
    {
        if (!Bstrcasecmp(parm->parms[0], sctokeylut[i].key))
            break;
    }

    // didn't find the key
    if (i == ARRAY_SSIZE(sctokeylut))
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
        if (!Bstrcasecmp(parm->parms[j],"norepeat"))
        {
            repeat = 0;
            j++;
        }

        Bstrcpy(buffer,parm->parms[j++]);
        for (; j<parm->numparms; j++)
        {
            Bstrcat(buffer," ");
            Bstrcat(buffer,parm->parms[j++]);
        }

        CONTROL_BindMouse(i, buffer, repeat, ConsoleButtons[i]);

        if (!OSD_ParsingScript())
            OSD_Printf("%s\n",parm->raw);
        return OSDCMD_OK;
    }

    if (parm->numparms < 2)
    {
        if (CONTROL_KeyIsBound(sctokeylut[i].sc))
            OSD_Printf("%-9s %s\"%s\"\n", sctokeylut[i].key, CONTROL_KeyBinds[sctokeylut[i].sc].repeat?"":"norepeat ",
                       CONTROL_KeyBinds[sctokeylut[i].sc].cmdstr);
        else OSD_Printf("%s is unbound\n", sctokeylut[i].key);

        return OSDCMD_OK;
    }

    j = 1;

    repeat = 1;
    if (!Bstrcasecmp(parm->parms[j],"norepeat"))
    {
        repeat = 0;
        j++;
    }

    Bstrcpy(buffer,parm->parms[j++]);
    for (; j<parm->numparms; j++)
    {
        Bstrcat(buffer," ");
        Bstrcat(buffer,parm->parms[j++]);
    }

    CONTROL_BindKey(sctokeylut[i].sc, buffer, repeat, sctokeylut[i].key);

    char *cp = buffer;

    // Populate the keyboard config menu based on the bind.
    // Take care of processing one-to-many bindings properly, too.
    static char const s_gamefunc_[] = "gamefunc_";
    int constexpr strlen_gamefunc_  = ARRAY_SIZE(s_gamefunc_) - 1;

    while ((cp = Bstrstr(cp, s_gamefunc_)))
    {
        cp += strlen_gamefunc_;

        char *semi = Bstrchr(cp, ';');

        if (semi)
            *semi = 0;

        j = CONFIG_FunctionNameToNum(cp);

        if (semi)
            cp = semi+1;

        if (j != -1)
        {
            KeyboardKeys[j][1] = KeyboardKeys[j][0];
            KeyboardKeys[j][0] = sctokeylut[i].sc;
//            CONTROL_MapKey(j, sctokeylut[i].sc, ud.config.KeyboardKeys[j][0]);

            if (j == gamefunc_Show_Console)
                OSD_CaptureKey(sctokeylut[i].sc);
        }
    }

    if (!OSD_ParsingScript())
        OSD_Printf("%s\n",parm->raw);

    return OSDCMD_OK;
}

static int osdcmd_unbindall(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    for (int i = 0; i < MAXBOUNDKEYS; ++i)
        CONTROL_FreeKeyBind(i);

    for (int i = 0; i < MAXMOUSEBUTTONS; ++i)
        CONTROL_FreeMouseBind(i);

    for (auto &KeyboardKey : KeyboardKeys)
        KeyboardKey[0] = KeyboardKey[1] = 0xff;

    if (!OSD_ParsingScript())
        OSD_Printf("unbound all controls\n");

    return OSDCMD_OK;
}

static int osdcmd_unbind(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    for (auto ConsoleKey : sctokeylut)
    {
        if (ConsoleKey.key && !Bstrcasecmp(parm->parms[0], ConsoleKey.key))
        {
            CONTROL_FreeKeyBind(ConsoleKey.sc);
            OSD_Printf("unbound key %s\n", ConsoleKey.key);
            return OSDCMD_OK;
        }
    }

    for (int i = 0; i < MAXMOUSEBUTTONS; i++)
    {
        if (!Bstrcasecmp(parm->parms[0], ConsoleButtons[i]))
        {
            CONTROL_FreeMouseBind(i);
            OSD_Printf("unbound %s\n", ConsoleButtons[i]);
            return OSDCMD_OK;
        }
    }

    return OSDCMD_SHOWHELP;
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

static int osdcmd_cvar_set_game(osdcmdptr_t parm)
{
    int const r = osdcmd_cvar_set(parm);

    if (r != OSDCMD_OK) return r;

    if (!Bstrcasecmp(parm->name, "r_upscalefactor"))
    {
        if (in3dmode())
        {
            videoSetGameMode(fullscreen, xres, yres, bpp, gUpscaleFactor);
        }
    }
    else if (!Bstrcasecmp(parm->name, "r_size"))
    {
        //ud.statusbarmode = (ud.screen_size < 8);
        viewResizeView(gViewSize);
    }
    else if (!Bstrcasecmp(parm->name, "r_maxfps") || !Bstrcasecmp(parm->name, "r_maxfpsoffset"))
    {
        if (r_maxfps != 0) r_maxfps = clamp(r_maxfps, 30, 1000);
        g_frameDelay = calcFrameDelay(r_maxfps + r_maxfpsoffset);
    }
    else if (!Bstrcasecmp(parm->name, "r_ambientlight"))
    {
        if (r_ambientlight == 0)
            r_ambientlightrecip = 256.f;
        else r_ambientlightrecip = 1.f/r_ambientlight;
    }
    else if (!Bstrcasecmp(parm->name, "in_mouse"))
    {
        CONTROL_MouseEnabled = (gSetup.usemouse && CONTROL_MousePresent);
    }
    else if (!Bstrcasecmp(parm->name, "in_joystick"))
    {
        CONTROL_JoystickEnabled = (gSetup.usejoystick && CONTROL_JoyPresent);
    }
    else if (!Bstrcasecmp(parm->name, "vid_gamma"))
    {
        gBrightness = GAMMA_CALC;
        gBrightness <<= 2;
        videoSetPalette(gBrightness>>2,gLastPal,0);
    }
    else if (!Bstrcasecmp(parm->name, "vid_brightness") || !Bstrcasecmp(parm->name, "vid_contrast"))
    {
        videoSetPalette(gBrightness>>2,gLastPal,0);
    }
#if 0
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
        ud.color = G_CheckPlayerColor(ud.color);
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
#endif

    return r;
}

static int osdcmd_cvar_set_multi(osdcmdptr_t parm)
{
    int const r = osdcmd_cvar_set_game(parm);

    if (r != OSDCMD_OK) return r;

    //G_UpdatePlayerFromMenu();

    return r;
}

int32_t registerosdcommands(void)
{
    char buffer[256];

//
//    if (VOLUMEONE)
//        OSD_RegisterFunction("changelevel","changelevel <level>: warps to the given level", osdcmd_changelevel);
//    else
//    {
    OSD_RegisterFunction("changelevel","changelevel <volume> <level>: warps to the given level", osdcmd_changelevel);
    OSD_RegisterFunction("map","map <mapfile>: loads the given user map", osdcmd_map);
    OSD_RegisterFunction("demo","demo <demofile or demonum>: starts the given demo", osdcmd_demo);
//    }
//
//    OSD_RegisterFunction("addpath","addpath <path>: adds path to game filesystem", osdcmd_addpath);
    OSD_RegisterFunction("bind",R"(bind <key> <string>: associates a keypress with a string of console input. Type "bind showkeys" for a list of keys and "listsymbols" for a list of valid console commands.)", osdcmd_bind);
//    OSD_RegisterFunction("cmenu","cmenu <#>: jumps to menu", osdcmd_cmenu);
    OSD_RegisterFunction("crosshaircolor","crosshaircolor: changes the crosshair color", osdcmd_crosshaircolor);
    OSD_RegisterFunction("crosshairreset", "crosshairreset: restores the original crosshair", osdcmd_resetcrosshair);
//
//#if !defined NETCODE_DISABLE
//    OSD_RegisterFunction("connect","connect: connects to a multiplayer game", osdcmd_connect);
//    OSD_RegisterFunction("disconnect","disconnect: disconnects from the local multiplayer game", osdcmd_disconnect);
//#endif

    for (auto & func : gamefunctions)
    {
        if (func[0] == '\0')
            continue;

//        if (!Bstrcmp(gamefunctions[i],"Show_Console")) continue;

        Bsprintf(buffer, "gamefunc_%s", func);

        char *const t = Bstrtolower(Xstrdup(buffer));

        Bstrcat(buffer, ": game button");

        OSD_RegisterFunction(t, Xstrdup(buffer), osdcmd_button);
    }

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
//        KB_KeyDown[sc_Pause] = 1;
}

void GAME_clearbackground(int numcols, int numrows)
{
    COMMON_clearbackground(numcols, numrows);
}

END_BLD_NS
