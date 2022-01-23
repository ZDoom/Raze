/*
** actorinfo.cpp
** Keeps track of available actors and their states
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
** Copyright 2005-2022 Christoph Oelckers
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


#include "actorinfo.h"
#include "c_dispatch.h"
#include "d_net.h"
#include "v_text.h"

#include "gi.h"
#include "coreactor.h"
#include "stats.h"
#include "types.h"
#include "filesystem.h"
#include "texturemanager.h"

extern void LoadActors ();

cycle_t ActionCycles;


//==========================================================================
//
// special type for the native ActorInfo. This allows to let this struct
// be handled by the generic object constructors for the VM.
//
//==========================================================================

class PActorInfo : public PCompoundType
{
public:
	PActorInfo()
		:PCompoundType(sizeof(FActorInfo), alignof(FActorInfo))
	{
	}

	void SetDefaultValue(void *base, unsigned offset, TArray<FTypeAndOffset> *special) override
	{
		if (base != nullptr) new((uint8_t *)base + offset) FActorInfo;
		if (special != nullptr)
		{
			special->Push(std::make_pair(this, offset));
		}
	}

	void InitializeValue(void *addr, const void *def) const override
	{
		new(addr) FActorInfo;
	}

	void DestroyValue(void *addr) const override
	{
		FActorInfo *self = (FActorInfo*)addr;
		self->~FActorInfo();
	}

};

void AddActorInfo(PClass *cls)
{
	auto type = new PActorInfo;
	TypeTable.AddType(type, NAME_Actor);
	cls->AddField("*", type, VARF_Meta);
}


//==========================================================================
//
// PClassActor :: StaticInit										STATIC
//
//==========================================================================

void PClassActor::StaticInit()
{
	for (auto cls : AllClasses)
	{
		if (cls->IsDescendantOf(RUNTIME_CLASS(DCoreActor)))
		{
			AllActorClasses.Push(static_cast<PClassActor*>(cls));
		}
	}
}

//==========================================================================
//
// PClassActor :: SetReplacement
//
// Sets as a replacement class for another class.
//
//==========================================================================

bool PClassActor::SetReplacement(FName replaceName)
{
	// Check for "replaces"
	if (replaceName != NAME_None)
	{
		// Get actor name
		PClassActor *replacee = PClass::FindActor(replaceName);

		if (replacee == nullptr)
		{
			return false;
		}
		if (replacee != nullptr)
		{
			replacee->ActorInfo()->Replacement = this;
			ActorInfo()->Replacee = replacee;
		}
	}
	return true;
}

//==========================================================================
//
// PClassActor :: InitializeNativeDefaults
//
//==========================================================================

void PClassActor::InitializeDefaults()
{
	if (IsDescendantOf(RUNTIME_CLASS(DCoreActor)))
	{
		assert(Defaults == nullptr);
		Defaults = (uint8_t*)M_Malloc(Size);

		ConstructNative(Defaults);
		// We must unlink the defaults from the class list because it's just a static block of data to the engine.
		DObject* optr = (DObject*)Defaults;
		GC::Root = optr->ObjNext;
		optr->ObjNext = nullptr;
		optr->SetClass(this);

		// Copy the defaults from the parent but leave the DObject part alone because it contains important data.
		if (ParentClass->Defaults != nullptr)
		{
			memcpy(Defaults + sizeof(DObject), ParentClass->Defaults + sizeof(DObject), ParentClass->Size - sizeof(DObject));
			if (Size > ParentClass->Size)
			{
				memset(Defaults + ParentClass->Size, 0, Size - ParentClass->Size);
			}
		}
		else
		{
			memset(Defaults + sizeof(DObject), 0, Size - sizeof(DObject));
		}

		assert(MetaSize >= ParentClass->MetaSize);
		if (MetaSize != 0)
		{
			Meta = (uint8_t*)M_Malloc(MetaSize);

			// Copy the defaults from the parent but leave the DObject part alone because it contains important data.
			if (ParentClass->Meta != nullptr)
			{
				memcpy(Meta, ParentClass->Meta, ParentClass->MetaSize);
				if (MetaSize > ParentClass->MetaSize)
				{
					memset(Meta + ParentClass->MetaSize, 0, MetaSize - ParentClass->MetaSize);
				}
			}
			else
			{
				memset(Meta, 0, MetaSize);
			}

			if (MetaSize > 0) memcpy(Meta, ParentClass->Meta, ParentClass->MetaSize);
			else memset(Meta, 0, MetaSize);
		}
	}
	PClass::InitializeDefaults();
}

//==========================================================================
//
// PClassActor :: GetReplacement
//
//==========================================================================

PClassActor *PClassActor::GetReplacement()
{
	PClassActor *Replacement = ActorInfo()->Replacement;
	if (Replacement == nullptr) return this;
	return Replacement;
}

//==========================================================================
//
// PClassActor :: GetReplacee
//
//==========================================================================

PClassActor *PClassActor::GetReplacee()
{
	PClassActor *Replacee = ActorInfo()->Replacee;
	if (Replacee == nullptr) return this;
	return Replacee;
}

