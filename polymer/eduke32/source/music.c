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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

/**********************************************************************
   module: MUSIC.C

   author: James R. Dose
   date:   March 25, 1994

   Device independant music playback routines.

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "music.h"
#include "midi.h"
#include "mpu401.h"
#include "compat.h"

#ifndef TRUE
#define TRUE  ( 1 == 1 )
#define FALSE ( !TRUE )
#endif

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef max
# define max(a,b) ( ((a) > (b)) ? (a) : (b) )
#endif

int32_t MUSIC_SoundDevice = -1;
int32_t MUSIC_ErrorCode = MUSIC_Ok;

static midifuncs MUSIC_MidiFunctions;

int32_t MUSIC_InitMidi(int32_t card, midifuncs *Funcs, int32_t Address);

#define MUSIC_SetErrorCode( status ) \
   MUSIC_ErrorCode = ( status );

/*---------------------------------------------------------------------
   Function: MUSIC_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/

const char *MUSIC_ErrorString
(
    int32_t ErrorNumber
)

{
    const char *ErrorString;

    switch (ErrorNumber)
    {
    case MUSIC_Warning :
    case MUSIC_Error :
        ErrorString = MUSIC_ErrorString(MUSIC_ErrorCode);
        break;

    case MUSIC_Ok :
        ErrorString = "Music ok.";
        break;

    case MUSIC_ASSVersion :
        ErrorString = "Apogee Sound System Version WinMM  "
                      "Programmed by Jim Dose, Ported by Jonathon Fowler\n"
                      "(c) Copyright 1996 James R. Dose.  All Rights Reserved.\n";
        break;

    case MUSIC_SoundCardError :
    case MUSIC_MPU401Error :
        ErrorString = "Could not detect MPU-401.";
        break;

    case MUSIC_InvalidCard :
        ErrorString = "Invalid Music device.";
        break;

    case MUSIC_MidiError :
        ErrorString = "Error playing MIDI file.";
        break;

    case MUSIC_TaskManError :
        ErrorString = "TaskMan error.";
        break;

    case MUSIC_DPMI_Error :
        ErrorString = "DPMI Error in MUSIC.";
        break;

    default :
        ErrorString = "Unknown Music error code.";
        break;
    }

    return(ErrorString);
}


/*---------------------------------------------------------------------
   Function: MUSIC_Init

   Selects which sound device to use.
---------------------------------------------------------------------*/

int32_t MUSIC_Init
(
    int32_t SoundCard,
    int32_t Address
)

{
    int32_t i;
    int32_t status;

    for (i = 0; i < 128; i++)
    {
        MIDI_PatchMap[ i ] = i;
    }

    MUSIC_SoundDevice = SoundCard;

    status = MUSIC_InitMidi(SoundCard, &MUSIC_MidiFunctions, Address);

    return(status);
}


/*---------------------------------------------------------------------
   Function: MUSIC_Shutdown

   Terminates use of sound device.
---------------------------------------------------------------------*/

int32_t MUSIC_Shutdown
(
    void
)

{
    int32_t status;

    status = MUSIC_Ok;

    MIDI_StopSong();

    //MPU_Reset();

    return(status);
}


/*---------------------------------------------------------------------
   Function: MUSIC_SetVolume

   Sets the volume of music playback.
---------------------------------------------------------------------*/

void MUSIC_SetVolume
(
    int32_t volume
)

{
    volume = max(0, volume);
    volume = min(volume, 255);
    if (MUSIC_SoundDevice != -1)
    {
        MIDI_SetVolume(volume);
    }
}


/*---------------------------------------------------------------------
   Function: MUSIC_SetMidiChannelVolume

   Sets the volume of music playback on the specified MIDI channel.
---------------------------------------------------------------------*/

void MUSIC_SetMidiChannelVolume
(
    int32_t channel,
    int32_t volume
)

{
    MIDI_SetUserChannelVolume(channel, volume);
}


/*---------------------------------------------------------------------
   Function: MUSIC_ResetMidiChannelVolumes

   Sets the volume of music playback on all MIDI channels to full volume.
---------------------------------------------------------------------*/

void MUSIC_ResetMidiChannelVolumes
(
    void
)

{
    MIDI_ResetUserChannelVolume();
}


/*---------------------------------------------------------------------
   Function: MUSIC_GetVolume

   Returns the volume of music playback.
---------------------------------------------------------------------*/

int32_t MUSIC_GetVolume
(
    void
)

{
    if (MUSIC_SoundDevice == -1)
    {
        return(0);
    }
    return(MIDI_GetVolume());
}


/*---------------------------------------------------------------------
   Function: MUSIC_SetLoopFlag

   Set whether the music will loop or end when it reaches the end of
   the song.
---------------------------------------------------------------------*/

void MUSIC_SetLoopFlag
(
    int32_t loopflag
)

{
    MIDI_SetLoopFlag(loopflag);
}


/*---------------------------------------------------------------------
   Function: MUSIC_SongPlaying

   Returns whether there is a song playing.
---------------------------------------------------------------------*/

int32_t MUSIC_SongPlaying
(
    void
)

{
    return(MIDI_SongPlaying());
}


/*---------------------------------------------------------------------
   Function: MUSIC_Continue

   Continues playback of a paused song.
---------------------------------------------------------------------*/

void MUSIC_Continue
(
    void
)

{
    MIDI_ContinueSong();
}


