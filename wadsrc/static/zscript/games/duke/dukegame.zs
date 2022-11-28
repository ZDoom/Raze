//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020-2021 Christoph Oelckers

This file is part of Raze.

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
aint with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
( not much left of the original code, though... ;) )
*/
//------------------------------------------------------------------------- 

// contains all global Duke definitions
struct Duke native
{
	enum ESpecialMusic
	{
		MUS_INTRO = 0,
		MUS_BRIEFING = 1,
		MUS_LOADING = 2,
	};

	enum EPalette
	{
		BASEPAL = 0,
		WATERPAL,
		SLIMEPAL,
		TITLEPAL,
		DREALMSPAL,
		ENDINGPAL,  // 5
		ANIMPAL,    // not used anymore. The anim code now generates true color textures.
		DRUGPAL,
		BASEPALCOUNT
	};

	enum dukeinvicon_t
	{
		ICON_NONE,  // 0
		ICON_FIRSTAID,
		ICON_STEROIDS,
		ICON_HOLODUKE,
		ICON_JETPACK,
		ICON_HEATS,  // 5
		ICON_SCUBA,
		ICON_BOOTS,
		ICON_MAX
	};
	
	enum EClipMask
	{
		CLIPMASK0 = (1 << 16) + 1,
		CLIPMASK1 = (256 << 16) + 64
	}

	enum ETextureFlags
	{
		TFLAG_WALLSWITCH			= 1,
		TFLAG_ADULT					= 2,
		TFLAG_ELECTRIC				= 4,
		TFLAG_CLEARINVENTORY		= 8,	// really dumb Duke stuff...
		TFLAG_SLIME					= 16,
		TFLAG_NOCIRCLEREFLECT			= 32,
	};

	enum ESoundFlags
	{
		SF_LOOP = 1,
		SF_MSFX = 2,
		SF_TALK = 4,
		SF_ADULT = 8,
		SF_GLOBAL = 16,
		SF_ONEINST_INTERNAL = 32,

		SF_DTAG = 128,
	};

	enum ESyncBits
	{
		SB_FIRST_WEAPON_BIT = 1 << 0,
		SB_ITEM_BIT_1 = 1 << 4,
		SB_ITEM_BIT_2 = 1 << 5,
		SB_ITEM_BIT_3 = 1 << 6,
		SB_ITEM_BIT_4 = 1 << 7,
		SB_ITEM_BIT_5 = 1 << 8,
		SB_ITEM_BIT_6 = 1 << 9,
		SB_ITEM_BIT_7 = 1 << 10,

		SB_INVPREV = 1 << 11,
		SB_INVNEXT = 1 << 12,
		SB_INVUSE = 1 << 13,
		SB_CENTERVIEW = 1 << 14,
		SB_TURNAROUND = 1 << 15,
		SB_HOLSTER = 1 << 16,
		SB_OPEN = 1 << 17,

		SB_AIMMODE = 1 << 18,   
		SB_QUICK_KICK = 1 << 19,
		SB_ESCAPE = 1 << 20,

		SB_AIM_UP = 1 << 21,
		SB_AIM_DOWN = 1 << 22,
		SB_LOOK_LEFT = 1 << 23,
		SB_LOOK_RIGHT = 1 << 24,
		SB_LOOK_UP = 1 << 25,
		SB_LOOK_DOWN = 1 << 26,
		SB_RUN = 1 << 27,
		SB_JUMP = 1 << 28,
		SB_CROUCH = 1 << 29,
		SB_FIRE = 1 << 30,
		SB_ALTFIRE = 1u << 31,

		SB_WEAPONMASK_BITS = (15u * SB_FIRST_WEAPON_BIT), // Weapons take up 4 bits
		SB_ITEMUSE_BITS = (127u * SB_ITEM_BIT_1),

