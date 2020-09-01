//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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

#include "ns.h"	// Must come before everything else!

#include "build.h"
#include "mmulti.h"
#include "compat.h"
#include "common.h"
#include "common_game.h"
#include "g_input.h"

#include "db.h"
#include "blood.h"
#include "choke.h"
#include "controls.h"
#include "dude.h"
#include "endgame.h"
#include "eventq.h"
#include "fx.h"
#include "gib.h"
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "network.h"
#include "sectorfx.h"
#include "seq.h"
#include "sound.h"
#include "triggers.h"
#include "view.h"
#include "misc.h"
#include "gameconfigfile.h"
#include "gamecontrol.h"
#include "m_argv.h"
#include "statistics.h"
#include "menu.h"
#include "raze_sound.h"
#include "nnexts.h"
#include "secrets.h"
#include "gamestate.h"
#include "screenjob.h"
#include "mapinfo.h"

BEGIN_BLD_NS

void InitCheats();

bool bNoDemo = false;

char gUserMapFilename[BMAX_PATH];

short BloodVersion = 0x115;

bool gameRestart;
int gNetPlayers;
int gQuitRequest;

int gChokeCounter = 0;

bool gQuitGame;

int blood_globalflags;

void QuitGame(void)
{
    throw CExitEvent(0);
}


void EndLevel(void)
{
    gViewPos = VIEWPOS_0;
    sndKillAllSounds();
    sfxKillAllSounds();
    ambKillAll();
    seqKillAll();
}


PLAYER gPlayerTemp[kMaxPlayers];
int gHealthTemp[kMaxPlayers];

vec3_t startpos;
int16_t startang, startsectnum;

