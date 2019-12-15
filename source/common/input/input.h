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

#ifndef input_h_
#define input_h_


enum {
	TYPEBUFSIZE = 141
};
extern char typebuf[TYPEBUFSIZE];


extern int32_t I_CheckAllInput(void);
extern void I_ClearAllInput(void);

// Advance = Selecting a menu option || Saying "Yes" || Going forward in Help/Credits
// Return = Closing a sub-menu || Saying "No"
// General = Advance + Return = Skipping screens
// Escape = Opening the menu in-game (should not be any gamefuncs)

extern int32_t I_AdvanceTrigger(void);
extern void I_AdvanceTriggerClear(void);
extern int32_t I_ReturnTrigger(void);
extern void I_ReturnTriggerClear(void);
extern int32_t I_GeneralTrigger(void);
extern void I_GeneralTriggerClear(void);
extern int32_t I_EscapeTrigger(void);
extern void I_EscapeTriggerClear(void);


enum EnterTextFlags_t {
    INPUT_NUMERIC        = 0x00000001,
};

extern int32_t I_EnterText(char *t, int32_t maxlength, int32_t flags);

#endif
