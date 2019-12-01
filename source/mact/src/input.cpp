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

#include "gamecontrol.h"
#include "keyboard.h"
#include "mouse.h"
#include "joystick.h"
#include "control.h"
#include "input.h"
#include "inputstate.h"

char typebuf[TYPEBUFSIZE];


bool mouseInactiveConditional(bool condition)
{
	return condition;
}

int32_t I_CheckAllInput(void)
{
    return
        inputState.keyBufferWaiting()
        || inputState.MouseGetButtons()
        || JOYSTICK_GetButtons();
}
void I_ClearAllInput(void)
{
    inputState.keyFlushChars();
    inputState.ClearKeysDown();
    inputState.MouseClearAllButtonss();
    JOYSTICK_ClearAllButtons();
    buttonMap.ResetButtonStates();
}


int32_t I_TextSubmit(void)
{
    return
        inputState.GetKeyStatus(sc_Enter)
        || inputState.GetKeyStatus(sc_kpad_Enter)
        || mouseInactiveConditional(inputState.MouseGetButtons()&LEFT_MOUSE)
        || (JOYSTICK_GetGameControllerButtons()&(1<<GAMECONTROLLER_BUTTON_A));
}

void I_TextSubmitClear(void)
{
    inputState.keyFlushChars();
    inputState.ClearKeyStatus(sc_kpad_Enter);
    inputState.ClearKeyStatus(sc_Enter);
    inputState.MouseClearButton(LEFT_MOUSE);
    JOYSTICK_ClearGameControllerButton(1<<GAMECONTROLLER_BUTTON_A);
}

int32_t I_AdvanceTrigger(void)
{
    return
        I_TextSubmit()
        || inputState.GetKeyStatus(sc_Space);
}

void I_AdvanceTriggerClear(void)
{
    I_TextSubmitClear();
    inputState.ClearKeyStatus(sc_Space);
}

int32_t I_ReturnTrigger(void)
{
    return
        inputState.GetKeyStatus(sc_Escape)
        || (inputState.MouseGetButtons()&RIGHT_MOUSE)
        || (JOYSTICK_GetGameControllerButtons()&(1<<GAMECONTROLLER_BUTTON_B));
}

void I_ReturnTriggerClear(void)
{
    inputState.keyFlushChars();
    inputState.ClearKeyStatus(sc_Escape);
    inputState.MouseClearButton(RIGHT_MOUSE);
    JOYSTICK_ClearGameControllerButton(1<<GAMECONTROLLER_BUTTON_B);
}

int32_t I_GeneralTrigger(void)
{
    return
        I_AdvanceTrigger()
        || I_ReturnTrigger()
        || buttonMap.ButtonDown(gamefunc_Open)
        || mouseInactiveConditional(buttonMap.ButtonDown(gamefunc_Fire))
        || buttonMap.ButtonDown(gamefunc_Crouch)
        || (JOYSTICK_GetGameControllerButtons()&(1<<GAMECONTROLLER_BUTTON_START));
}

void I_GeneralTriggerClear(void)
{
    I_AdvanceTriggerClear();
    I_ReturnTriggerClear();
    buttonMap.ClearButton(gamefunc_Open);
    buttonMap.ClearButton(gamefunc_Fire);
    buttonMap.ClearButton(gamefunc_Crouch);
    JOYSTICK_ClearGameControllerButton(1<<GAMECONTROLLER_BUTTON_START);
}


int32_t I_EscapeTrigger(void)
{
    return
        inputState.GetKeyStatus(sc_Escape)
        || (JOYSTICK_GetGameControllerButtons()&(1<<GAMECONTROLLER_BUTTON_START));
}

void I_EscapeTriggerClear(void)
{
    inputState.keyFlushChars();
    inputState.ClearKeyStatus(sc_Escape);
    JOYSTICK_ClearGameControllerButton(1<<GAMECONTROLLER_BUTTON_START);
}


int32_t I_MenuUp(void)
{
    return
        inputState.GetKeyStatus(sc_UpArrow)
        || inputState.GetKeyStatus(sc_kpad_8)
        || (inputState.MouseGetButtons()&WHEELUP_MOUSE)
        || buttonMap.ButtonDown(gamefunc_Move_Forward)
        || (JOYSTICK_GetHat(0)&HAT_UP)
        || (JOYSTICK_GetGameControllerButtons()&(1<<GAMECONTROLLER_BUTTON_DPAD_UP))
        || CONTROL_GetGameControllerDigitalAxisNeg(GAMECONTROLLER_AXIS_LEFTY);
}

void I_MenuUpClear(void)
{
    inputState.ClearKeyStatus(sc_UpArrow);
    inputState.ClearKeyStatus(sc_kpad_8);
    inputState.MouseClearButton(WHEELUP_MOUSE);
    buttonMap.ClearButton(gamefunc_Move_Forward);
    JOYSTICK_ClearHat(0);
    JOYSTICK_ClearGameControllerButton(1<<GAMECONTROLLER_BUTTON_DPAD_UP);
    CONTROL_ClearGameControllerDigitalAxisNeg(GAMECONTROLLER_AXIS_LEFTY);
}


