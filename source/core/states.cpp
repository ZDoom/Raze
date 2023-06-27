/*
** p_states.cpp
** state management
**
**---------------------------------------------------------------------------
** Copyright 1998-2008 Randy Heit
** Copyright 2006-2008 Christoph Oelckers
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
**
*/
#include "coreactor.h"
#include "cmdlib.h"
#include "c_dispatch.h"
#include "v_text.h"
#include "thingdef.h"
#include "templates.h"
#include "states.h"
#include "texturemanager.h"


// stores indices for symbolic state labels for some old-style DECORATE functions.
FStateLabelStorage StateLabels;
TArray<FSpriteFrame> SpriteFrames;
TArray<FSpriteDef> SpriteDefs;

void InitSpriteDefs(const char** names, size_t count)
{
	SpriteDefs.Clear();
	SpriteDefs.Push({});

	// allocate one buffer for all sprite names.
	size_t maxlen = 0;
	for (size_t i = 0; i < count; i++)
	{
		size_t ll = strlen(names[i]);
		if (ll > maxlen) maxlen = ll;
	}
	TArray<char> work(maxlen + 4, true);

	for (size_t i = 0; i < count; i++)
	{
		FSpriteFrame swork[26]{};
		size_t ll = strlen(names[i]);
		memcpy(work.Data(), names[i], ll);
		work[ll] = '@';

		int maxframe = -1;
		for (int j = 0; j < 26; j++)
		{
			int bits = 0;
			FTextureID texs[9];
			work[ll + 1] = 'A' + j;
			work[ll + 3] = 0;

			for (int k = 0; k <= 8; k++)
			{
				work[ll + 2] = '0' + k;
				texs[k] = TexMan.CheckForTexture(work.Data(), ETextureType::Any);
				if (texs[k].isValid()) bits |= (1 << k);
			}
			// use the base name without frame extensions if no frame found.
			if (bits == 0)
			{
				texs[0] = TexMan.CheckForTexture(names[i], ETextureType::Any);
				if (texs[0].isValid()) bits = 1;
				else continue;
			}
			if (bits == 1)
			{
				swork[j].Texture[0] = texs[0];
				swork[j].RotMode = 1;
				maxframe = j;
			}
			else if (bits == 2)
			{
				swork[j].Texture[0] = texs[1];
				swork[j].RotMode = 1;
				maxframe = j;
			}
			else if (bits == 62)
			{
				memcpy(swork[j].Texture, &texs[1], 5 * sizeof(FTextureID));
				swork[j].RotMode = 5;
				maxframe = j;
			}
			else if (bits == 510)
			{
				memcpy(swork[j].Texture, &texs[1], 8 * sizeof(FTextureID));
				swork[j].RotMode = 8;
				maxframe = j;
			}
			else
			{
				Printf("Incomplete frames for sprite %s, frame %c\n", names[i], 'A' + j);
			}
		}
		maxframe++;
		auto pos = SpriteFrames.Reserve(maxframe);
		memcpy(&SpriteFrames[pos], swork, maxframe * sizeof(FSpriteFrame));
		FSpriteDef def = { names[i], (uint8_t)maxframe, (uint16_t)pos };
		SpriteDefs.Push(def);
	}
}


// Each state is owned by an actor. Actors can own any number of
// states, but a single state cannot be owned by more than one
// actor. States are archived by recording the actor they belong
// to and the index into that actor's list of states.


//==========================================================================
//
// This wraps everything needed to get a current sprite from a state into
// one single script function.
//
//==========================================================================

/*
DEFINE_ACTION_FUNCTION(FState, GetSpriteTexture)
{
	PARAM_SELF_STRUCT_PROLOGUE(FState);
	PARAM_INT(rotation);
	PARAM_INT(skin);
	PARAM_FLOAT(scalex);
	PARAM_FLOAT(scaley);

	spriteframe_t *sprframe;
	if (skin == 0)
	{
		sprframe = &SpriteFrames[sprites[self->sprite].spriteframes + self->GetFrame()];
	}
	else
	{
		sprframe = &SpriteFrames[sprites[Skins[skin].sprite].spriteframes + self->GetFrame()];
		scalex = Skins[skin].Scale.X;
		scaley = Skins[skin].Scale.Y;
	}
	if (numret > 0) ret[0].SetInt(sprframe->Texture[rotation].GetIndex());
	if (numret > 1) ret[1].SetInt(!!(sprframe->Flip & (1 << rotation)));
	if (numret > 2) ret[2].SetVector2(DVector2(scalex, scaley));
	return min(3, numret);
}
*/