		SB_BUTTON_MASK = SB_ALTFIRE|SB_FIRE|SB_CROUCH|SB_JUMP|SB_LOOK_UP|SB_LOOK_DOWN|SB_AIM_UP|SB_AIM_DOWN|SB_LOOK_LEFT|SB_LOOK_RIGHT,     // all input from buttons (i.e. active while held)
		SB_INTERFACE_MASK = (SB_INVPREV|SB_INVNEXT|SB_INVUSE|SB_CENTERVIEW|SB_TURNAROUND|SB_HOLSTER|SB_OPEN|SB_ESCAPE|SB_QUICK_KICK),  // all input from CCMDs
		SB_INTERFACE_BITS = (SB_WEAPONMASK_BITS | SB_ITEMUSE_BITS | SB_INTERFACE_MASK),
		SB_ALL = ~0u
	};

	native static void PlaySpecialMusic(int which);
	native static int PlaySound(Sound num, int channel = CHAN_AUTO, int flags = 0, float vol =0.8f);
	native static void StopSound(Sound num);
	native static bool CheckSoundPlaying(Sound num);
	native static DukePlayer GetViewPlayer();
	native static DukePlayer GetLocalPlayer();
	native static int MaxAmmoAmount(int weap);
	native static DukePlayer checkcursectnums(sectortype sect);
	native static int global_random();
	native static int GetSoundFlags(Sound snd);
	native static int badguyID(int id);
	static int rnd(int val)
	{
		return (random(0, 255) >= (255 - (val)));
	}

	static void PlayBonusMusic()
	{
		if (System.MusicEnabled())
			PlaySound("BONUSMUSIC", CHAN_AUTO, CHANF_UI);
	}

	//==========================================================================
	//
	// wrappers around DrawText to allow easier reuse of the old code.
	// The vertical displacements are to have the same positioning as with the original code.
	//
	//==========================================================================

	static void BigText(double x, double y, String text, int align = -1, double alpha = 1.)
	{
		let myfont = Raze.PickBigFont();
		if (!Raze.isRR())
		{
			if (align != -1) x -= myfont.StringWidth(text) * (align == 0 ? 0.5 : 1);
			Screen.DrawText(myfont, Font.CR_UNTRANSLATED, x, y - 12, text, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Alpha, alpha);
		}
		else
		{
			if (align != -1) x -= myfont.StringWidth(text) * (align == 0 ? 0.2 : 0.4);
			Screen.DrawText(myfont, Font.CR_UNTRANSLATED, x, y - 12, text, DTA_FullscreenScale, FSMode_Fit320x200, DTA_ScaleX, 0.4, DTA_ScaleY, 0.4, DTA_Alpha, alpha);
		}
	}

	static void GameText(double x, double y, String t, int shade, int align = -1, int trans = 0)
	{
		let myfont = Raze.PickSmallFont();
		int fsmode = FSMode_Fit320x200;
		if (Raze.isRR())
		{
			x *= 2;
			y *= 2;
			fsmode = FSMode_Fit640x400;
		}
		if (align != -1) x -= myfont.StringWidth(t) * (align == 0 ? 0.5 : 1);
		Screen.DrawText(myfont, Font.CR_NATIVEPAL, x, y + 2, t, DTA_FullscreenScale, fsmode, DTA_TranslationIndex, Translation.MakeID(Translation_Remap, trans), DTA_Color, Raze.shadeToLight(shade));
	}
}

struct DukePlayer native
{
	/*
	// player's horizon and angle structs.
	PlayerHorizon horizon;
	PlayerAngle angle;

	uint16_t frags[MAXPLAYERS];
	*/

	native bool gotweapon[DukeWpn.MAX_WEAPONS];

	// Palette management uses indices into the engine's palette table now.
	native color pals;

	// this was a global variable originally.
	native Vector2 fric;

	// weapon drawer variables and their interpolation counterparts.
	native int weapon_sway;
	native int oweapon_sway;
	native int16 weapon_pos, kickback_pic, random_club_frame;
	native int16 oweapon_pos, okickback_pic, orandom_club_frame;
	native uint8 hard_landing;
	native uint8 ohard_landing;

	// Store current psectlotag as determined in processinput() for use with scaling angle aiming.
	native int16 psectlotag;

