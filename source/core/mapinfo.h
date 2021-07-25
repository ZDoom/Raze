#pragma once

#include "gstrings.h"
#include "cmdlib.h"
#include "quotemgr.h"
#include "palentry.h"
#include "vectors.h"
#include "screenjob.h"

#ifdef GetMessage
#undef GetMessage	// Windows strikes...
#endif

enum EMax
{
	MAXSKILLS = 7,
	MAXMENUGAMEPLAYENTRIES = 7,
};

enum EVolFlags
{
	VF_HIDEFROMSP = 1,
	VF_OPTIONAL = 2,
	VF_SHAREWARELOCK = 4,	// show in shareware but lock access.
	VF_NOSKILL = 8,
};

enum EMapFlags
{
	LEVEL_NOINTERMISSION = 1,
	LEVEL_SECRETEXITOVERRIDE = 2,	// when given an explicit level number, override with secret exit in the map, mainly for compiling episodes out of single levels.
	LEVEL_CLEARINVENTORY = 4,
	LEVEL_CLEARWEAPONS = 8,
	LEVEL_FORCENOEOG = 16,			// RR E1L7 needs this to override its boss's death ending the game.
};

enum EMapGameFlags
{
	LEVEL_RR_HULKSPAWN = 1,
	LEVEL_RR_CLEARMOONSHINE = 2,

	LEVEL_EX_COUNTDOWN = 4,
	LEVEL_EX_TRAINING = 8,
	LEVEL_EX_ALTSOUND = 16,
	LEVEL_EX_MULTI = 32,

	LEVEL_SW_SPAWNMINES = 64,
	LEVEL_SW_BOSSMETER_SERPENT = 128,
	LEVEL_SW_BOSSMETER_SUMO = 256,
	LEVEL_SW_BOSSMETER_ZILLA = 512,
	LEVEL_SW_DEATHEXIT_SERPENT = 1024,
	LEVEL_SW_DEATHEXIT_SUMO = 2048,
	LEVEL_SW_DEATHEXIT_ZILLA = 4096,
	LEVEL_SW_DEATHEXIT_SERPENT_NEXT = 8192,

	LEVEL_WT_BOSSSPAWN = 16384,


};

// These get filled in by the map definition parsers of the front ends.
extern FString gSkillNames[MAXSKILLS];
extern int gDefaultVolume, gDefaultSkill;


// Localization capable replacement of the game specific solutions.

inline void MakeStringLocalizable(FString &quote)
{
	// Only prepend a quote if the string is localizable.
	if (quote.Len() > 0 && quote[0] != '$' && GStrings[quote]) quote.Insert(0, "$");
}

enum
{
	MI_FORCEEOG = 1,
	MI_USERMAP = 2,
};

enum {
	MAX_MESSAGES = 32
};

class DObject;
struct MapRecord;

struct GlobalCutscenes
{
	CutsceneDef Intro;
	CutsceneDef DefaultMapIntro;
	CutsceneDef DefaultMapOutro;
	CutsceneDef DefaultGameover;
	CutsceneDef SharewareEnd;
	CutsceneDef LoadingScreen;
	FString MPSummaryScreen;
	FString SummaryScreen;
	FString StatusBarClass;
};

struct ClusterDef
{
	FString name;			// What gets displayed for this cluster. In Duke this is normally the corresponding volume name but does not have to be.
	CutsceneDef intro;		// plays when entering this cluster
	CutsceneDef outro;		// plays when leaving this cluster
	CutsceneDef gameover;	// when defined, plays when the player dies in this cluster
	FString InterBackground;
	int index = -1;
	int flags = 0;			// engine and common flags
};

struct VolumeRecord	// episodes
{
	FString startmap;
	FString name;
	FString subtitle;
	int index = -1;
	int flags = 0;
	int shortcut = 0;
};

struct MapRecord
{
	int parTime = 0;
	int designerTime = 0;
	FString fileName;
	FString labelName;
	FString name;
	FString music;
	FString Author;
	FString NextMap;
	FString NextSecret;
	int cdSongId = -1;
	int musicorder = -1;

	CutsceneDef intro;
	CutsceneDef outro;
	int flags = 0;
	int gameflags = 0;
	int levelNumber = -1;
	int cluster = -1;

	PalEntry fadeto = 0;
	int fogdensity = 0;
	int skyfog = 0;
	FString BorderTexture;
	FString InterBackground;
	TArray<FString> PrecacheTextures;
	FVector4 skyrotatevector;

	// The rest is only used by Blood
	FString messages[MAX_MESSAGES];
	int8_t fog = -1, weather = -1;	// Blood defines these but they aren't used.

	// game specific stuff
	int rr_startsound = 0;
	int rr_mamaspawn = 15;
	int ex_ramses_horiz = 11;
	int ex_ramses_cdtrack = -1; // this is not music, it is the actual dialogue!
	FString ex_ramses_pup;
	FString ex_ramses_text;
	
	const char* LabelName() const
	{
		if (flags & MI_USERMAP) return GStrings("TXT_USERMAP");
		return labelName;
	}
	const char *DisplayName() const
	{
		if (name.IsEmpty()) return labelName;
		return GStrings.localize(name);
	}
	void SetName(const char *n)
	{
		name = n;
		MakeStringLocalizable(name);
	}
	void SetFileName(const char* n)
	{
		if (*n == '/' || *n == '\\') n++;
		fileName = n;
		FixPathSeperator(fileName);
		labelName = ExtractFileBase(n);
	}
	const char* GetMessage(int num)
	{
		if (num < 0 || num>= MAX_MESSAGES) return "";
		return GStrings(messages[num]);
	}
	
	void AddMessage(int num, const FString &msg)
	{
		messages[num] = msg;
	}
};

struct SummaryInfo
{
	int kills;
	int maxkills;
	int secrets;
	int maxsecrets;
	int supersecrets;
	int time;
	int playercount;
	bool cheated;
	bool endofgame;
};

extern GlobalCutscenes globalCutscenes;
extern MapRecord *currentLevel;	

bool SetMusicForMap(const char* mapname, const char* music, bool namehack = false);

MapRecord *FindMapByName(const char *nm);
MapRecord *FindMapByLevelNum(int num);
MapRecord* FindMapByIndexOnly(int clst, int num); // this is for map setup where fallbacks are undesirable.
MapRecord* FindMapByIndex(int clst, int num);
MapRecord *FindNextMap(MapRecord *thismap);
MapRecord* FindNextSecretMap(MapRecord* thismap);
MapRecord* SetupUserMap(const char* boardfilename, const char *defaultmusic = nullptr);
MapRecord* AllocateMap();

VolumeRecord* FindVolume(int index);
ClusterDef* FindCluster(int index);
ClusterDef* AllocateCluster();
VolumeRecord* AllocateVolume();
void SetLevelNum(MapRecord* info, int num);

inline VolumeRecord* MustFindVolume(int index)
{
	auto r = FindVolume(index);
	if (r) return r;
	r = AllocateVolume();
	r->index = index;
	return r;
}
inline ClusterDef* MustFindCluster(int index)
{
	auto r = FindCluster(index);
	if (r) return r;
	r = AllocateCluster();
	r->index = index;
	return r;
}


// These should be the only places converting between level numbers and volume/map pairs
constexpr inline int makelevelnum(int vol, int map)
{
	return vol * 1000 + map;
}

enum
{
	RRENDSLOT = 127
};
