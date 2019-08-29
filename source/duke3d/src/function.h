//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
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

#define NUMGAMEFUNCTIONS 63
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
   gamefunc_Inventory,
   gamefunc_Inventory_Left,
   gamefunc_Inventory_Right,
   gamefunc_Holo_Duke,
   gamefunc_Jetpack,
   gamefunc_NightVision,
   gamefunc_MedKit,
   gamefunc_TurnAround,
   gamefunc_SendMessage,
   gamefunc_Map,
   gamefunc_Shrink_Screen,
   gamefunc_Enlarge_Screen,
   gamefunc_Center_View,
   gamefunc_Holster_Weapon,
   gamefunc_Show_Opponents_Weapon,
   gamefunc_Map_Follow_Mode,
   gamefunc_See_Coop_View,
   gamefunc_Mouse_Aiming,
   gamefunc_Toggle_Crosshair,
   gamefunc_Steroids,
   gamefunc_Quick_Kick,
   gamefunc_Next_Weapon,
   gamefunc_Previous_Weapon,
   gamefunc_Show_Console,
   gamefunc_Show_DukeMatch_Scores,
   gamefunc_Dpad_Select,
   gamefunc_Dpad_Aiming,
   gamefunc_AutoRun,
   gamefunc_Last_Weapon,
   gamefunc_Quick_Save,
   gamefunc_Quick_Load,
   gamefunc_Alt_Weapon,
   gamefunc_Third_Person_View,
   gamefunc_Toggle_Crouch,
   };
#ifdef __cplusplus
}
#endif
#endif
