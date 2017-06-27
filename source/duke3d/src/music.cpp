//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

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

// This object is shared by all Build games with MIDI playback!

#include "compat.h"
#include "music.h"
#include "midi.h"
#include "mpu401.h"

int32_t MUSIC_SoundDevice = -1;
int32_t MUSIC_ErrorCode = MUSIC_Ok;

static midifuncs MUSIC_MidiFunctions;

int32_t MUSIC_InitMidi(int32_t card, midifuncs *Funcs, int32_t Address);

#define MUSIC_SetErrorCode(status) MUSIC_ErrorCode = (status);

const char *MUSIC_ErrorString(int32_t ErrorNumber)
{
    const char *ErrorString;

    switch (ErrorNumber)
    {
        case MUSIC_Warning:
        case MUSIC_Error:       ErrorString = MUSIC_ErrorString(MUSIC_ErrorCode); break;
        case MUSIC_Ok:          ErrorString = "Music ok."; break;
        case MUSIC_MidiError:   ErrorString = "Error playing MIDI file."; break;
        default:                ErrorString = "Unknown Music error code."; break;
    }

    return ErrorString;
}


int32_t MUSIC_Init(int32_t SoundCard, int32_t Address)
{
    MUSIC_SoundDevice = SoundCard;

    return MUSIC_InitMidi(SoundCard, &MUSIC_MidiFunctions, Address);
}


int32_t MUSIC_Shutdown(void)
{
    MIDI_StopSong();

    return MUSIC_Ok;
}


void MUSIC_SetVolume(int32_t volume)
{
    if (MUSIC_SoundDevice != -1)
        MIDI_SetVolume(min(max(0, volume), 255));
}


int32_t MUSIC_GetVolume(void) { return MUSIC_SoundDevice == -1 ? 0 : MIDI_GetVolume(); }
void MUSIC_SetLoopFlag(int32_t loopflag) { MIDI_SetLoopFlag(loopflag); }
void MUSIC_Continue(void) { MIDI_ContinueSong(); }
void MUSIC_Pause(void) { MIDI_PauseSong(); }

int32_t MUSIC_StopSong(void)
{
    MIDI_StopSong();
    MUSIC_SetErrorCode(MUSIC_Ok);
    return MUSIC_Ok;
}


int32_t MUSIC_PlaySong(char *song, int32_t loopflag)
{
    MUSIC_StopSong();

    if (MIDI_PlaySong(song, loopflag) != MIDI_Ok)
    {
        MUSIC_SetErrorCode(MUSIC_MidiError);
        return MUSIC_Warning;
    }

    return MUSIC_Ok;
}


int32_t MUSIC_InitMidi(int32_t card, midifuncs *Funcs, int32_t Address)
{
    UNREFERENCED_PARAMETER(card);
    UNREFERENCED_PARAMETER(Address);
    Funcs->NoteOff = MPU_NoteOff;
    Funcs->NoteOn = MPU_NoteOn;
    Funcs->PolyAftertouch = MPU_PolyAftertouch;
    Funcs->ControlChange = MPU_ControlChange;
    Funcs->ProgramChange = MPU_ProgramChange;
    Funcs->ChannelAftertouch = MPU_ChannelAftertouch;
    Funcs->PitchBend = MPU_PitchBend;

    MIDI_SetMidiFuncs(Funcs);

    return MIDI_Ok;
}

void MUSIC_Update(void) { MIDI_UpdateMusic(); }
