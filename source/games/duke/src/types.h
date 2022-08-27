#pragma once
#include "coreactor.h"
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

struct FireProj
{
	vec3_t pos, vel;
};

// Todo - put more state in here
struct ActorInfo
{
	uint32_t scriptaddress;
	EDukeFlags1 flags;
	EDukeFlags2 flags2;
	int aimoffset;
	int falladjustz;
	int gutsoffset;
};

class DDukeActor : public DCoreActor
{
	DECLARE_CLASS(DDukeActor, DCoreActor)
	HAS_OBJECT_POINTERS
public:
	TObjPtr<DDukeActor*> ownerActor, hitOwnerActor;

	uint8_t cgg;
	uint8_t spriteextra;	// moved here for easier maintenance. This was originally a hacked in field in the sprite structure called 'filler'.
	short attackertype, hitang, hitextra, movflag;
	short tempang, dispicnum, basepicnum;
	short timetosleep;
	vec2_t ovel;
	double floorz, ceilingz;
	union
	{
		int saved_ammo;
		int palvals;
		int tempsound;
	};
	int temp_data[6];
	// Some SE's stored indices in temp_data. For purposes of clarity avoid that. These variables are meant to store these elements now
	walltype* temp_walls[2]; // SE20 + SE128
	sectortype* temp_sect, *actorstayput;

	TObjPtr<DDukeActor*> temp_actor, seek_actor;

	TArray<GameVarValue> uservars;

	EDukeFlags1 flags1;
	EDukeFlags2 flags2;

	// Todo: Once we start assigning subclasses to actors, this one needs to be moved to the proper FIREBALL subclass.
	FireProj fproj;

	DDukeActor() = default;
	size_t PropagateMark() override;
	const ActorInfo* actorInfo() const;

	// This once was stored in the owner field of the sprite
	inline DDukeActor* GetOwner()
	{
		return ownerActor;
	}

	inline void SetOwner(DDukeActor* a)
	{
		ownerActor = a;
	}

	inline DDukeActor* GetHitOwner()
	{
		return hitOwnerActor;
	}

	inline void SetHitOwner(DDukeActor* a)
	{
		hitOwnerActor = a;
	}

	inline bool IsActiveCrane()
	{
		return spr.intowner == -2;
	}

	inline void SetActiveCrane(bool yes)
	{
		ownerActor = nullptr;
		spr.intowner = yes ? -2 : -1;
	}

	int PlayerIndex() const
	{
		// only valid for real players - just here to abstract yvel.
		return spr.yvel;
	}

	bool isPlayer() const
	{
		return spr.picnum == TILE_APLAYER;
	}

	void Serialize(FSerializer& arc) override;

	int actor_int_ceilingz() const
	{
		return ceilingz * zworldtoint;
	}

	int actor_int_floorz() const
	{
		return floorz * zworldtoint;
	}


	void ChangeType(PClass* newtype)
	{
		if (newtype->IsDescendantOf(RUNTIME_CLASS(DDukeActor)) && newtype->Size == RUNTIME_CLASS(DDukeActor)->Size && GetClass()->Size == RUNTIME_CLASS(DDukeActor)->Size)
		{
			// It sucks having to do this but the game heavily depends on being able to swap out the class type and often uses this to manage actor state.
			// We'll allow this only for classes that do not add their own data, though.
			SetClass(newtype);
		}
	}

};

// subclassed to add a game specific actor() method
using HitInfo = THitInfo<DDukeActor>;
using Collision = TCollision<DDukeActor>;

struct animwalltype
{
	walltype* wall;
	int tag;
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

	TObjPtr<DDukeActor*> cameraactor;

};

struct player_orig
{
	DVector3 opos;
	short oa;
	sectortype* os;
};

struct CraneDef
{
	DVector3 pos;
	DVector2 pole;
	TObjPtr<DDukeActor*> poleactor;
};

struct player_struct 
{
	vec3_t vel;
	DVector3 pos, opos;
	DVector2 bobpos;

	// player's horizon and angle structs.
	PlayerHorizon horizon;
	PlayerAngle angle;

	uint16_t frags[MAXPLAYERS];

	bool gotweapon[MAX_WEAPONS];

	// Palette management uses indices into the engine's palette table now.
	PalEntry pals;

	// this was a global variable originally.
	vec2_t fric, exit, loogie[64];

