#pragma once

#include "build.h"
#include "compat.h"
#include "duke3d.h"
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
	int respawnactortime;
	int bouncemineblastradius;
	int respawnitemtime;
	int morterblastradius;
	int numfreezebounces;
	int pipebombblastradius;
	int playerfriction;
	int rpgblastradius;
	int seenineblastradius;
	int shrinkerblastradius;
	int gravity;
	int tripbombblastradius;
	int camerashitable;
	int max_player_health;
	int max_armour_amount;
	int lasermode;
	int freezerhurtowner;
	int impact_damage;

	TileInfo tileinfo[MAXTILES]; // This is not from EDuke32.
	ActorInfo actorinfo[MAXTILES];
	int16_t max_ammo_amount[MAX_WEAPONS];
	int16_t weaponsandammosprites[15];
	int playerheight;
	int displayflags;
};

extern DukeGameInfo gs;

extern int otherp; // transient helper, MP only
extern int actor_tog; // cheat state
extern intptr_t apScriptGameEvent[];
extern TArray<int> ScriptCode;
extern int playerswhenstarted;
extern int show_shareware;
extern int screenpeek;

// Variables that must be saved
extern uint8_t sectorextra[MAXSECTORS]; // these hold fields that were formerly in sprite and sector. Move these back into the base structs!
extern uint8_t shadedsector[MAXSECTORS];

extern int rtsplaying;
extern int tempwallptr;

extern bool sound445done;
extern player_struct ps[MAXPLAYERS];
extern int spriteqamount;
extern int lastvisinc;
extern animwalltype animwall[MAXANIMWALLS];
extern int numanimwalls;
extern int animatecnt;
extern int numclouds;
extern DDukeActor* camsprite;
extern int numcyclers;
extern int earthquaketime;
extern int global_random;
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
extern sectortype* animatesect[MAXANIMATES];
extern int animatetarget[MAXANIMATES];
extern int8_t animatetype[MAXANIMATES];
extern int animategoal[MAXANIMATES];
extern int animatevel[MAXANIMATES];

extern sectortype* clouds[256];
extern float cloudx;
extern float cloudy;
extern int cloudclock;

extern DDukeActor *spriteq[1024];
extern Cycler cyclers[MAXCYCLERS];
extern sectortype* mirrorsector[64];
extern walltype* mirrorwall[64];

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
extern int msx[MAXANIMPOINTS], msy[MAXANIMPOINTS]; // todo: unlimit
extern TArray<CraneDef> cranes;
extern int WindTime, WindDir;
extern short fakebubba_spawn, mamaspawn_count, banjosound;
extern short BellTime;
extern DDukeActor* BellSprite /* word_119BE0*/;
extern uint8_t enemysizecheat /*raat607*/, ufospawnsminion, pistonsound, chickenphase /* raat605*/, RRRA_ExitedLevel, fogactive;
extern uint32_t everyothertime;
extern player_orig po[MAXPLAYERS];

END_DUKE_NS

#include "inlines.h"

