#ifndef buildtypes_h__
#define buildtypes_h__

//ceilingstat/floorstat:
//   bit 0: 1 = parallaxing, 0 = not                                 "P"
//   bit 1: 1 = sloped, 0 = not
//   bit 2: 1 = swap x&y, 0 = not                                    "F"
//   bit 3: 1 = double smooshiness                                   "E"
//   bit 4: 1 = x-flip                                               "F"
//   bit 5: 1 = y-flip                                               "F"
//   bit 6: 1 = Align texture to first wall of sector                "R"
//   bits 8-7:                                                       "T"
//          00 = normal floors
//          01 = masked floors
//          10 = transluscent masked floors
//          11 = reverse transluscent masked floors
//   bit 9: 1 = blocking ceiling/floor
//   bit 10: 1 = YAX'ed ceiling/floor
//   bit 11: 1 = hitscan-sensitive ceiling/floor
//   bits 12-14: reserved
//   bit 15: SW: block FAF hitscans

//////////////////// Version 7 map format ////////////////////
enum
{
    CSTAT_SECTOR_SKY = 1,
    CSTAT_SECTOR_SLOPE = 2,
    CSTAT_SECTOR_SWAPXY = 4,
    CSTAT_SECTOR_TEXHALF = 8,
    CSTAT_SECTOR_XFLIP = 16,
    CSTAT_SECTOR_YFLIP = 32,
    CSTAT_SECTOR_ALIGN = 64,
    CSTAT_SECTOR_TRANS = 128,
    CSTAT_SECTOR_TRANS_INVERT = 256,
    CSTAT_SECTOR_METHOD = 384,

    SECTOREX_CLOUDSCROLL = 1,
};

enum
{
    PORTAL_SECTOR_FLOOR = 1,
    PORTAL_SECTOR_CEILING = 2,
    PORTAL_SECTOR_FLOOR_REFLECT = 3,
    PORTAL_SECTOR_CEILING_REFLECT = 4,
    PORTAL_WALL_VIEW = 5,
    PORTAL_WALL_MIRROR = 6,
    PORTAL_WALL_TO_SPRITE = 7,
    PORTAL_SECTOR_GEOMETRY = 8,
};


//40 bytes
struct walltype;
struct sectortype
{
    int16_t wallptr, wallnum;
    int32_t ceilingz, floorz;
    uint16_t ceilingstat, floorstat;
    int16_t ceilingpicnum, ceilingheinum;
    int8_t ceilingshade;
    uint8_t ceilingpal; /*CM_FLOORZ:*/
    int16_t floorpicnum, floorheinum;
    int8_t floorshade;
    uint8_t floorpal;;
    uint8_t /*CM_CEILINGZ:*/ visibility, fogpal;
    union {
        int16_t lotag, type;
    };
    int16_t hitag;
    int16_t extra;

    uint8_t dirty;
    uint8_t exflags;
    float ceilingxpan_, ceilingypan_, floorxpan_, floorypan_;
    uint8_t portalflags;
    int8_t portalnum;

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
    walltype *firstWall() const;
};

//cstat:
//   bit 0: 1 = Blocking wall (use with clipmove, getzrange)         "B"
//   bit 1: 1 = bottoms of invisible walls swapped, 0 = not          "2"
//   bit 2: 1 = align picture on bottom (for doors), 0 = top         "O"
//   bit 3: 1 = x-flipped, 0 = normal                                "F"
//   bit 4: 1 = masking wall, 0 = not                                "M"
//   bit 5: 1 = 1-way wall, 0 = not                                  "1"
//   bit 6: 1 = Blocking wall (use with hitscan / cliptype 1)        "H"
//   bit 7: 1 = Transluscence, 0 = not                               "T"
//   bit 8: 1 = y-flipped, 0 = normal                                "F"
//   bit 9: 1 = Transluscence reversing, 0 = normal                  "T"
//   bits 10 and 11: reserved (in use by YAX)
//   bits 12-15: reserved  (14: temp use by editor)

//32 bytes
struct walltype
{
    union {
        struct
        {
            int32_t x, y;
        };
        vec2_t pos;
    };
    int16_t point2, nextwall, sector, nextsector;
    uint16_t cstat;
    int16_t picnum, overpicnum;
    int8_t shade;
    uint8_t pal, xrepeat, yrepeat;
    union {
        int16_t lotag, type;
    };
    int16_t hitag;
    int16_t extra;
    float xpan_, ypan_;
    binangle clipangle;
    uint8_t portalflags;
    uint16_t portalnum;

