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
    MENU_KEYBOARDASSIGN = 210,
    MENU_LOAD           = 300,
    MENU_STORY          = 400,
    MENU_F1HELP         = 401,
    MENU_QUIT           = 500,
    MENU_QUITTOTITLE    = 501,
    MENU_QUIT2          = 502,
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
    MENU_ADULTMODE      = 10000,
    MENU_ADULTPASSWORD  = 10001,
    MENU_BUYDUKE        = 20000,

};
extern char inputloc;
extern int16_t g_skillSoundID;
extern int32_t g_lastSaveSlot;
extern int32_t g_quitDeadline;
extern int32_t probey;
extern int32_t voting;
int32_t menutext_(int32_t x,int32_t y,int32_t s,int32_t p,char *t,int32_t bits);
void M_ChangeMenu(int32_t cm);
void G_CheckPlayerColor(int32_t *color,int32_t prev_color);
void M_DisplayMenus(void);

#endif
