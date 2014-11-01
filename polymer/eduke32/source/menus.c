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

#include "duke3d.h"
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

// common positions
#define MENU_MARGIN_REGULAR 40
#define MENU_MARGIN_WIDE    32
#define MENU_MARGIN_CENTER  160
#define MENU_HEIGHT_CENTER  100

int32_t g_skillSoundVoice = -1;

extern int32_t voting;

#define USERMAPENTRYLENGTH 25

#define mgametext(x,y,t) G_ScreenText(STARTALPHANUM, x, y, 65536, 0, 0, t, 0, 0, 2|8|16|ROTATESPRITE_FULL16, 0, (5<<16), (8<<16), (-1<<16), 0, TEXT_GAMETEXTNUMHACK, 0, 0, xdim-1, ydim-1)
#define mgametextcenter(x,y,t) G_ScreenText(STARTALPHANUM, (MENU_MARGIN_CENTER<<16) + (x), y, 65536, 0, 0, t, 0, 0, 2|8|16|ROTATESPRITE_FULL16, 0, (5<<16), (8<<16), (-1<<16), 0, TEXT_GAMETEXTNUMHACK|TEXT_XCENTER, 0, 0, xdim-1, ydim-1)
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
*/


// common font types
// tilenums are set after namesdyn runs
static MenuTextType_t MF_Redfont          = { -1, 10, 0,  1,  5<<16, 15<<16,  0<<16, 0<<16, TEXT_BIGALPHANUM | TEXT_UPPERCASE, };
static MenuTextType_t MF_RedfontRt        = { -1, 10, 0,  1,  5<<16, 15<<16,  0<<16, 0<<16, TEXT_BIGALPHANUM | TEXT_UPPERCASE | TEXT_XRIGHT, };
static MenuTextType_t MF_RedfontBlue      = { -1, 10, 1,  1,  5<<16, 15<<16,  0<<16, 0<<16, TEXT_BIGALPHANUM | TEXT_UPPERCASE, };
static MenuTextType_t MF_RedfontGreen     = { -1, 10, 8,  1,  5<<16, 15<<16,  0<<16, 0<<16, TEXT_BIGALPHANUM | TEXT_UPPERCASE, };
static MenuTextType_t MF_Bluefont         = { -1, 10, 0,  16, 5<<16,  7<<16, -1<<16, 0<<16, 0, };
static MenuTextType_t MF_BluefontRed      = { -1, 10, 10, 16, 5<<16,  7<<16, -1<<16, 0<<16, 0, };
static MenuTextType_t MF_Minifont         = { -1, 10, 0,  16, 4<<16,  5<<16,  1<<16, 1<<16, 0, };
static MenuTextType_t MF_MinifontRed      = { -1, 16, 21, 16, 4<<16,  5<<16,  1<<16, 1<<16, 0, };
static MenuTextType_t MF_MinifontDarkGray = { -1, 10, 13, 16, 4<<16,  5<<16,  1<<16, 1<<16, 0, };


static MenuPos_t MP_TOP_MAIN =             { { MENU_MARGIN_CENTER<<16,       55<<16, }, 0,     4<<16,  0,       -170<<16, 110<<16, 65536, };
static MenuPos_t MP_TOP_EPISODE =          { { MENU_MARGIN_CENTER<<16,       48<<16, }, 4<<16, 7<<16,  0,       -190<<16, 110<<16, 65536, };
static MenuPos_t MP_TOP_SKILL =            { { MENU_MARGIN_CENTER<<16,       58<<16, }, 0,     7<<16, 0,       -190<<16, 110<<16, 65536, };
static MenuPos_t MP_TOP_OPTIONS =          { { MENU_MARGIN_CENTER<<16,       38<<16, }, 0,     7<<16,  0,       -190<<16, 110<<16, 65536, };
static MenuPos_t MP_TOP_JOYSTICK_NETWORK = { { MENU_MARGIN_CENTER<<16,       70<<16, }, 0,     7<<16,  0,       -190<<16, 110<<16, 65536, };
static MenuPos_t MP_BIGOPTIONS =           { { MENU_MARGIN_WIDE<<16,         38<<16, }, 2<<16, 4<<16,  190<<16, -190<<16,  20<<16, 65536, };
static MenuPos_t MP_BIGOPTIONSRT =         { { MENU_MARGIN_WIDE<<16,         38<<16, }, 2<<16, 4<<16,  260<<16, -190<<16,  20<<16, 65536, };
#ifdef USE_OPENGL
static MenuPos_t MP_OPTIONS =              { { MENU_MARGIN_WIDE<<16,         37<<16, }, 4<<16, 1<<16,  216<<16, 160<<16,  10<<16, 32768, };
#endif
static MenuPos_t MP_PLAYER_1 =             { { MENU_MARGIN_WIDE<<16,         37<<16, }, 4<<16, 1<<16,   90<<16, 160<<16,  10<<16, 32768, };
#ifndef DROIDMENU
static MenuPos_t MP_PLAYER_3 =             { { MENU_MARGIN_WIDE<<16,         37<<16, }, 8<<16, 1<<16,  190<<16, 160<<16,  10<<16, 32768, };
#endif
static MenuPos_t MP_MACROS =               { { 26<<16,                       40<<16, }, 4<<16, 2<<16,    1<<16, 160<<16,  10<<16, 32768, };
static MenuPos_t MP_VIDEOSETUP =           { { MENU_MARGIN_REGULAR<<16,      38<<16, }, 6<<16, 7<<16,  168<<16, -190<<16,  20<<16, 65536, };
static MenuPos_t MP_VIDEOSETUP_APPLY =     { { (MENU_MARGIN_REGULAR+16)<<16, 38<<16, }, 6<<16, 7<<16,  168<<16, -190<<16,  36<<16, 65536, };
static MenuPos_t MP_KEYBOARDSETUPFUNCS =   { { 70<<16,                       34<<16, }, 4<<16, 3<<16,  100<<16, 151<<16,  10<<16, 32768, };
static MenuPos_t MP_MOUSEJOYSETUPBTNS =    { { 76<<16,                       34<<16, }, 4<<16, 3<<16,  100<<16, 143<<16,  10<<16, 32768, };
static MenuPos_t MP_GAMEFUNCLIST =         { { 100<<16,                      51<<16, }, 4<<16, 3<<16,  100<<16, 152<<16,  10<<16, 32768, };
static MenuPos_t MP_COLCORR =              { { MENU_MARGIN_REGULAR<<16,      86<<16, }, 8<<16, 2<<16, -240<<16, 190<<16,  20<<16, 65536, };
static MenuPos_t MP_REDSLIDE =             { { MENU_MARGIN_WIDE<<16,         37<<16, }, 8<<16, 2<<16,  170<<16, 190<<16,  20<<16, 65536, };
static MenuPos_t MP_LOADSAVE =             { { 223<<16,                      48<<16, }, 4<<16, 7<<16,    1<<16, 320<<16,  20<<16, 65536, };
static MenuPos_t MP_NETSETUP =             { { (MENU_MARGIN_REGULAR-4)<<16,  38<<16, }, 4<<16, 2<<16,  114<<16, 190<<16,  20<<16, 65536, };


// common menu option sets
#define MAKE_MENUOPTIONSET(a, b, c) { a, b, c, ARRAY_SIZE(b), &MP_GAMEFUNCLIST, -1, 0, }
#define MAKE_MENUOPTIONSETLIST(a, b, c, d) { a, b, c, ARRAY_SIZE(b), d, -1, 0, }
#define MAKE_MENUOPTIONSETDYN(...) { __VA_ARGS__, &MP_GAMEFUNCLIST, -1, 0, }
#define MAKE_MENUOPTIONSETDYNLIST(...) { __VA_ARGS__, -1, 0, }

static char *MEOSN_OffOn[] = { "Off", "On", };
static MenuOptionSet_t MEOS_OffOn = MAKE_MENUOPTIONSET( 0x3, MEOSN_OffOn, NULL );
static char *MEOSN_OnOff[] = { "On", "Off", };
static MenuOptionSet_t MEOS_OnOff = MAKE_MENUOPTIONSET( 0x3, MEOSN_OnOff, NULL );
static char *MEOSN_NoYes[] = { "No", "Yes", };
static MenuOptionSet_t MEOS_NoYes = MAKE_MENUOPTIONSET( 0x3, MEOSN_NoYes, NULL );
static char *MEOSN_YesNo[] = { "Yes", "No", };
static MenuOptionSet_t MEOS_YesNo = MAKE_MENUOPTIONSET( 0x3, MEOSN_YesNo, NULL );


static char MenuGameFuncs[NUMGAMEFUNCTIONS][MAXGAMEFUNCLEN];
static char *MenuGameFuncNone = "  -None-";
static char *MEOSN_Gamefuncs[NUMGAMEFUNCTIONS+1];
static MenuOptionSet_t MEOS_Gamefuncs = MAKE_MENUOPTIONSET( 0x1, MEOSN_Gamefuncs, NULL );



/*
MenuEntry_t and MenuGroup_t are passed in arrays of pointers so that the callback function
that is called when an entry is modified or activated can test equality of the current
entry/group pointer directly against the known ones, instead of relying on an ID number.

That way, individual menu entries/groups can be ifdef'd out painlessly.
*/

static MenuLink_t MEO_NULL = { MENU_NULL, MA_None, };
static const char* MenuCustom = "Custom";

#define MAKE_MENUSTRING(...) { __VA_ARGS__, NULL, }
#define MAKE_MENUOPTION(...) { __VA_ARGS__, -1, }
#define MAKE_MENURANGE(...) { __VA_ARGS__, }
#define MAKE_MENUENTRY(...) { __VA_ARGS__, 0, 0, 0, }

#define MAKE_MENU_TOP_ENTRYLINK(EntryName, LinkID, Title) \
static MenuLink_t MEO_ ## EntryName = { LinkID, MA_Advance, };\
static MenuEntry_t ME_ ## EntryName = MAKE_MENUENTRY( &MF_Redfont, Title, Link, &MEO_ ## EntryName )


MAKE_MENU_TOP_ENTRYLINK( MAIN_NEWGAME, MENU_EPISODE, "New Game" );
MAKE_MENU_TOP_ENTRYLINK( MAIN_NEWGAME_INGAME, MENU_NEWVERIFY, "New Game" );
static MenuLink_t MEO_MAIN_NEWGAME_NETWORK = { MENU_NETWORK, MA_Advance, };
MAKE_MENU_TOP_ENTRYLINK( MAIN_SAVEGAME, MENU_SAVE, "Save Game" );
MAKE_MENU_TOP_ENTRYLINK( MAIN_LOADGAME, MENU_LOAD, "Load Game" );
MAKE_MENU_TOP_ENTRYLINK( MAIN_OPTIONS, MENU_OPTIONS, "Options" );
#ifndef DROIDMENU
MAKE_MENU_TOP_ENTRYLINK( MAIN_HELP, MENU_STORY, "Help" );
#endif
MAKE_MENU_TOP_ENTRYLINK( MAIN_CREDITS, MENU_CREDITS, "Credits" );
MAKE_MENU_TOP_ENTRYLINK( MAIN_QUITTOTITLE, MENU_QUITTOTITLE, "Quit To Title" );
MAKE_MENU_TOP_ENTRYLINK( MAIN_QUIT, MENU_QUIT, "Quit" );
MAKE_MENU_TOP_ENTRYLINK( MAIN_QUITGAME, MENU_QUIT, "Quit Game" );

static MenuEntry_t *MEL_MAIN[] = {
    &ME_MAIN_NEWGAME,
    &ME_MAIN_OPTIONS,
    &ME_MAIN_LOADGAME,
#ifndef DROIDMENU
    &ME_MAIN_HELP,
#endif
    &ME_MAIN_CREDITS,
    &ME_MAIN_QUIT,
};

static MenuEntry_t *MEL_MAIN_INGAME[] = {
    &ME_MAIN_NEWGAME_INGAME,
    &ME_MAIN_SAVEGAME,
    &ME_MAIN_LOADGAME,
    &ME_MAIN_OPTIONS,
#ifndef DROIDMENU
    &ME_MAIN_HELP,
#endif
    &ME_MAIN_QUITTOTITLE,
    &ME_MAIN_QUITGAME,
};

// Episode and Skill will be dynamically generated after CONs are parsed
static MenuLink_t MEO_EPISODE = { MENU_SKILL, MA_Advance, };
static MenuLink_t MEO_EPISODE_SHAREWARE = { MENU_BUYDUKE, MA_Advance, };
static MenuEntry_t ME_EPISODE_TEMPLATE = MAKE_MENUENTRY( &MF_Redfont, NULL, Link, &MEO_EPISODE );
static MenuEntry_t ME_EPISODE[MAXVOLUMES];
static MenuEntry_t *MEL_EPISODE[MAXVOLUMES];

static MenuLink_t MEO_EPISODE_USERMAP = { MENU_USERMAP, MA_Advance, };
static MenuEntry_t ME_EPISODE_USERMAP = MAKE_MENUENTRY( &MF_Redfont, "User Map", Link, &MEO_EPISODE_USERMAP );
static MenuEntry_t *MEL_EPISODE_USERMAP[] = {
    &ME_EPISODE_USERMAP,
};

static MenuEntry_t ME_SKILL_TEMPLATE = MAKE_MENUENTRY( &MF_Redfont, NULL, Link, &MEO_NULL );
static MenuEntry_t ME_SKILL[MAXSKILLS];
static MenuEntry_t *MEL_SKILL[MAXSKILLS];

#ifndef DROIDMENU
static MenuOption_t MEO_GAMESETUP_STARTWIN = MAKE_MENUOPTION( &MF_RedfontRt, &MEOS_OffOn, &ud.config.ForceSetup );
static MenuEntry_t ME_GAMESETUP_STARTWIN = MAKE_MENUENTRY( &MF_Redfont, "Startup window:", Option, &MEO_GAMESETUP_STARTWIN );
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

static MenuOptionSet_t MEOS_GAMESETUP_AIM_AUTO = MAKE_MENUOPTIONSET(0x2, MEOSN_GAMESETUP_AIM_AUTO, MEOSV_GAMESETUP_AIM_AUTO);
static MenuOption_t MEO_GAMESETUP_AIM_AUTO = MAKE_MENUOPTION(&MF_RedfontRt, &MEOS_GAMESETUP_AIM_AUTO, &ud.config.AutoAim);
static MenuEntry_t ME_GAMESETUP_AIM_AUTO = MAKE_MENUENTRY(&MF_Redfont, "Auto aim:", Option, &MEO_GAMESETUP_AIM_AUTO);

static char *MEOSN_GAMESETUP_WEAPSWITCH_PICKUP[] = { "Never", "If new", "By rating", };
static MenuOptionSet_t MEOS_GAMESETUP_WEAPSWITCH_PICKUP = MAKE_MENUOPTIONSET(0x2, MEOSN_GAMESETUP_WEAPSWITCH_PICKUP, NULL);
static MenuOption_t MEO_GAMESETUP_WEAPSWITCH_PICKUP = MAKE_MENUOPTION(&MF_RedfontRt, &MEOS_GAMESETUP_WEAPSWITCH_PICKUP, NULL);
static MenuEntry_t ME_GAMESETUP_WEAPSWITCH_PICKUP = MAKE_MENUENTRY(&MF_Redfont, "Equip pickups:", Option, &MEO_GAMESETUP_WEAPSWITCH_PICKUP);

static char *MEOSN_DemoRec[] = { "Off", "Running", };
static MenuOptionSet_t MEOS_DemoRec = MAKE_MENUOPTIONSET( 0x3, MEOSN_DemoRec, NULL );
static MenuOption_t MEO_GAMESETUP_DEMOREC = MAKE_MENUOPTION( &MF_RedfontRt, &MEOS_OffOn, &ud.m_recstat );
static MenuEntry_t ME_GAMESETUP_DEMOREC = MAKE_MENUENTRY( &MF_Redfont, "Record demo:", Option, &MEO_GAMESETUP_DEMOREC );

#ifdef _WIN32
static MenuOption_t MEO_GAMESETUP_UPDATES = MAKE_MENUOPTION( &MF_RedfontRt, &MEOS_NoYes, &ud.config.CheckForUpdates );
static MenuEntry_t ME_GAMESETUP_UPDATES = MAKE_MENUENTRY( &MF_Redfont, "Online updates:", Option, &MEO_GAMESETUP_UPDATES );
#endif

static MenuOption_t MEO_ADULTMODE = MAKE_MENUOPTION(&MF_RedfontRt, &MEOS_OffOn, &ud.lockout);
static MenuEntry_t ME_ADULTMODE = MAKE_MENUENTRY(&MF_Redfont, "Parental lock:", Option, &MEO_ADULTMODE);
// static MenuLink_t MEO_ADULTMODE_PASSWORD = { MENU_ADULTPASSWORD, MA_None, };
// static MenuEntry_t ME_ADULTMODE_PASSWORD = MAKE_MENUENTRY( &MF_Redfont, "Enter Password", Link, &MEO_ADULTMODE_PASSWORD );

static MenuEntry_t *MEL_GAMESETUP1[] = {
	&ME_ADULTMODE,
#ifndef DROIDMENU
    &ME_GAMESETUP_STARTWIN,
#endif
	&ME_GAMESETUP_AIM_AUTO,
	&ME_GAMESETUP_WEAPSWITCH_PICKUP,
#ifndef DROIDMENU
    &ME_GAMESETUP_DEMOREC,
#ifdef _WIN32
	&ME_GAMESETUP_UPDATES,
#endif
#endif
};

MAKE_MENU_TOP_ENTRYLINK( OPTIONS_GAMESETUP, MENU_GAMESETUP, "Game Setup" );
MAKE_MENU_TOP_ENTRYLINK( OPTIONS_SOUNDSETUP, MENU_SOUND, "Sound Setup" );
MAKE_MENU_TOP_ENTRYLINK( OPTIONS_DISPLAYSETUP, MENU_DISPLAYSETUP, "Display Setup" );
MAKE_MENU_TOP_ENTRYLINK( OPTIONS_PLAYERSETUP, MENU_PLAYER, "Player Setup" );
MAKE_MENU_TOP_ENTRYLINK( OPTIONS_CONTROLS, MENU_CONTROLS, "Control Setup" );
MAKE_MENU_TOP_ENTRYLINK( OPTIONS_KEYBOARDSETUP, MENU_KEYBOARDSETUP, "Keyboard Setup" );
MAKE_MENU_TOP_ENTRYLINK( OPTIONS_MOUSESETUP, MENU_MOUSESETUP, "Mouse Setup" );
MAKE_MENU_TOP_ENTRYLINK( OPTIONS_JOYSTICKSETUP, MENU_JOYSTICKSETUP, "Joystick Setup" );


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
static MenuOptionSet_t MEOS_VIDEOSETUP_RESOLUTION = MAKE_MENUOPTIONSETDYN( 0x0, MEOSN_VIDEOSETUP_RESOLUTION, NULL, 0 );
static MenuOption_t MEO_VIDEOSETUP_RESOLUTION = MAKE_MENUOPTION( &MF_RedfontRt, &MEOS_VIDEOSETUP_RESOLUTION, &newresolution );
static MenuEntry_t ME_VIDEOSETUP_RESOLUTION = MAKE_MENUENTRY( &MF_Redfont, "Resolution:", Option, &MEO_VIDEOSETUP_RESOLUTION );

#ifdef USE_OPENGL
#ifdef POLYMER
static char *MEOSN_VIDEOSETUP_RENDERER[] = { "Classic", "Polymost.f", "Polymer", };
static int32_t MEOSV_VIDEOSETUP_RENDERER[] = { REND_CLASSIC, REND_POLYMOST, REND_POLYMER, };
#else
static char *MEOSN_VIDEOSETUP_RENDERER[] = { "Classic", "OpenGL", };
static int32_t MEOSV_VIDEOSETUP_RENDERER[] = { REND_CLASSIC, REND_POLYMOST, };
#endif
#else
static char *MEOSN_VIDEOSETUP_RENDERER[] = { "Classic", };
static int32_t MEOSV_VIDEOSETUP_RENDERER[] = { REND_CLASSIC, };
#endif

static MenuOptionSet_t MEOS_VIDEOSETUP_RENDERER = MAKE_MENUOPTIONSET( 0x2, MEOSN_VIDEOSETUP_RENDERER, MEOSV_VIDEOSETUP_RENDERER );

static MenuOption_t MEO_VIDEOSETUP_RENDERER = MAKE_MENUOPTION( &MF_RedfontRt, &MEOS_VIDEOSETUP_RENDERER, &newrendermode );
static MenuEntry_t ME_VIDEOSETUP_RENDERER = MAKE_MENUENTRY( &MF_Redfont, "Renderer:", Option, &MEO_VIDEOSETUP_RENDERER );
static MenuOption_t MEO_VIDEOSETUP_FULLSCREEN = MAKE_MENUOPTION( &MF_RedfontRt, &MEOS_NoYes, &newfullscreen );
static MenuEntry_t ME_VIDEOSETUP_FULLSCREEN = MAKE_MENUENTRY( &MF_Redfont, "Fullscreen:", Option, &MEO_VIDEOSETUP_FULLSCREEN );
static MenuEntry_t ME_VIDEOSETUP_APPLY = MAKE_MENUENTRY( &MF_Redfont, "Apply Changes", Link, &MEO_NULL );