void StartLevel(MapRecord *level)
{
    if (!level) return;
	STAT_Update(0);
    EndLevel();
    gInput = {};
    gStartNewGame = nullptr;
    ready2send = 0;
    netWaitForEveryone(0);
    currentLevel = level;

    if (gGameOptions.nGameType == 0)
    {
        ///////
        gGameOptions.weaponsV10x = gWeaponsV10x;
        ///////
    }
#if 0
    else if (gGameOptions.nGameType > 0 && !(gGameOptions.uGameFlags&1))
    {
        // todo
        gBlueFlagDropped = false;
        gRedFlagDropped = false;
    }
#endif
    if (gGameOptions.uGameFlags&1)
    {
        for (int i = connecthead; i >= 0; i = connectpoint2[i])
        {
            memcpy(&gPlayerTemp[i],&gPlayer[i],sizeof(PLAYER));
            gHealthTemp[i] = xsprite[gPlayer[i].pSprite->extra].health;
        }
    }
    bVanilla = false;
    memset(xsprite,0,sizeof(xsprite));
    memset(sprite,0,kMaxSprites*sizeof(spritetype));
    //drawLoadingScreen();
    if (dbLoadMap(currentLevel->fileName,(int*)&startpos.x,(int*)&startpos.y,(int*)&startpos.z,&startang,&startsectnum,nullptr))
    {
        I_Error("Unable to load map");
    }
    SECRET_SetMapName(currentLevel->DisplayName(), currentLevel->name);
	STAT_NewLevel(currentLevel->fileName);
    G_LoadMapHack(currentLevel->fileName);
    wsrand(dbReadMapCRC(currentLevel->LabelName()));
    gKillMgr.Clear();
    gSecretMgr.Clear();
    gLevelTime = 0;
    automapping = 1;
  
    int modernTypesErased = 0;
    for (int i = 0; i < kMaxSprites; i++)
    {
        spritetype *pSprite = &sprite[i];
        if (pSprite->statnum < kMaxStatus && pSprite->extra > 0) {
            
            XSPRITE *pXSprite = &xsprite[pSprite->extra];
            if ((pXSprite->lSkill & (1 << gGameOptions.nDifficulty)) || (pXSprite->lS && gGameOptions.nGameType == 0)
                || (pXSprite->lB && gGameOptions.nGameType == 2) || (pXSprite->lT && gGameOptions.nGameType == 3)
                || (pXSprite->lC && gGameOptions.nGameType == 1)) {
                
                DeleteSprite(i);
                continue;
            }

            
            #ifdef NOONE_EXTENSIONS
            if (!gModernMap && nnExtEraseModernStuff(pSprite, pXSprite))
               modernTypesErased++;
            #endif
                }
                }
                
    #ifdef NOONE_EXTENSIONS
    if (!gModernMap)
        Printf(PRINT_NONOTIFY, "> Modern types erased: %d.\n", modernTypesErased);
    #endif

    startpos.z = getflorzofslope(startsectnum,startpos.x,startpos.y);
    for (int i = 0; i < kMaxPlayers; i++) {
        gStartZone[i].x = startpos.x;
        gStartZone[i].y = startpos.y;
        gStartZone[i].z = startpos.z;
        gStartZone[i].sectnum = startsectnum;
        gStartZone[i].ang = startang;

        #ifdef NOONE_EXTENSIONS
        // Create spawn zones for players in teams mode.
        if (gModernMap && i <= kMaxPlayers / 2) {
            gStartZoneTeam1[i].x = startpos.x;
            gStartZoneTeam1[i].y = startpos.y;
            gStartZoneTeam1[i].z = startpos.z;
            gStartZoneTeam1[i].sectnum = startsectnum;
            gStartZoneTeam1[i].ang = startang;

            gStartZoneTeam2[i].x = startpos.x;
            gStartZoneTeam2[i].y = startpos.y;
            gStartZoneTeam2[i].z = startpos.z;
            gStartZoneTeam2[i].sectnum = startsectnum;
            gStartZoneTeam2[i].ang = startang;
        }
        #endif
    }
    InitSectorFX();
    warpInit();
    actInit(false);
    evInit();
    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        if (!(gGameOptions.uGameFlags&1))
        {
            if (numplayers == 1)
            {
                gProfile[i].skill = gSkill;
                gProfile[i].nAutoAim = cl_autoaim;
                gProfile[i].nWeaponSwitch = cl_weaponswitch;
            }
            playerInit(i,0);
        }
        playerStart(i, 1);
    }
    if (gGameOptions.uGameFlags&1)
    {
        for (int i = connecthead; i >= 0; i = connectpoint2[i])
        {
            PLAYER *pPlayer = &gPlayer[i];
            pPlayer->pXSprite->health &= 0xf000;
            pPlayer->pXSprite->health |= gHealthTemp[i];
            pPlayer->weaponQav = gPlayerTemp[i].weaponQav;
            pPlayer->curWeapon = gPlayerTemp[i].curWeapon;
            pPlayer->weaponState = gPlayerTemp[i].weaponState;
            pPlayer->weaponAmmo = gPlayerTemp[i].weaponAmmo;
            pPlayer->qavCallback = gPlayerTemp[i].qavCallback;
            pPlayer->qavLoop = gPlayerTemp[i].qavLoop;
            pPlayer->weaponTimer = gPlayerTemp[i].weaponTimer;
            pPlayer->nextWeapon = gPlayerTemp[i].nextWeapon;
        }
    }
    gGameOptions.uGameFlags &= ~3;
    PreloadCache();
    InitMirrors();
    gFrameClock = 0;
    trInit();
    if (!bVanilla && !gMe->packSlots[1].isActive) // if diving suit is not active, turn off reverb sound effect
        sfxSetReverb(0);
    ambInit();
    netReset();
    gFrameCount = 0;
    gChokeCounter = 0;
	M_ClearMenus();
    // viewSetMessage("");
    viewSetErrorMessage("");
    netWaitForEveryone(0);
    gameclock = 0;
    lastTic = -1;
    paused = 0;
    ready2send = 1;
    levelTryPlayMusic();
}


