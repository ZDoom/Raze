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

#include "compat.h"
#include "build.h"
#include "common.h"
#include "keyboard.h"
#include "control.h"
#include "exhumed.h"
#include "config.h"
#include "osdcmds.h"

#include "vfs.h"

static inline int osdcmd_quit(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    OSD_ShowDisplay(0);
    ShutDown();
    return OSDCMD_OK;
}


int osdcmd_restartvid(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    videoResetMode();
    if (videoSetGameMode(gSetup.fullscreen,gSetup.xdim,gSetup.ydim,gSetup.bpp,0))
        bail2dos("restartvid: Reset failed...\n");
    onvideomodechange(gSetup.bpp>8);
    UpdateScreenSize();

    return OSDCMD_OK;
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
            bail2dos("vidmode: Reset failed!\n");
    }
    gSetup.bpp = newbpp;
    gSetup.xdim = newwidth;
    gSetup.ydim = newheight;
    gSetup.fullscreen = newfs;
    onvideomodechange(gSetup.bpp>8);
    UpdateScreenSize();
    return OSDCMD_OK;
}

static int osdcmd_addpath(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    addsearchpath(parm->parms[0]);

    return OSDCMD_OK;
}

static int osdcmd_initgroupfile(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    initgroupfile(parm->parms[0]);

    return OSDCMD_OK;
}

void onvideomodechange(int32_t newmode)
{
    // TODO:
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
    g_crosshairSum = -1;
#endif
}

