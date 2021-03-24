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

void COVERupdatesector(int32_t x, int32_t y, int16_t* newsector)
{
    // ASSERT(*newsector>=0 && *newsector<MAXSECTORS);
    updatesector(x,y,newsector);
}

int COVERinsertsprite(short sectnum, short stat)
{
    short spnum;
    spnum = insertsprite(sectnum, stat);

    PRODUCTION_ASSERT(spnum >= 0);

    sprite[spnum].x = sprite[spnum].y = sprite[spnum].z = 0;
    sprite[spnum].cstat = 0;
    sprite[spnum].picnum = 0;
    sprite[spnum].shade = 0;
    sprite[spnum].pal = 0;
    sprite[spnum].clipdist = 0;
    sprite[spnum].xrepeat = sprite[spnum].yrepeat = 0;
    sprite[spnum].xoffset = sprite[spnum].yoffset = 0;
    sprite[spnum].ang = 0;
    sprite[spnum].owner = -1;
    sprite[spnum].xvel = sprite[spnum].yvel = sprite[spnum].zvel = 0;
    sprite[spnum].lotag = 0;
    sprite[spnum].hitag = 0;
    sprite[spnum].extra = 0;

    return spnum;
}

bool
FAF_Sector(short sectnum)
{
    int SpriteNum;
    SPRITEp sp;

    SectIterator it(sectnum);
    while ((SpriteNum = it.NextIndex()) >= 0)
    {
        sp = &sprite[SpriteNum];

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
    SPRITEp sp_warp;

    if (!WarpSectorInfo(sectnum, &sp_warp))
        return;

    if (!sp_warp)
        return;

    // move the the next wall
    wall_num = start_wall = sector[sectnum].wallptr;

    // Travel all the way around loop setting wall bits
    do
    {
        if ((uint16_t)wall[wall_num].nextwall < MAXWALLS)
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
           hitdata_t* hitinfo, int32_t clipmask)
{
    vec3_t firstpos = { x, y, z };
    int loz, hiz;
    short newsectnum = sectnum;
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

bool
FAFcansee(int32_t xs, int32_t ys, int32_t zs, int16_t sects,
          int32_t xe, int32_t ye, int32_t ze, int16_t secte)
{
    int loz, hiz;
    short newsectnum = sects;
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


int
GetZadjustment(short sectnum, short hitag)
{
    int i;
    SPRITEp sp;

    if (sectnum < 0 || !TEST(sector[sectnum].extra, SECTFX_Z_ADJUST))
        return 0L;

    StatIterator it(STAT_ST1);
    while ((i = it.NextIndex()) >= 0)
    {
        sp = &sprite[i];

        if (sp->hitag == hitag && sp->sectnum == sectnum)
        {
            return Z(sp->lotag);
        }
    }

    return 0L;
}

bool SectorZadjust(int ceilhit, int32_t* hiz, short florhit, int32_t* loz)
{
    extern int PlaxCeilGlobZadjust, PlaxFloorGlobZadjust;
    int z_amt = 0;

    bool SkipFAFcheck = false;

    if ((int)florhit != -1)
    {
        switch (TEST(florhit, HIT_MASK))
        {
        case HIT_SECTOR:
        {
            short hit_sector = NORM_SECTOR(florhit);

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

    if ((int)ceilhit != -1)
    {
        switch (TEST(ceilhit, HIT_MASK))
        {
        case HIT_SECTOR:
        {
            short hit_sector = NORM_SECTOR(ceilhit);

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

void WaterAdjust(short florhit, int32_t* loz)
{
    switch (TEST(florhit, HIT_MASK))
    {
    case HIT_SECTOR:
    {
        SECT_USERp sectu = SectUser[NORM_SECTOR(florhit)];

        if (sectu && sectu->depth)
            *loz += Z(sectu->depth);
    }
    break;
    case HIT_SPRITE:
        break;
    }
}

void FAFgetzrange(int32_t x, int32_t y, int32_t z, int16_t sectnum,
                  int32_t* hiz, int32_t* ceilhit,
                  int32_t* loz, int32_t* florhit,
                  int32_t clipdist, int32_t clipmask)
{
    int foo1;
    int foo2;
    bool SkipFAFcheck;

    // IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // This will return invalid FAF ceiling and floor heights inside of analyzesprite
    // because the ceiling and floors get moved out of the way for drawing.

    // early out to regular routine
    if (sectnum < 0 || !FAF_ConnectArea(sectnum))
    {
        getzrange_old(x, y, z, sectnum, hiz,  ceilhit, loz,  florhit, clipdist, clipmask);
        SectorZadjust(*ceilhit, hiz, *florhit, loz);
        WaterAdjust(*florhit, loz);
        return;
    }

    getzrange_old(x, y, z, sectnum, hiz,  ceilhit, loz,  florhit, clipdist, clipmask);
    SkipFAFcheck = SectorZadjust(*ceilhit, hiz, *florhit, loz);
    WaterAdjust(*florhit, loz);

    if (SkipFAFcheck)
        return;

    if (FAF_ConnectCeiling(sectnum))
    {
        short uppersect = sectnum;
        int newz = *hiz - Z(2);

        switch (TEST(*ceilhit, HIT_MASK))
        {
        case HIT_SPRITE:
            return;
        }

        updatesectorz(x, y, newz, &uppersect);
        if (uppersect < 0)
            return; // _ErrMsg(ERR_STD_ARG, "Did not find a sector at %d, %d, %d", x, y, newz);
        getzrange_old(x, y, newz, uppersect, hiz,  ceilhit, &foo1,  &foo2, clipdist, clipmask);
        SectorZadjust(*ceilhit, hiz, -1, NULL);
    }
    else if (FAF_ConnectFloor(sectnum) && !TEST(sector[sectnum].floorstat, FLOOR_STAT_FAF_BLOCK_HITSCAN))
    //if (FAF_ConnectFloor(sectnum))
    {
        short lowersect = sectnum;
        int newz = *loz + Z(2);

        switch (TEST(*florhit, HIT_MASK))
        {
        case HIT_SECTOR:
        {
            break;
        }
        case HIT_SPRITE:
            return;
        }

        updatesectorz(x, y, newz, &lowersect);
        if (lowersect < 0)
            return; // _ErrMsg(ERR_STD_ARG, "Did not find a sector at %d, %d, %d", x, y, newz);
        getzrange_old(x, y, newz, lowersect, &foo1,  &foo2, loz,  florhit, clipdist, clipmask);
        SectorZadjust(-1, NULL, *florhit, loz);
        WaterAdjust(*florhit, loz);
    }
}

void FAFgetzrangepoint(int32_t x, int32_t y, int32_t z, int16_t sectnum,
                       int32_t* hiz, int32_t* ceilhit,
                       int32_t* loz, int32_t* florhit)
{
    int foo1;
    int foo2;
    bool SkipFAFcheck;

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
        short uppersect = sectnum;
        int newz = *hiz - Z(2);
        switch (TEST(*ceilhit, HIT_MASK))
        {
        case HIT_SPRITE:
            return;
        }
        updatesectorz(x, y, newz, &uppersect);
        if (uppersect < 0)
            return; // _ErrMsg(ERR_STD_ARG, "Did not find a sector at %d, %d, %d, sectnum %d", x, y, newz, sectnum);
        getzrangepoint(x, y, newz, uppersect, hiz,  ceilhit, &foo1,  &foo2);
        SectorZadjust(*ceilhit, hiz, -1, NULL);
    }
    else if (FAF_ConnectFloor(sectnum) && !TEST(sector[sectnum].floorstat, FLOOR_STAT_FAF_BLOCK_HITSCAN))
    //if (FAF_ConnectFloor(sectnum))
    {
        short lowersect = sectnum;
        int newz = *loz + Z(2);
        switch (TEST(*florhit, HIT_MASK))
        {
        case HIT_SPRITE:
            return;
        }
        updatesectorz(x, y, newz, &lowersect);
        if (lowersect < 0)
            return; // _ErrMsg(ERR_STD_ARG, "Did not find a sector at %d, %d, %d, sectnum %d", x, y, newz, sectnum);
        getzrangepoint(x, y, newz, lowersect, &foo1,  &foo2, loz,  florhit);
        SectorZadjust(-1, NULL, *florhit, loz);
        WaterAdjust(*florhit, loz);
    }
}

// doesn't work for blank pics
bool
PicInView(short tile_num, bool reset)
{
    if (TEST(gotpic[tile_num >> 3], 1 << (tile_num & 7)))
    {
        if (reset)
            RESET(gotpic[tile_num >> 3], 1 << (tile_num & 7));

        return true;
    }

    return false;
}

void
SetupMirrorTiles(void)
{
    int i;
    SPRITEp sp;

    StatIterator it(STAT_FAF);
    while ((i = it.NextIndex()) >= 0)
    {
        sp = &sprite[i];

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


void SetupSectorPortals()
{
    TArray<int> foundf, foundc;
    // Search Stat List For closest ceiling view sprite
    // Get the match, xoff, yoff from this point
    StatIterator it(STAT_FAF);
    int i;
    while ((i = it.NextIndex()) >= 0)
    {
        auto sp = &sprite[i];

        if (sp->hitag == VIEW_THRU_CEILING) foundc.Push(i);
        if (sp->hitag == VIEW_THRU_FLOOR) foundf.Push(i);
    }

    portalClear();
    while (foundf.Size())
    {
        auto spf = &sprite[foundf[0]];
        auto cindex = foundc.FindEx([=](int i) { return spf->lotag == sprite[i].lotag; });
        if (cindex != foundc.Size())
        {
            auto spc = &sprite[foundf[cindex]];
            sector[spf->sectnum].portalflags = PORTAL_SECTOR_FLOOR;
            sector[spf->sectnum].portalnum = portalAdd(PORTAL_SECTOR_FLOOR, spc->sectnum, spc->x - spf->x, spc->y - spf->y, 0);

            sector[spc->sectnum].portalflags = PORTAL_SECTOR_CEILING;
            sector[spc->sectnum].portalnum = portalAdd(PORTAL_SECTOR_CEILING, spf->sectnum, spf->x - spc->x, spf->y - spc->y, 0);

            //Printf("Portal with tag %d\n", sprite[foundf[0]].lotag);
            foundf.Delete(0);
            foundc.Delete(cindex);
        }
        else
        {
            //Printf("Floor portal %d without partner\n", sprite[foundf[0]].lotag);
            foundf.Delete(0);
        }
    }
    for (auto c : foundc)
    {
        //Printf("Ceiling portal %d without partner\n", sprite[c].lotag);
    }
}

END_SW_NS
