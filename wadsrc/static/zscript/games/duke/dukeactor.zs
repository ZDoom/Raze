// sector effector lotags. Preferably these should be eliminated once SE's get converted to classes. Until then we still need these.
enum EEffectorTypes
{
	SE_0_ROTATING_SECTOR              = 0,
	SE_1_PIVOT                        = 1,
	SE_2_EARTHQUAKE                   = 2,
	SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT = 3,
	SE_4_RANDOM_LIGHTS                = 4,
	SE_5_BOSS                         = 5,
	SE_6_SUBWAY                       = 6,

	SE_7_TELEPORT                      = 7,
	SE_8_UP_OPEN_DOOR_LIGHTS           = 8,
	SE_9_DOWN_OPEN_DOOR_LIGHTS         = 9,
	SE_10_DOOR_AUTO_CLOSE              = 10,
	SE_11_SWINGING_DOOR                = 11,
	SE_12_LIGHT_SWITCH                 = 12,
	SE_13_EXPLOSIVE                    = 13,
	SE_14_SUBWAY_CAR                   = 14,
	SE_15_SLIDING_DOOR                 = 15,
	SE_16_REACTOR                      = 16,
	SE_17_WARP_ELEVATOR                = 17,
	SE_18_INCREMENTAL_SECTOR_RISE_FALL = 18,
	SE_19_EXPLOSION_LOWERS_CEILING     = 19,
	SE_20_STRETCH_BRIDGE               = 20,
	SE_21_DROP_FLOOR                   = 21,
	SE_22_TEETH_DOOR                   = 22,
	SE_23_ONE_WAY_TELEPORT             = 23,
	SE_24_CONVEYOR                     = 24,
	SE_25_PISTON                       = 25,
	SE_26                              = 26,
	SE_27_DEMO_CAM                     = 27,
	SE_28_LIGHTNING                    = 28,
	SE_29_WAVES                        = 29,
	SE_30_TWO_WAY_TRAIN                = 30,
	SE_31_FLOOR_RISE_FALL              = 31,
	SE_32_CEILING_RISE_FALL            = 32,
	SE_33_QUAKE_DEBRIS                 = 33,
	SE_34                              = 34,
	SE_35                              = 35,
	SE_36_PROJ_SHOOTER                 = 36,
	SE_47_LIGHT_SWITCH                 = 47,
	SE_48_LIGHT_SWITCH                 = 48,
	SE_49_POINT_LIGHT                  = 49,
	SE_50_SPOT_LIGHT                   = 50,
	SE_128_GLASS_BREAKING              = 128,
	SE_130                             = 130,
	SE_131                             = 131,
	SE_156_CONVEYOR_NOSCROLL           = 156,
};

enum ESectorTriggers
{
	ST_0_NO_EFFECT   = 0,
	ST_1_ABOVE_WATER = 1,
	ST_2_UNDERWATER  = 2,
	ST_3             = 3,
	// ^^^ maybe not complete substitution in code
	ST_9_SLIDING_ST_DOOR     = 9,
	ST_15_WARP_ELEVATOR      = 15,
	ST_16_PLATFORM_DOWN      = 16,
	ST_17_PLATFORM_UP        = 17,
	ST_18_ELEVATOR_DOWN      = 18,
	ST_19_ELEVATOR_UP        = 19,
	ST_20_CEILING_DOOR       = 20,
	ST_21_FLOOR_DOOR         = 21,
	ST_22_SPLITTING_DOOR     = 22,
	ST_23_SWINGING_DOOR      = 23,
	ST_25_SLIDING_DOOR       = 25,
	ST_26_SPLITTING_ST_DOOR  = 26,
	ST_27_STRETCH_BRIDGE     = 27,
	ST_28_DROP_FLOOR         = 28,
	ST_29_TEETH_DOOR         = 29,
	ST_30_ROTATE_RISE_BRIDGE = 30,
	ST_31_TWO_WAY_TRAIN      = 31,

	ST_41_JAILDOOR			= 41,
	ST_42_MINECART			= 42,

	ST_160_FLOOR_TELEPORT	= 160,
	ST_161_CEILING_TELEPORT = 161,

	// left: ST 32767, 65534, 65535
};

class DukeActor : CoreActor native
{
	enum EStatnums
	{
		STAT_DEFAULT        = 0,
		STAT_ACTOR          = 1,
		STAT_ZOMBIEACTOR    = 2,
		STAT_EFFECTOR       = 3,
		STAT_PROJECTILE     = 4,
		STAT_MISC           = 5,
		STAT_STANDABLE      = 6,
		STAT_LOCATOR        = 7,
		STAT_ACTIVATOR      = 8,
		STAT_TRANSPORT      = 9,
		STAT_PLAYER         = 10,
		STAT_FX             = 11,
		STAT_FALLER         = 12,
		STAT_DUMMYPLAYER    = 13,
		STAT_LIGHT          = 14,
		STAT_RAROR          = 15,

