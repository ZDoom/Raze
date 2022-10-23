#pragma once

#include "gamecontrol.h"
#include "gamestruct.h"
#include "build.h"
#include "coreactor.h"
#include "intrect.h"
#include "geometry.h"
#include "c_cvars.h"

extern IntRect viewport3d;

EXTERN_CVAR(Bool, hw_hightile)
EXTERN_CVAR(Bool, hw_models)
EXTERN_CVAR(Float, gl_texture_filter_anisotropic)
EXTERN_CVAR(Int, gl_texture_filter)
extern bool hw_int_useindexedcolortextures;
EXTERN_CVAR(Bool, hw_useindexedcolortextures)
EXTERN_CVAR(Bool, r_voxels)

inline int leveltimer;
inline int Numsprites;
inline int display_mirror;
inline int randomseed;
inline int g_visibility = 512, g_relvisibility = 0;

constexpr int SLOPEVAL_FACTOR = 4096;

enum
{
	CLIPMASK0 = (1 << 16) + 1,
	CLIPMASK1 = (256 << 16) + 64
};

//==========================================================================
//
// breadth first search, this gets used multiple times throughout the engine, mainly for iterating over sectors.
// Only works on indices, this has no knowledge of the actual objects being looked at.
// All objects of this type operate on the same shared store. Interleaved use is not allowed, nested use is fine.
//
//==========================================================================

class BFSSearch
{
	static inline TArray<unsigned> store;

	unsigned bitpos;
	unsigned startpos;
	unsigned curpos;

public:
	enum { EOL = ~0u };
	BFSSearch(unsigned datasize, unsigned startnode)
	{
		bitpos = store.Size();
		unsigned bitsize = (datasize + 31) >> 5;
		store.Reserve(bitsize);
		memset(&store[bitpos], 0, bitsize*4);

		startpos = store.Size();
		curpos = startpos;
		Set(startnode);
		store.Push(startnode);
	}

	// This allows this object to just work as a bit array
	// which is useful for using its shared storage.
	BFSSearch(unsigned datasize)
	{
		bitpos = store.Size();
		unsigned bitsize = (datasize + 31) >> 5;
		store.Reserve(bitsize);
		memset(&store[bitpos], 0, bitsize * 4);
	}

	~BFSSearch()
	{
		store.Clamp(bitpos);
	}

	bool Check(unsigned index) const
	{
		return !!(store[bitpos + (index >> 5)] & (1 << (index & 31)));
	}

	void Set(unsigned index)
	{
		store[bitpos + (index >> 5)] |= (1 << (index & 31));
	}


private:
public:
	unsigned GetNext()
	{
		curpos++;
		if (curpos <= store.Size())
			return store[curpos-1];
		else
			return ~0;
	}

	void Rewind()
	{
		curpos = startpos;
	}

	void Add(unsigned elem)
	{
		if (!Check(elem))
		{
			Set(elem);
			store.Push(elem);
		}
	}
};

class BFSSectorSearch : public BFSSearch
{
public:

	BFSSectorSearch(const sectortype* startnode) : BFSSearch(sector.Size(), sector.IndexOf(startnode))
	{
	}

	bool Check(const sectortype* index) const
	{
		return BFSSearch::Check(sector.IndexOf(index));
	}

	void Set(const sectortype* index)
	{
		BFSSearch::Set(sector.IndexOf(index));
	}

	sectortype* GetNext()
	{
		unsigned ret = BFSSearch::GetNext();
		return ret == EOL? nullptr : &sector[ret];
	}

	void Add(sectortype* elem)
	{
		BFSSearch::Add(sector.IndexOf(elem));
	}
};

//==========================================================================
//
// scans all vertices equivalent with a given spot and performs some work on them.
//
//==========================================================================

template<class func>
void vertexscan(walltype* startwall, func mark)
{
	BFSSearch walbitmap(wall.Size());

	// first pass: scan the the next-in-loop of the partner
	auto wal = startwall;
	do
	{
		mark(wal);
		walbitmap.Set(wall.IndexOf(wal));
		if (wal->nextwall < 0) break;
		wal = wal->nextWall()->point2Wall();
	} while (!walbitmap.Check(wall.IndexOf(wal)));

	// second pass: scan the partner of the previous-in-loop.
	wal = startwall;
	while (true)
	{
		auto thelastwall = wal->lastWall();
		// thelastwall can be null here if the map is bogus. 
		if (!thelastwall || thelastwall->nextwall < 0) break;
		wal = thelastwall->nextWall();
		if (walbitmap.Check(wall.IndexOf(wal))) break;
		mark(wal);
		walbitmap.Set(wall.IndexOf(wal));
	}
}


