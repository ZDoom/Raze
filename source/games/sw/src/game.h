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

#ifndef GAME_H

#define GAME_H

#ifdef _MSC_VER
#pragma warning(disable:4101) // there's too many of these... :(
#endif

#include "build.h"
#include "d_net.h"
#include "gamefuncs.h"
#include "coreactor.h"

#include "sounds.h"
#include "gamecvars.h"
#include "raze_sound.h"
#include "c_cvars.h"
#include "mapinfo.h"
#include "gamecontrol.h"
#include "gamestruct.h"
#include "packet.h"
#include "gameinput.h"
#include "serialize_obj.h"
#include "texturemanager.h"

EXTERN_CVAR(Bool, sw_ninjahack)
EXTERN_CVAR(Bool, sw_darts)
EXTERN_CVAR(Bool, sw_bunnyrockets)

BEGIN_SW_NS

class DSWActor;
using HitInfo = THitInfo<DSWActor>;
using Collision = TCollision<DSWActor>;

constexpr int BIT(int shift)
{
    return 1 << shift;
}

struct GAME_SET
{
    // Net Options from Menus
    uint8_t NetGameType;   // 0=DeathMatch [spawn], 1=Cooperative 2=DeathMatch [no spawn]
    uint8_t NetMonsters;   // Cycle skill levels
    bool NetHurtTeammate;  // Allow friendly kills
    bool NetSpawnMarkers;    // Respawn markers on/off
    bool NetTeamPlay;   // Team play
    uint8_t NetKillLimit;  // Number of frags at which game ends
    uint8_t NetTimeLimit;  // Limit time of game
    uint8_t NetColor;      // Chosen color for player
    bool NetNuke;
};

extern const GAME_SET gs_defaults;
extern GAME_SET gs;

enum
{
    DREALMSPAL = 1,

    MAXMIRRORS          = 8,
    // This is just some, high, blank tile number not used
    // by real graphics to put the MAXMIRRORS mirrors in
    MIRRORLABEL         = 6000,

};

//#define SW_SHAREWARE 1     // This determines whether game is shareware compile or not!
#define SW_SHAREWARE (!!(g_gameType & GAMEFLAG_SHAREWARE))

// Turn warning off for unreferenced variables.
// I really should fix them at some point
//#pragma off(unreferenced)


