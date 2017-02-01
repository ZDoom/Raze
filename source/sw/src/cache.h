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

#define CACHE_NONE          0       // don't use this - this is for ken
#define CACHE_LOCK_MAX      255
#define CACHE_LOCK_START    200
#define CACHE_UNLOCK_START  1
#define CACHE_UNLOCK_MAX    199

#define CACHE_SOUND_PRECACHE 0
#define CACHE_SOUND_PLAY     1

void SetupPreCache(void);
void PreCacheRange(short start_pic, short end_pic);
void DoTheCache(void);
void precache(void);

