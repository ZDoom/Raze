
#include "build.h"

#include "common_game.h"

static const char *defaultgrpfilename = "sw.grp";
static const char *defaultdeffilename = "sw.def";

// g_grpNamePtr can ONLY point to a malloc'd block (length BMAX_PATH)
char *g_grpNamePtr = NULL;

void clearGrpNamePtr(void)
{
    Bfree(g_grpNamePtr);
    // g_grpNamePtr assumed to be assigned to right after
}

const char *G_DefaultGrpFile(void)
{
    return defaultgrpfilename;
}
const char *G_GrpFile(void)
{
    if (g_grpNamePtr == NULL)
        return G_DefaultGrpFile();
    else
        return g_grpNamePtr;
}

const char *G_DefaultDefFile(void)
{
    return defaultdeffilename;
}
const char *G_DefFile(void)
{
    if (g_defNamePtr == NULL)
        return G_DefaultDefFile();
    else
        return g_defNamePtr;
}


void SW_InitMultiPsky(void)
{
    // default
    psky_t * const defaultsky = E_DefinePsky(DEFAULTPSKY);
    defaultsky->lognumtiles = 1;
    defaultsky->horizfrac = 8192;
}
