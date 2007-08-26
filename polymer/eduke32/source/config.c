//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2004, 2007 - EDuke32 developers

This file is part of EDuke32

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

/*
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
*/

#include "duke3d.h"
#include "scriplib.h"
#include "osd.h"

#include "baselayer.h"

// we load this in to get default button and key assignments
// as well as setting up function mappings

#define __SETUP__   // JBF 20031211
#include "_functio.h"

config_t config;

/*
===================
=
= CONFIG_FunctionNameToNum
=
===================
*/

int32 CONFIG_FunctionNameToNum(char * func)
{
    int32 i;

    for (i=0;i<NUMGAMEFUNCTIONS;i++)
    {
        if (!Bstrcasecmp(func,gamefunctions[i]))
        {
            return i;
        }
    }
    for (i=0;i<NUMGAMEFUNCTIONS;i++)
    {
        if (!Bstrcasecmp(func,defaultgamefunctions[i]))
        {
            return i;
        }
    }
    return -1;
}

/*
===================
=
= CONFIG_FunctionNumToName
=
===================
*/

char * CONFIG_FunctionNumToName(int32 func)
{
    if ((unsigned)func >= (unsigned)NUMGAMEFUNCTIONS)
        return NULL;
    return gamefunctions[func];
}

/*
===================
=
= CONFIG_AnalogNameToNum
=
===================
*/


int32 CONFIG_AnalogNameToNum(char * func)
{

    if (!Bstrcasecmp(func,"analog_turning"))
    {
        return analog_turning;
    }
    if (!Bstrcasecmp(func,"analog_strafing"))
    {
        return analog_strafing;
    }
    if (!Bstrcasecmp(func,"analog_moving"))
    {
        return analog_moving;
    }
    if (!Bstrcasecmp(func,"analog_lookingupanddown"))
    {
        return analog_lookingupanddown;
    }

    return -1;
}


char * CONFIG_AnalogNumToName(int32 func)
{
    switch (func)
    {
    case analog_turning:
        return "analog_turning";
    case analog_strafing:
        return "analog_strafing";
    case analog_moving:
        return "analog_moving";
    case analog_lookingupanddown:
        return "analog_lookingupanddown";
    }

    return NULL;
}


/*
===================
=
= CONFIG_SetDefaults
=
===================
*/

void CONFIG_SetDefaultKeys(int type)
{
    int32 i,f;

    memset(config.KeyboardKeys, 0xff, sizeof(config.KeyboardKeys));

    if (type == 1)
    {
        for (i=0; i < (int32)(sizeof(oldkeydefaults)/sizeof(oldkeydefaults[0])); i+=3)
        {
            f = CONFIG_FunctionNameToNum((char *)oldkeydefaults[i+0]);
            if (f == -1) continue;
            config.KeyboardKeys[f][0] = KB_StringToScanCode((char *)oldkeydefaults[i+1]);
            config.KeyboardKeys[f][1] = KB_StringToScanCode((char *)oldkeydefaults[i+2]);

            if (f == gamefunc_Show_Console) OSD_CaptureKey(config.KeyboardKeys[f][0]);
            else CONTROL_MapKey(f, config.KeyboardKeys[f][0], config.KeyboardKeys[f][1]);
        }
        return;
    }

    for (i=0; i < (int32)(sizeof(keydefaults)/sizeof(keydefaults[0])); i+=3)
    {
        f = CONFIG_FunctionNameToNum(keydefaults[i+0]);
        if (f == -1) continue;
        config.KeyboardKeys[f][0] = KB_StringToScanCode(keydefaults[i+1]);
        config.KeyboardKeys[f][1] = KB_StringToScanCode(keydefaults[i+2]);

        if (f == gamefunc_Show_Console) OSD_CaptureKey(config.KeyboardKeys[f][0]);
        else CONTROL_MapKey(f, config.KeyboardKeys[f][0], config.KeyboardKeys[f][1]);
    }
}

