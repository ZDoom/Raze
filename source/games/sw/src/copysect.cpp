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

void CopySectorWalls(sectortype* dest_sect, sectortype* src_sect)
{
    SECTOR_OBJECT* sop;
    sectortype* *sectp;

    auto dwall = dest_sect->firstWall();
    auto swall = src_sect->firstWall();
    auto firstwall = dwall;

    // this looks broken.
    do
    {
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

        if (dwall->twoSided() && swall->twoSided())
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

        dwall = dwall->point2Wall();
        swall = swall->point2Wall();
    }
    while (dwall != firstwall);

    // TODO: Mapping a sector to the sector object to which it belongs is better
    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (SO_EMPTY(sop))
            continue;

        for (sectp = sop->sectp; *sectp; sectp++)
            if (*sectp == dest_sect)
            {
                so_setinterpolationtics(sop, 0);
                break;
            }
    }
}

void CopySectorMatch(int match)
{
    sectortype* dsectp, *ssectp;
    int kill;

    SWStatIterator it(STAT_COPY_DEST);
    while (auto dActor = it.Next())
    {
        dsectp = dActor->sector();

        if (match != dActor->spr.lotag)
            continue;

        SWStatIterator it2(STAT_COPY_SOURCE);
        while (auto sActor = it2.Next())
        {
            if (SP_TAG2(sActor) == SP_TAG2(dActor) &&
                SP_TAG3(sActor) == SP_TAG3(dActor))
            {
                ssectp = sActor->sector();

                // !!!!!AAAAAAAAAAAAAAAAAAAAAAHHHHHHHHHHHHHHHHHHHHHH
                // Don't kill anything you don't have to
                // this wall killing things on a Queue causing
                // invalid situations

#if 1
                // kill all sprites in the dest sector that need to be
                SWSectIterator itsec(dsectp);
                while (auto itActor = itsec.Next())
                {
                    // kill anything not invisible
                    if (!(itActor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
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

                CopySectorWalls(dsectp, ssectp);

                itsec.Reset(ssectp);
                while (auto itActor = itsec.Next())
                {
                    // don't move ST1 Copy Tags
                    if (SP_TAG1(itActor) != SECT_COPY_SOURCE)
                    {
                        int sx,sy,dx,dy,src_xoff,src_yoff,trash;

                        // move sprites from source to dest - use center offset

                        // get center of src and dest sect
                        SectorMidPoint(sActor->sector(), &sx, &sy, &trash);
                        SectorMidPoint(dActor->sector(), &dx, &dy, &trash);

                        // get offset
                        src_xoff = sx - itActor->int_pos().X;
                        src_yoff = sy - itActor->int_pos().Y;

                        // move sprite to dest sector
                        itActor->set_int_xy(dx - src_xoff, dy - src_yoff);

                        // change sector
                        ChangeActorSect(itActor, dsectp);

                        // check to see if it moved on to a sector object
                        if ((dsectp->extra & SECTFX_SECTOR_OBJECT))
                        {
                            SECTOR_OBJECT* sop;

                            // find and add sprite to SO
                            sop = DetectSectorObject(itActor->sector());
                            AddSpriteToSectorObject(itActor, sop);

                            // update sprites postions so they aren't in the
                            // wrong place for one frame
                            GlobSpeedSO = 0;
                            RefreshPoints(sop, 0, 0, true);
                        }
                    }
                }

                // copy sector user if there is one

                dsectp->flags        = ssectp->flags;
                dsectp->depth_fixed  = ssectp->depth_fixed;
                dsectp->stag         = ssectp->stag;
                dsectp->ang          = ssectp->ang;
                dsectp->height       = ssectp->height;
                dsectp->speed        = ssectp->speed;
                dsectp->damage       = ssectp->damage;
                dsectp->number       = ssectp->number;
                if (ssectp->u_defined) dsectp->u_defined    = true;
                dsectp->flags2       = ssectp->flags2;

                dsectp->hitag = ssectp->hitag;
                dsectp->lotag = ssectp->lotag;

                dsectp->set_int_floorz(ssectp->int_floorz());
                dsectp->set_int_ceilingz(ssectp->int_ceilingz());

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
        if (match == dActor->spr.lotag)
            KillActor(dActor);
    }

    // kill all matching sources
    it.Reset(STAT_COPY_SOURCE);
    while (auto sActor = it.Next())
    {
        if (match == sActor->spr.lotag)
            KillActor(sActor);
    }

}
END_SW_NS
