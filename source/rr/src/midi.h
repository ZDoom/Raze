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
   MIDI_Warning = -2,
   MIDI_Error   = -1,
   MIDI_Ok      = 0,
   MIDI_NullMidiModule,
   MIDI_InvalidMidiFile,
   MIDI_UnknownMidiFormat,
   MIDI_NoTracks,
   MIDI_InvalidTrack,
   MIDI_NoMemory,
   MIDI_DPMI_Error
   };


#define MIDI_PASS_THROUGH 1
#define MIDI_DONT_PLAY    0

#define MIDI_MaxVolume 255

extern char MIDI_PatchMap[ 128 ];

typedef struct
{
    void (*NoteOff)(int32_t channel, int32_t key, int32_t velocity);
    void (*NoteOn)(int32_t channel, int32_t key, int32_t velocity);
    void (*PolyAftertouch)(int32_t channel, int32_t key, int32_t pressure);
    void (*ControlChange)(int32_t channel, int32_t number, int32_t value);
    void (*ProgramChange)(int32_t channel, int32_t program);
    void (*ChannelAftertouch)(int32_t channel, int32_t pressure);
    void (*PitchBend)(int32_t channel, int32_t lsb, int32_t msb);
    void (*FinishBuffer)(void);
} midifuncs;

void MIDI_RerouteMidiChannel( int32_t channel, int32_t ( *function )( int32_t event, int32_t c1, int32_t c2 ) );
int32_t  MIDI_AllNotesOff( void );
void MIDI_SetUserChannelVolume( int32_t channel, int32_t volume );
void MIDI_ResetUserChannelVolume( void );
int32_t  MIDI_Reset( void );
int32_t  MIDI_SetVolume( int32_t volume );
int32_t  MIDI_GetVolume( void );
void MIDI_SetMidiFuncs( midifuncs *funcs );
void MIDI_SetContext( int32_t context );
int32_t  MIDI_GetContext( void );
void MIDI_SetLoopFlag( int32_t loopflag );
void MIDI_ContinueSong( void );
void MIDI_PauseSong( void );
int32_t  MIDI_SongPlaying( void );
void MIDI_StopSong( void );
int32_t  MIDI_PlaySong( char *song, int32_t loopflag );
void MIDI_SetTempo( int32_t tempo );
int32_t  MIDI_GetTempo( void );
void MIDI_SetSongTick( uint32_t PositionInTicks );
void MIDI_SetSongTime( uint32_t milliseconds );
void MIDI_SetSongPosition( int32_t measure, int32_t beat, int32_t tick );
void MIDI_GetSongPosition( songposition *pos );
void MIDI_GetSongLength( songposition *pos );
void MIDI_LoadTimbres( void );
void MIDI_UpdateMusic(void);
void MIDI_SetDivision( int32_t division );

#endif
