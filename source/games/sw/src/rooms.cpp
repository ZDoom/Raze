//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "ns.h"
#include "build.h"

#include "names2.h"
#include "panel.h"
#include "misc.h"
#include "hw_drawinfo.h"

BEGIN_SW_NS

////////////////////////////////////////////////////////////////////
//
// FLOOR ABOVE FLOOR
//
////////////////////////////////////////////////////////////////////


#define ZMAX 400
typedef struct
{
    int32_t zval[ZMAX];
    int16_t sectnum[ZMAX];
    int16_t pic[ZMAX];
    int16_t zcount;
    int16_t slope[ZMAX];
} SAVE, *SAVEp;

SAVE save;

bool FAF_DebugView = false;

DSWActor* InsertActor(int sectnum, int stat)
{
    short spnum;
    spnum = insertsprite(sectnum, stat);
    auto pActor = &swActors[spnum];
    auto pSprite = &pActor->s();

    PRODUCTION_ASSERT(spnum >= 0);

    pSprite->x = pSprite->y = pSprite->z = 0;
    pSprite->cstat = 0;
    pSprite->picnum = 0;
    pSprite->shade = 0;
    pSprite->pal = 0;
    pSprite->clipdist = 0;
    pSprite->xrepeat = pSprite->yrepeat = 0;
    pSprite->xoffset = pSprite->yoffset = 0;
    pSprite->ang = 0;
    pSprite->owner = -1;
    pSprite->xvel = pSprite->yvel = pSprite->zvel = 0;
    pSprite->lotag = 0;
    pSprite->hitag = 0;
    pSprite->extra = 0;

    return pActor;
}

bool FAF_Sector(short sectnum)
{
    SPRITEp sp;

    SWSectIterator it(sectnum);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (sp->statnum == STAT_FAF &&
            (sp->hitag >= VIEW_LEVEL1 && sp->hitag <= VIEW_LEVEL6))
        {
            return true;
        }
    }

    return false;
}

void SetWallWarpHitscan(short sectnum)
{
    short start_wall, wall_num;
    DSWActor* sp_warp;

    if (!WarpSectorInfo(sectnum, &sp_warp))
        return;

    if (!sp_warp)
        return;

    // move the the next wall
    wall_num = start_wall = sector[sectnum].wallptr;

    // Travel all the way around loop setting wall bits
    do
    {
        if (validWallIndex(wall[wall_num].nextwall))
            SET(wall[wall_num].cstat, CSTAT_WALL_WARP_HITSCAN);
        wall_num = wall[wall_num].point2;
    }
    while (wall_num != start_wall);
}

void ResetWallWarpHitscan(short sectnum)
{
    short start_wall, wall_num;

    // move the the next wall
    wall_num = start_wall = sector[sectnum].wallptr;

    // Travel all the way around loop setting wall bits
    do
    {
        RESET(wall[wall_num].cstat, CSTAT_WALL_WARP_HITSCAN);
        wall_num = wall[wall_num].point2;
    }
    while (wall_num != start_wall);
}

void
FAFhitscan(int32_t x, int32_t y, int32_t z, int16_t sectnum,
    int32_t xvect, int32_t yvect, int32_t zvect,
    HITINFO* hitinfo, int32_t clipmask)
{
    hitdata_t hitdata;
    FAFhitscan(x, y, z, sectnum, xvect, yvect, zvect, &hitdata, clipmask);
    hitinfo->set(&hitdata);
}