	native Vector2 loogie[64];
	native int numloogs, loogcnt;
	native int invdisptime;
	native double pyoff, opyoff;
	native int last_pissed_time;
	native double truefz, truecz;
	native int player_par, visibility;
	native int bobcounter;
	native int randomflamex, crack_time;
	native int aim_mode, ftt;

	//native int cursectnum;// , one_parallax_sectnum
	//native walltype access_wall; // can't do yet.

	native int16 last_extra, subweapon;
	native int16 ammo_amount[DukeWpn.MAX_WEAPONS], frag, fraggedself;

	native int16 curr_weapon, last_weapon, tipincs, wantweaponfire;
	native int16 holoduke_amount, hurt_delay, hbomb_hold_delay;
	native int16 jumping_counter, airleft, knee_incs, access_incs;
	native int16 ftq;
	native int16 got_access, weapon_ang, firstaid_amount;
	native int16 over_shoulder_on, fist_incs;
	native int16 cheat_phase;
	native int16 extra_extra8, quick_kick, last_quick_kick;
	native int16 heat_amount, timebeforeexit, customexitsound;
	native DukeActor actor, actorsqu, wackedbyactor, on_crane, somethingonplayer, access_spritenum, dummyplayersprite, newOwner, holoduke_on;
	native sectortype cursector;

	native int16 weaprecs[256], weapreccnt;
	native uint interface_toggle_flag;

	native int16 dead_flag, show_empty_weapon;	// JBF 20031220: added orotscrnang
	native int16 scuba_amount, jetpack_amount, steroids_amount, shield_amount;
	native int16 pycount, frag_ps;
	native int16 transporter_hold, last_full_weapon, footprintshade, boot_amount;

	native uint8 on_warping_sector, footprintcount;
	native uint8 hbomb_on, jumping_toggle, rapid_fire_hold, on_ground;
	//char name[32];
	native uint8 inven_icon, buttonpalette;

	native uint8 jetpack_on, spritebridge, lastrandomspot;
	native uint8 scuba_on, footprintpal, heat_on;

	native uint8  holster_weapon;
	native uint8 falling_counter;
	native uint8 refresh_inventory;

	native uint8 toggle_key_flag, knuckle_incs; // ,select_dir;
	native uint8 walking_snd_toggle, palookup;
	native bool quick_kick_msg;

	native int max_secret_rooms, secret_rooms, max_actors_killed, actors_killed;

	native bool resurrected;

	// Redneck Rampage additions. Those which did not have names in the reconstructed source got one from either RedneckGDX or RedNukem.
	// Items were reordered by size.
	native int stairs;
	native int detonate_count; // at57e
	native double noise_radius; // at286, at28a, at290
	native int drink_timer; // at58e
	native int eat_timer; // at592
	native int SlotWin;
	native int16 recoil;
	native int16 detonate_time; // at57c
	native int16 yehaa_timer;
	native int16 drink_amt, eat, drunkang, eatang;
	native uint8 shotgun_state[2];
	native uint8 donoise; // at28e
	native uint8 keys[5];

	// RRRA. The same as for the RR block applies.
	native int drug_aspect;
	native int drug_timer;
	native int SeaSick;
	native int16 MamaEnd; // raat609
	native int16 moto_drink;
	native float TiltStatus, oTiltStatus;
	native double VBumpNow, VBumpTarget;
	native int16 TurbCount;
	native int16 drug_stat[3]; // raat5f1..5
	native uint8 DrugMode, lotag800kill;
	native uint8 sea_sick_stat; // raat5dd
	native uint8 hurt_delay2, nocheat;
	native uint8 OnMotorcycle, OnBoat, moto_underwater, NotOnWater, MotoOnGround;
	native uint8 moto_do_bump, moto_bump_fast, moto_on_oil, moto_on_mud;
	native double vehForwardScale, vehReverseScale, MotoSpeed;
	native bool vehTurnLeft, vehTurnRight, vehBraking;

