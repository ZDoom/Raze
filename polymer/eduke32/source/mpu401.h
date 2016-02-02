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

#ifndef __MPU401_H
#define __MPU401_H

#include "compat.h"

enum MPU_ERRORS
{
    MPU_Error = -1,
    MPU_Ok = 0
};

extern int32_t _MPU_BuffersWaiting;

void MPU_SendMidi( char *data, int32_t count );
int32_t  MPU_Reset( void );
int32_t  MPU_Init( int32_t addr );
void MPU_NoteOff( char channel, char key, char velocity );
void MPU_NoteOn( char channel, char key, char velocity );
void MPU_PolyAftertouch( char channel, char key, char pressure );
void MPU_ControlChange( char channel, char number, char value );
void MPU_ProgramChange( char channel, char program );
void MPU_ChannelAftertouch( char channel, char pressure );
void MPU_PitchBend( char channel, char lsb, char msb );

void MPU_SetTempo(int32_t tempo);
void MPU_SetDivision(int32_t division);

void MPU_BeginPlayback( void );
void MPU_Pause(void);
void MPU_Unpause(void);

#endif
