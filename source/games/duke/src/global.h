#pragma once

#include "build.h"
#include "duke3d.h"
#include "quotemgr.h"
#include "sounds.h"
#include "constants.h"
#include "types.h"
#include "d_net.h"
#include "serialize_obj.h"

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

inline TObjPtr<DDukeActor*> camsprite;
inline TObjPtr<DDukeActor*> BellSprite;
inline TObjPtr<DDukeActor*> spriteq[1024];
inline TObjPtr<DDukeActor*> currentCommentarySprite;


extern int otherp; // transient helper, MP only
extern int actor_tog; // cheat state
extern intptr_t apScriptGameEvent[];
extern TArray<int> ScriptCode;
extern int playerswhenstarted;
extern int show_shareware;
extern int screenpeek;

// Variables that must be saved
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
extern int numcyclers;
extern int earthquaketime;
extern int global_random;
extern int mirrorcnt;
extern int numplayersprites;
extern int spriteqloc;
extern int thunder_brightness;
inline DukeLevel dlevel;

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

extern sectortype* geosectorwarp[MAXGEOSECTORS];
extern sectortype* geosectorwarp2[MAXGEOSECTORS];
extern sectortype* geosector[MAXGEOSECTORS];
extern double geox[MAXGEOSECTORS];
extern double geoy[MAXGEOSECTORS];
extern double geox2[MAXGEOSECTORS];
extern double geoy2[MAXGEOSECTORS];
extern int geocnt;

extern short ambientlotag[64];
extern short ambienthitag[64];
extern unsigned ambientfx;
extern int msx[MAXANIMPOINTS], msy[MAXANIMPOINTS]; // todo: unlimit
extern TArray<CraneDef> cranes;
extern int WindTime, WindDir;
extern short fakebubba_spawn, mamaspawn_count, banjosound;
extern short BellTime;
extern uint8_t enemysizecheat /*raat607*/, ufospawnsminion, pistonsound, chickenphase /* raat605*/, RRRA_ExitedLevel, fogactive;
extern uint32_t everyothertime;
extern player_orig po[MAXPLAYERS];
extern int32_t g_cdTrack;

END_DUKE_NS

#include "inlines.h"

