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
		auto oldactions = ps[i].sync.actions;
		ps[i].sync = playercmds[i].ucmd;
		if (oldactions & SB_CENTERVIEW) ps[i].sync.actions |= SB_CENTERVIEW;
	}
	if (rtsplaying > 0) rtsplaying--;

	if (show_shareware > 0)
	{
		show_shareware--;
	}

	updateinterpolations();

	if (playrunning())
	{
		if (earthquaketime > 0) earthquaketime--;

		ud.camerasprite = -1;
		everyothertime++;

		global_random = krand();
		movedummyplayers();//ST 13
		
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
		
		fi.think();

		if ((everyothertime & 1) == 0)
		{
			fi.animatewalls();
			movecyclers();
		}

		if (isRR() && ud.recstat == 0 && ud.multimode < 2)
			dotorch();

		r_NoInterpolate = false;
		ud.levelclock+= 4;		// This must be at the end of this block so that the first tic receives a value of 0!
		if (ud.levelclock == 8) gameaction = ga_autosave;	// let the game run for 1 frame before saving.

	}
	else r_NoInterpolate = true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::Startup()
{
		ps[myconnectindex].ftq = 0;

		if (userConfig.CommandMap.IsNotEmpty())
		{
			auto maprecord = FindMapByName(userConfig.CommandMap);
			userConfig.CommandMap = "";
			if (maprecord)
			{
				ud.m_respawn_monsters = ud.player_skill == 4;

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
			if (!userConfig.nologo) fi.ShowLogo([](bool) { gameaction = ga_mainmenu; });
			else gameaction = ga_mainmenu;
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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------
void loadscreen_d(MapRecord* rec, CompletionFunc func);
void loadscreen_r(MapRecord* rec, CompletionFunc func);

void GameInterface::NextLevel(MapRecord* map, int skill)
{
	if (skill != -1) ud.player_skill = skill + 1;

#if 0
	// Loading is so fast on modern system so that this only serves as an irritant, not an asset.
	auto loadscreen = isRR() ? loadscreen_r : loadscreen_d;
	loadscreen_d(map, [=](bool)
		{
			enterlevel(map, 0);
			gameaction = ga_level;
		});
#endif
	enterlevel(map, 0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::NewGame(MapRecord* map, int skill)
{
	// Hmm... What about the other players?
	ps[0].last_extra = max_player_health;
	resetweapons(0);
	resetinventory(0);

	startnewgame(map, skill);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::LevelCompleted(MapRecord* map, int skill)
{
	exitlevel(map);
}

END_DUKE_NS

