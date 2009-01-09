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

Modifications for JonoF's port by Jonathon Fowler (jonof@edgenetwk.com)
*/
/**********************************************************************
   file:   MULTIVOC.H

   author: James R. Dose
   date:   December 20, 1993

   Public header for MULTIVOC.C

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifndef __MULTIVOC_H
#define __MULTIVOC_H

#include "compat.h"

#define MV_MinVoiceHandle  1

extern int32_t MV_ErrorCode;

enum MV_Errors
   {
   MV_Warning = -2,
   MV_Error   = -1,
   MV_Ok      = 0,
   MV_UnsupportedCard,
   MV_NotInstalled,
   MV_NoVoices,
   MV_NoMem,
   MV_VoiceNotFound,
   MV_BlasterError,
   MV_DPMI_Error,
   MV_InvalidVOCFile,
   MV_InvalidWAVFile,
   MV_InvalidOGGFile,
   MV_InvalidMixMode,
   MV_IrqFailure,
   MV_DMAFailure,
   MV_DMA16Failure,
   MV_NullRecordFunction
   };

typedef int16_t MONO16;
typedef int8_t  MONO8;

typedef MONO8  VOLUME8[ 256 ];
typedef MONO16 VOLUME16[ 256 ];

char *MV_ErrorString( int32_t ErrorNumber );
int32_t   MV_VoicePlaying( int32_t handle );
int32_t   MV_KillAllVoices( void );
int32_t   MV_Kill( int32_t handle );
int32_t   MV_VoicesPlaying( void );
int32_t   MV_VoiceAvailable( int32_t priority );
int32_t   MV_SetPitch( int32_t handle, int32_t pitchoffset );
int32_t   MV_SetFrequency( int32_t handle, int32_t frequency );
int32_t   MV_EndLooping( int32_t handle );
int32_t   MV_SetPan( int32_t handle, int32_t vol, int32_t left, int32_t right );
int32_t   MV_Pan3D( int32_t handle, int32_t angle, int32_t distance );
void  MV_SetReverb( int32_t reverb );
void  MV_SetFastReverb( int32_t reverb );
int32_t   MV_GetMaxReverbDelay( void );
int32_t   MV_GetReverbDelay( void );
void  MV_SetReverbDelay( int32_t delay );
int32_t   MV_SetMixMode( int32_t numchannels, int32_t samplebits );
int32_t   MV_StartPlayback( void );
void  MV_StopPlayback( void );
int32_t   MV_StartRecording( int32_t MixRate, void ( *function )( char *ptr, int32_t length ) );
void  MV_StopRecord( void );
int32_t   MV_StartDemandFeedPlayback( void ( *function )( char **ptr, uint32_t *length ),
         int32_t rate, int32_t pitchoffset, int32_t vol, int32_t left, int32_t right,
         int32_t priority, uint32_t callbackval );
int32_t   MV_PlayRaw( char *ptr, uint32_t length,
         unsigned rate, int32_t pitchoffset, int32_t vol, int32_t left,
         int32_t right, int32_t priority, uint32_t callbackval );
int32_t   MV_PlayLoopedRaw( char *ptr, int32_t length,
         char *loopstart, char *loopend, unsigned rate, int32_t pitchoffset,
         int32_t vol, int32_t left, int32_t right, int32_t priority,
         uint32_t callbackval );
int32_t   MV_PlayWAV( char *ptr, int32_t pitchoffset, int32_t vol, int32_t left,
         int32_t right, int32_t priority, uint32_t callbackval );
int32_t   MV_PlayWAV3D( char *ptr, int32_t pitchoffset, int32_t angle, int32_t distance,
         int32_t priority, uint32_t callbackval );
int32_t   MV_PlayLoopedWAV( char *ptr, int32_t loopstart, int32_t loopend,
         int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority,
         uint32_t callbackval );
int32_t   MV_PlayOGG( char *ptr, int32_t pitchoffset, int32_t vol, int32_t left,
         int32_t right, int32_t priority, uint32_t callbackval );
int32_t   MV_PlayOGG3D( char *ptr, int32_t pitchoffset, int32_t angle, int32_t distance,
         int32_t priority, uint32_t callbackval );
int32_t   MV_PlayLoopedOGG( char *ptr, int32_t loopstart, int32_t loopend,
         int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority,
         uint32_t callbackval );
int32_t   MV_PlayVOC3D( char *ptr, int32_t pitchoffset, int32_t angle, int32_t distance,
         int32_t priority, uint32_t callbackval );
int32_t   MV_PlayVOC( char *ptr, int32_t pitchoffset, int32_t vol, int32_t left, int32_t right,
         int32_t priority, uint32_t callbackval );
int32_t   MV_PlayLoopedVOC( char *ptr, int32_t loopstart, int32_t loopend,
         int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority,
         uint32_t callbackval );
void  MV_CreateVolumeTable( int32_t index, int32_t volume, int32_t MaxVolume );
void  MV_SetVolume( int32_t volume );
int32_t   MV_GetVolume( void );
void  MV_SetCallBack( void ( *function )( uint32_t ) );
void  MV_SetReverseStereo( int32_t setting );
int32_t   MV_GetReverseStereo( void );
int32_t   MV_Init( int32_t soundcard, int32_t MixRate, int32_t Voices, int32_t numchannels,
         int32_t samplebits );
int32_t   MV_Shutdown( void );

void MV_Update(void);

#endif
