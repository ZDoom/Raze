//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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

#include "ns.h"	// Must come before everything else!

#include "build.h"
#include "compat.h"
#include "common_game.h"
#include "blood.h"
#include "config.h"
#include "globals.h"
#include "inifile.h"
#include "levels.h"
#include "qav.h"
#include "demo.h"
#include "view.h"
#include "c_bind.h"
#include "gstrings.h"

BEGIN_BLD_NS


void drawLoadingScreen(void)
{
	char buffer[80];
	if (gGameOptions.nGameType == 0)
	{
		if (gDemo.at1)
			strcpy(buffer, GStrings("TXTB_LDEMO"));
		else
			strcpy(buffer, GStrings("TXTB_LLEVEL"));
	}
	else
		strcpy(buffer, GStrings(FStringf("TXTB_NETGT%d", gGameOptions.nGameType)));
	viewLoadingScreen(2049, buffer, levelGetTitle(), NULL);
}


END_BLD_NS
