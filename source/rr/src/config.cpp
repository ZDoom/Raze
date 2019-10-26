//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

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

#include "duke3d.h"
#include "scriplib.h"
#include "osdcmds.h"
#include "renderlayer.h"
#include "cmdline.h"

#if defined RENDERTYPESDL && defined SDL_TARGET && SDL_TARGET > 1
# include "sdl_inc.h"
#endif

// we load this in to get default button and key assignments
// as well as setting up function mappings

#define __SETUP__   // JBF 20031211
#include "_functio.h"

BEGIN_RR_NS






void CONFIG_SetDefaults(void)
{
    // JBF 20031211
    int32_t i;

    ud.config.scripthandle = -1;
#ifdef __ANDROID__
    droidinput.forward_sens = 5.f;
    droidinput.strafe_sens = 5.f;
    droidinput.pitch_sens = 5.f;
    droidinput.yaw_sens = 5.f;
    droidinput.hideStick = 0;
    droidinput.gameControlsAlpha = 0.5;
    droidinput.toggleCrouch = 1;
    droidinput.quickSelectWeapon = 1;

    ud.setup.xdim = droidinfo.screen_width;
    ud.setup.ydim = droidinfo.screen_height;
#else
# if defined RENDERTYPESDL && SDL_MAJOR_VERSION > 1
    uint32_t inited = SDL_WasInit(SDL_INIT_VIDEO);
    if (inited == 0)
        SDL_Init(SDL_INIT_VIDEO);
    else if (!(inited & SDL_INIT_VIDEO))
        SDL_InitSubSystem(SDL_INIT_VIDEO);

    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) == 0)
    {
        ud.setup.xdim = dm.w;
        ud.setup.ydim = dm.h;
    }
    else
# endif
    {
        ud.setup.xdim = 1024;
        ud.setup.ydim = 768;
    }
#endif

#ifdef USE_OPENGL
    ud.setup.bpp = 32;
#else
    ud.setup.bpp = 8;
