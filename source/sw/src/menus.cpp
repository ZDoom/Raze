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
#include "osd.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "sector.h"
#include "sprite.h"
#include "weapon.h"
#include "player.h"
#include "jsector.h"
#include "control.h"
#include "menus.h"
#include "sw_strs.h"
#include "pal.h"
#include "demo.h"

#include "function.h"
#include "gamedefs.h"
#include "config.h"
#include "network.h"
#include "fx_man.h"
#include "music.h"
#include "text.h"

#include "colormap.h"
#include "config.h"

//#define PLOCK_VERSION TRUE


short TimeLimitTable[9] = {0,3,5,10,15,20,30,45,60};

short QuickLoadNum = -1;
char QuickLoadDescrDialog[128];
SWBOOL QuickSaveMode = FALSE;
SWBOOL SavePrompt = FALSE;
extern SWBOOL InMenuLevel, LoadGameOutsideMoveLoop, LoadGameFromDemo;
extern uint8_t RedBookSong[40];
extern SWBOOL ExitLevel, NewGame;
extern short Level, Skill;
extern SWBOOL MusicInitialized, FxInitialized;
SWBOOL MNU_CheckUserMap(MenuItem *item);
SWBOOL MNU_SaveGameCheck(MenuItem_p item);
SWBOOL MNU_TeamPlayCheck(MenuItem *item);
SWBOOL MNU_CoopPlayCheck(MenuItem *item);
SWBOOL MNU_StatCheck(MenuItem *item);
SWBOOL MNU_LoadGameCheck(MenuItem *item);
SWBOOL MNU_TenCheck(MenuItem *item);
static SWBOOL MNU_TryMusicInit(void);
static void MNU_UpLevel(void);

SWBOOL MNU_LoadSaveDraw(UserCall call, MenuItem *item);
SWBOOL MNU_LoadSaveMove(UserCall call, MenuItem *item);

SWBOOL MenuButtonAutoRun = FALSE;
SWBOOL MenuButtonAutoAim = FALSE;
// misc load-save vars
short LastSaveNum = 99;
char SaveGameDescr[10][80];
char BackupSaveGameDescr[80];
short screen_tile = -1;

SWBOOL MenuInputMode = FALSE;
int16_t MenuTextShade = 0;
SWBOOL passwordvalid = FALSE;

SWBOOL MNU_HurtTeammateCheck(MenuItem *item);
SWBOOL MNU_TeamPlayChange(void);

// Font pic table
unsigned short xlatfont[] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 32
    2274, 2294, 2274, 2277, 2278, 2280, 2295, 2282, 2283, 2281, 2286, 2297, 2285,       // 45
    2299, 2301, 2271, 2262, 2263, 2264, 2265, 2266, 2267, 2268, 2269, 2270,     // 57
    2292, 2293, 2296, 2287, 2298, 2300, 2275, 2236, 2237, 2238, 2239, 2240,     // 69
    2241, 2242, 2243, 2244, 2245, 2246, 2247, 2248, 2249, 2250, 2251, 2252,     // 81
    2253, 2254, 2255, 2256, 2257, 2258, 2259, 2260, 2261, 2290, 2289, 2291,     // 93
    2279, 2284, 2295, 2236, 2237, 2238, 2239, 2240, 2241, 2242, 2243, 2244,     // 105
    2245, 2246, 2247, 2248, 2249, 2250, 2251, 2252, 2253, 2254, 2255, 2256,     // 117
    2257, 2258, 2259, 2260, 2261, 2282, 2288, 2283, 2272        // 126
};

int slidersettings [sldr_max] =
{
    0, SENSE_DEFAULT, FXVOL_DEFAULT, MUSIC_DEFAULT, SCRSIZE_DEFAULT,
    BRIGHTNESS_DEFAULT, BORDERTILE_DEFAULT, GAMETYPE_DEFAULT, NETLEVEL_DEFAULT,
    MONSTERS_DEFAULT, KILLLIMIT_DEFAULT, TIMELIMIT_DEFAULT, PLAYERCOLOR_DEFAULT,
    0,0,    // video mode
    32767>>12, 32767>>12,  // advanced mouse scale
};

short buttonsettings[btn_max];

// EXTERNS ////////////////////////////
#define XDIM    320
#define YDIM    200
extern SWBOOL QuitFlag;

void TerminateGame(void);
void ResetKeys(void);

// GLOBALS ////////////////////////////

char playertextbuffer[80];              // Used for various input strings
char playerbuflen = 0;                  // Current length of the string in
// the buffer
char maxtextlen;                        // max length allowed for current

static struct { int xdim,ydim; } validresolutions[MAXVALIDMODES];
static int numvalidresolutions = 0, validbpps[8], numvalidbpps = 0;

static void UpdateValidModes(int bpp, int fs)
{
    int i, j;

    numvalidresolutions = numvalidbpps = 0;
    for (i=0; i<validmodecnt; i++)
    {
        if ((validmode[i].fs & 1) != fs) continue;

        for (j=0; j<numvalidbpps; j++)
            if (validbpps[j] == validmode[i].bpp) break;
        if (j==numvalidbpps) validbpps[numvalidbpps++] = validmode[i].bpp;

        if (validmode[i].bpp != bpp) continue;

        validresolutions[numvalidresolutions].xdim = validmode[i].xdim;
        validresolutions[numvalidresolutions].ydim = validmode[i].ydim;
        numvalidresolutions++;
    }
}
static SWBOOL ApplyModeSettings(void)
{
    int lastx, lasty, lastbpp, lastfs;
    int newx, newy, newbpp, newfs;

    lastx = xdim; lasty = ydim; lastbpp = bpp; lastfs = fullscreen;
    newx   = validresolutions[slidersettings[sldr_videores]].xdim;
    newy   = validresolutions[slidersettings[sldr_videores]].ydim;
    newbpp = validbpps[slidersettings[sldr_videobpp]];
    newfs  = buttonsettings[btn_videofs];

    if (lastx == newx && lasty == newy && lastbpp == newbpp && lastfs == newfs) return FALSE;

    if (videoSetGameMode(newfs, newx, newy, newbpp, upscalefactor))
        videoSetGameMode(lastfs, lastx, lasty, lastbpp, upscalefactor);
    else
    {
        extern int32_t ScreenMode,ScreenWidth,ScreenHeight,ScreenBPP; // Because I'm too lazy to include config.h
        ScreenMode = newfs;
        ScreenWidth = newx;
        ScreenHeight = newy;
        ScreenBPP = newbpp;

        SetupAspectRatio();
        SetRedrawScreen(Player + myconnectindex);
    }
    return FALSE;
}


MenuItem sound_i[] =
{
    {DefButton(btn_music, 0, "Music"), OPT_XS,                OPT_LINE(0), 1, m_defshade, 0, MNU_TryMusicInit, MNU_MusicCheck, NULL},
    {DefSlider(sldr_musicvolume, 0, "Music Volume"), OPT_XS,  OPT_LINE(1), 1, m_defshade, 0, MNU_TryMusicInit, MNU_MusicCheck, NULL},
    {DefInert(0, NULL), OPT_XSIDE,                            OPT_LINE(1), 0, m_defshade, 0, NULL, NULL, NULL},

    {DefButton(btn_sound, 0, "Sounds"), OPT_XS,               OPT_LINE(2), 1, m_defshade, 0, NULL, MNU_FxCheck, NULL},
    {DefSlider(sldr_sndfxvolume, 0, "Sound Volume"), OPT_XS,  OPT_LINE(3), 1, m_defshade, 0, NULL, MNU_FxCheck, NULL},
    {DefInert(0, NULL), OPT_XSIDE,                            OPT_LINE(3), 0, m_defshade, 0, NULL, NULL, NULL},

    //{DefButton(btn_talking, 0, "Talking"), OPT_XS,            OPT_LINE(4), 1, m_defshade, 0, NULL, MNU_FxCheck, NULL},
    {DefButton(btn_ambience, 0, "Ambience"), OPT_XS,          OPT_LINE(4), 1, m_defshade, 0, NULL, MNU_FxCheck, NULL},
    {DefButton(btn_flipstereo, 0, "Flip Stereo"), OPT_XS,     OPT_LINE(5), 1, m_defshade, 0, NULL, MNU_FxCheck, NULL},
    //{DefButton(btn_playcd, 0, "Play CD"), OPT_XS,         OPT_LINE(6), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefNone}
};

MenuGroup soundgroup = {110,5,"^Sound",sound_i,pic_optionstitl,0,m_defshade, NULL,NULL, 0};

MenuItem parental_i[] =
{
    {DefButton(btn_parental, 0, "Kid Mode"), OPT_XS, OPT_LINE(0), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefOption(KEYSC_P, "Change Password"),              OPT_XS, OPT_LINE(1), 1, m_defshade, 0, MNU_ParentalCustom, NULL, NULL},
    {DefNone}
};

MenuGroup parentalgroup = {65, 5, "^Kid Mode", parental_i, pic_newgametitl, 0, m_defshade, MNU_DoParentalPassword,NULL,0};

MenuItem screen_i[] =
{
    {DefSlider(sldr_scrsize, 0, "Screen Size"), OPT_XS,          OPT_LINE(0), 1, m_defshade, 0, NULL, NULL, NULL}, //, MNU_BorderCheck},
    {DefInert(0, NULL), OPT_XSIDE,                               OPT_LINE(0), 0, m_defshade, 0, NULL, NULL, NULL},

    {DefSlider(sldr_bordertile, 0, "Border Tile"), OPT_XS,       OPT_LINE(1), 1, m_defshade, 0, NULL, NULL, NULL}, //, MNU_BorderCheck},
    {DefInert(0, NULL), OPT_XSIDE,                               OPT_LINE(1), 0, m_defshade, 0, NULL, NULL, NULL},

    {DefSlider(sldr_brightness, KEYSC_B, "Brightness"), OPT_XS,  OPT_LINE(2), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefInert(0, NULL), OPT_XSIDE,                               OPT_LINE(2), 0, m_defshade, 0, NULL, NULL, NULL},

    {DefButton(btn_videofs, 0, "Fullscreen"), OPT_XS,            OPT_LINE(4), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefSlider(sldr_videobpp, 0, "Colour"), OPT_XS,              OPT_LINE(5), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefInert(0, NULL), OPT_XSIDE,                               OPT_LINE(5), 0, m_defshade, 0, NULL, NULL, NULL},
    {DefSlider(sldr_videores, 0, "Resolution"), OPT_XS,          OPT_LINE(6), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefInert(0, NULL), OPT_XSIDE,                               OPT_LINE(6), 0, m_defshade, 0, NULL, NULL, NULL},
    {DefOption(0, "Apply Settings"), OPT_XSIDE,                  OPT_LINE(8), 1, m_defshade, 0, ApplyModeSettings, NULL, NULL},
    {DefNone}
};

MenuGroup screengroup = {65, 5, "^Screen", screen_i, pic_newgametitl, 0, m_defshade, NULL,NULL, 0};

MenuItem mouse_i[] =
{
    {DefSlider(sldr_mouse, 0, "Mouse Speed"), OPT_XS,            OPT_LINE(0), 1, m_defshade, 0, NULL, MNU_MouseCheck, NULL},
    {DefInert(0, NULL), OPT_XSIDE,                               OPT_LINE(0), 0, m_defshade, 0, NULL, NULL, NULL},    // Blank line for mouse

    {DefButton(btn_mouse_aim, 0, "Mouse Aiming"), OPT_XS,        OPT_LINE(1), 1, m_defshade, 0, NULL, MNU_MouseCheck, NULL},
    {DefButton(btn_mouse_invert, 0, "Invert Mouse"), OPT_XS,     OPT_LINE(2), 1, m_defshade, 0, NULL, MNU_MouseCheck, NULL},
    {DefNone}
};

MenuGroup mousegroup = {65, 5, "^Mouse", mouse_i, pic_newgametitl, 0, m_defshade, NULL,NULL, 0};

MenuGroup keysetupgroup = {0, 0, NULL, NULL, 0, 0, m_defshade, MNU_KeySetupCustom, NULL, 0};

static char MouseButtonFunctions[10][MAXFUNCTIONLENGTH];
static SWBOOL MNU_SetMouseButtonFunctions(MenuItem_p item);
static SWBOOL MNU_MouseButtonPostProcess(MenuItem_p item);
static SWBOOL MNU_MouseButtonSetupCustom(UserCall call, MenuItem_p item);
MenuGroup mousebuttongroup = {0, 0, NULL, NULL, 0, 0, m_defshade, MNU_MouseButtonSetupCustom, NULL, 0};
MenuItem mousesetup_i[] =
{
    {DefLayer(0, "Left", &mousebuttongroup),          OPT_XS, OPT_LINE(0), 1, m_defshade, 0,   NULL, NULL, MNU_MouseButtonPostProcess},
    {DefInert(0, MouseButtonFunctions[0]),         OPT_XSIDE, OPT_LINE(0), 1, m_defshade, 0,   NULL, MNU_SetMouseButtonFunctions, NULL},
    {DefLayer(0, "Double Left", &mousebuttongroup),   OPT_XS, OPT_LINE(1), 1, m_defshade, 6+0, NULL, NULL, MNU_MouseButtonPostProcess},
    {DefInert(0, MouseButtonFunctions[6+0]),       OPT_XSIDE, OPT_LINE(1), 1, m_defshade, 6+0, NULL, MNU_SetMouseButtonFunctions, NULL},
    {DefLayer(0, "Right", &mousebuttongroup),         OPT_XS, OPT_LINE(2), 1, m_defshade, 1,   NULL, NULL, MNU_MouseButtonPostProcess},
    {DefInert(0, MouseButtonFunctions[1]),         OPT_XSIDE, OPT_LINE(2), 1, m_defshade, 1,   NULL, MNU_SetMouseButtonFunctions, NULL},
    {DefLayer(0, "Double Right", &mousebuttongroup),  OPT_XS, OPT_LINE(3), 1, m_defshade, 6+1, NULL, NULL, MNU_MouseButtonPostProcess},
    {DefInert(0, MouseButtonFunctions[6+1]),       OPT_XSIDE, OPT_LINE(3), 1, m_defshade, 6+1, NULL, MNU_SetMouseButtonFunctions, NULL},
    {DefLayer(0, "Middle", &mousebuttongroup),        OPT_XS, OPT_LINE(4), 1, m_defshade, 2,   NULL, NULL, MNU_MouseButtonPostProcess},
    {DefInert(0, MouseButtonFunctions[2]),         OPT_XSIDE, OPT_LINE(4), 1, m_defshade, 2,   NULL, MNU_SetMouseButtonFunctions, NULL},
    {DefLayer(0, "Double Middle", &mousebuttongroup), OPT_XS, OPT_LINE(5), 1, m_defshade, 6+2, NULL, NULL, MNU_MouseButtonPostProcess},
    {DefInert(0, MouseButtonFunctions[6+2]),       OPT_XSIDE, OPT_LINE(5), 1, m_defshade, 6+2, NULL, MNU_SetMouseButtonFunctions, NULL},
    {DefLayer(0, "Thumb", &mousebuttongroup),         OPT_XS, OPT_LINE(6), 1, m_defshade, 3,   NULL, NULL, MNU_MouseButtonPostProcess},
    {DefInert(0, MouseButtonFunctions[3]),         OPT_XSIDE, OPT_LINE(6), 1, m_defshade, 3,   NULL, MNU_SetMouseButtonFunctions, NULL},
    {DefLayer(0, "Double Thumb", &mousebuttongroup),  OPT_XS, OPT_LINE(7), 1, m_defshade, 6+3, NULL, NULL, MNU_MouseButtonPostProcess},
    {DefInert(0, MouseButtonFunctions[6+3]),       OPT_XSIDE, OPT_LINE(7), 1, m_defshade, 6+3, NULL, MNU_SetMouseButtonFunctions, NULL},
    {DefLayer(0, "Wheel Up", &mousebuttongroup),      OPT_XS, OPT_LINE(8), 1, m_defshade, 4,   NULL, NULL, MNU_MouseButtonPostProcess},
    {DefInert(0, MouseButtonFunctions[4]),         OPT_XSIDE, OPT_LINE(8), 1, m_defshade, 4,   NULL, MNU_SetMouseButtonFunctions, NULL},
    {DefLayer(0, "Wheel Down", &mousebuttongroup),    OPT_XS, OPT_LINE(9), 1, m_defshade, 5,   NULL, NULL, MNU_MouseButtonPostProcess},
    {DefInert(0, MouseButtonFunctions[5]),         OPT_XSIDE, OPT_LINE(9), 1, m_defshade, 5,   NULL, MNU_SetMouseButtonFunctions, NULL},
    {DefNone}
};
MenuGroup mousesetupgroup = {65, 5, "^Mouse Setup", mousesetup_i, pic_newgametitl, 0, m_defshade, NULL, NULL, 0};

#define JOYSTICKITEMSPERPAGE 12
#define MAXJOYSTICKBUTTONPAGES (MAXJOYBUTTONS*2 / JOYSTICKITEMSPERPAGE)
static char JoystickButtonPageName[MAXJOYSTICKBUTTONPAGES][64];
static char JoystickButtonFunctions[MAXJOYBUTTONS*2][MAXFUNCTIONLENGTH];
static char JoystickButtonNames[MAXJOYBUTTONS*2][64];
static int JoystickButtonPage = 0;
static SWBOOL MNU_JoystickButtonsInitialise(MenuItem_p item);
static SWBOOL MNU_SetJoystickButtonFunctions(MenuItem_p item);
static SWBOOL MNU_JoystickButtonPostProcess(MenuItem_p item);
static SWBOOL MNU_JoystickButtonSetupCustom(UserCall call, MenuItem_p item);
static SWBOOL MNU_JoystickButtonNextPage(void);
MenuGroup joybuttonsgroup = {0, 0, NULL, NULL, 0, 0, m_defshade, MNU_JoystickButtonSetupCustom, NULL, 0};
MenuItem joybuttons_i[MAXJOYSTICKBUTTONPAGES][JOYSTICKITEMSPERPAGE*2+3] =    // itemsperpage * Layer/Inert + Pagetext + Next + DefNone
{
    // this menu gets defined by the call to MNU_JoystickButtonsInitialise
    { {DefNone} },
};
MenuGroup joybuttonssetupgroup = {65, 5, "^Joystick Setup", NULL, pic_newgametitl, 0, m_defshade, NULL, NULL, 0};

static char JoystickAxisName[64];
static char JoystickAxisPageName[64];
static char JoystickAxisFunctions[2][MAXFUNCTIONLENGTH];
static int JoystickAxisPage = 0;
static SWBOOL MNU_JoystickAxesInitialise(MenuItem_p item);
static SWBOOL MNU_SetJoystickAxisFunctions(MenuItem_p item);
static SWBOOL MNU_JoystickAxisPostProcess(MenuItem_p item);
static SWBOOL MNU_JoystickAxisSetupCustom(UserCall call, MenuItem_p item);
static SWBOOL MNU_JoystickAxisNextPage(void);
MenuGroup joyaxesgroup = {0, 0, NULL, NULL, 0, 0, m_defshade, MNU_JoystickAxisSetupCustom, NULL, 0};
MenuItem joyaxes_i[] =
{
    {DefInert(0, JoystickAxisName), OPT_XS, OPT_LINE(0), 1, MENU_SHADE_INACTIVE, 0, NULL, NULL, NULL},

    {DefSlider(sldr_joyaxisscale, 0, "Axis Scale"),     OPT_XS, OPT_LINE(2), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefInert(0, NULL),                              OPT_XSIDE, OPT_LINE(2), 0, m_defshade, 0, NULL, NULL, NULL},

    {DefSlider(sldr_joyaxisanalog, 0, "Analog"),        OPT_XS, OPT_LINE(4), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefInert(0, NULL),                              OPT_XSIDE, OPT_LINE(4), 0, m_defshade, 0, NULL, NULL, NULL},
    {DefLayer(0, "Digital +ve", &joyaxesgroup),         OPT_XS, OPT_LINE(5), 1, m_defshade, 1, NULL, NULL, MNU_JoystickAxisPostProcess},
    {DefInert(0, JoystickAxisFunctions[1]),          OPT_XSIDE, OPT_LINE(5), 1, m_defshade, 1, NULL, MNU_SetJoystickAxisFunctions, NULL},
    {DefLayer(0, "Digital -ve", &joyaxesgroup),         OPT_XS, OPT_LINE(6), 1, m_defshade, 0, NULL, NULL, MNU_JoystickAxisPostProcess},
    {DefInert(0, JoystickAxisFunctions[0]),          OPT_XSIDE, OPT_LINE(6), 1, m_defshade, 0, NULL, MNU_SetJoystickAxisFunctions, NULL},

    {DefSlider(sldr_joyaxisdead, 0, "Dead Zone"),       OPT_XS, OPT_LINE(8), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefInert(0, NULL),                              OPT_XSIDE, OPT_LINE(8), 0, m_defshade, 0, NULL, NULL, NULL},
    {DefSlider(sldr_joyaxissatur, 0, "Saturate"),       OPT_XS, OPT_LINE(9), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefInert(0, NULL),                              OPT_XSIDE, OPT_LINE(9), 0, m_defshade, 0, NULL, NULL, NULL},

    {DefInert(0, JoystickAxisPageName),                 OPT_XS, OPT_LINE(11), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefOption(0, "Next..."),                        OPT_XSIDE, OPT_LINE(11), 1, m_defshade, 0, MNU_JoystickAxisNextPage, NULL, NULL},

    {DefNone},
};
MenuGroup joyaxessetupgroup = {65, 5, "^Joystick Axes", joyaxes_i, pic_newgametitl, 0, m_defshade, NULL, NULL, 1};


