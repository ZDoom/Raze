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
	};
	
	native void SetSpritesetImage(int index);

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
	native private int flags1, flags2;
	native walltype temp_walls[2];
	native sectortype temp_sect, actorstayput;

	native DukeActor temp_actor, seek_actor;
	native Vector3 temp_pos, temp_pos2;
	native double temp_angle;


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
	native int ifhitbyweapon();
	native int domove(int clipmask);
	native void PlayActorSound(int snd);
	native DukeActor spawn(Name type);
	native DukeActor spawnweaponorammo(int type);
	native void lotsofglass(int count);
	native void makeitfall();
	
	virtual void BeginPlay() {}
	virtual void Initialize() {}
	virtual void Tick() {}
	virtual void onHit(DukeActor hitter) {}
	virtual void onHurt(DukePlayer p) {}
	virtual void onUse(DukePlayer user) {}
	virtual bool animate(tspritetype tspr) { return false; }
	virtual void RunState() {}	// this is the CON function.
	
	native void RandomScrap();
	native void hitradius(int r, int hp1, int hp2, int hp3, int hp4);
	native double, DukeActor hitasprite();
	native void ChangeSector(sectortype s, bool forcetail = false);
	native void ChangeStat(int s, bool forcetail = false);
	

	// temporary flag accessors - need to be eliminated once we can have true actor flags
	native int actorflag1(int mask);
	native int actorflag2(int mask);
	native int attackerflag1(int mask);
	native int attackerflag2(int mask);


}

extend struct _
{
	native @DukeGameInfo gs;
	native @DukeUserDefs ud;
	native DukeLevel dlevel;
}

// The level struct is a wrapper to group all level related global variables and static functions into one object.
// On the script side we do not really want scattered global data that is publicly accessible.
struct DukeLevel
{
	native DukeActor SpawnActor(sectortype sect, Vector3 pos, class<DukeActor> type, int shade, Vector2 scale, double angle, double vel, double zvel, DukeActor owner, int stat = -1);
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
};
