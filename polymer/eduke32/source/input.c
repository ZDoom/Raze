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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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

int32_t I_CheckAllInput(void)
{
    return (
            KB_KeyWaiting() ||
            MOUSE_GetButtons() ||
            JOYSTICK_GetButtons()
            );
}
void I_ClearAllInput(void)
{
    KB_FlushKeyboardQueue();
    KB_ClearKeysDown();
    MOUSE_ClearAllButtons();
    JOYSTICK_ClearAllButtons();
}

int32_t I_CheckInputWaiting(void)
{
    return (
            KB_KeyWaiting() ||
            (MOUSE_GetButtons()&LEFT_MOUSE) ||
            I_JoystickAdvanceTrigger()
            );
}
int32_t I_ClearInputWaiting(void)
{
    KB_FlushKeyboardQueue();
    KB_ClearKeysDown(); // JBF
    I_JoystickAdvanceTriggerClear();
    return (
            MOUSE_ClearButton(LEFT_MOUSE)
            );
}

int32_t I_JoystickAdvanceTrigger(void)
{
    return (
#if defined(GEKKO)
            (JOYSTICK_GetButtons()&WII_A)
#else
            BUTTON(gamefunc_Open) ||
            BUTTON(gamefunc_Fire)
#endif
            );
}
int32_t I_JoystickAdvanceTriggerClear(void)
{
#if defined(GEKKO)
    return JOYSTICK_ClearButton(WII_A);
#else
    CONTROL_ClearButton(gamefunc_Open);
    CONTROL_ClearButton(gamefunc_Fire);
    return 0;
#endif
}
int32_t I_JoystickReturnTrigger(void)
{
    return (
            BUTTON(gamefunc_Crouch)
#if defined(GEKKO)
            || (JOYSTICK_GetButtons()&(WII_B|WII_HOME))
#endif
            );
}
int32_t I_JoystickReturnTriggerClear(void)
{
    CONTROL_ClearButton(gamefunc_Crouch);
    return (
#if defined(GEKKO)
            JOYSTICK_ClearButton(WII_B) ||
            JOYSTICK_ClearButton(WII_HOME)
#else
            0
#endif
            );
}
int32_t I_JoystickEscapeTrigger(void)
{
    return (
#if defined(GEKKO)
            (JOYSTICK_GetButtons()&WII_HOME)
#else
            0
#endif
            );
}
int32_t I_JoystickEscapeTriggerClear(void)
{
    return (
#if defined(GEKKO)
            JOYSTICK_ClearButton(WII_HOME)
#else
            0
#endif
            );
}

int32_t I_AdvanceTrigger(void)
{
    return (
            KB_KeyPressed(sc_kpad_Enter) ||
            KB_KeyPressed(sc_Enter) ||
            (MOUSE_GetButtons()&LEFT_MOUSE) ||
            I_JoystickAdvanceTrigger()
            );
}
int32_t I_AdvanceTriggerClear(void)
{
    KB_FlushKeyboardQueue();
    KB_ClearKeyDown(sc_kpad_Enter);
    KB_ClearKeyDown(sc_Enter);
    I_JoystickAdvanceTriggerClear();
    return (
            MOUSE_ClearButton(LEFT_MOUSE)
            );
}
int32_t I_ReturnTrigger(void)
{
    return (
            KB_KeyPressed(sc_Escape) ||
            (MOUSE_GetButtons()&RIGHT_MOUSE) ||
            I_JoystickReturnTrigger()
            );
}
int32_t I_ReturnTriggerClear(void)
{
    KB_FlushKeyboardQueue();
    KB_ClearKeyDown(sc_Escape);
    return (
            MOUSE_ClearButton(RIGHT_MOUSE) ||
            I_JoystickReturnTriggerClear()
            );
}
int32_t I_EscapeTrigger(void)
{
    return (
            KB_KeyPressed(sc_Escape) ||
            I_JoystickEscapeTrigger()
            );
}
int32_t I_EscapeTriggerClear(void)
{
    KB_FlushKeyboardQueue();
    KB_ClearKeyDown(sc_Escape);
    return (
            I_JoystickEscapeTriggerClear()
            );
}



