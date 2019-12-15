//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

//****************************************************************************
//
// sounds.h
//
//****************************************************************************

#ifndef sounds_public_h_
#define sounds_public_h_

#include "sounds_common.h"
#include "z_music.h"

BEGIN_DUKE_NS

// KEEPINSYNC lunatic/con_lang.lua
#define MAXSOUNDS           4096
#define LOUDESTVOLUME       111

typedef struct
{
    int pitchStart, pitchEnd, volAdjust;
    int priority, flags;
} sound_t;

int A_CheckSoundPlaying(int spriteNum,int soundNum);
int A_PlaySound(int soundNum, int spriteNum, bool looped = false);
void S_Callback(intptr_t num);
int A_CheckAnySoundPlaying(int spriteNum);
int S_CheckSoundPlaying(int soundNum);
inline void S_ClearSoundLocks(void) {}
void cacheAllSounds(void);
void S_MenuSound(void);
void S_PauseMusic(bool paused);
void S_PauseSounds(bool paused);
void S_PlayLevelMusicOrNothing(unsigned int);
int S_TryPlaySpecialMusic(unsigned int);
void S_PlaySpecialMusicOrNothing(unsigned int);
void S_ContinueLevelMusic(void);
int S_PlaySound(int num, bool looped = false);
int S_PlaySound3D(int num, int spriteNum, const vec3_t *pos, bool looped = false);
void S_StopEnvSound(int sndNum,int sprNum);
void S_StopAllSounds(void);
void S_Update(void);
void S_ChangeSoundPitch(int soundNum, int spriteNum, int pitchoffset);
int S_GetUserFlags(int sndnum);
int S_DefineSound(unsigned index, const char* filename, int ps, int pe, int pr, int m, int vo, float vol);
void S_InitSound();

static inline bool S_IsAmbientSFX(int spriteNum)
{
    return (sprite[spriteNum].picnum == MUSICANDSFX && sprite[spriteNum].lotag < 999);
}

END_DUKE_NS

#endif