void
FAFhitscan(int32_t x, int32_t y, int32_t z, int16_t sectnum,
           int32_t xvect, int32_t yvect, int32_t zvect,
           hitdata_t* hitinfo, int32_t clipmask)
{
    vec3_t firstpos = { x, y, z };
    int loz, hiz;
    int newsectnum = sectnum;
    int startclipmask = 0;
    bool plax_found = false;

    if (clipmask == CLIPMASK_MISSILE)
        startclipmask = CLIPMASK_WARP_HITSCAN;

    hitscan(&firstpos, sectnum, xvect, yvect, zvect,
            hitinfo, startclipmask);

    if (hitinfo->sect < 0)
        return;

    if (hitinfo->wall >= 0)
    {
        // hitscan warping
        if (TEST(wall[hitinfo->wall].cstat, CSTAT_WALL_WARP_HITSCAN))
        {
            short dest_sect;

            MONO_PRINT(ds);

            // back it up a bit to get a correct warp location
            hitinfo->pos.x -= xvect>>9;
            hitinfo->pos.y -= yvect>>9;

            // warp to new x,y,z, sectnum
            if (Warp(&hitinfo->pos.x, &hitinfo->pos.y, &hitinfo->pos.z, &hitinfo->sect))
            {
                vec3_t pos = hitinfo->pos;

                dest_sect = hitinfo->sect;

                // hitscan needs to pass through dest sect
                ResetWallWarpHitscan(dest_sect);

                // NOTE: This could be recursive I think if need be
                hitscan(&pos, hitinfo->sect, xvect, yvect, zvect,
                        hitinfo, startclipmask);

                // reset hitscan block for dest sect
                SetWallWarpHitscan(dest_sect);

                return;
            }
            else
            {
                //DSPRINTF(ds,"hitinfo->pos.x %d, hitinfo->pos.y %d, hitinfo->pos.z %d",hitinfo->pos.x, hitinfo->pos.y, hitinfo->pos.z);
                MONO_PRINT(ds);
                ASSERT(true == false);
            }
        }
    }

    // make sure it hit JUST a sector before doing a check
    if (hitinfo->wall < 0 && hitinfo->sprite < 0)
    {
        if (TEST(sector[hitinfo->sect].extra, SECTFX_WARP_SECTOR))
        {
            if (TEST(wall[sector[hitinfo->sect].wallptr].cstat, CSTAT_WALL_WARP_HITSCAN))
            {
                // hit the floor of a sector that is a warping sector
                if (Warp(&hitinfo->pos.x, &hitinfo->pos.y, &hitinfo->pos.z, &hitinfo->sect))
                {
                    vec3_t pos = hitinfo->pos;
                    hitscan(&pos, hitinfo->sect, xvect, yvect, zvect,
                            hitinfo, clipmask);

                    return;
                }
            }
            else
            {
                if (WarpPlane(&hitinfo->pos.x, &hitinfo->pos.y, &hitinfo->pos.z, &hitinfo->sect))
                {
                    vec3_t pos = hitinfo->pos;
                    hitscan(&pos, hitinfo->sect, xvect, yvect, zvect,
                            hitinfo, clipmask);

                    return;
                }
            }
        }

        getzsofslope(hitinfo->sect, hitinfo->pos.x, hitinfo->pos.y, &hiz, &loz);
        if (labs(hitinfo->pos.z - loz) < Z(4))
        {
            if (FAF_ConnectFloor(hitinfo->sect) && !TEST(sector[hitinfo->sect].floorstat, FLOOR_STAT_FAF_BLOCK_HITSCAN))
            {
                updatesectorz(hitinfo->pos.x, hitinfo->pos.y, hitinfo->pos.z + Z(12), &newsectnum);
                plax_found = true;
            }
        }
        else if (labs(hitinfo->pos.z - hiz) < Z(4))
        {
            if (FAF_ConnectCeiling(hitinfo->sect) && !TEST(sector[hitinfo->sect].floorstat, CEILING_STAT_FAF_BLOCK_HITSCAN))
            {
                updatesectorz(hitinfo->pos.x, hitinfo->pos.y, hitinfo->pos.z - Z(12), &newsectnum);
                plax_found = true;
            }
        }
    }

    if (plax_found)
    {
        vec3_t pos = hitinfo->pos;
        hitscan(&pos, newsectnum, xvect, yvect, zvect,
                hitinfo, clipmask);
    }
}

