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

#include "names.h"
#include "names2.h"
//#include "panel.h"

#ifdef MAKE_CONPIC_ENUM
#define CONPIC_ENTRY(tile,name) ID_ ## name = tile,
#endif

////////////////////////////////////////////////////////////////////////////////
//
// SWORD
//
////////////////////////////////////////////////////////////////////////////////

#define SWORD_REST 2080
#define SWORD_SWING0 2081
#define SWORD_SWING1 2082
#define SWORD_SWING2 2083

CONPIC_ENTRY(SWORD_REST+0,SwordPresent0)

CONPIC_ENTRY(SWORD_SWING0,SwordSwing0)
CONPIC_ENTRY(SWORD_SWING1,SwordSwing1)
CONPIC_ENTRY(SWORD_SWING2,SwordSwing2)

CONPIC_ENTRY(SWORD_SWING0,SwordSwingR0)
CONPIC_ENTRY(SWORD_SWING1,SwordSwingR1)
CONPIC_ENTRY(SWORD_SWING2,SwordSwingR2)

#define BLOODYSWORD_REST 4090
#define BLOODYSWORD_SWING0 4091
#define BLOODYSWORD_SWING1 4092
#define BLOODYSWORD_SWING2 4093

CONPIC_ENTRY(BLOODYSWORD_REST,BloodySwordPresent0)

CONPIC_ENTRY(BLOODYSWORD_SWING0,BloodySwordSwing0)
CONPIC_ENTRY(BLOODYSWORD_SWING1,BloodySwordSwing1)
CONPIC_ENTRY(BLOODYSWORD_SWING2,BloodySwordSwing2)

CONPIC_ENTRY(BLOODYSWORD_SWING0,BloodySwordSwingR0)
CONPIC_ENTRY(BLOODYSWORD_SWING1,BloodySwordSwingR1)
CONPIC_ENTRY(BLOODYSWORD_SWING2,BloodySwordSwingR2)

////////////////////////////////////////////////////////////////////////////////
//
// FIST
//
////////////////////////////////////////////////////////////////////////////////

#define FIST_REST 4070
#define FIST_SWING0 4071
#define FIST_SWING1 4072
#define FIST_SWING2 4073

CONPIC_ENTRY(FIST_REST+0,FistPresent0)

CONPIC_ENTRY(FIST_SWING0,FistSwing0)
CONPIC_ENTRY(FIST_SWING1,FistSwing1)
CONPIC_ENTRY(FIST_SWING2,FistSwing2)

#define BLOODYFIST_REST 4074
#define BLOODYFIST_SWING0 4075
#define BLOODYFIST_SWING1 4076
#define BLOODYFIST_SWING2 4077

#define FIST2_REST 4050
#define FIST2_SWING0 4051
#define FIST2_SWING1 4052
#define FIST2_SWING2 4053

CONPIC_ENTRY(FIST2_REST+0,Fist2Present0)

CONPIC_ENTRY(FIST2_SWING0,Fist2Swing0)
CONPIC_ENTRY(FIST2_SWING1,Fist2Swing1)
CONPIC_ENTRY(FIST2_SWING2,Fist2Swing2)

#define BLOODYFIST2_REST 4054
#define BLOODYFIST2_SWING0 4055
#define BLOODYFIST2_SWING1 4056
#define BLOODYFIST2_SWING2 4057

#define FIST3_REST 4060
#define FIST3_SWING0 4061
#define FIST3_SWING1 4062
#define FIST3_SWING2 4063

CONPIC_ENTRY(FIST3_REST+0,Fist3Present0)

CONPIC_ENTRY(FIST3_SWING0,Fist3Swing0)
CONPIC_ENTRY(FIST3_SWING1,Fist3Swing1)
CONPIC_ENTRY(FIST3_SWING2,Fist3Swing2)

#define BLOODYFIST3_REST 4064
#define BLOODYFIST3_SWING0 4065
#define BLOODYFIST3_SWING1 4066
#define BLOODYFIST3_SWING2 4067

////////////////////////////////////////////////////////////////////////////////
//
// KICK
//
////////////////////////////////////////////////////////////////////////////////

#define KICK0 4080
#define KICK1 4081

CONPIC_ENTRY(KICK0,Kick0)
CONPIC_ENTRY(KICK1,Kick1)

#define BLOODYKICK0 4082
#define BLOODYKICK1 4083


///////////////////////////////////////////////////////////////////////////////
//
// SHRUIKEN
//
///////////////////////////////////////////////////////////////////////////////

#define STAR_REST 2130
#define STAR_THROW 2133

CONPIC_ENTRY(STAR_REST+0,StarPresent0)

CONPIC_ENTRY(STAR_REST+1,StarDown0)
CONPIC_ENTRY(STAR_REST+2,StarDown1)

