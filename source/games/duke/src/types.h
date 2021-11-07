#pragma once
#include "names.h"
#include "packet.h"
#include "d_net.h"
#include "gameinput.h"

BEGIN_DUKE_NS


// all the struct types from JFDuke's duke3d.h

struct STATUSBARTYPE 
{
	short frag[MAXPLAYERS], got_access, last_extra, shield_amount, curr_weapon;
	short ammo_amount[MAX_WEAPONS];
	uint8_t inven_icon, jetpack_on, heat_on;
	short firstaid_amount, steroids_amount, holoduke_amount, jetpack_amount;
	short heat_amount, scuba_amount, boot_amount;
	short last_weapon, weapon_pos, kickback_pic;
	bool gotweapon[MAX_WEAPONS];
};

struct DDukeActor
{
	uint8_t cgg;
	uint8_t spriteextra;	// moved here for easier maintenance. This was originally a hacked in field in the sprite structure called 'filler'.
	short owner; // todo: make a pointer.
	short picnum, ang, extra, movflag;
	short tempang, actorstayput, dispicnum;
	short timetosleep;
	int floorz, ceilingz, lastvx, lastvy, aflags;
	union
	{
		int saved_ammo;
		int palvals;
	};
	int temp_data[6];
	DDukeActor* temp_actor, *seek_actor;
	spritetype* s;	// direct reference to the corresponding sprite.

	static DDukeActor* array();	// this is necessary to allow define inline functions referencing the global array inside the definition itself.

	DDukeActor() : s(&sprite[this - array()]) {}
	DDukeActor(const DDukeActor& other) = delete;				// we also do not want to allow copies.
	DDukeActor& operator=(const DDukeActor& other) = delete;
	void clear()
	{
		cgg = spriteextra = 0;
		picnum = ang = extra = owner = movflag = tempang = actorstayput = dispicnum = timetosleep = 0;
		floorz = ceilingz = lastvx = lastvy = aflags = saved_ammo = 0;
		memset(temp_data, 0, sizeof(temp_data));
	}
	int GetSpriteIndex() const { return int(this - array()); }

	// Wrapper around some ugliness. The 'owner' field gets abused by some actors, so better wrap its real use in access functions to keep things in order.
	inline DDukeActor* GetOwner()
	{
		return s->owner < 0 ? nullptr : &array()[s->owner];
	}

	inline void SetOwner(DDukeActor* a)
	{
		s->owner = a? a->GetSpriteIndex() : -1;
	}

	// same for the 'hittype' owner - which is normally the shooter in an attack.
	inline DDukeActor* GetHitOwner()
	{
		return owner < 0 ? nullptr : &array()[owner];
	}

	inline void SetHitOwner(DDukeActor* a)
	{
		owner = a ? a->GetSpriteIndex() : -1;
	}

	// This used the Owner field - better move this to something more safe.
	inline bool IsActiveCrane()
	{
		return s->owner == -2;
	}

	inline void SetActiveCrane(bool yes)
	{
		s->owner = yes ? -2 : -1;
	}

	int PlayerIndex() const
	{
		// only valid for real players - just here to abstract yvel.
		return s->yvel;
	}

	sectortype* getSector() const
	{
		return &sector[s->sectnum];
	}


};
extern DDukeActor hittype[MAXSPRITES + 1];
inline DDukeActor* DDukeActor::array() { return hittype; }

struct animwalltype
{
	int wallnum, tag;
};

// Todo - put more state in here
struct ActorInfo
{
	uint32_t scriptaddress;
	uint32_t flags;
	int aimoffset;
	int falladjustz;
	int gutsoffset;
};

// for now just flags not related to actors, may get more info later.
struct TileInfo
{
	int flags;
	int loadeventscriptptr;
};

struct user_defs
{
	uint8_t god, cashman, eog;
	uint8_t clipping;
	uint8_t user_pals[MAXPLAYERS];

	short from_bonus;
	short last_level, secretlevel;
	short bomb_tag;

	int const_visibility;

	int runkey_mode;

	int shadows;
	int coords, levelstats, m_coop, coop;
	int wchoice[MAXPLAYERS][MAX_WEAPONS];

	int respawn_monsters, respawn_items, respawn_inventory, recstat, monsters_off, brightness;
	int m_respawn_items, m_respawn_monsters, m_respawn_inventory, m_recstat, m_monsters_off;
	int m_ffire, ffire, m_player_skill, multimode;
	int player_skill, marker;

	DDukeActor* cameraactor;

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
	vec3_t pos;

	// player's horizon and angle structs.
	PlayerHorizon horizon;
	PlayerAngle angle;

	uint16_t frags[MAXPLAYERS];

	bool gotweapon[MAX_WEAPONS];

	// Palette management uses indices into the engine's palette table now.
	PalEntry pals;

	// this was a global variable originally.
	vec2_t fric;

	// weapon drawer variables and their interpolation counterparts.
	int weapon_sway;
	int oweapon_sway;
	short weapon_pos, kickback_pic, random_club_frame;
	short oweapon_pos, okickback_pic, orandom_club_frame;
	uint8_t hard_landing;
	uint8_t ohard_landing;

	// Store current psectlotag as determined in processinput() for use with scaling angle aiming.
	short psectlotag;