bool FState::CallAction(DCoreActor* self)
{
	if (AnimatorPtr && *AnimatorPtr) self->callFunction(*AnimatorPtr);
	else if (ActionFunc) self->callFunction(ActionFunc);
	return true;
}


//==========================================================================
//
// Find the actor that a state belongs to.
//
//==========================================================================

PClassActor *FState::StaticFindStateOwner (const FState *state)
{
	for (unsigned int i = 0; i < PClassActor::AllActorClasses.Size(); ++i)
	{
		PClassActor *info = PClassActor::AllActorClasses[i];
		if (info->OwnsState(state))
		{
			return info;
		}
	}

	return nullptr;
}

//==========================================================================
//
// Find the actor that a state belongs to, but restrict the search to
// the specified type and its ancestors.
//
//==========================================================================

PClassActor *FState::StaticFindStateOwner (const FState *state, PClassActor *info)
{
	while (info != nullptr)
	{
		if (info->OwnsState(state))
		{
			return info;
		}
		info = ValidateActor(info->ParentClass);
	}

	return nullptr;
}

//==========================================================================
//
//
//==========================================================================

FString FState::StaticGetStateName(const FState *state, PClassActor *info)
{
	auto so = FState::StaticFindStateOwner(state);
	if (so == nullptr)
	{
		so = FState::StaticFindStateOwner(state, info);
	}
	if (so == nullptr)
	{
		return "<unknown>";
	}
	return FStringf("%s.%d", so->TypeName.GetChars(), int(state - so->GetStates()));
}

//==========================================================================
//
//
//==========================================================================

FStateLabel *FStateLabels::FindLabel (FName label)
{
	return const_cast<FStateLabel *>(BinarySearch<FStateLabel, FName>(Labels, NumLabels, &FStateLabel::Label, label));
}

void FStateLabels::Destroy ()
{
	for(int i = 0; i < NumLabels; i++)
	{
		if (Labels[i].Children != nullptr)
		{
			Labels[i].Children->Destroy();
			M_Free(Labels[i].Children);	// These are malloc'd, not new'd!
			Labels[i].Children = nullptr;
		}
	}
}


//==========================================================================
//
// Creates a list of names from a string. Dots are used as separator
//
//==========================================================================

TArray<FName> &MakeStateNameList(const char * fname)
{
	static TArray<FName> namelist(3);
	FName firstpart = NAME_None, secondpart = NAME_None;
	char *c;

	// Handle the old names for the existing death states
	char *name = copystring(fname);
	firstpart = strtok(name, ".");

	namelist.Clear();
	namelist.Push(firstpart);
	if (secondpart != NAME_None)
	{
		namelist.Push(secondpart);
	}

	while ((c = strtok(nullptr, ".")) != nullptr)
	{
		FName cc = c;
		namelist.Push(cc);
	}
	delete[] name;
	return namelist;
}

//===========================================================================
//
// FindState (multiple names version)
//
// Finds a state that matches as many of the supplied names as possible.
// A state with more names than those provided does not match.
// A state with fewer names can match if there are no states with the exact
// same number of names.
//
// The search proceeds like this. For the current class, keeping matching
// names until there are no more. If both the argument list and the state
// are out of names, it's an exact match, so return it. If the state still
// has names, ignore it. If the argument list still has names, remember it.
//
//===========================================================================

FState *PClassActor::FindState(int numnames, FName *names, bool exact) const
{
	FStateLabels *labels = GetStateLabels();
	FState *best = nullptr;

	if (labels != nullptr)
	{
		int count = 0;

		// Find the best-matching label for this class.
		while (labels != nullptr && count < numnames)
		{
			FName label = *names++;
			FStateLabel *slabel = labels->FindLabel(label);

			if (slabel != nullptr)
			{
				count++;
				labels = slabel->Children;
				best = slabel->State;
			}
			else
			{
				break;
			}
		}
		if (count < numnames && exact)
		{
			return nullptr;
		}
	}
	return best;
}

//==========================================================================
//
// Finds the state associated with the given string
//
//==========================================================================