bool FAFcansee(int32_t xs, int32_t ys, int32_t zs, int16_t sects,
          int32_t xe, int32_t ye, int32_t ze, int16_t secte)
{
    int loz, hiz;
    int newsectnum = sects;
    int xvect, yvect, zvect;
    short ang;
    hitdata_t hitinfo;
    int dist;
    bool plax_found = false;
    vec3_t s = { xs, ys, zs };

    // ASSERT(sects >= 0 && secte >= 0);

    // early out to regular routine
    if ((sects < 0 || !FAF_Sector(sects)) && (secte < 0 || !FAF_Sector(secte)))
    {
        return !!cansee(xs,ys,zs,sects,xe,ye,ze,secte);
    }

    // get angle
    ang = getangle(xe - xs, ye - ys);

    // get x,y,z, vectors
    xvect = bcos(ang);
    yvect = bsin(ang);

    // find the distance to the target
    dist = ksqrt(SQ(xe - xs) + SQ(ye - ys));

    if (dist != 0)
    {
        if (xe - xs != 0)
            zvect = Scale(xvect, ze - zs, xe - xs);
        else if (ye - ys != 0)
            zvect = Scale(yvect, ze - zs, ye - ys);
        else
            zvect = 0;
    }
    else
        zvect = 0;

    hitscan(&s, sects, xvect, yvect, zvect,
            &hitinfo, CLIPMASK_MISSILE);

    if (hitinfo.sect < 0)
        return false;

    // make sure it hit JUST a sector before doing a check
    if (hitinfo.wall < 0 && hitinfo.sprite < 0)
    {
        getzsofslope(hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, &hiz, &loz);
        if (labs(hitinfo.pos.z - loz) < Z(4))
        {
            if (FAF_ConnectFloor(hitinfo.sect))
            {
                updatesectorz(hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z + Z(12), &newsectnum);
                plax_found = true;
            }
        }
        else if (labs(hitinfo.pos.z - hiz) < Z(4))
        {
            if (FAF_ConnectCeiling(hitinfo.sect))
            {
                updatesectorz(hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z - Z(12), &newsectnum);
                plax_found = true;
            }
        }
    }
    else
    {
        return !!cansee(xs,ys,zs,sects,xe,ye,ze,secte);
    }

    if (plax_found)
        return !!cansee(hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,newsectnum,xe,ye,ze,secte);

    return false;
}


int GetZadjustment(short sectnum, short hitag)
{
    SPRITEp sp;

    if (sectnum < 0 || !TEST(sector[sectnum].extra, SECTFX_Z_ADJUST))
        return 0L;

    SWStatIterator it(STAT_ST1);
    while (auto itActor = it.Next())
    {
        sp = &itActor->s();

        if (sp->hitag == hitag && sp->sectnum == sectnum)
        {
            return Z(sp->lotag);
        }
    }

    return 0L;
}

bool SectorZadjust(const Collision& ceilhit, int32_t* hiz, const Collision& florhit, int32_t* loz)
{
    extern int PlaxCeilGlobZadjust, PlaxFloorGlobZadjust;
    int z_amt = 0;

    bool SkipFAFcheck = false;

    if (florhit.type != -1)
    {
        switch (florhit.type)
        {
        case kHitSector:
        {
            short hit_sector = florhit.index;

            // don't jack with connect sectors
            if (FAF_ConnectFloor(hit_sector))
            {
                // rippers were dying through the floor in $rock
                if (TEST(sector[hit_sector].floorstat, CEILING_STAT_FAF_BLOCK_HITSCAN))
                    break;

                if (TEST(sector[hit_sector].extra, SECTFX_Z_ADJUST))
                {
                    // see if a z adjust ST1 is around
                    z_amt = GetZadjustment(hit_sector, FLOOR_Z_ADJUST);

                    if (z_amt)
                    {
                        // explicit z adjust overrides Connect Floor
                        *loz += z_amt;
                        SkipFAFcheck = true;
                    }
                }

                break;
            }

            if (!TEST(sector[hit_sector].extra, SECTFX_Z_ADJUST))
                break;

            // see if a z adjust ST1 is around
            z_amt = GetZadjustment(hit_sector, FLOOR_Z_ADJUST);

            if (z_amt)
            {
                // explicit z adjust overrides plax default
                *loz += z_amt;
            }
            else
            // default adjustment for plax
            if (TEST(sector[hit_sector].floorstat, FLOOR_STAT_PLAX))
            {
                *loz += PlaxFloorGlobZadjust;
            }

            break;
        }
        }
    }

    if (ceilhit.type != -1)
    {
        switch (ceilhit.type)
        {
        case kHitSector:
        {
            short hit_sector = ceilhit.index;

            // don't jack with connect sectors
            if (FAF_ConnectCeiling(hit_sector))
            {
                if (TEST(sector[hit_sector].extra, SECTFX_Z_ADJUST))
                {
                    // see if a z adjust ST1 is around
                    z_amt = GetZadjustment(hit_sector, CEILING_Z_ADJUST);

                    if (z_amt)
                    {
                        // explicit z adjust overrides Connect Floor
                        *loz += z_amt;
                        SkipFAFcheck = true;
                    }
                }

                break;
            }

            if (!TEST(sector[hit_sector].extra, SECTFX_Z_ADJUST))
                break;

            // see if a z adjust ST1 is around
            z_amt = GetZadjustment(hit_sector, CEILING_Z_ADJUST);

            if (z_amt)
            {
                // explicit z adjust overrides plax default
                *hiz -= z_amt;
            }
            else
            // default adjustment for plax
            if (TEST(sector[hit_sector].ceilingstat, CEILING_STAT_PLAX))
            {
                *hiz -= PlaxCeilGlobZadjust;
            }

            break;
        }
        }
    }

    return SkipFAFcheck;
}

