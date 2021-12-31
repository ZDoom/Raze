#pragma once

#include "names.h"

BEGIN_DUKE_NS

// gamedef.c

class DDukeActor;

// Game vars can reference actors, we need a type-safe way to handle that so that index values won't get misappropriated and actors can be GC'd.
class GameVarValue
{
	enum EType
	{
		Actor = 0,
		Value = 1
	};
	union
	{
		DDukeActor* ActorP;
		uint64_t index;
	};

public:
	GameVarValue() = default;
	explicit GameVarValue(DDukeActor* actor_) { ActorP = actor_; assert(isActor()); }
	explicit GameVarValue(int val) { index = (val << 8) | Value; }

	bool isActor() const { return (index & 7) == Actor; }
	bool isValue() const { return (index & 7) == Value; }

	DDukeActor* actor() { assert(isActor()); return GC::ReadBarrier(ActorP); }
	int value() { assert(isValue()); return index >> 8; }
	int safeValue() { return isValue() ? value() : actor() == nullptr ? 0 : -1; }	// return -1 for valid actors and 0 for null. This allows most comparisons to work.
	DDukeActor* safeActor() { return isActor() ? actor() : nullptr; }

	bool operator==(const GameVarValue& other) const { return index == other.index; }
	bool operator!=(const GameVarValue& other) const { return index != other.index; }
	void Mark()
	{
		if (isActor()) GC::Mark(ActorP);
	}
};

enum
{
	// store global game definitions
	MAXGAMEVARS = 512,
	MAXVARLABEL = 26,
	MAXGAMEEVENTS = 128

};

enum
{
	GAMEVAR_FLAG_PERPLAYER = 1,	// per-player variable
	GAMEVAR_FLAG_PERACTOR = 2,	// per-actor variable
	GAMEVAR_FLAG_USER_MASK = 3,

		// internal flags only...
	GAMEVAR_FLAG_DEFAULT = 256,		// allow override
	GAMEVAR_FLAG_SECRET = 512,		// don't dump...
	GAMEVAR_FLAG_NODEFAULT = 1024,	// don't add to 'default' array.	
	GAMEVAR_FLAG_SYSTEM = 2048,		// cannot change mode flags...(only default value)
	GAMEVAR_FLAG_READONLY = 4096,	// values are read-only (no setvar allowed)
	GAMEVAR_FLAG_PLONG = 8192,		// plValue is a pointer to a long
	GAMEVAR_FLAG_PFUNC = 8192,		// plValue is a pointer to a getter function
};

enum
{
	NAM_GRENADE_LIFETIME = 120,
	NAM_GRENADE_LIFETIME_VAR = 30,
};

// Keep the gory details away from the main game code.

int aplWeaponClip(int weapon, int player);		// number of items in clip
int aplWeaponReload(int weapon, int player);		// delay to reload (include fire)
int aplWeaponFireDelay(int weapon, int player);	// delay to fire
int aplWeaponHoldDelay(int weapon, int player);	// delay after release fire button to fire (0 for none)
int aplWeaponTotalTime(int weapon, int player);	// The total time the weapon is cycling before next fire.
int aplWeaponFlags(int weapon, int player);		// Flags for weapon
int aplWeaponShoots(int weapon, int player);		// what the weapon shoots
int aplWeaponSpawnTime(int weapon, int player);	// the frame at which to spawn an item
int aplWeaponSpawn(int weapon, int player);		// the item to spawn
int aplWeaponShotsPerBurst(int weapon, int player);	// number of shots per 'burst' (one ammo per 'burst'
int aplWeaponWorksLike(int weapon, int player);	// What original the weapon works like
int aplWeaponInitialSound(int weapon, int player);	// Sound made when initialy firing. zero for no sound
int aplWeaponFireSound(int weapon, int player);	// Sound made when firing (each time for automatic)
int aplWeaponSound2Time(int weapon, int player);	// Alternate sound time
int aplWeaponSound2Sound(int weapon, int player);	// Alternate sound sound ID


