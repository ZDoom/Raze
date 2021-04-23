
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
	GAMEFLAG_WORLDTOUR = 0x00008000,
	GAMEFLAG_DUKEDC = 0x00010000,
	GAMEFLAGMASK        = 0x0000FFFF, // flags allowed from grpinfo

	// We still need these for the parsers.
	GAMEFLAG_FURY = 0,
	GAMEFLAG_DEER = 0,

};

struct UserConfigStruct native
{
	native bool nomonsters;
	native bool nosound;
	native bool nologo;
}

extend struct _
{
	native @UserConfigStruct userConfig;
}

struct Raze
{
	static int calcSinTableValue(int ang)
	{
		return int(16384 * sin((360./2048) * ang));
	}
	
	native static Color shadeToLight(int shade);
	native static void StopAllSounds();
	native static bool SoundEnabled();
	native static bool MusicEnabled();
	
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

// dummy definitions for the status bar. We need them to create the class descriptors

class BaseStatusBar : StatusBarCore native 
{}


class BloodStatusBar : BaseStatusBar native
{}

class DukeCommonStatusBar : BaseStatusBar native
{}

class DukeStatusBar : DukeCommonStatusBar native
{}

class RedneckStatusBar : DukeCommonStatusBar native
{}

class ExhumedStatusBar : BaseStatusBar native
{}

class SWStatusBar : BaseStatusBar native
{}