static MenuLink_t MEO_DISPLAYSETUP_COLORCORR =  { MENU_COLCORR, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_COLORCORR = MAKE_MENUENTRY( &MF_Redfont, "Color Correction", Link, &MEO_DISPLAYSETUP_COLORCORR );
static MenuOption_t MEO_DISPLAYSETUP_PIXELDOUBLING = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OnOff, &ud.detail );
static MenuEntry_t ME_DISPLAYSETUP_PIXELDOUBLING = MAKE_MENUENTRY( &MF_Redfont, "Pixel Doubling:", Option, &MEO_DISPLAYSETUP_PIXELDOUBLING );


#ifdef USE_OPENGL
static MenuOption_t MEO_DISPLAYSETUP_ASPECTRATIO = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, NULL);
#else
static MenuOption_t MEO_DISPLAYSETUP_ASPECTRATIO = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &r_usenewaspect);
#endif
static MenuEntry_t ME_DISPLAYSETUP_ASPECTRATIO = MAKE_MENUENTRY(&MF_Redfont, "Widescreen:", Option, &MEO_DISPLAYSETUP_ASPECTRATIO);
#ifdef POLYMER
static char *MEOSN_DISPLAYSETUP_ASPECTRATIO_POLYMER[] = { "Auto", "4:3", "16:10", "5:3", "16:9", "1.85:1", "2.39:1", };
static double MEOSV_DISPLAYSETUP_ASPECTRATIO_POLYMER[] = { 0., 1.33, 1.6, 1.66, 1.78, 1.85, 2.39, };
static MenuOptionSet_t MEOS_DISPLAYSETUP_ASPECTRATIO_POLYMER = MAKE_MENUOPTIONSET(0x1, MEOSN_DISPLAYSETUP_ASPECTRATIO_POLYMER, NULL);
static MenuOption_t MEO_DISPLAYSETUP_ASPECTRATIO_POLYMER = MAKE_MENUOPTION(&MF_Redfont, &MEOS_DISPLAYSETUP_ASPECTRATIO_POLYMER, NULL);
static MenuEntry_t ME_DISPLAYSETUP_ASPECTRATIO_POLYMER = MAKE_MENUENTRY(&MF_Redfont, "Aspect ratio:", Option, &MEO_DISPLAYSETUP_ASPECTRATIO_POLYMER);
#endif


#ifdef USE_OPENGL
static char *MEOSN_DISPLAYSETUP_TEXFILTER[] = { "Classic", "Filtered" };
static MenuOptionSet_t MEOS_DISPLAYSETUP_TEXFILTER = MAKE_MENUOPTIONSET( 0x2, MEOSN_DISPLAYSETUP_TEXFILTER, NULL );
int32_t menufiltermode;
static MenuOption_t MEO_DISPLAYSETUP_TEXFILTER = MAKE_MENUOPTION( &MF_Redfont, &MEOS_DISPLAYSETUP_TEXFILTER, &menufiltermode );
static MenuEntry_t ME_DISPLAYSETUP_TEXFILTER = MAKE_MENUENTRY( &MF_Redfont, "Texture Mode:", Option, &MEO_DISPLAYSETUP_TEXFILTER );

static char *MEOSN_DISPLAYSETUP_ANISOTROPY[] = { "None", "2x", "4x", "8x", "16x", };
static int32_t MEOSV_DISPLAYSETUP_ANISOTROPY[] = { 0, 2, 4, 8, 16, };
static MenuOptionSet_t MEOS_DISPLAYSETUP_ANISOTROPY = MAKE_MENUOPTIONSET(0x0, MEOSN_DISPLAYSETUP_ANISOTROPY, MEOSV_DISPLAYSETUP_ANISOTROPY);
static MenuOption_t MEO_DISPLAYSETUP_ANISOTROPY = MAKE_MENUOPTION(&MF_Redfont, &MEOS_DISPLAYSETUP_ANISOTROPY, &glanisotropy);
static MenuEntry_t ME_DISPLAYSETUP_ANISOTROPY = MAKE_MENUENTRY(&MF_Redfont, "Anisotropy:", Option, &MEO_DISPLAYSETUP_ANISOTROPY);
static char *MEOSN_DISPLAYSETUP_VSYNC[] = { "NVIDIA", "Off", "On", };
static int32_t MEOSV_DISPLAYSETUP_VSYNC[] = { -1, 0, 1, };
static MenuOptionSet_t MEOS_DISPLAYSETUP_VSYNC = MAKE_MENUOPTIONSET(0x2, MEOSN_DISPLAYSETUP_VSYNC, MEOSV_DISPLAYSETUP_VSYNC);
static MenuOption_t MEO_DISPLAYSETUP_VSYNC = MAKE_MENUOPTION(&MF_Redfont, &MEOS_DISPLAYSETUP_VSYNC, &vsync);
static MenuEntry_t ME_DISPLAYSETUP_VSYNC = MAKE_MENUENTRY(&MF_Redfont, "VSync:", Option, &MEO_DISPLAYSETUP_VSYNC);

#endif

static MenuOption_t MEO_SCREENSETUP_CROSSHAIR = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &ud.crosshair);
static MenuEntry_t ME_SCREENSETUP_CROSSHAIR = MAKE_MENUENTRY(&MF_Redfont, "Crosshair:", Option, &MEO_SCREENSETUP_CROSSHAIR);
static MenuRangeInt32_t MEO_SCREENSETUP_CROSSHAIRSIZE = MAKE_MENURANGE(&MF_Redfont, 2, 25, 100, 16, 0, &ud.crosshairscale);
static MenuEntry_t ME_SCREENSETUP_CROSSHAIRSIZE = MAKE_MENUENTRY(&MF_Redfont, "Size:", RangeInt32, &MEO_SCREENSETUP_CROSSHAIRSIZE);

static int32_t vpsize;
static MenuRangeInt32_t MEO_SCREENSETUP_SCREENSIZE = MAKE_MENURANGE(&MF_Redfont, 0, 12, 0, 4, 0, &vpsize);
static MenuEntry_t ME_SCREENSETUP_SCREENSIZE = MAKE_MENUENTRY(&MF_Redfont, "Screen size:", RangeInt32, &MEO_SCREENSETUP_SCREENSIZE);
static MenuRangeInt32_t MEO_SCREENSETUP_TEXTSIZE = MAKE_MENURANGE(&MF_Redfont, 2, 100, 400, 7, 0, &ud.textscale);
static MenuEntry_t ME_SCREENSETUP_TEXTSIZE = MAKE_MENUENTRY(&MF_Redfont, "Size:", RangeInt32, &MEO_SCREENSETUP_TEXTSIZE);
static MenuOption_t MEO_SCREENSETUP_LEVELSTATS = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &ud.levelstats);
static MenuEntry_t ME_SCREENSETUP_LEVELSTATS = MAKE_MENUENTRY(&MF_Redfont, "Level stats:", Option, &MEO_SCREENSETUP_LEVELSTATS);


static MenuOption_t MEO_SCREENSETUP_SHOWPICKUPMESSAGES = MAKE_MENUOPTION(&MF_Redfont, &MEOS_OffOn, &ud.fta_on);
static MenuEntry_t ME_SCREENSETUP_SHOWPICKUPMESSAGES = MAKE_MENUENTRY(&MF_Redfont, "Pickup messages:", Option, &MEO_SCREENSETUP_SHOWPICKUPMESSAGES);

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

static MenuOptionSet_t MEOS_SCREENSETUP_NEWSTATUSBAR = MAKE_MENUOPTIONSET(0x2, MEOSN_SCREENSETUP_NEWSTATUSBAR, MEOSV_SCREENSETUP_NEWSTATUSBAR);
static MenuOption_t MEO_SCREENSETUP_NEWSTATUSBAR = MAKE_MENUOPTION(&MF_Redfont, &MEOS_SCREENSETUP_NEWSTATUSBAR, &ud.althud);
static MenuEntry_t ME_SCREENSETUP_NEWSTATUSBAR = MAKE_MENUENTRY(&MF_Redfont, "Status bar:", Option, &MEO_SCREENSETUP_NEWSTATUSBAR);



static MenuRangeInt32_t MEO_SCREENSETUP_SBARSIZE = MAKE_MENURANGE(&MF_Redfont, 2, 36, 100, 17, 0, &ud.statusbarscale);
static MenuEntry_t ME_SCREENSETUP_SBARSIZE = MAKE_MENUENTRY(&MF_Redfont, "Size:", RangeInt32, &MEO_SCREENSETUP_SBARSIZE);


