
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
