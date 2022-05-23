#pragma once

#include "coreactor.h"

BEGIN_BLD_NS

class DBloodActor;

struct SPRITEHIT
{
	// These must use read barriers as they can live longer and need proper GC maintenance.
	Collision hit, ceilhit, florhit;
};

class DBloodActor : public DCoreActor
{
	DECLARE_CLASS(DBloodActor, DCoreActor)
	HAS_OBJECT_POINTERS

public:
	int dudeSlope;
	vec3_t vel;
	bool hasx;
	XSPRITE xspr;
	SPRITEHIT hit;
	DUDEEXTRA dudeExtra;
	SPRITEMASS spriteMass;
	GENDUDEEXTRA genDudeExtra;
	TObjPtr<DBloodActor*> prevmarker;	// needed by the nnext marker code. This originally hijacked targetX in XSPRITE
	TObjPtr<DBloodActor*> ownerActor;	// was previously stored in the sprite's owner field.
	POINT3D basePoint;
	EventObject condition[2];
	bool explosionhackflag; // this originally hijacked the target field which is not safe when working with pointers.

	// transient data (not written to savegame)
	int cumulDamage;
	bool interpolated;

	DBloodActor() = default;
	void Serialize(FSerializer& arc) override;
	size_t PropagateMark() override;

	bool hasX() { return hasx; }
	void addX() { hasx = true; }

	void SetOwner(DBloodActor* own)
	{
		ownerActor = own;
	}

	DBloodActor* GetOwner()
	{
		return ownerActor;
	}

	void SetTarget(DBloodActor* own)
	{
		xspr.target = own;
	}

	DBloodActor* GetTarget()
	{
		return xspr.target;
	}

	bool ValidateTarget(const char* func)
	{
		if (GetTarget() == nullptr)
		{
			Printf(PRINT_HIGH | PRINT_NOTIFY, "%s: invalid target in calling actor\n", func);
			return false;
		}
		return true;
	}

	void SetBurnSource(DBloodActor* own)
	{
		xspr.burnSource = own;
	}

	DBloodActor* GetBurnSource()
	{
		return xspr.burnSource;
	}

	void SetSpecialOwner() // nnext hackery
	{
		ownerActor = nullptr;
		spr.intowner = kMagicOwner;
	}

	bool GetSpecialOwner()
	{
		return  ownerActor == nullptr && (spr.intowner == kMagicOwner);
	}

	bool IsPlayerActor()
	{
		return spr.type >= kDudePlayer1 && spr.type <= kDudePlayer8;
	}

	bool IsDudeActor()
	{
		return spr.type >= kDudeBase && spr.type < kDudeMax;
	}

	bool IsItemActor()
	{
		return spr.type >= kItemBase && spr.type < kItemMax;
	}

	bool IsWeaponActor()
	{
		return spr.type >= kItemWeaponBase && spr.type < kItemWeaponMax;
	}

	bool IsAmmoActor()
	{
		return spr.type >= kItemAmmoBase && spr.type < kItemAmmoMax;
	}

	bool isActive()
	{
		if (!hasX())
			return false;

		switch (xspr.aiState->stateType)
		{
		case kAiStateIdle:
		case kAiStateGenIdle:
		case kAiStateSearch:
		case kAiStateMove:
		case kAiStateOther:
			return false;
		default:
			return true;
		}
	}
};

// subclassed to add a game specific actor() method

extern HitInfo gHitInfo;


// Iterator wrappers that return an actor pointer, not an index.

using BloodStatIterator = TStatIterator<DBloodActor>;
using BloodSectIterator = TSectIterator<DBloodActor>;
using BloodSpriteIterator = TSpriteIterator<DBloodActor>;

inline void GetActorExtents(DBloodActor* actor, int* top, int* bottom)
{
	GetSpriteExtents(&actor->spr, top, bottom);
}

inline bool CheckSector(const BitArray& bits, DBloodActor* act)
{
	return bits[act->sectno()];
}

inline bool IsTargetTeammate(DBloodActor* pSource, DBloodActor* pTarget)
{
	if (!pSource->IsPlayerActor())
		return false;
	PLAYER* pSourcePlayer = &gPlayer[pSource->spr.type - kDudePlayer1];
	return IsTargetTeammate(pSourcePlayer, pTarget);
}


END_BLD_NS
