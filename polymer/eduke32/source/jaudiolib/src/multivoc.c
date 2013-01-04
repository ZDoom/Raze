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
   module: MULTIVOC.C

   author: James R. Dose
   date:   December 20, 1993

   Routines to provide multichannel digitized sound playback for
   Sound Blaster compatible sound cards.

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "linklist.h"
#include "sndcards.h"
#include "drivers.h"
#include "pitch.h"
#include "multivoc.h"
#include "_multivc.h"

static void MV_Mix( VoiceNode *voice, int32_t buffer );
static void MV_StopVoice( VoiceNode *voice );
static void MV_ServiceVoc( void );

static VoiceNode *MV_GetVoice( int32_t handle );

static int16_t     *MV_GetVolumeTable( int32_t vol );

static void       MV_CalcVolume( int32_t MaxLevel );
static void       MV_CalcPanTable( void );

/*
#define RoundFixed( fixedval, bits )            \
        (                                       \
          (                                     \
            (fixedval) + ( 1 << ( (bits) - 1 ) )\
          ) >> (bits)                           \
        )
*/
#define IS_QUIET( ptr )  ( ( void * )( ptr ) == ( void * )&MV_VolumeTable[ 0 ] )

static int32_t       MV_ReverbLevel;
static int32_t       MV_ReverbDelay;
static VOLUME16 *MV_ReverbTable = NULL;

static int16_t MV_VolumeTable[ MV_MAXVOLUME + 1 ][ 256 ];
Pan MV_PanTable[ MV_NUMPANPOSITIONS ][ MV_MAXVOLUME + 1 ];

int32_t MV_Installed   = FALSE;
static int32_t MV_TotalVolume = MV_MAXTOTALVOLUME;
static int32_t MV_MaxVoices   = 1;

static int32_t MV_BufferSize = MV_MIXBUFFERSIZE;
static int32_t MV_BufferLength;

static int32_t MV_NumberOfBuffers = MV_NUMBEROFBUFFERS;

static int32_t MV_MixMode    = MONO_8BIT;
static int32_t MV_Channels   = 1;
static int32_t MV_Bits       = 8;

static int32_t MV_Silence    = SILENCE_8BIT;
static int32_t MV_SwapLeftRight = FALSE;

static int32_t MV_RequestedMixRate;
int32_t MV_MixRate;

static int32_t MV_BuffShift;

static int32_t MV_TotalMemory;

static int32_t   MV_BufferEmpty[ MV_NUMBEROFBUFFERS ];
char *MV_MixBuffer[ MV_NUMBEROFBUFFERS + 1 ];

static VoiceNode *MV_Voices = NULL;

static volatile VoiceNode VoiceList;
static volatile VoiceNode VoicePool;

static int32_t MV_MixPage      = 0;
static int32_t MV_VoiceHandle  = MV_MINVOICEHANDLE;

void (*MV_Printf)(const char *fmt, ...) = NULL;
static void (*MV_CallBackFunc)(uint32_t) = NULL;
static void (*MV_MixFunction)(VoiceNode *voice, int32_t buffer);

char  *MV_HarshClipTable;
char  *MV_MixDestination;
const int16_t *MV_LeftVolume;
const int16_t *MV_RightVolume;
int32_t    MV_SampleSize = 1;
int32_t    MV_RightChannelOffset;

uint32_t MV_MixPosition;

int32_t MV_ErrorCode = MV_Ok;

static int32_t lockdepth = 0;

static inline void DisableInterrupts(void)
{
    if (lockdepth++ > 0)
        return;
    SoundDriver_Lock();
    return;
}

static inline void RestoreInterrupts(void)
{
    if (--lockdepth > 0)
        return;
    SoundDriver_Unlock();
}


/*---------------------------------------------------------------------
   Function: MV_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/

const char *MV_ErrorString(int32_t ErrorNumber)
{
    switch (ErrorNumber)
    {
    case MV_Warning :
    case MV_Error :
        return MV_ErrorString(MV_ErrorCode);

    case MV_Ok :
        return "Multivoc ok.";

    case MV_UnsupportedCard :
        return "Selected sound card is not supported by Multivoc.";

    case MV_NotInstalled :
        return "Multivoc not installed.";

    case MV_DriverError :
        return SoundDriver_ErrorString(SoundDriver_GetError());

    case MV_NoVoices :
        return "No free voices available to Multivoc.";

    case MV_NoMem :
        return "Out of memory in Multivoc.";

    case MV_VoiceNotFound :
        return "No voice with matching handle found.";

    case MV_InvalidVOCFile :
        return "Invalid VOC file passed in to Multivoc.";

    case MV_InvalidWAVFile :
        return "Invalid WAV file passed in to Multivoc.";

    case MV_InvalidVorbisFile :
        return "Invalid OggVorbis file passed in to Multivoc.";

    case MV_InvalidFLACFile :
        return "Invalid FLAC file passed in to Multivoc.";

    case MV_InvalidMixMode :
        return "Invalid mix mode request in Multivoc.";

    default :
        return "Unknown Multivoc error code.";
    }
}


/*---------------------------------------------------------------------
   Function: MV_Mix

   Mixes the sound into the buffer.
---------------------------------------------------------------------*/

