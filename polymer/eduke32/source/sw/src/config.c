//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "build.h"
#include "baselayer.h"
#include "osd.h"

#include "settings.h"
#include "mytypes.h"
#include "scriplib.h"
#include "file_lib.h"
#include "gamedefs.h"
#include "keyboard.h"
#include "function.h"
#include "control.h"
#include "fx_man.h"
#include "sounds.h"
#include "config.h"
#include "common_game.h"

// we load this in to get default button and key assignments
// as well as setting up function mappings

#include "_functio.h"
#include "_config.h"

extern void ReadGameSetup(int32_t scripthandle);
extern void WriteGameSetup(int32_t scripthandle);

//
// Comm variables
//

char CommPlayerName[32];
int32_t NumberPlayers,CommPort,PortSpeed,IrqNumber,UartAddress;

//
// Sound variables
//
int32_t FXDevice    = 0;
int32_t MusicDevice = 0;
int32_t NumVoices   = 32;
int32_t NumChannels = 2;
int32_t NumBits     = 16;
int32_t MixRate     = 44100;

int32_t UseMouse = 1, UseJoystick = 0;

uint8_t KeyboardKeys[NUMGAMEFUNCTIONS][2];
int32_t MouseButtons[MAXMOUSEBUTTONS];
int32_t MouseButtonsClicked[MAXMOUSEBUTTONS];
int32_t MouseDigitalAxes[MAXMOUSEAXES][2];
int32_t MouseAnalogAxes[MAXMOUSEAXES];
int32_t MouseAnalogScale[MAXMOUSEAXES];
int32_t JoystickButtons[MAXJOYBUTTONS];
int32_t JoystickButtonsClicked[MAXJOYBUTTONS];
int32_t JoystickDigitalAxes[MAXJOYAXES][2];
int32_t JoystickAnalogAxes[MAXJOYAXES];
int32_t JoystickAnalogScale[MAXJOYAXES];
int32_t JoystickAnalogDead[MAXJOYAXES];
int32_t JoystickAnalogSaturate[MAXJOYAXES];

//
// Screen variables
//

int32_t ScreenMode = 1;
int32_t ScreenWidth = 640;
int32_t ScreenHeight = 480;
int32_t ScreenBPP = 8;
int32_t ForceSetup = 1;

extern char WangBangMacro[10][64];
char  RTSName[MAXRTSNAMELENGTH];
//static char setupfilename[64]={SETUPFILENAME};
char setupfilename[64]= {SETUPFILENAME};
static int32_t scripthandle = -1;



/*
===================
=
= CONFIG_FunctionNameToNum
=
===================
*/

