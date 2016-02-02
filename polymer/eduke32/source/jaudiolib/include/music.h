//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#ifndef __MUSIC_H
#define __MUSIC_H

#include "compat.h"

extern int32_t MUSIC_ErrorCode;

enum MUSIC_ERRORS
{
    MUSIC_Warning = -2,
    MUSIC_Error = -1,
    MUSIC_Ok = 0,
    MUSIC_MidiError,
};

#define MUSIC_LoopSong ( 1 == 1 )
#define MUSIC_PlayOnce ( !MUSIC_LoopSong )

const char *MUSIC_ErrorString(int32_t ErrorNumber);
int32_t     MUSIC_Init(int32_t SoundCard);
int32_t     MUSIC_Shutdown(void);
void        MUSIC_SetVolume(int32_t volume);
void        MUSIC_Continue(void);
void        MUSIC_Pause(void);
int32_t     MUSIC_StopSong(void);
int32_t     MUSIC_PlaySong(char *song, int32_t loopflag);
void        MUSIC_Update(void);

#endif
