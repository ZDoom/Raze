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


// global crap for event management
extern int g_i, g_p;
extern int g_x;
extern int* g_t;
extern uint8_t killit_flag;
//extern sprite_ype* g_sp;

bool AddGameVar(const char *pszLabel, intptr_t lValue, unsigned dwFlags);
int GetGameID(const char *szGameLabel);
int GetDefID(const char *szGameLabel);
void FreeGameVars(void);
void ClearGameVars(void);
void AddSystemVars();
void ResetGameVars(void);
int GetGameVarID(int id, int sActor, int sPlayer);
void SetGameVarID(int id, int lValue, int sActor, int sPlayer);
int GetGameVar(char* szGameLabel, int lDefault, int sActor, int sPlayer);

void ClearGameEvents();
bool IsGameEvent(int i);
void InitGameVarPointers(void);
void ResetSystemDefaults(void);



END_DUKE_NS