static char AdvancedMouseAxisFunctions[4][MAXAXISFUNCTIONLENGTH] = { "", "", "", "" };
static SWBOOL MNU_SetAdvancedMouseFunctions(MenuItem_p item);
static SWBOOL MNU_MouseDigitalPostProcess(MenuItem_p item);
static SWBOOL MNU_MouseDigitalSetupCustom(UserCall call, MenuItem_p item);
MenuGroup advancedmousedigigroup = {0, 0, NULL, NULL, 0, 0, m_defshade, MNU_MouseDigitalSetupCustom, NULL, 0};
MenuItem advancedmouse_i[] =
{
    {DefSlider(sldr_mousescalex, 0, "X-Axis Scale"), OPT_XS,     OPT_LINE(0), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefInert(0, NULL), OPT_XSIDE,                               OPT_LINE(0), 0, m_defshade, 0, NULL, NULL, NULL},
    {DefSlider(sldr_mousescaley, 0, "Y-Axis Scale"), OPT_XS,     OPT_LINE(1), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefInert(0, NULL), OPT_XSIDE,                               OPT_LINE(1), 0, m_defshade, 0, NULL, NULL, NULL},
    {DefLayer(0, "Digital Up", &advancedmousedigigroup), OPT_XS, OPT_LINE(3), 1, m_defshade, 0, NULL, NULL, MNU_MouseDigitalPostProcess},
    {DefInert(0, AdvancedMouseAxisFunctions[0]), OPT_XSIDE,      OPT_LINE(3), 1, m_defshade, 0, NULL, MNU_SetAdvancedMouseFunctions, NULL},
    {DefLayer(0, "Digital Down", &advancedmousedigigroup), OPT_XS, OPT_LINE(4), 1, m_defshade, 1, NULL, NULL, MNU_MouseDigitalPostProcess},
    {DefInert(0, AdvancedMouseAxisFunctions[1]), OPT_XSIDE,      OPT_LINE(4), 1, m_defshade, 1, NULL, MNU_SetAdvancedMouseFunctions, NULL},
    {DefLayer(0, "Digital Left", &advancedmousedigigroup), OPT_XS, OPT_LINE(5), 1, m_defshade, 2, NULL, NULL, MNU_MouseDigitalPostProcess},
    {DefInert(0, AdvancedMouseAxisFunctions[2]), OPT_XSIDE,      OPT_LINE(5), 1, m_defshade, 2, NULL, MNU_SetAdvancedMouseFunctions, NULL},
    {DefLayer(0, "Digital Right", &advancedmousedigigroup), OPT_XS, OPT_LINE(6), 1, m_defshade, 3, NULL, NULL, MNU_MouseDigitalPostProcess},
    {DefInert(0, AdvancedMouseAxisFunctions[3]), OPT_XSIDE,      OPT_LINE(6), 1, m_defshade, 3, NULL, MNU_SetAdvancedMouseFunctions, NULL},
    {DefNone}
};
MenuGroup mouseadvancedgroup = {65, 5, "^Adv'd Mouse", advancedmouse_i, pic_newgametitl, 0, m_defshade, NULL, NULL, 0};

MenuItem inputsetup_i[] =
{
    {DefLayer(0, "Keys Setup", &keysetupgroup),OPT_XS,                 OPT_LINE(0),1,m_defshade,0,NULL,NULL,NULL},
    {DefLayer(0, "Mouse Setup", &mousesetupgroup),OPT_XS,              OPT_LINE(1),1,m_defshade,0,NULL,NULL,NULL},
    {DefLayer(0, "Joystick Buttons Setup", &joybuttonssetupgroup),OPT_XS,OPT_LINE(2),1,m_defshade,0,NULL,MNU_JoystickCheck,MNU_JoystickButtonsInitialise},
    {DefLayer(0, "Joystick Axes Setup", &joyaxessetupgroup), OPT_XS,   OPT_LINE(3),1,m_defshade,0,NULL,MNU_JoystickCheck,MNU_JoystickAxesInitialise},
    {DefLayer(0, "Advanced Mouse Setup", &mouseadvancedgroup),OPT_XS,  OPT_LINE(5),1,m_defshade,0,NULL,NULL,NULL},
    {DefOption(0, "Apply Modern Defaults"), OPT_XS,                    OPT_LINE(7),1,m_defshade,0,MNU_LoadModernDefaults,NULL,NULL},
    {DefOption(0, "Apply Classic Defaults"), OPT_XS,                   OPT_LINE(8),1,m_defshade,0,MNU_LoadClassicDefaults,NULL,NULL},
    {DefNone}
};
MenuGroup inputsetupgroup = {65, 5, "^Input Setup", inputsetup_i, pic_newgametitl, 0, m_defshade, NULL,NULL, 0};

MenuItem options_i[] =
{
    {DefLayer(0, "Screen Menu", &screengroup),OPT_XS,            OPT_LINE(0), 1, m_defshade,0,NULL, NULL, NULL},
    {DefLayer(0, "Mouse Menu", &mousegroup),OPT_XS,              OPT_LINE(1), 1, m_defshade,0,NULL, MNU_MouseCheck, NULL},
    {DefLayer(0, "Sound Menu", &soundgroup),OPT_XS,              OPT_LINE(2), 1, m_defshade,0,MNU_TryMusicInit, MNU_MusicFxCheck, NULL},
    {DefLayer(0, "Input Setup", &inputsetupgroup),OPT_XS,        OPT_LINE(3), 1,m_defshade,0,NULL,NULL,NULL},
#ifndef PLOCK_VERSION // No need for this in weener version
    {DefLayer(0, "Kid Mode", &parentalgroup),OPT_XS,             OPT_LINE(4), 1, m_defshade,0,NULL, NULL, NULL},
#endif
    {DefButton(btn_messages, 0, "Messages"), OPT_XS,             OPT_LINE(5), 1, m_defshade, 0, NULL, NULL, NULL},
//    {DefButton(btn_bobbing, 0, "View Bobbing"), OPT_XS,          OPT_LINE(7), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefButton(btn_shadows, 0, "Shadows"), OPT_XS,               OPT_LINE(6), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefButton(btn_auto_run, 0, "Auto Run"), OPT_XS,             OPT_LINE(7), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefButton(btn_crosshair, 0, "Crosshair"), OPT_XS,           OPT_LINE(8), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefButton(btn_auto_aim, 0, "Auto-Aiming"), OPT_XS,          OPT_LINE(9), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefButton(btn_voxels, 0, "Voxel Sprites"), OPT_XS,          OPT_LINE(10), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefButton(btn_stats, 0, "Level Stats"), OPT_XS,             OPT_LINE(11), 1, m_defshade, 0, NULL, MNU_StatCheck, NULL},
    {DefNone}
};

MenuGroup optiongroup = {100, 5, "^Options", options_i, pic_optionstitl, 0, m_defshade, NULL,NULL, 0};

MenuItem skill_i[] =
{
    {DefOption(KEYSC_E, &SkillNames[0][0]), 30, 46, pic_easy, m_defshade, 0, MNU_StartGame, NULL, NULL},
    {DefOption(KEYSC_N, &SkillNames[1][0]), 30, 62, pic_normal, m_defshade, 0, MNU_StartGame, NULL, NULL},
    {DefOption(KEYSC_H, &SkillNames[2][0]), 30, 78, pic_hard, m_defshade, 0, MNU_StartGame, NULL, NULL},
    {DefOption(KEYSC_I, &SkillNames[3][0]), 30, 94, pic_impossible, m_defshade, 0, MNU_StartGame, NULL, NULL},
    {DefNone}
};

MenuGroup skillgroup = {100, 5, "^Skill", skill_i, pic_newgametitl, 0, m_defshade, NULL, NULL, 2};

MenuItem episode_i[] =
{
    {DefLayer(KEYSC_S, &EpisodeNames[0][0], &skillgroup), 30, 46, pic_episode1, m_defshade, 0, MNU_EpisodeCustom, NULL, NULL},
    {DefLayer(KEYSC_F, &EpisodeNames[1][0], &skillgroup), 30, 78, pic_episode2, m_defshade, 0, MNU_EpisodeCustom, MNU_ShareWareCheck, MNU_ShareWareMessage},
    //{DefLayer(KEYSC_S, NULL, &skillgroup), 60, 30, pic_episode1, m_defshade, 0, MNU_EpisodeCustom},
    //{DefLayer(KEYSC_F, NULL, &skillgroup), 60, 46, pic_episode2, m_defshade, 0, MNU_EpisodeCustom},
    //{DefLayer(KEYSC_T, NULL, &skillgroup), 60, 62, pic_episode3, m_defshade, 0, MNU_EpisodeCustom},
    {DefNone}
};

MenuGroup episodegroup = {100, 5, "^Episode", episode_i, pic_newgametitl, 0, m_defshade, MNU_DoEpisodeSelect, NULL, 0};

extern char UserMapName[80];

MenuItem network_extra_i[] =
{
    {DefSlider(sldr_monsters, 0, "Monsters"),OPT_XS, OPT_LINE(0), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefInert(0, NULL),               OPT_XSIDE,OPT_LINE(0), 0, m_defshade, 0, NULL, NULL, NULL},

    {DefButton(btn_teamplay,     0, "Team Play"),    OPT_XS, OPT_LINE(1), 1, m_defshade, 0, MNU_TeamPlayChange, MNU_TeamPlayCheck, NULL},
    {DefButton(btn_friendlyfire, 0, "Hurt Teammate"),OPT_XS, OPT_LINE(2), 1, m_defshade, 0, NULL, MNU_HurtTeammateCheck, NULL},
    {DefButton(btn_nuke,         0, "Play with Nuke"),OPT_XS, OPT_LINE(3), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefNone}
};

MenuGroup networkextragroup = {50, 5, "^Net Options", network_extra_i, pic_newgametitl, 0, m_defshade, NULL, NULL, 0};

MenuItem network_i[] =
{
    {DefSlider(sldr_gametype, 0, "Game Type"),   OPT_XS,         OPT_LINE(0), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefInert(0, NULL),                          OPT_XSIDE-38,   OPT_LINE(0), 0, m_defshade, 0, NULL, NULL, NULL},

    {DefSlider(sldr_netlevel, 0, "Level"),       OPT_XS,         OPT_LINE(1), 1, m_defshade, 0, NULL, MNU_CheckUserMap, NULL},
    {DefInert(0, NULL),                          OPT_XSIDE-70,   OPT_LINE(1), 0, m_defshade, 0, NULL, MNU_CheckUserMap, NULL},
    {DefInert(0, " "),                           OPT_XS,         OPT_LINE(2), pic_episode1, m_defshade, 0, NULL, MNU_CheckUserMap, NULL},

    {DefButton(btn_markers,      0, "Markers"),  OPT_XS,         OPT_LINE(3), 1, m_defshade, 0, NULL, NULL, NULL},

    {DefSlider(sldr_killlimit, 0, "Kill Limit"),OPT_XS,          OPT_LINE(4), 1, m_defshade, 0, NULL, MNU_CoopPlayCheck, NULL},
    {DefInert(0, NULL),               OPT_XSIDE,                 OPT_LINE(4), 0, m_defshade, 0, NULL, MNU_CoopPlayCheck, NULL},

    {DefSlider(sldr_timelimit, 0, "Time Limit"),OPT_XS,          OPT_LINE(5), 1, m_defshade, 0, NULL, MNU_CoopPlayCheck, NULL},
    {DefInert(0, NULL),               OPT_XSIDE,                 OPT_LINE(5), 0, m_defshade, 0, NULL, MNU_CoopPlayCheck, NULL},

    {DefSlider(sldr_playercolor, 0, "Player Color"),OPT_XS,      OPT_LINE(6), 1, m_defshade, 0, NULL, NULL, NULL},
    {DefInert(0, NULL),               OPT_XSIDE,                 OPT_LINE(6), 0, m_defshade, 0, NULL, NULL, NULL},

    {DefLayer(0, "Other Options", &networkextragroup),OPT_XS,    OPT_LINE(7), 1, m_defshade,0,NULL, NULL, NULL},

    {DefInert(0, UserMapName),         OPT_XSIDE,                OPT_LINE(8), pic_episode1, m_defshade, 0, NULL, NULL, NULL},
    {DefOption(KEYSC_S, "Start Game"),   OPT_XS,                    OPT_LINE(8), pic_episode1, m_defshade, 0, MNU_StartNetGame, NULL, NULL},

    {DefNone}

};

MenuGroup networkgroup = {50, 5, "^Network Game", network_i, pic_newgametitl, 0, m_defshade, NULL, NULL, 0};


MenuItem load_i[] =
{
#define SD_YSTART 26
#define SD_XSTART 5
#define SD_YOFF 13
#define SD_LINE(line) (SD_YSTART + (line * SD_YOFF))
    {DefOption(0, NULL), SD_XSTART, SD_LINE(0), 0, m_defshade, 0, MNU_GetLoadCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(1), 0, m_defshade, 0, MNU_GetLoadCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(2), 0, m_defshade, 0, MNU_GetLoadCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(3), 0, m_defshade, 0, MNU_GetLoadCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(4), 0, m_defshade, 0, MNU_GetLoadCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(5), 0, m_defshade, 0, MNU_GetLoadCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(6), 0, m_defshade, 0, MNU_GetLoadCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(7), 0, m_defshade, 0, MNU_GetLoadCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(8), 0, m_defshade, 0, MNU_GetLoadCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(9), 0, m_defshade, 0, MNU_GetLoadCustom, NULL, NULL},
    {DefNone}
};

MenuItem save_i[] =
{
    {DefOption(0, NULL), SD_XSTART, SD_LINE(0), 0, m_defshade, 0, MNU_GetSaveCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(1), 0, m_defshade, 0, MNU_GetSaveCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(2), 0, m_defshade, 0, MNU_GetSaveCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(3), 0, m_defshade, 0, MNU_GetSaveCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(4), 0, m_defshade, 0, MNU_GetSaveCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(5), 0, m_defshade, 0, MNU_GetSaveCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(6), 0, m_defshade, 0, MNU_GetSaveCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(7), 0, m_defshade, 0, MNU_GetSaveCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(8), 0, m_defshade, 0, MNU_GetSaveCustom, NULL, NULL},
    {DefOption(0, NULL), SD_XSTART, SD_LINE(9), 0, m_defshade, 0, MNU_GetSaveCustom, NULL, NULL},
    {DefNone}
};

// No actual submenus for this, just quit text.
MenuGroup quitgroup = {0, 0, NULL, NULL, 0, 0, m_defshade, MNU_QuitCustom, NULL, 0};
MenuGroup quickloadgroup = {0, 0, NULL, NULL, 0, 0, m_defshade, MNU_QuickLoadCustom, NULL, 0};
MenuGroup ordergroup = {0, 0, NULL, NULL, 0, 0, m_defshade, MNU_OrderCustom, NULL, 0};

// save and load function calls
MenuGroup SaveGameGroup = {100, 5, "^Save Game", save_i, pic_savegame, 0, m_defshade, MNU_LoadSaveDraw, MNU_LoadSaveMove, 0};
MenuGroup LoadGameGroup = {100, 5, "^Load Game", load_i, pic_loadgame, 0, m_defshade, MNU_LoadSaveDraw, MNU_LoadSaveMove, 0};

#define MAIN_YSTART 32
#define MAIN_YOFF 17
#define MAIN_XSTART 55
#define MAIN_LINE(line) (MAIN_YSTART + (MAIN_YOFF * line))

#define MAIN_MENU_COOL_STUFF "^Cool Stuff"
#define MAIN_MENU_HOW_TO_ORDER "^How to Order"

MenuItem main_i[] =
{
    {DefLayer(KEYSC_N, "^New Game", &episodegroup),      MAIN_XSTART, MAIN_LINE(0), pic_newgame, m_defshade, 0, NULL, NULL, NULL},
    {DefLayer(KEYSC_L, "^Load Game", &LoadGameGroup),    MAIN_XSTART, MAIN_LINE(1), pic_load, m_defshade, 0, NULL, MNU_LoadGameCheck, NULL},
    {DefLayer(KEYSC_S, "^Save Game", &SaveGameGroup),    MAIN_XSTART, MAIN_LINE(2), pic_save, m_defshade, 0, NULL, MNU_SaveGameCheck, NULL},
    {DefLayer(KEYSC_O, "^Options", &optiongroup),        MAIN_XSTART, MAIN_LINE(3), pic_options, m_defshade, 0, NULL, NULL, NULL},
    {DefLayer(KEYSC_H, "^Oh Dear", &ordergroup),         MAIN_XSTART, MAIN_LINE(4), pic_orderinfo, m_defshade, 0, NULL, NULL, NULL},
    {DefLayer(KEYSC_Q, "^Quit", &quitgroup),             MAIN_XSTART, MAIN_LINE(5), pic_quit, m_defshade, 0, NULL, NULL, NULL},
    {DefNone}
};

MenuGroup maingroup = {160, 15, NULL, main_i, pic_shadow_warrior, 0, m_defshade, NULL, NULL, 0};

CTLType ControlPanelType;

#define MaxLayers    10         // Maximum layers deep a menu can go
short       menuarrayptr;       // Index into menuarray
MenuGroup   *menuarray[MaxLayers], *currentmenu;
SWBOOL UsingMenus = FALSE;

#define MAXDIALOG       2       // Maximum number of dialog strings allowed
const char *dialog[MAXDIALOG];

// Global menu setting values ////////////////////////////////////////////////////////////////////
// Mouse slider vars
#define SENSE_MIN               75
#define SENSE_MUL               10
int SENSITIVITY = SENSE_MIN + (SENSE_DEFAULT *SENSE_MUL);

// Sound vars
#define FX_MIN                  0
#define MUSIC_MIN               0
#define VOL_MUL                 16

// User input data for all devices
UserInput mnu_input, mnu_input_buffered, order_input_buffered;

// Menu function call back pointer for multiplay menus
SWBOOL(*cust_callback)(UserCall call, MenuItem_p item);
UserCall cust_callback_call;
MenuItem_p cust_callback_item;

// Prototypes ///////////////////////////////////////////////////////////////////////////////////

static void MNU_ClearDialog(void);
static SWBOOL MNU_Dialog(void);
void LoadSaveMsg(const char *msg);
static void MNU_ItemPreProcess(MenuGroup *group);
static void MNU_SelectItem(MenuGroup *group, short index, SWBOOL draw);
static void MNU_PushItem(MenuItem *item, SWBOOL draw);

static int MNU_ControlAxisOffset(int num);
static int MNU_ControlAxisNum(int offset);


// F U N C T I O N S ////////////////////////////////////////////////////////////////////////////

// CUSTOM ROUTINES ////////////////////////////////////////////////////////////////////////////////
// CTW REMOVED
/*
SWBOOL
MNU_Ten(void)
    {
    TEN_Setup();

    return(FALSE);
    }
*/
// CTW REMOVED END
SWBOOL
MNU_DoEpisodeSelect(UserCall call, MenuItem *item)
{
    short w,h;
    char TempString[80];
    char *extra_text;

    extra_text = EpisodeSubtitles[0];
    MNU_MeasureString(extra_text, &w, &h);
    MNU_DrawString(30, 63, extra_text, 1, 16);
    extra_text = EpisodeSubtitles[1];
    MNU_MeasureString(extra_text, &w, &h);
    MNU_DrawString(30, 96, extra_text, 1, 16);

    return TRUE;
}

SWBOOL
MNU_DoParentalPassword(UserCall call, MenuItem_p item)
{
    short w,h;
    signed char MNU_InputString(char *, short);
    static SWBOOL cur_show;
    char TempString[80];
    const char *extra_text;


    extra_text = "This mode should remove most of the";
    MNU_MeasureString(extra_text, &w, &h);
    MNU_DrawString(TEXT_XCENTER(w), 60, extra_text, 1, 16);
    extra_text = "offensive content.  We still recommend";
    MNU_MeasureString(extra_text, &w, &h);
    MNU_DrawString(TEXT_XCENTER(w), 70, extra_text, 1, 16);
    extra_text = "you review the game prior to play.";
    MNU_MeasureString(extra_text, &w, &h);
    MNU_DrawString(TEXT_XCENTER(w), 80, extra_text, 1, 16);

    // get input
    if (MenuInputMode)
    {
        switch (MNU_InputString(MessageInputString, 80))
        {
        case -1: // Cancel Input (pressed ESC) or Err
            KB_ClearKeysDown();
            KB_FlushKeyboardQueue();
            MenuInputMode = FALSE;
            memset(MessageInputString, '\0', sizeof(MessageInputString));
            break;
        case FALSE: // Input finished (RETURN)
            if (MessageInputString[0] == '\0')
            {
                MenuInputMode = FALSE;
                KB_ClearKeysDown();
                KB_FlushKeyboardQueue();
                memset(MessageInputString, '\0', sizeof(MessageInputString));
            }
            else
            {
                MenuInputMode = FALSE;
                KB_ClearKeysDown();
                KB_FlushKeyboardQueue();

                if (gs.Password[0] != '\0' && passwordvalid == FALSE)
                {
                    if (!Bstrcasecmp(gs.Password,MessageInputString))
                    {
                        passwordvalid = TRUE;
                        if (currentmenu->cursor == 0 && gs.ParentalLock == TRUE)
                        {
                            buttonsettings[btn_parental] = gs.ParentalLock = FALSE;
                            if (!InMenuLevel)
                                JS_ToggleLockouts();
                        }

                        if (currentmenu->cursor == 1) // Is it on the password line?
                        {
                            MenuInputMode = TRUE;
                            KB_ClearKeysDown();
                            KB_FlushKeyboardQueue();
                        }
                        //memset(gs.Password, '\0', sizeof(gs.Password));
                    }
                }
                else
                {
                    if (currentmenu->cursor == 1) // Is it on the password line?
                    {
                        strcpy(gs.Password,MessageInputString);
                        passwordvalid = FALSE;
                    }
                }

                memset(MessageInputString, '\0', sizeof(MessageInputString));
            }
            break;
        case TRUE: // Got input
            break;
        }

        //CON_Message("Password = '%s'",gs.Password);
        //CON_Message("Passwordvalid = %d",passwordvalid);

        if (gs.Password[0] != '\0' && passwordvalid == FALSE && currentmenu->cursor == 1)
        {
            sprintf(TempString,"Enter Old Password");
            MNU_MeasureString(TempString, &w, &h);
            MNU_DrawString(TEXT_XCENTER(w), MESSAGE_LINE-10, TempString,1,16);
        }
        else if (passwordvalid == TRUE && currentmenu->cursor == 1)
        {
            sprintf(TempString,"Enter New Password");
            MNU_MeasureString(TempString, &w, &h);
            MNU_DrawString(TEXT_XCENTER(w), MESSAGE_LINE-10, TempString,1,16);
        }
        else
        {
            sprintf(TempString,"Enter Password");
            MNU_MeasureString(TempString, &w, &h);
            MNU_DrawString(TEXT_XCENTER(w), MESSAGE_LINE-10, TempString,1,16);
        }

        MNU_MeasureString(MessageInputString, &w, &h);

        cur_show ^= 1;
        if (cur_show)
        {
            MNU_DrawString(TEXT_XCENTER(w), MESSAGE_LINE, MessageInputString,1,16);
            rotatesprite((TEXT_XCENTER(w)+w+7)<<16,(MESSAGE_LINE+3)<<16,64<<9,0,COINCURSOR+(((int32_t) totalclock>>3)%7),0,0,MenuDrawFlags,0,0,xdim-1,ydim-1);
        }
        else
        {
            MNU_DrawString(TEXT_XCENTER(w), MESSAGE_LINE, MessageInputString,1,16);
            rotatesprite((TEXT_XCENTER(w)+w+7)<<16,(MESSAGE_LINE+3)<<16,64<<9,0,COINCURSOR+(((int32_t) totalclock>>3)%7),0,0,MenuDrawFlags,0,0,xdim-1,ydim-1);
        }

    }

    return TRUE;
}

