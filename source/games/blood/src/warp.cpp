//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#include "ns.h"	// Must come before everything else!

#include "build.h"

#include "blood.h"

BEGIN_BLD_NS

ZONE gStartZone[8];
#ifdef NOONE_EXTENSIONS
    ZONE gStartZoneTeam1[8];
    ZONE gStartZoneTeam2[8];
    bool gTeamsSpawnUsed = false;
#endif

void validateLinks()
{
    int snum = 0;
    for (auto& sect: sector)
    {
        DCoreActor* upper = sect.upperLink;
        if (upper && !static_cast<DBloodActor*>(upper)->GetOwner())
        {
            Printf(PRINT_HIGH, "Unpartnered upper link in sector %d\n", snum);
            sect.upperLink = nullptr;
        }
        DCoreActor* lower = sect.lowerLink;
        if (lower && !static_cast<DBloodActor*>(lower)->GetOwner())
        {
            Printf(PRINT_HIGH, "Unpartnered lower link in sector %d\n", snum);
            sect.lowerLink = nullptr;
        }
        snum++;
    }
}


void warpInit(TArray<DBloodActor*>& actors)
{
    #ifdef NOONE_EXTENSIONS
    int team1 = 0; int team2 = 0; gTeamsSpawnUsed = false; // increment if team start positions specified.
    #endif

    for(auto actor : actors)
    {
        if (!actor->exists()) continue;
        spritetype* pSprite = &actor->s();
            if (actor->hasX()) {
                XSPRITE *pXSprite = &actor->x();
                switch (pSprite->type) {
                    case kMarkerSPStart:
                        if (gGameOptions.nGameType < 2 && pXSprite->data1 >= 0 && pXSprite->data1 < kMaxPlayers) {
                            ZONE *pZone = &gStartZone[pXSprite->data1];
                            pZone->x = pSprite->pos.X;
                            pZone->y = pSprite->pos.Y;
                            pZone->z = pSprite->z;
                            pZone->sector = pSprite->sector();
                            pZone->ang = pSprite->ang;
                        }
                        DeleteSprite(actor);
                        break;
                    case kMarkerMPStart:
                        if (pXSprite->data1 >= 0 && pXSprite->data2 < kMaxPlayers) {
                            if (gGameOptions.nGameType >= 2) {
                                // default if BB or teams without data2 specified
                                ZONE* pZone = &gStartZone[pXSprite->data1];
                                pZone->x = pSprite->pos.X;
                                pZone->y = pSprite->pos.Y;
                                pZone->z = pSprite->z;
                                pZone->sector = pSprite->sector();
                                pZone->ang = pSprite->ang;
                            
                                #ifdef NOONE_EXTENSIONS
                                    // fill player spawn position according team of player in TEAMS mode.
                                    if (gModernMap && gGameOptions.nGameType == 3) {
                                        if (pXSprite->data2 == 1) {
                                            pZone = &gStartZoneTeam1[team1];
                                            pZone->x = pSprite->pos.X;
                                            pZone->y = pSprite->pos.Y;
                                            pZone->z = pSprite->z;
                                            pZone->sector = pSprite->sector();
                                            pZone->ang = pSprite->ang;
                                            team1++;

                                        } else if (pXSprite->data2 == 2) {
                                            pZone = &gStartZoneTeam2[team2];
                                            pZone->x = pSprite->pos.X;
                                            pZone->y = pSprite->pos.Y;
                                            pZone->z = pSprite->z;
                                            pZone->sector = pSprite->sector();
                                            pZone->ang = pSprite->ang;
                                            team2++;
                                        }
                                    }
                                #endif

                            }
                            DeleteSprite(actor);
                        }
                        break;
                    case kMarkerUpLink:
                        pSprite->sector()->upperLink = actor;
                        pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
                        pSprite->cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
                        break;
                    case kMarkerLowLink:
                        pSprite->sector()->lowerLink = actor;
                        pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
                        pSprite->cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
                        break;
                    case kMarkerUpWater:
                    case kMarkerUpStack:
                    case kMarkerUpGoo:
                        pSprite->sector()->upperLink = actor;
                        pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
                        pSprite->cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
                        pSprite->z = getflorzofslopeptr(pSprite->sector(), pSprite->pos.X, pSprite->pos.Y);
                        break;
                    case kMarkerLowWater:
                    case kMarkerLowStack:
                    case kMarkerLowGoo:
                        pSprite->sector()->lowerLink = actor;
                        pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
                        pSprite->cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
                        pSprite->z = getceilzofslopeptr(pSprite->sector(), pSprite->pos.X, pSprite->pos.Y);
                        break;
                }
            }
    }
    
    #ifdef NOONE_EXTENSIONS
    // check if there is enough start positions for teams, if any used
    if (team1 > 0 || team2 > 0) {
        gTeamsSpawnUsed = true;
        if (team1 < kMaxPlayers / 2 || team2 < kMaxPlayers / 2) {
            viewSetSystemMessage("At least 4 spawn positions for each team is recommended.");
            viewSetSystemMessage("Team A positions: %d, Team B positions: %d.", team1, team2);
        }
    }
    #endif

    for(auto& sect: sector)
    {
        auto actor = barrier_cast<DBloodActor*>(sect.upperLink);
        if (actor && actor->hasX())
        {
            spritetype *pSprite = &actor->s();
            XSPRITE *pXSprite = &actor->x();
            int nLink = pXSprite->data1;
            for(auto& sect: sector)
            {
                auto actor2 = barrier_cast<DBloodActor*>(sect.lowerLink);
                if (actor2 && actor2->hasX())
                {
                    spritetype *pSprite2 = &actor2->s();
                    XSPRITE *pXSprite2 = &actor2->x();
                    if (pXSprite2->data1 == nLink)
                    {
                        actor->SetOwner(actor2);
                        actor2->SetOwner(actor);
                    }
                }
            }
        }
    }
	validateLinks();
}

