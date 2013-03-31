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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

//****************************************************************************
//
// sounds.h
//
//****************************************************************************

#ifndef _sounds_public_
#define _sounds_public_

// KEEPINSYNC lunatic/con_lang.lua
#define MAXSOUNDS           4096
#define MAXSOUNDINSTANCES   8
#define LOUDESTVOLUME       150
#define MUSIC_ID            -65536

struct audioenumdev
{
	char *def;
	char **devs;
	struct audioenumdev *next;
};
struct audioenumdrv
{
	char *def;
	char **drvs;
	struct audioenumdev *devs;
};
int32_t EnumAudioDevs(struct audioenumdrv **wave, struct audioenumdev **midi, struct audioenumdev **cda);

typedef struct
{
    int16_t voice;
    int16_t ow;
    uint32_t sndist;
    uint32_t clock;
} SOUNDOWNER;


typedef struct
{
    int32_t  length, num, soundsiz; // 12b
    char *filename, *ptr, *filename1; // 12b/24b
    SOUNDOWNER SoundOwner[MAXSOUNDINSTANCES]; // 64b
    int16_t ps,pe,vo; // 6b
    char pr,m; // 2b
} sound_t;

extern volatile char g_soundlocks[MAXSOUNDS];
extern sound_t g_sounds[MAXSOUNDS];
extern int16_t g_skillSoundID;
extern int32_t g_numEnvSoundsPlaying,g_maxSoundPos;

int32_t A_CheckSoundPlaying(int32_t i,int32_t num);
int32_t A_PlaySound(uint32_t num,int32_t i);
void S_Callback(uint32_t num);
int32_t A_CheckAnySoundPlaying(int32_t i);
int32_t S_CheckSoundPlaying(int32_t i,int32_t num);
void S_Cleanup(void);
void S_ClearSoundLocks(void);
int32_t S_LoadSound(uint32_t num);
void S_MenuSound(void);
void S_MusicShutdown(void);
void S_MusicStartup(void);
void S_MusicVolume(int32_t volume);
void S_PauseMusic(int32_t onf);
int32_t S_PlayMusic(const char *fn,const int32_t sel);
int32_t S_PlaySound(int32_t num);
int32_t S_PlaySound3D(int32_t num,int32_t i,const vec3_t *pos);
void S_SoundShutdown(void);
void S_SoundStartup(void);
void S_StopEnvSound(int32_t num,int32_t i);
void S_StopMusic(void);
void S_Update(void);
void S_ChangeSoundPitch(int32_t num, int32_t i, int32_t pitchoffset);

static inline int32_t S_IsAmbientSFX(int32_t i)
{
    return (sprite[i].picnum==MUSICANDSFX && sprite[i].lotag < 999);
}

#endif