static MenuLink_t MEO_DISPLAYSETUP_SCREENSETUP = { MENU_SCREENSETUP, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_SCREENSETUP = MAKE_MENUENTRY(&MF_Redfont, "On-screen displays", Link, &MEO_DISPLAYSETUP_SCREENSETUP);


#ifndef DROIDMENU
static MenuLink_t MEO_DISPLAYSETUP_RENDERERSETUP =  { MENU_RENDERERSETUP, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_RENDERERSETUP = MAKE_MENUENTRY( &MF_Redfont, "Advanced", Link, &MEO_DISPLAYSETUP_RENDERERSETUP );

static MenuLink_t MEO_DISPLAYSETUP_VIDEOSETUP = { MENU_VIDEOSETUP, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_VIDEOSETUP = MAKE_MENUENTRY(&MF_Redfont, "Video mode", Link, &MEO_DISPLAYSETUP_VIDEOSETUP);
#endif

static MenuEntry_t *MEL_OPTIONS[] = {
    &ME_OPTIONS_GAMESETUP,
    &ME_OPTIONS_SOUNDSETUP,
    &ME_OPTIONS_DISPLAYSETUP,
#ifndef DROIDMENU
    &ME_OPTIONS_PLAYERSETUP,
#endif
    &ME_OPTIONS_CONTROLS,
};

static MenuEntry_t *MEL_CONTROLS[] = {
	&ME_OPTIONS_KEYBOARDSETUP,
	&ME_OPTIONS_MOUSESETUP,
	&ME_OPTIONS_JOYSTICKSETUP,
};


static MenuEntry_t *MEL_VIDEOSETUP1[] = {
    &ME_VIDEOSETUP_RESOLUTION,
    &ME_VIDEOSETUP_RENDERER,
    &ME_VIDEOSETUP_FULLSCREEN,
};
static MenuEntry_t *MEL_VIDEOSETUP_APPLY[] = {
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
#ifndef DROIDMENU
    &ME_DISPLAYSETUP_RENDERERSETUP,
#endif
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
#ifndef DROIDMENU
	&ME_DISPLAYSETUP_ANISOTROPY,
	&ME_DISPLAYSETUP_VSYNC,
    &ME_DISPLAYSETUP_RENDERERSETUP,
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
	&ME_DISPLAYSETUP_RENDERERSETUP,
#endif
};

#endif
#endif



static char *MenuKeyNone = "  -";
static char *MEOSN_Keys[NUMKEYS];

static MenuCustom2Col_t MEO_KEYBOARDSETUPFUNCS_TEMPLATE = { &MF_MinifontRed, 54<<16, MEOSN_Keys, NUMKEYS, { NULL, NULL, }, 0, };
static MenuCustom2Col_t MEO_KEYBOARDSETUPFUNCS[NUMGAMEFUNCTIONS];
static MenuEntry_t ME_KEYBOARDSETUPFUNCS_TEMPLATE = MAKE_MENUENTRY( &MF_Minifont, NULL, Custom2Col, &MEO_KEYBOARDSETUPFUNCS_TEMPLATE );
static MenuEntry_t ME_KEYBOARDSETUPFUNCS[NUMGAMEFUNCTIONS];
static MenuEntry_t *MEL_KEYBOARDSETUPFUNCS[NUMGAMEFUNCTIONS];

static MenuLink_t MEO_KEYBOARDSETUP_KEYS =  { MENU_KEYBOARDKEYS, MA_Advance, };
static MenuEntry_t ME_KEYBOARDSETUP_KEYS = MAKE_MENUENTRY( &MF_Redfont, "Configure Keys", Link, &MEO_KEYBOARDSETUP_KEYS );
static MenuEntry_t ME_KEYBOARDSETUP_RESET = MAKE_MENUENTRY( &MF_Redfont, "Reset To Defaults", Link, &MEO_NULL );
static MenuEntry_t ME_KEYBOARDSETUP_RESETCLASSIC = MAKE_MENUENTRY( &MF_Redfont, "Reset To Classic", Link, &MEO_NULL );

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
static MenuEntry_t ME_MOUSEJOYSETUPBTNS_TEMPLATE = MAKE_MENUENTRY( &MF_Minifont, NULL, Option, NULL );
static MenuEntry_t ME_MOUSESETUPBTNS[MENUMOUSEFUNCTIONS];
static MenuEntry_t *MEL_MOUSESETUPBTNS[MENUMOUSEFUNCTIONS];

static MenuLink_t MEO_MOUSESETUP_BTNS =  { MENU_MOUSEBTNS, MA_Advance, };
static MenuEntry_t ME_MOUSESETUP_BTNS = MAKE_MENUENTRY( &MF_Redfont, "Button assignment", Link, &MEO_MOUSESETUP_BTNS );
static MenuRangeFloat_t MEO_MOUSESETUP_SENSITIVITY = MAKE_MENURANGE( &MF_RedfontRt, 1, .5f, 16.f, 32, 0.f, &CONTROL_MouseSensitivity );
static MenuEntry_t ME_MOUSESETUP_SENSITIVITY = MAKE_MENUENTRY( &MF_Redfont, "Sensitivity:", RangeFloat, &MEO_MOUSESETUP_SENSITIVITY );
static MenuOption_t MEO_MOUSESETUP_MOUSEAIMING = MAKE_MENUOPTION( &MF_RedfontRt, &MEOS_NoYes, &g_myAimMode );
static MenuEntry_t ME_MOUSESETUP_MOUSEAIMING = MAKE_MENUENTRY( &MF_Redfont, "Vertical aiming:", Option, &MEO_MOUSESETUP_MOUSEAIMING );
static MenuOption_t MEO_MOUSESETUP_INVERT = MAKE_MENUOPTION( &MF_RedfontRt, &MEOS_YesNo, &ud.mouseflip );
static MenuEntry_t ME_MOUSESETUP_INVERT = MAKE_MENUENTRY( &MF_Redfont, "Invert aiming:", Option, &MEO_MOUSESETUP_INVERT );
static MenuOption_t MEO_MOUSESETUP_SMOOTH = MAKE_MENUOPTION( &MF_RedfontRt, &MEOS_NoYes, &ud.config.SmoothInput );
static MenuEntry_t ME_MOUSESETUP_SMOOTH = MAKE_MENUENTRY( &MF_Redfont, "Filter input:", Option, &MEO_MOUSESETUP_SMOOTH );
static MenuLink_t MEO_MOUSESETUP_ADVANCED =  { MENU_MOUSEADVANCED, MA_Advance, };
static MenuEntry_t ME_MOUSESETUP_ADVANCED = MAKE_MENUENTRY( &MF_Redfont, "Advanced setup", Link, &MEO_MOUSESETUP_ADVANCED );

static MenuEntry_t *MEL_MOUSESETUP1[] = {
    &ME_MOUSESETUP_SENSITIVITY,
	&ME_MOUSESETUP_BTNS,
};

static MenuEntry_t *MEL_MOUSESETUP2[] = {
	&ME_MOUSESETUP_MOUSEAIMING,
	&ME_MOUSESETUP_INVERT,
	&ME_MOUSESETUP_SMOOTH,
	&ME_MOUSESETUP_ADVANCED,
};

MAKE_MENU_TOP_ENTRYLINK( JOYSTICK_EDITBUTTONS, MENU_JOYSTICKBTNS, "Edit Buttons" );
MAKE_MENU_TOP_ENTRYLINK( JOYSTICK_EDITAXES, MENU_JOYSTICKAXES, "Edit Axes" );

static MenuEntry_t *MEL_JOYSTICKSETUP[] = {
    &ME_JOYSTICK_EDITBUTTONS,
    &ME_JOYSTICK_EDITAXES,
};

#define MAXJOYBUTTONSTRINGLENGTH 32

static char MenuJoystickNames[MAXJOYBUTTONS<<1][MAXJOYBUTTONSTRINGLENGTH];

static MenuOption_t MEO_JOYSTICKBTNS[MAXJOYBUTTONS<<1];
static MenuEntry_t ME_JOYSTICKBTNS[MAXJOYBUTTONS<<1];
static MenuEntry_t *MEL_JOYSTICKBTNS[MAXJOYBUTTONS<<1];

static MenuLink_t MEO_JOYSTICKAXES = { MENU_JOYSTICKAXIS, MA_Advance, };
static MenuEntry_t ME_JOYSTICKAXES_TEMPLATE = MAKE_MENUENTRY( &MF_Redfont, NULL, Link, &MEO_JOYSTICKAXES );
static MenuEntry_t ME_JOYSTICKAXES[MAXJOYAXES];
static char MenuJoystickAxes[MAXJOYAXES][MAXJOYBUTTONSTRINGLENGTH];

static MenuEntry_t *MEL_JOYSTICKAXES[MAXJOYAXES];

static MenuRangeInt32_t MEO_MOUSEADVANCED_SCALEX = MAKE_MENURANGE( &MF_Bluefont, 3, -262144, 262144, 65, 65536, &ud.config.MouseAnalogueScale[0] );
static MenuEntry_t ME_MOUSEADVANCED_SCALEX = MAKE_MENUENTRY( &MF_Redfont, "X-Axis Scale", RangeInt32, &MEO_MOUSEADVANCED_SCALEX );
static MenuRangeInt32_t MEO_MOUSEADVANCED_SCALEY = MAKE_MENURANGE( &MF_Bluefont, 3, -262144, 262144, 65, 65536, &ud.config.MouseAnalogueScale[1] );
static MenuEntry_t ME_MOUSEADVANCED_SCALEY = MAKE_MENUENTRY( &MF_Redfont, "Y-Axis Scale", RangeInt32, &MEO_MOUSEADVANCED_SCALEY );

static MenuEntry_t *MEL_MOUSEADVANCED[] = {
    &ME_MOUSEADVANCED_SCALEX,
    &ME_MOUSEADVANCED_SCALEY,
};

static MenuOption_t MEO_MOUSEADVANCED_DAXES_UP = MAKE_MENUOPTION( &MF_BluefontRed, &MEOS_Gamefuncs, &ud.config.MouseDigitalFunctions[1][0] );
static MenuEntry_t ME_MOUSEADVANCED_DAXES_UP = MAKE_MENUENTRY( &MF_Redfont, "Digital Up", Option, &MEO_MOUSEADVANCED_DAXES_UP );
static MenuOption_t MEO_MOUSEADVANCED_DAXES_DOWN = MAKE_MENUOPTION( &MF_BluefontRed, &MEOS_Gamefuncs, &ud.config.MouseDigitalFunctions[1][1] );
static MenuEntry_t ME_MOUSEADVANCED_DAXES_DOWN = MAKE_MENUENTRY( &MF_Redfont, "Digital Down", Option, &MEO_MOUSEADVANCED_DAXES_DOWN );
static MenuOption_t MEO_MOUSEADVANCED_DAXES_LEFT = MAKE_MENUOPTION( &MF_BluefontRed, &MEOS_Gamefuncs, &ud.config.MouseDigitalFunctions[0][0] );
static MenuEntry_t ME_MOUSEADVANCED_DAXES_LEFT = MAKE_MENUENTRY( &MF_Redfont, "Digital Left", Option, &MEO_MOUSEADVANCED_DAXES_LEFT );
static MenuOption_t MEO_MOUSEADVANCED_DAXES_RIGHT = MAKE_MENUOPTION( &MF_BluefontRed, &MEOS_Gamefuncs, &ud.config.MouseDigitalFunctions[0][1] );
static MenuEntry_t ME_MOUSEADVANCED_DAXES_RIGHT = MAKE_MENUENTRY( &MF_Redfont, "Digital Right", Option, &MEO_MOUSEADVANCED_DAXES_RIGHT );

static MenuEntry_t *MEL_MOUSEADVANCED_DAXES[] = {
    &ME_MOUSEADVANCED_DAXES_UP,
    &ME_MOUSEADVANCED_DAXES_DOWN,
    &ME_MOUSEADVANCED_DAXES_LEFT,
    &ME_MOUSEADVANCED_DAXES_RIGHT,
};

static const char *MenuJoystickHatDirections[] = { "Up", "Right", "Down", "Left", };

static char *MEOSN_JOYSTICKAXIS_ANALOG[] = { "  -None-", "Turning", "Strafing", "Looking", "Moving", };
static int32_t MEOSV_JOYSTICKAXIS_ANALOG[] = { -1, analog_turning, analog_strafing, analog_lookingupanddown, analog_moving, };
static MenuOptionSet_t MEOS_JOYSTICKAXIS_ANALOG = MAKE_MENUOPTIONSET( 0x0, MEOSN_JOYSTICKAXIS_ANALOG, MEOSV_JOYSTICKAXIS_ANALOG );
static MenuOption_t MEO_JOYSTICKAXIS_ANALOG = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_JOYSTICKAXIS_ANALOG, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_ANALOG = MAKE_MENUENTRY( &MF_Redfont, "Analog", Option, &MEO_JOYSTICKAXIS_ANALOG );
static MenuRangeInt32_t MEO_JOYSTICKAXIS_SCALE = MAKE_MENURANGE( &MF_Bluefont, 3, -262144, 262144, 65, 65536, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_SCALE = MAKE_MENUENTRY( &MF_Redfont, "Scale", RangeInt32, &MEO_JOYSTICKAXIS_SCALE );
static MenuRangeInt32_t MEO_JOYSTICKAXIS_DEAD = MAKE_MENURANGE( &MF_Bluefont, 2, 0, 1000000, 33, 0, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_DEAD = MAKE_MENUENTRY( &MF_Redfont, "Dead Zone", RangeInt32, &MEO_JOYSTICKAXIS_DEAD );
static MenuRangeInt32_t MEO_JOYSTICKAXIS_SATU = MAKE_MENURANGE( &MF_Bluefont, 2, 0, 1000000, 33, 0, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_SATU = MAKE_MENUENTRY( &MF_Redfont, "Saturation", RangeInt32, &MEO_JOYSTICKAXIS_SATU );

static MenuOption_t MEO_JOYSTICKAXIS_DIGITALNEGATIVE = MAKE_MENUOPTION( &MF_Minifont, &MEOS_Gamefuncs, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_DIGITALNEGATIVE = MAKE_MENUENTRY( &MF_BluefontRed, "Digital -", Option, &MEO_JOYSTICKAXIS_DIGITALNEGATIVE );
static MenuOption_t MEO_JOYSTICKAXIS_DIGITALPOSITIVE = MAKE_MENUOPTION( &MF_Minifont, &MEOS_Gamefuncs, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_DIGITALPOSITIVE = MAKE_MENUENTRY( &MF_BluefontRed, "Digital +", Option, &MEO_JOYSTICKAXIS_DIGITALPOSITIVE );

static MenuEntry_t *MEL_JOYSTICKAXIS[] = {
    &ME_JOYSTICKAXIS_ANALOG,
    &ME_JOYSTICKAXIS_SCALE,
    &ME_JOYSTICKAXIS_DEAD,
    &ME_JOYSTICKAXIS_SATU,
};

static MenuEntry_t *MEL_JOYSTICKAXIS_DIGITAL[] = {
    &ME_JOYSTICKAXIS_DIGITALNEGATIVE,
    &ME_JOYSTICKAXIS_DIGITALPOSITIVE,
};

#ifdef USE_OPENGL
static MenuOption_t MEO_RENDERERSETUP_HIGHTILE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &usehightile );
static MenuEntry_t ME_RENDERERSETUP_HIGHTILE = MAKE_MENUENTRY( &MF_BluefontRed, "Hires textures:", Option, &MEO_RENDERERSETUP_HIGHTILE );
static MenuRangeInt32_t MEO_RENDERERSETUP_TEXQUALITY = MAKE_MENURANGE( &MF_Bluefont, 0, 2, 0, 3, 0, &r_downsize );
static MenuEntry_t ME_RENDERERSETUP_TEXQUALITY = MAKE_MENUENTRY( &MF_BluefontRed, "Hires texture quality", RangeInt32, &MEO_RENDERERSETUP_TEXQUALITY );
static MenuOption_t MEO_RENDERERSETUP_PRECACHE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.config.useprecache );
static MenuEntry_t ME_RENDERERSETUP_PRECACHE = MAKE_MENUENTRY( &MF_BluefontRed, "Pre-load map textures", Option, &MEO_RENDERERSETUP_PRECACHE );
static char *MEOSN_RENDERERSETUP_TEXCACHE[] = { "Off", "On", "Compress", };
static MenuOptionSet_t MEOS_RENDERERSETUP_TEXCACHE = MAKE_MENUOPTIONSET( 0x2, MEOSN_RENDERERSETUP_TEXCACHE, NULL );
static MenuOption_t MEO_RENDERERSETUP_TEXCACHE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_RENDERERSETUP_TEXCACHE, &glusetexcache );
static MenuEntry_t ME_RENDERERSETUP_TEXCACHE = MAKE_MENUENTRY( &MF_BluefontRed, "On disk texture cache", Option, &MEO_RENDERERSETUP_TEXCACHE );
static MenuOption_t MEO_RENDERERSETUP_DETAILTEX = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &r_detailmapping );
static MenuEntry_t ME_RENDERERSETUP_DETAILTEX = MAKE_MENUENTRY( &MF_BluefontRed, "Detail textures:", Option, &MEO_RENDERERSETUP_DETAILTEX );
static MenuOption_t MEO_RENDERERSETUP_MODELS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &usemodels );
static MenuEntry_t ME_RENDERERSETUP_MODELS = MAKE_MENUENTRY( &MF_BluefontRed, "Models:", Option, &MEO_RENDERERSETUP_MODELS );
#endif

#ifdef USE_OPENGL
static MenuEntry_t *MEL_RENDERERSETUP_GL2[] = {
    &ME_RENDERERSETUP_HIGHTILE,
    &ME_RENDERERSETUP_TEXQUALITY,
    &ME_RENDERERSETUP_PRECACHE,
    &ME_RENDERERSETUP_TEXCACHE,
    &ME_RENDERERSETUP_DETAILTEX,
};
static MenuEntry_t *MEL_RENDERERSETUP_GL3[] = {
    &ME_RENDERERSETUP_MODELS,
};
#endif

#ifdef DROIDMENU
static MenuRangeFloat_t MEO_COLCORR_GAMMA = MAKE_MENURANGE( &MF_Bluefont, 1, 1.f, 2.5f, 39.f, 0.f, &vid_gamma );
#else
static MenuRangeFloat_t MEO_COLCORR_GAMMA = MAKE_MENURANGE( &MF_Bluefont, 1, 0.2f, 4.f, 39, 0.f, &vid_gamma );
#endif
static MenuEntry_t ME_COLCORR_GAMMA = MAKE_MENUENTRY( &MF_Redfont, "Gamma:", RangeFloat, &MEO_COLCORR_GAMMA );
static MenuRangeFloat_t MEO_COLCORR_CONTRAST = MAKE_MENURANGE( &MF_Bluefont, 1, 0.1f, 2.7f, 53, 0.f, &vid_contrast );
static MenuEntry_t ME_COLCORR_CONTRAST = MAKE_MENUENTRY( &MF_Redfont, "Contrast:", RangeFloat, &MEO_COLCORR_CONTRAST );
static MenuRangeFloat_t MEO_COLCORR_BRIGHTNESS = MAKE_MENURANGE( &MF_Bluefont, 1, -0.8f, 0.8f, 33, 0.f, &vid_brightness );
static MenuEntry_t ME_COLCORR_BRIGHTNESS = MAKE_MENUENTRY( &MF_Redfont, "Brightness:", RangeFloat, &MEO_COLCORR_BRIGHTNESS );
static MenuEntry_t ME_COLCORR_RESET = MAKE_MENUENTRY( &MF_Redfont, "Reset To Defaults", Link, &MEO_NULL );
static MenuRangeFloat_t MEO_COLCORR_AMBIENT = MAKE_MENURANGE(&MF_Bluefont, 1, 0.125f, 4.f, 32, 0.f, &r_ambientlight);
static MenuEntry_t ME_COLCORR_AMBIENT = MAKE_MENUENTRY(&MF_Redfont, "Visibility:", RangeFloat, &MEO_COLCORR_AMBIENT);

static MenuEntry_t *MEL_COLCORR[] = {
    &ME_COLCORR_GAMMA,
#ifndef DROIDMENU
    &ME_COLCORR_CONTRAST,
    &ME_COLCORR_BRIGHTNESS,
#endif
	&ME_COLCORR_AMBIENT,
};

static MenuEntry_t *MEL_COLCORR_RESET[] = {
    &ME_COLCORR_RESET,
};

static MenuEntry_t *MEL_SCREENSETUP1[] = {
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
static MenuEntry_t ME_LOAD_TEMPLATE = MAKE_MENUENTRY( &MF_MinifontRed, NULL, Link, &MEO_LOAD );
static MenuEntry_t ME_LOAD[MAXSAVEGAMES];
static MenuEntry_t *MEL_LOAD[MAXSAVEGAMES];

static MenuString_t MEO_SAVE_TEMPLATE = MAKE_MENUSTRING( &MF_MinifontRed, NULL, MAXSAVEGAMENAME, 0 );
static MenuString_t MEO_SAVE[MAXSAVEGAMES];
static MenuEntry_t ME_SAVE_TEMPLATE = MAKE_MENUENTRY( &MF_MinifontRed, NULL, String, &MEO_SAVE_TEMPLATE );
static MenuEntry_t ME_SAVE[MAXSAVEGAMES];
static MenuEntry_t *MEL_SAVE[MAXSAVEGAMES];

static int32_t soundrate, soundbits, soundvoices;
static MenuOption_t MEO_SOUND = MAKE_MENUOPTION( &MF_RedfontRt, &MEOS_OffOn, &ud.config.SoundToggle );
static MenuEntry_t ME_SOUND = MAKE_MENUENTRY( &MF_Redfont, "Sound:", Option, &MEO_SOUND );

static MenuOption_t MEO_SOUND_MUSIC = MAKE_MENUOPTION( &MF_RedfontRt, &MEOS_OffOn, &ud.config.MusicToggle );
static MenuEntry_t ME_SOUND_MUSIC = MAKE_MENUENTRY( &MF_Redfont, "Music:", Option, &MEO_SOUND_MUSIC );

static MenuRangeInt32_t MEO_SOUND_VOLUME_MASTER = MAKE_MENURANGE( &MF_RedfontRt, 2, 0, 255, 33, 0, &ud.config.MasterVolume );
static MenuEntry_t ME_SOUND_VOLUME_MASTER = MAKE_MENUENTRY( &MF_Redfont, "Volume:", RangeInt32, &MEO_SOUND_VOLUME_MASTER );

static MenuRangeInt32_t MEO_SOUND_VOLUME_EFFECTS = MAKE_MENURANGE( &MF_RedfontRt, 2, 0, 255, 33, 0, &ud.config.FXVolume );
static MenuEntry_t ME_SOUND_VOLUME_EFFECTS = MAKE_MENUENTRY( &MF_Redfont, "Effects:", RangeInt32, &MEO_SOUND_VOLUME_EFFECTS );

static MenuRangeInt32_t MEO_SOUND_VOLUME_MUSIC = MAKE_MENURANGE( &MF_RedfontRt, 2, 0, 255, 33, 0, &ud.config.MusicVolume );
static MenuEntry_t ME_SOUND_VOLUME_MUSIC = MAKE_MENUENTRY( &MF_Redfont, "Music:", RangeInt32, &MEO_SOUND_VOLUME_MUSIC );

static MenuOption_t MEO_SOUND_DUKETALK = MAKE_MENUOPTION(&MF_RedfontRt, &MEOS_NoYes, NULL);
static MenuEntry_t ME_SOUND_DUKETALK = MAKE_MENUENTRY(&MF_Redfont, "Duke talk:", Option, &MEO_SOUND_DUKETALK);

static char *MEOSN_SOUND_SAMPLINGRATE[] = { "22050Hz", "44100Hz", "48000Hz", };
static int32_t MEOSV_SOUND_SAMPLINGRATE[] = { 22050, 44100, 48000, };
static MenuOptionSet_t MEOS_SOUND_SAMPLINGRATE = MAKE_MENUOPTIONSET( 0x3, MEOSN_SOUND_SAMPLINGRATE, MEOSV_SOUND_SAMPLINGRATE );
static MenuOption_t MEO_SOUND_SAMPLINGRATE = MAKE_MENUOPTION( &MF_RedfontRt, &MEOS_SOUND_SAMPLINGRATE, &soundrate );
static MenuEntry_t ME_SOUND_SAMPLINGRATE = MAKE_MENUENTRY( &MF_Redfont, "Sample rate:", Option, &MEO_SOUND_SAMPLINGRATE );

static char *MEOSN_SOUND_SAMPLESIZE[] = { "8-bit", "16-bit", };
static int32_t MEOSV_SOUND_SAMPLESIZE[] = { 8, 16, };
static MenuOptionSet_t MEOS_SOUND_SAMPLESIZE = MAKE_MENUOPTIONSET( 0x3, MEOSN_SOUND_SAMPLESIZE, MEOSV_SOUND_SAMPLESIZE );
static MenuOption_t MEO_SOUND_SAMPLESIZE = MAKE_MENUOPTION( &MF_RedfontRt, &MEOS_SOUND_SAMPLESIZE, &soundbits );
static MenuEntry_t ME_SOUND_SAMPLESIZE = MAKE_MENUENTRY( &MF_Redfont, "Sample size:", Option, &MEO_SOUND_SAMPLESIZE );

static MenuRangeInt32_t MEO_SOUND_NUMVOICES = MAKE_MENURANGE( &MF_RedfontRt, 1, 16, 256, 16, 0, &soundvoices );
static MenuEntry_t ME_SOUND_NUMVOICES = MAKE_MENUENTRY( &MF_Redfont, "Voices:", RangeInt32, &MEO_SOUND_NUMVOICES );

static MenuEntry_t ME_SOUND_RESTART = MAKE_MENUENTRY( &MF_Redfont, "Restart sound system", Link, &MEO_NULL );

#ifndef DROIDMENU
static MenuLink_t MEO_ADVSOUND = { MENU_ADVSOUND, MA_Advance, };
static MenuEntry_t ME_SOUND_ADVSOUND = MAKE_MENUENTRY( &MF_Redfont, "Advanced", Link, &MEO_ADVSOUND );
#endif

static MenuEntry_t *MEL_SOUND1[] = {
    &ME_SOUND,
    &ME_SOUND_MUSIC,
};
static MenuEntry_t *MEL_SOUND2[] = {
	&ME_SOUND_VOLUME_MASTER,
	&ME_SOUND_VOLUME_EFFECTS,
	&ME_SOUND_VOLUME_MUSIC,
};
static MenuEntry_t *MEL_SOUND3[] = {
    &ME_SOUND_DUKETALK,
#ifndef DROIDMENU
	&ME_SOUND_ADVSOUND,
#endif
};

static MenuEntry_t *MEL_ADVSOUND1[] = {
	&ME_SOUND_SAMPLINGRATE,
	&ME_SOUND_SAMPLESIZE,
};

static MenuEntry_t *MEL_ADVSOUND2[] = {
	&ME_SOUND_NUMVOICES,
};

static MenuEntry_t *MEL_ADVSOUND3[] = {
	&ME_SOUND_RESTART,
};

MAKE_MENU_TOP_ENTRYLINK( NETWORK_PLAYERSETUP, MENU_PLAYER, "Player Setup" );
MAKE_MENU_TOP_ENTRYLINK( NETWORK_JOINGAME, MENU_NETJOIN, "Join Game" );
MAKE_MENU_TOP_ENTRYLINK( NETWORK_HOSTGAME, MENU_NETHOST, "Host Game" );

static MenuEntry_t *MEL_NETWORK[] = {
    &ME_NETWORK_PLAYERSETUP,
    &ME_NETWORK_JOINGAME,
    &ME_NETWORK_HOSTGAME,
};

static MenuString_t MEO_PLAYER_NAME = MAKE_MENUSTRING( &MF_Bluefont, szPlayerName, MAXPLAYERNAME, 0 );
static MenuEntry_t ME_PLAYER_NAME = MAKE_MENUENTRY( &MF_BluefontRed, "Name", String, &MEO_PLAYER_NAME );
static char *MEOSN_PLAYER_COLOR[] = { "Auto", "Blue", "Red", "Green", "Gray", "Dark gray", "Dark green", "Brown", "Dark blue", "Bright red", "Yellow", };
static int32_t MEOSV_PLAYER_COLOR[] = { 0, 9, 10, 11, 12, 13, 14, 15, 16, 22, 23, };
static MenuOptionSet_t MEOS_PLAYER_COLOR = MAKE_MENUOPTIONSET( 0x2, MEOSN_PLAYER_COLOR, MEOSV_PLAYER_COLOR );
static MenuOption_t MEO_PLAYER_COLOR = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_PLAYER_COLOR, &ud.color );
static MenuEntry_t ME_PLAYER_COLOR = MAKE_MENUENTRY( &MF_BluefontRed, "Color", Option, &MEO_PLAYER_COLOR );
static char *MEOSN_PLAYER_TEAM[] = { "Blue", "Red", "Green", "Gray", };
static MenuOptionSet_t MEOS_PLAYER_TEAM = MAKE_MENUOPTIONSET( 0x2, MEOSN_PLAYER_TEAM, NULL );
static MenuOption_t MEO_PLAYER_TEAM = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_PLAYER_TEAM, &ud.team );
static MenuEntry_t ME_PLAYER_TEAM = MAKE_MENUENTRY( &MF_BluefontRed, "Team", Option, &MEO_PLAYER_TEAM );
#ifndef DROIDMENU
static MenuLink_t MEO_PLAYER_MACROS = { MENU_MACROS, MA_Advance, };
static MenuEntry_t ME_PLAYER_MACROS = MAKE_MENUENTRY( &MF_BluefontRed, "Multiplayer macros", Link, &MEO_PLAYER_MACROS );
#endif

static MenuEntry_t *MEL_PLAYER_NAME[] = {
    &ME_PLAYER_NAME,
};
static MenuEntry_t *MEL_PLAYER_COLOR[] = {
    &ME_PLAYER_COLOR,
};
static MenuEntry_t *MEL_PLAYER_TEAM[] = {
    &ME_PLAYER_TEAM,
};
#ifndef DROIDMENU
static MenuEntry_t *MEL_PLAYER_MACROS[] = {
    &ME_PLAYER_MACROS,
};
#endif

static MenuString_t MEO_MACROS_TEMPLATE = MAKE_MENUSTRING( &MF_Bluefont, NULL, MAXRIDECULELENGTH, 0 );
static MenuString_t MEO_MACROS[MAXSAVEGAMES];
static MenuEntry_t ME_MACROS_TEMPLATE = MAKE_MENUENTRY( &MF_Bluefont, NULL, String, &MEO_MACROS_TEMPLATE );
static MenuEntry_t ME_MACROS[MAXRIDECULE];
static MenuEntry_t *MEL_MACROS[MAXRIDECULE];

static char *MenuUserMap = "User Map";
static char *MenuSkillNone = "None";

static char *MEOSN_NetGametypes[MAXGAMETYPES];
static char *MEOSN_NetEpisodes[MAXVOLUMES+1];
static char *MEOSN_NetLevels[MAXVOLUMES][MAXLEVELS];
static char *MEOSN_NetSkills[MAXSKILLS+1];

static MenuLink_t MEO_NETHOST_OPTIONS =  { MENU_NETOPTIONS, MA_Advance, };
static MenuEntry_t ME_NETHOST_OPTIONS = MAKE_MENUENTRY( &MF_Redfont, "Game Options", Link, &MEO_NETHOST_OPTIONS );
static MenuEntry_t ME_NETHOST_LAUNCH = MAKE_MENUENTRY( &MF_Redfont, "Launch Game", Link, &MEO_NULL );

static MenuEntry_t *MEL_NETHOST[] = {
    &ME_NETHOST_OPTIONS,
    &ME_NETHOST_LAUNCH,
};

static MenuOptionSet_t MEOS_NETOPTIONS_GAMETYPE = MAKE_MENUOPTIONSET( 0x0, MEOSN_NetGametypes, NULL );
static MenuOption_t MEO_NETOPTIONS_GAMETYPE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NETOPTIONS_GAMETYPE, &ud.m_coop );
static MenuEntry_t ME_NETOPTIONS_GAMETYPE = MAKE_MENUENTRY( &MF_Redfont, "Game Type", Option, &MEO_NETOPTIONS_GAMETYPE );
static MenuOptionSet_t MEOS_NETOPTIONS_EPISODE = MAKE_MENUOPTIONSET( 0x0, MEOSN_NetEpisodes, NULL );
static MenuOption_t MEO_NETOPTIONS_EPISODE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NETOPTIONS_EPISODE, NULL );
static MenuEntry_t ME_NETOPTIONS_EPISODE = MAKE_MENUENTRY( &MF_Redfont, "Episode", Option, &MEO_NETOPTIONS_EPISODE );
static MenuOptionSet_t MEOS_NETOPTIONS_LEVEL[MAXVOLUMES];
static MenuOption_t MEO_NETOPTIONS_LEVEL = MAKE_MENUOPTION( &MF_Bluefont, NULL, &ud.m_level_number );
static MenuEntry_t ME_NETOPTIONS_LEVEL = MAKE_MENUENTRY( &MF_Redfont, "Level", Option, &MEO_NETOPTIONS_LEVEL );
static MenuLink_t MEO_NETOPTIONS_USERMAP =  { MENU_NETUSERMAP, MA_Advance, };
static MenuEntry_t ME_NETOPTIONS_USERMAP = MAKE_MENUENTRY( &MF_Redfont, "User Map", Link, &MEO_NETOPTIONS_USERMAP );
static MenuOptionSet_t MEOS_NETOPTIONS_MONSTERS = MAKE_MENUOPTIONSET( 0x0, MEOSN_NetSkills, NULL );
static MenuOption_t MEO_NETOPTIONS_MONSTERS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NETOPTIONS_MONSTERS, NULL );
static MenuEntry_t ME_NETOPTIONS_MONSTERS = MAKE_MENUENTRY( &MF_Redfont, "Monsters", Option, &MEO_NETOPTIONS_MONSTERS );
static MenuOption_t MEO_NETOPTIONS_MARKERS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.m_marker );
static MenuEntry_t ME_NETOPTIONS_MARKERS = MAKE_MENUENTRY( &MF_Redfont, "Markers", Option, &MEO_NETOPTIONS_MARKERS );
static MenuEntry_t ME_NETOPTIONS_MARKERS_DISABLED = MAKE_MENUENTRY( &MF_RedfontBlue, "Markers", Dummy, NULL );
static MenuOption_t MEO_NETOPTIONS_MAPEXITS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OnOff, &ud.m_noexits );
static MenuEntry_t ME_NETOPTIONS_MAPEXITS = MAKE_MENUENTRY( &MF_Redfont, "Map Exits", Option, &MEO_NETOPTIONS_MAPEXITS );
static MenuOption_t MEO_NETOPTIONS_FRFIRE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.m_ffire );
static MenuEntry_t ME_NETOPTIONS_FRFIRE = MAKE_MENUENTRY( &MF_Redfont, "Fr. Fire", Option, &MEO_NETOPTIONS_FRFIRE );
static MenuEntry_t ME_NETOPTIONS_ACCEPT = MAKE_MENUENTRY( &MF_RedfontGreen, "Accept", Link, &MEO_NULL );

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
static MenuString_t MEO_NETJOIN_SERVER = MAKE_MENUSTRING( &MF_Bluefont, MenuServer, BMAX_PATH, 0 );
static MenuEntry_t ME_NETJOIN_SERVER = MAKE_MENUENTRY( &MF_Redfont, "Server", String, &MEO_NETJOIN_SERVER );
#define MAXPORTSTRINGLENGTH 6 // unsigned 16-bit integer
static char MenuPort[MAXPORTSTRINGLENGTH] = "19014";
static MenuString_t MEO_NETJOIN_PORT = MAKE_MENUSTRING( &MF_Bluefont, MenuPort, MAXPORTSTRINGLENGTH, INPUT_NUMERIC );
static MenuEntry_t ME_NETJOIN_PORT = MAKE_MENUENTRY( &MF_Redfont, "Port", String, &MEO_NETJOIN_PORT );
static MenuEntry_t ME_NETJOIN_CONNECT = MAKE_MENUENTRY( &MF_RedfontGreen, "Connect", Link, &MEO_NULL );

