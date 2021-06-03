
struct Witchaven native
{
	enum EConst
	{
		MAXWEAPONS = 10,
		MAXPOTIONS = 5,
		MAXFRAMES = 12,


		BACKBUTTON = 9216,
		BACKSCALE = 16384,
		MOUSECURSOR = 9217,
		RESERVED1 = 9218,
		RESERVED2 = 9219,
		
		WHLOGO = 9220,
		MENUFONT = 9221, //+32
		
		MINITEXTPOINT = 9254,
		MINITEXTSLASH = 9255,
		
		WH2FONTBIGLETTERS = 9256,
		WH2FONTLILLETTERS = 9282,
		WH2FONTNUMBERS = 9308,
		WH2LOGO = 9335,
		WH2FONTBACKGROUND = 9336,
		
		ARROWCOUNTLIMIT = 100,
		THROWPIKELIMIT = 100,
		
		GRAVITYCONSTANT = (4 << 7) - 32,
		JUMPVEL = (4 << 10) - 256,
		
		WH2GRAVITYCONSTANT = (8 << 5),
		WH2JUMPVEL = (6 << 10), //(8 << 10)
		GROUNDBIT = 1,
		PLATFORMBIT = 2,

		TICSPERFRAME = 3,
		TIMERRATE = 120,

		PLAYERHEIGHT = 48,
		WH2PLAYERHEIGHT = 64,
		MAXNUMORBS = 8,
		MAXHEALTH = 100,
		MAXKEYS = 4,
		MAXTREASURES = 18,

		LOW = 1,
		HIGH = 2,

		TOP = 1,
		BOTTOM = 2,

		INACTIVE = 0,
		PATROL = 1,
		CHASE = 2,
		AMBUSH = 3,
		BIRTH = 4,
		DODGE = 5,
		ATTACK = 6,
		DEATH = 7,
		STAND = 8,

		MISSILE = 100,
		FX = 101,
		HEATSEEKER = 102,
		YELL = 103,
		CAST = 104,
		PUSH = 105,
		FALL = 106,
		DIE = 107,
		DEAD = 108,
		FACE = 109,
		SHOVE = 110,
		SHATTER = 111,
		FIRE = 112,
		LIFTUP = 113,
		LIFTDN = 114,
		PENDULUM = 115,
		RESURECT = 116,
		BOB = 117,
		SHOVER = 118,
		TORCHER = 119,
		MASPLASH = 120,
		CHUNKOMEAT = 121,
		FLEE = 122,
		DORMANT = 123,
		ACTIVE = 124,
		ATTACK2 = 125,
		WITCHSIT = 126,
		CHILL = 127,
		SKIRMISH = 128,
		FLOCK = 129,
		FLOCKSPAWN = 130,
		PAIN = 131,
		WAR = 132,
		TORCHLIGHT = 133,
		GLOWLIGHT = 134,
		BLOOD = 135,
		DRIP = 136,
		DEVILFIRE = 137,
		FROZEN = 138,
		PULLTHECHAIN = 139,
		FLOCKCHIRP = 140,
		CHUNKOWALL = 141,
		FINDME = 142,
		DRAIN = 143,
		RATRACE = 144,
		SMOKE = 145,
		EXPLO = 146,
		JAVLIN = 147,
		ANIMLEVERUP = 148,
		ANIMLEVERDN = 149,
		BROKENVASE = 150,
		NUKED = 151,
		WARPFX = 152,
		PATROLFLAG = 153,
		PLACECONE = 154,
		REMOVECONE = 155,
		FIRSTCONE = 156,
		GOTOCONE = 157,
		TORCHFRONT = 158,
		APATROLPOINT = 159,
		SHADE = 160,
		EVILSPIRIT = 161,
		STONETOFLESH = 162,
		SPARKS = 163,
		SPARKSUP = 164,
		SPARKSDN = 165,
		LAND = 166,
		SHARDOFGLASS = 167,
		FIRECHUNK = 168,
		DUDE = 170,
		
		  ACTIVATESECTOR  =    1,
		  ACTIVATESECTORONCE = 2,

		  DOORUPTAG   =   6,
		  DOORDOWNTAG  =  7,
		  DOORSPLITHOR =  8,
		  DOORSPLITVER =  9,
		  DOORSWINGTAG =  13,
		  DOORBOX     =   16,