static void MV_Mix(VoiceNode *voice,int32_t buffer)
{
    const char          *start;
    int32_t            length;
    int32_t            voclength;
    uint32_t   position;
    uint32_t   rate;
    uint32_t   FixedPointBufferSize;

    /* cheap fix for a crash under 64-bit linux */
    /*                            v  v  v  v    */
    if (voice->length == 0 && (!voice->GetSound || voice->GetSound(voice) != KeepPlaying))
        return;

    length               = MV_MIXBUFFERSIZE;
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
                voclength = (voice->length - position + rate - voice->channels) / rate;
            }
            else
            {
                voice->GetSound(voice);
                return;
            }
        }
        else
            voclength = length;

        if (voice->mix)
            voice->mix(position, rate, start, voclength);

        voice->position = MV_MixPosition;

        length -= voclength;

        if (voice->position >= voice->length)
        {
            // Get the next block of sound
            if (voice->GetSound(voice) != KeepPlaying)
                return;

            if (length > (voice->channels - 1))
            {
                // Get the position of the last sample in the buffer
                FixedPointBufferSize = voice->RateScale * (length - voice->channels);
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
    DisableInterrupts();
    LL_SortedInsertion(&VoiceList, voice, prev, next, VoiceNode, priority);
    RestoreInterrupts();
}


/*---------------------------------------------------------------------
   Function: MV_StopVoice

   Removes the voice from the play list and adds it to the free list.
---------------------------------------------------------------------*/

static void MV_StopVoice(VoiceNode *voice)
{
    DisableInterrupts();

    // move the voice from the play list to the free list
    LL_Remove(voice, next, prev);
    LL_Add((VoiceNode*) &VoicePool, voice, next, prev);

    RestoreInterrupts();

#ifdef HAVE_VORBIS
    if (voice->wavetype == Vorbis)
        MV_ReleaseVorbisVoice(voice);
#endif
#ifdef HAVE_FLAC
    if (voice->wavetype == FLAC)
        MV_ReleaseFLACVoice(voice);
#endif

    voice->handle = 0;
}


/*---------------------------------------------------------------------
   Function: MV_ServiceVoc

   Starts playback of the waiting buffer and mixes the next one.

   JBF: no synchronisation happens inside MV_ServiceVoc nor the
        supporting functions it calls. This would cause a deadlock
        between the mixer thread in the driver vs the nested
        locking in the user-space functions of MultiVoc. The call
        to MV_ServiceVoc is synchronised in the driver.
---------------------------------------------------------------------*/
static void MV_ServiceVoc(void)
{
    VoiceNode *voice;
    VoiceNode *next;
    //int32_t        flags;
    int32_t iter;

    // Toggle which buffer we'll mix next
    if (++MV_MixPage >= MV_NumberOfBuffers)
        MV_MixPage -= MV_NumberOfBuffers;

    if (MV_ReverbLevel == 0)
    {
        // Initialize buffer
        //Commented out so that the buffer is always cleared.
        //This is so the guys at Echo Speech can mix into the
        //buffer even when no sounds are playing.
        if (!MV_BufferEmpty[MV_MixPage])
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
        int32_t   count;
        int32_t   length;

        end = MV_MixBuffer[ 0 ] + MV_BufferLength;
        dest = MV_MixBuffer[ MV_MixPage ];
        source = MV_MixBuffer[ MV_MixPage ] - MV_ReverbDelay;

        if (source < MV_MixBuffer[ 0 ])
            source += MV_BufferLength;

        length = MV_BufferSize;
        while (length > 0)
        {
            count = length;
            if (source + count > end)
                count = end - source;

            if (MV_Bits == 16)
            {
                if (MV_ReverbTable != NULL)
                    MV_16BitReverb(source, dest, MV_ReverbTable, count / 2);
                else
                    MV_16BitReverbFast(source, dest, count / 2, MV_ReverbLevel);
            }
            else // if (MV_Bits == 8)
            {
                if (MV_ReverbTable != NULL)
                    MV_8BitReverb((int8_t *) source, (int8_t *) dest, MV_ReverbTable, count);
                else
                    MV_8BitReverbFast((int8_t *) source, (int8_t *) dest, count, MV_ReverbLevel);
            }

            // if we go through the loop again, it means that we've wrapped around the buffer
            source  = MV_MixBuffer[ 0 ];
            dest   += count;
            length -= count;
        }
    }

    // Play any waiting voices
    //DisableInterrupts();

    if (!VoiceList.next || (voice = VoiceList.next) == &VoiceList)
        return;

    iter = 0;

    do
    {
        next = voice->next;
        iter++;

        if (iter > MV_MaxVoices && MV_Printf)
            MV_Printf("more iterations than voices! iter: %d\n",iter);

        if (voice->Paused)
            continue;

        MV_BufferEmpty[ MV_MixPage ] = FALSE;

        MV_MixFunction(voice, MV_MixPage);


        // Is this voice done?
        if (!voice->Playing)
        {
            //JBF: prevent a deadlock caused by MV_StopVoice grabbing the mutex again
            //MV_StopVoice( voice );
            LL_Remove(voice, next, prev);
            LL_Add((VoiceNode*) &VoicePool, voice, next, prev);
#ifdef HAVE_VORBIS
            if (voice->wavetype == Vorbis)
                MV_ReleaseVorbisVoice(voice);
#endif
#ifdef HAVE_FLAC
            if (voice->wavetype == FLAC)
                MV_ReleaseFLACVoice(voice);
#endif
            voice->handle = 0;

            if (MV_CallBackFunc)
                MV_CallBackFunc(voice->callbackval);
        }
    }
    while ((voice = next) != &VoiceList);

    //RestoreInterrupts();
}


/*---------------------------------------------------------------------
   Function: MV_GetVoice

   Locates the voice with the specified handle.
---------------------------------------------------------------------*/

static VoiceNode *MV_GetVoice(int32_t handle)
{
    VoiceNode *voice;

    if (handle < MV_MINVOICEHANDLE || handle > MV_MaxVoices)
    {
        if (MV_Printf)
            MV_Printf("MV_GetVoice(): bad handle (%d)!\n", handle);
        return NULL;
    }

    DisableInterrupts();

    for (voice = VoiceList.next; voice != &VoiceList; voice = voice->next)
    {
        if (handle == voice->handle)
        {
            RestoreInterrupts();
            return voice;
        }
    }

    RestoreInterrupts();
    MV_SetErrorCode(MV_VoiceNotFound);
    return NULL;
}


/*---------------------------------------------------------------------
   Function: MV_VoicePlaying

   Checks if the voice associated with the specified handle is
   playing.
---------------------------------------------------------------------*/

int32_t MV_VoicePlaying(int32_t handle)
{
    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return FALSE;
    }

    return MV_GetVoice(handle) ? TRUE : FALSE;
}


