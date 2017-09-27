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

#ifndef player_h_
#define player_h_

#include "inv.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int32_t g_mostConcurrentPlayers;

#define MOVEFIFOSIZ                 2

#define NAM_GRENADE_LIFETIME        120
#define NAM_GRENADE_LIFETIME_VAR    30

#define HORIZ_MIN                   -99
#define HORIZ_MAX                   299
#define AUTO_AIM_ANGLE              48
#define PHEIGHT                     (38<<8)

#define TRIPBOMB_TRIPWIRE       0x00000001
#define TRIPBOMB_TIMER          0x00000002

#define PIPEBOMB_REMOTE         0x00000001
#define PIPEBOMB_TIMER          0x00000002

#define WEAPON_POS_LOWER            -9
#define WEAPON_POS_RAISE            10
#define WEAPON_POS_START             6

enum weaponflags_t {
    WEAPON_SPAWNTYPE1           = 0x00000000, // just spawn
    WEAPON_HOLSTER_CLEARS_CLIP  = 0x00000001, // 'holstering' clears the current clip
    WEAPON_GLOWS                = 0x00000002, // weapon 'glows' (shrinker and grower)
    WEAPON_AUTOMATIC            = 0x00000004, // automatic fire (continues while 'fire' is held down
    WEAPON_FIREEVERYOTHER       = 0x00000008, // during 'hold time' fire every frame
    WEAPON_FIREEVERYTHIRD       = 0x00000010, // during 'hold time' fire every third frame
    WEAPON_RANDOMRESTART        = 0x00000020, // restart for automatic is 'randomized' by RND 3
    WEAPON_AMMOPERSHOT          = 0x00000040, // uses ammo for each shot (for automatic)
    WEAPON_BOMB_TRIGGER         = 0x00000080, // weapon is the 'bomb' trigger
    WEAPON_NOVISIBLE            = 0x00000100, // weapon use does not cause user to become 'visible'
    WEAPON_THROWIT              = 0x00000200, // weapon 'throws' the 'shoots' item...
    WEAPON_CHECKATRELOAD        = 0x00000400, // check weapon availability at 'reload' time
    WEAPON_STANDSTILL           = 0x00000800, // player stops jumping before actual fire (like tripbomb in duke)
    WEAPON_SPAWNTYPE2           = 0x00001000, // spawn like shotgun shells
    WEAPON_SPAWNTYPE3           = 0x00002000, // spawn like chaingun shells
    WEAPON_SEMIAUTO             = 0x00004000, // cancel button press after each shot
    WEAPON_RELOAD_TIMING        = 0x00008000, // special casing for pistol reload sounds
    WEAPON_RESET                = 0x00010000  // cycle weapon back to frame 1 if fire is held, 0 if not
};

enum gamemode_t {
    MODE_MENU                   = 0x00000001,
    MODE_DEMO                   = 0x00000002,
    MODE_GAME                   = 0x00000004,
    MODE_EOL                    = 0x00000008,
    MODE_TYPE                   = 0x00000010,
    MODE_RESTART                = 0x00000020,
    MODE_SENDTOWHOM             = 0x00000040,
};

// Player Actions.
enum playeraction_t {
    pstanding                   = 0x00000001,
    pwalking                    = 0x00000002,
    prunning                    = 0x00000004,
    pducking                    = 0x00000008,
    pfalling                    = 0x00000010,
    pjumping                    = 0x00000020,
    phigher                     = 0x00000040,
    pwalkingback                = 0x00000080,
    prunningback                = 0x00000100,
    pkicking                    = 0x00000200,
    pshrunk                     = 0x00000400,
    pjetpack                    = 0x00000800,
    ponsteroids                 = 0x00001000,
    ponground                   = 0x00002000,
    palive                      = 0x00004000,
    pdead                       = 0x00008000,
    pfacing                     = 0x00010000
};

typedef struct {
    vec3_t pos;
    int16_t ang, sect;
} playerspawn_t;

typedef struct {
    int16_t got_access, last_extra, inv_amount[GET_MAX], curr_weapon, holoduke_on;
    int16_t last_weapon, weapon_pos, kickback_pic;
    int16_t ammo_amount[MAX_WEAPONS], frag[MAXPLAYERS];
    uint16_t gotweapon;
    char inven_icon, jetpack_on, heat_on;
} DukeStatus_t;

typedef struct {
    uint32_t bits;
    int16_t fvel, svel, avel;
    int8_t horz, extbits;
} input_t;

