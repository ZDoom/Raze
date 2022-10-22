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
#include "texturemanager.h"
#include "buildtiles.h"

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

static inline int bsin(const int ang)
{
    return int(g_sinbam(ang * (1 << 21)) * 16384);
}

static inline int bcos(const int ang)
{
    return int(g_cosbam(ang * (1 << 21)) * 16384);
}

////////// CLIPMOVE //////////
inline uint8_t bitmap_test(uint8_t const* const ptr, int const n) { return ptr[n >> 3] & (1 << (n & 7)); }


// x1, y1: in/out
// rest x/y: out
static inline void get_wallspr_points(DCoreActor* actor, int32_t *x1, int32_t *x2, int32_t *y1, int32_t *y2)
{
    DVector2 out[2];
    GetWallSpritePosition(&actor->spr, DVector2(*x1 * inttoworld, *y1 * inttoworld), out);
    *x1 = int(out[0].X * worldtoint); *y1 = int(out[0].Y * worldtoint);
    *x2 = int(out[1].X * worldtoint); *y2 = int(out[1].Y * worldtoint);
}

// x1, y1: in/out
// rest x/y: out
static inline void get_floorspr_points(DCoreActor *spr, int32_t px, int32_t py,
                                       int32_t *x1, int32_t *x2, int32_t *x3, int32_t *x4,
                                       int32_t *y1, int32_t *y2, int32_t *y3, int32_t *y4)
{
    DVector2 out[4];
    // very messed up interface... :(
    GetFlatSpritePosition(spr, DVector2((*x1 - px) * inttoworld, (*y1 - py) * inttoworld), out);
    *x1 = int(out[0].X * worldtoint); *y1 = int(out[0].Y * worldtoint);
    *x2 = int(out[1].X * worldtoint); *y2 = int(out[1].Y * worldtoint);
    *x3 = int(out[2].X * worldtoint); *y3 = int(out[2].Y * worldtoint);
    *x4 = int(out[3].X * worldtoint); *y4 = int(out[3].Y * worldtoint);

}

inline int32_t spriteGetZOfSlope(const spritetypebase* tspr, int dax, int day, int heinum)
{
    return int(spriteGetZOfSlopef(tspr, DVector2(dax * inttoworld, day * inttoworld), heinum) * zworldtoint);
}


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

