//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020, 2021 - Christoph Oelckers

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#pragma once

#include <stdint.h>
#include "ns.h"
#include "tarray.h"
#include "tflags.h"
#include "intvec.h"
#include "dobject.h"

void MarkVerticesForSector(int sector);

static constexpr double maptoworld = (1 / 16.);	// this for necessary conversions to convert map data to floating point representation.
static constexpr double inttoworld = (1 / 16.); // this is for conversions needed to make floats coexist with existing code.
static constexpr double worldtoint = 16.;

static constexpr double zmaptoworld = (1 / 256.);	// this for necessary conversions to convert map data to floating point representation.
static constexpr double zinttoworld = (1 / 256.); // this is for conversions needed to make floats coexist with existing code.
static constexpr double zworldtoint = 256.;

static constexpr double REPEAT_SCALE = (1 / 64.);	// map's 'repeat' values use 2.6 fixed point.

//=============================================================================
//
// Constants
//
//=============================================================================

// the various portal types
enum EPortalBits
{
	PORTAL_SECTOR_FLOOR				= 1,
	PORTAL_SECTOR_CEILING			= 2,
	PORTAL_SECTOR_FLOOR_REFLECT		= 3,
	PORTAL_SECTOR_CEILING_REFLECT	= 4,
	PORTAL_WALL_VIEW				= 5,
	PORTAL_WALL_MIRROR				= 6,
	PORTAL_WALL_TO_SPRITE			= 7,
	PORTAL_SECTOR_GEOMETRY			= 8,
};

// Original Build sector bit flags.
enum ESectorBits
{
	CSTAT_SECTOR_SKY				= 1,
	CSTAT_SECTOR_SLOPE				= 2,
	CSTAT_SECTOR_SWAPXY				= 4,
	CSTAT_SECTOR_TEXHALF			= 8,
	CSTAT_SECTOR_XFLIP				= 16,
	CSTAT_SECTOR_YFLIP				= 32,
	CSTAT_SECTOR_ALIGN				= 64,
	CSTAT_SECTOR_TRANS				= 128,
	CSTAT_SECTOR_TRANS_INVERT		= 256,
	CSTAT_SECTOR_METHOD				= 384,
	CSTAT_SECTOR_FAF_BLOCK_HITSCAN	= 32768,	// SW only

	CSTAT_SECTOR_EXHUMED_BIT1			= 1 << 14,
	CSTAT_SECTOR_EXHUMED_BIT2			= 1 << 15,

	CSTAT_SECTOR_NO_CEILINGSHADE	= 32768,	// Blood: Force use of floorshade for sprites, even in sky sectors.

};

typedef TFlags<ESectorBits, uint16_t> ESectorFlags;
DEFINE_TFLAGS_OPERATORS(ESectorFlags)


// Extended sector bit flags.
enum ESectorExBits
{
	SECTOREX_CLOUDSCROLL			= 1,
	SECTOREX_DRAGGED				= 2,
	SECTOREX_DONTCLIP				= 4,
};

// Flags for retriangulation
enum EDirty
{
	FloorDirty		= 1,
	CeilingDirty	= 2,
	GeometryDirty	= 4,
	AllDirty		= 7
};

enum EWallBits // names are from Shadow Warrior
{
	CSTAT_WALL_BLOCK				= 1,			//   bit 0: 1 = Blocking wall (use with clipmove, getzrange)         "B"
	CSTAT_WALL_BOTTOM_SWAP			= 2,			//   bit 1: 1 = bottoms of invisible walls swapped, 0 = not          "2"
	CSTAT_WALL_ALIGN_BOTTOM			= 4,			//   bit 2: 1 = align picture on bottom (for doors), 0 = top         "O"
	CSTAT_WALL_XFLIP				= 8,			//   bit 3: 1 = x-flipped, 0 = normal                                "F"
	CSTAT_WALL_MASKED				= 16,			//   bit 4: 1 = masking wall, 0 = not                                "M"
	CSTAT_WALL_1WAY					= 32,			//   bit 5: 1 = 1-way wall, 0 = not                                  "1"
	CSTAT_WALL_BLOCK_HITSCAN		= 64,			//   bit 6: 1 = Blocking wall (use with hitscan / cliptype 1)        "H"
	CSTAT_WALL_TRANSLUCENT			= 128,			//   bit 7: 1 = Transluscence, 0 = not                               "T"
	CSTAT_WALL_YFLIP				= 256,			//   bit 8: 1 = y-flipped, 0 = normal                                "F"
	CSTAT_WALL_TRANS_FLIP			= 512,			//   bit 9: 1 = Transluscence reversing, 0 = normal                  "T"
	CSTAT_WALL_ANY_EXCEPT_BLOCK		= 254,			// Duke stupidity