static int osdcmd_button(osdcmdptr_t parm)
{
    static char const s_gamefunc_[] = "gamefunc_";
    int constexpr strlen_gamefunc_  = ARRAY_SIZE(s_gamefunc_) - 1;

    char const *p = parm->name + strlen_gamefunc_;

//    if (g_player[myconnectindex].ps->gm == MODE_GAME) // only trigger these if in game
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
    char tempbuf[256];
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

    Bstrcpy(tempbuf,parm->parms[j++]);
    for (; j<parm->numparms; j++)
    {
        Bstrcat(tempbuf," ");
        Bstrcat(tempbuf,parm->parms[j++]);
    }

    CONTROL_BindKey(sctokeylut[i].sc, tempbuf, repeat, sctokeylut[i].key);

    char *cp = tempbuf;

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

    for (auto & ConsoleKey : sctokeylut)
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

static int osdcmd_screenshot(osdcmdptr_t parm)
{
//    KB_ClearKeysDown();
    static const char *fn = "capt0000.png";

    if (parm->numparms == 1 && !Bstrcasecmp(parm->parms[0], "tga"))
        videoCaptureScreenTGA(fn, 0);
    else videoCaptureScreen(fn, 0);

    return OSDCMD_OK;
}

static int osdcmd_cvar_set_game(osdcmdptr_t parm)
{
    int const r = osdcmd_cvar_set(parm);

    if (r != OSDCMD_OK) return r;

    // TODO:
    /*if (!Bstrcasecmp(parm->name, "r_upscalefactor"))
    {
        if (in3dmode())
        {
            videoSetGameMode(fullscreen, xres, yres, bpp, ud.detail);
        }
    }
    else if (!Bstrcasecmp(parm->name, "r_size"))
    {
        ud.statusbarmode = (ud.screen_size < 8);
        G_UpdateScreenArea();
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
    else */if (!Bstrcasecmp(parm->name, "in_mouse"))
    {
        CONTROL_MouseEnabled = (gSetup.usemouse && CONTROL_MousePresent);
    }
    else if (!Bstrcasecmp(parm->name, "in_joystick"))
    {
        CONTROL_JoystickEnabled = (gSetup.usejoystick && CONTROL_JoyPresent);
    }
    else if (!Bstrcasecmp(parm->name, "vid_gamma"))
    {
        // TODO
        //ud.brightness = GAMMA_CALC;
        //ud.brightness <<= 2;
        //videoSetPalette(ud.brightness>>2,g_player[myconnectindex].ps->palette,0);
    }
    else if (!Bstrcasecmp(parm->name, "vid_brightness") || !Bstrcasecmp(parm->name, "vid_contrast"))
    {
        // TODO
        //videoSetPalette(ud.brightness>>2,g_player[myconnectindex].ps->palette,0);
    }
    //else if (!Bstrcasecmp(parm->name, "hud_scale")
    //         || !Bstrcasecmp(parm->name, "hud_statusbarmode")
    //         || !Bstrcasecmp(parm->name, "r_rotatespritenowidescreen"))
    //{
    //    G_UpdateScreenArea();
    //}
    //else if (!Bstrcasecmp(parm->name, "skill"))
    //{
    //    if (numplayers > 1)
    //        return r;
    //
    //    ud.player_skill = ud.m_player_skill;
    //}
    //else if (!Bstrcasecmp(parm->name, "color"))
    //{
    //    ud.color = G_CheckPlayerColor(ud.color);
    //    g_player[0].ps->palookup = g_player[0].pcolor = ud.color;
    //}
    //else if (!Bstrcasecmp(parm->name, "osdscale"))
    //{
    //    osdrscale = 1.f/osdscale;
    //
    //    if (xdim && ydim)
    //        OSD_ResizeDisplay(xdim, ydim);
    //}
    //else if (!Bstrcasecmp(parm->name, "wchoice"))
    //{
    //    if (parm->numparms == 1)
    //    {
    //        if (g_forceWeaponChoice) // rewrite ud.wchoice because osdcmd_cvar_set already changed it
    //        {
    //            int j = 0;
    //
    //            while (j < 10)
    //            {
    //                ud.wchoice[j] = g_player[myconnectindex].wchoice[j] + '0';
    //                j++;
    //            }
    //
    //            ud.wchoice[j] = 0;
    //        }
    //        else
    //        {
    //            char const *c = parm->parms[0];
    //
    //            if (*c)
    //            {
    //                int j = 0;
    //
    //                while (*c && j < 10)
    //                {
    //                    g_player[myconnectindex].wchoice[j] = *c - '0';
    //                    c++;
    //                    j++;
    //                }
    //
    //                while (j < 10)
    //                {
    //                    if (j == 9)
    //                        g_player[myconnectindex].wchoice[9] = 1;
    //                    else
    //                        g_player[myconnectindex].wchoice[j] = 2;
    //
    //                    j++;
    //                }
    //            }
    //        }
    //
    //        g_forceWeaponChoice = 0;
    //    }
    //
    //    /*    Net_SendClientInfo();*/
    //}

    return r;
}

int32_t registerosdcommands(void)
{
    char tempbuf[256];
    static osdcvardata_t cvars_game[] =
    {
        // TODO:
        //{ "benchmarkmode", "Set the benchmark mode (0: off, 1: performance test, 2: generate reference screenshots for correctness testing)", (void *) &g_BenchmarkMode, CVAR_INT|CVAR_NOSAVE, 0, 2 },

        //{ "crosshair", "enable/disable crosshair", (void *)&ud.crosshair, CVAR_BOOL, 0, 1 },

        //{ "cl_autoaim", "enable/disable weapon autoaim", (void *)&ud.config.AutoAim, CVAR_INT|CVAR_MULTI, 0, 3 },
        //{ "cl_automsg", "enable/disable automatically sending messages to all players", (void *)&ud.automsg, CVAR_BOOL, 0, 1 },
        //{ "cl_autorun", "enable/disable autorun", (void *)&ud.auto_run, CVAR_BOOL, 0, 1 },

        //{ "cl_autosave", "enable/disable autosaves", (void *) &ud.autosave, CVAR_BOOL, 0, 1 },
        //{ "cl_autosavedeletion", "enable/disable automatic deletion of autosaves", (void *) &ud.autosavedeletion, CVAR_BOOL, 0, 1 },
        //{ "cl_maxautosaves", "number of autosaves to keep before deleting the oldest", (void *) &ud.maxautosaves, CVAR_INT, 1, 100 },

        //{ "cl_autovote", "enable/disable automatic voting", (void *)&ud.autovote, CVAR_INT, 0, 2 },

        //{ "cl_cheatmask", "configure what cheats show in the cheats menu", (void *)&cl_cheatmask, CVAR_UINT, 0, ~0 },

        //{ "cl_obituaries", "enable/disable multiplayer death messages", (void *)&ud.obituaries, CVAR_BOOL, 0, 1 },
        //{ "cl_democams", "enable/disable demo playback cameras", (void *)&ud.democams, CVAR_BOOL, 0, 1 },

        //{ "cl_idplayers", "enable/disable name display when aiming at opponents", (void *)&ud.idplayers, CVAR_BOOL, 0, 1 },

        //{ "cl_runmode", "enable/disable modernized run key operation", (void *)&ud.runkey_mode, CVAR_BOOL, 0, 1 },

//        { "cl_showcoords", "show your position in the game world", (void *)&ud.coords, CVAR_INT, 0,
//#ifdef USE_OPENGL
//          2
//#else
//          1
//#endif
//        },

        //{ "cl_viewbob", "enable/disable player head bobbing", (void *)&ud.viewbob, CVAR_BOOL, 0, 1 },

        //{ "cl_weaponsway", "enable/disable player weapon swaying", (void *)&ud.weaponsway, CVAR_BOOL, 0, 1 },
        //{ "cl_weaponswitch", "enable/disable auto weapon switching", (void *)&ud.weaponswitch, CVAR_INT|CVAR_MULTI, 0, 7 },

        //{ "color", "changes player palette", (void *)&ud.color, CVAR_INT|CVAR_MULTI, 0, MAXPALOOKUPS-1 },

        //{ "crosshairscale","changes the size of the crosshair", (void *)&ud.crosshairscale, CVAR_INT, 10, 100 },

        //{ "demorec_diffs","enable/disable diff recording in demos",(void *)&demorec_diffs_cvar, CVAR_BOOL, 0, 1 },
        //{ "demorec_force","enable/disable forced demo recording",(void *)&demorec_force_cvar, CVAR_BOOL|CVAR_NOSAVE, 0, 1 },
        //{
        //    "demorec_difftics","sets game tic interval after which a diff is recorded",
        //    (void *)&demorec_difftics_cvar, CVAR_INT, 2, 60*REALGAMETICSPERSEC
        //},
        //{ "demorec_diffcompress","Compression method for diffs. (0: none, 1: KSLZW)",(void *)&demorec_diffcompress_cvar, CVAR_BOOL, 0, 1 },
        //{ "demorec_synccompress","Compression method for input. (0: none, 1: KSLZW)",(void *)&demorec_synccompress_cvar, CVAR_BOOL, 0, 1 },
        //{ "demorec_seeds","enable/disable recording of random seed for later sync checking",(void *)&demorec_seeds_cvar, CVAR_BOOL, 0, 1 },
        //{ "demoplay_diffs","enable/disable application of diffs in demo playback",(void *)&demoplay_diffs, CVAR_BOOL, 0, 1 },
        //{ "demoplay_showsync","enable/disable display of sync status",(void *)&demoplay_showsync, CVAR_BOOL, 0, 1 },

        //{ "fov", "change the field of view", (void *)&ud.fov, CVAR_INT|CVAR_FUNCPTR, 60, 120 },

        //{ "hud_althud", "enable/disable alternate mini-hud", (void *)&ud.althud, CVAR_BOOL, 0, 1 },
        //{ "hud_custom", "change the custom hud", (void *)&ud.statusbarcustom, CVAR_INT, 0, ud.statusbarrange },
        //{ "hud_position", "aligns the status bar to the bottom/top", (void *)&ud.hudontop, CVAR_BOOL, 0, 1 },
        //{ "hud_bgstretch", "enable/disable background image stretching in wide resolutions", (void *)&ud.bgstretch, CVAR_BOOL, 0, 1 },
        //{ "hud_messagetime", "length of time to display multiplayer chat messages", (void *)&ud.msgdisptime, CVAR_INT, 0, 3600 },
        //{ "hud_numbertile", "first tile in alt hud number set", (void *)&althud_numbertile, CVAR_INT, 0, MAXUSERTILES-10 },
        //{ "hud_numberpal", "pal for alt hud numbers", (void *)&althud_numberpal, CVAR_INT, 0, MAXPALOOKUPS-1 },
        //{ "hud_shadows", "enable/disable althud shadows", (void *)&althud_shadows, CVAR_BOOL, 0, 1 },
        //{ "hud_flashing", "enable/disable althud flashing", (void *)&althud_flashing, CVAR_BOOL, 0, 1 },
        //{ "hud_glowingquotes", "enable/disable \"glowing\" quote text", (void *)&hud_glowingquotes, CVAR_BOOL, 0, 1 },
        //{ "hud_scale","changes the hud scale", (void *)&ud.statusbarscale, CVAR_INT|CVAR_FUNCPTR, 36, 100 },
        //{ "hud_showmapname", "enable/disable map name display on load", (void *)&hud_showmapname, CVAR_BOOL, 0, 1 },
        //{ "hud_stats", "enable/disable level statistics display", (void *)&ud.levelstats, CVAR_BOOL, 0, 1 },
        //{ "hud_textscale", "sets multiplayer chat message size", (void *)&ud.textscale, CVAR_INT, 100, 400 },
        //{ "hud_weaponscale","changes the weapon scale", (void *)&ud.weaponscale, CVAR_INT, 10, 100 },
        //{ "hud_statusbarmode", "change overlay mode of status bar", (void *)&ud.statusbarmode, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },

//#ifdef EDUKE32_TOUCH_DEVICES
//        { "hud_hidestick", "hide the touch input stick", (void *)&droidinput.hideStick, CVAR_BOOL, 0, 1 },
//#endif

        { "in_joystick","enables input from the joystick if it is present",(void *)&gSetup.usejoystick, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "in_mouse","enables input from the mouse if it is present",(void *)&gSetup.usemouse, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },

        //{ "in_aimmode", "0:toggle, 1:hold to aim", (void *)&ud.mouseaiming, CVAR_BOOL, 0, 1 },
        {
            "in_mousebias", "emulates the original mouse code's weighting of input towards whichever axis is moving the most at any given time",
            (void *)&MouseBias, CVAR_INT, 0, 32
        },
        { "in_mousedeadzone", "amount of mouse movement to filter out", (void *)&MouseDeadZone, CVAR_INT, 0, 512 },
        //{ "in_mouseflip", "invert vertical mouse movement", (void *)&ud.mouseflip, CVAR_BOOL, 0, 1 },
        //{ "in_mousemode", "toggles vertical mouse view", (void *)&g_myAimMode, CVAR_BOOL, 0, 1 },
        { "in_mousesmoothing", "enable/disable mouse input smoothing", (void *)&SmoothInput, CVAR_BOOL, 0, 1 },

        //{ "mus_enabled", "enables/disables music", (void *)&ud.config.MusicToggle, CVAR_BOOL, 0, 1 },
        //{ "mus_volume", "controls music volume", (void *)&ud.config.MusicVolume, CVAR_INT, 0, 255 },

        //{ "osdhightile", "enable/disable hires art replacements for console text", (void *)&osdhightile, CVAR_BOOL, 0, 1 },
        //{ "osdscale", "adjust console text size", (void *)&osdscale, CVAR_FLOAT|CVAR_FUNCPTR, 1, 4 },

        //{ "r_camrefreshdelay", "minimum delay between security camera sprite updates, 120 = 1 second", (void *)&ud.camera_time, CVAR_INT, 1, 240 },
        //{ "r_drawweapon", "enable/disable weapon drawing", (void *)&ud.drawweapon, CVAR_INT, 0, 2 },
        //{ "r_showfps", "show the frame rate counter", (void *)&ud.showfps, CVAR_INT, 0, 3 },
        //{ "r_showfpsperiod", "time in seconds before averaging min and max stats for r_showfps 2+", (void *)&ud.frameperiod, CVAR_INT, 0, 5 },
        //{ "r_shadows", "enable/disable sprite and model shadows", (void *)&ud.shadows, CVAR_BOOL, 0, 1 },
        //{ "r_size", "change size of viewable area", (void *)&ud.screen_size, CVAR_INT|CVAR_FUNCPTR, 0, 64 },
        //{ "r_rotatespritenowidescreen", "pass bit 1024 to all CON rotatesprite calls", (void *)&g_rotatespriteNoWidescreen, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        //{ "r_upscalefactor", "increase performance by rendering at upscalefactor less than the screen resolution and upscale to the full resolution in the software renderer", (void *)&ud.detail, CVAR_INT|CVAR_FUNCPTR, 1, 16 },
        { "r_precache", "enable/disable the pre-level caching routine", (void *)&useprecache, CVAR_BOOL, 0, 1 },

       // { "r_ambientlight", "sets the global map light level",(void *)&r_ambientlight, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        //{ "r_maxfps", "limit the frame rate",(void *)&r_maxfps, CVAR_INT|CVAR_FUNCPTR, 0, 1000 },
        //{ "r_maxfpsoffset", "menu-controlled offset for r_maxfps",(void *)&r_maxfpsoffset, CVAR_INT|CVAR_FUNCPTR, -10, 10 },

        { "sensitivity","changes the mouse sensitivity", (void *)&CONTROL_MouseSensitivity, CVAR_FLOAT|CVAR_FUNCPTR, 0, 25 },

        //{ "skill","changes the game skill setting", (void *)&ud.m_player_skill, CVAR_INT|CVAR_FUNCPTR|CVAR_NOSAVE/*|CVAR_NOMULTI*/, 0, 5 },

        //{ "snd_ambience", "enables/disables ambient sounds", (void *)&ud.config.AmbienceToggle, CVAR_BOOL, 0, 1 },
        //{ "snd_enabled", "enables/disables sound effects", (void *)&ud.config.SoundToggle, CVAR_BOOL, 0, 1 },
        //{ "snd_fxvolume", "controls volume for sound effects", (void *)&ud.config.FXVolume, CVAR_INT, 0, 255 },
        //{ "snd_mixrate", "sound mixing rate", (void *)&ud.config.MixRate, CVAR_INT, 0, 48000 },
        //{ "snd_numchannels", "the number of sound channels", (void *)&ud.config.NumChannels, CVAR_INT, 0, 2 },
        //{ "snd_numvoices", "the number of concurrent sounds", (void *)&ud.config.NumVoices, CVAR_INT, 1, 128 },
        //{ "snd_reversestereo", "reverses the stereo channels", (void *)&ud.config.ReverseStereo, CVAR_BOOL, 0, 1 },
        //{ "snd_speech", "enables/disables player speech", (void *)&ud.config.VoiceToggle, CVAR_INT, 0, 5 },

        //{ "team","change team in multiplayer", (void *)&ud.team, CVAR_INT|CVAR_MULTI, 0, 3 },

        { "vid_gamma","adjusts gamma component of gamma ramp",(void *)&g_videoGamma, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "vid_contrast","adjusts contrast component of gamma ramp",(void *)&g_videoContrast, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "vid_brightness","adjusts brightness component of gamma ramp",(void *)&g_videoBrightness, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        //{ "wchoice","sets weapon autoselection order", (void *)ud.wchoice, CVAR_STRING|CVAR_FUNCPTR, 0, MAX_WEAPONS },
    };

    //osdcmd_cheatsinfo_stat.cheatnum = -1;

    for (auto & cv : cvars_game)
    {
        switch (cv.flags & (CVAR_FUNCPTR|CVAR_MULTI))
        {
            case CVAR_FUNCPTR:
                OSD_RegisterCvar(&cv, osdcmd_cvar_set_game); break;
            //case CVAR_MULTI:
            //case CVAR_FUNCPTR|CVAR_MULTI:
            //    OSD_RegisterCvar(&cv, osdcmd_cvar_set_multi); break;
            default:
                OSD_RegisterCvar(&cv, osdcmd_cvar_set); break;
        }
    }

    //if (VOLUMEONE)
    //    OSD_RegisterFunction("changelevel","changelevel <level>: warps to the given level", osdcmd_changelevel);
    //else
    //{
    //    OSD_RegisterFunction("changelevel","changelevel <volume> <level>: warps to the given level", osdcmd_changelevel);
    //    OSD_RegisterFunction("map","map <mapfile>: loads the given user map", osdcmd_map);
    //    OSD_RegisterFunction("demo","demo <demofile or demonum>: starts the given demo", osdcmd_demo);
    //}

    OSD_RegisterFunction("addpath","addpath <path>: adds path to game filesystem", osdcmd_addpath);
    OSD_RegisterFunction("bind",R"(bind <key> <string>: associates a keypress with a string of console input. Type "bind showkeys" for a list of keys and "listsymbols" for a list of valid console commands.)", osdcmd_bind);
    //OSD_RegisterFunction("cmenu","cmenu <#>: jumps to menu", osdcmd_cmenu);
    //OSD_RegisterFunction("crosshaircolor","crosshaircolor: changes the crosshair color", osdcmd_crosshaircolor);

    for (auto & func : gamefunctions)
    {
        if (func[0] == '\0')
            continue;

//        if (!Bstrcmp(gamefunctions[i],"Show_Console")) continue;

        Bsprintf(tempbuf, "gamefunc_%s", func);

        char *const t = Bstrtolower(Xstrdup(tempbuf));

        Bstrcat(tempbuf, ": game button");

        OSD_RegisterFunction(t, Xstrdup(tempbuf), osdcmd_button);
    }

    //OSD_RegisterFunction("give","give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item", osdcmd_give);
    //OSD_RegisterFunction("god","god: toggles god mode", osdcmd_god);
    //OSD_RegisterFunction("activatecheat","activatecheat <id>: activates a cheat code", osdcmd_activatecheat);

    OSD_RegisterFunction("initgroupfile","initgroupfile <path>: adds a grp file into the game filesystem", osdcmd_initgroupfile);
//#ifdef DEBUGGINGAIDS
//    OSD_RegisterFunction("inittimer","debug", osdcmd_inittimer);
//#endif
    //OSD_RegisterFunction("music","music E<ep>L<lev>: change music", osdcmd_music);

    //OSD_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);


    //OSD_RegisterFunction("printtimes", "printtimes: prints VM timing statistics", osdcmd_printtimes);

    //OSD_RegisterFunction("purgesaves", "purgesaves: deletes obsolete and unreadable save files", osdcmd_purgesaves);

    //OSD_RegisterFunction("quicksave","quicksave: performs a quick save", osdcmd_quicksave);
    //OSD_RegisterFunction("quickload","quickload: performs a quick load", osdcmd_quickload);
    OSD_RegisterFunction("quit","quit: exits the game immediately", osdcmd_quit);
    OSD_RegisterFunction("exit","exit: exits the game immediately", osdcmd_quit);

    //OSD_RegisterFunction("restartmap", "restartmap: restarts the current map", osdcmd_restartmap);
    //OSD_RegisterFunction("restartsound","restartsound: reinitializes the sound system",osdcmd_restartsound);
    OSD_RegisterFunction("restartvid","restartvid: reinitializes the video mode",osdcmd_restartvid);
    OSD_RegisterFunction("screenshot","screenshot [format]: takes a screenshot.", osdcmd_screenshot);

    //OSD_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",osdcmd_spawn);

    OSD_RegisterFunction("unbind","unbind <key>: unbinds a key", osdcmd_unbind);
    OSD_RegisterFunction("unbindall","unbindall: unbinds all keys", osdcmd_unbindall);
    OSD_RegisterFunction("unbound", NULL, osdcmd_unbound);

    OSD_RegisterFunction("vidmode","vidmode <xdim> <ydim> <bpp> <fullscreen>: change the video mode",osdcmd_vidmode);
#ifdef USE_OPENGL
    baselayer_osdcmd_vidmode_func = osdcmd_vidmode;
#endif

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