void CONFIG_SetDefaults(void)
{
    // JBF 20031211
    int32 i,f;

    config.scripthandle = -1;
    config.useprecache = 1;
    config.ForceSetup = 1;
    config.AmbienceToggle = 1;
    config.AutoAim = 1;
    config.FXDevice = 0;
    config.FXVolume = 220;
    config.MixRate = 44100;
    config.MouseBias = 0;
    config.MouseFilter = 0;
    config.MusicDevice = 0;
    config.MusicToggle = 1;
    config.MusicVolume = 200;
    myaimmode = ps[0].aim_mode = 1;
    config.NumBits = 16;
    config.NumChannels = 2;
    config.NumVoices = 32;
    config.ReverseStereo = 0;
    config.RunMode = ud.auto_run = 1;
    config.ShowOpponentWeapons = 0;
    config.SmoothInput = 1;
    config.SoundToggle = 1;
    ud.automsg = 0;
    ud.autovote = 0;
    ud.brightness = 8;
    ud.color = 0;
    ud.crosshair = 2;
    ud.deathmsgs = 1;
    ud.democams = 1;
    ud.detail = 1;
    ud.drawweapon = 1;
    ud.idplayers = 1;
    ud.levelstats = 0;
    ud.lockout = 0;
    ud.m_ffire = 1;
    ud.m_marker = 1;
    ud.mouseaiming = 0;
    ud.mouseflip = 1;
    ud.msgdisptime = 120;
    ud.pwlockout[0] = '\0';
    ud.runkey_mode = 0;
    ud.screen_size = 4;
    ud.screen_tilting = 1;
    ud.shadows = 1;
    ud.statusbarmode = 0;
    ud.statusbarscale = 100;
    ud.team = 0;
    ud.viewbob = 1;
    ud.weaponsway = 1;
    ud.weaponswitch = 3;	// new+empty
    ud.angleinterpolation = 0;
    config.UseJoystick = 0;
    config.UseMouse = 1;
    config.VoiceToggle = 2;

    Bstrcpy(ud.rtsname, "DUKE.RTS");
    Bstrcpy(myname, "Duke");

    Bstrcpy(ud.ridecule[0], "An inspiration for birth control.");
    Bstrcpy(ud.ridecule[1], "You're gonna die for that!");
    Bstrcpy(ud.ridecule[2], "It hurts to be you.");
    Bstrcpy(ud.ridecule[3], "Lucky Son of a Bitch.");
    Bstrcpy(ud.ridecule[4], "Hmmm....Payback time.");
    Bstrcpy(ud.ridecule[5], "You bottom dwelling scum sucker.");
    Bstrcpy(ud.ridecule[6], "Damn, you're ugly.");
    Bstrcpy(ud.ridecule[7], "Ha ha ha...Wasted!");
    Bstrcpy(ud.ridecule[8], "You suck!");
    Bstrcpy(ud.ridecule[9], "AARRRGHHHHH!!!");

    // JBF 20031211

    CONFIG_SetDefaultKeys(0);

    memset(config.MouseFunctions, -1, sizeof(config.MouseFunctions));
    for (i=0; i<MAXMOUSEBUTTONS; i++)
    {
        config.MouseFunctions[i][0] = CONFIG_FunctionNameToNum(mousedefaults[i]);
        CONTROL_MapButton(config.MouseFunctions[i][0], i, 0, controldevice_mouse);
        if (i>=4) continue;
        config.MouseFunctions[i][1] = CONFIG_FunctionNameToNum(mouseclickeddefaults[i]);
        CONTROL_MapButton(config.MouseFunctions[i][1], i, 1, controldevice_mouse);
    }

    memset(config.MouseDigitalFunctions, -1, sizeof(config.MouseDigitalFunctions));
    for (i=0; i<MAXMOUSEAXES; i++)
    {
        config.MouseAnalogueScale[i] = 65536;
        CONTROL_SetAnalogAxisScale(i, config.MouseAnalogueScale[i], controldevice_mouse);

        config.MouseDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(mousedigitaldefaults[i*2]);
        config.MouseDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(mousedigitaldefaults[i*2+1]);
        CONTROL_MapDigitalAxis(i, config.MouseDigitalFunctions[i][0], 0, controldevice_mouse);
        CONTROL_MapDigitalAxis(i, config.MouseDigitalFunctions[i][1], 1, controldevice_mouse);

        config.MouseAnalogueAxes[i] = CONFIG_AnalogNameToNum(mouseanalogdefaults[i]);
        CONTROL_MapAnalogAxis(i, config.MouseAnalogueAxes[i], controldevice_mouse);
    }
    CONTROL_SetMouseSensitivity(DEFAULTMOUSESENSITIVITY);

    memset(config.JoystickFunctions, -1, sizeof(config.JoystickFunctions));
    for (i=0; i<MAXJOYBUTTONS; i++)
    {
        config.JoystickFunctions[i][0] = CONFIG_FunctionNameToNum(joystickdefaults[i]);
        config.JoystickFunctions[i][1] = CONFIG_FunctionNameToNum(joystickclickeddefaults[i]);
        CONTROL_MapButton(config.JoystickFunctions[i][0], i, 0, controldevice_joystick);
        CONTROL_MapButton(config.JoystickFunctions[i][1], i, 1, controldevice_joystick);
    }

    memset(config.JoystickDigitalFunctions, -1, sizeof(config.JoystickDigitalFunctions));
    for (i=0; i<MAXJOYAXES; i++)
    {
        config.JoystickAnalogueScale[i] = 65536;
        config.JoystickAnalogueDead[i] = 1000;
        config.JoystickAnalogueSaturate[i] = 9500;
        CONTROL_SetAnalogAxisScale(i, config.JoystickAnalogueScale[i], controldevice_joystick);

        config.JoystickDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(joystickdigitaldefaults[i*2]);
        config.JoystickDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(joystickdigitaldefaults[i*2+1]);
        CONTROL_MapDigitalAxis(i, config.JoystickDigitalFunctions[i][0], 0, controldevice_joystick);
        CONTROL_MapDigitalAxis(i, config.JoystickDigitalFunctions[i][1], 1, controldevice_joystick);

        config.JoystickAnalogueAxes[i] = CONFIG_AnalogNameToNum(joystickanalogdefaults[i]);
        CONTROL_MapAnalogAxis(i, config.JoystickAnalogueAxes[i], controldevice_joystick);
    }
}
/*
===================
=
= CONFIG_ReadKeys
=
===================
*/

