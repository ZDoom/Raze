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
   module: MULTIVOC.C

   author: James R. Dose
   date:   December 20, 1993

   Routines to provide multichannel digitized sound playback for
   Sound Blaster compatible sound cards.

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <duke3d.h>
#ifdef _WIN32
#include "dsoundout.h"
#ifdef USE_OPENAL
#include "openal.h"
#endif
#else
#include "dsl.h"
#endif
#include "compat.h"
#include "baselayer.h"
#include "linklist.h"
#include "pitch.h"
#include "multivoc.h"
#include "_multivc.h"

#define STEREO      1
#define SIXTEEN_BIT 2

#define MONO_8BIT    0
#define STEREO_8BIT  (STEREO)
#define MONO_16BIT   (SIXTEEN_BIT)
#define STEREO_16BIT (STEREO | SIXTEEN_BIT)

#define IS_QUIET(ptr)  ((void *)(ptr) == (void *)&MV_VolumeTable[ 0 ])

static int32_t MV_ReverbLevel;
static int32_t MV_ReverbDelay;
static VOLUME16 *MV_ReverbTable = NULL;

//static signed short MV_VolumeTable[ MV_MaxVolume + 1 ][ 256 ];
static int16_t MV_VolumeTable[ 63 + 1 ][ 256 ];

//static Pan MV_PanTable[ MV_NumPanPositions ][ MV_MaxVolume + 1 ];
static Pan MV_PanTable[ MV_NumPanPositions ][ 63 + 1 ];

static int32_t MV_Installed   = FALSE;
static int32_t MV_SoundCard   = -1;
static int32_t MV_TotalVolume = MV_MaxTotalVolume;
static int32_t MV_MaxVoices   = 1;
static int32_t MV_Recording;

static int32_t MV_BufferSize = 0;
static int32_t MV_BufferLength;

static int32_t MV_NumberOfBuffers = NumberOfBuffers;

static int32_t MV_MixMode    = MONO_8BIT;
static int32_t MV_Channels   = 1;
static int32_t MV_Bits       = 8;

static int32_t MV_Silence    = SILENCE_8BIT;
static int32_t MV_SwapLeftRight = FALSE;

static int32_t MV_RequestedMixRate;
static int32_t MV_MixRate;

static int32_t MV_BuffShift;

static int32_t MV_TotalMemory;

static int32_t MV_BufferEmpty[ NumberOfBuffers ];
char *MV_MixBuffer[ NumberOfBuffers + 1 ];
static char *MV_MixBufferPtr = NULL;

static VoiceNode *MV_Voices = NULL;

static VoiceNode VoiceList;
static VoiceNode VoicePool;

int32_t MV_MixPage      = 0;
static int32_t MV_VoiceHandle  = MV_MinVoiceHandle;

static void(*MV_CallBackFunc)(uint32_t) = NULL;
static void(*MV_RecordFunc)(char *ptr, int32_t length) = NULL;
static void(*MV_MixFunction)(VoiceNode *voice, int32_t buffer);

static int32_t MV_MaxVolume = 63;

char  *MV_HarshClipTable;
char  *MV_MixDestination;
int16_t *MV_LeftVolume;
int16_t *MV_RightVolume;
int32_t MV_SampleSize = 1;
int32_t MV_RightChannelOffset;

uint32_t MV_MixPosition;

int32_t MV_ErrorCode = MV_Ok;

#define MV_SetErrorCode(status) \
   MV_ErrorCode   = (status);


/*---------------------------------------------------------------------
   Function: ClearBuffer_DW

   Function code relocated from _multivc.h due to linking issues.
---------------------------------------------------------------------*/

void ClearBuffer_DW(void *ptr, int32_t data, int32_t length)
{
    int32_t *pptr = ptr;
    for (; length>0; length--) *(pptr++) = data;
}


/*---------------------------------------------------------------------
   Function: MV_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/

char *MV_ErrorString(int32_t ErrorNumber)
{
    char *ErrorString;

    switch (ErrorNumber)
    {
    case MV_Warning :
    case MV_Error :
        ErrorString = MV_ErrorString(MV_ErrorCode);
        break;

    case MV_Ok :
        ErrorString = "Multivoc ok.";
        break;

    case MV_UnsupportedCard :
        ErrorString = "Selected sound card is not supported by Multivoc.";
        break;

    case MV_NotInstalled :
        ErrorString = "Multivoc not installed.";
        break;

    case MV_NoVoices :
        ErrorString = "No free voices available to Multivoc.";
        break;

    case MV_NoMem :
        ErrorString = "Out of memory in Multivoc.";
        break;

    case MV_VoiceNotFound :
        ErrorString = "No voice with matching handle found.";
        break;

    case MV_BlasterError :
#if defined(_WIN32)
        ErrorString = DSOUND_ErrorString(DSOUND_ErrorCode);
#else
        ErrorString = DSL_ErrorString(DSL_ErrorCode);
#endif
        break;


    case MV_DPMI_Error :
        ErrorString = "DPMI Error in Multivoc.";
        break;

    case MV_InvalidVOCFile :
        ErrorString = "Invalid VOC file passed in to Multivoc.";
        break;

    case MV_InvalidWAVFile :
        ErrorString = "Invalid WAV file passed in to Multivoc.";
        break;
    case MV_InvalidOGGFile :
        ErrorString = "Invalid OGG file passed in to Multivoc.";
        break;
    case MV_InvalidMixMode :
        ErrorString = "Invalid mix mode request in Multivoc.";
        break;

    case MV_IrqFailure :
        ErrorString = "Playback failed, possibly due to an invalid or conflicting IRQ.";
        break;

    case MV_DMAFailure :
        ErrorString = "Playback failed, possibly due to an invalid or conflicting DMA channel.";
        break;

    case MV_DMA16Failure :
        ErrorString = "Playback failed, possibly due to an invalid or conflicting DMA channel.  \n"
                      "Make sure the 16-bit DMA channel is correct.";
        break;

    case MV_NullRecordFunction :
        ErrorString = "Null record function passed to MV_StartRecording.";
        break;

    default :
        ErrorString = "Unknown Multivoc error code.";
        break;
    }

    return(ErrorString);
}


/**********************************************************************

   Memory locked functions:

**********************************************************************/


/*---------------------------------------------------------------------
   Function: MV_GetBufferSize

   Returns the buffer size for the given samplerate.
---------------------------------------------------------------------*/
#if defined(_WIN32)
#define BASEBUFSZ (512+128)
static uint32_t MV_GetBufferSize(unsigned samplerate)
{
    static unsigned lastsr = 0, lastbufsz = 0;

    if (samplerate == lastsr) return lastbufsz;

    lastsr = samplerate;
    lastbufsz = (samplerate*BASEBUFSZ/22050)&(~15);

#ifdef RENDERTYPEWIN
    {
        extern int32_t is_vista;

        if (is_vista)
            lastbufsz = (samplerate*BASEBUFSZ/22050*2)&(~15);
    }
#endif

    return lastbufsz;
}
#endif

/*---------------------------------------------------------------------
   Function: MV_Mix

   Mixes the sound into the buffer.
---------------------------------------------------------------------*/

static void MV_Mix(VoiceNode *voice, int32_t buffer)
{
    char          *start;
    int32_t length;
    int32_t voclength;
    uint32_t position;
    uint32_t rate;
    uint32_t FixedPointBufferSize;

    if ((voice->length == 0) && (voice->GetSound(voice) != KeepPlaying))
    {
        return;
    }

    length               = MixBufferSize;
    FixedPointBufferSize = voice->FixedPointBufferSize;

    MV_MixDestination    = MV_MixBuffer[ buffer ];
    MV_LeftVolume        = voice->LeftVolume;
    MV_RightVolume       = voice->RightVolume;

    if ((MV_Channels == 2) && (IS_QUIET(MV_LeftVolume)))
    {
        MV_LeftVolume      = MV_RightVolume;
        MV_MixDestination += MV_RightChannelOffset;
    }

    // Add this voice to the mix
    while (length > 0)
    {
        start    = voice->sound;
        rate     = voice->RateScale;
        position = voice->position;

        // Check if the last sample in this buffer would be
        // beyond the length of the sample block
        if ((position + FixedPointBufferSize) >= voice->length)
        {
            if (position < voice->length)
            {
                voclength = (voice->length - position + rate - 1) / rate;
            }
            else
            {
                voice->GetSound(voice);
                return;
            }
        }
        else
        {
            voclength = length;
        }

        voice->mix(position, rate, start, voclength);

        if (voclength & 1)
        {
            MV_MixPosition += rate;
            voclength -= 1;
        }
        voice->position = MV_MixPosition;

        length -= voclength;

        if (voice->position >= voice->length)
        {
            // Get the next block of sound
            if (voice->GetSound(voice) != KeepPlaying)
            {
                return;
            }

            if (length > 0)
            {
                // Get the position of the last sample in the buffer
                FixedPointBufferSize = voice->RateScale * (length - 1);
            }
        }
    }
}


/*---------------------------------------------------------------------
   Function: MV_PlayVoice

   Adds a voice to the play list.
---------------------------------------------------------------------*/

