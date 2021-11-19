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
        auto dwall = &wall[dest_wall_num];
        auto swall = &wall[src_wall_num];
        dwall->picnum = swall->picnum;

        dwall->xrepeat =       swall->xrepeat;
        dwall->yrepeat =       swall->yrepeat;
        dwall->overpicnum =    swall->overpicnum;
        dwall->pal =           swall->pal;
        dwall->cstat =         swall->cstat;
        dwall->shade =         swall->shade;
        dwall->xpan_ =         swall->xpan_;
        dwall->ypan_ =         swall->ypan_;
        dwall->hitag =         swall->hitag;
        dwall->lotag =         swall->lotag;
        dwall->extra =         swall->extra;

        if (validWallIndex(dwall->nextwall) && validWallIndex(swall->nextwall))
        {
            auto const dest_nextwall = dwall->nextWall();
            auto const src_nextwall = swall->nextWall();
            dest_nextwall->picnum = src_nextwall->picnum;
            dest_nextwall->xrepeat = src_nextwall->xrepeat;
            dest_nextwall->yrepeat = src_nextwall->yrepeat;
            dest_nextwall->overpicnum = src_nextwall->overpicnum;
            dest_nextwall->pal = src_nextwall->pal;
            dest_nextwall->cstat = src_nextwall->cstat;
            dest_nextwall->shade = src_nextwall->shade;
            dest_nextwall->xpan_ = src_nextwall->xpan_;
            dest_nextwall->ypan_ = src_nextwall->ypan_;
            dest_nextwall->hitag = src_nextwall->hitag;
            dest_nextwall->lotag = src_nextwall->lotag;
            dest_nextwall->extra = src_nextwall->extra;
        }

        dest_wall_num = dwall->point2;
        src_wall_num = swall->point2;
    }
    while (dest_wall_num != start_wall);

    // TODO: Mapping a sector to the sector object to which it belongs is better
    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (SO_EMPTY(sop))
            continue;

        for (sectp = sop->sectp; *sectp; sectp++)
            if (sectnum(*sectp) == dest_sectnum)
            {
                so_setinterpolationtics(sop, 0);
                break;
            }
    }
}

void CopySectorMatch(short match)
{
    SPRITEp dest_sp, src_sp;
    SECTORp dsectp,ssectp;
    int kill;
    SPRITEp k;

    SWStatIterator it(STAT_COPY_DEST);
    while (auto dActor = it.Next())
    {
        dest_sp = &dActor->s();
        dsectp = &sector[dest_sp->sectnum];

        if (match != dest_sp->lotag)
            continue;

        SWStatIterator it2(STAT_COPY_SOURCE);
        while (auto sActor = it2.Next())
        {
            src_sp = &sActor->s();

            if (SP_TAG2(src_sp) == SP_TAG2(dest_sp) &&
                SP_TAG3(src_sp) == SP_TAG3(dest_sp))
            {
                ssectp = &sector[src_sp->sectnum];

                // !!!!!AAAAAAAAAAAAAAAAAAAAAAHHHHHHHHHHHHHHHHHHHHHH
                // Don't kill anything you don't have to
                // this wall killing things on a Queue causing
                // invalid situations

#if 1
                // kill all sprites in the dest sector that need to be
                SWSectIterator itsec(dest_sp->sectnum);
                while (auto itActor = itsec.Next())
                {
                    k = &itActor->s();

                    // kill anything not invisible
                    if (!TEST(k->cstat, CSTAT_SPRITE_INVISIBLE))
                    {
                        if (itActor->hasU())
                        {
                            // be safe with the killing
                            //SetSuicide(kill);
                        }
                        else
                        {
                            SpriteQueueDelete(itActor); // new function to allow killing - hopefully
                            KillActor(itActor);
                        }
                    }
                }
#endif

                CopySectorWalls(dest_sp->sectnum, src_sp->sectnum);

                itsec.Reset(src_sp->sectnum);
                while (auto itActor = itsec.Next())
                {
                    auto sp = &itActor->s();
                    // don't move ST1 Copy Tags
                    if (SP_TAG1(sp) != SECT_COPY_SOURCE)
                    {
                        int sx,sy,dx,dy,src_xoff,src_yoff,trash;

                        // move sprites from source to dest - use center offset

                        // get center of src and dest sect
                        SectorMidPoint(src_sp->sectnum, &sx, &sy, &trash);
                        SectorMidPoint(dest_sp->sectnum, &dx, &dy, &trash);

                        // get offset
                        src_xoff = sx - sp->x;
                        src_yoff = sy - sp->y;

                        // move sprite to dest sector
                        sp->x = dx - src_xoff;
                        sp->y = dy - src_yoff;

                        // change sector
                        ChangeActorSect(itActor, dest_sp->sectnum);

                        // check to see if it moved on to a sector object
                        if (TEST(sector[dest_sp->sectnum].extra, SECTFX_SECTOR_OBJECT))
                        {
                            SECTOR_OBJECTp sop;

                            // find and add sprite to SO
                            sop = DetectSectorObject(sp->sector());
                            AddSpriteToSectorObject(itActor, sop);

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

                if (ssectp->floorpicnum == FAF_MIRROR_PIC || ssectp->ceilingpicnum == FAF_MIRROR_PIC)
                {
                    CollectPortals(); // unavoidable. Since these portals are not static we have to reinitialize all of them.
                }
            }
        }
    }

    // do this outside of processing loop for safety

    // kill all matching dest
    it.Reset(STAT_COPY_DEST);
    while (auto dActor = it.Next())
    {
        if (match == dActor->s().lotag)
            KillActor(dActor);
    }

    // kill all matching sources
    it.Reset(STAT_COPY_SOURCE);
    while (auto sActor = it.Next())
    {
        if (match == sActor->s().lotag)
            KillActor(sActor);
    }

}
END_SW_NS