int32_t I_MenuDown(void)
{
    return
        inputState.GetKeyStatus(sc_DownArrow)
        || inputState.GetKeyStatus(sc_kpad_2)
        || (inputState.MouseGetButtons()&WHEELDOWN_MOUSE)
        || buttonMap.ButtonDown(gamefunc_Move_Backward)
        || (JOYSTICK_GetHat(0)&HAT_DOWN)
        || (JOYSTICK_GetGameControllerButtons()&(1<<GAMECONTROLLER_BUTTON_DPAD_DOWN))
        || CONTROL_GetGameControllerDigitalAxisPos(GAMECONTROLLER_AXIS_LEFTY);
}

void I_MenuDownClear(void)
{
    inputState.ClearKeyStatus(sc_DownArrow);
    inputState.ClearKeyStatus(sc_kpad_2);
    inputState.ClearKeyStatus(sc_PgDn);
    inputState.MouseClearButton(WHEELDOWN_MOUSE);
    buttonMap.ClearButton(gamefunc_Move_Backward);
    JOYSTICK_ClearHat(0);
    JOYSTICK_ClearGameControllerButton(1<<GAMECONTROLLER_BUTTON_DPAD_DOWN);
    CONTROL_ClearGameControllerDigitalAxisPos(GAMECONTROLLER_AXIS_LEFTY);
}


int32_t I_MenuLeft(void)
{
    return
        inputState.GetKeyStatus(sc_LeftArrow)
        || inputState.GetKeyStatus(sc_kpad_4)
        || (inputState.ShiftPressed() && inputState.GetKeyStatus(sc_Tab))
        || buttonMap.ButtonDown(gamefunc_Turn_Left)
        || buttonMap.ButtonDown(gamefunc_Strafe_Left)
        || (JOYSTICK_GetHat(0)&HAT_LEFT)
        || (JOYSTICK_GetGameControllerButtons()&(1<<GAMECONTROLLER_BUTTON_DPAD_LEFT))
        || CONTROL_GetGameControllerDigitalAxisNeg(GAMECONTROLLER_AXIS_LEFTX);
}

void I_MenuLeftClear(void)
{
    inputState.ClearKeyStatus(sc_LeftArrow);
    inputState.ClearKeyStatus(sc_kpad_4);
    inputState.ClearKeyStatus(sc_Tab);
    buttonMap.ClearButton(gamefunc_Turn_Left);
    buttonMap.ClearButton(gamefunc_Strafe_Left);
    JOYSTICK_ClearHat(0);
    JOYSTICK_ClearGameControllerButton(1<<GAMECONTROLLER_BUTTON_DPAD_LEFT);
    CONTROL_ClearGameControllerDigitalAxisNeg(GAMECONTROLLER_AXIS_LEFTX);
}


int32_t I_MenuRight(void)
{
    return
        inputState.GetKeyStatus(sc_RightArrow)
        || inputState.GetKeyStatus(sc_kpad_6)
        || (!inputState.ShiftPressed() && inputState.GetKeyStatus(sc_Tab))
        || buttonMap.ButtonDown(gamefunc_Turn_Right)
        || buttonMap.ButtonDown(gamefunc_Strafe_Right)
        || (inputState.MouseGetButtons()&MIDDLE_MOUSE)
        || (JOYSTICK_GetHat(0)&HAT_RIGHT)
        || (JOYSTICK_GetGameControllerButtons()&(1<<GAMECONTROLLER_BUTTON_DPAD_RIGHT))
        || CONTROL_GetGameControllerDigitalAxisPos(GAMECONTROLLER_AXIS_LEFTX)
        ;
}

void I_MenuRightClear(void)
{
    inputState.ClearKeyStatus(sc_RightArrow);
    inputState.ClearKeyStatus(sc_kpad_6);
    inputState.ClearKeyStatus(sc_Tab);
    buttonMap.ClearButton(gamefunc_Turn_Right);
    buttonMap.ClearButton(gamefunc_Strafe_Right);
    inputState.MouseClearButton(MIDDLE_MOUSE);
    JOYSTICK_ClearHat(0);
    JOYSTICK_ClearGameControllerButton(1<<GAMECONTROLLER_BUTTON_DPAD_RIGHT);
    CONTROL_ClearGameControllerDigitalAxisPos(GAMECONTROLLER_AXIS_LEFTX);
}


int32_t I_EnterText(char *t, int32_t maxlength, int32_t flags)
{
    char ch;
    int32_t inputloc = strlen(typebuf);

    while ((ch = inputState.keyGetChar()) != 0)
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

    if (I_TextSubmit())
    {
        I_TextSubmitClear();
        return 1;
    }
    if (I_ReturnTrigger())
    {
        I_ReturnTriggerClear();
        return -1;
    }

    return 0;
}

