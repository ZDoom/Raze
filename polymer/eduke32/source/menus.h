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

#ifndef menus_h_
#define menus_h_

#include "compat.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined EDUKE32_TOUCH_DEVICES
# define DROIDMENU
#endif

// #define DROIDMENU

enum MenuIndex_t {
    MENU_NULL           = INT32_MIN, // sentinel for "do nothing"
    MENU_CLOSE          = -2, // sentinel for "close the menu"/"no menu"
    MENU_PREVIOUS       = -1, // sentinel for "go to previous menu"
    MENU_MAIN           = 0,
    MENU_MAIN_INGAME    = 50,
    MENU_EPISODE        = 100,
    MENU_USERMAP        = 101,
    MENU_SKILL          = 110,
    MENU_GAMESETUP      = 200,
    MENU_OPTIONS        = 202,
    MENU_VIDEOSETUP     = 203,
    MENU_KEYBOARDSETUP  = 204,
    MENU_MOUSESETUP     = 205,
    MENU_JOYSTICKSETUP  = 206,
    MENU_JOYSTICKBTNS   = 207,
    MENU_JOYSTICKAXES   = 208,
    MENU_KEYBOARDKEYS   = 209,
    MENU_MOUSEBTNS      = 210,
    MENU_MOUSEADVANCED  = 212,
    MENU_JOYSTICKAXIS   = 213,
    MENU_TOUCHSETUP     = 214,
	MENU_CONTROLS       = 220,
    MENU_RENDERERSETUP  = 230,
    MENU_COLCORR        = 231,
    MENU_COLCORR_INGAME = 232,
	MENU_SCREENSETUP    = 233,
	MENU_DISPLAYSETUP   = 234,
    MENU_LOAD           = 300,
    MENU_SAVE           = 350,
    MENU_STORY          = 400,
    MENU_F1HELP         = 401,
    MENU_QUIT           = 500,
    MENU_QUITTOTITLE    = 501,
    MENU_QUIT_INGAME    = 502,
    MENU_NETSETUP       = 600,
    MENU_NETWAITMASTER  = 601,
    MENU_NETWAITVOTES   = 603,
    MENU_SOUND          = 700,
    MENU_SOUND_INGAME   = 701,
	MENU_ADVSOUND       = 702,
    MENU_CHEATS         = 800,
    MENU_CHEATENTRY     = 801,
    MENU_CHEAT_WARP     = 802,
    MENU_CHEAT_SKILL    = 803,
    MENU_CREDITS        = 990,
    MENU_CREDITS2       = 991,
    MENU_CREDITS3       = 992,
    MENU_CREDITS4       = 993,
    MENU_CREDITS5       = 994,
    MENU_LOADVERIFY     = 1000,
    MENU_NEWVERIFY      = 1500,
    MENU_SAVEVERIFY     = 2000,
    MENU_ADULTPASSWORD  = 10001,
    MENU_RESETPLAYER    = 15000,
    MENU_BUYDUKE        = 20000,
    MENU_NETWORK        = 20001,
    MENU_PLAYER         = 20002,
    MENU_MACROS         = 20004,
    MENU_NETHOST        = 20010,
    MENU_NETOPTIONS     = 20011,
    MENU_NETUSERMAP     = 20012,
    MENU_NETJOIN        = 20020,
};



typedef int32_t MenuID_t;


typedef enum MenuAnimationType_t
{ // Note: This enum is for logical categories, not visual types.
    MA_None,
    MA_Return,
    MA_Advance,
} MenuAnimationType_t;



// a subset of screentext parameters, restricted because menus require accessibility
typedef struct MenuFont_t
{
    int32_t tilenum;
    int32_t shade_deselected; // selected entries are mandated to glow
    int32_t pal, pal_disabled;
    int32_t xspace, yline;
    int32_t xbetween, ybetween;
    int32_t textflags;
} MenuFont_t;



