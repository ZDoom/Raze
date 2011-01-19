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
/**********************************************************************
   module: PITCH.C

   author: James R. Dose
   date:   June 14, 1993

   Routines for pitch scaling.

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "pitch.h"

#define MAXDETUNE 50

static uint32_t PitchTable[ 12 ][ MAXDETUNE ];
static int32_t PITCH_Installed = 0;


/*---------------------------------------------------------------------
   Function: PITCH_Init

   Initializes pitch table.
---------------------------------------------------------------------*/


void PITCH_Init(void)
{
    int32_t note;
    int32_t detune;

    if (!PITCH_Installed)
    {
        for (note = 0; note < 12; note++)
        {
            for (detune = 0; detune < MAXDETUNE; detune++)
            {
                PitchTable[ note ][ detune ] = 0x10000 *
                                               pow(2, (note * MAXDETUNE + detune) / (12.0 * MAXDETUNE));
            }
        }

        PITCH_Installed = 1;
    }
}


/*---------------------------------------------------------------------
   Function: PITCH_GetScale

   Returns a fixed-point value to scale number the specified amount.
---------------------------------------------------------------------*/

uint32_t PITCH_GetScale(int32_t pitchoffset)
{
    uint32_t scale;
    int32_t octaveshift;
    int32_t noteshift;
    int32_t note;
    int32_t detune;

    if ( !PITCH_Installed )
        PITCH_Init();

    if (pitchoffset == 0)
        return(PitchTable[ 0 ][ 0 ]);

    noteshift = pitchoffset % 1200;
    if (noteshift < 0)
        noteshift += 1200;

    note   = noteshift / 100;
    detune = (noteshift % 100) / (100 / MAXDETUNE);
    octaveshift = (pitchoffset - noteshift) / 1200;

    if (detune < 0)
    {
        detune += (100 / MAXDETUNE);
        note--;
        if (note < 0)
        {
            note += 12;
            octaveshift--;
        }
    }

    scale = PitchTable[ note ][ detune ];

    return (octaveshift < 0) ? (scale >> -octaveshift) : (scale <<= octaveshift);
}

