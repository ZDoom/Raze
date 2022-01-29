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
#include "engine_priv.h"
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
# include "polymost.h"
#include "v_video.h"
#include "../../glbackend/glbackend.h"
#include "gl_renderer.h"
#endif

int32_t mdtims, omdtims;

float fcosglobalang, fsinglobalang;
float fydimen, fviewingrange;

uint8_t globalr = 255, globalg = 255, globalb = 255;

static int16_t radarang[1280];

// adapted from build.c
static void getclosestpointonwall_internal(vec2_t const p, int32_t const dawall, vec2_t *const closest)
{
    vec2_t const w  = wall[dawall].wall_int_pos();
    vec2_t const w2 = wall[dawall].point2Wall()->wall_int_pos();
    vec2_t const d  = { w2.X - w.X, w2.Y - w.Y };

    int64_t i = d.X * ((int64_t)p.X - w.X) + d.Y * ((int64_t)p.Y - w.Y);

    if (i <= 0)
    {
        *closest = w;
        return;
    }

    int64_t const j = (int64_t)d.X * d.X + (int64_t)d.Y * d.Y;

    if (i >= j)
    {
        *closest = w2;
        return;
    }

    i = ((i << 15) / j) << 15;

    *closest = { (int32_t)(w.X + ((d.X * i) >> 30)), (int32_t)(w.Y + ((d.Y * i) >> 30)) };
}

int32_t xdimen = -1, xdimenscale, xdimscale;
float fxdimen = -1.f;
int32_t ydimen;

int32_t globalposx, globalposy, globalposz;
fixed_t qglobalhoriz;
float fglobalposx, fglobalposy, fglobalposz;
int16_t globalang, globalcursectnum;
fixed_t qglobalang;
int32_t globalpal, globalfloorpal, cosglobalang, singlobalang;
int32_t cosviewingrangeglobalang, sinviewingrangeglobalang;

int32_t viewingrangerecip;

int32_t globalshade, globalorientation;
int16_t globalpicnum;


static int32_t globaly1, globalx2;

int16_t pointhighlight=-1, linehighlight=-1, highlightcnt=0;

static int16_t numhits;



//
// Internal Engine Functions
//

BEGIN_BLD_NS
int qanimateoffs(int a1, int a2);
END_BLD_NS

//
// animateoffs (internal)
//
int32_t animateoffs(int const tilenum, int fakevar)
{
    if (isBlood())
    {
        return Blood::qanimateoffs(tilenum, fakevar);
    }

    int const animnum = picanm[tilenum].num;

    if (animnum <= 0)
        return 0;

    int const i = (int) I_GetBuildTime() >> (picanm[tilenum].sf & PICANM_ANIMSPEED_MASK);
    int offs = 0;

    switch (picanm[tilenum].sf & PICANM_ANIMTYPE_MASK)
    {
        case PICANM_ANIMTYPE_OSC:
        {
            int k = (i % (animnum << 1));
            offs = (k < animnum) ? k : (animnum << 1) - k;
        }
        break;
        case PICANM_ANIMTYPE_FWD: offs = i % (animnum + 1); break;
        case PICANM_ANIMTYPE_BACK: offs = -(i % (animnum + 1)); break;
    }

    return offs;
}

void engineInit(void)
{
    int32_t i;

    for (i=0; i<=512; i++)
        sintable[i] = int(sin(i * BAngRadian) * +SINTABLEUNIT);
    for (i=513; i<1024; i++)
        sintable[i] = sintable[1024-i];
    for (i=1024; i<2048; i++)
        sintable[i] = -sintable[i-1024];

    for (i=0; i<640; i++)
        radarang[i] = atan((639.5 - i) / 160.) * (-64. / BAngRadian);
    for (i=0; i<640; i++)
        radarang[1279-i] = -radarang[i];
}

