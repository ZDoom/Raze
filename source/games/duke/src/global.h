#pragma once

#include "build.h"
#include "compat.h"
#include "duke3d.h"
#include "mmulti.h"
#include "quotemgr.h"
#include "sounds.h"
#include "constants.h"
#include "types.h"
#include "d_net.h"

BEGIN_DUKE_NS

extern user_defs ud;

struct DukeGameInfo
{
	// Static constant global state
	int respawnactortime		= 768;
	int bouncemineblastradius	= 2500;
	int respawnitemtime			= 768;
	int morterblastradius		= 2500;
	int numfreezebounces		= 3;
	int pipebombblastradius		= 2500;
	int playerfriction			= 0xCFD0;
	int rpgblastradius			= 1780;
	int seenineblastradius		= 2048;
	int shrinkerblastradius		= 650;
	int gravity						= 176;
	int tripbombblastradius		= 3880;
	int camerashitable			= 0;
	int max_player_health		= 0;
	int max_armour_amount		= 0;
	int lasermode				= 0;
	TileInfo tileinfo[MAXTILES] = {}; // This is not from EDuke32.
	ActorInfo actorinfo[MAXTILES] = {};
	int16_t max_ammo_amount[MAX_WEAPONS] = {};
	int16_t weaponsandammosprites[15] = {};
	int playerheight					= PHEIGHT_DUKE;
	int displayflags		= DUKE3D_NO_WIDESCREEN_PINNING;
};

extern DukeGameInfo gs;

extern int cameraclock;
extern int cameradist;
extern int otherp; // transient helper, MP only
extern int actor_tog; // cheat state
extern intptr_t apScriptGameEvent[];
extern TArray<int> ScriptCode;
extern int playerswhenstarted;
extern int show_shareware;
extern int screenpeek;

// Variables that must be saved
extern uint8_t sectorextra[MAXSECTORS]; // these hold fields that were formerly in sprite and sector. Move these back into the base structs!

extern int rtsplaying;
extern int tempwallptr;

extern bool sound445done;
extern uint16_t frags[MAXPLAYERS][MAXPLAYERS];
extern player_struct ps[MAXPLAYERS];
extern int spriteqamount;
extern uint8_t shadedsector[MAXSECTORS];
extern int lastvisinc;
extern animwalltype animwall[MAXANIMWALLS];
extern int numanimwalls;
extern int animatecnt;
extern int numclouds;
extern DDukeActor* camsprite;
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
extern int cloudclock;

extern DDukeActor *spriteq[1024];
extern int16_t cyclers[MAXCYCLERS][6];
extern int16_t mirrorsector[64];
extern int16_t mirrorwall[64];

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
extern short BellTime;
extern DDukeActor* BellSprite /* word_119BE0*/;
extern uint8_t enemysizecheat /*raat607*/, ufospawnsminion, pistonsound, chickenphase /* raat605*/, RRRA_ExitedLevel, fogactive;
extern uint32_t everyothertime;
extern player_orig po[MAXPLAYERS];

END_DUKE_NS

#include "inlines.h"