FState *PClassActor::FindStateByString(const char *name, bool exact)
{
	TArray<FName> &namelist = MakeStateNameList(name);
	return FindState(namelist.Size(), &namelist[0], exact);
}

//==========================================================================
//
// Get a state pointer from a symbolic label
//
//==========================================================================

FState *FStateLabelStorage::GetState(int pos, PClassActor *cls, bool exact)
{
	if (pos >= 0x10000000)
	{
		return cls? cls->FindState(ENamedName(pos - 0x10000000)) : nullptr;
	}
	else if (pos > 0)
	{
		int val;
		pos = (pos - 1) * 4;
		memcpy(&val, &Storage[pos], sizeof(int));

		if (val == 0)
		{
			FState *state;
			memcpy(&state, &Storage[pos + sizeof(int)], sizeof(state));
			return state;
		}
		else if (cls != nullptr)
		{
			FName *labels = (FName*)&Storage[pos + sizeof(int)];
			return cls->FindState(val, labels, exact);
		}
	}
	return nullptr;
}

//==========================================================================
//
// State label conversion function for scripts
//
//==========================================================================

/*
DEFINE_ACTION_FUNCTION(AActor, FindState)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_INT(newstate);
	PARAM_BOOL(exact)
	ACTION_RETURN_STATE(StateLabels.GetState(newstate, self->GetClass(), exact));
}
*/

//==========================================================================
//
// Search one list of state definitions for the given name
//
//==========================================================================

FStateDefine *FStateDefinitions::FindStateLabelInList(TArray<FStateDefine> & list, FName name, bool create)
{
	for(unsigned i = 0; i<list.Size(); i++)
	{
		if (list[i].Label == name)
		{
			return &list[i];
		}
	}
	if (create)
	{
		FStateDefine def;
		def.Label = name;
		def.State = nullptr;
		def.DefineFlags = SDF_NEXT;
		return &list[list.Push(def)];
	}
	return nullptr;
}

//==========================================================================
//
// Finds the address of a state label given by name. 
// Adds the state label if it doesn't exist
//
//==========================================================================

FStateDefine *FStateDefinitions::FindStateAddress(const char *name)
{
	FStateDefine *statedef = nullptr;
	TArray<FName> &namelist = MakeStateNameList(name);
	TArray<FStateDefine> *statelist = &StateLabels;

	for(unsigned i = 0; i < namelist.Size(); i++)
	{
		statedef = FindStateLabelInList(*statelist, namelist[i], true);
		statelist = &statedef->Children;
	}
	return statedef;
}

//==========================================================================
//
// Adds a new state to the curremt list
//
//==========================================================================

void FStateDefinitions::SetStateLabel(const char *statename, FState *state, uint8_t defflags)
{
	FStateDefine *std = FindStateAddress(statename);
	std->State = state;
	std->DefineFlags = defflags;
}

//==========================================================================
//
// Adds a new state to the current list
//
//==========================================================================

void FStateDefinitions::AddStateLabel(const char *statename)
{
	intptr_t index = StateArray.Size();
	FStateDefine *std = FindStateAddress(statename);
	std->State = (FState *)(index+1);
	std->DefineFlags = SDF_INDEX;
	laststate = nullptr;
	lastlabel = index;
}

//==========================================================================
//
// Returns the index a state label points to. May only be called before
// installing states.
//
//==========================================================================

int FStateDefinitions::GetStateLabelIndex (FName statename)
{
	FStateDefine *std = FindStateLabelInList(StateLabels, statename, false);
	if (std == nullptr)
	{
		return -1;
	}
	assert((size_t)std->State <= StateArray.Size() + 1);
	return (int)((ptrdiff_t)std->State - 1);
}

//==========================================================================
//
// Finds the state associated with the given name
// returns nullptr if none found
//
//==========================================================================

FState *FStateDefinitions::FindState(const char * name)
{
	FStateDefine *statedef = nullptr;

	TArray<FName> &namelist = MakeStateNameList(name);

	TArray<FStateDefine> *statelist = &StateLabels;
	for(unsigned i = 0; i < namelist.Size(); i++)
	{
		statedef = FindStateLabelInList(*statelist, namelist[i], false);
		if (statedef == nullptr)
		{
			return nullptr;
		}
		statelist = &statedef->Children;
	}
	return statedef ? statedef->State : nullptr;
}