void MV_PlayVoice(VoiceNode *voice)
{
    unsigned flags;

    flags = DisableInterrupts();
    LL_SortedInsertion(&VoiceList, voice, prev, next, VoiceNode, priority);

//    if(!voice->bufsnd)voice->bufsnd=(char *)Bcalloc(0x8000*4,sizeof(uint8_t));
//    if(!voice->bufsnd)initprintf("Attention. It gonna crash! Thank you."); // FIXME: change the msg
    RestoreInterrupts(flags);
}


/*---------------------------------------------------------------------
   Function: MV_StopVoice

   Removes the voice from the play list and adds it to the free list.
---------------------------------------------------------------------*/

void MV_StopVoice(VoiceNode *voice)
{
    uint32_t flags;

    flags = DisableInterrupts();
//    if(!voice->bufsnd)Bfree(voice->bufsnd);
    // move the voice from the play list to the free list
    LL_Remove(voice, next, prev);
    LL_Add(&VoicePool, voice, next, prev);

    RestoreInterrupts(flags);
}


/*---------------------------------------------------------------------
   Function: MV_ServiceVoc

   Starts playback of the waiting buffer and mixes the next one.
---------------------------------------------------------------------*/

// static int32_t backcolor = 1;

int32_t MV_ServiceVoc(int32_t buffer)
{
    VoiceNode *voice;
    VoiceNode *next;

    // Get the currently playing buffer
#if defined(_WIN32)
    MV_MixPage   = buffer;
#else
    UNREFERENCED_PARAMETER(buffer);
#endif

    // Toggle which buffer we'll mix next
    MV_MixPage++;
    if (MV_MixPage >= MV_NumberOfBuffers)
    {
        MV_MixPage -= MV_NumberOfBuffers;
    }

    if (MV_ReverbLevel == 0)
    {
        // Initialize buffer
        //Commented out so that the buffer is always cleared.
        //This is so the guys at Echo Speech can mix into the
        //buffer even when no sounds are playing.
        //if (!MV_BufferEmpty[ MV_MixPage ])
        {
            ClearBuffer_DW(MV_MixBuffer[ MV_MixPage ], MV_Silence, MV_BufferSize >> 2);
            MV_BufferEmpty[ MV_MixPage ] = TRUE;
        }
    }
    else
    {
        char *end;
        char *source;
        char *dest;
        int32_t count;
        int32_t length;

        end = MV_MixBuffer[ 0 ] + MV_BufferLength;
        dest = MV_MixBuffer[ MV_MixPage ];
        source = MV_MixBuffer[ MV_MixPage ] - MV_ReverbDelay;
        if (source < MV_MixBuffer[ 0 ])
        {
            source += MV_BufferLength;
        }

        length = MV_BufferSize;
        while (length > 0)
        {
            count = length;
            if (source + count > end)
            {
                count = end - source;
            }

            if (MV_Bits == 16)
            {
                if (MV_ReverbTable != NULL)
                {
                    MV_16BitReverb(source, dest, MV_ReverbTable, count / 2);
                }
                else
                {
                    MV_16BitReverbFast(source, dest, count / 2, MV_ReverbLevel);
                }
            }
            else
            {
                if (MV_ReverbTable != NULL)
                {
                    MV_8BitReverb((char *)source, (char *)dest, MV_ReverbTable, count);
                }
                else
                {
                    MV_8BitReverbFast((char *)source, (char *)dest, count, MV_ReverbLevel);
                }
            }

            // if we go through the loop again, it means that we've wrapped around the buffer
            source  = MV_MixBuffer[ 0 ];
            dest   += count;
            length -= count;
        }
    }

    // Play any waiting voices
    for (voice = VoiceList.next; voice != &VoiceList; voice = next)
    {
//      if ((voice < &MV_Voices[ 0 ]) || (voice > &MV_Voices[ 8 ]))
//         {
//         SetBorderColor(backcolor++);
//         break;
//         }

        MV_BufferEmpty[ MV_MixPage ] = FALSE;

        MV_MixFunction(voice, MV_MixPage);

        next = voice->next;

        // Is this voice done?
        if (!voice->Playing)
        {
            MV_StopVoice(voice);

            if (MV_CallBackFunc)
            {
                MV_CallBackFunc(voice->callbackval);
            }
        }
    }

    return MV_MixPage;
}


/*---------------------------------------------------------------------
   Function: MV_GetNextVOCBlock

   Interperate the information of a VOC format sound file.
---------------------------------------------------------------------*/

static playbackstatus MV_GetNextVOCBlock(VoiceNode *voice)
{
    char *ptr;
    int32_t blocktype;
    int32_t lastblocktype;
    uint32_t blocklength = 0;
    uint32_t samplespeed = 0;
    uint32_t tc = 0;
    int32_t packtype;
    int32_t voicemode;
    int32_t done;
    unsigned       BitsPerSample;
    unsigned       Channels;
    unsigned       Format;

    if (voice->BlockLength > 0)
    {
        voice->position    -= voice->length;
        voice->sound       += voice->length >> 16;
        if (voice->bits == 16)
        {
            voice->sound += voice->length >> 16;
        }
        voice->length       = min(voice->BlockLength, 0x8000);
        voice->BlockLength -= voice->length;
        voice->length     <<= 16;
        return(KeepPlaying);
    }

    if ((voice->length > 0) && (voice->LoopEnd != NULL) && (voice->LoopStart != NULL))
    {
        voice->BlockLength  = voice->LoopSize;
        voice->sound        = voice->LoopStart;
        voice->position     = 0;
        voice->length       = min(voice->BlockLength, 0x8000);
        voice->BlockLength -= voice->length;
        voice->length     <<= 16;
        return(KeepPlaying);
    }

    ptr = (char *)voice->NextBlock;

    voice->Playing = TRUE;

    voicemode = 0;
    lastblocktype = 0;
    packtype = 0;

    done = FALSE;
    while (!done)
    {
        // Stop playing if we get a NULL pointer
        if (ptr == NULL)
        {
            voice->Playing = FALSE;
            done = TRUE;
            break;
        }

        blocktype = (int32_t)*ptr;
        blocklength = (*(uint32_t *)(ptr + 1)) & 0x00ffffff;
        ptr += 4;

        switch (blocktype)
        {
        case 0 :
            // End of data
            if ((voice->LoopStart == NULL) || ((uint32_t *)voice->LoopStart >= (uint32_t *)(ptr - 4)))
            {
                voice->Playing = FALSE;
                done = TRUE;
            }
            else
            {
                voice->BlockLength  = (char*)(ptr - 4) - voice->LoopStart;
                voice->sound        = voice->LoopStart;
                voice->position     = 0;
                voice->length       = min(voice->BlockLength, 0x8000);
                voice->BlockLength -= voice->length;
                voice->length     <<= 16;
                return(KeepPlaying);
            }
            break;

        case 1 :
            // Sound data block
            voice->bits  = 8;
            if (lastblocktype != 8)
            {
                tc = (uint32_t)*ptr << 8;
                packtype = *(ptr + 1);
            }

            ptr += 2;
            blocklength -= 2;

            samplespeed = 256000000L / (65536 - tc);

            // Skip packed or stereo data
            if ((packtype != 0) || (voicemode != 0))
            {
                ptr += blocklength;
            }
            else
            {
                done = TRUE;
            }
            voicemode = 0;
            break;

        case 2 :
            // Sound continuation block
            samplespeed = voice->SamplingRate;
            done = TRUE;
            break;

        case 3 :
            // Silence
            // Not implimented.
            ptr += blocklength;
            break;

        case 4 :
            // Marker
            // Not implimented.
            ptr += blocklength;
            break;

        case 5 :
            // ASCII string
            // Not implimented.
            ptr += blocklength;
            break;

        case 6 :
            // Repeat begin
            if (voice->LoopEnd == NULL)
            {
                voice->LoopCount = *(uint16_t *)ptr;
                voice->LoopStart = (char *)(ptr + blocklength);
            }
            ptr += blocklength;
            break;

        case 7 :
            // Repeat end
            ptr += blocklength;
            if (lastblocktype == 6)
            {
                voice->LoopCount = 0;
            }
            else
            {
                if ((voice->LoopCount > 0) && (voice->LoopStart != NULL))
                {
                    ptr = (char *)voice->LoopStart;
                    if (voice->LoopCount < 0xffff)
                    {
                        voice->LoopCount--;
                        if (voice->LoopCount == 0)
                        {
                            voice->LoopStart = NULL;
                        }
                    }
                }
            }
            break;

        case 8 :
            // Extended block
            voice->bits  = 8;
            tc = *(uint16_t *)ptr;
            packtype = *(ptr + 2);
            voicemode = *(ptr + 3);
            ptr += blocklength;
            break;

        case 9 :
            // New sound data block
            samplespeed = *(uint32_t *)ptr;
            BitsPerSample = (unsigned)*(ptr + 4);
            Channels = (unsigned)*(ptr + 5);
            Format = (unsigned)*(uint16_t *)(ptr + 6);

            if ((BitsPerSample == 8) && (Channels == 1) && (Format == VOC_8BIT))
            {
                ptr         += 12;
                blocklength -= 12;
                voice->bits  = 8;
                done         = TRUE;
            }
            else if ((BitsPerSample == 16) && (Channels == 1) && (Format == VOC_16BIT))
            {
                ptr         += 12;
                blocklength -= 12;
                voice->bits  = 16;
                done         = TRUE;
            }
            else
            {
                ptr += blocklength;
            }
            break;

        default :
            // Unknown data.  Probably not a VOC file.
            voice->Playing = FALSE;
            done = TRUE;
            break;
        }

        lastblocktype = blocktype;
    }

    if (voice->Playing)
    {
        voice->NextBlock    = (char *)(ptr + blocklength);
        voice->sound        = (char *)ptr;

        voice->SamplingRate = samplespeed;
        voice->RateScale    = (voice->SamplingRate * voice->PitchScale) / MV_MixRate;

        // Multiply by MixBufferSize - 1
        voice->FixedPointBufferSize = (voice->RateScale * MixBufferSize) -
                                      voice->RateScale;

        if (voice->LoopEnd != NULL)
        {
            if (blocklength > (uintptr_t)voice->LoopEnd)
            {
                blocklength = (uintptr_t)voice->LoopEnd;
            }
            else
            {
                voice->LoopEnd = (char *)blocklength;
            }

            voice->LoopStart = voice->sound + (uintptr_t)voice->LoopStart;
            voice->LoopEnd   = voice->sound + (uintptr_t)voice->LoopEnd;
            voice->LoopSize  = voice->LoopEnd - voice->LoopStart;
        }

        if (voice->bits == 16)
        {
            blocklength /= 2;
        }

        voice->position     = 0;
        voice->length       = min(blocklength, 0x8000);
        voice->BlockLength  = blocklength - voice->length;
        voice->length     <<= 16;

        MV_SetVoiceMixMode(voice);

        return(KeepPlaying);
    }

    return(NoMoreData);
}


