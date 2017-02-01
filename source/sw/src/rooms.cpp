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
#include "build.h"

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "warp.h"

void _ErrMsg(char *strFile, unsigned uLine, char *format, ...);
void FAF_DrawRooms(int posx, int posy, int posz, short ang, int horiz, short cursectnum);

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

SWBOOL FAF_DebugView = 0;

void COVERupdatesector(int32_t x, int32_t y, int16_t* newsector)
{
    ASSERT(*newsector>=0 && *newsector<MAXSECTORS);
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

SWBOOL
FAF_Sector(short sectnum)
{
    short SpriteNum, Next;
    SPRITEp sp;
    SWBOOL found = FALSE;

    TRAVERSE_SPRITE_SECT(headspritesect[sectnum], SpriteNum, Next)
    {
        sp = &sprite[SpriteNum];

        if (sp->statnum == STAT_FAF &&
            (sp->hitag >= VIEW_LEVEL1 && sp->hitag <= VIEW_LEVEL6))
        {
            return TRUE;
        }
    }

    return FALSE;
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
        if (wall[wall_num].nextwall >= 0)
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
    SWBOOL plax_found = FALSE;

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
            short src_sect = hitinfo->sect;
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
                ASSERT(TRUE == FALSE);
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
                plax_found = TRUE;
            }
        }
        else if (labs(hitinfo->pos.z - hiz) < Z(4))
        {
            if (FAF_ConnectCeiling(hitinfo->sect) && !TEST(sector[hitinfo->sect].floorstat, CEILING_STAT_FAF_BLOCK_HITSCAN))
            {
                updatesectorz(hitinfo->pos.x, hitinfo->pos.y, hitinfo->pos.z - Z(12), &newsectnum);
                plax_found = TRUE;
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

SWBOOL
FAFcansee(int32_t xs, int32_t ys, int32_t zs, int16_t sects,
          int32_t xe, int32_t ye, int32_t ze, int16_t secte)
{
    int loz, hiz;
    short newsectnum = sects;
    int xvect, yvect, zvect;
    short ang;
    hitdata_t hitinfo;
    int dist;
    SWBOOL plax_found = FALSE;
    vec3_t s = { xs, ys, zs };

    ASSERT(sects >= 0 && secte >= 0);

    // early out to regular routine
    if (!FAF_Sector(sects) && !FAF_Sector(secte))
    {
        return cansee(xs,ys,zs,sects,xe,ye,ze,secte);
    }

    // get angle
    ang = getangle(xe - xs, ye - ys);

    // get x,y,z, vectors
    xvect = sintable[NORM_ANGLE(ang + 512)];
    yvect = sintable[NORM_ANGLE(ang)];

    // find the distance to the target
    dist = ksqrt(SQ(xe - xs) + SQ(ye - ys));

    if (dist != 0)
    {
        if (xe - xs != 0)
            zvect = scale(xvect, ze - zs, xe - xs);
        else if (ye - ys != 0)
            zvect = scale(yvect, ze - zs, ye - ys);
        else
            zvect = 0;
    }
    else
        zvect = 0;

    hitscan(&s, sects, xvect, yvect, zvect,
            &hitinfo, CLIPMASK_MISSILE);

    if (hitinfo.sect < 0)
        return FALSE;

    // make sure it hit JUST a sector before doing a check
    if (hitinfo.wall < 0 && hitinfo.sprite < 0)
    {
        getzsofslope(hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, &hiz, &loz);
        if (labs(hitinfo.pos.z - loz) < Z(4))
        {
            if (FAF_ConnectFloor(hitinfo.sect))
            {
                updatesectorz(hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z + Z(12), &newsectnum);
                plax_found = TRUE;
            }
        }
        else if (labs(hitinfo.pos.z - hiz) < Z(4))
        {
            if (FAF_ConnectCeiling(hitinfo.sect))
            {
                updatesectorz(hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z - Z(12), &newsectnum);
                plax_found = TRUE;
            }
        }
    }
    else
    {
        return cansee(xs,ys,zs,sects,xe,ye,ze,secte);
    }

    if (plax_found)
        return cansee(hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,newsectnum,xe,ye,ze,secte);

    return FALSE;
}


int
GetZadjustment(short sectnum, short hitag)
{
    short i, nexti;
    SPRITEp sp;

    if (!TEST(sector[sectnum].extra, SECTFX_Z_ADJUST))
        return 0L;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_ST1], i, nexti)
    {
        sp = &sprite[i];

        if (sp->hitag == hitag && sp->sectnum == sectnum)
        {
            return Z(sp->lotag);
        }
    }

    return 0L;
}

