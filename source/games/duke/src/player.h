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
    int16_t firstaid_amount;
    int16_t steroids_amount;
    int16_t holoduke_amount;
    int16_t jetpack_amount;
    int16_t heat_amount;
    int16_t scuba_amount;
    int16_t boot_amount;
    int16_t shield_amount;

    int16_t got_access, last_extra, curr_weapon, holoduke_on;
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

typedef struct player_struct 
{
    // This is basically the version from JFDuke but this first block contains a few changes to make it work with other parts of Raze.
    
    // The sound code wants to read a vector out of this so we need to define one for the main coordinate.
    union
    {
        vec3_t pos;
        struct { int32_t posx, posy, posz; };
    };

    // input handles angle and horizon as fixed16 numbers. We need to account for that as well.
    fixed_t q16ang, oq16ang, q16horiz, q16horizoff; // oq16horiz, oq16horizoff; // These two are currently not used but may be again later.

    // using a bit field for this to save a bit of space.
    FixedBitArray<MAX_WEAPONS> gotweapon;

    // Palette management uses indices into the engine's palette table now.
    unsigned int palette;
    PalEntry pals;

    // these did not exist in JFDuke.
    uint8_t movement_lock;
    vec2_t fric;

    // From here on it is unaltered from JFDuke with the exception of a few fields that are no longer needed and were removed.
    int zoom, exitx, exity, loogiex[64], loogiey[64], numloogs, loogcnt;
    int invdisptime;
    int bobposx, bobposy, oposx, oposy, oposz, pyoff, opyoff;
    int posxv, posyv, poszv, last_pissed_time, truefz, truecz;
    int player_par, visibility;
    int bobcounter, weapon_sway;
    int pals_time, randomflamex, crack_time;

    int aim_mode, auto_aim, weaponswitch;

    short angvel, cursectnum, look_ang, last_extra, subweapon;
    short ammo_amount[MAX_WEAPONS], wackedbyactor, frag, fraggedself;

    short curr_weapon, last_weapon, tipincs, wantweaponfire;
    short holoduke_amount, newowner, hurt_delay, hbomb_hold_delay;
    short jumping_counter, airleft, knee_incs, access_incs;
    short ftq, access_wallnum, access_spritenum;
    short kickback_pic, got_access, weapon_ang, firstaid_amount;
    short somethingonplayer, on_crane, i, one_parallax_sectnum;
    short over_shoulder_on, random_club_frame, fist_incs;
    short one_eighty_count, cheat_phase;
    short dummyplayersprite, extra_extra8, quick_kick;
    short heat_amount, actorsqu, timebeforeexit, customexitsound;

    short weaprecs[256], weapreccnt;
    unsigned int interface_toggle_flag;

    short orotscrnang, rotscrnang, dead_flag, show_empty_weapon;	// JBF 20031220: added orotscrnang
    short scuba_amount, jetpack_amount, steroids_amount, shield_amount;
    short holoduke_on, pycount, weapon_pos, frag_ps;
    short transporter_hold, last_full_weapon, footprintshade, boot_amount;

    int scream_voice;

    unsigned char gm;
    unsigned char on_warping_sector, footprintcount;
    unsigned char hbomb_on, jumping_toggle, rapid_fire_hold, on_ground;
    char name[32];
    unsigned char inven_icon, buttonpalette;

    unsigned char jetpack_on, spritebridge, lastrandomspot;
    unsigned char scuba_on, footprintpal, heat_on;

    unsigned char  holster_weapon;
    unsigned char falling_counter;
    unsigned char refresh_inventory;

    unsigned char toggle_key_flag, knuckle_incs; // ,select_dir;
    unsigned char walking_snd_toggle, palookup, hard_landing;
    unsigned char return_to_center;

    int max_secret_rooms, secret_rooms, max_actors_killed, actors_killed;

    // Redneck Rampage additions. Those which did not have names in the reconstructed source got one from either RedneckGDX or RedNukem.
    // Items were reordered by size.
    int stairs;
    int detonate_count; // at57e
    int noise_x, noise_y, noise_radius; // at286, at28a, at290
    int drink_timer; // at58e
    int eat_timer; // at592
    int SlotWin;
    short recoil;
    short detonate_time; // at57c
    short yehaa_timer;
    short drink_amt, eat, drunkang, eatang;
    uint8_t shotgun_state[2];
    uint8_t donoise; // at28e
    uint8_t keys[5];

    // RRRA. The same as for the RR block applies.
    int drug_aspect;
    int drug_timer;
    int SeaSick;
    short MamaEnd; // raat609
    short MotoSpeed, TiltStatus, moto_drink;
    short VBumpNow, VBumpTarget, TurbCount;
    short drug_stat[3]; // raat5f1..5
    uint8_t DrugMode, lotag800kill;
    uint8_t sea_sick_stat; // raat5dd
    uint8_t hurt_delay2, nocheat;
    uint8_t OnMotorcycle, OnBoat, moto_underwater, NotOnWater, MotoOnGround;
    uint8_t moto_do_bump, moto_bump_fast, moto_on_oil, moto_on_mud;

    int8_t crouch_toggle;

    // Access helpers for the widened angle and horizon fields.
    int getlookang() { return look_ang; }
    void setlookang(int b) { look_ang = b; }
    void addlookang(int b) { look_ang += b; }
    int getrotscrnang() { return rotscrnang; }
    void setrotscrnang(int b) { rotscrnang = b; }
    void addrotscrnang(int b) { rotscrnang += b; }
    int getang() { return q16ang >> FRACBITS; }
    int getoang() { return oq16ang >> FRACBITS; }
    void setang(int v) { q16ang = v << FRACBITS; }
    void addang(int v) { q16ang = (q16ang + (v << FRACBITS)) & ((2048 << FRACBITS) - 1); }
    void setoang(int v) { oq16ang = v << FRACBITS; }
    void addhoriz(int v) { q16horiz += (v << FRACBITS); }
    void addhorizoff(int v) { q16horiz += (v << FRACBITS); }
    void sethoriz(int v) { q16horiz = (v << FRACBITS); }
    void sethorizoff(int v) { q16horizoff = (v << FRACBITS); }
    int gethoriz() { return q16horiz >> FRACBITS; }
    int gethorizof() { return q16horizoff >> FRACBITS; }
    int gethorizsum() { return (q16horiz + q16horizoff) >> FRACBITS; }

} DukePlayer_t;


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
    pPlayer->pals = pe;
}

int hitawall(DukePlayer_t* pPlayer, int* hitWall);
int hits(int spriteNum);
void    P_GetInput(int playerNum);
void    P_GetInputMotorcycle(int playerNum);
void    P_GetInputBoat(int playerNum);

extern int16_t max_ammo_amount[MAX_WEAPONS];


extern int lastvisinc;

END_DUKE_NS

#endif
