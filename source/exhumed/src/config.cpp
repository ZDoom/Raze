#include "compat.h"
#include "renderlayer.h"
#include "_control.h"
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

const char gamefunctions[kMaxGameFunctions][kMaxGameFuncLen] =
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

const char keydefaults[kMaxGameFunctions * 2][kMaxGameFuncLen] =
{
   "W", "Kpad8",
   "S", "Kpad2",
   "Left", "Kpad4",
   "Right", "KPad6",
   "LAlt", "RAlt",
   "A", "",
   "D", "",
   "LShift", "RShift",
   "Space", "",
   "LCtrl", "",
   "RCtrl", "",
   "E", "",
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

const char oldkeydefaults[kMaxGameFunctions * 2][kMaxGameFuncLen] =
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

char setupfilename[128] = {kSetupFilename};

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
// TODO: implement precaching toggle
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
    CONTROL_DefineFlag(gamefunc_Move_Forward,			kFalse);
    CONTROL_DefineFlag(gamefunc_Move_Backward,			kFalse);
    CONTROL_DefineFlag(gamefunc_Turn_Left,				kFalse);
    CONTROL_DefineFlag(gamefunc_Turn_Right,				kFalse);
    CONTROL_DefineFlag(gamefunc_Strafe,					kFalse);
    CONTROL_DefineFlag(gamefunc_Strafe_Left,			kFalse);
    CONTROL_DefineFlag(gamefunc_Strafe_Right,			kFalse);
    CONTROL_DefineFlag(gamefunc_Jump,					kFalse);
    CONTROL_DefineFlag(gamefunc_Crouch,					kFalse);
    CONTROL_DefineFlag(gamefunc_Fire,					kFalse);
    CONTROL_DefineFlag(gamefunc_Open,					kFalse);
    CONTROL_DefineFlag(gamefunc_Aim_Up,					kFalse);
    CONTROL_DefineFlag(gamefunc_Aim_Down,				kFalse);
    CONTROL_DefineFlag(gamefunc_Look_Up,				kFalse);
    CONTROL_DefineFlag(gamefunc_Look_Down,				kFalse);
    CONTROL_DefineFlag(gamefunc_Look_Straight,			kFalse);
    CONTROL_DefineFlag(gamefunc_Run,					kFalse);
    CONTROL_DefineFlag(gamefunc_SendMessage,			kFalse);
    CONTROL_DefineFlag(gamefunc_Weapon_1,				kFalse);
    CONTROL_DefineFlag(gamefunc_Weapon_2,				kFalse);
    CONTROL_DefineFlag(gamefunc_Weapon_3,				kFalse);
    CONTROL_DefineFlag(gamefunc_Weapon_4,				kFalse);
    CONTROL_DefineFlag(gamefunc_Weapon_5,				kFalse);
    CONTROL_DefineFlag(gamefunc_Weapon_6,				kFalse);
    CONTROL_DefineFlag(gamefunc_Weapon_7,				kFalse);
    CONTROL_DefineFlag(gamefunc_Pause,					kFalse);
    CONTROL_DefineFlag(gamefunc_Map,					kFalse);
    CONTROL_DefineFlag(gamefunc_Gamma_Correction,		kFalse);
    CONTROL_DefineFlag(gamefunc_Escape,					kFalse);
    CONTROL_DefineFlag(gamefunc_Shrink_Screen,			kFalse);
    CONTROL_DefineFlag(gamefunc_Enlarge_Screen,			kFalse);
    CONTROL_DefineFlag(gamefunc_Zoom_In,				kFalse);
    CONTROL_DefineFlag(gamefunc_Zoom_Out,				kFalse);
    CONTROL_DefineFlag(gamefunc_Inventory_Left,			kFalse);
    CONTROL_DefineFlag(gamefunc_Inventory_Right,		kFalse);
    CONTROL_DefineFlag(gamefunc_Mouseview,				kFalse);
    CONTROL_DefineFlag(gamefunc_Inventory,				kFalse);
    CONTROL_DefineFlag(gamefunc_Mouse_Sensitivity_Up,	kFalse);
    CONTROL_DefineFlag(gamefunc_Mouse_Sensitivity_Down,	kFalse);
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


void CONFIG_SetDefaultKeys(const char (*keyptr)[kMaxGameFuncLen], bool lazy/*=false*/)
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
    scripthandle = -1;
# if defined RENDERTYPESDL && SDL_MAJOR_VERSION > 1
    uint32_t inited = SDL_WasInit(SDL_INIT_VIDEO);
    if (inited == 0)
        SDL_Init(SDL_INIT_VIDEO);
    else if (!(inited & SDL_INIT_VIDEO))
        SDL_InitSubSystem(SDL_INIT_VIDEO);

    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) == 0)
    {
        gSetup.xdim = dm.w;
        gSetup.ydim = dm.h;
    }
    else
