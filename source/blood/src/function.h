//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

// function.h

// file created by makehead.exe
// these headers contain default key assignments, as well as
// default button assignments and game function names
// axis defaults are also included


#ifndef function_public_h_
#define function_public_h_
#ifdef __cplusplus
extern "C" {
#endif

#define NUMGAMEFUNCTIONS 55
#define MAXGAMEFUNCLEN 32

extern char gamefunctions[NUMGAMEFUNCTIONS][MAXGAMEFUNCLEN];
extern const char keydefaults[NUMGAMEFUNCTIONS*2][MAXGAMEFUNCLEN];
extern const char oldkeydefaults[NUMGAMEFUNCTIONS*2][MAXGAMEFUNCLEN];

enum GameFunction_t
   {
   gamefunc_Move_Forward,
   gamefunc_Move_Backward,
   gamefunc_Turn_Left,
   gamefunc_Turn_Right,
   gamefunc_Turn_Around,
   gamefunc_Strafe,
   gamefunc_Strafe_Left,
   gamefunc_Strafe_Right,
   gamefunc_Jump,
   gamefunc_Crouch,
   gamefunc_Run,
   gamefunc_AutoRun,
   gamefunc_Open,
   gamefunc_Weapon_Fire,
   gamefunc_Weapon_Special_Fire,
   gamefunc_Aim_Up,
   gamefunc_Aim_Down,
   gamefunc_Aim_Center,
   gamefunc_Look_Up,
   gamefunc_Look_Down,
   gamefunc_Tilt_Left,
   gamefunc_Tilt_Right,
   gamefunc_Weapon_1,
   gamefunc_Weapon_2,
   gamefunc_Weapon_3,
   gamefunc_Weapon_4,
   gamefunc_Weapon_5,
   gamefunc_Weapon_6,
   gamefunc_Weapon_7,
   gamefunc_Weapon_8,
   gamefunc_Weapon_9,
   gamefunc_Weapon_10,
   gamefunc_Inventory_Use,
   gamefunc_Inventory_Left,
   gamefunc_Inventory_Right,
   gamefunc_Map_Toggle,
   gamefunc_Map_Follow_Mode,
   gamefunc_Shrink_Screen,
   gamefunc_Enlarge_Screen,
   gamefunc_Send_Message,
   gamefunc_See_Coop_View,
   gamefunc_See_Chase_View,
   gamefunc_Mouse_Aiming,
   gamefunc_Toggle_Crosshair,
   gamefunc_Next_Weapon,
   gamefunc_Previous_Weapon,
   gamefunc_Holster_Weapon,
   gamefunc_Show_Opponents_Weapon,
   gamefunc_BeastVision,
   gamefunc_CrystalBall,
   gamefunc_JumpBoots,
   gamefunc_MedKit,
   gamefunc_ProximityBombs,
   gamefunc_RemoteBombs,
   gamefunc_Show_Console,
   };
#ifdef __cplusplus
}
#endif
#endif