//==========================================================================
//
// Creates the final list of states from the state definitions
//
//==========================================================================

static int labelcmp(const void *a, const void *b)
{
	FStateLabel *A = (FStateLabel *)a;
	FStateLabel *B = (FStateLabel *)b;
	return ((int)A->Label.GetIndex() - (int)B->Label.GetIndex());
}

FStateLabels *FStateDefinitions::CreateStateLabelList(TArray<FStateDefine> & statelist)
{
	// First delete all empty labels from the list
	for (int i = statelist.Size() - 1; i >= 0; i--)
	{
		if (statelist[i].Label == NAME_None || (statelist[i].State == nullptr && statelist[i].Children.Size() == 0))
		{
			statelist.Delete(i);
		}
	}

	int count = statelist.Size();

	if (count == 0)
	{
		return nullptr;
	}
	FStateLabels *list = (FStateLabels*)M_Malloc(sizeof(FStateLabels)+(count-1)*sizeof(FStateLabel));
	list->NumLabels = count;

	for (int i=0;i<count;i++)
	{
		list->Labels[i].Label = statelist[i].Label;
		list->Labels[i].State = statelist[i].State;
		list->Labels[i].Children = CreateStateLabelList(statelist[i].Children);
	}
	qsort(list->Labels, count, sizeof(FStateLabel), labelcmp);
	return list;
}

//===========================================================================
//
// InstallStates
//
// Creates the actor's state list from the current definition
//
//===========================================================================

void FStateDefinitions::InstallStates(PClassActor *info, DCoreActor *defaults)
{
	if (defaults == nullptr)
	{
		I_Error("Called InstallStates without actor defaults in %s", info->TypeName.GetChars());
	}


	// First ensure we have a valid spawn state.
	/*
	FState *state = FindState("Spawn");

	if (state == nullptr)
	{
		// A nullptr spawn state will crash the engine so set it to something valid.
		SetStateLabel("Spawn", GetDefault<AActor>()->SpawnState);
	}
	*/

	auto &sl = info->ActorInfo()->StateList;
	if (sl != nullptr) 
	{
		sl->Destroy();
		M_Free(sl);
	}
	sl = CreateStateLabelList(StateLabels);
}

//===========================================================================
//
// MakeStateDefines
//
// Creates a list of state definitions from an existing actor
// Used by Dehacked to modify an actor's state list
//
//===========================================================================

void FStateDefinitions::MakeStateList(const FStateLabels *list, TArray<FStateDefine> &dest)
{
	dest.Clear();
	if (list != nullptr) for (int i = 0; i < list->NumLabels; i++)
	{
		FStateDefine def;

		def.Label = list->Labels[i].Label;
		def.State = list->Labels[i].State;
		def.DefineFlags = SDF_STATE;
		dest.Push(def);
		if (list->Labels[i].Children != nullptr)
		{
			MakeStateList(list->Labels[i].Children, dest[dest.Size()-1].Children);
		}
	}
}

void FStateDefinitions::MakeStateDefines(const PClassActor *cls)
{
	StateArray.Clear();
	laststate = nullptr;
	laststatebeforelabel = nullptr;
	lastlabel = -1;

	if (cls != nullptr && cls->GetStateLabels() != nullptr)
	{
		MakeStateList(cls->GetStateLabels(), StateLabels);
	}
	else
	{
		StateLabels.Clear();
	}
}

//===========================================================================
//
// AddStateDefines
//
// Adds a list of states to the current definitions
//
//===========================================================================

void FStateDefinitions::AddStateDefines(const FStateLabels *list)
{
	if (list != nullptr) for(int i = 0; i < list->NumLabels; i++)
	{
		if (list->Labels[i].Children == nullptr)
		{
			if (!FindStateLabelInList(StateLabels, list->Labels[i].Label, false))
			{
				FStateDefine def;

				def.Label = list->Labels[i].Label;
				def.State = list->Labels[i].State;
				def.DefineFlags = SDF_STATE;
				StateLabels.Push(def);
			}
		}
	}
}

//==========================================================================
//
// RetargetState(Pointer)s
//
// These functions are used when a goto follows one or more labels.
// Because multiple labels are permitted to occur consecutively with no
// intervening states, it is not enough to remember the last label defined
// and adjust it. So these functions search for all labels that point to
// the current position in the state array and give them a copy of the
// target string instead.
//
//==========================================================================

