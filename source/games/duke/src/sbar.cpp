//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2020 - Christoph Oelckers

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

*/
//------------------------------------------------------------------------- 
#include "ns.h"	// Must come before everything else!

#include "v_font.h"
#include "duke3d.h"
#include "v_draw.h"
#include "texturemanager.h"
#include "mapinfo.h"
#include "automap.h"

BEGIN_DUKE_NS

//==========================================================================
//
// 3D viewport size management
//
//==========================================================================

void DrawBorder()
{
	auto tex = tileGetTexture(TILE_SCREENBORDER);
	if (tex != nullptr && tex->isValid())
	{
		if (windowxy1.Y > 0)
		{
			twod->AddFlatFill(0, 0, twod->GetWidth(), windowxy1.Y, tex, false, 1);
		}
		if (windowxy2.Y + 1 < twod->GetHeight())
		{
			twod->AddFlatFill(0, windowxy2.Y + 1, twod->GetWidth(), twod->GetHeight(), tex, false, 1);
		}
		if (windowxy1.X > 0)
		{
			twod->AddFlatFill(0, windowxy1.Y, windowxy1.X, windowxy2.Y + 1, tex, false, 1);
		}
		if (windowxy2.X + 1 < twod->GetWidth())
		{
			twod->AddFlatFill(windowxy2.X + 1, windowxy1.Y, twod->GetWidth(), windowxy2.Y + 1, tex, false, 1);
		}
		auto vb = tileGetTexture(TILE_VIEWBORDER);
		auto ve = tileGetTexture(TILE_VIEWBORDER + 1);
		int x1 = windowxy1.X - 4;
		int y1 = windowxy1.Y - 4;
		int x2 = windowxy2.X + 5;
		int y2 = windowxy2.Y + 5;
		twod->AddFlatFill(x1, y1, x2, y1 + 4, vb, 5);
		twod->AddFlatFill(x1, y2 - 4, x2, y2, vb, 6);
		twod->AddFlatFill(x1, y1, x1 + 4, y2, vb, 1);
		twod->AddFlatFill(x2 - 4, y1, x2, y2, vb, 3);
		twod->AddFlatFill(x1, y1, x1 + 4, y1 + 4, ve, 1);
		twod->AddFlatFill(x2 - 4, y1, x2, y1 + 4, ve, 3);
		twod->AddFlatFill(x1, y2 - 4, x1 + 4, y2, ve, 2);
		twod->AddFlatFill(x2 - 4, y2 - 4, x2, y2, ve, 4);
	}
}

void DrawStatusBar()
{
	DrawBorder();

	SummaryInfo info{};

	info.kills = ps[0].actors_killed;
	info.maxkills = ps[0].max_actors_killed;
	info.secrets = ps[0].secret_rooms;
	info.maxsecrets = ps[0].max_secret_rooms;
	info.time = Scale(PlayClock, 1000, 120);
	UpdateStatusBar(&info);
}

//==========================================================================
//
// view sizing game interface
//
//==========================================================================

void GameInterface::PlayHudSound() 
{
	S_PlaySound(isRR() ? 341 : THUD, CHAN_AUTO, CHANF_UI);
}


END_DUKE_NS