	CSTAT_WALL_ROTATE_90			= 1<<12,		// EDuke32 extension supported by Raze

	CSTAT_WALL_BLOCK_ACTOR			= 1<<14,		// SW specific.
	CSTAT_WALL_WARP_HITSCAN			= 1<<15,		// SW specific.

	CSTAT_WALL_MOVE_FORWARD			= 1 << 14,		// Blood specific
	CSTAT_WALL_MOVE_BACKWARD		= 1 << 15,		// Blood specific
	CSTAT_WALL_MOVE_MASK			= CSTAT_WALL_MOVE_FORWARD | CSTAT_WALL_MOVE_BACKWARD

};

typedef TFlags<EWallBits, uint16_t> EWallFlags;
DEFINE_TFLAGS_OPERATORS(EWallFlags)

enum ESpriteBits // names mostly from SW.
{
	CSTAT_SPRITE_BLOCK				= 1,			//   bit 0: 1 = Blocking sprite (use with clipmove, getzrange)       "B"
	CSTAT_SPRITE_TRANSLUCENT		= 2,			//   bit 1: 1 = transluscence, 0 = normal                            "T"
	CSTAT_SPRITE_XFLIP				= 4,			//   bit 2: 1 = x-flipped, 0 = normal                                "F"
	CSTAT_SPRITE_YFLIP				= 8,			//   bit 3: 1 = y-flipped, 0 = normal                                "F"

	CSTAT_SPRITE_ALIGNMENT_FACING	= 0,			//   bits 5-4: 00 = FACE sprite (default)                            "R"
	CSTAT_SPRITE_ALIGNMENT_WALL		= 16,			//             01 = WALL sprite (like masked walls)
	CSTAT_SPRITE_ALIGNMENT_FLOOR	= 32,			//             10 = FLOOR sprite (parallel to ceilings&floors)
	CSTAT_SPRITE_ALIGNMENT_SLAB		= 48,			//             11 = either voxel or slope sprite, depending on the situation
	CSTAT_SPRITE_ALIGNMENT_SLOPE	= 48,
	CSTAT_SPRITE_ALIGNMENT_MASK		= 48,

	CSTAT_SPRITE_ONE_SIDE			= 64,			//   bit 6: 1 = 1-sided sprite, 0 = normal                           "1"
	CSTAT_SPRITE_YCENTER			= 128,			//   bit 7: 1 = Real centered centering, 0 = foot center             "C"
	CSTAT_SPRITE_BLOCK_HITSCAN		= 256,			//   bit 8: 1 = Blocking sprite (use with hitscan / cliptype 1)      "H"
	CSTAT_SPRITE_TRANS_FLIP			= 512,			//   bit 9: 1 = Transluscence reversing, 0 = normal                  "T"

	CSTAT_SPRITE_BLOCK_ALL			= CSTAT_SPRITE_BLOCK_HITSCAN | CSTAT_SPRITE_BLOCK, // 257
	CSTAT_SPRITE_INVISIBLE = 32768,		//   bit 15: 1 = Invisible sprite, 0 = not invisible

	// SW flags
	CSTAT_SPRITE_RESTORE			= 1<<12, 
	CSTAT_SPRITE_CLOSE_FLOOR		= 1<<13, //tells whether a sprite started out close to a ceiling or floor
	CSTAT_SPRITE_BLOCK_MISSILE		= 1<<14, 
	CSTAT_SPRITE_BREAKABLE			= CSTAT_SPRITE_BLOCK_HITSCAN|CSTAT_SPRITE_BLOCK_MISSILE,

	// Blood flags
	CSTAT_SPRITE_BLOOD_BIT2			= 1024, // Both of these get set but not checked directly, so no idea what they mean...
	CSTAT_SPRITE_BLOOD_BIT1			= 4096,
	CSTAT_SPRITE_MOVE_FORWARD		= 8192,
	CSTAT_SPRITE_MOVE_REVERSE		= 16384,
	CSTAT_SPRITE_MOVE_MASK			= CSTAT_SPRITE_MOVE_FORWARD | CSTAT_SPRITE_MOVE_REVERSE,
};

