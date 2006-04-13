/*
 * mathutil.c
 * Mathematical utility functions to emulate MACT
 *
 * by Jonathon Fowler
 *
 * Since we weren't given the source for MACT386.LIB so I've had to do some
 * creative interpolation here.
 *
 */
//-------------------------------------------------------------------------
/*
Duke Nukem Copyright (C) 1996, 2003 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "types.h"

#if 0
// Ken's reverse-engineering job
int32 FindDistance2D(int32 x, int32 y)
{
	int32 i;
	x = labs(x);
	y = labs(y);
	if (!x) return(y);
	if (!y) return(x);
	if (y < x) { i = x; x = y; y = i; } //swap x, y
	x += (x>>1);
	return ((x>>6)+(x>>2)+y-(y>>5)-(y>>7)); //handle 1 octant
}

// My abomination
#define square(x) ((x)*(x))
/*
int32 FindDistance2D(int32 dx, int32 dy)
{
//	return (int32)floor(sqrt((double)(sqr(dx)+sqr(dy))));
	return ksqrt(square(dx)+square(dy));
}
*/
int32 FindDistance3D(int32 dx, int32 dy, int32 dz)
{
//	return (int32)floor(sqrt((double)(sqr(dx)+sqr(dy)+sqr(dz))));
	return ksqrt(square(dx)+square(dy)+square(dz));
}

#else

#include <stdlib.h>

// This extracted from the Rise of the Triad source RT_UTIL.C :-|

#define SWAP(a,b) \
   {              \
   a=(a)^(b);     \
   b=(a)^(b);     \
   a=(a)^(b);     \
   }

int32 FindDistance2D(int32 x, int32 y)
{
  int32 t;

  x= abs(x);        /* absolute values */
  y= abs(y);

  if (x<y)
     SWAP(x,y);

  t = y + (y>>1);

  return (x - (x>>5) - (x>>7)  + (t>>2) + (t>>6));
}


int32 FindDistance3D(int32 x, int32 y, int32 z)
   {
   int32 t;

   x= abs(x);           /* absolute values */
   y= abs(y);
   z= abs(z);

   if (x<y)
     SWAP(x,y);

   if (x<z)
     SWAP(x,z);

   t = y + z;

   return (x - (x>>4) + (t>>2) + (t>>3));
   }
#endif