/*---------------------------------------------------------------------
   Function: MV_GetNextDemandFeedBlock

   Controls playback of demand fed data.
---------------------------------------------------------------------*/

playbackstatus MV_GetNextDemandFeedBlock(VoiceNode *voice)
{
    if (voice->BlockLength > 0)
    {
        voice->position    -= voice->length;
        voice->sound       += voice->length >> 16;
        voice->length       = min(voice->BlockLength, 0x8000);
        voice->BlockLength -= voice->length;
        voice->length     <<= 16;

        return(KeepPlaying);
    }

    if (voice->DemandFeed == NULL)
    {
        return(NoMoreData);
    }

    voice->position     = 0;
    (voice->DemandFeed)(&voice->sound, &voice->BlockLength);
    voice->length       = min(voice->BlockLength, 0x8000);
    voice->BlockLength -= voice->length;
    voice->length     <<= 16;

    if ((voice->length > 0) && (voice->sound != NULL))
    {
        return(KeepPlaying);
    }
    return(NoMoreData);
}


/*---------------------------------------------------------------------
   Function: MV_GetNextRawBlock

   Controls playback of demand fed data.
---------------------------------------------------------------------*/

playbackstatus MV_GetNextRawBlock(VoiceNode *voice)
{
    if (voice->BlockLength <= 0)
    {
        if (voice->LoopStart == NULL)
        {
            voice->Playing = FALSE;
            return(NoMoreData);
        }

        voice->BlockLength = voice->LoopSize;
        voice->NextBlock   = voice->LoopStart;
        voice->length = 0;
        voice->position = 0;
    }

    voice->sound        = voice->NextBlock;
    voice->position    -= voice->length;
    voice->length       = min(voice->BlockLength, 0x8000);
    voice->NextBlock   += voice->length;
    if (voice->bits == 16)
    {
        voice->NextBlock += voice->length;
    }
    voice->BlockLength -= voice->length;
    voice->length     <<= 16;

    return(KeepPlaying);
}


/*---------------------------------------------------------------------
   Function: MV_GetNextWAVBlock

   Controls playback of demand fed data.
---------------------------------------------------------------------*/

playbackstatus MV_GetNextWAVBlock(VoiceNode *voice)
{
    if (voice->BlockLength <= 0)
    {
        if (voice->LoopStart == NULL)
        {
            voice->Playing = FALSE;
            return(NoMoreData);
        }

        voice->BlockLength = voice->LoopSize;
        voice->NextBlock   = voice->LoopStart;
        voice->length      = 0;
        voice->position    = 0;
    }

    voice->sound        = voice->NextBlock;
    voice->position    -= voice->length;
    voice->length       = min(voice->BlockLength, 0x8000);
    voice->NextBlock   += voice->length;
    if (voice->bits == 16)
    {
        voice->NextBlock += voice->length;
    }
    voice->BlockLength -= voice->length;
    voice->length     <<= 16;

    return(KeepPlaying);
}

/*---------------------------------------------------------------------
   Function: MV_GetNextOGGBlock

   Controls playback of demand fed data.
---------------------------------------------------------------------*/

void downsample(char *ptr,int32_t size,int32_t factor)
{
    int16_t *pti=(int16_t *)ptr;int32_t i,j,sum;

    for (i=0;i<size>>factor;i++)
    {
        sum=0;
        for (j=0;j<1<<factor;j++)sum+=pti[(i<<factor)+j];
        pti[i]=sum>>factor;
    }
}

playbackstatus MV_GetNextOGGBlock(VoiceNode *voice)
{
    int32_t sz;
    int32_t size=0;
    int32_t section,result;

//    initprintf("_%d %d, %d\n",voice->callbackval,voice->position,voice->BlockLength);
    if (voice->BlockLength <= 0)
    {
        if (voice->LoopStart == NULL)
        {
//            initprintf("END %d\n",voice->callbackval);
            voice->Playing = FALSE;
            return(NoMoreData);
        }
//        initprintf("repeat 2\n");

        voice->BlockLength = voice->LoopSize;
        voice->length      = 0;
        voice->position    = 0;
        ov_pcm_seek(&voice->OGGstream.oggStream,0);
    }

    voice->position    -= voice->length;
    sz=voice->length    = min(voice->BlockLength, 0x8000);
    voice->BlockLength -= voice->length;
    voice->length     <<= 16;

    sz<<=voice->downsample+1;
    while (size<sz)
    {
        result=ov_read(&voice->OGGstream.oggStream,voice->bufsnd+(size>>voice->downsample),sz-size,0,2,1,&section);
        if (result> 0)
        {
            downsample(voice->bufsnd+(size>>voice->downsample),result,voice->downsample);
            size+=result;
        }
        else if (result==0)
        {
//           initprintf("!repeat %d\n",voice->callbackval);
            voice->BlockLength=0;
            voice->length=size<<16;break;
        }
        else
        {
            initprintf("#%d\n",result);
            break;
        }
    }
    voice->sound=voice->bufsnd;

    return(KeepPlaying);
}

/*---------------------------------------------------------------------
   Function: MV_ServiceRecord

   Starts recording of the waiting buffer.
---------------------------------------------------------------------*/
#if 0
static void MV_ServiceRecord(void)
{
    if (MV_RecordFunc)
    {
        MV_RecordFunc(MV_MixBuffer[ 0 ] + MV_MixPage * MixBufferSize, MixBufferSize);
    }

    // Toggle which buffer we'll mix next
    MV_MixPage++;
    if (MV_MixPage >= NumberOfBuffers)
    {
        MV_MixPage = 0;
    }
}
#endif

/*---------------------------------------------------------------------
   Function: MV_GetVoice

   Locates the voice with the specified handle.
---------------------------------------------------------------------*/

VoiceNode *MV_GetVoice(int32_t handle)
{
    VoiceNode *voice;
    unsigned  flags;

    flags = DisableInterrupts();

    for (voice = VoiceList.next; voice != &VoiceList; voice = voice->next)
    {
        if (handle == voice->handle)
        {
            break;
        }
    }

    RestoreInterrupts(flags);

    if (voice == &VoiceList)
    {
        MV_SetErrorCode(MV_VoiceNotFound);
        voice = NULL;
    }

    return(voice);
}


/*---------------------------------------------------------------------
   Function: MV_VoicePlaying

   Checks if the voice associated with the specified handle is
   playing.
---------------------------------------------------------------------*/

int32_t MV_VoicePlaying(int32_t handle)
{
    VoiceNode *voice;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(FALSE);
    }

    voice = MV_GetVoice(handle);

    if (voice == NULL)
    {
        return(FALSE);
    }

    return(TRUE);
}


/*---------------------------------------------------------------------
   Function: MV_KillAllVoices

   Stops output of all currently active voices.
---------------------------------------------------------------------*/

int32_t MV_KillAllVoices(void)
{
    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(MV_Error);
    }

    // Remove all the voices from the list
    while (VoiceList.next != &VoiceList)
    {
        MV_Kill(VoiceList.next->handle);
    }

    return(MV_Ok);
}


/*---------------------------------------------------------------------
   Function: MV_Kill

   Stops output of the voice associated with the specified handle.
---------------------------------------------------------------------*/

int32_t MV_Kill(int32_t handle)
{
    VoiceNode *voice;
    unsigned  flags;
    uint32_t callbackval;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(MV_Error);
    }

    flags = DisableInterrupts();

    voice = MV_GetVoice(handle);
    if (voice == NULL)
    {
        RestoreInterrupts(flags);
        MV_SetErrorCode(MV_VoiceNotFound);
        return(MV_Error);
    }

    callbackval = voice->callbackval;

    MV_StopVoice(voice);

    RestoreInterrupts(flags);

    if (MV_CallBackFunc)
    {
        MV_CallBackFunc(callbackval);
    }

    return(MV_Ok);
}