bool gRestartGame = false;

void ProcessFrame(void)
{
    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        auto& inp = gPlayer[i].input;
        auto oldactions = inp.actions;

        inp = gFifoInput[gNetFifoTail & 255][i];
        inp.actions |= oldactions & ~(SB_BUTTON_MASK|SB_RUN|SB_WEAPONMASK_BITS);  // should be everything non-button and non-weapon

        int newweap = inp.getNewWeapon();
        if (newweap > 0 && newweap < WeaponSel_MaxBlood) gPlayer[i].newWeapon = newweap;
    }
    gNetFifoTail++;

#if 0
    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        if (gPlayer[i].input.syncFlags.quit)
        {
            gPlayer[i].input.syncFlags.quit = 0;
            netBroadcastPlayerLogoff(i);
            if (i == myconnectindex)
            {
                // netBroadcastMyLogoff(gQuitRequest == 2);
                gQuitGame = true;
                gRestartGame = gQuitRequest == 2;
                netDeinitialize();
                netResetToSinglePlayer();
                return;
            }
        }
        if (gPlayer[i].input.syncFlags.restart)
        {
            gPlayer[i].input.syncFlags.restart = 0;
            levelRestart();
            return;
        }
    }
#endif
    // This is single player only.
    if (gameRestart)
    {
        gameRestart = false;
        levelRestart();
        return;
    }
    viewClearInterpolations();
    {
        if (paused || gEndGameMgr.at0 || (gGameOptions.nGameType == 0 && M_Active()))
            return;
    }

    thinktime.Reset();
    thinktime.Clock();

    actortime.Reset();
    actortime.Clock();
    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        viewBackupView(i);
        playerProcess(&gPlayer[i]);
    }
    actortime.Unclock();

    trProcessBusy();
    evProcess(gFrameClock);
    seqProcess(4);
    DoSectorPanning();
    actProcessSprites();
    actPostProcess();
#ifdef POLYMER
    G_RefreshLights();
#endif
    viewCorrectPrediction();
    ambProcess();
    viewUpdateDelirium();
    viewUpdateShake();
    gi->UpdateSounds();
    if (gMe->hand == 1)
    {
        const int CHOKERATE = 8;
        const int COUNTRATE = 30;
        gChokeCounter += CHOKERATE;
        while (gChokeCounter >= COUNTRATE)
        {
            gChoke.at1c(gMe);
            gChokeCounter -= COUNTRATE;
        }
    }

    thinktime.Unclock();

    gLevelTime++;
    gFrameCount++;
    gFrameClock += 4;
    if ((gGameOptions.uGameFlags&1) != 0 && !gStartNewGame)
    {
        ready2send = 0;
#if 0
        if (gNetPlayers > 1 && gNetMode == NETWORK_SERVER && gPacketMode == PACKETMODE_1 && myconnectindex == connecthead)
        {
            while (gNetFifoMasterTail < gNetFifoTail)
            {
                netGetPackets();
                h andleevents();
                netMasterUpdate();
            }
        }
#endif
        seqKillAll();
        if (gGameOptions.uGameFlags&2)
        {
            STAT_Update(true);
            if (gGameOptions.nGameType == 0)
            {
				auto completion = [] (bool) {
					gamestate = GS_MENUSCREEN;
					M_StartControlPanel(false);
                    M_SetMenu(NAME_Mainmenu);
                    M_SetMenu(NAME_CreditsMenu);
					gGameOptions.uGameFlags &= ~3;
					gQuitGame = 1;
                    gRestartGame = true;
                };
				
                if (gGameOptions.uGameFlags&8)
				{
                    levelPlayEndScene(volfromlevelnum(currentLevel->levelNumber), completion);
				}
				else completion(false);
            }
			else
			{
				gGameOptions.uGameFlags &= ~3;
				gRestartGame = 1;
				gQuitGame = 1;
			}
        }
        else
        {
            gEndGameMgr.Setup();
        }
    }
}



