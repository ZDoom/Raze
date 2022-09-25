// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#define engine_c_

#include "gl_load.h"
#include "build.h"
#include "automap.h"

#include "imagehelpers.h"
#include "palette.h"
#include "gamecvars.h"
#include "c_console.h"
#include "v_2ddrawer.h"
#include "v_draw.h"
#include "stats.h"
#include "razemenu.h"
#include "version.h"
#include "earcut.hpp"
#include "gamestate.h"
#include "inputstate.h"
#include "printf.h"
#include "gamecontrol.h"
#include "render.h"
#include "gamefuncs.h"
#include "hw_voxels.h"
#include "coreactor.h"

#ifdef USE_OPENGL
# include "mdsprite.h"
#include "v_video.h"
#include "gl_renderer.h"
#endif

int32_t mdtims, omdtims;

uint8_t globalr = 255, globalg = 255, globalb = 255;

//
// Internal Engine Functions
//

void engineInit(void)
{
    int32_t i;

    for (i=0; i<=512; i++)
        sintable[i] = int(sin(i * BAngRadian) * +SINTABLEUNIT);
    for (i=513; i<1024; i++)
        sintable[i] = sintable[1024-i];
    for (i=1024; i<2048; i++)
        sintable[i] = -sintable[i-1024];

}


int tilehasmodelorvoxel(int const tilenume, int pal)
{
    return
        (mdinited && hw_models && tile2model[Ptile2tile(tilenume, pal)].modelid != -1) ||
        (r_voxels && tiletovox[tilenume] != -1);
}
