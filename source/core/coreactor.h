#pragma once

#include <stdint.h>
#include "buildtypes.h"

class DCoreActor
{
	// common part of the game actors
protected:
	int index;

public:

	spritetype& s() 
	{ 
		return sprite[index]; 
	}

	int GetIndex() 
	{ 
		// For error printing only! This is only identical with the sprite index for items spawned at map start.
		return s().time; 
	}	

	int GetSpriteIndex() 
	{ 
		// this is only here to mark places that need changing later! It will be removed once the sprite array goes.
		return index; 
	}	
	


	sectortype* sector()
	{
		return s().sector();
	}


};

// holds pointers to the game-side actors.
inline DCoreActor* actorArray[16384];
extern TArray<sectortype> sector;
extern TArray<walltype> wall;


// Masking these into the object index to keep it in 16 bit was probably the single most dumbest and pointless thing Build ever did.
// Gonna be fun to globally replace these to finally lift the limit this imposes on map size.
// Names taken from DukeGDX
enum EHitBits
{
    kHitNone = 0,
    kHitTypeMask = 0xC000,
    kHitTypeMaskSW = 0x1C000, // SW has one more relevant bit
    kHitIndexMask = 0x3FFF,
    kHitSector = 0x4000,
    kHitWall = 0x8000,
    kHitSprite = 0xC000,
    kHitVoid = 0x10000,      // SW only


};

// This serves as input/output for all functions dealing with collisions, hits, etc.
// Not all utilities use all variables.
template<class TActor>
struct HitInfoBase
{
	static_assert(std::is_convertible_v<TActor*, DCoreActor*>, "Actor class for Collision needs to inherit from DCoreActor");

    int type;
	vec3_t hitpos;
	sectortype* hitSector;
	walltype* hitWall;
    TActor* hitActor;

	HitInfoBase() = default;
    explicit HitInfoBase(int legacyval) { setFromEngine(legacyval); }

	void invalidate()
	{
		*this = {};
		type = -1; // something invalid that's not a valid hit type.
	}

	int setNone()
	{
		*this = {};
		return kHitNone;
	}

	int setSector(int num)
	{
		*this = {};
		type = kHitSector;
		hitSector = &sector[num];
		return kHitSector;
	}

	int setWall(int num)
	{
		*this = {};
		type = kHitWall;
		hitWall = &wall[num];
		return kHitWall;
	}

	int setSprite(int num)
	{
		*this = {};
		type = kHitSprite;
		hitActor = static_cast<TActor*>(actorArray[num]);
		return kHitSprite;
	}

	int setSprite(TActor* num)
	{
		*this = {};
		type = kHitSprite;
		hitActor = num;
		return kHitSprite;
	}

	int setVoid() 
	{ 
		*this = {};
		type = kHitVoid;
		return kHitVoid; 
	}

	// this hack job needs to go. We still need it for the time being.
	int setFromEngine(int value)
	{
		type = value & kHitTypeMaskSW;
		if (type == kHitSector) setSector(value & kHitIndexMask);
		else if (type == kHitWall) setWall(value & kHitIndexMask);
		else if (type == kHitSprite) setSprite(value & kHitIndexMask);
		else setNone();
		return type;
	}
};