SWBOOL
MNU_ParentalCustom(void)
{

    if (MenuInputMode)
    {
        // toggle edit mode
        MenuInputMode = FALSE;
        memset(MessageInputString, '\0', sizeof(MessageInputString));
    }
    else
    {
        // clear keyboard buffer
        while (KB_KeyWaiting())
        {
            if (KB_GetCh() == 0)
                KB_GetCh();
        }

        // toggle edit mode
        MenuInputMode = TRUE;
    }

    return TRUE;
}

SWBOOL MNU_KeySetupCustom(UserCall call, MenuItem *item)
{
    static int currentkey = 0, currentcol = 0;
    static int currentmode = 0;

    if (call == uc_touchup)
        return TRUE;

    if (cust_callback == NULL)
    {
        if (call != uc_setup)
            return FALSE;
        currentkey = 0;
        currentcol = 0;
        currentmode = 0;
    }

    cust_callback = MNU_KeySetupCustom;
    cust_callback_call = call;
    cust_callback_item = item;

    {
        short w, h = 0;
        const char *s = "Keys Setup";
        rotatesprite(10 << 16, (5-3) << 16, MZ, 0, 2427,
                     m_defshade, 0, MenuDrawFlags|ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);
        MNU_MeasureStringLarge(s, &w, &h);
        MNU_DrawStringLarge(TEXT_XCENTER(w), 5, s);
    }

    if (currentmode)
    {
        // customising a key
        const char *strs[] = { "Press the key to assign to", "\"%s\" %s", "or ESCAPE to cancel." };
        const char *col[2] = { "(primary)", "(secondary)" };
        short w, h = 8;
        int i, j, y;

        if (KEY_PRESSED(KEYSC_ESC))
        {
            KB_ClearKeyDown(sc_Escape);
            currentmode = 0;
        }
        else if (KB_GetLastScanCode() > 0)
        {
            KB_ClearKeyDown(KB_GetLastScanCode());

            KeyboardKeys[currentkey][currentcol] = KB_GetLastScanCode();
            if (currentkey != gamefunc_Show_Console)
            {
#if 0 // [JM] Re-do this shit !CHECKME!
                CONTROL_MapKey(currentkey,
                               KeyboardKeys[currentkey][0],
                               KeyboardKeys[currentkey][1]);
#endif
            }
            else
            {
                OSD_CaptureKey(KB_GetLastScanCode());
            }

            currentmode = 0;
        }

        MNU_MeasureString(strs[0], &w, &h);

        y = (YDIM - (h+3) * SIZ(strs)) / 2;

        for (i=0; i<(int)SIZ(strs); i++)
        {
            w = 0;
            sprintf(ds,strs[i],gamefunctions[currentkey],col[currentcol]);
            for (j=0; ds[j]; j++) if (ds[j] == '_') ds[j] = ' ';
            MNU_MeasureString(ds, &w, &h);
            MNU_DrawString((XDIM - w)/2, y, ds, 0, 16);
            y += h+3;
        }

    }
    else
    {
        // key list
#define PGSIZ 14
        int topitem = 0, botitem = NUMGAMEFUNCTIONS;
        int i,j;
        const char *morestr = "More...";
        const char *p;

        UserInput inpt = {FALSE,FALSE,dir_None};
        CONTROL_GetUserInput(&inpt);

        if (KEY_PRESSED(KEYSC_ESC) || inpt.button1)
        {
            KEY_PRESSED(KEYSC_ESC) = FALSE;
            cust_callback = NULL;
            CONTROL_ClearUserInput(&inpt);
            return TRUE;
        }
        else if (KB_KeyPressed(sc_Delete))
        {
            KB_ClearKeyDown(sc_Delete);
            if (currentkey != gamefunc_Show_Console)
            {
                KeyboardKeys[currentkey][currentcol] = 0xff;
#if 0 // [JM] Re-do this shit !CHECKME!
                CONTROL_MapKey(currentkey,
                               KeyboardKeys[currentkey][0],
                               KeyboardKeys[currentkey][1]);
#endif
            }
        }
        else if (KB_KeyPressed(sc_Home))
        {
            currentkey = 0;
            KB_ClearKeyDown(sc_Home);
        }
        else if (KB_KeyPressed(sc_End))
        {
            currentkey = NUMGAMEFUNCTIONS-1;
            KB_ClearKeyDown(sc_End);
        }
        else if (KB_KeyPressed(sc_PgDn))
        {
            currentkey += PGSIZ;
            if (currentkey >= NUMGAMEFUNCTIONS) currentkey = NUMGAMEFUNCTIONS-1;
            KB_ClearKeyDown(sc_PgDn);
        }
        else if (KB_KeyPressed(sc_PgUp))
        {
            currentkey -= PGSIZ;
            if (currentkey < 0) currentkey = 0;
            KB_ClearKeyDown(sc_PgUp);
        }
        else if (inpt.button0)
        {
            currentmode = 1;
            KB_ClearLastScanCode();
            KB_ClearKeysDown();
        }
        else if (inpt.dir == dir_North) currentkey = max(0,currentkey-1);
        else if (inpt.dir == dir_South) currentkey = min(NUMGAMEFUNCTIONS-1,currentkey+1);
        else if (inpt.dir == dir_East) currentcol = 1;
        else if (inpt.dir == dir_West) currentcol = 0;

        if (currentkey == gamefunc_Show_Console) currentcol = 0;

        CONTROL_ClearUserInput(&inpt);

        if (NUMGAMEFUNCTIONS > PGSIZ)
        {
            topitem = currentkey - PGSIZ/2;
            botitem = topitem + PGSIZ;

            if (topitem < 0)
            {
                botitem += -topitem;
                topitem = 0;
            }
            else if (botitem >= NUMGAMEFUNCTIONS)
            {
                botitem = NUMGAMEFUNCTIONS-1;
                topitem = botitem - PGSIZ;
            }
        }

        for (i = topitem; i <= botitem; i++)
        {
            for (j = 0; gamefunctions[i][j]; j++)
            {
                if (gamefunctions[i][j] == '_') ds[j] = ' ';
                else ds[j] = gamefunctions[i][j];
            }
            ds[j] = 0;

            j = OPT_LINE(0)+(i-topitem)*8;
            MNU_DrawSmallString(OPT_XS, j, ds, (i==currentkey) ? 0 : 12, 16);

            p = keyGetName(KeyboardKeys[i][0]);
            if (!p || KeyboardKeys[i][0]==0xff) p = "  -";
            MNU_DrawSmallString(OPT_XSIDE, j, p, (i==currentkey) ? -5 : 12,
                                (i==currentkey && currentcol==0) ? 14 : 16);

            if (i == gamefunc_Show_Console) continue;

            p = keyGetName(KeyboardKeys[i][1]);
            if (!p || KeyboardKeys[i][1]==0xff) p = "  -";
            MNU_DrawSmallString(OPT_XSIDE + 4*14, j, p, (i==currentkey) ? -5 : 12,
                                (i==currentkey && currentcol==1) ? 14 : 16);
        }

        {
            short dx,dy;
            dx = 0, dy = 8;
            MNU_MeasureSmallString(morestr,&dx,&dy);
            if (topitem > 0)
                MNU_DrawSmallString(XDIM - OPT_XS - dx, OPT_LINE(0), morestr, 8,16);
            if (botitem < NUMGAMEFUNCTIONS-1)
                MNU_DrawSmallString(XDIM - OPT_XS - dx, OPT_LINE(0)+PGSIZ*8, morestr, 8,16);
        }
#undef PGSIZ
    }

    return TRUE;
}

static int MNU_SelectButtonFunction(const char *buttonname, int *currentfunc)
{
    const int PGSIZ = 9;
    const char *strs[] = { "Select the function to assign to", "%s", "or ESCAPE to cancel." };
    int topitem = 0, botitem = NUMGAMEFUNCTIONS-1;
    int i, j, y;
    short w, h=0;
    int returnval = 0;

    UserInput inpt = {FALSE,FALSE,dir_None};
    CONTROL_GetUserInput(&inpt);

    if (inpt.button1)
    {
        KB_ClearKeyDown(sc_Escape);
        returnval = -1;
    }
    else if (KB_KeyPressed(sc_Home))
    {
        *currentfunc = 0;
        KB_ClearKeyDown(sc_Home);
    }
    else if (KB_KeyPressed(sc_End))
    {
        *currentfunc = NUMGAMEFUNCTIONS-1;   // -1 because the last one is the console and the top is 'none'
        KB_ClearKeyDown(sc_End);
    }
    else if (KB_KeyPressed(sc_PgDn))
    {
        *currentfunc += PGSIZ;
        if (*currentfunc >= NUMGAMEFUNCTIONS) *currentfunc = NUMGAMEFUNCTIONS-1;
        KB_ClearKeyDown(sc_PgDn);
    }
    else if (KB_KeyPressed(sc_PgUp))
    {
        *currentfunc -= PGSIZ;
        if (*currentfunc < 0) *currentfunc = 0;
        KB_ClearKeyDown(sc_PgUp);
    }
    else if (inpt.button0)
    {
        returnval = 1;
    }
    else if (inpt.dir == dir_North) *currentfunc = max(0, *currentfunc-1);
    else if (inpt.dir == dir_South) *currentfunc = min(NUMGAMEFUNCTIONS-1, *currentfunc+1);

    CONTROL_ClearUserInput(&inpt);

    if (NUMGAMEFUNCTIONS-1 > PGSIZ)
    {
        topitem = *currentfunc - PGSIZ/2;
        botitem = topitem + PGSIZ;

        if (topitem < 0)
        {
            botitem += -topitem;
            topitem = 0;
        }
        else if (botitem >= NUMGAMEFUNCTIONS)
        {
            botitem = NUMGAMEFUNCTIONS-1;
            topitem = botitem - PGSIZ;
        }
    }

    y = OPT_LINE(0);
    for (i=0; i<(int)SIZ(strs); i++)
    {
        w = 0;
        sprintf(ds, strs[i], buttonname);
        for (j=0; ds[j]; j++) if (ds[j] == '_') ds[j] = ' ';
        MNU_MeasureString(ds, &w, &h);
        MNU_DrawString((XDIM - w)/2, y, ds, 0, 16);
        y += h;
    }

    for (i = topitem; i <= botitem; i++)
    {
        if (i == 0)
        {
            strcpy(ds, "  -none-");
        }
        else
        {
            for (j = 0; gamefunctions[i-1][j]; j++)
            {
                if (gamefunctions[i-1][j] == '_') ds[j] = ' ';
                else ds[j] = gamefunctions[i-1][j];
            }
            ds[j] = 0;
        }

        j = OPT_LINE(4)+(i-topitem)*8;
        MNU_DrawSmallString(130, j, ds, (i == *currentfunc) ? 0 : 12, 16);
    }

    {
        short dx = 0, dy = 8;
        const char *morestr = "More...";

        MNU_MeasureSmallString(morestr,&dx,&dy);
        if (topitem > 0)
            MNU_DrawSmallString(XDIM - OPT_XS - dx, OPT_LINE(4), morestr, 8,16);
        if (botitem < NUMGAMEFUNCTIONS-1)
            MNU_DrawSmallString(XDIM - OPT_XS - dx, OPT_LINE(4)+PGSIZ*8, morestr, 8,16);
    }

    return returnval;
}


static MenuItem_p mouse_button_item = NULL;

static SWBOOL MNU_MouseButtonPostProcess(MenuItem_p item)
{
    mouse_button_item = item;
    return TRUE;
}

SWBOOL MNU_MouseButtonSetupCustom(UserCall call, MenuItem_p item)
{
    static int currentfunc = 0;

    if (call == uc_touchup)
        return TRUE;

    if (cust_callback == NULL)
    {
        if (call != uc_setup)
            return FALSE;
        if (mouse_button_item->tics >= 6)
        {
            currentfunc = MouseButtonsClicked[mouse_button_item->tics % 6];
        }
        else
        {
            currentfunc = MouseButtons[mouse_button_item->tics % 6];
        }
        currentfunc++;

        cust_callback = MNU_MouseButtonSetupCustom;
        cust_callback_call = call;
        cust_callback_item = item;
    }

    {
        short w, h = 0;
        const char *s = "Mouse Setup";

        rotatesprite(10 << 16, (5-3) << 16, MZ, 0, 2427,
                     m_defshade, 0, MenuDrawFlags|ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);
        MNU_MeasureStringLarge(s, &w, &h);
        MNU_DrawStringLarge(TEXT_XCENTER(w), 5, s);
    }

    int selection = MNU_SelectButtonFunction(mouse_button_item->text, &currentfunc);
    switch (selection)
    {
    case -1:    //cancel
        cust_callback = NULL;
        break;
    case 1:     //acknowledge
        currentfunc--;
        if (mouse_button_item->tics >= 6)
        {
            MouseButtonsClicked[mouse_button_item->tics % 6] = currentfunc;
            CONTROL_MapButton(currentfunc, mouse_button_item->tics % 6, 1, controldevice_mouse);
        }
        else
        {
            MouseButtons[mouse_button_item->tics % 6] = currentfunc;
            CONTROL_MapButton(currentfunc, mouse_button_item->tics % 6, 0, controldevice_mouse);
        }
        MNU_SetMouseButtonFunctions(mouse_button_item);
        cust_callback = NULL;
        break;
    default: break;
    }

    return TRUE;
}

static SWBOOL MNU_SetMouseButtonFunctions(MenuItem_p item)
{
    int button, clicked, function;
    char *p;

    clicked = item->tics >= 6;
    button = item->tics % 6;
    ASSERT(button >= 0 && button <= 5);

    if (clicked)
    {
        function = MouseButtonsClicked[button];
    }
    else
    {
        function = MouseButtons[button];
    }

    if (function < 0)
    {
        strcpy(MouseButtonFunctions[item->tics], "  -");
    }
    else
    {
        strcpy(MouseButtonFunctions[item->tics], CONFIG_FunctionNumToName(function));
        for (p = MouseButtonFunctions[item->tics]; *p; p++)
        {
            if (*p == '_')
                *p = ' ';
        }
    }
    return TRUE;
}


static MenuItem_p mouse_digital_item = NULL;

static SWBOOL MNU_MouseDigitalPostProcess(MenuItem_p item)
{
    mouse_digital_item = item;
    return TRUE;
}

static SWBOOL MNU_MouseDigitalSetupCustom(UserCall call, MenuItem_p item)
{
    static int currentfunc = 0;

    if (call == uc_touchup)
        return TRUE;

    if (cust_callback == NULL)
    {
        if (call != uc_setup)
            return FALSE;
        currentfunc = MouseDigitalAxes[mouse_digital_item->tics/2][mouse_digital_item->tics%2];
        currentfunc++;

        cust_callback = MNU_MouseDigitalSetupCustom;
        cust_callback_call = call;
        cust_callback_item = item;
    }

    {
        short w, h = 0;
        const char *s = "Adv'd Mouse";

        rotatesprite(10 << 16, (5-3) << 16, MZ, 0, 2427,
                     m_defshade, 0, MenuDrawFlags|ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);
        MNU_MeasureStringLarge(s, &w, &h);
        MNU_DrawStringLarge(TEXT_XCENTER(w), 5, s);
    }

    int selection = MNU_SelectButtonFunction(mouse_digital_item->text, &currentfunc);
    switch (selection)
    {
    case -1:    //cancel
        cust_callback = NULL;
        break;
    case 1:     //acknowledge
        currentfunc--;
        MouseDigitalAxes[mouse_digital_item->tics/2][mouse_digital_item->tics%2] = currentfunc;
        CONTROL_MapDigitalAxis(mouse_digital_item->tics/2, currentfunc, mouse_digital_item->tics%2, controldevice_mouse);
        MNU_SetAdvancedMouseFunctions(mouse_digital_item);
        cust_callback = NULL;
        break;
    default: break;
    }

    return TRUE;
}

static SWBOOL MNU_SetAdvancedMouseFunctions(MenuItem_p item)
{
    int axis;
    char *p;

    axis = item->tics;
    ASSERT(axis >= 0 && axis < 4);

    if (MouseDigitalAxes[axis/2][axis%2] < 0)
    {
        strcpy(AdvancedMouseAxisFunctions[axis], "  -");
    }
    else
    {
        strcpy(AdvancedMouseAxisFunctions[axis], CONFIG_FunctionNumToName(MouseDigitalAxes[axis/2][axis%2]));
        for (p = AdvancedMouseAxisFunctions[axis]; *p; p++)
        {
            if (*p == '_')
                *p = ' ';
        }
    }
    return TRUE;
}


static MenuItem_p joystick_button_item = NULL;

static SWBOOL MNU_JoystickButtonsInitialise(MenuItem_p mitem)
{
    MenuItem_p item;
    MenuItem templayer = { DefLayer(0, JoystickButtonNames[0], &joybuttonsgroup), OPT_XS, OPT_LINE(0), 1, m_defshade, 0, NULL, NULL, MNU_JoystickButtonPostProcess };
    MenuItem tempinert = { DefInert(0, JoystickButtonFunctions[0]), OPT_XSIDE, OPT_LINE(0), 1, m_defshade, 0, NULL, MNU_SetJoystickButtonFunctions, NULL };
    MenuItem temppagename = { DefInert(0, JoystickButtonPageName[0]), OPT_XS, OPT_LINE(JOYSTICKITEMSPERPAGE+1), 1, m_defshade, 0, NULL, NULL, NULL };
    MenuItem tempnextpage = { DefOption(0, "Next..."), OPT_XSIDE, OPT_LINE(JOYSTICKITEMSPERPAGE+1), 1, m_defshade, 0, MNU_JoystickButtonNextPage, NULL, NULL };
    MenuItem tempnone = { DefNone };
    const char *hatdirs[] = { " Up", " Right", " Down", " Left" };
    int button, page, pageitem;

    if (joybuttonssetupgroup.items != NULL)
    {
        return TRUE;
    }

    page = 0;
    pageitem = 0;
    joybuttonssetupgroup.items = joybuttons_i[0];
    item = &joybuttons_i[0][0];

    for (button = 0; button < joystick.numButtons * 2 + (joystick.numHats > 0) * 4; )
    {
        if (button < joystick.numButtons * 2)
        {
            int dbutton = button / 2;

            strcpy(JoystickButtonNames[dbutton], joyGetName(1, dbutton));

            templayer.text = JoystickButtonNames[dbutton];
            templayer.y = OPT_LINE(pageitem);
            templayer.tics = dbutton;
            memcpy(item, &templayer, sizeof(MenuItem));
            item++;

            tempinert.text = JoystickButtonFunctions[dbutton];
            tempinert.y = OPT_LINE(pageitem);
            tempinert.tics = dbutton;
            memcpy(item, &tempinert, sizeof(MenuItem));
            item++;

            pageitem++;

            strcpy(JoystickButtonNames[dbutton + MAXJOYBUTTONS], "Double ");
            strcat(JoystickButtonNames[dbutton + MAXJOYBUTTONS], joyGetName(1, dbutton));

            templayer.text = JoystickButtonNames[dbutton + MAXJOYBUTTONS];
            templayer.y = OPT_LINE(pageitem);
            templayer.tics = 128 | dbutton;
            memcpy(item, &templayer, sizeof(MenuItem));
            item++;

            tempinert.text = JoystickButtonFunctions[dbutton + MAXJOYBUTTONS];
            tempinert.y = OPT_LINE(pageitem);
            tempinert.tics = 128 | dbutton;
            memcpy(item, &tempinert, sizeof(MenuItem));
            item++;

            pageitem++;

            button += 2;
        }
        else
        {
            int dir = button - joystick.numButtons * 2;
            int dbutton = joystick.numButtons + dir;

            strcpy(JoystickButtonNames[dbutton], joyGetName(2, 0));
            strcat(JoystickButtonNames[dbutton], hatdirs[dir]);

            templayer.text = JoystickButtonNames[dbutton];
            templayer.y = OPT_LINE(pageitem);
            templayer.tics = dbutton;
            memcpy(item, &templayer, sizeof(MenuItem));
            item++;

            tempinert.text = JoystickButtonFunctions[dbutton];
            tempinert.y = OPT_LINE(pageitem);
            tempinert.tics = dbutton;
            memcpy(item, &tempinert, sizeof(MenuItem));
            item++;

            pageitem++;
            button++;
        }

        if (pageitem == JOYSTICKITEMSPERPAGE || button == joystick.numButtons * 2 + (joystick.numHats > 0) * 4)
        {
            // next page
            sprintf(JoystickButtonPageName[page], "Page %d / %d", page+1,
                    ((joystick.numButtons * 2 + (joystick.numHats > 0) * 4) / JOYSTICKITEMSPERPAGE) + 1);

            temppagename.text = JoystickButtonPageName[page];
            memcpy(item, &temppagename, sizeof(MenuItem));
            item++;

            memcpy(item, &tempnextpage, sizeof(MenuItem));
            item++;

            memcpy(item, &tempnone, sizeof(MenuItem));

            page++;
            pageitem = 0;
            item = &joybuttons_i[page][0];
        }
    }

    return TRUE;
}

static SWBOOL MNU_JoystickButtonPostProcess(MenuItem_p item)
{
    joystick_button_item = item;
    return TRUE;
}

