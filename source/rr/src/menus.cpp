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

#include "compat.h"
#include "duke3d.h"
#include "osdcmds.h"
#include "savegame.h"
#include "demo.h"
#include "input.h"
#include "menus.h"
#include "cheats.h"
#include "gamecvars.h"
#include "menu/menu.h"
#include "version.h"
#include "../../glbackend/glbackend.h"

BEGIN_RR_NS

#if 0

// common positions
#define MENU_MARGIN_REGULAR 40
#define MENU_MARGIN_WIDE    32
#define MENU_MARGIN_CENTER  160
#define MENU_HEIGHT_CENTER  100


#define USERMAPENTRYLENGTH 25

static FORCE_INLINE void Menu_StartTextInput()
{
    inputState.keyFlushChars();
    inputState.ClearKeysDown();
}




/*
All MAKE_* macros are generally for the purpose of keeping state initialization
separate from actual data. Alternatively, they can serve to factor out repetitive
stuff and keep the important bits from getting lost to our eyes.

They serve as a stand-in for C++ default value constructors, since we're using C89.

Note that I prefer to include a space on the inside of the macro parentheses, since
they effectively stand in for curly braces as struct initializers.
*/


// common font types
// tilenums are set after namesdyn runs

static MenuMenuFormat_t MMF_Top_Main =             { {  MENU_MARGIN_CENTER<<16, 55<<16, }, -(170<<16) };
static MenuMenuFormat_t MMF_Top_Episode =          { {  MENU_MARGIN_CENTER<<16, 48<<16, }, -(190<<16) };
static MenuMenuFormat_t MMF_Top_Skill =            { {  MENU_MARGIN_CENTER<<16, 58<<16, }, -(190<<16) };
static MenuMenuFormat_t MMF_Top_Options =          { {  MENU_MARGIN_CENTER<<16, 38<<16, }, -(190<<16) };
static MenuMenuFormat_t MMF_Top_Joystick_Network = { {  MENU_MARGIN_CENTER<<16, 70<<16, }, -(190<<16) };
static MenuMenuFormat_t MMF_BigOptions =           { {    MENU_MARGIN_WIDE<<16, 38<<16, }, -(190<<16) };
static MenuMenuFormat_t MMF_SmallOptions =         { {    MENU_MARGIN_WIDE<<16, 37<<16, },    170<<16 };
static MenuMenuFormat_t MMF_Macros =               { {                  26<<16, 40<<16, },    160<<16 };
static MenuMenuFormat_t MMF_SmallOptionsNarrow  =  { { MENU_MARGIN_REGULAR<<16, 38<<16, }, -(190<<16) };
static MenuMenuFormat_t MMF_KeyboardSetupFuncs =   { {                  50<<16, 34<<16, },    151<<16 };
static MenuMenuFormat_t MMF_MouseJoySetupBtns =    { {                  76<<16, 34<<16, },    143<<16 };
static MenuMenuFormat_t MMF_FuncList =             { {                 100<<16, 51<<16, },    152<<16 };
static MenuMenuFormat_t MMF_ColorCorrect =         { { MENU_MARGIN_REGULAR<<16, 86<<16, },    190<<16 };
static MenuMenuFormat_t MMF_BigSliders =           { {    MENU_MARGIN_WIDE<<16, 37<<16, },    190<<16 };
static MenuMenuFormat_t MMF_LoadSave =             { {                 200<<16, 49<<16, },    145<<16 };
static MenuMenuFormat_t MMF_NetSetup =             { {                  36<<16, 38<<16, },    190<<16 };
static MenuMenuFormat_t MMF_FileSelectLeft =       { {                  40<<16, 45<<16, },    162<<16 };
static MenuMenuFormat_t MMF_FileSelectRight =      { {                 164<<16, 45<<16, },    162<<16 };

static MenuEntryFormat_t MEF_Null =             {     0,      0,          0 };
static MenuEntryFormat_t MEF_MainMenu =         { 4<<16,      0,          0 };
static MenuEntryFormat_t MEF_OptionsMenu =      { 7<<16,      0,          0 };
static MenuEntryFormat_t MEF_CenterMenu =       { 7<<16,      0,          0 };
static MenuEntryFormat_t MEF_BigOptions_Apply = { 4<<16, 16<<16, -(260<<16) };
static MenuEntryFormat_t MEF_BigOptionsRt =     { 4<<16,      0, -(260<<16) };
#if defined USE_OPENGL || !defined EDUKE32_ANDROID_MENU
static MenuEntryFormat_t MEF_SmallOptions =     { 1<<16,      0, -(260<<16) };
#endif
static MenuEntryFormat_t MEF_BigCheats =        { 3<<16,      0, -(260<<16) };
static MenuEntryFormat_t MEF_Cheats =           { 2<<16,      0, -(260<<16) };
static MenuEntryFormat_t MEF_PlayerNarrow =     { 1<<16,      0,     90<<16 };
static MenuEntryFormat_t MEF_Macros =           { 2<<16,     -1,    268<<16 };
static MenuEntryFormat_t MEF_VideoSetup =       { 4<<16,      0,    168<<16 };
static MenuEntryFormat_t MEF_VideoSetup_Apply = { 4<<16, 16<<16,    168<<16 };
static MenuEntryFormat_t MEF_KBFuncList =       { 3<<16,      0, -(225<<16) };
static MenuEntryFormat_t MEF_FuncList =         { 3<<16,      0, -(170<<16) };
static MenuEntryFormat_t MEF_ColorCorrect =     { 2<<16,      0, -(240<<16) };
static MenuEntryFormat_t MEF_BigSliders =       { 2<<16,      0, -(260<<16) };
static MenuEntryFormat_t MEF_LoadSave =         { 2<<16,     -1,     78<<16 };
static MenuEntryFormat_t MEF_NetSetup =         { 4<<16,      0,    112<<16 };
static MenuEntryFormat_t MEF_NetSetup_Confirm = { 4<<16, 16<<16,    112<<16 };

// common menu option sets
#define MAKE_MENUOPTIONSET(optionNames, optionValues, features) { optionNames, optionValues, &MMF_FuncList, &MEF_FuncList, &MF_Minifont, ARRAY_SIZE(optionNames), -1, 0, features }
#define MAKE_MENUOPTIONSETDYN(optionNames, optionValues, numOptions, features) { optionNames, optionValues, &MMF_FuncList, &MEF_FuncList, &MF_Minifont, numOptions, -1, 0, features }
#define MAKE_MENUOPTIONSETNULL { NULL, NULL, &MMF_FuncList, &MEF_FuncList, &MF_Minifont, 0, -1, 0, 0 }

static char const *MEOSN_OffOn[] = { "Off", "On", };
static MenuOptionSet_t MEOS_OffOn = MAKE_MENUOPTIONSET( MEOSN_OffOn, NULL, 0x3 );
static char const *MEOSN_OnOff[] = { "On", "Off", };
static MenuOptionSet_t MEOS_OnOff = MAKE_MENUOPTIONSET( MEOSN_OnOff, NULL, 0x3 );
static char const *MEOSN_NoYes[] = { "No", "Yes", };
static MenuOptionSet_t MEOS_NoYes = MAKE_MENUOPTIONSET( MEOSN_NoYes, NULL, 0x3 );
static char const *MEOSN_YesNo[] = { "Yes", "No", };
static MenuOptionSet_t MEOS_YesNo = MAKE_MENUOPTIONSET( MEOSN_YesNo, NULL, 0x3 );


static FString MenuGameFuncs[NUMGAMEFUNCTIONS];
static char const *MenuGameFuncNone = "  -None-";
static char const *MEOSN_Gamefuncs[NUMGAMEFUNCTIONS+1];
static int32_t MEOSV_Gamefuncs[NUMGAMEFUNCTIONS+1];
static MenuOptionSet_t MEOS_Gamefuncs = MAKE_MENUOPTIONSET( MEOSN_Gamefuncs, MEOSV_Gamefuncs, 0x1 );



/*
MenuEntry_t is passed in arrays of pointers so that the callback function
that is called when an entry is modified or activated can test equality of the current
entry pointer directly against the known ones, instead of relying on an ID number.

That way, individual menu entries can be ifdef'd out painlessly.
*/

static MenuLink_t MEO_NULL = { MENU_NULL, MA_None, };
static const char* MenuCustom = "Custom";

#define MAKE_MENUSTRING(...) { NULL, __VA_ARGS__, }
#define MAKE_MENUOPTION(...) { __VA_ARGS__, -1, }
#define MAKE_MENURANGE(...) { __VA_ARGS__, }
#define MAKE_MENUENTRY(...) { __VA_ARGS__, 0, 0, 0, }


#define MAKE_SPACER( EntryName, Height ) \
static MenuSpacer_t MEO_ ## EntryName = { Height };

MAKE_SPACER( Space2, 2<<16 ); // bigoptions
MAKE_SPACER( Space4, 4<<16 ); // usermap, smalloptions, anything else non-top
MAKE_SPACER( Space6, 6<<16 ); // videosetup
MAKE_SPACER( Space8, 8<<16 ); // colcorr, redslide

static MenuEntry_t ME_Space2_Redfont = MAKE_MENUENTRY( NULL, &MF_Redfont, &MEF_Null, &MEO_Space2, Spacer );
static MenuEntry_t ME_Space4_Bluefont = MAKE_MENUENTRY( NULL, &MF_Bluefont, &MEF_Null, &MEO_Space4, Spacer );
#ifndef EDUKE32_SIMPLE_MENU
static MenuEntry_t ME_Space4_Redfont = MAKE_MENUENTRY( NULL, &MF_Redfont, &MEF_Null, &MEO_Space4, Spacer );
static MenuEntry_t ME_Space8_Bluefont = MAKE_MENUENTRY( NULL, &MF_Bluefont, &MEF_Null, &MEO_Space8, Spacer );
#endif
static MenuEntry_t ME_Space6_Redfont = MAKE_MENUENTRY( NULL, &MF_Redfont, &MEF_Null, &MEO_Space6, Spacer );
static MenuEntry_t ME_Space8_Redfont = MAKE_MENUENTRY( NULL, &MF_Redfont, &MEF_Null, &MEO_Space8, Spacer );

#define MAKE_MENU_TOP_ENTRYLINK( Title, Format, EntryName, LinkID ) \
static MenuLink_t MEO_ ## EntryName = { LinkID, MA_Advance, };\
static MenuEntry_t ME_ ## EntryName = MAKE_MENUENTRY( Title, &MF_Redfont, &Format, &MEO_ ## EntryName, Link )

static char const s_NewGame[] = "New Game";
static char const s_SaveGame[] = "Save Game";
static char const s_LoadGame[] = "Load Game";
static char const s_Continue[] = "Continue";
static char const s_Options[] = "Options";
static char const s_Credits[] = "Credits";

MAKE_MENU_TOP_ENTRYLINK( s_NewGame, MEF_MainMenu, MAIN_NEWGAME, MENU_EPISODE );
#ifdef EDUKE32_SIMPLE_MENU
MAKE_MENU_TOP_ENTRYLINK( "Resume Game", MEF_MainMenu, MAIN_RESUMEGAME, MENU_CLOSE );
#endif
MAKE_MENU_TOP_ENTRYLINK( s_NewGame, MEF_MainMenu, MAIN_NEWGAME_INGAME, MENU_NEWVERIFY );
static MenuLink_t MEO_MAIN_NEWGAME_NETWORK = { MENU_NETWORK, MA_Advance, };
MAKE_MENU_TOP_ENTRYLINK( s_SaveGame, MEF_MainMenu, MAIN_SAVEGAME, MENU_SAVE );
MAKE_MENU_TOP_ENTRYLINK( s_LoadGame, MEF_MainMenu, MAIN_LOADGAME, MENU_LOAD );
MAKE_MENU_TOP_ENTRYLINK( s_Options, MEF_MainMenu, MAIN_OPTIONS, MENU_OPTIONS );
MAKE_MENU_TOP_ENTRYLINK("Help", MEF_MainMenu, MAIN_HELP, MENU_STORY);
#ifndef EDUKE32_SIMPLE_MENU
MAKE_MENU_TOP_ENTRYLINK( s_Credits, MEF_MainMenu, MAIN_CREDITS, MENU_CREDITS );
#endif
MAKE_MENU_TOP_ENTRYLINK( "End Game", MEF_MainMenu, MAIN_QUITTOTITLE, MENU_QUITTOTITLE );
MAKE_MENU_TOP_ENTRYLINK( "Quit", MEF_MainMenu, MAIN_QUIT, MENU_QUIT );
#ifndef EDUKE32_SIMPLE_MENU
MAKE_MENU_TOP_ENTRYLINK( "Quit Game", MEF_MainMenu, MAIN_QUITGAME, MENU_QUIT );
#endif

static MenuEntry_t *MEL_MAIN[] = {
    &ME_MAIN_NEWGAME,
    &ME_MAIN_LOADGAME,
    &ME_MAIN_OPTIONS,
    &ME_MAIN_HELP,
#ifndef EDUKE32_SIMPLE_MENU
    &ME_MAIN_CREDITS,
#endif
    &ME_MAIN_QUIT,
};

static MenuEntry_t *MEL_MAIN_INGAME[] = {
#ifdef EDUKE32_SIMPLE_MENU
    &ME_MAIN_RESUMEGAME,
#else
    &ME_MAIN_NEWGAME_INGAME,
#endif
    &ME_MAIN_SAVEGAME,
    &ME_MAIN_LOADGAME,
    &ME_MAIN_OPTIONS,
    &ME_MAIN_HELP,
    &ME_MAIN_QUITTOTITLE,
#ifndef EDUKE32_SIMPLE_MENU
    &ME_MAIN_QUITGAME,
#endif
};