void ParseOptions(void)
{

}

void ReadAllRFS();

void GameInterface::app_init()
{
    InitCheats();
    memcpy(&gGameOptions, &gSingleGameOptions, sizeof(GAMEOPTIONS));
    gGameOptions.nMonsterSettings = !userConfig.nomonsters;
    ReadAllRFS();

    HookReplaceFunctions();

    Printf(PRINT_NONOTIFY, "Initializing Build 3D engine\n");
    engineInit();

    Printf(PRINT_NONOTIFY, "Loading tiles\n");
    if (!tileInit(0, NULL))
        I_FatalError("TILES###.ART files not found");

    levelLoadDefaults();

    loaddefinitionsfile(BLOODWIDESCREENDEF);

    const char* defsfile = G_DefFile();
    uint32_t stime = I_msTime();
    if (!loaddefinitionsfile(defsfile))
    {
        uint32_t etime = I_msTime();
        Printf(PRINT_NONOTIFY, "Definitions file \"%s\" loaded in %d ms.\n", defsfile, etime - stime);
    }
    powerupInit();
    Printf(PRINT_NONOTIFY, "Loading cosine table\n");
    trigInit();
    Printf(PRINT_NONOTIFY, "Initializing view subsystem\n");
    viewInit();
    Printf(PRINT_NONOTIFY, "Initializing dynamic fire\n");
    FireInit();
    Printf(PRINT_NONOTIFY, "Initializing weapon animations\n");
    WeaponInit();
    LoadSaveSetup();
    LoadSavedInfo();

    Printf(PRINT_NONOTIFY, "Initializing network users\n");
    netInitialize(true);
    Printf(PRINT_NONOTIFY, "Initializing sound system\n");
    sndInit();
    registerosdcommands();

    gChoke.sub_83ff0(518, sub_84230);
    UpdateDacs(0, true);

    enginecompatibility_mode = ENGINECOMPATIBILITY_19960925;//bVanilla;
}

static void gameInit()
{
//RESTART:
    netReset();
    gViewIndex = myconnectindex;
    gMe = gView = &gPlayer[myconnectindex];
    netBroadcastPlayerInfo(myconnectindex);
#if 0
    Printf("Waiting for network players!\n");
    netWaitForEveryone(0);
    if (gRestartGame)
    {
        // Network error
        gQuitGame = false;
        gRestartGame = false;
        netDeinitialize();
        netResetToSinglePlayer();
        goto RESTART;
    }
#endif
	UpdateNetworkMenus();
    gQuitGame = 0;
    gRestartGame = 0;
    if (gGameOptions.nGameType > 0)
    {
        inputState.ClearAllInput();
    }

}

static void gameTicker()
{
    bool gameUpdate = false;
    double const gameUpdateStartTime = I_msTimeF();
    while (gPredictTail < gNetFifoHead[myconnectindex] && !paused)
    {
        viewUpdatePrediction(&gFifoInput[gPredictTail & 255][myconnectindex]);
    }
    if (numplayers == 1)
        gBufferJitter = 0;

    int const currentTic = I_GetTime();
    gameclock = I_GetBuildTime();

    gameupdatetime.Reset();
    gameupdatetime.Clock();

    while (currentTic - lastTic >= 1 && ready2send)
    {
        gNetInput = gInput;
        gInput = {};
        netGetInput();
        lastTic = currentTic;
        while (gNetFifoHead[myconnectindex] - gNetFifoTail > gBufferJitter && !gStartNewGame && !gQuitGame)
        {
            int i;
            for (i = connecthead; i >= 0; i = connectpoint2[i])
                if (gNetFifoHead[i] == gNetFifoTail)
                    break;
            if (i >= 0)
                break;
            ProcessFrame();
            gameUpdate = true;
        }
    }

    gameupdatetime.Unclock();

    if (gQuitRequest && gQuitGame)
        videoClearScreen(0);
    else
    {
        netCheckSync();

        drawtime.Reset();
        drawtime.Clock();
        viewDrawScreen();
        drawtime.Unclock();
    }
}

