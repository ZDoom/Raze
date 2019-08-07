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
 * DirectSound output driver for MultiVoc
 */

#define NEED_MMSYSTEM_H
#define NEED_DSOUND_H

#include "driver_directsound.h"

#include "compat.h"
#include "multivoc.h"
#include "mutex.h"
#include "windows_inc.h"

#define MIXBUFFERPOSITIONS 8

static int32_t ErrorCode;
static int32_t Initialised;
static int32_t Playing;

static char *  MixBuffer;
static int32_t MixBufferSize;
static int32_t MixBufferCount;
static int32_t MixBufferCurrent;
static int32_t MixBufferUsed;

static void (*MixCallBack)(void);

static LPDIRECTSOUND lpds;
static LPDIRECTSOUNDBUFFER lpdsbprimary, lpdsbsec;
static LPDIRECTSOUNDNOTIFY lpdsnotify;

static HANDLE mixThread;
static mutex_t mutex;

static DSBPOSITIONNOTIFY notifyPositions[MIXBUFFERPOSITIONS + 1] = {};

static void FillBufferPosition(char * ptr, int32_t remaining)
{
    int32_t len = 0;

    do
    {
        if (MixBufferUsed == MixBufferSize)
        {
            MixCallBack();
            MixBufferUsed = 0;

            if (++MixBufferCurrent >= MixBufferCount)
                MixBufferCurrent -= MixBufferCount;
        }

        do
        {
            char *sptr = MixBuffer + (MixBufferCurrent * MixBufferSize) + MixBufferUsed;

            len = MixBufferSize - MixBufferUsed;

            if (remaining < len)
                len = remaining;

            memcpy(ptr, sptr, len);

            ptr += len;
            MixBufferUsed += len;
            remaining -= len;
        }
        while (remaining >= len && MixBufferUsed < MixBufferSize);
    }
    while (remaining >= len);
}

static void FillBuffer(int32_t bufnum)
{
    LPVOID ptr, ptr2;
    DWORD remaining, remaining2;
    int32_t retries = 1;

    do
    {
        HRESULT err = IDirectSoundBuffer_Lock(lpdsbsec, notifyPositions[bufnum].dwOffset, notifyPositions[1].dwOffset,
                                              &ptr, &remaining, &ptr2, &remaining2, 0);

        if (EDUKE32_PREDICT_FALSE(FAILED(err)))
        {
            if (err == DSERR_BUFFERLOST)
            {
                if (FAILED(err = IDirectSoundBuffer_Restore(lpdsbsec)))
                    goto fail;

                if (retries-- > 0)
                    continue;
            }
fail:
            if (MV_Printf)
                MV_Printf("DirectSound FillBuffer: err %x\n", (uint32_t)err);

            return;
        }
        break;
    }
    while (1);

    if (ptr && remaining)
        FillBufferPosition((char *)ptr, remaining);

    if (ptr2 && remaining2)
        FillBufferPosition((char *)ptr2, remaining2);

    IDirectSoundBuffer_Unlock(lpdsbsec, ptr, remaining, ptr2, remaining2);
}

static DWORD WINAPI fillDataThread(LPVOID lpParameter)
{
    UNREFERENCED_PARAMETER(lpParameter);

    HANDLE handles[MIXBUFFERPOSITIONS+1];

    for (int i = 0; i < ARRAY_SSIZE(handles); i++)
        handles[i] = notifyPositions[i].hEventNotify;

    do
    {
        DWORD const waitret = WaitForMultipleObjects(MIXBUFFERPOSITIONS, handles, FALSE, INFINITE);

        if (waitret >= WAIT_OBJECT_0 && waitret < WAIT_OBJECT_0+MIXBUFFERPOSITIONS)
        {
            mutex_lock(&mutex);
            FillBuffer((waitret + MIXBUFFERPOSITIONS - 1 - WAIT_OBJECT_0) % MIXBUFFERPOSITIONS);
            mutex_unlock(&mutex);
        }
        else
        {
            switch (waitret)
            {
                case WAIT_OBJECT_0 + MIXBUFFERPOSITIONS:
                    ExitThread(0);
                    break;

                default:
                    if (MV_Printf)
                        MV_Printf("DirectSound fillDataThread: wfmo err %d\n", (int32_t)waitret);
                    break;
            }
        }
    }
    while (1);

    return 0;
}

