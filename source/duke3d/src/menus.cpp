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

#include "compat.h"
#include "duke3d.h"
#include "osdcmds.h"
#include "savegame.h"
#include "demo.h"
#include "xxhash.h"
#include "input.h"
#include "menus.h"
#include "cheats.h"

#include "in_android.h"
#ifndef __ANDROID__
droidinput_t droidinput;
#endif

// common positions
#define MENU_MARGIN_REGULAR 40
#define MENU_MARGIN_WIDE    32
#define MENU_MARGIN_CENTER  160
#define MENU_HEIGHT_CENTER  100

int32_t g_skillSoundVoice = -1;

extern int32_t voting;

#define USERMAPENTRYLENGTH 25

static FORCE_INLINE void WithSDL2_StartTextInput()
{
#if defined EDUKE32_TOUCH_DEVICES && defined SDL_MAJOR_VERSION && SDL_MAJOR_VERSION > 1
# if defined __ANDROID__
    AndroidShowKeyboard(1);
# else
    SDL_StartTextInput();
# endif
#endif
}

static FORCE_INLINE void WithSDL2_StopTextInput()
{
#if defined EDUKE32_TOUCH_DEVICES && defined SDL_MAJOR_VERSION && SDL_MAJOR_VERSION > 1
# if defined __ANDROID__
    AndroidShowKeyboard(0);
# else
    SDL_StopTextInput();
# endif
#endif
}

static FORCE_INLINE void rotatesprite_ybounds(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum, int8_t dashade, char dapalnum, int32_t dastat, int32_t ydim_upper, int32_t ydim_lower)
{
    rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, 0, 0, 0, ydim_upper, xdim-1, ydim_lower);
}

#ifndef EDUKE32_STANDALONE
static void mgametext(int32_t x, int32_t y, char const * t)
{
    G_ScreenText(MF_Bluefont.tilenum, x, y, MF_Bluefont.zoom, 0, 0, t, 0, MF_Bluefont.pal, 2|8|16|ROTATESPRITE_FULL16, 0, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, MF_Bluefont.between.x, MF_Bluefont.between.y, MF_Bluefont.textflags, 0, 0, xdim-1, ydim-1);
}
#endif
static vec2_t mgametextcenter(int32_t x, int32_t y, char const * t, int32_t f = 0)
{
    return G_ScreenText(MF_Bluefont.tilenum, (MENU_MARGIN_CENTER<<16) + x, y, MF_Bluefont.zoom, 0, 0, t, 0, MF_Bluefont.pal, 2|8|16|ROTATESPRITE_FULL16, 0, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, MF_Bluefont.between.x, MF_Bluefont.between.y, MF_Bluefont.textflags|f|TEXT_XCENTER, 0, 0, xdim-1, ydim-1);
}

#define mminitext(x,y,t,p) minitext_(x, y, t, 0, p, 2|8|16|ROTATESPRITE_FULL16)
#define mmenutext menutext

#ifndef EDUKE32_STANDALONE
static void shadowminitext(int32_t x, int32_t y, const char *t, int32_t p)
{
    int32_t f = 0;

    if (!minitext_lowercase)
        f |= TEXT_UPPERCASE;

    G_ScreenTextShadow(1, 1, MINIFONT, x, y, 65536, 0, 0, t, 0, p, 2|8|16|ROTATESPRITE_FULL16, 0, 4<<16, 8<<16, 1<<16, 0, f, 0, 0, xdim-1, ydim-1);
}
#endif
static void creditsminitext(int32_t x, int32_t y, const char *t, int32_t p)
{
    int32_t f = TEXT_XCENTER;

    if (!minitext_lowercase)
        f |= TEXT_UPPERCASE;

    G_ScreenTextShadow(1, 1, MINIFONT, x, y, 65536, 0, 0, t, 0, p, 2|8|16|ROTATESPRITE_FULL16, 0, 4<<16, 8<<16, 1<<16, 0, f, 0, 0, xdim-1, ydim-1);
}

#pragma pack(push,1)
static savehead_t savehead;
#pragma pack(pop)

static void Menu_DrawBackground(const vec2_t origin)
{
    rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + (100<<16), 65536L,0,MENUSCREEN,16,0,10+64);
}

static void Menu_DrawTopBar(const vec2_t origin)
{
    if ((G_GetLogoFlags() & LOGO_NOTITLEBAR) == 0)
        rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + (19<<16), MF_Redfont.cursorScale, 0,MENUBAR,16,0,10);
}

static void Menu_DrawTopBarCaption(const char *caption, const vec2_t origin)
{
    char *s = Bstrdup(caption), p = Bstrlen(caption)-1;
    if (s[p] == ':') s[p] = 0;
    menutext_(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + (24<<16) + (15<<15), 0, s, 10|16, TEXT_XCENTER|TEXT_YCENTER);
    Bfree(s);
}

static FORCE_INLINE int32_t Menu_CursorShade(void)
{
    return 4-(sintable[(totalclock<<4)&2047]>>11);
}
static void Menu_DrawCursorCommon(int32_t x, int32_t y, int32_t z, int32_t picnum, int32_t ydim_upper = 0, int32_t ydim_lower = ydim-1)
{
    rotatesprite_(x, y, z, 0, picnum, Menu_CursorShade(), 0, 2|8, 0, 0, 0, ydim_upper, xdim-1, ydim_lower);
}
static void Menu_DrawCursorLeft(int32_t x, int32_t y, int32_t z)
{
    if (KXDWN) return;
    Menu_DrawCursorCommon(x, y, z, SPINNINGNUKEICON+((totalclock>>3)%7));
}
static void Menu_DrawCursorRight(int32_t x, int32_t y, int32_t z)
{
    if (KXDWN) return;
    Menu_DrawCursorCommon(x, y, z, SPINNINGNUKEICON+6-((6+(totalclock>>3))%7));
}
static void Menu_DrawCursorTextTile(int32_t x, int32_t y, int32_t h, int32_t picnum, vec2s_t const & siz, int32_t ydim_upper = 0, int32_t ydim_lower = ydim-1)
{
    vec2_t const adjsiz = { siz.x<<15, siz.y<<16 };
    Menu_DrawCursorCommon(x + scale(adjsiz.x, h, adjsiz.y), y, divscale16(h, adjsiz.y), picnum, ydim_upper, ydim_lower);
}
static void Menu_DrawCursorText(int32_t x, int32_t y, int32_t h, int32_t ydim_upper = 0, int32_t ydim_lower = ydim-1)
{
    vec2s_t const & siz = tilesiz[SPINNINGNUKEICON];

    if (KXDWN || siz.x == 0)
    {
        Menu_DrawCursorTextTile(x, y, h, SMALLFNTCURSOR, tilesiz[SMALLFNTCURSOR], ydim_upper, ydim_lower);
        return;
    }

    Menu_DrawCursorTextTile(x, y, h, SPINNINGNUKEICON+((totalclock>>3)%7), siz, ydim_upper, ydim_lower);
}

extern int32_t g_quitDeadline;













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
MenuFont_t MF_Redfont =          { { 5<<16, 15<<16 }, {        0,     0 }, 65536, 20<<16, 110<<16, 65536, TEXT_BIGALPHANUM | TEXT_UPPERCASE, -1, 10,  0,  1 };
MenuFont_t MF_Bluefont =         { { 5<<16,  7<<16 }, { -(1<<16), 2<<16 }, 65536, 10<<16, 110<<16, 32768, 0, -1, 10,  0, 16 };
MenuFont_t MF_BluefontRed =      { { 5<<16,  7<<16 }, { -(1<<16), 2<<16 }, 65536, 10<<16, 110<<16, 32768, 0, -1, 10, 10, 16 };
MenuFont_t MF_BluefontGame =     { { 5<<16,  7<<16 }, {        0,     0 }, 65536, 10<<16, 110<<16, 32768, 0, -1, 10,  0, 16 };
static MenuFont_t MF_Minifont =         { { 4<<16,  5<<16 }, {    1<<16, 1<<16 }, 65536, 10<<16, 110<<16, 32768, 0, -1, 10,  0, 16 };
static MenuFont_t MF_MinifontRed =      { { 4<<16,  5<<16 }, {    1<<16, 1<<16 }, 65536, 10<<16, 110<<16, 32768, 0, -1, 16, 21, 16 };
MenuFont_t MF_MinifontSave     = { { 4<<16,  5<<16 }, {    1<<16, 1<<16 }, 65536, 10<<16, 110<<16, 32768, 0, -1, 10,  0, 13 };


static MenuMenuFormat_t MMF_Top_Main =             { {  MENU_MARGIN_CENTER<<16, 55<<16, }, -(170<<16) };
static MenuMenuFormat_t MMF_Top_Episode =          { {  MENU_MARGIN_CENTER<<16, 48<<16, }, -(190<<16) };
static MenuMenuFormat_t MMF_Top_Skill =            { {  MENU_MARGIN_CENTER<<16, 58<<16, }, -(190<<16) };
static MenuMenuFormat_t MMF_Top_Options =          { {  MENU_MARGIN_CENTER<<16, 38<<16, }, -(190<<16) };
static MenuMenuFormat_t MMF_Top_Joystick_Network = { {  MENU_MARGIN_CENTER<<16, 70<<16, }, -(190<<16) };
static MenuMenuFormat_t MMF_BigOptions =           { {    MENU_MARGIN_WIDE<<16, 38<<16, }, -(190<<16) };
static MenuMenuFormat_t MMF_SmallOptions =         { {    MENU_MARGIN_WIDE<<16, 37<<16, },    170<<16 };
static MenuMenuFormat_t MMF_Macros =               { {                  26<<16, 40<<16, },    160<<16 };
static MenuMenuFormat_t MMF_SmallOptionsNarrow  =  { { MENU_MARGIN_REGULAR<<16, 38<<16, }, -(190<<16) };
static MenuMenuFormat_t MMF_KeyboardSetupFuncs =   { {                  70<<16, 34<<16, },    151<<16 };
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
static MenuEntryFormat_t MEF_BigOptions_Apply = { 4<<16, 16<<16, -(244<<16) };
static MenuEntryFormat_t MEF_BigOptionsRt =     { 4<<16,      0, -(260<<16) };
#if defined USE_OPENGL || !defined EDUKE32_ANDROID_MENU
static MenuEntryFormat_t MEF_SmallOptions =     { 1<<16,      0,    216<<16 };
#endif
static MenuEntryFormat_t MEF_BigCheats =        { 3<<16,      0, -(260<<16) };
static MenuEntryFormat_t MEF_Cheats =           { 2<<16,      0, -(260<<16) };
static MenuEntryFormat_t MEF_PlayerNarrow =     { 1<<16,      0,     90<<16 };
static MenuEntryFormat_t MEF_Macros =           { 2<<16,     -1,    268<<16 };
static MenuEntryFormat_t MEF_VideoSetup =       { 4<<16,      0,    168<<16 };
static MenuEntryFormat_t MEF_VideoSetup_Apply = { 4<<16, 16<<16,    168<<16 };
static MenuEntryFormat_t MEF_FuncList =         { 3<<16,      0,    100<<16 };
static MenuEntryFormat_t MEF_ColorCorrect =     { 2<<16,      0, -(240<<16) };
static MenuEntryFormat_t MEF_BigSliders =       { 2<<16,      0,    170<<16 };
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


static char MenuGameFuncs[NUMGAMEFUNCTIONS][MAXGAMEFUNCLEN];
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
MAKE_MENU_TOP_ENTRYLINK( "Help", MEF_MainMenu, MAIN_HELP, MENU_STORY );
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

