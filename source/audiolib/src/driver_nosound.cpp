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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
 */

/**
 * Stub driver for no output
 */

#include "compat.h"
#include "midifuncs.h"

int NoSoundDrv_GetError(void) { return 0; }

const char *NoSoundDrv_ErrorString(int ErrorNumber)
{
    UNREFERENCED_PARAMETER(ErrorNumber);
    return "No sound, Ok.";
}

int NoSoundDrv_PCM_Init(int *mixrate, int *numchannels, void *initdata)
{
    UNREFERENCED_PARAMETER(mixrate);
    UNREFERENCED_PARAMETER(numchannels);
    UNREFERENCED_PARAMETER(initdata);
    return 0;
}

void NoSoundDrv_PCM_Shutdown(void) {}

int NoSoundDrv_PCM_BeginPlayback(char *BufferStart, int BufferSize, int NumDivisions, void (*CallBackFunc)(void))
{
    UNREFERENCED_PARAMETER(BufferStart);
    UNREFERENCED_PARAMETER(BufferSize);
    UNREFERENCED_PARAMETER(NumDivisions);
    UNREFERENCED_PARAMETER(CallBackFunc);
    return 0;
}

void NoSoundDrv_PCM_StopPlayback(void) {}
void NoSoundDrv_PCM_Lock(void) {}
void NoSoundDrv_PCM_Unlock(void) {}

int NoSoundDrv_MIDI_Init(midifuncs *funcs)
{
    Bmemset(funcs, 0, sizeof(midifuncs));
    return 0;
}

void NoSoundDrv_MIDI_Shutdown(void) {}

int NoSoundDrv_MIDI_StartPlayback(void (*service)(void))
{
    UNREFERENCED_PARAMETER(service);
    return 0;
}

void         NoSoundDrv_MIDI_HaltPlayback(void) {}
uint32_t NoSoundDrv_MIDI_GetTick(void) { return 0; }

void NoSoundDrv_MIDI_SetTempo(int tempo, int division)
{
    UNREFERENCED_PARAMETER(tempo);
    UNREFERENCED_PARAMETER(division);
}

void NoSoundDrv_MIDI_Lock(void) {}
void NoSoundDrv_MIDI_Unlock(void) {}
