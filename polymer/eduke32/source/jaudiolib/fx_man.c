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

Modifications for JonoF's port by Jonathon Fowler (jonof@edgenetwk.com)
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
//#include <duke3d.h>
#include "multivoc.h"
#include "ll_man.h"
#include "fx_man.h"

#ifndef TRUE
#define TRUE  ( 1 == 1 )
#define FALSE ( !TRUE )
#endif

static unsigned FX_MixRate;

static char tempbuf[2048];
extern void initprintf(const char *, ...);

int32_t FX_SoundDevice = -1;
int32_t FX_ErrorCode = FX_Ok;
int32_t FX_Installed = FALSE;

#define FX_SetErrorCode( status ) \
   FX_ErrorCode = ( status );

/*---------------------------------------------------------------------
   Function: FX_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/

char *FX_ErrorString
(
    int32_t ErrorNumber
)

{
    char *ErrorString;

    switch (ErrorNumber)
    {
    case FX_Warning :
    case FX_Error :
        ErrorString = FX_ErrorString(FX_ErrorCode);
        break;

    case FX_Ok :
        ErrorString = "Fx ok.";
        break;

    case FX_ASSVersion :
        ErrorString = "Apogee Sound System Version 0  "
                      "Programmed by Jim Dose\n"
                      "(c) Copyright 1995 James R. Dose.  All Rights Reserved.\n";
        break;

    case FX_BlasterError :
    case FX_SoundCardError :
        ErrorString = "Sound device error.";
        break;

    case FX_InvalidCard :
        ErrorString = "Invalid Sound Fx device.";
        break;

    case FX_MultiVocError :
        ErrorString = MV_ErrorString(MV_Error);
        break;

    case FX_DPMI_Error :
        ErrorString = "DPMI Error in FX_MAN.";
        break;

    default :
        ErrorString = "Unknown Fx error code.";
        break;
    }

    return(ErrorString);
}


#if 0
/*---------------------------------------------------------------------
   Function: FX_SetupCard

   Sets the configuration of a sound device.
---------------------------------------------------------------------*/

int32_t FX_SetupCard
(
    int32_t SoundCard,
    fx_device *device
)

{
    int32_t status;
    int32_t DeviceStatus;

    FX_SoundDevice = SoundCard;

    status = FX_Ok;
    FX_SetErrorCode(FX_Ok);

    switch (SoundCard)
    {
    case SoundBlaster :
        DeviceStatus = BLASTER_Init();
        if (DeviceStatus != BLASTER_Ok)
        {
            FX_SetErrorCode(FX_SoundCardError);
            status = FX_Error;
            break;
        }

        device->MaxVoices = 32;
        BLASTER_GetCardInfo(&device->MaxSampleBits, &device->MaxChannels);
        break;

    default :
        FX_SetErrorCode(FX_InvalidCard);
        status = FX_Error;
    }
    return(status);
}
#endif


#if 0
/*---------------------------------------------------------------------
   Function: FX_GetBlasterSettings

   Returns the current BLASTER environment variable settings.
---------------------------------------------------------------------*/

int32_t FX_GetBlasterSettings
(
    fx_blaster_config *blaster
)

{
    int32_t status;
    BLASTER_CONFIG Blaster;

    FX_SetErrorCode(FX_Ok);

    status = BLASTER_GetEnv(&Blaster);
    if (status != BLASTER_Ok)
    {
        FX_SetErrorCode(FX_BlasterError);
        return(FX_Error);
    }

    blaster->Type      = Blaster.Type;
    blaster->Address   = Blaster.Address;
    blaster->Interrupt = Blaster.Interrupt;
    blaster->Dma8      = Blaster.Dma8;
    blaster->Dma16     = Blaster.Dma16;
    blaster->Midi      = Blaster.Midi;
    blaster->Emu       = Blaster.Emu;

    return(FX_Ok);
}
#endif


#if 0
/*---------------------------------------------------------------------
   Function: FX_SetupSoundBlaster

   Handles manual setup of the Sound Blaster information.
---------------------------------------------------------------------*/

int32_t FX_SetupSoundBlaster
(
    fx_blaster_config blaster,
    int32_t *MaxVoices,
    int32_t *MaxSampleBits,
    int32_t *MaxChannels
)

