//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2005 - 3D Realms Entertainment

This file is NOT part of Shadow Warrior version 1.2
However, it is either an older version of a file that is, or is
some test code written during the development of Shadow Warrior.
This file is provided purely for educational interest.

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

Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include "build.h"
#include "editor.h"

#include "keys.h"
#include "names2.h"
#include "game.h"


extern int qsetmode;

SWBOOL FindCeilingView(short match, int32_t* x, int32_t* y, int32_t z, int16_t* sectnum);
SWBOOL FindFloorView(short match, int32_t* x, int32_t* y, int32_t z, int16_t* sectnum);
short ViewSectorInScene(short cursectnum, short type, short level);
void Message(char *string, char color);


void
_Assert(char *expr, char *strFile, unsigned uLine)
{
    printf(ds, "Assertion failed: %s %s, line %u\n", expr, strFile, uLine);
    //DSPRINTF(ds, "Assertion failed: %s %s, line %u", expr, strFile, uLine);
    MONO_PRINT(ds);
    exit(0);
}

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
    int16_t slope[ZMAX];
    int16_t zcount;
} SAVE, *SAVEp;

SAVE save;

SWBOOL FAF_DebugView = 0;
SWBOOL FAFon = 0;
SWBOOL FAF_DontMoveSectors = FALSE;

short bak_searchsector, bak_searchwall, bak_searchstat;
extern short searchsector, searchwall, searchstat, searchit;

void SetupBuildFAF(void);
void ResetBuildFAF(void);

void ToggleFAF(void)
{
    if (keystatus[KEYSC_3])
    {
        keystatus[KEYSC_3] = FALSE;

        FLIP(FAFon, 1);
        if (FAFon)
        {
            SetupBuildFAF();
        }
        else
        {
            ResetBuildFAF();
        }
    }

    if (FAFon && qsetmode == 200)
    {
        DrawOverlapRoom(pos.x, pos.y, pos.z, ang, horiz, cursectnum);

        // make it so that you can edit both areas in 3D
        // back up vars after the first drawrooms
        bak_searchsector = searchsector;
        bak_searchwall = searchwall;
        bak_searchstat = searchstat;
        searchit = 2;
    }

    if (FAFon && qsetmode == 200 && keystatus[KEYSC_4])
    {
        short match;
        int tx,ty,tz;
        short tsectnum;
        short i;
        keystatus[KEYSC_4] = FALSE;

        tx = pos.x;
        ty = pos.y;
        tz = pos.z;
        tsectnum = cursectnum;

        save.zcount = 0;

        if (sector[cursectnum].ceilingpicnum == FAF_MIRROR_PIC)
        {
            match = ViewSectorInScene(tsectnum, VIEW_THRU_CEILING, VIEW_LEVEL1);

            FAF_DontMoveSectors = TRUE;
            FindCeilingView(match, &tx, &ty, tz, &tsectnum);
            FAF_DontMoveSectors = FALSE;

            pos.x = tx;
            pos.y = ty;
            cursectnum = tsectnum;
            pos.z = sector[cursectnum].floorz - Z(20);
        }
        else if (sector[cursectnum].floorpicnum == FAF_MIRROR_PIC)
        {
            match = ViewSectorInScene(tsectnum, VIEW_THRU_FLOOR, VIEW_LEVEL2);

            FAF_DontMoveSectors = TRUE;
            FindFloorView(match, &tx, &ty, tz, &tsectnum);
            FAF_DontMoveSectors = FALSE;

            pos.x = tx;
            pos.y = ty;
            cursectnum = tsectnum;
            pos.z = sector[cursectnum].ceilingz + Z(20);
        }
    }


}


void FAF_AfterDrawRooms(void)
{
    // make it so that you can edit both areas in 3D
    // if your cursor is in the FAF_MIRROR_PIC area use the vars from the first
    // drawrooms instead
    if ((searchstat == 1 && sector[searchsector].ceilingpicnum == FAF_MIRROR_PIC) ||
        (searchstat == 2 && sector[searchsector].floorpicnum == FAF_MIRROR_PIC))
    {
        searchsector = bak_searchsector;
        searchwall = bak_searchwall;
        searchstat = bak_searchstat;
    }
}