/*---------------------------------------------------------------------
   Function: MV_VoicesPlaying

   Determines the number of currently active voices.
---------------------------------------------------------------------*/

int32_t MV_VoicesPlaying(void)
{
    VoiceNode   *voice;
    int32_t NumVoices = 0;
    unsigned    flags;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(0);
    }

    flags = DisableInterrupts();

    for (voice = VoiceList.next; voice != &VoiceList; voice = voice->next)
    {
        NumVoices++;
    }

    RestoreInterrupts(flags);

    return(NumVoices);
}


/*---------------------------------------------------------------------
   Function: MV_AllocVoice

   Retrieve an inactive or lower priority voice for output.
---------------------------------------------------------------------*/

VoiceNode *MV_AllocVoice(int32_t priority)
{
    VoiceNode   *voice;
    VoiceNode   *node;
    unsigned    flags;

//return(NULL);
    if (MV_Recording)
    {
        return(NULL);
    }

    flags = DisableInterrupts();

    // Check if we have any free voices
    if (LL_Empty(&VoicePool, next, prev))
    {
        // check if we have a higher priority than a voice that is playing.
        voice = VoiceList.next;
        for (node = voice->next; node != &VoiceList; node = node->next)
        {
            if (node->priority < voice->priority)
            {
                voice = node;
            }
        }

        if (priority >= voice->priority)
        {
            MV_Kill(voice->handle);
        }
    }

    // Check if any voices are in the voice pool
    if (LL_Empty(&VoicePool, next, prev))
    {
        // No free voices
        RestoreInterrupts(flags);
        return(NULL);
    }

    voice = VoicePool.next;
    LL_Remove(voice, next, prev);
    RestoreInterrupts(flags);

    // Find a free voice handle
    do
    {
        MV_VoiceHandle++;
        if (MV_VoiceHandle < MV_MinVoiceHandle)
        {
            MV_VoiceHandle = MV_MinVoiceHandle;
        }
    }
    while (MV_VoicePlaying(MV_VoiceHandle));

    voice->handle = MV_VoiceHandle;

    return(voice);
}


/*---------------------------------------------------------------------
   Function: MV_VoiceAvailable

   Checks if a voice can be play at the specified priority.
---------------------------------------------------------------------*/

int32_t MV_VoiceAvailable(int32_t priority)
{
    VoiceNode   *voice;
    VoiceNode   *node;
    unsigned    flags;

    // Check if we have any free voices
    if (!LL_Empty(&VoicePool, next, prev))
    {
        return(TRUE);
    }

    flags = DisableInterrupts();

    // check if we have a higher priority than a voice that is playing.
    voice = VoiceList.next;
    for (node = VoiceList.next; node != &VoiceList; node = node->next)
    {
        if (node->priority < voice->priority)
        {
            voice = node;
        }
    }

    RestoreInterrupts(flags);

    if ((voice != &VoiceList) && (priority >= voice->priority))
    {
        return(TRUE);
    }

    return(FALSE);
}


/*---------------------------------------------------------------------
   Function: MV_SetVoicePitch

   Sets the pitch for the specified voice.
---------------------------------------------------------------------*/

void MV_SetVoicePitch(VoiceNode *voice, uint32_t rate, int32_t pitchoffset)
{
    voice->SamplingRate = rate;
    voice->PitchScale   = PITCH_GetScale(pitchoffset);
    voice->RateScale    = (rate * voice->PitchScale) / MV_MixRate;

    // Multiply by MixBufferSize - 1
    voice->FixedPointBufferSize = (voice->RateScale * MixBufferSize) -
                                  voice->RateScale;
}


/*---------------------------------------------------------------------
   Function: MV_SetPitch

   Sets the pitch for the voice associated with the specified handle.
---------------------------------------------------------------------*/

int32_t MV_SetPitch(int32_t handle, int32_t pitchoffset)
{
    VoiceNode *voice;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(MV_Error);
    }

    voice = MV_GetVoice(handle);
    if (voice == NULL)
    {
        MV_SetErrorCode(MV_VoiceNotFound);
        return(MV_Error);
    }

    MV_SetVoicePitch(voice, voice->SamplingRate, pitchoffset);

    return(MV_Ok);
}


/*---------------------------------------------------------------------
   Function: MV_SetFrequency

   Sets the frequency for the voice associated with the specified handle.
---------------------------------------------------------------------*/

int32_t MV_SetFrequency(int32_t handle, int32_t frequency)
{
    VoiceNode *voice;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(MV_Error);
    }

    voice = MV_GetVoice(handle);
    if (voice == NULL)
    {
        MV_SetErrorCode(MV_VoiceNotFound);
        return(MV_Error);
    }

    MV_SetVoicePitch(voice, frequency, 0);

    return(MV_Ok);
}


/*---------------------------------------------------------------------
   Function: MV_GetVolumeTable

   Returns a pointer to the volume table associated with the specified
   volume.
---------------------------------------------------------------------*/

static int16_t *MV_GetVolumeTable(int32_t vol)
{
    int32_t volume;
    int16_t *table;

    volume = MIX_VOLUME(vol);

    table = (int16_t*)&MV_VolumeTable[ volume ];

    return(table);
}


/*---------------------------------------------------------------------
   Function: MV_SetVoiceMixMode

   Selects which method should be used to mix the voice.
---------------------------------------------------------------------*/

static void MV_SetVoiceMixMode(VoiceNode *voice)
{
    unsigned flags;
    int32_t test;

    flags = DisableInterrupts();

    test = T_DEFAULT;
    if (MV_Bits == 8)
    {
        test |= T_8BITS;
    }

    if (voice->bits == 16)
    {
        test |= T_16BITSOURCE;
    }

    if (MV_Channels == 1)
    {
        test |= T_MONO;
    }
    else
    {
        if (IS_QUIET(voice->RightVolume))
        {
            test |= T_RIGHTQUIET;
        }
        else if (IS_QUIET(voice->LeftVolume))
        {
            test |= T_LEFTQUIET;
        }
    }

    // Default case
    voice->mix = MV_Mix8BitMono;

    switch (test)
    {
    case T_8BITS | T_MONO | T_16BITSOURCE :
        voice->mix = MV_Mix8BitMono16;
        break;

    case T_8BITS | T_MONO :
        voice->mix = MV_Mix8BitMono;
        break;

    case T_8BITS | T_16BITSOURCE | T_LEFTQUIET :
        MV_LeftVolume = MV_RightVolume;
        voice->mix = MV_Mix8BitMono16;
        break;

    case T_8BITS | T_LEFTQUIET :
        MV_LeftVolume = MV_RightVolume;
        voice->mix = MV_Mix8BitMono;
        break;

    case T_8BITS | T_16BITSOURCE | T_RIGHTQUIET :
        voice->mix = MV_Mix8BitMono16;
        break;

    case T_8BITS | T_RIGHTQUIET :
        voice->mix = MV_Mix8BitMono;
        break;

    case T_8BITS | T_16BITSOURCE :
        voice->mix = MV_Mix8BitStereo16;
        break;

    case T_8BITS :
        voice->mix = MV_Mix8BitStereo;
        break;

    case T_MONO | T_16BITSOURCE :
        voice->mix = MV_Mix16BitMono16;
        break;

    case T_MONO :
        voice->mix = MV_Mix16BitMono;
        break;

    case T_16BITSOURCE | T_LEFTQUIET :
        MV_LeftVolume = MV_RightVolume;
        voice->mix = MV_Mix16BitMono16;
        break;

    case T_LEFTQUIET :
        MV_LeftVolume = MV_RightVolume;
        voice->mix = MV_Mix16BitMono;
        break;

    case T_16BITSOURCE | T_RIGHTQUIET :
        voice->mix = MV_Mix16BitMono16;
        break;

    case T_RIGHTQUIET :
        voice->mix = MV_Mix16BitMono;
        break;

    case T_16BITSOURCE :
        voice->mix = MV_Mix16BitStereo16;
        break;

    case T_SIXTEENBIT_STEREO :
        voice->mix = MV_Mix16BitStereo;
        break;

    default :
        voice->mix = MV_Mix8BitMono;
    }

    RestoreInterrupts(flags);
}


/*---------------------------------------------------------------------
   Function: MV_SetVoiceVolume

   Sets the stereo and mono volume level of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

void MV_SetVoiceVolume(VoiceNode *voice, int32_t vol, int32_t left, int32_t right)
{
    if (MV_Channels == 1)
    {
        left  = vol;
        right = vol;
    }

    if (MV_SwapLeftRight)
    {
        // SBPro uses reversed panning
        voice->LeftVolume  = MV_GetVolumeTable(right);
        voice->RightVolume = MV_GetVolumeTable(left);
    }
    else
    {
        voice->LeftVolume  = MV_GetVolumeTable(left);
        voice->RightVolume = MV_GetVolumeTable(right);
    }

    MV_SetVoiceMixMode(voice);
}


/*---------------------------------------------------------------------
   Function: MV_EndLooping

   Stops the voice associated with the specified handle from looping
   without stoping the sound.
---------------------------------------------------------------------*/