	// From here on it is unaltered from JFDuke with the exception of a few fields that are no longer needed and were removed.
	int exitx, exity, loogiex[64], loogiey[64], numloogs, loogcnt;
	int invdisptime;
	int bobposx, bobposy, oposx, oposy, oposz, pyoff, opyoff;
	int posxv, posyv, poszv, last_pissed_time, truefz, truecz;
	int player_par, visibility;
	int bobcounter;
	int randomflamex, crack_time;

	int aim_mode, ftt;

	int cursectnum, one_parallax_sectnum, access_wallnum; // wall + sector references. Make them pointers later?

	short last_extra, subweapon;
	short ammo_amount[MAX_WEAPONS], frag, fraggedself;

	short curr_weapon, last_weapon, tipincs, wantweaponfire;
	short holoduke_amount, hurt_delay, hbomb_hold_delay;
	short jumping_counter, airleft, knee_incs, access_incs;
	short ftq;
	short got_access, weapon_ang, firstaid_amount;
	short i;
	short over_shoulder_on, fist_incs;
	short cheat_phase;
	short extra_extra8, quick_kick, last_quick_kick;
	short heat_amount, timebeforeexit, customexitsound;
	DDukeActor* actorsqu, *wackedbyactor, *on_crane, *holoduke_on, *somethingonplayer, *access_spritenum, *dummyplayersprite, *newOwner;

	short weaprecs[256], weapreccnt;
	unsigned int interface_toggle_flag;

	short dead_flag, show_empty_weapon;	// JBF 20031220: added orotscrnang
	short scuba_amount, jetpack_amount, steroids_amount, shield_amount;
	short pycount, frag_ps;
	short transporter_hold, last_full_weapon, footprintshade, boot_amount;

	uint8_t on_warping_sector, footprintcount;
	uint8_t hbomb_on, jumping_toggle, rapid_fire_hold, on_ground;
	char name[32];
	uint8_t inven_icon, buttonpalette;

	uint8_t jetpack_on, spritebridge, lastrandomspot;
	uint8_t scuba_on, footprintpal, heat_on;

	uint8_t  holster_weapon;
	uint8_t falling_counter;
	uint8_t refresh_inventory;

	uint8_t toggle_key_flag, knuckle_incs; // ,select_dir;
	uint8_t walking_snd_toggle, palookup;
	bool quick_kick_msg;

	int max_secret_rooms, secret_rooms, max_actors_killed, actors_killed;

	bool resurrected;

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
	short moto_drink;
	float TiltStatus, oTiltStatus;
	short VBumpNow, VBumpTarget, TurbCount;
	short drug_stat[3]; // raat5f1..5
	uint8_t DrugMode, lotag800kill;
	uint8_t sea_sick_stat; // raat5dd
	uint8_t hurt_delay2, nocheat;
	uint8_t OnMotorcycle, OnBoat, moto_underwater, NotOnWater, MotoOnGround;
	uint8_t moto_do_bump, moto_bump_fast, moto_on_oil, moto_on_mud;
	double vehForwardScale, vehReverseScale, MotoSpeed;
	bool vehTurnLeft, vehTurnRight, vehBraking;

	// input stuff.
	InputPacket sync;

	DDukeActor* GetActor();
	int GetPlayerNum();

	void apply_seasick(double factor);
	void backuppos(bool noclipping = false);
	void backupweapon();
	void checkhardlanding();
	void playerweaponsway(int xvel);

	float adjustavel(float avel)
	{
		return (psectlotag == ST_2_UNDERWATER)? avel * 0.875f : avel;
	}

	sectortype* cursector() const
	{
		return &::sector[cursectnum];
	}
	sectortype* one_parallax_sector() const
	{
		return &::sector[one_parallax_sectnum];
	}


};

struct Cycler
{
	int sectnum;
	int16_t lotag;
	int16_t hitag;
	int16_t shade1;
	int16_t shade2;
	bool state;

	sectortype* sector() const { return &::sector[sectnum]; }
};

// Wrapper around the insane collision info mess from Build.
struct Collision
{
	int type;
	int index;
	int legacyVal;	// should be removed later, but needed for converting back for unadjusted code.
	DDukeActor* actor;

	Collision() = default;
	explicit Collision(int v)
	{
		setFromEngine(v);
	}
	int setNone()
	{
		type = kHitNone;
		index = -1;
		legacyVal = 0;
		actor = nullptr;
		return kHitNone;
	}

	int setSector(int num)
	{
		type = kHitSector;
		index = num;
		legacyVal = type | index;
		actor = nullptr;
		return kHitSector;
	}
	int setWall(int num)
	{
		type = kHitWall;
		index = num;
		legacyVal = type | index;
		actor = nullptr;
		return kHitWall;
	}
	int setSprite(DDukeActor* num)
	{
		type = kHitSprite;
		index = -1;
		legacyVal = type | int(num - hittype);
		actor = num;
		return kHitSprite;
	}

	int setFromEngine(int value)
	{
		legacyVal = value;
		type = value & kHitTypeMask;
		if (type == 0) { index = -1; actor = nullptr; }
		else if (type != kHitSprite) { index = value & kHitIndexMask; actor = nullptr; }
		else { index = -1; actor = &hittype[value & kHitIndexMask]; }
		return type;
	}

	walltype* wall() const
	{
		assert(type == kHitWall);
		return &::wall[index];
	}

	sectortype* sector() const
	{
		assert(type == kHitSector);
		return &::sector[index];
	}

};



END_DUKE_NS
