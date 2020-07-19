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
#include "ns.h"	// Must come before everything else!

#define global_c_
#include "global.h"
#include "duke3d.h"

BEGIN_DUKE_NS

user_defs ud;

// Variables that do not need to be saved.
int respawnactortime		= 768;
int bouncemineblastradius	= 2500;
int respawnitemtime			= 768;
int morterblastradius		= 2500;
int numfreezebounces		= 3;
int pipebombblastradius		= 2500;
int dukefriction			= 0xCFD0;
int rpgblastradius			= 1780;
int seenineblastradius		= 2048;
int shrinkerblastradius		= 650;
int gc						= 176;
int tripbombblastradius		= 3880;

int cameradist = 0, cameraclock = 0;
int otherp;	
TileInfo tileinfo[MAXTILES]; // This is not from EDuke32.
ActorInfo actorinfo[MAXTILES];
int actor_tog;
input_t sync[MAXPLAYERS];
int16_t max_ammo_amount[MAX_WEAPONS];
int16_t weaponsandammosprites[15];
int PHEIGHT = PHEIGHT_DUKE;



// Variables that must be saved
int rtsplaying;
int tempwallptr;
weaponhit hittype[MAXSPRITES];
bool sound445done; // this was local state inside a function, but this must be maintained globally and serialized
int levelTextTime; // must be serialized
uint16_t frags[MAXPLAYERS][MAXPLAYERS];
player_struct ps[MAXPLAYERS];
int spriteqamount = 64;
uint8_t shadedsector[MAXSECTORS];
int lastvisinc;









END_DUKE_NS