typedef TFlags<ESpriteBits, uint16_t> ESpriteFlags;
DEFINE_TFLAGS_OPERATORS(ESpriteFlags)

enum ESpriteBits2
{
	CSTAT2_SPRITE_NOFIND = 1,		// Invisible to neartag and hitscan
	CSTAT2_SPRITE_MAPPED = 2,		// sprite was mapped for automap
	CSTAT2_SPRITE_NOSHADOW = 4,		// cast no shadow.
	CSTAT2_SPRITE_DECAL = 8,		// always attached to a wall.
};

// tsprite flags use the otherwise unused clipdist field.
enum ETSprFlags
{
	TSPR_FLAGS_MDHACK = 1,		// Currently unused: set for model shadows
	TSPR_FLAGS_DRAW_LAST = 2,	// Currently unused: checked by Polymost but never set.
	TSPR_MDLROTATE = 4,			// rotate if this is a model or voxel.
	TSPR_SLOPESPRITE = 8,       // render as sloped sprite
};


//=============================================================================
//
// external references
//
//=============================================================================


// For Blood we need to pull in some game specific references.
BEGIN_BLD_NS
	struct XWALL;
	struct XSECTOR;
END_BLD_NS

class DCoreActor;
struct walltype;

// enable for running a compile-check to ensure that renderer-critical variables are not being written to directly.
//#define SECTOR_HACKJOB

//=============================================================================
//
// internal sector struct - no longer identical with on-disk format
//
//=============================================================================

struct sectortype
{

	// Fields were reordered by size, some also enlarged.
	DCoreActor* firstEntry, * lastEntry;

	int32_t wallptr;
#ifdef SECTOR_HACKJOB
	// Debug hack job for finding all places where ceilingz and floorz get written to.
	// If the engine does not compile with this block on, we got a problem.
	// Since this is only for compile verification there's no need to provide a working implementation.
	const double floorz, ceilingz;
	sectortype(double a = 0, double b = 0) : ceilingz(a), floorz(b) {}

	void set_int_ceilingz(int cc, bool temp = false) {}
	void set_int_floorz(int cc, bool temp = false) {}
	void add_int_ceilingz(int cc, bool temp = false) {}
	void add_int_floorz(int cc, bool temp = false) {}

#else
	// Do not change directly!
	double floorz, ceilingz;

	void setceilingz(double cc, bool temp = false);
	void setfloorz(double cc, bool temp = false);
	void addceilingz(double cc, bool temp = false);
	void addfloorz(double cc, bool temp = false);

	void set_int_ceilingz(int cc, bool temp = false) { setceilingz(cc * zinttoworld, temp); }
	void set_int_floorz(int cc, bool temp = false) { setfloorz(cc * zinttoworld, temp); }
	void add_int_ceilingz(int cc, bool temp = false) { addceilingz(cc * zinttoworld, temp); }
	void add_int_floorz(int cc, bool temp = false) { addfloorz(cc * zinttoworld, temp); }

	void setzfrommap(int c, int f)
	{
		ceilingz = c * zmaptoworld;
		floorz = f * zmaptoworld;
	}

#endif

	int int_ceilingz() const { return ceilingz * zworldtoint; }
	int int_floorz() const { return floorz * zworldtoint; }


	// panning byte fields were promoted to full floats to enable panning interpolation.
	float ceilingxpan_;
	float ceilingypan_;
	float floorxpan_;
	float floorypan_;

	int16_t wallnum;
	ESectorFlags ceilingstat;
	ESectorFlags floorstat;
	int16_t ceilingpicnum;
	int16_t ceilingheinum;
	int16_t floorpicnum;
	int16_t floorheinum;
	union { int16_t lotag, type; }; // type is for Blood.
	int16_t hitag;
	int16_t extra;

	int8_t ceilingshade;
	uint8_t ceilingpal;
	int8_t floorshade;
	uint8_t floorpal;;
	uint8_t visibility;
	uint8_t fogpal; // EDuke32 extension - was originally a filler byte

	// new additions not from the binary map format.
	uint8_t dirty;
	uint8_t exflags;
	uint8_t portalflags;
	int8_t portalnum;

	DAngle angle; // this is SW only. GCC is stupid and does not allow it inside an anonmyous struct.

