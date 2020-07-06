#pragma once
#include "tflags.h"

// Most of these should be replaced by CCMDs eventually
enum GameFunction_t
{
	gamefunc_Move_Forward,
	gamefunc_Move_Backward,
	gamefunc_Turn_Left,
	gamefunc_Turn_Right,
	gamefunc_Strafe,
	gamefunc_Fire,
	gamefunc_Open,
	gamefunc_Run,
	gamefunc_Alt_Fire,
	gamefunc_Jump,
	gamefunc_Crouch,
	gamefunc_Look_Up,
	gamefunc_Look_Down,
	gamefunc_Look_Left,
	gamefunc_Look_Right,
	gamefunc_Strafe_Left,
	gamefunc_Strafe_Right,
	gamefunc_Aim_Up,
	gamefunc_Aim_Down,
	gamefunc_Weapon_1, // CCMD
	gamefunc_Weapon_2, // CCMD
	gamefunc_Weapon_3, // CCMD
	gamefunc_Weapon_4, // CCMD
	gamefunc_Weapon_5, // CCMD
	gamefunc_Weapon_6, // CCMD
	gamefunc_Weapon_7, // CCMD
	gamefunc_Weapon_8, // CCMD
	gamefunc_Weapon_9, // CCMD
	gamefunc_Weapon_10, // CCMD
	gamefunc_Inventory, // CCMD
	gamefunc_Inventory_Left, // CCMD
	gamefunc_Inventory_Right, // CCMD
	gamefunc_Holo_Duke, // CCMD
	gamefunc_Jetpack, // CCMD
	gamefunc_NightVision, // CCMD
	gamefunc_MedKit, // CCMD
	gamefunc_TurnAround,
	gamefunc_SendMessage,
	gamefunc_Map, // CCMD
	gamefunc_Shrink_Screen, // CCMD
	gamefunc_Enlarge_Screen, // CCMD
	gamefunc_Center_View, // CCMD
	gamefunc_Holster_Weapon, // CCMD
	gamefunc_Show_Opponents_Weapon, // CCMD
	gamefunc_Map_Follow_Mode, // CCMD
	gamefunc_See_Coop_View, // CCMD
	gamefunc_Mouse_Aiming, // CCMD
	gamefunc_Toggle_Crosshair, // CCMD
	gamefunc_Steroids, // CCMD
	gamefunc_Quick_Kick,
	gamefunc_Next_Weapon, // CCMD
	gamefunc_Previous_Weapon, // CCMD
	gamefunc_Dpad_Select,
	gamefunc_Dpad_Aiming,
	gamefunc_Last_Weapon, // CCMD
	gamefunc_Alt_Weapon,
	gamefunc_Third_Person_View, // CCMD
	gamefunc_Show_DukeMatch_Scores, // CCMD
	gamefunc_Toggle_Crouch,
	NUM_ACTIONS
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

// And yet another bit field that was for all intents and purposes undocumented, depending on numeric literals again.
// And again, the symbolic names are from EDuke32.
enum ESyncVals
{
    // Todo: Make this bit masks - cannot be done before eliminating all old code using it
    SK_JUMP         = 0 ,
    SK_CROUCH       = 1 ,
    SK_FIRE         = 2 ,
    SK_AIM_UP       = 3 ,
    SK_AIM_DOWN     = 4 ,
    SK_RUN          = 5 ,
    SK_LOOK_LEFT    = 6 ,
    SK_LOOK_RIGHT   = 7 ,
    // weapons take up 4 bits...
    SK_WEAPON_BITS  = 8 ,
    SK_WEAPON_BITS1 = 9 ,
    SK_WEAPON_BITS2 = 10,
    SK_WEAPON_BITS3 = 11,
    SK_STEROIDS     = 12,
    SK_LOOK_UP      = 13,
    SK_LOOK_DOWN    = 14,
    SK_NIGHTVISION  = 15,
    SK_MEDKIT       = 16,
    SK_MULTIFLAG    = 17,
    SK_CENTER_VIEW  = 18,
    SK_HOLSTER      = 19,
    SK_INV_LEFT     = 20,
    SK_PAUSE        = 21,
    SK_QUICK_KICK   = 22,
    SK_AIMMODE      = 23,
    SK_HOLODUKE     = 24,
    SK_JETPACK      = 25,
    SK_GAMEQUIT     = 26,
    SK_INV_RIGHT    = 27,
    SK_TURNAROUND   = 28,
    SK_OPEN         = 29,
    SK_INVENTORY    = 30,
    SK_ESCAPE       = 31,
};

enum ESyncBits_ : uint32_t
{
    SKB_JUMP = 1 << 0,
    SKB_CROUCH = 1 << 1,
    SKB_FIRE = 1 << 2,
    SKB_AIM_UP = 1 << 3,
    SKB_AIM_DOWN = 1 << 4,
    SKB_RUN = 1 << 5,
    SKB_LOOK_LEFT = 1 << 6,
    SKB_LOOK_RIGHT = 1 << 7,
    SKB_STEROIDS = 1 << 12,
    SKB_LOOK_UP = 1 << 13,
    SKB_LOOK_DOWN = 1 << 14,
    SKB_NIGHTVISION = 1 << 15,
    SKB_MEDKIT = 1 << 16,
    SKB_MULTIFLAG = 1 << 17,
    SKB_CENTER_VIEW = 1 << 18,
    SKB_HOLSTER = 1 << 19,
    SKB_INV_LEFT = 1 << 20,
    SKB_PAUSE = 1 << 21,
    SKB_QUICK_KICK = 1 << 22,
    SKB_AIMMODE = 1 << 23,
    SKB_HOLODUKE = 1 << 24,
    SKB_JETPACK = 1 << 25,
    SKB_GAMEQUIT = 1 << 26,
    SKB_INV_RIGHT = 1 << 27,
    SKB_TURNAROUND = 1 << 28,
    SKB_OPEN = 1 << 29,
    SKB_INVENTORY = 1 << 30,
    SKB_ESCAPE = 1u << 31,

    SKB_WEAPONMASK_BITS = (15u << int(SK_WEAPON_BITS)),
    SKB_INTERFACE_BITS = (SKB_WEAPONMASK_BITS | SKB_STEROIDS | SKB_NIGHTVISION | SKB_MEDKIT | SKB_QUICK_KICK | \
        SKB_HOLSTER | SKB_INV_LEFT | SKB_PAUSE | SKB_HOLODUKE | SKB_JETPACK | SKB_INV_RIGHT | \
        SKB_TURNAROUND | SKB_OPEN | SKB_INVENTORY | SKB_ESCAPE),

    SKB_NONE = 0,
    SKB_ALL = ~0u

};

// enforce type safe operations on the input bits.
using ESyncBits = TFlags<ESyncBits_, uint32_t>;
inline ESyncBits operator <<(int v, ESyncVals s) { return ESyncBits::FromInt(v << int(s)); }
inline ESyncBits operator <<(unsigned v, ESyncVals s) { return ESyncBits::FromInt(v << int(s)); }
inline ESyncBits operator <<(bool v, ESyncVals s) { return ESyncBits::FromInt(v << int(s)); }
DEFINE_TFLAGS_OPERATORS(ESyncBits)

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
	STAT_NETALLOC       = MAXSTATUS-1
};

