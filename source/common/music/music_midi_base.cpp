/*
** music_midi_base.cpp
**
**---------------------------------------------------------------------------
** Copyright 1998-2010 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#define DEF_MIDIDEV -5

static uint32_t	nummididevices;

#define NUM_DEF_DEVICES 7

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

void I_InitMusicWin32 ()
{
	nummididevices = midiOutGetNumDevs ();
}


#endif

#include "c_dispatch.h"

#include "v_text.h"
#include "zmusic/zmusic.h"
#include "s_music.h"
#include "c_cvars.h"
#include "printf.h"

EXTERN_CVAR(Int, snd_mididevice)


CUSTOM_CVAR (Int, snd_mididevice, DEF_MIDIDEV, CVAR_ARCHIVE|CVAR_GLOBALCONFIG|CVAR_NOINITCALL)
{
	if ((self >= (signed)nummididevices) || (self < -5))
	{
		// Don't do repeated message spam if there is no valid device.
		if (self != 0)
		{
			Printf("ID out of range. Using default device.\n");
		}
		self = DEF_MIDIDEV;
		return;
	}
	else if (self == -1)
	{
		self = DEF_MIDIDEV;
		return;
	}
	bool change = ChangeMusicSetting(ZMusic::snd_mididevice, nullptr, self);
	if (change) S_MIDIDeviceChanged(self);
}
