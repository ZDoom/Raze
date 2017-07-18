/*
Copyright (C) 1994-1995 Apogee Software, Ltd.
Copyright (C) 2015 EDuke32 developers
Copyright (C) 2015 Voidpoint, LLC

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
   module: MULTIVOC.C

   author: James R. Dose
   date:   December 20, 1993

   Routines to provide multichannel digitized sound playback for
   Sound Blaster compatible sound cards.

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include "compat.h"
#include "pragmas.h"
#include "linklist.h"
#include "drivers.h"
#include "pitch.h"
#include "multivoc.h"
#include "_multivc.h"
#include "fx_man.h"

static void MV_StopVoice(VoiceNode *voice);
static void MV_ServiceVoc(void);

static VoiceNode *MV_GetVoice(int32_t handle);

static const int16_t *MV_GetVolumeTable(int32_t vol);

#define IS_QUIET(ptr) ((void const *)(ptr) == (void *)&MV_VolumeTable[0])

static int32_t MV_ReverbLevel;
static int32_t MV_ReverbDelay;
static int16_t *MV_ReverbTable = NULL;

static int16_t MV_VolumeTable[MV_MAXVOLUME + 1][256];
Pan MV_PanTable[MV_NUMPANPOSITIONS][MV_MAXVOLUME + 1];

int32_t MV_Installed = FALSE;
static int32_t MV_TotalVolume = MV_MAXTOTALVOLUME;
static int32_t MV_MaxVoices = 1;

static int32_t MV_BufferSize = MV_MIXBUFFERSIZE;
static int32_t MV_BufferLength;

static int32_t MV_NumberOfBuffers = MV_NUMBEROFBUFFERS;

static int32_t MV_Channels = 1;

static int32_t MV_ReverseStereo = FALSE;

int32_t MV_MixRate;

static int32_t MV_BufferEmpty[MV_NUMBEROFBUFFERS];
char *MV_MixBuffer[MV_NUMBEROFBUFFERS + 1];

static VoiceNode *MV_Voices = NULL;

static VoiceNode VoiceList;
static VoiceNode VoicePool;

static int32_t MV_MixPage = 0;

void (*MV_Printf)(const char *fmt, ...) = NULL;
static void (*MV_CallBackFunc)(uint32_t) = NULL;

char *MV_MixDestination;
const int16_t *MV_LeftVolume;
const int16_t *MV_RightVolume;
int32_t MV_SampleSize = 1;
int32_t MV_RightChannelOffset;

uint32_t MV_MixPosition;

int32_t MV_ErrorCode = MV_NotInstalled;

static int32_t lockdepth = 0;

static FORCE_INLINE void DisableInterrupts(void)
{
    if (lockdepth++ <= 0)
        SoundDriver_Lock();
}

static FORCE_INLINE void RestoreInterrupts(void)
{
    if (--lockdepth <= 0)
        SoundDriver_Unlock();
}

const char *MV_ErrorString(int32_t ErrorNumber)
{
    switch (ErrorNumber)
    {
        case MV_Error:
            return MV_ErrorString(MV_ErrorCode);
        case MV_Ok:
            return "Multivoc ok.";
        case MV_NotInstalled:
            return "Multivoc not installed.";
        case MV_DriverError:
            return SoundDriver_ErrorString(SoundDriver_GetError());
        case MV_NoVoices:
            return "No free voices available to Multivoc.";
        case MV_NoMem:
            return "Out of memory in Multivoc.";
        case MV_VoiceNotFound:
            return "No voice with matching handle found.";
        case MV_InvalidFile:
            return "Invalid file passed in to Multivoc.";
        default:
            return "Unknown Multivoc error code.";
    }
}

static void MV_Mix(VoiceNode *voice, int const buffer)
{
    /* cheap fix for a crash under 64-bit linux */
    /*                            v  v  v  v    */
    if (voice->length == 0 && (voice->GetSound == NULL || voice->GetSound(voice) != KeepPlaying))
        return;

    int32_t length = MV_MIXBUFFERSIZE;
    uint32_t FixedPointBufferSize = voice->FixedPointBufferSize;

    MV_MixDestination = MV_MixBuffer[buffer];
    MV_LeftVolume = voice->LeftVolume;
    MV_RightVolume = voice->RightVolume;

    if ((MV_Channels == 2) && (IS_QUIET(MV_LeftVolume)))
    {
        MV_LeftVolume = MV_RightVolume;
        MV_MixDestination += MV_RightChannelOffset;
    }

    // Add this voice to the mix
    do
    {
        const char *start = voice->sound;
        uint32_t const rate = voice->RateScale;
        uint32_t const position = voice->position;
        int32_t voclength;

        // Check if the last sample in this buffer would be
        // beyond the length of the sample block
        if ((position + FixedPointBufferSize) >= voice->length)
        {
            if (position >= voice->length)
            {
                voice->GetSound(voice);
                return;
            }

            voclength = (voice->length - position + rate - voice->channels) / rate;
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
    } while (length > 0);
}

