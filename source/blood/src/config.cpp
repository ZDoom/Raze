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


#include "baselayer.h"
#include "common_game.h"
#include "build.h"
#include "cache1d.h"
#include "hash.h"
#include "scriplib.h"
#include "renderlayer.h"
#include "function.h"
#include "blood.h"
#include "config.h"
#include "gamedefs.h"
#include "globals.h"
#include "screen.h"
#include "sound.h"
#include "tile.h"
#include "view.h"

#if defined RENDERTYPESDL && defined SDL_TARGET && SDL_TARGET > 1
# include "sdl_inc.h"
#endif

// we load this in to get default button and key assignments
// as well as setting up function mappings

#define __SETUP__   // JBF 20031211
#include "_functio.h"

BEGIN_BLD_NS


hashtable_t h_gamefuncs    = { NUMGAMEFUNCTIONS<<1, NULL };

int32_t MouseDeadZone, MouseBias;
int32_t SmoothInput;
int32_t MouseFunctions[MAXMOUSEBUTTONS][2];
int32_t MouseDigitalFunctions[MAXMOUSEAXES][2];
int32_t MouseAnalogueAxes[MAXMOUSEAXES];
int32_t MouseAnalogueScale[MAXMOUSEAXES];
int32_t JoystickFunctions[MAXJOYBUTTONSANDHATS][2];
int32_t JoystickDigitalFunctions[MAXJOYAXES][2];
int32_t JoystickAnalogueAxes[MAXJOYAXES];
int32_t JoystickAnalogueScale[MAXJOYAXES];
int32_t JoystickAnalogueDead[MAXJOYAXES];
int32_t JoystickAnalogueSaturate[MAXJOYAXES];
uint8_t KeyboardKeys[NUMGAMEFUNCTIONS][2];
int32_t scripthandle;
int32_t setupread;
int32_t mus_restartonload;
int32_t configversion;
int32_t CheckForUpdates;
int32_t LastUpdateCheck;
int32_t useprecache;
char CommbatMacro[MAXRIDECULE][MAXRIDECULELENGTH];
char szPlayerName[MAXPLAYERNAME];
int32_t gTurnSpeed;
int32_t gDetail;
int32_t gMouseAim;
int32_t cl_weaponswitch;
int32_t gAutoRun;
int32_t gFollowMap;
int32_t gOverlayMap;
int32_t gRotateMap;
int32_t gMessageState;
int32_t gMessageCount;
int32_t gMessageTime;
int32_t gMessageFont;
int32_t gbAdultContent;
char gzAdultPassword[9];
int32_t gMouseSensitivity;
int32_t gMouseAiming;
int32_t gMouseAimingFlipped;
bool gNoClip;
bool gInfiniteAmmo;
bool gFullMap;
int32_t gUpscaleFactor;
int32_t gPowerupDuration;
int32_t gDeliriumBlur;

//////////
int gWeaponsV10x;
/////////

int32_t CONFIG_FunctionNameToNum(const char *func)
{
    int32_t i;

    if (!func)
        return -1;

    i = hash_find(&h_gamefuncs,func);

    if (i < 0)
    {
        char *str = Bstrtolower(Xstrdup(func));
        i = hash_find(&h_gamefuncs,str);
        Bfree(str);

        return i;
    }

    return i;
}


char *CONFIG_FunctionNumToName(int32_t func)
{
    if ((unsigned)func >= (unsigned)NUMGAMEFUNCTIONS)
        return NULL;
    return gamefunctions[func];
}


int32_t CONFIG_AnalogNameToNum(const char *func)
{
    if (!func)
        return -1;

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
    }

    return NULL;
}


