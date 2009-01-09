//-------------------------------------------------------------------------
/*
Copyright (C) 2008 - EDuke32 developers

This file is part of EDuke32

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#ifndef __OPENAL_H
#define __OPENAL_H

#include <vorbis/vorbisfile.h>

int32_t AL_Init();
void AL_Shutdown();
void AL_Update();
char *AL_ErrorString(int32_t code);
char *ALC_ErrorString(int32_t code);

void AL_Stop();
int32_t  AL_isntALmusic();
void AL_PlaySong(char *song,int32_t loopflag);
void AL_Pause();
void AL_Continue();
void AL_SetMusicVolume(int32_t volume);

int32_t openal_disabled;

typedef struct sounddef
{
        unsigned pos;
        char *ptrsnd;
        unsigned size;
        OggVorbis_File  oggStream;
} sounddef;
#endif
