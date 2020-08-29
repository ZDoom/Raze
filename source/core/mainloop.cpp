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


// For TryRunTics the following applies:
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



// Forces playsim processing time to be consistent across frames.
// This improves interpolation for frames in between tics.
//
// With this cvar off the mods with a high playsim processing time will appear
// less smooth as the measured time used for interpolation will vary.

CVAR(Bool, r_ticstability, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(Bool, cl_capfps, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

static uint64_t stabilityticduration = 0;
static uint64_t stabilitystarttime = 0;

bool pauseext;
bool r_NoInterpolate;
int entertic;
int oldentertics;
int gametic;


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

//
// TryRunTics
//
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
	if (pauseext) 
		r_NoInterpolate = true;
	bool doWait = cl_capfps || r_NoInterpolate /*|| netgame*/;

	// get real tics
	if (doWait)
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

	if (ticdup == 1)
	{
		availabletics = lowtic - gametic;
	}
	else
	{
		availabletics = lowtic - gametic / ticdup;
	}

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
			//if (debugfile) fprintf (debugfile, "run tic %d\n", gametic);
			C_Ticker ();
			M_Ticker ();
			//G_Ticker();
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
// D_DoomLoop
//
// Manages timing and IO, calls all ?_Responder, ?_Ticker, and ?_Drawer,
// calls I_GetTime, I_StartFrame, and I_StartTic
//
//==========================================================================

void MainLoop ()
{
	int lasttic = 0;

	// Clamp the timer to TICRATE until the playloop has been entered.
	r_NoInterpolate = true;

	//vid_cursor.Callback();

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
			I_StartTic ();
			gi->Render();
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


//---------------------------------------------------------------------------
//
// The one and only main loop in the entire engine. Yay!
//
//---------------------------------------------------------------------------
#if 0

void _TickSubsystems()
{
	// run these on an independent timer until we got something working for the games.
	static const uint64_t tickInterval = 1'000'000'000 / 30;
	static uint64_t nexttick = 0;

	auto nowtick = I_nsTime();
	if (nexttick == 0) nexttick = nowtick;
	int cnt = 0;
	while (nexttick <= nowtick && cnt < 5)
	{
		nexttick += tickInterval;
		C_Ticker();
		M_Ticker();
		C_RunDelayedCommands();
		cnt++;
	}
	// If this took too long the engine was most likely suspended so recalibrate the timer.
	// Perfect precision is not needed here.
	if (cnt == 5) nexttick = nowtick + tickInterval;
}

static void _updatePauseStatus()
{
	// This must go through the network in multiplayer games.
	if (M_Active() || System_WantGuiCapture())
	{
		paused = 1;
	}
	else if (!M_Active() || !System_WantGuiCapture())
	{
		if (!pausedWithKey)
		{
			paused = 0;
		}

		if (sendPause)
		{
			sendPause = false;
			paused = pausedWithKey ? 0 : 2;
			pausedWithKey = !!paused;
		}
	}

	paused ? S_PauseSound(!pausedWithKey, !paused) : S_ResumeSound(paused);
}


void _app_loop()
{
	gamestate = GS_STARTUP;

	while (true)
	{
		try
		{
			I_SetFrameTime();
			TickSubsystems();
			twod->SetSize(screen->GetWidth(), screen->GetHeight());
			twodpsp.SetSize(screen->GetWidth(), screen->GetHeight());

			handleevents();
			updatePauseStatus();
			D_ProcessEvents();

			gi->RunGameFrame();

			// Draw overlay elements to the 2D drawer
			FStat::PrintStat(twod);
			CT_Drawer();
			C_DrawConsole();
			M_Drawer();

			// Handle the final 2D overlays.
			if (gamestate == GS_LEVEL) DrawFullscreenBlends();
			DrawRateStuff();


			videoShowFrame(0);
			videoSetBrightness(0);	// immediately reset this so that the value doesn't stick around in the backend.
		}
		catch (CRecoverableError& err)
		{
			C_FullConsole();
			Printf(TEXTCOLOR_RED "%s\n", err.what());
		}
	}
}


#endif