static void TeardownDSound(HRESULT err)
{
    if (FAILED(err))
    {
        if (MV_Printf)
            MV_Printf("Dying error: %x\n", (uint32_t)err);
    }

    if (lpdsnotify)
        IDirectSoundNotify_Release(lpdsnotify), lpdsnotify = NULL;

    for (int i = 0; i < MIXBUFFERPOSITIONS + 1; i++)
    {
        if (notifyPositions[i].hEventNotify)
            CloseHandle(notifyPositions[i].hEventNotify);
        notifyPositions[i].hEventNotify = 0;
    }

#ifdef RENDERTYPEWIN
    if (mutex)
        CloseHandle(mutex), mutex = NULL;
#endif

    if (lpdsbsec)
        IDirectSoundBuffer_Release(lpdsbsec), lpdsbsec = NULL;

    if (lpdsbprimary)
        IDirectSoundBuffer_Release(lpdsbprimary), lpdsbprimary = NULL;

    if (lpds)
        IDirectSound_Release(lpds), lpds = NULL;
}

static int DirectSound_Error(HRESULT err, int code)
{
    TeardownDSound(err);
    ErrorCode = code;
    return DSErr_Error;
}

int32_t DirectSoundDrv_PCM_Init(int32_t *mixrate, int32_t *numchannels, void * initdata)
{
    HRESULT err;
    DSBUFFERDESC bufdesc = {};
    WAVEFORMATEX wfex    = {};

    if (Initialised)
        DirectSoundDrv_PCM_Shutdown();

    if (FAILED(err = DirectSoundCreate(0, &lpds, 0)))
        return DirectSound_Error(err, DSErr_DirectSoundCreate);

    if (FAILED(err = IDirectSound_SetCooperativeLevel(lpds, (HWND) initdata, DSSCL_PRIORITY)))
        return DirectSound_Error(err, DSErr_SetCooperativeLevel);

    bufdesc.dwSize = sizeof(DSBUFFERDESC);
    bufdesc.dwFlags = DSBCAPS_LOCSOFTWARE | DSBCAPS_PRIMARYBUFFER | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_STICKYFOCUS;

    if (FAILED(err = IDirectSound_CreateSoundBuffer(lpds, &bufdesc, &lpdsbprimary, 0)))
        return DirectSound_Error(err, DSErr_CreateSoundBuffer);

    wfex.wFormatTag      = WAVE_FORMAT_PCM;
    wfex.nChannels       = *numchannels;
    wfex.nSamplesPerSec  = *mixrate;
    wfex.wBitsPerSample  = 16;
    wfex.nBlockAlign     = wfex.nChannels * wfex.wBitsPerSample / 8;
    wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;

    if (FAILED(err = IDirectSoundBuffer_SetFormat(lpdsbprimary, &wfex)))
        return DirectSound_Error(err, DSErr_SetFormat);

    bufdesc.dwFlags = DSBCAPS_LOCSOFTWARE | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_STICKYFOCUS;

    bufdesc.dwBufferBytes = wfex.nBlockAlign * 2048 * 2;
    bufdesc.lpwfxFormat = &wfex;

    if (FAILED(err = IDirectSound_CreateSoundBuffer(lpds, &bufdesc, &lpdsbsec, 0)))
        return DirectSound_Error(err, DSErr_CreateSoundBufferSecondary);

    if (FAILED(err = IDirectSoundBuffer_QueryInterface(lpdsbsec, &IID_IDirectSoundNotify, (LPVOID *)&lpdsnotify)))
        return DirectSound_Error(err, DSErr_Notify);

    for (int i = 0; i < MIXBUFFERPOSITIONS; i++)
    {
        notifyPositions[i].dwOffset = (bufdesc.dwBufferBytes/MIXBUFFERPOSITIONS)*i;
        notifyPositions[i].hEventNotify = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!notifyPositions[i].hEventNotify)
            return DirectSound_Error(DS_OK, DSErr_NotifyEvents);
    }

    notifyPositions[MIXBUFFERPOSITIONS].dwOffset = DSBPN_OFFSETSTOP;
    notifyPositions[MIXBUFFERPOSITIONS].hEventNotify = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (FAILED(err = IDirectSoundNotify_SetNotificationPositions(lpdsnotify, MIXBUFFERPOSITIONS+1, notifyPositions)))
        return DirectSound_Error(err, DSErr_SetNotificationPositions);

    if (FAILED(err = IDirectSoundBuffer_Play(lpdsbprimary, 0, 0, DSBPLAY_LOOPING)))
        return DirectSound_Error(err, DSErr_Play);

    mutex_init(&mutex);

    Initialised = 1;

    return DSErr_Ok;
}

