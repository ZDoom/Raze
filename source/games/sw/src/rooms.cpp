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

// Polymost only!
void SW_FloorPortalHack(DSWActor* actor, int z, int match);
void SW_CeilingPortalHack(DSWActor* actor, int z, int match);


enum { ZMAX = 400 };
struct SAVE
{
    sectortype* sect[ZMAX];
    int32_t zval[ZMAX];
    int16_t pic[ZMAX];
    int16_t zcount;
    int16_t slope[ZMAX];
};

SAVE save;

bool FAF_DebugView = false;

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DSWActor* insertActor(sectortype* sect, int statnum)
{
    auto pActor = static_cast<DSWActor*>(::InsertActor(RUNTIME_CLASS(DSWActor), sect, statnum));

    pActor->spr.intowner = -1;
    return pActor;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool FAF_Sector(sectortype* sect)
{
    SWSectIterator it(sect);
    while (auto actor = it.Next())
    {
        if (actor->spr.statnum == STAT_FAF &&
            (actor->spr.hitag >= VIEW_LEVEL1 && actor->spr.hitag <= VIEW_LEVEL6))
        {
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SetWallWarpHitscan(sectortype* sect)
{
    DSWActor* sp_warp;

    if (!WarpSectorInfo(sect, &sp_warp))
        return;

    if (!sp_warp)
        return;

    auto start_wall = sect->firstWall();
    auto wall_num = start_wall;

    // Travel all the way around loop setting wall bits
    do
    {
        if (wall_num->twoSided())
            wall_num->cstat |= (CSTAT_WALL_WARP_HITSCAN);
        wall_num = wall_num->point2Wall();
    }
    while (wall_num != start_wall);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ResetWallWarpHitscan(sectortype* sect)
{
    auto start_wall = sect->firstWall();
    auto wall_num = start_wall;

    // Travel all the way around loop setting wall bits
    do
    {
        wall_num->cstat &= ~(CSTAT_WALL_WARP_HITSCAN);
        wall_num = wall_num->point2Wall();
    }
    while (wall_num != start_wall);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void FAFhitscan(const DVector3& start, sectortype* sect, const DVector3& vect, HitInfo& hit, int32_t clipmask)
{
    auto newsector = sect;
    int startclipmask = 0;
    bool plax_found = false;

    if (clipmask == CLIPMASK_MISSILE)
        startclipmask = CLIPMASK_WARP_HITSCAN;

    hitscan(start, sect, vect, hit, startclipmask);

    if (hit.hitSector == nullptr)
        return;

    if (hit.hitWall != nullptr)
    {
        // hitscan warping
        if ((hit.hitWall->cstat & CSTAT_WALL_WARP_HITSCAN))
        {
            // back it up a bit to get a correct warp location
            hit.hitpos.XY() -= vect.XY() * (1 / 512.);

            // warp to new x,y,z, sectnum
            if (Warp(hit.hitpos, &hit.hitSector))
            {
                // hitscan needs to pass through dest sect
                ResetWallWarpHitscan(hit.hitSector);

                // NOTE: This could be recursive I think if need be
                hitscan(hit.hitpos, hit.hitSector, vect, hit, startclipmask);

                // reset hitscan block for dest sect
                SetWallWarpHitscan(hit.hitSector);

                return;
            }
            else
            {
                //ASSERT(true == false); // some maps in SWC20 trigger this due to poor design.
                return;
            }
        }
    }

    // make sure it hit JUST a sector before doing a check
    if (hit.hitWall == nullptr && hit.actor() == nullptr)
    {
        if ((hit.hitSector->extra & SECTFX_WARP_SECTOR))
        {
            if ((hit.hitSector->firstWall()->cstat & CSTAT_WALL_WARP_HITSCAN))
            {
                // hit the floor of a sector that is a warping sector
                sectortype* newsect = nullptr;
                if (Warp(hit.hitpos, &newsect))
                {
                    hitscan(hit.hitpos, newsect, vect, hit, clipmask);
                    return;
                }
            }
            else
            {
                sectortype* newsect = nullptr;
                if (WarpPlane(hit.hitpos, &newsect))
                {
                    hitscan(hit.hitpos, newsect, vect, hit, clipmask);
                    return;
                }
            }
        }

        double loz, hiz;
        getzsofslopeptr(hit.hitSector, hit.hitpos, &hiz, &loz);
        if (abs(hit.hitpos.Z - loz) < 4)
        {
            if (FAF_ConnectFloor(hit.hitSector) && !(hit.hitSector->floorstat & CSTAT_SECTOR_FAF_BLOCK_HITSCAN))
            {
                updatesectorz(hit.hitpos.plusZ(12), &newsector);
                plax_found = true;
            }
        }
        else if (abs(hit.hitpos.Z - hiz) < 4)
        {
            if (FAF_ConnectCeiling(hit.hitSector) && !(hit.hitSector->floorstat & CSTAT_SECTOR_FAF_BLOCK_HITSCAN))
            {
                updatesectorz(hit.hitpos.plusZ(-12), &newsector);
                plax_found = true;
            }
        }
    }

    if (plax_found)
    {
        hitscan(hit.hitpos, newsector, vect, hit, clipmask);
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool FAFcansee(const DVector3& start, sectortype* sects, const DVector3& end, sectortype* secte)
{
    auto newsect = sects;
    bool plax_found = false;
    // ASSERT(sects >= 0 && secte >= 0);

    // early out to regular routine
    if ((!sects || !FAF_Sector(sects)) && (!secte || !FAF_Sector(secte)))
    {
        return !!cansee(start, sects, end, secte);
    }

    // get angle
    DVector3 diff = end - start;
    DAngle ang = VecToAngle(diff);
    DVector3 vect; 
    vect.XY() = ang.ToVector() * 1024;
    double dist = diff.XY().Length();

    // get x,y,z, vectors
    if (dist != 0)
    {
        if (end.X != start.X)
            vect.Z = vect.X * diff.Z / diff.X;
        else if (end.Y != start.Y)
            vect.Z = vect.Y * diff.Z / diff.Y;
        else
            vect.Z = 0;
    }
    else
        vect.Z = 0;

    HitInfo hit{};
    hitscan(start, sects, vect, hit, CLIPMASK_MISSILE);

    if (hit.hitSector == nullptr)
        return false;

    // make sure it hit JUST a sector before doing a check
    if (hit.hitWall == nullptr && hit.actor() == nullptr)
    {
        double loz, hiz;
        getzsofslopeptr(hit.hitSector, hit.hitpos.X, hit.hitpos.Y, &hiz, &loz);
        if (abs(hit.hitpos.Z - loz) < 4)
        {
            if (FAF_ConnectFloor(hit.hitSector))
            {
                updatesectorz(hit.hitpos.plusZ(12), &newsect);
                plax_found = true;
            }
        }
        else if (abs(hit.hitpos.Z - hiz) < 4)
        {
            if (FAF_ConnectCeiling(hit.hitSector))
            {
                updatesectorz(hit.hitpos.plusZ(-12), &newsect);
                plax_found = true;
            }
        }
    }
    else
    {
        return !!cansee(start, sects, end, secte);
    }

    if (plax_found)
        return !!cansee(hit.hitpos, newsect, end, secte);

    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

double GetZadjustment(sectortype* sect, short hitag)
{
    if (sect == nullptr || !(sect->extra & SECTFX_Z_ADJUST))
        return 0;

    SWStatIterator it(STAT_ST1);
    while (auto itActor = it.Next())
    {
        if (itActor->spr.hitag == hitag && itActor->sector() == sect)
        {
            return itActor->spr.lotag;
        }
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool SectorZadjust(const Collision& ceilhit, double* hiz, const Collision& florhit, double* loz)
{
    double z_amt = 0;

    bool SkipFAFcheck = false;

    if (florhit.type != -1)
    {
        switch (florhit.type)
        {
        case kHitSector:
        {
            auto hit_sector = florhit.hitSector;

            // don't jack with connect sectors
            if (FAF_ConnectFloor(hit_sector))
            {
                // rippers were dying through the floor in $rock
                if ((hit_sector->floorstat & CSTAT_SECTOR_FAF_BLOCK_HITSCAN))
                    break;

                if ((hit_sector->extra & SECTFX_Z_ADJUST))
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

            if (!(hit_sector->extra & SECTFX_Z_ADJUST))
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
            if ((hit_sector->floorstat & CSTAT_SECTOR_SKY))
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
            auto hit_sector = ceilhit.hitSector;

            // don't jack with connect sectors
            if (FAF_ConnectCeiling(hit_sector))
            {
                if ((hit_sector->extra & SECTFX_Z_ADJUST))
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

            if (!(hit_sector->extra & SECTFX_Z_ADJUST))
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
            if ((hit_sector->ceilingstat & CSTAT_SECTOR_SKY))
            {
                *hiz -= PlaxCeilGlobZadjust;
            }

            break;
        }
        }
    }

    return SkipFAFcheck;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void WaterAdjust(const Collision& florhit, double* loz)
{
    if (florhit.type == kHitSector)
    {
        auto sect = florhit.hitSector;
        if (!sect->hasU()) return;

        if (sect->hasU() && FixedToInt(sect->depth_fixed))
            *loz += FixedToInt(sect->depth_fixed);
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void FAFgetzrange(const DVector3& pos, sectortype* sect, double* hiz, Collision* ceilhit, double* loz, Collision* florhit, int32_t clipdist, int32_t clipmask)
{
    double foo1;
    Collision foo2;
    bool SkipFAFcheck;
    Collision trash; trash.invalidate();

    // IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // This will return invalid FAF ceiling and floor heights inside of analyzesprite
    // because the ceiling and floors get moved out of the way for drawing.

    // early out to regular routine
    if (sect == nullptr || !FAF_ConnectArea(sect))
    {
        getzrange(pos, sect, hiz,  *ceilhit, loz,  *florhit, clipdist, clipmask);
        SectorZadjust(*ceilhit, hiz, *florhit, loz);
        WaterAdjust(*florhit, loz);
        return;
    }

    getzrange(pos, sect, hiz,  *ceilhit, loz,  *florhit, clipdist, clipmask);
    SkipFAFcheck = SectorZadjust(*ceilhit, hiz, *florhit, loz);
    WaterAdjust(*florhit, loz);

    if (SkipFAFcheck)
        return;

    if (FAF_ConnectCeiling(sect))
    {
        auto uppersect = sect;
		auto newpos = DVector3(pos.XY(), *hiz - 2);

        if (ceilhit->type == kHitSprite) return;

        updatesectorz(newpos, &uppersect);
        if (uppersect == nullptr)
            return;

        getzrange(newpos, uppersect, hiz,  *ceilhit, &foo1, foo2, clipdist, clipmask);
        SectorZadjust(*ceilhit, hiz, trash, nullptr);
    }
    else if (FAF_ConnectFloor(sect) && !(sect->floorstat & CSTAT_SECTOR_FAF_BLOCK_HITSCAN))
    {
        auto lowersect = sect;
		auto newpos = DVector3(pos.XY(), *loz + 2);

        if (florhit->type == kHitSprite) return;

        updatesectorz(newpos, &lowersect);
        if (lowersect == nullptr)
            return; // _ErrMsg(ERR_STD_ARG, "Did not find a sector at %d, %d, %d", x, y, newz);

        getzrange(newpos, lowersect, &foo1, foo2, loz,  *florhit, clipdist, clipmask);
        SectorZadjust(trash, nullptr, *florhit, loz);
        WaterAdjust(*florhit, loz);
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void FAFgetzrangepoint(const DVector3& pos, sectortype* sect, double* hiz, Collision* ceilhit, double* loz, Collision* florhit)
{
    double foo1;
    Collision foo2;
    bool SkipFAFcheck;
    Collision trash; trash.invalidate();

    // IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // This will return invalid FAF ceiling and floor heights inside of analyzesprite
    // because the ceiling and floors get moved out of the way for drawing.

    // early out to regular routine
    if (!FAF_ConnectArea(sect))
    {
        getzrangepoint(pos, sect, hiz,  ceilhit, loz,  florhit);
        SectorZadjust(*ceilhit, hiz, *florhit, loz);
        WaterAdjust(*florhit, loz);
        return;
    }

    getzrangepoint(pos, sect, hiz,  ceilhit, loz,  florhit);
    SkipFAFcheck = SectorZadjust(*ceilhit, hiz, *florhit, loz);
    WaterAdjust(*florhit, loz);

    if (SkipFAFcheck)
        return;

    if (FAF_ConnectCeiling(sect))
    {
        auto uppersect = sect;
		auto newpos = DVector3(pos.XY(), *hiz - 2);
        if (ceilhit->type == kHitSprite)
            return;

        updatesectorz(newpos, &uppersect);
        if (uppersect == nullptr)
            return;
        getzrangepoint(newpos, uppersect, hiz,  ceilhit, &foo1,  &foo2);
        SectorZadjust(*ceilhit, hiz, trash, nullptr);
    }
    else if (FAF_ConnectFloor(sect) && !(sect->floorstat & CSTAT_SECTOR_FAF_BLOCK_HITSCAN))
    {
        auto lowersect = sect;
		auto newpos = DVector3(pos.XY(), *loz + 2);
        if (florhit->type == kHitSprite)
            return;
        updatesectorz(newpos, &lowersect);
        if (lowersect == nullptr)
            return;
        getzrangepoint(newpos, lowersect, &foo1,  &foo2, loz,  florhit);
        SectorZadjust(trash, nullptr, *florhit, loz);
        WaterAdjust(*florhit, loz);
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SetupMirrorTiles(void)
{
    SWStatIterator it(STAT_FAF);
    while (auto actor = it.Next())
    {
        if (actor->sector()->ceilingpicnum == FAF_PLACE_MIRROR_PIC)
        {
            actor->sector()->ceilingpicnum = FAF_MIRROR_PIC;
            actor->sector()->ceilingstat |= (CSTAT_SECTOR_SKY);
        }

        if (actor->sector()->floorpicnum == FAF_PLACE_MIRROR_PIC)
        {
            actor->sector()->floorpicnum = FAF_MIRROR_PIC;
            actor->sector()->floorstat |= (CSTAT_SECTOR_SKY);
        }

        if (actor->sector()->ceilingpicnum == FAF_PLACE_MIRROR_PIC+1)
            actor->sector()->ceilingpicnum = FAF_MIRROR_PIC+1;

        if (actor->sector()->floorpicnum == FAF_PLACE_MIRROR_PIC+1)
            actor->sector()->floorpicnum = FAF_MIRROR_PIC+1;
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GetUpperLowerSector(int match, double x, double y, sectortype** upper, sectortype** lower)
{
    int i;
    sectortype* sectorlist[16];
    int sln = 0;

    for(auto& sect: sector)
    {
        if (inside(x, y, &sect) == 1)
        {
            bool found = false;

            SWSectIterator it(&sect);
            while (auto actor = it.Next())
            {
                if (actor->spr.statnum == STAT_FAF &&
                    (actor->spr.hitag >= VIEW_LEVEL1 && actor->spr.hitag <= VIEW_LEVEL6)
                    && actor->spr.lotag == match)
                {
                    found = true;
                }
            }

            if (!found)
                continue;
            if (sln < (int)SIZ(sectorlist))
                sectorlist[sln] = &sect;
            sln++;
        }
    }

    // might not find ANYTHING if not tagged right
    if (sln == 0)
    {
        *upper = nullptr;
        *lower = nullptr;
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
        GetUpperLowerSector(match, x - 1/16., y, upper, lower);
    }

    if (sln == 2)
    {
        if (sectorlist[0]->floorz < sectorlist[1]->floorz)
        {
            // swap
            // make sectorlist[0] the LOW sector
            std::swap(sectorlist[0],sectorlist[1]);
        }

        *lower = sectorlist[0];
        *upper = sectorlist[1];
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool FindCeilingView(int match, double* x, double * y, double z, sectortype** sect)
{
    double xoff = 0;
    double yoff = 0;

    save.zcount = 0;
    DSWActor* actor = nullptr;

    // Search Stat List For closest ceiling view sprite
    // Get the match, xoff, yoff from this point
    SWStatIterator it(STAT_FAF);
    while ((actor = it.Next()))
    {
        if (actor->spr.hitag == VIEW_THRU_CEILING && actor->spr.lotag == match)
        {
            xoff = *x - actor->spr.pos.X;
            yoff = *y - actor->spr.pos.Y;
            break;
        }
    }

    it.Reset(STAT_FAF);
    while ((actor = it.Next()))
    {
        if (actor->spr.lotag == match)
        {
            // determine x,y position
            if (actor->spr.hitag == VIEW_THRU_FLOOR)
            {
                sectortype* upper,* lower;

                *x = actor->spr.pos.X + xoff;
                *y = actor->spr.pos.Y + yoff;

                // get new sector
                GetUpperLowerSector(match, *x, *y, &upper, &lower);
                *sect = upper;
                break;
            }
        }
    }

    if (*sect == nullptr)
        return false;

    if (!actor || actor->spr.hitag != VIEW_THRU_FLOOR)
    {
        *sect = nullptr;
        return false;
    }


    return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool FindFloorView(int match, double* x, double* y, double z, sectortype** sect)
{
    double xoff = 0;
    double yoff = 0;

    save.zcount = 0;
    DSWActor* actor = nullptr;

    // Search Stat List For closest ceiling view sprite
    // Get the match, xoff, yoff from this point
    SWStatIterator it(STAT_FAF);
    while ((actor = it.Next()))
    {
        if (actor->spr.hitag == VIEW_THRU_FLOOR && actor->spr.lotag == match)
        {
            xoff = *x - actor->spr.pos.X;
            yoff = *y - actor->spr.pos.Y;
            break;
        }
    }


    it.Reset(STAT_FAF);
    while ((actor = it.Next()))
    {
        if (actor->spr.lotag == match)
        {
            // determine x,y position
            if (actor->spr.hitag == VIEW_THRU_CEILING)
            {
                sectortype* upper,* lower;

                *x = actor->spr.pos.X + xoff;
                *y = actor->spr.pos.Y + yoff;

                // get new sector
                GetUpperLowerSector(match, *x, *y, &upper, &lower);
                *sect = lower;
                break;
            }
        }
    }

    if (*sect == nullptr)
        return false;

    if (!actor || actor->spr.hitag != VIEW_THRU_CEILING)
    {
        *sect = nullptr;
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

short FindViewSectorInScene(sectortype* cursect, int level)
{
    int match;

    SWStatIterator it(STAT_FAF);
    while (auto actor = it.Next())
    {
        if (actor->spr.hitag == level)
        {
            if (cursect == actor->sector())
            {
                // ignore case if sprite is pointing up
                if (actor->spr.angle == DAngle270)
                    continue;

                // only gets to here is sprite is pointing down

                // found a potential match
                match = actor->spr.lotag;

                return match;
            }
        }
    }

    return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

struct PortalGroup
{
    TArray<int> sectors;
    int othersector = -1;
	DVector3 offset = {0,0,0};
};

    // This is very messy because some portals are linked outside the actual portal sectors, so we have to use the complicated original linking logic to find the connection. :?
void CollectPortals()
{
    TArray<PortalGroup> floorportals;
    TArray<PortalGroup> ceilingportals;
    BitArray floordone(sector.Size()), ceilingdone(sector.Size());

    for (auto& sec : sector)
    {
        sec.portalflags = sec.portalnum = 0;
    }
    floordone.Zero();
    ceilingdone.Zero();
    portalClear();

    for (unsigned i = 0; i < sector.Size(); i++)
    {
        if (sector[i].floorpicnum == FAF_MIRROR_PIC && !floordone[i])
        {
            auto& fp = floorportals[floorportals.Reserve(1)];
            fp.sectors.Push(i);
            floordone.Set(i);
            for (unsigned ii = 0; ii < fp.sectors.Size(); ii++)
            {
                for (auto& wal : wallsofsector(fp.sectors[ii]))
                {
                    if (!wal.twoSided()) continue;
                    auto nsec = wal.nextSector();
                    auto ns = sectnum(nsec);
                    if (floordone[ns] || nsec->floorpicnum != FAF_MIRROR_PIC) continue;
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
                for (auto& wal : wallsofsector(fp.sectors[ii]))
                {
                    if (!wal.twoSided()) continue;
                    auto nsec = wal.nextSector();
                    auto ns = sectnum(nsec);
                    if (ceilingdone[ns] || nsec->ceilingpicnum != FAF_MIRROR_PIC) continue;
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
                auto tpos = actor->spr.pos;
                auto tsect = &sector[sec];

                int match = FindViewSectorInScene(tsect, VIEW_LEVEL1);
                if (match != -1)
                {
                    FindCeilingView(match, &tpos.X, &tpos.Y, tpos.Z, &tsect);
                    if (tsect != nullptr &&tsect->floorpicnum == FAF_MIRROR_PIC)
                    {
                        // got something!
                        fp.othersector = sectnum(tsect);
                        fp.offset = tpos - actor->spr.pos;
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
				auto tpos = actor->spr.pos;
                auto tsect = &sector[sec];

                int match = FindViewSectorInScene(tsect, VIEW_LEVEL2);
                if (match != -1)
                {
                    FindFloorView(match, &tpos.X, &tpos.Y, tpos.Z, &tsect);
                    if (tsect != nullptr && tsect->ceilingpicnum == FAF_MIRROR_PIC)
                    {
                        // got something!
                        fp.othersector = sectnum(tsect);
						fp.offset = tpos - actor->spr.pos;
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
				pt.offset.Z = 0;
                int pnum = portalAdd(PORTAL_SECTOR_FLOOR, -1, pt.offset);
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
				pt.offset.Z = 0;
                int pnum = portalAdd(PORTAL_SECTOR_CEILING, -1, pt.offset);
                allPortals[pnum].targets = std::move(pt2->sectors);
                for (auto sec : pt.sectors)
                {
                    sector[sec].portalflags = PORTAL_SECTOR_CEILING;
                    sector[sec].portalnum = pnum;
                }
            }
        }
    }
}


END_SW_NS
