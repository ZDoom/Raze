//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful
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

BEGIN_BLD_NS

#ifndef function_private_h_
#define function_private_h_

#ifdef __SETUP__

static const char * mousedefaults[MAXMOUSEBUTTONS] =
   {
   "Weapon_Fire"
   "Weapon_Special_Fire"
   ""
   ""
   "Previous_Weapon"
   "Next_Weapon"
   };


static const char * mouseclickeddefaults[MAXMOUSEBUTTONS] =
   {
   };


static const char * mouseanalogdefaults[MAXMOUSEAXES] =
   {
   "analog_turning"
   "analog_moving"
   };


static const char * mousedigitaldefaults[MAXMOUSEDIGITAL] =
   {
   };

static const char * joystickdefaults[MAXJOYBUTTONSANDHATS] =
   {
   "Fire"
   "Strafe"
   "Run"
   "Open"
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   ""
   "Aim_Down"
   "Look_Right"
   "Aim_Up"
   "Look_Left"
   };


static const char * joystickclickeddefaults[MAXJOYBUTTONSANDHATS] =
   {
   ""
   "Inventory"
   "Jump"
   "Crouch"
   };


static const char * joystickanalogdefaults[MAXJOYAXES] =
   {
   "analog_turning"
   "analog_moving"
   "analog_strafing"
   };


static const char * joystickdigitaldefaults[MAXJOYDIGITAL] =
   {
   ""
   ""
   ""
   ""
   ""
   ""
   "Run"
   };

#endif
#endif

END_BLD_NS
