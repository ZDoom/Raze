#pragma once

BEGIN_PS_NS

class DExhumedActor;

enum
{
	kHitAuxMask = 0x30000,
	kHitAux1 = 0x10000,
	kHitAux2 = 0x20000,
};

// Wrapper around the insane collision info mess from Build.
struct Collision
{
	int type;
	int index;
	int exbits;
	int legacyVal;	// should be removed later, but needed for converting back for unadjusted code.
	DExhumedActor* actor;

	Collision() = default;
	explicit Collision(int legacyval) { setFromEngine(legacyval); }

	// need forward declarations of these.
	int actorIndex(DExhumedActor*);
	DExhumedActor* Actor(int);

	int setNone()
	{
		type = kHitNone;
		index = -1;
		exbits = 0;
		legacyVal = 0;
		actor = nullptr;
		return kHitNone;
	}

	int setSector(int num)
	{
		type = kHitSector;
		index = num;
		exbits = 0;
		legacyVal = type | index;
		actor = nullptr;
		return kHitSector;
	}
	int setWall(int num)
	{
		type = kHitWall;
		index = num;
		exbits = 0;
		legacyVal = type | index;
		actor = nullptr;
		return kHitWall;
	}
	int setSprite(DExhumedActor* num)
	{
		type = kHitSprite;
		index = -1;
		exbits = 0;
		legacyVal = type | actorIndex(num);
		actor = num;
		return kHitSprite;
	}

	int setFromEngine(int value)
	{
		legacyVal = value;
		type = value & kHitTypeMask;
		exbits = value & kHitAuxMask;
		if (type == 0) { index = -1; actor = nullptr; }
		else if (type != kHitSprite) { index = value & kHitIndexMask; actor = nullptr; }
		else { index = -1; actor = Actor(value & kHitIndexMask); }
		return type;
	}
};

class DExhumedActor
{
	int index;
	DExhumedActor* base();

public:
	DExhumedActor* pTarget;

	short nPhase;

	short nHealth;
	short nFrame;
	short nAction;
	short nCount;
	short nRun;
	union { short nIndex; short nAngle; };	// angle is for wasp.
	union { short nIndex2; short nAngle2; }; // index2 is for scorpion, angle2 is for wasp.
	union { short nChannel; short nVel; };	// channel is for scorpion, vel is for wasp.
	union { short nDamage; short nAction2; }; // nAction2 is for the queen.

	// for the grenade.
	int nTurn;
	int x;
	int y;


	DExhumedActor() :index(int(this - base())) {}
	DExhumedActor& operator=(const DExhumedActor& other) = default;

	void Clear()
	{
	}

	spritetype& s() { return sprite[index]; }
	int GetIndex() { return index; }	// should only be for error reporting or for masking to a slot index
	int GetSpriteIndex() { return index; }	// this is only here to mark places that need changing later!
};

extern DExhumedActor exhumedActors[MAXSPRITES];

inline DExhumedActor* Collision::Actor(int i)
{
	return &exhumedActors[i];
}

inline int Collision::actorIndex(DExhumedActor* a)
{
	return a->GetSpriteIndex();
}


inline DExhumedActor* DExhumedActor::base() { return exhumedActors; }

// Iterator wrappers that return an actor pointer, not an index.
class ExhumedStatIterator : public StatIterator
{
public:
	ExhumedStatIterator(int stat) : StatIterator(stat)
	{
	}

	DExhumedActor* Next()
	{
		int n = NextIndex();
		return n >= 0 ? &exhumedActors[n] : nullptr;
	}

	DExhumedActor* Peek()
	{
		int n = PeekIndex();
		return n >= 0 ? &exhumedActors[n] : nullptr;
	}
};

class ExhumedSectIterator : public SectIterator
{
public:
	ExhumedSectIterator(int stat) : SectIterator(stat)
	{
	}

	DExhumedActor* Next()
	{
		int n = NextIndex();
		return n >= 0 ? &exhumedActors[n] : nullptr;
	}

	DExhumedActor* Peek()
	{
		int n = PeekIndex();
		return n >= 0 ? &exhumedActors[n] : nullptr;
	}
};

// An iterator to iterate over all sprites.
class ExhumedSpriteIterator
{
	ExhumedStatIterator it;
	int stat = 0;

public:
	ExhumedSpriteIterator() : it(0) {}

	DExhumedActor* Next()
	{
		while (stat < MAXSTATUS)
		{
			auto ac = it.Next();
			if (ac) return ac;
			stat++;
			if (stat < MAXSTATUS) it.Reset(stat);
		}
		return nullptr;
	}
};

// For iterating linearly over map spawned sprites.
class ExhumedLinearSpriteIterator
{
	int index = 0;
public:

	void Reset()
	{
		index = 0;
	}

	DExhumedActor* Next()
	{
		while (index < MAXSPRITES)
		{
			auto p = &exhumedActors[index++];
			if (p->s().statnum != MAXSTATUS) return p;
		}
		return nullptr;
	}
};



inline FSerializer& Serialize(FSerializer& arc, const char* keyname, DExhumedActor*& w, DExhumedActor** def)
{
	int index = w? int(w - exhumedActors) : -1;
	Serialize(arc, keyname, index, nullptr);
	if (arc.isReading()) w = index == -1? nullptr : &exhumedActors[index];
	return arc;
}

inline void ChangeActorStat(DExhumedActor* actor, int stat)
{
	changespritestat(actor->GetSpriteIndex(), stat);
}

inline void ChangeActorSect(DExhumedActor* actor, int stat)
{
	changespritesect(actor->GetSpriteIndex(), stat);
}

inline void setActorPos(DExhumedActor* actor, vec3_t* pos)
{
	setsprite(actor->GetSpriteIndex(), pos);
}

inline DExhumedActor* GetActor(const hitdata_t& hitData)
{
	return hitData.sprite < 0? nullptr : &exhumedActors[hitData.sprite];
}

END_BLD_NS
