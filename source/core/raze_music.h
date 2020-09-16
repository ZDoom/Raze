#pragma once

#include "zstring.h"
#include "tarray.h"
#include "name.h"

typedef TMap<FName, FName> MusicAliasMap;
extern MusicAliasMap MusicAliases;

// Totally minimalistic interface - should be all the game modules need.
void Mus_InitMusic();
void Mus_UpdateMusic();
int Mus_Play(const char *mapname, const char *fn, bool loop);
void Mus_Stop();
bool Mus_IsPlaying();
void Mus_SetPaused(bool on);
void Mus_ResumeSaved();
FString G_SetupFilenameBasedMusic(const char* fileName, const char *defaultfn);
class FSerializer;
void Mus_Serialize(FSerializer& arc);
int LookupMusic(const char* fn, bool onlyextended = false);
