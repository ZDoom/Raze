//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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

#include "aistuff.h"
extern int localclock;

int TimeSlot[KMaxTimeSlots];


void InitTimeSlot()
{
    for (int i = 0; i < KMaxTimeSlots; i++) {
        TimeSlot[i] = 0;
    }
}

int GrabTimeSlot(int nVal)
{
    return -1;

    // BJD - below code found in an early Powerslave release. Doesn't seem to do anything and is missing in later releases.
#if 0
    int ebx = -1;
    int esi;

    for (int i = 0; i < nVal; i++)
    {
        int nSlot = (localclock + i) & 0xF;

        if (ebx >= 0)
        {
            if (esi <= TimeSlot[nSlot]) {
                continue;
            }
        }

        esi = TimeSlot[nSlot];
        ebx = i;
    }

    esi = localclock;

    int edx = ebx;

    while (edx < 16)
    {
        TimeSlot[(edx + esi) & 0xF]++;
        edx += nVal;
    }
#endif
}
