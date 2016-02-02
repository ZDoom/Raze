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

#ifndef __MIDI_H
#define __MIDI_H

enum MIDI_Errors
{
    MIDI_Error = -1,
    MIDI_Ok = 0,
    MIDI_NullMidiModule,
    MIDI_InvalidMidiFile,
    MIDI_UnknownMidiFormat,
    MIDI_NoTracks,
    MIDI_InvalidTrack,
};


#define MIDI_PASS_THROUGH 1
#define MIDI_DONT_PLAY    0

#define MIDI_MaxVolume 255

extern char MIDI_PatchMap[ 128 ];

typedef struct
{
    void(*NoteOff)(char channel, char key, char velocity);
    void(*NoteOn)(char channel, char key, char velocity);
    void(*PolyAftertouch)(char channel, char key, char pressure);
    void(*ControlChange)(char channel, char number, char value);
    void(*ProgramChange)(char channel, char program);
    void(*ChannelAftertouch)(char channel, char pressure);
    void(*PitchBend)(char channel, char lsb, char msb);
} midifuncs;

int32_t     MIDI_AllNotesOff(void);
int32_t     MIDI_Reset(void);
int32_t     MIDI_SetVolume(int32_t volume);
void        MIDI_SetMidiFuncs(midifuncs *funcs);
void        MIDI_ContinueSong(void);
void        MIDI_PauseSong(void);
void        MIDI_StopSong(void);
int32_t     MIDI_PlaySong(char *song, int32_t loopflag);
void        MIDI_SetTempo(int32_t tempo);
void        MIDI_UpdateMusic(void);
void        MIDI_SetDivision(int32_t division);

#endif