void CONFIG_ReadKeys(void)
{
    int32 i;
    int32 numkeyentries;
    int32 function;
    char keyname1[80];
    char keyname2[80];
    kb_scancode key1,key2;

    if (config.scripthandle < 0) return;

    numkeyentries = SCRIPT_NumberEntries(config.scripthandle,"KeyDefinitions");

    for (i=0;i<numkeyentries;i++)
    {
        function = CONFIG_FunctionNameToNum(SCRIPT_Entry(config.scripthandle,"KeyDefinitions", i));
        if (function != -1)
        {
            memset(keyname1,0,sizeof(keyname1));
            memset(keyname2,0,sizeof(keyname2));
            SCRIPT_GetDoubleString
            (
                config.scripthandle,
                "KeyDefinitions",
                SCRIPT_Entry(config.scripthandle, "KeyDefinitions", i),
                keyname1,
                keyname2
            );
            key1 = 0xff;
            key2 = 0xff;
            if (keyname1[0])
            {
                key1 = (byte) KB_StringToScanCode(keyname1);
            }
            if (keyname2[0])
            {
                key2 = (byte) KB_StringToScanCode(keyname2);
            }
            config.KeyboardKeys[function][0] = key1;
            config.KeyboardKeys[function][1] = key2;
        }
    }

    for (i=0; i<NUMGAMEFUNCTIONS; i++)
    {
        if (i == gamefunc_Show_Console)
            OSD_CaptureKey(config.KeyboardKeys[i][0]);
        else
            CONTROL_MapKey(i, config.KeyboardKeys[i][0], config.KeyboardKeys[i][1]);
    }
}


/*
===================
=
= CONFIG_SetupMouse
=
===================
*/

