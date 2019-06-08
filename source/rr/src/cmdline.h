//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

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

#ifndef cmdline_h__
#define cmdline_h__

#include "compat.h"

extern void G_CheckCommandLine(int32_t argc, char const * const * argv);
extern void G_ShowParameterHelp(void);
extern void G_ShowDebugHelp(void);

extern int32_t g_commandSetup;
extern int32_t g_noSetup;
extern int32_t g_noAutoLoad;
extern int32_t g_noSound;
extern int32_t g_noMusic;
extern const char *CommandMap;
extern const char *CommandName;
extern int32_t g_forceWeaponChoice;
extern int32_t g_fakeMultiMode;
#endif // cmdline_h__
