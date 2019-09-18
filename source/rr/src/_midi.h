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

#ifndef ___MIDI_H
#define ___MIDI_H
#include "compat.h"

#define RELATIVE_BEAT( measure, beat, tick ) \
   ( ( tick ) + ( ( beat ) << 9 ) + ( ( measure ) << 16 ) )

//Bobby Prince thinks this may be 100
//#define GENMIDI_DefaultVolume 100
#define GENMIDI_DefaultVolume 90

#define MAX_FORMAT            1

#define NUM_MIDI_CHANNELS     16

#define TIME_PRECISION        16

#define MIDI_HEADER_SIGNATURE 0x6468544d    // "MThd"
#define MIDI_TRACK_SIGNATURE  0x6b72544d    // "MTrk"

#define MIDI_VOLUME                7
#define MIDI_PAN                   10
#define MIDI_DETUNE                94
#define MIDI_RHYTHM_CHANNEL        9
#define MIDI_RPN_MSB               100
#define MIDI_RPN_LSB               101
#define MIDI_DATAENTRY_MSB         6
#define MIDI_DATAENTRY_LSB         38
#define MIDI_PITCHBEND_MSB         0
#define MIDI_PITCHBEND_LSB         0
#define MIDI_RUNNING_STATUS        0x80
#define MIDI_NOTE_OFF              0x8
#define MIDI_NOTE_ON               0x9
#define MIDI_POLY_AFTER_TCH        0xA
#define MIDI_CONTROL_CHANGE        0xB
#define MIDI_PROGRAM_CHANGE        0xC
#define MIDI_AFTER_TOUCH           0xD
#define MIDI_PITCH_BEND            0xE
#define MIDI_SPECIAL               0xF
#define MIDI_SYSEX                 0xF0
#define MIDI_SYSEX_CONTINUE        0xF7
#define MIDI_META_EVENT            0xFF
#define MIDI_END_OF_TRACK          0x2F
#define MIDI_TEMPO_CHANGE          0x51
#define MIDI_TIME_SIGNATURE        0x58
#define MIDI_RESET_ALL_CONTROLLERS 0x79
#define MIDI_ALL_NOTES_OFF         0x7b
#define MIDI_MONO_MODE_ON          0x7E
#define MIDI_SYSTEM_RESET          0xFF

#define GET_NEXT_EVENT( track, data ) do { \
    ( data ) = *( track )->pos; \
    ( track )->pos += 1; \
} while (0)

#define GET_MIDI_CHANNEL( event )       ( ( event ) & 0xf )
#define GET_MIDI_COMMAND( event )       ( ( event ) >> 4 )

#define EMIDI_INFINITE          -1
#define EMIDI_END_LOOP_VALUE    127
#define EMIDI_ALL_CARDS         127
#define EMIDI_INCLUDE_TRACK     110
#define EMIDI_EXCLUDE_TRACK     111
#define EMIDI_PROGRAM_CHANGE    112
#define EMIDI_VOLUME_CHANGE     113
#define EMIDI_CONTEXT_START     114
#define EMIDI_CONTEXT_END       115
#define EMIDI_LOOP_START        116
#define EMIDI_LOOP_END          117
#define EMIDI_SONG_LOOP_START   118
#define EMIDI_SONG_LOOP_END     119

#define EMIDI_GeneralMIDI       0

#define EMIDI_AffectsCurrentCard(c, type) (((c) == EMIDI_ALL_CARDS) || ((c) == (type)))
#define EMIDI_NUM_CONTEXTS      7

typedef struct
{
    char *pos;
    char *loopstart;
    int16_t loopcount;
    int16_t RunningStatus;
    unsigned time;
    int32_t FPSecondsPerTick;
    int16_t tick;
    int16_t beat;
    int16_t measure;
    int16_t BeatsPerMeasure;
    int16_t TicksPerBeat;
    int16_t TimeBase;
    int32_t delay;
    int16_t active;
} songcontext;

typedef struct
{
    char *start;
    char *pos;

    int32_t delay;
    int16_t active;
    int16_t RunningStatus;

    int16_t currentcontext;
    songcontext context[EMIDI_NUM_CONTEXTS];

    char EMIDI_IncludeTrack;
    char EMIDI_ProgramChange;
    char EMIDI_VolumeChange;
} track;

static int32_t _MIDI_ReadNumber(void *from, size_t size);
static int32_t _MIDI_ReadDelta(track *ptr);
static void _MIDI_ResetTracks(void);
static void _MIDI_AdvanceTick(void);
static void _MIDI_MetaEvent(track *Track);
static void _MIDI_SysEx(track *Track);
static int32_t _MIDI_InterpretControllerInfo(track *Track, int32_t TimeSet, int32_t channel, int32_t c1, int32_t c2);
static int32_t _MIDI_SendControlChange(int32_t channel, int32_t c1, int32_t c2);
static void _MIDI_SetChannelVolume(int32_t channel, int32_t volume);
static void _MIDI_SendChannelVolumes(void);
static void _MIDI_InitEMIDI(void);

#endif
