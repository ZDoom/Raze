//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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

#include "music.h"

#include "compat.h"
#include "drivers.h"
#include "midi.h"
#include "multivoc.h"
#include "sndcards.h"

int MUSIC_ErrorCode = MUSIC_Ok;

static midifuncs MUSIC_MidiFunctions;

#define MUSIC_SetErrorCode(status) MUSIC_ErrorCode = (status);

const char *MUSIC_ErrorString(int ErrorNumber)
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


int MUSIC_Init(int SoundCard, int EMIDICard /*= -1*/)
{
    int detected = 0;

    if (SoundCard == ASS_AutoDetect)
    {
redetect:
        detected++;
        SoundCard = ASS_OPL3;
    }

    if (SoundCard < 0 || SoundCard >= ASS_NumSoundCards)
    {
failed:
        MV_Printf("failed!\n");
        MUSIC_ErrorCode = MUSIC_MidiError;
        return MUSIC_Error;
    }

    if (!SoundDriver_IsMIDISupported(SoundCard))
    {
        MV_Printf("Couldn't init %s\n", SoundDriver_GetName(SoundCard));

        if (detected < 2)
            goto redetect;

        goto failed;
    }

    ASS_MIDISoundDriver = SoundCard;
    ASS_EMIDICard       = EMIDICard;

    int status = SoundDriver_MIDI_Init(&MUSIC_MidiFunctions);

    if (status != MUSIC_Ok)
    {
        if (detected < 2)
            goto redetect;

        goto failed;
    }

    MV_Printf("%s\n", SoundDriver_GetName(SoundCard));

    MIDI_SetMidiFuncs(&MUSIC_MidiFunctions);

    return MUSIC_Ok;
}


int MUSIC_Shutdown(void)
{
    MIDI_StopSong();

    return MUSIC_Ok;
}


void MUSIC_SetVolume(int volume) { MIDI_SetVolume(min(max(0, volume), 255)); }


int MUSIC_GetVolume(void) { return MIDI_GetVolume(); }
void MUSIC_SetLoopFlag(int loopflag) { MIDI_SetLoopFlag(loopflag); }
void MUSIC_Continue(void) { MIDI_ContinueSong(); }
void MUSIC_Pause(void) { MIDI_PauseSong(); }

int MUSIC_StopSong(void)
{
    MIDI_StopSong();
    MUSIC_SetErrorCode(MUSIC_Ok);
    return MUSIC_Ok;
}


int MUSIC_PlaySong(char *song, int songsize, int loopflag, const char *fn /*= nullptr*/)
{
    UNREFERENCED_PARAMETER(songsize);
    UNREFERENCED_PARAMETER(fn);

    MUSIC_SetErrorCode(MUSIC_Ok)

    if (MIDI_PlaySong(song, loopflag) != MIDI_Ok)
    {
        MUSIC_SetErrorCode(MUSIC_MidiError);
        return MUSIC_Warning;
    }

    return MUSIC_Ok;
}


void MUSIC_Update(void)
{
}
