/*
** states.h
**
**---------------------------------------------------------------------------
** Copyright 1998-2007 Randy Heit
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

#ifndef __INFO_H__
#define __INFO_H__

#include <stddef.h>
#include <stdint.h>

#include "dobject.h"

struct Baggage;
class FScanner;
struct FActorInfo;
class FIntCVar;
class FStateDefinitions;
class FInternalLightAssociation;
struct FState;

struct FSpriteFrame
{
	FTextureID Texture[8];
	uint8_t RotMode;	// 1, 5, 8 are the regular ones. Duke also has mirrored 3 and 7 frame options.
};
struct FSpriteDef
{
	FName name;
	uint8_t numframes;
	uint16_t spriteframes;

	FTextureID GetSpriteFrame(int frame, int rot, DAngle ang, bool* mirror, bool flipagain = false);
};

extern TArray<FSpriteFrame> SpriteFrames;
extern TArray<FSpriteDef> SpriteDefs;

enum EStateDefineFlags
{
	SDF_NEXT = 0,
	SDF_STATE = 1,
	SDF_STOP = 2,
	SDF_WAIT = 3,
	SDF_LABEL = 4,
	SDF_INDEX = 5,
	SDF_MASK = 7,
};

enum EStateFlags
{
	STF_TICADJUST = 1,
	STF_FULLBRIGHT = 4,	// State is fullbright
};

enum EStateType : int // this must ensure proper alignment.
{
	STATE_Actor,
	STATE_Psprite,
	STATE_StateChain,
};

struct FStateParamInfo
{
	FState *mCallingState;
	EStateType mStateType;
	int mPSPIndex;
};


class VMNativeFunction;
class DCoreActor;
struct FState
{
	int     sprite;
	int     Frame;
	int     Tics;
#if 0

	uint8_t	StateFlags;
	uint8_t DefineFlags;
	uint8_t UseFlags;
	FState		*NextState;
	VMFunction	*ActionFunc; // called when an attached animation triggers an event. (i.e. Blood's SEQs. Should be made game independent.)
	VMFunction  *EnterFunc;  // called when entering the state.
	VMFunction  *TickFunc;   // called when ticking the state.
	VMFunction  *MoveFunc;   // called when moving the actor
	VMNativeFunction** AnimatorPtr;    // temporary kludge so we can define state arrays statically.
#else
	// this order is suboptimal but needed for SW's still existing in-code definitions.
	VMNativeFunction** AnimatorPtr;    // temporary kludge so we can define state arrays statically.
	FState* NextState;
	uint8_t	StateFlags;
	uint8_t DefineFlags;
	uint8_t UseFlags;
	VMFunction* ActionFunc; // called when an attached animation triggers an event. (i.e. Blood's SEQs. Should be made game independent.)
	VMFunction* EnterFunc;  // called when entering the state.
	VMFunction* TickFunc;   // called when ticking the state.
	VMFunction* MoveFunc;   // called when moving the actor
#endif
public:
	inline int GetFullbright() const
	{
		return (StateFlags & STF_FULLBRIGHT)? 0x10 /*RF_FULLBRIGHT*/ : 0;
	}
	inline int GetTics() const
	{
		return Tics;
	}
	inline FState *GetNextState() const
	{
		return NextState;
	}
	void SetAction(VMFunction *func) { ActionFunc = func; }
	void ClearAction() { ActionFunc = NULL; }
	bool CallAction(DCoreActor *self);

	static PClassActor *StaticFindStateOwner (const FState *state);
	static PClassActor *StaticFindStateOwner (const FState *state, PClassActor *info);
	static FString StaticGetStateName(const FState *state, PClassActor *info = nullptr);

};

// used during definition of AIState properties.
struct FDefiningState
{
	int     sprite;
	int     Type;
	int     Tics;
	FName Label;
	FName	NextStaten;
	VMFunction* ActionFunc; // called when an attached animation triggers an event. (i.e. Blood's SEQs. Should be made game independent.)
	VMFunction* EnterFunc;  // called when entering the state.
	VMFunction* TickFunc;   // called when ticking the state.
	VMFunction* MoveFunc;   // called when moving the actor

