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
#include "razemenu.h"
#include "i_system.h"
#include "raze_sound.h"
#include "raze_music.h"
#include "vm.h"
#include "gamestate.h"
#include "screenjob_.h"
#include "c_console.h"
#include "uiinput.h"
#include "v_video.h"
#include "palette.h"
#include "build.h"
#include "mapinfo.h"
#include "automap.h"
#include "statusbar.h"
#include "gamestruct.h"
#include "savegamehelp.h"
#include "v_draw.h"
#include "gamehud.h"
#include "wipe.h"
#include "i_interface.h"
#include "texinfo.h"
#include "texturemanager.h"
#include "gameinput.h"

CVAR(Bool, vid_activeinbackground, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(Bool, vid_lowerinbackground, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
CVAR(Bool, r_ticstability, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(Bool, vid_dontdowait, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
EXTERN_CVAR(Bool, cl_capfps)
CVAR(Bool, cl_resumesavegame, true, CVAR_ARCHIVE)
EXTERN_CVAR (Bool, vid_vsync)
EXTERN_CVAR (Int, vid_maxfps)

static uint64_t stabilityticduration = 0;
static uint64_t stabilitystarttime = 0;

DCorePlayer* PlayerArray[MAXPLAYERS];

IMPLEMENT_CLASS(DCorePlayer, true, true)
IMPLEMENT_POINTERS_START(DCorePlayer)
IMPLEMENT_POINTER(actor)
IMPLEMENT_POINTERS_END

void MarkPlayers()
{
	GC::MarkArray(PlayerArray, MAXPLAYERS);
}

bool r_NoInterpolate;
int entertic;
int oldentertics;
int gametic;
int nextwipe = wipe_None;

FString savename;
FString BackupSaveGame;

void DoLoadGame(const char* name);

bool sendsave;
FString	savedescription;
FString	savegamefile;

//==========================================================================
//
// 
//
//==========================================================================

void G_BuildTiccmd(ticcmd_t* cmd) 
{
	if (sendsave)
	{
		sendsave = false;
		Net_WriteByte(DEM_SAVEGAME);
		Net_WriteString(savegamefile.GetChars());
		Net_WriteString(savedescription.GetChars());
		savegamefile = "";
	}
	cmd->ucmd = {};
	gameInput.getInput(&cmd->ucmd);
	cmd->consistency = consistency[myconnectindex][(maketic / ticdup) % BACKUPTICS];
}

//==========================================================================
//
//
//
//==========================================================================
bool newGameStarted;

void NewGame(MapRecord* map, int skill, bool ns = false)
{
	gi->FreeLevelData();
	newGameStarted = true;
	ShowIntermission(nullptr, map, nullptr, [=](bool) { 
		gi->NewGame(map, skill, ns); 
		gameaction = ga_level;
		ResetStatusBar();
		gameInput.resetCrouchToggle();
		});
}

//==========================================================================
//
//
//
//==========================================================================

static void GameTicker()
{
	handleevents();

	// Todo: Migrate state changes to here instead of doing them ad-hoc
	while (gameaction != ga_nothing)
	{
		auto ga = gameaction;
		gameaction = ga_nothing;
		switch (ga)
		{
		case ga_autoloadgame:
			C_FlushDisplay();
			if (BackupSaveGame.IsNotEmpty() && cl_resumesavegame)
			{
				DoLoadGame(BackupSaveGame.GetChars());
			}
			else
			{
				g_nextmap = currentLevel;
				FX_StopAllSounds();
				S_SetReverb(0);
				NewGame(g_nextmap, -1);
				BackupSaveGame = "";
			}
			break;

		case ga_completed:
			FX_StopAllSounds();
			S_SetReverb(0);
			gi->LevelCompleted(g_nextmap, g_nextskill);
			break;

		case ga_nextlevel:
			gi->FreeLevelData();
			gameaction = ga_level;
			gi->NextLevel(g_nextmap, g_nextskill);
			ResetStatusBar();
			if (!isBlood()) M_Autosave();
			break;

		case ga_newgame:
			FX_StopAllSounds();
			[[fallthrough]];
		case ga_newgamenostopsound:
			DeleteScreenJob();
			S_SetReverb(0);
			C_FlushDisplay();
			BackupSaveGame = "";
			NewGame(g_nextmap, g_nextskill, ga == ga_newgamenostopsound);
			break;

		case ga_startup:
			Mus_Stop();
			FX_StopAllSounds();
			gi->FreeLevelData();
			gamestate = GS_STARTUP;
			break;

		case ga_mainmenu:
			FX_StopAllSounds();
			if (isBlood()) Mus_Stop();
			[[fallthrough]];
		case ga_mainmenunostopsound:
			gi->FreeLevelData();
			gamestate = GS_MENUSCREEN;
			M_StartControlPanel(ga == ga_mainmenu);
			M_SetMenu(NAME_Mainmenu);
			break;

		case ga_creditsmenu:
			FX_StopAllSounds();
			gi->FreeLevelData();
			gamestate = GS_MENUSCREEN;
			M_StartControlPanel(false);
			M_SetMenu(NAME_Mainmenu);
			M_SetMenu(NAME_CreditsMenu);
			break;

		case ga_savegame:
			G_DoSaveGame(true, false, savegamefile.GetChars(), savedescription.GetChars());
			gameaction = ga_nothing;
			savegamefile = "";
			savedescription = "";
			break;

		case ga_loadgame:
		case ga_loadgamehidecon:
		//case ga_autoloadgame:
			G_DoLoadGame();
			break;

		case ga_autosave:
			if (gamestate == GS_LEVEL && !newGameStarted) M_Autosave();
			newGameStarted = false;
			break;

		case ga_level:
			Net_ClearFifo();
			inputState.ClearAllInput();
			gameInput.Clear();
			gamestate = GS_LEVEL;
			return;

		case ga_intro:
			gamestate = GS_INTRO;
			break;

		case ga_intermission:
			gamestate = GS_CUTSCENE;
			break;

		case ga_fullconsole:
			C_FullConsole();
			Mus_Stop();
			gameaction = ga_nothing;
			break;

		case ga_endscreenjob:
			EndScreenJob();
			break;

			// for later
		// case ga_recordgame,			// start a new demo recording (later)
		// case ga_loadgameplaydemo,	// load a savegame and play a demo.

		default:
			break;
		}
		C_AdjustBottom();
	}

	// get commands, check consistancy, and build new consistancy check
	int buf = (gametic / ticdup) % BACKUPTICS;

	for (int i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			ticcmd_t* cmd = &PlayerArray[i]->cmd;
			ticcmd_t* newcmd = &netcmds[i][buf];
			PlayerArray[i]->lastcmd = *cmd;
			PlayerArray[i]->resetCameraAngles();

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
				consistency[i][buf] = gi->GetPlayerChecksum(i);
			}
		}
	}

	C_RunDelayedCommands();
	updatePauseStatus();

	switch (gamestate)
	{
	default:
	case GS_STARTUP:
		gi->Startup();
		break;

	case GS_LEVEL:
		gameupdatetime.Reset();
		gameupdatetime.Clock();
		gameInput.ResetInputSync();
		gi->Ticker();
		TickStatusBar();
		levelTextTime--;
		gameupdatetime.Unclock();
		break;

	case GS_MENUSCREEN:
	case GS_FULLCONSOLE:
		break;
	case GS_CUTSCENE:
	case GS_INTRO:
		if (ScreenJobTick())
		{
			// synchronize termination with the playsim.
			Net_WriteByte(DEM_ENDSCREENJOB);
		}
		break;

	}
}


void DrawOverlays()
{
	NetUpdate();			// send out any new accumulation

	if (gamestate != GS_INTRO) // do not draw overlays on the intros
	{
		// Draw overlay elements
		CT_Drawer();
		C_DrawConsole();
		M_Drawer();
		FStat::PrintStat(twod);
	}
	DrawRateStuff();
}

//==========================================================================
//
// Display
//
//==========================================================================
CVAR(String, drawtile, "", 0)	// debug stuff. Draws the tile with the given number on top of thze HUD

void Display()
{
	if (screen == nullptr || (!AppActive && (screen->IsFullscreen() || !vid_activeinbackground)))
	{
		return;
	}
	
	FTexture* wipestart = nullptr;
	if (nextwipe != wipe_None)
	{
		wipestart = screen->WipeStartScreen();
	}

	screen->FrameTime = I_msTimeFS();
	tileUpdateAnimations();
	screen->BeginFrame();
	twodpsp.Clear();
	twodpsp.SetSize(screen->GetWidth(), screen->GetHeight());
	twodpsp.ClearClipRect();
	twod->Clear();
	//twod->SetSize(screen->GetWidth(), screen->GetHeight());
	twod->Begin(screen->GetWidth(), screen->GetHeight());
	twod->ClearClipRect();
	switch (gamestate)
	{
	case GS_MENUSCREEN:
	case GS_FULLCONSOLE:
		gi->DrawBackground();
		break;

	case GS_INTRO:
	case GS_CUTSCENE:
		ScreenJobDraw();
		break;

	case GS_LEVEL:
		if (gametic != 0)
		{
			screen->FrameTime = I_msTimeFS();
			screen->BeginFrame();
			screen->SetSceneRenderTarget(gl_ssao != 0);
			//updateModelInterpolation();
			gi->Render();
			DrawFullscreenBlends();
			drawMapTitle();
			break;
		}
		[[fallthrough]];

	default:
		twod->ClearScreen();
		break;
	}
	
	if (nextwipe == wipe_None)
	{
		DrawOverlays();
		if (drawtile[0])
		{
			auto tex = TexMan.CheckForTexture(drawtile, ETextureType::Any);
			if (!tex.isValid()) tex = tileGetTextureID(atoi(drawtile));
			if (tex.isValid())
			{
				auto tx = TexMan.GetGameTexture(tex, true);
				if (tx)
				{
					int width = (int)tx->GetDisplayWidth();
					int height = (int)tx->GetDisplayHeight();
					int dwidth, dheight;
					if (width > height)
					{
						dwidth = screen->GetWidth() / 4;
						dheight = height * dwidth / width;
					}
					else
					{
						dheight = screen->GetHeight() / 4;
						dwidth = width * dheight / height;
					}
					DrawTexture(twod, tx, 0, 0, DTA_DestWidth, dwidth, DTA_DestHeight, dheight, TAG_DONE);
				}
			}
		}
	}
	else
	{
		PerformWipe(wipestart, screen->WipeEndScreen(), nextwipe, true, DrawOverlays);
		nextwipe = wipe_None;
	}

	screen->Update();
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
	stabilityticduration = min(stabilityendtime - stabilitystarttime, (uint64_t)1'000'000);
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
	bool doWait = (cl_capfps || pauseext || (r_NoInterpolate && !M_IsAnimated() && gamestate != GS_CUTSCENE && gamestate != GS_INTRO));

	if (vid_dontdowait && ((vid_maxfps > 0) || (vid_vsync == true)))
		doWait = false;

	if (!AppActive && vid_lowerinbackground)
		doWait = true;

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
		if (!gameInput.SyncInput())
		{
			gameInput.getInput();
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

	if (userConfig.CommandMap.IsNotEmpty())
	{
		auto maprecord = FindMapByName(userConfig.CommandMap.GetChars());
		if (maprecord == nullptr)
		{
			maprecord = SetupUserMap(userConfig.CommandMap.GetChars(), g_gameType & GAMEFLAG_DUKE? "dethtoll.mid" : nullptr);
		}
		userConfig.CommandMap = "";
		if (maprecord)
		{
			DeferredStartGame(maprecord, g_nextskill);
		}
	}

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

			// update the scale factor for unsynchronised input here.
			gameInput.UpdateInputScale();

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
			M_ClearMenus();
			C_FullConsole();
			gameaction = ga_nothing;
		}
		catch (CVMAbortException &error)
		{
			error.MaybePrintMessage();
			Printf("%s", error.stacktrace.GetChars());
			gi->ErrorCleanup();
			twod->SetOffset(DVector2(0, 0));
			M_ClearMenus();
			C_FullConsole();
		}
	}
}

