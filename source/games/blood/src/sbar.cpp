//-------------------------------------------------------------------------
/*
Copyright (C) 2020-2021 Christoph Oelckers

This file is part of Raze.

This is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

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

#include <stdlib.h>
#include <string.h>

#include "build.h"
#include "v_font.h"

#include "blood.h"
#include "zstring.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "v_font.h"
#include "statusbar.h"
#include "automap.h"
#include "v_draw.h"
#include "gamecvars.h"

CVARD(Bool, hud_powerupduration, true, CVAR_ARCHIVE/*|CVAR_FRONTEND_BLOOD*/, "enable/disable displaying the remaining seconds for power-ups")
CVAR(Bool, hud_ctf_vanilla, false, CVAR_ARCHIVE)

BEGIN_BLD_NS

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void UpdateFrame(void)
{
	auto tex = tileGetTexture(kBackTile);
	int width = twod->GetWidth();
	int height = twod->GetHeight();

	twod->AddFlatFill(0, 0, width, viewport3d.Top() - 3, tex);
	twod->AddFlatFill(0, viewport3d.Bottom() + 3, width, height, tex);
	twod->AddFlatFill(0, viewport3d.Top() - 3, viewport3d.Left() - 3, viewport3d.Bottom() + 3, tex);
	twod->AddFlatFill(viewport3d.Right() + 3, viewport3d.Top() - 3, width, viewport3d.Bottom() + 3, tex);

	twod->AddFlatFill(viewport3d.Left() - 3, viewport3d.Top() - 3, viewport3d.Left(), viewport3d.Bottom(), tex, 0, 1, 0xff545454);
	twod->AddFlatFill(viewport3d.Left(), viewport3d.Top() - 3, viewport3d.Right() + 3, viewport3d.Top(), tex, 0, 1, 0xff545454);
	twod->AddFlatFill(viewport3d.Right(), viewport3d.Top(), viewport3d.Right() + 3, viewport3d.Bottom() + 3, tex, 0, 1, 0xff2a2a2a);
	twod->AddFlatFill(viewport3d.Left() - 3, viewport3d.Bottom(), viewport3d.Right(), viewport3d.Bottom() + 3, tex, 0, 1, 0xff2a2a2a);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdateStatusBar()
{
	if (automapMode == am_off && hud_size <= Hud_Stbar)
	{
		UpdateFrame();
	}
	SummaryInfo sum;
	if (gGameOptions.nGameType == 3)
	{
		sum.kills = gView ? gView->fragCount : 0;
		sum.maxkills = -3;
	}
	else
	{
		sum.kills = gKillMgr.Kills;
		sum.maxkills = gKillMgr.TotalKills;
	}
	sum.secrets = gSecretMgr.Founds;
	sum.supersecrets = gSecretMgr.Super;
	sum.maxsecrets = max(gSecretMgr.Founds, gSecretMgr.Total); // If we found more than there are, increase the total. Some levels have a bugged counter.
	sum.time = Scale(PlayClock, 1000, 120);
	UpdateStatusBar(&sum);
}


END_BLD_NS
