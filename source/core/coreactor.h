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

	DVector3 opos;
	DAngle oang;
	DVector3 vel;
	double oviewzoffset, viewzoffset;
	double clipdist;

	int time;
	int16_t spritesetindex;


	DCoreActor() = default;
	virtual ~DCoreActor() = default;
	DCoreActor(const DCoreActor& other) = delete;				// we also do not want to allow copies.
	DCoreActor& operator=(const DCoreActor& other) = delete;

	virtual void Serialize(FSerializer& arc);

	virtual void BeginPlay() {}
	void OnDestroy() override;
	size_t PropagateMark() override;
	double GetOffsetAndHeight(double& height);
	
	void initFromSprite(spritetype* pspr);

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

	constexpr int int_ang() const
	{
		return spr.angle.Buildang();
	}

	void norm_ang()
	{
		spr.angle = spr.angle.Normalized360();
	}

	void ZeroVelocityXY()
	{
		vel .X = vel .Y = 0;
	}

	void ZeroVelocity()
	{
		vel = { 0,0,0 };
	}

	DVector3 interpolatedpos(double const interpfrac)
	{
		return ::interpolatedvalue(opos, spr.pos, interpfrac);
	}

	DAngle interpolatedangle(double const interpfrac)
	{
		return ::interpolatedvalue(oang, spr.angle, interpfrac);
	}

	void backupz()
	{
		opos.Z = spr.pos.Z;
		oviewzoffset = viewzoffset;
	}

	void backupvec2()
	{
		opos.XY() = spr.pos.XY();
	}

	void backuppos()
	{
		opos = spr.pos;
		oviewzoffset = viewzoffset;
	}

	void backupang()
	{
		oang = spr.angle;
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
	
	int native_clipdist()
	{
		return int(clipdist * 4);
	}
	
	void copy_clipdist(DCoreActor* other)
	{
		clipdist = other->clipdist;
	}

};

// holds pointers to the game-side actors.
extern TArray<sectortype> sector;
extern TArray<walltype> wall;


// Masking these into the object index to keep it in 16 bit was probably the single most dumbest and pointless thing Build ever did.
// Names taken from DukeGDX
enum EHitBits
{
	kHitNone = 0,
	kHitSector = 0x4000,
	kHitWall = 0x8000,
	kHitSprite = 0xC000,
	kHitVoid = 0x10000,      // SW only
};

// This serves as input/output for all functions dealing with collisions, hits, etc.
// Not all utilities use all variables.
struct HitInfoBase
{
	DVector3 hitpos;
	sectortype* hitSector;
	walltype* hitWall;
	DCoreActor* hitActor;

	void clearObj()
	{
		hitSector = nullptr;
		hitWall = nullptr;
		hitActor = nullptr;
	}

	void set(sectortype* sect, walltype* wal, DCoreActor* actor, const DVector3& pos)
	{
		hitSector = sect;
		hitWall = wal;
		hitActor = actor;
		hitpos = pos;
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

	TSectIterator(int stat)
	{
		next = sector[stat].firstEntry;
	}

	TSectIterator(sectortype* stat)
	{
		next = stat->firstEntry;
	}

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


void SetActorZ(DCoreActor* actor, const DVector3& newpos);
void SetActor(DCoreActor* actor, const DVector3& newpos);

inline int clipmove(DVector3& pos, sectortype** const sect, const DVector2& mvec,
	double const walldist, double const ceildist, double const flordist, unsigned const cliptype, CollisionBase& result, int clipmoveboxtracenum = 3)
{
	auto vect = vec3_t(int(pos.X * worldtoint), int(pos.Y * worldtoint), int(pos.Z * zworldtoint));
	int sectno = *sect ? sector.IndexOf(*sect) : -1;
	result = clipmove_(&vect, &sectno, FloatToFixed<18>(mvec.X), FloatToFixed<18>(mvec.Y), int(walldist * worldtoint), int(ceildist * zworldtoint), int(flordist * zworldtoint), cliptype, clipmoveboxtracenum);
	pos = { vect.X * inttoworld, vect.Y * inttoworld, vect.Z * zinttoworld };
	*sect = sectno == -1 ? nullptr : &sector[sectno];
	return result.type;
}

inline int clipmove(DVector2& pos, double z, sectortype** const sect, const DVector2& mvec,
	double const walldist, double const ceildist, double const flordist, unsigned const cliptype, CollisionBase& result, int clipmoveboxtracenum = 3)
{
	auto vect = DVector3(pos, z);
	auto res = clipmove(vect, sect, mvec, walldist, ceildist, flordist, cliptype, result);
	pos = vect.XY();
	return res;
}


inline PClassActor* PClass::FindActor(FName name)
{
	auto cls = FindClass(name);
	return cls && cls->IsDescendantOf(RUNTIME_CLASS(DCoreActor)) ? static_cast<PClassActor*>(cls) : nullptr;
}

inline DCoreActor* GetDefaultByType(const PClass* type)
{
	return (DCoreActor*)(type->Defaults);
}