void CONFIG_SetupMouse(void)
{
    int32 i;
    char str[80];
    char temp[80];
    int32 function, scale;

    if (config.scripthandle < 0) return;

    for (i=0;i<MAXMOUSEBUTTONS;i++)
    {
        Bsprintf(str,"MouseButton%ld",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(config.scripthandle,"Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                config.MouseFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseButtonClicked%ld",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(config.scripthandle,"Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                config.MouseFunctions[i][1] = CONFIG_FunctionNameToNum(temp);
    }

    // map over the axes
    for (i=0;i<MAXMOUSEAXES;i++)
    {
        Bsprintf(str,"MouseAnalogAxes%ld",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(config.scripthandle, "Controls", str,temp))
            if (CONFIG_AnalogNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                config.MouseAnalogueAxes[i] = CONFIG_AnalogNameToNum(temp);

        Bsprintf(str,"MouseDigitalAxes%ld_0",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                config.MouseDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseDigitalAxes%ld_1",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                config.MouseDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseAnalogScale%ld",i);
        scale = config.MouseAnalogueScale[i];
        SCRIPT_GetNumber(config.scripthandle, "Controls", str,&scale);
        config.MouseAnalogueScale[i] = scale;
    }

    function = DEFAULTMOUSESENSITIVITY;
    SCRIPT_GetNumber(config.scripthandle, "Controls","Mouse_Sensitivity",&function);
    CONTROL_SetMouseSensitivity(function);

    for (i=0; i<MAXMOUSEBUTTONS; i++)
    {
        CONTROL_MapButton(config.MouseFunctions[i][0], i, 0, controldevice_mouse);
        CONTROL_MapButton(config.MouseFunctions[i][1], i, 1,  controldevice_mouse);
    }
    for (i=0; i<MAXMOUSEAXES; i++)
    {
        CONTROL_MapAnalogAxis(i, config.MouseAnalogueAxes[i], controldevice_mouse);
        CONTROL_MapDigitalAxis(i, config.MouseDigitalFunctions[i][0], 0,controldevice_mouse);
        CONTROL_MapDigitalAxis(i, config.MouseDigitalFunctions[i][1], 1,controldevice_mouse);
        CONTROL_SetAnalogAxisScale(i, config.MouseAnalogueScale[i], controldevice_mouse);
    }
}

/*
===================
=
= CONFIG_SetupJoystick
=
===================
*/

void CONFIG_SetupJoystick(void)
{
    int32 i;
    char str[80];
    char temp[80];
    int32 scale;

    if (config.scripthandle < 0) return;

    for (i=0;i<MAXJOYBUTTONS;i++)
    {
        Bsprintf(str,"JoystickButton%ld",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(config.scripthandle,"Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                config.JoystickFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickButtonClicked%ld",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(config.scripthandle,"Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                config.JoystickFunctions[i][1] = CONFIG_FunctionNameToNum(temp);
    }

    // map over the axes
    for (i=0;i<MAXJOYAXES;i++)
    {
        Bsprintf(str,"JoystickAnalogAxes%ld",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(config.scripthandle, "Controls", str,temp))
            if (CONFIG_AnalogNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                config.JoystickAnalogueAxes[i] = CONFIG_AnalogNameToNum(temp);

        Bsprintf(str,"JoystickDigitalAxes%ld_0",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                config.JoystickDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickDigitalAxes%ld_1",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                config.JoystickDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickAnalogScale%ld",i);
        scale = config.JoystickAnalogueScale[i];
        SCRIPT_GetNumber(config.scripthandle, "Controls", str,&scale);
        config.JoystickAnalogueScale[i] = scale;

        Bsprintf(str,"JoystickAnalogDead%ld",i);
        scale = config.JoystickAnalogueDead[i];
        SCRIPT_GetNumber(config.scripthandle, "Controls", str,&scale);
        config.JoystickAnalogueDead[i] = scale;

        Bsprintf(str,"JoystickAnalogSaturate%ld",i);
        scale = config.JoystickAnalogueSaturate[i];
        SCRIPT_GetNumber(config.scripthandle, "Controls", str,&scale);
        config.JoystickAnalogueSaturate[i] = scale;
    }

    for (i=0;i<MAXJOYBUTTONS;i++)
    {
        CONTROL_MapButton(config.JoystickFunctions[i][0], i, 0, controldevice_joystick);
        CONTROL_MapButton(config.JoystickFunctions[i][1], i, 1,  controldevice_joystick);
    }
    for (i=0;i<MAXJOYAXES;i++)
    {
        CONTROL_MapAnalogAxis(i, config.JoystickAnalogueAxes[i], controldevice_joystick);
        CONTROL_MapDigitalAxis(i, config.JoystickDigitalFunctions[i][0], 0, controldevice_joystick);
        CONTROL_MapDigitalAxis(i, config.JoystickDigitalFunctions[i][1], 1, controldevice_joystick);
        CONTROL_SetAnalogAxisScale(i, config.JoystickAnalogueScale[i], controldevice_joystick);
    }
}

/*
===================
=
= CONFIG_ReadSetup
=
===================
*/
extern char *duke3dgrp;
extern void check_player_color(int *color,int prev_color);

int32 CONFIG_ReadSetup(void)
{
    int32 dummy, i;
    char commmacro[] = "CommbatMacro# ";
    extern int32 CommandWeaponChoice;

    CONTROL_ClearAssignments();
    CONFIG_SetDefaults();

    config.setupread = 1;

    pathsearchmode = 1;
    if (SafeFileExists(setupfilename) && config.scripthandle < 0)  // JBF 20031211
        config.scripthandle = SCRIPT_Load(setupfilename);
    pathsearchmode = 0;

    if (config.scripthandle < 0) return -1;

    if (config.scripthandle >= 0)
    {
        for (dummy = 0;dummy < 10;dummy++)
        {
            commmacro[13] = dummy+'0';
            SCRIPT_GetString(config.scripthandle, "Comm Setup",commmacro,&ud.ridecule[dummy][0]);
        }

        SCRIPT_GetString(config.scripthandle, "Comm Setup","PlayerName",&tempbuf[0]);

        while (Bstrlen(stripcolorcodes(tempbuf)) > 10)
            tempbuf[Bstrlen(tempbuf)-1] = '\0';

        Bstrncpy(myname,tempbuf,sizeof(myname)-1);
        myname[sizeof(myname)] = '\0';

        SCRIPT_GetString(config.scripthandle, "Comm Setup","RTSName",&ud.rtsname[0]);

        SCRIPT_GetNumber(config.scripthandle, "Comm Setup", "Rate",(int32 *)&packetrate);
        packetrate = min(max(packetrate,50),1000);

        {
            extern char defaultduke3dgrp[BMAX_PATH];
            if (!Bstrcmp(defaultduke3dgrp,"duke3d.grp"))
                SCRIPT_GetString(config.scripthandle, "Misc","SelectedGRP",&duke3dgrp[0]);
        }

        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "Shadows",&ud.shadows);

        if (!NAM)
        {
            SCRIPT_GetString(config.scripthandle, "Screen Setup","Password",&ud.pwlockout[0]);
            SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "Out",&ud.lockout);
        }

        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "Detail",&ud.detail);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "Tilt",&ud.screen_tilting);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "Messages",&ud.fta_on);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "ScreenWidth",&config.ScreenWidth);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "ScreenHeight",&config.ScreenHeight);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "ScreenMode",&config.ScreenMode);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "ScreenGamma",&ud.brightness);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "ScreenSize",&ud.screen_size);

#if defined(POLYMOST) && defined(USE_OPENGL)
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "ScreenBPP", &config.ScreenBPP);
        if (config.ScreenBPP < 8) config.ScreenBPP = 32;
#endif

