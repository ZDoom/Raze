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
#include "baselayer.h"
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
#include "core/menu/menu.h"

BEGIN_PS_NS

short nBestLevel;
static int32_t nonsharedtimer;

int GameAction=-1;

extern uint8_t nCinemaSeen;
extern ClockTicks tclocks;

void RunCinemaScene(int num);
void GameMove(void);
void DrawClock();
int32_t calc_smoothratio(ClockTicks totalclk, ClockTicks ototalclk);
void DoTitle(CompletionFunc completion);

static int FinishLevel(TArray<JobDesc> &jobs)
{
    int lnum = currentLevel->levelNumber;
    if (lnum > nBestLevel) {
        nBestLevel = lnum - 1;
    }


    StopAllSounds();

    bCamera = false;
    nMapMode = 0;

    STAT_Update(lnum == kMap20);
    if (lnum != kMap20)
    {
        if (EndLevel != 2)
        {
            // There's really no choice but to enter an active wait loop here to make the sound play out.
            PlayLocalSound(StaticSound[59], 0, true, CHANF_UI);
            int nTicks = totalclock + 12;
            while (nTicks > (int)totalclock) { HandleAsync(); }
        }
    }
    else nPlayerLives[0] = 0;

    DoAfterCinemaScene(lnum, jobs);
    return lnum == kMap20? -1 : lnum + 1;
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


static void GameDisplay(void)
{
    if (currentLevel->levelNumber == kMap20)
    {
        DoEnergyTile();
        DrawClock();
    }

    auto smoothRatio = calc_smoothratio(totalclock, tclocks);

    DrawView(smoothRatio);
    DrawStatusBar();
    if (paused && !M_Active())
    {
        auto tex = GStrings("TXTB_PAUSED");
		int nStringWidth = SmallFont->StringWidth(tex);
		DrawText(twod, SmallFont, CR_UNTRANSLATED, 160 - nStringWidth / 2, 100, tex, DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, TAG_DONE);
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void startmainmenu()
{
    gamestate = GS_MENUSCREEN;
    M_StartControlPanel(false);
    M_SetMenu(NAME_Mainmenu);
    StopAllSounds();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void drawmenubackground()
{
    auto nLogoTile = EXHUMED ? kExhumedLogo : kPowerslaveLogo;
    int dword_9AB5F = ((int)totalclock / 16) & 3;

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

void CheckProgression()
{
    TArray<JobDesc> jobs;
    bool startlevel = false;
    int mylevelnew = -1;

    if (GameAction >= 0)
    {
        if (GameAction < 1000)
        {
            // start a new game on the given level
            currentLevel = nullptr;
            mylevelnew = GameAction;
            GameAction = -1;
            InitNewGame();
            if (mylevelnew > 0) STAT_StartNewGame("Exhumed", 1);
            if (mylevelnew != 0) nBestLevel = mylevelnew - 1;
        }
        else
        {
            // A savegame was loaded. Just start the level without any further actions.
            GameAction = -1;
            gamestate = GS_LEVEL;
            return;
        }
    }
    else if (EndLevel)
    {
        if (currentLevel->levelNumber == 0) startmainmenu();
        else mylevelnew = FinishLevel(jobs);
        EndLevel = false;
    }
    if (mylevelnew > -1 && mylevelnew < kMap20)
    {
        startlevel = true;
        // start a new game at the given level
        if (!nNetPlayerCount && mylevelnew > 0)
        {
            showmap(currentLevel? currentLevel->levelNumber : -1, mylevelnew, nBestLevel, jobs);
        }
        else
            jobs.Push({ Create<DScreenJob>() }); // we need something in here even in the multiplayer case.
    }
    if (jobs.Size() > 0)
    {
        selectedlevelnew = mylevelnew;
        RunScreenJob(jobs.Data(), jobs.Size(), [=](bool)
            {
                if (!startlevel) gamestate = GS_STARTUP;
                else
                {
                    gamestate = GS_LEVEL;

                    InitLevel(selectedlevelnew);
                    tclocks = totalclock;
#if 0
                    // this would be the place to autosave upon level start
                    if (!bInDemo && selectedlevelnew > nBestLevel && selectedlevelnew != 0 && selectedlevelnew <= kMap20) {
                        menu_GameSave(SavePosition);
                    }
#endif
                    if (selectedlevelnew > nBestLevel)
                    {
                        nBestLevel = selectedlevelnew;
                    }

                    if (selectedlevelnew == 11) nCinemaSeen |= 2;
                    if (mylevelnew != selectedlevelnew) STAT_Cancel();
                    else STAT_NewLevel(currentLevel->labelName);
                }
            });
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameLoop()
{
    CheckKeys();
    GameTicker();
    PlayerInterruptKeys();
    CheckKeys2();
    fps++;
}


void GameInterface::RunGameFrame()
{
    again:
    try
    {
        HandleAsync();
        updatePauseStatus();
        D_ProcessEvents();
        CheckProgression();
        switch (gamestate)
        {
        default:
        case GS_STARTUP:
            totalclock = 0;
            ototalclock = 0;
            GameAction = -1;
            EndLevel = false;

            if (userConfig.CommandMap.IsNotEmpty())
            {
                auto map = FindMapByName(userConfig.CommandMap);
                if (map) GameAction = map->levelNumber;
                userConfig.CommandMap = "";
                goto again;
            }
            else
            {
                DoTitle([](bool) { startmainmenu(); });
            }
            break;

        case GS_MENUSCREEN:
        case GS_FULLCONSOLE:
            drawmenubackground();
            break;

        case GS_LEVEL:
            GameLoop();
            GameDisplay();
            break;

        case GS_INTERMISSION:
        case GS_INTRO:
            RunScreenJobFrame();	// This handles continuation through its completion callback.
            break;

        }
    }
    catch (CRecoverableError&)
    {
        // Clear all progression sensitive variables here.
        GameAction = -1;
        EndLevel = false;
        throw;
    }

}

END_PS_NS
