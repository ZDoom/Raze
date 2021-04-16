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
#include "engine.h"
#include "exhumed.h"
#include "sequence.h"
#include "names.h"
#include "player.h"
#include "input.h"
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
#include "razemenu.h"
#include "d_net.h"
#include "automap.h"
#include "raze_music.h"
#include "v_draw.h"

BEGIN_PS_NS

short nBestLevel;

extern uint8_t nCinemaSeen;

void RunCinemaScene(int num);
void GameMove(void);
void DrawClock();
double calc_smoothratio();
void DoTitle(CompletionFunc completion);

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

    double const smoothratio = calc_smoothratio();

    DrawView(smoothratio);
    DrawStatusBar();
    DrawCrosshair(MAXTILES, PlayerList[nLocalPlayer].nHealth >> 3, -PlayerList[nLocalPlayer].angle.look_anghalf(smoothratio), 0, 1);

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
    auto nLogoTile = GameLogo();
    int dword_9AB5F = (I_GetBuildTime() / 16) & 3;

    twod->ClearScreen();

    DrawRel(kSkullHead, 160, 100, 0);
    DrawRel(kSkullJaw, 161, 130, 0);
    DrawRel(nLogoTile, 160, 40, 0);

    // draw the fire urn/lamp thingies
    DrawRel(kTile3512 + dword_9AB5F, 50, 150, 0);
    DrawRel(kTile3512 + ((dword_9AB5F + 2) & 3), 270, 150, 0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void Intermission(MapRecord *from_map, MapRecord *to_map)
{
    TArray<JobDesc> jobs;

    if (from_map) StopAllSounds();
    bCamera = false;
    automapMode = am_off;

    if (to_map)
    {
        if (to_map->levelNumber != 0)
            nBestLevel = to_map->levelNumber - 1;

        STAT_Update(false);
        if (to_map->levelNumber == kMap20)
            nPlayerLives[0] = 0;

        if (to_map->levelNumber == 0) // skip all intermission stuff when going to the training map.
        {
            gameaction = ga_nextlevel;
            return;
        }
        else
        {
            DoAfterCinemaScene(to_map->levelNumber - 1, jobs);
        }
        if (to_map->levelNumber > -1 && to_map->levelNumber < kMap20)
        {
            // start a new game at the given level
            if (!nNetPlayerCount && to_map->levelNumber > 0)
            {
                showmap(from_map ? from_map->levelNumber : -1, to_map->levelNumber, nBestLevel, jobs);
            }
            else
                jobs.Push({ Create<DBlackScreen>(1) }); // we need something in here even in the multiplayer case.
        }
    }
    else
    {
        DoAfterCinemaScene(20, jobs);
        STAT_Update(true);
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

void GameInterface::NewGame(MapRecord *map, int skill, bool frommenu)
{
	// start a new game on the given level
	InitNewGame();
	if (map->levelNumber == 1) STAT_StartNewGame("Exhumed", 1);
    if (frommenu) Intermission(nullptr, map);
    else NextLevel(map, skill);
}

void GameInterface::LevelCompleted(MapRecord *map, int skill)
{
    Mus_Stop();
    if (currentLevel->levelNumber == 0) gameaction = ga_mainmenu;
    else if (ConsoleState == c_up) Intermission(currentLevel, map);
    else gameaction = ga_nextlevel;
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
