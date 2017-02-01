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

// _functio.h

// file created by makehead.exe
// these headers contain default key assignments, as well as
// default button assignments and game function names
// axis defaults are also included

#include "_control.h"
#include "control.h"

#ifndef function_private_h_
#define function_private_h_
#ifdef __cplusplus
extern "C" {
#endif
// KEEPINSYNC lunatic/con_lang.lua
char gamefunctions[NUMGAMEFUNCTIONS][MAXGAMEFUNCLEN] =
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
   "Look_Left",
   "Look_Right",
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
   "Holo_Duke",
   "Jetpack",
   "NightVision",
   "MedKit",
   "TurnAround",
   "SendMessage",
   "Map",
   "Shrink_Screen",
   "Enlarge_Screen",
   "Center_View",
   "Holster_Weapon",
   "Show_Opponents_Weapon",
   "Map_Follow_Mode",
   "See_Coop_View",
   "Mouse_Aiming",
   "Toggle_Crosshair",
   "Steroids",
   "Quick_Kick",
   "Next_Weapon",
   "Previous_Weapon",
   "Show_Console",
   "Show_DukeMatch_Scores",
   "Dpad_Select",
   "Dpad_Aiming"
   };

#ifdef __SETUP__

#define NUMKEYENTRIES 56

char keydefaults[NUMGAMEFUNCTIONS*3][MAXGAMEFUNCLEN] =
   {
   "Move_Forward", "W", "Kpad8",
   "Move_Backward", "S", "Kpad2",
   "Turn_Left", "Left", "Kpad4",
   "Turn_Right", "Right", "KPad6",
   "Strafe", "LAlt", "RAlt",
   "Fire", "", "RCtrl",
   "Open", "E", "",
   "Run", "LShift", "RShift",
   "AutoRun", "CapLck", "",
   "Jump", "Space", "/",
   "Crouch", "LCtrl", "",
   "Look_Up", "PgUp", "Kpad9",
   "Look_Down", "PgDn", "Kpad3",
   "Look_Left", "Insert", "Kpad0",
   "Look_Right", "Delete", "Kpad.",
   "Strafe_Left", "A", "",
   "Strafe_Right", "D", "",
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
   "Holo_Duke", "H", "",
   "Jetpack", "J", "",
   "NightVision", "N", "",
   "MedKit", "M", "",
   "TurnAround", "BakSpc", "",
   "SendMessage", "T", "",
   "Map", "Tab", "",
   "Shrink_Screen", "-", "Kpad-",
   "Enlarge_Screen", "=", "Kpad+",
   "Center_View", "KPad5", "",
   "Holster_Weapon", "ScrLck", "",
   "Show_Opponents_Weapon", "Y", "",
   "Map_Follow_Mode", "F", "",
   "See_Coop_View", "K", "",
   "Mouse_Aiming", "U", "",
   "Toggle_Crosshair", "I", "",
   "Steroids", "R", "",
   "Quick_Kick", "Q", "",
   "Next_Weapon", "'", "",
   "Previous_Weapon", ";", "",
   "Show_Console", "`", "",
   "Show_DukeMatch_Scores", "", "",
   "Dpad_Select", "", "",
   "Dpad_Aiming", "", "",
   };

const char oldkeydefaults[NUMGAMEFUNCTIONS*3][MAXGAMEFUNCLEN] =
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
   "Look_Left", "Insert", "Kpad0",
   "Look_Right", "Delete", "Kpad.",
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
   "Holo_Duke", "H", "",
   "Jetpack", "J", "",
   "NightVision", "N", "",
   "MedKit", "M", "",
   "TurnAround", "BakSpc", "",
   "SendMessage", "T", "",
   "Map", "Tab", "",
   "Shrink_Screen", "-", "Kpad-",
   "Enlarge_Screen", "=", "Kpad+",
   "Center_View", "KPad5", "",
   "Holster_Weapon", "ScrLck", "",
   "Show_Opponents_Weapon", "W", "",
   "Map_Follow_Mode", "F", "",
   "See_Coop_View", "K", "",
   "Mouse_Aiming", "U", "",
   "Toggle_Crosshair", "I", "",
   "Steroids", "R", "",
   "Quick_Kick", "`", "",
   "Next_Weapon", "'", "",
   "Previous_Weapon", ";", "",
   "Show_Console", "C", "",
   "Show_DukeMatch_Scores", "", "",
   "Dpad_Select", "", "",
   "Dpad_Aiming", "", "",
   };

static const char * mousedefaults[MAXMOUSEBUTTONS] =
   {
   "Fire",
   "MedKit",
   "Jetpack",
   "",
   "Previous_Weapon",
   "Next_Weapon",
   };


static const char * mouseclickeddefaults[MAXMOUSEBUTTONS] =
   {
   };


static const char * mouseanalogdefaults[MAXMOUSEAXES] =
   {
   "analog_turning",
   "analog_moving",
   };


static const char * mousedigitaldefaults[MAXMOUSEDIGITAL] =
   {
   };

#if defined(GEKKO)
static const char * joystickdefaults[MAXJOYBUTTONSANDHATS] =
   {
   "Open", // A
   "Fire", // B
   "Run", // 1
   "Map", // 2
   "Previous_Weapon", // -
   "Next_Weapon", // +
   "", // Home
   "Jump", // Z
   "Crouch", // C
   "Map", // X
   "Run", // Y
   "Jump", // L
   "Quick_Kick", // R
   "Crouch", // ZL
   "Fire", // ZR
   "Quick_Kick", // D-Pad Up
   "Inventory_Right", // D-Pad Right
   "Inventory", // D-Pad Down
   "Inventory_Left", // D-Pad Left
   };


static const char * joystickclickeddefaults[MAXJOYBUTTONSANDHATS] =
   {
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
   "Inventory",
   };


static const char * joystickanalogdefaults[MAXJOYAXES] =
   {
   "analog_strafing",
   "analog_moving",
   "analog_turning",
   "analog_lookingupanddown",
   };


static const char * joystickdigitaldefaults[MAXJOYDIGITAL] =
   {
   };
#else
static const char * joystickdefaults[MAXJOYBUTTONSANDHATS] =
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


static const char * joystickclickeddefaults[MAXJOYBUTTONSANDHATS] =
   {
   "",
   "Inventory",
   "Jump",
   "Crouch",
   };


static const char * joystickanalogdefaults[MAXJOYAXES] =
   {
   "analog_turning",
   "analog_moving",
   "analog_strafing",
   };


static const char * joystickdigitaldefaults[MAXJOYDIGITAL] =
   {
   "",
   "",
   "",
   "",
   "",
   "",
   "Run",
   };
#endif

#endif
#ifdef __cplusplus
}
#endif
#endif
