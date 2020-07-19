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

#ifndef global_h_
#define global_h_

#include "build.h"
#include "compat.h"
#include "duke3d.h"
#include "mmulti.h"
#include "quotemgr.h"
#include "sounds.h"
#include "constants.h"
#include "types.h"

BEGIN_DUKE_NS

extern user_defs ud;

// Variables that do not need to be saved.
extern int respawnactortime;
extern int bouncemineblastradius;
extern int respawnitemtime;
extern int morterblastradius;
extern int numfreezebounces;
extern int pipebombblastradius;
extern int dukefriction;
extern int rpgblastradius;
extern int seenineblastradius;
extern int shrinkerblastradius;
extern int gc;
extern int tripbombblastradius;

extern int cameraclock;
extern int cameradist;
extern int otherp; // transient helper, MP only
extern TileInfo tileinfo[MAXTILES]; // static state
extern ActorInfo actorinfo[MAXTILES]; // static state
extern int actor_tog; // cheat state
extern intptr_t apScriptGameEvent[];
extern TArray<int> ScriptCode;
extern input_t sync[MAXPLAYERS];
extern int16_t max_ammo_amount[MAX_WEAPONS];
extern int16_t weaponsandammosprites[15];
extern int32_t PHEIGHT;

// Interpolation code is the same in all games with slightly different naming - this needs to be unified and cleaned up.
// Interpolations are reconstructed on load and do not need to be saved.
#define MAXINTERPOLATIONS MAXSPRITES
extern int numinterpolations;
extern int* curipos[MAXINTERPOLATIONS];
extern int bakipos[MAXINTERPOLATIONS];


// Variables that must be saved

extern int rtsplaying;
extern int tempwallptr;
extern weaponhit hittype[MAXSPRITES];
extern bool sound445done;
extern int levelTextTime;
extern uint16_t frags[MAXPLAYERS][MAXPLAYERS];
extern player_struct ps[MAXPLAYERS];
extern int spriteqamount;
extern uint8_t shadedsector[MAXSECTORS];
extern int lastvisinc;



// todo








#ifdef global_c_
    #define G_EXTERN
#else
    #define G_EXTERN extern
#endif

G_EXTERN int duke3d_globalflags;

G_EXTERN animwalltype animwall[MAXANIMWALLS];
G_EXTERN bool synchronized_input;

G_EXTERN char ready2send;
G_EXTERN char tempbuf[MAXSECTORS<<1],buf[1024];

G_EXTERN input_t loc;

G_EXTERN int avgfvel, avgsvel, avgbits;
G_EXTERN fix16_t avgavel, avghorz;
G_EXTERN int8_t avgextbits;

G_EXTERN int movefifosendplc;

G_EXTERN int predictfifoplc;

G_EXTERN int g_networkBroadcastMode;

G_EXTERN int numanimwalls;
G_EXTERN int animatecnt;
G_EXTERN int numclouds;
G_EXTERN int camsprite;
G_EXTERN int numcyclers;
G_EXTERN int camerashitable;
G_EXTERN int earthquaketime;
G_EXTERN int freezerhurtowner;
G_EXTERN int gamequit;
G_EXTERN int global_random;
G_EXTERN int impact_damage;
G_EXTERN int mirrorcnt;
G_EXTERN int playerswhenstarted;
G_EXTERN int numplayersprites;
G_EXTERN int show_shareware;
G_EXTERN int spriteqloc;
G_EXTERN int max_player_health;
G_EXTERN int max_armour_amount;
G_EXTERN int lasermode;
G_EXTERN int screenpeek;

G_EXTERN int16_t animatesect[MAXANIMATES];
G_EXTERN int * animateptr[MAXANIMATES];
G_EXTERN int animategoal[MAXANIMATES];
G_EXTERN int animatevel[MAXANIMATES];

G_EXTERN int16_t clouds[256];
G_EXTERN int16_t cloudx;
G_EXTERN int16_t cloudy;
G_EXTERN ClockTicks cloudtotalclock;

G_EXTERN int16_t spriteq[1024];
G_EXTERN int16_t cyclers[MAXCYCLERS][6];
G_EXTERN int16_t mirrorsector[64];
G_EXTERN int16_t mirrorwall[64];
G_EXTERN ClockTicks lockclock;
G_EXTERN ClockTicks ototalclock;

G_EXTERN int wupass;
G_EXTERN int chickenplant;
G_EXTERN int thunderon;
G_EXTERN int ufospawn;
G_EXTERN int ufocnt;
G_EXTERN int hulkspawn;
G_EXTERN int lastlevel;

G_EXTERN int geosectorwarp[MAXGEOSECTORS];
G_EXTERN int geosectorwarp2[MAXGEOSECTORS];
G_EXTERN int geosector[MAXGEOSECTORS];
G_EXTERN int geox[MAXGEOSECTORS];
G_EXTERN int geoy[MAXGEOSECTORS];
G_EXTERN int geox2[MAXGEOSECTORS];
G_EXTERN int geoy2[MAXGEOSECTORS];
G_EXTERN uint32_t geocnt;

G_EXTERN int g_thunderFlash;
G_EXTERN int g_thunderTime;
G_EXTERN int g_winderFlash;
G_EXTERN int g_winderTime;
G_EXTERN int g_brightness;

G_EXTERN int16_t ambientlotag[64];
G_EXTERN int16_t ambienthitag[64];
G_EXTERN uint32_t ambientfx;

G_EXTERN int msx[MAXANIMPOINTS], msy[MAXANIMPOINTS];

G_EXTERN int WindTime, WindDir;
G_EXTERN int16_t fakebubba_spawn, mamaspawn_count, banjosound, BellTime, BellSprite /* word_119BE0*/;
G_EXTERN uint8_t g_spriteExtra[MAXSPRITES], g_sectorExtra[MAXSECTORS]; // move these back into the base structs!
G_EXTERN uint8_t enemysizecheat /*raat607*/, ufospawnsminion, pistonsound, chickenphase /* raat605*/, RRRA_ExitedLevel, fogactive;

G_EXTERN player_orig po[MAXPLAYERS];

G_EXTERN uint32_t everyothertime;


END_DUKE_NS

#include "inlines.h"

#endif
