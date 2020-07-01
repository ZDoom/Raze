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
	
	const char *DisplayName()
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
extern MapRecord userMapRecord;
extern MapRecord *currentLevel;	
extern MapRecord* lastLevel;

inline bool SetMusicForMap(const char* mapname, const char* music, bool namehack = false)
{
	static const char* specials[] = { "intro", "briefing", "loading" };
	for (int i = 0; i < 3; i++)
	{
		if (!stricmp(mapname, specials[i]))
		{
			// todo: store this properly.
			return true;
		}
	}

	int index = -1; // = FindMap(mapname);

	// This is for the DEFS parser's MUSIC command which never bothered to check for the real map name.
	if (index < 0 && namehack)
	{
		int lev, ep;
		signed char b1, b2;

		int numMatches = sscanf(mapname, "%c%d%c%d", &b1, &ep, &b2, &lev);

		if (numMatches != 4 || toupper(b1) != 'E' || toupper(b2) != 'L')
			return false;

		index = -1; // = FindMapByIndex(ep, lev);

	}
	if (index >= 0)
	{
		mapList[index].music = music;
		return true;
	}
	return false;
}


inline void InitRREndMap()
{
	// RR defines its end map ad-hoc so give it a proper entry to reference (the last one in episode 2 because it needs to be in Ep. 2.)
	mapList[127].SetName("$TXT_CLOSEENCOUNTERS");
	mapList[127].SetFileName("endgame.map");
	mapList[127].levelNumber = 163;	// last one in Ep. 2.
}

enum
{
	RRENDSLOT = 127
};