/*---------------------------------------------------------------------
   Function: MV_KillAllVoices

   Stops output of all currently active voices.
---------------------------------------------------------------------*/

int32_t MV_KillAllVoices(void)
{
    VoiceNode * voice;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return MV_Error;
    }

    DisableInterrupts();

    if (&VoiceList == VoiceList.next)
    {
        RestoreInterrupts();
        return MV_Ok;
    }

    voice = VoiceList.prev;

    // Remove all the voices from the list
    while (voice != &VoiceList)
    {
        if (voice->priority == MV_MUSIC_PRIORITY)
        {
            voice = voice->prev;
            continue;
        }

        MV_Kill(voice->handle);
        voice = VoiceList.prev;
    }

    RestoreInterrupts();

    return MV_Ok;
}


/*---------------------------------------------------------------------
   Function: MV_Kill

   Stops output of the voice associated with the specified handle.
---------------------------------------------------------------------*/

int32_t MV_Kill(int32_t handle)
{
    VoiceNode *voice;
    uint32_t callbackval;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return MV_Error;
    }

    DisableInterrupts();

    if ((voice = MV_GetVoice(handle)) == NULL)
    {
        RestoreInterrupts();
        MV_SetErrorCode(MV_VoiceNotFound);
        return MV_Error;
    }

    callbackval = voice->callbackval;

    MV_StopVoice(voice);

    RestoreInterrupts();

    if (MV_CallBackFunc)
        MV_CallBackFunc(callbackval);

    return MV_Ok;
}


/*---------------------------------------------------------------------
   Function: MV_VoicesPlaying

   Determines the number of currently active voices.
---------------------------------------------------------------------*/

