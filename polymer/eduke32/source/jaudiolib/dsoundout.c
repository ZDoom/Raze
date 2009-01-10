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

#include "dsound.h"
#include "dsoundout.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>

#ifdef USE_OPENAL
#include "openal.h"
#endif
#include "compat.h"
#include "winlayer.h"

#if defined(__WATCOMC__) || defined(_MSC_VER)
#include <malloc.h>
#endif


#ifndef RENDERTYPEWIN
#error The DirectSound output module for AudioLib only works with the Windows interface.
#endif

static CRITICAL_SECTION mutex;
static int32_t _DSOUND_CriticalSectionAlloced = FALSE;
static HANDLE isrthread = NULL;
static HANDLE isrfinish = NULL;

static HMODULE             hDSoundDLL = NULL;
static LPDIRECTSOUND       lpDS = NULL;
static LPDIRECTSOUNDBUFFER lpDSBPrimary = NULL;
static LPDIRECTSOUNDBUFFER lpDSBSecondary = NULL;
static LPDIRECTSOUNDNOTIFY lpDSNotify = NULL;
static HANDLE              *hPosNotify = NULL;

static int32_t(*_DSOUND_CallBack)(int32_t) = NULL;
static int32_t _DSOUND_BufferLength = 0;
static int32_t _DSOUND_NumBuffers   = 0;
static char *_DSOUND_MixBuffer  = NULL;

static int32_t DSOUND_Installed = FALSE;

int32_t DSOUND_ErrorCode = DSOUND_Ok;

#define DSOUND_SetErrorCode( status ) \
   DSOUND_ErrorCode   = ( status );



/*
 * DisableInterrupts
 * Enter the critical section.
 */
int32_t DisableInterrupts(void)
{
    if (!_DSOUND_CriticalSectionAlloced) return -1;

    EnterCriticalSection(&mutex);

    return 0;
}


/*
 * RestoreInterrupts
 * Leave the critical section.
 */
int32_t RestoreInterrupts(int32_t a)
{
    if (!_DSOUND_CriticalSectionAlloced) return -1;

    LeaveCriticalSection(&mutex);

    return 0;
    a=a;
}


/*
 * DSOUND_ErrorString
 * Returns a description of an error code.
 */
char *DSOUND_ErrorString(int32_t errorcode)
{
    switch (errorcode)
    {
    case DSOUND_Warning:
    case DSOUND_Error:
        return DSOUND_ErrorString(DSOUND_ErrorCode);

    case DSOUND_Ok:
        return "DirectSound ok.";

    case DSOUND_NoDLL:
        return "Failed loading DSOUND.DLL.";

    case DSOUND_NoDirectSoundCreate:
        return "Failed getting DirectSoundCreate entry point.";

    case DSOUND_FailedDSC:
        return "DirectSoundCreate failed.";

    case DSOUND_FailedSetCoopLevel:
        return "Failed setting cooperative level.";

    case DSOUND_FailedCreatePrimary:
        return "Failed creating primary buffer.";

    case DSOUND_FailedSetFormat:
        return "Failed setting primary buffer format.";

    case DSOUND_FailedCreateSecondary:
        return "Failed creating secondary buffer.";

    case DSOUND_FailedCreateNotifyEvent:
        return "Failed creating notification event object.";

    case DSOUND_FailedQueryNotify:
        return "Failed querying notification interface.";

    case DSOUND_FailedSetNotify:
        return "Failed setting notification positions.";

    case DSOUND_FailedCreateFinishEvent:
        return "Failed creating finish event object.";

    case DSOUND_FailedCreateThread:
        return "Failed creating playback thread.";

    case DSOUND_FailedPlaySecondary:
        return "Failed playing secondary buffer.";

    case DSOUND_FailedGetExitCode:
        return "GetExitCodeThread failed.";

    default:
        return "Unknown DirectSound error code.";
    }
}


/*
 * DSOUND_Init
 * Initializes the DirectSound objects.
 */
