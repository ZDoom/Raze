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

#ifndef TYPES_H

#define TYPES_H

#include "compat.h"

#define OFF     0
#define ON     (!OFF)

typedef unsigned char SWBOOL;

/*
===========================
=
= FAST calculations
=
===========================
*/

// For fast DIVision of integers

#define DIV2(x)  ((x) >> 1)
#define DIV4(x)  ((x) >> 2)
#define DIV8(x)  ((x) >> 3)
#define DIV16(x) ((x) >> 4)
#define DIV32(x) ((x) >> 5)
#define DIV64(x) ((x) >> 6)
#define DIV128(x) ((x) >> 7)
#define DIV256(x) ((x) >> 8)

// Constants used in fast mods

#define C_MOD2   1
#define C_MOD4   3
#define C_MOD8   7
#define C_MOD16  15
#define C_MOD32  31
#define C_MOD64  63
#define C_MOD128 127
#define C_MOD256 255

// Fast mods of select 2 power numbers

#define MOD2(x) ((x) & C_MOD2)
#define MOD4(x) ((x) & C_MOD4)
#define MOD8(x) ((x) & C_MOD8)
#define MOD16(x) ((x) & C_MOD16)
#define MOD32(x) ((x) & C_MOD32)
#define MOD64(x) ((x) & C_MOD64)
#define MOD128(x) ((x) & C_MOD128)
#define MOD256(x) ((x) & C_MOD256)

// Fast mods of any power of 2

#define MOD_P2(number,modby)  ((number) & ((modby)-1))

// Truncates to select 2 power numbers

#define TRUNC2(x) ((x) & ~C_MOD2)
#define TRUNC4(x) ((x) & ~C_MOD4)
#define TRUNC8(x) ((x) & ~C_MOD8)
#define TRUNC16(x) ((x) & ~C_MOD16)
#define TRUNC32(x) ((x) & ~C_MOD32)
#define TRUNC64(x) ((x) & ~C_MOD64)
#define TRUNC128(x) ((x) & ~C_MOD128)
#define TRUNC256(x) ((x) & ~C_MOD256)

#define POWER2_TRUNC(number,truncby) ((number) & ~((truncby)-1))

// moves value to closest power of 2 pixel boundry

#define BOUND_2PIX(x) ( TRUNC2((x) + MOD2(x)) )
#define BOUND_4PIX(x) ( TRUNC4((x) + MOD4(x)) )
#define BOUND_8PIX(x) ( TRUNC8((x) + MOD8(x)) )
#define BOUND_16PIX(x) ( TRUNC16((x) + MOD16(x)) )
#define BOUND_32PIX(x) ( TRUNC32((x) + MOD32(x)) )
#define BOUND_64PIX(x) ( TRUNC64((x) + MOD64(x)) )
#define BOUND_128PIX(x) ( TRUNC128((x) + MOD128(x)) )
#define BOUND_256PIX(x) ( TRUNC256((x) + MOD256(x)) )

#define BOUND_POWER2_PIX(x,bound) ( POWER2_TRUNC((x,bound) + POWER2_MOD(x,bound)) )

// A few muls with shifts and adds
// probably not needed with good compiler
#define MUL2(x) ((x)*2)
#define MUL3(x) (((x)<<1) + (x))
#define MUL5(x) (((x)<<2) + (x))
#define MUL6(x) (((x)<<2) + (x) + (x))
#define MUL7(x) (((x)<<2) + (x) + (x) + (x))
#define MUL8(x) ((x)*8)
#define MUL9(x) (((x)<<3) + (x))
#define MUL10(x) (((x)<<3) + (x) + (x))
#define MUL11(x) (((x)<<3) + (x) + (x) + (x))
#define MUL12(x) (((x)<<3) + ((x)<<2))
#define MUL13(x) (((x)<<3) + ((x)<<2) + (x))
#define MUL14(x) (((x)<<3) + ((x)<<2) + (x) + (x))
#define MUL15(x) (((x)<<3) + ((x)<<2) + (x) + (x) + (x))
#define MUL16(x) ((x)*16)


/*
===========================
=
= Bit manipulation
=
===========================
*/

#define TEST(flags,mask) ((flags) & (mask))
#define SET(flags,mask) ((flags) |= (mask))
#define RESET(flags,mask) ((flags) &= ~(mask))
#define FLIP(flags,mask) ((flags) ^= (mask))

// mask definitions

#define BIT(shift)     (1<<(shift))

/*
===========================
=
= Miscellaneous
=
===========================
*/

//#define ABS(num) ((num) < 0 ? -(num) : (num))

#define BETWEEN(x,low,high) (((x) >= (low)) && ((x) <= (high)))

#endif