static SWBOOL MNU_JoystickButtonSetupCustom(UserCall call, MenuItem *item)
{
    static int currentfunc = 0;

    if (call == uc_touchup)
        return TRUE;

    if (cust_callback == NULL)
    {
        if (call != uc_setup)
            return FALSE;

        if (joystick_button_item->tics & 128)
        {
            currentfunc = JoystickButtonsClicked[joystick_button_item->tics & 127];
        }
        else
        {
            currentfunc = JoystickButtons[joystick_button_item->tics & 127];
        }
        currentfunc++;

        cust_callback = MNU_JoystickButtonSetupCustom;
        cust_callback_call = call;
        cust_callback_item = item;
    }

    {
        short w, h = 0;
        const char *s = "Joystick Setup";

        rotatesprite(10 << 16, (5-3) << 16, MZ, 0, 2427,
                     m_defshade, 0, MenuDrawFlags|ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);
        MNU_MeasureStringLarge(s, &w, &h);
        MNU_DrawStringLarge(TEXT_XCENTER(w), 5, s);
    }

    int selection = MNU_SelectButtonFunction(joystick_button_item->text, &currentfunc);
    switch (selection)
    {
    case -1:    //cancel
        cust_callback = NULL;
        break;
    case 1:     //acknowledge
        currentfunc--;
        if (joystick_button_item->tics & 128)
        {
            JoystickButtonsClicked[joystick_button_item->tics & 127] = currentfunc;
            CONTROL_MapButton(currentfunc, joystick_button_item->tics & 127, 1, controldevice_joystick);
        }
        else
        {
            JoystickButtons[joystick_button_item->tics & 127] = currentfunc;
            CONTROL_MapButton(currentfunc, joystick_button_item->tics & 127, 0, controldevice_joystick);
        }
        MNU_SetJoystickButtonFunctions(joystick_button_item);
        cust_callback = NULL;
        break;
    default: break;
    }

    return TRUE;
}

static SWBOOL MNU_JoystickButtonNextPage(void)
{
    JoystickButtonPage = (JoystickButtonPage + 1) % (((joystick.numButtons * 2 + (joystick.numHats > 0) * 4) / JOYSTICKITEMSPERPAGE) + 1);
    joybuttonssetupgroup.items = &joybuttons_i[JoystickButtonPage][0];
    joybuttonssetupgroup.cursor = 0;
    MNU_ItemPreProcess(&joybuttonssetupgroup);
    return TRUE;
}

static SWBOOL MNU_SetJoystickButtonFunctions(MenuItem_p item)
{
    int button, clicked, function;
    char *p;

    clicked = (item->tics & 128) > 0;
    button = item->tics & 127;
    ASSERT(button >= 0 && button < MAXJOYBUTTONS);

    if (clicked)
    {
        function = JoystickButtonsClicked[button];
    }
    else
    {
        function = JoystickButtons[button];
    }
    if (function < 0)
    {
        strcpy(JoystickButtonFunctions[button + clicked*MAXJOYBUTTONS], "  -");
    }
    else
    {
        strcpy(JoystickButtonFunctions[button + clicked*MAXJOYBUTTONS], CONFIG_FunctionNumToName(function));
        for (p = JoystickButtonFunctions[button + clicked*MAXJOYBUTTONS]; *p; p++)
        {
            if (*p == '_')
                *p = ' ';
        }
    }
    return TRUE;
}


static MenuItem_p joystick_axis_item = NULL;

static SWBOOL MNU_JoystickAxesInitialise(MenuItem_p mitem)
{
    if (!CONTROL_JoyPresent)
    {
        return TRUE;
    }
    if (JoystickAxisPage < 0 || JoystickAxisPage >= joystick.numAxes)
    {
        JoystickAxisPage = 0;
    }

    strcpy(JoystickAxisName, joyGetName(0, JoystickAxisPage));
    sprintf(JoystickAxisPageName, "Page %d / %d", JoystickAxisPage+1, joystick.numAxes);
    slidersettings[sldr_joyaxisanalog] = MNU_ControlAxisOffset(JoystickAnalogAxes[JoystickAxisPage]);
    slidersettings[sldr_joyaxisscale] = JoystickAnalogScale[JoystickAxisPage] >> 13;
    slidersettings[sldr_joyaxisdead] = JoystickAnalogDead[JoystickAxisPage] >> 10;
    slidersettings[sldr_joyaxissatur] = JoystickAnalogSaturate[JoystickAxisPage] >> 10;

    return TRUE;
}

static SWBOOL MNU_JoystickAxisPostProcess(MenuItem_p item)
{
    joystick_axis_item = item;
    return TRUE;
}

static SWBOOL MNU_JoystickAxisSetupCustom(UserCall call, MenuItem *item)
{
    static int currentfunc = 0;

    if (call == uc_touchup)
        return TRUE;

    if (cust_callback == NULL)
    {
        if (call != uc_setup)
            return FALSE;

        currentfunc = JoystickDigitalAxes[JoystickAxisPage][joystick_axis_item->tics];
        currentfunc++;

        cust_callback = MNU_JoystickAxisSetupCustom;
        cust_callback_call = call;
        cust_callback_item = item;
    }

    {
        short w, h = 0;
        const char *s = "Joystick Axes";

        rotatesprite(10 << 16, (5-3) << 16, MZ, 0, 2427,
                     m_defshade, 0, MenuDrawFlags|ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);
        MNU_MeasureStringLarge(s, &w, &h);
        MNU_DrawStringLarge(TEXT_XCENTER(w), 5, s);
    }

    int selection = MNU_SelectButtonFunction(joystick_axis_item->text, &currentfunc);
    switch (selection)
    {
    case -1:    //cancel
        cust_callback = NULL;
        break;
    case 1:     //acknowledge
        currentfunc--;
        JoystickDigitalAxes[JoystickAxisPage][joystick_axis_item->tics] = currentfunc;
        CONTROL_MapDigitalAxis(JoystickAxisPage, currentfunc, joystick_axis_item->tics, controldevice_joystick);
        MNU_SetJoystickAxisFunctions(joystick_axis_item);
        cust_callback = NULL;
        break;
    default: break;
    }

    return TRUE;
}

static SWBOOL MNU_JoystickAxisNextPage(void)
{
    JoystickAxisPage = (JoystickAxisPage + 1) % MNU_ControlAxisOffset(analog_maxtype);
    joyaxessetupgroup.cursor = 1;
    MNU_ItemPreProcess(&joyaxessetupgroup);
    MNU_JoystickAxesInitialise(NULL);
    return TRUE;
}

static SWBOOL MNU_SetJoystickAxisFunctions(MenuItem_p item)
{
    int function;
    char *p;

    function = JoystickDigitalAxes[JoystickAxisPage][item->tics];
    if (function < 0)
    {
        strcpy(JoystickAxisFunctions[item->tics], "  -");
    }
    else
    {
        strcpy(JoystickAxisFunctions[item->tics], CONFIG_FunctionNumToName(function));
        for (p = JoystickAxisFunctions[item->tics]; *p; p++)
        {
            if (*p == '_')
                *p = ' ';
        }
    }
    return TRUE;
}


SWBOOL
MNU_OrderCustom(UserCall call, MenuItem *item)
{
    static signed char on_screen = 0,last_screen = 0;
    UserInput order_input;
    static int limitmove=0;
    UserInput tst_input;
    SWBOOL select_held = FALSE;
    int zero = 0;
    static SWBOOL DidOrderSound = FALSE;
    short choose_snd;
    static int wanghandle;

    static short RegOrderScreen[] =
    {
        5262,
        5261,
        4979,
        5111,
        5118,
        5113,
        //5111,
        //5118,
        //4979,
        //5261,
        //5262

        5114    // JBF: for my credits
    };
    static short SWOrderScreen[] =
    {
        5262,
        5110,
        5112,
        5113,
        5111,
        5118,
        4979,

        5114    // JBF: for my credits
    };
    short *OrderScreen, OrderScreenSiz;

    if (SW_SHAREWARE)
    {
        OrderScreen = SWOrderScreen;
        OrderScreenSiz = SIZ(SWOrderScreen);
    }
    else
    {
        OrderScreen = RegOrderScreen;
        OrderScreenSiz = SIZ(RegOrderScreen);
    }

    // Ignore the special touchup calls
    if (call == uc_touchup)
        return TRUE;

    if (cust_callback == NULL)
    {
        if (call != uc_setup)
            return FALSE;
    }

    if (SW_SHAREWARE && on_screen == 0 && !DidOrderSound)
    {
        DidOrderSound = TRUE;
        choose_snd = STD_RANDOM_RANGE(1000);
        if (choose_snd > 500 && !FX_SoundActive(wanghandle))
            wanghandle = PlaySound(DIGI_WANGORDER1, &zero, &zero, &zero, v3df_dontpan);
        else if (!FX_SoundActive(wanghandle))
            wanghandle = PlaySound(DIGI_WANGORDER2, &zero, &zero, &zero, v3df_dontpan);
    }

    order_input.button0 = order_input.button1 = FALSE;
    order_input.dir = dir_None;

    // Zero out the input structure
    tst_input.button0 = tst_input.button1 = FALSE;
    tst_input.dir = dir_None;

    if (!select_held)
    {
        CONTROL_GetUserInput(&tst_input);
        //order_input_buffered.dir = tst_input.dir;
        // Support a few other keys too
        if (KEY_PRESSED(KEYSC_SPACE)||KEY_PRESSED(KEYSC_ENTER))
        {
            KEY_PRESSED(KEYSC_SPACE) = FALSE;
            KEY_PRESSED(KEYSC_ENTER) = FALSE;
            tst_input.dir = dir_South;
        }
    }

    if (order_input_buffered.button0 || order_input_buffered.button1 || order_input_buffered.dir != dir_None)
    {
        if (tst_input.button0 == order_input_buffered.button0 &&
            tst_input.button1 == order_input_buffered.button1 &&
            tst_input.dir == order_input_buffered.dir)
        {
            select_held = TRUE;
        }
        else
        {
            if (labs((int32_t) totalclock - limitmove) > 7)
            {
                order_input.button0 = order_input_buffered.button0;
                order_input.button1 = order_input_buffered.button1;
                order_input.dir = order_input_buffered.dir;

                order_input_buffered.button0 = tst_input.button0;
                order_input_buffered.button1 = tst_input.button1;
                order_input_buffered.dir = tst_input.dir;

                limitmove = (int32_t) totalclock;
            }
        }
    }
    else
    {
        select_held = FALSE;
        order_input_buffered.button0 = tst_input.button0;
        order_input_buffered.button1 = tst_input.button1;
        order_input_buffered.dir = tst_input.dir;
    }

    if (!KEY_PRESSED(KEYSC_ESC) && !order_input_buffered.button1)
    {
        cust_callback = MNU_OrderCustom;
        cust_callback_call = call;
        cust_callback_item = item;
    }
    else
    {
        KEY_PRESSED(KEYSC_ESC) = FALSE;
        cust_callback = NULL;
        DidOrderSound = FALSE;
        on_screen = 0;
        ExitMenus();
    }

    if (order_input.dir == dir_North)
    {
        on_screen--;
    }
    else if (order_input.dir == dir_South)
    {
        on_screen++;
    }
    else if (order_input.dir == dir_West)
    {
        on_screen--;
    }
    else if (order_input.dir == dir_East)
    {
        on_screen++;
    }

// CTW MODIFICATION
// I reversed the logic in here to allow the user to loop around.
// Yeah... I changed default behavior just because I wanted to.
// AND YOU CAN'T STOP ME SUCKER!!!
    if (on_screen < 0)
        on_screen = OrderScreenSiz-1;

    if (on_screen > OrderScreenSiz-1)
        on_screen = 0;
// CTW MODIFICATION END

    rotatesprite(0,0,RS_SCALE,0,OrderScreen[on_screen],0,0,
                 (ROTATE_SPRITE_CORNER|ROTATE_SPRITE_SCREEN_CLIP|ROTATE_SPRITE_NON_MASK|ROTATE_SPRITE_IGNORE_START_MOST),
                 0, 0, xdim-1, ydim-1);

    if (on_screen == OrderScreenSiz-1)
    {
        // Jonathon's credits page hack :-)

        static const char *jtitle = "^Port Credits";
        static const char *jtext[] =
        {
            "*GAME AND ENGINE PORT",
            " Jonathon \"JonoF\" Fowler",
            "-",
            "*\"POLYMOST\" 3D RENDERER",
            "*NETWORKING, OTHER CODE",
            " Ken \"Awesoken\" Silverman",
            "-",
            " Visit http://www.jonof.id.au/jfsw for the",
            " source code, latest news, and updates of this port."
        };
        static const char *scroller[] =
        {
            "This program is free software; you can redistribute it",
            "and/or modify it under the terms of the GNU General",
            "Public License as published by the Free Software",
            "Foundation; either version 2 of the License, or (at your",
            "option) any later version.",
            "",
            "This program is distributed in the hope that it will be",
            "useful but WITHOUT ANY WARRANTY; without even the implied",
            "warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR",
            "PURPOSE. See the GNU General Public License (GPL.TXT) for",
            "more details.",
            "",
            "",
            "",
            "",
            "Thanks to these people for their input and contributions:",
            "",
            "Richard \"TerminX\" Gobeille,",
            "Par \"Parkar\" Karlsson", // "Pär \"Parkar\" Karlsson",
            "Ben \"ProAsm\" Smit",
            "",
            "and all those who submitted bug reports and ",
            "supported the project financially!",
            "",
            "",
            "--x--",
            "",
            "",
            "",
            ""
        };
        const int numscrollerlines = SIZ(scroller);
        short dimx, dimy;
        int ycur = 54;
        unsigned ji;

        dimy = 0; MNU_MeasureString(jtitle, &dimx, &dimy);
        MNU_DrawString(160-(dimx>>1), ycur, jtitle, 0, 0);
        ycur += dimy + 8;

        for (ji = 0; ji < SIZ(jtext); ji++)
        {
            switch (jtext[ji][0])
            {
            case '-':
                ycur += 6;
                break;
            case '*':
                dimx = dimy = 0;
                MNU_MeasureString(&jtext[ji][1], &dimx, &dimy);
                MNU_DrawString(160-(dimx>>1), ycur, &jtext[ji][1], 0, 16);
                ycur += dimy+1;
                break;
            default:
                if (ji>0 && jtext[ji-1][0] == '*') ycur += 3;
                dimx = dimy = 0;
                MNU_MeasureSmallString(&jtext[ji][1], &dimx, &dimy);
                MNU_DrawSmallString(160-(dimx>>1), ycur, &jtext[ji][1], 0, 0);
                ycur += dimy+1;
                break;
            }
        }

        int m,i;
        for (m=0, i=((int32_t) totalclock/104)%numscrollerlines; m<4; m++,i++)
        {
            if (i == numscrollerlines)
                i=0;
            dimx = dimy = 0;
            MNU_MeasureSmallString(scroller[i], &dimx, &dimy);
            MNU_DrawSmallString(160-(dimx>>1), 154+(m*7), scroller[i], 0, 8);
        }
    }

    //KB_ClearKeysDown();

    return TRUE;
}

SWBOOL MNU_LoadModernDefaults(void)
{
    SetDefaultKeyDefinitions(1);
    SetMouseDefaults(1);
    return TRUE;
}

SWBOOL MNU_LoadClassicDefaults(void)
{
    SetDefaultKeyDefinitions(0);
    SetMouseDefaults(0);
    return TRUE;
}


short EpisodeMenuSelection;

void
ExitMenus(void)
{
    ControlPanelType = ct_mainmenu;
    UsingMenus = FALSE;

    if (LoadGameOutsideMoveLoop)
        return;

    ResumeGame();
    SetRedrawScreen(&Player[myconnectindex]);
}

SWBOOL
MNU_StartGame(void)
{
    PLAYERp pp = Player + screenpeek;
    int handle = 0;
    int zero = 0;

    // always assumed that a demo is playing

    ready2send = 0;
    Skill = currentmenu->cursor;

    if (EpisodeMenuSelection >= 1)
        Level = 5;
    else
        Level = 1;

    ExitMenus();
    DemoPlaying = FALSE;
    ExitLevel = TRUE;
    NewGame = TRUE;
    DemoMode = FALSE;
    CameraTestMode = FALSE;

    //InitNewGame();

    if (Skill == 0)
        handle = PlaySound(DIGI_TAUNTAI3,&zero,&zero,&zero,v3df_none);
    else if (Skill == 1)
        handle = PlaySound(DIGI_NOFEAR,&zero,&zero,&zero,v3df_none);
    else if (Skill == 2)
        handle = PlaySound(DIGI_WHOWANTSWANG,&zero,&zero,&zero,v3df_none);
    else if (Skill == 3)
        handle = PlaySound(DIGI_NOPAIN,&zero,&zero,&zero,v3df_none);

    if (handle > FX_Ok)
        while (FX_SoundActive(handle))
            handleevents();

    return TRUE;
}

void ResetMenuInput(void)
{
    cust_callback = NULL;
    InputMode = FALSE;
}

SWBOOL
MNU_StartNetGame(void)
{
    extern SWBOOL ExitLevel, ShortGameMode, DemoInitOnce, FirstTimeIntoGame;
    extern short Level, Skill;
    // CTW REMOVED
    //extern int gTenActivated;
    // CTW REMOVED END
    int pnum;

    // always assumed that a demo is playing

    ready2send = 0;
    // Skill can go negative here
    Skill = gs.NetMonsters-1;
    Level = gs.NetLevel + 1;
    if (!AutoNet)
        ExitMenus();
    DemoPlaying = FALSE;
    ExitLevel = TRUE;
    NewGame = TRUE;
    // restart demo for multi-play mode
    DemoInitOnce = FALSE;
    ResetMenuInput();

    // TENSW: return if a joiner
    if (/* CTW REMOVED gTenActivated && */ !AutoNet && FirstTimeIntoGame)
        return TRUE;

    // need to set gNet vars for self
    // everone else gets a packet to set them
    gNet.AutoAim            = gs.AutoAim;
    gNet.SpawnMarkers       = gs.NetSpawnMarkers;
    gNet.HurtTeammate       = gs.NetHurtTeammate;
    gNet.Nuke               = gs.NetNuke;
    gNet.KillLimit          = gs.NetKillLimit*10;
    gNet.TimeLimit          = TimeLimitTable[gs.NetTimeLimit]*60*120;

    if (ShortGameMode)
    {
        gNet.KillLimit /= 10;
        gNet.TimeLimit /= 2;
    }

    gNet.TimeLimitClock     = gNet.TimeLimit;
    gNet.TeamPlay           = gs.NetTeamPlay;
    gNet.MultiGameType      = gs.NetGameType+1;

    if (gNet.MultiGameType == MULTI_GAME_COMMBAT_NO_RESPAWN)
    {
        gNet.MultiGameType = MULTI_GAME_COMMBAT;
        gNet.NoRespawn = TRUE;
    }
    else
    {
        gNet.NoRespawn = FALSE;
    }

    if (CommEnabled)
    {
        PACKET_NEW_GAME p;

        p.PacketType = PACKET_TYPE_NEW_GAME;
        p.Level = Level;
        p.Skill = Skill;
        p.GameType = gs.NetGameType;
        p.AutoAim = gs.AutoAim;
        p.HurtTeammate = gs.NetHurtTeammate;
        p.TeamPlay = gs.NetTeamPlay;
        p.SpawnMarkers = gs.NetSpawnMarkers;
        p.KillLimit = gs.NetKillLimit;
        p.TimeLimit = gs.NetTimeLimit;
        p.Nuke = gs.NetNuke;

        netbroadcastpacket((uint8_t*)(&p), sizeof(p));            // TENSW
    }


    return TRUE;
}


SWBOOL
MNU_EpisodeCustom(void)
{
    EpisodeMenuSelection = currentmenu->cursor;

    return TRUE;
}

SWBOOL
MNU_QuitCustom(UserCall call, MenuItem_p item)
{
    int select;
    int ret;
    extern SWBOOL DrawScreen;

    // Ignore the special touchup calls
    if (call == uc_touchup)
        return TRUE;

    if (cust_callback == NULL)
    {
        if (call != uc_setup)
            return FALSE;

        memset(dialog, 0, sizeof(dialog));

        dialog[0] = S_QUITYN;
    }

    ret = MNU_Dialog();

    if (DrawScreen)
        return TRUE;

    if (!ret)
    {
        if (!mnu_input.button1 && !KB_KeyPressed(sc_N))
        {
            cust_callback = MNU_QuitCustom;
            cust_callback_call = call;
            cust_callback_item = item;
        }
        else
        {
            cust_callback = NULL;
            ExitMenus();
        }
    }
    else
    {
        cust_callback = NULL;
        ExitMenus();
    }

    if (KB_KeyPressed(sc_Y) || KB_KeyPressed(sc_Enter) || mnu_input.button0)
    {
        if (CommPlayers >= 2)
            MultiPlayQuitFlag = TRUE;
        else
            QuitFlag = TRUE;

        ExitMenus();
    }

    KB_ClearKeysDown();

    return TRUE;
}

SWBOOL
MNU_QuickLoadCustom(UserCall call, MenuItem_p item)
{
    int select;
    extern SWBOOL ReloadPrompt;
    int bak;
    PLAYERp pp = Player + myconnectindex;
    extern short GlobInfoStringTime;
    extern SWBOOL DrawScreen;
    int ret;

    if (cust_callback == NULL)
    {
        if (call != uc_setup)
            return FALSE;

        memset(dialog, 0, sizeof(dialog));

        dialog[0] = "Load saved game";
        sprintf(QuickLoadDescrDialog,"\"%s\" (Y/N)?",SaveGameDescr[QuickLoadNum]);
        dialog[1] = QuickLoadDescrDialog;
    }

    // Ignore the special touchup calls
    if (call == uc_touchup)
        return TRUE;

    ret = MNU_Dialog();

    if (DrawScreen)
    {
        return TRUE;
    }

    if (ret == FALSE)
    {
        if (KB_KeyPressed(sc_N) || KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter))
        {
            cust_callback = NULL;
            if (ReloadPrompt)
            {
                ReloadPrompt = FALSE;
                bak = GlobInfoStringTime;
                GlobInfoStringTime = 999;
                PutStringInfo(pp, "Press SPACE to restart");
                GlobInfoStringTime = bak;
            }

            KB_ClearKeysDown();
            ExitMenus();
        }
        else
        {
            cust_callback = MNU_QuickLoadCustom;
            cust_callback_call = call;
            cust_callback_item = item;
        }
    }
    else
    {
        // Y pressed
        cust_callback = NULL;

        KB_ClearKeysDown();
        LoadSaveMsg("Loading...");

        PauseAction();

        ReloadPrompt = FALSE;
        if (LoadGame(QuickLoadNum) == -1)
        {
            ResumeAction();
            return FALSE;
        }

        ready2send = 1;
        LastSaveNum = -1;

        // do a load game here
        KB_ClearKeysDown();
        ExitMenus();
    }

    KB_ClearKeysDown();

    return TRUE;
}