SWBOOL SectorZadjust(int ceilhit, int32_t* hiz, short florhit, int32_t* loz)
{
    extern int PlaxCeilGlobZadjust, PlaxFloorGlobZadjust;
    int z_amt = 0;

    SWBOOL SkipFAFcheck = FALSE;

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
                        SkipFAFcheck = TRUE;
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
                        SkipFAFcheck = TRUE;
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
    SWBOOL SkipFAFcheck;

    // IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // This will return invalid FAF ceiling and floor heights inside of analyzesprite
    // because the ceiling and floors get moved out of the way for drawing.

    // early out to regular routine
    if (!FAF_ConnectArea(sectnum))
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
            _ErrMsg(ERR_STD_ARG, "Did not find a sector at %d, %d, %d", x, y, newz);
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
            short hit_sector = NORM_SECTOR(*florhit);
            break;
        }
        case HIT_SPRITE:
            return;
        }

        updatesectorz(x, y, newz, &lowersect);
        if (lowersect < 0)
            _ErrMsg(ERR_STD_ARG, "Did not find a sector at %d, %d, %d", x, y, newz);
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
    SWBOOL SkipFAFcheck;

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
            _ErrMsg(ERR_STD_ARG, "Did not find a sector at %d, %d, %d, sectnum %d", x, y, newz, sectnum);
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
            _ErrMsg(ERR_STD_ARG, "Did not find a sector at %d, %d, %d, sectnum %d", x, y, newz, sectnum);
        getzrangepoint(x, y, newz, lowersect, &foo1,  &foo2, loz,  florhit);
        SectorZadjust(-1, NULL, *florhit, loz);
        WaterAdjust(*florhit, loz);
    }
}

#if 0
SWBOOL
FAF_ConnectCeiling(short sectnum)
{
    return sector[sectnum].ceilingpicnum == FAF_MIRROR_PIC;
}

SWBOOL
FAF_ConnectFloor(short sectnum)
{
    return sector[sectnum].floorpicnum == FAF_MIRROR_PIC;
}
#endif


// doesn't work for blank pics
SWBOOL
PicInView(short tile_num, SWBOOL reset)
{
    if (TEST(gotpic[tile_num >> 3], 1 << (tile_num & 7)))
    {
        if (reset)
            RESET(gotpic[tile_num >> 3], 1 << (tile_num & 7));

        return TRUE;
    }

    return FALSE;
}

void
SetupMirrorTiles(void)
{
    short i, nexti;
    short j, nextj;
    SPRITEp sp;
    SWBOOL found;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], i, nexti)
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


//This function is like updatesector, but it takes a z-coordinate in addition
//   to help it get the right sector when there's overlapping.  (I may be
//   adding this function to the engine or making the standard updatesector
//   use z's.  Until then, use this.  )

#if 0
void
updatesectorz(int x, int y, int z, short *sectnum)
{
    walltype *wal;
    int i, j, cz, fz;

    ASSERT(*sectnum >=0 && *sectnum <= MAXSECTORS);

    getzsofslope(*sectnum, x, y, &cz, &fz);
    // go ahead and check the current sector
    if ((z >= cz) && (z <= fz))
        if (inside(x, y, *sectnum) != 0)
            return;

    // Test the sectors immediately around your current sector
    if ((*sectnum >= 0) && (*sectnum < numsectors))
    {
        wal = &wall[sector[*sectnum].wallptr];
        j = sector[*sectnum].wallnum;
        do
        {
            i = wal->nextsector;
            if (i >= 0)
            {
                getzsofslope(i, x, y, &cz, &fz);
                if ((z >= cz) && (z <= fz))
                {
                    if (inside(x, y, (short) i) == 1)
                    {
                        *sectnum = i;
                        return;
                    }
                }
            }
            wal++;
            j--;
        }
        while (j != 0);
    }

    // didn't find it yet so test ALL sectors
    for (i = numsectors - 1; i >= 0; i--)
    {
        getzsofslope(i, x, y, &cz, &fz);
        if ((z >= cz) && (z <= fz))
        {
            if (inside(x, y, (short) i) == 1)
            {
                *sectnum = i;
                return;
            }
        }
    }

    *sectnum = -1;
}
#endif

