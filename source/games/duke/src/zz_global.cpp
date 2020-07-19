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
int camerashitable;
int max_player_health;
int max_armour_amount;
int lasermode;

int cameradist = 0, cameraclock = 0;
int otherp;	
TileInfo tileinfo[MAXTILES]; // This is not from EDuke32.
ActorInfo actorinfo[MAXTILES];
int actor_tog;
input_t sync[MAXPLAYERS];
int16_t max_ammo_amount[MAX_WEAPONS];
int16_t weaponsandammosprites[15];
int PHEIGHT = PHEIGHT_DUKE;
int duke3d_globalflags;
input_t loc;
bool synchronized_input;
uint8_t ready2send;
int gamequit;
int playerswhenstarted;
int show_shareware;
int screenpeek;
ClockTicks ototalclock;



// Variables that must be saved
uint8_t spriteextra[MAXSPRITES], sectorextra[MAXSECTORS]; // move these back into the base structs!

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
animwalltype animwall[MAXANIMWALLS];
int numanimwalls;
int animatecnt;
int numclouds;
int camsprite;
int numcyclers;
int earthquaketime;
int freezerhurtowner;
int global_random;
int impact_damage;
int mirrorcnt;
int numplayersprites;
int spriteqloc;

int16_t animatesect[MAXANIMATES];
int* animateptr[MAXANIMATES];
int animategoal[MAXANIMATES];
int animatevel[MAXANIMATES];

int16_t clouds[256];
int16_t cloudx;
int16_t cloudy;
ClockTicks cloudtotalclock;

int16_t spriteq[1024];
int16_t cyclers[MAXCYCLERS][6];
int16_t mirrorsector[64];
int16_t mirrorwall[64];

ClockTicks lockclock;

// Redneck Rampage
int wupass;
int chickenplant;
int thunderon;
int ufospawn;
int ufocnt;
int hulkspawn;
int lastlevel;

int geosectorwarp[MAXGEOSECTORS];
int geosectorwarp2[MAXGEOSECTORS];
int geosector[MAXGEOSECTORS];
int geox[MAXGEOSECTORS];
int geoy[MAXGEOSECTORS];
int geox2[MAXGEOSECTORS];
int geoy2[MAXGEOSECTORS];
int geocnt;

short ambientlotag[64];
short ambienthitag[64];
unsigned ambientfx;
int msx[MAXANIMPOINTS], msy[MAXANIMPOINTS];
int WindTime, WindDir;
short fakebubba_spawn, mamaspawn_count, banjosound;
short BellTime, BellSprite /* word_119BE0*/;
uint8_t enemysizecheat /*raat607*/, ufospawnsminion, pistonsound, chickenphase /* raat605*/, RRRA_ExitedLevel, fogactive;
uint32_t everyothertime;
player_orig po[MAXPLAYERS];

END_DUKE_NS
