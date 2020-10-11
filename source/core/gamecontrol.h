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
#include "pragmas.h"
#include "binaryangle.h"

extern FString currentGame;
extern FString LumpFilter;
class FArgs;
extern bool GUICapture;
extern bool AppActive;
extern cycle_t drawtime, actortime, thinktime, gameupdatetime;
extern bool r_NoInterpolate;

struct MapRecord;
extern MapRecord* g_nextmap;
extern int g_nextskill;	

extern FMemArena dump;	// this is for memory blocks than cannot be deallocated without some huge effort. Put them in here so that they do not register on shutdown.

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

void DrawCrosshair(int deftile, int health, double xdelta, double ydelta, double scale, PalEntry color = 0xffffffff);
void updatePauseStatus();
void DeferedStartGame(MapRecord* map, int skill, bool nostopsound = false);
void ChangeLevel(MapRecord* map, int skill);
void CompleteLevel(MapRecord* map);

int getincangle(int c, int n);
fixed_t getincangleq16(fixed_t c, fixed_t n);

struct PlayerHorizon
{
	fixedhoriz horiz, ohoriz, horizoff, ohorizoff;
	fixed_t target;
	double adjustment;

	void backup()
	{
		ohoriz = horiz;
		ohorizoff = horizoff;
	}

	void restore()
	{
		horiz = ohoriz;
		horizoff = ohorizoff;
	}

	void addadjustment(double value)
	{
		if (!cl_syncinput)
		{
			adjustment += value;
		}
		else
		{
			horiz += q16horiz(FloatToFixed(value));
		}
	}

	void resetadjustment()
	{
		adjustment = 0;
	}

	void settarget(double value, bool backup = false)
	{
		if (!cl_syncinput)
		{
			target = FloatToFixed(value);
			if (target == 0) target += 1;
		}
		else
		{
			horiz = q16horiz(FloatToFixed(value));
			if (backup) ohoriz = horiz;
		}
	}

	void processhelpers(double const scaleAdjust)
	{
		if (target)
		{
			horiz += q16horiz(xs_CRoundToInt(scaleAdjust * (target - horiz.asq16())));

			if (abs(horiz.asq16() - target) < FRACUNIT)
			{
				horiz = q16horiz(target);
				target = 0;
			}
		}
		else if (adjustment)
		{
			horiz += q16horiz(FloatToFixed(scaleAdjust * adjustment));
		}
	}

	fixedhoriz sum()
	{
		return horiz + horizoff;
	}

	fixedhoriz interpolatedsum(double const smoothratio)
	{
		double const ratio = smoothratio / FRACUNIT;
		fixed_t const prev = (ohoriz + ohorizoff).asq16();
		fixed_t const curr = (horiz + horizoff).asq16();
		return q16horiz(prev + xs_CRoundToInt(ratio * (curr - prev)));
	}
};

struct PlayerAngle
{
	binangle ang, oang;
	lookangle look_ang, olook_ang, rotscrnang, orotscrnang, spin;
	double adjustment, target;

	void backup()
	{
		oang = ang;
		olook_ang = look_ang;
		orotscrnang = rotscrnang;
	}

	void restore()
	{
		ang = oang;
		look_ang = olook_ang;
		rotscrnang = orotscrnang;
	}

	void addadjustment(double value)
	{
		if (!cl_syncinput)
		{
			adjustment += value;
		}
		else
		{
			ang += bamang(xs_CRoundToUInt(value * BAMUNIT));
		}
	}

	void resetadjustment()
	{
		adjustment = 0;
	}

	void settarget(double value, bool backup = false)
	{
		if (!cl_syncinput)
		{
			if (value == 0) value += (1. / BAMUNIT);
			target = xs_CRoundToUInt(value * BAMUNIT);
		}
		else
		{
			ang = bamang(xs_CRoundToUInt(value * BAMUNIT));
			if (backup) oang = ang;
		}
	}

	void processhelpers(double const scaleAdjust)
	{
		if (target)
		{
			ang = bamang(ang.asbam() + xs_CRoundToInt(scaleAdjust * (target - ang.asbam())));

			if (ang.asbam() - target < BAMUNIT)
			{
				ang = bamang(target);
				target = 0;
			}
		}
		else if (adjustment)
		{
			ang += bamang(xs_CRoundToUInt(scaleAdjust * adjustment * BAMUNIT));
		}
	}

	binangle sum()
	{
		return bamang(ang.asbam() + look_ang.asbam());
	}

	binangle interpolatedsum(double const smoothratio)
	{
		double const ratio = smoothratio / FRACUNIT;
		int32_t const dang = UINT32_MAX / 2;
		int64_t const prev = oang.asbam() + olook_ang.asbam();
		int64_t const curr = ang.asbam() + look_ang.asbam();
		return bamang(prev + xs_CRoundToUInt(ratio * (((curr + dang - prev) & 0xFFFFFFFF) - dang)));
	}

	lookangle interpolatedrotscrn(double const smoothratio)
	{
		double const ratio = smoothratio / FRACUNIT;
		return bamlook(orotscrnang.asbam() + xs_CRoundToInt(ratio * (rotscrnang.asbam() - orotscrnang.asbam())));
	}
};

void processMovement(InputPacket* currInput, InputPacket* inputBuffer, ControlInfo* const hidInput, double const scaleAdjust, int const drink_amt = 0, bool const allowstrafe = true, double const turnscale = 1);
void sethorizon(fixedhoriz* horiz, float const horz, ESyncBits* actions, double const scaleAdjust);
void applylook(PlayerAngle* angle, float const avel, ESyncBits* actions, double const scaleAdjust, bool const crouching);

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

extern bool sendPause;
extern int lastTic;

//---------------------------------------------------------------------------
//
// Return half player's q16look_ang directly or interpolated as required.
//
//---------------------------------------------------------------------------

inline double getHalfLookAng(fixed_t const oq16look_ang, fixed_t const q16look_ang, bool interpolate, double smoothratio)
{
	return (!interpolate ? q16look_ang : oq16look_ang + fmulscale16(q16look_ang - oq16look_ang, smoothratio)) * (0.5 / FRACUNIT);
}
