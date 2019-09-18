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

#ifndef sounds_mapster32_h_
#define sounds_mapster32_h_

#include "build.h"
#include "sounds_common.h"

#pragma pack(push,1)

/// vvv sound structs from duke3d.h
typedef struct {
    int32_t voice;
    int32_t ow;
} SOUNDOWNER;

typedef struct {
    char *filename, *ptr;
    int32_t  length, num, soundsiz;
    SOUNDOWNER SoundOwner[4];
    int16_t ps,pe,vo;
    char pr,m;
    char lock;
    char *definedname;  // new
} sound_t;

#define MAXSOUNDS 4096
extern sound_t g_sounds[MAXSOUNDS];

extern int32_t g_numEnvSoundsPlaying;
extern int32_t NumVoices;

int32_t S_SoundStartup(void);
void S_SoundShutdown(void);
int32_t S_PlaySound3D(int32_t, int32_t, const vec3_t*);
void S_PlaySound(int32_t);
int32_t A_PlaySound(uint32_t num, int32_t i);
void S_StopSound(int32_t num);
void S_StopEnvSound(int32_t num,int32_t i);
void S_StopAllSounds(void);
void S_Update(void);
int32_t A_CheckSoundPlaying(int32_t i, int32_t num);
int32_t S_CheckSoundPlaying(int32_t i, int32_t num);
void S_ClearSoundLocks(void);
int32_t S_SoundsPlaying(int32_t i);
int32_t S_InvalidSound(int32_t num);
int32_t S_SoundFlags(int32_t num);

#pragma pack(pop)

#endif
