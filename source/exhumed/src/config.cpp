#include "compat.h"
#include "renderlayer.h"
#include "build.h"
#include "cache1d.h"
#include "keyboard.h"
#include "control.h"
#include "exhumed.h"
#include "typedefs.h"
#include "scriplib.h"

#include "config.h"

#include <string>
#include <io.h>
#include <stdio.h>
#include <time.h>

#define kMaxGameFunctions	40
#define kMaxGameFuncLen     64

// KEEPINSYNC mact/include/_control.h, build/src/sdlayer.cpp
#define MAXJOYBUTTONS 32
#define MAXJOYBUTTONSANDHATS (MAXJOYBUTTONS+4)

// KEEPINSYNC mact/include/_control.h, build/src/sdlayer.cpp
#define MAXMOUSEAXES 2
#define MAXMOUSEDIGITAL (MAXMOUSEAXES*2)

// KEEPINSYNC mact/include/_control.h, build/src/sdlayer.cpp
#define MAXJOYAXES 9
#define MAXJOYDIGITAL (MAXJOYAXES*2)

// default mouse scale
#define DEFAULTMOUSEANALOGUESCALE           65536

// default joystick settings

#if defined(GEKKO)
#define DEFAULTJOYSTICKANALOGUESCALE        16384
#define DEFAULTJOYSTICKANALOGUEDEAD         1000
#define DEFAULTJOYSTICKANALOGUESATURATE     9500
#else
#define DEFAULTJOYSTICKANALOGUESCALE        65536
#define DEFAULTJOYSTICKANALOGUEDEAD         2000
#define DEFAULTJOYSTICKANALOGUESATURATE     9500
#endif

static const char gamefunctions[kMaxGameFunctions][kMaxGameFuncLen] =
{
  "Move_Forward",
  "Move_Backward",
  "Turn_Left",
  "Turn_Right",
  "Strafe",
  "Strafe_Left",
  "Strafe_Right",
  "Run",
  "Jump",
  "Crouch",
  "Fire",
  "Open",
  "Look_Up",
  "Look_Down",
  "Look_Straight",
  "Aim_Up",
  "Aim_Down",
  "SendMessage",
  "Weapon_1",
  "Weapon_2",
  "Weapon_3",
  "Weapon_4",
  "Weapon_5",
  "Weapon_6",
  "Weapon_7",
  "Mouseview",
  "Pause",
  "Map",
  "Zoom_In",
  "Zoom_Out",
  "Gamma_Correction",
  "Escape",
  "Shrink_Screen",
  "Enlarge_Screen",
  "Inventory",
  "Inventory_Left",
  "Inventory_Right",
  "Mouse_Sensitivity_Up",
  "Mouse_Sensitivity_Down"
  "Show_Console",
};

static const char keydefaults[kMaxGameFunctions * 2][kMaxGameFuncLen] =
{
   "Up", "Kpad8",
   "Down", "Kpad2",
   "Left", "Kpad4",
   "Right", "KPad6",
   "LAlt", "RAlt",
   ",", "",
   ".", "",
   "LShift", "RShift",
   "A", "",
   "Z", "",
   "LCtrl", "RCtrl",
   "Space", "",
   "PgUp", "",
   "PgDn", "",
   "Home", "",
   "Insert", "",
   "Delete", "",
   "T", "",
   "1", "",
   "2", "",
   "3", "",
   "4", "",
   "5", "",
   "6", "",
   "7", "",
   "/", "",
   "Pause", "",
   "Tab", "",
   "=", "",
   "-", "",
   "F11", "",
   "Escape", "",
   "Kpad-", "",
   "Kpad+", "",
   "Enter", "",
   "[", "",
   "]", "",
   "F7", "",
   "F8", "",
   "`", "",
};

static const char *mousedefaults[MAXMOUSEBUTTONS] =
{
   "Fire",
   "Strafe",
   "Move_Forward"
   "",
   "",
   "",
};

static const char *mouseclickeddefaults[MAXMOUSEBUTTONS] =
{
};

static const char* mouseanalogdefaults[MAXMOUSEAXES] =
{
"analog_strafing",
"analog_moving",
};


static const char* mousedigitaldefaults[MAXMOUSEDIGITAL] =
{
};

ud_setup_t gSetup;

#define kMaxSetupFiles		20
#define kSetupFilename		"SETUP.CFG"

static char setupfilename[128] = {kSetupFilename};

