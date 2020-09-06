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

extern FString currentGame;
extern FString LumpFilter;
class FArgs;
extern bool GUICapture;
extern bool AppActive;
extern cycle_t drawtime, actortime, thinktime, gameupdatetime;
extern bool r_NoInterpolate;

struct MapRecord;
struct FSaveGameNode;
extern MapRecord* g_nextmap;
extern int g_nextskill;
extern FSaveGameNode* g_savenode;

extern FMemArena dump;	// this is for memory blocks than cannot be deallocated without some huge effort. Put them in here so that they do not register on shutdown.

extern TMap<FName, int32_t> NameToTileIndex;

int CONFIG_Init();

// I am not sure if anything below will survive for long...

#define MAXMOUSEAXES 2
#define MAXMOUSEDIGITAL (MAXMOUSEAXES*2)

// default mouse scale
#define DEFAULTMOUSEANALOGUESCALE           65536

// default joystick settings

#define DEFAULTJOYSTICKANALOGUESCALE        65536
#define DEFAULTJOYSTICKANALOGUEDEAD         1000
#define DEFAULTJOYSTICKANALOGUESATURATE     9500


void CONFIG_SetupJoystick(void);

void CONFIG_SetGameControllerDefaultsClear();

extern FStringCVar* const CombatMacros[];
void CONFIG_ReadCombatMacros();

int GameMain();
int GetAutomapZoom(int gZoom);
inline void DrawOverheadMap(int x, int y, int ang) {} // transitional helper inline.
void DrawCrosshair(int deftile, int health, double xdelta, double scale, PalEntry color = 0xffffffff);
void updatePauseStatus();
void DeferedStartGame(MapRecord* map, int skill);
void ChangeLevel(MapRecord* map, int skill);
void CompleteLevel(MapRecord* map);

struct UserConfig
{
	FString gamegrp;
	FString CommandMap;
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

extern int nomusic;
extern bool nosound;
inline bool MusicEnabled()
{
	return mus_enabled && !nomusic;
}

inline bool SoundEnabled()
{
	return snd_enabled && !nosound;
}


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
	GAMEFLAG_WORLDTOUR = 0x00008000,
	GAMEFLAG_DUKEDC = 0x00010000,
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
	uint32_t CRC = 0;
	uint32_t dependencyCRC = 0;
	size_t size = 0;
	int flags = 0;
	bool loaddirectory = false;
	bool isAddon = false;
	TArray<FString> mustcontain;
	TArray<FString> tobedeleted;
	TArray<FString> loadfiles;
	TArray<FString> loadart;
};


struct GrpEntry
{
	FString FileName;
	GrpInfo FileInfo;
	uint32_t FileIndex;
};
extern int g_gameType;
const char* G_DefaultDefFile(void);
const char* G_DefFile(void);

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

TArray<GrpEntry> GrpScan();
void S_PauseSound(bool notmusic, bool notsfx);
void S_ResumeSound(bool notsfx);
void S_SetSoundPaused(int state);

void G_FatalEngineError(void);
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

enum AM_Mode
{
	am_off,
	am_overlay,
	am_full,
	am_count
};
extern int automapMode;
extern bool automapFollow;
extern bool sendPause;
extern int lastTic;

