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

    SB_INVPREV = 1 << 11,
    SB_INVNEXT = 1 << 12,
    SB_INVUSE = 1 << 13,
    SB_CENTERVIEW = 1 << 14,
    SB_TURNAROUND = 1 << 15,
    SB_HOLSTER = 1 << 16,
    SB_OPEN = 1 << 17,

    SB_RUN = 1 << 27,
    SB_JUMP = 1 << 28,
    SB_CROUCH = 1 << 29,
    SB_FIRE = 1 << 30,
    SB_ALTFIRE = 1u << 31,

    SB_WEAPONMASK_BITS = (15u * SB_FIRST_WEAPON_BIT), // Weapons take up 4 bits
    SB_ITEMUSE_BITS = (127u * SB_ITEM_BIT_1),

    SB_BUTTON_MASK = SB_ALTFIRE|SB_FIRE|SB_CROUCH|SB_JUMP,     // all input from buttons (i.e. active while held)
    SB_INTERFACE_MASK = (SB_INVPREV|SB_INVNEXT|SB_INVUSE|SB_CENTERVIEW|SB_TURNAROUND|SB_HOLSTER|SB_OPEN),  // all input from CCMDs
    SB_INTERFACE_BITS = (SB_WEAPONMASK_BITS | SB_ITEMUSE_BITS | SB_INTERFACE_MASK),
    SB_ALL = ~0u
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
	SKB_AIM_UP = 1 << 3,
	SKB_AIM_DOWN = 1 << 4,
	SKB_LOOK_LEFT = 1 << 6,
	SKB_LOOK_RIGHT = 1 << 7,
	SKB_LOOK_UP = 1 << 13,
	SKB_LOOK_DOWN = 1 << 14,
	SKB_QUICK_KICK = 1 << 22,
	SKB_AIMMODE = 1 << 23,
	SKB_ESCAPE = 1u << 31,

	SKB_INTERFACE_BITS = (SKB_QUICK_KICK | \
		SKB_ESCAPE),

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
        unsigned int _run : 1;
        unsigned int _jump : 1;
        unsigned int _crouch : 1;
        unsigned int _shoot : 1;
        unsigned int _shoot2 : 1;
        unsigned int lookUp : 1;
        unsigned int lookDown : 1;


        unsigned int jab : 1;
        unsigned int lookLeft : 1;
        unsigned int lookRight : 1;
    };
};

// SW

//
// NETWORK - REDEFINABLE SHARED (SYNC) KEYS BIT POSITIONS
//


#define SK_AUTO_AIM    7

#define SK_LOOK_UP    12
#define SK_LOOK_DOWN  13
#define SK_CRAWL_LOCK 14
#define SK_FLY        15

#define SK_AIM_UP    21
#define SK_AIM_DOWN  22

#define SK_SPACE_BAR  31


// Exhumed

enum {
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