int32_t DSOUND_Init(int32_t soundcard, int32_t mixrate, int32_t numchannels, int32_t samplebits, int32_t buffersize)
{
    HRESULT(WINAPI *aDirectSoundCreate)(LPGUID,LPDIRECTSOUND*,LPUNKNOWN);
    HRESULT hr;
    DSBUFFERDESC dsbuf;
    WAVEFORMATEX wfex;
    DSBPOSITIONNOTIFY posn;

    UNREFERENCED_PARAMETER(soundcard);

    if (DSOUND_Installed)
    {
        DSOUND_Shutdown();
    }

    initprintf("Initializing DirectSound...\n");

    if (!_DSOUND_CriticalSectionAlloced)
    {
        // initialize the critical section object we'll use to
        // simulate (dis|en)abling interrupts
        InitializeCriticalSection(&mutex);
        _DSOUND_CriticalSectionAlloced = TRUE;
    }

//    initprintf("  - Loading DSOUND.DLL\n");
    hDSoundDLL = LoadLibrary("DSOUND.DLL");
    if (!hDSoundDLL)
    {
        DSOUND_Shutdown();
        DSOUND_SetErrorCode(DSOUND_NoDLL);
        return DSOUND_Error;
    }

    aDirectSoundCreate = (void *)GetProcAddress(hDSoundDLL, "DirectSoundCreate");
    if (!aDirectSoundCreate)
    {
        DSOUND_Shutdown();
        DSOUND_SetErrorCode(DSOUND_NoDirectSoundCreate);
        return DSOUND_Error;
    }

//    initprintf("  - Creating DirectSound object\n");
    hr = aDirectSoundCreate(NULL, &lpDS, NULL);
    if (hr != DS_OK)
    {
        DSOUND_Shutdown();
        DSOUND_SetErrorCode(DSOUND_FailedDSC);
        return DSOUND_Error;
    }

    hr = IDirectSound_SetCooperativeLevel(lpDS, (HWND)win_gethwnd(), DSSCL_EXCLUSIVE);
    if (hr != DS_OK)
    {
        DSOUND_Shutdown();
        DSOUND_SetErrorCode(DSOUND_FailedSetCoopLevel);
        return DSOUND_Error;
    }

//    initprintf("  - Creating primary buffer\n");
    ZeroMemory(&dsbuf, sizeof(dsbuf));
    dsbuf.dwSize = sizeof(DSBUFFERDESC);
    dsbuf.dwFlags = DSBCAPS_PRIMARYBUFFER;
    hr = IDirectSound_CreateSoundBuffer(lpDS, &dsbuf, &lpDSBPrimary, NULL);
    if (hr != DS_OK)
    {
        DSOUND_Shutdown();
        DSOUND_SetErrorCode(DSOUND_FailedCreatePrimary);
        return DSOUND_Error;
    }

/*    initprintf("  - Setting primary buffer format\n"
               "      Channels:    %d\n"
               "      Sample rate: %dHz\n"
               "      Sample size: %d bits\n",
               numchannels, mixrate, samplebits); */
    initprintf("  - Primary buffer format: %d ch, %dHz, %d bits\n",
               numchannels, mixrate, samplebits);
    ZeroMemory(&wfex, sizeof(wfex));
    wfex.wFormatTag      = WAVE_FORMAT_PCM;
    wfex.nChannels       = numchannels;
    wfex.nSamplesPerSec  = mixrate;
    wfex.wBitsPerSample  = samplebits;
    wfex.nBlockAlign     = (wfex.wBitsPerSample / 8) * wfex.nChannels;
    wfex.nAvgBytesPerSec = wfex.nBlockAlign * wfex.nSamplesPerSec;
    hr = IDirectSoundBuffer_SetFormat(lpDSBPrimary, &wfex);
    if (hr != DS_OK)
    {
        DSOUND_Shutdown();
        DSOUND_SetErrorCode(DSOUND_FailedSetFormat);
        return DSOUND_Error;
    }

    initprintf("  - Creating secondary buffer\n");
    ZeroMemory(&dsbuf, sizeof(dsbuf));
    dsbuf.dwSize = sizeof(DSBUFFERDESC);
    dsbuf.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_LOCSOFTWARE;
    dsbuf.dwBufferBytes = buffersize;
    dsbuf.lpwfxFormat = &wfex;
    hr = IDirectSound_CreateSoundBuffer(lpDS, &dsbuf, &lpDSBSecondary, NULL);
    if (hr != DS_OK)
    {
        DSOUND_Shutdown();
        DSOUND_SetErrorCode(DSOUND_FailedCreateSecondary);
        return DSOUND_Error;
    }

    hr = IDirectSoundBuffer_QueryInterface(lpDSBSecondary, &IID_IDirectSoundNotify, (LPVOID *)&lpDSNotify);
    if (hr != DS_OK)
    {
        DSOUND_Shutdown();
        DSOUND_SetErrorCode(DSOUND_FailedQueryNotify);
        return DSOUND_Error;
    }

    hPosNotify = (HANDLE *)malloc(sizeof(HANDLE));
    if (!hPosNotify)
    {
        DSOUND_Shutdown();
        DSOUND_SetErrorCode(DSOUND_FailedSetNotify);
        return DSOUND_Error;
    }

    hPosNotify[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!hPosNotify)
    {
        DSOUND_Shutdown();
        DSOUND_SetErrorCode(DSOUND_FailedCreateNotifyEvent);
        return DSOUND_Error;
    }

    _DSOUND_BufferLength = 0;
    _DSOUND_NumBuffers   = 1;
    posn.dwOffset = 0;
    posn.hEventNotify = hPosNotify[0];

    hr = IDirectSoundNotify_SetNotificationPositions(lpDSNotify, 1, &posn);
    if (hr != DS_OK)
    {
        DSOUND_Shutdown();
        DSOUND_SetErrorCode(DSOUND_FailedSetNotify);
        return DSOUND_Error;
    }

    DSOUND_Installed = TRUE;

    DSOUND_SetErrorCode(DSOUND_Ok);
    return DSOUND_Ok;
}


