/*
 * Copyright (C) 2018, 2022 nukeykt
 * Copyright (C) 2020-2023 Christoph Oelckers
 *
 * This file is part of Raze
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "ns.h"	// Must come before everything else!
#include "build.h"
#include "blood.h"
#include "view.h"
#include "g_input.h"
#include "precache.h"

BEGIN_BLD_NS

void fxPrecache();
void gibPrecache();


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void tilePrecacheTile(FTextureID nTex, int nType, int palette)
{
	int n = 1;
	if (!nTex.isValid()) return;
	switch (GetExtInfo(nTex).picanm.extra & 7)
	{
	case 1: n = 5; break;
	case 2: n = 8; break;
	case 3: n = 2; break;
	default: break;
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

extern TArray<int> preloadseqs;	// this is only useful for sequence IDs.


void PrecacheDude(DBloodActor* actor)
{
	auto cls = actor->GetClass();
	auto seqname = actor->seqStartName();
	int seqstart = actor->seqStartID();
	int palette = actor->spr.pal;
	if (seqname == NAME_None)
	{
		if (seqstart > 0)
		{
			while (cls->IsDescendantOf(BloodDudeBaseClass))
			{
				auto firstindex = static_cast<PClassActor*>(cls)->ActorInfo()->FirstAction;
				auto numseqs = static_cast<PClassActor*>(cls)->ActorInfo()->NumActions;
				for (int i = 0; i < numseqs; i++)
				{
					seqPrecacheId(NAME_None, seqstart + preloadseqs[i], palette);
				}
				cls = cls->ParentClass;
			}
		}
	}
	else
	{
		auto firstindex = static_cast<PClassActor*>(cls)->ActorInfo()->FirstAction;
		auto numseqs = static_cast<PClassActor*>(cls)->ActorInfo()->NumActions;
		if (numseqs > 0)
		{
			for (int i = 0; i < numseqs; i++)
			{
				seqPrecacheId(seqname, preloadseqs[i], palette);
			}
		}
		else
		{
			for (int i = 0; i < 100; i++) // cache everything with the given name.
			{
				seqPrecacheId(seqname, i, palette);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PrecacheThing(DBloodActor* actor)
{
	auto cls = actor->GetClass();
	auto seqname = actor->seqStartName();
	int palette = actor->spr.pal;
	if (seqname == NAME_None)
	{
		while (cls->IsDescendantOf(BloodThingBaseClass))
		{
			auto firstindex = static_cast<PClassActor*>(cls)->ActorInfo()->FirstAction;
			auto numseqs = static_cast<PClassActor*>(cls)->ActorInfo()->NumActions;
			for (int i = 0; i < numseqs; i++)
			{
				seqPrecacheId(NAME_None, preloadseqs[i], palette);
			}
			cls = cls->ParentClass;
		}
	}
	tilePrecacheTile(actor->spr.spritetexture(), -1, palette);
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
			tilePrecacheTile(actor->spr.spritetexture(), -1, actor->spr.pal);
			break;
		}
	}

	// On modern systems it makes sense to precache all of the common SEQs unconditionally.
	for (int i = 0; i < 100; i++)
	{
		seqPrecacheId(NAME_None, i, 0);
	}

	tilePrecacheTile(aTexIds[kTexWATERDRIP], -1, 0); // water drip
	tilePrecacheTile(aTexIds[kTexBLOODDRIP], -1, 0); // blood drip

	// Player SEQs
	auto playerdef = static_cast<DBloodActor*>(GetDefaultByType(PClass::FindActor("BloodPlayerBase")));
	int seqStartID = playerdef->seqStartID();
	seqPrecacheId(NAME_None, seqStartID + 6, 0);
	seqPrecacheId(NAME_None, seqStartID + 7, 0);
	seqPrecacheId(NAME_None, seqStartID + 8, 0);
	seqPrecacheId(NAME_None, seqStartID + 9, 0);
	seqPrecacheId(NAME_None, seqStartID + 10, 0);
	seqPrecacheId(NAME_None, seqStartID + 14, 0);
	seqPrecacheId(NAME_None, seqStartID + 15, 0);
	seqPrecacheId(NAME_None, seqStartID + 12, 0);
	seqPrecacheId(NAME_None, seqStartID + 16, 0);
	seqPrecacheId(NAME_None, seqStartID + 17, 0);
	seqPrecacheId(NAME_None, seqStartID + 18, 0);

	// fixme: this needs to cache the composite sky.

	WeaponPrecache();
	fxPrecache();
	gibPrecache();

	I_GetEvent();
	precacheMarkedTiles();
}

END_BLD_NS