	// Game specific extensions. Due to how sectors are used they need to be defined in the global class. :(
	union
	{
		struct // DukeRR
		{
			uint8_t keyinfo; // This was originally the repurposed filler byte.
			uint8_t shadedsector;
			TObjPtr<DCoreActor*> hitagactor;    // we need this because Duke stores an actor in the hitag field. Is really a DDukeActor, but cannot be declared here safely.
		};
		struct // Blood
		{
			BLD_NS::XSECTOR* _xs;
			TObjPtr<DCoreActor*> upperLink;
			TObjPtr<DCoreActor*> lowerLink;
			double baseFloor;
			double baseCeil;
			double velFloor;
			double velCeil;
			uint8_t slopewallofs; // This was originally the repurposed filler byte.
		};
		struct // Exhumed
		{
			sectortype* pSoundSect;
			sectortype* pAbove;
			sectortype* pBelow;
			int   Depth;
			short Sound;
			short Flag;
			short Damage;
			short Speed;
		};
		struct // SW
		{
			// No need to allocate this on demand as it is smaller than what Blood needs.
			int flags;
			int depth_fixed;
			short stag;    // ST? tag number - for certain things it helps to know it
			short height;
			short speed;
			short damage;
			short number;  // usually used for matching number
			bool u_defined;
			uint8_t flags2;
		};

	};

	int ceilingxpan() const { return int(ceilingxpan_); }
	int ceilingypan() const { return int(ceilingypan_); }
	int floorxpan() const { return int(floorxpan_); }
	int floorypan() const { return int(floorypan_); }
	void setfloorxpan(float val) { floorxpan_ = fmodf(val + 512, 256); } // +512 is for handling negative offsets
	void setfloorypan(float val) { floorypan_ = fmodf(val + 512, 256); } // +512 is for handling negative offsets
	void setceilingxpan(float val) { ceilingxpan_ = fmodf(val + 512, 256); } // +512 is for handling negative offsets
	void setceilingypan(float val) { ceilingypan_ = fmodf(val + 512, 256); } // +512 is for handling negative offsets
	void addfloorxpan(float add) { floorxpan_ = fmodf(floorxpan_ + add + 512, 256); } // +512 is for handling negative offsets
	void addfloorypan(float add) { floorypan_ = fmodf(floorypan_ + add + 512, 256); } // +512 is for handling negative offsets
	void addceilingxpan(float add) { ceilingxpan_ = fmodf(ceilingxpan_ + add + 512, 256); } // +512 is for handling negative offsets
	void addceilingypan(float add) { ceilingypan_ = fmodf(ceilingypan_ + add + 512, 256); } // +512 is for handling negative offsets
	void setceilingslope(int heinum) { ceilingheinum = heinum; if (heinum) ceilingstat |= CSTAT_SECTOR_SLOPE; else ceilingstat &= ~CSTAT_SECTOR_SLOPE; }
	void setfloorslope(int heinum) { floorheinum = heinum; if (heinum) floorstat |= CSTAT_SECTOR_SLOPE; else floorstat &= ~CSTAT_SECTOR_SLOPE; }
	int getfloorslope() const { return floorstat & CSTAT_SECTOR_SLOPE ? floorheinum : 0; }
	int getceilingslope() const { return ceilingstat & CSTAT_SECTOR_SLOPE ? ceilingheinum : 0; }
	walltype* firstWall() const;
	walltype* lastWall() const;


	Blood::XSECTOR& xs() const { return *_xs;  }
	bool hasX() const { return _xs != nullptr; } // 0 is invalid!
	void allocX();

	// same for SW
	bool hasU() const { return u_defined; }
};

//=============================================================================
//
// internal wall struct - no longer identical with on-disk format
//
//=============================================================================

struct walltype
{
	DVector2 pos;

	vec2_t wall_int_pos() const { return vec2_t(pos.X * worldtoint, pos.Y * worldtoint); };
	vec2_t int_delta() const { return point2Wall()->wall_int_pos() - wall_int_pos(); }

	void setPosFromMap(int x, int y) { pos = { x * maptoworld, y * maptoworld }; }

	int32_t point2;
	int32_t nextwall;
	int32_t sector;	// Build never had this...
	int32_t nextsector;

	// Again, panning fields extended for interpolation.
	float xpan_;
	float ypan_;

	EWallFlags cstat;
	int16_t picnum;
	int16_t overpicnum;
	union { int16_t lotag, type; }; // type is for Blood
	int16_t hitag;
	int16_t extra;

	int8_t shade;
	uint8_t pal;
	uint8_t xrepeat;
	uint8_t yrepeat;

	// extensions not from the binary map format.
	angle_t clipangle;
	int length; // cached value to avoid calling sqrt repeatedly.