int32_t MV_VoicesPlaying(void)
{
    VoiceNode   *voice;
    int32_t     NumVoices = 0;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return 0;
    }

    DisableInterrupts();

    for (voice = VoiceList.next; voice != &VoiceList; voice = voice->next)
        NumVoices++;

    RestoreInterrupts();

    return NumVoices;
}


/*---------------------------------------------------------------------
   Function: MV_AllocVoice

   Retrieve an inactive or lower priority voice for output.
---------------------------------------------------------------------*/

VoiceNode *MV_AllocVoice(int32_t priority)
{
    VoiceNode   *voice;
    VoiceNode   *node;

    DisableInterrupts();

    // Check if we have any free voices
    if (LL_Empty(&VoicePool, next, prev))
    {
        // check if we have a higher priority than a voice that is playing.
        for (voice = node = VoiceList.next; node != &VoiceList; node = node->next)
        {
            if (node->priority < voice->priority)
                voice = node;
        }

        if (priority >= voice->priority && voice != &VoiceList && voice->handle >= MV_MINVOICEHANDLE)
            MV_Kill(voice->handle);

        if (LL_Empty(&VoicePool, next, prev))
        {
            // No free voices
            RestoreInterrupts();
            return NULL;
        }
    }

    voice = VoicePool.next;
    LL_Remove(voice, next, prev);
    RestoreInterrupts();

    MV_VoiceHandle = MV_MINVOICEHANDLE;

    // Find a free voice handle
    do
    {
        if (++MV_VoiceHandle < MV_MINVOICEHANDLE || MV_VoiceHandle > MV_MaxVoices)
            MV_VoiceHandle = MV_MINVOICEHANDLE;
    }
    while (MV_VoicePlaying(MV_VoiceHandle));

    voice->handle = MV_VoiceHandle;

    return voice;
}


/*---------------------------------------------------------------------
   Function: MV_VoiceAvailable

   Checks if a voice can be played at the specified priority.
---------------------------------------------------------------------*/

int32_t MV_VoiceAvailable(int32_t priority)
{
    VoiceNode   *voice;
    VoiceNode   *node;

    // Check if we have any free voices
    if (!LL_Empty(&VoicePool, next, prev))
        return TRUE;

    DisableInterrupts();

    // check if we have a higher priority than a voice that is playing.
    for (voice = node = VoiceList.next; node != &VoiceList; node = node->next)
    {
        if (node->priority < voice->priority)
            voice = node;
    }

    if ((voice == &VoiceList) || (priority < voice->priority))
    {
        RestoreInterrupts();
        return FALSE;
    }

    RestoreInterrupts();
    return TRUE;
}


/*---------------------------------------------------------------------
   Function: MV_SetVoicePitch

   Sets the pitch for the specified voice.
---------------------------------------------------------------------*/

void MV_SetVoicePitch
(
    VoiceNode *voice,
    uint32_t rate,
    int32_t pitchoffset
)

{
    voice->SamplingRate = rate;
    voice->PitchScale   = PITCH_GetScale(pitchoffset);
    voice->RateScale    = (rate * voice->PitchScale) / MV_MixRate;

    // Multiply by MV_MIXBUFFERSIZE - 1
    voice->FixedPointBufferSize = (voice->RateScale * MV_MIXBUFFERSIZE) -
                                  voice->RateScale;
}


/*---------------------------------------------------------------------
   Function: MV_SetPitch

   Sets the pitch for the voice associated with the specified handle.
---------------------------------------------------------------------*/

int32_t MV_SetPitch
(
    int32_t handle,
    int32_t pitchoffset
)

{
    VoiceNode *voice;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return MV_Error;
    }

    voice = MV_GetVoice(handle);
    if (voice == NULL)
    {
        MV_SetErrorCode(MV_VoiceNotFound);
        return MV_Error;
    }

    MV_SetVoicePitch(voice, voice->SamplingRate, pitchoffset);

    return MV_Ok;
}


/*---------------------------------------------------------------------
   Function: MV_SetFrequency

   Sets the frequency for the voice associated with the specified handle.
---------------------------------------------------------------------*/

int32_t MV_SetFrequency
(
    int32_t handle,
    int32_t frequency
)

{
    VoiceNode *voice;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return MV_Error;
    }

    voice = MV_GetVoice(handle);
    if (voice == NULL)
    {
        MV_SetErrorCode(MV_VoiceNotFound);
        return MV_Error;
    }

    MV_SetVoicePitch(voice, frequency, 0);

    return MV_Ok;
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

    table = (int16_t *) &MV_VolumeTable[ volume ];

    return table;
}


