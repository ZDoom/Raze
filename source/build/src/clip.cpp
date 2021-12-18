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
template <typename T>
static inline void get_floorspr_points(T const * const spr, int32_t px, int32_t py,
                                       int32_t *x1, int32_t *x2, int32_t *x3, int32_t *x4,
                                       int32_t *y1, int32_t *y2, int32_t *y3, int32_t *y4)
{
    const int32_t tilenum = spr->picnum;
    const int32_t cosang = bcos(spr->ang);
    const int32_t sinang = bsin(spr->ang);

    vec2_t const span = { tileWidth(tilenum), tileHeight(tilenum)};
    vec2_t const repeat = { spr->xrepeat, spr->yrepeat };

    vec2_t adjofs = { tileLeftOffset(tilenum) + spr->xoffset, tileTopOffset(tilenum) + spr->yoffset };

    if (spr->cstat & CSTAT_SPRITE_XFLIP)
        adjofs.x = -adjofs.x;

    if (spr->cstat & CSTAT_SPRITE_YFLIP)
        adjofs.y = -adjofs.y;

    vec2_t const center = { ((span.x >> 1) + adjofs.x) * repeat.x, ((span.y >> 1) + adjofs.y) * repeat.y };
    vec2_t const rspan  = { span.x * repeat.x, span.y * repeat.y };
    vec2_t const ofs    = { -MulScale(cosang, rspan.y, 16), -MulScale(sinang, rspan.y, 16) };

    *x1 += DMulScale(sinang, center.x, cosang, center.y, 16) - px;
    *y1 += DMulScale(sinang, center.y, -cosang, center.x, 16) - py;

    *x2 = *x1 - MulScale(sinang, rspan.x, 16);
    *y2 = *y1 + MulScale(cosang, rspan.x, 16);

    *x3 = *x2 + ofs.x, *x4 = *x1 + ofs.x;
    *y3 = *y2 + ofs.y, *y4 = *y1 + ofs.y;
}

//
// clipinsidebox
//
int clipinsidebox(vec2_t *vect, int wallnum, int walldist)
{
    int const r = walldist << 1;

    auto const wal1 = (uwallptr_t)&wall[wallnum];
    auto const wal2 = (uwallptr_t)wal1->point2Wall();

    vec2_t const v1 = { wal1->x + walldist - vect->x, wal1->y + walldist - vect->y };
    vec2_t       v2 = { wal2->x + walldist - vect->x, wal2->y + walldist - vect->y };

    if (((v1.x < 0) && (v2.x < 0)) || ((v1.y < 0) && (v2.y < 0)) || ((v1.x >= r) && (v2.x >= r)) || ((v1.y >= r) && (v2.y >= r)))
        return 0;

    v2.x -= v1.x; v2.y -= v1.y;

    if (v2.x * (walldist - v1.y) >= v2.y * (walldist - v1.x))  // Front
    {
        v2.x *= ((v2.x > 0) ? (0 - v1.y) : (r - v1.y));
        v2.y *= ((v2.y > 0) ? (r - v1.x) : (0 - v1.x));
        return v2.x < v2.y;
    }

    v2.x *= ((v2.x > 0) ? (r - v1.y) : (0 - v1.y));
    v2.y *= ((v2.y > 0) ? (0 - v1.x) : (r - v1.x));
    return (v2.x >= v2.y) << 1;
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
        rintersect(pos->x, pos->y, 0, gx, gy, 0, x1, y1, x2, y2, daxptr, dayptr, &daz) == -1)
    {
        *daxptr = pos->x;
        *dayptr = pos->y;
    }
}