static MenuEntry_t *MEL_NETJOIN[] = {
    &ME_NETJOIN_SERVER,
    &ME_NETJOIN_PORT,
    &ME_NETJOIN_CONNECT,
};


#define MAKE_MENUGROUP(Entries, ...) { Entries, ARRAY_SIZE(Entries), __VA_ARGS__, 0, 0, }

static MenuGroup_t MG_MAIN = MAKE_MENUGROUP( MEL_MAIN, &MP_TOP_MAIN );
static MenuGroup_t *MGL_MAIN[] = {
    &MG_MAIN,
};

static MenuGroup_t MG_MAIN_INGAME = MAKE_MENUGROUP( MEL_MAIN_INGAME, &MP_TOP_MAIN );
static MenuGroup_t *MGL_MAIN_INGAME[] = {
    &MG_MAIN_INGAME,
};

static MenuGroup_t MG_EPISODE = MAKE_MENUGROUP( MEL_EPISODE, &MP_TOP_EPISODE );
static MenuGroup_t MG_EPISODE_USERMAP = MAKE_MENUGROUP( MEL_EPISODE_USERMAP, &MP_TOP_EPISODE );
static MenuGroup_t *MGL_EPISODE[] = {
    &MG_EPISODE,
    &MG_EPISODE_USERMAP,
};
static MenuGroup_t *MGL_EPISODE_SHAREWARE[] = {
    &MG_EPISODE,
};

static MenuGroup_t MG_SKILL = MAKE_MENUGROUP( MEL_SKILL, &MP_TOP_SKILL );
static MenuGroup_t *MGL_SKILL[] = {
    &MG_SKILL,
};

static MenuGroup_t MG_GAMESETUP1 = MAKE_MENUGROUP( MEL_GAMESETUP1, &MP_BIGOPTIONSRT );

static MenuGroup_t MG_SCREENSETUP1 = MAKE_MENUGROUP( MEL_SCREENSETUP1, &MP_BIGOPTIONS );

static MenuGroup_t *MGL_SCREENSETUP[] = {
	&MG_SCREENSETUP1
};


static MenuGroup_t MG_DISPLAYSETUP = MAKE_MENUGROUP(MEL_DISPLAYSETUP, &MP_BIGOPTIONS);

static MenuGroup_t *MGL_DISPLAYSETUP[] = {
	&MG_DISPLAYSETUP,
};

#ifdef USE_OPENGL
static MenuGroup_t MG_DISPLAYSETUP_GL = MAKE_MENUGROUP(MEL_DISPLAYSETUP_GL, &MP_BIGOPTIONS);

static MenuGroup_t *MGL_DISPLAYSETUP_GL[] = {
	&MG_DISPLAYSETUP_GL,
};

#ifdef POLYMER
static MenuGroup_t MG_DISPLAYSETUP_GL_POLYMER = MAKE_MENUGROUP(MEL_DISPLAYSETUP_GL_POLYMER, &MP_BIGOPTIONS);

static MenuGroup_t *MGL_DISPLAYSETUP_GL_POLYMER[] = {
	&MG_DISPLAYSETUP_GL_POLYMER,
};
#endif
#endif

static MenuGroup_t *MGL_GAMESETUP[] = {
    &MG_GAMESETUP1,
};

static MenuGroup_t MG_OPTIONS = MAKE_MENUGROUP( MEL_OPTIONS, &MP_TOP_OPTIONS );
static MenuGroup_t *MGL_OPTIONS[] = {
    &MG_OPTIONS,
};

static MenuGroup_t MG_VIDEOSETUP1 = MAKE_MENUGROUP( MEL_VIDEOSETUP1, &MP_BIGOPTIONSRT );
static MenuGroup_t MG_VIDEOSETUP_APPLY = MAKE_MENUGROUP( MEL_VIDEOSETUP_APPLY, &MP_VIDEOSETUP_APPLY );
static MenuGroup_t *MGL_VIDEOSETUP[] = {
    &MG_VIDEOSETUP1,
    &MG_VIDEOSETUP_APPLY,
};

static MenuGroup_t MG_CONTROLS = MAKE_MENUGROUP(MEL_CONTROLS, &MP_TOP_OPTIONS);
static MenuGroup_t *MGL_CONTROLS[] = {
	&MG_CONTROLS,
};

static MenuGroup_t MG_KEYBOARDSETUP = MAKE_MENUGROUP( MEL_KEYBOARDSETUP, &MP_TOP_OPTIONS );
static MenuGroup_t *MGL_KEYBOARDSETUP[] = {
    &MG_KEYBOARDSETUP,
};

static MenuGroup_t MG_MOUSESETUP1 = MAKE_MENUGROUP( MEL_MOUSESETUP1, &MP_BIGOPTIONS );
static MenuGroup_t MG_MOUSESETUP2 = MAKE_MENUGROUP( MEL_MOUSESETUP2, &MP_BIGOPTIONSRT );

static MenuGroup_t *MGL_MOUSESETUP[] = {
    &MG_MOUSESETUP1,
	&MG_MOUSESETUP2,
};

static MenuGroup_t MG_JOYSTICKSETUP = MAKE_MENUGROUP( MEL_JOYSTICKSETUP, &MP_TOP_JOYSTICK_NETWORK );
static MenuGroup_t *MGL_JOYSTICKSETUP[] = {
    &MG_JOYSTICKSETUP,
};

static MenuGroup_t MG_JOYSTICKBTNS = MAKE_MENUGROUP( MEL_JOYSTICKBTNS, &MP_MOUSEJOYSETUPBTNS );
static MenuGroup_t *MGL_JOYSTICKBTNS[] = {
    &MG_JOYSTICKBTNS,
};

static MenuGroup_t MG_KEYBOARDSETUPFUNCS = MAKE_MENUGROUP( MEL_KEYBOARDSETUPFUNCS, &MP_KEYBOARDSETUPFUNCS );
static MenuGroup_t *MGL_KEYBOARDKEYS[] = {
    &MG_KEYBOARDSETUPFUNCS,
};

static MenuGroup_t MG_MOUSESETUPBTNS = MAKE_MENUGROUP( MEL_MOUSESETUPBTNS, &MP_MOUSEJOYSETUPBTNS );
static MenuGroup_t *MGL_MOUSEBTNS[] = {
    &MG_MOUSESETUPBTNS,
};

static MenuGroup_t MG_JOYSTICKAXES = MAKE_MENUGROUP( MEL_JOYSTICKAXES, &MP_REDSLIDE );
static MenuGroup_t *MGL_JOYSTICKAXES[] = {
    &MG_JOYSTICKAXES,
};

static MenuGroup_t MG_MOUSEADVANCED = MAKE_MENUGROUP( MEL_MOUSEADVANCED, &MP_REDSLIDE );
static MenuGroup_t MG_MOUSEADVANCED_DAXES = MAKE_MENUGROUP( MEL_MOUSEADVANCED_DAXES, &MP_REDSLIDE );
static MenuGroup_t *MGL_MOUSEADVANCED[] = {
    &MG_MOUSEADVANCED,
    &MG_MOUSEADVANCED_DAXES,
};

static MenuGroup_t MG_JOYSTICKAXIS = MAKE_MENUGROUP( MEL_JOYSTICKAXIS, &MP_REDSLIDE );
static MenuGroup_t MG_JOYSTICKAXIS_DIGITAL = MAKE_MENUGROUP( MEL_JOYSTICKAXIS_DIGITAL, &MP_REDSLIDE );
static MenuGroup_t *MGL_JOYSTICKAXIS[] = {
    &MG_JOYSTICKAXIS,
    &MG_JOYSTICKAXIS_DIGITAL,
};

#ifdef USE_OPENGL
static MenuGroup_t MG_RENDERERSETUP_GL2 = MAKE_MENUGROUP( MEL_RENDERERSETUP_GL2, &MP_OPTIONS );
static MenuGroup_t MG_RENDERERSETUP_GL3 = MAKE_MENUGROUP( MEL_RENDERERSETUP_GL3, &MP_OPTIONS );

static MenuGroup_t *MGL_RENDERERSETUP[] = {
    &MG_RENDERERSETUP_GL2,
    &MG_RENDERERSETUP_GL3,
};
#endif

static MenuGroup_t MG_COLCORR = MAKE_MENUGROUP( MEL_COLCORR, &MP_COLCORR );
static MenuGroup_t MG_COLCORR_RESET = MAKE_MENUGROUP( MEL_COLCORR_RESET, &MP_COLCORR );
static MenuGroup_t *MGL_COLCORR[] = {
    &MG_COLCORR,
    &MG_COLCORR_RESET,
};

static MenuGroup_t MG_LOAD = MAKE_MENUGROUP( MEL_LOAD, &MP_LOADSAVE );
static MenuGroup_t *MGL_LOAD[] = {
    &MG_LOAD,
};

static MenuGroup_t MG_SAVE = MAKE_MENUGROUP( MEL_SAVE, &MP_LOADSAVE );
static MenuGroup_t *MGL_SAVE[] = {
    &MG_SAVE,
};

static MenuGroup_t MG_SOUND1 = MAKE_MENUGROUP( MEL_SOUND1, &MP_BIGOPTIONSRT );
static MenuGroup_t MG_SOUND2 = MAKE_MENUGROUP( MEL_SOUND2, &MP_BIGOPTIONS );
static MenuGroup_t MG_SOUND3 = MAKE_MENUGROUP( MEL_SOUND3, &MP_BIGOPTIONSRT );

static MenuGroup_t *MGL_SOUND[] = {
	&MG_SOUND1,
	&MG_SOUND2,
	&MG_SOUND3,
};

static MenuGroup_t MG_ADVSOUND1 = MAKE_MENUGROUP( MEL_ADVSOUND1, &MP_BIGOPTIONSRT );
static MenuGroup_t MG_ADVSOUND2 = MAKE_MENUGROUP( MEL_ADVSOUND2, &MP_BIGOPTIONS );
static MenuGroup_t MG_ADVSOUND3 = MAKE_MENUGROUP( MEL_ADVSOUND3, &MP_BIGOPTIONSRT );

static MenuGroup_t *MGL_ADVSOUND[] = {
	&MG_ADVSOUND1,
	&MG_ADVSOUND2,
	&MG_ADVSOUND3,
};


static MenuGroup_t MG_NETWORK = MAKE_MENUGROUP( MEL_NETWORK, &MP_TOP_JOYSTICK_NETWORK );
static MenuGroup_t *MGL_NETWORK[] = {
    &MG_NETWORK,
};

static MenuGroup_t MG_PLAYER_NAME = MAKE_MENUGROUP( MEL_PLAYER_NAME, &MP_PLAYER_1 );
static MenuGroup_t MG_PLAYER_COLOR = MAKE_MENUGROUP( MEL_PLAYER_COLOR, &MP_PLAYER_1 );
static MenuGroup_t MG_PLAYER_TEAM = MAKE_MENUGROUP( MEL_PLAYER_TEAM, &MP_PLAYER_1 );
#ifndef DROIDMENU
static MenuGroup_t MG_PLAYER_MACROS = MAKE_MENUGROUP( MEL_PLAYER_MACROS, &MP_PLAYER_3 );
#endif
static MenuGroup_t *MGL_PLAYER[] = {
    &MG_PLAYER_NAME,
    &MG_PLAYER_COLOR,
    &MG_PLAYER_TEAM,
#ifndef DROIDMENU
    &MG_PLAYER_MACROS,
#endif
};

static MenuGroup_t MG_MACROS = MAKE_MENUGROUP( MEL_MACROS, &MP_MACROS );
static MenuGroup_t *MGL_MACROS[] = {
    &MG_MACROS,
};

static MenuGroup_t MG_NETHOST = MAKE_MENUGROUP( MEL_NETHOST, &MP_VIDEOSETUP );
static MenuGroup_t *MGL_NETHOST[] = {
    &MG_NETHOST,
};

static MenuGroup_t MG_NETOPTIONS = MAKE_MENUGROUP( MEL_NETOPTIONS, &MP_NETSETUP );
static MenuGroup_t *MGL_NETOPTIONS[] = {
    &MG_NETOPTIONS,
};

static MenuGroup_t MG_NETJOIN = MAKE_MENUGROUP( MEL_NETJOIN, &MP_VIDEOSETUP );
static MenuGroup_t *MGL_NETJOIN[] = {
    &MG_NETJOIN,
};



#define NoTitle NULL

#define MAKE_MENUMENU(Groups, ...) { Groups, ARRAY_SIZE(Groups), __VA_ARGS__, 0, INT32_MAX, INT32_MIN, 0, 0, }

static MenuMenu_t M_MAIN = MAKE_MENUMENU( MGL_MAIN, NoTitle );
static MenuMenu_t M_MAIN_INGAME = MAKE_MENUMENU( MGL_MAIN_INGAME, NoTitle );
static MenuMenu_t M_EPISODE = MAKE_MENUMENU( MGL_EPISODE, "Select An Episode" );
static MenuMenu_t M_SKILL = MAKE_MENUMENU( MGL_SKILL, "Select Skill" );
static MenuMenu_t M_GAMESETUP = MAKE_MENUMENU( MGL_GAMESETUP, "Game Setup" );
static MenuMenu_t M_OPTIONS = MAKE_MENUMENU( MGL_OPTIONS, "Options" );
static MenuMenu_t M_VIDEOSETUP = MAKE_MENUMENU( MGL_VIDEOSETUP, "Video Mode" );
static MenuMenu_t M_KEYBOARDSETUP = MAKE_MENUMENU( MGL_KEYBOARDSETUP, "Keyboard Setup" );
static MenuMenu_t M_CONTROLS = MAKE_MENUMENU( MGL_CONTROLS, "Control Setup" );
static MenuMenu_t M_MOUSESETUP = MAKE_MENUMENU( MGL_MOUSESETUP, "Mouse Setup" );
static MenuMenu_t M_JOYSTICKSETUP = MAKE_MENUMENU( MGL_JOYSTICKSETUP, "Joystick Setup" );
static MenuMenu_t M_JOYSTICKBTNS = MAKE_MENUMENU( MGL_JOYSTICKBTNS, "Joystick Buttons" );
static MenuMenu_t M_JOYSTICKAXES = MAKE_MENUMENU( MGL_JOYSTICKAXES, "Joystick Axes" );
static MenuMenu_t M_KEYBOARDKEYS = MAKE_MENUMENU( MGL_KEYBOARDKEYS, "Keyboard Keys" );
static MenuMenu_t M_MOUSEBTNS = MAKE_MENUMENU( MGL_MOUSEBTNS, "Mouse Buttons" );
static MenuMenu_t M_MOUSEADVANCED = MAKE_MENUMENU( MGL_MOUSEADVANCED, "Advanced Mouse" );
static MenuMenu_t M_JOYSTICKAXIS = MAKE_MENUMENU( MGL_JOYSTICKAXIS, NULL );
#ifdef USE_OPENGL
static MenuMenu_t M_RENDERERSETUP = MAKE_MENUMENU( MGL_RENDERERSETUP, "Advanced Video" );
#endif
static MenuMenu_t M_COLCORR = MAKE_MENUMENU( MGL_COLCORR, "Color Correction" );
static MenuMenu_t M_SCREENSETUP = MAKE_MENUMENU( MGL_SCREENSETUP, "On-Screen Displays" );
static MenuMenu_t M_DISPLAYSETUP = MAKE_MENUMENU( MGL_DISPLAYSETUP, "Display Setup" );
static MenuMenu_t M_LOAD = MAKE_MENUMENU( MGL_LOAD, "Load Game" );
static MenuMenu_t M_SAVE = MAKE_MENUMENU( MGL_SAVE, "Save Game" );
static MenuMenu_t M_SOUND = MAKE_MENUMENU( MGL_SOUND, "Sound Setup" );
static MenuMenu_t M_ADVSOUND = MAKE_MENUMENU( MGL_ADVSOUND, "Sound Setup" );
static MenuMenu_t M_NETWORK = MAKE_MENUMENU( MGL_NETWORK, "Network Game" );
static MenuMenu_t M_PLAYER = MAKE_MENUMENU( MGL_PLAYER, "Player Setup" );
static MenuMenu_t M_MACROS = MAKE_MENUMENU( MGL_MACROS, "Multiplayer Macros" );
static MenuMenu_t M_NETHOST = MAKE_MENUMENU( MGL_NETHOST, "Host Network Game" );
static MenuMenu_t M_NETOPTIONS = MAKE_MENUMENU( MGL_NETOPTIONS, "Net Game Options" );
static MenuMenu_t M_NETJOIN = MAKE_MENUMENU( MGL_NETJOIN, "Join Network Game" );

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
static MenuPanel_t M_CREDITS4 = { "Meltdown Collection", MENU_CREDITS3, MA_Return, MENU_CREDITS5, MA_Advance, };
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

static MenuPassword_t M_ADULTPASSWORD = { MAXPWLOCKOUT, NULL, };

#define MAKE_MENUFILESELECT(...) { __VA_ARGS__, FNLIST_INITIALIZER, { NULL, NULL, }, 0, { 0, 0, }, }

static MenuFileSelect_t M_USERMAP = MAKE_MENUFILESELECT( "Select A User Map", { &MF_Minifont, &MF_MinifontRed, }, "*.map", boardfilename );

static const int32_t MenuFileSelect_entryheight = 8<<16;
static const int32_t MenuFileSelect_startx[2] = { 40<<16, 180<<16, };
static const int32_t MenuFileSelect_ytop[2] = { ((1+12+32+8)<<16), (1+12+32)<<16, };
static const int32_t MenuFileSelect_ybottom = (1+12+32+8*13)<<16;