int32_t MV_EndLooping(int32_t handle)
{
    VoiceNode *voice;
    unsigned flags;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(MV_Error);
    }

    flags = DisableInterrupts();

    voice = MV_GetVoice(handle);
    if (voice == NULL)
    {
        RestoreInterrupts(flags);
        MV_SetErrorCode(MV_VoiceNotFound);
        return(MV_Warning);
    }

    voice->LoopCount = 0;
    voice->LoopStart = NULL;
    voice->LoopEnd   = NULL;

    RestoreInterrupts(flags);

    return(MV_Ok);
}


/*---------------------------------------------------------------------
   Function: MV_SetPan

   Sets the stereo and mono volume level of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int32_t MV_SetPan(int32_t handle, int32_t vol, int32_t left, int32_t right)
{
    VoiceNode *voice;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(MV_Error);
    }

    voice = MV_GetVoice(handle);
    if (voice == NULL)
    {
        MV_SetErrorCode(MV_VoiceNotFound);
        return(MV_Warning);
    }

    MV_SetVoiceVolume(voice, vol, left, right);

    return(MV_Ok);
}


/*---------------------------------------------------------------------
   Function: MV_Pan3D

   Set the angle and distance from the listener of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int32_t MV_Pan3D(int32_t handle, int32_t angle, int32_t distance)
{
    int32_t left;
    int32_t right;
    int32_t mid;
    int32_t volume;
    int32_t status;

    if (distance < 0)
    {
        distance  = -distance;
        angle    += MV_NumPanPositions / 2;
    }

    volume = MIX_VOLUME(distance);

    // Ensure angle is within 0 - 31
    angle &= MV_MaxPanPosition;

    left  = MV_PanTable[ angle ][ volume ].left;
    right = MV_PanTable[ angle ][ volume ].right;
    mid   = max(0, 255 - distance);

    status = MV_SetPan(handle, mid, left, right);

    return(status);
}


/*---------------------------------------------------------------------
   Function: MV_SetReverb

   Sets the level of reverb to add to mix.
---------------------------------------------------------------------*/

void MV_SetReverb(int32_t reverb)
{
    MV_ReverbLevel = MIX_VOLUME(reverb);
    MV_ReverbTable = &MV_VolumeTable[ MV_ReverbLevel ];
}


/*---------------------------------------------------------------------
   Function: MV_SetFastReverb

   Sets the level of reverb to add to mix.
---------------------------------------------------------------------*/

void MV_SetFastReverb(int32_t reverb)
{
    MV_ReverbLevel = max(0, min(16, reverb));
    MV_ReverbTable = NULL;
}


/*---------------------------------------------------------------------
   Function: MV_GetMaxReverbDelay

   Returns the maximum delay time for reverb.
---------------------------------------------------------------------*/

int32_t MV_GetMaxReverbDelay(void)
{
    int32_t maxdelay;

    maxdelay = MixBufferSize * MV_NumberOfBuffers;

    return maxdelay;
}


/*---------------------------------------------------------------------
   Function: MV_GetReverbDelay

   Returns the current delay time for reverb.
---------------------------------------------------------------------*/

int32_t MV_GetReverbDelay(void)
{
    return MV_ReverbDelay / MV_SampleSize;
}


/*---------------------------------------------------------------------
   Function: MV_SetReverbDelay

   Sets the delay level of reverb to add to mix.
---------------------------------------------------------------------*/

void MV_SetReverbDelay(int32_t delay)
{
    int32_t maxdelay;

    maxdelay = MV_GetMaxReverbDelay();
    MV_ReverbDelay = max((signed)MixBufferSize, min(delay, maxdelay));
    MV_ReverbDelay *= MV_SampleSize;
}


/*---------------------------------------------------------------------
   Function: MV_SetMixMode

   Prepares Multivoc to play stereo of mono digitized sounds.
---------------------------------------------------------------------*/

int32_t MV_SetMixMode(int32_t numchannels, int32_t samplebits)
{
    int32_t mode;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(MV_Error);
    }

    mode = 0;
    if (numchannels == 2)
    {
        mode |= STEREO;
    }
    if (samplebits == 16)
    {
        mode |= SIXTEEN_BIT;
    }

//#if defined(_WIN32)
//    MV_MixMode = DSOUND_SetMixMode(mode);
//#else
    MV_MixMode = mode;
//#endif

    MV_Channels = 1;
    if (MV_MixMode & STEREO)
    {
        MV_Channels = 2;
    }

    MV_Bits = 8;
    if (MV_MixMode & SIXTEEN_BIT)
    {
        MV_Bits = 16;
    }

    MV_BuffShift  = 7 + MV_Channels;
    MV_SampleSize = sizeof(MONO8) * MV_Channels;

    if (MV_Bits == 8)
    {
        MV_Silence = SILENCE_8BIT;
    }
    else
    {
        MV_Silence     = SILENCE_16BIT;
        MV_BuffShift  += 1;
        MV_SampleSize *= 2;
    }

    MV_BufferSize = MixBufferSize * MV_SampleSize;
    MV_NumberOfBuffers = TotalBufferSize / MV_BufferSize;
    MV_BufferLength = TotalBufferSize;

    MV_RightChannelOffset = MV_SampleSize / 2;

    return(MV_Ok);
}


// ---------------------------------------------------------------------
//  OGG file
// ---------------------------------------------------------------------
ov_callbacks cb;

size_t ReadOgg(void *ptr, size_t size1, size_t nmemb, void *datasource)
{
    sounddef *d=(sounddef *)datasource;
    size1*=nmemb;
    /*    if (d->pos>=d->size)
            return 0;*/
    if (d->pos+size1>=d->size)
        size1=d->size-d->pos;
    Bmemcpy(ptr,(d->ptrsnd+d->pos),size1);
    d->pos+=size1;
    return size1;
}

int32_t SeekOgg(void *datasource,ogg_int64_t offset,int32_t whence)
{
    sounddef *d=(sounddef *)datasource;
    switch (whence)
    {
    case SEEK_SET:
        whence=offset;
        break;
    case SEEK_CUR:
        whence=d->pos+offset;
        break;
    case SEEK_END:
        whence=d->size-offset;
        break;
    default: return -1;
    }
    /*    if (whence>=(int32_t)d->size||whence<0)
            return -1;*/
    d->pos=whence;
    return 0;
}

intptr_t TellOgg(void *datasource)
{
    sounddef *d=(sounddef *)datasource;
    return d->pos;
}


int32_t CloseOgg(void *datasource)
{
    UNREFERENCED_PARAMETER(datasource);
    return 0;
}


/*---------------------------------------------------------------------
   Function: MV_StartPlayback

   Starts the sound playback engine.
---------------------------------------------------------------------*/

int32_t MV_StartPlayback(void)
{
    int32_t status;
    int32_t buffer;

    cb.close_func=CloseOgg;
    cb.read_func =ReadOgg;
    cb.seek_func =SeekOgg;
    cb.tell_func =(void *)TellOgg;

    // Initialize the buffers
    ClearBuffer_DW(MV_MixBuffer[ 0 ], MV_Silence, TotalBufferSize >> 2);
    for (buffer = 0; buffer < MV_NumberOfBuffers; buffer++)
    {
        MV_BufferEmpty[ buffer ] = TRUE;
    }

    // Set the mix buffer variables
    MV_MixPage = 1;
    MV_MixFunction = MV_Mix;
    MV_MixRate = MV_RequestedMixRate;

    // Start playback
#if defined(_WIN32)
    status = DSOUND_BeginBufferedPlayback(MV_MixBuffer[ 0 ], MV_ServiceVoc, TotalBufferSize, MV_NumberOfBuffers);
#else
    status = DSL_BeginBufferedPlayback(MV_MixBuffer[ 0 ], MV_ServiceVoc, TotalBufferSize, MV_NumberOfBuffers);
#endif

    if (status != 0)
    {
        MV_SetErrorCode(MV_BlasterError);
        return(MV_Error);
    }

    return(MV_Ok);
}


/*---------------------------------------------------------------------
   Function: MV_StopPlayback

   Stops the sound playback engine.
---------------------------------------------------------------------*/

void MV_StopPlayback(void)
{
    VoiceNode   *voice;
    VoiceNode   *next;
    unsigned    flags;

    // Stop sound playback
#if defined(_WIN32)
    DSOUND_StopPlayback();
#else
    DSL_StopPlayback();
#endif

    // Make sure all callbacks are done.
    flags = DisableInterrupts();

    for (voice = VoiceList.next; voice != &VoiceList; voice = next)
    {
        next = voice->next;

        MV_StopVoice(voice);

        if (MV_CallBackFunc)
        {
            MV_CallBackFunc(voice->callbackval);
        }
    }

    RestoreInterrupts(flags);
}


#if 0
/*---------------------------------------------------------------------
   Function: MV_StartRecording

   Starts the sound recording engine.
---------------------------------------------------------------------*/

