//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#ifndef __actors_h__
#define __actors_h__

#include "player.h"

#define MAXSLEEPDIST        16384
#define SLEEPTIME           1536
#define ZOFFSET             (1<<8)

#define STAT_DEFAULT        0
#define STAT_ACTOR          1
#define STAT_ZOMBIEACTOR    2
#define STAT_EFFECTOR       3
#define STAT_PROJECTILE     4
#define STAT_MISC           5
#define STAT_STANDABLE      6
#define STAT_LOCATOR        7
#define STAT_ACTIVATOR      8
#define STAT_TRANSPORT      9
#define STAT_PLAYER         10
#define STAT_FX             11
#define STAT_FALLER         12
#define STAT_DUMMYPLAYER    13
#define STAT_LIGHT          14


// Defines the motion characteristics of an actor
enum amoveflags_t {
    face_player         = 1,
    geth                = 2,
    getv                = 4,
    random_angle        = 8,
    face_player_slow    = 16,
    spin                = 32,
    face_player_smart   = 64,
    fleeenemy           = 128,
    jumptoplayer        = 257,
    seekplayer          = 512,
    furthestdir         = 1024,
    dodgebullet         = 4096
};

// Defines for 'useractor' keyword
enum uactortypes_t {
    notenemy,
    enemy,
    enemystayput
};

#pragma pack(push,1)

#ifdef LUNATIC
struct action {
    int16_t startframe, numframes;
    int16_t viewtype, incval, delay;
};

struct move {
    int16_t hvel, vvel;
};
#endif

typedef struct {
    int32_t workslike, cstat; // 8b
    int32_t hitradius, range, flashcolor; // 12b
    int16_t spawns, sound, isound, vel; // 8b
    int16_t decal, trail, tnum, drop; // 8b
    int16_t offset, bounces, bsound; // 6b
    int16_t toffset; // 2b
    int16_t extra, extra_rand; // 4b
    int8_t sxrepeat, syrepeat, txrepeat, tyrepeat; // 4b
    int8_t shade, xrepeat, yrepeat, pal; // 4b
    int8_t movecnt; // 1b
    uint8_t clipdist; // 1b
    int8_t filler[6]; // 6b
} projectile_t;

// (+ 40 8 6 16 16 4 8 6 4 20)
typedef struct {
    int32_t t_data[10];  // 40b sometimes used to hold offsets to con code

#ifdef LUNATIC
    // TODO: rearrange for better packing when enabling Lunatic
    struct move mv;
    struct action ac;
    const int16_t padding_;
#endif

    int16_t picnum,ang,extra,owner; //8b
    int16_t movflag,tempang,timetosleep; //6b

    int32_t flags, bposx,bposy,bposz; //16b
    int32_t floorz,ceilingz,lastvx,lastvy; //16b
    int32_t lasttransport; //4b

    int16_t actorstayput, dispicnum, shootzvel, cgg; // 8b
    int16_t lightId, lightcount, lightmaxrange; //6b

#ifdef POLYMER
    _prlight *lightptr; //4b/8b
#else
    void *lightptr;
#endif

// pad struct to 128 bytes
#if !defined UINTPTR_MAX
# error Need UINTPTR_MAX define to select between 32- and 64-bit structs
#endif
#if UINTPTR_MAX == 0xffffffff
    /* 32-bit */
# ifndef LUNATIC
    const int8_t filler[20];
# else
    const int8_t filler[4];
# endif
#else
    /* 64-bit */
# ifndef LUNATIC
    const int8_t filler[16];
# else
    /* no padding */
#endif
#endif
} actor_t;

// this struct needs to match the beginning of actor_t above
typedef struct {
    int32_t t_data[10];  // 40b sometimes used to hold offsets to con code

#ifdef LUNATIC
    struct move mv;
    struct action ac;
    const int16_t padding_;
#endif

    int16_t picnum,ang,extra,owner; //8b
    int16_t movflag,tempang,timetosleep; // 6b

    int32_t flags, bposx,bposy,bposz; //16b
    int32_t floorz,ceilingz,lastvx,lastvy; //16b
    int32_t lasttransport; //4b

    int16_t actorstayput, dispicnum, shootzvel, cgg; // 8b

} netactor_t;

typedef struct {
    intptr_t *execPtr; // pointer to CON script for this tile, formerly actorscrptr
    intptr_t *loadPtr; // pointer to load time CON script, formerly actorLoadEventScrPtr or something

    uint32_t flags;    // formerly SpriteFlags, ActorType

    int16_t cacherange[2]; // formerly SpriteCache

    // todo: make these pointers and allocate at runtime
    projectile_t proj;
    projectile_t defproj;
} tiledata_t;

#pragma pack(pop)

