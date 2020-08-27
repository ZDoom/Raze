#pragma once

#include <stdint.h>
#include "fix16.h"
#include "tflags.h"

enum ESyncBits_ : uint32_t
{
    SB_FIRST_WEAPON_BIT = 1 << 0,
    SB_ITEM_BIT_1 = 1 << 4,
    SB_ITEM_BIT_2 = 1 << 5,
    SB_ITEM_BIT_3 = 1 << 6,
    SB_ITEM_BIT_4 = 1 << 7,
    SB_ITEM_BIT_5 = 1 << 8,
    SB_ITEM_BIT_6 = 1 << 9,
    SB_ITEM_BIT_7 = 1 << 10,

    // Exhumed has 6 items but doesn't use the network packet to activate them. Need to change


    SB_WEAPONMASK_BITS = (15u * SB_FIRST_WEAPON_BIT), // Weapons take up 4 bits
    SB_ITEMUSE_BITS = (127u * SB_ITEM_BIT_1),

    SB_BUTTON_MASK = 0,     // all input from buttons (i.e. active while held)
    SB_INTERFACE_MASK = 0,  // all input from CCMDs
    SB_INTERFACE_BITS = (SB_WEAPONMASK_BITS | SB_ITEMUSE_BITS | SB_INTERFACE_MASK)
};

// enforce type safe operations on the input bits.
using ESyncBits = TFlags<ESyncBits_, uint32_t>;
DEFINE_TFLAGS_OPERATORS(ESyncBits)


// Blood flags
enum
{
    flag_buttonmask = 127,
    flag_buttonmask_norun = 126
};


enum
{
    // The maximum valid weapons for the respective games.
    WeaponSel_Max = 10,
    WeaponSel_MaxExhumed = 7,
    WeaponSel_MaxBlood = 12,

    // Use named constants instead of magic values for these. The maximum of the supported games is 12 weapons for Blood so these 3 are free.
    // Should there ever be need for more weapons to select, the bit field needs to be expanded.
    WeaponSel_Next = 13,
    WeaponSel_Prev = 14,
    WeaponSel_Alt = 15
};

enum EDukeSyncBits_ : uint32_t
{
	SKB_JUMP = 1 << 0,
	SKB_CROUCH = 1 << 1,
	SKB_FIRE = 1 << 2,
	SKB_AIM_UP = 1 << 3,
	SKB_AIM_DOWN = 1 << 4,
	SKB_RUN = 1 << 5,
	SKB_LOOK_LEFT = 1 << 6,
	SKB_LOOK_RIGHT = 1 << 7,
	SKB_LOOK_UP = 1 << 13,
	SKB_LOOK_DOWN = 1 << 14,
	SKB_MULTIFLAG = 1 << 17,
	SKB_CENTER_VIEW = 1 << 18,
	SKB_HOLSTER = 1 << 19,
	SKB_INV_LEFT = 1 << 20,
	SKB_PAUSE = 1 << 21,
	SKB_QUICK_KICK = 1 << 22,
	SKB_AIMMODE = 1 << 23,
	SKB_GAMEQUIT = 1 << 26,
	SKB_INV_RIGHT = 1 << 27,
	SKB_TURNAROUND = 1 << 28,
	SKB_OPEN = 1 << 29,
	SKB_INVENTORY = 1 << 30,
	SKB_ESCAPE = 1u << 31,

	SKB_INTERFACE_BITS = (SKB_QUICK_KICK | \
		SKB_HOLSTER | SKB_INV_LEFT | SKB_PAUSE | SKB_INV_RIGHT | \
		SKB_TURNAROUND | SKB_OPEN | SKB_INVENTORY | SKB_ESCAPE),

	SKB_NONE = 0,
	SKB_ALL = ~0u

};

// enforce type safe operations on the input bits.
using EDukeSyncBits = TFlags<EDukeSyncBits_, uint32_t>;
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
        unsigned int holsterWeapon : 1;
        unsigned int lookCenter : 1;
        unsigned int lookLeft : 1;
        unsigned int lookRight : 1;
        unsigned int spin180 : 1;
        unsigned int pause : 1;
        unsigned int quit : 1;
        unsigned int restart : 1;
    };
};

// SW

//
// NETWORK - REDEFINABLE SHARED (SYNC) KEYS BIT POSITIONS
//


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
};






struct InputPacket
{
    int16_t svel;
    int16_t fvel;
    fix16_t q16avel;
    fix16_t q16horz;
    fix16_t q16aimvel;	// only used by SW
    fix16_t q16ang;		// only used by SW
    ESyncBits actions;
	
    // Making this a union lets some constructs fail. Since these names are transitional only the added memory use doesn't really matter.
    // for Duke
    EDukeSyncBits sbits;

    // for SW
    int32_t bits;

    // for Blood
    SYNCFLAGS syncFlags;

    // For Exhumed
    uint16_t buttons;

    int getNewWeapon() const
    {
        return (actions & SB_WEAPONMASK_BITS).GetValue();
    }

    void setNewWeapon(int weap)
    {
        actions = (actions & ~SB_WEAPONMASK_BITS) | (ESyncBits::FromInt(weap) & SB_WEAPONMASK_BITS);
    }

    bool isItemUsed(int num)
    {
        return !!(actions & ESyncBits::FromInt(SB_ITEM_BIT_1 << num));
    }

    void setItemUsed(int num)
    {
        actions |= ESyncBits::FromInt(SB_ITEM_BIT_1 << num);
    }

    void clearItemUsed(int num)
    {
        actions &= ~ESyncBits::FromInt(SB_ITEM_BIT_1 << num);
    }

};


