#pragma once

BEGIN_BLD_NS

class DBloodActor;

// Wrapper around the insane collision info mess from Build.
struct Collision
{
	int type;
	int index;
	int legacyVal;	// should be removed later, but needed for converting back for unadjusted code.
	DBloodActor* actor;

	Collision() = default;
	Collision(int legacyval) { setFromEngine(legacyval); }

	// need forward declarations of these.
	int actorIndex(DBloodActor*);
	DBloodActor* Actor(int);

	int setNone()
	{
		type = kHitNone;
		index = -1;
		legacyVal = 0;
		actor = nullptr;
		return kHitNone;
	}

	int setSector(int num)
	{
		type = kHitSector;
		index = num;
		legacyVal = type | index;
		actor = nullptr;
		return kHitSector;
	}
	int setWall(int num)
	{
		type = kHitWall;
		index = num;
		legacyVal = type | index;
		actor = nullptr;
		return kHitWall;
	}
	int setSprite(DBloodActor* num)
	{
		type = kHitSprite;
		index = -1;
		legacyVal = type | actorIndex(num);
		actor = num;
		return kHitSprite;
	}

	int setFromEngine(int value)
	{
		legacyVal = value;
		type = value & kHitTypeMask;
		if (type == 0) { index = -1; actor = nullptr; }
		else if (type != kHitSprite) { index = value & kHitIndexMask; actor = nullptr; }
		else { index = -1; actor = Actor(value & kHitIndexMask); }
		return type;
	}

	walltype* wall()
	{
		assert(type == kHitWall);
		return &::wall[index];
	}

	sectortype* sector()
	{
		assert(type == kHitSector);
		return &::sector[index];
	}

};

struct SPRITEHIT
{
	Collision hit, ceilhit, florhit;
};


// Due to the messed up array storage of all the game data we cannot do any direct references here yet. We have to access everything via wrapper functions for now.
// Note that the indexing is very inconsistent - partially by sprite index, partially by xsprite index.
class DBloodActor
{
	int index;
	DBloodActor* base();

public:
	int dudeSlope;
	SPRITEHIT hit;
	DUDEEXTRA dudeExtra;
	SPRITEMASS spriteMass;
	GENDUDEEXTRA genDudeExtra;
	DBloodActor* prevmarker;	// needed by the nnext marker code. This originally hijacked targetX in XSPRITE

	int cumulDamage; // this one's transient and does not need to be saved.
	bool explosionhackflag; // this originally hijacked the target field which is not safe when working with pointers.

	DBloodActor() :index(int(this - base())) { /*assert(index >= 0 && index < kMaxSprites);*/ }
	DBloodActor& operator=(const DBloodActor& other) = default;

	void Clear()
	{
		explosionhackflag = false;
		prevmarker = nullptr;
		dudeSlope = 0;
		dudeExtra = {};
		spriteMass = {};
		hit = {};
	}
	bool hasX() { return sprite[index].extra > 0; }
	void addX()
	{
		if (s().extra == -1) dbInsertXSprite(s().index);
	}
	spritetype& s() { return sprite[index]; }
	XSPRITE& x() { return xsprite[sprite[index].extra]; }	// calling this does not validate the xsprite!
	int& xvel() { return Blood::xvel[index]; }
	int& yvel() { return Blood::yvel[index]; }
	int& zvel() { return Blood::zvel[index]; }
	int GetIndex() { return s().time; }	// For error printing only! This is only identical with the sprite index for items spawned at map start.

	POINT3D& basePoint() { return Blood::baseSprite[index]; }

	void SetOwner(DBloodActor* own)
	{
		s().owner = own ? own->s().index : -1;
	}

	DBloodActor* GetOwner()
	{
		if (s().owner == -1 || s().owner == kMaxSprites - 1) return nullptr;
		return base() + s().owner;
	}

	void SetTarget(DBloodActor* own)
	{
		x().target_i = own ? own->s().index : -1;
	}

	DBloodActor* GetTarget()
	{
		if (x().target_i <= -1 || x().target_i == kMaxSprites - 1) return nullptr;
		return base() + x().target_i;
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
		x().burnSource = own ? own->s().index : -1;
	}

	DBloodActor* GetBurnSource()
	{
		if (x().burnSource == -1 || x().burnSource == kMaxSprites - 1) return nullptr;
		return base() + x().burnSource;
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

inline DBloodActor* PLAYER::actor()
{
	return &bloodActors[pSprite->index];
}


inline DBloodActor* getUpperLink(int sect)
{
	auto l = gUpperLink[sect];
	return l == -1 ? nullptr : &bloodActors[l];
}

inline DBloodActor* getLowerLink(int sect)
{
	auto l = gLowerLink[sect];
	return l == -1 ? nullptr : &bloodActors[l];
}

inline FSerializer& Serialize(FSerializer& arc, const char* keyname, DBloodActor*& w, DBloodActor** def)
{
	int index = w? int(w - bloodActors) : -1;
	Serialize(arc, keyname, index, nullptr);
	if (arc.isReading()) w = index == -1? nullptr : &bloodActors[index];
	return arc;
}

inline void sfxPlay3DSound(DBloodActor* pSprite, int soundId, int a3 = -1, int a4 = 0)
{
	sfxPlay3DSound(&pSprite->s(), soundId, a3, a4);
}
inline void sfxPlay3DSoundCP(DBloodActor* pSprite, int soundId, int a3 = -1, int a4 = 0, int pitch = 0, int volume = 0)
{
	sfxPlay3DSoundCP(&pSprite->s(), soundId, a3, a4, pitch, volume);
}
inline void sfxKill3DSound(DBloodActor* pSprite, int a2 = -1, int a3 = -1)
{
	sfxKill3DSound(&pSprite->s(), a2, a3);
}

inline void ChangeActorStat(DBloodActor* actor, int stat)
{
	ChangeSpriteStat(actor->s().index, stat);
}

inline void ChangeActorSect(DBloodActor* actor, int stat)
{
	ChangeSpriteSect(actor->s().index, stat);
}

inline int Collision::actorIndex(DBloodActor* actor)
{
	return int(actor - bloodActors);
}

inline DBloodActor* Collision::Actor(int a)
{
	return &bloodActors[a];
}

inline void setActorPos(DBloodActor* actor, vec3_t* pos)
{
	setsprite(actor->s().index, pos);
}

END_BLD_NS
