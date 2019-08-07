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

#include "inttypes.h"

enum
{
    DSErr_Warning = -2,
    DSErr_Error   = -1,
    DSErr_Ok      = 0,
    DSErr_Uninitialised,
    DSErr_DirectSoundCreate,
    DSErr_SetCooperativeLevel,
    DSErr_CreateSoundBuffer,
    DSErr_CreateSoundBufferSecondary,
    DSErr_SetFormat,
    DSErr_SetFormatSecondary,
    DSErr_Notify,
    DSErr_NotifyEvents,
    DSErr_SetNotificationPositions,
    DSErr_Play,
    DSErr_PlaySecondary,
    DSErr_CreateThread,
};

int32_t DirectSoundDrv_GetError(void);
const char *DirectSoundDrv_ErrorString(int32_t ErrorNumber);

int32_t DirectSoundDrv_PCM_Init(int32_t *mixrate, int32_t *numchannels, void *initdata);
int32_t DirectSoundDrv_PCM_BeginPlayback(char *BufferStart, int32_t BufferSize, int32_t NumDivisions, void (*CallBackFunc)(void));
void    DirectSoundDrv_PCM_StopPlayback(void);
void    DirectSoundDrv_PCM_Lock(void);
void    DirectSoundDrv_PCM_Unlock(void);
void    DirectSoundDrv_PCM_Shutdown(void);