int32_t I_PanelUp(void)
{
    return (
            KB_KeyPressed(sc_LeftArrow) ||
            KB_KeyPressed(sc_kpad_4) ||
            KB_KeyPressed(sc_UpArrow) ||
            KB_KeyPressed(sc_kpad_8) ||
            KB_KeyPressed(sc_PgUp) ||
            (MOUSE_GetButtons()&WHEELUP_MOUSE) ||
            BUTTON(gamefunc_Move_Forward) ||
            BUTTON(gamefunc_Turn_Left) ||
            BUTTON(gamefunc_Strafe_Left) ||
            (JOYSTICK_GetHat(0)&HAT_UP)
            );
}
int32_t I_PanelUpClear(void)
{
    KB_FlushKeyboardQueue();
    KB_ClearKeyDown(sc_LeftArrow);
    KB_ClearKeyDown(sc_kpad_4);
    KB_ClearKeyDown(sc_UpArrow);
    KB_ClearKeyDown(sc_kpad_8);
    KB_ClearKeyDown(sc_PgUp);
    CONTROL_ClearButton(gamefunc_Move_Forward);
    CONTROL_ClearButton(gamefunc_Turn_Left);
    CONTROL_ClearButton(gamefunc_Strafe_Left);
    JOYSTICK_ClearHat(0);
    return (
            MOUSE_ClearButton(WHEELUP_MOUSE)
            );
}

int32_t I_PanelDown(void)
{
    return (
            KB_KeyPressed(sc_RightArrow) ||
            KB_KeyPressed(sc_kpad_6) ||
            KB_KeyPressed(sc_DownArrow) ||
            KB_KeyPressed(sc_kpad_2) ||
            KB_KeyPressed(sc_PgDn) ||
            (MOUSE_GetButtons()&WHEELDOWN_MOUSE) ||
            BUTTON(gamefunc_Move_Backward) ||
            BUTTON(gamefunc_Turn_Right) ||
            BUTTON(gamefunc_Strafe_Right) ||
            (JOYSTICK_GetHat(0)&HAT_DOWN) ||
            I_AdvanceTrigger()
            );
}
int32_t I_PanelDownClear(void)
{
    KB_FlushKeyboardQueue();
    KB_ClearKeyDown(sc_RightArrow);
    KB_ClearKeyDown(sc_kpad_6);
    KB_ClearKeyDown(sc_DownArrow);
    KB_ClearKeyDown(sc_kpad_2);
    KB_ClearKeyDown(sc_PgDn);
    CONTROL_ClearButton(gamefunc_Move_Backward);
    CONTROL_ClearButton(gamefunc_Turn_Right);
    CONTROL_ClearButton(gamefunc_Strafe_Right);
    JOYSTICK_ClearHat(0);
    return (
            MOUSE_ClearButton(WHEELDOWN_MOUSE) ||
            I_AdvanceTriggerClear()
            );
}





char inputloc = 0;

int32_t _EnterText(int32_t small,int32_t x,int32_t y,char *t,int32_t dalen,int32_t c)
{
    char ch;
    int32_t i;

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
                return (1);
            }
            else if (ch == asc_Escape)
            {
                I_ReturnTriggerClear();
                return (-1);
            }
            else if (ch >= 32 && inputloc < dalen && ch < 127)
            {
                if (c != 997 || (ch >= '0' && ch <= '9'))
                {
                    // JBF 20040508: so we can have numeric only if we want
                    *(t+inputloc) = ch;
                    *(t+inputloc+1) = 0;
                    inputloc++;
                }
            }
        }
    }

    if (c == 999) return(0);
    if (c == 998)
    {
        char b[91],ii;
        for (ii=0; ii<inputloc; ii++)
            b[(uint8_t)ii] = '*';
        b[(uint8_t)inputloc] = 0;
        if (g_player[myconnectindex].ps->gm&MODE_TYPE)
            x = mpgametext(y,b,c,2+8+16);
        else x = gametext(x,y,b,c,2+8+16);
    }
    else
    {
        if (g_player[myconnectindex].ps->gm&MODE_TYPE)
            x = mpgametext(y,t,c,2+8+16);
        else x = gametext(x,y,t,c,2+8+16);
    }
    c = 4-(sintable[(totalclock<<4)&2047]>>11);

    i = G_GameTextLen(USERQUOTE_LEFTOFFSET,OSD_StripColors(tempbuf,t));
    while (i > (ud.config.ScreenWidth - USERQUOTE_RIGHTOFFSET))
    {
        i -= (ud.config.ScreenWidth - USERQUOTE_RIGHTOFFSET);
        if (small&1)
            y += textsc(6);
        y += 8;
    }

    if (small&1)
        rotatesprite_fs(textsc(x)<<16,(y<<16),32768,0,SPINNINGNUKEICON+((totalclock>>3)%7),c,0,(8|16));
    else rotatesprite_fs((x+8)<<16,(y+4)<<16,32768,0,SPINNINGNUKEICON+((totalclock>>3)%7),c,0,2+8);
    return (0);
}
