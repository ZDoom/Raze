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

    memset(ud.config.KeyboardKeys, 0xff, sizeof(ud.config.KeyboardKeys));

    if (type == 1)
    {
        for (i=0; i < (int32)(sizeof(oldkeydefaults)/sizeof(oldkeydefaults[0])); i+=3)
        {
            f = CONFIG_FunctionNameToNum((char *)oldkeydefaults[i+0]);
            if (f == -1) continue;
            ud.config.KeyboardKeys[f][0] = KB_StringToScanCode((char *)oldkeydefaults[i+1]);
            ud.config.KeyboardKeys[f][1] = KB_StringToScanCode((char *)oldkeydefaults[i+2]);

            if (f == gamefunc_Show_Console) OSD_CaptureKey(ud.config.KeyboardKeys[f][0]);
            else CONTROL_MapKey(f, ud.config.KeyboardKeys[f][0], ud.config.KeyboardKeys[f][1]);
        }
        return;
    }

    for (i=0; i < (int32)(sizeof(keydefaults)/sizeof(keydefaults[0])); i+=3)
    {
        f = CONFIG_FunctionNameToNum(keydefaults[i+0]);
        if (f == -1) continue;
        ud.config.KeyboardKeys[f][0] = KB_StringToScanCode(keydefaults[i+1]);
        ud.config.KeyboardKeys[f][1] = KB_StringToScanCode(keydefaults[i+2]);

        if (f == gamefunc_Show_Console) OSD_CaptureKey(ud.config.KeyboardKeys[f][0]);
        else CONTROL_MapKey(f, ud.config.KeyboardKeys[f][0], ud.config.KeyboardKeys[f][1]);
    }
}

