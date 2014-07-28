
#include "compat.h"
#include "build.h"

#include "names.h"
#include "common_game.h"

const char *G_DefaultDefFile(void)
{
    return "kenbuild.def";
}
const char *G_DefFile(void)
{
    return "kenbuild.def";
}

uint8_t *basepaltable[1] = {
    palette
};

#define NUMPSKYMULTIS 2
EDUKE32_STATIC_ASSERT(NUMPSKYMULTIS <= MAXPSKYMULTIS);
EDUKE32_STATIC_ASSERT(PSKYOFF_MAX <= MAXPSKYTILES);

// Set up new-style multi-psky handling.
void Ken_InitMultiPsky(void)
{
    int32_t i;

    static int32_t inited;
    if (inited)
        return;
    inited = 1;

    multipskytile[0] = -1;
    multipskytile[1] = DAYSKY;
    multipskytile[2] = NIGHTSKY;

    pskynummultis = NUMPSKYMULTIS;

    // When adding other multi-skies, take care that the tileofs[] values are
    // <= PSKYOFF_MAX. (It can be increased up to MAXPSKYTILES, but should be
    // set as tight as possible.)

    // The default sky properties (all others are implicitly zero):
    multipsky[0].lognumtiles = 1;
    multipsky[0].horizfrac = 65536;

    // DAYSKY
    multipsky[1].lognumtiles = 1;
    multipsky[1].horizfrac = 65536;

    // DAYSKY
    multipsky[2].lognumtiles = 3;
    multipsky[2].horizfrac = 65536;

    for (i=0; i<pskynummultis; ++i)
    {
        int32_t j;
        for (j=0; j<(1<<multipsky[i].lognumtiles); ++j)
            Bassert(multipsky[i].tileofs[j] <= PSKYOFF_MAX);
    }
}
