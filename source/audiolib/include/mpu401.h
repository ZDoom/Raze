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

extern int _MPU_CurrentBuffer;
extern int _MPU_BuffersWaiting;

void MPU_SendMidi( char *data, int count );
void MPU_SendMidiImmediate( char *data, int count );
int  MPU_Reset( void );
int  MPU_Init( int addr );
void MPU_NoteOff( int channel, int key, int velocity );
void MPU_NoteOn( int channel, int key, int velocity );
void MPU_PolyAftertouch( int channel, int key, int pressure );
void MPU_ControlChange( int channel, int number, int value );
void MPU_ProgramChange( int channel, int program );
void MPU_ChannelAftertouch( int channel, int pressure );
void MPU_PitchBend( int channel, int lsb, int msb );

void MPU_SetTempo(int tempo);
void MPU_SetDivision(int division);
void MPU_SetVolume(int volume);
int  MPU_GetVolume(void);

void MPU_BeginPlayback( void );
void MPU_Pause(void);
void MPU_Unpause(void);

#endif
