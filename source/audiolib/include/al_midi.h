/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

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
#ifndef __AL_MIDI_H
#define __AL_MIDI_H

#include "opl3.h"

#include <inttypes.h>

typedef struct
{
    uint8_t SAVEK[2];
    uint8_t Level[2];
    uint8_t Env1[2];
    uint8_t Env2[2];
    uint8_t Wave[2];
    uint8_t Feedback;
    int8_t  Transpose;
    int8_t  Velocity;
} AdLibTimbre;

extern AdLibTimbre ADLIB_TimbreBank[256];

opl3_chip *AL_GetChip(void);
void AL_RegisterTimbreBank(uint8_t *timbres);
void AL_SetStereo(int const stereo);

#endif