// MENU FUNCTIONS /////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////
// Set some global menu related defaults
////////////////////////////////////////////////
void
MNU_InitMenus(void)
{
    pClearTextLine(Player + myconnectindex, TEXT_INFO_LINE(0));

    slidersettings[sldr_mouse] = gs.MouseSpeed/(MOUSE_SENS_MAX_VALUE/SLDR_MOUSESENSEMAX);

    slidersettings[sldr_sndfxvolume] = gs.SoundVolume / (FX_VOL_MAX_VALUE/SLDR_SNDFXVOLMAX);
    slidersettings[sldr_musicvolume] = gs.MusicVolume / (MUSIC_VOL_MAX_VALUE/SLDR_MUSICVOLMAX);
    slidersettings[sldr_scrsize] = gs.BorderNum;
    slidersettings[sldr_brightness] = gs.Brightness;
    slidersettings[sldr_bordertile] = gs.BorderTile;

    {
        int i,newx=xdim,newy=ydim;

        buttonsettings[btn_videofs] = fullscreen;

        UpdateValidModes(bpp,fullscreen);
        for (i=0; i<numvalidbpps; i++)
            if (validbpps[i] == bpp)
                slidersettings[sldr_videobpp] = i;

        i = videoCheckMode(&newx, &newy, bpp, fullscreen, 1);
        if (i != 0x7fffffff && i >= 0)
            for (i=0; i<numvalidresolutions; i++)
                if (validresolutions[i].xdim == newx && validresolutions[i].ydim == newy)
                    slidersettings[sldr_videores] = i;
    }

    buttonsettings[btn_auto_run] = gs.AutoRun;
    buttonsettings[btn_auto_aim] = gs.AutoAim;
    buttonsettings[btn_messages] = gs.Messages;
    buttonsettings[btn_crosshair] = gs.Crosshair;
    //    buttonsettings[btn_bobbing] = gs.Bobbing;
    buttonsettings[btn_shadows] = gs.Shadows;

    buttonsettings[btn_mouse_aim] = gs.MouseAimingType;
    buttonsettings[btn_mouse_invert] = gs.MouseInvert;
    buttonsettings[btn_sound] = gs.FxOn;
    buttonsettings[btn_music] = gs.MusicOn;
    buttonsettings[btn_talking] = gs.Talking;

    buttonsettings[btn_voxels] = gs.Voxels;
    buttonsettings[btn_ambience] = gs.Ambient;
    buttonsettings[btn_playcd] = gs.PlayCD;
    buttonsettings[btn_flipstereo] = gs.FlipStereo;
    buttonsettings[btn_stats] = gs.Stats;

    slidersettings[sldr_gametype] = gs.NetGameType;
    slidersettings[sldr_netlevel] = gs.NetLevel;
    slidersettings[sldr_monsters] = gs.NetMonsters;
    slidersettings[sldr_killlimit] = gs.NetKillLimit;
    slidersettings[sldr_timelimit] = gs.NetTimeLimit;
    slidersettings[sldr_playercolor] = gs.NetColor;

    buttonsettings[btn_nuke] = gs.NetNuke;
    buttonsettings[btn_markers] = gs.NetSpawnMarkers;
    buttonsettings[btn_teamplay] = gs.NetTeamPlay;
    buttonsettings[btn_friendlyfire] = gs.NetHurtTeammate;
    buttonsettings[btn_parental] = gs.ParentalLock;

    slidersettings[sldr_mousescalex] = MouseAnalogScale[0]>>13;
    slidersettings[sldr_mousescaley] = MouseAnalogScale[1]>>13;

    slidersettings[sldr_joyaxisscale] = 0;
    slidersettings[sldr_joyaxisanalog] = 0;
    slidersettings[sldr_joyaxisdead] = 0;
    slidersettings[sldr_joyaxissatur] = 0;

    // Distinguish between Single or Multiplay for new game menu types
    if (numplayers > 1)
        main_i[0].child = &networkgroup;
    else
//        #ifdef SW_SHAREWARE
        main_i[0].child = &episodegroup;
//        #else
//        main_i[0].child = &skillgroup;
//        #endif
    main_i[4].text = (SW_SHAREWARE) ? MAIN_MENU_HOW_TO_ORDER : MAIN_MENU_COOL_STUFF;
    main_i[4].hotkey = (SW_SHAREWARE) ? KEYSC_H : KEYSC_C;
}

////////////////////////////////////////////////
// Measure the pixel width of a graphic string
////////////////////////////////////////////////
static char lg_xlat_num[] = {0,1,2,3,4,5,6,7,8,9};
#define FONT_LARGE_ALPHA 3706
#define FONT_LARGE_DIGIT 3732

void
MNU_MeasureStringLarge(const char *string, short *w, short *h)
{
    short ndx, width, height;
    char c;
    short pic;

    width = 0;
    height = *h;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        if (isalpha(c))
        {
            c = toupper(c);
            pic = FONT_LARGE_ALPHA + (c - 'A');
        }
        else if (isdigit(c))
        {
            pic = FONT_LARGE_DIGIT + lg_xlat_num[(c - '0')];
        }
        else if (c == ' ')
        {
            width += 10;                 // Special case for space char
            continue;
        }
        else
        {
            continue;
        }

        width += tilesiz[pic].x+1;
        if (height < tilesiz[pic].y)
            height = tilesiz[pic].y;
    }

    *w = width;
    *h = height;
}

////////////////////////////////////////////////
// Draw a string using a graphic font
////////////////////////////////////////////////
void
MNU_DrawStringLarge(short x, short y, const char *string)
{
    int ndx, offset;
    char c;
    short pic;

    offset = x;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        if (isalpha(c))
        {
            c = toupper(c);
            pic = FONT_LARGE_ALPHA + (c - 'A');
        }
        else if (isdigit(c))
        {
            pic = FONT_LARGE_DIGIT + lg_xlat_num[(c - '0')];
        }
        else if (c == ' ')
        {
            offset += 10;
            continue;
        }
        else
        {
            continue;
        }

        rotatesprite(offset << 16, y << 16, MZ, 0, pic, MenuTextShade, 0, MenuDrawFlags|ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);
        offset += tilesiz[pic].x + 1;
    }

}


////////////////////////////////////////////////
// Measure the pixel width of a graphic string
////////////////////////////////////////////////
void
MNU_MeasureString(const char *string, short *w, short *h)
{
    short ndx, width, height;
    char c;
    short ac;

    if (string[0] == '^')
    {
        MNU_MeasureStringLarge(&string[1], w, h);
        return;
    }

    width = 0;
    height = *h;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        ac = c - '!' + STARTALPHANUM;
        if ((ac < STARTALPHANUM || ac > ENDALPHANUM)  && c != asc_Space)
            break;

        if (c > asc_Space && c < 127)
        {
            width += tilesiz[ac].x;
            if (height < tilesiz[ac].y)
                height = tilesiz[ac].y;
        }
        else if (c == asc_Space)
            width += 4;                 // Special case for space char
    }

    *w = width;
    *h = height;
}

////////////////////////////////////////////////
// Draw a string using a graphic font
//
// MenuTextShade and MenuDrawFlags
////////////////////////////////////////////////
void
MNU_DrawString(short x, short y, const char *string, short shade, short pal)
{
    int ndx, offset;
    char c;
    short ac;

    if (string[0] == '^')
    {
        MNU_DrawStringLarge(x,y, &string[1]);
        return;
    }

    offset = x;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        ac = c - '!' + STARTALPHANUM;
        if ((ac < STARTALPHANUM || ac > ENDALPHANUM)  && c != asc_Space)
            break;

        if (c > asc_Space && c < 127)
        {
            rotatesprite(offset<<16,y<<16,MZ,0,ac, shade, pal, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);
            offset += tilesiz[ac].x;
        }
        else if (c == asc_Space)
            offset += 4;                // Special case for space char
    }

}
/*   Original code
void
MNU_DrawString(short x, short y, char *string)
{
    int ndx, offset;
    char c;

    if (string[0] == '^')
    {
        MNU_DrawStringLarge(x,y, &string[1]);
        return;
    }

    offset = x;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        if (c > asc_Space && c < 127)
        {
            rotatesprite(offset << 16, y << 16, MZ, 0, xlatfont[c], MenuTextShade, 0, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);
            offset += tilesiz[xlatfont[c]].x;
        } else
        if (c == asc_Space)
            offset += 4;                // Special case for space char
    }

}
*/

////////////////////////////////////////////////
// Measure the pixel width of a small font string
////////////////////////////////////////////////
void
MNU_MeasureSmallString(const char *string, short *w, short *h)
{
    short ndx, width, height;
    char c;
    short ac;

    width = 0;
    height = *h;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        ac = (c - '!') + 2930;
        if ((ac < 2930 || ac > 3023) && c != asc_Space)
            break;

        if (c > asc_Space && c < 127)
        {
            width += tilesiz[ac].x;
            if (height < tilesiz[ac].y)
                height = tilesiz[ac].y;
        }
        else if (c == asc_Space)
            width += 4;                 // Special case for space char
    }

    *w = width;
    *h = height;
}

////////////////////////////////////////////////
// Draw a string using a small graphic font
////////////////////////////////////////////////
void
MNU_DrawSmallString(short x, short y, const char *string, short shade, short pal)
{
    int ndx;
    char c;
    short ac,offset;


    offset = x;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        ac = c - '!' + 2930;
        if ((ac < 2930 || ac > 3023)  && c != asc_Space)
            break;

        if (c > asc_Space && c < 127)
        {
            rotatesprite(offset<<16,y<<16,MZ,0,ac, shade, pal, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);

            offset += tilesiz[ac].x;

        }
        else if (c == asc_Space)
        {
            offset += 4;                // Special case for space char
        }
    }

}

////////////////////////////////////////////////
//  Get an input string from user using small font
////////////////////////////////////////////////

signed char
MNU_InputSmallString(char *name, short pix_width)
{
    char ch;
    short w, h;
    UserInput con_input;

#define ascii_backspace 8
#define ascii_esc 27
#define ascii_return 13

    if (!MoveSkip4 && !MessageInputMode)
    {
        con_input.dir = dir_None;
        CONTROL_GetUserInput(&con_input);

        if (con_input.dir == dir_North)
        {
            CON_CommandHistory(1);
        }
        else if (con_input.dir == dir_South)
        {
            CON_CommandHistory(-1);
        }
    }

    while (KB_KeyWaiting())
    {
        ch = KB_GetCh();

        // skip any extended key
        if (ch == 0)
        {
            ch = KB_GetCh();
            if (ch == 104) // extended enter
                ch = ascii_return;
            else
                continue;
        }

        if (ch == ascii_backspace)
        {
            name[strlen(name) - 1] = '\0';
            continue;
        }
        else if (ch == ascii_esc)
        {
            return -1;
        }
        else if (ch == ascii_return)
        {
            return FALSE;
        }
        else if (!isprint(ch))
            continue;

        MNU_MeasureSmallString(name, &w, &h);
        if (w < pix_width)
        {
            size_t const namelen = strlen(name);
            if (namelen < 256) // Dont let it go too far!
            {
                name[namelen] = ch;
                name[namelen+1] = '\0';
            }
        }
    }

    return TRUE;

}

////////////////////////////////////////////////
// Draw dialog text on screen
////////////////////////////////////////////////
static SWBOOL
MNU_Dialog(void)
{
    short ndx, linecnt, w[MAXDIALOG], h, x, y;

    linecnt = 0;
    h = 8;

    for (ndx = 0; ndx < MAXDIALOG && dialog[ndx]; ndx++)
    {
        MNU_MeasureString(dialog[ndx], &w[ndx], &h);
        ASSERT(w[ndx] < XDIM);
        linecnt++;
    }

    y = ((YDIM - ((h * linecnt) + (linecnt * 2))) / 2);

    for (ndx = 0; ndx < linecnt; ndx++)
    {
        x = ((XDIM - w[ndx]) / 2);
        MNU_DrawString(x, y, dialog[ndx],1,16);
        y += (h + 3);
    }

    mnu_input.button0 = mnu_input.button1 = FALSE;
    CONTROL_ClearUserInput(&mnu_input);
    CONTROL_GetUserInput(&mnu_input);

    if (KB_KeyPressed(sc_Y) || KB_KeyPressed(sc_Enter) || mnu_input.button0)
        return TRUE;
    else
        return FALSE;
}

////////////////////////////////////////////////
//  Get an input string from user
////////////////////////////////////////////////

signed char
MNU_InputString(char *name, short pix_width)
{
    char ch;
    short w, h;

#define ascii_backspace 8
#define ascii_esc 27
#define ascii_return 13

    while (KB_KeyWaiting())
    {
        ch = KB_GetCh();

        ////DSPRINTF(ds, "%c %d", ch, ch);
        //MONO_PRINT(ds);

        // skip most extended keys
        if (ch == 0)
        {
            ch = KB_GetCh();

            ////DSPRINTF(ds, "extended key %c %d", ch, ch);
            //MONO_PRINT(ds);

            if (ch == 104) // extended enter
                ch = ascii_return;
            else
                continue;
        }

        if (ch == ascii_backspace)
        {
            name[strlen(name) - 1] = '\0';
            continue;
        }
        else if (ch == ascii_esc)
        {
            return -1;
        }
        else if (ch == ascii_return)
        {
            return FALSE;
        }
        else if (!isprint(ch))
            continue;

        MNU_MeasureString(name, &w, &h);
        if (w < pix_width)
        {
            size_t const namelen = strlen(name);
            name[namelen] = ch;
            name[namelen+1] = '\0';
        }
    }

    return TRUE;

}

#define SS_XSTART 146L
#define SS_YSTART SD_YSTART
#define SS_BORDER_SIZE 5L

void LoadSaveMsg(const char *msg)
{
    short w,h;

    renderFlushPerms();
    DrawMenuLevelScreen();
    strcpy(ds, msg);
    MNU_MeasureString(ds, &w, &h);
    MNU_DrawString(TEXT_XCENTER(w), 170, ds, 1, 16);
    videoNextPage();
}


////////////////////////////////////////////////
//  Load Game menu
//  This function gets called whenever you
//  press enter on one of the load game
//  spots.
//  I'm figuring it need to do the following:
//  . Load the game if there is one by calling: MNU_LoadGameCustom.
////////////////////////////////////////////////
SWBOOL
MNU_GetLoadCustom(void)
{
    short load_num;

    load_num = currentmenu->cursor;

    // no saved game exists - don't do anything
    if (SaveGameDescr[load_num][0] == '\0')
        return FALSE;

    if (InMenuLevel || DemoMode || DemoPlaying)
    {
        LoadSaveMsg("Loading...");

        if (LoadGame(load_num) == -1)
            return FALSE;

        QuickLoadNum = load_num;
        // the (Quick)Save menu should default to the last loaded game
        SaveGameGroup.cursor = load_num;

        ExitMenus();
        ExitLevel = TRUE;
        LoadGameOutsideMoveLoop = TRUE;
        if (DemoMode || DemoPlaying)
            LoadGameFromDemo = TRUE;

        return TRUE;
    }

    LoadSaveMsg("Loading...");

    PauseAction();

    if (LoadGame(load_num) == -1)
    {
        ResumeAction();
        return FALSE;
    }

    QuickLoadNum = load_num;
    // the (Quick)Save menu should default to the last loaded game
    SaveGameGroup.cursor = load_num;

    ready2send = 1;
    LastSaveNum = -1;
    ExitMenus();

    if (DemoMode)
    {
        ExitLevel = TRUE;
        DemoPlaying = FALSE;
    }

    return TRUE;
}

////////////////////////////////////////////////
//  Save Game menu
//  This function gets called whenever you
//  press enter on one of the save game
//  spots.
//  I'm figuring it need to do the following:
//  . Call MNU_GetInput to allow string input of description.
//  . Save the game if there is one by calling: MNU_SaveGameCustom.
////////////////////////////////////////////////
SWBOOL
MNU_GetSaveCustom(void)
{
    short save_num;
    extern SWBOOL InMenuLevel, LoadGameOutsideMoveLoop;

    save_num = currentmenu->cursor;

    if (InMenuLevel)
        return FALSE;

    if (MenuInputMode)
    {
        PauseAction();

        LoadSaveMsg("Saving...");

        if (SaveGame(save_num) != -1)
        {
            QuickLoadNum = save_num;

            LoadGameGroup.cursor = save_num;
            LastSaveNum = -1;
        }

        ResumeAction();
        ExitMenus();

        // toggle edit mode
        MenuInputMode = FALSE;
    }
    else
    {
        strcpy(BackupSaveGameDescr, SaveGameDescr[save_num]);

        // clear keyboard buffer
        while (KB_KeyWaiting())
        {
            if (KB_GetCh() == 0)
                KB_GetCh();
        }

        // toggle edit mode
        MenuInputMode = TRUE;
    }

    return TRUE;
}

////////////////////////////////////////////////
//  Load/Save Touchup function
//  This function gets called each frame by DrawMenus
////////////////////////////////////////////////

static SWBOOL
MNU_DrawLoadSave(short game_num)
{
    // screen border
    rotatesprite(SS_XSTART << 16, SS_YSTART << 16, MZ, 0, pic_loadsavescreen,
                 0, 0, MenuDrawFlags | ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);

    // description box
    rotatesprite((SD_XSTART) << 16, (SD_YSTART) << 16, MZ, 0, pic_savedescr,
                 0, 0, MenuDrawFlags | ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);

    // cursor for text boxes
    rotatesprite((SD_XSTART + 3) << 16, (SD_LINE(game_num) + 1) << 16, MZ, 0, pic_loadsavecursor,
                 0, 0, MenuDrawFlags | ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);

    return TRUE;
}

static char SaveGameInfo1[80];
static char SaveGameInfo2[80];

SWBOOL
MNU_LoadSaveMove(UserCall call, MenuItem_p item)
{
    short i;
    short game_num;
    short tile;
    static short SaveGameEpisode, SaveGameLevel, SaveGameSkill;
    SWBOOL GotInput = FALSE;

    if (!UsingMenus)
        return TRUE;

    game_num = currentmenu->cursor;

    // read all descr first time through - LastSaveNum starts at 99
    if (LastSaveNum == 99)
    {
        memset(SaveGameDescr, '\0', sizeof(SaveGameDescr));

        for (i = 0; i < 10; i++)
            LoadGameDescr(i, SaveGameDescr[i]);
    }

    // cursor has moved - read header
    if (game_num != LastSaveNum)
    {
        screen_tile = LoadGameFullHeader(game_num, SaveGameDescr[game_num],
                                         &SaveGameLevel, &SaveGameSkill);

        sprintf(SaveGameInfo1, "Level %d, Skill %d", SaveGameLevel, SaveGameSkill+1);
        SaveGameInfo2[0] = 0;
    }

    if (QuickSaveMode)
    {
        QuickSaveMode = FALSE;
        MenuInputMode = TRUE;
        strcpy(BackupSaveGameDescr, SaveGameDescr[game_num]);
        KB_ClearKeysDown();
        KB_FlushKeyboardQueue();
    }

    LastSaveNum = game_num;

    // input mode check
    if (MenuInputMode)
    {
        MenuItem *item = &currentmenu->items[currentmenu->cursor];

        if (SavePrompt)
        {
            if (KB_KeyPressed(sc_Y) || KB_KeyPressed(sc_Enter))
            {
                KB_ClearKeyDown(sc_Y);
                KB_ClearKeyDown(sc_Enter);
                SavePrompt = FALSE;
                // use input
                item->custom();
            }
            else if (KB_KeyPressed(sc_N))
            {
                KB_ClearKeyDown(sc_N);
                strcpy(SaveGameDescr[game_num], BackupSaveGameDescr);
                SavePrompt = FALSE;
                MenuInputMode = FALSE;
            }
        }
        else
            // get input
            switch (MNU_InputString(SaveGameDescr[game_num], 114))
            {
            case -1:                        // Cancel Input (pressed ESC) or Err
                strcpy(SaveGameDescr[game_num], BackupSaveGameDescr);
                MenuInputMode = FALSE;
                KB_ClearKeysDown();
                break;
            case FALSE:                     // Input finished (RETURN)
                // no input
                if (SaveGameDescr[game_num][0] == '\0')
                {
                    strcpy(SaveGameDescr[game_num], BackupSaveGameDescr);
                    MenuInputMode = FALSE;
                }
                else
                {
                    GotInput = TRUE;
                }
                KB_ClearKeyDown(sc_Enter);
                break;
            case TRUE:                      // Got input
                break;
            }

        if (GotInput)
        {
            if (BackupSaveGameDescr[0])
                SavePrompt = TRUE;

            if (!SavePrompt)
            {
                // use input
                item->custom();
            }
        }
    }

    return TRUE;
}

SWBOOL
MNU_LoadSaveDraw(UserCall call, MenuItem_p item)
{
    short i;
    short game_num;
    short tile;

    game_num = currentmenu->cursor;

    // misc drawing
    MNU_DrawLoadSave(game_num);

    // print game descriptions
    for (i = 0; i < 10; i++)
    {
        if (i == game_num && MenuInputMode && !SavePrompt)
        {
            static SWBOOL cur_show;
            char tmp[sizeof(SaveGameDescr[0])*2];

            //cur_show ^= 1;
            cur_show = ((int32_t) totalclock & 32);
            if (cur_show)
            {
                // add a cursor to the end
                sprintf(tmp, "%s_", SaveGameDescr[i]);
            }
            else
                strcpy(tmp, SaveGameDescr[i]);

            MNU_DrawString(SD_XSTART + 4, SD_YSTART + (i * SD_YOFF) + 2, tmp, 1, 16);
        }
        else if (SaveGameDescr[i][0] != '\0')
        {
            MNU_DrawString(SD_XSTART + 4, SD_YSTART + (i * SD_YOFF) + 2, SaveGameDescr[i], 1, 16);
        }
    }

    if (screen_tile != -1)
    {
        // draw 160x100 save screen
        rotatesprite((SS_XSTART + SS_BORDER_SIZE) << 16, (SS_YSTART + SS_BORDER_SIZE) << 16, (1 << 16), 0 + 512, screen_tile,
                     0, 0, MenuDrawFlags | ROTATE_SPRITE_CORNER | ROTATE_SPRITE_NON_MASK | ROTATE_SPRITE_YFLIP, 0, 0, xdim - 1, ydim - 1);

        // draw info string
        MNU_DrawString(SS_XSTART + 13, SS_YSTART + 100 + 10, SaveGameInfo1, 1, 16);
        MNU_DrawString(SS_XSTART + 13, SS_YSTART + 100 + 18, SaveGameInfo2, 1, 16);

        if (SavePrompt)
        {
            MNU_DrawString(SS_XSTART + SS_BORDER_SIZE + 5, SS_YSTART + SS_BORDER_SIZE + 47, "Overwrite previous", 1, 16);
            MNU_DrawString(SS_XSTART + SS_BORDER_SIZE + 5, SS_YSTART + SS_BORDER_SIZE + 47 + 12, "  saved game (Y/N)", 1, 16);
        }
    }
    else
    {
        // draw 160x100 black pic
        rotatesprite((SS_XSTART + SS_BORDER_SIZE) << 16, (SS_YSTART + SS_BORDER_SIZE) << 16, (1 << 16), 0, pic_loadsavescreenbak,
                     0, 0, MenuDrawFlags | ROTATE_SPRITE_CORNER | ROTATE_SPRITE_NON_MASK, 0, 0, xdim - 1, ydim - 1);

        MNU_DrawString(SS_XSTART + SS_BORDER_SIZE + 60, SS_YSTART + SS_BORDER_SIZE + 47, "Empty", 1, 16);
    }


    return TRUE;
}

