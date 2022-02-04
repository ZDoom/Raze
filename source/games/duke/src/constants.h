#pragma once
#include "tflags.h"
#include "coreactor.h"

// all game constants got collected here.

enum
{
	TICRATE = 120,
	REALGAMETICSPERSEC = 30, // The number of game state updates per second:
	TICSPERFRAME = (TICRATE/REALGAMETICSPERSEC) // (This used to be TICRATE/GAMETICSPERSEC, which was 120/26 = 4.615~ truncated to 4 by integer division.)
};

// tile names which are identical for all games.
enum
{
	SECTOREFFECTOR = 1,
	ACTIVATOR = 2,
	TOUCHPLATE = 3,
	ACTIVATORLOCKED = 4,
	MUSICANDSFX = 5,
	LOCATORS = 6,
	CYCLER = 7,
	MASTERSWITCH = 8,
	RESPAWN = 9,
	GPSPEED = 10,
	FOF = 13,

	TILE_VIEWSCR = (MAXTILES-5)

};	

// the available palettes. These are indices into the global table of translations.
enum basepal_t {
	BASEPAL = 0,
	WATERPAL,
	SLIMEPAL,
	TITLEPAL,
	DREALMSPAL,
	ENDINGPAL,  // 5
	ANIMPAL,    // not used anymore. The anim code now generates true color textures.
	DRUGPAL,
	BASEPALCOUNT
};

// sector effector lotags, from EDuke32. The original code used numeric literals for these, substitution is not complete.
enum
{
	SE_0_ROTATING_SECTOR              = 0,
	SE_1_PIVOT                        = 1,
	SE_2_EARTHQUAKE                   = 2,
	SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT = 3,
	SE_4_RANDOM_LIGHTS                = 4,
	SE_5_BOSS                         = 5,
	SE_6_SUBWAY                       = 6,

	SE_7_TELEPORT                      = 7,
	SE_8_UP_OPEN_DOOR_LIGHTS           = 8,
	SE_9_DOWN_OPEN_DOOR_LIGHTS         = 9,
	SE_10_DOOR_AUTO_CLOSE              = 10,
	SE_11_SWINGING_DOOR                = 11,
	SE_12_LIGHT_SWITCH                 = 12,
	SE_13_EXPLOSIVE                    = 13,
	SE_14_SUBWAY_CAR                   = 14,
	SE_15_SLIDING_DOOR                 = 15,
	SE_16_REACTOR                      = 16,
	SE_17_WARP_ELEVATOR                = 17,
	SE_18_INCREMENTAL_SECTOR_RISE_FALL = 18,
	SE_19_EXPLOSION_LOWERS_CEILING     = 19,
	SE_20_STRETCH_BRIDGE               = 20,
	SE_21_DROP_FLOOR                   = 21,
	SE_22_TEETH_DOOR                   = 22,
	SE_23_ONE_WAY_TELEPORT             = 23,
	SE_24_CONVEYOR                     = 24,
	SE_25_PISTON                       = 25,
	SE_26                              = 26,
	SE_27_DEMO_CAM                     = 27,
	SE_28_LIGHTNING                    = 28,
	SE_29_WAVES                        = 29,
	SE_30_TWO_WAY_TRAIN                = 30,
	SE_31_FLOOR_RISE_FALL              = 31,
	SE_32_CEILING_RISE_FALL            = 32,
	SE_33_QUAKE_DEBRIS                 = 33,
	SE_34                              = 34,
	SE_35                              = 35,
	SE_36_PROJ_SHOOTER                 = 36,
	SE_47_LIGHT_SWITCH                 = 47,
	SE_48_LIGHT_SWITCH                 = 48,
	SE_49_POINT_LIGHT                  = 49,
	SE_50_SPOT_LIGHT                   = 50,
	SE_128_GLASS_BREAKING              = 128,
	SE_130                             = 130,
	SE_131                             = 131,
};

