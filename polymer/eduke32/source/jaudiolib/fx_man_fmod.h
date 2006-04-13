//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
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

Original Source: 1994 - Jim Dose
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jonof@edgenetwk.com)
*/
//-------------------------------------------------------------------------

#ifndef __FX_MAN_H
#define __FX_MAN_H


enum FX_ERRORS
   {
   FX_Warning = -2,
   FX_Error = -1,
   FX_Ok = 0,
   FX_ASSVersion,
   FX_FMODInit
   };


char *FX_ErrorString( int ErrorNumber );
int   FX_Init( int SoundCard, int numvoices, int numchannels, int samplebits, unsigned mixrate );
int   FX_Shutdown( void );
int   FX_SetCallBack( void ( *function )( unsigned long ) );
void  FX_SetVolume( int volume );

void  FX_SetReverseStereo( int setting );
int   FX_GetReverseStereo( void );
void  FX_SetReverb( int reverb );
void  FX_SetReverbDelay( int delay );

int FX_VoiceAvailable( int priority );

int FX_PlayLoopedVOC( char *ptr, long loopstart, long loopend,
       int pitchoffset, int vol, int left, int right, int priority,
       unsigned long callbackval );
int FX_PlayLoopedWAV( char *ptr, long loopstart, long loopend,
       int pitchoffset, int vol, int left, int right, int priority,
       unsigned long callbackval );
int FX_PlayVOC3D( char *ptr, int pitchoffset, int angle, int distance,
       int priority, unsigned long callbackval );
int FX_PlayWAV3D( char *ptr, int pitchoffset, int angle, int distance,
       int priority, unsigned long callbackval );

int FX_Pan3D( int handle, int angle, int distance );
int FX_StopSound( int handle );
int FX_StopAllSounds( void );

int FX_LoadSample(char *ptr, long size, unsigned long number, int priority);
int FX_SampleLoaded(unsigned long number);

int FX_PlayLoopedSound(int,int,unsigned long);
int FX_PlayPositionedSound(int,int,int,unsigned long);

int FX_SimulateCallbacks(void);

#endif
