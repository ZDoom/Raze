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

#include "midifuncs.h"

int AdlibDrv_GetError(void);
const char *AdlibDrv_ErrorString( int ErrorNumber );

int  AdlibDrv_MIDI_Init(midifuncs *);
void AdlibDrv_MIDI_Shutdown(void);
int  AdlibDrv_MIDI_StartPlayback(void (*service)(void));
void AdlibDrv_MIDI_HaltPlayback(void);
void AdlibDrv_MIDI_SetTempo(int tempo, int division);
void AdlibDrv_MIDI_Lock(void);
void AdlibDrv_MIDI_Unlock(void);