/*---------------------------------------------------------------------
   Function: MV_SetVoiceMixMode

   Selects which method should be used to mix the voice.

 8Bit  16Bit  8Bit  16Bit |  8Bit  16Bit  8Bit  16Bit |
 Mono  Mono   Ster  Ster  |  Mono  Mono   Ster  Ster  |  Mixer
 Out   Out    Out   Out   |  In    In     In    In    |
--------------------------+---------------------------+-------------
  X                       |         X                 | Mix8BitMono16
  X                       |   X                       | Mix8BitMono
               X          |         X                 | Mix8BitStereo16
               X          |   X                       | Mix8BitStereo
        X                 |         X                 | Mix16BitMono16
        X                 |   X                       | Mix16BitMono
                     X    |         X                 | Mix16BitStereo16
                     X    |   X                       | Mix16BitStereo
--------------------------+---------------------------+-------------
                     X    |                      X    | Mix16BitStereo16Stereo
                     X    |                X          | Mix16BitStereo8Stereo
               X          |                      X    | Mix8BitStereo16Stereo
               X          |                X          | Mix8BitStereo8Stereo
        X                 |                      X    | Mix16BitMono16Stereo
        X                 |                X          | Mix16BitMono8Stereo
  X                       |                      X    | Mix8BitMono16Stereo
  X                       |                X          | Mix8BitMono8Stereo

---------------------------------------------------------------------*/

void MV_SetVoiceMixMode(VoiceNode *voice)
{
    //int32_t flags;
    int32_t test;

    //DisableInterrupts();

    test = T_DEFAULT;
    if (MV_Bits == 8)
    {
        test |= T_8BITS;
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

    if (voice->bits == 16)
    {
        test |= T_16BITSOURCE;
    }

    if (voice->channels == 2)
    {
        test |= T_STEREOSOURCE;
        test &= ~(T_RIGHTQUIET | T_LEFTQUIET);
    }

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

    case T_16BITSOURCE | T_STEREOSOURCE:
        voice->mix = MV_Mix16BitStereo16Stereo;
        break;

    case T_16BITSOURCE | T_STEREOSOURCE | T_8BITS:
        voice->mix = MV_Mix8BitStereo16Stereo;
        break;

    case T_16BITSOURCE | T_STEREOSOURCE | T_MONO:
        voice->mix = MV_Mix16BitMono16Stereo;
        break;

    case T_16BITSOURCE | T_STEREOSOURCE | T_8BITS | T_MONO:
        voice->mix = MV_Mix8BitMono16Stereo;
        break;

    case T_STEREOSOURCE:
        voice->mix = MV_Mix16BitStereo8Stereo;
        break;

    case T_STEREOSOURCE | T_8BITS:
        voice->mix = MV_Mix8BitStereo8Stereo;
        break;

    case T_STEREOSOURCE | T_MONO:
        voice->mix = MV_Mix16BitMono8Stereo;
        break;

    case T_STEREOSOURCE | T_8BITS | T_MONO:
        voice->mix = MV_Mix8BitMono8Stereo;
        break;

    default :
        voice->mix = 0;
    }

    //RestoreInterrupts( flags );
}


/*---------------------------------------------------------------------
   Function: MV_SetVoiceVolume

   Sets the stereo and mono volume level of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

void MV_SetVoiceVolume
(
    VoiceNode *voice,
    int32_t vol,
    int32_t left,
    int32_t right
)

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
Function: MV_PauseVoice

Pauses the voice associated with the specified handle
without stoping the sound.
---------------------------------------------------------------------*/

int32_t MV_PauseVoice
(
    int32_t handle,
    int32_t pause
)

{
    VoiceNode *voice;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return MV_Error;
    }

    DisableInterrupts();

    voice = MV_GetVoice(handle);
    if (voice == NULL)
    {
        RestoreInterrupts();
        MV_SetErrorCode(MV_VoiceNotFound);
        return MV_Warning;
    }

    voice->Paused = pause;

    RestoreInterrupts();

    return MV_Ok;
}


/*---------------------------------------------------------------------
   Function: MV_EndLooping

   Stops the voice associated with the specified handle from looping
   without stoping the sound.
---------------------------------------------------------------------*/

int32_t MV_EndLooping(int32_t handle)
{
    VoiceNode *voice;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return MV_Error;
    }

    DisableInterrupts();

    voice = MV_GetVoice(handle);
    if (voice == NULL)
    {
        RestoreInterrupts();
        MV_SetErrorCode(MV_VoiceNotFound);
        return MV_Warning;
    }

    voice->LoopCount = 0;
    voice->LoopStart = NULL;
    voice->LoopEnd   = NULL;

    RestoreInterrupts();

    return MV_Ok;
}