void
SetupBuildFAF(void)
{
    short i, nexti;
    SPRITEp sp,vc_sp,vf_sp,vl_sp;
    short SpriteNum, NextSprite;
    short vc,nextvc,vf,nextvf,l,nextl;
    int zdiff;

    // move every sprite to the correct list
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_DEFAULT], SpriteNum, NextSprite)
    {
        sp = &sprite[SpriteNum];

        if (sp->picnum != ST1)
            continue;

        switch (sp->hitag)
        {
        case VIEW_THRU_CEILING:
        case VIEW_THRU_FLOOR:
        {
            int i,nexti;
            // make sure there is only one set per level of these
            TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], i, nexti)
            {
                if (sprite[i].hitag == sp->hitag && sprite[i].lotag == sp->lotag)
                {
                    sprintf(ds,"Two VIEW_THRU_ tags with same match found on level\n1: x %d, y %d \n2: x %d, y %d", sp->x, sp->y, sprite[i].x, sprite[i].y);
                    Message(ds,0);
                }
            }

            changespritestat(SpriteNum, STAT_FAF);
            break;
        }

        case VIEW_LEVEL1:
        case VIEW_LEVEL2:
        case VIEW_LEVEL3:
        case VIEW_LEVEL4:
        case VIEW_LEVEL5:
        case VIEW_LEVEL6:
        {
            changespritestat(SpriteNum, STAT_FAF);
            break;
        }
        }
    }

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], i, nexti)
    {
        sp = &sprite[i];

        if (sector[sp->sectnum].ceilingpicnum == FAF_PLACE_MIRROR_PIC)
        {
            sector[sp->sectnum].ceilingpicnum = FAF_MIRROR_PIC;
            if (sector[sp->sectnum].floorz == sector[sp->sectnum].ceilingz)
            {
                sprintf(ds, "Mirror used for non-connect area. Use tile 342. Sect %d, x %d, y %d\n", sp->sectnum, wall[sector[sp->sectnum].wallptr].x, wall[sector[sp->sectnum].wallptr].y);
                Message(ds,0);
            }
        }

        if (sector[sp->sectnum].floorpicnum == FAF_PLACE_MIRROR_PIC)
        {
            sector[sp->sectnum].floorpicnum = FAF_MIRROR_PIC;
            if (sector[sp->sectnum].floorz == sector[sp->sectnum].ceilingz)
            {
                sprintf(ds, "Mirror used for non-connect area. Use tile 342. Sect %d, x %d, y %d\n", sp->sectnum, wall[sector[sp->sectnum].wallptr].x, wall[sector[sp->sectnum].wallptr].y);
                Message(ds,0);
            }
        }

        if (sector[sp->sectnum].ceilingpicnum == FAF_PLACE_MIRROR_PIC+1)
            sector[sp->sectnum].ceilingpicnum = FAF_MIRROR_PIC+1;

        if (sector[sp->sectnum].floorpicnum == FAF_PLACE_MIRROR_PIC+1)
            sector[sp->sectnum].floorpicnum = FAF_MIRROR_PIC+1;
    }


    for (i = 0; i < numwalls; i++)
    {
        if (wall[i].picnum == FAF_PLACE_MIRROR_PIC)
            wall[i].picnum = FAF_MIRROR_PIC;

        if (wall[i].picnum == FAF_PLACE_MIRROR_PIC+1)
            wall[i].picnum = FAF_MIRROR_PIC+1;
    }

#if 0
    // check ceiling and floor heights
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], vc, nextvc)
    {
        vc_sp = &sprite[vc];

        if (vc_sp->hitag == VIEW_THRU_CEILING)
        {
            TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], vf, nextvf)
            {
                vf_sp = &sprite[vf];

                if (vf_sp->hitag == VIEW_THRU_FLOOR && vf_sp->lotag == vc_sp->lotag)
                {
                    zdiff = labs(sector[vc_sp->sectnum].ceilingz - sector[vf_sp->sectnum].floorz);

                    //DSPRINTF(ds,"zdiff %d",zdiff);
                    MONO_PRINT(ds);

                    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], l, nextl)
                    {
                        vl_sp = &sprite[l];

                        if (vl_sp->hitag == VIEW_LEVEL1)
                        {
                            if (sector[vl_sp->sectnum].ceilingz < sector[vc_sp->sectnum].ceilingz + zdiff)
                            {
                                sprintf(ds,"Sector %d (x %d, y %d) ceiling z to close to VIEW_THRU_CEILING z",
                                        vl_sp->sectnum, wall[sector[vl_sp->sectnum].wallptr].x, wall[sector[vl_sp->sectnum].wallptr].y);
                                Message(ds,0);
                            }
                        }
                        else if (vl_sp->hitag == VIEW_LEVEL2)
                        {
                            if (sector[vl_sp->sectnum].floorz > sector[vf_sp->sectnum].floorz + zdiff)
                            {
                                sprintf(ds,"Sector %d (x %d, y %d)floor z to close to VIEW_THRU_FLOOR z",
                                        vl_sp->sectnum, wall[sector[vl_sp->sectnum].wallptr].x, wall[sector[vl_sp->sectnum].wallptr].y);
                                Message(ds,0);
                            }
                        }
                    }
                }
            }
        }
    }
