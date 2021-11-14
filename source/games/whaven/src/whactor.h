#pragma once

// included by game.h

BEGIN_WH_NS


class DWHActor
{
	int index;
	DWHActor* base();

public:

	DWHActor() :index(int(this - base())) { /*assert(index >= 0 && index < kMaxSprites);*/ }
	DWHActor& operator=(const DWHActor& other) = default;

	void Clear()
	{
	}
	spritetype& s() { return sprite[index]; }

	void SetOwner(DWHActor* own)
	{
		s().owner = own ? own->s().index : -1;
	}

	DWHActor* GetOwner()
	{
		if (s().owner >= 4096) return nullptr; // player index hackery
		if (s().owner == -1 || s().owner == MAXSPRITES - 1) return nullptr;
		return base() + s().owner;
	}

	void SetPlayerOwner(int num)
	{
		s().owner = num + 4096; // caution! This needs to be changed later!!!
	}

	int GetPlayerOwner()
	{
		if (s().owner >= 4096) return s().owner - 4096;
		return -1;
	}

	// we need this because the owner later needs to be split up - but some stuff copies the value without checking.
	void CopyOwner(DWHActor* other)
	{
		s().owner = other->s().owner;
	}

	int GetSpriteIndex() const
	{
		return index;
	}
};

extern DWHActor whActors[MAXSPRITES];

inline DWHActor* DWHActor::base() { return whActors; }

// Iterator wrappers that return an actor pointer, not an index.
class WHStatIterator : public StatIterator
{
public:
	WHStatIterator(int stat) : StatIterator(stat)
	{
	}

	DWHActor* Next()
	{
		int n = NextIndex();
		return n >= 0 ? &whActors[n] : nullptr;
	}

	DWHActor* Peek()
	{
		int n = PeekIndex();
		return n >= 0 ? &whActors[n] : nullptr;
	}
};

class WHSectIterator : public SectIterator
{
public:
	WHSectIterator(int stat) : SectIterator(stat)
	{
	}

	DWHActor* Next()
	{
		int n = NextIndex();
		return n >= 0 ? &whActors[n] : nullptr;
	}

	DWHActor* Peek()
	{
		int n = PeekIndex();
		return n >= 0 ? &whActors[n] : nullptr;
	}
};

// An iterator to iterate over all sprites.
class WHSpriteIterator
{
	WHStatIterator it;
	int stat = 0;

public:
	WHSpriteIterator() : it(0) {}

	DWHActor* Next()
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
class WHLinearSpriteIterator
{
	int index = 0;
public:

	void Reset()
	{
		index = 0;
	}

	DWHActor* Next()
	{
		while (index < MAXSPRITES)
		{
			auto p = &whActors[index++];
			if (p->s().statnum != MAXSTATUS) return p;
		}
		return nullptr;
	}
};


// Wrapper around the insane collision info mess from Build.
struct Collision
{
	int type;
	int index;
	int legacyVal;	// should be removed later, but needed for converting back for unadjusted code.
	DWHActor* actor;

	Collision() = default;
	Collision(int legacyval) { setFromEngine(legacyval); }

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
	int setSprite(DWHActor* num)
	{
		type = kHitSprite;
		index = -1;
		legacyVal = type | int(num - whActors);
		actor = num;
		return kHitSprite;
	}

	int setFromEngine(int value)
	{
		legacyVal = value;
		type = value & kHitTypeMask;
		if (type == 0) { index = -1; actor = nullptr; }
		else if (type != kHitSprite) { index = value & kHitIndexMask; actor = nullptr; }
		else { index = -1; actor = &whActors[value & kHitIndexMask]; }
		return type;
	}
};


inline FSerializer& Serialize(FSerializer& arc, const char* keyname, DWHActor*& w, DWHActor** def)
{
	int index = w? int(w - whActors) : -1;
	Serialize(arc, keyname, index, nullptr);
	if (arc.isReading()) w = index == -1? nullptr : &whActors[index];
	return arc;
}

inline void ChangeActorStat(DWHActor* actor, int newstat)
{
	changespritestat(actor->GetSpriteIndex(), newstat);
}

inline void SetActorPos(DWHActor* actor, int x, int y, int z)
{
	vec3_t v = { x, y, z };
	setsprite(actor->GetSpriteIndex(), &v);
}

inline void SetActorPos(DWHActor* actor, vec3_t* pos)
{
	setsprite(actor->GetSpriteIndex(), pos);
}

inline void DeleteActor(DWHActor* actor)
{
	deletesprite(actor->GetSpriteIndex());
}

inline DWHActor* InsertActor(int sectnum, int statnum)
{
	return &whActors[insertsprite(sectnum, statnum)];
}

END_WH_NS