// MUST be in ascending order of MenuID enum values due to binary search
static Menu_t Menus[] = {
    { MENU_MAIN, MENU_CLOSE, MA_None, Menu, &M_MAIN, },
    { MENU_MAIN_INGAME, MENU_CLOSE, MA_None, Menu, &M_MAIN_INGAME, },
    { MENU_EPISODE, MENU_MAIN, MA_Return, Menu, &M_EPISODE, },
    { MENU_USERMAP, MENU_EPISODE, MA_Return, FileSelect, &M_USERMAP, },
    { MENU_SKILL, MENU_EPISODE, MA_Return, Menu, &M_SKILL, },
    { MENU_GAMESETUP, MENU_OPTIONS, MA_Return, Menu, &M_GAMESETUP, },
    { MENU_OPTIONS, MENU_MAIN, MA_Return, Menu, &M_OPTIONS, },
    { MENU_VIDEOSETUP, MENU_DISPLAYSETUP, MA_Return, Menu, &M_VIDEOSETUP, },
    { MENU_KEYBOARDSETUP, MENU_CONTROLS, MA_Return, Menu, &M_KEYBOARDSETUP, },
    { MENU_MOUSESETUP, MENU_CONTROLS, MA_Return, Menu, &M_MOUSESETUP, },
    { MENU_JOYSTICKSETUP, MENU_CONTROLS, MA_Return, Menu, &M_JOYSTICKSETUP, },
    { MENU_JOYSTICKBTNS, MENU_JOYSTICKSETUP, MA_Return, Menu, &M_JOYSTICKBTNS, },
    { MENU_JOYSTICKAXES, MENU_JOYSTICKSETUP, MA_Return, Menu, &M_JOYSTICKAXES, },
    { MENU_KEYBOARDKEYS, MENU_KEYBOARDSETUP, MA_Return, Menu, &M_KEYBOARDKEYS, },
    { MENU_MOUSEBTNS, MENU_MOUSESETUP, MA_Return, Menu, &M_MOUSEBTNS, },
    { MENU_MOUSEADVANCED, MENU_MOUSESETUP, MA_Return, Menu, &M_MOUSEADVANCED, },
    { MENU_JOYSTICKAXIS, MENU_JOYSTICKAXES, MA_Return, Menu, &M_JOYSTICKAXIS, },
    { MENU_CONTROLS, MENU_OPTIONS, MA_Return, Menu, &M_CONTROLS, },
#ifdef USE_OPENGL
    { MENU_RENDERERSETUP, MENU_DISPLAYSETUP, MA_Return, Menu, &M_RENDERERSETUP, },
#endif
    { MENU_COLCORR, MENU_DISPLAYSETUP, MA_Return, Menu, &M_COLCORR, },
    { MENU_COLCORR_INGAME, MENU_CLOSE, MA_Return, Menu, &M_COLCORR, },
    { MENU_SCREENSETUP, MENU_DISPLAYSETUP, MA_Return, Menu, &M_SCREENSETUP, },
    { MENU_DISPLAYSETUP, MENU_OPTIONS, MA_Return, Menu, &M_DISPLAYSETUP, },
    { MENU_LOAD, MENU_MAIN, MA_Return, Menu, &M_LOAD, },
    { MENU_SAVE, MENU_MAIN, MA_Return, Menu, &M_SAVE, },
    { MENU_STORY, MENU_MAIN, MA_Return, Panel, &M_STORY, },
    { MENU_F1HELP, MENU_MAIN, MA_Return, Panel, &M_F1HELP, },
    { MENU_QUIT, MENU_PREVIOUS, MA_Return, Verify, &M_QUIT, },
    { MENU_QUITTOTITLE, MENU_PREVIOUS, MA_Return, Verify, &M_QUITTOTITLE, },
    { MENU_QUIT_INGAME, MENU_CLOSE, MA_None, Verify, &M_QUIT, },
    { MENU_NETSETUP, MENU_MAIN, MA_Return, Menu, &M_NETHOST, },
    { MENU_NETWAITMASTER, MENU_MAIN, MA_Return, Message, &M_NETWAITMASTER, },
    { MENU_NETWAITVOTES, MENU_MAIN, MA_Return, Message, &M_NETWAITVOTES, },
    { MENU_SOUND, MENU_OPTIONS, MA_Return, Menu, &M_SOUND, },
    { MENU_SOUND_INGAME, MENU_CLOSE, MA_Return, Menu, &M_SOUND, },
    { MENU_ADVSOUND, MENU_SOUND, MA_Return, Menu, &M_ADVSOUND, },
    { MENU_CREDITS, MENU_MAIN, MA_Return, Panel, &M_CREDITS, },
    { MENU_CREDITS2, MENU_MAIN, MA_Return, Panel, &M_CREDITS2, },
    { MENU_CREDITS3, MENU_MAIN, MA_Return, Panel, &M_CREDITS3, },
    { MENU_CREDITS4, MENU_MAIN, MA_Return, Panel, &M_CREDITS4, },
    { MENU_CREDITS5, MENU_MAIN, MA_Return, Panel, &M_CREDITS5, },
    { MENU_LOADVERIFY, MENU_LOAD, MA_None, Verify, &M_LOADVERIFY, },
    { MENU_NEWVERIFY, MENU_MAIN, MA_Return, Verify, &M_NEWVERIFY, },
    { MENU_SAVEVERIFY, MENU_SAVE, MA_None, Verify, &M_SAVEVERIFY, },
    { MENU_ADULTPASSWORD, MENU_GAMESETUP, MA_None, Password, &M_ADULTPASSWORD, },
    { MENU_RESETPLAYER, MENU_CLOSE, MA_None, Verify, &M_RESETPLAYER, },
    { MENU_BUYDUKE, MENU_EPISODE, MA_Return, Message, &M_BUYDUKE, },
    { MENU_NETWORK, MENU_MAIN, MA_Return, Menu, &M_NETWORK, },
    { MENU_PLAYER, MENU_OPTIONS, MA_Return, Menu, &M_PLAYER, },
    { MENU_MACROS, MENU_PLAYER, MA_Return, Menu, &M_MACROS, },
    { MENU_NETHOST, MENU_NETWORK, MA_Return, Menu, &M_NETHOST, },
    { MENU_NETOPTIONS, MENU_NETWORK, MA_Return, Menu, &M_NETOPTIONS, },
    { MENU_NETUSERMAP, MENU_NETOPTIONS, MA_Return, FileSelect, &M_USERMAP, },
    { MENU_NETJOIN, MENU_NETWORK, MA_Return, Menu, &M_NETJOIN, },
};

static const size_t numMenus = ARRAY_SIZE(Menus);

Menu_t *m_currentMenu = &Menus[0];
static Menu_t *m_previousMenu = &Menus[0];

MenuID_t g_currentMenu;
static MenuID_t g_previousMenu;


#define MenuMenu_ChangeGroupList(_, gl)\
    do {\
        _.grouplist = gl;\
        _.numGroups = ARRAY_SIZE(gl);\
    } while (0)

#define MenuGroup_ChangeEntryList(g, el)\
    do {\
        g.entrylist = el;\
        g.numEntries = ARRAY_SIZE(el);\
    } while (0)


