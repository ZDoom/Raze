/*
 Copyright (C) 2009 Jonathon Fowler <jf@jonof.id.au>

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

/**
 * libSDL output driver for MultiVoc
 */

#include "driver_sdl.h"

#include "compat.h"
#include "multivoc.h"
#include "mutex.h"
#include "sdl_inc.h"
#include "vfs.h"

enum {
   SDLErr_Warning = -2,
   SDLErr_Error   = -1,
   SDLErr_Ok      = 0,
   SDLErr_Uninitialised,
   SDLErr_InitSubSystem,
   SDLErr_OpenAudio
};

static int32_t ErrorCode = SDLErr_Ok;
static int32_t Initialised;
static int32_t Playing;
static uint32_t StartedSDL;

static char *MixBuffer;
static int32_t MixBufferSize;
static int32_t MixBufferCount;
static int32_t MixBufferCurrent;
static int32_t MixBufferUsed;
static void (*MixCallBack)(void);

#if (SDL_MAJOR_VERSION == 2)
static SDL_AudioDeviceID audio_dev;
#endif

static void fillData(void * userdata, Uint8 * ptr, int remaining)
{
    UNREFERENCED_PARAMETER(userdata);

    int len;
    char *sptr;

    while (remaining > 0) {
        if (MixBufferUsed == MixBufferSize) {
            MixCallBack();

            MixBufferUsed = 0;
            MixBufferCurrent++;
            if (MixBufferCurrent >= MixBufferCount) {
                MixBufferCurrent -= MixBufferCount;
            }
        }

        while (remaining > 0 && MixBufferUsed < MixBufferSize) {
            sptr = MixBuffer + (MixBufferCurrent * MixBufferSize) + MixBufferUsed;

            len = MixBufferSize - MixBufferUsed;
            if (remaining < len) {
                len = remaining;
            }

            memcpy(ptr, sptr, len);

            ptr += len;
            MixBufferUsed += len;
            remaining -= len;
        }
    }
}

int32_t SDLDrv_GetError(void)
{
    return ErrorCode;
}

const char *SDLDrv_ErrorString( int32_t ErrorNumber )
{
    const char *ErrorString;

    switch( ErrorNumber ) {
        case SDLErr_Warning :
        case SDLErr_Error :
            ErrorString = SDLDrv_ErrorString( ErrorCode );
            break;

        case SDLErr_Ok :
            ErrorString = "SDL Audio ok.";
            break;

        case SDLErr_Uninitialised:
            ErrorString = "SDL Audio uninitialised.";
            break;

        case SDLErr_InitSubSystem:
            ErrorString = "SDL Audio: error in Init or InitSubSystem.";
            break;

        case SDLErr_OpenAudio:
            ErrorString = "SDL Audio: error in OpenAudio.";
            break;

        default:
            ErrorString = "Unknown SDL Audio error code.";
            break;
    }

    return ErrorString;
}