void MV_PlayVoice(VoiceNode *voice)
{
    DisableInterrupts();
    LL_SortedInsertion(&VoiceList, voice, prev, next, VoiceNode, priority);
    RestoreInterrupts();
}

static void MV_StopVoice(VoiceNode *voice)
{
    DisableInterrupts();

    // move the voice from the play list to the free list
    LL_Remove(voice, next, prev);
    LL_Add((VoiceNode*) &VoicePool, voice, next, prev);

    RestoreInterrupts();

    switch (voice->wavetype)
    {
#ifdef HAVE_VORBIS
        case FMT_VORBIS: MV_ReleaseVorbisVoice(voice); break;
#endif
#ifdef HAVE_FLAC
        case FMT_FLAC: MV_ReleaseFLACVoice(voice); break;
#endif
        case FMT_XA: MV_ReleaseXAVoice(voice); break;
#ifdef HAVE_XMP
        case FMT_XMP: MV_ReleaseXMPVoice(voice); break;
#endif
        default: break;
    }

    voice->handle = 0;
}

/*---------------------------------------------------------------------
   JBF: no synchronisation happens inside MV_ServiceVoc nor the
        supporting functions it calls. This would cause a deadlock
        between the mixer thread in the driver vs the nested
        locking in the user-space functions of MultiVoc. The call
        to MV_ServiceVoc is synchronised in the driver.
---------------------------------------------------------------------*/
static void MV_ServiceVoc(void)
{
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
            Bmemset(MV_MixBuffer[MV_MixPage], 0, MV_BufferSize);
            MV_BufferEmpty[ MV_MixPage ] = TRUE;
        }
    }
    else
    {
        char const *const end = MV_MixBuffer[0] + MV_BufferLength;
        char *dest = MV_MixBuffer[MV_MixPage];
        char const *source = MV_MixBuffer[MV_MixPage] - MV_ReverbDelay;

        if (source < MV_MixBuffer[ 0 ])
            source += MV_BufferLength;

        int32_t length = MV_BufferSize;

        while (length > 0)
        {
            int const count = (source + length > end) ? (end - source) : length;

            MV_16BitReverb(source, dest, MV_ReverbTable, count / 2);

            // if we go through the loop again, it means that we've wrapped around the buffer
            source  = MV_MixBuffer[ 0 ];
            dest   += count;
            length -= count;
        }
    }

    // Play any waiting voices
    //DisableInterrupts();


    if (!VoiceList.next || VoiceList.next == &VoiceList)
        return;

    VoiceNode *voice = VoiceList.next;

    int iter = 0;

    VoiceNode *next;

    do
    {
        next = voice->next;

        if (++iter > MV_MaxVoices && MV_Printf)
            MV_Printf("more iterations than voices! iter: %d\n",iter);

        if (voice->Paused)
            continue;

        MV_BufferEmpty[ MV_MixPage ] = FALSE;

        MV_Mix(voice, MV_MixPage);

        // Is this voice done?
        if (!voice->Playing)
        {
            //JBF: prevent a deadlock caused by MV_StopVoice grabbing the mutex again
            //MV_StopVoice( voice );
            LL_Remove(voice, next, prev);
            LL_Add((VoiceNode*) &VoicePool, voice, next, prev);

            switch (voice->wavetype)
            {
#ifdef HAVE_VORBIS
                case FMT_VORBIS: MV_ReleaseVorbisVoice(voice); break;
#endif
#ifdef HAVE_FLAC
                case FMT_FLAC: MV_ReleaseFLACVoice(voice); break;
#endif
                case FMT_XA: MV_ReleaseXAVoice(voice); break;
#ifdef HAVE_XMP
                case FMT_XMP: MV_ReleaseXMPVoice(voice); break;
#endif
                default: break;
            }

            voice->handle = 0;

            if (MV_CallBackFunc)
                MV_CallBackFunc(voice->callbackval);
        }
    }
    while ((voice = next) != &VoiceList);

    //RestoreInterrupts();
}