    int xpan() const { return int(xpan_); }
    int ypan() const { return int(ypan_); }
    void setxpan(float add) { xpan_ = fmodf(add + 512, 256); } // +512 is for handling negative offsets
    void setypan(float add) { ypan_ = fmodf(add + 512, 256); } // +512 is for handling negative offsets
    void addxpan(float add) { xpan_ = fmodf(xpan_ + add + 512, 256); } // +512 is for handling negative offsets
    void addypan(float add) { ypan_ = fmodf(ypan_ + add + 512, 256); } // +512 is for handling negative offsets
    sectortype* nextSector() const;
    walltype* nextWall() const;
    walltype* point2Wall() const;
	vec2_t delta() const { return point2Wall()->pos - pos; }
	vec2_t center() const { return(point2Wall()->pos + pos) / 2; }
	int deltax() const { return point2Wall()->x - x; }
	int deltay() const { return point2Wall()->y - y; }
    bool twoSided() const { return nextsector >= 0; }

#if 0
    // make sure we do not accidentally copy this
    walltype() = default;
    walltype(const walltypev7&) = delete;
    walltype& operator=(const walltypev7&) = delete;
#endif

};

//cstat:
//   bit 0: 1 = Blocking sprite (use with clipmove, getzrange)       "B"
//   bit 1: 1 = transluscence, 0 = normal                            "T"
//   bit 2: 1 = x-flipped, 0 = normal                                "F"
//   bit 3: 1 = y-flipped, 0 = normal                                "F"
//   bits 5-4: 00 = FACE sprite (default)                            "R"
//             01 = WALL sprite (like masked walls)
//             10 = FLOOR sprite (parallel to ceilings&floors)
//   bit 6: 1 = 1-sided sprite, 0 = normal                           "1"
//   bit 7: 1 = Real centered centering, 0 = foot center             "C"
//   bit 8: 1 = Blocking sprite (use with hitscan / cliptype 1)      "H"
//   bit 9: 1 = Transluscence reversing, 0 = normal                  "T"
//   bit 10: reserved (in use by a renderer hack, see CSTAT_SPRITE_MDHACK)
//   bit 11: 1 = determine shade based only on its own shade member (see CON's spritenoshade command), i.e.
//               don't take over shade from parallaxed ceiling/nonparallaxed floor
//               (NOTE: implemented on the game side)
//   bit 12: reserved
//   bit 13: 1 = does not cast shadow
//   bit 14: 1 = invisible but casts shadow
//   bit 15: 1 = Invisible sprite, 0 = not invisible
#ifndef buildtypes_h__enums
enum
{
    CSTAT_SPRITE_BLOCK = 1u,
    CSTAT_SPRITE_TRANSLUCENT = 1u<<1u,
    CSTAT_SPRITE_XFLIP = 1u<<2u,
    CSTAT_SPRITE_YFLIP = 1u<<3u,
    CSTAT_SPRITE_ALIGNMENT = 1u<<4u | 1u<<5u, // (cstat & CSTAT_SPRITE_ALIGNMENT) == CSTAT_SPRITE_ALIGNMENT_xxxxxx can be used to check sprite alignment
    CSTAT_SPRITE_ONE_SIDED = 1u<<6u,
    CSTAT_SPRITE_YCENTER = 1u<<7u,
    CSTAT_SPRITE_BLOCK_HITSCAN = 1u<<8u,
    CSTAT_SPRITE_BLOCK_ALL = CSTAT_SPRITE_BLOCK_HITSCAN | CSTAT_SPRITE_BLOCK,
    CSTAT_SPRITE_TRANSLUCENT_INVERT = 1u<<9u,

    CSTAT_SPRITE_RESERVED1 = 1u<<10u, // used by Duke 3D (Polymost)
    CSTAT_SPRITE_RESERVED2 = 1u<<11u, // used by Duke 3D (EDuke32 game code extension)
    CSTAT_SPRITE_RESERVED3 = 1u<<12u, // used by Shadow Warrior, Blood
    CSTAT_SPRITE_RESERVED4 = 1u<<13u, // used by Duke 3D (Polymer), Shadow Warrior, Blood
    CSTAT_SPRITE_RESERVED5 = 1u<<14u, // used by Duke 3D (Polymer), Shadow Warrior, Blood

    CSTAT_SPRITE_INVISIBLE = 1u<<15u,
};