//
// lintersect (internal)
//
int32_t lintersect(const int32_t originX, const int32_t originY, const int32_t originZ,
                   const int32_t destX, const int32_t destY, const int32_t destZ,
                   const int32_t lineStartX, const int32_t lineStartY, const int32_t lineEndX, const int32_t lineEndY,
                   int32_t *intersectionX, int32_t *intersectionY, int32_t *intersectionZ)
{
    const vec2_t ray = { destX-originX,
                         destY-originY };
    const vec2_t lineVec = { lineEndX-lineStartX,
                             lineEndY-lineStartY };
    const vec2_t originDiff = { lineStartX-originX,
                                lineStartY-originY };

    const int32_t rayCrossLineVec = ray.X*lineVec.Y - ray.Y*lineVec.X;
    const int32_t originDiffCrossRay = originDiff.X*ray.Y - originDiff.Y*ray.X;

    if (rayCrossLineVec == 0)
    {
        if (originDiffCrossRay != 0 || enginecompatibility_mode != ENGINECOMPATIBILITY_NONE)
        {
            // line segments are parallel
            return 0;
        }

        // line segments are collinear
        const int32_t rayLengthSquared = ray.X*ray.X + ray.Y*ray.Y;
        const int32_t rayDotOriginDiff = ray.X*originDiff.X + ray.Y*originDiff.Y;
        const int32_t rayDotLineEndDiff = rayDotOriginDiff + ray.X*lineVec.X + ray.Y*lineVec.Y;
        int64_t t = min(rayDotOriginDiff, rayDotLineEndDiff);
        if (rayDotOriginDiff < 0)
        {
            if (rayDotLineEndDiff < 0)
                return 0;

            t = 0;
        }
        else if (rayDotOriginDiff > rayLengthSquared)
        {
            if (rayDotLineEndDiff > rayLengthSquared)
                return 0;

            t = rayDotLineEndDiff;
        }
        t = (t << 24) / rayLengthSquared;

        *intersectionX = originX + MulScale(ray.X, t, 24);
        *intersectionY = originY + MulScale(ray.Y, t, 24);
        *intersectionZ = originZ + MulScale(destZ-originZ, t, 24);

        return 1;
    }

    const int32_t originDiffCrossLineVec = originDiff.X*lineVec.Y - originDiff.Y*lineVec.X;
    static const int32_t signBit = 1u<<31u;
    // Any point on either line can be expressed as p+t*r and q+u*s
    // The two line segments intersect when we can find a t & u such that p+t*r = q+u*s
    // If the point is outside of the bounds of the line segment, we know we don't have an intersection.
    // t is < 0 if (originDiffCrossLineVec^rayCrossLineVec) & signBit)
    // u is < 0 if (originDiffCrossRay^rayCrossLineVec) & signBit
    // t is > 1 if abs(originDiffCrossLineVec) > abs(rayCrossLineVec)
    // u is > 1 if abs(originDiffCrossRay) > abs(rayCrossLineVec)
    // where int32_t u = tabledivide64(((int64_t) originDiffCrossRay) << 24L, rayCrossLineVec);
    if (((originDiffCrossLineVec^rayCrossLineVec) & signBit) ||
        ((originDiffCrossRay^rayCrossLineVec) & signBit) ||
        abs(originDiffCrossLineVec) > abs(rayCrossLineVec) ||
        abs(originDiffCrossRay) > abs(rayCrossLineVec))
    {
        // line segments do not overlap
        return 0;
    }

    int64_t t = (int64_t(originDiffCrossLineVec) << 24) / rayCrossLineVec;
    // For sake of completeness/readability, alternative to the above approach for an early out & avoidance of an extra division:

    *intersectionX = originX + MulScale(ray.X, t, 24);
    *intersectionY = originY + MulScale(ray.Y, t, 24);
    *intersectionZ = originZ + MulScale(destZ-originZ, t, 24);

    return 1;
}

//
// rintersect (internal)
//
// returns: -1 if didn't intersect, coefficient (x3--x4 fraction)<<16 else
int32_t rintersect(int32_t x1, int32_t y1, int32_t z1,
                   int32_t vx, int32_t vy, int32_t vz,
                   int32_t x3, int32_t y3, int32_t x4, int32_t y4,
                   int32_t *intx, int32_t *inty, int32_t *intz)
{
    //p1 towards p2 is a ray

    int64_t const x34=x3-x4, y34=y3-y4;
    int64_t const x31=x3-x1, y31=y3-y1;

    int64_t const bot  = vx*y34 - vy*x34;
    int64_t const topt = x31*y34 - y31*x34;

    if (bot == 0)
        return -1;

    int64_t const topu = vx*y31 - vy*x31;

    if (bot > 0 && (topt < 0 || topu < 0 || topu >= bot))
        return -1;
    else if (bot < 0 && (topt > 0 || topu > 0 || topu <= bot))
        return -1;

    int64_t t = (topt << 16) / bot;
    *intx = x1 + ((vx*t) >> 16);
    *inty = y1 + ((vy*t) >> 16);
    *intz = z1 + ((vz*t) >> 16);

    t = (topu << 16) / bot;

    assert((unsigned)t < 65536);

    return t;
}


int32_t getangle(int32_t xvect, int32_t yvect)
{
    int32_t rv;

    if ((xvect | yvect) == 0)
        rv = 0;
    else if (xvect == 0)
        rv = 512 + ((yvect < 0) << 10);
    else if (yvect == 0)
        rv = ((xvect < 0) << 10);
    else if (xvect == yvect)
        rv = 256 + ((xvect < 0) << 10);
    else if (xvect == -yvect)
        rv = 768 + ((xvect > 0) << 10);
    else if (abs(xvect) > abs(yvect))
        rv = ((radarang[640 + Scale(160, yvect, xvect)] >> 6) + ((xvect < 0) << 10)) & 2047;
    else rv = ((radarang[640 - Scale(160, xvect, yvect)] >> 6) + 512 + ((yvect < 0) << 10)) & 2047;

    return rv;
}


// Gets the BUILD unit height and z offset of a sprite.
// Returns the z offset, 'height' may be NULL.
int32_t spriteheightofsptr(DCoreActor* spr, int32_t *height, int32_t alsotileyofs)
{
    int32_t hei, zofs=0;
    const int32_t picnum=spr->spr.picnum, yrepeat=spr->spr.yrepeat;

    hei = (tileHeight(picnum)*yrepeat)<<2;
    if (height != NULL)
        *height = hei;

    if (spr->spr.cstat & CSTAT_SPRITE_YCENTER)
        zofs = hei>>1;

    // NOTE: a positive per-tile yoffset translates the sprite into the
    // negative world z direction (i.e. upward).
    if (alsotileyofs)
        zofs -= tileTopOffset(picnum) *yrepeat<<2;

    return zofs;
}

//
// nextsectorneighborz
//
// -1: ceiling or up
//  1: floor or down
sectortype* nextsectorneighborzptr(sectortype* sectp, int refz, int topbottom, int direction)
{
    int nextz = (direction==1) ? INT32_MAX : INT32_MIN;
    sectortype* sectortouse = nullptr;

    for(auto& wal : wallsofsector(sectp))
    {
        if (wal.twoSided())
        {
            auto ns = wal.nextSector();

            const int32_t testz = (topbottom == 1) ? ns->floorz : ns->ceilingz;

            const int32_t update = (direction == 1) ?
                (nextz > testz && testz > refz) :
                (nextz < testz && testz < refz);

            if (update)
            {
                nextz = testz;
                sectortouse = ns;
            }
        }
    }
    return sectortouse;
}


//
// cansee
//

int cansee(int x1, int y1, int z1, sectortype* sect1, int x2, int y2, int z2, sectortype* sect2)
{
    if (!sect1 || !sect2) return false;

    const int32_t x21 = x2-x1, y21 = y2-y1, z21 = z2-z1;

    if (x1 == x2 && y1 == y2)
        return (sect1 == sect2);

    BFSSectorSearch search(sect1);

    while (auto sec = search.GetNext())
    {
        const walltype* wal;
        int cnt;
        for (cnt=sec->wallnum,wal=sec->firstWall(); cnt>0; cnt--,wal++)
        {
            auto const wal2 = wal->point2Wall();
            const int32_t x31 = wal->wall_int_pos().X-x1, x34 = wal->wall_int_pos().X-wal2->wall_int_pos().X;
            const int32_t y31 = wal->wall_int_pos().Y-y1, y34 = wal->wall_int_pos().Y-wal2->wall_int_pos().Y;

            int32_t x, y, z, t, bot;
            int32_t cfz[2];

            bot = y21*x34-x21*y34; if (bot <= 0) continue;
            // XXX: OVERFLOW
            t = y21*x31-x21*y31; if ((unsigned)t >= (unsigned)bot) continue;
            t = y31*x34-x31*y34;
            if ((unsigned)t >= (unsigned)bot)
            {
                continue;
            }


            if (!wal->twoSided() || wal->cstat & CSTAT_WALL_1WAY)
                return 0;

            t = DivScale(t,bot, 24);
            x = x1 + MulScale(x21,t, 24);
            y = y1 + MulScale(y21,t, 24);
            z = z1 + MulScale(z21,t, 24);

            getzsofslopeptr(sec, x,y, &cfz[0],&cfz[1]);

            if (z <= cfz[0] || z >= cfz[1])
            {
                return 0;
            }

            auto nexts = wal->nextSector();
            getzsofslopeptr(nexts, x,y, &cfz[0],&cfz[1]);
            if (z <= cfz[0] || z >= cfz[1])
                return 0;

            search.Add(nexts);
        }

    }
    return search.Check(sect2);
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
                    result.hitpos.X = DMulScale(intx - sv.X, bcos(ange), inty - sv.Y, bsin(ange), 14);
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
                if (try_facespr_intersect(actor, sv, vx, vy, 0, &hitv, 1))
                {
                    result.hitActor = actor;
                    result.hitpos.X = DMulScale(hitv.X-sv.X, bcos(ange), hitv.Y-sv.Y, bsin(ange), 14);
                }
            }
        }
    }
}


////////// UPDATESECTOR* FAMILY OF FUNCTIONS //////////

/* Different "is inside" predicates.
 * NOTE: The redundant bound checks are expected to be optimized away in the
 * inlined code. */

/* NOTE: no bound check */
static inline int inside_z_p(int32_t const x, int32_t const y, int32_t const z, int const sectnum)
{
    int32_t cz, fz;
    getzsofslopeptr(&sector[sectnum], x, y, &cz, &fz);
    return (z >= cz && z <= fz && inside_p(x, y, sectnum));
}

int32_t getwalldist(vec2_t const in, int const wallnum)
{
    vec2_t closest;
    getclosestpointonwall_internal(in, wallnum, &closest);
    return abs(closest.X - in.X) + abs(closest.Y - in.Y);
}

int32_t getwalldist(vec2_t const in, int const wallnum, vec2_t * const out)
{
    getclosestpointonwall_internal(in, wallnum, out);
    return abs(out->X - in.X) + abs(out->Y - in.Y);
}


int32_t getsectordist(vec2_t const in, int const sectnum, vec2_t * const out /*= nullptr*/)
{
    if (inside_p(in.X, in.Y, sectnum))
    {
        if (out)
            *out = in;
        return 0;
    }

    int32_t distance = INT32_MAX;

    vec2_t     closest = {};

    for (auto& wal : wallsofsector(sectnum))
    {
        vec2_t p;
        int32_t const walldist = getwalldist(in, wallnum(&wal), &p);

        if (walldist < distance)
        {
            distance = walldist;
            closest = p;
        }
    }

    if (out)
        *out = closest;

    return distance;
}


template<class Inside>
void updatesectorneighborz(int32_t const x, int32_t const y, int32_t const z, int* const sectnum, int32_t maxDistance, Inside checker)
{
    int const initialsectnum = *sectnum;

    if ((validSectorIndex(initialsectnum)))
    {
        if (checker(x, y, z, initialsectnum))
            return;

        BFSSearch search(sector.Size(), *sectnum);

        int iter = 0;
        for (unsigned listsectnum; (listsectnum = search.GetNext()) != BFSSearch::EOL;)
        {
            if (checker(x, y, z, listsectnum))
            {
                *sectnum = listsectnum;
                return;
            }

            for (auto& wal : wallsofsector(listsectnum))
            {
                if (wal.nextsector >= 0 && (iter == 0 || getsectordist({ x, y }, wal.nextsector) <= maxDistance))
                    search.Add(wal.nextsector);
            }
            iter++;
        }
    }

    *sectnum = -1;
}

void updatesectorneighbor(int32_t const x, int32_t const y, int* const sectnum, int32_t maxDistance)
{
    updatesectorneighborz(x, y, 0, sectnum, maxDistance, inside_p0);
}


//
// updatesector[z]
//
void updatesector(int32_t const x, int32_t const y, int * const sectnum)
{
    int sect = *sectnum;

    updatesectorneighbor(x, y, &sect, MAXUPDATESECTORDIST);
    if (sect != -1)
        SET_AND_RETURN(*sectnum, sect);

    // we need to support passing in a sectnum of -1, unfortunately

    for (int i = (int)sector.Size() - 1; i >= 0; --i)
        if (inside_p(x, y, i))
            SET_AND_RETURN(*sectnum, i);

    *sectnum = -1;
}


void updatesectorz(int32_t const x, int32_t const y, int32_t const z, int* const sectnum)
{
    int sect = *sectnum;

    updatesectorneighborz(x, y, z, &sect, MAXUPDATESECTORDIST, inside_z_p);
    if (sect != -1)
        SET_AND_RETURN(*sectnum, sect);


    // we need to support passing in a sectnum of -1, unfortunately
    for (int i = (int)sector.Size() - 1; i >= 0; --i)
        if (inside_z_p(x, y, z, i))
            SET_AND_RETURN(*sectnum, i);

    *sectnum = -1;
}


//
// rotatepoint
//
void rotatepoint(vec2_t const pivot, vec2_t p, int16_t const daang, vec2_t * const p2)
{
    int const dacos = bcos(daang);
    int const dasin = bsin(daang);
    p.X -= pivot.X;
    p.Y -= pivot.Y;
    p2->X = DMulScale(p.X, dacos, -p.Y, dasin, 14) + pivot.X;
    p2->Y = DMulScale(p.Y, dacos, p.X, dasin, 14) + pivot.Y;
}

//
// setview
//
void videoSetViewableArea(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    windowxy1.X = x1;
    windowxy1.Y = y1;
    windowxy2.X = x2;
    windowxy2.Y = y2;

    xdimen = (x2-x1)+1;
    ydimen = (y2-y1)+1;

    fxdimen = (float) xdimen;
    fydimen = (float) ydimen;
    videoSetCorrectedAspect();
}



#include "v_2ddrawer.h"



//MUST USE RESTOREFORDRAWROOMS AFTER DRAWING

static int32_t setviewcnt = 0; // interface layers use this now
static int32_t bakxsiz, bakysiz;
static vec2_t bakwindowxy1, bakwindowxy2;

//
// setviewtotile
//
FCanvasTexture* renderSetTarget(int16_t tilenume)
{
    auto tex = tileGetTexture(tilenume);
    if (!tex || !tex->isHardwareCanvas()) return nullptr;
    auto canvas = static_cast<FCanvasTexture*>(tex->GetTexture());
    if (!canvas) return nullptr;
    int xsiz = tex->GetTexelWidth(), ysiz = tex->GetTexelHeight();
    if (setviewcnt > 0 || xsiz <= 0 || ysiz <= 0)
        return nullptr;

    //DRAWROOMS TO TILE BACKUP&SET CODE
    bakxsiz = xdim; bakysiz = ydim;
    bakwindowxy1 = windowxy1;
    bakwindowxy2 = windowxy2;

    setviewcnt++;

    xdim = ysiz;
    ydim = xsiz;
    videoSetViewableArea(0,0,ysiz-1,xsiz-1);
    renderSetAspect(65536,65536);
    return canvas;
}


//
// setviewback
//
void renderRestoreTarget()
{
    if (setviewcnt <= 0) return;
    setviewcnt--;

    xdim = bakxsiz;
    ydim = bakysiz;
    videoSetViewableArea(bakwindowxy1.X,bakwindowxy1.Y,
            bakwindowxy2.X,bakwindowxy2.Y);

}


int tilehasmodelorvoxel(int const tilenume, int pal)
{
    return
        (mdinited && hw_models && tile2model[Ptile2tile(tilenume, pal)].modelid != -1) ||
        (r_voxels && tiletovox[tilenume] != -1);
}


CCMD(updatesectordebug)
{
    int sect = 319;
    updatesector(1792, 24334, &sect);
    int blah = sect;
}