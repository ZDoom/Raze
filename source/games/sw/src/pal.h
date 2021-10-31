//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#pragma once

BEGIN_SW_NS
////
//
// Misc Defines
//
////

enum
{
	LT_GREY         = (16 * 0 + 1),
	DK_GREY         = (16 * 1),
	LT_BROWN        = (16 * 2),
	DK_BROWN        = (16 * 3),
	LT_TAN          = (16 * 4),
	DK_TAN          = (16 * 5),
	RUST_RED        = (16 * 6),
	RED             = (16 * 7),
	YELLOW          = (16 * 8),
	BRIGHT_GREEN    = (16 * 9),
	DK_GREEN        = (16 * 10),
	GREEN           = (16 * 11),
	LT_BLUE         = (16 * 12),
	DK_BLUE         = (16 * 13),
	PURPLE          = (16 * 14),
	FIRE            = (16 * 15),
};
//
// Palette numbers and meanings
//

enum
{
	PALETTE_DEFAULT = 0,
	PALETTE_FOG = 1,
	// blue sword blade test
	PALETTE_MENU_HIGHLIGHT = 2,
	// used for the elector gore pieces
	PALETTE_ELECTRO_GORE = 3,
	// turns ninjas belt and headband red
	PALETTE_BASIC_NINJA = 4,
	// diving in lava
	PALETTE_DIVE_LAVA = 5,
	// turns ninjas belt and headband red
	PALETTE_RED_NINJA = 6,
	// used for the mother ripper - she is bigger/stronger/brown
	PALETTE_BROWN_RIPPER = 7,
	// turns ninjas belt and headband red
	PALETTE_GREEN_NINJA = 8,
	// reserved diving palette this is copied over the default palette
	// when needed - NOTE: could move this to a normal memory buffer if palette
	// slot is needed.
	PALETTE_DIVE = 9,
	PALETTE_SKEL_GORE = 10,
	// turns ALL colors to shades of GREEN/BLUE/RED
	PALETTE_GREEN_LIGHTING = 11,
	PALETTE_BLUE_LIGHTING = 13,
	PALETTE_RED_LIGHTING = 14,

	// for brown bubbling sludge
	PALETTE_SLUDGE = 15,
};



// Player 0 uses default palette - others use these
// turns ninja's vests (when we get them) into different color ranges
enum
{
 PALETTE_PLAYER0           = 16,
 PAL_XLAT_BROWN            = 16,
 PALETTE_PLAYER1           = 17,
 PAL_XLAT_LT_GREY          = 17,
 PALETTE_PLAYER2           = 18,
 PAL_XLAT_PURPLE           = 18,
 PALETTE_PLAYER3           = 19,
 PAL_XLAT_RUST_RED         = 19,
 PALETTE_PLAYER4           = 20,
 PAL_XLAT_YELLOW           = 20,
 PALETTE_PLAYER5           = 21,
 PAL_XLAT_DK_GREEN         = 21,
 PALETTE_PLAYER6           = 22,
 PAL_XLAT_GREEN            = 22,
 PALETTE_PLAYER7           = 23,
 PAL_XLAT_LT_BLUE          = 23,
 PALETTE_PLAYER8           = 24,
 PAL_XLAT_LT_TAN           = 24,
 PALETTE_PLAYER9           = 25,
 PAL_XLAT_RED              = 25,
 PALETTE_PLAYER10          = 26,
 PAL_XLAT_DK_GREY          = 26,
 PALETTE_PLAYER11          = 27,
 PAL_XLAT_BRIGHT_GREEN     = 27,
 PALETTE_PLAYER12          = 28,
 PAL_XLAT_DK_BLUE          = 28,
 PALETTE_PLAYER13          = 29,
 PAL_XLAT_FIRE             = 29,
 PALETTE_PLAYER14          = 30,
 PALETTE_PLAYER15          = 31,
 PALETTE_ILLUMINATE        = 32, // Used to make sprites bright green in night vision
};

END_SW_NS