# endif
    {
        gSetup.xdim = 1024;
        gSetup.ydim = 768;
    }

#ifdef USE_OPENGL
    gSetup.bpp = 32;
#else
    gSetup.bpp = 8;
#endif

    // currently settings.cfg is only read after the startup window launches the game,
    // and rereading binds might be fickle so just enable this
    gSetup.usejoystick = 1;

    gSetup.forcesetup = 1;
    gSetup.noautoload = 1;
    gSetup.fullscreen = 1;
    gSetup.usemouse = 1;

    MixRate = 44100;
    FXVolume = 255;
    MusicVolume = 255;
    NumChannels = 2;
    NumBits = 16;
    NumVoices = 32;

    mouseaiming = 0;
    aimmode = 1;
    mouseflip = 1;
    runkey_mode = 0;
    auto_run = 1;

    CONFIG_SetDefaultKeys(keydefaults);

    memset(MouseFunctions, -1, sizeof(MouseFunctions));
    memset(MouseDigitalFunctions, -1, sizeof(MouseDigitalFunctions));
    memset(JoystickFunctions, -1, sizeof(JoystickFunctions));
    memset(JoystickDigitalFunctions, -1, sizeof(JoystickDigitalFunctions));

    CONTROL_MouseSensitivity = DEFAULTMOUSESENSITIVITY;

    for (int i=0; i<MAXMOUSEBUTTONS; i++)
    {
        MouseFunctions[i][0] = CONFIG_FunctionNameToNum(mousedefaults[i]);
        CONTROL_MapButton(MouseFunctions[i][0], i, 0, controldevice_mouse);
        if (i>=4) continue;
        MouseFunctions[i][1] = CONFIG_FunctionNameToNum(mouseclickeddefaults[i]);
        CONTROL_MapButton(MouseFunctions[i][1], i, 1, controldevice_mouse);
    }

    for (int i=0; i<MAXMOUSEAXES; i++)
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

    // TODO:
    //CONFIG_SetGameControllerDefaultsStandard();

#if 0
    FXVolume       = 128;
    MusicVolume    = 128;
    ReverseStereo  = 0;
    ControllerType = controltype_keyboardandmouse;
    lMouseSens     = 8;
#endif
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
    

    if (g_noSetup == 0 && g_modDir[0] == '/')
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

