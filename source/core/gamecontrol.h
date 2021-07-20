#pragma once

#include <memory>
#include "c_cvars.h"
#include "zstring.h"
#include "inputstate.h"
#include "gamecvars.h"
#include "tarray.h"
#include "name.h"
#include "memarena.h"
#include "stats.h"
#include "i_time.h"
#include "palentry.h"
#include "build.h"

EXTERN_CVAR(Bool, hud_textfont)

extern bool sendsave;
extern FString	savedescription;
extern FString	savegamefile;

extern FString currentGame;
extern FString LumpFilter;
extern int PlayClock;
class FArgs;
extern bool GUICapture;
extern bool AppActive;
extern cycle_t drawtime, actortime, thinktime, gameupdatetime;
extern bool r_NoInterpolate;
extern bool crouch_toggle;

struct MapRecord;
extern MapRecord* g_nextmap;
extern int g_nextskill;	

extern FMemArena dump;	// this is for memory blocks than cannot be deallocated without some huge effort. Put them in here so that they do not register on shutdown.

extern FStringCVar* const CombatMacros[];
void CONFIG_ReadCombatMacros();

int GameMain();
int GetAutomapZoom(int gZoom);

void DrawCrosshair(int deftile, int health, double xdelta, double ydelta, double scale, PalEntry color = 0xffffffff);
void updatePauseStatus();
void DeferredStartGame(MapRecord* map, int skill, bool nostopsound = false);
void ChangeLevel(MapRecord* map, int skill);
void CompleteLevel(MapRecord* map);

struct UserConfig
{
	FString gamegrp;
	FString CommandMap;
	FString UserDef;
	FString DefaultDef;
	FString DefaultCon;
	FString CommandDemo;
	FString CommandName;
	FString CommandIni;
	std::unique_ptr<FArgs> AddDefs;
	std::unique_ptr<FArgs> AddCons;
	std::unique_ptr<FArgs> AddFiles;
	std::unique_ptr<FArgs> AddFilesPre;	//To be added before the main directory. Only for legacy options.
	std::unique_ptr<FArgs> AddArt;
	TArray<FString> toBeDeleted;

	bool nomonsters = false;
	bool nosound = false;
	//bool nomusic = false;
	bool nologo = false;
	int setupstate = -1;

	void ProcessOptions();
};

extern UserConfig userConfig;


enum
{
	GAMEFLAG_DUKE       = 0x00000001,
	GAMEFLAG_NAM        = 0x00000002,
	GAMEFLAG_NAPALM     = 0x00000004,
	GAMEFLAG_WW2GI      = 0x00000008,
	GAMEFLAG_ADDON      = 0x00000010,
	GAMEFLAG_SHAREWARE  = 0x00000020,
	GAMEFLAG_DUKEBETA   = 0x00000060, // includes 0x20 since it's a shareware beta
	GAMEFLAG_PLUTOPAK	= 0x00000080,
	GAMEFLAG_RR         = 0x00000100,
	GAMEFLAG_RRRA       = 0x00000200,
	GAMEFLAG_RRALL		= GAMEFLAG_RR | GAMEFLAG_RRRA,
	GAMEFLAG_BLOOD      = 0x00000800,
	GAMEFLAG_SW			= 0x00001000,
	GAMEFLAG_POWERSLAVE	= 0x00002000,
	GAMEFLAG_EXHUMED	= 0x00004000,
	GAMEFLAG_PSEXHUMED  = GAMEFLAG_POWERSLAVE | GAMEFLAG_EXHUMED,	// the two games really are the same, except for the name and the publisher.
	GAMEFLAG_WORLDTOUR	= 0x00008000,
	GAMEFLAG_DUKEDC		= 0x00010000,
	GAMEFLAG_DUKENW		= 0x00020000,
	GAMEFLAG_DUKEVACA	= 0x00040000,
	GAMEFLAG_BLOODCP	= 0x00080000,
	GAMEFLAG_ROUTE66	= 0x00100000,
	GAMEFLAG_SWWANTON	= 0x00200000,
	GAMEFLAG_SWTWINDRAG	= 0x00400000,

