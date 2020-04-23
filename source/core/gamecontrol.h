#pragma once

#include <memory>
#include "c_cvars.h"
#include "zstring.h"
#include "inputstate.h"
#include "gamecvars.h"
#include "tarray.h"
#include "name.h"
#include "memarena.h"

extern FString currentGame;
extern FString LumpFilter;
class FArgs;
extern bool GUICapture;

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

int32_t CONFIG_GetMapBestTime(char const* const mapname, uint8_t const* const mapmd4);
int CONFIG_SetMapBestTime(uint8_t const* const mapmd4, int32_t tm);
int GameMain();

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

	int netPort = 0;			// g_netPort = Batoi(argv[i + 1]);
	int netServerMode = -1;		// g_networkMode = NET_SERVER;	g_noSetup = g_noLogo = TRUE;
	FString netServerAddress;	// Net_Connect(argv[i + 1]); g_noSetup = g_noLogo = TRUE;
	FString netPassword;		// Bstrncpyz(g_netPassword, argv[i + 1], sizeof(g_netPassword));

	void ProcessOptions();
};

extern UserConfig userConfig;

extern int nomusic;
extern bool nosound;
inline bool MusicEnabled()
{
	return !nomusic;
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
	GAMEFLAG_FURY       = 0x00000080,
	GAMEFLAG_RR         = 0x00000100,
	GAMEFLAG_RRRA       = 0x00000200,
	GAMEFLAG_DEER		= 0x00000400,
	GAMEFLAG_RRALL		= GAMEFLAG_RR | GAMEFLAG_RRRA | GAMEFLAG_DEER,
	GAMEFLAG_BLOOD      = 0x00000800,
	GAMEFLAG_SW			= 0x00001000,
	GAMEFLAG_POWERSLAVE	= 0x00002000,
	GAMEFLAG_EXHUMED	= 0x00004000,
	GAMEFLAG_PSEXHUMED  = GAMEFLAG_POWERSLAVE | GAMEFLAG_EXHUMED,	// the two games really are the same, except for the name and the publisher.
	GAMEFLAG_STANDALONE = 0x00008000,
	GAMEFLAGMASK        = 0x00007FFF, // flags allowed from grpinfo

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
const char* G_DefaultConFile(void);
const char* G_ConFile(void);

TArray<GrpEntry> GrpScan();

void G_FatalEngineError(void);
int CalcSmoothRatio(const ClockTicks& totalclk, const ClockTicks& ototalclk, int realgameticspersec);

FString G_GetDemoPath();

enum
{
	PAUSESFX_MENU = 1,
	PAUSESFX_CONSOLE = 2
};

