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
** 4. When not used as part of ZDoom or a ZDoom derivative, this code will be
**    covered by the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or (at
**    your option) any later version.
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


static FFlagDef ActorFlagDefs[]=
{
	DEFINE_FLAG2d(CSTAT_SPRITE_BLOCK, BLOCK, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_TRANSLUCENT, TRANSLUCENT, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_XFLIP, XFLIP, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_YFLIP, YFLIP, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_ONE_SIDE, ONE_SIDE, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_YCENTER, YCENTER, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_BLOCK_HITSCAN, BLOCK_HITSCAN, DCoreActor, spr.cstat),
	DEFINE_FLAG2d(CSTAT_SPRITE_INVISIBLE, INVISIBLE, DCoreActor, spr.cstat),
	DEFINE_FLAG2(CSTAT2_SPRITE_MAPPED, MAPPED, DCoreActor, spr.cstat2),
	DEFINE_FLAG2(CSTAT2_SPRITE_NOSHADOW, NOSHADOW, DCoreActor, spr.cstat2),
	DEFINE_FLAG2(CSTAT2_SPRITE_DECAL, DECAL, DCoreActor, spr.cstat2),
	// todo: game specific flags
};


static const struct FFlagList { const PClass * const *Type; FFlagDef *Defs; int NumDefs; int Use; } FlagLists[] =
{
	{ &RUNTIME_CLASS_CASTLESS(DCoreActor), 		ActorFlagDefs,		countof(ActorFlagDefs), 3 },	// -1 to account for the terminator
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
		int max = strict ? 1 : NUM_FLAG_LISTS;
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

	auto sectorstruct = NewStruct("Sector", nullptr, true);
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

	auto linestruct = NewStruct("Wall", nullptr, true);
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