void WaterAdjust(const Collision& florhit, int32_t* loz)
{
    if (florhit.type == kHitSector)
    {
        SECT_USERp sectu = SectUser[florhit.index].Data();

        if (sectu && FixedToInt(sectu->depth_fixed))
            *loz += Z(FixedToInt(sectu->depth_fixed));
    }
}

static void getzrange(vec3_t* pos, int16_t sectnum, int32_t* hiz, Collision* ceilhit, int32_t* loz, Collision* florhit, int32_t clipdist, int32_t clipmask)
{
    int f, c;
    ::getzrange(pos, sectnum, hiz, &c, loz, &f, clipdist, clipmask);
    ceilhit->setFromEngine(c);
    florhit->setFromEngine(f);
}

void FAFgetzrange(vec3_t pos, int16_t sectnum, int32_t* hiz, Collision* ceilhit, int32_t* loz, Collision* florhit, int32_t clipdist, int32_t clipmask)
{
    int foo1;
    Collision foo2;
    bool SkipFAFcheck;
    Collision trash; trash.invalidate();

    // IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // This will return invalid FAF ceiling and floor heights inside of analyzesprite
    // because the ceiling and floors get moved out of the way for drawing.

    // early out to regular routine
    if (sectnum < 0 || !FAF_ConnectArea(sectnum))
    {
        getzrange(&pos, sectnum, hiz,  ceilhit, loz,  florhit, clipdist, clipmask);
        SectorZadjust(*ceilhit, hiz, *florhit, loz);
        WaterAdjust(*florhit, loz);
        return;
    }

    getzrange(&pos, sectnum, hiz,  ceilhit, loz,  florhit, clipdist, clipmask);
    SkipFAFcheck = SectorZadjust(*ceilhit, hiz, *florhit, loz);
    WaterAdjust(*florhit, loz);

    if (SkipFAFcheck)
        return;

    if (FAF_ConnectCeiling(sectnum))
    {
        int uppersect = sectnum;
        int newz = *hiz - Z(2);

        if (ceilhit->type == kHitSprite) return

        updatesectorz(pos.x, pos.y, newz, &uppersect);
        if (uppersect < 0)
            return; // _ErrMsg(ERR_STD_ARG, "Did not find a sector at %d, %d, %d", x, y, newz);
        vec3_t npos = pos;
        npos.z = newz;
        getzrange(&npos, uppersect, hiz,  ceilhit, &foo1,  &foo2, clipdist, clipmask);
        SectorZadjust(*ceilhit, hiz, trash, nullptr);
    }
    else if (FAF_ConnectFloor(sectnum) && !TEST(sector[sectnum].floorstat, FLOOR_STAT_FAF_BLOCK_HITSCAN))
    {
        int lowersect = sectnum;
        int newz = *loz + Z(2);

        if (florhit->type == kHitSprite) return

        updatesectorz(pos.x, pos.y, newz, &lowersect);
        if (lowersect < 0)
            return; // _ErrMsg(ERR_STD_ARG, "Did not find a sector at %d, %d, %d", x, y, newz);
        vec3_t npos = pos;
        npos.z = newz;
        getzrange(&npos, lowersect, &foo1,  &foo2, loz,  florhit, clipdist, clipmask);
        SectorZadjust(trash, nullptr, *florhit, loz);
        WaterAdjust(*florhit, loz);
    }
}

