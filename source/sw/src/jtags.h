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

#ifndef JTAGS_H
#define JTAGS_H

//////////////////////////////////////////////////////////////////////////////////////////
//
// SPRITE TAGS (TAG THE SPRITES'S HITAG)  ST1 tags
//
//////////////////////////////////////////////////////////////////////////////////////////
#define AI_NORMAL  0
#define AI_EVASIVE 1
#define AI_SNIPER  2
#define AI_GUNGHO  3

#define SWITCH_LOCKED 29

// My sprite sprite tags start at 1000 to be separate from Frank's

//* Magic mirror cameras
//* LOTAG is the unique camera number
#define MIRROR_CAM       1000
//* These are spots at which a pissed off mirror will spawn a coolie ghost
//* Make sure to set the skill levels on these sprites too.
#define MIRROR_SPAWNSPOT 1001

//* Ambient Sounds
//* LOTAG is the enumerated sound num to play
#define AMBIENT_SOUND      1002
#define TAG_NORESPAWN_FLAG 1003
#define TAG_GET_STAR       1004
#define TAG_ECHO_SOUND     1005
#define TAG_DRIPGEN        1006
#define TAG_BUBBLEGEN      1007
#define TAG_SWARMSPOT      1008

#define TAG_PACHINKOLIGHT 9997
#define TAG_INVISONINJA 9998
#define LUMINOUS 9999

//////////////////////////////////////////////////////////////////////////////////////////
//
// WALL TAGS (TAG THE WALL'S LOTAG)
//
//////////////////////////////////////////////////////////////////////////////////////////

//* Turns a regular mirror into a magic mirror that shows a room containing ST1 sprite at
//* sprite's angle and z height.
//* HITAG is unique camera sprite number matching the ST1 camera sprite
#define TAG_WALL_MAGIC_MIRROR   306


//////////////////////////////////////////////////////////////////////////////////////////
//
// LIGHTING TAGS (TAG THE SECTOR'S LOTAG)
//
//////////////////////////////////////////////////////////////////////////////////////////

//* Fade effect.  Fades in and out smoothly.
//* Ceiling is minimum darkness.
//* Floor is maximum darkness.
//* High byte is speed of flicker.
//* The lower the number the faster.  Default is 3.  I recommend 8.
//* Use TAG_LIGHT_FADE_DIFFUSE tags around the initial torch sector just like light fade.
//* A good value to use for torches, is a 2 in high tag of TAG_LIGHT_FADE_DIFFUSE

#define TAG_LIGHT_TORCH_FADE    305




#endif
