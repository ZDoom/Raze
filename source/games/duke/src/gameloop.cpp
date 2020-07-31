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
#include "baselayer.h"
#include "m_argv.h"
#include "mapinfo.h"
#include "texturemanager.h"

BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// abstract the queue's implementation
// All access to the input queues should go through this function interface.
//
//---------------------------------------------------------------------------
static input_t inputfifo[MOVEFIFOSIZ][MAXPLAYERS];
static int movefifoend[MAXPLAYERS];
static int movefifoplc;
static int bufferjitter;

void clearfifo(void)
{
	loc = {};
	memset(&inputfifo, 0, sizeof(inputfifo));
	memset(sync, 0, sizeof(sync));
}


static inline void GetNextInput()
{
	for (int i = connecthead; i >= 0; i = connectpoint2[i])
		memcpy(&sync[i], &inputfifo[movefifoplc & (MOVEFIFOSIZ - 1)][i], sizeof(input_t));

	movefifoplc++;
}

static void advancequeue(int myconnectindex)
{
	movefifoend[myconnectindex]++;
}

static input_t& nextinput(int myconnectindex)
{
	return inputfifo[movefifoend[myconnectindex] & (MOVEFIFOSIZ - 1)][myconnectindex];
}

bool shouldprocessinput(int myconnectindex)
{
	if (movefifoend[myconnectindex] - movefifoplc > bufferjitter)
	{
		int i;
		for (i = connecthead; i >= 0; i = connectpoint2[i])
			if (movefifoplc == movefifoend[i]) return false;
		if (i >= 0) return false;
		return true;
	}
	return false;
}

static void fakedomovethings()
{
	// prediction
}

static void fakedomovethingscorrect()
{
	// unprediction
}