int lMouseSens = 32;
unsigned int dword_1B82E0 = 0;

int32_t FXVolume;
int32_t MusicVolume;
int32_t MixRate;
int32_t MidiPort;
int32_t NumVoices;
int32_t NumChannels;
int32_t NumBits;
int32_t ReverseStereo;
int32_t MusicDevice;
int32_t FXDevice;
int32_t ControllerType;

int32_t scripthandle;
int32_t setupread;
int32_t useprecache;
int32_t MouseDeadZone, MouseBias;
int32_t SmoothInput;

// JBF 20031211: Store the input settings because
// (currently) mact can't regurgitate them
int32_t MouseFunctions[MAXMOUSEBUTTONS][2];
int32_t MouseDigitalFunctions[MAXMOUSEAXES][2];
int32_t MouseAnalogueAxes[MAXMOUSEAXES];
int32_t MouseAnalogueScale[MAXMOUSEAXES];
int32_t JoystickFunctions[MAXJOYBUTTONSANDHATS][2];
int32_t JoystickDigitalFunctions[MAXJOYAXES][2];
int32_t JoystickAnalogueAxes[MAXJOYAXES];
int32_t JoystickAnalogueScale[MAXJOYAXES];
int32_t JoystickAnalogueInvert[MAXJOYAXES];
int32_t JoystickAnalogueDead[MAXJOYAXES];
int32_t JoystickAnalogueSaturate[MAXJOYAXES];
uint8_t KeyboardKeys[kMaxGameFunctions][2];

int32_t MAXCACHE1DSIZE = (96*1024*1024);


void SetupGameButtons()
{
	CONTROL_DefineFlag(gamefunc_Move_Forward,			FALSE);
	CONTROL_DefineFlag(gamefunc_Move_Backward,			FALSE);
	CONTROL_DefineFlag(gamefunc_Turn_Left,				FALSE);
	CONTROL_DefineFlag(gamefunc_Turn_Right,				FALSE);
	CONTROL_DefineFlag(gamefunc_Strafe,					FALSE);
	CONTROL_DefineFlag(gamefunc_Strafe_Left,			FALSE);
	CONTROL_DefineFlag(gamefunc_Strafe_Right,			FALSE);
	CONTROL_DefineFlag(gamefunc_Jump,					FALSE);
	CONTROL_DefineFlag(gamefunc_Crouch,					FALSE);
	CONTROL_DefineFlag(gamefunc_Fire,					FALSE);
	CONTROL_DefineFlag(gamefunc_Open,					FALSE);
	CONTROL_DefineFlag(gamefunc_Aim_Up,					FALSE);
	CONTROL_DefineFlag(gamefunc_Aim_Down,				FALSE);
	CONTROL_DefineFlag(gamefunc_Look_Up,				FALSE);
	CONTROL_DefineFlag(gamefunc_Look_Down,				FALSE);
	CONTROL_DefineFlag(gamefunc_Look_Straight,			FALSE);
	CONTROL_DefineFlag(gamefunc_Run,					FALSE);
	CONTROL_DefineFlag(gamefunc_SendMessage,			FALSE);
	CONTROL_DefineFlag(gamefunc_Weapon_1,				FALSE);
	CONTROL_DefineFlag(gamefunc_Weapon_2,				FALSE);
	CONTROL_DefineFlag(gamefunc_Weapon_3,				FALSE);
	CONTROL_DefineFlag(gamefunc_Weapon_4,				FALSE);
	CONTROL_DefineFlag(gamefunc_Weapon_5,				FALSE);
	CONTROL_DefineFlag(gamefunc_Weapon_6,				FALSE);
	CONTROL_DefineFlag(gamefunc_Weapon_7,				FALSE);
	CONTROL_DefineFlag(gamefunc_Pause,					FALSE);
	CONTROL_DefineFlag(gamefunc_Map,					FALSE);
	CONTROL_DefineFlag(gamefunc_Gamma_Correction,		FALSE);
	CONTROL_DefineFlag(gamefunc_Escape,					FALSE);
	CONTROL_DefineFlag(gamefunc_Shrink_Screen,			FALSE);
	CONTROL_DefineFlag(gamefunc_Enlarge_Screen,			FALSE);
	CONTROL_DefineFlag(gamefunc_Zoom_In,				FALSE);
	CONTROL_DefineFlag(gamefunc_Zoom_Out,				FALSE);
	CONTROL_DefineFlag(gamefunc_Inventory_Left,			FALSE);
	CONTROL_DefineFlag(gamefunc_Inventory_Right,		FALSE);
	CONTROL_DefineFlag(gamefunc_Mouseview,				FALSE);
	CONTROL_DefineFlag(gamefunc_Inventory,				FALSE);
	CONTROL_DefineFlag(gamefunc_Mouse_Sensitivity_Up,	FALSE);
	CONTROL_DefineFlag(gamefunc_Mouse_Sensitivity_Down,	FALSE);
}