#if defined STARTUP_SETUP_WINDOW && !defined EDUKE32_SIMPLE_MENU
static MenuOption_t MEO_GAMESETUP_STARTWIN = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OffOn, &ud.config.ForceSetup );
static MenuEntry_t ME_GAMESETUP_STARTWIN = MAKE_MENUENTRY( "Startup window:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_STARTWIN, Option );
#endif

static char const *MEOSN_GAMESETUP_AIM_AUTO[] = { "Never", "Always", "Hitscan only",
#ifdef EDUKE32_ANDROID_MENU
"Extra wide"
#endif
};
static int32_t MEOSV_GAMESETUP_AIM_AUTO[] = { 0, 1, 2,
#ifdef EDUKE32_ANDROID_MENU
3,
#endif
};

static MenuOptionSet_t MEOS_GAMESETUP_AIM_AUTO = MAKE_MENUOPTIONSET( MEOSN_GAMESETUP_AIM_AUTO, MEOSV_GAMESETUP_AIM_AUTO, 0x2 );
static MenuOption_t MEO_GAMESETUP_AIM_AUTO = MAKE_MENUOPTION( &MF_Redfont, &MEOS_GAMESETUP_AIM_AUTO, &ud.config.AutoAim );
static MenuEntry_t ME_GAMESETUP_AIM_AUTO = MAKE_MENUENTRY( "Auto aim:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_AIM_AUTO, Option );

static char const *MEOSN_GAMESETUP_WEAPSWITCH_PICKUP[] = { "Never", "If new", "By rating", };
static MenuOptionSet_t MEOS_GAMESETUP_WEAPSWITCH_PICKUP = MAKE_MENUOPTIONSET( MEOSN_GAMESETUP_WEAPSWITCH_PICKUP, NULL, 0x2 );
static MenuOption_t MEO_GAMESETUP_WEAPSWITCH_PICKUP = MAKE_MENUOPTION( &MF_Redfont, &MEOS_GAMESETUP_WEAPSWITCH_PICKUP, NULL );
static MenuEntry_t ME_GAMESETUP_WEAPSWITCH_PICKUP = MAKE_MENUENTRY( "Equip pickups:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_WEAPSWITCH_PICKUP, Option );

static char const *MEOSN_DemoRec[] = { "Off", "Running", };
static MenuOptionSet_t MEOS_DemoRec = MAKE_MENUOPTIONSET( MEOSN_DemoRec, NULL, 0x3 );
static MenuOption_t MEO_GAMESETUP_DEMOREC = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OffOn, &ud.m_recstat );
static MenuEntry_t ME_GAMESETUP_DEMOREC = MAKE_MENUENTRY( "Record demo:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_DEMOREC, Option );

#ifdef _WIN32
static MenuOption_t MEO_GAMESETUP_UPDATES = MAKE_MENUOPTION( &MF_Redfont, &MEOS_NoYes, &ud.config.CheckForUpdates );
static MenuEntry_t ME_GAMESETUP_UPDATES = MAKE_MENUENTRY( "Online updates:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_UPDATES, Option );
#endif

static MenuOption_t MEO_ADULTMODE = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &ud.lockout);
static MenuEntry_t ME_ADULTMODE = MAKE_MENUENTRY( "Parental lock:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_ADULTMODE, Option );
// static MenuLink_t MEO_ADULTMODE_PASSWORD = { MENU_ADULTPASSWORD, MA_None, };
// static MenuEntry_t ME_ADULTMODE_PASSWORD = MAKE_MENUENTRY( "Enter Password", &MF_Redfont, &, &MEO_ADULTMODE_PASSWORD, Link );

#ifdef EDUKE32_ANDROID_MENU
static MenuOption_t MEO_GAMESETUP_CROUCHLOCK = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &droidinput.toggleCrouch);
static MenuEntry_t ME_GAMESETUP_CROUCHLOCK = MAKE_MENUENTRY("Crouch lock:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_CROUCHLOCK, Option);

static MenuOption_t MEO_GAMESETUP_QUICKSWITCH = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &droidinput.quickSelectWeapon);
static MenuEntry_t ME_GAMESETUP_QUICKSWITCH = MAKE_MENUENTRY("Quick weapon switch:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_QUICKSWITCH, Option);
#endif

#if defined(EDUKE32_ANDROID_MENU) || !defined(EDUKE32_SIMPLE_MENU)
static MenuLink_t MEO_GAMESETUP_CHEATS = { MENU_CHEATS, MA_Advance, };
static MenuEntry_t ME_GAMESETUP_CHEATS = MAKE_MENUENTRY( "Cheats", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_CHEATS, Link );

static MenuEntry_t *MEL_GAMESETUP[] = {
    &ME_ADULTMODE,
#if defined STARTUP_SETUP_WINDOW && !defined EDUKE32_SIMPLE_MENU
    &ME_GAMESETUP_STARTWIN,
#endif
    &ME_GAMESETUP_AIM_AUTO,
    &ME_GAMESETUP_WEAPSWITCH_PICKUP,
#ifdef EDUKE32_ANDROID_MENU
    &ME_GAMESETUP_QUICKSWITCH,
    &ME_GAMESETUP_CROUCHLOCK,
#else
    &ME_GAMESETUP_DEMOREC,
#ifdef _WIN32
    &ME_GAMESETUP_UPDATES,
#endif
#endif
    &ME_GAMESETUP_CHEATS,
};
#endif

#ifndef EDUKE32_SIMPLE_MENU
MAKE_MENU_TOP_ENTRYLINK( "Game Setup", MEF_OptionsMenu, OPTIONS_GAMESETUP, MENU_GAMESETUP );
#endif
MAKE_MENU_TOP_ENTRYLINK( "Sound Setup", MEF_OptionsMenu, OPTIONS_SOUNDSETUP, MENU_SOUND );
MAKE_MENU_TOP_ENTRYLINK( "Display Setup", MEF_OptionsMenu, OPTIONS_DISPLAYSETUP, MENU_DISPLAYSETUP );
MAKE_MENU_TOP_ENTRYLINK( "Player Setup", MEF_OptionsMenu, OPTIONS_PLAYERSETUP, MENU_PLAYER );
#ifndef EDUKE32_ANDROID_MENU
MAKE_MENU_TOP_ENTRYLINK( "Control Setup", MEF_OptionsMenu, OPTIONS_CONTROLS, MENU_CONTROLS );

MAKE_MENU_TOP_ENTRYLINK( "Keyboard Setup", MEF_CenterMenu, OPTIONS_KEYBOARDSETUP, MENU_KEYBOARDSETUP );
MAKE_MENU_TOP_ENTRYLINK( "Mouse Setup", MEF_CenterMenu, OPTIONS_MOUSESETUP, MENU_MOUSESETUP );
#endif
MAKE_MENU_TOP_ENTRYLINK( "Joystick Setup", MEF_CenterMenu, OPTIONS_JOYSTICKSETUP, MENU_JOYSTICKSETUP );
#ifdef EDUKE32_ANDROID_MENU
MAKE_MENU_TOP_ENTRYLINK( "Touch Setup", MEF_CenterMenu, OPTIONS_TOUCHSETUP, MENU_TOUCHSETUP );
#endif
#ifdef EDUKE32_SIMPLE_MENU
MAKE_MENU_TOP_ENTRYLINK("Cheats", MEF_OptionsMenu, OPTIONS_CHEATS, MENU_CHEATS);
#endif

static int32_t newresolution, newrendermode, newfullscreen, newvsync;

enum resflags_t {
    RES_FS  = 0x1,
    RES_WIN = 0x2,
};

#define MAXRESOLUTIONSTRINGLENGTH 16

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
#ifdef POLYMER
static char const *MEOSN_VIDEOSETUP_RENDERER[] = { "Classic", "Polymost", "Polymer", };
static int32_t MEOSV_VIDEOSETUP_RENDERER[] = { REND_CLASSIC, REND_POLYMOST, REND_POLYMER, };
#else
static char const *MEOSN_VIDEOSETUP_RENDERER[] = { "Classic", "OpenGL", };
static int32_t MEOSV_VIDEOSETUP_RENDERER[] = { REND_CLASSIC, REND_POLYMOST, };
#endif

static MenuOptionSet_t MEOS_VIDEOSETUP_RENDERER = MAKE_MENUOPTIONSET( MEOSN_VIDEOSETUP_RENDERER, MEOSV_VIDEOSETUP_RENDERER, 0x2 );

static MenuOption_t MEO_VIDEOSETUP_RENDERER = MAKE_MENUOPTION( &MF_Redfont, &MEOS_VIDEOSETUP_RENDERER, &newrendermode );
static MenuEntry_t ME_VIDEOSETUP_RENDERER = MAKE_MENUENTRY( "Renderer:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_RENDERER, Option );
#endif

static MenuOption_t MEO_VIDEOSETUP_FULLSCREEN = MAKE_MENUOPTION( &MF_Redfont, &MEOS_NoYes, &newfullscreen );
static MenuEntry_t ME_VIDEOSETUP_FULLSCREEN = MAKE_MENUENTRY( "Fullscreen:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_FULLSCREEN, Option );


static char const *MEOSN_VIDEOSETUP_VSYNC [] = { "Adaptive", "Off", "On", };
static int32_t MEOSV_VIDEOSETUP_VSYNC [] = { -1, 0, 1, };
static MenuOptionSet_t MEOS_VIDEOSETUP_VSYNC = MAKE_MENUOPTIONSET(MEOSN_VIDEOSETUP_VSYNC, MEOSV_VIDEOSETUP_VSYNC, 0x2);
static MenuOption_t MEO_VIDEOSETUP_VSYNC = MAKE_MENUOPTION(&MF_Redfont, &MEOS_VIDEOSETUP_VSYNC, &newvsync);
static MenuEntry_t ME_VIDEOSETUP_VSYNC = MAKE_MENUENTRY("VSync:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_VSYNC, Option);

static char const *MEOSN_VIDEOSETUP_FRAMELIMIT [] = { "None", "30 fps", "60 fps", "120 fps", "144 fps", };
static int32_t MEOSV_VIDEOSETUP_FRAMELIMIT [] = { 0, 30, 60, 120, 144 };
static MenuOptionSet_t MEOS_VIDEOSETUP_FRAMELIMIT = MAKE_MENUOPTIONSET(MEOSN_VIDEOSETUP_FRAMELIMIT, MEOSV_VIDEOSETUP_FRAMELIMIT, 0x2);
static MenuOption_t MEO_VIDEOSETUP_FRAMELIMIT= MAKE_MENUOPTION(&MF_Redfont, &MEOS_VIDEOSETUP_FRAMELIMIT, &r_maxfps);
static MenuEntry_t ME_VIDEOSETUP_FRAMELIMIT = MAKE_MENUENTRY("Framerate limit:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_FRAMELIMIT, Option);


static MenuEntry_t ME_VIDEOSETUP_APPLY = MAKE_MENUENTRY( "Apply Changes", &MF_Redfont, &MEF_BigOptions_Apply, &MEO_NULL, Link );


static MenuLink_t MEO_DISPLAYSETUP_COLORCORR = { MENU_COLCORR, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_COLORCORR = MAKE_MENUENTRY( "Color Correction", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_COLORCORR, Link );


static MenuOption_t MEO_DISPLAYSETUP_PIXELDOUBLING = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OffOn, &ud.detail );
static MenuEntry_t ME_DISPLAYSETUP_PIXELDOUBLING = MAKE_MENUENTRY( "Pixel Doubling:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_PIXELDOUBLING, Option );


#ifndef EDUKE32_ANDROID_MENU
static MenuOption_t MEO_DISPLAYSETUP_ASPECTRATIO = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &r_usenewaspect);
static MenuEntry_t ME_DISPLAYSETUP_ASPECTRATIO = MAKE_MENUENTRY( "Widescreen:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_ASPECTRATIO, Option );
#endif


#ifdef USE_OPENGL
#ifdef EDUKE32_SIMPLE_MENU
static MenuOption_t MEO_DISPLAYSETUP_PALETTEEMULATION = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &r_usetileshades);
static MenuEntry_t ME_DISPLAYSETUP_PALETTEEMULATION = MAKE_MENUENTRY("Palette emulation:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_PALETTEEMULATION, Option);
#endif

static char const *MEOSN_DISPLAYSETUP_TEXFILTER[] = { "Classic", "Filtered" };
static int32_t MEOSV_DISPLAYSETUP_TEXFILTER[] = { TEXFILTER_OFF, TEXFILTER_ON };
static MenuOptionSet_t MEOS_DISPLAYSETUP_TEXFILTER = MAKE_MENUOPTIONSET( MEOSN_DISPLAYSETUP_TEXFILTER, MEOSV_DISPLAYSETUP_TEXFILTER, 0x2 );
static MenuOption_t MEO_DISPLAYSETUP_TEXFILTER = MAKE_MENUOPTION( &MF_Redfont, &MEOS_DISPLAYSETUP_TEXFILTER, &gltexfiltermode );
static MenuEntry_t ME_DISPLAYSETUP_TEXFILTER = MAKE_MENUENTRY( "Texture Mode:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_TEXFILTER, Option );

static char const *MEOSN_DISPLAYSETUP_ANISOTROPY[] = { "Max", "None", "2x", "4x", "8x", "16x", };
static int32_t MEOSV_DISPLAYSETUP_ANISOTROPY[] = { 0, 1, 2, 4, 8, 16, };
static MenuOptionSet_t MEOS_DISPLAYSETUP_ANISOTROPY = MAKE_MENUOPTIONSET( MEOSN_DISPLAYSETUP_ANISOTROPY, MEOSV_DISPLAYSETUP_ANISOTROPY, 0x0 );
static MenuOption_t MEO_DISPLAYSETUP_ANISOTROPY = MAKE_MENUOPTION(&MF_Redfont, &MEOS_DISPLAYSETUP_ANISOTROPY, &glanisotropy);
static MenuEntry_t ME_DISPLAYSETUP_ANISOTROPY = MAKE_MENUENTRY( "Anisotropy:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_ANISOTROPY, Option );

#ifdef EDUKE32_ANDROID_MENU
static MenuOption_t MEO_DISPLAYSETUP_HIDEDPAD = MAKE_MENUOPTION(&MF_Redfont, &MEOS_NoYes, &droidinput.hideStick);
static MenuEntry_t ME_DISPLAYSETUP_HIDEDPAD = MAKE_MENUENTRY("Hide touch D-pad:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_HIDEDPAD, Option);

static MenuRangeFloat_t MEO_DISPLAYSETUP_TOUCHALPHA = MAKE_MENURANGE(&droidinput.gameControlsAlpha, &MF_Redfont, 0, 1, 0, 16, 2);
static MenuEntry_t ME_DISPLAYSETUP_TOUCHALPHA = MAKE_MENUENTRY("UI opacity:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_TOUCHALPHA, RangeFloat);
#endif

#endif

static char const s_Scale[] = "Scale:";

static MenuOption_t MEO_SCREENSETUP_CROSSHAIR = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &ud.crosshair);
static MenuEntry_t ME_SCREENSETUP_CROSSHAIR = MAKE_MENUENTRY( "Crosshair:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SCREENSETUP_CROSSHAIR, Option );
static MenuRangeInt32_t MEO_SCREENSETUP_CROSSHAIRSIZE = MAKE_MENURANGE( &ud.crosshairscale, &MF_Redfont, 25, 100, 0, 16, 2 );
static MenuEntry_t ME_SCREENSETUP_CROSSHAIRSIZE = MAKE_MENUENTRY( s_Scale, &MF_Redfont, &MEF_BigOptions_Apply, &MEO_SCREENSETUP_CROSSHAIRSIZE, RangeInt32 );

static int32_t vpsize;
static MenuRangeInt32_t MEO_SCREENSETUP_SCREENSIZE = MAKE_MENURANGE( &vpsize, &MF_Redfont, 16, 0, 0, 5, EnforceIntervals );
static MenuEntry_t ME_SCREENSETUP_SCREENSIZE = MAKE_MENUENTRY( "Screen size:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SCREENSETUP_SCREENSIZE, RangeInt32 );
static MenuRangeInt32_t MEO_SCREENSETUP_TEXTSIZE = MAKE_MENURANGE( &ud.textscale, &MF_Redfont, 100, 400, 0, 16, 2 );
static MenuEntry_t ME_SCREENSETUP_TEXTSIZE = MAKE_MENUENTRY( s_Scale, &MF_Redfont, &MEF_BigOptions_Apply, &MEO_SCREENSETUP_TEXTSIZE, RangeInt32 );
static MenuOption_t MEO_SCREENSETUP_LEVELSTATS = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &ud.levelstats);
static MenuEntry_t ME_SCREENSETUP_LEVELSTATS = MAKE_MENUENTRY( "Level stats:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SCREENSETUP_LEVELSTATS, Option );


static MenuOption_t MEO_SCREENSETUP_SHOWPICKUPMESSAGES = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &ud.fta_on);
static MenuEntry_t ME_SCREENSETUP_SHOWPICKUPMESSAGES = MAKE_MENUENTRY( "Game messages:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SCREENSETUP_SHOWPICKUPMESSAGES, Option );



static MenuRangeInt32_t MEO_SCREENSETUP_SBARSIZE = MAKE_MENURANGE( &ud.statusbarscale, &MF_Redfont, 36, 100, 0, 17, 2 );
static MenuEntry_t ME_SCREENSETUP_SBARSIZE = MAKE_MENUENTRY( s_Scale, &MF_Redfont, &MEF_BigOptions_Apply, &MEO_SCREENSETUP_SBARSIZE, RangeInt32 );


static MenuLink_t MEO_DISPLAYSETUP_SCREENSETUP = { MENU_SCREENSETUP, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_SCREENSETUP = MAKE_MENUENTRY( "HUD setup", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_SCREENSETUP, Link );


#ifndef EDUKE32_SIMPLE_MENU
#ifdef USE_OPENGL
static MenuLink_t MEO_DISPLAYSETUP_ADVANCED_GL_POLYMOST = { MENU_POLYMOST, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_ADVANCED_GL_POLYMOST = MAKE_MENUENTRY( "Polymost setup", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_ADVANCED_GL_POLYMOST, Link );

#ifdef POLYMER
static MenuLink_t MEO_DISPLAYSETUP_ADVANCED_GL_POLYMER = { MENU_POLYMER, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_ADVANCED_GL_POLYMER = MAKE_MENUENTRY("Polymer setup", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_ADVANCED_GL_POLYMER, Link);
#endif
#endif
#endif

#ifndef EDUKE32_ANDROID_MENU
static MenuLink_t MEO_DISPLAYSETUP_VIDEOSETUP = { MENU_VIDEOSETUP, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_VIDEOSETUP = MAKE_MENUENTRY( "Video mode", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_VIDEOSETUP, Link );
#endif


static MenuLink_t MEO_ENTERCHEAT = { MENU_CHEATENTRY, MA_None, };
static MenuEntry_t ME_ENTERCHEAT = MAKE_MENUENTRY( "Enter Cheat Code", &MF_Redfont, &MEF_BigCheats, &MEO_ENTERCHEAT, Link );

static MenuLink_t MEO_CHEAT_WARP = { MENU_CHEAT_WARP, MA_None, };
static MenuLink_t MEO_CHEAT_SKILL = { MENU_CHEAT_SKILL, MA_None, };
// KEEPINSYNC game.h: enum CheatCodeFunctions
// KEEPINSYNC game.c: uint8_t CheatFunctionIDs[]
#define MAKE_MENUCHEAT( Name ) MAKE_MENUENTRY( Name, &MF_Bluefont, &MEF_Cheats, &MEO_NULL, Link )
static MenuEntry_t ME_CheatCodes[] = {
    MAKE_MENUCHEAT( "Toggle Cashman" ),
    MAKE_MENUCHEAT( "Toggle God Mode" ),
    MAKE_MENUCHEAT( "Give Everything" ),
    MAKE_MENUCHEAT( "Give Weapons" ),
    MAKE_MENUCHEAT( "Give All Items" ),
    MAKE_MENUCHEAT( "Give Inventory" ),
    MAKE_MENUCHEAT( "Give Keys" ),
    MAKE_MENUCHEAT( "Toggle Hyper" ),
    MAKE_MENUCHEAT( "Toggle 3rd-Person View" ),
    MAKE_MENUCHEAT( "Toggle Show All Map" ),
    MAKE_MENUCHEAT( "Toggle All Locks" ),
    MAKE_MENUCHEAT( "Toggle Clipping" ),
    MAKE_MENUENTRY( "Level Warp", &MF_Bluefont, &MEF_Cheats, &MEO_CHEAT_WARP, Link ),
    MAKE_MENUENTRY( "Change Skill", &MF_Bluefont, &MEF_Cheats, &MEO_CHEAT_SKILL, Link ),
    MAKE_MENUCHEAT( "Toggle Monsters" ),
    MAKE_MENUCHEAT( "Toggle Framerate Display" ),
    MAKE_MENUCHEAT( NULL ),
    MAKE_MENUCHEAT( NULL ),
    MAKE_MENUCHEAT( NULL ),
    MAKE_MENUCHEAT( "Toggle Coordinate Display" ),
    MAKE_MENUCHEAT( "Toggle Debug Data Dump" ),
};

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
    &ME_VIDEOSETUP_FRAMELIMIT,
    &ME_Space6_Redfont,
    &ME_VIDEOSETUP_APPLY,
};
static MenuEntry_t *MEL_DISPLAYSETUP[] = {
    &ME_DISPLAYSETUP_SCREENSETUP,
    &ME_DISPLAYSETUP_COLORCORR,
#ifndef EDUKE32_ANDROID_MENU
    &ME_DISPLAYSETUP_VIDEOSETUP,
    &ME_DISPLAYSETUP_ASPECTRATIO,
#endif
    &ME_DISPLAYSETUP_PIXELDOUBLING,
};

#ifdef USE_OPENGL
static MenuEntry_t *MEL_DISPLAYSETUP_GL[] = {
    &ME_DISPLAYSETUP_SCREENSETUP,
    &ME_DISPLAYSETUP_COLORCORR,
#ifndef EDUKE32_ANDROID_MENU
    &ME_DISPLAYSETUP_VIDEOSETUP,
    &ME_DISPLAYSETUP_ASPECTRATIO,
#endif
    &ME_DISPLAYSETUP_TEXFILTER,
#ifdef EDUKE32_ANDROID_MENU
    &ME_DISPLAYSETUP_HIDEDPAD,
    &ME_DISPLAYSETUP_TOUCHALPHA,
#else
    &ME_DISPLAYSETUP_ANISOTROPY,
#ifdef EDUKE32_SIMPLE_MENU
    &ME_DISPLAYSETUP_PALETTEEMULATION,
#else
    &ME_DISPLAYSETUP_ADVANCED_GL_POLYMOST,
#endif
#endif
};

#ifdef POLYMER
static MenuEntry_t *MEL_DISPLAYSETUP_GL_POLYMER[] = {
    &ME_DISPLAYSETUP_SCREENSETUP,
    &ME_DISPLAYSETUP_COLORCORR,
#ifndef EDUKE32_ANDROID_MENU
    &ME_DISPLAYSETUP_VIDEOSETUP,
#endif
    &ME_DISPLAYSETUP_TEXFILTER,
    &ME_DISPLAYSETUP_ANISOTROPY,
#ifndef EDUKE32_SIMPLE_MENU
    &ME_DISPLAYSETUP_ADVANCED_GL_POLYMER,
#endif
};

#endif
#endif



static char const *MenuKeyNone = "  -";
static char const *MEOSN_Keys[NUMKEYS];

static MenuCustom2Col_t MEO_KEYBOARDSETUPFUNCS_TEMPLATE = { { NULL, NULL, }, MEOSN_Keys, &MF_MinifontRed, NUMKEYS, 54<<16, 0 };
static MenuCustom2Col_t MEO_KEYBOARDSETUPFUNCS[NUMGAMEFUNCTIONS];
static MenuEntry_t ME_KEYBOARDSETUPFUNCS_TEMPLATE = MAKE_MENUENTRY( NULL, &MF_Minifont, &MEF_FuncList, &MEO_KEYBOARDSETUPFUNCS_TEMPLATE, Custom2Col );
static MenuEntry_t ME_KEYBOARDSETUPFUNCS[NUMGAMEFUNCTIONS];
static MenuEntry_t *MEL_KEYBOARDSETUPFUNCS[NUMGAMEFUNCTIONS];

static MenuLink_t MEO_KEYBOARDSETUP_KEYS = { MENU_KEYBOARDKEYS, MA_Advance, };
static MenuEntry_t ME_KEYBOARDSETUP_KEYS = MAKE_MENUENTRY( "Configure Keys", &MF_Redfont, &MEF_CenterMenu, &MEO_KEYBOARDSETUP_KEYS, Link );
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
static MenuEntry_t ME_MOUSESETUPBTNS[MENUMOUSEFUNCTIONS];
static MenuEntry_t *MEL_MOUSESETUPBTNS[MENUMOUSEFUNCTIONS];

static MenuLink_t MEO_MOUSESETUP_BTNS = { MENU_MOUSEBTNS, MA_Advance, };
static MenuEntry_t ME_MOUSESETUP_BTNS = MAKE_MENUENTRY( "Button assignment", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_BTNS, Link );
static MenuRangeFloat_t MEO_MOUSESETUP_SENSITIVITY = MAKE_MENURANGE( &CONTROL_MouseSensitivity, &MF_Redfont, .5f, 16.f, 0.f, 32, 1 );
static MenuEntry_t ME_MOUSESETUP_SENSITIVITY = MAKE_MENUENTRY( "Sensitivity:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_SENSITIVITY, RangeFloat );

#ifndef EDUKE32_SIMPLE_MENU
static char const *MEOSN_MOUSESETUP_AIM_TYPE [] = { "Toggle", "Hold" };
static MenuOptionSet_t MEOS_MOUSESETUP_AIM_TYPE = MAKE_MENUOPTIONSET(MEOSN_MOUSESETUP_AIM_TYPE, NULL, 0x2);
static MenuOption_t MEO_MOUSESETUP_MOUSEAIMINGTYPE = MAKE_MENUOPTION(&MF_Redfont, &MEOS_MOUSESETUP_AIM_TYPE, &ud.mouseaiming);
static MenuEntry_t ME_MOUSESETUP_MOUSEAIMINGTYPE = MAKE_MENUENTRY("Aiming type:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_MOUSEAIMINGTYPE, Option);
static MenuOption_t MEO_MOUSESETUP_MOUSEAIMING = MAKE_MENUOPTION( &MF_Redfont, &MEOS_NoYes, &g_myAimMode );
static MenuEntry_t ME_MOUSESETUP_MOUSEAIMING = MAKE_MENUENTRY( "Vertical aiming:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_MOUSEAIMING, Option );
#endif
static MenuOption_t MEO_MOUSESETUP_INVERT = MAKE_MENUOPTION( &MF_Redfont, &MEOS_YesNo, &ud.mouseflip );
static MenuEntry_t ME_MOUSESETUP_INVERT = MAKE_MENUENTRY( "Invert aiming:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_INVERT, Option );
static MenuOption_t MEO_MOUSESETUP_SMOOTH = MAKE_MENUOPTION( &MF_Redfont, &MEOS_NoYes, &ud.config.SmoothInput );
static MenuEntry_t ME_MOUSESETUP_SMOOTH = MAKE_MENUENTRY( "Filter input:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_SMOOTH, Option );
#ifndef EDUKE32_SIMPLE_MENU
static MenuLink_t MEO_MOUSESETUP_ADVANCED = { MENU_MOUSEADVANCED, MA_Advance, };
static MenuEntry_t ME_MOUSESETUP_ADVANCED = MAKE_MENUENTRY( "Advanced setup", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_ADVANCED, Link );
#endif
static MenuRangeInt32_t MEO_MOUSEADVANCED_SCALEX = MAKE_MENURANGE(&ud.config.MouseAnalogueScale[0], &MF_Redfont, 512, 65536, 65536, 63, 3);
static MenuEntry_t ME_MOUSEADVANCED_SCALEX = MAKE_MENUENTRY("X-Axis Scale:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSEADVANCED_SCALEX, RangeInt32);
static MenuRangeInt32_t MEO_MOUSEADVANCED_SCALEY = MAKE_MENURANGE(&ud.config.MouseAnalogueScale[1], &MF_Redfont, 512, 65536, 65536, 63, 3);
static MenuEntry_t ME_MOUSEADVANCED_SCALEY = MAKE_MENUENTRY("Y-Axis Scale:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSEADVANCED_SCALEY, RangeInt32);

static MenuEntry_t *MEL_MOUSESETUP[] = {
    &ME_MOUSESETUP_BTNS,
    &ME_MOUSESETUP_SENSITIVITY,
#ifdef EDUKE32_SIMPLE_MENU
    &ME_MOUSEADVANCED_SCALEX,
    &ME_MOUSEADVANCED_SCALEY,
#endif
    &ME_Space2_Redfont,
#ifdef EDUKE32_SIMPLE_MENU
    &ME_GAMESETUP_AIM_AUTO,
#endif
    &ME_MOUSESETUP_INVERT,
    &ME_MOUSESETUP_SMOOTH,
#ifndef EDUKE32_SIMPLE_MENU
    &ME_MOUSESETUP_MOUSEAIMINGTYPE,
    &ME_MOUSESETUP_MOUSEAIMING,
    &ME_MOUSESETUP_ADVANCED,
#endif
};

#ifdef EDUKE32_ANDROID_MENU
static MenuRangeFloat_t MEO_TOUCHSETUP_SENSITIVITY_MOVE = MAKE_MENURANGE(&droidinput.forward_sens, &MF_Redfont, 1.f, 9.f, 0.f, 17, 1 + EnforceIntervals);
static MenuEntry_t ME_TOUCHSETUP_SENSITIVITY_MOVE = MAKE_MENUENTRY("Running:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_TOUCHSETUP_SENSITIVITY_MOVE, RangeFloat);

static MenuRangeFloat_t MEO_TOUCHSETUP_SENSITIVITY_STRAFE = MAKE_MENURANGE(&droidinput.strafe_sens, &MF_Redfont, 1.f, 9.f, 0.f, 17, 1 + EnforceIntervals);
static MenuEntry_t ME_TOUCHSETUP_SENSITIVITY_STRAFE = MAKE_MENUENTRY("Strafing:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_TOUCHSETUP_SENSITIVITY_STRAFE, RangeFloat);

static MenuRangeFloat_t MEO_TOUCHSETUP_SENSITIVITY_LOOK = MAKE_MENURANGE(&droidinput.pitch_sens, &MF_Redfont, 1.f, 9.f, 0.f, 17, 1 + EnforceIntervals);
static MenuEntry_t ME_TOUCHSETUP_SENSITIVITY_LOOK = MAKE_MENUENTRY("Looking:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_TOUCHSETUP_SENSITIVITY_LOOK, RangeFloat);

static MenuRangeFloat_t MEO_TOUCHSETUP_SENSITIVITY_TURN = MAKE_MENURANGE(&droidinput.yaw_sens, &MF_Redfont, 1.f, 9.f, 0.f, 17, 1 + EnforceIntervals);
static MenuEntry_t ME_TOUCHSETUP_SENSITIVITY_TURN = MAKE_MENUENTRY("Turning:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_TOUCHSETUP_SENSITIVITY_TURN, RangeFloat);

static MenuOption_t MEO_TOUCHSETUP_INVERT = MAKE_MENUOPTION(&MF_Redfont, &MEOS_NoYes, &droidinput.invertLook);
static MenuEntry_t ME_TOUCHSETUP_INVERT = MAKE_MENUENTRY("Invert look:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_TOUCHSETUP_INVERT, Option);

MAKE_MENU_TOP_ENTRYLINK("Sensitivity", MEF_CenterMenu, TOUCHSENS, MENU_TOUCHSENS);
MAKE_MENU_TOP_ENTRYLINK("Button Setup", MEF_CenterMenu, TOUCHBUTTONS, MENU_TOUCHBUTTONS);

static MenuEntry_t *MEL_TOUCHSETUP [] = {
    &ME_TOUCHSENS,
    &ME_TOUCHBUTTONS,
};

static MenuEntry_t *MEL_TOUCHSENS [] = {
    &ME_TOUCHSETUP_SENSITIVITY_MOVE,
    &ME_TOUCHSETUP_SENSITIVITY_STRAFE,
    &ME_TOUCHSETUP_SENSITIVITY_LOOK,
    &ME_TOUCHSETUP_SENSITIVITY_TURN,
    &ME_Space2_Redfont,
    &ME_TOUCHSETUP_INVERT,
};
#endif

MAKE_MENU_TOP_ENTRYLINK( "Edit Buttons", MEF_CenterMenu, JOYSTICK_EDITBUTTONS, MENU_JOYSTICKBTNS );
MAKE_MENU_TOP_ENTRYLINK( "Edit Axes", MEF_CenterMenu, JOYSTICK_EDITAXES, MENU_JOYSTICKAXES );

static MenuEntry_t *MEL_JOYSTICKSETUP[] = {
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

static MenuOption_t MEO_MOUSEADVANCED_DAXES_UP = MAKE_MENUOPTION( &MF_BluefontRed, &MEOS_Gamefuncs, &ud.config.MouseDigitalFunctions[1][0] );
static MenuEntry_t ME_MOUSEADVANCED_DAXES_UP = MAKE_MENUENTRY( "Digital Up", &MF_Redfont, &MEF_BigSliders, &MEO_MOUSEADVANCED_DAXES_UP, Option );
static MenuOption_t MEO_MOUSEADVANCED_DAXES_DOWN = MAKE_MENUOPTION( &MF_BluefontRed, &MEOS_Gamefuncs, &ud.config.MouseDigitalFunctions[1][1] );
static MenuEntry_t ME_MOUSEADVANCED_DAXES_DOWN = MAKE_MENUENTRY( "Digital Down", &MF_Redfont, &MEF_BigSliders, &MEO_MOUSEADVANCED_DAXES_DOWN, Option );
static MenuOption_t MEO_MOUSEADVANCED_DAXES_LEFT = MAKE_MENUOPTION( &MF_BluefontRed, &MEOS_Gamefuncs, &ud.config.MouseDigitalFunctions[0][0] );
static MenuEntry_t ME_MOUSEADVANCED_DAXES_LEFT = MAKE_MENUENTRY( "Digital Left", &MF_Redfont, &MEF_BigSliders, &MEO_MOUSEADVANCED_DAXES_LEFT, Option );
static MenuOption_t MEO_MOUSEADVANCED_DAXES_RIGHT = MAKE_MENUOPTION( &MF_BluefontRed, &MEOS_Gamefuncs, &ud.config.MouseDigitalFunctions[0][1] );
static MenuEntry_t ME_MOUSEADVANCED_DAXES_RIGHT = MAKE_MENUENTRY( "Digital Right", &MF_Redfont, &MEF_BigSliders, &MEO_MOUSEADVANCED_DAXES_RIGHT, Option );

static MenuEntry_t *MEL_MOUSEADVANCED[] = {
    &ME_MOUSEADVANCED_SCALEX,
    &ME_MOUSEADVANCED_SCALEY,
    &ME_Space8_Redfont,
    &ME_MOUSEADVANCED_DAXES_UP,
    &ME_MOUSEADVANCED_DAXES_DOWN,
    &ME_MOUSEADVANCED_DAXES_LEFT,
    &ME_MOUSEADVANCED_DAXES_RIGHT,
};

static MenuEntry_t *MEL_INTERNAL_MOUSEADVANCED_DAXES[] = {
    &ME_MOUSEADVANCED_DAXES_UP,
    &ME_MOUSEADVANCED_DAXES_DOWN,
    &ME_MOUSEADVANCED_DAXES_LEFT,
    &ME_MOUSEADVANCED_DAXES_RIGHT,
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
static MenuEntry_t ME_JOYSTICKAXIS_DIGITALNEGATIVE = MAKE_MENUENTRY( "Digital -", &MF_BluefontRed, &MEF_BigSliders, &MEO_JOYSTICKAXIS_DIGITALNEGATIVE, Option );
static MenuOption_t MEO_JOYSTICKAXIS_DIGITALPOSITIVE = MAKE_MENUOPTION( &MF_Minifont, &MEOS_Gamefuncs, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_DIGITALPOSITIVE = MAKE_MENUENTRY( "Digital +", &MF_BluefontRed, &MEF_BigSliders, &MEO_JOYSTICKAXIS_DIGITALPOSITIVE, Option );

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
static MenuOption_t MEO_RENDERERSETUP_HIGHTILE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &usehightile );
static MenuEntry_t ME_RENDERERSETUP_HIGHTILE = MAKE_MENUENTRY( "True color textures:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_RENDERERSETUP_HIGHTILE, Option );

static char const *MEOSN_RENDERERSETUP_TEXQUALITY [] = { "Full", "Half", "Barf", };
static MenuOptionSet_t MEOS_RENDERERSETUP_TEXQUALITY = MAKE_MENUOPTIONSET(MEOSN_RENDERERSETUP_TEXQUALITY, NULL, 0x2);
static MenuOption_t MEO_RENDERERSETUP_TEXQUALITY = MAKE_MENUOPTION(&MF_Bluefont, &MEOS_RENDERERSETUP_TEXQUALITY, &r_downsize);
static MenuEntry_t ME_RENDERERSETUP_TEXQUALITY = MAKE_MENUENTRY("GL texture quality:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_RENDERERSETUP_TEXQUALITY, Option);


static MenuOption_t MEO_RENDERERSETUP_PRECACHE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.config.useprecache );
static MenuEntry_t ME_RENDERERSETUP_PRECACHE = MAKE_MENUENTRY( "Pre-load map textures:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_RENDERERSETUP_PRECACHE, Option );
# ifndef EDUKE32_GLES
static char const *MEOSN_RENDERERSETUP_TEXCACHE[] = { "Off", "On", "Compr.", };
static MenuOptionSet_t MEOS_RENDERERSETUP_TEXCACHE = MAKE_MENUOPTIONSET( MEOSN_RENDERERSETUP_TEXCACHE, NULL, 0x2 );
static MenuOption_t MEO_RENDERERSETUP_TEXCACHE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_RENDERERSETUP_TEXCACHE, &glusetexcache );
static MenuEntry_t ME_RENDERERSETUP_TEXCACHE = MAKE_MENUENTRY( "On-disk texture cache:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_RENDERERSETUP_TEXCACHE, Option );
# endif
# ifdef USE_GLEXT
static MenuOption_t MEO_RENDERERSETUP_DETAILTEX = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &r_detailmapping );
static MenuEntry_t ME_RENDERERSETUP_DETAILTEX = MAKE_MENUENTRY( "Detail textures:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_RENDERERSETUP_DETAILTEX, Option );
static MenuOption_t MEO_RENDERERSETUP_GLOWTEX = MAKE_MENUOPTION(&MF_Bluefont, &MEOS_NoYes, &r_glowmapping);
static MenuEntry_t ME_RENDERERSETUP_GLOWTEX = MAKE_MENUENTRY("Glow textures:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_RENDERERSETUP_GLOWTEX, Option);
# endif
static MenuOption_t MEO_RENDERERSETUP_MODELS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &usemodels );
static MenuEntry_t ME_RENDERERSETUP_MODELS = MAKE_MENUENTRY( "3D models:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_RENDERERSETUP_MODELS, Option );
static MenuOption_t MEO_RENDERERSETUP_PALETTEEMULATION = MAKE_MENUOPTION(&MF_Bluefont, &MEOS_NoYes, &r_usetileshades);
static MenuEntry_t ME_RENDERERSETUP_PALETTEEMULATION = MAKE_MENUENTRY("Palette emulation:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_RENDERERSETUP_PALETTEEMULATION, Option);
#endif

#ifdef POLYMER
static char const *MEOSN_POLYMER_LIGHTS [] = { "Off", "Full", "Map only", };
static MenuOptionSet_t MEOS_POLYMER_LIGHTS = MAKE_MENUOPTIONSET(MEOSN_POLYMER_LIGHTS, NULL, 0x2);
static MenuOption_t MEO_POLYMER_LIGHTS = MAKE_MENUOPTION(&MF_Bluefont, &MEOS_POLYMER_LIGHTS, &pr_lighting);
static MenuEntry_t ME_POLYMER_LIGHTS = MAKE_MENUENTRY("Dynamic lights:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_POLYMER_LIGHTS, Option);

static MenuRangeInt32_t MEO_POLYMER_LIGHTPASSES = MAKE_MENURANGE(&r_pr_maxlightpasses, &MF_Bluefont, 1, 10, 1, 10, 1);
static MenuEntry_t ME_POLYMER_LIGHTPASSES = MAKE_MENUENTRY("Lights per surface:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_POLYMER_LIGHTPASSES, RangeInt32);

static MenuOption_t MEO_POLYMER_SHADOWS = MAKE_MENUOPTION(&MF_Bluefont, &MEOS_OffOn, &pr_shadows);
static MenuEntry_t ME_POLYMER_SHADOWS = MAKE_MENUENTRY("Dynamic shadows:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_POLYMER_SHADOWS, Option);

static MenuRangeInt32_t MEO_POLYMER_SHADOWCOUNT = MAKE_MENURANGE(&pr_shadowcount, &MF_Bluefont, 1, 10, 1, 10, 1);
static MenuEntry_t ME_POLYMER_SHADOWCOUNT = MAKE_MENUENTRY("Shadows per surface:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_POLYMER_SHADOWCOUNT, RangeInt32);

static MenuOption_t MEO_POLYMER_PALETTEEMULATION = MAKE_MENUOPTION(&MF_Bluefont, &MEOS_NoYes, &pr_artmapping);
static MenuEntry_t ME_POLYMER_PALETTEEMULATION = MAKE_MENUENTRY("Palette emulation:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_POLYMER_PALETTEEMULATION, Option);

#endif

#ifdef USE_OPENGL
static MenuEntry_t *MEL_RENDERERSETUP_POLYMOST[] = {
    &ME_RENDERERSETUP_HIGHTILE,
    &ME_RENDERERSETUP_TEXQUALITY,
    &ME_RENDERERSETUP_PRECACHE,
# ifndef EDUKE32_GLES
    &ME_RENDERERSETUP_TEXCACHE,
# endif
# ifdef USE_GLEXT
    &ME_RENDERERSETUP_DETAILTEX,
    &ME_RENDERERSETUP_GLOWTEX,
# endif
    &ME_Space4_Bluefont,
    &ME_RENDERERSETUP_MODELS,
    &ME_RENDERERSETUP_PALETTEEMULATION,
};

#ifdef POLYMER
static MenuEntry_t *MEL_RENDERERSETUP_POLYMER [] = {
    &ME_RENDERERSETUP_HIGHTILE,
    &ME_RENDERERSETUP_TEXQUALITY,
    &ME_RENDERERSETUP_PRECACHE,
# ifndef EDUKE32_GLES
    &ME_RENDERERSETUP_TEXCACHE,
# endif
# ifdef USE_GLEXT
    &ME_RENDERERSETUP_DETAILTEX,
    &ME_RENDERERSETUP_GLOWTEX,
    &ME_POLYMER_PALETTEEMULATION,
# endif
    &ME_Space4_Bluefont,
    &ME_RENDERERSETUP_MODELS,
    &ME_Space4_Bluefont,
    &ME_POLYMER_LIGHTS,
    &ME_POLYMER_LIGHTPASSES,
    &ME_POLYMER_SHADOWS,
    &ME_POLYMER_SHADOWCOUNT,
};
#endif
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
#ifndef EDUKE32_SIMPLE_MENU
    &ME_SCREENSETUP_SCREENSIZE,
#endif

    &ME_SCREENSETUP_SBARSIZE,

    &ME_SCREENSETUP_CROSSHAIR,
    &ME_SCREENSETUP_CROSSHAIRSIZE,

    &ME_SCREENSETUP_LEVELSTATS,
    &ME_SCREENSETUP_TEXTSIZE,

    &ME_SCREENSETUP_SHOWPICKUPMESSAGES,
};

// Save and load will be filled in before every viewing of the save/load screen.
static MenuLink_t MEO_LOAD = { MENU_LOADVERIFY, MA_None, };
static MenuEntry_t ME_LOAD_TEMPLATE = MAKE_MENUENTRY( NULL, &MF_MinifontSave, &MEF_LoadSave, &MEO_LOAD, Link );
static MenuEntry_t ME_LOAD_EMPTY = MAKE_MENUENTRY( NULL, &MF_MinifontSave, &MEF_LoadSave, nullptr, Dummy );
static MenuEntry_t *ME_LOAD;
static MenuEntry_t **MEL_LOAD;

static char const s_NewSaveGame[] = "(New Save Game)";
static MenuString_t MEO_SAVE_TEMPLATE = MAKE_MENUSTRING( NULL, &MF_MinifontSave, MAXSAVEGAMENAME, 0 );
static MenuString_t MEO_SAVE_NEW = MAKE_MENUSTRING( NULL, &MF_MinifontSave, MAXSAVEGAMENAME, 0 );
static MenuString_t *MEO_SAVE;
static MenuEntry_t ME_SAVE_TEMPLATE = MAKE_MENUENTRY( NULL, &MF_MinifontSave, &MEF_LoadSave, &MEO_SAVE_TEMPLATE, String );
static MenuEntry_t ME_SAVE_NEW = MAKE_MENUENTRY( s_NewSaveGame, &MF_MinifontSave, &MEF_LoadSave, &MEO_SAVE_NEW, String );
static MenuEntry_t *ME_SAVE;
static MenuEntry_t **MEL_SAVE;

static int32_t soundrate, soundvoices;
static MenuOption_t MEO_SOUND = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OffOn, &ud.config.SoundToggle );
static MenuEntry_t ME_SOUND = MAKE_MENUENTRY( "Sound:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND, Option );

static MenuOption_t MEO_SOUND_MUSIC = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OffOn, &ud.config.MusicToggle );
static MenuEntry_t ME_SOUND_MUSIC = MAKE_MENUENTRY( "Music:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_MUSIC, Option );

static MenuRangeInt32_t MEO_SOUND_VOLUME_MASTER = MAKE_MENURANGE( &ud.config.MasterVolume, &MF_Redfont, 0, 255, 0, 33, 2 );
static MenuEntry_t ME_SOUND_VOLUME_MASTER = MAKE_MENUENTRY( "Volume:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_VOLUME_MASTER, RangeInt32 );

static MenuRangeInt32_t MEO_SOUND_VOLUME_EFFECTS = MAKE_MENURANGE( &ud.config.FXVolume, &MF_Redfont, 0, 255, 0, 33, 2 );
static MenuEntry_t ME_SOUND_VOLUME_EFFECTS = MAKE_MENUENTRY( "Effects:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_VOLUME_EFFECTS, RangeInt32 );

static MenuRangeInt32_t MEO_SOUND_VOLUME_MUSIC = MAKE_MENURANGE( &ud.config.MusicVolume, &MF_Redfont, 0, 255, 0, 33, 2 );
static MenuEntry_t ME_SOUND_VOLUME_MUSIC = MAKE_MENUENTRY( "Music:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_VOLUME_MUSIC, RangeInt32 );

static MenuOption_t MEO_SOUND_DUKETALK = MAKE_MENUOPTION(&MF_Redfont, &MEOS_NoYes, NULL);
#ifndef EDUKE32_STANDALONE
static MenuEntry_t ME_SOUND_DUKETALK = MAKE_MENUENTRY( "Duke talk:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_DUKETALK, Option );
#else
static MenuEntry_t ME_SOUND_DUKETALK = MAKE_MENUENTRY("Player speech:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_DUKETALK, Option);
#endif

static char const *MEOSN_SOUND_SAMPLINGRATE[] = { "22050Hz", "44100Hz", "48000Hz", };
static int32_t MEOSV_SOUND_SAMPLINGRATE[] = { 22050, 44100, 48000, };
static MenuOptionSet_t MEOS_SOUND_SAMPLINGRATE = MAKE_MENUOPTIONSET( MEOSN_SOUND_SAMPLINGRATE, MEOSV_SOUND_SAMPLINGRATE, 0x3 );
static MenuOption_t MEO_SOUND_SAMPLINGRATE = MAKE_MENUOPTION( &MF_Redfont, &MEOS_SOUND_SAMPLINGRATE, &soundrate );
static MenuEntry_t ME_SOUND_SAMPLINGRATE = MAKE_MENUENTRY( "Sample rate:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_SAMPLINGRATE, Option );

#ifndef EDUKE32_SIMPLE_MENU
static MenuRangeInt32_t MEO_SOUND_NUMVOICES = MAKE_MENURANGE( &soundvoices, &MF_Redfont, 16, 256, 0, 16, 1 );
static MenuEntry_t ME_SOUND_NUMVOICES = MAKE_MENUENTRY( "Voices:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_NUMVOICES, RangeInt32 );
#endif

static MenuEntry_t ME_SOUND_RESTART = MAKE_MENUENTRY( "Apply Changes", &MF_Redfont, &MEF_BigOptions_Apply, &MEO_NULL, Link );

#ifndef EDUKE32_SIMPLE_MENU
static MenuLink_t MEO_ADVSOUND = { MENU_ADVSOUND, MA_Advance, };
static MenuEntry_t ME_SOUND_ADVSOUND = MAKE_MENUENTRY( "Advanced", &MF_Redfont, &MEF_BigOptionsRt, &MEO_ADVSOUND, Link );
#endif

static MenuEntry_t *MEL_SOUND[] = {
    &ME_SOUND,
    &ME_SOUND_MUSIC,
    &ME_SOUND_VOLUME_MASTER,
    &ME_SOUND_VOLUME_EFFECTS,
    &ME_SOUND_VOLUME_MUSIC,
    &ME_SOUND_DUKETALK,
#ifndef EDUKE32_SIMPLE_MENU
    &ME_SOUND_ADVSOUND,
#endif
};

static MenuEntry_t *MEL_ADVSOUND[] = {
    &ME_SOUND_SAMPLINGRATE,
    &ME_Space2_Redfont,
#ifndef EDUKE32_SIMPLE_MENU
    &ME_SOUND_NUMVOICES,
    &ME_Space2_Redfont,
#endif
    &ME_SOUND_RESTART,
};

MAKE_MENU_TOP_ENTRYLINK( "Player Setup", MEF_CenterMenu, NETWORK_PLAYERSETUP, MENU_PLAYER );
MAKE_MENU_TOP_ENTRYLINK( "Join Game", MEF_CenterMenu, NETWORK_JOINGAME, MENU_NETJOIN );
MAKE_MENU_TOP_ENTRYLINK( "Host Game", MEF_CenterMenu, NETWORK_HOSTGAME, MENU_NETHOST );

static MenuEntry_t *MEL_NETWORK[] = {
    &ME_NETWORK_PLAYERSETUP,
    &ME_NETWORK_JOINGAME,
    &ME_NETWORK_HOSTGAME,
};

static MenuString_t MEO_PLAYER_NAME = MAKE_MENUSTRING( szPlayerName, &MF_Bluefont, MAXPLAYERNAME, 0 );
static MenuEntry_t ME_PLAYER_NAME = MAKE_MENUENTRY( "Name", &MF_BluefontRed, &MEF_PlayerNarrow, &MEO_PLAYER_NAME, String );
static char const *MEOSN_PLAYER_COLOR[] = { "Auto", "Blue", "Red", "Green", "Gray", "Dark gray", "Dark green", "Brown", "Dark blue", "Bright red", "Yellow", };
static int32_t MEOSV_PLAYER_COLOR[] = { 0, 9, 10, 11, 12, 13, 14, 15, 16, 21, 23, };
static MenuOptionSet_t MEOS_PLAYER_COLOR = MAKE_MENUOPTIONSET( MEOSN_PLAYER_COLOR, MEOSV_PLAYER_COLOR, 0x2 );
static MenuOption_t MEO_PLAYER_COLOR = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_PLAYER_COLOR, &ud.color );
static MenuEntry_t ME_PLAYER_COLOR = MAKE_MENUENTRY( "Color", &MF_BluefontRed, &MEF_PlayerNarrow, &MEO_PLAYER_COLOR, Option );
static char const *MEOSN_PLAYER_TEAM[] = { "Blue", "Red", "Green", "Gray", };
static MenuOptionSet_t MEOS_PLAYER_TEAM = MAKE_MENUOPTIONSET( MEOSN_PLAYER_TEAM, NULL, 0x2 );
static MenuOption_t MEO_PLAYER_TEAM = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_PLAYER_TEAM, &ud.team );
static MenuEntry_t ME_PLAYER_TEAM = MAKE_MENUENTRY( "Team", &MF_BluefontRed, &MEF_PlayerNarrow, &MEO_PLAYER_TEAM, Option );
#ifndef EDUKE32_SIMPLE_MENU
static MenuLink_t MEO_PLAYER_MACROS = { MENU_MACROS, MA_Advance, };
static MenuEntry_t ME_PLAYER_MACROS = MAKE_MENUENTRY( "Multiplayer macros", &MF_BluefontRed, &MEF_SmallOptions, &MEO_PLAYER_MACROS, Link );
#endif

static MenuEntry_t *MEL_PLAYER[] = {
    &ME_PLAYER_NAME,
    &ME_Space4_Bluefont,
    &ME_PLAYER_COLOR,
    &ME_Space4_Bluefont,
    &ME_PLAYER_TEAM,
#ifndef EDUKE32_SIMPLE_MENU
    &ME_Space8_Bluefont,
    &ME_PLAYER_MACROS,
#endif
};

static MenuString_t MEO_MACROS_TEMPLATE = MAKE_MENUSTRING( NULL, &MF_Bluefont, MAXRIDECULELENGTH, 0 );
static MenuString_t MEO_MACROS[10];
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
static MenuOption_t MEO_NETOPTIONS_GAMETYPE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NETOPTIONS_GAMETYPE, &ud.m_coop );
static MenuEntry_t ME_NETOPTIONS_GAMETYPE = MAKE_MENUENTRY( "Game Type", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_GAMETYPE, Option );
static MenuOptionSet_t MEOS_NETOPTIONS_EPISODE = MAKE_MENUOPTIONSET( MEOSN_NetEpisodes, MEOSV_NetEpisodes, 0x0 );
static int32_t NetEpisode;
static MenuOption_t MEO_NETOPTIONS_EPISODE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NETOPTIONS_EPISODE, &NetEpisode );
static MenuEntry_t ME_NETOPTIONS_EPISODE = MAKE_MENUENTRY( "Episode", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_EPISODE, Option );
static MenuOptionSet_t MEOS_NETOPTIONS_LEVEL_TEMPLATE = MAKE_MENUOPTIONSETNULL;
static MenuOptionSet_t MEOS_NETOPTIONS_LEVEL[MAXVOLUMES];
static MenuOption_t MEO_NETOPTIONS_LEVEL = MAKE_MENUOPTION( &MF_Bluefont, NULL, &ud.m_level_number );
static MenuEntry_t ME_NETOPTIONS_LEVEL = MAKE_MENUENTRY( "Level", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_LEVEL, Option );
static MenuLink_t MEO_NETOPTIONS_USERMAP = { MENU_NETUSERMAP, MA_Advance, };
static MenuEntry_t ME_NETOPTIONS_USERMAP = MAKE_MENUENTRY( "User Map", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_USERMAP, Link );
static MenuOptionSet_t MEOS_NETOPTIONS_MONSTERS = MAKE_MENUOPTIONSET( MEOSN_NetSkills, NULL, 0x0 );
static MenuOption_t MEO_NETOPTIONS_MONSTERS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NETOPTIONS_MONSTERS, NULL );
static MenuEntry_t ME_NETOPTIONS_MONSTERS = MAKE_MENUENTRY( "Monsters", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_MONSTERS, Option );
static MenuOption_t MEO_NETOPTIONS_MARKERS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.m_marker );
static MenuEntry_t ME_NETOPTIONS_MARKERS = MAKE_MENUENTRY( "Markers", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_MARKERS, Option );
static MenuOption_t MEO_NETOPTIONS_MAPEXITS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OnOff, &ud.m_noexits );
static MenuEntry_t ME_NETOPTIONS_MAPEXITS = MAKE_MENUENTRY( "Map Exits", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_MAPEXITS, Option );
static MenuOption_t MEO_NETOPTIONS_FRFIRE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.m_ffire );
static MenuEntry_t ME_NETOPTIONS_FRFIRE = MAKE_MENUENTRY( "Fr. Fire", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_FRFIRE, Option );
static MenuEntry_t ME_NETOPTIONS_ACCEPT = MAKE_MENUENTRY( "Accept", &MF_Redfont, &MEF_NetSetup_Confirm, &MEO_NULL, Link );

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
static MenuMenu_t M_KEYBOARDSETUP = MAKE_MENUMENU( "Keyboard Setup", &MMF_Top_Options, MEL_KEYBOARDSETUP );
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
static MenuMenu_t M_MOUSEBTNS = MAKE_MENUMENU( "Mouse Buttons", &MMF_MouseJoySetupBtns, MEL_MOUSESETUPBTNS );
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

static MenuPanel_t M_F1HELP = { NoTitle, MENU_STORY, MA_Return, MENU_STORY, MA_Advance, };
static MenuPanel_t M_CREDITS = { NoTitle, MENU_CREDITS5, MA_Return, MENU_CREDITS2, MA_Advance, };
static MenuPanel_t M_CREDITS2 = { NoTitle, MENU_CREDITS, MA_Return, MENU_CREDITS3, MA_Advance, };
static MenuPanel_t M_CREDITS3 = { NoTitle, MENU_CREDITS2, MA_Return, MENU_CREDITS4, MA_Advance, };
static MenuPanel_t M_CREDITS4 = { "About " APPNAME, MENU_CREDITS3, MA_Return, MENU_CREDITS5, MA_Advance, };
static MenuPanel_t M_CREDITS5 = { "About " APPNAME, MENU_CREDITS4, MA_Return, MENU_CREDITS, MA_Advance, };

#define CURSOR_CENTER_2LINE { MENU_MARGIN_CENTER<<16, 120<<16, }
#define CURSOR_CENTER_3LINE { MENU_MARGIN_CENTER<<16, 129<<16, }
#define CURSOR_BOTTOMRIGHT { 304<<16, 186<<16, }

static MenuVerify_t M_QUIT = { CURSOR_CENTER_2LINE, MENU_CLOSE, MA_None, };
static MenuVerify_t M_QUITTOTITLE = { CURSOR_CENTER_2LINE, MENU_CLOSE, MA_None, };
static MenuVerify_t M_LOADVERIFY = { CURSOR_CENTER_3LINE, MENU_CLOSE, MA_None, };
static MenuVerify_t M_NEWVERIFY = { CURSOR_CENTER_2LINE, MENU_EPISODE, MA_Advance, };
static MenuVerify_t M_SAVEVERIFY = { CURSOR_CENTER_2LINE, MENU_SAVE, MA_None, };
static MenuVerify_t M_RESETPLAYER = { CURSOR_CENTER_3LINE, MENU_CLOSE, MA_None, };

static MenuMessage_t M_NETWAITMASTER = { CURSOR_BOTTOMRIGHT, MENU_NULL, MA_None, };
static MenuMessage_t M_NETWAITVOTES = { CURSOR_BOTTOMRIGHT, MENU_NULL, MA_None, };
static MenuMessage_t M_BUYDUKE = { CURSOR_BOTTOMRIGHT, MENU_EPISODE, MA_Return, };

static MenuTextForm_t M_ADULTPASSWORD = { NULL, "Enter Password:", MAXPWLOCKOUT, MTF_Password };
static MenuTextForm_t M_CHEATENTRY = { NULL, "Enter Cheat Code:", MAXCHEATLEN, 0 };
static MenuTextForm_t M_CHEAT_WARP = { NULL, "Enter Warp #:", 3, 0 };
static MenuTextForm_t M_CHEAT_SKILL = { NULL, "Enter Skill #:", 1, 0 };

#define MAKE_MENUFILESELECT(a, b, c) { a, { &MMF_FileSelectLeft, &MMF_FileSelectRight }, { &MF_Minifont, &MF_MinifontRed }, b, c, { NULL, NULL }, { 0, 0 }, { 3<<16, 3<<16 }, FNLIST_INITIALIZER, 0 }

static MenuFileSelect_t M_USERMAP = MAKE_MENUFILESELECT( "Select A User Map", "*.map", boardfilename );

// MUST be in ascending order of MenuID enum values due to binary search
static Menu_t Menus[] = {
    { &M_MAIN, MENU_MAIN, MENU_CLOSE, MA_None, Menu },
    { &M_MAIN_INGAME, MENU_MAIN_INGAME, MENU_CLOSE, MA_None, Menu },
    { &M_EPISODE, MENU_EPISODE, MENU_MAIN, MA_Return, Menu },
    { &M_USERMAP, MENU_USERMAP, MENU_EPISODE, MA_Return, FileSelect },
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
    { &M_MOUSEBTNS, MENU_MOUSEBTNS, MENU_MOUSESETUP, MA_Return, Menu },
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
    { &M_QUIT, MENU_QUIT, MENU_PREVIOUS, MA_Return, Verify },
    { &M_QUITTOTITLE, MENU_QUITTOTITLE, MENU_PREVIOUS, MA_Return, Verify },
    { &M_QUIT, MENU_QUIT_INGAME, MENU_CLOSE, MA_None, Verify },
    { &M_NETHOST, MENU_NETSETUP, MENU_MAIN, MA_Return, Menu },
    { &M_NETWAITMASTER, MENU_NETWAITMASTER, MENU_MAIN, MA_Return, Message },
    { &M_NETWAITVOTES, MENU_NETWAITVOTES, MENU_MAIN, MA_Return, Message },
    { &M_SOUND, MENU_SOUND, MENU_OPTIONS, MA_Return, Menu },
    { &M_SOUND, MENU_SOUND_INGAME, MENU_CLOSE, MA_Return, Menu },
    { &M_ADVSOUND, MENU_ADVSOUND, MENU_SOUND, MA_Return, Menu },
#ifdef EDUKE32_SIMPLE_MENU
    { &M_CHEATS, MENU_CHEATS, MENU_OPTIONS, MA_Return, Menu },
#else
    { &M_CHEATS, MENU_CHEATS, MENU_GAMESETUP, MA_Return, Menu },
#endif
    { &M_CHEATENTRY, MENU_CHEATENTRY, MENU_CHEATS, MA_None, TextForm },
    { &M_CHEAT_WARP, MENU_CHEAT_WARP, MENU_CHEATS, MA_None, TextForm },
    { &M_CHEAT_SKILL, MENU_CHEAT_SKILL, MENU_CHEATS, MA_None, TextForm },
    { &M_CREDITS, MENU_CREDITS, MENU_MAIN, MA_Return, Panel },
    { &M_CREDITS2, MENU_CREDITS2, MENU_MAIN, MA_Return, Panel },
    { &M_CREDITS3, MENU_CREDITS3, MENU_MAIN, MA_Return, Panel },
    { &M_CREDITS4, MENU_CREDITS4, MENU_MAIN, MA_Return, Panel },
    { &M_CREDITS5, MENU_CREDITS5, MENU_MAIN, MA_Return, Panel },
    { &M_LOADVERIFY, MENU_LOADVERIFY, MENU_LOAD, MA_None, Verify },
    { &M_NEWVERIFY, MENU_NEWVERIFY, MENU_MAIN, MA_Return, Verify },
    { &M_SAVEVERIFY, MENU_SAVEVERIFY, MENU_SAVE, MA_None, Verify },
    { &M_ADULTPASSWORD, MENU_ADULTPASSWORD, MENU_GAMESETUP, MA_None, TextForm },
    { &M_RESETPLAYER, MENU_RESETPLAYER, MENU_CLOSE, MA_None, Verify },
    { &M_BUYDUKE, MENU_BUYDUKE, MENU_EPISODE, MA_Return, Message },
    { &M_NETWORK, MENU_NETWORK, MENU_MAIN, MA_Return, Menu },
    { &M_PLAYER, MENU_PLAYER, MENU_OPTIONS, MA_Return, Menu },
    { &M_MACROS, MENU_MACROS, MENU_PLAYER, MA_Return, Menu },
    { &M_NETHOST, MENU_NETHOST, MENU_NETWORK, MA_Return, Menu },
    { &M_NETOPTIONS, MENU_NETOPTIONS, MENU_NETWORK, MA_Return, Menu },
    { &M_USERMAP, MENU_NETUSERMAP, MENU_NETOPTIONS, MA_Return, FileSelect },
    { &M_NETJOIN, MENU_NETJOIN, MENU_NETWORK, MA_Return, Menu },
};

static CONSTEXPR const size_t numMenus = ARRAY_SIZE(Menus);

Menu_t *m_currentMenu = &Menus[0];
static Menu_t *m_previousMenu = &Menus[0];
static Menu_t * m_parentMenu;

MenuID_t g_currentMenu;
static MenuID_t g_previousMenu;

#define MenuMenu_ChangeEntryList(m, el)\
    do {\
        m.entrylist = el;\
        m.numEntries = ARRAY_SIZE(el);\
    } while (0)

static void MenuEntry_DisableOnCondition(MenuEntry_t * const entry, const int32_t condition)
{
    if (condition)
        entry->flags |= MEF_Disabled;
    else
        entry->flags &= ~MEF_Disabled;
}
static void MenuEntry_LookDisabledOnCondition(MenuEntry_t * const entry, const int32_t condition)
{
    if (condition)
        entry->flags |= MEF_LookDisabled;
    else
        entry->flags &= ~MEF_LookDisabled;
}

static int32_t M_RunMenu_Menu(Menu_t *cm, MenuMenu_t *menu, MenuEntry_t *currentry, int32_t state, const vec2_t origin, bool actually_draw = true);
static void Menu_EntryFocus(/*MenuEntry_t *entry*/);

static MenuEntry_t *Menu_AdjustForCurrentEntryAssignment(MenuMenu_t *menu)
{
    MenuEntry_t *currentry = menu->entrylist[menu->currentEntry];

    Menu_EntryFocus(/*currentry*/);

    if (currentry->ybottom - menu->scrollPos > klabs(menu->format->bottomcutoff))
        menu->scrollPos = currentry->ybottom - klabs(menu->format->bottomcutoff);
    else if (currentry->ytop - menu->scrollPos < menu->format->pos.y)
        menu->scrollPos = currentry->ytop - menu->format->pos.y;

    return currentry;
}

static MenuEntry_t *Menu_AdjustForCurrentEntryAssignmentBlind(MenuMenu_t *menu)
{
    M_RunMenu_Menu(nullptr, menu, nullptr, 0, { 0, 0 }, false);
    return Menu_AdjustForCurrentEntryAssignment(menu);
}

/*
This function prepares data after ART and CON have been processed.
It also initializes some data in loops rather than statically at compile time.
*/
void Menu_Init(void)
{
    int32_t i, j, k;

    // prepare menu fonts
    MF_Redfont.tilenum = BIGALPHANUM;
    MF_Bluefont.tilenum = MF_BluefontRed.tilenum = MF_BluefontGame.tilenum = STARTALPHANUM;
    MF_Minifont.tilenum = MF_MinifontRed.tilenum = MF_MinifontSave.tilenum = MINIFONT;
    MF_Redfont.emptychar.y = tilesiz[BIGALPHANUM].y<<16;
    MF_Bluefont.emptychar.y = MF_BluefontRed.emptychar.y = MF_BluefontGame.emptychar.y = tilesiz[STARTALPHANUM].y<<16;
    MF_Minifont.emptychar.y = MF_MinifontRed.emptychar.y = MF_MinifontSave.emptychar.y = tilesiz[MINIFONT].y<<16;
    if (NAM_WW2GI)
        MF_Bluefont.between.x = MF_BluefontRed.between.x = 0;
    if (!minitext_lowercase)
    {
        MF_Minifont.textflags |= TEXT_UPPERCASE;
        MF_MinifontRed.textflags |= TEXT_UPPERCASE;
        MF_MinifontSave.textflags |= TEXT_UPPERCASE;
    }

    // prepare gamefuncs and keys
    MEOSN_Gamefuncs[0] = MenuGameFuncNone;
    MEOSV_Gamefuncs[0] = -1;
    k = 1;
    for (i = 0; i < NUMGAMEFUNCTIONS; ++i)
    {
        Bstrcpy(MenuGameFuncs[i], gamefunctions[i]);

        for (j = 0; j < MAXGAMEFUNCLEN; ++j)
            if (MenuGameFuncs[i][j] == '_')
                MenuGameFuncs[i][j] = ' ';

        if (gamefunctions[i][0] != '\0')
        {
            MEOSN_Gamefuncs[k] = MenuGameFuncs[i];
            MEOSV_Gamefuncs[k] = i;
            ++k;
        }
    }
    MEOS_Gamefuncs.numOptions = k;

    for (i = 0; i < NUMKEYS; ++i)
        MEOSN_Keys[i] = key_names[i];
    MEOSN_Keys[NUMKEYS-1] = MenuKeyNone;


    // prepare episodes
    k = 0;
    for (i = 0; i < g_volumeCnt; ++i)
    {
        if (g_volumeNames[i][0])
        {
            if (!(g_volumeFlags[i] & EF_HIDEFROMSP))
            {
                MEL_EPISODE[i] = &ME_EPISODE[i];
                ME_EPISODE[i] = ME_EPISODE_TEMPLATE;
                ME_EPISODE[i].name = g_volumeNames[i];
            }

            // if (!(EpisodeFlags[i] & EF_HIDEFROMMP))
            {
                MEOSN_NetEpisodes[k] = g_volumeNames[i];
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

    // prepare skills
    k = -1;
    for (i = 0; i < g_skillCnt && g_skillNames[i][0]; ++i)
    {
        MEL_SKILL[i] = &ME_SKILL[i];
        ME_SKILL[i] = ME_SKILL_TEMPLATE;
        ME_SKILL[i].name = g_skillNames[i];

        MEOSN_NetSkills[i] = g_skillNames[i];

        k = i;
    }
    ++k;
    M_SKILL.numEntries = g_skillCnt; // k;
    MEOS_NETOPTIONS_MONSTERS.numOptions = g_skillCnt + 1; // k+1;
    MEOSN_NetSkills[g_skillCnt] = MenuSkillNone;
    MMF_Top_Skill.pos.y = (58 + (4-g_skillCnt)*6)<<16;
    M_SKILL.currentEntry = 1;
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
    if (NAM_WW2GI)
        ME_NETOPTIONS_MONSTERS.name = "Enemies";

    // prepare text chat macros
    for (i = 0; i < MAXRIDECULE; ++i)
    {
        MEL_MACROS[i] = &ME_MACROS[i];
        ME_MACROS[i] = ME_MACROS_TEMPLATE;
        ME_MACROS[i].entry = &MEO_MACROS[i];
        MEO_MACROS[i] = MEO_MACROS_TEMPLATE;

        MEO_MACROS[i].variable = ud.ridecule[i];
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
        MEO_KEYBOARDSETUPFUNCS[i].column[0] = &ud.config.KeyboardKeys[i][0];
        MEO_KEYBOARDSETUPFUNCS[i].column[1] = &ud.config.KeyboardKeys[i][1];
    }
    M_KEYBOARDKEYS.numEntries = NUMGAMEFUNCTIONS;
    for (i = 0; i < MENUMOUSEFUNCTIONS; ++i)
    {
        MEL_MOUSESETUPBTNS[i] = &ME_MOUSESETUPBTNS[i];
        ME_MOUSESETUPBTNS[i] = ME_MOUSEJOYSETUPBTNS_TEMPLATE;
        ME_MOUSESETUPBTNS[i].name = MenuMouseNames[i];
        ME_MOUSESETUPBTNS[i].entry = &MEO_MOUSESETUPBTNS[i];
        MEO_MOUSESETUPBTNS[i] = MEO_MOUSEJOYSETUPBTNS_TEMPLATE;
        MEO_MOUSESETUPBTNS[i].data = &ud.config.MouseFunctions[MenuMouseDataIndex[i][0]][MenuMouseDataIndex[i][1]];
    }
    for (i = 0; i < 2*joynumbuttons + 8*joynumhats; ++i)
    {
        if (i < 2*joynumbuttons)
        {
            if (i & 1)
                Bsnprintf(MenuJoystickNames[i], MAXJOYBUTTONSTRINGLENGTH, "Double %s", getjoyname(1, i>>1));
            else
                Bstrncpy(MenuJoystickNames[i], getjoyname(1, i>>1), MAXJOYBUTTONSTRINGLENGTH);
        }
        else
        {
            Bsnprintf(MenuJoystickNames[i], MAXJOYBUTTONSTRINGLENGTH, (i & 1) ? "Double Hat %d %s" : "Hat %d %s", ((i - 2*joynumbuttons)>>3), MenuJoystickHatDirections[((i - 2*joynumbuttons)>>1) % 4]);
        }

        MEL_JOYSTICKBTNS[i] = &ME_JOYSTICKBTNS[i];
        ME_JOYSTICKBTNS[i] = ME_MOUSEJOYSETUPBTNS_TEMPLATE;
        ME_JOYSTICKBTNS[i].name = MenuJoystickNames[i];
        ME_JOYSTICKBTNS[i].entry = &MEO_JOYSTICKBTNS[i];
        MEO_JOYSTICKBTNS[i] = MEO_MOUSEJOYSETUPBTNS_TEMPLATE;
        MEO_JOYSTICKBTNS[i].data = &ud.config.JoystickFunctions[i>>1][i&1];
    }
    M_JOYSTICKBTNS.numEntries = 2*joynumbuttons + 8*joynumhats;
    for (i = 0; i < joynumaxes; ++i)
    {
        ME_JOYSTICKAXES[i] = ME_JOYSTICKAXES_TEMPLATE;
        Bstrncpy(MenuJoystickAxes[i], getjoyname(0, i), MAXJOYBUTTONSTRINGLENGTH);
        ME_JOYSTICKAXES[i].name = MenuJoystickAxes[i];
        MEL_JOYSTICKAXES[i] = &ME_JOYSTICKAXES[i];
    }
    M_JOYSTICKAXES.numEntries = joynumaxes;

    // prepare video setup
    for (i = 0; i < validmodecnt; ++i)
    {
        int32_t *const r = &MEOS_VIDEOSETUP_RESOLUTION.numOptions;

        for (j = 0; j < *r; ++j)
        {
            if (validmode[i].xdim == resolution[j].xdim && validmode[i].ydim == resolution[j].ydim)
            {
                resolution[j].flags |= validmode[i].fs ? RES_FS : RES_WIN;
                if (validmode[i].bpp > resolution[j].bppmax)
                    resolution[j].bppmax = validmode[i].bpp;
                break;
            }
        }

        if (j == *r) // no match found
        {
            resolution[j].xdim = validmode[i].xdim;
            resolution[j].ydim = validmode[i].ydim;
            resolution[j].bppmax = validmode[i].bpp;
            resolution[j].flags = validmode[i].fs ? RES_FS : RES_WIN;
            Bsnprintf(resolution[j].name, MAXRESOLUTIONSTRINGLENGTH, "%d x %d", resolution[j].xdim, resolution[j].ydim);
            MEOSN_VIDEOSETUP_RESOLUTION[j] = resolution[j].name;
            ++*r;
        }
    }

    // prepare sound setup
#ifndef EDUKE32_STANDALONE
    if (WW2GI)
        ME_SOUND_DUKETALK.name = "GI talk:";
    else if (NAM)
        ME_SOUND_DUKETALK.name = "Grunt talk:";
#endif

    if (KXDWN)
    {
        MF_Redfont.between.x = 2<<16;
        MF_Redfont.cursorScale = 32768;
        MF_Redfont.zoom = 16384;
        MF_Bluefont.between.x = MF_BluefontRed.between.x = 0;
        MF_Bluefont.zoom = MF_BluefontRed.zoom = MF_BluefontGame.zoom = 16384;

        // hack; should swap out pointers
        MF_Minifont = MF_Bluefont;
        MF_MinifontRed = MF_BluefontRed;
        MF_MinifontSave.zoom = 32768;

        MMF_Top_Main.pos.x = 40<<16;
        MMF_Top_Main.pos.y = 130<<16;
        MMF_Top_Main.bottomcutoff = 190<<16;
        M_OPTIONS.format = &MMF_Top_Main;

        MEF_MainMenu.width = MEF_OptionsMenu.width = -(160<<16);
        MEF_MainMenu.marginBottom = 7<<16;

        M_OPTIONS.title = NoTitle;
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

    // prepare pre-Atomic
    if (!VOLUMEALL || !PLUTOPAK)
    {
        // prepare credits
        M_CREDITS.title = M_CREDITS2.title = M_CREDITS3.title = s_Credits;
    }
}

static void Menu_Run(Menu_t *cm, vec2_t origin);


static void Menu_BlackRectangle(int32_t x, int32_t y, int32_t width, int32_t height, int32_t orientation);

/*
At present, no true difference is planned between Menu_Pre() and Menu_PreDraw().
They are separate for purposes of organization.
*/

static void Menu_Pre(MenuID_t cm)
{
    int32_t i;
    DukePlayer_t *ps = g_player[myconnectindex].ps;

    switch (cm)
    {
    case MENU_MAIN_INGAME:
        MenuEntry_DisableOnCondition(&ME_MAIN_SAVEGAME, ud.recstat == 2);
        MenuEntry_DisableOnCondition(&ME_MAIN_QUITTOTITLE, g_netServer || numplayers > 1);
        fallthrough__;
    case MENU_MAIN:
        if ((g_netServer || ud.multimode > 1) && ud.recstat != 2)
        {
            ME_MAIN_NEWGAME.entry = &MEO_MAIN_NEWGAME_NETWORK;
            ME_MAIN_NEWGAME_INGAME.entry = &MEO_MAIN_NEWGAME_NETWORK;
        }
        else
        {
            ME_MAIN_NEWGAME.entry = &MEO_MAIN_NEWGAME;
            ME_MAIN_NEWGAME_INGAME.entry = &MEO_MAIN_NEWGAME_INGAME;
        }
        break;

    case MENU_GAMESETUP:
        MEO_GAMESETUP_DEMOREC.options = (ps->gm&MODE_GAME) ? &MEOS_DemoRec : &MEOS_OffOn;
        MenuEntry_DisableOnCondition(&ME_GAMESETUP_DEMOREC, (ps->gm&MODE_GAME) && ud.m_recstat != 1);
        break;

#ifdef USE_OPENGL
    case MENU_DISPLAYSETUP:
        if (getrendermode() == REND_CLASSIC)
            MenuMenu_ChangeEntryList(M_DISPLAYSETUP, MEL_DISPLAYSETUP);
#ifdef POLYMER
        else if (getrendermode() == REND_POLYMER)
            MenuMenu_ChangeEntryList(M_DISPLAYSETUP, MEL_DISPLAYSETUP_GL_POLYMER);
#endif
        else
            MenuMenu_ChangeEntryList(M_DISPLAYSETUP, MEL_DISPLAYSETUP_GL);

        vpsize = ud.screen_size + 4*(ud.screen_size>=8 && ud.statusbarmode==0) + 4*(ud.screen_size>=4 && ud.althud==0);

        if (getrendermode() != REND_CLASSIC)
        {
            for (i = (int32_t) ARRAY_SIZE(MEOSV_DISPLAYSETUP_ANISOTROPY) - 1; i >= 0; --i)
            {
                if (MEOSV_DISPLAYSETUP_ANISOTROPY[i] <= glinfo.maxanisotropy)
                {
                    MEOS_DISPLAYSETUP_ANISOTROPY.numOptions = i + 1;
                    break;
                }
            }
        }
        break;

    case MENU_POLYMER:
    case MENU_POLYMOST:
        MenuEntry_DisableOnCondition(&ME_RENDERERSETUP_TEXQUALITY, !usehightile);
        MenuEntry_DisableOnCondition(&ME_RENDERERSETUP_PRECACHE, !usehightile);
# ifndef EDUKE32_GLES
        MenuEntry_DisableOnCondition(&ME_RENDERERSETUP_TEXCACHE, !(glusetexcompr && usehightile));
# endif
# ifdef USE_GLEXT
        MenuEntry_DisableOnCondition(&ME_RENDERERSETUP_DETAILTEX, !usehightile);
        MenuEntry_DisableOnCondition(&ME_RENDERERSETUP_GLOWTEX, !usehightile);
# endif
        break;
#endif

    case MENU_VIDEOSETUP:
    {
        const int32_t nr = newresolution;

        MenuEntry_DisableOnCondition(&ME_VIDEOSETUP_APPLY,
             (xdim == resolution[nr].xdim && ydim == resolution[nr].ydim &&
              getrendermode() == newrendermode && fullscreen == newfullscreen
              && vsync == newvsync
             )
             || (newfullscreen ? !(resolution[nr].flags & RES_FS) : !(resolution[nr].flags & RES_WIN))
             || (newrendermode != REND_CLASSIC && resolution[nr].bppmax <= 8));
        break;
    }

    case MENU_SOUND:
    case MENU_SOUND_INGAME:
    case MENU_ADVSOUND:
        MenuEntry_DisableOnCondition(&ME_SOUND_VOLUME_MASTER, !ud.config.SoundToggle && !ud.config.MusicToggle);
        MenuEntry_DisableOnCondition(&ME_SOUND_VOLUME_EFFECTS, !ud.config.SoundToggle);
        MenuEntry_DisableOnCondition(&ME_SOUND_VOLUME_MUSIC, !ud.config.MusicToggle);
        MenuEntry_DisableOnCondition(&ME_SOUND_DUKETALK, !ud.config.SoundToggle);
        MenuEntry_DisableOnCondition(&ME_SOUND_SAMPLINGRATE, !ud.config.SoundToggle && !ud.config.MusicToggle);
#ifndef EDUKE32_SIMPLE_MENU
        MenuEntry_DisableOnCondition(&ME_SOUND_NUMVOICES, !ud.config.SoundToggle);
#endif
        MenuEntry_DisableOnCondition(&ME_SOUND_RESTART, soundrate == ud.config.MixRate &&
                                                        soundvoices == ud.config.NumVoices);
        break;

#ifndef EDUKE32_SIMPLE_MENU
    case MENU_MOUSESETUP:
        MenuEntry_DisableOnCondition(&ME_MOUSESETUP_MOUSEAIMING, ud.mouseaiming);
        break;
#endif
    case MENU_NETOPTIONS:
        if (MEOSV_NetEpisodes[MEO_NETOPTIONS_EPISODE.currentOption] == MAXVOLUMES)
            MEL_NETOPTIONS[2] = &ME_NETOPTIONS_USERMAP;
        else
        {
            MEL_NETOPTIONS[2] = &ME_NETOPTIONS_LEVEL;
            MEO_NETOPTIONS_LEVEL.options = &MEOS_NETOPTIONS_LEVEL[MEOSV_NetEpisodes[MEO_NETOPTIONS_EPISODE.currentOption]];
        }
        if (!(g_gametypeFlags[ud.m_coop] & GAMETYPE_MARKEROPTION))
        {
            ME_NETOPTIONS_MARKERS.type = Dummy;
            ME_NETOPTIONS_MARKERS.flags |= MEF_Disabled;
        }
        else
        {
            ME_NETOPTIONS_MARKERS.type = Option;
            ME_NETOPTIONS_MARKERS.flags &= ~MEF_Disabled;
        }
        MEL_NETOPTIONS[5] = (g_gametypeFlags[ud.m_coop] & (GAMETYPE_PLAYERSFRIENDLY|GAMETYPE_TDM)) ? &ME_NETOPTIONS_FRFIRE : &ME_NETOPTIONS_MAPEXITS;
        break;

    case MENU_OPTIONS:
        MenuEntry_DisableOnCondition(&ME_OPTIONS_PLAYERSETUP, ud.recstat == 1);
        MenuEntry_DisableOnCondition(&ME_OPTIONS_JOYSTICKSETUP, CONTROL_JoyPresent == 0);
        break;

    case MENU_COLCORR:
    case MENU_COLCORR_INGAME:
        MenuEntry_DisableOnCondition(&ME_COLCORR_CONTRAST, !gammabrightness);
        MenuEntry_DisableOnCondition(&ME_COLCORR_BRIGHTNESS, !gammabrightness);
        break;

    case MENU_CHEATS:
    case MENU_CHEATENTRY:
    case MENU_CHEAT_WARP:
    case MENU_CHEAT_SKILL:
    {
        const int32_t menucheatsdisabled = numplayers != 1 || !(g_player[myconnectindex].ps->gm & MODE_GAME);

        for (i = 0; i < NUMCHEATFUNCS; i++)
        {
            uint32_t cheatmask = cl_cheatmask & (1<<i);

            // KEEPINSYNC: NAM_WW2GI_CHEATS
            if (NAM_WW2GI)
            {
                switch (i)
                {
                case CHEATFUNC_CASHMAN:
                case CHEATFUNC_GIVEALLITEMS:
                case CHEATFUNC_QUOTEBETA:
                case CHEATFUNC_MONSTERS:
                case CHEATFUNC_QUOTEALLEN:
                case CHEATFUNC_GIVEKEYS:
                    cheatmask = 0;
                    break;
                }
            }
            if (WW2GI)
            {
                switch (i)
                {
                case CHEATFUNC_HYPER:
                    cheatmask = 0;
                    break;
                }
            }

            // only show cheats that have been typed in before
            MEL_CHEATS[i+1] = cheatmask ? &ME_CheatCodes[i] : NULL;

            // disable outside of a single-player game
            MenuEntry_DisableOnCondition(&ME_CheatCodes[i], menucheatsdisabled);
        }

        // refresh display names of quote cheats
        if (!DUKEBETA)
        {
            ME_CheatCodes[CHEATFUNC_QUOTEBETA].name = apStrings[QUOTE_CHEAT_BETA];
            ME_CheatCodes[CHEATFUNC_QUOTETODD].name = NAM ? g_NAMMattCheatQuote : apStrings[QUOTE_CHEAT_TODD];
            ME_CheatCodes[CHEATFUNC_QUOTEALLEN].name = apStrings[QUOTE_CHEAT_ALLEN];
        }

        MenuEntry_DisableOnCondition(&ME_ENTERCHEAT, (cl_cheatmask == UINT32_MAX));

        break;
    }

    default:
        break;
    }
}


static void Menu_PreDrawBackground(MenuID_t cm, const vec2_t origin)
{
    switch (cm)
    {
    case MENU_CREDITS:
    case MENU_CREDITS2:
    case MENU_CREDITS3:
        if (!VOLUMEALL || !PLUTOPAK)
            Menu_DrawBackground(origin);
        else
            rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + (100<<16), 65536L,0,2504+cm-MENU_CREDITS,0,0,10+64);
        break;

    case MENU_LOAD:
    case MENU_SAVE:
    case MENU_CREDITS4:
    case MENU_CREDITS5:
        Menu_DrawBackground(origin);
        break;

    case MENU_STORY:
        rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + (100<<16), 65536L,0,TEXTSTORY,0,0,10+64);
        break;

    case MENU_F1HELP:
        rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + (100<<16), 65536L,0,F1HELP,0,0,10+64);
        break;
    }
}


static void Menu_PreDraw(MenuID_t cm, MenuEntry_t *entry, const vec2_t origin)
{
    int32_t i, j, l = 0, m;

    switch (cm)
    {
    case MENU_MAIN_INGAME:
        l += 4;
        fallthrough__;
    case MENU_MAIN:
        if ((G_GetLogoFlags() & LOGO_NOGAMETITLE) == 0)
        {
            rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + ((28+l)<<16), 65536L,0,INGAMEDUKETHREEDEE,0,0,10);
            if (PLUTOPAK)   // JBF 20030804
                rotatesprite_fs(origin.x + ((MENU_MARGIN_CENTER+100)<<16), origin.y + (36<<16), 65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8);
        }
        break;

    case MENU_PLAYER:
        rotatesprite_fs(origin.x + (260<<16), origin.y + ((24+(tilesiz[APLAYER].y>>1))<<16), 49152L,0,1441-((((4-(totalclock>>4)))&3)*5),0,entry == &ME_PLAYER_TEAM ? G_GetTeamPalette(ud.team) : ud.color,10);
        break;

    case MENU_MACROS:
        mgametextcenter(origin.x, origin.y + (144<<16), "Activate in-game with Shift-F#");
        break;

    case MENU_COLCORR:
    case MENU_COLCORR_INGAME:
        // center panel
        rotatesprite_fs(origin.x + (120<<16), origin.y + (32<<16), 16384, 0, 3290, 0, 0, 2|8|16);
        rotatesprite_fs(origin.x + (160<<16) - (tilesiz[BOTTOMSTATUSBAR].x<<13), origin.y + (82<<16) - (tilesiz[BOTTOMSTATUSBAR].y<<14), 16384, 0, BOTTOMSTATUSBAR, 0, 0, 2|8|16);

        // left panel
        rotatesprite_fs(origin.x + (40<<16), origin.y + (32<<16), 16384, 0, BONUSSCREEN, 0, 0, 2|8|16);

        // right panel
        rotatesprite_fs(origin.x + (200<<16), origin.y + (32<<16), 16384, 0, LOADSCREEN, 0, 0, 2|8|16);
        break;

    case MENU_NETSETUP:
    case MENU_NETHOST:
        mminitext(origin.x + (90<<16), origin.y + (90<<16), "Game Type", 2);
        mminitext(origin.x + (90<<16), origin.y + ((90+8)<<16), "Episode", 2);
        mminitext(origin.x + (90<<16), origin.y + ((90+8+8)<<16), "Level", 2);
        mminitext(origin.x + (90<<16), origin.y + ((90+8+8+8)<<16), ME_NETOPTIONS_MONSTERS.name, 2);
        if (ud.m_coop == 0)
            mminitext(origin.x + (90<<16), origin.y + ((90+8+8+8+8)<<16), "Markers", 2);
        else if (ud.m_coop == 1)
            mminitext(origin.x + (90<<16), origin.y + ((90+8+8+8+8)<<16), "Friendly Fire", 2);
        mminitext(origin.x + (90<<16), origin.y + ((90+8+8+8+8+8)<<16), "User Map", 2);

        mminitext(origin.x + ((90+60)<<16), origin.y + (90<<16), g_gametypeNames[ud.m_coop], 0);

        mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8)<<16), g_volumeNames[ud.m_volume_number], 0);
        mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8)<<16), g_mapInfo[MAXLEVELS*ud.m_volume_number+ud.m_level_number].name, 0);
        if (ud.m_monsters_off == 0 || ud.m_player_skill > 0)
            mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8+8)<<16), g_skillNames[ud.m_player_skill], 0);
        else mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8+8)<<16), "None", 0);
        if (ud.m_coop == 0)
        {
            if (ud.m_marker) mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8+8+8)<<16), "On", 0);
            else mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8+8+8)<<16), "Off", 0);
        }
        else if (ud.m_coop == 1)
        {
            if (ud.m_ffire) mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8+8+8)<<16), "On", 0);
            else mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8+8+8)<<16), "Off", 0);
        }
        break;

    case MENU_MOUSEADVANCED:
    {
        size_t i;
        for (i = 0; i < ARRAY_SIZE(MEL_INTERNAL_MOUSEADVANCED_DAXES); i++)
            if (entry == MEL_INTERNAL_MOUSEADVANCED_DAXES[i])
            {
                mgametextcenter(origin.x, origin.y + (162<<16), "Digital axes are not for mouse look\n"
                                                                "or for aiming up and down");
            }
    }
        break;

    case MENU_RESETPLAYER:
        fade_screen_black(1);
        Bsprintf(tempbuf, "Load last game:\n\"%s\""
#ifndef EDUKE32_ANDROID_MENU
                          "\n(Y/N)"
#endif
        , g_quickload->name);
        mgametextcenter(origin.x, origin.y + (90<<16), tempbuf);
        break;

    case MENU_LOAD:
    {
#if 0
        for (i = 0; i <= 108; i += 12)
            rotatesprite_fs(origin.x + ((160+64+91-64)<<16), origin.y + ((i+56)<<16), 65536L,0,TEXTBOX,24,0,10);
#endif
        Menu_BlackRectangle(origin.x + (198<<16), origin.y + (47<<16), 102<<16, 100<<16, 1|32);

        rotatesprite_fs(origin.x + (22<<16), origin.y + (97<<16), 65536L,0,WINDOWBORDER2,24,0,10);
        rotatesprite_fs(origin.x + (180<<16), origin.y + (97<<16), 65536L,1024,WINDOWBORDER2,24,0,10);
        rotatesprite_fs(origin.x + (99<<16), origin.y + (50<<16), 65536L,512,WINDOWBORDER1,24,0,10);
        rotatesprite_fs(origin.x + (103<<16), origin.y + (144<<16), 65536L,1024+512,WINDOWBORDER1,24,0,10);

        if (M_LOAD.currentEntry >= (int32_t)g_nummenusaves)
        {
            mmenutext(origin.x + (72<<16), origin.y + (100<<16), "Empty");
            break;
        }

        menusave_t & msv = g_menusaves[M_LOAD.currentEntry];

        if (msv.brief.isValid())
        {
            rotatesprite_fs(origin.x + (101<<16), origin.y + (97<<16), 65536>>1,512,TILE_LOADSHOT,-32,0,4+10+64);

            if (msv.isOldVer)
            {
                mmenutext(origin.x + (53<<16), origin.y + (70<<16), "Previous");
                mmenutext(origin.x + (58<<16), origin.y + (90<<16), "Version");

#ifndef EDUKE32_SIMPLE_MENU
                Bsprintf(tempbuf,"Saved: %d.%d.%d %d-bit", savehead.majorver, savehead.minorver,
                         savehead.bytever, 8*savehead.ptrsize);
                mgametext(origin.x + (31<<16), origin.y + (104<<16), tempbuf);
                Bsprintf(tempbuf,"Our: %d.%d.%d %d-bit", SV_MAJOR_VER, SV_MINOR_VER, BYTEVERSION,
                         (int32_t)(8*sizeof(intptr_t)));
                mgametext(origin.x + ((31+16)<<16), origin.y + (114<<16), tempbuf);
#endif
            }

            if (savehead.numplayers > 1)
            {
                Bsprintf(tempbuf, "Players: %-2d                      ", savehead.numplayers);
                mgametextcenter(origin.x, origin.y + (156<<16), tempbuf);
            }

            {
                const char *name = g_mapInfo[(savehead.volnum*MAXLEVELS) + savehead.levnum].name;
                Bsprintf(tempbuf, "%s / %s", name ? name : "^10unnamed^0", g_skillNames[savehead.skill-1]);
            }

            mgametextcenter(origin.x, origin.y + (168<<16), tempbuf);
            if (savehead.volnum == 0 && savehead.levnum == 7)
                mgametextcenter(origin.x, origin.y + (180<<16), savehead.boardfn);
        }
        break;
    }

    case MENU_SAVE:
    {
#if 0
        for (i = 0; i <= 108; i += 12)
            rotatesprite_fs(origin.x + ((160+64+91-64)<<16), origin.y + ((i+56)<<16), 65536L,0,TEXTBOX,24,0,10);
#endif
        Menu_BlackRectangle(origin.x + (198<<16), origin.y + (47<<16), 102<<16, 100<<16, 1|32);

        rotatesprite_fs(origin.x + (22<<16), origin.y + (97<<16), 65536L,0,WINDOWBORDER2,24,0,10);
        rotatesprite_fs(origin.x + (180<<16), origin.y + (97<<16), 65536L,1024,WINDOWBORDER2,24,0,10);
        rotatesprite_fs(origin.x + (99<<16), origin.y + (50<<16), 65536L,512,WINDOWBORDER1,24,0,10);
        rotatesprite_fs(origin.x + (103<<16), origin.y + (144<<16), 65536L,1024+512,WINDOWBORDER1,24,0,10);

        j = 0;
        for (size_t k = 0; k < g_nummenusaves+1; ++k)
            if (((MenuString_t*)M_SAVE.entrylist[k]->entry)->editfield)
                j |= 1;

        if (j)
            rotatesprite_fs(origin.x + (101<<16), origin.y + (97<<16), 65536L>>1,512,TILE_SAVESHOT,-32,0,4+10+64);
        else if (0 < M_SAVE.currentEntry && M_SAVE.currentEntry <= (int32_t)g_nummenusaves)
        {
            if (g_menusaves[M_SAVE.currentEntry-1].brief.isValid())
            {
                rotatesprite_fs(origin.x + (101<<16), origin.y + (97<<16), 65536L>>1,512,TILE_LOADSHOT,-32,0,4+10+64);

                if (g_menusaves[M_SAVE.currentEntry-1].isOldVer)
                {
                    mmenutext(origin.x + (53<<16), origin.y + (70<<16), "Previous");
                    mmenutext(origin.x + (58<<16), origin.y + (90<<16), "Version");

#ifndef EDUKE32_SIMPLE_MENU
                    Bsprintf(tempbuf,"Saved: %d.%d.%d %d-bit", savehead.majorver, savehead.minorver,
                             savehead.bytever, 8*savehead.ptrsize);
                    mgametext(origin.x + (31<<16), origin.y + (104<<16), tempbuf);
                    Bsprintf(tempbuf,"Our: %d.%d.%d %d-bit", SV_MAJOR_VER, SV_MINOR_VER, BYTEVERSION,
                             (int32_t)(8*sizeof(intptr_t)));
                    mgametext(origin.x + ((31+16)<<16), origin.y + (114<<16), tempbuf);
#endif
                }
            }
        }
        else
            mmenutext(origin.x + (82<<16), origin.y + (100<<16), "New");

        if (ud.multimode > 1)
        {
            Bsprintf(tempbuf, "Players: %-2d                      ", ud.multimode);
            mgametextcenter(origin.x, origin.y + (156<<16), tempbuf);
        }

        Bsprintf(tempbuf,"%s / %s",g_mapInfo[(ud.volume_number*MAXLEVELS) + ud.level_number].name, g_skillNames[ud.player_skill-1]);
        mgametextcenter(origin.x, origin.y + (168<<16), tempbuf);
        if (ud.volume_number == 0 && ud.level_number == 7)
            mgametextcenter(origin.x, origin.y + (180<<16), currentboardfilename);
        break;
    }

#ifdef EDUKE32_ANDROID_MENU
    case MENU_SKILL:
    {
        static const char *s[] = { "EASY - Few enemies, and lots of stuff.", "MEDIUM - Normal difficulty.", "HARD - For experienced players.", "EXPERTS - Lots of enemies, plus they respawn!" };
        if ((size_t)M_SKILL.currentEntry < ARRAY_SIZE(s))
            mgametextcenter(origin.x, origin.y + (168<<16), s[M_SKILL.currentEntry]);
    }
        break;
#endif

    case MENU_LOADVERIFY:
    {
        fade_screen_black(1);
        menusave_t & msv = g_menusaves[M_LOAD.currentEntry];
        if (msv.isOldVer)
        {
            Bsprintf(tempbuf, "Start new game:\n%s / %s"
#ifndef EDUKE32_ANDROID_MENU
                              "\n(Y/N)"
#endif
            , g_mapInfo[(ud.volume_number*MAXLEVELS) + ud.level_number].name, g_skillNames[ud.player_skill-1]);
            mgametextcenter(origin.x, origin.y + (90<<16), tempbuf);
        }
        else
        {
            Bsprintf(tempbuf, "Load game:\n\"%s\""
#ifndef EDUKE32_ANDROID_MENU
                              "\n(Y/N)"
#endif
            , msv.brief.name);
            mgametextcenter(origin.x, origin.y + (90<<16), tempbuf);
        }
        break;
    }

    case MENU_SAVEVERIFY:
        fade_screen_black(1);
        mgametextcenter(origin.x, origin.y + (90<<16), "Overwrite previous saved game?"
#ifndef EDUKE32_ANDROID_MENU
                                                       "\n(Y/N)"
#endif
        );
        break;

    case MENU_NEWVERIFY:
        fade_screen_black(1);
        mgametextcenter(origin.x, origin.y + (90<<16), "Abort this game?"
#ifndef EDUKE32_ANDROID_MENU
                                                       "\n(Y/N)"
#endif
        );
        break;

    case MENU_QUIT:
    case MENU_QUIT_INGAME:
        fade_screen_black(1);
        mgametextcenter(origin.x, origin.y + (90<<16), "Are you sure you want to quit?"
#ifndef EDUKE32_ANDROID_MENU
                                                       "\n(Y/N)"
#endif
        );
        break;

    case MENU_QUITTOTITLE:
        fade_screen_black(1);
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

#ifndef EDUKE32_STANDALONE
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
#endif
    case MENU_CREDITS:
    case MENU_CREDITS2:
    case MENU_CREDITS3:
#ifndef EDUKE32_STANDALONE
        if (!VOLUMEALL || !PLUTOPAK)
        {
            switch (cm)
            {
            case MENU_CREDITS:
                m = origin.x + (20<<16);
                l = origin.y + (33<<16);

                shadowminitext(m, l, "Original Concept", 12); l += 7<<16;
                shadowminitext(m, l, "Todd Replogle and Allen H. Blum III", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "Produced & Directed By", 12); l += 7<<16;
                shadowminitext(m, l, "Greg Malone", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "Executive Producer", 12); l += 7<<16;
                shadowminitext(m, l, "George Broussard", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "BUILD Engine", 12); l += 7<<16;
                shadowminitext(m, l, "Ken Silverman", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "Game Programming", 12); l += 7<<16;
                shadowminitext(m, l, "Todd Replogle", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "3D Engine/Tools/Net", 12); l += 7<<16;
                shadowminitext(m, l, "Ken Silverman", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "Network Layer/Setup Program", 12); l += 7<<16;
                shadowminitext(m, l, "Mark Dochtermann", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "Map Design", 12); l += 7<<16;
                shadowminitext(m, l, "Allen H. Blum III", 12); l += 7<<16;
                shadowminitext(m, l, "Richard Gray", 12); l += 7<<16;

                m = origin.x + (180<<16);
                l = origin.y + (33<<16);

                shadowminitext(m, l, "3D Modeling", 12); l += 7<<16;
                shadowminitext(m, l, "Chuck Jones", 12); l += 7<<16;
                shadowminitext(m, l, "Sapphire Corporation", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "Artwork", 12); l += 7<<16;
                shadowminitext(m, l, "Dirk Jones, Stephen Hornback", 12); l += 7<<16;
                shadowminitext(m, l, "James Storey, David Demaret", 12); l += 7<<16;
                shadowminitext(m, l, "Douglas R. Wood", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "Sound Engine", 12); l += 7<<16;
                shadowminitext(m, l, "Jim Dose", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "Sound & Music Development", 12); l += 7<<16;
                shadowminitext(m, l, "Robert Prince", 12); l += 7<<16;
                shadowminitext(m, l, "Lee Jackson", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "Voice Talent", 12); l += 7<<16;
                shadowminitext(m, l, "Lani Minella - Voice Producer", 12); l += 7<<16;
                shadowminitext(m, l, "Jon St. John as \"Duke Nukem\"", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "Graphic Design", 12); l += 7<<16;
                shadowminitext(m, l, "Packaging, Manual, Ads", 12); l += 7<<16;
                shadowminitext(m, l, "Robert M. Atkins", 12); l += 7<<16;
                shadowminitext(m, l, "Michael Hadwin", 12); l += 7<<16;
                break;

            case MENU_CREDITS2:
                m = origin.x + (20<<16);
                l = origin.y + (33<<16);

                shadowminitext(m, l, "Special Thanks To", 12); l += 7<<16;
                shadowminitext(m, l, "Steven Blackburn, Tom Hall", 12); l += 7<<16;
                shadowminitext(m, l, "Scott Miller, Joe Siegler", 12); l += 7<<16;
                shadowminitext(m, l, "Terry Nagy, Colleen Compton", 12); l += 7<<16;
                shadowminitext(m, l, "HASH, Inc., FormGen, Inc.", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "The 3D Realms Beta Testers", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "Nathan Anderson, Wayne Benner", 12); l += 7<<16;
                shadowminitext(m, l, "Glenn Brensinger, Rob Brown", 12); l += 7<<16;
                shadowminitext(m, l, "Erik Harris, Ken Heckbert", 12); l += 7<<16;
                shadowminitext(m, l, "Terry Herrin, Greg Hively", 12); l += 7<<16;
                shadowminitext(m, l, "Hank Leukart, Eric Baker", 12); l += 7<<16;
                shadowminitext(m, l, "Jeff Rausch, Kelly Rogers", 12); l += 7<<16;
                shadowminitext(m, l, "Mike Duncan, Doug Howell", 12); l += 7<<16;
                shadowminitext(m, l, "Bill Blair", 12); l += 7<<16;

                m = origin.x + (160<<16);
                l = origin.y + (33<<16);

                shadowminitext(m, l, "Company Product Support", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "The following companies were cool", 12); l += 7<<16;
                shadowminitext(m, l, "enough to give us lots of stuff", 12); l += 7<<16;
                shadowminitext(m, l, "during the making of Duke Nukem 3D.", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "Altec Lansing Multimedia", 12); l += 7<<16;
                shadowminitext(m, l, "for tons of speakers and the", 12); l += 7<<16;
                shadowminitext(m, l, "THX-licensed sound system.", 12); l += 7<<16;
                shadowminitext(m, l, "For info call 1-800-548-0620", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "Creative Labs, Inc.", 12); l += 7<<16;
                l += 3<<16;
                shadowminitext(m, l, "Thanks for the hardware, guys.", 12); l += 7<<16;
                break;

            case MENU_CREDITS3:
                mgametextcenter(origin.x, origin.y + (50<<16), "Duke Nukem 3D is a trademark of\n"
                                                               "3D Realms Entertainment"
                                                               "\n"
                                                               "Duke Nukem 3D\n"
                                                               "(C) 1996 3D Realms Entertainment");

#if !defined(EDUKE32_ANDROID_MENU) && !defined(EDUKE32_STANDALONE)
                if (VOLUMEONE)
                {
                    mgametextcenter(origin.x, origin.y + (106<<16), "Please read LICENSE.DOC for shareware\n"
                                                                    "distribution grants and restrictions.");
                }
#endif
                mgametextcenter(origin.x, origin.y + ((VOLUMEONE?134:115)<<16), "Made in Dallas, Texas USA");
                break;
            }
        }
        break;
#endif
    case MENU_CREDITS4:   // JBF 20031220
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

    case MENU_CREDITS5:
        l = 7;

        mgametextcenter(origin.x, origin.y + ((38-l)<<16), "License and Other Contributors");
        {
            const char *header[] =
            {
                "This program is distributed under the terms of the",
                "GNU General Public License version 2 as published by the",
                "Free Software Foundation. See gpl-2.0.txt for details.",
                " ",
                "The EDuke32 team thanks the following people for their contributions:",
                " ",
            };
            const char *body[] =
            {
                "Bioman",            // GTK work, APT repository and package upkeep
                "Brandon Bergren",   // "Bdragon" - tiles.cfg
                "Charlie Honig",     // "CONAN" - showview command
                "Dan Gaskill",       // "DeeperThought" - testing
                "David Koenig",      // "Bargle" - Merged a couple of things from duke3d_w32
                "Ed Coolidge",       // Mapster32 improvements
                "Emile Belanger",    // original Android work
                "Hunter_rus",        // tons of stuff
                "James Bentler",     // Mapster32 improvements
                "Jasper Foreman",    // netcode contributions
                "Javier Martinez",   // "Malone3D" - EDuke 2.1.1 components
                "Jeff Hart",         // website graphics
                "Jonathan Strander", // "Mblackwell" - testing and feature speccing
                "Jose del Castillo", // "Renegado" - EDuke 2.1.1 components
                "Lachlan McDonald",  // official EDuke32 icon
                "LSDNinja",          // OS X help and testing
                "Marcus Herbert",    // "rhoenie" - OS X compatibility work
                "Matthew Palmer",    // "Usurper" - testing and eduke32.com domain
                "Matt Saettler",     // original DOS EDuke/WW2GI enhancements
                "Ozkan Sezer",       // SDL/GTK version checking improvements
                "Peter Green",       // "Plugwash" - dynamic remapping, custom gametypes
                "Peter Veenstra",    // "Qbix" - port to 64-bit
                "Robin Green",       // CON array support
                "Ryan Gordon",       // "icculus" - icculus.org Duke3D port sound code
                "Stephen Anthony",   // early 64-bit porting work
                "tueidj",            // Wii port
                " ",
            };
            const char *footer[] =
            {
                " ",
                "BUILD engine technology available under license. See buildlic.txt.",
            };

            const int32_t header_numlines = sizeof(header)/sizeof(char *);
            const int32_t body_numlines = sizeof(body)/sizeof(char *);
            const int32_t footer_numlines = sizeof(footer)/sizeof(char *);

            i = 0;
            for (m=0; m<header_numlines; m++)
                creditsminitext(origin.x + (160<<16), origin.y + ((17+10+10+8+4+(m*7)-l)<<16), header[m], 8);
            i += m;
#define CCOLUMNS 3
#define CCOLXBUF 20
            for (m=0; m<body_numlines; m++)
                creditsminitext(origin.x + ((CCOLXBUF+((320-CCOLXBUF*2)/(CCOLUMNS*2)) +((320-CCOLXBUF*2)/CCOLUMNS)*(m/(body_numlines/CCOLUMNS)))<<16), origin.y + ((17+10+10+8+4+((m%(body_numlines/CCOLUMNS))*7)+(i*7)-l)<<16), body[m], 8);
            i += m/CCOLUMNS;
            for (m=0; m<footer_numlines; m++)
                creditsminitext(origin.x + (160<<16), origin.y + ((17+10+10+8+4+(m*7)+(i*7)-l)<<16), footer[m], 8);

            creditsminitext(origin.x + (160<<16), origin.y + ((138+10+10+10+10+4-l)<<16), "Visit www.eduke32.com for news and updates", 8);
        }

        break;

    default:
        break;
    }
}



static void Menu_PreInput(MenuEntry_t *entry)
{
    switch (g_currentMenu)
    {

    case MENU_KEYBOARDKEYS:
        if (KB_KeyPressed(sc_Delete))
        {
            MenuCustom2Col_t *column = (MenuCustom2Col_t*)entry->entry;
            char key[2];
            key[0] = ud.config.KeyboardKeys[M_KEYBOARDKEYS.currentEntry][0];
            key[1] = ud.config.KeyboardKeys[M_KEYBOARDKEYS.currentEntry][1];
            *column->column[M_KEYBOARDKEYS.currentColumn] = 0xff;
            CONFIG_MapKey(M_KEYBOARDKEYS.currentEntry, ud.config.KeyboardKeys[M_KEYBOARDKEYS.currentEntry][0], key[0], ud.config.KeyboardKeys[M_KEYBOARDKEYS.currentEntry][1], key[1]);
            S_PlaySound(KICK_HIT);
            KB_ClearKeyDown(sc_Delete);
        }
        break;

    default:
        break;
    }
}

static void Menu_PreOptionListDraw(MenuEntry_t *entry, const vec2_t origin)
{
    switch (g_currentMenu)
    {
    case MENU_MOUSEBTNS:
    case MENU_MOUSEADVANCED:
    case MENU_JOYSTICKBTNS:
    case MENU_JOYSTICKAXIS:
        mgametextcenter(origin.x, origin.y + (31<<16), "Select a function to assign");

        Bsprintf(tempbuf, "to %s", entry->name);

        mgametextcenter(origin.x, origin.y + ((31+9)<<16), tempbuf);

        mgametextcenter(origin.x, origin.y + (161<<16), "Press \"Escape\" To Cancel");
        break;
    }
}

static int32_t Menu_PreCustom2ColScreen(MenuEntry_t *entry)
{
    if (g_currentMenu == MENU_KEYBOARDKEYS)
    {
        MenuCustom2Col_t *column = (MenuCustom2Col_t*)entry->entry;

        int32_t sc = KB_GetLastScanCode();
        if (sc != sc_None)
        {
            char key[2];
            key[0] = ud.config.KeyboardKeys[M_KEYBOARDKEYS.currentEntry][0];
            key[1] = ud.config.KeyboardKeys[M_KEYBOARDKEYS.currentEntry][1];

            S_PlaySound(PISTOL_BODYHIT);

            *column->column[M_KEYBOARDKEYS.currentColumn] = KB_GetLastScanCode();

            CONFIG_MapKey(M_KEYBOARDKEYS.currentEntry, ud.config.KeyboardKeys[M_KEYBOARDKEYS.currentEntry][0], key[0], ud.config.KeyboardKeys[M_KEYBOARDKEYS.currentEntry][1], key[1]);

            KB_ClearKeyDown(sc);

            return -1;
        }
    }

    return 0;
}

static void Menu_PreCustom2ColScreenDraw(MenuEntry_t *entry, const vec2_t origin)
{
    if (g_currentMenu == MENU_KEYBOARDKEYS)
    {
        Bsprintf(tempbuf, "Press the key to assign as\n"
                          "%s for \"%s\"\n"
                          "\n"
                          "Press \"Escape\" To Cancel"
                          , M_KEYBOARDKEYS.currentColumn?"secondary":"primary", entry->name);
        mgametextcenter(origin.x, origin.y + (90<<16), tempbuf);
    }
}

static void Menu_EntryFocus(/*MenuEntry_t *entry*/)
{
    switch (g_currentMenu)
    {
    case MENU_LOAD:
        if (M_LOAD.currentEntry < (int32_t)g_nummenusaves)
        {
            savebrief_t & sv = g_menusaves[M_LOAD.currentEntry].brief;
            if (sv.isValid())
                G_LoadSaveHeaderNew(sv.path, &savehead);
        }
        break;

    case MENU_SAVE:
        if (0 < M_SAVE.currentEntry && M_SAVE.currentEntry <= (int32_t)g_nummenusaves)
        {
            savebrief_t & sv = g_menusaves[M_SAVE.currentEntry-1].brief;
            if (sv.isValid())
                G_LoadSaveHeaderNew(sv.path, &savehead);
        }
        break;

    default:
        break;
    }
}

static void Menu_StartGameWithoutSkill(void)
{
    ud.m_player_skill = M_SKILL.currentEntry+1;

    g_skillSoundVoice = S_PlaySound(PISTOL_BODYHIT);

    ud.m_respawn_monsters = 0;

    ud.m_monsters_off = ud.monsters_off = 0;

    ud.m_respawn_items = 0;
    ud.m_respawn_inventory = 0;

    ud.multimode = 1;

    G_NewGame_EnterLevel();
}

static void Menu_DoCheat(int32_t cheatID)
{
    if (numplayers != 1 || !(g_player[myconnectindex].ps->gm & MODE_GAME))
        return;

    osdcmd_cheatsinfo_stat.cheatnum = cheatID;
}

static int32_t Menu_Cheat_Warp(char const * const numbers)
{
    if (numplayers != 1 || !(g_player[myconnectindex].ps->gm & MODE_GAME))
        return 0;

    if (numbers == NULL || !numbers[0] || !numbers[1] || (VOLUMEALL && !numbers[2]))
        return 1;

    if (VOLUMEALL)
    {
        osdcmd_cheatsinfo_stat.volume = numbers[0] - '0';
        osdcmd_cheatsinfo_stat.level = (numbers[1] - '0')*10+(numbers[2]-'0');
    }
    else
    {
        osdcmd_cheatsinfo_stat.volume =  numbers[0] - '0';
        osdcmd_cheatsinfo_stat.level =  numbers[1] - '0';
    }

    osdcmd_cheatsinfo_stat.volume--;
    osdcmd_cheatsinfo_stat.level--;

    if ((VOLUMEONE && osdcmd_cheatsinfo_stat.volume > 0) || osdcmd_cheatsinfo_stat.volume > g_volumeCnt-1 ||
            osdcmd_cheatsinfo_stat.level >= MAXLEVELS || g_mapInfo[osdcmd_cheatsinfo_stat.volume *MAXLEVELS+osdcmd_cheatsinfo_stat.level].filename == NULL)
        return 1;

    osdcmd_cheatsinfo_stat.cheatnum = CHEAT_SCOTTY;

    return 0;
}

static int32_t Menu_Cheat_Skill(char const * const number)
{
    if (numplayers != 1 || !(g_player[myconnectindex].ps->gm & MODE_GAME))
        return 0;

    if (number == NULL || !number[0])
        return 1;

    osdcmd_cheatsinfo_stat.volume = number[0] - '1';

    osdcmd_cheatsinfo_stat.cheatnum = CHEAT_SKILL;

    return 0;
}

/*
Functions where a "newValue" or similar is passed are run *before* the linked variable is actually changed.
That way you can compare the new and old values and potentially block the change.
*/
static void Menu_EntryLinkActivate(MenuEntry_t *entry)
{
    switch (g_currentMenu)
    {
    case MENU_EPISODE:
        if (entry != &ME_EPISODE_USERMAP)
        {
            ud.m_volume_number = M_EPISODE.currentEntry;
            ud.m_level_number = 0;

            if (g_skillCnt == 0)
                Menu_StartGameWithoutSkill();
        }
        break;

    case MENU_SKILL:
    {
        int32_t skillsound = PISTOL_BODYHIT;

        switch (M_SKILL.currentEntry)
        {
        case 0:
            skillsound = JIBBED_ACTOR6;
            break;
        case 1:
            skillsound = BONUS_SPEECH1;
            break;
        case 2:
            skillsound = DUKE_GETWEAPON2;
            break;
        case 3:
            skillsound = JIBBED_ACTOR5;
            break;
        }

        ud.m_player_skill = M_SKILL.currentEntry+1;

        g_skillSoundVoice = S_PlaySound(skillsound);

        if (M_SKILL.currentEntry == 3) ud.m_respawn_monsters = 1;
        else ud.m_respawn_monsters = 0;

        ud.m_monsters_off = ud.monsters_off = 0;

        ud.m_respawn_items = 0;
        ud.m_respawn_inventory = 0;

        ud.multimode = 1;

        G_NewGame_EnterLevel();
        break;
    }

    case MENU_JOYSTICKAXES:
        M_JOYSTICKAXIS.title = getjoyname(0, M_JOYSTICKAXES.currentEntry);
        MEO_JOYSTICKAXIS_ANALOG.data = &ud.config.JoystickAnalogueAxes[M_JOYSTICKAXES.currentEntry];
        MEO_JOYSTICKAXIS_SCALE.variable = &ud.config.JoystickAnalogueScale[M_JOYSTICKAXES.currentEntry];
        MEO_JOYSTICKAXIS_DEAD.variable = &ud.config.JoystickAnalogueDead[M_JOYSTICKAXES.currentEntry];
        MEO_JOYSTICKAXIS_SATU.variable = &ud.config.JoystickAnalogueSaturate[M_JOYSTICKAXES.currentEntry];
        MEO_JOYSTICKAXIS_DIGITALNEGATIVE.data = &ud.config.JoystickDigitalFunctions[M_JOYSTICKAXES.currentEntry][0];
        MEO_JOYSTICKAXIS_DIGITALPOSITIVE.data = &ud.config.JoystickDigitalFunctions[M_JOYSTICKAXES.currentEntry][1];
        break;

    case MENU_CHEATS:
    {
        const int32_t cheatFuncID = M_CHEATS.currentEntry - 1;
        switch (cheatFuncID)
        {
            case -1:
            case CHEATFUNC_WARP:
            case CHEATFUNC_SKILL:
                break;
            default:
                Menu_DoCheat(CheatFunctionIDs[cheatFuncID]);
                break;
        }
        break;
    }

    default:
        break;
    }

    if (entry == &ME_VIDEOSETUP_APPLY)
    {
        resolution_t p = { xdim, ydim, fullscreen, bpp, 0 };
        int32_t prend = getrendermode();
        int32_t pvsync = vsync;

        resolution_t n = { resolution[newresolution].xdim, resolution[newresolution].ydim, newfullscreen,
                           (newrendermode == REND_CLASSIC) ? 8 : resolution[newresolution].bppmax, 0 };
        int32_t nrend = newrendermode;
        int32_t nvsync = newvsync;

        if (setgamemode(n.flags, n.xdim, n.ydim, n.bppmax) < 0)
        {
            if (setgamemode(p.flags, p.xdim, p.ydim, p.bppmax) < 0)
            {
                setrendermode(prend);
                G_GameExit("Failed restoring old video mode.");
            }
            else
            {
                onvideomodechange(p.bppmax > 8);
                vsync = setvsync(pvsync);
            }
        }
        else onvideomodechange(n.bppmax > 8);

        g_restorePalette = -1;
        G_UpdateScreenArea();
        setrendermode(nrend);
        vsync = setvsync(nvsync);
        ud.config.ScreenMode = fullscreen;
        ud.config.ScreenWidth = xdim;
        ud.config.ScreenHeight = ydim;
        ud.config.ScreenBPP = bpp;
    }
    else if (entry == &ME_SOUND_RESTART)
    {
        ud.config.MixRate = soundrate;
        ud.config.NumVoices = soundvoices;

        S_SoundShutdown();
        S_MusicShutdown();

        S_MusicStartup();
        S_SoundStartup();

        FX_StopAllSounds();
        S_ClearSoundLocks();

        if (ud.config.MusicToggle)
            S_RestartMusic();
    }
    else if (entry == &ME_COLCORR_RESET)
    {
        vid_gamma = DEFAULT_GAMMA;
        vid_contrast = DEFAULT_CONTRAST;
        vid_brightness = DEFAULT_BRIGHTNESS;
        ud.brightness = 0;
        r_ambientlight = r_ambientlightrecip = 1.f;
        setbrightness(ud.brightness>>2,g_player[myconnectindex].ps->palette,0);
    }
    else if (entry == &ME_KEYBOARDSETUP_RESET)
        CONFIG_SetDefaultKeys(keydefaults);
    else if (entry == &ME_KEYBOARDSETUP_RESETCLASSIC)
        CONFIG_SetDefaultKeys(oldkeydefaults);
    else if (entry == &ME_NETHOST_LAUNCH)
    {
        // master does whatever it wants
        if (g_netServer)
        {
            Net_FillNewGame(&pendingnewgame, 1);
            Net_StartNewGame();
            Net_SendNewGame(1, NULL);
        }
        else if (voting == -1)
        {
            Net_SendMapVoteInitiate();
            Menu_Change(MENU_NETWAITVOTES);
        }
    }
}

static int32_t Menu_EntryOptionModify(MenuEntry_t *entry, int32_t newOption)
{
    int32_t x;
    DukePlayer_t *ps = g_player[myconnectindex].ps;

    if (entry == &ME_GAMESETUP_DEMOREC)
    {
        if ((ps->gm&MODE_GAME))
            G_CloseDemoWrite();
    }
#ifdef _WIN32
    else if (entry == &ME_GAMESETUP_UPDATES)
        ud.config.LastUpdateCheck = 0;
#endif
    else if (entry == &ME_GAMESETUP_WEAPSWITCH_PICKUP)
    {
        ud.weaponswitch &= ~(1|4);
        switch (newOption)
        {
        case 2:
            ud.weaponswitch |= 4;
            fallthrough__;
        case 1:
            ud.weaponswitch |= 1;
            break;
        default:
            break;
        }
    }
    else if (entry == &ME_SOUND)
    {
        if (newOption == 0)
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();
        }
    }
    else if (entry == &ME_SOUND_MUSIC)
    {
        ud.config.MusicToggle = newOption;

        if (newOption == 0)
            S_PauseMusic(1);
        else
        {
            S_RestartMusic();
            S_PauseMusic(0);
        }
    }
    else if (entry == &ME_SOUND_DUKETALK)
        ud.config.VoiceToggle = (ud.config.VoiceToggle&~1) | newOption;
    else if (entry == &ME_MOUSESETUP_SMOOTH)
        CONTROL_SmoothMouse = ud.config.SmoothInput;
    else if (entry == &ME_JOYSTICKAXIS_ANALOG)
        CONTROL_MapAnalogAxis(M_JOYSTICKAXES.currentEntry, newOption, controldevice_joystick);
    else if (entry == &ME_NETOPTIONS_EPISODE)
    {
        if (newOption < g_volumeCnt)
            ud.m_volume_number = newOption;
    }
    else if (entry == &ME_NETOPTIONS_MONSTERS)
    {
        ud.m_monsters_off = (newOption == g_skillCnt);
        if (newOption < g_skillCnt)
            ud.m_player_skill = newOption;
    }
    else if (entry == &ME_ADULTMODE)
    {
        if (newOption)
        {
            for (x=0; x<g_animWallCnt; x++)
                switch (DYNAMICTILEMAP(wall[animwall[x].wallnum].picnum))
                {
                case FEMPIC1__STATIC:
                    wall[animwall[x].wallnum].picnum = BLANKSCREEN;
                    break;
                case FEMPIC2__STATIC:
                case FEMPIC3__STATIC:
                    wall[animwall[x].wallnum].picnum = SCREENBREAK6;
                    break;
                }

            ud.pwlockout[0] = 0;
            Menu_Change(MENU_ADULTPASSWORD);
//            return -1;
        }
        else
        {
            if (ud.pwlockout[0] == 0)
            {
                ud.lockout = 0;
#if 0
                for (x=0; x<g_numAnimWalls; x++)
                    if (wall[animwall[x].wallnum].picnum != W_SCREENBREAK &&
                            wall[animwall[x].wallnum].picnum != W_SCREENBREAK+1 &&
                            wall[animwall[x].wallnum].picnum != W_SCREENBREAK+2)
                        if (wall[animwall[x].wallnum].extra >= 0)
                            wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
#endif
            }
            else
            {
                Menu_Change(MENU_ADULTPASSWORD);
                return -1;
            }
        }
    }
    else if (entry == &ME_VIDEOSETUP_FRAMELIMIT)
    {
        g_frameDelay = newOption ? (getu64tickspersec()/newOption) : 0;
    }

    switch (g_currentMenu)
    {
    case MENU_MOUSEBTNS:
        CONTROL_MapButton(newOption, MenuMouseDataIndex[M_MOUSEBTNS.currentEntry][0], MenuMouseDataIndex[M_MOUSEBTNS.currentEntry][1], controldevice_mouse);
        CONTROL_FreeMouseBind(MenuMouseDataIndex[M_MOUSEBTNS.currentEntry][0]);
        break;
    case MENU_MOUSEADVANCED:
    {
        size_t i;
        for (i = 0; i < ARRAY_SIZE(MEL_INTERNAL_MOUSEADVANCED_DAXES); i++)
            if (entry == MEL_INTERNAL_MOUSEADVANCED_DAXES[i])
                CONTROL_MapDigitalAxis(i>>1, newOption, i&1, controldevice_mouse);
    }
        break;
    case MENU_JOYSTICKBTNS:
        CONTROL_MapButton(newOption, M_JOYSTICKBTNS.currentEntry>>1, M_JOYSTICKBTNS.currentEntry&1, controldevice_joystick);
        break;
    case MENU_JOYSTICKAXIS:
    {
        size_t i;
        for (i = 0; i < ARRAY_SIZE(MEL_INTERNAL_JOYSTICKAXIS_DIGITAL); i++)
            if (entry == MEL_INTERNAL_JOYSTICKAXIS_DIGITAL[i])
                CONTROL_MapDigitalAxis(i>>1, newOption, i&1, controldevice_joystick);
    }
        break;
    }

    return 0;
}

static void Menu_EntryOptionDidModify(MenuEntry_t *entry)
{
#ifdef USE_OPENGL
    int domodechange = 0;
#endif

    if (entry == &ME_GAMESETUP_AIM_AUTO ||
        entry == &ME_GAMESETUP_WEAPSWITCH_PICKUP ||
        entry == &ME_PLAYER_NAME ||
        entry == &ME_PLAYER_COLOR ||
        entry == &ME_PLAYER_TEAM)
        G_UpdatePlayerFromMenu();
#ifdef USE_OPENGL
    else if (entry == &ME_DISPLAYSETUP_ANISOTROPY || entry == &ME_DISPLAYSETUP_TEXFILTER)
        gltexapplyprops();
    else if (entry == &ME_RENDERERSETUP_TEXQUALITY)
    {
        texcache_invalidate();
        r_downsizevar = r_downsize;
        domodechange = 1;
    }
#ifdef POLYMER
    else if (entry == &ME_POLYMER_LIGHTS ||
             entry == &ME_POLYMER_LIGHTPASSES ||
             entry == &ME_POLYMER_SHADOWCOUNT)
        domodechange = 1;
#endif

    if (domodechange)
    {
        resetvideomode();
        if (setgamemode(fullscreen, xdim, ydim, bpp))
            OSD_Printf("restartvid: Reset failed...\n");
        onvideomodechange(ud.config.ScreenBPP>8);
        G_RefreshLights();
    }
#endif
}

static void Menu_Custom2ColScreen(/*MenuEntry_t *entry*/)
{
    if (g_currentMenu == MENU_KEYBOARDKEYS)
    {
        KB_FlushKeyboardQueue();
        KB_ClearLastScanCode();
    }
}

static int32_t Menu_EntryRangeInt32Modify(MenuEntry_t *entry, int32_t newValue)
{
    if (entry == &ME_SCREENSETUP_SCREENSIZE)
        G_SetViewportShrink(newValue - vpsize);
    else if (entry == &ME_SCREENSETUP_SBARSIZE)
        G_SetStatusBarScale(newValue);
    else if (entry == &ME_SOUND_VOLUME_MASTER)
    {
        FX_SetVolume(newValue);
        S_MusicVolume(MASTER_VOLUME(ud.config.MusicVolume));
    }
    else if (entry == &ME_SOUND_VOLUME_MUSIC)
        S_MusicVolume(MASTER_VOLUME(newValue));
    else if (entry == &ME_MOUSEADVANCED_SCALEX)
        CONTROL_SetAnalogAxisScale(0, newValue, controldevice_mouse);
    else if (entry == &ME_MOUSEADVANCED_SCALEY)
        CONTROL_SetAnalogAxisScale(1, newValue, controldevice_mouse);
    else if (entry == &ME_JOYSTICKAXIS_SCALE)
        CONTROL_SetAnalogAxisScale(M_JOYSTICKAXES.currentEntry, newValue, controldevice_joystick);
    else if (entry == &ME_JOYSTICKAXIS_DEAD)
        setjoydeadzone(M_JOYSTICKAXES.currentEntry, newValue, *MEO_JOYSTICKAXIS_SATU.variable);
    else if (entry == &ME_JOYSTICKAXIS_SATU)
        setjoydeadzone(M_JOYSTICKAXES.currentEntry, *MEO_JOYSTICKAXIS_DEAD.variable, newValue);

    return 0;
}

static int32_t Menu_EntryRangeFloatModify(MenuEntry_t *entry, float newValue)
{
#ifndef EDUKE32_SIMPLE_MENU
    if (entry == &ME_COLCORR_AMBIENT)
        r_ambientlightrecip = 1.f/newValue;
#else
    UNREFERENCED_PARAMETER(entry);
    UNREFERENCED_PARAMETER(newValue);
#endif

    return 0;
}

static int32_t Menu_EntryRangeFloatDidModify(MenuEntry_t *entry)
{
    if (entry == &ME_COLCORR_GAMMA)
    {
        ud.brightness = GAMMA_CALC<<2;
        setbrightness(ud.brightness>>2, g_player[myconnectindex].ps->palette, 0);
    }
    else if (entry == &ME_COLCORR_CONTRAST || entry == &ME_COLCORR_BRIGHTNESS)
    {
        setbrightness(ud.brightness>>2, g_player[myconnectindex].ps->palette, 0);
    }

    return 0;
}

#ifdef MENU_ENABLE_RANGEDOUBLE
static int32_t Menu_EntryRangeDoubleModify(void /*MenuEntry_t *entry, double newValue*/)
{

    return 0;
}
#endif

static uint32_t save_xxh = 0;

static void Menu_EntryStringActivate(/*MenuEntry_t *entry*/)
{
    switch (g_currentMenu)
    {
    case MENU_SAVE:
        if (M_SAVE.currentEntry > 0)
        {
            savebrief_t & sv = g_menusaves[M_SAVE.currentEntry-1].brief;
            if (!save_xxh)
                save_xxh = XXH32((uint8_t *)sv.name, MAXSAVEGAMENAME, 0xDEADBEEF);
            if (sv.isValid())
                Menu_Change(MENU_SAVEVERIFY);
        }
        else
        {
            ME_SAVE_NEW.name = nullptr;
            save_xxh = 0;
        }
        break;

    default:
        break;
    }
}

static int32_t Menu_EntryStringSubmit(/*MenuEntry_t *entry, */char *input)
{
    int32_t returnvar = 0;

    switch (g_currentMenu)
    {
    case MENU_SAVE:
    {
        savebrief_t & sv = g_lastusersave = M_SAVE.currentEntry == 0 ? savebrief_t{input} : g_menusaves[M_SAVE.currentEntry-1].brief;

        // dirty hack... char 127 in last position indicates an auto-filled name
#ifdef __ANDROID__
        if (1)
#else
        if (input[0] == 0 || (sv.name[MAXSAVEGAMENAME] == 127 &&
            strncmp(sv.name, input, MAXSAVEGAMENAME) == 0 &&
            save_xxh == XXH32((uint8_t *)sv.name, MAXSAVEGAMENAME, 0xDEADBEEF)))
#endif
        {
            strncpy(sv.name, g_mapInfo[ud.volume_number * MAXLEVELS + ud.level_number].name, MAXSAVEGAMENAME);
            sv.name[MAXSAVEGAMENAME] = 127;
            returnvar = -1;
        }
        else
        {
            strncpy(sv.name, input, MAXSAVEGAMENAME);
            sv.name[MAXSAVEGAMENAME] = 0;
        }

        G_SavePlayerMaybeMulti(sv);

        g_quickload = &sv;
        g_player[myconnectindex].ps->gm = MODE_GAME;

        Menu_Change(MENU_CLOSE);
        save_xxh = 0;
        break;
    }

    default:
        break;
    }

    return returnvar;
}

static void Menu_EntryStringCancel(/*MenuEntry_t *entry*/)
{
    switch (g_currentMenu)
    {
    case MENU_SAVE:
        save_xxh = 0;
        ME_SAVE_NEW.name = s_NewSaveGame;
        break;

    default:
        break;
    }
}

/*
This is polled when the menu code is populating the screen but for some reason doesn't have the data.
*/
static int32_t Menu_EntryOptionSource(MenuEntry_t *entry, int32_t currentValue)
{
    if (entry == &ME_GAMESETUP_WEAPSWITCH_PICKUP)
        return (ud.weaponswitch & 1) ? ((ud.weaponswitch & 4) ? 2 : 1) : 0;
    else if (entry == &ME_SOUND_DUKETALK)
        return ud.config.VoiceToggle & 1;
    else if (entry == &ME_NETOPTIONS_MONSTERS)
        return (ud.m_monsters_off ? g_skillCnt : ud.m_player_skill);

    return currentValue;
}

static void Menu_Verify(int32_t input)
{
    switch (g_currentMenu)
    {
    case MENU_RESETPLAYER:
        if (input)
        {
            KB_FlushKeyboardQueue();
            KB_ClearKeysDown();
            FX_StopAllSounds();
            S_ClearSoundLocks();

            G_LoadPlayerMaybeMulti(*g_quickload);
        }
        else
        {
            if (sprite[g_player[myconnectindex].ps->i].extra <= 0)
            {
                if (G_EnterLevel(MODE_GAME)) G_BackToMenu();
                return;
            }

            Menu_Change(MENU_CLOSE);
        }
        break;

    case MENU_LOADVERIFY:
        if (input)
        {
            savebrief_t & sv = g_menusaves[M_LOAD.currentEntry].brief;

            if (strcmp(sv.path, g_lastusersave.path) != 0)
            {
                g_freshload = sv;
                g_lastusersave.reset();
                g_lastautosave.reset();
                g_quickload = &g_freshload;
            }
            else
            {
                g_quickload = &g_lastusersave;
            }

            KB_FlushKeyboardQueue();
            KB_ClearKeysDown();

            Menu_Change(MENU_CLOSE);

            G_LoadPlayerMaybeMulti(sv);
        }
        break;

    case MENU_SAVEVERIFY:
        if (!input)
        {
            save_xxh = 0;

            ((MenuString_t*)M_SAVE.entrylist[M_SAVE.currentEntry]->entry)->editfield = NULL;
        }
        break;

    case MENU_QUIT:
    case MENU_QUIT_INGAME:
        if (input)
            G_GameQuit();
        else
            g_quitDeadline = 0;
        break;

    case MENU_QUITTOTITLE:
        if (input)
        {
            g_player[myconnectindex].ps->gm = MODE_DEMO;
            if (ud.recstat == 1)
                G_CloseDemoWrite();
            E_MapArt_Clear();
        }
        break;

    case MENU_NETWAITVOTES:
        if (!input)
            Net_SendMapVoteCancel(0);
        break;

    default:
        break;
    }
}

static int Menu_CheatStringMatch(char const * input, char const * cheat)
{
    while (*cheat || *input)
    {
        if (*cheat != *input)
        {
            if (!(*cheat == '#' && Bisdigit(*input)))
                return 0;
        }

        ++cheat;
        ++input;
    }

    return 1;
}

static void Menu_TextFormSubmit(char *input)
{
    switch (g_currentMenu)
    {
    case MENU_ADULTPASSWORD:
        if (Bstrlen(input) && (ud.pwlockout[0] == 0 || ud.lockout == 0))
            Bstrcpy(&ud.pwlockout[0], input);
        else if (Bstrcmp(input, &ud.pwlockout[0]) == 0)
        {
            for (bssize_t x=0; x<g_animWallCnt; x++)
                if ((unsigned) animwall[x].wallnum < (unsigned)numwalls && wall[animwall[x].wallnum].picnum != W_SCREENBREAK &&
                        wall[animwall[x].wallnum].picnum != W_SCREENBREAK+1 &&
                        wall[animwall[x].wallnum].picnum != W_SCREENBREAK+2)
                    if (wall[animwall[x].wallnum].extra >= 0)
                        wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
            ud.lockout = 0;
        }

        S_PlaySound(PISTOL_BODYHIT);
        Menu_Change(MENU_GAMESETUP);
        break;

    case MENU_CHEATENTRY:
    {
        const size_t inputlength = Bstrlen(input);
        Bstrcpy(tempbuf, input);
        for (size_t i = 0; i < inputlength; i++)
            tempbuf[i] = Btolower(tempbuf[i]);

        int8_t cheatID = -1;

        if (inputlength > 2 && tempbuf[0] == scantoasc[CheatKeys[0]] && tempbuf[1] == scantoasc[CheatKeys[1]])
        {
            for (size_t i = 0; i < NUMCHEATS; i++)
                if (Menu_CheatStringMatch(tempbuf+2, CheatStrings[i]))
                {
                    cheatID = i;
                    break;
                }
        }

        switch (cheatID)
        {
            case -1:
                S_PlaySound(KICK_HIT);
                break;
            case CHEAT_SCOTTY:
            {
                char const * const numberpos = Bstrchr(CheatStrings[CHEAT_SCOTTY], '#');
                if (numberpos == NULL)
                {
                    S_PlaySound(KICK_HIT);
                    break;
                }

                Menu_Cheat_Warp(input + (numberpos - CheatStrings[CHEAT_SCOTTY]) + 2);
                if (g_player[myconnectindex].ps->gm&MODE_MENU)
                    S_PlaySound(DUKE_GET);
                break;
            }
            case CHEAT_SKILL:
            {
                char const * const numberpos = Bstrchr(CheatStrings[CHEAT_SKILL], '#');
                if (numberpos == NULL)
                {
                    S_PlaySound(KICK_HIT);
                    break;
                }

                Menu_Cheat_Skill(input + (numberpos - CheatStrings[CHEAT_SKILL]) + 2);
                if (g_player[myconnectindex].ps->gm&MODE_MENU)
                    S_PlaySound(DUKE_GET);
                break;
            }
            default:
                Menu_DoCheat(cheatID);
                S_PlaySound(DUKE_GET);
                break;
        }

        if (cheatID >= 0)
            cl_cheatmask |= CheatFunctionFlags[cheatID];

        if ((NAM_WW2GI && (cl_cheatmask & (1<<CHEATFUNC_QUOTETODD))) ||
            ((cl_cheatmask & (1<<CHEATFUNC_QUOTEBETA)) && (cl_cheatmask & (1<<CHEATFUNC_QUOTETODD)) && (cl_cheatmask & (1<<CHEATFUNC_QUOTEALLEN))))
        {
            S_PlaySound(DUKE_GETWEAPON6);
            cl_cheatmask = ~0;
        }

        Menu_Change(MENU_CHEATS);
        break;
    }

    case MENU_CHEAT_WARP:
        if (Menu_Cheat_Warp(input))
            S_PlaySound(KICK_HIT);
        Menu_Change(MENU_CHEATS);
        break;

    case MENU_CHEAT_SKILL:
        if (Menu_Cheat_Skill(input))
            S_PlaySound(KICK_HIT);
        Menu_Change(MENU_CHEATS);
        break;

    default:
        break;
    }
}

void klistbookends(CACHE1D_FIND_REC *start)
{
    CACHE1D_FIND_REC *end = start, *n;
    size_t i = 0;

    if (!start)
        return;

    while (start->prev)
        start = start->prev;

    while (end->next)
        end = end->next;

    for (n = start; n; n = n->next)
    {
        n->type = i; // overload this...
        n->usera = start;
        n->userb = end;
        i++;
    }
}

static void Menu_FileSelectInit(MenuFileSelect_t *object)
{
    size_t i;

    fnlist_clearnames(&object->fnlist);

    if (object->destination[0] == 0)
        Bstrcpy(object->destination, "./");
    Bcorrectfilename(object->destination, 1);

    fnlist_getnames(&object->fnlist, object->destination, object->pattern, 0, 0);
    object->findhigh[0] = object->fnlist.finddirs;
    object->findhigh[1] = object->fnlist.findfiles;

    for (i = 0; i < 2; ++i)
    {
        object->scrollPos[i] = 0;
        klistbookends(object->findhigh[i]);
    }

    object->currentList = 0;
    if (object->findhigh[1])
        object->currentList = 1;

    KB_FlushKeyboardQueue();
}

static void Menu_FileSelect(int32_t input)
{
    switch (g_currentMenu)
    {
    case MENU_NETUSERMAP:
        if ((g_netServer || ud.multimode > 1))
            Net_SendUserMapName();
        fallthrough__;
    case MENU_USERMAP:
        if (input)
        {
            ud.m_volume_number = 0;
            ud.m_level_number = 7;

            if (g_skillCnt > 0)
                Menu_AnimateChange(MENU_SKILL, MA_Advance);
            else
                Menu_StartGameWithoutSkill();
        }
        break;

    default:
        break;
    }
}





static Menu_t* Menu_BinarySearch(MenuID_t query, size_t searchstart, size_t searchend)
{
    const size_t thissearch = (searchstart + searchend) / 2;
    const MenuID_t difference = query - Menus[thissearch].menuID;

    if (difference == 0)
        return &Menus[thissearch];
    else if (searchstart == searchend)
        return NULL;
    else if (difference > 0)
    {
        if (thissearch == searchend)
            return NULL;
        searchstart = thissearch + 1;
    }
    else if (difference < 0)
    {
        if (thissearch == searchstart)
            return NULL;
        searchend = thissearch - 1;
    }

    return Menu_BinarySearch(query, searchstart, searchend);
}

static Menu_t* Menu_Find(MenuID_t query)
{
    if ((unsigned) query > (unsigned) Menus[numMenus-1].menuID)
        return NULL;

    return Menu_BinarySearch(query, 0, numMenus-1);
}

static Menu_t* Menu_FindFiltered(MenuID_t query)
{
    if ((g_player[myconnectindex].ps->gm&MODE_GAME) && query == MENU_MAIN)
        query = MENU_MAIN_INGAME;

    return Menu_Find(query);
}

MenuAnimation_t m_animation;

int32_t Menu_Anim_SinOutRight(MenuAnimation_t *animdata)
{
    return sintable[divscale10(totalclock - animdata->start, animdata->length) + 512] - 16384;
}
int32_t Menu_Anim_SinInRight(MenuAnimation_t *animdata)
{
    return sintable[divscale10(totalclock - animdata->start, animdata->length) + 512] + 16384;
}
int32_t Menu_Anim_SinOutLeft(MenuAnimation_t *animdata)
{
    return -sintable[divscale10(totalclock - animdata->start, animdata->length) + 512] + 16384;
}
int32_t Menu_Anim_SinInLeft(MenuAnimation_t *animdata)
{
    return -sintable[divscale10(totalclock - animdata->start, animdata->length) + 512] - 16384;
}

void Menu_AnimateChange(int32_t cm, MenuAnimationType_t animtype)
{
    if (KXDWN)
    {
        m_animation.start  = 0;
        m_animation.length = 0;
        Menu_Change(cm);
        return;
    }

    switch (animtype)
    {
        case MA_Advance:
        {
            Menu_t * const previousMenu = m_currentMenu;

            if (!Menu_Change(cm))
            {
                m_animation.out    = Menu_Anim_SinOutRight;
                m_animation.in     = Menu_Anim_SinInRight;
                m_animation.start  = totalclock;
                m_animation.length = 30;

                m_animation.previous = previousMenu;
                m_animation.current  = m_currentMenu;
            }

            break;
        }
        case MA_Return:
        {
            Menu_t * const previousMenu = m_currentMenu;

            if (!Menu_Change(cm))
            {
                m_animation.out    = Menu_Anim_SinOutLeft;
                m_animation.in     = Menu_Anim_SinInLeft;
                m_animation.start  = totalclock;
                m_animation.length = 30;

                m_animation.previous = previousMenu;
                m_animation.current  = m_currentMenu;
            }

            break;
        }
        default:
            m_animation.start  = 0;
            m_animation.length = 0;
            Menu_Change(cm);
            break;
    }
}

static void Menu_MaybeSetSelectionToChild(Menu_t * m, MenuID_t id)
{
    if (m->type == Menu)
    {
        MenuMenu_t * menu = (MenuMenu_t *)m->object;

        for (size_t i = 0, i_end = menu->numEntries; i < i_end; ++i)
        {
            MenuEntry_t const * entry = menu->entrylist[i];
            if (entry != NULL && entry->type == Link)
            {
                MenuLink_t const * link = (MenuLink_t const *)entry->entry;
                if (link->linkID == id)
                {
                    menu->currentEntry = i;
                    Menu_AdjustForCurrentEntryAssignmentBlind(menu);
                    break;
                }
            }
        }
    }
}

static void Menu_ReadSaveGameHeaders()
{
    ReadSaveGameHeaders();

    size_t const numloaditems = max(g_nummenusaves, 1), numsaveitems = g_nummenusaves+1;
    ME_LOAD = (MenuEntry_t *)realloc(ME_LOAD, g_nummenusaves * sizeof(MenuEntry_t));
    MEL_LOAD = (MenuEntry_t **)realloc(MEL_LOAD, numloaditems * sizeof(MenuEntry_t *));
    MEO_SAVE = (MenuString_t *)realloc(MEO_SAVE, g_nummenusaves * sizeof(MenuString_t));
    ME_SAVE = (MenuEntry_t *)realloc(ME_SAVE, g_nummenusaves * sizeof(MenuEntry_t));
    MEL_SAVE = (MenuEntry_t **)realloc(MEL_SAVE, numsaveitems * sizeof(MenuEntry_t *));

    MEL_SAVE[0] = &ME_SAVE_NEW;
    ME_SAVE_NEW.name = s_NewSaveGame;
    for (size_t i = 0; i < g_nummenusaves; ++i)
    {
        MEL_LOAD[i] = &ME_LOAD[i];
        MEL_SAVE[i+1] = &ME_SAVE[i];
        ME_LOAD[i] = ME_LOAD_TEMPLATE;
        ME_SAVE[i] = ME_SAVE_TEMPLATE;
        ME_SAVE[i].entry = &MEO_SAVE[i];
        MEO_SAVE[i] = MEO_SAVE_TEMPLATE;

        ME_LOAD[i].name = g_menusaves[i].brief.name;
        MEO_SAVE[i].variable = g_menusaves[i].brief.name;
    }

    if (g_nummenusaves == 0)
        MEL_LOAD[0] = &ME_LOAD_EMPTY;

    M_LOAD.entrylist = MEL_LOAD;
    M_LOAD.numEntries = numloaditems;
    M_SAVE.entrylist = MEL_SAVE;
    M_SAVE.numEntries = numsaveitems;

    // lexicographical sorting?
}

static void Menu_AboutToStartDisplaying(Menu_t * m)
{
    switch (m->menuID)
    {
    case MENU_MAIN:
        if (KXDWN)
            ME_MAIN_LOADGAME.name = s_Continue;
        break;

    case MENU_MAIN_INGAME:
        if (KXDWN)
            ME_MAIN_LOADGAME.name = s_LoadGame;
        break;

    case MENU_LOAD:
        if (KXDWN)
            M_LOAD.title = (g_player[myconnectindex].ps->gm & MODE_GAME) ? s_LoadGame : s_Continue;

        Menu_ReadSaveGameHeaders();

        for (size_t i = 0; i < g_nummenusaves; ++i)
            MenuEntry_LookDisabledOnCondition(&ME_LOAD[i], g_menusaves[i].isOldVer);

        if (g_quickload && g_quickload->isValid())
        {
            for (size_t i = 0; i < g_nummenusaves; ++i)
            {
                if (strcmp(g_menusaves[i].brief.path, g_quickload->path) == 0)
                {
                    M_LOAD.currentEntry = i;
                    Menu_AdjustForCurrentEntryAssignmentBlind(&M_LOAD);
                    break;
                }
            }
        }
        break;

    case MENU_SAVE:
        if (g_previousMenu == MENU_SAVEVERIFY)
            break;

        Menu_ReadSaveGameHeaders();

        if (g_lastusersave.isValid())
        {
            for (size_t i = 0; i < g_nummenusaves; ++i)
            {
                if (strcmp(g_menusaves[i].brief.path, g_lastusersave.path) == 0)
                {
                    M_SAVE.currentEntry = i+1;
                    Menu_AdjustForCurrentEntryAssignmentBlind(&M_SAVE);
                    break;
                }
            }
        }

        for (size_t i = 0; i < g_nummenusaves; ++i)
            MenuEntry_LookDisabledOnCondition(&ME_SAVE[i], g_menusaves[i].isOldVer);

        if (g_player[myconnectindex].ps->gm&MODE_GAME)
        {
            g_screenCapture = 1;
            G_DrawRooms(myconnectindex,65536);
            g_screenCapture = 0;
        }
        break;

    case MENU_VIDEOSETUP:
        newresolution = 0;
        for (size_t i = 0; i < MAXVALIDMODES; ++i)
        {
            if (resolution[i].xdim == xdim && resolution[i].ydim == ydim)
            {
                newresolution = i;
                break;
            }
        }
        newrendermode = getrendermode();
        newfullscreen = fullscreen;
        newvsync = vsync;
        break;

    case MENU_ADVSOUND:
        soundrate = ud.config.MixRate;
        soundvoices = ud.config.NumVoices;
        break;

    default:
        break;
    }

    switch (m->type)
    {
    case TextForm:
        typebuf[0] = 0;
        ((MenuTextForm_t*)m->object)->input = typebuf;
        break;
    case FileSelect:
        Menu_FileSelectInit((MenuFileSelect_t*)m->object);
        break;
    case Menu:
    {
        MenuMenu_t *menu = (MenuMenu_t*)m->object;
        // MenuEntry_t* currentry = menu->entrylist[menu->currentEntry];

        // need this for MENU_SKILL
        if (menu->currentEntry >= menu->numEntries)
            menu->currentEntry = 0;

        int32_t i = menu->currentEntry;
        while (!menu->entrylist[menu->currentEntry] || ((MenuEntry_t*)menu->entrylist[menu->currentEntry])->type == Spacer)
        {
            menu->currentEntry++;
            if (menu->currentEntry >= menu->numEntries)
                menu->currentEntry = 0;
            if (menu->currentEntry == i)
                G_GameExit("Menu_Change: Attempted to show a menu with no entries.");
        }

        Menu_EntryFocus(/*currentry*/);
        break;
    }
    default:
        break;
    }
}

static void Menu_ChangingTo(Menu_t * m)
{
    switch (m->menuID)
    {
#ifdef __ANDROID__
    case MENU_TOUCHBUTTONS:
        AndroidToggleButtonEditor();
        break;
#endif
    default:
        break;
    }

    switch (m->type)
    {
    case TextForm:
        WithSDL2_StartTextInput();
        break;
    default:
        break;
    }
}

int Menu_Change(MenuID_t cm)
{
    Menu_t * beginMenu = m_currentMenu;

    cm = VM_OnEventWithReturn(EVENT_CHANGEMENU, g_player[screenpeek].ps->i, screenpeek, cm);

    if (cm == MENU_PREVIOUS)
    {
        m_currentMenu = m_previousMenu;
        g_currentMenu = g_previousMenu;
    }
    else if (cm == MENU_CLOSE)
        Menu_Close(myconnectindex);
    else if (cm >= 0)
    {
        Menu_t * search = Menu_FindFiltered(cm);

        if (search == NULL)
            return 0; // intentional, so that users don't use any random value as "don't change"

        m_previousMenu = m_currentMenu;
        g_previousMenu = g_currentMenu;
        m_currentMenu = search;
        g_currentMenu = search->menuID;
    }
    else
        return 1;

    if (KXDWN)
    {
        Menu_t * parent = m_currentMenu, * result = NULL;

        while (parent != NULL && parent->menuID != MENU_OPTIONS && parent->menuID != MENU_MAIN && parent->menuID != MENU_MAIN_INGAME)
        {
            result = parent = Menu_FindFiltered(parent->parentID);
        }

        m_parentMenu = result;

        if (result)
        {
            Menu_MaybeSetSelectionToChild(result, m_currentMenu->menuID);
            Menu_AboutToStartDisplaying(result);
        }
    }

    Menu_MaybeSetSelectionToChild(m_currentMenu, beginMenu->menuID);
    Menu_AboutToStartDisplaying(m_currentMenu);
    Menu_ChangingTo(m_currentMenu);

#if !defined EDUKE32_TOUCH_DEVICES
    m_menuchange_watchpoint = 1;
#endif

    return 0;
}










void G_CheckPlayerColor(int32_t *color, int32_t prev_color)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(MEOSV_PLAYER_COLOR); ++i)
        if (*color == MEOSV_PLAYER_COLOR[i])
            return;

    *color = prev_color;
}


int32_t Menu_DetermineSpecialState(MenuEntry_t *entry)
{
    if (entry == NULL)
        return 0;

    if (entry->type == String)
    {
        if (((MenuString_t*)entry->entry)->editfield)
            return 1;
    }
    else if (entry->type == Option)
    {
        if (((MenuOption_t*)entry->entry)->options->currentEntry >= 0)
            return 2;
    }
    else if (entry->type == Custom2Col)
    {
        if (((MenuCustom2Col_t*)entry->entry)->screenOpen)
            return 2;
    }

    return 0;
}

int32_t Menu_IsTextInput(Menu_t *cm)
{
    switch (m_currentMenu->type)
    {
        case Verify:
        case TextForm:
        case FileSelect:
        case Message:
            return 1;
            break;
        case Panel:
            return 0;
            break;
        case Menu:
        {
            MenuMenu_t *menu = (MenuMenu_t*)cm->object;
            MenuEntry_t *entry = menu->entrylist[menu->currentEntry];
            return Menu_DetermineSpecialState(entry);
        }
            break;
    }

    return 0;
}

static inline int32_t Menu_BlackTranslucentBackgroundOK(MenuID_t cm)
{
    switch (cm)
    {
        case MENU_COLCORR:
        case MENU_COLCORR_INGAME:
            return 0;
            break;
        default:
            return 1;
            break;
    }

    return 1;
}

static inline int32_t Menu_UpdateScreenOK(MenuID_t cm)
{
    switch (cm)
    {
        case MENU_LOAD:
        case MENU_SAVE:
        case MENU_LOADVERIFY:
        case MENU_SAVEVERIFY:
            return 0;
            break;
        default:
            return 1;
            break;
    }

    return 1;
}


/*
    Code below this point is entirely general,
    so if you want to change or add a menu,
    chances are you should scroll up.
*/

int32_t m_mouselastactivity;
#if !defined EDUKE32_TOUCH_DEVICES
int32_t m_mousewake_watchpoint, m_menuchange_watchpoint;
#endif
int32_t m_mousecaught;
static vec2_t m_prevmousepos, m_mousepos, m_mousedownpos;

void Menu_Open(size_t playerID)
{
    g_player[playerID].ps->gm |= MODE_MENU;

    readmouseabsxy(&m_prevmousepos, &mouseabs);
    m_mouselastactivity = -M_MOUSETIMEOUT;

#if !defined EDUKE32_TOUCH_DEVICES
    m_mousewake_watchpoint = 0;
#endif

    AppGrabMouse(0);
}

void Menu_Close(size_t playerID)
{
    if (g_player[playerID].ps->gm & MODE_GAME)
    {
        // The following lines are here so that you cannot close the menu when no game is running.
        g_player[playerID].ps->gm &= ~MODE_MENU;
        AppGrabMouse(1);

        if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
        {
            ready2send = 1;
            totalclock = ototalclock;
            CAMERACLOCK = totalclock;
            CAMERADIST = 65536;
            m_animation.start = 0;
            m_animation.length = 0;

            // Reset next-viewscreen-redraw counter.
            // XXX: are there any other cases like that in need of handling?
            if (g_curViewscreen >= 0)
                actor[g_curViewscreen].t_data[0] = totalclock;
        }

        walock[TILE_SAVESHOT] = 199;
        G_UpdateScreenArea();
    }
}

static int32_t x_widescreen_left(void)
{
    return (320<<15) - scale(240<<15, xdim, ydim);
}

static int32_t xdim_from_320_16(int32_t x)
{
    const int32_t screenwidth = scale(240<<16, xdim, ydim);
    return scale(x + (screenwidth>>1) - (160<<16), xdim, screenwidth);
}
static int32_t ydim_from_200_16(int32_t y)
{
    return scale(y, ydim, 200<<16);
}

static void Menu_BlackRectangle(int32_t x, int32_t y, int32_t width, int32_t height, int32_t orientation)
{
    const int32_t xscale = divscale16(width, tilesiz[0].x<<16), yscale = divscale16(height, tilesiz[0].y<<16);

    rotatesprite_(x, y, max(xscale, yscale), 0, 0, 127, 4, (orientation&(1|32))|2|8|16, 0, 0, xdim_from_320_16(x), ydim_from_200_16(y), xdim_from_320_16(x + width), ydim_from_200_16(y + height));
}

enum MenuTextFlags_t
{
    MT_Selected = 1<<0,
    MT_Disabled = 1<<1,
    MT_XCenter  = 1<<2,
    MT_XRight   = 1<<3,
    MT_YCenter  = 1<<4,
    MT_Literal  = 1<<5,
};

static void Menu_GetFmt(const MenuFont_t *font, uint8_t const status, int32_t *s, int32_t *p, int32_t *z)
{
    if (KXDWN) *z = (status & MT_Selected) ? *z + (*z >> 4) : *z;
    *s = (status & MT_Selected) ? (sintable[(totalclock<<5)&2047]>>12) : font->shade_deselected;
    *p = (status & MT_Disabled) ? font->pal_disabled : font->pal;
}

static vec2_t Menu_Text(int32_t x, int32_t y, const MenuFont_t *font, const char *t, uint8_t status, int32_t ydim_upper, int32_t ydim_lower)
{
    int32_t s, p, ybetween = font->between.y;
    int32_t f = font->textflags;
    if (status & MT_XCenter)
        f |= TEXT_XCENTER;
    if (status & MT_XRight)
        f |= TEXT_XRIGHT;
    if (status & MT_YCenter)
    {
        f |= TEXT_YCENTER | TEXT_YOFFSETZERO;
        ybetween = font->emptychar.y; // <^ the battle against 'Q'
    }
    if (status & MT_Literal)
        f |= TEXT_LITERALESCAPE;

    int32_t z = font->zoom;

    Menu_GetFmt(font, status, &s, &p, &z);

    return G_ScreenText(font->tilenum, x, y, z, 0, 0, t, s, p, 2|8|16|ROTATESPRITE_FULL16, 0, font->emptychar.x, font->emptychar.y, font->between.x, ybetween, f, 0, ydim_upper, xdim-1, ydim_lower);
}

#if 0
static vec2_t Menu_TextSize(int32_t x, int32_t y, const MenuFont_t *font, const char *t, uint8_t status)
{
    int32_t f = font->textflags;
    if (status & MT_Literal)
        f |= TEXT_LITERALESCAPE;

    return G_ScreenTextSize(font->tilenum, x, y, font->zoom, 0, t, 2|8|16|ROTATESPRITE_FULL16, font->emptychar.x, font->emptychar.y, font->between.x, font->between.y, f, 0, 0, xdim-1, ydim-1);
}
#endif

static int32_t Menu_FindOptionBinarySearch(MenuOption_t *object, const int32_t query, size_t searchstart, size_t searchend)
{
    const size_t thissearch = (searchstart + searchend) / 2;
    const bool isIdentityMap = object->options->optionValues == NULL;

    const int32_t destination = isIdentityMap ? (int32_t) thissearch : object->options->optionValues[thissearch];

    const int32_t difference = query - destination;

    Bassert(!isIdentityMap || query >= 0);

    if (difference == 0)
        return thissearch;
    else if (searchstart == searchend)
        return -1;
    else if (difference > 0)
    {
        if (thissearch == searchend)
            return -1;
        searchstart = thissearch + 1;
    }
    else if (difference < 0)
    {
        if (thissearch == searchstart)
            return -1;
        searchend = thissearch - 1;
    }

    return Menu_FindOptionBinarySearch(object, query, searchstart, searchend);
}

static int32_t Menu_MouseOutsideBounds(vec2_t const * const pos, const int32_t x, const int32_t y, const int32_t width, const int32_t height)
{
    return pos->x < x || pos->x >= x + width || pos->y < y || pos->y >= y + height;
}

static void Menu_RunScrollbar(Menu_t *cm, MenuMenuFormat_t const * const format, const int32_t totalextent, int32_t * const scrollPos, const int32_t rightedge, const vec2_t origin)
{
    if (totalextent > klabs(format->bottomcutoff))
    {
        const int32_t scrollx = origin.x + rightedge - (tilesiz[SELECTDIR].x<<16), scrolly = origin.y + format->pos.y;
        const int32_t scrollwidth = tilesiz[SELECTDIR].x<<16;
        const int32_t scrollheight = klabs(format->bottomcutoff) - format->pos.y;
        const int32_t scrollregionheight = scrollheight - (tilesiz[SELECTDIR].y<<16);
        const int32_t scrollPosMax = totalextent - klabs(format->bottomcutoff);

        Menu_BlackRectangle(scrollx, scrolly, scrollwidth, scrollheight, 1|32);

        rotatesprite_fs(scrollx, scrolly + scale(scrollregionheight, *scrollPos, scrollPosMax), 65536, 0, SELECTDIR, 0, 0, 26);

        if (cm == m_currentMenu && !m_mousecaught && MOUSEACTIVECONDITIONAL(mousepressstate == Mouse_Pressed || mousepressstate == Mouse_Held))
        {
            const int32_t scrolltilehalfheight = tilesiz[SELECTDIR].y<<15;
            const int32_t scrollregiony = scrolly + scrolltilehalfheight;

            // region between the y-midline of the arrow at the extremes scrolls proportionally
            if (!Menu_MouseOutsideBounds(&m_mousepos, scrollx, scrollregiony, scrollwidth, scrollregionheight))
            {
                *scrollPos = scale(m_mousepos.y - scrollregiony, scrollPosMax, scrollregionheight);

                m_mousecaught = 1;
            }
            // region outside the y-midlines clamps to the extremes
            else if (!Menu_MouseOutsideBounds(&m_mousepos, scrollx, scrolly, scrollwidth, scrollheight))
            {
                if (m_mousepos.y > scrolly + scrollheight/2)
                    *scrollPos = scrollPosMax;
                else
                    *scrollPos = 0;

                m_mousecaught = 1;
            }
        }
    }
}

typedef enum MenuMovement_t
{
    MM_Up = 1,
    MM_End = 3,
    MM_Down = 4,
    MM_Home = 12,
    MM_Left = 16,
    MM_AllTheWayLeft = 48,
    MM_Right = 64,
    MM_AllTheWayRight = 192,
    MM_Swap = 80,
} MenuMovement_t;

static MenuEntry_t *Menu_RunInput_Menu_MovementVerify(MenuMenu_t *menu);
static MenuEntry_t *Menu_RunInput_Menu_Movement(MenuMenu_t *menu, MenuMovement_t direction);
static void Menu_RunInput_EntryLink_Activate(MenuEntry_t *entry);
static void Menu_RunInput_EntryOptionList_MovementVerify(MenuOption_t *object);
static void Menu_RunInput_EntryOptionList_Movement(MenuOption_t *object, MenuMovement_t direction);
static int32_t Menu_RunInput_EntryOption_Modify(MenuEntry_t *entry, MenuOption_t *object, int32_t newValueIndex);
static int32_t Menu_RunInput_EntryOption_Movement(MenuEntry_t *entry, MenuOption_t *object, MenuMovement_t direction);
static int32_t Menu_RunInput_EntryOption_Activate(MenuEntry_t *entry, MenuOption_t *object);
static int32_t Menu_RunInput_EntryOptionList_Activate(MenuEntry_t *entry, MenuOption_t *object);
static void Menu_RunInput_EntryCustom2Col_Activate(MenuEntry_t *entry);
static void Menu_RunInput_EntryRangeInt32_MovementVerify(MenuEntry_t *entry, MenuRangeInt32_t *object, int32_t newValue);
static void Menu_RunInput_EntryRangeInt32_MovementArbitrary(MenuEntry_t *entry, MenuRangeInt32_t *object, int32_t newValue);
static void Menu_RunInput_EntryRangeInt32_Movement(MenuEntry_t *entry, MenuRangeInt32_t *object, MenuMovement_t direction);
static void Menu_RunInput_EntryRangeFloat_MovementVerify(MenuEntry_t *entry, MenuRangeFloat_t *object, float newValue);
static void Menu_RunInput_EntryRangeFloat_MovementArbitrary(MenuEntry_t *entry, MenuRangeFloat_t *object, float newValue);
static void Menu_RunInput_EntryRangeFloat_Movement(MenuEntry_t *entry, MenuRangeFloat_t *object, MenuMovement_t direction);
#ifdef MENU_ENABLE_RANGEDOUBLE
static void Menu_RunInput_EntryRangeDouble_MovementVerify(/*MenuEntry_t *entry, */MenuRangeDouble_t *object, double newValue);
static void Menu_RunInput_EntryRangeDouble_MovementArbitrary(/*MenuEntry_t *entry, */MenuRangeDouble_t *object, double newValue);
static void Menu_RunInput_EntryRangeDouble_Movement(/*MenuEntry_t *entry, */MenuRangeDouble_t *object, MenuMovement_t direction);
#endif
static void Menu_RunInput_EntryString_Activate(MenuEntry_t *entry);
static void Menu_RunInput_EntryString_Submit(/*MenuEntry_t *entry, */MenuString_t *object);
static void Menu_RunInput_EntryString_Cancel(/*MenuEntry_t *entry, */MenuString_t *object);
static void Menu_RunInput_FileSelect_MovementVerify(MenuFileSelect_t *object);
static void Menu_RunInput_FileSelect_Movement(MenuFileSelect_t *object, MenuMovement_t direction);
static void Menu_RunInput_FileSelect_Select(MenuFileSelect_t *object);

static int32_t M_RunMenu_Menu(Menu_t *cm, MenuMenu_t *menu, MenuEntry_t *currentry, int32_t state, const vec2_t origin, bool actually_draw)
{
    int32_t totalHeight = 0;

    // RIP MenuGroup_t b. 2014-03-?? d. 2014-11-29
    {
        int32_t e;
        const int32_t y_upper = menu->format->pos.y;
        const int32_t y_lower = klabs(menu->format->bottomcutoff);
        int32_t y = 0;
        int32_t calculatedentryspacing = 0;

        if (menu->format->bottomcutoff < 0)
        {
            int32_t totalheight = 0, numvalidentries = 0;

            for (e = 0; e < menu->numEntries; ++e)
            {
                MenuEntry_t *entry = menu->entrylist[e];

                if (entry == NULL)
                    continue;

                ++numvalidentries;

                // assumes height == font->get_yline()!
                totalheight += entry->getHeight();
            }

            calculatedentryspacing = (klabs(menu->format->bottomcutoff) - menu->format->pos.y - totalheight) / (numvalidentries > 1 ? numvalidentries - 1 : 1);
        }

        // totalHeight calculating pass
        for (e = 0; e < menu->numEntries; ++e)
        {
            MenuEntry_t *entry = menu->entrylist[e];

            if (entry == NULL)
                continue;

            int32_t const height = entry->getHeight();

            y += height;
            totalHeight = y;
            y += (!calculatedentryspacing || calculatedentryspacing > entry->getMarginBottom()) ? entry->getMarginBottom() : calculatedentryspacing;
        }
        y = 0;

        int32_t ydim_upper, ydim_lower;
        if (y_upper + totalHeight > y_lower)
        {
            ydim_upper = ydim_from_200_16(origin.y + y_upper);
            ydim_lower = ydim_from_200_16(origin.y + y_lower);
        }
        else
        {
            ydim_upper = 0;
            ydim_lower = ydim-1;
        }

        for (e = 0; e < menu->numEntries; ++e)
        {
            MenuEntry_t *entry = menu->entrylist[e];

            if (entry == NULL)
                continue;

            int32_t x = menu->format->pos.x + entry->getIndent();

            uint8_t status = 0;
            if (e == menu->currentEntry)
                status |= MT_Selected;
            if (entry->flags & (MEF_Disabled|MEF_LookDisabled))
                status |= MT_Disabled;
            if (entry->format->width == 0)
                status |= MT_XCenter;

            bool const dodraw = entry->type != Spacer && actually_draw &&
                                0 <= y - menu->scrollPos + entry->font->get_yline() &&
                                y - menu->scrollPos <= klabs(menu->format->bottomcutoff) - menu->format->pos.y;

            int32_t const height = entry->getHeight(); // max(textsize.y, entry->font->get_yline()); // bluefont Q ruins this
            status |= MT_YCenter;
            int32_t const y_internal = origin.y + y_upper + y + (height>>1) - menu->scrollPos;

            vec2_t textsize;
            if (dodraw)
                textsize = Menu_Text(origin.x + x, y_internal, entry->font, entry->name, status, ydim_upper, ydim_lower);

            if (entry->format->width < 0)
                status |= MT_XRight;

            if (dodraw && (status & MT_Selected) && state != 1)
            {
                if (status & MT_XCenter)
                {
                    Menu_DrawCursorLeft(origin.x + (MENU_MARGIN_CENTER<<16) + entry->font->cursorCenterPosition, y_internal, entry->font->cursorScale);
                    Menu_DrawCursorRight(origin.x + (MENU_MARGIN_CENTER<<16) - entry->font->cursorCenterPosition, y_internal, entry->font->cursorScale);
                }
                else
                    Menu_DrawCursorLeft(origin.x + x - entry->font->cursorLeftPosition, y_internal, entry->font->cursorScale);
            }

            // need this up here to avoid race conditions
            entry->ybottom = (entry->ytop = y_upper + y) + height;

            if (dodraw)
            {
                const int32_t mousex = origin.x + entry->format->width == 0 ? x - textsize.x/2 : x;
                const int32_t mousey = origin.y + y_upper + y - menu->scrollPos;
                int32_t mousewidth = entry->format->width == 0 ? textsize.x : klabs(entry->format->width);

                if (entry->name)
                    x += klabs(entry->format->width);

                switch (entry->type)
                {
                    case Spacer:
                        break;
                    case Dummy:
                        if (MOUSEACTIVECONDITIONAL(state != 1 && cm == m_currentMenu && !Menu_MouseOutsideBounds(&m_mousepos, mousex, mousey, mousewidth, height)))
                        {
                            if (MOUSEWATCHPOINTCONDITIONAL(Menu_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, height)))
                            {
                                menu->currentEntry = e;
                                Menu_RunInput_Menu_MovementVerify(menu);
                            }
                        }
                        break;
                    case Link:
                        if (MOUSEACTIVECONDITIONAL(state != 1 && cm == m_currentMenu && !Menu_MouseOutsideBounds(&m_mousepos, mousex, mousey, mousewidth, height)))
                        {
                            if (MOUSEWATCHPOINTCONDITIONAL(Menu_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, height)))
                            {
                                menu->currentEntry = e;
                                Menu_RunInput_Menu_MovementVerify(menu);
                            }

                            if (!m_mousecaught && mousepressstate == Mouse_Released && !Menu_MouseOutsideBounds(&m_mousedownpos, mousex, mousey, mousewidth, entry->font->get_yline()))
                            {
                                menu->currentEntry = e;
                                Menu_RunInput_Menu_MovementVerify(menu);

                                if (entry->flags & MEF_Disabled)
                                    break;

                                Menu_RunInput_EntryLink_Activate(entry);

                                if (g_player[myconnectindex].ps->gm&MODE_MENU) // for skill selection
                                    S_PlaySound(PISTOL_BODYHIT);

                                m_mousecaught = 1;
                            }
                        }
                        break;
                    case Option:
                    {
                        MenuOption_t *object = (MenuOption_t*)entry->entry;
                        int32_t currentOption = Menu_FindOptionBinarySearch(object, object->data == NULL ? Menu_EntryOptionSource(entry, object->currentOption) : *object->data, 0, object->options->numOptions);

                        if (currentOption >= 0)
                            object->currentOption = currentOption;

                        int32_t optiontextx = origin.x + x;
                        const int32_t optiontexty = origin.y + y_upper + y - menu->scrollPos;

                        const vec2_t optiontextsize = Menu_Text(optiontextx, optiontexty + (height>>1), object->font,
                            currentOption < 0 ? MenuCustom : currentOption < object->options->numOptions ? object->options->optionNames[currentOption] : NULL,
                            status, ydim_upper, ydim_lower);

                        if (entry->format->width > 0)
                            mousewidth += optiontextsize.x;
                        else
                            optiontextx -= optiontextsize.x;

                        if (MOUSEACTIVECONDITIONAL(state != 1 && cm == m_currentMenu && !Menu_MouseOutsideBounds(&m_mousepos, mousex, mousey, mousewidth, height)))
                        {
                            if (MOUSEWATCHPOINTCONDITIONAL(Menu_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, height)))
                            {
                                menu->currentEntry = e;
                                Menu_RunInput_Menu_MovementVerify(menu);
                            }

                            if (!m_mousecaught && mousepressstate == Mouse_Released && !Menu_MouseOutsideBounds(&m_mousedownpos, mousex, mousey, mousewidth, entry->font->get_yline()))
                            {
                                menu->currentEntry = e;
                                Menu_RunInput_Menu_MovementVerify(menu);

                                if (entry->flags & MEF_Disabled)
                                    break;

                                Menu_RunInput_EntryOption_Activate(entry, object);

                                S_PlaySound(PISTOL_BODYHIT);

                                m_mousecaught = 1;
                            }
                        }

                        break;
                    }
                    case Custom2Col:
                    {
                        MenuCustom2Col_t *object = (MenuCustom2Col_t*)entry->entry;
                        int32_t columnx[2] = { origin.x + x - ((status & MT_XRight) ? object->columnWidth : 0), origin.x + x + ((status & MT_XRight) ? 0 : object->columnWidth) };
                        const int32_t columny = origin.y + y_upper + y - menu->scrollPos;

                        const vec2_t column0textsize = Menu_Text(columnx[0], columny + (height>>1), object->font, object->key[*object->column[0]], menu->currentColumn == 0 ? status : (status & ~MT_Selected), ydim_upper, ydim_lower);
                        const vec2_t column1textsize = Menu_Text(columnx[1], columny + (height>>1), object->font, object->key[*object->column[1]], menu->currentColumn == 1 ? status : (status & ~MT_Selected), ydim_upper, ydim_lower);

                        if (entry->format->width > 0)
                            mousewidth += object->columnWidth + column1textsize.x;
                        else
                        {
                            columnx[0] -= column0textsize.x;
                            columnx[1] -= column0textsize.x;
                        }

                        if (MOUSEACTIVECONDITIONAL(state != 1 && cm == m_currentMenu && !Menu_MouseOutsideBounds(&m_mousepos, mousex, mousey, mousewidth, height)))
                        {
                            if (MOUSEWATCHPOINTCONDITIONAL(Menu_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, height)))
                            {
                                menu->currentEntry = e;
                                Menu_RunInput_Menu_MovementVerify(menu);
                            }

                            if (!Menu_MouseOutsideBounds(&m_mousepos, columnx[1], mousey, column1textsize.x, object->font->get_yline()))
                            {
                                if (MOUSEWATCHPOINTCONDITIONAL(Menu_MouseOutsideBounds(&m_prevmousepos, columnx[1], mousey, column1textsize.x, object->font->get_yline())))
                                {
                                    menu->currentColumn = 1;
                                }

                                if (!m_mousecaught && mousepressstate == Mouse_Released && !Menu_MouseOutsideBounds(&m_mousedownpos, columnx[1], mousey, column1textsize.x, object->font->get_yline()))
                                {
                                    menu->currentEntry = e;
                                    Menu_RunInput_Menu_MovementVerify(menu);
                                    menu->currentColumn = 1;

                                    if (entry->flags & MEF_Disabled)
                                        break;

                                    Menu_RunInput_EntryCustom2Col_Activate(entry);

                                    S_PlaySound(PISTOL_BODYHIT);

                                    m_mousecaught = 1;
                                }
                            }
                            else if (!Menu_MouseOutsideBounds(&m_mousepos, columnx[0], mousey, column0textsize.x, object->font->get_yline()))
                            {
                                if (MOUSEWATCHPOINTCONDITIONAL(Menu_MouseOutsideBounds(&m_prevmousepos, columnx[0], mousey, column0textsize.x, object->font->get_yline())))
                                {
                                    menu->currentColumn = 0;
                                }

                                if (!m_mousecaught && mousepressstate == Mouse_Released && !Menu_MouseOutsideBounds(&m_mousedownpos, columnx[0], mousey, column0textsize.x, object->font->get_yline()))
                                {
                                    menu->currentEntry = e;
                                    Menu_RunInput_Menu_MovementVerify(menu);
                                    menu->currentColumn = 0;

                                    if (entry->flags & MEF_Disabled)
                                        break;

                                    Menu_RunInput_EntryCustom2Col_Activate(entry);

                                    S_PlaySound(PISTOL_BODYHIT);

                                    m_mousecaught = 1;
                                }
                            }
                        }
                        break;
                    }
                    case RangeInt32:
                    {
                        MenuRangeInt32_t *object = (MenuRangeInt32_t*)entry->entry;

                        int32_t s, p;
                        int32_t z = entry->font->cursorScale;
                        Menu_GetFmt(object->font, status, &s, &p, &z);

                        const int32_t slidebarwidth = mulscale16(tilesiz[SLIDEBAR].x<<16, z);
                        const int32_t slidebarheight = mulscale16(tilesiz[SLIDEBAR].y<<16, z);

                        if (status & MT_XRight)
                            x -= slidebarwidth;
                        else
                            mousewidth += slidebarwidth;

                        const int32_t slidebarx = origin.x + x;
                        const int32_t slidebary = origin.y + y_upper + y + ((height - slidebarheight)>>1) - menu->scrollPos;

                        rotatesprite_ybounds(slidebarx, slidebary, z, 0, SLIDEBAR, s, (entry->flags & (MEF_Disabled|MEF_LookDisabled)) ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16, ydim_upper, ydim_lower);

                        const int32_t slideregionwidth = mulscale16((tilesiz[SLIDEBAR].x-2-tilesiz[SLIDEBAR+1].x)<<16, z);
                        const int32_t slidepointx = slidebarx + (1<<16) + scale(slideregionwidth, *object->variable - object->min, object->max - object->min);
                        const int32_t slidepointy = slidebary + mulscale16((tilesiz[SLIDEBAR].y-tilesiz[SLIDEBAR+1].y)<<15, z);

                        rotatesprite_ybounds(slidepointx, slidepointy, z, 0, SLIDEBAR+1, s, (entry->flags & (MEF_Disabled|MEF_LookDisabled)) ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16, ydim_upper, ydim_lower);

                        if (object->flags & DisplayTypeMask)
                        {
                            status |= MT_XRight;

                            int32_t onehundredpercent = object->onehundredpercent;
                            if (onehundredpercent == 0)
                                onehundredpercent = object->max;

                            switch (object->flags & DisplayTypeMask)
                            {
                                case DisplayTypeInteger:
                                    Bsprintf(tempbuf, "%d", *object->variable);
                                    break;
                                case DisplayTypePercent:
                                    Bsprintf(tempbuf, "%d%%", roundscale(*object->variable, 100, onehundredpercent));
                                    break;
                                case DisplayTypeNormalizedDecimal:
                                    Bsprintf(tempbuf, "%.2f", (double) *object->variable / (double) onehundredpercent);
                                    break;
                            }

                            Menu_Text(origin.x + x - (4<<16), origin.y + y_upper + y + (height>>1) - menu->scrollPos, object->font, tempbuf, status, ydim_upper, ydim_lower);
                        }

                        if (MOUSEACTIVECONDITIONAL(state != 1 && cm == m_currentMenu && !Menu_MouseOutsideBounds(&m_mousepos, mousex, mousey, mousewidth, height)))
                        {
                            if (MOUSEWATCHPOINTCONDITIONAL(Menu_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, height)))
                            {
                                menu->currentEntry = e;
                                Menu_RunInput_Menu_MovementVerify(menu);
                            }

                            if (!m_mousecaught && (mousepressstate == Mouse_Pressed || mousepressstate == Mouse_Held))
                            {
                                const int32_t slidepointhalfwidth = mulscale16((2+tilesiz[SLIDEBAR+1].x)<<15, z);
                                const int32_t slideregionx = slidebarx + slidepointhalfwidth;

                                menu->currentEntry = e;
                                Menu_RunInput_Menu_MovementVerify(menu);

                                if (entry->flags & MEF_Disabled)
                                    break;

                                // region between the x-midline of the slidepoint at the extremes slides proportionally
                                if (!Menu_MouseOutsideBounds(&m_mousepos, slideregionx, mousey, slideregionwidth, height))
                                {
                                    Menu_RunInput_EntryRangeInt32_MovementArbitrary(entry, object, roundscale(object->max - object->min, m_mousepos.x - slideregionx, slideregionwidth) + object->min);

                                    m_mousecaught = 1;
                                }
                                // region outside the x-midlines clamps to the extremes
                                else if (!Menu_MouseOutsideBounds(&m_mousepos, slidebarx, mousey, slidebarwidth, height))
                                {
                                    if (m_mousepos.x > slideregionx + slideregionwidth/2)
                                        Menu_RunInput_EntryRangeInt32_MovementVerify(entry, object, object->max);
                                    else
                                        Menu_RunInput_EntryRangeInt32_MovementVerify(entry, object, object->min);

                                    m_mousecaught = 1;
                                }
                            }
                        }

                        break;
                    }
                    case RangeFloat:
                    {
                        MenuRangeFloat_t *object = (MenuRangeFloat_t*)entry->entry;

                        int32_t s, p;
                        int32_t z = entry->font->cursorScale;
                        Menu_GetFmt(object->font, status, &s, &p, &z);

                        const int32_t slidebarwidth = mulscale16(tilesiz[SLIDEBAR].x<<16, z);
                        const int32_t slidebarheight = mulscale16(tilesiz[SLIDEBAR].y<<16, z);

                        if (status & MT_XRight)
                            x -= slidebarwidth;
                        else
                            mousewidth += slidebarwidth;

                        const int32_t slidebarx = origin.x + x;
                        const int32_t slidebary = origin.y + y_upper + y + ((height - slidebarheight)>>1) - menu->scrollPos;

                        rotatesprite_ybounds(slidebarx, slidebary, z, 0, SLIDEBAR, s, (entry->flags & (MEF_Disabled|MEF_LookDisabled)) ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16, ydim_upper, ydim_lower);

                        const int32_t slideregionwidth = mulscale16((tilesiz[SLIDEBAR].x-2-tilesiz[SLIDEBAR+1].x)<<16, z);
                        const int32_t slidepointx = slidebarx + (1<<16) + Blrintf((float) slideregionwidth * (*object->variable - object->min) / (object->max - object->min));
                        const int32_t slidepointy = slidebary + mulscale16((tilesiz[SLIDEBAR].y-tilesiz[SLIDEBAR+1].y)<<15, z);

                        rotatesprite_ybounds(slidepointx, slidepointy, z, 0, SLIDEBAR+1, s, (entry->flags & (MEF_Disabled|MEF_LookDisabled)) ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16, ydim_upper, ydim_lower);

                        if (object->flags & DisplayTypeMask)
                        {
                            status |= MT_XRight;

                            float onehundredpercent = object->onehundredpercent;
                            if (onehundredpercent == 0.f)
                                onehundredpercent = 1.f;

                            switch (object->flags & DisplayTypeMask)
                            {
                                case DisplayTypeInteger:
                                    Bsprintf(tempbuf, "%.2f", *object->variable);
                                    break;
                                case DisplayTypePercent:
                                    Bsprintf(tempbuf, "%ld%%", lrintf(*object->variable * 100.f / onehundredpercent));
                                    break;
                                case DisplayTypeNormalizedDecimal:
                                    Bsprintf(tempbuf, "%.2f", *object->variable / onehundredpercent);
                                    break;
                            }

                            Menu_Text(origin.x + x - (4<<16), origin.y + y_upper + y + (height>>1) - menu->scrollPos, object->font, tempbuf, status, ydim_upper, ydim_lower);
                        }

                        if (MOUSEACTIVECONDITIONAL(state != 1 && cm == m_currentMenu && !Menu_MouseOutsideBounds(&m_mousepos, mousex, mousey, mousewidth, height)))
                        {
                            if (MOUSEWATCHPOINTCONDITIONAL(Menu_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, height)))
                            {
                                menu->currentEntry = e;
                                Menu_RunInput_Menu_MovementVerify(menu);
                            }

                            if (!m_mousecaught && (mousepressstate == Mouse_Pressed || mousepressstate == Mouse_Held))
                            {
                                const int32_t slidepointhalfwidth = mulscale16((2+tilesiz[SLIDEBAR+1].x)<<15, z);
                                const int32_t slideregionx = slidebarx + slidepointhalfwidth;

                                menu->currentEntry = e;
                                Menu_RunInput_Menu_MovementVerify(menu);

                                if (entry->flags & MEF_Disabled)
                                    break;

                                // region between the x-midline of the slidepoint at the extremes slides proportionally
                                if (!Menu_MouseOutsideBounds(&m_mousepos, slideregionx, mousey, slideregionwidth, height))
                                {
                                    Menu_RunInput_EntryRangeFloat_MovementArbitrary(entry, object, (object->max - object->min) * (m_mousepos.x - slideregionx) / slideregionwidth + object->min);

                                    m_mousecaught = 1;
                                }
                                // region outside the x-midlines clamps to the extremes
                                else if (!Menu_MouseOutsideBounds(&m_mousepos, slidebarx, mousey, slidebarwidth, height))
                                {
                                    if (m_mousepos.x > slideregionx + slideregionwidth/2)
                                        Menu_RunInput_EntryRangeFloat_MovementVerify(entry, object, object->max);
                                    else
                                        Menu_RunInput_EntryRangeFloat_MovementVerify(entry, object, object->min);

                                    m_mousecaught = 1;
                                }
                            }
                        }

                        break;
                    }
#ifdef MENU_ENABLE_RANGEDOUBLE
                    case RangeDouble:
                    {
                        MenuRangeDouble_t *object = (MenuRangeDouble_t*)entry->entry;

                        int32_t s, p;
                        int32_t z = entry->font->cursorScale;
                        Menu_GetFmt(object->font, status, &s, &p, &z);

                        const int32_t slidebarwidth = mulscale16(tilesiz[SLIDEBAR].x<<16, z);
                        const int32_t slidebarheight = mulscale16(tilesiz[SLIDEBAR].y<<16, z);

                        if (status & MT_XRight)
                            x -= slidebarwidth;
                        else
                            mousewidth += slidebarwidth;

                        const int32_t slidebarx = origin.x + x;
                        const int32_t slidebary = origin.y + y_upper + y + ((height - slidebarheight)>>1) - menu->scrollPos;

                        rotatesprite_ybounds(slidebarx, slidebary, z, 0, SLIDEBAR, s, (entry->flags & (MEF_Disabled|MEF_LookDisabled)) ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16, ydim_upper, ydim_lower);

                        const int32_t slideregionwidth = mulscale16((tilesiz[SLIDEBAR].x-2-tilesiz[SLIDEBAR+1].x)<<16, z);
                        const int32_t slidepointx = slidebarx + (1<<16) + lrint((double) slideregionwidth * (*object->variable - object->min) / (object->max - object->min));
                        const int32_t slidepointy = slidebary + mulscale16((tilesiz[SLIDEBAR].y-tilesiz[SLIDEBAR+1].y)<<15, z);

                        rotatesprite_ybounds(slidepointx, slidepointy, z, 0, SLIDEBAR+1, s, (entry->flags & (MEF_Disabled|MEF_LookDisabled)) ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16, ydim_upper, ydim_lower);

                        if (object->flags & DisplayTypeMask)
                        {
                            status |= MT_XRight;

                            double onehundredpercent = object->onehundredpercent;
                            if (onehundredpercent == 0.)
                                onehundredpercent = 1.;

                            switch (object->flags & DisplayTypeMask)
                            {
                                case DisplayTypeInteger:
                                    Bsprintf(tempbuf, "%.2f", *object->variable);
                                    break;
                                case DisplayTypePercent:
                                    Bsprintf(tempbuf, "%ld%%", lrint(*object->variable * 100. / onehundredpercent));
                                    break;
                                case DisplayTypeNormalizedDecimal:
                                    Bsprintf(tempbuf, "%.2f", *object->variable / onehundredpercent);
                                    break;
                            }

                            Menu_Text(origin.x + x - (4<<16), origin.y + y_upper + y + (height>>1) - menu->scrollPos, object->font, tempbuf, status, ydim_upper, ydim_lower);
                        }

                        if (MOUSEACTIVECONDITIONAL(state != 1 && cm == m_currentMenu && !Menu_MouseOutsideBounds(&m_mousepos, mousex, mousey, mousewidth, height)))
                        {
                            if (MOUSEWATCHPOINTCONDITIONAL(Menu_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, height)))
                            {
                                menu->currentEntry = e;
                                Menu_RunInput_Menu_MovementVerify(menu);
                            }

                            if (!m_mousecaught && (mousepressstate == Mouse_Pressed || mousepressstate == Mouse_Held))
                            {
                                const int32_t slidepointhalfwidth = mulscale16((2+tilesiz[SLIDEBAR+1].x)<<15, z);
                                const int32_t slideregionx = slidebarx + slidepointhalfwidth;

                                menu->currentEntry = e;
                                Menu_RunInput_Menu_MovementVerify(menu);

                                if (entry->flags & MEF_Disabled)
                                    break;

                                // region between the x-midline of the slidepoint at the extremes slides proportionally
                                if (!Menu_MouseOutsideBounds(&m_mousepos, slideregionx, mousey, slideregionwidth, height))
                                {
                                    Menu_RunInput_EntryRangeDouble_MovementArbitrary(/*entry, */object, (object->max - object->min) * (m_mousepos.x - slideregionx) / slideregionwidth + object->min);

                                    m_mousecaught = 1;
                                }
                                // region outside the x-midlines clamps to the extremes
                                else if (!Menu_MouseOutsideBounds(&m_mousepos, slidebarx, mousey, slidebarwidth, height))
                                {
                                    if (m_mousepos.x > slideregionx + slideregionwidth/2)
                                        Menu_RunInput_EntryRangeDouble_MovementVerify(/*entry, */object, object->max);
                                    else
                                        Menu_RunInput_EntryRangeDouble_MovementVerify(/*entry, */object, object->min);

                                    m_mousecaught = 1;
                                }
                            }
                        }

                        break;
                    }
#endif
                    case String:
                    {
                        MenuString_t *object = (MenuString_t*)entry->entry;

                        vec2_t dim;
                        int32_t stringx = x;
                        const int32_t stringy = origin.y + y_upper + y + (height>>1) - menu->scrollPos;
                        int32_t h;

                        if (entry == currentry && object->editfield != NULL)
                        {
                            dim = Menu_Text(origin.x + stringx, stringy, object->font, object->editfield, (status & ~MT_Disabled) | MT_Literal, ydim_upper, ydim_lower);
                            h = max(dim.y, entry->font->get_yline());

                            Menu_DrawCursorText(origin.x + x + dim.x + (1<<16), stringy, h, ydim_upper, ydim_lower);
                        }
                        else
                        {
                            dim = Menu_Text(origin.x + stringx, stringy, object->font, object->variable, status, ydim_upper, ydim_lower);
                            h = max(dim.y, entry->font->get_yline());
                        }

                        if (entry->format->width > 0)
                        {
                            if (entry->name)
                                mousewidth += dim.x;
                        }
                        else
                            stringx -= dim.x;

                        if (MOUSEACTIVECONDITIONAL(cm == m_currentMenu && !Menu_MouseOutsideBounds(&m_mousepos, mousex, mousey, mousewidth, h)))
                        {
                            if (state != 1 && Menu_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, h))
                            {
                                menu->currentEntry = e;
                                Menu_RunInput_Menu_MovementVerify(menu);
                            }

    #ifndef EDUKE32_TOUCH_DEVICES
                            if (!m_mousecaught && mousepressstate == Mouse_Released && !Menu_MouseOutsideBounds(&m_mousepos, mousex, mousey, mousewidth, h) && !Menu_MouseOutsideBounds(&m_mousedownpos, mousex, mousey, mousewidth, h))
    #endif
                            {
                                if (entry == currentry && object->editfield != NULL)
                                {
                                    Menu_RunInput_EntryString_Submit(/*entry, */object);

                                    S_PlaySound(PISTOL_BODYHIT);

                                    m_mousecaught = 1;
                                }
                                else if (state != 1)
                                {
                                    menu->currentEntry = e;
                                    Menu_RunInput_Menu_MovementVerify(menu);

                                    if (entry->flags & MEF_Disabled)
                                        break;

                                    Menu_RunInput_EntryString_Activate(entry);

                                    S_PlaySound(PISTOL_BODYHIT);

                                    m_mousecaught = 1;
                                }
                            }
                        }

                        break;
                    }
                }
            }

            // prepare for the next line
            y += height;
            y += (!calculatedentryspacing || calculatedentryspacing > entry->getMarginBottom()) ? entry->getMarginBottom() : calculatedentryspacing;
        }

        // draw indicators if applicable
        if (actually_draw)
            Menu_RunScrollbar(cm, menu->format, y_upper + totalHeight, &menu->scrollPos, 320<<16, origin);
    }

    return totalHeight;
}

static void Menu_RunOptionList(Menu_t *cm, MenuEntry_t *entry, MenuOption_t *object, const vec2_t origin)
{
    int32_t e, y = 0;
    const int32_t y_upper = object->options->menuFormat->pos.y;
    const int32_t y_lower = object->options->menuFormat->bottomcutoff;
    int32_t calculatedentryspacing = object->options->getMarginBottom();

    // assumes height == font->get_yline()!
    if (calculatedentryspacing < 0)
        calculatedentryspacing = (-calculatedentryspacing - object->options->font->get_yline()) / (object->options->numOptions - 1) - object->options->font->get_yline();

    int32_t totalHeight = 0;
    for (e = 0; e < object->options->numOptions; ++e)
    {
        int32_t const height = object->options->font->get_yline();

        y += height;
        totalHeight = y;
        y += calculatedentryspacing;
    }
    y = 0;

    int32_t ydim_upper, ydim_lower;
    if (y_upper + totalHeight > y_lower)
    {
        ydim_upper = ydim_from_200_16(origin.y + y_upper);
        ydim_lower = ydim_from_200_16(origin.y + y_lower);
    }
    else
    {
        ydim_upper = 0;
        ydim_lower = ydim-1;
    }

    for (e = 0; e < object->options->numOptions; ++e)
    {
        int32_t const x = object->options->menuFormat->pos.x;

        uint8_t status = 0;
        if (e == object->options->currentEntry)
            status |= MT_Selected;
        if (object->options->entryFormat->width == 0)
            status |= MT_XCenter;

        bool const dodraw = 0 <= y - object->options->scrollPos + object->options->font->get_yline() &&
                            y - object->options->scrollPos <= object->options->menuFormat->bottomcutoff - object->options->menuFormat->pos.y;

        int32_t const height = object->options->font->get_yline(); // max(textsize.y, object->options->font->get_yline());
        status |= MT_YCenter;
        int32_t const y_internal = origin.y + y_upper + y + (height>>1) - object->options->scrollPos;

        vec2_t textsize;
        if (dodraw)
            textsize = Menu_Text(origin.x + x, y_internal, object->options->font, object->options->optionNames[e], status, ydim_upper, ydim_lower);

        if (object->options->entryFormat->width < 0)
            status |= MT_XRight;

        if (dodraw && (status & MT_Selected))
        {
            if (status & MT_XCenter)
            {
                Menu_DrawCursorLeft(origin.x + (MENU_MARGIN_CENTER<<16) + object->options->font->cursorCenterPosition, y_internal, object->options->font->cursorScale);
                Menu_DrawCursorRight(origin.x + (MENU_MARGIN_CENTER<<16) - object->options->font->cursorCenterPosition, y_internal, object->options->font->cursorScale);
            }
            else
                Menu_DrawCursorLeft(origin.x + x - object->options->font->cursorLeftPosition, y_internal, object->options->font->cursorScale);
        }

        if (dodraw)
        {
            const int32_t mousex = origin.x + object->options->entryFormat->width == 0 ? x - textsize.x/2 : x;
            const int32_t mousey = origin.y + y_upper + y - object->options->scrollPos;
            const int32_t mousewidth = object->options->entryFormat->width == 0 ? textsize.x : klabs(object->options->entryFormat->width);

            if (MOUSEACTIVECONDITIONAL(cm == m_currentMenu && !Menu_MouseOutsideBounds(&m_mousepos, mousex, mousey, mousewidth, object->options->font->get_yline())))
            {
                if (MOUSEWATCHPOINTCONDITIONAL(Menu_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, object->options->font->get_yline())))
                {
                    object->options->currentEntry = e;
                }

                if (!m_mousecaught && mousepressstate == Mouse_Released && !Menu_MouseOutsideBounds(&m_mousedownpos, mousex, mousey, mousewidth, object->options->font->get_yline()))
                {
                    object->options->currentEntry = e;

                    if (!Menu_RunInput_EntryOptionList_Activate(entry, object))
                        S_PlaySound(PISTOL_BODYHIT);

                    m_mousecaught = 1;
                }
            }
        }

        // prepare for the next line
        y += height;
        y += calculatedentryspacing;
    }

    // draw indicators if applicable
    Menu_RunScrollbar(cm, object->options->menuFormat, y_upper + totalHeight, &object->options->scrollPos, 320<<16, origin);
}

static int32_t Menu_RunInput_MouseAdvance(void)
{
    return MOUSEACTIVECONDITIONAL(!m_mousecaught && mousepressstate == Mouse_Released);
}

static int32_t Menu_RunInput_MouseReturn_status;

#if !defined EDUKE32_TOUCH_DEVICES
static void Menu_Run_MouseReturn(Menu_t *cm, const vec2_t origin)
{
    if (!MOUSEACTIVECONDITION)
        return;

    if (cm->menuID == MENU_MAIN)
        return;

    rotatesprite_(origin.x + (tilesiz[SELECTDIR].y << 16), 0, 65536, 512, SELECTDIR,
                  Menu_RunInput_MouseReturn_status ? 4 - (sintable[(totalclock << 4) & 2047] >> 11) : 6, 0,
                  2 | 8 | 16 | RS_ALIGN_L, MOUSEALPHA, 0, xdim_from_320_16(origin.x + x_widescreen_left()), 0,
                  xdim_from_320_16(origin.x + x_widescreen_left() + (tilesiz[SELECTDIR].y << 15)), ydim - 1);
}
#endif

static int32_t Menu_RunInput_MouseReturn(void)
{
#if !defined EDUKE32_TOUCH_DEVICES
    if (!MOUSEACTIVECONDITION)
    {
        Menu_RunInput_MouseReturn_status = 0;
        return 0;
    }
#endif

    if (g_currentMenu == MENU_MAIN)
        return 0;

    const int32_t MouseReturnRegionX = x_widescreen_left();

    if (!Menu_MouseOutsideBounds(&m_mousepos, MouseReturnRegionX, 0, tilesiz[SELECTDIR].y<<15, tilesiz[SELECTDIR].x<<16))
    {
#if !defined EDUKE32_TOUCH_DEVICES
        Menu_RunInput_MouseReturn_status = 1;
#else
        Menu_RunInput_MouseReturn_status = (mousepressstate == Mouse_Pressed || mousepressstate == Mouse_Held);
#endif

        return !m_mousecaught && mousepressstate == Mouse_Released && !Menu_MouseOutsideBounds(&m_mousedownpos, MouseReturnRegionX, 0, tilesiz[SELECTDIR].y<<15, tilesiz[SELECTDIR].x<<16);
    }

    Menu_RunInput_MouseReturn_status = 0;

    return 0;
}

static void Menu_Run_AbbreviateNameIntoBuffer(const char* name, int32_t entrylength)
{
    int32_t len = Bstrlen(name);
    Bstrncpy(tempbuf, name, len);
    if (len > entrylength)
    {
        len = entrylength-3;
        tempbuf[len] = 0;
        while (len < entrylength)
            tempbuf[len++] = '.';
    }
    tempbuf[len] = 0;
}

static void Menu_Recurse(MenuID_t cm, const vec2_t origin)
{
    switch (cm)
    {
    case MENU_LOADVERIFY:
    case MENU_SAVEVERIFY:
    case MENU_ADULTPASSWORD:
    case MENU_CHEATENTRY:
    case MENU_CHEAT_WARP:
    case MENU_CHEAT_SKILL:
        Menu_Run(m_previousMenu, origin);
        break;
    default:
        break;
    }
}

static void Menu_Run(Menu_t *cm, const vec2_t origin)
{
    Menu_Recurse(cm->menuID, origin);

    switch (cm->type)
    {
        case Verify:
        {
            MenuVerify_t *object = (MenuVerify_t*)cm->object;

            Menu_Pre(cm->menuID);

            Menu_PreDrawBackground(cm->menuID, origin);

            Menu_PreDraw(cm->menuID, NULL, origin);

            Menu_DrawCursorLeft(origin.x + object->cursorpos.x, origin.y + object->cursorpos.y, 65536);

            break;
        }

        case Message:
        {
            MenuMessage_t *object = (MenuMessage_t*)cm->object;

            Menu_Pre(cm->menuID);

            Menu_PreDrawBackground(cm->menuID, origin);

            Menu_PreDraw(cm->menuID, NULL, origin);

            Menu_DrawCursorLeft(origin.x + object->cursorpos.x, origin.y + object->cursorpos.y, 65536);

            break;
        }

        case TextForm:
        {
            MenuTextForm_t *object = (MenuTextForm_t*)cm->object;

            Menu_Pre(cm->menuID);

            Menu_PreDrawBackground(cm->menuID, origin);

            Menu_BlackRectangle(origin.x + (60<<16), origin.y + (86<<16), 200<<16, 28<<16, 0);

            mgametextcenter(origin.x, origin.y + (98<<16), object->instructions, TEXT_YBOTTOM);

            const char *displaytext = object->input;

            if (object->flags & MTF_Password)
            {
                size_t x;
                for (x = 0; x < Bstrlen(object->input); ++x)
                    tempbuf[x] = '*';
                tempbuf[x] = 0;

                displaytext = tempbuf;
            }

            const vec2_t textreturn = mgametextcenter(origin.x, origin.y + (102<<16), displaytext);

            Menu_PreDraw(cm->menuID, NULL, origin);

            int32_t const h = MF_Bluefont.get_yline();

            Menu_DrawCursorText(origin.x + (MENU_MARGIN_CENTER<<16) + (textreturn.x>>1) + (1<<16), origin.y + (102<<16) + (h>>1), h);

            break;
        }

        case FileSelect:
        {
            MenuFileSelect_t *object = (MenuFileSelect_t*)cm->object;
            const int32_t MenuFileSelect_scrollbar_rightedge[2] = { 160<<16, 284<<16 };
            int32_t i, selected = 0;

            Menu_Pre(cm->menuID);

            Menu_PreDrawBackground(cm->menuID, origin);

            if (object->title != NoTitle)
                Menu_DrawTopBar(origin);


            // black translucent background underneath file lists
            Menu_BlackRectangle(origin.x + (36<<16), origin.y + (42<<16), 248<<16, 123<<16, 1|32);

            // path
            Bsnprintf(tempbuf, sizeof(tempbuf), "Path: %s", object->destination);
            Menu_Text(origin.x + object->format[0]->pos.x, origin.y + (32<<16) + (MF_Bluefont.get_yline()>>1), &MF_Bluefont, tempbuf, MT_YCenter, 0, ydim-1);

            for (i = 0; i < 2; ++i)
            {
                if (object->findhigh[i])
                {
                    CACHE1D_FIND_REC *dir;
                    int32_t y = 0;
                    const int32_t y_upper = object->format[i]->pos.y;
                    const int32_t y_lower = klabs(object->format[i]->bottomcutoff);

                    int32_t totalHeight = 0;
                    for (dir = object->findhigh[i]->usera; dir; dir = dir->next)
                    {
                        y += object->font[i]->get_yline();
                        totalHeight = y;
                        y += object->getMarginBottom(i);
                    }
                    y = 0;

                    int32_t ydim_upper, ydim_lower;
                    if (y_upper + totalHeight > y_lower)
                    {
                        ydim_upper = ydim_from_200_16(origin.y + y_upper);
                        ydim_lower = ydim_from_200_16(origin.y + y_lower);
                    }
                    else
                    {
                        ydim_upper = 0;
                        ydim_lower = ydim-1;
                    }

                    for (dir = object->findhigh[i]->usera; dir; dir = dir->next)
                    {
                        uint8_t status = (dir == object->findhigh[i] && object->currentList == i) ? MT_Selected : 0;

                        // pal = dir->source==CACHE1D_SOURCE_ZIP ? 8 : 2

                        Menu_Run_AbbreviateNameIntoBuffer(dir->name, USERMAPENTRYLENGTH);

                        const int32_t thisx = object->format[i]->pos.x;
                        const int32_t thisy = y - object->scrollPos[i];

                        int32_t const height = object->font[i]->get_yline();

                        if (0 <= thisy + height && thisy <= klabs(object->format[i]->bottomcutoff) - object->format[i]->pos.y)
                        {
                            status |= MT_YCenter;

                            const int32_t mousex = origin.x + thisx;
                            const int32_t mousey = origin.y + y_upper + thisy + (height>>1);

                            vec2_t textdim = Menu_Text(mousex, mousey, object->font[i], tempbuf, status, ydim_upper, ydim_lower);

                            if (MOUSEACTIVECONDITIONAL(cm == m_currentMenu && !Menu_MouseOutsideBounds(&m_mousepos, mousex, mousey, textdim.x, object->font[i]->get_yline())))
                            {
                                if (MOUSEWATCHPOINTCONDITIONAL(Menu_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, textdim.x, object->font[i]->get_yline())))
                                {
                                    object->findhigh[i] = dir;
                                    object->currentList = i;

                                    Menu_RunInput_FileSelect_MovementVerify(object);
                                }

                                if (!m_mousecaught && mousepressstate == Mouse_Released && !Menu_MouseOutsideBounds(&m_mousedownpos, mousex, mousey, textdim.x, object->font[i]->get_yline()))
                                {
                                    object->findhigh[i] = dir;
                                    object->currentList = i;

                                    Menu_RunInput_FileSelect_MovementVerify(object);

                                    m_mousecaught = 1;
                                    selected = 1;
                                }
                            }
                        }

                        y += object->font[i]->get_yline() + object->getMarginBottom(i);
                    }

                    Menu_RunScrollbar(cm, object->format[i], y_upper + totalHeight, &object->scrollPos[i], MenuFileSelect_scrollbar_rightedge[i], origin);
                }
            }

            Menu_PreDraw(cm->menuID, NULL, origin);

            if (object->title != NoTitle)
                Menu_DrawTopBarCaption(object->title, origin);

            if (selected)
            {
                Menu_RunInput_FileSelect_Select(object);

                S_PlaySound(PISTOL_BODYHIT);

                m_mousecaught = 1;
            }

            break;
        }

        case Panel:
        {
            MenuPanel_t *object = (MenuPanel_t*)cm->object;

            Menu_Pre(cm->menuID);

            Menu_PreDrawBackground(cm->menuID, origin);

            if (object->title != NoTitle)
                Menu_DrawTopBar(origin);

            Menu_PreDraw(cm->menuID, NULL, origin);

            if (object->title != NoTitle)
                Menu_DrawTopBarCaption(object->title, origin);

            break;
        }

        case Menu:
        {
            int32_t state;

            MenuMenu_t *menu = (MenuMenu_t*)cm->object;
            MenuEntry_t *currentry = menu->entrylist[menu->currentEntry];

            state = Menu_DetermineSpecialState(currentry);

            if (state != 2)
            {
                Menu_Pre(cm->menuID);

                Menu_PreDrawBackground(cm->menuID, origin);

                if (menu->title != NoTitle)
                    Menu_DrawTopBar(origin);

                Menu_PreDraw(cm->menuID, currentry, origin);

                M_RunMenu_Menu(cm, menu, currentry, state, origin);
            }
            else
            {
                Menu_PreDrawBackground(cm->menuID, origin);

                if (menu->title != NoTitle)
                    Menu_DrawTopBar(origin);

                if (currentry->type == Option)
                {
                    if (currentry->name)
                        Menu_DrawTopBarCaption(currentry->name, origin);

                    Menu_PreOptionListDraw(currentry, origin);

                    Menu_RunOptionList(cm, currentry, (MenuOption_t*)currentry->entry, origin);
                }
                else if (currentry->type == Custom2Col)
                {
                    Menu_PreCustom2ColScreenDraw(currentry, origin);
                }
            }

            if ((currentry->type != Option || state != 2) && menu->title != NoTitle)
                Menu_DrawTopBarCaption(menu->title, origin);

            break;
        }
    }

#if !defined EDUKE32_TOUCH_DEVICES
    Menu_Run_MouseReturn(cm, origin);
#endif
}

/*
Note: When menus are exposed to scripting, care will need to be taken so that
a user cannot define an empty MenuEntryList, or one containing only spacers,
or else this function will recurse infinitely.
*/
static MenuEntry_t *Menu_RunInput_Menu_MovementVerify(MenuMenu_t *menu)
{
    return Menu_AdjustForCurrentEntryAssignment(menu);
}

static MenuEntry_t *Menu_RunInput_Menu_Movement(MenuMenu_t *menu, MenuMovement_t direction)
{
    if (menu->numEntries == 1)
        return menu->entrylist[menu->currentEntry];

    switch (direction)
    {
        case MM_End:
            menu->currentEntry = menu->numEntries;
            fallthrough__;
        case MM_Up:
                do
                {
                    --menu->currentEntry;
                    if (menu->currentEntry < 0)
                        return Menu_RunInput_Menu_Movement(menu, MM_End);
                }
                while (!menu->entrylist[menu->currentEntry] || menu->entrylist[menu->currentEntry]->type == Spacer);
            break;

        case MM_Home:
            menu->currentEntry = -1;
            fallthrough__;
        case MM_Down:
                do
                {
                    ++menu->currentEntry;
                    if (menu->currentEntry >= menu->numEntries)
                        return Menu_RunInput_Menu_Movement(menu, MM_Home);
                }
                while (!menu->entrylist[menu->currentEntry] || menu->entrylist[menu->currentEntry]->type == Spacer);
            break;

        case MM_Swap:
            menu->currentColumn = !menu->currentColumn;
            break;

        default:
            break;
    }

    return Menu_RunInput_Menu_MovementVerify(menu);
}

static void Menu_RunInput_EntryLink_Activate(MenuEntry_t *entry)
{
    MenuLink_t *link = (MenuLink_t*)entry->entry;

    Menu_EntryLinkActivate(entry);

    Menu_AnimateChange(link->linkID, link->animation);
}

static void Menu_RunInput_EntryOptionList_MovementVerify(MenuOption_t *object)
{
    const int32_t listytop = object->options->menuFormat->pos.y;
    // assumes height == font->get_yline()!
    const int32_t unitheight = object->options->getMarginBottom() < 0 ? (-object->options->getMarginBottom() - object->options->font->get_yline()) / object->options->numOptions : (object->options->font->get_yline() + object->options->getMarginBottom());
    const int32_t ytop = listytop + object->options->currentEntry * unitheight;
    const int32_t ybottom = ytop + object->options->font->get_yline();

    if (ybottom - object->options->scrollPos > object->options->menuFormat->bottomcutoff)
        object->options->scrollPos = ybottom - object->options->menuFormat->bottomcutoff;
    else if (ytop - object->options->scrollPos < listytop)
        object->options->scrollPos = ytop - listytop;
}

static void Menu_RunInput_EntryOptionList_Movement(MenuOption_t *object, MenuMovement_t direction)
{
    switch (direction)
    {
        case MM_Up:
            --object->options->currentEntry;
            if (object->options->currentEntry >= 0)
                break;
            fallthrough__;
        case MM_End:
            object->options->currentEntry = object->options->numOptions-1;
            break;

        case MM_Down:
            ++object->options->currentEntry;
            if (object->options->currentEntry < object->options->numOptions)
                break;
            fallthrough__;
        case MM_Home:
            object->options->currentEntry = 0;
            break;

        default:
            break;
    }

    Menu_RunInput_EntryOptionList_MovementVerify(object);
}

static int32_t Menu_RunInput_EntryOption_Modify(MenuEntry_t *entry, MenuOption_t *object, int32_t newValueIndex)
{
    int32_t newValue = (object->options->optionValues == NULL) ? newValueIndex : object->options->optionValues[newValueIndex];
    if (!Menu_EntryOptionModify(entry, newValue))
    {
        object->currentOption = newValueIndex;

        if ((int32_t*)object->data != NULL) // NULL implies the functions will handle it
            *((int32_t*)object->data) = newValue;

        Menu_EntryOptionDidModify(entry);

        return 0;
    }

    return -1;
}

static int32_t Menu_RunInput_EntryOption_Movement(MenuEntry_t *entry, MenuOption_t *object, MenuMovement_t direction)
{
    int32_t newValueIndex = object->currentOption;

    switch (direction)
    {
        case MM_Left:
            --newValueIndex;
            if (newValueIndex >= 0)
                break;
            fallthrough__;
        case MM_AllTheWayRight:
            newValueIndex = object->options->numOptions-1;
            break;

        case MM_Right:
            ++newValueIndex;
            if (newValueIndex < object->options->numOptions)
                break;
            fallthrough__;
       case MM_AllTheWayLeft:
            newValueIndex = 0;
            break;

        default:
            break;
    }

    return Menu_RunInput_EntryOption_Modify(entry, object, newValueIndex);
}

static int32_t Menu_RunInput_EntryOption_Activate(MenuEntry_t *entry, MenuOption_t *object)
{
    if (object->options->features & 2)
        return Menu_RunInput_EntryOption_Movement(entry, object, MM_Right);
    else
    {
        object->options->currentEntry = object->currentOption >= 0 ? object->currentOption : 0;
        Menu_RunInput_EntryOptionList_MovementVerify(object);

        return 0;
    }
}

static int32_t Menu_RunInput_EntryOptionList_Activate(MenuEntry_t *entry, MenuOption_t *object)
{
    if (!Menu_RunInput_EntryOption_Modify(entry, object, object->options->currentEntry))
    {
        object->options->currentEntry = -1;

        return 0;
    }

    return -1;
}

static void Menu_RunInput_EntryCustom2Col_Activate(MenuEntry_t *entry)
{
    MenuCustom2Col_t *object = (MenuCustom2Col_t*)entry->entry;

    Menu_Custom2ColScreen(/*entry*/);

    object->screenOpen = 1;
}

static void Menu_RunInput_EntryRangeInt32_MovementVerify(MenuEntry_t *entry, MenuRangeInt32_t *object, int32_t newValue)
{
    if (!Menu_EntryRangeInt32Modify(entry, newValue))
        *object->variable = newValue;
}

static void Menu_RunInput_EntryRangeInt32_MovementArbitrary(MenuEntry_t *entry, MenuRangeInt32_t *object, int32_t newValue)
{
    if (object->flags & EnforceIntervals)
    {
        int32_t const range = object->max - object->min;
        int32_t const maxInterval = object->steps - 1;
        int32_t const newValueIndex = roundscale(newValue - object->min, maxInterval, range);
        newValue = newValueIndex * range / maxInterval + object->min;
    }

    Menu_RunInput_EntryRangeInt32_MovementVerify(entry, object, newValue);
}

static void Menu_RunInput_EntryRangeInt32_Movement(MenuEntry_t *entry, MenuRangeInt32_t *object, MenuMovement_t direction)
{
    int32_t const oldValue = *object->variable;
    int32_t const range = object->max - object->min;
    int32_t const maxInterval = object->steps - 1;
    int32_t newValueIndex = roundscale(oldValue - object->min, maxInterval, range);
    int32_t const newValueProjected = newValueIndex * range / maxInterval + object->min;

    switch (direction)
    {
        case MM_Left:
            if (newValueProjected >= oldValue)
                --newValueIndex;
            if (newValueIndex >= 0)
                break;
            fallthrough__;
        case MM_AllTheWayLeft:
            newValueIndex = 0;
            break;

        case MM_Right:
            if (newValueProjected <= oldValue)
                ++newValueIndex;
            if (newValueIndex <= maxInterval)
                break;
            fallthrough__;
        case MM_AllTheWayRight:
            newValueIndex = maxInterval;
            break;

        default:
            break;
    }

    int32_t const newValue = newValueIndex * range / maxInterval + object->min;
    Menu_RunInput_EntryRangeInt32_MovementVerify(entry, object, newValue);
}

static void Menu_RunInput_EntryRangeFloat_MovementVerify(MenuEntry_t *entry, MenuRangeFloat_t *object, float newValue)
{
    if (!Menu_EntryRangeFloatModify(entry, newValue))
    {
        *object->variable = newValue;
        Menu_EntryRangeFloatDidModify(entry);
    }
}

static void Menu_RunInput_EntryRangeFloat_MovementArbitrary(MenuEntry_t *entry, MenuRangeFloat_t *object, float newValue)
{
    if (object->flags & EnforceIntervals)
    {
        float const range = object->max - object->min;
        float const maxInterval = (float)(object->steps - 1);
        float const newValueIndex = rintf((newValue - object->min) * maxInterval / range);
        newValue = newValueIndex * range / maxInterval + object->min;
    }

    Menu_RunInput_EntryRangeFloat_MovementVerify(entry, object, newValue);
}

static void Menu_RunInput_EntryRangeFloat_Movement(MenuEntry_t *entry, MenuRangeFloat_t *object, MenuMovement_t direction)
{
    float const oldValue = *object->variable;
    float const range = object->max - object->min;
    float const maxInterval = (float)(object->steps - 1);
    float newValueIndex = rintf((oldValue - object->min) * maxInterval / range);
    float const newValueProjected = newValueIndex * range / maxInterval + object->min;

    switch (direction)
    {
        case MM_Left:
            if (newValueProjected >= oldValue)
                newValueIndex -= 1.f;
            if (newValueIndex >= 0.f)
                break;
            fallthrough__;
        case MM_AllTheWayLeft:
            newValueIndex = 0.f;
            break;

        case MM_Right:
            if (newValueProjected <= oldValue)
                newValueIndex += 1.f;
            if (newValueIndex <= maxInterval)
                break;
            fallthrough__;
        case MM_AllTheWayRight:
            newValueIndex = maxInterval;
            break;

        default:
            break;
    }

    float const newValue = newValueIndex * range / maxInterval + object->min;
    Menu_RunInput_EntryRangeFloat_MovementVerify(entry, object, newValue);
}

#ifdef MENU_ENABLE_RANGEDOUBLE
static void Menu_RunInput_EntryRangeDouble_MovementVerify(/*MenuEntry_t *entry, */MenuRangeDouble_t *object, double newValue)
{
    if (!Menu_EntryRangeDoubleModify(/*entry, newValue*/))
        *object->variable = newValue;
}

static void Menu_RunInput_EntryRangeDouble_MovementArbitrary(/*MenuEntry_t *entry, */MenuRangeDouble_t *object, double newValue)
{
    if (object->flags & EnforceIntervals)
    {
        double const range = object->max - object->min;
        double const maxInterval = object->steps - 1;
        double const newValueIndex = rint((newValue - object->min) * maxInterval / range);
        newValue = newValueIndex * range / maxInterval + object->min;
    }

    Menu_RunInput_EntryRangeDouble_MovementVerify(/*entry, */object, newValue);
}

static void Menu_RunInput_EntryRangeDouble_Movement(/*MenuEntry_t *entry, */MenuRangeDouble_t *object, MenuMovement_t direction)
{
    double const oldValue = *object->variable;
    double const range = object->max - object->min;
    double const maxInterval = object->steps - 1;
    double newValueIndex = rint((oldValue - object->min) * maxInterval / range);
    double const newValueProjected = newValueIndex * range / maxInterval + object->min;

    switch (direction)
    {
        case MM_Left:
            if (newValueProjected >= oldValue)
                newValueIndex -= 1.;
            if (newValueIndex >= 0.)
                break;
            fallthrough__;
        case MM_AllTheWayLeft:
            newValueIndex = 0.;
            break;

        case MM_Right:
            if (newValueProjected <= oldValue)
                newValueIndex += 1.;
            if (newValueIndex <= maxInterval)
                break;
            fallthrough__;
        case MM_AllTheWayRight:
            newValueIndex = maxInterval;
            break;

        default:
            break;
    }

    double const newValue = newValueIndex * range / maxInterval + object->min;
    Menu_RunInput_EntryRangeDouble_MovementVerify(/*entry, */object, newValue);
}
#endif

static void Menu_RunInput_EntryString_Activate(MenuEntry_t *entry)
{
    MenuString_t *object = (MenuString_t*)entry->entry;

    if (object->variable)
        strncpy(typebuf, object->variable, TYPEBUFSIZE);
    else
        typebuf[0] = '\0';
    object->editfield = typebuf;

    // this limitation is an arbitrary implementation detail
    if (object->bufsize > TYPEBUFSIZE)
        object->bufsize = TYPEBUFSIZE;

    Menu_EntryStringActivate(/*entry*/);
    WithSDL2_StartTextInput();
}

static void Menu_RunInput_EntryString_Submit(/*MenuEntry_t *entry, */MenuString_t *object)
{
    if (!Menu_EntryStringSubmit(/*entry, */object->editfield))
    {
        if (object->variable)
            strncpy(object->variable, object->editfield, object->bufsize);
    }

    object->editfield = NULL;
    WithSDL2_StopTextInput();
}

static void Menu_RunInput_EntryString_Cancel(/*MenuEntry_t *entry, */MenuString_t *object)
{
    Menu_EntryStringCancel(/*entry*/);

    object->editfield = NULL;
    WithSDL2_StopTextInput();
}

static void Menu_RunInput_FileSelect_MovementVerify(MenuFileSelect_t *object)
{
    const int32_t listytop = object->format[object->currentList]->pos.y;
    const int32_t listybottom = klabs(object->format[object->currentList]->bottomcutoff);
    const int32_t ytop = listytop + object->findhigh[object->currentList]->type * (object->font[object->currentList]->get_yline() + object->getMarginBottom(object->currentList));
    const int32_t ybottom = ytop + object->font[object->currentList]->get_yline();

    if (ybottom - object->scrollPos[object->currentList] > listybottom)
        object->scrollPos[object->currentList] = ybottom - listybottom;
    else if (ytop - object->scrollPos[object->currentList] < listytop)
        object->scrollPos[object->currentList] = ytop - listytop;
}

static void Menu_RunInput_FileSelect_Movement(MenuFileSelect_t *object, MenuMovement_t direction)
{
    switch (direction)
    {
        case MM_Up:
            if (!object->findhigh[object->currentList])
                break;
            if (object->findhigh[object->currentList]->prev)
            {
                object->findhigh[object->currentList] = object->findhigh[object->currentList]->prev;
                break;
            }
            fallthrough__;
        case MM_End:
            object->findhigh[object->currentList] = object->findhigh[object->currentList]->userb;
            break;

        case MM_Down:
            if (!object->findhigh[object->currentList])
                break;
            if (object->findhigh[object->currentList]->next)
            {
                object->findhigh[object->currentList] = object->findhigh[object->currentList]->next;
                break;
            }
            fallthrough__;
        case MM_Home:
            object->findhigh[object->currentList] = object->findhigh[object->currentList]->usera;
            break;

        case MM_Swap:
            object->currentList = !object->currentList;
            break;

        default:
            break;
    }

    Menu_RunInput_FileSelect_MovementVerify(object);
}

static void Menu_RunInput_FileSelect_Select(MenuFileSelect_t *object)
{
    if (!object->findhigh[object->currentList])
        return;

    Bstrcat(object->destination, object->findhigh[object->currentList]->name);

    if (object->currentList == 0)
    {
        Bstrcat(object->destination, "/");
        Bcorrectfilename(object->destination, 1);

        Menu_FileSelectInit(object);
    }
    else
    {
        Menu_FileSelect(1);
    }
}

static void Menu_RunInput(Menu_t *cm)
{
    switch (cm->type)
    {
        case Panel:
        {
            MenuPanel_t *panel = (MenuPanel_t*)cm->object;

            if (I_ReturnTrigger() || Menu_RunInput_MouseReturn())
            {
                I_ReturnTriggerClear();
                m_mousecaught = 1;

                S_PlaySound(EXITMENUSOUND);

                Menu_AnimateChange(cm->parentID, cm->parentAnimation);
            }
            else if (I_PanelUp())
            {
                I_PanelUpClear();

                S_PlaySound(KICK_HIT);
                Menu_AnimateChange(panel->previousID, panel->previousAnimation);
            }
            else if (I_PanelDown() || Menu_RunInput_MouseAdvance())
            {
                I_PanelDownClear();
                m_mousecaught = 1;

                S_PlaySound(KICK_HIT);
                Menu_AnimateChange(panel->nextID, panel->nextAnimation);
            }
            break;
        }

        case TextForm:
        {
            MenuTextForm_t *object = (MenuTextForm_t*)cm->object;
            int32_t hitstate = I_EnterText(object->input, object->bufsize-1, 0);

            if (hitstate == -1 || Menu_RunInput_MouseReturn())
            {
                m_mousecaught = 1;

                S_PlaySound(EXITMENUSOUND);

                object->input = NULL;

                Menu_AnimateChange(cm->parentID, cm->parentAnimation);
                WithSDL2_StopTextInput();
            }
            else if (hitstate == 1 || Menu_RunInput_MouseAdvance())
            {
                m_mousecaught = 1;

                Menu_TextFormSubmit(object->input);

                object->input = NULL;
                WithSDL2_StopTextInput();
            }
            break;
        }

        case FileSelect:
        {
            MenuFileSelect_t *object = (MenuFileSelect_t*)cm->object;

            if (I_ReturnTrigger() || Menu_RunInput_MouseReturn())
            {
                I_ReturnTriggerClear();
                m_mousecaught = 1;

                S_PlaySound(EXITMENUSOUND);

                object->destination[0] = 0;

                Menu_FileSelect(0);

                Menu_AnimateChange(cm->parentID, MA_Return);
            }
            else if (I_AdvanceTrigger())
            {
                I_AdvanceTriggerClear();

                Menu_RunInput_FileSelect_Select(object);

                S_PlaySound(PISTOL_BODYHIT);
            }
            else if (KB_KeyPressed(sc_Home))
            {
                KB_ClearKeyDown(sc_Home);

                Menu_RunInput_FileSelect_Movement(object, MM_Home);

                S_PlaySound(KICK_HIT);
            }
            else if (KB_KeyPressed(sc_End))
            {
                KB_ClearKeyDown(sc_End);

                Menu_RunInput_FileSelect_Movement(object, MM_End);

                S_PlaySound(KICK_HIT);
            }
            else if (KB_KeyPressed(sc_PgUp))
            {
                int32_t i;

                CACHE1D_FIND_REC *seeker = object->findhigh[object->currentList];

                KB_ClearKeyDown(sc_PgUp);

                for (i = 0; i < 6; ++i)
                {
                    if (seeker && seeker->prev)
                        seeker = seeker->prev;
                }

                if (seeker)
                {
                    object->findhigh[object->currentList] = seeker;

                    Menu_RunInput_FileSelect_MovementVerify(object);

                    S_PlaySound(KICK_HIT);
                }
            }
            else if (KB_KeyPressed(sc_PgDn))
            {
                int32_t i;

                CACHE1D_FIND_REC *seeker = object->findhigh[object->currentList];

                KB_ClearKeyDown(sc_PgDn);

                for (i = 0; i < 6; ++i)
                {
                    if (seeker && seeker->next)
                        seeker = seeker->next;
                }

                if (seeker)
                {
                    object->findhigh[object->currentList] = seeker;

                    Menu_RunInput_FileSelect_MovementVerify(object);

                    S_PlaySound(KICK_HIT);
                }
            }
            else if (I_MenuLeft() || I_MenuRight())
            {
                I_MenuLeftClear();
                I_MenuRightClear();

                if ((object->currentList ? object->fnlist.numdirs : object->fnlist.numfiles) > 0)
                {
                    Menu_RunInput_FileSelect_Movement(object, MM_Swap);

                    S_PlaySound(KICK_HIT);
                }
            }
            else if (I_MenuUp())
            {
                I_MenuUpClear();

                Menu_RunInput_FileSelect_Movement(object, MM_Up);

                S_PlaySound(KICK_HIT);
            }
            else if (I_MenuDown())
            {
                I_MenuDownClear();

                Menu_RunInput_FileSelect_Movement(object, MM_Down);

                S_PlaySound(KICK_HIT);
            }
            else
            {
                // JBF 20040208: seek to first name matching pressed character
                char ch2, ch;
                ch = KB_GetCh();
                if (ch > 0 && ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')))
                {
                    CACHE1D_FIND_REC *seeker = object->findhigh[object->currentList]->usera;
                    if (ch >= 'a')
                        ch -= ('a'-'A');
                    while (seeker)
                    {
                        ch2 = seeker->name[0];
                        if (ch2 >= 'a' && ch2 <= 'z')
                            ch2 -= ('a'-'A');
                        if (ch2 == ch)
                            break;
                        seeker = seeker->next;
                    }
                    if (seeker)
                    {
                        object->findhigh[object->currentList] = seeker;

                        Menu_RunInput_FileSelect_MovementVerify(object);

                        S_PlaySound(KICK_HIT);
                    }
                }
            }

            Menu_PreInput(NULL);
            break;
        }

        case Message:
            if (I_ReturnTrigger() || Menu_RunInput_MouseReturn())
            {
                I_ReturnTriggerClear();
                m_mousecaught = 1;

                S_PlaySound(EXITMENUSOUND);

                Menu_AnimateChange(cm->parentID, cm->parentAnimation);
            }

            if (I_CheckAllInput())
            {
                MenuMessage_t *message = (MenuMessage_t*)cm->object;

                I_ClearAllInput();

                S_PlaySound(EXITMENUSOUND);

                Menu_AnimateChange(message->linkID, message->animation);
            }

            Menu_PreInput(NULL);
            break;

        case Verify:
            if (I_ReturnTrigger() || KB_KeyPressed(sc_N) || Menu_RunInput_MouseReturn())
            {
                I_ReturnTriggerClear();
                KB_ClearKeyDown(sc_N);
                m_mousecaught = 1;

                Menu_Verify(0);

                Menu_AnimateChange(cm->parentID, cm->parentAnimation);

                S_PlaySound(EXITMENUSOUND);
            }

            if (I_AdvanceTrigger() || KB_KeyPressed(sc_Y) || Menu_RunInput_MouseAdvance())
            {
                MenuVerify_t *verify = (MenuVerify_t*)cm->object;

                I_AdvanceTriggerClear();
                KB_ClearKeyDown(sc_Y);
                m_mousecaught = 1;

                Menu_Verify(1);

                Menu_AnimateChange(verify->linkID, verify->animation);

                S_PlaySound(PISTOL_BODYHIT);
            }

            Menu_PreInput(NULL);
            break;

        case Menu:
        {
            int32_t state;

            MenuMenu_t *menu = (MenuMenu_t*)cm->object;
            MenuEntry_t *currentry = menu->entrylist[menu->currentEntry];

            state = Menu_DetermineSpecialState(currentry);

            if (state == 0)
            {
                if (currentry != NULL)
                switch (currentry->type)
                {
                    case Dummy:
                    case Spacer:
                        break;
                    case Link:
                        if (currentry->flags & MEF_Disabled)
                            break;
                        if (I_AdvanceTrigger())
                        {
                            I_AdvanceTriggerClear();

                            Menu_RunInput_EntryLink_Activate(currentry);

                            if (g_player[myconnectindex].ps->gm&MODE_MENU) // for skill selection
                                S_PlaySound(PISTOL_BODYHIT);
                        }
                        break;
                    case Option:
                    {
                        MenuOption_t *object = (MenuOption_t*)currentry->entry;

                        if (currentry->flags & MEF_Disabled)
                            break;

                        if (I_AdvanceTrigger())
                        {
                            I_AdvanceTriggerClear();

                            Menu_RunInput_EntryOption_Activate(currentry, object);

                            S_PlaySound(PISTOL_BODYHIT);
                        }
                        else if (I_MenuRight())
                        {
                            I_MenuRightClear();

                            Menu_RunInput_EntryOption_Movement(currentry, object, MM_Right);

                            S_PlaySound(PISTOL_BODYHIT);
                        }
                        else if (I_MenuLeft())
                        {
                            I_MenuLeftClear();

                            Menu_RunInput_EntryOption_Movement(currentry, object, MM_Left);

                            S_PlaySound(PISTOL_BODYHIT);
                        }
                    }
                        break;
                    case Custom2Col:
                        if (I_MenuLeft() || I_MenuRight())
                        {
                            I_MenuLeftClear();
                            I_MenuRightClear();

                            Menu_RunInput_Menu_Movement(menu, MM_Swap);

                            S_PlaySound(KICK_HIT);
                        }

                        if (currentry->flags & MEF_Disabled)
                            break;

                        if (I_AdvanceTrigger())
                        {
                            I_AdvanceTriggerClear();

                            Menu_RunInput_EntryCustom2Col_Activate(currentry);

                            S_PlaySound(PISTOL_BODYHIT);
                        }
                        break;
                    case RangeInt32:
                    {
                        MenuRangeInt32_t *object = (MenuRangeInt32_t*)currentry->entry;

                        if (currentry->flags & MEF_Disabled)
                            break;

                        if (I_SliderLeft())
                        {
                            I_SliderLeftClear();

                            Menu_RunInput_EntryRangeInt32_Movement(currentry, object, MM_Left);

                            S_PlaySound(KICK_HIT);
                        }
                        else if (I_SliderRight())
                        {
                            I_SliderRightClear();

                            Menu_RunInput_EntryRangeInt32_Movement(currentry, object, MM_Right);

                            S_PlaySound(KICK_HIT);
                        }
                        break;
                    }
                    case RangeFloat:
                    {
                        MenuRangeFloat_t *object = (MenuRangeFloat_t*)currentry->entry;

                        if (currentry->flags & MEF_Disabled)
                            break;

                        if (I_SliderLeft())
                        {
                            I_SliderLeftClear();

                            Menu_RunInput_EntryRangeFloat_Movement(currentry, object, MM_Left);

                            S_PlaySound(KICK_HIT);
                        }
                        else if (I_SliderRight())
                        {
                            I_SliderRightClear();

                            Menu_RunInput_EntryRangeFloat_Movement(currentry, object, MM_Right);

                            S_PlaySound(KICK_HIT);
                        }
                        break;
                    }
#ifdef MENU_ENABLE_RANGEDOUBLE
                    case RangeDouble:
                    {
                        MenuRangeDouble_t *object = (MenuRangeDouble_t*)currentry->entry;

                        if (currentry->flags & MEF_Disabled)
                            break;

                        if (I_SliderLeft())
                        {
                            I_SliderLeftClear();

                            Menu_RunInput_EntryRangeDouble_Movement(/*currentry, */object, MM_Left);

                            S_PlaySound(KICK_HIT);
                        }
                        else if (I_SliderRight())
                        {
                            I_SliderRightClear();

                            Menu_RunInput_EntryRangeDouble_Movement(/*currentry, */object, MM_Right);

                            S_PlaySound(KICK_HIT);
                        }
                        break;
                    }
#endif

                    case String:
                    {
                        if (currentry->flags & MEF_Disabled)
                            break;

                        if (I_AdvanceTrigger())
                        {
                            I_AdvanceTriggerClear();

                            Menu_RunInput_EntryString_Activate(currentry);

                            S_PlaySound(PISTOL_BODYHIT);
                        }

                        break;
                    }
                }

                if (I_ReturnTrigger() || I_EscapeTrigger() || Menu_RunInput_MouseReturn())
                {
                    I_ReturnTriggerClear();
                    I_EscapeTriggerClear();
                    m_mousecaught = 1;

                    if (cm->parentID != MENU_CLOSE || (g_player[myconnectindex].ps->gm & MODE_GAME))
                        S_PlaySound(EXITMENUSOUND);

                    Menu_AnimateChange(cm->parentID, cm->parentAnimation);
                }
                else if (KB_KeyPressed(sc_Home))
                {
                    KB_ClearKeyDown(sc_Home);

                    S_PlaySound(KICK_HIT);

                    currentry = Menu_RunInput_Menu_Movement(menu, MM_Home);
                }
                else if (KB_KeyPressed(sc_End))
                {
                    KB_ClearKeyDown(sc_End);

                    S_PlaySound(KICK_HIT);

                    currentry = Menu_RunInput_Menu_Movement(menu, MM_End);
                }
                else if (I_MenuUp())
                {
                    I_MenuUpClear();

                    S_PlaySound(KICK_HIT);

                    currentry = Menu_RunInput_Menu_Movement(menu, MM_Up);
                }
                else if (I_MenuDown())
                {
                    I_MenuDownClear();

                    S_PlaySound(KICK_HIT);

                    currentry = Menu_RunInput_Menu_Movement(menu, MM_Down);
                }

                if (currentry != NULL && !(currentry->flags & MEF_Disabled))
                    Menu_PreInput(currentry);
            }
            else if (state == 1)
            {
                if (currentry->type == String)
                {
                    MenuString_t *object = (MenuString_t*)currentry->entry;

                    int32_t hitstate = I_EnterText(object->editfield, object->bufsize-1, object->flags);

                    if (hitstate == -1 || Menu_RunInput_MouseReturn())
                    {
                        m_mousecaught = 1;

                        Menu_RunInput_EntryString_Cancel(/*currentry, */object);

                        S_PlaySound(EXITMENUSOUND);
                    }
                    else if (hitstate == 1)
                    {
                        Menu_RunInput_EntryString_Submit(/*currentry, */object);

                        S_PlaySound(PISTOL_BODYHIT);
                    }
                }
            }
            else if (state == 2)
            {
                if (currentry->type == Option)
                {
                    MenuOption_t *object = (MenuOption_t*)currentry->entry;

                    if (I_ReturnTrigger() || Menu_RunInput_MouseReturn())
                    {
                        I_ReturnTriggerClear();
                        m_mousecaught = 1;

                        S_PlaySound(EXITMENUSOUND);

                        object->options->currentEntry = -1;
                    }
                    else if (I_AdvanceTrigger())
                    {
                        I_AdvanceTriggerClear();

                        if (!Menu_RunInput_EntryOptionList_Activate(currentry, object))
                            S_PlaySound(PISTOL_BODYHIT);
                    }
                    else if (KB_KeyPressed(sc_Home))
                    {
                        KB_ClearKeyDown(sc_Home);

                        S_PlaySound(KICK_HIT);

                        Menu_RunInput_EntryOptionList_Movement(object, MM_Home);
                    }
                    else if (KB_KeyPressed(sc_End))
                    {
                        KB_ClearKeyDown(sc_End);

                        S_PlaySound(KICK_HIT);

                        Menu_RunInput_EntryOptionList_Movement(object, MM_End);
                    }
                    else if (I_MenuUp())
                    {
                        I_MenuUpClear();

                        S_PlaySound(KICK_HIT);

                        Menu_RunInput_EntryOptionList_Movement(object, MM_Up);
                    }
                    else if (I_MenuDown())
                    {
                        I_MenuDownClear();

                        S_PlaySound(KICK_HIT);

                        Menu_RunInput_EntryOptionList_Movement(object, MM_Down);
                    }
                }
                else if (currentry->type == Custom2Col)
                {
                    if (I_EscapeTrigger() || Menu_RunInput_MouseReturn())
                    {
                        I_EscapeTriggerClear();
                        m_mousecaught = 1;

                        S_PlaySound(EXITMENUSOUND);

                        ((MenuCustom2Col_t*)currentry->entry)->screenOpen = 0;
                    }
                    else if (Menu_PreCustom2ColScreen(currentry))
                        ((MenuCustom2Col_t*)currentry->entry)->screenOpen = 0;
                }
            }

            break;
        }
    }
}

// This function MUST NOT RECURSE. That is why Menu_Run is separate.
void M_DisplayMenus(void)
{
    vec2_t origin = { 0, 0 }, previousOrigin = { 0, 0 };

    Net_GetPackets();

    if ((g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
    {
        walock[TILE_LOADSHOT] = 1;
        return;
    }

    if (!Menu_IsTextInput(m_currentMenu) && KB_KeyPressed(sc_Q))
        Menu_AnimateChange(MENU_QUIT, MA_Advance);

    int32_t mousestatus = readmouseabsxy(&m_mousepos, &mouseabs);
    if (mousestatus && mousepressstate == Mouse_Pressed)
        m_mousedownpos = m_mousepos;

    Menu_RunInput(m_currentMenu);

    g_player[myconnectindex].ps->gm &= (0xff-MODE_TYPE);
    // g_player[myconnectindex].ps->fta = 0;

    int32_t const backgroundOK = Menu_BlackTranslucentBackgroundOK(g_currentMenu);

    // need EVENT_DISPLAYMENUBACKGROUND here

    if (!KXDWN && ((g_player[myconnectindex].ps->gm&MODE_GAME) || ud.recstat==2) && backgroundOK)
        fade_screen_black(1);

    if (Menu_UpdateScreenOK(g_currentMenu))
        G_UpdateScreenArea();

#if !defined EDUKE32_TOUCH_DEVICES
    if (m_menuchange_watchpoint > 0)
        m_menuchange_watchpoint++;
#endif

    if (m_parentMenu)
    {
        ud.m_origin = origin;
        VM_OnEventWithReturn(EVENT_DISPLAYINACTIVEMENU, g_player[screenpeek].ps->i, screenpeek, m_parentMenu->menuID);
        origin = ud.m_origin;
    }

    // Determine animation values.
    if (totalclock < m_animation.start + m_animation.length)
    {
        const int32_t screenwidth = scale(240<<16, xdim, ydim);

        origin.x = mulscale15(screenwidth, m_animation.in(&m_animation));
        previousOrigin.x = mulscale15(screenwidth, m_animation.out(&m_animation));

        ud.m_origin = previousOrigin;
        VM_OnEventWithReturn(EVENT_DISPLAYINACTIVEMENU, g_player[screenpeek].ps->i, screenpeek, m_animation.previous->menuID);
        previousOrigin = ud.m_origin;
    }

    ud.m_origin = origin;
    VM_OnEventWithReturn(EVENT_DISPLAYMENU, g_player[screenpeek].ps->i, screenpeek, g_currentMenu);
    origin = ud.m_origin;

    if (m_parentMenu && backgroundOK)
    {
        Menu_Run(m_parentMenu, origin);
    }

    // hack; need EVENT_DISPLAYMENUBACKGROUND above
    if (KXDWN && ((g_player[myconnectindex].ps->gm&MODE_GAME) || ud.recstat==2 || m_parentMenu != NULL) && backgroundOK)
        fade_screen_black(1);

    // Display the menu, with a transition animation if applicable.
    if (totalclock < m_animation.start + m_animation.length)
    {
        Menu_Run(m_animation.previous, previousOrigin);
        Menu_Run(m_animation.current, origin);
    }
    else
        Menu_Run(m_currentMenu, origin);

#if !defined EDUKE32_TOUCH_DEVICES
    if (m_menuchange_watchpoint >= 3)
        m_menuchange_watchpoint = 0;
#endif

    if (m_parentMenu)
    {
        ud.m_origin = origin;
        VM_OnEventWithReturn(EVENT_DISPLAYINACTIVEMENUREST, g_player[screenpeek].ps->i, screenpeek, m_parentMenu->menuID);
    }

    if (totalclock < m_animation.start + m_animation.length)
    {
        ud.m_origin = previousOrigin;
        VM_OnEventWithReturn(EVENT_DISPLAYINACTIVEMENUREST, g_player[screenpeek].ps->i, screenpeek, m_animation.previous->menuID);
    }

    ud.m_origin = origin;
    VM_OnEventWithReturn(EVENT_DISPLAYMENUREST, g_player[screenpeek].ps->i, screenpeek, g_currentMenu);

#if !defined EDUKE32_TOUCH_DEVICES
    if (tilesiz[CROSSHAIR].x > 0 && mousestatus)
#else
    if (mousestatus)
#endif
    {
#if !defined EDUKE32_TOUCH_DEVICES
        if (!MOUSEACTIVECONDITION)
            m_mousewake_watchpoint = 1;
#endif

        if (MOUSEACTIVECONDITIONAL(mousepressstateadvance()) || m_mousepos.x != m_prevmousepos.x || m_mousepos.y != m_prevmousepos.y)
        {
            m_prevmousepos = m_mousepos;
            m_mouselastactivity = totalclock;
        }
#if !defined EDUKE32_TOUCH_DEVICES
        else
            m_mousewake_watchpoint = 0;
#endif

        m_mousecaught = 0;
    }
    else
    {
        m_mouselastactivity = -M_MOUSETIMEOUT;

#if !defined EDUKE32_TOUCH_DEVICES
        m_mousewake_watchpoint = 0;
#endif
    }

#ifndef EDUKE32_TOUCH_DEVICES
    // Display the mouse cursor, except on touch devices.
    if (MOUSEACTIVECONDITION)
    {
        int32_t a = VM_OnEventWithReturn(EVENT_DISPLAYCURSOR, g_player[screenpeek].ps->i, screenpeek, CROSSHAIR);

        if ((unsigned) a < MAXTILES)
            rotatesprite_fs_alpha(m_mousepos.x, m_mousepos.y, 65536, 0, a, 0, CROSSHAIR_PAL, 2|1, MOUSEALPHA);
    }
    else
        mousepressstate = Mouse_Idle;
#endif

    if ((g_player[myconnectindex].ps->gm&MODE_MENU) != MODE_MENU)
    {
        G_UpdateScreenArea();
        CAMERACLOCK = totalclock;
        CAMERADIST = 65536;
    }
}
