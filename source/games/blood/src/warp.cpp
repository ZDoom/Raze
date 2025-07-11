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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void validateLinks()
{
	int snum = 0;
	for (auto& sect : sector)
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


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void warpInit(TArray<DBloodActor*>& actors)
{
#ifdef NOONE_EXTENSIONS
	int team1 = 0; int team2 = 0; gTeamsSpawnUsed = false; // increment if team start positions specified.
#endif

	for (auto actor : actors)
	{
		if (!actor->exists()) continue;
		if (actor->hasX()) {
			switch (actor->spr.type) {
			case kMarkerSPStart:
				if (gGameOptions.nGameType < 2 && actor->xspr.data1 >= 0 && actor->xspr.data1 < kMaxPlayers) {
					ZONE* pZone = &gStartZone[actor->xspr.data1];
					pZone->pos = actor->spr.pos;
					pZone->sector = actor->sector();
					pZone->angle = actor->spr.Angles.Yaw;
				}
				DeleteSprite(actor);
				break;
			case kMarkerMPStart:
				if (actor->xspr.data1 >= 0 && actor->xspr.data2 < kMaxPlayers) {
					if (gGameOptions.nGameType >= 2) {
						// default if BB or teams without data2 specified
						ZONE* pZone = &gStartZone[actor->xspr.data1];
						pZone->pos = actor->spr.pos;
						pZone->sector = actor->sector();
						pZone->angle = actor->spr.Angles.Yaw;

#ifdef NOONE_EXTENSIONS
						// fill player spawn position according team of player in TEAMS mode.
						if (gModernMap && gGameOptions.nGameType == 3) {
							if (actor->xspr.data2 == 1) {
								pZone = &gStartZoneTeam1[team1];
								pZone->pos = actor->spr.pos;
								pZone->sector = actor->sector();
								pZone->angle = actor->spr.Angles.Yaw;
								team1++;

							}
							else if (actor->xspr.data2 == 2) {
								pZone = &gStartZoneTeam2[team2];
								pZone->pos = actor->spr.pos;
								pZone->sector = actor->sector();
								pZone->angle = actor->spr.Angles.Yaw;
								team2++;
							}
						}
#endif

					}
					DeleteSprite(actor);
				}
				break;
			case kMarkerUpLink:
				actor->sector()->upperLink = actor;
				actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
				actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
				break;
			case kMarkerLowLink:
				actor->sector()->lowerLink = actor;
				actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
				actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
				break;
			case kMarkerUpWater:
			case kMarkerUpStack:
			case kMarkerUpGoo:
				actor->sector()->upperLink = actor;
				actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
				actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
				actor->spr.pos.Z = getflorzofslopeptr(actor->sector(), actor->spr.pos);
				break;
			case kMarkerLowWater:
			case kMarkerLowStack:
			case kMarkerLowGoo:
				actor->sector()->lowerLink = actor;
				actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
				actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
				actor->spr.pos.Z = getceilzofslopeptr(actor->sector(), actor->spr.pos);
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

	for (auto& sect : sector)
	{
		auto actor = barrier_cast<DBloodActor*>(sect.upperLink);
		if (actor && actor->hasX())
		{
			int nLink = actor->xspr.data1;
			for (auto& isect : sector)
			{
				auto actor2 = barrier_cast<DBloodActor*>(isect.lowerLink);
				if (actor2 && actor2->hasX())
				{
					if (actor2->xspr.data1 == nLink)
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int CheckLink(DBloodActor* actor)
{
	auto pSector = actor->sector();
	auto aUpper = barrier_cast<DBloodActor*>(pSector->upperLink);
	auto aLower = barrier_cast<DBloodActor*>(pSector->lowerLink);
	if (aUpper)
	{
		double z;
		if (aUpper->spr.type == kMarkerUpLink)
			z = aUpper->spr.pos.Z;
		else
			z = getflorzofslopeptr(actor->sector(), actor->spr.pos);
		if (z <= actor->spr.pos.Z)
		{
			aLower = aUpper->GetOwner();
			assert(aLower);
			assert(aLower->insector());
			ChangeActorSect(actor, aLower->sector());
			actor->spr.pos += aLower->spr.pos.XY() - aUpper->spr.pos.XY();
			double z2;
			if (aLower->spr.type == kMarkerLowLink)
				z2 = aLower->spr.pos.Z;
			else
				z2 = getceilzofslopeptr(actor->sector(), actor->spr.pos);
			actor->spr.pos.Z += z2 - z;
			actor->interpolated = false;
			return aUpper->spr.type;
		}
	}
	if (aLower)
	{
		double z;
		if (aLower->spr.type == kMarkerLowLink)
			z = aLower->spr.pos.Z;
		else
			z = getceilzofslopeptr(actor->sector(), actor->spr.pos);
		if (z >= actor->spr.pos.Z)
		{
			aUpper = aLower->GetOwner();
			assert(aUpper);
			assert(aUpper->insector());
			ChangeActorSect(actor, aUpper->sector());
			actor->spr.pos += aUpper->spr.pos.XY() - aLower->spr.pos.XY();
			double z2;
			if (aUpper->spr.type == kMarkerUpLink)
				z2 = aUpper->spr.pos.Z;
			else
				z2 = getflorzofslopeptr(actor->sector(), actor->spr.pos);
			actor->spr.pos.Z += z2 - z;
			actor->interpolated = false;
			return aLower->spr.type;
		}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int CheckLink(DVector3& cPos, sectortype** pSector)
{
	auto aUpper = barrier_cast<DBloodActor*>((*pSector)->upperLink);
	auto aLower = barrier_cast<DBloodActor*>((*pSector)->lowerLink);
	if (aUpper)
	{
		double z1;
		if (aUpper->spr.type == kMarkerUpLink)
			z1 = aUpper->spr.pos.Z;
		else
			z1 = getflorzofslopeptr(*pSector, cPos);
		if (z1 <= cPos.Z)
		{
			aLower = aUpper->GetOwner();
			assert(aLower);
			assert(aLower->insector());
			*pSector = aLower->sector();
			cPos += aLower->spr.pos.XY() - aUpper->spr.pos.XY();
			double z2;
			if (aUpper->spr.type == kMarkerLowLink)
				z2 = aLower->spr.pos.Z;
			else
				z2 = getceilzofslopeptr(*pSector, cPos);
			cPos.Z += z2 - z1;
			return aUpper->spr.type;
		}
	}
	if (aLower)
	{
		double z1;
		if (aLower->spr.type == kMarkerLowLink)
			z1 = aLower->spr.pos.Z;
		else
			z1 = getceilzofslopeptr(*pSector, cPos);
		if (z1 >= cPos.Z)
		{
			aUpper = aLower->GetOwner();
			assert(aUpper);
			*pSector = aUpper->sector();
			cPos += aUpper->spr.pos.XY() - aLower->spr.pos.XY();
			double z2;
			if (aLower->spr.type == kMarkerUpLink)
				z2 = aUpper->spr.pos.Z;
			else
				z2 = getflorzofslopeptr(*pSector, cPos);
			cPos.Z += z2 - z1;
			return aLower->spr.type;
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
		arc("pos", w.pos)
			("sector", w.sector)
			("angle", w.angle)
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
