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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/**********************************************************************
   module: FX_MAN.C

   author: James R. Dose
   date:   March 17, 1994

   Device independant sound effect routines.

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sndcards.h"
#include "drivers.h"
#include "multivoc.h"
#include "fx_man.h"

int32_t FX_ErrorCode = FX_Ok;
int32_t FX_Installed = FALSE;

#define FX_SetErrorCode( status ) \
   FX_ErrorCode = ( status );


/*---------------------------------------------------------------------
   Function: FX_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/

const char *FX_ErrorString
(
    int32_t ErrorNumber
)

{
    const char *ErrorString;

    switch (ErrorNumber)
    {
    case FX_Warning :
    case FX_Error :
        ErrorString = FX_ErrorString(FX_ErrorCode);
        break;

    case FX_Ok :
        ErrorString = "Fx ok.";
        break;

    case FX_SoundCardError :
        ErrorString = SoundDriver_ErrorString(SoundDriver_GetError());
        break;

    case FX_InvalidCard :
        ErrorString = "Invalid Sound Fx device.";
        break;

    case FX_MultiVocError :
        ErrorString = MV_ErrorString(MV_Error);
        break;

    default :
        ErrorString = "Unknown Fx error code.";
        break;
    }

    return ErrorString;
}


/*---------------------------------------------------------------------
   Function: FX_Init

   Selects which sound device to use.
---------------------------------------------------------------------*/

int32_t FX_Init
(
    int32_t SoundCard,
    int32_t numvoices,
    int32_t numchannels,
    int32_t samplebits,
    unsigned mixrate,
    void * initdata
)

{
    int32_t status;
    int32_t devicestatus;

    if (FX_Installed)
    {
        FX_Shutdown();
    }

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
        status = FX_Error;
        return status;
    }

    if (SoundDriver_IsSupported(SoundCard) == 0)
    {
        // unsupported cards fall back to no sound
        SoundCard = ASS_NoSound;
    }

    status = FX_Ok;
    devicestatus = MV_Init(SoundCard, mixrate, numvoices, numchannels, samplebits, initdata);
    if (devicestatus != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Error;
    }

    if (status == FX_Ok)
    {
        FX_Installed = TRUE;
    }

    return status;
}


/*---------------------------------------------------------------------
   Function: FX_Shutdown

   Terminates use of sound device.
---------------------------------------------------------------------*/

int32_t FX_Shutdown
(
    void
)

{
    int32_t status;

    if (!FX_Installed)
    {
        return FX_Ok;
    }

    status = MV_Shutdown();
    if (status != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Error;
    }

    FX_Installed = FALSE;

    return status;
}


/*---------------------------------------------------------------------
   Function: FX_SetCallback

   Sets the function to call when a voice is done.
---------------------------------------------------------------------*/

int32_t FX_SetCallBack
(
    void (*function)(uint32_t)
)

{
    int32_t status;

    status = FX_Ok;

    MV_SetCallBack(function);

    return status;
}


/*---------------------------------------------------------------------
   Function: FX_SetVolume

   Sets the volume of the current sound device.
---------------------------------------------------------------------*/

void FX_SetVolume
(
    int32_t volume
)

{
    MV_SetVolume(volume);
}


/*---------------------------------------------------------------------
   Function: FX_GetVolume

   Returns the volume of the current sound device.
---------------------------------------------------------------------*/

int32_t FX_GetVolume
(
    void
)

{
    int32_t volume;

    volume = MV_GetVolume();

    return volume;
}


/*---------------------------------------------------------------------
   Function: FX_SetReverseStereo

   Set the orientation of the left and right channels.
---------------------------------------------------------------------*/

void FX_SetReverseStereo
(
    int32_t setting
)

{
    MV_SetReverseStereo(setting);
}


/*---------------------------------------------------------------------
   Function: FX_GetReverseStereo

   Returns the orientation of the left and right channels.
---------------------------------------------------------------------*/

int32_t FX_GetReverseStereo
(
    void
)

{
    return MV_GetReverseStereo();
}


/*---------------------------------------------------------------------
   Function: FX_SetReverb

   Sets the reverb level.
---------------------------------------------------------------------*/

void FX_SetReverb
(
    int32_t reverb
)

{
    MV_SetReverb(reverb);
}


/*---------------------------------------------------------------------
   Function: FX_SetFastReverb

   Sets the reverb level.
---------------------------------------------------------------------*/

void FX_SetFastReverb
(
    int32_t reverb
)

{
    MV_SetFastReverb(reverb);
}


/*---------------------------------------------------------------------
   Function: FX_GetMaxReverbDelay

   Returns the maximum delay time for reverb.
---------------------------------------------------------------------*/

int32_t FX_GetMaxReverbDelay
(
    void
)

{
    return MV_GetMaxReverbDelay();
}


