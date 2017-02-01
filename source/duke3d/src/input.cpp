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

#include "global.h"
#include "game.h"
#include "function.h"
#include "keyboard.h"
#include "mouse.h"
#include "joystick.h"
#include "control.h"
#include "input.h"
#include "menus.h"

int32_t I_CheckAllInput(void)
{
    return (
#if defined EDUKE32_IOS
            mousepressstate == Mouse_Pressed ||
#endif
            KB_KeyWaiting() ||
            MOUSE_GetButtons() ||
            JOYSTICK_GetButtons()
            );
}
void I_ClearAllInput(void)
{
#if defined EDUKE32_IOS
    mousepressstateadvance();
#endif
    KB_FlushKeyboardQueue();
    KB_ClearKeysDown();
    MOUSE_ClearAllButtons();
    JOYSTICK_ClearAllButtons();
}


int32_t I_AdvanceTrigger(void)
{
    return (
            KB_KeyPressed(sc_kpad_Enter) ||
            KB_KeyPressed(sc_Enter) ||
#if !defined EDUKE32_TOUCH_DEVICES
            MOUSEINACTIVECONDITIONAL(MOUSE_GetButtons()&LEFT_MOUSE) ||
#endif
#if defined(GEKKO)
            MOUSEINACTIVECONDITIONAL(JOYSTICK_GetButtons()&WII_A)
#else
            BUTTON(gamefunc_Open) ||
# if !defined EDUKE32_TOUCH_DEVICES
            MOUSEINACTIVECONDITIONAL(BUTTON(gamefunc_Fire))
# else
            BUTTON(gamefunc_Fire)
# endif
#endif
            );
}

void I_AdvanceTriggerClear(void)
{
    KB_FlushKeyboardQueue();
    KB_ClearKeyDown(sc_kpad_Enter);
    KB_ClearKeyDown(sc_Enter);
    MOUSE_ClearButton(LEFT_MOUSE);
#if defined(GEKKO)
    JOYSTICK_ClearButton(WII_A);
#else
    CONTROL_ClearButton(gamefunc_Open);
    CONTROL_ClearButton(gamefunc_Fire);
#endif
}

int32_t I_ReturnTrigger(void)
{
    return (
            KB_KeyPressed(sc_Escape) ||
            (MOUSE_GetButtons()&RIGHT_MOUSE) ||
            BUTTON(gamefunc_Crouch)
#if defined(GEKKO)
            || (JOYSTICK_GetButtons()&(WII_B|WII_HOME))
#endif
            );
}

void I_ReturnTriggerClear(void)
{
    KB_FlushKeyboardQueue();
    KB_ClearKeyDown(sc_Escape);
    MOUSE_ClearButton(RIGHT_MOUSE);
    CONTROL_ClearButton(gamefunc_Crouch);
#if defined(GEKKO)
    JOYSTICK_ClearButton(WII_B);
    JOYSTICK_ClearButton(WII_HOME);
#endif
}


int32_t I_EscapeTrigger(void)
{
    return (
            KB_KeyPressed(sc_Escape)
#if defined(GEKKO)
            || (JOYSTICK_GetButtons()&WII_HOME)
#endif
            );
}

void I_EscapeTriggerClear(void)
{
    KB_FlushKeyboardQueue();
    KB_ClearKeyDown(sc_Escape);
#if defined(GEKKO)
    JOYSTICK_ClearButton(WII_HOME);
#endif
}


int32_t I_MenuUp(void)
{
    return (
            KB_KeyPressed(sc_UpArrow) ||
            KB_KeyPressed(sc_kpad_8) ||
            (MOUSE_GetButtons()&WHEELUP_MOUSE) ||
            BUTTON(gamefunc_Move_Forward) ||
            (JOYSTICK_GetHat(0)&HAT_UP)
            );
}

void I_MenuUpClear(void)
{
    KB_ClearKeyDown(sc_UpArrow);
    KB_ClearKeyDown(sc_kpad_8);
    MOUSE_ClearButton(WHEELUP_MOUSE);
    CONTROL_ClearButton(gamefunc_Move_Forward);
    JOYSTICK_ClearHat(0);
}


int32_t I_MenuDown(void)
{
    return (
            KB_KeyPressed(sc_DownArrow) ||
            KB_KeyPressed(sc_kpad_2) ||
            (MOUSE_GetButtons()&WHEELDOWN_MOUSE) ||
            BUTTON(gamefunc_Move_Backward) ||
            (JOYSTICK_GetHat(0)&HAT_DOWN)
            );
}

void I_MenuDownClear(void)
{
    KB_ClearKeyDown(sc_DownArrow);
    KB_ClearKeyDown(sc_kpad_2);
    KB_ClearKeyDown(sc_PgDn);
    MOUSE_ClearButton(WHEELDOWN_MOUSE);
    CONTROL_ClearButton(gamefunc_Move_Backward);
    JOYSTICK_ClearHat(0);
}


