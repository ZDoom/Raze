/*
 Copyright (C) 2009 Jonathon Fowler <jf@jonof.id.au>

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 */

#ifndef DRIVERS_H
#define DRIVERS_H

#include "sndcards.h"
#include "midifuncs.h"

extern int ASS_PCMSoundDriver;
extern int ASS_MIDISoundDriver;
extern int ASS_EMIDICard;

int SoundDriver_IsPCMSupported(int driver);
int SoundDriver_IsMIDISupported(int driver);

const char *SoundDriver_GetName(int driver);

int         SoundDriver_PCM_GetError(void);
const char *SoundDriver_PCM_ErrorString(int ErrorNumber);
int         SoundDriver_MIDI_GetError(void);
const char *SoundDriver_MIDI_ErrorString(int ErrorNumber);

int  SoundDriver_PCM_Init(int *mixrate, int *numchannels, void *initdata);
void SoundDriver_PCM_Shutdown(void);
int  SoundDriver_PCM_BeginPlayback(char *BufferStart, int BufferSize, int NumDivisions, void (*CallBackFunc)(void));
void SoundDriver_PCM_StopPlayback(void);
void SoundDriver_PCM_Lock(void);
void SoundDriver_PCM_Unlock(void);

int  SoundDriver_MIDI_Init(midifuncs *);
void SoundDriver_MIDI_Shutdown(void);
int  SoundDriver_MIDI_StartPlayback(void (*service)(void));
void SoundDriver_MIDI_HaltPlayback(void);
void SoundDriver_MIDI_SetTempo(int tempo, int division);
void SoundDriver_MIDI_Lock(void);
void SoundDriver_MIDI_Unlock(void);

#endif
