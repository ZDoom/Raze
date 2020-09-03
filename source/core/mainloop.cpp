/*
** mainloop.cpp
** Implements the main game loop
**
**---------------------------------------------------------------------------
** Copyright 2020 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/


// For G_Ticker and TryRunTics the following applies:
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright 1999-2016 Randy Heit
// Copyright 2002-2020 Christoph Oelckers
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//		DOOM Network game communication and protocol,
//		all OS independent parts.
//
//-----------------------------------------------------------------------------


#include <chrono>
#include <thread>
#include "c_cvars.h"
#include "i_time.h"
#include "d_net.h"
#include "gamecontrol.h"
#include "c_console.h"
#include "menu.h"
#include "i_system.h"
#include "raze_sound.h"
#include "raze_music.h"
#include "vm.h"
#include "gamestate.h"
#include "screenjob.h"
#include "mmulti.h"
#include "c_console.h"
#include "menu.h"
#include "uiinput.h"
#include "v_video.h"
#include "glbackend/glbackend.h"
#include "palette.h"
#include "build.h"
#include "g_input.h"

CVAR(Bool, vid_activeinbackground, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(Bool, r_ticstability, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(Bool, cl_capfps, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

ticcmd_t playercmds[MAXPLAYERS];

static uint64_t stabilityticduration = 0;
static uint64_t stabilitystarttime = 0;

bool pauseext;
bool r_NoInterpolate;
int entertic;
int oldentertics;
int gametic;


//==========================================================================
//
// 
//
//==========================================================================

void G_BuildTiccmd(ticcmd_t* cmd) 
{
	I_GetEvent();
	gi->GetInput(&cmd->ucmd);
	cmd->consistancy = consistancy[myconnectindex][(maketic / ticdup) % BACKUPTICS];
}

//==========================================================================
//
//
//
//==========================================================================

static void GameTicker()
{
	int i;

	handleevents();

#if 0
	// Todo: Migrate state changes to here instead of doing them ad-hoc
	while (gameaction != ga_nothing)
	{
		switch (gameaction)
		{
		}
		C_AdjustBottom();
	}
#endif

	// get commands, check consistancy, and build new consistancy check
	int buf = (gametic / ticdup) % BACKUPTICS;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			ticcmd_t* cmd = &playercmds[i];
			ticcmd_t* newcmd = &netcmds[i][buf];

			if ((gametic % ticdup) == 0)
			{
				RunNetSpecs(i, buf);
			}
#if 0
			if (demorecording)
			{
				G_WriteDemoTiccmd(newcmd, i, buf);
			}
			if (demoplayback)
			{
				G_ReadDemoTiccmd(cmd, i);
			}
			else
#endif
			{
				*cmd = *newcmd;
			}


			if (netgame && /*!demoplayback &&*/ (gametic % ticdup) == 0)
			{
#if 0
				//players[i].inconsistant = 0;
				if (gametic > BACKUPTICS * ticdup && consistancy[i][buf] != cmd->consistancy)
				{
					players[i].inconsistant = gametic - BACKUPTICS * ticdup;
				}
#endif
				consistancy[i][buf] = gi->GetPlayerChecksum(i);
			}
		}
	}

	C_RunDelayedCommands();
	updatePauseStatus();

	switch (gamestate)
	{
	default:
	case GS_STARTUP:
		artClearMapArt();
		gi->Startup();
		break;

	case GS_LEVEL:
		gameupdatetime.Reset();
		gameupdatetime.Clock();
		gi->Ticker();
		gameupdatetime.Unclock();
		break;

	case GS_MENUSCREEN:
	case GS_FULLCONSOLE:
	case GS_INTERMISSION:
	case GS_INTRO:
		// These elements do not tick at game rate.
		break;

	}
}


//==========================================================================
//
// Display
//
//==========================================================================

void Display()
{
	if (screen == nullptr || (!AppActive && (screen->IsFullscreen() || !vid_activeinbackground)))
	{
		return;
	}

	screen->FrameTime = I_msTimeFS();
	screen->BeginFrame();
	twodpsp.ClearClipRect();
	twod->ClearClipRect();
	switch (gamestate)
	{
	case GS_MENUSCREEN:
		gi->DrawBackground();
		break;

	case GS_INTRO:
	case GS_INTERMISSION:
		// screen jobs are not bound by the game ticker so they need to be ticked in the display loop.
		RunScreenJobFrame();
		break;

	case GS_LEVEL:
		if (gametic != 0)
		{
			screen->FrameTime = I_msTimeFS();
			screen->BeginFrame();
			screen->SetSceneRenderTarget(gl_ssao != 0);
			twodpsp.Clear();
			twod->Clear();
			twod->SetSize(screen->GetWidth(), screen->GetHeight());
			twodpsp.SetSize(screen->GetWidth(), screen->GetHeight());
			gi->Render();
			DrawFullscreenBlends();
		}
		break;

	default:
		break;
	}

	NetUpdate();			// send out any new accumulation

	if (gamestate != GS_INTRO) // do not draw overlays on the intros
	{
		// Draw overlay elements
		CT_Drawer();
		C_DrawConsole();
		M_Drawer();
		FStat::PrintStat(twod);
		DrawRateStuff();
	}

	videoShowFrame(1);
}

//==========================================================================
//
// Forces playsim processing time to be consistent across frames.
// This improves interpolation for frames in between tics.
//
// With this cvar off the mods with a high playsim processing time will appear
// less smooth as the measured time used for interpolation will vary.
//
//==========================================================================