void CONFIG_SetDefaults(void)
{
    // JBF 20031211
    int32 i;

    ud.config.scripthandle = -1;
    ud.config.ScreenWidth = 1024;
    ud.config.ScreenHeight = 768;
    ud.config.ScreenMode = 0;
#if defined(POLYMOST) && defined(USE_OPENGL)
    ud.config.ScreenBPP = 32;
#else
    ud.config.ScreenBPP = 8;
#endif
    ud.config.useprecache = 1;
    ud.config.ForceSetup = 1;
    ud.config.AmbienceToggle = 1;
    ud.config.AutoAim = 1;
    ud.config.FXDevice = 0;
    ud.config.FXVolume = 220;
    ud.config.MixRate = 44100;
    ud.config.MouseBias = 0;
    ud.config.MouseFilter = 0;
    ud.config.MusicDevice = 0;
    ud.config.MusicToggle = 1;
    ud.config.MusicVolume = 200;
    myaimmode = g_player[0].ps->aim_mode = 1;
    ud.config.NumBits = 16;
    ud.config.NumChannels = 2;
    ud.config.NumVoices = 32;
    ud.config.ReverseStereo = 0;
    ud.config.RunMode = ud.auto_run = 1;
    ud.config.ShowOpponentWeapons = 0;
    ud.config.SmoothInput = 1;
    ud.config.SoundToggle = 1;
    ud.automsg = 0;
    ud.autovote = 0;
    ud.brightness = 8;
    ud.camerasprite = -1;
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
    ud.config.UseJoystick = 0;
    ud.config.UseMouse = 1;
    ud.config.VoiceToggle = 2;

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

    memset(ud.config.MouseFunctions, -1, sizeof(ud.config.MouseFunctions));
    for (i=0; i<MAXMOUSEBUTTONS; i++)
    {
        ud.config.MouseFunctions[i][0] = CONFIG_FunctionNameToNum(mousedefaults[i]);
        CONTROL_MapButton(ud.config.MouseFunctions[i][0], i, 0, controldevice_mouse);
        if (i>=4) continue;
        ud.config.MouseFunctions[i][1] = CONFIG_FunctionNameToNum(mouseclickeddefaults[i]);
        CONTROL_MapButton(ud.config.MouseFunctions[i][1], i, 1, controldevice_mouse);
    }

    memset(ud.config.MouseDigitalFunctions, -1, sizeof(ud.config.MouseDigitalFunctions));
    for (i=0; i<MAXMOUSEAXES; i++)
    {
        ud.config.MouseAnalogueScale[i] = 65536;
        CONTROL_SetAnalogAxisScale(i, ud.config.MouseAnalogueScale[i], controldevice_mouse);

        ud.config.MouseDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(mousedigitaldefaults[i*2]);
        ud.config.MouseDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(mousedigitaldefaults[i*2+1]);
        CONTROL_MapDigitalAxis(i, ud.config.MouseDigitalFunctions[i][0], 0, controldevice_mouse);
        CONTROL_MapDigitalAxis(i, ud.config.MouseDigitalFunctions[i][1], 1, controldevice_mouse);

        ud.config.MouseAnalogueAxes[i] = CONFIG_AnalogNameToNum(mouseanalogdefaults[i]);
        CONTROL_MapAnalogAxis(i, ud.config.MouseAnalogueAxes[i], controldevice_mouse);
    }
    CONTROL_SetMouseSensitivity(DEFAULTMOUSESENSITIVITY);

    memset(ud.config.JoystickFunctions, -1, sizeof(ud.config.JoystickFunctions));
    for (i=0; i<MAXJOYBUTTONS; i++)
    {
        ud.config.JoystickFunctions[i][0] = CONFIG_FunctionNameToNum(joystickdefaults[i]);
        ud.config.JoystickFunctions[i][1] = CONFIG_FunctionNameToNum(joystickclickeddefaults[i]);
        CONTROL_MapButton(ud.config.JoystickFunctions[i][0], i, 0, controldevice_joystick);
        CONTROL_MapButton(ud.config.JoystickFunctions[i][1], i, 1, controldevice_joystick);
    }

    memset(ud.config.JoystickDigitalFunctions, -1, sizeof(ud.config.JoystickDigitalFunctions));
    for (i=0; i<MAXJOYAXES; i++)
    {
        ud.config.JoystickAnalogueScale[i] = 65536;
        ud.config.JoystickAnalogueDead[i] = 1000;
        ud.config.JoystickAnalogueSaturate[i] = 9500;
        CONTROL_SetAnalogAxisScale(i, ud.config.JoystickAnalogueScale[i], controldevice_joystick);

        ud.config.JoystickDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(joystickdigitaldefaults[i*2]);
        ud.config.JoystickDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(joystickdigitaldefaults[i*2+1]);
        CONTROL_MapDigitalAxis(i, ud.config.JoystickDigitalFunctions[i][0], 0, controldevice_joystick);
        CONTROL_MapDigitalAxis(i, ud.config.JoystickDigitalFunctions[i][1], 1, controldevice_joystick);

        ud.config.JoystickAnalogueAxes[i] = CONFIG_AnalogNameToNum(joystickanalogdefaults[i]);
        CONTROL_MapAnalogAxis(i, ud.config.JoystickAnalogueAxes[i], controldevice_joystick);
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

    if (ud.config.scripthandle < 0) return;

    numkeyentries = SCRIPT_NumberEntries(ud.config.scripthandle,"KeyDefinitions");

    for (i=0;i<numkeyentries;i++)
    {
        function = CONFIG_FunctionNameToNum(SCRIPT_Entry(ud.config.scripthandle,"KeyDefinitions", i));
        if (function != -1)
        {
            memset(keyname1,0,sizeof(keyname1));
            memset(keyname2,0,sizeof(keyname2));
            SCRIPT_GetDoubleString
            (
                ud.config.scripthandle,
                "KeyDefinitions",
                SCRIPT_Entry(ud.config.scripthandle, "KeyDefinitions", i),
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
            ud.config.KeyboardKeys[function][0] = key1;
            ud.config.KeyboardKeys[function][1] = key2;
        }
    }

    for (i=0; i<NUMGAMEFUNCTIONS; i++)
    {
        if (i == gamefunc_Show_Console)
            OSD_CaptureKey(ud.config.KeyboardKeys[i][0]);
        else
            CONTROL_MapKey(i, ud.config.KeyboardKeys[i][0], ud.config.KeyboardKeys[i][1]);
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

    if (ud.config.scripthandle < 0) return;

    for (i=0;i<MAXMOUSEBUTTONS;i++)
    {
        Bsprintf(str,"MouseButton%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle,"Controls", str,temp))
        	ud.config.MouseFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseButtonClicked%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle,"Controls", str,temp))
        	ud.config.MouseFunctions[i][1] = CONFIG_FunctionNameToNum(temp);
    }

    // map over the axes
    for (i=0;i<MAXMOUSEAXES;i++)
    {
        Bsprintf(str,"MouseAnalogAxes%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_AnalogNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                ud.config.MouseAnalogueAxes[i] = CONFIG_AnalogNameToNum(temp);

        Bsprintf(str,"MouseDigitalAxes%d_0",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                ud.config.MouseDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseDigitalAxes%d_1",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                ud.config.MouseDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseAnalogScale%d",i);
        scale = ud.config.MouseAnalogueScale[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        ud.config.MouseAnalogueScale[i] = scale;
    }

    function = DEFAULTMOUSESENSITIVITY;
    SCRIPT_GetNumber(ud.config.scripthandle, "Controls","Mouse_Sensitivity",&function);
    CONTROL_SetMouseSensitivity(function);

    for (i=0; i<MAXMOUSEBUTTONS; i++)
    {
        CONTROL_MapButton(ud.config.MouseFunctions[i][0], i, 0, controldevice_mouse);
        CONTROL_MapButton(ud.config.MouseFunctions[i][1], i, 1,  controldevice_mouse);
    }
    for (i=0; i<MAXMOUSEAXES; i++)
    {
        CONTROL_MapAnalogAxis(i, ud.config.MouseAnalogueAxes[i], controldevice_mouse);
        CONTROL_MapDigitalAxis(i, ud.config.MouseDigitalFunctions[i][0], 0,controldevice_mouse);
        CONTROL_MapDigitalAxis(i, ud.config.MouseDigitalFunctions[i][1], 1,controldevice_mouse);
        CONTROL_SetAnalogAxisScale(i, ud.config.MouseAnalogueScale[i], controldevice_mouse);
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

    if (ud.config.scripthandle < 0) return;

    for (i=0;i<MAXJOYBUTTONS;i++)
    {
        Bsprintf(str,"JoystickButton%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle,"Controls", str,temp))
        	ud.config.JoystickFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickButtonClicked%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle,"Controls", str,temp))
        	ud.config.JoystickFunctions[i][1] = CONFIG_FunctionNameToNum(temp);
    }

    // map over the axes
    for (i=0;i<MAXJOYAXES;i++)
    {
        Bsprintf(str,"JoystickAnalogAxes%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_AnalogNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                ud.config.JoystickAnalogueAxes[i] = CONFIG_AnalogNameToNum(temp);

        Bsprintf(str,"JoystickDigitalAxes%d_0",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                ud.config.JoystickDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickDigitalAxes%d_1",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                ud.config.JoystickDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickAnalogScale%d",i);
        scale = ud.config.JoystickAnalogueScale[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        ud.config.JoystickAnalogueScale[i] = scale;

        Bsprintf(str,"JoystickAnalogDead%d",i);
        scale = ud.config.JoystickAnalogueDead[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        ud.config.JoystickAnalogueDead[i] = scale;

        Bsprintf(str,"JoystickAnalogSaturate%d",i);
        scale = ud.config.JoystickAnalogueSaturate[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        ud.config.JoystickAnalogueSaturate[i] = scale;
    }

    for (i=0;i<MAXJOYBUTTONS;i++)
    {
        CONTROL_MapButton(ud.config.JoystickFunctions[i][0], i, 0, controldevice_joystick);
        CONTROL_MapButton(ud.config.JoystickFunctions[i][1], i, 1,  controldevice_joystick);
    }
    for (i=0;i<MAXJOYAXES;i++)
    {
        CONTROL_MapAnalogAxis(i, ud.config.JoystickAnalogueAxes[i], controldevice_joystick);
        CONTROL_MapDigitalAxis(i, ud.config.JoystickDigitalFunctions[i][0], 0, controldevice_joystick);
        CONTROL_MapDigitalAxis(i, ud.config.JoystickDigitalFunctions[i][1], 1, controldevice_joystick);
        CONTROL_SetAnalogAxisScale(i, ud.config.JoystickAnalogueScale[i], controldevice_joystick);
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

    ud.config.setupread = 1;

    pathsearchmode = 1;
    if (SafeFileExists(setupfilename) && ud.config.scripthandle < 0)  // JBF 20031211
        ud.config.scripthandle = SCRIPT_Load(setupfilename);
    pathsearchmode = 0;

    if (ud.config.scripthandle < 0) return -1;

    if (ud.config.scripthandle >= 0)
    {
        for (dummy = 0;dummy < 10;dummy++)
        {
            commmacro[13] = dummy+'0';
            SCRIPT_GetString(ud.config.scripthandle, "Comm Setup",commmacro,&ud.ridecule[dummy][0]);
        }

        SCRIPT_GetString(ud.config.scripthandle, "Comm Setup","PlayerName",&tempbuf[0]);

        while (Bstrlen(stripcolorcodes(tempbuf)) > 10)
            tempbuf[Bstrlen(tempbuf)-1] = '\0';

        Bstrncpy(myname,tempbuf,sizeof(myname)-1);
        myname[sizeof(myname)] = '\0';

        SCRIPT_GetString(ud.config.scripthandle, "Comm Setup","RTSName",&ud.rtsname[0]);

        SCRIPT_GetNumber(ud.config.scripthandle, "Comm Setup", "Rate",(int32 *)&packetrate);
        packetrate = min(max(packetrate,50),1000);

        {
            extern char defaultduke3dgrp[BMAX_PATH];
            if (!Bstrcmp(defaultduke3dgrp,"duke3d.grp"))
                SCRIPT_GetString(ud.config.scripthandle, "Misc","SelectedGRP",&duke3dgrp[0]);
        }

        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "Shadows",&ud.shadows);

        if (!NAM)
        {
            SCRIPT_GetString(ud.config.scripthandle, "Screen Setup","Password",&ud.pwlockout[0]);
            SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "Out",&ud.lockout);
        }

        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "Detail",&ud.detail);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "Tilt",&ud.screen_tilting);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "Messages",&ud.fta_on);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "ScreenWidth",&ud.config.ScreenWidth);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "ScreenHeight",&ud.config.ScreenHeight);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "ScreenMode",&ud.config.ScreenMode);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "ScreenGamma",&ud.brightness);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "ScreenSize",&ud.screen_size);

#if defined(POLYMOST) && defined(USE_OPENGL)
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "ScreenBPP", &ud.config.ScreenBPP);
        if (ud.config.ScreenBPP < 8) ud.config.ScreenBPP = 32;
