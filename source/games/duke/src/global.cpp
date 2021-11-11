//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT
Copyright (C) 2020 - Christoph Oelckers

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

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//------------------------------------------------------------------------- 

#include "ns.h"

#include "global.h"
#include "duke3d.h"

BEGIN_DUKE_NS

//------------------------------------------------------------------------- 
//
// variables that need a script export
//
//------------------------------------------------------------------------- 

user_defs ud;		// partially serialized

// not serialized - read only
DukeGameInfo gs;
int screenpeek;

// serialized
player_struct ps[MAXPLAYERS];

//------------------------------------------------------------------------- 
//
// variables that only need an export if the entire game logic gets scriptified.
// Otherwise all code referencing these variables should remain native.
//
//------------------------------------------------------------------------- 

int lastvisinc;								// weapon flash
DDukeActor* camsprite;						// active camera
int earthquaketime;
int global_random;							// readonly - one single global per-frame random value. Ugh...

// Redneck Rampage
int chickenplant;							// readonly - used to trigger some special behavior if a special item is found in a map.
int thunderon;								// readonly - enables thunder effect in RR if true.
int ufospawn;								// UFO spawn flag
int ufocnt;									// UFO spawn count
int hulkspawn;								// Spawn a hulk?
int lastlevel;								// Set at the end of RRRA's E2L7.
short fakebubba_spawn, mamaspawn_count, banjosound; // RRRA special effects
short BellTime;
DDukeActor* BellSprite /* word_119BE0*/;
int WindTime, WindDir;
uint8_t enemysizecheat /*raat607*/, ufospawnsminion, pistonsound, chickenphase /* raat605*/, RRRA_ExitedLevel, fogactive;

//------------------------------------------------------------------------- 
//
// variables that do not need a script export.
//
//------------------------------------------------------------------------- 

// not serialized
int otherp;									// internal helper
int actor_tog;								// cheat helper
int playerswhenstarted;						// why is this needed?
int show_shareware;							// display only.
int rtsplaying;								// RTS playback state
int tempwallptr;							// msx/y index.
int msx[MAXANIMPOINTS], msy[MAXANIMPOINTS];
bool sound445done;							// used in checksectors_r. This was local state inside a function, but this must be maintained globally and serialized

// serialized
uint8_t sectorextra[MAXSECTORS];			// something about keys, all access through the haskey function.
uint8_t shadedsector[MAXSECTORS];			// display hackiness

DDukeActor hittype[MAXSPRITES + 1];			// +1 to have a blank entry for serialization, all access in game code through the iterators.
int spriteqamount = 64;						// internal sprite queue
int spriteqloc;
DDukeActor* spriteq[1024];
animwalltype animwall[MAXANIMWALLS];		// animated walls
int numanimwalls;
int animatecnt;								// sector plane movement
int animatesect[MAXANIMATES];
int8_t animatetype[MAXANIMATES];
int animatetarget[MAXANIMATES];
int animategoal[MAXANIMATES];
int animatevel[MAXANIMATES];
int numclouds;								// cloudy skies
int clouds[256];
float cloudx;
float cloudy;
int cloudclock;
int numcyclers;								// sector lighting effects
Cycler cyclers[MAXCYCLERS];
int mirrorcnt;
int mirrorsector[64];					// mirrors
int mirrorwall[64];
int numplayersprites;						// player management for some SEs.
player_orig po[MAXPLAYERS];
unsigned ambientfx;							// used by soundtag and soundtagonce script commands. If exported, export the commands, not the data!
short ambientlotag[64];
short ambienthitag[64];
uint32_t everyothertime;					// Global animation ticker helper.

// Redneck Rampage
int thunder_brightness;
int wupass;									// used to play the level entry sound only once.
int geosectorwarp[MAXGEOSECTORS];			// geometry render hack (overlay a secondary scene)
int geosectorwarp2[MAXGEOSECTORS];
int geosector[MAXGEOSECTORS];
int geox[MAXGEOSECTORS];
int geoy[MAXGEOSECTORS];
int geox2[MAXGEOSECTORS];
int geoy2[MAXGEOSECTORS];
int geocnt;


END_DUKE_NS
