#pragma once

#include <stdint.h>
#include "fix16.h"
#include "tflags.h"


// Blood flags
enum
{
    flag_buttonmask = 127,
    flag_buttonmask_norun = 126
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
	SKB_FIRST_WEAPON_BIT = 1 << 8,
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

	SKB_WEAPONMASK_BITS = (15u * SKB_FIRST_WEAPON_BIT),
	SKB_INTERFACE_BITS = (SKB_WEAPONMASK_BITS | SKB_STEROIDS | SKB_NIGHTVISION | SKB_MEDKIT | SKB_QUICK_KICK | \
		SKB_HOLSTER | SKB_INV_LEFT | SKB_PAUSE | SKB_HOLODUKE | SKB_JETPACK | SKB_INV_RIGHT | \
		SKB_TURNAROUND | SKB_OPEN | SKB_INVENTORY | SKB_ESCAPE),

	SKB_NONE = 0,
	SKB_ALL = ~0u

};

// enforce type safe operations on the input bits.
using EDukeSyncBits = TFlags<ESyncBits_, uint32_t>;
DEFINE_TFLAGS_OPERATORS(EDukeSyncBits)

union SYNCFLAGS
{
    uint32_t value;
    struct
    {
        unsigned int run : 1;
        unsigned int jump : 1;
        unsigned int crouch : 1;
        unsigned int shoot : 1;
        unsigned int shoot2 : 1;
        unsigned int lookUp : 1;
        unsigned int lookDown : 1;
        unsigned int action : 1;
        unsigned int jab : 1;
        unsigned int prevItem : 1;
        unsigned int nextItem : 1;
        unsigned int useItem : 1;
        unsigned int prevWeapon : 1;
        unsigned int nextWeapon : 1;
        unsigned int holsterWeapon : 1;
        unsigned int lookCenter : 1;
        unsigned int lookLeft : 1;
        unsigned int lookRight : 1;
        unsigned int spin180 : 1;
        unsigned int pause : 1;
        unsigned int quit : 1;
        unsigned int restart : 1;
        unsigned int useBeastVision : 1;
        unsigned int useCrystalBall : 1;
        unsigned int useJumpBoots : 1;
        unsigned int useMedKit : 1;
        unsigned int newWeapon : 4;
    };
};

// SW

//
// NETWORK - REDEFINABLE SHARED (SYNC) KEYS BIT POSITIONS
//

// weapons takes up 4 bits
#define SK_WEAPON_BIT0 0
#define SK_WEAPON_BIT1 1
#define SK_WEAPON_BIT2 2
#define SK_WEAPON_BIT3 3
#define SK_WEAPON_MASK (BIT(SK_WEAPON_BIT0)| \
                        BIT(SK_WEAPON_BIT1)| \
                        BIT(SK_WEAPON_BIT2)| \
                        BIT(SK_WEAPON_BIT3))     // 16 possible numbers 0-15

#define SK_INV_HOTKEY_BIT0 4
#define SK_INV_HOTKEY_BIT1 5
#define SK_INV_HOTKEY_BIT2 6
#define SK_INV_HOTKEY_MASK (BIT(SK_INV_HOTKEY_BIT0)|BIT(SK_INV_HOTKEY_BIT1)|BIT(SK_INV_HOTKEY_BIT2))

#define SK_AUTO_AIM    7
#define SK_CENTER_VIEW 8
#define SK_PAUSE       9

#define SK_MESSAGE    11
#define SK_LOOK_UP    12
#define SK_LOOK_DOWN  13
#define SK_CRAWL_LOCK 14
#define SK_FLY        15

#define SK_RUN        16
#define SK_SHOOT      17
#define SK_OPERATE    18
#define SK_JUMP       19
#define SK_CRAWL      20
#define SK_SNAP_UP    21
#define SK_SNAP_DOWN  22
#define SK_QUIT_GAME  23

#define SK_MULTI_VIEW 24

#define SK_TURN_180   25

#define SK_INV_LEFT   26
#define SK_INV_RIGHT  27

#define SK_INV_USE   29
#define SK_HIDE_WEAPON  30
#define SK_SPACE_BAR  31


// Exhumed

enum {
    kButtonJump = 0x1,
    kButtonOpen = 0x4,
    kButtonFire = 0x8,
    kButtonCrouch = 0x10,
    kButtonCheatGuns = 0x20,
    kButtonCheatGodMode = 0x40,
    kButtonCheatKeys = 0x80,
    kButtonCheatItems = 0x100,

    kButtonWeaponShift = 13,
    kButtonWeaponBits = 7 << kButtonWeaponShift, // upper 3 bits.
};






struct InputPacket
{
    int16_t svel;
    int16_t fvel;
    fix16_t q16avel;
    fix16_t q16horz;
    fix16_t q16aimvel;	// only used by SW
    fix16_t q16ang;		// only used by SW
	
    // Making this a union lets some constructs fail. Since these names are transitional only the added memory use doesn't really matter.
        // for Duke
        EDukeSyncBits sbits;

        // for SW
        int32_t bits;

        // for Blood
        SYNCFLAGS syncFlags;

        // For Exhumed
        uint16_t buttons;
};
