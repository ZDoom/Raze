
enum EGameType
{
	GAMEFLAG_DUKE       = 0x00000001,
	GAMEFLAG_NAM        = 0x00000002,
	GAMEFLAG_NAPALM     = 0x00000004,
	GAMEFLAG_WW2GI      = 0x00000008,
	GAMEFLAG_ADDON      = 0x00000010,
	GAMEFLAG_SHAREWARE  = 0x00000020,
	GAMEFLAG_DUKEBETA   = 0x00000060, // includes 0x20 since it's a shareware beta
	GAMEFLAG_PLUTOPAK	= 0x00000080,
	GAMEFLAG_RR         = 0x00000100,
	GAMEFLAG_RRRA       = 0x00000200,
	GAMEFLAG_RRALL		= GAMEFLAG_RR | GAMEFLAG_RRRA,
	GAMEFLAG_BLOOD      = 0x00000800,
	GAMEFLAG_SW			= 0x00001000,
	GAMEFLAG_POWERSLAVE	= 0x00002000,
	GAMEFLAG_EXHUMED	= 0x00004000,
	GAMEFLAG_PSEXHUMED  = GAMEFLAG_POWERSLAVE | GAMEFLAG_EXHUMED,	// the two games really are the same, except for the name and the publisher.
	GAMEFLAG_WORLDTOUR	= 0x00008000,
	GAMEFLAG_DUKEDC		= 0x00010000,
	GAMEFLAG_DUKENW		= 0x00020000,
	GAMEFLAG_DUKEVACA	= 0x00040000,
	GAMEFLAG_BLOODCP	= 0x00080000,
	GAMEFLAG_ROUTE66	= 0x00100000,
	GAMEFLAG_SWWANTON	= 0x00200000,
	GAMEFLAG_SWTWINDRAG	= 0x00400000,
	GAMEFLAGMASK        = 0x0000FFFF, // flags allowed from grpinfo

	// We still need these for the parsers.
	GAMEFLAG_FURY = 0,
	GAMEFLAG_DEER = 0,

};

enum AM_Mode
{
	am_off,
	am_overlay,
	am_full,
	am_count
}

enum EHudSize
{
	Hud_Current = -1,
	Hud_Frame50 = 0,
	Hud_Frame60,
	Hud_Frame70,
	Hud_Frame80,
	Hud_Frame90,
	Hud_Stbar,
	Hud_StbarOverlay,
	Hud_Mini,
	Hud_Full,
	Hud_Althud,
	Hud_Nothing,
	Hud_MAX
}

struct UserConfigStruct native
{
	native readonly bool nomonsters;
	native readonly bool nosound;
	native readonly bool nologo;
}

extend struct _
{
	native @UserConfigStruct userConfig;
	native readonly MapRecord currentLevel;
	native readonly int automapMode;
	native readonly int PlayClock;
	
	native Array<@sectortype> Sectors;
	native Array<@walltype> Walls;
	
}

struct MapRecord native
{
	enum MIFlags
	{
		FORCEEOG = 1,
		USERMAP = 2,
	}

	enum EMapFlags
	{
		LEVEL_NOINTERMISSION = 1,
		LEVEL_SECRETEXITOVERRIDE = 2,	// when given an explicit level number, override with secret exit in the map, mainly for compiling episodes out of single levels.
		LEVEL_CLEARINVENTORY = 4,
		LEVEL_CLEARWEAPONS = 8,
		LEVEL_FORCENOEOG = 16,			// RR E1L7 needs this to override its boss's death ending the game.
	};

	enum EMapGameFlags
	{
		LEVEL_RR_HULKSPAWN = 1,
		LEVEL_RR_CLEARMOONSHINE = 2,

		LEVEL_EX_COUNTDOWN = 4,
		LEVEL_EX_TRAINING = 8,
		LEVEL_EX_ALTSOUND = 16,
		LEVEL_EX_MULTI = 32,

		LEVEL_SW_SPAWNMINES = 64,
		LEVEL_SW_BOSSMETER_SERPENT = 128,
		LEVEL_SW_BOSSMETER_SUMO = 256,
		LEVEL_SW_BOSSMETER_ZILLA = 512,
		LEVEL_SW_DEATHEXIT_SERPENT = 1024,
		LEVEL_SW_DEATHEXIT_SUMO = 2048,
		LEVEL_SW_DEATHEXIT_ZILLA = 4096,
		LEVEL_SW_DEATHEXIT_SERPENT_NEXT = 8192,

