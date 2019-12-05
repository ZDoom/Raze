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
   module: FX_MAN.H

   author: James R. Dose
   date:   March 17, 1994

   Public header for FX_MAN.C

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifndef FX_MAN_H_
#define FX_MAN_H_

#include "drivers.h"
#include <inttypes.h>
#include "limits.h"
#include "multivoc.h"

enum FX_ERRORS
{
    FX_Warning = -2,
    FX_Error = -1,
    FX_Ok = 0,
    FX_InvalidCard,
    FX_MultiVocError,
};

enum FX_LOOP_HOW
{
    FX_ONESHOT = -1,
    FX_LOOP = 0,
};

#define FX_MUSIC_PRIORITY	INT_MAX

const char *FX_ErrorString(int ErrorNumber);
int FX_Init(int numvoices, int numchannels, int mixrate, void *initdata);
int FX_Shutdown(void);
int FX_GetDevice(void);



int FX_Play(char *ptr, uint32_t ptrlength, int loopstart, int loopend, int pitchoffset,
                      int vol, int left, int right, int priority, float volume, intptr_t callbackval);
int FX_Play3D(char *ptr, uint32_t ptrlength, int loophow, int pitchoffset, int angle,
                  int distance, int priority, float volume, intptr_t callbackval);
int FX_PlayRaw(char *ptr, uint32_t ptrlength, int rate, int pitchoffset, int vol,
    int left, int right, int priority, float volume, intptr_t callbackval);
int FX_PlayLoopedRaw(char *ptr, uint32_t ptrlength, char *loopstart, char *loopend, int rate,
    int pitchoffset, int vol, int left, int right, int priority, float volume, intptr_t callbackval);

int FX_StartDemandFeedPlayback(void (*function)(const char** ptr, uint32_t* length), int rate, int pitchoffset,
                    int vol, int left, int right, int priority, fix16_t volume, uint32_t callbackval);

int FX_SetPrintf(void(*function)(const char *, ...));

extern int FX_ErrorCode;
#define FX_SetErrorCode(status) FX_ErrorCode = (status);

static FORCE_INLINE int FX_CheckMVErr(int status)
{
    if (status != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Warning;
    }

    return status;
}

static FORCE_INLINE void FX_SetCallBack(void(*function)(intptr_t)) { MV_SetCallBack(function); }
static FORCE_INLINE void FX_SetVolume(int volume) { MV_SetVolume(volume); }
static FORCE_INLINE int FX_GetVolume(void) { return MV_GetVolume(); }
static FORCE_INLINE void FX_SetReverseStereo(int setting) { MV_SetReverseStereo(setting); }
static FORCE_INLINE int FX_GetReverseStereo(void) { return MV_GetReverseStereo(); }
static FORCE_INLINE void FX_SetReverb(int reverb) { MV_SetReverb(reverb); }
static FORCE_INLINE int FX_GetMaxReverbDelay(void) { return MV_GetMaxReverbDelay(); }
static FORCE_INLINE int FX_GetReverbDelay(void) { return MV_GetReverbDelay(); }
static FORCE_INLINE void FX_SetReverbDelay(int delay) { MV_SetReverbDelay(delay); }
static FORCE_INLINE int FX_VoiceAvailable(int priority) { return MV_VoiceAvailable(priority); }
static FORCE_INLINE int FX_PauseVoice(int handle, int pause) { return FX_CheckMVErr(MV_PauseVoice(handle, pause)); }
static FORCE_INLINE int FX_EndLooping(int handle) { return FX_CheckMVErr(MV_EndLooping(handle)); }
static FORCE_INLINE int FX_SetPan(int handle, int vol, int left, int right)
{
    return FX_CheckMVErr(MV_SetPan(handle, vol, left, right));
}
static FORCE_INLINE int FX_SetPitch(int handle, int pitchoffset) { return FX_CheckMVErr(MV_SetPitch(handle, pitchoffset)); }
static FORCE_INLINE int FX_SetFrequency(int handle, int frequency) { return FX_CheckMVErr(MV_SetFrequency(handle, frequency)); }
static FORCE_INLINE int32_t FX_GetFrequency(int32_t handle, int32_t *frequency) { return FX_CheckMVErr(MV_GetFrequency(handle, frequency)); }
static FORCE_INLINE int FX_Pan3D(int handle, int angle, int distance)
{
    return FX_CheckMVErr(MV_Pan3D(handle, angle, distance));
}
static FORCE_INLINE int FX_SoundActive(int handle) { return MV_VoicePlaying(handle); }
static FORCE_INLINE int FX_SoundValidAndActive(int handle) { return handle > 0 && MV_VoicePlaying(handle); }
static FORCE_INLINE int FX_SoundsPlaying(void) { return MV_VoicesPlaying(); }
static FORCE_INLINE int FX_StopSound(int handle) { return FX_CheckMVErr(MV_Kill(handle)); }
static FORCE_INLINE int FX_StopAllSounds(void) { return FX_CheckMVErr(MV_KillAllVoices()); }

#endif