static VoiceNode *MV_GetVoice(int32_t handle)
{
    if (handle < MV_MINVOICEHANDLE || handle > MV_MaxVoices)
    {
        if (MV_Printf)
            MV_Printf("MV_GetVoice(): bad handle (%d)!\n", handle);
        return NULL;
    }

    DisableInterrupts();

    for (VoiceNode *voice = VoiceList.next; voice != &VoiceList; voice = voice->next)
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

VoiceNode *MV_BeginService(int32_t handle)
{
    if (!MV_Installed)
        return NULL;

    DisableInterrupts();

    VoiceNode *voice;

    if ((voice = MV_GetVoice(handle)) == NULL)
    {
        RestoreInterrupts();
        MV_SetErrorCode(MV_VoiceNotFound);
        return NULL;
    }

    return voice;
}

static inline void MV_EndService(void) { RestoreInterrupts(); }

int32_t MV_VoicePlaying(int32_t handle)
{
    return (MV_Installed && MV_GetVoice(handle)) ? TRUE : FALSE;
}

int32_t MV_KillAllVoices(void)
{
    if (!MV_Installed)
        return MV_Error;

    DisableInterrupts();

    if (&VoiceList == VoiceList.next)
    {
        RestoreInterrupts();
        return MV_Ok;
    }

    VoiceNode * voice = VoiceList.prev;

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

int32_t MV_Kill(int32_t handle)
{
    VoiceNode *voice = MV_BeginService(handle);

    if (voice == NULL)
        return MV_Error;

    uint32_t const callbackval = voice->callbackval;

    MV_StopVoice(voice);

    MV_EndService();

    if (MV_CallBackFunc)
        MV_CallBackFunc(callbackval);

    return MV_Ok;
}

int32_t MV_VoicesPlaying(void)
{
    if (!MV_Installed)
        return 0;

    DisableInterrupts();

    int NumVoices = 0;

    for (VoiceNode *voice = VoiceList.next; voice != &VoiceList; voice = voice->next)
        NumVoices++;

    RestoreInterrupts();

    return NumVoices;
}

VoiceNode *MV_AllocVoice(int32_t priority)
{
    VoiceNode   *voice, *node;

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

    int32_t vhan = MV_MINVOICEHANDLE;

    // Find a free voice handle
    do
    {
        if (++vhan < MV_MINVOICEHANDLE || vhan > MV_MaxVoices)
            vhan = MV_MINVOICEHANDLE;
    } while (MV_VoicePlaying(vhan));

    voice->handle = vhan;

    return voice;
}

int32_t MV_VoiceAvailable(int32_t priority)
{
    // Check if we have any free voices
    if (!LL_Empty(&VoicePool, next, prev))
        return TRUE;

    DisableInterrupts();

    VoiceNode   *voice, *node;

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

void MV_SetVoicePitch(VoiceNode *voice, uint32_t rate, int32_t pitchoffset)
{
    voice->SamplingRate = rate;
    voice->PitchScale   = PITCH_GetScale(pitchoffset);
    voice->RateScale    = (rate * voice->PitchScale) / MV_MixRate;

    // Multiply by MV_MIXBUFFERSIZE - 1
    voice->FixedPointBufferSize = (voice->RateScale * MV_MIXBUFFERSIZE) -
                                  voice->RateScale;
}

int32_t MV_SetPitch(int32_t handle, int32_t pitchoffset)
{
    VoiceNode *voice = MV_BeginService(handle);

    if (voice == NULL)
        return MV_Error;

    MV_SetVoicePitch(voice, voice->SamplingRate, pitchoffset);
    MV_EndService();

    return MV_Ok;
}

int32_t MV_SetFrequency(int32_t handle, int32_t frequency)
{
    VoiceNode *voice = MV_BeginService(handle);

    if (voice == NULL)
        return MV_Error;

    MV_SetVoicePitch(voice, frequency, 0);
    MV_EndService();

    return MV_Ok;
}

static inline const int16_t *MV_GetVolumeTable(int32_t vol) { return MV_VolumeTable[MIX_VOLUME(vol)]; }

/*---------------------------------------------------------------------
   Function: MV_SetVoiceMixMode

   Selects which method should be used to mix the voice.

   16Bit        16Bit |  8Bit  16Bit  8Bit  16Bit |
   Mono         Ster  |  Mono  Mono   Ster  Ster  |  Mixer
   Out          Out   |  In    In     In    In    |
----------------------+---------------------------+-------------
    X                 |         X                 | Mix16BitMono16
    X                 |   X                       | Mix16BitMono
                 X    |         X                 | Mix16BitStereo16
                 X    |   X                       | Mix16BitStereo
----------------------+---------------------------+-------------
                 X    |                      X    | Mix16BitStereo16Stereo
                 X    |                X          | Mix16BitStereo8Stereo
    X                 |                      X    | Mix16BitMono16Stereo
    X                 |                X          | Mix16BitMono8Stereo
---------------------------------------------------------------------*/

void MV_SetVoiceMixMode(VoiceNode *voice)
{
    int32_t type = T_DEFAULT;

    if (MV_Channels == 1)
        type |= T_MONO;
    else
    {
        if (IS_QUIET(voice->RightVolume))
            type |= T_RIGHTQUIET;
        else if (IS_QUIET(voice->LeftVolume))
            type |= T_LEFTQUIET;
    }

    if (voice->bits == 16)
        type |= T_16BITSOURCE;

    if (voice->channels == 2)
    {
        type |= T_STEREOSOURCE;
        type &= ~(T_RIGHTQUIET | T_LEFTQUIET);
    }

    switch (type)
    {
        case T_16BITSOURCE | T_LEFTQUIET:
            MV_LeftVolume = MV_RightVolume;
            fallthrough__;
        case T_16BITSOURCE | T_MONO:
        case T_16BITSOURCE | T_RIGHTQUIET: voice->mix = MV_Mix16BitMono16; break;

        case T_LEFTQUIET:
            MV_LeftVolume = MV_RightVolume;
            fallthrough__;
        case T_MONO:
        case T_RIGHTQUIET: voice->mix = MV_Mix16BitMono; break;

        case T_16BITSOURCE: voice->mix = MV_Mix16BitStereo16; break;

        case T_SIXTEENBIT_STEREO: voice->mix = MV_Mix16BitStereo; break;

        case T_16BITSOURCE | T_STEREOSOURCE: voice->mix = MV_Mix16BitStereo16Stereo; break;

        case T_16BITSOURCE | T_STEREOSOURCE | T_MONO: voice->mix = MV_Mix16BitMono16Stereo; break;

        case T_STEREOSOURCE: voice->mix = MV_Mix16BitStereo8Stereo; break;

        case T_STEREOSOURCE | T_MONO: voice->mix = MV_Mix16BitMono8Stereo; break;

        default: voice->mix = NULL; break;
    }
}

void MV_SetVoiceVolume(VoiceNode *voice, int32_t vol, int32_t left, int32_t right)
{
    if (MV_Channels == 1)
        left = right = vol;

    voice->LeftVolume = MV_GetVolumeTable(left);

    if (left == right)
        voice->RightVolume = voice->LeftVolume;
    else
    {
        voice->RightVolume = MV_GetVolumeTable(right);

        if (MV_ReverseStereo)
            swapptr(&voice->LeftVolume, &voice->RightVolume);
    }

    MV_SetVoiceMixMode(voice);
}

int32_t MV_PauseVoice(int32_t handle, int32_t pause)
{
    VoiceNode *voice = MV_BeginService(handle);

    if (voice == NULL)
        return MV_Error;

    voice->Paused = pause;
    MV_EndService();

    return MV_Ok;
}

int32_t MV_GetPosition(int32_t handle, int32_t *position)
{
    VoiceNode *voice = MV_BeginService(handle);

    if (voice == NULL)
        return MV_Error;

    switch (voice->wavetype)
    {
#ifdef HAVE_VORBIS
        case FMT_VORBIS: *position = MV_GetVorbisPosition(voice); break;
#endif
#ifdef HAVE_FLAC
        case FMT_FLAC: *position = MV_GetFLACPosition(voice); break;
#endif
        case FMT_XA: *position = MV_GetXAPosition(voice); break;
#ifdef HAVE_XMP
        case FMT_XMP: *position = MV_GetXMPPosition(voice); break;
#endif
        default: break;
    }

    MV_EndService();

    return MV_Ok;
}

int32_t MV_SetPosition(int32_t handle, int32_t position)
{
    VoiceNode *voice = MV_BeginService(handle);

    if (voice == NULL)
        return MV_Error;

    switch (voice->wavetype)
    {
#ifdef HAVE_VORBIS
        case FMT_VORBIS: MV_SetVorbisPosition(voice, position); break;
#endif
#ifdef HAVE_FLAC
        case FMT_FLAC: MV_SetFLACPosition(voice, position); break;
#endif
        case FMT_XA: MV_SetXAPosition(voice, position); break;
#ifdef HAVE_XMP
        case FMT_XMP: MV_SetXMPPosition(voice, position); break;
#endif
        default: break;
    }

    MV_EndService();

    return MV_Ok;
}

int32_t MV_EndLooping(int32_t handle)
{
    VoiceNode *voice = MV_BeginService(handle);

    if (voice == NULL)
        return MV_Error;

    voice->LoopCount = 0;
    voice->LoopStart = NULL;
    voice->LoopEnd = NULL;

    MV_EndService();

    return MV_Ok;
}

int32_t MV_SetPan(int32_t handle, int32_t vol, int32_t left, int32_t right)
{
    VoiceNode *voice = MV_BeginService(handle);

    if (voice == NULL)
        return MV_Error;

    MV_SetVoiceVolume(voice, vol, left, right);
    MV_EndService();
    return MV_Ok;
}

int32_t MV_Pan3D(int32_t handle, int32_t angle, int32_t distance)
{
    if (distance < 0)
    {
        distance = -distance;
        angle += MV_NUMPANPOSITIONS / 2;
    }

    int const volume = MIX_VOLUME(distance);

    angle &= MV_MAXPANPOSITION;

    return MV_SetPan(handle, max(0, 255 - distance),
        MV_PanTable[ angle ][ volume ].left,
        MV_PanTable[ angle ][ volume ].right);
}

void MV_SetReverb(int32_t reverb)
{
    MV_ReverbLevel = MIX_VOLUME(reverb);
    MV_ReverbTable = &MV_VolumeTable[MV_ReverbLevel][0];
}

int32_t MV_GetMaxReverbDelay(void) { return MV_MIXBUFFERSIZE * MV_NumberOfBuffers; }

int32_t MV_GetReverbDelay(void) { return MV_ReverbDelay / MV_SampleSize; }

void MV_SetReverbDelay(int32_t delay)
{
    MV_ReverbDelay = max(MV_MIXBUFFERSIZE, min(delay, MV_GetMaxReverbDelay())) * MV_SampleSize;
}

static int32_t MV_SetMixMode(int32_t numchannels)
{
    if (!MV_Installed)
        return MV_Error;

    MV_Channels = 1 + (numchannels == 2);
    MV_SampleSize = sizeof(int8_t) * MV_Channels * 2;

    MV_BufferSize = MV_MIXBUFFERSIZE * MV_SampleSize;
    MV_NumberOfBuffers = MV_TOTALBUFFERSIZE / MV_BufferSize;
    MV_BufferLength = MV_TOTALBUFFERSIZE;

    MV_RightChannelOffset = MV_SampleSize >> 1;

    return MV_Ok;
}

static int32_t MV_StartPlayback(void)
{
    // Initialize the buffers
    Bmemset(MV_MixBuffer[0], 0, MV_TOTALBUFFERSIZE);

    for (int buffer = 0; buffer < MV_NumberOfBuffers; buffer++)
        MV_BufferEmpty[buffer] = TRUE;

    MV_MixPage = 1;

    if (SoundDriver_BeginPlayback(MV_MixBuffer[0], MV_BufferSize, MV_NumberOfBuffers, MV_ServiceVoc) != MV_Ok)
    {
        MV_SetErrorCode(MV_DriverError);
        return MV_Error;
    }

    return MV_Ok;
}

static void MV_StopPlayback(void)
{
    SoundDriver_StopPlayback();

    // Make sure all callbacks are done.
    DisableInterrupts();

    for (VoiceNode *voice = VoiceList.next, *next; voice != &VoiceList; voice = next)
    {
        next = voice->next;

        MV_StopVoice(voice);

        if (MV_CallBackFunc)
            MV_CallBackFunc(voice->callbackval);
    }

    RestoreInterrupts();
}

static void MV_CalcVolume(int32_t MaxVolume)
{
    // For each volume level, create a translation table with the
    // appropriate volume calculated.

    for (int volume = 0; volume <= MV_MAXVOLUME; volume++)
    {
        int const level = (volume * MaxVolume) / MV_MAXTOTALVOLUME;

        for (int i = 0; i < 65536; i += 256)
            MV_VolumeTable[volume][i / 256] = ((i - 0x8000) * level) / MV_MAXVOLUME;
    }
}

static void MV_CalcPanTable(void)
{
    const int32_t HalfAngle = MV_NUMPANPOSITIONS / 2;
    const int32_t QuarterAngle = HalfAngle / 2;

    for (int distance = 0; distance <= MV_MAXVOLUME; distance++)
    {
        const int32_t level = (255 * (MV_MAXVOLUME - distance)) / MV_MAXVOLUME;

        for (int angle = 0; angle <= QuarterAngle; angle++)
        {
            const int32_t ramp = level - (level * angle) / QuarterAngle;

            MV_PanTable[angle][distance].left = ramp;
            MV_PanTable[angle][distance].right = level;

            MV_PanTable[HalfAngle - angle][distance].left = ramp;
            MV_PanTable[HalfAngle - angle][distance].right = level;

            MV_PanTable[HalfAngle + angle][distance].left = level;
            MV_PanTable[HalfAngle + angle][distance].right = ramp;

            MV_PanTable[MV_MAXPANPOSITION - angle][distance].left = level;
            MV_PanTable[MV_MAXPANPOSITION - angle][distance].right = ramp;
        }
    }
}

void MV_SetVolume(int32_t volume)
{
    MV_TotalVolume = min(max(0, volume), MV_MAXTOTALVOLUME);
    MV_CalcVolume(MV_TotalVolume);
}

int32_t MV_GetVolume(void) { return MV_TotalVolume; }

void MV_SetCallBack(void (*function)(uint32_t)) { MV_CallBackFunc = function; }

void MV_SetReverseStereo(int32_t setting) { MV_ReverseStereo = setting; }

int32_t MV_GetReverseStereo(void) { return MV_ReverseStereo; }

int32_t MV_Init(int32_t soundcard, int32_t MixRate, int32_t Voices, int32_t numchannels, void *initdata)
{
    if (MV_Installed)
        MV_Shutdown();

    MV_SetErrorCode(MV_Ok);

    // MV_TotalMemory + 2: FIXME, see valgrind_errors.log
    int const totalmem = Voices * sizeof(VoiceNode) + MV_TOTALBUFFERSIZE + 2;

    char *ptr = (char *) Xaligned_alloc(16, totalmem);

    if (!ptr)
    {
        MV_SetErrorCode(MV_NoMem);
        return MV_Error;
    }

    Bmemset(ptr, 0, totalmem);

    MV_Voices = (VoiceNode *)ptr;
    ptr += Voices * sizeof(VoiceNode);

    MV_MaxVoices = Voices;

    LL_Reset((VoiceNode*) &VoiceList, next, prev);
    LL_Reset((VoiceNode*) &VoicePool, next, prev);

    for (int index = 0; index < Voices; index++)
        LL_Add((VoiceNode*) &VoicePool, &MV_Voices[ index ], next, prev);

    MV_SetReverseStereo(FALSE);

    ASS_SoundDriver = soundcard;

    // Initialize the sound card

    if (SoundDriver_Init(&MixRate, &numchannels, initdata) != MV_Ok)
        MV_SetErrorCode(MV_DriverError);

    if (MV_ErrorCode != MV_Ok)
    {
        ALIGNED_FREE_AND_NULL(MV_Voices);

        return MV_Error;
    }

    MV_Installed    = TRUE;
    MV_CallBackFunc = NULL;
    MV_ReverbLevel  = 0;
    MV_ReverbTable  = NULL;

    // Set the sampling rate
    MV_MixRate = MixRate;

    // Set Mixer to play stereo digitized sound
    MV_SetMixMode(numchannels);
    MV_ReverbDelay = MV_BufferSize * 3;

    // Make sure we don't cross a physical page
    MV_MixBuffer[ MV_NumberOfBuffers ] = ptr;
    for (int buffer = 0; buffer < MV_NumberOfBuffers; buffer++)
    {
        MV_MixBuffer[ buffer ] = ptr;
        ptr += MV_BufferSize;
    }

    // Calculate pan table
    MV_CalcPanTable();

    MV_SetVolume(MV_MAXTOTALVOLUME);

    // Start the playback engine
    if (MV_StartPlayback() != MV_Ok)
    {
        // Preserve error code while we shutdown.
        int status = MV_ErrorCode;
        MV_Shutdown();
        MV_SetErrorCode(status);
        return MV_Error;
    }

    return MV_Ok;
}

int32_t MV_Shutdown(void)
{
    if (!MV_Installed)
        return MV_Ok;

    MV_KillAllVoices();

    MV_Installed = FALSE;

    // Stop the sound playback engine
    MV_StopPlayback();

    // Shutdown the sound card
    SoundDriver_Shutdown();

    // Free any voices we allocated
    ALIGNED_FREE_AND_NULL(MV_Voices);

    LL_Reset((VoiceNode*) &VoiceList, next, prev);
    LL_Reset((VoiceNode*) &VoicePool, next, prev);

    MV_MaxVoices = 1;

    // Release the descriptor from our mix buffer
    for (int buffer = 0; buffer < MV_NUMBEROFBUFFERS; buffer++)
        MV_MixBuffer[ buffer ] = NULL;

    MV_SetErrorCode(MV_NotInstalled);

    return MV_Ok;
}

void MV_SetPrintf(void (*function)(const char *, ...)) { MV_Printf = function; }

const char *loopStartTags[loopStartTagCount] = { "LOOP_START", "LOOPSTART", "LOOP" };
const char *loopEndTags[loopEndTagCount] = { "LOOP_END", "LOOPEND" };
const char *loopLengthTags[loopLengthTagCount] = { "LOOP_LENGTH", "LOOPLENGTH" };