void FStateDefinitions::RetargetStatePointers (intptr_t count, const char *target, TArray<FStateDefine> & statelist)
{
	for(unsigned i = 0;i<statelist.Size(); i++)
	{
		if (statelist[i].State == (FState*)count && statelist[i].DefineFlags == SDF_INDEX)
		{
			if (target == nullptr)
			{
				statelist[i].State = nullptr;
				statelist[i].DefineFlags = SDF_STOP;
			}
			else
			{
				statelist[i].State = (FState *)copystring (target);
				statelist[i].DefineFlags = SDF_LABEL;
			}
		}
		if (statelist[i].Children.Size() > 0)
		{
			RetargetStatePointers(count, target, statelist[i].Children);
		}
	}
}

void FStateDefinitions::RetargetStates (intptr_t count, const char *target)
{
	RetargetStatePointers(count, target, StateLabels);
}


//==========================================================================
//
// ResolveGotoLabel
//
// Resolves any strings being stored in a state's NextState field
//
//==========================================================================

FState *FStateDefinitions::ResolveGotoLabel (PClassActor *mytype, char *name)
{
	PClassActor *type = mytype;
	FState *state;
	char *namestart = name;
	char *label, *offset, *pt;
	int v;

	// Check for classname
	if ((pt = strstr (name, "::")) != nullptr)
	{
		const char *classname = name;
		*pt = '\0';
		name = pt + 2;

		// The classname may either be "Super" to identify this class's immediate
		// superclass, or it may be the name of any class that this one derives from.
		if (stricmp (classname, "Super") == 0)
		{
			type = ValidateActor(type->ParentClass);
		}
		else
		{
			// first check whether a state of the desired name exists
			PClass *stype = PClass::FindClass (classname);
			if (stype == nullptr)
			{
				I_Error ("%s is an unknown class.", classname);
			}
			if (!stype->IsDescendantOf (RUNTIME_CLASS(DCoreActor)))
			{
				I_Error ("%s is not an actor class, so it has no states.", stype->TypeName.GetChars());
			}
			if (!stype->IsAncestorOf (type))
			{
				I_Error ("%s is not derived from %s so cannot access its states.",
					type->TypeName.GetChars(), stype->TypeName.GetChars());
			}
			if (type != stype)
			{
				type = static_cast<PClassActor *>(stype);
			}
		}
	}
	label = name;
	// Check for offset
	offset = nullptr;
	if ((pt = strchr (name, '+')) != nullptr)
	{
		*pt = '\0';
		offset = pt + 1;
	}
	v = offset ? (int)strtoll (offset, nullptr, 0) : 0;

	// Get the state's address.
	if (type == mytype)
	{
		state = FindState (label);
	}
	else
	{
		state = type->FindStateByString(label, true);
	}

	if (state != nullptr)
	{
		state += v;
	}
	else if (v != 0)
	{
		I_Error ("Attempt to get invalid state %s from actor %s.", label, type->TypeName.GetChars());
	}
	else
	{
		Printf (TEXTCOLOR_RED "Attempt to get invalid state %s from actor %s.\n", label, type->TypeName.GetChars());
	}
	delete[] namestart;		// free the allocated string buffer
	return state;
}

//==========================================================================
//
// FixStatePointers
//
// Fixes an actor's default state pointers.
//
//==========================================================================

void FStateDefinitions::FixStatePointers (PClassActor *actor, TArray<FStateDefine> & list)
{
	for (unsigned i = 0; i < list.Size(); i++)
	{
		if (list[i].DefineFlags == SDF_INDEX)
		{
			size_t v = (size_t)list[i].State;
			list[i].State = actor->GetStates() + v - 1;
			list[i].DefineFlags = SDF_STATE;
		}
		if (list[i].Children.Size() > 0)
		{
			FixStatePointers(actor, list[i].Children);
		}
	}
}

//==========================================================================
//
// ResolveGotoLabels
//
// Resolves an actor's state pointers that were specified as jumps.
//
//==========================================================================

void FStateDefinitions::ResolveGotoLabels (PClassActor *actor, TArray<FStateDefine> & list)
{
	for (unsigned i = 0; i < list.Size(); i++)
	{
		if (list[i].State != nullptr && list[i].DefineFlags == SDF_LABEL)
		{ // It's not a valid state, so it must be a label string. Resolve it.
			list[i].State = ResolveGotoLabel (actor, (char *)list[i].State);
			list[i].DefineFlags = SDF_STATE;
		}
		if (list[i].Children.Size() > 0) ResolveGotoLabels(actor, list[i].Children);
	}
}