static void drawBackground()
{
    twod->ClearScreen();
	DrawTexture(twod, tileGetTexture(2518, true), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, TAG_DONE);
#if 0
    if (gQuitRequest && !gQuitGame)
        netBroadcastMyLogoff(gQuitRequest == 2);
#endif
}

static void commonTicker()
{
    if (TestBitString(gotpic, 2342))
    {
        FireProcess();
        ClearBitString(gotpic, 2342);
    }
    if (gStartNewGame)
    {
        auto sng = gStartNewGame;
        gStartNewGame = nullptr;
        gQuitGame = false;
        auto completion = [=](bool = false)
        {
            StartLevel(sng);
            gFrameClock = gameclock;
            gamestate = GS_LEVEL;
        };

        bool startedCutscene = false;
        if (!(sng->flags & MI_USERMAP))
        {
            int episode = volfromlevelnum(sng->levelNumber);
            int level = mapfromlevelnum(sng->levelNumber);
            if (gEpisodeInfo[episode].cutALevel == level && gEpisodeInfo[episode].cutsceneAName[0])
            {
                levelPlayIntroScene(episode, completion);
                startedCutscene = true;
            }

        }
		if (!startedCutscene) completion(false);
    }
    else if (gRestartGame)
    {
        Mus_Stop();
        soundEngine->StopAllChannels();
        gQuitGame = 0;
        gQuitRequest = 0;
        gRestartGame = 0;

        // Don't switch to startup if we're already outside the game.
        if (gamestate == GS_LEVEL)
        {
            gamestate = GS_MENUSCREEN;
            M_StartControlPanel(false);
            M_SetMenu(NAME_Mainmenu);
        }
    }
}

void GameInterface::RunGameFrame()
{
    commonTicker();
    netGetPackets();
    ctrlGetInput();

    switch (gamestate)
    {
    default:
    case GS_STARTUP:
        gameInit();
        if (userConfig.CommandMap.IsNotEmpty())
        {
        }
        else
        {
            if (!userConfig.nologo && gGameOptions.nGameType == 0) playlogos();
            else
            {
                gamestate = GS_MENUSCREEN;
                M_StartControlPanel(false);
                M_SetMenu(NAME_Mainmenu);
            }
        }
        break;

    case GS_MENUSCREEN:
    case GS_FULLCONSOLE:
        drawBackground();
        break;

    case GS_INTRO:
    case GS_INTERMISSION:
        RunScreenJobFrame();	// This handles continuation through its completion callback.
        break;

    case GS_LEVEL:
        gameTicker();
        break;

    case GS_FINALE:
        gEndGameMgr.ProcessKeys();
        gEndGameMgr.Draw();
        break;
    }
}

bool DemoRecordStatus(void) {
    return false;
}

bool VanillaMode() {
    return false;
}

int sndTryPlaySpecialMusic(int nMusic)
{
    if (Mus_Play(nullptr, quoteMgr.GetQuote(nMusic), true))
    {
        return 0;
    }
    return 1;
}

void sndPlaySpecialMusicOrNothing(int nMusic)
{
    if (sndTryPlaySpecialMusic(nMusic))
    {
        Mus_Stop();
    }
}

extern  IniFile* BloodINI;
void GameInterface::FreeGameData()
{
    if (BloodINI) delete BloodINI;
    netDeinitialize();
}

ReservedSpace GameInterface::GetReservedScreenSpace(int viewsize)
{
    int top = 0;
    if (gGameOptions.nGameType > 0 && gGameOptions.nGameType <= 3)
    {
        top = (tilesiz[2229].y * ((gNetPlayers + 3) / 4));
    }
    return { top, 25 };
}

::GameInterface* CreateInterface()
{
	return new GameInterface;
}

END_BLD_NS