#pragma pack(push,1)
// XXX: r1625 changed a lot types here, among others
//  * int32_t --> int16_t
//  * int16_t --> int8_t
//  * char --> int8_t
// Need to carefully think about implications!
// TODO: rearrange this if the opportunity arises!
// KEEPINSYNC lunatic/_defs_game.lua
typedef struct {
    vec3_t pos, opos, vel, npos;
    vec2_t bobpos, fric;
    int32_t truefz, truecz, player_par;
    int32_t randomflamex, exitx, exity;
    int32_t runspeed, max_player_health, max_shield_amount;
    int32_t autostep, autostep_sbw;

    uint32_t interface_toggle_flag;
#ifdef LUNATIC
    int32_t pipebombControl, pipebombLifetime, pipebombLifetimeVar;
    int32_t tripbombControl, tripbombLifetime, tripbombLifetimeVar;

    int32_t zrange;
    int16_t angrange, autoaimang;
#endif
    uint16_t max_actors_killed, actors_killed;
    uint16_t gotweapon, zoom;

    int16_t loogiex[64], loogiey[64], sbs, sound_pitch;

    int16_t ang, oang, angvel, cursectnum, look_ang, last_extra, subweapon;
    int16_t max_ammo_amount[MAX_WEAPONS], ammo_amount[MAX_WEAPONS], inv_amount[GET_MAX];
    int16_t wackedbyactor, pyoff, opyoff;

    int16_t horiz, horizoff, ohoriz, ohorizoff;
    int16_t newowner, jumping_counter, airleft;
    int16_t fta, ftq, access_wallnum, access_spritenum;
    int16_t got_access, weapon_ang, visibility;
    int16_t somethingonplayer, on_crane, i, one_parallax_sectnum;
    int16_t random_club_frame, one_eighty_count;
    int16_t dummyplayersprite, extra_extra8;
    int16_t actorsqu, timebeforeexit, customexitsound, last_pissed_time;

    int16_t weaprecs[MAX_WEAPONS], weapon_sway, crack_time, bobcounter;

    int16_t orotscrnang, rotscrnang, dead_flag;   // JBF 20031220: added orotscrnang
    int16_t holoduke_on, pycount;
    int16_t transporter_hold, clipdist;

    uint8_t max_secret_rooms, secret_rooms;
    // XXX: 255 values for frag(gedself) seems too small.
    uint8_t frag, fraggedself, quick_kick, last_quick_kick;
    uint8_t return_to_center, reloading, weapreccnt;
    uint8_t aim_mode, auto_aim, weaponswitch, movement_lock, team;
    uint8_t tipincs, hbomb_hold_delay, frag_ps, kickback_pic;

    uint8_t gm, on_warping_sector, footprintcount, hurt_delay;
    uint8_t hbomb_on, jumping_toggle, rapid_fire_hold, on_ground;
    uint8_t inven_icon, buttonpalette, over_shoulder_on, show_empty_weapon;

    uint8_t jetpack_on, spritebridge, lastrandomspot;
    uint8_t scuba_on, footprintpal, heat_on, invdisptime;

    uint8_t holster_weapon, falling_counter, footprintshade;
    uint8_t refresh_inventory, last_full_weapon;

    uint8_t toggle_key_flag, knuckle_incs, knee_incs, access_incs;
    uint8_t walking_snd_toggle, palookup, hard_landing, fist_incs;

    int8_t numloogs, loogcnt, scream_voice;
    int8_t last_weapon, cheat_phase, weapon_pos, wantweaponfire, curr_weapon;

    uint8_t palette;
    palette_t pals;

#ifdef LUNATIC
    int8_t palsfadespeed, palsfadenext, palsfadeprio, padding2_;

    // The player index. Always valid since we have no loose DukePlayer_t's
    // anywhere (like with spritetype_t): g_player[i].ps->wa.idx == i.
    struct { int32_t idx; } wa;
#endif
    int8_t padding_[3];
} DukePlayer_t;

// KEEPINSYNC lunatic/_defs_game.lua
typedef struct {
    DukePlayer_t *ps;
    input_t *inputBits;

    int32_t netsynctime;
    int16_t ping, filler;
    int32_t pcolor, pteam;
    // NOTE: wchoice[HANDREMOTE_WEAPON .. MAX_WEAPONS-1] unused
    uint8_t frags[MAXPLAYERS], wchoice[MAX_WEAPONS];

    char vote, gotvote, pingcnt, playerquitflag, ready;
    char user_name[32];
    uint32_t revision;
} playerdata_t;
#pragma pack(pop)

