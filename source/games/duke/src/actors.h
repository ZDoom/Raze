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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#ifndef actors_h_
#define actors_h_

#include "player.h"
# include "namesdyn.h"

BEGIN_DUKE_NS

#define MAXSLEEPDIST        16384
#define SLEEPTIME           1536
#define ZOFFSET             (1<<8)
#define ZOFFSET2            (16<<8)
#define ZOFFSET3            (8<<8)
#define ZOFFSET4            (12<<8)
#define ZOFFSET5            (32<<8)
#define ZOFFSET6            (4<<8)
#define FOURSLEIGHT ZOFFSET

#define ACTOR_MAXFALLINGZVEL 6144
#define ACTOR_ONWATER_ADDZ (24<<8)

// KEEPINSYNC lunatic/con_lang.lua
enum
{
	STAT_DEFAULT        = 0,
	STAT_ACTOR          = 1,
	STAT_ZOMBIEACTOR    = 2,
	STAT_EFFECTOR       = 3,
	STAT_PROJECTILE     = 4,
	STAT_MISC           = 5,
	STAT_STANDABLE      = 6,
	STAT_LOCATOR        = 7,
	STAT_ACTIVATOR      = 8,
	STAT_TRANSPORT      = 9,
	STAT_PLAYER         = 10,
	STAT_FX             = 11,
	STAT_FALLER         = 12,
	STAT_DUMMYPLAYER    = 13,
	STAT_LIGHT          = 14,
	STAT_RAROR          = 15,
	STAT_NETALLOC       = MAXSTATUS-1
};


// Defines the motion characteristics of an actor
enum amoveflags_t
{
    face_player       = 1,
    geth              = 2,
    getv              = 4,
    random_angle      = 8,
    face_player_slow  = 16,
    spin              = 32,
    face_player_smart = 64,
    fleeenemy         = 128,
    jumptoplayer_only = 256,
    justjump1 = 256,
    jumptoplayer_bits = 257,  // NOTE: two bits set!
    jumptoplayer = 257,  // NOTE: two bits set!
    seekplayer        = 512,
    furthestdir       = 1024,
    dodgebullet       = 4096,
    justjump2         = 8192,
    windang           = 16384,
    antifaceplayerslow = 32768
};

// Defines for 'useractor' keyword
enum uactortypes_t
{
    notenemy,
    enemy,
    enemystayput
};

// These macros are there to give names to the t_data[]/T*/vm.g_t[] indices
// when used with actors. Greppability of source code is certainly a virtue.
#define AC_COUNT(t) ((t)[0])  /* the actor's count */
/* The ID of the actor's current move. In C-CON, the bytecode offset to the
 * move composite: */
#define AC_MOVE_ID(t) ((t)[1])
#define AC_ACTION_COUNT(t) ((t)[2])  /* the actor's action count */
#define AC_CURFRAME(t) ((t)[3])  /* the actor's current frame offset */
/* The ID of the actor's current action. In C-CON, the bytecode offset to the
 * action composite: */
#define AC_ACTION_ID(t) ((t)[4])
#define AC_AI_ID(t) ((t)[5])  /* the ID of the actor's current ai */

enum actionparams
{
    ACTION_STARTFRAME = 0,
    ACTION_NUMFRAMES,
    ACTION_VIEWTYPE,
    ACTION_INCVAL,
    ACTION_DELAY,
    ACTION_FLAGS,
    ACTION_PARAM_COUNT,
};

enum actionflags
{
    AF_VIEWPOINT = 1u<<0u,
};

// Select an actor's actiontics and movflags locations depending on
// whether we compile the Lunatic build.
// <spr>: sprite pointer
// <a>: actor_t pointer
# define AC_ACTIONTICS(spr, a) ((spr)->lotag)
# define AC_MOVFLAGS(spr, a) ((spr)->hitag)

// (+ 40 16 16 4 8 6 8 6 4 20)
#pragma pack(push, 1)
typedef struct
{
    int32_t t_data[10];  // 40b sometimes used to hold offsets to con code

    int32_t aflags;                             // 4b
    union
    {
        vec3_t  bpos;                              // 12b
        struct { int bposx, bposy, bposz; };
    };
    int32_t floorz, ceilingz;                  // 8b
    union
    {
        vec2_t lastv;                              // 8b
        struct { int lastvx, lastvy; };
    };
    int16_t picnum, ang, extra, owner;         // 8b
    int16_t movflag, tempang, timetosleep;     // 6b
    int16_t actorstayput;                      // 2b

    uint8_t cgg, lasttransport;                // 2b
    // NOTE: 'dispicnum' is updated every frame, not in sync with game tics!
    int16_t dispicnum;                         // 2b

#ifdef POLYMER
    int16_t lightId, lightmaxrange;  // 4b
    _prlight *lightptr;              // 4b/8b  aligned on 96 bytes
    uint8_t lightcount, filler[3];
#endif
} actor_t;
#define temp_data t_data

