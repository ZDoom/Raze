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

static int32_t MV_ReverbLevel;
static int32_t MV_ReverbDelay;
static float MV_ReverbVolume;

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
static void (*MV_CallBackFunc)(intptr_t) = NULL;

char *MV_MixDestination;
int32_t MV_SampleSize = 1;
int32_t MV_RightChannelOffset;

int32_t MV_ErrorCode = MV_NotInstalled;

float MV_GlobalVolume = 1.f;
float MV_VolumeSmooth = 1.f;

static int lockdepth = 0;

static inline void MV_Lock(void)
{
    if (!lockdepth++)
        SoundDriver_Lock();
}

static inline void MV_Unlock(void)
{
    if (!--lockdepth)
        SoundDriver_Unlock();
    else if (lockdepth < 0 && MV_Printf)
        MV_Printf("RestoreInterrupts(): lockdepth < 0!\n");
}

static bool MV_Mix(VoiceNode * const voice, int const buffer)
{
    /* cheap fix for a crash under 64-bit linux */
    /*                            v  v  v  v    */
    if (voice->length == 0 && (voice->GetSound == NULL || voice->GetSound(voice) != KeepPlaying))
        return false;

    float const gv = MV_GlobalVolume;

    if (voice->priority == FX_MUSIC_PRIORITY)
        MV_GlobalVolume = 1.f;

    int32_t        length = MV_MIXBUFFERSIZE;
    uint32_t       bufsiz = voice->FixedPointBufferSize;
    uint32_t const rate   = voice->RateScale;

    MV_MixDestination = MV_MixBuffer[buffer];

    // Add this voice to the mix
    do
    {
        int32_t        mixlen   = length;
        uint32_t const position = voice->position;
        uint32_t const voclen   = voice->length;

        // Check if the last sample in this buffer would be
        // beyond the length of the sample block
        if ((position + bufsiz) >= voclen)
        {
            if (position >= voclen)
            {
                voice->GetSound(voice);
                break;
            }

            mixlen = (voclen - position + rate - voice->channels) / rate;
        }

        voice->position = voice->mix(voice, mixlen);
        length -= mixlen;

        if (voice->position >= voclen)
        {
            // Get the next block of sound
            if (voice->GetSound(voice) == NoMoreData)
            {
                MV_GlobalVolume = gv;
                return false;
            }

            // Get the position of the last sample in the buffer
            if (length > (voice->channels - 1))
                bufsiz = voice->RateScale * (length - voice->channels);
        }
    } while (length > 0);

    MV_GlobalVolume = gv;
    return true;
}

void MV_PlayVoice(VoiceNode *voice)
{
    MV_Lock();
    LL::SortedInsert(&VoiceList, voice, &VoiceNode::priority);
    voice->LeftVolume = voice->LeftVolumeDest;
    voice->RightVolume = voice->RightVolumeDest;
    MV_Unlock();
}

