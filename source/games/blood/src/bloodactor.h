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
		if (s().owner == -1) return nullptr;
		return base() + s().owner;
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


END_BLD_NS
