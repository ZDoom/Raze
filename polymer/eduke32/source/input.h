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

#ifndef __input_h__
#define __input_h__

extern int32_t I_CheckAllInput(void);
extern void I_ClearAllInput(void);

extern int32_t I_CheckInputWaiting(void);
extern int32_t I_ClearInputWaiting(void);

// Advance = Selecting a menu option || Saying "Yes" || Going forward in Help/Credits
// Return = Closing a sub-menu || Saying "No"
// Escape = Opening the menu in-game (should not be any gamefuncs)

// Joysticks have separate functions to avoid spamming duplicated "ifdef GEKKO" everywhere.
extern int32_t I_JoystickAdvanceTrigger(void);
extern int32_t I_JoystickAdvanceTriggerClear(void);
extern int32_t I_JoystickReturnTrigger(void);
extern int32_t I_JoystickReturnTriggerClear(void);
extern int32_t I_JoystickEscapeTrigger(void);
extern int32_t I_JoystickEscapeTriggerClear(void);

extern int32_t I_AdvanceTrigger(void);
extern int32_t I_AdvanceTriggerClear(void);
extern int32_t I_ReturnTrigger(void);
extern int32_t I_ReturnTriggerClear(void);
extern int32_t I_EscapeTrigger(void);
extern int32_t I_EscapeTriggerClear(void);

extern int32_t I_PanelUp(void);
extern int32_t I_PanelUpClear(void);

extern int32_t I_PanelDown(void);
extern int32_t I_PanelDownClear(void);

extern char inputloc;
extern int32_t _EnterText(int32_t small,int32_t x,int32_t y,char *t,int32_t dalen,int32_t c);

#endif