#endif
    g_player[0].ps->aim_mode = 1;
    ud.config.ShowOpponentWeapons = 0;
    ud.althud = 1;
    ud.automsg = 0;
    ud.brightness = 8;
    ud.camerasprite = -1;

	ud.camera_time = 0;//4;

    ud.color = 0;
    ud.detail = 0;
    ud.lockout = 0;
    ud.m_ffire = 1;
    ud.m_marker = 1;
    ud.menu_slidebarz = 65536;
    ud.menu_slidebarmargin = RR ? 6 * 65536 : 65536;
    ud.menu_slidecursorz = RR ? 32768 : 65536;
    ud.pwlockout[0] = '\0';
    ud.runkey_mode = 0;
    ud.screen_size = 4;
	ud.screen_tilting = 1;
    ud.statusbarflags = STATUSBAR_NOSHRINK;
    ud.statusbarmode = 1;
    ud.statusbarscale = 100;
    ud.team = 0;
    ud.angleinterpolation = 0;

    ud.setup.forcesetup = 1;
    ud.setup.noautoload = 1;
    ud.setup.fullscreen = 1;

    ud.display_bonus_screen = 1;
    ud.show_level_text = 1;
    ud.configversion = 0;
    ud.screenfade = 1;
    ud.menubackground = 1;
    ud.default_skill = 1;
    ud.slidebar_paldisabled = 1;
    ud.shadow_pal = 4;
    ud.menu_scrollbartilenum = -1;
    ud.menu_scrollbarz = 65536;
    ud.menu_scrollcursorz = 65536;

    ud.config.CheckForUpdates = 1;

    Bstrcpy(ud.rtsname, G_DefaultRtsFile());

    Bstrcpy(szPlayerName, "Player");

    //if (RR)
    //{
    //    Bstrcpy(ud.ridecule[0], "Yer as ugly as a mud fence!");
    //    Bstrcpy(ud.ridecule[1], "Duck you pecker-head!");
    //    Bstrcpy(ud.ridecule[2], "You like that boy?");
    //    Bstrcpy(ud.ridecule[3], "Yer lower than catfish crap!");
    //    Bstrcpy(ud.ridecule[4], "Eat lead, you shit monkey!");
    //    Bstrcpy(ud.ridecule[5], "You dumb-ass!");
    //    Bstrcpy(ud.ridecule[6], "Yer slower'n a three legged dog!");
    //    Bstrcpy(ud.ridecule[7], "Come on...Squeal like a pig!");
    //    Bstrcpy(ud.ridecule[8], "Haw, haw, haw!");
    //    Bstrcpy(ud.ridecule[9], "Now you gone and done it!");
    //}
    //else
    //{
        Bstrcpy(ud.ridecule[0], "An inspiration for birth control.");
        Bstrcpy(ud.ridecule[1], "You're gonna die for that!");
        Bstrcpy(ud.ridecule[2], "It hurts to be you.");
        Bstrcpy(ud.ridecule[3], "Lucky son of a bitch.");
        Bstrcpy(ud.ridecule[4], "Hmmm... payback time.");
        Bstrcpy(ud.ridecule[5], "You bottom dwelling scum sucker.");
        Bstrcpy(ud.ridecule[6], "Damn, you're ugly.");
        Bstrcpy(ud.ridecule[7], "Ha ha ha... wasted!");
        Bstrcpy(ud.ridecule[8], "You suck!");
        Bstrcpy(ud.ridecule[9], "AARRRGHHHHH!!!");
    //}

    // JBF 20031211

    memset(MouseFunctions, -1, sizeof(MouseFunctions));
    for (i=0; i<MAXMOUSEBUTTONS; i++)
    {
        MouseFunctions[i][0] = CONFIG_FunctionNameToNum(mousedefaults[i]);
        CONTROL_MapButton(MouseFunctions[i][0], i, 0, controldevice_mouse);
        if (i>=4) continue;
        MouseFunctions[i][1] = CONFIG_FunctionNameToNum(mouseclickeddefaults[i]);
        CONTROL_MapButton(MouseFunctions[i][1], i, 1, controldevice_mouse);
    }

    memset(MouseDigitalFunctions, -1, sizeof(MouseDigitalFunctions));
    for (i=0; i<MAXMOUSEAXES; i++)
    {
        MouseAnalogueScale[i] = DEFAULTMOUSEANALOGUESCALE;
        CONTROL_SetAnalogAxisScale(i, MouseAnalogueScale[i], controldevice_mouse);

        MouseDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(mousedigitaldefaults[i*2]);
        MouseDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(mousedigitaldefaults[i*2+1]);
        CONTROL_MapDigitalAxis(i, MouseDigitalFunctions[i][0], 0, controldevice_mouse);
        CONTROL_MapDigitalAxis(i, MouseDigitalFunctions[i][1], 1, controldevice_mouse);

        MouseAnalogueAxes[i] = CONFIG_AnalogNameToNum(mouseanalogdefaults[i]);
        CONTROL_MapAnalogAxis(i, MouseAnalogueAxes[i], controldevice_mouse);
    }
    memset(JoystickFunctions, -1, sizeof(JoystickFunctions));
    for (i=0; i<MAXJOYBUTTONSANDHATS; i++)
    {
        JoystickFunctions[i][0] = CONFIG_FunctionNameToNum(joystickdefaults[i]);
        JoystickFunctions[i][1] = CONFIG_FunctionNameToNum(joystickclickeddefaults[i]);
        CONTROL_MapButton(JoystickFunctions[i][0], i, 0, controldevice_joystick);
        CONTROL_MapButton(JoystickFunctions[i][1], i, 1, controldevice_joystick);
    }

    memset(JoystickDigitalFunctions, -1, sizeof(JoystickDigitalFunctions));
    for (i=0; i<MAXJOYAXES; i++)
    {
        JoystickAnalogueScale[i] = DEFAULTJOYSTICKANALOGUESCALE;
        JoystickAnalogueDead[i] = DEFAULTJOYSTICKANALOGUEDEAD;
        JoystickAnalogueSaturate[i] = DEFAULTJOYSTICKANALOGUESATURATE;
        CONTROL_SetAnalogAxisScale(i, JoystickAnalogueScale[i], controldevice_joystick);

        JoystickDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(joystickdigitaldefaults[i*2]);
        JoystickDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(joystickdigitaldefaults[i*2+1]);
        CONTROL_MapDigitalAxis(i, JoystickDigitalFunctions[i][0], 0, controldevice_joystick);
        CONTROL_MapDigitalAxis(i, JoystickDigitalFunctions[i][1], 1, controldevice_joystick);

        JoystickAnalogueAxes[i] = CONFIG_AnalogNameToNum(joystickanalogdefaults[i]);
        CONTROL_MapAnalogAxis(i, JoystickAnalogueAxes[i], controldevice_joystick);
    }
}

