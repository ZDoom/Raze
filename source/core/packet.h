#pragma once

#include <stdint.h>
#include "m_fixed.h"
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

    SB_AIMMODE = 1 << 18,   
    SB_QUICK_KICK = 1 << 19,
    SB_ESCAPE = 1 << 20,

    SB_AIM_UP = 1 << 21,
    SB_AIM_DOWN = 1 << 22,
    SB_LOOK_LEFT = 1 << 23,
    SB_LOOK_RIGHT = 1 << 24,
    SB_LOOK_UP = 1 << 25,
    SB_LOOK_DOWN = 1 << 26,
    SB_RUN = 1 << 27,
    SB_JUMP = 1 << 28,
    SB_CROUCH = 1 << 29,
    SB_FIRE = 1 << 30,
    SB_ALTFIRE = 1u << 31,

    SB_WEAPONMASK_BITS = (15u * SB_FIRST_WEAPON_BIT), // Weapons take up 4 bits
    SB_ITEMUSE_BITS = (127u * SB_ITEM_BIT_1),

    SB_BUTTON_MASK = SB_ALTFIRE|SB_FIRE|SB_CROUCH|SB_JUMP|SB_LOOK_UP|SB_LOOK_DOWN|SB_AIM_UP|SB_AIM_DOWN|SB_LOOK_LEFT|SB_LOOK_RIGHT,     // all input from buttons (i.e. active while held)
    SB_INTERFACE_MASK = (SB_INVPREV|SB_INVNEXT|SB_INVUSE|SB_CENTERVIEW|SB_TURNAROUND|SB_HOLSTER|SB_OPEN|SB_ESCAPE|SB_QUICK_KICK),  // all input from CCMDs
    SB_INTERFACE_BITS = (SB_WEAPONMASK_BITS | SB_ITEMUSE_BITS | SB_INTERFACE_MASK),
    SB_ALL = ~0u
};

// enforce type safe operations on the input bits.
using ESyncBits = TFlags<ESyncBits_, uint32_t>;
DEFINE_TFLAGS_OPERATORS(ESyncBits)


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

struct InputPacket
{
    int16_t svel;
    int16_t fvel;
    float avel;
    float horz;
    ESyncBits actions;


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