// sector lotags, also from EDuke32, for the same reason as above.
enum
{
	ST_0_NO_EFFECT   = 0,
	ST_1_ABOVE_WATER = 1,
	ST_2_UNDERWATER  = 2,
	ST_3             = 3,
	// ^^^ maybe not complete substitution in code
	ST_9_SLIDING_ST_DOOR     = 9,
	ST_15_WARP_ELEVATOR      = 15,
	ST_16_PLATFORM_DOWN      = 16,
	ST_17_PLATFORM_UP        = 17,
	ST_18_ELEVATOR_DOWN      = 18,
	ST_19_ELEVATOR_UP        = 19,
	ST_20_CEILING_DOOR       = 20,
	ST_21_FLOOR_DOOR         = 21,
	ST_22_SPLITTING_DOOR     = 22,
	ST_23_SWINGING_DOOR      = 23,
	ST_25_SLIDING_DOOR       = 25,
	ST_26_SPLITTING_ST_DOOR  = 26,
	ST_27_STRETCH_BRIDGE     = 27,
	ST_28_DROP_FLOOR         = 28,
	ST_29_TEETH_DOOR         = 29,
	ST_30_ROTATE_RISE_BRIDGE = 30,
	ST_31_TWO_WAY_TRAIN      = 31,

	ST_41_JAILDOOR			= 41,
	ST_42_MINECART			= 42,
	// left: ST 32767, 65534, 65535
};

// These actually existed in the original source but were never used. Weird.
enum dukeinv_t
{
	GET_STEROIDS,  // 0
	GET_SHIELD,
	GET_SCUBA,
	GET_HOLODUKE,
	GET_JETPACK,
	GET_DUMMY1,  // 5
	GET_ACCESS,
	GET_HEATS,
	GET_DUMMY2,
	GET_FIRSTAID,
	GET_BOOTS,  // 10
	GET_MAX
};

// Again from EDuke - only numeric literals were used in the original source.
// these are not in the same order as the above, and it can't be changed for compat reasons. lame!
enum dukeinvicon_t
{
	ICON_NONE,  // 0
	ICON_FIRSTAID,
	ICON_STEROIDS,
	ICON_HOLODUKE,
	ICON_JETPACK,
	ICON_HEATS,  // 5
	ICON_SCUBA,
	ICON_BOOTS,
	ICON_MAX
};



