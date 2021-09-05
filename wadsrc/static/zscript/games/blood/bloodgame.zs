// contains all global Blood definitions
struct Blood native
{
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

	native static void PlayIntroMusic();
	native static bool OriginalLoadScreen(); // doing it generically would necessitate exporting the tile manage which we do not want.
	native static void sndStartSample(int resid, int volume, int channel, bool loop = false, int chanflags = 0);
	native static void sndStartSampleNamed(String sname, int volume, int channel);
	native static TextureID PowerupIcon(int pwup);
	native static BloodPlayer GetViewPlayer();
	
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

struct PACKINFO // not native!
{
	bool isActive;
	int curAmount;
}

struct BloodPlayer native
{
	native int GetHealth(); // health is stored in the XSPRITE which cannot be safely exported to scripting at the moment due to pending refactoring.
	native int powerupCheck(int pwup);
	//spritetype*     pSprite;
	//XSPRITE*        pXSprite;
	//DUDEINFO*       pDudeInfo;
	//PlayerHorizon   horizon;
	//PlayerAngle     angle;
	native uint8      newWeapon;
	native int        weaponQav;
	native int        qavCallback;
	native bool       isRunning;
	native int        posture;   // stand, crouch, swim
	native int        sceneQav;  // by NoOne: used to keep qav id
	native int        bobPhase;
	native int        bobAmp;
	native int        bobHeight;
	native int        bobWidth;
	native int        swayPhase;
	native int        swayAmp;
	native int        swayHeight;
	native int        swayWidth;
	native int        nPlayer;  // Connect id
	//native int        nSprite;
	native int        lifeMode;
	native int        zView;
	native int        zViewVel;
	native int        zWeapon;
	native int        zWeaponVel;
	native int        slope;
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
	native int        throwPower;
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
};