/*
This function prepares data after ART and CON have been processed.
It also initializes some data in loops rather than statically at compile time.
*/
void M_Init(void)
{
    int32_t i, j, k;

    // prepare menu fonts
    MF_Redfont.tilenum = MF_RedfontRt.tilenum = MF_RedfontBlue.tilenum = MF_RedfontGreen.tilenum = BIGALPHANUM;
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
    k = -1;
    for (i = 0; i < MAXVOLUMES; ++i)
    {
        if (EpisodeNames[i][0])
        {
            MEL_EPISODE[i] = &ME_EPISODE[i];
            ME_EPISODE[i] = ME_EPISODE_TEMPLATE;
            ME_EPISODE[i].name = EpisodeNames[i];

            MEOSN_NetEpisodes[i] = EpisodeNames[i];

            k = i;
        }

        // prepare levels
        for (j = 0; j < MAXLEVELS; ++j)
        {
            MEOSN_NetLevels[i][j] = MapInfo[MAXLEVELS*i+j].name;
            if (MapInfo[i*MAXLEVELS+j].filename != NULL)
                MEOS_NETOPTIONS_LEVEL[i].numOptions = j+1;
        }
        MEOS_NETOPTIONS_LEVEL[i].optionNames = MEOSN_NetLevels[i];
    }
    ++k;
    MG_EPISODE.numEntries = g_numVolumes; // k;
    MEOS_NETOPTIONS_EPISODE.numOptions = g_numVolumes + 1; // k+1;
    MEOSN_NetEpisodes[g_numVolumes] = MenuUserMap;
    MP_TOP_EPISODE.pos.y = (48-(g_numVolumes*2))<<16;

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
    MG_SKILL.numEntries = g_numSkills; // k;
    MEOS_NETOPTIONS_MONSTERS.numOptions = g_numSkills + 1; // k+1;
    MEOSN_NetSkills[g_numSkills] = MenuSkillNone;
    MP_TOP_SKILL.pos.y = (58 + (4-g_numSkills)*6)<<16;
    MG_SKILL.currentEntry = 1;

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
    for (i = 0; i < 2*joynumbuttons + 2*joynumhats; ++i)
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
    MG_JOYSTICKBTNS.numEntries = 2*joynumbuttons + 2*joynumhats;
    for (i = 0; i < joynumaxes; ++i)
    {
        ME_JOYSTICKAXES[i] = ME_JOYSTICKAXES_TEMPLATE;
        Bstrncpy(MenuJoystickAxes[i], getjoyname(0, i), MAXJOYBUTTONSTRINGLENGTH);
        ME_JOYSTICKAXES[i].name = MenuJoystickAxes[i];
        MEL_JOYSTICKAXES[i] = &ME_JOYSTICKAXES[i];
    }
    MG_JOYSTICKAXES.numEntries = joynumaxes;

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
        // remove User Map
        MenuMenu_ChangeGroupList(M_EPISODE, MGL_EPISODE_SHAREWARE);

        // blue out episodes beyond the first
        for (i = 1; i < MAXVOLUMES; ++i)
        {
            if (MEL_EPISODE[i])
            {
                ME_EPISODE[i].font = &MF_RedfontBlue;
                ME_EPISODE[i].entry = &MEO_EPISODE_SHAREWARE;
            }
        }
        MEOS_NETOPTIONS_EPISODE.numOptions = 1;
        ME_NETOPTIONS_EPISODE.disabled = 1;
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
        ME_MAIN_SAVEGAME.disabled = (ud.recstat == 2);
        ME_MAIN_QUITTOTITLE.disabled = (g_netServer || numplayers > 1);
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
        ME_GAMESETUP_DEMOREC.disabled = ((ps->gm&MODE_GAME) && ud.m_recstat != 1);
        break;

#ifdef USE_OPENGL
	case MENU_DISPLAYSETUP:
        if (getrendermode() == REND_CLASSIC)
            MenuMenu_ChangeGroupList(M_DISPLAYSETUP, MGL_DISPLAYSETUP);
#ifdef POLYMER
        else if (getrendermode() == REND_POLYMER)
            MenuMenu_ChangeGroupList(M_DISPLAYSETUP, MGL_DISPLAYSETUP_GL_POLYMER);
#endif
        else
            MenuMenu_ChangeGroupList(M_DISPLAYSETUP, MGL_DISPLAYSETUP_GL);

        if (getrendermode() != REND_CLASSIC)
            for (i = (int32_t) ARRAY_SIZE(MEOSV_DISPLAYSETUP_ANISOTROPY) - 1; i >= 0; --i)
            {
                if (MEOSV_DISPLAYSETUP_ANISOTROPY[i] <= glinfo.maxanisotropy)
                {
                    MEOS_DISPLAYSETUP_ANISOTROPY.numOptions = i + 1;
                    break;
                }
            }
		break;

	case MENU_RENDERERSETUP:
        ME_RENDERERSETUP_TEXQUALITY.disabled = !usehightile;
        ME_RENDERERSETUP_PRECACHE.disabled = !usehightile;
        ME_RENDERERSETUP_TEXCACHE.disabled = !(glusetexcompr && usehightile);
        ME_RENDERERSETUP_DETAILTEX.disabled = !usehightile;
        break;
#endif

    case MENU_VIDEOSETUP:
        ME_VIDEOSETUP_APPLY.disabled = ((xdim == resolution[newresolution].xdim && ydim == resolution[newresolution].ydim && getrendermode() == newrendermode && fullscreen == newfullscreen) || (newfullscreen ? !(resolution[newresolution].flags & RES_FS) : !(resolution[newresolution].flags & RES_WIN)) || (newrendermode != REND_CLASSIC && resolution[newresolution].bppmax <= 8));
        break;

    case MENU_SOUND:
    case MENU_SOUND_INGAME:
	case MENU_ADVSOUND:
        ME_SOUND.disabled = (ud.config.FXDevice < 0);
        ME_SOUND_MUSIC.disabled = (ud.config.MusicDevice < 0);
        ME_SOUND_VOLUME_MASTER.disabled = (!ud.config.SoundToggle || ud.config.FXDevice < 0) && (!ud.config.MusicToggle || ud.config.MusicDevice < 0);
        ME_SOUND_VOLUME_EFFECTS.disabled = (!ud.config.SoundToggle || ud.config.FXDevice < 0);
        ME_SOUND_VOLUME_MUSIC.disabled = (!ud.config.MusicToggle || ud.config.MusicDevice < 0);
        ME_SOUND_DUKETALK.disabled = (!ud.config.SoundToggle || ud.config.FXDevice < 0);
        ME_SOUND_SAMPLINGRATE.disabled = (!ud.config.SoundToggle || ud.config.FXDevice < 0) && (!ud.config.MusicToggle || ud.config.MusicDevice < 0);
        ME_SOUND_SAMPLESIZE.disabled = (!ud.config.SoundToggle || ud.config.FXDevice < 0) && (!ud.config.MusicToggle || ud.config.MusicDevice < 0);
        ME_SOUND_NUMVOICES.disabled = (!ud.config.SoundToggle || ud.config.FXDevice < 0);
        ME_SOUND_RESTART.disabled = (soundrate == ud.config.MixRate && soundvoices == ud.config.NumVoices && soundbits == ud.config.NumBits);
        break;

    case MENU_MOUSESETUP:
        ME_MOUSESETUP_MOUSEAIMING.disabled = ud.mouseaiming;
        break;

    case MENU_NETOPTIONS:
        if (MEO_NETOPTIONS_EPISODE.currentOption == MEOS_NETOPTIONS_EPISODE.numOptions-1)
            MEL_NETOPTIONS[2] = &ME_NETOPTIONS_USERMAP;
        else
        {
            MEL_NETOPTIONS[2] = &ME_NETOPTIONS_LEVEL;
            MEO_NETOPTIONS_LEVEL.options = &MEOS_NETOPTIONS_LEVEL[MEO_NETOPTIONS_EPISODE.currentOption];
        }
        MEL_NETOPTIONS[4] = (GametypeFlags[ud.m_coop] & GAMETYPE_MARKEROPTION) ? &ME_NETOPTIONS_MARKERS : &ME_NETOPTIONS_MARKERS_DISABLED;
        MEL_NETOPTIONS[5] = (GametypeFlags[ud.m_coop] & (GAMETYPE_PLAYERSFRIENDLY|GAMETYPE_TDM)) ? &ME_NETOPTIONS_FRFIRE : &ME_NETOPTIONS_MAPEXITS;
        break;

    case MENU_OPTIONS:
        ME_OPTIONS_PLAYERSETUP.disabled = ud.recstat == 1;
        ME_OPTIONS_JOYSTICKSETUP.disabled = CONTROL_JoyPresent == 0;
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


static void M_PreMenuDraw(MenuID_t cm, MenuGroup_t *group, MenuEntry_t *entry, const vec2_t origin)
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
        if (group == &MG_KEYBOARDSETUPFUNCS)
        {
            mgametextcenter(origin.x, origin.y + ((144+9+3)<<16), "Up/Down = Select Action");
            mgametextcenter(origin.x, origin.y + ((144+9+9+3)<<16), "Left/Right = Select List");
            mgametextcenter(origin.x, origin.y + ((144+9+9+9+3)<<16), "Enter = Modify   Delete = Clear");
        }
        break;

    case MENU_MOUSESETUP:
        if (entry == &ME_MOUSESETUP_MOUSEAIMING)
        {
            if (entry->disabled)
            {
                mgametextcenter(origin.x, origin.y + ((140+9+9+9)<<16), "Set mouse aim type to toggle on/off");
                mgametextcenter(origin.x, origin.y + ((140+9+9+9+9)<<16), "in the Player Setup menu to enable");
            }
        }
        break;

    case MENU_MOUSEBTNS:
        if (group == &MG_MOUSESETUPBTNS)
        {
            mgametextcenter(origin.x, origin.y + ((160+9)<<16), "Up/Down = Select Button");
            mgametextcenter(origin.x, origin.y + ((160+9+9)<<16), "Enter = Modify");
        }
        break;

    case MENU_MOUSEADVANCED:
        if (group == &MG_MOUSEADVANCED_DAXES)
        {
            mgametextcenter(origin.x, origin.y + ((144+9+9)<<16), "Digital axes are not for mouse look");
            mgametextcenter(origin.x, origin.y + ((144+9+9+9)<<16), "or for aiming up and down");
        }
        break;

    case MENU_JOYSTICKBTNS:
        if (group == &MG_JOYSTICKBTNS)
        {
            mgametextcenter(origin.x, origin.y + (149<<16), "Up/Down = Select Button");
            mgametextcenter(origin.x, origin.y + ((149+9)<<16), "Enter = Modify");
        }
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

        if (ud.savegame[group->currentEntry][0])
        {
            rotatesprite_fs(origin.x + (101<<16), origin.y + (97<<16), 65536>>1,512,TILE_LOADSHOT,-32,0,4+10+64);

            if (g_oldverSavegame[group->currentEntry])
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

        if (ud.savegame[group->currentEntry][0])
        {
            j = 0;
            for (i = 0; i < 10; ++i)
                if (((MenuString_t*)group->entrylist[i]->entry)->editfield)
                    j |= 1;

            rotatesprite_fs(origin.x + (101<<16), origin.y + (97<<16), 65536L>>1,512,j?TILE_SAVESHOT:TILE_LOADSHOT,-32,0,4+10+64);

            if (g_oldverSavegame[group->currentEntry])
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
        }
        else
            mmenutext(origin.x + (69<<16), origin.y + (70<<16), "Empty");

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
        char *s[MAXSKILLS] = {"EASY - Few enemies, and lots of stuff.", "MEDIUM - Normal difficulty.", "HARD - For experienced players.", "EXPERTS - Lots of enemies, plus they respawn!" };
        mgametextcenter(origin.x, origin.y + (168<<16), s[group->currentEntry]);
    }
        break;
#endif

    case MENU_LOADVERIFY:
		fade_screen_black(1);
		if (g_oldverSavegame[MG_LOAD.currentEntry])
		{
			mgametextcenter(origin.x, origin.y + (90<<16), "Start new game:");
			Bsprintf(tempbuf,"%s / %s",MapInfo[(ud.volume_number*MAXLEVELS) + ud.level_number].name, SkillNames[ud.player_skill-1]);
			mgametextcenter(origin.x, origin.y + (99<<16), tempbuf);
		}
		else
		{
			mgametextcenter(origin.x, origin.y + (90<<16), "Load game:");
			Bsprintf(tempbuf, "\"%s\"", ud.savegame[MG_LOAD.currentEntry]);
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
        mgametextcenter(origin.x, origin.y + (50<<16), "Waiting for master");
        mgametextcenter(origin.x, origin.y + (59<<16), "to select level");
        break;

    case MENU_NETWAITVOTES:
        G_DrawFrags();
        mgametextcenter(origin.x, origin.y + (90<<16), "Waiting for votes");
        break;

    case MENU_BUYDUKE:
        mgametextcenter(origin.x, origin.y + ((41-8)<<16), "You are playing the shareware");
        mgametextcenter(origin.x, origin.y + ((50-8)<<16), "version of Duke Nukem 3D.  While");
        mgametextcenter(origin.x, origin.y + ((59-8)<<16), "this version is really cool, you");
        mgametextcenter(origin.x, origin.y + ((68-8)<<16), "are missing over 75% of the total");
        mgametextcenter(origin.x, origin.y + ((77-8)<<16), "game, along with other great extras");
        mgametextcenter(origin.x, origin.y + ((86-8)<<16), "which you'll get when you order");
        mgametextcenter(origin.x, origin.y + ((95-8)<<16), "the complete version and get");
        mgametextcenter(origin.x, origin.y + ((104-8)<<16), "the final three episodes.");

#ifndef DROIDMENU
        mgametextcenter(origin.x, origin.y + ((104+8)<<16), "Please visit Steam and purchase");
		mgametextcenter(origin.x, origin.y + ((113+8)<<16), "Duke Nukem 3D: Megaton Edition");
#else
		mgametextcenter(origin.x, origin.y + ((113+8)<<16), "Please visit the Play Store");
#endif

        mgametextcenter(origin.x, origin.y + ((122+8)<<16), "to upgrade to the full registered");
        mgametextcenter(origin.x, origin.y + ((131+8)<<16), "version of Duke Nukem 3D.");

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
                    mgametextcenter(origin.x, origin.y + (106<<16), "Please read LICENSE.DOC for shareware");
                    mgametextcenter(origin.x, origin.y + ((106+9)<<16), "distribution grants and restrictions.");
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

#ifdef DROIDMENU
        mgametextcenter(origin.x, origin.y + ((110+7-l)<<16), "Android support programming", 0);
        creditsminitext(origin.x + (160<<16), origin.y + ((110+7+10-l)<<16), "Emile Belanger", 8);
#endif

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



static void M_PreMenuInput(MenuGroup_t *group, MenuEntry_t *entry)
{
    switch (g_currentMenu)
    {

    case MENU_KEYBOARDKEYS:
        if (group == &MG_KEYBOARDSETUPFUNCS)
        {
            if (KB_KeyPressed(sc_Delete))
            {
                MenuCustom2Col_t *column = (MenuCustom2Col_t*)entry->entry;
                char key[2];
                key[0] = ud.config.KeyboardKeys[group->currentEntry][0];
                key[1] = ud.config.KeyboardKeys[group->currentEntry][1];
                *column->column[group->currentColumn] = 0xff;
                CONFIG_MapKey(group->currentEntry, ud.config.KeyboardKeys[group->currentEntry][0], key[0], ud.config.KeyboardKeys[group->currentEntry][1], key[1]);
                S_PlaySound(KICK_HIT);
                KB_ClearKeyDown(sc_Delete);
            }
        }
        break;

    default:
        break;
    }
}

static void M_PreMenuOptionListDraw(/*MenuGroup_t *group,*/ MenuEntry_t *entry, const vec2_t origin)
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

static int32_t M_PreMenuCustom2ColScreen(MenuGroup_t *group, MenuEntry_t *entry)
{
    if (g_currentMenu == MENU_KEYBOARDKEYS)
    {
        MenuCustom2Col_t *column = (MenuCustom2Col_t*)entry->entry;

        int32_t sc = KB_GetLastScanCode();
        if (sc != sc_None)
        {
            char key[2];
            key[0] = ud.config.KeyboardKeys[group->currentEntry][0];
            key[1] = ud.config.KeyboardKeys[group->currentEntry][1];

            S_PlaySound(PISTOL_BODYHIT);

            *column->column[group->currentColumn] = KB_GetLastScanCode();

            CONFIG_MapKey(group->currentEntry, ud.config.KeyboardKeys[group->currentEntry][0], key[0], ud.config.KeyboardKeys[group->currentEntry][1], key[1]);

            KB_ClearKeyDown(sc);

            return -1;
        }
    }

    return 0;
}

static void M_PreMenuCustom2ColScreenDraw(MenuGroup_t *group, MenuEntry_t *entry, const vec2_t origin)
{
    if (g_currentMenu == MENU_KEYBOARDKEYS)
    {
        mgametextcenter(origin.x, origin.y + (90<<16), "Press the key to assign as");
        Bsprintf(tempbuf,"%s for \"%s\"", group->currentColumn?"secondary":"primary", entry->name);
        mgametextcenter(origin.x, origin.y + ((90+9)<<16), tempbuf);
        mgametextcenter(origin.x, origin.y + ((90+9+9+9)<<16), "Press \"Escape\" To Cancel");
    }
}

static void M_MenuEntryFocus(MenuGroup_t *group/*, MenuEntry_t *entry*/)
{
    switch (g_currentMenu)
    {
    case MENU_LOAD:
    case MENU_SAVE:
        G_LoadSaveHeaderNew(group->currentEntry, &savehead);
        break;

    default:
        break;
    }
}

/*
Functions where a "newValue" or similar is passed are run *before* the linked variable is actually changed.
That way you can compare the new and old values and potentially block the change.
*/
static void M_MenuEntryLinkActivate(MenuGroup_t *group, MenuEntry_t *entry)
{
    switch (g_currentMenu)
    {
    case MENU_EPISODE:
        if (group == &MG_EPISODE)
        {
            ud.m_volume_number = group->currentEntry;
            ud.m_level_number = 0;
        }
        break;

    case MENU_SKILL:
    {
        int32_t skillsound = 0;

        switch (group->currentEntry)
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

        g_skillSoundVoice = S_PlaySound(skillsound);

        ud.m_player_skill = group->currentEntry+1;
        if (group->currentEntry == 3) ud.m_respawn_monsters = 1;
        else ud.m_respawn_monsters = 0;

        ud.m_monsters_off = ud.monsters_off = 0;

        ud.m_respawn_items = 0;
        ud.m_respawn_inventory = 0;

        ud.multimode = 1;

        if (ud.m_volume_number == 3 && (G_GetLogoFlags() & LOGO_NOE4CUTSCENE)==0)
        {
            flushperms();
            setview(0,0,xdim-1,ydim-1);
            clearview(0L);
            nextpage();
        }

        G_NewGame_EnterLevel();
        break;
    }

    case MENU_JOYSTICKAXES:
        M_JOYSTICKAXIS.title = getjoyname(0, group->currentEntry);
        MEO_JOYSTICKAXIS_ANALOG.data = &ud.config.JoystickAnalogueAxes[group->currentEntry];
        MEO_JOYSTICKAXIS_SCALE.variable = &ud.config.JoystickAnalogueScale[group->currentEntry];
        MEO_JOYSTICKAXIS_DEAD.variable = &ud.config.JoystickAnalogueDead[group->currentEntry];
        MEO_JOYSTICKAXIS_SATU.variable = &ud.config.JoystickAnalogueSaturate[group->currentEntry];
        MEO_JOYSTICKAXIS_DIGITALNEGATIVE.data = &ud.config.JoystickDigitalFunctions[group->currentEntry][0];
        MEO_JOYSTICKAXIS_DIGITALPOSITIVE.data = &ud.config.JoystickDigitalFunctions[group->currentEntry][1];
        break;

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
        ud.config.NumBits = soundbits;

        S_SoundShutdown();
        S_MusicShutdown();

        S_MusicStartup();
        S_SoundStartup();

        FX_StopAllSounds();
        S_ClearSoundLocks();

        if (ud.config.MusicToggle == 1)
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

static int32_t M_MenuEntryOptionModify(MenuGroup_t* group, MenuEntry_t *entry, int32_t newOption)
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
        gltexfiltermode = newOption ? 5 : 2;
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
    else if (entry == &ME_DISPLAYSETUP_ANISOTROPY)
    {
        glanisotropy = newOption;
        gltexapplyprops();
    }
    else if (entry == &ME_DISPLAYSETUP_VSYNC)
        setvsync(newOption);
    else if (entry == &ME_RENDERERSETUP_TEXQUALITY)
    {
        r_downsize = newOption;
        texcache_invalidate();
        resetvideomode();
        if (setgamemode(fullscreen,xdim,ydim,bpp))
            OSD_Printf("restartvid: Reset failed...\n");
        r_downsizevar = r_downsize;
    }
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
        CONTROL_MapAnalogAxis(MG_JOYSTICKAXES.currentEntry, newOption, controldevice_joystick);
    else if (entry == &ME_NETOPTIONS_EPISODE)
    {
        if (newOption < MAXVOLUMES)
            ud.m_volume_number = newOption;
    }
    else if (entry == &ME_NETOPTIONS_MONSTERS)
    {
        ud.m_monsters_off = (newOption == MAXSKILLS);
        if (newOption < MAXSKILLS)
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

    if (entry == &ME_GAMESETUP_AIM_AUTO || entry == &ME_GAMESETUP_WEAPSWITCH_PICKUP)
        G_UpdatePlayerFromMenu();

    switch (g_currentMenu)
    {
    case MENU_MOUSEBTNS:
    case MENU_MOUSEADVANCED:
    case MENU_JOYSTICKBTNS:
    case MENU_JOYSTICKAXIS:
        if (group == &MG_MOUSESETUPBTNS)
        {
            CONTROL_MapButton(newOption, MenuMouseDataIndex[group->currentEntry][0], MenuMouseDataIndex[group->currentEntry][1], controldevice_mouse);
            CONTROL_FreeMouseBind(MenuMouseDataIndex[group->currentEntry][0]);
        }
        else if (group == &MG_MOUSEADVANCED_DAXES)
        {
            CONTROL_MapDigitalAxis(group->currentEntry>>1, newOption, group->currentEntry&1, controldevice_mouse);
        }
        else if (group == &MG_JOYSTICKBTNS)
        {
            CONTROL_MapButton(newOption, group->currentEntry>>1, group->currentEntry&1, controldevice_joystick);
        }
        else if (group == &MG_JOYSTICKAXIS_DIGITAL)
        {
            CONTROL_MapDigitalAxis(group->currentEntry>>1, newOption, group->currentEntry&1, controldevice_joystick);
        }
        break;
    }

    return 0;
}

static void M_MenuCustom2ColScreen(MenuGroup_t *group/*, MenuEntry_t *entry*/)
{
    if (group == &MG_KEYBOARDSETUPFUNCS)
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
        CONTROL_SetAnalogAxisScale(MG_JOYSTICKAXES.currentEntry, newValue, controldevice_joystick);
    else if (entry == &ME_JOYSTICKAXIS_DEAD)
        setjoydeadzone(MG_JOYSTICKAXES.currentEntry, newValue, *MEO_JOYSTICKAXIS_SATU.variable);
    else if (entry == &ME_JOYSTICKAXIS_SATU)
        setjoydeadzone(MG_JOYSTICKAXES.currentEntry, *MEO_JOYSTICKAXIS_DEAD.variable, newValue);

    return 0;
}

static int32_t M_MenuEntryRangeFloatModify(MenuEntry_t *entry, float newValue)
{
    if (entry == &ME_COLCORR_GAMMA)
    {
        vid_gamma = newValue;
        ud.brightness = GAMMA_CALC<<2;
        setbrightness(ud.brightness>>2, g_player[myconnectindex].ps->palette, 0);
        return 0;
    }

    if (entry == &ME_COLCORR_CONTRAST)
    {
        vid_contrast = newValue;
        setbrightness(ud.brightness>>2, g_player[myconnectindex].ps->palette, 0);
        return 0;
    }

    if (entry == &ME_COLCORR_BRIGHTNESS)
    {
        vid_brightness = newValue;
        setbrightness(ud.brightness>>2, g_player[myconnectindex].ps->palette, 0);
        return 0;
    }

    if (entry == &ME_COLCORR_AMBIENT)
    {
        r_ambientlightrecip = 1.f/newValue;
        return 0;
    }

    return 0;
}

static int32_t M_MenuEntryRangeDoubleModify(void /*MenuEntry_t *entry, double newValue*/)
{

    return 0;
}

static uint32_t save_xxh = 0;

static void M_MenuEntryStringActivate(MenuGroup_t *group/*, MenuEntry_t *entry*/)
{
    switch (g_currentMenu)
    {
    case MENU_SAVE:
        if (!save_xxh)
            save_xxh = XXH32((uint8_t *)&ud.savegame[group->currentEntry][0], 19, 0xDEADBEEF);
        if (ud.savegame[group->currentEntry][0])
            M_ChangeMenu(MENU_SAVEVERIFY);
        break;

    default:
        break;
    }
}

static int32_t M_MenuEntryStringSubmit(MenuGroup_t *group, MenuEntry_t *entry, char *input)
{
    MenuString_t *object = (MenuString_t*)entry->entry;
    int32_t returnvar = 0;

    switch (g_currentMenu)
    {
    case MENU_SAVE:
        // dirty hack... char 127 in last position indicates an auto-filled name
        if (ud.savegame[group->currentEntry][0] == 0 || (ud.savegame[group->currentEntry][MAXSAVEGAMENAME-2] == 127 &&
            save_xxh == XXH32((uint8_t *)&ud.savegame[group->currentEntry][0], 19, 0xDEADBEEF)))
        {
            Bstrncpy(&ud.savegame[group->currentEntry][0], MapInfo[ud.volume_number * MAXLEVELS + ud.level_number].name, 19);
            ud.savegame[group->currentEntry][MAXSAVEGAMENAME-2] = 127;
            returnvar = -1;
        }
        else
            Bstrncpy(object->variable, input, object->maxlength);

        G_SavePlayerMaybeMulti(group->currentEntry);

        g_lastSaveSlot = group->currentEntry;
        g_player[myconnectindex].ps->gm = MODE_GAME;

        M_ChangeMenu(MENU_CLOSE);
        save_xxh = 0;
        break;

    default:
        break;
    }

    return returnvar;
}

static void M_MenuEntryStringCancel(/*MenuGroup_t *group, MenuEntry_t *entry*/)
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
    else if (entry == &ME_NETOPTIONS_EPISODE)
        return (currentValue < MAXVOLUMES ? ud.m_volume_number : MAXVOLUMES);
    else if (entry == &ME_NETOPTIONS_MONSTERS)
        return (ud.m_monsters_off ? MAXSKILLS : ud.m_player_skill);

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
            g_lastSaveSlot = MG_LOAD.currentEntry;

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

            ((MenuString_t*)MG_SAVE.entrylist[MG_SAVE.currentEntry]->entry)->editfield = NULL;
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

static void M_MenuPasswordSubmit(char *input)
{
    switch (g_currentMenu)
    {
    case MENU_ADULTPASSWORD:
        if (Bstrlen(input) && (ud.pwlockout[0] == 0 || ud.lockout == 0))
            Bstrcpy(&ud.pwlockout[0], input);
        else if (Bstrcmp(input, &ud.pwlockout[0]) == 0)
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

        M_ChangeMenu(MENU_GAMESETUP);
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
            M_ChangeMenuAnimate(MENU_SKILL, MA_Advance);
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
    int32_t (*func)(void);
    Menu_t *a;
    Menu_t *b;
    int32_t start;
    int32_t length;
} MenuAnimation_t;

static MenuAnimation_t m_animation;

int32_t M_Anim_SinRight(void)
{
    return sintable[scale(1024, totalclock - m_animation.start, m_animation.length) + 512] + 16384;
}
int32_t M_Anim_SinLeft(void)
{
    return -sintable[scale(1024, totalclock - m_animation.start, m_animation.length) + 512] + 16384;
}

void M_ChangeMenuAnimate(int32_t cm, MenuAnimationType_t animtype)
{
    switch (animtype)
    {
        case MA_Advance:
            m_animation.func = M_Anim_SinRight;
            m_animation.start = totalclock;
            m_animation.length = 30;

            m_animation.a = m_currentMenu;
            M_ChangeMenu(cm);
            m_animation.b = m_currentMenu;
            break;
        case MA_Return:
            m_animation.func = M_Anim_SinLeft;
            m_animation.start = totalclock;
            m_animation.length = 30;

            m_animation.b = m_currentMenu;
            M_ChangeMenu(cm);
            m_animation.a = m_currentMenu;
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

    cm = VM_OnEvent(EVENT_CHANGEMENU, g_player[myconnectindex].ps->i, myconnectindex, -1, cm);

    if (cm == MENU_PREVIOUS)
    {
        m_currentMenu = m_previousMenu;
        g_currentMenu = g_previousMenu;
    }
    else if (cm == MENU_CLOSE)
    {
        if (g_player[myconnectindex].ps->gm & MODE_GAME)
        {
            g_player[myconnectindex].ps->gm &= ~MODE_MENU;
            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
                CAMERACLOCK = totalclock;
                CAMERADIST = 65536;
                m_animation.start = 0;
                m_animation.length = 0;
            }
            walock[TILE_SAVESHOT] = 199;
            G_UpdateScreenArea();
        }
    }
    else if (cm >= 0)
    {
        if ((g_player[myconnectindex].ps->gm&MODE_GAME) && cm == MENU_MAIN)
            cm = MENU_MAIN_INGAME;

        search = M_FindMenu(cm);

        if (search == NULL)
            return;

        m_previousMenu = m_currentMenu;
        m_currentMenu = search;
        g_currentMenu = cm;
    }
    else
        return;

    switch (g_currentMenu)
    {
    case MENU_SAVE:
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
        soundbits = ud.config.NumBits;
        break;

    default:
        break;
    }

    if (m_currentMenu->type == Password)
    {
        typebuf[0] = 0;
        ((MenuPassword_t*)m_currentMenu->object)->input = typebuf;
    }
    else if (m_currentMenu->type == FileSelect)
        M_MenuFileSelectInit((MenuFileSelect_t*)m_currentMenu->object);
    else if (m_currentMenu->type == Menu)
    {
        MenuMenu_t *menu = (MenuMenu_t*)m_currentMenu->object;
        MenuGroup_t* currgroup = menu->grouplist[menu->currentGroup];
        // MenuEntry_t* currentry = currgroup->entrylist[currgroup->currentEntry];

        M_MenuEntryFocus(currgroup/*, currentry*/);
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
        case Password:
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
            MenuGroup_t *group = menu->grouplist[menu->currentGroup];
            MenuEntry_t *entry = group->entrylist[group->currentEntry];
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

static void M_BlackRectangle(int32_t x, int32_t y, int32_t width, int32_t height)
{
    const int32_t xscale = scale(65536, width, tilesiz[0].x<<16), yscale = scale(65536, height, tilesiz[0].y<<16);

    if (xscale >= yscale)
    {
        const int32_t patchwidth = scale(tilesiz[0].x<<16, yscale, 65536);
        const int32_t finalx = x + width - patchwidth;

        while (x < finalx)
        {
            rotatesprite_fs(x, y, yscale, 0, 0, 127, 4, 2|8|16|64);
            x += patchwidth;
        }

        rotatesprite_fs(finalx, y, yscale, 0, 0, 127, 4, 2|8|16|64);
    }
    else
    {
        const int32_t patchheight = scale(tilesiz[0].y<<16, xscale, 65536);
        const int32_t finaly = y + height - patchheight;

        while (y < finaly)
        {
            rotatesprite_fs(x, y, xscale, 0, 0, 127, 4, 2|8|16|64);
            y += patchheight;
        }

        rotatesprite_fs(x, finaly, xscale, 0, 0, 127, 4, 2|8|16|64);
    }
}

static void M_ShadePal(MenuTextType_t *font, uint8_t status, int32_t *s, int32_t *p)
{
    *s = (status & (1<<0)) ? (sintable[(totalclock<<5)&2047]>>12) : font->shade_deselected;
    *p = (status & (1<<1)) ? font->pal_disabled : font->pal;
}

static vec2_t M_MenuText(int32_t x, int32_t y, MenuTextType_t *font, const char *t, uint8_t status)
{
    int32_t s, p, ybetween = font->ybetween;
    int32_t f = font->textflags;
    if (status & (1<<2))
        f |= TEXT_XCENTER;
    if (status & (1<<3))
        f |= TEXT_XRIGHT;
    if (status & (1<<4))
    {
        f |= TEXT_YCENTER | TEXT_YOFFSETZERO;
        ybetween = font->yline; // <^ the battle against 'Q'
    }
    if (status & (1<<5))
        f |= TEXT_LITERALESCAPE;

    M_ShadePal(font, status, &s, &p);

    return G_ScreenText(font->tilenum, x, y, 65536, 0, 0, t, s, p, 2|8|16|ROTATESPRITE_FULL16, 0, font->xspace, font->yline, font->xbetween, ybetween, f, 0, 0, xdim-1, ydim-1);
}

#if 0
static vec2_t M_MenuTextSize(int32_t x, int32_t y, MenuTextType_t *font, const char *t, uint8_t status)
{
    int32_t f = font->textflags;
    if (status & (1<<5))
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

static int32_t M_RunMenu_MenuMenu(MenuMenu_t *menu, MenuEntry_t *currentry, int32_t state, const vec2_t origin)
{
    const int32_t cursorShade = 4-(sintable[(totalclock<<4)&2047]>>11);
    int32_t g, rely = INT32_MIN, srcy = INT32_MIN, totalextent = 0;

    menu->ytop = INT32_MAX;
    menu->bottomcutoff = INT32_MIN;

    // find min/max
    for (g = 0; g < menu->numGroups; ++g)
    {
        MenuGroup_t *group = menu->grouplist[g];
        int32_t realcutoff;

        if (group == NULL || group->numEntries == 0)
            continue;

        if (group->position->pos.y < menu->ytop)
            menu->ytop = group->position->pos.y;

        realcutoff = group->position->bottomcutoff > 0 ? group->position->bottomcutoff : -group->position->bottomcutoff;

        if (realcutoff > menu->bottomcutoff)
            menu->bottomcutoff = realcutoff;
    }

    // iterate through menu
    for (g = 0; g < menu->numGroups; ++g)
    {
        MenuGroup_t *group = menu->grouplist[g];
        int32_t e, y;
        int32_t calculatedentryspacing = 0;
        int32_t realcutoff;

        if (group == NULL || group->numEntries == 0)
            continue;

        realcutoff = group->position->bottomcutoff > 0 ? group->position->bottomcutoff : -group->position->bottomcutoff;

        y = group->position->pos.y;

        if (y == srcy)
            y = rely + group->position->groupspacing;
        else
            srcy = y;

        if (group->position->bottomcutoff < 0)
        {
            int32_t totalheight = 0, numvalidentries = 0;

            for (e = 0; e < group->numEntries; ++e)
            {
                MenuEntry_t *entry = group->entrylist[e];

                if (entry == NULL)
                    continue;

                ++numvalidentries;

                // assumes height == font->yline!
                totalheight += entry->font->yline;
            }

            calculatedentryspacing = (realcutoff - group->position->pos.y - totalheight) / (numvalidentries > 1 ? numvalidentries - 1 : 1);
        }

        if (!calculatedentryspacing || calculatedentryspacing > group->position->entryspacing)
            calculatedentryspacing = group->position->entryspacing;

        for (e = 0; e < group->numEntries; ++e)
        {
            MenuEntry_t *entry = group->entrylist[e];
            uint8_t status = 0;
            int32_t height, x;
            // vec2_t textsize;
            int32_t dodraw = 1;

            if (entry == NULL)
                continue;

            x = group->position->pos.x;

            status |= (g == menu->currentGroup && e == group->currentEntry)<<0;
            status |= (entry->disabled)<<1;
            status |= (group->position->width == 0)<<2;

            dodraw &= menu->ytop <= y - menu->scrollPos && y - menu->scrollPos + entry->font->yline <= menu->bottomcutoff;

            if (dodraw)
            /*textsize =*/ M_MenuText(origin.x + x, origin.y + y - menu->scrollPos, entry->font, entry->name, status);

            height = entry->font->yline; // max(textsize.y, entry->font->yline); // bluefont Q ruins this

            status |= (group->position->width < 0)<<3 | (1<<4);

            if (dodraw && (status & (1<<0)) && state != 1)
            {
                if (status & (1<<2))
                {
                    rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16) + group->position->cursorPosition, origin.y + y + (height>>1) - menu->scrollPos, group->position->cursorScale, 0, SPINNINGNUKEICON+6-((6+(totalclock>>3))%7), cursorShade, 0, 10);
                    rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16) - group->position->cursorPosition, origin.y + y + (height>>1) - menu->scrollPos, group->position->cursorScale, 0, SPINNINGNUKEICON+((totalclock>>3)%7), cursorShade, 0, 10);
                }
                else
                    rotatesprite_fs(origin.x + x - group->position->cursorPosition, origin.y + y + (height>>1) - menu->scrollPos, group->position->cursorScale, 0, SPINNINGNUKEICON+(((totalclock>>3))%7), cursorShade, 0, 10);
            }

            x += klabs(group->position->width);

            if (dodraw)
            switch (entry->type)
            {
                case Dummy:
                    break;
                case Link:
                    break;
                case Option:
                {
                    MenuOption_t *object = (MenuOption_t*)entry->entry;
                    int32_t currentOption = M_FindOptionBinarySearch(object, object->data == NULL ? M_MenuEntryOptionSource(entry, object->currentOption) : *object->data, 0, object->options->numOptions);

                    if (currentOption >= 0)
                        object->currentOption = currentOption;

                    M_MenuText(origin.x + x, origin.y + y + (height>>1) - menu->scrollPos, object->font,
                        currentOption < 0 ? MenuCustom : currentOption < object->options->numOptions ? object->options->optionNames[currentOption] : NULL,
                        status);
                    break;
                }
                case Custom2Col:
                {
                    MenuCustom2Col_t *object = (MenuCustom2Col_t*)entry->entry;
                    M_MenuText(origin.x + x - ((status & (1<<3)) ? object->columnWidth : 0), origin.y + y + (height>>1) - menu->scrollPos, object->font, object->key[*object->column[0]], status & ~((group->currentColumn != 0)<<0));
                    M_MenuText(origin.x + x + ((status & (1<<3)) ? 0 : object->columnWidth), origin.y + y + (height>>1) - menu->scrollPos, object->font, object->key[*object->column[1]], status & ~((group->currentColumn != 1)<<0));
                    break;
                }
                case RangeInt32:
                {
                    MenuRangeInt32_t *object = (MenuRangeInt32_t*)entry->entry;

                    int32_t s, p;
                    const int32_t z = scale(65536, height, tilesiz[SLIDEBAR].y<<16);
                    M_ShadePal(object->font, status, &s, &p);

                    if (status & (1<<3))
                        x -= scale(tilesiz[SLIDEBAR].x<<16, height, tilesiz[SLIDEBAR].y<<16);

                    rotatesprite_fs(origin.x + x, origin.y + y - menu->scrollPos, z, 0, SLIDEBAR, s, entry->disabled ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16);

                    rotatesprite_fs(
                    origin.x + x + (1<<16) + scale(scale((tilesiz[SLIDEBAR].x-2-tilesiz[SLIDEBAR+1].x)<<16, height, tilesiz[SLIDEBAR].y<<16), *object->variable - object->min, object->max - object->min),
                    origin.y + y + scale((tilesiz[SLIDEBAR].y-tilesiz[SLIDEBAR+1].y)<<15, height, tilesiz[SLIDEBAR].y<<16) - menu->scrollPos,
                    z, 0, SLIDEBAR+1, s, entry->disabled ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16);

                    if (object->displaytype > 0)
                    {
                        status |= (1<<3);

                        if (object->onehundredpercent == 0)
                            object->onehundredpercent = object->max;

                        switch (object->displaytype)
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

                        M_MenuText(origin.x + x - (4<<16), origin.y + y + (height>>1) - menu->scrollPos, object->font, tempbuf, status);
                    }
                    break;
                }
                case RangeFloat:
                {
                    MenuRangeFloat_t *object = (MenuRangeFloat_t*)entry->entry;

                    int32_t s, p;
                    const int32_t z = scale(65536, height, tilesiz[SLIDEBAR].y<<16);
                    M_ShadePal(object->font, status, &s, &p);

                    if (status & (1<<3))
                        x -= scale(tilesiz[SLIDEBAR].x<<16, height, tilesiz[SLIDEBAR].y<<16);

                    rotatesprite_fs(origin.x + x, origin.y + y - menu->scrollPos, z, 0, SLIDEBAR, s, p, 2|8|16|ROTATESPRITE_FULL16);

                    rotatesprite_fs(
                    origin.x + x + (1<<16) + (int32_t)((float) scale((tilesiz[SLIDEBAR].x-2-tilesiz[SLIDEBAR+1].x)<<16, height, tilesiz[SLIDEBAR].y<<16) * (*object->variable - object->min) / (object->max - object->min)),
                    origin.y + y + scale((tilesiz[SLIDEBAR].y-tilesiz[SLIDEBAR+1].y)<<15, height, tilesiz[SLIDEBAR].y<<16) - menu->scrollPos,
                    z, 0, SLIDEBAR+1, s, p, 2|8|16|ROTATESPRITE_FULL16);

                    if (object->displaytype > 0)
                    {
                        status |= (1<<3);

                        if (object->onehundredpercent == 0)
                            object->onehundredpercent = 1.f;

                        switch (object->displaytype)
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

                        M_MenuText(origin.x + x - (4<<16), origin.y + y + (height>>1) - menu->scrollPos, object->font, tempbuf, status);
                    }
                    break;
                }
                case RangeDouble:
                {
                    MenuRangeDouble_t *object = (MenuRangeDouble_t*)entry->entry;

                    int32_t s, p;
                    const int32_t z = scale(65536, height, tilesiz[SLIDEBAR].y<<16);
                    M_ShadePal(object->font, status, &s, &p);

                    if (status & (1<<3))
                        x -= scale(tilesiz[SLIDEBAR].x<<16, height, tilesiz[SLIDEBAR].y<<16);

                    rotatesprite_fs(origin.x + x, origin.y + y - menu->scrollPos, z, 0, SLIDEBAR, s, p, 2|8|16|ROTATESPRITE_FULL16);

                    rotatesprite_fs(
                    origin.x + x + (1<<16) + (int32_t)((double) scale((tilesiz[SLIDEBAR].x-2-tilesiz[SLIDEBAR+1].x)<<16, height, tilesiz[SLIDEBAR].y<<16) * (*object->variable - object->min) / (object->max - object->min)),
                    origin.y + y + scale((tilesiz[SLIDEBAR].y-tilesiz[SLIDEBAR+1].y)<<15, height, tilesiz[SLIDEBAR].y<<16) - menu->scrollPos,
                    z, 0, SLIDEBAR+1, s, p, 2|8|16|ROTATESPRITE_FULL16);

                    if (object->displaytype > 0)
                    {
                        status |= (1<<3);

                        if (object->onehundredpercent == 0)
                            object->onehundredpercent = 1.;

                        switch (object->displaytype)
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

                        M_MenuText(origin.x + x - (4<<16), origin.y + y + (height>>1) - menu->scrollPos, object->font, tempbuf, status);
                    }
                    break;
                }
                case String:
                {
                    MenuString_t *object = (MenuString_t*)entry->entry;

                    if (entry == currentry && object->editfield != NULL)
                    {
                        const vec2_t dim = M_MenuText(x, y + (height>>1), object->font, object->editfield, status | (1<<5));
                        const int32_t h = max(dim.y, entry->font->yline);

                        rotatesprite_fs(origin.x + x + dim.x + (1<<16) + scale(tilesiz[SPINNINGNUKEICON].x<<15, origin.y + h, tilesiz[SPINNINGNUKEICON].y<<16), y + (height>>1) - menu->scrollPos, scale(65536, h, tilesiz[SPINNINGNUKEICON].y<<16), 0, SPINNINGNUKEICON+(((totalclock>>3))%7), cursorShade, 0, 10);
                    }
                    else
                        M_MenuText(origin.x + x, origin.y + y + (height>>1) - menu->scrollPos, object->font, object->variable, status);
                    break;
                }
            }

            // prepare for the next line
            entry->ytop = y;

            y += height;
            entry->ybottom = y;
            totalextent = y;

            y += calculatedentryspacing;
            rely = y;
        }
    }

    menu->totalHeight = totalextent - menu->ytop;

    // draw indicators if applicable
    if (totalextent > menu->bottomcutoff)
    {
        const int32_t scrollx = origin.x + ((320 - tilesiz[SELECTDIR].x)<<16), scrolly = origin.y + menu->ytop;
        const int32_t scrollheight = menu->bottomcutoff - menu->ytop;
        M_BlackRectangle(scrollx, scrolly, tilesiz[SELECTDIR].x<<16, scrollheight);

        rotatesprite_fs(scrollx, scrolly + scale(scrollheight - (tilesiz[SELECTDIR].y<<16), menu->scrollPos, totalextent - menu->bottomcutoff), 65536, 0, SELECTDIR, 0, 0, 26);
    }

    return menu->totalHeight;
}

static void M_RunMenu_MenuOptionList(MenuOption_t *object, const vec2_t origin)
{
    const int32_t cursorShade = 4-(sintable[(totalclock<<4)&2047]>>11);
    int32_t e, y = object->options->list->pos.y;
    int32_t calculatedentryspacing = object->options->list->entryspacing;

    // assumes height == font->yline!
    if (calculatedentryspacing < 0)
        calculatedentryspacing = (-calculatedentryspacing - object->font->yline) / (object->options->numOptions - 1) - object->font->yline;

    for (e = 0; e < object->options->numOptions; ++e)
    {
        uint8_t status = 0;
        int32_t height, x;
        // vec2_t textsize;
        int32_t dodraw = 1;

        x = object->options->list->pos.x;

        status |= (e == object->options->currentEntry)<<0;
        status |= (object->options->list->width == 0)<<2;

        dodraw &= object->options->list->pos.y <= y - object->options->scrollPos && y - object->options->scrollPos + object->font->yline <= object->options->list->bottomcutoff;

        if (dodraw)
        /*textsize =*/ M_MenuText(origin.x + x, origin.y + y - object->options->scrollPos, object->font, object->options->optionNames[e], status);

        height = object->font->yline; // max(textsize.y, object->font->yline);

        status |= (object->options->list->width < 0)<<3 | (1<<4);

        if (dodraw && (status & (1<<0)))
        {
            if (status & (1<<2))
            {
                rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16) + object->options->list->cursorPosition, origin.y + y + (height>>1) - object->options->scrollPos, object->options->list->cursorScale, 0, SPINNINGNUKEICON+6-((6+(totalclock>>3))%7), cursorShade, 0, 10);
                rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16) - object->options->list->cursorPosition, origin.y + y + (height>>1) - object->options->scrollPos, object->options->list->cursorScale, 0, SPINNINGNUKEICON+((totalclock>>3)%7), cursorShade, 0, 10);
            }
            else
                rotatesprite_fs(origin.x + x - object->options->list->cursorPosition, origin.y + y + (height>>1) - object->options->scrollPos, object->options->list->cursorScale, 0, SPINNINGNUKEICON+(((totalclock>>3))%7), cursorShade, 0, 10);
        }

        // prepare for the next line
        y += height;
        y += calculatedentryspacing;
    }

    y -= calculatedentryspacing;

    // draw indicators if applicable
    if (y > object->options->list->bottomcutoff)
    {
        const int32_t scrollx = origin.x + ((320 - tilesiz[SELECTDIR].x)<<16), scrolly = origin.y + object->options->list->pos.y;
        const int32_t scrollheight = object->options->list->bottomcutoff - object->options->list->pos.y;
        M_BlackRectangle(scrollx, scrolly, tilesiz[SELECTDIR].x<<16, scrollheight);

        rotatesprite_fs(scrollx, scrolly + scale(scrollheight - (tilesiz[SELECTDIR].y<<16), object->options->scrollPos, y - object->options->list->bottomcutoff), 65536, 0, SELECTDIR, 0, 0, 26);
    }
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

            M_PreMenuDraw(cm->menuID, NULL, NULL, origin);

            rotatesprite_fs(origin.x + object->cursorpos.x, origin.y + object->cursorpos.y, 65536, 0, SPINNINGNUKEICON + (((totalclock >> 3)) % 7), cursorShade, 0, 10);

            break;
        }

        case Message:
        {
            MenuMessage_t *object = (MenuMessage_t*)cm->object;

            M_PreMenu(cm->menuID);

            M_PreMenuDrawBackground(cm->menuID, origin);

            M_PreMenuDraw(cm->menuID, NULL, NULL, origin);

            rotatesprite_fs(origin.x + object->cursorpos.x, origin.y + object->cursorpos.y, 65536, 0, SPINNINGNUKEICON + (((totalclock >> 3)) % 7), cursorShade, 0, 10);

            break;
        }

        case Password:
        {
            MenuPassword_t *object = (MenuPassword_t*)cm->object;
            vec2_t textreturn;
            size_t x;

            M_PreMenu(cm->menuID);

            M_PreMenuDrawBackground(cm->menuID, origin);

            mgametextcenter(origin.x, origin.y + ((50+16+16+16+16-12)<<16), "Enter Password");

            for (x=0; x < Bstrlen(object->input); ++x)
                tempbuf[x] = '*';
            tempbuf[x] = 0;

            textreturn = mgametextcenter(origin.x, origin.y + ((50+16+16+16+16)<<16), tempbuf);

            M_PreMenuDraw(cm->menuID, NULL, NULL, origin);

            rotatesprite_fs(origin.x + (168<<16) + (textreturn.x>>1), origin.y + ((50+16+16+16+16+4)<<16), 32768, 0, SPINNINGNUKEICON + ((totalclock >> 3) % 7), cursorShade, 0, 2 | 8);

            break;
        }

        case FileSelect:
        {
            MenuFileSelect_t *object = (MenuFileSelect_t*)cm->object;

            int32_t i;

            M_PreMenu(cm->menuID);

            M_PreMenuDrawBackground(cm->menuID, origin);

            if (object->title != NoTitle)
                M_DrawTopBar(origin);


            // black translucent background underneath file lists
            M_BlackRectangle(origin.x + (36<<16), origin.y + (42<<16), 248<<16, 118<<16);

            // path
            mminitext(origin.x + (38<<16), origin.y + (45<<16), object->destination, 16);

            mgametext(origin.x + ((40+4)<<16), origin.y + (32<<16), "Directories");

            mgametext(origin.x + ((180+4)<<16), origin.y + (32<<16), "Files");

            for (i = 0; i < 2; ++i)
            {
                if (object->findhigh[i])
                {
                    CACHE1D_FIND_REC *dir;
                    int32_t y = MenuFileSelect_ytop[i] - object->scrollPos[i];
                    for (dir = object->findhigh[i]->usera; dir; dir = dir->next)
                    {
                        uint8_t status = (dir == object->findhigh[i] && object->currentList == i)<<0;

                        // pal = dir->source==CACHE1D_SOURCE_ZIP ? 8 : 2

                        M_RunMenu_AbbreviateNameIntoBuffer(dir->name, USERMAPENTRYLENGTH);

                        if (MenuFileSelect_ytop[i] <= y && y <= MenuFileSelect_ybottom)
                            M_MenuText(origin.x + MenuFileSelect_startx[i], origin.y + y, object->font[i], tempbuf, status);

                        y += MenuFileSelect_entryheight;
                    }
                }
            }

            M_PreMenuDraw(cm->menuID, NULL, NULL, origin);

            if (object->title != NoTitle)
                M_DrawTopBarCaption(object->title, origin);

            rotatesprite_fs(origin.x + ((object->currentList == 0 ? 45 : 185)<<16) - (21<<15), origin.y + ((32+4+1-2)<<16), 32768, 0, SPINNINGNUKEICON + (((totalclock >> 3)) % 7), cursorShade, 0, 10);

            break;
        }

        case Panel:
        {
            MenuPanel_t *object = (MenuPanel_t*)cm->object;

            M_PreMenu(cm->menuID);

            M_PreMenuDrawBackground(cm->menuID, origin);

            if (object->title != NoTitle)
                M_DrawTopBar(origin);

            M_PreMenuDraw(cm->menuID, NULL, NULL, origin);

            if (object->title != NoTitle)
                M_DrawTopBarCaption(object->title, origin);

            break;
        }

        case Menu:
        {
            int32_t state;

            MenuMenu_t *menu = (MenuMenu_t*)cm->object;
            MenuGroup_t *currgroup = menu->grouplist[menu->currentGroup];
            MenuEntry_t *currentry = currgroup->entrylist[currgroup->currentEntry];

            state = M_DetermineMenuSpecialState(currentry);

            if (state != 2)
            {
                M_PreMenu(cm->menuID);

                M_PreMenuDrawBackground(cm->menuID, origin);

                if (menu->title != NoTitle)
                    M_DrawTopBar(origin);

                M_PreMenuDraw(cm->menuID, currgroup, currentry, origin);

                M_RunMenu_MenuMenu(menu, currentry, state, origin);
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

                    M_PreMenuOptionListDraw(/*currgroup,*/ currentry, origin);

                    M_RunMenu_MenuOptionList((MenuOption_t*)currentry->entry, origin);
                }
                else if (currentry->type == Custom2Col)
                {
                    M_PreMenuCustom2ColScreenDraw(currgroup, currentry, origin);
                }
            }

            if ((currentry->type != Option || state != 2) && menu->title != NoTitle)
                M_DrawTopBarCaption(menu->title, origin);

            break;
        }
    }
}

