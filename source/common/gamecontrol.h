#pragma once

#include "keyboard.h"
#include "control.h"
#include "_control.h"
#include "c_cvars.h"
#include "zstring.h"
#include "inputstate.h"
#include "gamecvars.h"

extern FString currentGame;
class FArgs;


extern uint8_t KeyboardKeys[NUMGAMEFUNCTIONS][2];

void CONFIG_Init();
void CONFIG_SetDefaultKeys(const char *defbinds, bool lazy=false);
int32_t CONFIG_FunctionNameToNum(const char* func);
const char* CONFIG_FunctionNumToName(int32_t func);
const char* CONFIG_FunctionNumToRealName(int32_t func);
void CONFIG_ReplaceButtonName(int num, const char* text);
void CONFIG_DeleteButtonName(int num);
void CONFIG_MapKey(int which, kb_scancode key1, kb_scancode oldkey1, kb_scancode key2, kb_scancode oldkey2);

// I am not sure if anything below will survive for long...

#define MAXMOUSEAXES 2
#define MAXMOUSEDIGITAL (MAXMOUSEAXES*2)

// default mouse scale
#define DEFAULTMOUSEANALOGUESCALE           65536

// default joystick settings

#define DEFAULTJOYSTICKANALOGUESCALE        65536
#define DEFAULTJOYSTICKANALOGUEDEAD         1000
#define DEFAULTJOYSTICKANALOGUESATURATE     9500


extern int32_t MouseFunctions[MAXMOUSEBUTTONS][2];
extern int32_t MouseDigitalFunctions[MAXMOUSEAXES][2];
extern int32_t MouseAnalogueAxes[MAXMOUSEAXES];
extern int32_t MouseAnalogueScale[MAXMOUSEAXES];
extern int32_t JoystickFunctions[MAXJOYBUTTONSANDHATS][2];
extern int32_t JoystickDigitalFunctions[MAXJOYAXES][2];
extern int32_t JoystickAnalogueAxes[MAXJOYAXES];
extern int32_t JoystickAnalogueScale[MAXJOYAXES];
extern int32_t JoystickAnalogueDead[MAXJOYAXES];
extern int32_t JoystickAnalogueSaturate[MAXJOYAXES];
extern int32_t JoystickAnalogueInvert[MAXJOYAXES];

int32_t CONFIG_AnalogNameToNum(const char* func);
const char* CONFIG_AnalogNumToName(int32_t func);
void CONFIG_SetupMouse(void);
void CONFIG_SetupJoystick(void);
void CONFIG_WriteControllerSettings();
void CONFIG_InitMouseAndController();

void CONFIG_SetGameControllerDefaultsStandard();
void CONFIG_SetGameControllerDefaultsPro();
void CONFIG_SetGameControllerDefaultsClear();

FString CONFIG_GetBoundKeyForLastInput(int gameFunc);

int osdcmd_bind(osdcmdptr_t parm);
int osdcmd_unbindall(osdcmdptr_t);
int osdcmd_unbind(osdcmdptr_t parm);


extern FStringCVar* const CombatMacros[];
void CONFIG_ReadCombatMacros();

int32_t CONFIG_GetMapBestTime(char const* const mapname, uint8_t const* const mapmd4);
int CONFIG_SetMapBestTime(uint8_t const* const mapmd4, int32_t tm);

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

	bool nomonsters = false;
	bool nosound = false;
	bool nomusic = false;
	bool nologo = false;
	int setupstate = -1;

	int netPort = 0;			// g_netPort = Batoi(argv[i + 1]);
	int netServerMode = -1;		// g_networkMode = NET_SERVER;	g_noSetup = g_noLogo = TRUE;
	FString netServerAddress;	// Net_Connect(argv[i + 1]); g_noSetup = g_noLogo = TRUE;
	FString netPassword;		// Bstrncpyz(g_netPassword, argv[i + 1], sizeof(g_netPassword));

	void ProcessOptions();
};

extern UserConfig userConfig;

inline bool MusicEnabled()
{
	return mus_enabled && !userConfig.nomusic;
}

inline bool SoundEnabled()
{
	return snd_enabled && !userConfig.nosound;
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
	GAMEFLAG_BLOOD      = 0x00000400,
	GAMEFLAG_STANDALONE = 0x00000800,
	GAMEFLAGMASK        = 0x000007FF, // flags allowed from grpinfo

};


struct GrpInfo
{
	FString name;
	FString scriptname;
	FString dirname;
	FString defname;
	FString rtsname;
	FString gamefilter;
	uint32_t CRC = 0;
	uint32_t dependencyCRC = 0;
	size_t size = 0;
	int flags = 0;
	TArray<FString> loadfiles;
	TArray<FString> loadart;
};



struct GrpEntry
{
	FString FileName;
	GrpInfo FileInfo;
	uint32_t FileIndex;
};

TArray<GrpEntry> GrpScan();
