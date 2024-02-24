/*
** serializer.cpp
** Savegame wrapper around RapidJSON
**
**---------------------------------------------------------------------------
** Copyright 2016 Christoph Oelckers
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

// The #defines here *MUST* match serializer.cpp, or we will get countless strange errors.
#define RAPIDJSON_48BITPOINTER_OPTIMIZATION 0	// disable this insanity which is bound to make the code break over time.
#define RAPIDJSON_HAS_CXX11_RVALUE_REFS 1
#define RAPIDJSON_HAS_CXX11_RANGE_FOR 1
#define RAPIDJSON_PARSE_DEFAULT_FLAGS kParseFullPrecisionFlag

#include <miniz.h>
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"
#include "serializer_raze.h"
#include "actorinfo.h"
#include "printf.h"
#include "utf8.h"
#include "coreactor.h"

#include "serializer_internal.h"


//==========================================================================
//
//
//
//==========================================================================

FSerializer &FRazeSerializer::Sprite(const char *key, int32_t &spritenum, int32_t *def)
{
#if 0
	if (isWriting())
	{
		if (w->inObject() && def != nullptr && *def == spritenum) return *this;
		WriteKey(key);
		w->String(sprites[spritenum].name);
	}
	else
	{
		auto val = r->FindKey(key);
		if (val != nullptr)
		{
			if (val->IsString())
			{
				FName name = val->GetString();
				for (auto hint = NumStdSprites; hint-- != 0; )
				{
					if (sprites[hint].dwName == name)
					{
						spritenum = hint;
						break;
					}
				}
			}
		}
	}
#endif
	return *this;
}

//==========================================================================
//
//
//
//==========================================================================

FSerializer& FRazeSerializer::StatePointer(const char* key, void* ptraddr, bool *res)
{
	if (isWriting())
	{
		if (res) *res = true;
		(*this)(key, *(FState**)ptraddr);
	}
	else
	{
		::Serialize(*this, key, *(FState**)ptraddr, nullptr, res);
	}
	return *this;
}




//==========================================================================
//
//
//
//==========================================================================

template<> FSerializer &Serialize(FSerializer &arc, const char *key, PClassActor *&clst, PClassActor **def)
{
	if (arc.isWriting())
	{
		if (!arc.w->inObject() || def == nullptr || clst != *def)
		{
			arc.WriteKey(key);
			if (clst == nullptr)
			{
				arc.w->Null();
			}
			else
			{
				arc.w->String(clst->TypeName.GetChars());
			}
		}
	}
	else
	{
		auto val = arc.r->FindKey(key);
		if (val != nullptr)
		{
			assert(val->IsString() || val->IsNull());
			if (val->IsString())
			{
				clst = PClass::FindActor(UnicodeToString(val->GetString()));
			}
			else if (val->IsNull())
			{
				clst = nullptr;
			}
			else
			{
				Printf(TEXTCOLOR_RED "string type expected for '%s'\n", key);
				clst = nullptr;
				arc.mErrors++;
			}
		}
	}
	return arc;

}

//==========================================================================
//
//
//
//==========================================================================

FSerializer &Serialize(FSerializer &arc, const char *key, FState *&state, FState **def, bool *retcode)
{
#if 0
	if (retcode) *retcode = false;
	if (arc.isWriting())
	{
		if (!arc.w->inObject() || def == nullptr || state != *def)
		{
			if (retcode) *retcode = true;
			arc.WriteKey(key);
			if (state == nullptr)
			{
				arc.w->Null();
			}
			else
			{
				PClassActor *info = FState::StaticFindStateOwner(state);

				if (info != NULL)
				{
					arc.w->StartArray();
					arc.w->String(info->TypeName.GetChars());
					arc.w->Uint((uint32_t)(state - info->GetStates()));
					arc.w->EndArray();
				}
				else
				{
					arc.w->Null();
				}
			}
		}
	}
	else
	{
		auto val = arc.r->FindKey(key);
		if (val != nullptr)
		{
			if (val->IsNull())
			{
				if (retcode) *retcode = true;
				state = nullptr;
			}
			else if (val->IsArray())
			{
				if (retcode) *retcode = true;
				const rapidjson::Value &cls = (*val)[0];
				const rapidjson::Value &ndx = (*val)[1];

				state = nullptr;
				assert(cls.IsString() && ndx.IsUint());
				if (cls.IsString() && ndx.IsUint())
				{
					PClassActor *clas = PClass::FindActor(UnicodeToString(cls.GetString()));
					if (clas && ndx.GetUint() < (unsigned)clas->GetStateCount())
					{
						state = clas->GetStates() + ndx.GetUint();
					}
					else
					{
						// this can actually happen by changing the DECORATE so treat it as a warning, not an error.
						state = nullptr;
						Printf(TEXTCOLOR_ORANGE "Invalid state '%s+%d' for '%s'\n", cls.GetString(), ndx.GetInt(), key);
					}
				}
				else
				{
					assert(false && "not a state");
					Printf(TEXTCOLOR_RED "data does not represent a state for '%s'\n", key);
					arc.mErrors++;
				}
			}
			else if (!retcode)
			{
				assert(false && "not an array");
				Printf(TEXTCOLOR_RED "array type expected for '%s'\n", key);
				arc.mErrors++;
			}
		}
	}
#endif
	return arc;

}

