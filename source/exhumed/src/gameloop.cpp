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
#include "menu.h"
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
int forcelevel = -1;
static int32_t nonsharedtimer;

extern int MenuExitCondition;

extern short nCinemaSeen[30];
extern ClockTicks tclocks;

void RunCinemaScene(int num);
void GameMove(void);
void InitGame();
void LockEnergyTiles();
void DrawClock();
int32_t calc_smoothratio(ClockTicks totalclk, ClockTicks ototalclk);
int SyncScreenJob();
void DoTitle(CompletionFunc completion);

void FinishLevel()
{
    if (levelnum > nBestLevel) {
        nBestLevel = levelnum - 1;
    }

    levelnew = levelnum + 1;

    StopAllSounds();

    bCamera = false;
    nMapMode = 0;

    if (levelnum != kMap20)
    {
        EraseScreen(4);
        PlayLocalSound(StaticSound[59], 0, true, CHANF_UI);
        videoNextPage();
        //WaitTicks(12);
        DrawView(65536);
        videoNextPage();
    }

    FadeOut(1);
    EraseScreen(overscanindex);

    if (levelnum == 0)
    {
        nPlayerLives[0] = 0;
        levelnew = 100;
    }
    else
    {
        DoAfterCinemaScene(levelnum);
        if (levelnum == kMap20)
        {
            //DoCredits();
            nPlayerLives[0] = 0;
        }
    }
}




short nBeforeScene[] = { 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 };


void CheckBeforeScene(int nLevel)
{
    if (nLevel == kMap20)
    {
        RunCinemaScene(-1);
        return;
    }

    short nScene = nBeforeScene[nLevel];

    if (nScene)
    {
        if (!nCinemaSeen[nScene])
        {
            RunCinemaScene(nScene);
            nCinemaSeen[nScene] = 1;
        }
    }
}

int SyncScreenJob();

int showmap(short nLevel, short nLevelNew, short nLevelBest)
{
    FadeOut(0);
    EraseScreen(overscanindex);
    GrabPalette();
    BlackOut();

    if (nLevelNew != 11) {
        CheckBeforeScene(nLevelNew);
    }

	int selectedLevel;
	menu_DrawTheMap(nLevel, nLevelNew, nLevelBest, [&](int lev){
		gamestate = GS_LEVEL;
		selectedLevel = lev;
        if (lev != nLevelNew) STAT_Cancel();
	});
	SyncScreenJob();
    if (selectedLevel == 11) {
        CheckBeforeScene(selectedLevel);
    }

    return selectedLevel;
}

void DoAfterCinemaScene(int nLevel)
{
    short nAfterScene[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 7, 0, 0, 0, 0, 6 };

    if (nAfterScene[nLevel]) {
        RunCinemaScene(nAfterScene[nLevel]);
    }
}

void DoFailedFinalScene()
{
    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);

    if (CDplaying()) {
        fadecdaudio();
    }

    playCDtrack(9, false);
    FadeToWhite();

    RunCinemaScene(4);
}

void DoGameOverScene()
{
    FadeOut(0);
    inputState.ClearAllInput();

    NoClip();
    overwritesprite(0, 0, kTile3591, 0, 2, kPalNormal, 16);
    videoNextPage();
    PlayGameOverSound();
    //WaitAnyKey(3);
    FadeOut(0);
}



static void GameDisplay(void)
{
    // End Section B

    SetView1();

    if (levelnum == kMap20)
    {
        LockEnergyTiles();
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
    if (M_Active())
    {
        D_ProcessEvents();
    }

    videoNextPage();
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


void CheckProgression()
{
    if (EndLevel)
    {
        EndLevel = false;
        FinishLevel();
    }
    if (levelnew > -1)
    {
        if (levelnew > 99)
        {
            // end the game (but don't abort like the original game did!)
            gamestate = GS_STARTUP;
        }
        else if (levelnew == -2)
        {
            // A savegame was loaded. Just start the level without any further actions.
            gamestate = GS_LEVEL;
        }
        else
        {
            // start a new game at the given level
            if (!nNetPlayerCount && levelnew <= kMap20)
            {
                levelnew = showmap(levelnum, levelnew, nBestLevel);
            }

            if (levelnew > nBestLevel)
            {
                nBestLevel = levelnew;
            }
            InitNewGame();
        }
    }

}


void GameLoop()
{
    CheckKeys();
    GameTicker();
    PlayerInterruptKeys();
    CheckKeys2();
    fps++;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int app_loop()// GameInterface::app_main()
{
    InitGame();
    gamestate = GS_STARTUP;

    while (true)
    {
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

                if (userConfig.CommandMap.IsNotEmpty())
                {
                    auto map = FindMapByName(userConfig.CommandMap);
                    if (map) levelnew = map->levelNumber;
                    userConfig.CommandMap = "";
                    continue;
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



int GameInterface::app_main()
{
    int nMenu = 0;

    InitGame();
    if (!userConfig.nologo)
    {
        DoTitle([](bool) { gamestate = GS_MENUSCREEN; });
        SyncScreenJob();
        gamestate = GS_LEVEL;
    }
    // loc_11811:
    if (forcelevel > -1)
    {
        levelnew = forcelevel;
        goto STARTGAME1;
    }

    SavePosition = -1;
    nMenu = menu_Menu(0);
    switch (nMenu)
    {
    case 3:
        forcelevel = 0;
        goto STARTGAME2;
    case 6:
        goto GAMELOOP;
    }
STARTGAME1:
    levelnew = 1;
    levelnum = 1;
    if (!nNetPlayerCount) {
        FadeOut(0);
    }
STARTGAME2:

    InitNewGame();

    nBestLevel = levelnew - 1;
LOOP1:

    if (!bInDemo && levelnew > nBestLevel && levelnew != 0 && levelnew <= kMap20 && SavePosition > -1) {
        menu_GameSave(SavePosition);
    }
    if (!nNetPlayerCount && levelnew > 0 && levelnew <= kMap20) {
        levelnew = showmap(levelnum, levelnew, nBestLevel);
    }

    if (levelnew > nBestLevel) {
        nBestLevel = levelnew;
    }
    InitLevel(levelnew);
    tclocks = totalclock;
    levelnew = -1;
    // Game Loop
GAMELOOP:
    while (1)
    {
        if (levelnew >= 0)
        {
            goto LOOP1;
        }

        HandleAsync();
        C_RunDelayedCommands();

        updatePauseStatus();
        GameLoop();
        GameDisplay();

        if (EndLevel)
		{
            EndLevel = false;
            FinishLevel();
		}
    }
}



END_PS_NS