		  PLATFORMELEVTAG = 1000,
		  BOXELEVTAG    = 1003,

		  SECTOREFFECT  = 104,
		  PULSELIGHT    = 0,
		  FLICKERLIGHT  = 1,
		  DELAYEFFECT  =  2,
		  XPANNING     =  3,

		  DOORDELAY    =  480, // 4 second delay for doors to close
		  DOORSPEED    =  128,
		  ELEVSPEED   =   256,

		  PICKDISTANCE =  512, // for picking up sprites
		  PICKHEIGHT  =   40,
		  PICKHEIGHT2  =  64,

		  JETPACKPIC   =  93 ,  // sprites available to pick up

		  MAXSWINGDOORS = 32,
		
		  JETPACKITEM =   0,
		  SHOTGUNITEM =   1,

		  SHOTGUNPIC   =  101,
		  SHOTGUNVIEW  =  102,

		  KILLSECTOR = 4444,
		
		TDIAMONDRING = 0, //armortype3 200units
		TSHADOWAMULET = 1, //shadow
		TGLASSSKULL = 2, //add score
		TAHNK = 3, //health 250units
		TBLUESCEPTER = 4, //lava walk
		TYELLOWSCEPTER = 5, //water walk
		TADAMANTINERING = 6, //unfinity attack protection
		TONYXRING = 7, //projectiles protection
		TPENTAGRAM = 8, //exit
		TCRYSTALSTAFF = 9, //health 250units armortype2 200units
		TAMULETOFTHEMIST = 10, //invisible
		THORNEDSKULL = 11, //end game
		TTHEHORN = 12, //vampire
		TSAPHIRERING = 13, //set armortype to 3 
		TBRASSKEY = 14,
		TBLACKKEY = 15,
		TGLASSKEY = 16,
		TIVORYKEY = 17,

		NORMALCLIP = 0,
		PROJECTILECLIP = 1,
		CLIFFCLIP = 2,

		WALLX = 1 << 0,
		WALLY = 1 << 1,
		FLOORZ = 1 << 2,
		CEILZ = 1 << 3,

		MAXANIMATES = 512,


	}

	native static void PlaySound(int s);
	native static WhPlayer GetViewPlayer();
}

struct WhWeaponInf native {
	native int daweapontics;
	native int daweaponframe;
	native int currx;
	native int curry;
};

struct WhPlayer native {
	
	native WhWeaponInf GetSpellbookAnim();
	
	
	native int spellnum;
	native int x;
	native int y;
	native int z;
	//PlayerAngle angle;
     //PlayerHorizon horizon;
    native int height;
    native int hvel;               
    native int16 sector;
	native int16 oldsector;
	native int16 spritenum;
	native bool keytoggle;
	native int flags;
	native int weapon[Witchaven.MAXWEAPONS], preenchantedweapon[Witchaven.MAXWEAPONS];
	native int ammo[Witchaven.MAXWEAPONS], preenchantedammo[Witchaven.MAXWEAPONS];
	native int orbammo[Witchaven.MAXNUMORBS];
	native int treasure[Witchaven.MAXTREASURES];
	native int orbactive[Witchaven.MAXNUMORBS];
	native int orb[Witchaven.MAXNUMORBS];
	native int potion[Witchaven.MAXPOTIONS];
    native int lvl;
    native int score;
    native int health;
    native int maxhealth;
    native int armor;
    native int armortype;
    native int onsomething;
    native int fallz;
    native bool dead;
	native int shadowtime;
	native int helmettime;
	native int scoretime;
	native int vampiretime;
    native int selectedgun;
    native int currweapon;
	native int currweapontics;
	native int currweaponanim;
	native int currweaponframe;
	native int currweaponfired;
	native int currweaponattackstyle;
	native int currweaponflip;
	native int hasshot;
	native int currentpotion;
	native int strongtime;
	native int manatime;
	native int invisibletime;
	native int orbshot;
	native int spellbooktics;
	native int spellbook;
	native int spellbookframe;
	native int spellbookflip;
	native int nightglowtime;
	native int showbook;
	native int showbooktype;
	native int showbookflip;
	native int showbookanim;
	native int currentorb;
	native int spelltime;
	native int shieldpoints;
	native int shieldtype;
	native int poisoned;
	native int poisontime;
	native int shockme;
	native int invincibletime;
	native int spiked;
	native int spiketics;
	native int spikeframe;
	native int currspikeframe;
	native bool godMode;
	native bool noclip;
	native bool over_shoulder_on;
	native int justwarpedfx;
	native int justwarpedcnt;
	
}


