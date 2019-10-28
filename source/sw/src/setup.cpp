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
#include "ns.h"
#include "build.h"

#include "keys.h"
#include "game.h"

#include "mytypes.h"
#include "fx_man.h"
#include "music.h"
#include "gamedefs.h"
#include "keyboard.h"

#include "control.h"
#include "config.h"
#include "sounds.h"
#include "gamecontrol.h"

#include "rts.h"

BEGIN_SW_NS

void CenterCenter(void)
{
    printf("\nCenter the joystick and press a button\n");
}

void UpperLeft(void)
{
    printf("Move joystick to upper-left corner and press a button\n");
}

void LowerRight(void)
{
    printf("Move joystick to lower-right corner and press a button\n");
}

void CenterThrottle(void)
{
    printf("Center the throttle control and press a button\n");
}

void CenterRudder(void)
{
    printf("Center the rudder control and press a button\n");
}

/*
===================
=
= GetTime
=
===================
*/

static int32_t timert;

int32_t GetTime(void)
{
    return (int32_t) totalclock;
    //return timert++;
}

void InitSetup(void)
{
    CONFIG_SetupMouse();
    CONFIG_SetupJoystick();
}

#if 0
void TermSetup(void)
{
    //FreeKeyDefList();
}
#endif


END_SW_NS