	// input stuff.
	//InputPacket sync;

	/*
	DDukeActor* GetActor();
	int GetPlayerNum();

	void apply_seasick(double factor);
	void backuppos(bool noclipping = false);
	void backupweapon();
	void checkhardlanding();
	void playerweaponsway(double xvel);

	float adjustavel(float avel)
	{
		return (psectlotag == ST_2_UNDERWATER)? avel * 0.875f : avel;
	}
	*/

	native bool IsFrozen();
	native int GetGameVar(String varname, int defval);
	native void backuppos();
	native void backupxyz();
	native void setpos(Vector3 pos);
	native void addpos(Vector3 pos);
	native void settargetangle(double angle, bool backup = false);
	native double angle();
	native void clearcameras();
	native void quickkill();
	native void addPitch(double p);
	native void centerView();
	native int playerinput(int bit);


}

struct DukeWpn
{
	enum dukeweapon_t
	{
		KNEE_WEAPON,  // 0
		PISTOL_WEAPON,
		SHOTGUN_WEAPON,
		CHAINGUN_WEAPON,
		RPG_WEAPON,
		HANDBOMB_WEAPON,  // 5
		SHRINKER_WEAPON,
		DEVISTATOR_WEAPON,
		TRIPBOMB_WEAPON,
		FREEZE_WEAPON,
		HANDREMOTE_WEAPON,  // 10
		GROW_WEAPON,
		FLAMETHROWER_WEAPON,    // World Tour

		MIN_WEAPON = 0,
		MAX_WEAPON = 9,
		MAX_WEAPONS = 17
	}
}

struct RRWpn
{
	enum redneck_weapon_t
	{
		// These names have been pieced together from RedneckGDX and RedNukem because the reconstructed source recycled Duke's names for the first 11 weapons.
		// Names for 0-2 are the same
		KNEE_WEAPON,  // 0
		PISTOL_WEAPON,
		SHOTGUN_WEAPON,
		RIFLEGUN_WEAPON,
		DYNAMITE_WEAPON,
		CROSSBOW_WEAPON, // 5
		THROWSAW_WEAPON,
		ALIENBLASTER_WEAPON,
		POWDERKEG_WEAPON,
		TIT_WEAPON,
		THROWINGDYNAMITE_WEAPON, // 10
		BUZZSAW_WEAPON,
		BOWLING_WEAPON,
		MOTORCYCLE_WEAPON,
		BOAT_WEAPON,
		SLINGBLADE_WEAPON, // 15
		CHICKEN_WEAPON,
		MAX_WEAPONS
	}
}



struct DukeGameInfo native
{
	// Static constant global state
	readonly native double playerfriction;
	readonly native double gravity;

	readonly native int respawnactortime;
	readonly native int bouncemineblastradius;
	readonly native int respawnitemtime;
	readonly native int morterblastradius;
	readonly native int numfreezebounces;
	readonly native int pipebombblastradius;
	readonly native int rpgblastradius;
	readonly native int seenineblastradius;
	readonly native int shrinkerblastradius;
	readonly native int tripbombblastradius;
	readonly native int camerashitable;
	readonly native int max_player_health;
	readonly native int max_armour_amount;
	readonly native int lasermode;
	readonly native int freezerhurtowner;
	readonly native int impact_damage;
	readonly native double playerheight;
	readonly native double gutsscale;
	readonly native int displayflags;
}


struct DukeUserDefs native
{
	native readonly uint8 god, cashman, eog;
	native readonly uint8 clipping;
	native readonly uint8 user_pals[MAXPLAYERS];
	native readonly int16 from_bonus;
	native readonly int16 last_level, secretlevel;
	native readonly int const_visibility;
	native readonly int coop;
	native readonly int respawn_monsters, respawn_items, respawn_inventory, recstat, monsters_off, brightness;
	native readonly int ffire, multimode;
	native readonly int player_skill, marker, chickenplant;

	native uint8 ufospawnsminion;
	native int16 bomb_tag;
	native DukeActor cameraactor;
}
