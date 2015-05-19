
#include "build.h"

#include "common_game.h"

#define NUMPSKYMULTIS 1
EDUKE32_STATIC_ASSERT(NUMPSKYMULTIS <= MAXPSKYMULTIS);
EDUKE32_STATIC_ASSERT(PSKYOFF_MAX <= MAXPSKYTILES);

// Set up new-style multi-psky handling.
void SW_InitMultiPsky(void)
{
    int32_t i;

    static int32_t inited;
    if (inited)
        return;
    inited = 1;

    multipskytile[0] = -1;

    pskynummultis = NUMPSKYMULTIS;

    // When adding other multi-skies, take care that the tileofs[] values are
    // <= PSKYOFF_MAX. (It can be increased up to MAXPSKYTILES, but should be
    // set as tight as possible.)

    // The default sky properties (all others are implicitly zero):
    multipsky[0].lognumtiles = 1;
    multipsky[0].horizfrac = 8192;

    for (i=0; i<pskynummultis; ++i)
    {
        int32_t j;
        for (j=0; j<(1<<multipsky[i].lognumtiles); ++j)
            Bassert(multipsky[i].tileofs[j] <= PSKYOFF_MAX);
    }
}