static void MV_CleanupVoice(VoiceNode *voice)
{
    if (MV_CallBackFunc)
        MV_CallBackFunc(voice->callbackval);

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

static void MV_StopVoice(VoiceNode *voice)
{
    MV_Lock();
    MV_CleanupVoice(voice);
    // move the voice from the play list to the free list
    LL::Move(voice, &VoicePool);
    MV_Unlock();
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
    ++MV_MixPage &= MV_NumberOfBuffers-1;

    if (MV_ReverbLevel == 0)
    {
        if (!MV_BufferEmpty[MV_MixPage])
        {
            Bmemset(MV_MixBuffer[MV_MixPage], 0, MV_BufferSize);
            MV_BufferEmpty[ MV_MixPage ] = TRUE;
        }
    }
    else
    {
        char const *const end    = MV_MixBuffer[0] + MV_BufferLength;
        char *            dest   = MV_MixBuffer[MV_MixPage];
        char const *      source = MV_MixBuffer[MV_MixPage] - MV_ReverbDelay;

        if (source < MV_MixBuffer[ 0 ])
            source += MV_BufferLength;

        int32_t length = MV_BufferSize;

        do
        {
            int const count = (source + length > end) ? (end - source) : length;

            MV_16BitReverb(source, dest, MV_ReverbVolume, count >> 1);

            // if we go through the loop again, it means that we've wrapped around the buffer
            source  = MV_MixBuffer[ 0 ];
            dest   += count;
            length -= count;
        } while (length > 0);
    }

    if (!VoiceList.next || VoiceList.next == &VoiceList)
        return;

    VoiceNode *voice = VoiceList.next;
    VoiceNode *next;

    do
    {
        next = voice->next;

        if (voice->Paused)
            continue;

        MV_BufferEmpty[ MV_MixPage ] = FALSE;

        // Is this voice done?
        if (!MV_Mix(voice, MV_MixPage))
        {
            MV_CleanupVoice(voice);
            LL::Move(voice, &VoicePool);
        }
    }
    while ((voice = next) != &VoiceList);
}

static VoiceNode *MV_GetVoice(int32_t handle)
{
    if (handle < MV_MINVOICEHANDLE || handle > MV_MaxVoices)
    {
        if (MV_Printf)
            MV_Printf("MV_GetVoice(): bad handle (%d)!\n", handle);
        return NULL;
    }

    MV_Lock();

    for (VoiceNode *voice = VoiceList.next; voice != &VoiceList; voice = voice->next)
    {
        if (handle == voice->handle)
        {
            MV_Unlock();
            return voice;
        }
    }

    MV_Unlock();
    MV_SetErrorCode(MV_VoiceNotFound);
    return NULL;
}

VoiceNode *MV_BeginService(int32_t handle)
{
    if (!MV_Installed)
        return NULL;

    VoiceNode *voice = MV_GetVoice(handle);

    if (voice == NULL)
    {
        MV_SetErrorCode(MV_VoiceNotFound);
        return NULL;
    }

    MV_Lock();

    return voice;
}

static inline void MV_EndService(void) { MV_Unlock(); }

int32_t MV_VoicePlaying(int32_t handle)
{
    return (MV_Installed && MV_GetVoice(handle)) ? TRUE : FALSE;
}

int32_t MV_KillAllVoices(void)
{
    if (!MV_Installed)
        return MV_Error;

    MV_Lock();

    if (&VoiceList == VoiceList.next)
    {
        MV_Unlock();
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

    MV_Unlock();

    return MV_Ok;
}

int32_t MV_Kill(int32_t handle)
{
    VoiceNode *voice = MV_BeginService(handle);

    if (voice == NULL)
        return MV_Error;

    MV_StopVoice(voice);
    MV_EndService();

    return MV_Ok;
}

int32_t MV_VoicesPlaying(void)
{
    if (!MV_Installed)
        return 0;

    MV_Lock();

    int NumVoices = 0;

    for (VoiceNode *voice = VoiceList.next; voice != &VoiceList; voice = voice->next)
        NumVoices++;

    MV_Unlock();

    return NumVoices;
}

VoiceNode *MV_AllocVoice(int32_t priority)
{
    VoiceNode   *voice, *node;

    MV_Lock();

    // Check if we have any free voices
    if (LL::Empty(&VoicePool))
    {
        // check if we have a higher priority than a voice that is playing.
        for (voice = node = VoiceList.next; node != &VoiceList; node = node->next)
        {
            if (node->priority < voice->priority)
                voice = node;
        }

        if (priority >= voice->priority && voice != &VoiceList && voice->handle >= MV_MINVOICEHANDLE)
            MV_Kill(voice->handle);

        if (LL::Empty(&VoicePool))
        {
            // No free voices
            MV_Unlock();
            return NULL;
        }
    }

    voice = VoicePool.next;
    LL::Remove(voice);
    MV_Unlock();

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
    if (!LL::Empty(&VoicePool))
        return TRUE;

    MV_Lock();

    VoiceNode   *voice, *node;

    // check if we have a higher priority than a voice that is playing.
    for (voice = node = VoiceList.next; node != &VoiceList; node = node->next)
    {
        if (node->priority < voice->priority)
            voice = node;
    }

    if ((voice == &VoiceList) || (priority < voice->priority))
    {
        MV_Unlock();
        return FALSE;
    }

    MV_Unlock();
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
    int type = T_DEFAULT;

    if (MV_Channels == 1)
        type |= T_MONO;

    if (voice->bits == 16)
        type |= T_16BITSOURCE;

    if (voice->channels == 2)
        type |= T_STEREOSOURCE;

    // stereo look-up table
    static constexpr decltype(voice->mix) mixslut[]
    = { MV_Mix16BitStereo,        MV_Mix16BitMono,        MV_Mix16BitStereo16,       MV_Mix16BitMono16,
        MV_Mix16BitStereo8Stereo, MV_Mix16BitMono8Stereo, MV_Mix16BitStereo16Stereo, MV_Mix16BitMono16Stereo };

    voice->mix = mixslut[type];
}

void MV_SetVoiceVolume(VoiceNode *voice, int32_t vol, int32_t left, int32_t right, float volume)
{
    if (MV_Channels == 1)
        left = right = vol;

    voice->LeftVolumeDest = float(left)*(1.f/MV_MAXTOTALVOLUME);
    voice->RightVolumeDest = float(right)*(1.f/MV_MAXTOTALVOLUME);

    if (MV_ReverseStereo)
        swapfloat(&voice->LeftVolumeDest, &voice->RightVolumeDest);

    voice->volume = volume;

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

    MV_SetVoiceVolume(voice, vol, left, right, voice->volume);
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
    MV_ReverbVolume = float(MV_ReverbLevel)*(1.f/MV_MAXVOLUME);
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
    Bassert(isPow2(MV_NumberOfBuffers));
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
    MV_Lock();

    for (VoiceNode *voice = VoiceList.next, *next; voice != &VoiceList; voice = next)
    {
        next = voice->next;
        MV_StopVoice(voice);
    }

    MV_Unlock();
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
    MV_TotalVolume  = min(max(0, volume), MV_MAXTOTALVOLUME);
    MV_GlobalVolume = (float)volume / 255.f;
    // MV_CalcVolume(MV_TotalVolume);
}

int32_t MV_GetVolume(void) { return MV_TotalVolume; }

void MV_SetCallBack(void (*function)(intptr_t)) { MV_CallBackFunc = function; }

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

    LL::Reset((VoiceNode*) &VoiceList);
    LL::Reset((VoiceNode*) &VoicePool);

    for (int index = 0; index < Voices; index++)
        LL::Insert(&VoicePool, &MV_Voices[index]);

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
    MV_ReverbVolume = 0.f;

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

    MV_VolumeSmooth = 1.f-powf(0.1f, 30.f/MixRate);

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

    LL::Reset((VoiceNode*) &VoiceList);
    LL::Reset((VoiceNode*) &VoicePool);

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

