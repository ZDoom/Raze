#pragma once

#include <stddef.h>
#include <stdint.h>

#include "maptypes.h"
#include "dobject.h"
#include "m_fixed.h"
#include "m_random.h"
#include "states.h"

class FScanner;
class FInternalLightAssociation;

enum EDefaultFlags
{
	DEFF_PICNUM = 1,
	DEFF_STATNUM = 2,
	DEFF_SCALE = 4,
};

struct FActorInfo
{
	TArray<FInternalLightAssociation *> LightAssociations;
	TArray<FTextureID> SpriteSet;
	PClassActor *Replacement = nullptr;
	PClassActor *Replacee = nullptr;
	DVector2 DefaultScale = { 0, 0 };
	int TypeNum = -1; // game specific identifier.
	int FirstAction = -1;
	int NumActions = 0;
	int FirstMove = -1;
	int NumMoves = 0;
	int FirstAI = -1;
	int NumAIs = 0;
	FName DefaultAction = NAME_Null;	// 'none' is valíd content here so use 'null' as 'not set'.
	FName DefaultMove = NAME_Null;
	int DefaultMoveflags = 0;

	int DefaultFlags = 0;
	int DefaultCstat = 0;
	FName DamageType = NAME_None;	// damage type this item inflicts
	TArray<PClassActor*> precacheClasses;

	// these are temporary. Due to how Build games handle their tiles, we cannot look up the textures when scripts are being parsed.
	TArray<FString> SpriteSetNames;

	FState* OwnedStates = nullptr;
	int NumOwnedStates = 0;
	FStateLabels* StateList = nullptr;

	FActorInfo() = default;
	FActorInfo(const FActorInfo& other)
	{
		// only copy the fields that get inherited
		TypeNum = other.TypeNum;
		DefaultFlags = other.DefaultFlags;
		DefaultCstat = other.DefaultCstat;
		SpriteSetNames = other.SpriteSetNames;
		DefaultScale = other.DefaultScale;
		DefaultAction = other.DefaultAction;
		DefaultMove = other.DefaultMove;
		DefaultMoveflags = other.DefaultMoveflags;
	}

	void ResolveTextures(const char* clsname, DCoreActor *defaults);

};

// No objects of this type will be created ever - its only use is to static_cast
// PClass to it.
class PClassActor : public PClass
{
protected:
public:
	static void StaticInit ();

	void BuildDefaults();
	void ApplyDefaults(uint8_t *defaults);
	bool SetReplacement(FName replaceName);
	void InitializeDefaults();

	FActorInfo *ActorInfo() const
	{
		return (FActorInfo*)Meta;
	}

	PClassActor *GetReplacement();
	PClassActor *GetReplacee();

	bool OwnsState(const FState* state) const 
	{
		auto i = ActorInfo();
		return i != nullptr && state >= i->OwnedStates && state < i->OwnedStates + i->NumOwnedStates;
	}

	FState* GetStates() const
	{
		return ActorInfo()->OwnedStates;
	}

	FStateLabels* GetStateLabels() const
	{
		return ActorInfo()->StateList;
	}

	FState* FindState(int numnames, FName* names, bool exact = false) const;
	FState* FindStateByString(const char* name, bool exact = false);
	FState* FindState(FName name) const
	{
		return FindState(1, &name);
	}


	// For those times when being able to scan every kind of actor is convenient
	inline static TArray<PClassActor *> AllActorClasses;
};
