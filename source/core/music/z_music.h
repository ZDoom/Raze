#pragma once

#include "zstring.h"

// Totally minimalistic interface - should be all the game modules need.

void Mus_Init();
int Mus_Play(const char *mapname, const char *fn, bool loop);
void Mus_Stop();
bool Mus_IsPlaying();
void Mus_Fade(double seconds);
void Mus_SetPaused(bool on);
void Mus_ResumeSaved();
FString G_SetupFilenameBasedMusic(const char* fileName, const char *defaultfn);
class FSerializer;
void Mus_Serialize(FSerializer& arc);
