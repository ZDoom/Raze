//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

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
#include "renderlayer.h"
#include "net.h"
#include "player.h"
#include "mouse.h"
#include "joystick.h"
#include "osd.h"
#include "osdcmds.h"
#include "gamedef.h"
#include "gameexec.h"
#include "savegame.h"
#include "premap.h"
#include "demo.h"
#include "xxhash.h"
#include "common.h"
#include "common_game.h"
#include "input.h"
#include "menus.h"

#include <sys/stat.h>

#include "android/in_android.h"
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

FORCE_INLINE void WithSDL2_StartTextInput()
{
#if defined EDUKE32_TOUCH_DEVICES && defined SDL_MAJOR_VERSION && SDL_MAJOR_VERSION > 1
    SDL_StartTextInput();
#endif
}

FORCE_INLINE void WithSDL2_StopTextInput()
{
#if defined EDUKE32_TOUCH_DEVICES && defined SDL_MAJOR_VERSION && SDL_MAJOR_VERSION > 1
    SDL_StopTextInput();
#endif
}

#define mgametext(x,y,t) G_ScreenText(STARTALPHANUM, x, y, 65536, 0, 0, t, 0, 0, 2|8|16|ROTATESPRITE_FULL16, 0, 5<<16, 8<<16, -1<<16, 0, 0, 0, 0, xdim-1, ydim-1)
#define mgametextcenter(x,y,t) G_ScreenText(STARTALPHANUM, (MENU_MARGIN_CENTER<<16) + (x), y, 65536, 0, 0, t, 0, 0, 2|8|16|ROTATESPRITE_FULL16, 0, 5<<16, 8<<16, -1<<16, 1<<16, TEXT_XCENTER, 0, 0, xdim-1, ydim-1)
#define mminitext(x,y,t,p) minitext_(x, y, t, 0, p, 2|8|16|ROTATESPRITE_FULL16)
#define mmenutext(x,y,t) G_ScreenText(BIGALPHANUM, x, (y) - (12<<16), 65536L, 0, 0, (const char *)OSD_StripColors(menutextbuf,t), 0, 0, 2|8|16|ROTATESPRITE_FULL16, 0, 5<<16, 16<<16, 0, 0, TEXT_BIGALPHANUM|TEXT_UPPERCASE|TEXT_LITERALESCAPE, 0, 0, xdim-1, ydim-1)
#define mmenutextcenter(x,y,t) G_ScreenText(BIGALPHANUM, (MENU_MARGIN_CENTER<<16) + (x), (y) - (12<<16), 65536L, 0, 0, (const char *)OSD_StripColors(menutextbuf,t), 0, 0, 2|8|16|ROTATESPRITE_FULL16, 0, 5<<16, 16<<16, 0, 0, TEXT_BIGALPHANUM|TEXT_UPPERCASE|TEXT_LITERALESCAPE|TEXT_XCENTER, 0, 0, xdim-1, ydim-1)

static void shadowminitext(int32_t x, int32_t y, const char *t, int32_t p)
{
    int32_t f = 0;

    if (!minitext_lowercase)
        f |= TEXT_UPPERCASE;

    G_ScreenTextShadow(1, 1, MINIFONT, x, y, 65536, 0, 0, t, 0, p, 2|8|16|ROTATESPRITE_FULL16, 0, 4<<16, 8<<16, 1<<16, 0, f, 0, 0, xdim-1, ydim-1);
}
static void creditsminitext(int32_t x, int32_t y, const char *t, int32_t p)
{
    int32_t f = TEXT_XCENTER;

    if (!minitext_lowercase)
        f |= TEXT_UPPERCASE;

    G_ScreenTextShadow(1, 1, MINIFONT, x, y, 65536, 0, 0, t, 0, p, 2|8|16|ROTATESPRITE_FULL16, 0, 4<<16, 8<<16, 1<<16, 0, f, 0, 0, xdim-1, ydim-1);
}

int32_t menutext_(int32_t x, int32_t y, int32_t s, int32_t p, char *t, int32_t bits)
{
    vec2_t dim;
    int32_t f = TEXT_BIGALPHANUM|TEXT_UPPERCASE|TEXT_LITERALESCAPE;

    if (!(bits & ROTATESPRITE_FULL16))
    {
        x<<=16;
        y<<=16;
    }

    if (x == (160<<16))
        f |= TEXT_XCENTER;

    dim = G_ScreenText(BIGALPHANUM, x, y - (12<<16), 65536L, 0, 0, t, s, p, bits|ROTATESPRITE_FULL16, 0, 5<<16, 16<<16, 0, 0, f, 0, 0, xdim-1, ydim-1);

    if (!(bits & ROTATESPRITE_FULL16))
        x >>= 16;

    return dim.x;
}

#pragma pack(push,1)
static savehead_t savehead;
#pragma pack(pop)

static void M_DrawBackground(const vec2_t origin)
{
    rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + (100<<16), 65536L,0,MENUSCREEN,16,0,10+64);
}

static void M_DrawTopBar(const vec2_t origin)
{
    rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + (19<<16), 65536L,0,MENUBAR,16,0,10);
}

