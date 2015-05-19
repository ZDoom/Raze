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

////
//
// Misc Defines
//
////

#define LT_GREY         (16 * 0 + 1)
#define DK_GREY         (16 * 1)
#define LT_BROWN        (16 * 2)
#define DK_BROWN        (16 * 3)
#define LT_TAN          (16 * 4)
#define DK_TAN          (16 * 5)
#define RUST_RED        (16 * 6)
#define RED             (16 * 7)
#define YELLOW          (16 * 8)
#define BRIGHT_GREEN    (16 * 9)
#define DK_GREEN        (16 * 10)
#define GREEN           (16 * 11)
#define LT_BLUE         (16 * 12)
#define DK_BLUE         (16 * 13)
#define PURPLE          (16 * 14)
#define FIRE            (16 * 15)

//
// Palette numbers and meanings
//

#define PALETTE_DEFAULT             0
#define PALETTE_FOG                 1
// blue sword blade test
#define PALETTE_MENU_HIGHLIGHT      2
// used for the elector gore pieces
#define PALETTE_ELECTRO_GORE        3
// turns ninjas belt and headband red
#define PALETTE_BASIC_NINJA         4
// diving in lava
#define PALETTE_DIVE_LAVA           5
// turns ninjas belt and headband red
#define PALETTE_RED_NINJA           6
// used for the mother ripper - she is bigger/stronger/brown
#define PALETTE_BROWN_RIPPER        7
// turns ninjas belt and headband red
#define PALETTE_GREEN_NINJA         8
// reserved diving palette this is copied over the default palette
// when needed - NOTE: could move this to a normal memory buffer if palette
// slot is needed.
#define PALETTE_DIVE                9
#define PALETTE_SKEL_GORE           10
// turns ALL colors to shades of GREEN/BLUE/RED
#define PALETTE_GREEN_LIGHTING      11
#define PALETTE_BLUE_LIGHTING       13
#define PALETTE_RED_LIGHTING        14

// for brown bubbling sludge
#define PALETTE_SLUDGE              15




// Player 0 uses default palette - others use these
// turns ninja's vests (when we get them) into different color ranges
#define PALETTE_PLAYER0             16
#define PAL_XLAT_BROWN              16
#define PALETTE_PLAYER1             17
#define PAL_XLAT_LT_GREY            17
#define PALETTE_PLAYER2             18
#define PAL_XLAT_PURPLE             18
#define PALETTE_PLAYER3             19
#define PAL_XLAT_RUST_RED           19
#define PALETTE_PLAYER4             20
#define PAL_XLAT_YELLOW             20
#define PALETTE_PLAYER5             21
#define PAL_XLAT_DK_GREEN           21
#define PALETTE_PLAYER6             22
#define PAL_XLAT_GREEN              22
#define PALETTE_PLAYER7             23
#define PAL_XLAT_LT_BLUE            23
#define PALETTE_PLAYER8             24
#define PAL_XLAT_LT_TAN             24
#define PALETTE_PLAYER9             25
#define PAL_XLAT_RED                25
#define PALETTE_PLAYER10            26
#define PAL_XLAT_DK_GREY            26
#define PALETTE_PLAYER11            27
#define PAL_XLAT_BRIGHT_GREEN       27
#define PALETTE_PLAYER12            28
#define PAL_XLAT_DK_BLUE            28
#define PALETTE_PLAYER13            29
#define PAL_XLAT_FIRE               29
#define PALETTE_PLAYER14            30
#define PALETTE_PLAYER15            31

#define PALETTE_ILLUMINATE          32  // Used to make sprites bright green in night vision