enum sflags_t {
    SPRITE_SHADOW           = 0x00000001,
    SPRITE_NVG              = 0x00000002,
    SPRITE_NOSHADE          = 0x00000004,
    SPRITE_PROJECTILE       = 0x00000008,
    SPRITE_DECAL            = 0x00000010,
    SPRITE_BADGUY           = 0x00000020,
    SPRITE_NOPAL            = 0x00000040,
    SPRITE_NOEVENTCODE      = 0x00000080,
    SPRITE_NOLIGHT          = 0x00000100,
    SPRITE_USEACTIVATOR     = 0x00000200,
    SPRITE_NULL             = 0x00000400, // null sprite in multiplayer
    SPRITE_NOCLIP           = 0x00000800, // clipmove it with cliptype 0
    SPRITE_NOFLOORSHADOW    = 0x00001000, // for temp. internal use, per-tile flag not checked
    SPRITE_SMOOTHMOVE       = 0x00002000,
    SPRITE_NOTELEPORT       = 0x00004000,
    SPRITE_BADGUYSTAYPUT    = 0x00008000,
    SPRITE_CACHE            = 0x00010000,
};

// custom projectiles
enum pflags_t {
    PROJECTILE_HITSCAN             = 0x00000001,
    PROJECTILE_RPG                 = 0x00000002,
    PROJECTILE_BOUNCESOFFWALLS     = 0x00000004,
    PROJECTILE_BOUNCESOFFMIRRORS   = 0x00000008,
    PROJECTILE_KNEE                = 0x00000010,
    PROJECTILE_WATERBUBBLES        = 0x00000020,
    PROJECTILE_TIMED               = 0x00000040,
    PROJECTILE_BOUNCESOFFSPRITES   = 0x00000080,
    PROJECTILE_SPIT                = 0x00000100,
    PROJECTILE_COOLEXPLOSION1      = 0x00000200,
    PROJECTILE_BLOOD               = 0x00000400,
    PROJECTILE_LOSESVELOCITY       = 0x00000800,
    PROJECTILE_NOAIM               = 0x00001000,
    PROJECTILE_RANDDECALSIZE       = 0x00002000,
    PROJECTILE_EXPLODEONTIMER      = 0x00004000,
    PROJECTILE_RPG_IMPACT          = 0x00008000,
    PROJECTILE_RADIUS_PICNUM       = 0x00010000,
    PROJECTILE_ACCURATE_AUTOAIM    = 0x00020000,
    PROJECTILE_FORCEIMPACT         = 0x00040000,
    PROJECTILE_REALCLIPDIST        = 0x00080000,
    PROJECTILE_ACCURATE            = 0x00100000,
};

extern tiledata_t       g_tile[MAXTILES];
extern actor_t          actor[MAXSPRITES];
extern int32_t          block_deletesprite;
extern int32_t          g_noEnemies;
extern int32_t          otherp;
extern int32_t          ticrandomseed;
extern intptr_t         *g_parsingActorPtr;
extern projectile_t     SpriteProjectile[MAXSPRITES];


#ifdef LUNATIC
// Legacy action/move setters from the CON script + t_data pointer:
extern intptr_t *script;

// tptr[4] expected to be set
static inline void set_action_members(int32_t i)
{
    actor_t *const aptr = &actor[i];
    int32_t acofs = aptr->t_data[4];

    aptr->ac.startframe = script[acofs];
    aptr->ac.numframes = script[acofs+1];
    aptr->ac.viewtype = script[acofs+2];
    aptr->ac.incval = script[acofs+3];
    aptr->ac.delay = script[acofs+4];
}

// tptr[1] expected to be set
static inline void set_move_members(int32_t i)
{
    actor_t *const aptr = &actor[i];
    int32_t mvofs = aptr->t_data[1];

    aptr->mv.hvel = script[mvofs];
    aptr->mv.vvel = script[mvofs+1];
}
#endif

void                A_AddToDeleteQueue(int32_t i);
int32_t             A_CheckEnemySprite(const spritetype *s);
int32_t             A_CheckEnemyTile(int32_t pn);
int32_t             A_CheckSwitchTile(int32_t i);
void                A_DeleteSprite(int32_t s);
void                A_DoGuts(int32_t sp,int32_t gtype,int32_t n);
void                A_DoGutsDir(int32_t sp,int32_t gtype,int32_t n);
int32_t             A_IncurDamage(int32_t sn);
void                A_MoveCyclers(void);
void                A_MoveDummyPlayers(void);
int32_t             A_MoveSprite(int32_t spritenum,const vec3_t *change,uint32_t cliptype);
void                A_PlayAlertSound(int32_t i);
void                A_RadiusDamage(int32_t i,int32_t r,int32_t hp1,int32_t hp2,int32_t hp3,int32_t hp4);
int32_t             A_SetSprite(int32_t i,uint32_t cliptype);
void                A_SpawnMultiple(int32_t sp,int32_t pic,int32_t n);

void                G_AddGameLight(int32_t radius,int32_t srcsprite,int32_t zoffset,int32_t range,int32_t color,int32_t priority);
int32_t             G_CheckForSpaceCeiling(int32_t sectnum);
int32_t             G_CheckForSpaceFloor(int32_t sectnum);
void                G_ClearCameraView(DukePlayer_t *ps);
void                G_DoInterpolations(int32_t smoothratio);
void                G_MoveWorld(void);
extern inline void  G_RestoreInterpolations(void);
void                G_SetInterpolation(int32_t *posptr);
void                G_StopInterpolation(int32_t *posptr);
extern inline void  G_UpdateInterpolations(void);

// PK 20110701: changed input argument: int32_t i (== sprite, whose sectnum...) --> sectnum directly
void                Sect_ClearInterpolation(int32_t sectnum);
void                Sect_SetInterpolation(int32_t sectnum);

#endif