static void M_DrawTopBarCaption(const char *caption, const vec2_t origin)
{
    char *s = Bstrdup(caption), p = Bstrlen(caption)-1;
    if (s[p] == ':') s[p] = 0;
    mmenutextcenter(origin.x, origin.y + (24<<16), s);
    Bfree(s);
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
static MenuFont_t MF_Null             = { -1, 10,  0,  0,     0,      0,      0,     0, 0 };
static MenuFont_t MF_Redfont          = { -1, 10,  0,  1, 5<<16, 15<<16,  0<<16, 0<<16, TEXT_BIGALPHANUM | TEXT_UPPERCASE };
static MenuFont_t MF_RedfontBlue      = { -1, 10,  1,  1, 5<<16, 15<<16,  0<<16, 0<<16, TEXT_BIGALPHANUM | TEXT_UPPERCASE };
static MenuFont_t MF_RedfontGreen     = { -1, 10,  8,  1, 5<<16, 15<<16,  0<<16, 0<<16, TEXT_BIGALPHANUM | TEXT_UPPERCASE };
static MenuFont_t MF_Bluefont         = { -1, 10,  0, 16, 5<<16,  7<<16, -1<<16, 0<<16, 0 };
static MenuFont_t MF_BluefontRed      = { -1, 10, 10, 16, 5<<16,  7<<16, -1<<16, 0<<16, 0 };
static MenuFont_t MF_Minifont         = { -1, 10,  0, 16, 4<<16,  5<<16,  1<<16, 1<<16, 0 };
static MenuFont_t MF_MinifontRed      = { -1, 16, 21, 16, 4<<16,  5<<16,  1<<16, 1<<16, 0 };
static MenuFont_t MF_MinifontDarkGray = { -1, 10, 13, 16, 4<<16,  5<<16,  1<<16, 1<<16, 0 };


static MenuMenuFormat_t MMF_Top_Main =             { {  MENU_MARGIN_CENTER<<16, 55<<16, }, -170<<16 };
static MenuMenuFormat_t MMF_Top_Episode =          { {  MENU_MARGIN_CENTER<<16, 48<<16, }, -190<<16 };
static MenuMenuFormat_t MMF_Top_Skill =            { {  MENU_MARGIN_CENTER<<16, 58<<16, }, -190<<16 };
static MenuMenuFormat_t MMF_Top_Options =          { {  MENU_MARGIN_CENTER<<16, 38<<16, }, -190<<16 };
static MenuMenuFormat_t MMF_Top_Joystick_Network = { {  MENU_MARGIN_CENTER<<16, 70<<16, }, -190<<16 };
static MenuMenuFormat_t MMF_BigOptions =           { {    MENU_MARGIN_WIDE<<16, 38<<16, }, -190<<16 };
static MenuMenuFormat_t MMF_SmallOptions =         { {    MENU_MARGIN_WIDE<<16, 37<<16, },  170<<16 };
static MenuMenuFormat_t MMF_Macros =               { {                  26<<16, 40<<16, },  160<<16 };
static MenuMenuFormat_t MMF_SmallOptionsNarrow  =  { { MENU_MARGIN_REGULAR<<16, 38<<16, }, -190<<16 };
static MenuMenuFormat_t MMF_KeyboardSetupFuncs =   { {                  70<<16, 34<<16, },  151<<16 };
static MenuMenuFormat_t MMF_MouseJoySetupBtns =    { {                  76<<16, 34<<16, },  143<<16 };
static MenuMenuFormat_t MMF_FuncList =             { {                 100<<16, 51<<16, },  152<<16 };
static MenuMenuFormat_t MMF_ColorCorrect =         { { MENU_MARGIN_REGULAR<<16, 86<<16, },  190<<16 };
static MenuMenuFormat_t MMF_BigSliders =           { {    MENU_MARGIN_WIDE<<16, 37<<16, },  190<<16 };
static MenuMenuFormat_t MMF_LoadSave =             { {                 223<<16, 48<<16, },  320<<16 };
static MenuMenuFormat_t MMF_NetSetup =             { {                  36<<16, 38<<16, },  190<<16 };
static MenuMenuFormat_t MMF_FileSelectLeft =       { {                  40<<16, 45<<16, },  162<<16 };
static MenuMenuFormat_t MMF_FileSelectRight =      { {                 164<<16, 45<<16, },  162<<16 };

static MenuEntryFormat_t MEF_Null =             {     0,      0,        0,  20<<16, 65536 };
static MenuEntryFormat_t MEF_MainMenu =         { 4<<16,      0,        0, 110<<16, 65536 };
static MenuEntryFormat_t MEF_CenterMenu =       { 7<<16,      0,        0, 110<<16, 65536 };
static MenuEntryFormat_t MEF_BigOptions =       { 4<<16,      0,  190<<16,  20<<16, 65536 };
static MenuEntryFormat_t MEF_BigOptions_Apply = { 4<<16, 16<<16,  190<<16,  20<<16, 65536 };
static MenuEntryFormat_t MEF_BigOptionsRt =     { 4<<16,      0, -260<<16,  20<<16, 65536 };
#if defined USE_OPENGL || !defined DROIDMENU
static MenuEntryFormat_t MEF_SmallOptions =     { 1<<16,      0,  216<<16,  10<<16, 32768 };
#endif
static MenuEntryFormat_t MEF_BigCheats =        { 3<<16,      0, -260<<16,  20<<16, 65536 };
static MenuEntryFormat_t MEF_Cheats =           { 2<<16,      0, -260<<16,  10<<16, 32768 };
static MenuEntryFormat_t MEF_PlayerNarrow =     { 1<<16,      0,   90<<16,  10<<16, 32768 };
static MenuEntryFormat_t MEF_Macros =           { 2<<16,     -1,  268<<16,  10<<16, 32768 };
static MenuEntryFormat_t MEF_VideoSetup =       { 4<<16,      0,  168<<16,  20<<16, 65536 };
static MenuEntryFormat_t MEF_FuncList =         { 3<<16,      0,  100<<16,  10<<16, 32768 };
static MenuEntryFormat_t MEF_ColorCorrect =     { 2<<16,      0, -240<<16,  20<<16, 65536 };
static MenuEntryFormat_t MEF_BigSliders =       { 2<<16,      0,  170<<16,  20<<16, 65536 };
static MenuEntryFormat_t MEF_LoadSave =         { 7<<16,     -1,   78<<16,  20<<16, 65536 };
static MenuEntryFormat_t MEF_NetSetup =         { 4<<16,      0,  112<<16,  20<<16, 65536 };

// common menu option sets
#define MAKE_MENUOPTIONSET(optionNames, optionValues, features) { optionNames, optionValues, &MMF_FuncList, &MEF_FuncList, &MF_Minifont, ARRAY_SIZE(optionNames), -1, 0, features }
#define MAKE_MENUOPTIONSETDYN(optionNames, optionValues, numOptions, features) { optionNames, optionValues, &MMF_FuncList, &MEF_FuncList, &MF_Minifont, numOptions, -1, 0, features }
#define MAKE_MENUOPTIONSETNULL { NULL, NULL, &MMF_FuncList, &MEF_FuncList, &MF_Minifont, 0, -1, 0, 0 }

static char *MEOSN_OffOn[] = { "Off", "On", };
static MenuOptionSet_t MEOS_OffOn = MAKE_MENUOPTIONSET( MEOSN_OffOn, NULL, 0x3 );
static char *MEOSN_OnOff[] = { "On", "Off", };
static MenuOptionSet_t MEOS_OnOff = MAKE_MENUOPTIONSET( MEOSN_OnOff, NULL, 0x3 );
static char *MEOSN_NoYes[] = { "No", "Yes", };
static MenuOptionSet_t MEOS_NoYes = MAKE_MENUOPTIONSET( MEOSN_NoYes, NULL, 0x3 );
static char *MEOSN_YesNo[] = { "Yes", "No", };
static MenuOptionSet_t MEOS_YesNo = MAKE_MENUOPTIONSET( MEOSN_YesNo, NULL, 0x3 );


static char MenuGameFuncs[NUMGAMEFUNCTIONS][MAXGAMEFUNCLEN];
static char *MenuGameFuncNone = "  -None-";
static char *MEOSN_Gamefuncs[NUMGAMEFUNCTIONS+1];
static MenuOptionSet_t MEOS_Gamefuncs = MAKE_MENUOPTIONSET( MEOSN_Gamefuncs, NULL, 0x1 );



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
static MenuSpacer_t MEO_ ## EntryName = { Height };\
static MenuEntry_t ME_ ## EntryName = MAKE_MENUENTRY( NULL, &MF_Null, &MEF_Null, &MEO_ ## EntryName, Spacer )

MAKE_SPACER( Space2, 2<<16 ); // bigoptions
MAKE_SPACER( Space4, 4<<16 ); // usermap, smalloptions, anything else non-top
MAKE_SPACER( Space6, 6<<16 ); // videosetup
MAKE_SPACER( Space8, 8<<16 ); // colcorr, redslide


#define MAKE_MENU_TOP_ENTRYLINK( Title, Format, EntryName, LinkID ) \
static MenuLink_t MEO_ ## EntryName = { LinkID, MA_Advance, };\
static MenuEntry_t ME_ ## EntryName = MAKE_MENUENTRY( Title, &MF_Redfont, &Format, &MEO_ ## EntryName, Link )


MAKE_MENU_TOP_ENTRYLINK( "New Game", MEF_MainMenu, MAIN_NEWGAME, MENU_EPISODE );
#ifdef DROIDMENU
MAKE_MENU_TOP_ENTRYLINK( "Resume Game", MEF_MainMenu, MAIN_RESUMEGAME, MENU_CLOSE );
#endif
MAKE_MENU_TOP_ENTRYLINK( "New Game", MEF_MainMenu, MAIN_NEWGAME_INGAME, MENU_NEWVERIFY );
static MenuLink_t MEO_MAIN_NEWGAME_NETWORK = { MENU_NETWORK, MA_Advance, };
MAKE_MENU_TOP_ENTRYLINK( "Save Game", MEF_MainMenu, MAIN_SAVEGAME, MENU_SAVE );
MAKE_MENU_TOP_ENTRYLINK( "Load Game", MEF_MainMenu, MAIN_LOADGAME, MENU_LOAD );
MAKE_MENU_TOP_ENTRYLINK( "Options", MEF_MainMenu, MAIN_OPTIONS, MENU_OPTIONS );
#ifndef DROIDMENU
MAKE_MENU_TOP_ENTRYLINK( "Help", MEF_MainMenu, MAIN_HELP, MENU_STORY );
#endif
MAKE_MENU_TOP_ENTRYLINK( "Credits", MEF_MainMenu, MAIN_CREDITS, MENU_CREDITS );
MAKE_MENU_TOP_ENTRYLINK( "Quit To Title", MEF_MainMenu, MAIN_QUITTOTITLE, MENU_QUITTOTITLE );
MAKE_MENU_TOP_ENTRYLINK( "Quit", MEF_MainMenu, MAIN_QUIT, MENU_QUIT );
MAKE_MENU_TOP_ENTRYLINK( "Quit Game", MEF_MainMenu, MAIN_QUITGAME, MENU_QUIT );

static MenuEntry_t *MEL_MAIN[] = {
    &ME_MAIN_NEWGAME,
    &ME_MAIN_LOADGAME,
    &ME_MAIN_OPTIONS,
#ifndef DROIDMENU
    &ME_MAIN_HELP,
#endif
    &ME_MAIN_CREDITS,
    &ME_MAIN_QUIT,
};

static MenuEntry_t *MEL_MAIN_INGAME[] = {
#ifdef DROIDMENU
    &ME_MAIN_RESUMEGAME,
#else
    &ME_MAIN_NEWGAME_INGAME,
#endif
    &ME_MAIN_SAVEGAME,
    &ME_MAIN_LOADGAME,
    &ME_MAIN_OPTIONS,
#ifndef DROIDMENU
    &ME_MAIN_HELP,
    &ME_MAIN_QUITTOTITLE,
    &ME_MAIN_QUITGAME,
#else
    &ME_MAIN_QUITTOTITLE,
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

#if defined STARTUP_SETUP_WINDOW && !defined DROIDMENU
static MenuOption_t MEO_GAMESETUP_STARTWIN = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OffOn, &ud.config.ForceSetup );
static MenuEntry_t ME_GAMESETUP_STARTWIN = MAKE_MENUENTRY( "Startup window:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_STARTWIN, Option );
#endif

static char *MEOSN_GAMESETUP_AIM_AUTO[] = { "None", "Regular", "Bullets only",
#ifdef DROIDMENU
"Extra wide"
#endif
};
static int32_t MEOSV_GAMESETUP_AIM_AUTO[] = { 0, 1, 2,
#ifdef DROIDMENU
3,
#endif
};

static MenuOptionSet_t MEOS_GAMESETUP_AIM_AUTO = MAKE_MENUOPTIONSET( MEOSN_GAMESETUP_AIM_AUTO, MEOSV_GAMESETUP_AIM_AUTO, 0x2 );
static MenuOption_t MEO_GAMESETUP_AIM_AUTO = MAKE_MENUOPTION( &MF_Redfont, &MEOS_GAMESETUP_AIM_AUTO, &ud.config.AutoAim );
static MenuEntry_t ME_GAMESETUP_AIM_AUTO = MAKE_MENUENTRY( "Auto aim:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_AIM_AUTO, Option );

static char *MEOSN_GAMESETUP_WEAPSWITCH_PICKUP[] = { "Never", "If new", "By rating", };
static MenuOptionSet_t MEOS_GAMESETUP_WEAPSWITCH_PICKUP = MAKE_MENUOPTIONSET( MEOSN_GAMESETUP_WEAPSWITCH_PICKUP, NULL, 0x2 );
static MenuOption_t MEO_GAMESETUP_WEAPSWITCH_PICKUP = MAKE_MENUOPTION( &MF_Redfont, &MEOS_GAMESETUP_WEAPSWITCH_PICKUP, NULL );
static MenuEntry_t ME_GAMESETUP_WEAPSWITCH_PICKUP = MAKE_MENUENTRY( "Equip pickups:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_WEAPSWITCH_PICKUP, Option );

static char *MEOSN_DemoRec[] = { "Off", "Running", };
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

#ifdef DROIDMENU
static MenuOption_t MEO_GAMESETUP_CROUCHLOCK = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &droidinput.toggleCrouch);
static MenuEntry_t ME_GAMESETUP_CROUCHLOCK = MAKE_MENUENTRY("Crouch lock:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_CROUCHLOCK, Option);

static MenuOption_t MEO_GAMESETUP_QUICKSWITCH = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &droidinput.quickSelectWeapon);
static MenuEntry_t ME_GAMESETUP_QUICKSWITCH = MAKE_MENUENTRY("Quick weapon switch:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_GAMESETUP_QUICKSWITCH, Option);
#endif

static MenuEntry_t *MEL_GAMESETUP[] = {
    &ME_ADULTMODE,
#if defined STARTUP_SETUP_WINDOW && !defined DROIDMENU
    &ME_GAMESETUP_STARTWIN,
#endif
    &ME_GAMESETUP_AIM_AUTO,
    &ME_GAMESETUP_WEAPSWITCH_PICKUP,
#ifdef DROIDMENU
    &ME_GAMESETUP_QUICKSWITCH,
    &ME_GAMESETUP_CROUCHLOCK,
#else
    &ME_GAMESETUP_DEMOREC,
#ifdef _WIN32
    &ME_GAMESETUP_UPDATES,
#endif
#endif
};

MAKE_MENU_TOP_ENTRYLINK( "Game Setup", MEF_CenterMenu, OPTIONS_GAMESETUP, MENU_GAMESETUP );
MAKE_MENU_TOP_ENTRYLINK( "Sound Setup", MEF_CenterMenu, OPTIONS_SOUNDSETUP, MENU_SOUND );
MAKE_MENU_TOP_ENTRYLINK( "Display Setup", MEF_CenterMenu, OPTIONS_DISPLAYSETUP, MENU_DISPLAYSETUP );
MAKE_MENU_TOP_ENTRYLINK( "Player Setup", MEF_CenterMenu, OPTIONS_PLAYERSETUP, MENU_PLAYER );
MAKE_MENU_TOP_ENTRYLINK( "Control Setup", MEF_CenterMenu, OPTIONS_CONTROLS, MENU_CONTROLS );
MAKE_MENU_TOP_ENTRYLINK( "Keyboard Setup", MEF_CenterMenu, OPTIONS_KEYBOARDSETUP, MENU_KEYBOARDSETUP );
MAKE_MENU_TOP_ENTRYLINK( "Mouse Setup", MEF_CenterMenu, OPTIONS_MOUSESETUP, MENU_MOUSESETUP );
MAKE_MENU_TOP_ENTRYLINK( "Joystick Setup", MEF_CenterMenu, OPTIONS_JOYSTICKSETUP, MENU_JOYSTICKSETUP );
#ifdef DROIDMENU
MAKE_MENU_TOP_ENTRYLINK( "Touch Setup", MEF_CenterMenu, OPTIONS_TOUCHSETUP, MENU_TOUCHSETUP );
#endif
MAKE_MENU_TOP_ENTRYLINK( "Cheats", MEF_CenterMenu, OPTIONS_CHEATS, MENU_CHEATS );

static int32_t newresolution, newrendermode, newfullscreen;

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

static char *MEOSN_VIDEOSETUP_RESOLUTION[MAXVALIDMODES];
static MenuOptionSet_t MEOS_VIDEOSETUP_RESOLUTION = MAKE_MENUOPTIONSETDYN( MEOSN_VIDEOSETUP_RESOLUTION, NULL, 0, 0x0 );
static MenuOption_t MEO_VIDEOSETUP_RESOLUTION = MAKE_MENUOPTION( &MF_Redfont, &MEOS_VIDEOSETUP_RESOLUTION, &newresolution );
static MenuEntry_t ME_VIDEOSETUP_RESOLUTION = MAKE_MENUENTRY( "Resolution:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_RESOLUTION, Option );

#ifdef USE_OPENGL
#ifdef POLYMER
static char *MEOSN_VIDEOSETUP_RENDERER[] = { "Classic", "Polymost", "Polymer", };
static int32_t MEOSV_VIDEOSETUP_RENDERER[] = { REND_CLASSIC, REND_POLYMOST, REND_POLYMER, };
#else
static char *MEOSN_VIDEOSETUP_RENDERER[] = { "Classic", "OpenGL", };
static int32_t MEOSV_VIDEOSETUP_RENDERER[] = { REND_CLASSIC, REND_POLYMOST, };
#endif

static MenuOptionSet_t MEOS_VIDEOSETUP_RENDERER = MAKE_MENUOPTIONSET( MEOSN_VIDEOSETUP_RENDERER, MEOSV_VIDEOSETUP_RENDERER, 0x2 );

static MenuOption_t MEO_VIDEOSETUP_RENDERER = MAKE_MENUOPTION( &MF_Redfont, &MEOS_VIDEOSETUP_RENDERER, &newrendermode );
static MenuEntry_t ME_VIDEOSETUP_RENDERER = MAKE_MENUENTRY( "Renderer:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_RENDERER, Option );
#endif

static MenuOption_t MEO_VIDEOSETUP_FULLSCREEN = MAKE_MENUOPTION( &MF_Redfont, &MEOS_NoYes, &newfullscreen );
static MenuEntry_t ME_VIDEOSETUP_FULLSCREEN = MAKE_MENUENTRY( "Fullscreen:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_FULLSCREEN, Option );
static MenuEntry_t ME_VIDEOSETUP_APPLY = MAKE_MENUENTRY( "Apply Changes", &MF_Redfont, &MEF_BigOptions_Apply, &MEO_NULL, Link );


static MenuLink_t MEO_DISPLAYSETUP_COLORCORR = { MENU_COLCORR, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_COLORCORR = MAKE_MENUENTRY( "Color Correction", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_COLORCORR, Link );
static MenuOption_t MEO_DISPLAYSETUP_PIXELDOUBLING = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OnOff, &ud.detail );
static MenuEntry_t ME_DISPLAYSETUP_PIXELDOUBLING = MAKE_MENUENTRY( "Pixel Doubling:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_PIXELDOUBLING, Option );


#ifdef USE_OPENGL
static MenuOption_t MEO_DISPLAYSETUP_ASPECTRATIO = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, NULL);
#else
static MenuOption_t MEO_DISPLAYSETUP_ASPECTRATIO = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &r_usenewaspect);
#endif
static MenuEntry_t ME_DISPLAYSETUP_ASPECTRATIO = MAKE_MENUENTRY( "Widescreen:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_ASPECTRATIO, Option );
#ifdef POLYMER
static char *MEOSN_DISPLAYSETUP_ASPECTRATIO_POLYMER[] = { "Auto", "4:3", "16:10", "5:3", "16:9", "1.85:1", "2.39:1", };
static double MEOSV_DISPLAYSETUP_ASPECTRATIO_POLYMER[] = { 0., 1.33, 1.6, 1.66, 1.78, 1.85, 2.39, };
static MenuOptionSet_t MEOS_DISPLAYSETUP_ASPECTRATIO_POLYMER = MAKE_MENUOPTIONSET( MEOSN_DISPLAYSETUP_ASPECTRATIO_POLYMER, NULL, 0x1 );
static MenuOption_t MEO_DISPLAYSETUP_ASPECTRATIO_POLYMER = MAKE_MENUOPTION(&MF_Redfont, &MEOS_DISPLAYSETUP_ASPECTRATIO_POLYMER, NULL);
static MenuEntry_t ME_DISPLAYSETUP_ASPECTRATIO_POLYMER = MAKE_MENUENTRY( "Aspect ratio:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_ASPECTRATIO_POLYMER, Option );
#endif


#ifdef USE_OPENGL
static char *MEOSN_DISPLAYSETUP_TEXFILTER[] = { "Classic", "Filtered" };
static MenuOptionSet_t MEOS_DISPLAYSETUP_TEXFILTER = MAKE_MENUOPTIONSET( MEOSN_DISPLAYSETUP_TEXFILTER, NULL, 0x2 );
int32_t menufiltermode;
static MenuOption_t MEO_DISPLAYSETUP_TEXFILTER = MAKE_MENUOPTION( &MF_Redfont, &MEOS_DISPLAYSETUP_TEXFILTER, &menufiltermode );
static MenuEntry_t ME_DISPLAYSETUP_TEXFILTER = MAKE_MENUENTRY( "Texture Mode:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_TEXFILTER, Option );

static char *MEOSN_DISPLAYSETUP_ANISOTROPY[] = { "None", "2x", "4x", "8x", "16x", };
static int32_t MEOSV_DISPLAYSETUP_ANISOTROPY[] = { 1, 2, 4, 8, 16, };
static MenuOptionSet_t MEOS_DISPLAYSETUP_ANISOTROPY = MAKE_MENUOPTIONSET( MEOSN_DISPLAYSETUP_ANISOTROPY, MEOSV_DISPLAYSETUP_ANISOTROPY, 0x0 );
static MenuOption_t MEO_DISPLAYSETUP_ANISOTROPY = MAKE_MENUOPTION(&MF_Redfont, &MEOS_DISPLAYSETUP_ANISOTROPY, &glanisotropy);
static MenuEntry_t ME_DISPLAYSETUP_ANISOTROPY = MAKE_MENUENTRY( "Anisotropy:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_ANISOTROPY, Option );
static char *MEOSN_DISPLAYSETUP_VSYNC[] = { "Adaptive", "Off", "On", };
static int32_t MEOSV_DISPLAYSETUP_VSYNC[] = { -1, 0, 1, };
static MenuOptionSet_t MEOS_DISPLAYSETUP_VSYNC = MAKE_MENUOPTIONSET( MEOSN_DISPLAYSETUP_VSYNC, MEOSV_DISPLAYSETUP_VSYNC, 0x2 );
static MenuOption_t MEO_DISPLAYSETUP_VSYNC = MAKE_MENUOPTION(&MF_Redfont, &MEOS_DISPLAYSETUP_VSYNC, &vsync);
static MenuEntry_t ME_DISPLAYSETUP_VSYNC = MAKE_MENUENTRY( "VSync:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_VSYNC, Option );

#ifdef DROIDMENU
static MenuOption_t MEO_DISPLAYSETUP_HIDEDPAD = MAKE_MENUOPTION(&MF_Redfont, &MEOS_NoYes, &droidinput.hideStick);
static MenuEntry_t ME_DISPLAYSETUP_HIDEDPAD = MAKE_MENUENTRY("Hide touch d-pad:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_HIDEDPAD, Option);

static MenuRangeFloat_t MEO_DISPLAYSETUP_TOUCHALPHA = MAKE_MENURANGE(&droidinput.gameControlsAlpha, &MF_Redfont, 0, 1, 0, 16, 2);
static MenuEntry_t ME_DISPLAYSETUP_TOUCHALPHA = MAKE_MENUENTRY("UI opacity:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_TOUCHALPHA, RangeFloat);
#endif

#endif

static MenuOption_t MEO_SCREENSETUP_CROSSHAIR = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &ud.crosshair);
static MenuEntry_t ME_SCREENSETUP_CROSSHAIR = MAKE_MENUENTRY( "Crosshair:", &MF_Redfont, &MEF_BigOptions, &MEO_SCREENSETUP_CROSSHAIR, Option );
static MenuRangeInt32_t MEO_SCREENSETUP_CROSSHAIRSIZE = MAKE_MENURANGE( &ud.crosshairscale, &MF_Redfont, 25, 100, 0, 16, 2 );
static MenuEntry_t ME_SCREENSETUP_CROSSHAIRSIZE = MAKE_MENUENTRY( "Size:", &MF_Redfont, &MEF_BigOptions, &MEO_SCREENSETUP_CROSSHAIRSIZE, RangeInt32 );

static int32_t vpsize;
static MenuRangeInt32_t MEO_SCREENSETUP_SCREENSIZE = MAKE_MENURANGE( &vpsize, &MF_Redfont, 12, 0, 0, 4, EnforceIntervals );
static MenuEntry_t ME_SCREENSETUP_SCREENSIZE = MAKE_MENUENTRY( "Screen size:", &MF_Redfont, &MEF_BigOptions, &MEO_SCREENSETUP_SCREENSIZE, RangeInt32 );
static MenuRangeInt32_t MEO_SCREENSETUP_TEXTSIZE = MAKE_MENURANGE( &ud.textscale, &MF_Redfont, 100, 400, 0, 16, 2 );
static MenuEntry_t ME_SCREENSETUP_TEXTSIZE = MAKE_MENUENTRY( "Size:", &MF_Redfont, &MEF_BigOptions, &MEO_SCREENSETUP_TEXTSIZE, RangeInt32 );
static MenuOption_t MEO_SCREENSETUP_LEVELSTATS = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &ud.levelstats);
static MenuEntry_t ME_SCREENSETUP_LEVELSTATS = MAKE_MENUENTRY( "Level stats:", &MF_Redfont, &MEF_BigOptions, &MEO_SCREENSETUP_LEVELSTATS, Option );


static MenuOption_t MEO_SCREENSETUP_SHOWPICKUPMESSAGES = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &ud.fta_on);
static MenuEntry_t ME_SCREENSETUP_SHOWPICKUPMESSAGES = MAKE_MENUENTRY( "Game messages:", &MF_Redfont, &MEF_BigOptions, &MEO_SCREENSETUP_SHOWPICKUPMESSAGES, Option );

static char *MEOSN_SCREENSETUP_NEWSTATUSBAR[] = { "Classic", "New",
#ifdef DROIDMENU
"On top",
#endif
};

static int32_t MEOSV_SCREENSETUP_NEWSTATUSBAR[] = { 0, 1,
#ifdef DROIDMENU
2,
#endif
};

static MenuOptionSet_t MEOS_SCREENSETUP_NEWSTATUSBAR = MAKE_MENUOPTIONSET( MEOSN_SCREENSETUP_NEWSTATUSBAR, MEOSV_SCREENSETUP_NEWSTATUSBAR, 0x2 );
static MenuOption_t MEO_SCREENSETUP_NEWSTATUSBAR = MAKE_MENUOPTION(&MF_Redfont, &MEOS_SCREENSETUP_NEWSTATUSBAR, &ud.althud);
static MenuEntry_t ME_SCREENSETUP_NEWSTATUSBAR = MAKE_MENUENTRY( "Status bar:", &MF_Redfont, &MEF_BigOptions, &MEO_SCREENSETUP_NEWSTATUSBAR, Option );



static MenuRangeInt32_t MEO_SCREENSETUP_SBARSIZE = MAKE_MENURANGE( &ud.statusbarscale, &MF_Redfont, 36, 100, 0, 17, 2 );
static MenuEntry_t ME_SCREENSETUP_SBARSIZE = MAKE_MENUENTRY( "Size:", &MF_Redfont, &MEF_BigOptions, &MEO_SCREENSETUP_SBARSIZE, RangeInt32 );


static MenuLink_t MEO_DISPLAYSETUP_SCREENSETUP = { MENU_SCREENSETUP, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_SCREENSETUP = MAKE_MENUENTRY( "Status and crosshair", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_SCREENSETUP, Link );


#ifndef DROIDMENU
#ifdef USE_OPENGL
static MenuLink_t MEO_DISPLAYSETUP_ADVANCED_GL = { MENU_RENDERERSETUP, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_ADVANCED_GL = MAKE_MENUENTRY( "Advanced", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_ADVANCED_GL, Link );
#endif

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
    &ME_OPTIONS_GAMESETUP,
    &ME_OPTIONS_SOUNDSETUP,
    &ME_OPTIONS_DISPLAYSETUP,
#ifndef DROIDMENU
    &ME_OPTIONS_PLAYERSETUP,
    &ME_OPTIONS_CONTROLS,
#else
    &ME_OPTIONS_TOUCHSETUP,
#endif
    &ME_OPTIONS_CHEATS
};

static MenuEntry_t *MEL_CONTROLS[] = {
#ifndef DROIDMENU
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
    &ME_Space6,
    &ME_VIDEOSETUP_APPLY,
};
static MenuEntry_t *MEL_DISPLAYSETUP[] = {
    &ME_DISPLAYSETUP_SCREENSETUP,
    &ME_DISPLAYSETUP_COLORCORR,
#ifndef DROIDMENU
    &ME_DISPLAYSETUP_VIDEOSETUP,
#endif
    &ME_DISPLAYSETUP_ASPECTRATIO,
    &ME_DISPLAYSETUP_PIXELDOUBLING,
};

#ifdef USE_OPENGL
static MenuEntry_t *MEL_DISPLAYSETUP_GL[] = {
    &ME_DISPLAYSETUP_SCREENSETUP,
    &ME_DISPLAYSETUP_COLORCORR,
#ifndef DROIDMENU
    &ME_DISPLAYSETUP_VIDEOSETUP,
#endif
    &ME_DISPLAYSETUP_ASPECTRATIO,
    &ME_DISPLAYSETUP_TEXFILTER,
#ifdef DROIDMENU
    &ME_DISPLAYSETUP_HIDEDPAD,
    &ME_DISPLAYSETUP_TOUCHALPHA,
#endif
#ifndef DROIDMENU
    &ME_DISPLAYSETUP_ANISOTROPY,
    &ME_DISPLAYSETUP_VSYNC,
    &ME_DISPLAYSETUP_ADVANCED_GL,
#endif
};

#ifdef POLYMER
static MenuEntry_t *MEL_DISPLAYSETUP_GL_POLYMER[] = {
    &ME_DISPLAYSETUP_SCREENSETUP,
    &ME_DISPLAYSETUP_COLORCORR,
#ifndef DROIDMENU
    &ME_DISPLAYSETUP_VIDEOSETUP,
#endif
    &ME_DISPLAYSETUP_ASPECTRATIO_POLYMER,
    &ME_DISPLAYSETUP_TEXFILTER,
#ifndef DROIDMENU
    &ME_DISPLAYSETUP_ANISOTROPY,
    &ME_DISPLAYSETUP_VSYNC,
    &ME_DISPLAYSETUP_ADVANCED_GL,
#endif
};

#endif
#endif



static char *MenuKeyNone = "  -";
static char *MEOSN_Keys[NUMKEYS];

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

static char *MenuMouseNames[MENUMOUSEFUNCTIONS] = {
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
static MenuOption_t MEO_MOUSESETUP_MOUSEAIMING = MAKE_MENUOPTION( &MF_Redfont, &MEOS_NoYes, &g_myAimMode );
static MenuEntry_t ME_MOUSESETUP_MOUSEAIMING = MAKE_MENUENTRY( "Vertical aiming:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_MOUSEAIMING, Option );
static MenuOption_t MEO_MOUSESETUP_INVERT = MAKE_MENUOPTION( &MF_Redfont, &MEOS_YesNo, &ud.mouseflip );
static MenuEntry_t ME_MOUSESETUP_INVERT = MAKE_MENUENTRY( "Invert aiming:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_INVERT, Option );
static MenuOption_t MEO_MOUSESETUP_SMOOTH = MAKE_MENUOPTION( &MF_Redfont, &MEOS_NoYes, &ud.config.SmoothInput );
static MenuEntry_t ME_MOUSESETUP_SMOOTH = MAKE_MENUENTRY( "Filter input:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_SMOOTH, Option );
static MenuLink_t MEO_MOUSESETUP_ADVANCED = { MENU_MOUSEADVANCED, MA_Advance, };
static MenuEntry_t ME_MOUSESETUP_ADVANCED = MAKE_MENUENTRY( "Advanced setup", &MF_Redfont, &MEF_BigOptionsRt, &MEO_MOUSESETUP_ADVANCED, Link );

static MenuEntry_t *MEL_MOUSESETUP[] = {
    &ME_MOUSESETUP_SENSITIVITY,
    &ME_MOUSESETUP_BTNS,
    &ME_Space2,
    &ME_MOUSESETUP_MOUSEAIMING,
    &ME_MOUSESETUP_INVERT,
    &ME_MOUSESETUP_SMOOTH,
    &ME_MOUSESETUP_ADVANCED,
};

#ifdef DROIDMENU
static MenuRangeFloat_t MEO_TOUCHSETUP_SENSITIVITY_MOVE = MAKE_MENURANGE(&droidinput.forward_sens, &MF_Redfont, 1.f, 9.f, 0.f, 17, 1 + EnforceIntervals);
static MenuEntry_t ME_TOUCHSETUP_SENSITIVITY_MOVE = MAKE_MENUENTRY("Run sens:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_TOUCHSETUP_SENSITIVITY_MOVE, RangeFloat);

static MenuRangeFloat_t MEO_TOUCHSETUP_SENSITIVITY_STRAFE = MAKE_MENURANGE(&droidinput.strafe_sens, &MF_Redfont, 1.f, 9.f, 0.f, 17, 1 + EnforceIntervals);
static MenuEntry_t ME_TOUCHSETUP_SENSITIVITY_STRAFE = MAKE_MENUENTRY("Strafe sens:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_TOUCHSETUP_SENSITIVITY_STRAFE, RangeFloat);

static MenuRangeFloat_t MEO_TOUCHSETUP_SENSITIVITY_LOOK = MAKE_MENURANGE(&droidinput.pitch_sens, &MF_Redfont, 1.f, 9.f, 0.f, 17, 1 + EnforceIntervals);
static MenuEntry_t ME_TOUCHSETUP_SENSITIVITY_LOOK = MAKE_MENUENTRY("Look sens:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_TOUCHSETUP_SENSITIVITY_LOOK, RangeFloat);

static MenuRangeFloat_t MEO_TOUCHSETUP_SENSITIVITY_TURN = MAKE_MENURANGE(&droidinput.yaw_sens, &MF_Redfont, 1.f, 9.f, 0.f, 17, 1 + EnforceIntervals);
static MenuEntry_t ME_TOUCHSETUP_SENSITIVITY_TURN = MAKE_MENUENTRY("Turn sens:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_TOUCHSETUP_SENSITIVITY_TURN, RangeFloat);

static MenuOption_t MEO_TOUCHSETUP_INVERT = MAKE_MENUOPTION(&MF_Redfont, &MEOS_NoYes, &droidinput.invertLook);
static MenuEntry_t ME_TOUCHSETUP_INVERT = MAKE_MENUENTRY("Invert look:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_TOUCHSETUP_INVERT, Option);

static MenuEntry_t *MEL_TOUCHSETUP [] ={
    &ME_TOUCHSETUP_SENSITIVITY_MOVE,
    &ME_TOUCHSETUP_SENSITIVITY_STRAFE,
    &ME_TOUCHSETUP_SENSITIVITY_LOOK,
    &ME_TOUCHSETUP_SENSITIVITY_TURN,
    &ME_Space2,
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

static MenuRangeInt32_t MEO_MOUSEADVANCED_SCALEX = MAKE_MENURANGE( &ud.config.MouseAnalogueScale[0], &MF_Bluefont, -262144, 262144, 65536, 65, 3 );
static MenuEntry_t ME_MOUSEADVANCED_SCALEX = MAKE_MENUENTRY( "X-Axis Scale", &MF_Redfont, &MEF_BigSliders, &MEO_MOUSEADVANCED_SCALEX, RangeInt32 );
static MenuRangeInt32_t MEO_MOUSEADVANCED_SCALEY = MAKE_MENURANGE( &ud.config.MouseAnalogueScale[1], &MF_Bluefont, -262144, 262144, 65536, 65, 3 );
static MenuEntry_t ME_MOUSEADVANCED_SCALEY = MAKE_MENUENTRY( "Y-Axis Scale", &MF_Redfont, &MEF_BigSliders, &MEO_MOUSEADVANCED_SCALEY, RangeInt32 );

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
    &ME_Space8,
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

static char *MEOSN_JOYSTICKAXIS_ANALOG[] = { "  -None-", "Turning", "Strafing", "Looking", "Moving", };
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
    &ME_Space8,
    &ME_JOYSTICKAXIS_DIGITALNEGATIVE,
    &ME_JOYSTICKAXIS_DIGITALPOSITIVE,
};

static MenuEntry_t *MEL_INTERNAL_JOYSTICKAXIS_DIGITAL[] = {
    &ME_JOYSTICKAXIS_DIGITALNEGATIVE,
    &ME_JOYSTICKAXIS_DIGITALPOSITIVE,
};

#ifdef USE_OPENGL
static MenuOption_t MEO_RENDERERSETUP_HIGHTILE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &usehightile );
static MenuEntry_t ME_RENDERERSETUP_HIGHTILE = MAKE_MENUENTRY( "Hires textures:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_RENDERERSETUP_HIGHTILE, Option );
static MenuRangeInt32_t MEO_RENDERERSETUP_TEXQUALITY = MAKE_MENURANGE( &r_downsize, &MF_Bluefont, 2, 0, 0, 3, 0 );
static MenuEntry_t ME_RENDERERSETUP_TEXQUALITY = MAKE_MENUENTRY( "Hires texture quality", &MF_BluefontRed, &MEF_SmallOptions, &MEO_RENDERERSETUP_TEXQUALITY, RangeInt32 );
static MenuOption_t MEO_RENDERERSETUP_PRECACHE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.config.useprecache );
static MenuEntry_t ME_RENDERERSETUP_PRECACHE = MAKE_MENUENTRY( "Pre-load map textures", &MF_BluefontRed, &MEF_SmallOptions, &MEO_RENDERERSETUP_PRECACHE, Option );
static char *MEOSN_RENDERERSETUP_TEXCACHE[] = { "Off", "On", "Compress", };
static MenuOptionSet_t MEOS_RENDERERSETUP_TEXCACHE = MAKE_MENUOPTIONSET( MEOSN_RENDERERSETUP_TEXCACHE, NULL, 0x2 );
static MenuOption_t MEO_RENDERERSETUP_TEXCACHE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_RENDERERSETUP_TEXCACHE, &glusetexcache );
static MenuEntry_t ME_RENDERERSETUP_TEXCACHE = MAKE_MENUENTRY( "On disk texture cache", &MF_BluefontRed, &MEF_SmallOptions, &MEO_RENDERERSETUP_TEXCACHE, Option );
#ifndef EDUKE32_GLES
static MenuOption_t MEO_RENDERERSETUP_DETAILTEX = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &r_detailmapping );
static MenuEntry_t ME_RENDERERSETUP_DETAILTEX = MAKE_MENUENTRY( "Detail textures:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_RENDERERSETUP_DETAILTEX, Option );
#endif
static MenuOption_t MEO_RENDERERSETUP_MODELS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &usemodels );
static MenuEntry_t ME_RENDERERSETUP_MODELS = MAKE_MENUENTRY( "Models:", &MF_BluefontRed, &MEF_SmallOptions, &MEO_RENDERERSETUP_MODELS, Option );
#endif

#ifdef USE_OPENGL
static MenuEntry_t *MEL_RENDERERSETUP[] = {
    &ME_RENDERERSETUP_HIGHTILE,
    &ME_RENDERERSETUP_TEXQUALITY,
    &ME_RENDERERSETUP_PRECACHE,
#ifndef EDUKE32_GLES
    &ME_RENDERERSETUP_TEXCACHE,
    &ME_RENDERERSETUP_DETAILTEX,
#endif
    &ME_Space4,
    &ME_RENDERERSETUP_MODELS,
};
#endif

#ifdef DROIDMENU
static MenuRangeFloat_t MEO_COLCORR_GAMMA = MAKE_MENURANGE( &vid_gamma, &MF_Bluefont, 1.f, 2.5f, 0.f, 39, 1 );
#else
static MenuRangeFloat_t MEO_COLCORR_GAMMA = MAKE_MENURANGE( &vid_gamma, &MF_Bluefont, 0.2f, 4.f, 0.f, 39, 1 );
#endif
static MenuEntry_t ME_COLCORR_GAMMA = MAKE_MENUENTRY( "Gamma:", &MF_Redfont, &MEF_ColorCorrect, &MEO_COLCORR_GAMMA, RangeFloat );
static MenuRangeFloat_t MEO_COLCORR_CONTRAST = MAKE_MENURANGE( &vid_contrast, &MF_Bluefont, 0.1f, 2.7f, 0.f, 53, 1 );
static MenuEntry_t ME_COLCORR_CONTRAST = MAKE_MENUENTRY( "Contrast:", &MF_Redfont, &MEF_ColorCorrect, &MEO_COLCORR_CONTRAST, RangeFloat );
static MenuRangeFloat_t MEO_COLCORR_BRIGHTNESS = MAKE_MENURANGE( &vid_brightness, &MF_Bluefont, -0.8f, 0.8f, 0.f, 33, 1 );
static MenuEntry_t ME_COLCORR_BRIGHTNESS = MAKE_MENUENTRY( "Brightness:", &MF_Redfont, &MEF_ColorCorrect, &MEO_COLCORR_BRIGHTNESS, RangeFloat );
static MenuEntry_t ME_COLCORR_RESET = MAKE_MENUENTRY( "Reset To Defaults", &MF_Redfont, &MEF_ColorCorrect, &MEO_NULL, Link );
#ifdef DROIDMENU
#define MINVIS 1.f
#else
#define MINVIS 0.125f
#endif
static MenuRangeFloat_t MEO_COLCORR_AMBIENT = MAKE_MENURANGE( &r_ambientlight, &MF_Bluefont, MINVIS, 4.f, 0.f, 32, 1 );
static MenuEntry_t ME_COLCORR_AMBIENT = MAKE_MENUENTRY( "Visibility:", &MF_Redfont, &MEF_ColorCorrect, &MEO_COLCORR_AMBIENT, RangeFloat );

static MenuEntry_t *MEL_COLCORR[] = {
    &ME_COLCORR_GAMMA,
#ifndef DROIDMENU
    &ME_COLCORR_CONTRAST,
    &ME_COLCORR_BRIGHTNESS,
#endif
    &ME_COLCORR_AMBIENT,
    &ME_Space8,
    &ME_COLCORR_RESET,
};

static MenuEntry_t *MEL_SCREENSETUP[] = {
#ifndef DROIDMENU
    &ME_SCREENSETUP_SCREENSIZE,
#endif

    &ME_SCREENSETUP_NEWSTATUSBAR,
    &ME_SCREENSETUP_SBARSIZE,

    &ME_SCREENSETUP_CROSSHAIR,
    &ME_SCREENSETUP_CROSSHAIRSIZE,

    &ME_SCREENSETUP_LEVELSTATS,
    &ME_SCREENSETUP_TEXTSIZE,

    &ME_SCREENSETUP_SHOWPICKUPMESSAGES,
};

// Save and load will be filled in before every viewing of the save/load screen.
static MenuLink_t MEO_LOAD = { MENU_NULL, MA_None, };
static MenuLink_t MEO_LOAD_VALID = { MENU_LOADVERIFY, MA_None, };
static MenuEntry_t ME_LOAD_TEMPLATE = MAKE_MENUENTRY( NULL, &MF_MinifontRed, &MEF_LoadSave, &MEO_LOAD, Link );
static MenuEntry_t ME_LOAD[MAXSAVEGAMES];
static MenuEntry_t *MEL_LOAD[MAXSAVEGAMES];

static MenuString_t MEO_SAVE_TEMPLATE = MAKE_MENUSTRING( NULL, &MF_MinifontRed, MAXSAVEGAMENAME, 0 );
static MenuString_t MEO_SAVE[MAXSAVEGAMES];
static MenuEntry_t ME_SAVE_TEMPLATE = MAKE_MENUENTRY( NULL, &MF_MinifontRed, &MEF_LoadSave, &MEO_SAVE_TEMPLATE, String );
static MenuEntry_t ME_SAVE[MAXSAVEGAMES];
static MenuEntry_t *MEL_SAVE[MAXSAVEGAMES];

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
static MenuEntry_t ME_SOUND_DUKETALK = MAKE_MENUENTRY( "Duke talk:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_DUKETALK, Option );

static char *MEOSN_SOUND_SAMPLINGRATE[] = { "22050Hz", "44100Hz", "48000Hz", };
static int32_t MEOSV_SOUND_SAMPLINGRATE[] = { 22050, 44100, 48000, };
static MenuOptionSet_t MEOS_SOUND_SAMPLINGRATE = MAKE_MENUOPTIONSET( MEOSN_SOUND_SAMPLINGRATE, MEOSV_SOUND_SAMPLINGRATE, 0x3 );
static MenuOption_t MEO_SOUND_SAMPLINGRATE = MAKE_MENUOPTION( &MF_Redfont, &MEOS_SOUND_SAMPLINGRATE, &soundrate );
static MenuEntry_t ME_SOUND_SAMPLINGRATE = MAKE_MENUENTRY( "Sample rate:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_SAMPLINGRATE, Option );

static MenuRangeInt32_t MEO_SOUND_NUMVOICES = MAKE_MENURANGE( &soundvoices, &MF_Redfont, 16, 256, 0, 16, 1 );
static MenuEntry_t ME_SOUND_NUMVOICES = MAKE_MENUENTRY( "Voices:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_NUMVOICES, RangeInt32 );

static MenuEntry_t ME_SOUND_RESTART = MAKE_MENUENTRY( "Restart sound system", &MF_Redfont, &MEF_BigOptionsRt, &MEO_NULL, Link );

#ifndef DROIDMENU
static MenuLink_t MEO_ADVSOUND = { MENU_ADVSOUND, MA_Advance, };
static MenuEntry_t ME_SOUND_ADVSOUND = MAKE_MENUENTRY( "Advanced", &MF_Redfont, &MEF_BigOptionsRt, &MEO_ADVSOUND, Link );
#endif

static MenuEntry_t *MEL_SOUND[] = {
    &ME_SOUND,
    &ME_SOUND_MUSIC,
    &ME_Space2,
    &ME_SOUND_VOLUME_MASTER,
    &ME_SOUND_VOLUME_EFFECTS,
    &ME_SOUND_VOLUME_MUSIC,
    &ME_Space2,
    &ME_SOUND_DUKETALK,
#ifndef DROIDMENU
    &ME_SOUND_ADVSOUND,
#endif
};

static MenuEntry_t *MEL_ADVSOUND[] = {
    &ME_SOUND_SAMPLINGRATE,
    &ME_Space2,
    &ME_SOUND_NUMVOICES,
    &ME_Space2,
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
static char *MEOSN_PLAYER_COLOR[] = { "Auto", "Blue", "Red", "Green", "Gray", "Dark gray", "Dark green", "Brown", "Dark blue", "Bright red", "Yellow", };
static int32_t MEOSV_PLAYER_COLOR[] = { 0, 9, 10, 11, 12, 13, 14, 15, 16, 21, 23, };
static MenuOptionSet_t MEOS_PLAYER_COLOR = MAKE_MENUOPTIONSET( MEOSN_PLAYER_COLOR, MEOSV_PLAYER_COLOR, 0x2 );
static MenuOption_t MEO_PLAYER_COLOR = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_PLAYER_COLOR, &ud.color );
static MenuEntry_t ME_PLAYER_COLOR = MAKE_MENUENTRY( "Color", &MF_BluefontRed, &MEF_PlayerNarrow, &MEO_PLAYER_COLOR, Option );
static char *MEOSN_PLAYER_TEAM[] = { "Blue", "Red", "Green", "Gray", };
static MenuOptionSet_t MEOS_PLAYER_TEAM = MAKE_MENUOPTIONSET( MEOSN_PLAYER_TEAM, NULL, 0x2 );
static MenuOption_t MEO_PLAYER_TEAM = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_PLAYER_TEAM, &ud.team );
static MenuEntry_t ME_PLAYER_TEAM = MAKE_MENUENTRY( "Team", &MF_BluefontRed, &MEF_PlayerNarrow, &MEO_PLAYER_TEAM, Option );
#ifndef DROIDMENU
static MenuLink_t MEO_PLAYER_MACROS = { MENU_MACROS, MA_Advance, };
static MenuEntry_t ME_PLAYER_MACROS = MAKE_MENUENTRY( "Multiplayer macros", &MF_BluefontRed, &MEF_SmallOptions, &MEO_PLAYER_MACROS, Link );
#endif

static MenuEntry_t *MEL_PLAYER[] = {
    &ME_PLAYER_NAME,
    &ME_Space4,
    &ME_PLAYER_COLOR,
    &ME_Space4,
    &ME_PLAYER_TEAM,
#ifndef DROIDMENU
    &ME_Space8,
    &ME_PLAYER_MACROS,
#endif
};

static MenuString_t MEO_MACROS_TEMPLATE = MAKE_MENUSTRING( NULL, &MF_Bluefont, MAXRIDECULELENGTH, 0 );
static MenuString_t MEO_MACROS[MAXSAVEGAMES];
static MenuEntry_t ME_MACROS_TEMPLATE = MAKE_MENUENTRY( NULL, &MF_Bluefont, &MEF_Macros, &MEO_MACROS_TEMPLATE, String );
static MenuEntry_t ME_MACROS[MAXRIDECULE];
static MenuEntry_t *MEL_MACROS[MAXRIDECULE];

static char *MenuUserMap = "User Map";
static char *MenuSkillNone = "None";

static char *MEOSN_NetGametypes[MAXGAMETYPES];
static char *MEOSN_NetEpisodes[MAXVOLUMES+1];
static int32_t MEOSV_NetEpisodes[MAXVOLUMES+1];
static char *MEOSN_NetLevels[MAXVOLUMES][MAXLEVELS];
static char *MEOSN_NetSkills[MAXSKILLS+1];

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
static MenuEntry_t ME_NETOPTIONS_MARKERS_DISABLED = MAKE_MENUENTRY( "Markers", &MF_RedfontBlue, &MEF_NetSetup, NULL, Dummy );
static MenuOption_t MEO_NETOPTIONS_MAPEXITS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OnOff, &ud.m_noexits );
static MenuEntry_t ME_NETOPTIONS_MAPEXITS = MAKE_MENUENTRY( "Map Exits", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_MAPEXITS, Option );
static MenuOption_t MEO_NETOPTIONS_FRFIRE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.m_ffire );
static MenuEntry_t ME_NETOPTIONS_FRFIRE = MAKE_MENUENTRY( "Fr. Fire", &MF_Redfont, &MEF_NetSetup, &MEO_NETOPTIONS_FRFIRE, Option );
static MenuEntry_t ME_NETOPTIONS_ACCEPT = MAKE_MENUENTRY( "Accept", &MF_RedfontGreen, &MEF_NetSetup, &MEO_NULL, Link );

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
static MenuEntry_t ME_NETJOIN_CONNECT = MAKE_MENUENTRY( "Connect", &MF_RedfontGreen, &MEF_VideoSetup, &MEO_NULL, Link );

static MenuEntry_t *MEL_NETJOIN[] = {
    &ME_NETJOIN_SERVER,
    &ME_NETJOIN_PORT,
    &ME_NETJOIN_CONNECT,
};


#define NoTitle NULL

#define MAKE_MENUMENU(Title, Format, Entries) { Title, Format, Entries, ARRAY_SIZE(Entries), 0, 0, 0, 0 }

static MenuMenu_t M_MAIN = MAKE_MENUMENU( NoTitle, &MMF_Top_Main, MEL_MAIN );
static MenuMenu_t M_MAIN_INGAME = MAKE_MENUMENU( NoTitle, &MMF_Top_Main, MEL_MAIN_INGAME );
static MenuMenu_t M_EPISODE = MAKE_MENUMENU( "Select An Episode", &MMF_Top_Episode, MEL_EPISODE );
static MenuMenu_t M_SKILL = MAKE_MENUMENU( "Select Skill", &MMF_Top_Skill, MEL_SKILL );
static MenuMenu_t M_GAMESETUP = MAKE_MENUMENU( "Game Setup", &MMF_BigOptions, MEL_GAMESETUP );
static MenuMenu_t M_OPTIONS = MAKE_MENUMENU( "Options", &MMF_Top_Options, MEL_OPTIONS );
static MenuMenu_t M_VIDEOSETUP = MAKE_MENUMENU( "Video Mode", &MMF_BigOptions, MEL_VIDEOSETUP );
static MenuMenu_t M_KEYBOARDSETUP = MAKE_MENUMENU( "Keyboard Setup", &MMF_Top_Options, MEL_KEYBOARDSETUP );
static MenuMenu_t M_CONTROLS = MAKE_MENUMENU( "Control Setup", &MMF_Top_Options, MEL_CONTROLS );
static MenuMenu_t M_CHEATS = MAKE_MENUMENU( "Cheats", &MMF_SmallOptions, MEL_CHEATS );
static MenuMenu_t M_MOUSESETUP = MAKE_MENUMENU( "Mouse Setup", &MMF_BigOptions, MEL_MOUSESETUP );
#ifdef DROIDMENU
static MenuMenu_t M_TOUCHSETUP = MAKE_MENUMENU( "Touch Setup", &MMF_BigOptions, MEL_TOUCHSETUP );
#endif
static MenuMenu_t M_JOYSTICKSETUP = MAKE_MENUMENU( "Joystick Setup", &MMF_Top_Joystick_Network, MEL_JOYSTICKSETUP );
static MenuMenu_t M_JOYSTICKBTNS = MAKE_MENUMENU( "Joystick Buttons", &MMF_MouseJoySetupBtns, MEL_JOYSTICKBTNS );
static MenuMenu_t M_JOYSTICKAXES = MAKE_MENUMENU( "Joystick Axes", &MMF_BigSliders, MEL_JOYSTICKAXES );
static MenuMenu_t M_KEYBOARDKEYS = MAKE_MENUMENU( "Keyboard Keys", &MMF_KeyboardSetupFuncs, MEL_KEYBOARDSETUPFUNCS );
static MenuMenu_t M_MOUSEBTNS = MAKE_MENUMENU( "Mouse Buttons", &MMF_MouseJoySetupBtns, MEL_MOUSESETUPBTNS );
static MenuMenu_t M_MOUSEADVANCED = MAKE_MENUMENU( "Advanced Mouse", &MMF_BigSliders, MEL_MOUSEADVANCED );
static MenuMenu_t M_JOYSTICKAXIS = MAKE_MENUMENU( NULL, &MMF_BigSliders, MEL_JOYSTICKAXIS );
#ifdef USE_OPENGL
static MenuMenu_t M_RENDERERSETUP = MAKE_MENUMENU( "Advanced Video", &MMF_SmallOptions, MEL_RENDERERSETUP );
#endif
static MenuMenu_t M_COLCORR = MAKE_MENUMENU( "Color Correction", &MMF_ColorCorrect, MEL_COLCORR );
static MenuMenu_t M_SCREENSETUP = MAKE_MENUMENU( "Status and crosshair", &MMF_BigOptions, MEL_SCREENSETUP );
static MenuMenu_t M_DISPLAYSETUP = MAKE_MENUMENU( "Display Setup", &MMF_BigOptions, MEL_DISPLAYSETUP );
static MenuMenu_t M_LOAD = MAKE_MENUMENU( "Load Game", &MMF_LoadSave, MEL_LOAD );
static MenuMenu_t M_SAVE = MAKE_MENUMENU( "Save Game", &MMF_LoadSave, MEL_SAVE );
static MenuMenu_t M_SOUND = MAKE_MENUMENU( "Sound Setup", &MMF_BigOptions, MEL_SOUND );
static MenuMenu_t M_ADVSOUND = MAKE_MENUMENU( "Advanced Sound", &MMF_BigOptions, MEL_ADVSOUND );
static MenuMenu_t M_NETWORK = MAKE_MENUMENU( "Network Game", &MMF_Top_Joystick_Network, MEL_NETWORK );
static MenuMenu_t M_PLAYER = MAKE_MENUMENU( "Player Setup", &MMF_SmallOptions, MEL_PLAYER );
static MenuMenu_t M_MACROS = MAKE_MENUMENU( "Multiplayer Macros", &MMF_Macros, MEL_MACROS );
static MenuMenu_t M_NETHOST = MAKE_MENUMENU( "Host Network Game", &MMF_SmallOptionsNarrow, MEL_NETHOST );
static MenuMenu_t M_NETOPTIONS = MAKE_MENUMENU( "Net Game Options", &MMF_NetSetup, MEL_NETOPTIONS );
static MenuMenu_t M_NETJOIN = MAKE_MENUMENU( "Join Network Game", &MMF_SmallOptionsNarrow, MEL_NETJOIN );

#ifdef DROIDMENU
static MenuPanel_t M_STORY = { NoTitle, MENU_STORY, MA_Return, MENU_STORY, MA_Advance, };
#else
static MenuPanel_t M_STORY = { NoTitle, MENU_F1HELP, MA_Return, MENU_F1HELP, MA_Advance, };
#endif

static MenuPanel_t M_F1HELP = { NoTitle, MENU_STORY, MA_Return, MENU_STORY, MA_Advance, };
static const char* MenuCredits = "Credits";
static MenuPanel_t M_CREDITS = { NoTitle, MENU_CREDITS5, MA_Return, MENU_CREDITS2, MA_Advance, };
static MenuPanel_t M_CREDITS2 = { NoTitle, MENU_CREDITS, MA_Return, MENU_CREDITS3, MA_Advance, };
static MenuPanel_t M_CREDITS3 = { NoTitle, MENU_CREDITS2, MA_Return, MENU_CREDITS4, MA_Advance, };
#ifdef DROIDMENU
static MenuPanel_t M_CREDITS4 = { "Hail to the King", MENU_CREDITS3, MA_Return, MENU_CREDITS5, MA_Advance, };
#else
static MenuPanel_t M_CREDITS4 = { "About EDuke32", MENU_CREDITS3, MA_Return, MENU_CREDITS5, MA_Advance, };
#endif
static MenuPanel_t M_CREDITS5 = { "About EDuke32", MENU_CREDITS4, MA_Return, MENU_CREDITS, MA_Advance, };

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
    { &M_GAMESETUP, MENU_GAMESETUP, MENU_OPTIONS, MA_Return, Menu },
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
#ifdef DROIDMENU
    { &M_TOUCHSETUP, MENU_TOUCHSETUP, MENU_OPTIONS, MA_Return, Menu },
#endif
    { &M_CONTROLS, MENU_CONTROLS, MENU_OPTIONS, MA_Return, Menu },
#ifdef USE_OPENGL
    { &M_RENDERERSETUP, MENU_RENDERERSETUP, MENU_DISPLAYSETUP, MA_Return, Menu },
#endif
    { &M_COLCORR, MENU_COLCORR, MENU_DISPLAYSETUP, MA_Return, Menu },
    { &M_COLCORR, MENU_COLCORR_INGAME, MENU_CLOSE, MA_Return, Menu },
    { &M_SCREENSETUP, MENU_SCREENSETUP, MENU_DISPLAYSETUP, MA_Return, Menu },
    { &M_DISPLAYSETUP, MENU_DISPLAYSETUP, MENU_OPTIONS, MA_Return, Menu },
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
    { &M_CHEATS, MENU_CHEATS, MENU_OPTIONS, MA_Return, Menu },
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

static const size_t numMenus = ARRAY_SIZE(Menus);

Menu_t *m_currentMenu = &Menus[0];
static Menu_t *m_previousMenu = &Menus[0];

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
        entry->flags |= Disabled;
    else
        entry->flags &= ~Disabled;
}

/*
This function prepares data after ART and CON have been processed.
It also initializes some data in loops rather than statically at compile time.
*/
void M_Init(void)
{
    int32_t i, j, k;

    // prepare menu fonts
    MF_Redfont.tilenum = MF_RedfontBlue.tilenum = MF_RedfontGreen.tilenum = BIGALPHANUM;
    MF_Bluefont.tilenum = MF_BluefontRed.tilenum = STARTALPHANUM;
    MF_Minifont.tilenum = MF_MinifontRed.tilenum = MF_MinifontDarkGray.tilenum = MINIFONT;
    if (!minitext_lowercase)
    {
        MF_Minifont.textflags |= TEXT_UPPERCASE;
        MF_MinifontRed.textflags |= TEXT_UPPERCASE;
        MF_MinifontDarkGray.textflags |= TEXT_UPPERCASE;
    }

    // prepare gamefuncs and keys
    for (i = 0; i < NUMGAMEFUNCTIONS; ++i)
    {
        Bstrcpy(MenuGameFuncs[i], gamefunctions[i]);

        for (j = 0; j < MAXGAMEFUNCLEN; ++j)
            if (MenuGameFuncs[i][j] == '_')
                MenuGameFuncs[i][j] = ' ';

        MEOSN_Gamefuncs[i] = MenuGameFuncs[i];
    }
    MEOSN_Gamefuncs[NUMGAMEFUNCTIONS] = MenuGameFuncNone;
    for (i = 0; i < NUMKEYS; ++i)
        MEOSN_Keys[i] = key_names[i];
    MEOSN_Keys[NUMKEYS-1] = MenuKeyNone;


    // prepare episodes
    k = 0;
    for (i = 0; i < g_numVolumes; ++i)
    {
        if (EpisodeNames[i][0])
        {
            if (!(EpisodeFlags[i] & EF_HIDEFROMSP))
            {
                MEL_EPISODE[i] = &ME_EPISODE[i];
                ME_EPISODE[i] = ME_EPISODE_TEMPLATE;
                ME_EPISODE[i].name = EpisodeNames[i];
            }

            // if (!(EpisodeFlags[i] & EF_HIDEFROMMP))
            {
                MEOSN_NetEpisodes[k] = EpisodeNames[i];
                MEOSV_NetEpisodes[k] = i;

                k++;
            }
        }

        // prepare levels
        MEOS_NETOPTIONS_LEVEL[i] = MEOS_NETOPTIONS_LEVEL_TEMPLATE;
        for (j = 0; j < MAXLEVELS; ++j)
        {
            MEOSN_NetLevels[i][j] = MapInfo[MAXLEVELS*i+j].name;
            if (MapInfo[i*MAXLEVELS+j].filename != NULL)
                MEOS_NETOPTIONS_LEVEL[i].numOptions = j+1;
        }
        MEOS_NETOPTIONS_LEVEL[i].optionNames = MEOSN_NetLevels[i];
    }
    M_EPISODE.numEntries = g_numVolumes+2;
    MEL_EPISODE[g_numVolumes] = &ME_Space4;
    MEL_EPISODE[g_numVolumes+1] = &ME_EPISODE_USERMAP;
    MEOS_NETOPTIONS_EPISODE.numOptions = k + 1;
    MEOSN_NetEpisodes[k] = MenuUserMap;
    MEOSV_NetEpisodes[k] = MAXVOLUMES;
    NetEpisode = MEOSV_NetEpisodes[0];
    MMF_Top_Episode.pos.y = (48-(g_numVolumes*2))<<16;
    if (g_numSkills == 0)
        MEO_EPISODE.linkID = MENU_NULL;

    // prepare skills
    k = -1;
    for (i = 0; i < g_numSkills && SkillNames[i][0]; ++i)
    {
        MEL_SKILL[i] = &ME_SKILL[i];
        ME_SKILL[i] = ME_SKILL_TEMPLATE;
        ME_SKILL[i].name = SkillNames[i];

        MEOSN_NetSkills[i] = SkillNames[i];

        k = i;
    }
    ++k;
    M_SKILL.numEntries = g_numSkills; // k;
    MEOS_NETOPTIONS_MONSTERS.numOptions = g_numSkills + 1; // k+1;
    MEOSN_NetSkills[g_numSkills] = MenuSkillNone;
    MMF_Top_Skill.pos.y = (58 + (4-g_numSkills)*6)<<16;
    M_SKILL.currentEntry = 1;

    // prepare multiplayer gametypes
    k = -1;
    for (i = 0; i < MAXGAMETYPES; ++i)
        if (GametypeNames[i][0])
        {
            MEOSN_NetGametypes[i] = GametypeNames[i];
            k = i;
        }
    ++k;
    MEOS_NETOPTIONS_GAMETYPE.numOptions = k;

    // prepare savegames
    for (i = 0; i < MAXSAVEGAMES; ++i)
    {
        MEL_LOAD[i] = &ME_LOAD[i];
        MEL_SAVE[i] = &ME_SAVE[i];
        ME_LOAD[i] = ME_LOAD_TEMPLATE;
        ME_SAVE[i] = ME_SAVE_TEMPLATE;
        ME_SAVE[i].entry = &MEO_SAVE[i];
        MEO_SAVE[i] = MEO_SAVE_TEMPLATE;

        ME_LOAD[i].name = ud.savegame[i];
        MEO_SAVE[i].variable = ud.savegame[i];
    }

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
        MEL_KEYBOARDSETUPFUNCS[i] = &ME_KEYBOARDSETUPFUNCS[i];
        ME_KEYBOARDSETUPFUNCS[i] = ME_KEYBOARDSETUPFUNCS_TEMPLATE;
        ME_KEYBOARDSETUPFUNCS[i].name = MenuGameFuncs[i];
        ME_KEYBOARDSETUPFUNCS[i].entry = &MEO_KEYBOARDSETUPFUNCS[i];
        MEO_KEYBOARDSETUPFUNCS[i] = MEO_KEYBOARDSETUPFUNCS_TEMPLATE;
        MEO_KEYBOARDSETUPFUNCS[i].column[0] = &ud.config.KeyboardKeys[i][0];
        MEO_KEYBOARDSETUPFUNCS[i].column[1] = &ud.config.KeyboardKeys[i][1];
    }
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

    // prepare shareware
    if (VOLUMEONE)
    {
        // blue out episodes beyond the first
        for (i = 1; i < g_numVolumes; ++i)
        {
            if (MEL_EPISODE[i])
            {
                ME_EPISODE[i].font = &MF_RedfontBlue;
                ME_EPISODE[i].entry = &MEO_EPISODE_SHAREWARE;
            }
        }
        M_EPISODE.numEntries = g_numVolumes; // remove User Map (and spacer)
        MEOS_NETOPTIONS_EPISODE.numOptions = 1;
        MenuEntry_DisableOnCondition(&ME_NETOPTIONS_EPISODE, 1);
    }

    // prepare pre-Atomic
    if (!VOLUMEALL || !PLUTOPAK)
    {
        // prepare credits
        M_CREDITS.title = M_CREDITS2.title = M_CREDITS3.title = MenuCredits;
    }
}

static void M_RunMenu(Menu_t *cm, vec2_t origin);



/*
At present, no true difference is planned between M_PreMenu() and M_PreMenuDraw().
They are separate for purposes of organization.
*/

static void M_PreMenu(MenuID_t cm)
{
    int32_t i;
    DukePlayer_t *ps = g_player[myconnectindex].ps;

    switch (cm)
    {
    case MENU_MAIN_INGAME:
        MenuEntry_DisableOnCondition(&ME_MAIN_SAVEGAME, ud.recstat == 2);
        MenuEntry_DisableOnCondition(&ME_MAIN_QUITTOTITLE, g_netServer || numplayers > 1);
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

        vpsize = ud.screen_size + 4*(ud.screen_size>=8 && ud.statusbarmode==0);

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

    case MENU_RENDERERSETUP:
        MenuEntry_DisableOnCondition(&ME_RENDERERSETUP_TEXQUALITY, !usehightile);
        MenuEntry_DisableOnCondition(&ME_RENDERERSETUP_PRECACHE, !usehightile);
#ifndef EDUKE32_GLES
        MenuEntry_DisableOnCondition(&ME_RENDERERSETUP_TEXCACHE, !(glusetexcompr && usehightile));
        MenuEntry_DisableOnCondition(&ME_RENDERERSETUP_DETAILTEX, !usehightile);
#endif
        break;
#endif

    case MENU_VIDEOSETUP:
    {
        const int32_t nr = newresolution;

        MenuEntry_DisableOnCondition(&ME_VIDEOSETUP_APPLY,
             (xdim == resolution[nr].xdim && ydim == resolution[nr].ydim &&
              getrendermode() == newrendermode && fullscreen == newfullscreen)
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
        MenuEntry_DisableOnCondition(&ME_SOUND_NUMVOICES, !ud.config.SoundToggle);
        MenuEntry_DisableOnCondition(&ME_SOUND_RESTART, soundrate == ud.config.MixRate &&
                                                        soundvoices == ud.config.NumVoices);
        break;

    case MENU_MOUSESETUP:
        MenuEntry_DisableOnCondition(&ME_MOUSESETUP_MOUSEAIMING, ud.mouseaiming);
        break;

    case MENU_NETOPTIONS:
        if (MEOSV_NetEpisodes[MEO_NETOPTIONS_EPISODE.currentOption] == MAXVOLUMES)
            MEL_NETOPTIONS[2] = &ME_NETOPTIONS_USERMAP;
        else
        {
            MEL_NETOPTIONS[2] = &ME_NETOPTIONS_LEVEL;
            MEO_NETOPTIONS_LEVEL.options = &MEOS_NETOPTIONS_LEVEL[MEOSV_NetEpisodes[MEO_NETOPTIONS_EPISODE.currentOption]];
        }
        MEL_NETOPTIONS[4] = (GametypeFlags[ud.m_coop] & GAMETYPE_MARKEROPTION) ? &ME_NETOPTIONS_MARKERS : &ME_NETOPTIONS_MARKERS_DISABLED;
        MEL_NETOPTIONS[5] = (GametypeFlags[ud.m_coop] & (GAMETYPE_PLAYERSFRIENDLY|GAMETYPE_TDM)) ? &ME_NETOPTIONS_FRFIRE : &ME_NETOPTIONS_MAPEXITS;
        break;

    case MENU_OPTIONS:
        MenuEntry_DisableOnCondition(&ME_OPTIONS_PLAYERSETUP, ud.recstat == 1);
        MenuEntry_DisableOnCondition(&ME_OPTIONS_JOYSTICKSETUP, CONTROL_JoyPresent == 0);
        break;

    case MENU_LOAD:
        for (i = 0; i < MAXSAVEGAMES; ++i)
        {
            ME_LOAD[i].entry = (ud.savegame[i][0] /*&& !g_oldverSavegame[i]*/) ? &MEO_LOAD_VALID : &MEO_LOAD;
            ME_LOAD[i].font = g_oldverSavegame[i] ? &MF_MinifontDarkGray : &MF_MinifontRed;
        }
        break;

    case MENU_SAVE:
        for (i = 0; i < MAXSAVEGAMES; ++i)
            MEO_SAVE[i].font = (g_oldverSavegame[i] && MEO_SAVE[i].editfield == NULL) ? &MF_MinifontDarkGray : &MF_MinifontRed;
        break;

    case MENU_CHEATS:
    case MENU_CHEATENTRY:
    case MENU_CHEAT_WARP:
    case MENU_CHEAT_SKILL:
    {
        const int32_t menucheatsdisabled = numplayers != 1 || !(g_player[myconnectindex].ps->gm & MODE_GAME);

        for (i = 0; i < NUMCHEATFUNCS; i++)
        {
            // only show cheats that have been typed in before
            MEL_CHEATS[i+1] = (cl_cheatmask & (1<<i)) ? &ME_CheatCodes[i] : NULL;

            // disable outside of a single-player game
            MenuEntry_DisableOnCondition(&ME_CheatCodes[i], menucheatsdisabled);
        }

        // refresh display names of quote cheats
        if (!DUKEBETA)
        {
            ME_CheatCodes[CHEATFUNC_QUOTEBETA].name = ScriptQuotes[QUOTE_CHEAT_BETA];
            ME_CheatCodes[CHEATFUNC_QUOTETODD].name = ScriptQuotes[QUOTE_CHEAT_TODD];
            ME_CheatCodes[CHEATFUNC_QUOTEALLEN].name = ScriptQuotes[QUOTE_CHEAT_ALLEN];
        }
        break;
    }

    default:
        break;
    }
}


static void M_PreMenuDrawBackground(MenuID_t cm, const vec2_t origin)
{
    switch (cm)
    {
    case MENU_CREDITS:
    case MENU_CREDITS2:
    case MENU_CREDITS3:
        if (!VOLUMEALL || !PLUTOPAK)
            M_DrawBackground(origin);
        else
            rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + (100<<16), 65536L,0,2504+cm-MENU_CREDITS,0,0,10+64);
        break;

    case MENU_LOAD:
    case MENU_SAVE:
    case MENU_CREDITS4:
    case MENU_CREDITS5:
        M_DrawBackground(origin);
        break;

    case MENU_STORY:
        rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + (100<<16), 65536L,0,TEXTSTORY,0,0,10+64);
        break;

    case MENU_F1HELP:
        rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + (100<<16), 65536L,0,F1HELP,0,0,10+64);
        break;
    }
}


static void M_PreMenuDraw(MenuID_t cm, MenuEntry_t *entry, const vec2_t origin)
{
    int32_t i, j, l = 0, m;

    switch (cm)
    {
    case MENU_MAIN_INGAME:
        l += 4;
    case MENU_MAIN:
        rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + ((28+l)<<16), 65536L,0,INGAMEDUKETHREEDEE,0,0,10);
        if (PLUTOPAK)   // JBF 20030804
            rotatesprite_fs(origin.x + ((MENU_MARGIN_CENTER+100)<<16), origin.y + (36<<16), 65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8);
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
        mminitext(origin.x + (90<<16), origin.y + ((90+8+8+8)<<16), "Monsters", 2);
        if (ud.m_coop == 0)
            mminitext(origin.x + (90<<16), origin.y + ((90+8+8+8+8)<<16), "Markers", 2);
        else if (ud.m_coop == 1)
            mminitext(origin.x + (90<<16), origin.y + ((90+8+8+8+8)<<16), "Friendly Fire", 2);
        mminitext(origin.x + (90<<16), origin.y + ((90+8+8+8+8+8)<<16), "User Map", 2);

        mminitext(origin.x + ((90+60)<<16), origin.y + (90<<16), GametypeNames[ud.m_coop], 0);

        mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8)<<16), EpisodeNames[ud.m_volume_number], 0);
        mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8)<<16), MapInfo[MAXLEVELS*ud.m_volume_number+ud.m_level_number].name, 0);
        if (ud.m_monsters_off == 0 || ud.m_player_skill > 0)
            mminitext(origin.x + ((90+60)<<16), origin.y + ((90+8+8+8)<<16), SkillNames[ud.m_player_skill], 0);
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

    case MENU_KEYBOARDKEYS:
        mgametextcenter(origin.x, origin.y + (156<<16), "Up/Down = Select Action\n"
                                                        "Left/Right = Select List\n"
                                                        "Enter = Modify   Delete = Clear");
        break;

    case MENU_MOUSESETUP:
        if (entry == &ME_MOUSESETUP_MOUSEAIMING)
        {
            if (entry->flags & Disabled)
            {
                mgametextcenter(origin.x, origin.y + (167<<16), "Set mouse aim type to toggle on/off\n"
                                                                "in the Player Setup menu to enable");
            }
        }
        break;

    case MENU_MOUSEBTNS:
        mgametextcenter(origin.x, origin.y + (169<<16), "Up/Down = Select Button\n"
                                                        "Enter = Modify");
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

    case MENU_JOYSTICKBTNS:
        mgametextcenter(origin.x, origin.y + (149<<16), "Up/Down = Select Button\n"
                                                        "Enter = Modify");
        break;

    case MENU_RESETPLAYER:
        fade_screen_black(1);
        mgametextcenter(origin.x, origin.y + (90<<16), "Load last game:");

        Bsprintf(tempbuf,"\"%s\"",ud.savegame[g_lastSaveSlot]);
        mgametextcenter(origin.x, origin.y + (99<<16), tempbuf);

#ifndef DROIDMENU
        mgametextcenter(origin.x, origin.y + ((99+9)<<16), "(Y/N)");
#endif
        break;

    case MENU_LOAD:
        for (i = 0; i <= 108; i += 12)
            rotatesprite_fs(origin.x + ((160+64+91-64)<<16), origin.y + ((i+56)<<16), 65536L,0,TEXTBOX,24,0,10);

        rotatesprite_fs(origin.x + (22<<16), origin.y + (97<<16), 65536L,0,WINDOWBORDER2,24,0,10);
        rotatesprite_fs(origin.x + (180<<16), origin.y + (97<<16), 65536L,1024,WINDOWBORDER2,24,0,10);
        rotatesprite_fs(origin.x + (99<<16), origin.y + (50<<16), 65536L,512,WINDOWBORDER1,24,0,10);
        rotatesprite_fs(origin.x + (103<<16), origin.y + (144<<16), 65536L,1024+512,WINDOWBORDER1,24,0,10);

        if (ud.savegame[M_LOAD.currentEntry][0])
        {
            rotatesprite_fs(origin.x + (101<<16), origin.y + (97<<16), 65536>>1,512,TILE_LOADSHOT,-32,0,4+10+64);

            if (g_oldverSavegame[M_LOAD.currentEntry])
            {
                mmenutext(origin.x + (53<<16), origin.y + (70<<16), "Previous");
                mmenutext(origin.x + (58<<16), origin.y + (90<<16), "Version");

#ifndef DROIDMENU
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
                const char *name = MapInfo[(savehead.volnum*MAXLEVELS) + savehead.levnum].name;
                Bsprintf(tempbuf, "%s / %s", name ? name : "^10unnamed^0", SkillNames[savehead.skill-1]);
            }

            mgametextcenter(origin.x, origin.y + (168<<16), tempbuf);
            if (savehead.volnum == 0 && savehead.levnum == 7)
                mgametextcenter(origin.x, origin.y + (180<<16), savehead.boardfn);
        }
        else
            mmenutext(origin.x + (69<<16), origin.y + (70<<16), "Empty");
        break;

    case MENU_SAVE:
        for (i = 0; i <= 108; i += 12)
            rotatesprite_fs(origin.x + ((160+64+91-64)<<16), origin.y + ((i+56)<<16), 65536L,0,TEXTBOX,24,0,10);

        rotatesprite_fs(origin.x + (22<<16), origin.y + (97<<16), 65536L,0,WINDOWBORDER2,24,0,10);
        rotatesprite_fs(origin.x + (180<<16), origin.y + (97<<16), 65536L,1024,WINDOWBORDER2,24,0,10);
        rotatesprite_fs(origin.x + (99<<16), origin.y + (50<<16), 65536L,512,WINDOWBORDER1,24,0,10);
        rotatesprite_fs(origin.x + (103<<16), origin.y + (144<<16), 65536L,1024+512,WINDOWBORDER1,24,0,10);

        j = 0;
        for (i = 0; i < 10; ++i)
            if (((MenuString_t*)M_SAVE.entrylist[i]->entry)->editfield)
                j |= 1;

        if (j)
            rotatesprite_fs(origin.x + (101<<16), origin.y + (97<<16), 65536L>>1,512,TILE_SAVESHOT,-32,0,4+10+64);
        else if (ud.savegame[M_SAVE.currentEntry][0])
            rotatesprite_fs(origin.x + (101<<16), origin.y + (97<<16), 65536L>>1,512,TILE_LOADSHOT,-32,0,4+10+64);
        else
            mmenutext(origin.x + (69<<16), origin.y + (70<<16), "Empty");

        if (ud.savegame[M_SAVE.currentEntry][0] && g_oldverSavegame[M_SAVE.currentEntry])
        {
            mmenutext(origin.x + (53<<16), origin.y + (70<<16), "Previous");
            mmenutext(origin.x + (58<<16), origin.y + (90<<16), "Version");

#ifndef DROIDMENU
            Bsprintf(tempbuf,"Saved: %d.%d.%d %d-bit", savehead.majorver, savehead.minorver,
                     savehead.bytever, 8*savehead.ptrsize);
            mgametext(origin.x + (31<<16), origin.y + (104<<16), tempbuf);
            Bsprintf(tempbuf,"Our: %d.%d.%d %d-bit", SV_MAJOR_VER, SV_MINOR_VER, BYTEVERSION,
                     (int32_t)(8*sizeof(intptr_t)));
            mgametext(origin.x + ((31+16)<<16), origin.y + (114<<16), tempbuf);
#endif
        }

        if (ud.multimode > 1)
        {
            Bsprintf(tempbuf, "Players: %-2d                      ", ud.multimode);
            mgametextcenter(origin.x, origin.y + (156<<16), tempbuf);
        }

        Bsprintf(tempbuf,"%s / %s",MapInfo[(ud.volume_number*MAXLEVELS) + ud.level_number].name, SkillNames[ud.player_skill-1]);
        mgametextcenter(origin.x, origin.y + (168<<16), tempbuf);
        if (ud.volume_number == 0 && ud.level_number == 7)
            mgametextcenter(origin.x, origin.y + (180<<16), currentboardfilename);
        break;

#ifdef DROIDMENU
    case MENU_SKILL:
    {
        static const char *s[] = { "EASY - Few enemies, and lots of stuff.", "MEDIUM - Normal difficulty.", "HARD - For experienced players.", "EXPERTS - Lots of enemies, plus they respawn!" };
        if ((size_t)M_SKILL.currentEntry < ARRAY_SIZE(s))
            mgametextcenter(origin.x, origin.y + (168<<16), s[M_SKILL.currentEntry]);
    }
        break;
#endif

    case MENU_LOADVERIFY:
        fade_screen_black(1);
        if (g_oldverSavegame[M_LOAD.currentEntry])
        {
            mgametextcenter(origin.x, origin.y + (90<<16), "Start new game:");
            Bsprintf(tempbuf,"%s / %s",MapInfo[(ud.volume_number*MAXLEVELS) + ud.level_number].name, SkillNames[ud.player_skill-1]);
            mgametextcenter(origin.x, origin.y + (99<<16), tempbuf);
        }
        else
        {
            mgametextcenter(origin.x, origin.y + (90<<16), "Load game:");
            Bsprintf(tempbuf, "\"%s\"", ud.savegame[M_LOAD.currentEntry]);
            mgametextcenter(origin.x, origin.y + (99<<16), tempbuf);
        }
#ifndef DROIDMENU
        mgametextcenter(origin.x, origin.y + ((99+9)<<16), "(Y/N)");
#endif
        break;

    case MENU_SAVEVERIFY:
        fade_screen_black(1);
        mgametextcenter(origin.x, origin.y + (90<<16), "Overwrite previous saved game?");
#ifndef DROIDMENU
        mgametextcenter(origin.x, origin.y + ((90+9)<<16), "(Y/N)");
#endif
        break;

    case MENU_NEWVERIFY:
        fade_screen_black(1);
        mgametextcenter(origin.x, origin.y + (90<<16), "Abort this game?");
#ifndef DROIDMENU
        mgametextcenter(origin.x, origin.y + ((90+9)<<16), "(Y/N)");
#endif
        break;

    case MENU_QUIT:
    case MENU_QUIT_INGAME:
        fade_screen_black(1);
        mgametextcenter(origin.x, origin.y + (90<<16), "Are you sure you want to quit?");
#ifndef DROIDMENU
        mgametextcenter(origin.x, origin.y + (99<<16), "(Y/N)");
#endif
        break;

    case MENU_QUITTOTITLE:
        fade_screen_black(1);
        mgametextcenter(origin.x, origin.y + (90<<16), "Quit to Title?");
#ifndef DROIDMENU
        mgametextcenter(origin.x, origin.y + (99<<16), "(Y/N)");
#endif
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

#ifndef DROIDMENU
        mgametextcenter(origin.x, origin.y + (112<<16), "Please visit Steam and purchase\n"
                                                        "Duke Nukem 3D: Megaton Edition\n"
                                                        "to upgrade to the full registered\n"
                                                        "version of Duke Nukem 3D.");
#else
# if defined EDUKE32_IOS
        mgametextcenter(origin.x, origin.y + (121<<16), "Please visit the App Store\n"
                                                        "to upgrade to the full registered\n"
                                                        "version of Duke Nukem 3D.");
# else
        mgametextcenter(origin.x, origin.y + (121<<16), "Please visit the Play Store\n"
                                                        "to upgrade to the full registered\n"
                                                        "version of Duke Nukem 3D.");
# endif
#endif
        mgametextcenter(origin.x, origin.y + ((148+16)<<16), "Press any key or button...");
        break;

    case MENU_CREDITS:
    case MENU_CREDITS2:
    case MENU_CREDITS3:
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
                mgametextcenter(origin.x, origin.y + (50<<16), "Duke Nukem 3D is a trademark of");
                mgametextcenter(origin.x, origin.y + ((50+9)<<16), "3D Realms Entertainment");

                mgametextcenter(origin.x, origin.y + ((50+9+9+9)<<16), "Duke Nukem 3D");
                mgametextcenter(origin.x, origin.y + ((50+9+9+9+9)<<16), "(C) 1996, 2014 3D Realms Entertainment");

#ifndef DROIDMENU
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
    case MENU_CREDITS4:   // JBF 20031220
        l = 7;

        mgametextcenter(origin.x, origin.y + ((50-l)<<16), "Production, design, and programming");
        creditsminitext(origin.x + (160<<16), origin.y + ((50+10-l)<<16), "Richard \"TerminX\" Gobeille", 8);

#if !defined(POLYMER) || defined(DROIDMENU)
        mgametextcenter(origin.x, origin.y + ((70-l)<<16), "Rendering and support programming");
#else
        mgametextcenter(origin.x, origin.y + ((70-l)<<16), "Polymer Rendering System by");
#endif
        creditsminitext(origin.x + (160<<16), origin.y + ((70+10-l)<<16), "Pierre-Loup \"Plagman\" Griffais", 8);

        mgametextcenter(origin.x, origin.y + ((90-l)<<16), "Engine and game programming");
        creditsminitext(origin.x + (160<<16), origin.y + ((90+10-l)<<16), "Philipp \"Helixhorned\" Kutin", 8);
        creditsminitext(origin.x + (160<<16), origin.y + ((90+7+10-l)<<16), "Evan \"Hendricks266\" Ramos", 8);


        mgametextcenter(origin.x, origin.y + ((130+7-l)<<16), "Based on \"JFDuke3D\" by");
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
                "Alan Ondra",        // testing
                "Bioman",            // GTK work, APT repository and package upkeep
                "Brandon Bergren",   // "Bdragon" - tiles.cfg
                "Charlie Honig",     // "CONAN" - showview command
                "Dan Gaskill",       // "DeeperThought" - testing
                "David Koenig",      // "Bargle" - Merged a couple of things from duke3d_w32
                "Ed Coolidge",       // Mapster32 improvements
                "Emile Belanger",    // original Android work
                "Ferry Landzaat",    // ? (listed on the wiki page)
                "Hunter_rus",        // tons of stuff
                "James Bentler",     // Mapster32 improvements
                "Jasper Foreman",    // netcode contributions
                "Javier Martinez",   // "Malone3D" - EDuke 2.1.1 components
                "Jeff Hart",         // website graphics
                "Jonathan Smith",    // "Mblackwell" - testing
                "Jose del Castillo", // "Renegado" - EDuke 2.1.1 components
                "Lachlan McDonald",  // official EDuke32 icon
                "LSDNinja",          // OS X help and testing
                "Marcus Herbert",    // "rhoenie" - OS X compatibility work
                "Matthew Palmer",    // "Usurper" - testing and eduke32.com domain
                "Matt Saettler",     // original DOS EDuke/WW2GI enhancements
                "Ozkan Sezer",       // SDL/GTK version checking improvements
                "Peter Green",       // "Plugwash" - dynamic remapping, custom gametypes
                "Peter Veenstra",    // "Qbix" - port to 64-bit
                "Randy Heit",        // random snippets of ZDoom here and there
                "Robin Green",       // CON array support
                "Ryan Gordon",       // "icculus" - icculus.org Duke3D port sound code
                "Stephen Anthony",   // early 64-bit porting work
                "Thijs Leenders",    // Android icon work
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



static void M_PreMenuInput(MenuEntry_t *entry)
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

static void M_PreMenuOptionListDraw(MenuEntry_t *entry, const vec2_t origin)
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

static int32_t M_PreMenuCustom2ColScreen(MenuEntry_t *entry)
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

static void M_PreMenuCustom2ColScreenDraw(MenuEntry_t *entry, const vec2_t origin)
{
    if (g_currentMenu == MENU_KEYBOARDKEYS)
    {
        mgametextcenter(origin.x, origin.y + (90<<16), "Press the key to assign as");
        Bsprintf(tempbuf,"%s for \"%s\"", M_KEYBOARDKEYS.currentColumn?"secondary":"primary", entry->name);
        mgametextcenter(origin.x, origin.y + ((90+9)<<16), tempbuf);
        mgametextcenter(origin.x, origin.y + ((90+9+9+9)<<16), "Press \"Escape\" To Cancel");
    }
}

static void M_MenuEntryFocus(/*MenuEntry_t *entry*/)
{
    switch (g_currentMenu)
    {
    case MENU_LOAD:
        G_LoadSaveHeaderNew(M_LOAD.currentEntry, &savehead);
        break;
    case MENU_SAVE:
        G_LoadSaveHeaderNew(M_SAVE.currentEntry, &savehead);
        break;

    default:
        break;
    }
}

static void M_StartGameWithoutSkill(void)
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

static void M_Cheat(int32_t cheatID)
{
    if (numplayers != 1 || !(g_player[myconnectindex].ps->gm & MODE_GAME))
        return;

    osdcmd_cheatsinfo_stat.cheatnum = cheatID;
}

static int32_t M_Cheat_Warp(char const * const numbers)
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

    if ((VOLUMEONE && osdcmd_cheatsinfo_stat.volume > 0) || osdcmd_cheatsinfo_stat.volume > g_numVolumes-1 ||
            osdcmd_cheatsinfo_stat.level >= MAXLEVELS || MapInfo[osdcmd_cheatsinfo_stat.volume *MAXLEVELS+osdcmd_cheatsinfo_stat.level].filename == NULL)
        return 1;

    osdcmd_cheatsinfo_stat.cheatnum = CHEAT_SCOTTY;

    return 0;
}

static int32_t M_Cheat_Skill(char const * const number)
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
static void M_MenuEntryLinkActivate(MenuEntry_t *entry)
{
    switch (g_currentMenu)
    {
    case MENU_EPISODE:
        if (entry != &ME_EPISODE_USERMAP)
        {
            ud.m_volume_number = M_EPISODE.currentEntry;
            ud.m_level_number = 0;

            if (g_numSkills == 0)
                M_StartGameWithoutSkill();
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
                M_Cheat(CheatFunctionIDs[cheatFuncID]);
                break;
        }
        break;
    }

    default:
        break;
    }

    if (entry == &ME_VIDEOSETUP_APPLY)
    {
        int32_t pxdim, pydim, pfs, pbpp, prend;
        int32_t nxdim, nydim, nfs, nbpp, nrend;

        pxdim = xdim;
        pydim = ydim;
        pbpp  = bpp;
        pfs   = fullscreen;
        prend = getrendermode();
        nxdim = resolution[newresolution].xdim;
        nydim = resolution[newresolution].ydim;
        nfs   = newfullscreen;

        nbpp  = (newrendermode == REND_CLASSIC) ? 8 : resolution[newresolution].bppmax;
        nrend = newrendermode;

        if (setgamemode(nfs, nxdim, nydim, nbpp) < 0)
        {
            if (setgamemode(pfs, pxdim, pydim, pbpp) < 0)
            {
                setrendermode(prend);
                G_GameExit("Failed restoring old video mode.");
            }
            else onvideomodechange(pbpp > 8);
        }
        else onvideomodechange(nbpp > 8);

        g_restorePalette = -1;
        G_UpdateScreenArea();
        setrendermode(nrend);

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
        CONFIG_SetDefaultKeys((const char (*)[MAXGAMEFUNCLEN])keydefaults);
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
            M_ChangeMenu(MENU_NETWAITVOTES);
        }
    }
}

static int32_t M_MenuEntryOptionModify(MenuEntry_t *entry, int32_t newOption)
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
        case 1:
            ud.weaponswitch |= 1;
            break;
        default:
            break;
        }
    }
#ifdef USE_OPENGL
    else if (entry == &ME_DISPLAYSETUP_TEXFILTER)
    {
        gltexfiltermode = newOption ? TEXFILTER_ON : TEXFILTER_OFF;
        gltexapplyprops();
    }
    else if (entry == &ME_DISPLAYSETUP_ASPECTRATIO)
    {
        r_usenewaspect = newOption & 1;
    }
#ifdef POLYMER
    else if (entry == &ME_DISPLAYSETUP_ASPECTRATIO_POLYMER)
    {
        pr_customaspect = MEOSV_DISPLAYSETUP_ASPECTRATIO_POLYMER[newOption];
    }
#endif
    else if (entry == &ME_DISPLAYSETUP_VSYNC)
        setvsync(newOption);
#endif
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
        if ((unsigned)newOption < g_numVolumes)
            ud.m_volume_number = newOption;
    }
    else if (entry == &ME_NETOPTIONS_MONSTERS)
    {
        ud.m_monsters_off = (newOption == g_numSkills);
        if (newOption < g_numSkills)
            ud.m_player_skill = newOption;
    }
    else if (entry == &ME_ADULTMODE)
    {
        if (newOption)
        {
            for (x=0; x<g_numAnimWalls; x++)
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
            M_ChangeMenu(MENU_ADULTPASSWORD);
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
                M_ChangeMenu(MENU_ADULTPASSWORD);
                return -1;
            }
        }
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

static void M_MenuEntryOptionDidModify(MenuEntry_t *entry)
{
    if (entry == &ME_GAMESETUP_AIM_AUTO ||
        entry == &ME_GAMESETUP_WEAPSWITCH_PICKUP ||
        entry == &ME_PLAYER_NAME ||
        entry == &ME_PLAYER_COLOR ||
        entry == &ME_PLAYER_TEAM)
        G_UpdatePlayerFromMenu();
#ifdef USE_OPENGL
    else if (entry == &ME_DISPLAYSETUP_ANISOTROPY)
        gltexapplyprops();
    else if (entry == &ME_RENDERERSETUP_TEXQUALITY)
    {
        texcache_invalidate();
        resetvideomode();
        if (setgamemode(fullscreen,xdim,ydim,bpp))
            OSD_Printf("restartvid: Reset failed...\n");
        r_downsizevar = r_downsize;
    }
#endif
}

static void M_MenuCustom2ColScreen(/*MenuEntry_t *entry*/)
{
    if (g_currentMenu == MENU_KEYBOARDKEYS)
    {
        KB_FlushKeyboardQueue();
        KB_ClearLastScanCode();
    }
}

static int32_t M_MenuEntryRangeInt32Modify(MenuEntry_t *entry, int32_t newValue)
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

static int32_t M_MenuEntryRangeFloatModify(MenuEntry_t *entry, float newValue)
{
    if (entry == &ME_COLCORR_AMBIENT)
        r_ambientlightrecip = 1.f/newValue;

    return 0;
}

static int32_t M_MenuEntryRangeFloatDidModify(MenuEntry_t *entry)
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

static int32_t M_MenuEntryRangeDoubleModify(void /*MenuEntry_t *entry, double newValue*/)
{

    return 0;
}

static uint32_t save_xxh = 0;

static void M_MenuEntryStringActivate(/*MenuEntry_t *entry*/)
{
    switch (g_currentMenu)
    {
    case MENU_SAVE:
        if (!save_xxh)
            save_xxh = XXH32((uint8_t *)&ud.savegame[M_SAVE.currentEntry][0], 19, 0xDEADBEEF);
        if (ud.savegame[M_SAVE.currentEntry][0])
            M_ChangeMenu(MENU_SAVEVERIFY);
        break;

    default:
        break;
    }
}

static int32_t M_MenuEntryStringSubmit(MenuEntry_t *entry, char *input)
{
    MenuString_t *object = (MenuString_t*)entry->entry;
    int32_t returnvar = 0;

    switch (g_currentMenu)
    {
    case MENU_SAVE:
        // dirty hack... char 127 in last position indicates an auto-filled name
#ifdef __ANDROID__
        if (1)
#else
        if (input[0] == 0 || (ud.savegame[M_SAVE.currentEntry][MAXSAVEGAMENAME-2] == 127 &&
            Bstrncmp(&ud.savegame[M_SAVE.currentEntry][0], input, MAXSAVEGAMENAME-3) == 0 &&
            save_xxh == XXH32((uint8_t *)&ud.savegame[M_SAVE.currentEntry][0], MAXSAVEGAMENAME-3, 0xDEADBEEF)))
#endif
        {
            Bstrncpy(&ud.savegame[M_SAVE.currentEntry][0], MapInfo[ud.volume_number * MAXLEVELS + ud.level_number].name, MAXSAVEGAMENAME-3);
            ud.savegame[M_SAVE.currentEntry][MAXSAVEGAMENAME-2] = 127;
            returnvar = -1;
        }
        else
        {
            ud.savegame[M_SAVE.currentEntry][MAXSAVEGAMENAME-2] = 0;
            Bstrncpy(object->variable, input, object->maxlength);
        }

        G_SavePlayerMaybeMulti(M_SAVE.currentEntry);

        g_lastSaveSlot = M_SAVE.currentEntry;
        g_player[myconnectindex].ps->gm = MODE_GAME;

        M_ChangeMenu(MENU_CLOSE);
        save_xxh = 0;
        break;

    default:
        break;
    }

    return returnvar;
}

static void M_MenuEntryStringCancel(/*MenuEntry_t *entry*/)
{
    switch (g_currentMenu)
    {
    case MENU_SAVE:
        save_xxh = 0;
        ReadSaveGameHeaders();
        break;

    default:
        break;
    }
}

/*
This is polled when the menu code is populating the screen but for some reason doesn't have the data.
*/
static int32_t M_MenuEntryOptionSource(MenuEntry_t *entry, int32_t currentValue)
{
    if (entry == &ME_GAMESETUP_WEAPSWITCH_PICKUP)
        return (ud.weaponswitch & 1) ? ((ud.weaponswitch & 4) ? 2 : 1) : 0;
#ifdef USE_OPENGL
/*
    else if (entry == &ME_DISPLAYSETUP_TEXFILTER)
        return gltexfiltermode;
*/
    else if (entry == &ME_DISPLAYSETUP_ASPECTRATIO)
        return r_usenewaspect;
#ifdef POLYMER
    else if (entry == &ME_DISPLAYSETUP_ASPECTRATIO_POLYMER)
        return clamp(currentValue, 0, ARRAY_SIZE(MEOSV_DISPLAYSETUP_ASPECTRATIO_POLYMER)-1);
#endif
#endif
    else if (entry == &ME_SOUND_DUKETALK)
        return ud.config.VoiceToggle & 1;
    else if (entry == &ME_NETOPTIONS_MONSTERS)
        return (ud.m_monsters_off ? g_numSkills : ud.m_player_skill);

    return currentValue;
}

static void M_MenuVerify(int32_t input)
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

            G_LoadPlayerMaybeMulti(g_lastSaveSlot);
        }
        else
        {
            if (sprite[g_player[myconnectindex].ps->i].extra <= 0)
            {
                if (G_EnterLevel(MODE_GAME)) G_BackToMenu();
                return;
            }

            M_ChangeMenu(MENU_CLOSE);
        }
        break;

    case MENU_LOADVERIFY:
        if (input)
        {
            g_lastSaveSlot = M_LOAD.currentEntry;

            KB_FlushKeyboardQueue();
            KB_ClearKeysDown();

            M_ChangeMenu(MENU_CLOSE);

            G_LoadPlayerMaybeMulti(g_lastSaveSlot);
        }
        break;

    case MENU_SAVEVERIFY:
        if (!input)
        {
            save_xxh = 0;
            ReadSaveGameHeaders();

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

static void M_MenuTextFormSubmit(char *input)
{
    switch (g_currentMenu)
    {
    case MENU_ADULTPASSWORD:
        if (Bstrlen(input) && (ud.pwlockout[0] == 0 || ud.lockout == 0))
            Bstrcpy(&ud.pwlockout[0], input);
#if 0
        else if (Bstrcmp(input, &ud.pwlockout[0]) == 0)
        {
#if 0
            for (x=0; x<g_numAnimWalls; x++)
                if (wall[animwall[x].wallnum].picnum != W_SCREENBREAK &&
                        wall[animwall[x].wallnum].picnum != W_SCREENBREAK+1 &&
                        wall[animwall[x].wallnum].picnum != W_SCREENBREAK+2)
                    if (wall[animwall[x].wallnum].extra >= 0)
                        wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
#endif
        }
#endif

        S_PlaySound(PISTOL_BODYHIT);
        M_ChangeMenu(MENU_GAMESETUP);
        break;

    case MENU_CHEATENTRY:
    {
        const size_t inputlength = Bstrlen(input);
        Bstrcpy(tempbuf, input);
        const char *numberpos = NULL;
        for (size_t i = 0; i < inputlength; i++)
        {
            tempbuf[i] = Btolower(tempbuf[i]);
            if (Bisdigit(tempbuf[i]))
            {
                if (numberpos == NULL)
                    numberpos = &input[i];
                tempbuf[i] = '#';
            }
        }

        int8_t cheatID = -1;

        if (inputlength > 2 && tempbuf[0] == scantoasc[CheatKeys[0]] && tempbuf[1] == scantoasc[CheatKeys[1]])
        {
            for (size_t i = 0; i < NUMCHEATS; i++)
                if (!Bstrcmp(tempbuf+2, CheatStrings[i]))
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
                M_Cheat_Warp(numberpos);
                if (g_player[myconnectindex].ps->gm&MODE_MENU)
                    S_PlaySound(DUKE_GET);
                break;
            case CHEAT_SKILL:
                M_Cheat_Skill(numberpos);
                if (g_player[myconnectindex].ps->gm&MODE_MENU)
                    S_PlaySound(DUKE_GET);
                break;
            default:
                M_Cheat(cheatID);
                S_PlaySound(DUKE_GET);
                break;
        }

        if (cheatID >= 0)
            cl_cheatmask |= CheatFunctionFlags[cheatID];

        if ((cl_cheatmask & (1<<CHEATFUNC_QUOTEBETA)) && (cl_cheatmask & (1<<CHEATFUNC_QUOTETODD)) && (cl_cheatmask & (1<<CHEATFUNC_QUOTEALLEN)))
            cl_cheatmask = ~0;

        M_ChangeMenu(MENU_CHEATS);
        break;
    }

    case MENU_CHEAT_WARP:
        if (M_Cheat_Warp(input))
            S_PlaySound(KICK_HIT);
        M_ChangeMenu(MENU_CHEATS);
        break;

    case MENU_CHEAT_SKILL:
        if (M_Cheat_Skill(input))
            S_PlaySound(KICK_HIT);
        M_ChangeMenu(MENU_CHEATS);
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

static void M_MenuFileSelectInit(MenuFileSelect_t *object)
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

static void M_MenuFileSelect(int32_t input)
{
    switch (g_currentMenu)
    {
    case MENU_NETUSERMAP:
        if ((g_netServer || ud.multimode > 1))
            Net_SendUserMapName();
    case MENU_USERMAP:
        if (input)
        {
            ud.m_volume_number = 0;
            ud.m_level_number = 7;

            if (g_numSkills > 0)
                M_ChangeMenuAnimate(MENU_SKILL, MA_Advance);
            else
                M_StartGameWithoutSkill();
        }
        break;

    default:
        break;
    }
}





static Menu_t* M_FindMenuBinarySearch(MenuID_t query, size_t searchstart, size_t searchend)
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

    return M_FindMenuBinarySearch(query, searchstart, searchend);
}

static Menu_t* M_FindMenu(MenuID_t query)
{
    if ((unsigned) query > (unsigned) Menus[numMenus-1].menuID)
        return NULL;

    return M_FindMenuBinarySearch(query, 0, numMenus-1);
}

typedef struct MenuAnimation_t
{
    int32_t (*out)(struct MenuAnimation_t *);
    int32_t (*in)(struct MenuAnimation_t *);

    Menu_t *previous;
    Menu_t *current;

    int32_t start;
    int32_t length;
} MenuAnimation_t;

static MenuAnimation_t m_animation;

int32_t M_Anim_SinOutRight(MenuAnimation_t *animdata)
{
    return sintable[scale(1024, totalclock - animdata->start, animdata->length) + 512] - 16384;
}
int32_t M_Anim_SinInRight(MenuAnimation_t *animdata)
{
    return sintable[scale(1024, totalclock - animdata->start, animdata->length) + 512] + 16384;
}
int32_t M_Anim_SinOutLeft(MenuAnimation_t *animdata)
{
    return -sintable[scale(1024, totalclock - animdata->start, animdata->length) + 512] + 16384;
}
int32_t M_Anim_SinInLeft(MenuAnimation_t *animdata)
{
    return -sintable[scale(1024, totalclock - animdata->start, animdata->length) + 512] - 16384;
}

void M_ChangeMenuAnimate(int32_t cm, MenuAnimationType_t animtype)
{
    switch (animtype)
    {
        case MA_Advance:
            m_animation.out = M_Anim_SinOutRight;
            m_animation.in = M_Anim_SinInRight;
            m_animation.start = totalclock;
            m_animation.length = 30;

            m_animation.previous = m_currentMenu;
            M_ChangeMenu(cm);
            m_animation.current = m_currentMenu;
            break;
        case MA_Return:
            m_animation.out = M_Anim_SinOutLeft;
            m_animation.in = M_Anim_SinInLeft;
            m_animation.start = totalclock;
            m_animation.length = 30;

            m_animation.previous = m_currentMenu;
            M_ChangeMenu(cm);
            m_animation.current = m_currentMenu;
            break;
        default:
            m_animation.start = 0;
            m_animation.length = 0;
            M_ChangeMenu(cm);
            break;
    }
}

void M_ChangeMenu(MenuID_t cm)
{
    Menu_t *search;
    int32_t i;

    cm = VM_OnEventWithReturn(EVENT_CHANGEMENU, g_player[myconnectindex].ps->i, myconnectindex, cm);

    if (cm == MENU_PREVIOUS)
    {
        m_currentMenu = m_previousMenu;
        g_currentMenu = g_previousMenu;
    }
    else if (cm == MENU_CLOSE)
        M_CloseMenu(myconnectindex);
    else if (cm >= 0)
    {
        if ((g_player[myconnectindex].ps->gm&MODE_GAME) && cm == MENU_MAIN)
            cm = MENU_MAIN_INGAME;

        search = M_FindMenu(cm);

        if (search == NULL)
            return;

        m_previousMenu = m_currentMenu;
        g_previousMenu = g_currentMenu;
        m_currentMenu = search;
        g_currentMenu = cm;
    }
    else
        return;

    switch (g_currentMenu)
    {
    case MENU_LOAD:
        if (g_lastSaveSlot >= 0 && g_previousMenu != MENU_LOADVERIFY)
            M_LOAD.currentEntry = g_lastSaveSlot;
        break;

    case MENU_SAVE:
        if (g_lastSaveSlot >= 0 && g_previousMenu != MENU_SAVEVERIFY)
            M_SAVE.currentEntry = g_lastSaveSlot;
        if (g_player[myconnectindex].ps->gm&MODE_GAME)
        {
            g_screenCapture = 1;
            G_DrawRooms(myconnectindex,65536);
            g_screenCapture = 0;
        }
        break;

    case MENU_VIDEOSETUP:
        newresolution = 0;
        for (i = 0; i < MAXVALIDMODES; ++i)
        {
            if (resolution[i].xdim == xdim && resolution[i].ydim == ydim)
            {
                newresolution = i;
                break;
            }
        }
        newrendermode = getrendermode();
        newfullscreen = fullscreen;
        break;

    case MENU_ADVSOUND:
        soundrate = ud.config.MixRate;
        soundvoices = ud.config.NumVoices;
        break;

    default:
        break;
    }

#if !defined EDUKE32_TOUCH_DEVICES
    m_menuchange_watchpoint = 1;
#endif

    if (m_currentMenu->type == TextForm)
    {
        typebuf[0] = 0;
        ((MenuTextForm_t*)m_currentMenu->object)->input = typebuf;
        WithSDL2_StartTextInput();
    }
    else if (m_currentMenu->type == FileSelect)
        M_MenuFileSelectInit((MenuFileSelect_t*)m_currentMenu->object);
    else if (m_currentMenu->type == Menu)
    {
        MenuMenu_t *menu = (MenuMenu_t*)m_currentMenu->object;
        // MenuEntry_t* currentry = menu->entrylist[menu->currentEntry];

        // need this for MENU_SKILL
        if (menu->currentEntry >= menu->numEntries)
            menu->currentEntry = 0;

        i = menu->currentEntry;
        while (!menu->entrylist[menu->currentEntry] || ((MenuEntry_t*)menu->entrylist[menu->currentEntry])->type == Spacer)
        {
            menu->currentEntry++;
            if (menu->currentEntry >= menu->numEntries)
                menu->currentEntry = 0;
            if (menu->currentEntry == i)
                G_GameExit("M_ChangeMenu: Attempted to change to a menu with no entries.");
        }

        M_MenuEntryFocus(/*currentry*/);
    }
}










void G_CheckPlayerColor(int32_t *color, int32_t prev_color)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(MEOSV_PLAYER_COLOR); ++i)
        if (*color == MEOSV_PLAYER_COLOR[i])
            return;

    *color = prev_color;
}


int32_t M_DetermineMenuSpecialState(MenuEntry_t *entry)
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

int32_t M_IsTextInput(Menu_t *cm)
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
            return M_DetermineMenuSpecialState(entry);
        }
            break;
    }

    return 0;
}