enum
{
	// 'holstering' clears the current clip
	WEAPON_FLAG_HOLSTER_CLEARS_CLIP = 1,
	// weapon 'glows' (shrinker and grower)
	WEAPON_FLAG_GLOWS = 2,
	// automatic fire (continues while 'fire' is held down
	WEAPON_FLAG_AUTOMATIC = 4,
	// during 'hold time' fire every frame
	WEAPON_FLAG_FIREEVERYOTHER = 8,
	// during 'hold time' fire every third frame.
	WEAPON_FLAG_FIREEVERYTHIRD = 16,
	// restart for automatic is 'randomized' by RND 3
	WEAPON_FLAG_RANDOMRESTART = 32,
	// uses ammo for each shot (for automatic)
	WEAPON_FLAG_AMMOPERSHOT = 64,
	// weapon is the 'bomb' trigger
	WEAPON_FLAG_BOMB_TRIGGER = 128,
	// weapon use does not cause user to become 'visible'
	WEAPON_FLAG_NOVISIBLE = 256,
	// weapon 'throws' the 'shoots' item...
	WEAPON_FLAG_THROWIT = 512,
	// check weapon availability at 'reload' time
	WEAPON_FLAG_CHECKATRELOAD = 1024,
	// player stops jumping before actual fire (like tripbomb in duke)
	WEAPON_FLAG_STANDSTILL = 2048,
	// just spawn
	WEAPON_FLAG_SPAWNTYPE1 = 0,
	// spawn like shotgun shells
	WEAPON_FLAG_SPAWNTYPE2 = 4096,
	// spawn like chaingun shells
	WEAPON_FLAG_SPAWNTYPE3 = 8192
};


struct MATTGAMEVAR
{
	union
	{
		GameVarValue lValue;
		int indexValue;
		int* plValue;
		int (*getter)();
	};
	GameVarValue defaultValue;
	GameVarValue initValue;	// this is what gets copied to players/actors upon spawn. This is not the same as the default!
	unsigned int dwFlags;
	char szLabel[MAXVARLABEL];
};

extern MATTGAMEVAR aGameVars[MAXGAMEVARS];
extern int iGameVarCount;

extern int g_iReturnVarID;	// var ID of "RETURN"
extern int g_iWeaponVarID;	// var ID of "WEAPON"
extern int g_iWorksLikeVarID;	// var ID of "WORKSLIKE"
extern int g_iZRangeVarID;	// var ID of "ZRANGE"
extern int g_iAngRangeVarID;	// var ID of "ANGRANGE"
extern int g_iAimAngleVarID;	// var ID of "AUTOAIMANGLE"
extern int g_iAtWithVarID;		// var ID of "AtWith"
extern int g_iLoTagID;			// var ID of "LOTAG"
extern int g_iHiTagID;			// ver ID of "HITAG"
extern int g_iTextureID;		// var ID of "TEXTURE"
extern int g_iThisActorID;		// var ID of "THISACTOR"

int AddGameVar(const char *pszLabel, intptr_t lValue, unsigned dwFlags);
int GetGameID(const char *szGameLabel);
int GetDefID(const char *szGameLabel);
void ClearGameVars(void);
void AddSystemVars();
void ResetGameVars(void);
class DDukeActor;
class GameVarValue;
GameVarValue GetGameVarID(int id, DDukeActor* sActor, int sPlayer);
void SetGameVarID(int id, GameVarValue lValue, DDukeActor* sActor, int sPlayer);
GameVarValue GetGameVar(const char* szGameLabel, GameVarValue lDefault, DDukeActor* sActor, int sPlayer);
GameVarValue GetGameVar(const char* szGameLabel, int lDefault, DDukeActor* sActor, int sPlayer);
void SetGameVarID(int id, int lValue, DDukeActor* sActor, int sPlayer);
void SetGameVarID(int id, DDukeActor* lValue, DDukeActor* sActor, int sPlayer);


void ClearGameEvents();
bool IsGameEvent(int i);
void InitGameVarPointers(void);
void FinalizeGameVars(void);
void SetupGameVarsForActor(DDukeActor* actor);



END_DUKE_NS
