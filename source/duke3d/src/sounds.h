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

#ifdef __cplusplus
extern "C" {
#endif

// KEEPINSYNC lunatic/con_lang.lua
#define MAXSOUNDS           4096
#define MAXSOUNDINSTANCES   8
#define LOUDESTVOLUME       111
#define MUSIC_ID            -65536

typedef struct
{
    int16_t  owner;
    int16_t  id;
    uint16_t dist;
    uint16_t clock;
} assvoice_t;

typedef struct
{
    char *    ptr, *filename;                // 8b/16b
    int32_t   length, num, siz;              // 12b
    float     volume;                        // 4b
    assvoice_t voices[MAXSOUNDINSTANCES];  // 64b
    int16_t   ps, pe, vo;                    // 6b
    char      pr, m;                         // 2b
} sound_t;

extern char g_soundlocks[MAXSOUNDS];
extern sound_t g_sounds[MAXSOUNDS];
extern int32_t g_numEnvSoundsPlaying,g_highestSoundIdx;

bool A_CheckSoundPlaying(int spriteNum,int soundNum);
int A_PlaySound(int soundNum, int spriteNum);
void S_Callback(uint32_t num);
bool A_CheckAnySoundPlaying(int spriteNum);
bool S_CheckSoundPlaying(int soundNum);
void S_Cleanup(void);
void S_ClearSoundLocks(void);
int32_t S_LoadSound(uint32_t num);
void cacheAllSounds(void);
void S_MenuSound(void);
void S_MusicShutdown(void);
void S_MusicStartup(void);
void S_MusicVolume(int32_t volume);
void S_RestartMusic(void);
void S_PauseMusic(bool paused);
void S_PauseSounds(bool paused);
bool S_TryPlayLevelMusic(unsigned int m);
void S_PlayLevelMusicOrNothing(unsigned int);
int S_TryPlaySpecialMusic(unsigned int);
void S_PlaySpecialMusicOrNothing(unsigned int);
void S_ContinueLevelMusic(void);
int S_PlaySound(int num);
int S_PlaySound3D(int num, int spriteNum, const vec3_t *pos);
void S_SoundShutdown(void);
void S_SoundStartup(void);
void S_StopEnvSound(int32_t num,int32_t i);
void S_StopAllSounds(void);
void S_StopMusic(void);
void S_Update(void);
void S_ChangeSoundPitch(int soundNum, int spriteNum, int pitchoffset);
int32_t S_GetMusicPosition(void);
void S_SetMusicPosition(int32_t position);

static inline bool S_IsAmbientSFX(int spriteNum)
{
    return (sprite[spriteNum].picnum == MUSICANDSFX && sprite[spriteNum].lotag < 999);
}

#ifdef __cplusplus
}
#endif

#endif