inline void clipmove_tweak_pos(const vec3_t *pos, int32_t gx, int32_t gy, int32_t x1, int32_t y1, int32_t x2,
                                      int32_t y2, double *daxptr, double *dayptr)
{
    double x = pos->X * inttoworld, y = pos->Y * inttoworld;
    double result = InterceptLineSegments(x, y, gx * inttoworld, gy * inttoworld, x1 * inttoworld, y1 * inttoworld, (x2 - x1) * inttoworld, (y2 - y1) * inttoworld);
    if (result > 0)
    {
        *daxptr = x + result * gx * inttoworld;
        *dayptr = y + result * gy * inttoworld;
    }
    else
    {
        *daxptr = x;
        *dayptr = y;
    }
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

        auto const sec       = &sector[dasect];

        for (auto& wal : sec->walls)
        {
			clip.pos = { pos->X * inttoworld, pos->Y * inttoworld, pos->Z * zinttoworld};
			
			int clipyou2 = checkClipWall(clip, &wal);
		
            auto const wal2 = wal.point2Wall();
			vec2_t p1 = wal.wall_int_pos();
			vec2_t p2 = wal2->wall_int_pos();

            if ((p1.X < clipMin.X && p2.X < clipMin.X) || (p1.X > clipMax.X && p2.X > clipMax.X) ||
                (p1.Y < clipMin.Y && p2.Y < clipMin.Y) || (p1.Y > clipMax.Y && p2.Y > clipMax.Y))
            {
                assert(clipyou2 != 1);
                continue;
            }

            vec2_t d  = { p2.X-p1.X, p2.Y-p1.Y };

            if (d.X * (pos->Y-p1.Y) < (pos->X-p1.X) * d.Y)
            {
                assert(clipyou2 != 1);
                continue;
            }

            vec2_t const r = { (d.Y > 0) ? clipMax.X : clipMin.X, (d.X > 0) ? clipMin.Y : clipMax.Y };
            vec2_t       v = { d.X * (r.Y - p1.Y), d.Y * (r.X - p1.X) };

            if (v.X >= v.Y)
            {
                assert(clipyou2 != 1);
                continue;
            }

            int clipyou = 0;

            if (wal.nextsector < 0 || (wal.cstat & EWallFlags::FromInt(dawalclipmask)))
            {
                clipyou = 1;
            }
            else
            {
                DVector2 ipos;
                clipmove_tweak_pos(pos, diff.X, diff.Y, p1.X, p1.Y, p2.X, p2.Y, &ipos.X, &ipos.Y);
                clipyou = checkOpening(ipos, pos->Z * zinttoworld, &sector[dasect], wal.nextSector(),
                    ceildist * zinttoworld, flordist * zinttoworld, enginecompatibility_mode == ENGINECOMPATIBILITY_NONE);
                v.X = int(ipos.X * worldtoint);
                v.Y = int(ipos.Y * worldtoint);
            }


            assert(clipyou == clipyou2);
            if (clipyou)
            {
                CollisionBase objtype;
                objtype.setWall(&wal);

                //Add 2 boxes at endpoints
                int32_t bsz = walldist; if (diff.X < 0) bsz = -bsz;
                addclipline(p1.X-bsz, p1.Y-bsz, p1.X-bsz, p1.Y+bsz, objtype, false);
                addclipline(p2.X-bsz, p2.Y-bsz, p2.X-bsz, p2.Y+bsz, objtype, false);
                bsz = walldist; if (diff.Y < 0) bsz = -bsz;
                addclipline(p1.X+bsz, p1.Y-bsz, p1.X-bsz, p1.Y-bsz, objtype, false);
                addclipline(p2.X+bsz, p2.Y-bsz, p2.X-bsz, p2.Y-bsz, objtype, false);

				vec2_t v;
                v.X = walldist; if (d.Y > 0) v.X = -v.X;
                v.Y = walldist; if (d.X < 0) v.Y = -v.Y;

                if (enginecompatibility_mode == ENGINECOMPATIBILITY_NONE && d.X * (pos->Y-p1.Y-v.Y) < (pos->X-p1.X-v.X) * d.Y)
                    v.X >>= 1, v.Y >>= 1;

                addclipline(p1.X+v.X, p1.Y+v.Y, p2.X+v.X, p2.Y+v.Y, objtype, false);
            }
            else if (wal.nextsector>=0)
            {
                if (!clipsectormap[wal.nextsector])
                    addclipsect(wal.nextsector);
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
            const int32_t cstat = actor->spr.cstat;

            if (actor->spr.cstat2 & CSTAT2_SPRITE_NOFIND) continue;
            if ((cstat&dasprclipmask) == 0)
                continue;

            auto p1 = actor->int_pos().vec2;

            CollisionBase obj;
            obj.setSprite(actor);

            switch (cstat & (CSTAT_SPRITE_ALIGNMENT_MASK))
            {
            case CSTAT_SPRITE_ALIGNMENT_FACING:
                if (p1.X >= clipMin.X && p1.X <= clipMax.X && p1.Y >= clipMin.Y && p1.Y <= clipMax.Y)
                {
                    double height_, daz_ = actor->spr.pos.Z + actor->GetOffsetAndHeight(height_);
                    int height = int(height_ * zworldtoint), daz = int(daz_ * zworldtoint);

                    if (pos->Z > daz-height-flordist && pos->Z < daz+ceildist)
                    {
                        int cd = int(actor->clipdist * worldtoint);
                        int32_t bsz = cd + walldist;
                        if (diff.X < 0) bsz = -bsz;
                        addclipline(p1.X-bsz, p1.Y-bsz, p1.X-bsz, p1.Y+bsz, obj, false);
                        bsz = cd + walldist;
                        if (diff.Y < 0) bsz = -bsz;
                        addclipline(p1.X+bsz, p1.Y-bsz, p1.X-bsz, p1.Y-bsz, obj, false);
                    }
                }
                break;

            case CSTAT_SPRITE_ALIGNMENT_WALL:
            {
                double height_, daz_ = actor->spr.pos.Z + actor->GetOffsetAndHeight(height_);
                int height = int(height_ * zworldtoint), daz = int(daz_ * zworldtoint);

                if (pos->Z > daz-height-flordist && pos->Z < daz+ceildist)
                {
                    vec2_t p2;

                    get_wallspr_points(actor, &p1.X, &p2.X, &p1.Y, &p2.Y);

                    if (clipinsideboxline(cent.X, cent.Y, p1.X, p1.Y, p2.X, p2.Y, rad) != 0)
                    {
                        vec2_t v = { MulScale(bcos(actor->int_ang() + 256), walldist, 14),
                                     MulScale(bsin(actor->int_ang() + 256), walldist, 14) };

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
                int heinum = spriteGetSlope(actor);
                int sz = spriteGetZOfSlope(&actor->spr, pos->X, pos->Y, heinum);

                if (pos->Z > sz - flordist && pos->Z < sz + ceildist)
                {
                    if ((cstat & CSTAT_SPRITE_ONE_SIDE) != 0)
                        if ((pos->Z > sz) == ((cstat & CSTAT_SPRITE_YFLIP)==0))
                            continue;

                    rxi[0] = p1.X;
                    ryi[0] = p1.Y;

                    get_floorspr_points(actor, 0, 0, &rxi[0], &rxi[1], &rxi[2], &rxi[3],
                        &ryi[0], &ryi[1], &ryi[2], &ryi[3]);

                    vec2_t v = { MulScale(bcos(actor->int_ang() - 256), walldist, 14),
                                 MulScale(bsin(actor->int_ang() - 256), walldist, 14) };

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
                auto tex = TexMan.GetGameTexture(actor->spr.spritetexture());
                const int32_t cosang = bcos(actor->int_ang());
                const int32_t sinang = bsin(actor->int_ang());
                vec2_t const span = { (int)tex->GetDisplayWidth(), (int)tex->GetDisplayHeight() };
                vec2_t const repeat = { int(actor->spr.scale.X * scaletoint), int(actor->spr.scale.Y * scaletoint) };
                vec2_t adjofs = { (int)tex->GetDisplayLeftOffset(), (int)tex->GetDisplayTopOffset() };

                if (actor->spr.cstat & CSTAT_SPRITE_XFLIP)
                    adjofs.X = -adjofs.X;

                if (actor->spr.cstat & CSTAT_SPRITE_YFLIP)
                    adjofs.Y = -adjofs.Y;

                int32_t const centerx = ((span.X >> 1) + adjofs.X) * repeat.X;
                int32_t const centery = ((span.Y >> 1) + adjofs.Y) * repeat.Y;
                int32_t const rspanx = span.X * repeat.X;
                int32_t const rspany = span.Y * repeat.Y;
                int32_t const ratio = ksqrt(heinum * heinum + SLOPEVAL_FACTOR * SLOPEVAL_FACTOR);
                int32_t zz[3] = { pos->Z, pos->Z + flordist, pos->Z - ceildist };
                for (int k = 0; k < 3; k++)
                {
                    int32_t jj = DivScale(actor->int_pos().Z - zz[k], heinum, 18);
                    int32_t jj2 = MulScale(jj, ratio, 12);
                    if (jj2 > (centery << 8) || jj2 < ((centery - rspany) << 8))
                        continue;
                    int32_t x1 = actor->int_pos().X + MulScale(sinang, centerx, 16) + MulScale(jj, cosang, 24);
                    int32_t y1 = actor->int_pos().Y - MulScale(cosang, centerx, 16) + MulScale(jj, sinang, 24);
                    int32_t x2 = x1 - MulScale(sinang, rspanx, 16);
                    int32_t y2 = y1 + MulScale(cosang, rspanx, 16);

                    vec2_t const v = { MulScale(bcos(actor->int_ang() - 256), walldist, 14),
                                       MulScale(bsin(actor->int_ang() - 256), walldist, 14) };

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
                        *sectnum = sectp ? ::sectindex(sectp) : -1;
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
            *sectnum = ::sectindex(sect);
		}

        pos->X = vec.X;
        pos->Y = vec.Y;
        cnt--;
    } while ((xvect|yvect) != 0 && hitwall >= 0 && cnt > 0);

    if (enginecompatibility_mode != ENGINECOMPATIBILITY_NONE)
    {
        DVector3 fpos(pos->X* inttoworld, pos->Y* inttoworld, pos->Z* zinttoworld);

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


