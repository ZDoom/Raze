/*
** thingdef_data.cpp
**
** DECORATE data tables
**
**---------------------------------------------------------------------------
** Copyright 2002-2008 Christoph Oelckers
** Copyright 2004-2008 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "thingdef.h"
#include "gi.h"
#include "gstrings.h"
#include "v_font.h"
#include "menu.h"
#include "types.h"
#include "dictionary.h"
#include "savegamehelp.h"
#include "games/duke/src/duke3d.h"

using Duke3d::DDukeActor;

static TArray<FPropertyInfo*> properties;
static TArray<AFuncDesc> AFTable;
static TArray<FieldDesc> FieldTable;
extern int				BackbuttonTime;
extern float			BackbuttonAlpha;

//==========================================================================
//
// List of all flags
//
//==========================================================================

// [RH] Keep GCC quiet by not using offsetof on Actor types.
#define DEFINE_FLAG(prefix, name, type, variable) { (unsigned int)prefix##_##name, #name, (int)(size_t)&((type*)1)->variable - 1, sizeof(((type *)0)->variable), VARF_Native }
#define DEFINE_PROTECTED_FLAG(prefix, name, type, variable) { (unsigned int)prefix##_##name, #name, (int)(size_t)&((type*)1)->variable - 1, sizeof(((type *)0)->variable), VARF_Native|VARF_ReadOnly|VARF_InternalAccess }
#define DEFINE_FLAG2(symbol, name, type, variable) { (unsigned int)symbol, #name, (int)(size_t)&((type*)1)->variable - 1, sizeof(((type *)0)->variable), VARF_Native | VARF_Protected }
#define DEFINE_FLAG2d(symbol, name, type, variable) { (unsigned int)symbol, #name, (int)(size_t)&((type*)1)->variable - 1, sizeof(((type *)0)->variable), VARF_Native }
#define DEFINE_FLAG2_DEPRECATED(symbol, name, type, variable) { (unsigned int)symbol, #name, (int)(size_t)&((type*)1)->variable - 1, sizeof(((type *)0)->variable), VARF_Native|VARF_Deprecated }
#define DEFINE_DEPRECATED_FLAG(name) { DEPF_##name, #name, -1, 0, true }
#define DEFINE_DUMMY_FLAG(name, deprec) { DEPF_UNUSED, #name, -1, 0, deprec? VARF_Deprecated:0 }

// internal flags. These do not get exposed to actor definitions but scripts need to be able to access them as variables.
static FFlagDef InternalActorFlagDefs[]=
{
	DEFINE_FLAG2(CSTAT2_SPRITE_NOFIND, BLOCK, DCoreActor, spr.cstat2)
	// todo
};


static FFlagDef ActorFlagDefs[] =
{
	DEFINE_FLAG2d(CSTAT_SPRITE_BLOCK, BLOCK, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_TRANSLUCENT, TRANSLUCENT, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_XFLIP, XFLIP, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_YFLIP, YFLIP, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_ONE_SIDE, ONE_SIDE, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_ALIGNMENT_WALL, WALLSPRITE, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_ALIGNMENT_FLOOR, FLOORSPRITE, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_YCENTER, YCENTER, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_BLOCK_HITSCAN, BLOCK_HITSCAN, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_INVISIBLE, INVISIBLE, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_TRANS_FLIP, TRANS_FLIP, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_BLOCK_ALL, BLOCK_ALL, DCoreActor, spr.cstat),
	DEFINE_FLAG2(CSTAT2_SPRITE_MAPPED, MAPPED, DCoreActor, spr.cstat2),
	DEFINE_FLAG2(CSTAT2_SPRITE_NOSHADOW, NOSHADOW, DCoreActor, spr.cstat2),
	DEFINE_FLAG2(CSTAT2_SPRITE_DECAL, DECAL, DCoreActor, spr.cstat2),
	DEFINE_FLAG2(CSTAT2_SPRITE_FULLBRIGHT, FULLBRIGHT, DCoreActor, spr.cstat2),
	DEFINE_FLAG2(CSTAT2_SPRITE_NOANIMATE, NOANIMATE, DCoreActor, spr.cstat2),
	DEFINE_FLAG2(CSTAT2_SPRITE_NOMODEL, NOMODEL, DCoreActor, spr.cstat2),
};

	// These are here because we do not want to have a prefix on them.
static FFlagDef DukeActorFlagDefs[] =
{
	DEFINE_FLAG(SFLAG, INVENTORY, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, SHRINKAUTOAIM, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, BADGUY, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, FORCEAUTOAIM, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, BOSS, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, BADGUYSTAYPUT, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, GREENSLIMEFOOD, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, NOAUTOAIM, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, NOWATERDIP, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, INTERNAL_BADGUY, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, KILLCOUNT, DDukeActor, flags1),
	//DEFINE_FLAG(SFLAG, NOCANSEECHECK, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, HITRADIUSCHECK, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, LOOKALLAROUND, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, MOVEFTA_MAKESTANDABLE, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, TRIGGER_IFHITSECTOR, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, MOVEFTA_WAKEUPCHECK, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, LOOKALLAROUNDWITHPAL8, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, NOSHADOW, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, SE24_NOCARRY, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, NOINTERPOLATE, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, FALLINGFLAMMABLE, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, FLAMMABLEPOOLEFFECT, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, INFLAME, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, NOFLOORFIRE, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, HITRADIUS_CHECKHITONLY, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, HITRADIUS_FORCEEFFECT, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, CHECKSLEEP, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, NOTELEPORT, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, SE24_REMOVE, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, BLOCK_TRIPBOMB, DDukeActor, flags1),
	DEFINE_FLAG(SFLAG, NOFALLER, DDukeActor, flags1),

	DEFINE_FLAG(SFLAG2, USEACTIVATOR, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, NOROTATEWITHSECTOR, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, SHOWWALLSPRITEONMAP, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, NOFLOORPAL, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, EXPLOSIVE, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, BRIGHTEXPLODE, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, DOUBLEDMGTHRUST, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, BREAKMIRRORS, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, CAMERA, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, DONTANIMATE, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, ALTHITSCANDIRECTION, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, GREENBLOOD, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, ALWAYSROTATE1, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, DIENOW, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, TRANSFERPALTOJIBS, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, NORADIUSPUSH, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, FREEZEDAMAGE, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, REFLECTIVE, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, ALWAYSROTATE2, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, SPECIALAUTOAIM, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, NODAMAGEPUSH, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, IGNOREHITOWNER, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, DONTDIVE, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, FLOATING, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, PAL8OOZ, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, SPAWNRABBITGUTS, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, NONSMOKYROCKET, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, MIRRORREFLECT, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, ALTPROJECTILESPRITE, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, UNDERWATERSLOWDOWN, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, TRIGGERRESPAWN, DDukeActor, flags2),
	DEFINE_FLAG(SFLAG2, FORCESECTORSHADE, DDukeActor, flags2),

	DEFINE_FLAG(SFLAG3, DONTDIVEALIVE, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, BLOODY, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, BROWNBLOOD, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, LIGHTDAMAGE, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, FORCERUNCON, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, BIGHEALTH, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, NOGRAVITY, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, SIMPLEINIT, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, NOHITSCANHIT, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, SPECIALINIT, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, DONTLIGHTSHOOTER, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, SHOOTCENTERED, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, NOCEILINGBLAST, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, HITRADIUS_DONTHURTSHOOTER, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, HITRADIUS_NODAMAGE, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, HITRADIUS_NOEFFECT, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, HITRADIUS_DONTHURTSPECIES, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, ST3CONFINED, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, DONTENTERWATER, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, DONTENTERWATERONGROUND, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, RANDOMANGLEONWATER, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, NORANDOMANGLEWHENBLOCKED, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, QUICKALTERANG, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, SPAWNWEAPONDEBRIS, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, NOJIBS, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, NOVERTICALMOVE, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, MOVE_NOPLAYERINTERACT, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, MAGMAIMMUNE, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, DESTRUCTOIMMUNE, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, NOHITJIBS, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, CANHURTSHOOTER, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG3, NOSHOTGUNBLOOD, DDukeActor, flags3),
	DEFINE_FLAG(SFLAG4, DOUBLEHITDAMAGE, DDukeActor, flags4),
	DEFINE_FLAG(SFLAG4, NODAMAGETURN, DDukeActor, flags4),

};


static const struct FFlagList { const PClass * const *Type; FFlagDef *Defs; int NumDefs; int Use; } FlagLists[] =
{
	{ &RUNTIME_CLASS_CASTLESS(DCoreActor), 		ActorFlagDefs,		countof(ActorFlagDefs), 3 },
	{ &RUNTIME_CLASS_CASTLESS(DDukeActor), 		DukeActorFlagDefs,		countof(DukeActorFlagDefs), 3 },
	{ &RUNTIME_CLASS_CASTLESS(DCoreActor), 	InternalActorFlagDefs,	countof(InternalActorFlagDefs), 2 },
};
#define NUM_FLAG_LISTS (countof(FlagLists))

static FFlagDef forInternalFlags;

//==========================================================================
//
// Find a flag by name using a binary search
//
//==========================================================================
static FFlagDef *FindFlag (FFlagDef *flags, int numflags, const char *flag)
{
	int min = 0, max = numflags - 1;

	while (min <= max)
	{
		int mid = (min + max) / 2;
		int lexval = stricmp (flag, flags[mid].name);
		if (lexval == 0)
		{
			return &flags[mid];
		}
		else if (lexval > 0)
		{
			min = mid + 1;
		}
		else
		{
			max = mid - 1;
		}
	}
	return NULL;
}

//==========================================================================
//
// Finds a flag that may have a qualified name
//
//==========================================================================

FFlagDef *FindFlag (const PClass *type, const char *part1, const char *part2, bool strict)
{

	if (part2 == nullptr)
	{
		FStringf internalname("@flagdef@%s", part1);
		FName name(internalname, true);
		if (name != NAME_None)
		{
			auto field = dyn_cast<PPropFlag>(type->FindSymbol(name, true));
			if (field != nullptr && (!strict || !field->decorateOnly))
			{
				forInternalFlags.fieldsize = 4;
				forInternalFlags.name = "";
				forInternalFlags.flagbit = field->Offset? 1 << field->bitval : field->bitval;
				forInternalFlags.structoffset = field->Offset? (int)field->Offset->Offset : -1;
				forInternalFlags.varflags = field->Offset == nullptr && field->bitval > 0? VARF_Deprecated : 0;
				return &forInternalFlags;
			}
		}
	}
	else
	{
		FStringf internalname("@flagdef@%s.%s", part1, part2);
		FName name(internalname, true);
		if (name != NAME_None)
		{
			auto field = dyn_cast<PPropFlag>(type->FindSymbol(name, true));
			if (field != nullptr)
			{
				forInternalFlags.fieldsize = 4;
				forInternalFlags.name = "";
				forInternalFlags.flagbit = field->Offset ? 1 << field->bitval : field->bitval;
				forInternalFlags.structoffset = field->Offset ? (int)field->Offset->Offset : -1;
				forInternalFlags.varflags = field->Offset == nullptr && field->bitval > 0? VARF_Deprecated : 0;
				return &forInternalFlags;
			}
		}
	}

	// Not found. Try the internal flag definitions.


	FFlagDef *def;

	if (part2 == NULL)
	{ // Search all lists
		int max = strict ? 2 : NUM_FLAG_LISTS;
		for (int i = 0; i < max; ++i)
		{
			if ((FlagLists[i].Use & 1) && type->IsDescendantOf (*FlagLists[i].Type))
			{
				def = FindFlag (FlagLists[i].Defs, FlagLists[i].NumDefs, part1);
				if (def != NULL)
				{
					return def;
				}
			}
		}
	}
	else
	{ // Search just the named list
		for (size_t i = 0; i < NUM_FLAG_LISTS; ++i)
		{
			if (stricmp ((*FlagLists[i].Type)->TypeName.GetChars(), part1) == 0)
			{
				if (type->IsDescendantOf (*FlagLists[i].Type))
				{
					return FindFlag (FlagLists[i].Defs, FlagLists[i].NumDefs, part2);
				}
				else
				{
					return NULL;
				}
			}
		}
	}

	return NULL;
}


//==========================================================================
//
// Gets the name of an actor flag
//
//==========================================================================

const char *GetFlagName(unsigned int flagnum, int flagoffset)
{
	for(size_t i = 0; i < countof(ActorFlagDefs); i++)
	{
		if (ActorFlagDefs[i].flagbit == flagnum && ActorFlagDefs[i].structoffset == flagoffset)
		{
			return ActorFlagDefs[i].name;
		}
	}
	return "(unknown)";	// return something printable
}

//==========================================================================
//
// Find a property by name using a binary search
//
//==========================================================================

FPropertyInfo *FindProperty(const char * string)
{
	int min = 0, max = properties.Size()-1;

	while (min <= max)
	{
		int mid = (min + max) / 2;
		int lexval = stricmp (string, properties[mid]->name);
		if (lexval == 0)
		{
			return properties[mid];
		}
		else if (lexval > 0)
		{
			min = mid + 1;
		}
		else
		{
			max = mid - 1;
		}
	}
	return NULL;
}

//==========================================================================
//
// Sorting helpers
//
//==========================================================================

static int flagcmp (const void * a, const void * b)
{
	return stricmp( ((FFlagDef*)a)->name, ((FFlagDef*)b)->name);
}

static int propcmp(const void * a, const void * b)
{
	return stricmp( (*(FPropertyInfo**)a)->name, (*(FPropertyInfo**)b)->name);
}

//==========================================================================
//
// Initialization
//
//==========================================================================
void InitImports();

void InitThingdef()
{
	// Some native types need size and serialization information added before the scripts get compiled.
	//auto secplanestruct = NewStruct("Secplane", nullptr, true);
	//secplanestruct->Size = sizeof(secplane_t);
	//secplanestruct->Align = alignof(secplane_t);

	auto sectorstruct = NewStruct("sectortype", nullptr, true);
	sectorstruct->Size = sizeof(sectortype);
	sectorstruct->Align = alignof(sectortype);
	NewPointer(sectorstruct, false)->InstallHandlers(
		[](FSerializer &ar, const char *key, const void *addr)
		{
			ar(key, *(sectortype **)addr);
		},
		[](FSerializer &ar, const char *key, void *addr)
		{
			Serialize<sectortype>(ar, key, *(sectortype **)addr, nullptr);
			return true;
		}
	);

	auto linestruct = NewStruct("walltype", nullptr, true);
	linestruct->Size = sizeof(walltype);
	linestruct->Align = alignof(walltype);
	NewPointer(linestruct, false)->InstallHandlers(
		[](FSerializer &ar, const char *key, const void *addr)
		{
			ar(key, *(walltype **)addr);
		},
		[](FSerializer &ar, const char *key, void *addr)
		{
			Serialize<walltype>(ar, key, *(walltype **)addr, nullptr);
			return true;
		}
	);

	auto collstruct = NewStruct("Collision", nullptr, true);
	collstruct->Size = sizeof(CollisionBase);
	collstruct->Align = alignof(CollisionBase);


	auto sidestruct = NewStruct("TSprite", nullptr, true);
	sidestruct->Size = sizeof(tspritetype);
	sidestruct->Align = alignof(tspritetype);
	// This may not be serialized

	// Sort the flag lists
	for (size_t i = 0; i < NUM_FLAG_LISTS; ++i)
	{
		qsort (FlagLists[i].Defs, FlagLists[i].NumDefs, sizeof(FFlagDef), flagcmp);
	}

	// Create a sorted list of properties
	if (properties.Size() == 0)
	{
		AutoSegs::Properties.ForEach([](FPropertyInfo* propertyInfo)
		{
			properties.Push(propertyInfo);
		});

		properties.ShrinkToFit();
		qsort(&properties[0], properties.Size(), sizeof(properties[0]), propcmp);
	}

	InitImports();
}

void SynthesizeFlagFields()
{
	// synthesize a symbol for each flag from the flag name tables to avoid redundant declaration of them.
	for (auto &fl : FlagLists)
	{
		auto cls = const_cast<PClass*>(*fl.Type);
		if (fl.Use & 2)
		{
			for (int i = 0; i < fl.NumDefs; i++)
			{
				if (fl.Defs[i].structoffset > 0) // skip the deprecated entries in this list
				{
					cls->VMType->AddNativeField(FStringf("b%s", fl.Defs[i].name), (fl.Defs[i].fieldsize == 4 ? TypeSInt32 : TypeSInt16), fl.Defs[i].structoffset, fl.Defs[i].varflags, fl.Defs[i].flagbit);
				}
			}
		}
	}
}
