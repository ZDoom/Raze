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
#include "namesdyn.h"
#include "fix16.h"
#include "net.h"
#include "tarray.h"

BEGIN_DUKE_NS

extern int32_t g_mostConcurrentPlayers;

#define MOVEFIFOSIZ                 256

#define HORIZ_MIN                   -99
#define HORIZ_MAX                   299
#define AUTO_AIM_ANGLE              48
#define PHEIGHT_DUKE                (38<<8)
#define PHEIGHT_RR                  (40<<8);
extern int32_t PHEIGHT;

enum
{
    PIPEBOMB_REMOTE = 0x00000001,
    PIPEBOMB_TIMER = 0x00000002,
    TRIPBOMB_TRIPWIRE = 1,
    TRIPBOMB_TIMER = 2
};



#define WEAPON_POS_LOWER            -9
#define WEAPON_POS_RAISE            10
#define WEAPON_POS_START             6

#define MAX_WEAPON_RECS             256

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
    union
    {
        vec3_t pos;
        struct { int ox, oy, oz; };
    };
    union
    {
        int16_t oa;
        int16_t ang;
    };
    union
    {
        int16_t sect;
        int16_t os;
    };
} playerspawn_t;

typedef struct STATUSBARTYPE {
    int16_t got_access, last_extra, inv_amount[GET_MAX], curr_weapon, holoduke_on;
    int16_t last_weapon, weapon_pos, kickback_pic;
    int16_t ammo_amount[MAX_WEAPONS], frag[MAXPLAYERS];
    FixedBitArray<MAX_WEAPONS> gotweapon;
    char inven_icon, jetpack_on, heat_on;
} DukeStatus_t;

typedef struct {
    uint32_t bits;
    int16_t fvel, svel;
    fix16_t q16avel, q16horz;
    int8_t extbits;
} input_t;

#pragma pack(push,1)
// XXX: r1625 changed a lot types here, among others
//  * int32_t --> int16_t
//  * int16_t --> int8_t
//  * char --> int8_t
// Need to carefully think about implications!
// TODO: rearrange this if the opportunity arises!
// KEEPINSYNC lunatic/_defs_game.lua
typedef struct player_struct {
    union
    {
        vec3_t pos;
        struct { int32_t posx, posy, posz; };
    };
    union
    {
        vec3_t opos;
        struct { int32_t oposx, oposy, oposz; };
    };

    union
    {
        vec3_t vel;
        struct { int32_t posxv, posyv, poszv; };
    };
    vec3_t npos;
    union
    {
        vec2_t bobpos;
        struct { int32_t bobposx, bobposy; };
    };

    vec2_t fric;

    fix16_t q16horiz, q16horizoff;
    fix16_t q16ang, oq16ang;
    int look_ang;
    int16_t orotscrnang, rotscrnang;   // JBF 20031220: added orotscrnang

    int getlookang() { return look_ang; }
    void setlookang(int b) { look_ang = b; }
    void addlookang(int b) { look_ang += b; }
    int getrotscrnang() { return rotscrnang; }
    void setrotscrnang(int b) { rotscrnang = b; }
    void addrotscrnang(int b) { rotscrnang += b; }
    int getang() { return q16ang >> FRACBITS; }
    int getoang() { return oq16ang >> FRACBITS; }
    void setang(int v) { q16ang = v << FRACBITS; }
    void addang(int v) { q16ang = (q16ang + (v << FRACBITS)) & ((2048 << FRACBITS)-1); }
    void setoang(int v) { oq16ang = v << FRACBITS; }
    void addhoriz(int v) { q16horiz += (v << FRACBITS); }
    void addhorizoff(int v) { q16horiz += (v << FRACBITS); }
    void sethoriz(int v) { q16horiz = (v << FRACBITS); }
    void sethorizoff(int v) { q16horizoff = (v << FRACBITS); }
    int gethoriz() { return q16horiz >> FRACBITS; }
    int gethorizof() { return q16horizoff >> FRACBITS; }
    int gethorizsum() { return (q16horiz + q16horizoff) >> FRACBITS; }

    int32_t truefz, truecz, player_par;
    int32_t randomflamex, exitx, exity;
    int32_t runspeed;

    uint32_t interface_toggle_flag;
    uint16_t max_actors_killed, actors_killed;
    FixedBitArray<MAX_WEAPONS> gotweapon;
    uint16_t zoom;

    int16_t loogiex[64], loogiey[64], sbs, sound_pitch;

    int16_t cursectnum, last_extra, subweapon;
    int16_t ammo_amount[MAX_WEAPONS], inv_amount[GET_MAX];
    int16_t wackedbyactor, pyoff, opyoff;

    int16_t newowner, jumping_counter, airleft;
    int16_t fta, ftq, access_wallnum, access_spritenum;
    int16_t got_access, weapon_ang, visibility;
    int16_t somethingonplayer, on_crane, i, one_parallax_sectnum;
    int16_t random_club_frame, one_eighty_count;
    int16_t dummyplayersprite, extra_extra8;
    int16_t actorsqu, timebeforeexit, customexitsound, last_pissed_time;

    int16_t weaprecs[MAX_WEAPON_RECS], weapon_sway, crack_time, bobcounter;

    int16_t dead_flag;
    int16_t holoduke_on, pycount;
    int16_t transporter_hold/*, clipdist*/;

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

    int8_t last_used_weapon;

    int16_t recoil;
    int32_t stairs;
    int32_t detonate_count;
    int16_t detonate_time;
    uint8_t shotgun_state[2];
    uint8_t make_noise;
    int32_t noise_x, noise_y, noise_radius;
    uint8_t keys[5];
    int16_t yehaa_timer;
    int16_t drink_amt, eat, drunkang, eatang;
    int32_t drink_timer, eat_timer;
    int16_t MamaEnd;
    int16_t MotoSpeed, TiltStatus, moto_drink;
    uint8_t OnMotorcycle, OnBoat, moto_underwater, NotOnWater, MotoOnGround;
    uint8_t moto_do_bump, moto_bump_fast, moto_on_oil, moto_on_mud;
    int16_t VBumpNow, VBumpTarget, TurbCount;
    int16_t drug_stat[3];
    int32_t drug_aspect;
    uint8_t DrugMode, lotag800kill, sea_sick_stat;
    int32_t drug_timer;
    int32_t sea_sick;
    uint8_t hurt_delay2, nocheat;

    int32_t dhat60f, dhat613, dhat617, dhat61b, dhat61f;

    int8_t crouch_toggle;
    int SlotWin;
    int8_t padding_[3];
} DukePlayer_t;