SWBOOL
MNU_ShareWareCheck(MenuItem *item)
{
    if (SW_SHAREWARE)
    {
        SET(item->flags, mf_disabled);
    }

    return TRUE;
}

SWBOOL
MNU_CheckUserMap(MenuItem *item)
{
    if (UserMapName[0] == '\0')
        RESET(item->flags, mf_disabled);
    else
        SET(item->flags, mf_disabled);
    return TRUE;
}

SWBOOL
MNU_ShareWareMessage(MenuItem *item)
{
    const char *extra_text;
    short w,h;

    if (SW_SHAREWARE)
    {
        extra_text = "Be sure to call 800-3DREALMS today";
        MNU_MeasureString(extra_text, &w, &h);
        MNU_DrawString(TEXT_XCENTER(w), 110, extra_text, 1, 16);
        extra_text = "and order the game.";
        MNU_MeasureString(extra_text, &w, &h);
        MNU_DrawString(TEXT_XCENTER(w), 120, extra_text, 1, 16);
        extra_text = "You are only playing the first ";
        MNU_MeasureString(extra_text, &w, &h);
        MNU_DrawString(TEXT_XCENTER(w), 130, extra_text, 1, 16);
        extra_text = "four levels, and are missing most";
        MNU_MeasureString(extra_text, &w, &h);
        MNU_DrawString(TEXT_XCENTER(w), 140, extra_text, 1, 16);
        extra_text = "of the game, weapons and monsters.";
        MNU_MeasureString(extra_text, &w, &h);
        MNU_DrawString(TEXT_XCENTER(w), 150, extra_text, 1, 16);
        extra_text = "See the ordering information.";
        MNU_MeasureString(extra_text, &w, &h);
        MNU_DrawString(TEXT_XCENTER(w), 160, extra_text, 1, 16);
        SET(item->flags, mf_disabled);
    }
    return TRUE;
}

SWBOOL
MNU_SaveGameCheck(MenuItem *item)
{
    extern SWBOOL InMenuLevel;
    extern SWBOOL DemoMode;

    if (0)      // JBF: Until we fix the symbol table dilemma, saving is off limits
    {
        SET(item->flags, mf_disabled);
        return TRUE;
    }

    if (CommEnabled || numplayers > 1 || DemoMode)
    {
        SET(item->flags, mf_disabled);
        return TRUE;
    }

    if (InMenuLevel)
        SET(item->flags, mf_disabled);
    else
    {
        if (TEST(Player[myconnectindex].Flags, PF_DEAD))
            SET(item->flags, mf_disabled);
        else
            RESET(item->flags, mf_disabled);
    }


    return TRUE;
}

SWBOOL
MNU_TenCheck(MenuItem *item)
{
    if (CommEnabled || numplayers > 1)
    {
        SET(item->flags, mf_disabled);
        return TRUE;
    }

    return TRUE;
}

SWBOOL
MNU_LoadGameCheck(MenuItem *item)
{

    if (0)      // JBF: Until we fix the symbol table dilemma, loading is off limits
    {
        SET(item->flags, mf_disabled);
        return TRUE;
    }

    if (CommEnabled || numplayers > 1)
    {
        SET(item->flags, mf_disabled);
        return TRUE;
    }

    return TRUE;
}

SWBOOL
MNU_StatCheck(MenuItem *item)
{
    if (CommEnabled || numplayers > 1)
    {
        SET(item->flags, mf_disabled);
        return TRUE;
    }

    return TRUE;
}

SWBOOL
MNU_HurtTeammateCheck(MenuItem *item)
{
    switch (gs.NetGameType+1)
    {
    // deathmatch and deathmatch no respawn
    case MULTI_GAME_COMMBAT:
    case MULTI_GAME_COMMBAT_NO_RESPAWN:
        if (gs.NetTeamPlay)
            RESET(item->flags, mf_disabled);
        else
            SET(item->flags, mf_disabled);
        break;
    // co-op
    case MULTI_GAME_COOPERATIVE:
        RESET(item->flags, mf_disabled);
        break;
    }

    return TRUE;
}

SWBOOL
MNU_TeamPlayCheck(MenuItem *item)
{
    switch (gs.NetGameType+1)
    {
    // co-op
    case MULTI_GAME_COOPERATIVE:
        SET(item->flags, mf_disabled);
        break;
    default:
        RESET(item->flags, mf_disabled);
        break;
    }

    return TRUE;
}

SWBOOL
MNU_CoopPlayCheck(MenuItem *item)
{
    switch (gs.NetGameType+1)
    {
    // co-op
    case MULTI_GAME_COOPERATIVE:
        SET(item->flags, mf_disabled);
        break;
    default:
        RESET(item->flags, mf_disabled);
        break;
    }

    return TRUE;
}

SWBOOL
MNU_TeamPlayChange(void)
{
    // if team play changes then do a pre process again
    MNU_ItemPreProcess(currentmenu);
    return TRUE;
}

SWBOOL
MNU_MouseCheck(MenuItem *item)
{
    if (!CONTROL_MousePresent)
    {
        SET(item->flags, mf_disabled);
    }
    else
    {
        RESET(item->flags, mf_disabled);
    }

    return TRUE;
}

SWBOOL
MNU_JoystickCheck(MenuItem *item)
{
    if (!CONTROL_JoyPresent)
    {
        SET(item->flags, mf_disabled);
    }
    else
    {
        RESET(item->flags, mf_disabled);
    }

    return TRUE;
}

// This is only called when Enter is pressed
static SWBOOL
MNU_TryMusicInit(void)
{
    if (PlaySong(0, RedBookSong[Level], TRUE, FALSE))
    {
        if (currentmenu->cursor == 0)
            MNU_MusicCheck(&currentmenu->items[currentmenu->cursor+1]);
    }

    return TRUE;
}

SWBOOL
MNU_MusicCheck(MenuItem *item)
{
    if (SW_SHAREWARE)
    {
        if (MusicDevice < 0 || !MusicInitialized)
        {
            SET(item->flags, mf_disabled);
        }
        else
        {
            RESET(item->flags, mf_disabled);
        }
    }
    else
    {
        // Redbook audio stuff
        //JBF
        //if (!cdvalid)
        //    {
        //    SET(item->flags, mf_disabled); // Just don't let CD Redbook ever be invalid!
        //    }
        //else
        {
            RESET(item->flags, mf_disabled);
        }
    }

    return TRUE;
}

SWBOOL
MNU_FxCheck(MenuItem *item)
{
    if (FXDevice < 0 || !FxInitialized)
    {
        SET(item->flags, mf_disabled);
    }
    else
    {
        RESET(item->flags, mf_disabled);
    }

    return TRUE;
}

SWBOOL
MNU_MusicFxCheck(MenuItem *item)
{
    if (FXDevice < 0 && MusicDevice < 0)
    {
        SET(item->flags, mf_disabled);
    }
    else
    {
        RESET(item->flags, mf_disabled);
    }

    return TRUE;
}

////////////////////////////////////////////////
// Do a toggle button
////////////////////////////////////////////////
void
MNU_DoButton(MenuItem_p item, SWBOOL draw)
{
    int x, y;
    SWBOOL state;
    int last_value;
    short shade = MENU_SHADE_DEFAULT;
    extern char LevelSong[];
    const char *extra_text = NULL;
    PLAYERp pp = &Player[myconnectindex];
    int button_x,zero=0;
    int handle=0;
    extern SWBOOL MusicInitialized,FxInitialized;

    button_x = OPT_XSIDE;

    x = item->x;
    y = item->y;

    if (TEST(item->flags, mf_disabled))
    {
        shade = MENU_SHADE_INACTIVE;
    }

    if (!draw)
    {
        switch (item->button)
        {
        case btn_nuke:
            gs.NetNuke = state = buttonsettings[item->button];
            break;
        case btn_voxels:
            gs.Voxels = state = buttonsettings[item->button];
            break;
        case btn_stats:
            gs.Stats = state = buttonsettings[item->button];
            break;
        case btn_markers:
            gs.NetSpawnMarkers = state = buttonsettings[item->button];
            break;
        case btn_teamplay:
            gs.NetTeamPlay = state = buttonsettings[item->button];
            break;
        case btn_friendlyfire:
            gs.NetHurtTeammate = state = buttonsettings[item->button];
            break;
        case btn_crosshair:
            gs.Crosshair = state = buttonsettings[item->button];
            break;
        case btn_auto_aim:
            last_value = gs.AutoAim;
            gs.AutoAim = state = buttonsettings[item->button];
            if (gs.AutoAim != last_value)
                MenuButtonAutoAim = TRUE;
            break;
        case btn_messages:
            gs.Messages = state = buttonsettings[item->button];
            break;
        case btn_auto_run:
            last_value = gs.AutoRun;
            gs.AutoRun = state = buttonsettings[item->button];
            if (gs.AutoRun != last_value)
                MenuButtonAutoRun = TRUE;
            break;
        case btn_mouse_aim:
            last_value = gs.MouseAimingType;
            gs.MouseAimingType = state = buttonsettings[item->button];
            if (gs.MouseAimingType != last_value)
            {
                //RESET(pp->Flags, PF_MOUSE_AIMING_ON);
                //gs.MouseAimingOn = FALSE;
            }
            //extra_text = gs.MouseAimingType ? "Momentary" : "Toggle";
            break;
        case btn_mouse_invert:
            gs.MouseInvert = state = buttonsettings[item->button];
            break;
//        case btn_bobbing:
//            gs.Bobbing = state = buttonsettings[item->button];
//            break;
        case btn_sound:

            if (!FxInitialized)
                break;

            last_value = gs.FxOn;
            gs.FxOn = state = buttonsettings[item->button];
            if (gs.FxOn != last_value)
            {
                if (!gs.FxOn)
                    StopFX();
            }
            break;
        case btn_music:
            last_value = gs.MusicOn;
            gs.MusicOn = state = buttonsettings[item->button];
            if (gs.MusicOn != last_value)
            {
                SWBOOL bak;

                if (gs.MusicOn)
                {
                    bak = DemoMode;
                    PlaySong(LevelSong, RedBookSong[Level], TRUE, TRUE);
                    DemoMode = bak;
                }
                else
                {
                    bak = DemoMode;
                    StopSong();
                    DemoMode = bak;

                    if (SW_SHAREWARE)
                    {
                        handle = PlaySound(DIGI_NOLIKEMUSIC,&zero,&zero,&zero,v3df_none);

                        if (handle > FX_Ok)
                            while (FX_SoundActive(handle))
                                handleevents();
                    }
                }
            }
            break;
        case btn_talking:
            gs.Talking = state = buttonsettings[item->button];
            break;
        case btn_playcd:
            last_value = gs.PlayCD;
            gs.PlayCD = state = buttonsettings[item->button];
            break;
        case btn_ambience:
            last_value = gs.Ambient;
            gs.Ambient = state = buttonsettings[item->button];
            if (gs.Ambient != last_value)
            {
                if (!InMenuLevel)
                {
                    if (gs.Ambient)
                        StartAmbientSound();
                    else
                        StopAmbientSound();
                }
            }
            break;
        case btn_flipstereo:
            last_value = gs.FlipStereo;
            gs.FlipStereo = state = buttonsettings[item->button];
            if (gs.FlipStereo != last_value)
                FlipStereo();
            break;
        case btn_shadows:
            gs.Shadows = state = buttonsettings[item->button];
            break;

        case btn_parental:
            if (gs.Password[0] != '\0' && gs.ParentalLock == TRUE)
            {
                if (passwordvalid)
                {
                    state = buttonsettings[btn_parental] = gs.ParentalLock = FALSE;
                    if (!InMenuLevel)
                        JS_ToggleLockouts();
                }
                else
                {
                    state = buttonsettings[btn_parental] = gs.ParentalLock = TRUE;
                    MenuInputMode = TRUE;
                    memset(MessageInputString, '\0', sizeof(MessageInputString));
                    KB_ClearKeysDown();
                    KB_FlushKeyboardQueue();
                }
            }
            else
            {
                gs.ParentalLock = state = buttonsettings[item->button];
                if (!InMenuLevel)
                    JS_ToggleLockouts();
            }
            break;

        case btn_videofs:
        {
            int lastx, lasty, lastbpp, newoffset, i;

            state = buttonsettings[btn_videofs];

            lastx   = validresolutions[slidersettings[sldr_videores]].xdim;
            lasty   = validresolutions[slidersettings[sldr_videores]].ydim;
            lastbpp = validbpps[slidersettings[sldr_videobpp]];
            UpdateValidModes(lastbpp, buttonsettings[btn_videofs]);

            // check if the last bpp is still a valid choice
            for (i=0; i<numvalidbpps; i++)
                if (validbpps[i] == lastbpp) break;
            if (i == numvalidbpps)
            {
                // it wasn't
                slidersettings[sldr_videobpp] = 0;
                lastbpp = validbpps[0];
                UpdateValidModes(lastbpp, buttonsettings[btn_videofs]);
            }
            else
            {
                slidersettings[sldr_videobpp] = i;
            }

            // find the nearest resolution to the one last selected
            newoffset = 0;
            for (i=0; i<numvalidresolutions; i++)
            {
                if (abs(lastx * lasty - validresolutions[i].xdim         * validresolutions[i].ydim) <
                    abs(lastx * lasty - validresolutions[newoffset].xdim * validresolutions[newoffset].ydim))
                    newoffset = i;
            }
            slidersettings[sldr_videores] = newoffset;
        } break;

        default:
            state = buttonsettings[item->button];
            break;
        }
    }

    if (!draw)
        return;

    switch (item->button)
    {
    case btn_mouse_aim:
        extra_text = gs.MouseAimingType ? "Momentary" : "Toggle";
        break;
    default: break;
    }


    state = buttonsettings[item->button];

    // Draw the button
    if (item->text)
    {
        if (state)
        {
            // set
            rotatesprite(button_x << 16, y << 16, MZ, 0, pic_radiobuttn2, shade, 0, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);
        }
        else
        {
            // not set
            rotatesprite(button_x << 16, y << 16, MZ, 0, pic_radiobuttn1, shade, 0, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);
        }

        MenuTextShade = shade;
        MNU_DrawString(x, y, item->text, MenuTextShade, 16);

        if (extra_text)
            MNU_DrawString(OPT_XSIDE + tilesiz[pic_radiobuttn1].x + 6, y, extra_text, MenuTextShade, 16);
        MenuTextShade = MENU_SHADE_DEFAULT;
    }
    else
    {
        if (state)
            rotatesprite(x << 16, y << 16, MZ, 0, pic_radiobuttn2, 2, 0, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);
        else
            rotatesprite(x << 16, y << 16, MZ, 0, pic_radiobuttn1, 2, 0, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);

        x += tilesiz[pic_radiobuttn1].x + 4;

        // Draw the menu item text
        rotatesprite(x << 16, y << 16, MZ, 0, item->pic, 2, 0, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);
    }

}

//char *gametype[] = {"War [Respawn]","Cooperative","War [No Respawn]"};
const char *gametype[] = {"WangBang (spawn)","WangBang (no spawn)","Cooperative"};
const char *playercolors[] = {"Brown","Gray","Purple","Red","Yellow","Olive","Green","Blue"};
const char *monsterskills[] = {"No Monsters","Easy","Normal","Hard","Insane!"};

