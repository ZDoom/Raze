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

BEGIN_SW_NS


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
END_SW_NS
#endif
