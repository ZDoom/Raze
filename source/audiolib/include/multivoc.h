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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    FMT_UNKNOWN,
    FMT_RAW,
    FMT_VOC,
    FMT_WAV,
    FMT_VORBIS,
    FMT_FLAC,
    FMT_XA,
    FMT_XMP,
    FMT_MAX
} wavefmt_t;

#define MV_MINVOICEHANDLE 1

extern int32_t MV_ErrorCode;

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
const char *MV_ErrorString(int32_t ErrorNumber);
int32_t MV_VoicePlaying(int32_t handle);
int32_t MV_KillAllVoices(void);
int32_t MV_Kill(int32_t handle);
int32_t MV_VoicesPlaying(void);
int32_t MV_VoiceAvailable(int32_t priority);
int32_t MV_SetPitch(int32_t handle, int32_t pitchoffset);
int32_t MV_SetFrequency(int32_t handle, int32_t frequency);
int32_t MV_PauseVoice(int32_t handle, int32_t pause);
int32_t MV_EndLooping(int32_t handle);
int32_t MV_SetPan(int32_t handle, int32_t vol, int32_t left, int32_t right);
int32_t MV_Pan3D(int32_t handle, int32_t angle, int32_t distance);
void MV_SetReverb(int32_t reverb);
int32_t MV_GetMaxReverbDelay(void);
int32_t MV_GetReverbDelay(void);
void MV_SetReverbDelay(int32_t delay);
// int32_t   MV_SetMixMode( int32_t numchannels, int32_t samplebits );
// int32_t   MV_StartPlayback( void );
// void  MV_StopPlayback( void );
int32_t MV_PlayWAV3D(char *ptr, uint32_t length, int32_t loophow, int32_t pitchoffset, int32_t angle, int32_t distance,
                     int32_t priority, uint32_t callbackval);
int32_t MV_PlayWAV(char *ptr, uint32_t length, int32_t loopstart, int32_t loopend, int32_t pitchoffset, int32_t vol,
                   int32_t left, int32_t right, int32_t priority, uint32_t callbackval);
int32_t MV_PlayVOC3D(char *ptr, uint32_t length, int32_t loophow, int32_t pitchoffset, int32_t angle, int32_t distance,
                     int32_t priority, uint32_t callbackval);
int32_t MV_PlayVOC(char *ptr, uint32_t length, int32_t loopstart, int32_t loopend, int32_t pitchoffset, int32_t vol,
                   int32_t left, int32_t right, int32_t priority, uint32_t callbackval);
int32_t MV_PlayVorbis3D(char *ptr, uint32_t length, int32_t loophow, int32_t pitchoffset, int32_t angle,
                        int32_t distance, int32_t priority, uint32_t callbackval);
int32_t MV_PlayVorbis(char *ptr, uint32_t length, int32_t loopstart, int32_t loopend, int32_t pitchoffset, int32_t vol,
                      int32_t left, int32_t right, int32_t priority, uint32_t callbackval);
int32_t MV_PlayFLAC3D(char *ptr, uint32_t length, int32_t loophow, int32_t pitchoffset, int32_t angle, int32_t distance,
                      int32_t priority, uint32_t callbackval);
int32_t MV_PlayFLAC(char *ptr, uint32_t length, int32_t loopstart, int32_t loopend, int32_t pitchoffset, int32_t vol,
                    int32_t left, int32_t right, int32_t priority, uint32_t callbackval);
int32_t MV_PlayXA3D(char *ptr, uint32_t length, int32_t loophow, int32_t pitchoffset, int32_t angle, int32_t distance,
                    int32_t priority, uint32_t callbackval);
int32_t MV_PlayXA(char *ptr, uint32_t length, int32_t loopstart, int32_t loopend, int32_t pitchoffset, int32_t vol,
                  int32_t left, int32_t right, int32_t priority, uint32_t callbackval);
int32_t MV_PlayXMP3D(char *ptr, uint32_t length, int32_t loophow, int32_t pitchoffset, int32_t angle, int32_t distance,
                    int32_t priority, uint32_t callbackval);
int32_t MV_PlayXMP(char *ptr, uint32_t length, int32_t loopstart, int32_t loopend, int32_t pitchoffset, int32_t vol,
                  int32_t left, int32_t right, int32_t priority, uint32_t callbackval);

int MV_IdentifyXMP(char const *ptr, uint32_t length);

int32_t MV_GetPosition(int32_t handle, int32_t *position);
int32_t MV_SetPosition(int32_t handle, int32_t position);

// void  MV_CreateVolumeTable( int32_t index, int32_t volume, int32_t MaxVolume );
void MV_SetVolume(int32_t volume);
int32_t MV_GetVolume(void);
void MV_SetCallBack(void (*function)(uint32_t));
void MV_SetReverseStereo(int32_t setting);
int32_t MV_GetReverseStereo(void);
int32_t MV_Init(int32_t soundcard, int32_t MixRate, int32_t Voices, int32_t numchannels,
                void *initdata);
int32_t MV_Shutdown(void);
void MV_SetPrintf(void (*function)(const char *fmt, ...));

#ifdef __cplusplus
}
#endif

#endif
