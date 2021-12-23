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
	int xvel, yvel, zvel;
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

	DBloodActor& operator=(const DBloodActor& other) = default;

	void ClearContent() override
	{
		dudeSlope = 0;
		hit = {};
		dudeExtra = {};
		spriteMass = {};
		genDudeExtra = {};
		prevmarker = nullptr;
		ownerActor = nullptr;
		basePoint = {};
		xspr = {};
		hasx = false;
		interpolated = false;
		xvel = yvel = zvel = 0;
		explosionhackflag = false;
		interpolated = false;
		condition[0] = {};
		condition[1] = {};
		cumulDamage = 0;
		Super::ClearContent();
	}
	bool hasX() { return hasx; }
	void addX() { hasx = true; }

	XSPRITE& x() { return xspr; }	// calling this does not validate the xsprite!

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
		x().target = own;
	}

	DBloodActor* GetTarget()
	{
		return x().target;
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
		x().burnSource = own;
	}

	DBloodActor* GetBurnSource()
	{
		return x().burnSource;
	}

	void SetSpecialOwner() // nnext hackery
	{
		ownerActor = nullptr;
		s().owner = kMagicOwner;
	}

	bool GetSpecialOwner()
	{
		return  ownerActor == nullptr && (s().owner == kMagicOwner);
	}

	bool IsPlayerActor()
	{
		return s().type >= kDudePlayer1 && s().type <= kDudePlayer8;
	}

	bool IsDudeActor()
	{
		return s().type >= kDudeBase && s().type < kDudeMax;
	}

	bool IsItemActor()
	{
		return s().type >= kItemBase && s().type < kItemMax;
	}

	bool IsWeaponActor()
	{
		return s().type >= kItemWeaponBase && s().type < kItemWeaponMax;
	}

	bool IsAmmoActor()
	{
		return s().type >= kItemAmmoBase && s().type < kItemAmmoMax;
	}

	bool isActive() 
	{
		if (!hasX())
			return false;

		switch (x().aiState->stateType) 
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
	return bits[act->spr.sectno()];
}

inline bool IsTargetTeammate(DBloodActor* pSource, DBloodActor* pTarget)
{
	if (!pSource->IsPlayerActor())
		return false;
	PLAYER* pSourcePlayer = &gPlayer[pSource->spr.type - kDudePlayer1];
	return IsTargetTeammate(pSourcePlayer, pTarget);
}


END_BLD_NS