/*---------------------------------------------------------------------
   Function: FX_GetReverbDelay

   Returns the current delay time for reverb.
---------------------------------------------------------------------*/

int32_t FX_GetReverbDelay
(
    void
)

{
    return MV_GetReverbDelay();
}


/*---------------------------------------------------------------------
   Function: FX_SetReverbDelay

   Sets the delay level of reverb to add to mix.
---------------------------------------------------------------------*/

void FX_SetReverbDelay
(
    int32_t delay
)

{
    MV_SetReverbDelay(delay);
}


/*---------------------------------------------------------------------
   Function: FX_VoiceAvailable

   Checks if a voice can be play at the specified priority.
---------------------------------------------------------------------*/

int32_t FX_VoiceAvailable
(
    int32_t priority
)

{
    return MV_VoiceAvailable(priority);
}

/*---------------------------------------------------------------------
Function: FX_PauseVoice

Stops the voice associated with the specified handle from looping
without stoping the sound.
---------------------------------------------------------------------*/

int32_t FX_PauseVoice
(
    int32_t handle,
    int32_t pause
)

{
    int32_t status;

    status = MV_PauseVoice(handle, pause);
    if (status == MV_Error)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Warning;
    }

    return status;
}


/*---------------------------------------------------------------------
   Function: FX_EndLooping

   Stops the voice associated with the specified handle from looping
   without stoping the sound.
---------------------------------------------------------------------*/

int32_t FX_EndLooping
(
    int32_t handle
)

{
    int32_t status;

    status = MV_EndLooping(handle);
    if (status == MV_Error)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Warning;
    }

    return status;
}

/*---------------------------------------------------------------------
   Function: FX_SetPan

   Sets the stereo and mono volume level of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int32_t FX_SetPan
(
    int32_t handle,
    int32_t vol,
    int32_t left,
    int32_t right
)

{
    int32_t status;

    status = MV_SetPan(handle, vol, left, right);
    if (status == MV_Error)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Warning;
    }

    return status;
}


/*---------------------------------------------------------------------
   Function: FX_SetPitch

   Sets the pitch of the voice associated with the specified handle.
---------------------------------------------------------------------*/

int32_t FX_SetPitch
(
    int32_t handle,
    int32_t pitchoffset
)

{
    int32_t status;

    status = MV_SetPitch(handle, pitchoffset);
    if (status == MV_Error)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Warning;
    }

    return status;
}


/*---------------------------------------------------------------------
   Function: FX_SetFrequency

   Sets the frequency of the voice associated with the specified handle.
---------------------------------------------------------------------*/

int32_t FX_SetFrequency
(
    int32_t handle,
    int32_t frequency
)

{
    int32_t status;

    status = MV_SetFrequency(handle, frequency);
    if (status == MV_Error)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Warning;
    }

    return status;
}


/*---------------------------------------------------------------------
   Function: FX_PlayVOC

   Begin playback of sound data with the given volume and priority.
---------------------------------------------------------------------*/

int32_t FX_PlayVOC
(
    char *ptr,
    uint32_t ptrlength,
    int32_t pitchoffset,
    int32_t vol,
    int32_t left,
    int32_t right,
    int32_t priority,
    uint32_t callbackval
)

{
    return FX_PlayLoopedVOC(ptr, ptrlength, -1, -1, pitchoffset, vol, left, right, priority, callbackval);
}


/*---------------------------------------------------------------------
   Function: FX_PlayLoopedVOC

   Begin playback of sound data with the given volume and priority.
---------------------------------------------------------------------*/

int32_t FX_PlayLoopedVOC
(
    char *ptr,
    uint32_t ptrlength,
    int32_t loopstart,
    int32_t loopend,
    int32_t pitchoffset,
    int32_t vol,
    int32_t left,
    int32_t right,
    int32_t priority,
    uint32_t callbackval
)