// KEEPINSYNC lunatic/con_lang.lua
typedef struct
{
    // NOTE: the member names must be identical to aplWeapon* suffixes.
    int32_t WorksLike;  // What the original works like
    int32_t Clip;  // number of items in magazine
    int32_t Reload;  // delay to reload (include fire)
    int32_t FireDelay;  // delay to fire
    int32_t TotalTime;  // The total time the weapon is cycling before next fire.
    int32_t HoldDelay;  // delay after release fire button to fire (0 for none)
    int32_t Flags;  // Flags for weapon
    int32_t Shoots;  // what the weapon shoots
    int32_t SpawnTime;  // the frame at which to spawn an item
    int32_t Spawn;  // the item to spawn
    int32_t ShotsPerBurst;  // number of shots per 'burst' (one ammo per 'burst')
    int32_t InitialSound;  // Sound made when weapon starts firing. zero for no sound
    int32_t FireSound;  // Sound made when firing (each time for automatic)
    int32_t Sound2Time;  // Alternate sound time
    int32_t Sound2Sound;  // Alternate sound sound ID
    int32_t ReloadSound1;  // Sound of magazine being removed
    int32_t ReloadSound2;  // Sound of magazine being inserted
    int32_t SelectSound;  // Sound of weapon being selected
    int32_t FlashColor;  // Muzzle flash color
} weapondata_t;

#ifdef LUNATIC
# define PWEAPON(Player, Weapon, Wmember) (g_playerWeapon[Player][Weapon].Wmember)
extern weapondata_t g_playerWeapon[MAXPLAYERS][MAX_WEAPONS];
#else
# define PWEAPON(Player, Weapon, Wmember) (aplWeapon ## Wmember [Weapon][Player])
extern intptr_t         *aplWeaponClip[MAX_WEAPONS];            // number of items in clip
extern intptr_t         *aplWeaponReload[MAX_WEAPONS];          // delay to reload (include fire)
extern intptr_t         *aplWeaponFireDelay[MAX_WEAPONS];       // delay to fire
extern intptr_t         *aplWeaponHoldDelay[MAX_WEAPONS];       // delay after release fire button to fire (0 for none)
extern intptr_t         *aplWeaponTotalTime[MAX_WEAPONS];       // The total time the weapon is cycling before next fire.
extern intptr_t         *aplWeaponFlags[MAX_WEAPONS];           // Flags for weapon
extern intptr_t         *aplWeaponShoots[MAX_WEAPONS];          // what the weapon shoots
extern intptr_t         *aplWeaponSpawnTime[MAX_WEAPONS];       // the frame at which to spawn an item
extern intptr_t         *aplWeaponSpawn[MAX_WEAPONS];           // the item to spawn
extern intptr_t         *aplWeaponShotsPerBurst[MAX_WEAPONS];   // number of shots per 'burst' (one ammo per 'burst'
extern intptr_t         *aplWeaponWorksLike[MAX_WEAPONS];       // What original the weapon works like
extern intptr_t         *aplWeaponInitialSound[MAX_WEAPONS];    // Sound made when initialy firing. zero for no sound
extern intptr_t         *aplWeaponFireSound[MAX_WEAPONS];       // Sound made when firing (each time for automatic)
extern intptr_t         *aplWeaponSound2Time[MAX_WEAPONS];      // Alternate sound time
extern intptr_t         *aplWeaponSound2Sound[MAX_WEAPONS];     // Alternate sound sound ID
extern intptr_t         *aplWeaponReloadSound1[MAX_WEAPONS];    // Sound of magazine being removed
extern intptr_t         *aplWeaponReloadSound2[MAX_WEAPONS];    // Sound of magazine being inserted
extern intptr_t         *aplWeaponSelectSound[MAX_WEAPONS];     // Sound for weapon selection
extern intptr_t         *aplWeaponFlashColor[MAX_WEAPONS];      // Color for polymer muzzle flash
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
    int8_t filler[2]; // 2b
    int32_t userdata; // 4b
} projectile_t;

// KEEPINSYNC lunatic/_defs_game.lua
typedef struct {
    int32_t cur, count;  // "cur" is the only member that is *used*
    int32_t gunposx, lookhalfang;  // weapon_xoffset, ps->look_ang>>1
    int32_t gunposy, lookhoriz;  // gun_pos, looking_arc
    int32_t shade;
} hudweapon_t;

