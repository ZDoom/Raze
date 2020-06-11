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

#ifndef menus_h_
#define menus_h_

#include "compat.h"

BEGIN_EDUKE_NS

// a subset of screentext parameters, restricted because menus require accessibility
struct MenuFont_t
{
	//    int32_t xspace, yline;
	vec2_t emptychar, between;
	int32_t zoom;
	int32_t cursorLeftPosition, cursorCenterPosition, cursorScale;
	int32_t textflags;
	int16_t tilenum;
	// selected shade glows, deselected shade is used by Blood, disabled shade is used by SW
	int8_t shade_deselected, shade_disabled;
	uint8_t pal;
	uint8_t pal_selected, pal_deselected, pal_disabled;
	uint8_t pal_selected_right, pal_deselected_right, pal_disabled_right;

	int32_t get_yline() const { return mulscale16(emptychar.y, zoom); }
};

extern MenuFont_t MF_Redfont, MF_Bluefont, MF_Minifont;

void Menu_Init(void);

inline int G_CheckPlayerColor(int color)
{
	static int32_t player_pals[] = { 0, 9, 10, 11, 12, 13, 14, 15, 16, 21, 23, };
	if (color >= 0 && color < 10) return player_pals[color];
	return 0;
}


END_EDUKE_NS

#endif
