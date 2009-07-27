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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
 */

/**
 * Abstraction layer for hiding the various supported sound devices
 * behind a common and opaque interface called on by MultiVoc.
 */

#include "drivers.h"

#include "driver_nosound.h"

#ifdef HAVE_SDL
# include "driver_sdl.h"
#endif

#ifdef __APPLE__
# include "driver_coreaudio.h"
#endif

#ifdef WIN32
# include "driver_directsound.h"
#endif

int32_t ASS_SoundDriver = -1;

#define UNSUPPORTED { 0,0,0,0,0,0,0,0, },

static struct {
	int32_t          (* GetError)(void);
	const char * (* ErrorString)(int32_t);
	int32_t          (* Init)(int32_t, int32_t, int32_t, void *);
	void         (* Shutdown)(void);
	int32_t          (* BeginPlayback)(char *, int32_t, int32_t, void ( * )(void) );
	void         (* StopPlayback)(void);
	void         (* Lock)(void);
	void         (* Unlock)(void);
} SoundDrivers[ASS_NumSoundCards] = {
	
	// Everyone gets the "no sound" driver
	{
		NoSoundDrv_GetError,
		NoSoundDrv_ErrorString,
		NoSoundDrv_Init,
		NoSoundDrv_Shutdown,
		NoSoundDrv_BeginPlayback,
		NoSoundDrv_StopPlayback,
		NoSoundDrv_Lock,
		NoSoundDrv_Unlock,
	},
	
	// Simple DirectMedia Layer
	#ifdef HAVE_SDL
	{
		SDLDrv_GetError,
		SDLDrv_ErrorString,
		SDLDrv_Init,
		SDLDrv_Shutdown,
		SDLDrv_BeginPlayback,
		SDLDrv_StopPlayback,
		SDLDrv_Lock,
		SDLDrv_Unlock,
	},
	#else
		UNSUPPORTED
	#endif
	
	// OS X CoreAudio
	#ifdef __APPLE__
	{
		CoreAudioDrv_GetError,
		CoreAudioDrv_ErrorString,
		CoreAudioDrv_Init,
		CoreAudioDrv_Shutdown,
		CoreAudioDrv_BeginPlayback,
		CoreAudioDrv_StopPlayback,
		CoreAudioDrv_Lock,
		CoreAudioDrv_Unlock,
	},
	#else
		UNSUPPORTED
	#endif
	
	// Windows DirectSound
	#ifdef WIN32
	{
		DirectSoundDrv_GetError,
		DirectSoundDrv_ErrorString,
		DirectSoundDrv_Init,
		DirectSoundDrv_Shutdown,
		DirectSoundDrv_BeginPlayback,
		DirectSoundDrv_StopPlayback,
		DirectSoundDrv_Lock,
		DirectSoundDrv_Unlock,
	},
	#else
		UNSUPPORTED
	#endif
};


int32_t SoundDriver_IsSupported(int32_t driver)
{
	return (SoundDrivers[driver].GetError != 0);
}


int32_t SoundDriver_GetError(void)
{
	if (!SoundDriver_IsSupported(ASS_SoundDriver)) {
		return -1;
	}
	return SoundDrivers[ASS_SoundDriver].GetError();
}

const char * SoundDriver_ErrorString( int32_t ErrorNumber )
{
	if (ASS_SoundDriver < 0 || ASS_SoundDriver >= ASS_NumSoundCards) {
		return "No sound driver selected.";
	}
	if (!SoundDriver_IsSupported(ASS_SoundDriver)) {
		return "Unsupported sound driver selected.";
	}
	return SoundDrivers[ASS_SoundDriver].ErrorString(ErrorNumber);
}

int32_t SoundDriver_Init(int32_t mixrate, int32_t numchannels, int32_t samplebits, void * initdata)
{
	return SoundDrivers[ASS_SoundDriver].Init(mixrate, numchannels, samplebits, initdata);
}

void SoundDriver_Shutdown(void)
{
	SoundDrivers[ASS_SoundDriver].Shutdown();
}

int32_t SoundDriver_BeginPlayback(char *BufferStart, int32_t BufferSize,
		int32_t NumDivisions, void ( *CallBackFunc )( void ) )
{
	return SoundDrivers[ASS_SoundDriver].BeginPlayback(BufferStart,
			BufferSize, NumDivisions, CallBackFunc);
}

void SoundDriver_StopPlayback(void)
{
	SoundDrivers[ASS_SoundDriver].StopPlayback();
}

void SoundDriver_Lock(void)
{
	SoundDrivers[ASS_SoundDriver].Lock();
}

void SoundDriver_Unlock(void)
{
	SoundDrivers[ASS_SoundDriver].Unlock();
}
