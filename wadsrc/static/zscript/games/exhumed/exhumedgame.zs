

struct Exhumed native
{
	native static void PlayLocalSound(int snd, int pitch, bool b, int chanf);
	native static void StopLocalSound();
	native static bool LocalSoundPlaying();
	native static void playCDTrack(int track, bool looped);
	native static void DrawPlasma();
	native static int, int GetStatusSequence(int seq, int index);
	native static int MoveStatusSequence(int s1, int s2);
	native static int SizeOfStatusSequence(int s1);
	native static ExhumedPlayer GetViewPlayer();
	native static int GetPistolClip();
	native static int GetPlayerClip();
	
	static void DrawAbs(String img, int x, int y, int shade = 0)
	{
	    Screen.DrawTexture(TexMan.CheckForTexture(img, TexMan.Type_Any), false, x, y, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true, DTA_Color, Raze.shadeToLight(shade));
	}
	
	static void DrawRel(String img, int x, int y, int shade = 0)
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

struct ExhumedPlayer native
{
	native int16 nHealth;
	native int16 nLives;
	native int16 nDouble;
	native int16 nInvisible;
	native int16 nTorch;
	native int16 field_2;
	native int16 nAction;
	//native int16 nSprite;
	native int16 bIsMummified;
	native int16 invincibility;
	native int16 nAir;
	native int16 nSeq;
	native int16 nMaskAmount;
	native uint16 keys;
	native int16 nMagic;
	native int16 nItem;
	native uint8 items[8];
	native int16 nAmmo[7]; // TODO - kMaxWeapons?

	native int16 nCurrentWeapon;
	native int16 field_3FOUR;
	native int16 bIsFiring;
	native int16 field_38;
	native int16 field_3A;
	native int16 field_3C;
	native int16 nRun;
	native bool bPlayerPan, bLockPan;
	//fixedhoriz nDestVertPan;

	//PlayerHorizon horizon;
	//PlayerAngle angle;
	
	native bool IsUnderwater();
	native int GetAngle();
}

enum EEWeap
{
    kWeaponSword = 0,
    kWeaponPistol,
    kWeaponM60,
    kWeaponFlamer,
    kWeaponGrenade,
    kWeaponStaff,
    kWeaponRing,
    kWeaponMummified
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