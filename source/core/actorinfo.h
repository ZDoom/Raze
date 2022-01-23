#pragma once

#include <stddef.h>
#include <stdint.h>

#include "dobject.h"
#include "m_fixed.h"
#include "m_random.h"

class FScanner;
class FInternalLightAssociation;

struct FActorInfo
{
	TArray<FInternalLightAssociation *> LightAssociations;
	PClassActor *Replacement = nullptr;
	PClassActor *Replacee = nullptr;
	int TypeNum = -1;

	FActorInfo() = default;
	FActorInfo(const FActorInfo & other) = delete;
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

