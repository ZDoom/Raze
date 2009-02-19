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
   file:   _MULTIVC.H

   author: James R. Dose
   date:   December 20, 1993

   Private header for MULTIVOC.C

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifndef ___MULTIVC_H
#define ___MULTIVC_H

#include "openal.h"

#ifndef TRUE
#define TRUE  ( 1 == 1 )
#define FALSE ( !TRUE )
#endif

#define VOC_8BIT            0x0
#define VOC_CT4_ADPCM       0x1
#define VOC_CT3_ADPCM       0x2
#define VOC_CT2_ADPCM       0x3
#define VOC_16BIT           0x4
#define VOC_ALAW            0x6
#define VOC_MULAW           0x7
#define VOC_CREATIVE_ADPCM  0x200

#define T_SIXTEENBIT_STEREO 0
#define T_8BITS       1
#define T_MONO        2
#define T_16BITSOURCE 4
#define T_LEFTQUIET   8
#define T_RIGHTQUIET  16
#define T_DEFAULT     T_SIXTEENBIT_STEREO

#define MV_MaxPanPosition  31
#define MV_NumPanPositions ( MV_MaxPanPosition + 1 )
#define MV_MaxTotalVolume  255
//#define MV_MaxVolume       63
#define MV_NumVoices       8

#define MIX_VOLUME( volume ) \
   ( ( max( 0, min( ( volume ), 255 ) ) * ( MV_MaxVolume + 1 ) ) >> 8 )
//   ( ( max( 0, min( ( volume ), 255 ) ) ) >> 2 )

//#define SILENCE_16BIT     0x80008000
#define SILENCE_16BIT     0
#define SILENCE_8BIT      0x80808080
//#define SILENCE_16BIT_PAS 0

#if defined(_WIN32)
#define MixBufferSize     (MV_GetBufferSize(MV_RequestedMixRate))
#else
#define MixBufferSize     (512)
#endif

#define NumberOfBuffers   16
#define TotalBufferSize   ( MixBufferSize * NumberOfBuffers )

#define PI                3.14159265358979323

typedef enum
{
    Raw,
    VOC,
    DemandFeed,
    WAV,
    OGG
} wavedata;

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

    playbackstatus(*GetSound)(struct VoiceNode *voice);

    void (*mix)(uint32_t position, uint32_t rate,
                char *start, uint32_t length);

    char         *NextBlock;
    char         *LoopStart;
    char         *LoopEnd;
    unsigned      LoopCount;
    uint32_t LoopSize;
    uint32_t BlockLength;

    uint32_t PitchScale;
    uint32_t FixedPointBufferSize;

    char         *sound;
    uint32_t length;
    uint32_t SamplingRate;
    uint32_t RateScale;
    uint32_t position;
    int32_t           Playing;

    int32_t           handle;
    int32_t           priority;

    void (*DemandFeed)(char **ptr, uint32_t *length);

    sounddef OGGstream;
//   char         *bufsnd;
    char         bufsnd[0x8000*4];
    int32_t          downsample;

    int16_t        *LeftVolume;
    int16_t        *RightVolume;

    uint32_t callbackval;

} VoiceNode;

typedef struct
{
    VoiceNode *start;
    VoiceNode *end;
} VList;

typedef struct
{
    char left;
    char right;
} Pan;

typedef struct
{
    MONO16 left;
    MONO16 right;
} STEREO16;

typedef struct
{
    MONO16 left;
    MONO16 right;
} SIGNEDSTEREO16;

typedef struct
{
    char left;
    char right;
} STEREO8;

typedef struct
{
    char          RIFF[ 4 ];
    uint32_t file_size;
    char          WAVE[ 4 ];
    char          fmt[ 4 ];
    uint32_t format_size;
} riff_header;

typedef struct
{
    uint16_t wFormatTag;
    uint16_t nChannels;
    uint32_t  nSamplesPerSec;
    uint32_t  nAvgBytesPerSec;
    uint16_t nBlockAlign;
    uint16_t nBitsPerSample;
} format_header;

typedef struct
{
    char DATA[ 4 ];
    uint32_t size;
} data_header;

typedef char HARSH_CLIP_TABLE_8[ MV_NumVoices * 256 ];

#if defined(_WIN32)
static uint32_t MV_GetBufferSize(unsigned);
#endif
static void MV_Mix(VoiceNode *voice, int32_t buffer);
static void MV_PlayVoice(VoiceNode *voice);
static void MV_StopVoice(VoiceNode *voice);
static int32_t  MV_ServiceVoc(int32_t);