int32_t I_MenuLeft(void)
{
    return (
            KB_KeyPressed(sc_LeftArrow) ||
            KB_KeyPressed(sc_kpad_4) ||
            (SHIFTS_IS_PRESSED && KB_KeyPressed(sc_Tab)) ||
            BUTTON(gamefunc_Turn_Left) ||
            BUTTON(gamefunc_Strafe_Left) ||
            (JOYSTICK_GetHat(0)&HAT_LEFT)
            );
}

void I_MenuLeftClear(void)
{
    KB_ClearKeyDown(sc_LeftArrow);
    KB_ClearKeyDown(sc_kpad_4);
    KB_ClearKeyDown(sc_Tab);
    CONTROL_ClearButton(gamefunc_Turn_Left);
    CONTROL_ClearButton(gamefunc_Strafe_Left);
    JOYSTICK_ClearHat(0);
}


int32_t I_MenuRight(void)
{
    return (
            KB_KeyPressed(sc_RightArrow) ||
            KB_KeyPressed(sc_kpad_6) ||
            (!SHIFTS_IS_PRESSED && KB_KeyPressed(sc_Tab)) ||
            BUTTON(gamefunc_Turn_Right) ||
            BUTTON(gamefunc_Strafe_Right) ||
            (MOUSE_GetButtons()&MIDDLE_MOUSE) ||
            (JOYSTICK_GetHat(0)&HAT_RIGHT)
            );
}

void I_MenuRightClear(void)
{
    KB_ClearKeyDown(sc_RightArrow);
    KB_ClearKeyDown(sc_kpad_6);
    KB_ClearKeyDown(sc_Tab);
    CONTROL_ClearButton(gamefunc_Turn_Right);
    CONTROL_ClearButton(gamefunc_Strafe_Right);
    MOUSE_ClearButton(MIDDLE_MOUSE);
    JOYSTICK_ClearHat(0);
}


int32_t I_PanelUp(void)
{
    return (
            KB_KeyPressed(sc_PgUp) ||
            I_MenuUp() ||
            I_MenuLeft()
            );
}

void I_PanelUpClear(void)
{
    KB_ClearKeyDown(sc_PgUp);
    I_MenuUpClear();
    I_MenuLeftClear();
}


int32_t I_PanelDown(void)
{
    return (
            KB_KeyPressed(sc_PgDn) ||
            I_MenuDown() ||
            I_MenuRight() ||
            I_AdvanceTrigger()
            );
}

void I_PanelDownClear(void)
{
    KB_ClearKeyDown(sc_PgDn);
    I_MenuDownClear();
    I_MenuRightClear();
    I_AdvanceTriggerClear();
}


int32_t I_SliderLeft(void)
{
    return (
#if !defined EDUKE32_TOUCH_DEVICES
            MOUSEINACTIVECONDITIONAL((MOUSE_GetButtons()&LEFT_MOUSE) && (MOUSE_GetButtons()&WHEELUP_MOUSE)) ||
#endif
            I_MenuLeft()
            );
}

void I_SliderLeftClear(void)
{
    I_MenuLeftClear();
    MOUSE_ClearButton(WHEELUP_MOUSE);
}


int32_t I_SliderRight(void)
{
    return (
#if !defined EDUKE32_TOUCH_DEVICES
            MOUSEINACTIVECONDITIONAL((MOUSE_GetButtons()&LEFT_MOUSE) && (MOUSE_GetButtons()&WHEELDOWN_MOUSE)) ||
#endif
            I_MenuRight()
            );
}

void I_SliderRightClear(void)
{
    I_MenuRightClear();
    MOUSE_ClearButton(WHEELDOWN_MOUSE);
}


int32_t I_EnterText(char *t, int32_t maxlength, int32_t flags)
{
    char ch;
    int32_t inputloc = Bstrlen(typebuf);

    while ((ch = KB_GetCh()) != 0)
    {
        if (ch == asc_BackSpace)
        {
            if (inputloc > 0)
            {
                inputloc--;
                *(t+inputloc) = 0;
            }
        }
        else
        {
            if (ch == asc_Enter)
            {
                I_AdvanceTriggerClear();
                return 1;
            }
            else if (ch == asc_Escape)
            {
                I_ReturnTriggerClear();
                return -1;
            }
            else if (ch >= 32 && inputloc < maxlength && ch < 127)
            {
                if (!(flags & INPUT_NUMERIC) || (ch >= '0' && ch <= '9'))
                {
                    // JBF 20040508: so we can have numeric only if we want
                    *(t+inputloc) = ch;
                    *(t+inputloc+1) = 0;
                    inputloc++;
                }
            }
        }
    }

    // All gamefuncs (and *only* _gamefuncs_) in I_ReturnTriggerClear() should be replicated here.
    CONTROL_ClearButton(gamefunc_Crouch);
    if (I_ReturnTrigger())
    {
        I_ReturnTriggerClear();
        return -1;
    }

    return 0;
}