typedef enum MenuEntryType_t
{
    Dummy,
    Link,
    Option,
    Custom2Col,
    RangeInt32,
    RangeFloat,
    RangeDouble,
    String,
    Spacer,
} MenuEntryType_t;

typedef struct MenuEntryFormat_t
{
    int32_t marginBottom;
    int32_t indent;
    int32_t width; // 0: center, >0: width of the label column (left-aligned options), <0: -width of everything (right-aligned)

    int32_t cursorPosition, cursorScale;
} MenuEntryFormat_t;


typedef struct MenuMenuFormat_t
{
    vec2_t pos;
    int32_t bottomcutoff; // >0: the bottom edge of the menu before automatic scrolling kicks in, <0: -total height for vertical justification
} MenuMenuFormat_t;

typedef struct MenuLink_t
{
    // traits
    MenuID_t linkID;
    MenuAnimationType_t animation;
} MenuLink_t;
typedef struct MenuOptionSet_t
{
    // traits
    char **optionNames;
    int32_t *optionValues; // If NULL, the identity of currentOption is assumed.

    // pop-up list appearance
    MenuMenuFormat_t *menuFormat;
    MenuEntryFormat_t *entryFormat;
    MenuFont_t *font;

    // traits
    int32_t numOptions;

    // pop-up list state
    int32_t currentEntry;
    int32_t scrollPos;

    // appearance
    uint8_t features; // bit 1 = disable left/right arrows, bit 2 = disable list
} MenuOptionSet_t;
typedef struct MenuOption_t
{
    // appearance
    MenuFont_t *font;

    // traits
    MenuOptionSet_t *options; // so that common sets such as Yes/No, On/Off can be reused

    // effect
    int32_t *data;

    // state
    int32_t currentOption;
} MenuOption_t;
typedef struct MenuCustom2Col_t
{
    // effect
    uint8_t *column[2];
    char **key;

    // appearance
    MenuFont_t *font;

    // effect
    size_t numvalid;

    // appearance
    int32_t columnWidth;

    // state
    int8_t screenOpen;
} MenuCustom2Col_t;

enum MenuRangeFlags_t
{
    DisplayTypeInteger = 1,
    DisplayTypePercent = 2,
    DisplayTypeNormalizedDecimal = 3,
    DisplayTypeMask = (1<<0)|(1<<1),

    EnforceIntervals = 1<<7,
};
typedef struct MenuRangeInt32_t
{
    // effect
    int32_t *variable;

    // appearance
    MenuFont_t *font;

    // traits
    int32_t min;
    int32_t max;
    int32_t onehundredpercent; // 0 implies max
    int32_t steps;

    uint8_t flags;
} MenuRangeInt32_t;
typedef struct MenuRangeFloat_t
{
    // effect
    float *variable;

    // appearance
    MenuFont_t *font;

    // traits
    float min;
    float max;
    float onehundredpercent; // 0 implies 1.0
    int32_t steps;

    uint8_t flags;
} MenuRangeFloat_t;
typedef struct MenuRangeDouble_t
{
    // effect
    double *variable;

    // appearance
    MenuFont_t *font;

    // traits
    double min;
    double max;
    double onehundredpercent; // 0 implies 1.0
    int32_t steps;

    uint8_t flags;
} MenuRangeDouble_t;
typedef struct MenuString_t
{
    // state
    char* editfield;

    // effect
    char* variable;

    // appearance
    MenuFont_t *font;

    // effect
    int32_t maxlength;
    int32_t flags;
} MenuString_t;
typedef struct MenuSpacer_t
{
    int32_t height;
} MenuSpacer_t;

// For internal use only.
enum MenuEntryFlags_t
{
    Disabled = 1,
};

typedef struct MenuEntry_t
{
    // traits
    const char *name;

    // appearance
    MenuFont_t *font;
    MenuEntryFormat_t *format;

    void *entry;
    MenuEntryType_t type;

    // state
    int32_t flags;
    int32_t ytop, ybottom;
} MenuEntry_t;


