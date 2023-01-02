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
#include "view.h"
#include "g_input.h"
#include "precache.h"

BEGIN_BLD_NS

int nPrecacheCount;

void fxPrecache();
void gibPrecache();


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void tilePrecacheTile(int nTile, int nType, int palette)
{
	auto nTex = tileGetTextureID(nTile);
	int n = 1;
	switch (GetExtInfo(nTex).picanm.extra & 7)
	{
	case 0:
		n = 1;
		break;
	case 1:
		n = 5;
		break;
	case 2:
		n = 8;
		break;
	case 3:
		n = 2;
		break;
	}
	while (n--)
	{
		markTextureForPrecache(nTex, palette);
		nTex = nTex + 1 + GetExtInfo(nTex).picanm.num;
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PrecacheDude(DBloodActor* actor)
{
	int palette = actor->spr.pal;
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	seqPrecacheId(pDudeInfo->seqStartID, palette);
	seqPrecacheId(pDudeInfo->seqStartID + 5, palette);
	seqPrecacheId(pDudeInfo->seqStartID + 1, palette);
	seqPrecacheId(pDudeInfo->seqStartID + 2, palette);
	switch (actor->spr.type)
	{
	case kDudeCultistTommy:
	case kDudeCultistShotgun:
	case kDudeCultistTesla:
	case kDudeCultistTNT:
		seqPrecacheId(pDudeInfo->seqStartID + 6, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 7, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 8, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 9, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 13, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 14, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 15, palette);
		break;
	case kDudeZombieButcher:
	case kDudeGillBeast:
		seqPrecacheId(pDudeInfo->seqStartID + 6, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 7, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 8, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 9, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 10, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 11, palette);
		break;
	case kDudeGargoyleStatueFlesh:
	case kDudeGargoyleStatueStone:
		seqPrecacheId(pDudeInfo->seqStartID + 6, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 6, palette); //???
		[[fallthrough]];
	case kDudeGargoyleFlesh:
	case kDudeGargoyleStone:
		seqPrecacheId(pDudeInfo->seqStartID + 6, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 7, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 8, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 9, palette);
		break;
	case kDudePhantasm:
	case kDudeHellHound:
	case kDudeSpiderBrown:
	case kDudeSpiderRed:
	case kDudeSpiderBlack:
	case kDudeSpiderMother:
	case kDudeTchernobog:
		seqPrecacheId(pDudeInfo->seqStartID + 6, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 7, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 8, palette);
		break;
	case kDudeCerberusTwoHead:
		seqPrecacheId(pDudeInfo->seqStartID + 6, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 7, palette);
		[[fallthrough]];
	case kDudeHand:
	case kDudeBoneEel:
	case kDudeBat:
	case kDudeRat:
		seqPrecacheId(pDudeInfo->seqStartID + 6, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 7, palette);
		break;
	case kDudeCultistBeast:
		seqPrecacheId(pDudeInfo->seqStartID + 6, palette);
		break;
	case kDudeZombieAxeBuried:
		seqPrecacheId(pDudeInfo->seqStartID + 12, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 9, palette);
		[[fallthrough]];
	case kDudeZombieAxeLaying:
		seqPrecacheId(pDudeInfo->seqStartID + 10, palette);
		[[fallthrough]];
	case kDudeZombieAxeNormal:
		seqPrecacheId(pDudeInfo->seqStartID + 6, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 7, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 8, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 11, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 13, palette);
		seqPrecacheId(pDudeInfo->seqStartID + 14, palette);
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PrecacheThing(DBloodActor* actor)
{
	int palette = actor->spr.pal;
	switch (actor->spr.type) {
	case kThingGlassWindow: // worthless...
	case kThingFluorescent:
		seqPrecacheId(12, palette);
		break;
	case kThingSpiderWeb:
		seqPrecacheId(15, palette);
		break;
	case kThingMetalGrate:
		seqPrecacheId(21, palette);
		break;
	case kThingFlammableTree:
		seqPrecacheId(25, palette);
		seqPrecacheId(26, palette);
		break;
	case kTrapMachinegun:
		seqPrecacheId(38, palette);
		seqPrecacheId(40, palette);
		seqPrecacheId(28, palette);
		break;
	case kThingObjectGib:
		//case kThingObjectExplode: weird that only gib object is precached and this one is not
		break;
	}
	tilePrecacheTile(actor->spr.picnum, -1, palette);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PreloadCache()
{
	if (!r_precache) return;
	int skyTile = -1;
	// Fonts
	precacheMap();

	BloodSpriteIterator it;
	while (auto actor = it.Next())
	{
		switch (actor->spr.statnum)
		{
		case kStatDude:
			PrecacheDude(actor);
			break;
		case kStatThing:
			PrecacheThing(actor);
			break;
		default:
			tilePrecacheTile(actor->spr.picnum, -1, actor->spr.pal);
			break;
		}
	}

	// Precache common SEQs
	for (int i = 0; i < 100; i++)
	{
		seqPrecacheId(i, 0);
	}

	tilePrecacheTile(1147, -1, 0); // water drip
	tilePrecacheTile(1160, -1, 0); // blood drip

	// Player SEQs
	seqPrecacheId(dudeInfo[31].seqStartID + 6, 0);
	seqPrecacheId(dudeInfo[31].seqStartID + 7, 0);
	seqPrecacheId(dudeInfo[31].seqStartID + 8, 0);
	seqPrecacheId(dudeInfo[31].seqStartID + 9, 0);
	seqPrecacheId(dudeInfo[31].seqStartID + 10, 0);
	seqPrecacheId(dudeInfo[31].seqStartID + 14, 0);
	seqPrecacheId(dudeInfo[31].seqStartID + 15, 0);
	seqPrecacheId(dudeInfo[31].seqStartID + 12, 0);
	seqPrecacheId(dudeInfo[31].seqStartID + 16, 0);
	seqPrecacheId(dudeInfo[31].seqStartID + 17, 0);
	seqPrecacheId(dudeInfo[31].seqStartID + 18, 0);

	/* fixme: cache the composite sky. These are useless.
	for (auto& sect : sector)
	{
		if ((sect.ceilingstat & CSTAT_SECTOR_SKY) != 0 && skyTile == -1)
			skyTile = sect.ceilingtexture;
	}
	if (skyTile > -1 && skyTile < kMaxTiles)
	{
		for (int i = 1; i < gSkyCount; i++)
			tilePrecacheTile(skyTile + i, 0, 0);
	}
	*/

	WeaponPrecache();
	fxPrecache();
	gibPrecache();

	I_GetEvent();
	precacheMarkedTiles();
}

END_BLD_NS

