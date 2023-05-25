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

#ifndef NAMES2_H
#define NAMES2_H

#include "names.h"

BEGIN_SW_NS

// Rotation info

//      4
//    5   3
//  6       2
//    7   1
//      0


enum STAT_ENUMS
{
    STAT_DEFAULT=               0,
    STAT_MISC=                  1,

    STAT_SKIP2_START =          2,
    STAT_ENEMY=                 2,
    STAT_DEAD_ACTOR=            3, //misc actor stuff - dead guys etc
    STAT_MISSILE=               4,
    STAT_SHRAP=                 5,
    STAT_SKIP2_END =            5,
    STAT_SKIP2_INTERP_END =     5,

    STAT_SKIP4_START =          6,
    STAT_SKIP4=                 6,
    STAT_MISSILE_SKIP4=         7,
    STAT_MINE_STUCK=                  8, //solely for mines
    STAT_ENEMY_SKIP4=           9,
    STAT_SKIP4_INTERP_END =     9,

    STAT_ITEM=                  10,
    STAT_SKIP4_END =            10,

    STAT_FAF_COPY,
    STAT_MAX=20,  // everything below here can move
    STAT_PLAYER0,
    STAT_PLAYER1,
    STAT_PLAYER2,
    STAT_PLAYER3,

    STAT_PLAYER4,
    STAT_PLAYER5,
    STAT_PLAYER6,
    STAT_PLAYER7,
    STAT_PLAYER_UNDER0,

    STAT_PLAYER_UNDER1,
    STAT_PLAYER_UNDER2,
    STAT_PLAYER_UNDER3,
    STAT_PLAYER_UNDER4,
    STAT_PLAYER_UNDER5,

    STAT_PLAYER_UNDER6,
    STAT_PLAYER_UNDER7,
    STAT_GENERIC_QUEUE,
    STAT_OBJECT_SPRITE,   // sprites that move with objects
    STAT_WALLBLOOD_QUEUE,
    STAT_FLOORBLOOD_QUEUE,
    STAT_NO_STATE,        // sprites that don't need state control
    STAT_STATIC_FIRE,
    STAT_STAR_QUEUE,
    STAT_HOLE_QUEUE,
    STAT_BREAKABLE,
    STAT_SPRITE_HIT_MATCH, // TAG_SPRITE_HIT_MATCH
//
// Everything from here down does not get drawn
//
    STAT_DONT_DRAW,
    STAT_SUICIDE,
    STAT_DIVE_AREA,
    STAT_UNDERWATER,
    STAT_UNDERWATER2,

    STAT_CEILING_FLOOR_PIC_OVERRIDE,
    STAT_CLIMB_MARKER,
    STAT_ALL,
    STAT_SO_SHOOT_POINT,  // operational SO shooting point
    STAT_SPAWN_TRIGGER,   // triggers spawn sprites
    STAT_TRAP,            // triggered traps - spear/fireball etc
    STAT_TRIGGER,         // attempt to replace tagging sectors - use ST1 sprites
    STAT_DEMO_CAMERA,     // demo camera placement
    STAT_FAF,             // floor above floor stat
    STAT_SO_SP_CHILD,
    STAT_WARP,
    STAT_WARP_COPY_SPRITE1,

    STAT_WARP_COPY_SPRITE2,
    STAT_SOUND_SPOT,
    STAT_STOP_SOUND_SPOT,
    STAT_SPAWN_SPOT,
    STAT_AMBIENT,
    STAT_ECHO,
    STAT_VATOR,
    STAT_ROTATOR,
    STAT_ROTATOR_PIVOT,
    STAT_SLIDOR,
    STAT_FLOOR_SLOPE_DONT_DRAW,

    STAT_EXPLODING_CEIL_FLOOR,
    STAT_COPY_SOURCE,
    STAT_COPY_DEST,
    STAT_WALL_MOVE,
    STAT_WALL_MOVE_CANSEE,

    STAT_SPAWN_ITEMS,
    STAT_DELETE_SPRITE,
    STAT_SPIKE,
    STAT_LIGHTING,
    STAT_LIGHTING_DIFFUSE,
    STAT_WALL_DONT_MOVE_UPPER,
    STAT_WALL_DONT_MOVE_LOWER,
    STAT_FLOOR_PAN,
    STAT_CEILING_PAN,
    STAT_WALL_PAN,
    STAT_NO_RIDE,
    STAT_QUAKE_SPOT,
    STAT_QUAKE_ON,
    STAT_VIS_ON,
    STAT_CHANGOR,

    STAT_TRACK=200,
    STAT_ST1=500,
    STAT_QUICK_JUMP,
    STAT_QUICK_JUMP_DOWN,
    STAT_QUICK_SUPER_JUMP,
    STAT_QUICK_SCAN,
    STAT_QUICK_EXIT,
    STAT_QUICK_LADDER,
    STAT_QUICK_OPERATE,
    STAT_QUICK_DUCK,
    STAT_QUICK_DEFEND,

    STAT_MULTI_START    = 600,
    STAT_MULTI_START1   = 601,
    STAT_MULTI_START2   = 602,
    STAT_MULTI_START3   = 603,
    STAT_MULTI_START4   = 604,
    STAT_MULTI_START5   = 605,
    STAT_MULTI_START6   = 606,
    STAT_MULTI_START7   = 607,

    STAT_CO_OP_START    = 610,
    STAT_CO_OP_START1   = 611,
    STAT_CO_OP_START2   = 612,
    STAT_CO_OP_START3   = 613,
    STAT_CO_OP_START4   = 614,
    STAT_CO_OP_START5   = 615,
    STAT_CO_OP_START6   = 616,
    STAT_CO_OP_START7   = 617,

};

