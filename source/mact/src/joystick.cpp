/*
 * joystick.c
 * MACT library -to- Build Port Joystick Glue
 *
 * by Hendricks266
 *
 * We needed raw joystick access for the Wii port.
 * We only had raw mouse and keyboard access.
 * I made raw joystick access.
 *
 */
//-------------------------------------------------------------------------
/*
Duke Nukem Copyright (C) 1996, 2003 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
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
*/
//-------------------------------------------------------------------------

#include "compat.h"

#include "joystick.h"
#include "baselayer.h"

#include "keyboard.h"
#include "_control.h"

int32_t JOYSTICK_GetButtons(void)
{
    int32_t buttons;

    readjoybstatus(&buttons);

    if (joynumhats > 0)
    {
        int32_t hat = JOYSTICK_GetHat(0);
        if (hat != 0)
            buttons |= hat << min(MAXJOYBUTTONS, joynumbuttons);
    }

    return buttons;
}
int32_t JOYSTICK_ClearButton(int32_t b)
{
    return (joyb &= ~b);
}
void JOYSTICK_ClearAllButtons(void)
{
    joyb = 0;
}

int32_t JOYSTICK_GetHat(int32_t h)
{
    if (h>=0 && h<joynumhats)
    {
        if (joyhat[h] == -1)
            return (HAT_CENTERED);
        else
        {
            static const int32_t hatstate[] = { HAT_UP, HAT_RIGHTUP, HAT_RIGHT, HAT_RIGHTDOWN, HAT_DOWN, HAT_LEFTDOWN, HAT_LEFT, HAT_LEFTUP };
            int32_t val;

            // thanks SDL for this much more sensible method
            val = ((joyhat[0] + 4500 / 2) % 36000) / 4500;
            if (val < 8)
                return hatstate[val];
        }
    }
    return 0;
}
void JOYSTICK_ClearHat(int32_t h)
{
    if (h>=0 && h<joynumhats)
        joyhat[h] = -1;
}
void JOYSTICK_ClearAllHats(void)
{
    int32_t h;
    for (h=0; h<joynumhats; ++h)
        joyhat[h] = -1;
}

int32_t JOYSTICK_GetAxis(int32_t a)
{
    return ((a>=0 && a<joynumaxes)?joyaxis[a]:0);
}
void JOYSTICK_ClearAxis(int32_t a)
{
    if (a>=0 && a<joynumaxes)
        joyaxis[a] = 0;
}
void JOYSTICK_ClearAllAxes(void)
{
    int32_t a;
    for (a=0; a<joynumaxes; ++a)
        joyaxis[a] = 0;
}