/*---------------------------------------------------------------------
   Function: MV_SetPan

   Sets the stereo and mono volume level of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int32_t MV_SetPan
(
    int32_t handle,
    int32_t vol,
    int32_t left,
    int32_t right
)

{
    VoiceNode *voice;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return MV_Error;
    }

    voice = MV_GetVoice(handle);
    if (voice == NULL)
    {
        MV_SetErrorCode(MV_VoiceNotFound);
        return MV_Warning;
    }

    MV_SetVoiceVolume(voice, vol, left, right);

    return MV_Ok;
}


/*---------------------------------------------------------------------
   Function: MV_Pan3D

   Set the angle and distance from the listener of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int32_t MV_Pan3D
(
    int32_t handle,
    int32_t angle,
    int32_t distance
)

{
    int32_t left;
    int32_t right;
    int32_t mid;
    int32_t volume;
    int32_t status;

    if (distance < 0)
    {
        distance  = -distance;
        angle    += MV_NUMPANPOSITIONS / 2;
    }

    volume = MIX_VOLUME(distance);

    // Ensure angle is within 0 - 127
    angle &= MV_MAXPANPOSITION;

    left  = MV_PanTable[ angle ][ volume ].left;
    right = MV_PanTable[ angle ][ volume ].right;
    mid   = max(0, 255 - distance);

    status = MV_SetPan(handle, mid, left, right);

    return status;
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

    maxdelay = MV_MIXBUFFERSIZE * MV_NumberOfBuffers;

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
    MV_ReverbDelay = max(MV_MIXBUFFERSIZE, min(delay, maxdelay));
    MV_ReverbDelay *= MV_SampleSize;
}


/*---------------------------------------------------------------------
   Function: MV_SetMixMode

   Prepares Multivoc to play stereo of mono digitized sounds.
---------------------------------------------------------------------*/

static int32_t MV_SetMixMode
(
    int32_t numchannels,
    int32_t samplebits
)

{
    int32_t mode;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return MV_Error;
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

    MV_MixMode = mode;

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

    MV_BufferSize = MV_MIXBUFFERSIZE * MV_SampleSize;
    MV_NumberOfBuffers = MV_TOTALBUFFERSIZE / MV_BufferSize;
    MV_BufferLength = MV_TOTALBUFFERSIZE;

    MV_RightChannelOffset = MV_SampleSize / 2;

    return MV_Ok;
}


/*---------------------------------------------------------------------
   Function: MV_StartPlayback

   Starts the sound playback engine.
---------------------------------------------------------------------*/

static int32_t MV_StartPlayback(void)
{
    int32_t status;
    int32_t buffer;

    // Initialize the buffers
    ClearBuffer_DW(MV_MixBuffer[ 0 ], MV_Silence, MV_TOTALBUFFERSIZE >> 2);
    for (buffer = 0; buffer < MV_NumberOfBuffers; buffer++)
    {
        MV_BufferEmpty[ buffer ] = TRUE;
    }

    // Set the mix buffer variables
    MV_MixPage = 1;

    MV_MixFunction = MV_Mix;

//JIM
//   MV_MixRate = MV_RequestedMixRate;
//   return MV_Ok;

    // Start playback
    status = SoundDriver_BeginPlayback(MV_MixBuffer[0], MV_BufferSize,
                                       MV_NumberOfBuffers, MV_ServiceVoc);
    if (status != MV_Ok)
    {
        MV_SetErrorCode(MV_DriverError);
        return MV_Error;
    }

    MV_MixRate = MV_RequestedMixRate;

    return MV_Ok;
}


/*---------------------------------------------------------------------
   Function: MV_StopPlayback

   Stops the sound playback engine.
---------------------------------------------------------------------*/

static void MV_StopPlayback(void)
{
    VoiceNode   *voice;
    VoiceNode   *next;

    // Stop sound playback
    SoundDriver_StopPlayback();

    // Make sure all callbacks are done.
    DisableInterrupts();

    for (voice = VoiceList.next; voice != &VoiceList; voice = next)
    {
        next = voice->next;

        MV_StopVoice(voice);

        if (MV_CallBackFunc)
            MV_CallBackFunc(voice->callbackval);
    }

    RestoreInterrupts();
}


/*---------------------------------------------------------------------
   Function: MV_CreateVolumeTable

   Create the table used to convert sound data to a specific volume
   level.
---------------------------------------------------------------------*/

static void MV_CreateVolumeTable
(
    int32_t index,
    int32_t volume,
    int32_t MaxVolume
)