hashtable_t h_gamefuncs    = { kMaxGameFunctions<<1, NULL };

int32_t CONFIG_FunctionNameToNum(const char *func)
{
    if (!func)
        return -1;

    return hash_findcase(&h_gamefuncs, func);
}


static char const * CONFIG_FunctionNumToName(int32_t func)
{
    if ((unsigned)func >= (unsigned)kMaxGameFunctions)
        return "";
    return gamefunctions[func];
}


int32_t CONFIG_AnalogNameToNum(const char *func)
{
    if (!func)
        return -1;
    else if (!Bstrcasecmp(func, "analog_turning"))
        return analog_turning;
    else if (!Bstrcasecmp(func, "analog_strafing"))
        return analog_strafing;
    else if (!Bstrcasecmp(func, "analog_moving"))
        return analog_moving;
    else if (!Bstrcasecmp(func, "analog_lookingupanddown"))
        return analog_lookingupanddown;
    else
        return -1;
}


static char const * CONFIG_AnalogNumToName(int32_t func)
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

    return "";
}


static void CONFIG_SetJoystickButtonFunction(int i, int j, int function)
{
    JoystickFunctions[i][j] = function;
    CONTROL_MapButton(function, i, j, controldevice_joystick);
}
static void CONFIG_SetJoystickAnalogAxisScale(int i, int scale)
{
    JoystickAnalogueScale[i] = scale;
    CONTROL_SetAnalogAxisScale(i, scale, controldevice_joystick);
}
static void CONFIG_SetJoystickAnalogAxisInvert(int i, int invert)
{
    JoystickAnalogueInvert[i] = invert;
    CONTROL_SetAnalogAxisInvert(i, invert, controldevice_joystick);
}
static void CONFIG_SetJoystickAnalogAxisDeadSaturate(int i, int dead, int saturate)
{
    JoystickAnalogueDead[i] = dead;
    JoystickAnalogueSaturate[i] = saturate;
    joySetDeadZone(i, dead, saturate);
}
static void CONFIG_SetJoystickDigitalAxisFunction(int i, int j, int function)
{
    JoystickDigitalFunctions[i][j] = function;
    CONTROL_MapDigitalAxis(i, function, j, controldevice_joystick);
}
static void CONFIG_SetJoystickAnalogAxisFunction(int i, int function)
{
    JoystickAnalogueAxes[i] = function;
    CONTROL_MapAnalogAxis(i, function, controldevice_joystick);
}


void CONFIG_SetDefaultKeys(const char (*keyptr)[kMaxGameFunctions], bool lazy/*=false*/)
{
    static char const s_gamefunc_[] = "gamefunc_";
    int constexpr strlen_gamefunc_  = ARRAY_SIZE(s_gamefunc_) - 1;

    if (!lazy)
    {
        Bmemset(KeyboardKeys, 0xff, sizeof(KeyboardKeys));
        CONTROL_ClearAllBinds();
    }

    for (int i=0; i < ARRAY_SSIZE(gamefunctions); ++i)
    {
        if (gamefunctions[i][0] == '\0')
            continue;

        auto &key = KeyboardKeys[i];

        int const default0 = KB_StringToScanCode(keyptr[i<<1]);
        int const default1 = KB_StringToScanCode(keyptr[(i<<1)+1]);

        // skip the function if the default key is already used
        // or the function is assigned to another key
        if (lazy && (key[0] != 0xff || (CONTROL_KeyIsBound(default0) && Bstrlen(CONTROL_KeyBinds[default0].cmdstr) > strlen_gamefunc_
                        && CONFIG_FunctionNameToNum(CONTROL_KeyBinds[default0].cmdstr + strlen_gamefunc_) >= 0)))
        {
#if 0 // defined(DEBUGGINGAIDS)
            if (key[0] != 0xff)
                initprintf("Skipping %s bound to %s\n", keyptr[i<<1], CONTROL_KeyBinds[default0].cmdstr);
#endif
            continue;
        }

        key[0] = default0;
        key[1] = default1;

        if (key[0])
            CONTROL_FreeKeyBind(key[0]);

        if (key[1])
            CONTROL_FreeKeyBind(key[1]);

        if (i == gamefunc_Show_Console)
            OSD_CaptureKey(key[0]);
        else
            CONFIG_MapKey(i, key[0], 0, key[1], 0);
    }
}

