// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#include "build.h"
#include "clip.h"
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

////////// CLIPMOVE //////////
inline uint8_t bitmap_test(uint8_t const* const ptr, int const n) { return ptr[n >> 3] & (1 << (n & 7)); }

//
// clipinsideboxline
//
static int clipinsideboxline(int x, int y, int x1, int y1, int x2, int y2, int walldist)
{
    return (int)IsCloseToLine(DVector2(x * inttoworld, y * inttoworld), DVector2(x1 * inttoworld, y1 * inttoworld), DVector2(x2 * inttoworld, y2 * inttoworld), walldist * inttoworld);
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

void addClipLine(MoveClipper& clip, const DVector2& start, const DVector2& end, const CollisionBase& daoval, int nofix)
{
    addclipline(int(start.X * worldtoint), int(start.Y * worldtoint), int(end.X * worldtoint), int(end.Y * worldtoint), daoval, nofix);
}

void addClipSect(MoveClipper& clip, int sec)
{
    if (!clipsectormap[sec])
        addclipsect(sec);
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
    uint8_t first = (abs(dx) <= abs(dy));

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

//
// clipmove
//
CollisionBase clipmove_(vec3_t * const pos, int * const sectnum, int32_t xvect, int32_t yvect,
                 int32_t const walldist, int32_t const ceildist, int32_t const flordist, uint32_t const cliptype, int clipmoveboxtracenum)
{
    CollisionBase b{};
    if ((xvect|yvect) == 0 || *sectnum < 0)
        return b;

    int const initialsectnum = *sectnum;

    int32_t const dawalclipmask = (cliptype & 65535);  // CLIPMASK0 = 0x00010001 (in desperate need of getting fixed!)
    int32_t const dasprclipmask = (cliptype >> 16);    // CLIPMASK1 = 0x01000040

    vec2_t const move = { xvect, yvect };
    vec2_t       goal = { pos->X + (xvect >> 14), pos->Y + (yvect >> 14) };
    vec2_t const cent = { (pos->X + goal.X) >> 1, (pos->Y + goal.Y) >> 1 };

    //Extra walldist for sprites on sector lines
    vec2_t const  diff    = { goal.X - (pos->X), goal.Y - (pos->Y) };
    int32_t const rad     = ksqrt((int64_t)diff.X * diff.X + (int64_t)diff.Y * diff.Y) + MAXCLIPDIST + walldist + 8;
    vec2_t const  clipMin = { cent.X - rad, cent.Y - rad };
    vec2_t const  clipMax = { cent.X + rad, cent.Y + rad };
	
	MoveClipper clip;
	
	clip.moveDelta = { (xvect >> 14) * inttoworld, (yvect >> 14) * inttoworld }; // beware of excess precision here!
	clip.rect.min = { clipMin.X * inttoworld, clipMin.Y * inttoworld };
	clip.rect.max = { clipMax.X * inttoworld, clipMax.Y * inttoworld };
	clip.wallflags = EWallFlags::FromInt(dawalclipmask);
	clip.ceilingdist = ceildist * zinttoworld;
	clip.floordist = flordist * zinttoworld;
	clip.walldist = walldist * inttoworld;
    clip.pos = { pos->X * inttoworld, pos->Y * inttoworld, pos->Z * zinttoworld };
    clip.dest = { goal.X * inttoworld, goal.Y * inttoworld };
    clip.center = (clip.pos.XY() + clip.dest) * 0.5;
    clip.movedist = clip.moveDelta.Length() + clip.walldist + 0.5 + MAXCLIPDIST * inttoworld;

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

        ////////// Walls //////////
        processClipWalls(clip, &sector[dasect]);

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
            int cstat = actor->spr.cstat;

            if (actor->spr.cstat2 & CSTAT2_SPRITE_NOFIND) continue;
            if ((cstat & dasprclipmask) == 0)
                continue;

            switch (cstat & (CSTAT_SPRITE_ALIGNMENT_MASK))
            {
            case CSTAT_SPRITE_ALIGNMENT_FACING:
                processClipFaceSprite(clip, actor);
                break;

            case CSTAT_SPRITE_ALIGNMENT_WALL:
                processClipWallSprite(clip, actor);
                break;

            case CSTAT_SPRITE_ALIGNMENT_FLOOR:
                processClipFloorSprite(clip, actor);
                break;

            case CSTAT_SPRITE_ALIGNMENT_SLOPE:
                processClipSlopeSprite(clip, actor);
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
                    if (inside(pos->X * inttoworld, pos->Y * inttoworld, &sector[*sectnum]) != 1)
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
            int32_t const templl = (int32_t)clamp<int64_t>(((int64_t)clipr.X * clipr.X + (int64_t)clipr.Y * clipr.Y), INT32_MIN, INT32_MAX);

            if (templl > 0)
            {
                // I don't know if this one actually overflows or not, but I highly doubt it hurts to check
                int32_t const templl2
                = (int32_t)clamp<int64_t>(((int64_t)(goal.X - vec.X) * clipr.X + (int64_t)(goal.Y - vec.Y) * clipr.Y), INT32_MIN, INT32_MAX);
                int32_t const i = ((abs(templl2)>>11) < templl) ? (int)DivScaleL(templl2, templl, 20) : 0;

                goal = { MulScale(clipr.X, i, 20)+vec.X, MulScale(clipr.Y, i, 20)+vec.Y };
            }

            int32_t tempint;
            tempint = DMulScale(clipr.X, move.X, clipr.Y, move.Y, 6);

            for (int i=cnt+1, j; i<=clipmoveboxtracenum; ++i)
            {
                j = hitwalls[i];

                int32_t tempint2;
                tempint2 = DMulScale(clipit[j].x2-clipit[j].x1, move.X, clipit[j].y2-clipit[j].y1, move.Y, 6);

                if ((tempint ^ tempint2) < 0)
                {
                    if (enginecompatibility_mode == ENGINECOMPATIBILITY_19961112)
                    {
                        auto sectp = &sector[*sectnum];
                        updatesector(DVector2(pos->X * inttoworld, pos->Y * inttoworld), &sectp);
                        *sectnum = sectp ? ::sectnum(sectp) : -1;
                    }
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
            DVector2 v(vec.X* inttoworld, vec.Y* inttoworld);
            sectortype* sect = &sector[*sectnum];
			updatesector(v, &sect, rad * inttoworld);
            *sectnum = ::sectnum(sect);
		}

        pos->X = vec.X;
        pos->Y = vec.Y;
        cnt--;
    } while ((xvect|yvect) != 0 && hitwall >= 0 && cnt > 0);

    if (enginecompatibility_mode != ENGINECOMPATIBILITY_NONE)
    {
        DVector3 fpos(pos->X* inttoworld, pos->Y* inttoworld, pos->Z* inttoworld);

        for (int j=0; j<clipsectnum; j++)
            if (inside(fpos.X, fpos.Y, &sector[clipsectorlist[j]]) == 1)
            {
                *sectnum = clipsectorlist[j];
                return clipReturn;
            }

        *sectnum = FindBestSector(fpos);
    }

    return clipReturn;
}


