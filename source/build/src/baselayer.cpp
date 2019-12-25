#include "compat.h"
#include "osd.h"
#include "build.h"
#include "baselayer.h"

#include "renderlayer.h"

#include "a.h"
#include "polymost.h"

#include "inputstate.h"
#include "d_event.h"
#include "../../glbackend/glbackend.h"

int GUICapture = false;


// Calculate ylookup[] and call setvlinebpl()
void calc_ylookup(int32_t bpl, int32_t lastyidx)
{
    int32_t i, j=0;
    static int32_t ylookupsiz;

    Bassert(lastyidx <= MAXYDIM);

    lastyidx++;

    if (lastyidx > ylookupsiz)
    {
        ylookup.Resize(lastyidx);
        ylookupsiz = lastyidx;
    }

    for (i=0; i<=lastyidx-4; i+=4)
    {
        ylookup[i] = j;
        ylookup[i + 1] = j + bpl;
        ylookup[i + 2] = j + (bpl << 1);
        ylookup[i + 3] = j + (bpl * 3);
        j += (bpl << 2);
    }

    for (; i<lastyidx; i++)
    {
        ylookup[i] = j;
        j += bpl;
    }

    setvlinebpl(bpl);
}


int32_t g_logFlushWindow = 1;

