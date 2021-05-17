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
#include "game.h"
#include "tags.h"
#include "weapon.h"
#include "sprite.h"
#include "misc.h"
#include "interpso.h"
#include "render.h"

BEGIN_SW_NS

extern int GlobSpeedSO;

void CopySectorWalls(short dest_sectnum, short src_sectnum)
{
    short dest_wall_num, src_wall_num, start_wall;
    SECTOR_OBJECTp sop;
    SECTORp *sectp;

    dest_wall_num = sector[dest_sectnum].wallptr;
    src_wall_num = sector[src_sectnum].wallptr;

    start_wall = dest_wall_num;

    do
    {
        wall[dest_wall_num].picnum = wall[src_wall_num].picnum;

        wall[dest_wall_num].xrepeat =       wall[src_wall_num].xrepeat;
        wall[dest_wall_num].yrepeat =       wall[src_wall_num].yrepeat;
        wall[dest_wall_num].overpicnum =    wall[src_wall_num].overpicnum;
        wall[dest_wall_num].pal =           wall[src_wall_num].pal;
        wall[dest_wall_num].cstat =         wall[src_wall_num].cstat;
        wall[dest_wall_num].shade =         wall[src_wall_num].shade;
        wall[dest_wall_num].xpan_ =      wall[src_wall_num].xpan_;
        wall[dest_wall_num].ypan_ =      wall[src_wall_num].ypan_;
        wall[dest_wall_num].hitag =         wall[src_wall_num].hitag;
        wall[dest_wall_num].lotag =         wall[src_wall_num].lotag;
        wall[dest_wall_num].extra =         wall[src_wall_num].extra;

        uint16_t const dest_nextwall = wall[dest_wall_num].nextwall;
        uint16_t const src_nextwall = wall[src_wall_num].nextwall;

        if (dest_nextwall < MAXWALLS && src_nextwall < MAXWALLS)
        {
            wall[dest_nextwall].picnum = wall[src_nextwall].picnum;
            wall[dest_nextwall].xrepeat = wall[src_nextwall].xrepeat;
            wall[dest_nextwall].yrepeat = wall[src_nextwall].yrepeat;
            wall[dest_nextwall].overpicnum = wall[src_nextwall].overpicnum;
            wall[dest_nextwall].pal = wall[src_nextwall].pal;
            wall[dest_nextwall].cstat = wall[src_nextwall].cstat;
            wall[dest_nextwall].shade = wall[src_nextwall].shade;
            wall[dest_nextwall].xpan_ = wall[src_nextwall].xpan_;
            wall[dest_nextwall].ypan_ = wall[src_nextwall].ypan_;
            wall[dest_nextwall].hitag = wall[src_nextwall].hitag;
            wall[dest_nextwall].lotag = wall[src_nextwall].lotag;
            wall[dest_nextwall].extra = wall[src_nextwall].extra;
        }

        dest_wall_num = wall[dest_wall_num].point2;
        src_wall_num = wall[src_wall_num].point2;
    }
    while (dest_wall_num != start_wall);

    // TODO: Mapping a sector to the sector object to which it belongs is better
    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (SO_EMPTY(sop))
            continue;

        for (sectp = sop->sectp; *sectp; sectp++)
            if (*sectp - sector == dest_sectnum)
            {
                so_setinterpolationtics(sop, 0);
                break;
            }
    }
}

