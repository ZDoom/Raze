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
#include "input.h"
#include "inputstate.h"

char typebuf[TYPEBUFSIZE];


bool mouseInactiveConditional(bool condition)
{
	return condition;
}

int32_t I_TextSubmit(void)
{
    return
        inputState.GetKeyStatus(sc_Enter)
        || inputState.GetKeyStatus(sc_kpad_Enter)
        || inputState.GetKeyStatus(KEY_MOUSE1)
        /*|| (JOYSTICK_GetGameControllerButtons()&(1<<GAMECONTROLLER_BUTTON_A))*/;
}

void I_TextSubmitClear(void)
{
    inputState.keyFlushChars();
    inputState.ClearKeyStatus(sc_kpad_Enter);
    inputState.ClearKeyStatus(sc_Enter);
    inputState.ClearKeyStatus(KEY_MOUSE1);
    //JOYSTICK_ClearGameControllerButton(1<<GAMECONTROLLER_BUTTON_A);
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
        || inputState.GetKeyStatus(KEY_MOUSE2)
        /*|| (JOYSTICK_GetGameControllerButtons()&(1<<GAMECONTROLLER_BUTTON_B))*/;
}

void I_ReturnTriggerClear(void)
{
    inputState.keyFlushChars();
    inputState.ClearKeyStatus(sc_Escape);
    inputState.ClearKeyStatus(KEY_MOUSE2);
    //JOYSTICK_ClearGameControllerButton(1<<GAMECONTROLLER_BUTTON_B);
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