		STAT_DESTRUCT		= 100,
		STAT_BOWLING		= 105,
		STAT_CHICKENPLANT	= 106,
		STAT_LUMBERMILL		= 107,
		STAT_TELEPORT		= 108,
		STAT_BOBBING		= 118,
		STAT_RABBITSPAWN	= 119,

		STAT_REMOVED		= MAXSTATUS-2,

	};
	
	native void SetSpritesetImage(int index);
	native int GetSpritesetSize();

	native DukeActor ownerActor, hitOwnerActor;
	native uint8 cgg;
	native uint8 spriteextra;	// moved here for easier maintenance. This was originally a hacked in field in the sprite structure called 'filler'.
	native int16 /*attackertype, hitang,*/ hitextra, movflag;
	native int16 tempval; /*, dispicnum;*/
	native int16 timetosleep;
	native double floorz, ceilingz;
	native int saved_ammo;
	native int palvals;
	native int temp_data[6];
	native private int flags1, flags2, flags3;
	native walltype temp_walls[2];
	native sectortype temp_sect, actorstayput;

	native DukeActor temp_actor, seek_actor;
	native Vector3 temp_pos, temp_pos2;
	native double temp_angle;

	// this is not really usable unless all actors are properly scriptified.
	flagdef Inventory: flags1, 0;
	flagdef ShrinkAutoaim: flags1, 1;
	flagdef Badguy: flags1, 2;
	flagdef ForceAutoaim: flags1, 3;
	flagdef Boss: flags1, 4;
	flagdef Badguystayput: flags1, 5;
	flagdef GreenSlimeFood: flags1, 6;
	flagdef NoDamagePush: flags1, 7;
	flagdef NoWaterDrip: flags1, 8;
	flagdef InternalBadguy: flags1, 9;
	flagdef Killcount: flags1, 10;
	flagdef NoCanSeeCheck: flags1, 11;
	flagdef HitRadiusCheck: flags1, 12;
	flagdef MoveFTA_CheckSee: flags1, 13;
	flagdef MoveFTA_MakeStandable: flags1, 14;
	flagdef TriggerIfHitSector: flags1, 15;
	//flagdef MoveFTA_WakeupCheck: flags1, 16; // this one needs to be auto-set for RR, not for Duke, should not be exposed unless the feature becomes generally available.
	flagdef CheckSeeWithPal8: flags1, 17;
	flagdef NoShadow: flags1, 18;
	flagdef SE24_NoFloorCheck: flags1, 19;
	flagdef NoInterpolate: flags1, 20;
	
	native void getglobalz();
	native DukePlayer, double findplayer();
	native DukePlayer GetPlayer();
	native int ifhitbyweapon();
	native int domove(int clipmask);
	native int PlayActorSound(Sound snd, int chan = CHAN_AUTO, int flags = 0);
	native int CheckSoundPlaying(Sound snd, int chan = CHAN_AUTO);
	native void StopSound(Sound snd, int flags = 0);
	native DukeActor spawn(Name type);
	native DukeActor spawnsprite(int type);	// for cases where the map has a picnum stored. Avoid when possible.
	native DukeActor spawnweaponorammo(int type);
	native void lotsofglass(int count);
	native void lotsofcolourglass(int count);
	native void makeitfall();
	native void detonate(name type);
	native void checkhitdefault(DukeActor proj);
	native void operatesectors(sectortype sec);
	native int SpriteWidth();

	native void checkhitsprite(DukeActor hitter);


	
	virtual void BeginPlay() {}
	virtual void StaticSetup() {}
	virtual void Initialize() {}
	virtual void Tick() {}
	virtual void onHit(DukeActor hitter) { checkhitdefault(hitter); }
	virtual void onHurt(DukePlayer p) {}
	virtual bool onUse(DukePlayer user) { return false; }
	virtual void onTouch(DukePlayer toucher) {}
	virtual void onMotoSmash(DukePlayer toucher) {}
	virtual void onRespawn(int tag) { }
	virtual bool animate(tspritetype tspr) { return false; }
	virtual void RunState() {}	// this is the CON function.
	virtual void PlayFTASound() {}
	virtual void StandingOn(DukePlayer p) {}
	virtual bool shootthis(DukeActor actor, DukePlayer p, Vector3 pos, double ang) const // this gets called on the defaults.
	{
		return false;
	}
	
