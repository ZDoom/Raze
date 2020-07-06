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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#ifndef EDUKE32_MACROS_H_
#define EDUKE32_MACROS_H_

#include "mmulti.h"

BEGIN_DUKE_NS

// Macros, some from SW source

static FORCE_INLINE int32_t krand2(void)
{
    randomseed = (randomseed * 27584621ul) + 1ul;
    return ((uint32_t) randomseed)>>16;
}

#define BGSTRETCH (hud_bgstretch ? 1024 : 0)

#define TRAVERSE_SPRITE_SECT(l, o, n)    (o) = (l); ((o) != -1) && ((n) = nextspritesect[o]); (o) = (n)
#define TRAVERSE_SPRITE_STAT(l, o, n)    (o) = (l); ((o) != -1) && ((n) = nextspritestat[o]); (o) = (n)
#define TRAVERSE_CONNECT(i)              i = 0; i != -1; i = connectpoint2[i]

#define TEST(flags,mask) ((flags) & (mask))
#define SET(flags,mask) ((flags) |= (mask))
#define RESET(flags,mask) ((flags) &= ~(mask))
#define FLIP(flags,mask) ((flags) ^= (mask))

// mask definitions

#define BIT(shift)     (1u<<(shift))

#define rnd(X) ((krand2()>>8)>=(255-(X)))

//
// NETWORK - REDEFINABLE SHARED (SYNC) KEYS BIT POSITIONS
//

//cstat, bit 0: 1 = Blocking sprite (use with clipmove, getzrange)    "B"
//       bit 1: 1 = 50/50 transluscence, 0 = normal                   "T"
//       bit 2: 1 = x-flipped, 0 = normal                             "F"
//       bit 3: 1 = y-flipped, 0 = normal                             "F"
//       bits 5-4: 00 = FACE sprite (default)                         "R"
//                 01 = WALL sprite (like masked walls)
//                 10 = FLOOR sprite (parallel to ceilings&floors)
//                 11 = SPIN sprite (face sprite that can spin 2draw style - not done yet)
//       bit 6: 1 = 1-sided sprite, 0 = normal                        "1"
//       bit 7: 1 = Real centered centering, 0 = foot center          "C"
//       bit 8: 1 = Blocking sprite (use with hitscan)                "H"
//       bit 9: reserved
//       bit 10: reserved
//       bit 11: 1 = determine shade based only on its own shade member (see CON's spritenoshade command), i.e.
//                   don't take over shade from parallaxed ceiling/nonparallaxed floor
//                   (NOTE: implemented on the game side)
//       bit 12: reserved
//       bit 13: reserved
//       bit 14: reserved
//       bit 15: 1 = Invisible sprite, 0 = not invisible
#define CSTAT_SPRITE_NOSHADE        BIT(11)
#define CSTAT_SPRITE_BREAKABLE (CSTAT_SPRITE_BLOCK_HITSCAN)


END_DUKE_NS

#endif