static inline int32_t M_BlackTranslucentBackgroundOK(MenuID_t cm)
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

static inline int32_t M_UpdateScreenOK(MenuID_t cm)
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

void M_OpenMenu(size_t playerID)
{
    g_player[playerID].ps->gm |= MODE_MENU;

    readmouseabsxy(&m_prevmousepos, &mouseabs);
    m_mouselastactivity = -M_MOUSETIMEOUT;

#if !defined EDUKE32_TOUCH_DEVICES
    m_mousewake_watchpoint = 0;
#endif

    AppGrabMouse(0);
}

void M_CloseMenu(size_t playerID)
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

static void M_BlackRectangle(int32_t x, int32_t y, int32_t width, int32_t height, int32_t orientation)
{
    const int32_t xscale = scale(65536, width, tilesiz[0].x<<16), yscale = scale(65536, height, tilesiz[0].y<<16);

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

static void M_ShadePal(const MenuFont_t *font, uint8_t status, int32_t *s, int32_t *p)
{
    *s = (status & MT_Selected) ? (sintable[(totalclock<<5)&2047]>>12) : font->shade_deselected;
    *p = (status & MT_Disabled) ? font->pal_disabled : font->pal;
}

FORCE_INLINE void rotatesprite_ybounds(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum, int8_t dashade, char dapalnum, int32_t dastat, int32_t y_upper, int32_t y_lower)
{
    rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, 0, 0, 0, ydim_from_200_16(y_upper), xdim-1, ydim_from_200_16(y_lower));
}