enum EQuote
{
	QUOTE_SHOW_MAP_OFF          = 1  ,
	QUOTE_ACTIVATED             = 2  ,
	QUOTE_MEDKIT                = 3  ,
	QUOTE_LOCKED                = 4  ,
	QUOTE_CHEAT_EVERYTHING      = 5  ,
	QUOTE_BOOTS                 = 6  ,
	QUOTE_WASTED                = 7  ,
	QUOTE_UNLOCKED              = 8  ,
	QUOTE_FOUND_SECRET          = 9  ,
	QUOTE_SQUISHED              = 10 ,
	QUOTE_USED_STEROIDS         = 12 ,
	QUOTE_DEAD                  = 13 ,
	QUOTE_DEACTIVATED           = 15 ,
	QUOTE_CHEAT_GODMODE_ON      = 17 ,
	QUOTE_CHEAT_GODMODE_OFF     = 18 ,
	QUOTE_CROSSHAIR_OFF         = 21 ,
	QUOTE_CHEATS_DISABLED       = 22 ,
	QUOTE_MESSAGES_ON           = 23 ,
	QUOTE_MESSAGES_OFF          = 24 ,
	QUOTE_MUSIC                 = 26 ,
	QUOTE_CHEAT_STEROIDS        = 37 ,
	QUOTE_F1HELP                = 40 ,
	QUOTE_MOUSE_AIMING_OFF      = 44 ,
	QUOTE_HOLODUKE_ON           = 47 ,
	QUOTE_HOLODUKE_OFF          = 48 ,
	QUOTE_HOLODUKE_NOT_FOUND    = 49 ,
	QUOTE_JETPACK_NOT_FOUND     = 50 ,
	QUOTE_JETPACK_ON            = 52 ,
	QUOTE_JETPACK_OFF           = 53 ,
	QUOTE_NEED_BLUE_KEY         = 70 ,
	QUOTE_NEED_RED_KEY          = 71 ,
	QUOTE_NEED_YELLOW_KEY       = 72 ,
	QUOTE_WEAPON_LOWERED        = 73 ,
	QUOTE_WEAPON_RAISED         = 74 ,
	QUOTE_BOOTS_ON              = 75 ,
	QUOTE_SCUBA_ON              = 76 ,
	QUOTE_CHEAT_ALLEN           = 79 ,
	QUOTE_MIGHTY_FOOT           = 80 ,
	QUOTE_WEAPON_MODE_OFF       = 82 ,
	QUOTE_MAP_FOLLOW_OFF        = 83 ,
	QUOTE_MAP_FOLLOW_ON         = 84 ,
	QUOTE_RUN_MODE_OFF          = 85 ,
	QUOTE_JETPACK               = 88 ,
	QUOTE_SCUBA                 = 89 ,
	QUOTE_STEROIDS              = 90 ,
	QUOTE_HOLODUKE              = 91 ,
	QUOTE_CHEAT_TODD            = 99 ,
	QUOTE_CHEAT_UNLOCK          = 100,
	QUOTE_NVG                   = 101,
	QUOTE_WEREGONNAFRYYOURASS   = 102,
	QUOTE_SCREEN_SAVED          = 103,
	QUOTE_CHEAT_BETA            = 105,
	QUOTE_NVG_OFF               = 107,
	QUOTE_VIEW_MODE_OFF         = 109,
	QUOTE_SHOW_MAP_ON           = 111,
	QUOTE_CHEAT_CLIP            = 112,
	QUOTE_CHEAT_NOCLIP          = 113,
	QUOTE_SAVE_BAD_VERSION      = 114,
	QUOTE_RESERVED              = 115,
	QUOTE_RESERVED2             = 116,
	QUOTE_RESERVED3             = 117,
	QUOTE_SAVE_DEAD             = 118,
	QUOTE_CHEAT_ALL_WEAPONS     = 119,
	QUOTE_CHEAT_ALL_INV         = 120,
	QUOTE_CHEAT_ALL_KEYS        = 121,
	QUOTE_RESERVED4             = 122,
	QUOTE_SAVE_BAD_PLAYERS      = 124,

	QUOTE_ON_BIKE				= 126,
	QUOTE_CHEAT_KILL			= 127,
	QUOTE_YERFUCKED				= 128,
	QUOTE_BOATMODEON			= 129,
	QUOTE_INSTADRUNK			= 131,
	QUOTE_INSTASOBER			= 132,
	QUOTE_NOCHEATS				= 139,
	QUOTE_ON_BOAT				= 136,
	QUOTE_BOATMODEOFF			= 137,
	QUOTE_CHEAT_KFC				= 139,
};

enum
{
	STAT_DEFAULT        = 0,
	STAT_ACTOR          = 1,
	STAT_ZOMBIEACTOR    = 2,
	STAT_EFFECTOR       = 3,
	STAT_PROJECTILE     = 4,
	STAT_MISC           = 5,
	STAT_STANDABLE      = 6,
	STAT_LOCATOR        = 7,
	STAT_ACTIVATOR      = 8,
	STAT_TRANSPORT      = 9,
	STAT_PLAYER         = 10,
	STAT_FX             = 11,
	STAT_FALLER         = 12,
	STAT_DUMMYPLAYER    = 13,
	STAT_LIGHT          = 14,
	STAT_RAROR          = 15,

	STAT_DESTRUCT		= 100,
	STAT_BOWLING		= 105,
	STAT_REMOVED		= MAXSTATUS-2,
	STAT_NETALLOC       = MAXSTATUS-1
};

