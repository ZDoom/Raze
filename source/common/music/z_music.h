#pragma once

// Totally minimalistic interface - should be all the game modules need.

void Mus_Play(const char *fn, bool loop);
void Mus_Stop();
void Mus_SetVolume(float vol);
void Mus_SetPaused(bool on);
