#pragma once

#include <stdint.h>
#include "build.h"
#include "iterators.h"

class DCoreActor
{
	// common part of the game actors
protected:
	int index;

public:

	spritetype& s() const
	{ 
		return sprite[index]; 
	}

	int GetIndex() const
	{ 
		// For error printing only! This is only identical with the sprite index for items spawned at map start.
		return s().time; 
	}	

	int GetSpriteIndex() const 
	{ 
		// this is only here to mark places that need changing later! It will be removed once the sprite array goes.
		return index; 
	}	
	
	sectortype* sector() const
	{
		return s().sector();
	}

	bool insector() const
	{
		return s().insector();
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

inline FSerializer& Serialize(FSerializer& arc, const char* keyname, DCoreActor*& w, DCoreActor** def)
{
	int index = w ? w->GetSpriteIndex() : -1;
	Serialize(arc, keyname, index, nullptr);
	if (arc.isReading()) w = index == -1 ? nullptr : actorArray[index];
	return arc;
}

// This serves as input/output for all functions dealing with collisions, hits, etc.
// Not all utilities use all variables.
struct HitInfoBase
{
    //int type;
	vec3_t hitpos;
	sectortype* hitSector;
	walltype* hitWall;
    DCoreActor* hitActor;

	//HitInfoBase() = default;
    //explicit HitInfoBase(int legacyval) { setFromEngine(legacyval); }

#if 0
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

	int setSector(sectortype* num)
	{
		*this = {};
		type = kHitSector;
		hitSector = num;
		return kHitSector;
	}

	int setWall(int num)
	{
		*this = {};
		type = kHitWall;
		hitWall = &wall[num];
		return kHitWall;
	}

	int setWall(walltype* num)
	{
		*this = {};
		type = kHitWall;
		hitWall = num;
		return kHitWall;
	}

	int setSprite(int num)
	{
		*this = {};
		type = kHitSprite;
		hitActor = actorArray[num];
		return kHitSprite;
	}

	int setSprite(DCoreActor* num)
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
#endif

	void clearObj()
	{
		hitSector = nullptr;
		hitWall = nullptr;
		hitActor = nullptr;
	}
};

template<class T>
struct THitInfo : public HitInfoBase
{
	T* actor() { return static_cast<T*>(hitActor); }
};

// Iterator wrappers that return an actor pointer, not an index.
template<class TActor>
class TStatIterator : public StatIterator
{
public:
	TStatIterator(int stat) : StatIterator(stat)
	{
	}

	TActor* Next()
	{
		int n = NextIndex();
		return n >= 0 ? static_cast<TActor*>(actorArray[n]) : nullptr;
	}

	TActor* Peek()
	{
		int n = PeekIndex();
		return n >= 0 ? static_cast<TActor*>(actorArray[n]) : nullptr;
	}
};

template<class TActor>
class TSectIterator : public SectIterator
{
public:
	TSectIterator(int stat) : SectIterator(stat)
	{
	}

	TSectIterator(sectortype* stat) : SectIterator(stat)
	{
	}

	TActor* Next()
	{
		int n = NextIndex();
		return n >= 0 ? static_cast<TActor*>(actorArray[n]) : nullptr;
	}

	TActor* Peek()
	{
		int n = PeekIndex();
		return n >= 0 ? static_cast<TActor*>(actorArray[n]) : nullptr;
	}
};

// An iterator to iterate over all sprites.
template<class TActor>
class TSpriteIterator
{
	TStatIterator<TActor> it;
	int stat = 0;

public:
	TSpriteIterator() : it(0) {}

	TActor* Next()
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

// For iterating linearly over map spawned sprites. Will later only be valid on map load
template<class TActor>
class TLinearSpriteIterator
{
	int index = 0;
public:

	void Reset()
	{
		index = 0;
	}

	TActor* Next()
	{
		while (index < MAXSPRITES)
		{
			auto p = static_cast<TActor*>(actorArray[index++]);
			if (p->s().statnum != MAXSTATUS) return p;
		}
		return nullptr;
	}
};


[[deprecated]]
inline int hitscan(const vec3_t* sv, int sectnum, int vx, int vy, int vz, hitdata_t* hitinfo, unsigned cliptype)
{
	return hitscan_(sv, sectnum, vx, vy, vz, hitinfo, cliptype);
}

[[deprecated]]
inline int hitscan(int x, int y, int z, int sectnum, int vx, int vy, int vz,
	short* hitsect, short* hitwall, short* hitspr, int* hitx, int* hity, int* hitz, uint32_t cliptype)
{
	vec3_t v{ x,y,z };
	hitdata_t hd{};
	int res = hitscan_(&v, sectnum, vx, vy, vz, &hd, cliptype);
	if (hitsect) *hitsect = hd.sect;
	if (hitwall) *hitwall = hd.wall;
	if (hitspr) *hitspr = hd.sprite;
	*hitx = hd.pos.x;
	*hity = hd.pos.y;
	*hitz = hd.pos.z;
	return res;
}

inline int hitscan(const vec3_t& start, const sectortype* startsect, const vec3_t& direction, HitInfoBase& hitinfo, unsigned cliptype)
{
	hitdata_t hd{};
	hd.pos.z = hitinfo.hitpos.z;	// this can pass through unaltered.
	int res = hitscan_(&start, sector.IndexOf(startsect), direction.x, direction.y, direction.z, &hd, cliptype);
	hitinfo.hitpos = hd.pos;
	hitinfo.hitSector = hd.sect == -1? nullptr : &sector[hd.sect];
	hitinfo.hitWall = hd.wall == -1? nullptr : &wall[hd.wall];
	hitinfo.hitActor = hd.sprite == -1? nullptr : actorArray[hd.sprite];
	return res;
}
