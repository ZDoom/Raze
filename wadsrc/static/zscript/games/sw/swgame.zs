//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment
Copyright (C) 2019-2021 Christoph Oelckers

This file is part of Raze

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

struct SW native
{
	const MAX_INVENTORY = 7;
	const MAX_WEAPONS = 14;

	enum ESWSoundFlag
	{
		v3df_none       = 0,    // Default, take no action, use all defaults
		v3df_follow     = 1,    // 1 = Do coordinate updates on sound
		// Use this only if the sprite won't be deleted soon
		v3df_kill       = 2,    // 1 = Sound is to be deleted
		v3df_doppler    = 4,    // 1 = Don't use doppler pitch variance
		v3df_dontpan        = 8,    // 1 = Don't do panning of sound
		v3df_ambient    = 16,   // 1 = Sound is ambient, use ambient struct info.
		v3df_intermit   = 32,   // 1 = Intermittant sound
		v3df_init       = 64,   // 1 = First pass of sound, don't play it.
		// This is mainly used for intermittent sounds
		v3df_nolookup   = 128,   // don't use ambient table lookup
	}

	enum EWeaponAndDamage
	{
		WPN_FIST,            
		WPN_STAR,        
		WPN_SHOTGUN,     
		WPN_UZI,         
		WPN_MICRO,       
		WPN_GRENADE,     
		WPN_MINE,        
		WPN_RAIL,        
		WPN_HOTHEAD,     
		WPN_HEART,       

		WPN_NAPALM,          
		WPN_RING,            
		WPN_ROCKET,          
		WPN_SWORD,           

		// extra weapons connected to other

		// spell
		DMG_NAPALM,          
		DMG_MIRV_METEOR,     
		DMG_SERP_METEOR,     

		// radius damage
		DMG_ELECTRO_SHARD,   
		DMG_SECTOR_EXP,      
		DMG_BOLT_EXP,        
		DMG_TANK_SHELL_EXP,  
		DMG_FIREBALL_EXP,    
		DMG_NAPALM_EXP,      
		DMG_SKULL_EXP,       
		DMG_BASIC_EXP,       
		DMG_GRENADE_EXP,     
		DMG_MINE_EXP,        
		DMG_MINE_SHRAP,      
		DMG_MICRO_EXP,       
		DMG_NUCLEAR_EXP, 
		DMG_RADIATION_CLOUD, 
		DMG_FLASHBOMB,       

		DMG_FIREBALL_FLAMES, 

		// actor
		DMG_RIPPER_SLASH,    
		DMG_SKEL_SLASH,      
		DMG_COOLG_BASH,      
		DMG_COOLG_FIRE,      
		DMG_GORO_CHOP,       
		DMG_GORO_FIREBALL,   
		DMG_SERP_SLASH,      
		DMG_LAVA_BOULDER,    
		DMG_LAVA_SHARD,      
		DMG_HORNET_STING,    
		DMG_EEL_ELECTRO,     

		// misc
		DMG_SPEAR_TRAP,      
		DMG_VOMIT,           

		// inanimate objects
		DMG_BLADE,           
	}

	enum EInvFlags
	{
		INVF_AUTO_USE = 1,
		INVF_TIMED = 2,
		INVF_COUNT = 4
	}

	native static void PlaySound(int sound, int flags, int channel = CHAN_AUTO, int cflags = 0);
	native static void StopSound();
	native static bool IsSoundPlaying(int channel); //  soundEngine.IsSourcePlayingSomething(SOURCE_None, nullptr, CHAN_VOICE))
	native static void PlaySong(int trackid);
	native static int WeaponMaxAmmo(int weap);
	native static int InventoryFlags(int inv);
	native static SWPlayer GetViewPlayer();
	native static int RealWeapon(int wp);
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	static void DrawString(int x, int y, String text, int shade, int pal, int align = -1)
	{
		if (align != -1) x -= SmallFont.StringWidth(text) * (align == 0 ? 0.5 : 1); 		
		Screen.DrawText(SmallFont, Font.CR_NATIVEPAL, x, y, text, DTA_FullscreenScale, FSMode_Fit320x200,
			DTA_Color, Raze.shadeToLight(shade), DTA_TranslationIndex, Translation.MakeID(Translation_Remap, pal));
	}
}


struct SWPlayer native
{
    // variable that fit in the sprite or user structure
	/*
    union
    {
        struct { int32_t posx, posy, posz; };
        vec3_t pos;
    };
	*/
    
    // interpolation
    //int oposx, oposy, oposz;

    // holds last valid move position
    //int16 lv_sectnum;
    //int lv_x,lv_y,lv_z;

