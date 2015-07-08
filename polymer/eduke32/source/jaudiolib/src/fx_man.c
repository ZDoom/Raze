/*
Copyright (C) 1994-1995 Apogee Software, Ltd.
Copyright (C) 2015 EDuke32 developers
Copyright (C) 2015 Voidpoint, LLC

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
   module: FX_MAN.C

   author: James R. Dose
   date:   March 17, 1994

   Device independant sound effect routines.

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include "compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drivers.h"
#include "multivoc.h"
#include "fx_man.h"

int32_t FX_ErrorCode = FX_Ok;
int32_t FX_Installed = FALSE;

#define FX_SetErrorCode(status) FX_ErrorCode = (status);

const char *FX_ErrorString(int32_t ErrorNumber)
{
    const char *ErrorString;

    switch (ErrorNumber)
    {
        case FX_Warning:
        case FX_Error: ErrorString = FX_ErrorString(FX_ErrorCode); break;

        case FX_Ok: ErrorString = "Fx ok."; break;

        case FX_InvalidCard: ErrorString = "Invalid Sound Fx device."; break;

        case FX_MultiVocError: ErrorString = MV_ErrorString(MV_Error); break;

        default: ErrorString = "Unknown Fx error code."; break;
    }

    return ErrorString;
}

static inline int32_t FX_CheckMVErr(int32_t status)
{
    if (status != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Warning;
    }

    return status;
}

int32_t FX_Init(int32_t SoundCard, int32_t numvoices, int32_t numchannels, unsigned mixrate, void *initdata)
{
    if (FX_Installed)
        FX_Shutdown();

    if (SoundCard == ASS_AutoDetect)
    {
#if defined HAVE_DS
        SoundCard = ASS_DirectSound;
#elif defined HAVE_SDL
        SoundCard = ASS_SDL;
#else
#warning No sound driver selected!
        SoundCard = ASS_NoSound;
#endif
    }

    if (SoundCard < 0 || SoundCard >= ASS_NumSoundCards)
    {
        FX_SetErrorCode(FX_InvalidCard);
        return FX_Error;
    }

    if (SoundDriver_IsSupported(SoundCard) == 0)
    {
        // unsupported cards fall back to no sound
        SoundCard = ASS_NoSound;
    }

    int status = FX_Ok;

    if (MV_Init(SoundCard, mixrate, numvoices, numchannels, initdata) != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Error;
    }

    if (status == FX_Ok)
        FX_Installed = TRUE;

    return status;
}

int32_t FX_Shutdown(void)
{
    if (!FX_Installed)
        return FX_Ok;

    int status = MV_Shutdown();

    if (status != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Error;
    }

    FX_Installed = FALSE;

    return status;
}

void FX_SetCallBack(void (*function)(uint32_t)) { MV_SetCallBack(function); }

void FX_SetVolume(int32_t volume) { MV_SetVolume(volume); }

int32_t FX_GetVolume(void) { return MV_GetVolume(); }

void FX_SetReverseStereo(int32_t setting) { MV_SetReverseStereo(setting); }

int32_t FX_GetReverseStereo(void) { return MV_GetReverseStereo(); }

void FX_SetReverb(int32_t reverb) { MV_SetReverb(reverb); }

int32_t FX_GetMaxReverbDelay(void) { return MV_GetMaxReverbDelay(); }

int32_t FX_GetReverbDelay(void) { return MV_GetReverbDelay(); }

void FX_SetReverbDelay(int32_t delay) { MV_SetReverbDelay(delay); }

int32_t FX_VoiceAvailable(int32_t priority) { return MV_VoiceAvailable(priority); }

int32_t FX_PauseVoice(int32_t handle, int32_t pause) { return FX_CheckMVErr(MV_PauseVoice(handle, pause)); }

int32_t FX_GetPosition(int32_t handle, int32_t *position) { return FX_CheckMVErr(MV_GetPosition(handle, position)); }

int32_t FX_SetPosition(int32_t handle, int32_t position) { return FX_CheckMVErr(MV_SetPosition(handle, position)); }

int32_t FX_EndLooping(int32_t handle) { return FX_CheckMVErr(MV_EndLooping(handle)); }

int32_t FX_SetPan(int32_t handle, int32_t vol, int32_t left, int32_t right)
{
    return FX_CheckMVErr(MV_SetPan(handle, vol, left, right));
}

int32_t FX_SetPitch(int32_t handle, int32_t pitchoffset) { return FX_CheckMVErr(MV_SetPitch(handle, pitchoffset)); }

int32_t FX_SetFrequency(int32_t handle, int32_t frequency) { return FX_CheckMVErr(MV_SetFrequency(handle, frequency)); }

int32_t FX_Pan3D(int32_t handle, int32_t angle, int32_t distance)
{
    return FX_CheckMVErr(MV_Pan3D(handle, angle, distance));
}

int32_t FX_SoundActive(int32_t handle) { return MV_VoicePlaying(handle); }

int32_t FX_SoundsPlaying(void) { return MV_VoicesPlaying(); }

int32_t FX_StopSound(int32_t handle) { return FX_CheckMVErr(MV_Kill(handle)); }

int32_t FX_StopAllSounds(void) { return FX_CheckMVErr(MV_KillAllVoices()); }

static wavefmt_t FX_AutoDetectFormat(const char *ptr, uint32_t length)
{
    wavefmt_t fmt = FMT_UNKNOWN;

    if (length < 12)
        return fmt;

    switch (LITTLE32(*(int32_t *)ptr))
    {
        case 'C' + ('r' << 8) + ('e' << 16) + ('a' << 24):  // Crea
            fmt = FMT_VOC;
            break;
        case 'O' + ('g' << 8) + ('g' << 16) + ('S' << 24):  // OggS
            fmt = FMT_VORBIS;
            break;
        case 'R' + ('I' << 8) + ('F' << 16) + ('F' << 24):  // RIFF
            switch (LITTLE32(*(int32_t *)(ptr + 8)))
            {
                case 'C' + ('D' << 8) + ('X' << 16) + ('A' << 24):  // CDXA
                    fmt = FMT_XA;
                    break;
                default: fmt = FMT_WAV; break;
            }
            break;
        case 'f' + ('L' << 8) + ('a' << 16) + ('C' << 24):  // fLaC
            fmt = FMT_FLAC;
            break;
        default:
            switch (LITTLE32(*(int32_t *)(ptr + 8)))
            {
                case 'W' + ('A' << 8) + ('V' << 16) + ('E' << 24):  // WAVE
                    fmt = FMT_WAV;
                    break;
            }
            break;
    }

    return fmt;
}

int32_t FX_PlayAuto(char *ptr, uint32_t length, int32_t pitchoffset, int32_t vol,
                int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{
    return FX_PlayLoopedAuto(ptr, length, -1, -1, pitchoffset, vol, left, right, priority, callbackval);
}

int32_t FX_PlayLoopedAuto(char *ptr, uint32_t length, int32_t loopstart, int32_t loopend, int32_t pitchoffset,
                          int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{
    int32_t handle = -1;

    EDUKE32_STATIC_ASSERT(FMT_MAX == 7);

    static int32_t(*const func[FMT_MAX])(char *, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, uint32_t) =
    { NULL, NULL, MV_PlayVOC, MV_PlayWAV, MV_PlayVorbis, MV_PlayFLAC, MV_PlayXA };

    wavefmt_t const fmt = FX_AutoDetectFormat(ptr, length);

    if (func[fmt])
        handle = func[fmt](ptr, length, loopstart, loopend, pitchoffset, vol, left, right, priority, callbackval);

    if (handle <= MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return handle;
}

int32_t FX_PlayAuto3D(char *ptr, uint32_t length, int32_t loophow, int32_t pitchoffset, int32_t angle, int32_t distance,
                      int32_t priority, uint32_t callbackval)
{
    int32_t handle = -1;

    EDUKE32_STATIC_ASSERT(FMT_MAX == 7);

    static int32_t (*const func[FMT_MAX])(char *, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t, uint32_t) =
    { NULL, NULL, MV_PlayVOC3D, MV_PlayWAV3D, MV_PlayVorbis3D, MV_PlayFLAC3D, MV_PlayXA3D };

    wavefmt_t const fmt = FX_AutoDetectFormat(ptr, length);

    if (func[fmt])
        handle = func[fmt](ptr, length, loophow, pitchoffset, angle, distance, priority, callbackval);

    if (handle <= MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return handle;
}

int32_t FX_SetVoiceCallback(int32_t handle, uint32_t callbackval)
{
    int32_t status = MV_SetVoiceCallback(handle, callbackval);

    if (status != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        return FX_Warning;
    }

    return FX_Ok;
}

int32_t FX_SetPrintf(void (*function)(const char *, ...))
{
    MV_SetPrintf(function);

    return FX_Ok;
}
