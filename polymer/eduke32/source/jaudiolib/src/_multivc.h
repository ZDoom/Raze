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
   file:   _MULTIVC.H

   author: James R. Dose
   date:   December 20, 1993

   Private header for MULTIVOC.C

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifndef ___MULTIVC_H
#define ___MULTIVC_H

#include "limits.h"
#include "inttypes.h"
#include "multivoc.h"

#define VOC_8BIT            0x0
#define VOC_CT4_ADPCM       0x1
#define VOC_CT3_ADPCM       0x2
#define VOC_CT2_ADPCM       0x3
#define VOC_16BIT           0x4
#define VOC_ALAW            0x6
#define VOC_MULAW           0x7
#define VOC_CREATIVE_ADPCM  0x200

#define T_SIXTEENBIT_STEREO 0
#define T_8BITS        1
#define T_MONO         2
#define T_16BITSOURCE  4
#define T_STEREOSOURCE 8
#define T_LEFTQUIET    16
#define T_RIGHTQUIET   32
#define T_DEFAULT      T_SIXTEENBIT_STEREO

#define MV_MAXPANPOSITION  127  /* formerly 31 */
#define MV_NUMPANPOSITIONS ( MV_MAXPANPOSITION + 1 )
#define MV_MAXTOTALVOLUME  255
#define MV_MAXVOLUME       255  /* formerly 63 */
#define MV_NUMVOICES       8

// mirrors FX_MUSIC_PRIORITY from fx_man.h
#define MV_MUSIC_PRIORITY INT_MAX

#define MIX_VOLUME( volume ) \
   ( ( max( 0, min( ( volume ), 255 ) ) * ( MV_MAXVOLUME + 1 ) ) >> 8 )
//   ( ( max( 0, min( ( volume ), 255 ) ) ) >> 2 )

#define STEREO      1
#define SIXTEEN_BIT 2

#define MONO_8BIT    0
#define STEREO_8BIT  ( STEREO )
#define MONO_16BIT   ( SIXTEEN_BIT )
#define STEREO_16BIT ( STEREO | SIXTEEN_BIT )

#define MONO_8BIT_SAMPLE_SIZE    1
#define MONO_16BIT_SAMPLE_SIZE   2
#define STEREO_8BIT_SAMPLE_SIZE  ( 2 * MONO_8BIT_SAMPLE_SIZE )
#define STEREO_16BIT_SAMPLE_SIZE ( 2 * MONO_16BIT_SAMPLE_SIZE )

//#define SILENCE_16BIT     0x80008000
#define SILENCE_16BIT     0
#define SILENCE_8BIT      0x80808080
//#define SILENCE_16BIT_PAS 0

#define MV_MIXBUFFERSIZE     256
#define MV_NUMBEROFBUFFERS   16
#define MV_TOTALBUFFERSIZE   ( MV_MIXBUFFERSIZE * MV_NUMBEROFBUFFERS )

//#define PI                3.1415926536

typedef enum
   {
   NoMoreData,
   KeepPlaying
   } playbackstatus;


typedef struct VoiceNode
   {
   struct VoiceNode *next;
   struct VoiceNode *prev;

   wavedata      wavetype;
   char          bits;
   char          channels;

   playbackstatus ( *GetSound )( struct VoiceNode *voice );

   void ( *mix )( uint32_t position, uint32_t rate,
      const char *start, uint32_t length );

   const uint8_t *rawdataptr;
   const char         *NextBlock;
   const char         *LoopStart;
   const char         *LoopEnd;
   unsigned      LoopCount;
   uint32_t  LoopSize;
   uint32_t  BlockLength;

   int32_t ptrlength;  // ptrlength-1 is the max permissible index for rawdataptr

   uint32_t  PitchScale;
   uint32_t  FixedPointBufferSize;

   const char         *sound;
   uint32_t  length;
   uint32_t  SamplingRate;
   uint32_t  RateScale;
   uint32_t  position;
   int32_t           Playing;
   int32_t           Paused;

   int32_t           handle;
   int32_t           priority;

   void          ( *DemandFeed )( char **ptr, uint32_t *length );
   void         *extra;

   const int16_t        *LeftVolume;
   const int16_t        *RightVolume;

   uint32_t  callbackval;

   } VoiceNode;

typedef struct
   {
   VoiceNode *start;
   VoiceNode *end;
   } VList;

typedef struct
   {
   uint8_t left;
   uint8_t right;
   } Pan;

typedef int16_t MONO16;
typedef int8_t  MONO8;

typedef struct
   {
   MONO16 left;
   MONO16 right;
//   uint16_t left;
//   uint16_t right;
   } STEREO16;

typedef struct
   {
   MONO16 left;
   MONO16 right;
   } SIGNEDSTEREO16;

typedef struct
   {
//   MONO8 left;
//   MONO8 right;
   char left;
   char right;
   } STEREO8;