void CopySectorMatch(short match)
{
    int ed,ss;
    SPRITEp dest_sp, src_sp;
    SECTORp dsectp,ssectp;
    int kill;
    SPRITEp k;

    StatIterator it(STAT_COPY_DEST);
    while ((ed = it.NextIndex()) >= 0)
    {
        dest_sp = &sprite[ed];
        dsectp = &sector[dest_sp->sectnum];

        if (match != sprite[ed].lotag)
            continue;

        StatIterator it2(STAT_COPY_SOURCE);
        while ((ss = it2.NextIndex()) >= 0)
        {
            src_sp = &sprite[ss];

            if (SP_TAG2(src_sp) == SPRITE_TAG2(ed) &&
                SP_TAG3(src_sp) == SPRITE_TAG3(ed))
            {
                int src_move;
                ssectp = &sector[src_sp->sectnum];

                // !!!!!AAAAAAAAAAAAAAAAAAAAAAHHHHHHHHHHHHHHHHHHHHHH
                // Don't kill anything you don't have to
                // this wall killing things on a Queue causing
                // invalid situations

#if 1
                // kill all sprites in the dest sector that need to be
                SectIterator itsec(dest_sp->sectnum);
                while ((kill = itsec.NextIndex()) >= 0)
                {
                    k = &sprite[kill];

                    // kill anything not invisible
                    if (!TEST(k->cstat, CSTAT_SPRITE_INVISIBLE))
                    {
                        if (User[kill].Data())
                        {
                            // be safe with the killing
                            //SetSuicide(kill);
                        }
                        else
                        {
                            SpriteQueueDelete(kill); // new function to allow killing - hopefully
                            KillSprite(kill);
                        }
                    }
                }
#endif

                CopySectorWalls(dest_sp->sectnum, src_sp->sectnum);

                itsec.Reset(src_sp->sectnum);
                while ((src_move = itsec.NextIndex()) >= 0)
                {
                    // don't move ST1 Copy Tags
                    if (SPRITE_TAG1(src_move) != SECT_COPY_SOURCE)
                    {
                        int sx,sy,dx,dy,src_xoff,src_yoff,trash;

                        // move sprites from source to dest - use center offset

                        // get center of src and dest sect
                        SectorMidPoint(src_sp->sectnum, &sx, &sy, &trash);
                        SectorMidPoint(dest_sp->sectnum, &dx, &dy, &trash);

                        // get offset
                        src_xoff = sx - sprite[src_move].x;
                        src_yoff = sy - sprite[src_move].y;

                        // move sprite to dest sector
                        sprite[src_move].x = dx - src_xoff;
                        sprite[src_move].y = dy - src_yoff;

                        // change sector
                        changespritesect(src_move, dest_sp->sectnum);

                        // check to see if it moved on to a sector object
                        if (TEST(sector[dest_sp->sectnum].extra, SECTFX_SECTOR_OBJECT))
                        {
                            SECTOR_OBJECTp sop;

                            // find and add sprite to SO
                            sop = DetectSectorObject(&sector[sprite[src_move].sectnum]);
                            AddSpriteToSectorObject(src_move, sop);

                            // update sprites postions so they aren't in the
                            // wrong place for one frame
                            GlobSpeedSO = 0;
                            RefreshPoints(sop, 0, 0, true);
                        }
                    }
                }

                // copy sector user if there is one
                if (SectUser[src_sp->sectnum].Data() || SectUser[dest_sp->sectnum].Data())
                {
                    SECT_USERp ssectu = GetSectUser(src_sp->sectnum);
                    SECT_USERp dsectu = GetSectUser(dest_sp->sectnum);

                    memcpy(dsectu, ssectu, sizeof(SECT_USER));
                }

                dsectp->hitag = ssectp->hitag;
                dsectp->lotag = ssectp->lotag;

                dsectp->floorz = ssectp->floorz;
                dsectp->ceilingz = ssectp->ceilingz;

                dsectp->floorshade = ssectp->floorshade;
                dsectp->ceilingshade = ssectp->ceilingshade;

                dsectp->floorpicnum = ssectp->floorpicnum;
                dsectp->ceilingpicnum = ssectp->ceilingpicnum;

                dsectp->floorheinum = ssectp->floorheinum;
                dsectp->ceilingheinum = ssectp->ceilingheinum;

                dsectp->floorpal = ssectp->floorpal;
                dsectp->ceilingpal = ssectp->ceilingpal;

                dsectp->floorxpan_ = ssectp->floorxpan_;
                dsectp->ceilingxpan_ = ssectp->ceilingxpan_;

                dsectp->floorypan_ = ssectp->floorypan_;
                dsectp->ceilingypan_ = ssectp->ceilingypan_;

                dsectp->floorstat = ssectp->floorstat;
                dsectp->ceilingstat = ssectp->ceilingstat;

                dsectp->extra = ssectp->extra;
                dsectp->visibility = ssectp->visibility;

                dsectp->portalnum = ssectp->portalnum;
                dsectp->portalflags = ssectp->portalflags;

                if (ssectp->portalflags & (PORTAL_SECTOR_CEILING|PORTAL_SECTOR_FLOOR)) allPortals[ssectp->portalnum].dx = allPortals[ssectp->portalnum].dy = 0;
            }
        }
    }

    // do this outside of processing loop for safety

    // kill all matching dest
    it.Reset(STAT_COPY_DEST);
    while ((ed = it.NextIndex()) >= 0)
    {
        if (match == sprite[ed].lotag)
            KillSprite(ed);
    }

    // kill all matching sources
    it.Reset(STAT_COPY_SOURCE);
    while ((ss = it.NextIndex()) >= 0)
    {
        if (match == sprite[ss].lotag)
            KillSprite(ss);
    }

}
END_SW_NS