struct Wh1Snd native
{
	const S_THUNDER1 = 0;
	const S_THUNDER2 = 1;
	const S_THUNDER3 = 2;
	const S_THUNDER4 = 3;
	const S_WINDLOOP1 = 4;
	const S_WAVELOOP1 = 5;
	const S_LAVALOOP1 = 6;
	const S_FIRELOOP1 = 7;
	const S_STONELOOP1 = 8;
	const S_BATSLOOP = 9;
	const S_PLRWEAPON0 = 10;
	const S_PLRWEAPON1 = 11;
	const S_PLRWEAPON2 = 12;
	const S_PLRWEAPON3 = 13;
	const S_PLRWEAPON4 = 14;
	const S_GOBLIN1 = 15;
	const S_GOBLIN2 = 16;
	const S_GOBLIN3 = 17;
	const S_GOBPAIN1 = 18;
	const S_GOBPAIN2 = 19;
	const S_GOBDIE1 = 20;
	const S_GOBDIE2 = 21;
	const S_GOBDIE3 = 22;
	const S_KSNARL1 = 23;
	const S_KSNARL2 = 24;
	const S_KSNARL3 = 25;
	const S_KSNARL4 = 26;
	const S_KPAIN1 = 27;
	const S_KPAIN2 = 28;
	const S_KDIE1 = 29;
	const S_KDIE2 = 30;
	const S_DEMON1 = 31;
	const S_DEMON2 = 32;
	const S_DEMON3 = 33;
	const S_DEMON4 = 34;
	const S_DEMON5 = 35;
	const S_DEMONDIE1 = 36;
	const S_DEMONDIE2 = 37;
	const S_MSNARL1 = 38;
	const S_MSNARL2 = 39;
	const S_MSNARL3 = 40;
	const S_MSNARL4 = 41;
	const S_MPAIN1 = 42;
	const S_MDEATH1 = 43;
	const S_DRAGON1 = 44;
	const S_DRAGON2 = 45;
	const S_DRAGON3 = 46;
	const S_RIP1 = 47;
	const S_RIP2 = 48;
	const S_RIP3 = 49;
	const S_SKELHIT1 = 50;
	const S_SKELHIT2 = 51;
	const S_SKELETONDIE = 52;
	const S_GUARDIAN1 = 53;
	const S_GUARDIAN2 = 54;
	const S_GUARDIANPAIN1 = 55;
	const S_GUARDIANPAIN2 = 56;
	const S_GUARDIANDIE = 57;
	const S_WISP = 58;
	const S_WISP2 = 59;
	const S_SPLASH1 = 60;
	const S_SPLASH2 = 61;
	const S_SPLASH3 = 62;
	const S_SPLASH4 = 63;
	const S_SPLASH5 = 64;
	const S_SPLASH6 = 65;
	const S_WILLOWDIE = 66;
	const S_FATWITCHDIE = 67;
	const S_JUDY1 = 68;
	const S_JUDY2 = 69;
	const S_JUDY3 = 70;
	const S_JUDY4 = 71;
	const S_JUDYDIE = 72;
	const S_SKULLWITCH1 = 73;
	const S_SKULLWITCH2 = 74;
	const S_SKULLWITCH3 = 75;
	const S_SKULLWITCHDIE = 76;
	const S_GRONDEATHA = 77;
	const S_GRONDEATHB = 78;
	const S_GRONDEATHC = 79;
	const S_CHAIN1 = 80;
	const S_FLAME1 = 81;
	const S_GRONPAINA = 82;
	const S_GRONPAINB = 83;
	const S_GRONPAINC = 84;
	const S_CLUNK = 85;
	const S_DEMONTHROW = 86;
	const S_WITCHTHROW = 87;
	const S_DOOR1 = 88;
	const S_DOOR2 = 89;
	const S_DOOR3 = 90;
	const S_CREAKDOOR1 = 91;
	const S_CREAKDOOR2 = 92;
	const S_CREAKDOOR3 = 93;
	const S_STING1 = 94;
	const S_STING2 = 95;
	const S_POTION1 = 96;
	const S_GENTHROW = 97;
	const S_GENSWING = 98;
	const S_ARROWHIT = 99;
	const S_WALLHIT1 = 100;
	const S_GONG = 101;
	const S_SPELL1 = 102;
	const S_FREEZE = 103;
	const S_FREEZEDIE = 104;
	const S_TRAP1 = 105;
	const S_TRAP2 = 106;
	const S_RATS1 = 107;
	const S_RATS2 = 108;
	const S_WINDLOOP2 = 109;
	const S_BREATH1 = 110;
	const S_BREATH2 = 111;
	const S_PUSH1 = 112;
	const S_PUSH2 = 113;
	const S_PLRPAIN1 = 114;
	const S_PLRPAIN2 = 115;
	const S_GORE1 = 116;
	const S_GORE2 = 117;
	const S_GORE1A = 118;
	const S_GORE1B = 119;
	const S_DEADSTEP = 120;
	const S_HEARTBEAT = 121;
	const S_SOFTCHAINWALK = 122;
	const S_SOFTCREAKWALK = 123;
	const S_LOUDCHAINWALK = 124;
	const S_GRATEWALK = 125;
	const S_SCARYDUDE = 126;
	const S_WATERY = 127;
	const S_GLASSBREAK1 = 128;
	const S_GLASSBREAK2 = 129;
	const S_GLASSBREAK3 = 130;
	const S_TREASURE1 = 131;
	const S_SWORD1 = 132;
	const S_SWORD2 = 133;
	const S_SWORDCLINK1 = 134;
	const S_SWORDCLINK2 = 135;
	const S_SWORDCLINK3 = 136;
	const S_SWORDCLINK4 = 137;
	const S_SOCK1 = 138;
	const S_SOCK2 = 139;
	const S_SOCK3 = 140;
	const S_SOCK4 = 141;
	const S_KOBOLDHIT = 142;
	const S_SPIDERBITE = 143;
	const S_FIREBALL = 144;
	const S_WOOD1 = 145;
	const S_CHAINDOOR1 = 146;
	const S_PULLCHAIN1 = 147;
	const S_PICKUPAXE = 148;
	const S_EXPLODE = 149;
	const S_SKELSEE = 150;
	const S_BARRELBREAK = 151;
	const S_WARP = 152;
	const S_PICKUPFLAG = 155;
	const S_DROPFLAG = 156;
	const S_LAUGH = 157;
	const S_DRINK = 158;
	const S_PAGE = 159;
	const S_BOTTLES = 160;
	const S_CRACKING = 161;
	const S_PLRDIE1 = 162;
	const S_FATLAUGH = 163;
}