void FAFgetzrangepoint(int32_t x, int32_t y, int32_t z, int16_t sectnum,
                       int32_t* hiz, Collision* ceilhit,
                       int32_t* loz, Collision* florhit)
{
    int foo1;
    Collision foo2;
    bool SkipFAFcheck;
    Collision trash; trash.invalidate();

    // IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // This will return invalid FAF ceiling and floor heights inside of analyzesprite
    // because the ceiling and floors get moved out of the way for drawing.

    // early out to regular routine
    if (!FAF_ConnectArea(sectnum))
    {
        getzrangepoint(x, y, z, sectnum, hiz,  ceilhit, loz,  florhit);
        SectorZadjust(*ceilhit, hiz, *florhit, loz);
        WaterAdjust(*florhit, loz);
        return;
    }

    getzrangepoint(x, y, z, sectnum, hiz,  ceilhit, loz,  florhit);
    SkipFAFcheck = SectorZadjust(*ceilhit, hiz, *florhit, loz);
    WaterAdjust(*florhit, loz);

    if (SkipFAFcheck)
        return;

    if (FAF_ConnectCeiling(sectnum))
    {
        int uppersect = sectnum;
        int newz = *hiz - Z(2);
        if (ceilhit->type == kHitSprite)
            return;

        updatesectorz(x, y, newz, &uppersect);
        if (uppersect < 0)
            return; // _ErrMsg(ERR_STD_ARG, "Did not find a sector at %d, %d, %d, sectnum %d", x, y, newz, sectnum);
        getzrangepoint(x, y, newz, uppersect, hiz,  ceilhit, &foo1,  &foo2);
        SectorZadjust(*ceilhit, hiz, trash, nullptr);
    }
    else if (FAF_ConnectFloor(sectnum) && !TEST(sector[sectnum].floorstat, FLOOR_STAT_FAF_BLOCK_HITSCAN))
    //if (FAF_ConnectFloor(sectnum))
    {
        int lowersect = sectnum;
        int newz = *loz + Z(2);
        if (florhit->type == kHitSprite)
            return;
        updatesectorz(x, y, newz, &lowersect);
        if (lowersect < 0)
            return; // _ErrMsg(ERR_STD_ARG, "Did not find a sector at %d, %d, %d, sectnum %d", x, y, newz, sectnum);
        getzrangepoint(x, y, newz, lowersect, &foo1,  &foo2, loz,  florhit);
        SectorZadjust(trash, nullptr, *florhit, loz);
        WaterAdjust(*florhit, loz);
    }
}

void
SetupMirrorTiles(void)
{
    SPRITEp sp;

    SWStatIterator it(STAT_FAF);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (sector[sp->sectnum].ceilingpicnum == FAF_PLACE_MIRROR_PIC)
        {
            sector[sp->sectnum].ceilingpicnum = FAF_MIRROR_PIC;
            SET(sector[sp->sectnum].ceilingstat, CEILING_STAT_PLAX);
        }

        if (sector[sp->sectnum].floorpicnum == FAF_PLACE_MIRROR_PIC)
        {
            sector[sp->sectnum].floorpicnum = FAF_MIRROR_PIC;
            SET(sector[sp->sectnum].floorstat, FLOOR_STAT_PLAX);
        }

        if (sector[sp->sectnum].ceilingpicnum == FAF_PLACE_MIRROR_PIC+1)
            sector[sp->sectnum].ceilingpicnum = FAF_MIRROR_PIC+1;

        if (sector[sp->sectnum].floorpicnum == FAF_PLACE_MIRROR_PIC+1)
            sector[sp->sectnum].floorpicnum = FAF_MIRROR_PIC+1;
    }
}

