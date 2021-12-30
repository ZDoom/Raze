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

	native static void PlaySpecialMusic(int which);
	native static int PlaySound(int num, int channel = CHAN_AUTO, int flags = 0, float vol =0.8f);
	native static void StopSound(int num);
	native static bool CheckSoundPlaying(int num);
	native static DukePlayer GetViewPlayer();
	native static int MaxPlayerHealth();
	native static int MaxAmmoAmount(int weap);

	static void PlayBonusMusic()
	{
		if (System.MusicEnabled())
			PlaySound(DukeSnd.BONUSMUSIC, CHAN_AUTO, CHANF_UI);
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

struct DukePlayer 
{
	// The sound code wants to read a vector out of this so we need to define one for the main coordinate.
	/*
	union
	{
		vec3_t pos;
		struct { int32_t posx, posy, posz; };
	};

	// player's horizon and angle structs.
	PlayerHorizon horizon;
	PlayerAngle angle;

	uint16_t frags[MAXPLAYERS];
	*/

	native bool gotweapon[DukeWpn.MAX_WEAPONS];

	// Palette management uses indices into the engine's palette table now.
	native color pals;

	// this was a global variable originally.
	//vec2_t fric;

	// weapon drawer variables and their interpolation counterparts.
	native int weapon_sway;
	native int oweapon_sway;
	native int16 weapon_pos, kickback_pic, random_club_frame;
	native int16 oweapon_pos, okickback_pic, orandom_club_frame;
	native uint8 hard_landing;
	native uint8 ohard_landing;

	// Store current psectlotag as determined in processinput() for use with scaling angle aiming.
	native int16 psectlotag;

	// From here on it is unaltered from JFDuke with the exception of a few fields that are no longer needed and were removed.
	native int numloogs, loogcnt;
	native int invdisptime;
	native int pyoff, opyoff;
	native int last_pissed_time, truefz, truecz;
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
	//DDukeActor* actor, actorsqu, *wackedbyactor, *on_crane, *holoduke_on, *somethingonplayer, *access_spritenum, *dummyplayersprite, *newOwner; // later
	native voidptr holoduke_on; // cannot do it as a proper actor pointer yet - but the status bar needs it.

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
	native int noise_radius; // at286, at28a, at290
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
	void playerweaponsway(int xvel);

	float adjustavel(float avel)
	{
		return (psectlotag == ST_2_UNDERWATER)? avel * 0.875f : avel;
	}
	*/

	native bool IsFrozen();
	native int GetGameVar(String varname, int defval);


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

struct DukeSnd native
{
	// This really needs to be done better...
	enum EDukeSounds
	{
		KICK_HIT                   =  0,
		PISTOL_RICOCHET            =  1,
		PISTOL_BODYHIT             =  2,
		PISTOL_FIRE                =  3,
		EJECT_CLIP                 =  4,
		INSERT_CLIP                =  5,
		CHAINGUN_FIRE              =  6,
		RPG_SHOOT                  =  7,
		POOLBALLHIT                =  8,
		RPG_EXPLODE                =  9,
		CAT_FIRE                   = 10,
		SHRINKER_FIRE              = 11,
		ACTOR_SHRINKING            = 12,
		PIPEBOMB_BOUNCE            = 13,
		PIPEBOMB_EXPLODE           = 14,
		LASERTRIP_ONWALL           = 15,
		LASERTRIP_ARMING           = 16,
		LASERTRIP_EXPLODE          = 17,
		VENT_BUST                  = 18,
		GLASS_BREAKING             = 19,
		GLASS_HEAVYBREAK           = 20,
		SHORT_CIRCUIT              = 21,
		ITEM_SPLASH                = 22,
		DUKE_BREATHING             = 23,
		DUKE_EXHALING              = 24,
		DUKE_GASP                  = 25,
		SLIM_RECOG                 = 26,

		DUKE_URINATE               = 28,
		ENDSEQVOL3SND2             = 29,
		ENDSEQVOL3SND3             = 30,
		DUKE_PASSWIND              = 32,
		DUKE_CRACK                 = 33,
		SLIM_ATTACK                = 34,
		SOMETHINGHITFORCE          = 35,
		DUKE_DRINKING              = 36,
		DUKE_KILLED1               = 37,
		DUKE_GRUNT                 = 38,
		DUKE_HARTBEAT              = 39,
		DUKE_ONWATER               = 40,
		DUKE_DEAD                  = 41,
		DUKE_LAND                  = 42,
		DUKE_WALKINDUCTS           = 43,
		DUKE_GLAD                  = 44,
		DUKE_YES                   = 45,
		DUKE_HEHE                  = 46,
		DUKE_SHUCKS                = 47,
		DUKE_UNDERWATER            = 48,
		DUKE_JETPACK_ON            = 49,
		DUKE_JETPACK_IDLE          = 50,
		DUKE_JETPACK_OFF           = 51,
		LIZTROOP_GROWL             = 52,
		LIZTROOP_TALK1             = 53,
		LIZTROOP_TALK2             = 54,
		LIZTROOP_TALK3             = 55,
		DUKETALKTOBOSS             = 56,
		LIZCAPT_GROWL              = 57,
		LIZCAPT_TALK1              = 58,
		LIZCAPT_TALK2              = 59,
		LIZCAPT_TALK3              = 60,
		LIZARD_BEG                 = 61,
		LIZARD_PAIN                = 62,
		LIZARD_DEATH               = 63,
		LIZARD_SPIT                = 64,
		DRONE1_HISSRATTLE          = 65,
		DRONE1_HISSSCREECH         = 66,
		DUKE_TIP2                  = 67,
		FLESH_BURNING              = 68,
		SQUISHED                   = 69,
		TELEPORTER                 = 70,
		ELEVATOR_ON                = 71,
		DUKE_KILLED3               = 72,
		ELEVATOR_OFF               = 73,
		DOOR_OPERATE1              = 74,
		SUBWAY                     = 75,
		SWITCH_ON                  = 76,
		FAN                        = 77,
		DUKE_GETWEAPON3            = 78,
		FLUSH_TOILET               = 79,
		HOVER_CRAFT                = 80,
		EARTHQUAKE                 = 81,
		INTRUDER_ALERT             = 82,
		END_OF_LEVEL_WARN          = 83,
		ENGINE_OPERATING           = 84,
		REACTOR_ON                 = 85,
		COMPUTER_AMBIENCE          = 86,
		GEARS_GRINDING             = 87,
		BUBBLE_AMBIENCE            = 88,
		MACHINE_AMBIENCE           = 89,
		SEWER_AMBIENCE             = 90,
		WIND_AMBIENCE              = 91,
		SOMETHING_DRIPPING         = 92,
		STEAM_HISSING              = 93,
		THEATER_BREATH             = 94,
		BAR_MUSIC                  = 95,
		BOS1_ROAM                  = 96,
		BOS1_RECOG                 = 97,
		BOS1_ATTACK1               = 98,
		BOS1_PAIN                  = 99,
		BOS1_DYING                 =100,
		BOS2_ROAM                  =101,
		BOS2_RECOG                 =102,
		BOS2_ATTACK                =103,
		BOS2_PAIN                  =104,
		BOS2_DYING                 =105,
		GETATOMICHEALTH            =106,
		DUKE_GETWEAPON2            =107,
		BOS3_DYING                 =108,
		SHOTGUN_FIRE               =109,
		PRED_ROAM                  =110,
		PRED_RECOG                 =111,
		PRED_ATTACK                =112,
		PRED_PAIN                  =113,
		PRED_DYING                 =114,
		CAPT_ROAM                  =115,
		CAPT_ATTACK                =116,
		CAPT_RECOG                 =117,
		CAPT_PAIN                  =118,
		CAPT_DYING                 =119,
		PIG_ROAM                   =120,
		PIG_RECOG                  =121,
		PIG_ATTACK                 =122,
		PIG_PAIN                   =123,
		PIG_DYING                  =124,
		RECO_ROAM                  =125,
		RECO_RECOG                 =126,
		RECO_ATTACK                =127,
		RECO_PAIN                  =128,
		RECO_DYING                 =129,
		DRON_ROAM                  =130,
		DRON_RECOG                 =131,
		DRON_ATTACK1               =132,
		DRON_PAIN                  =133,
		DRON_DYING                 =134,
		COMM_ROAM                  =135,
		COMM_RECOG                 =136,
		COMM_ATTACK                =137,
		COMM_PAIN                  =138,
		COMM_DYING                 =139,
		OCTA_ROAM                  =140,
		OCTA_RECOG                 =141,
		OCTA_ATTACK1               =142,
		OCTA_PAIN                  =143,
		OCTA_DYING                 =144,
		TURR_ROAM                  =145,
		TURR_RECOG                 =146,
		TURR_ATTACK                =147,
		DUMPSTER_MOVE              =148,
		SLIM_DYING                 =149,
		BOS3_ROAM                  =150,
		BOS3_RECOG                 =151,
		BOS3_ATTACK1               =152,
		BOS3_PAIN                  =153,
		BOS1_ATTACK2               =154,
		COMM_SPIN                  =155,
		BOS1_WALK                  =156,
		DRON_ATTACK2               =157,
		THUD                       =158,
		OCTA_ATTACK2               =159,
		WIERDSHOT_FLY              =160,
		TURR_PAIN                  =161,
		TURR_DYING                 =162,
		SLIM_ROAM                  =163,
		LADY_SCREAM                =164,
		DOOR_OPERATE2              =165,
		DOOR_OPERATE3              =166,
		DOOR_OPERATE4              =167,
		BORNTOBEWILDSND            =168,
		SHOTGUN_COCK               =169,
		GENERIC_AMBIENCE1          =170,
		GENERIC_AMBIENCE2          =171,
		GENERIC_AMBIENCE3          =172,
		GENERIC_AMBIENCE4          =173,
		GENERIC_AMBIENCE5          =174,
		GENERIC_AMBIENCE6          =175,
		BOS3_ATTACK2               =176,
		GENERIC_AMBIENCE17         =177,
		GENERIC_AMBIENCE18         =178,
		GENERIC_AMBIENCE19         =179,
		GENERIC_AMBIENCE20         =180,
		GENERIC_AMBIENCE21         =181,
		GENERIC_AMBIENCE22         =182,
		SECRETLEVELSND             =183,
		GENERIC_AMBIENCE8          =184,
		GENERIC_AMBIENCE9          =185,
		GENERIC_AMBIENCE10         =186,
		GENERIC_AMBIENCE11         =187,
		GENERIC_AMBIENCE12         =188,
		GENERIC_AMBIENCE13         =189,
		GENERIC_AMBIENCE14         =190,
		GENERIC_AMBIENCE15         =192,
		GENERIC_AMBIENCE16         =193,
		FIRE_CRACKLE               =194,
		BONUS_SPEECH1              =195,
		BONUS_SPEECH2              =196,
		BONUS_SPEECH3              =197,
		PIG_CAPTURE_DUKE           =198,
		BONUS_SPEECH4              =199,
		DUKE_LAND_HURT             =200,
		DUKE_HIT_STRIPPER1         =201,
		DUKE_TIP1                  =202,
		DUKE_KILLED2               =203,
		PRED_ROAM2                 =204,
		PIG_ROAM2                  =205,
		DUKE_GETWEAPON1            =206,
		DUKE_SEARCH2               =207,
		DUKE_CRACK2                =208,
		DUKE_SEARCH                =209,
		DUKE_GET                   =210,
		DUKE_LONGTERM_PAIN         =211,
		MONITOR_ACTIVE             =212,
		NITEVISION_ONOFF           =213,
		DUKE_HIT_STRIPPER2         =214,
		DUKE_CRACK_FIRST           =215,
		DUKE_USEMEDKIT             =216,
		DUKE_TAKEPILLS             =217,
		DUKE_PISSRELIEF            =218,
		SELECT_WEAPON              =219,
		WATER_GURGLE               =220,
		DUKE_GETWEAPON4            =221,
		JIBBED_ACTOR1              =222,
		JIBBED_ACTOR2              =223,
		JIBBED_ACTOR3              =224,
		JIBBED_ACTOR4              =225,
		JIBBED_ACTOR5              =226,
		JIBBED_ACTOR6              =227,
		JIBBED_ACTOR7              =228,
		DUKE_GOTHEALTHATLOW        =229,
		BOSSTALKTODUKE             =230,
		WAR_AMBIENCE1              =231,
		WAR_AMBIENCE2              =232,
		WAR_AMBIENCE3              =233,
		WAR_AMBIENCE4              =234,
		WAR_AMBIENCE5              =235,
		WAR_AMBIENCE6              =236,
		WAR_AMBIENCE7              =237,
		WAR_AMBIENCE8              =238,
		WAR_AMBIENCE9              =239,
		WAR_AMBIENCE10             =240,
		ALIEN_TALK1                =241,
		ALIEN_TALK2                =242,
		EXITMENUSOUND              =243,
		FLY_BY                     =244,
		DUKE_SCREAM                =245,
		SHRINKER_HIT               =246,
		RATTY                      =247,
		INTO_MENU                  =248,
		BONUSMUSIC                 =249,
		DUKE_BOOBY                 =250,
		DUKE_TALKTOBOSSFALL        =251,
		DUKE_LOOKINTOMIRROR        =252,
		PIG_ROAM3                  =253,
		KILLME                     =254,
		DRON_JETSND                =255,
		SPACE_DOOR1                =256,
		SPACE_DOOR2                =257,
		SPACE_DOOR3                =258,
		SPACE_DOOR4                =259,
		SPACE_DOOR5                =260,
		ALIEN_ELEVATOR1            =261,
		VAULT_DOOR                 =262,
		JIBBED_ACTOR13             =263,
		DUKE_GETWEAPON6            =264,
		JIBBED_ACTOR8              =265,
		JIBBED_ACTOR9              =266,
		JIBBED_ACTOR10             =267,
		JIBBED_ACTOR11             =268,
		JIBBED_ACTOR12             =269,
		DUKE_KILLED4               =270,
		DUKE_KILLED5               =271,
		ALIEN_SWITCH1              =272,
		DUKE_STEPONFECES           =273,
		DUKE_LONGTERM_PAIN2        =274,
		DUKE_LONGTERM_PAIN3        =275,
		DUKE_LONGTERM_PAIN4        =276,
		COMPANB2                   =277,
		KTIT                       =278,
		HELICOP_IDLE               =279,
		STEPNIT                    =280,
		SPACE_AMBIENCE1            =281,
		SPACE_AMBIENCE2            =282,
		SLIM_HATCH                 =283,
		RIPHEADNECK                =284,
		FOUNDJONES                 =285,
		ALIEN_DOOR1                =286,
		ALIEN_DOOR2                =287,
		ENDSEQVOL3SND4             =288,
		ENDSEQVOL3SND5             =289,
		ENDSEQVOL3SND6             =290,
		ENDSEQVOL3SND7             =291,
		ENDSEQVOL3SND8             =292,
		ENDSEQVOL3SND9             =293,
		WHIPYOURASS                =294,
		ENDSEQVOL2SND1             =295,
		ENDSEQVOL2SND2             =296,
		ENDSEQVOL2SND3             =297,
		ENDSEQVOL2SND4             =298,
		ENDSEQVOL2SND5             =299,
		ENDSEQVOL2SND6             =300,
		ENDSEQVOL2SND7             =301,
		GENERIC_AMBIENCE23         =302,
		SOMETHINGFROZE             =303,
		DUKE_LONGTERM_PAIN5        =304,
		DUKE_LONGTERM_PAIN6        =305,
		DUKE_LONGTERM_PAIN7        =306,
		DUKE_LONGTERM_PAIN8        =307,
		WIND_REPEAT                =308,
		MYENEMY_ROAM               =309,
		MYENEMY_HURT               =310,
		MYENEMY_DEAD               =311,
		MYENEMY_SHOOT              =312,
		STORE_MUSIC                =313,
		STORE_MUSIC_BROKE          =314,
		ACTOR_GROWING              =315,
		NEWBEAST_ROAM              =316,
		NEWBEAST_RECOG             =317,
		NEWBEAST_ATTACK            =318,
		NEWBEAST_PAIN              =319,
		NEWBEAST_DYING             =320,
		NEWBEAST_SPIT              =321,
		VOL4_1                     =322,
		SUPERMARKET                =323,
		MOUSEANNOY                 =324,
		BOOKEM                     =325,
		SUPERMARKETCRY             =326,
		DESTRUCT                   =327,
		EATFOOD                    =328,
		MAKEMYDAY                  =329,
		WITNESSSTAND               =330,
		VACATIONSPEECH             =331,
		YIPPEE1                    =332,
		YOHOO1                     =333,
		YOHOO2                     =334,
		DOLPHINSND                 =335,
		TOUGHGALSND1               =336,
		TOUGHGALSND2               =337,
		TOUGHGALSND3               =338,
		TOUGHGALSND4               =339,
		TANK_ROAM                  =340,
		BOS4_ROAM                  =341,
		BOS4_RECOG                 =342,
		BOS4_ATTACK                =343,
		BOS4_PAIN                  =344,
		BOS4_DYING                 =345,
		NEWBEAST_ATTACKMISS        =346,
		VOL4_2                     =347,
		COOKINGDEEPFRIER           =348,
		WHINING_DOG                =349,
		DEAD_DOG                   =350,
		LIGHTNING_SLAP             =351,
		THUNDER                    =352,
		HAPPYMOUSESND1             =353,
		HAPPYMOUSESND2             =354,
		HAPPYMOUSESND3             =355,
		HAPPYMOUSESND4             =356,
		ALARM                      =357,
		RAIN                       =358,
		DTAG_GREENRUN              =359,
		DTAG_BROWNRUN              =360,
		DTAG_GREENSCORE            =361,
		DTAG_BROWNSCORE            =362,
		INTRO4_1                   =363,
		INTRO4_2                   =364,
		INTRO4_3                   =365,
		INTRO4_4                   =366,
		INTRO4_5                   =367,
		INTRO4_6                   =368,
		SCREECH                    =369,
		BOSS4_DEADSPEECH           =370,
		BOSS4_FIRSTSEE             =371,
		PARTY_SPEECH               =372,
		POSTAL_SPEECH              =373,
		TGSPEECH                   =374,
		DOGROOMSPEECH              =375,
		SMACKED                    =376,
		MDEVSPEECH                 =377,
		AREA51SPEECH               =378,
		JEEPSOUND                  =379,
		BIGDOORSLAM                =380,
		BOS4_LAY                   =381,
		WAVESOUND                  =382,
		ILLBEBACK                  =383,
		VOL4ENDSND1                =384,
		VOL4ENDSND2                =385,
		EXPANDERHIT                =386,
		SNAKESPEECH                =387,
		EXPANDERSHOOT              =388,
		GETBACKTOWORK              =389,
		JIBBED_ACTOR14             =390,
		JIBBED_ACTOR15             =391,
		INTRO4_B                   =392,
		BIGBANG                    =393,
		SMACKIT                    =394,
		BELLSND                    =395,
		GOAWAY                     =396,
		JOKE                       =397,
		FLAMETHROWER_INTRO         =398,
		FLAMETHROWER_LOOP          =399,
		FLAMETHROWER_END           =400,
		E5L7_DUKE_QUIT_YOU         =401,
	}
}

struct RRSnd native
{
	enum ESnd
	{
		KICK_HIT     = 0  ,
		RICOCHET     = 1  ,
		BULITHIT     = 2  ,
		CASUL_FIRE   = 3  ,
		PISCOCK      = 4  ,
		PISLOAD      = 5  ,
		AK3          = 6  ,
		XBOWFIRE     = 7  ,
		BUB_HRT1     = 8  ,
		XBOWEXPL     = 9  ,
		LASERA       = 10 ,
		SHRINKER     = 11 ,
		CRAPFLOW     = 12 ,
		DYNOCLMP     = 13 ,
		DYNEW        = 14 ,
		CRAPSTIR     = 15 ,
		BRICDOOR     = 16 ,
		BOMBEXPL     = 17 ,
		VENTBUST     = 18 ,
		GLASSSND     = 19 ,
		GLASSHVY     = 20 ,
		BUBBLES      = 21 ,
		SPLASH       = 22 ,
		BUB_HRT2     = 23 ,
		BUB_HRT3     = 24 ,
		GASP         = 25 ,
		BUB_HRT4     = 26 ,

		ONECART      = 27 , // RR
		MINEWIND     = 28 ,
		URANUS       = 29 ,

		MIRROR1      = 27 , // RRRA
		MIRROR2      = 28 ,
		MIRROR3      = 29 ,

		COMPUTER     = 30 ,
		NEON         = 31 ,
		VX_FINAL     = 32 ,
		LN_WAIT      = 33 ,
		BUB_LN1      = 34 ,
		LN_FINAL     = 35 ,
		CLOCKTK      = 36 ,
		LN_STANK     = 37 ,
		LNRD_GRUNT   = 38 ,
		CLOCKCHM     = 39 ,
		WETFEET      = 40 ,
		LNRD_DEAD    = 41 ,
		LAND         = 42 ,
		END_PIPE     = 43 ,
		ICARUMBA     = 44 ,
		BUB_LN2      = 45 ,
		LN_CRAP      = 46 ,
		WOODBREK     = 47 ,
		SCUBA        = 48 ,
		TRUCK_LP2    = 49 ,
		COW1         = 50 ,
		COW2         = 51 ,
		COW3         = 52 ,
		COW4         = 53 ,
		COW5         = 54 ,
		BUB_LN3      = 55 ,
		LATCH        = 56 ,
		BUB_LN5      = 57 ,
		BUB_LN6      = 58 ,
		BUB_LN7      = 59 ,
		BUB_PIK1     = 60 ,
		BUB_PIK2     = 61 ,
		BUB_PISS     = 62 ,
		E1L1         = 63 ,
		E1L2         = 64 ,
		UFOINSID     = 65 ,
		LN_RODE      = 66 ,
		CURTAIN      = 67 ,
		FIRE09       = 68 ,
		SQUISHED     = 69 ,
		TELEPORT     = 70 ,
		GBELEV01     = 71 ,
		LN_BNCH      = 72 ,
		GBELEV02     = 73 ,
		FROG1        = 74 ,
		TRUCK_LP     = 75 ,
		SWITCH1      = 76 ,
		E1L3         = 77 ,
		LN_HOTDM     = 78 ,
		FLUSH        = 79 ,
		E1L4         = 80 ,
		QUAKE        = 81 ,
		CHKAMMO      = 82 ,
		MONITORA     = 83 ,
		FROG2        = 84 ,
		AS_AMB2      = 85 ,
		AS_AMB1      = 86 ,
		FBOATIDL     = 87 ,
		FBOATRUN     = 88 ,
		FBOATUP      = 89 ,
		FBOATDN      = 90 ,
		FBOATTRN     = 91 ,
		DRIP3        = 92 ,
		SWAMPAMB     = 93 ,
		MORTAR       = 94 ,
		JUKEBOX      = 95 ,
		AS_DROPN     = 96 ,
		AS_CRYPT     = 97 ,
		AS_DRCLS     = 98 ,
		LOKGATE      = 99 ,
		METLGAT2     = 100,
		METLGAT1     = 101,
		E1L5         = 102,
		E1L6         = 103,
		E1L7         = 104,
		E2L1         = 105,
		PADDLE       = 106,
		LN_HOLD      = 107,
		VX_TAKIT     = 108,
		SHOT6        = 109,
		CT_LAF2      = 110,
		CT_GET       = 111,
		CT_LAF       = 112,
		CT_PAIN      = 113,
		CT_DIE       = 114,
		PIGSOUND1    = 115,
		PIGSOUND2    = 116,
		PIGSOUND3    = 117,
		PIGSOUND4    = 118,
		PIGSOUND5    = 119,
		BR_ROAM1     = 120,
		BR_RECOG     = 121,
		WHISTLE      = 122,
		BR_PAIN      = 123,
		BR_DTH       = 124,
		VX_ISTHT     = 125,
		LASERH       = 126,
		PIGSOUND6    = 127,
		PIGSOUND7    = 128,
		VX_DIE1      = 129,
		MJ_JIB1      = 130,
		VX_DIE4      = 131,
		VX_DIE5      = 132,
		VX_DIE6      = 133,
		VX_DIE7      = 134,
		VX_OOH       = 135,
		VX_PAIN1     = 136,
		VX_SEX1      = 137,
		VX_SEX2      = 138,
		VX_SEX3      = 139,
		VX_GRNT      = 140,
		RENO         = 141,
		BK_MAKE1     = 142,
		BK_MAKE2     = 143,
		VX_BRNG3     = 144,
		VX_CLSR1     = 145,
		VX_CLSR2     = 146,
		VX_2FAR      = 147,
		KINGHUH      = 148,
		VX_BRING     = 149,
		VX_BITE      = 150,
		MJ_FART      = 151,
		VX_LAFF2     = 152,
		VX_LAFF3     = 153,
		VX_HMMM2     = 154,
		VX_HURT2     = 155,
		VX_BABY2     = 156,
		VX_MHMM      = 157,
		THUD         = 158,
		VX_ITSOK     = 159,
		MJ_RECO2     = 160,
		// VX_TPOT1  = 161,
		VX_TPOT4     = 162,
		// VX_TPIN1  = 163,
		ROPECRK      = 164,
		DR_CRK8      = 165,
		DR_ROLL      = 166,
		STEELAMB     = 167,
		ROULETTE     = 168,
		GUNCHANG     = 169,
		FLIES        = 170,
		AMB_1        = 171,
		GRAVAMB      = 172,
		HOOTOWL      = 173,
		WOODS2       = 174,
		CATAMB       = 175,
		E2L2         = 176,
		E2L3         = 177,
		FBOATX_1     = 178,
		FBOATX_2     = 179,
		FBOATX_3     = 180,
		FBOATX_4     = 181,
		FBOATSLW     = 182,
		PLANE        = 183,
		CNTAMB       = 184,
		JUNKAMB2     = 185,
		BIKESTRT     = 186,
		BIKEIDLE     = 187,
		BIKELOOP     = 188,
		BIKEJMPS     = 189,
		BIKEJMPL     = 190,
		BIKELAND     = 191,
		JACKJMP1     = 192,
		JACKJMP2     = 193,
		FIRE_CRACKLE = 194,
		BNS_SPCH1    = 195,
		BNS_SPCH2    = 196,
		BNS_SPCH3    = 197,
		E2L4         = 198,
		BNS_SPCH4    = 199,
		LN_LNDHT     = 200,
		JACKATK2     = 201,
		JACKPAIN     = 202,
		LN_BITCH     = 203,
		CT_LAND      = 204,
		BR_ROAM2     = 205,
		LN_HUSH      = 206,
		LN_PAIN4     = 207,
		LN_SLOW      = 208,
		LN_PAIN4A    = 209,
		JUG          = 210,
		LN_PAIN8     = 211,
		MONITOR      = 212,
		JACKATK1     = 213,
		BIKEUP       = 214,
		PLANEXP      = 215,
		JUGALUG7     = 216,
		DIDDLP       = 217,
		ELVISMOD     = 218,
		// PISCOCK  =  219,
		BIKESKID     = 220,
		LN_STINK     = 221,
		JIBBED1      = 222,
		JIBBED2      = 223,
		JIBBED3      = 224,
		JIBBED4      = 225,
		JIBBED5      = 226,
		JIBBED6      = 227,
		JIBBED7      = 228,
		LN_BACON     = 229,
		E2L5         = 230,
		REGISTER     = 231,
		BBQGRILL     = 232,
		CRSSBELL     = 233,
		TRAIN        = 234,
		SLOTS        = 235,
		INDIANS      = 236,
		RADIO        = 237,
		BIKEX_1      = 238,
		BIKEX_2      = 239,
		BIKEX_3      = 240,
		TVSNOW       = 241,
		WINDLITE     = 242,
		EXITMENU     = 243,
		CHKBOWFR     = 244,
		DSCREM04     = 245,
		SHRNK_HIT    = 246,
		CHKBOWEX     = 247,
		INTOMENU     = 248,
		LAVAFLOW     = 249,
		LAVA_RK      = 250,
		BIKELOO2     = 251,
		SLINGBL      = 252,
		BR_ROAM3     = 253,
		KILLME       = 254,
		E2L6         = 255,
		RINTRO       = 256,
		MIRROR4      = 257,
		MIRROR5      = 258,
		GAMBELEV     = 259,
		SLINGHIT     = 260,
		PIANO_P1     = 261,
		BANJO1       = 262,
		JIBBED13     = 263,
		LN_BBQ       = 264,
		JIBBED8      = 265,
		JIBBED9      = 266,
		JIBBED10     = 267,
		JIBBED11     = 268,
		JIBBED12     = 269,
		LNRD_KILLED4 = 270,
		LNRD_KILLED5 = 271,
		BANJO2       = 272,
		BANJO3       = 273,
		LN_PAIN2     = 274,
		LN_PAIN3     = 275,
		BK_ALIVE     = 276,
		BK_BOURB     = 277,
		BK_CHEER     = 278,
		BK_DIENB     = 279,
		BK_DNTLK     = 280,
		BK_FUN       = 281,
		BK_HEY       = 282,
		E2L7         = 283,
		BK_HEYNB     = 284,
		BK_JOYRD     = 285,
		BK_KEEPA     = 286,
		BK_PLEAS     = 287,
		BK_RIDE      = 288,
		BK_ROAD      = 289,
		BK_SCRAT     = 290,
		BK_SHTUP     = 291,
		BK_SNORT     = 292,
		BK_TOHEL     = 293,
		WHIPYOU      = 294,
		BK_TRYIN     = 295,
		BK_PAIN1     = 296,
		BK_PAIN2     = 297,
		BK_PAIN3     = 298,
		CH_BALD      = 299,
		CH_TEAS1     = 300,
		CH_TEAS2     = 301,
		CH_TEAS3     = 302,
		CH_SANDP     = 303,
		LN_PAIN5     = 304,
		LN_PAIN6     = 305,
		LN_PAIN7     = 306,
		CH_DONIT     = 307,
		CH_WHOOP     = 308,
		CH_NIPPL     = 309,
		CH_BARN      = 310,
		CH_GTEAM     = 311,
		CH_GOGOG     = 312,
		CH_REDOK     = 313,
		CH_2468      = 314,
		CH_BIGON     = 315,
		HULK_ROAM    = 316,
		HULK_RECOG   = 317,
		HULK_ATTACK  = 318,
		HULK_PAIN    = 319,
		HULK_DYING   = 320,
		HULK_SPIT    = 321,
		CH_PAIN1     = 322,
		CH_PAIN2     = 323,
		CH_PAIN3     = 324,
		CH_HURT      = 325,
		AK4          = 326,
		CHKSCR1      = 327,
		SHIPWREK     = 328,
		HYDROGLY     = 329,
		PIANO_P2     = 330,
		FROGTOSS     = 331,
		TRAIN2       = 332,
		CRICKET1     = 333,
		CRICKET2     = 334,
		PIGRUNT      = 335,
		GOCATGO      = 336,
		ANNOUNC1     = 337,
		ANNOUNC2     = 338,
		TRACTOR      = 339,
		PIANO_P3     = 340,
		RESIZE       = 341,
		VX_TPIN2     = 342,
		VX_TPIN4     = 343,
		VX_HLPME     = 344,
		ATFSPEAK     = 345,
		WINDCAVE     = 346,
		ALARM        = 347,
		SF_THLAW     = 348,
		SF_TLAW2     = 349,
		LN_SCREW     = 350,
		THUNDER1     = 351,
		THUNDER2     = 352,
		THUNDER3     = 353,
		BOWLSTRT     = 354,
		BOWLPIN      = 355,
		BOWLLOOP     = 356,
		MJ_JIB2      = 357,
		VX_KICK2     = 358,
		VX_KICK3     = 359,
		MJ_RECO1     = 360,
		VX_HIYA      = 361,
		VX_HIYA2     = 362,
		SF_ATTN      = 363,
		SF_DETH1     = 364,
		SF_DETH2     = 365,
		SF_DETH3     = 366,
		TEDOUT       = 367,
		SF_FREZ2     = 368,
		SF_GETYA     = 369,
		SF_HANDS     = 370,
		STEELAM2     = 371,
		STEELAM3     = 372,
		SF_HEY       = 373,
		SF_HOLD      = 374,
		SF_LAFF1     = 375,
		LN_FLYOP     = 376,
		LN_SHTHD     = 377,
		SF_NAME      = 378,
		SF_OVER      = 379,
		SF_PAIN1     = 380,
		SF_PAIN2     = 381,
		SF_PAIN3     = 382,
		SF_RLOAD     = 383,
		SF_RLOD2     = 384,
		SF_SHOOT     = 385,
		JAWHARP      = 386,
		LN_TIGHT     = 387,
		DR_CLS       = 388,
		SCRAPE_1     = 389,
		YEHAA16      = 390,
		LN_WHUP      = 391,
		CHKNFLAP     = 392,
		CHKN_1       = 393,
		CHKN_2       = 394,
		CHIKDETH     = 395,
		AMB_ROOM     = 396,
		BR_ITCH      = 397,
		BR_SCRTH     = 398,
		BR_SNIFF     = 399,
		TRUKDIE      = 400,
		ZIPOPEN      = 401,
		ZIPPSTRK     = 402,
		MOSQUI4      = 403,
		FART1        = 404,
		SWITCH2      = 405,
		SQUEAKY      = 406,
		CATDOOR      = 407,
		JUNKSWCH     = 408,
		CONVEYR      = 409,
		SWITCH3      = 410,
		BIKEENEM     = 411,
		BIGDOOR      = 412,
		FLOODGAT     = 413,
		JACK_RM1     = 414,
		MN_FREAK     = 415,
		MN_PN        = 416,
		MN_REC       = 417,
		MN_AMB       = 418,
		LOKDOOR      = 419,
		VOMIT        = 420,
		TOSS         = 421,
		FART2        = 422,
		FART3        = 423,
		FART4        = 424,
		CHUG         = 425,
		CROWUSH      = 426,
		WUSSLAF      = 427,
		LN_CITY      = 428,
		MUNCH2       = 429,
		TESLARC      = 430,
		BUZSAWSND    = 431,
		ELEVLOOP     = 432,
		PISSEND      = 433,
		PISSLOOP     = 434,
		PISSSTRT     = 435,
		CRAP         = 436,
		PEE          = 437,
		JACK_RM2     = 438,
		BELL         = 439,
		TRAINWRK     = 440,
		DOOR_PKT     = 441,
		GAMBDOOR     = 442,
		OVEN         = 443,
		CREMATOR     = 444,
		JOE9000A     = 445,
		JOE9000B     = 446,
		JOE9000C     = 447,
		CHINESE      = 448,
		SIGNROT      = 449,
		XBOWCOCK     = 450,
		PWDERKEG     = 451,
		DG_BARK1     = 452,
		DG_GRWL1     = 453,
		DG_YELP      = 454,
		DG_DIE       = 455,
		UFO          = 456,
		UFOLET       = 457,
		JACKJIB1     = 458,
		JACKJIB2     = 459,
		JACKJIB3     = 460,
		JACKJIB4     = 461,
		JACKJIB5     = 462,
		WTRFALL      = 463,
		BK_JIB1      = 464,
		FRIDGEOP     = 465,
		FRIDGECL     = 466,
		DG_LUNGE     = 467,
		DRIVTHRU     = 468,
		FAN          = 469,
		CRUSHER      = 470,
		BALLOON      = 471,
		POOLBUD      = 472,
		STAMPER      = 473,
		BK_JIB2      = 474,
		MORNING      = 475,
		DG_BARK2     = 476,
		DG_GRWL2     = 477,
		REDNECK2     = 478,
		XATRIX       = 479,
		MJ_ATTK1     = 480,
		MJ_JUMP      = 485,
		MJ_PAIN1     = 481,
		MJ_PAIN2     = 482,
		MJ_ROAM1     = 483,
		MJ_ROAM2     = 484,
		MJ_ROLL      = 486,
		DISHES       = 487,
		BUB_ELV1     = 488,
		BUB_ELV2     = 489,
		BUB_ELV3     = 490,
		BK_JIB3      = 491,
		CH_JIB1      = 492,
		CH_JIB2      = 493,
		CH_JIB3      = 494,
		SIGNHIT      = 495,
		UMHUM        = 496,
		COYOTE       = 497,
		BUB_HEY1     = 498,
		BUB_HEY2     = 499,
	}
}