	// can't do the pointers yet.
	/*
    SPRITEp remote_sprite;
    REMOTE_CONTROL remote;
    SECTOR_OBJECTp sop_remote;
    SECTOR_OBJECTp sop;  // will either be sop_remote or sop_control
    SECTORp hi_sectp, lo_sectp;
    SPRITEp hi_sp, lo_sp;
    SPRITEp last_camera_sp;
    PlayerHorizon horizon;
    PlayerAngle angle;
    binangle RevolveAng;
    // under vars are for wading and swimming
    //int16 PlayerSprite, PlayerUnderSprite;
    SPRITEp SpriteP, UnderSpriteP;
    PLAYER_ACTION_FUNCp DoPlayerAction;
    ESyncBits KeyPressBits;

    SECTOR_OBJECTp sop_control; // sector object pointer
    SECTOR_OBJECTp sop_riding; // sector object pointer

    struct
    {
        PANEL_SPRITEp Next, Prev;
    } PanelSpriteList;
    PANEL_SPRITEp CurWpn;
    PANEL_SPRITEp Wpn[SW.MAX_WEAPONS];
    PANEL_SPRITEp Chops;
	*/
    native voidptr sop_remote;	// the status bar needs to check this - remove once the underlying type can be supported.

    native int jump_count, jump_speed;     // jumping
    native int16 down_speed, up_speed; // diving
    native int z_speed,oz_speed; // used for diving and flying instead of down_speed, up_speed
    native int climb_ndx;
    native int hiz,loz;
    native int ceiling_dist,floor_dist;
    native int circle_camera_dist;
    native int six,siy,siz; // save player interp position for PlayerSprite
    native int16 siang;

    native int xvect, yvect;
    native int oxvect, oyvect;
    native int friction;
    native int slide_xvect, slide_yvect;
    native int16 slide_ang;
    native int slide_dec;
    native float drive_avel;

    native int16 view_outside_dang;  // outside view delta ang
    native int16 circle_camera_ang;
    native int16 camera_check_time_delay;

    native int cursectnum,lastcursectnum;
    native int turn180_target; // 180 degree turn

    // variables that do not fit into sprite structure
    native int hvel,tilt,tilt_dest;
    native int16 recoil_amt;
    native int16 recoil_speed;
    native int16 recoil_ndx;
    native int recoil_horizoff;

    native int oldposx,oldposy,oldposz;
    native int RevolveX, RevolveY;
    native int16 RevolveDeltaAng;

    native int16 pnum; // carry along the player number

    native int16 LadderSector;
    native int lx,ly; // ladder x and y
    native int16 JumpDuration;
    native int16 WadeDepth;
    native int16 bob_amt;
    native int16 bob_ndx;
    native int16 bcnt; // bob count
    native int bob_z, obob_z;

    // must start out as 0
    native int playerreadyflag;

    native int Flags, Flags2;

    // Key stuff
    native uint8 HasKey[8];

    // Weapon stuff
    native int16 SwordAng;
    native int WpnGotOnceFlags; // for no respawn mode where weapons are allowed grabbed only once
    native int WpnFlags;
    native int16 WpnAmmo[SW.MAX_WEAPONS];
    native int16 WpnNum;
    native uint8 WpnRocketType; // rocket type
    native uint8 WpnRocketHeat; // 5 to 0 range
    native uint8 WpnRocketNuke; // 1, you have it, or you don't
    native uint8 WpnFlameType; // Guardian weapons fire
    native uint8 WpnFirstType; // First weapon type - Sword/Shuriken
    native uint8 WeaponType; // for weapons with secondary functions
    native int16 FirePause; // for sector objects - limits rapid firing
    //
    // Inventory Vars
    //
    native int16 InventoryNum;
    native int16 InventoryBarTics;
    native int16 InventoryTics[SW.MAX_INVENTORY];
    native int16 InventoryPercent[SW.MAX_INVENTORY];
    native int8 InventoryAmount[SW.MAX_INVENTORY];
    native bool InventoryActive[SW.MAX_INVENTORY];

    native int16 DiveTics;
    native int16 DiveDamageTics;

    // Death stuff
    native uint16 DeathType;
    native int16 Kills;
    //native int16 Killer;  //who killed me
    //native int16 KilledPlayer[MAX_SW_PLAYERS_REG];
    native int16 SecretsFound;

    // Health
    native int16 Armor;
    native int16 MaxHealth;

    //char RocketBarrel;

    native uint8 UziShellLeftAlt;
    native uint8 UziShellRightAlt;
    native uint8 TeamColor;  // used in team play and also used in regular mulit-play for show

    // palette fading up and down for player hit and get items
    native int16 FadeTics;                 // Tics between each fade cycle
    native int16 FadeAmt;                  // Current intensity of fade
    native bool NightVision;               // Is player's night vision active?
    native uint8 StartColor;       // Darkest color in color range being used
    //native int16 electro[64];
    native bool IsAI;                      // Is this and AI character?
    native int16 fta,ftq;                  // First time active and first time quote, for talking in multiplayer games
    native int16 NumFootPrints;            // Number of foot prints left to lay down
    native uint8 WpnUziType;                // Toggle between single or double uzi's if you own 2.
    native uint8 WpnShotgunType;            // Shotgun has normal or fully automatic fire
    native uint8 WpnShotgunAuto;            // 50-0 automatic shotgun rounds
    native uint8 WpnShotgunLastShell;       // Number of last shell fired
    native uint8 WpnRailType;               // Normal Rail Gun or EMP Burst Mode
    native bool Bloody;                    // Is player gooey from the slaughter?
    native bool InitingNuke;
    native bool TestNukeInit;
    native bool NukeInitialized;           // Nuke already has counted down
    native int16 FistAng;                  // KungFu attack angle
    native uint8 WpnKungFuMove;             // KungFu special moves
    //native int16 HitBy;                    // SpriteNum of whatever player was last hit by
    native int16 Reverb;                   // Player's current reverb setting
    native int16 Heads;                    // Number of Accursed Heads orbiting player
    native int PlayerVersion;
    native uint8 WpnReloadState;

