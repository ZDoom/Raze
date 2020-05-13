#pragma once

BEGIN_DUKE_NS

// gamedef.c
//void OnEvent(int iEventID, int i,int p,int x);

enum
{
	EVENT_INIT = 0,
	EVENT_ENTERLEVEL,
	EVENT_RESETWEAPONS,	// for each player
	EVENT_RESETINVENTORY, // for each player
	EVENT_HOLSTER,		// for each player
	EVENT_LOOKLEFT,		// for each player
	EVENT_LOOKRIGHT,	// for each player
	EVENT_SOARUP,		// for each player
	EVENT_SOARDOWN,		// for each player
	EVENT_CROUCH,		// for each player
	EVENT_JUMP,			// for each player
	EVENT_RETURNTOCENTER,	// for each player
	EVENT_LOOKUP,		// for each player
	EVENT_LOOKDOWN,		// for each player
	EVENT_AIMUP,		// for each player
	EVENT_AIMDOWN,		// for each player
	EVENT_FIRE,			// for each player
	EVENT_CHANGEWEAPON,	// for each player
	EVENT_GETSHOTRANGE,	// for each player
	EVENT_GETAUTOAIMANGLE,	// for each player
	EVENT_GETLOADTILE,
	
	EVENT_CHEATGETSTEROIDS,
	EVENT_CHEATGETHEAT,
	EVENT_CHEATGETBOOT,
	EVENT_CHEATGETSHIELD,
	EVENT_CHEATGETSCUBA,
	EVENT_CHEATGETHOLODUKE,
	EVENT_CHEATGETJETPACK,
	EVENT_CHEATGETFIRSTAID,
	EVENT_QUICKKICK,
	EVENT_INVENTORY,
	EVENT_USENIGHTVISION,
	EVENT_USESTEROIDS,
	EVENT_INVENTORYLEFT,
	EVENT_INVENTORYRIGHT,
	EVENT_HOLODUKEON,
	EVENT_HOLODUKEOFF,
	EVENT_USEMEDKIT,
	EVENT_USEJETPACK,
	EVENT_TURNAROUND,
	
	EVENT_NUMEVENTS,
	EVENT_MAXEVENT = EVENT_NUMEVENTS-1
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
};	

enum
{
	NAM_GRENADE_LIFETIME = 120,
	NAM_GRENADE_LIFETIME_VAR = 30,
};

extern int* aplWeaponClip[MAX_WEAPONS];		// number of items in clip
extern int* aplWeaponReload[MAX_WEAPONS];		// delay to reload (include fire)
extern int* aplWeaponFireDelay[MAX_WEAPONS];	// delay to fire
extern int* aplWeaponHoldDelay[MAX_WEAPONS];	// delay after release fire button to fire (0 for none)
extern int* aplWeaponTotalTime[MAX_WEAPONS];	// The total time the weapon is cycling before next fire.
extern int* aplWeaponFlags[MAX_WEAPONS];		// Flags for weapon
extern int* aplWeaponShoots[MAX_WEAPONS];		// what the weapon shoots
extern int* aplWeaponSpawnTime[MAX_WEAPONS];	// the frame at which to spawn an item
extern int* aplWeaponSpawn[MAX_WEAPONS];		// the item to spawn
extern int* aplWeaponShotsPerBurst[MAX_WEAPONS];	// number of shots per 'burst' (one ammo per 'burst'
extern int* aplWeaponWorksLike[MAX_WEAPONS];	// What original the weapon works like
extern int* aplWeaponInitialSound[MAX_WEAPONS];	// Sound made when initialy firing. zero for no sound
extern int* aplWeaponFireSound[MAX_WEAPONS];	// Sound made when firing (each time for automatic)
extern int* aplWeaponSound2Time[MAX_WEAPONS];	// Alternate sound time
extern int* aplWeaponSound2Sound[MAX_WEAPONS];	// Alternate sound sound ID


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


typedef struct
{
	union
	{
		int lValue;
		int* plValue;
	};
	int defaultValue;
	unsigned int dwFlags;
	char szLabel[MAXVARLABEL];
	TArray<int> plArray;
} MATTGAMEVAR;

extern MATTGAMEVAR aGameVars[MAXGAMEVARS];
extern int iGameVarCount;

extern int g_iReturnVarID;	// var ID of "RETURN"
extern int g_iWeaponVarID;	// var ID of "WEAPON"
extern int g_iWorksLikeVarID;	// var ID of "WORKSLIKE"
extern int g_iZRangeVarID;	// var ID of "ZRANGE"
extern int g_iAngRangeVarID;	// var ID of "ANGRANGE"
extern int g_iAimAngleVarID;	// var ID of "AUTOAIMANGLE"

#if 0
// global crap for event management
extern int g_i, g_p;
extern int g_x;
extern int* g_t;
extern uint8_t killit_flag;
//extern sprite_ype* g_sp;
#endif

bool AddGameVar(const char *pszLabel, intptr_t lValue, unsigned dwFlags);
int GetGameID(const char *szGameLabel);
int GetDefID(const char *szGameLabel);
void ClearGameVars(void);
void AddSystemVars();
void ResetGameVars(void);
int GetGameVarID(int id, int sActor, int sPlayer);
void SetGameVarID(int id, int lValue, int sActor, int sPlayer);
int GetGameVar(const char* szGameLabel, int lDefault, int sActor, int sPlayer);

void ClearGameEvents();
bool IsGameEvent(int i);
void InitGameVarPointers(void);
void ResetSystemDefaults(void);



END_DUKE_NS
