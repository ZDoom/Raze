/*
 * DirectSound output code for MultiVoc
 * by Jonathon Fowler (jonof@edgenetwk.com)
 */
//-------------------------------------------------------------------------
/*
Duke Nukem Copyright (C) 1996, 2003 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
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
//-------------------------------------------------------------------------

#include "sdlout.h"
#include "sdl_inc.h"
#include <stdio.h>
#include <string.h>

#include "sdlayer.h"

#if defined(__WATCOMC__) || defined(_MSC_VER)
#include <malloc.h>
#endif


#ifndef RENDERTYPESDL
#error The SDL output module for AudioLib only works with the SDL interface.
#endif

static int32_t(*_SDLSOUND_CallBack)(int32_t) = NULL;
static int32_t _SDLSOUND_BufferLength = 0;
static int32_t _SDLSOUND_NumBuffers   = 0;
static char *_SDLSOUND_MixBuffer  = NULL;

static int32_t SDLSOUND_Installed = FALSE;

int32_t SDLSOUND_ErrorCode = SDLSOUND_Ok;

#define SDLSOUND_SetErrorCode( status ) \
   SDLSOUND_ErrorCode   = ( status );

static void isr(void *userdata, char *stream, int32_t len);

/*
 * DisableInterrupts
 * Enter the critical section.
 */
int32_t DisableInterrupts(void)
{
    SDL_LockAudio();

    return 0;
}


/*
 * RestoreInterrupts
 * Leave the critical section.
 */
int32_t RestoreInterrupts(int32_t a)
{
    SDL_UnlockAudio();

    return 0;
    a=a;
}


/*
 * SDLSOUND_ErrorString
 * Returns a description of an error code.
 */
char *SDLSOUND_ErrorString(int32_t errorcode)
{
    switch (errorcode)
    {
    case SDLSOUND_Warning:
    case SDLSOUND_Error:
        return SDLSOUND_ErrorString(SDLSOUND_ErrorCode);

    case SDLSOUND_Ok:
        return "SDL Sound ok.";

    default:
        return "Unknown SDL sound error code.";
    }
}


/*
 * SDLSOUND_Init
 * Initializes the SDL sound objects.
 */
int32_t SDLSOUND_Init(int32_t soundcard, int32_t mixrate, int32_t numchannels, int32_t samplebits, int32_t buffersize)
{
    SDL_AudioSpec spec,got;

    if (SDLSOUND_Installed)
    {
        SDLSOUND_Shutdown();
    }

    initprintf("Initializing SDL sound...\n");

    initprintf("  - Requested sound format\n"
               "      Channels:    %d\n"
               "      Sample rate: %dHz\n"
               "      Sample size: %d bits\n",
               numchannels, mixrate, samplebits);

    spec.freq = mixrate;
    spec.format = (samplebits == 8 ? AUDIO_U8 : AUDIO_S16LSB);
    spec.channels = (numchannels == 1 ? 1:2);
    spec.samples = (buffersize >> (spec.channels-1)) >> (samplebits==16);
    spec.callback = isr;
    spec.userdata = NULL;


    SDLSOUND_Installed = TRUE;

    SDLSOUND_SetErrorCode(SDLSOUND_Ok);
    return SDLSOUND_Ok;
}


/*
 * SDLSOUND_Shutdown
 * Shuts down SDL sound and it's associates.
 */
int32_t SDLSOUND_Shutdown(void)
{
    int32_t i;

    if (SDLSOUND_Installed) initprintf("Uninitializing SDL sound...\n");

    SDLSOUND_Installed = FALSE;

    SDLSOUND_StopPlayback();


    SDLSOUND_SetErrorCode(SDLSOUND_Ok);
    return SDLSOUND_Ok;
}


/*
 * SDLSOUND_SetMixMode
 * Bit of filler for the future.
 */
int32_t SDLSOUND_SetMixMode(int32_t mode)
{
    return mode;
}


static void isr(void *userdata, char *stream, int32_t len)
{
    // otherwise we just service the interrupt
    if (_DSOUND_CallBack)
    {

        p = _DSOUND_CallBack(rv-WAIT_OBJECT_0-1);

        hr = IDirectSoundBuffer_Lock(lpDSBSecondary, p*_DSOUND_BufferLength, _DSOUND_BufferLength,
                                     &lockptr, &lockbytes, &lockptr2, &lockbytes2, 0);
        if (hr == DSERR_BUFFERLOST)
        {
            hr = IDirectSoundBuffer_Restore(lpDSBSecondary);
        }
        if (hr == DS_OK)
        {
            /*
            #define copybuf(S,D,c) \
            ({ void *__S=(S), *__D=(D); int32_t __c=(c); \
            __asm__ __volatile__ ("rep; movsl" \
            : "+S" (__S), "+D" (__D), "+c" (__c) : : "memory", "cc"); \
            0; })
            */
            //copybuf(_DSOUND_MixBuffer + p * _DSOUND_BufferLength, lockptr, _DSOUND_BufferLength >> 2);
            memcpy(lockptr, _DSOUND_MixBuffer + p * _DSOUND_BufferLength, _DSOUND_BufferLength);
            IDirectSoundBuffer_Unlock(lpDSBSecondary, lockptr, lockbytes, lockptr2, lockbytes2);
        }

    }
}
}


/*
 * SDLSOUND_BeginBufferedPlayback
 * Unpause SDL sound playback.
 */
int32_t DSOUND_BeginBufferedPlayback(char *BufferStart, int32_t(*CallBackFunc)(int32_t), int32_t buffersize, int32_t numdivisions)
{
    _SDLSOUND_CallBack = CallBackFunc;
    _SDLSOUND_MixBuffer = BufferStart;

    _SDLSOUND_BufferLength = buffersize/numdivisions;
    _SDLSOUND_NumBuffers   = numdivisions;

    return SDLSOUND_Ok;
}


/*
 * DSOUND_StopPlayback
 * Halts the playback thread.
 */
int32_t DSOUND_StopPlayback(void)
{
//	DWORD exitcode;
    BOOL t;
    int32_t i;

    if (isrthread)
    {
        SetEvent(isrfinish);

        initprintf("DirectSound: Waiting for sound thread to exit\n");
        if (WaitForSingleObject(isrthread, 300) == WAIT_OBJECT_0)
            initprintf("DirectSound: Sound thread has exited\n");
        else
            initprintf("DirectSound: Sound thread failed to exit!\n");
        /*
        while (1) {
        	if (!GetExitCodeThread(isrthread, &exitcode)) {
        		DSOUND_SetErrorCode(DSOUND_FailedGetExitCode);
        		return DSOUND_Warning;
        	}
        	if (exitcode != STILL_ACTIVE) break;
        }*/

        CloseHandle(isrthread);
        isrthread = NULL;
    }

    if (isrfinish)
    {
        CloseHandle(isrfinish);
        isrfinish = NULL;
    }

    if (lpDSBSecondary)
    {
        IDirectSoundBuffer_Stop(lpDSBSecondary);
    }

    if (hPosNotify)
    {
        for (i=0; i<_DSOUND_NumBuffers; i++)
        {
            if (hPosNotify[i]) CloseHandle(hPosNotify[i]);
        }
        free(hPosNotify);
        hPosNotify = NULL;
    }

    return DSOUND_Ok;
}


