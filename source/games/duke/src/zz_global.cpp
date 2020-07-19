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

char    g_gametypeNames[MAXGAMETYPES][33]
= { "DukeMatch (Spawn)", "Cooperative Play", "DukeMatch (No Spawn)", "Team DM (Spawn)", "Team DM (No Spawn)" };


float g_gameUpdateAvgTime = -1.f;

int32_t respawnactortime   = 768;
int32_t bouncemineblastradius = 2500;
int32_t respawnitemtime    = 768;

int32_t g_morterRadius       = 2500;
int32_t numfreezebounces   = 3;
int32_t g_gametypeCnt       = 5;
int32_t g_volumeCnt         = 3;
int32_t g_pipebombRadius     = 2500;
int32_t dukefriction     = 0xCFD0;
int32_t rpgblastradius          = 1780;
int32_t g_scriptSize         = 1048576;
int32_t g_seenineRadius      = 2048;
int32_t g_shrinkerRadius     = 650;
int32_t g_spriteGravity      = 176;
int32_t g_timerTicsPerSecond = TICRATE;
int32_t g_tripbombRadius     = 3880;
int32_t g_cdTrack = -1;

uint16_t frags[MAXPLAYERS][MAXPLAYERS];
input_t sync[MAXPLAYERS];

int16_t weaponsandammosprites[15];

TileInfo tileinfo[MAXTILES]; // This is not from EDuke32.


int levelTextTime; // must be serialized
int rtsplaying;		// must be serialized
int otherp;			// MP only
bool sound445done; // this was local state inside a function, but this must be maintained globally and serialized

int16_t max_ammo_amount[MAX_WEAPONS];
int32_t spriteqamount = 64;

uint8_t shadedsector[MAXSECTORS];

int32_t cameradist = 0, cameraclock = 0;

int32_t g_Shareware = 0;

int32_t tempwallptr;
int32_t      actor_tog;

weaponhit hittype[MAXSPRITES];
ActorInfo actorinfo[MAXTILES];
player_struct ps[MAXPLAYERS];

int32_t PHEIGHT = PHEIGHT_DUKE;

int32_t lastvisinc;

END_DUKE_NS