void GetUpperLowerSector(short match, int x, int y, short *upper, short *lower)
{
    int i;
    int sectorlist[16];
    int sln = 0;
    SPRITEp sp;

    for (i = 0; i < numsectors; i++)// - 1; i >= 0; i--)
    {
        if (inside(x, y, (short) i) == 1)
        {
            bool found = false;

            SWSectIterator it(i);
            while (auto actor = it.Next())
            {
                sp = &actor->s();

                if (sp->statnum == STAT_FAF &&
                    (sp->hitag >= VIEW_LEVEL1 && sp->hitag <= VIEW_LEVEL6)
                    && sp->lotag == match)
                {
                    found = true;
                }
            }

            if (!found)
                continue;
            if (sln < (int)SIZ(sectorlist))
                sectorlist[sln] = i;
            sln++;
        }
    }

    // might not find ANYTHING if not tagged right
    if (sln == 0)
    {
        *upper = -1;
        *lower = -1;
        return;
    }
    // Map rooms have NOT been dragged on top of each other
    else if (sln == 1)
    {
        *lower = sectorlist[0];
        *upper = sectorlist[0];
        return;
    }
    // Map rooms HAVE been dragged on top of each other
    // inside will somtimes find that you are in two different sectors if the x,y
    // is exactly on a sector line.
    else if (sln > 2)
    {
        // try again moving the x,y pos around until you only get two sectors
        GetUpperLowerSector(match, x - 1, y, upper, lower);
    }

    if (sln == 2)
    {
        if (sector[sectorlist[0]].floorz < sector[sectorlist[1]].floorz)
        {
            // swap
            // make sectorlist[0] the LOW sector
            short hold;

            hold = sectorlist[0];
            sectorlist[0] = sectorlist[1];
            sectorlist[1] = hold;
        }

        *lower = sectorlist[0];
        *upper = sectorlist[1];
    }
}

bool FindCeilingView(short match, int32_t* x, int32_t* y, int32_t z, int16_t* sectnum)
{
    int xoff = 0;
    int yoff = 0;
    SPRITEp sp = nullptr;
    int pix_diff;
    int newz;

    save.zcount = 0;

    // Search Stat List For closest ceiling view sprite
    // Get the match, xoff, yoff from this point
    SWStatIterator it(STAT_FAF);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (sp->hitag == VIEW_THRU_CEILING && sp->lotag == match)
        {
            xoff = *x - sp->x;
            yoff = *y - sp->y;
            break;
        }
    }

    it.Reset(STAT_FAF);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (sp->lotag == match)
        {
            // determine x,y position
            if (sp->hitag == VIEW_THRU_FLOOR)
            {
                short upper, lower;

                *x = sp->x + xoff;
                *y = sp->y + yoff;

                // get new sector
                GetUpperLowerSector(match, *x, *y, &upper, &lower);
                *sectnum = upper;
                break;
            }
        }
    }

    if (*sectnum < 0)
        return false;

    if (!sp || sp->hitag != VIEW_THRU_FLOOR)
    {
        *sectnum = 0;
        return false;
    }


    if (!testnewrenderer)
    {
        pix_diff = labs(z - sector[sp->sectnum].floorz) >> 8;
        newz = sector[sp->sectnum].floorz + ((pix_diff / 128) + 1) * Z(128);

        it.Reset(STAT_FAF);
        while (auto actor = it.Next())
        {
            sp = &actor->s();

            if (sp->lotag == match)
            {
                // move lower levels ceilings up for the correct view
                if (sp->hitag == VIEW_LEVEL2)
                {
                    // save it off
                    save.sectnum[save.zcount] = sp->sectnum;
                    save.zval[save.zcount] = sector[sp->sectnum].floorz;
                    save.pic[save.zcount] = sector[sp->sectnum].floorpicnum;
                    save.slope[save.zcount] = sector[sp->sectnum].floorheinum;

                    sector[sp->sectnum].floorz = newz;
                    // don't change FAF_MIRROR_PIC - ConnectArea
                    if (sector[sp->sectnum].floorpicnum != FAF_MIRROR_PIC)
                        sector[sp->sectnum].floorpicnum = FAF_MIRROR_PIC + 1;
                    sector[sp->sectnum].floorheinum = 0;

                    save.zcount++;
                    PRODUCTION_ASSERT(save.zcount < ZMAX);
                }
            }
        }
    }

    return true;
}

