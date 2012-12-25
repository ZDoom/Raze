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

void GAME_drawosdchar(int32_t x, int32_t y, char ch, int32_t shade, int32_t pal);
void GAME_drawosdstr(int32_t x, int32_t y, const char *ch, int32_t len, int32_t shade, int32_t pal);
void GAME_drawosdcursor(int32_t x, int32_t y, int32_t type, int32_t lastkeypress);
int32_t GAME_getcolumnwidth(int32_t w);
int32_t GAME_getrowheight(int32_t w);
void GAME_onshowosd(int32_t shown);

extern int32_t osdhightile;

#define OSDCHAR_WIDTH 8