short GlobStackSect[2];

void
GetUpperLowerSector(short match, int x, int y, short *upper, short *lower)
{
    int i, j;
    short sectorlist[16];
    int sln = 0;
    short SpriteNum, Next;
    SPRITEp sp;

    // keep a list of the last stacked sectors the view was in and
    // check those fisrt
    sln = 0;
    for (i = 0; i < (int)SIZ(GlobStackSect); i++)
    {
        // will not hurt if GlobStackSect is invalid - inside checks for this
        if (inside(x, y, GlobStackSect[i]) == 1)
        {
            SWBOOL found = FALSE;

            TRAVERSE_SPRITE_SECT(headspritesect[GlobStackSect[i]], SpriteNum, Next)
            {
                sp = &sprite[SpriteNum];

                if (sp->statnum == STAT_FAF &&
                    (sp->hitag >= VIEW_LEVEL1 && sp->hitag <= VIEW_LEVEL6)
                    && sp->lotag == match)
                {
                    found = TRUE;
                }
            }

            if (!found)
                continue;

            sectorlist[sln] = GlobStackSect[i];
            sln++;
        }
    }

    // didn't find it yet so test ALL sectors
    if (sln < 2)
    {
        sln = 0;
        for (i = numsectors - 1; i >= 0; i--)
        {
            if (inside(x, y, (short) i) == 1)
            {
                SWBOOL found = FALSE;

                TRAVERSE_SPRITE_SECT(headspritesect[i], SpriteNum, Next)
                {
                    sp = &sprite[SpriteNum];

                    if (sp->statnum == STAT_FAF &&
                        (sp->hitag >= VIEW_LEVEL1 && sp->hitag <= VIEW_LEVEL6)
                        && sp->lotag == match)
                    {
                        found = TRUE;
                    }
                }

                if (!found)
                    continue;

                sectorlist[sln] = i;
                if (sln < (int)SIZ(GlobStackSect))
                    GlobStackSect[sln] = i;
                sln++;
            }
        }
    }

    // might not find ANYTHING if not tagged right
    if (sln == 0)
    {
        *upper = -1;
        *lower = -1;
        return;
    }
    else
    // inside will somtimes find that you are in two different sectors if the x,y
    // is exactly on a sector line.
    if (sln > 2)
    {
        //DSPRINTF(ds, "TOO MANY SECTORS FOUND: x=%d, y=%d, match=%d, num sectors %d, %d, %d, %d, %d, %d", x, y, match, sln, sectorlist[0], sectorlist[1], sectorlist[2], sectorlist[3], sectorlist[4]);
        MONO_PRINT(ds);
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

SWBOOL
FindCeilingView(short match, int32_t* x, int32_t* y, int32_t z, int16_t* sectnum)
{
    int xoff = 0;
    int yoff = 0;
    short i, nexti;
    SPRITEp sp = NULL;
    short top_sprite = -1;
    int pix_diff;
    int newz;

    save.zcount = 0;

    // Search Stat List For closest ceiling view sprite
    // Get the match, xoff, yoff from this point
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], i, nexti)
    {
        sp = &sprite[i];

        if (sp->hitag == VIEW_THRU_CEILING && sp->lotag == match)
        {
            xoff = *x - sp->x;
            yoff = *y - sp->y;
            break;
        }
    }

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], i, nexti)
    {
        sp = &sprite[i];

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
        return FALSE;

    ASSERT(sp);
    ASSERT(sp->hitag == VIEW_THRU_FLOOR);

    pix_diff = labs(z - sector[sp->sectnum].floorz) >> 8;
    newz = sector[sp->sectnum].floorz + ((pix_diff / 128) + 1) * Z(128);

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], i, nexti)
    {
        sp = &sprite[i];

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
                    sector[sp->sectnum].floorpicnum = FAF_MIRROR_PIC+1;
                sector[sp->sectnum].floorheinum = 0;

                save.zcount++;
                PRODUCTION_ASSERT(save.zcount < ZMAX);
            }
        }
    }

    return TRUE;
}