// Episode and Skill will be dynamically generated after CONs are parsed
static MenuLink_t MEO_EPISODE = { MENU_SKILL, MA_Advance, };
static MenuLink_t MEO_EPISODE_SHAREWARE = { MENU_BUYDUKE, MA_Advance, };
static MenuEntry_t ME_EPISODE_TEMPLATE = MAKE_MENUENTRY( NULL, &MF_Redfont, &MEF_CenterMenu, &MEO_EPISODE, Link );
static MenuEntry_t ME_EPISODE[MAXVOLUMES];
static MenuLink_t MEO_EPISODE_USERMAP = { MENU_USERMAP, MA_Advance, };
static MenuEntry_t ME_EPISODE_USERMAP = MAKE_MENUENTRY( "User Map", &MF_Redfont, &MEF_CenterMenu, &MEO_EPISODE_USERMAP, Link );
static MenuEntry_t *MEL_EPISODE[MAXVOLUMES+2]; // +2 for spacer and User Map

static MenuEntry_t ME_SKILL_TEMPLATE = MAKE_MENUENTRY( NULL, &MF_Redfont, &MEF_CenterMenu, &MEO_NULL, Link );
static MenuEntry_t ME_SKILL[MAXSKILLS];
static MenuEntry_t *MEL_SKILL[MAXSKILLS];

static char const *MEOSN_GAMESETUP_AIM_AUTO[] = { "Never", "Always", "Hitscan only",
};
static int32_t MEOSV_GAMESETUP_AIM_AUTO[] = { 0, 1, 2,
};

