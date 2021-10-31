#pragma once

// included by game.h

BEGIN_SW_NS


class DSWActor
{
	int index;
	DSWActor* base();

public:

	int cumulDamage;

	DSWActor() :index(int(this - base())) { /*assert(index >= 0 && index < kMaxSprites);*/ }
	DSWActor& operator=(const DSWActor& other) = default;

	void Clear()
	{
	}
	bool hasU() { return User[index].Data() != nullptr; }
	/*
	void addU()
	{
		if (s().extra == -1) dbInsertXSprite(s().index);
	}
	*/
	spritetype& s() { return sprite[index]; }
	USER* u() { return User[index].Data(); }

	int GetSpriteIndex() const
	{
		return index;
	}
};

extern DSWActor swActors[MAXSPRITES];

inline DSWActor* DSWActor::base() { return swActors; }

// Iterator wrappers that return an actor pointer, not an index.
class SWStatIterator : public StatIterator
{
public:
	SWStatIterator(int stat) : StatIterator(stat)
	{
	}

	DSWActor* Next()
	{
		int n = NextIndex();
		return n >= 0 ? &swActors[n] : nullptr;
	}

	DSWActor* Peek()
	{
		int n = PeekIndex();
		return n >= 0 ? &swActors[n] : nullptr;
	}
};

class SWSectIterator : public SectIterator
{
public:
	SWSectIterator(int stat) : SectIterator(stat)
	{
	}

	DSWActor* Next()
	{
		int n = NextIndex();
		return n >= 0 ? &swActors[n] : nullptr;
	}

	DSWActor* Peek()
	{
		int n = PeekIndex();
		return n >= 0 ? &swActors[n] : nullptr;
	}
};

// An iterator to iterate over all sprites.
class SWSpriteIterator
{
	SWStatIterator it;
	int stat = 0;

public:
	SWSpriteIterator() : it(0) {}

	DSWActor* Next()
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
class SWLinearSpriteIterator
{
	int index = 0;
public:

	void Reset()
	{
		index = 0;
	}

	DSWActor* Next()
	{
		while (index < MAXSPRITES)
		{
			auto p = &swActors[index++];
			if (p->s().statnum != MAXSTATUS) return p;
		}
		return nullptr;
	}
};


enum EHitBitsSW
{
	kHitTypeMaskSW = 0x1C000,
	kHitSky = 0x10000,      // SW only
};


inline int Collision::setNone()
{
	type = kHitNone;
	index = -1;
	legacyVal = 0;
	actor = nullptr;
	return kHitNone;
}

inline int Collision::setSector(int num)
{
	type = kHitSector;
	index = num;
	legacyVal = type | index;
	actor = nullptr;
	return kHitSector;
}
inline int Collision::setWall(int num)
{
	type = kHitWall;
	index = num;
	legacyVal = type | index;
	actor = nullptr;
	return kHitWall;
}
inline int Collision::setSprite(DSWActor* num)
{
	type = kHitSprite;
	index = -1;
	legacyVal = type | int(num - swActors);
	actor = num;
	return kHitSprite;
}

inline int Collision::setFromEngine(int value)
{
	legacyVal = value;
	type = value & kHitTypeMaskSW;
	if (type == 0) { index = -1; actor = nullptr; }
	else if (type != kHitSprite) { index = value & kHitIndexMask; actor = nullptr; }
	else { index = -1; actor = &swActors[value & kHitIndexMask]; }
	return type;
}


inline FSerializer& Serialize(FSerializer& arc, const char* keyname, DSWActor*& w, DSWActor** def)
{
	int index = w? int(w - swActors) : -1;
	Serialize(arc, keyname, index, nullptr);
	if (arc.isReading()) w = index == -1? nullptr : &swActors[index];
	return arc;
}

inline void ChangeActorSect(DSWActor* actor, int sect)
{
	changespritesect(actor->GetSpriteIndex(), sect);
}

inline int SetActorZ(DSWActor* actor, const vec3_t* newpos)
{
	return setspritez(actor->GetSpriteIndex(), newpos);
}

END_SW_NS
