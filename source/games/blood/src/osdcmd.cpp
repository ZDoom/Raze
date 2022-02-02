//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) 2020 Raze developers and contributors

This file was part of NBlood.

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

#include "blood.h"
#include "mapinfo.h"
#include "gamestate.h"

BEGIN_BLD_NS

void GameInterface::WarpToCoords(int x, int y, int z, int ang, int horz)
{
	PLAYER* pPlayer = &gPlayer[myconnectindex];
	VIEW* pView = &gPrevView[myconnectindex];

	pPlayer->actor->copyXY(gView->actor);
	pView->x = gView->actor->int_pos().X;
	pView->y = gView->actor->int_pos().Y;
	pPlayer->zView = pView->viewz = gView->zView = z;

	if (ang != INT_MIN)
	{
		pPlayer->angle.oang = pPlayer->angle.ang = pView->angle = gView->angle.ang = buildang(ang);
	}

	if (horz != INT_MIN)
	{
		pPlayer->horizon.ohoriz = pPlayer->horizon.horiz = pView->horiz = gView->horizon.horiz = buildhoriz(horz);
	}
}

void GameInterface::ToggleThirdPerson()
{
	if (gamestate != GS_LEVEL) return;
	if (gViewPos > VIEWPOS_0)
		gViewPos = VIEWPOS_0;
	else
		gViewPos = VIEWPOS_1;
}

void GameInterface::SwitchCoopView()
{
	if (gamestate != GS_LEVEL) return;
	if (gGameOptions.nGameType == 1)
	{
		gViewIndex = connectpoint2[gViewIndex];
		if (gViewIndex == -1)
			gViewIndex = connecthead;
		gView = &gPlayer[gViewIndex];
	}
	else if (gGameOptions.nGameType == 3)
	{
		int oldViewIndex = gViewIndex;
		do
		{
			gViewIndex = connectpoint2[gViewIndex];
			if (gViewIndex == -1)
				gViewIndex = connecthead;
			if (oldViewIndex == gViewIndex || gMe->teamId == gPlayer[gViewIndex].teamId)
				break;
		} while (oldViewIndex != gViewIndex);
		gView = &gPlayer[gViewIndex];
	}
}

void GameInterface::ToggleShowWeapon()
{
	if (gamestate != GS_LEVEL) return;
	cl_showweapon = (cl_showweapon + 1) & 3;
}

END_BLD_NS