		LEVEL_WT_BOSSSPAWN = 16384,

		LEVEL_BOSSONLYCUTSCENE = 32768,
	};


	native readonly int parTime;
	native readonly int designerTime;
	native readonly String fileName;
	native readonly String labelName;
	native readonly String name;
	native readonly String music;
	native readonly int cdSongId;
	native readonly int flags;
	native readonly int gameflags;
	native readonly int levelNumber;
	native readonly int cluster;
	native readonly String InterBackground;

	native readonly String nextMap;
	native readonly String nextSecret;

	//native readonly String messages[MAX_MESSAGES];
	native readonly String author;

	String GetLabelName()
	{
		if (flags & USERMAP) return StringTable.Localize("$MNU_USERMAP");
		return labelName;
	}
	String DisplayName()
	{
		if (name == "") return labelName;
		return StringTable.Localize(name);
	}

	native ClusterDef GetCluster();
}

struct ClusterDef
{
	native readonly String name;
	native readonly String InterBackground;
}

struct SummaryInfo native
{
	native readonly int kills; 
	native readonly int maxkills;
	native readonly int secrets; 
	native readonly int maxsecrets;
	native readonly int supersecrets;
	native readonly int time;
	native readonly int totaltime;
	native readonly int playercount;
	native readonly bool cheated;
	native readonly bool endofgame;
}

// this only allows function getters to enable validation on the target.
struct CollisionData
{
	int type;
	int exbits;
	voidptr hit;	// do not access!
	native walltype hitWall();
	native sectortype hitSector();
	native CoreActor hitActor();
	native void setSector(sectortype s);
	native void setWall(walltype w);
	native void setActor(CoreActor a);
	native void setVoid();
}

struct HitInfo
{
	Vector3 hitpos;
	sectortype hitSector;
	walltype hitWall;
	CoreActor hitActor;
}

struct Raze
{
	const kAngleMask	= 0x7FF;
	const BAngToDegree = 360. / 2048.;

	native static Color shadeToLight(int shade);
	native static String PlayerName(int i);
	static int bsin(double angle) { return int(sin(angle * (360. / 2048)) * 16384); }
	static double bobval(double angle) { return sin(angle * (360. / 2048)); }
	native static TextureID PickTexture(TextureID texid);
	native static int GetBuildTime();
	native static void forceSyncInput(int playeridx);
	native static Font PickBigFont(String cmptext = "");
	native static Font PickSmallFont(String cmptext = "");
	native static int SoundEnabled();
	native static void SetReverb(int r);
	native static void SetReverbDelay(int d);
	native static Sound FindSoundByResID(int id);

	native static int tileflags(TextureID tex);
	native static int tilesurface(TextureID tex);
	
	native static sectortype updatesector(Vector2 pos, sectortype lastsect, double maxdist = 96);
	native static sectortype, Vector3 clipmove(Vector3 pos, sectortype sect, Vector2 move, double walldist, double ceildist, double flordist, uint cliptype, CollisionData coll, int clipmoveboxtracenum = 3);
	native static bool cansee(Vector3 start, sectortype startsec, Vector3 end, sectortype endsec);
	native static int hitscan(Vector3 start, sectortype startsect, Vector3 vect, HitInfo hitinfo, uint cliptype, double maxrange = -1);

	// Dont know yet how to best export this, so for now these are just placeholders as MP is not operational anyway.
	static int playerPalette(int i)
	{
		return 0;
	}

	static int playerFrags(int i, int j)
	{
		return 0;
	}

	static int playerFraggedSelf(int i)
	{
		return 0;
	}

	static void DrawScoreboard(int top)
	{
		// todo: reimplement this in a game independent fashion based on GZDoom's code.
		// Right now, with no MP support there is no need, though.
	}
	
	static double, double setFreeAimVelocity(double vel, double zvel, double pitch, double zvspeed)
	{
		return vel * cos(pitch), sin(pitch) * zvspeed;
	}
	
}

/*
struct TileFiles
{
	native static TextureID GetTexture(int tile, bool animate = false);
}
*/

class RazeMenuDelegate : MenuDelegateBase
{
	// Todo: Fix this so that it can be done outside the games' sound modules.
	native override void PlaySound(name sname);
	// This is native for security reasons. Having a script call to open the console could be subject to abuse.
	native override void MenuDismissed();

}
