// contains all global Blood definitions
struct Blood native
{
	enum DIFFICULTY
	{
		DIFFICULTY_0 = 0,
		DIFFICULTY_1,
		DIFFICULTY_2,
		DIFFICULTY_3,
		DIFFICULTY_4,
	};

	enum MONSTERSETTINGS
	{
		MONSTERSETTINGS_0 = 0,
		MONSTERSETTINGS_1,
		MONSTERSETTINGS_2,
	};

	enum WEAPONSETTINGS
	{
		WEAPONSETTINGS_0 = 0,
		WEAPONSETTINGS_1,
		WEAPONSETTINGS_2,
		WEAPONSETTINGS_3,
	};

	enum ITEMSETTINGS
	{
		ITEMSETTINGS_0 = 0,
		ITEMSETTINGS_1,
		ITEMSETTINGS_2,
	};
	
	enum GAME_TYPE
	{
		kSingleplayer = 1,
		kDeathmatch = 2,
		kTeamplay = 3
	}
	
	
	// POWERUPS /////////////////////////////////////////////////////
	enum EPowerupType {
		kPwUpFeatherFall = 12,
		kPwUpShadowCloak = 13,
		kPwUpDeathMask = 14,
		kPwUpJumpBoots = 15,
		kPwUpTwoGuns = 17,
		kPwUpDivingSuit = 18,
		kPwUpGasMask = 19,
		kPwUpCrystalBall = 21,
		kPwUpDoppleganger = 23,
		kPwUpReflectShots = 24,
		kPwUpBeastVision = 25,
		kPwUpShadowCloakUseless = 26,
		kPwUpDeliriumShroom = 28,
		kPwUpGrowShroom = 29,
		kPwUpShrinkShroom = 30,
		kPwUpDeathMaskUseless = 31,
		kPwUpAsbestArmor = 39,
		kMaxPowerUps = 51,
	};

	enum EWeapon
	{
		kWeapNone = 0,
		kWeapPitchFork = 1,
		kWeapFlareGun = 2,
		kWeapShotgun = 3,
		kWeapTommyGun = 4,
		kWeapNapalm = 5,
		kWeapDynamite = 6,
		kWeapSpraycan = 7,
		kWeapTeslaCannon = 8,
		kWeapLifeLeech = 9,
		kWeapVoodooDoll = 10,
		kWeapProximity = 11,
		kWeapRemote = 12,
		kWeapBeast = 13,
		kWeapMax = 14,
	};


	native static void PlayIntroMusic();
	native static bool OriginalLoadScreen(); // doing it generically would necessitate exporting the tile manage which we do not want.
	native static void sndStartSample(int resid, int volume, int channel, bool loop = false, int chanflags = 0);
	native static void sndStartSampleNamed(String sname, int volume, int channel);
	native static TextureID PowerupIcon(int pwup);
	native static BloodPlayer GetViewPlayer();

	native static double Random2F(int max, int scale = 16);
	native static int Chance(int ch);

	// These are just dummies to make the MP statusbar code compile.

	static void GetPlayers(Array<BloodPlayer> players)
	{
		players.Clear();
		players.Push(GetViewPlayer());
	}
	static int getGameType()
	{
		return 0;
	}
}

struct GAMEOPTIONS native
{
	native readonly uint8 nGameType;
	native readonly uint8 nDifficulty;
	native readonly uint8 nMonsterSettings;
	native readonly int uGameFlags;
	native readonly int uNetGameFlags;
	native readonly uint8 nWeaponSettings;
	native readonly uint8 nItemSettings;
	native readonly uint8 nRespawnSettings;
	native readonly uint8 nTeamSettings;
	native readonly int nMonsterRespawnTime;
	native readonly int nWeaponRespawnTime;
	native readonly int nItemRespawnTime;
	native readonly int nSpecialRespawnTime;
	native readonly int weaponsV10x;
	native readonly bool bFriendlyFire;
	native readonly bool bKeepKeysOnRespawn;
}

extend struct _
{
	native readonly @GAMEOPTIONS gGameOptions;
	native HitInfo gHitInfo;	// having this global sucks but there's too many side effects depending on it...
}

struct PACKINFO // not native!
{
	bool isActive;
	int curAmount;
}

class BloodPlayer : CorePlayer native
{
	native int GetHealth(); // health is stored in the XSPRITE which cannot be safely exported to scripting at the moment due to pending refactoring.
	native int powerupCheck(int pwup);

    native readonly BloodActor actor;

	//DUDEINFO*       pDudeInfo;
	//PlayerHorizon   horizon;
	//PlayerAngle     angle;
	native uint8      newWeapon;
	native int        weaponQav;
	native int        qavCallback;
	native bool       isRunning;
	native int        posture;   // stand, crouch, swim
	native int        sceneQav;  // by NoOne: used to keep qav id
	native double     bobPhase;
	native int        bobAmp;
	native double     bobHeight;
	native double     bobWidth;
	native int        swayAmp;
	native double     swayHeight;
	native double     swayWidth;
	//native int        nSprite;
	native int        lifeMode;
	native double     zView;
	native double     zViewVel;
	native double     zWeapon;
	native double     zWeaponVel;
	native double     slope;
	native bool       isUnderwater;
	native bool       hasKey[8];
	native int8       hasFlag;
	native int        damageControl[7];
	native int8       curWeapon;
	native int8       nextWeapon;
	native int        weaponTimer;
	native int        weaponState;
	native int        weaponAmmo;  //rename
	native bool       hasWeapon[14];
	native int        weaponMode[14];
	native int        weaponOrder[2][14];
	native int        ammoCount[12];
	native bool       qavLoop;
	native int        fuseTime;
	native int        throwTime;
	native double        throwPower;
	//native Aim        aim;  // world
	//native int        aimTarget;  // aim target sprite
	native int        aimTargetsCount;
	//native short      aimTargets[16];
	native int        deathTime;
	native int        pwUpTime[51];	// kMaxPowerUps
	native int        fragCount;
	native int        fragInfo[8];
	native int        teamId;
	native int        underwaterTime;
	native int        bubbleTime;
	native int        restTime;
	native int        kickPower;
	native int        laughCount;
	native bool       godMode;
	native bool       fallScream;
	native bool       cantJump;
	native int        packItemTime;  // pack timer
	native int        packItemId;    // pack id 1: diving suit, 2: crystal ball, 3: beast vision 4: jump boots
	native PACKINFO   packSlots[5];  // at325 [1]: diving suit, [2]: crystal ball, [3]: beast vision [4]: jump boots
	native int        armor[3];      // armor
	//native int        voodooTarget;
	native int        flickerEffect;
	native int        tiltEffect;
	native int        visibility;
	native int        painEffect;
	native int        blindEffect;
	native int        chokeEffect;
	native int        handTime;
	native bool       hand;  // if true, there is hand start choking the player
	native int        pickupEffect;
	native bool       flashEffect;  // if true, reduce pPlayer->visibility counter
	native int        quakeEffect;
	native int        player_par;
	native int        nWaterPal;
	//POSTURE             pPosture[kModeMax][kPostureMax];
}