//==========================================================================
//
// SetGotoLabel
//
// sets a jump at the current state or retargets a label
//
//==========================================================================

bool FStateDefinitions::SetGotoLabel(const char *string)
{
	// copy the text - this must be resolved later!
	if (laststate != nullptr)
	{ // Following a state definition: Modify it.
		laststate->NextState = (FState*)copystring(string);	
		laststate->DefineFlags = SDF_LABEL;
		laststatebeforelabel = nullptr;
		return true;
	}
	else if (lastlabel >= 0)
	{ // Following a label: Retarget it.
		RetargetStates (lastlabel+1, string);
		if (laststatebeforelabel != nullptr)
		{
			laststatebeforelabel->NextState = (FState*)copystring(string);	
			laststatebeforelabel->DefineFlags = SDF_LABEL;
			laststatebeforelabel = nullptr;
		}
		return true;
	}
	return false;
}

//==========================================================================
//
// SetStop
//
// sets a stop operation
//
//==========================================================================

bool FStateDefinitions::SetStop()
{
	if (laststate != nullptr)
	{
		laststate->DefineFlags = SDF_STOP;
		laststatebeforelabel = nullptr;
		return true;
	}
	else if (lastlabel >=0)
	{
		RetargetStates (lastlabel+1, nullptr);
		if (laststatebeforelabel != nullptr)
		{
			laststatebeforelabel->DefineFlags = SDF_STOP;
			laststatebeforelabel = nullptr;
		}
		return true;
	}
	return false;
}

//==========================================================================
//
// SetWait
//
// sets a wait or fail operation
//
//==========================================================================

bool FStateDefinitions::SetWait()
{
	if (laststate != nullptr)
	{
		laststate->DefineFlags = SDF_WAIT;
		laststatebeforelabel = nullptr;
		return true;
	}
	return false;
}

//==========================================================================
//
// SetLoop
//
// sets a loop operation
//
//==========================================================================

bool FStateDefinitions::SetLoop()
{
	if (laststate != nullptr)
	{
		laststate->DefineFlags = SDF_INDEX;
		laststate->NextState = (FState*)(lastlabel+1);
		laststatebeforelabel = nullptr;
		return true;
	}
	return false;
}

//==========================================================================
//
// AddStates
//
// Adds some state to the current definition set. Returns the number of
// states added. Positive = no errors, negative = errors.
//
//==========================================================================

int FStateDefinitions::AddStates(FState *state, const char *framechars, const FScriptPosition &sc)
{
	bool error = false;
	int frame = 0;
	int count = 0;
	while (*framechars)
	{
		bool noframe = false;

		if (*framechars == '#')
			noframe = true;
		else if (*framechars == '^')
			frame = '\\' - 'A';
		else
			frame = (*framechars & 223) - 'A';

		framechars++;
		if (frame < 0 || frame > 28)
		{
			frame = 0;
			error = true;
		}

		state->Frame = frame;
		StateArray.Push(*state);
		SourceLines.Push(sc);
		++count;
	}
	laststate = &StateArray[StateArray.Size() - 1];
	laststatebeforelabel = laststate;
	return !error ? count : -count;
}

//==========================================================================
//
// FinishStates
// copies a state block and fixes all state links using the current list of labels
//
//==========================================================================

int FStateDefinitions::FinishStates(PClassActor *actor)
{
	int count = StateArray.Size();

	if (count > 0)
	{
		FState *realstates = (FState*)ClassDataAllocator.Alloc(count * sizeof(FState));
		int i;

		memcpy(realstates, &StateArray[0], count*sizeof(FState));
		actor->ActorInfo()->OwnedStates = realstates;
		actor->ActorInfo()->NumOwnedStates = count;
//		SaveStateSourceLines(realstates, SourceLines); todo

		// adjust the state pointers
		// In the case new states are added these must be adjusted, too!
		FixStatePointers(actor, StateLabels);

		// Fix state pointers that are gotos
		ResolveGotoLabels(actor, StateLabels);

		for (i = 0; i < count; i++)
		{
			// resolve labels and jumps
			switch (realstates[i].DefineFlags)
			{
			case SDF_STOP:		// stop
				realstates[i].NextState = nullptr;
				break;

			case SDF_WAIT:		// wait
				realstates[i].NextState = &realstates[i];
				break;

			case SDF_NEXT:		// next
				realstates[i].NextState = (i < count-1 ? &realstates[i+1] : &realstates[0]);
				break;

			case SDF_INDEX:		// loop
				realstates[i].NextState = &realstates[(size_t)realstates[i].NextState-1];
				break;

			case SDF_LABEL:
				realstates[i].NextState = ResolveGotoLabel(actor, (char *)realstates[i].NextState);
				break;
			}
		}
	}
	else
	{
		// Fix state pointers that are gotos
		ResolveGotoLabels(actor, StateLabels);
	}
	return count;
}