void DirectSoundDrv_PCM_Shutdown(void)
{
    if (!Initialised)
        return;

    DirectSoundDrv_PCM_StopPlayback();
    TeardownDSound(DS_OK);

    Initialised = 0;
}

int32_t DirectSoundDrv_PCM_BeginPlayback(char *BufferStart, int32_t BufferSize, int32_t NumDivisions, void (*CallBackFunc)(void))
{
    if (!Initialised)
    {
        ErrorCode = DSErr_Uninitialised;
        return DSErr_Error;
    }

    DirectSoundDrv_PCM_StopPlayback();

    MixBuffer        = BufferStart;
    MixBufferSize    = BufferSize;
    MixBufferCount   = NumDivisions;
    MixBufferCurrent = 0;
    MixBufferUsed    = 0;
    MixCallBack      = CallBackFunc;

    // prime the buffer
    FillBuffer(0);

    if ((mixThread = CreateThread(NULL, 0, fillDataThread, 0, 0, 0)) == NULL)
    {
        ErrorCode = DSErr_CreateThread;
        return DSErr_Error;
    }

    SetThreadPriority(mixThread, THREAD_PRIORITY_ABOVE_NORMAL);

    HRESULT err = IDirectSoundBuffer_Play(lpdsbsec, 0, 0, DSBPLAY_LOOPING);

    if (FAILED(err))
    {
        ErrorCode = DSErr_PlaySecondary;
        return DSErr_Error;
    }

    Playing = 1;

    return DSErr_Ok;
}

void DirectSoundDrv_PCM_StopPlayback(void)
{
    if (!Playing)
        return;

    IDirectSoundBuffer_Stop(lpdsbsec);
    IDirectSoundBuffer_SetCurrentPosition(lpdsbsec, 0);

    Playing = 0;
}

void DirectSoundDrv_PCM_Lock(void)
{
    mutex_lock(&mutex);
}

void DirectSoundDrv_PCM_Unlock(void)
{
    mutex_unlock(&mutex);
}

int32_t DirectSoundDrv_GetError(void)
{
    return ErrorCode;
}

const char *DirectSoundDrv_ErrorString(int32_t ErrorNumber)
{
    const char *ErrorString;

    switch (ErrorNumber)
    {
        case DSErr_Warning:
        case DSErr_Error:
            ErrorString = DirectSoundDrv_ErrorString(ErrorCode);
            break;

        case DSErr_Ok:
            ErrorString = "DirectSound ok.";
            break;

        case DSErr_Uninitialised:
            ErrorString = "DirectSound uninitialised.";
            break;

        case DSErr_DirectSoundCreate:
            ErrorString = "DirectSound error: DirectSoundCreate failed.";
            break;

        case DSErr_SetCooperativeLevel:
            ErrorString = "DirectSound error: SetCooperativeLevel failed.";
            break;

        case DSErr_CreateSoundBuffer:
            ErrorString = "DirectSound error: primary CreateSoundBuffer failed.";
            break;

        case DSErr_CreateSoundBufferSecondary:
            ErrorString = "DirectSound error: secondary CreateSoundBuffer failed.";
            break;

        case DSErr_SetFormat:
            ErrorString = "DirectSound error: primary buffer SetFormat failed.";
            break;

        case DSErr_SetFormatSecondary:
            ErrorString = "DirectSound error: secondary buffer SetFormat failed.";
            break;

        case DSErr_Notify:
            ErrorString = "DirectSound error: failed querying secondary buffer for notify interface.";
            break;

        case DSErr_NotifyEvents:
            ErrorString = "DirectSound error: failed creating notify events.";
            break;

        case DSErr_SetNotificationPositions:
            ErrorString = "DirectSound error: failed setting notification positions.";
            break;

        case DSErr_Play:
            ErrorString = "DirectSound error: primary buffer Play failed.";
            break;

        case DSErr_PlaySecondary:
            ErrorString = "DirectSound error: secondary buffer Play failed.";
            break;

        case DSErr_CreateThread:
            ErrorString = "DirectSound error: failed creating mix thread.";
            break;

        default:
            ErrorString = "Unknown DirectSound error code.";
            break;
    }

    return ErrorString;
}
