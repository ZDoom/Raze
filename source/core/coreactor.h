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

	bool exists() const
	{
		return (unsigned)s().statnum < MAXSTATUS;
	}

	spritetype& s() const
	{ 
		return sprite[index]; 
	}

	spriteext_t& sx() const
	{
		return spriteext[index];
	}

	spritesmooth_t& sm() const
	{
		return spritesmooth[index];
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
	vec3_t hitpos;
	sectortype* hitSector;
	walltype* hitWall;
    DCoreActor* hitActor;

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
	T* actor() const { return static_cast<T*>(hitActor); }
};


struct CollisionBase
{
	int type;
	int exbits;	// extended game-side info (only used by Exhumed)
	union
	{
		// can only have one at a time
		sectortype* hitSector;
		walltype* hitWall;
		DCoreActor* hitActor;
	};

	void invalidate()
	{
		type = -1; // something invalid that's not a valid hit type.
		hitSector = nullptr;
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
		hitSector = nullptr;
		type = kHitVoid;
		return kHitVoid;
	}

	// this hack job needs to go. We still need it for the time being.
	int setFromEngine(int value)
	{
		int type = value & kHitTypeMaskSW;
		if (type == kHitSector) setSector(value & kHitIndexMask);
		else if (type == kHitWall) setWall(value & kHitIndexMask);
		else if (type == kHitSprite) setSprite(value & kHitIndexMask);
		else setNone();
		return type;
	}
};

template<class T>
struct TCollision : public CollisionBase
{
	T* actor() const { return static_cast<T*>(hitActor); }
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

	void Reset()
	{
		stat = 0;
		it.Reset(0);
	}
};

using CoreSectIterator = TSectIterator<DCoreActor>;


inline void ChangeActorStat(DCoreActor* actor, int stat)
{
	changespritestat(actor->GetSpriteIndex(), stat);
}

inline void ChangeActorSect(DCoreActor* actor, sectortype* sect)
{
	changespritesect(actor->GetSpriteIndex(), sector.IndexOf(sect));
}


void SetActorZ(DCoreActor* actor, const vec3_t* newpos);
void SetActor(DCoreActor* actor, const vec3_t* newpos);

inline void SetActor(DCoreActor* actor, const vec3_t& newpos)
{
	SetActor(actor, &newpos);
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

inline int clipmove(vec3_t& pos, sectortype** const sect, int xvect, int yvect,
	int const walldist, int const ceildist, int const flordist, unsigned const cliptype, CollisionBase& result, int clipmoveboxtracenum = 3)
{
	int sectno = *sect ? sector.IndexOf(*sect) : -1;
	int res = clipmove_(&pos, &sectno, xvect, yvect, walldist, ceildist, flordist, cliptype, clipmoveboxtracenum);
	*sect = sectno == -1 ? nullptr : &sector[sectno];
	return result.setFromEngine(res);
}

inline void neartag(const vec3_t& pos, sectortype* sect, int angle, HitInfoBase& result, int neartagrange, int tagsearch)
{
	short ntsect, ntwal, ntsprt;
	int ntdist;
	neartag_(pos.x, pos.y, pos.z, sect == nullptr ? -1 : sector.IndexOf(sect), angle, &ntsect, &ntwal, &ntsprt, &ntdist, neartagrange, tagsearch);
	result.hitpos.x = ntdist;
	result.hitSector = ntsect == -1 ? nullptr : &sector[ntsect];
	result.hitWall = ntwal == -1 ? nullptr : &wall[ntwal];
	result.hitActor = ntsprt == -1 ? nullptr : actorArray[ntsprt];
}

inline void getzrange(const vec3_t& pos, sectortype* sect, int32_t* ceilz, CollisionBase& ceilhit, int32_t* florz,
	CollisionBase& florhit, int32_t walldist, uint32_t cliptype)
{
	int fh, ch;
	getzrange_(&pos, sector.IndexOf(sect), ceilz, &ch, florz, &fh, walldist, cliptype);
	ceilhit.setFromEngine(ch);
	florhit.setFromEngine(fh);
}

inline int pushmove(vec3_t* const vect, sectortype** const sect, int32_t const walldist, int32_t const ceildist, int32_t const flordist,
	uint32_t const cliptype, bool clear = true)
{
	int sectno = *sect ? sector.IndexOf(*sect) : -1;
	int res = pushmove_(vect, &sectno, walldist, ceildist, flordist, cliptype, clear);
	*sect = sectno == -1 ? nullptr : &sector[sectno];
	return res;
}