//////////////////////
//
// TIMERS
//
//////////////////////
enum ETimers
{
    TICS_PER_SEC = 120,

    FLY_INVENTORY_TIME = 30,
    CLOAK_INVENTORY_TIME = 30,
    ENVIRON_SUIT_INVENTORY_TIME = 30,

    DAMAGE_TIME = (1 * TICS_PER_SEC),
};

// multi player start

//******************************************************************************

enum EPicnums
{
    // enemy/item spawn IDs are still needed
    COOLIE_RUN_R0 = 1400,
    COOLG_RUN_R0 = 4277,
    BUNNY_RUN_R0 = 4550,
    EEL_RUN_R0 = 3780,
    GIRLNINJA_RUN_R0 = 5162,
    GORO_RUN_R0 = 1469,
    HORNET_RUN_R0 = 800,
    LAVA_RUN_R0 = 2355,
    NINJA_RUN_R0 = 4096,
    NINJA_DEAD = 4227,
    NINJA_CRAWL_R0 = 4162,
    PLAYER_NINJA_RUN_R0 = 1094,
    ZOMBIE_RUN_R0 = PLAYER_NINJA_RUN_R0 + 1,
    NINJA_Head_R0 = 1142,
    RIPPER_RUN_R0 = 1580,
    RIPPER2_RUN_R0 = 4320,
    SERP_RUN_R0 = 1300,
    SKEL_RUN_R0 = 1367, 
    ZILLA_RUN_R0 = 5426,
    SUMO_RUN_R0 = 1210,

    TOILETGIRL_R0 = 5023,
    WASHGIRL_R0 = 5032,
    MECHANICGIRL_R0 = 4590,
    CARGIRL_R0 = 4594,
    SAILORGIRL_R0 = 4600,
    PRUNEGIRL_R0 = 4604,
    Red_COIN = 2440,
    Yellow_COIN = 2450,
    Green_COIN = 2460,


    EXP = 3100, // Use new digitized explosion for big stuff
    RADIATION_CLOUD = 3258,
    MUSHROOM_CLOUD = 3280,
    CHEMBOMB_R0 = 3038,
    CALTROPSR = CALTROPS - 1,
    TRASHCAN = 2540,
    PACHINKO1 = 4768,
    PACHINKO2 = 4792,
    PACHINKO3 = 4816,
    PACHINKO4 = 4840,
    PACHINKOLIGHT_R0 = 618,
    SKULL_R0 = 820,
    SKULL_SERP = (SKULL_R0 + 2),
    BETTY_R0 = 817,
    PUFF = 1748,



    

    SWITCH_OFF = 561,

    TRACK_SPRITE = 1900, //start of track sprites
    ST1 = 2307,
    ST2 = 2308,
    ST_QUICK_JUMP = 2309,
    ST_QUICK_JUMP_DOWN = 2310,
    ST_QUICK_SUPER_JUMP = 2311,
    ST_QUICK_SCAN = 2312,
    ST_QUICK_EXIT = 2313,
    ST_QUICK_OPERATE = 2314,
    ST_QUICK_DUCK = 2315,
    ST_QUICK_DEFEND = 2316,

//////////////////////
//
// WEAPON RELATED
//
//////////////////////

    CROSSBOLT = 2230,

    STAR = 2039,
    ELECTRO = 2025,

    UZI_SMOKE = 2146,
    UZI_SPARK = 2140,
    SPIKES = 2092,
    GRENADE = 2019,
    BLANK = 2200,


    BODY = 1002,
    BODY_BURN = 1003,
    BODY_SIZZLE = 1011,


    DART_R0 = 2130,
    DART_R1 = 2131,
    DART_R2 = 2132,
    DART_R3 = 2133,
    DART_R4 = 2134,
    DART_R5 = 2135,
    DART_R6 = 2136,
    DART_R7 = 2137,

    BOLT_THINMAN_R0 = 2018,
    BOLT_THINMAN_R1 = 2019,
    BOLT_THINMAN_R2 = 2020,
    BOLT_THINMAN_R3 = 2021,
    BOLT_THINMAN_R4 = 2022,

    SPEAR_R0 = 2030,
    EMP = 2058,

    EXP2 = 2160, // My old explosion is still used for goro head
    FIREBALL = 2035,
    FIREBALL_FLAMES = 3212,
    SPLASH = 772,
    BUBBLE = 716,

    //////////////////////
    //
    // MISC
    //
    //////////////////////

     WATER_BEGIN = 320,
     WATER_END = 320+8,

     WATER_BOIL = 2305,

     FIRE_FLY0 = 630,
     FIRE_FLY1 = 631,
     FIRE_FLY2 = 632,
     FIRE_FLY3 = 633,
     FIRE_FLY4 = 634,

     FIRE_FLY_RATE = 50,

     BREAK_BARREL = 453,
     BREAK_PEDISTAL = 463,
     BREAK_BOTTLE1 = 468,
     BREAK_BOTTLE2 = 475,

     LAVA_BOULDER = 2196,

 };


//////////////////////
//
// FIREBALL
//
//////////////////////

#define FIREBALL_FRAMES 4
#define FIREBALL_R0 3192
#define FIREBALL_R1 FIREBALL_R0 + (FIREBALL_FRAMES * 1)
#define FIREBALL_R2 FIREBALL_R0 + (FIREBALL_FRAMES * 2)
#define FIREBALL_R3 FIREBALL_R0 + (FIREBALL_FRAMES * 3)
#define FIREBALL_R4 FIREBALL_R0 + (FIREBALL_FRAMES * 4)


////////////////////////////////////

#define SCROLL 516
#define SCROLL_FIRE 524

#define FLOORBLOOD1 389

END_SW_NS

#endif