CONPIC_ENTRY(STAR_THROW+0,ThrowStar0)
CONPIC_ENTRY(STAR_THROW+1,ThrowStar1)
CONPIC_ENTRY(STAR_THROW+2,ThrowStar2)
CONPIC_ENTRY(STAR_THROW+3,ThrowStar3)
CONPIC_ENTRY(STAR_THROW+4,ThrowStar4)

///////////////////////////////////////////////////////////////////////////////////////
//
// UZI
//
///////////////////////////////////////////////////////////////////////////////////////

#if 0
#define UZI_REST 2000
#define UZI_FIRE_0 2001
#define UZI_FIRE_1 2001
#define UZI_EJECT 2003
#define UZI_CLIP 2005
#define UZI_RELOAD 2007
#else
// silencer
#define UZI_REST 2004
#define UZI_FIRE_0 2006
#define UZI_FIRE_1 2008
#define UZI_EJECT 2009
#define UZI_CLIP 2005
#define UZI_RELOAD 2007
#endif

#define UZI_SHELL 2152


// RIGHT UZI
CONPIC_ENTRY(UZI_REST,UziPresent0)

CONPIC_ENTRY(UZI_FIRE_0,UziFire0)
CONPIC_ENTRY(UZI_FIRE_1,UziFire1)

// LEFT UZI
CONPIC_ENTRY(UZI_REST,Uzi2Present0)

CONPIC_ENTRY(UZI_FIRE_0,Uzi2Fire0)
CONPIC_ENTRY(UZI_FIRE_1,Uzi2Fire1)
//eject
CONPIC_ENTRY(UZI_EJECT,UziEject0)

//clip
CONPIC_ENTRY(UZI_CLIP,UziClip0)

//reload
CONPIC_ENTRY(UZI_RELOAD,UziReload0)

CONPIC_ENTRY(UZI_SHELL+0,UziShell0)
CONPIC_ENTRY(UZI_SHELL+1,UziShell1)
CONPIC_ENTRY(UZI_SHELL+2,UziShell2)
CONPIC_ENTRY(UZI_SHELL+3,UziShell3)
CONPIC_ENTRY(UZI_SHELL+4,UziShell4)
CONPIC_ENTRY(UZI_SHELL+5,UziShell5)

CONPIC_ENTRY(UZI_SHELL+0,Uzi2Shell0)
CONPIC_ENTRY(UZI_SHELL+1,Uzi2Shell1)
CONPIC_ENTRY(UZI_SHELL+2,Uzi2Shell2)
CONPIC_ENTRY(UZI_SHELL+3,Uzi2Shell3)
CONPIC_ENTRY(UZI_SHELL+4,Uzi2Shell4)
CONPIC_ENTRY(UZI_SHELL+5,Uzi2Shell5)

////////////////////////////////////////////////////////////////////////////////
//
// SHOTGUN
//
////////////////////////////////////////////////////////////////////////////////

#define SHOTGUN_REST 2213
#define SHOTGUN_FIRE 2214
#define SHOTGUN_RELOAD0 2216
#define SHOTGUN_RELOAD1 2211
#define SHOTGUN_RELOAD2 2212

CONPIC_ENTRY(SHOTGUN_REST,ShotgunPresent0)

CONPIC_ENTRY(SHOTGUN_REST,ShotgunRest0)

CONPIC_ENTRY(SHOTGUN_RELOAD0,ShotgunReload0)
CONPIC_ENTRY(SHOTGUN_RELOAD1,ShotgunReload1)
CONPIC_ENTRY(SHOTGUN_RELOAD2,ShotgunReload2)

CONPIC_ENTRY(SHOTGUN_FIRE+0,ShotgunFire0)
CONPIC_ENTRY(SHOTGUN_FIRE+1,ShotgunFire1)

#define SHOTGUN_SHELL 2180
CONPIC_ENTRY(SHOTGUN_SHELL+0,ShotgunShell0)
CONPIC_ENTRY(SHOTGUN_SHELL+1,ShotgunShell1)
CONPIC_ENTRY(SHOTGUN_SHELL+2,ShotgunShell2)
CONPIC_ENTRY(SHOTGUN_SHELL+3,ShotgunShell3)
CONPIC_ENTRY(SHOTGUN_SHELL+4,ShotgunShell4)
CONPIC_ENTRY(SHOTGUN_SHELL+5,ShotgunShell5)
CONPIC_ENTRY(SHOTGUN_SHELL+6,ShotgunShell6)
CONPIC_ENTRY(SHOTGUN_SHELL+7,ShotgunShell7)

////////////////////////////////////////////////////////////////////////////////
//
// ROCKET
//
////////////////////////////////////////////////////////////////////////////////

