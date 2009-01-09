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

#include "music.h"


#define TRUE  ( 1 == 1 )
#define FALSE ( !TRUE )


int32_t MUSIC_ErrorCode = MUSIC_Ok;


/*---------------------------------------------------------------------
   Function: MUSIC_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/

char *MUSIC_ErrorString
(
    int32_t ErrorNumber
)

{
    char *ErrorString;

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
        ErrorString = "Apogee Sound System Version   "
                      "Programmed by Jim Dose\n"
                      "(c) Copyright 1996 James R. Dose.  All Rights Reserved.\n";
        break;

    case MUSIC_SoundCardError :
        break;

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

    status = MUSIC_Ok;

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


    return(status);
}


/*---------------------------------------------------------------------
   Function: MUSIC_SetMaxFMMidiChannel

   Sets the maximum MIDI channel that FM cards respond to.
---------------------------------------------------------------------*/

void MUSIC_SetMaxFMMidiChannel
(
    int32_t channel
)

{}


/*---------------------------------------------------------------------
   Function: MUSIC_SetVolume

   Sets the volume of music playback.
---------------------------------------------------------------------*/

void MUSIC_SetVolume
(
    int32_t volume
)

{}


/*---------------------------------------------------------------------
   Function: MUSIC_SetMidiChannelVolume

   Sets the volume of music playback on the specified MIDI channel.
---------------------------------------------------------------------*/

void MUSIC_SetMidiChannelVolume
(
    int32_t channel,
    int32_t volume
)

{}


/*---------------------------------------------------------------------
   Function: MUSIC_ResetMidiChannelVolumes

   Sets the volume of music playback on all MIDI channels to full volume.
---------------------------------------------------------------------*/

void MUSIC_ResetMidiChannelVolumes
(
    void
)

{}


/*---------------------------------------------------------------------
   Function: MUSIC_GetVolume

   Returns the volume of music playback.
---------------------------------------------------------------------*/

int32_t MUSIC_GetVolume
(
    void
)

{
    return(0);
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

{}


/*---------------------------------------------------------------------
   Function: MUSIC_SongPlaying

   Returns whether there is a song playing.
---------------------------------------------------------------------*/

int32_t MUSIC_SongPlaying
(
    void
)

{
    return(0);
}


/*---------------------------------------------------------------------
   Function: MUSIC_Continue

   Continues playback of a paused song.
---------------------------------------------------------------------*/

void MUSIC_Continue
(
    void
)

{}


/*---------------------------------------------------------------------
   Function: MUSIC_Pause

   Pauses playback of a song.
---------------------------------------------------------------------*/

void MUSIC_Pause
(
    void
)

{}


/*---------------------------------------------------------------------
   Function: MUSIC_StopSong

   Stops playback of current song.
---------------------------------------------------------------------*/

int32_t MUSIC_StopSong
(
    void
)

{
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

{}


/*---------------------------------------------------------------------
   Function: MUSIC_GetContext

   Returns the current song context.
---------------------------------------------------------------------*/

int32_t MUSIC_GetContext
(
    void
)

{ return 0; }


/*---------------------------------------------------------------------
   Function: MUSIC_SetSongTick

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_SetSongTick
(
    uint32_t PositionInTicks
)

{}


/*---------------------------------------------------------------------
   Function: MUSIC_SetSongTime

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_SetSongTime
(
    uint32_t milliseconds
)

{}


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

{}


/*---------------------------------------------------------------------
   Function: MUSIC_GetSongPosition

   Returns the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_GetSongPosition
(
    songposition *pos
)

{}


/*---------------------------------------------------------------------
   Function: MUSIC_GetSongLength

   Returns the length of the song.
---------------------------------------------------------------------*/

void MUSIC_GetSongLength
(
    songposition *pos
)

{}






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
    int32_t(*function)(int32_t event, int32_t c1, int32_t c2)
)

{}


/*---------------------------------------------------------------------
   Function: MUSIC_RegisterTimbreBank

   Halts playback of all sounds.
---------------------------------------------------------------------*/

void MUSIC_RegisterTimbreBank
(
    char *timbres
)

{}


void MUSIC_Update(void)
{}

void MUSIC_PlayMusic(char *_filename, int32_t loopflag)
{}
