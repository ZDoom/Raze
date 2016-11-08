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

*/
/**********************************************************************
   file:   _MULTIVC.H

   author: James R. Dose
   date:   December 20, 1993

   Private header for MULTIVOC.C

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifndef MULTIVC_H_
#define MULTIVC_H_

#include "limits.h"
#include "inttypes.h"
#include "multivoc.h"

#define VOC_8BIT            0x0
#define VOC_16BIT           0x4

#define T_SIXTEENBIT_STEREO 0
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

#define MIX_VOLUME(volume) ((max(0, min((volume), 255)) * (MV_MAXVOLUME + 1)) >> 8)

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

    playbackstatus (*GetSound)(struct VoiceNode *voice);

    void (*mix)(uint32_t position, uint32_t rate, const char *start, uint32_t length);

    const char *sound;

    const int16_t *LeftVolume;
    const int16_t *RightVolume;

    void *rawdataptr;

    const char *NextBlock;
    const char *LoopStart;
    const char *LoopEnd;

    wavefmt_t wavetype;
    char bits;
    char channels;

    unsigned LoopCount;
    uint32_t LoopSize;
    uint32_t BlockLength;

    int32_t ptrlength;  // ptrlength-1 is the max permissible index for rawdataptr

    uint32_t PitchScale;
    uint32_t FixedPointBufferSize;

    uint32_t length;
    uint32_t SamplingRate;
    uint32_t RateScale;
    uint32_t position;
    int32_t Playing;
    int32_t Paused;

    int32_t handle;
    int32_t priority;

    uint32_t callbackval;
} VoiceNode;

typedef struct
{
    uint8_t left;
    uint8_t right;
} Pan;

typedef struct
{
    char RIFF[4];
    uint32_t file_size;
    char WAVE[4];
    char fmt[4];
    uint32_t format_size;
} riff_header;

typedef struct
{
    uint16_t wFormatTag;
    uint16_t nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t nBlockAlign;
    uint16_t nBitsPerSample;
} format_header;

typedef struct
{
    uint8_t DATA[4];
    uint32_t size;
} data_header;

extern Pan MV_PanTable[ MV_NUMPANPOSITIONS ][ MV_MAXVOLUME + 1 ];
extern int32_t MV_ErrorCode;
extern int32_t MV_Installed;
extern int32_t MV_MixRate;
typedef char HARSH_CLIP_TABLE_8[ MV_NUMVOICES * 256 ];

#define MV_SetErrorCode(status) MV_ErrorCode = (status);

void MV_PlayVoice(VoiceNode *voice);

VoiceNode *MV_AllocVoice(int32_t priority);

void MV_SetVoiceMixMode(VoiceNode *voice);
void MV_SetVoiceVolume(VoiceNode *voice, int32_t vol, int32_t left, int32_t right);
void MV_SetVoicePitch(VoiceNode *voice, uint32_t rate, int32_t pitchoffset);

int32_t MV_GetVorbisPosition(VoiceNode *voice);
void MV_SetVorbisPosition(VoiceNode *voice, int32_t position);
int32_t MV_GetFLACPosition(VoiceNode *voice);
void MV_SetFLACPosition(VoiceNode *voice, int32_t position);
int32_t MV_GetXAPosition(VoiceNode *voice);
void MV_SetXAPosition(VoiceNode *voice, int32_t position);
int32_t MV_GetXMPPosition(VoiceNode *voice);
void MV_SetXMPPosition(VoiceNode *voice, int32_t position);

void MV_ReleaseVorbisVoice(VoiceNode *voice);
void MV_ReleaseFLACVoice(VoiceNode *voice);
void MV_ReleaseXAVoice(VoiceNode *voice);
void MV_ReleaseXMPVoice(VoiceNode *voice);

// implemented in mix.c
void MV_Mix16BitMono(uint32_t position, uint32_t rate, const char *start, uint32_t length);
void MV_Mix16BitStereo(uint32_t position, uint32_t rate, const char *start, uint32_t length);
void MV_Mix16BitMono16(uint32_t position, uint32_t rate, const char *start, uint32_t length);
void MV_Mix16BitStereo16(uint32_t position, uint32_t rate, const char *start, uint32_t length);
void MV_16BitReverb( char const *src, char *dest, int16_t *volume, int32_t count );

// implemented in mixst.c
void MV_Mix16BitMono8Stereo(uint32_t position, uint32_t rate, const char *start, uint32_t length);
void MV_Mix16BitStereo8Stereo(uint32_t position, uint32_t rate, const char *start, uint32_t length);
void MV_Mix16BitMono16Stereo(uint32_t position, uint32_t rate, const char *start, uint32_t length);
void MV_Mix16BitStereo16Stereo(uint32_t position, uint32_t rate, const char *start, uint32_t length);

extern char *MV_MixDestination;  // pointer to the next output sample
extern uint32_t MV_MixPosition;  // return value of where the source pointer got to
extern const int16_t *MV_LeftVolume;
extern const int16_t *MV_RightVolume;
extern int32_t MV_SampleSize;
extern int32_t MV_RightChannelOffset;

#define loopStartTagCount 3
extern const char *loopStartTags[loopStartTagCount];
#define loopEndTagCount 2
extern const char *loopEndTags[loopEndTagCount];
#define loopLengthTagCount 2
extern const char *loopLengthTags[loopLengthTagCount];

#if defined __POWERPC__ || defined GEKKO
# define BIGENDIAN
#endif

#endif
