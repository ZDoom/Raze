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

// _functio.h

// file created by makehead.exe
// these headers contain default key assignments, as well as
// default button assignments and game function names
// axis defaults are also included


#ifndef function_private_
#define function_private_
const char *gamefunctions[] =
{
    "Move_Forward",
    "Move_Backward",
    "Turn_Left",
    "Turn_Right",
    "Strafe",
    "Fire",
    "Open",
    "Run",
    "AutoRun",
    "Jump",
    "Crouch",
    "Look_Up",
    "Look_Down",
    "Strafe_Left",
    "Strafe_Right",
    "Aim_Up",
    "Aim_Down",
    "Weapon_1",
    "Weapon_2",
    "Weapon_3",
    "Weapon_4",
    "Weapon_5",
    "Weapon_6",
    "Weapon_7",
    "Weapon_8",
    "Weapon_9",
    "Weapon_10",
    "Inventory",
    "Inventory_Left",
    "Inventory_Right",
    "Med_Kit",
    "Smoke_Bomb",
    "Night_Vision",
    "Gas_Bomb",
    "Flash_Bomb",
    "Caltrops",
    "TurnAround",
    "SendMessage",
    "Map",
    "Shrink_Screen",
    "Enlarge_Screen",
    "Center_View",
    "Holster_Weapon",
    "Map_Follow_Mode",
    "See_Co_Op_View",
    "Mouse_Aiming",
    "Toggle_Crosshair",
    "Next_Weapon",
    "Previous_Weapon",
    "Show_Menu",
    "Show_Console",
};

#define NUMKEYENTRIES 50

static const char *keydefaults[] =
{
    "Move_Forward", "Up", "Kpad8",
    "Move_Backward", "Down", "Kpad2",
    "Turn_Left", "Left", "Kpad4",
    "Turn_Right", "Right", "KPad6",
    "Strafe", "LAlt", "RAlt",
    "Fire", "LCtrl", "RCtrl",
    "Open", "Space", "",
    "Run", "LShift", "RShift",
    "AutoRun", "CapLck", "",
    "Jump", "A", "/",
    "Crouch", "Z", "",
    "Look_Up", "PgUp", "Kpad9",
    "Look_Down", "PgDn", "Kpad3",
    "Strafe_Left", ",", "",
    "Strafe_Right", ".", "",
    "Aim_Up", "Home", "KPad7",
    "Aim_Down", "End", "Kpad1",
    "Weapon_1", "1", "",
    "Weapon_2", "2", "",
    "Weapon_3", "3", "",
    "Weapon_4", "4", "",
    "Weapon_5", "5", "",
    "Weapon_6", "6", "",
    "Weapon_7", "7", "",
    "Weapon_8", "8", "",
    "Weapon_9", "9", "",
    "Weapon_10", "0", "",
    "Inventory", "Enter", "KpdEnt",
    "Inventory_Left", "[", "",
    "Inventory_Right", "]", "",
    "Med_Kit", "M", "",
    "Smoke_Bomb", "S", "",
    "Night_Vision", "N", "",
    "Gas_Bomb", "G", "",
    "Flash_Bomb", "F", "",
    "Caltrops", "C", "",
    "TurnAround", "BakSpc", "",
    "SendMessage", "T", "",
    "Map", "Tab", "",
    "Shrink_Screen", "-", "Kpad-",
    "Enlarge_Screen", "=", "Kpad+",
    "Center_View", "KPad5", "",
    "Holster_Weapon", "ScrLck", "",
    "Map_Follow_Mode", "F", "",
    "See_Co_Op_View", "K", "",
    "Mouse_Aiming", "U", "",
    "Toggle_Crosshair", "I", "",
    "Next_Weapon", "'", "",
    "Previous_Weapon", ";", "",
    "Show_Console", "NumLck", "",
};

static const char *keydefaults_modern[] =
{
    "Move_Forward", "W", "",
    "Move_Backward", "S", "",
    "Turn_Left", "", "",
    "Turn_Right", "", "",
    "Strafe", "", "",
    "Fire", "", "",
    "Open", "E", "",
    "Run", "LShift", "",
    "AutoRun", "CapLck", "",
    "Jump", "Space", "",
    "Crouch", "LAlt", "",
    "Look_Up", "", "",
    "Look_Down", "", "",
    "Strafe_Left", "A", "",
    "Strafe_Right", "D", "",
    "Aim_Up", "", "",
    "Aim_Down", "", "",
    "Weapon_1", "1", "",
    "Weapon_2", "2", "",
    "Weapon_3", "3", "",
    "Weapon_4", "4", "",
    "Weapon_5", "5", "",
    "Weapon_6", "6", "",
    "Weapon_7", "7", "",
    "Weapon_8", "8", "",
    "Weapon_9", "9", "",
    "Weapon_10", "0", "",
    "Inventory", "Enter", "",
    "Inventory_Left", "[", "",
    "Inventory_Right", "]", "",
    "Med_Kit", "M", "",
    "Smoke_Bomb", "B", "",
    "Night_Vision", "N", "",
    "Gas_Bomb", "G", "",
    "Flash_Bomb", "F", "",
    "Caltrops", "C", "",
    "TurnAround", "BakSpc", "",
    "SendMessage", "T", "",
    "Map", "Tab", "",
    "Shrink_Screen", "-", "",
    "Enlarge_Screen", "=", "",
    "Center_View", "", "",
    "Holster_Weapon", "H", "",
    "Map_Follow_Mode", "F", "",
    "See_Co_Op_View", "K", "",
    "Mouse_Aiming", "U", "",
    "Toggle_Crosshair", "I", "",
    "Next_Weapon", "", "",
    "Previous_Weapon", "", "",
    "Show_Console", "NumLck", "",
};


static const char *mousedefaults[] =
{
    "Fire",
    "Strafe",
    "Move_Forward",
    "",
    ""
};

static const char *mousedefaults_modern[] =
{
    "Fire",
    "Open",
    "",
    "",
    "Next_Weapon",
    "Previous_Weapon"
};


static const char *mouseclickeddefaults[] =
{
    "",
    "Open",
    "",
    "",
    ""
};

static const char *mouseclickeddefaults_modern[] =
{
    "",
    "",
    "",
    "",
    "",
    ""
};


static const char *joystickdefaults[] =
{
    "Fire",
    "Strafe",
    "Run",
    "Open",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "Aim_Down",
    "Look_Right",
    "Aim_Up",
    "Look_Left",
};


static const char *joystickclickeddefaults[] =
{
    "",
    "Inventory",
    "Jump",
    "Crouch",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};


static const char *mouseanalogdefaults[] =
{
    "analog_turning",
    "analog_moving",
};


static const char *mousedigitaldefaults[] =
{
    "",
    "",
    "",
    "",
};


#if 0
static const char *gamepaddigitaldefaults[] =
{
    "Turn_Left",
    "Turn_Right",
    "Move_Forward",
    "Move_Backward",
};
#endif


static const char *joystickanalogdefaults[] =
{
    "analog_turning",
    "analog_moving",
    "analog_strafing",
    "",
    "",
    "",
    "",
    "",
};


static const char *joystickdigitaldefaults[] =
{
    "",
    "",
    "",
    "",
    "",
    "",
    "Run",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};
#endif
