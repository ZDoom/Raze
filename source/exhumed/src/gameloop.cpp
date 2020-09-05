//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "ns.h"
#include "compat.h"
#include "common.h"
#include "engine.h"
#include "exhumed.h"
#include "sequence.h"
#include "names.h"
#include "player.h"
#include "ps_input.h"
#include "sound.h"
#include "view.h"
#include "status.h"
#include "version.h"
#include "aistuff.h"
#include "mapinfo.h"
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include "gamecvars.h"
#include "savegamehelp.h"
#include "c_dispatch.h"
#include "raze_sound.h"
#include "gamestate.h"
#include "screenjob.h"
#include "c_console.h"
#include "cheathandler.h"
#include "statistics.h"
#include "g_input.h"
#include "core/menu/menu.h"
#include "d_net.h"

BEGIN_PS_NS

short nBestLevel;

extern uint8_t nCinemaSeen;

void RunCinemaScene(int num);
void GameMove(void);
void DrawClock();
double calc_smoothratio();
void DoTitle(CompletionFunc completion);

static void FinishLevel(int lnum, TArray<JobDesc> &jobs)
{
    StopAllSounds();

    bCamera = false;
    automapMode = am_off;

    STAT_Update(lnum == kMap20);
    if (lnum != kMap20)
    {
        if (EndLevel == 13 && !netgame)
        {
            // There's really no choice but to enter an active wait loop here to make the sound play out.
            PlayLocalSound(StaticSound[59], 0, true, CHANF_UI);
            int nTicks = I_msTime() + 100;
            while (nTicks > I_msTime()) 
            { 
                I_GetEvent();
                soundEngine->UpdateSounds(I_GetTime());
            }
            Net_ClearFifo();
        }
    }
    else nPlayerLives[0] = 0;

    DoAfterCinemaScene(lnum, jobs);
}


static void showmap(short nLevel, short nLevelNew, short nLevelBest, TArray<JobDesc> &jobs)
{
    if (nLevelNew == 5 && !(nCinemaSeen & 1)) {
        nCinemaSeen |= 1;
        DoBeforeCinemaScene(5, jobs);
    }

    menu_DrawTheMap(nLevel, nLevelNew, nLevelBest, jobs);

    if (nLevelNew == 11 && !(nCinemaSeen & 2)) {
        DoBeforeCinemaScene(11, jobs);
    }
}


void GameInterface::Render()
{
    CheckKeys2();
    drawtime.Reset();
    drawtime.Clock();

    if (currentLevel && currentLevel->levelNumber == kMap20)
    {
        DoEnergyTile();
        DrawClock();
    }

    DrawView(calc_smoothratio());
    DrawStatusBar();
    if (paused && !M_Active())
    {
        auto tex = GStrings("TXTB_PAUSED");
		int nStringWidth = SmallFont->StringWidth(tex);
		DrawText(twod, SmallFont, CR_UNTRANSLATED, 160 - nStringWidth / 2, 100, tex, DTA_FullscreenScale, FSMode_Fit320x200, TAG_DONE);
    }

    drawtime.Unclock();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::DrawBackground()
{
    auto nLogoTile = EXHUMED ? kExhumedLogo : kPowerslaveLogo;
    int dword_9AB5F = (I_GetBuildTime() / 16) & 3;

    twod->ClearScreen();

    DrawRel(kSkullHead, 160, 100, 32);
    DrawRel(kSkullJaw, 161, 130, 32);
    DrawRel(nLogoTile, 160, 40, 32);

    // draw the fire urn/lamp thingies
    DrawRel(kTile3512 + dword_9AB5F, 50, 150, 32);
    DrawRel(kTile3512 + ((dword_9AB5F + 2) & 3), 270, 150, 32);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void Intermission(MapRecord *from_map, MapRecord *to_map)
{
    TArray<JobDesc> jobs;

	if (to_map && to_map->levelNumber != 0)
	{
		nBestLevel = to_map->levelNumber - 1;
		FinishLevel(to_map->levelNumber, jobs);
	}
	
	if (to_map->levelNumber > -1 && to_map->levelNumber < kMap20)
	{
		// start a new game at the given level
		if (!nNetPlayerCount && to_map->levelNumber > 0)
		{
			showmap(from_map? from_map->levelNumber : -1, to_map->levelNumber, nBestLevel, jobs);
		}
		else
			jobs.Push({ Create<DScreenJob>() }); // we need something in here even in the multiplayer case.
	}
	if (jobs.Size() > 0)
	{
		RunScreenJob(jobs.Data(), jobs.Size(), [=](bool)
		{
			if (!to_map) gameaction = ga_startup; // this was the end of the game
			else
			{
				if (to_map->levelNumber != selectedlevelnew)
				{
					// User can switch destination on the scrolling map.
					g_nextmap = FindMapByLevelNum(selectedlevelnew);
					STAT_Cancel();
				}
				gameaction = ga_nextlevel;

			}
		});
	}
	
}

void GameInterface::NextLevel(MapRecord *map, int skill)
{
	InitLevel(map->levelNumber);

	if (map->levelNumber > nBestLevel)
	{
		nBestLevel = selectedlevelnew;
	}
	
	if (map->levelNumber == 11) nCinemaSeen |= 2;
	STAT_NewLevel(currentLevel->labelName);
	
}

void GameInterface::NewGame(MapRecord *map, int skill)
{
	// start a new game on the given level
	InitNewGame();
	if (map->levelNumber == 1) STAT_StartNewGame("Exhumed", 1);
	Intermission(nullptr, map);
}

void GameInterface::LevelCompleted(MapRecord *map, int skill)
{
	if (currentLevel->levelNumber == 0) gameaction = ga_mainmenu;
	else Intermission(currentLevel, map);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::Startup()
{
    resettiming();
    EndLevel = 0;

    if (userConfig.CommandMap.IsNotEmpty())
    {
        /*
        auto map = FindMapByName(userConfig.CommandMap);
        if (map) DeferedStartMap(map, 0);
        userConfig.CommandMap = "";
        goto again;
        */
    }
    else
    {
        if (!userConfig.nologo) DoTitle([](bool) { gameaction = ga_mainmenu; });
        else gameaction = ga_mainmenu;
    }

}

void GameInterface::ErrorCleanup()
{
    // Clear all progression sensitive variables here.
    EndLevel = 0;
}

END_PS_NS