int32_t SDLDrv_PCM_Init(int32_t *mixrate, int32_t *numchannels, void * initdata)
{
    UNREFERENCED_PARAMETER(initdata);

    Uint32 inited;
    int err = 0;
    SDL_AudioSpec spec, actual;

    if (Initialised) {
        SDLDrv_PCM_Shutdown();
    }

    inited = SDL_WasInit(SDL_INIT_AUDIO);

    if (!(inited & SDL_INIT_AUDIO)) {
        err = SDL_InitSubSystem(SDL_INIT_AUDIO);
        StartedSDL = SDL_WasInit(SDL_INIT_AUDIO);
    }

    if (err < 0) {
        ErrorCode = SDLErr_InitSubSystem;
        return SDLErr_Error;
    }

    int chunksize = 512;
#ifdef __ANDROID__
    chunksize = droidinfo.audio_buffer_size;
#endif

    spec.freq = *mixrate;
    spec.format = AUDIO_S16SYS;
    spec.channels = *numchannels;
    spec.samples = chunksize;
    spec.callback = fillData;
    spec.userdata = nullptr;

    Bmemset(&actual, 0, sizeof(actual));

#if (SDL_MAJOR_VERSION == 1)
    err = !SDL_OpenAudio(&spec, &actual);
#else
    audio_dev = err = SDL_OpenAudioDevice(NULL, 0, &spec, &actual, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
#endif

    if (err == 0) {
        ErrorCode = SDLErr_OpenAudio;
        return SDLErr_Error;
    }

#if (SDL_MAJOR_VERSION == 1)
    char drivername[64] = "(error)";
    SDL_AudioDriverName(drivername, sizeof(drivername));
    MV_Printf("SDL %s driver\n", drivername);
#else
    char *drivername = Xstrdup(SDL_GetCurrentAudioDriver());

    for (int i=0;drivername[i] != 0;++i)
        drivername[i] = toupperlookup[drivername[i]];

    MV_Printf("SDL %s driver on %s\n", drivername ? drivername : "(error)", SDL_GetAudioDeviceName(0, 0));
    Xfree(drivername);
#endif

#if (SDL_MAJOR_VERSION == 1)
    if (actual.freq == 0 || actual.channels == 0) {
        // hack for when SDL said it opened the audio, but clearly didn't
        SDL_CloseAudio();
            ErrorCode = SDLErr_OpenAudio;
            return SDLErr_Error;
        }
#endif
    err = 0;

    *mixrate = actual.freq;
    if (actual.format == AUDIO_U8 || actual.format == AUDIO_S8)
    {
        ErrorCode = SDLErr_OpenAudio;
        err = 1;
    }

    *numchannels = actual.channels;
    if (actual.channels != 1 && actual.channels != 2)
    {
        ErrorCode = SDLErr_OpenAudio;
        err = 1;
    }

    if (err)
    {
        SDL_CloseAudio();
        return SDLErr_Error;
    }

    Initialised = 1;
    return SDLErr_Ok;
}

void SDLDrv_PCM_Shutdown(void)
{
    if (!Initialised)
        return;

    if (StartedSDL)
        SDL_QuitSubSystem(StartedSDL);

    StartedSDL = 0;
    Initialised = 0;
}

int32_t SDLDrv_PCM_BeginPlayback(char *BufferStart, int32_t BufferSize,
                        int32_t NumDivisions, void ( *CallBackFunc )( void ) )
{
    if (!Initialised) {
        ErrorCode = SDLErr_Uninitialised;
        return SDLErr_Error;
    }

    if (Playing) {
        SDLDrv_PCM_StopPlayback();
    }

    MixBuffer = BufferStart;
    MixBufferSize = BufferSize;
    MixBufferCount = NumDivisions;
    MixBufferCurrent = 0;
    MixBufferUsed = 0;
    MixCallBack = CallBackFunc;

    // prime the buffer
    MixCallBack();

#if (SDL_MAJOR_VERSION == 2)
    SDL_PauseAudioDevice(audio_dev, 0);
#else
    SDL_PauseAudio(0);
#endif
    Playing = 1;

    return SDLErr_Ok;
}

void SDLDrv_PCM_StopPlayback(void)
{
    if (!Initialised || !Playing) {
        return;
    }

#if (SDL_MAJOR_VERSION == 2)
    SDL_PauseAudioDevice(audio_dev, 1);
#else
    SDL_PauseAudio(1);
#endif

    Playing = 0;
}

void SDLDrv_PCM_Lock(void)
{
#if (SDL_MAJOR_VERSION == 2)
    SDL_LockAudioDevice(audio_dev);
#else
    SDL_LockAudio();
#endif
}

void SDLDrv_PCM_Unlock(void)
{
#if (SDL_MAJOR_VERSION == 2)
    SDL_UnlockAudioDevice(audio_dev);
#else
    SDL_UnlockAudio();
#endif
}