int32_t MV_StartRecording(int32_t MixRate, void(*function)(char *ptr, int32_t length))
{
    int32_t status;

    switch (MV_SoundCard)
    {
    case SoundBlaster :
        break;

    default :
        MV_SetErrorCode(MV_UnsupportedCard);
        return(MV_Error);
        break;
    }

    if (function == NULL)
    {
        MV_SetErrorCode(MV_NullRecordFunction);
        return(MV_Error);
    }

    MV_StopPlayback();

    // Initialize the buffers
    ClearBuffer_DW(MV_MixBuffer[ 0 ], SILENCE_8BIT, TotalBufferSize >> 2);

    // Set the mix buffer variables
    MV_MixPage  = 0;

    MV_RecordFunc = function;

    // Start playback
    switch (MV_SoundCard)
    {
    case SoundBlaster :
        status = BLASTER_BeginBufferedRecord(MV_MixBuffer[ 0 ], TotalBufferSize, NumberOfBuffers, MixRate, MONO_8BIT, MV_ServiceRecord);

        if (status != BLASTER_Ok)
        {
            MV_SetErrorCode(MV_BlasterError);
            return(MV_Error);
        }
        break;
    }

    MV_Recording = TRUE;
    return(MV_Ok);
}
#endif

#if 0
/*---------------------------------------------------------------------
   Function: MV_StopRecord

   Stops the sound record engine.
---------------------------------------------------------------------*/

void MV_StopRecord(void)
{
    // Stop sound playback
    switch (MV_SoundCard)
    {
    case SoundBlaster :
        BLASTER_StopPlayback();
        break;
    }

    MV_Recording = FALSE;
    MV_StartPlayback();
}
#endif


/*---------------------------------------------------------------------
   Function: MV_StartDemandFeedPlayback

   Plays a digitized sound from a user controlled buffering system.
---------------------------------------------------------------------*/

int32_t MV_StartDemandFeedPlayback(void(*function)(char **ptr, uint32_t *length), int32_t rate, int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{
    VoiceNode *voice;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(MV_Error);
    }

    // Request a voice from the voice pool
    voice = MV_AllocVoice(priority);
    if (voice == NULL)
    {
        MV_SetErrorCode(MV_NoVoices);
        return(MV_Error);
    }

    voice->wavetype    = DemandFeed;
    voice->bits        = 8;
    voice->GetSound    = MV_GetNextDemandFeedBlock;
    voice->NextBlock   = NULL;
    voice->DemandFeed  = function;
    voice->LoopStart   = NULL;
    voice->LoopCount   = 0;
    voice->BlockLength = 0;
    voice->position    = 0;
    voice->sound       = NULL;
    voice->length      = 0;
    voice->BlockLength = 0;
    voice->Playing     = TRUE;
    voice->next        = NULL;
    voice->prev        = NULL;
    voice->priority    = priority;
    voice->callbackval = callbackval;

    MV_SetVoicePitch(voice, rate, pitchoffset);
    MV_SetVoiceVolume(voice, vol, left, right);
    MV_PlayVoice(voice);

    return(voice->handle);
}


/*---------------------------------------------------------------------
   Function: MV_PlayRaw

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int32_t MV_PlayRaw(char *ptr, uint32_t length, unsigned rate, int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{
    int32_t status;

    status = MV_PlayLoopedRaw(ptr, length, NULL, NULL, rate, pitchoffset, vol, left, right, priority, callbackval);

    return(status);
}


/*---------------------------------------------------------------------
   Function: MV_PlayLoopedRaw

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int32_t MV_PlayLoopedRaw(char *ptr, int32_t length, char *loopstart, char *loopend, unsigned rate, int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{
    VoiceNode *voice;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(MV_Error);
    }

    // Request a voice from the voice pool
    voice = MV_AllocVoice(priority);
    if (voice == NULL)
    {
        MV_SetErrorCode(MV_NoVoices);
        return(MV_Error);
    }

    voice->wavetype    = Raw;
    voice->bits        = 8;
    voice->GetSound    = MV_GetNextRawBlock;
    voice->Playing     = TRUE;
    voice->NextBlock   = ptr;
    voice->position    = 0;
    voice->BlockLength = length;
    voice->length      = 0;
    voice->next        = NULL;
    voice->prev        = NULL;
    voice->priority    = priority;
    voice->callbackval = callbackval;
    voice->LoopStart   = loopstart;
    voice->LoopEnd     = loopend;
    voice->LoopSize    = (voice->LoopEnd - voice->LoopStart) + 1;

    MV_SetVoicePitch(voice, rate, pitchoffset);
    MV_SetVoiceVolume(voice, vol, left, right);
    MV_PlayVoice(voice);

    return(voice->handle);
}


/*---------------------------------------------------------------------
   Function: MV_PlayWAV

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int32_t MV_PlayWAV(char *ptr, int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{
    int32_t status;

    status = MV_PlayLoopedWAV(ptr, -1, -1, pitchoffset, vol, left, right, priority, callbackval);
    if (status < MV_Ok)
    {
        Bsprintf(tempbuf, "Sound error %d: %s\n",callbackval, FX_ErrorString(FX_Error));
        initprintf(tempbuf);
    }

    return(status);
}


/*---------------------------------------------------------------------
   Function: MV_PlayWAV3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int32_t MV_PlayWAV3D(char *ptr, int32_t pitchoffset, int32_t angle, int32_t distance, int32_t priority, uint32_t callbackval)
{
    int32_t left;
    int32_t right;
    int32_t mid;
    int32_t volume;
    int32_t status;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(MV_Error);
    }

    if (distance < 0)
    {
        distance  = -distance;
        angle    += MV_NumPanPositions / 2;
    }

    volume = MIX_VOLUME(distance);

    // Ensure angle is within 0 - 31
    angle &= MV_MaxPanPosition;

    left  = MV_PanTable[ angle ][ volume ].left;
    right = MV_PanTable[ angle ][ volume ].right;
    mid   = max(0, 255 - distance);

    status = MV_PlayWAV(ptr, pitchoffset, mid, left, right, priority, callbackval);

    return(status);
}


/*---------------------------------------------------------------------
   Function: MV_PlayLoopedWAV

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int32_t MV_PlayLoopedWAV(char *ptr, int32_t loopstart, int32_t loopend, int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{
    riff_header   *riff;
    format_header *format;
    data_header   *data;
    VoiceNode     *voice;
    int32_t length;
    int32_t absloopend;
    int32_t absloopstart;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(MV_Error);
    }

    riff = (riff_header *)ptr;

    if ((strncmp(riff->RIFF, "RIFF", 4) != 0) || (strncmp(riff->WAVE, "WAVE", 4) != 0) || (strncmp(riff->fmt, "fmt ", 4) != 0))
    {
        MV_SetErrorCode(MV_InvalidWAVFile);
        return(MV_Error);
    }

    format = (format_header *)(riff + 1);
    data   = (data_header *)(((char *)format) + riff->format_size);

    // Check if it's PCM data.
    if (format->wFormatTag != 1)
    {
        MV_SetErrorCode(MV_InvalidWAVFile);
        return(MV_Error);
    }

    if (format->nChannels != 1)
    {
        MV_SetErrorCode(MV_InvalidWAVFile);
        return(MV_Error);
    }

    if ((format->nBitsPerSample != 8) && (format->nBitsPerSample != 16))
    {
        MV_SetErrorCode(MV_InvalidWAVFile);
        return(MV_Error);
    }

    if (strncmp((char *)data->DATA, "data", 4) != 0)
    {
        MV_SetErrorCode(MV_InvalidWAVFile);
        return(MV_Error);
    }

    // Request a voice from the voice pool
    voice = MV_AllocVoice(priority);
    if (voice == NULL)
    {
        MV_SetErrorCode(MV_NoVoices);
        return(MV_Error);
    }

    voice->wavetype    = WAV;
    voice->bits        = format->nBitsPerSample;
    voice->GetSound    = MV_GetNextWAVBlock;

    length = data->size;
    absloopstart = loopstart;
    absloopend   = loopend;
    if (voice->bits == 16)
    {
        loopstart  *= 2;
        data->size &= ~1;
        loopend    *= 2;
        length     /= 2;
    }

    loopend    = min(loopend, (signed)data->size);
    absloopend = min(absloopend, length);

    voice->Playing     = TRUE;
    voice->DemandFeed  = NULL;
    voice->LoopStart   = NULL;
    voice->LoopCount   = 0;
    voice->position    = 0;
    voice->length      = 0;
    voice->BlockLength = absloopend;
    voice->NextBlock   = (char *)(data + 1);
    voice->next        = NULL;
    voice->prev        = NULL;
    voice->priority    = priority;
    voice->callbackval = callbackval;
    voice->LoopStart   = voice->NextBlock + loopstart;
    voice->LoopEnd     = voice->NextBlock + loopend;
    voice->LoopSize    = absloopend - absloopstart;

    if ((loopstart >= (signed)data->size) || (loopstart < 0))
    {
        voice->LoopStart = NULL;
        voice->LoopEnd   = NULL;
        voice->BlockLength = length;
    }

    MV_SetVoicePitch(voice, format->nSamplesPerSec, pitchoffset);
    MV_SetVoiceVolume(voice, vol, left, right);
    MV_PlayVoice(voice);

    return(voice->handle);
}


/*---------------------------------------------------------------------
   Function: MV_PlayVOC3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int32_t MV_PlayVOC3D(char *ptr, int32_t pitchoffset, int32_t angle, int32_t distance, int32_t priority, uint32_t callbackval)
{
    int32_t left;
    int32_t right;
    int32_t mid;
    int32_t volume;
    int32_t status;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(MV_Error);
    }

    if (distance < 0)
    {
        distance  = -distance;
        angle    += MV_NumPanPositions / 2;
    }

    volume = MIX_VOLUME(distance);

    // Ensure angle is within 0 - 31
    angle &= MV_MaxPanPosition;

    left  = MV_PanTable[ angle ][ volume ].left;
    right = MV_PanTable[ angle ][ volume ].right;
    mid   = max(0, 255 - distance);

    status = MV_PlayVOC(ptr, pitchoffset, mid, left, right, priority, callbackval);

    return(status);
}


/*---------------------------------------------------------------------
   Function: MV_PlayVOC

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int32_t MV_PlayVOC(char *ptr, int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{
    int32_t status;

    status = MV_PlayLoopedVOC(ptr, -1, -1, pitchoffset, vol, left, right, priority, callbackval);
    if (status < MV_Ok)
    {
        Bsprintf(tempbuf, "Sound error %d: %s\n",callbackval, FX_ErrorString(FX_Error));
        initprintf(tempbuf);
    }

    return(status);
}


/*---------------------------------------------------------------------
   Function: MV_PlayLoopedVOC

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int32_t MV_PlayLoopedVOC(char *ptr, int32_t loopstart, int32_t loopend, int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{
    VoiceNode   *voice;
    int32_t status;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(MV_Error);
    }

    // Make sure it's a valid VOC file.
    status = strncmp(ptr, "Creative Voice File", 19);
    if (status != 0)
    {
        MV_SetErrorCode(MV_InvalidVOCFile);
        return(MV_Error);
    }

    // Request a voice from the voice pool
    voice = MV_AllocVoice(priority);
    if (voice == NULL)
    {
        MV_SetErrorCode(MV_NoVoices);
        return(MV_Error);
    }

    voice->wavetype    = VOC;
    voice->bits        = 8;
    voice->GetSound    = MV_GetNextVOCBlock;
    voice->NextBlock   = ptr + *(uint16_t *)(ptr + 0x14);
    voice->DemandFeed  = NULL;
    voice->LoopStart   = NULL;
    voice->LoopCount   = 0;
    voice->BlockLength = 0;
    voice->PitchScale  = PITCH_GetScale(pitchoffset);
    voice->length      = 0;
    voice->next        = NULL;
    voice->prev        = NULL;
    voice->priority    = priority;
    voice->callbackval = callbackval;
    voice->LoopStart   = (char *)loopstart;
    voice->LoopEnd     = (char *)loopend;
    voice->LoopSize    = loopend - loopstart + 1;

    if (loopstart < 0)
    {
        voice->LoopStart = NULL;
        voice->LoopEnd   = NULL;
    }

    MV_SetVoiceVolume(voice, vol, left, right);
    MV_PlayVoice(voice);

    return(voice->handle);
}

/*---------------------------------------------------------------------
   Function: MV_PlayLoopedOGG

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/
VoiceNode     *voice;

int32_t MV_PlayLoopedOGG(char *ptr, int32_t loopstart, int32_t loopend, int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{
    vorbis_info *vorbisInfo;
    int32_t length;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(MV_Error);
    }

    // Request a voice from the voice pool
    voice = MV_AllocVoice(priority);
    if (voice == NULL)
    {
        MV_SetErrorCode(MV_NoVoices);
        return(MV_Error);
    }

    voice->OGGstream.pos=0;
    voice->OGGstream.ptrsnd=ptr;
    voice->OGGstream.size=g_sounds[callbackval].soundsiz;
    voice->downsample=0;
    if (ov_open_callbacks(&voice->OGGstream,&voice->OGGstream.oggStream,0,0,cb)<0)
    {
        MV_SetErrorCode(MV_InvalidOGGFile);
        return(MV_Error);
    }

    vorbisInfo=ov_info(&voice->OGGstream.oggStream,-1);
    if (!vorbisInfo)
    {
        MV_SetErrorCode(MV_InvalidOGGFile);
        return(MV_Error);
    }
    while ((ogg_int64_t)(vorbisInfo->rate)/(1<<voice->downsample)*PITCH_GetScale(pitchoffset)/0x1000000/0x100)
        voice->downsample++;
    length=ov_pcm_total(&voice->OGGstream.oggStream,-1);
    if (!length)
        length=0xffffff;
    /*    if (length == OV_EINVAL)
        {
            MV_SetErrorCode(MV_InvalidOGGFile);
            return(MV_Error);
        }*/

    loopend=length=length>>voice->downsample;

    voice->wavetype    = OGG;
    voice->bits        = 16;
    voice->GetSound    = MV_GetNextOGGBlock;

    voice->Playing     = TRUE;
    voice->DemandFeed  = NULL;
    voice->LoopStart   = NULL;
    voice->LoopCount   = 0;
    voice->position    = 0;
    voice->length      = 0;
    voice->BlockLength = loopend;
    voice->NextBlock   = NULL;
    voice->next        = NULL;
    voice->prev        = NULL;
    voice->priority    = priority;
    voice->callbackval = callbackval;
    voice->LoopStart   = voice->NextBlock + loopstart+1;
    voice->LoopEnd     = voice->NextBlock + loopend+1;
    voice->LoopSize    = loopend - loopstart;

    if (loopstart < 0)
    {
        voice->LoopStart = NULL;
        voice->LoopEnd   = NULL;
        voice->BlockLength = length;
    }

    MV_SetVoicePitch(voice, vorbisInfo->rate>>voice->downsample, pitchoffset);
    if (vorbisInfo->channels==2)
        voice->downsample++;
    MV_SetVoiceVolume(voice, vol, left, right);
    MV_PlayVoice(voice);

    return(voice->handle);
}