	native int WeaponNum();
	native int GetBuildAngle();
	native int Health();
	native int MaxUserHealth();
}




struct SWSnd native
{
	enum ESounds
	{
		DIGI_NULL= 0,
		DIGI_SWORDSWOOSH= 1,
		DIGI_STAR= 2,
		DIGI_STARCLINK= 3,
		DIGI_STARWIZ= 4,
		DIGI_UZIFIRE= 5,
		DIGI_RICHOCHET1= 6,
		DIGI_RICHOCHET2= 7,
		DIGI_REMOVECLIP= 8,
		DIGI_REPLACECLIP= 9,
		DIGI_SHELL= 10,
		DIGI_RIOTFIRE= 11,
		DIGI_RIOTFIRE2= 12,
		DIGI_RIOTRELOAD= 13,
		DIGI_BOLTEXPLODE= 14,
		DIGI_BOLTWIZ= 15,
		DIGI_30MMFIRE= 16,
		DIGI_30MMRELOAD= 17,
		DIGI_30MMEXPLODE= 18,
		DIGI_30MMWIZ= 19,
		DIGI_HEADFIRE= 20,
		DIGI_HEADSHOTWIZ= 21,
		DIGI_HEADSHOTHIT= 22,
		DIGI_MINETHROW= 23,
		DIGI_MINEBOUNCE= 24,
		DIGI_MINEBLOW= 25,
		DIGI_MINEBEEP= 26,
		DIGI_HEARTBEAT= 27,
		DIGI_HEARTFIRE= 28,
		DIGI_HEARTWIZ= 29,
		DIGI_MISSLFIRE= 30,
		DIGI_MISSLEXP= 31,
		DIGI_RFWIZ= 32,
		DIGI_NAPFIRE= 33,
		DIGI_NAPWIZ= 34,
		DIGI_NAPPUFF= 35,
		DIGI_MIRVFIRE= 36,
		DIGI_MIRVWIZ= 37,
		DIGI_SPIRALFIRE= 38,
		DIGI_SPIRALWIZ= 39,
		DIGI_MAGIC1= 40,
		DIGI_MAGIC2= 41,
		DIGI_MAGIC3= 42,
		DIGI_MAGIC4= 43,
		DIGI_MAGIC5= 44,
		DIGI_MAGIC6= 45,
		DIGI_MAGIC7= 46,
		DIGI_SWCLOAKUNCLOAK= 47,
		DIGI_DHVOMIT= 48,
		DIGI_DHCLUNK= 49,
		DIGI_DHSQUISH= 50,
		DIGI_NULL_DHSQUISH= 50,
		DIGI_PROJECTILELAVAHIT=51,
		DIGI_PROJECTILEWATERHIT=52,
		DIGI_KEY= 53,
		DIGI_ITEM= 54,
		DIGI_BIGITEM= 55,
		DIGI_BODYFALL1= 56,
		DIGI_HITGROUND= 57,
		DIGI_BODYSQUISH1= 58,
		DIGI_BODYBURN= 59,
		DIGI_BODYBURNSCREAM= 60,
		DIGI_BODYCRUSHED1= 61,
		DIGI_BODYHACKED1= 62,
		DIGI_BODYSINGED= 63,
		DIGI_DROWN= 64,
		DIGI_SCREAM1= 65,
		DIGI_SCREAM2= 66,
		DIGI_SCREAM3= 67,
		DIGI_HIT1= 68,
		DIGI_ELECTRICUTE1= 69,
		DIGI_REMOVEME= 70,
		DIGI_IMPALED= 71,
		DIGI_OOF1= 72,
		DIGI_ACTORBODYFALL1= 73,
		IGI_ACTORHITGROUND= 74,
		DIGI_COOLIEEXPLODE= 75,
		DIGI_COOLIESCREAM= 76,
		DIGI_COOLIEALERT= 77,
		DIGI_COOLIEAMBIENT= 78,
		DIGI_COOLIEPAIN= 79,
		DIGI_CGMATERIALIZE= 80,
		DIGI_CGALERT= 81,
		DIGI_CGTHIGHBONE= 82,
		DIGI_CGAMBIENT= 83,
		DIGI_CGPAIN= 84,
		DIGI_CGMAGIC= 85,
		DIGI_CGMAGICHIT= 86,
		DIGI_CGSCREAM= 87,
		DIGI_NINJAAMBIENT= 88,
		DIGI_NINJASTAR= 89,
		DIGI_NINJAPAIN= 90,
		DIGI_NINJASCREAM= 91,
		DIGI_NINJAALERT= 92,
		DIGI_NINJAUZIATTACK= 93, 
		DIGI_NINJARIOTATTACK= 94, 
		DIGI_RIPPERAMBIENT= 95, 
		DIGI_RIPPERALERT= 96, 
		DIGI_RIPPERATTACK= 97, 
		DIGI_RIPPERPAIN= 98, 
		DIGI_RIPPERSCREAM= 99, 
		DIGI_RIPPERHEARTOUT= 100, 
		DIGI_GRDAMBIENT= 101, 
		DIGI_GRDALERT= 102, 
		DIGI_GRDPAIN= 103, 
		DIGI_GRDSCREAM= 104, 
		DIGI_GRDFIREBALL= 105, 
		DIGI_GRDSWINGAXE= 106, 
		DIGI_GRDAXEHIT= 107, 
		DIGI_SPAMBIENT= 108, 
		DIGI_SPALERT= 109, 
		DIGI_SPPAIN= 110, 
		DIGI_SPSCREAM= 111, 
		DIGI_SPBLADE= 112, 
		DIGI_SPELEC= 113, 
		DIGI_SPTELEPORT= 114, 
		DIGI_AHAMBIENT= 115, 
		DIGI_AHSCREAM= 116, 
		DIGI_AHEXPLODE= 117, 
		DIGI_AHSWOOSH= 118, 
		DIGI_HORNETBUZZ= 119, 
		DIGI_HORNETSTING= 120, 
		DIGI_HORNETPAIN= 121, 
		DIGI_HORNETDEATH= 122, 
		DIGI_SERPAMBIENT= 123, 
		DIGI_SERPALERT= 124, 
		DIGI_SERPPAIN= 125, 
		DIGI_SERPSCREAM= 126, 
		DIGI_SERPDEATHEXPLODE=127, 
		DIGI_SERPSWORDATTACK= 128, 
		DIGI_SERPMAGICLAUNCH= 129, 
		DIGI_SERPSUMMONHEADS= 130, 
		DIGI_SERPTAUNTYOU= 131, 
		DIGI_LAVABOSSAMBIENT= 132, 
		DIGI_LAVABOSSSWIM= 133, 
		DIGI_LAVABOSSRISE= 134, 
		DIGI_LAVABOSSALERT= 135, 
		DIGI_LAVABOSSFLAME= 136, 
		DIGI_LAVABOSSMETEOR= 137, 
		DIGI_LAVABOSSMETEXP= 138, 
		DIGI_LAVABOSSPAIN= 139, 
		DIGI_LAVABOSSSIZZLE= 140, 
		DIGI_LAVABOSSEXPLODE= 141, 
		DIGI_BOATSTART= 142, 
		DIGI_BOATRUN= 143, 
		DIGI_BOATSTOP= 144, 
		DIGI_BOATFIRE= 145, 
		DIGI_TANKSTART= 146, 
		DIGI_TANKRUN= 147, 
		DIGI_TANKSTOP= 148, 
		DIGI_TANKIDLE= 149, 
		DIGI_TANKFIRE= 150, 
		DIGI_TRUKRUN= 151, 
		DIGI_TRUKIDLE= 152, 
		DIGI_SUBRUN= 153, 
		DIGI_SUBIDLE= 154, 
		DIGI_SUBDOOR= 155, 
		DIGI_BOMBRFLYING= 156, 
		DIGI_BOMBRDROPBOMB= 157, 
		DIGI_BUBBLES= 158, 
		DIGI_CHAIN= 159, 
		DIGI_CHAINDOOR= 160, 
		DIGI_CRICKETS= 161, 
		DIGI_WOODDOOROPEN= 162, 
		DIGI_WOODDOORCLOSE= 163, 
		DIGI_METALDOOROPEN= 164, 
		DIGI_METALDOORCLOSE= 165, 
		DIGI_SLIDEDOOROPEN= 166, 
		DIGI_SLIDEDOORCLOSE= 167, 
		DIGI_STONEDOOROPEN= 168, 
		DIGI_STONEDOORCLOSE= 169, 
		DIGI_SQUEAKYDOOROPEN= 170, 
		DIGI_SQUEAKYDOORCLOSE=171, 
		DIGI_DRILL= 172, 
		DIGI_CAVEDRIP1= 173, 
		DIGI_CAVEDRIP2= 174, 
		DIGI_DRIP= 175, 
		DIGI_WATERFALL1= 176, 
		DIGI_WATERFALL2= 177, 
		DIGI_WATERFLOW1= 178, 
		DIGI_WATERFLOW2= 179, 
		DIGI_ELEVATOR= 180, 
		DIGI_SMALLEXP= 181, 
		DIGI_MEDIUMEXP= 182, 
		DIGI_LARGEEXP= 183, 
		DIGI_HUGEEXP= 184, 
		DIGI_NULL_HUGEEXP= 184, 
		DIGI_FIRE1= 185, 
		DIGI_FIRE2= 186, 
		DIGI_FIREBALL1= 187, 
		DIGI_FIREBALL2= 188, 
		DIGI_GEAR1= 189, 
		DIGI_GONG= 190, 
		DIGI_LAVAFLOW1= 191, 
		DIGI_MACHINE1= 192, 
		DIGI_MUBBUBBLES1= 193, 
		DIGI_EARTHQUAKE= 194, 
		DIGI_SEWERFLOW1= 195, 
		DIGI_SPLASH1= 196, 
		DIGI_STEAM1= 197, 
		DIGI_VOLCANOSTEAM1= 198, 
		DIGI_STOMPER= 199, 
		DIGI_SWAMP= 200, 
		DIGI_REGULARSWITCH= 201, 
		DIGI_BIGSWITCH= 202, 
		DIGI_STONESWITCH= 203, 
		DIGI_GLASSSWITCH= 204, 
		DIGI_HUGESWITCH= 205, 
		DIGI_THUNDER= 206, 
		DIGI_TELEPORT= 207, 
		DIGI_UNDERWATER= 208, 
		DIGI_UNLOCK= 209, 
		DIGI_SQUEAKYVALVE= 210, 
		DIGI_VOID1= 211, 
		DIGI_VOID2= 212, 
		DIGI_VOID3= 213, 
		DIGI_VOID4= 214, 
		DIGI_VOID5= 215, 
		DIGI_ERUPTION= 216, 
		DIGI_VOLCANOPROJECTILE= 217,
		DIGI_LIGHTWIND= 218, 
		DIGI_STRONGWIND= 219, 
		DIGI_BREAKINGWOOD= 220, 
		DIGI_BREAKSTONES= 221, 
		DIGI_ENGROOM1= 222, 
		DIGI_ENGROOM2= 223, 
		DIGI_ENGROOM3= 224, 
		DIGI_ENGROOM4= 225, 
		DIGI_ENGROOM5= 226, 
		DIGI_BREAKGLASS= 227, 
		DIGI_MUSSTING= 228, 
		DIGI_HELI= 229, 
		DIGI_BIGHART= 230, 
		DIGI_WIND4= 231, 
		DIGI_SPOOKY1= 232, 
		DIGI_DRILL1= 233, 
		DIGI_JET= 234, 
		DIGI_DRUMCHANT= 235, 
		DIGI_BUZZZ= 236, 
		DIGI_CHOP_CLICK= 237, 
		DIGI_SWORD_UP= 238, 
		DIGI_UZI_UP= 239, 
		DIGI_SHOTGUN_UP= 240, 
		DIGI_ROCKET_UP= 241, 
		DIGI_GRENADE_UP= 242, 
		DIGI_RAIL_UP= 243, 
		DIGI_MINE_UP= 244, 
		DIGI_TAUNTAI1= 246, 
		DIGI_TAUNTAI2= 247, 
		DIGI_TAUNTAI3= 248, 
		DIGI_TAUNTAI4= 249, 
		DIGI_TAUNTAI5= 250, 
		DIGI_TAUNTAI6= 251, 
		DIGI_TAUNTAI7= 252, 
		DIGI_TAUNTAI8= 253, 
		DIGI_TAUNTAI9= 254, 
		DIGI_TAUNTAI10= 255, 
		DIGI_PLAYERPAIN1= 256, 
		DIGI_PLAYERPAIN2= 257, 
		DIGI_PLAYERPAIN3= 258, 
		DIGI_PLAYERPAIN4= 259, 
		DIGI_PLAYERPAIN5= 260, 
		DIGI_PLAYERYELL1= 261, 
		DIGI_PLAYERYELL2= 262, 
		DIGI_PLAYERYELL3= 263, 
		DIGI_SEARCHWALL= 264, 
		DIGI_NOURINAL= 265, 
		DIGI_FALLSCREAM= 266, 
		DIGI_GOTITEM1= 267, 
		DIGI_LASTPLAYERVOICE= 268, 
		DIGI_RAILFIRE= 269, 
		DIGI_NULL_RAILFIRE= 269, 
		DIGI_RAILREADY= 270, 
		DIGI_RAILPWRUP= 271, 
		DIGI_NUCLEAREXP= 272, 
		DIGI_NUKESTDBY= 273, 
		DIGI_NUKECDOWN= 274, 
		DIGI_NUKEREADY= 275, 
		DIGI_CHEMGAS= 276, 
		DIGI_CHEMBOUNCE= 277, 
		DIGI_THROW= 278, 
		DIGI_PULL= 279, 
		DIGI_MINEARM= 280, 
		DIGI_HEARTDOWN= 281, 
		DIGI_TOOLBOX= 282, 
		DIGI_NULL_TOOLBOX= 282, 
		DIGI_GASPOP= 283, 
		DIGI_40MMBNCE= 284, 
		DIGI_BURGLARALARM= 285, 
		DIGI_CARALARM= 286, 
		DIGI_CARALARMOFF= 287, 
		DIGI_CALTROPS= 288, 
		DIGI_NIGHTON= 289, 
		DIGI_NIGHTOFF= 290, 
		DIGI_SHOTSHELLSPENT= 291, 
		DIGI_BUSSKID= 292, 
		DIGI_BUSCRASH= 293, 
		DIGI_BUSENGINE= 294, 
		DIGI_ARMORHIT= 295, 
		DIGI_ASIREN1= 296, 
		DIGI_FIRETRK1= 297, 
		DIGI_TRAFFIC1= 298, 
		DIGI_TRAFFIC2= 299, 
		DIGI_TRAFFIC3= 300, 
		DIGI_TRAFFIC4= 301, 
		DIGI_TRAFFIC5= 302, 
		DIGI_TRAFFIC6= 303, 
		DIGI_HELI1= 304, 
		DIGI_JET1= 305, 
		DIGI_MOTO1= 306, 
		DIGI_MOTO2= 307, 
		DIGI_NEON1= 308, 
		DIGI_SUBWAY= 309, 
		DIGI_TRAIN1= 310, 
		DIGI_COINS= 311, 
		DIGI_SWORDCLANK= 312, 
		DIGI_RIPPER2AMBIENT= 313, 
		DIGI_RIPPER2ALERT= 314, 
		DIGI_RIPPER2ATTACK= 315, 
		DIGI_RIPPER2PAIN= 316, 
		DIGI_RIPPER2SCREAM= 317, 
		DIGI_RIPPER2HEARTOUT=318, 
		DIGI_M60= 319, 
		DIGI_SUMOSCREAM= 320, 
		DIGI_SUMOALERT= 321, 
		DIGI_SUMOAMBIENT= 322, 
		DIGI_SUMOPAIN= 323, 
		DIGI_RAMUNLOCK= 324, 
		DIGI_CARDUNLOCK= 325, 
		DIGI_ANCIENTSECRET= 326, 
		DIGI_AMERICANDRIVER= 327, 
		DIGI_DRIVELIKEBABOON= 328, 
		DIGI_BURNBABY= 329, 
		DIGI_LIKEBIGWEAPONS= 330, 
		DIGI_COWABUNGA= 331, 
		DIGI_NOCHARADE= 332, 
		DIGI_TIMETODIE= 333, 
		DIGI_EATTHIS= 334, 
		DIGI_FIRECRACKERUPASS=335, 
		DIGI_HOLYCOW= 336, 
		DIGI_HOLYPEICESOFCOW= 337, 
		DIGI_HOLYSHIT= 338, 
		DIGI_HOLYPEICESOFSHIT=339, 
		DIGI_PAYINGATTENTION= 340, 
		DIGI_EVERYBODYDEAD= 341, 
		DIGI_KUNGFU= 342, 
		DIGI_HOWYOULIKEMOVE= 343, 
		DIGI_NOMESSWITHWANG= 344, 
		DIGI_RAWREVENGE= 345, 
		DIGI_YOULOOKSTUPID= 346, 
		DIGI_TINYDICK= 347, 
		DIGI_NOTOURNAMENT= 348, 
		DIGI_WHOWANTSWANG= 349, 
		DIGI_MOVELIKEYAK= 350, 
		DIGI_ALLINREFLEXES= 351, 
		DIGI_EVADEFOREVER= 352, 
		DIGI_MRFLY= 353, 
		DIGI_SHISEISI= 354, 
		DIGI_LIKEFIREWORKS= 355, 
		DIGI_LIKEHIROSHIMA= 356, 
		DIGI_LIKENAGASAKI= 357, 
		DIGI_LIKEPEARL= 358, 
		DIGI_IAMSHADOW= 359, 
		DIGI_ILIKENUKES= 360, 
		DIGI_ILIKESWORD= 361, 
		DIGI_ILIKESHURIKEN= 362, 
		DIGI_BADLUCK= 363, 
		DIGI_NOMOVIEMRCHAN= 364, 
		DIGI_REALLIFEMRCHAN= 365, 
		DIGI_NOLIKEMUSIC= 366, 
		DIGI_NODIFFERENCE= 367, 
		DIGI_NOFEAR= 368, 
		DIGI_NOPAIN= 369, 
		DIGI_NOREPAIRMAN= 370, 
		DIGI_SONOFABITCH= 371, 
		DIGI_PAINFORWEAK= 372, 
		DIGI_GOSPEEDY= 373, 
		DIGI_GETTINGSTIFF= 374, 
		DIGI_TOMBRAIDER= 375, 
		DIGI_STICKYGOTU1= 376, 
		DIGI_STICKYGOTU2= 377, 
		DIGI_STICKYGOTU3= 378, 
		DIGI_STICKYGOTU4= 379, 
		DIGI_SWORDGOTU1= 380, 
		DIGI_SWORDGOTU2= 381, 
		DIGI_SWORDGOTU3= 382, 
		DIGI_HURTBAD1= 383, 
		DIGI_HURTBAD2= 384, 
		DIGI_HURTBAD3= 385, 
		DIGI_HURTBAD4= 386, 
		DIGI_HURTBAD5= 387, 
		DIGI_TOILETGIRLSCREAM= 388, 
		DIGI_TOILETGIRLALERT= 389, 
		DIGI_TOILETGIRLAMBIENT=390, 
		DIGI_TOILETGIRLPAIN= 391, 
		DIGI_TOILETGIRLTAUNT1= 392, 
		DIGI_TOILETGIRLTAUNT2= 393, 
		DIGI_SUMOFART= 394, 
		DIGI_GIBS1= 395, 
		DIGI_GIBS2= 396, 
		DIGI_BIRDS1= 397, 
		DIGI_BIRDS2= 398, 
		DIGI_TOILET= 399, 
		DIGI_FORKLIFTIDLE= 400, 
		DIGI_FORKLIFTRUN= 401, 
		DIGI_TOYCAR= 402, 
		DIGI_UZIMATIC= 403, 
		DIGI_COMPUTERPOWER= 404, 
		DIGI_GENERATORON= 405, 
		DIGI_GENERATORRUN= 406, 
		DIGI_BIGDRILL= 407, 
		DIGI_FLUORLIGHT= 408, 
		DIGI_AMOEBA= 409, 
		DIGI_BODYFALL2= 410, 
		DIGI_GIBS3= 411, 
		DIGI_NINJACHOKE= 412, 
		DIGI_TRAIN3= 413, 
		DIGI_TRAINR02= 414, 
		DIGI_TRAIN8= 415, 
		DIGI_TRASHLID= 416, 
		DIGI_GETMEDKIT= 417, 
		DIGI_AHH= 418, 
		DIGI_PALARM= 419, 
		DIGI_PFLIP= 420, 
		DIGI_PROLL1= 421, 
		DIGI_PROLL2= 422, 
		DIGI_PROLL3= 423, 
		DIGI_BUNNYATTACK= 424, 
		DIGI_BUNNYDIE1= 425, 
		DIGI_BUNNYDIE2= 426, 
		DIGI_BUNNYDIE3= 427, 
		DIGI_BUNNYAMBIENT= 428, 
		DIGI_STONESLIDE= 429, 
		DIGI_NINJAINHALF= 430, 
		DIGI_RIPPER2CHEST= 431, 
		DIGI_WHIPME= 432, 
		DIGI_ENDLEV= 433, 
		DIGI_MDALARM= 434, 
		DIGI_BREAKMETAL= 435, 
		DIGI_BREAKDEBRIS= 436, 
		DIGI_BREAKMARBELS= 437, 
		DIGI_BANZAI= 438, 
		DIGI_HAHA1= 439, 
		DIGI_HAHA2= 440, 
		DIGI_HAHA3= 441, 
		DIGI_ITEM_SPAWN= 442, 
		DIGI_NOREPAIRMAN2= 443, 
		DIGI_NOPOWER= 444, 
		DIGI_DOUBLEUZI= 445, 
		DIGI_NOTORDBUNNY= 446, 
		DIGI_CANBEONLYONE= 447, 
		DIGI_MIRROR1= 448, 
		DIGI_MIRROR2= 449, 
		DIGI_HITTINGWALLS= 450, 
		DIGI_GOTRAILGUN= 451, 
		DIGI_RABBITHUMP1= 452, 
		DIGI_RABBITHUMP2= 453, 
		DIGI_RABBITHUMP3= 454, 
		DIGI_RABBITHUMP4= 455, 
		DIGI_FAGRABBIT1= 456, 
		DIGI_FAGRABBIT2= 457, 
		DIGI_FAGRABBIT3= 458, 
		DIGI_STINKLIKEBABBOON= 459, 
		DIGI_WHATYOUEATBABY= 460, 
		DIGI_WHATDIEDUPTHERE= 461, 
		DIGI_YOUGOPOOPOO= 462, 
		DIGI_PULLMYFINGER= 463, 
		DIGI_SOAPYOUGOOD= 464, 
		DIGI_WASHWANG= 465, 
		DIGI_DROPSOAP= 466, 
		DIGI_REALTITS= 467, 
		DIGI_MSTRLEEP= 468, 
		DIGI_SEEKLEEPADVICE= 469, 
		DIGI_AVENGELEEPDEATH= 470, 
		DIGI_LEEPGHOST= 471, 
		DIGI_DOOR1= 472, 
		DIGI_DOOR2= 473, 
		DIGI_DOOR3= 474, 
		DIGI_FLAGWAVE= 475, 
		DIGI_SURFACE= 476, 
		DIGI_GASHURT= 477, 
		DIGI_BONUS_GRAB= 478, 
		DIGI_ANIMECRY= 479, 
		DIGI_ANIMESING1= 480, 
		DIGI_ANIMEMAD1= 481, 
		DIGI_ANIMESING2= 482, 
		DIGI_ANIMEMAD2= 483, 
		DIGI_PLAYER_TELEPORT= 484, 
		DIGI_INTRO_SLASH= 485, 
		DIGI_WARNING= 486, 
		DIGI_INTRO_WHIRL= 487, 
		DIGI_TOILETGIRLFART1= 488, 
		DIGI_TOILETGIRLFART2= 489, 
		DIGI_TOILETGIRLFART3= 490, 
		DIGI_WINDCHIMES= 491, 
		DIGI_MADATCARPET= 492, 
		DIGI_JUMPONCARPET= 493, 
		DIGI_USEBROKENVEHICLE= 494, 
		DIGI_STEPONCALTROPS= 495, 
		DIGI_WANGSEESERP= 496, 
		DIGI_SERPTAUNTWANG= 497, 
		DIGI_WANGTAUNTSERP1= 498, 
		DIGI_WANGTAUNTSERP2= 499, 
		DIGI_WANGORDER1= 500, 
		DIGI_WANGORDER2= 501, 
		DIGI_WANGDROWNING= 502, 
		DIGI_ZILLAREGARDS= 503, 
		DIGI_PMESSAGE= 504, 
		DIGI_SHAREND_UGLY1= 505, 
		DIGI_SHAREND_UGLY2= 506, 
		DIGI_SHAREND_TELEPORT= 507, 
		DIGI_HOTHEADSWITCH= 508, 
		DIGI_BOATCREAK= 509, 
		DIGI_BOATRUN2= 510, 
		DIGI_BOATIDLE= 511, 
		DIGI_SHIPBELL= 512, 
		DIGI_FOGHORN= 513, 
		DIGI_CANNON= 514, 
		DIGI_JG41001= 515, 
		DIGI_JG41012= 516, 
		DIGI_JG41018= 517, 
		DIGI_JG41028= 518, 
		DIGI_JG41048= 519, 
		DIGI_JG41052= 520, 
		DIGI_JG41058= 521, 
		DIGI_JG41060= 522, 
		DIGI_JG41075= 523, 
		DIGI_JG42004= 524, 
		DIGI_JG42019= 525, 
		DIGI_JG42021= 526, 
		DIGI_JG42028= 527, 
		DIGI_JG42033= 528, 
		DIGI_JG42034= 529, 
		DIGI_JG42050= 530, 
		DIGI_JG42056= 531, 
		DIGI_JG42061= 532, 
		DIGI_JG43004= 533, 
		DIGI_JG43015= 534, 
		DIGI_JG43019= 535, 
		DIGI_JG43021= 536, 
		DIGI_JG44011= 537, 
		DIGI_JG44014= 538, 
		DIGI_JG44027= 539, 
		DIGI_JG44038= 540, 
		DIGI_JG44039= 541, 
		DIGI_JG44048= 542, 
		DIGI_JG44052= 543, 
		DIGI_JG45014= 544, 
		DIGI_JG44068= 545, 
		DIGI_JG45010= 546, 
		DIGI_JG45018= 547, 
		DIGI_JG45030= 548, 
		DIGI_JG45033= 549, 
		DIGI_JG45043= 550, 
		DIGI_JG45053= 551, 
		DIGI_JG45067= 552, 
		DIGI_JG46005= 553, 
		DIGI_JG46010= 554, 
		DIGI_LANI049= 555, 
		DIGI_LANI051= 556, 
		DIGI_LANI052= 557, 
		DIGI_LANI054= 558, 
		DIGI_LANI060= 559, 
		DIGI_LANI063= 560, 
		DIGI_LANI065= 561, 
		DIGI_LANI066= 562, 
		DIGI_LANI073= 563, 
		DIGI_LANI075= 564, 
		DIGI_LANI077= 565, 
		DIGI_LANI079= 566, 
		DIGI_LANI089= 567, 
		DIGI_LANI091= 568, 
		DIGI_LANI093= 569, 
		DIGI_LANI095= 570, 
		DIGI_VENTWALK= 571, 
		DIGI_CARWALK= 572, 
		DIGI_JETSOAR= 573, 
		DIGI_VACUUM= 574, 
		DIGI_GIRLNINJAALERTT= 575, 
		DIGI_GIRLNINJASCREAM= 576, 
		DIGI_GIRLNINJAALERT= 577, 
		DIGI_PRUNECACKLE= 578, 
		DIGI_PRUNECACKLE2= 579, 
		DIGI_PRUNECACKLE3= 580, 
		DIGI_SUMOSTOMP= 581, 
		DIGI_VATOR= 582, 
		DIGI_JG9009= 583, 
		DIGI_Z16004= 584, 
		DIGI_Z16012= 585, 
		DIGI_Z16022= 586, 
		DIGI_Z16027= 587, 
		DIGI_JG93030= 588, 
		DIGI_JG94002= 589, 
		DIGI_Z17010= 590, 
		DIGI_Z17052= 591, 
		DIGI_Z17025= 592, 
		DIGI_ML25014= 593, 
		DIGI_ML250101= 594, 
		DIGI_JG9022= 595, 
		DIGI_JG9032= 596, 
		DIGI_JG9038= 597, 
		DIGI_JG9055= 598, 
		DIGI_JG9060= 599, 
		DIGI_JG92055= 600, 
		DIGI_ML25032= 601, 
		DIGI_JG92036= 602, 
		DIGI_JG92042= 603, 
		DIGI_ML26001= 604, 
		DIGI_JG93000= 605, 
		DIGI_JG93011= 606, 
		DIGI_JG93018= 607, 
		DIGI_JG93023= 608, 
		DIGI_ML26008= 609, 
		DIGI_ML26011= 610, 
		DIGI_JG94007= 611, 
		DIGI_JG94024= 612, 
		DIGI_JG94039= 613, 
		DIGI_JG95012= 614, 
		DIGI_ZILLASTOMP= 615, 
		DIGI_ZC1= 616, 
		DIGI_ZC2= 617, 
		DIGI_ZC3= 618, 
		DIGI_ZC4= 619, 
		DIGI_ZC5= 620, 
		DIGI_ZC6= 621, 
		DIGI_ZC7= 622, 
		DIGI_ZC8= 623, 
		DIGI_ZC9= 624, 
		DIGI_Z16043= 625, 
	}
}
