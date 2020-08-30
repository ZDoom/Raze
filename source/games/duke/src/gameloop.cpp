//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020 - Christoph Oelckers

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
aint with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//------------------------------------------------------------------------- 

#include "ns.h"	// Must come before everything else!

#include "gamestate.h"
#include "duke3d.h"
#include "sbar.h"
#include "m_argv.h"
#include "mapinfo.h"
#include "texturemanager.h"
#include "glbackend/glbackend.h"

BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::Ticker()
{
	// Make copies so that the originals do not have to be modified.
	for (int i = 0; i < MAXPLAYERS; i++)
	{
		sync[i] = playercmds[i].ucmd;
	}
	ud.camerasprite = -1;

	if (earthquaketime > 0) earthquaketime--;
	if (rtsplaying > 0) rtsplaying--;

	if (show_shareware > 0)
	{
		show_shareware--;
	}

	everyothertime++;
	updateinterpolations();

	if (playrunning())
	{
		global_random = krand();
		movedummyplayers();//ST 13
		r_NoInterpolate = false;
	}
	else r_NoInterpolate = true;

	for (int i = connecthead; i >= 0; i = connectpoint2[i])
	{
		if (playrunning())
		{
			auto p = &ps[i];
			if (p->pals.a > 0)
				p->pals.a--;

			hud_input(i);
			fi.processinput(i);
			fi.checksectors(i);
		}
	}

	if (playrunning())
	{
		if (levelTextTime > 0)
			levelTextTime--;

		fi.think();
	}

	if ((everyothertime & 1) == 0)
	{
		fi.animatewalls();
		movecyclers();
	}

	if (isRR() && ud.recstat == 0 && ud.multimode < 2)
		dotorch();

	if (ps[myconnectindex].gm & (MODE_EOL | MODE_RESTART))
	{
		exitlevel();
	}
 	nonsharedkeys();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void resetGameClock()
{
	I_SetFrameTime();
	gameclockstart = I_GetBuildTime();
	gameclock = 0;
	cloudclock = 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::Startup()
{
	resetGameClock();
		ps[myconnectindex].ftq = 0;

		if (userConfig.CommandMap.IsNotEmpty())
		{
			auto maprecord = FindMapByName(userConfig.CommandMap);
			userConfig.CommandMap = "";
			if (maprecord)
			{
				ud.m_respawn_monsters = ud.m_player_skill == 4;

				for (int i = 0; i != -1; i = connectpoint2[i])
				{
					resetweapons(i);
					resetinventory(i);
				}
				startnewgame(maprecord, /*userConfig.skill*/2);
			}
		}
		else
		{
			fi.ShowLogo([](bool) { startmainmenu(); });
		}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::Render()
{
	drawtime.Reset();
	drawtime.Clock();
	videoSetBrightness(thunder_brightness);
	double const smoothRatio = playrunning() ? I_GetTimeFrac() * MaxSmoothRatio : MaxSmoothRatio;
	displayrooms(screenpeek, smoothRatio);
	drawoverlays(smoothRatio);
	drawtime.Unclock();
}


END_DUKE_NS