#endif

#ifdef RENDERTYPEWIN
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "MaxRefreshFreq", (int32*)&maxrefreshfreq);
#endif
#if defined(POLYMOST) && defined(USE_OPENGL)
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLTextureMode", &gltexfiltermode);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLAnisotropy", &glanisotropy);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLProjectionFix", &glprojectionhacks);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLUseTextureCompr", &glusetexcompr);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLWidescreen", &glwidescreen);

        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLUseCompressedTextureCache", &glusetexcache);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLUseTextureCacheCompression", &glusetexcachecompression);

        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLDepthPeeling", &r_depthpeeling);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLPeelsCount", &r_peelscount);

        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLDetailMapping", &r_detailmapping);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLGlowMapping", &r_glowmapping);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLVertexArrays", &r_vertexarrays);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLVBOs", &r_vbos);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLVBOCount", &r_vbocount);

        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLAnimationSmoothing", &r_animsmoothing);

        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLParallaxSkyClamping", &r_parallaxskyclamping);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "GLParallaxSkyPanning", &r_parallaxskypanning);
        dummy = usemodels;
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "UseModels",&dummy);
        usemodels = dummy != 0;
        dummy = usehightile;
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "UseHightile",&dummy);
        usehightile = dummy != 0;
#endif
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "Executions",&ud.executions);
        SCRIPT_GetNumber(ud.config.scripthandle, "Setup", "ForceSetup",&ud.config.ForceSetup);
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "RunMode",&ud.config.RunMode);
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "Crosshairs",&ud.crosshair);
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "StatusBarScale",&ud.statusbarscale);
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "ShowLevelStats",&ud.levelstats);
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "ShowOpponentWeapons",&ud.config.ShowOpponentWeapons);
        ud.showweapons = ud.config.ShowOpponentWeapons;
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "ShowViewWeapon",&ud.drawweapon);
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "DeathMessages",&ud.deathmsgs);
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "DemoCams",&ud.democams);
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "ShowFPS",&ud.tickrate);
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "Color",&ud.color);
        check_player_color((int *)&ud.color,-1);
        g_player[0].ps->palookup = g_player[0].pcolor = ud.color;
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "Team",&dummy);
        ud.team = 0;
        if (dummy < 4 && dummy > -1) ud.team = dummy;
        g_player[0].pteam = ud.team;
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "MPMessageDisplayTime",&ud.msgdisptime);
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "StatusBarMode",&ud.statusbarmode);
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "AutoVote",&ud.autovote);
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "AutoMsg",&ud.automsg);
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "IDPlayers",&ud.automsg);
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "ViewBobbing",&ud.viewbob);
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "WeaponSway",&ud.weaponsway);

        dummy = ud.config.useprecache;
        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "UsePrecache",&dummy);
        ud.config.useprecache = dummy != 0;

        SCRIPT_GetNumber(ud.config.scripthandle, "Misc","AngleInterpolation",&ud.angleinterpolation);

        // weapon choices are defaulted in checkcommandline, which may override them
        if (!CommandWeaponChoice)
            for (i=0;i<10;i++)
            {
                Bsprintf(buf,"WeaponChoice%d",i);
                dummy = -1;
                SCRIPT_GetNumber(ud.config.scripthandle, "Misc", buf, &dummy);
                if (dummy >= 0) g_player[0].wchoice[i] = dummy;
            }

        SCRIPT_GetNumber(ud.config.scripthandle, "Sound Setup", "FXDevice",&ud.config.FXDevice);
        SCRIPT_GetNumber(ud.config.scripthandle, "Sound Setup", "MusicDevice",&ud.config.MusicDevice);
        SCRIPT_GetNumber(ud.config.scripthandle, "Sound Setup", "FXVolume",&ud.config.FXVolume);
        SCRIPT_GetNumber(ud.config.scripthandle, "Sound Setup", "MusicVolume",&ud.config.MusicVolume);
        SCRIPT_GetNumber(ud.config.scripthandle, "Sound Setup", "SoundToggle",&ud.config.SoundToggle);
        SCRIPT_GetNumber(ud.config.scripthandle, "Sound Setup", "MusicToggle",&ud.config.MusicToggle);
        SCRIPT_GetNumber(ud.config.scripthandle, "Sound Setup", "VoiceToggle",&ud.config.VoiceToggle);
        SCRIPT_GetNumber(ud.config.scripthandle, "Sound Setup", "AmbienceToggle",&ud.config.AmbienceToggle);
        SCRIPT_GetNumber(ud.config.scripthandle, "Sound Setup", "NumVoices",&ud.config.NumVoices);
        SCRIPT_GetNumber(ud.config.scripthandle, "Sound Setup", "NumChannels",&ud.config.NumChannels);
        SCRIPT_GetNumber(ud.config.scripthandle, "Sound Setup", "NumBits",&ud.config.NumBits);
        SCRIPT_GetNumber(ud.config.scripthandle, "Sound Setup", "MixRate",&ud.config.MixRate);
        SCRIPT_GetNumber(ud.config.scripthandle, "Sound Setup", "ReverseStereo",&ud.config.ReverseStereo);

        SCRIPT_GetNumber(ud.config.scripthandle, "Controls","MouseAimingFlipped",&ud.mouseflip);  // mouse aiming inverted
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls","MouseAiming",&ud.mouseaiming);		// 1=momentary/0=toggle
        g_player[0].ps->aim_mode = ud.mouseaiming;
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls","MouseBias",&ud.config.MouseBias);
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls","MouseFilter",&ud.config.MouseFilter);
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls","SmoothInput",&ud.config.SmoothInput);
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls","UseJoystick",&ud.config.UseJoystick);
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls","UseMouse",&ud.config.UseMouse);
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls","AimingFlag",(int32 *)&myaimmode);    // (if toggle mode) gives state
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls","RunKeyBehaviour",&ud.runkey_mode);   // JBF 20031125
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls","AutoAim",&ud.config.AutoAim);          // JBF 20031125
        g_player[0].ps->auto_aim = ud.config.AutoAim;
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls","WeaponSwitchMode",&ud.weaponswitch);
        g_player[0].ps->weaponswitch = ud.weaponswitch;