static void M_RunMenuInput(Menu_t *cm)
{
    switch (cm->type)
    {
        case Panel:
        {
            MenuPanel_t *panel = (MenuPanel_t*)cm->object;

            if (I_ReturnTrigger())
            {
                I_ReturnTriggerClear();

                S_PlaySound(EXITMENUSOUND);

                M_ChangeMenuAnimate(cm->parentID, cm->parentAnimation);
            }
            else if (I_PanelUp())
            {
                I_PanelUpClear();

                S_PlaySound(KICK_HIT);
                M_ChangeMenuAnimate(panel->previousID, panel->previousAnimation);
            }
            else if (I_PanelDown())
            {
                I_PanelDownClear();

                S_PlaySound(KICK_HIT);
                M_ChangeMenuAnimate(panel->nextID, panel->nextAnimation);
            }
        }

        case Password:
        {
            MenuPassword_t *object = (MenuPassword_t*)cm->object;
            int32_t hitstate = I_EnterText(object->input, object->maxlength, 0);

            if (hitstate == 1)
            {
                S_PlaySound(PISTOL_BODYHIT);

                M_MenuPasswordSubmit(object->input);

                object->input = NULL;
            }
            else if (hitstate == -1)
            {
                S_PlaySound(EXITMENUSOUND);

                object->input = NULL;

                M_ChangeMenuAnimate(cm->parentID, cm->parentAnimation);
            }
            break;
        }

        case FileSelect:
        {
            MenuFileSelect_t *object = (MenuFileSelect_t*)cm->object;
            int32_t movement = 0;

            if (I_ReturnTrigger())
            {
                I_ReturnTriggerClear();

                S_PlaySound(EXITMENUSOUND);

                object->destination[0] = 0;

                M_MenuFileSelect(0);

                M_ChangeMenuAnimate(cm->parentID, MA_Return);
            }
            else if (I_AdvanceTrigger())
            {
                I_AdvanceTriggerClear();

                S_PlaySound(PISTOL_BODYHIT);

                if (object->currentList == 0)
                {
                    if (!object->findhigh[0]) break;
                    Bstrcat(object->destination, object->findhigh[0]->name);
                    Bstrcat(object->destination, "/");
                    Bcorrectfilename(object->destination, 1);

                    M_MenuFileSelectInit(object);
                }
                else
                {
                    if (!object->findhigh[1]) break;
                    Bstrcat(object->destination, object->findhigh[1]->name);

                    M_MenuFileSelect(1);
                }
            }
            else if (KB_KeyPressed(sc_Home))
            {
                movement = 1;

                object->findhigh[object->currentList] = object->findhigh[object->currentList]->usera;

                KB_ClearKeyDown(sc_Home);

                S_PlaySound(KICK_HIT);
            }
            else if (KB_KeyPressed(sc_End))
            {
                movement = 1;

                object->findhigh[object->currentList] = object->findhigh[object->currentList]->userb;

                KB_ClearKeyDown(sc_End);

                S_PlaySound(KICK_HIT);
            }
            else if (KB_KeyPressed(sc_PgUp))
            {
                int32_t i;

                CACHE1D_FIND_REC *seeker = object->findhigh[object->currentList];

                for (i = 0; i < 6; ++i)
                {
                    if (seeker && seeker->prev)
                        seeker = seeker->prev;
                }

                if (seeker)
                {
                    movement = 1;

                    object->findhigh[object->currentList] = seeker;

                    KB_ClearKeyDown(sc_PgUp);

                    S_PlaySound(KICK_HIT);
                }
            }
            else if (KB_KeyPressed(sc_PgDn))
            {
                int32_t i;

                CACHE1D_FIND_REC *seeker = object->findhigh[object->currentList];

                for (i = 0; i < 6; ++i)
                {
                    if (seeker && seeker->next)
                        seeker = seeker->next;
                }

                if (seeker)
                {
                    movement = 1;

                    object->findhigh[object->currentList] = seeker;

                    KB_ClearKeyDown(sc_PgDn);

                    S_PlaySound(KICK_HIT);
                }
            }
            else if (I_MenuLeft() || I_MenuRight())
            {
                I_MenuLeftClear();
                I_MenuRightClear();

                if ((object->currentList ? object->fnlist.numdirs : object->fnlist.numfiles) > 0)
                {
                    object->currentList = !object->currentList;

                    S_PlaySound(KICK_HIT);
                }
            }
            else if (I_MenuUp())
            {
                movement = 1;

                I_MenuUpClear();

                S_PlaySound(KICK_HIT);

                if (object->findhigh[object->currentList])
                {
                    if (object->findhigh[object->currentList]->prev)
                        object->findhigh[object->currentList] = object->findhigh[object->currentList]->prev;
                    else
                        object->findhigh[object->currentList] = object->findhigh[object->currentList]->userb;
                }
            }
            else if (I_MenuDown())
            {
                movement = 1;

                I_MenuDownClear();

                S_PlaySound(KICK_HIT);

                if (object->findhigh[object->currentList])
                {
                    if (object->findhigh[object->currentList]->next)
                        object->findhigh[object->currentList] = object->findhigh[object->currentList]->next;
                    else
                        object->findhigh[object->currentList] = object->findhigh[object->currentList]->usera;
                }
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
                        movement = 1;

                        object->findhigh[object->currentList] = seeker;

                        S_PlaySound(KICK_HIT);
                    }
                }
            }

            if (movement)
            {
                int32_t ypos = MenuFileSelect_ytop[object->currentList] + object->findhigh[object->currentList]->type * MenuFileSelect_entryheight;
                if (ypos - object->scrollPos[object->currentList] > MenuFileSelect_ybottom)
                    object->scrollPos[object->currentList] = ypos - MenuFileSelect_ybottom;
                else if (ypos - object->scrollPos[object->currentList] < MenuFileSelect_ytop[object->currentList])
                    object->scrollPos[object->currentList] = ypos - MenuFileSelect_ytop[object->currentList];
            }

            M_PreMenuInput(NULL, NULL);
            break;
        }

        case Message:
            if (I_ReturnTrigger())
            {
                I_ReturnTriggerClear();

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

            M_PreMenuInput(NULL, NULL);
            break;

        case Verify:
            if (I_ReturnTrigger() || KB_KeyPressed(sc_N))
            {
                I_ReturnTriggerClear();
                KB_ClearKeyDown(sc_N);

                M_MenuVerify(0);

                M_ChangeMenuAnimate(cm->parentID, cm->parentAnimation);

                S_PlaySound(EXITMENUSOUND);
            }

            if (I_AdvanceTrigger() || KB_KeyPressed(sc_Y))
            {
                MenuVerify_t *verify = (MenuVerify_t*)cm->object;

                I_AdvanceTriggerClear();
                KB_ClearKeyDown(sc_Y);

                M_MenuVerify(1);

                M_ChangeMenuAnimate(verify->linkID, verify->animation);

                S_PlaySound(PISTOL_BODYHIT);
            }

            M_PreMenuInput(NULL, NULL);
            break;

        case Menu:
        {
            int32_t state, movement = 0;

            MenuMenu_t *menu = (MenuMenu_t*)cm->object;
            MenuGroup_t *currgroup = menu->grouplist[menu->currentGroup];
            MenuEntry_t *currentry = currgroup->entrylist[currgroup->currentEntry];

            state = M_DetermineMenuSpecialState(currentry);

            if (state == 0)
            {
                if (currentry != NULL)
                switch (currentry->type)
                {
                    case Dummy:
                        break;
                    case Link:
                        if (currentry->disabled)
                            break;
                        if (I_AdvanceTrigger())
                        {
                            MenuLink_t *link = (MenuLink_t*)currentry->entry;
                            I_AdvanceTriggerClear();

                            M_MenuEntryLinkActivate(currgroup, currentry);

                            if (g_currentMenu != MENU_SKILL)
                                S_PlaySound(PISTOL_BODYHIT);

                            M_ChangeMenuAnimate(link->linkID, link->animation);
                        }
                        break;
                    case Option:
                    {
                        MenuOption_t *object = (MenuOption_t*)currentry->entry;
                        int32_t modification = -1;

                        if (currentry->disabled)
                            break;

                        if (I_AdvanceTrigger())
                        {
                            I_AdvanceTriggerClear();

                            S_PlaySound(PISTOL_BODYHIT);

                            if (object->options->features & 2)
                            {
                                modification = object->currentOption + 1;
                                if (modification >= object->options->numOptions)
                                    modification = 0;
                            }
                            else
                            {
                                object->options->currentEntry = object->currentOption;
                            }
                        }
                        else if (I_MenuRight())
                        {
                            modification = object->currentOption + 1;
                            if (modification >= object->options->numOptions)
                                modification = 0;

                            I_MenuRightClear();

                            S_PlaySound(PISTOL_BODYHIT);
                        }
                        else if (I_MenuLeft())
                        {
                            modification = object->currentOption - 1;
                            if (modification < 0)
                                modification = object->options->numOptions - 1;

                            I_MenuLeftClear();

                            S_PlaySound(PISTOL_BODYHIT);
                        }

                        if (modification >= 0)
                        {
                            int32_t temp = (object->options->optionValues == NULL) ? modification : object->options->optionValues[modification];
                            if (!M_MenuEntryOptionModify(currgroup, currentry, temp))
                            {
                                object->currentOption = modification;
                                if ((int32_t*)object->data != NULL)
                                    *((int32_t*)object->data) = temp;
                            }
                        }
                    }
                        break;
                    case Custom2Col:
                        if (I_MenuLeft() || I_MenuRight())
                        {
                            currgroup->currentColumn = !currgroup->currentColumn;

                            I_MenuLeftClear();
                            I_MenuRightClear();

                            S_PlaySound(KICK_HIT);
                        }

                        if (currentry->disabled)
                            break;

                        if (I_AdvanceTrigger())
                        {
                            I_AdvanceTriggerClear();

                            S_PlaySound(PISTOL_BODYHIT);

                            M_MenuCustom2ColScreen(currgroup/*, currentry*/);

                            ((MenuCustom2Col_t*)currentry->entry)->screenOpen = 1;
                        }
                        break;
                    case RangeInt32:
                    {
                        MenuRangeInt32_t *object = (MenuRangeInt32_t*)currentry->entry;
                        const float interval = (float) (object->max - object->min) / (float) (object->steps - 1);
                        int32_t step;
                        int32_t modification = 0;

                        if (currentry->disabled)
                            break;

                        step = Blrintf((float) (*object->variable - object->min) / interval);

                        if (I_SliderLeft())
                        {
                            I_SliderLeftClear();

                            modification = -1;
                            S_PlaySound(KICK_HIT);
                        }
                        else if (I_SliderRight())
                        {
                            I_SliderRightClear();

                            modification = 1;
                            S_PlaySound(KICK_HIT);
                        }

                        if (modification != 0)
                        {
                            int32_t temp;

                            step += modification;

                            if (step < 0)
                                step = 0;
                            else if (step >= object->steps)
                                step = object->steps - 1;

                            temp = Blrintf(interval * step + (object->min));

                            if (!M_MenuEntryRangeInt32Modify(currentry, temp))
                                *object->variable = temp;
                        }

                        break;
                    }
                    case RangeFloat:
                    {
                        MenuRangeFloat_t *object = (MenuRangeFloat_t*)currentry->entry;
                        const float interval = (object->max - object->min) / (object->steps - 1);
                        int32_t step;
                        int32_t modification = 0;

                        if (currentry->disabled)
                            break;

                        step = Blrintf((*object->variable - object->min) / interval);

                        if (I_SliderLeft())
                        {
                            I_SliderLeftClear();

                            modification = -1;
                            S_PlaySound(KICK_HIT);
                        }
                        else if (I_SliderRight())
                        {
                            I_SliderRightClear();

                            modification = 1;
                            S_PlaySound(KICK_HIT);
                        }

                        if (modification != 0)
                        {
                            float temp;

                            step += modification;

                            if (step < 0)
                                step = 0;
                            else if (step >= object->steps)
                                step = object->steps - 1;

                            temp = interval * (float)step + object->min;

                            if (!M_MenuEntryRangeFloatModify(currentry, temp))
                                *object->variable = temp;
                        }

                        break;
                    }
                    case RangeDouble:
                    {
                        MenuRangeDouble_t *object = (MenuRangeDouble_t*)currentry->entry;
                        const double interval = (object->max - object->min) / (object->steps - 1);
                        int32_t step;
                        int32_t modification = 0;

                        if (currentry->disabled)
                            break;

                        step = Blrintf((*object->variable - object->min) / interval);

                        if (I_SliderLeft())
                        {
                            I_SliderLeftClear();

                            modification = -1;
                            S_PlaySound(KICK_HIT);
                        }
                        else if (I_SliderRight())
                        {
                            I_SliderRightClear();

                            modification = 1;
                            S_PlaySound(KICK_HIT);
                        }

                        if (modification != 0)
                        {
                            double temp;

                            step += modification;

                            if (step < 0)
                                step = 0;
                            else if (step >= object->steps)
                                step = object->steps - 1;

                            temp = interval * step + object->min;

                            if (!M_MenuEntryRangeDoubleModify(/*currentry, temp*/))
                                *object->variable = temp;
                        }

                        break;
                    }

                    case String:
                    {
                        MenuString_t *object = (MenuString_t*)currentry->entry;

                        if (I_AdvanceTrigger())
                        {
                            I_AdvanceTriggerClear();

                            S_PlaySound(PISTOL_BODYHIT);

                            Bstrncpy(typebuf, object->variable, TYPEBUFSIZE);
                            object->editfield = typebuf;

                            // this limitation is an arbitrary implementation detail
                            if (object->maxlength > TYPEBUFSIZE)
                                object->maxlength = TYPEBUFSIZE;

                            M_MenuEntryStringActivate(currgroup/*, currentry*/);
                        }

                        break;
                    }
                }

                if (I_ReturnTrigger() || I_EscapeTrigger())
                {
                    I_ReturnTriggerClear();
                    I_EscapeTriggerClear();

                    S_PlaySound(EXITMENUSOUND);

                    M_ChangeMenuAnimate(cm->parentID, cm->parentAnimation);
                }
                else if (KB_KeyPressed(sc_Home))
                {
                    movement = 1;

                    KB_ClearKeyDown(sc_Home);

                    S_PlaySound(KICK_HIT);

                    menu->currentGroup = 0;
                    menu->grouplist[menu->currentGroup]->currentEntry = 0;

                    currgroup = menu->grouplist[menu->currentGroup];
                    currentry = currgroup->entrylist[currgroup->currentEntry];

                    M_MenuEntryFocus(currgroup/*, currentry*/);
                }
                else if (KB_KeyPressed(sc_End))
                {
                    movement = 1;

                    KB_ClearKeyDown(sc_End);

                    S_PlaySound(KICK_HIT);

                    menu->currentGroup = menu->numGroups-1;
                    menu->grouplist[menu->currentGroup]->currentEntry = menu->grouplist[menu->currentGroup]->numEntries-1;

                    currgroup = menu->grouplist[menu->currentGroup];
                    currentry = currgroup->entrylist[currgroup->currentEntry];

                    M_MenuEntryFocus(currgroup/*, currentry*/);
                }
                else if (I_MenuUp())
                {
                    movement = 1;

                    I_MenuUpClear();

                    S_PlaySound(KICK_HIT);

                    --currgroup->currentEntry;
                    if (currgroup->currentEntry < 0)
                    {
                        currgroup->currentEntry = 0;
                        --menu->currentGroup;

                        if (menu->currentGroup < 0)
                            menu->currentGroup = menu->numGroups-1;

                        currgroup = menu->grouplist[menu->currentGroup];
                        currgroup->currentEntry = currgroup->numEntries-1;
                    }
                    currentry = currgroup->entrylist[currgroup->currentEntry];

                    M_MenuEntryFocus(currgroup/*, currentry*/);
                }
                else if (I_MenuDown())
                {
                    movement = 1;

                    I_MenuDownClear();

                    S_PlaySound(KICK_HIT);

                    ++currgroup->currentEntry;
                    if (currgroup->currentEntry >= currgroup->numEntries)
                    {
                        currgroup->currentEntry = currgroup->numEntries-1;
                        ++menu->currentGroup;

                        if (menu->currentGroup >= menu->numGroups)
                            menu->currentGroup = 0;

                        currgroup = menu->grouplist[menu->currentGroup];
                        currgroup->currentEntry = 0;
                    }
                    currentry = currgroup->entrylist[currgroup->currentEntry];

                    M_MenuEntryFocus(currgroup/*, currentry*/);
                }

                if (movement)
                {
                    if (currentry->ybottom - menu->scrollPos > menu->bottomcutoff)
                        menu->scrollPos = currentry->ybottom - menu->bottomcutoff;
                    else if (currentry->ytop - menu->scrollPos < menu->ytop)
                        menu->scrollPos = currentry->ytop - menu->ytop;
                }

                if (currentry != NULL && !currentry->disabled)
                    M_PreMenuInput(currgroup, currentry);
            }
            else if (state == 1)
            {
                if (currentry->type == String)
                {
                    MenuString_t *object = (MenuString_t*)currentry->entry;

                    int32_t hitstate = I_EnterText(object->editfield, object->maxlength, object->flags);

                    if (hitstate == 1)
                    {
                        S_PlaySound(PISTOL_BODYHIT);

                        if (!M_MenuEntryStringSubmit(currgroup, currentry, object->editfield))
                            Bstrncpy(object->variable, object->editfield, object->maxlength);

                        object->editfield = NULL;
                    }
                    else if (hitstate == -1)
                    {
                        S_PlaySound(EXITMENUSOUND);

                        M_MenuEntryStringCancel(/*currgroup, currentry*/);

                        object->editfield = NULL;
                    }
                }
            }
            else if (state == 2)
            {
                if (currentry->type == Option)
                {
                    MenuOption_t *object = (MenuOption_t*)currentry->entry;
                    int32_t movement = 0;

                    if (I_ReturnTrigger())
                    {
                        I_ReturnTriggerClear();

                        S_PlaySound(EXITMENUSOUND);

                        object->options->currentEntry = -1;
                    }
                    else if (I_AdvanceTrigger())
                    {
                        int32_t temp = (object->options->optionValues == NULL) ? object->options->currentEntry : object->options->optionValues[object->options->currentEntry];

                        I_AdvanceTriggerClear();

                        S_PlaySound(PISTOL_BODYHIT);

                        if (!M_MenuEntryOptionModify(currgroup, currentry, temp))
                        {
                            object->currentOption = object->options->currentEntry;
                            if ((int32_t*)object->data != NULL)
                                *((int32_t*)object->data) = temp;
                        }

                        object->options->currentEntry = -1;
                    }
                    else if (KB_KeyPressed(sc_Home))
                    {
                        movement = 1;

                        KB_ClearKeyDown(sc_Home);

                        S_PlaySound(KICK_HIT);

                        object->options->currentEntry = 0;
                    }
                    else if (KB_KeyPressed(sc_End))
                    {
                        movement = 1;

                        KB_ClearKeyDown(sc_End);

                        S_PlaySound(KICK_HIT);

                        object->options->currentEntry = object->options->numOptions-1;
                    }
                    else if (I_MenuUp())
                    {
                        movement = 1;

                        I_MenuUpClear();

                        S_PlaySound(KICK_HIT);

                        --object->options->currentEntry;

                        if (object->options->currentEntry < 0)
                            object->options->currentEntry = object->options->numOptions-1;
                    }
                    else if (I_MenuDown())
                    {
                        movement = 1;

                        I_MenuDownClear();

                        S_PlaySound(KICK_HIT);

                        ++object->options->currentEntry;

                        if (object->options->currentEntry >= object->options->numOptions)
                            object->options->currentEntry = 0;
                    }

                    if (movement)
                    {
                        const int32_t listytop = object->options->list->pos.y;
                        // assumes height == font->yline!
                        const int32_t unitheight = object->options->list->entryspacing < 0 ? (-object->options->list->entryspacing - object->font->yline) / object->options->numOptions : (object->font->yline + object->options->list->entryspacing);
                        const int32_t ytop = listytop + object->options->currentEntry * unitheight;
                        const int32_t ybottom = ytop + object->font->yline;

                        if (ybottom - object->options->scrollPos > object->options->list->bottomcutoff)
                            object->options->scrollPos = ybottom - object->options->list->bottomcutoff;
                        else if (ytop - object->options->scrollPos < listytop)
                            object->options->scrollPos = ytop - listytop;
                    }
                }
                else if (currentry->type == Custom2Col)
                {
                    if (I_EscapeTrigger())
                    {
                        I_EscapeTriggerClear();

                        S_PlaySound(EXITMENUSOUND);

                        ((MenuCustom2Col_t*)currentry->entry)->screenOpen = 0;
                    }
                    else if (M_PreMenuCustom2ColScreen(currgroup, currentry))
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
    vec2_t origin = { 0, 0 };

    Net_GetPackets();

    if ((g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
    {
        walock[TILE_LOADSHOT] = 1;
        return;
    }

    VM_OnEvent(EVENT_DISPLAYMENU, g_player[screenpeek].ps->i, screenpeek, -1, 0);

    g_player[myconnectindex].ps->gm &= (0xff-MODE_TYPE);
    g_player[myconnectindex].ps->fta = 0;

    if (((g_player[myconnectindex].ps->gm&MODE_GAME) || ud.recstat==2) && M_BlackTranslucentBackgroundOK(g_currentMenu))
        fade_screen_black(1);

    if (M_UpdateScreenOK(g_currentMenu))
        G_UpdateScreenArea();

    if (!M_IsTextInput(m_currentMenu) && KB_KeyPressed(sc_Q))
    {
        g_previousMenu = g_currentMenu;
        M_ChangeMenuAnimate(MENU_QUIT, MA_Advance);
    }

#if defined(USE_OPENGL) && defined(DROIDMENU)
    gltexfiltermode = 1;
    gltexapplyprops();
#endif

    if (totalclock < m_animation.start + m_animation.length)
    {
        vec2_t previousOrigin = { 0, 0 };
        const int32_t screenwidth = scale(240<<16, xdim, ydim);

        origin.x = scale(screenwidth, m_animation.func(), 32768);
        previousOrigin.x = origin.x - screenwidth;

        M_RunMenu(m_animation.a, previousOrigin);
        M_RunMenu(m_animation.b, origin);
    }
    else
        M_RunMenu(m_currentMenu, origin);

    M_RunMenuInput(m_currentMenu);

#if defined(USE_OPENGL) && defined(DROIDMENU)
    gltexfiltermode = menufiltermode ? 5 : 2;
    gltexapplyprops();
#endif

    if (VM_HaveEvent(EVENT_DISPLAYMENUREST))
        VM_OnEvent(EVENT_DISPLAYMENUREST, g_player[screenpeek].ps->i, screenpeek, -1, 0);

    if ((g_player[myconnectindex].ps->gm&MODE_MENU) != MODE_MENU)
    {
        G_UpdateScreenArea();
        CAMERACLOCK = totalclock;
        CAMERADIST = 65536;
    }
}
