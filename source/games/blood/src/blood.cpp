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
#include "g_input.h"
#include "automap.h"

#include "blood.h"
#include "choke.h"

#include "view.h"
#include "misc.h"
#include "gameconfigfile.h"
#include "gamecontrol.h"
#include "m_argv.h"
#include "statistics.h"
#include "razemenu.h"
#include "raze_sound.h"
#include "secrets.h"
#include "gamestate.h"
#include "screenjob.h"
#include "mapinfo.h"
#include "d_net.h"
#include "v_video.h"
#include "v_draw.h"
#include "texturemanager.h"
#include "statusbar.h"

BEGIN_BLD_NS

void InitCheats();

bool bNoDemo = false;
int gNetPlayers;
int gChokeCounter = 0;
int blood_globalflags;
PLAYER gPlayerTemp[kMaxPlayers];
int gHealthTemp[kMaxPlayers];
vec3_t startpos;
int16_t startang, startsectnum;


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

void StartLevel(MapRecord* level)
{
	if (!level) return;
	gFrameCount = 0;
	PlayClock = 0;
	STAT_Update(0);
	EndLevel();
	inputState.ClearAllInput();
	currentLevel = level;

	if (gGameOptions.nGameType == 0)
	{
		///////
		gGameOptions.weaponsV10x = gWeaponsV10x;
		///////
	}
#if 0
	else if (gGameOptions.nGameType > 0 && !(gGameOptions.uGameFlags & GF_AdvanceLevel))
	{
		// todo
		gBlueFlagDropped = false;
		gRedFlagDropped = false;
	}
#endif
	if (gGameOptions.uGameFlags & GF_AdvanceLevel)
	{
		for (int i = connecthead; i >= 0; i = connectpoint2[i])
		{
			memcpy(&gPlayerTemp[i], &gPlayer[i], sizeof(PLAYER));
			gHealthTemp[i] = xsprite[gPlayer[i].pSprite->extra].health;
		}
	}
	memset(xsprite, 0, sizeof(xsprite));
	//drawLoadingScreen();
	dbLoadMap(currentLevel->fileName, (int*)&startpos.x, (int*)&startpos.y, (int*)&startpos.z, &startang, &startsectnum, nullptr);
	SECRET_SetMapName(currentLevel->DisplayName(), currentLevel->name);
	STAT_NewLevel(currentLevel->fileName);
	wsrand(dbReadMapCRC(currentLevel->LabelName()));
	gKillMgr.Clear();
	gSecretMgr.Clear();
	automapping = 1;

	int modernTypesErased = 0;
	for (int i = 0; i < kMaxSprites; i++)
	{
		spritetype* pSprite = &sprite[i];
		if (pSprite->statnum < kMaxStatus && pSprite->extra > 0) {

			XSPRITE* pXSprite = &xsprite[pSprite->extra];
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
	if (!gModernMap && modernTypesErased > 0)
		Printf(PRINT_NONOTIFY, "> Modern types erased: %d.\n", modernTypesErased);
#endif

	startpos.z = getflorzofslope(startsectnum, startpos.x, startpos.y);
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
		if (!(gGameOptions.uGameFlags & GF_AdvanceLevel))
		{
			playerInit(i, 0);
		}
		playerStart(i, 1);
	}
	if (gGameOptions.uGameFlags & GF_AdvanceLevel)
	{
		for (int i = connecthead; i >= 0; i = connectpoint2[i])
		{
			PLAYER* pPlayer = &gPlayer[i];
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
	gGameOptions.uGameFlags &= ~(GF_AdvanceLevel|GF_EndGame);
	PreloadCache();
	InitMirrors();
	trInit();
	if (!gMe->packSlots[1].isActive) // if diving suit is not active, turn off reverb sound effect
		sfxSetReverb(0);
	ambInit();
	Net_ClearFifo();
	gChokeCounter = 0;
	M_ClearMenus();
	// viewSetMessage("");
	viewSetErrorMessage("");
	paused = 0;
	levelTryPlayMusic();
	gChoke.reset();
	setLevelStarted(level);
}


void NewLevel(MapRecord *sng, int skill)
{
	auto completion = [=](bool = false)
	{
		if (skill != -1) gGameOptions.nDifficulty = skill;
		gSkill = gGameOptions.nDifficulty;
		StartLevel(sng);
		gameaction = ga_level;
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

void GameInterface::NewGame(MapRecord *sng, int skill)
{
	gGameOptions.uGameFlags = 0;
	cheatReset();
	NewLevel(sng, skill);
}

void GameInterface::NextLevel(MapRecord *map, int skill)
{
	gGameOptions.uGameFlags = GF_AdvanceLevel;
	NewLevel(map, skill);
}

void GameInterface::Ticker()
{
	for (int i = connecthead; i >= 0; i = connectpoint2[i])
	{
		auto& inp = gPlayer[i].input;
		auto oldactions = inp.actions;

		inp = playercmds[i].ucmd;
		inp.actions |= oldactions & ~(SB_BUTTON_MASK | SB_RUN | SB_WEAPONMASK_BITS);  // should be everything non-button and non-weapon

		int newweap = inp.getNewWeapon();
		if (newweap > 0 && newweap < WeaponSel_MaxBlood) gPlayer[i].newWeapon = newweap;
	}

	gInterpolateSprite.Zero();
	ClearMovementInterpolations();
	UpdateInterpolations();

	if (!(paused || (gGameOptions.nGameType == 0 && M_Active())))
	{
		thinktime.Reset();
		thinktime.Clock();
		for (int i = connecthead; i >= 0; i = connectpoint2[i])
		{
			viewBackupView(i);
			playerProcess(&gPlayer[i]);
		}

		trProcessBusy();
		evProcess(PlayClock);
		seqProcess(4);
		DoSectorPanning();

		actortime.Reset();
		actortime.Clock();
		actProcessSprites();
		actPostProcess();
		actortime.Unclock();

		viewCorrectPrediction();
		ambProcess();
		viewUpdateDelirium();
		gi->UpdateSounds();
		if (gMe->hand == 1)
		{
			const int CHOKERATE = 8;
			const int COUNTRATE = 30;
			gChokeCounter += CHOKERATE;
			while (gChokeCounter >= COUNTRATE)
			{
				gChoke.callback(gMe);
				gChokeCounter -= COUNTRATE;
			}
		}
		thinktime.Unclock();

		gFrameCount++;
		PlayClock += kTicsPerFrame;
		if (PlayClock == 8) gameaction = ga_autosave;	// let the game run for 1 frame before saving.

		for (int i = 0; i < 8; i++)
		{
			team_ticker[i] = team_ticker[i] -= 4;
			if (team_ticker[i] < 0)
				team_ticker[i] = 0;
		}

		if ((gGameOptions.uGameFlags & GF_AdvanceLevel) != 0)
		{
			seqKillAll();
			if (gGameOptions.uGameFlags & GF_EndGame)
			{
				STAT_Update(true);
				if (gGameOptions.nGameType == 0)
				{
					auto completion = [](bool) {
						gGameOptions.uGameFlags &= ~(GF_AdvanceLevel|GF_EndGame);
						gameaction = ga_creditsmenu;
					};

					if (gGameOptions.uGameFlags & GF_PlayCutscene)
					{
						levelPlayEndScene(volfromlevelnum(currentLevel->levelNumber), completion);
					}
					else completion(false);
				}
				else
				{
					gGameOptions.uGameFlags &= ~(GF_AdvanceLevel|GF_EndGame);
				}
			}
			else
			{
				STAT_Update(false);
				EndLevel();
				Mus_Stop();
				// Fixme: Link maps, not episode/level pairs.
				int ep = volfromlevelnum(currentLevel->levelNumber);
				auto map = FindMapByLevelNum(levelnum(ep, gNextLevel));
				CompleteLevel(map);
			}
		}
		r_NoInterpolate = false;
	}
	else r_NoInterpolate = true;
}

void GameInterface::DrawBackground()
{
	twod->ClearScreen();
	DrawTexture(twod, tileGetTexture(2518, true), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, TAG_DONE);
}

#define x(a, b) registerName(#a, b);
static void SetTileNames()
{
	auto registerName = [](const char* name, int index)
	{
		TexMan.AddAlias(name, tileGetTexture(index));
	};
#include "namelist.h"
}
#undef x


void ReadAllRFS();

void GameInterface::loadPalette(void)
{
	// in nearly typical Blood fashion it had to use an inverse of the original translucency settings...
	static glblend_t const bloodglblend =
	{
		{
			{ 1.f / 3.f, STYLEALPHA_Src, STYLEALPHA_InvSrc, 0 },
			{ 2.f / 3.f, STYLEALPHA_Src, STYLEALPHA_InvSrc, 0 },
		},
	};

	static const char* PLU[15] = {
		"NORMAL.PLU",
		"SATURATE.PLU",
		"BEAST.PLU",
		"TOMMY.PLU",
		"SPIDER3.PLU",
		"GRAY.PLU",
		"GRAYISH.PLU",
		"SPIDER1.PLU",
		"SPIDER2.PLU",
		"FLAME.PLU",
		"COLD.PLU",
		"P1.PLU",
		"P2.PLU",
		"P3.PLU",
		"P4.PLU"
	};

	static const char* PAL[5] = {
		"BLOOD.PAL",
		"WATER.PAL",
		"BEAST.PAL",
		"SEWER.PAL",
		"INVULN1.PAL"
	};

	for (auto& x : glblend) x = bloodglblend;

	for (int i = 0; i < 5; i++)
	{
		auto pal = fileSystem.LoadFile(PAL[i]);
		if (pal.Size() < 768) I_FatalError("%s: file too small", PAL[i]);
		paletteSetColorTable(i, pal.Data(), false, false);
	}

	numshades = 64;
	for (int i = 0; i < MAXPALOOKUPS; i++)
	{
		int lump = i < 15 ? fileSystem.FindFile(PLU[i]) : fileSystem.FindResource(i, "PLU");
		if (lump < 0)
		{
			if (i < 15) I_FatalError("%s: file not found", PLU[i]);
			else continue;
		}
		auto data = fileSystem.GetFileData(lump);
		if (data.Size() != 64 * 256)
		{
			if (i < 15) I_FatalError("%s: Incorrect PLU size", PLU[i]);
			else continue;
		}
		lookups.setTable(i, data.Data());
	}

	lookups.setFadeColor(1, 255, 255, 255);
	paletteloaded = PALETTE_SHADE | PALETTE_TRANSLUC | PALETTE_MAIN;
}

void GameInterface::app_init()
{
	InitCheats();
	memcpy(&gGameOptions, &gSingleGameOptions, sizeof(GAMEOPTIONS));
	gGameOptions.nMonsterSettings = !userConfig.nomonsters;
	ReadAllRFS();

	HookReplaceFunctions();

	Printf(PRINT_NONOTIFY, "Loading tiles\n");
	if (!tileInit(0, NULL))
		I_FatalError("TILES###.ART files not found");

	levelLoadDefaults();
	LoadDefinitions();
	SetTileNames();
	C_InitConback(TexMan.CheckForTexture("BACKTILE", ETextureType::Any), true, 0.25);

	TileFiles.SetBackup();
	powerupInit();
	Printf(PRINT_NONOTIFY, "Loading cosine table\n");
	trigInit();
	Printf(PRINT_NONOTIFY, "Initializing view subsystem\n");
	viewInit();
	Printf(PRINT_NONOTIFY, "Initializing dynamic fire\n");
	FireInit();
	Printf(PRINT_NONOTIFY, "Initializing weapon animations\n");
	WeaponInit();

	myconnectindex = connecthead = 0;
	gNetPlayers = numplayers = 1;
	connectpoint2[0] = -1;
	gGameOptions.nGameType = 0;
	UpdateNetworkMenus();

	Printf(PRINT_NONOTIFY, "Initializing sound system\n");
	sndInit();

	gChoke.init(518, chokeCallback);
	UpdateDacs(0, true);

	enginecompatibility_mode = ENGINECOMPATIBILITY_19960925;
}

static void gameInit()
{
	//RESTART:
	gViewIndex = myconnectindex;
	gMe = gView = &gPlayer[myconnectindex];

	UpdateNetworkMenus();
	if (gGameOptions.nGameType > 0)
	{
		inputState.ClearAllInput();
	}

}


void GameInterface::Startup()
{
	gameInit();
	if (userConfig.CommandMap.IsNotEmpty())
	{
	}
	else
	{
		if (!userConfig.nologo && gGameOptions.nGameType == 0) playlogos();
		else
		{
			gameaction = ga_mainmenu;
		}
	}
}



void GameInterface::Render()
{
	drawtime.Reset();
	drawtime.Clock();
	viewDrawScreen();
	drawtime.Unclock();
}


void sndPlaySpecialMusicOrNothing(int nMusic)
{
	if (!Mus_Play(nullptr, quoteMgr.GetQuote(nMusic), true))
	{
		Mus_Stop();
	}
}

extern  IniFile* BloodINI;
void GameInterface::FreeGameData()
{
	if (BloodINI) delete BloodINI;
}

void GameInterface::FreeLevelData()
{
	EndLevel();
	::GameInterface::FreeLevelData();
}


ReservedSpace GameInterface::GetReservedScreenSpace(int viewsize)
{
	int top = 0;
	if (gGameOptions.nGameType > 0 && gGameOptions.nGameType <= 3)
	{
		top = (tileHeight(2229) * ((gNetPlayers + 3) / 4));
	}
	return { top, 25 };
}

::GameInterface* CreateInterface()
{
	return new GameInterface;
}

END_BLD_NS
