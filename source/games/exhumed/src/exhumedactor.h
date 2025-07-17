#pragma once

#include "coreactor.h"

BEGIN_PS_NS


enum
{
	kHitAuxMask = 0x30000,
	kHitAux1 = 0x10000,
	kHitAux2 = 0x20000,
};


class DExhumedActor : public DCoreActor
{
	DECLARE_CLASS(DExhumedActor, DCoreActor)
	HAS_OBJECT_POINTERS

public:
	TObjPtr<DExhumedActor*> pTarget;

	int16_t nPhase;
	int16_t nHealth;
	int16_t nCount;
	int16_t nRun;
	union { int16_t nIndex; int16_t nAngle; };	// angle is for wasp.
	union { int16_t nIndex2; int16_t nAngle2; }; // index2 is for scorpion, angle2 is for wasp.
	union { int16_t nChannel; int16_t nVel; };	// channel is for scorpion, vel is for wasp.
	int16_t nDamage;
	DAngle pitch;	// used by AngleChase

	// for the grenade.
	int nTurn;
	DVector2 vec;

	// for sequences.
	FName nSeqFile;
	uint16_t nFrame;
	int16_t nAction;

	// for anims.
	int16_t nSeqIndex;
	int16_t nFlags;


	DExhumedActor() = default;

	void Serialize(FSerializer& arc) override;
	void VelFromAngle(int shift = 0)
	{
		vel.SetXY(spr.Angles.Yaw.ToVector() * (1 << (10 + shift)));
	}


};

// subclassed to add a game specific actor() method
using HitInfo = THitInfo<DExhumedActor>;
using Collision = TCollision<DExhumedActor>;

using ExhumedStatIterator = TStatIterator<DExhumedActor>;
using ExhumedSectIterator = TSectIterator<DExhumedActor>;
using ExhumedSpriteIterator = TSpriteIterator<DExhumedActor>;


END_BLD_NS