bool FindFloorView(short match, int32_t* x, int32_t* y, int32_t z, int16_t* sectnum)
{
    int xoff = 0;
    int yoff = 0;
    SPRITEp sp = nullptr;
    int newz;
    int pix_diff;

    save.zcount = 0;

    // Search Stat List For closest ceiling view sprite
    // Get the match, xoff, yoff from this point
    SWStatIterator it(STAT_FAF);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (sp->hitag == VIEW_THRU_FLOOR && sp->lotag == match)
        {
            xoff = *x - sp->x;
            yoff = *y - sp->y;
            break;
        }
    }


    it.Reset(STAT_FAF);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (sp->lotag == match)
        {
            // determine x,y position
            if (sp->hitag == VIEW_THRU_CEILING)
            {
                short upper, lower;

                *x = sp->x + xoff;
                *y = sp->y + yoff;

                // get new sector
                GetUpperLowerSector(match, *x, *y, &upper, &lower);
                *sectnum = lower;
                break;
            }
        }
    }

    if (*sectnum < 0)
        return false;

    if (!sp || sp->hitag != VIEW_THRU_CEILING)
    {
        *sectnum = 0;
        return false;
    }

    if (!testnewrenderer)
    {
        // move ceiling multiple of 128 so that the wall tile will line up
        pix_diff = labs(z - sector[sp->sectnum].ceilingz) >> 8;
        newz = sector[sp->sectnum].ceilingz - ((pix_diff / 128) + 1) * Z(128);

        it.Reset(STAT_FAF);
        while (auto actor = it.Next())
        {
            sp = &actor->s();

            if (sp->lotag == match)
            {
                // move upper levels floors down for the correct view
                if (sp->hitag == VIEW_LEVEL1)
                {
                    // save it off
                    save.sectnum[save.zcount] = sp->sectnum;
                    save.zval[save.zcount] = sector[sp->sectnum].ceilingz;
                    save.pic[save.zcount] = sector[sp->sectnum].ceilingpicnum;
                    save.slope[save.zcount] = sector[sp->sectnum].ceilingheinum;

                    sector[sp->sectnum].ceilingz = newz;

                    // don't change FAF_MIRROR_PIC - ConnectArea
                    if (sector[sp->sectnum].ceilingpicnum != FAF_MIRROR_PIC)
                        sector[sp->sectnum].ceilingpicnum = FAF_MIRROR_PIC + 1;
                    sector[sp->sectnum].ceilingheinum = 0;

                    save.zcount++;
                    PRODUCTION_ASSERT(save.zcount < ZMAX);
                }
            }
        }
    }
    return true;
}

short FindViewSectorInScene(short cursectnum, short level)
{
    SPRITEp sp;
    short match;

    SWStatIterator it(STAT_FAF);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (sp->hitag == level)
        {
            if (cursectnum == sp->sectnum)
            {
                // ignore case if sprite is pointing up
                if (sp->ang == 1536)
                    continue;

                // only gets to here is sprite is pointing down

                // found a potential match
                match = sp->lotag;

                return match;
            }
        }
    }

    return -1;
}

struct PortalGroup
{
    TArray<int> sectors;
    int othersector = -1;
    vec3_t offset = { 0,0,0 };
};

    // This is very messy because some portals are linked outside the actual portal sectors, so we have to use the complicated original linking logic to find the connection. :?
