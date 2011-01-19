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
    int8_t velmult; // 1b
    uint8_t clipdist; // 1b
    int8_t filler[6]; // 6b
} projectile_t;

typedef struct {
    intptr_t t_data[10]; // 40b/80b sometimes used to hold pointers to con code

    int16_t picnum,ang,extra,owner; //8b
    int16_t movflag,tempang,timetosleep; //6b

    int32_t flags, bposx,bposy,bposz; //16b
    int32_t floorz,ceilingz,lastvx,lastvy; //16b
    int32_t lasttransport; //4b

    int16_t lightId, lightcount, lightmaxrange, cgg; //8b
    int16_t actorstayput, dispicnum, shootzvel; // 6b

#ifdef POLYMER
    _prlight *lightptr; //4b/8b
#else
    void *lightptr;
#endif

    projectile_t *projectile; //4b/8b

    int8_t filler[16]; // pad struct to 128 bytes
} actor_t;

// this struct needs to match the beginning of actor_t above
typedef struct {
    intptr_t t_data[10]; // 40b/80b sometimes used to hold pointers to con code

    int16_t picnum,ang,extra,owner; //8b
    int16_t movflag,tempang,timetosleep; // 6b

    int32_t flags; // 4b
} netactor_t;
#pragma pack(pop)

enum sflags_t {
    SPRITE_SHADOW       = 0x00000001,
    SPRITE_NVG          = 0x00000002,
    SPRITE_NOSHADE      = 0x00000004,
    SPRITE_PROJECTILE   = 0x00000008,
    SPRITE_DECAL        = 0x00000010,
    SPRITE_BADGUY       = 0x00000020,
    SPRITE_NOPAL        = 0x00000040,
    SPRITE_NOEVENTCODE  = 0x00000080,
    SPRITE_NOLIGHT      = 0x00000100,
    SPRITE_USEACTIVATOR = 0x00000200,
    SPRITE_NULL         = 0x00000400, // null sprite in multiplayer
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

extern actor_t          actor[MAXSPRITES];
extern char             ActorType[MAXTILES];
extern int16_t          SpriteCacheList[MAXTILES][3];
extern int32_t          SpriteFlags[MAXTILES];
extern int32_t          block_deletesprite;
extern int32_t          g_noEnemies;
extern int32_t          otherp;
extern int32_t          ticrandomseed;
extern intptr_t         *actorLoadEventScrptr[MAXTILES];
extern intptr_t         *actorscrptr[MAXTILES];
extern intptr_t         *g_parsingActorPtr;
extern projectile_t     DefaultProjectileData[MAXTILES];
extern projectile_t     ProjectileData[MAXTILES];
extern projectile_t     SpriteProjectile[MAXSPRITES];

void                A_AddToDeleteQueue(int32_t i);
int32_t             A_CheckEnemySprite(spritetype *s);
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
void                G_DoInterpolations(int32_t smoothratio);
void                G_MoveWorld(void);
extern inline void  G_RestoreInterpolations(void);
void                G_SetInterpolation(int32_t *posptr);
void                G_StopInterpolation(int32_t *posptr);
extern inline void  G_UpdateInterpolations(void);

void                Sect_ClearInterpolation(int32_t i);
void                Sect_SetInterpolation(int32_t i);

#endif
