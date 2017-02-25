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

#pragma once

extern int32_t althud_flashing;
extern int32_t althud_numberpal;
extern int32_t althud_numbertile;
extern int32_t althud_shadows;

static FORCE_INLINE int32_t sbarsc(int32_t sc)
{
    return scale(sc, ud.statusbarscale, 100);
}

int32_t sbarx16(int32_t x);
int32_t sbary16(int32_t y);
void G_DrawInventory(const DukePlayer_t *p);
void G_DrawStatusBar(int32_t snum);
