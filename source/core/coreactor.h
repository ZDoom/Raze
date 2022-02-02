#pragma once

#include <stdint.h>
#include "maptypes.h"
#include "build.h"
#include "actorinfo.h"

enum
{
	MAXSTATUS = 1024
};

struct FWallSpriteDesc
{
	walltype* wall;
	float offsetOnWall;
};

// This is for quick determination of walls a wall sprite is attached to.
struct FWallSpriteInfo
{
	tspritetype base;						// this is needed for checking if something has changed.
	TArray<FWallSpriteDesc> connections[2]; // two lists - for front and back
};

class DCoreActor : public DObject
{
	DECLARE_CLASS(DCoreActor, DObject)
	HAS_OBJECT_POINTERS
	// common part of the game actors
public:

	// These two are needed because we cannot rely on the ones in the sprites for unlinking.
	int link_stat;
	sectortype* link_sector;
	DCoreActor* prevStat, * nextStat;
	DCoreActor* prevSect, * nextSect;
	FWallSpriteInfo* wallspriteinfo; // this is render data but needs to be attached to the actor so it can be found.

	spritetype spr;
	spriteext_t sprext;
	spritesmooth_t spsmooth;

	vec3_t opos;
	int time;
	int16_t oang;
	int16_t spritesetindex;


	DCoreActor() = default;
	virtual ~DCoreActor() = default;
	DCoreActor(const DCoreActor& other) = delete;				// we also do not want to allow copies.
	DCoreActor& operator=(const DCoreActor& other) = delete;

	virtual void Serialize(FSerializer& arc);

	virtual void BeginPlay() {}
	void OnDestroy() override;
	size_t PropagateMark() override;
	int GetOffsetAndHeight(int& height);

	bool exists() const
	{
		return (unsigned)spr.statnum < MAXSTATUS;
	}

	int GetIndex() const
	{ 
		// This is only identical with the sprite index for items spawned at map start.
		return time; 
	}
	
	const vec3_t int_pos() const
	{
		return { int(spr.pos.X * worldtoint), int(spr.pos.Y * worldtoint), int(spr.pos.Z * zworldtoint) };
	}

	void set_int_z(int z)
	{
		spr.pos.Z = z * zinttoworld;
	}

	void add_int_z(int z)
	{
		spr.pos.Z += z * zinttoworld;
	}

	void add_int_pos(const vec3_t& add)
	{
		spr.pos += { add.X* inttoworld, add.Y* inttoworld, add.Z* zinttoworld };
	}

	void set_int_pos(const vec3_t& add)
	{
		spr.pos = { add.X* inttoworld, add.Y* inttoworld, add.Z* zinttoworld };
	}

	void copyXY(DCoreActor* other)
	{
		spr.pos.X = other->spr.pos.X;
		spr.pos.Y = other->spr.pos.Y;
	}

	void set_int_xy(int x, int y)
	{
		spr.pos.X = x * inttoworld;
		spr.pos.Y = y * inttoworld;
	}

	DVector3 float_pos() const
	{
		return spr.pos;
	}
	
	void set_float_z(int z)
	{
		spr.pos.Z = z;
	}

	void add_float_z(int z)
	{
		spr.pos.Z += z;
	}


	// Same as above but with invertex y and z axes to match the renderer's coordinate system.
	DVector3 render_pos() const
	{
		return { spr.pos.X, -spr.pos.Y, -spr.pos.Z };
	}

	int32_t interpolatedx(double const smoothratio, int const scale = 16)
	{
		return interpolatedvalue(opos.X, spr.int_pos().X, smoothratio, scale);
	}

	int32_t interpolatedy(double const smoothratio, int const scale = 16)
	{
		return interpolatedvalue(opos.Y, spr.int_pos().Y, smoothratio, scale);
	}

	int32_t interpolatedz(double const smoothratio, int const scale = 16)
	{
		return interpolatedvalue(opos.Z, spr.int_pos().Z, smoothratio, scale);
	}

	vec2_t interpolatedvec2(double const smoothratio, int const scale = 16)
	{
		return
		{
			interpolatedx(smoothratio, scale),
			interpolatedy(smoothratio, scale)
		};
	}

	vec3_t interpolatedvec3(double const smoothratio, int const scale = 16)
	{
		return
		{
			interpolatedx(smoothratio, scale),
			interpolatedy(smoothratio, scale),
			interpolatedz(smoothratio, scale)
		};
	}

	int16_t interpolatedang(double const smoothratio)
	{
		return interpolatedangle(oang, spr.ang, smoothratio, 16);
	}

	void backupx()
	{
		opos.X = spr.int_pos().X;
	}

	void backupy()
	{
		opos.Y = spr.int_pos().Y;
	}

	void backupz()
	{
		opos.Z = spr.int_pos().Z;
	}

	void backupvec2()
	{
		opos.vec2 = spr.int_pos().vec2;
	}

	void backuppos()
	{
		opos = spr.int_pos();
	}

	void backupang()
	{
		oang = spr.ang;
	}

	void backuploc()
	{
		backuppos();
		backupang();
	}

