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
#define ON     1


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
#define DIV256(x) ((x) >> 8)

// Fast mods of any power of 2

#define MOD_P2(number,modby)  ((number) & ((modby)-1))



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

constexpr int BIT(int shift)
{
	return 1 << shift;
}

#endif

