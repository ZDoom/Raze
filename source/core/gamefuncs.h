#pragma once

#include "gamecontrol.h"
#include "build.h"
#include "coreactor.h"
#include "intrect.h"

extern IntRect viewport3d;

// breadth first search, this gets used multiple times throughout the engine, mainly for iterating over sectors.
// Only works on indices, this has no knowledge of the actual objects being looked at.
// All objects of this type operate on the same shared store. Interleaved use is not allowed, nested use is fine.
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
	BUILDSINSHIFT = SINTABLEBITS - BUILDSINBITS,
};

constexpr double BAngRadian = pi::pi() * (1. / 1024.);
constexpr double BAngToDegree = 360. / 2048.;
constexpr DAngle DAngleBuildToDeg = DAngle::fromDeg(BAngToDegree);

extern int sintable[2048];

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
	return (shift -= BUILDSINSHIFT) < 0 ? sintable[ang & 2047] >> abs(shift) : sintable[ang & 2047] << shift;
}
inline double bsinf(const double ang, const int shift = 0)
{
	return g_sinbam(ang * BAMUNIT) * sinscale(shift);
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
	return (shift -= BUILDSINSHIFT) < 0 ? sintable[(ang + 512) & 2047] >> abs(shift) : sintable[(ang + 512) & 2047] << shift;
}
inline double bcosf(const double ang, const int shift = 0)
{
	return g_cosbam(ang * BAMUNIT) * sinscale(shift);
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

inline int getangle(const DVector2& vec)
{
	return getangle(vec.X, vec.Y);
}

inline int getangle(const vec2_t& vec)
{
	return getangle(vec.X, vec.Y);
}


//---------------------------------------------------------------------------
//
// Returns an angle delta for Build angles.
//
//---------------------------------------------------------------------------

inline constexpr int getincangle(int a, int na)
{
	return int(unsigned(na << 21) - unsigned(a << 21)) >> 21;
}


extern int cameradist, cameraclock;

void loaddefinitionsfile(const char* fn, bool cumulative = false, bool maingrp = false);

bool calcChaseCamPos(int* px, int* py, int* pz, DCoreActor* pspr, sectortype** psectnum, DAngle ang, fixedhoriz horiz, double const smoothratio);

void PlanesAtPoint(const sectortype* sec, float dax, float day, float* ceilz, float* florz);

int getslopeval(sectortype* sect, int x, int y, int z, int planez);



void setWallSectors();
void GetWallSpritePosition(const spritetypebase* spr, const DVector2& pos, DVector2* out, bool render = false);
void GetFlatSpritePosition(DCoreActor* spr, const DVector2& pos, DVector2* out, bool render = false);
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
void dragpoint(walltype* wal, int newx, int newy);
void dragpoint(walltype* wal, const DVector2& pos);
DVector2 rotatepoint(const DVector2& pivot, const DVector2& point, DAngle angle);
int32_t inside(double x, double y, const sectortype* sect);
void getcorrectzsofslope(int sectnum, int dax, int day, int* ceilz, int* florz);
int getceilzofslopeptr(const sectortype* sec, int dax, int day);
int getflorzofslopeptr(const sectortype* sec, int dax, int day);
void getzsofslopeptr(const sectortype* sec, int dax, int day, int* ceilz, int* florz);
void getzsofslopeptr(const sectortype* sec, double dax, double day, double* ceilz, double* florz);

inline double getceilzofslopeptrf(const sectortype* sec, double dax, double day)
{
	return getceilzofslopeptr(sec, dax * worldtoint, day * worldtoint) * zinttoworld;
}
inline double getflorzofslopeptrf(const sectortype* sec, double dax, double day)
{
	return getflorzofslopeptr(sec, dax * worldtoint, day * worldtoint) * zinttoworld;
}


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
sectortype* nextsectorneighborzptr(sectortype* sectp, int startz, int flags);



inline double WallStartX(int wallnum)
{
	return wall[wallnum].pos.X;
}

inline double WallStartY(int wallnum)
{
	return -wall[wallnum].pos.Y;
}

inline double WallEndX(int wallnum)
{
	return wall[wallnum].point2Wall()->pos.X;
}

inline double WallEndY(int wallnum)
{
	return -wall[wallnum].point2Wall()->pos.Y;
}

inline double WallStartX(const walltype* wallnum)
{
	return wallnum->pos.X;
}

inline double WallStartY(const walltype* wallnum)
{
	return -wallnum->pos.Y;
}

inline DVector2 WallStart(const walltype* wallnum)
{
	return { WallStartX(wallnum), WallStartY(wallnum) };
}

inline double WallEndX(const walltype* wallnum)
{
	return wallnum->point2Wall()->pos.X;
}

inline double WallEndY(const walltype* wallnum)
{
	return -wallnum->point2Wall()->pos.Y;
}

inline DVector2 WallEnd(const walltype* wallnum)
{
	return { WallEndX(wallnum), WallEndY(wallnum) };
}

inline DVector2 WallDelta(const walltype* wallnum)
{
	return WallEnd(wallnum) - WallStart(wallnum);
}

inline double PointOnLineSide(double x, double y, double linex, double liney, double deltax, double deltay)
{
	return (x - linex) * deltay - (y - liney) * deltax;
}

inline double PointOnLineSide(const DVector2 &pos, const walltype *line)
{
	return (pos.X - WallStartX(line)) * WallDelta(line).Y - (pos.Y - WallStartY(line)) * WallDelta(line).X;
}

template<class T>
inline double PointOnLineSide(const TVector2<T>& pos, const TVector2<T>& linestart, const TVector2<T>& lineend)
{
	return (pos.X - linestart.X) * (lineend.Y - linestart.Y) - (pos.Y - linestart.Y) * (lineend.X - linestart.X);
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

inline int32_t spriteGetZOfSlope(const spritetypebase* tspr, int dax, int day, int heinum)
{
	if (heinum == 0) return tspr->int_pos().Z;

	int const j = DMulScale(bsin(tspr->int_ang() + 1024), day - tspr->int_pos().Y, -bsin(tspr->int_ang() + 512), dax - tspr->int_pos().X, 4);
	return tspr->int_pos().Z + MulScale(heinum, j, 18);
}

inline int inside(int x, int y, const sectortype* sect)
{
	return inside(x * inttoworld, y * inttoworld, sect);
}

// still needed by some parts in the engine.
inline int inside_p(int x, int y, int sectnum) { return (sectnum >= 0 && inside(x, y, &sector[sectnum]) == 1); }



inline int I_GetBuildTime()
{
	return I_GetTime(120);
}

inline int32_t getangle(walltype* wal)
{
	return getangle(wal->fdelta());
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

inline double SquareDist(double lx1, double ly1, double lx2, double ly2)
{
	double dx = lx2 - lx1;
	double dy = ly2 - ly1;
	return dx * dx + dy * dy;
}

inline DVector2 NearestPointLine(double px, double py, const walltype* wal)
{
	double lx1 = wal->pos.X;
	double ly1 = wal->pos.Y;
	double lx2 = wal->point2Wall()->pos.X;
	double ly2 = wal->point2Wall()->pos.Y;

	double wall_length = SquareDist(lx1, ly1, lx2, ly2);

	if (wall_length == 0) return { lx1, ly1 };

	double t = ((px - lx1) * (lx2 - lx1) + (py - ly1) * (ly2 - ly1)) / wall_length;
	double xx = lx1 + t * (lx2 - lx1);
	double yy = ly1 + t * (ly2 - ly1);
	return { xx, yy };
}

inline DVector2 NearestPointOnWall(double px, double py, const walltype* wal)
{
	double lx1 = wal->pos.X;
	double ly1 = wal->pos.Y;
	double lx2 = wal->point2Wall()->pos.X;
	double ly2 = wal->point2Wall()->pos.Y;

	double wall_length = SquareDist(lx1, ly1, lx2, ly2);

	if (wall_length == 0) return { lx1, ly1 };

	double t = ((px - lx1) * (lx2 - lx1) + (py - ly1) * (ly2 - ly1)) / wall_length;
	if (t <= 0) return { lx1, ly1 };
	if (t >= 1) return { lx2, ly2 };
	double xx = lx1 + t * (lx2 - lx1);
	double yy = ly1 + t * (ly2 - ly1);
	return { xx, yy };
}

inline double SquareDistToWall(double px, double py, const walltype* wal, DVector2* point = nullptr) 
{
	auto pt = NearestPointOnWall(px, py, wal);
	if (point) *point = pt;
	return SquareDist(px, py, pt.X, pt.Y);
}

double SquareDistToSector(double px, double py, const sectortype* sect, DVector2* point = nullptr);

inline double SquareDistToLine(double px, double py, double lx1, double ly1, double lx2, double ly2)
{
	double wall_length = SquareDist(lx1, ly1, lx2, ly2);

	if (wall_length == 0) return SquareDist(px, py, lx1, ly1);

	double t = ((px - lx1) * (lx2 - lx1) + (py - ly1) * (ly2 - ly1)) / wall_length;
	t = clamp(t, 0., 1.);
	double xx = lx1 + t * (lx2 - lx1);
	double yy = ly1 + t * (ly2 - ly1);
	return SquareDist(px, py, xx, yy);
}

inline void alignceilslope(sectortype* sect, int x, int y, int z)
{
	sect->setceilingslope(getslopeval(sect, x, y, z, sect->int_ceilingz()));
}

inline void alignflorslope(sectortype* sect, int x, int y, int z)
{
	sect->setfloorslope(getslopeval(sect, x, y, z, sect->int_floorz()));
}

#include "updatesector.h"
