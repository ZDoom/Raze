#pragma once

BEGIN_DUKE_NS

// all the struct types from JFDuke's duke3d.h

struct STATUSBARTYPE 
{
    short frag[MAXPLAYERS], got_access, last_extra, shield_amount, curr_weapon;
    short ammo_amount[MAX_WEAPONS], holoduke_on;
    unsigned char inven_icon, jetpack_on, heat_on;
    short firstaid_amount, steroids_amount, holoduke_amount, jetpack_amount;
    short heat_amount, scuba_amount, boot_amount;
    short last_weapon, weapon_pos, kickback_pic;
    FixedBitArray<MAX_WEAPONS> gotweapon; // must be the same type as in player_struct
};

struct weaponhit
{
	uint8_t cgg;
	short picnum, ang, extra, owner, movflag;
	short tempang, actorstayput, dispicnum;
	short timetosleep;
	int floorz, ceilingz, lastvx, lastvy, bposx, bposy, bposz, aflags;
	int temp_data[6];
};


// Todo - put more state in here
struct ActorInfo
{
	uint32_t scriptaddress;
	uint32_t flags;
	int aimoffset;
};

struct input_t  // original name was input which is too generic for a type name.
{
    fixed_t q16avel, q16horz; // These were expanded to 16.16 fixed point.
    short fvel, svel;
    ESyncBits bits;
};

struct user_defs
{
    unsigned char god, cashman, eog, showallmap;
    unsigned char show_help, scrollmode, clipping;
    char user_name[MAXPLAYERS][32];
    unsigned char overhead_on, last_overhead, showweapons;
    unsigned char user_pals[MAXPLAYERS];

    short from_bonus;
    short camerasprite, last_camsprite;
    short last_level, secretlevel;

    int const_visibility;
    int folfvel, folavel, folx, foly, fola;
    int reccnt;

    int runkey_mode, statusbarscale, mouseaiming, weaponswitch;

    int entered_name, shadows, executions, auto_run;
    int coords, tickrate, levelstats, m_coop, coop, screen_size, lockout, crosshair;
    int wchoice[MAXPLAYERS][MAX_WEAPONS], playerai;

    int respawn_monsters, respawn_items, respawn_inventory, recstat, monsters_off, brightness;
    int m_respawn_items, m_respawn_monsters, m_respawn_inventory, m_recstat, m_monsters_off, detail;
    int m_ffire, ffire, m_player_skill, multimode;
    int player_skill, m_marker, marker, mouseflip;
    int statusbarmode, althud;
    MapRecord* nextLevel;

};

struct player_orig
{
    int ox, oy, oz;
    short oa, os;
};

struct player_struct 
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

};


END_DUKE_NS
