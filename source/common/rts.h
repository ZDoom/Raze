#pragma once

void RTS_Init(const char *filename);
bool RTS_IsInitialized();
int RTS_SoundLength(int lump);
void *RTS_GetSound(int lump);
int RTS_GetSoundID(int lump);