	native void RandomScrap();
	native void hitradius(int r, int hp1, int hp2, int hp3, int hp4);
	native double, DukeActor hitasprite();
	native int badguy();
	native int isplayer();
	native void lotsofstuff(Name type, int count);
	native double gutsoffset();
	native int movesprite(Vector3 move, int clipmask);
	native int movesprite_ex(Vector3 move, int clipmask, CollisionData coll);
	native void shoot(Name spawnclass);
	native void setClipDistFromTile();
	native void insertspriteq();
	native void operateforcefields(int tag);
	native void restoreloc();
	

	// temporary flag accessors - need to be eliminated once we can have true actor flags
	native int actorflag1(int mask);
	native int actorflag2(int mask);
	native int actorflag3(int mask);
	native int attackerflag1(int mask);
	native int attackerflag2(int mask);
	deprecated("4.9") native bool checktype(String name);	// this must not stay in the code, so mark it deprecated to keep the annoying warning at startup.
	
	
	void commonEnemySetup(bool countkill = true)
	{
		if (self.ownerActor != self) self.lotag = 0;

		if ((self.lotag > ud.player_skill) || ud.monsters_off == 1)
		{
			self.scale = (0, 0);
			self.ChangeStat(STAT_MISC);
		}
		else
		{
			self.makeitfall();

			self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
			if (countkill)
				Duke.GetLocalPlayer().max_actors_killed++;

			if (self.ownerActor != self)
			{
				self.timetosleep = 0;
				self.PlayFTASound();
				self.ChangeStat(STAT_ACTOR);
			}
			else self.ChangeStat(STAT_ZOMBIEACTOR);
		}
	}
	
	int checkLocationForFloorSprite(double radius)
	{
		bool away = self.isAwayFromWall(radius);
		
		if (!away)
		{
			self.scale = (0, 0); 
			self.ChangeStat(STAT_MISC); 
			return false;
		}

		if (self.sector.lotag == ST_1_ABOVE_WATER)
		{
			self.scale = (0, 0); 
			self.ChangeStat(STAT_MISC);
			return false;
		}
		return true;
	}
	
}

extend struct _
{
	native @DukeGameInfo gs;
	native @DukeUserDefs ud;
	native DukeLevel dlevel;
	native DukeActor camsprite;
}

// The level struct is a wrapper to group all level related global variables and static functions into one object.
// On the script side we do not really want scattered global data that is publicly accessible.
struct DukeLevel
{
	enum animtype_t
	{
		anim_floorz,
		anim_ceilingz,
		anim_vertexx,
		anim_vertexy,
	};

	native DukeActor SpawnActor(sectortype sect, Vector3 pos, class<DukeActor> type, int shade, Vector2 scale, double angle, double vel, double zvel, DukeActor owner, int stat = -1);
	native static int check_activator_motion(int lotag);
	native static void operatemasterswitches(int lotag);
	native static void operateactivators(int lotag, DukePlayer p);
	native static int floorflags(sectortype s);
	native static int ceilingflags(sectortype s);
	native static int wallflags(walltype s, int which);
	native static void AddCycler(sectortype sector, int lotag, int shade, int shade2, int hitag, int state);
	native static void addtorch(sectortype sector, int shade, int lotag);
	native static void addlightning(sectortype sector, int shade);
	native static int addambient(int hitag, int lotag);
	native static void resetswitch(int tag);	// 
	native static bool isMirror(walltype wal);
	native static void checkhitwall(walltype wal, DukeActor hitter, Vector3 hitpos);
	native static void checkhitceiling(sectortype wal, DukeActor hitter);
	native static DukeActor LocateTheLocator(int n, sectortype sect);
	native static int getanimationindex(int type, sectortype sec);
	native static int setanimation(sectortype animsect, int type, sectortype sec, double target, double vel);
}

struct DukeStatIterator
{
	private DukeActor nextp;
	native DukeActor Next();
	native DukeActor First(int stat);
}

struct DukeSectIterator
{
	private DukeActor nextp;
	native DukeActor Next();
	native DukeActor First(sectortype sect);
}

struct DukeSpriteIterator
{
	private DukeActor nextp;
	private int stat;
	native DukeActor Next();
	native DukeActor First();
}