// transition helpers
#define SeaSick sea_sick
#define firstaid_amount inv_amount[GET_FIRSTAID]
#define steroids_amount inv_amount[GET_STEROIDS]
#define holoduke_amount inv_amount[GET_HOLODUKE]
#define jetpack_amount inv_amount[GET_JETPACK]
#define heat_amount inv_amount[GET_HEATS]
#define scuba_amount inv_amount[GET_SCUBA]
#define boot_amount inv_amount[GET_BOOTS]
#define shield_amount inv_amount[GET_SHIELD]
#define raat609 MamaEnd
#define raat5dd sea_sick_stat
#define at57e detonate_count
#define at57c detonate_time
#define at58e drink_timer
#define at592 eat_timer
#define raat5f1 drug_stat[0]
#define raat5f3 drug_stat[1]
#define raat5f5 drug_stat[2]

// KEEPINSYNC lunatic/_defs_game.lua
typedef struct
{
    DukePlayer_t *ps;
    input_t *input;

    bool    horizRecenter;
    float   horizAngleAdjust;
    fix16_t horizSkew;

    int32_t movefifoend, syncvalhead, myminlag;
    int32_t pcolor, pteam;
    // NOTE: wchoice[HANDREMOTE_WEAPON .. MAX_WEAPONS-1] unused
    uint8_t frags[MAXPLAYERS];

    char vote, gotvote, playerreadyflag, playerquitflag, connected;
    char user_name[32];
    char syncval[SYNCFIFOSIZ][MAXSYNCBYTES];

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
    int32_t FlashColor;  // Muzzle flash color
} weapondata_t;

#ifdef LUNATIC
# define PWEAPON(Player, Weapon, Wmember) (g_playerWeapon[Player][Weapon].Wmember)
extern weapondata_t g_playerWeapon[MAXPLAYERS][MAX_WEAPONS];
#else
# define PWEAPON(Player, Weapon, Wmember) (aplWeapon ## Wmember [Weapon][Player])
#endif

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
extern int32_t          g_numObituaries;
extern int32_t          g_numSelfObituaries;
extern int32_t          mouseyaxismode;
extern int32_t          ticrandomseed;

