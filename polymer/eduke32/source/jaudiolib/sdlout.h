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

#ifndef __sdlout_h__
#define __sdlout_h__

enum SDLSOUND_ERRORS {
	SDLSOUND_Warning = -2,
	SDLSOUND_Error = -1,
	SDLSOUND_Ok = 0
};

extern int32_t SDLSOUND_ErrorCode;

char *SDLSOUND_ErrorString(int32_t);

int32_t DisableInterrupts(void);	// simulated using critical sections
int32_t RestoreInterrupts(int32_t);

int32_t SDLSOUND_Init(int32_t soundcard, int32_t mixrate, int32_t numchannels, int32_t samplebits, int32_t buffersize);
int32_t SDLSOUND_Shutdown(void);

int32_t SDLSOUND_SetMixMode(int32_t mode);
int32_t SDLSOUND_BeginBufferedPlayback(char *BufferStart, int32_t (*CallBackFunc)(int32_t), int32_t buffersize, int32_t numdivisions);
int32_t SDLSOUND_StopPlayback(void);

#endif	// __sdlout_h__