// this is only temporary. We cannot check the actor flags as long as we still need to deal with internal actors whose picnum defines their type.
enum sflags_t
{
	SFLAG_INVENTORY				= 0x00000001,
	SFLAG_SHRINKAUTOAIM			= 0x00000002,
	SFLAG_BADGUY				= 0x00000004,
	SFLAG_FORCEAUTOAIM			= 0x00000008,
	SFLAG_BOSS					= 0x00000010,
	SFLAG_BADGUYSTAYPUT			= 0x00000020,
	SFLAG_GREENSLIMEFOOD		= 0x00800040,
	SFLAG_NODAMAGEPUSH			= 0x00000080,
	SFLAG_NOWATERDIP			= 0x00000100,
	SFLAG_INTERNAL_BADGUY		= 0x00000200, // a separate flag is needed for the internal ones because SFLAG_BADGUY has additional semantics.
	SFLAG_KILLCOUNT				= 0x00000400,
	SFLAG_NOCANSEECHECK			= 0x00000800,
	SFLAG_HITRADIUSCHECK		= 0x00001000,
	SFLAG_MOVEFTA_CHECKSEE		= 0x00002000,
	SFLAG_MOVEFTA_MAKESTANDABLE = 0x00004000,
	SFLAG_TRIGGER_IFHITSECTOR	= 0x00008000,
	SFLAG_MOVEFTA_WAKEUPCHECK	= 0x00010000,
	SFLAG_MOVEFTA_CHECKSEEWITHPAL8 = 0x00020000,	// let's hope this can be done better later. For now this was what blocked merging the Duke and RR variants of movefta
	SFLAG_NOSHADOW				= 0x00040000,
	SFLAG_SE24_NOCARRY			= 0x00080000,
	SFLAG_NOINTERPOLATE			= 0x00100000,
	SFLAG_FALLINGFLAMMABLE		= 0x00200000,
	SFLAG_FLAMMABLEPOOLEFFECT	= 0x00400000,
	SFLAG_INFLAME				= 0x00800000,
	SFLAG_NOFLOORFIRE			= 0x01000000,
	SFLAG_HITRADIUS_FLAG1		= 0x02000000,
	SFLAG_HITRADIUS_FLAG2		= 0x04000000,
	SFLAG_CHECKSLEEP			= 0x08000000,
	SFLAG_NOTELEPORT			= 0x10000000,
	SFLAG_SE24_REMOVE			= 0x20000000,
	SFLAG_BLOCK_TRIPBOMB		= 0x40000000,
	SFLAG_NOFALLER				= 0x80000000,
};

enum sflags2_t
{
	SFLAG2_USEACTIVATOR = 0x00000001,
	SFLAG2_NOROTATEWITHSECTOR	= 0x00000002,
	SFLAG2_SHOWWALLSPRITEONMAP	= 0x00000004,
	SFLAG2_NOFLOORPAL			= 0x00000008,
	SFLAG2_EXPLOSIVE			= 0x00000010,
	SFLAG2_BRIGHTEXPLODE		= 0x00000020,
	SFLAG2_DOUBLEDMGTHRUST		= 0x00000040,
	SFLAG2_BREAKMIRRORS			= 0x00000080,
	SFLAG2_CAMERA				= 0x00000100,
	SFLAG2_DONTANIMATE			= 0x00000200,
	SFLAG2_INTERPOLATEANGLE		= 0x00000400,
	SFLAG2_GREENBLOOD			= 0x00000800,
	SFLAG2_ALWAYSROTATE1		= 0x00001000,
	SFLAG2_DIENOW				= 0x00002000,
	SFLAG2_TRANFERPALTOJIBS		= 0x00004000,
	SFLAG2_NORADIUSPUSH			= 0x00008000,
	SFLAG2_FREEZEDAMAGE			= 0x00010000,
	SFLAG2_REFLECTIVE			= 0x00020000,
	SFLAG2_ALWAYSROTATE2		= 0x00040000,
	SFLAG2_SPECIALAUTOAIM		= 0x00080000,
	SFLAG2_NODAMAGEPUSH			= 0x00100000,
	SFLAG2_IGNOREHITOWNER		= 0x00200000,
	SFLAG2_DONTDIVE				= 0x00400000,
	SFLAG2_FLOATING				= 0x00800000,
	SFLAG2_PAL8OOZ				= 0x01000000,	// dirty hack - only needed because this needs to work from CON.
	SFLAG2_SPAWNRABBITGUTS		= 0x02000000, // this depends on the shooter, not the projectile so it has to be done with a flag.
	SFLAG2_NONSMOKYROCKET		= 0x04000000, // same with this one. Flags should later be copied to the projectile once posible.
	SFLAG2_MIRRORREFLECT		= 0x08000000,
	SFLAG2_ALTPROJECTILESPRITE	= 0x10000000, // yet another shooter flag. RRRA has some projectiles with alternative visuals, again this is on the caller thanks to CON.
	SFLAG2_UNDERWATERSLOWDOWN	= 0x20000000,

};

enum sflags3_t
{
	SFLAG3_DONTDIVEALIVE = 0x00000001,
	SFLAG3_BLOODY = 0x00000002,
	SFLAG3_BROWNBLOOD = 0x00000004,
	SFLAG3_LIGHTDAMAGE = 0x00000008,
};
