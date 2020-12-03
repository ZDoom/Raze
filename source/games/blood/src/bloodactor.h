#pragma once

BEGIN_BLD_NS

extern int cumulDamage[kMaxXSprites];

// Due to the messed up array storage of all the game data we cannot do any direct references here yet. We have to access everything via wrapper functions for now.
// Note that the indexing is very inconsistent - partially by sprite index, partially by xsprite index.
class DBloodActor
{
	int index;
    DBloodActor* base();

public:
    int dudeSlope;

    DBloodActor() :index(int(this - base())) { /*assert(index >= 0 && index < kMaxSprites);*/ }
    DBloodActor& operator=(const DBloodActor& other) = default;
	
	void Clear()
	{
		dudeSlope = 0;
	}
    bool hasX() { return sprite[index].extra > 0; }
	void addX()
	{
		if (s().extra == -1) dbInsertXSprite(s().index);
	}
	spritetype& s() { return sprite[index]; }
	XSPRITE& x() { return xsprite[sprite[index].extra]; }	// calling this does not validate the xsprite!
    SPRITEHIT& hit() { return gSpriteHit[sprite[index].extra]; }
    int& xvel() { return Blood::xvel[index]; }
    int& yvel() { return Blood::yvel[index]; }
    int& zvel() { return Blood::zvel[index]; }

    int& cumulDamage() { return Blood::cumulDamage[sprite[index].extra]; }
    DUDEEXTRA& dudeExtra() { return gDudeExtra[sprite[index].extra]; }
    SPRITEMASS& spriteMass() { return gSpriteMass[sprite[index].extra]; }
    GENDUDEEXTRA& genDudeExtra() { return Blood::gGenDudeExtra[index]; }
    POINT3D& basePoint() { return Blood::baseSprite[index]; }

	void SetOwner(DBloodActor* own)
	{
		s().owner = own? own->s().index : -1;
	}

	DBloodActor* GetOwner()
	{
		if (s().owner == -1 || s().owner == kMaxSprites-1) return nullptr;
		return base() + s().owner;
	}

	void SetTarget(DBloodActor* own)
	{
		x().target = own ? own->s().index : -1;
	}

	DBloodActor* GetTarget()
	{
		if (x().target == -1 || x().target == kMaxSprites - 1) return nullptr;
		return base() + x().target;
	}

	void SetSpecialOwner() // nnext hackery
	{
		s().owner = kMaxSprites - 1;
	}

	bool GetSpecialOwner()
	{
		return (s().owner == kMaxSprites - 1);
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

};

extern DBloodActor bloodActors[kMaxSprites];

inline DBloodActor* DBloodActor::base() { return bloodActors; }

// Iterator wrappers that return an actor pointer, not an index.
class BloodStatIterator : public StatIterator
{
public:
	BloodStatIterator(int stat) : StatIterator(stat)
	{
	}

	DBloodActor* Next()
	{
		int n = NextIndex();
		return n >= 0 ? &bloodActors[n] : nullptr;
	}

	DBloodActor* Peek()
	{
		int n = PeekIndex();
		return n >= 0 ? &bloodActors[n] : nullptr;
	}
};

class BloodSectIterator : public SectIterator
{
public:
	BloodSectIterator(int stat) : SectIterator(stat)
	{
	}

	DBloodActor* Next()
	{
		int n = NextIndex();
		return n >= 0 ? &bloodActors[n] : nullptr;
	}

	DBloodActor* Peek()
	{
		int n = PeekIndex();
		return n >= 0 ? &bloodActors[n] : nullptr;
	}
};

inline int DeleteSprite(DBloodActor* nSprite)
{
	if (nSprite) return DeleteSprite(nSprite->s().index);
	return 0;
}

inline void actBurnSprite(DBloodActor* pSource, DBloodActor* pTarget, int nTime)
{
	auto pXSprite = &pTarget->x();
	pXSprite->burnTime = ClipHigh(pXSprite->burnTime + nTime, sprite[pXSprite->reference].statnum == kStatDude ? 2400 : 1200);
	pXSprite->burnSource = pSource? pSource->s().index : -1;
}

inline void GetActorExtents(DBloodActor* actor, int* top, int* bottom)
{
	GetSpriteExtents(&actor->s(), top, bottom);
}

inline DBloodActor *PLAYER::fragger()
{
	return fraggerId == -1? nullptr : &bloodActors[fraggerId];
}

inline void PLAYER::setFragger(DBloodActor* actor)
{
	fraggerId = actor == nullptr ? -1 : actor->s().index;
}

END_BLD_NS