static MenuOptionSet_t MEOS_GAMESETUP_AIM_AUTO = MAKE_MENUOPTIONSET( MEOSN_GAMESETUP_AIM_AUTO, MEOSV_GAMESETUP_AIM_AUTO, 0x2 );
static MenuOption_t MEO_GAMESETUP_AIM_AUTO = MAKE_MENUOPTION( &MF_Redfont, &MEOS_GAMESETUP_AIM_AUTO, &cl_autoaim );
static MenuEntry_t ME_GAMESETUP_AIM_AUTO = MAKE_MENUENTRY( "Auto aim:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_AIM_AUTO, Option );

static char const *MEOSN_GAMESETUP_WEAPSWITCH_PICKUP[] = { "Never", "If new", "By rating", };
static MenuOptionSet_t MEOS_GAMESETUP_WEAPSWITCH_PICKUP = MAKE_MENUOPTIONSET( MEOSN_GAMESETUP_WEAPSWITCH_PICKUP, NULL, 0x2 );
static MenuOption_t MEO_GAMESETUP_WEAPSWITCH_PICKUP = MAKE_MENUOPTION( &MF_Redfont, &MEOS_GAMESETUP_WEAPSWITCH_PICKUP, NULL );
static MenuEntry_t ME_GAMESETUP_WEAPSWITCH_PICKUP = MAKE_MENUENTRY( "Equip pickups:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_WEAPSWITCH_PICKUP, Option );

static char const *MEOSN_DemoRec[] = { "Off", "Running", };
static MenuOptionSet_t MEOS_DemoRec = MAKE_MENUOPTIONSET( MEOSN_DemoRec, NULL, 0x3 );
static MenuOption_t MEO_GAMESETUP_DEMOREC = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OffOn, &m_recstat );
static MenuEntry_t ME_GAMESETUP_DEMOREC = MAKE_MENUENTRY( "Record demo:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_DEMOREC, Option );

static MenuOption_t MEO_ADULTMODE = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &adult_lockout);
static MenuEntry_t ME_ADULTMODE = MAKE_MENUENTRY( "Parental lock:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_ADULTMODE, Option );

#if defined(EDUKE32_ANDROID_MENU) || !defined(EDUKE32_SIMPLE_MENU)
static MenuLink_t MEO_GAMESETUP_CHEATS = { MENU_CHEATS, MA_Advance, };
static MenuEntry_t ME_GAMESETUP_CHEATS = MAKE_MENUENTRY( "Cheats", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_CHEATS, Link );

static MenuEntry_t *MEL_GAMESETUP[] = {
    &ME_ADULTMODE,
#if defined STARTUP_SETUP_WINDOW && !defined EDUKE32_SIMPLE_MENU
    &ME_GAMESETUP_STARTWIN,
#endif
    &ME_GAMESETUP_AIM_AUTO,
    //&ME_GAMESETUP_WEAPSWITCH_PICKUP,
#ifdef EDUKE32_ANDROID_MENU
    &ME_GAMESETUP_QUICKSWITCH,
    &ME_GAMESETUP_CROUCHLOCK,
#else
    &ME_GAMESETUP_DEMOREC,
#endif
    &ME_GAMESETUP_CHEATS,
};
#endif


// Zhe menu code lacks flexibility, it can either be hardwired to ints or to CVARs.
// Since CVARs are more important it means that these need to be implemented as CVARs even though they are just temporaries.
// By giving them no name we ensure that they remain outside the CVAR system.
CVAR_UNAMED(Int, newresolution)
CVAR_UNAMED(Int, newrendermode)
CVAR_UNAMED(Int, newfullscreen)
CVAR_UNAMED(Int, newvsync)
CVAR_UNAMED(Int, newborderless)

enum resflags_t {
    RES_FS  = 0x1,
    RES_WIN = 0x2,
};

#define MAXRESOLUTIONSTRINGLENGTH 19

typedef struct resolution_t {
    int32_t xdim, ydim;
    int32_t flags;
    int32_t bppmax;
    char name[MAXRESOLUTIONSTRINGLENGTH];
} resolution_t;

resolution_t resolution[MAXVALIDMODES];

static char const *MEOSN_VIDEOSETUP_RESOLUTION[MAXVALIDMODES];
static MenuOptionSet_t MEOS_VIDEOSETUP_RESOLUTION = MAKE_MENUOPTIONSETDYN( MEOSN_VIDEOSETUP_RESOLUTION, NULL, 0, 0x0 );
static MenuOption_t MEO_VIDEOSETUP_RESOLUTION = MAKE_MENUOPTION( &MF_Redfont, &MEOS_VIDEOSETUP_RESOLUTION, &newresolution );
static MenuEntry_t ME_VIDEOSETUP_RESOLUTION = MAKE_MENUENTRY( "Resolution:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_RESOLUTION, Option );

#ifdef USE_OPENGL
static char const *MEOSN_VIDEOSETUP_RENDERER[] = { "Classic", "OpenGL", };
static int32_t MEOSV_VIDEOSETUP_RENDERER[] = { REND_CLASSIC, REND_POLYMOST, };

static MenuOptionSet_t MEOS_VIDEOSETUP_RENDERER = MAKE_MENUOPTIONSET( MEOSN_VIDEOSETUP_RENDERER, MEOSV_VIDEOSETUP_RENDERER, 0x2 );

static MenuOption_t MEO_VIDEOSETUP_RENDERER = MAKE_MENUOPTION( &MF_Redfont, &MEOS_VIDEOSETUP_RENDERER, &newrendermode );
static MenuEntry_t ME_VIDEOSETUP_RENDERER = MAKE_MENUENTRY( "Renderer:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_RENDERER, Option );
#endif

static MenuOption_t MEO_VIDEOSETUP_FULLSCREEN = MAKE_MENUOPTION( &MF_Redfont, &MEOS_YesNo, &newfullscreen );
static MenuEntry_t ME_VIDEOSETUP_FULLSCREEN = MAKE_MENUENTRY( "Windowed:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_FULLSCREEN, Option );


static char const *MEOSN_VIDEOSETUP_VSYNC [] = { "Adaptive", "Off", "On", };
static int32_t MEOSV_VIDEOSETUP_VSYNC [] = { -1, 0, 1, };
static MenuOptionSet_t MEOS_VIDEOSETUP_VSYNC = MAKE_MENUOPTIONSET(MEOSN_VIDEOSETUP_VSYNC, MEOSV_VIDEOSETUP_VSYNC, 0x2);
static MenuOption_t MEO_VIDEOSETUP_VSYNC = MAKE_MENUOPTION(&MF_Redfont, &MEOS_VIDEOSETUP_VSYNC, &newvsync);
static MenuEntry_t ME_VIDEOSETUP_VSYNC = MAKE_MENUENTRY("VSync:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_VSYNC, Option);

//static char const *MEOSN_VIDEOSETUP_FRAMELIMIT [] = { "30 fps", "60 fps", "75 fps", "100 fps", "120 fps", "144 fps", "165 fps", "240 fps" };

static MenuEntry_t ME_VIDEOSETUP_APPLY = MAKE_MENUENTRY( "Apply Changes", &MF_Redfont, &MEF_BigOptions_Apply, &MEO_NULL, Link );


static MenuLink_t MEO_DISPLAYSETUP_COLORCORR = { MENU_COLCORR, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_COLORCORR = MAKE_MENUENTRY( "Color Correction", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_COLORCORR, Link );


#ifndef EDUKE32_ANDROID_MENU
static MenuOption_t MEO_DISPLAYSETUP_ASPECTRATIO = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &r_usenewaspect);
static MenuEntry_t ME_DISPLAYSETUP_ASPECTRATIO = MAKE_MENUENTRY( "Widescreen:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_ASPECTRATIO, Option );
#endif

static MenuOption_t MEO_DISPLAYSETUP_VOXELS = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &r_voxels);
static MenuEntry_t ME_DISPLAYSETUP_VOXELS = MAKE_MENUENTRY( "Voxels:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_VOXELS, Option );

static MenuRangeInt32_t MEO_DISPLAYSETUP_FOV = MAKE_MENURANGE( &r_fov, &MF_Redfont, 70, 120, 0, 11, 1 );
static MenuEntry_t ME_DISPLAYSETUP_FOV = MAKE_MENUENTRY( "FOV:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_FOV, RangeInt32 );


#ifdef USE_OPENGL

//POGOTODO: allow filtering again in standalone once indexed colour textures support filtering
static char const *MEOSN_DISPLAYSETUP_TEXFILTER[] = { "Classic", "Filtered" };
static int32_t MEOSV_DISPLAYSETUP_TEXFILTER[] = { TEXFILTER_OFF, TEXFILTER_ON };
static MenuOptionSet_t MEOS_DISPLAYSETUP_TEXFILTER = MAKE_MENUOPTIONSET( MEOSN_DISPLAYSETUP_TEXFILTER, MEOSV_DISPLAYSETUP_TEXFILTER, 0x2 );
static MenuOption_t MEO_DISPLAYSETUP_TEXFILTER = MAKE_MENUOPTION( &MF_Redfont, &MEOS_DISPLAYSETUP_TEXFILTER, &hw_texfilter );
static MenuEntry_t ME_DISPLAYSETUP_TEXFILTER = MAKE_MENUENTRY( "Texture Mode:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_TEXFILTER, Option );

static char const *MEOSN_DISPLAYSETUP_ANISOTROPY[] = { "Max", "None", "2x", "4x", "8x", "16x", };
static int32_t MEOSV_DISPLAYSETUP_ANISOTROPY[] = { 0, 1, 2, 4, 8, 16, };
static MenuOptionSet_t MEOS_DISPLAYSETUP_ANISOTROPY = MAKE_MENUOPTIONSET( MEOSN_DISPLAYSETUP_ANISOTROPY, MEOSV_DISPLAYSETUP_ANISOTROPY, 0x0 );
static MenuOption_t MEO_DISPLAYSETUP_ANISOTROPY = MAKE_MENUOPTION(&MF_Redfont, &MEOS_DISPLAYSETUP_ANISOTROPY, &hw_anisotropy);
static MenuEntry_t ME_DISPLAYSETUP_ANISOTROPY = MAKE_MENUENTRY( "Anisotropy:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_ANISOTROPY, Option );

#endif

static char const s_Scale[] = "Scale:";

static MenuOption_t MEO_SCREENSETUP_CROSSHAIR = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &cl_crosshair);
static MenuEntry_t ME_SCREENSETUP_CROSSHAIR = MAKE_MENUENTRY( "Crosshair:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SCREENSETUP_CROSSHAIR, Option );
static MenuRangeInt32_t MEO_SCREENSETUP_CROSSHAIRSIZE = MAKE_MENURANGE( &cl_crosshairscale, &MF_Redfont, 25, 100, 0, 16, 2 );
static MenuEntry_t ME_SCREENSETUP_CROSSHAIRSIZE = MAKE_MENUENTRY( s_Scale, &MF_Redfont, &MEF_BigOptions_Apply, &MEO_SCREENSETUP_CROSSHAIRSIZE, RangeInt32 );

static MenuRangeInt32_t MEO_SCREENSETUP_SCREENSIZE = MAKE_MENURANGE( &hud_size, &MF_Redfont, 0, 11, 0, 1, EnforceIntervals );
static MenuOption_t MEO_SCREENSETUP_SCREENSIZE_TWO = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OffOn, &hud_size );
static MenuEntry_t ME_SCREENSETUP_SCREENSIZE = MAKE_MENUENTRY( "Status bar:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SCREENSETUP_SCREENSIZE, RangeInt32 );
static MenuRangeInt32_t MEO_SCREENSETUP_TEXTSIZE = MAKE_MENURANGE( &hud_textscale, &MF_Redfont, 100, 400, 0, 16, 2 );
static MenuEntry_t ME_SCREENSETUP_TEXTSIZE = MAKE_MENUENTRY( s_Scale, &MF_Redfont, &MEF_BigOptions_Apply, &MEO_SCREENSETUP_TEXTSIZE, RangeInt32 );
static MenuOption_t MEO_SCREENSETUP_LEVELSTATS = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &hud_stats);
static MenuEntry_t ME_SCREENSETUP_LEVELSTATS = MAKE_MENUENTRY( "Level stats:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SCREENSETUP_LEVELSTATS, Option );


static MenuOption_t MEO_SCREENSETUP_SHOWPICKUPMESSAGES = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &hud_messages);
static MenuEntry_t ME_SCREENSETUP_SHOWPICKUPMESSAGES = MAKE_MENUENTRY( "Game messages:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SCREENSETUP_SHOWPICKUPMESSAGES, Option );

static MenuRangeInt32_t MEO_SCREENSETUP_SBARSIZE = MAKE_MENURANGE( &hud_scale, &MF_Redfont, 50, 100, 0, 10, 2 );
static MenuEntry_t ME_SCREENSETUP_SBARSIZE = MAKE_MENUENTRY( s_Scale, &MF_Redfont, &MEF_BigOptions_Apply, &MEO_SCREENSETUP_SBARSIZE, RangeInt32 );


static MenuLink_t MEO_DISPLAYSETUP_SCREENSETUP = { MENU_SCREENSETUP, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_SCREENSETUP = MAKE_MENUENTRY( "HUD setup", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_SCREENSETUP, Link );


#ifdef USE_OPENGL
static MenuLink_t MEO_DISPLAYSETUP_ADVANCED_GL_POLYMOST = { MENU_POLYMOST, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_ADVANCED_GL_POLYMOST = MAKE_MENUENTRY( "Polymost setup", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_ADVANCED_GL_POLYMOST, Link );
#endif

#ifndef EDUKE32_ANDROID_MENU
static MenuLink_t MEO_DISPLAYSETUP_VIDEOSETUP = { MENU_VIDEOSETUP, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_VIDEOSETUP = MAKE_MENUENTRY( "Video mode", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_VIDEOSETUP, Link );
#endif


static MenuLink_t MEO_ENTERCHEAT = { MENU_CHEATENTRY, MA_None, };
static MenuEntry_t ME_ENTERCHEAT = MAKE_MENUENTRY( "Enter Cheat Code", &MF_Redfont, &MEF_BigCheats, &MEO_ENTERCHEAT, Link );

static MenuLink_t MEO_CHEAT_WARP = { MENU_CHEAT_WARP, MA_None, };
static MenuLink_t MEO_CHEAT_SKILL = { MENU_CHEAT_SKILL, MA_None, };

static MenuEntry_t *MEL_OPTIONS[] = {
#ifndef EDUKE32_SIMPLE_MENU
    &ME_OPTIONS_GAMESETUP,
#endif
    &ME_OPTIONS_DISPLAYSETUP,
    &ME_OPTIONS_SOUNDSETUP,
#ifndef EDUKE32_ANDROID_MENU
#ifndef EDUKE32_SIMPLE_MENU
    &ME_OPTIONS_PLAYERSETUP,
#endif
    &ME_OPTIONS_CONTROLS,
#else
    &ME_OPTIONS_TOUCHSETUP,
#endif
#ifdef EDUKE32_SIMPLE_MENU
    &ME_GAMESETUP_SAVESETUP,
    &ME_OPTIONS_CHEATS
#endif
};

static MenuEntry_t *MEL_CONTROLS[] = {
#ifndef EDUKE32_ANDROID_MENU
    &ME_OPTIONS_KEYBOARDSETUP,
    &ME_OPTIONS_MOUSESETUP,
    &ME_OPTIONS_JOYSTICKSETUP,
#else
    &ME_OPTIONS_TOUCHSETUP
#endif
};

static MenuEntry_t *MEL_CHEATS[ARRAY_SIZE(ME_CheatCodes)+1] = {
    &ME_ENTERCHEAT,
};

static MenuEntry_t *MEL_VIDEOSETUP[] = {
    &ME_VIDEOSETUP_RESOLUTION,
#ifdef USE_OPENGL
    &ME_VIDEOSETUP_RENDERER,
#endif
    &ME_VIDEOSETUP_FULLSCREEN,
    &ME_VIDEOSETUP_VSYNC,
    &ME_Space6_Redfont,
    &ME_VIDEOSETUP_APPLY,
};
static MenuEntry_t *MEL_DISPLAYSETUP[] = {
    &ME_DISPLAYSETUP_SCREENSETUP,
    &ME_DISPLAYSETUP_COLORCORR,
#ifndef EDUKE32_ANDROID_MENU
    &ME_DISPLAYSETUP_VIDEOSETUP,
    &ME_DISPLAYSETUP_ASPECTRATIO,
    &ME_DISPLAYSETUP_VOXELS,
    &ME_DISPLAYSETUP_FOV,
#endif
};

#ifdef USE_OPENGL
static MenuEntry_t *MEL_DISPLAYSETUP_GL[] = {
    &ME_DISPLAYSETUP_SCREENSETUP,
    &ME_DISPLAYSETUP_COLORCORR,
    &ME_DISPLAYSETUP_VIDEOSETUP,
    &ME_DISPLAYSETUP_ASPECTRATIO,
    &ME_DISPLAYSETUP_VOXELS,
    &ME_DISPLAYSETUP_FOV,
    &ME_DISPLAYSETUP_TEXFILTER,
    &ME_DISPLAYSETUP_ANISOTROPY,
    &ME_DISPLAYSETUP_ADVANCED_GL_POLYMOST,
};


#endif



static char const *MenuKeyNone = "  -";
static char const *MEOSN_Keys[NUMKEYS];

static MenuCustom2Col_t MEO_KEYBOARDSETUPFUNCS_TEMPLATE = { { NULL, NULL, }, MEOSN_Keys, &MF_Minifont, NUMKEYS, 54<<16, 0 };
static MenuCustom2Col_t MEO_KEYBOARDSETUPFUNCS[NUMGAMEFUNCTIONS];
static MenuEntry_t ME_KEYBOARDSETUPFUNCS_TEMPLATE = MAKE_MENUENTRY( NULL, &MF_Minifont, &MEF_KBFuncList, &MEO_KEYBOARDSETUPFUNCS_TEMPLATE, Custom2Col );
static MenuEntry_t ME_KEYBOARDSETUPFUNCS[NUMGAMEFUNCTIONS];
static MenuEntry_t *MEL_KEYBOARDSETUPFUNCS[NUMGAMEFUNCTIONS];

static MenuLink_t MEO_KEYBOARDSETUP_KEYS = { MENU_KEYBOARDKEYS, MA_Advance, };
static MenuEntry_t ME_KEYBOARDSETUP_KEYS = MAKE_MENUENTRY( "Edit Configuration", &MF_Redfont, &MEF_CenterMenu, &MEO_KEYBOARDSETUP_KEYS, Link );
static MenuEntry_t ME_KEYBOARDSETUP_RESET = MAKE_MENUENTRY( "Reset To Defaults", &MF_Redfont, &MEF_CenterMenu, &MEO_NULL, Link );
static MenuEntry_t ME_KEYBOARDSETUP_RESETCLASSIC = MAKE_MENUENTRY( "Reset To Classic", &MF_Redfont, &MEF_CenterMenu, &MEO_NULL, Link );

static MenuEntry_t *MEL_KEYBOARDSETUP[] = {
    &ME_KEYBOARDSETUP_KEYS,
    &ME_KEYBOARDSETUP_RESET,
    &ME_KEYBOARDSETUP_RESETCLASSIC,
};


// There is no better way to do this than manually.

#define MENUMOUSEFUNCTIONS 12

static char const *MenuMouseNames[MENUMOUSEFUNCTIONS] = {
    "Button 1",
    "Double Button 1",
    "Button 2",
    "Double Button 2",
    "Button 3",
    "Double Button 3",

    "Wheel Up",
    "Wheel Down",

    "Button 4",
    "Double Button 4",
    "Button 5",
    "Double Button 5",
};
static int32_t MenuMouseDataIndex[MENUMOUSEFUNCTIONS][2] = {
    { 0, 0, },
    { 0, 1, },
    { 1, 0, },
    { 1, 1, },
    { 2, 0, },
    { 2, 1, },

    // note the mouse wheel
    { 4, 0, },
    { 5, 0, },

    { 3, 0, },
    { 3, 1, },
    { 6, 0, },
    { 6, 1, },
};

static MenuOption_t MEO_MOUSEJOYSETUPBTNS_TEMPLATE = MAKE_MENUOPTION( &MF_Minifont, &MEOS_Gamefuncs, NULL );
static MenuOption_t MEO_MOUSESETUPBTNS[MENUMOUSEFUNCTIONS];
static MenuEntry_t ME_MOUSEJOYSETUPBTNS_TEMPLATE = MAKE_MENUENTRY( NULL, &MF_Minifont, &MEF_FuncList, NULL, Option );

static MenuRangeFloat_t MEO_MOUSESETUP_SENSITIVITY = MAKE_MENURANGE( &in_mousesensitivity, &MF_Redfont, .5f, 16.f, 0.f, 32, 1 );
static MenuEntry_t ME_MOUSESETUP_SENSITIVITY = MAKE_MENUENTRY( "Sensitivity:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_SENSITIVITY, RangeFloat );

#ifndef EDUKE32_SIMPLE_MENU
static char const *MEOSN_MOUSESETUP_AIM_TYPE [] = { "Toggle", "Hold" };
static MenuOptionSet_t MEOS_MOUSESETUP_AIM_TYPE = MAKE_MENUOPTIONSET(MEOSN_MOUSESETUP_AIM_TYPE, NULL, 0x2);
static MenuOption_t MEO_MOUSESETUP_MOUSEAIMINGTYPE = MAKE_MENUOPTION(&MF_Redfont, &MEOS_MOUSESETUP_AIM_TYPE, &in_aimmode);
static MenuEntry_t ME_MOUSESETUP_MOUSEAIMINGTYPE = MAKE_MENUENTRY("Aiming type:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_MOUSEAIMINGTYPE, Option);
static MenuOption_t MEO_MOUSESETUP_MOUSEAIMING = MAKE_MENUOPTION( &MF_Redfont, &MEOS_NoYes, &in_mousemode );
static MenuEntry_t ME_MOUSESETUP_MOUSEAIMING = MAKE_MENUENTRY( "Vertical aiming:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_MOUSEAIMING, Option );
#endif
static MenuOption_t MEO_MOUSESETUP_INVERT = MAKE_MENUOPTION( &MF_Redfont, &MEOS_YesNo, &in_mouseflip );
static MenuEntry_t ME_MOUSESETUP_INVERT = MAKE_MENUENTRY( "Invert aiming:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_INVERT, Option );
static MenuOption_t MEO_MOUSESETUP_SMOOTH = MAKE_MENUOPTION( &MF_Redfont, &MEOS_NoYes, &in_mousesmoothing );
static MenuEntry_t ME_MOUSESETUP_SMOOTH = MAKE_MENUENTRY( "Filter input:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_SMOOTH, Option );
#ifndef EDUKE32_SIMPLE_MENU
static MenuLink_t MEO_MOUSESETUP_ADVANCED = { MENU_MOUSEADVANCED, MA_Advance, };
static MenuEntry_t ME_MOUSESETUP_ADVANCED = MAKE_MENUENTRY( "Advanced setup", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_ADVANCED, Link );
#endif
static MenuRangeInt32_t MEO_MOUSEADVANCED_SCALEX = MAKE_MENURANGE(&in_mousescalex, &MF_Redfont, -262144, 262144, 65536, 161, 3);
static MenuEntry_t      ME_MOUSEADVANCED_SCALEX  = MAKE_MENUENTRY("X-Scale:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSEADVANCED_SCALEX, RangeInt32);
static MenuRangeInt32_t MEO_MOUSEADVANCED_SCALEY = MAKE_MENURANGE(&in_mousescaley, &MF_Redfont, -262144, 262144, 65536, 161, 3);
static MenuEntry_t      ME_MOUSEADVANCED_SCALEY  = MAKE_MENUENTRY("Y-Scale:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSEADVANCED_SCALEY, RangeInt32);

static MenuEntry_t *MEL_MOUSESETUP[] = {
    &ME_MOUSESETUP_SENSITIVITY,
#ifdef EDUKE32_SIMPLE_MENU
    &ME_MOUSEADVANCED_SCALEX,
    &ME_MOUSEADVANCED_SCALEY,
#endif
    &ME_Space2_Redfont,
    &ME_MOUSESETUP_INVERT,
    &ME_MOUSESETUP_SMOOTH,
#ifndef EDUKE32_SIMPLE_MENU
    &ME_MOUSESETUP_MOUSEAIMINGTYPE,
    &ME_MOUSESETUP_MOUSEAIMING,
    &ME_MOUSESETUP_ADVANCED,
#endif
};



static MenuOption_t MEO_JOYSTICK_ENABLE = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OffOn, &in_joystick );
static MenuEntry_t ME_JOYSTICK_ENABLE = MAKE_MENUENTRY( "Enable Gamepad:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_JOYSTICK_ENABLE, Option );

MAKE_MENU_TOP_ENTRYLINK( "Edit Buttons", MEF_CenterMenu, JOYSTICK_EDITBUTTONS, MENU_JOYSTICKBTNS );
MAKE_MENU_TOP_ENTRYLINK( "Edit Axes", MEF_CenterMenu, JOYSTICK_EDITAXES, MENU_JOYSTICKAXES );

static MenuEntry_t *MEL_JOYSTICKSETUP[] = {
    &ME_JOYSTICK_ENABLE,
    &ME_Space6_Redfont,
    &ME_JOYSTICK_EDITBUTTONS,
    &ME_JOYSTICK_EDITAXES,
};

#define MAXJOYBUTTONSTRINGLENGTH 32

static char MenuJoystickNames[MAXJOYBUTTONSANDHATS*2][MAXJOYBUTTONSTRINGLENGTH];

static MenuOption_t MEO_JOYSTICKBTNS[MAXJOYBUTTONSANDHATS*2];
static MenuEntry_t ME_JOYSTICKBTNS[MAXJOYBUTTONSANDHATS*2];
static MenuEntry_t *MEL_JOYSTICKBTNS[MAXJOYBUTTONSANDHATS*2];

static MenuLink_t MEO_JOYSTICKAXES = { MENU_JOYSTICKAXIS, MA_Advance, };
static MenuEntry_t ME_JOYSTICKAXES_TEMPLATE = MAKE_MENUENTRY( NULL, &MF_Redfont, &MEF_BigSliders, &MEO_JOYSTICKAXES, Link );
static MenuEntry_t ME_JOYSTICKAXES[MAXJOYAXES];
static char MenuJoystickAxes[MAXJOYAXES][MAXJOYBUTTONSTRINGLENGTH];

static MenuEntry_t *MEL_JOYSTICKAXES[MAXJOYAXES];

static MenuEntry_t *MEL_MOUSEADVANCED[] = {
    &ME_MOUSEADVANCED_SCALEX,
    &ME_MOUSEADVANCED_SCALEY,
};

static const char *MenuJoystickHatDirections[] = { "Up", "Right", "Down", "Left", };

static char const *MEOSN_JOYSTICKAXIS_ANALOG[] = { "  -None-", "Turning", "Strafing", "Looking", "Moving", };
static int32_t MEOSV_JOYSTICKAXIS_ANALOG[] = { -1, analog_turning, analog_strafing, analog_lookingupanddown, analog_moving, };
static MenuOptionSet_t MEOS_JOYSTICKAXIS_ANALOG = MAKE_MENUOPTIONSET( MEOSN_JOYSTICKAXIS_ANALOG, MEOSV_JOYSTICKAXIS_ANALOG, 0x0 );
static MenuOption_t MEO_JOYSTICKAXIS_ANALOG = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_JOYSTICKAXIS_ANALOG, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_ANALOG = MAKE_MENUENTRY( "Analog", &MF_Redfont, &MEF_BigSliders, &MEO_JOYSTICKAXIS_ANALOG, Option );
static MenuRangeInt32_t MEO_JOYSTICKAXIS_SCALE = MAKE_MENURANGE( NULL, &MF_Bluefont, -262144, 262144, 65536, 65, 3 );
static MenuEntry_t ME_JOYSTICKAXIS_SCALE = MAKE_MENUENTRY( "Scale", &MF_Redfont, &MEF_BigSliders, &MEO_JOYSTICKAXIS_SCALE, RangeInt32 );
static MenuRangeInt32_t MEO_JOYSTICKAXIS_DEAD = MAKE_MENURANGE( NULL, &MF_Bluefont, 0, 1000000, 0, 33, 2 );
static MenuEntry_t ME_JOYSTICKAXIS_DEAD = MAKE_MENUENTRY( "Dead Zone", &MF_Redfont, &MEF_BigSliders, &MEO_JOYSTICKAXIS_DEAD, RangeInt32 );
static MenuRangeInt32_t MEO_JOYSTICKAXIS_SATU = MAKE_MENURANGE( NULL, &MF_Bluefont, 0, 1000000, 0, 33, 2 );
static MenuEntry_t ME_JOYSTICKAXIS_SATU = MAKE_MENUENTRY( "Saturation", &MF_Redfont, &MEF_BigSliders, &MEO_JOYSTICKAXIS_SATU, RangeInt32 );

static MenuOption_t MEO_JOYSTICKAXIS_DIGITALNEGATIVE = MAKE_MENUOPTION( &MF_Minifont, &MEOS_Gamefuncs, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_DIGITALNEGATIVE = MAKE_MENUENTRY( "Digital -", &MF_Bluefont, &MEF_BigSliders, &MEO_JOYSTICKAXIS_DIGITALNEGATIVE, Option );
static MenuOption_t MEO_JOYSTICKAXIS_DIGITALPOSITIVE = MAKE_MENUOPTION( &MF_Minifont, &MEOS_Gamefuncs, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_DIGITALPOSITIVE = MAKE_MENUENTRY( "Digital +", &MF_Bluefont, &MEF_BigSliders, &MEO_JOYSTICKAXIS_DIGITALPOSITIVE, Option );

static MenuEntry_t *MEL_JOYSTICKAXIS[] = {
    &ME_JOYSTICKAXIS_ANALOG,
    &ME_JOYSTICKAXIS_SCALE,
    &ME_JOYSTICKAXIS_DEAD,
    &ME_JOYSTICKAXIS_SATU,
    &ME_Space8_Redfont,
    &ME_JOYSTICKAXIS_DIGITALNEGATIVE,
    &ME_JOYSTICKAXIS_DIGITALPOSITIVE,
};

static MenuEntry_t *MEL_INTERNAL_JOYSTICKAXIS_DIGITAL[] = {
    &ME_JOYSTICKAXIS_DIGITALNEGATIVE,
    &ME_JOYSTICKAXIS_DIGITALPOSITIVE,
};

#ifdef USE_OPENGL
static MenuOption_t MEO_RENDERERSETUP_HIGHTILE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &hw_hightile );
static MenuEntry_t ME_RENDERERSETUP_HIGHTILE = MAKE_MENUENTRY( "True color textures:", &MF_Bluefont, &MEF_SmallOptions, &MEO_RENDERERSETUP_HIGHTILE, Option );

static MenuOption_t MEO_RENDERERSETUP_PRECACHE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &r_precache );
static MenuEntry_t ME_RENDERERSETUP_PRECACHE = MAKE_MENUENTRY( "Pre-load map textures:", &MF_Bluefont, &MEF_SmallOptions, &MEO_RENDERERSETUP_PRECACHE, Option );
# ifndef EDUKE32_GLES
static char const *MEOSN_RENDERERSETUP_TEXCACHE[] = { "Off", "On", "Compr.", };
static MenuOptionSet_t MEOS_RENDERERSETUP_TEXCACHE = MAKE_MENUOPTIONSET( MEOSN_RENDERERSETUP_TEXCACHE, NULL, 0x2 );
# endif
# ifdef USE_GLEXT
static MenuOption_t MEO_RENDERERSETUP_DETAILTEX = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &hw_detailmapping);
static MenuEntry_t ME_RENDERERSETUP_DETAILTEX = MAKE_MENUENTRY( "Detail textures:", &MF_Bluefont, &MEF_SmallOptions, &MEO_RENDERERSETUP_DETAILTEX, Option );
static MenuOption_t MEO_RENDERERSETUP_GLOWTEX = MAKE_MENUOPTION(&MF_Bluefont, &MEOS_NoYes, &hw_glowmapping);
static MenuEntry_t ME_RENDERERSETUP_GLOWTEX = MAKE_MENUENTRY("Glow textures:", &MF_Bluefont, &MEF_SmallOptions, &MEO_RENDERERSETUP_GLOWTEX, Option);
# endif
static MenuOption_t MEO_RENDERERSETUP_MODELS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &hw_models );
static MenuEntry_t ME_RENDERERSETUP_MODELS = MAKE_MENUENTRY( "3D models:", &MF_Bluefont, &MEF_SmallOptions, &MEO_RENDERERSETUP_MODELS, Option );
#endif


#ifdef USE_OPENGL
static MenuEntry_t *MEL_RENDERERSETUP_POLYMOST[] = {
    &ME_RENDERERSETUP_HIGHTILE,
    &ME_RENDERERSETUP_PRECACHE,
# ifdef USE_GLEXT
    &ME_RENDERERSETUP_DETAILTEX,
    &ME_RENDERERSETUP_GLOWTEX,
# endif
    &ME_Space4_Bluefont,
    &ME_RENDERERSETUP_MODELS,
};

#endif

#ifdef EDUKE32_ANDROID_MENU
static MenuRangeFloat_t MEO_COLCORR_GAMMA = MAKE_MENURANGE( &vid_gamma, &MF_Bluefont, 1.f, 2.5f, 0.f, 39, 1 );
#else
static MenuRangeFloat_t MEO_COLCORR_GAMMA = MAKE_MENURANGE( &vid_gamma, &MF_Bluefont, 0.3f, 4.f, 0.f, 38, 1 );
#endif
static MenuEntry_t ME_COLCORR_GAMMA = MAKE_MENUENTRY( "Gamma:", &MF_Redfont, &MEF_ColorCorrect, &MEO_COLCORR_GAMMA, RangeFloat );
static MenuRangeFloat_t MEO_COLCORR_CONTRAST = MAKE_MENURANGE( &vid_contrast, &MF_Bluefont, 0.1f, 2.7f, 0.f, 53, 1 );
static MenuEntry_t ME_COLCORR_CONTRAST = MAKE_MENUENTRY( "Contrast:", &MF_Redfont, &MEF_ColorCorrect, &MEO_COLCORR_CONTRAST, RangeFloat );
static MenuRangeFloat_t MEO_COLCORR_BRIGHTNESS = MAKE_MENURANGE( &vid_brightness, &MF_Bluefont, -0.8f, 0.8f, 0.f, 33, 1 );
static MenuEntry_t ME_COLCORR_BRIGHTNESS = MAKE_MENUENTRY( "Brightness:", &MF_Redfont, &MEF_ColorCorrect, &MEO_COLCORR_BRIGHTNESS, RangeFloat );
static MenuEntry_t ME_COLCORR_RESET = MAKE_MENUENTRY( "Reset To Defaults", &MF_Redfont, &MEF_ColorCorrect, &MEO_NULL, Link );
#ifdef EDUKE32_ANDROID_MENU
#define MINVIS 1.f
#else
#define MINVIS 0.125f
#endif
#ifndef EDUKE32_SIMPLE_MENU
static MenuRangeFloat_t MEO_COLCORR_AMBIENT = MAKE_MENURANGE( &r_ambientlight, &MF_Bluefont, MINVIS, 4.f, 0.f, 32, 1 );
static MenuEntry_t ME_COLCORR_AMBIENT = MAKE_MENUENTRY( "Visibility:", &MF_Redfont, &MEF_ColorCorrect, &MEO_COLCORR_AMBIENT, RangeFloat );
#endif
static MenuEntry_t *MEL_COLCORR[] = {
    &ME_COLCORR_GAMMA,
#ifndef EDUKE32_ANDROID_MENU
    &ME_COLCORR_CONTRAST,
    &ME_COLCORR_BRIGHTNESS,
#endif
#ifndef EDUKE32_SIMPLE_MENU
    &ME_COLCORR_AMBIENT,
#endif
    &ME_Space8_Redfont,
    &ME_COLCORR_RESET,
};

static MenuEntry_t *MEL_SCREENSETUP[] = {
#ifdef EDUKE32_ANDROID_MENU
    &ME_SCREENSETUP_STATUSBARONTOP,
#endif
    &ME_SCREENSETUP_SCREENSIZE,
    &ME_SCREENSETUP_SBARSIZE,

    &ME_SCREENSETUP_CROSSHAIR,
    &ME_SCREENSETUP_CROSSHAIRSIZE,

    &ME_SCREENSETUP_LEVELSTATS,
    &ME_SCREENSETUP_TEXTSIZE,

    &ME_SCREENSETUP_SHOWPICKUPMESSAGES,
};



CVAR_UNAMED(Int, soundrate)
CVAR_UNAMED(Int, soundvoices)
CVAR_UNAMED(Int, musicdevice)
static MenuOption_t MEO_SOUND = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OffOn, &snd_enabled );
static MenuEntry_t ME_SOUND = MAKE_MENUENTRY( "Sound:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND, Option );

static MenuOption_t MEO_SOUND_MUSIC = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OffOn, &mus_enabled );
static MenuEntry_t ME_SOUND_MUSIC = MAKE_MENUENTRY( "Music:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_MUSIC, Option );

static MenuLink_t MEO_SOUND_CDPLAYER = { MENU_CDPLAYER, MA_Advance, };
static MenuEntry_t ME_SOUND_CDPLAYER = MAKE_MENUENTRY( "8 Track Player", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_CDPLAYER, Link );

static MenuRangeInt32_t MEO_SOUND_VOLUME_FX = MAKE_MENURANGE( &snd_fxvolume, &MF_Redfont, 0, 255, 0, 33, 2 );
static MenuEntry_t ME_SOUND_VOLUME_FX = MAKE_MENUENTRY( "Volume:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_VOLUME_FX, RangeInt32 );

static MenuRangeInt32_t MEO_SOUND_VOLUME_MUSIC = MAKE_MENURANGE( &mus_volume, &MF_Redfont, 0, 255, 0, 33, 2 );
static MenuEntry_t ME_SOUND_VOLUME_MUSIC = MAKE_MENUENTRY( "Music:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_VOLUME_MUSIC, RangeInt32 );

static MenuOption_t MEO_SOUND_DUKETALK = MAKE_MENUOPTION(&MF_Redfont, &MEOS_NoYes, NULL);
static MenuEntry_t ME_SOUND_DUKETALK = MAKE_MENUENTRY( "Duke talk:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_DUKETALK, Option );

static char const *MEOSN_SOUND_SAMPLINGRATE[] = { "22050Hz", "44100Hz", "48000Hz", };
static int32_t MEOSV_SOUND_SAMPLINGRATE[] = { 22050, 44100, 48000, };
static MenuOptionSet_t MEOS_SOUND_SAMPLINGRATE = MAKE_MENUOPTIONSET( MEOSN_SOUND_SAMPLINGRATE, MEOSV_SOUND_SAMPLINGRATE, 0x3 );
static MenuOption_t MEO_SOUND_SAMPLINGRATE = MAKE_MENUOPTION( &MF_Redfont, &MEOS_SOUND_SAMPLINGRATE, &soundrate );
static MenuEntry_t ME_SOUND_SAMPLINGRATE = MAKE_MENUENTRY( "Sample rate:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_SAMPLINGRATE, Option );

#ifndef EDUKE32_SIMPLE_MENU
static MenuRangeInt32_t MEO_SOUND_NUMVOICES = MAKE_MENURANGE( &soundvoices, &MF_Redfont, 16, 256, 0, 16, 1 );
static MenuEntry_t ME_SOUND_NUMVOICES = MAKE_MENUENTRY( "Voices:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_NUMVOICES, RangeInt32 );
#endif



static MenuEntry_t ME_CDPLAYER_TRACK = MAKE_MENUENTRY( NULL, &MF_Redfont, &MEF_Null, &MEO_NULL, Dummy);

static MenuEntry_t *MEL_CDPLAYER[] = {
    &ME_CDPLAYER_TRACK,
    &ME_CDPLAYER_TRACK,
    &ME_CDPLAYER_TRACK,
    &ME_CDPLAYER_TRACK,
    &ME_CDPLAYER_TRACK,
    &ME_CDPLAYER_TRACK,
    &ME_CDPLAYER_TRACK,
    &ME_CDPLAYER_TRACK
};


static MenuOption_t MEO_SAVESETUP_AUTOSAVE = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OffOn, &cl_autosave );
static MenuEntry_t ME_SAVESETUP_AUTOSAVE = MAKE_MENUENTRY( "Autosaves:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SAVESETUP_AUTOSAVE, Option );

static MenuOption_t MEO_SAVESETUP_AUTOSAVEDELETION = MAKE_MENUOPTION( &MF_Redfont, &MEOS_NoYes, &cl_autosavedeletion );
static MenuEntry_t ME_SAVESETUP_AUTOSAVEDELETION = MAKE_MENUENTRY( "Auto-Delete:", &MF_Redfont, &MEF_BigOptions_Apply, &MEO_SAVESETUP_AUTOSAVEDELETION, Option );
static MenuRangeInt32_t MEO_SAVESETUP_MAXAUTOSAVES = MAKE_MENURANGE( &cl_maxautosaves, &MF_Redfont, 1, 10, 0, 10, 1 );
static MenuEntry_t ME_SAVESETUP_MAXAUTOSAVES = MAKE_MENUENTRY( "Limit:", &MF_Redfont, &MEF_BigOptions_Apply, &MEO_SAVESETUP_MAXAUTOSAVES, RangeInt32 );

static MenuEntry_t ME_SAVESETUP_CLEANUP = MAKE_MENUENTRY( "Clean Up Saves", &MF_Redfont, &MEF_BigOptionsRt, &MEO_NULL, Link );

static MenuEntry_t *MEL_SAVESETUP[] = {
    &ME_SAVESETUP_AUTOSAVE,
    &ME_SAVESETUP_AUTOSAVEDELETION,
    &ME_SAVESETUP_MAXAUTOSAVES,
    &ME_SAVESETUP_CLEANUP,
};


MAKE_MENU_TOP_ENTRYLINK( "Player Setup", MEF_CenterMenu, NETWORK_PLAYERSETUP, MENU_PLAYER );
MAKE_MENU_TOP_ENTRYLINK( "Join Game", MEF_CenterMenu, NETWORK_JOINGAME, MENU_NETJOIN );
MAKE_MENU_TOP_ENTRYLINK( "Host Game", MEF_CenterMenu, NETWORK_HOSTGAME, MENU_NETHOST );

static MenuEntry_t *MEL_NETWORK[] = {
    &ME_NETWORK_PLAYERSETUP,
    &ME_NETWORK_JOINGAME,
    &ME_NETWORK_HOSTGAME,
};

//static MenuString_t MEO_PLAYER_NAME = MAKE_MENUSTRING( playername, &MF_Bluefont, MAXPLAYERNAME, 0 );
//static MenuEntry_t ME_PLAYER_NAME = MAKE_MENUENTRY( "Name", &MF_Bluefont, &MEF_PlayerNarrow, &MEO_PLAYER_NAME, String );
static char const *MEOSN_PLAYER_COLOR[] = { "Auto", "Blue", "Red", "Green", "Gray", "Dark gray", "Dark green", "Brown", "Dark blue", "Bright red", "Yellow", };
static int32_t MEOSV_PLAYER_COLOR[] = { 0, 9, 10, 11, 12, 13, 14, 15, 16, 21, 23, };
static MenuOptionSet_t MEOS_PLAYER_COLOR = MAKE_MENUOPTIONSET( MEOSN_PLAYER_COLOR, MEOSV_PLAYER_COLOR, 0x2 );
static MenuOption_t MEO_PLAYER_COLOR = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_PLAYER_COLOR, &playercolor );
static MenuEntry_t ME_PLAYER_COLOR = MAKE_MENUENTRY( "Color", &MF_Bluefont, &MEF_PlayerNarrow, &MEO_PLAYER_COLOR, Option );
static char const *MEOSN_PLAYER_TEAM[] = { "Blue", "Red", "Green", "Gray", };
static MenuOptionSet_t MEOS_PLAYER_TEAM = MAKE_MENUOPTIONSET( MEOSN_PLAYER_TEAM, NULL, 0x2 );
static MenuOption_t MEO_PLAYER_TEAM = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_PLAYER_TEAM, &playerteam );
static MenuEntry_t ME_PLAYER_TEAM = MAKE_MENUENTRY( "Team", &MF_Bluefont, &MEF_PlayerNarrow, &MEO_PLAYER_TEAM, Option );
#ifndef EDUKE32_SIMPLE_MENU
static MenuLink_t MEO_PLAYER_MACROS = { MENU_MACROS, MA_Advance, };
static MenuEntry_t ME_PLAYER_MACROS = MAKE_MENUENTRY( "Multiplayer macros", &MF_Bluefont, &MEF_SmallOptions, &MEO_PLAYER_MACROS, Link );
#endif

static MenuEntry_t *MEL_PLAYER[] = {
    //&ME_PLAYER_NAME,
    &ME_Space4_Bluefont,
    &ME_PLAYER_COLOR,
    &ME_Space4_Bluefont,
    &ME_PLAYER_TEAM,
#ifndef EDUKE32_SIMPLE_MENU
    &ME_Space8_Bluefont,
    &ME_PLAYER_MACROS,
#endif
};

#define MAXRIDECULE 10
#define MAXRIDECULELENGTH 40
static MenuString_t MEO_MACROS_TEMPLATE = MAKE_MENUSTRING( NULL, &MF_Bluefont, MAXRIDECULELENGTH, 0 );
static MenuString_t MEO_MACROS[10];
static char sink[50];
static MenuEntry_t ME_MACROS_TEMPLATE = MAKE_MENUENTRY( NULL, &MF_Bluefont, &MEF_Macros, &MEO_MACROS_TEMPLATE, String );
static MenuEntry_t ME_MACROS[MAXRIDECULE];
static MenuEntry_t *MEL_MACROS[MAXRIDECULE];

#ifndef EDUKE32_SIMPLE_MENU
static char const *MenuUserMap = "User Map";
#endif
static char const *MenuSkillNone = "None";

static char const *MEOSN_NetGametypes[MAXGAMETYPES];
static char const *MEOSN_NetEpisodes[MAXVOLUMES+1];
static int32_t MEOSV_NetEpisodes[MAXVOLUMES+1];
static char const *MEOSN_NetLevels[MAXVOLUMES][MAXLEVELS];
static char const *MEOSN_NetSkills[MAXSKILLS+1];

static MenuLink_t MEO_NETHOST_OPTIONS = { MENU_NETOPTIONS, MA_Advance, };
static MenuEntry_t ME_NETHOST_OPTIONS = MAKE_MENUENTRY( "Game Options", &MF_Redfont, &MEF_VideoSetup, &MEO_NETHOST_OPTIONS, Link );
static MenuEntry_t ME_NETHOST_LAUNCH = MAKE_MENUENTRY( "Launch Game", &MF_Redfont, &MEF_VideoSetup, &MEO_NULL, Link );

static MenuEntry_t *MEL_NETHOST[] = {
    &ME_NETHOST_OPTIONS,
    &ME_NETHOST_LAUNCH,
};

static MenuOptionSet_t MEOS_NETOPTIONS_GAMETYPE = MAKE_MENUOPTIONSET( MEOSN_NetGametypes, NULL, 0x0 );
static MenuOption_t MEO_NETOPTIONS_GAMETYPE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NETOPTIONS_GAMETYPE, &m_coop );
static MenuEntry_t ME_NETOPTIONS_GAMETYPE = MAKE_MENUENTRY( "Game Type", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_GAMETYPE, Option );
static MenuOptionSet_t MEOS_NETOPTIONS_EPISODE = MAKE_MENUOPTIONSET( MEOSN_NetEpisodes, MEOSV_NetEpisodes, 0x0 );
CVAR_UNAMED(Int, NetEpisode);
static MenuOption_t MEO_NETOPTIONS_EPISODE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NETOPTIONS_EPISODE, &NetEpisode );
static MenuEntry_t ME_NETOPTIONS_EPISODE = MAKE_MENUENTRY( "Episode", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_EPISODE, Option );
static MenuOptionSet_t MEOS_NETOPTIONS_LEVEL_TEMPLATE = MAKE_MENUOPTIONSETNULL;
static MenuOptionSet_t MEOS_NETOPTIONS_LEVEL[MAXVOLUMES];
static MenuOption_t MEO_NETOPTIONS_LEVEL = MAKE_MENUOPTION( &MF_Bluefont, NULL, &m_level_number );
static MenuEntry_t ME_NETOPTIONS_LEVEL = MAKE_MENUENTRY( "Level", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_LEVEL, Option );
static MenuLink_t MEO_NETOPTIONS_USERMAP = { MENU_NETUSERMAP, MA_Advance, };
static MenuEntry_t ME_NETOPTIONS_USERMAP = MAKE_MENUENTRY( "User Map", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_USERMAP, Link );
static MenuOptionSet_t MEOS_NETOPTIONS_MONSTERS = MAKE_MENUOPTIONSET( MEOSN_NetSkills, NULL, 0x0 );
static MenuOption_t MEO_NETOPTIONS_MONSTERS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NETOPTIONS_MONSTERS, NULL );
static MenuEntry_t ME_NETOPTIONS_MONSTERS = MAKE_MENUENTRY( "Monsters", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_MONSTERS, Option );
static MenuOption_t MEO_NETOPTIONS_MARKERS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &m_marker);
static MenuEntry_t ME_NETOPTIONS_MARKERS = MAKE_MENUENTRY( "Markers", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_MARKERS, Option );
static MenuOption_t MEO_NETOPTIONS_MAPEXITS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OnOff, &m_noexits);
static MenuEntry_t ME_NETOPTIONS_MAPEXITS = MAKE_MENUENTRY( "Map Exits", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_MAPEXITS, Option );
static MenuOption_t MEO_NETOPTIONS_FRFIRE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &m_ffire);
static MenuEntry_t ME_NETOPTIONS_FRFIRE = MAKE_MENUENTRY( "Fr. Fire", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_FRFIRE, Option );
static MenuEntry_t ME_NETOPTIONS_ACCEPT = MAKE_MENUENTRY( "Accept", &MF_Redfont, &MEF_NetSetup_Confirm, &MEO_NETWORK_HOSTGAME, Link );

static MenuEntry_t *MEL_NETOPTIONS[] = {
    &ME_NETOPTIONS_GAMETYPE,
    &ME_NETOPTIONS_EPISODE,
    &ME_NETOPTIONS_LEVEL,
    &ME_NETOPTIONS_MONSTERS,
    &ME_NETOPTIONS_MARKERS,
    &ME_NETOPTIONS_MAPEXITS,
    &ME_NETOPTIONS_ACCEPT,
};

static char MenuServer[BMAX_PATH] = "localhost";
static MenuString_t MEO_NETJOIN_SERVER = MAKE_MENUSTRING( MenuServer, &MF_Bluefont, BMAX_PATH, 0 );
static MenuEntry_t ME_NETJOIN_SERVER = MAKE_MENUENTRY( "Server", &MF_Redfont, &MEF_VideoSetup, &MEO_NETJOIN_SERVER, String );
#define MAXPORTSTRINGLENGTH 6 // unsigned 16-bit integer
static char MenuPort[MAXPORTSTRINGLENGTH] = "19014";
static MenuString_t MEO_NETJOIN_PORT = MAKE_MENUSTRING( MenuPort, &MF_Bluefont, MAXPORTSTRINGLENGTH, INPUT_NUMERIC );
static MenuEntry_t ME_NETJOIN_PORT = MAKE_MENUENTRY( "Port", &MF_Redfont, &MEF_VideoSetup, &MEO_NETJOIN_PORT, String );
static MenuEntry_t ME_NETJOIN_CONNECT = MAKE_MENUENTRY( "Connect", &MF_Redfont, &MEF_VideoSetup_Apply, &MEO_NULL, Link );

static MenuEntry_t *MEL_NETJOIN[] = {
    &ME_NETJOIN_SERVER,
    &ME_NETJOIN_PORT,
    &ME_NETJOIN_CONNECT,
};


#define NoTitle NULL

#define MAKE_MENUMENU(Title, Format, Entries) { Title, Format, Entries, ARRAY_SIZE(Entries), 0, 0, 0 }
#define MAKE_MENUMENU_CUSTOMSIZE(Title, Format, Entries) { Title, Format, Entries, 0, 0, 0, 0 }

static MenuMenu_t M_MAIN = MAKE_MENUMENU( NoTitle, &MMF_Top_Main, MEL_MAIN );
static MenuMenu_t M_MAIN_INGAME = MAKE_MENUMENU( NoTitle, &MMF_Top_Main, MEL_MAIN_INGAME );
static MenuMenu_t M_EPISODE = MAKE_MENUMENU( "Select An Episode", &MMF_Top_Episode, MEL_EPISODE );
static MenuMenu_t M_SKILL = MAKE_MENUMENU( "Select Skill", &MMF_Top_Skill, MEL_SKILL );
#ifndef EDUKE32_SIMPLE_MENU
static MenuMenu_t M_GAMESETUP = MAKE_MENUMENU( "Game Setup", &MMF_BigOptions, MEL_GAMESETUP );
#endif
static MenuMenu_t M_OPTIONS = MAKE_MENUMENU( s_Options, &MMF_Top_Options, MEL_OPTIONS );
static MenuMenu_t M_VIDEOSETUP = MAKE_MENUMENU( "Video Mode", &MMF_BigOptions, MEL_VIDEOSETUP );
static MenuMenu_t M_KEYBOARDSETUP = MAKE_MENUMENU( "Configure Controls", &MMF_Top_Options, MEL_KEYBOARDSETUP );
static MenuMenu_t M_CONTROLS = MAKE_MENUMENU( "Control Setup", &MMF_Top_Options, MEL_CONTROLS );
static MenuMenu_t M_CHEATS = MAKE_MENUMENU( "Cheats", &MMF_SmallOptions, MEL_CHEATS );
static MenuMenu_t M_MOUSESETUP = MAKE_MENUMENU( "Mouse Setup", &MMF_BigOptions, MEL_MOUSESETUP );
#ifdef EDUKE32_ANDROID_MENU
static MenuMenu_t M_TOUCHSETUP = MAKE_MENUMENU( "Touch Setup", &MMF_Top_Options, MEL_TOUCHSETUP );
static MenuMenu_t M_TOUCHSENS = MAKE_MENUMENU( "Sensitivity", &MMF_BigOptions, MEL_TOUCHSENS);
static MenuPanel_t M_TOUCHBUTTONS = { "Button Setup", MENU_TOUCHSETUP, MA_Return, MENU_TOUCHSETUP, MA_Advance, };
#endif
static MenuMenu_t M_JOYSTICKSETUP = MAKE_MENUMENU( "Joystick Setup", &MMF_Top_Joystick_Network, MEL_JOYSTICKSETUP );
static MenuMenu_t M_JOYSTICKBTNS = MAKE_MENUMENU( "Joystick Buttons", &MMF_MouseJoySetupBtns, MEL_JOYSTICKBTNS );
static MenuMenu_t M_JOYSTICKAXES = MAKE_MENUMENU( "Joystick Axes", &MMF_BigSliders, MEL_JOYSTICKAXES );
static MenuMenu_t M_KEYBOARDKEYS = MAKE_MENUMENU( "Key Configuration", &MMF_KeyboardSetupFuncs, MEL_KEYBOARDSETUPFUNCS );
static MenuMenu_t M_MOUSEADVANCED = MAKE_MENUMENU( "Advanced Mouse", &MMF_BigSliders, MEL_MOUSEADVANCED );
static MenuMenu_t M_JOYSTICKAXIS = MAKE_MENUMENU( NULL, &MMF_BigSliders, MEL_JOYSTICKAXIS );
#ifdef USE_OPENGL
static MenuMenu_t M_RENDERERSETUP_POLYMOST = MAKE_MENUMENU( "Polymost Setup", &MMF_SmallOptions, MEL_RENDERERSETUP_POLYMOST );
# ifdef POLYMER
static MenuMenu_t M_RENDERERSETUP_POLYMER = MAKE_MENUMENU("Polymer Setup", &MMF_SmallOptions, MEL_RENDERERSETUP_POLYMER );
# endif
#endif
static MenuMenu_t M_COLCORR = MAKE_MENUMENU( "Color Correction", &MMF_ColorCorrect, MEL_COLCORR );
static MenuMenu_t M_SCREENSETUP = MAKE_MENUMENU( "HUD Setup", &MMF_BigOptions, MEL_SCREENSETUP );
static MenuMenu_t M_DISPLAYSETUP = MAKE_MENUMENU( "Display Setup", &MMF_BigOptions, MEL_DISPLAYSETUP );
static MenuMenu_t M_LOAD = MAKE_MENUMENU_CUSTOMSIZE( s_LoadGame, &MMF_LoadSave, MEL_LOAD );
static MenuMenu_t M_SAVE = MAKE_MENUMENU_CUSTOMSIZE( s_SaveGame, &MMF_LoadSave, MEL_SAVE );
static MenuMenu_t M_SOUND = MAKE_MENUMENU( "Sound Setup", &MMF_BigOptions, MEL_SOUND );
static MenuMenu_t M_ADVSOUND = MAKE_MENUMENU( "Advanced Sound", &MMF_BigOptions, MEL_ADVSOUND );
static MenuMenu_t M_CDPLAYER = MAKE_MENUMENU( "8 Track Player", &MMF_BigOptions, MEL_CDPLAYER );
static MenuMenu_t M_SAVESETUP = MAKE_MENUMENU( "Save Setup", &MMF_BigOptions, MEL_SAVESETUP );
static MenuMenu_t M_NETWORK = MAKE_MENUMENU( "Network Game", &MMF_Top_Joystick_Network, MEL_NETWORK );
static MenuMenu_t M_PLAYER = MAKE_MENUMENU( "Player Setup", &MMF_SmallOptions, MEL_PLAYER );
static MenuMenu_t M_MACROS = MAKE_MENUMENU( "Multiplayer Macros", &MMF_Macros, MEL_MACROS );
static MenuMenu_t M_NETHOST = MAKE_MENUMENU( "Host Network Game", &MMF_SmallOptionsNarrow, MEL_NETHOST );
static MenuMenu_t M_NETOPTIONS = MAKE_MENUMENU( "Net Game Options", &MMF_NetSetup, MEL_NETOPTIONS );
static MenuMenu_t M_NETJOIN = MAKE_MENUMENU( "Join Network Game", &MMF_SmallOptionsNarrow, MEL_NETJOIN );

#ifdef EDUKE32_SIMPLE_MENU
static MenuPanel_t M_STORY = { NoTitle, MENU_STORY, MA_Return, MENU_STORY, MA_Advance, };
#else
static MenuPanel_t M_STORY = { NoTitle, MENU_F1HELP, MA_Return, MENU_F1HELP, MA_Advance, };
#endif


#define CURSOR_CENTER_2LINE { MENU_MARGIN_CENTER<<16, 120<<16, }
#define CURSOR_CENTER_3LINE { MENU_MARGIN_CENTER<<16, 129<<16, }
#define CURSOR_BOTTOMRIGHT { 304<<16, 186<<16, }

static MenuVerify_t M_SAVECLEANVERIFY = { CURSOR_CENTER_3LINE, MENU_SAVESETUP, MA_None, };
static MenuVerify_t M_QUIT = { CURSOR_CENTER_2LINE, MENU_CLOSE, MA_None, };
static MenuVerify_t M_QUITTOTITLE = { CURSOR_CENTER_2LINE, MENU_CLOSE, MA_None, };
static MenuVerify_t M_LOADVERIFY = { CURSOR_CENTER_3LINE, MENU_CLOSE, MA_None, };
static MenuVerify_t M_LOADDELVERIFY = { CURSOR_CENTER_3LINE, MENU_LOAD, MA_None, };
static MenuVerify_t M_NEWVERIFY = { CURSOR_CENTER_2LINE, MENU_EPISODE, MA_Advance, };
static MenuVerify_t M_SAVEVERIFY = { CURSOR_CENTER_2LINE, MENU_SAVE, MA_None, };
static MenuVerify_t M_SAVEDELVERIFY = { CURSOR_CENTER_3LINE, MENU_SAVE, MA_None, };
static MenuVerify_t M_RESETPLAYER = { CURSOR_CENTER_3LINE, MENU_CLOSE, MA_None, };

static MenuMessage_t M_NETWAITMASTER = { CURSOR_BOTTOMRIGHT, MENU_NULL, MA_None, };
static MenuMessage_t M_NETWAITVOTES = { CURSOR_BOTTOMRIGHT, MENU_NULL, MA_None, };
static MenuMessage_t M_BUYDUKE = { CURSOR_BOTTOMRIGHT, MENU_EPISODE, MA_Return, };

static MenuTextForm_t M_ADULTPASSWORD = { NULL, "Enter Password:", MAXPWLOCKOUT, MTF_Password };
static MenuTextForm_t M_CHEATENTRY = { NULL, "Enter Cheat Code:", MAXCHEATLEN, 0 };
static MenuTextForm_t M_CHEAT_WARP = { NULL, "Enter Warp #:", 3, 0 };
static MenuTextForm_t M_CHEAT_SKILL = { NULL, "Enter Skill #:", 1, 0 };

//#define MAKE_MENUFILESELECT(a, b, c) { a, { &MMF_FileSelectLeft, &MMF_FileSelectRight }, { &MF_Minifont, &MF_Minifont }, b, c, { NULL, NULL }, { 0, 0 }, { 3<<16, 3<<16 }, FNLIST_INITIALIZER, 0 }

//static MenuFileSelect_t M_USERMAP = MAKE_MENUFILESELECT( "Select A User Map", "*.map", boardfilename );

// MUST be in ascending order of MenuID enum values due to binary search
static Menu_t Menus[] = {
    { &M_MAIN, MENU_MAIN, MENU_CLOSE, MA_None, Menu },
    { &M_MAIN_INGAME, MENU_MAIN_INGAME, MENU_CLOSE, MA_None, Menu },
    { &M_EPISODE, MENU_EPISODE, MENU_MAIN, MA_Return, Menu },
    //{ &M_USERMAP, MENU_USERMAP, MENU_EPISODE, MA_Return, FileSelect },
    { &M_SKILL, MENU_SKILL, MENU_EPISODE, MA_Return, Menu },
#ifndef EDUKE32_SIMPLE_MENU
    { &M_GAMESETUP, MENU_GAMESETUP, MENU_OPTIONS, MA_Return, Menu },
#endif
    { &M_OPTIONS, MENU_OPTIONS, MENU_MAIN, MA_Return, Menu },
    { &M_VIDEOSETUP, MENU_VIDEOSETUP, MENU_DISPLAYSETUP, MA_Return, Menu },
    { &M_KEYBOARDSETUP, MENU_KEYBOARDSETUP, MENU_CONTROLS, MA_Return, Menu },
    { &M_MOUSESETUP, MENU_MOUSESETUP, MENU_CONTROLS, MA_Return, Menu },
    { &M_JOYSTICKSETUP, MENU_JOYSTICKSETUP, MENU_CONTROLS, MA_Return, Menu },
    { &M_JOYSTICKBTNS, MENU_JOYSTICKBTNS, MENU_JOYSTICKSETUP, MA_Return, Menu },
    { &M_JOYSTICKAXES, MENU_JOYSTICKAXES, MENU_JOYSTICKSETUP, MA_Return, Menu },
    { &M_KEYBOARDKEYS, MENU_KEYBOARDKEYS, MENU_KEYBOARDSETUP, MA_Return, Menu },
    { &M_MOUSEADVANCED, MENU_MOUSEADVANCED, MENU_MOUSESETUP, MA_Return, Menu },
    { &M_JOYSTICKAXIS, MENU_JOYSTICKAXIS, MENU_JOYSTICKAXES, MA_Return, Menu },
#ifdef EDUKE32_ANDROID_MENU
    { &M_TOUCHSETUP, MENU_TOUCHSETUP, MENU_OPTIONS, MA_Return, Menu },
    { &M_TOUCHSENS, MENU_TOUCHSENS, MENU_TOUCHSETUP, MA_Return, Menu },
    { &M_TOUCHBUTTONS, MENU_TOUCHBUTTONS, MENU_TOUCHSETUP, MA_Return, Panel },
#endif
    { &M_CONTROLS, MENU_CONTROLS, MENU_OPTIONS, MA_Return, Menu },
#ifdef USE_OPENGL
    { &M_RENDERERSETUP_POLYMOST, MENU_POLYMOST, MENU_DISPLAYSETUP, MA_Return, Menu },
#endif
    { &M_COLCORR, MENU_COLCORR, MENU_DISPLAYSETUP, MA_Return, Menu },
    { &M_COLCORR, MENU_COLCORR_INGAME, MENU_CLOSE, MA_Return, Menu },
    { &M_SCREENSETUP, MENU_SCREENSETUP, MENU_DISPLAYSETUP, MA_Return, Menu },
    { &M_DISPLAYSETUP, MENU_DISPLAYSETUP, MENU_OPTIONS, MA_Return, Menu },
#ifdef POLYMER
    { &M_RENDERERSETUP_POLYMER, MENU_POLYMER, MENU_DISPLAYSETUP, MA_Return, Menu },
#endif
    { &M_LOAD, MENU_LOAD, MENU_MAIN, MA_Return, Menu },
    { &M_SAVE, MENU_SAVE, MENU_MAIN, MA_Return, Menu },
    { &M_STORY, MENU_STORY, MENU_MAIN, MA_Return, Panel },
    { &M_F1HELP, MENU_F1HELP, MENU_MAIN, MA_Return, Panel },
    { &M_F1HELP2, MENU_F1HELP2, MENU_MAIN, MA_Return, Panel },
    { &M_QUIT, MENU_QUIT, MENU_PREVIOUS, MA_Return, Verify },
    { &M_QUITTOTITLE, MENU_QUITTOTITLE, MENU_PREVIOUS, MA_Return, Verify },
    { &M_QUIT, MENU_QUIT_INGAME, MENU_CLOSE, MA_None, Verify },
    { &M_NETHOST, MENU_NETSETUP, MENU_MAIN, MA_Return, Menu },
    { &M_NETWAITMASTER, MENU_NETWAITMASTER, MENU_MAIN, MA_Return, Message },
    { &M_NETWAITVOTES, MENU_NETWAITVOTES, MENU_MAIN, MA_Return, Message },
    { &M_SOUND, MENU_SOUND, MENU_OPTIONS, MA_Return, Menu },
    { &M_SOUND, MENU_SOUND_INGAME, MENU_CLOSE, MA_Return, Menu },
    { &M_ADVSOUND, MENU_ADVSOUND, MENU_SOUND, MA_Return, Menu },
    { &M_CDPLAYER, MENU_CDPLAYER, MENU_SOUND, MA_Return, CdPlayer },
    { &M_SAVESETUP, MENU_SAVESETUP, MENU_OPTIONS, MA_Return, Menu },
    { &M_SAVECLEANVERIFY, MENU_SAVECLEANVERIFY, MENU_SAVESETUP, MA_None, Verify },
#ifdef EDUKE32_SIMPLE_MENU
    { &M_CHEATS, MENU_CHEATS, MENU_OPTIONS, MA_Return, Menu },
#else
    { &M_CHEATS, MENU_CHEATS, MENU_GAMESETUP, MA_Return, Menu },
#endif
    { &M_CHEATENTRY, MENU_CHEATENTRY, MENU_CHEATS, MA_None, TextForm },
    { &M_CHEAT_WARP, MENU_CHEAT_WARP, MENU_CHEATS, MA_None, TextForm },
    { &M_CHEAT_SKILL, MENU_CHEAT_SKILL, MENU_CHEATS, MA_None, TextForm },
    { &M_LOADVERIFY, MENU_LOADVERIFY, MENU_LOAD, MA_None, Verify },
    { &M_LOADDELVERIFY, MENU_LOADDELVERIFY, MENU_LOAD, MA_None, Verify },
    { &M_NEWVERIFY, MENU_NEWVERIFY, MENU_PREVIOUS, MA_Return, Verify },
    { &M_SAVEVERIFY, MENU_SAVEVERIFY, MENU_SAVE, MA_None, Verify },
    { &M_SAVEDELVERIFY, MENU_SAVEDELVERIFY, MENU_SAVE, MA_None, Verify },
    { &M_ADULTPASSWORD, MENU_ADULTPASSWORD, MENU_GAMESETUP, MA_None, TextForm },
    { &M_RESETPLAYER, MENU_RESETPLAYER, MENU_CLOSE, MA_None, Verify },
    { &M_BUYDUKE, MENU_BUYDUKE, MENU_EPISODE, MA_Return, Message },
    { &M_NETWORK, MENU_NETWORK, MENU_MAIN, MA_Return, Menu },
    { &M_PLAYER, MENU_PLAYER, MENU_OPTIONS, MA_Return, Menu },
    { &M_MACROS, MENU_MACROS, MENU_PLAYER, MA_Return, Menu },
    { &M_NETHOST, MENU_NETHOST, MENU_NETWORK, MA_Return, Menu },
    { &M_NETOPTIONS, MENU_NETOPTIONS, MENU_NETWORK, MA_Return, Menu },
    //{ &M_USERMAP, MENU_NETUSERMAP, MENU_NETOPTIONS, MA_Return, FileSelect },
    { &M_NETJOIN, MENU_NETJOIN, MENU_NETWORK, MA_Return, Menu },
};


/*
This function prepares data after ART and CON have been processed.
It also initializes some data in loops rather than statically at compile time.
*/
void Menu_Init(void)
{
    int32_t i, j, k;

    // prepare menu fonts
    // check if tilenum is -1 in case it was set in EVENT_SETDEFAULTS
    if ((unsigned)MF_Redfont.tilenum >= MAXTILES) MF_Redfont.tilenum = BIGALPHANUM;
    if ((unsigned)MF_Bluefont.tilenum >= MAXTILES) MF_Bluefont.tilenum = STARTALPHANUM;
    if ((unsigned)MF_Minifont.tilenum >= MAXTILES) MF_Minifont.tilenum = MINIFONT;
    MF_Redfont.emptychar.y = tilesiz[MF_Redfont.tilenum].y<<16;
    MF_Bluefont.emptychar.y = tilesiz[MF_Bluefont.tilenum].y<<16;
    MF_Minifont.emptychar.y = tilesiz[MF_Minifont.tilenum].y<<16;
    if (!minitext_lowercase)
        MF_Minifont.textflags |= TEXT_UPPERCASE;

    // prepare gamefuncs and keys
    MEOSN_Gamefuncs[0] = MenuGameFuncNone;
    MEOSV_Gamefuncs[0] = -1;
    k = 1;
    for (i = 0; i < NUMGAMEFUNCTIONS; ++i)
    {
		MenuGameFuncs[i] = buttonMap.GetButtonAlias(i);
		MenuGameFuncs[i].Substitute('_', ' ');
    }
    if (RR)
    {
        MenuGameFuncs[gamefunc_Holo_Duke] = "Beer";
        MenuGameFuncs[gamefunc_Jetpack] = "CowPie";
        MenuGameFuncs[gamefunc_NightVision] = "Yeehaa";
        MenuGameFuncs[gamefunc_MedKit] = "Whiskey";
        MenuGameFuncs[gamefunc_Steroids] = "Moonshine";
        MenuGameFuncs[gamefunc_Quick_Kick] = "Pee";
    }
	for (i = 0; i < NUMGAMEFUNCTIONS; ++i)
	{
		if (MenuGameFuncs[i][0] != '\0')
		{
			MEOSN_Gamefuncs[k] = MenuGameFuncs[i];
			MEOSV_Gamefuncs[k] = i;
			++k;
		}
	}

    MEOS_Gamefuncs.numOptions = k;

    for (i = 0; i < NUMKEYS; ++i)
        MEOSN_Keys[i] = KB_ScanCodeToString(i);
    MEOSN_Keys[NUMKEYS-1] = MenuKeyNone;


    // prepare episodes
    k = 0;
    for (i = 0; i < g_volumeCnt; ++i)
    {
        if (gVolumeNames[i].IsNotEmpty())
        {
            if (!(gVolumeFlags[i] & EF_HIDEFROMSP))
            {
                MEL_EPISODE[i] = &ME_EPISODE[i];
                ME_EPISODE[i] = ME_EPISODE_TEMPLATE;
                ME_EPISODE[i].name = gVolumeNames[i];
            }

            // if (!(EpisodeFlags[i] & EF_HIDEFROMMP))
            {
                MEOSN_NetEpisodes[k] = gVolumeNames[i];
                MEOSV_NetEpisodes[k] = i;

                k++;
            }
        }

        // prepare levels
        MEOS_NETOPTIONS_LEVEL[i] = MEOS_NETOPTIONS_LEVEL_TEMPLATE;
        for (j = 0; j < MAXLEVELS; ++j)
        {
            MEOSN_NetLevels[i][j] = g_mapInfo[MAXLEVELS*i+j].name;
            if (g_mapInfo[i*MAXLEVELS+j].filename != NULL)
                MEOS_NETOPTIONS_LEVEL[i].numOptions = j+1;
        }
        MEOS_NETOPTIONS_LEVEL[i].optionNames = MEOSN_NetLevels[i];
    }
    M_EPISODE.numEntries = g_volumeCnt+2;
#ifndef EDUKE32_SIMPLE_MENU
    MEL_EPISODE[g_volumeCnt] = &ME_Space4_Redfont;
    MEL_EPISODE[g_volumeCnt+1] = &ME_EPISODE_USERMAP;
    MEOSN_NetEpisodes[k] = MenuUserMap;
    MEOSV_NetEpisodes[k] = MAXVOLUMES;
#else
    M_EPISODE.numEntries = g_volumeCnt;
    k--;
#endif
    MEOS_NETOPTIONS_EPISODE.numOptions = k + 1;
    NetEpisode = MEOSV_NetEpisodes[0];
    MMF_Top_Episode.pos.y = (58 + (3-k)*6)<<16;
    if (g_skillCnt == 0)
        MEO_EPISODE.linkID = MENU_NULL;
	M_EPISODE.currentEntry = gDefaultVolume;

    // prepare skills
    k = -1;
    for (i = 0; i < g_skillCnt && gSkillNames[i].IsNotEmpty(); ++i)
    {
        MEL_SKILL[i] = &ME_SKILL[i];
        ME_SKILL[i] = ME_SKILL_TEMPLATE;
        ME_SKILL[i].name = gSkillNames[i];

        MEOSN_NetSkills[i] = gSkillNames[i];

        k = i;
    }
    ++k;
    M_SKILL.numEntries = g_skillCnt; // k;
    MEOS_NETOPTIONS_MONSTERS.numOptions = g_skillCnt + 1; // k+1;
    MEOSN_NetSkills[g_skillCnt] = MenuSkillNone;
    MMF_Top_Skill.pos.y = (58 + (4-g_skillCnt)*6)<<16;
    M_SKILL.currentEntry = gDefaultSkill;
    Menu_AdjustForCurrentEntryAssignmentBlind(&M_SKILL);

    // prepare multiplayer gametypes
    k = -1;
    for (i = 0; i < MAXGAMETYPES; ++i)
        if (g_gametypeNames[i][0])
        {
            MEOSN_NetGametypes[i] = g_gametypeNames[i];
            k = i;
        }
    ++k;
    MEOS_NETOPTIONS_GAMETYPE.numOptions = k;

    // prepare cheats
    for (i = 0; i < NUMCHEATFUNCS; ++i)
        MEL_CHEATS[i+1] = &ME_CheatCodes[i];

    // prepare text chat macros
    for (i = 0; i < MAXRIDECULE; ++i)
    {
        MEL_MACROS[i] = &ME_MACROS[i];
        ME_MACROS[i] = ME_MACROS_TEMPLATE;
        ME_MACROS[i].entry = &MEO_MACROS[i];
        MEO_MACROS[i] = MEO_MACROS_TEMPLATE;

		MEO_MACROS[i].variable = sink;// ud.ridecule[i];	temporarily disabled
	}

    // prepare input
    for (i = 0; i < NUMGAMEFUNCTIONS; ++i)
    {
        if (MenuGameFuncs[i][0] == '\0')
        {
            MEL_KEYBOARDSETUPFUNCS[i] = NULL;
            continue;
        }

        MEL_KEYBOARDSETUPFUNCS[i] = &ME_KEYBOARDSETUPFUNCS[i];
        ME_KEYBOARDSETUPFUNCS[i] = ME_KEYBOARDSETUPFUNCS_TEMPLATE;
        ME_KEYBOARDSETUPFUNCS[i].name = MenuGameFuncs[i];
        ME_KEYBOARDSETUPFUNCS[i].entry = &MEO_KEYBOARDSETUPFUNCS[i];
        MEO_KEYBOARDSETUPFUNCS[i] = MEO_KEYBOARDSETUPFUNCS_TEMPLATE;
    }
    M_KEYBOARDKEYS.numEntries = NUMGAMEFUNCTIONS;
    for (i = 0; i < 2*joystick.numButtons + 8*joystick.numHats; ++i)
    {
        if (i < 2*joystick.numButtons)
        {
            if (i & 1)
                Bsnprintf(MenuJoystickNames[i], MAXJOYBUTTONSTRINGLENGTH, "Double %s", joyGetName(1, i>>1));
            else
                Bstrncpy(MenuJoystickNames[i], joyGetName(1, i>>1), MAXJOYBUTTONSTRINGLENGTH);
        }
        else
        {
            Bsnprintf(MenuJoystickNames[i], MAXJOYBUTTONSTRINGLENGTH, (i & 1) ? "Double Hat %d %s" : "Hat %d %s", ((i - 2*joystick.numButtons)>>3), MenuJoystickHatDirections[((i - 2*joystick.numButtons)>>1) % 4]);
        }
    }
    for (i = 0; i < joystick.numAxes; ++i)
    {
        ME_JOYSTICKAXES[i] = ME_JOYSTICKAXES_TEMPLATE;
        Bstrncpy(MenuJoystickAxes[i], joyGetName(0, i), MAXJOYBUTTONSTRINGLENGTH);
        ME_JOYSTICKAXES[i].name = MenuJoystickAxes[i];
        MEL_JOYSTICKAXES[i] = &ME_JOYSTICKAXES[i];
    }
    M_JOYSTICKAXES.numEntries = joystick.numAxes;

    // prepare video setup
    for (i = 0; i < validmodecnt; ++i)
    {
        for (j = 0; j < MEOS_VIDEOSETUP_RESOLUTION.numOptions; ++j)
        {
            if (validmode[i].xdim == resolution[j].xdim && validmode[i].ydim == resolution[j].ydim)
            {
                resolution[j].flags |= validmode[i].fs ? RES_FS : RES_WIN;
                Bsnprintf(resolution[j].name, MAXRESOLUTIONSTRINGLENGTH, "%d x %d%s", resolution[j].xdim, resolution[j].ydim, (resolution[j].flags & RES_FS) ? "" : "Win");
                MEOSN_VIDEOSETUP_RESOLUTION[j] = resolution[j].name;
                if (validmode[i].bpp > resolution[j].bppmax)
                    resolution[j].bppmax = validmode[i].bpp;
                break;
            }
        }

        if (j == MEOS_VIDEOSETUP_RESOLUTION.numOptions) // no match found
        {
            resolution[j].xdim = validmode[i].xdim;
            resolution[j].ydim = validmode[i].ydim;
            resolution[j].bppmax = validmode[i].bpp;
            resolution[j].flags = validmode[i].fs ? RES_FS : RES_WIN;
            Bsnprintf(resolution[j].name, MAXRESOLUTIONSTRINGLENGTH, "%d x %d%s", resolution[j].xdim, resolution[j].ydim, (resolution[j].flags & RES_FS) ? "" : "Win");
            MEOSN_VIDEOSETUP_RESOLUTION[j] = resolution[j].name;
            ++MEOS_VIDEOSETUP_RESOLUTION.numOptions;
        }
    }


    if (RR)
    {
        MF_Redfont.zoom = 32768;
        MF_Redfont.emptychar.x <<= 1;
        MF_Redfont.cursorScale = 13107;
        MF_Redfont.cursorScale2 = 6553;
        //MF_Redfont.emptychar.y <<= 1;
        MF_Bluefont.zoom = 32768;
        MF_Bluefont.emptychar.x <<= 1;
        MF_Bluefont.cursorScale = 6553;
        MF_Bluefont.cursorScale2 = 6553;
        //MF_Bluefont.emptychar.y <<= 1;
        MF_Minifont.zoom = 32768;
        MF_Minifont.emptychar.x <<= 1;
        MF_Minifont.cursorScale = 6553;
        MF_Minifont.cursorScale2 = 6553;
        //MF_Minifont.emptychar.y <<= 1;
        ME_SOUND_DUKETALK.name = "Leonard Talk:";
    }
    else if (NAPALM)
    {
        ME_SOUND_DUKETALK.name = "NAPALM Talk:";
    }
    else if (NAM)
    {
        ME_SOUND_DUKETALK.name = "NAM Talk:";
    }

    // prepare shareware
    if (VOLUMEONE)
    {
        // blue out episodes beyond the first
        for (i = 1; i < g_volumeCnt; ++i)
        {
            if (MEL_EPISODE[i])
            {
                ME_EPISODE[i].entry = &MEO_EPISODE_SHAREWARE;
                ME_EPISODE[i].flags |= MEF_LookDisabled;
            }
        }
        M_EPISODE.numEntries = g_volumeCnt; // remove User Map (and spacer)
        MEOS_NETOPTIONS_EPISODE.numOptions = 1;
        MenuEntry_DisableOnCondition(&ME_NETOPTIONS_EPISODE, 1);
    }

    if (RR)
    {
        MEL_SOUND[0] = &ME_SOUND_CDPLAYER;
        MEL_SOUND[1] = &ME_SOUND;
        M_CREDITS.title = M_CREDITS2.title = M_CREDITS3.title = s_Credits;
        M_CREDITS3.nextID = MENU_CREDITS4;
        M_STORY.previousID = M_STORY.nextID = MENU_F1HELP;
        if (RRRA)
        {
            M_CREDITS31.previousID = MENU_CREDITS30;
            M_STORY.previousID = MENU_F1HELP2;
            M_F1HELP.nextID = MENU_F1HELP2;
        }
        else
        {
            M_CREDITS31.previousID = MENU_CREDITS23;
            M_CREDITS23.nextID = MENU_CREDITS31;
        }
    }
    // prepare pre-Atomic
    else if (!VOLUMEALL || !PLUTOPAK)
    {
        // prepare credits
        M_CREDITS.title = M_CREDITS2.title = M_CREDITS3.title = s_Credits;
    }
}


static void Menu_PreDraw(MenuID_t cm, MenuEntry_t *entry, const vec2_t origin)
{
    int32_t i, j, l = 0;

    switch (cm)
    {
    case MENU_MAIN_INGAME:
        l += 4;
        fallthrough__;
    case MENU_MAIN:
        if (RR)
        {
            if (RRRA)
                rotatesprite_fs(origin.x + ((MENU_MARGIN_CENTER-5)<<16), origin.y + ((57+l)<<16), 16592L,0,THREEDEE,0,0,10);
            else
                rotatesprite_fs(origin.x + ((MENU_MARGIN_CENTER+5)<<16), origin.y + ((24+l)<<16), 23592L,0,INGAMEDUKETHREEDEE,0,0,10);
        }
        else
        {
            rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + ((28+l)<<16), 65536L,0,INGAMEDUKETHREEDEE,0,0,10);
            if (PLUTOPAK)   // JBF 20030804
                rotatesprite_fs(origin.x + ((MENU_MARGIN_CENTER+100)<<16), origin.y + (36<<16), 65536L,0,PLUTOPAKSPRITE+2,(sintable[((int32_t) totalclock<<4)&2047]>>11),0,2+8);
        }
        break;

    case MENU_CDPLAYER:
        rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y+(100<<16),32768L,0,CDPLAYER,16,0,10);
        break;

    case MENU_PLAYER:
        if (RR)
            rotatesprite_fs(origin.x + (260<<16), origin.y + ((24+(tilesiz[APLAYER].y>>2))<<16), 24576L,0,3845+36-((((8-((int32_t) totalclock>>4)))&7)*5),0,entry == &ME_PLAYER_TEAM ? G_GetTeamPalette(playerteam) : playercolor,10);
        else
            rotatesprite_fs(origin.x + (260<<16), origin.y + ((24+(tilesiz[APLAYER].y>>1))<<16), 49152L,0,1441-((((4-((int32_t) totalclock>>4)))&3)*5),0,entry == &ME_PLAYER_TEAM ? G_GetTeamPalette(playerteam) : playercolor,10);
        break;

    case MENU_MACROS:
        mgametextcenter(origin.x, origin.y + (144<<16), "Activate in-game with Shift-F#");
        break;

    case MENU_COLCORR:
    case MENU_COLCORR_INGAME:
        // center panel
        if (!RR)
            rotatesprite_fs(origin.x + (120<<16), origin.y + (32<<16), 16384, 0, 3290, 0, 0, 2|8|16);
        rotatesprite_fs(origin.x + (160<<16) - (tilesiz[BOTTOMSTATUSBAR].x<<(RR ? 12 : 13)), origin.y + (82<<16) - (tilesiz[BOTTOMSTATUSBAR].y<<14), RR ? 8192 : 16384, 0, BOTTOMSTATUSBAR, 0, 0, 2|8|16);

        // left panel
        rotatesprite_fs(origin.x + (40<<16), origin.y + (32<<16), 16384, 0, RR ? RRTILE403 : BONUSSCREEN, 0, 0, 2|8|16);

        // right panel
        rotatesprite_fs(origin.x + (200<<16), origin.y + (32<<16), 16384, 0, LOADSCREEN, 0, 0, 2|8|16);
        break;

    case MENU_NETSETUP:
    case MENU_NETHOST:
        mminitext(origin.x + (90<<16), origin.y + (90<<16), "Game Type", MF_Minifont.pal_deselected);
        mminitext(origin.x + (90<<16), origin.y + ((90+8)<<16), "Episode", MF_Minifont.pal_deselected);
        mminitext(origin.x + (90<<16), origin.y + ((90+8+8)<<16), "Level", MF_Minifont.pal_deselected);
        mminitext(origin.x + (90<<16), origin.y + ((90+8+8+8)<<16), ME_NETOPTIONS_MONSTERS.name, MF_Minifont.pal_deselected);
        if (m_coop == 0)
            mminitext(origin.x + (90<<16), origin.y + ((90+8+8+8+8)<<16), "Markers", MF_Minifont.pal_deselected);
        else if (m_coop == 1)
            mminitext(origin.x + (90<<16), origin.y + ((90+8+8+8+8)<<16), "Friendly Fire", MF_Minifont.pal_deselected);
        mminitext(origin.x + (90<<16), origin.y + ((90+8+8+8+8+8)<<16), "User Map", MF_Minifont.pal_deselected);

        mminitext(origin.x + ((90+60)<<16), origin.y + (90<<16), g_gametypeNames[m_coop], MF_Minifont.pal_deselected_right);

        mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8)<<16), gVolumeNames[ud.m_volume_number], MF_Minifont.pal_deselected_right);
        mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8)<<16), g_mapInfo[MAXLEVELS*ud.m_volume_number+m_level_number].name, MF_Minifont.pal_deselected_right);
        if (ud.m_monsters_off == 0 || ud.m_player_skill > 0)
            mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8+8)<<16), gSkillNames[ud.m_player_skill], MF_Minifont.pal_deselected_right);
        else mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8+8)<<16), "None", MF_Minifont.pal_deselected_right);
        if (m_coop == 0)
        {
            if (m_marker) mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8+8+8)<<16), "On", MF_Minifont.pal_deselected_right);
            else mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8+8+8)<<16), "Off", MF_Minifont.pal_deselected_right);
        }
        else if (m_coop == 1)
        {
            if (m_ffire) mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8+8+8)<<16), "On", MF_Minifont.pal_deselected_right);
            else mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8+8+8)<<16), "Off", MF_Minifont.pal_deselected_right);
        }
        break;

    case MENU_MOUSEADVANCED:
        break;

    case MENU_RESETPLAYER:
        videoFadeToBlack(1);
        Bsprintf(tempbuf, "Load last game:\n\"%s\""
#ifndef EDUKE32_ANDROID_MENU
                          "\n(Y/N)"
#endif
        , g_quickload->name);
        mgametextcenter(origin.x, origin.y + (90<<16), tempbuf);
        break;


    case MENU_NEWVERIFY:
        videoFadeToBlack(1);
        mgametextcenter(origin.x, origin.y + (90<<16), "Abort this game?"
#ifndef EDUKE32_ANDROID_MENU
                                                       "\n(Y/N)"
#endif
        );
        break;

    case MENU_QUIT:
    case MENU_QUIT_INGAME:
        videoFadeToBlack(1);
        mgametextcenter(origin.x, origin.y + (90<<16), "Are you sure you want to quit?"
#ifndef EDUKE32_ANDROID_MENU
                                                       "\n(Y/N)"
#endif
        );
        break;

    case MENU_QUITTOTITLE:
        videoFadeToBlack(1);
        mgametextcenter(origin.x, origin.y + (90<<16), "End game and return to title screen?"
#ifndef EDUKE32_ANDROID_MENU
                                                       "\n(Y/N)"
#endif
        );
        break;

    case MENU_NETWAITMASTER:
        G_DrawFrags();
        mgametextcenter(origin.x, origin.y + (50<<16), "Waiting for master\n"
                                                       "to select level");
        break;

    case MENU_NETWAITVOTES:
        G_DrawFrags();
        mgametextcenter(origin.x, origin.y + (90<<16), "Waiting for votes");
        break;

    case MENU_BUYDUKE:
        mgametextcenter(origin.x, origin.y + (33<<16), "You are playing the shareware\n"
                                                       "version of Duke Nukem 3D.  While\n"
                                                       "this version is really cool, you\n"
                                                       "are missing over 75% of the total\n"
                                                       "game, along with other great extras\n"
                                                       "which you'll get when you order\n"
                                                       "the complete version and get\n"
                                                       "the final three episodes.");

        mgametextcenter(origin.x, origin.y + ((148+16)<<16), "Press any key or button...");
        break;


    case MENU_CREDITS31:
        l = 7;

        mgametextcenter(origin.x, origin.y + ((55-l)<<16), "Developer");
        creditsminitext(origin.x + (160<<16), origin.y + ((60+10-l)<<16), "Alexey \"Nuke.YKT\" Skrybykin", 8);

        mgametextcenter(origin.x, origin.y + ((85-l)<<16), "Tester & support");
        creditsminitext(origin.x + (160<<16), origin.y + ((90+10-l)<<16), "Sergey \"Maxi Clouds\" Skrybykin", 8);

        mgametextcenter(origin.x, origin.y + ((115-l)<<16), "Special thanks to");
        creditsminitext(origin.x + (160<<16), origin.y + ((120+10-l)<<16), "Evan \"Hendricks266\" Ramos", 8);
        creditsminitext(origin.x + (160<<16), origin.y + ((120+20-l)<<16), "Richard \"TerminX\" Gobeille", 8);
        creditsminitext(origin.x + (160<<16), origin.y + ((120+30-l)<<16), "\"NY00123\"", 8);
        creditsminitext(origin.x + (160<<16), origin.y + ((120+40-l)<<16), "\"MetHy\"", 8);

        break;

    case MENU_CREDITS32:   // JBF 20031220
        l = 7;

        mgametextcenter(origin.x, origin.y + ((50-l)<<16), "Developers");
        creditsminitext(origin.x + (160<<16), origin.y + ((50+10-l)<<16), "Richard \"TerminX\" Gobeille", 8);
        creditsminitext(origin.x + (160<<16), origin.y + ((50+7+10-l)<<16), "Evan \"Hendricks266\" Ramos", 8);

        mgametextcenter(origin.x, origin.y + ((80-l)<<16), "Retired developers");
        creditsminitext(origin.x + (160<<16), origin.y + ((80+10-l)<<16), "Pierre-Loup \"Plagman\" Griffais", 8);
        creditsminitext(origin.x + (160<<16), origin.y + ((80+7+10-l)<<16), "Philipp \"Helixhorned\" Kutin", 8);

        mgametextcenter(origin.x, origin.y + ((130+7-l)<<16), "Special thanks to");
        creditsminitext(origin.x + (160<<16), origin.y + ((130+7+10-l)<<16), "Jonathon \"JonoF\" Fowler", 8);

        mgametextcenter(origin.x, origin.y + ((150+7-l)<<16), "Uses BUILD Engine technology by");
        creditsminitext(origin.x + (160<<16), origin.y + ((150+7+10-l)<<16), "Ken \"Awesoken\" Silverman", 8);


        break;

    case MENU_CREDITS33:
        l = 7;

        mgametextcenter(origin.x, origin.y + ((38-l)<<16), "License and Other Contributors");
        {
            static const char *header[] =
            {
                "This program is distributed under the terms of the",
                "GNU General Public License version 2 as published by the",
                "Free Software Foundation. See gpl-2.0.txt for details.",
                "BUILD engine technology available under license. See buildlic.txt.",
                nullptr,
                "The EDuke32 team thanks the following people for their contributions:",
                nullptr,
            };
            static const char *body[] =
            {
                "Alex Dawson",       // "pogokeen" - Polymost2, renderer work, bugfixes
                "Bioman",            // GTK work, APT repository and package upkeep
                "Brandon Bergren",   // "Bdragon" - tiles.cfg
                "Charlie Honig",     // "CONAN" - showview command
                "Dan Gaskill",       // "DeeperThought" - testing
                "David Koenig",      // "Bargle" - Merged a couple of things from duke3d_w32
                "Ed Coolidge",       // Mapster32 improvements
                "Emile Belanger",    // original Android work
                "Fox",               // various patches
                "Hunter_rus",        // tons of stuff
                "James Bentler",     // Mapster32 improvements
                "Jasper Foreman",    // netcode contributions
                "Javier Martinez",   // "Malone3D" - EDuke 2.1.1 components
                "Jeff Hart",         // website graphics
                "Jonathan Strander", // "Mblackwell" - testing and feature speccing
                "Jordon Moss",       // "Striker" - various patches, OldMP work
                "Jose del Castillo", // "Renegado" - EDuke 2.1.1 components
                "Lachlan McDonald",  // official EDuke32 icon
                "LSDNinja",          // OS X help and testing
                "Marcus Herbert",    // "rhoenie" - OS X compatibility work
                "Matthew Palmer",    // "Usurper" - testing and eduke32.com domain
                "Matt Saettler",     // original DOS EDuke/WW2GI enhancements
                "NY00123",           // Linux / SDL usability patches
                "Ozkan Sezer",       // SDL/GTK version checking improvements
                "Peter Green",       // "Plugwash" - dynamic remapping, custom gametypes
                "Peter Veenstra",    // "Qbix" - port to 64-bit
                "Robin Green",       // CON array support
                "Ryan Gordon",       // "icculus" - icculus.org Duke3D port sound code
                "Stephen Anthony",   // early 64-bit porting work
                "tueidj",            // Wii port
            };
            EDUKE32_STATIC_ASSERT(ARRAY_SIZE(body) % 3 == 0);
            static const char *footer[] =
            {
                nullptr,
                "Visit eduke32.com for news and updates",
            };

            static constexpr int header_numlines = ARRAY_SIZE(header);
            static constexpr int body_numlines   = ARRAY_SIZE(body);
            static constexpr int footer_numlines = ARRAY_SIZE(footer);

            static constexpr int CCOLUMNS = 3;
            static constexpr int CCOLXBUF = 20;

            int c;
            i = 0;
            for (c = 0; c < header_numlines; c++)
                if (header[c])
                    creditsminitext(origin.x + (160<<16), origin.y + ((17+10+10+8+4+(c*7)-l)<<16), header[c], 8);
            i += c;
            for (c = 0; c < body_numlines; c++)
                if (body[c])
                    creditsminitext(origin.x + ((CCOLXBUF+((320-CCOLXBUF*2)/(CCOLUMNS*2)) +((320-CCOLXBUF*2)/CCOLUMNS)*(c/(body_numlines/CCOLUMNS)))<<16), origin.y + ((17+10+10+8+4+((c%(body_numlines/CCOLUMNS))*7)+(i*7)-l)<<16), body[c], 8);
            i += c/CCOLUMNS;
            for (c = 0; c < footer_numlines; c++)
                if (footer[c])
                    creditsminitext(origin.x + (160<<16), origin.y + ((17+10+10+8+4+(c*7)+(i*7)-l)<<16), footer[c], 8);
        }

        break;

    default:
        break;
    }
}


#endif

END_RR_NS