void CONFIG_SetupMouse(void)
{
    int32_t i;
    char str[80];
    char temp[80];
    int32_t scale;

    if (ud.config.scripthandle < 0) return;

    for (i=0; i<MAXMOUSEBUTTONS; i++)
    {
        Bsprintf(str,"MouseButton%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle,"Controls", str,temp))
            MouseFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseButtonClicked%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle,"Controls", str,temp))
            MouseFunctions[i][1] = CONFIG_FunctionNameToNum(temp);
    }

    // map over the axes
    for (i=0; i<MAXMOUSEAXES; i++)
    {
        Bsprintf(str,"MouseAnalogAxes%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_AnalogNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                MouseAnalogueAxes[i] = CONFIG_AnalogNameToNum(temp);

        Bsprintf(str,"MouseDigitalAxes%d_0",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                MouseDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseDigitalAxes%d_1",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                MouseDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseAnalogScale%d",i);
        scale = MouseAnalogueScale[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        MouseAnalogueScale[i] = scale;
    }

    {
        tempbuf[0] = 0;
        SCRIPT_GetString(ud.config.scripthandle, "Controls","Mouse_Sensitivity",&tempbuf[0]);
        if (tempbuf[0]) in_mousesensitivity = atof(tempbuf);
    }

    for (i=0; i<MAXMOUSEBUTTONS; i++)
    {
        CONTROL_MapButton(MouseFunctions[i][0], i, 0, controldevice_mouse);
        CONTROL_MapButton(MouseFunctions[i][1], i, 1,  controldevice_mouse);
    }
    for (i=0; i<MAXMOUSEAXES; i++)
    {
        CONTROL_MapAnalogAxis(i, MouseAnalogueAxes[i], controldevice_mouse);
        CONTROL_MapDigitalAxis(i, MouseDigitalFunctions[i][0], 0,controldevice_mouse);
        CONTROL_MapDigitalAxis(i, MouseDigitalFunctions[i][1], 1,controldevice_mouse);
        CONTROL_SetAnalogAxisScale(i, MouseAnalogueScale[i], controldevice_mouse);
    }
}


void CONFIG_SetupJoystick(void)
{
    int32_t i;
    char str[80];
    char temp[80];
    int32_t scale;

    if (ud.config.scripthandle < 0) return;

    for (i=0; i<MAXJOYBUTTONSANDHATS; i++)
    {
        Bsprintf(str,"JoystickButton%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle,"Controls", str,temp))
            JoystickFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickButtonClicked%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle,"Controls", str,temp))
            JoystickFunctions[i][1] = CONFIG_FunctionNameToNum(temp);
    }

    // map over the axes
    for (i=0; i<MAXJOYAXES; i++)
    {
        Bsprintf(str,"JoystickAnalogAxes%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_AnalogNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                JoystickAnalogueAxes[i] = CONFIG_AnalogNameToNum(temp);

        Bsprintf(str,"JoystickDigitalAxes%d_0",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                JoystickDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickDigitalAxes%d_1",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                JoystickDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickAnalogScale%d",i);
        scale = JoystickAnalogueScale[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        JoystickAnalogueScale[i] = scale;

        Bsprintf(str,"JoystickAnalogDead%d",i);
        scale = JoystickAnalogueDead[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        JoystickAnalogueDead[i] = scale;

        Bsprintf(str,"JoystickAnalogSaturate%d",i);
        scale = JoystickAnalogueSaturate[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        JoystickAnalogueSaturate[i] = scale;
    }

    for (i=0; i<MAXJOYBUTTONSANDHATS; i++)
    {
        CONTROL_MapButton(JoystickFunctions[i][0], i, 0, controldevice_joystick);
        CONTROL_MapButton(JoystickFunctions[i][1], i, 1,  controldevice_joystick);
    }
    for (i=0; i<MAXJOYAXES; i++)
    {
        CONTROL_MapAnalogAxis(i, JoystickAnalogueAxes[i], controldevice_joystick);
        CONTROL_MapDigitalAxis(i, JoystickDigitalFunctions[i][0], 0, controldevice_joystick);
        CONTROL_MapDigitalAxis(i, JoystickDigitalFunctions[i][1], 1, controldevice_joystick);
        CONTROL_SetAnalogAxisScale(i, JoystickAnalogueScale[i], controldevice_joystick);
    }
}


int32_t CONFIG_ReadSetup(void)
{
    int32_t dummy;
    char commmacro[] = "CommbatMacro# ";
    char tempbuf[1024];

    CONFIG_SetDefaults();

    ud.config.setupread = 1;
    pathsearchmode = 1;

    if (ud.config.scripthandle < 0)
    {
        if (buildvfs_exists(g_setupFileName))  // JBF 20031211
            ud.config.scripthandle = SCRIPT_Load(g_setupFileName);
#if !defined(EDUKE32_TOUCH_DEVICES)
        else if (buildvfs_exists(SETUPFILENAME))
        {
            int32_t i;
            i=wm_ynbox("Import Configuration Settings", "The configuration file \"%s\" was not found. "
                "Import configuration data from \"%s\"?", g_setupFileName, SETUPFILENAME);
            if (i) ud.config.scripthandle = SCRIPT_Load(SETUPFILENAME);
        }
        else if (buildvfs_exists("duke3d.cfg"))
        {
            int32_t i;
            i=wm_ynbox("Import Configuration Settings", "The configuration file \"%s\" was not found. "
                "Import configuration data from \"duke3d.cfg\"?", g_setupFileName);
            if (i) ud.config.scripthandle = SCRIPT_Load("duke3d.cfg");
        }
#endif
    }

    pathsearchmode = 0;

    if (ud.config.scripthandle < 0)
        return -1;

    for (dummy = 0; dummy < MAXRIDECULE; dummy++)
    {
        commmacro[13] = dummy+'0';
        SCRIPT_GetString(ud.config.scripthandle, "Comm Setup",commmacro,&ud.ridecule[dummy][0]);
    }

    Bmemset(tempbuf, 0, sizeof(tempbuf));
    SCRIPT_GetString(ud.config.scripthandle, "Comm Setup","PlayerName",&tempbuf[0]);

    char nameBuf[64];

    while (Bstrlen(OSD_StripColors(nameBuf, tempbuf)) > 10)
        tempbuf[Bstrlen(tempbuf) - 1] = '\0';

    Bstrncpyz(szPlayerName, tempbuf, sizeof(szPlayerName));

    SCRIPT_GetString(ud.config.scripthandle, "Comm Setup","RTSName",&ud.rtsname[0]);

    SCRIPT_GetNumber(ud.config.scripthandle, "Setup", "ConfigVersion", &ud.configversion);
    SCRIPT_GetNumber(ud.config.scripthandle, "Setup", "ForceSetup", &ud.setup.forcesetup);
    SCRIPT_GetNumber(ud.config.scripthandle, "Setup", "NoAutoLoad", &ud.setup.noautoload);
    SCRIPT_GetNumber(ud.config.scripthandle, "Setup", "CacheSize", &dummy);

    if (g_noSetup == 0 && g_modDir[0] == '/')
    {
        struct Bstat st;
        SCRIPT_GetString(ud.config.scripthandle, "Setup","ModDir",&g_modDir[0]);

        if (Bstat(g_modDir, &st))
        {
            if ((st.st_mode & S_IFDIR) != S_IFDIR)
            {
                initprintf("Invalid mod dir in cfg!\n");
                Bsprintf(g_modDir,"/");
            }
        }
    }

    if (g_grpNamePtr == NULL && g_addonNum == 0)
    {
        SCRIPT_GetStringPtr(ud.config.scripthandle, "Setup","SelectedGRP",&g_grpNamePtr);
        if (g_grpNamePtr && !Bstrlen(g_grpNamePtr))
            g_grpNamePtr = dup_filename(G_DefaultGrpFile());
    }

    SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "Out",&ud.lockout);
    SCRIPT_GetString(ud.config.scripthandle, "Screen Setup","Password",&ud.pwlockout[0]);
    
    SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "ScreenHeight", &ud.setup.ydim);
    SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "ScreenMode", &ud.setup.fullscreen);
    SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "ScreenWidth", &ud.setup.xdim);

    SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "WindowPositioning", (int32_t *)&windowpos);

    windowx = -1;
    windowy = -1;
    SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "WindowPosX", (int32_t *)&windowx);
    SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "WindowPosY", (int32_t *)&windowy);

    SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "MaxRefreshFreq", (int32_t *)&maxrefreshfreq);
    SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "ScreenBPP", &ud.setup.bpp);
    
    if (ud.setup.bpp < 8) ud.setup.bpp = 32;

