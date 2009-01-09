/*
Copyright (C) 2003-2004 Ryan C. Gordon. and James Bentler

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

Originally written by Ryan C. Gordon. (icculus@clutteredmind.org)
Adapted to work with JonoF's port by James Bentler (bentler@cs.umn.edu)

*/
#ifndef AUDIOLIB__DSL_H
#define AUDIOLIB__DSL_H

#define MONO_8BIT    0
#define STEREO      1
#define SIXTEEN_BIT 2

enum DSL_ERRORS
   {
   DSL_Warning = -2,
   DSL_Error = -1,
   DSL_Ok = 0,
   DSL_SDLInitFailure,
   DSL_MixerActive,
   DSL_MixerInitFailure
   };

extern int32_t DSL_ErrorCode;
char *DSL_ErrorString( int32_t ErrorNumber );

int32_t DisableInterrupts(void);	// simulated using critical sections
int32_t RestoreInterrupts(int32_t);

int32_t   DSL_Init(int32_t soundcard, int32_t mixrate, int32_t numchannels, int32_t samplebits, int32_t buffersize);
void  DSL_StopPlayback( void );
unsigned DSL_GetPlaybackRate( void );
int32_t   DSL_BeginBufferedPlayback(char *BufferStart, int32_t (*CallBackFunc)(int32_t), int32_t buffersize, int32_t numdivisions);
void  DSL_Shutdown( void );

#endif