struct WH2Snd native
{
	const S_THUNDER1 = 0;
	const S_THUNDER2 = 1;
	const S_THUNDER3 = 2;
	const S_WINDLOOP1 = 3;
	const S_WAVELOOP1 = 4;
	const S_LAVALOOP1 = 5;
	const S_FIRELOOP1 = 6;
	const S_STONELOOP1 = 7;
	const S_BATSLOOP = 8;
	const S_PLRWEAPON0 = 9;
	const S_PLRWEAPON1 = 10;
	const S_PLRWEAPON2 = 11;
	const S_PLRWEAPON3 = 12;
	const S_PLRWEAPON4 = 13;
	const S_KSNARL1 = 14;
	const S_KSNARL2 = 15;
	const S_KSNARL3 = 16;
	const S_KSNARL4 = 17;
	const S_KPAIN1 = 18;
	const S_KPAIN2 = 19;
	const S_KDIE1 = 20;
	const S_KDIE2 = 21;
	const S_DEMON1 = 22;
	const S_DEMON2 = 23;
	const S_DEMON3 = 24;
	const S_DEMON4 = 25;
	const S_DEMON5 = 26;
	const S_DEMONDIE1 = 27;
	const S_DEMONDIE2 = 28;
	const S_MSNARL1 = 29;
	const S_MSNARL2 = 30;
	const S_MSNARL3 = 31;
	const S_MSNARL4 = 32;
	const S_MPAIN1 = 33;
	const S_MDEATH1 = 34;
	const S_RIP1 = 35;
	const S_RIP2 = 36;
	const S_RIP3 = 37;
	const S_SKELHIT1 = 38;
	const S_SKELHIT2 = 39;
	const S_SKELETONDIE = 40;
	const S_SKELSEE = 41;
	const S_GUARDIAN1 = 42;
	const S_GUARDIAN2 = 43;
	const S_GUARDIANPAIN1 = 44;
	const S_GUARDIANPAIN2 = 45;
	const S_GUARDIANDIE = 46;
	const S_WISP = 47;
	const S_WISP2 = 48;
	const S_SPLASH1 = 49;
	const S_SPLASH2 = 50;
	const S_SPLASH3 = 51;
	const S_SPLASH4 = 52;
	const S_SPLASH5 = 53;
	const S_WILLOWDIE = 54;
	const S_JUDY1 = 55;
	const S_JUDYDIE = 56;
	const S_CHAIN1 = 57;
	const S_FLAME1 = 58;
	const S_CLUNK = 59;
	const S_DEMONTHROW = 60;
	const S_DOOR1 = 61;
	const S_DOOR2 = 62;
	const S_DOOR3 = 63;
	const S_CREAKDOOR1 = 64;
	const S_CREAKDOOR2 = 65;
	const S_CREAKDOOR3 = 66;
	const S_STING1 = 67;
	const S_STING2 = 68;
	const S_POTION1 = 69;
	const S_GENTHROW = 70;
	const S_GENSWING = 71;
	const S_ARROWHIT = 72;
	const S_WALLHIT1 = 73;
	const S_GONG = 74;
	const S_SPELL1 = 75;
	const S_FREEZE = 76;
	const S_FREEZEDIE = 77;
	const S_TRAP1 = 78;
	const S_TRAP2 = 79;
	const S_RATS1 = 80;
	const S_RATS2 = 81;
	const S_WINDLOOP2 = 82;
	const S_BREATH1 = 83;
	const S_BREATH2 = 84;
	const S_PUSH1 = 85;
	const S_PUSH2 = 86;
	const S_PLRPAIN1 = 87;
	const S_PLRPAIN2 = 88;
	const S_GORE1 = 89;
	const S_GORE2 = 90;
	const S_GORE1A = 91;
	const S_GORE1B = 92;
	const S_DEADSTEP = 93;
	const S_HEARTBEAT = 94;
	const S_SOFTCHAINWALK = 95;
	const S_SOFTCREAKWALK = 96;
	const S_LOUDCHAINWALK = 97;
	const S_SCARYDUDE = 98;
	const S_WATERY = 99;
	const S_GLASSBREAK1 = 100;
	const S_GLASSBREAK2 = 101;
	const S_GLASSBREAK3 = 102;
	const S_TREASURE1 = 103;
	const S_SWORD1 = 104;
	const S_SWORD2 = 105;
	const S_SWORDCLINK1 = 106;
	const S_SWORDCLINK2 = 107;
	const S_SWORDCLINK3 = 108;
	const S_SWORDCLINK4 = 109;
	const S_SOCK1 = 110;
	const S_SOCK2 = 111;
	const S_SOCK3 = 112;
	const S_SOCK4 = 113;
	const S_KOBOLDHIT = 114;
	const S_FIREBALL = 115;
	const S_PULLCHAIN1 = 116;
	const S_PICKUPAXE = 117;
	const S_EXPLODE = 118;
	const S_BARRELBREAK = 119;
	const S_WARP = 120;
	const S_PICKUPFLAG = 121;
	const S_DROPFLAG = 122;
	const S_LAUGH = 123;
	const S_DRINK = 124;
	const S_PAGE = 125;
	const S_BOTTLES = 126;
	const S_CRACKING = 127;
	const S_PLRDIE1 = 128;
	const S_GRONDEATHA = 136;
	const S_GRONDEATHB = 137;
	const S_GRONDEATHC = 138;
	const S_GRONPAINA = 142;

	const S_CHAINDOOR1 = 146;
	const S_FATLAUGH = 163;
}