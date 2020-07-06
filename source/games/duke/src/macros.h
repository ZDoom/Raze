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

void RANDOMSCRAP(spritetype* s, int i);

#define TRAVERSE_SPRITE_SECT(l, o, n)    (o) = (l); ((o) != -1) && ((n) = nextspritesect[o]); (o) = (n)
#define TRAVERSE_SPRITE_STAT(l, o, n)    (o) = (l); ((o) != -1) && ((n) = nextspritestat[o]); (o) = (n)
#define TRAVERSE_CONNECT(i)              i = 0; i != -1; i = connectpoint2[i]

#define TEST(flags,mask) ((flags) & (mask))
#define SET(flags,mask) ((flags) |= (mask))
#define RESET(flags,mask) ((flags) &= ~(mask))
#define FLIP(flags,mask) ((flags) ^= (mask))

// mask definitions

#define BIT(shift)     (1u<<(shift))

inline bool AFLAMABLE(int X)
{
    return (X == TILE_BOX || X == TILE_TREE1 || X == TILE_TREE2 || X == TILE_TIRE || X == TILE_CONE);
}

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

#define SP(i)  sprite[i].yvel
#define SX(i)  sprite[i].x
#define SY(i)  sprite[i].y
#define SZ(i)  sprite[i].z
#define SS(i)  sprite[i].shade
#define PN(i)  sprite[i].picnum
#define SA(i)  sprite[i].ang
//#define SV  sprite[i].xvel
//#define ZV  sprite[i].zvel
//#define RX  sprite[i].xrepeat
//#define RY  sprite[i].yrepeat
#define OW(i)  sprite[i].owner
#define CS(i)  sprite[i].cstat
#define SH(i)  sprite[i].extra
//#define CX  sprite[i].xoffset
//#define CY  sprite[i].yoffset
//#define CD  sprite[i].clipdist
//#define PL  sprite[i].pal
#define SLT(i)  sprite[i].lotag
#define SHT(i)  sprite[i].hitag
#define SECT(i) sprite[i].sectnum

#define T1(i)  actor[i].t_data[0]
#define T2(i)  actor[i].t_data[1]
#define T3(i)  actor[i].t_data[2]
#define T4(i)  actor[i].t_data[3]
#define T5(i)  actor[i].t_data[4]
#define T6(i)  actor[i].t_data[5]

END_DUKE_NS

#endif
