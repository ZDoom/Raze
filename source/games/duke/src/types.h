#pragma once
#include "coreactor.h"
#include "names.h"
#include "packet.h"
#include "d_net.h"
#include "coreplayer.h"
#include "texturemanager.h"

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

struct ActorMove
{
	FName qualifiedName;	// this is only used for serialization.
	FName name;
	float movex, movez;
};
struct ActorAction
{
	FName qualifiedName;	// this is only used for serialization.
	FName name;
	FTextureID base;
	int offset;
	int16_t numframes;
	int16_t rotationtype;
	int16_t increment;
	int16_t delay;
};

struct ActorAI
{
	FName name;
	uint32_t move;
	uint32_t action;
	int moveflags;
};

inline TArray<ActorMove> moves;
inline TArray<ActorAction> actions;
inline TArray<ActorAI> ais;

struct ActorInfo
{
	uint32_t loadeventscriptptr;
	uint32_t scriptaddress;
	EDukeFlags1 enemyflags;	// placeholder during parsing. Since CON gets parsed before the spawn type table we cannot copy these to their final location yet.
};

class DDukeActor : public DCoreActor
{
	DECLARE_CLASS(DDukeActor, DCoreActor)
	HAS_OBJECT_POINTERS
public:
	TObjPtr<DDukeActor*> ownerActor, hitOwnerActor;
	PClassActor* attackertype;

	const DDukeActor* attackerDefaults()
	{
		return static_cast<DDukeActor*>(GetDefaultByType(attackertype? attackertype : RUNTIME_CLASS(DDukeActor)));
	}

	uint8_t cgg;
	uint8_t spriteextra;	// moved here for easier maintenance. This was originally a hacked in field in the sprite structure called 'filler'.
	uint16_t movflag;
	short hitextra;
	short tempval, basepicnum;
	unsigned short timetosleep;
	bool mapSpawned;
	DVector2 ovel;
	DAngle hitang;
	double floorz, ceilingz;
	union
	{
		int saved_ammo;
		int palvals;
		int tempsound;
	};
	// note: all this temp shit needs to be moved to subclass specific variables once things get cleaned up. This is a major issue with code readability.
	int counter;	// sprite animation counters - were previously stored in temp_data.
	int temp_data[5];
	// Some SE's stored indices in temp_data. For purposes of clarity avoid that. These variables are meant to store these elements now
	walltype* temp_walls[2]; // SE20 + SE128
	sectortype* temp_sect, *actorstayput;
	DAngle temp_angle;
	DVector3 temp_pos, temp_pos2;
	ActorMove* curMove;
	ActorAction* curAction;
	FName curAI;	// no need to store the pointer here.
	int16_t actioncounter, curframe;	// sprite animation counters - were previously stored in temp_data.

	EDukeFlags1 flags1;
	EDukeFlags2 flags2;
	EDukeFlags3 flags3;
	EDukeFlags4 flags4;

	TObjPtr<DDukeActor*> temp_actor, seek_actor;
	TArray<GameVarValue> uservars;

	DDukeActor() = default;
	size_t PropagateMark() override;
	const ActorInfo* conInfo() const;

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

	int PlayerIndex() const
	{
		// only valid for real players - just here to abstract yvel.
		return spr.yint;
	}

	bool isPlayer() const;

	void Serialize(FSerializer& arc) override;

	void ChangeType(PClass* newtype)
	{
		if (newtype->IsDescendantOf(RUNTIME_CLASS(DDukeActor)) && newtype->Size == RUNTIME_CLASS(DDukeActor)->Size && GetClass()->Size == RUNTIME_CLASS(DDukeActor)->Size)
		{
			// It sucks having to do this but the game heavily depends on being able to swap out the class type and often uses this to manage actor state.
			// We'll allow this only for classes that do not add their own data, though.
			SetClass(newtype);
			spr.setspritetexture(GetDefaultByType(newtype)->spr.spritetexture());
		}
	}
	
	void setClipDistFromTile()
	{
		auto tex = TexMan.GetGameTexture(spr.spritetexture());
		clipdist = spr.scale.X * tex->GetDisplayWidth() * 0.125;
	}
};

// subclassed to add a game specific actor() method
using HitInfo = THitInfo<DDukeActor>;
using Collision = TCollision<DDukeActor>;


// This is to satisfy the CON-based requirement of 'killit' immediately aborting all script code execution.
class CDukeKillEvent : public std::exception
{
	int Type_;
public:
	CDukeKillEvent(int type): Type_(type) {}
	int Type() const { return Type_; }
	// to print a meaningful message if killit got called from the wrong place.
	const char* what() const noexcept override { return "killit called from outside RunState!"; }
};

struct animwalltype
{
	walltype* wall;
	int tag;
	FTextureID origtex;
	bool overpic;
};

