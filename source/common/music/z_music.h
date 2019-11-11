#pragma once

// Totally minimalistic interface - should be all the game modules need.

void Mus_Init();
void Mus_Play(const char *fn, bool loop);
void Mus_Stop();
void Mus_SetPaused(bool on);
