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

using ExhumedStatIterator = TStatIterator<DExhumedActor>;
using ExhumedSectIterator = TSectIterator<DExhumedActor>;
using ExhumedSpriteIterator = TSpriteIterator<DExhumedActor>;


inline FSerializer& Serialize(FSerializer& arc, const char* keyname, DExhumedActor*& w, DExhumedActor** def)
{
	int index = w? int(w - exhumedActors) : -1;
	Serialize(arc, keyname, index, nullptr);
	if (arc.isReading()) w = index == -1? nullptr : &exhumedActors[index];
	return arc;
}

END_BLD_NS
