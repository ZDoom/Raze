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

#include "screenjob_.h"
#include "gamestate.h"
#include "duke3d.h"
#include "m_argv.h"
#include "mapinfo.h"
#include "texturemanager.h"
#include "interpolate.h"

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

	UpdateInterpolations();

	if (playrunning())
	{
		if (earthquaketime > 0) earthquaketime--;

		ud.cameraactor = nullptr;
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
		PlayClock+= 4;		// This must be at the end of this block so that the first tic receives a value of 0!
		if (PlayClock == 8) gameaction = ga_autosave;	// let the game run for 1 frame before saving.

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
	PlayLogos(ga_mainmenunostopsound, ga_mainmenunostopsound, false);
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

void GameInterface::NextLevel(MapRecord* map, int skill)
{
	ud.player_skill = skill + 1;
	enterlevel(map, 0);
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

