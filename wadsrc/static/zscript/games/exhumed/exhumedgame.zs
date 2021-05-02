

struct Exhumed native
{
	native static void PlayLocalSound(int snd, int pitch, bool b, int chanf);
	native static void StopLocalSound();
	native static bool LocalSoundPlaying();
	native static void playCDTrack(int track, bool looped);
	native static void DrawPlasma();
	
	
	static void DrawAbs(String img, int x, int y, int shade = 0)
	{
	    Screen.DrawTexture(TexMan.CheckForTexture(img, TexMan.Type_Any), false, x, y, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true, DTA_Color, Raze.shadeToLight(shade));
	}
	
	static void DRawRel(String img, int x, int y, int shade = 0)
	{
		let tex = TexMan.CheckForTexture(img, TexMan.Type_Any);
		if (!tex.IsValid()) return;
		let size = TexMan.GetScaledSize(tex);
		let offs = TexMan.GetScaledOffset(tex);
		// The integer truncation here is important. Old Build versions were bugged here.
		x -= (int(size.x) >> 1) + int(offs.x);
		y -= (int(size.y) >> 1) + int(offs.y);
	    Screen.DrawTexture(tex, false, x, y, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true, DTA_Color, Raze.shadeToLight(shade));
	}
}


struct ExhumedSnd native
{
	enum ESounds
	{
		kSound0 = 0,
		kSound1,
		kSound2,
		kSound3,
		kSound4,
		kSound5,
		kSound6,
		kSound7,
		kSound8,
		kSound9,
		kSoundItemSpecial,
		kSound11,
		kSoundTorchOn,
		kSound13,
		kSound14,
		kSound15,
		kSound16,
		kSound17,
		kSound18,
		kSound19,
		kSound20,
		kSound21,
		kSound22,
		kSound23,
		kSound24,
		kSound25,
		kSound26,
		kSound27,
		kSoundJonLaugh2,
		kSound29,
		kSound30,
		kSound31,
		kSound32,
		kSound33,
		kSound34,
		kSound35,
		kSound36,
		kSound38 = 38,
		kSound39,
		kSound40,
		kSound41,
		kSound42,
		kSound43,
		kSound47 = 47,
		kSound48 = 48,
		kSoundQTail = 50,
		kSound52 = 52,
		kSoundTauntStart = 53,
		kSoundJonFDie = 60,
		kSound61,
		kSound62,
		kSound63,
		kSound64,
		kSound65,
		kSound66,
		kSoundMana1,
		kSoundMana2,
		kSoundAmmoPickup,
		kSound70,
		kSound71,
		kSound72,
		kSoundAlarm,
		kSound74,
		kSound75,
		kSound76,
		kSound77,
		kSound78,
		kSound79,
	}
}