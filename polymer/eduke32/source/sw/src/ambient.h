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

#ifdef AMBIENT_TABLE
#define AMB_ENTRY(name, diginame, ambient_flags, maxtics) {name, diginame, ambient_flags, maxtics},
#endif

#ifdef AMBIENT_ENUM
#define AMB_ENTRY(name, diginame, ambient_flags, maxtics) name,
#endif

// Ambient Flags, flags can be added whenever needed, up to 16 bits
#define AMB_NONE        0
#define AMB_FOLLOW      1       // 1 = Do coordinate updates on sound
// Use this only if the sprite won't be deleted soon
#define AMB_DOPPLER     4       // 1 = Don't use doppler pitch variance

#define AMB_PAN         8       // 1 = Don't do panning of sound
#define AMB_INTERMIT    32      // 1 = This is a non-looping intermittant sound

// Tic counts used for intermittent sounds
#define AMB_TICRATE  12     // 120/10 since it's only called 10x per second
#define AMB_NOTICS   0
#define AMB_5        5 *AMB_TICRATE
#define AMB_10       10*AMB_TICRATE      // AMB_TICRATE is the game's tic rate
#define AMB_15       15*AMB_TICRATE
#define AMB_20       20*AMB_TICRATE
#define AMB_30       30*AMB_TICRATE
#define AMB_45       45*AMB_TICRATE
#define AMB_60       60*AMB_TICRATE
#define AMB_120      120*AMB_TICRATE


// BUBBLES
AMB_ENTRY(0,     DIGI_BUBBLES,           AMB_PAN,       AMB_NOTICS)

// CRICKETS
AMB_ENTRY(1,     DIGI_CRICKETS,          AMB_PAN,       AMB_NOTICS)