	// weapon drawer variables and their interpolation counterparts.
	int weapon_sway;
	int oweapon_sway;
	short weapon_pos, kickback_pic, random_club_frame;
	short oweapon_pos, okickback_pic, orandom_club_frame;
	uint8_t hard_landing;
	uint8_t ohard_landing;
	int fistsign, ofistsign;

	// Store current psectlotag as determined in processinput() for use with scaling angle aiming.
	short psectlotag;

	// From here on it is unaltered from JFDuke with the exception of a few fields that are no longer needed and were removed.
	int numloogs, oloogcnt, loogcnt;
	int invdisptime;
	int pyoff, opyoff;
	int last_pissed_time;
	int player_par, visibility;
	int bobcounter;
	int randomflamex, crack_time;

	int aim_mode, ftt;

	double truefz, truecz;
	sectortype* cursector;
	sectortype* one_parallax_sectnum; // wall + sector references.
	walltype* access_wall;
	DDukeActor* actor;
	TObjPtr<DDukeActor*> actorsqu, wackedbyactor, on_crane, holoduke_on, somethingonplayer, access_spritenum, dummyplayersprite, newOwner;

	short last_extra, subweapon;
	short ammo_amount[MAX_WEAPONS], frag, fraggedself;

	short curr_weapon, last_weapon, otipincs, tipincs, wantweaponfire;
	short holoduke_amount, hurt_delay, hbomb_hold_delay;
	short jumping_counter, airleft, oknee_incs, knee_incs, oaccess_incs, access_incs;
	short ftq;
	short got_access, weapon_ang, firstaid_amount;
	short over_shoulder_on, ofist_incs, fist_incs;
	short cheat_phase;
	short extra_extra8, quick_kick, last_quick_kick;
	short heat_amount, timebeforeexit, customexitsound;

	short weaprecs[256], weapreccnt;
	unsigned int interface_toggle_flag;

	short dead_flag, show_empty_weapon;
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
	vec2_t noise;
	int noise_radius; // at286, at28a, at290
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
	double VBumpNow, VBumpTarget;
	short TurbCount;
	short drug_stat[3]; // raat5f1..5
	uint8_t DrugMode, lotag800kill;
	uint8_t sea_sick_stat; // raat5dd
	uint8_t hurt_delay2, nocheat;
	uint8_t OnMotorcycle, OnBoat, moto_underwater, NotOnWater, MotoOnGround;
	uint8_t moto_do_bump, moto_bump_fast, moto_on_oil, moto_on_mud;
	double vehForwardScale, vehReverseScale, MotoSpeed;
	bool vehTurnLeft, vehTurnRight, vehBraking;

	TArray<GameVarValue> uservars;


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

	void setCursector(sectortype* sect)
	{
		cursector = sect;
	}

	bool insector() const
	{
		return cursector != nullptr;
	}

	void backupxyz()
	{
		opos = pos;
	}

	void restorexyz()
	{
		pos = opos;
	}

	void backupxy()
	{
		opos.X = pos.X;
		opos.Y = pos.Y;
	}

	void backupz()
	{
		opos.Z = pos.Z;
	}

	void setbobpos()
	{
		bobpos = pos.XY();
	}

	void getposfromactor(DCoreActor* actor, double addz = 0)
	{
		pos = actor->spr.pos;
		if (addz) pos.Z  += addz;
	}

	void getxyfromactor(DCoreActor* actor)
	{
		pos.X = actor->spr.pos.X;
		pos.Y = actor->spr.pos.Y;
	}

	vec3_t player_int_pos() const
	{
		return { int(pos.X * worldtoint), int(pos.Y * worldtoint), int(pos.Z * zworldtoint) };
	}

	vec3_t player_int_opos() const
	{
		return { int(opos.X * worldtoint), int(opos.Y * worldtoint), int(opos.Z * zworldtoint) };
	}

	void player_add_int_z(int z)
	{
		pos.Z  += z * zinttoworld;
	}

	void player_set_int_z(int z)
	{
		pos.Z  = z * zinttoworld;
	}
	void player_add_int_xy(const vec2_t& v)
	{
		pos.X  += v.X * inttoworld;
		pos.Y  += v.Y * inttoworld;
	}
};

struct Cycler
{
	sectortype* sector;
	int16_t lotag;
	int16_t hitag;
	int16_t shade1;
	int16_t shade2;
	bool state;
};


struct DukeLevel
{

};



END_DUKE_NS
