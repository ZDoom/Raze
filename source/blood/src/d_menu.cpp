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
#include "mouse.h"
#include "common_game.h"
#include "blood.h"
#include "config.h"
#include "gamemenu.h"
#include "globals.h"
#include "inifile.h"
#include "levels.h"
#include "menu.h"
#include "qav.h"
#include "resource.h"
#include "view.h"
#include "demo.h"
#include "network.h"
#include "c_bind.h"

bool ShowOptionMenu();

BEGIN_BLD_NS

CMenuTextMgr gMenuTextMgr;

CMenuTextMgr::CMenuTextMgr()
{
	at0 = -1;
}

void CMenuTextMgr::DrawText(const char* pString, int nFont, int x, int y, int nShade, int nPalette, bool shadow)
{
	viewDrawText(nFont, pString, x, y, nShade, nPalette, 0, shadow);
}

void CMenuTextMgr::GetFontInfo(int nFont, const char* pString, int* pXSize, int* pYSize)
{
	if (nFont < 0 || nFont >= 5)
		return;
	viewGetFontInfo(nFont, pString, pXSize, pYSize);
}

const char* zNetGameTypes[] =
{
	"Cooperative",
	"Bloodbath",
	"Teams",
};

void drawLoadingScreen(void)
{
	char buffer[80];
	if (gGameOptions.nGameType == 0)
	{
		if (gDemo.at1)
			sprintf(buffer, "Loading Demo");
		else
			sprintf(buffer, "Loading Level");
	}
	else
		sprintf(buffer, "%s", zNetGameTypes[gGameOptions.nGameType - 1]);
	viewLoadingScreen(2049, buffer, levelGetTitle(), NULL);
}

void UpdateNetworkMenus(void)
{
	// Kept as a reminder to reimplement later.
#if 0
	if (gGameOptions.nGameType > 0)
	{
		itemMain1.at24 = &menuNetStart;
		itemMain1.at28 = 2;
	}
	else
	{
		itemMain1.at24 = &menuEpisode;
		itemMain1.at28 = -1;
	}
	if (gGameOptions.nGameType > 0)
	{
		itemMainSave1.at24 = &menuNetStart;
		itemMainSave1.at28 = 2;
	}
	else
	{
		itemMainSave1.at24 = &menuEpisode;
		itemMainSave1.at28 = -1;
	}
#endif
}

void MenuSetupEpisodeInfo(void)
{
#if 0
	memset(zEpisodeNames, 0, sizeof(zEpisodeNames));
	memset(zLevelNames, 0, sizeof(zLevelNames));
	for (int i = 0; i < 6; i++)
	{
		if (i < gEpisodeCount)
		{
			EPISODEINFO* pEpisode = &gEpisodeInfo[i];
			zEpisodeNames[i] = pEpisode->at0;
			for (int j = 0; j < 16; j++)
			{
				if (j < pEpisode->nLevels)
				{
					zLevelNames[i][j] = pEpisode->at28[j].at90;
				}
			}
		}
	}
#endif
}


FSavegameInfo GameInterface::GetSaveSig()
{
	return { SAVESIG_BLD, MINSAVEVER_BLD, SAVEVER_BLD };
}


END_BLD_NS
