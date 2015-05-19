//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#ifdef MAKE_STAG_ENUM
#define STAG_ENTRY(st1_name, number, flgs)  st1_name = number,
#endif

#ifdef MAKE_STAG_TABLE
#define STAG_ENTRY(st1_name, number, flgs)                      \
    strcpy(StagInfo[number].name, #st1_name);                       \
    StagInfo[number].flags = flgs;
#endif

#define STAGF_MATCH_TAG2 (BIT(31))

STAG_ENTRY(SECT_SINK,                   0,      0)
STAG_ENTRY(SECT_OPERATIONAL,            1,      0)
STAG_ENTRY(SECT_CURRENT,                3,      0)
STAG_ENTRY(SECT_NO_RIDE,                5,      0)
STAG_ENTRY(SECT_DIVE_AREA,              7,      STAGF_MATCH_TAG2)
STAG_ENTRY(SECT_UNDERWATER,             8,      STAGF_MATCH_TAG2)
STAG_ENTRY(SECT_UNDERWATER2,            9,      STAGF_MATCH_TAG2)

STAG_ENTRY(SO_ANGLE,                    16,     0)
STAG_ENTRY(SECT_FLOOR_PAN,              19,     0)

STAG_ENTRY(SECT_CEILING_PAN,            21,     0)

STAG_ENTRY(SECT_WALL_PAN_SPEED,         23,     0)
STAG_ENTRY(SECT_WALL_PAN_ANG,           24,     0)

STAG_ENTRY(SECT_DEBRIS_SEWER,           25,     0)

STAG_ENTRY(SECT_SO_CENTER,              27,     0)

STAG_ENTRY(SECT_MATCH,                  28,     STAGF_MATCH_TAG2)
STAG_ENTRY(SECT_LOCK_DOOR,              29,     0)
STAG_ENTRY(SPRI_CLIMB_MARKER,           30,     0)
STAG_ENTRY(SECT_SO_SPRITE_OBJ,          31,     0)
STAG_ENTRY(SECT_SO_DONT_BOB,            32,     0)

STAG_ENTRY(SECT_SO_SINK_DEST,           33,     0)
STAG_ENTRY(SECT_SO_DONT_SINK,           34,     0)

STAG_ENTRY(SECT_SO_FORM_WHIRLPOOL,      37,     0)
STAG_ENTRY(SECT_ACTOR_BLOCK,            38,     0)
STAG_ENTRY(SECT_SO_CLIP_DIST,           39,     0)

STAG_ENTRY(MULTI_PLAYER_START,          42,     0)

STAG_ENTRY(FIREBALL_TRAP,               43,     STAGF_MATCH_TAG2)
STAG_ENTRY(BOLT_TRAP,                   44,     STAGF_MATCH_TAG2)
STAG_ENTRY(SECT_SO_DONT_ROTATE,         45,     0)

STAG_ENTRY(PARALLAX_LEVEL,              46,     0)
STAG_ENTRY(SECT_DONT_COPY_PALETTE,      47,     0)

STAG_ENTRY(MULTI_COOPERATIVE_START,     48,     0)

STAG_ENTRY(SO_SET_SPEED,                49,     0)
STAG_ENTRY(SO_SPIN,                     50,     0)
STAG_ENTRY(SO_SPIN_REVERSE,             51,     0)
STAG_ENTRY(SO_BOB_START,                52,     0)
STAG_ENTRY(SO_BOB_SPEED,                53,     0)

STAG_ENTRY(SO_TURN_SPEED,               55,     0)

STAG_ENTRY(LAVA_ERUPT,                  56,     0)

STAG_ENTRY(SECT_EXPLODING_CEIL_FLOOR,   57,     STAGF_MATCH_TAG2)
STAG_ENTRY(SECT_COPY_DEST,              58,     STAGF_MATCH_TAG2)
STAG_ENTRY(SECT_COPY_SOURCE,            59,     STAGF_MATCH_TAG2)

STAG_ENTRY(SO_SHOOT_POINT,              62,     0)
STAG_ENTRY(SPEAR_TRAP,                  63,     STAGF_MATCH_TAG2)

STAG_ENTRY(SO_SYNC1,                    64,     0)
STAG_ENTRY(SO_SYNC2,                    65,     0)

STAG_ENTRY(DEMO_CAMERA,                 66,     0)
STAG_ENTRY(SO_LIMIT_TURN,               68,     0)

STAG_ENTRY(SPAWN_SPOT,                  69,     STAGF_MATCH_TAG2)
STAG_ENTRY(SO_MATCH_EVENT,              70,     STAGF_MATCH_TAG2)

STAG_ENTRY(SO_SLOPE_FLOOR_TO_POINT,     71,     0)
STAG_ENTRY(SO_SLOPE_CEILING_TO_POINT,   72,     0)

STAG_ENTRY(SO_TORNADO,                  73,     0)
STAG_ENTRY(SO_FLOOR_MORPH,              74,     0)
STAG_ENTRY(SO_AMOEBA,                   75,     0)

STAG_ENTRY(SO_MAX_DAMAGE,               76,     0)
STAG_ENTRY(SO_RAM_DAMAGE,               77,     0)
STAG_ENTRY(SO_CLIP_BOX,                 78,     0)

STAG_ENTRY(SO_SLIDE,                    79,     0)
STAG_ENTRY(SO_KILLABLE,                 80,     0)
STAG_ENTRY(SO_AUTO_TURRET,              81,     0)
STAG_ENTRY(SECT_DAMAGE,                 82,     0)

STAG_ENTRY(WARP_TELEPORTER,             84,     STAGF_MATCH_TAG2)
STAG_ENTRY(WARP_CEILING_PLANE,          85,     STAGF_MATCH_TAG2)
STAG_ENTRY(WARP_FLOOR_PLANE,            86,     STAGF_MATCH_TAG2)
STAG_ENTRY(WARP_COPY_SPRITE1,           87,     STAGF_MATCH_TAG2)
STAG_ENTRY(WARP_COPY_SPRITE2,           88,     STAGF_MATCH_TAG2)

STAG_ENTRY(PLAX_GLOB_Z_ADJUST,          90,     0)
STAG_ENTRY(PLAX_Z_ADJUST,               91,     0)

STAG_ENTRY(SECT_VATOR,                  92,     STAGF_MATCH_TAG2)
STAG_ENTRY(SECT_VATOR_DEST,             94,     0)

STAG_ENTRY(CEILING_Z_ADJUST,            97,     0)
STAG_ENTRY(FLOOR_Z_ADJUST,              98,     0)
STAG_ENTRY(FLOOR_SLOPE_DONT_DRAW,       99,     0)

STAG_ENTRY(SO_SCALE_INFO,               100,    0)
STAG_ENTRY(SO_SCALE_POINT_INFO,         101,    0)
STAG_ENTRY(SO_SCALE_XY_MULT,            102,    0)
STAG_ENTRY(SECT_WALL_MOVE,              103,    STAGF_MATCH_TAG2)
STAG_ENTRY(SECT_WALL_MOVE_CANSEE,       104,    STAGF_MATCH_TAG2)

STAG_ENTRY(SECT_SPIKE,                  106,    STAGF_MATCH_TAG2)
STAG_ENTRY(LIGHTING,                    108,    STAGF_MATCH_TAG2)
STAG_ENTRY(LIGHTING_DIFFUSE,            109,    STAGF_MATCH_TAG2)

STAG_ENTRY(VIEW_THRU_CEILING,           120,    0)
STAG_ENTRY(VIEW_THRU_FLOOR,             121,    0)

STAG_ENTRY(VIEW_LEVEL1,                 110,    0)
STAG_ENTRY(VIEW_LEVEL2,                 111,    0)
STAG_ENTRY(VIEW_LEVEL3,                 112,    0)
STAG_ENTRY(VIEW_LEVEL4,                 113,    0)
STAG_ENTRY(VIEW_LEVEL5,                 114,    0)
STAG_ENTRY(VIEW_LEVEL6,                 115,    0)

STAG_ENTRY(SO_WALL_DONT_MOVE_UPPER,     130,    STAGF_MATCH_TAG2)
STAG_ENTRY(SO_WALL_DONT_MOVE_LOWER,     131,    STAGF_MATCH_TAG2)
STAG_ENTRY(BREAKABLE,                   132,    STAGF_MATCH_TAG2)
STAG_ENTRY(QUAKE_SPOT,                  133,    STAGF_MATCH_TAG2)
STAG_ENTRY(SOUND_SPOT,                  134,    STAGF_MATCH_TAG2)
STAG_ENTRY(SLIDE_SECTOR,                135,    0)
STAG_ENTRY(CEILING_FLOOR_PIC_OVERRIDE,  136,    0)
STAG_ENTRY(TRIGGER_SECTOR,              140,    STAGF_MATCH_TAG2)
STAG_ENTRY(DELETE_SPRITE,               141,    STAGF_MATCH_TAG2)
STAG_ENTRY(SECT_ROTATOR,                143,    STAGF_MATCH_TAG2)
STAG_ENTRY(SECT_ROTATOR_PIVOT,          144,    STAGF_MATCH_TAG2)
STAG_ENTRY(SECT_SLIDOR,                 145,    STAGF_MATCH_TAG2)
STAG_ENTRY(SECT_CHANGOR,                146,    STAGF_MATCH_TAG2)
STAG_ENTRY(SO_DRIVABLE_ATTRIB,          147,    0)
STAG_ENTRY(WALL_DONT_STICK,             148,    0)
STAG_ENTRY(SPAWN_ITEMS,                 149,    0)
STAG_ENTRY(STOP_SOUND_SPOT,             150,    0)

STAG_ENTRY(BOUND_FLOOR_UPPER,           200,    0)
STAG_ENTRY(BOUND_FLOOR_LOWER,           201,    0)
STAG_ENTRY(BOUND_FLOOR_BASE_OFFSET,     202,    0)
STAG_ENTRY(BOUND_FLOOR_OFFSET,          203,    0)

STAG_ENTRY(BOUND_SO_UPPER0,             500,    0)
STAG_ENTRY(BOUND_SO_LOWER0,             501,    0)
STAG_ENTRY(BOUND_SO_UPPER1,             505,    0)
STAG_ENTRY(BOUND_SO_LOWER1,             506,    0)
STAG_ENTRY(BOUND_SO_UPPER2,             510,    0)
STAG_ENTRY(BOUND_SO_LOWER2,             511,    0)
STAG_ENTRY(BOUND_SO_UPPER3,             515,    0)
STAG_ENTRY(BOUND_SO_LOWER3,             516,    0)
STAG_ENTRY(BOUND_SO_UPPER4,             520,    0)
STAG_ENTRY(BOUND_SO_LOWER4,             521,    0)
STAG_ENTRY(BOUND_SO_UPPER5,             525,    0)
STAG_ENTRY(BOUND_SO_LOWER5,             526,    0)
STAG_ENTRY(BOUND_SO_UPPER6,             530,    0)
STAG_ENTRY(BOUND_SO_LOWER6,             531,    0)
STAG_ENTRY(BOUND_SO_UPPER7,             535,    0)
STAG_ENTRY(BOUND_SO_LOWER7,             536,    0)
STAG_ENTRY(BOUND_SO_UPPER8,             540,    0)
STAG_ENTRY(BOUND_SO_LOWER8,             541,    0)
STAG_ENTRY(BOUND_SO_UPPER9,             545,    0)
STAG_ENTRY(BOUND_SO_LOWER9,             546,    0)
STAG_ENTRY(BOUND_SO_UPPER10,            550,    0)
STAG_ENTRY(BOUND_SO_LOWER10,            551,    0)
STAG_ENTRY(BOUND_SO_UPPER11,            555,    0)
STAG_ENTRY(BOUND_SO_LOWER11,            556,    0)
STAG_ENTRY(BOUND_SO_UPPER12,            560,    0)
STAG_ENTRY(BOUND_SO_LOWER12,            561,    0)
STAG_ENTRY(BOUND_SO_UPPER13,            565,    0)
STAG_ENTRY(BOUND_SO_LOWER13,            566,    0)
STAG_ENTRY(BOUND_SO_UPPER14,            570,    0)
STAG_ENTRY(BOUND_SO_LOWER14,            571,    0)
STAG_ENTRY(BOUND_SO_UPPER15,            575,    0)
STAG_ENTRY(BOUND_SO_LOWER15,            576,    0)
STAG_ENTRY(BOUND_SO_UPPER16,            580,    0)
STAG_ENTRY(BOUND_SO_LOWER16,            581,    0)
STAG_ENTRY(BOUND_SO_UPPER17,            585,    0)
STAG_ENTRY(BOUND_SO_LOWER17,            586,    0)
STAG_ENTRY(BOUND_SO_UPPER18,            590,    0)
STAG_ENTRY(BOUND_SO_LOWER18,            591,    0)
STAG_ENTRY(BOUND_SO_UPPER19,            595,    0)
STAG_ENTRY(BOUND_SO_LOWER19,            596,    0)
STAG_ENTRY(TV_CAMERA,                  1000,    0)
STAG_ENTRY(AMBIENT,                    1002,    0)
STAG_ENTRY(ECHOSPOT,                   1005,    0)
STAG_ENTRY(DRIP_GENERATOR,             1006,    0)

#undef STAG_ENTRY