static vec2_t M_MenuText(int32_t x, int32_t y, const MenuFont_t *font, const char *t, uint8_t status, int32_t y_upper, int32_t y_lower)
{
    int32_t s, p, ybetween = font->ybetween;
    int32_t f = font->textflags;
    if (status & MT_XCenter)
        f |= TEXT_XCENTER;
    if (status & MT_XRight)
        f |= TEXT_XRIGHT;
    if (status & MT_YCenter)
    {
        f |= TEXT_YCENTER | TEXT_YOFFSETZERO;
        ybetween = font->yline; // <^ the battle against 'Q'
    }
    if (status & MT_Literal)
        f |= TEXT_LITERALESCAPE;

    M_ShadePal(font, status, &s, &p);

    return G_ScreenText(font->tilenum, x, y, 65536, 0, 0, t, s, p, 2|8|16|ROTATESPRITE_FULL16, 0, font->xspace, font->yline, font->xbetween, ybetween, f, 0, ydim_from_200_16(y_upper), xdim-1, ydim_from_200_16(y_lower));
}

#if 0
static vec2_t M_MenuTextSize(int32_t x, int32_t y, const MenuFont_t *font, const char *t, uint8_t status)
{
    int32_t f = font->textflags;
    if (status & MT_Literal)
        f |= TEXT_LITERALESCAPE;

    return G_ScreenTextSize(font->tilenum, x, y, 65536, 0, t, 2|8|16|ROTATESPRITE_FULL16, font->xspace, font->yline, font->xbetween, font->ybetween, f, 0, 0, xdim-1, ydim-1);
}
#endif

