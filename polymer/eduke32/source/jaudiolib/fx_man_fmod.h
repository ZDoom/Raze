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


char *FX_ErrorString( int32_t ErrorNumber );
int32_t   FX_Init( int32_t SoundCard, int32_t numvoices, int32_t numchannels, int32_t samplebits, unsigned mixrate );
int32_t   FX_Shutdown( void );
int32_t   FX_SetCallBack( void ( *function )( uint32_t ) );
void  FX_SetVolume( int32_t volume );

void  FX_SetReverseStereo( int32_t setting );
int32_t   FX_GetReverseStereo( void );
void  FX_SetReverb( int32_t reverb );
void  FX_SetReverbDelay( int32_t delay );

int32_t FX_VoiceAvailable( int32_t priority );

int32_t FX_PlayLoopedVOC( char *ptr, int32_t loopstart, int32_t loopend,
       int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority,
       uint32_t callbackval );
int32_t FX_PlayLoopedWAV( char *ptr, int32_t loopstart, int32_t loopend,
       int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority,
       uint32_t callbackval );
int32_t FX_PlayVOC3D( char *ptr, int32_t pitchoffset, int32_t angle, int32_t distance,
       int32_t priority, uint32_t callbackval );
int32_t FX_PlayWAV3D( char *ptr, int32_t pitchoffset, int32_t angle, int32_t distance,
       int32_t priority, uint32_t callbackval );

int32_t FX_Pan3D( int32_t handle, int32_t angle, int32_t distance );
int32_t FX_StopSound( int32_t handle );
int32_t FX_StopAllSounds( void );

int32_t FX_LoadSample(char *ptr, int32_t size, uint32_t number, int32_t priority);
int32_t FX_SampleLoaded(uint32_t number);

int32_t FX_PlayLoopedSound(int32_t,int32_t,uint32_t);
int32_t FX_PlayPositionedSound(int32_t,int32_t,int32_t,uint32_t);

int32_t FX_SimulateCallbacks(void);

#endif