//==========================================================================
//
// Prints all state label info to the logfile
//
//==========================================================================

void DumpStateHelper(FStateLabels *StateList, const FString &prefix)
{
	for (int i = 0; i < StateList->NumLabels; i++)
	{
		if (StateList->Labels[i].State != nullptr)
		{
			const PClassActor *owner = FState::StaticFindStateOwner(StateList->Labels[i].State);
			if (owner == nullptr)
			{
				Printf(PRINT_LOG, "%s%s: invalid\n", prefix.GetChars(), StateList->Labels[i].Label.GetChars());
			}
			else
			{
				Printf(PRINT_LOG, "%s%s: %s\n", prefix.GetChars(), StateList->Labels[i].Label.GetChars(), FState::StaticGetStateName(StateList->Labels[i].State).GetChars());
			}
		}
		if (StateList->Labels[i].Children != nullptr)
		{
			DumpStateHelper(StateList->Labels[i].Children, prefix + '.' + StateList->Labels[i].Label.GetChars());
		}
	}
}

CCMD(dumpstates)
{
	for (unsigned int i = 0; i < PClassActor::AllActorClasses.Size(); ++i)
	{
		PClassActor *info = PClassActor::AllActorClasses[i];
		Printf(PRINT_LOG, "State labels for %s\n", info->TypeName.GetChars());
		DumpStateHelper(info->GetStateLabels(), "");
		Printf(PRINT_LOG, "----------------------------\n");
	}
}

//==========================================================================
//
// sets up the script-side version of states
//
//==========================================================================

DEFINE_FIELD(FState, NextState)
DEFINE_FIELD(FState, sprite)
DEFINE_FIELD(FState, Tics)
DEFINE_FIELD_BIT(FState, StateFlags, bFullbright, STF_FULLBRIGHT)



TArray<VMValue> actionParams;

/*
bool FState::CallAction(DCoreActor *self, FStateParamInfo *info)
{
	if (ActionFunc != nullptr)
	{
		ActionCycles.Clock();

		VMReturn ret;
		ret.PointerAt((void **)stateret);
		try
		{

			VMValue params[3] = { self, stateowner, VMValue(info) };
			VMCallAction(ActionFunc, params, ActionFunc->ImplicitArgs, &ret, stateret != nullptr);
		}
		catch (CVMAbortException &err)
		{
			err.MaybePrintMessage();

			if (stateowner != nullptr)
			{
				const char *callinfo = "";
				if (info != nullptr && info->mStateType == STATE_Psprite)
				{
					if (stateowner->IsKindOf(NAME_Weapon) && stateowner != self) callinfo = "weapon ";
					else callinfo = "overlay ";
				}
				err.stacktrace.AppendFormat("Called from %sstate %s in %s\n", callinfo, FState::StaticGetStateName(this).GetChars(), stateowner->GetClass()->TypeName.GetChars());
			}
			else
			{
				err.stacktrace.AppendFormat("Called from state %s\n", FState::StaticGetStateName(this).GetChars());
			}

			throw;
		}

		ActionCycles.Unclock();
		return true;
	}
	else
	{
		return false;
	}
}
*/

//==========================================================================
//
//
//==========================================================================

int GetSpriteIndex(const char * spritename, bool add)
{
	return 0;
}

/*
DEFINE_ACTION_FUNCTION(FState, ValidateSpriteFrame)
{
	PARAM_SELF_STRUCT_PROLOGUE(FState);
	ACTION_RETURN_BOOL(self->Frame < sprites[self->sprite].numframes);
}
*/