static void TicStabilityWait()
{
	using namespace std::chrono;
	using namespace std::this_thread;

	if (!r_ticstability)
		return;

	uint64_t start = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
	while (true)
	{
		uint64_t cur = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
		if (cur - start > stabilityticduration)
			break;
	}
}

static void TicStabilityBegin()
{
	using namespace std::chrono;
	stabilitystarttime = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
}

static void TicStabilityEnd()
{
	using namespace std::chrono;
	uint64_t stabilityendtime = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
	stabilityticduration = std::min(stabilityendtime - stabilitystarttime, (uint64_t)1'000'000);
}

//==========================================================================
//
// The most important function in the engine.
//
//==========================================================================

void TryRunTics (void)
{
	int 		i;
	int 		lowtic;
	int 		realtics;
	int 		availabletics;
	int 		counts;
	int 		numplaying;

	// If paused, do not eat more CPU time than we need, because it
	// will all be wasted anyway.
	bool doWait = cl_capfps || pauseext || (r_NoInterpolate && !M_IsAnimated());

	// get real tics
	if (doWait && gamestate != GS_INTERMISSION)	// GS_INTERMISSION needs uncapped frame rate.
	{
		entertic = I_WaitForTic (oldentertics);
	}
	else
	{
		entertic = I_GetTime ();
	}
	realtics = entertic - oldentertics;
	oldentertics = entertic;

	// get available tics
	NetUpdate ();

	if (pauseext)
		return;

	lowtic = INT_MAX;
	numplaying = 0;
	for (i = 0; i < doomcom.numnodes; i++)
	{
		if (nodeingame[i])
		{
			numplaying++;
			if (nettics[i] < lowtic)
				lowtic = nettics[i];
		}
	}

	availabletics = lowtic - gametic / ticdup;

	// decide how many tics to run
	if (realtics < availabletics-1)
		counts = realtics+1;
	else if (realtics < availabletics)
		counts = realtics;
	else
		counts = availabletics;
	
	// Uncapped framerate needs seprate checks
	if (counts == 0 && !doWait)
	{
		TicStabilityWait();

		// Check possible stall conditions
		Net_CheckLastReceived(counts);
		if (realtics >= 1)
		{
			C_Ticker();
			M_Ticker();
			// Repredict the player for new buffered movement
#if 0
			gi->Unpredict();
			gi->Predict(myconnectindex);
#endif
		}
		if (!cl_syncinput)
		{
			I_GetEvent();
			gi->GetInput(nullptr);
		}
		return;
	}

	if (counts < 1)
		counts = 1;

	// wait for new tics if needed
	while (lowtic < gametic + counts)
	{
		NetUpdate ();
		lowtic = INT_MAX;

		for (i = 0; i < doomcom.numnodes; i++)
			if (nodeingame[i] && nettics[i] < lowtic)
				lowtic = nettics[i];

		lowtic = lowtic * ticdup;

		if (lowtic < gametic)
			I_Error ("TryRunTics: lowtic < gametic");

		// Check possible stall conditions
		Net_CheckLastReceived (counts);

		// Update time returned by I_GetTime, but only if we are stuck in this loop
		if (lowtic < gametic + counts)
			I_SetFrameTime();

		// don't stay in here forever -- give the menu a chance to work
		if (I_GetTime () - entertic >= 1)
		{
			C_Ticker ();
			M_Ticker ();
			// Repredict the player for new buffered movement
#if 0
			gi->Unpredict();
			gi->Predict(myconnectindex);
#endif
			return;
		}
	}

	//Tic lowtic is high enough to process this gametic. Clear all possible waiting info
	hadlate = false;
#if 0
	for (i = 0; i < MAXPLAYERS; i++)
		players[i].waiting = false;
#endif
	lastglobalrecvtime = I_GetTime (); //Update the last time the game tic'd over

	// run the count tics
	if (counts > 0)
	{
#if 0
		gi->Unpredict();
#endif
		while (counts--)
		{
			TicStabilityBegin();
			if (gametic > lowtic)
			{
				I_Error ("gametic>lowtic");
			}
#if 0
			if (advancedemo)
			{
				D_DoAdvanceDemo ();
			}
#endif
			C_Ticker ();
			M_Ticker ();
			GameTicker();
			gametic++;

			NetUpdate ();	// check for new console commands
			TicStabilityEnd();
		}
#if 0
		gi->Predict(myconnectindex);
#endif
		gi->UpdateSounds();
		soundEngine->UpdateSounds(I_GetTime());
	}
	else
	{
		TicStabilityWait();
	}
}


//==========================================================================
//
// MainLoop - will never return aside from exceptions being thrown.
//
//==========================================================================

void MainLoop ()
{
	int lasttic = 0;

	// Clamp the timer to TICRATE until the playloop has been entered.
	r_NoInterpolate = true;

	for (;;)
	{
		try
		{
			// frame syncronous IO operations
			if (gametic > lasttic)
			{
				lasttic = gametic;
				I_StartFrame ();
			}
			I_SetFrameTime();

			TryRunTics (); // will run at least one tic
			// Update display, next frame, with current state.
			I_StartTic();

			Display();
			Mus_UpdateMusic();		// must be at the end.
		}
		catch (CRecoverableError &error)
		{
			if (error.GetMessage ())
			{
				Printf (PRINT_BOLD, "\n%s\n", error.GetMessage());
			}
			gi->ErrorCleanup();
			C_FullConsole();
		}
		catch (CVMAbortException &error)
		{
			error.MaybePrintMessage();
			Printf("%s", error.stacktrace.GetChars());
			gi->ErrorCleanup();
			C_FullConsole();
		}
	}
}