#define PRODUCTION_ASSERT(f) \
    assert(f);\
    do { \
        if (!(f)) \
            I_FatalError("Assertion failed: %s %s, line %u", #f, __FILE__, __LINE__); \
    } while (0)

#define ASSERT assert

int RandomRange(int);
inline int RANDOM(void)
{
    randomseed = ((randomseed * 21 + 1) & 65535);
    return randomseed;
}
inline double RandomRangeF(double range)
{
    return RANDOM() * range / 65536;
}
inline DAngle RandomAngle(double range = 360.)
{
	return DAngle::fromDeg(RandomRangeF(range));
}
inline DAngle RandomAngle(DAngle range)
{
	return DAngle::fromDeg(RandomRangeF(range.Degrees()));
}

int RANDOM_P2(int pwr_of_2) { return (RANDOM() & (pwr_of_2 - 1)); }
double RANDOM_P2F(int pwr_of_2, int shift) { return (RANDOM() & ((pwr_of_2 << shift) - 1)) * (1./(1 << shift)); }

//
// Map directions/degrees
//

#if 0
y--
^ 1536
|
|
|
|
|
|            2047
<---------------------------->
1024         |              0
x--          |             x++
|
|
|
|
V 512
y++

#endif

//////////////////////////////////////////////////////
//
// KEYBOARD
//
//////////////////////////////////////////////////////

extern bool MenuInputMode;

//
// Defines
//

// dist at which actors will not move (unless shot?? to do)
constexpr int MAX_ACTIVE_RANGE = 2625;

enum
{
    CIRCLE_CAMERA_DIST_MIN = 12000,
    // dist at which actors roam about on their own
    MIN_ACTIVE_RANGE = 20000,
};

constexpr double CIRCLE_CAMERA_DIST_MINF = 12000. / 65536.;

constexpr int32_t FIXED(int32_t msw, int32_t lsw)
{
    return IntToFixed(msw) | lsw;
}

// Ouch...
#ifndef WORDS_BIGENDIAN
# define MSB_VAR(fixed) (*(((uint8_t*)&(fixed)) + 1))
# define LSB_VAR(fixed) (*((uint8_t*)&(fixed)))
#else

# define LSB_VAR(fixed) (*(((uint8_t*)&(fixed)) + 1))
# define MSB_VAR(fixed) (*((uint8_t*)&(fixed)))
#endif

#define TRAVERSE_CONNECT(i)   for (i = connecthead; i != -1; i = connectpoint2[i])


constexpr int NORM_ANGLE(int ang) { return ((ang) & 2047); }

int StdRandomRange(int range);



// actual Z for TOS and BOS - handles both WYSIWYG and old style
inline double GetSpriteZOfTop(const spritetypebase* sp)
{
    auto tex = TexMan.GetGameTexture(sp->spritetexture());
    auto sizez = tex->GetDisplayHeight() * sp->scale.Y;
    return (sp->cstat & CSTAT_SPRITE_YCENTER) ?
        sp->pos.Z - ((sizez * 0.5) + tex->GetDisplayTopOffset()) :
        sp->pos.Z - sizez;
}

inline double GetSpriteZOfBottom(const spritetypebase* sp)
{
    auto tex = TexMan.GetGameTexture(sp->spritetexture());
    auto sizez = tex->GetDisplayHeight() * sp->scale.Y;
    return (sp->cstat & CSTAT_SPRITE_YCENTER) ?
        sp->pos.Z + ((sizez * 0.5) - tex->GetDisplayTopOffset()) :
        sp->pos.Z;
}

// mid and upper/lower sprite calculations
constexpr int Z(int value)
{
    return value << 8;
}

constexpr int PIXZ(int value)
{
    return value >> 8;
}


constexpr int SEC(int value)
{
    return ((value) * 120);
}


enum
{
    CEILING_DIST = 4,
    FLOOR_DIST = 4
};

// Clip Sprite adjustment
constexpr int CS(int sprite_bit)
{
    return (sprite_bit) << 16;
}

enum EClip
{
    // for players to clip against walls
    CLIPMASK_PLAYER = CS(CSTAT_SPRITE_BLOCK) | CSTAT_WALL_BLOCK,

    // for actors to clip against walls
    CLIPMASK_ACTOR = CS(CSTAT_SPRITE_BLOCK) | CSTAT_WALL_BLOCK | CSTAT_WALL_BLOCK_ACTOR,

    // for missiles to clip against actors
    CLIPMASK_MISSILE = CS(CSTAT_SPRITE_BLOCK_HITSCAN | CSTAT_SPRITE_BLOCK_MISSILE) | CSTAT_WALL_BLOCK_HITSCAN,

    CLIPMASK_WARP_HITSCAN = CS(CSTAT_SPRITE_BLOCK_HITSCAN) | CSTAT_WALL_BLOCK_HITSCAN | CSTAT_WALL_WARP_HITSCAN,
};


#define SIZ countof


//
// Directions
//

enum
{
    DEGREE_45 = 256,
    DEGREE_90 = 512
};

////
//
// Directional enumerations
//
////

enum DirOrd
{
    ORD_NORTH, ORD_NE, ORD_EAST, ORD_SE, ORD_SOUTH, ORD_SW, ORD_WEST, ORD_NW
};

enum Dir8
{
    NORTH   = ORD_NORTH * DEGREE_45,
    NE      = ORD_NE    * DEGREE_45,
    EAST    = ORD_EAST  * DEGREE_45,
    SE      = ORD_SE    * DEGREE_45,
    SOUTH   = ORD_SOUTH * DEGREE_45,
    SW      = ORD_SW    * DEGREE_45,
    WEST    = ORD_WEST  * DEGREE_45,
    NW      = ORD_NW    * DEGREE_45,
};

// Auto building enumerations

#include "digi.h"

#define DAMAGE_ENUM
enum dam
{
#include "damage.h"
};
#undef DAMAGE_ENUM

////
//
// State declarations
//
////


// Forward declarations
struct STATE;
struct PANEL_STATE;
class DSWPlayer;
struct PERSONALITY;
struct ATTRIBUTE;
struct SECTOR_OBJECT;
class DPanelSprite;
struct ANIM;
class DSWActor;

typedef int ANIMATOR (DSWActor* actor);
typedef void pANIMATOR (DPanelSprite*);
typedef void (*soANIMATORp) (SECTOR_OBJECT*);

struct STATE
{
    short     Pic;
    int       Tics;
    ANIMATOR* Animator;

    STATE*   NextState;
};

//
// State Flags
//

enum
{
    SF_TICS_MASK = 0xFFFF,
    SF_QUICK_CALL = BIT(16),
    SF_PLAYER_FUNC = BIT(17), // only for players to execute
    SF_TIC_ADJUST = BIT(18), // use tic adjustment for these frames
    SF_WALL_STATE = BIT(19), // use for walls instead of sprite
};

///////////////////////////////////////////////////////////////////////////////
// Jim's MISC declarations from other files
///////////////////////////////////////////////////////////////////////////////

enum  FOOT_TYPE
{WATER_FOOT, BLOOD_FOOT};

extern FOOT_TYPE FootMode;
ANIMATOR QueueFloorBlood;                // Weapon.c
int QueueFootPrint(DSWActor*);                 // Weapon.c
void QueueLoWangs(DSWActor*);                   // Weapon.c
int SpawnShell(DSWActor* actor, int ShellNum);     // JWeapon.c
void UnlockKeyLock(short key_num, DSWActor* actor);  // JSector.c

enum
{
    MAX_PAIN = 5,
    MAX_TAUNTAI = 33,
    MAX_GETSOUNDS = 5,
    MAX_YELLSOUNDS = 3,
};

extern int PlayerPainVocs[MAX_PAIN];
extern int PlayerLowHealthPainVocs[MAX_PAIN];
extern int TauntAIVocs[MAX_TAUNTAI];
extern int PlayerGetItemVocs[MAX_GETSOUNDS];
extern int PlayerYellVocs[MAX_YELLSOUNDS];

void BossHealthMeter(void);

// Global variables used for modifying variouse things from the Console

///////////////////////////////////////////////////////////////////////////////////////////
//
// JPlayer
//
///////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////
//
// Weapon
//
///////////////////////////////////////////////////////////////////////////////////////////

enum
{
    MAX_WEAPONS_KEYS = 10,
    MAX_WEAPONS_EXTRA = 4, // extra weapons like the two extra head attacks
    MAX_WEAPONS = (MAX_WEAPONS_KEYS + MAX_WEAPONS_EXTRA),

    // weapons that not missile type sprites
    WPN_NM_LAVA = (-8),
    WPN_NM_SECTOR_SQUISH = (-9),
};
//#define WEAP_ENTRY(id, init_func, damage_lo, damage_hi, radius)

struct DAMAGE_DATA
{
    void (*Init)(DSWPlayer*);
    int16_t damage_lo;
    int16_t damage_hi;
    unsigned int radius;
    int16_t max_ammo;
    int16_t min_ammo;
    int16_t with_weapon;
    int16_t weapon_pickup;
    int16_t ammo_pickup;
};

extern DAMAGE_DATA DamageData[];

// bit arrays that determine if a) Weapon has no ammo b) Weapon is the ammo (no weapon exists)
extern int WeaponHasNoAmmo, WeaponIsAmmo;


void InitWeaponFist(DSWPlayer*);
void InitWeaponStar(DSWPlayer*);
void InitWeaponShotgun(DSWPlayer*);
void InitWeaponRocket(DSWPlayer*);
void InitWeaponRail(DSWPlayer*);
void InitWeaponMicro(DSWPlayer*);
void InitWeaponUzi(DSWPlayer*);
void InitWeaponSword(DSWPlayer*);
void InitWeaponHothead(DSWPlayer*);
void InitWeaponElectro(DSWPlayer*);
void InitWeaponHeart(DSWPlayer*);
void InitWeaponGrenade(DSWPlayer*);
void InitWeaponMine(DSWPlayer*);

void InitWeaponNapalm(DSWPlayer*);
void InitWeaponRing(DSWPlayer*);

extern void (*InitWeapon[MAX_WEAPONS]) (DSWPlayer*);

///////////////////////////////////////////////////////////////////////////////////////////
//
// Player
//
///////////////////////////////////////////////////////////////////////////////////////////

enum
{
    MAX_SW_PLAYERS_SW = (4),
    MAX_SW_PLAYERS_REG = (8)
};
#define MAX_SW_PLAYERS (SW_SHAREWARE ? MAX_SW_PLAYERS_SW : MAX_SW_PLAYERS_REG)

extern int   ThemeTrack[6];                                          // w
extern FString ThemeSongs[6];                                          //

enum
{
    MAX_KEYS = 8,
    MAX_FORTUNES = 16,
    MAX_INVENTORY_Q = 11,//InvDecl_TOTAL

    QUOTE_KEYMSG = 1,
    QUOTE_DOORMSG = QUOTE_KEYMSG + MAX_KEYS,
    // 23+24 are reserved.
    QUOTE_COOKIE = 25,  
    QUOTE_INVENTORY = QUOTE_COOKIE + MAX_FORTUNES,
    QUOTE_WPNFIST = QUOTE_INVENTORY + MAX_INVENTORY_Q,
    QUOTE_WPNSWORD,
    QUOTE_WPNSHURIKEN,
    QUOTE_WPNSTICKY,
    QUOTE_WPNUZI,
    QUOTE_WPNLAUNCH,
    QUOTE_WPNNUKE,
    QUOTE_WPNGRENADE,
    QUOTE_WPNRAILGUN,
    QUOTE_WPNRIOT,
    QUOTE_WPNHEAD,
    QUOTE_WPNRIPPER,
    // Here a gap of two needs to be inserted because the weapon array contains two bogus entries the parser can access.
    // Not all ammo types here are used, but the entries must be reserved for the parser.
    QUOTE_AMMOFIST = QUOTE_WPNRIPPER + 2,
    QUOTE_AMMOSWORD,
    QUOTE_AMMOSHURIKEN,
    QUOTE_AMMOSTICKY,
    QUOTE_AMMOUZI,
    QUOTE_AMMOLAUNCH,
    QUOTE_AMMONUKE,
    QUOTE_AMMOGRENADE,
    QUOTE_AMMORAILGUN,
    QUOTE_AMMORIOT,
    QUOTE_AMMOHEAD,
    QUOTE_AMMORIPPER,
    // Again, here a gap of two needs to be inserted because the weapon array contains two bogus entries the parser can access.

};

enum PlayerDeathTypes
{
    PLAYER_DEATH_FLIP, PLAYER_DEATH_CRUMBLE, PLAYER_DEATH_EXPLODE, PLAYER_DEATH_RIPPER, PLAYER_DEATH_SQUISH, PLAYER_DEATH_DROWN, MAX_PLAYER_DEATHS
};

typedef void (*PLAYER_ACTION_FUNCp)(DSWPlayer*);

#include "inv.h"

struct REMOTE_CONTROL
{
    sectortype* cursectp, * lastcursectp;
    int pang;
    DVector2 vect, ovect, slide_vect;
    DVector3 pos;
    SECTOR_OBJECT* sop_control;
};


//
// Player Flags
//

enum
{
    PF_DEAD                     = (BIT(1)),
    PF_JUMPING                  = (BIT(2)),
    PF_FALLING                  = (BIT(3)),
    PF_PLAYER_MOVED             = (BIT(7)),
    PF_PLAYER_RIDING            = (BIT(8)),
    PF_RECOIL                   = (BIT(10)),
    PF_FLYING                   = (BIT(11)),
    PF_WEAPON_RETRACT           = (BIT(12)),
    PF_PICKED_UP_AN_UZI         = (BIT(13)),
    PF_CRAWLING                 = (BIT(14)),
    PF_CLIMBING                 = (BIT(15)),
    PF_SWIMMING                 = (BIT(16)),
    PF_DIVING                   = (BIT(17)),
    PF_DIVING_IN_LAVA           = (BIT(18)),
    PF_TWO_UZI                  = (BIT(19)),
    PF_DEAD_HEAD                = (BIT(22)), // are your a dead head
    PF_HEAD_CONTROL             = (BIT(23)), // have control of turning when a head?
    PF_CLIP_CHEAT               = (BIT(24)), // cheat for wall clipping
    PF_SLIDING                  = (BIT(25)), // cheat for wall clipping
    PF_VIEW_FROM_OUTSIDE        = (BIT(26)),
    PF_VIEW_OUTSIDE_WEAPON      = (BIT(27)),
    PF_VIEW_FROM_CAMERA         = (BIT(28)),
    PF_TANK                     = (BIT(29)), // Doin the tank thang
    PF_WEAPON_DOWN              = (BIT(31)),
    PF2_TELEPORTED              = (BIT(0)),
    PF2_INPUT_CAN_TURN_VEHICLE  = (BIT(3)), // Allow calling DoPlayerTurnVehicle() from processMovement()
    PF2_INPUT_CAN_TURN_TURRET   = (BIT(4)), // Allow calling DoPlayerTurnTurret() from processMovement()
};

///////////////////////////////////////////////////////////////////////////////////////////
//
// Actor
//
///////////////////////////////////////////////////////////////////////////////////////////

//
// Hit Points
//

enum
{
    HEALTH_RIPPER            = 70  ,
    HEALTH_RIPPER2           = 200 ,
    HEALTH_MOMMA_RIPPER      = 500 ,
    HEALTH_NINJA             = 40  ,
    HEALTH_RED_NINJA         = 160 ,
    HEALTH_COOLIE            = 120 ,
    HEALTH_COOLIE_GHOST      = 65  ,
    HEALTH_SKEL_PRIEST       = 90  ,
    HEALTH_GORO              = 200 ,
    HEALTH_HORNET            = 4   ,
    HEALTH_SKULL             = 4   ,
    HEALTH_EEL               = 100 ,
    HEALTH_SERP_GOD          = 3800,
};

//
// Action Set Structure
//

enum
{
    MAX_ACTOR_CLOSE_ATTACK = 2,
    MAX_ACTOR_ATTACK = 6,
};

struct ACTOR_ACTION_SET
{
    STATE* *Stand;
    STATE* *Run;
    STATE* *Jump;
    STATE* *Fall;
    STATE* *Crawl;
    STATE* *Swim;
    STATE* *Fly;
    STATE* *Rise;
    STATE* *Sit;
    STATE* *Look;
    STATE* *Climb;
    STATE* *Pain;
    STATE* *Death1;
    STATE* *Death2;
    STATE* *Dead;
    STATE* *DeathJump;
    STATE* *DeathFall;

    STATE* *CloseAttack[MAX_ACTOR_CLOSE_ATTACK];
    int16_t  CloseAttackPercent[MAX_ACTOR_CLOSE_ATTACK];

    STATE* *Attack[MAX_ACTOR_ATTACK];
    int16_t  AttackPercent[MAX_ACTOR_ATTACK];

    STATE* *Special[2];
    STATE* *Duck;
    STATE* *Dive;
};

struct ROTATOR
{
    int pos;           // current position - always moves toward tgt
    int open_dest;     // destination of open position
    int tgt;           // current target
    int speed;         // speed of movement
    int orig_speed;    // original speed - vel jacks with speed
    int vel;           // velocity adjuments

	TArray<DVector2> orig;

    void SetNumWalls(int num)
    {
        orig.Resize(num);
        memset(orig.Data(), 0, num * sizeof(DVector2));
    }

    void ClearWalls()
    {
        orig.Reset();
    }

};

//
// User Extension record
//

struct USER
{
    void Clear()
    {
        rotator.Clear();
        WallShade.Clear();
        memset(&WallP, 0, sizeof(USER) - myoffsetof(USER, WallP));
    }

	// frequently repeated patterns
	void addCounterToChange() { change.Z += Counter * zmaptoworld; }
	

    //
    // Variables that can be used by actors and Player
    //
    TPointer<ROTATOR> rotator;

    // wall vars for lighting
    TArray<int8_t> WallShade;

    walltype* WallP; // operate on wall instead of sprite
    STATE* State;
    STATE* *Rot;
    STATE* StateStart;
    STATE* StateEnd;
    STATE* *StateFallOverride; // a bit kludgy - override std fall state

    ANIMATOR* ActorActionFunc;
    ACTOR_ACTION_SET* ActorActionSet;
    PERSONALITY* Personality;
    ATTRIBUTE* Attrib;
    SECTOR_OBJECT* sop_parent;  // denotes that this sprite is a part of the
    // sector object - contains info for the SO

    // referenced actors
    TObjPtr<DSWActor*> lowActor, highActor;
    TObjPtr<DSWActor*> targetActor; // target player for the enemy - can only handle one player at at time
    TObjPtr<DSWActor*> flameActor;
    TObjPtr<DSWActor*> attachActor;  // attach to sprite if needed - electro snake
    TObjPtr<DSWActor*> flagOwnerActor;
    TObjPtr<DSWActor*> WpnGoalActor;

    DVector3 pos;
	DVector3 change; // precalculated vectors
    double hiz, loz;
    double oz; // serialized copy of sprite.oz
    double z_tgt;
    double ceiling_dist;
    double floor_dist;
    double zclip; // z height to move up for clipmove
		

    int Flags;
    int Flags2;
    int Tics;

    int16_t RotNum;
    int16_t ID;

    // Health/Pain related
    int16_t Health;
    int16_t MaxHealth;

    int16_t LastDamage;           // last damage amount taken
    int16_t PainThreshold;       // amount of damage that can be taken before
    // going into pain frames.

    // jump & fall
    int16_t jump_speed;
    int16_t jump_grav;

    // clipmove
    double lo_step;
    int active_range;
    sectortype* hi_sectp, *lo_sectp;


    // if a player's sprite points to player structure
    DSWPlayer* PlayerP;
    int16_t Sibling;


    //
    // Possibly used by both.
    //

    // velocity
    int  vel_tgt;
    int16_t vel_rate;
    uint8_t speed; // Ordinal Speed Range 0-3 from slow to fast

    int16_t Counter;
    int16_t Counter2;
    int16_t Counter3;
    int16_t DamageTics;
    int16_t BladeDamageTics;

    unsigned int Radius;    // for distance checking
    double fRadius() const { return Radius * inttoworld; }
    double  OverlapZ;  // for z overlap variable

    //
    // Only have a place for actors
    //



    // scaling
    int16_t scale_speed;
    unsigned short scale_value;
    int16_t scale_tgt;

    // zig zagging
    double DistCheck;

    double Dist;
    double TargetDist;
    int16_t WaitTics;

    // track
    int16_t track;
    int16_t point;
    int16_t track_dir;
    int  track_vel;

    // sliding variables - slide backwards etc
    DAngle slide_ang;
    double slide_vel;
    double slide_dec;

    int16_t motion_blur_dist;
    int16_t motion_blur_num;

    int16_t wait_active_check;  // for enemy checking of player
    int16_t inactive_time; // length of time actor has been unaware of his tgt
    DAngle sang;
    uint8_t spal;  // save off default palette number

    Collision coll; // same thing broken up into useful components.

    // Need to get rid of these flags
    int  Flag1;

    int8_t  LastWeaponNum;
    int8_t  WeaponNum;

    int16_t bounce;           // count bounces off wall for killing shrap stuff
    // !JIM! my extensions
    int ShellNum;          // This is shell no. 0 to whatever
    // Shell gets deleted when ShellNum < (ShellCount - MAXSHELLS)
    int16_t FlagOwner;        // Not the spritenum of the original flag (abused to hell by other things)
    int16_t Vis;              // Shading upgrade, for shooting, etc...
    bool DidAlert;          // Has actor done his alert noise before?

    DAngle oangdiff;      // Used for interpolating sprite angles

    uint8_t filler;
};

enum
{
    // sprite->extra flags
    // BUILD AND GAME - DO NOT MOVE THESE
    SPRX_SKILL = (BIT(0) | BIT(1) | BIT(2)),

    // BIT(4) ST1 BUILD AND GAME
    SPRX_STAY_PUT_VATOR = (BIT(5)),    // BUILD AND GAME - will not move with vators etc
    // DO NOT MOVE THIS

    SPRX_STAG = (BIT(6)),    // BUILD AND GAME - NON-ST1 sprite with ST1 type tagging
    // DO NOT MOVE

    SPRX_QUEUE_SPRITE = (BIT(7)),    // Queue sprite -check queue when deleting
    SPRX_MULTI_ITEM = (BIT(9)),    // BUILD AND GAME - multi player item

    // have users - could be moved
    SPRX_PLAYER_OR_ENEMY = (BIT(11)),   // for checking quickly if sprite is a
    // player or actor
    // do not need Users
    SPRX_FOUND = (BIT(12)),   // BUILD ONLY INTERNAL - used for finding sprites
    SPRX_BLADE = (BIT(12)),   // blade sprite
    SPRX_BREAKABLE = (BIT(13)),   // breakable items
    SPRX_BURNABLE = (BIT(14)),   // used for burnable sprites in the game

    // temp use
    SPRX_BLOCK = (BIT(15)),   // BUILD AND GAME
};

// BUILD - tell which actors should not spawn
// GAME - used for internal game code
// ALT-M debug mode

// !LIGHT
// all three bits set - should never happen with skill
// #define SPRX_USER_NON_STANDARD  (BIT(0)|BIT(1)|BIT(2))   // used for lighting

// boolean flags carried over from build
enum
{
    SPRX_BOOL11 = (BIT(5)),
    SPRX_BOOL1 = (BIT(6)),
    SPRX_BOOL2 = (BIT(7)),
    SPRX_BOOL3 = (BIT(8)),
    SPRX_BOOL4 = (BIT(9)),
    SPRX_BOOL5 = (BIT(10)),
    SPRX_BOOL6 = (BIT(11)),
    SPRX_BOOL7 = (BIT(4)),  // bit 12 was used build
    SPRX_BOOL8 = (BIT(13)),
    SPRX_BOOL9 = (BIT(14)),
    SPRX_BOOL10 = (BIT(15)),
};

enum ETexFlags
{
    TFLAG_BUNNYFRIENDLY = 1, // bunnies like to stay in sectors with this as floor.
};

enum ESurfType
{
    TSURF_WATER         = 1,
    TSURF_LAVA          = 2,
    TSURF_SHALLOWWATER  = 3,
};

// User->Flags flags
enum
{
    SPR_MOVED               = BIT(0), // Did actor move
    SPR_ATTACKED            = BIT(1), // Is sprite being attacked?
    SPR_TARGETED            = BIT(2), // Is sprite a target of a weapon?
    SPR_ACTIVE              = BIT(3), // Is sprite aware of the player?
    SPR_ELECTRO_TOLERANT    = BIT(4), // Electro spell does not slow actor
    SPR_JUMPING             = BIT(5), // Actor is jumping
    SPR_FALLING             = BIT(6), // Actor is falling
    SPR_CLIMBING            = BIT(7), // Actor is falling
    SPR_DEAD               = BIT(8), // Actor is dying

    SPR_ZDIFF_MODE          = BIT(10), // For following tracks at different z heights
    SPR_SPEED_UP            = BIT(11), // For following tracks at different speeds
    SPR_SLOW_DOWN           = BIT(12), // For following tracks at different speeds
    SPR_DONT_UPDATE_ANG     = BIT(13), // For tracks - don't update the angle for a while

    SPR_SO_ATTACHED         = BIT(14), // sprite is part of a sector object
    SPR_SUICIDE             = BIT(15), // sprite is set to kill itself

    SPR_RUN_AWAY            = BIT(16), // sprite is in "Run Away" track mode.
    SPR_FIND_PLAYER         = BIT(17), // sprite is in "Find Player" track mode.

    SPR_SWIMMING            = BIT(18), // Actor is swimming
    SPR_WAIT_FOR_PLAYER     = BIT(19), // Track Mode - Actor is waiting for player to come close
    SPR_WAIT_FOR_TRIGGER    = BIT(20), // Track Mode - Actor is waiting for player to trigger
    SPR_SLIDING             = BIT(21), // Actor is sliding
    SPR_ON_SO_SECTOR        = BIT(22), // sprite is on a sector object sector

    SPR_SHADE_DIR           = BIT(23), // sprite is on a sector object sector
    SPR_XFLIP_TOGGLE        = BIT(24), // sprite rotation xflip bit
    SPR_NO_SCAREDZ          = BIT(25), // not afraid of falling

    SPR_SET_POS_DONT_KILL   = BIT(26), // Don't kill sprites in MissileSetPos
    SPR_SKIP2               = BIT(27), // 20 moves ps
    SPR_SKIP4               = BIT(28), // 10 moves ps

    SPR_BOUNCE              = BIT(29), // For shrapnel types that can bounce once
    SPR_UNDERWATER          = BIT(30), // For missiles etc

    SPR_SHADOW              = BIT(31), // Sprites that have shadows

    // User->Flags2 flags
    SPR2_BLUR_TAPER         = BIT(13)|BIT(14),   // taper type
    SPR2_BLUR_TAPER_FAST    = BIT(13),   // taper fast
    SPR2_BLUR_TAPER_SLOW    = BIT(14),   // taper slow
    SPR2_SPRITE_FAKE_BLOCK  = BIT(15),   // fake blocking bit for damage
    SPR2_NEVER_RESPAWN      = BIT(16),   // for item respawning
    SPR2_ATTACH_WALL        = BIT(17),
    SPR2_ATTACH_FLOOR       = BIT(18),
    SPR2_ATTACH_CEILING     = BIT(19),
    SPR2_CHILDREN           = BIT(20),   // sprite OWNS children
    SPR2_SO_MISSILE         = BIT(21),   // this is a missile from a SO
    SPR2_DYING              = BIT(22),   // Sprite is currently dying
    SPR2_VIS_SHADING        = BIT(23),   // Sprite shading to go along with vis adjustments
    SPR2_DONT_TARGET_OWNER  = BIT(24),
    SPR2_FLAMEDIE           = BIT(25),  // was previously 'flame == -2'
};


///////////////////////////////////////////////////////////////////////////////////////////
//
// Sector Stuff - Sector Objects and Tracks
//
///////////////////////////////////////////////////////////////////////////////////////////

// flags in EXTRA variable
enum
{
    SECTFX_SINK                  = BIT(0),
    SECTFX_OPERATIONAL           = BIT(1),
    SECTFX_WARP_SECTOR           = BIT(2),
    SECTFX_CURRENT               = BIT(3),
    SECTFX_Z_ADJUST              = BIT(4), // adjust ceiling/floor
    SECTFX_NO_RIDE               = BIT(5), // moving sector - don't ride it
    SECTFX_DYNAMIC_AREA          = BIT(6),
    SECTFX_DIVE_AREA             = BIT(7), // Diving area
    SECTFX_UNDERWATER            = BIT(8), // Underwater area
    SECTFX_UNDERWATER2           = BIT(9), // Underwater area

    SECTFX_LIQUID_MASK           = (BIT(10)|BIT(11)), // only valid for sectors with depth
    SECTFX_LIQUID_NONE           = (0),
    SECTFX_LIQUID_LAVA           = BIT(10),
    SECTFX_LIQUID_WATER          = BIT(11),
    SECTFX_SECTOR_OBJECT         = BIT(12),  // for collision detection
    SECTFX_VATOR                 = BIT(13),  // denotes that this is a vertical moving sector vator type
    SECTFX_TRIGGER               = BIT(14),  // trigger type to replace tags.h trigger types
};

// flags in sector USER structure
enum
{
    SECTFU_SO_DONT_BOB                = BIT(0),
    SECTFU_SO_SINK_DEST               = BIT(1),
    SECTFU_SO_DONT_SINK               = BIT(2),
    SECTFU_DONT_COPY_PALETTE          = BIT(3),
    SECTFU_SO_SLOPE_FLOOR_TO_POINT    = BIT(4),
    SECTFU_SO_SLOPE_CEILING_TO_POINT  = BIT(5),
    SECTFU_DAMAGE_ABOVE_SECTOR        = BIT(6),
    SECTFU_VATOR_BOTH                 = BIT(7), // vators set up for both ceiling and floor
    SECTFU_CANT_SURFACE               = BIT(8), // for diving
    SECTFU_SLIDE_SECTOR               = BIT(9), // for diving
};

#define MAKE_STAG_ENUM
enum STAG_ID
{
#include "stag.h"
};
#undef MAKE_STAG_ENUM


enum {
    WALLFX_LOOP_DONT_SPIN         = BIT(0),
    WALLFX_LOOP_REVERSE_SPIN      = BIT(1),
    WALLFX_LOOP_SPIN_2X           = BIT(2),
    WALLFX_LOOP_SPIN_4X           = BIT(3),
    WALLFX_LOOP_OUTER             = BIT(4), // for sector object
    WALLFX_DONT_MOVE              = BIT(5), // for sector object
    WALLFX_SECTOR_OBJECT          = BIT(6), // for collision detection
    WALLFX_DONT_STICK             = BIT(7), // for bullet holes and stars
    WALLFX_DONT_SCALE             = BIT(8), // for sector object
    WALLFX_LOOP_OUTER_SECONDARY   = BIT(9), // for sector object
};

enum ShrapType
{
    SHRAP_NONE              = 0,
    SHRAP_GLASS             = 1,  //
    SHRAP_TREE_BARK         = 2,  // (NEED) outside tree bark
    SHRAP_SO_SMOKE          = 3,  // only used for damaged SO's
    SHRAP_PAPER             = 4,  //
    SHRAP_BLOOD             = 5,  // std blood from gibs
    SHRAP_EXPLOSION         = 6,  // small explosion
    SHRAP_LARGE_EXPLOSION   = 7,  // large explosion
    SHRAP_METAL             = 8,  //
    SHRAP_STONE             = 9,  // what we have might be ok
    SHRAP_PLANT             = 10, // (NEED)
    SHRAP_GIBS              = 11, // std blood and guts
    SHRAP_WOOD              = 12, //
    SHRAP_GENERIC           = 13, // what we have might be ok - sort of gray brown rock look
    SHRAP_TREE_PULP         = 14, // (NEED) inside tree wood
    SHRAP_COIN              = 15,
    SHRAP_METALMIX          = 16,
    SHRAP_WOODMIX           = 17,
    SHRAP_MARBELS           = 18,
    SHRAP_PAPERMIX          = 19,
    SHRAP_USER_DEFINED      = 99
};

# define CallocMem(size, num) M_Calloc(size, num)
# define FreeMem(ptr) M_Free(ptr)

struct TARGET_SORT
{
    DSWActor* actor;
    DAngle dangle;
    double dst;
    double weight;
};

enum { MAX_TARGET_SORT = 16 };
extern TARGET_SORT TargetSort[MAX_TARGET_SORT];
extern unsigned TargetSortCount;

enum DOOR_TYPE
{
    OPERATE_TYPE,
    DOOR_HORIZ_TYPE,
    DOOR_SLIDE_TYPE,
    DOOR_SWING_TYPE,
    DOOR_ROTATE_TYPE
};

struct DOOR_AUTO_CLOSE
{
    DOOR_TYPE Type;
    int Sector;
    int16_t Speed;
    int16_t TimeOut;
};

struct SWING
{
    int origx[17], origy[17];
    int sector;
    int16_t angopen, angclosed, angopendir, sang, anginc;
};

struct SINE_WAVE_FLOOR
{
    sectortype* sectp;
    double floorOrigz, ceilingOrigz, Range;
    int16_t sintable_ndx, speed_shift;
    uint8_t flags;
};

enum
{
    MAX_SINE_WAVE = 12,
    MAX_SINE_WALL = 10,
    MAX_SINE_WALL_POINTS = 64,
};

extern SINE_WAVE_FLOOR SineWaveFloor[MAX_SINE_WAVE][21];

struct SINE_WALL
{
    walltype* wallp;
	double origXY;
	double Range;
    int16_t sintable_ndx, speed_shift, type;
};

extern SINE_WALL SineWall[MAX_SINE_WALL][MAX_SINE_WALL_POINTS];

struct SPRING_BOARD
{
    sectortype* sectp;
    int TimeOut;
};

extern SPRING_BOARD SpringBoard[20];
extern SWING Rotate[17];

enum
{
    MAX_DOOR_AUTO_CLOSE = 16,
    MAXANIM = 256
};


extern DOOR_AUTO_CLOSE DoorAutoClose[MAX_DOOR_AUTO_CLOSE];

typedef void (*ANIM_CALLBACKp) (ANIM*, void *);

enum
{
    ANIM_Floorz,
    ANIM_SopZ,
    ANIM_Spritez,
    ANIM_Userz,
    ANIM_SUdepth,
};


struct TRACK_POINT
{
    DVector3 pos;
    DAngle angle;
    int16_t tag_low, tag_high;
};

struct TRACK
{
    TRACK_POINT* TrackPoint;
    int ttflags;
    int flags;
    int NumPoints;

    void FreeTrackPoints()
    {
        if (TrackPoint)
        {
            M_Free(TrackPoint);
            // !JIM! I added null assigner
            TrackPoint = nullptr;
        }
    }

    TRACK_POINT* SetTrackSize(unsigned newsize)
    {
        FreeTrackPoints();
        TrackPoint = (TRACK_POINT*)M_Calloc((newsize * sizeof(TRACK_POINT)), 1);
        return TrackPoint;
    }

};

// Most track type flags are in tags.h

// Regular track flags

struct COLOR_MAP
{
    uint8_t FromRange,ToRange,FromColor,ToColor;
};

enum
{
    TF_TRACK_OCCUPIED = BIT(0),
    MAX_TRACKS = 100,
    MAX_SO_SECTOR = 50,
    MAX_SO_POINTS = (MAX_SO_SECTOR*15),
    MAX_SO_SPRITE = 60,
    MAX_CLIPBOX = 32
};

extern TRACK Track[MAX_TRACKS];

struct SECTOR_OBJECT
{
    soANIMATORp PreMoveAnimator;
    soANIMATORp PostMoveAnimator;
    soANIMATORp Animator;
    TObjPtr<DSWActor*> controller;

    TObjPtr<DSWActor*> sp_child;  // child sprite that holds info for the sector object

    DVector3 pmid;  // midpoints of the sector object

	TObjPtr<DSWActor*> so_actors[MAX_SO_SPRITE];    // hold the actors of the object
	TObjPtr<DSWActor*> match_event_actor; // spritenum of the match event sprite

    sectortype
        *sectp[MAX_SO_SECTOR],
        *scratch,           // Just a filler to account for shitty loop tests.
        *op_main_sector, // main sector operational SO moves in - for speed purposes
        *mid_sector;     // middle sector

    walltype
        * morph_wall_point;       // actual wall point to drag

    double
           target_dist,    // distance to next point
           zdelta;         // z delta from original


    int    vel,            // velocity
           vel_tgt,        // target velocity
           update;         // Distance from player at which you continue updating

    double  zorig_floor[MAX_SO_SECTOR],      // original z values for all sectors
           z_tgt;          // target z delta
    int    z_rate;         // rate at which z aproaches target
    // only works for single player.
    double bob_diff,       // bobbing difference for the frame
           floor_loz,      // floor low z
           floor_hiz,      // floor hi z
           bob_amt;        // bob amount max in z coord

    // variables set by mappers for drivables
    double drive_angspeed;
    int    drive_angslide,
           drive_speed,
           drive_slide,
           flags;

    double crush_z,
	       morph_z,        // morphing point z
           morph_z_min,    // morphing point z min
           morph_z_max;

    DVector2 orig[MAX_SO_POINTS];   // save the original x & y location of each wall so it can be
    int16_t max_damage,     // max damage
            ram_damage,     // damage taken by ramming
            wait_tics,      //
            num_sectors,    // number of sectors
            num_walls,      // number of sectors
            track,          // the track # 0 to 20
            point,          // the point on the track that the sector object is headed toward
            vel_rate,       // rate at which velocity aproaches target
            dir;            // direction traveling on the track
    double  clipdist;       // cliping distance for operational sector objects
    double  clipbox_dist[MAX_CLIPBOX]; // mult-clip box variables (clipdist equivalent)
    double  clipbox_vdist[MAX_CLIPBOX]; // mult-clip box variables
    DAngle  clipbox_ang[MAX_CLIPBOX]; // mult-clip box variables


    int16_t clipbox_num;
   DAngle   ang;            // angle facing
   DAngle   ang_moving;     // angle the SO is facing
   DAngle   ang_tgt;        // target angle
   DAngle   ang_orig;       // original angle
   DAngle   last_ang,       // last angle before started spinning
            old_ang;        // holding variable for the old angle
   DAngle   spin_speed,     // spin_speed
            save_spin_speed, // save spin speed
            spin_ang;       // spin angle
   int16_t         turn_speed,     // shift value determines how fast SO turns to match new angle
            bob_sine_ndx,   // index into sine table
            bob_speed,      // shift value for speed
            save_vel,       // save velocity
            match_event,    // match number
    // SO Scaling Vector Info
            scale_type,         // type of scaling - enum controled
            scale_active_type,  // activated by a switch or trigger

    // values for whole SO
            scale_rand_freq;    // freqency of direction change - based on rand(1024)

    double  scale_dist,         // distance from center
            scale_dist_min,     // absolute min
            scale_dist_max,     // absolute max
            scale_speed,        // speed of scaling

    // values for single point scaling
            scale_point_dist[MAX_SO_POINTS],         // distance from center
            scale_point_speed[MAX_SO_POINTS],        // speed of scaling
            scale_point_base_speed,                       // base speed of scaling
            scale_point_dist_min,     // absolute min
            scale_point_dist_max;     // absolute max
     int16_t scale_point_rand_freq,    // freqency of direction change - based on rand(1024)

            scale_x_mult,           // x multiplyer for scaling
            scale_y_mult,           // y multiplyer for scaling

    // Used for center point movement
            morph_rand_freq;        // freq of dir change

    DAngle  morph_ang;              // angle moving from CENTER
    double  morph_speed,            // speed of movement
            morph_dist_max,         // radius boundry
            morph_dist,             // dist from CENTER
            morph_z_speed;          // z speed for morph point
    DVector2 morph_off;             // save yoff from center

    //scale_rand_reverse;            // random at random interval
    // limit rotation angle
    DAngle  limit_ang_center, // for limiting the angle of turning - turrets etc
            limit_ang_delta; //

};

enum
{
    SOBJ_SPEED_UP           = BIT(0) ,
    SOBJ_SLOW_DOWN          = BIT(1) ,
    SOBJ_ZUP                = BIT(2) ,
    SOBJ_ZDOWN              = BIT(3) ,
    SOBJ_ZDIFF_MODE         = BIT(4) ,
    SOBJ_MOVE_VERTICAL      = BIT(5) ,// for sprite objects - move straight up/down
    SOBJ_ABSOLUTE_ANGLE     = BIT(7) ,
    SOBJ_SPRITE_OBJ         = BIT(8) ,
    SOBJ_DONT_ROTATE        = BIT(9) ,
    SOBJ_WAIT_FOR_EVENT     = BIT(10),
    SOBJ_HAS_WEAPON         = BIT(11),
    SOBJ_SYNC1              = BIT(12), // for syncing up several SO's perfectly
    SOBJ_SYNC2              = BIT(13), // for syncing up several SO's perfectly
    SOBJ_DYNAMIC            = BIT(14), // denotes scaling or morphing object
    SOBJ_ZMID_FLOOR         = BIT(15), // can't remember which sector objects need this think its the bobbing and sinking ones
    SOBJ_SLIDE              = BIT(16),    
    SOBJ_OPERATIONAL        = BIT(17),
    SOBJ_KILLABLE           = BIT(18),
    SOBJ_DIE_HARD           = BIT(19),
    SOBJ_UPDATE_ONCE        = BIT(20),
    SOBJ_UPDATE             = BIT(21),
    SOBJ_NO_QUAKE           = BIT(22),
    SOBJ_REMOTE_ONLY        = BIT(23),
    SOBJ_RECT_CLIP          = BIT(24),
    SOBJ_BROKEN             = BIT(25),
};

// track set to these to tell them apart
enum
{
    MAX_SECTOR_OBJECTS = 20,
    SO_OPERATE_TRACK_START = 90,
    SO_TURRET_MGUN = 96, // machine gun
    SO_TURRET = 97,
    SO_VEHICLE = 98,
    // #define SO_SPEED_BOAT 99
};

// make sure this does not overflow when converted to a Build int coordinate and survives a round trip through conversions.
constexpr double MAXSO = 0x7fffffe0 / 32; 
static_assert(MAXSO == int(MAXSO * worldtoint) * inttoworld);

inline bool SO_EMPTY(SECTOR_OBJECT* sop) { return (sop->pmid.X == MAXSO); }

extern SECTOR_OBJECT SectorObject[MAX_SECTOR_OBJECTS];


///////////////////////////////////////////////////////////////////////////////////////////
//
// Prototypes
//
///////////////////////////////////////////////////////////////////////////////////////////

ANIMATOR NullAnimator;

int NewStateGroup(DSWActor* actor, STATE* SpriteGroup[]);
DVector3 SectorMidPoint(sectortype* sectp);
void SpawnUser(DSWActor* actor, short id, STATE* state);

short ActorFindTrack(DSWActor* actor, int8_t player_dir, int track_type, int *track_point_num, int *track_dir);

// Some sounds were checked by storing handles in static local variables.
// Problems with this design:
// 1. The variables were unmaintained and could refer to handles that had been reused already.
// 2. No proper sound ownership tracking.
// 3. In some cases items that were supposed to use the same check referred to different handle variables.
// In short: It was very broken. This is a list of all sound items used this way, now each one gets a dedicated channel
// so that proper checks can be performed and sound ownership be tracked.

enum
{
    CHAN_ToiletFart = 1000,
    CHAN_AnimeMad = 1001,
    CHAN_AnimeSing = 1002,
    CHAN_CoyHandle = 1003,
    CHAN_RipHeart = 1004,
};

short SoundAngle(int x, int  y);
//void PlaySound(int num, short angle, short vol);
int _PlaySound(FSoundID num, DSWActor* sprite, DSWPlayer* player, const DVector3 *const pos, int flags, int channel, EChanFlags sndflags);
void InitAmbient(int num, DSWActor* actor);

inline void PlaySound(int num, DSWPlayer* player, int flags, int channel = 8, EChanFlags sndflags = CHANF_NONE)
{
    _PlaySound(soundEngine->FindSoundByResID(num), nullptr, player, nullptr, flags | v3df_follow, channel, sndflags);
}
inline void PlaySound(int num, int flags, int channel = 8, EChanFlags sndflags = CHANF_NONE)
{
    _PlaySound(soundEngine->FindSoundByResID(num), nullptr, nullptr, nullptr, flags, channel, sndflags);
}
inline void PlaySound(int num, const DVector3 &pos, int flags, int channel = 8, EChanFlags sndflags = CHANF_NONE)
{
	_PlaySound(soundEngine->FindSoundByResID(num), nullptr, nullptr, &pos, flags, channel, sndflags);
}

int _PlayerSound(int num, DSWPlayer* pp);
inline int PlayerSound(int num, int flags, DSWPlayer* pp) { return _PlayerSound(num, pp); }
void StopPlayerSound(DSWPlayer* pp, int which = -1);
 bool SoundValidAndActive(DSWActor* spr, int channel);


ANIMATOR DoActorBeginJump,DoActorJump,DoActorBeginFall,DoActorFall,DoActorDeathMove;

struct BREAK_INFO;
int SpawnShrap(DSWActor*, DSWActor*, int = -1, BREAK_INFO* breakinfo = nullptr);

void PlayerUpdateHealth(DSWPlayer* pp, short value);
void PlayerUpdateAmmo(DSWPlayer* pp, short WeaponNum, short value);
void PlayerUpdateWeapon(DSWPlayer* pp, short WeaponNum);
void PlayerUpdateKills(DSWPlayer* pp, short value);
void RefreshInfoLine(DSWPlayer* pp);

void DoAnim(int numtics);
void AnimDelete(int animtype, int animindex, DSWActor*);
short AnimGetGoal(int animtype, int animindex, DSWActor*);
int AnimSet(int animtype, int animindex, DSWActor* animactor, double thegoal, double thevel);
int AnimSet(int animtype, sectortype* animindex, double thegoal, double thevel)
{
    return AnimSet(animtype, sectindex(animindex), nullptr, thegoal, thevel);
}

short AnimSetCallback(short anim_ndx, ANIM_CALLBACKp call, SECTOR_OBJECT* data);
short AnimSetVelAdj(short anim_ndx, double vel_adj);

void EnemyDefaults(DSWActor* actor, ACTOR_ACTION_SET* action, PERSONALITY* person);

void getzrangepoint(const DVector3& pos, sectortype* sect, double* hiz, Collision* ceilhit, double* loz, Collision* florhit);

Collision move_sprite(DSWActor* actor, const DVector3& change, double ceildist, double flordist, uint32_t cliptype, int numtics);
Collision move_missile(DSWActor* actor, const DVector3& change, double ceildist, double flordist, uint32_t cliptype, int numtics);


DSWActor* DoPickTarget(DSWActor*, DAngle max_delta_ang, int skip_targets);

void change_actor_stat(DSWActor* actor, int stat, bool quick = false);
void SetOwner(DSWActor*, DSWActor*, bool flag = true);
void SetOwner(int a, int b); // we still need this...
void ClearOwner(DSWActor* ownr);
DSWActor* GetOwner(DSWActor* child);
void SetAttach(DSWActor*, DSWActor*);
void CollectPortals();

int SpawnBlood(DSWActor* actor, DSWActor* weapActor, DAngle hit_angle = nullAngle, const DVector3* hitpos = nullptr);

inline FTextureID FAFPlaceMirrorPic[2];
inline FTextureID FAFMirrorPic[2];

inline bool FAF_ConnectCeiling(sectortype* sect)
{
    return (sect && sect->ceilingtexture == FAFMirrorPic[0]);
}

inline bool FAF_ConnectFloor(sectortype* sect)
{
    return (sect && sect->floortexture == FAFMirrorPic[0]);
}

inline bool FAF_ConnectArea(sectortype* sect)
{
    return sect && (FAF_ConnectCeiling(sect) || FAF_ConnectFloor(sect));
}


void FAFhitscan(const DVector3& start, sectortype* sect, const DVector3& vect, HitInfo& hit, int32_t clipmask);
bool FAFcansee(const DVector3& start, sectortype* sects, const DVector3& end, sectortype* secte);
void FAFgetzrange(const DVector3& pos, sectortype* sect, double* hiz, Collision* ceilhit, double* loz, Collision* florhit, double clipdist, int32_t clipmask);
void FAFgetzrangepoint(const DVector3& pos, sectortype* sect, double* hiz, Collision* ceilhit, double* loz, Collision* florhit);


enum SoundType
{
    SOUND_OBJECT_TYPE,
    SOUND_EVERYTHING_TYPE
};

void DoSoundSpotMatch(short match, short sound_num, short sound_type);


///////////////////////////////////////////////////////////////////////////////////////////
//
//  Externs
//
///////////////////////////////////////////////////////////////////////////////////////////

extern bool NewGame;
extern uint8_t CommPlayers;
extern bool CommEnabled;
extern int LastFrameTics;
extern char ds[645];
extern short Skill;
extern int GodMode;

extern bool ReloadPrompt;

extern int lockspeed;

// Various scattered constants
enum
{
    ACTOR_GRAVITY = 8,
    // subtract value from clipdist on getzrange calls
    STAT_DAMAGE_LIST_SIZE = 20,
    COLOR_PAIN  = 128,  // Light red range

    ANIM_SERP = 1,
    ANIM_SUMO  =2,
    ANIM_ZILLA  =3

};

constexpr int synctics = 3;
constexpr int ACTORMOVETICS = (synctics << 1);
constexpr int TICSPERMOVEMENT = synctics;


constexpr double GETZRANGE_CLIP_ADJ = 0.5;


extern int *lastpacket2clock;


///////////////////////////
//
// RECENT network additions
//
///////////////////////////

extern int MoveSkip4, MoveSkip2, MoveSkip8;
extern int MinEnemySkill;
extern short screenpeek;

extern int16_t StatDamageList[STAT_DAMAGE_LIST_SIZE];

///////////////////////////////////////////////////////////////
//
// Stuff for player palette flashes when hurt or getting items
//
///////////////////////////////////////////////////////////////

extern void SetFadeAmt(DSWPlayer* pp, short damage, uint8_t startcolor);
extern void DoPaletteFlash(DSWPlayer* pp);
extern bool NightVision;



///////////////////////////////////////////////////////////////
//
// Stuff added by JonoF. These should get put into their own
// headers and included by that which needs them.
//
///////////////////////////////////////////////////////////////

int PickJumpMaxSpeed(DSWActor*, short max_speed); // ripper.c
int DoRipperRipHeart(DSWActor*);  // ripper.c
int DoRipper2RipHeart(DSWActor*); // ripper2.c
DSWActor* BunnyHatch2(DSWActor*);  // bunny.c

void TerminateLevel(void);  // game.c
void DrawMenuLevelScreen(void); // game.c
void DebugWriteString(char *string);    // game.c

void getsyncstat(void); // sync.c
void SyncStatMessage(void); // sync.c

int COVERsetgamemode(int mode, int xdim, int ydim, int bpp);    // draw.c
void ScreenCaptureKeys(void);   // draw.c

void SetupMirrorTiles(void);    // rooms.c
bool FAF_Sector(sectortype* sect); // rooms.c
double GetZadjustment(sectortype* sect,short hitag);  // rooms.c

void InitSetup(void);   // setup.c

void LoadKVXFromScript(TilesetBuildInfo& info, const char *filename); // scrip2.c
void LoadCustomInfoFromScript(const char *filename);  // scrip2.c

int PlayerInitChemBomb(DSWPlayer* pp); // jweapon.c
int PlayerInitFlashBomb(DSWPlayer* pp);    // jweapon.c
int PlayerInitCaltrops(DSWPlayer* pp); // jweapon.c
int InitPhosphorus(DSWActor*);    // jweapon.c
void SpawnFloorSplash(DSWActor*); // jweapon.c

int SaveGame(short save_num);   // save.c
int LoadGame(short save_num);   // save.c
int LoadGameFullHeader(short save_num, char *descr, short *level, short *skill);    // save,c
void LoadGameDescr(short save_num, char *descr);    // save.c

void SetRotatorActive(DSWActor* actor); // rotator.c

bool VatorSwitch(short match, short setting); // vator.c
void MoveSpritesWithSector(sectortype* sect,double z_amt,bool type);  // vator.c
void SetVatorActive(DSWActor*);   // vator.c

void DoSpikeMatch(short match); // spike.c
void SpikeAlign(DSWActor*);   // spike.c

short DoSectorObjectSetScale(short match);  // morph.c
short DoSOevent(short match,short state);   // morph.c
void SOBJ_AlignCeilingToPoint(SECTOR_OBJECT* sop, const DVector3& pos);    // morph.c
void SOBJ_AlignFloorToPoint(SECTOR_OBJECT* sop, const DVector3& pos);  // morph.c
void ScaleSectorObject(SECTOR_OBJECT* sop); // morph.c
void MorphTornado(SECTOR_OBJECT* sop);  // morph.c
void MorphFloor(SECTOR_OBJECT* sop);    // morph.c
DVector2 ScaleRandomPoint(SECTOR_OBJECT* sop, int k, DAngle ang, const DVector2& pos);    // morph.c

void CopySectorMatch(int match);  // copysect.c

int DoWallMoveMatch(short match);   // wallmove.c
int DoWallMove(DSWActor* sp); // wallmove.c
bool CanSeeWallMove(DSWActor* wp,int match);    // wallmove.c

void DoSpikeOperate(sectortype* sect); // spike.c
void SetSpikeActive(DSWActor*);   // spike.c

DSWActor* insertActor(sectortype* sect, int statnum);

void AudioUpdate(void); // stupid

extern short LastSaveNum;
void LoadSaveMsg(const char *msg);

void UpdateStatusBar();
int32_t registerosdcommands(void);

extern int OrigCommPlayers;

extern uint8_t PlayerGravity;
extern short wait_active_check_offset;
//extern short Zombies;
extern double PlaxCeilGlobZadjust, PlaxFloorGlobZadjust;
extern bool left_foot;
extern bool bosswasseen[3];
extern DSWActor* BossSpriteNum[3];
extern int ChopTics;
extern int Bunny_Count;
int HitBreakSprite(DSWActor* breakActor, int type);


END_SW_NS

#include "swactor.h"

BEGIN_SW_NS

class DSWPlayer final : public DCorePlayer
{
    DECLARE_CLASS(DSWPlayer, DCorePlayer)
    HAS_OBJECT_POINTERS
    DSWPlayer() = default;
public:
    void Clear();
    DSWPlayer(uint8_t p);
    DSWPlayer& operator=(DSWPlayer&) = delete;
    DSWPlayer(DSWPlayer&) = delete;
    void OnDestroy() override;
    void Serialize(FSerializer& arc) override;
    TObjPtr<DSWActor*> lowActor, highActor;
    TObjPtr<DSWActor*> remoteActor;
    TObjPtr<DSWActor*> PlayerUnderActor;
    TObjPtr<DSWActor*> KillerActor;  //who killed me
    TObjPtr<DSWActor*> HitBy;                    // Sprite num of whatever player was last hit by
    TObjPtr<DSWActor*> last_camera_act;

    // holds last valid move position
    DVector3 lv;

    REMOTE_CONTROL remote;
    SECTOR_OBJECT* sop_remote;
    SECTOR_OBJECT* sop;  // will either be sop_remote or sop_control

    double hiz, loz;
    double opbob_amt, pbob_amt;

    int jump_count, jump_speed;     // jumping
    double z_speed;
    int climb_ndx;
    double p_ceiling_dist,p_floor_dist;
    sectortype* hi_sectp, *lo_sectp;

    double circle_camera_dist;

    DVector2 vect, ovect, slide_vect; // these need floatification, but must be done together. vect is in 14.18 format!

    int friction;
    int16_t slide_ang; // todo: floatify
    int slide_dec;
    float drive_avel;

    DAngle circle_camera_ang;
    int16_t camera_check_time_delay;


    sectortype
        * cursector,
        * lastcursector,
        * lv_sector;

    void setcursector(sectortype* s) { cursector = s; }
    bool insector() const { return cursector != nullptr; }

    // variables that do not fit into sprite structure
    double recoil_amt;
    int16_t recoil_speed;
    int16_t recoil_ndx;
    DAngle recoil_ohorizoff, recoil_horizoff;

    DVector3 Revolve;
    DAngle RevolveDeltaAng;
    DAngle RevolveAng;

    sectortype* LadderSector;
    DVector2 LadderPosition; // ladder x and y

    int16_t JumpDuration;
    int16_t WadeDepth;
    int16_t bob_ndx;
    int16_t bcnt; // bob count
    double bob_z, obob_z;

    // must start out as 0
    int playerreadyflag;

    PLAYER_ACTION_FUNCp DoPlayerAction;
    int Flags, Flags2;
    ESyncBits KeyPressBits;

    SECTOR_OBJECT* sop_control; // sector object pointer
    SECTOR_OBJECT* sop_riding; // sector object pointer

    DPanelSprite* PanelSpriteList; // this is just a sentinel object that does not do anything. No read barrier here as this is strictly owned by the player.

    DPanelSprite* GetPanelSpriteList()
    {
        return PanelSpriteList;
    }

    // Key stuff
    uint8_t HasKey[8];

    // Weapon stuff
    int16_t SwordAng;
    int WpnGotOnceFlags; // for no respawn mode where weapons are allowed grabbed only once
    int WpnFlags;
    int16_t WpnAmmo[MAX_WEAPONS];
    int16_t WpnNum;
    TObjPtr<DPanelSprite*> CurWpn;
    TObjPtr<DPanelSprite*> Wpn[MAX_WEAPONS];
    TObjPtr<DPanelSprite*> Chops;
    uint8_t WpnRocketType; // rocket type
    uint8_t WpnRocketHeat; // 5 to 0 range
    uint8_t WpnRocketNuke; // 1, you have it, or you don't
    uint8_t WpnFlameType; // Guardian weapons fire
    uint8_t WpnFirstType; // First weapon type - Sword/Shuriken
    uint8_t WeaponType; // for weapons with secondary functions
    int16_t FirePause; // for sector objects - limits rapid firing
    //
    // Inventory Vars
    //
    int16_t InventoryNum;
    int16_t InventoryBarTics;
    int16_t InventoryTics[MAX_INVENTORY];
    int16_t InventoryPercent[MAX_INVENTORY];
    int8_t InventoryAmount[MAX_INVENTORY];
    bool InventoryActive[MAX_INVENTORY];

    int16_t DiveTics;
    int16_t DiveDamageTics;

    // Death stuff
    uint16_t DeathType;
    int16_t KilledPlayer[MAX_SW_PLAYERS_REG];

    // Health
    int16_t Armor;
    int16_t MaxHealth;

    char PlayerName[32];

    uint8_t UziShellLeftAlt;
    uint8_t UziShellRightAlt;
    uint8_t TeamColor;  // used in team play and also used in regular mulit-play for show

    // palette fading up and down for player hit and get items
    int16_t FadeTics;                 // Tics between each fade cycle
    int16_t FadeAmt;                  // Current intensity of fade
    bool NightVision;               // Is player's night vision active?
    uint8_t StartColor;       // Darkest color in color range being used
    //short electro[64];
    bool IsAI;                      // Is this and AI character?
    int16_t fta,ftq;                  // First time active and first time quote, for talking in multiplayer games
    int16_t NumFootPrints;            // Number of foot prints left to lay down
    uint8_t WpnUziType;                // Toggle between single or double uzi's if you own 2.
    uint8_t WpnShotgunType;            // Shotgun has normal or fully automatic fire
    uint8_t WpnShotgunAuto;            // 50-0 automatic shotgun rounds
    uint8_t WpnShotgunLastShell;       // Number of last shell fired
    uint8_t WpnRailType;               // Normal Rail Gun or EMP Burst Mode
    bool Bloody;                    // Is player gooey from the slaughter?
    bool InitingNuke;
    bool TestNukeInit;
    bool NukeInitialized;           // Nuke already has counted down
    int16_t FistAng;                  // KungFu attack angle
    uint8_t WpnKungFuMove;             // KungFu special moves
    int16_t Reverb;                   // Player's current reverb setting
    int16_t Heads;                    // Number of Accursed Heads orbiting player
    int PlayerVersion;

    char cookieQuote[256];          // Should be an FString but must be POD for now so that PLAYER remains POD.
    int cookieTime;
    double height;
    double svel;

    uint8_t WpnReloadState;

    double GetMaxInputVel() const override
    {
        return (380401538. / 36022361.);
    }

    const DVector2 GetInputVelocity() const override
    {
        return vect;
    }

    bool canSlopeTilt() const override
    {
        return (cmd.ucmd.actions & SB_AIMMODE) && !(Flags & (PF_FLYING|PF_SWIMMING|PF_DIVING|PF_CLIMBING|PF_JUMPING|PF_FALLING));
    }

    inline DSWActor* GetActor() override
    {
        return static_cast<DSWActor*>(actor);
    }

    void posZset(const double val)
    {
        GetActor()->spr.pos.Z = val - GetActor()->viewzoffset;
    }

    unsigned getCrouchFlags() const override
    {
        const bool crouchable = true;
        const bool disableToggle = (Flags & (PF_JUMPING|PF_FALLING|PF_CLIMBING|PF_DIVING|PF_DEAD)) || sop;
        return (CS_CANCROUCH * crouchable) | (CS_DISABLETOGGLE * disableToggle);
    }
};

inline DSWPlayer* getPlayer(int index)
{
    return static_cast<DSWPlayer*>(PlayerArray[index]);
}


struct GameInterface : public ::GameInterface
{
    const char* Name() override { return "ShadowWarrior"; }
    void app_init() override;
    void LoadTextureInfo(TilesetBuildInfo& info) override;
    void SetupSpecialTextures(TilesetBuildInfo& info) override;
    void loadPalette() override;
    void FreeLevelData() override;
    bool GenerateSavePic() override;
    void MenuSound(EMenuSounds snd) override;
    bool CanSave() override;
    bool StartGame(FNewGameStartup& gs) override;
    FSavegameInfo GetSaveSig() override;
    void SerializeGameState(FSerializer& arc);
    void SetAmbience(bool on) override { if (on) StartAmbientSound(); else StopAmbientSound(); }
    void UpdateSounds() override;
    void ErrorCleanup() override;
    void DrawBackground(void) override;
    void Ticker(void) override;
    void Render() override;
    //void DrawWeapons() override;
    void Startup() override;
    const char *CheckCheatMode() override;
    const char* GenericCheat(int player, int cheat) override;
    void LevelCompleted(MapRecord *map, int skill) override;
    void NextLevel(MapRecord *map, int skill) override;
    void NewGame(MapRecord *map, int skill, bool) override;
    bool DrawAutomapPlayer(const DVector2& mxy, const DVector2& cpos, const DAngle cang, const DVector2& xydim, const double czoom, double const interpfrac) override;
    void ToggleThirdPerson() override;
    void SwitchCoopView() override;
    void processSprites(tspriteArray& tsprites, const DVector3& view, DAngle viewang, double smoothRatio) override;
    void UpdateCameras(double smoothratio) override;
    void EnterPortal(DCoreActor* viewer, int type) override;
    void LeavePortal(DCoreActor* viewer, int type) override;
    void ExitFromMenu() override;
    int GetCurrentSkill() override;
    void StartSoundEngine() override;
    void doPlayerMovement() override
    {
        const auto pp = getPlayer(myconnectindex);
        gameInput.processMovement(!pp->sop_control ? 1 : (3. / 1.40625), !pp->sop);
    }
};


// OVER and UNDER water macros
inline bool SectorIsDiveArea(sectortype* sect)
{
    return sect->extra & SECTFX_DIVE_AREA;
}

inline bool SectorIsUnderwaterArea(sectortype* sect)
{
    return sect && sect->extra & (SECTFX_UNDERWATER | SECTFX_UNDERWATER2);
}

inline bool PlayerFacingRange(DSWPlayer* pp, DSWActor* a, DAngle range)
{
    return absangle((a->spr.pos.XY() - pp->GetActor()->spr.pos.XY()).Angle(), pp->GetActor()->spr.Angles.Yaw) < range;
}

inline bool FacingRange(DSWActor* a1, DSWActor* a2, DAngle range)
{
    return absangle((a1->spr.pos - a2->spr.pos).Angle(), a2->spr.Angles.Yaw) < range;
}
inline void SET_BOOL1(DSWActor* sp) { sp->spr.extra |= SPRX_BOOL1; }
inline void SET_BOOL2(DSWActor* sp) { sp->spr.extra |= SPRX_BOOL2; }
inline void SET_BOOL3(DSWActor* sp) { sp->spr.extra |= SPRX_BOOL3; }
inline void SET_BOOL4(DSWActor* sp) { sp->spr.extra |= SPRX_BOOL4; }
inline void SET_BOOL5(DSWActor* sp) { sp->spr.extra |= SPRX_BOOL5; }
inline void SET_BOOL6(DSWActor* sp) { sp->spr.extra |= SPRX_BOOL6; }
inline void SET_BOOL7(DSWActor* sp) { sp->spr.extra |= SPRX_BOOL7; }
inline void SET_BOOL8(DSWActor* sp) { sp->spr.extra |= SPRX_BOOL8; }
inline void SET_BOOL9(DSWActor* sp) { sp->spr.extra |= SPRX_BOOL9; }
inline void SET_BOOL10(DSWActor* sp) { sp->spr.extra |= SPRX_BOOL10; }
inline void SET_BOOL11(DSWActor* sp) { sp->spr.extra |= SPRX_BOOL11; }

inline void RESET_BOOL1(DSWActor* sp) { sp->spr.extra &= ~SPRX_BOOL1; }
inline void RESET_BOOL2(DSWActor* sp) { sp->spr.extra &= ~SPRX_BOOL2; }
inline void RESET_BOOL3(DSWActor* sp) { sp->spr.extra &= ~SPRX_BOOL3; }
inline void RESET_BOOL4(DSWActor* sp) { sp->spr.extra &= ~SPRX_BOOL4; }
inline void RESET_BOOL5(DSWActor* sp) { sp->spr.extra &= ~SPRX_BOOL5; }
inline void RESET_BOOL6(DSWActor* sp) { sp->spr.extra &= ~SPRX_BOOL6; }
inline void RESET_BOOL7(DSWActor* sp) { sp->spr.extra &= ~SPRX_BOOL7; }
inline void RESET_BOOL8(DSWActor* sp) { sp->spr.extra &= ~SPRX_BOOL8; }
inline void RESET_BOOL9(DSWActor* sp) { sp->spr.extra &= ~SPRX_BOOL9; }
inline void RESET_BOOL10(DSWActor* sp) { sp->spr.extra &= ~SPRX_BOOL10; }
inline void RESET_BOOL11(DSWActor* sp) { sp->spr.extra &= ~SPRX_BOOL11; }

inline int TEST_BOOL1(DSWActor* sp) { return sp->spr.extra & SPRX_BOOL1; }
inline int TEST_BOOL2(DSWActor* sp) { return sp->spr.extra & SPRX_BOOL2; }
inline int TEST_BOOL3(DSWActor* sp) { return sp->spr.extra & SPRX_BOOL3; }
inline int TEST_BOOL4(DSWActor* sp) { return sp->spr.extra & SPRX_BOOL4; }
inline int TEST_BOOL5(DSWActor* sp) { return sp->spr.extra & SPRX_BOOL5; }
inline int TEST_BOOL6(DSWActor* sp) { return sp->spr.extra & SPRX_BOOL6; }
inline int TEST_BOOL7(DSWActor* sp) { return sp->spr.extra & SPRX_BOOL7; }
inline int TEST_BOOL8(DSWActor* sp) { return sp->spr.extra & SPRX_BOOL8; }
inline int TEST_BOOL9(DSWActor* sp) { return sp->spr.extra & SPRX_BOOL9; }
inline int TEST_BOOL10(DSWActor* sp) { return sp->spr.extra & SPRX_BOOL10; }
inline int TEST_BOOL11(DSWActor* sp) { return sp->spr.extra & SPRX_BOOL11; }

// Defines for reading in ST1 sprite tagging
inline int16_t SP_TAG1(DSWActor* actor) { return actor->spr.hitag; }
inline int16_t& SP_TAG2(DSWActor* actor) { return actor->spr.lotag; }
inline uint8_t& SP_TAG3(DSWActor* actor) { return actor->spr.clipdist; }
inline int16_t& SP_TAG4(DSWActor* actor) { return actor->spr.intangle; } // this may not be transitioned to a real angular type
inline int16_t& SP_TAG5(DSWActor* actor) { return actor->spr.xint; }
inline int16_t& SP_TAG6(DSWActor* actor) { return actor->spr.yint; }
inline uint8_t& SP_TAG7(DSWActor* actor) { return MSB_VAR(actor->spr.inittype); }
inline uint8_t& SP_TAG8(DSWActor* actor) { return LSB_VAR(actor->spr.inittype); }
inline uint8_t& SP_TAG9(DSWActor* actor) { return MSB_VAR(actor->spr.intowner); }
inline uint8_t& SP_TAG10(DSWActor* actor) { return LSB_VAR(actor->spr.intowner); }
inline int8_t& SP_TAG11(DSWActor* actor) { return actor->spr.shade; }
inline uint8_t& SP_TAG12(DSWActor* actor) { return actor->spr.pal; }
inline int16_t SP_TAG13(DSWActor* actor) { return int16_t(uint8_t(actor->spr.xoffset) + (actor->spr.yoffset << 8)); }
inline void SET_SP_TAG13(DSWActor* actor, int val) { actor->spr.xoffset = uint8_t(val); actor->spr.yoffset = uint8_t(val >> 8); }

// actual Z for TOS and BOS - handles both WYSIWYG and old style
inline double ActorZOfTop(DSWActor* actor)
{
    return GetSpriteZOfTop(&actor->spr);
}

inline DVector3 ActorVectOfTop(DSWActor* actor)
{
    return DVector3(actor->spr.pos.XY(), ActorZOfTop(actor));
}

inline double ActorZOfBottom(DSWActor* actor)
{
    return GetSpriteZOfBottom(&actor->spr);
}

inline DVector3 ActorVectOfBottom(DSWActor* actor)
{
	return DVector3(actor->spr.pos.XY(), ActorZOfBottom(actor));
}

inline double ActorZOfMiddle(DSWActor* actor)
{
	return (ActorZOfTop(actor) + ActorZOfBottom(actor)) * 0.5;
}

inline DVector3 ActorVectOfMiddle(DSWActor* actor)
{
    return DVector3(actor->spr.pos.XY(), ActorZOfMiddle(actor));
}

inline double ActorSizeZ(DSWActor* actor)
{
    auto tex = TexMan.GetGameTexture(actor->spr.spritetexture());
    return (tex->GetDisplayHeight() * actor->spr.scale.Y);
}

inline double ActorUpperZ(DSWActor* actor)
{
    return (ActorZOfTop(actor) + (ActorSizeZ(actor) * 0.25));
}

inline DVector3 ActorUpperVect(DSWActor* actor)
{
    return DVector3(actor->spr.pos.XY(), ActorUpperZ(actor));
}

inline double ActorLowerZ(DSWActor* actor)
{
    return (ActorZOfBottom(actor) - (ActorSizeZ(actor) * 0.25));
}

inline DVector3 ActorLowerVect(DSWActor* actor)
{
    return DVector3(actor->spr.pos.XY(), ActorLowerZ(actor));
}

// Z size of top (TOS) and bottom (BOS) part of sprite
inline double ActorSizeToTop(DSWActor* a)
{
    auto tex = TexMan.GetGameTexture(a->spr.spritetexture());
    return (ActorSizeZ(a) + tex->GetDisplayTopOffset()) * 0.5;
}

inline void SetActorSizeX(DSWActor* sp)
{
    auto tex = TexMan.GetGameTexture(sp->spr.spritetexture());
    sp->clipdist = tex->GetDisplayWidth() * sp->spr.scale.X * 0.25;
}

inline bool Facing(DSWActor* actor1, DSWActor* actor2)
{
    return absangle((actor1->spr.pos - actor2->spr.pos).Angle(), actor2->spr.Angles.Yaw) < DAngle90;
}

// Given a z height and sprite return the correct y repeat value
inline int GetRepeatFromHeight(DSWActor* sp, double zh)
{
    auto tex = TexMan.GetGameTexture(sp->spr.spritetexture());
    return int(zh * 64) / int(tex->GetDisplayHeight());
}

inline bool SpriteInDiveArea(DSWActor* a)
{
    return a->sector()->extra & SECTFX_DIVE_AREA;
}

inline bool SpriteInUnderwaterArea(DSWActor* a)
{
    return a->sector()->extra & (SECTFX_UNDERWATER | SECTFX_UNDERWATER2);
}


// just determine if the player is moving
inline bool PLAYER_MOVING(DSWPlayer* pp)
{
    return !pp->vect.isZero();
}

inline void PlaySound(int num, DSWActor* actor, int flags, int channel = 8, EChanFlags sndflags = CHANF_NONE)
{
    _PlaySound(soundEngine->FindSoundByResID(num), actor, nullptr, nullptr, flags, channel, sndflags);
}

struct ANIM
{
	int animtype, animindex;
	double goal;
	double vel;
	double vel_adj;
	TObjPtr<DSWActor*> animactor;
	ANIM_CALLBACKp callback;
	SECTOR_OBJECT* callbackdata;    // only gets used in one place for this so having a proper type makes serialization easier.

	double getValue()
	{
		switch (animtype)
		{
		case ANIM_Floorz:
            return sector[animindex].floorz;
		case ANIM_SopZ:
			return SectorObject[animindex].pmid.Z;
		case ANIM_Spritez:
            if (animactor == nullptr) return 0;
			return animactor->spr.pos.Z;
		case ANIM_Userz:
            if (animactor == nullptr) return 0;
            return animactor->user.pos.Z;
		case ANIM_SUdepth:
			return sector[animindex].depth_fixed;
		default:
			return 0;
		}
	}

    void setValue(double value)
    {
        switch (animtype)
        {
        case ANIM_Floorz:
            sector[animindex].setfloorz(value);
			break;
        case ANIM_SopZ:
            SectorObject[animindex].pmid.Z = value;
            break;
        case ANIM_Spritez:
            if (animactor == nullptr) return;
            animactor->spr.pos.Z = value;
            break;
        case ANIM_Userz:
            if (animactor == nullptr) return;
            animactor->user.pos.Z = value;
			break;
        case ANIM_SUdepth:
            sector[animindex].depth_fixed = (int)value;
        default:
            return;
        }
    }
};

extern ANIM Anim[MAXANIM];
extern short AnimCnt;

struct USERSAVE
{
    int16_t Health;
    int8_t WeaponNum;
    int8_t LastWeaponNum;

    void CopyFromUser(DSWActor* u)
    {
        Health = u->user.Health;
        WeaponNum = u->user.WeaponNum;
        LastWeaponNum = u->user.LastWeaponNum;
    }

    void CopyToUser(DSWActor* u)
    {
        u->user.Health = Health;
        u->user.WeaponNum = WeaponNum;
        u->user.LastWeaponNum = LastWeaponNum;
    }

};

// save player info when moving to a new level (shortened to only cover the fields that actually are copied back.)
extern USERSAVE puser[MAX_SW_PLAYERS_REG];

constexpr double JUMP_FACTOR = 1. / 256.;

END_SW_NS

#endif

