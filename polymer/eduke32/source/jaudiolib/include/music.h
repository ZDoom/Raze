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
/**********************************************************************
   module: MUSIC.H

   author: James R. Dose
   date:   March 25, 1994

   Public header for MUSIC.C

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifndef __MUSIC_H
#define __MUSIC_H

#include "compat.h"

extern int32_t MUSIC_ErrorCode;

enum MUSIC_ERRORS
   {
   MUSIC_Warning = -2,
   MUSIC_Error   = -1,
   MUSIC_Ok      = 0,
   MUSIC_ASSVersion,
   MUSIC_SoundCardError,
   MUSIC_MPU401Error,
   MUSIC_InvalidCard,
   MUSIC_MidiError,
   MUSIC_TaskManError,
   MUSIC_DPMI_Error
   };

typedef struct
   {
   uint32_t tickposition;
   uint32_t milliseconds;
   uint32_t  measure;
   uint32_t  beat;
   uint32_t  tick;
   } songposition;

#define MUSIC_LoopSong ( 1 == 1 )
#define MUSIC_PlayOnce ( !MUSIC_LoopSong )

const char *MUSIC_ErrorString( int32_t ErrorNumber );
int32_t   MUSIC_Init( int32_t SoundCard, int32_t Address );
int32_t   MUSIC_Shutdown( void );
void  MUSIC_SetVolume( int32_t volume );
void  MUSIC_SetMidiChannelVolume( int32_t channel, int32_t volume );
void  MUSIC_ResetMidiChannelVolumes( void );
int32_t   MUSIC_GetVolume( void );
void  MUSIC_SetLoopFlag( int32_t loopflag );
int32_t   MUSIC_SongPlaying( void );
void  MUSIC_Continue( void );
void  MUSIC_Pause( void );
int32_t   MUSIC_StopSong( void );
int32_t   MUSIC_PlaySong( char *song, int32_t loopflag );
void  MUSIC_SetContext( int32_t context );
int32_t   MUSIC_GetContext( void );
void  MUSIC_SetSongTick( uint32_t PositionInTicks );
void  MUSIC_SetSongTime( uint32_t milliseconds );
void  MUSIC_SetSongPosition( int32_t measure, int32_t beat, int32_t tick );
void  MUSIC_GetSongPosition( songposition *pos );
void  MUSIC_GetSongLength( songposition *pos );
int32_t   MUSIC_FadeVolume( int32_t tovolume, int32_t milliseconds );
int32_t   MUSIC_FadeActive( void );
void  MUSIC_StopFade( void );
void  MUSIC_RerouteMidiChannel( int32_t channel, int32_t ( *function )( int32_t, int32_t, int32_t ) );
void  MUSIC_RegisterTimbreBank( char *timbres );
void  MUSIC_Update(void);

#endif