enum
{
    CSTAT2_SPRITE_MDLROTATE = 1,   // Only for tsprites: rotate if this is a model or voxel.
    CSTAT2_SPRITE_NOFIND = 2,      // Invisible to neartag and hitscan
    CSTAT2_SPRITE_MAPPED = 4,      // sprite was mapped for automap

};
enum
{
    CSTAT_SPRITE_ALIGNMENT_FACING = 0,
    CSTAT_SPRITE_ALIGNMENT_WALL   = 1u<<4u,
    CSTAT_SPRITE_ALIGNMENT_FLOOR  = 1u<<5u,
    CSTAT_SPRITE_ALIGNMENT_SLAB   = 1u<<4u | 1u<<5u,
    CSTAT_SPRITE_ALIGNMENT_SLOPE  = 1u << 4u | 1u << 5u,

    CSTAT_SPRITE_ALIGNMENT_MASK   = 1u<<4u | 1u<<5u,
};

enum
{
    CSTAT_WALL_BLOCK         = 1u,
    CSTAT_WALL_BOTTOM_SWAP   = 1u<<1u,
    CSTAT_WALL_ALIGN_BOTTOM  = 1u<<2u,
    CSTAT_WALL_XFLIP         = 1u<<3u,
    CSTAT_WALL_MASKED        = 1u<<4u,
    CSTAT_WALL_1WAY          = 1u<<5u,
    CSTAT_WALL_BLOCK_HITSCAN = 1u<<6u,
    CSTAT_WALL_TRANSLUCENT   = 1u<<7u,
    CSTAT_WALL_YFLIP         = 1u<<8u,
    CSTAT_WALL_TRANS_FLIP    = 1u<<9u,

    CSTAT_WALL_YAX_UPWALL    = 1u<<10u, // EDuke32 extension
    CSTAT_WALL_YAX_DOWNWALL  = 1u<<11u, // EDuke32 extension
    CSTAT_WALL_ROTATE_90     = 1u<<12u, // EDuke32 extension

    CSTAT_WALL_RESERVED1     = 1u<<13u,
    CSTAT_WALL_RESERVED2     = 1u<<14u, // used by Shadow Warrior, Blood
    CSTAT_WALL_RESERVED3     = 1u<<15u, // used by Shadow Warrior, Blood

};
#endif

#ifndef buildtypes_h__enums


struct spritetype
{
    union {
        struct
        {
            int32_t x, y, z;
        };
        vec3_t pos;
    };
    union {
        struct
        {
            int32_t ox, oy, oz;
        };
        vec3_t opos;
    };
    uint16_t cstat;
    int16_t picnum;
    int8_t shade;
    uint8_t pal, clipdist, blend;
    uint8_t xrepeat, yrepeat;
    int8_t xoffset, yoffset;
    int16_t sectnum, statnum;
    int16_t oang, ang, owner;
        union {
            int16_t xvel, index;
        };
        int16_t yvel;
        union {
            int16_t zvel, inittype;
        };
    union {
        int16_t lotag, type;
    };
    union {
        int16_t hitag, flags;
    };
    int16_t extra;
    int16_t detail;
    int time;
    int16_t wall;
    uint16_t cstat2;
    int8_t wdist;


#if 0
    // make sure we do not accidentally copy this
    spritetype() = default;
    spritetype(const spritetype&) = delete;
    spritetype& operator=(const spritetype&) = delete;
#endif
    void clear()
    {
        int save = time;    // this may not be cleared ever!!!
        memset(this, 0, sizeof(*this));
        time = save;
    }

    void backupx()
    {
        opos.x = pos.x;
    }

    void backupy()
    {
        opos.y = pos.y;
    }

    void backupz()
    {
        opos.z = pos.z;
    }

    void backupvec2()
    {
        opos.vec2 = pos.vec2;
    }

    void backuppos()
    {
        opos = pos;
    }

    void backupang()
    {
        oang = ang;
    }

    void backuploc()
    {
        backuppos();
        backupang();
    }

    int32_t interpolatedx(double const smoothratio, int const scale = 16)
    {
        return interpolatedvalue(ox, x, smoothratio, scale);
    }

    int32_t interpolatedy(double const smoothratio, int const scale = 16)
    {
        return interpolatedvalue(oy, y, smoothratio, scale);
    }

    int32_t interpolatedz(double const smoothratio, int const scale = 16)
    {
        return interpolatedvalue(oz, z, smoothratio, scale);
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
        return interpolatedangle(oang, ang, smoothratio, 16);
    }

    sectortype* sector() const;
    bool insector() const;
};

using tspritetype = spritetype;
#endif

//////////////////// END Version 7 map format ////////////////

#ifndef buildtypes_h__enums
#define buildtypes_h__enums
#endif

#endif // buildtypes_h__
