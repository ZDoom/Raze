/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
/**********************************************************************
   file:   MULTIVOC.H

   author: James R. Dose
   date:   December 20, 1993

   Public header for MULTIVOC.C

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifndef MULTIVOC_H_
#define MULTIVOC_H_

#include "compat.h"
#include "drivers.h"

typedef enum : char
{
    FMT_UNKNOWN,
    FMT_RAW,
    FMT_VOC,
    FMT_WAV,
	FMT_SNDFILE,
	FMT_ZMUSIC,
	// soon to be obsolete.
    FMT_VORBIS,
    FMT_FLAC,
    FMT_MAX
} wavefmt_t;

#define MV_MINVOICEHANDLE 1

extern int MV_ErrorCode;

enum MV_Errors
{
    MV_Error = -1,
    MV_Ok = 0,
    MV_NotInstalled,
    MV_DriverError,
    MV_NoVoices,
    MV_NoMem,
    MV_VoiceNotFound,
    MV_InvalidFile,
};

extern void (*MV_Printf)(const char *fmt, ...);
extern int MV_Locked;

static inline void MV_Lock(void)
{
    if (!MV_Locked++)
        SoundDriver_PCM_Lock();
}

static inline void MV_Unlock(void)
{
    if (!--MV_Locked)
        SoundDriver_PCM_Unlock();
    else if (MV_Locked < 0)
        MV_Printf("MV_Unlock(): lockdepth < 0!\n");
}

const char *MV_ErrorString(int ErrorNumber);

int  MV_VoicePlaying(int handle);
int  MV_KillAllVoices(void);
int  MV_Kill(int handle);
int  MV_VoicesPlaying(void);
int  MV_VoiceAvailable(int priority);
int  MV_SetPitch(int handle, int pitchoffset);
int  MV_SetFrequency(int handle, int frequency);
int32_t MV_GetFrequency(int32_t handle, int32_t* frequency);
int  MV_PauseVoice(int handle, int pause);
int  MV_EndLooping(int handle);
int  MV_SetPan(int handle, int vol, int left, int right);
int  MV_Pan3D(int handle, int angle, int distance);
void MV_SetReverb(int reverb);
int  MV_GetMaxReverbDelay(void);
int  MV_GetReverbDelay(void);
void MV_SetReverbDelay(int delay);

int MV_PlayVOC3D(char *ptr, uint32_t length, int loophow, int pitchoffset, int angle, int distance,
                 int priority, float volume, intptr_t callbackval);
int MV_PlayVOC(char *ptr, uint32_t length, int loopstart, int loopend, int pitchoffset, int vol,
               int left, int right, int priority, float volume, intptr_t callbackval);

int MV_StartDemandFeedPlayback(void (*function)(const char** ptr, uint32_t* length), int rate,
                int pitchoffset, int vol, int left, int right, int priority, fix16_t volume, uint32_t callbackval);

decltype(MV_PlayVOC3D) MV_PlayWAV3D;
decltype(MV_PlayVOC)   MV_PlayWAV;
decltype(MV_PlayVOC3D) MV_PlayVorbis3D;
decltype(MV_PlayVOC)   MV_PlayVorbis;
decltype(MV_PlayVOC3D) MV_PlayFLAC3D;
decltype(MV_PlayVOC)   MV_PlayFLAC;

int MV_PlayRAW(char *ptr, uint32_t length, int rate, char *loopstart, char *loopend, int pitchoffset, int vol,
               int left, int right, int priority, float volume, intptr_t callbackval);

int  MV_GetPosition(int handle, int *position);
int  MV_SetPosition(int handle, int position);
void MV_SetVolume(int volume);
int  MV_GetVolume(void);
void MV_SetCallBack(void (*function)(intptr_t));
void MV_SetReverseStereo(int setting);
int  MV_GetReverseStereo(void);
int  MV_Init(int soundcard, int MixRate, int Voices, int numchannels, void *initdata);
int  MV_Shutdown(void);
void MV_HookMusicRoutine(void (*callback)(void));
void MV_UnhookMusicRoutine(void);

struct MV_MusicRoutineBuffer
{
    char * buffer;
    int32_t size;
};
struct MV_MusicRoutineBuffer MV_GetMusicRoutineBuffer(void);

static inline void MV_SetPrintf(void (*function)(const char *, ...)) { if (function) MV_Printf = function; }

#endif