	uint16_t portalnum;
	uint8_t portalflags;
	uint8_t lengthflags;

	// Blood is the only game which extends the wall struct.
	Blood::XWALL* _xw;
	DVector2 baseWall;

	int xpan() const { return int(xpan_); }
	int ypan() const { return int(ypan_); }
	void setxpan(float add) { xpan_ = fmodf(add + 512, 256); } // +512 is for handling negative offsets
	void setypan(float add) { ypan_ = fmodf(add + 512, 256); } // +512 is for handling negative offsets
	void addxpan(float add) { xpan_ = fmodf(xpan_ + add + 512, 256); } // +512 is for handling negative offsets
	void addypan(float add) { ypan_ = fmodf(ypan_ + add + 512, 256); } // +512 is for handling negative offsets
	sectortype* nextSector() const;
	sectortype* sectorp() const;
	walltype* nextWall() const;
	walltype* lastWall(bool fast  = true) const;
	walltype* point2Wall() const;
	DVector2 delta() const { return point2Wall()->pos - pos; }
	DVector2 center() const { return(point2Wall()->pos + pos) / 2; }
	bool twoSided() const { return nextsector >= 0; }
	int Length();
	void calcLength();	// this is deliberately not inlined and stored in a file where it can't be found at compile time.
	void movexy(int newx, int newy);
	void move(const DVector2& vec)
	{
		pos = vec;
		moved();
	}

	void moved();

	Blood::XWALL& xw() const { return *_xw; }
	bool hasX() const { return _xw != nullptr; }
	void allocX();
};

//=============================================================================
//
// internal sprite struct - no longer identical with on-disk format
//
//=============================================================================

struct spritetypebase
{
	DVector3 pos;

	sectortype* sectp;
	DAngle angle;

	ESpriteFlags cstat;
	int16_t picnum;
	int16_t statnum;
	int16_t intangle;	// needs to be kept for SW's SP_TAG4
	int16_t xint;		// formerly known as xvel and yvel
	int16_t yint;
	int16_t inittype; // was zvel. All accesses for that have been wrapped. inittype, type and flags are for Blood.
	union { int16_t lotag, type; };
	union { int16_t hitag, flags; };
	int16_t extra;
	int16_t detail;
	uint16_t cstat2;

	int8_t shade;
	uint8_t pal;
	uint8_t clipdist;
	uint8_t blend;
	uint8_t xrepeat;
	uint8_t yrepeat;
	int8_t xoffset;
	int8_t yoffset;

	void SetMapPos(int x, int y, int z)
	{
		pos = { x * maptoworld, y * maptoworld, z * zmaptoworld };
	}

	const vec3_t int_pos() const
	{
		return { int(pos.X * worldtoint), int(pos.Y * worldtoint), int(pos.Z * zworldtoint) };
	}

	constexpr int int_ang() const
	{
 		return angle.Buildang();
	}

	void set_int_ang(int a)
	{
		angle = DAngle::fromBuild(a);
	}

	void add_int_ang(int a)
	{
		angle += DAngle::fromBuild(a);
	}
};


struct spritetype : public spritetypebase
{
	int16_t intowner;
};

//=============================================================================
//
// This is mostly the same, but it omits the 'owner' field in favor of a full actor pointer. 
// Incompatibility with spritetype regarding assignments is deliberate as these serve a fundamentally different purpose!
//
//=============================================================================

struct tspritetype : public spritetypebase
{
	DCoreActor* ownerActor;
	int time;

	void add_int_z(int x)
	{
		pos.Z += x * zinttoworld;
	}
	void set_int_z(int x)
	{
		pos.Z = x * zinttoworld;
	}
};

class tspriteArray
{
	static const int blockbits = 9;
	static const int blocksize = 1 << blockbits;
	struct block
	{
		tspritetype data[blocksize];
	};
	TDeletingArray<block*> blocks;
	unsigned size;

public:
	tspritetype* newTSprite()
	{
		if ((size & (blocksize - 1)) == 0) blocks.Push(new block);
		return get(size++);
	}

	tspritetype* get(unsigned index)
	{
		assert(index < size);
		return &blocks[index >> blockbits]->data[index & (blocksize - 1)];
	}

	tspritetype* current()
	{
		return get(size - 1);
	}

	void clear()
	{
		blocks.DeleteAndClear();
		size = 0;
	}

	unsigned Size() const
	{
		return size;
	}
};

extern TArray<sectortype> sector;
extern TArray<walltype> wall;
extern TArray<sectortype> sectorbackup;
extern TArray<walltype> wallbackup;
extern BitArray gotsector;

