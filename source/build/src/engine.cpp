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

//
// neartag
//

void neartag(const vec3_t& sv, sectortype* sect, int ange, HitInfoBase& result, int neartagrange, int tagsearch)
{
    const int32_t vx = MulScale(bcos(ange), neartagrange, 14);
    const int32_t vy = MulScale(bsin(ange), neartagrange, 14);
    vec3_t hitv = { sv.X+vx, sv.Y+vy, 0 };

    result.clearObj();
    result.hitpos.X = 0;

    if (!sect || (tagsearch & 3) == 0)
        return;

    BFSSectorSearch search(sect);

    while (auto dasect = search.GetNext())
    {
        for (auto& w : wallsofsector(dasect))
        {
            auto wal = &w;
            auto const wal2 = wal->point2Wall();
            const auto nextsect = wal->nextSector();

            const int32_t x1 = wal->wall_int_pos().X, y1 = wal->wall_int_pos().Y, x2 = wal2->wall_int_pos().X, y2 = wal2->wall_int_pos().Y;
            int32_t intx, inty, intz, good = 0;

            if (wal->twoSided())
            {
                if ((tagsearch & 1) && nextsect->lotag) good |= 1;
                if ((tagsearch & 2) && nextsect->hitag) good |= 1;
            }

            if ((tagsearch & 1) && wal->lotag) good |= 2;
            if ((tagsearch & 2) && wal->hitag) good |= 2;

            if ((good == 0) && (!wal->twoSided())) continue;
            if ((coord_t)(x1 - sv.X) * (y2 - sv.Y) < (coord_t)(x2 - sv.X) * (y1 - sv.Y)) continue;

            if (lintersect(sv.X, sv.Y, sv.Z, hitv.X, hitv.Y, hitv.Z, x1, y1, x2, y2, &intx, &inty, &intz) == 1)
            {
                if (good != 0)
                {
                    if (good & 1) result.hitSector = nextsect;
                    if (good & 2) result.hitWall = wal;
                    result.hitpos.X = DMulScale(intx - sv.X, bcos(ange), inty - sv.Y, bsin(ange), 14) * inttoworld;
                    hitv.X = intx; hitv.Y = inty; hitv.Z = intz;
                }

                if (wal->twoSided())
                {
                    search.Add(nextsect);
                }
            }
        }

        if (tagsearch & 4)
            continue; // skip sprite search

        TSectIterator<DCoreActor> it(dasect);
        while (auto actor = it.Next())
        {
            if (actor->spr.cstat2 & CSTAT2_SPRITE_NOFIND)
                continue;

            if (((tagsearch&1) && actor->spr.lotag) || ((tagsearch&2) && actor->spr.hitag))
            {
                DVector3 v;
                if (intersectSprite(actor, DVector3(sv.X * inttoworld, sv.Y * inttoworld, sv.Z * zinttoworld),
                    DVector3(vx * inttoworld, vy * inttoworld, 0), v, 1 / 256.))
                {
                    vec3_t hitv(v.X * worldtoint, v.Y * worldtoint, v.Z * zworldtoint);
                    result.hitActor = actor;
                    result.hitpos.X = DMulScale(hitv.X-sv.X, bcos(ange), hitv.Y-sv.Y, bsin(ange), 14) * inttoworld;
                }
            }
        }
    }
}

int tilehasmodelorvoxel(int const tilenume, int pal)
{
    return
        (mdinited && hw_models && tile2model[Ptile2tile(tilenume, pal)].modelid != -1) ||
        (r_voxels && tiletovox[tilenume] != -1);
}
