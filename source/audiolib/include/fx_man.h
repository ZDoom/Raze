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
#include "inttypes.h"
#include "limits.h"
#include "multivoc.h"

#ifdef __cplusplus
extern "C" {
#endif

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

const char *FX_ErrorString(int32_t ErrorNumber);
int32_t FX_Init(int32_t numvoices, int32_t numchannels, unsigned mixrate, void *initdata);
int32_t FX_Shutdown(void);



int32_t FX_Play(char *ptr, uint32_t ptrlength, int32_t loopstart, int32_t loopend, int32_t pitchoffset,
                      int32_t vol, int32_t left, int32_t right, int32_t priority, float volume, uint32_t callbackval);
int32_t FX_Play3D(char *ptr, uint32_t ptrlength, int32_t loophow, int32_t pitchoffset, int32_t angle,
                  int32_t distance, int32_t priority, float volume, uint32_t callbackval);


int32_t FX_SetPrintf(void(*function)(const char *, ...));

extern int32_t FX_ErrorCode;
#define FX_SetErrorCode(status) FX_ErrorCode = (status);

static FORCE_INLINE int FX_CheckMVErr(int32_t status)
{
    if (status != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Warning;
    }

    return status;
}

static FORCE_INLINE void FX_SetCallBack(void(*function)(uint32_t)) { MV_SetCallBack(function); }
static FORCE_INLINE void FX_SetVolume(int32_t volume) { MV_SetVolume(volume); }
static FORCE_INLINE int32_t FX_GetVolume(void) { return MV_GetVolume(); }
static FORCE_INLINE void FX_SetReverseStereo(int32_t setting) { MV_SetReverseStereo(setting); }
static FORCE_INLINE int32_t FX_GetReverseStereo(void) { return MV_GetReverseStereo(); }
static FORCE_INLINE void FX_SetReverb(int32_t reverb) { MV_SetReverb(reverb); }
static FORCE_INLINE int32_t FX_GetMaxReverbDelay(void) { return MV_GetMaxReverbDelay(); }
static FORCE_INLINE int32_t FX_GetReverbDelay(void) { return MV_GetReverbDelay(); }
static FORCE_INLINE void FX_SetReverbDelay(int32_t delay) { MV_SetReverbDelay(delay); }
static FORCE_INLINE int32_t FX_VoiceAvailable(int32_t priority) { return MV_VoiceAvailable(priority); }
static FORCE_INLINE int32_t FX_PauseVoice(int32_t handle, int32_t pause) { return FX_CheckMVErr(MV_PauseVoice(handle, pause)); }
static FORCE_INLINE int32_t FX_GetPosition(int32_t handle, int32_t *position) { return FX_CheckMVErr(MV_GetPosition(handle, position)); }
static FORCE_INLINE int32_t FX_SetPosition(int32_t handle, int32_t position) { return FX_CheckMVErr(MV_SetPosition(handle, position)); }
static FORCE_INLINE int32_t FX_EndLooping(int32_t handle) { return FX_CheckMVErr(MV_EndLooping(handle)); }
static FORCE_INLINE int32_t FX_SetPan(int32_t handle, int32_t vol, int32_t left, int32_t right)
{
    return FX_CheckMVErr(MV_SetPan(handle, vol, left, right));
}
static FORCE_INLINE int32_t FX_SetPitch(int32_t handle, int32_t pitchoffset) { return FX_CheckMVErr(MV_SetPitch(handle, pitchoffset)); }
static FORCE_INLINE int32_t FX_SetFrequency(int32_t handle, int32_t frequency) { return FX_CheckMVErr(MV_SetFrequency(handle, frequency)); }
static FORCE_INLINE int32_t FX_Pan3D(int32_t handle, int32_t angle, int32_t distance)
{
    return FX_CheckMVErr(MV_Pan3D(handle, angle, distance));
}
static FORCE_INLINE int32_t FX_SoundActive(int32_t handle) { return MV_VoicePlaying(handle); }
static FORCE_INLINE int32_t FX_SoundsPlaying(void) { return MV_VoicesPlaying(); }
static FORCE_INLINE int32_t FX_StopSound(int32_t handle) { return FX_CheckMVErr(MV_Kill(handle)); }
static FORCE_INLINE int32_t FX_StopAllSounds(void) { return FX_CheckMVErr(MV_KillAllVoices()); }

#ifdef __cplusplus
}
#endif

#endif
