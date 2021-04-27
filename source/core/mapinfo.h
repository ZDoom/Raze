#pragma once

#include "gstrings.h"
#include "cmdlib.h"
#include "quotemgr.h"
#ifdef GetMessage
#undef GetMessage	// Windows strikes...
#endif


enum EMax
{
	MAXSKILLS = 7,
	MAXVOLUMES = 7,
	MAXMENUGAMEPLAYENTRIES = 7,
};

enum EVolFlags
{
	EF_HIDEFROMSP = 1,
	EF_GOTONEXTVOLUME = 2,	// for RR which continues the game in the next volume
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

struct CutsceneDef
{
	FString video;
	FString function;
	int sound = 0;
	int framespersec = 0; // only relevant for ANM.

	void Create(DObject* runner);
	bool Create(DObject* runner, MapRecord* map);
};

struct GlobalCutscenes
{
	CutsceneDef Intro;
	CutsceneDef DefaultMapIntro;
	CutsceneDef DefaultMapOutro;
	CutsceneDef SharewareEnd;
	CutsceneDef LoadingScreen;
	FString MPSummaryScreen;
	FString SummaryScreen;
};

struct VolumeRecord
{
	FString name;
	FString subtitle;
	CutsceneDef intro;
	CutsceneDef outro;
	int32_t flags = 0;
};

struct MapRecord
{
	int parTime = 0;
	int designerTime = 0;
	FString fileName;
	FString labelName;
	FString name;
	FString music;
	CutsceneDef intro;
	CutsceneDef outro;
	int cdSongId = -1;
	int flags = 0;
	int levelNumber = -1;
	int cluster = -1;

	// The rest is only used by Blood
	int nextLevel = -1;
	int nextSecret = -1;
	FString messages[MAX_MESSAGES];
	FString author;
	int8_t fog = -1, weather = -1;	// Blood defines these but they aren't used.
	
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
extern VolumeRecord volumeList[MAXVOLUMES];
extern MapRecord *currentLevel;	

bool SetMusicForMap(const char* mapname, const char* music, bool namehack = false);

MapRecord *FindMapByName(const char *nm);
MapRecord *FindMapByLevelNum(int num);
MapRecord* FindMapByClusterAndLevelNum(int clst, int num);
MapRecord *FindNextMap(MapRecord *thismap);
MapRecord* SetupUserMap(const char* boardfilename, const char *defaultmusic = nullptr);
MapRecord* AllocateMap();

// These should be the only places converting between level numbers and volume/map pairs
constexpr inline int levelnum(int vol, int map)
{
	return vol * 1000 + map;
}

constexpr inline int volfromlevelnum(int num)
{
	return num >= 0 ? num / 1000 : 0;
}

constexpr inline int mapfromlevelnum(int num)
{
	return num >= 0 ? num % 1000 : -1;
}


enum
{
	RRENDSLOT = 127
};