enum
{
	MAXCYCLERS      = 1024,
	MAXANIMATES     = 1024,
	MAXANIMWALLS    = 512,
	MAXANIMPOINTS   = 2048,
	MAXCRANES		= 16,
};

enum amoveflags_t
{
	face_player       = 1,
	geth              = 2,
	getv              = 4,
	random_angle      = 8,
	face_player_slow  = 16,
	spin              = 32,
	face_player_smart = 64,
	fleeenemy         = 128,
	jumptoplayer_only = 256,
	justjump1 = 256,
	jumptoplayer      = 257,
	seekplayer        = 512,
	furthestdir       = 1024,
	dodgebullet       = 4096,
	justjump2         = 8192,
	windang           = 16384,
	antifaceplayerslow = 32768
};

enum sflags_t
{
	SFLAG_INVENTORY				= 0x00000001,
	SFLAG_SHRINKAUTOAIM			= 0x00000002,
	SFLAG_BADGUY				= 0x00000004,
	SFLAG_FORCEAUTOAIM			= 0x00000008,
	SFLAG_BOSS					= 0x00000010,
	SFLAG_BADGUYSTAYPUT			= 0x00000020,
	SFLAG_GREENSLIMEFOOD		= 0x00800040,
	SFLAG_NODAMAGEPUSH			= 0x00000080,
	SFLAG_NOWATERDIP			= 0x00000100,
	SFLAG_INTERNAL_BADGUY		= 0x00000200, // a separate flag is needed for the internal ones because SFLAG_BADGUY has additional semantics.
	SFLAG_KILLCOUNT				= 0x00000400,
	SFLAG_NOCANSEECHECK			= 0x00000800,
	SFLAG_HITRADIUSCHECK		= 0x00001000,
	SFLAG_MOVEFTA_CHECKSEE		= 0x00002000,
	SFLAG_MOVEFTA_MAKESTANDABLE = 0x00004000,
	SFLAG_TRIGGER_IFHITSECTOR	= 0x00008000,
	SFLAG_MOVEFTA_WAKEUPCHECK	= 0x00010000,
	SFLAG_MOVEFTA_CHECKSEEWITHPAL8 = 0x00020000,	// let's hope this can be done better later. For now this was what blocked merging the Duke and RR variants of movefta
	SFLAG_NOSHADOW				= 0x00040000,
	SFLAG_SE24_NOCARRY			= 0x00080000,
	SFLAG_NOINTERPOLATE			= 0x00100000,
	SFLAG_FALLINGFLAMMABLE		= 0x00200000,
	SFLAG_FLAMMABLEPOOLEFFECT	= 0x00400000,
	SFLAG_INFLAME				= 0x00800000,
	SFLAG_NOFLOORFIRE			= 0x01000000,
	SFLAG_HITRADIUS_FLAG1		= 0x02000000,
	SFLAG_HITRADIUS_FLAG2		= 0x04000000,
	SFLAG_CHECKSLEEP			= 0x08000000,
	SFLAG_NOTELEPORT			= 0x10000000,
	SFLAG_SE24_REMOVE			= 0x20000000,
	SFLAG_BLOCK_TRIPBOMB		= 0x40000000,
	SFLAG_NOFALLER				= 0x80000000,
};

using EDukeFlags1 = TFlags<sflags_t, uint32_t>;
DEFINE_TFLAGS_OPERATORS(EDukeFlags1)

enum sflags2_t
{
	SFLAG2_USEACTIVATOR = 0x00000001,
	SFLAG2_NOROTATEWITHSECTOR	= 0x00000002,
	SFLAG2_SHOWWALLSPRITEONMAP	= 0x00000004,
	SFLAG2_NOFLOORPAL			= 0x00000008,
	SFLAG2_EXPLOSIVE			= 0x00000010,
	SFLAG2_BRIGHTEXPLODE		= 0x00000020,
	SFLAG2_DOUBLEDMGTHRUST		= 0x00000040,
	SFLAG2_BREAKMIRRORS			= 0x00000080,
	SFLAG2_CAMERA				= 0x00000100,
	SFLAG2_DONTANIMATE			= 0x00000200,
};

