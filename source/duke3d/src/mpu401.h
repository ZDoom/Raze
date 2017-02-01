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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
#ifndef __MPU401_H
#define __MPU401_H

#include "compat.h"

#define MPU_DefaultAddress 0x330

enum MPU_ERRORS
   {
   MPU_Warning = -2,
   MPU_Error = -1,
   MPU_Ok = 0
   };

#define MPU_NotFound       -1
#define MPU_UARTFailed     -2

#define MPU_ReadyToWrite   0x40
#define MPU_ReadyToRead    0x80
#define MPU_CmdEnterUART   0x3f
#define MPU_CmdReset       0xff
#define MPU_CmdAcknowledge 0xfe

extern int32_t _MPU_CurrentBuffer;
extern int32_t _MPU_BuffersWaiting;

void MPU_SendMidi( char *data, int32_t count );
void MPU_SendMidiImmediate( char *data, int32_t count );
int32_t  MPU_Reset( void );
int32_t  MPU_Init( int32_t addr );
void MPU_NoteOff( int32_t channel, int32_t key, int32_t velocity );
void MPU_NoteOn( int32_t channel, int32_t key, int32_t velocity );
void MPU_PolyAftertouch( int32_t channel, int32_t key, int32_t pressure );
void MPU_ControlChange( int32_t channel, int32_t number, int32_t value );
void MPU_ProgramChange( int32_t channel, int32_t program );
void MPU_ChannelAftertouch( int32_t channel, int32_t pressure );
void MPU_PitchBend( int32_t channel, int32_t lsb, int32_t msb );

void MPU_SetTempo(int32_t tempo);
void MPU_SetDivision(int32_t division);
void MPU_SetVolume(int32_t volume);
int32_t  MPU_GetVolume(void);

void MPU_BeginPlayback( void );
void MPU_Pause(void);
void MPU_Unpause(void);

#endif
