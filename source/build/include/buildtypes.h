#ifndef buildtypes_h__
#define buildtypes_h__

#undef WALLTYPE
#undef SECTORTYPE
#undef SPRITETYPE

#ifdef UNTRACKED_STRUCTS

#define StructTracker(tracker, type) type
#define StructName(name) u ## name

#else

#define StructTracker Tracker
#define StructName(name) name

#endif

//ceilingstat/floorstat:
//   bit 0: 1 = parallaxing, 0 = not                                 "P"
//   bit 1: 1 = groudraw, 0 = not
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
//   bits 12-15: reserved

//////////////////// Version 7 map format ////////////////////

//40 bytes
typedef struct
{
    StructTracker(Sector, int16_t) wallptr, wallnum;
    StructTracker(Sector, int32_t) ceilingz, floorz;
    StructTracker(Sector, uint16_t) ceilingstat, floorstat;
    StructTracker(Sector, int16_t) ceilingpicnum, ceilingheinum;
    StructTracker(Sector, int8_t) ceilingshade;
    StructTracker(Sector, uint8_t) ceilingpal, /*CM_FLOORZ:*/ ceilingxpanning, ceilingypanning;
    StructTracker(Sector, int16_t) floorpicnum, floorheinum;
    StructTracker(Sector, int8_t) floorshade;
    StructTracker(Sector, uint8_t) floorpal, floorxpanning, floorypanning;
    StructTracker(Sector, uint8_t) /*CM_CEILINGZ:*/ visibility, fogpal;
    StructTracker(Sector, uint16_t) lotag, hitag;
    StructTracker(Sector, int16_t) extra;
} StructName(sectortypev7);

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
typedef struct
{
    StructTracker(Wall, int32_t) x, y;
    StructTracker(Wall, int16_t) point2, nextwall, nextsector;
    StructTracker(Wall, uint16_t) cstat;
    StructTracker(Wall, int16_t) picnum, overpicnum;
    StructTracker(Wall, int8_t) shade;
    StructTracker(Wall, uint8_t) pal, xrepeat, yrepeat, xpanning, ypanning;
    StructTracker(Wall, uint16_t) lotag, hitag;
    StructTracker(Wall, int16_t) extra;
} StructName(walltypev7);

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

//44 bytes
typedef struct
{
    StructTracker(Sprite, int32_t) x, y, z;
    StructTracker(Sprite, uint16_t) cstat;
    StructTracker(Sprite, int16_t) picnum;
    StructTracker(Sprite, int8_t) shade;
    StructTracker(Sprite, uint8_t) pal, clipdist, blend;
    StructTracker(Sprite, uint8_t) xrepeat, yrepeat;
    StructTracker(Sprite, int8_t) xoffset, yoffset;
    StructTracker(Sprite, int16_t) sectnum, statnum;
    StructTracker(Sprite, int16_t) ang, owner, xvel, yvel, zvel;
    StructTracker(Sprite, uint16_t) lotag, hitag;
    StructTracker(Sprite, int16_t) extra;
} StructName(spritetype);

//////////////////// END Version 7 map format ////////////////

//////////////////// Lunatic new-generation map format ////////////////////

// 44 bytes
typedef struct
{
    StructTracker(Sector, int16_t) wallptr, wallnum;

    StructTracker(Sector, int16_t) ceilingpicnum, ceilingheinum, ceilingbunch;
    StructTracker(Sector, uint16_t) ceilingstat;
    StructTracker(Sector, int32_t) ceilingz;
    StructTracker(Sector, int8_t) ceilingshade;
    StructTracker(Sector, uint8_t) ceilingpal, /*CM_FLOORZ:*/ ceilingxpanning, ceilingypanning;

    StructTracker(Sector, int16_t) floorpicnum, floorheinum, floorbunch;
    StructTracker(Sector, uint16_t) floorstat;
    StructTracker(Sector, int32_t) floorz;
    StructTracker(Sector, int8_t) floorshade;
    StructTracker(Sector, uint8_t) floorpal, floorxpanning, floorypanning;

    StructTracker(Sector, uint8_t) /*CM_CEILINGZ:*/ visibility, fogpal;
    StructTracker(Sector, uint16_t) lotag, hitag;
    StructTracker(Sector, int16_t) extra;
} StructName(sectortypevx);

// 38 bytes
typedef struct
{
    StructTracker(Wall, int32_t) x, y;
    StructTracker(Wall, int16_t) point2, nextwall, nextsector;
    StructTracker(Wall, int16_t) upwall, dnwall;
    StructTracker(Wall, uint16_t) cstat;
    StructTracker(Wall, int16_t) picnum, overpicnum;
    StructTracker(Wall, int8_t) shade;
    StructTracker(Wall, uint8_t) pal, xrepeat, yrepeat, xpanning, ypanning;
    StructTracker(Wall, uint16_t) lotag, hitag;
    StructTracker(Wall, int16_t) extra;
    StructTracker(Wall, uint8_t) blend, filler_;
} StructName(walltypevx);

// NOTE: spritetype is currently the same for V7/8/9 and VX in-memory map formats.

//////////////////// END Lunatic new-generation map format ////////////////

#undef StructTracker
#undef StructName

#endif // buildtypes_h__