void CollectPortals()
{
    int t = testnewrenderer;
    testnewrenderer = true;
    TArray<PortalGroup> floorportals;
    TArray<PortalGroup> ceilingportals;
    BitArray floordone(numsectors), ceilingdone(numsectors);

    for (int i = 0; i < numsectors; i++)
    {
        sector[i].portalflags = sector[i].portalnum = 0;
    }
    floordone.Zero();
    ceilingdone.Zero();
    portalClear();

    for (int i = 0; i < numsectors; i++)
    {
        if (sector[i].floorpicnum == FAF_MIRROR_PIC && !floordone[i])
        {
            auto& fp = floorportals[floorportals.Reserve(1)];
            fp.sectors.Push(i);
            floordone.Set(i);
            for (unsigned ii = 0; ii < fp.sectors.Size(); ii++)
            {
                auto sec = &sector[fp.sectors[ii]];
                for (int w = 0; w < sec->wallnum; w++)
                {
                    auto ns = wall[sec->wallptr + w].nextsector;
                    if (ns < 0 || floordone[ns] || sector[ns].floorpicnum != FAF_MIRROR_PIC) continue;
                    fp.sectors.Push(ns);
                    floordone.Set(ns);
                }
            }
        }
        if (sector[i].ceilingpicnum == FAF_MIRROR_PIC && !ceilingdone[i])
        {
            auto& fp = ceilingportals[ceilingportals.Reserve(1)];
            fp.sectors.Push(i);
            ceilingdone.Set(i);
            for (unsigned ii = 0; ii < fp.sectors.Size(); ii++)
            {
                auto sec = &sector[fp.sectors[ii]];
                for (int w = 0; w < sec->wallnum; w++)
                {
                    auto ns = wall[sec->wallptr + w].nextsector;
                    if (ns < 0 || ceilingdone[ns] || sector[ns].ceilingpicnum != FAF_MIRROR_PIC) continue;
                    fp.sectors.Push(ns);
                    ceilingdone.Set(ns);
                }
            }
        }
    }
    // now try to find connections.
    for (auto& fp : ceilingportals)
    {
        // pick one sprite out of the sectors, repeat until we get a valid connection
        for (auto sec : fp.sectors)
        {
            SWSectIterator it(sec);
            while (auto actor = it.Next())
            {
                int tx = actor->s().x;
                int ty = actor->s().y;
                int tz = actor->s().z;
                int16_t tsectnum = sec;

                int match = FindViewSectorInScene(tsectnum, VIEW_LEVEL1);
                if (match != -1)
                {
                    FindCeilingView(match, &tx, &ty, tz, &tsectnum);
                    if (tsectnum >= 0 && sector[tsectnum].floorpicnum == FAF_MIRROR_PIC)
                    {
                        // got something!
                        fp.othersector = tsectnum;
                        fp.offset = { tx, ty, tz };
                        fp.offset -= actor->s().pos;
                        goto nextfg;
                    }
                }
            }
        }
    nextfg:;
    }

    for (auto& fp : floorportals)
    {
        for (auto sec : fp.sectors)
        {
            SWSectIterator it(sec);
            while (auto actor = it.Next())
            {
                int tx = actor->s().x;
                int ty = actor->s().y;
                int tz = actor->s().z;
                int16_t tsectnum = sec;

                int match = FindViewSectorInScene(tsectnum, VIEW_LEVEL2);
                if (match != -1)
                {
                    FindFloorView(match, &tx, &ty, tz, &tsectnum);
                    if (tsectnum >= 0 && sector[tsectnum].ceilingpicnum == FAF_MIRROR_PIC)
                    {
                        // got something!
                        fp.othersector = tsectnum;
                        fp.offset = { tx, ty, tz };
                        fp.offset -= actor->s().pos;
                        goto nextcg;
                    }
                }
            }
        }
    nextcg:;
    }
    for (auto& pt : floorportals)
    {
        if (pt.othersector > -1)
        {
            auto findother = [&](int other) -> PortalGroup*
            {
                for (auto& pt2 : ceilingportals)
                {
                    if (pt2.sectors.Find(other) != pt2.sectors.Size()) return &pt2;
                }
                return nullptr;
            };

            auto pt2 = findother(pt.othersector);
            if (pt2)
            {
                int pnum = portalAdd(PORTAL_SECTOR_FLOOR, -1, pt.offset.x, pt.offset.y, 0);
                allPortals[pnum].targets = pt2->sectors; // do not move! We still need the original.
                for (auto sec : pt.sectors)
                {
                    sector[sec].portalflags = PORTAL_SECTOR_FLOOR;
                    sector[sec].portalnum = pnum;
                }
            }
        }
    }
    for (auto& pt : ceilingportals)
    {
        if (pt.othersector > -1)
        {
            auto findother = [&](int other) -> PortalGroup*
            {
                for (auto& pt2 : floorportals)
                {
                    if (pt2.sectors.Find(other) != pt2.sectors.Size()) return &pt2;
                }
                return nullptr;
            };

            auto pt2 = findother(pt.othersector);
            if (pt2)
            {
                int pnum = portalAdd(PORTAL_SECTOR_CEILING, -1, pt.offset.x, pt.offset.y, 0);
                allPortals[pnum].targets = std::move(pt2->sectors);
                for (auto sec : pt.sectors)
                {
                    sector[sec].portalflags = PORTAL_SECTOR_CEILING;
                    sector[sec].portalnum = pnum;
                }
            }
        }
    }
    testnewrenderer = t;
}


END_SW_NS