//==========================================================================
//
// 
//
//==========================================================================

inline void dragpoint(walltype* startwall, const DVector2& pos)
{
	vertexscan(startwall, [&](walltype* wal)
		{
			wal->move(pos);
			wal->sectorp()->exflags |= SECTOREX_DRAGGED;
		});
}


//---------------------------------------------------------------------------
//
// Constants used for Build sine/cosine functions.
//
//---------------------------------------------------------------------------

enum
{
	BAMBITS = 21,
	BAMUNIT = 1 << BAMBITS,
};

constexpr double BAngRadian = pi::pi() * (1. / 1024.);
constexpr double BAngToDegree = 360. / 2048.;
constexpr DAngle DAngleBuildToDeg = DAngle::fromDeg(BAngToDegree);

//---------------------------------------------------------------------------
//
// Build sine inline functions.
//
//---------------------------------------------------------------------------

inline int bsin(const int ang)
{
	return int(g_sinbam(ang * BAMUNIT) * 16384);
}

//---------------------------------------------------------------------------
//
// Build cosine inline functions.
//
//---------------------------------------------------------------------------

inline int bcos(const int ang)
{
	return int(g_cosbam(ang * BAMUNIT) * 16384);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

extern double cameradist, cameraclock;

void loaddefinitionsfile(const char* fn, bool cumulative = false, bool maingrp = false);

bool calcChaseCamPos(DVector3& ppos, DCoreActor* pspr, sectortype** psectnum, DAngle ang, DAngle horiz, double const interpfrac, double const backamp);
int getslopeval(sectortype* sect, const DVector3& pos, double bazez);
bool cansee(const DVector3& start, sectortype* sect1, const DVector3& end, sectortype* sect2);
double intersectSprite(DCoreActor* actor, const DVector3& start, const DVector3& direction, DVector3& result, double maxfactor);
double intersectWallSprite(DCoreActor* actor, const DVector3& start, const DVector3& direction, DVector3& result, double maxfactor, bool checktex = false);
double intersectFloorSprite(DCoreActor* actor, const DVector3& start, const DVector3& direction, DVector3& result, double maxfactor);
double intersectSlopeSprite(DCoreActor* actor, const DVector3& start, const DVector3& direction, DVector3& result, double maxfactor);
double checkWallHit(walltype* wal, EWallFlags flagmask, const DVector3& start, const DVector3& direction, DVector3& result, double maxfactor);
double checkSectorPlaneHit(sectortype* sec, const DVector3& start, const DVector3& direction, DVector3& result, double maxfactor);
void neartag(const DVector3& start, sectortype* sect, DAngle angle, HitInfoBase& result, double neartagrange, int tagsearch);
int testpointinquad(const DVector2& pt, const DVector2* quad);
int hitscan(const DVector3& start, const sectortype* startsect, const DVector3& vect, HitInfoBase& hitinfo, unsigned cliptype, double maxrange = -1);

bool checkRangeOfWall(walltype* wal, EWallFlags flagmask, const DVector3& pos, double maxdist, double* theZs);
bool checkRangeOfFaceSprite(DCoreActor* itActor, const DVector3& pos, double maxdist, double* theZs);
bool checkRangeOfWallSprite(DCoreActor* itActor, const DVector3& pos, double maxdist, double* theZs);
bool checkRangeOfFloorSprite(DCoreActor* itActor, const DVector3& pos, double maxdist, double& theZ);
void getzrange(const DVector3& pos, sectortype* sect, double* ceilz, CollisionBase& ceilhit, double* florz, CollisionBase& florhit, double walldist, uint32_t cliptype);

bool checkOpening(const DVector2& inpos, double z, const sectortype* sec, const sectortype* nextsec, double ceilingdist, double floordist, bool precise = false);
int pushmove(DVector3& pos, sectortype** pSect, double walldist, double ceildist, double flordist, unsigned cliptype);

struct ClipRect
{
	DVector2 min;
	DVector2 max;
};

struct MoveClipper
{
	DVector3 pos;
	DVector2 pest;
	DVector2 moveDelta;
	ClipRect rect;
	EWallFlags wallflags;
	double ceilingdist;
	double floordist;
	double walldist;
};

void addClipLine(MoveClipper& clip, const DVector2& start, const DVector2& end, const CollisionBase& daoval, int nofix = false);
void addClipSect(MoveClipper& clip, int sec);

void addWallsToClipList(MoveClipper& clip, sectortype* sec);

int FindBestSector(const DVector3& pos);


tspritetype* renderAddTsprite(tspriteArray& tsprites, DCoreActor* actor);

void setWallSectors();
void GetWallSpritePosition(const spritetypebase* spr, const DVector2& pos, DVector2* out, bool render = false);
void GetFlatSpritePosition(DCoreActor* spr, const DVector2& pos, DVector2* out, double* outz = nullptr, bool render = false);
void GetFlatSpritePosition(const tspritetype* spr, const DVector2& pos, DVector2* out, double* outz, bool render = false);

enum class EClose
{
	Outside,
	InFront,
	Behind
};
EClose IsCloseToLine(const DVector2& vect, const DVector2& start, const DVector2& end, double walldist);
EClose IsCloseToWall(const DVector2& vect, walltype* wal, double walldist);

void checkRotatedWalls();
bool sectorsConnected(int sect1, int sect2);
int32_t inside(double x, double y, const sectortype* sect);
int insidePoly(double x, double y, const DVector2* points, int count);

enum {
	NT_Lotag = 1,
	NT_Hitag = 2,
	NT_NoSpriteCheck = 4
};


//==========================================================================
//
// slope getter stuff (many wrappers, one worker only)
//
//==========================================================================

void calcSlope(const sectortype* sec, double xpos, double ypos, double* pceilz, double* pflorz);

//==========================================================================
//
// for the renderer
//
//==========================================================================

inline void PlanesAtPoint(const sectortype* sec, float dax, float day, float* pceilz, float* pflorz)
{
	double f, c;
	calcSlope(sec, dax, day, &c, &f);
	if (pceilz) *pceilz = -float(c);
	if (pflorz) *pflorz = -float(f);
}


//==========================================================================
//
// for the game engine
//
//==========================================================================

template<class Vector>
inline void calcSlope(const sectortype* sec, const Vector& pos, double* ceilz, double* florz)
{
	calcSlope(sec, pos.X, pos.Y, ceilz, florz);
}

inline double getceilzofslopeptr(const sectortype* sec, double dax, double day)
{
	double c;
	calcSlope(sec, dax, day, &c, nullptr);
	return c;
}
inline double getflorzofslopeptr(const sectortype* sec, double dax, double day)
{
	double f;
	calcSlope(sec, dax, day, nullptr, &f);
	return f;
}
template<class Vector>
inline double getceilzofslopeptr(const sectortype* sec, const Vector& pos)
{
	return getceilzofslopeptr(sec, pos.X, pos.Y);
}
template<class Vector>
inline double getflorzofslopeptr(const sectortype* sec, const Vector& pos)
{
	return getflorzofslopeptr(sec, pos.X, pos.Y);
}

//==========================================================================
//
// slope setters
//
//==========================================================================

inline void alignceilslope(sectortype* sect, const DVector3& pos)
{
	sect->setceilingslope(getslopeval(sect, pos, sect->ceilingz));
}

inline void alignflorslope(sectortype* sect, const DVector3& pos)
{
	sect->setfloorslope(getslopeval(sect, pos, sect->floorz));
}

//==========================================================================
//
// slope sprites
//
//==========================================================================

inline void spriteSetSlope(DCoreActor* actor, int heinum)
{
	if (actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)
	{
		actor->spr.xoffset = heinum & 255;
		actor->spr.yoffset = (heinum >> 8) & 255;
		actor->spr.cstat = (actor->spr.cstat & ~CSTAT_SPRITE_ALIGNMENT_MASK) | (heinum != 0 ? CSTAT_SPRITE_ALIGNMENT_SLOPE : CSTAT_SPRITE_ALIGNMENT_FLOOR);
	}
}

inline int spriteGetSlope(DCoreActor* actor)
{
	return ((actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_SLOPE) ? 0 : uint8_t(actor->spr.xoffset) + (int8_t(actor->spr.yoffset) << 8);
}

// same stuff, different flag...
inline int tspriteGetSlope(const tspritetype* spr)
{
	return !(spr->clipdist & TSPR_SLOPESPRITE) ? 0 : uint8_t(spr->xoffset) + (int8_t(spr->yoffset) << 8);
}

inline double spriteGetZOfSlopef(const spritetypebase* tspr, const DVector2& pos, int heinum)
{
	if (heinum == 0) return tspr->pos.Z;
	return tspr->pos.Z + heinum * -tspr->angle.ToVector().dot(pos - tspr->pos.XY()) * (1. / SLOPEVAL_FACTOR);
}

//==========================================================================
//
// end of slopes
//
//==========================================================================


enum EFindNextSector
{
	Find_Floor = 0,
	Find_Ceiling = 1,
	
	Find_Down = 0,
	Find_Up = 2,
	
	Find_Safe = 4,
	
	Find_CeilingUp = Find_Ceiling | Find_Up,
	Find_CeilingDown = Find_Ceiling | Find_Down,
	Find_FloorUp = Find_Floor | Find_Up,
	Find_FloorDown = Find_Floor | Find_Down,
};
sectortype* nextsectorneighborzptr(sectortype* sectp, double startz, int flags);
bool isAwayFromWall(DCoreActor* ac, double delta);


// important note: This returns positive for 'in front' with renderer coordinates.
// Due to Build's inverted coordinate system it will return negative for 'in front' there.
inline double PointOnLineSide(const DVector2 &pos, const walltype *line)
{
	return (pos.X - line->pos.X) * line->delta().Y - (pos.Y - line->pos.Y) * line->delta().X;
}


extern int numshades;

// Return type is int because this gets passed to variadic functions where structs may produce undefined behavior.
inline int shadeToLight(int shade)
{
	shade = clamp(shade, 0, numshades - 1);
	int light = Scale(numshades - 1 - shade, 255, numshades - 1);
	return PalEntry(255, light, light, light);
}

inline void copyfloorpal(tspritetype* spr, const sectortype* sect)
{
	if (!lookups.noFloorPal(sect->floorpal)) spr->pal = sect->floorpal;
}

inline int I_GetBuildTime()
{
	return I_GetTime(120);
}

inline TArrayView<walltype> wallsofsector(const sectortype* sec)
{
	return TArrayView<walltype>(sec->firstWall(), sec->wallnum);
}

inline TArrayView<walltype> wallsofsector(int sec)
{
	return wallsofsector(&sector[sec]);
}

// these are mainly meant as refactoring aids to mark function calls to work on.
inline int wallnum(const walltype* wal)
{
	return wall.IndexOf(wal);
}

inline int sectnum(const sectortype* sect)
{
	return sector.IndexOf(sect);
}

inline DVector2 NearestPointOnWall(double px, double py, const walltype* wal, bool clamp = true)
{
	return NearestPointOnLine(px, py, wal->pos.X, wal->pos.Y, wal->point2Wall()->pos.X, wal->point2Wall()->pos.Y, clamp);
}

inline double SquareDistToWall(double px, double py, const walltype* wal, DVector2* point = nullptr) 
{
	auto pt = NearestPointOnWall(px, py, wal);
	if (point) *point = pt;
	return SquareDist(px, py, pt.X, pt.Y);
}

double SquareDistToSector(double px, double py, const sectortype* sect, DVector2* point = nullptr);

inline double BobVal(int val)
{
	return g_sinbam((unsigned)val << 21);
}

inline double BobVal(double val)
{
	return g_sinbam(xs_CRoundToUInt(val * (1 << 21)));
}

inline DAngle GetMinPitch()
{
	return !cl_clampedpitch ? (DAngle90 - minAngle) : gi->playerPitchMin();
}

inline DAngle GetMaxPitch()
{
	return !cl_clampedpitch ? (minAngle - DAngle90) : gi->playerPitchMax();
}

inline DAngle ClampViewPitch(const DAngle pitch)
{
	return clamp(pitch, GetMaxPitch(), GetMinPitch());
}

inline void setFreeAimVelocity(double& vel, double& zvel, const DAngle pitch, const double zvspeed)
{
	vel *= pitch.Cos();
	zvel = pitch.Sin() * zvspeed;
}

#include "updatesector.h"