	// these are only here so we can use this struct directly in the first stage of transitioning the states in Blood.
	FDefiningState* NextState;
	int seqId;
};

struct FStateLabels;
struct FStateLabel
{
	FName Label;
	FState *State;
	FStateLabels *Children;
};

struct FStateLabels
{
	int NumLabels;
	FStateLabel Labels[1];

	FStateLabel *FindLabel (FName label);

	void Destroy();	// intentionally not a destructor!
};


struct FStateLabelStorage
{
	TArray<uint8_t> Storage;

	int AddPointer(FState *ptr)
	{
		if (ptr != nullptr)
		{
			int pos = Storage.Reserve(sizeof(ptr) + sizeof(int));
			memset(&Storage[pos], 0, sizeof(int));
			memcpy(&Storage[pos + sizeof(int)], &ptr, sizeof(ptr));
			return pos / 4 + 1;
		}
		else return 0;
	}

	int AddNames(TArray<FName> &names)
	{
		int siz = names.Size();
		if (siz > 1)
		{
			int pos = Storage.Reserve(sizeof(int) + sizeof(FName) * names.Size());
			memcpy(&Storage[pos], &siz, sizeof(int));
			memcpy(&Storage[pos + sizeof(int)], &names[0], sizeof(FName) * names.Size());
			return pos / 4 + 1;
		}
		else
		{
			// don't store single name states in the array.
			return names[0].GetIndex() + 0x10000000;
		}
	}

	FState *GetState(int pos, PClassActor *cls, bool exact = false);
};

extern FStateLabelStorage StateLabels;

int GetSpriteIndex(const char * spritename, bool add = true);
TArray<FName> &MakeStateNameList(const char * fname);
void AddStateLight(FState *state, const char *lname);
void InitSpriteDefs(const char** names, size_t count);


//==========================================================================
//
// State parser
//
//==========================================================================
class FxExpression;

struct FStateLabels;

struct FStateDefine
{
	FName Label;
	TArray<FStateDefine> Children;
	FState *State;
	uint8_t DefineFlags;
};

class FStateDefinitions
{
	TArray<FStateDefine> StateLabels;
	FState *laststate;
	FState *laststatebeforelabel;
	intptr_t lastlabel;
	TArray<FState> StateArray;
	TArray<FScriptPosition> SourceLines;

	static FStateDefine *FindStateLabelInList(TArray<FStateDefine> &list, FName name, bool create);
	static FStateLabels *CreateStateLabelList(TArray<FStateDefine> &statelist);
	static void MakeStateList(const FStateLabels *list, TArray<FStateDefine> &dest);
	static void RetargetStatePointers(intptr_t count, const char *target, TArray<FStateDefine> & statelist);
	FStateDefine *FindStateAddress(const char *name);
	FState *FindState(const char *name);

	FState *ResolveGotoLabel(PClassActor *mytype, char *name);
	static void FixStatePointers(PClassActor *actor, TArray<FStateDefine> & list);
	void ResolveGotoLabels(PClassActor *actor, TArray<FStateDefine> & list);
public:

	FStateDefinitions()
	{
		laststate = NULL;
		laststatebeforelabel = NULL;
		lastlabel = -1;
	}

	void SetStateLabel(const char *statename, FState *state, uint8_t defflags = SDF_STATE);
	void AddStateLabel(const char *statename);
	int GetStateLabelIndex (FName statename);
	void InstallStates(PClassActor *info, DCoreActor *defaults);
	int FinishStates(PClassActor *actor);

	void MakeStateDefines(const PClassActor *cls);
	void AddStateDefines(const FStateLabels *list);
	void RetargetStates (intptr_t count, const char *target);

	bool SetGotoLabel(const char *string);
	bool SetStop();
	bool SetWait();
	bool SetLoop();
	int AddStates(FState* state, const char* framechars, const FScriptPosition& sc);
	int GetStateCount() const { return StateArray.Size(); }
};


void SaveStateSourceLines(FState *firststate, TArray<FScriptPosition> &positions);
FScriptPosition & GetStateSource(FState *state);


#endif	// __INFO_H__
