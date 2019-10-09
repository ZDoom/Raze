//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

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

#ifdef DAMAGE_TABLE
#define DAMAGE_ENTRY(id, init_func, damage_lo, damage_hi, radius, max_ammo, min_ammo, with_weapon) \
    { init_func, damage_lo, damage_hi, radius, max_ammo, min_ammo, with_weapon, NULL, NULL, -1, -1 },
#define DAMAGE_ENTRY_WPN(id, init_func, damage_lo, damage_hi, radius, max_ammo, min_ammo, with_weapon, weapon_name, ammo_name, weapon_pickup, ammo_pickup ) \
    { init_func, damage_lo, damage_hi, radius, max_ammo, min_ammo, with_weapon, weapon_name, ammo_name, weapon_pickup, ammo_pickup },
#endif

#ifdef DAMAGE_ENUM
#define DAMAGE_ENTRY(id, init_func, damage_lo, damage_hi, radius, max_ammo, min_ammo, with_weapon) \
    id,
#define DAMAGE_ENTRY_WPN(id, init_func, damage_lo, damage_hi, radius, max_ammo, min_ammo, with_weapon, weapon_name, ammo_name, weapon_pickup, ammo_pickup ) \
    id,
#endif

// DAMAGES ////////////////////////////////////////////////////////////////////

// weapon
DAMAGE_ENTRY(WPN_FIST,            InitWeaponFist,       10,    40,   0,  -1, -1, -1)
DAMAGE_ENTRY_WPN(WPN_STAR,        InitWeaponStar,        5,    10,   0,  99,  3, -1, "Shurikens", NULL, 9, -1)
DAMAGE_ENTRY_WPN(WPN_SHOTGUN,     InitWeaponShotgun,     4,     4,   0,  52,  1, -1, "Riot Gun", "Shotshells", 8, 24)
DAMAGE_ENTRY_WPN(WPN_UZI,         InitWeaponUzi,         5,     7,   0, 200,  1, -1, "UZI Submachine Gun", "UZI Clip", 50, 50)
DAMAGE_ENTRY_WPN(WPN_MICRO,       InitWeaponMicro,      15,    30,   0,  50,  1, -1, "Missile Launcher", "Missiles", 5, 5)
DAMAGE_ENTRY_WPN(WPN_GRENADE,     InitWeaponGrenade,    15,    30,   0,  50,  1, -1, "Grenade Launcher", "Grenade Shells", 6, 8)
DAMAGE_ENTRY_WPN(WPN_MINE,        InitWeaponMine,        5,    10,   0,  20,  1, -1, "Sticky Bombs", NULL, 5, -1)
DAMAGE_ENTRY_WPN(WPN_RAIL,        InitWeaponRail,       40,    60,   0,  20,  1, -1, "Rail Gun", "Rail Gun Rods", 10, 10)
DAMAGE_ENTRY_WPN(WPN_HOTHEAD,     InitWeaponHothead,    10,    25,   0,  80,  1, -1, "Guardian Head", "Firebursts", 30, 60)
DAMAGE_ENTRY_WPN(WPN_HEART,       InitWeaponHeart,      75,   100,   0,   5,  1, -1, "Ripper Heart", "Deathcoils", 1, 6)

DAMAGE_ENTRY(WPN_NAPALM,          InitWeaponHothead,    50,   100,   0, 100,  40, WPN_HOTHEAD)
DAMAGE_ENTRY(WPN_RING,            InitWeaponHothead,    15,    50,   0, 100,  20, WPN_HOTHEAD)
DAMAGE_ENTRY(WPN_ROCKET,          InitWeaponMicro,      30,    60,   0, 100,   1, WPN_MICRO)
DAMAGE_ENTRY(WPN_SWORD,           InitWeaponSword,      50,    80,   0,  -1,  -1, -1)

// extra weapons connected to other

// spell
DAMAGE_ENTRY(DMG_NAPALM,          NULL,                90,   150, 0, -1, -1, -1)
DAMAGE_ENTRY(DMG_MIRV_METEOR,     NULL,                35,    65, 0, -1, -1, -1)
DAMAGE_ENTRY(DMG_SERP_METEOR,     NULL,                 7,    15, 0, -1, -1, -1)

