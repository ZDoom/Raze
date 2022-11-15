/*
** postprocessor.cpp
** Level postprocessing
**
**---------------------------------------------------------------------------
** Copyright 2009 Randy Heit
** Copyright 2009-2018 Christoph Oelckers
** Copyright 2019 Alexey Lysiuk
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
#include "dobject.h"
#include "vm.h"
#include "printf.h"
#include "types.h"
#include "maptypes.h"
#include "hw_sections.h"
#include "mapinfo.h"
#include "gamefuncs.h"

//==========================================================================
//
// PostProcessLevel
//
//==========================================================================

class DLevelPostProcessor : public DObject
{
	DECLARE_ABSTRACT_CLASS(DLevelPostProcessor, DObject)
public:
	SpawnSpriteDef* sprites;
};

IMPLEMENT_CLASS(DLevelPostProcessor, true, false);

void PostProcessLevel(const uint8_t* md4, const FString& mapname, SpawnSpriteDef& sprites)
{
	hw_ClearSplitSector();
	auto lc = Create<DLevelPostProcessor>();
	lc->sprites = &sprites;

	char md4string[33];
	for (int i = 0; i < 16; i++) mysnprintf(md4string + 2 * i, 3, "%02x", md4[i]);
	FName checksum(md4string, true);
	if (checksum == NAME_None) return;	// we do not have anything so save the work.

	for(auto cls : PClass::AllClasses)
	{
		if (cls->IsDescendantOf(RUNTIME_CLASS(DLevelPostProcessor)))
		{
			PFunction *const func = dyn_cast<PFunction>(cls->FindSymbol("Apply", false));
			if (func == nullptr)
			{
				Printf("Missing 'Apply' method in class '%s', level compatibility object ignored\n", cls->TypeName.GetChars());
				continue;
			}

			auto argTypes = func->Variants[0].Proto->ArgumentTypes;
			if (argTypes.Size() != 3 || argTypes[1] != TypeName || argTypes[2] != TypeString)
			{
				Printf("Wrong signature of 'Apply' method in class '%s', level compatibility object ignored\n", cls->TypeName.GetChars());
				continue;
			}

			VMValue param[] = { lc, checksum.GetIndex(), &mapname };
			VMCall(func->Variants[0].Implementation, param, 3, nullptr, 0);
		}
	}
}

DEFINE_ACTION_FUNCTION(DLevelPostProcessor, SetSpriteLotag)
{
	PARAM_SELF_PROLOGUE(DLevelPostProcessor);
	PARAM_UINT(sprite);
	PARAM_INT(lotag);
	if (sprite < self->sprites->sprites.Size())
		self->sprites->sprites[sprite].lotag = lotag;

	return 0;
}

DEFINE_ACTION_FUNCTION(DLevelPostProcessor, ChangeSpriteFlags)
{
	PARAM_SELF_PROLOGUE(DLevelPostProcessor);
	PARAM_UINT(sprite);
	PARAM_INT(clearmask);
	PARAM_INT(setmask);

	if (sprite < self->sprites->sprites.Size())
		self->sprites->sprites[sprite].cstat = (self->sprites->sprites[sprite].cstat & ~ESpriteFlags::FromInt(clearmask)) | ESpriteFlags::FromInt(setmask);

	return 0;
}

DEFINE_ACTION_FUNCTION(DLevelPostProcessor, SplitSector)
{
	PARAM_SELF_PROLOGUE(DLevelPostProcessor);
	PARAM_UINT(sectornum);
	PARAM_UINT(firstwall);
	PARAM_UINT(secondwall);

	if (sectornum < sector.Size())
	{
		unsigned sectstart = wallindex(sector[sectornum].walls.Data());
		if (firstwall >= sectstart && firstwall < sectstart + sector[sectornum].walls.Size() &&
			secondwall >= sectstart && secondwall < sectstart + sector[sectornum].walls.Size())

			hw_SetSplitSector(sectornum, firstwall, secondwall);
	}
	return 0;
}

DEFINE_ACTION_FUNCTION(DLevelPostProcessor, sw_serp_continue)
{
	PARAM_SELF_PROLOGUE(DLevelPostProcessor);
	currentLevel->gameflags |= LEVEL_SW_DEATHEXIT_SERPENT_NEXT;
	return 0;
}
