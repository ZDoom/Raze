#pragma once

#include "gstrings.h"
#include "cmdlib.h"
#include "quotemgr.h"
#ifdef GetMessage
#undef GetMessage	// Windows strikes...
#endif

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

struct MapRecord
{
	int parTime = 0;
	int designerTime = 0;
	FString fileName;
	FString labelName;
	FString name;
	FString music;
	int cdSongId = -1;
	int flags = 0;
	int levelNumber = -1;

	// The rest is only used by Blood
	int nextLevel = -1;
	int nextSecret = -1;
	int messageStart = 0;	// messages are stored in the quote array to reduce clutter.
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
		fileName = n;
		FixPathSeperator(fileName);
		labelName = ExtractFileBase(n);
	}
	const char* GetMessage(int num)
	{
		return quoteMgr.GetQuote(messageStart + num);
	}

};


extern MapRecord mapList[512];
extern MapRecord *currentLevel;	

bool SetMusicForMap(const char* mapname, const char* music, bool namehack = false);
void InitRREndMap();

MapRecord *FindMapByName(const char *nm);
MapRecord *FindMapByLevelNum(int num);
MapRecord *FindNextMap(MapRecord *thismap);
MapRecord* SetupUserMap(const char* boardfilename, const char *defaultmusic = nullptr);
MapRecord* AllocateMap();

enum
{
	RRENDSLOT = 127
};