#pragma once

#include "coreactor.h"

BEGIN_PS_NS

class DExhumedActor;

enum
{
	kHitAuxMask = 0x30000,
	kHitAux1 = 0x10000,
	kHitAux2 = 0x20000,
};


class DExhumedActor : public DCoreActor
{
	DExhumedActor* base();

public:
	DExhumedActor* pTarget;

	int16_t nPhase;

	int16_t nHealth;
	int16_t nFrame;
	int16_t nAction;
	int16_t nCount;
	int16_t nRun;
	union { int16_t nIndex; int16_t nAngle; };	// angle is for wasp.
	union { int16_t nIndex2; int16_t nAngle2; }; // index2 is for scorpion, angle2 is for wasp.
	union { int16_t nChannel; int16_t nVel; };	// channel is for scorpion, vel is for wasp.
	union { int16_t nDamage; int16_t nAction2; }; // nAction2 is for the queen.

	// for the grenade.
	int nTurn;
	int x;
	int y;


	DExhumedActor() 
	{
		index = (int(this - base()));
	}
	DExhumedActor& operator=(const DExhumedActor& other) = default;

	void Clear()
	{
	}
};

extern DExhumedActor exhumedActors[MAXSPRITES];

inline DExhumedActor* DExhumedActor::base() { return exhumedActors; }

// subclassed to add a game specific actor() method
using HitInfo = THitInfo<DExhumedActor>;
using Collision = TCollision<DExhumedActor>;

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

	ExhumedSectIterator(sectortype* stat) : SectIterator(stat)
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

inline void ChangeActorSect(DExhumedActor* actor, sectortype* stat)
{
	changespritesect(actor->GetSpriteIndex(), sector.IndexOf(stat));
}

inline void setActorPos(DExhumedActor* actor, vec3_t* pos)
{
	setsprite(actor->GetSpriteIndex(), pos);
}

END_BLD_NS