/*---------------------------------------------------------------------
   Function: MUSIC_Pause

   Pauses playback of a song.
---------------------------------------------------------------------*/

void MUSIC_Pause
(
    void
)

{
    MIDI_PauseSong();
}


/*---------------------------------------------------------------------
   Function: MUSIC_StopSong

   Stops playback of current song.
---------------------------------------------------------------------*/

int32_t MUSIC_StopSong
(
    void
)

{
    MUSIC_StopFade();
    MIDI_StopSong();
    MUSIC_SetErrorCode(MUSIC_Ok);
    return(MUSIC_Ok);
}


/*---------------------------------------------------------------------
   Function: MUSIC_PlaySong

   Begins playback of MIDI song.
---------------------------------------------------------------------*/

int32_t MUSIC_PlaySong
(
    char *song,
    int32_t loopflag
)

{
    int32_t status;

    {
        MUSIC_StopSong();
        status = MIDI_PlaySong(song, loopflag);
        if (status != MIDI_Ok)
        {
            MUSIC_SetErrorCode(MUSIC_MidiError);
            return(MUSIC_Warning);
        }
    }

    return(MUSIC_Ok);
}


/*---------------------------------------------------------------------
   Function: MUSIC_SetContext

   Sets the song context.
---------------------------------------------------------------------*/

void MUSIC_SetContext
(
    int32_t context
)

{
    MIDI_SetContext(context);
}


/*---------------------------------------------------------------------
   Function: MUSIC_GetContext

   Returns the current song context.
---------------------------------------------------------------------*/

int32_t MUSIC_GetContext
(
    void
)

{
    return MIDI_GetContext();
}


/*---------------------------------------------------------------------
   Function: MUSIC_SetSongTick

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_SetSongTick
(
    uint32_t PositionInTicks
)

{
    MIDI_SetSongTick(PositionInTicks);
}


/*---------------------------------------------------------------------
   Function: MUSIC_SetSongTime

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_SetSongTime
(
    uint32_t milliseconds
)

{
    MIDI_SetSongTime(milliseconds);
}


/*---------------------------------------------------------------------
   Function: MUSIC_SetSongPosition

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_SetSongPosition
(
    int32_t measure,
    int32_t beat,
    int32_t tick
)

{
    MIDI_SetSongPosition(measure, beat, tick);
}


/*---------------------------------------------------------------------
   Function: MUSIC_GetSongPosition

   Returns the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_GetSongPosition
(
    songposition *pos
)

{
    MIDI_GetSongPosition(pos);
}


/*---------------------------------------------------------------------
   Function: MUSIC_GetSongLength

   Returns the length of the song.
---------------------------------------------------------------------*/

void MUSIC_GetSongLength
(
    songposition *pos
)

{
    MIDI_GetSongLength(pos);
}


int32_t MUSIC_InitMidi
(
    int32_t        card,
    midifuncs *Funcs,
    int32_t        Address
)

{
    UNREFERENCED_PARAMETER(card);
    UNREFERENCED_PARAMETER(Address);
    Funcs->NoteOff           = MPU_NoteOff;
    Funcs->NoteOn            = MPU_NoteOn;
    Funcs->PolyAftertouch    = MPU_PolyAftertouch;
    Funcs->ControlChange     = MPU_ControlChange;
    Funcs->ProgramChange     = MPU_ProgramChange;
    Funcs->ChannelAftertouch = MPU_ChannelAftertouch;
    Funcs->PitchBend         = MPU_PitchBend;
    Funcs->ReleasePatches    = NULL;
    Funcs->LoadPatch         = NULL;
    Funcs->SetVolume         = NULL /*MPU_SetVolume*/;
    Funcs->GetVolume         = NULL /*MPU_GetVolume*/;

    MIDI_SetMidiFuncs(Funcs);

    return(MIDI_Ok);
}


/*---------------------------------------------------------------------
   Function: MUSIC_FadeVolume

   Fades music volume from current level to another over a specified
   period of time.
---------------------------------------------------------------------*/

int32_t MUSIC_FadeVolume
(
    int32_t tovolume,
    int32_t milliseconds
)

{
    UNREFERENCED_PARAMETER(milliseconds);
    MIDI_SetVolume(tovolume);
    return(MUSIC_Ok);
}


/*---------------------------------------------------------------------
   Function: MUSIC_FadeActive

   Returns whether the fade routine is active.
---------------------------------------------------------------------*/

int32_t MUSIC_FadeActive
(
    void
)

{
    return(0);
}


/*---------------------------------------------------------------------
   Function: MUSIC_StopFade

   Stops fading the music.
---------------------------------------------------------------------*/

void MUSIC_StopFade
(
    void
)

{}


/*---------------------------------------------------------------------
   Function: MUSIC_RerouteMidiChannel

   Sets callback function to reroute MIDI commands from specified
   function.
---------------------------------------------------------------------*/

void MUSIC_RerouteMidiChannel
(
    int32_t channel,
    int32_t(*function)(int32_t, int32_t, int32_t)
)

{
    MIDI_RerouteMidiChannel(channel, function);
}


/*---------------------------------------------------------------------
   Function: MUSIC_RegisterTimbreBank

   Halts playback of all sounds.
---------------------------------------------------------------------*/

void MUSIC_RegisterTimbreBank
(
    char *timbres
)

{
    UNREFERENCED_PARAMETER(timbres);
}


void MUSIC_Update(void)
{
    MIDI_UpdateMusic();
}

