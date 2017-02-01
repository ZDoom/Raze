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

#ifndef RESERVE_H
#define RESERVE_H

// This header is used for reserving tile space for programatic purposes
// MAXTILES is currently at 6144 in size - anything under this is ok

#define MAXMIRRORS          8
// This is just some, high, blank tile number not used
// by real graphics to put the MAXMIRRORS mirrors in
#define MIRRORLABEL         6000

#define TILT_TILE           6016

// save screen and tilt tile stuff
#define SAVE_SCREEN_TILE    6017
#define SAVE_SCREEN_XSIZE 160L
#define SAVE_SCREEN_YSIZE 100L
#endif