using EDukeFlags2 = TFlags<sflags2_t, uint32_t>;
DEFINE_TFLAGS_OPERATORS(EDukeFlags2)

enum
{
	TFLAG_WALLSWITCH			= 1,
	TFLAG_ADULT					= 2,
	TFLAG_ELECTRIC				= 4,
	TFLAG_CLEARINVENTORY		= 8,	// really dumb Duke stuff...
	TFLAG_SLIME					= 16,
};

enum
{
	EVENT_INIT = 0,
	EVENT_ENTERLEVEL,
	EVENT_RESETWEAPONS,	// for each player
	EVENT_RESETINVENTORY, // for each player
	EVENT_HOLSTER,		// for each player
	EVENT_LOOKLEFT,		// for each player
	EVENT_LOOKRIGHT,	// for each player
	EVENT_SOARUP,		// for each player
	EVENT_SOARDOWN,		// for each player
	EVENT_CROUCH,		// for each player
	EVENT_JUMP,			// for each player
	EVENT_RETURNTOCENTER,	// for each player
	EVENT_LOOKUP,		// for each player
	EVENT_LOOKDOWN,		// for each player
	EVENT_AIMUP,		// for each player
	EVENT_AIMDOWN = EVENT_AIMUP,		// for each player, typo in WW2GI
	EVENT_FIRE,			// for each player
	EVENT_CHANGEWEAPON,	// for each player
	EVENT_GETSHOTRANGE,	// for each player
	EVENT_GETAUTOAIMANGLE,	// for each player
	EVENT_GETLOADTILE,

	EVENT_CHEATGETSTEROIDS,
	EVENT_CHEATGETHEAT,
	EVENT_CHEATGETBOOT,
	EVENT_CHEATGETSHIELD,
	EVENT_CHEATGETSCUBA,
	EVENT_CHEATGETHOLODUKE,
	EVENT_CHEATGETJETPACK,
	EVENT_CHEATGETFIRSTAID,
	EVENT_QUICKKICK,
	EVENT_INVENTORY,
	EVENT_USENIGHTVISION,
	EVENT_USESTEROIDS,
	EVENT_INVENTORYLEFT,
	EVENT_INVENTORYRIGHT,
	EVENT_HOLODUKEON,
	EVENT_HOLODUKEOFF,
	EVENT_USEMEDKIT,
	EVENT_USEJETPACK,
	EVENT_TURNAROUND,
	// The ones in-between here are not supported and many may never be.
	EVENT_SHOOT = 51,

	EVENT_NUMEVENTS,
	EVENT_MAXEVENT = EVENT_NUMEVENTS - 1
};

enum miscConstants
{
	MAXSLEEPDIST = 16384,
	SLEEPTIME = 1536,
	ZOFFSET6 = (4 << 8),
	FOURSLEIGHT = (1 << 8),

	MOVEFIFOSIZ     =256,
	AUTO_AIM_ANGLE  =48,
	PHEIGHT_DUKE    =38,
	PHEIGHT_RR      =40,

	MAXMINECARTS = 16,
	MAXJAILDOORS = 32,
	MAXLIGHTNINSECTORS = 64,
	MAXTORCHSECTORS = 64,
	MAXGEOSECTORS = 64,

	CRACK_TIME = 777,
	PISTOL_MAXDEFAULT = 200,

	DUKE3D_NO_WIDESCREEN_PINNING = 1 << 0,
};

constexpr double FOURSLEIGHT_F = 1.0;

enum {
	MUS_INTRO = 0,
	MUS_BRIEFING = 1,
	MUS_LOADING = 2,
};


enum
{
	// Control flags for WW2GI weapons.
	TRIPBOMB_TRIPWIRE = 1,
	TRIPBOMB_TIMER = 2
};

enum gamemode_t {
	MODE_GAME = 0x00000004,
	MODE_EOL = 0x00000008,
};