// AMBIENT WATER DRIPPING IN CAVE
AMB_ENTRY(2,     DIGI_CAVEDRIP1,         AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(3,     DIGI_CAVEDRIP2,         AMB_INTERMIT,  AMB_10)
AMB_ENTRY(4,     DIGI_DRIP,              AMB_PAN|AMB_INTERMIT,  AMB_20)

// WATER FALL
AMB_ENTRY(5,     DIGI_WATERFALL1,        AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(6,     DIGI_WATERFALL2,        AMB_NONE,      AMB_NOTICS)

// WATER FLOWING
AMB_ENTRY(7,     DIGI_WATERFLOW1,        AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(8,     DIGI_WATERFLOW2,        AMB_NONE,      AMB_NOTICS)

// CRACKLING FIRE FOR CONTINUOUS BURN
AMB_ENTRY(9,     DIGI_FIRE1,             AMB_NONE,      AMB_NOTICS)

// POWERFULL HIGH HEAT CONTINUOUS BURN
AMB_ENTRY(10,    DIGI_FIRE2,             AMB_NONE,      AMB_NOTICS)

// AMBIENT GONG FOR USE IN TEMPLE/PALACE LEVELS
AMB_ENTRY(11,    DIGI_GONG,              AMB_INTERMIT,  AMB_120)

// AMBIENT LAVA FLOW
AMB_ENTRY(12,    DIGI_LAVAFLOW1,         AMB_NONE,      AMB_NOTICS)

// AMBIENT MUD BUBBLES
AMB_ENTRY(13,    DIGI_MUBBUBBLES1,       AMB_NONE,      AMB_NOTICS)

// AMBIENT EARTH QUAKE
AMB_ENTRY(14,    DIGI_EARTHQUAKE,        AMB_NONE,      AMB_NOTICS)

// YUCKY SEWER FLOW
AMB_ENTRY(15,    DIGI_SEWERFLOW1,        AMB_NONE,      AMB_NOTICS)

// STEAM FLOW
AMB_ENTRY(16,    DIGI_STEAM1,            AMB_NONE,      AMB_NOTICS)

// VOLCANIC STEAM VENT
AMB_ENTRY(17,    DIGI_VOLCANOSTEAM1,     AMB_NONE,      AMB_NOTICS)

// SCARY AMBIENT SWAMP SOUNDS
AMB_ENTRY(18,    DIGI_SWAMP,             AMB_NONE,      AMB_NOTICS)

// AMBIENT ROLLING THUNDER
AMB_ENTRY(19,    DIGI_THUNDER,           AMB_PAN|AMB_INTERMIT,  AMB_60)

// UNDERWATER AMBIENCE
AMB_ENTRY(20,    DIGI_UNDERWATER,        AMB_PAN,       AMB_NOTICS)

// SPOOKY ETHERAL void AMBIENCE (NETHERWORLDLY SOUNDS)
AMB_ENTRY(21,    DIGI_VOID1,             AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(22,    DIGI_VOID2,             AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(23,    DIGI_VOID3,             AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(24,    DIGI_VOID4,             AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(25,    DIGI_VOID5,             AMB_NONE,      AMB_NOTICS)

// VOLCANIC ERUPTION
AMB_ENTRY(26,    DIGI_ERUPTION,          AMB_NONE,      AMB_NOTICS)

// VOLCANIC SIZZLING PROJECTILES FLYING THROUGH AIR
AMB_ENTRY(27,    DIGI_VOLCANOPROJECTILE, AMB_NONE,      AMB_NOTICS)

// LIGHT WIND AMBIENCE
AMB_ENTRY(28,    DIGI_LIGHTWIND,         AMB_NONE,      AMB_NOTICS)

// STRONG BLOWING WIND AMBIENCE
AMB_ENTRY(29,    DIGI_STRONGWIND,        AMB_PAN|AMB_INTERMIT,  AMB_20)

// BREAKING WOOD AMBIENCE
AMB_ENTRY(30,    DIGI_BREAKINGWOOD,      AMB_INTERMIT,  AMB_120)

// BREAKING, TUMBLING STONES FALLING AMBIENCE
AMB_ENTRY(31,    DIGI_BREAKSTONES,       AMB_NONE,      AMB_NOTICS)

// MOTOR BOAT
AMB_ENTRY(32,    DIGI_NULL,              AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(33,    DIGI_NULL,              AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(34,    DIGI_NULL,              AMB_NONE,      AMB_NOTICS)

// WWII JAP ARMY TANK
AMB_ENTRY(35,    DIGI_NULL,              AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(36,    DIGI_NULL,              AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(37,    DIGI_NULL,              AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(38,    DIGI_NULL,              AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(39,    DIGI_NULL,              AMB_NONE,      AMB_NOTICS)

// WWII JAP BOMBER PLANE
AMB_ENTRY(40,    DIGI_BOMBRFLYING,       AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(41,    DIGI_BOMBRDROPBOMB,     AMB_NONE,      AMB_NOTICS)

// GIANT DRILL MACHINE
AMB_ENTRY(42,   DIGI_DRILL,              AMB_NONE,      AMB_NOTICS)

// SECTOR GEAR COG TURNING
AMB_ENTRY(43,   DIGI_GEAR1,              AMB_NONE,      AMB_NOTICS)

// GENERIC SECTOR OBJECT MACHINE RUNNING
AMB_ENTRY(44,   DIGI_MACHINE1,           AMB_NONE,      AMB_NOTICS)

// ENGINE ROOM
AMB_ENTRY(45,   DIGI_ENGROOM1,           AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(46,   DIGI_ENGROOM2,           AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(47,   DIGI_ENGROOM3,           AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(48,   DIGI_ENGROOM4,           AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(49,   DIGI_ENGROOM5,           AMB_NONE,      AMB_NOTICS)

// HELICOPTER, SPINNING BLADE SOUND
AMB_ENTRY(50,   DIGI_HELI,               AMB_NONE,      AMB_NOTICS)

// ECHOING HEART
AMB_ENTRY(51,   DIGI_BIGHART,            AMB_NONE,      AMB_NOTICS)

// ETHERAL WIND
AMB_ENTRY(52,   DIGI_WIND4,              AMB_NONE,      AMB_NOTICS)

// SPOOKY SINE WAVE
AMB_ENTRY(53,   DIGI_SPOOKY1,            AMB_NONE,      AMB_NOTICS)

// JET ENGINE
AMB_ENTRY(54,   DIGI_JET,                AMB_NONE,      AMB_NOTICS)

// CEREMONIAL DRUM CHANT
AMB_ENTRY(55,   DIGI_DRUMCHANT,          AMB_NONE,      AMB_NOTICS)

AMB_ENTRY(56,   DIGI_ASIREN1,           AMB_INTERMIT,  AMB_45)
AMB_ENTRY(57,   DIGI_FIRETRK1,          AMB_INTERMIT,  AMB_60)
AMB_ENTRY(58,   DIGI_TRAFFIC1,          AMB_INTERMIT,  AMB_60)
AMB_ENTRY(59,   DIGI_TRAFFIC2,          AMB_INTERMIT,  AMB_60)
AMB_ENTRY(60,   DIGI_TRAFFIC3,          AMB_INTERMIT,  AMB_60)
AMB_ENTRY(61,   DIGI_TRAFFIC4,          AMB_INTERMIT,  AMB_60)
AMB_ENTRY(62,   DIGI_TRAFFIC5,          AMB_INTERMIT,  AMB_30)
AMB_ENTRY(63,   DIGI_TRAFFIC6,          AMB_INTERMIT,  AMB_60)
AMB_ENTRY(64,   DIGI_HELI1,             AMB_INTERMIT,  AMB_120)
AMB_ENTRY(65,   DIGI_JET1,              AMB_INTERMIT,  AMB_120)
AMB_ENTRY(66,   DIGI_MOTO1,             AMB_INTERMIT,  AMB_45)
AMB_ENTRY(67,   DIGI_MOTO2,             AMB_INTERMIT,  AMB_60)
AMB_ENTRY(68,   DIGI_NEON1,             AMB_INTERMIT,  AMB_5)
AMB_ENTRY(69,   DIGI_SUBWAY,            AMB_INTERMIT,  AMB_30)
AMB_ENTRY(70,   DIGI_TRAIN1,            AMB_INTERMIT,  AMB_120)
AMB_ENTRY(71,   DIGI_BIRDS1,            AMB_INTERMIT,  AMB_10)
AMB_ENTRY(72,   DIGI_BIRDS2,            AMB_INTERMIT,  AMB_10)
AMB_ENTRY(73,   DIGI_AMOEBA,            AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(74,   DIGI_TRAIN3,            AMB_INTERMIT,  AMB_120)
AMB_ENTRY(75,   DIGI_TRAIN8,            AMB_INTERMIT,  AMB_120)
AMB_ENTRY(76,   DIGI_WHIPME,            AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(77,   DIGI_FLAGWAVE,          AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(78,   DIGI_ANIMECRY,          AMB_NONE,      AMB_NOTICS)
AMB_ENTRY(79,   DIGI_WINDCHIMES,        AMB_INTERMIT,  AMB_10)
AMB_ENTRY(80,   DIGI_BOATCREAK,         AMB_INTERMIT,  AMB_10)
AMB_ENTRY(81,   DIGI_SHIPBELL,          AMB_INTERMIT,  AMB_30)
AMB_ENTRY(82,   DIGI_FOGHORN,           AMB_INTERMIT,  AMB_120)

#undef AMB_ENTRY