	GAMEFLAG_DUKECOMPAT = GAMEFLAG_DUKE | GAMEFLAG_NAM | GAMEFLAG_NAPALM | GAMEFLAG_WW2GI | GAMEFLAG_RRALL,
	GAMEFLAGMASK        = 0x0000FFFF, // flags allowed from grpinfo

	// We still need these for the parsers.
	GAMEFLAG_FURY = 0,
	GAMEFLAG_DEER = 0,

};


struct GrpInfo
{
	FString name;
	FString scriptname;
	FString defname;
	FString rtsname;
	FString gamefilter;
	FString gameid;
	uint32_t CRC = 0;
	uint32_t dependencyCRC = 0;
	size_t size = 0;
	int flags = 0;
	bool loaddirectory = false;
	bool isAddon = false;
	int index = -1;
	TArray<FString> mustcontain;
	TArray<FString> tobedeleted;
	TArray<FString> loadfiles;
	TArray<FString> loadart;
	TArray<FString> exclepisodes;
	uint32_t FgColor = 0, BgColor = 0;
};


struct GrpEntry
{
	FString FileName;
	GrpInfo FileInfo;
};
extern int g_gameType;
const char* G_DefaultDefFile(void);
const char* G_DefFile(void);
void LoadDefinitions();

// game check shortcuts
inline bool isNam()
{
	return g_gameType & (GAMEFLAG_NAM | GAMEFLAG_NAPALM);
}

inline bool isNamWW2GI()
{
	return g_gameType & (GAMEFLAG_NAM | GAMEFLAG_NAPALM |GAMEFLAG_WW2GI);
}

inline bool isWW2GI()
{
	return g_gameType & (GAMEFLAG_WW2GI);
}

inline bool isDuke()
{
	return g_gameType & (GAMEFLAG_DUKE);
}

inline bool isRR()
{
	return g_gameType & (GAMEFLAG_RRALL);
}

inline bool isRRRA()
{
	return g_gameType & (GAMEFLAG_RRRA);
}

inline bool isWorldTour()
{
	return g_gameType & GAMEFLAG_WORLDTOUR;
}

inline bool isPlutoPak()
{
	return g_gameType & GAMEFLAG_PLUTOPAK;
}

inline bool isShareware()
{
	return g_gameType & GAMEFLAG_SHAREWARE;
}

inline bool isBlood()
{
	return g_gameType & GAMEFLAG_BLOOD;
}

inline bool isSWALL()
{
	return g_gameType & (GAMEFLAG_SW | GAMEFLAG_SWWANTON | GAMEFLAG_SWTWINDRAG);
}

TArray<GrpEntry> GrpScan();
void S_PauseSound(bool notmusic, bool notsfx);
void S_ResumeSound(bool notsfx);
void S_SetSoundPaused(int state);

enum
{
	MaxSmoothRatio = FRACUNIT
};

FString G_GetDemoPath();

enum
{
	PAUSESFX_MENU = 1,
	PAUSESFX_CONSOLE = 2
};

extern int paused;
extern int chatmodeon;

extern bool sendPause;
extern int lastTic;

extern int PlayClock;

enum gameaction_t : int
{
	ga_nothing,
	ga_level,				// Switch to play mode without any initialization
	ga_intro,
	ga_intermission,

	ga_startup,				// go back to intro after uninitializing the game state
	ga_mainmenu,			// go back to main menu after uninitializing the game state
	ga_mainmenunostopsound,	// Same but doesn't stop playing sounds.
	ga_creditsmenu,			// go to the credits menu after uninitializing the game state
	ga_newgame,				// start a new game
	ga_recordgame,			// start a new demo recording (later)
	ga_loadgame,			// load a savegame and resume play.
	ga_loadgameplaydemo,	// load a savegame and play a demo.
	ga_autoloadgame,		// load last autosave and resume play.
	ga_savegame,			// save the game
	ga_autosave,			// autosave the game (for triggering a save from within the game.)
	ga_completed,			// Level was exited.
	ga_nextlevel,			// Actually start the next level.
	ga_loadgamehidecon,
	ga_newgamenostopsound,	// start a new game
	ga_endscreenjob,

	ga_fullconsole,
};
extern gameaction_t		gameaction;