#ifdef POLYMER
    int32_t rendmode = 0;
    SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "Polymer", &rendmode);
    glrendmode = (rendmode > 0) ? REND_POLYMER : REND_POLYMOST;
#endif

    SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "Executions",&ud.executions);

#ifdef _WIN32
    SCRIPT_GetNumber(ud.config.scripthandle, "Updates", "CheckForUpdates", &ud.config.CheckForUpdates);
    SCRIPT_GetNumber(ud.config.scripthandle, "Updates", "LastUpdateCheck", &ud.config.LastUpdateCheck);
#endif

    ud.config.setupread = 1;
    return 0;
}


void CONFIG_WriteSettings(void) // save binds and aliases to <cfgname>_settings.cfg
{
}

void CONFIG_WriteSetup(uint32_t flags)
{
#if 0
    int32_t dummy;

    if (!ud.config.setupread) return;

    if (ud.config.scripthandle < 0)
        ud.config.scripthandle = SCRIPT_Init(g_setupFileName);

    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "Executions",ud.executions,FALSE,FALSE);

    SCRIPT_PutNumber(ud.config.scripthandle, "Setup","ConfigVersion",BYTEVERSION_EDUKE32,FALSE,FALSE);
    SCRIPT_PutNumber(ud.config.scripthandle, "Setup", "ForceSetup", ud.setup.forcesetup, FALSE, FALSE);
    SCRIPT_PutNumber(ud.config.scripthandle, "Setup", "NoAutoLoad", ud.setup.noautoload, FALSE, FALSE);
    SCRIPT_PutNumber(ud.config.scripthandle, "Setup", "CacheSize", MAXCACHE1DSIZE, FALSE, FALSE);