{
    int32_t val;
    int32_t level;
    int32_t i;

    level = (volume * MaxVolume) / MV_MAXTOTALVOLUME;
    if (MV_Bits == 16)
    {
        for (i = 0; i < 65536; i += 256)
        {
            val   = i - 0x8000;
            val  *= level;
            val  /= MV_MAXVOLUME;
            MV_VolumeTable[ index ][ i / 256 ] = val;
        }
    }
    else
    {
        for (i = 0; i < 256; i++)
        {
            val   = i - 0x80;
            val  *= level;
            val  /= MV_MAXVOLUME;
            MV_VolumeTable[ volume ][ i ] = val;
        }
    }
}


/*---------------------------------------------------------------------
   Function: MV_CalcVolume

   Create the table used to convert sound data to a specific volume
   level.
---------------------------------------------------------------------*/

static void MV_CalcVolume(int32_t MaxVolume)
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
    for (volume = 0; volume <= MV_MAXVOLUME; volume++)
    {
        MV_CreateVolumeTable(volume, volume, MaxVolume);
    }
}


/*---------------------------------------------------------------------
   Function: MV_CalcPanTable

   Create the table used to determine the stereo volume level of
   a sound located at a specific angle and distance from the listener.
---------------------------------------------------------------------*/

static void MV_CalcPanTable(void)
{
    int32_t   level;
    int32_t   angle;
    int32_t   distance;
    int32_t   HalfAngle;
    int32_t   ramp;

    HalfAngle = (MV_NUMPANPOSITIONS / 2);

    for (distance = 0; distance <= MV_MAXVOLUME; distance++)
    {
        level = (255 * (MV_MAXVOLUME - distance)) / MV_MAXVOLUME;
        for (angle = 0; angle <= HalfAngle / 2; angle++)
        {
            ramp = level - ((level * angle) /
                            (MV_NUMPANPOSITIONS / 4));

            MV_PanTable[ angle ][ distance ].left = ramp;
            MV_PanTable[ HalfAngle - angle ][ distance ].left = ramp;
            MV_PanTable[ HalfAngle + angle ][ distance ].left = level;
            MV_PanTable[ MV_MAXPANPOSITION - angle ][ distance ].left = level;

            MV_PanTable[ angle ][ distance ].right = level;
            MV_PanTable[ HalfAngle - angle ][ distance ].right = level;
            MV_PanTable[ HalfAngle + angle ][ distance ].right = ramp;
            MV_PanTable[ MV_MAXPANPOSITION - angle ][ distance ].right = ramp;
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
    volume = min(volume, MV_MAXTOTALVOLUME);

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
    return MV_TotalVolume;
}


/*---------------------------------------------------------------------
   Function: MV_SetCallBack

   Set the function to call when a voice stops.
---------------------------------------------------------------------*/

void MV_SetCallBack(void (*function)(uint32_t))
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
    return MV_SwapLeftRight;
}


/*---------------------------------------------------------------------
   Function: MV_Init

   Perform the initialization of variables and memory used by
   Multivoc.
---------------------------------------------------------------------*/

int32_t MV_Init
(
    int32_t soundcard,
    int32_t MixRate,
    int32_t Voices,
    int32_t numchannels,
    int32_t samplebits,
    void * initdata
)

{
    char *ptr;
    int32_t  status;
    int32_t  buffer;
    int32_t  index;

    if (MV_Installed)
    {
        MV_Shutdown();
    }

    MV_SetErrorCode(MV_Ok);

    // MV_TotalMemory + 2: FIXME
    //  Thread 3:
    //  Invalid read of size 2
    //     at 0x8730513: MV_Mix16BitStereo16Stereo (mixst.c:272)
    //     by 0x872A5A2: MV_Mix (multivoc.c:285)
    //     by 0x872B0EA: MV_ServiceVoc (multivoc.c:449)
    //     by 0x87342C1: fillData (driver_sdl.c:80)
    //     by 0x428F2AD: ??? (in /usr/lib/libSDL_mixer-1.2.so.0.2.6)
    //             . . .
    //   Address 0x11e9fa10 is 0 bytes after a block of size 9,728 alloc'd
    //     at 0x402732C: calloc (vg_replace_malloc.c:467)
    //     by 0x87288C8: MV_Init (multivoc.c:2528)
    //     by 0x871BD20: FX_Init (fx_man.c:160)
    //     by 0x84597CA: S_SoundStartup (sounds.c:62)
    //     by 0x80D7869: app_main (game.c:10378)
    //     by 0x870C9C0: main (sdlayer.c:222)
    MV_TotalMemory = Voices * sizeof(VoiceNode) + sizeof(HARSH_CLIP_TABLE_8) + MV_TOTALBUFFERSIZE + 2;

    ptr = (char *) calloc(1, MV_TotalMemory);
    if (!ptr)
    {
        MV_SetErrorCode(MV_NoMem);
        return MV_Error;
    }

    MV_Voices = (VoiceNode *)ptr;
    ptr += Voices * sizeof(VoiceNode);

    MV_HarshClipTable = ptr;
    ptr += sizeof(HARSH_CLIP_TABLE_8);

    // Set number of voices before calculating volume table
    MV_MaxVoices = Voices;

    LL_Reset((VoiceNode*) &VoiceList, next, prev);
    LL_Reset((VoiceNode*) &VoicePool, next, prev);

    for (index = 0; index < Voices; index++)
    {
        LL_Add((VoiceNode*) &VoicePool, &MV_Voices[ index ], next, prev);
    }

    MV_SetReverseStereo(FALSE);

    ASS_SoundDriver = soundcard;

    // Initialize the sound card
    status = SoundDriver_Init(&MixRate, &numchannels, &samplebits, initdata);
    if (status != MV_Ok)
    {
        MV_SetErrorCode(MV_DriverError);
    }

    if (MV_ErrorCode != MV_Ok)
    {
        status = MV_ErrorCode;

        free(MV_Voices);
        MV_Voices      = NULL;
        MV_HarshClipTable = NULL;
        MV_TotalMemory = 0;

        MV_SetErrorCode(status);
        return MV_Error;
    }

    MV_Installed    = TRUE;
    MV_CallBackFunc = NULL;
    MV_ReverbLevel  = 0;
    MV_ReverbTable  = NULL;

    // Set the sampling rate
    MV_RequestedMixRate = MixRate;

    // Set Mixer to play stereo digitized sound
    MV_SetMixMode(numchannels, samplebits);
    MV_ReverbDelay = MV_BufferSize * 3;

    // Make sure we don't cross a physical page
    MV_MixBuffer[ MV_NumberOfBuffers ] = ptr;
    for (buffer = 0; buffer < MV_NumberOfBuffers; buffer++)
    {
        MV_MixBuffer[ buffer ] = ptr;
        ptr += MV_BufferSize;
    }

    // Calculate pan table
    MV_CalcPanTable();

    MV_SetVolume(MV_MAXTOTALVOLUME);

    // Start the playback engine
    status = MV_StartPlayback();
    if (status != MV_Ok)
    {
        // Preserve error code while we shutdown.
        status = MV_ErrorCode;
        MV_Shutdown();
        MV_SetErrorCode(status);
        return MV_Error;
    }

    return MV_Ok;
}


/*---------------------------------------------------------------------
   Function: MV_Shutdown

   Restore any resources allocated by Multivoc back to the system.
---------------------------------------------------------------------*/

int32_t MV_Shutdown(void)
{
    int32_t      buffer;

    if (!MV_Installed)
    {
        return MV_Ok;
    }

    MV_KillAllVoices();

    MV_Installed = FALSE;

    // Stop the sound playback engine
    MV_StopPlayback();

    // Shutdown the sound card
    SoundDriver_Shutdown();

    // Free any voices we allocated
    free(MV_Voices);
    MV_Voices      = NULL;
    MV_TotalMemory = 0;

    LL_Reset((VoiceNode*) &VoiceList, next, prev);
    LL_Reset((VoiceNode*) &VoicePool, next, prev);

    MV_MaxVoices = 1;

    // Release the descriptor from our mix buffer
    for (buffer = 0; buffer < MV_NUMBEROFBUFFERS; buffer++)
    {
        MV_MixBuffer[ buffer ] = NULL;
    }

    return MV_Ok;
}

int32_t MV_SetVoiceCallback(int32_t handle, uint32_t callbackval)
{
    VoiceNode *voice;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return MV_Error;
    }

    DisableInterrupts();

    if ((voice = MV_GetVoice(handle)) == NULL)
    {
        RestoreInterrupts();
        MV_SetErrorCode(MV_VoiceNotFound);
        return MV_Error;
    }

    voice->callbackval = callbackval;

    RestoreInterrupts();

    return MV_Ok;
}

void MV_SetPrintf(void (*function)(const char *, ...))
{
    MV_Printf = function;
}


const char *loopStartTags[loopStartTagCount] = {
    "LOOP_START",
    "LOOPSTART"
};
const char *loopEndTags[loopEndTagCount] = {
    "LOOP_END",
    "LOOPEND"
};
const char *loopLengthTags[loopLengthTagCount] = {
    "LOOP_LENGTH",
    "LOOPLENGTH"
};


// vim:ts=3:expandtab:
