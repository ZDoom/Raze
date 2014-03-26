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

#include "savegame.h"

#define MENU_MARGIN_REGULAR 40
#define MENU_MARGIN_CENTER  160

enum MenuIndex_t {
    MENU_MAIN           = 0,
    MENU_MAIN_INGAME    = 50,
    MENU_EPISODE        = 100,
    MENU_USERMAP        = 101,
    MENU_SELECTMAP      = 102,
    MENU_SKILL          = 110,
    MENU_SETUP          = 200,
    MENU_GAMESETUP      = 201,
    MENU_OPTIONS        = 202,
    MENU_VIDEOSETUP     = 203,
    MENU_KEYBOARDSETUP  = 204,
    MENU_MOUSESETUP     = 205,
    MENU_JOYSTICKSETUP  = 206,
    MENU_JOYSTICKBTNS   = 207,
    MENU_JOYSTICKAXES   = 208,
    MENU_JOYSTICKAXES2  = 209,
    MENU_KEYBOARDASSIGN = 210,
    MENU_MOUSEASSIGN    = 211,
    MENU_MOUSEADVANCED  = 212,
    MENU_JOYSTICKDEAD   = 213,
    MENU_JOYSTICKDEAD2  = 214,
    MENU_JOYSTICKDEAD3  = 215,
    MENU_JOYSTICKDEAD4  = 216,
    MENU_JOYSTICKAXES3  = 217,
    MENU_JOYSTICKAXES4  = 218,
    MENU_JOYSTICKAXES5  = 219,
    MENU_JOYSTICKAXES6  = 220,
    MENU_JOYSTICKAXES7  = 221,
    MENU_JOYSTICKAXES8  = 222,
    MENU_RENDERERSETUP  = 230,
    MENU_COLCORR        = 231,
    MENU_COLCORR_INGAME = 232,
    MENU_LOAD           = 300,
    MENU_SAVE           = 350,
    MENU_SAVE_          = 351,
    MENU_SAVETYPING     = 360,
    MENU_SAVETYPING2    = 361,
    MENU_SAVETYPING3    = 362,
    MENU_SAVETYPING4    = 363,
    MENU_SAVETYPING5    = 364,
    MENU_SAVETYPING6    = 365,
    MENU_SAVETYPING7    = 366,
    MENU_SAVETYPING8    = 367,
    MENU_SAVETYPING9    = 368,
    MENU_SAVETYPING10   = 369,
    MENU_STORY          = 400,
    MENU_F1HELP         = 401,
    MENU_ORDERING       = 402,
    MENU_ORDERING2      = 403,
    MENU_QUIT           = 500,
    MENU_QUITTOTITLE    = 501,
    MENU_QUIT2          = 502,
    MENU_NETSETUP       = 600,
    MENU_NETWAITMASTER  = 601,
    MENU_PRENETSETUP    = 602,
    MENU_NETWAITVOTES   = 603,
    MENU_SOUND          = 700,
    MENU_SOUND_INGAME   = 701,
    MENU_CREDITS        = 990,
    MENU_CREDITS2       = 991,
    MENU_CREDITS3       = 992,
    MENU_CREDITS4       = 993,
    MENU_CREDITS5       = 994,
    MENU_CREDITS6       = 995,
    MENU_CREDITS7       = 996,
    MENU_CREDITS8       = 997,
    MENU_CREDITS9       = 998,
    MENU_CREDITS10      = 999,
    MENU_LOADVERIFY     = 1000,
    MENU_LOADVERIFY2    = 1001,
    MENU_LOADVERIFY3    = 1002,
    MENU_LOADVERIFY4    = 1003,
    MENU_LOADVERIFY5    = 1004,
    MENU_LOADVERIFY6    = 1005,
    MENU_LOADVERIFY7    = 1006,
    MENU_LOADVERIFY8    = 1007,
    MENU_LOADVERIFY9    = 1008,
    MENU_LOADVERIFY10   = 1009,
    MENU_NEWVERIFY      = 1500,
    MENU_SAVEVERIFY     = 2000,
    MENU_SAVEVERIFY2    = 2001,
    MENU_SAVEVERIFY3    = 2002,
    MENU_SAVEVERIFY4    = 2003,
    MENU_SAVEVERIFY5    = 2004,
    MENU_SAVEVERIFY6    = 2005,
    MENU_SAVEVERIFY7    = 2006,
    MENU_SAVEVERIFY8    = 2007,
    MENU_SAVEVERIFY9    = 2008,
    MENU_SAVEVERIFY10   = 2009,
    MENU_ADULTMODE      = 10000,
    MENU_ADULTPASSWORD  = 10001,
    MENU_RESETPLAYER    = 15000,
    MENU_RESETPLAYER2   = 15001,
    MENU_BUYDUKE        = 20000,
    MENU_NETWORK        = 20001,
    MENU_PLAYER         = 20002,
    MENU_PLAYERNAME     = 20003,
    MENU_MACROS         = 20004,
    MENU_MACROSTYPING   = 20005,
    MENU_NETHOST        = 20010,
    MENU_NETOPTIONS     = 20011,
    MENU_NETJOIN        = 20020,
    MENU_NETJOINSERVER  = 20021,
    MENU_NETJOINPORT    = 20022,

};
extern char inputloc;
extern int16_t g_skillSoundID;
extern int32_t g_lastSaveSlot;
extern int32_t g_quitDeadline;
extern int32_t probey;
extern int32_t voting;
int32_t menutext_(int32_t x,int32_t y,int32_t s,int32_t p,char *t,int32_t bits);
void M_ChangeMenu(int32_t cm);
int32_t M_IsTextInput(int32_t cm);
void G_CheckPlayerColor(int32_t *color,int32_t prev_color);
void M_DisplayMenus(void);

#endif
