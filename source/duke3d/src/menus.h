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

BEGIN_DUKE_NS

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


#if 0

enum MenuIndex_t {
    MENU_NULL           = INT32_MIN, // sentinel for "do nothing"
    MENU_CLOSE          = -2, // sentinel for "close the menu"/"no menu"
    MENU_PREVIOUS       = -1, // sentinel for "go to previous menu"
    MENU_MAIN           = 0,	// done
    MENU_MAIN_INGAME    = 50,	// done
    MENU_EPISODE        = 100,	// done
    MENU_USERMAP        = 101,
    MENU_NEWGAMECUSTOM  = 102,	// done
    MENU_NEWGAMECUSTOMSUB = 103,// done
    MENU_SKILL          = 110,	// done
    MENU_OPTIONS        = 202,
		MENU_GAMESETUP      = 200, 
		    MENU_CHEATS         = 800,		// IF script hacked
				MENU_CHEATENTRY     = 801,	// IF script hacked
				MENU_CHEAT_WARP = 802,
				MENU_CHEAT_SKILL = 803,
	    MENU_DISPLAYSETUP   = 234,
			MENU_SCREENSETUP = 233,		// HUD
			MENU_COLCORR = 231,			// color correction
			MENU_COLCORR_INGAME = 232,	// almost the same for ingame - not needed
			MENU_VIDEOSETUP = 203,
			MENU_POLYMOST = 230,
			MENU_POLYMER = 240,			// Who needs a renderer that's folding performance-wise with a single light?
		MENU_SOUND = 700,
		MENU_SOUND_INGAME = 701,		// Just the same with different exit logic.
			MENU_ADVSOUND = 702,		// Only needed for space reasons. Fold into main sound menu.
		MENU_PLAYER = 20002,
			MENU_MACROS = 20004,
		MENU_CONTROLS = 220,
			MENU_KEYBOARDSETUP = 204,
				MENU_KEYBOARDKEYS = 209,
			MENU_MOUSESETUP = 205,
				MENU_MOUSEBTNS = 210,	// folded with keyboard
				MENU_MOUSEADVANCED = 212,
			MENU_JOYSTICKSETUP = 206,
				MENU_JOYSTICKBTNS = 207,
				MENU_JOYSTICKAXES = 208,
				MENU_JOYSTICKAXIS = 213,
    MENU_LOAD           = 300,
    MENU_SAVE           = 350,
    MENU_STORY          = 400,
    MENU_F1HELP         = 401,
    MENU_CREDITS        = 990,
		MENU_CREDITS2       = 991,
		MENU_CREDITS3       = 992,
		MENU_CREDITS4       = 993,
		MENU_CREDITS5       = 994,
    MENU_QUIT           = 500,
    MENU_QUITTOTITLE    = 501,
    MENU_QUIT_INGAME    = 502,

    MENU_SAVESETUP      = 750,

    MENU_SAVECLEANVERIFY = 751,
    MENU_LOADVERIFY     = 1000,
    MENU_LOADDELVERIFY  = 1100,
    MENU_NEWVERIFY      = 1500,
    MENU_SAVEVERIFY     = 2000,
    MENU_SAVEDELVERIFY  = 2100,
    MENU_COLCORRRESETVERIFY = 2200,
    MENU_KEYSRESETVERIFY = 2201,
    MENU_KEYSCLASSICVERIFY = 2202,
    MENU_JOYSTANDARDVERIFY = 2203,
    MENU_JOYPROVERIFY   = 2204,
    MENU_JOYCLEARVERIFY = 2205,
    MENU_ADULTPASSWORD  = 10001,
    MENU_RESETPLAYER    = 15000,
    MENU_BUYDUKE        = 20000,

	MENU_NETSETUP       = 600,
    MENU_NETWAITMASTER  = 601,
    MENU_NETWAITVOTES   = 603,
    MENU_NETWORK        = 20001,
    MENU_NETHOST        = 20010,
    MENU_NETOPTIONS     = 20011,
    MENU_NETUSERMAP     = 20012,
    MENU_NETJOIN        = 20020,
};


#endif

END_DUKE_NS

#endif