// wrapper for CONTROL_MapKey(), generates key bindings to reflect changes to keyboard setup
void CONFIG_MapKey(int which, kb_scancode key1, kb_scancode oldkey1, kb_scancode key2, kb_scancode oldkey2)
{
    char tempbuf[256];
    int const keys[] = { key1, key2, oldkey1, oldkey2 };
    char buf[2*kMaxGameFuncLen];

    if (which == gamefunc_Show_Console)
        OSD_CaptureKey(key1);

    for (int k = 0; (unsigned)k < ARRAY_SIZE(keys); k++)
    {
        if (keys[k] == 0xff || !keys[k])
            continue;

        int match = 0;

        for (; match < ARRAY_SSIZE(sctokeylut); ++match)
        {
            if (keys[k] == sctokeylut[match].sc)
                break;
        }

        tempbuf[0] = 0;

        for (int i=kMaxGameFunctions-1; i>=0; i--)
        {
            if (KeyboardKeys[i][0] == keys[k] || KeyboardKeys[i][1] == keys[k])
            {
                Bsprintf(buf, "gamefunc_%s; ", CONFIG_FunctionNumToName(i));
                Bstrcat(tempbuf,buf);
            }
        }

        int const len = Bstrlen(tempbuf);

        if (len >= 2)
        {
            tempbuf[len-2] = 0;  // cut off the trailing "; "
            CONTROL_BindKey(keys[k], tempbuf, 1, sctokeylut[match].key ? sctokeylut[match].key : "<?>");
        }
        else
        {
            CONTROL_FreeKeyBind(keys[k]);
        }
    }
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
        int32_t scale = MouseAnalogueScale[i];
        SCRIPT_GetNumber(scripthandle, "Controls", str, &scale);
        MouseAnalogueScale[i] = scale;
    }

    for (int i=0; i<MAXMOUSEBUTTONS; i++)
    {
        CONTROL_MapButton(MouseFunctions[i][0], i, 0, controldevice_mouse);
        CONTROL_MapButton(MouseFunctions[i][1], i, 1,  controldevice_mouse);
    }
    for (int i=0; i<MAXMOUSEAXES; i++)
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

    if (scripthandle < 0) return;

    for (i=0; i<MAXJOYBUTTONSANDHATS; i++)
    {
        Bsprintf(str,"ControllerButton%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle,"Controls", str,temp))
            JoystickFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"ControllerButtonClicked%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle,"Controls", str,temp))
            JoystickFunctions[i][1] = CONFIG_FunctionNameToNum(temp);
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
            JoystickDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"ControllerAnalogScale%d",i);
        scale = JoystickAnalogueScale[i];
        SCRIPT_GetNumber(scripthandle, "Controls", str,&scale);
        JoystickAnalogueScale[i] = scale;

        Bsprintf(str,"ControllerAnalogInvert%d",i);
        scale = JoystickAnalogueInvert[i];
        SCRIPT_GetNumber(scripthandle, "Controls", str,&scale);
        JoystickAnalogueInvert[i] = scale;

        Bsprintf(str,"ControllerAnalogDead%d",i);
        scale = JoystickAnalogueDead[i];
        SCRIPT_GetNumber(scripthandle, "Controls", str,&scale);
        JoystickAnalogueDead[i] = scale;

        Bsprintf(str,"ControllerAnalogSaturate%d",i);
        scale = JoystickAnalogueSaturate[i];
        SCRIPT_GetNumber(scripthandle, "Controls", str,&scale);
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
        CONTROL_SetAnalogAxisInvert(i, JoystickAnalogueInvert[i], controldevice_joystick);
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

void CONFIG_WriteSettings(void) // save binds and aliases to <cfgname>_settings.cfg
{
    char filename[BMAX_PATH];

    if (!Bstrcmp(setupfilename, kSetupFilename))
        Bsprintf(filename, "settings.cfg");
    else
        Bsprintf(filename, "%s_settings.cfg", strtok(setupfilename, "."));

    buildvfs_FILE fp = buildvfs_fopen_write(filename);

    if (fp)
    {
        buildvfs_fputstr(fp, "// this file is automatically generated by ");
        buildvfs_fputstrptr(fp, AppProperName);
        buildvfs_fputstr(fp,"\nunbindall\n");

        for (int i=0; i<MAXBOUNDKEYS+MAXMOUSEBUTTONS; i++)
        {
            if (CONTROL_KeyIsBound(i))
            {
                buildvfs_fputstr(fp, "bind \"");
                buildvfs_fputstrptr(fp, CONTROL_KeyBinds[i].key);
                if (CONTROL_KeyBinds[i].repeat)
                    buildvfs_fputstr(fp, "\" \"");
                else
                    buildvfs_fputstr(fp, "\" norepeat \"");
                buildvfs_fputstrptr(fp, CONTROL_KeyBinds[i].cmdstr);
                buildvfs_fputstr(fp, "\"\n");
            }
        }

        for (int i=0; i<kMaxGameFunctions; ++i)
        {
            char const * name = CONFIG_FunctionNumToName(i);
            if (name && name[0] != '\0' && (KeyboardKeys[i][0] == 0xff || !KeyboardKeys[i][0]))
            {
                buildvfs_fputstr(fp, "unbound ");
                buildvfs_fputstrptr(fp, name);
                buildvfs_fputstr(fp, "\n");
            }
        }

        OSD_WriteAliases(fp);

        //if (g_crosshairSum != -1 && g_crosshairSum != DefaultCrosshairColors.r+(DefaultCrosshairColors.g<<8)+(DefaultCrosshairColors.b<<16))
        //{
        //    buildvfs_fputstr(fp, "crosshaircolor ");
        //    char buf[64];
        //    snprintf(buf, sizeof(buf), "%d %d %d\n", CrosshairColors.r, CrosshairColors.g, CrosshairColors.b);
        //    buildvfs_fputstrptr(fp, buf);
        //}

        OSD_WriteCvars(fp);

        buildvfs_fclose(fp);

        OSD_Printf("Wrote %s\n", filename);

        return;
    }

    OSD_Printf("Error writing %s: %s\n", filename, strerror(errno));
}

void CONFIG_WriteSetup(uint32_t flags)
{
    char buf[256];
    if (!setupread) return;

    if (scripthandle < 0)
        scripthandle = SCRIPT_Init(setupfilename);

    SCRIPT_PutNumber(scripthandle, "Setup", "CacheSize", MAXCACHE1DSIZE, FALSE, FALSE);
    //SCRIPT_PutNumber(scripthandle, "Setup", "ConfigVersion", BYTEVERSION_EDUKE32, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Setup", "ForceSetup", gSetup.forcesetup, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Setup", "NoAutoLoad", gSetup.noautoload, FALSE, FALSE);

//#ifdef POLYMER
//    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "Polymer", glrendmode == REND_POLYMER, FALSE, FALSE);
//#endif

    SCRIPT_PutNumber(scripthandle, "Screen Setup", "ScreenBPP", gSetup.bpp, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "ScreenHeight", gSetup.ydim, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "ScreenMode", gSetup.fullscreen, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "ScreenWidth", gSetup.xdim, FALSE, FALSE);

    //if (g_grpNamePtr && !g_addonNum)
    //    SCRIPT_PutString(ud.config.scripthandle, "Setup", "SelectedGRP", g_grpNamePtr);

#ifdef STARTUP_SETUP_WINDOW
    if (g_noSetup == 0)
        SCRIPT_PutString(scripthandle, "Setup", "ModDir", &g_modDir[0]);
#endif
    // exit early after only updating the values that can be changed from the startup window
    if (flags & 1)
    {
        SCRIPT_Save(scripthandle, setupfilename);
        SCRIPT_Free(scripthandle);
        return;
    }

    SCRIPT_PutNumber(scripthandle, "Screen Setup", "MaxRefreshFreq", maxrefreshfreq, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "WindowPosX", windowx, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "WindowPosY", windowy, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "WindowPositioning", windowpos, FALSE, FALSE);

    //if (!NAM_WW2GI)
    //{
    //    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "Out",ud.lockout,FALSE,FALSE);
    //    SCRIPT_PutString(ud.config.scripthandle, "Screen Setup", "Password",ud.pwlockout);
    //}

//#ifdef _WIN32
//    SCRIPT_PutNumber(ud.config.scripthandle, "Updates", "CheckForUpdates", ud.config.CheckForUpdates, FALSE, FALSE);
//    SCRIPT_PutNumber(ud.config.scripthandle, "Updates", "LastUpdateCheck", ud.config.LastUpdateCheck, FALSE, FALSE);
//#endif

    if (gSetup.usemouse)
    {
        for (int i=0; i<MAXMOUSEBUTTONS; i++)
        {
            if (CONFIG_FunctionNumToName(MouseFunctions[i][0]))
            {
                Bsprintf(buf, "MouseButton%d", i);
                SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(MouseFunctions[i][0]));
            }

            if (i >= (MAXMOUSEBUTTONS-2)) continue;

            if (CONFIG_FunctionNumToName(MouseFunctions[i][1]))
            {
                Bsprintf(buf, "MouseButtonClicked%d", i);
                SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(MouseFunctions[i][1]));
            }
        }

        for (int i=0; i<MAXMOUSEAXES; i++)
        {
            if (CONFIG_AnalogNumToName(MouseAnalogueAxes[i]))
            {
                Bsprintf(buf, "MouseAnalogAxes%d", i);
                SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_AnalogNumToName(MouseAnalogueAxes[i]));
            }

            if (CONFIG_FunctionNumToName(MouseDigitalFunctions[i][0]))
            {
                Bsprintf(buf, "MouseDigitalAxes%d_0", i);
                SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(MouseDigitalFunctions[i][0]));
            }

            if (CONFIG_FunctionNumToName(MouseDigitalFunctions[i][1]))
            {
                Bsprintf(buf, "MouseDigitalAxes%d_1", i);
                SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(MouseDigitalFunctions[i][1]));
            }

            Bsprintf(buf, "MouseAnalogScale%d", i);
            SCRIPT_PutNumber(scripthandle, "Controls", buf, MouseAnalogueScale[i], FALSE, FALSE);
        }
    }

    if (gSetup.usejoystick)
    {
        for (int dummy=0; dummy<MAXJOYBUTTONSANDHATS; dummy++)
        {
            if (CONFIG_FunctionNumToName(JoystickFunctions[dummy][0]))
            {
                Bsprintf(buf, "ControllerButton%d", dummy);
                SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickFunctions[dummy][0]));
            }

            if (CONFIG_FunctionNumToName(JoystickFunctions[dummy][1]))
            {
                Bsprintf(buf, "ControllerButtonClicked%d", dummy);
                SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickFunctions[dummy][1]));
            }
        }
        for (int dummy=0; dummy<MAXJOYAXES; dummy++)
        {
            if (CONFIG_AnalogNumToName(JoystickAnalogueAxes[dummy]))
            {
                Bsprintf(buf, "ControllerAnalogAxes%d", dummy);
                SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_AnalogNumToName(JoystickAnalogueAxes[dummy]));
            }

            if (CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][0]))
            {
                Bsprintf(buf, "ControllerDigitalAxes%d_0", dummy);
                SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][0]));
            }

            if (CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][1]))
            {
                Bsprintf(buf, "ControllerDigitalAxes%d_1", dummy);
                SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][1]));
            }

            Bsprintf(buf, "ControllerAnalogScale%d", dummy);
            SCRIPT_PutNumber(scripthandle, "Controls", buf, JoystickAnalogueScale[dummy], FALSE, FALSE);

            Bsprintf(buf, "ControllerAnalogInvert%d", dummy);
            SCRIPT_PutNumber(scripthandle, "Controls", buf, JoystickAnalogueInvert[dummy], FALSE, FALSE);

            Bsprintf(buf, "ControllerAnalogDead%d", dummy);
            SCRIPT_PutNumber(scripthandle, "Controls", buf, JoystickAnalogueDead[dummy], FALSE, FALSE);

            Bsprintf(buf, "ControllerAnalogSaturate%d", dummy);
            SCRIPT_PutNumber(scripthandle, "Controls", buf, JoystickAnalogueSaturate[dummy], FALSE, FALSE);
        }
    }

    //SCRIPT_PutString(ud.config.scripthandle, "Comm Setup","PlayerName",&szPlayerName[0]);

    //SCRIPT_PutString(ud.config.scripthandle, "Comm Setup","RTSName",&ud.rtsname[0]);

 //   char commmacro[] = "CommbatMacro# ";

    //for (int dummy = 0; dummy < MAXRIDECULE; dummy++)
    //{
    //    commmacro[13] = dummy+'0';
    //    SCRIPT_PutString(ud.config.scripthandle, "Comm Setup",commmacro,&ud.ridecule[dummy][0]);
    //}

    SCRIPT_Save(scripthandle, setupfilename);

    if ((flags & 2) == 0)
        SCRIPT_Free(scripthandle);

    OSD_Printf("Wrote %s\n",setupfilename);
    CONFIG_WriteSettings();
    Bfflush(NULL);
}
