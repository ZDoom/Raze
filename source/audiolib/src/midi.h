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
   module: MIDI.H

   author: James R. Dose
   date:   May 25, 1994

   Public header for MIDI.C.  Midi song file playback routines.

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifndef __MIDI_H
#define __MIDI_H

#include "compat.h"
#include "midifuncs.h"

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
    MIDI_DriverError
};


#define MIDI_PASS_THROUGH 1
#define MIDI_DONT_PLAY    0

#define MIDI_MaxVolume 255

int  MIDI_AllNotesOff(void);
int  MIDI_Reset(void);
int  MIDI_SetVolume(int volume);
int  MIDI_GetVolume(void);
void MIDI_SetMidiFuncs(midifuncs *funcs);
void MIDI_SetLoopFlag(int loopflag);
void MIDI_ContinueSong(void);
void MIDI_PauseSong(void);
void MIDI_StopSong(void);
int  MIDI_PlaySong(char *song, int loopflag);
void MIDI_SetTempo(int tempo);

#endif
