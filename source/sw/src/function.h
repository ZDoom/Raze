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

// function.h

// file created by makehead.exe
// these headers contain default key assignments, as well as
// default button assignments and game function names
// axis defaults are also included


#ifndef function_public_
#define function_public_

#define NUMGAMEFUNCTIONS 51

extern const char *gamefunctions[];

enum
{
    gamefunc_Move_Forward,
    gamefunc_Move_Backward,
    gamefunc_Turn_Left,
    gamefunc_Turn_Right,
    gamefunc_Strafe,
    gamefunc_Fire,
    gamefunc_Open,
    gamefunc_Run,
    gamefunc_AutoRun,
    gamefunc_Jump,
    gamefunc_Crouch,
    gamefunc_Look_Up,
    gamefunc_Look_Down,
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
    gamefunc_Med_Kit,
    gamefunc_Smoke_Bomb,
    gamefunc_Night_Vision,
    gamefunc_Gas_Bomb,
    gamefunc_Flash_Bomb,
    gamefunc_Caltrops,
    gamefunc_TurnAround,
    gamefunc_SendMessage,
    gamefunc_Map,
    gamefunc_Shrink_Screen,
    gamefunc_Enlarge_Screen,
    gamefunc_Center_View,
    gamefunc_Holster_Weapon,
    gamefunc_Map_Follow_Mode,
    gamefunc_See_Co_Op_View,
    gamefunc_Mouse_Aiming,
    gamefunc_Toggle_Crosshair,
    gamefunc_Next_Weapon,
    gamefunc_Previous_Weapon,
    gamefunc_Show_Menu,
    gamefunc_Show_Console,
};
#endif