static int32_t M_FindOptionBinarySearch(MenuOption_t *object, const int32_t query, size_t searchstart, size_t searchend)
{
    const size_t thissearch = (searchstart + searchend) / 2;
    const int32_t difference = ((object->options->optionValues == NULL && query < 0) ? object->options->numOptions-1 : query) - ((object->options->optionValues == NULL) ? (int32_t) thissearch : object->options->optionValues[thissearch]);

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

    return M_FindOptionBinarySearch(object, query, searchstart, searchend);
}

static int32_t M_MouseWithinBounds(vec2_t const * const pos, const int32_t x, const int32_t y, const int32_t width, const int32_t height)
{
    return pos->x >= x && pos->x < x + width && pos->y >= y && pos->y < y + height;
}

static int32_t M_MouseOutsideBounds(vec2_t const * const pos, const int32_t x, const int32_t y, const int32_t width, const int32_t height)
{
    return pos->x < x || pos->x >= x + width || pos->y < y || pos->y >= y + height;
}

static void M_RunMenu_Scrollbar(Menu_t *cm, MenuMenuFormat_t const * const format, const int32_t totalextent, int32_t * const scrollPos, const int32_t rightedge, const vec2_t origin)
{
    if (totalextent > klabs(format->bottomcutoff))
    {
        const int32_t scrollx = origin.x + rightedge - (tilesiz[SELECTDIR].x<<16), scrolly = origin.y + format->pos.y;
        const int32_t scrollwidth = tilesiz[SELECTDIR].x<<16;
        const int32_t scrollheight = klabs(format->bottomcutoff) - format->pos.y;
        const int32_t scrollregionheight = scrollheight - (tilesiz[SELECTDIR].y<<16);
        const int32_t scrollPosMax = totalextent - klabs(format->bottomcutoff);

        M_BlackRectangle(scrollx, scrolly, scrollwidth, scrollheight, 1|32);

        rotatesprite_fs(scrollx, scrolly + scale(scrollregionheight, *scrollPos, scrollPosMax), 65536, 0, SELECTDIR, 0, 0, 26);

        if (cm == m_currentMenu && !m_mousecaught && MOUSEACTIVECONDITIONAL(mousepressstate == Mouse_Pressed || mousepressstate == Mouse_Held))
        {
            const int32_t scrolltilehalfheight = tilesiz[SELECTDIR].y<<15;
            const int32_t scrollregiony = scrolly + scrolltilehalfheight;

            // region between the y-midline of the arrow at the extremes scrolls proportionally
            if (M_MouseWithinBounds(&m_mousepos, scrollx, scrollregiony, scrollwidth, scrollregionheight))
            {
                *scrollPos = scale(m_mousepos.y - scrollregiony, scrollPosMax, scrollregionheight);

                m_mousecaught = 1;
            }
            // region outside the y-midlines clamps to the extremes
            else if (M_MouseWithinBounds(&m_mousepos, scrollx, scrolly, scrollwidth, scrollheight))
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

static MenuEntry_t *M_RunMenuInput_MenuMenu_MovementVerify(MenuMenu_t *menu);
static MenuEntry_t *M_RunMenuInput_MenuMenu_Movement(MenuMenu_t *menu, MenuMovement_t direction);
static void M_RunMenuInput_MenuEntryLink_Activate(MenuEntry_t *entry);
static void M_RunMenuInput_MenuEntryOptionList_MovementVerify(MenuOption_t *object);
static void M_RunMenuInput_MenuEntryOptionList_Movement(MenuOption_t *object, MenuMovement_t direction);
static int32_t M_RunMenuInput_MenuEntryOption_Modify(MenuEntry_t *entry, MenuOption_t *object, int32_t newValueIndex);
static int32_t M_RunMenuInput_MenuEntryOption_Movement(MenuEntry_t *entry, MenuOption_t *object, MenuMovement_t direction);
static int32_t M_RunMenuInput_MenuEntryOption_Activate(MenuEntry_t *entry, MenuOption_t *object);
static int32_t M_RunMenuInput_MenuEntryOptionList_Activate(MenuEntry_t *entry, MenuOption_t *object);
static void M_RunMenuInput_MenuEntryCustom2Col_Activate(MenuEntry_t *entry);
static void M_RunMenuInput_MenuEntryRangeInt32_MovementVerify(MenuEntry_t *entry, MenuRangeInt32_t *object, int32_t newValue);
static void M_RunMenuInput_MenuEntryRangeInt32_MovementArbitrary(MenuEntry_t *entry, MenuRangeInt32_t *object, int32_t newValue);
static void M_RunMenuInput_MenuEntryRangeInt32_Movement(MenuEntry_t *entry, MenuRangeInt32_t *object, MenuMovement_t direction);
static void M_RunMenuInput_MenuEntryRangeFloat_MovementVerify(MenuEntry_t *entry, MenuRangeFloat_t *object, float newValue);
static void M_RunMenuInput_MenuEntryRangeFloat_MovementArbitrary(MenuEntry_t *entry, MenuRangeFloat_t *object, float newValue);
static void M_RunMenuInput_MenuEntryRangeFloat_Movement(MenuEntry_t *entry, MenuRangeFloat_t *object, MenuMovement_t direction);
static void M_RunMenuInput_MenuEntryRangeDouble_MovementVerify(/*MenuEntry_t *entry, */MenuRangeDouble_t *object, double newValue);
static void M_RunMenuInput_MenuEntryRangeDouble_MovementArbitrary(/*MenuEntry_t *entry, */MenuRangeDouble_t *object, double newValue);
static void M_RunMenuInput_MenuEntryRangeDouble_Movement(/*MenuEntry_t *entry, */MenuRangeDouble_t *object, MenuMovement_t direction);
static void M_RunMenuInput_MenuEntryString_Activate(MenuEntry_t *entry);
static void M_RunMenuInput_MenuEntryString_Submit(MenuEntry_t *entry, MenuString_t *object);
static void M_RunMenuInput_MenuEntryString_Cancel(/*MenuEntry_t *entry, */MenuString_t *object);
static void M_RunMenuInput_FileSelect_MovementVerify(MenuFileSelect_t *object);
static void M_RunMenuInput_FileSelect_Movement(MenuFileSelect_t *object, MenuMovement_t direction);
static void M_RunMenuInput_FileSelect_Select(MenuFileSelect_t *object);

static int32_t M_RunMenu_MenuMenu(Menu_t *cm, MenuMenu_t *menu, MenuEntry_t *currentry, int32_t state, const vec2_t origin)
{
    const int32_t cursorShade = 4-(sintable[(totalclock<<4)&2047]>>11);

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

                // assumes height == font->yline!
                totalheight += entry->type == Spacer ? ((MenuSpacer_t*)entry->entry)->height : entry->font->yline;
            }

            calculatedentryspacing = (klabs(menu->format->bottomcutoff) - menu->format->pos.y - totalheight) / (numvalidentries > 1 ? numvalidentries - 1 : 1);
        }

        for (e = 0; e < menu->numEntries; ++e)
        {
            MenuEntry_t *entry = menu->entrylist[e];
            uint8_t status = 0;
            int32_t height, x;
            vec2_t textsize;

            if (entry == NULL)
                continue;

            x = menu->format->pos.x + entry->format->indent;

            if (e == menu->currentEntry)
                status |= MT_Selected;
            if (entry->flags & Disabled)
                status |= MT_Disabled;
            if (entry->format->width == 0)
                status |= MT_XCenter;

            const int32_t dodraw = entry->type != Spacer && 0 <= y - menu->scrollPos + entry->font->yline && y - menu->scrollPos <= klabs(menu->format->bottomcutoff) - menu->format->pos.y;

            if (dodraw)
                textsize = M_MenuText(origin.x + x, origin.y + y_upper + y - menu->scrollPos, entry->font, entry->name, status, origin.y + y_upper, origin.y + y_lower);

            height = entry->type == Spacer ? ((MenuSpacer_t*)entry->entry)->height : entry->font->yline; // max(textsize.y, entry->font->yline); // bluefont Q ruins this

            if (entry->format->width < 0)
                status |= MT_XRight;
            status |= MT_YCenter;

            if (dodraw && (status & MT_Selected) && state != 1)
            {
                if (status & MT_XCenter)
                {
                    rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16) + entry->format->cursorPosition, origin.y + y_upper + y + (height>>1) - menu->scrollPos, entry->format->cursorScale, 0, SPINNINGNUKEICON+6-((6+(totalclock>>3))%7), cursorShade, 0, 10);
                    rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16) - entry->format->cursorPosition, origin.y + y_upper + y + (height>>1) - menu->scrollPos, entry->format->cursorScale, 0, SPINNINGNUKEICON+((totalclock>>3)%7), cursorShade, 0, 10);
                }
                else
                    rotatesprite_fs(origin.x + x - entry->format->cursorPosition, origin.y + y_upper + y + (height>>1) - menu->scrollPos, entry->format->cursorScale, 0, SPINNINGNUKEICON+(((totalclock>>3))%7), cursorShade, 0, 10);
            }

            // need these up here to avoid race conditions
            entry->ytop = y_upper + y;
            menu->totalHeight = y + height;
            entry->ybottom = y_upper + menu->totalHeight;

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
                    if (MOUSEACTIVECONDITIONAL(state != 1 && cm == m_currentMenu && M_MouseWithinBounds(&m_mousepos, mousex, mousey, mousewidth, height)))
                    {
                        if (MOUSEWATCHPOINTCONDITIONAL(M_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, height)))
                        {
                            menu->currentEntry = e;
                            M_RunMenuInput_MenuMenu_MovementVerify(menu);
                        }
                    }
                    break;
                case Link:
                    if (MOUSEACTIVECONDITIONAL(state != 1 && cm == m_currentMenu && M_MouseWithinBounds(&m_mousepos, mousex, mousey, mousewidth, height)))
                    {
                        if (MOUSEWATCHPOINTCONDITIONAL(M_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, height)))
                        {
                            menu->currentEntry = e;
                            M_RunMenuInput_MenuMenu_MovementVerify(menu);
                        }

                        if (!m_mousecaught && mousepressstate == Mouse_Released && M_MouseWithinBounds(&m_mousedownpos, mousex, mousey, mousewidth, entry->font->yline))
                        {
                            menu->currentEntry = e;
                            M_RunMenuInput_MenuMenu_MovementVerify(menu);

                            if (entry->flags & Disabled)
                                break;

                            M_RunMenuInput_MenuEntryLink_Activate(entry);

                            if (g_player[myconnectindex].ps->gm&MODE_MENU) // for skill selection
                                S_PlaySound(PISTOL_BODYHIT);

                            m_mousecaught = 1;
                        }
                    }
                    break;
                case Option:
                {
                    MenuOption_t *object = (MenuOption_t*)entry->entry;
                    int32_t currentOption = M_FindOptionBinarySearch(object, object->data == NULL ? M_MenuEntryOptionSource(entry, object->currentOption) : *object->data, 0, object->options->numOptions);

                    if (currentOption >= 0)
                        object->currentOption = currentOption;

                    int32_t optiontextx = origin.x + x;
                    const int32_t optiontexty = origin.y + y_upper + y - menu->scrollPos;

                    const vec2_t optiontextsize = M_MenuText(optiontextx, optiontexty + (height>>1), object->font,
                        currentOption < 0 ? MenuCustom : currentOption < object->options->numOptions ? object->options->optionNames[currentOption] : NULL,
                        status, origin.y + y_upper, origin.y + y_lower);

                    if (entry->format->width > 0)
                        mousewidth += optiontextsize.x;
                    else
                        optiontextx -= optiontextsize.x;

                    if (MOUSEACTIVECONDITIONAL(state != 1 && cm == m_currentMenu && M_MouseWithinBounds(&m_mousepos, mousex, mousey, mousewidth, height)))
                    {
                        if (MOUSEWATCHPOINTCONDITIONAL(M_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, height)))
                        {
                            menu->currentEntry = e;
                            M_RunMenuInput_MenuMenu_MovementVerify(menu);
                        }

                        if (!m_mousecaught && mousepressstate == Mouse_Released && M_MouseWithinBounds(&m_mousedownpos, mousex, mousey, mousewidth, entry->font->yline))
                        {
                            menu->currentEntry = e;
                            M_RunMenuInput_MenuMenu_MovementVerify(menu);

                            if (entry->flags & Disabled)
                                break;

                            M_RunMenuInput_MenuEntryOption_Activate(entry, object);

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

                    const vec2_t column0textsize = M_MenuText(columnx[0], columny + (height>>1), object->font, object->key[*object->column[0]], menu->currentColumn == 0 ? status : (status & ~MT_Selected), origin.y + y_upper, origin.y + y_lower);
                    const vec2_t column1textsize = M_MenuText(columnx[1], columny + (height>>1), object->font, object->key[*object->column[1]], menu->currentColumn == 1 ? status : (status & ~MT_Selected), origin.y + y_upper, origin.y + y_lower);

                    if (entry->format->width > 0)
                        mousewidth += object->columnWidth + column1textsize.x;
                    else
                    {
                        columnx[0] -= column0textsize.x;
                        columnx[1] -= column0textsize.x;
                    }

                    if (MOUSEACTIVECONDITIONAL(state != 1 && cm == m_currentMenu && M_MouseWithinBounds(&m_mousepos, mousex, mousey, mousewidth, height)))
                    {
                        if (MOUSEWATCHPOINTCONDITIONAL(M_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, height)))
                        {
                            menu->currentEntry = e;
                            M_RunMenuInput_MenuMenu_MovementVerify(menu);
                        }

                        if (M_MouseWithinBounds(&m_mousepos, columnx[1], mousey, column1textsize.x, object->font->yline))
                        {
                            if (MOUSEWATCHPOINTCONDITIONAL(M_MouseOutsideBounds(&m_prevmousepos, columnx[1], mousey, column1textsize.x, object->font->yline)))
                            {
                                menu->currentColumn = 1;
                            }

                            if (!m_mousecaught && mousepressstate == Mouse_Released && M_MouseWithinBounds(&m_mousedownpos, columnx[1], mousey, column1textsize.x, object->font->yline))
                            {
                                menu->currentEntry = e;
                                M_RunMenuInput_MenuMenu_MovementVerify(menu);
                                menu->currentColumn = 1;

                                if (entry->flags & Disabled)
                                    break;

                                M_RunMenuInput_MenuEntryCustom2Col_Activate(entry);

                                S_PlaySound(PISTOL_BODYHIT);

                                m_mousecaught = 1;
                            }
                        }
                        else if (M_MouseWithinBounds(&m_mousepos, columnx[0], mousey, column0textsize.x, object->font->yline))
                        {
                            if (MOUSEWATCHPOINTCONDITIONAL(M_MouseOutsideBounds(&m_prevmousepos, columnx[0], mousey, column0textsize.x, object->font->yline)))
                            {
                                menu->currentColumn = 0;
                            }

                            if (!m_mousecaught && mousepressstate == Mouse_Released && M_MouseWithinBounds(&m_mousedownpos, columnx[0], mousey, column0textsize.x, object->font->yline))
                            {
                                menu->currentEntry = e;
                                M_RunMenuInput_MenuMenu_MovementVerify(menu);
                                menu->currentColumn = 0;

                                if (entry->flags & Disabled)
                                    break;

                                M_RunMenuInput_MenuEntryCustom2Col_Activate(entry);

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
                    const int32_t z = entry->format->cursorScale;
                    M_ShadePal(object->font, status, &s, &p);

                    const int32_t slidebarwidth = scale(tilesiz[SLIDEBAR].x<<16, z, 65536);
                    const int32_t slidebarheight = scale(tilesiz[SLIDEBAR].y<<16, z, 65536);

                    if (status & MT_XRight)
                        x -= slidebarwidth;
                    else
                        mousewidth += slidebarwidth;

                    const int32_t slidebarx = origin.x + x;
                    const int32_t slidebary = origin.y + y_upper + y + ((height - slidebarheight)>>1) - menu->scrollPos;

                    rotatesprite_ybounds(slidebarx, slidebary, z, 0, SLIDEBAR, s, (entry->flags & Disabled) ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16, origin.y + y_upper, origin.y + y_lower);

                    const int32_t slideregionwidth = scale((tilesiz[SLIDEBAR].x-2-tilesiz[SLIDEBAR+1].x)<<16, z, 65536);
                    const int32_t slidepointx = slidebarx + (1<<16) + scale(slideregionwidth, *object->variable - object->min, object->max - object->min);
                    const int32_t slidepointy = slidebary + scale((tilesiz[SLIDEBAR].y-tilesiz[SLIDEBAR+1].y)<<15, z, 65536);

                    rotatesprite_ybounds(slidepointx, slidepointy, z, 0, SLIDEBAR+1, s, (entry->flags & Disabled) ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16, origin.y + y_upper, origin.y + y_lower);

                    if (object->flags & DisplayTypeMask)
                    {
                        status |= MT_XRight;

                        if (object->onehundredpercent == 0)
                            object->onehundredpercent = object->max;

                        switch (object->flags & DisplayTypeMask)
                        {
                            case 1:
                                Bsprintf(tempbuf, "%d", *object->variable);
                                break;
                            case 2:
                            {
                                int32_t v;
                                v = Blrintf(((float) *object->variable * 100.f) / (float) object->onehundredpercent);
                                Bsprintf(tempbuf, "%d%%", v);
                                break;
                            }
                            case 3:
                                Bsprintf(tempbuf, "%.2f", (double) *object->variable / (double) object->onehundredpercent);
                                break;
                        }

                        M_MenuText(origin.x + x - (4<<16), origin.y + y_upper + y + (height>>1) - menu->scrollPos, object->font, tempbuf, status, origin.y + y_upper, origin.y + y_lower);
                    }

                    if (MOUSEACTIVECONDITIONAL(state != 1 && cm == m_currentMenu && M_MouseWithinBounds(&m_mousepos, mousex, mousey, mousewidth, height)))
                    {
                        if (MOUSEWATCHPOINTCONDITIONAL(M_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, height)))
                        {
                            menu->currentEntry = e;
                            M_RunMenuInput_MenuMenu_MovementVerify(menu);
                        }

                        if (!m_mousecaught && (mousepressstate == Mouse_Pressed || mousepressstate == Mouse_Held))
                        {
                            const int32_t slidepointhalfwidth = scale((2+tilesiz[SLIDEBAR+1].x)<<15, z, 65536);
                            const int32_t slideregionx = slidebarx + slidepointhalfwidth;

                            menu->currentEntry = e;
                            M_RunMenuInput_MenuMenu_MovementVerify(menu);

                            if (entry->flags & Disabled)
                                break;

                            // region between the x-midline of the slidepoint at the extremes slides proportionally
                            if (M_MouseWithinBounds(&m_mousepos, slideregionx, mousey, slideregionwidth, height))
                            {
                                M_RunMenuInput_MenuEntryRangeInt32_MovementArbitrary(entry, object, Blrintf((float)((object->max - object->min) * (m_mousepos.x - slideregionx)) / slideregionwidth + object->min));

                                m_mousecaught = 1;
                            }
                            // region outside the x-midlines clamps to the extremes
                            else if (M_MouseWithinBounds(&m_mousepos, slidebarx, mousey, slidebarwidth, height))
                            {
                                if (m_mousepos.x > slideregionx + slideregionwidth/2)
                                    M_RunMenuInput_MenuEntryRangeInt32_MovementVerify(entry, object, object->max);
                                else
                                    M_RunMenuInput_MenuEntryRangeInt32_MovementVerify(entry, object, object->min);

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
                    const int32_t z = entry->format->cursorScale;
                    M_ShadePal(object->font, status, &s, &p);

                    const int32_t slidebarwidth = scale(tilesiz[SLIDEBAR].x<<16, z, 65536);
                    const int32_t slidebarheight = scale(tilesiz[SLIDEBAR].y<<16, z, 65536);

                    if (status & MT_XRight)
                        x -= slidebarwidth;
                    else
                        mousewidth += slidebarwidth;

                    const int32_t slidebarx = origin.x + x;
                    const int32_t slidebary = origin.y + y_upper + y + ((height - slidebarheight)>>1) - menu->scrollPos;

                    rotatesprite_ybounds(slidebarx, slidebary, z, 0, SLIDEBAR, s, (entry->flags & Disabled) ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16, origin.y + y_upper, origin.y + y_lower);

                    const int32_t slideregionwidth = scale((tilesiz[SLIDEBAR].x-2-tilesiz[SLIDEBAR+1].x)<<16, z, 65536);
                    const int32_t slidepointx = slidebarx + (1<<16) + (int32_t)((float) slideregionwidth * (*object->variable - object->min) / (object->max - object->min));
                    const int32_t slidepointy = slidebary + scale((tilesiz[SLIDEBAR].y-tilesiz[SLIDEBAR+1].y)<<15, z, 65536);

                    rotatesprite_ybounds(slidepointx, slidepointy, z, 0, SLIDEBAR+1, s, (entry->flags & Disabled) ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16, origin.y + y_upper, origin.y + y_lower);

                    if (object->flags & DisplayTypeMask)
                    {
                        status |= MT_XRight;

                        if (object->onehundredpercent == 0)
                            object->onehundredpercent = 1.f;

                        switch (object->flags & DisplayTypeMask)
                        {
                            case 1:
                                Bsprintf(tempbuf, "%.2f", *object->variable);
                                break;
                            case 2:
                            {
                                int32_t v;
                                v = Blrintf((*object->variable * 100.f) / object->onehundredpercent);
                                Bsprintf(tempbuf, "%d%%", v);
                                break;
                            }
                            case 3:
                                Bsprintf(tempbuf, "%.2f", *object->variable / object->onehundredpercent);
                                break;
                        }

                        M_MenuText(origin.x + x - (4<<16), origin.y + y_upper + y + (height>>1) - menu->scrollPos, object->font, tempbuf, status, origin.y + y_upper, origin.y + y_lower);
                    }

                    if (MOUSEACTIVECONDITIONAL(state != 1 && cm == m_currentMenu && M_MouseWithinBounds(&m_mousepos, mousex, mousey, mousewidth, height)))
                    {
                        if (MOUSEWATCHPOINTCONDITIONAL(M_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, height)))
                        {
                            menu->currentEntry = e;
                            M_RunMenuInput_MenuMenu_MovementVerify(menu);
                        }

                        if (!m_mousecaught && (mousepressstate == Mouse_Pressed || mousepressstate == Mouse_Held))
                        {
                            const int32_t slidepointhalfwidth = scale((2+tilesiz[SLIDEBAR+1].x)<<15, z, 65536);
                            const int32_t slideregionx = slidebarx + slidepointhalfwidth;

                            menu->currentEntry = e;
                            M_RunMenuInput_MenuMenu_MovementVerify(menu);

                            if (entry->flags & Disabled)
                                break;

                            // region between the x-midline of the slidepoint at the extremes slides proportionally
                            if (M_MouseWithinBounds(&m_mousepos, slideregionx, mousey, slideregionwidth, height))
                            {
                                M_RunMenuInput_MenuEntryRangeFloat_MovementArbitrary(entry, object, (object->max - object->min) * (m_mousepos.x - slideregionx) / slideregionwidth + object->min);

                                m_mousecaught = 1;
                            }
                            // region outside the x-midlines clamps to the extremes
                            else if (M_MouseWithinBounds(&m_mousepos, slidebarx, mousey, slidebarwidth, height))
                            {
                                if (m_mousepos.x > slideregionx + slideregionwidth/2)
                                    M_RunMenuInput_MenuEntryRangeFloat_MovementVerify(entry, object, object->max);
                                else
                                    M_RunMenuInput_MenuEntryRangeFloat_MovementVerify(entry, object, object->min);

                                m_mousecaught = 1;
                            }
                        }
                    }

                    break;
                }
                case RangeDouble:
                {
                    MenuRangeDouble_t *object = (MenuRangeDouble_t*)entry->entry;

                    int32_t s, p;
                    const int32_t z = entry->format->cursorScale;
                    M_ShadePal(object->font, status, &s, &p);

                    const int32_t slidebarwidth = scale(tilesiz[SLIDEBAR].x<<16, z, 65536);
                    const int32_t slidebarheight = scale(tilesiz[SLIDEBAR].y<<16, z, 65536);

                    if (status & MT_XRight)
                        x -= slidebarwidth;
                    else
                        mousewidth += slidebarwidth;

                    const int32_t slidebarx = origin.x + x;
                    const int32_t slidebary = origin.y + y_upper + y + ((height - slidebarheight)>>1) - menu->scrollPos;

                    rotatesprite_ybounds(slidebarx, slidebary, z, 0, SLIDEBAR, s, (entry->flags & Disabled) ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16, origin.y + y_upper, origin.y + y_lower);

                    const int32_t slideregionwidth = scale((tilesiz[SLIDEBAR].x-2-tilesiz[SLIDEBAR+1].x)<<16, z, 65536);
                    const int32_t slidepointx = slidebarx + (1<<16) + (int32_t)((double) slideregionwidth * (*object->variable - object->min) / (object->max - object->min));
                    const int32_t slidepointy = slidebary + scale((tilesiz[SLIDEBAR].y-tilesiz[SLIDEBAR+1].y)<<15, z, 65536);

                    rotatesprite_ybounds(slidepointx, slidepointy, z, 0, SLIDEBAR+1, s, (entry->flags & Disabled) ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16, origin.y + y_upper, origin.y + y_lower);

                    if (object->flags & DisplayTypeMask)
                    {
                        status |= MT_XRight;

                        if (object->onehundredpercent == 0)
                            object->onehundredpercent = 1.;

                        switch (object->flags & DisplayTypeMask)
                        {
                            case 1:
                                Bsprintf(tempbuf, "%.2f", *object->variable);
                                break;
                            case 2:
                            {
                                int32_t v;
                                v = Blrintf((*object->variable * 100.) / object->onehundredpercent);
                                Bsprintf(tempbuf, "%d%%", v);
                                break;
                            }
                            case 3:
                                Bsprintf(tempbuf, "%.2f", *object->variable / object->onehundredpercent);
                                break;
                        }

                        M_MenuText(origin.x + x - (4<<16), origin.y + y_upper + y + (height>>1) - menu->scrollPos, object->font, tempbuf, status, origin.y + y_upper, origin.y + y_lower);
                    }

                    if (MOUSEACTIVECONDITIONAL(state != 1 && cm == m_currentMenu && M_MouseWithinBounds(&m_mousepos, mousex, mousey, mousewidth, height)))
                    {
                        if (MOUSEWATCHPOINTCONDITIONAL(M_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, height)))
                        {
                            menu->currentEntry = e;
                            M_RunMenuInput_MenuMenu_MovementVerify(menu);
                        }

                        if (!m_mousecaught && (mousepressstate == Mouse_Pressed || mousepressstate == Mouse_Held))
                        {
                            const int32_t slidepointhalfwidth = scale((2+tilesiz[SLIDEBAR+1].x)<<15, z, 65536);
                            const int32_t slideregionx = slidebarx + slidepointhalfwidth;

                            menu->currentEntry = e;
                            M_RunMenuInput_MenuMenu_MovementVerify(menu);

                            if (entry->flags & Disabled)
                                break;

                            // region between the x-midline of the slidepoint at the extremes slides proportionally
                            if (M_MouseWithinBounds(&m_mousepos, slideregionx, mousey, slideregionwidth, height))
                            {
                                M_RunMenuInput_MenuEntryRangeDouble_MovementArbitrary(/*entry, */object, (object->max - object->min) * (m_mousepos.x - slideregionx) / slideregionwidth + object->min);

                                m_mousecaught = 1;
                            }
                            // region outside the x-midlines clamps to the extremes
                            else if (M_MouseWithinBounds(&m_mousepos, slidebarx, mousey, slidebarwidth, height))
                            {
                                if (m_mousepos.x > slideregionx + slideregionwidth/2)
                                    M_RunMenuInput_MenuEntryRangeDouble_MovementVerify(/*entry, */object, object->max);
                                else
                                    M_RunMenuInput_MenuEntryRangeDouble_MovementVerify(/*entry, */object, object->min);

                                m_mousecaught = 1;
                            }
                        }
                    }

                    break;
                }
                case String:
                {
                    MenuString_t *object = (MenuString_t*)entry->entry;

                    vec2_t dim;
                    int32_t stringx = x;
                    const int32_t stringy = origin.y + y_upper + y + (height>>1) - menu->scrollPos;
                    int32_t h;

                    if (entry == currentry && object->editfield != NULL)
                    {
                        dim = M_MenuText(origin.x + stringx, stringy, object->font, object->editfield, status | MT_Literal, origin.y + y_upper, origin.y + y_lower);
                        h = max(dim.y, entry->font->yline);

                        rotatesprite_ybounds(origin.x + x + dim.x + (1<<16) + scale(tilesiz[SPINNINGNUKEICON].x<<15, h, tilesiz[SPINNINGNUKEICON].y<<16), stringy, scale(65536, h, tilesiz[SPINNINGNUKEICON].y<<16), 0, SPINNINGNUKEICON+(((totalclock>>3))%7), cursorShade, 0, 10, origin.y + y_upper, origin.y + y_lower);
                    }
                    else
                    {
                        dim = M_MenuText(origin.x + stringx, stringy, object->font, object->variable, status, origin.y + y_upper, origin.y + y_lower);
                        h = max(dim.y, entry->font->yline);
                    }

                    if (entry->format->width > 0)
                    {
                        if (entry->name)
                            mousewidth += dim.x;
                    }
                    else
                        stringx -= dim.x;

                    if (MOUSEACTIVECONDITIONAL(cm == m_currentMenu && M_MouseWithinBounds(&m_mousepos, mousex, mousey, mousewidth, h)))
                    {
                        if (state != 1 && M_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, h))
                        {
                            menu->currentEntry = e;
                            M_RunMenuInput_MenuMenu_MovementVerify(menu);
                        }

#ifndef EDUKE32_TOUCH_DEVICES
                        if (!m_mousecaught && mousepressstate == Mouse_Released && M_MouseWithinBounds(&m_mousepos, mousex, mousey, mousewidth, h) && M_MouseWithinBounds(&m_mousedownpos, mousex, mousey, mousewidth, h))
#endif
                        {
                            if (entry == currentry && object->editfield != NULL)
                            {
                                M_RunMenuInput_MenuEntryString_Submit(entry, object);

                                S_PlaySound(PISTOL_BODYHIT);

                                m_mousecaught = 1;
                            }
                            else if (state != 1)
                            {
                                menu->currentEntry = e;
                                M_RunMenuInput_MenuMenu_MovementVerify(menu);

                                if (entry->flags & Disabled)
                                    break;

                                M_RunMenuInput_MenuEntryString_Activate(entry);

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
            y += (!calculatedentryspacing || calculatedentryspacing > entry->format->marginBottom) ? entry->format->marginBottom : calculatedentryspacing;
        }

        // draw indicators if applicable
        M_RunMenu_Scrollbar(cm, menu->format, y_upper + menu->totalHeight, &menu->scrollPos, 320<<16, origin);
    }

    return menu->totalHeight;
}

static void M_RunMenu_MenuOptionList(Menu_t *cm, MenuEntry_t *entry, MenuOption_t *object, const vec2_t origin)
{
    const int32_t cursorShade = 4-(sintable[(totalclock<<4)&2047]>>11);
    int32_t e, y = 0;
    const int32_t y_upper = object->options->menuFormat->pos.y;
    const int32_t y_lower = object->options->menuFormat->bottomcutoff;
    int32_t calculatedentryspacing = object->options->entryFormat->marginBottom;

    // assumes height == font->yline!
    if (calculatedentryspacing < 0)
        calculatedentryspacing = (-calculatedentryspacing - object->options->font->yline) / (object->options->numOptions - 1) - object->options->font->yline;

    for (e = 0; e < object->options->numOptions; ++e)
    {
        uint8_t status = 0;
        int32_t height, x;
        vec2_t textsize;

        x = object->options->menuFormat->pos.x;

        if (e == object->options->currentEntry)
            status |= MT_Selected;
        if (object->options->entryFormat->width == 0)
            status |= MT_XCenter;

        const int32_t dodraw = 0 <= y - object->options->scrollPos + object->options->font->yline && y - object->options->scrollPos <= object->options->menuFormat->bottomcutoff - object->options->menuFormat->pos.y;

        if (dodraw)
            textsize = M_MenuText(origin.x + x, origin.y + y_upper + y - object->options->scrollPos, object->options->font, object->options->optionNames[e], status, origin.y + y_upper, origin.y + y_lower);

        height = object->options->font->yline; // max(textsize.y, object->options->font->yline);

        if (object->options->entryFormat->width < 0)
            status |= MT_XRight;
        status |= MT_YCenter;

        if (dodraw && (status & MT_Selected))
        {
            if (status & MT_XCenter)
            {
                rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16) + object->options->entryFormat->cursorPosition, origin.y + y_upper + y + (height>>1) - object->options->scrollPos, object->options->entryFormat->cursorScale, 0, SPINNINGNUKEICON+6-((6+(totalclock>>3))%7), cursorShade, 0, 10);
                rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16) - object->options->entryFormat->cursorPosition, origin.y + y_upper + y + (height>>1) - object->options->scrollPos, object->options->entryFormat->cursorScale, 0, SPINNINGNUKEICON+((totalclock>>3)%7), cursorShade, 0, 10);
            }
            else
                rotatesprite_fs(origin.x + x - object->options->entryFormat->cursorPosition, origin.y + y_upper + y + (height>>1) - object->options->scrollPos, object->options->entryFormat->cursorScale, 0, SPINNINGNUKEICON+(((totalclock>>3))%7), cursorShade, 0, 10);
        }

        if (dodraw)
        {
            const int32_t mousex = origin.x + object->options->entryFormat->width == 0 ? x - textsize.x/2 : x;
            const int32_t mousey = origin.y + y_upper + y - object->options->scrollPos;
            const int32_t mousewidth = object->options->entryFormat->width == 0 ? textsize.x : klabs(object->options->entryFormat->width);

            if (MOUSEACTIVECONDITIONAL(cm == m_currentMenu && M_MouseWithinBounds(&m_mousepos, mousex, mousey, mousewidth, object->options->font->yline)))
            {
                if (MOUSEWATCHPOINTCONDITIONAL(M_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, mousewidth, object->options->font->yline)))
                {
                    object->options->currentEntry = e;
                }

                if (!m_mousecaught && mousepressstate == Mouse_Released && M_MouseWithinBounds(&m_mousedownpos, mousex, mousey, mousewidth, object->options->font->yline))
                {
                    object->options->currentEntry = e;

                    if (!M_RunMenuInput_MenuEntryOptionList_Activate(entry, object))
                        S_PlaySound(PISTOL_BODYHIT);

                    m_mousecaught = 1;
                }
            }
        }

        // prepare for the next line
        y += height;
        y += calculatedentryspacing;
    }

    y -= calculatedentryspacing;

    // draw indicators if applicable
    M_RunMenu_Scrollbar(cm, object->options->menuFormat, y_upper + y, &object->options->scrollPos, 320<<16, origin);
}

