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

extern short nAlarmTicks;
extern short nRedTicks;
extern short nClockVal;
extern int MenuExitCondition;
extern short fps;
extern short bInMove;

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
void ResetEngine();
void CheckKeys();
void CheckKeys2();
void GameTicker();

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


int GameInterface::app_main()
{
    int nMenu = 0;
    int lastlevel;

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

MENU:
    SavePosition = -1;
    nMenu = menu_Menu(0);
    switch (nMenu)
    {
    case -1:
        goto MENU;
    case 0:
        goto EXITGAME;
    case 3:
        forcelevel = 0;
        goto STARTGAME2;
    case 6:
        goto GAMELOOP;
    case 9:
        goto MENU;
    }
STARTGAME1:
    levelnew = 1;
    levelnum = 1;
    if (!nNetPlayerCount) {
        FadeOut(0);
    }
STARTGAME2:

    bCamera = false;
    ClearCinemaSeen();
    PlayerCount = 0;
    lastlevel = -1;

    for (int i = 0; i < nTotalPlayers; i++)
    {
        int nPlayer = GrabPlayer();
        if (nPlayer < 0) {
            I_Error("Can't create local player\n");
        }

        InitPlayerInventory(nPlayer);
    }

    nNetMoves = 0;

    if (forcelevel > -1)
    {
        // YELLOW SECTION
        levelnew = forcelevel;
        UpdateInputs();
        forcelevel = -1;

        goto LOOP3;
    }

    // PINK SECTION
    UpdateInputs();
    nNetMoves = 1;

    if (nMenu == 2)
    {
        levelnew = 1;
        levelnum = 1;
        levelnew = menu_GameLoad(SavePosition);
        lastlevel = -1;
    }

    nBestLevel = levelnew - 1;
LOOP1:

    if (nPlayerLives[nLocalPlayer] <= 0) {
        goto MENU;
    }
    if (levelnew > 99) {
        goto EXITGAME;
    }
    if (!bInDemo && levelnew > nBestLevel && levelnew != 0 && levelnew <= kMap20 && SavePosition > -1) {
        menu_GameSave(SavePosition);
    }
LOOP2:
    if (!nNetPlayerCount && levelnew > 0 && levelnew <= kMap20) {
        levelnew = showmap(levelnum, levelnew, nBestLevel);
    }

    if (levelnew > nBestLevel) {
        nBestLevel = levelnew;
    }
LOOP3:
    while (levelnew != -1)
    {
        // BLUE
        if (CDplaying()) {
            fadecdaudio();
        }

        if (levelnew == kMap20)
        {
            lCountDown = 81000;
            nAlarmTicks = 30;
            nRedTicks = 0;
            nClockVal = 0;
            nEnergyTowers = 0;
        }

        if (!LoadLevel(levelnew)) {
            // TODO "Can't load level %d...\n", nMap;
            goto EXITGAME;
        }
        levelnew = -1;
    }
    /* don't restore mid level savepoint if re-entering just completed level
    if (nNetPlayerCount == 0 && lastlevel == levelnum)
    {
        RestoreSavePoint(nLocalPlayer, &initx, &inity, &initz, &initsect, &inita);
    }
    */
    lastlevel = levelnum;

    for (int i = 0; i < nTotalPlayers; i++)
    {
        SetSavePoint(i, initx, inity, initz, initsect, inita);
        RestartPlayer(i);
        InitPlayerKeys(i);
    }

    fps = 0;
    lastfps = 0;
    InitStatus();
    ResetView();
    ResetEngine();
    totalmoves = 0;
    GrabPalette();
    ResetMoveFifo();
    moveframes = 0;
    bInMove = false;
    tclocks = totalclock;
    nPlayerDAng = 0;
    lPlayerXVel = 0;
    lPlayerYVel = 0;
    movefifopos = movefifoend;

    RefreshStatus();

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

        // Section B
        if (!CDplaying() && !nFreeze && !nNetPlayerCount)
        {
            int nTrack = levelnum;
            if (nTrack != 0) {
                nTrack--;
            }

            playCDtrack((nTrack % 8) + 11, true);
        }

// TODO		CONTROL_GetButtonInput();
        updatePauseStatus();
        CheckKeys();

        bInMove = true;

        if (paused)
        {
            tclocks = totalclock - 4;
            buttonMap.ResetButtonStates();
        }
        else
        {
            GameTicker();
        }
        bInMove = false;

        PlayerInterruptKeys();

        if (G_FPSLimit())
        {
            GameDisplay();
        }

        if (!EndLevel)
        {
            nMenu = MenuExitCondition;
            if (nMenu != -2)
            {
                MenuExitCondition = -2;
// MENU2:
                bInMove = true;

                switch (nMenu)
                {
                    case 0:
                        goto EXITGAME;

                    case 1:
                        goto STARTGAME1;

                    case 2:
                        levelnum = levelnew = menu_GameLoad(SavePosition);
                        lastlevel = -1;
                        nBestLevel = levelnew - 1;
                        goto LOOP2;

                    case 3:
                        forcelevel = 0;
                        goto STARTGAME2;
                    case 6:
                        goto GAMELOOP;
                }

                totalclock = ototalclock = tclocks;
                bInMove = false;
                RefreshStatus();
            }
			CheckKeys2();
        }
		else
		{
            EndLevel = false;
            FinishLevel();
		}
        fps++;
    }
EXITGAME:

    ExitGame();
    return 0;
}



END_PS_NS