int32_t CONFIG_FunctionNameToNum(const char *func)
{
    int32_t i;

    if (!func) return -1;
    for (i=0; i<NUMGAMEFUNCTIONS; i++)
    {
        if (!Bstrcasecmp(func,gamefunctions[i]))
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

const char *CONFIG_FunctionNumToName(int32_t func)
{
    if ((unsigned)func >= (unsigned)NUMGAMEFUNCTIONS)
    {
        return NULL;
    }
    else
    {
        return gamefunctions[func];
    }
}

/*
===================
=
= CONFIG_AnalogNameToNum
=
===================
*/
int32_t CONFIG_AnalogNameToNum(const char *func)
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

const char *CONFIG_AnalogNumToName(int32_t func)
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
    default: break;
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

void CONFIG_SetDefaults(void)
{
    // JBF 20031211
    int32_t i,f;
    uint8_t k1,k2;

    ScreenMode = 1;
    ScreenWidth = 640;
    ScreenHeight = 480;
    ScreenBPP = 8;
    FXDevice = 0;
    MusicDevice = 0;
    NumVoices = 32;
    NumChannels = 2;
    NumBits = 16;
    MixRate = 44100;
    memcpy(&gs, &gs_defaults, sizeof(gs));

    Bstrcpy(RTSName, DEFAULTRTSFILE);
    Bstrcpy(CommPlayerName, DEFAULTPLAYERNAME);

    Bstrcpy(WangBangMacro[0], MACRO1);
    Bstrcpy(WangBangMacro[1], MACRO2);
    Bstrcpy(WangBangMacro[2], MACRO3);
    Bstrcpy(WangBangMacro[3], MACRO4);
    Bstrcpy(WangBangMacro[4], MACRO5);
    Bstrcpy(WangBangMacro[5], MACRO6);
    Bstrcpy(WangBangMacro[6], MACRO7);
    Bstrcpy(WangBangMacro[7], MACRO8);
    Bstrcpy(WangBangMacro[8], MACRO9);
    Bstrcpy(WangBangMacro[9], MACRO10);

    SetDefaultKeyDefinitions(0);
    SetMouseDefaults(0);

    memset(MouseDigitalAxes, -1, sizeof(MouseDigitalAxes));
    for (i=0; i<MAXMOUSEAXES; i++)
    {
        MouseAnalogScale[i] = 65536;

        MouseDigitalAxes[i][0] = CONFIG_FunctionNameToNum(mousedigitaldefaults[i*2]);
        MouseDigitalAxes[i][1] = CONFIG_FunctionNameToNum(mousedigitaldefaults[i*2+1]);

        MouseAnalogAxes[i] = CONFIG_AnalogNameToNum(mouseanalogdefaults[i]);
    }
    CONTROL_SetMouseSensitivity(gs.MouseSpeed);

    memset(JoystickButtons, -1, sizeof(JoystickButtons));
    memset(JoystickButtonsClicked, -1, sizeof(JoystickButtonsClicked));
    for (i=0; i < (int32_t)(sizeof(joystickdefaults)/sizeof(char *)); i++)
    {
        JoystickButtons[i] = CONFIG_FunctionNameToNum(joystickdefaults[i]);
        JoystickButtonsClicked[i] = CONFIG_FunctionNameToNum(joystickclickeddefaults[i]);
    }

    memset(JoystickDigitalAxes, -1, sizeof(JoystickDigitalAxes));
    for (i=0; i < (int32_t)(sizeof(joystickanalogdefaults)/sizeof(char *)); i++)
    {
        JoystickAnalogScale[i] = 65536;
        JoystickAnalogDead[i] = 1024;
        JoystickAnalogSaturate[i] = 32767-1024;

        JoystickDigitalAxes[i][0] = CONFIG_FunctionNameToNum(joystickdigitaldefaults[i*2]);
        JoystickDigitalAxes[i][1] = CONFIG_FunctionNameToNum(joystickdigitaldefaults[i*2+1]);

        JoystickAnalogAxes[i] = CONFIG_AnalogNameToNum(joystickanalogdefaults[i]);
    }
}


void SetDefaultKeyDefinitions(int style)
{
    int numkeydefaults;
    char **keydefaultset;
    int i, f, k1, k2;

    if (style)
    {
        numkeydefaults = sizeof(keydefaults_modern) / sizeof(char *) / 3;
        keydefaultset = keydefaults_modern;
    }
    else
    {
        numkeydefaults = sizeof(keydefaults) / sizeof(char *) / 3;
        keydefaultset = keydefaults;
    }

    memset(KeyboardKeys, 0xff, sizeof(KeyboardKeys));
    for (i=0; i < numkeydefaults; i++)
    {
        f = CONFIG_FunctionNameToNum(keydefaultset[3*i+0]);
        if (f == -1) continue;
        k1 = KB_StringToScanCode(keydefaultset[3*i+1]);
        k2 = KB_StringToScanCode(keydefaultset[3*i+2]);

        CONTROL_MapKey(i, k1, k2);

        KeyboardKeys[f][0] = k1;
        KeyboardKeys[f][1] = k2;
    }
}

void SetMouseDefaults(int style)
{
    int nummousedefaults;
    char **mousedefaultset, **mouseclickeddefaultset;
    int i;

    if (style)
    {
        nummousedefaults = sizeof(mousedefaults_modern) / sizeof(char *);
        mousedefaultset = mousedefaults_modern;
        mouseclickeddefaultset = mouseclickeddefaults_modern;
    }
    else
    {
        nummousedefaults = sizeof(mousedefaults) / sizeof(char *);
        mousedefaultset = mousedefaults;
        mouseclickeddefaultset = mouseclickeddefaults;
    }

    memset(MouseButtons, -1, sizeof(MouseButtons));
    memset(MouseButtonsClicked, -1, sizeof(MouseButtonsClicked));
    for (i=0; i < nummousedefaults; i++)
    {
        MouseButtons[i] = CONFIG_FunctionNameToNum(mousedefaultset[i]);
        CONTROL_MapButton(MouseButtons[i], i, FALSE, controldevice_mouse);

        if (i<4) continue;

        MouseButtonsClicked[i] = CONFIG_FunctionNameToNum(mouseclickeddefaultset[i]);
        CONTROL_MapButton(MouseButtonsClicked[i], i, TRUE,  controldevice_mouse);
    }
}

/*
===================
=
= CONFIG_ReadKeys
=
===================
*/

void CONFIG_ReadKeys(int32_t scripthandle)
{
    int32_t i;
    int32_t numkeyentries;
    int32_t function;
    char keyname1[80];
    char keyname2[80];
    kb_scancode key1,key2;

    if (scripthandle < 0) return;

    numkeyentries = SCRIPT_NumberEntries(scripthandle,"KeyDefinitions");

    for (i=0; i<numkeyentries; i++)
    {
        function = CONFIG_FunctionNameToNum(SCRIPT_Entry(scripthandle,"KeyDefinitions", i));
        if (function != -1)
        {
            memset(keyname1,0,sizeof(keyname1));
            memset(keyname2,0,sizeof(keyname2));
            SCRIPT_GetDoubleString
            (
                scripthandle,
                "KeyDefinitions",
                SCRIPT_Entry(scripthandle, "KeyDefinitions", i),
                keyname1,
                keyname2
            );
            key1 = 0xff;
            key2 = 0xff;
            if (keyname1[0])
            {
                key1 = (uint8_t) KB_StringToScanCode(keyname1);
            }
            if (keyname2[0])
            {
                key2 = (uint8_t) KB_StringToScanCode(keyname2);
            }
            KeyboardKeys[function][0] = key1;
            KeyboardKeys[function][1] = key2;
        }
    }

    for (i=0; i<NUMGAMEFUNCTIONS; i++)
    {
        if (i == gamefunc_Show_Console)
            OSD_CaptureKey(KeyboardKeys[i][0]);
        else
            CONTROL_MapKey(i, KeyboardKeys[i][0], KeyboardKeys[i][1]);
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
    int32_t i;
    char str[80],*p;
    char temp[80];
    int32_t function, scale;

    if (scripthandle < 0) return;

    for (i=0; i<MAXMOUSEBUTTONS; i++)
    {
        Bsprintf(str,"MouseButton%d",i); temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle,"Controls", str,temp))
            MouseButtons[i] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseButtonClicked%d",i); temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle,"Controls", str,temp))
            MouseButtonsClicked[i] = CONFIG_FunctionNameToNum(temp);
    }

    // map over the axes
    for (i=0; i<MAXMOUSEAXES; i++)
    {
        Bsprintf(str,"MouseAnalogAxes%d",i); temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            MouseAnalogAxes[i] = CONFIG_AnalogNameToNum(temp);

        Bsprintf(str,"MouseDigitalAxes%d_0",i); temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            MouseDigitalAxes[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseDigitalAxes%d_1",i); temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            MouseDigitalAxes[i][1] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseAnalogScale%d",i);
        scale = MouseAnalogScale[i];
        SCRIPT_GetNumber(scripthandle, "Controls", str,&scale);
        MouseAnalogScale[i] = scale;
    }

    // 0 to 65536
    SCRIPT_GetNumber(scripthandle, "Controls","MouseSensitivity",&function);
    gs.MouseSpeed = function;

    for (i=0; i<MAXMOUSEBUTTONS; i++)
    {
        CONTROL_MapButton(MouseButtons[i], i, FALSE, controldevice_mouse);
        CONTROL_MapButton(MouseButtonsClicked[i], i, TRUE,  controldevice_mouse);
    }
    for (i=0; i<MAXMOUSEAXES; i++)
    {
        CONTROL_MapAnalogAxis(i, MouseAnalogAxes[i], controldevice_mouse);
        CONTROL_MapDigitalAxis(i, MouseDigitalAxes[i][0], 0,controldevice_mouse);
        CONTROL_MapDigitalAxis(i, MouseDigitalAxes[i][1], 1,controldevice_mouse);
        CONTROL_SetAnalogAxisScale(i, MouseAnalogScale[i], controldevice_mouse);
    }

    CONTROL_SetMouseSensitivity(gs.MouseSpeed);
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
    int32_t i;
    char str[80],*p;
    char temp[80];
    int32_t function, scale;

    if (scripthandle < 0) return;

    for (i=0; i<MAXJOYBUTTONS; i++)
    {
        Bsprintf(str,"JoystickButton%d",i); temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle,"Controls", str,temp))
            JoystickButtons[i] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickButtonClicked%d",i); temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle,"Controls", str,temp))
            JoystickButtonsClicked[i] = CONFIG_FunctionNameToNum(temp);
    }

    // map over the axes
    for (i=0; i<MAXJOYAXES; i++)
    {
        Bsprintf(str,"JoystickAnalogAxes%d",i); temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            JoystickAnalogAxes[i] = CONFIG_AnalogNameToNum(temp);

        Bsprintf(str,"JoystickDigitalAxes%d_0",i); temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            JoystickDigitalAxes[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickDigitalAxes%d_1",i); temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            JoystickDigitalAxes[i][1] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickAnalogScale%d",i);
        scale = JoystickAnalogScale[i];
        SCRIPT_GetNumber(scripthandle, "Controls", str,&scale);
        JoystickAnalogScale[i] = scale;

        Bsprintf(str,"JoystickAnalogDead%d",i);
        scale = JoystickAnalogDead[i];
        SCRIPT_GetNumber(scripthandle, "Controls", str,&scale);
        JoystickAnalogDead[i] = scale;

        Bsprintf(str,"JoystickAnalogSaturate%d",i);
        scale = JoystickAnalogSaturate[i];
        SCRIPT_GetNumber(scripthandle, "Controls", str,&scale);
        JoystickAnalogSaturate[i] = scale;
    }

    for (i=0; i<MAXJOYBUTTONS; i++)
    {
        CONTROL_MapButton(JoystickButtons[i], i, FALSE, controldevice_joystick);
        CONTROL_MapButton(JoystickButtonsClicked[i], i, TRUE,  controldevice_joystick);
    }
    for (i=0; i<MAXJOYAXES; i++)
    {
        CONTROL_MapAnalogAxis(i, JoystickAnalogAxes[i], controldevice_joystick);
        CONTROL_MapDigitalAxis(i, JoystickDigitalAxes[i][0], 0, controldevice_joystick);
        CONTROL_MapDigitalAxis(i, JoystickDigitalAxes[i][1], 1, controldevice_joystick);
        CONTROL_SetAnalogAxisScale(i, JoystickAnalogScale[i], controldevice_joystick);
        CONTROL_SetJoyAxisDead(i, JoystickAnalogDead[i]);
        CONTROL_SetJoyAxisSaturate(i, JoystickAnalogSaturate[i]);
    }
}