/*
 * DSOUND_Shutdown
 * Shuts down DirectSound and it's associates.
 */
int32_t DSOUND_Shutdown(void)
{
    int32_t i;

    if (DSOUND_Installed) initprintf("Uninitializing DirectSound...\n");

    DSOUND_Installed = FALSE;

    DSOUND_StopPlayback();

    if (lpDSNotify)
    {
        IDirectSoundNotify_Release(lpDSNotify);
        lpDSNotify = NULL;
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

    if (lpDSBSecondary)
    {
//        initprintf("  - Releasing secondary buffer\n");
        IDirectSoundBuffer_Stop(lpDSBSecondary);
        IDirectSoundBuffer_Release(lpDSBSecondary);
        lpDSBSecondary = NULL;
    }

    if (lpDSBPrimary)
    {
//        initprintf("  - Releasing primary buffer\n");
        IDirectSoundBuffer_Release(lpDSBPrimary);
        lpDSBPrimary = NULL;
    }

    if (lpDS)
    {
//        initprintf("  - Releasing DirectSound object\n");
        IDirectSound_Release(lpDS);
        lpDS = NULL;
    }

    if (hDSoundDLL)
    {
//        initprintf("  - Unloading DSOUND.DLL\n");
        FreeLibrary(hDSoundDLL);
        hDSoundDLL = NULL;
    }

    DSOUND_SetErrorCode(DSOUND_Ok);
    return DSOUND_Ok;
}


/*
 * DSOUND_SetMixMode
 * Bit of filler for the future.
 */
int32_t DSOUND_SetMixMode(int32_t mode)
{
    return mode;
}


//#define DEBUGAUDIO

#ifdef DEBUGAUDIO
#include <sys/stat.h>
#endif
static DWORD WINAPI isr(LPVOID parm)
{
    HANDLE *handles;
    HRESULT hr;
    int32_t rv;
#ifdef DEBUGAUDIO
    int32_t h;
#endif
    int32_t p;
    LPVOID lockptr; DWORD lockbytes;
    LPVOID lockptr2; DWORD lockbytes2;

    UNREFERENCED_PARAMETER(parm);

    handles = (HANDLE *)malloc(sizeof(HANDLE)*(1+_DSOUND_NumBuffers));
    if (!handles) return 1;

    handles[0] = isrfinish;
    for (p=0; p<_DSOUND_NumBuffers; p++)
        handles[p+1] = hPosNotify[p];

#ifdef DEBUGAUDIO
    h = creat("audio.raw",S_IREAD|S_IWRITE);
#endif

    while (1)
    {
#ifdef USE_OPENAL
        AL_Update();
#endif
        rv = WaitForMultipleObjects(1+_DSOUND_NumBuffers, handles, FALSE, INFINITE);

        if (!(rv >= WAIT_OBJECT_0 && rv <= WAIT_OBJECT_0+1+_DSOUND_NumBuffers)) return -1;

        if (rv == WAIT_OBJECT_0)
        {
            // we've been asked to finish up
            break;
        }

        // otherwise we just service the interrupt
        if (_DSOUND_CallBack)
        {
            DisableInterrupts();

            p = _DSOUND_CallBack(rv-WAIT_OBJECT_0-1);
#ifdef DEBUGAUDIO
            write(h, _DSOUND_MixBuffer + p*_DSOUND_BufferLength, _DSOUND_BufferLength);
#endif

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

            RestoreInterrupts(0);
        }
    }

#ifdef DEBUGAUDIO
    close(h);
#endif

    return 0;
}


/*
 * DSOUND_BeginBufferedPlayback
 * Spins off a thread that behaves somewhat like the SoundBlaster DMA ISR did.
 */
int32_t DSOUND_BeginBufferedPlayback(char *BufferStart, int32_t(*CallBackFunc)(int32_t), int32_t buffersize, int32_t numdivisions)
{
    DWORD threadid;
    HRESULT hr;
    DSBPOSITIONNOTIFY *posns;
    int32_t i;

    _DSOUND_CallBack = CallBackFunc;
    _DSOUND_MixBuffer = BufferStart;

    if (!lpDSBSecondary) return DSOUND_Error;

    if (isrthread)
    {
        DSOUND_StopPlayback();
    }

    isrfinish = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!isrfinish)
    {
        DSOUND_SetErrorCode(DSOUND_FailedCreateFinishEvent);
        return DSOUND_Error;
    }

    isrthread = CreateThread(NULL, 0, isr, NULL, CREATE_SUSPENDED, &threadid);
    if (!isrthread)
    {
        DSOUND_SetErrorCode(DSOUND_FailedCreateThread);
        return DSOUND_Error;
    }

    hPosNotify = (HANDLE *)malloc(sizeof(HANDLE)*numdivisions);
    if (!hPosNotify)
    {
        DSOUND_Shutdown();
        DSOUND_SetErrorCode(DSOUND_FailedSetNotify);
        return DSOUND_Error;
    }

    memset(hPosNotify, 0, sizeof(HANDLE)*numdivisions);
    for (i=0; i<numdivisions; i++)
    {
        hPosNotify[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!hPosNotify[i])
        {
            DSOUND_Shutdown();
            DSOUND_SetErrorCode(DSOUND_FailedSetNotify);
            return DSOUND_Error;
        }
    }

    posns = (LPDSBPOSITIONNOTIFY)malloc(sizeof(DSBPOSITIONNOTIFY)*numdivisions);
    if (!posns)
    {
        DSOUND_Shutdown();
        DSOUND_SetErrorCode(DSOUND_FailedSetNotify);
        return DSOUND_Error;
    }

    _DSOUND_BufferLength = buffersize/numdivisions;
    _DSOUND_NumBuffers   = numdivisions;
    for (i=0; i<numdivisions; i++)
    {
        posns[i].dwOffset = i*_DSOUND_BufferLength;
        posns[i].hEventNotify = hPosNotify[i];
    }

    hr = IDirectSoundNotify_SetNotificationPositions(lpDSNotify, numdivisions, posns);
    if (hr != DS_OK)
    {
        free(posns);
        DSOUND_Shutdown();
        DSOUND_SetErrorCode(DSOUND_FailedSetNotify);
        return DSOUND_Error;
    }

    free(posns);

    SetThreadPriority(isrthread, THREAD_PRIORITY_ABOVE_NORMAL);
    ResumeThread(isrthread);

    hr = IDirectSoundBuffer_Play(lpDSBSecondary, 0, 0, DSBPLAY_LOOPING);
    if (hr != DS_OK)
    {
        DSOUND_SetErrorCode(DSOUND_FailedPlaySecondary);
        return DSOUND_Error;
    }

    return DSOUND_Ok;
}


/*
 * DSOUND_StopPlayback
 * Halts the playback thread.
 */
int32_t DSOUND_StopPlayback(void)
{
//	DWORD exitcode;
    int32_t i;

    if (isrthread)
    {
        SetEvent(isrfinish);

//        initprintf("DirectSound: Waiting for sound thread to exit\n");
//        if (WaitForSingleObject(isrthread, 300) == WAIT_OBJECT_0)
//            initprintf("DirectSound: Sound thread has exited\n");
//        else
//            initprintf("DirectSound: Sound thread failed to exit!\n");
        if (WaitForSingleObject(isrthread, 300) != WAIT_OBJECT_0)
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


