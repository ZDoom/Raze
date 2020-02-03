#include "ns.h"

#include "build.h"

#include "common.h"
#include "common_game.h"

BEGIN_SW_NS

void SW_InitMultiPsky(void)
{
    // default
    psky_t * const defaultsky = tileSetupSky(DEFAULTPSKY);
    defaultsky->lognumtiles = 1;
    defaultsky->horizfrac = 8192;
}

void SW_CleanupSearchPaths()
{
}

void SW_ExtInit()
{

}
END_SW_NS