static playbackstatus MV_GetNextVOCBlock(VoiceNode *voice);
static playbackstatus MV_GetNextDemandFeedBlock(VoiceNode *voice);
static playbackstatus MV_GetNextRawBlock(VoiceNode *voice);
static playbackstatus MV_GetNextWAVBlock(VoiceNode *voice);

// static void       MV_ServiceRecord( void );
static VoiceNode *MV_GetVoice(int32_t handle);
static VoiceNode *MV_AllocVoice(int32_t priority);

static int16_t     *MV_GetVolumeTable(int32_t vol);

static void       MV_SetVoiceMixMode(VoiceNode *voice);

static void       MV_SetVoicePitch(VoiceNode *voice, uint32_t rate, int32_t pitchoffset);
static void       MV_CalcVolume(int32_t MaxLevel);
static void       MV_CalcPanTable(void);

static void ClearBuffer_DW(void *ptr, int32_t data, int32_t length);

/*
#define ClearBuffer_DW( ptr, data, length ) \
	({ void *__ptr=(ptr); unsigned __data=(data); int32_t __length=(length); \
	__asm__ __volatile__ ("rep; stosl" \
		: "+c" (__length), "+D" (__ptr) : "a" (__data) : "memory", "cc"); \
	0; })
*/
/*
#pragma aux ClearBuffer_DW = \
   "cld",                    \
   "push   es",              \
   "push   ds",              \
   "pop    es",              \
   "rep    stosd",           \
   "pop    es",              \
parm [ edi ] [ eax ] [ ecx ] modify exact [ ecx edi ];
*/

#if defined(__WATCOMC__)

#pragma aux MV_Mix8BitMono parm [eax] [edx] [ebx] [ecx]
#pragma aux MV_Mix8BitStereo parm [eax] [edx] [ebx] [ecx]
#pragma aux MV_Mix16BitMono parm [eax] [edx] [ebx] [ecx]
#pragma aux MV_Mix16BitStereo parm [eax] [edx] [ebx] [ecx]
#pragma aux MV_Mix16BitMono16 parm [eax] [edx] [ebx] [ecx]
#pragma aux MV_Mix8BitMono16 parm [eax] [edx] [ebx] [ecx]
#pragma aux MV_Mix8BitStereo16 parm [eax] [edx] [ebx] [ecx]
#pragma aux MV_Mix16BitStereo16 parm [eax] [edx] [ebx] [ecx]

#pragma aux MV_16BitReverb parm [eax] [edx] [ebx] [ecx] modify exact [eax ebx ecx edx esi edi]
#pragma aux MV_8BitReverb parm [eax] [edx] [ebx] [ecx] modify exact [eax ebx ecx edx esi edi]
#pragma aux MV_16BitReverbFast parm [eax] [edx] [ebx] [ecx] modify exact [eax ebx ecx edx esi edi]
#pragma aux MV_8BitReverbFast parm [eax] [edx] [ebx] [ecx] modify exact [eax ebx ecx edx esi edi]

#define CDEC

#elif defined(_MSC_VER)

#define CDEC _cdecl

#elif defined(_WIN32)

#define CDEC __cdecl

#else

#define CDEC

#endif


void CDEC MV_Mix8BitMono(uint32_t position, uint32_t rate,
                         char *start, uint32_t length);

void CDEC MV_Mix8BitStereo(uint32_t position,
                           uint32_t rate, char *start, uint32_t length);

void CDEC MV_Mix16BitMono(uint32_t position,
                          uint32_t rate, char *start, uint32_t length);

void CDEC MV_Mix16BitStereo(uint32_t position,
                            uint32_t rate, char *start, uint32_t length);

void CDEC MV_Mix16BitMono16(uint32_t position,
                            uint32_t rate, char *start, uint32_t length);

void CDEC MV_Mix8BitMono16(uint32_t position, uint32_t rate,
                           char *start, uint32_t length);

void CDEC MV_Mix8BitStereo16(uint32_t position,
                             uint32_t rate, char *start, uint32_t length);

void CDEC MV_Mix16BitStereo16(uint32_t position,
                              uint32_t rate, char *start, uint32_t length);

void CDEC MV_16BitReverb(char *src, char *dest, VOLUME16 *volume, int32_t count);
void CDEC MV_8BitReverb(char *src, char *dest, VOLUME16 *volume, int32_t count);
void CDEC MV_16BitReverbFast(char *src, char *dest, int32_t count, int32_t shift);
void CDEC MV_8BitReverbFast(char *src, char *dest, int32_t count, int32_t shift);

#undef CDEC

#endif