#ifdef POLYMER
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "Polymer",glrendmode == REND_POLYMER,FALSE,FALSE);
#endif
    
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "ScreenBPP", ud.setup.bpp, FALSE, FALSE);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "ScreenHeight", ud.setup.ydim, FALSE, FALSE);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "ScreenMode", ud.setup.fullscreen, FALSE, FALSE);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "ScreenWidth", ud.setup.xdim, FALSE, FALSE);

    if (g_grpNamePtr && !g_addonNum)
        SCRIPT_PutString(ud.config.scripthandle, "Setup","SelectedGRP",g_grpNamePtr);

#ifdef STARTUP_SETUP_WINDOW
    if (g_noSetup == 0)
        SCRIPT_PutString(ud.config.scripthandle, "Setup","ModDir",&g_modDir[0]);
#endif
    // exit early after only updating the values that can be changed from the startup window
    if (flags & 1)
    {
        SCRIPT_Save(ud.config.scripthandle, g_setupFileName);
        SCRIPT_Free(ud.config.scripthandle);
        return;
    }

    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "WindowPositioning", windowpos, FALSE, FALSE);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "WindowPosX", windowx, FALSE, FALSE);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "WindowPosY", windowy, FALSE, FALSE);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "MaxRefreshFreq", maxrefreshfreq, FALSE, FALSE);

    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "Out",ud.lockout,FALSE,FALSE);
    SCRIPT_PutString(ud.config.scripthandle, "Screen Setup", "Password",ud.pwlockout);

