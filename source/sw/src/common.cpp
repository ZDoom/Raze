#include "ns.h"

#include "build.h"

#ifdef _WIN32
# include "windows_inc.h"
#elif defined __APPLE__
# include "osxbits.h"
#endif

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