void CONFIG_SetDefaultKeys(const char (*keyptr)[MAXGAMEFUNCLEN], bool lazy/*=false*/)
{
    static char const s_gamefunc_[] = "gamefunc_";
    int constexpr strlen_gamefunc_ = ARRAY_SIZE(s_gamefunc_) - 1;

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


void CONFIG_SetDefaults(void)
{
    scripthandle = -1;

#ifdef __ANDROID__
    droidinput.forward_sens = 5.f;
    droidinput.gameControlsAlpha = 0.5;
    droidinput.hideStick = 0;
    droidinput.pitch_sens = 5.f;
    droidinput.quickSelectWeapon = 1;
    droidinput.strafe_sens = 5.f;
    droidinput.toggleCrouch = 1;
    droidinput.yaw_sens = 5.f;

    gSetup.xdim = droidinfo.screen_width;
    gSetup.ydim = droidinfo.screen_height;
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
        gSetup.xdim = dm.w;
        gSetup.ydim = dm.h;
    }
    else
# endif
    {
        gSetup.xdim = 1024;
        gSetup.ydim = 768;
    }
#endif

#ifdef USE_OPENGL
    gSetup.bpp = 32;
#else
    gSetup.bpp = 8;
#endif

#ifdef GEKKO
    gSetup.usejoystick = 1;
#else
    gSetup.usejoystick = 0;
#endif

    gSetup.forcesetup       = 1;
    gSetup.noautoload       = 1;
    gSetup.fullscreen       = 1;
    gSetup.usemouse         = 1;

    //snd_ambience  = 1;
    //ud.config.AutoAim         = 1;
    CheckForUpdates = 1;
    MouseBias       = 0;
    MouseDeadZone   = 0;
    gBrightness = 8;
    //ud.config.ShowWeapons     = 0;
    SmoothInput     = 1;

    useprecache     = 1;
    configversion          = 0;
    //ud.crosshair              = 1;
    //ud.default_skill          = 1;
    gUpscaleFactor = 0;
    //ud.display_bonus_screen   = 1;
    //ud.drawweapon             = 1;
    //ud.hudontop               = 0;        = 1;
    gPowerupDuration = 1;
    //ud.lockout                = 0;
    //ud.m_marker               = 1;
    //ud.maxautosaves           = 5;
    //ud.menu_scrollbartilenum  = -1;
    //ud.menu_scrollbarz        = 65536;
    //ud.menu_scrollcursorz     = 65536;
    //ud.menu_slidebarmargin    = 65536;
    //ud.menu_slidebarz         = 65536;
    //ud.menu_slidecursorz      = 65536;
    //ud.menubackground         = 1;
    //ud.mouseaiming            = 0;
    //ud.mouseflip              = 1;
    //ud.msgdisptime            = 120;
    //ud.pwlockout[0]           = '\0';
    //ud.screen_size            = 4;
    //ud.screen_tilting         = 1;
    //ud.screenfade             = 1;
    //ud.shadow_pal             = 4;
    //ud.shadows                = 1;
    //ud.show_level_text        = 1;
    //ud.slidebar_paldisabled   = 1;
    //ud.statusbarflags         = STATUSBAR_NOSHRINK;
    //ud.statusbarmode          = 1;
    //ud.statusbarscale         = 100;
    //ud.team                   = 0;
    //ud.textscale              = 200;
    //ud.weaponscale            = 100;
    //cl_weaponswitch           = 3;  // new+empty
    gDeliriumBlur = 1;
    gViewSize = 2;
    gTurnSpeed = 92;
    gDetail = 4;
    gAutoRun = 0;
    gFollowMap = 1;
    gOverlayMap = 0;
    gRotateMap = 0;

    gMessageState = 1;
    gMessageCount = 4;
    gMessageTime = 5;
    gMessageFont = 0;
    gbAdultContent = 0;
    gzAdultPassword[0] = 0;

    gMouseAimingFlipped = 0;
    gMouseAim = 1;
    cl_weaponswitch = 1;

    Bstrcpy(szPlayerName, "Player");

    Bstrcpy(CommbatMacro[0], "I love the smell of napalm...");
    Bstrcpy(CommbatMacro[1], "Is that gasoline I smell?");
    Bstrcpy(CommbatMacro[2], "Ta da!");
    Bstrcpy(CommbatMacro[3], "Who wants some, huh? Who's next?");
    Bstrcpy(CommbatMacro[4], "I have something for you.");
    Bstrcpy(CommbatMacro[5], "You just gonna stand there...");
    Bstrcpy(CommbatMacro[6], "That'll teach ya!");
    Bstrcpy(CommbatMacro[7], "Ooh, that wasn't a bit nice.");
    Bstrcpy(CommbatMacro[8], "Amateurs!");
    Bstrcpy(CommbatMacro[9], "Fool! You are already dead.");

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

    for (int i=0; i<MAXJOYBUTTONSANDHATS; i++)
    {
        JoystickFunctions[i][0] = CONFIG_FunctionNameToNum(joystickdefaults[i]);
        JoystickFunctions[i][1] = CONFIG_FunctionNameToNum(joystickclickeddefaults[i]);
        CONTROL_MapButton(JoystickFunctions[i][0], i, 0, controldevice_joystick);
        CONTROL_MapButton(JoystickFunctions[i][1], i, 1, controldevice_joystick);
    }

    for (int i=0; i<MAXJOYAXES; i++)
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


// wrapper for CONTROL_MapKey(), generates key bindings to reflect changes to keyboard setup
void CONFIG_MapKey(int which, kb_scancode key1, kb_scancode oldkey1, kb_scancode key2, kb_scancode oldkey2)
{
    int const keys[] = { key1, key2, oldkey1, oldkey2 };
    char buf[2*MAXGAMEFUNCLEN];
    char tempbuf[128];

    if (which == gamefunc_Show_Console)
        OSD_CaptureKey(key1);

    for (int k = 0; (unsigned)k < ARRAY_SIZE(keys); k++)
    {
        if (keys[k] == 0xff || !keys[k])
            continue;

        int match = 0;

        for (; sctokeylut[match].key; match++)
        {
            if (keys[k] == sctokeylut[match].sc)
                break;
        }

        tempbuf[0] = 0;

        for (int i=NUMGAMEFUNCTIONS-1; i>=0; i--)
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
            if (CONFIG_AnalogNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                MouseAnalogueAxes[i] = CONFIG_AnalogNameToNum(temp);

        Bsprintf(str,"MouseDigitalAxes%d_0",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                MouseDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseDigitalAxes%d_1",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
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
        Bsprintf(str,"JoystickButton%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle,"Controls", str,temp))
            JoystickFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickButtonClicked%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle,"Controls", str,temp))
            JoystickFunctions[i][1] = CONFIG_FunctionNameToNum(temp);
    }

    // map over the axes
    for (i=0; i<MAXJOYAXES; i++)
    {
        Bsprintf(str,"JoystickAnalogAxes%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            if (CONFIG_AnalogNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                JoystickAnalogueAxes[i] = CONFIG_AnalogNameToNum(temp);

        Bsprintf(str,"JoystickDigitalAxes%d_0",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                JoystickDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickDigitalAxes%d_1",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                JoystickDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickAnalogScale%d",i);
        scale = JoystickAnalogueScale[i];
        SCRIPT_GetNumber(scripthandle, "Controls", str,&scale);
        JoystickAnalogueScale[i] = scale;

        Bsprintf(str,"JoystickAnalogDead%d",i);
        scale = JoystickAnalogueDead[i];
        SCRIPT_GetNumber(scripthandle, "Controls", str,&scale);
        JoystickAnalogueDead[i] = scale;

        Bsprintf(str,"JoystickAnalogSaturate%d",i);
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
    }
}


int CONFIG_ReadSetup(void)
{
    char tempbuf[1024];

    CONTROL_ClearAssignments();
    CONFIG_SetDefaults();

    setupread = 1;
    pathsearchmode = 1;

    if (scripthandle < 0)
    {
        if (buildvfs_exists(SetupFilename))  // JBF 20031211
            scripthandle = SCRIPT_Load(SetupFilename);
#if !defined(EDUKE32_TOUCH_DEVICES) && !defined(EDUKE32_STANDALONE)
        else if (buildvfs_exists(SETUPFILENAME))
        {
            int const i = wm_ynbox("Import Configuration Settings",
                                   "The configuration file \"%s\" was not found. "
                                   "Import configuration data from \"%s\"?",
                                   SetupFilename, SETUPFILENAME);
            if (i)
                scripthandle = SCRIPT_Load(SETUPFILENAME);
        }
#endif
    }

    pathsearchmode = 0;

    if (scripthandle < 0)
        return -1;

    // Nuke: make cvar
    ///////
    SCRIPT_GetNumber(scripthandle, "Game Options", "WeaponsV10x", &gWeaponsV10x);
    ///////

    char commmacro[] = "CommbatMacro# ";

    for (int i = 0; i < MAXRIDECULE; i++)
    {
        commmacro[13] = i+'0';
        SCRIPT_GetString(scripthandle, "Comm Setup",commmacro,&CommbatMacro[i][0]);
    }

    Bmemset(tempbuf, 0, sizeof(tempbuf));
    SCRIPT_GetString(scripthandle, "Comm Setup","PlayerName",&tempbuf[0]);

    char nameBuf[64];

    while (Bstrlen(OSD_StripColors(nameBuf, tempbuf)) > 10)
        tempbuf[Bstrlen(tempbuf) - 1] = '\0';

    Bstrncpyz(szPlayerName, tempbuf, sizeof(szPlayerName));

    //SCRIPT_GetString(scripthandle, "Comm Setup","RTSName",&ud.rtsname[0]);

    SCRIPT_GetNumber(scripthandle, "Setup", "ConfigVersion", &configversion);
    SCRIPT_GetNumber(scripthandle, "Setup", "ForceSetup", &gSetup.forcesetup);
    SCRIPT_GetNumber(scripthandle, "Setup", "NoAutoLoad", &gSetup.noautoload);

    if (gNoSetup == 0 && g_modDir[0] == '/')
    {
        struct Bstat st;
        SCRIPT_GetString(scripthandle, "Setup","ModDir",&g_modDir[0]);

        if (Bstat(g_modDir, &st))
        {
            if ((st.st_mode & S_IFDIR) != S_IFDIR)
            {
                initprintf("Invalid mod dir in cfg!\n");
                Bsprintf(g_modDir,"/");
            }
        }
    }

    //if (g_grpNamePtr == NULL && g_addonNum == 0)
    //{
    //    SCRIPT_GetStringPtr(scripthandle, "Setup", "SelectedGRP", &g_grpNamePtr);
    //    if (g_grpNamePtr && !Bstrlen(g_grpNamePtr))
    //        g_grpNamePtr = dup_filename(G_DefaultGrpFile());
    //}
    //
    //if (!NAM_WW2GI)
    //{
    //    SCRIPT_GetNumber(scripthandle, "Screen Setup", "Out", &ud.lockout);
    //    SCRIPT_GetString(scripthandle, "Screen Setup", "Password", &ud.pwlockout[0]);
    //}

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

#ifdef POLYMER
    int32_t rendmode = 0;
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "Polymer", &rendmode);
    glrendmode = (rendmode > 0) ? REND_POLYMER : REND_POLYMOST;
#endif

    //SCRIPT_GetNumber(scripthandle, "Misc", "Executions", &ud.executions);

#ifdef _WIN32
    SCRIPT_GetNumber(scripthandle, "Updates", "CheckForUpdates", &CheckForUpdates);
    SCRIPT_GetNumber(scripthandle, "Updates", "LastUpdateCheck", &LastUpdateCheck);
#endif

    setupread = 1;
    return 0;
}


void CONFIG_WriteSettings(void) // save binds and aliases to <cfgname>_settings.cfg
{
    char *ptr = Xstrdup(SetupFilename);
    char filename[BMAX_PATH];

    if (!Bstrcmp(SetupFilename, SETUPFILENAME))
        Bsprintf(filename, "settings.cfg");
    else
        Bsprintf(filename, "%s_settings.cfg", strtok(ptr, "."));

    BFILE *fp = Bfopen(filename, "wt");

    if (fp)
    {
        Bfprintf(fp,"unbindall\n");

        for (int i=0; i<MAXBOUNDKEYS+MAXMOUSEBUTTONS; i++)
        {
            if (CONTROL_KeyIsBound(i))
            {
                Bfprintf(fp, "bind \"%s\"%s \"%s\"\n", CONTROL_KeyBinds[i].key,
                                                       CONTROL_KeyBinds[i].repeat ? "" : " norepeat",
                                                       CONTROL_KeyBinds[i].cmdstr);
            }
        }

        OSD_WriteAliases(fp);

        if (g_isAlterDefaultCrosshair)
            Bfprintf(fp, "crosshaircolor %d %d %d\n", CrosshairColors.r, CrosshairColors.g, CrosshairColors.b);

        OSD_WriteCvars(fp);

        Bfclose(fp);
        Bfree(ptr);

        OSD_Printf("Wrote %s\n", filename);

        return;
    }

    OSD_Printf("Error writing %s: %s\n", filename, strerror(errno));

    Bfree(ptr);
}

void CONFIG_WriteSetup(uint32_t flags)
{
    char buf[128];
    if (!setupread) return;

    if (scripthandle < 0)
        scripthandle = SCRIPT_Init(SetupFilename);

    //SCRIPT_PutNumber(scripthandle, "Misc", "Executions", ud.executions, FALSE, FALSE);

    SCRIPT_PutNumber(scripthandle, "Setup", "ConfigVersion", BYTEVERSION, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Setup", "ForceSetup", gSetup.forcesetup, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Setup", "NoAutoLoad", gSetup.noautoload, FALSE, FALSE);

#ifdef POLYMER
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "Polymer", glrendmode == REND_POLYMER, FALSE, FALSE);
#endif

    SCRIPT_PutNumber(scripthandle, "Screen Setup", "ScreenBPP", gSetup.bpp, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "ScreenHeight", gSetup.ydim, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "ScreenMode", gSetup.fullscreen, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "ScreenWidth", gSetup.xdim, FALSE, FALSE);

    //if (g_grpNamePtr && !g_addonNum)
    //    SCRIPT_PutString(scripthandle, "Setup", "SelectedGRP", g_grpNamePtr);

#ifdef STARTUP_SETUP_WINDOW
    if (gNoSetup == 0)
        SCRIPT_PutString(scripthandle, "Setup", "ModDir", &g_modDir[0]);
#endif
    // exit early after only updating the values that can be changed from the startup window
    if (flags & 1)
    {
        SCRIPT_Save(scripthandle, SetupFilename);
        SCRIPT_Free(scripthandle);
        return;
    }

    SCRIPT_PutNumber(scripthandle, "Screen Setup", "MaxRefreshFreq", maxrefreshfreq, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "WindowPosX", windowx, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "WindowPosY", windowy, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Screen Setup", "WindowPositioning", windowpos, FALSE, FALSE);

    //if (!NAM_WW2GI)
    //{
    //    SCRIPT_PutNumber(scripthandle, "Screen Setup", "Out",ud.lockout,FALSE,FALSE);
    //    SCRIPT_PutString(scripthandle, "Screen Setup", "Password",ud.pwlockout);
    //}

#ifdef _WIN32
    SCRIPT_PutNumber(scripthandle, "Updates", "CheckForUpdates", CheckForUpdates, FALSE, FALSE);
    SCRIPT_PutNumber(scripthandle, "Updates", "LastUpdateCheck", LastUpdateCheck, FALSE, FALSE);
#endif

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

            if (MouseAnalogueScale[i] != DEFAULTMOUSEANALOGUESCALE)
            {
                Bsprintf(buf, "MouseAnalogScale%d", i);
                SCRIPT_PutNumber(scripthandle, "Controls", buf, MouseAnalogueScale[i], FALSE, FALSE);
            }
        }
    }

    if (gSetup.usejoystick)
    {
        for (int dummy=0; dummy<MAXJOYBUTTONSANDHATS; dummy++)
        {
            if (CONFIG_FunctionNumToName(JoystickFunctions[dummy][0]))
            {
                Bsprintf(buf, "JoystickButton%d", dummy);
                SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickFunctions[dummy][0]));
            }

            if (CONFIG_FunctionNumToName(JoystickFunctions[dummy][1]))
            {
                Bsprintf(buf, "JoystickButtonClicked%d", dummy);
                SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickFunctions[dummy][1]));
            }
        }
        for (int dummy=0; dummy<MAXJOYAXES; dummy++)
        {
            if (CONFIG_AnalogNumToName(JoystickAnalogueAxes[dummy]))
            {
                Bsprintf(buf, "JoystickAnalogAxes%d", dummy);
                SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_AnalogNumToName(JoystickAnalogueAxes[dummy]));
            }

            if (CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][0]))
            {
                Bsprintf(buf, "JoystickDigitalAxes%d_0", dummy);
                SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][0]));
            }

            if (CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][1]))
            {
                Bsprintf(buf, "JoystickDigitalAxes%d_1", dummy);
                SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][1]));
            }

            if (JoystickAnalogueScale[dummy] != DEFAULTJOYSTICKANALOGUESCALE)
            {
                Bsprintf(buf, "JoystickAnalogScale%d", dummy);
                SCRIPT_PutNumber(scripthandle, "Controls", buf, JoystickAnalogueScale[dummy], FALSE, FALSE);
            }

            if (JoystickAnalogueDead[dummy] != DEFAULTJOYSTICKANALOGUEDEAD)
            {
                Bsprintf(buf, "JoystickAnalogDead%d", dummy);
                SCRIPT_PutNumber(scripthandle, "Controls", buf, JoystickAnalogueDead[dummy], FALSE, FALSE);
            }

            if (JoystickAnalogueSaturate[dummy] != DEFAULTJOYSTICKANALOGUESATURATE)
            {
                Bsprintf(buf, "JoystickAnalogSaturate%d", dummy);
                SCRIPT_PutNumber(scripthandle, "Controls", buf, JoystickAnalogueSaturate[dummy], FALSE, FALSE);
            }
        }
    }

    SCRIPT_PutString(scripthandle, "Comm Setup","PlayerName",&szPlayerName[0]);

    //SCRIPT_PutString(scripthandle, "Comm Setup","RTSName",&ud.rtsname[0]);

    char commmacro[] = "CommbatMacro# ";

    for (int dummy = 0; dummy < MAXRIDECULE; dummy++)
    {
        commmacro[13] = dummy+'0';
        SCRIPT_PutString(scripthandle, "Comm Setup",commmacro,&CommbatMacro[dummy][0]);
    }

    ///////
    SCRIPT_PutNumber(scripthandle, "Game Options", "WeaponsV10x", gWeaponsV10x, FALSE, FALSE);
    ///////
    
    SCRIPT_Save(scripthandle, SetupFilename);

    if ((flags & 2) == 0)
        SCRIPT_Free(scripthandle);

    OSD_Printf("Wrote %s\n",SetupFilename);
    CONFIG_WriteSettings();
	G_SaveConfig();
    Bfflush(NULL);
}


END_BLD_NS