static int32_t M_RunMenuInput_MouseAdvance(void)
{
    return MOUSEACTIVECONDITIONAL(!m_mousecaught && mousepressstate == Mouse_Released);
}

static int32_t M_RunMenuInput_MouseReturn_status;

static void M_RunMenu_MouseReturn(Menu_t *cm, const vec2_t origin)
{
#if !defined EDUKE32_TOUCH_DEVICES
    if (!MOUSEACTIVECONDITION)
        return;
#endif

    if (cm->menuID == MENU_MAIN)
        return;

    rotatesprite_(origin.x + (tilesiz[SELECTDIR].y << 16), 0, 65536, 512, SELECTDIR,
                  M_RunMenuInput_MouseReturn_status ? 4 - (sintable[(totalclock << 4) & 2047] >> 11) : 6, 0,
                  2 | 8 | 16 | RS_ALIGN_L, MOUSEALPHA, 0, xdim_from_320_16(origin.x + x_widescreen_left()), 0,
                  xdim_from_320_16(origin.x + x_widescreen_left() + (tilesiz[SELECTDIR].y << 15)), ydim - 1);
}

static int32_t M_RunMenuInput_MouseReturn(void)
{
#if !defined EDUKE32_TOUCH_DEVICES
    if (!MOUSEACTIVECONDITION)
    {
        M_RunMenuInput_MouseReturn_status = 0;
        return 0;
    }
#endif

    if (g_currentMenu == MENU_MAIN)
        return 0;

    const int32_t MouseReturnRegionX = x_widescreen_left();

    if (M_MouseWithinBounds(&m_mousepos, MouseReturnRegionX, 0, tilesiz[SELECTDIR].y<<15, tilesiz[SELECTDIR].x<<16))
    {
#if !defined EDUKE32_TOUCH_DEVICES
        M_RunMenuInput_MouseReturn_status = 1;
#else
        M_RunMenuInput_MouseReturn_status = (mousepressstate == Mouse_Pressed || mousepressstate == Mouse_Held);
#endif

        return !m_mousecaught && mousepressstate == Mouse_Released && M_MouseWithinBounds(&m_mousedownpos, MouseReturnRegionX, 0, tilesiz[SELECTDIR].y<<15, tilesiz[SELECTDIR].x<<16);
    }

    M_RunMenuInput_MouseReturn_status = 0;

    return 0;
}

