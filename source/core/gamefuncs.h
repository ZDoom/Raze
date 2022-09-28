#pragma once

#include "gamecontrol.h"
#include "gamestruct.h"
#include "build.h"
#include "coreactor.h"
#include "intrect.h"
#include "geometry.h"

extern IntRect viewport3d;

constexpr int SLOPEVAL_FACTOR = 4096;

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


//---------------------------------------------------------------------------
//
// Constants used for Build sine/cosine functions.
//
//---------------------------------------------------------------------------

enum
{
	BAMBITS = 21,
	BAMUNIT = 1 << BAMBITS,
	SINTABLEBITS = 30,
	SINTABLEUNIT = 1 << SINTABLEBITS,
	BUILDSINBITS = 14,
};

constexpr double BAngRadian = pi::pi() * (1. / 1024.);
constexpr double BAngToDegree = 360. / 2048.;
constexpr DAngle DAngleBuildToDeg = DAngle::fromDeg(BAngToDegree);

inline constexpr double sinscale(const int shift)
{
	return shift >= -BUILDSINBITS ? uint64_t(1) << (BUILDSINBITS + shift) : 1. / (uint64_t(1) << abs(BUILDSINBITS + shift));
}


//---------------------------------------------------------------------------
//
// Build sine inline functions.
//
//---------------------------------------------------------------------------

inline int bsin(const int ang, int shift = 0)
{
	return int(g_sinbam(ang * BAMUNIT) * sinscale(shift));
}

inline int Sin(int ang)
{
	return bsin(ang, 16);
}

//---------------------------------------------------------------------------
//
// Build cosine inline functions.
// 
// About shifts:
// -6 -> * 16
// -7 -> * 8
// -8 -> * 4
// -9 -> * 2
// -10 -> * 1
//
//---------------------------------------------------------------------------

inline int bcos(const int ang, int shift = 0)
{
	return int(g_cosbam(ang * BAMUNIT) * sinscale(shift));
}

inline int Cos(int ang)
{
	return bcos(ang, 16);
}

//---------------------------------------------------------------------------
//
// High precision vector angle function, mainly for the renderer.
//
//---------------------------------------------------------------------------

inline int getangle(double xvect, double yvect)
{
	return DVector2(xvect, yvect).Angle().Buildang();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

extern double cameradist, cameraclock;

void loaddefinitionsfile(const char* fn, bool cumulative = false, bool maingrp = false);

bool calcChaseCamPos(DVector3& ppos, DCoreActor* pspr, sectortype** psectnum, DAngle ang, DAngle horiz, double const interpfrac);
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
[[deprecated]] void dragpoint(walltype* wal, int newx, int newy);
void dragpoint(walltype* wal, const DVector2& pos);
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


// only used by clipmove et.al.
void getcorrectzsofslope(int sectnum, int dax, int day, double* ceilz, double* florz);

//==========================================================================
//
// for the game engine
//
//==========================================================================

inline void getzsofslopeptr(const sectortype* sec, double dax, double day, double* ceilz, double* florz)
{
	calcSlope(sec, dax, day, ceilz, florz);
}

template<class Vector>
inline void getzsofslopeptr(const sectortype* sec, const Vector& pos, double* ceilz, double* florz)
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
	return ((actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_SLOPE) ? 0 : uint8_t(actor->spr.xoffset) + (uint8_t(actor->spr.yoffset) << 8);
}

// same stuff, different flag...
inline int tspriteGetSlope(const tspritetype* spr)
{
	return !(spr->clipdist & TSPR_SLOPESPRITE) ? 0 : uint8_t(spr->xoffset) + (int8_t(spr->yoffset) << 8);
}

inline double spriteGetZOfSlopef(const spritetypebase* tspr, const DVector2& pos, int heinum)
{
	if (heinum == 0) return tspr->pos.Z;
	auto ang = tspr->angle;
	double factor = -ang.Sin() * (pos.X - tspr->pos.Y) - ang.Cos() * (pos.X - tspr->pos.X);
	return tspr->pos.Z + heinum * factor * (1. / SLOPEVAL_FACTOR);
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

inline double GetRayIntersect(const DVector3& start1, const DVector3& vect1, const DVector2& start2, const DVector2& vect2, DVector3& retv)
{
	double factor2;
	double factor = InterceptLineSegments(start1.X, start1.Y, vect1.X, vect1.Y, start2.X, start2.Y, vect2.X, vect2.Y, &factor2);
	if (factor <= 0) return -1;
	retv = start1 + factor * vect1;
	return factor2;
}

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
	return !cl_clampedpitch ? -DAngle90 : gi->playerPitchMin();
}

inline DAngle GetMaxPitch()
{
	return !cl_clampedpitch ? DAngle90 : gi->playerPitchMax();
}

inline DAngle ClampViewPitch(const DAngle pitch)
{
	return clamp(pitch, GetMinPitch(), GetMaxPitch());
}

//==========================================================================
//
// old deprecated integer versions
//
//==========================================================================

[[deprecated]]
inline int int_getceilzofslopeptr(const sectortype* sec, int dax, int day)
{
	double z;
	calcSlope(sec, dax * inttoworld, day * inttoworld, &z, nullptr);
	return int(z * zworldtoint);
}

[[deprecated]]
inline int int_getflorzofslopeptr(const sectortype* sec, int dax, int day)
{
	double z;
	calcSlope(sec, dax * inttoworld, day * inttoworld, nullptr, &z);
	return int(z * zworldtoint);
}

[[deprecated]]
inline void int_getzsofslopeptr(const sectortype* sec, int dax, int day, int* ceilz, int* florz)
{
	double c, f;
	calcSlope(sec, dax * inttoworld, day * inttoworld, &c, &f);
	*ceilz = int(c * zworldtoint);
	*florz = int(f * zworldtoint);
}

[[deprecated]]
inline int rintersect(int x1, int y1, int z1, int vx, int vy, int vz, int x3, int y3, int x4, int y4, int* intx, int* inty, int* intz)
{
	DVector3 retv;
	double result = GetRayIntersect(DVector3(x1 * inttoworld, y1 * inttoworld, z1 * zinttoworld), DVector3(vx * inttoworld, vy * inttoworld, vz * zinttoworld),
		DVector2(x3 * inttoworld, y3 * inttoworld), DVector2((x4 - x3) * inttoworld, (y4 - y3) * inttoworld), retv);
	if (result < 0) return -1;
	*intx = retv.X * worldtoint;
	*inty = retv.Y * worldtoint;
	*intz = retv.Z * zworldtoint;
	return FloatToFixed(result);
}

[[deprecated]]
inline int cansee(int x1, int y1, int z1, sectortype* sect1, int x2, int y2, int z2, sectortype* sect2)
{
	return cansee(DVector3(x1 * inttoworld, y1 * inttoworld, z1 * zinttoworld), sect1, DVector3(x2 * inttoworld, y2 * inttoworld, z2 * zinttoworld), sect2);
}

[[deprecated]]
inline int inside(int x, int y, const sectortype* sect)
{
	return inside(x * inttoworld, y * inttoworld, sect);
}

// still needed by some parts in the engine.
[[deprecated]]
inline int inside_p(int x, int y, int sectnum) { return (sectnum >= 0 && inside(x * inttoworld, y * inttoworld, &sector[sectnum]) == 1); }





#include "updatesector.h"