#define ROCKET_REST 2211
#define ROCKET_FIRE 2212

CONPIC_ENTRY(ROCKET_REST+0,RocketPresent0)
CONPIC_ENTRY(ROCKET_REST+0,RocketRest0)

CONPIC_ENTRY(ROCKET_FIRE+0,RocketFire0)
CONPIC_ENTRY(ROCKET_FIRE+1,RocketFire1)
CONPIC_ENTRY(ROCKET_FIRE+2,RocketFire2)
CONPIC_ENTRY(ROCKET_FIRE+3,RocketFire3)
CONPIC_ENTRY(ROCKET_FIRE+4,RocketFire4)
CONPIC_ENTRY(ROCKET_FIRE+5,RocketFire5)

////////////////////////////////////////////////////////////////////////////////
//
// RAIL
//
////////////////////////////////////////////////////////////////////////////////

#define RAIL_REST 2010
#define RAIL_CHARGE 2015
#define RAIL_FIRE 2018

CONPIC_ENTRY(RAIL_REST+0,RailPresent0)

CONPIC_ENTRY(RAIL_REST+0,RailRest0)
CONPIC_ENTRY(RAIL_REST+1,RailRest1)
CONPIC_ENTRY(RAIL_REST+2,RailRest2)
CONPIC_ENTRY(RAIL_REST+3,RailRest3)
CONPIC_ENTRY(RAIL_REST+4,RailRest4)

CONPIC_ENTRY(RAIL_FIRE+0,RailFire0)
CONPIC_ENTRY(RAIL_FIRE+1,RailFire1)

CONPIC_ENTRY(RAIL_CHARGE+0,RailCharge0)
CONPIC_ENTRY(RAIL_CHARGE+1,RailCharge1)
CONPIC_ENTRY(RAIL_CHARGE+2,RailCharge2)

////////////////////////////////////////////////////////////////////////////////
//
// FIREBALL
//
////////////////////////////////////////////////////////////////////////////////

//#define HOTHEAD_REST 2327
//#define HOTHEAD_ATTACK 2327
#define HOTHEAD_REST 2048
#define HOTHEAD_ATTACK 2049
#define HOTHEAD_CENTER 2327
#define HOTHEAD_TURN 2314
#define HOTHEAD_CHOMP 2318

CONPIC_ENTRY(HOTHEAD_REST+0,HotheadPresent0)
CONPIC_ENTRY(HOTHEAD_REST+0,HotheadRest0)
CONPIC_ENTRY(HOTHEAD_ATTACK+0,HotheadAttack0)

CONPIC_ENTRY(HOTHEAD_CENTER+0,HotheadCenter0)

CONPIC_ENTRY(HOTHEAD_TURN+0,HotheadTurn0)
CONPIC_ENTRY(HOTHEAD_TURN+1,HotheadTurn1)
CONPIC_ENTRY(HOTHEAD_TURN+2,HotheadTurn2)
CONPIC_ENTRY(HOTHEAD_TURN+3,HotheadTurn3)

CONPIC_ENTRY(HOTHEAD_CHOMP+0,HotheadChomp0)

#define ON_FIRE 3157

CONPIC_ENTRY(ON_FIRE+0,OnFire0)
CONPIC_ENTRY(ON_FIRE+1,OnFire1)
CONPIC_ENTRY(ON_FIRE+2,OnFire2)
CONPIC_ENTRY(ON_FIRE+3,OnFire3)
CONPIC_ENTRY(ON_FIRE+4,OnFire4)
CONPIC_ENTRY(ON_FIRE+5,OnFire5)
CONPIC_ENTRY(ON_FIRE+6,OnFire6)
CONPIC_ENTRY(ON_FIRE+7,OnFire7)
CONPIC_ENTRY(ON_FIRE+8,OnFire8)
CONPIC_ENTRY(ON_FIRE+9,OnFire9)
CONPIC_ENTRY(ON_FIRE+10,OnFire10)
CONPIC_ENTRY(ON_FIRE+11,OnFire11)
CONPIC_ENTRY(ON_FIRE+12,OnFire12)

////////////////////////////////////////////////////////////////////////////////
//
// MICRO
//
////////////////////////////////////////////////////////////////////////////////

#define MICRO_REST 2070
#define MICRO_FIRE 2071
#define MICRO_SINGLE_FIRE 2071

CONPIC_ENTRY(MICRO_REST+0,MicroPresent0)

CONPIC_ENTRY(MICRO_FIRE+0,MicroFire0)
CONPIC_ENTRY(MICRO_FIRE+1,MicroFire1)
CONPIC_ENTRY(MICRO_FIRE+2,MicroFire2)
CONPIC_ENTRY(MICRO_FIRE+3,MicroFire3)