SWBOOL
FindFloorView(short match, int32_t* x, int32_t* y, int32_t z, int16_t* sectnum)
{
    int xoff = 0;
    int yoff = 0;
    short i, nexti;
    SPRITEp sp = NULL;
    int newz;
    int pix_diff;

    save.zcount = 0;

    // Search Stat List For closest ceiling view sprite
    // Get the match, xoff, yoff from this point
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], i, nexti)
    {
        sp = &sprite[i];

        if (sp->hitag == VIEW_THRU_FLOOR && sp->lotag == match)
        {
            xoff = *x - sp->x;
            yoff = *y - sp->y;
            break;
        }
    }


    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], i, nexti)
    {
        sp = &sprite[i];

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
        return FALSE;

    ASSERT(sp);
    ASSERT(sp->hitag == VIEW_THRU_CEILING);

    // move ceiling multiple of 128 so that the wall tile will line up
    pix_diff = labs(z - sector[sp->sectnum].ceilingz) >> 8;
    newz = sector[sp->sectnum].ceilingz - ((pix_diff / 128) + 1) * Z(128);

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], i, nexti)
    {
        sp = &sprite[i];

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
                    sector[sp->sectnum].ceilingpicnum = FAF_MIRROR_PIC+1;
                sector[sp->sectnum].ceilingheinum = 0;

                save.zcount++;
                PRODUCTION_ASSERT(save.zcount < ZMAX);
            }
        }
    }

    return TRUE;
}

short
ViewSectorInScene(short cursectnum, short type, short level)
{
    int i, nexti;
    int j, nextj;
    SPRITEp sp;
    SPRITEp sp2;
    int cz, fz;
    short match;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], i, nexti)
    {
        sp = &sprite[i];

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

                if (!PicInView(FAF_MIRROR_PIC, TRUE))
                    return -1;

                return match;
            }
        }
    }

    return -1;
}

void
DrawOverlapRoom(int tx, int ty, int tz, short tang, int thoriz, short tsectnum)
{
    short i;
    short match;

    save.zcount = 0;

    match = ViewSectorInScene(tsectnum, VIEW_THRU_CEILING, VIEW_LEVEL1);
    if (match != -1)
    {
        FindCeilingView(match, &tx, &ty, tz, &tsectnum);

        if (tsectnum < 0)
            return;

        drawrooms(tx, ty, tz, tang, thoriz, tsectnum);
        //FAF_DrawRooms(tx, ty, tz, tang, thoriz, tsectnum);

        // reset Z's
        for (i = 0; i < save.zcount; i++)
        {
            sector[save.sectnum[i]].floorz = save.zval[i];
            sector[save.sectnum[i]].floorpicnum = save.pic[i];
            sector[save.sectnum[i]].floorheinum = save.slope[i];
        }

        analyzesprites(tx, ty, tz, FALSE);
        post_analyzesprites();
        drawmasks();

    }
    else
    {
        match = ViewSectorInScene(tsectnum, VIEW_THRU_FLOOR, VIEW_LEVEL2);
        if (match != -1)
        {
            FindFloorView(match, &tx, &ty, tz, &tsectnum);

            if (tsectnum < 0)
                return;

            drawrooms(tx, ty, tz, tang, thoriz, tsectnum);
            //FAF_DrawRooms(tx, ty, tz, tang, thoriz, tsectnum);

            // reset Z's
            for (i = 0; i < save.zcount; i++)
            {
                sector[save.sectnum[i]].ceilingz = save.zval[i];
                sector[save.sectnum[i]].ceilingpicnum = save.pic[i];
                sector[save.sectnum[i]].ceilingheinum = save.slope[i];
            }

            analyzesprites(tx, ty, tz, FALSE);
            post_analyzesprites();
            drawmasks();

        }
    }
}

