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
 * Abstraction layer for hiding the various supported sound devices
 * behind a common and opaque interface called on by MultiVoc.
 */

#include "drivers.h"

#include "driver_nosound.h"

#if defined MIXERTYPEWIN
# include "driver_directsound.h"
#elif defined MIXERTYPESDL
# include "driver_sdl.h"
#endif

int32_t ASS_SoundDriver = -1;

#define UNSUPPORTED { 0,0,0,0,0,0,0,0, },

static struct
{
    int32_t (*GetError)(void);
    const char *(*ErrorString)(int32_t);
    int32_t (*Init)(int32_t *, int32_t *, void *);
    void (*Shutdown)(void);
    int32_t (*BeginPlayback)(char *, int32_t, int32_t, void (*)(void));
    void (*StopPlayback)(void);
    void (*Lock)(void);
    void (*Unlock)(void);
} SoundDrivers[ASS_NumSoundCards] = {

    // Everyone gets the "no sound" driver
    {
    NoSoundDrv_GetError, NoSoundDrv_ErrorString, NoSoundDrv_PCM_Init, NoSoundDrv_PCM_Shutdown,
    NoSoundDrv_PCM_BeginPlayback, NoSoundDrv_PCM_StopPlayback, NoSoundDrv_PCM_Lock, NoSoundDrv_PCM_Unlock,
    },

    // Windows DirectSound
#if defined MIXERTYPEWIN
    {
    DirectSoundDrv_GetError, DirectSoundDrv_ErrorString, DirectSoundDrv_PCM_Init, DirectSoundDrv_PCM_Shutdown,
    DirectSoundDrv_PCM_BeginPlayback, DirectSoundDrv_PCM_StopPlayback, DirectSoundDrv_PCM_Lock,
    DirectSoundDrv_PCM_Unlock,
    },
    // Simple DirectMedia Layer
#elif defined MIXERTYPESDL
    {
    SDLDrv_GetError, SDLDrv_ErrorString, SDLDrv_PCM_Init, SDLDrv_PCM_Shutdown,
    SDLDrv_PCM_BeginPlayback, SDLDrv_PCM_StopPlayback, SDLDrv_PCM_Lock, SDLDrv_PCM_Unlock,
    },
#endif
};

int32_t SoundDriver_IsSupported(int32_t driver) { return (SoundDrivers[driver].GetError != 0); }

int32_t SoundDriver_GetError(void)
{
    return SoundDriver_IsSupported(ASS_SoundDriver) ? SoundDrivers[ASS_SoundDriver].GetError() : -1;
}

const char * SoundDriver_ErrorString( int32_t ErrorNumber )
{
    if (ASS_SoundDriver < 0 || ASS_SoundDriver >= ASS_NumSoundCards)
        return "No sound driver selected.";

    if (!SoundDriver_IsSupported(ASS_SoundDriver))
        return "Unsupported sound driver selected.";

    return SoundDrivers[ASS_SoundDriver].ErrorString(ErrorNumber);
}

int32_t SoundDriver_Init(int32_t *mixrate, int32_t *numchannels, void *initdata)
{
    return SoundDrivers[ASS_SoundDriver].Init(mixrate, numchannels, initdata);
}

void SoundDriver_Shutdown(void) { SoundDrivers[ASS_SoundDriver].Shutdown(); }

int32_t SoundDriver_BeginPlayback(char *BufferStart, int32_t BufferSize, int32_t NumDivisions,
                                  void (*CallBackFunc)(void))
{
    return SoundDrivers[ASS_SoundDriver].BeginPlayback(BufferStart, BufferSize, NumDivisions, CallBackFunc);
}

void SoundDriver_StopPlayback(void) { SoundDrivers[ASS_SoundDriver].StopPlayback(); }

void SoundDriver_Lock(void) { SoundDrivers[ASS_SoundDriver].Lock(); }

void SoundDriver_Unlock(void) { SoundDrivers[ASS_SoundDriver].Unlock(); }