// this struct needs to match the beginning of actor_t above
typedef struct
{
    int32_t t_data[10];  // 40b sometimes used to hold offsets to con code

    int32_t flags;                             // 4b
    vec3_t  bpos;                              // 12b
    int32_t floorz, ceilingz;                  // 8b
    vec2_t lastv;                              // 8b
    int16_t picnum, ang, extra, owner;         // 8b
    int16_t movflag, tempang, timetosleep;     // 6b
    int16_t actorstayput;

    uint8_t cgg, lasttransport;

    spritetype sprite;
    int16_t    netIndex;
} netactor_t;
#pragma pack(pop)

// Todo - put more state in here
struct ActorInfo
{
    uint32_t scriptaddress;
    uint32_t flags;
    int aimoffset;
};


// KEEPINSYNC lunatic/con_lang.lua
enum sflags_t
{
    SFLAG_SHADOW        = 0x00000001,
    SFLAG_NVG           = 0x00000002,
    SFLAG_INVENTORY     = 0x00000004,
    SFLAG_ADULT         = 0x00000008,
    SFLAG_SHRINKAUTOAIM     = 0x00000010,
    SFLAG_BADGUY        = 0x00000020,
    SFLAG_NOPAL         = 0x00000040,
    SFLAG_FORCEAUTOAIM  = 0x00000080,
    SFLAG_USEACTIVATOR  = 0x00000200,
    SFLAG_NOFLOORSHADOW = 0x00001000,  // for temp. internal use, per-tile flag not checked
    SFLAG_BADGUYSTAYPUT = 0x00008000,
    SFLAG_GREENSLIMEFOOD   = 0x00800000,
    SFLAG_NODAMAGEPUSH     = 0x00100000,
    SFLAG_NOWATERDIP       = 0x00200000,
    SFLAG_INTERNAL_BADGUY  = 0x08000000, // a separate flag is needed for the internal ones because SFLAG_BADGUY has additional semantics.
    SFLAG_KILLCOUNT        = 0x10000000,
    SFLAG_NOCANSEECHECK    = 0x20000000,

    //SFLAG_NOSHADE       = 0x00000004,
    //SFLAG_PROJECTILE    = 0x00000008,
    //SFLAG_DECAL         = 0x00000010,
    //SFLAG_NOEVENTCODE   = 0x00000080,
    //SFLAG_NOLIGHT       = 0x00000100,
    //SFLAG_NULL          = 0x00000400,  // null sprite in multiplayer
    //SFLAG_NOCLIP        = 0x00000800,  // clipmove it with cliptype 0
    //SFLAG_SMOOTHMOVE    = 0x00002000,
    //SFLAG_NOTELEPORT    = 0x00004000,
    //SFLAG_HURTSPAWNBLOOD   = 0x00400000,
    //SFLAG_REALCLIPDIST     = 0x01000000,
    //SFLAG_WAKEUPBADGUYS    = 0x02000000,
};


extern ActorInfo   actorinfo[MAXTILES];
extern actor_t      actor[MAXSPRITES];
extern actor_t* hittype;
extern int32_t      g_noEnemies;
#define actor_tog g_noEnemies
extern int32_t      otherp;
extern int32_t      ticrandomseed;
extern int g_canSeePlayer;

int A_FindLocator(int const tag, int const sectNum);
inline int LocateTheLocator(int const tag, int const sectNum)
{
    return A_FindLocator(tag, sectNum);
}

int A_IncurDamage(int spriteNum);
void A_DeleteSprite(int spriteNum);

void movecyclers(void);
void movedummyplayers(void);
void resetlanepics(void);

int  G_SetInterpolation(int32_t *posptr);
void G_ClearCameraView(DukePlayer_t *ps);
void clearcamera(player_struct* ps);
void G_DoInterpolations(int smoothRatio);
void G_MoveWorld(void);
void G_RefreshLights(void);
void G_StopInterpolation(const int32_t *posptr);

// PK 20110701: changed input argument: int32_t i (== sprite, whose sectnum...) --> sectnum directly
// Note: The entire interpolation system needs to be a lot smarter than what's backing these functions.
void                Sect_ToggleInterpolation(int sectnum, int setInterpolation);
static FORCE_INLINE void   Sect_ClearInterpolation(int sectnum) { Sect_ToggleInterpolation(sectnum, 0); }
static FORCE_INLINE void   Sect_SetInterpolation(int sectnum) { Sect_ToggleInterpolation(sectnum, 1); }
static FORCE_INLINE void   clearsectinterpolate(int sectnum) { Sect_ToggleInterpolation(sectnum, 0); }
static FORCE_INLINE void   setsectinterpolate(int sectnum) { Sect_ToggleInterpolation(sectnum, 1); }

#if KRANDDEBUG
# define ACTOR_INLINE __fastcall
# define ACTOR_INLINE_HEADER extern __fastcall
#else
# define ACTOR_INLINE EXTERN_INLINE
# define ACTOR_INLINE_HEADER EXTERN_INLINE_HEADER
#endif

