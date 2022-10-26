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

        double topu = area.X*(pos.Y-p1.Y) - (pos.X-p1.X)*area.Y;

        if (topu <= 0 || area.X*(goal->Y-p1.Y) > (goal->X-p1.X)*area.Y)
            continue;

        DVector2 const diff = { goal->X-pos.X, goal->Y-pos.Y };

        if (diff.X*(p1.Y-pos.Y) > (p1.X-pos.X)*diff.Y || diff.X*(p2.Y-pos.Y) <= (p2.X-pos.X)*diff.Y)
            continue;

        double const bot = diff.X*area.Y - area.X*diff.Y;
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
// clipmove
//
CollisionBase clipmove_(DVector3& inpos, sectortype** const sect, const DVector2& invect,
                 double const walldist, double const ceildist, double const flordist, uint32_t const cliptype, int clipmoveboxtracenum, bool precise)
{
    if (invect.isZero() || *sect == nullptr)
        return {};

    //Extra walldist for sprites on sector lines
	
	MoveClipper clip(*sect);
	
    clip.moveDelta = invect;
	clip.wallflags = EWallFlags::FromInt(cliptype & 65535);
	clip.ceilingdist = ceildist;
	clip.floordist = flordist;
	clip.walldist = walldist;
    clip.pos = inpos;
    clip.dest = clip.pos + clip.moveDelta;
    clip.center = (clip.pos.XY() + clip.dest) * 0.5;
    clip.movedist = clip.moveDelta.Length() + walldist + 0.5 + MAXCLIPDIST * inttoworld;
    clip.precise = precise;
    clip.rect.min = { clip.center.X - clip.movedist, clip.center.Y - clip.movedist };
    clip.rect.max = { clip.center.X + clip.movedist, clip.center.Y + clip.movedist };

    collectClipObjects(clip, (cliptype >> 16));

    int32_t hitwalls[4], hitwall;
    CollisionBase clipReturn{};

    int cnt = clipmoveboxtracenum;

    DVector2 fgoal = clip.dest;
    DVector2 fpos = clip.pos;
    DVector2 fvect = invect;

    auto copypos = [&]()
    {
        inpos.XY() = fpos;
    };


    do
    {
        if (clip.precise && !fvect.isZero())
        {
			PushAway(clip, fpos, *sect);
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
                        updatesector(fpos, sect);
                    }
                    copypos();
                    return clipReturn;
                }
            }

			keepaway(clip, fgoal, clip.clipobjects[hitwall]);
            
			//keepaway(clip, &goal.X, &goal.Y, hitwall);
            //xvect = (goal.X-vec.X)<<14;
            //yvect = (goal.Y-vec.Y)<<14;
            fvect = fgoal - fvec;

            if (cnt == clipmoveboxtracenum)
                clipReturn = clip.clipobjects[hitwall].obj;
            hitwalls[cnt] = hitwall;
        }

        if (clip.precise)
		{
            DVector3 v(fvec, 0);
			updatesector(v, sect, clip.movedist);
		}

        fpos = fvec;
        cnt--;
    } while (!fvect.isZero() && hitwall >= 0 && cnt > 0);

    if (!clip.precise)
    {
        DVector3 pos3(fpos, clip.pos.Z);
        *sect = FindSectorInSearchList(pos3, clip.search);
        if (*sect == nullptr) *sect = FindBestSector(pos3);
    }

    copypos();
    return clipReturn;
}