void
MNU_DoSlider(short dir, MenuItem_p item, SWBOOL draw)
{
    short offset, i, barwidth;
    int x, y, knobx;
    short shade = MENU_SHADE_DEFAULT;
    const char *extra_text=NULL;
    char tmp_text[256];

    memset(tmp_text,0,256);

    if (TEST(item->flags, mf_disabled))
    {
        shade = MENU_SHADE_INACTIVE;
        dir = 0;
    }

    switch (item->slider)
    {
    case sldr_mouse:
        barwidth = SLDR_MOUSESENSEMAX;
        offset = slidersettings[sldr_mouse] += dir;

        if (TEST(item->flags, mf_disabled))
            break;

        offset = max(offset, short(0));
        offset = min(offset, short(SLDR_MOUSESENSEMAX-1));

        slidersettings[sldr_mouse] = offset;

        gs.MouseSpeed = offset * (MOUSE_SENS_MAX_VALUE/SLDR_MOUSESENSEMAX);
        CONTROL_MouseSensitivity = float(gs.MouseSpeed); // [JM] Will need to verify this. !CHECKME!
        break;

    case sldr_sndfxvolume:
        barwidth = SLDR_SNDFXVOLMAX;
        offset = slidersettings[sldr_sndfxvolume] += dir;

        if (TEST(item->flags, mf_disabled))
            break;

        offset = max(offset, short(0));
        offset = min(offset, short(SLDR_SNDFXVOLMAX-1));

        slidersettings[sldr_sndfxvolume] = offset;
        gs.SoundVolume = FX_MIN + (offset * VOL_MUL);
        FX_SetVolume(gs.SoundVolume);
        break;

    case sldr_musicvolume:
        barwidth = SLDR_MUSICVOLMAX;
        offset = slidersettings[sldr_musicvolume] += dir;
        if (TEST(item->flags, mf_disabled))
            break;

        offset = max(offset, short(0));
        offset = min(offset, short(SLDR_MUSICVOLMAX-1));

        slidersettings[sldr_musicvolume] = offset;
        gs.MusicVolume = MUSIC_MIN + (offset * VOL_MUL);
        SetSongVolume(gs.MusicVolume);
        break;

    case sldr_scrsize:
    {
        short bnum;

        barwidth = SLDR_SCRSIZEMAX;
        slidersettings[sldr_scrsize] = gs.BorderNum;
        slidersettings[sldr_scrsize] -= dir;
        offset = slidersettings[sldr_scrsize];

        if (TEST(item->flags, mf_disabled))
            break;

        ////DSPRINTF(ds,"BorderNum %d",gs.BorderNum);
        //MONO_PRINT(ds);

        offset = max(offset, short(0));
        offset = min(offset, short(SLDR_SCRSIZEMAX - 1));

        bnum = offset;

        offset = (SLDR_SCRSIZEMAX-1) - offset;
        slidersettings[sldr_scrsize] = offset;

        if (!BorderAdjust)
            gs.BorderNum = bnum;

        SetBorder(&Player[myconnectindex], bnum);

        break;
    }

    case sldr_brightness:
        barwidth = SLDR_BRIGHTNESSMAX;
        offset = slidersettings[sldr_brightness] += dir;

        if (TEST(item->flags, mf_disabled))
            break;

        offset = max(offset, short(0));
        offset = min(offset, short(SLDR_BRIGHTNESSMAX - 1));
        slidersettings[sldr_brightness] = offset;

        if (gs.Brightness != offset)
        {
            gs.Brightness = offset;
            COVERsetbrightness(gs.Brightness,&palette_data[0][0]);
        }
        break;

    case sldr_bordertile:
        barwidth = SLDR_BORDERTILEMAX;
        offset = slidersettings[sldr_bordertile] += dir;

        if (TEST(item->flags, mf_disabled))
            break;

        offset = max(offset, short(0));
        offset = min(offset, short(SLDR_BORDERTILEMAX - 1));
        slidersettings[sldr_bordertile] = offset;

        if (gs.BorderTile != offset)
        {
            gs.BorderTile = offset;

            SetRedrawScreen(&Player[myconnectindex]);
        }
        break;

    case sldr_gametype:
        barwidth = SLDR_GAMETYPEMAX;
        offset = slidersettings[sldr_gametype] += dir;

        if (TEST(item->flags, mf_disabled))
            break;

        offset = max(offset, short(0));
        offset = min(offset, short(SLDR_GAMETYPEMAX - 1));
        slidersettings[sldr_gametype] = offset;

        extra_text = gametype[offset];
        MNU_DrawString(OPT_XSIDE, item->y, extra_text, 1, 16);
        gs.NetGameType = offset;
        // friendly fire menu
        MNU_ItemPreProcess(currentmenu);
        break;

    case sldr_netlevel:
        barwidth = SLDR_NETLEVELMAX;
        offset = slidersettings[sldr_netlevel] += dir;

        if (TEST(item->flags, mf_disabled))
            break;

        offset = max(offset, short(0));
        offset = min(offset, short(SLDR_NETLEVELMAX - 1));
        slidersettings[sldr_netlevel] = offset;

        // Show the currently selected level on next line
        //extra_text = MNU_LevelName[offset];
        //MNU_DrawString(OPT_XS, item->y+10, extra_text, 1, 16);
        sprintf(tmp_text, "L%02d: %s", offset+1, LevelInfo[offset+1].Description);
        MNU_DrawString(OPT_XS, item->y+10, tmp_text, 1, 16);
        gs.NetLevel = offset;
        break;

    case sldr_monsters:
        barwidth = SLDR_MONSTERSMAX;
        offset = slidersettings[sldr_monsters] += dir;

        if (TEST(item->flags, mf_disabled))
            break;

        offset = max(offset, short(0));
        offset = min(offset, short(SLDR_MONSTERSMAX - 1));
        slidersettings[sldr_monsters] = offset;

        extra_text = monsterskills[offset];
        MNU_DrawString(OPT_XSIDE+54, item->y, extra_text, 1, 16);
        gs.NetMonsters = offset;
        break;

    case sldr_killlimit:
        barwidth = SLDR_KILLLIMITMAX;
        offset = slidersettings[sldr_killlimit] += dir;

        if (TEST(item->flags, mf_disabled))
            break;

        offset = max(offset, short(0));
        offset = min(offset, short(SLDR_KILLLIMITMAX - 1));
        slidersettings[sldr_killlimit] = offset;

        if (offset == 0)
        {
            strcpy(tmp_text,"Infinite\n");
        }
        else
        {
            sprintf(tmp_text,"%d",offset*10);
            //itoa(offset*10,tmp_text,10);
        }
        MNU_DrawString(OPT_XSIDE+101, item->y, tmp_text, 1, 16);
        gs.NetKillLimit = offset;
        break;

    case sldr_timelimit:
        barwidth = SLDR_TIMELIMITMAX;
        offset = slidersettings[sldr_timelimit] += dir;

        if (TEST(item->flags, mf_disabled))
            break;

        offset = max(offset, short(0));
        offset = min(offset, short(SLDR_TIMELIMITMAX - 1));
        slidersettings[sldr_timelimit] = offset;

        if (offset == 0)
        {
            strcpy(tmp_text,"Infinite\n");
        }
        else
        {
            sprintf(tmp_text,"%d Minutes\n",TimeLimitTable[offset]);
        }

        MNU_DrawString(OPT_XSIDE+86, item->y, tmp_text, 1, 16);
        gs.NetTimeLimit = offset;
        break;

    case sldr_playercolor:
        barwidth = SLDR_PLAYERCOLORMAX;
        offset = slidersettings[sldr_playercolor] += dir;

        if (TEST(item->flags, mf_disabled))
            break;

        offset = max(offset, short(0));
        offset = min(offset, short(SLDR_PLAYERCOLORMAX - 1));
        slidersettings[sldr_playercolor] = offset;

        extra_text = playercolors[offset];
        MNU_DrawString(OPT_XSIDE+78, item->y, extra_text, 1, PALETTE_PLAYER0+offset);
        gs.NetColor = offset;
        break;

    case sldr_videores:
    {
        offset = max(0,min(slidersettings[sldr_videores] + dir, numvalidresolutions-1));
        barwidth = numvalidresolutions;

        if (TEST(item->flags, mf_disabled))
            break;

        slidersettings[sldr_videores] = offset;

        sprintf(tmp_text, "%dx%d", validresolutions[offset].xdim, validresolutions[offset].ydim);
        MNU_DrawString(OPT_XSIDE, item->y+OPT_YINC, tmp_text, 1, 16);
    } break;

    case sldr_videobpp:
    {
        offset = max(0,min(slidersettings[sldr_videobpp] + dir, numvalidbpps-1));
        barwidth = numvalidbpps;

        if (TEST(item->flags, mf_disabled))
            break;

        if (slidersettings[sldr_videobpp] != offset)
        {
            int lastx, lasty, newoffset, i;

            slidersettings[sldr_videobpp] = offset;

            // find the nearest resolution to the one last selected
            lastx = validresolutions[slidersettings[sldr_videores]].xdim;
            lasty = validresolutions[slidersettings[sldr_videores]].ydim;
            UpdateValidModes(validbpps[offset], buttonsettings[btn_videofs]);
            newoffset = 0;
            for (i=0; i<numvalidresolutions; i++)
            {
                if (abs(lastx * lasty - validresolutions[i].xdim         * validresolutions[i].ydim) <
                    abs(lastx * lasty - validresolutions[newoffset].xdim * validresolutions[newoffset].ydim))
                    newoffset = i;
            }
            slidersettings[sldr_videores] = newoffset;
        }

        sprintf(tmp_text, "%d bpp", validbpps[offset]);
        MNU_DrawString(OPT_XSIDE+tilesiz[pic_slidelend].x+tilesiz[pic_sliderend].x+(barwidth+1)*tilesiz[pic_slidebar].x, item->y, tmp_text, 1, 16);
    } break;

    case sldr_mousescalex:
    case sldr_mousescaley:
        barwidth = 8+1+8;
        offset = slidersettings[item->slider] + dir;

        if (TEST(item->flags, mf_disabled))
            break;

        offset = max(offset, short(0));
        offset = min(offset, short(barwidth-1));

        if (slidersettings[item->slider] != offset)
        {
            slidersettings[item->slider] = offset;
            MouseAnalogScale[item->slider - sldr_mousescalex] = offset<<13;
            CONTROL_SetAnalogAxisScale(item->slider - sldr_mousescalex, offset<<13, controldevice_mouse);
        }

        sprintf(tmp_text, "%.2f", (float)(slidersettings[item->slider]<<13) / 65535.f);
        MNU_DrawSmallString(OPT_XSIDE+tilesiz[pic_slidelend].x+tilesiz[pic_sliderend].x+(MAX_SLDR_WIDTH+1)*tilesiz[pic_slidebar].x, item->y+4, tmp_text, 1, 16);
        break;

    case sldr_joyaxisscale:
        barwidth = 8+1+8;
        offset = slidersettings[item->slider] + dir;

        if (TEST(item->flags, mf_disabled))
            break;

        offset = max(offset, short(0));
        offset = min(offset, short(barwidth-1));

        if (slidersettings[item->slider] != offset)
        {
            slidersettings[item->slider] = offset;
            JoystickAnalogScale[JoystickAxisPage] = offset<<13;
            CONTROL_SetAnalogAxisScale(JoystickAxisPage, offset<<13, controldevice_joystick);
        }

        sprintf(tmp_text, "%.2f", (float)(slidersettings[item->slider]<<13) / 65535.f);
        MNU_DrawSmallString(OPT_XSIDE+tilesiz[pic_slidelend].x+tilesiz[pic_sliderend].x+(MAX_SLDR_WIDTH+1)*tilesiz[pic_slidebar].x, item->y+4, tmp_text, 1, 16);
        break;

    case sldr_joyaxisanalog:
    {
        const char *p;

        barwidth = MNU_ControlAxisOffset(analog_maxtype);
        offset = slidersettings[item->slider] + dir;

        if (TEST(item->flags, mf_disabled))
            break;

        offset = max(offset, short(0));
        offset = min(offset, short(barwidth-1));

        if (slidersettings[item->slider] != offset)
        {
            slidersettings[item->slider] = offset;
            JoystickAnalogAxes[JoystickAxisPage] = MNU_ControlAxisNum(offset);
            CONTROL_MapAnalogAxis(JoystickAxisPage, MNU_ControlAxisNum(offset), controldevice_joystick);
        }

        p = CONFIG_AnalogNumToName(MNU_ControlAxisNum(offset));
        while (*p != 0 && *p != '_') p++;
        if (*p == '_') p++;
        MNU_DrawSmallString(OPT_XSIDE+tilesiz[pic_slidelend].x+tilesiz[pic_sliderend].x+(barwidth+1)*tilesiz[pic_slidebar].x, item->y+4, p, 1, 16);
    }
    break;

    case sldr_joyaxisdead:
    case sldr_joyaxissatur:
        barwidth = (32768>>10)+1;
        offset = slidersettings[item->slider] + dir;

        if (TEST(item->flags, mf_disabled))
            break;

        offset = max(offset, short(0));
        offset = min(offset, short(barwidth-1));

        if (slidersettings[item->slider] != offset)
        {
            slidersettings[item->slider] = offset;
            if (item->slider == sldr_joyaxisdead)
            {
                JoystickAnalogDead[JoystickAxisPage] = min((offset<<10), 32767);
                //CONTROL_SetJoyAxisDead(JoystickAxisPage, JoystickAnalogDead[JoystickAxisPage]);
            }
            else
            {
                JoystickAnalogSaturate[JoystickAxisPage] = min((offset<<10), 32767);
                //CONTROL_SetJoyAxisSaturate(JoystickAxisPage, JoystickAnalogSaturate[JoystickAxisPage]);
            }

            joySetDeadZone(JoystickAxisPage, JoystickAnalogDead[JoystickAxisPage], JoystickAnalogSaturate[JoystickAxisPage]); // [JM] !CHECKME!
        }

        sprintf(tmp_text, "%.2f%%", (float)(slidersettings[item->slider]<<10) / 32767.f);
        MNU_DrawSmallString(OPT_XSIDE+tilesiz[pic_slidelend].x+tilesiz[pic_sliderend].x+(MAX_SLDR_WIDTH+1)*tilesiz[pic_slidebar].x, item->y+4, tmp_text, 1, 16);
        break;

    default:
        return;
    }

    if (!draw)
        return;

    // Now draw it
    item++;
    x = item->x;
    y = item->y;

    // Draw the left end cap of the bar
    rotatesprite(x << 16, y << 16, MZ, 0, pic_slidelend, shade, 0, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);

    x += tilesiz[pic_slidelend].x;
    knobx = x;

    // Draw the in between sections
    for (i = 0; i < min(barwidth, short(MAX_SLDR_WIDTH)); i++)
    {
        rotatesprite(x << 16, y << 16, MZ, 0, pic_slidebar, shade, 0, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);
        x += tilesiz[pic_slidebar].x;
    }

    // Draw the right end cap
    rotatesprite(x << 16, y << 16, MZ, 0, pic_sliderend, shade, 0, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);

    // Draw the knob, compressing the X coordinate if the bar is too wide
    if (barwidth > MAX_SLDR_WIDTH)
    {
        knobx += offset * (MAX_SLDR_WIDTH*tilesiz[pic_slidebar].x-tilesiz[pic_sliderknob].x) / (barwidth-1);
    }
    else
    {
        knobx += tilesiz[pic_slidebar].x * offset;
    }
    rotatesprite(knobx << 16, (y + 2) << 16, MZ, 0, pic_sliderknob, shade, 0, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);
}

////////////////////////////////////////////////
// Start up menu array
////////////////////////////////////////////////
static void
MNU_SetupMenu(void)
{
    MenuGroup *rootmenu;

    static MenuGroup *rootmenulist[] =
    {
        &maingroup,
        &SaveGameGroup,
        &LoadGameGroup,
        &soundgroup,
        &optiongroup,
        &quickloadgroup,
        &quitgroup,
        &ordergroup,
        &episodegroup,
    };

    rootmenu = rootmenulist[ControlPanelType];
    ASSERT(ControlPanelType < ct_max);

    menuarrayptr = 0;
    menuarray[0] = currentmenu = rootmenu;
    if (ControlPanelType == ct_mainmenu)

        mnu_input_buffered.button0 = mnu_input_buffered.button1 = FALSE;
    mnu_input_buffered.dir = dir_None;
    order_input_buffered.button0 = order_input_buffered.button1 = FALSE;
    order_input_buffered.dir = dir_None;
    ResetKeys();

    // custom cust_callback starts out as null
    cust_callback = NULL;

    // for QuitCustom and QuickLoadCustom
    if (currentmenu->items == NULL)
    {
        if (currentmenu->draw_custom)
            currentmenu->draw_custom(uc_setup, NULL);
    }

    if (ControlPanelType == ct_mainmenu)
        currentmenu->cursor = 0;

    // disable any items necessary
    MNU_ItemPreProcess(currentmenu);
}

////////////////////////////////////////////////
// Draw an item
////////////////////////////////////////////////
static void
MNU_ClearFlags(MenuGroup *node)
{
    MenuItem *i;

    if (!node->items)
        return;

    for (i = node->items; i->type != mt_none; i++)
    {
        i->flags &= ~MenuSelectFlags;
        if (i->child)
            MNU_ClearFlags((MenuGroup *) i->child);
    }
}

////////////////////////////////////////////////
// Pop a group off the menu stack
////////////////////////////////////////////////
static void
MNU_PopGroup(void)
{
    if (!menuarrayptr)
        return;

    currentmenu = menuarray[--menuarrayptr];

    SetFragBar(Player + myconnectindex);
    //PanelRefresh(Player + myconnectindex);
}

////////////////////////////////////////////////
// Push a group on to the menu stack
////////////////////////////////////////////////
static void
MNU_PushGroup(MenuGroup *node)
{
    if (menuarrayptr == MaxLayers - 1)
        return;

    currentmenu = menuarray[++menuarrayptr] = node;

    SetFragBar(Player + myconnectindex);
}

////////////////////////////////////////////////
// Setup a new menu subgroup
////////////////////////////////////////////////
static void
MNU_SetupGroup(void)
{
    MNU_SelectItem(currentmenu, currentmenu->cursor, FALSE);
    MNU_DrawMenu();
}

static void
MNU_ItemPreProcess(MenuGroup *group)
{
    MenuItem *item;

    if (!group->items)
        return;

    // process all items when going down a level
    // to see if anything is disabled
    for (item = group->items; item->type != mt_none; item++)
    {
        if (item->preprocess)
            item->preprocess(item);
    }
}

void
MNU_ItemPostProcess(MenuGroup *group)
{
    MenuItem *item;
    int zero = 0;

    if (!group->items)
        return;

    item = &currentmenu->items[currentmenu->cursor];

    if (item->postprocess)
    {
        item->postprocess(item);
    }
}

////////////////////////////////////////////////
// Go to next menu subgroup
////////////////////////////////////////////////
static void
MNU_DownLevel(MenuGroup *group)
{

    if (!group)
    {
        TerminateGame();
        printf("MNU_DownLevel() - NULL card\n");
        exit(0);
    }

    MNU_PushGroup(group);

    if (group->items == NULL)
    {
        if (group->draw_custom && group->draw_custom(uc_setup, NULL))
            MNU_PopGroup();
    }

    MNU_ItemPreProcess(currentmenu);

    MNU_SetupGroup();

    SetRedrawScreen(&Player[myconnectindex]);
}

////////////////////////////////////////////////
// Go to previous menu subgroup
////////////////////////////////////////////////
static void
MNU_UpLevel(void)
{
    int zero = 0;
    static int handle1=0;
    // if run out of menus then EXIT
    if (!menuarrayptr)
    {
        if (!FX_SoundActive(handle1))
            handle1 = PlaySound(DIGI_STARCLINK,&zero,&zero,&zero,v3df_dontpan);
        ExitMenus();
        return;
    }

    if (currentmenu->items)
        currentmenu->items[currentmenu->cursor].flags &= ~mf_selected;
    MNU_PopGroup();
    MNU_SetupGroup();

    SetRedrawScreen(&Player[myconnectindex]);
}

////////////////////////////////////////////////
// Do a menu item action
////////////////////////////////////////////////
static void
MNU_DoItem(void)
{
    MenuItem *item;

    item = &currentmenu->items[currentmenu->cursor];
    if (!item) return;

    if (TEST(item->flags, mf_disabled))
    {
        // Try to process again
        if (item->preprocess)
            item->preprocess(item);

        // Check once more
        if (TEST(item->flags, mf_disabled))
            return;
    }

    switch (item->type)
    {
    case mt_option:
        if (item->custom != NULL)
            item->custom();
        break;
    case mt_button:
        MNU_PushItem(item, FALSE);
        if (item->custom != NULL)
            item->custom();
        break;
    case mt_layer:
        if (item->custom != NULL)
            item->custom();
        MNU_DownLevel(item->child);
        break;
    default: break;
    }
}

////////////////////////////////////////////////
// Draw an item icon or cursor
////////////////////////////////////////////////
static void
MNU_DrawItemIcon(MenuItem *item)
{
    //void BorderRefreshClip(PLAYERp pp, short x, short y, short x2, short y2);
    int x = item->x, y = item->y;
    int scale = MZ;
    short w,h;

    if (item->text)
    {
        scale /= 2;
        x -= mulscale17(tilesiz[pic_yinyang].x,scale) + 2;
        y += 4;
    }
    else
    {
        scale -= (1<<13);
        x -= ((tilesiz[pic_yinyang].x) / 2) - 3;
        y += 8;
    }

    rotatesprite(x << 16, y << 16,
                 scale, 0, pic_yinyang, item->shade, 0, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);

    SetRedrawScreen(&Player[myconnectindex]);
    //BorderRefreshClip(&Player[myconnectindex], x - 24, y - 24, x + 24, y + 24);
}

////////////////////////////////////////////////
// Draw an item
////////////////////////////////////////////////
static void
MNU_DrawItem(MenuItem *item)
{
    char *ptr;
    short px, py;

    MNU_ItemPostProcess(currentmenu);  // Put this in so things can be drawn on item select

    if (!item->pic)
        return;

    MNU_DrawItemIcon(item);

    // if text string skip this part
    if (item->text)
        return;

    if (TEST(item->flags, mf_selected) && !TEST(item->flags, mf_disabled))
    {
        // Highlighted
        if (item->type != mt_button)
            rotatesprite(item->x << 16, item->y << 16, MZ, 0, item->pic,
                         -30 + STD_RANDOM_RANGE(50), PALETTE_MENU_HIGHLIGHT, MenuDrawFlags,
                         0, 0, xdim - 1, ydim - 1);
        else
            rotatesprite((item->x + tilesiz[pic_radiobuttn1].x + 4) << 16, item->y << 16,
                         MZ, 0, item->pic, item->shade, PALETTE_MENU_HIGHLIGHT, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);
    }
    else
    {
        // Un highlighted
        if (item->type != mt_button)
            rotatesprite(item->x << 16, item->y << 16, MZ, 0, item->pic,
                         item->shade, 0, MenuDrawFlags, 0, 319, 199, 0);
        else
            rotatesprite((item->x + tilesiz[pic_radiobuttn1].x + 4) << 16, item->y << 16,
                         MZ, 0, item->pic, item->shade, 0, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);
    }
}

////////////////////////////////////////////////
// Draw the menu contents
////////////////////////////////////////////////
static void
MNU_DrawMenuContents(void)
{
    MenuItem *item;
    short w,h;

    ASSERT(currentmenu != NULL);

    if (currentmenu->text)
    {
        // Draw the backdrop bar
        rotatesprite(10 << 16, (currentmenu->y-3) << 16, MZ, 0, 2427,
                     currentmenu->shade, 0, MenuDrawFlags|ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);
        MNU_MeasureStringLarge(currentmenu->text, &w, &h);
        MNU_DrawString(TEXT_XCENTER(w), currentmenu->y, currentmenu->text, 1, 16);
    }
    else if (currentmenu->titlepic)
    {
        rotatesprite(currentmenu->x << 16, currentmenu->y << 16, MZ, 0, currentmenu->titlepic,
                     currentmenu->shade, 0, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);
    }

    if (!currentmenu->items)
        return;

    for (item = currentmenu->items; item->type != mt_none; item++)
    {
        if (item->pic)
        {
            if (item->type == mt_button)
            {
                // all drawing done here also
                MNU_DoButton(item, TRUE);
            }
            else
            {
                if (item->text)
                {
                    if (TEST(item->flags, mf_disabled))
                        MenuTextShade = MENU_SHADE_INACTIVE;
                    MNU_DrawString(item->x, item->y, item->text, MenuTextShade, 16);
                    MenuTextShade = MENU_SHADE_DEFAULT;
                }
                else
                {
                    rotatesprite(item->x << 16, item->y << 16, MZ, 0, item->pic,
                                 item->shade, 0, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);
                }
            }
        }

        // Is there a slider attached to this item?  Draw it.
        if (item->type == mt_slider)
            MNU_DoSlider(0, item, TRUE);
    }

    MNU_SelectItem(currentmenu, currentmenu->cursor, TRUE);

    if (currentmenu->draw_custom)
        currentmenu->draw_custom(uc_touchup, NULL);
}

////////////////////////////////////////////////
// Draw the menu
////////////////////////////////////////////////
void
MNU_DrawMenu(void)
{
    if (cust_callback != NULL)
    {
        cust_callback(cust_callback_call, cust_callback_item);
        return;
    }

    if (currentmenu->items || currentmenu->titlepic)
    {
        MNU_DrawMenuContents();
    }
}

////////////////////////////////////////////////
// Select a menu item
////////////////////////////////////////////////
void
MNU_SelectItem(MenuGroup *group, short index, SWBOOL draw)
{
    MenuItem *item;

    if (index != group->cursor)
    {
        item = &group->items[group->cursor];
        item->flags &= ~mf_selected;
        if (draw)
            MNU_DrawItem(item);
    }

    group->cursor = index;
    item = &group->items[group->cursor];
    item->flags |= mf_selected;
    if (draw)
        MNU_DrawItem(item);
}

////////////////////////////////////////////////
// Toggle a menu radio button on/off
////////////////////////////////////////////////
static void
MNU_PushItem(MenuItem *item, SWBOOL draw)
{
    if (item->type != mt_button)
        return;

    buttonsettings[item->button] ^= 1;

//    if (draw)
    MNU_DoButton(item, draw);
}

////////////////////////////////////////////////
// Go to next item on menu
////////////////////////////////////////////////
static void
MNU_NextItem(void)
{
    MenuTag type;
    MenuFlags flag;

    type = currentmenu->items[currentmenu->cursor + 1].type;
    flag = currentmenu->items[currentmenu->cursor + 1].flags;

    if (type == mt_none)
        MNU_SelectItem(currentmenu, 0, FALSE);
    else
        MNU_SelectItem(currentmenu, currentmenu->cursor + 1, FALSE);

    type = currentmenu->items[currentmenu->cursor].type;
    flag = currentmenu->items[currentmenu->cursor].flags;

    if (type == mt_inert || flag == mf_disabled)
        MNU_NextItem();
}

////////////////////////////////////////////////
// Go to previous item on menu
////////////////////////////////////////////////
static void
MNU_PrevItem(void)
{
    MenuTag type;
    MenuFlags flag;

    if (!currentmenu->cursor)
        while (currentmenu->items[++currentmenu->cursor].type != mt_none) ;

    MNU_SelectItem(currentmenu, currentmenu->cursor - 1, FALSE);

    type = currentmenu->items[currentmenu->cursor].type;
    flag = currentmenu->items[currentmenu->cursor].flags;
    if (type == mt_inert || flag == mf_disabled)
        MNU_PrevItem();
}

////////////////////////////////////////////////
// Find hotkey press on current menu, if any.
////////////////////////////////////////////////
static SWBOOL
MNU_DoHotkey(void)
{
    MenuItem_p item;
    short index;

    if (!currentmenu->items) return FALSE;

    index = 0;
    for (item = currentmenu->items; item->type != mt_none; item++)
    {
        if (KEY_PRESSED(item->hotkey) && item->hotkey != 0)
        {
            MNU_SelectItem(currentmenu, index, FALSE);
            return TRUE;
        }
        index++;
    }

    return FALSE;
}