#ifdef _WIN32
    SCRIPT_PutNumber(ud.config.scripthandle, "Updates", "CheckForUpdates", ud.config.CheckForUpdates, FALSE, FALSE);
    SCRIPT_PutNumber(ud.config.scripthandle, "Updates", "LastUpdateCheck", ud.config.LastUpdateCheck, FALSE, FALSE);
#endif

    if (in_mouse)
    {
        for (dummy=0; dummy<MAXMOUSEBUTTONS; dummy++)
        {
            if (CONFIG_FunctionNumToName(MouseFunctions[dummy][0]))
            {
                Bsprintf(buf, "MouseButton%d", dummy);
                SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(MouseFunctions[dummy][0]));
            }

            if (dummy >= (MAXMOUSEBUTTONS-2)) continue;

            if (CONFIG_FunctionNumToName(MouseFunctions[dummy][1]))
            {
                Bsprintf(buf, "MouseButtonClicked%d", dummy);
                SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(MouseFunctions[dummy][1]));
            }
        }

        for (dummy=0; dummy<MAXMOUSEAXES; dummy++)
        {
            if (CONFIG_AnalogNumToName(MouseAnalogueAxes[dummy]))
            {
                Bsprintf(buf, "MouseAnalogAxes%d", dummy);
                SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_AnalogNumToName(MouseAnalogueAxes[dummy]));
            }

            if (CONFIG_FunctionNumToName(MouseDigitalFunctions[dummy][0]))
            {
                Bsprintf(buf, "MouseDigitalAxes%d_0", dummy);
                SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(MouseDigitalFunctions[dummy][0]));
            }

            if (CONFIG_FunctionNumToName(MouseDigitalFunctions[dummy][1]))
            {
                Bsprintf(buf, "MouseDigitalAxes%d_1", dummy);
                SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(MouseDigitalFunctions[dummy][1]));
            }

            if (MouseAnalogueScale[dummy] != DEFAULTMOUSEANALOGUESCALE)
            {
                Bsprintf(buf, "MouseAnalogScale%d", dummy);
                SCRIPT_PutNumber(ud.config.scripthandle, "Controls", buf, MouseAnalogueScale[dummy], FALSE, FALSE);
            }
        }
    }

    if (in_joystick)
    {
        for (dummy=0; dummy<MAXJOYBUTTONSANDHATS; dummy++)
        {
            if (CONFIG_FunctionNumToName(JoystickFunctions[dummy][0]))
            {
                Bsprintf(buf, "JoystickButton%d", dummy);
                SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickFunctions[dummy][0]));
            }

            if (CONFIG_FunctionNumToName(JoystickFunctions[dummy][1]))
            {
                Bsprintf(buf, "JoystickButtonClicked%d", dummy);
                SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickFunctions[dummy][1]));
            }
        }
        for (dummy=0; dummy<MAXJOYAXES; dummy++)
        {
            if (CONFIG_AnalogNumToName(JoystickAnalogueAxes[dummy]))
            {
                Bsprintf(buf, "JoystickAnalogAxes%d", dummy);
                SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_AnalogNumToName(JoystickAnalogueAxes[dummy]));
            }

            if (CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][0]))
            {
                Bsprintf(buf, "JoystickDigitalAxes%d_0", dummy);
                SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][0]));
            }

            if (CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][1]))
            {
                Bsprintf(buf, "JoystickDigitalAxes%d_1", dummy);
                SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][1]));
            }

            if (JoystickAnalogueScale[dummy] != DEFAULTJOYSTICKANALOGUESCALE)
            {
                Bsprintf(buf, "JoystickAnalogScale%d", dummy);
                SCRIPT_PutNumber(ud.config.scripthandle, "Controls", buf, JoystickAnalogueScale[dummy], FALSE, FALSE);
            }

            if (JoystickAnalogueDead[dummy] != DEFAULTJOYSTICKANALOGUEDEAD)
            {
                Bsprintf(buf, "JoystickAnalogDead%d", dummy);
                SCRIPT_PutNumber(ud.config.scripthandle, "Controls", buf, JoystickAnalogueDead[dummy], FALSE, FALSE);
            }

            if (JoystickAnalogueSaturate[dummy] != DEFAULTJOYSTICKANALOGUESATURATE)
            {
                Bsprintf(buf, "JoystickAnalogSaturate%d", dummy);
                SCRIPT_PutNumber(ud.config.scripthandle, "Controls", buf, JoystickAnalogueSaturate[dummy], FALSE, FALSE);
            }
        }
    }

    SCRIPT_PutString(ud.config.scripthandle, "Comm Setup","PlayerName",&szPlayerName[0]);

    SCRIPT_PutString(ud.config.scripthandle, "Comm Setup","RTSName",&ud.rtsname[0]);

    char commmacro[] = "CommbatMacro# ";

    for (dummy = 0; dummy < MAXRIDECULE; dummy++)
    {
        commmacro[13] = dummy+'0';
        SCRIPT_PutString(ud.config.scripthandle, "Comm Setup",commmacro,&ud.ridecule[dummy][0]);
    }

    SCRIPT_Save(ud.config.scripthandle, g_setupFileName);

    if ((flags & 2) == 0)
        SCRIPT_Free(ud.config.scripthandle);

    OSD_Printf("Wrote %s\n",g_setupFileName);
    CONFIG_WriteSettings();
	Bfflush(NULL);
