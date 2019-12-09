#pragma once

#include "gstrings.h"

// Localization capable replacement of the game specific solutions.

inline void MakeStringLocalizable(FString &quote)
{
	// Only prepend a quote if the string is localizable.
	if (quote.Len() > 0 && quote[0] != '$' && GStrings[quote]) quote.Insert(0, "$");
}

struct MapRecord
{
	int parTime;
	int designerTime;
	FString fileName;
	FString name;
	FString music;
	int cdSongId;

	// The rest is only used by Blood
	int nextLevel;
	int nextSecret;
	int messageStart;	// messages are stored in the quote array to reduce clutter.
	FString author;
	// bool fog, weather;	// Blood defines these but they aren't used.
	
	const char *DisplayName()
	{
		return GStrings.localize(name);
	}
	void SetName(const char *n)
	{
		name = n;
		MakeStringLocalizable(name);
	}
};

extern MapRecord mapList[512];
extern MapRecord *currentLevel;	