////////////////////////////////////////////////
// Setup Menus
////////////////////////////////////////////////
void
SetupMenu(void)
{
    if (!UsingMenus && !ConPanel)       // Doing this check for multiplay
    // menus
    {
        MNU_SetupMenu();

        // Clear the previous ESC key press
        KEY_PRESSED(KEYSC_ESC) = FALSE;
        UsingMenus = TRUE;
    }
}

////////////////////////////////////////////////
// Setup the main menu
// This function will not loop if in modem
// or network game, otherwise it stops the
// game play until user finished in menus.
////////////////////////////////////////////////
#define MNU_SENSITIVITY 10              // The menu's mouse sensitivity, should be real low

void MNU_DoMenu(CTLType type, PLAYERp pp)
{
    SWBOOL resetitem;
    UCHAR key;
    int zero = 0;
    static int handle2 = 0;
    static int limitmove=0;
    static SWBOOL select_held=FALSE;

    resetitem = TRUE;

    if (cust_callback != NULL)
    {
        cust_callback(cust_callback_call, cust_callback_item);
        return;
    }

    //ControlPanelType = type;
    SetupMenu();

    // Zero out the input structure
    mnu_input.button0 = mnu_input.button1 = FALSE;
    mnu_input.dir = dir_None;

    // should not get input if you are editing a save game slot
    if (totalclock < limitmove) limitmove = (int32_t) totalclock;
    if (!MenuInputMode)
    {
        UserInput tst_input;
        SWBOOL select_held = FALSE;


        // Zero out the input structure
        tst_input.button0 = tst_input.button1 = FALSE;
        tst_input.dir = dir_None;

        if (!select_held)
        {
            CONTROL_GetUserInput(&tst_input);
            mnu_input_buffered.dir = tst_input.dir;
        }

        if (mnu_input_buffered.button0 || mnu_input_buffered.button1)
        {
            if (tst_input.button0 == mnu_input_buffered.button0 &&
                tst_input.button1 == mnu_input_buffered.button1)
            {
                select_held = TRUE;
            }
            else if (totalclock - limitmove > 7)
            {
                mnu_input.button0 = mnu_input_buffered.button0;
                mnu_input.button1 = mnu_input_buffered.button1;

                mnu_input_buffered.button0 = tst_input.button0;
                mnu_input_buffered.button1 = tst_input.button1;
            }
        }
        else
        {
            select_held = FALSE;
            mnu_input_buffered.button0 = tst_input.button0;
            mnu_input_buffered.button1 = tst_input.button1;
        }

        if (totalclock - limitmove > 7 && !select_held)
        {
            mnu_input.dir = mnu_input_buffered.dir;

            if (mnu_input.dir != dir_None)
                if (!FX_SoundActive(handle2))
                    handle2 = PlaySound(DIGI_STAR,&zero,&zero,&zero,v3df_dontpan);

            limitmove = (int32_t) totalclock;
            mnu_input_buffered.dir = dir_None;
        }
    }

    if (mnu_input.dir == dir_North)
    {
        MNU_PrevItem();
        resetitem = TRUE;
    }
    else if (mnu_input.dir == dir_South)
    {
        MNU_NextItem();
        resetitem = TRUE;
    }
    else if (mnu_input.button0)
    {
        static int handle5=0;
        if (!FX_SoundActive(handle5))
            handle5 = PlaySound(DIGI_SWORDSWOOSH,&zero,&zero,&zero,v3df_dontpan);
        KB_ClearKeysDown();
        MNU_DoItem();
        resetitem = TRUE;
    }
    else if (mnu_input.dir == dir_West
             && currentmenu->items[currentmenu->cursor].type == mt_slider)
    {
        MNU_DoSlider(-1, &currentmenu->items[currentmenu->cursor], FALSE);
        resetitem = TRUE;
    }
    else if (mnu_input.dir == dir_East
             && currentmenu->items[currentmenu->cursor].type == mt_slider)
    {
        MNU_DoSlider(1, &currentmenu->items[currentmenu->cursor], FALSE);
        resetitem = TRUE;
    }
    else if (mnu_input.button1 || BUTTON(gamefunc_Show_Menu))
    {
        static int handle3=0;
        CONTROL_ClearButton(gamefunc_Show_Menu);
        if (!FX_SoundActive(handle3))
            handle3 = PlaySound(DIGI_SWORDSWOOSH,&zero,&zero,&zero,v3df_dontpan);
        MNU_UpLevel();
        resetitem = TRUE;
    }
    else if (MNU_DoHotkey())
    {
        static int handle4=0;
        if (!FX_SoundActive(handle4))
            handle4 = PlaySound(DIGI_STAR,&zero,&zero,&zero,v3df_dontpan);
        resetitem = TRUE;
        mnu_input_buffered.button0 = mnu_input_buffered.button1 = FALSE;
    }
    else
        resetitem = FALSE;

    // !FRANK! I added this because the old custom was only called for drawing
    // Needed one for drawing and moving.
    if (currentmenu->move_custom)
        currentmenu->move_custom(uc_setup, NULL);

    if (resetitem)
    {
        KB_ClearKeysDown();
        ResetKeys();
    }
}

////////////////////////////////////////////////
//  Checks to see if we should be in menus
////////////////////////////////////////////////
void
MNU_CheckForMenus(void)
{
    extern SWBOOL GamePaused;

    if (UsingMenus)
    {
        //if (MoveSkip2 == 0)
        MNU_DoMenu(ct_mainmenu, Player + myconnectindex);
    }
    else
    {
        if ((KEY_PRESSED(KEYSC_ESC) || BUTTON(gamefunc_Show_Menu)) && dimensionmode == 3 && !ConPanel)
        {
            KEY_PRESSED(KEYSC_ESC) = 0;
            CONTROL_ClearButton(gamefunc_Show_Menu);
            KB_ClearKeysDown();
            // setup sliders/buttons
            MNU_InitMenus();
            MNU_DoMenu(ct_mainmenu, Player + myconnectindex);
            pMenuClearTextLine(Player + myconnectindex);
            PauseGame();
        }
    }
}

void
MNU_CheckForMenusAnyKey(void)
{
    if (UsingMenus)
    {
        //if (MoveSkip2 == 0)
        MNU_DoMenu(ct_mainmenu, Player + myconnectindex);
    }
    else
    {
        if (KeyPressed())
        {
            ResetKeys();
            KB_ClearKeysDown();
            MNU_InitMenus();
            MNU_DoMenu(ct_mainmenu, Player + myconnectindex);
            pMenuClearTextLine(Player + myconnectindex);
        }
    }
}

static int MNU_ControlAxisOffset(int num)
{
    switch (num)
    {
    case analog_turning: return 0;
    case analog_strafing: return 1;
    case analog_moving: return 2;
    case analog_lookingupanddown: return 3;
    case analog_maxtype: return 4;
    default: return 0;
    }
}

static int MNU_ControlAxisNum(int offset)
{
    switch (offset)
    {
    case 0: return analog_turning;
    case 1: return analog_strafing;
    case 2: return analog_moving;
    case 3: return analog_lookingupanddown;
    default: return analog_turning;
    }
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// Miscellaneous Routines
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RGB_color_typ
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} RGB_color, *RGB_color_ptr;

#define PALETTE_MASK            0x3c6
#define PALETTE_READ            0x3c7
#define PALETTE_WRITE           0x3c8
#define PALETTE_DATA            0x3c9

unsigned char palette_data[256][3];     // Global palette array

// V E R T I C A L   R E T R A C E   V A R I A B L E S //////////////////////////////////////////

#define VGA_INPUT_STATUS_1      0x3DA   // VGA status register 1, bit 3 is the vsync
// 1 = retrace in progress
// 0 = no retrace
#define VGA_VSYNC_MASK          0x08    // Masks off unwanted bits of status register.


// These routines are not used and should not be used.  Would interfere with VESA palette
// cards.
#if 0
/////////////////////////////////////////////////
// WaitForVsync
// Waits for a vertical retrace to occur.  If one is in progress, it waits for the next one.
/////////////////////////////////////////////////
void
WaitForVsync(void)
{
    while (inp(VGA_INPUT_STATUS_1) & VGA_VSYNC_MASK) ;
    // Retrace in progress, wait.

    // Wait for vsync, and exit.
    while (!inp(VGA_INPUT_STATUS_1) & VGA_VSYNC_MASK) ;
}

void
Get_Palette(unsigned char *pal)
{
    int i;

    outp(PALETTE_READ, 0);
    for (i = 0; i < 768; i++)
        pal[i] = inp(PALETTE_DATA);
}
void
Set_Palette(unsigned char *buff)
{
    int i;

    outp(PALETTE_WRITE, 0);             // Resets color ram pointer to 1st
    // color
    for (i = 0; i < 768; i++)
        outp(PALETTE_DATA, buff[i]);
}

#endif


/*
=================================================================================================
=
=       FadeOut - Fades the palette to color at assigned click rate
=
=================================================================================================
*/
// Heres some temp timer junk for this routine.  Replace it with game timer stuff later.
//unsigned int *clock  = (unsigned int *)0x046C;

void
Fade_Timer(int clicks)
{
//        unsigned int now;
    int now;

    now = (int32_t) totalclock;

    while (abs((int32_t) totalclock - now) < clicks) handleevents();
}

void
FadeIn(unsigned char startcolor, unsigned int clicks)
{
    int i, palreg, usereg, tmpreg1 = 0, tmpreg2 = 0;
    RGB_color color;
    unsigned char temp_pal[768], *palette;

    if (videoGetRenderMode() >= REND_POLYMOST) return;

    palette = &palette_data[0][0];

    color.red = palette_data[startcolor][0];
    color.green = palette_data[startcolor][1];
    color.blue = palette_data[startcolor][2];

    usereg = 0;
    for (i = 0; i < 768; i++)
    {
        if (usereg == 0)
            temp_pal[i] = color.red;
        else if (usereg == 1)
            temp_pal[i] = color.green;
        else
            temp_pal[i] = color.blue;

        if (++usereg > 2)
            usereg = 0;
    }

    for (i = 0; i < 32; i++)
    {
        for (palreg = 0; palreg < 768; palreg++)
        {
            tmpreg1 = (int)(temp_pal[palreg]) + 2;
            tmpreg2 = (int)(temp_pal[palreg]) - 2;
            if (tmpreg1 > 255)
                tmpreg1 = 255;
            if (tmpreg2 < 0)
                tmpreg2 = 0;

            if (temp_pal[palreg] < palette[palreg])
            {
                if ((temp_pal[palreg] = tmpreg1) > palette[palreg])
                    temp_pal[palreg] = palette[palreg];
            }
            else if (temp_pal[palreg] > palette[palreg])
                if ((temp_pal[palreg] = tmpreg2) < palette[palreg])
                    temp_pal[palreg] = palette[palreg];

        }

        set_pal(&temp_pal[0]);

        // Delay clicks
        Fade_Timer(clicks);
    }
}

void
FadeOut(unsigned char targetcolor, unsigned int clicks)
{
    int i, palreg, usereg = 0, tmpreg1 = 0, tmpreg2 = 0;
    RGB_color color;
    unsigned char temp_pal[768];

    if (videoGetRenderMode() >= REND_POLYMOST) return;

    color.red = palette_data[targetcolor][0];
    color.green = palette_data[targetcolor][1];
    color.blue = palette_data[targetcolor][2];

    memcpy(&temp_pal[0], &palette_data[0][0], 768);

    for (i = 0; i < 32; i++)
    {
        for (palreg = 0; palreg < 768; palreg++)
        {
            tmpreg1 = (int)(temp_pal[palreg]) + 2;
            tmpreg2 = (int)(temp_pal[palreg]) - 2;
            if (tmpreg1 > 255)
                tmpreg1 = 255;
            if (tmpreg2 < 0)
                tmpreg2 = 0;

            if (usereg == 0)
            {
                if (temp_pal[palreg] < color.red)
                {
                    if ((temp_pal[palreg] = tmpreg1) > color.red)
                        temp_pal[palreg] = color.red;
                }
                else if (temp_pal[palreg] > color.red)
                    if ((temp_pal[palreg] = tmpreg2) < color.red)
                        temp_pal[palreg] = color.red;
            }
            else if (usereg == 1)
            {
                if (temp_pal[palreg] < color.green)
                {
                    if ((temp_pal[palreg] = tmpreg1) > color.green)
                        temp_pal[palreg] = color.green;
                }
                else if (temp_pal[palreg] > color.green)
                    if ((temp_pal[palreg] = tmpreg2) < color.green)
                        temp_pal[palreg] = color.green;
            }
            else if (usereg == 2)
            {
                if (temp_pal[palreg] < color.blue)
                {
                    if ((temp_pal[palreg] = tmpreg1) > color.blue)
                        temp_pal[palreg] = color.blue;
                }
                else if (temp_pal[palreg] > color.blue)
                    if ((temp_pal[palreg] = tmpreg2) < color.blue)
                        temp_pal[palreg] = color.blue;
            }

            if (++usereg > 2)
                usereg = 0;
        }


        set_pal(&temp_pal[0]);

        // Delay clicks
        Fade_Timer(clicks);
    }
}

//////////////////////////////////////////////////////////////////////////////
#define FADE_DAMAGE_FACTOR  3   // 100 health / 32 shade cycles = 3.125

// Fades from 100% to 62.5% somewhat quickly,
//  then from 62.5% to 37.5% slowly,
//  then from 37.5% to 0% quickly.
// This seems to capture the pain caused by enemy shots, plus the extreme
//  fade caused by being blinded or intense pain.
// Perhaps the next step would be to apply a gentle smoothing to the
//  intersections of these lines.
static int faderamp[32] =
{
    // y=64-4x
    252,240,224,208,192,176,

    // y=44.8-(16/20)x
    160,156,152,152,148,
    144,140,136,136,132,
    128,124,120,120,116,
    112,108,104,104,100,

    // y=128-4x
    96,80,64,48,32,16
};

unsigned char ppalette[MAX_SW_PLAYERS_REG][768];

//////////////////////////////////////////
// Set the amount of redness for damage
// the player just took
//////////////////////////////////////////
void
SetFadeAmt(PLAYERp pp, short damage, unsigned char startcolor)
{
    int palreg, usereg = 0, tmpreg1 = 0, tmpreg2 = 0;
    short fadedamage=0;
    RGB_color color;

    //CON_ConMessage("SetAmt: fadeamt = %d, startcolor = %d, pp = %d",pp->FadeAmt,startcolor,pp->StartColor);

    if (abs(pp->FadeAmt) > 0 && startcolor == pp->StartColor)
        return;

    // Don't ever over ride flash bomb
    if (pp->StartColor == 1 && abs(pp->FadeAmt) > 0)
        return;

    // Reset the palette
    if (pp == Player + screenpeek)
    {
        if (videoGetRenderMode() < REND_POLYMOST)
            COVERsetbrightness(gs.Brightness,&palette_data[0][0]);
        else
            videoFadePalette(0,0,0,0);
        if (pp->FadeAmt <= 0)
            GetPaletteFromVESA(&ppalette[screenpeek][0]);
    }

    if (damage < -150 && damage > -1000) fadedamage = 150;
    else if (damage < -1000) // Underwater
        fadedamage = abs(damage+1000);
    else
        fadedamage = abs(damage);

    if (damage >= -5 && damage < 0)
        fadedamage += 10;

    // Don't let red to TOO red
    if (startcolor == COLOR_PAIN && fadedamage > 100) fadedamage = 100;

    pp->FadeAmt = fadedamage / FADE_DAMAGE_FACTOR;

    if (pp->FadeAmt <= 0)
    {
        pp->FadeAmt = 0;
        return;
    }

    // It's a health item, just do a preset flash amount
    if (damage > 0)
        pp->FadeAmt = 3;

    pp->StartColor = startcolor;

    pp->FadeTics = 0;

    // Set player's palette to current game palette
    GetPaletteFromVESA(pp->temp_pal);

    color.red = palette_data[pp->StartColor][0];
    color.green = palette_data[pp->StartColor][1];
    color.blue = palette_data[pp->StartColor][2];

    for (palreg = 0; palreg < 768; palreg++)
    {
        tmpreg1 = (int)(pp->temp_pal[palreg]) + ((2 * pp->FadeAmt) + 4);
        tmpreg2 = (int)(pp->temp_pal[palreg]) - ((2 * pp->FadeAmt) + 4);
        if (tmpreg1 > 255)
            tmpreg1 = 255;
        if (tmpreg2 < 0)
            tmpreg2 = 0;

        if (usereg == 0)
        {
            if (pp->temp_pal[palreg] < color.red)
            {
                if ((pp->temp_pal[palreg] = tmpreg1) > color.red)
                    pp->temp_pal[palreg] = color.red;
            }
            else if (pp->temp_pal[palreg] > color.red)
                if ((pp->temp_pal[palreg] = tmpreg2) < color.red)
                    pp->temp_pal[palreg] = color.red;
        }
        else if (usereg == 1)
        {
            if (pp->temp_pal[palreg] < color.green)
            {
                if ((pp->temp_pal[palreg] = tmpreg1) > color.green)
                    pp->temp_pal[palreg] = color.green;
            }
            else if (pp->temp_pal[palreg] > color.green)
                if ((pp->temp_pal[palreg] = tmpreg2) < color.green)
                    pp->temp_pal[palreg] = color.green;
        }
        else if (usereg == 2)
        {
            if (pp->temp_pal[palreg] < color.blue)
            {
                if ((pp->temp_pal[palreg] = tmpreg1) > color.blue)
                    pp->temp_pal[palreg] = color.blue;
            }
            else if (pp->temp_pal[palreg] > color.blue)
                if ((pp->temp_pal[palreg] = tmpreg2) < color.blue)
                    pp->temp_pal[palreg] = color.blue;
        }

        if (++usereg > 2)
            usereg = 0;
    }

    // Do initial palette set
    if (pp == Player + screenpeek)
    {
        if (videoGetRenderMode() < REND_POLYMOST) set_pal(pp->temp_pal);
        else videoFadePalette(color.red, color.green, color.blue, faderamp[min(31,max(0,32-abs(pp->FadeAmt)))]);
        if (damage < -1000)
            pp->FadeAmt = 1000;  // Don't call DoPaletteFlash for underwater stuff
    }
}

//////////////////////////////////////////
// Do the screen reddness based on damage
//////////////////////////////////////////
#define MAXFADETICS     5
void
DoPaletteFlash(PLAYERp pp)
{
    int i, palreg, tmpreg1 = 0, tmpreg2 = 0;
    unsigned char *pal_ptr = &ppalette[screenpeek][0];


    if (pp->FadeAmt <= 1)
    {
        pp->FadeAmt = 0;
        pp->StartColor = 0;
        if (pp == Player + screenpeek)
        {
            if (videoGetRenderMode() < REND_POLYMOST)
                COVERsetbrightness(gs.Brightness,&palette_data[0][0]);
            else
                videoFadePalette(0,0,0,0);
            memcpy(pp->temp_pal, palette_data, sizeof(palette_data));
            DoPlayerDivePalette(pp);  // Check Dive again
            DoPlayerNightVisionPalette(pp);  // Check Night Vision again
        }

        return;
    }


    pp->FadeTics += synctics;           // Add this frame's tic amount to
    // counter

    if (pp->FadeTics >= MAXFADETICS)
    {
        while (pp->FadeTics >= MAXFADETICS)
        {
            pp->FadeTics -= MAXFADETICS;

            pp->FadeAmt--;              // Decrement FadeAmt till it gets to
            // 0 again.
        }
    }
    else
        return;                         // Return if they were not >
    // MAXFADETICS

    if (pp->FadeAmt > 32)
        return;

    if (pp->FadeAmt <= 1)
    {
        pp->FadeAmt = 0;
        pp->StartColor = 0;
        if (pp == Player + screenpeek)
        {
            if (videoGetRenderMode() < REND_POLYMOST)
                COVERsetbrightness(gs.Brightness,&palette_data[0][0]);
            else
                videoFadePalette(0,0,0,0);
            memcpy(pp->temp_pal, palette_data, sizeof(palette_data));
            DoPlayerDivePalette(pp);  // Check Dive again
            DoPlayerNightVisionPalette(pp);  // Check Night Vision again
        }
        return;
    }
    else
    {
        //CON_Message("gamavalues = %d, %d, %d",pp->temp_pal[pp->StartColor],pp->temp_pal[pp->StartColor+1],pp->temp_pal[pp->StartColor+2]);
        for (palreg = 0; palreg < 768; palreg++)
        {
            tmpreg1 = (int)(pp->temp_pal[palreg]) + 2;
            tmpreg2 = (int)(pp->temp_pal[palreg]) - 2;
            if (tmpreg1 > 255)
                tmpreg1 = 255;
            if (tmpreg2 < 0)
                tmpreg2 = 0;

            if (pp->temp_pal[palreg] < pal_ptr[palreg])
            {
                if ((pp->temp_pal[palreg] = tmpreg1) > pal_ptr[palreg])
                    pp->temp_pal[palreg] = pal_ptr[palreg];
            }
            else if (pp->temp_pal[palreg] > pal_ptr[palreg])
                if ((pp->temp_pal[palreg] = tmpreg2) < pal_ptr[palreg])
                    pp->temp_pal[palreg] = pal_ptr[palreg];

        }

        // Only hard set the palette if this is currently the player's view
        if (pp == Player + screenpeek)
        {
            if (videoGetRenderMode() < REND_POLYMOST) set_pal(pp->temp_pal);
            else
            {
                videoFadePalette(
                    palette_data[pp->StartColor][0],
                    palette_data[pp->StartColor][1],
                    palette_data[pp->StartColor][2],
                    faderamp[min(31,max(0,32-abs(pp->FadeAmt)))]
                    );
            }
        }

    }

}

void ResetPalette(PLAYERp pp)
{
    if (videoGetRenderMode() < REND_POLYMOST)
        COVERsetbrightness(gs.Brightness,&palette_data[0][0]);
    else
        videoFadePalette(0,0,0,0);
    memcpy(pp->temp_pal, palette_data, sizeof(palette_data));
    //DoPlayerDivePalette(pp);  // Check Dive again
    //DoPlayerNightVisionPalette(pp);  // Check Night Vision again
    pp->FadeAmt = 0;
    pp->StartColor = 0;
    pp->FadeTics = 0;
}

// vim:ts=4:sw=4:enc=utf-8:


