// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#include "build.h"
#include "clip.h"
#include "engine_priv.h"
#include "printf.h"
#include "gamefuncs.h"
#include "coreactor.h"

enum { MAXCLIPDIST = 1024 };

static int clipnum;
static linetype clipit[MAXCLIPNUM];
static int32_t clipsectnum, origclipsectnum, clipspritenum;
int clipsectorlist[MAXCLIPSECTORS];
static int origclipsectorlist[MAXCLIPSECTORS];
static CollisionBase clipobjectval[MAXCLIPNUM];
static uint8_t clipignore[(MAXCLIPNUM+7)>>3];
static int32_t rxi[8], ryi[8];

BitArray clipsectormap;

int32_t quickloadboard=0;

vec2_t hitscangoal = { (1<<29)-1, (1<<29)-1 };
int32_t hitallsprites = 0;

////////// CLIPMOVE //////////
inline char bitmap_test(uint8_t const* const ptr, int const n) { return ptr[n >> 3] & (1 << (n & 7)); }


// x1, y1: in/out
// rest x/y: out
template <typename T>
static inline void get_wallspr_points(T const * const spr, int32_t *x1, int32_t *x2,
                                      int32_t *y1, int32_t *y2)
{
    //These lines get the 2 points of the rotated sprite
    //Given: (x1, y1) starts out as the center point

    const int32_t tilenum=spr->picnum, ang=spr->ang;
    const int32_t xrepeat = spr->xrepeat;
    int32_t xoff = tileLeftOffset(tilenum) + spr->xoffset;
    int32_t k, l, dax, day;

    if (spr->cstat & CSTAT_SPRITE_XFLIP)
        xoff = -xoff;

    dax = bsin(ang) * xrepeat;
    day = -bcos(ang) * xrepeat;

    l = tileWidth(tilenum);
    k = (l>>1)+xoff;

    *x1 -= MulScale(dax,k, 16);
    *x2 = *x1 + MulScale(dax,l, 16);

    *y1 -= MulScale(day,k, 16);
    *y2 = *y1 + MulScale(day,l, 16);
}

// x1, y1: in/out
// rest x/y: out
static inline void get_floorspr_points(DCoreActor *spr, int32_t px, int32_t py,
                                       int32_t *x1, int32_t *x2, int32_t *x3, int32_t *x4,
                                       int32_t *y1, int32_t *y2, int32_t *y3, int32_t *y4, int heinum = 0)
{
    const int32_t tilenum = spr->spr.picnum;
    const int32_t cosang = bcos(spr->spr.ang);
    const int32_t sinang = bsin(spr->spr.ang);

    vec2_t const span = { tileWidth(tilenum), tileHeight(tilenum)};
    vec2_t const repeat = { spr->spr.xrepeat, spr->spr.yrepeat };

    vec2_t adjofs = { tileLeftOffset(tilenum), tileTopOffset(tilenum) };
    if (heinum == 0)
    {
        adjofs.X += spr->spr.xoffset;
        adjofs.Y += spr->spr.yoffset;
    }

    int32_t const ratio = ksqrt(heinum * heinum + 4096 * 4096);

    if (spr->spr.cstat & CSTAT_SPRITE_XFLIP)
        adjofs.X = -adjofs.X;

    if (spr->spr.cstat & CSTAT_SPRITE_YFLIP)
        adjofs.Y = -adjofs.Y;

    vec2_t const center = { ((span.X >> 1) + adjofs.X) * repeat.X, ((span.Y >> 1) + adjofs.Y) * repeat.Y };
    vec2_t const rspan  = { span.X * repeat.X, span.Y * repeat.Y };
    vec2_t const ofs    = { -DivScale(MulScale(cosang, rspan.Y, 16), ratio, 12), -DivScale(MulScale(sinang, rspan.Y, 16), ratio, 12) };
    vec2_t const cossinslope = { DivScale(cosang, ratio, 12), DivScale(sinang, ratio, 12) };

    *x1 += DMulScale(sinang, center.X, cossinslope.X, center.Y, 16) - px;
    *y1 += DMulScale(cossinslope.Y, center.Y, -cosang, center.X, 16) - py;

    *x2 = *x1 - MulScale(sinang, rspan.X, 16);
    *y2 = *y1 + MulScale(cosang, rspan.X, 16);

    *x3 = *x2 + ofs.X, *x4 = *x1 + ofs.X;
    *y3 = *y2 + ofs.Y, *y4 = *y1 + ofs.Y;
}

//
// clipinsidebox
//
int clipinsidebox(vec2_t *vect, int wallnum, int walldist)
{
    int const r = walldist << 1;

    auto const wal1 = &wall[wallnum];
    auto const wal2 = wal1->point2Wall();

    vec2_t const v1 = { wal1->pos.X + walldist - vect->X, wal1->pos.Y + walldist - vect->Y };
    vec2_t       v2 = { wal2->pos.X + walldist - vect->X, wal2->pos.Y + walldist - vect->Y };

    if (((v1.X < 0) && (v2.X < 0)) || ((v1.Y < 0) && (v2.Y < 0)) || ((v1.X >= r) && (v2.X >= r)) || ((v1.Y >= r) && (v2.Y >= r)))
        return 0;

    v2.X -= v1.X; v2.Y -= v1.Y;

    if (v2.X * (walldist - v1.Y) >= v2.Y * (walldist - v1.X))  // Front
    {
        v2.X *= ((v2.X > 0) ? (0 - v1.Y) : (r - v1.Y));
        v2.Y *= ((v2.Y > 0) ? (r - v1.X) : (0 - v1.X));
        return v2.X < v2.Y;
    }

    v2.X *= ((v2.X > 0) ? (r - v1.Y) : (0 - v1.Y));
    v2.Y *= ((v2.Y > 0) ? (0 - v1.X) : (r - v1.X));
    return (v2.X >= v2.Y) << 1;
}

static int32_t spriteGetZOfSlope(DCoreActor* actor, int32_t dax, int32_t day)
{
    int16_t const heinum = spriteGetSlope(actor);
    if (heinum == 0)
        return actor->spr.pos.Z;

    int const j = DMulScale(bsin(actor->spr.ang + 1024), day - actor->spr.pos.Y, -bsin(actor->spr.ang + 512), dax - actor->spr.pos.X, 4);
    return actor->spr.pos.Z + MulScale(heinum, j, 18);
}


//
// clipinsideboxline
//
int clipinsideboxline(int x, int y, int x1, int y1, int x2, int y2, int walldist)
{
    int const r = walldist << 1;

    x1 += walldist - x;
    x2 += walldist - x;

    if (((x1 < 0) && (x2 < 0)) || ((x1 >= r) && (x2 >= r)))
        return 0;

    y1 += walldist - y;
    y2 += walldist - y;

    if (((y1 < 0) && (y2 < 0)) || ((y1 >= r) && (y2 >= r)))
        return 0;

    x2 -= x1;
    y2 -= y1;

    if (x2 * (walldist - y1) >= y2 * (walldist - x1))  // Front
    {
        x2 *= ((x2 > 0) ? (0 - y1) : (r - y1));
        y2 *= ((y2 > 0) ? (r - x1) : (0 - x1));
        return x2 < y2;
    }

    x2 *= ((x2 > 0) ? (r - y1) : (0 - y1));
    y2 *= ((y2 > 0) ? (0 - x1) : (r - x1));
    return (x2 >= y2) << 1;
}

static int32_t clipmove_warned;

static inline void addclipsect(int const sectnum)
{
    if (clipsectnum < MAXCLIPSECTORS)
    {
        clipsectormap.Set(sectnum);
        clipsectorlist[clipsectnum++] = sectnum;
    }
    else
        clipmove_warned |= 1;
}


static void addclipline(int32_t dax1, int32_t day1, int32_t dax2, int32_t day2, const CollisionBase& daoval, int nofix)
{
    if (clipnum >= MAXCLIPNUM)
    {
        clipmove_warned |= 2;
        return;
    }

    clipit[clipnum].x1 = dax1; clipit[clipnum].y1 = day1;
    clipit[clipnum].x2 = dax2; clipit[clipnum].y2 = day2;
    clipobjectval[clipnum] = daoval;

    uint32_t const mask = (1 << (clipnum&7));
    uint8_t &value = clipignore[clipnum>>3];
    value = (value & ~mask) | (-nofix & mask);

    clipnum++;
}

inline void clipmove_tweak_pos(const vec3_t *pos, int32_t gx, int32_t gy, int32_t x1, int32_t y1, int32_t x2,
                                      int32_t y2, int32_t *daxptr, int32_t *dayptr)
{
    int32_t daz;

    if (enginecompatibility_mode == ENGINECOMPATIBILITY_19950829 ||
        rintersect(pos->X, pos->Y, 0, gx, gy, 0, x1, y1, x2, y2, daxptr, dayptr, &daz) == -1)
    {
        *daxptr = pos->X;
        *dayptr = pos->Y;
    }
}

// Returns: should clip?
static int cliptestsector(int const dasect, int const nextsect, int32_t const flordist, int32_t const ceildist, vec2_t const pos, int32_t const posz)
{
	assert(validSectorIndex(dasect) && validSectorIndex(nextsect));

    auto const sec2 = &sector[nextsect];

    switch (enginecompatibility_mode)
    {
    case ENGINECOMPATIBILITY_NONE:
        break;
    default:
    {
        int32_t daz = getflorzofslopeptr(&sector[dasect], pos.X, pos.Y);
        int32_t daz2 = getflorzofslopeptr(sec2, pos.X, pos.Y);

        if (daz2 < daz-(1<<8) && (sec2->floorstat & CSTAT_SECTOR_SKY) == 0)
            if (posz >= daz2-(flordist-1)) return 1;
        daz = getceilzofslopeptr(&sector[dasect], pos.X, pos.Y);
        daz2 = getceilzofslopeptr(sec2, pos.X, pos.Y);
        if (daz2 > daz+(1<<8) && (sec2->ceilingstat & CSTAT_SECTOR_SKY) == 0)
            if (posz <= daz2+(ceildist-1)) return 1;

        return 0;
    }
    }

    int32_t daz2  = sec2->floorz;
    int32_t dacz2 = sec2->ceilingz;

    if ((sec2->floorstat|sec2->ceilingstat) & CSTAT_SECTOR_SLOPE)
        getcorrectzsofslope(nextsect, pos.X, pos.Y, &dacz2, &daz2);

    if (daz2 <= dacz2)
        return 1;

    auto const sec = &sector[dasect];

    int32_t daz  = sec->floorz;
    int32_t dacz = sec->ceilingz;

    if ((sec->floorstat|sec->ceilingstat) & CSTAT_SECTOR_SLOPE)
        getcorrectzsofslope(dasect, pos.X, pos.Y, &dacz, &daz);

    int32_t const sec2height = abs(daz2-dacz2);

    return ((abs(daz-dacz) > sec2height &&       // clip if the current sector is taller and the next is too small
            sec2height < (ceildist+(CLIPCURBHEIGHT<<1))) ||

            ((sec2->floorstat & CSTAT_SECTOR_SKY) == 0 &&    // parallaxed floor curbs don't clip
            posz >= daz2-(flordist-1) &&    // also account for desired z distance tolerance
            daz2 < daz-CLIPCURBHEIGHT) ||   // curbs less tall than 256 z units don't clip

            ((sec2->ceilingstat & CSTAT_SECTOR_SKY) == 0 && 
            posz <= dacz2+(ceildist-1) &&
            dacz2 > dacz+CLIPCURBHEIGHT));  // ceilings check the same conditions ^^^^^
}

//
// raytrace (internal)
//
static inline int32_t cliptrace(vec2_t const pos, vec2_t * const goal)
{
    int32_t hitwall = -1;

    for (int z=clipnum-1; z>=0; z--)
    {
        vec2_t const p1   = { clipit[z].x1, clipit[z].y1 };
        vec2_t const p2   = { clipit[z].x2, clipit[z].y2 };
        vec2_t const area = { p2.X-p1.X, p2.Y-p1.Y };

        int32_t topu = area.X*(pos.Y-p1.Y) - (pos.X-p1.X)*area.Y;

        if (topu <= 0 || area.X*(goal->Y-p1.Y) > (goal->X-p1.X)*area.Y)
            continue;

        vec2_t const diff = { goal->X-pos.X, goal->Y-pos.Y };

        if (diff.X*(p1.Y-pos.Y) > (p1.X-pos.X)*diff.Y || diff.X*(p2.Y-pos.Y) <= (p2.X-pos.X)*diff.Y)
            continue;

        int32_t const bot = diff.X*area.Y - area.X*diff.Y;
        int cnt = 256;

        if (!bot)
            continue;

        vec2_t n;

        do
        {
            if (--cnt < 0)
            {
                *goal = pos;
                return z;
            }

            n = { pos.X+Scale(diff.X, topu, bot), pos.Y+Scale(diff.Y, topu, bot) };
            topu--;
        } while (area.X*(n.Y-p1.Y) <= (n.X-p1.X)*area.Y);

        if (abs(pos.X-n.X)+abs(pos.Y-n.Y) < abs(pos.X-goal->X)+abs(pos.Y-goal->Y))
        {
            *goal = n;
            hitwall = z;
        }
    }

    return hitwall;
}

//
// keepaway (internal)
//
static inline void keepaway(int32_t *x, int32_t *y, int32_t w)
{
    const int32_t x1 = clipit[w].x1, dx = clipit[w].x2-x1;
    const int32_t y1 = clipit[w].y1, dy = clipit[w].y2-y1;
    const int32_t ox = Sgn(-dy), oy = Sgn(dx);
    char first = (abs(dx) <= abs(dy));

    do
    {
        if (dx*(*y-y1) > (*x-x1)*dy)
            return;

        if (first == 0)
            *x += ox;
        else
            *y += oy;

        first ^= 1;
    }
    while (1);
}

static int get_floorspr_clipyou(vec2_t const v1, vec2_t const v2, vec2_t const v3, vec2_t const v4)
{
    int clipyou = 0;

    if ((v1.Y^v2.Y) < 0)
    {
        if ((v1.X^v2.X) < 0) clipyou ^= (v1.X*v2.Y < v2.X*v1.Y)^(v1.Y<v2.Y);
        else if (v1.X >= 0) clipyou ^= 1;
    }
    if ((v2.Y^v3.Y) < 0)
    {
        if ((v2.X^v3.X) < 0) clipyou ^= (v2.X*v3.Y < v3.X*v2.Y)^(v2.Y<v3.Y);
        else if (v2.X >= 0) clipyou ^= 1;
    }
    if ((v3.Y^v4.Y) < 0)
    {
        if ((v3.X^v4.X) < 0) clipyou ^= (v3.X*v4.Y < v4.X*v3.Y)^(v3.Y<v4.Y);
        else if (v3.X >= 0) clipyou ^= 1;
    }
    if ((v4.Y^v1.Y) < 0)
    {
        if ((v4.X^v1.X) < 0) clipyou ^= (v4.X*v1.Y < v1.X*v4.Y)^(v4.Y<v1.Y);
        else if (v4.X >= 0) clipyou ^= 1;
    }

    return clipyou;
}

static void clipupdatesector(vec2_t const pos, int * const sectnum, int walldist)
{
#if 0
    if (enginecompatibility_mode != ENGINECOMPATIBILITY_NONE)
    {
        updatesector(pos.x, pos.y, sectnum);
        return;
    }
#endif

    if (inside_p(pos.X, pos.Y, *sectnum))
        return;

    int16_t nsecs = min<int16_t>(getsectordist(pos, *sectnum), INT16_MAX);

    if (nsecs > (walldist + 8))
    {
        Printf("%s(): initial position (%d, %d) not within initial sector %d; shortest distance %d.\n", __func__, pos.X, pos.Y, *sectnum, nsecs);
        walldist = 0x7fff;
    }

    {
        BFSSearch search(sector.Size(), *sectnum);

        for (unsigned listsectnum; (listsectnum = search.GetNext()) != BFSSearch::EOL;)
        {
            if (inside_p(pos.X, pos.Y, listsectnum))
            {
                *sectnum = listsectnum;
                return;
            }

            for (auto& wal : wallsofsector(listsectnum))
            {
                if (wal.nextsector >= 0 && clipsectormap[wal.nextsector])
                    search.Add(wal.nextsector);
            }
        }
    }

    {
        BFSSearch search(sector.Size(), *sectnum);

        for (unsigned listsectnum; (listsectnum = search.GetNext()) != BFSSearch::EOL;)
        {
            if (inside_p(pos.X, pos.Y, listsectnum))
            {
                *sectnum = listsectnum;
                return;
            }
            for (auto& wal : wallsofsector(listsectnum))
            {
                if (wal.nextsector >= 0 && getwalldist(pos, wallnum(&wal)) <= (walldist + 8))
                    search.Add(wal.nextsector);
            }
        }
    }

    *sectnum = -1;
}

//
// clipmove
//
CollisionBase clipmove_(vec3_t * const pos, int * const sectnum, int32_t xvect, int32_t yvect,
                 int32_t const walldist, int32_t const ceildist, int32_t const flordist, uint32_t const cliptype, int clipmoveboxtracenum)
{
    CollisionBase b{};
    if ((xvect|yvect) == 0 || *sectnum < 0)
        return b;

    DCoreActor* curspr=NULL;  // non-NULL when handling sprite with sector-like clipping

    int const initialsectnum = *sectnum;

    int32_t const dawalclipmask = (cliptype & 65535);  // CLIPMASK0 = 0x00010001 (in desperate need of getting fixed!)
    int32_t const dasprclipmask = (cliptype >> 16);    // CLIPMASK1 = 0x01000040

    vec2_t const move = { xvect, yvect };
    vec2_t       goal = { pos->X + (xvect >> 14), pos->Y + (yvect >> 14) };
    vec2_t const cent = { (pos->X + goal.X) >> 1, (pos->Y + goal.Y) >> 1 };

    //Extra walldist for sprites on sector lines
    vec2_t const  diff    = { goal.X - (pos->X), goal.Y - (pos->Y) };
    int32_t const rad     = ksqrt(compat_maybe_truncate_to_int32(uhypsq(diff.X, diff.Y))) + MAXCLIPDIST + walldist + 8;
    vec2_t const  clipMin = { cent.X - rad, cent.Y - rad };
    vec2_t const  clipMax = { cent.X + rad, cent.Y + rad };

    int clipsectcnt   = 0;
    int clipspritecnt = 0;

    clipsectorlist[0] = *sectnum;

    clipsectnum   = 1;
    clipnum       = 0;
    clipspritenum = 0;

    clipmove_warned = 0;

    clipsectormap.Zero();
    clipsectormap.Set(*sectnum);

    do
    {
        int const dasect = clipsectorlist[clipsectcnt++];
        //if (curspr)
        //    Printf("sprite %d/%d: sect %d/%d (%d)\n", clipspritecnt,clipspritenum, clipsectcnt,clipsectnum,dasect);

        ////////// Walls //////////

        auto const sec       = &sector[dasect];
        int const  startwall = sec->wallptr;
        int const  endwall   = startwall + sec->wallnum;
        auto       wal       = &wall[startwall];

        for (int j=startwall; j<endwall; j++, wal++)
        {
            auto const wal2 = wal->point2Wall();

            if ((wal->pos.X < clipMin.X && wal2->pos.X < clipMin.X) || (wal->pos.X > clipMax.X && wal2->pos.X > clipMax.X) ||
                (wal->pos.Y < clipMin.Y && wal2->pos.Y < clipMin.Y) || (wal->pos.Y > clipMax.Y && wal2->pos.Y > clipMax.Y))
                continue;

            vec2_t p1 = wal->pos;
            vec2_t p2 = wal2->pos;
            vec2_t d  = { p2.X-p1.X, p2.Y-p1.Y };

            if (d.X * (pos->Y-p1.Y) < (pos->X-p1.X) * d.Y)
                continue;  //If wall's not facing you

            vec2_t const r = { (d.Y > 0) ? clipMax.X : clipMin.X, (d.X > 0) ? clipMin.Y : clipMax.Y };
            vec2_t       v = { d.X * (r.Y - p1.Y), d.Y * (r.X - p1.X) };

            if (v.X >= v.Y)
                continue;

            int clipyou = 0;

                if (wal->nextsector < 0 || (wal->cstat & EWallFlags::FromInt(dawalclipmask)))
                {
                    clipyou = 1;
                }
                else
                {
                    clipmove_tweak_pos(pos, diff.X, diff.Y, p1.X, p1.Y, p2.X, p2.Y, &v.X, &v.Y);
                    clipyou = cliptestsector(dasect, wal->nextsector, flordist, ceildist, v, pos->Z);
                }

           // We're not interested in any sector reached by portal traversal that we're "inside" of.
            if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE && !curspr && dasect != initialsectnum
                && inside(pos->X, pos->Y, sec) == 1)
            {
                int k;
                for (k=startwall; k<endwall; k++)
                    if (wall[k].nextsector == initialsectnum)
                        break;
                if (k == endwall)
                    break;
            }

            if (clipyou)
            {
                CollisionBase objtype;
                if (curspr) objtype.setSprite(curspr);
                else objtype.setWall(j);

                //Add 2 boxes at endpoints
                int32_t bsz = walldist; if (diff.X < 0) bsz = -bsz;
                addclipline(p1.X-bsz, p1.Y-bsz, p1.X-bsz, p1.Y+bsz, objtype, false);
                addclipline(p2.X-bsz, p2.Y-bsz, p2.X-bsz, p2.Y+bsz, objtype, false);
                bsz = walldist; if (diff.Y < 0) bsz = -bsz;
                addclipline(p1.X+bsz, p1.Y-bsz, p1.X-bsz, p1.Y-bsz, objtype, false);
                addclipline(p2.X+bsz, p2.Y-bsz, p2.X-bsz, p2.Y-bsz, objtype, false);

                v.X = walldist; if (d.Y > 0) v.X = -v.X;
                v.Y = walldist; if (d.X < 0) v.Y = -v.Y;

                if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE && d.X * (pos->Y-p1.Y-v.Y) < (pos->X-p1.X-v.X) * d.Y)
                    v.X >>= 1, v.Y >>= 1;

                addclipline(p1.X+v.X, p1.Y+v.Y, p2.X+v.X, p2.Y+v.Y, objtype, false);
            }
            else if (wal->nextsector>=0)
            {
                if (!clipsectormap[wal->nextsector])
                    addclipsect(wal->nextsector);
            }
        }

        if (clipmove_warned & 1)
            Printf("clipsectnum >= MAXCLIPSECTORS!\n");

        if (clipmove_warned & 2)
            Printf("clipnum >= MAXCLIPNUM!\n");

        ////////// Sprites //////////

        if (dasprclipmask==0)
            continue;

        TSectIterator<DCoreActor> it(dasect);
        while (auto actor = it.Next())
        {
            auto const spr = &actor->spr;
            const int32_t cstat = spr->cstat;

            if (spr->cstat2 & CSTAT2_SPRITE_NOFIND) continue;
            if ((cstat&dasprclipmask) == 0)
                continue;

            auto p1 = spr->pos.vec2;

            CollisionBase obj;
            obj.setSprite(actor);

            switch (cstat & (CSTAT_SPRITE_ALIGNMENT_MASK))
            {
            case CSTAT_SPRITE_ALIGNMENT_FACING:
                if (p1.X >= clipMin.X && p1.X <= clipMax.X && p1.Y >= clipMin.Y && p1.Y <= clipMax.Y)
                {
                    int32_t height, daz = spr->pos.Z+spriteheightofsptr(actor, &height, 1);

                    if (pos->Z > daz-height-flordist && pos->Z < daz+ceildist)
                    {
                        int32_t bsz = (spr->clipdist << 2)+walldist;
                        if (diff.X < 0) bsz = -bsz;
                        addclipline(p1.X-bsz, p1.Y-bsz, p1.X-bsz, p1.Y+bsz, obj, false);
                        bsz = (spr->clipdist << 2)+walldist;
                        if (diff.Y < 0) bsz = -bsz;
                        addclipline(p1.X+bsz, p1.Y-bsz, p1.X-bsz, p1.Y-bsz, obj, false);
                    }
                }
                break;

            case CSTAT_SPRITE_ALIGNMENT_WALL:
            {
                int32_t height, daz = spr->pos.Z+spriteheightofsptr(actor, &height, 1);

                if (pos->Z > daz-height-flordist && pos->Z < daz+ceildist)
                {
                    vec2_t p2;

                    get_wallspr_points(spr, &p1.X, &p2.X, &p1.Y, &p2.Y);

                    if (clipinsideboxline(cent.X, cent.Y, p1.X, p1.Y, p2.X, p2.Y, rad) != 0)
                    {
                        vec2_t v = { MulScale(bcos(spr->ang + 256), walldist, 14),
                                     MulScale(bsin(spr->ang + 256), walldist, 14) };

                        if ((p1.X-pos->X) * (p2.Y-pos->Y) >= (p2.X-pos->X) * (p1.Y-pos->Y))  // Front
                            addclipline(p1.X+v.X, p1.Y+v.Y, p2.X+v.Y, p2.Y-v.X, obj, false);
                        else
                        {
                            if ((cstat & CSTAT_SPRITE_ONE_SIDE) != 0)
                                continue;
                            addclipline(p2.X-v.X, p2.Y-v.Y, p1.X-v.Y, p1.Y+v.X, obj, false);
                        }

                        //Side blocker
                        if ((p2.X-p1.X) * (pos->X-p1.X)+(p2.Y-p1.Y) * (pos->Y-p1.Y) < 0)
                            addclipline(p1.X-v.Y, p1.Y+v.X, p1.X+v.X, p1.Y+v.Y, obj, true);
                        else if ((p1.X-p2.X) * (pos->X-p2.X)+(p1.Y-p2.Y) * (pos->Y-p2.Y) < 0)
                            addclipline(p2.X+v.Y, p2.Y-v.X, p2.X-v.X, p2.Y-v.Y, obj, true);
                    }
                }
                break;
            }

            case CSTAT_SPRITE_ALIGNMENT_FLOOR:
            case CSTAT_SPRITE_ALIGNMENT_SLOPE:
            {
                int heinum, sz;

                if ((cstat & (CSTAT_SPRITE_ALIGNMENT_MASK)) == CSTAT_SPRITE_ALIGNMENT_SLOPE)
                {
                    heinum = spriteGetSlope(actor);
                    sz = spriteGetZOfSlope(actor, pos->X, pos->Y);
                }
                else
                {
                    heinum = 0;
                    sz = spr->pos.Z;
                }

                if (pos->Z > sz - flordist && pos->Z < sz + ceildist)
                {
                    if ((cstat & CSTAT_SPRITE_ONE_SIDE) != 0)
                        if ((pos->Z > sz) == ((cstat & CSTAT_SPRITE_YFLIP)==0))
                            continue;

                    rxi[0] = p1.X;
                    ryi[0] = p1.Y;

                    get_floorspr_points(actor, 0, 0, &rxi[0], &rxi[1], &rxi[2], &rxi[3],
                        &ryi[0], &ryi[1], &ryi[2], &ryi[3], heinum);

                    vec2_t v = { MulScale(bcos(spr->ang - 256), walldist, 14),
                                 MulScale(bsin(spr->ang - 256), walldist, 14) };

                    if ((rxi[0]-pos->X) * (ryi[1]-pos->Y) < (rxi[1]-pos->X) * (ryi[0]-pos->Y))
                    {
                        if (clipinsideboxline(cent.X, cent.Y, rxi[1], ryi[1], rxi[0], ryi[0], rad) != 0)
                            addclipline(rxi[1]-v.Y, ryi[1]+v.X, rxi[0]+v.X, ryi[0]+v.Y, obj, false);
                    }
                    else if ((rxi[2]-pos->X) * (ryi[3]-pos->Y) < (rxi[3]-pos->X) * (ryi[2]-pos->Y))
                    {
                        if (clipinsideboxline(cent.X, cent.Y, rxi[3], ryi[3], rxi[2], ryi[2], rad) != 0)
                            addclipline(rxi[3]+v.Y, ryi[3]-v.X, rxi[2]-v.X, ryi[2]-v.Y, obj, false);
                    }

                    if ((rxi[1]-pos->X) * (ryi[2]-pos->Y) < (rxi[2]-pos->X) * (ryi[1]-pos->Y))
                    {
                        if (clipinsideboxline(cent.X, cent.Y, rxi[2], ryi[2], rxi[1], ryi[1], rad) != 0)
                            addclipline(rxi[2]-v.X, ryi[2]-v.Y, rxi[1]-v.Y, ryi[1]+v.X, obj, false);
                    }
                    else if ((rxi[3]-pos->X) * (ryi[0]-pos->Y) < (rxi[0]-pos->X) * (ryi[3]-pos->Y))
                    {
                        if (clipinsideboxline(cent.X, cent.Y, rxi[0], ryi[0], rxi[3], ryi[3], rad) != 0)
                            addclipline(rxi[0]+v.X, ryi[0]+v.Y, rxi[3]+v.Y, ryi[3]-v.X, obj, false);
                    }
                }

                if (heinum == 0)
                    continue;

                // the rest is for slope sprites only.
                const int32_t tilenum = spr->picnum;
                const int32_t cosang = bcos(spr->ang);
                const int32_t sinang = bsin(spr->ang);
                vec2_t const span = { tileWidth(tilenum), tileHeight(tilenum) };
                vec2_t const repeat = { spr->xrepeat, spr->yrepeat };
                vec2_t adjofs = { tileLeftOffset(tilenum), tileTopOffset(tilenum) };

                if (spr->cstat & CSTAT_SPRITE_XFLIP)
                    adjofs.X = -adjofs.X;

                if (spr->cstat & CSTAT_SPRITE_YFLIP)
                    adjofs.Y = -adjofs.Y;

                int32_t const centerx = ((span.X >> 1) + adjofs.X) * repeat.X;
                int32_t const centery = ((span.Y >> 1) + adjofs.Y) * repeat.Y;
                int32_t const rspanx = span.X * repeat.X;
                int32_t const rspany = span.Y * repeat.Y;
                int32_t const ratio = ksqrt(heinum * heinum + 4096 * 4096);
                int32_t zz[3] = { pos->Z, pos->Z + flordist, pos->Z - ceildist };
                for (int k = 0; k < 3; k++)
                {
                    int32_t jj = DivScale(spr->pos.Z - zz[k], heinum, 18);
                    int32_t jj2 = MulScale(jj, ratio, 12);
                    if (jj2 > (centery << 8) || jj2 < ((centery - rspany) << 8))
                        continue;
                    int32_t x1 = spr->pos.X + MulScale(sinang, centerx, 16) + MulScale(jj, cosang, 24);
                    int32_t y1 = spr->pos.Y - MulScale(cosang, centerx, 16) + MulScale(jj, sinang, 24);
                    int32_t x2 = x1 - MulScale(sinang, rspanx, 16);
                    int32_t y2 = y1 + MulScale(cosang, rspanx, 16);

                    vec2_t const v = { MulScale(bcos(spr->ang - 256), walldist, 14),
                                       MulScale(bsin(spr->ang - 256), walldist, 14) };

                    if (clipinsideboxline(cent.X, cent.Y, x1, y1, x2, y2, rad) != 0)
                    {
                        if ((x1 - pos->X) * (y2 - pos->Y) >= (x2 - pos->X) * (y1 - pos->Y))
                        {
                            addclipline(x1 + v.X, y1 + v.Y, x2 + v.Y, y2 - v.X, obj, false);
                        }
                        else
                        {
		                    if ((cstat & CSTAT_SPRITE_ONE_SIDE) != 0)
                                continue;
                            addclipline(x2 - v.X, y2 - v.Y, x1 - v.Y, y1 + v.X, obj, false);
                        }
                    }
                }
                break;
            }
            }
        }
    } while (clipsectcnt < clipsectnum || clipspritecnt < clipspritenum);

    int32_t hitwalls[4], hitwall;
    CollisionBase clipReturn{};

    int cnt = clipmoveboxtracenum;

    do
    {
        if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE && (xvect|yvect)) 
        {
            for (int i=clipnum-1;i>=0;--i)
            {
                if (!bitmap_test(clipignore, i) && clipinsideboxline(pos->X, pos->Y, clipit[i].x1, clipit[i].y1, clipit[i].x2, clipit[i].y2, walldist))
                {
                    vec2_t const vec = pos->vec2;
                    keepaway(&pos->X, &pos->Y, i);
                    if (inside_p(pos->X,pos->Y, *sectnum) != 1)
                        pos->vec2 = vec;
                    break;
                }
            }
        }

        vec2_t vec = goal;

        if ((hitwall = cliptrace(pos->vec2, &vec)) >= 0)
        {
            vec2_t const  clipr  = { clipit[hitwall].x2 - clipit[hitwall].x1, clipit[hitwall].y2 - clipit[hitwall].y1 };
            // clamp to the max value we can utilize without reworking the scaling below
            // this works around the overflow issue that affects dukedc2.map
            int32_t const templl = (int32_t)clamp<int64_t>(compat_maybe_truncate_to_int32((int64_t)clipr.X * clipr.X + (int64_t)clipr.Y * clipr.Y), INT32_MIN, INT32_MAX);

            if (templl > 0)
            {
                // I don't know if this one actually overflows or not, but I highly doubt it hurts to check
                int32_t const templl2
                = (int32_t)clamp<int64_t>(compat_maybe_truncate_to_int32((int64_t)(goal.X - vec.X) * clipr.X + (int64_t)(goal.Y - vec.Y) * clipr.Y), INT32_MIN, INT32_MAX);
                int32_t const i = (enginecompatibility_mode == ENGINECOMPATIBILITY_19950829 || (abs(templl2)>>11) < templl) ?
                    (int)DivScaleL(templl2, templl, 20) : 0;

                goal = { MulScale(clipr.X, i, 20)+vec.X, MulScale(clipr.Y, i, 20)+vec.Y };
            }

            int32_t tempint;
            if (enginecompatibility_mode == ENGINECOMPATIBILITY_19950829)
                tempint = clipr.X*(move.X>>6)+clipr.Y*(move.Y>>6);
            else
                tempint = DMulScale(clipr.X, move.X, clipr.Y, move.Y, 6);

            for (int i=cnt+1, j; i<=clipmoveboxtracenum; ++i)
            {
                j = hitwalls[i];

                int32_t tempint2;
                if (enginecompatibility_mode == ENGINECOMPATIBILITY_19950829)
                    tempint2 = (clipit[j].x2-clipit[j].x1)*(move.X>>6)+(clipit[j].y2-clipit[j].y1)*(move.Y>>6);
                else
                    tempint2 = DMulScale(clipit[j].x2-clipit[j].x1, move.X, clipit[j].y2-clipit[j].y1, move.Y, 6);

                if ((tempint ^ tempint2) < 0)
                {
                    if (enginecompatibility_mode == ENGINECOMPATIBILITY_19961112)
                        updatesector(pos->X, pos->Y, sectnum);
                    return clipReturn;
                }
            }

            keepaway(&goal.X, &goal.Y, hitwall);
            xvect = (goal.X-vec.X)<<14;
            yvect = (goal.Y-vec.Y)<<14;

            if (cnt == clipmoveboxtracenum)
                clipReturn = clipobjectval[hitwall];
            hitwalls[cnt] = hitwall;
        }

        if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE)
		{
			clipupdatesector(vec, sectnum, rad);
		}

        pos->X = vec.X;
        pos->Y = vec.Y;
        cnt--;
    } while ((xvect|yvect) != 0 && hitwall >= 0 && cnt > 0);

    if (enginecompatibility_mode != ENGINECOMPATIBILITY_NONE)
    {
        for (int j=0; j<clipsectnum; j++)
            if (inside_p(pos->X, pos->Y, clipsectorlist[j]) == 1)
            {
                *sectnum = clipsectorlist[j];
                return clipReturn;
            }

        int32_t tempint2, tempint1 = INT32_MAX;
        *sectnum = -1;
        for (int j = (int)sector.Size() - 1; j >= 0; j--)
        {
            auto sect = &sector[j];
            if (inside(pos->X, pos->Y, sect) == 1)
            {
                if (enginecompatibility_mode != ENGINECOMPATIBILITY_19950829 && (sect->ceilingstat & CSTAT_SECTOR_SLOPE))
                    tempint2 = getceilzofslopeptr(sect, pos->X, pos->Y) - pos->Z;
                else
                    tempint2 = sect->ceilingz - pos->Z;

                if (tempint2 > 0)
                {
                    if (tempint2 < tempint1)
                    {
                        *sectnum = (int16_t)j;
                        tempint1 = tempint2;
                    }
                }
                else
                {
                    if (enginecompatibility_mode != ENGINECOMPATIBILITY_19950829 && (sect->ceilingstat & CSTAT_SECTOR_SLOPE))
                        tempint2 = pos->Z - getflorzofslopeptr(sect, pos->X, pos->Y);
                    else
                        tempint2 = pos->Z - sect->floorz;

                    if (tempint2 <= 0)
                    {
                        *sectnum = (int16_t)j;
                        return clipReturn;
                    }
                    if (tempint2 < tempint1)
                    {
                        *sectnum = (int16_t)j;
                        tempint1 = tempint2;
                    }
                }
            }
        }
    }

    return clipReturn;
}


//
// pushmove
//
int pushmove_(vec3_t *const vect, int *const sectnum,
    int32_t const walldist, int32_t const ceildist, int32_t const flordist, uint32_t const cliptype, bool clear /*= true*/)
{
    int bad;

    const int32_t dawalclipmask = (cliptype&65535);
    //    const int32_t dasprclipmask = (cliptype >> 16);

    if (*sectnum < 0)
        return -1;

    int32_t k = 32;

    int dir = 1;
    do
    {
        int32_t clipsectcnt = 0;

        bad = 0;

        if (clear)
        {
            if (enginecompatibility_mode != ENGINECOMPATIBILITY_NONE && *sectnum < 0)
                return 0;
            clipsectorlist[0] = *sectnum;
            clipsectnum = 1;

            clipsectormap.Zero();
            clipsectormap.Set(*sectnum);
        }

        do
        {
            const walltype* wal;
            int32_t startwall, endwall;

            auto sec = &sector[clipsectorlist[clipsectcnt]];
            if (dir > 0)
                startwall = sec->wallptr, endwall = startwall + sec->wallnum;
            else
                endwall = sec->wallptr, startwall = endwall + sec->wallnum - 1;

            int i;

            for (i=startwall, wal=&wall[startwall]; i!=endwall; i+=dir, wal+=dir)
                if (clipinsidebox(&vect->vec2, i, walldist-4) == 1)
                {
                    int j = 0;
                    if (wal->nextsector < 0 || wal->cstat & EWallFlags::FromInt(dawalclipmask)) j = 1;
                    else
                    {
                        int32_t daz2;
                        vec2_t closest;

                        if (enginecompatibility_mode == ENGINECOMPATIBILITY_19950829)
                            closest = vect->vec2;
                        else
                        {
                            //Find closest point on wall (dax, day) to (vect->x, vect->y)
                            int32_t dax = wal->point2Wall()->pos.X-wal->pos.X;
                            int32_t day = wal->point2Wall()->pos.Y-wal->pos.Y;
                            int32_t daz = dax*((vect->X)-wal->pos.X) + day*((vect->Y)-wal->pos.Y);
                            int32_t t;
                            if (daz <= 0)
                                t = 0;
                            else
                            {
                                daz2 = dax*dax+day*day;
                                if (daz >= daz2) t = (1<<30); else t = DivScale(daz, daz2, 30);
                            }
                            dax = wal->pos.X + MulScale(dax, t, 30);
                            day = wal->pos.Y + MulScale(day, t, 30);

                            closest = { dax, day };
                        }

                        j = cliptestsector(clipsectorlist[clipsectcnt], wal->nextsector, flordist, ceildist, closest, vect->Z);
                    }

                    if (j != 0)
                    {
                        j = getangle(wal->point2Wall()->pos.X-wal->pos.X, wal->point2Wall()->pos.Y-wal->pos.Y);
                        int32_t dx = -bsin(j, -11);
                        int32_t dy = bcos(j, -11);
                        int bad2 = 16;
                        do
                        {
                            vect->X = (vect->X) + dx; vect->Y = (vect->Y) + dy;
                            bad2--; if (bad2 == 0) break;
                        } while (clipinsidebox(&vect->vec2, i, walldist-4) != 0);
                        bad = -1;
                        k--; if (k <= 0) return bad;
                        clipupdatesector(vect->vec2, sectnum, walldist);
                        if (*sectnum < 0) return -1;
                    }
                    else if (!clipsectormap[wal->nextsector])
                        addclipsect(wal->nextsector);
                }

            clipsectcnt++;
        } while (clipsectcnt < clipsectnum);
        dir = -dir;
    } while (bad != 0);

    return bad;
}

//
// getzrange
//

void getzrange(const vec3_t& pos, sectortype* sect, int32_t* ceilz, CollisionBase& ceilhit, int32_t* florz, CollisionBase& florhit, int32_t walldist, uint32_t cliptype)
{
    if (sect == nullptr)
    {
        *ceilz = INT32_MIN; ceilhit.setVoid();
        *florz = INT32_MAX; florhit.setVoid();
        return;
    }

    int32_t clipsectcnt = 0;

    int32_t clipspritecnt = 0;

    //Extra walldist for sprites on sector lines
    const int32_t extradist = walldist+MAXCLIPDIST+1;
    const int32_t xmin = pos.X-extradist, ymin = pos.Y-extradist;
    const int32_t xmax = pos.X+extradist, ymax = pos.Y+extradist;

    const int32_t dawalclipmask = (cliptype&65535);
    const int32_t dasprclipmask = (cliptype >> 16);

    vec2_t closest = pos.vec2;
    int sectnum = ::sectnum(sect);
    if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE)
        getsectordist(closest, sectnum, &closest);
    else
        getzsofslopeptr(sect,closest.X,closest.Y,ceilz,florz);
    ceilhit.setSector(sect);
    florhit.setSector(sect);

    clipsectorlist[0] = sectnum;
    clipsectnum = 1;
    clipspritenum = 0;
    clipsectormap.Zero();
    clipsectormap.Set(sectnum);

    do  //Collect sectors inside your square first
    {
        ////////// Walls //////////

        auto const startsec = &sector[clipsectorlist[clipsectcnt]];

        for(auto&wal : wallsofsector(startsec))
        {
            if (wal.twoSided())
            {
                auto nextsect = wal.nextSector();
                vec2_t const v1 = wal.pos;
                vec2_t const v2 = wal.point2Wall()->pos;

                if ((v1.X < xmin && (v2.X < xmin)) || (v1.X > xmax && v2.X > xmax) ||
                    (v1.Y < ymin && (v2.Y < ymin)) || (v1.Y > ymax && v2.Y > ymax))
                    continue;

                vec2_t const d = { v2.X-v1.X, v2.Y-v1.Y };
                if (d.X*(pos.Y-v1.Y) < (pos.X-v1.X)*d.Y) continue; //back

                vec2_t da = { (d.X > 0) ? d.X*(ymin-v1.Y) : d.X*(ymax-v1.Y),
                              (d.Y > 0) ? d.Y*(xmax-v1.X) : d.Y*(xmin-v1.X) };

                if (da.X >= da.Y)
                    continue;

                if (wal.cstat & EWallFlags::FromInt(dawalclipmask)) continue;  // XXX?

                if (((nextsect->ceilingstat & CSTAT_SECTOR_SKY) == 0) && (pos.Z <= nextsect->ceilingz+(3<<8))) continue;
                if (((nextsect->floorstat & CSTAT_SECTOR_SKY) == 0) && (pos.Z >= nextsect->floorz-(3<<8))) continue;

                int nextsectno = ::sectnum(nextsect);
                if (!clipsectormap[nextsectno])
                    addclipsect(nextsectno);

                if (((v1.X < xmin + MAXCLIPDIST) && (v2.X < xmin + MAXCLIPDIST)) ||
                    ((v1.X > xmax - MAXCLIPDIST) && (v2.X > xmax - MAXCLIPDIST)) ||
                    ((v1.Y < ymin + MAXCLIPDIST) && (v2.Y < ymin + MAXCLIPDIST)) ||
                    ((v1.Y > ymax - MAXCLIPDIST) && (v2.Y > ymax - MAXCLIPDIST)))
                    continue;

                if (d.X > 0) da.X += d.X*MAXCLIPDIST; else da.X -= d.X*MAXCLIPDIST;
                if (d.Y > 0) da.Y -= d.Y*MAXCLIPDIST; else da.Y += d.Y*MAXCLIPDIST;
                if (da.X >= da.Y)
                    continue;
                //It actually got here, through all the continue's!!!
                int32_t daz = 0, daz2 = 0;
                closest = pos.vec2;
                if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE)
                    getsectordist(closest, nextsectno, &closest);
                else
                    getzsofslopeptr(nextsect, closest.X,closest.Y, &daz,&daz2);

                {
                    if (daz > *ceilz)
                        *ceilz = daz, ceilhit.setSector(nextsect);

                    if (daz2 < *florz)
                        *florz = daz2, florhit.setSector(nextsect);
                }
            }
        }
        clipsectcnt++;
    }
    while (clipsectcnt < clipsectnum || clipspritecnt < clipspritenum);

    ////////// Sprites //////////

    if (dasprclipmask)
    for (int i=0; i<clipsectnum; i++)
    {
        if (!validSectorIndex(clipsectorlist[i])) continue;    // we got a deleted sprite in here somewhere. Skip this entry.
        TSectIterator<DCoreActor> it(clipsectorlist[i]);
        while (auto actor = it.Next())
        {
            auto spr = &actor->spr;
            const int32_t cstat = spr->cstat;
            int32_t daz = 0, daz2 = 0;

            if (spr->cstat2 & CSTAT2_SPRITE_NOFIND) continue;
            if (cstat&dasprclipmask)
            {
                int32_t clipyou = 0;

                vec2_t v1 = spr->pos.vec2;

                switch (cstat & CSTAT_SPRITE_ALIGNMENT_MASK)
                {
                    case CSTAT_SPRITE_ALIGNMENT_FACING:
                    {
                        int32_t k = walldist+(spr->clipdist<<2)+1;
                        if ((abs(v1.X-pos.X) <= k) && (abs(v1.Y-pos.Y) <= k))
                        {
                            daz = spr->pos.Z + spriteheightofsptr(actor, &k, 1);
                            daz2 = daz - k;
                            clipyou = 1;
                        }
                        break;
                    }

                    case CSTAT_SPRITE_ALIGNMENT_WALL:
                    {
                        vec2_t v2;
                        get_wallspr_points(spr, &v1.X, &v2.X, &v1.Y, &v2.Y);

                        if (clipinsideboxline(pos.X,pos.Y,v1.X,v1.Y,v2.X,v2.Y,walldist+1) != 0)
                        {
                            int32_t k;
                            daz = spr->pos.Z + spriteheightofsptr(actor, &k, 1);
                            daz2 = daz-k;
                            clipyou = 1;
                        }
                        break;
                    }

                    case CSTAT_SPRITE_ALIGNMENT_FLOOR:
                    case CSTAT_SPRITE_ALIGNMENT_SLOPE:
                    {
                        if ((cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_FLOOR) daz = spr->pos.Z; 
                        else daz = spriteGetZOfSlope(actor, pos.X, pos.Y);
                        daz2 = daz;

                        if ((cstat & CSTAT_SPRITE_ONE_SIDE) != 0 && (pos.Z > daz) == ((cstat & CSTAT_SPRITE_YFLIP)==0))
                            continue;

                        vec2_t v2, v3, v4;
                        get_floorspr_points(actor, pos.X, pos.Y, &v1.X, &v2.X, &v3.X, &v4.X,
                                            &v1.Y, &v2.Y, &v3.Y, &v4.Y, spriteGetSlope(actor));

                        vec2_t const da = { MulScale(bcos(spr->ang - 256), walldist + 4, 14),
                                            MulScale(bsin(spr->ang - 256), walldist + 4, 14) };

                        v1.X += da.X; v2.X -= da.Y; v3.X -= da.X; v4.X += da.Y;
                        v1.Y += da.Y; v2.Y += da.X; v3.Y -= da.Y; v4.Y -= da.X;

                        clipyou = get_floorspr_clipyou(v1, v2, v3, v4);
                        break;
                    }
                }

                if (clipyou != 0)
                {
                    if ((pos.Z > daz) && (daz > *ceilz))
                    {
                        *ceilz = daz;
                        ceilhit.setSprite(actor);
                    }

                    if ((pos.Z < daz2) && (daz2 < *florz))
                    {
                        *florz = daz2;
                        florhit.setSprite(actor);
                    }
                }
            }
        }
    }

}


// intp: point of currently best (closest) intersection
int32_t try_facespr_intersect(DCoreActor* spr, vec3_t const in,
                              int32_t vx, int32_t vy, int32_t vz,
                              vec3_t * const intp, int32_t strictly_smaller_than_p)
{
    vec3_t const sprpos = spr->spr.pos;

    int32_t const topt = vx * (sprpos.X - in.X) + vy * (sprpos.Y - in.Y);

    if (topt <= 0) return 0;

    int32_t const bot = vx * vx + vy * vy;

    if (!bot) return 0;

    vec3_t        newpos = { 0, 0, in.Z + Scale(vz, topt, bot) };
    int32_t       siz;
    int32_t const z1 = sprpos.Z + spriteheightofsptr(spr, &siz, 1);

    if (newpos.Z < z1 - siz || newpos.Z > z1)
        return 0;

    int32_t const topu = vx * (sprpos.Y - in.Y) - vy * (sprpos.X - in.X);
    vec2_t  const off  = { Scale(vx, topu, bot), Scale(vy, topu, bot) };
    int32_t const dist = off.X * off.X + off.Y * off.Y;

    siz = tileWidth(spr->spr.picnum) * spr->spr.xrepeat;

    if (dist > MulScale(siz, siz, 7)) return 0;

    newpos.vec2 = { in.X + Scale(vx, topt, bot), in.Y + Scale(vy, topt, bot) };

    if (abs(newpos.X - in.X) + abs(newpos.Y - in.Y) + strictly_smaller_than_p >
        abs(intp->X - in.X) + abs(intp->Y - in.Y))
        return 0;

    *intp = newpos;
    return 1;
}

static inline void hit_set(HitInfoBase *hit, sectortype* sect, walltype* wal, DCoreActor* actor, int32_t x, int32_t y, int32_t z)
{
    hit->hitSector = sect;
    hit->hitWall = wal;
    hit->hitActor = actor;
    hit->hitpos.X = x;
    hit->hitpos.Y = y;
    hit->hitpos.Z = z;
}

static int32_t hitscan_hitsectcf=-1;

// stat, heinum, z: either ceiling- or floor-
// how: -1: behave like ceiling, 1: behave like floor
static int32_t hitscan_trysector(const vec3_t *sv, sectortype* sec, HitInfoBase *hit,
                                 int32_t vx, int32_t vy, int32_t vz,
                                 uint16_t stat, int16_t heinum, int32_t z, int32_t how)
{
    int32_t x1 = INT32_MAX, y1 = 0, z1 = 0;
    int32_t i;

    if (stat&2)
    {
        auto const wal  = sec->firstWall();
        auto const wal2 = wal->point2Wall();
        int32_t j, dax=wal2->pos.X-wal->pos.X, day=wal2->pos.Y-wal->pos.Y;

        i = ksqrt(compat_maybe_truncate_to_int32(uhypsq(dax,day))); if (i == 0) return 1; //continue;
        i = DivScale(heinum,i, 15);
        dax *= i; day *= i;

        j = (vz<<8)-DMulScale(dax,vy,-day,vx, 15);
        if (j != 0)
        {
            i = ((z - sv->Z)<<8)+DMulScale(dax,sv->Y-wal->pos.Y,-day,sv->X-wal->pos.X, 15);
            if (((i^j) >= 0) && ((abs(i)>>1) < abs(j)))
            {
                i = DivScale(i,j, 30);
                x1 = sv->X + MulScale(vx,i, 30);
                y1 = sv->Y + MulScale(vy,i, 30);
                z1 = sv->Z + MulScale(vz,i, 30);
            }
        }
    }
    else if ((how*vz > 0) && (how*sv->Z <= how*z))
    {
        z1 = z; i = z1-sv->Z;
        if ((abs(i)>>1) < vz*how)
        {
            i = DivScale(i,vz, 30);
            x1 = sv->X + MulScale(vx,i, 30);
            y1 = sv->Y + MulScale(vy,i, 30);
        }
    }

    if ((x1 != INT32_MAX) && (abs(x1-sv->X)+abs(y1-sv->Y) < abs((hit->hitpos.X)-sv->X)+abs((hit->hitpos.Y)-sv->Y)))
    {
        if (inside(x1,y1,sec) == 1)
        {
            hit_set(hit, sec, nullptr, nullptr, x1, y1, z1);
            hitscan_hitsectcf = (how+1)>>1;
        }
    }

    return 0;
}


//
// hitscan
//

int hitscan(const vec3_t& start, const sectortype* startsect, const vec3_t& direction, HitInfoBase& hitinfo, unsigned cliptype)
{
    auto const sv = &start;
    int const vx = direction.X, vy = direction.Y, vz = direction.Z;
    int32_t x1, y1=0, z1=0, x2, y2, intx, inty, intz;
    int32_t i, k, daz;

    const int32_t dawalclipmask = (cliptype&65535);
    const int32_t dasprclipmask = (cliptype >> 16);

    hitinfo.clearObj(); // note that this case leaves hitpos untouched.
    if (startsect == nullptr)
        return -1;

    hitinfo.hitpos.vec2 = hitscangoal;

    BFSSectorSearch search(startsect);
    while (auto sec = search.GetNext())
    {
        i = 1;
        if (hitscan_trysector(sv, sec, &hitinfo, vx,vy,vz, sec->ceilingstat, sec->ceilingheinum, sec->ceilingz, -i))
            continue;
        if (hitscan_trysector(sv, sec, &hitinfo, vx,vy,vz, sec->floorstat, sec->floorheinum, sec->floorz, i))
            continue;

        ////////// Walls //////////

        for(auto& w : wallsofsector(sec))
        {
            auto wal  = &w;
            auto wal2 = wal->point2Wall();

            auto const  nextsect = wal->nextSector();

            x1 = wal->pos.X; y1 = wal->pos.Y; x2 = wal2->pos.X; y2 = wal2->pos.Y;

            if (compat_maybe_truncate_to_int32((coord_t)(x1-sv->X)*(y2-sv->Y))
                < compat_maybe_truncate_to_int32((coord_t)(x2-sv->X)*(y1-sv->Y))) continue;
            if (rintersect(sv->X,sv->Y,sv->Z, vx,vy,vz, x1,y1, x2,y2, &intx,&inty,&intz) == -1) continue;

            if (abs(intx-sv->X)+abs(inty-sv->Y) >= abs((hitinfo.hitpos.X)-sv->X)+abs((hitinfo.hitpos.Y)-sv->Y))
                continue;

            if ((!wal->twoSided()) || (wal->cstat & EWallFlags::FromInt(dawalclipmask)))
            {
                hit_set(&hitinfo, sec, wal, nullptr, intx, inty, intz);
                continue;
            }

            int32_t daz2;
            getzsofslopeptr(nextsect,intx,inty,&daz,&daz2);
            if (intz <= daz || intz >= daz2)
            {
                hit_set(&hitinfo, sec, wal, nullptr, intx, inty, intz);
                continue;
            }

            search.Add(nextsect);
        }

        ////////// Sprites //////////

        if (dasprclipmask==0)
            continue;

        TSectIterator<DCoreActor> it(sec);
        while (auto actor = it.Next())
        {
            auto const spr = &actor->spr;
            uint32_t const cstat = spr->cstat;

            if (spr->cstat2 & CSTAT2_SPRITE_NOFIND)
                continue;

#ifdef USE_OPENGL
            if (!hitallsprites)
#endif
                if ((cstat&dasprclipmask) == 0)
                    continue;

            x1 = spr->pos.X; y1 = spr->pos.Y; z1 = spr->pos.Z;
            switch (cstat&CSTAT_SPRITE_ALIGNMENT_MASK)
            {
            case 0:
            {
                if (try_facespr_intersect(actor, *sv, vx, vy, vz, &hitinfo.hitpos, 0))
                {
                    hitinfo.hitSector = sec;
                    hitinfo.hitWall = nullptr;
                    hitinfo.hitActor = actor;
                }

                break;
            }

            case CSTAT_SPRITE_ALIGNMENT_WALL:
            {
                int32_t ucoefup16;
                int32_t tilenum = spr->picnum;

                get_wallspr_points(spr, &x1, &x2, &y1, &y2);

                if ((cstat & CSTAT_SPRITE_ONE_SIDE) != 0)   //back side of 1-way sprite
                    if (compat_maybe_truncate_to_int32((coord_t)(x1-sv->X)*(y2-sv->Y))
                        < compat_maybe_truncate_to_int32((coord_t)(x2-sv->X)*(y1-sv->Y))) continue;

                ucoefup16 = rintersect(sv->X,sv->Y,sv->Z,vx,vy,vz,x1,y1,x2,y2,&intx,&inty,&intz);
                if (ucoefup16 == -1) continue;

                if (abs(intx-sv->X)+abs(inty-sv->Y) > abs((hitinfo.hitpos.X)-sv->X)+abs((hitinfo.hitpos.Y)-sv->Y))
                    continue;

                daz = spr->pos.Z + spriteheightofsptr(actor, &k, 1);
                if (intz > daz-k && intz < daz)
                {
                    if (picanm[tilenum].sf&PICANM_TEXHITSCAN_BIT)
                    {
                        tileUpdatePicnum(&tilenum, 0, 0);

                        if (tileLoad(tilenum))
                        {
                            // daz-intz > 0 && daz-intz < k
                            int32_t xtex = MulScale(ucoefup16, tileWidth(tilenum), 16);
                            int32_t vcoefup16 = 65536-DivScale(daz-intz, k, 16);
                            int32_t ytex = MulScale(vcoefup16, tileHeight(tilenum), 16);

                            auto texel = (tilePtr(tilenum) + tileHeight(tilenum)*xtex + ytex);
                            if (*texel == TRANSPARENT_INDEX)
                                continue;
                        }
                    }

                    hit_set(&hitinfo, sec, nullptr, actor, intx, inty, intz);
                }
                break;
            }

            case CSTAT_SPRITE_ALIGNMENT_FLOOR:
            {
                int32_t x3, y3, x4, y4;
                intz = z1;

                if (vz == 0 || ((intz-sv->Z)^vz) < 0) continue;

                if ((cstat & CSTAT_SPRITE_ONE_SIDE) != 0)
                    if ((sv->Z > intz) == ((cstat & CSTAT_SPRITE_YFLIP)==0)) continue;

                // avoid overflow errors by using 64 bit math.
                intx = int(sv->X + (int64_t(intz) - sv->Z) * vx / vz);
                inty = int(sv->Y + (int64_t(intz) - sv->Z) * vy / vz);

                if (abs(intx-sv->X)+abs(inty-sv->Y) > abs((hitinfo.hitpos.X)-sv->X)+abs((hitinfo.hitpos.Y)-sv->Y))
                    continue;

                get_floorspr_points(actor, intx, inty, &x1, &x2, &x3, &x4,
                                    &y1, &y2, &y3, &y4, spriteGetSlope(actor));

                if (get_floorspr_clipyou({x1, y1}, {x2, y2}, {x3, y3}, {x4, y4}))
                    hit_set(&hitinfo, sec, nullptr, actor, intx, inty, intz);

                break;
            }

            case CSTAT_SPRITE_ALIGNMENT_SLOPE:
            {
                int32_t x3, y3, x4, y4;
                int32_t const heinum = spriteGetSlope(actor);
                int32_t const dax = (heinum * sintable[(spr->ang + 1024) & 2047]) << 1;
                int32_t const day = (heinum * sintable[(spr->ang + 512) & 2047]) << 1;
                int32_t const j = (vz << 8) - DMulScale(dax, vy, -day, vx, 15);
                if (j == 0) continue;
                if ((cstat & 64) != 0)
                    if ((j < 0) == ((cstat & 8) == 0)) continue;
                int32_t dist2 = ((spr->pos.Z - sv->Z) << 8) + DMulScale(dax, sv->Y - spr->pos.Y, -day, sv->X - spr->pos.X, 15);
                if ((dist2 ^ j) < 0 || (abs(dist2) >> 1) >= abs(j)) continue;

                dist2 = DivScale(dist2, j, 30);
                intx = sv->X + MulScale(vx, dist2, 30);
                inty = sv->Y + MulScale(vy, dist2, 30);
                intz = sv->Z + MulScale(vz, dist2, 30);

                if (abs(intx - sv->X) + abs(inty - sv->Y) > abs((hitinfo.hitpos.X) - sv->X) + abs((hitinfo.hitpos.Y) - sv->Y))
                    continue;

                get_floorspr_points(actor, intx, inty, &x1, &x2, &x3, &x4,
                    &y1, &y2, &y3, &y4, spriteGetSlope(actor));

                if (get_floorspr_clipyou({ x1, y1 }, { x2, y2 }, { x3, y3 }, { x4, y4 }))
                    hit_set(&hitinfo, sec, nullptr, actor, intx, inty, intz);

                break;
            }

            }
        }
    }

    return 0;
}