#ifdef _WIN32
        SCRIPT_GetNumber(ud.config.scripthandle, "Updates", "CheckForUpdates", &ud.config.CheckForUpdates);
        SCRIPT_GetNumber(ud.config.scripthandle, "Updates", "LastUpdateCheck", &ud.config.LastUpdateCheck);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "WindowPositioning", (int32 *)&windowpos);
        windowx = -1;
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "WindowPosX", (int32 *)&windowx);
        windowy = -1;
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "WindowPosY", (int32 *)&windowy);
#endif
    }

    CONFIG_ReadKeys();

    //CONFIG_SetupMouse(ud.config.scripthandle);
    //CONFIG_SetupJoystick(ud.config.scripthandle);
    ud.config.setupread = 1;
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

    if (!ud.config.setupread) return;

    if (ud.config.scripthandle < 0)
        ud.config.scripthandle = SCRIPT_Init(setupfilename);

    SCRIPT_PutNumber(ud.config.scripthandle, "Controls","AimingFlag",(int) myaimmode,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Controls","AutoAim",ud.config.AutoAim,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Controls","MouseAimingFlipped",ud.mouseflip,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Controls","MouseAiming",ud.mouseaiming,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Controls","MouseBias",ud.config.MouseBias,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Controls","MouseFilter",ud.config.MouseFilter,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Controls","SmoothInput",ud.config.SmoothInput,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Controls","RunKeyBehaviour",ud.runkey_mode,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Controls","UseJoystick",ud.config.UseJoystick,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Controls","UseMouse",ud.config.UseMouse,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Controls","WeaponSwitchMode",ud.weaponswitch,false,false);

    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "AutoMsg",ud.automsg,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "AutoVote",ud.autovote,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "Color",ud.color,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "Crosshairs",ud.crosshair,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "DeathMessages",ud.deathmsgs,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "DemoCams",ud.democams,false,false);
    ud.executions++;
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "Executions",ud.executions,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Setup", "ForceSetup",ud.config.ForceSetup,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "IDPlayers",ud.idplayers,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "MPMessageDisplayTime",ud.msgdisptime,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "RunMode",ud.config.RunMode,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "ShowFPS",ud.tickrate,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "ShowLevelStats",ud.levelstats,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "ShowOpponentWeapons",ud.config.ShowOpponentWeapons,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "ShowViewWeapon",ud.drawweapon,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "StatusBarMode",ud.statusbarmode,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "StatusBarScale",ud.statusbarscale,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "Team",ud.team,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "UsePrecache",ud.config.useprecache,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "ViewBobbing",ud.viewbob,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "WeaponSway",ud.weaponsway,false,false);
//    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "AngleInterpolation",ud.angleinterpolation,false,false);

    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "Detail",ud.detail,false,false);