#ifdef RENDERTYPEWIN
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "MaxRefreshFreq", (int32*)&maxrefreshfreq);
#endif
#if defined(POLYMOST) && defined(USE_OPENGL)
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "GLTextureMode", &gltexfiltermode);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "GLAnisotropy", &glanisotropy);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "GLProjectionFix", &glprojectionhacks);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "GLUseTextureCompr", &glusetexcompr);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "GLWidescreen", &glwidescreen);

        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "GLUseCompressedTextureCache", &glusetexcache);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "GLUseTextureCacheCompression", &glusetexcachecompression);

        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "GLDepthPeeling", &r_depthpeeling);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "GLPeelsCount", &r_peelscount);

        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "GLDetailMapping", &r_detailmapping);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "GLGlowMapping", &r_glowmapping);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "GLVertexArrays", &r_vertexarrays);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "GLVBOs", &r_vbos);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "GLVBOCount", &r_vbocount);

        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "GLAnimationSmoothing", &r_animsmoothing);

        dummy = usemodels;
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "UseModels",&dummy);
        usemodels = dummy != 0;
        dummy = usehightile;
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "UseHightile",&dummy);
        usehightile = dummy != 0;
#endif
        SCRIPT_GetNumber(config.scripthandle, "Misc", "Executions",&ud.executions);
        SCRIPT_GetNumber(config.scripthandle, "Setup", "ForceSetup",&config.ForceSetup);
        SCRIPT_GetNumber(config.scripthandle, "Misc", "RunMode",&config.RunMode);
        SCRIPT_GetNumber(config.scripthandle, "Misc", "Crosshairs",&ud.crosshair);
        SCRIPT_GetNumber(config.scripthandle, "Misc", "StatusBarScale",&ud.statusbarscale);
        SCRIPT_GetNumber(config.scripthandle, "Misc", "ShowLevelStats",&ud.levelstats);
        SCRIPT_GetNumber(config.scripthandle, "Misc", "ShowOpponentWeapons",&config.ShowOpponentWeapons);
        ud.showweapons = config.ShowOpponentWeapons;
        SCRIPT_GetNumber(config.scripthandle, "Misc", "ShowViewWeapon",&ud.drawweapon);
        SCRIPT_GetNumber(config.scripthandle, "Misc", "DeathMessages",&ud.deathmsgs);
        SCRIPT_GetNumber(config.scripthandle, "Misc", "DemoCams",&ud.democams);
        SCRIPT_GetNumber(config.scripthandle, "Misc", "ShowFPS",&ud.tickrate);
        SCRIPT_GetNumber(config.scripthandle, "Misc", "Color",&ud.color);
        check_player_color((int *)&ud.color,-1);
        ps[0].palookup = ud.pcolor[0] = ud.color;
        SCRIPT_GetNumber(config.scripthandle, "Misc", "Team",&dummy);
        ud.team = 0;
        if (dummy < 4 && dummy > -1) ud.team = dummy;
        ud.pteam[0] = ud.team;
        SCRIPT_GetNumber(config.scripthandle, "Misc", "MPMessageDisplayTime",&ud.msgdisptime);
        SCRIPT_GetNumber(config.scripthandle, "Misc", "StatusBarMode",&ud.statusbarmode);
        SCRIPT_GetNumber(config.scripthandle, "Misc", "AutoVote",&ud.autovote);
        SCRIPT_GetNumber(config.scripthandle, "Misc", "AutoMsg",&ud.automsg);
        SCRIPT_GetNumber(config.scripthandle, "Misc", "IDPlayers",&ud.automsg);
        SCRIPT_GetNumber(config.scripthandle, "Misc", "ViewBobbing",&ud.viewbob);
        SCRIPT_GetNumber(config.scripthandle, "Misc", "WeaponSway",&ud.weaponsway);

        dummy = config.useprecache;
        SCRIPT_GetNumber(config.scripthandle, "Misc", "UsePrecache",&dummy);
        config.useprecache = dummy != 0;

        SCRIPT_GetNumber(config.scripthandle, "Misc","AngleInterpolation",&ud.angleinterpolation);

        // weapon choices are defaulted in checkcommandline, which may override them
        if (!CommandWeaponChoice)
            for (i=0;i<10;i++)
            {
                Bsprintf(buf,"WeaponChoice%ld",i);
                dummy = -1;
                SCRIPT_GetNumber(config.scripthandle, "Misc", buf, &dummy);
                if (dummy >= 0) ud.wchoice[0][i] = dummy;
            }

        SCRIPT_GetNumber(config.scripthandle, "Sound Setup", "FXDevice",&config.FXDevice);
        SCRIPT_GetNumber(config.scripthandle, "Sound Setup", "MusicDevice",&config.MusicDevice);
        SCRIPT_GetNumber(config.scripthandle, "Sound Setup", "FXVolume",&config.FXVolume);
        SCRIPT_GetNumber(config.scripthandle, "Sound Setup", "MusicVolume",&config.MusicVolume);
        SCRIPT_GetNumber(config.scripthandle, "Sound Setup", "SoundToggle",&config.SoundToggle);
        SCRIPT_GetNumber(config.scripthandle, "Sound Setup", "MusicToggle",&config.MusicToggle);
        SCRIPT_GetNumber(config.scripthandle, "Sound Setup", "VoiceToggle",&config.VoiceToggle);
        SCRIPT_GetNumber(config.scripthandle, "Sound Setup", "AmbienceToggle",&config.AmbienceToggle);
        SCRIPT_GetNumber(config.scripthandle, "Sound Setup", "NumVoices",&config.NumVoices);
        SCRIPT_GetNumber(config.scripthandle, "Sound Setup", "NumChannels",&config.NumChannels);
        SCRIPT_GetNumber(config.scripthandle, "Sound Setup", "NumBits",&config.NumBits);
        SCRIPT_GetNumber(config.scripthandle, "Sound Setup", "MixRate",&config.MixRate);
        SCRIPT_GetNumber(config.scripthandle, "Sound Setup", "ReverseStereo",&config.ReverseStereo);

        SCRIPT_GetNumber(config.scripthandle, "Controls","MouseAimingFlipped",&ud.mouseflip);  // mouse aiming inverted
        SCRIPT_GetNumber(config.scripthandle, "Controls","MouseAiming",&ud.mouseaiming);		// 1=momentary/0=toggle
        ps[0].aim_mode = ud.mouseaiming;
        SCRIPT_GetNumber(config.scripthandle, "Controls","MouseBias",&config.MouseBias);
        SCRIPT_GetNumber(config.scripthandle, "Controls","MouseFilter",&config.MouseFilter);
        SCRIPT_GetNumber(config.scripthandle, "Controls","SmoothInput",&config.SmoothInput);
        SCRIPT_GetNumber(config.scripthandle, "Controls","UseJoystick",&config.UseJoystick);
        SCRIPT_GetNumber(config.scripthandle, "Controls","UseMouse",&config.UseMouse);
        SCRIPT_GetNumber(config.scripthandle, "Controls","AimingFlag",(int32 *)&myaimmode);    // (if toggle mode) gives state
        SCRIPT_GetNumber(config.scripthandle, "Controls","RunKeyBehaviour",&ud.runkey_mode);   // JBF 20031125
        SCRIPT_GetNumber(config.scripthandle, "Controls","AutoAim",&config.AutoAim);          // JBF 20031125
        ps[0].auto_aim = config.AutoAim;
        SCRIPT_GetNumber(config.scripthandle, "Controls","WeaponSwitchMode",&ud.weaponswitch);
        ps[0].weaponswitch = ud.weaponswitch;