/*---------------------------------------------------------------------
   Function: MV_PlayOGG

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int32_t MV_PlayOGG(char *ptr, int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{
    int32_t status;

    status = MV_PlayLoopedOGG(ptr, -1, -1, pitchoffset, vol, left, right, priority, callbackval);
    if (status < MV_Ok)
    {
        Bsprintf(tempbuf, "Sound error %d: %s\n",callbackval, FX_ErrorString(FX_Error));
        initprintf(tempbuf);
    }

    return(status);
}


/*---------------------------------------------------------------------
   Function: MV_PlayOGG3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int32_t MV_PlayOGG3D(char *ptr, int32_t pitchoffset, int32_t angle, int32_t distance, int32_t priority, uint32_t callbackval)
{
    int32_t left;
    int32_t right;
    int32_t mid;
    int32_t volume;
    int32_t status;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return(MV_Error);
    }

    if (distance < 0)
    {
        distance  = -distance;
        angle    += MV_NumPanPositions / 2;
    }

    volume = MIX_VOLUME(distance);

    // Ensure angle is within 0 - 31
    angle &= MV_MaxPanPosition;

    left  = MV_PanTable[ angle ][ volume ].left;
    right = MV_PanTable[ angle ][ volume ].right;
    mid   = max(0, 255 - distance);

    status = MV_PlayOGG(ptr, pitchoffset, mid, left, right, priority, callbackval);

    return(status);
}

/*---------------------------------------------------------------------
   Function: MV_CreateVolumeTable

   Create the table used to convert sound data to a specific volume
   level.
---------------------------------------------------------------------*/

void MV_CreateVolumeTable(int32_t index, int32_t volume, int32_t MaxVolume)
{
    int32_t val;
    int32_t level;
    int32_t i;

    level = (volume * MaxVolume) / MV_MaxTotalVolume;
    if (MV_Bits == 16)
    {
        for (i = 0; i < 65536; i += 256)
        {
            val   = i - 0x8000;
            val  *= level;
            val  /= MV_MaxVolume;
            MV_VolumeTable[ index ][ i / 256 ] = val;
        }
    }
    else
    {
        for (i = 0; i < 256; i++)
        {
            val   = i - 0x80;
            val  *= level;
            val  /= MV_MaxVolume;
            MV_VolumeTable[ volume ][ i ] = val;
        }
    }
}


/*---------------------------------------------------------------------
   Function: MV_CalcVolume

   Create the table used to convert sound data to a specific volume
   level.
---------------------------------------------------------------------*/

void MV_CalcVolume(int32_t MaxVolume)
{
    int32_t volume;

    for (volume = 0; volume < 128; volume++)
    {
        MV_HarshClipTable[ volume ] = 0;
        MV_HarshClipTable[ volume + 384 ] = 255;
    }
    for (volume = 0; volume < 256; volume++)
    {
        MV_HarshClipTable[ volume + 128 ] = volume;
    }

    // For each volume level, create a translation table with the
    // appropriate volume calculated.
    for (volume = 0; volume <= MV_MaxVolume; volume++)
    {
        MV_CreateVolumeTable(volume, volume, MaxVolume);
    }
}


/*---------------------------------------------------------------------
   Function: MV_CalcPanTable

   Create the table used to determine the stereo volume level of
   a sound located at a specific angle and distance from the listener.
---------------------------------------------------------------------*/

