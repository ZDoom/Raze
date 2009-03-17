#ifndef __sounds_mapster32_h__
#define __sounds_mapster32_h__

#include "build.h"

/// vvv sound structs from duke3d.h
typedef struct {
    int32_t voice;
    int32_t i;
} SOUNDOWNER;

typedef struct {
    int32_t  length, num, soundsiz;
    char *filename, *ptr, *filename1;
    SOUNDOWNER SoundOwner[4];
    int16_t ps,pe,vo;
    char pr,m;
    volatile char lock;
    char *definedname;  // new
} sound_t;

int32_t S_SoundStartup(void);
void S_SoundShutdown(void);
int32_t S_PlaySoundXYZ(int32_t, int32_t, const vec3_t*);
void S_PlaySound(int32_t);
int32_t A_PlaySound(uint32_t num, int32_t i);
void A_StopSound(int32_t num, int32_t i);
void S_StopSound(int32_t num);
void S_StopEnvSound(int32_t num,int32_t i);
void S_Pan3D(void);
int32_t A_CheckSoundPlaying(int32_t i, int32_t num);
int32_t S_CheckSoundPlaying(int32_t i, int32_t num);
void S_ClearSoundLocks(void);

#define MAXSOUNDS 2560

#endif