{
    int32_t DeviceStatus;
    BLASTER_CONFIG Blaster;

    FX_SetErrorCode(FX_Ok);

    FX_SoundDevice = SoundBlaster;

    Blaster.Type      = blaster.Type;
    Blaster.Address   = blaster.Address;
    Blaster.Interrupt = blaster.Interrupt;
    Blaster.Dma8      = blaster.Dma8;
    Blaster.Dma16     = blaster.Dma16;
    Blaster.Midi      = blaster.Midi;
    Blaster.Emu       = blaster.Emu;

    BLASTER_SetCardSettings(Blaster);

    DeviceStatus = BLASTER_Init();
    if (DeviceStatus != BLASTER_Ok)
    {
        FX_SetErrorCode(FX_SoundCardError);
        return(FX_Error);
    }

    *MaxVoices = 8;
    BLASTER_GetCardInfo(MaxSampleBits, MaxChannels);

    return(FX_Ok);
}
#endif


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
    unsigned mixrate
)

{
    int32_t status;
    int32_t devicestatus;

    if (FX_Installed)
    {
        FX_Shutdown();
    }

    FX_MixRate = mixrate;

    status = FX_Ok;
    FX_SoundDevice = SoundCard;

    devicestatus = MV_Init(SoundCard, FX_MixRate, numvoices,
                           numchannels, samplebits);
    if (devicestatus != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Error;
    }

    if (status == FX_Ok)
    {
        FX_Installed = TRUE;
    }

    return(status);
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
        return(FX_Ok);
    }

    status = MV_Shutdown();
    if (status != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Error;
    }

    FX_Installed = FALSE;

    return(status);
}


/*---------------------------------------------------------------------
   Function: FX_SetCallback

   Sets the function to call when a voice is done.
---------------------------------------------------------------------*/

int32_t FX_SetCallBack
(
    void(*function)(uint32_t)
)

{
    MV_SetCallBack(function);

    return(FX_Ok);
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
    return MV_GetVolume();
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

    return(status);
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

    return(status);
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

    return(status);
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

    return(status);
}


/*---------------------------------------------------------------------
   Function: FX_PlayVOC

   Begin playback of sound data with the given volume and priority.
---------------------------------------------------------------------*/

int32_t FX_PlayVOC
(
    char *ptr,
    int32_t pitchoffset,
    int32_t vol,
    int32_t left,
    int32_t right,
    int32_t priority,
    uint32_t callbackval
)

