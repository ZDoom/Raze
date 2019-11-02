// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#include "compat.h"
#include "build.h"
#include "baselayer.h"
#include "engine_priv.h"
#include "cache1d.h"
#include "lz4.h"

#include "vfs.h"


//
// copytilepiece
//
void tileCopySection(int32_t tilenume1, int32_t sx1, int32_t sy1, int32_t xsiz, int32_t ysiz,
    int32_t tilenume2, int32_t sx2, int32_t sy2)
{
    int32_t xsiz1, ysiz1, xsiz2, ysiz2, i, j, x1, y1, x2, y2;

    xsiz1 = tilesiz[tilenume1].x; ysiz1 = tilesiz[tilenume1].y;
    xsiz2 = tilesiz[tilenume2].x; ysiz2 = tilesiz[tilenume2].y;
    if ((xsiz1 > 0) && (ysiz1 > 0) && (xsiz2 > 0) && (ysiz2 > 0))
    {
        tileLoad(tilenume1);
        if (tileData(tilenume2) == 0) return;	// Error: Destination is not writable.

        x1 = sx1;
        for (i=0; i<xsiz; i++)
        {
            y1 = sy1;
            for (j=0; j<ysiz; j++)
            {
                x2 = sx2+i;
                y2 = sy2+j;
                if ((x2 >= 0) && (y2 >= 0) && (x2 < xsiz2) && (y2 < ysiz2))
                {
					auto ptr1 = tilePtr(tilenume1) + x1 * ysiz1 + y1;
					auto ptr2 = tileData(tilenume2) + x2 * ysiz2 + y2;
                    auto dat = *ptr1;
                    if (dat != 255)
                        *ptr2 = *ptr1;
                }

                y1++; if (y1 >= ysiz1) y1 = 0;
            }
            x1++; if (x1 >= xsiz1) x1 = 0;
        }
    }
    tileInvalidate(tilenume2, -1, -1);
}