#ifdef _WIN32
        SCRIPT_GetNumber(config.scripthandle, "Updates", "CheckForUpdates", &config.CheckForUpdates);
        SCRIPT_GetNumber(config.scripthandle, "Updates", "LastUpdateCheck", &config.LastUpdateCheck);
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "WindowPositioning", (int32 *)&windowpos);
        windowx = -1;
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "WindowPosX", (int32 *)&windowx);
        windowy = -1;
        SCRIPT_GetNumber(config.scripthandle, "Screen Setup", "WindowPosY", (int32 *)&windowy);
#endif
    }

    CONFIG_ReadKeys();

    //CONFIG_SetupMouse(config.scripthandle);
    //CONFIG_SetupJoystick(config.scripthandle);
    config.setupread = 1;
    return 0;
}

/*
===================
=
= CONFIG_WriteSetup
=
===================
*/

void CONFIG_WriteSetup(void)
{
    int32 dummy;

    if (!config.setupread) return;

    if (config.scripthandle < 0)
        config.scripthandle = SCRIPT_Init(setupfilename);

    SCRIPT_PutNumber(config.scripthandle, "Controls","AimingFlag",(long) myaimmode,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Controls","AutoAim",config.AutoAim,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Controls","MouseAimingFlipped",ud.mouseflip,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Controls","MouseAiming",ud.mouseaiming,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Controls","MouseBias",config.MouseBias,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Controls","MouseFilter",config.MouseFilter,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Controls","SmoothInput",config.SmoothInput,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Controls","RunKeyBehaviour",ud.runkey_mode,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Controls","UseJoystick",config.UseJoystick,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Controls","UseMouse",config.UseMouse,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Controls","WeaponSwitchMode",ud.weaponswitch,false,false);

    SCRIPT_PutNumber(config.scripthandle, "Misc", "AutoMsg",ud.automsg,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "AutoVote",ud.autovote,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "Color",ud.color,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "Crosshairs",ud.crosshair,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "DeathMessages",ud.deathmsgs,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "DemoCams",ud.democams,false,false);
    ud.executions++;
    SCRIPT_PutNumber(config.scripthandle, "Misc", "Executions",ud.executions,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Setup", "ForceSetup",config.ForceSetup,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "IDPlayers",ud.idplayers,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "MPMessageDisplayTime",ud.msgdisptime,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "RunMode",config.RunMode,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "ShowFPS",ud.tickrate,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "ShowLevelStats",ud.levelstats,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "ShowOpponentWeapons",config.ShowOpponentWeapons,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "ShowViewWeapon",ud.drawweapon,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "StatusBarMode",ud.statusbarmode,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "StatusBarScale",ud.statusbarscale,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "Team",ud.team,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "UsePrecache",config.useprecache,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "ViewBobbing",ud.viewbob,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Misc", "WeaponSway",ud.weaponsway,false,false);
//    SCRIPT_PutNumber(config.scripthandle, "Misc", "AngleInterpolation",ud.angleinterpolation,false,false);

    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "Detail",ud.detail,false,false);