struct user_defs
{
	int mapflags;
	uint8_t god, cashman, eog;
	uint8_t clipping;
	uint8_t user_pals[MAXPLAYERS];
	uint8_t ufospawnsminion;
	uint8_t pistonsound, fogactive;

	short from_bonus;
	short last_level, secretlevel;
	short bomb_tag;

	int const_visibility;

	int shadows;
	int coords, m_coop, coop;
	int wchoice[MAXPLAYERS][MAX_WEAPONS];

	int respawn_monsters, respawn_items, respawn_inventory, recstat, monsters_off, brightness;
	int m_respawn_items, m_respawn_monsters, m_respawn_inventory, m_recstat, m_monsters_off;
	int m_ffire, ffire, m_player_skill, multimode;
	int player_skill, marker;
	int chickenplant;							// readonly - used to trigger some special behavior if a special item is found in a map.
	int earthquaketime;
	bool joe9000;	// this was formerly a static local variable inside a function, but needs to be properly maintained and serialized.

	TObjPtr<DDukeActor*> cameraactor;

};

struct player_orig
{
	DVector3 opos;
	DAngle oa;
	sectortype* os;
};

class DDukePlayer final : public DCorePlayer
{
	DECLARE_CLASS(DDukePlayer, DCorePlayer)
	HAS_OBJECT_POINTERS
	size_t PropagateMark() override;
	DDukePlayer() = default;
public:
	DDukePlayer(uint8_t p) : DCorePlayer(p) {}
	DDukePlayer& operator=(DDukePlayer&) = delete;
	DDukePlayer(DDukePlayer&) = delete;
	void Serialize(FSerializer& arc) override;
	TArray<GameVarValue> uservars;
	DVector3 vel;
	DVector2 bobpos;
	DVector2 fric;
	DVector2 Exit;

	uint16_t frags[MAXPLAYERS];

	bool gotweapon[MAX_WEAPONS];

	// Palette management uses indices into the engine's palette table now.
	PalEntry pals;

	// this was a global variable originally.
	DVector2 loogie[64];

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
	int last_pissed_time;
	int player_par, visibility;
	int bobcounter;
	int randomflamex, crack_time;

	int aim_mode, ftt;

	double pyoff, opyoff;
	double truefz, truecz;
	sectortype* cursector;
	sectortype* one_parallax_sectnum; // wall + sector references.
	walltype* access_wall;
	DDukeActor* GetActor() override;
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

	PClass* weaprecs[32];
	int weapreccnt;
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

	// Redneck Rampage additions. Those which did not have names in the reconstructed source got one from either RedneckGDX or RedNukem.
	// Items were reordered by size.
	int stairs;
	int detonate_count; // at57e
	DVector2 noise;
	double noise_radius; // at286, at28a, at290
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
	DAngle TiltStatus, oTiltStatus;
	double VBumpNow, VBumpTarget;
	short TurbCount;
	short drug_stat[3]; // raat5f1..5
	uint8_t DrugMode, lotag800kill;
	uint8_t sea_sick_stat; // raat5dd
	uint8_t hurt_delay2, nocheat;
	uint8_t OnMotorcycle, OnBoat, moto_underwater, NotOnWater, MotoOnGround;
	uint8_t moto_do_bump, moto_bump_fast, moto_on_oil, moto_on_mud;
	double MotoSpeed;

	int GetPlayerNum();

	void apply_seasick();
	void backuppos(bool noclipping = false);
	void backupweapon();
	void checkhardlanding();
	void playerweaponsway(double xvel);

	inline void setCursector(sectortype* sect)
	{
		cursector = sect;
	}

	inline bool insector() const
	{
		return cursector != nullptr;
	}

	inline void setbobpos()
	{
		bobpos = GetActor()->spr.pos.XY();
	}

	void updatecentering(const int snum)
	{
		if (!(cmd.ucmd.actions & SB_CENTERVIEW))
			return;

		const bool returnlock = cl_dukepitchmode & kDukePitchLockReturn;
		const bool centertest = abs(GetActor()->spr.Angles.Pitch.Degrees()) > 2.2370; // Build horizon value of 5.

		if ((centertest && returnlock) || !cmd.ucmd.ang.Pitch.Degrees())
		{
			setForcedSyncInput(snum);
			cmd.ucmd.ang.Pitch = nullAngle;
		}
		else
		{
			cmd.ucmd.actions &= ~SB_CENTERVIEW;
		}
	}

	inline void doslopetilting()
	{
		Angles.doViewPitch(aim_mode == 0 && on_ground && cursector->lotag != ST_2_UNDERWATER);
	}

	inline bool itemUsed(int num)
	{
		return cmd.ucmd.isItemUsed(num - 1);
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

struct AmbientTags
{
	int lo, hi;
};

struct DukeLevel
{

};



END_DUKE_NS
