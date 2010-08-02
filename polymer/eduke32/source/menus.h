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

extern char inputloc;
extern int16_t g_skillSoundID;
extern int32_t g_demo_recFilePtr;
extern int32_t g_lastSaveSlot;
extern int32_t g_quitDeadline;
extern int32_t probey;
extern int32_t voting;
int32_t G_LoadSaveHeader(char spot,struct savehead *saveh);
int32_t menutext_(int32_t x,int32_t y,int32_t s,int32_t p,char *t,int32_t bits);
void ChangeToMenu(int32_t cm);
void G_CheckPlayerColor(int32_t *color,int32_t prev_color);
void M_DisplayMenus(void);
#endif