{
    int32_t handle;

    handle = MV_PlayVOC(ptr, ptrlength, loopstart, loopend, pitchoffset,
                              vol, left, right, priority, callbackval);
    if (handle < MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return handle;
}


/*---------------------------------------------------------------------
   Function: FX_PlayWAV

   Begin playback of sound data with the given volume and priority.
---------------------------------------------------------------------*/

int32_t FX_PlayWAV
(
    char *ptr,
    uint32_t ptrlength,
    int32_t pitchoffset,
    int32_t vol,
    int32_t left,
    int32_t right,
    int32_t priority,
    uint32_t callbackval
)

{
    return FX_PlayLoopedWAV(ptr, ptrlength, -1, -1, pitchoffset, vol, left, right, priority, callbackval);
}


/*---------------------------------------------------------------------
   Function: FX_PlayWAV

   Begin playback of sound data with the given volume and priority.
---------------------------------------------------------------------*/

int32_t FX_PlayLoopedWAV
(
    char *ptr,
    uint32_t ptrlength,
    int32_t loopstart,
    int32_t loopend,
    int32_t pitchoffset,
    int32_t vol,
    int32_t left,
    int32_t right,
    int32_t priority,
    uint32_t callbackval
)

{
    int32_t handle;

    handle = MV_PlayWAV(ptr, ptrlength, loopstart, loopend,
                              pitchoffset, vol, left, right, priority, callbackval);
    if (handle < MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return handle;
}


/*---------------------------------------------------------------------
   Function: FX_PlayVOC3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int32_t FX_PlayVOC3D
(
    char *ptr,
    uint32_t ptrlength,
    int32_t pitchoffset,
    int32_t angle,
    int32_t distance,
    int32_t priority,
    uint32_t callbackval
)

{
    int32_t handle;

    handle = MV_PlayVOC3D(ptr, ptrlength, FX_ONESHOT, pitchoffset, angle, distance,
                          priority, callbackval);
    if (handle < MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return handle;
}


/*---------------------------------------------------------------------
   Function: FX_PlayWAV3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int32_t FX_PlayWAV3D
(
    char *ptr,
    uint32_t ptrlength,
    int32_t pitchoffset,
    int32_t angle,
    int32_t distance,
    int32_t priority,
    uint32_t callbackval
)

{
    int32_t handle;

    handle = MV_PlayWAV3D(ptr, ptrlength, FX_ONESHOT, pitchoffset, angle, distance,
                          priority, callbackval);
    if (handle < MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return handle;
}


/*---------------------------------------------------------------------
   Function: FX_PlayRaw

   Begin playback of raw sound data with the given volume and priority.
---------------------------------------------------------------------*/

int32_t FX_PlayRaw
(
    char *ptr,
    uint32_t length,
    unsigned rate,
    int32_t pitchoffset,
    int32_t vol,
    int32_t left,
    int32_t right,
    int32_t priority,
    uint32_t callbackval
)

{
    return FX_PlayLoopedRaw(ptr, length, NULL, NULL, rate, pitchoffset, vol, left, right, priority, callbackval);
}


/*---------------------------------------------------------------------
   Function: FX_PlayLoopedRaw

   Begin playback of raw sound data with the given volume and priority.
---------------------------------------------------------------------*/

int32_t FX_PlayLoopedRaw
(
    char *ptr,
    uint32_t length,
    char *loopstart,
    char *loopend,
    unsigned rate,
    int32_t pitchoffset,
    int32_t vol,
    int32_t left,
    int32_t right,
    int32_t priority,
    uint32_t callbackval
)

{
    int32_t handle;

    handle = MV_PlayRaw(ptr, length, loopstart, loopend,
                              rate, pitchoffset, vol, left, right, priority, callbackval);
    if (handle < MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return handle;
}


/*---------------------------------------------------------------------
   Function: FX_Pan3D

   Set the angle and distance from the listener of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int32_t FX_Pan3D(int32_t handle,int32_t angle,int32_t distance)
{
    int32_t status;

    status = MV_Pan3D(handle, angle, distance);
    if (status != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Warning;
    }

    return status;
}


/*---------------------------------------------------------------------
   Function: FX_SoundActive

   Tests if the specified sound is currently playing.
---------------------------------------------------------------------*/

int32_t FX_SoundActive(int32_t handle)
{
    return MV_VoicePlaying(handle);
}


/*---------------------------------------------------------------------
   Function: FX_SoundsPlaying

   Reports the number of voices playing.
---------------------------------------------------------------------*/

int32_t FX_SoundsPlaying(void)
{
    return MV_VoicesPlaying();
}


/*---------------------------------------------------------------------
   Function: FX_StopSound

   Halts playback of a specific voice
---------------------------------------------------------------------*/

int32_t FX_StopSound(int32_t handle)
{
    int32_t status;

    status = MV_Kill(handle);
    if (status != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        return FX_Warning;
    }

    return FX_Ok;
}


/*---------------------------------------------------------------------
   Function: FX_StopAllSounds

   Halts playback of all sounds.
---------------------------------------------------------------------*/

int32_t FX_StopAllSounds
(
    void
)

{
    int32_t status;

    status = MV_KillAllVoices();
    if (status != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        return FX_Warning;
    }

    return FX_Ok;
}


/*---------------------------------------------------------------------
   Function: FX_StartDemandFeedPlayback

   Plays a digitized sound from a user controlled buffering system.
---------------------------------------------------------------------*/

int32_t FX_StartDemandFeedPlayback
(
    void (*function)(char **ptr, uint32_t *length),
    int32_t rate,
    int32_t pitchoffset,
    int32_t vol,
    int32_t left,
    int32_t right,
    int32_t priority,
    uint32_t callbackval
)

{
    int32_t handle;

    handle = MV_StartDemandFeedPlayback(function, rate,
                                        pitchoffset, vol, left, right, priority, callbackval);
    if (handle < MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return handle;
}


static wavedata FX_AutoDetectFormat(const char *ptr)
{
    switch (LITTLE32(*(int32_t *)ptr))
    {
    case 'C'+('r'<<8)+('e'<<16)+('a'<<24): // Crea
        return VOC;
        break;
    case 'R'+('I'<<8)+('F'<<16)+('F'<<24): // RIFF
        return WAV;
        break;
    case 'O'+('g'<<8)+('g'<<16)+('S'<<24): // OggS
        return Vorbis;
        break;
    case 'f'+('L'<<8)+('a'<<16)+('C'<<24): // fLaC
        return FLAC;
        break;
    default:
        switch (LITTLE32(*(int32_t *)(ptr + 8)))
        {
        case 'W'+('A'<<8)+('V'<<16)+('E'<<24): // WAVE
            return WAV;
            break;
        }
        break;
    }

    return Unknown;
}

/*---------------------------------------------------------------------
   Function: FX_PlayAuto

   Play a sound, autodetecting the format.
---------------------------------------------------------------------*/
int32_t FX_PlayAuto(char *ptr, uint32_t length, int32_t pitchoffset, int32_t vol,
                int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{
    return FX_PlayLoopedAuto(ptr, length, -1, -1, pitchoffset, vol, left, right, priority, callbackval);;
}

/*---------------------------------------------------------------------
   Function: FX_PlayLoopedAuto

   Play a looped sound, autodetecting the format.
---------------------------------------------------------------------*/
int32_t FX_PlayLoopedAuto(char *ptr, uint32_t length, int32_t loopstart, int32_t loopend,
                      int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority,
                      uint32_t callbackval)
{
    int32_t handle = -1;

    switch (FX_AutoDetectFormat(ptr))
    {
    case VOC:
        handle = MV_PlayVOC(ptr, length, loopstart, loopend, pitchoffset, vol, left, right, priority, callbackval);
        break;
    case WAV:
        handle = MV_PlayWAV(ptr, length, loopstart, loopend, pitchoffset, vol, left, right, priority, callbackval);
        break;
    case Vorbis:
#ifdef HAVE_VORBIS
        handle = MV_PlayVorbis(ptr, length, loopstart, loopend, pitchoffset, vol, left, right, priority, callbackval);
#else
        MV_Printf("FX_PlayLoopedAuto: OggVorbis support not included in this binary.\n");
#endif
        break;
    case FLAC:
#ifdef HAVE_FLAC
        handle = MV_PlayFLAC(ptr, length, loopstart, loopend, pitchoffset, vol, left, right, priority, callbackval);
#else
        MV_Printf("FX_PlayLoopedAuto: FLAC support not included in this binary.\n");
#endif
        break;
    default:
        MV_Printf("FX_PlayLoopedAuto: Unknown or unsupported format.\n");
        break;
    }


    if (handle <= MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return handle;
}

/*---------------------------------------------------------------------
   Function: FX_PlayAuto3D

   Play a positioned sound, autodetecting the format.
   <loophow>: one of FX_LOOP or FX_ONESHOT.
---------------------------------------------------------------------*/
int32_t FX_PlayAuto3D(char *ptr, uint32_t length, int32_t loophow, int32_t pitchoffset, int32_t angle,
                  int32_t distance, int32_t priority, uint32_t callbackval)
{
    int32_t handle = -1;

    switch (FX_AutoDetectFormat(ptr))
    {
    case VOC:
        handle = MV_PlayVOC3D(ptr, length, loophow, pitchoffset, angle, distance, priority, callbackval);
        break;
    case WAV:
        handle = MV_PlayWAV3D(ptr, length, loophow, pitchoffset, angle, distance, priority, callbackval);
        break;
    case Vorbis:
#ifdef HAVE_VORBIS
        handle = MV_PlayVorbis3D(ptr, length, loophow, pitchoffset, angle, distance, priority, callbackval);
#else
        MV_Printf("FX_PlayAuto3D: OggVorbis support not included in this binary.\n");
#endif
        break;
    case FLAC:
#ifdef HAVE_FLAC
        handle = MV_PlayFLAC3D(ptr, length, loophow, pitchoffset, angle, distance, priority, callbackval);
#else
        MV_Printf("FX_PlayAuto3D: FLAC support not included in this binary.\n");
#endif
        break;
    default:
        MV_Printf("FX_PlayAuto3D: Unknown or unsupported format.\n");
        break;
    }

    if (handle <= MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return handle;
}

int32_t FX_SetVoiceCallback(int32_t handle, uint32_t callbackval)
{
    int32_t status;

    status = MV_SetVoiceCallback(handle, callbackval);
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
