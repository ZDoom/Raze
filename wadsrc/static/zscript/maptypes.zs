
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

}

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

}

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
}

enum ESpriteBits2
{
	CSTAT2_SPRITE_NOFIND = 1,		// Invisible to neartag and hitscan
	CSTAT2_SPRITE_MAPPED = 2,		// sprite was mapped for automap
	CSTAT2_SPRITE_NOSHADOW = 4,		// cast no shadow.
	CSTAT2_SPRITE_DECAL = 8,		// always attached to a wall.
}

// tsprite flags use the otherwise unused clipdist field.
enum ETSprFlags
{
	TSPR_FLAGS_MDHACK = 1,		// Currently unused: set for model shadows
	TSPR_FLAGS_DRAW_LAST = 2,	// Currently unused: checked by Polymost but never set.
	TSPR_MDLROTATE = 4,			// rotate if this is a model or voxel.
	TSPR_SLOPESPRITE = 8,       // render as sloped sprite
}



//=============================================================================
//
// internal sector struct - no longer identical with on-disk format
//
//=============================================================================

struct sectortype native
{
	// panning byte fields were promoted to full floats to enable panning interpolation.
	native readonly float ceilingxpan;
	native readonly float ceilingypan;
	native readonly float floorxpan;
	native readonly float floorypan;

	native readonly int wallptr;
	native readonly int16 wallnum;
	native int16 ceilingstat;
	native int16 floorstat;
	//int16 ceilingpicnum;
	//int16 floorpicnum;

	native int16 lotag;
	native int16 type; // type is an alias of lotag for Blood.
	native int16 hitag;
	native int16 extra;

	native int8 ceilingshade;
	native uint8 ceilingpal;
	native int8 floorshade;
	native uint8 floorpal;
	native uint8 visibility;
	native uint8 fogpal; // EDuke32 extension - was originally a filler byte

	// new additions not from the binary map format.
	native uint8 exflags;


/*
	// Game specific extensions. Only export what's really needed.
	union
	{
		struct // DukeRR
		{
			uint8_t keyinfo; // This was originally the repurposed filler byte.
			uint8_t shadedsector;
			TObjPtr<DCoreActor*> hitagactor;    // we need this because Duke stores an actor in the hitag field. Is really a DDukeActor, but cannot be declared here safely.
		}
		struct // Blood
		{
			BLD_NS::XSECTOR* _xs;
			TObjPtr<DCoreActor*> upperLink;
			TObjPtr<DCoreActor*> lowerLink;
			int baseFloor;
			int baseCeil;
			int velFloor;
			int velCeil;
			uint8_t slopewallofs; // This was originally the repurposed filler byte.
		}
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
		}
		struct // SW
		{
			// No need to allocate this on demand as it is smaller than what Blood needs.
			int flags;
			int depth_fixed;
			short stag;    // ST? tag number - for certain things it helps to know it
			short ang;
			short height;
			short speed;
			short damage;
			short number;  // usually used for matching number
			bool u_defined;
			uint8_t flags2;
		}

	}
*/

	native double floorz();
	native double ceilingz();
	native void setceilingz(double cc, bool temp = false);
	native void setfloorz(double cc, bool temp = false);
	native void addceilingz(double cc, bool temp = false);
	native void addfloorz(double cc, bool temp = false);

	native void setfloorxpan(double val);
	native void setfloorypan(double val);
	native void setceilingxpan(double val);
	native void setceilingypan(double val);
	native void addfloorxpan(double add);
	native void addfloorypan(double add);
	native void addceilingxpan(double add);
	native void addceilingypan(double add);
	native void setceilingslope(int heinum);
	native void setfloorslope(int heinum);
	native int ceilingslope();
	native int floorslope();
}

//=============================================================================
//
// internal wall struct - no longer identical with on-disk format
//
//=============================================================================

struct walltype native
{
	native readonly Vector2 pos;

	native readonly int point2;
	native readonly int nextwall;
	native readonly int sector;	// Build never had this...
	native readonly int nextsector;

	// Again, panning fields extended for interpolation.
	native readonly float xpan;
	native readonly float ypan;

	native int16 cstat;
	
	// no access to pics!
	//int16 picnum;
	//int16 overpicnum;
	native int16 lotag;
	native int16 type; // type is an alias of lotag for Blood.
	native int16 hitag;
	native int16 extra;

	native int8 shade;
	native uint8 pal;
	native uint8 xrepeat;
	native uint8 yrepeat;


	native void setxpan(double add);
	native void setypan(double add);
	native void addxpan(double add);
	native void addypan(double add);
	native sectortype nextSectorp() const;
	native sectortype sectorp() const;
	native walltype nextWallp() const;
	native walltype lastWall() const;
	native walltype point2Wall() const;
	/*
	Vector2 delta() const;
	Vector2 center() const;
	*/
	native double deltax() const;
	native double deltay() const;
	native bool twoSided() const;

	native double Length();
	native void move(Vector2 vec);
}

//=============================================================================
//
// internal sprite struct - no longer identical with on-disk format
//
//=============================================================================

struct tspritetype native
{
	native sectortype sector;
	native int16 cstat;
	//native int16 picnum;
	native int16 statnum;
	native int16 ang;
	/* these are not needed for tsprites
	native int16 xvel;
	native int16 yvel;
	native int16 zvel;
	native int16 lotag;
	native int16 hitag;
	native int16 extra;
	native int16 detail;
	*/

	native int8 shade;
	native uint8 pal;
	native uint8 clipdist;
	native uint8 blend;
	native uint8 xrepeat;
	native uint8 yrepeat;
	native int8 xoffset;
	native int8 yoffset;
	native CoreActor ownerActor;
	native int time;
	
	//void setPic(string texture);
	native Vector3 pos();
	native void setPos(Vector3 pos);
	native void addPos(Vector3 pos);
}

enum ESprextFlags
{
	SPREXT_NOTMD = 1,
	SPREXT_NOMDANIM = 2,
	SPREXT_AWAY1 = 4,
	SPREXT_AWAY2 = 8,
	SPREXT_TSPRACCESS = 16,
	SPREXT_TEMPINVISIBLE = 32,
}

