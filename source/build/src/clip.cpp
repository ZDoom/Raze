// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#include "printf.h"
#include "gamefuncs.h"
#include "coreactor.h"

enum { MAXCLIPDIST = 1024 };

//
// clipinsideboxline
//
static int clipinsideboxline(int x, int y, int x1, int y1, int x2, int y2, int walldist)
{
    return (int)IsCloseToLine(DVector2(x * inttoworld, y * inttoworld), DVector2(x1 * inttoworld, y1 * inttoworld), DVector2(x2 * inttoworld, y2 * inttoworld), walldist * inttoworld);
}


//
// raytrace (internal)
//
static inline int32_t cliptrace(MoveClipper& clip, vec2_t const pos, vec2_t * const goal)
{
    int32_t hitwall = -1;

    for (int z=clip.clipobjects.Size() - 1; z >= 0; z--)
    {
        vec2_t const p1   = { clip.clipobjects[z].x1(), clip.clipobjects[z].y1()};
        vec2_t const p2   = { clip.clipobjects[z].x2(), clip.clipobjects[z].y2()};
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
static inline void keepaway(MoveClipper& clip, int32_t *x, int32_t *y, int32_t w)
{
    const int32_t x1 = clip.clipobjects[w].x1(), dx = clip.clipobjects[w].x2() - x1;
    const int32_t y1 = clip.clipobjects[w].y1(), dy = clip.clipobjects[w].y2() - y1;
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
                 int32_t const walldist, int32_t const ceildist, int32_t const flordist, uint32_t const cliptype, int clipmoveboxtracenum, bool precise)
{
    if ((xvect|yvect) == 0 || *sectnum < 0)
        return {};

    int const initialsectnum = *sectnum;

    vec2_t const move = { xvect, yvect };
    vec2_t       goal = { pos->X + (xvect >> 14), pos->Y + (yvect >> 14) };
    vec2_t const cent = { (pos->X + goal.X) >> 1, (pos->Y + goal.Y) >> 1 };

    //Extra walldist for sprites on sector lines
    vec2_t const  diff    = { goal.X - (pos->X), goal.Y - (pos->Y) };
    int32_t const rad     = (int)g_sqrt((double)diff.X * diff.X + (double)diff.Y * diff.Y) + MAXCLIPDIST + walldist + 8;
    vec2_t const  clipMin = { cent.X - rad, cent.Y - rad };
    vec2_t const  clipMax = { cent.X + rad, cent.Y + rad };
	
	MoveClipper clip(&sector[initialsectnum]);
	
	clip.moveDelta = { (xvect >> 14) * inttoworld, (yvect >> 14) * inttoworld }; // beware of excess precision here!
	clip.rect.min = { clipMin.X * inttoworld, clipMin.Y * inttoworld };
	clip.rect.max = { clipMax.X * inttoworld, clipMax.Y * inttoworld };
	clip.wallflags = EWallFlags::FromInt(cliptype & 65535);
	clip.ceilingdist = ceildist * zinttoworld;
	clip.floordist = flordist * zinttoworld;
	clip.walldist = walldist * inttoworld;
    clip.pos = { pos->X * inttoworld, pos->Y * inttoworld, pos->Z * zinttoworld };
    clip.dest = { goal.X * inttoworld, goal.Y * inttoworld };
    clip.center = (clip.pos.XY() + clip.dest) * 0.5;
    clip.movedist = clip.moveDelta.Length() + clip.walldist + 0.5 + MAXCLIPDIST * inttoworld;
    clip.precise = precise;

    collectClipObjects(clip, (cliptype >> 16));

    int32_t hitwalls[4], hitwall;
    CollisionBase clipReturn{};

    int cnt = clipmoveboxtracenum;

    do
    {
        if (clip.precise && (xvect|yvect)) 
        {
			DVector2 fpos(pos->X * inttoworld, pos->Y * inttoworld);
			PushAway(clip, fpos, &sector[*sectnum]);
			pos->X = int(fpos.X * worldtoint);
			pos->Y = int(fpos.Y * worldtoint);
        }

        vec2_t vec = goal;

        if ((hitwall = cliptrace(clip, pos->vec2, &vec)) >= 0)
        {
			auto clipdelta = clip.clipobjects[hitwall].line.end - clip.clipobjects[hitwall].line.start;
			DVector2 fgoal(goal.X * inttoworld, goal.Y * inttoworld);
			DVector2 fvec(vec.X * inttoworld, vec.Y * inttoworld);
		
			fgoal = NearestPointOnLine(fgoal.X, fgoal.Y, fvec.X, fvec.Y, fvec.X + clipdelta.X, fvec.Y + clipdelta.Y, false);
			
			vec2_t const  clipr  = { clip.clipobjects[hitwall].x2() - clip.clipobjects[hitwall].x1(), clip.clipobjects[hitwall].y2() - clip.clipobjects[hitwall].y1()};
            int32_t tempint;
            tempint = DMulScale(clipr.X, move.X, clipr.Y, move.Y, 6);

            for (int i=cnt+1, j; i<=clipmoveboxtracenum; ++i)
            {
                j = hitwalls[i];

                int32_t tempint2;
                tempint2 = DMulScale(clip.clipobjects[j].x2() - clip.clipobjects[j].x1(), move.X, clip.clipobjects[j].y2() - clip.clipobjects[j].y1(), move.Y, 6);

                if ((tempint ^ tempint2) < 0)
                {
                    if (!clip.precise)
                    {
                        auto sectp = &sector[*sectnum];
                        updatesector(DVector2(pos->X * inttoworld, pos->Y * inttoworld), &sectp);
                        *sectnum = sectp ? ::sectnum(sectp) : -1;
                    }
                    return clipReturn;
                }
            }

			keepaway(clip, fgoal, clip.clipobjects[hitwall]);
			goal.X = int(fgoal.X * worldtoint);
			goal.Y = int(fgoal.Y * worldtoint);
            
			//keepaway(clip, &goal.X, &goal.Y, hitwall);
            //xvect = (goal.X-vec.X)<<14;
            //yvect = (goal.Y-vec.Y)<<14;
			xvect = FloatToFixed<4>(fgoal.X - fvec.X) << 14;
			yvect = FloatToFixed<4>(fgoal.Y - fvec.Y) << 14;

            if (cnt == clipmoveboxtracenum)
                clipReturn = clip.clipobjects[hitwall].obj;
            hitwalls[cnt] = hitwall;
        }

        if (clip.precise)
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

    if (!clip.precise)
    {
        DVector3 fpos(pos->X* inttoworld, pos->Y* inttoworld, pos->Z* inttoworld);
        *sectnum = FindSectorInSearchList(fpos, clip.search);
        if (*sectnum == -1) *sectnum = FindBestSector(fpos);
    }

    return clipReturn;
}