#if defined(POLYMOST) && defined(USE_OPENGL)
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLAnisotropy",glanisotropy,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLProjectionFix",glprojectionhacks,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLTextureMode",gltexfiltermode,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLUseCompressedTextureCache", glusetexcache,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLUseTextureCacheCompression", glusetexcachecompression,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLUseTextureCompr",glusetexcompr,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLWidescreen",glwidescreen,false,false);

    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLDepthPeeling",r_depthpeeling,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLPeelsCount",r_peelscount,false,false);

    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLDetailMapping", r_detailmapping,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLGlowMapping", r_glowmapping,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLVertexArrays", r_vertexarrays,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLVBOs", r_vbos,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLVBOCount", r_vbocount,false,false);

    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLAnimationSmoothing",r_animsmoothing,false,false);

    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLParallaxSkyClamping",r_parallaxskyclamping,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "GLParallaxSkyPanning",r_parallaxskypanning,false,false);
#endif
#ifdef RENDERTYPEWIN
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "MaxRefreshFreq",maxrefreshfreq,false,false);
#endif
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "Messages",ud.fta_on,false,false);

    if (!NAM)
    {
        SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "Out",ud.lockout,false,false);
        SCRIPT_PutString(ud.config.scripthandle, "Screen Setup", "Password",ud.pwlockout);
    }

    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "ScreenBPP",ud.config.ScreenBPP,false,false);  // JBF 20040523
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "ScreenGamma",ud.brightness,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "ScreenHeight",ud.config.ScreenHeight,false,false);    // JBF 20031206
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "ScreenMode",ud.config.ScreenMode,false,false);    // JBF 20031206
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "ScreenSize",ud.screen_size,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "ScreenWidth",ud.config.ScreenWidth,false,false);  // JBF 20031206
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "Shadows",ud.shadows,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "Tilt",ud.screen_tilting,false,false);
#if defined(POLYMOST) && defined(USE_OPENGL)
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "UseHightile",usehightile,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "UseModels",usemodels,false,false);
#endif
    SCRIPT_PutNumber(ud.config.scripthandle, "Sound Setup", "AmbienceToggle",ud.config.AmbienceToggle,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Sound Setup", "FXVolume",ud.config.FXVolume,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Sound Setup", "MusicToggle",ud.config.MusicToggle,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Sound Setup", "MusicVolume",ud.config.MusicVolume,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Sound Setup", "NumVoices",ud.config.NumVoices,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Sound Setup", "NumChannels",ud.config.NumChannels,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Sound Setup", "NumBits",ud.config.NumBits,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Sound Setup", "MixRate",ud.config.MixRate,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Sound Setup", "ReverseStereo",ud.config.ReverseStereo,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Sound Setup", "SoundToggle",ud.config.SoundToggle,false,false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Sound Setup", "VoiceToggle",ud.config.VoiceToggle,false,false);