	sectortype* sector() const
	{
		return spr.sectp;
	}

	bool insector() const
	{
		return spr.sectp != nullptr;
	}

	void setsector(sectortype* sect)
	{
		// place for asserts.
		spr.sectp = sect;
	}

	int sectno() const
	{
		return spr.sectp ? ::sector.IndexOf(spr.sectp) : -1;
	}

	auto spriteset() const
	{
		return static_cast<PClassActor*>(GetClass())->ActorInfo()->SpriteSet;
	}

};

// holds pointers to the game-side actors.
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
};

template<class T>
struct TCollision : public CollisionBase
{
	T* actor() const
	{ 
		return static_cast<T*>(hitActor); 
	}

	// normally collision data is short lived, this is only needed in some very rare circumstances.
	T* safeActor()
	{
		return static_cast<T*>(GC::ReadBarrier(hitActor));
	}

	auto operator=(const CollisionBase& other)
	{
		*(CollisionBase*)this = other;
		return *this;
	}
};


struct ActorStatList
{
	DCoreActor* firstEntry, * lastEntry;
};

extern ActorStatList statList[MAXSTATUS];

template<class TActor>
class TStatIterator
{
	DCoreActor* next;
public:
	TStatIterator(int stat)
	{
		next = statList[stat].firstEntry;
	}

	void Reset(int stat)
	{
		next = statList[stat].firstEntry;
	}

	TActor* Next()
	{
		auto n = next;
		if (next) next = next->nextStat;
		return static_cast<TActor*>(n);
	}

	TActor* Peek()
	{
		return static_cast<TActor*>(next);
	}
};

template<class TActor>
class TSectIterator
{
	DCoreActor* next;
public:
	//[[deprecated]]
	TSectIterator(int stat)
	{
		next = sector[stat].firstEntry;
	}

	TSectIterator(sectortype* stat)
	{
		next = stat->firstEntry;
	}

	//[[deprecated]]
	void Reset(int stat)
	{
		next = sector[stat].firstEntry;
	}

	void Reset(sectortype* stat)
	{
		next = stat->firstEntry;
	}


	TActor* Next()
	{
		auto n = next;
		if (next) next = next->nextSect;
		return static_cast<TActor*>(n);
	}

	TActor* Peek()
	{
		return static_cast<TActor*>(next);
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

DCoreActor* InsertActor(PClass* type, sectortype* sector, int stat, bool forcetail = false);
void ChangeActorSect(DCoreActor* actor, sectortype* sector, bool forcetail = false);
int ChangeActorStat(DCoreActor* actor, int nStatus, bool forcetail = false);
void InitSpriteLists();


void SetActorZ(DCoreActor* actor, const vec3_t* newpos);
void SetActor(DCoreActor* actor, const vec3_t* newpos);

inline void SetActor(DCoreActor* actor, const vec3_t& newpos)
{
	SetActor(actor, &newpos);
}

inline void SetActorZ(DCoreActor* actor, const vec3_t& newpos)
{
	SetActorZ(actor, &newpos);
}


inline int clipmove(vec3_t& pos, sectortype** const sect, int xvect, int yvect,
	int const walldist, int const ceildist, int const flordist, unsigned const cliptype, CollisionBase& result, int clipmoveboxtracenum = 3)
{
	int sectno = *sect ? sector.IndexOf(*sect) : -1;
	result = clipmove_(&pos, &sectno, xvect, yvect, walldist, ceildist, flordist, cliptype, clipmoveboxtracenum);
	*sect = sectno == -1 ? nullptr : &sector[sectno];
	return result.type;
}

inline int pushmove(vec3_t* const vect, sectortype** const sect, int32_t const walldist, int32_t const ceildist, int32_t const flordist,
	uint32_t const cliptype, bool clear = true)
{
	int sectno = *sect ? sector.IndexOf(*sect) : -1;
	int res = pushmove_(vect, &sectno, walldist, ceildist, flordist, cliptype, clear);
	*sect = sectno == -1 ? nullptr : &sector[sectno];
	return res;
}

inline int pushmove(DCoreActor* actor, sectortype** const sect, int32_t const walldist, int32_t const ceildist, int32_t const flordist,
	uint32_t const cliptype, bool clear = true)
{
	auto vect = actor->int_pos();
	int sectno = *sect ? sector.IndexOf(*sect) : -1;
	int res = pushmove_(&vect, &sectno, walldist, ceildist, flordist, cliptype, clear);
	actor->set_int_pos(vect);
	*sect = sectno == -1 ? nullptr : &sector[sectno];
	return res;
}

tspritetype* renderAddTsprite(tspriteArray& tsprites, DCoreActor* actor);

inline PClassActor* PClass::FindActor(FName name)
{
	auto cls = FindClass(name);
	return cls && cls->IsDescendantOf(RUNTIME_CLASS(DCoreActor)) ? static_cast<PClassActor*>(cls) : nullptr;
}

inline DCoreActor* GetDefaultByType(const PClass* type)
{
	return (DCoreActor*)(type->Defaults);
}

