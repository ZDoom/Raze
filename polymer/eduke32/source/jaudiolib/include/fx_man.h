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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/**********************************************************************
   module: FX_MAN.H

   author: James R. Dose
   date:   March 17, 1994

   Public header for FX_MAN.C

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifndef __FX_MAN_H
#define __FX_MAN_H

#include "inttypes.h"
#include "limits.h"
#include "sndcards.h"

enum FX_ERRORS
   {
   FX_Warning = -2,
   FX_Error = -1,
   FX_Ok = 0,
   FX_ASSVersion,
   FX_SoundCardError,
   FX_InvalidCard,
   FX_MultiVocError,
   };

#define FX_MUSIC_PRIORITY	INT_MAX

const char *FX_ErrorString( int32_t ErrorNumber );
int32_t   FX_Init( int32_t SoundCard, int32_t numvoices, int32_t numchannels, int32_t samplebits, unsigned mixrate, void * initdata );
int32_t   FX_Shutdown( void );
int32_t   FX_SetCallBack( void ( *function )( uint32_t ) );
void  FX_SetVolume( int32_t volume );
int32_t   FX_GetVolume( void );

void  FX_SetReverseStereo( int32_t setting );
int32_t   FX_GetReverseStereo( void );
void  FX_SetReverb( int32_t reverb );
void  FX_SetFastReverb( int32_t reverb );
int32_t   FX_GetMaxReverbDelay( void );
int32_t   FX_GetReverbDelay( void );
void  FX_SetReverbDelay( int32_t delay );

int32_t FX_PauseVoice ( int32_t handle, int32_t pause );
int32_t FX_VoiceAvailable( int32_t priority );
int32_t FX_EndLooping( int32_t handle );
int32_t FX_SetPan( int32_t handle, int32_t vol, int32_t left, int32_t right );
int32_t FX_SetPitch( int32_t handle, int32_t pitchoffset );
int32_t FX_SetFrequency( int32_t handle, int32_t frequency );

int32_t FX_PlayVOC( char *ptr, uint32_t ptrlength, int32_t pitchoffset, int32_t vol, int32_t left, int32_t right,
       int32_t priority, uint32_t callbackval );
int32_t FX_PlayLoopedVOC( char *ptr, uint32_t ptrlength, int32_t loopstart, int32_t loopend,
       int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority,
       uint32_t callbackval );
int32_t FX_PlayWAV( char *ptr, uint32_t ptrlength, int32_t pitchoffset, int32_t vol, int32_t left, int32_t right,
       int32_t priority, uint32_t callbackval );
int32_t FX_PlayLoopedWAV( char *ptr, uint32_t ptrlength, int32_t loopstart, int32_t loopend,
       int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority,
       uint32_t callbackval );
int32_t FX_PlayVOC3D( char *ptr, uint32_t ptrlength, int32_t pitchoffset, int32_t angle, int32_t distance,
       int32_t priority, uint32_t callbackval );
int32_t FX_PlayWAV3D( char *ptr, uint32_t ptrlength, int32_t pitchoffset, int32_t angle, int32_t distance,
       int32_t priority, uint32_t callbackval );

int32_t FX_PlayAuto( char *ptr, uint32_t ptrlength, int32_t pitchoffset, int32_t vol, int32_t left, int32_t right,
                int32_t priority, uint32_t callbackval );
int32_t FX_PlayLoopedAuto( char *ptr, uint32_t ptrlength, int32_t loopstart, int32_t loopend,
                      int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority,
                      uint32_t callbackval );
int32_t FX_PlayAuto3D( char *ptr, uint32_t ptrlength, int32_t pitchoffset, int32_t angle, int32_t distance,
                  int32_t priority, uint32_t callbackval );

int32_t FX_PlayRaw( char *ptr, uint32_t length, unsigned rate,
       int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority,
       uint32_t callbackval );
int32_t FX_PlayLoopedRaw( char *ptr, uint32_t length, char *loopstart,
       char *loopend, unsigned rate, int32_t pitchoffset, int32_t vol, int32_t left,
       int32_t right, int32_t priority, uint32_t callbackval );
int32_t FX_Pan3D( int32_t handle, int32_t angle, int32_t distance );
int32_t FX_SoundActive( int32_t handle );
int32_t FX_SoundsPlaying( void );
int32_t FX_StopSound( int32_t handle );
int32_t FX_StopAllSounds( void );
int32_t FX_StartDemandFeedPlayback( void ( *function )( char **ptr, uint32_t *length ),
       int32_t rate, int32_t pitchoffset, int32_t vol, int32_t left, int32_t right,
       int32_t priority, uint32_t callbackval );

int32_t FX_SetVoiceCallback(int32_t handle, uint32_t callbackval);
int32_t FX_SetPrintf(void (*function)(const char *, ...));

#endif