#ifdef _WIN32
    SCRIPT_PutNumber(ud.config.scripthandle, "Updates", "CheckForUpdates", ud.config.CheckForUpdates, false, false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Updates", "LastUpdateCheck", ud.config.LastUpdateCheck, false, false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "WindowPositioning", windowpos, false, false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "WindowPosX", windowx, false, false);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "WindowPosY", windowy, false, false);
#endif

    // JBF 20031211
    for (dummy=0;dummy<NUMGAMEFUNCTIONS;dummy++)
    {
        SCRIPT_PutDoubleString(ud.config.scripthandle, "KeyDefinitions", CONFIG_FunctionNumToName(dummy),
                               KB_ScanCodeToString(ud.config.KeyboardKeys[dummy][0]), KB_ScanCodeToString(ud.config.KeyboardKeys[dummy][1]));
    }

    for (dummy=0;dummy<10;dummy++)
    {
        Bsprintf(buf,"WeaponChoice%d",dummy);
        SCRIPT_PutNumber(ud.config.scripthandle, "Misc",buf,g_player[myconnectindex].wchoice[dummy],false,false);
    }

    for (dummy=0;dummy<MAXMOUSEBUTTONS;dummy++)
    {
        Bsprintf(buf,"MouseButton%d",dummy);
        SCRIPT_PutString(ud.config.scripthandle,"Controls", buf, CONFIG_FunctionNumToName(ud.config.MouseFunctions[dummy][0]));

        if (dummy >= (MAXMOUSEBUTTONS-2)) continue;

        Bsprintf(buf,"MouseButtonClicked%d",dummy);
        SCRIPT_PutString(ud.config.scripthandle,"Controls", buf, CONFIG_FunctionNumToName(ud.config.MouseFunctions[dummy][1]));
    }

    for (dummy=0;dummy<MAXMOUSEAXES;dummy++)
    {
        Bsprintf(buf,"MouseAnalogAxes%d",dummy);
        SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_AnalogNumToName(ud.config.MouseAnalogueAxes[dummy]));

        Bsprintf(buf,"MouseDigitalAxes%d_0",dummy);
        SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(ud.config.MouseDigitalFunctions[dummy][0]));

        Bsprintf(buf,"MouseDigitalAxes%d_1",dummy);
        SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(ud.config.MouseDigitalFunctions[dummy][1]));

        Bsprintf(buf,"MouseAnalogScale%d",dummy);
        SCRIPT_PutNumber(ud.config.scripthandle, "Controls", buf, ud.config.MouseAnalogueScale[dummy], false, false);
    }
    dummy = CONTROL_GetMouseSensitivity();
    SCRIPT_PutNumber(ud.config.scripthandle, "Controls","Mouse_Sensitivity",dummy,false,false);

    for (dummy=0;dummy<MAXJOYBUTTONS;dummy++)
    {
        Bsprintf(buf,"JoystickButton%d",dummy);
        SCRIPT_PutString(ud.config.scripthandle,"Controls", buf, CONFIG_FunctionNumToName(ud.config.JoystickFunctions[dummy][0]));

        Bsprintf(buf,"JoystickButtonClicked%d",dummy);
        SCRIPT_PutString(ud.config.scripthandle,"Controls", buf, CONFIG_FunctionNumToName(ud.config.JoystickFunctions[dummy][1]));
    }
    for (dummy=0;dummy<MAXJOYAXES;dummy++)
    {
        Bsprintf(buf,"JoystickAnalogAxes%d",dummy);
        SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_AnalogNumToName(ud.config.JoystickAnalogueAxes[dummy]));

        Bsprintf(buf,"JoystickDigitalAxes%d_0",dummy);
        SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(ud.config.JoystickDigitalFunctions[dummy][0]));

        Bsprintf(buf,"JoystickDigitalAxes%d_1",dummy);
        SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(ud.config.JoystickDigitalFunctions[dummy][1]));

        Bsprintf(buf,"JoystickAnalogScale%d",dummy);
        SCRIPT_PutNumber(ud.config.scripthandle, "Controls", buf, ud.config.JoystickAnalogueScale[dummy], false, false);

        Bsprintf(buf,"JoystickAnalogDead%d",dummy);
        SCRIPT_PutNumber(ud.config.scripthandle, "Controls", buf, ud.config.JoystickAnalogueDead[dummy], false, false);

        Bsprintf(buf,"JoystickAnalogSaturate%d",dummy);
        SCRIPT_PutNumber(ud.config.scripthandle, "Controls", buf, ud.config.JoystickAnalogueSaturate[dummy], false, false);
    }

    SCRIPT_PutString(ud.config.scripthandle, "Comm Setup","PlayerName",&myname[0]);
    SCRIPT_PutString(ud.config.scripthandle, "Comm Setup","RTSName",&ud.rtsname[0]);

    SCRIPT_PutNumber(ud.config.scripthandle, "Comm Setup", "Rate", packetrate, false, false);


    SCRIPT_PutString(ud.config.scripthandle, "Misc","SelectedGRP",&duke3dgrp[0]);
    {
        char commmacro[] = "CommbatMacro# ";

        for (dummy = 0;dummy < 10;dummy++)
        {
            commmacro[13] = dummy+'0';
            SCRIPT_PutString(ud.config.scripthandle, "Comm Setup",commmacro,&ud.ridecule[dummy][0]);
        }
    }

    SCRIPT_Save(ud.config.scripthandle, setupfilename);
    SCRIPT_Free(ud.config.scripthandle);
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

    if (!ud.config.setupread) return -1;
    if (ud.config.scripthandle < 0) return -1;
    SCRIPT_GetNumber(ud.config.scripthandle, "MapTimes", m, &t);

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

    if (ud.config.scripthandle < 0) ud.config.scripthandle = SCRIPT_Init(setupfilename);
    if (ud.config.scripthandle < 0) return -1;

    SCRIPT_PutNumber(ud.config.scripthandle, "MapTimes", mapname, tm, false, false);
    return 0;
}

/*
 * vim:ts=4:sw=4:
 */

