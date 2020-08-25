#pragma once

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
// Interpolation code is the same in all games with slightly different naming - this needs to be unified and cleaned up.
// Interpolations are reconstructed on load and do not need to be saved.
#define MAXINTERPOLATIONS MAXSPRITES
extern int numinterpolations;
extern int* curipos[MAXINTERPOLATIONS];
extern int bakipos[MAXINTERPOLATIONS];

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
extern int camerashitable;
extern int max_player_health;
extern int max_armour_amount;
extern int lasermode;

extern int gameclock;
extern int lastTic;
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
extern int duke3d_globalflags;
extern uint8_t ready2send;
extern input_t loc;
extern int gamequit;
extern int playerswhenstarted;
extern int show_shareware;
extern int screenpeek;

// Variables that must be saved
extern uint8_t sectorextra[MAXSECTORS]; // these hold fields that were formerly in sprite and sector. Move these back into the base structs!

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
extern animwalltype animwall[MAXANIMWALLS];
extern int numanimwalls;
extern int animatecnt;
extern int numclouds;
extern int camsprite;
extern int numcyclers;
extern int earthquaketime;
extern int freezerhurtowner;
extern int global_random;
extern int impact_damage;
extern int mirrorcnt;
extern int numplayersprites;
extern int spriteqloc;
extern int thunder_brightness;

enum animtype_t
{
	anim_floorz,
	anim_ceilingz,
	anim_vertexx,
	anim_vertexy,
};
extern int16_t animatesect[MAXANIMATES];
extern int16_t animatetarget[MAXANIMATES];
extern int8_t animatetype[MAXANIMATES];
extern int animategoal[MAXANIMATES];
extern int animatevel[MAXANIMATES];

extern int16_t clouds[256];
extern int16_t cloudx;
extern int16_t cloudy;
extern ClockTicks cloudclock;

extern int16_t spriteq[1024];
extern int16_t cyclers[MAXCYCLERS][6];
extern int16_t mirrorsector[64];
extern int16_t mirrorwall[64];

extern ClockTicks lockclock;

extern int wupass;
extern int chickenplant;
extern int thunderon;
extern int ufospawn;
extern int ufocnt;
extern int hulkspawn;
extern int lastlevel;

extern int geosectorwarp[MAXGEOSECTORS];
extern int geosectorwarp2[MAXGEOSECTORS];
extern int geosector[MAXGEOSECTORS];
extern int geox[MAXGEOSECTORS];
extern int geoy[MAXGEOSECTORS];
extern int geox2[MAXGEOSECTORS];
extern int geoy2[MAXGEOSECTORS];
extern int geocnt;

extern short ambientlotag[64];
extern short ambienthitag[64];
extern unsigned ambientfx;
extern int msx[MAXANIMPOINTS], msy[MAXANIMPOINTS];
extern int WindTime, WindDir;
extern short fakebubba_spawn, mamaspawn_count, banjosound;
extern short BellTime, BellSprite /* word_119BE0*/;
extern uint8_t enemysizecheat /*raat607*/, ufospawnsminion, pistonsound, chickenphase /* raat605*/, RRRA_ExitedLevel, fogactive;
extern uint32_t everyothertime;
extern player_orig po[MAXPLAYERS];

END_DUKE_NS

#include "inlines.h"