/*
===================
=
= CONFIG_ReadSetup
=
===================
*/

int32_t CONFIG_ReadSetup(void)
{
    int32_t dummy;
    char ret;
    extern char ds[];
    extern char PlayerNameArg[32];

    char waveformtrackname[MAXWAVEFORMTRACKLENGTH] = {0};

    CONTROL_ClearAssignments();
    CONFIG_SetDefaults();

    if (SafeFileExists(setupfilename))
        scripthandle = SCRIPT_Load(setupfilename);

    if (scripthandle < 0) return -1;

    SCRIPT_GetNumber(scripthandle, "Screen Setup", "ScreenMode",&ScreenMode);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "ScreenWidth",&ScreenWidth);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "ScreenHeight",&ScreenHeight);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "ScreenBPP", &ScreenBPP);
    if (ScreenBPP < 8) ScreenBPP = 8;

#ifdef RENDERTYPEWIN
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "MaxRefreshFreq", (int32_t *)&maxrefreshfreq);
#endif

    SCRIPT_GetNumber(scripthandle, "Screen Setup", "GLTextureMode", &gltexfiltermode);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "GLAnisotropy", &glanisotropy);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "GLUseTextureCompr", &glusetexcompr);

    SCRIPT_GetNumber(scripthandle, "Sound Setup", "FXDevice",&FXDevice);
    SCRIPT_GetNumber(scripthandle, "Sound Setup", "MusicDevice",&MusicDevice);
    SCRIPT_GetNumber(scripthandle, "Sound Setup", "FXVolume",&dummy);
    gs.SoundVolume = dummy;
    SCRIPT_GetNumber(scripthandle, "Sound Setup", "MusicVolume",&dummy);
    gs.MusicVolume = dummy;

    SCRIPT_GetNumber(scripthandle, "Sound Setup", "NumVoices",&NumVoices);
    SCRIPT_GetNumber(scripthandle, "Sound Setup", "NumChannels",&NumChannels);
    SCRIPT_GetNumber(scripthandle, "Sound Setup", "NumBits",&NumBits);
    SCRIPT_GetNumber(scripthandle, "Sound Setup", "MixRate",&MixRate);
    SCRIPT_GetNumber(scripthandle, "Sound Setup", "ReverseStereo",&dummy);
    gs.FlipStereo = dummy;
    if (gs.FlipStereo) gs.FlipStereo = 1;

    SCRIPT_GetString(scripthandle, "Sound Setup", "WaveformTrackName", waveformtrackname);
    if (waveformtrackname[0] != '\0')
        memcpy(gs.WaveformTrackName, waveformtrackname, MAXWAVEFORMTRACKLENGTH);

    SCRIPT_GetNumber(scripthandle, "Setup", "ForceSetup",&ForceSetup);
    SCRIPT_GetNumber(scripthandle, "Controls","UseMouse",&UseMouse);
    SCRIPT_GetNumber(scripthandle, "Controls","UseJoystick",&UseJoystick);
    SCRIPT_GetString(scripthandle, "Comm Setup", "RTSName",RTSName);

    SCRIPT_GetString(scripthandle, "Comm Setup","PlayerName",CommPlayerName);

    ReadGameSetup(scripthandle);

    CONFIG_ReadKeys(scripthandle);

    //CONFIG_SetupMouse(scripthandle);
    //CONFIG_SetupJoystick(scripthandle);

    if (PlayerNameArg[0] != '\0')
    {
        strcpy(CommPlayerName, PlayerNameArg);
    }
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
    int32_t dummy;
    char buf[80];

    if (scripthandle < 0)
        scripthandle = SCRIPT_Init(setupfilename);

    SCRIPT_PutNumber(scripthandle, "Screen Setup", "ScreenWidth", ScreenWidth,FALSE,FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "ScreenHeight",ScreenHeight,FALSE,FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "ScreenMode",ScreenMode,FALSE,FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "ScreenBPP",ScreenBPP,FALSE,FALSE);