void CONFIG_SetDefaults()
{
	FXVolume       = 128;
	MusicVolume    = 128;
	ReverseStereo  = 0;
	ControllerType = controltype_keyboardandmouse;
	lMouseSens     = 8;
}

int CONFIG_ReadSetup()
{
    char tempbuf[1024];

    CONTROL_ClearAssignments();
	CONFIG_SetDefaults();

    setupread = 1;
    pathsearchmode = 1;

    if (scripthandle < 0)
    {
        if (buildvfs_exists(setupfilename))  // JBF 20031211
            scripthandle = SCRIPT_Load(setupfilename);
        else if (buildvfs_exists(kSetupFilename))
        {
            int const i = wm_ynbox("Import Configuration Settings",
                "The configuration file \"%s\" was not found. "
                "Import configuration data from \"%s\"?",
                setupfilename, kSetupFilename);
            if (i)
                scripthandle = SCRIPT_Load(kSetupFilename);
        }
    }

    pathsearchmode = 0;

    if (scripthandle < 0)
        return -1;

    SCRIPT_GetNumber(scripthandle, "Setup", "ForceSetup", &gSetup.forcesetup);
    SCRIPT_GetNumber(scripthandle, "Setup", "NoAutoLoad", &gSetup.noautoload);

    int32_t cachesize;
    SCRIPT_GetNumber(scripthandle, "Setup", "CacheSize", &cachesize);

    if (cachesize > MAXCACHE1DSIZE)
        MAXCACHE1DSIZE = cachesize;
    

    // TODO:
    if (/*g_noSetup == 0 && */g_modDir[0] == '/')
    {
        SCRIPT_GetString(scripthandle, "Setup","ModDir",&g_modDir[0]);

        if (!buildvfs_isdir(g_modDir))
        {
            initprintf("Invalid mod dir in cfg!\n");
            Bsprintf(g_modDir,"/");
        }
    }

    windowx = -1;
    windowy = -1;

    SCRIPT_GetNumber(scripthandle, "Screen Setup", "MaxRefreshFreq", (int32_t *)&maxrefreshfreq);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "ScreenBPP", &gSetup.bpp);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "ScreenHeight", &gSetup.ydim);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "ScreenMode", &gSetup.fullscreen);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "ScreenWidth", &gSetup.xdim);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "WindowPosX", (int32_t *)&windowx);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "WindowPosY", (int32_t *)&windowy);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "WindowPositioning", (int32_t *)&windowpos);

    if (gSetup.bpp < 8) gSetup.bpp = 32;

    setupread = 1;
}


void CONFIG_SetupMouse(void)
{
    if (scripthandle < 0)
        return;

    char str[80];
    char temp[80];

    for (int i=0; i<MAXMOUSEBUTTONS; i++)
    {
        Bsprintf(str,"MouseButton%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle,"Controls", str,temp))
            MouseFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseButtonClicked%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle,"Controls", str,temp))
            MouseFunctions[i][1] = CONFIG_FunctionNameToNum(temp);
    }

    // map over the axes
    for (int i=0; i<MAXMOUSEAXES; i++)
    {
        Bsprintf(str,"MouseAnalogAxes%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            MouseAnalogueAxes[i] = CONFIG_AnalogNameToNum(temp);

        Bsprintf(str,"MouseDigitalAxes%d_0",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            MouseDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseDigitalAxes%d_1",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            MouseDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseAnalogScale%d",i);
        int32_t scale = ud.config.MouseAnalogueScale[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str, &scale);
        ud.config.MouseAnalogueScale[i] = scale;
    }

    for (int i=0; i<MAXMOUSEBUTTONS; i++)
    {
        CONTROL_MapButton(ud.config.MouseFunctions[i][0], i, 0, controldevice_mouse);
        CONTROL_MapButton(ud.config.MouseFunctions[i][1], i, 1,  controldevice_mouse);
    }
    for (int i=0; i<MAXMOUSEAXES; i++)
    {
        CONTROL_MapAnalogAxis(i, ud.config.MouseAnalogueAxes[i], controldevice_mouse);
        CONTROL_MapDigitalAxis(i, ud.config.MouseDigitalFunctions[i][0], 0,controldevice_mouse);
        CONTROL_MapDigitalAxis(i, ud.config.MouseDigitalFunctions[i][1], 1,controldevice_mouse);
        CONTROL_SetAnalogAxisScale(i, ud.config.MouseAnalogueScale[i], controldevice_mouse);
    }
}