extern input_t          inputfifo[MOVEFIFOSIZ][MAXPLAYERS];
extern playerspawn_t    g_playerSpawnPoints[MAXPLAYERS];
extern playerdata_t     *const g_player;
extern int16_t          WeaponPickupSprites[MAX_WEAPONS];
extern hudweapon_t      hudweap;
extern int32_t          g_levelTextTime;
extern int32_t          g_myAimMode;
extern int32_t          g_numObituaries;
extern int32_t          g_numSelfObituaries;
extern int32_t          g_emuJumpTics;
extern int32_t          mouseyaxismode;
extern int32_t          ticrandomseed;

#define SHOOT_HARDCODED_ZVEL INT32_MIN

int A_ShootWithZvel(int spriteNum, int projecTile, int forceZvel);
static inline int A_Shoot(int spriteNum, int projecTile)
{
    return A_ShootWithZvel(spriteNum, projecTile, SHOOT_HARDCODED_ZVEL);
}

static inline void P_PalFrom(DukePlayer_t *pPlayer, uint8_t f, uint8_t r, uint8_t g, uint8_t b)
{
#ifdef LUNATIC
    // Compare with _defs_game.lua: player[]:_palfrom().
    if (pPlayer->pals.f == 0 || pPlayer->palsfadeprio <= 0)
#endif
    {
        pPlayer->pals.f = f;
        pPlayer->pals.r = r;
        pPlayer->pals.g = g;
        pPlayer->pals.b = b;
#ifdef LUNATIC
        pPlayer->palsfadespeed = pPlayer->palsfadenext = 0;
#endif
    }
}

void    P_AddKills(DukePlayer_t * const pPlayer, uint16_t kills);
int32_t A_GetHitscanRange(int spriteNum);
void    P_GetInput(int playerNum);
void P_AddAmmo(DukePlayer_t * const pPlayer, int const weaponNum, int const addAmount);
void    P_AddWeapon(DukePlayer_t *pPlayer, int weaponNum, int switchWeapon);
void    P_CheckWeapon(DukePlayer_t *pPlayer);
void    P_DisplayScuba(void);
void    P_DisplayWeapon(void);
void P_DropWeapon(int const playerNum);
int     P_FindOtherPlayer(int playerNum, int32_t *d);
void    P_FragPlayer(int playerNum);
void    P_UpdatePosWhenViewingCam(DukePlayer_t *pPlayer);
void    P_ProcessInput(int playerNum);
void    P_QuickKill(DukePlayer_t *pPlayer);
void    P_SelectNextInvItem(DukePlayer_t *pPlayer);
void    P_UpdateScreenPal(DukePlayer_t *pPlayer);
void    P_EndLevel(void);
void    P_CheckWeaponI(int playerNum);
int     P_GetHudPal(const DukePlayer_t *pPlayer);

int Proj_GetDamage(projectile_t const *pProj);

#if !defined LUNATIC
void P_SetWeaponGamevars(int playerNum, const DukePlayer_t *pPlayer);
#else
static inline void P_SetWeaponGamevars(int playerNum, const DukePlayer_t *pPlayer)
{
    UNREFERENCED_PARAMETER(playerNum);
    UNREFERENCED_PARAMETER(pPlayer);
}
#endif

// Get the player index given an APLAYER sprite pointer.
static inline int P_GetP(const void *pSprite)
{
#if 0  // unprotected player index retrieval
    return spr->yvel;
#elif defined NETCODE_DISABLE
    UNREFERENCED_PARAMETER(pSprite);  // for NDEBUG build
    // NOTE: In the no-netcode build, there's no point to pass player indices
    // at all since there is ever only one player. However, merely returning 0
    // would mean making this build less strict than the normal one.
    Bassert(((const uspritetype *)pSprite)->yvel == 0);
    return 0;
#else
    int playerNum = ((const uspritetype *)pSprite)->yvel;
    // [JM] Check against MAXPLAYERS as opposed to g_mostConcurrentPlayers
    //      to prevent CON for disconnected/fake players from executing as playernum 0.
    if ((unsigned)playerNum >= MAXPLAYERS)
        playerNum = 0;
    return playerNum;
#endif
}

// Get the player index given an APLAYER sprite index.
static inline int P_Get(int32_t spriteNum) { return P_GetP((const uspritetype *)&sprite[spriteNum]); }

#ifdef __cplusplus
}
#endif

#endif