//=============================================================================
//
// inlines
//
//=============================================================================

inline bool validSectorIndex(int sectnum)
{
	return (unsigned)sectnum < sector.Size();
}

inline bool validWallIndex(int wallnum)
{
	return (unsigned)wallnum < wall.Size();
}

inline sectortype* walltype::nextSector() const
{
	return !validSectorIndex(nextsector)? nullptr :  &::sector[nextsector];
}

inline walltype* walltype::nextWall() const
{
	return !validWallIndex(nextwall)? nullptr : &::wall[nextwall];
}

inline sectortype* walltype::sectorp() const
{
	return &::sector[sector]; // cannot be -1 in a proper map.
}

inline walltype* walltype::lastWall(bool fast) const
{
	int index = wall.IndexOf(this);
	if (fast && index > 0 && wall[index - 1].point2 == index) return &wall[index - 1];

	int check = index;
	for (int i = 0; i < 16384; i++)	// don't run endlessly in case of malformed sectors.
	{
		int next = wall[check].point2;
		if (next == index) return &wall[check];
		check = next;
	}
	return nullptr;
}

inline walltype* walltype::point2Wall() const
{
	return &::wall[point2]; // cannot be -1 in a proper map.
}

inline walltype* sectortype::firstWall() const
{
	return &wall[wallptr]; // cannot be -1 in a proper map
}

inline walltype* sectortype::lastWall() const
{
	return &wall[wallptr + wallnum - 1]; // cannot be -1 in a proper map
}

inline void walltype::moved() 
{
	lengthflags = 3;
	sectorp()->dirty = EDirty::AllDirty;
}

inline void walltype::movexy(int newx, int newy)
{
	pos.X = newx * inttoworld;
	pos.Y = newy * inttoworld;
	lengthflags = 3;
	sectorp()->dirty = EDirty::AllDirty;
}

inline int walltype::Length()
{
	if ((lengthflags & 1) || (point2Wall()->lengthflags & 2))
	{
		// value is stale, recreate
		calcLength();
	}
	return length;
}

#ifndef SECTOR_HACKJOB

inline void sectortype::setceilingz(double cc, bool temp)
{
	ceilingz = cc;
	if (!temp) MarkVerticesForSector(sector.IndexOf(this));
}
inline void sectortype::setfloorz(double cc, bool temp)
{
	floorz = cc;
	if (!temp) MarkVerticesForSector(sector.IndexOf(this));
}
inline void sectortype::addceilingz(double cc, bool temp)
{
	ceilingz += cc;
	if (!temp) MarkVerticesForSector(sector.IndexOf(this));
}
inline void sectortype::addfloorz(double cc, bool temp)
{
	floorz += cc;
	if (!temp) MarkVerticesForSector(sector.IndexOf(this));
}

#endif

// copied from build.h so that coreactor.h does not have to include build.h

enum
{
	SPREXT_NOTMD = 1,
	SPREXT_NOMDANIM = 2,
	SPREXT_AWAY1 = 4,
	SPREXT_AWAY2 = 8,
	SPREXT_TSPRACCESS = 16,
	SPREXT_TEMPINVISIBLE = 32,
};

struct spriteext_t
{
	uint32_t mdanimtims;
	int16_t mdanimcur;
	int16_t angoff, pitch, roll;
	vec3_t pivot_offset;
	DVector3 position_offset;
	uint8_t renderflags;
	float alpha;
};

struct spritesmooth_t
{
	float smoothduration;
	int16_t mdcurframe, mdoldframe;
	int16_t mdsmooth;
};

struct SpawnSpriteDef
{
	TArray<spritetype> sprites;
	TArray<spriteext_t> sprext;
};


//=============================================================================
//
// Map loader stuff
//
//=============================================================================
struct SpawnSpriteDef;

void allocateMapArrays(int numwall, int numsector, int numsprites);
void validateSprite(spritetype& spr, int secno, int index);
void fixSectors();
void loadMap(const char *filename, int flags, DVector3 *pos, int16_t *ang, int *cursectnum, SpawnSpriteDef& sprites);
TArray<walltype> loadMapWalls(const char* filename);
void loadMapBackup(const char* filename);
void loadMapHack(const char* filename, const uint8_t*, SpawnSpriteDef& sprites);
void validateStartSector(const char* filename, const DVector3& pos, int* cursectnum, unsigned numsectors, bool noabort = false);
