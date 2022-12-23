#pragma once

#include "build.h"
#include "duke3d.h"
#include "quotemgr.h"
#include "sounds.h"
#include "constants.h"
#include "types.h"
#include "d_net.h"
#include "serialize_obj.h"
#include "tiletexture.h"

BEGIN_DUKE_NS

extern user_defs ud;

struct DukeGameInfo
{
	// Static constant global state
	double playerfriction;
	double gravity;
	double playerheight;
	double gutsscale;

	int respawnactortime;
	int bouncemineblastradius;
	int respawnitemtime;
	int morterblastradius;
	int numfreezebounces;
	int pipebombblastradius;
	int rpgblastradius;
	int seenineblastradius;
	int shrinkerblastradius;
	int tripbombblastradius;
	int camerashitable;
	int max_player_health;
	int max_armour_amount;
	int lasermode;
	int freezerhurtowner;
	int impact_damage;
	int firstdebris;

	ActorInfo actorinfo[MAXTILES];
	int16_t max_ammo_amount[MAX_WEAPONS];
	int16_t weaponsandammosprites[15];
	int displayflags;

	// global gamevars from WW2GI. Put here so we can modify these values without having to depend on CON.
	int tripbombcontrol;
	int stickybomb_lifetime;
	int stickybomb_lifetime_var;
	int grenade_lifetime;
	int grenade_lifetime_var;
};

extern DukeGameInfo gs;

inline TObjPtr<DDukeActor*> camsprite;
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

extern player_struct ps[MAXPLAYERS];
extern int spriteqamount;
extern int lastvisinc;
extern animwalltype animwall[MAXANIMWALLS];
extern int numanimwalls;
extern int numclouds;
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

struct animate
{
	sectortype* sect;
	int target;
	int8_t type;
	double goal;
	double vel;

};

extern TArray<animate> animates;

extern sectortype* clouds[256];
extern float cloudx;
extern float cloudy;
extern int cloudclock;

extern TArray<Cycler> cyclers;
extern TArray<AmbientTags> ambienttags;
extern sectortype* mirrorsector[64];
extern walltype* mirrorwall[64];

extern int wupass;
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

extern TArray<DVector2> mspos;
extern int WindTime;
extern DAngle WindDir;
extern short fakebubba_spawn, mamaspawn_count, banjosound;
extern uint8_t enemysizecheat /*raat607*/, chickenphase /* raat605*/, RRRA_ExitedLevel;
extern player_orig po[MAXPLAYERS];
extern int32_t g_cdTrack;

END_DUKE_NS

#include "inlines.h"

