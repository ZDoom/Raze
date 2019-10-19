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

#include "al_midi.h"
#include "midifuncs.h"
#include "opl3.h"

extern int AL_Stereo;

int         AdLibDrv_GetError(void);
const char *AdLibDrv_ErrorString(int ErrorNumber);

int  AdLibDrv_MIDI_Init(midifuncs *);
void AdLibDrv_MIDI_Shutdown(void);
int  AdLibDrv_MIDI_StartPlayback(void (*service)(void));
void AdLibDrv_MIDI_HaltPlayback(void);
void AdLibDrv_MIDI_SetTempo(int tempo, int division);

