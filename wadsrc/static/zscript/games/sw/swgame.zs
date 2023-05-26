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

	enum ETexFlags // texture flags
	{
		TFLAG_BUNNYFRIENDLY = 1,
	};

	native static void PlaySound(Sound snd, int flags, int channel = CHAN_AUTO, int cflags = 0);
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
    double RevolveAng;
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
    native double z_speed;
    native int climb_ndx;
    native double hiz,loz;
    native double p_ceiling_dist,p_floor_dist;
    native double circle_camera_dist;

    native int friction;
    native int16 slide_ang;
    native int slide_dec;
    native float drive_avel;

    native double circle_camera_ang;
    native int16 camera_check_time_delay;

    //native int /*cursectnum,*/lastcursectnum;

    // variables that do not fit into sprite structure
    native double recoil_amt;
    native int16 recoil_speed;
    native int16 recoil_ndx;
    native double recoil_horizoff;

    native double RevolveDeltaAng;

    native int16 pnum; // carry along the player number

    //native int16 LadderSector;
    native int16 JumpDuration;
    native int16 WadeDepth;
    native double pbob_amt;
    native int16 bob_ndx;
    native int16 bcnt; // bob count
    native double bob_z, obob_z;

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