void prediction()
{
#if 0
	// We currently have no net code driving this.
	if (numplayers > 1)
		while (fakemovefifoplc < movefifoend[myconnectindex]) fakedomovethings();
	getpackets();
#endif
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------
/*
void mploadsave()
{
	for(int i=connecthead;i>=0;i=connectpoint2[i])
		if( sync[i].bits&(1<<17) )
	{
		multiflag = 2;
		multiwhat = (sync[i].bits>>18)&1;
		multipos = (unsigned) (sync[i].bits>>19)&15;
		multiwho = i;

		if( multiwhat )
		{
			saveplayer( multipos );
			multiflag = 0;

			if(multiwho != myconnectindex)
			{
				strcpy(&fta_quotes[122],&ud.user_name[multiwho][0]);
				strcat(&fta_quotes[122]," SAVED A MULTIPLAYER GAME");
				FTA(122,&ps[myconnectindex]);
			}
			else
			{
				strcpy(&fta_quotes[122],"MULTIPLAYER GAME SAVED");
				FTA(122,&ps[myconnectindex]);
			}
			break;
		}
		else
		{
//            waitforeverybody();

			j = loadplayer( multipos );

			multiflag = 0;

			if(j == 0 && !isRR())
			{
				if(multiwho != myconnectindex)
				{
					strcpy(&fta_quotes[122],&ud.user_name[multiwho][0]);
					strcat(&fta_quotes[122]," LOADED A MULTIPLAYER GAME");
					FTA(122,&ps[myconnectindex]);
				}
				else
				{
					strcpy(&fta_quotes[122],"MULTIPLAYER GAME LOADED");
					FTA(122,&ps[myconnectindex]);
				}
				return 1;
			}
		}
	}
}
*/

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int domovethings()
{
	int i, j;

	// mplpadsave();

	ud.camerasprite = -1;
	lockclock += TICSPERFRAME;

	if (earthquaketime > 0) earthquaketime--;
	if (rtsplaying > 0) rtsplaying--;

	if (show_shareware > 0)
	{
		show_shareware--;
	}

	everyothertime++;
	GetNextInput();
	updateinterpolations();

	j = -1;
	for (i = connecthead; i >= 0; i = connectpoint2[i])
	{
		if (PlayerInput(i, SKB_GAMEQUIT))
		{
			if (i == myconnectindex) gameexitfrommenu();
			if (screenpeek == i)
			{
				screenpeek = connectpoint2[i];
				if (screenpeek < 0) screenpeek = connecthead;
			}

			if (i == connecthead) connecthead = connectpoint2[connecthead];
			else connectpoint2[j] = connectpoint2[i];

			numplayers--;
			ud.multimode--;

			//closedemowrite();

			if (numplayers < 2 && !isRR())
				S_PlaySound(GENERIC_AMBIENCE17, CHAN_AUTO, CHANF_UI);

			Printf(PRINT_NOTIFY, "%s is history!", ud.user_name[i]);

			quickkill(&ps[i]);
			deletesprite(ps[i].i);
		}
		else j = i;
	}

	//if(ud.recstat == 1) record();

	if (playrunning())
	{
		global_random = krand();
		movedummyplayers();//ST 13
	}

	for (i = connecthead; i >= 0; i = connectpoint2[i])
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

	fakedomovethingscorrect();

	if ((everyothertime & 1) == 0)
	{
		fi.animatewalls();
		movecyclers();
	}

	if (isRR() && ud.recstat == 0 && ud.multimode < 2)
		dotorch();

	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int moveloop()
{
	prediction();

	if (numplayers < 2) bufferjitter = 0;
	while (shouldprocessinput(myconnectindex))
	{
		if( domovethings() ) return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool GameTicker()
{
	if (ps[myconnectindex].gm == MODE_DEMO)
	{
		M_ClearMenus();
		return true;
	}

	//Net_GetPackets();

	nonsharedkeys();

	C_RunDelayedCommands();

	gameupdatetime.Reset();
	gameupdatetime.Clock();

	while (playrunning() && (int)(totalclock - ototalclock) >= TICSPERFRAME)
	{
		ototalclock += TICSPERFRAME;

		GetInput();
		auto const pPlayer = &ps[myconnectindex];
		auto const q16ang = fix16_to_int(pPlayer->q16ang);
		auto& input = nextinput(myconnectindex);

		input = loc;
		input.fvel = mulscale9(loc.fvel, sintable[(q16ang + 2560) & 2047]) +
			mulscale9(loc.svel, sintable[(q16ang + 2048) & 2047]) +
			pPlayer->fric.x;
		input.svel = mulscale9(loc.fvel, sintable[(q16ang + 2048) & 2047]) +
			mulscale9(loc.svel, sintable[(q16ang + 1536) & 2047]) +
			pPlayer->fric.y;
		loc = {};

		advancequeue(myconnectindex);

		if (playrunning())
		{
			moveloop();
		}
	}
	if (!playrunning())
	{
		ototalclock = totalclock - 1;
	}
	int const smoothRatio = calc_smoothratio(totalclock, ototalclock);

	gameupdatetime.Unclock();

	if (ps[myconnectindex].gm & (MODE_EOL | MODE_RESTART))
	{
		exitlevel();
	}

	if (!cl_syncinput)
	{
		GetInput();
	}

	drawtime.Reset();
	drawtime.Clock();
	displayrooms(screenpeek, smoothRatio);
	displayrest(smoothRatio);
	drawtime.Unclock();

	return (ps[myconnectindex].gm & MODE_DEMO);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void startmainmenu()
{
	M_StartControlPanel(false);
	M_SetMenu(NAME_Mainmenu);
	FX_StopAllSounds();
	gamestate = GS_DEMOSCREEN;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void app_loop()
{
	gamestate = GS_STARTUP;
	enginecompatibility_mode = ENGINECOMPATIBILITY_19961112;//bVanilla;

	while (true)
	{
		try
		{
			handleevents();
			updatePauseStatus();
			D_ProcessEvents();
			switch (gamestate)
			{
			default:
			case GS_STARTUP:
				totalclock = 0;
				ototalclock = 0;
				lockclock = 0;

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
				break;

			case GS_DEMOSCREEN:
			case GS_FULLCONSOLE:
				drawbackground();
				break;

			case GS_LEVEL:
				if (GameTicker()) gamestate = GS_STARTUP;
				else videoSetBrightness(thunder_brightness);
				break;

			case GS_INTERMISSION:
			case GS_INTRO:
				RunScreenJobFrame();	// This handles continuation through its completion callback.
				break;

			}
			videoNextPage();
			videoSetBrightness(0);	// immediately reset this so that the value doesn't stick around in the backend.
		}
		catch (CRecoverableError& err)
		{
			C_FullConsole();
			Printf(TEXTCOLOR_RED "%s\n", err.what());
		}
	}
}


END_DUKE_NS