typedef struct
   {
   char          RIFF[ 4 ];
   uint32_t  file_size;
   char          WAVE[ 4 ];
   char          fmt[ 4 ];
   uint32_t  format_size;
   } riff_header;

typedef struct
   {
   uint16_t wFormatTag;
   uint16_t nChannels;
   uint32_t   nSamplesPerSec;
   uint32_t   nAvgBytesPerSec;
   uint16_t nBlockAlign;
   uint16_t nBitsPerSample;
   } format_header;

typedef struct
   {
   uint8_t DATA[ 4 ];
   uint32_t  size;
   } data_header;

typedef MONO8  VOLUME8[ 256 ];
typedef MONO16 VOLUME16[ 256 ];

extern Pan MV_PanTable[ MV_NUMPANPOSITIONS ][ MV_MAXVOLUME + 1 ];
extern int32_t MV_ErrorCode;
extern int32_t MV_Installed;
extern int32_t MV_MixRate;
typedef char HARSH_CLIP_TABLE_8[ MV_NUMVOICES * 256 ];

#define MV_SetErrorCode( status ) \
   MV_ErrorCode   = ( status );

void MV_PlayVoice( VoiceNode *voice );

VoiceNode *MV_AllocVoice( int32_t priority );

void MV_SetVoiceMixMode( VoiceNode *voice );
void MV_SetVoiceVolume ( VoiceNode *voice, int32_t vol, int32_t left, int32_t right );
void MV_SetVoicePitch  ( VoiceNode *voice, uint32_t rate, int32_t pitchoffset );

void MV_ReleaseVorbisVoice( VoiceNode * voice );
void MV_ReleaseFLACVoice( VoiceNode * voice );

// implemented in mix.c
void ClearBuffer_DW( void *ptr, unsigned data, int32_t length );

void MV_Mix8BitMono( uint32_t position, uint32_t rate,
   const char *start, uint32_t length );

void MV_Mix8BitStereo( uint32_t position,
   uint32_t rate, const char *start, uint32_t length );

void MV_Mix16BitMono( uint32_t position,
   uint32_t rate, const char *start, uint32_t length );

void MV_Mix16BitStereo( uint32_t position,
   uint32_t rate, const char *start, uint32_t length );

void MV_Mix16BitMono16( uint32_t position,
   uint32_t rate, const char *start, uint32_t length );

void MV_Mix8BitMono16( uint32_t position, uint32_t rate,
   const char *start, uint32_t length );

void MV_Mix8BitStereo16( uint32_t position,
   uint32_t rate, const char *start, uint32_t length );

void MV_Mix16BitStereo16( uint32_t position,
   uint32_t rate, const char *start, uint32_t length );

void MV_16BitReverb( char *src, char *dest, VOLUME16 *volume, int32_t count );

void MV_8BitReverb( int8_t *src, int8_t *dest, VOLUME16 *volume, int32_t count );

void MV_16BitReverbFast( char *src, char *dest, int32_t count, int32_t shift );

void MV_8BitReverbFast( int8_t *src, int8_t *dest, int32_t count, int32_t shift );

// implemented in mixst.c
void ClearBuffer_DW( void *ptr, unsigned data, int32_t length );

void MV_Mix8BitMono8Stereo( uint32_t position, uint32_t rate,
							const char *start, uint32_t length );

void MV_Mix8BitStereo8Stereo( uint32_t position,
							  uint32_t rate, const char *start, uint32_t length );

void MV_Mix16BitMono8Stereo( uint32_t position,
							 uint32_t rate, const char *start, uint32_t length );

void MV_Mix16BitStereo8Stereo( uint32_t position,
								uint32_t rate, const char *start, uint32_t length );

void MV_Mix16BitMono16Stereo( uint32_t position,
								uint32_t rate, const char *start, uint32_t length );

void MV_Mix8BitMono16Stereo( uint32_t position, uint32_t rate,
							  const char *start, uint32_t length );

void MV_Mix8BitStereo16Stereo( uint32_t position,
								 uint32_t rate, const char *start, uint32_t length );

void MV_Mix16BitStereo16Stereo( uint32_t position,
								  uint32_t rate, const char *start, uint32_t length );


extern char  *MV_HarshClipTable;
extern char  *MV_MixDestination;			// pointer to the next output sample
extern uint32_t MV_MixPosition;		// return value of where the source pointer got to
extern const int16_t *MV_LeftVolume;
extern const int16_t *MV_RightVolume;
extern int32_t    MV_SampleSize;
extern int32_t    MV_RightChannelOffset;

#define loopStartTagCount 2
extern const char *loopStartTags[loopStartTagCount];
#define loopEndTagCount 2
extern const char *loopEndTags[loopEndTagCount];
#define loopLengthTagCount 2
extern const char *loopLengthTags[loopLengthTagCount];

#if defined __POWERPC__ || defined GEKKO
# define BIGENDIAN
#endif

#endif