int CheckLink(DBloodActor *actor)
{
    auto pSprite = &actor->s();
    auto pSector = pSprite->sector();
    auto aUpper = barrier_cast<DBloodActor*>(pSector->upperLink);
    auto aLower = barrier_cast<DBloodActor*>(pSector->lowerLink);
    if (aUpper)
    {
        spritetype* pUpper = &aUpper->s();
        int z;
        if (pUpper->type == kMarkerUpLink)
            z = pUpper->z;
        else
            z = getflorzofslopeptr(pSprite->sector(), pSprite->pos.X, pSprite->pos.Y);
        if (z <= pSprite->z)
        {
            aLower = aUpper->GetOwner();
            assert(aLower);
            spritetype *pLower = &aLower->s();
            assert(pLower->insector());
            ChangeActorSect(actor, pLower->sector());
            pSprite->pos.X += pLower->pos.X - pUpper->pos.X;
            pSprite->pos.Y += pLower->pos.Y - pUpper->pos.Y;
            int z2;
            if (pLower->type == kMarkerLowLink)
                z2 = pLower->z;
            else
                z2 = getceilzofslopeptr(pSprite->sector(), pSprite->pos.X, pSprite->pos.Y);
            pSprite->z += z2-z;
            actor->interpolated = false;
            return pUpper->type;
        }
    }
    if (aLower)
    {
        spritetype *pLower = &aLower->s();
        int z;
        if (pLower->type == kMarkerLowLink)
            z = pLower->z;
        else
            z = getceilzofslopeptr(pSprite->sector(), pSprite->pos.X, pSprite->pos.Y);
        if (z >= pSprite->z)
        {
            aUpper = aLower->GetOwner();
            assert(aUpper);
            spritetype *pUpper = &aUpper->s();
            assert(pUpper->insector());
            ChangeActorSect(actor, pUpper->sector());
            pSprite->pos.X += pUpper->pos.X - pLower->pos.X;
            pSprite->pos.Y += pUpper->pos.Y - pLower->pos.Y;
            int z2;
            if (pUpper->type == kMarkerUpLink)
                z2 = pUpper->z;
            else
                z2 = getflorzofslopeptr(pSprite->sector(), pSprite->pos.X, pSprite->pos.Y);
            pSprite->z += z2-z;
            actor->interpolated = false;
            return pLower->type;
        }
    }
    return 0;
}

int CheckLink(int *x, int *y, int *z, sectortype** pSector)
{
    auto upper = barrier_cast<DBloodActor*>((*pSector)->upperLink);
    auto lower = barrier_cast<DBloodActor*>((*pSector)->lowerLink);
    if (upper)
    {
        spritetype *pUpper = &upper->s();
        int z1;
        if (pUpper->type == kMarkerUpLink)
            z1 = pUpper->z;
        else
            z1 = getflorzofslopeptr(*pSector, *x, *y);
        if (z1 <= *z)
        {
            lower = upper->GetOwner();
            assert(lower);
            spritetype *pLower = &lower->s();
            assert(pLower->insector());
            *pSector = pLower->sector();
            *x += pLower->pos.X - pUpper->pos.X;
            *y += pLower->pos.Y - pUpper->pos.Y;
            int z2;
            if (pUpper->type == kMarkerLowLink)
                z2 = pLower->z;
            else
                z2 = getceilzofslopeptr(*pSector, *x, *y);
            *z += z2-z1;
            return pUpper->type;
        }
    }
    if (lower)
    {
        spritetype *pLower = &lower->s();
        int z1;
        if (pLower->type == kMarkerLowLink)
            z1 = pLower->z;
        else
            z1 = getceilzofslopeptr(*pSector, *x, *y);
        if (z1 >= *z)
        {
            upper = lower->GetOwner();
            assert(upper);
            spritetype *pUpper = &upper->s();
			assert(pUpper);
            *pSector = pUpper->sector();
            *x += pUpper->pos.X - pLower->pos.X;
            *y += pUpper->pos.Y - pLower->pos.Y;
            int z2;
            if (pLower->type == kMarkerUpLink)
                z2 = pUpper->z;
            else
                z2 = getflorzofslopeptr(*pSector, *x, *y);
            *z += z2-z1;
            return pLower->type;
        }
    }
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, ZONE& w, ZONE* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("x", w.x)
			("y", w.y)
			("z", w.z)
			("sector", w.sector)
			("angle", w.ang)
			.EndObject();
	}
	return arc;
}

void SerializeWarp(FSerializer& arc)
{
	if (arc.BeginObject("warp"))
	{
		arc.Array("startzone", gStartZone, kMaxPlayers)
			.EndObject();
	}
}

END_BLD_NS