void MV_CalcPanTable(void)
{
    int32_t level;
    int32_t angle;
    int32_t distance;
    int32_t HalfAngle;
    int32_t ramp;

    HalfAngle = (MV_NumPanPositions / 2);

    for (distance = 0; distance <= MV_MaxVolume; distance++)
    {
        level = (255 * (MV_MaxVolume - distance)) / MV_MaxVolume;
        for (angle = 0; angle <= HalfAngle / 2; angle++)
        {
            ramp = level - ((level * angle) /
                            (MV_NumPanPositions / 4));

            MV_PanTable[ angle ][ distance ].left = ramp;
            MV_PanTable[ HalfAngle - angle ][ distance ].left = ramp;
            MV_PanTable[ HalfAngle + angle ][ distance ].left = level;
            MV_PanTable[ MV_MaxPanPosition - angle ][ distance ].left = level;

            MV_PanTable[ angle ][ distance ].right = level;
            MV_PanTable[ HalfAngle - angle ][ distance ].right = level;
            MV_PanTable[ HalfAngle + angle ][ distance ].right = ramp;
            MV_PanTable[ MV_MaxPanPosition - angle ][ distance ].right = ramp;
        }
    }
}


/*---------------------------------------------------------------------
   Function: MV_SetVolume

   Sets the volume of digitized sound playback.
---------------------------------------------------------------------*/

void MV_SetVolume(int32_t volume)
{
    volume = max(0, volume);
    volume = min(volume, MV_MaxTotalVolume);

    MV_TotalVolume = volume;

    // Calculate volume table
    MV_CalcVolume(volume);
}


/*---------------------------------------------------------------------
   Function: MV_GetVolume

   Returns the volume of digitized sound playback.
---------------------------------------------------------------------*/

int32_t MV_GetVolume(void)
{
    return(MV_TotalVolume);
}


/*---------------------------------------------------------------------
   Function: MV_SetCallBack

   Set the function to call when a voice stops.
---------------------------------------------------------------------*/

void MV_SetCallBack(void(*function)(uint32_t))
{
    MV_CallBackFunc = function;
}


/*---------------------------------------------------------------------
   Function: MV_SetReverseStereo

   Set the orientation of the left and right channels.
---------------------------------------------------------------------*/

void MV_SetReverseStereo(int32_t setting)
{
    MV_SwapLeftRight = setting;
}


/*---------------------------------------------------------------------
   Function: MV_GetReverseStereo

   Returns the orientation of the left and right channels.
---------------------------------------------------------------------*/

int32_t MV_GetReverseStereo(void)
{
    return(MV_SwapLeftRight);
}

#if 0
/*---------------------------------------------------------------------
   Function: MV_TestPlayback

   Checks if playback has started.
---------------------------------------------------------------------*/

int32_t MV_TestPlayback(void)
{
    unsigned flags;
    int32_t time;
    int32_t start;
    int32_t status;
    int32_t pos;

    flags = DisableInterrupts();
    _enable();

    status = MV_Error;
    start  = MV_MixPage;
    time   = clock() + CLOCKS_PER_SEC * 2;

    while (clock() < time)
    {
        if (MV_MixPage != start)
        {
            status = MV_Ok;
        }
    }

    RestoreInterrupts(flags);

    if (status != MV_Ok)
    {
        // Just in case an error doesn't get reported
        MV_SetErrorCode(MV_DMAFailure);

        switch (MV_SoundCard)
        {
        case SoundBlaster :
            pos = BLASTER_GetCurrentPos();
            break;

        default :
            MV_SetErrorCode(MV_UnsupportedCard);
            pos = -2;
            break;
        }

        if (pos > 0)
        {
            MV_SetErrorCode(MV_IrqFailure);
        }
        else if (pos == 0)
        {
            if (MV_Bits == 16)
            {
                MV_SetErrorCode(MV_DMA16Failure);
            }
            else
            {
                MV_SetErrorCode(MV_DMAFailure);
            }
        }
    }

    return(status);
}
#endif

/*---------------------------------------------------------------------
   Function: MV_Init

   Perform the initialization of variables and memory used by
   Multivoc.
---------------------------------------------------------------------*/

int32_t MV_Init(int32_t soundcard, int32_t MixRate, int32_t Voices, int32_t numchannels, int32_t samplebits)
{
    char *ptr;
    int32_t status;
    int32_t buffer;
    int32_t index;

    if (MV_Installed)
    {
        MV_Shutdown();
    }

    initprintf("Initializing MultiVoc... \n");

    MV_SetErrorCode(MV_Ok);

    MV_TotalMemory = Voices * sizeof(VoiceNode) + sizeof(HARSH_CLIP_TABLE_8);
    ptr = Bmalloc(MV_TotalMemory);
    if (ptr == NULL)
    {
        MV_SetErrorCode(MV_NoMem);
        return(MV_Error);
    }

    MV_Voices = (VoiceNode *)ptr;
    MV_HarshClipTable = ptr + (MV_TotalMemory - sizeof(HARSH_CLIP_TABLE_8));

    // Set number of voices before calculating volume table
    MV_MaxVoices = Voices;

//    initprintf("  - Maximum voices: %d\n", MV_MaxVoices);

    LL_Reset(&VoiceList, next, prev);
    LL_Reset(&VoicePool, next, prev);

    for (index = 0; index < Voices; index++)
    {
        LL_Add(&VoicePool, &MV_Voices[ index ], next, prev);
    }

    // Set the sampling rate
    MV_RequestedMixRate = MixRate;

    initprintf("  - %d voices, %d byte mixing buffers\n", MV_MaxVoices, MixBufferSize);

    // Allocate mix buffer within 1st megabyte
    // use calloc to clear the memory for the first playback.
    ptr = (char *)Bcalloc(1,TotalBufferSize + 8);	// FIXME: temporarily fixes bounds error somewhere...
    if (!ptr)
    {
        Bfree(MV_Voices);
        MV_Voices      = NULL;
        MV_TotalMemory = 0;

        MV_SetErrorCode(MV_NoMem);
        return(MV_Error);
    }

    MV_MixBufferPtr = ptr;

    MV_SetReverseStereo(FALSE);

    // Initialize the sound card
#if defined(_WIN32)
    status = DSOUND_Init(soundcard, MixRate, numchannels, samplebits, TotalBufferSize);
#else
    status = DSL_Init(soundcard, MixRate, numchannels, samplebits, TotalBufferSize);
#endif
    if (status != 0)
    {
        MV_SetErrorCode(MV_BlasterError);
    }

    if (MV_ErrorCode != MV_Ok)
    {
        status = MV_ErrorCode;

        Bfree(MV_Voices);
        MV_Voices      = NULL;
        MV_TotalMemory = 0;

        free(ptr);

        MV_SetErrorCode(status);
        return(MV_Error);
    }

    MV_SoundCard    = soundcard;
    MV_Installed    = TRUE;
    MV_CallBackFunc = NULL;
    MV_RecordFunc   = NULL;
    MV_Recording    = FALSE;
    MV_ReverbLevel  = 0;
    MV_ReverbTable  = NULL;

    // Set Mixer to play stereo digitized sound
    MV_SetMixMode(numchannels, samplebits);
    MV_ReverbDelay = MV_BufferSize * 3;

    //MV_MixBuffer[ MV_NumberOfBuffers ] = ptr;
    for (buffer = 0; buffer < MV_NumberOfBuffers; buffer++)
    {
        MV_MixBuffer[ buffer ] = ptr;
        ptr += MV_BufferSize;
    }

    // Calculate pan table
    MV_CalcPanTable();

    MV_SetVolume(MV_MaxTotalVolume);

    // Start the playback engine
    status = MV_StartPlayback();
    if (status != MV_Ok)
    {
        // Preserve error code while we shutdown.
        status = MV_ErrorCode;
        MV_Shutdown();
        MV_SetErrorCode(status);
        return(MV_Error);
    }

    return(MV_Ok);
}


/*---------------------------------------------------------------------
   Function: MV_Shutdown

   Restore any resources allocated by Multivoc back to the system.
---------------------------------------------------------------------*/

int32_t MV_Shutdown(void)
{
    int32_t buffer;
    unsigned flags;

    if (!MV_Installed)
    {
        return(MV_Ok);
    }

    initprintf("Uninitializing MultiVoc...\n");

    flags = DisableInterrupts();

    MV_KillAllVoices();

    MV_Installed = FALSE;

    // Stop the sound recording engine
    if (MV_Recording)
    {
        //MV_StopRecord();
    }

    // Stop the sound playback engine
    MV_StopPlayback();

    // G_Shutdown the sound card
#if defined(_WIN32)
    DSOUND_Shutdown();
#else
    DSL_Shutdown();
#endif

    RestoreInterrupts(flags);

    // Free any voices we allocated
    Bfree(MV_Voices);
    MV_Voices      = NULL;
    MV_TotalMemory = 0;

    LL_Reset(&VoiceList, next, prev);
    LL_Reset(&VoicePool, next, prev);

    MV_MaxVoices = 1;

    // Release the descriptor from our mix buffer
    if (MV_MixBufferPtr) free(MV_MixBufferPtr);
    MV_MixBufferPtr = NULL;

    for (buffer = 0; buffer < NumberOfBuffers; buffer++)
    {
        MV_MixBuffer[ buffer ] = NULL;
    }

    return(MV_Ok);
}


