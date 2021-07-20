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
#include "screenjob_.h"
#include "c_console.h"
#include "cheathandler.h"
#include "statistics.h"
#include "g_input.h"
#include "razemenu.h"
#include "d_net.h"
#include "automap.h"
#include "raze_music.h"
#include "v_draw.h"
#include "vm.h"

BEGIN_PS_NS

short nBestLevel;

void RunCinemaScene(int num);
void GameMove(void);
void DrawClock();
double calc_smoothratio();
void DoTitle(CompletionFunc completion);



void GameInterface::Render()
{
    CheckKeys2();
    drawtime.Reset();
    drawtime.Clock();

    if (currentLevel && (currentLevel->gameflags & LEVEL_EX_COUNTDOWN))
    {
        DoEnergyTile();
        DrawClock();
    }

    double const smoothratio = calc_smoothratio();

    DrawView(smoothratio);
    DrawStatusBar();
    DrawCrosshair(kCrosshairTile, PlayerList[nLocalPlayer].nHealth >> 3, -PlayerList[nLocalPlayer].angle.look_anghalf(smoothratio), 0, 1);

    if (paused && !M_Active())
    {
        auto tex = GStrings("TXTB_PAUSED");
        auto font = PickSmallFont(tex);
        int nStringWidth = font->StringWidth(tex);
		DrawText(twod, font, CR_UNTRANSLATED, 160 - nStringWidth / 2, 100, tex, DTA_FullscreenScale, FSMode_Fit320x200, TAG_DONE);
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

void GameInterface::NextLevel(MapRecord *map, int skill)
{
	InitLevel(map);

	if (map->levelNumber >= nBestLevel)
	{
		nBestLevel = map->levelNumber - 1;
	}
	
	STAT_NewLevel(currentLevel->labelName);
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::NewGame(MapRecord *map, int skill, bool frommenu)
{
	// start a new game on the given level
	InitNewGame();
    InitLevel(map);
    gameaction = ga_level;
}

int GameInterface::GetCurrentSkill()
{
    return -2;
}

int selectedlevelnew;

DEFINE_ACTION_FUNCTION(DMapScreen, SetNextLevel)
{
    PARAM_PROLOGUE;
    PARAM_INT(v);
    selectedlevelnew = v;
    return 0;
}

void GameInterface::LevelCompleted(MapRecord *to_map, int skill)
{
    Mus_Stop();
    if (currentLevel->gameflags & LEVEL_EX_TRAINING)
    {
        gameaction = ga_mainmenu;
        return;
    }

    StopAllSounds();
    bCamera = false;
    automapMode = am_off;

    STAT_Update(to_map == nullptr);
    if (to_map)
    {
        if (to_map->levelNumber > nBestLevel) nBestLevel = to_map->levelNumber - 1;

        if (to_map->gameflags & LEVEL_EX_COUNTDOWN) PlayerList[0].nLives = 0;
        if (to_map->gameflags & LEVEL_EX_TRAINING)
        {
            gameaction = ga_nextlevel;
            return;
        }
    }
    SummaryInfo info{};
    info.kills = nCreaturesKilled;
    info.maxkills = nCreaturesTotal;
    info.supersecrets = nBestLevel;
    info.time = PlayClock * GameTicRate / 120;
    selectedlevelnew = to_map->levelNumber;
    ShowIntermission(currentLevel, to_map, &info, [=](bool)
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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::Startup()
{
    resettiming();
    EndLevel = 0;
    PlayLogos(ga_mainmenu, ga_mainmenu, false);
}

void GameInterface::ErrorCleanup()
{
    // Clear all progression sensitive variables here.
    EndLevel = 0;
}

END_PS_NS