CONPIC_ENTRY(MICRO_SINGLE_FIRE+0,MicroSingleFire0)
CONPIC_ENTRY(MICRO_SINGLE_FIRE+1,MicroSingleFire1)
CONPIC_ENTRY(MICRO_SINGLE_FIRE+2,MicroSingleFire2)
CONPIC_ENTRY(MICRO_SINGLE_FIRE+3,MicroSingleFire3)

#if 0
////////////////////////////////////////////////////////////////////////////////
//
// NAPALM
//
////////////////////////////////////////////////////////////////////////////////

#define NAPALM_REST 2020
#define NAPALM_FIRE 2021

CONPIC_ENTRY(NAPALM_REST+0,NapalmPresent0)

CONPIC_ENTRY(NAPALM_FIRE+0,NapalmFire0)
CONPIC_ENTRY(NAPALM_FIRE+1,NapalmFire1)
CONPIC_ENTRY(NAPALM_FIRE+2,NapalmFire2)
CONPIC_ENTRY(NAPALM_FIRE+3,NapalmFire3)

////////////////////////////////////////////////////////////////////////////////
//
// RING
//
////////////////////////////////////////////////////////////////////////////////

#define RING_REST 2020
#define RING_FIRE 2021

CONPIC_ENTRY(RING_REST+0,RingPresent0)

CONPIC_ENTRY(RING_FIRE+0,RingFire0)
CONPIC_ENTRY(RING_FIRE+1,RingFire1)
CONPIC_ENTRY(RING_FIRE+2,RingFire2)
CONPIC_ENTRY(RING_FIRE+3,RingFire3)


////////////////////////////////////////////////////////////////////////////////
//
// ELECTRO
//
////////////////////////////////////////////////////////////////////////////////

#define ELECTRO_REST 2020
#define ELECTRO_FIRE 2021

CONPIC_ENTRY(ELECTRO_REST+0,ElectroPresent0)

CONPIC_ENTRY(ELECTRO_FIRE+0,ElectroFire0)
CONPIC_ENTRY(ELECTRO_FIRE+1,ElectroFire1)
CONPIC_ENTRY(ELECTRO_FIRE+2,ElectroFire2)
CONPIC_ENTRY(ELECTRO_FIRE+3,ElectroFire3)
#endif

////////////////////////////////////////////////////////////////////////////////
//
// GRENADE
//
////////////////////////////////////////////////////////////////////////////////

#define GRENADE_REST 2121
#define GRENADE_FIRE 2122
#define GRENADE_RELOAD 2125

CONPIC_ENTRY(GRENADE_REST+0,GrenadePresent0)

CONPIC_ENTRY(GRENADE_FIRE+0,GrenadeFire0)
CONPIC_ENTRY(GRENADE_FIRE+1,GrenadeFire1)
CONPIC_ENTRY(GRENADE_FIRE+2,GrenadeFire2)

CONPIC_ENTRY(GRENADE_RELOAD+0,GrenadeReload0)
CONPIC_ENTRY(GRENADE_RELOAD+1,GrenadeReload1)

////////////////////////////////////////////////////////////////////////////////
//
// MINE
//
////////////////////////////////////////////////////////////////////////////////

#define MINE_REST 2220
#define MINE_THROW 2222
#define MINE_RELOAD 2222

CONPIC_ENTRY(MINE_REST+0,MinePresent0)
CONPIC_ENTRY(MINE_REST+1,MinePresent1)

CONPIC_ENTRY(MINE_THROW+0,MineThrow0)

CONPIC_ENTRY(MINE_RELOAD+0,MineReload0)

////////////////////////////////////////////////////////////////////////////////
//
// HEART
//
////////////////////////////////////////////////////////////////////////////////

#define HEART_REST 2050

#define HEART_ATTACK 2052

CONPIC_ENTRY(HEART_REST+0,HeartPresent0)
CONPIC_ENTRY(HEART_REST+1,HeartPresent1)

CONPIC_ENTRY(HEART_ATTACK+0,HeartAttack0)
CONPIC_ENTRY(HEART_ATTACK+1,HeartAttack1)

//#define HEART_BLOOD 2430
#define HEART_BLOOD 2420
CONPIC_ENTRY(HEART_BLOOD+0,HeartBlood0)
CONPIC_ENTRY(HEART_BLOOD+1,HeartBlood1)
CONPIC_ENTRY(HEART_BLOOD+2,HeartBlood2)
CONPIC_ENTRY(HEART_BLOOD+3,HeartBlood3)
CONPIC_ENTRY(HEART_BLOOD+4,HeartBlood4)
CONPIC_ENTRY(HEART_BLOOD+5,HeartBlood5)

#undef CONPIC_ENTRY




