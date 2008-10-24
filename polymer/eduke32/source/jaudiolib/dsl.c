/*
Copyright (C) 2003-2004 Ryan C. Gordon. and James Bentler

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

Originally written by Ryan C. Gordon. (icculus@clutteredmind.org)
Adapted to work with JonoF's port by James Bentler (bentler@cs.umn.edu)

*/
#include <stdlib.h>
#include <string.h>

#include "dsl.h"
#include "compat.h"
#include "pragmas.h"

#define _NEED_SDLMIXER	1
#include "sdl_inc.h"

extern int MV_MixPage;

int DSL_ErrorCode = DSL_Ok;

static int mixer_initialized;
static int interrupts_disabled = 0;

static int(*_DSL_CallBackFunc)(int);
static volatile char *_DSL_BufferStart;
static int _DSL_BufferSize;
static int _DSL_SampleRate;
static int _DSL_remainder;
static Uint16 _DSL_format;
static int _DSL_channels;


static Mix_Chunk *blank;
static unsigned char *blank_buf;

/*
possible todo ideas: cache sdl/sdl mixer error messages.
*/

char *DSL_ErrorString(int ErrorNumber)
{
    char *ErrorString;

    switch (ErrorNumber)
    {
    case DSL_Warning:
    case DSL_Error:
        ErrorString = DSL_ErrorString(DSL_ErrorCode);
        break;

    case DSL_Ok:
        ErrorString = "SDL Driver ok.";
        break;

    case DSL_SDLInitFailure:
        ErrorString = "SDL Audio initialization failed.";
        break;

    case DSL_MixerActive:
        ErrorString = "SDL Mixer already initialized.";
        break;

    case DSL_MixerInitFailure:
        ErrorString = "SDL Mixer initialization failed.";
        break;

    default:
        ErrorString = "Unknown SDL Driver error.";
        break;
    }

    return ErrorString;
}

static void DSL_SetErrorCode(int ErrorCode)
{
    DSL_ErrorCode = ErrorCode;
}

int DSL_Init(int soundcard, int mixrate, int numchannels, int samplebits, int buffersize)
{
    /* FIXME: Do I need an SDL_mixer version check
     * like that in sdlmusic.h here, too???
     */
    DSL_SetErrorCode(DSL_Ok);

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    {
        DSL_SetErrorCode(DSL_SDLInitFailure);

        return DSL_Error;
    }

    _DSL_channels = numchannels;
    _DSL_SampleRate = mixrate;
    _DSL_format = (samplebits == 16) ? AUDIO_S16SYS : AUDIO_U8;

    return DSL_Ok;
}

void DSL_Shutdown(void)
{
    DSL_StopPlayback();
}

static void mixer_callback(int chan, void *stream, int len, void *udata)
{
    Uint8 *stptr;
    Uint8 *fxptr;
    int copysize;
    UNREFERENCED_PARAMETER(chan);
    UNREFERENCED_PARAMETER(udata);
    /* len should equal _DSL_BufferSize, else this is screwed up */

    stptr = (Uint8 *)stream;

    if (_DSL_remainder > 0)
    {
        copysize = min(len, _DSL_remainder);

        fxptr = (Uint8 *)(&_DSL_BufferStart[MV_MixPage *
                                        _DSL_BufferSize]);

        memcpy(stptr, fxptr+(_DSL_BufferSize-_DSL_remainder), copysize);

        len -= copysize;
        _DSL_remainder -= copysize;

        stptr += copysize;
    }

    while (len > 0)
    {
        /* new buffer */

        _DSL_CallBackFunc(0);

        fxptr = (Uint8 *)(&_DSL_BufferStart[MV_MixPage *
                                        _DSL_BufferSize]);

        copysize = min(len, _DSL_BufferSize);

        memcpy(stptr, fxptr, copysize);

        len -= copysize;

        stptr += copysize;
    }

    _DSL_remainder = len;
}

//int   DSL_BeginBufferedPlayback(char *BufferStart,
  //                              int BufferSize, int NumDivisions, unsigned SampleRate,
    //                            int MixMode, void(*CallBackFunc)(void))
int DSL_BeginBufferedPlayback(char *BufferStart, int(*CallBackFunc)(int), int BufferSize, int NumDivisions)
{
    int chunksize;

    if (mixer_initialized)
    {
        DSL_SetErrorCode(DSL_MixerActive);

        return DSL_Error;
    }

    _DSL_CallBackFunc = CallBackFunc;
    _DSL_BufferStart = BufferStart;
    _DSL_BufferSize = (BufferSize / NumDivisions);
    
    _DSL_remainder = 0;

    /*
       23ms is typically ideal (11025,22050,44100)
       46ms isn't bad
    */

    chunksize = 512;

    if (_DSL_SampleRate >= 16000) chunksize *= 2;
    if (_DSL_SampleRate >= 32000) chunksize *= 2;

    if (Mix_OpenAudio(_DSL_SampleRate, _DSL_format, _DSL_channels, chunksize) < 0)
    {
        DSL_SetErrorCode(DSL_MixerInitFailure);

        return DSL_Error;
    }

    /*
       Mix_SetPostMix(mixer_callback, NULL);
    */
    /* have to use a channel because postmix will overwrite the music... */
    Mix_RegisterEffect(0, mixer_callback, NULL, NULL);

    /* create a dummy sample just to allocate that channel */
    blank_buf = (Uint8 *)malloc(4096);
    memset(blank_buf, 0, 4096);

    blank = Mix_QuickLoad_RAW(blank_buf, 4096);

    Mix_PlayChannel(0, blank, -1);

    mixer_initialized = 1;

    return DSL_Ok;
}

void DSL_StopPlayback(void)
{
    if (mixer_initialized)
    {
        Mix_HaltChannel(0);
    }

    if (blank != NULL)
    {
        Mix_FreeChunk(blank);
    }

    blank = NULL;

    if (blank_buf  != NULL)
    {
        free(blank_buf);
    }

    blank_buf = NULL;

    if (mixer_initialized)
    {
        Mix_CloseAudio();
    }

    mixer_initialized = 0;
}

unsigned DSL_GetPlaybackRate(void)
{
    return _DSL_SampleRate;
}

int DisableInterrupts(void)
{
    if (interrupts_disabled++)
        return 0;
//    interrupts_disabled = 1;
    SDL_LockAudio();
    return(0);
}

int RestoreInterrupts(int flags)
{
    UNREFERENCED_PARAMETER(flags);
    if (--interrupts_disabled)
        return 0;
//   interrupts_disabled = 0;
    SDL_UnlockAudio();
    return(0);
}
