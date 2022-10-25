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
static inline int32_t cliptrace(MoveClipper& clip, const DVector2& pos, DVector2* goal)
{
    int32_t hitwall = -1;

    for (int z=clip.clipobjects.Size() - 1; z >= 0; z--)
    {
        DVector2 const p1 = clip.clipobjects[z].line.start;
        DVector2 const p2 = clip.clipobjects[z].line.end;
        DVector2 const area = { p2.X-p1.X, p2.Y-p1.Y };

        int32_t topu = area.X*(pos.Y-p1.Y) - (pos.X-p1.X)*area.Y;

        if (topu <= 0 || area.X*(goal->Y-p1.Y) > (goal->X-p1.X)*area.Y)
            continue;

        DVector2 const diff = { goal->X-pos.X, goal->Y-pos.Y };

        if (diff.X*(p1.Y-pos.Y) > (p1.X-pos.X)*diff.Y || diff.X*(p2.Y-pos.Y) <= (p2.X-pos.X)*diff.Y)
            continue;

        int32_t const bot = diff.X*area.Y - area.X*diff.Y;
        int cnt = 256;

        if (!bot)
            continue;

        DVector2 n;

        do
        {
            if (--cnt < 0)
            {
                *goal = pos;
                return z;
            }

            n = { pos.X+ (diff.X * (topu / bot)), pos.Y + diff.Y *(topu / bot) };
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
CollisionBase clipmove_(vec3_t * const ipos, int * const sectnum, int32_t xvect, int32_t yvect,
                 int32_t const walldist, int32_t const ceildist, int32_t const flordist, uint32_t const cliptype, int clipmoveboxtracenum, bool precise)
{
    if ((xvect|yvect) == 0 || *sectnum < 0)
        return {};

    int const initialsectnum = *sectnum;

    vec2_t const move = { xvect, yvect };

    //Extra walldist for sprites on sector lines
    vec2_t const  diff = { xvect >> 14, yvect >> 14 };
	
	MoveClipper clip(&sector[initialsectnum]);
	
	clip.moveDelta = { (xvect >> 14) * inttoworld, (yvect >> 14) * inttoworld }; // beware of excess precision here!
	clip.wallflags = EWallFlags::FromInt(cliptype & 65535);
	clip.ceilingdist = ceildist * zinttoworld;
	clip.floordist = flordist * zinttoworld;
	clip.walldist = walldist * inttoworld;
    clip.pos = { ipos->X * inttoworld, ipos->Y * inttoworld, ipos->Z * zinttoworld };
    clip.dest = clip.pos + clip.moveDelta;
    clip.center = (clip.pos.XY() + clip.dest) * 0.5;
    clip.movedist = clip.moveDelta.Length() + clip.walldist + 0.5 + MAXCLIPDIST * inttoworld;
    clip.precise = precise;
    clip.rect.min = { clip.center.X - clip.movedist, clip.center.Y - clip.movedist };
    clip.rect.max = { clip.center.X + clip.movedist, clip.center.Y + clip.movedist };

    collectClipObjects(clip, (cliptype >> 16));

    int32_t hitwalls[4], hitwall;
    CollisionBase clipReturn{};

    int cnt = clipmoveboxtracenum;

    DVector2 fgoal = clip.dest;
    DVector2 fpos = clip.pos;

    auto copypos = [&]()
    {
        ipos->X = fpos.X * worldtoint;
        ipos->Y = fpos.Y * worldtoint;
    };


    do
    {
        if (clip.precise && (xvect|yvect)) 
        {
			PushAway(clip, fpos, &sector[*sectnum]);
        }

        auto fvec = fgoal;

        if ((hitwall = cliptrace(clip, fpos, &fvec)) >= 0)
        {
			auto clipdelta = clip.clipobjects[hitwall].line.end - clip.clipobjects[hitwall].line.start;
		
			fgoal = NearestPointOnLine(fgoal.X, fgoal.Y, fvec.X, fvec.Y, fvec.X + clipdelta.X, fvec.Y + clipdelta.Y, false);
			
            double dot1 = clipdelta.dot(clip.moveDelta);

            for (int i=cnt+1, j; i<=clipmoveboxtracenum; ++i)
            {
                j = hitwalls[i];

                double dot2 = clip.clipobjects[i].line.delta().dot(clip.moveDelta);

                if ((dot1 < 0) != (dot2 < 0))
                {
                    if (!clip.precise)
                    {
                        auto sectp = &sector[*sectnum];
                        updatesector(fpos, &sectp);
                        *sectnum = sectp ? ::sectnum(sectp) : -1;
                    }
                    copypos();
                    return clipReturn;
                }
            }

			keepaway(clip, fgoal, clip.clipobjects[hitwall]);
            
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
            DVector3 v(fvec, 0);
            sectortype* sect = &sector[*sectnum];
			updatesector(v, &sect, clip.movedist);
            *sectnum = ::sectnum(sect);
		}

        fpos = fvec;
        cnt--;
    } while ((xvect|yvect) != 0 && hitwall >= 0 && cnt > 0);

    if (!clip.precise)
    {

        *sectnum = FindSectorInSearchList(fpos, clip.search);
        if (*sectnum == -1) *sectnum = FindBestSector(fpos);
    }

    copypos();
    return clipReturn;
}