#if defined(POLYMOST) && defined(USE_OPENGL)
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "GLAnisotropy",glanisotropy,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "GLProjectionFix",glprojectionhacks,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "GLTextureMode",gltexfiltermode,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "GLUseCompressedTextureCache", glusetexcache,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "GLUseTextureCacheCompression", glusetexcachecompression,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "GLUseTextureCompr",glusetexcompr,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "GLWidescreen",glwidescreen,false,false);

    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "GLDepthPeeling",r_depthpeeling,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "GLPeelsCount",r_peelscount,false,false);

    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "GLDetailMapping", r_detailmapping,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "GLGlowMapping", r_glowmapping,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "GLVertexArrays", r_vertexarrays,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "GLVBOs", r_vbos,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "GLVBOCount", r_vbocount,false,false);

    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "GLAnimationSmoothing",r_animsmoothing,false,false);
#endif
#ifdef RENDERTYPEWIN
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "MaxRefreshFreq",maxrefreshfreq,false,false);
#endif
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "Messages",ud.fta_on,false,false);

    if (!NAM)
    {
        SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "Out",ud.lockout,false,false);
        SCRIPT_PutString(config.scripthandle, "Screen Setup", "Password",ud.pwlockout);
    }

    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "ScreenBPP",config.ScreenBPP,false,false);  // JBF 20040523
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "ScreenGamma",ud.brightness,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "ScreenHeight",config.ScreenHeight,false,false);    // JBF 20031206
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "ScreenMode",config.ScreenMode,false,false);    // JBF 20031206
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "ScreenSize",ud.screen_size,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "ScreenWidth",config.ScreenWidth,false,false);  // JBF 20031206
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "Shadows",ud.shadows,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "Tilt",ud.screen_tilting,false,false);
#if defined(POLYMOST) && defined(USE_OPENGL)
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "UseHightile",usehightile,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "UseModels",usemodels,false,false);
#endif
    SCRIPT_PutNumber(config.scripthandle, "Sound Setup", "AmbienceToggle",config.AmbienceToggle,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Sound Setup", "FXVolume",config.FXVolume,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Sound Setup", "MusicToggle",config.MusicToggle,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Sound Setup", "MusicVolume",config.MusicVolume,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Sound Setup", "NumVoices",config.NumVoices,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Sound Setup", "NumChannels",config.NumChannels,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Sound Setup", "NumBits",config.NumBits,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Sound Setup", "MixRate",config.MixRate,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Sound Setup", "ReverseStereo",config.ReverseStereo,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Sound Setup", "SoundToggle",config.SoundToggle,false,false);
    SCRIPT_PutNumber(config.scripthandle, "Sound Setup", "VoiceToggle",config.VoiceToggle,false,false);

#ifdef _WIN32
    SCRIPT_PutNumber(config.scripthandle, "Updates", "CheckForUpdates", config.CheckForUpdates, false, false);
    SCRIPT_PutNumber(config.scripthandle, "Updates", "LastUpdateCheck", config.LastUpdateCheck, false, false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "WindowPositioning", windowpos, false, false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "WindowPosX", windowx, false, false);
    SCRIPT_PutNumber(config.scripthandle, "Screen Setup", "WindowPosY", windowy, false, false);
#endif

    // JBF 20031211
    for (dummy=0;dummy<NUMGAMEFUNCTIONS;dummy++)
    {
        SCRIPT_PutDoubleString(config.scripthandle, "KeyDefinitions", CONFIG_FunctionNumToName(dummy),
                               KB_ScanCodeToString(config.KeyboardKeys[dummy][0]), KB_ScanCodeToString(config.KeyboardKeys[dummy][1]));
    }

    for (dummy=0;dummy<10;dummy++)
    {
        Bsprintf(buf,"WeaponChoice%ld",dummy);
        SCRIPT_PutNumber(config.scripthandle, "Misc",buf,ud.wchoice[myconnectindex][dummy],false,false);
    }

    for (dummy=0;dummy<MAXMOUSEBUTTONS;dummy++)
    {
        Bsprintf(buf,"MouseButton%ld",dummy);
        SCRIPT_PutString(config.scripthandle,"Controls", buf, CONFIG_FunctionNumToName(config.MouseFunctions[dummy][0]));

        if (dummy >= (MAXMOUSEBUTTONS-2)) continue;

        Bsprintf(buf,"MouseButtonClicked%ld",dummy);
        SCRIPT_PutString(config.scripthandle,"Controls", buf, CONFIG_FunctionNumToName(config.MouseFunctions[dummy][1]));
    }
    for (dummy=0;dummy<MAXMOUSEAXES;dummy++)
    {
        Bsprintf(buf,"MouseAnalogAxes%ld",dummy);
        SCRIPT_PutString(config.scripthandle, "Controls", buf, CONFIG_AnalogNumToName(config.MouseAnalogueAxes[dummy]));

        Bsprintf(buf,"MouseDigitalAxes%ld_0",dummy);
        SCRIPT_PutString(config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(config.MouseDigitalFunctions[dummy][0]));

        Bsprintf(buf,"MouseDigitalAxes%ld_1",dummy);
        SCRIPT_PutString(config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(config.MouseDigitalFunctions[dummy][1]));

        Bsprintf(buf,"MouseAnalogScale%ld",dummy);
        SCRIPT_PutNumber(config.scripthandle, "Controls", buf, config.MouseAnalogueScale[dummy], false, false);
    }
    dummy = CONTROL_GetMouseSensitivity();
    SCRIPT_PutNumber(config.scripthandle, "Controls","Mouse_Sensitivity",dummy,false,false);

    for (dummy=0;dummy<MAXJOYBUTTONS;dummy++)
    {
        Bsprintf(buf,"JoystickButton%ld",dummy);
        SCRIPT_PutString(config.scripthandle,"Controls", buf, CONFIG_FunctionNumToName(config.JoystickFunctions[dummy][0]));

        Bsprintf(buf,"JoystickButtonClicked%ld",dummy);
        SCRIPT_PutString(config.scripthandle,"Controls", buf, CONFIG_FunctionNumToName(config.JoystickFunctions[dummy][1]));
    }
    for (dummy=0;dummy<MAXJOYAXES;dummy++)
    {
        Bsprintf(buf,"JoystickAnalogAxes%ld",dummy);
        SCRIPT_PutString(config.scripthandle, "Controls", buf, CONFIG_AnalogNumToName(config.JoystickAnalogueAxes[dummy]));

        Bsprintf(buf,"JoystickDigitalAxes%ld_0",dummy);
        SCRIPT_PutString(config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(config.JoystickDigitalFunctions[dummy][0]));

        Bsprintf(buf,"JoystickDigitalAxes%ld_1",dummy);
        SCRIPT_PutString(config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(config.JoystickDigitalFunctions[dummy][1]));

        Bsprintf(buf,"JoystickAnalogScale%ld",dummy);
        SCRIPT_PutNumber(config.scripthandle, "Controls", buf, config.JoystickAnalogueScale[dummy], false, false);

        Bsprintf(buf,"JoystickAnalogDead%ld",dummy);
        SCRIPT_PutNumber(config.scripthandle, "Controls", buf, config.JoystickAnalogueDead[dummy], false, false);

        Bsprintf(buf,"JoystickAnalogSaturate%ld",dummy);
        SCRIPT_PutNumber(config.scripthandle, "Controls", buf, config.JoystickAnalogueSaturate[dummy], false, false);
    }

    SCRIPT_PutString(config.scripthandle, "Comm Setup","PlayerName",&myname[0]);
    SCRIPT_PutString(config.scripthandle, "Comm Setup","RTSName",&ud.rtsname[0]);

    SCRIPT_PutNumber(config.scripthandle, "Comm Setup", "Rate", packetrate, false, false);


    SCRIPT_PutString(config.scripthandle, "Misc","SelectedGRP",&duke3dgrp[0]);
    {
        char commmacro[] = "CommbatMacro# ";

        for (dummy = 0;dummy < 10;dummy++)
        {
            commmacro[13] = dummy+'0';
            SCRIPT_PutString(config.scripthandle, "Comm Setup",commmacro,&ud.ridecule[dummy][0]);
        }
    }

    SCRIPT_Save(config.scripthandle, setupfilename);
    SCRIPT_Free(config.scripthandle);
}


int32 CONFIG_GetMapBestTime(char *mapname)
{
    int32 t = -1;
    char m[BMAX_PATH], *p;

    strcpy(m, mapname);
    p = strrchr(m, '/');
    if (!p) p = strrchr(m, '\\');
    if (p) strcpy(m, p);
    for (p=m;*p;p++) *p = tolower(*p);

    if (!config.setupread) return -1;
    if (config.scripthandle < 0) return -1;
    SCRIPT_GetNumber(config.scripthandle, "MapTimes", m, &t);

    return t;
}

int32 CONFIG_SetMapBestTime(char *mapname, int32 tm)
{
    char m[BMAX_PATH], *p;

    strcpy(m, mapname);
    p = strrchr(m, '/');
    if (!p) p = strrchr(m, '\\');
    if (p) strcpy(m, p);
    for (p=m;*p;p++) *p = tolower(*p);

    if (config.scripthandle < 0) config.scripthandle = SCRIPT_Init(setupfilename);
    if (config.scripthandle < 0) return -1;

    SCRIPT_PutNumber(config.scripthandle, "MapTimes", mapname, tm, false, false);
    return 0;
}

/*
 * vim:ts=4:sw=4:
 */