typedef enum MenuType_t
{
    Menu,
    Panel,
    Verify,
    Message,
    TextForm,
    FileSelect,
} MenuType_t;

typedef struct MenuMenu_t
{
    const char *title;

    MenuMenuFormat_t *format;

    MenuEntry_t **entrylist;
    int32_t numEntries;

    // state
    int32_t currentEntry, currentColumn;
    int32_t totalHeight, scrollPos;
} MenuMenu_t;
typedef struct MenuPanel_t
{
    const char *title;

    MenuID_t previousID;
    MenuAnimationType_t previousAnimation;
    MenuID_t nextID;
    MenuAnimationType_t nextAnimation;
} MenuPanel_t;
typedef struct MenuVerify_t
{
    vec2_t cursorpos;

    MenuID_t linkID;
    MenuAnimationType_t animation;
} MenuVerify_t;
typedef struct MenuMessage_t
{
    vec2_t cursorpos;

    MenuID_t linkID;
    MenuAnimationType_t animation;
} MenuMessage_t;
enum MenuTextFormFlags_t
{
    MTF_Password = 1<<0,
};
typedef struct MenuTextForm_t
{
    // state
    char *input;

    // traits
    const char *instructions;
    int32_t maxlength;
    uint8_t flags;
} MenuTextForm_t;
typedef struct MenuFileSelect_t
{
    const char *title;

    // appearance
    MenuMenuFormat_t *format[2];
    MenuFont_t *font[2];

    // traits
    const char *pattern;
    char *destination;

    // state
    CACHE1D_FIND_REC *findhigh[2];
    int32_t scrollPos[2];

    // appearance
    int32_t marginBottom[2];

    // state
    fnlist_t fnlist;
    int32_t currentList;
} MenuFileSelect_t;

typedef struct Menu_t
{
    void *object;
    MenuID_t menuID;
    MenuID_t parentID;
    MenuAnimationType_t parentAnimation;
    MenuType_t type;
} Menu_t;


extern MenuID_t g_currentMenu;
extern Menu_t *m_currentMenu;

extern int32_t g_lastSaveSlot;
extern int32_t g_quitDeadline;
extern int32_t voting;
int32_t menutext_(int32_t x,int32_t y,int32_t s,int32_t p,char *t,int32_t bits);
void M_ChangeMenu(int32_t cm);
void M_ChangeMenuAnimate(int32_t cm, MenuAnimationType_t animtype);
int32_t M_IsTextInput(Menu_t *cm);
void G_CheckPlayerColor(int32_t *color,int32_t prev_color);
void M_Init(void);
void M_OpenMenu(size_t playerID);
void M_CloseMenu(size_t playerID);
void M_DisplayMenus(void);

#define M_MOUSETIMEOUT 210
extern int32_t m_mouselastactivity;

#if defined EDUKE32_TOUCH_DEVICES
# define MOUSEALPHA 0
# define MOUSEACTIVECONDITIONAL(condition) (condition)
# define MOUSEWATCHPOINTCONDITIONAL(condition) (condition)
#else
extern int32_t m_mousewake_watchpoint, m_menuchange_watchpoint;
// alpha increments of 3 --> 255 / 3 = 85 --> round up to power of 2 --> 128 --> divide by 2 --> 64 alphatabs required
# define MOUSEALPHA ((getrendermode() == REND_CLASSIC && numalphatabs < 64) ? 0 : clamp((totalclock - m_mouselastactivity - 90)*3, 0, 255))
# define MOUSEACTIVECONDITION (totalclock - m_mouselastactivity < M_MOUSETIMEOUT)
# define MOUSEACTIVECONDITIONAL(condition) (MOUSEACTIVECONDITION && (condition))
# define MOUSEINACTIVECONDITIONAL(condition) (!MOUSEACTIVECONDITION && (condition))
# define MOUSEWATCHPOINTCONDITIONAL(condition) ((condition) || m_mousewake_watchpoint || m_menuchange_watchpoint == 3)
#endif

#ifdef __cplusplus
}
#endif

#endif
