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
	int parTime = -1;
	int designerTime = -1;
	FString fileName;
	FString labelName;
	FString name;
	FString music;
	int cdSongId = -1;
	int flags = 0;

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


inline void InitRREndMap()
{
	// RR defines its end map ad-hoc so give it a proper entry to reference (the last one in episode 2 because it needs to be in Ep. 2.)
	mapList[127].SetName("$TXT_CLOSEENCOUNTERS");
	mapList[127].SetFileName("endgame.map");
}

enum
{
	RRENDSLOT = 127
};