//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

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

#ifndef anim_h_
#define anim_h_

#include "compat.h"
#include "hash.h"

typedef struct {
    uint16_t frame;
    int16_t sound;
} animsound_t;

typedef struct
{
    double frameaspect1, frameaspect2;
    uint8_t* animbuf;
    animsound_t *sounds;
    uint16_t numsounds;
    uint8_t framedelay;
    uint8_t frameflags;
    char animlock;
} dukeanim_t;

extern dukeanim_t * g_animPtr;
extern hashtable_t h_dukeanim;
extern dukeanim_t * Anim_Find(const char *s);
extern dukeanim_t * Anim_Create(const char *fn);
int32_t Anim_Play(const char *fn);
void Anim_Init(void);

#endif
