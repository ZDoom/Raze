
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
	Hud_full,
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
}

struct MapRecord native
{
	enum MIFlags
	{
		FORCEEOG = 1,
		USERMAP = 2,
	}
	
	native readonly int parTime;
	native readonly int designerTime;
	native readonly String fileName;
	native readonly String labelName;
	native readonly String name;
	native readonly String music;
	native readonly int cdSongId;
	native readonly int flags;
	native readonly int levelNumber;
	native readonly int cluster;
	native readonly String InterBackground;

	native readonly String nextMap;
	native readonly String nextSecret;

	//native readonly String messages[MAX_MESSAGES];
	native readonly String author;
	
	String GetLabelName()
	{
		if (flags & USERMAP) return StringTable.Localize("$TXT_USERMAP");
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
	native readonly int playercount;
	native readonly bool cheated;
	native readonly bool endofgame;
}

struct Raze
{
	const kAngleMask	= 0x7FF;
	const BAngToDegree = 360. / 2048.;
	
	native static Color shadeToLight(int shade);
	native static String PlayerName(int i);
	native static int bsin(int angle, int shift = 0);
	native static int bcos(int angle, int shift = 0);
	native static TextureID PickTexture(TextureID texid);
	native static int GetBuildTime();
	native static Font PickBigFont(String cmptext = "");
	native static Font PickSmallFont(String cmptext = "");

	// game check shortcuts
	static bool isNam()
	{
		return gameinfo.gametype & (GAMEFLAG_NAM | GAMEFLAG_NAPALM);
	}

	static bool isNamWW2GI()
	{
		return gameinfo.gametype & (GAMEFLAG_NAM | GAMEFLAG_NAPALM |GAMEFLAG_WW2GI);
	}

	static bool isWW2GI()
	{
		return gameinfo.gametype & (GAMEFLAG_WW2GI);
	}

	static bool isRR()
	{
		return gameinfo.gametype & (GAMEFLAG_RRALL);
	}

	static bool isRRRA()
	{
		return gameinfo.gametype & (GAMEFLAG_RRRA);
	}

	static bool isWorldTour()
	{
		return gameinfo.gametype & GAMEFLAG_WORLDTOUR;
	}

	static bool isPlutoPak()
	{
		return gameinfo.gametype & GAMEFLAG_PLUTOPAK;
	}

	static bool isShareware()
	{
		return gameinfo.gametype & GAMEFLAG_SHAREWARE;
	}

	static bool isBlood()
	{
		return gameinfo.gametype & GAMEFLAG_BLOOD;
	}
	
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
