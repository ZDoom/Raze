#pragma once

#include <stddef.h>
#include <stdint.h>

#include "maptypes.h"
#include "dobject.h"
#include "m_fixed.h"
#include "m_random.h"

class FScanner;
class FInternalLightAssociation;

enum EDefaultFlags
{
	DEFF_PICNUM = 1,
	DEFF_STATNUM = 2,
};

struct FActorInfo
{
	TArray<FInternalLightAssociation *> LightAssociations;
	TArray<int> SpriteSet;
	PClassActor *Replacement = nullptr;
	PClassActor *Replacee = nullptr;
	int TypeNum = -1;
	int DefaultFlags = 0;
	int DefaultCstat = 0;
	int Health = 0;	// not used yet - this will stand in if no CON defines a health value for Duke.
	FName DamageType = NAME_None;	// damage type this item inflicts

	// these are temporary. Due to how Build games handle their tiles, we cannot look up the textures when scripts are being parsed.
	TArray<FString> SpriteSetNames;

	FActorInfo() = default;
	FActorInfo(const FActorInfo& other)
	{
		// only copy the fields that get inherited
		TypeNum = other.TypeNum;
		DefaultFlags = other.DefaultFlags;
		DefaultCstat = other.DefaultCstat;
		SpriteSetNames = other.SpriteSetNames;
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

	// For those times when being able to scan every kind of actor is convenient
	inline static TArray<PClassActor *> AllActorClasses;
};