// radius damage
DAMAGE_ENTRY(DMG_ELECTRO_SHARD,   NULL,                 2,     6,      0, -1, -1, -1)
DAMAGE_ENTRY(DMG_SECTOR_EXP,      NULL,                50,   100,   3200, -1, -1, -1)
DAMAGE_ENTRY(DMG_BOLT_EXP,        NULL,                80,   160,   3200, -1, -1, -1)
DAMAGE_ENTRY(DMG_TANK_SHELL_EXP,  NULL,                80,   200,   4500, -1, -1, -1)
DAMAGE_ENTRY(DMG_FIREBALL_EXP,    NULL,                -1,    -1,   1000, -1, -1, -1)
DAMAGE_ENTRY(DMG_NAPALM_EXP,      NULL,                60,    90,   3200, -1, -1, -1)
DAMAGE_ENTRY(DMG_SKULL_EXP,       NULL,                40,    75,   4500, -1, -1, -1)
DAMAGE_ENTRY(DMG_BASIC_EXP,       NULL,                10,    25,   1000, -1, -1, -1)
DAMAGE_ENTRY(DMG_GRENADE_EXP,     NULL,                70,   140,   6500, -1, -1, -1)
DAMAGE_ENTRY(DMG_MINE_EXP,        NULL,                85,   115,   6500, -1, -1, -1)
DAMAGE_ENTRY(DMG_MINE_SHRAP,      NULL,                15,    30,      0, -1, -1, -1)
DAMAGE_ENTRY(DMG_MICRO_EXP,       NULL,                50,   100,   4500, -1, -1, -1)
DAMAGE_ENTRY_WPN(DMG_NUCLEAR_EXP, NULL,                 0,   800,  30000, -1, -1, -1, "Nuclear Warhead", "Heat Seeker Card", 1, 5)
DAMAGE_ENTRY(DMG_RADIATION_CLOUD, NULL,                 2,     6,   5000, -1, -1, -1)
DAMAGE_ENTRY(DMG_FLASHBOMB,       NULL,               100,   150,  16384, -1, -1, -1)

DAMAGE_ENTRY(DMG_FIREBALL_FLAMES, NULL,                 2,     6,    300, -1, -1, -1)

// actor
DAMAGE_ENTRY(DMG_RIPPER_SLASH,    NULL,                10,    30,   0, -1, -1, -1)
DAMAGE_ENTRY(DMG_SKEL_SLASH,      NULL,                10,    20,   0, -1, -1, -1)
DAMAGE_ENTRY(DMG_COOLG_BASH,      NULL,                10,    20,   0, -1, -1, -1)
DAMAGE_ENTRY(DMG_COOLG_FIRE,      NULL,                15,    30,   0, -1, -1, -1)
DAMAGE_ENTRY(DMG_GORO_CHOP,       NULL,                20,    40,   0, -1, -1, -1)
DAMAGE_ENTRY(DMG_GORO_FIREBALL,   NULL,                5,     20,   0, -1, -1, -1)
DAMAGE_ENTRY(DMG_SERP_SLASH,      NULL,                75,    75,   0, -1, -1, -1)
DAMAGE_ENTRY(DMG_LAVA_BOULDER,    NULL,                100,  100,   0, -1, -1, -1)
DAMAGE_ENTRY(DMG_LAVA_SHARD,      NULL,                25,    25,   0, -1, -1, -1)
DAMAGE_ENTRY(DMG_HORNET_STING,    NULL,                5,     10,    0, -1, -1, -1)
DAMAGE_ENTRY(DMG_EEL_ELECTRO,     NULL,                10,    40, 3400, -1, -1, -1)

// misc
DAMAGE_ENTRY(DMG_SPEAR_TRAP,      NULL,                15,   20,    0, -1, -1, -1)
DAMAGE_ENTRY(DMG_VOMIT,           NULL,                 5,    15,   0, -1, -1, -1)

// inanimate objects
DAMAGE_ENTRY(DMG_BLADE,           NULL,                10,    20,   0, -1, -1, -1)
DAMAGE_ENTRY(MAX_WEAPONS,         NULL,                10,    20,   0, -1, -1, -1)

#undef DAMAGE_ENTRY
#undef DAMAGE_ENTRY_WPN
