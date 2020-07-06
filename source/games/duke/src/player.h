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

#include "names.h"
#include "fix16.h"
#include "net.h"
#include "tarray.h"
#include "constants.h"

BEGIN_DUKE_NS

extern int32_t playerswhenstarted;

#define MOVEFIFOSIZ                 256

#define HORIZ_MIN                   -99
#define HORIZ_MAX                   299
#define AUTO_AIM_ANGLE              48
#define PHEIGHT_DUKE                (38<<8)
#define PHEIGHT_RR                  (40<<8);
extern int32_t PHEIGHT;

enum
{
    // Control flags for WW2GI weapons.
    TRIPBOMB_TRIPWIRE = 1,
    TRIPBOMB_TIMER = 2
};



#define WEAPON_POS_LOWER            -9
#define WEAPON_POS_RAISE            10
#define WEAPON_POS_START             6

#define MAX_WEAPON_RECS             256

enum gamemode_t {
    MODE_MENU                   = 0x00000001,
    MODE_DEMO                   = 0x00000002,
    MODE_GAME                   = 0x00000004,
    MODE_EOL                    = 0x00000008,
    MODE_TYPE                   = 0x00000010,
    MODE_RESTART                = 0x00000020,
    MODE_SENDTOWHOM             = 0x00000040,
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
    ESyncBits bits;
    int16_t fvel, svel;
    fix16_t q16avel, q16horz;
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
    int16_t /*fta,*/ ftq, access_wallnum, access_spritenum;
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
    uint8_t make_noise; // at28e
    int32_t noise_x, noise_y, noise_radius; // at286, at28a, at290
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

    int32_t pcolor, pteam;
    // NOTE: wchoice[HANDREMOTE_WEAPON .. MAX_WEAPONS-1] unused
    uint8_t frags[MAXPLAYERS];

    char playerreadyflag, playerquitflag, connected;
    char user_name[32];
    char syncval[SYNCFIFOSIZ][MAXSYNCBYTES];
    double  lastInputTicks;

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

# define PWEAPON(Player, Weapon, Wmember) (aplWeapon ## Wmember [Weapon][Player])

// KEEPINSYNC lunatic/_defs_game.lua
typedef struct {
    int32_t cur, count;  // "cur" is the only member that is *used*
    int32_t gunposx, lookhalfang;  // weapon_xoffset, ps->look_ang>>1
    int32_t gunposy, lookhoriz;  // gun_pos, looking_arc
    int32_t shade;
} hudweapon_t;

extern playerspawn_t    g_playerSpawnPoints[MAXPLAYERS];
extern playerdata_t     *const g_player;
extern hudweapon_t      hudweap;
extern int32_t          mouseyaxismode;


inline void SetPlayerPal(DukePlayer_t* pPlayer, PalEntry pe)
{
    pPlayer->pals.f = pe.a;
    pPlayer->pals.r = pe.r;
    pPlayer->pals.g = pe.g;
    pPlayer->pals.b = pe.b;
}

int hitawall(DukePlayer_t* pPlayer, int* hitWall);
int hits(int spriteNum);
void    P_GetInput(int playerNum);
void    P_GetInputMotorcycle(int playerNum);
void    P_GetInputBoat(int playerNum);
void checkweapons(DukePlayer_t* const pPlayer);
int findotherplayer(int p, int* d);
void quickkill(DukePlayer_t* pPlayer);
void setpal(DukePlayer_t* pPlayer);
int madenoise(int playerNum);
int haskey(int sect, int snum);

extern int16_t max_ammo_amount[MAX_WEAPONS];

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
void playerLookUp(int snum, ESyncBits sb_snum);
void playerLookDown(int snum, ESyncBits sb_snum);
void playerAimUp(int snum, ESyncBits sb_snum);
void playerAimDown(int snum, ESyncBits sb_snum);
bool view(struct player_struct* pp, int* vx, int* vy, int* vz, short* vsectnum, int ang, int horiz);

extern int lastvisinc;

END_DUKE_NS

#endif