static void M_RunMenu_AbbreviateNameIntoBuffer(const char* name, int32_t entrylength)
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

static void M_RunMenuRecurse(MenuID_t cm, const vec2_t origin)
{
    switch (cm)
    {
    case MENU_LOADVERIFY:
    case MENU_SAVEVERIFY:
    case MENU_ADULTPASSWORD:
    case MENU_CHEATENTRY:
    case MENU_CHEAT_WARP:
    case MENU_CHEAT_SKILL:
        M_RunMenu(m_previousMenu, origin);
        break;
    default:
        break;
    }
}

static void M_RunMenu(Menu_t *cm, const vec2_t origin)
{
    const int32_t cursorShade = 4-(sintable[(totalclock<<4)&2047]>>11);

    M_RunMenuRecurse(cm->menuID, origin);

    switch (cm->type)
    {
        case Verify:
        {
            MenuVerify_t *object = (MenuVerify_t*)cm->object;

            M_PreMenu(cm->menuID);

            M_PreMenuDrawBackground(cm->menuID, origin);

            M_PreMenuDraw(cm->menuID, NULL, origin);

            rotatesprite_fs(origin.x + object->cursorpos.x, origin.y + object->cursorpos.y, 65536, 0, SPINNINGNUKEICON + (((totalclock >> 3)) % 7), cursorShade, 0, 10);

            break;
        }

        case Message:
        {
            MenuMessage_t *object = (MenuMessage_t*)cm->object;

            M_PreMenu(cm->menuID);

            M_PreMenuDrawBackground(cm->menuID, origin);

            M_PreMenuDraw(cm->menuID, NULL, origin);

            rotatesprite_fs(origin.x + object->cursorpos.x, origin.y + object->cursorpos.y, 65536, 0, SPINNINGNUKEICON + (((totalclock >> 3)) % 7), cursorShade, 0, 10);

            break;
        }

        case TextForm:
        {
            MenuTextForm_t *object = (MenuTextForm_t*)cm->object;

            M_PreMenu(cm->menuID);

            M_PreMenuDrawBackground(cm->menuID, origin);

            M_BlackRectangle(origin.x + (60<<16), origin.y + (86<<16), 200<<16, 28<<16, 0);

            G_ScreenText(STARTALPHANUM, origin.x + (160<<16), origin.y + (98<<16), 65536, 0, 0, object->instructions, 0, 0, 2|8|16|ROTATESPRITE_FULL16, 0, 5<<16, 8<<16, -1<<16, 1<<16, TEXT_XCENTER|TEXT_YBOTTOM, 0, 0, xdim-1, ydim-1);

            const char *displaytext = object->input;

            if (object->flags & MTF_Password)
            {
                size_t x;
                for (x = 0; x < Bstrlen(object->input); ++x)
                    tempbuf[x] = '*';
                tempbuf[x] = 0;

                displaytext = tempbuf;
            }

            const vec2_t textreturn = G_ScreenText(STARTALPHANUM, origin.x + (160<<16), origin.y + (102<<16), 65536, 0, 0, displaytext, 0, 0, 2|8|16|ROTATESPRITE_FULL16, 0, 5<<16, 8<<16, -1<<16, 1<<16, TEXT_XCENTER, 0, 0, xdim-1, ydim-1);

            M_PreMenuDraw(cm->menuID, NULL, origin);

            rotatesprite_fs(origin.x + (168<<16) + (textreturn.x>>1), origin.y + (106<<16), 32768, 0, SPINNINGNUKEICON + ((totalclock >> 3) % 7), cursorShade, 0, 2 | 8);

            break;
        }

        case FileSelect:
        {
            MenuFileSelect_t *object = (MenuFileSelect_t*)cm->object;
            const int32_t MenuFileSelect_scrollbar_rightedge[2] = { 160<<16, 284<<16 };
            int32_t i, selected = 0;

            M_PreMenu(cm->menuID);

            M_PreMenuDrawBackground(cm->menuID, origin);

            if (object->title != NoTitle)
                M_DrawTopBar(origin);


            // black translucent background underneath file lists
            M_BlackRectangle(origin.x + (36<<16), origin.y + (42<<16), 248<<16, 123<<16, 1|32);

            // path
            Bsnprintf(tempbuf, sizeof(tempbuf), "Path: %s", object->destination);
            M_MenuText(origin.x + object->format[0]->pos.x, origin.y + (32<<16), &MF_Bluefont, tempbuf, 0, 0, 200<<16);

            for (i = 0; i < 2; ++i)
            {
                if (object->findhigh[i])
                {
                    CACHE1D_FIND_REC *dir;
                    int32_t y = 0;
                    const int32_t y_upper = object->format[i]->pos.y;
                    const int32_t y_lower = klabs(object->format[i]->bottomcutoff);

                    for (dir = object->findhigh[i]->usera; dir; dir = dir->next)
                    {
                        uint8_t status = (dir == object->findhigh[i] && object->currentList == i) ? MT_Selected : 0;

                        // pal = dir->source==CACHE1D_SOURCE_ZIP ? 8 : 2

                        M_RunMenu_AbbreviateNameIntoBuffer(dir->name, USERMAPENTRYLENGTH);

                        const int32_t thisx = object->format[i]->pos.x;
                        const int32_t thisy = y - object->scrollPos[i];

                        if (0 <= thisy + object->font[i]->yline && thisy <= klabs(object->format[i]->bottomcutoff) - object->format[i]->pos.y)
                        {
                            const int32_t mousex = origin.x + thisx;
                            const int32_t mousey = origin.y + y_upper + thisy;

                            vec2_t textdim = M_MenuText(mousex, mousey, object->font[i], tempbuf, status, origin.y + y_upper, origin.y + y_lower);

                            if (MOUSEACTIVECONDITIONAL(cm == m_currentMenu && M_MouseWithinBounds(&m_mousepos, mousex, mousey, textdim.x, object->font[i]->yline)))
                            {
                                if (MOUSEWATCHPOINTCONDITIONAL(M_MouseOutsideBounds(&m_prevmousepos, mousex, mousey, textdim.x, object->font[i]->yline)))
                                {
                                    object->findhigh[i] = dir;
                                    object->currentList = i;

                                    M_RunMenuInput_FileSelect_MovementVerify(object);
                                }

                                if (!m_mousecaught && mousepressstate == Mouse_Released && M_MouseWithinBounds(&m_mousedownpos, mousex, mousey, textdim.x, object->font[i]->yline))
                                {
                                    object->findhigh[i] = dir;
                                    object->currentList = i;

                                    M_RunMenuInput_FileSelect_MovementVerify(object);

                                    m_mousecaught = 1;
                                    selected = 1;
                                }
                            }
                        }

                        y += object->font[i]->yline + object->marginBottom[i];
                    }

                    y -= object->marginBottom[i];

                    M_RunMenu_Scrollbar(cm, object->format[i], y_upper + y, &object->scrollPos[i], MenuFileSelect_scrollbar_rightedge[i], origin);
                }
            }

            M_PreMenuDraw(cm->menuID, NULL, origin);

            if (object->title != NoTitle)
                M_DrawTopBarCaption(object->title, origin);

            if (selected)
            {
                M_RunMenuInput_FileSelect_Select(object);

                S_PlaySound(PISTOL_BODYHIT);

                m_mousecaught = 1;
            }

            break;
        }

        case Panel:
        {
            MenuPanel_t *object = (MenuPanel_t*)cm->object;

            M_PreMenu(cm->menuID);

            M_PreMenuDrawBackground(cm->menuID, origin);

            if (object->title != NoTitle)
                M_DrawTopBar(origin);

            M_PreMenuDraw(cm->menuID, NULL, origin);

            if (object->title != NoTitle)
                M_DrawTopBarCaption(object->title, origin);

            break;
        }

        case Menu:
        {
            int32_t state;

            MenuMenu_t *menu = (MenuMenu_t*)cm->object;
            MenuEntry_t *currentry = menu->entrylist[menu->currentEntry];

            state = M_DetermineMenuSpecialState(currentry);

            if (state != 2)
            {
                M_PreMenu(cm->menuID);

                M_PreMenuDrawBackground(cm->menuID, origin);

                if (menu->title != NoTitle)
                    M_DrawTopBar(origin);

                M_PreMenuDraw(cm->menuID, currentry, origin);

                M_RunMenu_MenuMenu(cm, menu, currentry, state, origin);
            }
            else
            {
                M_PreMenuDrawBackground(cm->menuID, origin);

                if (menu->title != NoTitle)
                    M_DrawTopBar(origin);

                if (currentry->type == Option)
                {
                    if (currentry->name)
                        M_DrawTopBarCaption(currentry->name, origin);

                    M_PreMenuOptionListDraw(currentry, origin);

                    M_RunMenu_MenuOptionList(cm, currentry, (MenuOption_t*)currentry->entry, origin);
                }
                else if (currentry->type == Custom2Col)
                {
                    M_PreMenuCustom2ColScreenDraw(currentry, origin);
                }
            }

            if ((currentry->type != Option || state != 2) && menu->title != NoTitle)
                M_DrawTopBarCaption(menu->title, origin);

            break;
        }
    }

    M_RunMenu_MouseReturn(cm, origin);
}

/*
Note: When menus are exposed to scripting, care will need to be taken so that
a user cannot define an empty MenuEntryList, or one containing only spacers,
or else this function will recurse infinitely.
*/
static MenuEntry_t *M_RunMenuInput_MenuMenu_MovementVerify(MenuMenu_t *menu)
{
    MenuEntry_t *currentry = menu->entrylist[menu->currentEntry];

    M_MenuEntryFocus(/*currentry*/);

    if (currentry->ybottom - menu->scrollPos > klabs(menu->format->bottomcutoff))
        menu->scrollPos = currentry->ybottom - klabs(menu->format->bottomcutoff);
    else if (currentry->ytop - menu->scrollPos < menu->format->pos.y)
        menu->scrollPos = currentry->ytop - menu->format->pos.y;

    return currentry;
}

static MenuEntry_t *M_RunMenuInput_MenuMenu_Movement(MenuMenu_t *menu, MenuMovement_t direction)
{
    switch (direction)
    {
        case MM_End:
            menu->currentEntry = menu->numEntries;
        case MM_Up:
                do
                {
                    --menu->currentEntry;
                    if (menu->currentEntry < 0)
                        return M_RunMenuInput_MenuMenu_Movement(menu, MM_End);
                }
                while (!menu->entrylist[menu->currentEntry] || menu->entrylist[menu->currentEntry]->type == Spacer);
            break;

        case MM_Home:
            menu->currentEntry = -1;
        case MM_Down:
                do
                {
                    ++menu->currentEntry;
                    if (menu->currentEntry >= menu->numEntries)
                        return M_RunMenuInput_MenuMenu_Movement(menu, MM_Home);
                }
                while (!menu->entrylist[menu->currentEntry] || menu->entrylist[menu->currentEntry]->type == Spacer);
            break;

        case MM_Swap:
            menu->currentColumn = !menu->currentColumn;
            break;

        default:
            break;
    }

    return M_RunMenuInput_MenuMenu_MovementVerify(menu);
}

static void M_RunMenuInput_MenuEntryLink_Activate(MenuEntry_t *entry)
{
    MenuLink_t *link = (MenuLink_t*)entry->entry;

    M_MenuEntryLinkActivate(entry);

    M_ChangeMenuAnimate(link->linkID, link->animation);
}

static void M_RunMenuInput_MenuEntryOptionList_MovementVerify(MenuOption_t *object)
{
    const int32_t listytop = object->options->menuFormat->pos.y;
    // assumes height == font->yline!
    const int32_t unitheight = object->options->entryFormat->marginBottom < 0 ? (-object->options->entryFormat->marginBottom - object->options->font->yline) / object->options->numOptions : (object->options->font->yline + object->options->entryFormat->marginBottom);
    const int32_t ytop = listytop + object->options->currentEntry * unitheight;
    const int32_t ybottom = ytop + object->options->font->yline;

    if (ybottom - object->options->scrollPos > object->options->menuFormat->bottomcutoff)
        object->options->scrollPos = ybottom - object->options->menuFormat->bottomcutoff;
    else if (ytop - object->options->scrollPos < listytop)
        object->options->scrollPos = ytop - listytop;
}

static void M_RunMenuInput_MenuEntryOptionList_Movement(MenuOption_t *object, MenuMovement_t direction)
{
    switch (direction)
    {
        case MM_Up:
            --object->options->currentEntry;
            if (object->options->currentEntry >= 0)
                break;
        case MM_End:
            object->options->currentEntry = object->options->numOptions-1;
            break;

        case MM_Down:
            ++object->options->currentEntry;
            if (object->options->currentEntry < object->options->numOptions)
                break;
        case MM_Home:
            object->options->currentEntry = 0;
            break;

        default:
            break;
    }

    M_RunMenuInput_MenuEntryOptionList_MovementVerify(object);
}

static int32_t M_RunMenuInput_MenuEntryOption_Modify(MenuEntry_t *entry, MenuOption_t *object, int32_t newValueIndex)
{
    int32_t newValue = (object->options->optionValues == NULL) ? newValueIndex : object->options->optionValues[newValueIndex];
    if (!M_MenuEntryOptionModify(entry, newValue))
    {
        object->currentOption = newValueIndex;

        if ((int32_t*)object->data != NULL) // NULL implies the functions will handle it
            *((int32_t*)object->data) = newValue;

        M_MenuEntryOptionDidModify(entry);

        return 0;
    }

    return -1;
}

static int32_t M_RunMenuInput_MenuEntryOption_Movement(MenuEntry_t *entry, MenuOption_t *object, MenuMovement_t direction)
{
    int32_t newValueIndex = object->currentOption;

    switch (direction)
    {
        case MM_Left:
            --newValueIndex;
            if (newValueIndex >= 0)
                break;
        case MM_AllTheWayRight:
            newValueIndex = object->options->numOptions-1;
            break;

        case MM_Right:
            ++newValueIndex;
            if (newValueIndex < object->options->numOptions)
                break;
       case MM_AllTheWayLeft:
            newValueIndex = 0;
            break;

        default:
            break;
    }

    return M_RunMenuInput_MenuEntryOption_Modify(entry, object, newValueIndex);
}

static int32_t M_RunMenuInput_MenuEntryOption_Activate(MenuEntry_t *entry, MenuOption_t *object)
{
    if (object->options->features & 2)
        return M_RunMenuInput_MenuEntryOption_Movement(entry, object, MM_Right);
    else
    {
        object->options->currentEntry = object->currentOption >= 0 ? object->currentOption : 0;
        M_RunMenuInput_MenuEntryOptionList_MovementVerify(object);

        return 0;
    }
}

static int32_t M_RunMenuInput_MenuEntryOptionList_Activate(MenuEntry_t *entry, MenuOption_t *object)
{
    if (!M_RunMenuInput_MenuEntryOption_Modify(entry, object, object->options->currentEntry))
    {
        object->options->currentEntry = -1;

        return 0;
    }

    return -1;
}

static void M_RunMenuInput_MenuEntryCustom2Col_Activate(MenuEntry_t *entry)
{
    MenuCustom2Col_t *object = (MenuCustom2Col_t*)entry->entry;

    M_MenuCustom2ColScreen(/*entry*/);

    object->screenOpen = 1;
}

static void M_RunMenuInput_MenuEntryRangeInt32_MovementVerify(MenuEntry_t *entry, MenuRangeInt32_t *object, int32_t newValue)
{
    if (!M_MenuEntryRangeInt32Modify(entry, newValue))
        *object->variable = newValue;
}

static void M_RunMenuInput_MenuEntryRangeInt32_MovementArbitrary(MenuEntry_t *entry, MenuRangeInt32_t *object, int32_t newValue)
{
    if (object->flags & EnforceIntervals)
    {
        const float interval = ((float) (object->max - object->min)) / (object->steps - 1);
        const float currentPos = (float) (newValue - object->min) / interval;
        const int32_t newValueIndex = Blrintf(currentPos);
        newValue = Blrintf(interval * newValueIndex + object->min);
    }

    M_RunMenuInput_MenuEntryRangeInt32_MovementVerify(entry, object, newValue);
}

