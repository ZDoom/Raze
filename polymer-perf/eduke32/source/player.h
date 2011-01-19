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

#ifndef __player_h__
#define __player_h__

#define MOVEFIFOSIZ                 2

#define NAM_GRENADE_LIFETIME        120
#define NAM_GRENADE_LIFETIME_VAR    30

#define HORIZ_MIN                   -99
#define HORIZ_MAX                   299
#define AUTO_AIM_ANGLE              48
#define PHEIGHT                     (38<<8)

enum dukeinv_t {
    GET_STEROIDS,
    GET_SHIELD,
    GET_SCUBA,
    GET_HOLODUKE,
    GET_JETPACK,
    GET_DUMMY1,
    GET_ACCESS,
    GET_HEATS,
    GET_DUMMY2,
    GET_FIRSTAID,
    GET_BOOTS,
    GET_MAX
};

enum dukeweapon_t {
    KNEE_WEAPON,
    PISTOL_WEAPON,
    SHOTGUN_WEAPON,
    CHAINGUN_WEAPON,
    RPG_WEAPON,
    HANDBOMB_WEAPON,
    SHRINKER_WEAPON,
    DEVISTATOR_WEAPON,
    TRIPBOMB_WEAPON,
    FREEZE_WEAPON,
    HANDREMOTE_WEAPON,
    GROW_WEAPON,
    MAX_WEAPONS
};

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

#define TRIPBOMB_TRIPWIRE       0x00000001
#define TRIPBOMB_TIMER          0x00000002

#define PIPEBOMB_REMOTE         0x00000001
#define PIPEBOMB_TIMER          0x00000002

#pragma pack(push,1)
typedef struct {
    int32_t ox,oy,oz;
    int16_t oa,os;
} playerspawn_t;

typedef struct {
    int16_t got_access, last_extra, inv_amount[GET_MAX], curr_weapon, holoduke_on;
    int16_t last_weapon, weapon_pos, kickback_pic;
    int16_t ammo_amount[MAX_WEAPONS], frag[MAXPLAYERS];
    uint16_t gotweapon;
    char inven_icon, jetpack_on, heat_on;
} DukeStatus_t;

typedef struct {
    vec3_t pos, opos, posvel;
    int32_t bobposx, bobposy;
    int32_t truefz, truecz, player_par;
    int32_t randomflamex, exitx, exity;
    int32_t runspeed, max_player_health, max_shield_amount;

    uint32_t interface_toggle_flag;

    uint8_t palette;

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

    uint8_t max_secret_rooms, secret_rooms;
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

    int8_t numloogs, loogcnt, scream_voice, transporter_hold;
    int8_t last_weapon, cheat_phase, weapon_pos, wantweaponfire, curr_weapon;

    palette_t pals;

    char name[32];
} DukePlayer_t;

typedef struct {
    uint32_t bits; // 4b
    int16_t fvel, svel; // 4b
    int8_t avel, horz; // 2b
    int8_t extbits, filler; // 2b
} input_t;

typedef struct {
    DukePlayer_t *ps;
    input_t *sync;

    int32_t netsynctime;
    int16_t ping, filler;
    int32_t pcolor, pteam;
    uint8_t frags[MAXPLAYERS], wchoice[MAX_WEAPONS];

    char vote, gotvote, playerreadyflag, playerquitflag;
    char user_name[32];
} playerdata_t;
#pragma pack(pop)

extern char             g_numPlayerSprites;
extern int32_t          fricxv,fricyv;

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

#pragma pack(push,1)
extern input_t          inputfifo[MOVEFIFOSIZ][MAXPLAYERS];
extern playerspawn_t    g_playerSpawnPoints[MAXPLAYERS];
extern playerdata_t     g_player[MAXPLAYERS];
#pragma pack(pop)
extern char             dashow2dsector[(MAXSECTORS+7)>>3];
extern int16_t          searchsect[MAXSECTORS],searchparent[MAXSECTORS];
extern int16_t          WeaponPickupSprites[MAX_WEAPONS];
extern int32_t          g_currentweapon;
extern int32_t          g_gs;
extern int32_t          g_gun_pos;
extern int32_t          g_kb;
extern int32_t          g_levelTextTime;
extern int32_t          g_looking_angSR1;
extern int32_t          g_looking_arc;
extern int32_t          g_myAimMode;
extern int32_t          g_numObituaries;
extern int32_t          g_numSelfObituaries;
extern int32_t          g_weapon_offset;
extern int32_t          g_weapon_xoffset;
extern int32_t          jump_timer;
extern int32_t          lastvisinc;
extern int32_t          mouseyaxismode;
extern int32_t          ticrandomseed;

int32_t     A_GetHitscanRange(int32_t i);
int32_t     A_Shoot(int32_t i,int32_t atwith);
void        computergetinput(int32_t snum,input_t *syn);
void        getinput(int32_t snum);
int32_t     getspritescore(int32_t snum,int32_t dapicnum);
void        P_AddAmmo(int32_t weapon,DukePlayer_t *p,int32_t amount);
void        P_AddWeapon(DukePlayer_t *p,int32_t weapon);
void        P_AddWeaponNoSwitch(DukePlayer_t *p,int32_t weapon);
int32_t     P_CheckFloorDamage(DukePlayer_t *p,int32_t j);
void        P_CheckTouchDamage(DukePlayer_t *p,int32_t j);
void        P_CheckWeapon(DukePlayer_t *p);
void        P_DisplayScuba(int32_t snum);
void        P_DisplayWeapon(int32_t snum);
int32_t     P_DoFist(DukePlayer_t *p);
void        P_DoWeaponSpawn(DukePlayer_t *p);
void        P_DropWeapon(DukePlayer_t *p);
int32_t     P_FindOtherPlayer(int32_t p,int32_t *d);
void        P_FireWeapon(DukePlayer_t *p);
void        P_FragPlayer(int32_t snum);
void        P_ProcessInput(int32_t snum);
void        P_ProcessWeapon(int32_t snum);
void        P_QuickKill(DukePlayer_t *p);
void        P_SelectNextInvItem(DukePlayer_t *p);
void        P_UpdateScreenPal(DukePlayer_t *p);
#endif