#define SHOOT_HARDCODED_ZVEL INT32_MIN

static inline void P_PalFrom(DukePlayer_t *pPlayer, uint8_t f, uint8_t r, uint8_t g, uint8_t b)
{
    pPlayer->pals.f = f;
    pPlayer->pals.r = r;
    pPlayer->pals.g = g;
    pPlayer->pals.b = b;
}

inline void SetPlayerPal(DukePlayer_t* pPlayer, PalEntry pe)
{
    pPlayer->pals.f = pe.a;
    pPlayer->pals.r = pe.r;
    pPlayer->pals.g = pe.g;
    pPlayer->pals.b = pe.b;
}

int hitawall(DukePlayer_t* pPlayer, int* hitWall);
void    P_AddKills(DukePlayer_t * pPlayer, uint16_t kills);
int hits(int spriteNum);
void    P_GetInput(int playerNum);
void    P_GetInputMotorcycle(int playerNum);
void    P_GetInputBoat(int playerNum);
void P_AddAmmo(DukePlayer_t * pPlayer, int weaponNum, int addAmount);
inline void addammo(int weaponNum, DukePlayer_t* pPlayer, int addAmount)
{
    P_AddAmmo(pPlayer, weaponNum, addAmount);
}
void    P_CheckWeapon(DukePlayer_t *pPlayer);
void    P_DisplayWeapon(void);
void checkweapons(DukePlayer_t* const pPlayer);
int findotherplayer(int p, int* d);
void    P_FragPlayer(int playerNum);
void    P_UpdatePosWhenViewingCam(DukePlayer_t *pPlayer);
void    P_ProcessInput(int playerNum);
void    P_DHProcessInput(int playerNum);
void quickkill(DukePlayer_t* pPlayer);
void    P_UpdateScreenPal(DukePlayer_t *pPlayer);
inline void setpal(DukePlayer_t* pPlayer)
{
    P_UpdateScreenPal(pPlayer);
}

void    P_EndLevel(void);
void    P_CheckWeaponI(int playerNum);
int     P_GetOverheadPal(const DukePlayer_t *pPlayer);
void madenoise(int playerNum);
int haskey(int sect, int snum);

// Get the player index given an TILE_APLAYER sprite pointer.
static inline int P_GetP(const void *pSprite)
{
    return ((const uspritetype*)pSprite)->yvel;
}

// Get the player index given an TILE_APLAYER sprite index.
static inline int P_Get(int32_t spriteNum) { return P_GetP((const uspritetype *)&sprite[spriteNum]); }
extern int16_t max_ammo_amount[MAX_WEAPONS];

void P_SetWeaponGamevars(int playerNum, const DukePlayer_t* const pPlayer);

void tracers(int x1, int y1, int z1, int x2, int y2, int z2, int n);
int hits(int i);
int hitasprite(int i, short* hitsp);
int aim(spritetype* s, int aang);

int timedexit(int snum);
void dokneeattack(int snum, int pi, const std::initializer_list<int>& respawnlist);
int endoflevel(int snum);
void playerisdead(int snum, int psectlotag, int fz, int cz);
void footprints(int snum);
int makepainsounds(int snum, int type);
void playerCrouch(int snum);
void playerJump(int snum, int fz, int cz);
void playerLookLeft(int snum);
void playerLookRight(int snum);
void playerCenterView(int snum);
void playerLookUp(int snum, int sb_snum);
void playerLookDown(int snum, int sb_snum);
void playerAimUp(int snum, int sb_snum);
void playerAimDown(int snum, int sb_snum);

extern int lastvisinc;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

inline void hud_rotatesprite(int sx, int sy, int z, int16_t a, int16_t picnum, int8_t dashade, uint8_t dapalnum, int dastat, int cx1, int cy1, int cx2, int cy2)
{
    twod_rotatesprite(&twodpsp, sx, sy, z, a, picnum, dashade, dapalnum, dastat, 0, 0, cx1, cy1, cx2, cy2);
}

END_DUKE_NS

#endif
