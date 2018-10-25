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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
/**********************************************************************
   module: PITCH.C

   author: James R. Dose
   date:   June 14, 1993

   Routines for pitch scaling.

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include "compat.h"
#include "pitch.h"

#define MAXDETUNE 50

static uint32_t PitchTable[12][MAXDETUNE];


/*---------------------------------------------------------------------
   Function: PITCH_Init

   Initializes pitch table.
---------------------------------------------------------------------*/


void PITCH_Init(void)
{
    for (int note = 0; note < 12; note++)
        for (int detune = 0; detune < MAXDETUNE; detune++)
            PitchTable[note][detune] = (uint32_t) (65536.f * powf(2.f, (note * MAXDETUNE + detune) / (12.f * MAXDETUNE)));
}


/*---------------------------------------------------------------------
   Function: PITCH_GetScale

   Returns a fixed-point value to scale number the specified amount.
---------------------------------------------------------------------*/

uint32_t PITCH_GetScale(int const pitchoffset)
{
    static bool bInitialized;

    if (!bInitialized)
    {
        PITCH_Init();
        bInitialized = true;
    }

    if (pitchoffset == 0)
        return PitchTable[0][0];

    int noteshift = pitchoffset % 1200;

    if (noteshift < 0)
        noteshift += 1200;

    int const   note   = noteshift / 100;
    int const   detune = (noteshift % 100) / (100 / MAXDETUNE);
    int const   oshift = (pitchoffset - noteshift) / 1200;
    auto const &scale  = PitchTable[note][detune];

    return (oshift < 0) ? (scale >> -oshift) : (scale << oshift);
}