void CONFIG_SetupJoystick(void)
{
    int32_t i;
    char str[80];
    char temp[80];
    int32_t scale;

    if (scripthandle < 0) return;

    for (i=0; i<MAXJOYBUTTONSANDHATS; i++)
    {
        Bsprintf(str,"ControllerButton%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle,"Controls", str,temp))
            ud.config.JoystickFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"ControllerButtonClicked%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle,"Controls", str,temp))
            ud.config.JoystickFunctions[i][1] = CONFIG_FunctionNameToNum(temp);
    }

    // map over the axes
    for (i=0; i<MAXJOYAXES; i++)
    {
        Bsprintf(str,"ControllerAnalogAxes%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            JoystickAnalogueAxes[i] = CONFIG_AnalogNameToNum(temp);

        Bsprintf(str,"ControllerDigitalAxes%d_0",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            JoystickDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"ControllerDigitalAxes%d_1",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            ud.config.JoystickDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"ControllerAnalogScale%d",i);
        scale = ud.config.JoystickAnalogueScale[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        ud.config.JoystickAnalogueScale[i] = scale;

        Bsprintf(str,"ControllerAnalogInvert%d",i);
        scale = ud.config.JoystickAnalogueInvert[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        ud.config.JoystickAnalogueInvert[i] = scale;

        Bsprintf(str,"ControllerAnalogDead%d",i);
        scale = ud.config.JoystickAnalogueDead[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        ud.config.JoystickAnalogueDead[i] = scale;

        Bsprintf(str,"ControllerAnalogSaturate%d",i);
        scale = ud.config.JoystickAnalogueSaturate[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        ud.config.JoystickAnalogueSaturate[i] = scale;
    }

    for (i=0; i<MAXJOYBUTTONSANDHATS; i++)
    {
        CONTROL_MapButton(ud.config.JoystickFunctions[i][0], i, 0, controldevice_joystick);
        CONTROL_MapButton(ud.config.JoystickFunctions[i][1], i, 1,  controldevice_joystick);
    }
    for (i=0; i<MAXJOYAXES; i++)
    {
        CONTROL_MapAnalogAxis(i, ud.config.JoystickAnalogueAxes[i], controldevice_joystick);
        CONTROL_MapDigitalAxis(i, ud.config.JoystickDigitalFunctions[i][0], 0, controldevice_joystick);
        CONTROL_MapDigitalAxis(i, ud.config.JoystickDigitalFunctions[i][1], 1, controldevice_joystick);
        CONTROL_SetAnalogAxisScale(i, ud.config.JoystickAnalogueScale[i], controldevice_joystick);
        CONTROL_SetAnalogAxisInvert(i, ud.config.JoystickAnalogueInvert[i], controldevice_joystick);
    }
}

void SetupInput()
{
    if (CONTROL_Startup(controltype_keyboardandmouse, &BGetTime, kTimerTicks))
    {
        ERRprintf("There was an error initializing the CONTROL system.\n");
        engineUnInit();
        Bexit(5);
    }
	SetupGameButtons();
    CONFIG_SetupMouse();
    CONFIG_SetupJoystick();

    CONTROL_JoystickEnabled = (gSetup.usejoystick && CONTROL_JoyPresent);
    CONTROL_MouseEnabled    = (gSetup.usemouse && CONTROL_MousePresent);

    // JBF 20040215: evil and nasty place to do this, but joysticks are evil and nasty too
    for (int i=0; i<joystick.numAxes; i++)
        joySetDeadZone(i,JoystickAnalogueDead[i],JoystickAnalogueSaturate[i]);
}

void LoadConfig()
{
	CONFIG_ReadSetup();
	/*
	Joy_x = 0;
	Joy_y = 0;
	Joy_xs = 1;
	Joy_ys = 1;
	Joy_xb = 2;
	Joy_yb = 2;
	*/
}