// Returns: should clip?
static int cliptestsector(int const dasect, int const nextsect, int32_t const flordist, int32_t const ceildist, vec2_t const pos, int32_t const posz)
{
	assert(validSectorIndex(dasect) && validSectorIndex(nextsect));

    auto const sec2 = (usectorptr_t)&sector[nextsect];

    switch (enginecompatibility_mode)
    {
    case ENGINECOMPATIBILITY_NONE:
        break;
    default:
    {
        int32_t daz = getflorzofslopeptr(&sector[dasect], pos.x, pos.y);
        int32_t daz2 = getflorzofslopeptr(sec2, pos.x, pos.y);

        if (daz2 < daz-(1<<8) && (sec2->floorstat & CSTAT_SECTOR_SKY) == 0)
            if (posz >= daz2-(flordist-1)) return 1;
        daz = getceilzofslopeptr(&sector[dasect], pos.x, pos.y);
        daz2 = getceilzofslopeptr(sec2, pos.x, pos.y);
        if (daz2 > daz+(1<<8) && (sec2->ceilingstat & CSTAT_SECTOR_SKY) == 0)
            if (posz <= daz2+(ceildist-1)) return 1;

        return 0;
    }
    }

    int32_t daz2  = sec2->floorz;
    int32_t dacz2 = sec2->ceilingz;

    if ((sec2->floorstat|sec2->ceilingstat) & CSTAT_SECTOR_SLOPE)
        getcorrectzsofslope(nextsect, pos.x, pos.y, &dacz2, &daz2);

    if (daz2 <= dacz2)
        return 1;

    auto const sec = (usectorptr_t)&sector[dasect];

    int32_t daz  = sec->floorz;
    int32_t dacz = sec->ceilingz;

    if ((sec->floorstat|sec->ceilingstat) & CSTAT_SECTOR_SLOPE)
        getcorrectzsofslope(dasect, pos.x, pos.y, &dacz, &daz);

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
        vec2_t const area = { p2.x-p1.x, p2.y-p1.y };

        int32_t topu = area.x*(pos.y-p1.y) - (pos.x-p1.x)*area.y;

        if (topu <= 0 || area.x*(goal->y-p1.y) > (goal->x-p1.x)*area.y)
            continue;

        vec2_t const diff = { goal->x-pos.x, goal->y-pos.y };

        if (diff.x*(p1.y-pos.y) > (p1.x-pos.x)*diff.y || diff.x*(p2.y-pos.y) <= (p2.x-pos.x)*diff.y)
            continue;

        int32_t const bot = diff.x*area.y - area.x*diff.y;
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

            n = { pos.x+Scale(diff.x, topu, bot), pos.y+Scale(diff.y, topu, bot) };
            topu--;
        } while (area.x*(n.y-p1.y) <= (n.x-p1.x)*area.y);

        if (abs(pos.x-n.x)+abs(pos.y-n.y) < abs(pos.x-goal->x)+abs(pos.y-goal->y))
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

    if ((v1.y^v2.y) < 0)
    {
        if ((v1.x^v2.x) < 0) clipyou ^= (v1.x*v2.y < v2.x*v1.y)^(v1.y<v2.y);
        else if (v1.x >= 0) clipyou ^= 1;
    }
    if ((v2.y^v3.y) < 0)
    {
        if ((v2.x^v3.x) < 0) clipyou ^= (v2.x*v3.y < v3.x*v2.y)^(v2.y<v3.y);
        else if (v2.x >= 0) clipyou ^= 1;
    }
    if ((v3.y^v4.y) < 0)
    {
        if ((v3.x^v4.x) < 0) clipyou ^= (v3.x*v4.y < v4.x*v3.y)^(v3.y<v4.y);
        else if (v3.x >= 0) clipyou ^= 1;
    }
    if ((v4.y^v1.y) < 0)
    {
        if ((v4.x^v1.x) < 0) clipyou ^= (v4.x*v1.y < v1.x*v4.y)^(v4.y<v1.y);
        else if (v4.x >= 0) clipyou ^= 1;
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

    if (inside_p(pos.x, pos.y, *sectnum))
        return;

    int16_t nsecs = min<int16_t>(getsectordist(pos, *sectnum), INT16_MAX);

    if (nsecs > (walldist + 8))
    {
        Printf("%s(): initial position (%d, %d) not within initial sector %d; shortest distance %d.\n", __func__, pos.x, pos.y, *sectnum, nsecs);
        walldist = 0x7fff;
    }

    {
        BFSSearch search(numsectors, *sectnum);

        for (unsigned listsectnum; (listsectnum = search.GetNext()) != BFSSearch::EOL;)
        {
            if (inside_p(pos.x, pos.y, listsectnum))
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
        BFSSearch search(numsectors, *sectnum);

        for (unsigned listsectnum; (listsectnum = search.GetNext()) != BFSSearch::EOL;)
        {
            if (inside_p(pos.x, pos.y, listsectnum))
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
    vec2_t       goal = { pos->x + (xvect >> 14), pos->y + (yvect >> 14) };
    vec2_t const cent = { (pos->x + goal.x) >> 1, (pos->y + goal.y) >> 1 };

    //Extra walldist for sprites on sector lines
    vec2_t const  diff    = { goal.x - (pos->x), goal.y - (pos->y) };
    int32_t const rad     = ksqrt(compat_maybe_truncate_to_int32(uhypsq(diff.x, diff.y))) + MAXCLIPDIST + walldist + 8;
    vec2_t const  clipMin = { cent.x - rad, cent.y - rad };
    vec2_t const  clipMax = { cent.x + rad, cent.y + rad };

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

        auto const sec       = (usectorptr_t)&sector[dasect];
        int const  startwall = sec->wallptr;
        int const  endwall   = startwall + sec->wallnum;
        auto       wal       = (uwallptr_t)&wall[startwall];

        for (int j=startwall; j<endwall; j++, wal++)
        {
            auto const wal2 = (uwallptr_t)wal->point2Wall();

            if ((wal->x < clipMin.x && wal2->x < clipMin.x) || (wal->x > clipMax.x && wal2->x > clipMax.x) ||
                (wal->y < clipMin.y && wal2->y < clipMin.y) || (wal->y > clipMax.y && wal2->y > clipMax.y))
                continue;

            vec2_t p1 = wal->pos;
            vec2_t p2 = wal2->pos;
            vec2_t d  = { p2.x-p1.x, p2.y-p1.y };

            if (d.x * (pos->y-p1.y) < (pos->x-p1.x) * d.y)
                continue;  //If wall's not facing you

            vec2_t const r = { (d.y > 0) ? clipMax.x : clipMin.x, (d.x > 0) ? clipMin.y : clipMax.y };
            vec2_t       v = { d.x * (r.y - p1.y), d.y * (r.x - p1.x) };

            if (v.x >= v.y)
                continue;

            int clipyou = 0;

                if (wal->nextsector < 0 || (wal->cstat & EWallFlags::FromInt(dawalclipmask)))
                {
                    clipyou = 1;
                }
                else
                {
                    clipmove_tweak_pos(pos, diff.x, diff.y, p1.x, p1.y, p2.x, p2.y, &v.x, &v.y);
                    clipyou = cliptestsector(dasect, wal->nextsector, flordist, ceildist, v, pos->z);
                }

           // We're not interested in any sector reached by portal traversal that we're "inside" of.
            if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE && !curspr && dasect != initialsectnum
                && inside(pos->x, pos->y, sec) == 1)
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
                int32_t bsz = walldist; if (diff.x < 0) bsz = -bsz;
                addclipline(p1.x-bsz, p1.y-bsz, p1.x-bsz, p1.y+bsz, objtype, false);
                addclipline(p2.x-bsz, p2.y-bsz, p2.x-bsz, p2.y+bsz, objtype, false);
                bsz = walldist; if (diff.y < 0) bsz = -bsz;
                addclipline(p1.x+bsz, p1.y-bsz, p1.x-bsz, p1.y-bsz, objtype, false);
                addclipline(p2.x+bsz, p2.y-bsz, p2.x-bsz, p2.y-bsz, objtype, false);

                v.x = walldist; if (d.y > 0) v.x = -v.x;
                v.y = walldist; if (d.x < 0) v.y = -v.y;

                if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE && d.x * (pos->y-p1.y-v.y) < (pos->x-p1.x-v.x) * d.y)
                    v.x >>= 1, v.y >>= 1;

                addclipline(p1.x+v.x, p1.y+v.y, p2.x+v.x, p2.y+v.y, objtype, false);
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
            auto const spr = &actor->s();
            const int32_t cstat = spr->cstat;

            if ((cstat&dasprclipmask) == 0)
                continue;

            auto p1 = spr->pos.vec2;

            CollisionBase obj;
            obj.setSprite(actor);

            switch (cstat & (CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_ALIGNMENT_FLOOR))
            {
            case CSTAT_SPRITE_ALIGNMENT_FACING:
                if (p1.x >= clipMin.x && p1.x <= clipMax.x && p1.y >= clipMin.y && p1.y <= clipMax.y)
                {
                    int32_t height, daz = spr->z+spriteheightofsptr(spr, &height, 1);

                    if (pos->z > daz-height-flordist && pos->z < daz+ceildist)
                    {
                        int32_t bsz = (spr->clipdist << 2)+walldist;
                        if (diff.x < 0) bsz = -bsz;
                        addclipline(p1.x-bsz, p1.y-bsz, p1.x-bsz, p1.y+bsz, obj, false);
                        bsz = (spr->clipdist << 2)+walldist;
                        if (diff.y < 0) bsz = -bsz;
                        addclipline(p1.x+bsz, p1.y-bsz, p1.x-bsz, p1.y-bsz, obj, false);
                    }
                }
                break;

            case CSTAT_SPRITE_ALIGNMENT_WALL:
            {
                int32_t height, daz = spr->z+spriteheightofsptr(spr, &height, 1);

                if (pos->z > daz-height-flordist && pos->z < daz+ceildist)
                {
                    vec2_t p2;

                    get_wallspr_points(spr, &p1.x, &p2.x, &p1.y, &p2.y);

                    if (clipinsideboxline(cent.x, cent.y, p1.x, p1.y, p2.x, p2.y, rad) != 0)
                    {
                        vec2_t v = { MulScale(bcos(spr->ang + 256), walldist, 14),
                                     MulScale(bsin(spr->ang + 256), walldist, 14) };

                        if ((p1.x-pos->x) * (p2.y-pos->y) >= (p2.x-pos->x) * (p1.y-pos->y))  // Front
                            addclipline(p1.x+v.x, p1.y+v.y, p2.x+v.y, p2.y-v.x, obj, false);
                        else
                        {
                            if ((cstat & CSTAT_SPRITE_ONE_SIDE) != 0)
                                continue;
                            addclipline(p2.x-v.x, p2.y-v.y, p1.x-v.y, p1.y+v.x, obj, false);
                        }

                        //Side blocker
                        if ((p2.x-p1.x) * (pos->x-p1.x)+(p2.y-p1.y) * (pos->y-p1.y) < 0)
                            addclipline(p1.x-v.y, p1.y+v.x, p1.x+v.x, p1.y+v.y, obj, true);
                        else if ((p1.x-p2.x) * (pos->x-p2.x)+(p1.y-p2.y) * (pos->y-p2.y) < 0)
                            addclipline(p2.x+v.y, p2.y-v.x, p2.x-v.x, p2.y-v.y, obj, true);
                    }
                }
                break;
            }

            case CSTAT_SPRITE_ALIGNMENT_FLOOR:
            {
                if (pos->z > spr->z-flordist && pos->z < spr->z+ceildist)
                {
                    if ((cstat & CSTAT_SPRITE_ONE_SIDE) != 0)
                        if ((pos->z > spr->z) == ((cstat & CSTAT_SPRITE_YFLIP)==0))
                            continue;

                    rxi[0] = p1.x;
                    ryi[0] = p1.y;

                    get_floorspr_points((uspriteptr_t) spr, 0, 0, &rxi[0], &rxi[1], &rxi[2], &rxi[3],
                        &ryi[0], &ryi[1], &ryi[2], &ryi[3]);

                    vec2_t v = { MulScale(bcos(spr->ang - 256), walldist, 14),
                                 MulScale(bsin(spr->ang - 256), walldist, 14) };

                    if ((rxi[0]-pos->x) * (ryi[1]-pos->y) < (rxi[1]-pos->x) * (ryi[0]-pos->y))
                    {
                        if (clipinsideboxline(cent.x, cent.y, rxi[1], ryi[1], rxi[0], ryi[0], rad) != 0)
                            addclipline(rxi[1]-v.y, ryi[1]+v.x, rxi[0]+v.x, ryi[0]+v.y, obj, false);
                    }
                    else if ((rxi[2]-pos->x) * (ryi[3]-pos->y) < (rxi[3]-pos->x) * (ryi[2]-pos->y))
                    {
                        if (clipinsideboxline(cent.x, cent.y, rxi[3], ryi[3], rxi[2], ryi[2], rad) != 0)
                            addclipline(rxi[3]+v.y, ryi[3]-v.x, rxi[2]-v.x, ryi[2]-v.y, obj, false);
                    }

                    if ((rxi[1]-pos->x) * (ryi[2]-pos->y) < (rxi[2]-pos->x) * (ryi[1]-pos->y))
                    {
                        if (clipinsideboxline(cent.x, cent.y, rxi[2], ryi[2], rxi[1], ryi[1], rad) != 0)
                            addclipline(rxi[2]-v.x, ryi[2]-v.y, rxi[1]-v.y, ryi[1]+v.x, obj, false);
                    }
                    else if ((rxi[3]-pos->x) * (ryi[0]-pos->y) < (rxi[0]-pos->x) * (ryi[3]-pos->y))
                    {
                        if (clipinsideboxline(cent.x, cent.y, rxi[0], ryi[0], rxi[3], ryi[3], rad) != 0)
                            addclipline(rxi[0]+v.x, ryi[0]+v.y, rxi[3]+v.y, ryi[3]-v.x, obj, false);
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
                if (!bitmap_test(clipignore, i) && clipinsideboxline(pos->x, pos->y, clipit[i].x1, clipit[i].y1, clipit[i].x2, clipit[i].y2, walldist))
                {
                    vec2_t const vec = pos->vec2;
                    keepaway(&pos->x, &pos->y, i);
                    if (inside_p(pos->x,pos->y, *sectnum) != 1)
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
            int32_t const templl = (int32_t)clamp<int64_t>(compat_maybe_truncate_to_int32((int64_t)clipr.x * clipr.x + (int64_t)clipr.y * clipr.y), INT32_MIN, INT32_MAX);

            if (templl > 0)
            {
                // I don't know if this one actually overflows or not, but I highly doubt it hurts to check
                int32_t const templl2
                = (int32_t)clamp<int64_t>(compat_maybe_truncate_to_int32((int64_t)(goal.x - vec.x) * clipr.x + (int64_t)(goal.y - vec.y) * clipr.y), INT32_MIN, INT32_MAX);
                int32_t const i = (enginecompatibility_mode == ENGINECOMPATIBILITY_19950829 || (abs(templl2)>>11) < templl) ?
                    (int)DivScaleL(templl2, templl, 20) : 0;

                goal = { MulScale(clipr.x, i, 20)+vec.x, MulScale(clipr.y, i, 20)+vec.y };
            }

            int32_t tempint;
            if (enginecompatibility_mode == ENGINECOMPATIBILITY_19950829)
                tempint = clipr.x*(move.x>>6)+clipr.y*(move.y>>6);
            else
                tempint = DMulScale(clipr.x, move.x, clipr.y, move.y, 6);

            for (int i=cnt+1, j; i<=clipmoveboxtracenum; ++i)
            {
                j = hitwalls[i];

                int32_t tempint2;
                if (enginecompatibility_mode == ENGINECOMPATIBILITY_19950829)
                    tempint2 = (clipit[j].x2-clipit[j].x1)*(move.x>>6)+(clipit[j].y2-clipit[j].y1)*(move.y>>6);
                else
                    tempint2 = DMulScale(clipit[j].x2-clipit[j].x1, move.x, clipit[j].y2-clipit[j].y1, move.y, 6);

                if ((tempint ^ tempint2) < 0)
                {
                    if (enginecompatibility_mode == ENGINECOMPATIBILITY_19961112)
                        updatesector(pos->x, pos->y, sectnum);
                    return clipReturn;
                }
            }

            keepaway(&goal.x, &goal.y, hitwall);
            xvect = (goal.x-vec.x)<<14;
            yvect = (goal.y-vec.y)<<14;

            if (cnt == clipmoveboxtracenum)
                clipReturn = clipobjectval[hitwall];
            hitwalls[cnt] = hitwall;
        }

        if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE)
		{
			clipupdatesector(vec, sectnum, rad);
		}

        pos->x = vec.x;
        pos->y = vec.y;
        cnt--;
    } while ((xvect|yvect) != 0 && hitwall >= 0 && cnt > 0);

    if (enginecompatibility_mode != ENGINECOMPATIBILITY_NONE)
    {
        for (int j=0; j<clipsectnum; j++)
            if (inside_p(pos->x, pos->y, clipsectorlist[j]) == 1)
            {
                *sectnum = clipsectorlist[j];
                return clipReturn;
            }

        int32_t tempint2, tempint1 = INT32_MAX;
        *sectnum = -1;
        for (int j = numsectors - 1; j >= 0; j--)
        {
            auto sect = &sector[j];
            if (inside(pos->x, pos->y, sect) == 1)
            {
                if (enginecompatibility_mode != ENGINECOMPATIBILITY_19950829 && (sect->ceilingstat & CSTAT_SECTOR_SLOPE))
                    tempint2 = getceilzofslopeptr(sect, pos->x, pos->y) - pos->z;
                else
                    tempint2 = sect->ceilingz - pos->z;

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
                        tempint2 = pos->z - getflorzofslopeptr(sect, pos->x, pos->y);
                    else
                        tempint2 = pos->z - sect->floorz;

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
            uwallptr_t wal;
            int32_t startwall, endwall;

            auto sec = (usectorptr_t)&sector[clipsectorlist[clipsectcnt]];
            if (dir > 0)
                startwall = sec->wallptr, endwall = startwall + sec->wallnum;
            else
                endwall = sec->wallptr - 1, startwall = endwall + sec->wallnum - 1;

            int i;

            for (i=startwall, wal=(uwallptr_t)&wall[startwall]; i!=endwall; i+=dir, wal+=dir)
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
                            int32_t dax = wal->point2Wall()->x-wal->x;
                            int32_t day = wal->point2Wall()->y-wal->y;
                            int32_t daz = dax*((vect->x)-wal->x) + day*((vect->y)-wal->y);
                            int32_t t;
                            if (daz <= 0)
                                t = 0;
                            else
                            {
                                daz2 = dax*dax+day*day;
                                if (daz >= daz2) t = (1<<30); else t = DivScale(daz, daz2, 30);
                            }
                            dax = wal->x + MulScale(dax, t, 30);
                            day = wal->y + MulScale(day, t, 30);

                            closest = { dax, day };
                        }
                       
                        j = cliptestsector(clipsectorlist[clipsectcnt], wal->nextsector, flordist, ceildist, closest, vect->z);
                    }

                    if (j != 0)
                    {
                        j = getangle(wal->point2Wall()->x-wal->x, wal->point2Wall()->y-wal->y);
                        int32_t dx = -bsin(j, -11);
                        int32_t dy = bcos(j, -11);
                        int bad2 = 16;
                        do
                        {
                            vect->x = (vect->x) + dx; vect->y = (vect->y) + dy;
                            bad2--; if (bad2 == 0) break;
                        } while (clipinsidebox(&vect->vec2, i, walldist-4) != 0);
                        bad = -1;
                        k--; if (k <= 0) return bad;
                        clipupdatesector(vect->vec2, sectnum, walldist);
                        if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE && *sectnum < 0) return -1;
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
    const int32_t xmin = pos.x-extradist, ymin = pos.y-extradist;
    const int32_t xmax = pos.x+extradist, ymax = pos.y+extradist;

    const int32_t dawalclipmask = (cliptype&65535);
    const int32_t dasprclipmask = (cliptype >> 16);

    vec2_t closest = pos.vec2;
    int sectnum = ::sectnum(sect);
    if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE)
        getsectordist(closest, sectnum, &closest);
    else
        getzsofslopeptr(sect,closest.x,closest.y,ceilz,florz);
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

        auto const startsec = (usectorptr_t)&sector[clipsectorlist[clipsectcnt]];

        for(auto&wal : wallsofsector(startsec))
        {
            if (wal.twoSided())
            {
                auto nextsect = wal.nextSector();
                vec2_t const v1 = wal.pos;
                vec2_t const v2 = wal.point2Wall()->pos;

                if ((v1.x < xmin && (v2.x < xmin)) || (v1.x > xmax && v2.x > xmax) ||
                    (v1.y < ymin && (v2.y < ymin)) || (v1.y > ymax && v2.y > ymax))
                    continue;

                vec2_t const d = { v2.x-v1.x, v2.y-v1.y };
                if (d.x*(pos.y-v1.y) < (pos.x-v1.x)*d.y) continue; //back

                vec2_t da = { (d.x > 0) ? d.x*(ymin-v1.y) : d.x*(ymax-v1.y),
                              (d.y > 0) ? d.y*(xmax-v1.x) : d.y*(xmin-v1.x) };

                if (da.x >= da.y)
                    continue;

                if (wal.cstat & EWallFlags::FromInt(dawalclipmask)) continue;  // XXX?

                if (((nextsect->ceilingstat & CSTAT_SECTOR_SKY) == 0) && (pos.z <= nextsect->ceilingz+(3<<8))) continue;
                if (((nextsect->floorstat & CSTAT_SECTOR_SKY) == 0) && (pos.z >= nextsect->floorz-(3<<8))) continue;

                int nextsectno = ::sectnum(nextsect);
                if (!clipsectormap[nextsectno])
                    addclipsect(nextsectno);

                if (((v1.x < xmin + MAXCLIPDIST) && (v2.x < xmin + MAXCLIPDIST)) ||
                    ((v1.x > xmax - MAXCLIPDIST) && (v2.x > xmax - MAXCLIPDIST)) ||
                    ((v1.y < ymin + MAXCLIPDIST) && (v2.y < ymin + MAXCLIPDIST)) ||
                    ((v1.y > ymax - MAXCLIPDIST) && (v2.y > ymax - MAXCLIPDIST)))
                    continue;

                if (d.x > 0) da.x += d.x*MAXCLIPDIST; else da.x -= d.x*MAXCLIPDIST;
                if (d.y > 0) da.y -= d.y*MAXCLIPDIST; else da.y += d.y*MAXCLIPDIST;
                if (da.x >= da.y)
                    continue;
                //It actually got here, through all the continue's!!!
                int32_t daz = 0, daz2 = 0;
                closest = pos.vec2;
                if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE)
                    getsectordist(closest, nextsectno, &closest);
                else
                    getzsofslopeptr(nextsect, closest.x,closest.y, &daz,&daz2);

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
            auto spr = &actor->s();
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
                        if ((abs(v1.x-pos.x) <= k) && (abs(v1.y-pos.y) <= k))
                        {
                            daz = spr->z + spriteheightofsptr(spr, &k, 1);
                            daz2 = daz - k;
                            clipyou = 1;
                        }
                        break;
                    }

                    case CSTAT_SPRITE_ALIGNMENT_WALL:
                    {
                        vec2_t v2;
                        get_wallspr_points(spr, &v1.x, &v2.x, &v1.y, &v2.y);

                        if (clipinsideboxline(pos.x,pos.y,v1.x,v1.y,v2.x,v2.y,walldist+1) != 0)
                        {
                            int32_t k;
                            daz = spr->z + spriteheightofsptr(spr, &k, 1);
                            daz2 = daz-k;
                            clipyou = 1;
                        }
                        break;
                    }

                    case CSTAT_SPRITE_ALIGNMENT_FLOOR:
                    {
                        daz = spr->z; daz2 = daz;

                        if ((cstat & CSTAT_SPRITE_ONE_SIDE) != 0 && (pos.z > daz) == ((cstat & CSTAT_SPRITE_YFLIP)==0))
                            continue;

                        vec2_t v2, v3, v4;
                        get_floorspr_points((uspriteptr_t) spr, pos.x, pos.y, &v1.x, &v2.x, &v3.x, &v4.x,
                                            &v1.y, &v2.y, &v3.y, &v4.y);

                        vec2_t const da = { MulScale(bcos(spr->ang - 256), walldist + 4, 14),
                                            MulScale(bsin(spr->ang - 256), walldist + 4, 14) };

                        v1.x += da.x; v2.x -= da.y; v3.x -= da.x; v4.x += da.y;
                        v1.y += da.y; v2.y += da.x; v3.y -= da.y; v4.y -= da.x;

                        clipyou = get_floorspr_clipyou(v1, v2, v3, v4);
                        break;
                    }
                }

                if (clipyou != 0)
                {
                    if ((pos.z > daz) && (daz > *ceilz))
                    {
                        *ceilz = daz;
                        ceilhit.setSprite(actor);
                    }

                    if ((pos.z < daz2) && (daz2 < *florz))
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
int32_t try_facespr_intersect(uspriteptr_t const spr, vec3_t const in,
                              int32_t vx, int32_t vy, int32_t vz,
                              vec3_t * const intp, int32_t strictly_smaller_than_p)
{
    vec3_t const sprpos = spr->pos;

    int32_t const topt = vx * (sprpos.x - in.x) + vy * (sprpos.y - in.y);

    if (topt <= 0) return 0;

    int32_t const bot = vx * vx + vy * vy;

    if (!bot) return 0;

    vec3_t        newpos = { 0, 0, in.z + Scale(vz, topt, bot) };
    int32_t       siz;
    int32_t const z1 = sprpos.z + spriteheightofsptr(spr, &siz, 1);

    if (newpos.z < z1 - siz || newpos.z > z1)
        return 0;

    int32_t const topu = vx * (sprpos.y - in.y) - vy * (sprpos.x - in.x);
    vec2_t  const off  = { Scale(vx, topu, bot), Scale(vy, topu, bot) };
    int32_t const dist = off.x * off.x + off.y * off.y;

    siz = tileWidth(spr->picnum) * spr->xrepeat;

    if (dist > MulScale(siz, siz, 7)) return 0;

    newpos.vec2 = { in.x + Scale(vx, topt, bot), in.y + Scale(vy, topt, bot) };

    if (abs(newpos.x - in.x) + abs(newpos.y - in.y) + strictly_smaller_than_p >
        abs(intp->x - in.x) + abs(intp->y - in.y))
        return 0;

    *intp = newpos;
    return 1;
}

static inline void hit_set(HitInfoBase *hit, sectortype* sect, walltype* wal, DCoreActor* actor, int32_t x, int32_t y, int32_t z)
{
    hit->hitSector = sect;
    hit->hitWall = wal;
    hit->hitActor = actor;
    hit->hitpos.x = x;
    hit->hitpos.y = y;
    hit->hitpos.z = z;
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
        auto const wal  = (uwallptr_t)sec->firstWall();
        auto const wal2 = (uwallptr_t)wal->point2Wall();
        int32_t j, dax=wal2->x-wal->x, day=wal2->y-wal->y;

        i = ksqrt(compat_maybe_truncate_to_int32(uhypsq(dax,day))); if (i == 0) return 1; //continue;
        i = DivScale(heinum,i, 15);
        dax *= i; day *= i;

        j = (vz<<8)-DMulScale(dax,vy,-day,vx, 15);
        if (j != 0)
        {
            i = ((z - sv->z)<<8)+DMulScale(dax,sv->y-wal->y,-day,sv->x-wal->x, 15);
            if (((i^j) >= 0) && ((abs(i)>>1) < abs(j)))
            {
                i = DivScale(i,j, 30);
                x1 = sv->x + MulScale(vx,i, 30);
                y1 = sv->y + MulScale(vy,i, 30);
                z1 = sv->z + MulScale(vz,i, 30);
            }
        }
    }
    else if ((how*vz > 0) && (how*sv->z <= how*z))
    {
        z1 = z; i = z1-sv->z;
        if ((abs(i)>>1) < vz*how)
        {
            i = DivScale(i,vz, 30);
            x1 = sv->x + MulScale(vx,i, 30);
            y1 = sv->y + MulScale(vy,i, 30);
        }
    }

    if ((x1 != INT32_MAX) && (abs(x1-sv->x)+abs(y1-sv->y) < abs((hit->hitpos.x)-sv->x)+abs((hit->hitpos.y)-sv->y)))
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
    int const vx = direction.x, vy = direction.y, vz = direction.z;
    int32_t x1, y1=0, z1=0, x2, y2, intx, inty, intz;
    int32_t i, k, daz;

    uspriteptr_t curspr = NULL;
    // tmp: { (int32_t)curidx, (spritetype *)curspr, (!=0 if outer sector) }
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
            if (curspr && !wal->twoSided()) continue;

            x1 = wal->x; y1 = wal->y; x2 = wal2->x; y2 = wal2->y;

            if (compat_maybe_truncate_to_int32((coord_t)(x1-sv->x)*(y2-sv->y))
                < compat_maybe_truncate_to_int32((coord_t)(x2-sv->x)*(y1-sv->y))) continue;
            if (rintersect(sv->x,sv->y,sv->z, vx,vy,vz, x1,y1, x2,y2, &intx,&inty,&intz) == -1) continue;

            if (abs(intx-sv->x)+abs(inty-sv->y) >= abs((hitinfo.hitpos.x)-sv->x)+abs((hitinfo.hitpos.y)-sv->y))
                continue;

            if (!curspr)
            {
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
            }
            search.Add(nextsect);
        }

        ////////// Sprites //////////

        if (dasprclipmask==0)
            continue;

        TSectIterator<DCoreActor> it(sec);
        while (auto actor = it.Next())
        {
            auto const spr = &actor->s();
            uint32_t const cstat = spr->cstat;

            if (spr->cstat2 & CSTAT2_SPRITE_NOFIND)
                continue;

#ifdef USE_OPENGL
            if (!hitallsprites)
#endif
                if ((cstat&dasprclipmask) == 0)
                    continue;

            x1 = spr->x; y1 = spr->y; z1 = spr->z;
            switch (cstat&CSTAT_SPRITE_ALIGNMENT_MASK)
            {
            case 0:
            {
                if (try_facespr_intersect(spr, *sv, vx, vy, vz, &hitinfo.hitpos, 0))
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
                    if (compat_maybe_truncate_to_int32((coord_t)(x1-sv->x)*(y2-sv->y))
                        < compat_maybe_truncate_to_int32((coord_t)(x2-sv->x)*(y1-sv->y))) continue;

                ucoefup16 = rintersect(sv->x,sv->y,sv->z,vx,vy,vz,x1,y1,x2,y2,&intx,&inty,&intz);
                if (ucoefup16 == -1) continue;

                if (abs(intx-sv->x)+abs(inty-sv->y) > abs((hitinfo.hitpos.x)-sv->x)+abs((hitinfo.hitpos.y)-sv->y))
                    continue;

                daz = spr->z + spriteheightofsptr(&actor->s(), &k, 1);
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

                if (vz == 0 || ((intz-sv->z)^vz) < 0) continue;

                if ((cstat & CSTAT_SPRITE_ONE_SIDE) != 0)
                    if ((sv->z > intz) == ((cstat & CSTAT_SPRITE_YFLIP)==0)) continue;

                // avoid overflow errors by using 64 bit math.
                intx = int(sv->x + (int64_t(intz) - sv->z) * vx / vz);
                inty = int(sv->y + (int64_t(intz) - sv->z) * vy / vz);

                if (abs(intx-sv->x)+abs(inty-sv->y) > abs((hitinfo.hitpos.x)-sv->x)+abs((hitinfo.hitpos.y)-sv->y))
                    continue;

                get_floorspr_points((uspriteptr_t)spr, intx, inty, &x1, &x2, &x3, &x4,
                                    &y1, &y2, &y3, &y4);

                if (get_floorspr_clipyou({x1, y1}, {x2, y2}, {x3, y3}, {x4, y4}))
                    hit_set(&hitinfo, sec, nullptr, actor, intx, inty, intz);

                break;
            }
            }
        }
    }

    return 0;
}