#ifdef RENDERTYPEWIN
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "MaxRefreshFreq",maxrefreshfreq,FALSE,FALSE);
#endif
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "GLTextureMode",gltexfiltermode,FALSE,FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "GLAnisotropy",glanisotropy,FALSE,FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "GLUseTextureCompr",glusetexcompr,FALSE,FALSE);

    SCRIPT_PutNumber(scripthandle, "Sound Setup", "FXDevice", FXDevice, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Sound Setup", "MusicDevice", MusicDevice, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Sound Setup", "NumVoices", NumVoices, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Sound Setup", "NumChannels", NumChannels, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Sound Setup", "NumBits", NumBits, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Sound Setup", "MixRate", MixRate, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Sound Setup", "FXVolume",gs.SoundVolume,FALSE,FALSE);
    SCRIPT_PutNumber(scripthandle, "Sound Setup", "MusicVolume",gs.MusicVolume,FALSE,FALSE);
    dummy = gs.FlipStereo;
    SCRIPT_PutNumber(scripthandle, "Sound Setup", "ReverseStereo",dummy,FALSE,FALSE);
    SCRIPT_PutString(scripthandle, "Sound Setup", "WaveformTrackName", gs.WaveformTrackName);

    SCRIPT_PutNumber(scripthandle, "Setup", "ForceSetup",ForceSetup,FALSE,FALSE);
    SCRIPT_PutNumber(scripthandle, "Controls","UseMouse",UseMouse,FALSE,FALSE);
    SCRIPT_PutNumber(scripthandle, "Controls","UseJoystick",UseJoystick,FALSE,FALSE);
    SCRIPT_PutNumber(scripthandle, "Controls","MouseSensitivity",gs.MouseSpeed,FALSE,FALSE);

    WriteGameSetup(scripthandle);

    for (dummy=0; dummy<NUMGAMEFUNCTIONS; dummy++)
    {
        SCRIPT_PutDoubleString(scripthandle, "KeyDefinitions", CONFIG_FunctionNumToName(dummy),
                               KB_ScanCodeToString(KeyboardKeys[dummy][0]), KB_ScanCodeToString(KeyboardKeys[dummy][1]));
    }

    for (dummy=0; dummy<MAXMOUSEBUTTONS; dummy++)
    {
        Bsprintf(buf,"MouseButton%d",dummy);
        SCRIPT_PutString(scripthandle,"Controls", buf, CONFIG_FunctionNumToName(MouseButtons[dummy]));

        if (dummy >= (MAXMOUSEBUTTONS-2)) continue; // scroll wheel

        Bsprintf(buf,"MouseButtonClicked%d",dummy);
        SCRIPT_PutString(scripthandle,"Controls", buf, CONFIG_FunctionNumToName(MouseButtonsClicked[dummy]));
    }

    for (dummy=0; dummy<MAXMOUSEAXES; dummy++)
    {
        Bsprintf(buf,"MouseAnalogAxes%d",dummy);
        SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_AnalogNumToName(MouseAnalogAxes[dummy]));

        Bsprintf(buf,"MouseDigitalAxes%d_0",dummy);
        SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(MouseDigitalAxes[dummy][0]));

        Bsprintf(buf,"MouseDigitalAxes%d_1",dummy);
        SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(MouseDigitalAxes[dummy][1]));

        Bsprintf(buf,"MouseAnalogScale%d",dummy);
        SCRIPT_PutNumber(scripthandle, "Controls", buf, MouseAnalogScale[dummy], FALSE, FALSE);
    }

    for (dummy=0; dummy<MAXJOYBUTTONS; dummy++)
    {
        Bsprintf(buf,"JoystickButton%d",dummy);
        SCRIPT_PutString(scripthandle,"Controls", buf, CONFIG_FunctionNumToName(JoystickButtons[dummy]));

        Bsprintf(buf,"JoystickButtonClicked%d",dummy);
        SCRIPT_PutString(scripthandle,"Controls", buf, CONFIG_FunctionNumToName(JoystickButtonsClicked[dummy]));
    }

    for (dummy=0; dummy<MAXJOYAXES; dummy++)
    {
        Bsprintf(buf,"JoystickAnalogAxes%d",dummy);
        SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_AnalogNumToName(JoystickAnalogAxes[dummy]));

        Bsprintf(buf,"JoystickDigitalAxes%d_0",dummy);
        SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickDigitalAxes[dummy][0]));

        Bsprintf(buf,"JoystickDigitalAxes%d_1",dummy);
        SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickDigitalAxes[dummy][1]));

        Bsprintf(buf,"JoystickAnalogScale%d",dummy);
        SCRIPT_PutNumber(scripthandle, "Controls", buf, JoystickAnalogScale[dummy], FALSE, FALSE);

        Bsprintf(buf,"JoystickAnalogDead%d",dummy);
        SCRIPT_PutNumber(scripthandle, "Controls", buf, JoystickAnalogDead[dummy], FALSE, FALSE);

        Bsprintf(buf,"JoystickAnalogSaturate%d",dummy);
        SCRIPT_PutNumber(scripthandle, "Controls", buf, JoystickAnalogSaturate[dummy], FALSE, FALSE);
    }

    SCRIPT_Save(scripthandle, setupfilename);
    SCRIPT_Free(scripthandle);
}

