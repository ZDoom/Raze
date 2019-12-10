#pragma once

#include "gstrings.h"
#include "cmdlib.h"

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
	int parTime;
	int designerTime;
	FString fileName;
	FString labelName;
	FString name;
	FString music;
	int cdSongId;

	// The rest is only used by Blood
	int nextLevel;
	int nextSecret;
	int messageStart;	// messages are stored in the quote array to reduce clutter.
	FString author;
	// bool fog, weather;	// Blood defines these but they aren't used.
	int flags;
	
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
		labelName = ExtractFileBase(n);
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
	mapList[127].SetFileName("endmap.map");
}

enum
{
	RRENDSLOT = 127
};