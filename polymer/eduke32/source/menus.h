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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#ifndef __menus_h__
#define __menus_h__

#include "compat.h"

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
    MENU_RENDERERSETUP  = 230,
    MENU_COLCORR        = 231,
    MENU_COLCORR_INGAME = 232,
    MENU_LOAD           = 300,
    MENU_SAVE           = 350,
    MENU_STORY          = 400,
    MENU_F1HELP         = 401,
    MENU_ORDERING       = 402,
    MENU_ORDERING2      = 403,
    MENU_ORDERING3      = 404,
    MENU_ORDERING4      = 405,
    MENU_QUIT           = 500,
    MENU_QUITTOTITLE    = 501,
    MENU_QUIT_INGAME    = 502,
    MENU_NETSETUP       = 600,
    MENU_NETWAITMASTER  = 601,
    MENU_NETWAITVOTES   = 603,
    MENU_SOUND          = 700,
    MENU_SOUND_INGAME   = 701,
    MENU_CREDITS        = 990,
    MENU_CREDITS2       = 991,
    MENU_CREDITS3       = 992,
    MENU_CREDITS4       = 993,
    MENU_CREDITS5       = 994,
    MENU_LOADVERIFY     = 1000,
    MENU_NEWVERIFY      = 1500,
    MENU_SAVEVERIFY     = 2000,
    MENU_ADULTMODE      = 10000,
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



// a subset of screentext parameters, restricted because menus require accessibility
typedef struct MenuTextType_t
{
    int32_t tilenum;
    int32_t shade_deselected; // selected entries are mandated to glow
    int32_t pal, pal_disabled;
    int32_t xspace, yline;
    int32_t xbetween, ybetween;
    int32_t textflags;
} MenuTextType_t;



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
} MenuEntryType_t;



typedef struct MenuPos_t
{
    vec2_t pos;

    int32_t groupspacing;
    int32_t entryspacing;
    int32_t width; // 0: center, >0: width of the label column (left-aligned options), <0: -width of everything (right-aligned)
    int32_t bottomcutoff;

    int32_t cursorPosition, cursorScale;
} MenuPos_t;

typedef struct MenuLink_t
{
    // traits
    MenuID_t linkID;
} MenuLink_t;
typedef struct MenuOptionSet_t
{
    uint8_t features; // bit 1 = disable left/right arrows, bit 2 = disable list

    // traits
    char **optionNames;
    int32_t *optionValues; // If NULL, the identity of currentOption is assumed.
    int32_t numOptions;

    // specifically for a pop-up list
    MenuPos_t *list;
    int32_t currentEntry;
    int32_t scrollPos;
} MenuOptionSet_t;
typedef struct MenuOption_t
{
    // appearance
    MenuTextType_t *font;

    // traits
    MenuOptionSet_t *options; // so that common sets such as Yes/No, On/Off can be reused

    // effect
    int32_t *data;

    // state
    int32_t currentOption;
} MenuOption_t;
typedef struct MenuCustom2Col_t
{
    // appearance
    MenuTextType_t *font;
    int32_t columnWidth;

    char **key;
    size_t numvalid;

    uint8_t *column[2];

    // state
    int8_t screenOpen;
} MenuCustom2Col_t;
typedef struct MenuRangeInt32_t
{
    // appearance
    MenuTextType_t *font;
    uint8_t displaytype; // 0 = none, 1 = integer, 2 = percent, 3 = normalized decimal

    // traits
    int32_t min;
    int32_t max;
    int32_t steps;
    int32_t onehundredpercent; // 0 implies max

    // effect
    int32_t* variable;
} MenuRangeInt32_t;
typedef struct MenuRangeFloat_t
{
    // appearance
    MenuTextType_t *font;
    uint8_t displaytype; // 0 = none, 1 = decimal, 2 = percent, 3 = normalized decimal

    // traits
    float min;
    float max;
    int32_t steps;
    float onehundredpercent; // 0 implies 1.0

    // effect
    float* variable;
} MenuRangeFloat_t;
typedef struct MenuRangeDouble_t
{
    // appearance
    MenuTextType_t *font;
    uint8_t displaytype; // 0 = none, 1 = decimal, 2 = percent

    // traits
    double min;
    double max;
    int32_t steps;
    double onehundredpercent; // 0 implies 1.0

    // effect
    double* variable;
} MenuRangeDouble_t;
typedef struct MenuString_t
{
    // appearance
    MenuTextType_t *font;

    // effect
    char* variable;
    int32_t maxlength;

    int32_t flags;

    // state
    char* editfield;
} MenuString_t;


typedef struct MenuEntry_t
{
    // appearance
    MenuTextType_t *font;

    // traits
    const char *name;

    MenuEntryType_t type;
    void *entry;

    // state
    int32_t disabled;

    int32_t ytop, ybottom;
} MenuEntry_t;


typedef struct MenuGroup_t
{
    MenuEntry_t **entrylist;
    int32_t numEntries;

    MenuPos_t *position;

    int32_t currentEntry;
    int32_t currentColumn; // a little hackish but not too bad
} MenuGroup_t;


typedef enum MenuType_t
{
    Menu,
    Panel,
    Verify,
    Message,
    Password,
    FileSelect,
} MenuType_t;

typedef struct MenuMenu_t
{
    MenuGroup_t **grouplist;
    int32_t numGroups;

    const char *title;

    // state
    int32_t currentGroup;
    int32_t ytop, bottomcutoff, totalHeight, scrollPos;
} MenuMenu_t;
typedef struct MenuPanel_t
{
    const char *title;

    MenuID_t previousID;
    MenuID_t nextID;
} MenuPanel_t;
typedef struct MenuVerify_t
{
    vec2_t cursorpos;

    MenuID_t linkID;
} MenuVerify_t;
typedef struct MenuMessage_t
{
    vec2_t cursorpos;

    MenuID_t linkID;
} MenuMessage_t;
typedef struct MenuPassword_t
{
    // traits
    int32_t maxlength;

    // state
    char *input;
} MenuPassword_t;
typedef struct MenuFileSelect_t
{
    const char *title;

    // appearance
    MenuTextType_t *dirfont;
    MenuTextType_t *filefont;

    // traits
    const char *pattern;
    char *destination;

    // state
    fnlist_t fnlist;
    CACHE1D_FIND_REC *finddirshigh, *findfileshigh, *dir;
    int32_t currentEntry, currentlist;
} MenuFileSelect_t;

typedef struct Menu_t
{
    MenuID_t menuID;
    MenuID_t parentID;
    MenuType_t type;
    void *object;
} Menu_t;

extern MenuID_t g_currentMenu;
extern Menu_t *m_currentMenu;

extern int32_t g_lastSaveSlot;
extern int32_t g_quitDeadline;
extern int32_t voting;
int32_t menutext_(int32_t x,int32_t y,int32_t s,int32_t p,char *t,int32_t bits);
void M_ChangeMenu(int32_t cm);
int32_t M_IsTextInput(Menu_t *cm);
void G_CheckPlayerColor(int32_t *color,int32_t prev_color);
void M_Init(void);
void M_DisplayMenus(void);

#endif