#endif
}

static const char *CONFIG_GetMapEntryName(char m[], char const * const mapname)
{
    strcpy(m, mapname);

    char *p = strrchr(m, '/');
    if (!p) p = strrchr(m, '\\');
    if (p) Bmemmove(m, p, Bstrlen(p)+1);
    for (p=m; *p; p++) *p = tolower(*p);

    // cheap hack because SCRIPT_GetNumber doesn't like the slashes
    p = m;
    while (*p == '/') p++;

    return p;
}

static void CONFIG_GetMD4EntryName(char m[], uint8_t const * const md4)
{
    sprintf(m, "MD4_%08x%08x%08x%08x",
            B_BIG32(B_UNBUF32(&md4[0])), B_BIG32(B_UNBUF32(&md4[4])),
            B_BIG32(B_UNBUF32(&md4[8])), B_BIG32(B_UNBUF32(&md4[12])));
}

int32_t CONFIG_GetMapBestTime(char const * const mapname, uint8_t const * const mapmd4)
{
    if (!ud.config.setupread || ud.config.scripthandle < 0)
        return -1;

    char m[37];
    CONFIG_GetMD4EntryName(m, mapmd4);

    int32_t t = -1;
    if (SCRIPT_GetNumber(ud.config.scripthandle, "MapTimes", m, &t))
    {
        // fall back to map filenames
        char m2[BMAX_PATH];
        char const * const p = CONFIG_GetMapEntryName(m2, mapname);
        SCRIPT_GetNumber(ud.config.scripthandle, "MapTimes", p, &t);
    }
    return t;
}

int32_t CONFIG_SetMapBestTime(uint8_t const * const mapmd4, int32_t const tm)
{
    if (ud.config.scripthandle < 0) ud.config.scripthandle = SCRIPT_Init(g_setupFileName);
    if (ud.config.scripthandle < 0) return -1;

    char m[37];
    CONFIG_GetMD4EntryName(m, mapmd4);

    SCRIPT_PutNumber(ud.config.scripthandle, "MapTimes", m, tm, FALSE, FALSE);
    return 0;
}

END_RR_NS