static void M_RunMenuInput_MenuEntryRangeInt32_Movement(MenuEntry_t *entry, MenuRangeInt32_t *object, MenuMovement_t direction)
{
    const float interval = ((float) (object->max - object->min)) / (object->steps - 1);
    const float currentPos = (float) (*object->variable - object->min) / interval;
    int32_t newValueIndex = Blrintf(currentPos);
    const int32_t newValueProjected = Blrintf(interval * newValueIndex + object->min);

    switch (direction)
    {
        case MM_Left:
            if (newValueIndex >= currentPos || *object->variable == newValueProjected)
                --newValueIndex;
            if (newValueIndex >= 0)
                break;
        case MM_AllTheWayLeft:
            newValueIndex = 0;
            break;

        case MM_Right:
            if (newValueIndex <= currentPos || *object->variable == newValueProjected)
                ++newValueIndex;
            if (newValueIndex < object->steps)
                break;
        case MM_AllTheWayRight:
            newValueIndex = object->steps-1;
            break;

        default:
            break;
    }

    M_RunMenuInput_MenuEntryRangeInt32_MovementVerify(entry, object, Blrintf(interval * newValueIndex + object->min));
}

static void M_RunMenuInput_MenuEntryRangeFloat_MovementVerify(MenuEntry_t *entry, MenuRangeFloat_t *object, float newValue)
{
    if (!M_MenuEntryRangeFloatModify(entry, newValue))
    {
        *object->variable = newValue;
        M_MenuEntryRangeFloatDidModify(entry);
    }
}

static void M_RunMenuInput_MenuEntryRangeFloat_MovementArbitrary(MenuEntry_t *entry, MenuRangeFloat_t *object, float newValue)
{
    if (object->flags & EnforceIntervals)
    {
        const float interval = (object->max - object->min) / (object->steps - 1);
        const float currentPos = (newValue - object->min) / interval;
        const int32_t newValueIndex = Blrintf(currentPos);
        newValue = interval * newValueIndex + object->min;
    }

    M_RunMenuInput_MenuEntryRangeFloat_MovementVerify(entry, object, newValue);
}

static void M_RunMenuInput_MenuEntryRangeFloat_Movement(MenuEntry_t *entry, MenuRangeFloat_t *object, MenuMovement_t direction)
{
    const float interval = (object->max - object->min) / (object->steps - 1);
    const float currentPos = (*object->variable - object->min) / interval;
    int32_t newValueIndex = Blrintf(currentPos);
    const float newValueProjected = interval * newValueIndex + object->min;

    switch (direction)
    {
        case MM_Left:
            if (newValueIndex >= currentPos || *object->variable == newValueProjected)
                --newValueIndex;
            if (newValueIndex >= 0)
                break;
        case MM_AllTheWayLeft:
            newValueIndex = 0;
            break;

        case MM_Right:
            if (newValueIndex <= currentPos || *object->variable == newValueProjected)
                ++newValueIndex;
            if (newValueIndex < object->steps)
                break;
        case MM_AllTheWayRight:
            newValueIndex = object->steps-1;
            break;

        default:
            break;
    }

    M_RunMenuInput_MenuEntryRangeFloat_MovementVerify(entry, object, interval * newValueIndex + object->min);
}

static void M_RunMenuInput_MenuEntryRangeDouble_MovementVerify(/*MenuEntry_t *entry, */MenuRangeDouble_t *object, double newValue)
{
    if (!M_MenuEntryRangeDoubleModify(/*entry, newValue*/))
        *object->variable = newValue;
}

static void M_RunMenuInput_MenuEntryRangeDouble_MovementArbitrary(/*MenuEntry_t *entry, */MenuRangeDouble_t *object, double newValue)
{
    if (object->flags & EnforceIntervals)
    {
        const double interval = (object->max - object->min) / (object->steps - 1);
        const double currentPos = (newValue - object->min) / interval;
        const int32_t newValueIndex = Blrintf(currentPos);
        newValue = interval * newValueIndex + object->min;
    }

    M_RunMenuInput_MenuEntryRangeDouble_MovementVerify(/*entry, */object, newValue);
}

static void M_RunMenuInput_MenuEntryRangeDouble_Movement(/*MenuEntry_t *entry, */MenuRangeDouble_t *object, MenuMovement_t direction)
{
    const double interval = (object->max - object->min) / (object->steps - 1);
    const double currentPos = (*object->variable - object->min) / interval;
    int32_t newValueIndex = Blrintf(currentPos);
    const double newValueProjected = interval * newValueIndex + object->min;

    switch (direction)
    {
        case MM_Left:
            if (newValueIndex >= currentPos || *object->variable == newValueProjected)
                --newValueIndex;
            if (newValueIndex >= 0)
                break;
        case MM_AllTheWayLeft:
            newValueIndex = 0;
            break;

        case MM_Right:
            if (newValueIndex <= currentPos || *object->variable == newValueProjected)
                ++newValueIndex;
            if (newValueIndex < object->steps)
                break;
        case MM_AllTheWayRight:
            newValueIndex = object->steps-1;
            break;

        default:
            break;
    }

    M_RunMenuInput_MenuEntryRangeDouble_MovementVerify(/*entry, */object, interval * newValueIndex + object->min);
}

static void M_RunMenuInput_MenuEntryString_Activate(MenuEntry_t *entry)
{
    MenuString_t *object = (MenuString_t*)entry->entry;

    Bstrncpy(typebuf, object->variable, TYPEBUFSIZE);
    object->editfield = typebuf;

    // this limitation is an arbitrary implementation detail
    if (object->maxlength > TYPEBUFSIZE)
        object->maxlength = TYPEBUFSIZE;

    M_MenuEntryStringActivate(/*entry*/);
    WithSDL2_StartTextInput();
}

static void M_RunMenuInput_MenuEntryString_Submit(MenuEntry_t *entry, MenuString_t *object)
{
    if (!M_MenuEntryStringSubmit(entry, object->editfield))
        Bstrncpy(object->variable, object->editfield, object->maxlength);

    object->editfield = NULL;
    WithSDL2_StopTextInput();
}

static void M_RunMenuInput_MenuEntryString_Cancel(/*MenuEntry_t *entry, */MenuString_t *object)
{
    M_MenuEntryStringCancel(/*entry*/);

    object->editfield = NULL;
    WithSDL2_StopTextInput();
}

static void M_RunMenuInput_FileSelect_MovementVerify(MenuFileSelect_t *object)
{
    const int32_t listytop = object->format[object->currentList]->pos.y;
    const int32_t listybottom = klabs(object->format[object->currentList]->bottomcutoff);
    const int32_t ytop = listytop + object->findhigh[object->currentList]->type * (object->font[object->currentList]->yline + object->marginBottom[object->currentList]);
    const int32_t ybottom = ytop + object->font[object->currentList]->yline;

    if (ybottom - object->scrollPos[object->currentList] > listybottom)
        object->scrollPos[object->currentList] = ybottom - listybottom;
    else if (ytop - object->scrollPos[object->currentList] < listytop)
        object->scrollPos[object->currentList] = ytop - listytop;
}

static void M_RunMenuInput_FileSelect_Movement(MenuFileSelect_t *object, MenuMovement_t direction)
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
        case MM_Home:
            object->findhigh[object->currentList] = object->findhigh[object->currentList]->usera;
            break;

        case MM_Swap:
            object->currentList = !object->currentList;
            break;

        default:
            break;
    }

    M_RunMenuInput_FileSelect_MovementVerify(object);
}

static void M_RunMenuInput_FileSelect_Select(MenuFileSelect_t *object)
{
    if (!object->findhigh[object->currentList])
        return;

    Bstrcat(object->destination, object->findhigh[object->currentList]->name);

    if (object->currentList == 0)
    {
        Bstrcat(object->destination, "/");
        Bcorrectfilename(object->destination, 1);

        M_MenuFileSelectInit(object);
    }
    else
    {
        M_MenuFileSelect(1);
    }
}

static void M_RunMenuInput(Menu_t *cm)
{
    switch (cm->type)
    {
        case Panel:
        {
            MenuPanel_t *panel = (MenuPanel_t*)cm->object;

            if (I_ReturnTrigger() || M_RunMenuInput_MouseReturn())
            {
                I_ReturnTriggerClear();
                m_mousecaught = 1;

                S_PlaySound(EXITMENUSOUND);

                M_ChangeMenuAnimate(cm->parentID, cm->parentAnimation);
            }
            else if (I_PanelUp())
            {
                I_PanelUpClear();

                S_PlaySound(KICK_HIT);
                M_ChangeMenuAnimate(panel->previousID, panel->previousAnimation);
            }
            else if (I_PanelDown() || M_RunMenuInput_MouseAdvance())
            {
                I_PanelDownClear();
                m_mousecaught = 1;

                S_PlaySound(KICK_HIT);
                M_ChangeMenuAnimate(panel->nextID, panel->nextAnimation);
            }
            break;
        }

        case TextForm:
        {
            MenuTextForm_t *object = (MenuTextForm_t*)cm->object;
            int32_t hitstate = I_EnterText(object->input, object->maxlength, 0);

            if (hitstate == -1 || M_RunMenuInput_MouseReturn())
            {
                m_mousecaught = 1;

                S_PlaySound(EXITMENUSOUND);

                object->input = NULL;

                M_ChangeMenuAnimate(cm->parentID, cm->parentAnimation);
                WithSDL2_StopTextInput();
            }
            else if (hitstate == 1 || M_RunMenuInput_MouseAdvance())
            {
                m_mousecaught = 1;

                M_MenuTextFormSubmit(object->input);

                object->input = NULL;
                WithSDL2_StopTextInput();
            }
            break;
        }

        case FileSelect:
        {
            MenuFileSelect_t *object = (MenuFileSelect_t*)cm->object;

            if (I_ReturnTrigger() || M_RunMenuInput_MouseReturn())
            {
                I_ReturnTriggerClear();
                m_mousecaught = 1;

                S_PlaySound(EXITMENUSOUND);

                object->destination[0] = 0;

                M_MenuFileSelect(0);

                M_ChangeMenuAnimate(cm->parentID, MA_Return);
            }
            else if (I_AdvanceTrigger())
            {
                I_AdvanceTriggerClear();

                M_RunMenuInput_FileSelect_Select(object);

                S_PlaySound(PISTOL_BODYHIT);
            }
            else if (KB_KeyPressed(sc_Home))
            {
                KB_ClearKeyDown(sc_Home);

                M_RunMenuInput_FileSelect_Movement(object, MM_Home);

                S_PlaySound(KICK_HIT);
            }
            else if (KB_KeyPressed(sc_End))
            {
                KB_ClearKeyDown(sc_End);

                M_RunMenuInput_FileSelect_Movement(object, MM_End);

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

                    M_RunMenuInput_FileSelect_MovementVerify(object);

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

                    M_RunMenuInput_FileSelect_MovementVerify(object);

                    S_PlaySound(KICK_HIT);
                }
            }
            else if (I_MenuLeft() || I_MenuRight())
            {
                I_MenuLeftClear();
                I_MenuRightClear();

                if ((object->currentList ? object->fnlist.numdirs : object->fnlist.numfiles) > 0)
                {
                    M_RunMenuInput_FileSelect_Movement(object, MM_Swap);

                    S_PlaySound(KICK_HIT);
                }
            }
            else if (I_MenuUp())
            {
                I_MenuUpClear();

                M_RunMenuInput_FileSelect_Movement(object, MM_Up);

                S_PlaySound(KICK_HIT);
            }
            else if (I_MenuDown())
            {
                I_MenuDownClear();

                M_RunMenuInput_FileSelect_Movement(object, MM_Down);

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

                        M_RunMenuInput_FileSelect_MovementVerify(object);

                        S_PlaySound(KICK_HIT);
                    }
                }
            }

            M_PreMenuInput(NULL);
            break;
        }

        case Message:
            if (I_ReturnTrigger() || M_RunMenuInput_MouseReturn())
            {
                I_ReturnTriggerClear();
                m_mousecaught = 1;

                S_PlaySound(EXITMENUSOUND);

                M_ChangeMenuAnimate(cm->parentID, cm->parentAnimation);
            }

            if (I_CheckAllInput())
            {
                MenuMessage_t *message = (MenuMessage_t*)cm->object;

                I_ClearAllInput();

                S_PlaySound(EXITMENUSOUND);

                M_ChangeMenuAnimate(message->linkID, message->animation);
            }

            M_PreMenuInput(NULL);
            break;

        case Verify:
            if (I_ReturnTrigger() || KB_KeyPressed(sc_N) || M_RunMenuInput_MouseReturn())
            {
                I_ReturnTriggerClear();
                KB_ClearKeyDown(sc_N);
                m_mousecaught = 1;

                M_MenuVerify(0);

                M_ChangeMenuAnimate(cm->parentID, cm->parentAnimation);

                S_PlaySound(EXITMENUSOUND);
            }

            if (I_AdvanceTrigger() || KB_KeyPressed(sc_Y) || M_RunMenuInput_MouseAdvance())
            {
                MenuVerify_t *verify = (MenuVerify_t*)cm->object;

                I_AdvanceTriggerClear();
                KB_ClearKeyDown(sc_Y);
                m_mousecaught = 1;

                M_MenuVerify(1);

                M_ChangeMenuAnimate(verify->linkID, verify->animation);

                S_PlaySound(PISTOL_BODYHIT);
            }

            M_PreMenuInput(NULL);
            break;

        case Menu:
        {
            int32_t state;

            MenuMenu_t *menu = (MenuMenu_t*)cm->object;
            MenuEntry_t *currentry = menu->entrylist[menu->currentEntry];

            state = M_DetermineMenuSpecialState(currentry);

            if (state == 0)
            {
                if (currentry != NULL)
                switch (currentry->type)
                {
                    case Dummy:
                    case Spacer:
                        break;
                    case Link:
                        if (currentry->flags & Disabled)
                            break;
                        if (I_AdvanceTrigger())
                        {
                            I_AdvanceTriggerClear();

                            M_RunMenuInput_MenuEntryLink_Activate(currentry);

                            if (g_player[myconnectindex].ps->gm&MODE_MENU) // for skill selection
                                S_PlaySound(PISTOL_BODYHIT);
                        }
                        break;
                    case Option:
                    {
                        MenuOption_t *object = (MenuOption_t*)currentry->entry;

                        if (currentry->flags & Disabled)
                            break;

                        if (I_AdvanceTrigger())
                        {
                            I_AdvanceTriggerClear();

                            M_RunMenuInput_MenuEntryOption_Activate(currentry, object);

                            S_PlaySound(PISTOL_BODYHIT);
                        }
                        else if (I_MenuRight())
                        {
                            I_MenuRightClear();

                            M_RunMenuInput_MenuEntryOption_Movement(currentry, object, MM_Right);

                            S_PlaySound(PISTOL_BODYHIT);
                        }
                        else if (I_MenuLeft())
                        {
                            I_MenuLeftClear();

                            M_RunMenuInput_MenuEntryOption_Movement(currentry, object, MM_Left);

                            S_PlaySound(PISTOL_BODYHIT);
                        }
                    }
                        break;
                    case Custom2Col:
                        if (I_MenuLeft() || I_MenuRight())
                        {
                            I_MenuLeftClear();
                            I_MenuRightClear();

                            M_RunMenuInput_MenuMenu_Movement(menu, MM_Swap);

                            S_PlaySound(KICK_HIT);
                        }

                        if (currentry->flags & Disabled)
                            break;

                        if (I_AdvanceTrigger())
                        {
                            I_AdvanceTriggerClear();

                            M_RunMenuInput_MenuEntryCustom2Col_Activate(currentry);

                            S_PlaySound(PISTOL_BODYHIT);
                        }
                        break;
                    case RangeInt32:
                    {
                        MenuRangeInt32_t *object = (MenuRangeInt32_t*)currentry->entry;

                        if (currentry->flags & Disabled)
                            break;

                        if (I_SliderLeft())
                        {
                            I_SliderLeftClear();

                            M_RunMenuInput_MenuEntryRangeInt32_Movement(currentry, object, MM_Left);

                            S_PlaySound(KICK_HIT);
                        }
                        else if (I_SliderRight())
                        {
                            I_SliderRightClear();

                            M_RunMenuInput_MenuEntryRangeInt32_Movement(currentry, object, MM_Right);

                            S_PlaySound(KICK_HIT);
                        }
                        break;
                    }
                    case RangeFloat:
                    {
                        MenuRangeFloat_t *object = (MenuRangeFloat_t*)currentry->entry;

                        if (currentry->flags & Disabled)
                            break;

                        if (I_SliderLeft())
                        {
                            I_SliderLeftClear();

                            M_RunMenuInput_MenuEntryRangeFloat_Movement(currentry, object, MM_Left);

                            S_PlaySound(KICK_HIT);
                        }
                        else if (I_SliderRight())
                        {
                            I_SliderRightClear();

                            M_RunMenuInput_MenuEntryRangeFloat_Movement(currentry, object, MM_Right);

                            S_PlaySound(KICK_HIT);
                        }
                        break;
                    }
                    case RangeDouble:
                    {
                        MenuRangeDouble_t *object = (MenuRangeDouble_t*)currentry->entry;

                        if (currentry->flags & Disabled)
                            break;

                        if (I_SliderLeft())
                        {
                            I_SliderLeftClear();

                            M_RunMenuInput_MenuEntryRangeDouble_Movement(/*currentry, */object, MM_Left);

                            S_PlaySound(KICK_HIT);
                        }
                        else if (I_SliderRight())
                        {
                            I_SliderRightClear();

                            M_RunMenuInput_MenuEntryRangeDouble_Movement(/*currentry, */object, MM_Right);

                            S_PlaySound(KICK_HIT);
                        }
                        break;
                    }

                    case String:
                    {
                        if (currentry->flags & Disabled)
                            break;

                        if (I_AdvanceTrigger())
                        {
                            I_AdvanceTriggerClear();

                            M_RunMenuInput_MenuEntryString_Activate(currentry);

                            S_PlaySound(PISTOL_BODYHIT);
                        }

                        break;
                    }
                }

                if (I_ReturnTrigger() || I_EscapeTrigger() || M_RunMenuInput_MouseReturn())
                {
                    I_ReturnTriggerClear();
                    I_EscapeTriggerClear();
                    m_mousecaught = 1;

                    S_PlaySound(EXITMENUSOUND);

                    M_ChangeMenuAnimate(cm->parentID, cm->parentAnimation);
                }
                else if (KB_KeyPressed(sc_Home))
                {
                    KB_ClearKeyDown(sc_Home);

                    S_PlaySound(KICK_HIT);

                    currentry = M_RunMenuInput_MenuMenu_Movement(menu, MM_Home);
                }
                else if (KB_KeyPressed(sc_End))
                {
                    KB_ClearKeyDown(sc_End);

                    S_PlaySound(KICK_HIT);

                    currentry = M_RunMenuInput_MenuMenu_Movement(menu, MM_End);
                }
                else if (I_MenuUp())
                {
                    I_MenuUpClear();

                    S_PlaySound(KICK_HIT);

                    currentry = M_RunMenuInput_MenuMenu_Movement(menu, MM_Up);
                }
                else if (I_MenuDown())
                {
                    I_MenuDownClear();

                    S_PlaySound(KICK_HIT);

                    currentry = M_RunMenuInput_MenuMenu_Movement(menu, MM_Down);
                }

                if (currentry != NULL && !(currentry->flags & Disabled))
                    M_PreMenuInput(currentry);
            }
            else if (state == 1)
            {
                if (currentry->type == String)
                {
                    MenuString_t *object = (MenuString_t*)currentry->entry;

                    int32_t hitstate = I_EnterText(object->editfield, object->maxlength, object->flags);

                    if (hitstate == -1 || M_RunMenuInput_MouseReturn())
                    {
                        m_mousecaught = 1;

                        M_RunMenuInput_MenuEntryString_Cancel(/*currentry, */object);

                        S_PlaySound(EXITMENUSOUND);
                    }
                    else if (hitstate == 1)
                    {
                        M_RunMenuInput_MenuEntryString_Submit(currentry, object);

                        S_PlaySound(PISTOL_BODYHIT);
                    }
                }
            }
            else if (state == 2)
            {
                if (currentry->type == Option)
                {
                    MenuOption_t *object = (MenuOption_t*)currentry->entry;

                    if (I_ReturnTrigger() || M_RunMenuInput_MouseReturn())
                    {
                        I_ReturnTriggerClear();
                        m_mousecaught = 1;

                        S_PlaySound(EXITMENUSOUND);

                        object->options->currentEntry = -1;
                    }
                    else if (I_AdvanceTrigger())
                    {
                        I_AdvanceTriggerClear();

                        if (!M_RunMenuInput_MenuEntryOptionList_Activate(currentry, object))
                            S_PlaySound(PISTOL_BODYHIT);
                    }
                    else if (KB_KeyPressed(sc_Home))
                    {
                        KB_ClearKeyDown(sc_Home);

                        S_PlaySound(KICK_HIT);

                        M_RunMenuInput_MenuEntryOptionList_Movement(object, MM_Home);
                    }
                    else if (KB_KeyPressed(sc_End))
                    {
                        KB_ClearKeyDown(sc_End);

                        S_PlaySound(KICK_HIT);

                        M_RunMenuInput_MenuEntryOptionList_Movement(object, MM_End);
                    }
                    else if (I_MenuUp())
                    {
                        I_MenuUpClear();

                        S_PlaySound(KICK_HIT);

                        M_RunMenuInput_MenuEntryOptionList_Movement(object, MM_Up);
                    }
                    else if (I_MenuDown())
                    {
                        I_MenuDownClear();

                        S_PlaySound(KICK_HIT);

                        M_RunMenuInput_MenuEntryOptionList_Movement(object, MM_Down);
                    }
                }
                else if (currentry->type == Custom2Col)
                {
                    if (I_EscapeTrigger() || M_RunMenuInput_MouseReturn())
                    {
                        I_EscapeTriggerClear();
                        m_mousecaught = 1;

                        S_PlaySound(EXITMENUSOUND);

                        ((MenuCustom2Col_t*)currentry->entry)->screenOpen = 0;
                    }
                    else if (M_PreMenuCustom2ColScreen(currentry))
                        ((MenuCustom2Col_t*)currentry->entry)->screenOpen = 0;
                }
            }

            break;
        }
    }
}

// This function MUST NOT RECURSE. That is why M_RunMenu is separate.
void M_DisplayMenus(void)
{
    vec2_t origin = { 0, 0 }, previousOrigin = { 0, 0 };

    Net_GetPackets();

    if ((g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
    {
        walock[TILE_LOADSHOT] = 1;
        return;
    }

    if (!M_IsTextInput(m_currentMenu) && KB_KeyPressed(sc_Q))
        M_ChangeMenuAnimate(MENU_QUIT, MA_Advance);

    int32_t mousestatus = readmouseabsxy(&m_mousepos, &mouseabs);
    if (mousestatus && mousepressstate == Mouse_Pressed)
        m_mousedownpos = m_mousepos;

    M_RunMenuInput(m_currentMenu);

    g_player[myconnectindex].ps->gm &= (0xff-MODE_TYPE);
    // g_player[myconnectindex].ps->fta = 0;

    if (((g_player[myconnectindex].ps->gm&MODE_GAME) || ud.recstat==2) && M_BlackTranslucentBackgroundOK(g_currentMenu))
        fade_screen_black(1);

    if (M_UpdateScreenOK(g_currentMenu))
        G_UpdateScreenArea();

#if !defined EDUKE32_TOUCH_DEVICES
    if (m_menuchange_watchpoint > 0)
        m_menuchange_watchpoint++;
#endif

    // Determine animation values.
    if (totalclock < m_animation.start + m_animation.length)
    {
        const int32_t screenwidth = scale(240<<16, xdim, ydim);

        origin.x = scale(screenwidth, m_animation.in(&m_animation), 32768);
        previousOrigin.x = scale(screenwidth, m_animation.out(&m_animation), 32768);

        ud.m_origin = previousOrigin;
        VM_OnEventWithReturn(EVENT_DISPLAYINACTIVEMENU, g_player[screenpeek].ps->i, screenpeek, m_animation.previous->menuID);
        previousOrigin = ud.m_origin;
    }

    ud.m_origin = origin;
    VM_OnEventWithReturn(EVENT_DISPLAYMENU, g_player[screenpeek].ps->i, screenpeek, g_currentMenu);
    origin = ud.m_origin;

    // Display the menu, with a transition animation if applicable.
    if (totalclock < m_animation.start + m_animation.length)
    {
        M_RunMenu(m_animation.previous, previousOrigin);
        M_RunMenu(m_animation.current, origin);
    }
    else
        M_RunMenu(m_currentMenu, origin);

#if !defined EDUKE32_TOUCH_DEVICES
    if (m_menuchange_watchpoint >= 3)
        m_menuchange_watchpoint = 0;
#endif

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