ACTOR_INLINE_HEADER int A_CheckEnemyTile(int tileNum);
ACTOR_INLINE_HEADER int A_SetSprite(int spriteNum, uint32_t cliptype);

EXTERN_INLINE_HEADER int A_CheckEnemySprite(void const * s);

#if defined actors_c_ || !defined DISABLE_INLINING

# if !KRANDDEBUG || (KRANDDEBUG && defined actors_c_)

ACTOR_INLINE int A_CheckEnemyTile(int const tileNum)
{
    return ((actorinfo[tileNum].flags & (SFLAG_INTERNAL_BADGUY | SFLAG_BADGUY)) != 0);
}

int ssp(short i, unsigned int cliptype); //The set sprite function
void insertspriteq(int i);

ACTOR_INLINE int A_SetSprite(int const spriteNum, uint32_t cliptype)
{
    return ssp(spriteNum, cliptype);
}

# endif


EXTERN_INLINE int A_CheckEnemySprite(void const * const pSprite)
{
    return A_CheckEnemyTile(((uspritetype const *) pSprite)->picnum);
}

inline int badguy(void const* const pSprite)
{
    return A_CheckEnemySprite(pSprite);
}

inline int badguypic(int tile)
{
    return A_CheckEnemyTile(tile);
}
int G_WakeUp(spritetype* const pSprite, int const playerNum);
inline int wakeup(int sn, int pn)
{
    return G_WakeUp(&sprite[sn], pn);
}

// shared functions
void ms(short i);
void movecrane(int i, int crane);
void movefountain(int i, int fountain);
void moveflammable(int i, int tire, int box, int pool);
void detonate(int i, int explosion);
void movemasterswitch(int i, int spectype1, int spectype2);
void movetrash(int i);
void movewaterdrip(int i, int drip);
void movedoorshock(int i);
void movetouchplate(int i, int plate);
void movecanwithsomething(int i);
void bounce(int i);
void movetongue(int i, int tongue, int jaw);
void moveooz(int i, int seenine, int seeninedead, int ooz, int explosion);
void lotsofstuff(spritetype* s, short n, int spawntype);
bool respawnmarker(int i, int yellow, int green);
bool rat(int i, bool makesound);
bool queball(int i, int pocket, int queball, int stripeball);
void forcesphere(int i, int forcesphere);
void recon(int i, int explosion, int firelaser, int attacksnd, int painsnd, int roamsnd, int shift, int (*getspawn)(int i));
void ooz(int i);
void reactor(int i, int REACTOR, int REACTOR2, int REACTORBURNT, int REACTOR2BURNT);
void camera(int i);
void forcesphere(int i);
void watersplash2(int i);
void frameeffect1(int i);
bool money(int i, int BLOODPOOL);
bool jibs(int i, int JIBS6, bool timeout, bool callsetsprite, bool floorcheck, bool zcheck1, bool zcheck2);
bool bloodpool(int i, bool puke, int TIRE);
void shell(int i, bool morecheck);
void glasspieces(int i);
void scrap(int i, int SCRAP1, int SCRAP6);

void handle_se00(int i, int LASERLINE);
void handle_se01(int i);
void handle_se14(int i, bool checkstat, int RPG, int JIBS6);
void handle_se30(int i, int JIBS6);
void handle_se02(int i);
void handle_se03(int i);
void handle_se04(int i);
void handle_se05(int i, int FIRELASER);
void handle_se08(int i, bool checkhitag1);
void handle_se10(int i, const int *);
void handle_se11(int i);
void handle_se12(int i, int planeonly = 0);
void handle_se13(int i);
void handle_se15(int i);
void handle_se16(int i, int REACTOR, int REACTOR2);
void handle_se17(int i);
void handle_se18(int i, bool morecheck);
void handle_se19(int i, int BIGFORCE);
void handle_se20(int i);
void handle_se21(int i);
void handle_se22(int i);
void handle_se26(int i);
void handle_se27(int i);
void handle_se32(int i);
void handle_se35(int i, int SMALLSMOKE, int EXPLOSION2);
void handle_se128(int i);
void handle_se130(int i, int countmax, int EXPLOSION2);

void respawn_rrra(int i, int j);

int dodge(spritetype*);
void alterang(int a, int g_i, int g_p);
void fall_common(int g_i, int g_p, int JIBS6, int DRONE, int BLOODPOOL, int SHOTSPARK1, int squished, int thud, int(*fallspecial)(int, int), void (*falladjustz)(spritetype*));
void checkavailweapon(struct player_struct* p);

// tile names which are identical for all games.
enum
{
	SECTOREFFECTOR = 1,
	ACTIVATOR = 2,
	TOUCHPLATE = 3,
	ACTIVATORLOCKED = 4,
	MUSICANDSFX = 5,
	LOCATORS = 6,
	CYCLER = 7,
	MASTERSWITCH = 8,
	RESPAWN = 9,
	GPSPEED = 10,
	FOF = 13,
};	

#endif

END_DUKE_NS

#endif