#endif

}

void
ResetBuildFAF(void)
{
    short i, nexti;
    SPRITEp sp;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], i, nexti)
    {
        sp = &sprite[i];

        if (sector[sp->sectnum].ceilingpicnum == FAF_MIRROR_PIC)
            sector[sp->sectnum].ceilingpicnum = FAF_PLACE_MIRROR_PIC;

        if (sector[sp->sectnum].floorpicnum == FAF_MIRROR_PIC)
            sector[sp->sectnum].floorpicnum = FAF_PLACE_MIRROR_PIC;

        if (sector[sp->sectnum].ceilingpicnum == FAF_MIRROR_PIC+1)
            sector[sp->sectnum].ceilingpicnum = FAF_PLACE_MIRROR_PIC+1;

        if (sector[sp->sectnum].floorpicnum == FAF_MIRROR_PIC+1)
            sector[sp->sectnum].floorpicnum = FAF_PLACE_MIRROR_PIC+1;
    }

    for (i = 0; i < numwalls; i++)
    {
        if (wall[i].picnum == FAF_MIRROR_PIC)
            wall[i].picnum = FAF_PLACE_MIRROR_PIC;

        if (wall[i].picnum == FAF_MIRROR_PIC+1)
            wall[i].picnum = FAF_PLACE_MIRROR_PIC+1;
    }

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], i, nexti)
    {
        changespritestat(i, STAT_DEFAULT);
    }
}


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
GetUpperLowerSector(short match, int x, int y, short *upper, short *lower)
{
    int i, j;
    short sectorlist[16];
    short sln = 0;
    short SpriteNum, Next;
    SPRITEp sp;

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
                sln++;
            }
        }
    }

    if (sln == 0)
    {
        *upper = -1;
        *lower = -1;
        return;
    }

    // Map rooms have NOT been dragged on top of each other
    if (sln == 1)
    {
        *lower = sectorlist[0];
        *upper = sectorlist[0];
        return;
    }
    else
    // Map rooms HAVE been dragged on top of each other
    if (sln > 2)
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

    if (FAF_DontMoveSectors)
        return TRUE;

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
                sector[sp->sectnum].floorpicnum = FAF_MIRROR_PIC+1;
                sector[sp->sectnum].floorheinum = 0;

                save.zcount++;
                ASSERT(save.zcount < ZMAX);
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

    if (FAF_DontMoveSectors)
        return TRUE;

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

                sector[sp->sectnum].ceilingpicnum = FAF_MIRROR_PIC+1;
                sector[sp->sectnum].ceilingheinum = 0;

                save.zcount++;
                ASSERT(save.zcount < ZMAX);
            }
        }
    }

    return TRUE;
}

SWBOOL
SectorInScene(short tile_num)
{
    if (TEST(gotsector[tile_num >> 3], 1 << (tile_num & 7)))
    {
        RESET(gotsector[tile_num >> 3], 1 << (tile_num & 7));
        return TRUE;
    }

    return FALSE;
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
        {
            sprintf(ds,"COULD NOT FIND TAGGED LEVEL2 SECTOR FROM X %d, Y %d, SECTNUM %d.",pos.x,pos.y,cursectnum);
            Message(ds, 0);
            return;
        }

        drawrooms(tx, ty, tz, tang, thoriz, tsectnum);
        drawmasks();

        // reset Z's
        for (i = 0; i < save.zcount; i++)
        {
            sector[save.sectnum[i]].floorz = save.zval[i];
            sector[save.sectnum[i]].floorpicnum = save.pic[i];
            sector[save.sectnum[i]].floorheinum = save.slope[i];
        }
    }
    else
    {
        match = ViewSectorInScene(tsectnum, VIEW_THRU_FLOOR, VIEW_LEVEL2);
        if (match != -1)
        {
            FindFloorView(match, &tx, &ty, tz, &tsectnum);

            if (tsectnum < 0)
            {
                sprintf(ds,"COULD NOT FIND TAGGED LEVEL1 SECTOR FROM X %d, Y %d, SECTNUM %d.",pos.x,pos.y,cursectnum);
                Message(ds, 0);
                return;
            }

            drawrooms(tx, ty, tz, tang, thoriz, tsectnum);
            drawmasks();

            // reset Z's
            for (i = 0; i < save.zcount; i++)
            {
                sector[save.sectnum[i]].ceilingz = save.zval[i];
                sector[save.sectnum[i]].ceilingpicnum = save.pic[i];
                sector[save.sectnum[i]].ceilingheinum = save.slope[i];
            }
        }
    }
}