{
    int32_t handle;

    handle = MV_PlayVOC(ptr, pitchoffset, vol, left, right,
                        priority, callbackval);
    if (handle < MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return(handle);
}


/*---------------------------------------------------------------------
   Function: FX_PlayLoopedVOC

   Begin playback of sound data with the given volume and priority.
---------------------------------------------------------------------*/

int32_t FX_PlayLoopedVOC
(
    char *ptr,
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

    handle = MV_PlayLoopedVOC(ptr, loopstart, loopend, pitchoffset,
                              vol, left, right, priority, callbackval);
    if (handle < MV_Ok)
    {
        Bsprintf(tempbuf, "Sound error %d: %s\n",callbackval, FX_ErrorString(FX_Error));
        initprintf(tempbuf);
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return(handle);
}


/*---------------------------------------------------------------------
   Function: FX_PlayWAV

   Begin playback of sound data with the given volume and priority.
---------------------------------------------------------------------*/

int32_t FX_PlayWAV
(
    char *ptr,
    int32_t pitchoffset,
    int32_t vol,
    int32_t left,
    int32_t right,
    int32_t priority,
    uint32_t callbackval
)

{
    int32_t handle;

    handle = MV_PlayWAV(ptr, pitchoffset, vol, left, right,
                        priority, callbackval);
    if (handle < MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return(handle);
}

/*---------------------------------------------------------------------
   Function: FX_PlayOGG

   Begin playback of sound data with the given volume and priority.
---------------------------------------------------------------------*/

int32_t FX_PlayOGG
(
    char *ptr,
    int32_t pitchoffset,
    int32_t vol,
    int32_t left,
    int32_t right,
    int32_t priority,
    uint32_t callbackval
)

{
    int32_t handle;

    handle = MV_PlayOGG(ptr, pitchoffset, vol, left, right,
                        priority, callbackval);
    if (handle < MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return(handle);
}

/*---------------------------------------------------------------------
   Function: FX_PlayWAV

   Begin playback of sound data with the given volume and priority.
---------------------------------------------------------------------*/

int32_t FX_PlayLoopedWAV
(
    char *ptr,
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

    handle = MV_PlayLoopedWAV(ptr, loopstart, loopend,
                              pitchoffset, vol, left, right, priority, callbackval);
    if (handle < MV_Ok)
    {
        Bsprintf(tempbuf, "Sound error %d: %s\n",callbackval, FX_ErrorString(FX_Error));
        initprintf(tempbuf);
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return(handle);
}

int32_t FX_PlayLoopedOGG
(
    char *ptr,
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

    handle = MV_PlayLoopedOGG(ptr, loopstart, loopend,
                              pitchoffset, vol, left, right, priority, callbackval);
    if (handle < MV_Ok)
    {
        Bsprintf(tempbuf, "Sound error %d: %s\n",callbackval, FX_ErrorString(FX_Error));
        initprintf(tempbuf);
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return(handle);
}

/*---------------------------------------------------------------------
   Function: FX_PlayVOC3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int32_t FX_PlayVOC3D
(
    char *ptr,
    int32_t pitchoffset,
    int32_t angle,
    int32_t distance,
    int32_t priority,
    uint32_t callbackval
)

{
    int32_t handle;

    handle = MV_PlayVOC3D(ptr, pitchoffset, angle, distance,
                          priority, callbackval);
    if (handle < MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return(handle);
}

int32_t FX_PlayOGG3D
(
    char *ptr,
    int32_t pitchoffset,
    int32_t angle,
    int32_t distance,
    int32_t priority,
    uint32_t callbackval
)

{
    int32_t handle;

    handle = MV_PlayOGG3D(ptr, pitchoffset, angle, distance,
                          priority, callbackval);
    if (handle < MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return(handle);
}

/*---------------------------------------------------------------------
   Function: FX_PlayWAV3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int32_t FX_PlayWAV3D
(
    char *ptr,
    int32_t pitchoffset,
    int32_t angle,
    int32_t distance,
    int32_t priority,
    uint32_t callbackval
)

{
    int32_t handle;

    handle = MV_PlayWAV3D(ptr, pitchoffset, angle, distance,
                          priority, callbackval);
    if (handle < MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return(handle);
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
    int32_t handle;

    handle = MV_PlayRaw(ptr, length, rate, pitchoffset,
                        vol, left, right, priority, callbackval);
    if (handle < MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return(handle);
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

    handle = MV_PlayLoopedRaw(ptr, length, loopstart, loopend,
                              rate, pitchoffset, vol, left, right, priority, callbackval);
    if (handle < MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return(handle);
}


/*---------------------------------------------------------------------
   Function: FX_Pan3D

   Set the angle and distance from the listener of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int32_t FX_Pan3D
(
    int32_t handle,
    int32_t angle,
    int32_t distance
)

{
    int32_t status;

    status = MV_Pan3D(handle, angle, distance);
    if (status != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Warning;
    }

    return(status);
}


/*---------------------------------------------------------------------
   Function: FX_SoundActive

   Tests if the specified sound is currently playing.
---------------------------------------------------------------------*/

int32_t FX_SoundActive
(
    int32_t handle
)

{
    return(MV_VoicePlaying(handle));
}


/*---------------------------------------------------------------------
   Function: FX_SoundsPlaying

   Reports the number of voices playing.
---------------------------------------------------------------------*/

int32_t FX_SoundsPlaying
(
    void
)

{
    return(MV_VoicesPlaying());
}


/*---------------------------------------------------------------------
   Function: FX_StopSound

   Halts playback of a specific voice
---------------------------------------------------------------------*/

int32_t FX_StopSound
(
    int32_t handle
)

{
    int32_t status;

    status = MV_Kill(handle);
    if (status != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        return(FX_Warning);
    }

    return(FX_Ok);
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
        return(FX_Warning);
    }

    return(FX_Ok);
}


/*---------------------------------------------------------------------
   Function: FX_StartDemandFeedPlayback

   Plays a digitized sound from a user controlled buffering system.
---------------------------------------------------------------------*/

int32_t FX_StartDemandFeedPlayback
(
    void(*function)(char **ptr, uint32_t *length),
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

    return(handle);
}

#if 0
/*---------------------------------------------------------------------
   Function: FX_StartRecording

   Starts the sound recording engine.
---------------------------------------------------------------------*/

int32_t FX_StartRecording
(
    int32_t MixRate,
    void(*function)(char *ptr, int32_t length)
)

{
    int32_t status;

    switch (FX_SoundDevice)
    {
    case SoundBlaster :
        status = MV_StartRecording(MixRate, function);
        if (status != MV_Ok)
        {
            FX_SetErrorCode(FX_MultiVocError);
            status = FX_Warning;
        }
        else
        {
            status = FX_Ok;
        }
        break;

    default :
        FX_SetErrorCode(FX_InvalidCard);
        status = FX_Warning;
        break;
    }

    return(status);
}
#endif

#if 0
/*---------------------------------------------------------------------
   Function: FX_StopRecord

   Stops the sound record engine.
---------------------------------------------------------------------*/

void FX_StopRecord
(
    void
)

{
    // Stop sound playback
    switch (FX_SoundDevice)
    {
    case SoundBlaster :
        MV_StopRecord();
        break;
    }
}
#endif

extern void MUSIC_Update(void);
void AudioUpdate(void) { MUSIC_Update(); }
