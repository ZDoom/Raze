//-------------------------------------------------------------------------
/*
Duke Nukem Copyright (C) 1996, 2003 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
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

  Dummy AudioLib stub implementation by Jonathon Fowler (jonof@edgenetwk.com)
*/
//-------------------------------------------------------------------------

#include "fx_man.h"


#define TRUE  ( 1 == 1 )
#define FALSE ( !TRUE )


int32_t FX_ErrorCode = FX_Ok;

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

    default :
        ErrorString = "Unknown Fx error code.";
        break;
    }

    return(ErrorString);
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
    unsigned mixrate
)

{
    return(FX_Ok);
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
    return(FX_Ok);
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

{}


/*---------------------------------------------------------------------
   Function: FX_SetReverseStereo

   Set the orientation of the left and right channels.
---------------------------------------------------------------------*/

void FX_SetReverseStereo
(
    int32_t setting
)

{}


/*---------------------------------------------------------------------
   Function: FX_GetReverseStereo

   Returns the orientation of the left and right channels.
---------------------------------------------------------------------*/

int32_t FX_GetReverseStereo
(
    void
)

{
    return 0;
}


/*---------------------------------------------------------------------
   Function: FX_SetReverb

   Sets the reverb level.
---------------------------------------------------------------------*/

void FX_SetReverb
(
    int32_t reverb
)

{}


/*---------------------------------------------------------------------
   Function: FX_SetReverbDelay

   Sets the delay level of reverb to add to mix.
---------------------------------------------------------------------*/

void FX_SetReverbDelay
(
    int32_t delay
)

{}


/*---------------------------------------------------------------------
   Function: FX_VoiceAvailable

   Checks if a voice can be play at the specified priority.
---------------------------------------------------------------------*/

int32_t FX_VoiceAvailable
(
    int32_t priority
)

{
    return 0;
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
    return(0);
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
    return(0);
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
    return(0);
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
    return(0);
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
    return(0);
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
    return(FX_Ok);
}


void AudioUpdate(void) { }

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
    return(0);
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
    return(0);
}