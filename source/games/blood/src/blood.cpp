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
#include "screenjob_.h"
#include "mapinfo.h"
#include "d_net.h"
#include "v_video.h"
#include "v_draw.h"
#include "texturemanager.h"
#include "statusbar.h"
#include "vm.h"

BEGIN_BLD_NS


IMPLEMENT_CLASS(DBloodActor, false, true)
IMPLEMENT_POINTERS_START(DBloodActor)
IMPLEMENT_POINTER(prevmarker)
IMPLEMENT_POINTER(ownerActor)
IMPLEMENT_POINTER(genDudeExtra.pLifeLeech)
IMPLEMENT_POINTER(genDudeExtra.slave[0])
IMPLEMENT_POINTER(genDudeExtra.slave[1])
IMPLEMENT_POINTER(genDudeExtra.slave[2])
IMPLEMENT_POINTER(genDudeExtra.slave[3])
IMPLEMENT_POINTER(genDudeExtra.slave[4])
IMPLEMENT_POINTER(genDudeExtra.slave[5])
IMPLEMENT_POINTER(genDudeExtra.slave[6])
IMPLEMENT_POINTER(xspr.burnSource)
IMPLEMENT_POINTER(xspr.target)
IMPLEMENT_POINTERS_END

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

size_t DBloodActor::PropagateMark()
{
	if (hit.hit.type == kHitSprite) GC::Mark(hit.hit.hitActor);
	if (hit.ceilhit.type == kHitSprite) GC::Mark(hit.ceilhit.hitActor);
	if (hit.florhit.type == kHitSprite) GC::Mark(hit.florhit.hitActor);
	condition[0].Mark();
	condition[1].Mark();
	return Super::PropagateMark();
}

static void markgcroots()
{
	GC::MarkArray(gProxySpritesList, gProxySpritesCount);
	GC::MarkArray(gSightSpritesList, gSightSpritesCount);
	GC::MarkArray(gPhysSpritesList, gPhysSpritesCount);
	GC::MarkArray(gImpactSpritesList, gImpactSpritesCount);
	for (auto& pl : gPlayer)
	{
		GC::Mark(pl.actor);
		GC::MarkArray(pl.ctfFlagState, 2);
		GC::Mark(pl.aimTarget);
		GC::MarkArray(pl.aimTargets, 16);
		GC::Mark(pl.fragger);
		GC::Mark(pl.voodooTarget);
	}
}


void InitCheats();

bool bNoDemo = false;
int gNetPlayers;
int gChokeCounter = 0;
int blood_globalflags;
PLAYER gPlayerTemp[kMaxPlayers];
int gHealthTemp[kMaxPlayers];
vec3_t startpos;
int16_t startang;
sectortype* startsector;


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

TArray<DBloodActor*> SpawnActors(BloodSpawnSpriteDef& sprites)
{
	TArray<DBloodActor*> spawns(sprites.sprites.Size(), true);
	InitSpriteLists();
	int j = 0;
	for (unsigned i = 0; i < sprites.sprites.Size(); i++)
	{
		if (sprites.sprites[i].statnum == MAXSTATUS)
		{
			spawns.Pop();
			continue;
		}
		auto sprt = &sprites.sprites[i];
		auto actor = InsertSprite(sprt->sectp, sprt->statnum);
		spawns[j++] = actor;
		actor->time = i;
		actor->spr = sprites.sprites[i];
		if (sprites.sprext.Size()) actor->sprext = sprites.sprext[i];
		else actor->sprext = {};
		actor->spsmooth = {};

		if (sprites.sprites[i].extra > 0)
		{
			actor->addX();
			actor->xspr = sprites.xspr[i];
		}
	}
	leveltimer = sprites.sprites.Size();
	return spawns;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PropagateMarkerReferences(void)
{
	BloodStatIterator it(kStatMarker);
	while (auto actor = it.Next())
	{
		switch (actor->spr.type)
		{
		case kMarkerOff:
		case kMarkerAxis:
		case kMarkerWarpDest:
		{
			int nOwner = actor->spr.owner;
			if (validSectorIndex(nOwner))
			{
				if (sector[nOwner].hasX())
				{
					sector[nOwner].xs().marker0 = actor;
					continue;
				}
			}
		}
		break;
		case kMarkerOn:
		{
			int nOwner = actor->spr.owner;
			if (validSectorIndex(nOwner))
			{
				if (sector[nOwner].hasX())
				{
					sector[nOwner].xs().marker1 = actor;
					continue;
				}
			}
		}
		break;
		}

		DeleteSprite(actor);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void StartLevel(MapRecord* level, bool newgame)
{
	if (!level) return;
	gFrameCount = 0;
	PlayClock = 0;
	inputState.ClearAllInput();
	currentLevel = level;

	if (gGameOptions.nGameType == 0)
	{
		///////
		gGameOptions.weaponsV10x = cl_bloodoldweapbalance;
		///////
	}
#if 0
	else if (gGameOptions.nGameType > 0 && newgame)
	{
		// todo
		gBlueFlagDropped = false;
		gRedFlagDropped = false;
	}
#endif
	//drawLoadingScreen();
	BloodSpawnSpriteDef sprites;
	int startsectno;
	dbLoadMap(currentLevel->fileName, (int*)&startpos.X, (int*)&startpos.Y, (int*)&startpos.Z, &startang, &startsectno, nullptr, sprites);
	startsector = &sector[startsectno];
	SECRET_SetMapName(currentLevel->DisplayName(), currentLevel->name);
	STAT_NewLevel(currentLevel->fileName);
	TITLE_InformName(currentLevel->name);
	wsrand(dbReadMapCRC(currentLevel->LabelName()));
	gKillMgr.Clear();
	gSecretMgr.Clear();
	automapping = 1;

	// Here is where later the actors must be spawned.
	auto actorlist = SpawnActors(sprites);
	PropagateMarkerReferences();
	int modernTypesErased = 0;
	for (auto actor : actorlist)
	{
		if (actor->exists() && actor->hasX()) 
		{
			if ((actor->xspr.lSkill & (1 << gGameOptions.nDifficulty)) || (actor->xspr.lS && gGameOptions.nGameType == 0)
				|| (actor->xspr.lB && gGameOptions.nGameType == 2) || (actor->xspr.lT && gGameOptions.nGameType == 3)
				|| (actor->xspr.lC && gGameOptions.nGameType == 1)) {

				DeleteSprite(actor);
				continue;
			}


#ifdef NOONE_EXTENSIONS
			if (!gModernMap && nnExtEraseModernStuff(actor))
				modernTypesErased++;
#endif
		}
	}

#ifdef NOONE_EXTENSIONS
	if (!gModernMap && modernTypesErased > 0)
		Printf(PRINT_NONOTIFY, "> Modern types erased: %d.\n", modernTypesErased);
#endif

	startpos.Z = getflorzofslopeptr(startsector, startpos.X, startpos.Y);
	for (int i = 0; i < kMaxPlayers; i++) {
		gStartZone[i].x = startpos.X;
		gStartZone[i].y = startpos.Y;
		gStartZone[i].z = startpos.Z;
		gStartZone[i].sector = startsector;
		gStartZone[i].ang = startang;

#ifdef NOONE_EXTENSIONS
		// Create spawn zones for players in teams mode.
		if (gModernMap && i <= kMaxPlayers / 2) {
			gStartZoneTeam1[i].x = startpos.X;
			gStartZoneTeam1[i].y = startpos.Y;
			gStartZoneTeam1[i].z = startpos.Z;
			gStartZoneTeam1[i].sector = startsector;
			gStartZoneTeam1[i].ang = startang;

			gStartZoneTeam2[i].x = startpos.X;
			gStartZoneTeam2[i].y = startpos.Y;
			gStartZoneTeam2[i].z = startpos.Z;
			gStartZoneTeam2[i].sector = startsector;
			gStartZoneTeam2[i].ang = startang;
		}
#endif
	}
	InitSectorFX();
	warpInit(actorlist);
	actInit(actorlist);
	evInit(actorlist);
	for (int i = connecthead; i >= 0; i = connectpoint2[i])
	{
		if (newgame)
		{
			playerInit(i, 0);
		}
		playerStart(i, 1);
	}
	if (!newgame)
	{
		for (int i = connecthead; i >= 0; i = connectpoint2[i])
		{
			PLAYER* pPlayer = &gPlayer[i];
			pPlayer->actor->xspr.health &= 0xf000;
			pPlayer->actor->xspr.health |= gHealthTemp[i];
			pPlayer->weaponQav = gPlayerTemp[i].weaponQav;
			pPlayer->curWeapon = gPlayerTemp[i].curWeapon;
			pPlayer->weaponState = gPlayerTemp[i].weaponState;
			pPlayer->weaponAmmo = gPlayerTemp[i].weaponAmmo;
			pPlayer->qavCallback = gPlayerTemp[i].qavCallback;
			pPlayer->qavLoop = gPlayerTemp[i].qavLoop;
			pPlayer->weaponTimer = gPlayerTemp[i].weaponTimer;
			pPlayer->nextWeapon = gPlayerTemp[i].nextWeapon;
			pPlayer->qavLastTick = gPlayerTemp[i].qavLastTick;
			pPlayer->qavTimer = gPlayerTemp[i].qavTimer;			
		}
	}
	PreloadCache();
	InitMirrors();
	trInit(actorlist);
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


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void NewLevel(MapRecord *sng, int skill, bool newgame)
{
	if (skill != -1) gGameOptions.nDifficulty = skill;
	gSkill = gGameOptions.nDifficulty;
	StartLevel(sng, newgame);
	gameaction = ga_level;
}

void GameInterface::NewGame(MapRecord *sng, int skill, bool)
{
	gGameOptions.uGameFlags = 0;
	cheatReset();
	NewLevel(sng, skill, true);
}

void GameInterface::NextLevel(MapRecord *map, int skill)
{
	NewLevel(map, skill, false);
}

int GameInterface::GetCurrentSkill()
{
	return gGameOptions.nDifficulty;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

	BloodSpriteIterator it;
	while (DBloodActor* act = it.Next()) act->interpolated = false;

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
			team_ticker[i] -= 4;
			if (team_ticker[i] < 0)
				team_ticker[i] = 0;
		}

		if (gGameOptions.uGameFlags & GF_AdvanceLevel)
		{
			gGameOptions.uGameFlags &= ~GF_AdvanceLevel;
			seqKillAll();
			STAT_Update(gNextLevel == nullptr);
			CompleteLevel(gNextLevel);
		}
		r_NoInterpolate = false;
	}
	else r_NoInterpolate = true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::DrawBackground()
{
	twod->ClearScreen();
	auto tex = TexMan.CheckForTexture("titlescreen", ETextureType::Any);
	DrawTexture(twod, TexMan.GetGameTexture(tex), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, TAG_DONE);
}

#define x(a, b) registerName(#a, b);
static void SetTileNames()
{
	auto registerName = [](const char* name, int index)
	{
		TexMan.AddAlias(name, tileGetTexture(index));
		TileFiles.addName(name, index);
	};
#include "namelist.h"
	// Oh Joy! Plasma Pak changes the tile number of the title screen, but we preferably want mods that use the original one to display it.
	// So let's make this remapping depend on the CRC.
	if (tileGetCRC32(2518) == 1170870757 && (tileGetCRC32(2046) != 290208654 || tileWidth(2518) == 0)) registerName("titlescreen", 2046);
	else registerName("titlescreen", 2518); 
}
#undef x


void ReadAllRFS();

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::app_init()
{
	GC::AddMarkerFunc(markgcroots);
	InitCheats();
	memcpy(&gGameOptions, &gSingleGameOptions, sizeof(GAMEOPTIONS));
	gGameOptions.nMonsterSettings = !userConfig.nomonsters;
	ReadAllRFS();

	levelLoadDefaults();

	//---------
	SetTileNames();
	C_InitConback(TexMan.CheckForTexture("BACKTILE", ETextureType::Any), true, 0.25);

	Printf(PRINT_NONOTIFY, "Initializing view subsystem\n");
	viewInit();
	Printf(PRINT_NONOTIFY, "Initializing dynamic fire\n");
	FireInit();
	Printf(PRINT_NONOTIFY, "Initializing weapon animations\n");
	WeaponInit();

	Printf(PRINT_NONOTIFY, "Initializing sound system\n");
	sndInit();

	myconnectindex = connecthead = 0;
	gNetPlayers = numplayers = 1;
	connectpoint2[0] = -1;
	gGameOptions.nGameType = 0;
	UpdateNetworkMenus();

	gChoke.init(518, chokeCallback);
	UpdateDacs(0, true);

	enginecompatibility_mode = ENGINECOMPATIBILITY_19960925;

	gViewIndex = myconnectindex;
	gMe = gView = &gPlayer[myconnectindex];
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void gameInit()
{
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
	PlayLogos(ga_mainmenu, ga_mainmenu, true);
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
	if (!Mus_Play(quoteMgr.GetQuote(nMusic), true))
	{
		Mus_Stop();
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

enum
{
	kLoadScreenCRC = -2051908571,
	kLoadScreenWideBackWidth = 256,
	kLoadScreenWideSideWidth = 128,

};


DEFINE_ACTION_FUNCTION(_Blood, OriginalLoadScreen)
{
	static int bLoadScreenCrcMatch = -1;
	if (bLoadScreenCrcMatch == -1) bLoadScreenCrcMatch = tileGetCRC32(kLoadScreen) == kLoadScreenCRC;
	ACTION_RETURN_INT(bLoadScreenCrcMatch);
}

DEFINE_ACTION_FUNCTION(_Blood, PlayIntroMusic)
{
	Mus_Play("PESTIS.MID", false);
	return 0;
}

DEFINE_ACTION_FUNCTION(_Blood, sndStartSample)
{
	PARAM_PROLOGUE;
	PARAM_INT(id);
	PARAM_INT(vol);
	PARAM_INT(chan);
	PARAM_BOOL(looped);
	PARAM_INT(chanflags);
	sndStartSample(id, vol, chan, looped, EChanFlags::FromInt(chanflags));
	return 0;
}

DEFINE_ACTION_FUNCTION(_Blood, sndStartSampleNamed)
{
	PARAM_PROLOGUE;
	PARAM_STRING(id);
	PARAM_INT(vol);
	PARAM_INT(chan);
	sndStartSample(id, vol, chan);
	return 0;
}

DEFINE_ACTION_FUNCTION(_Blood, PowerupIcon)
{
	PARAM_PROLOGUE;
	PARAM_INT(pwup);
	int tile = -1;
	if (pwup >= 0 && pwup < (int)countof(gPowerUpInfo))
	{
		tile = gPowerUpInfo[pwup].picnum;
	}
	FGameTexture* tex = tileGetTexture(tile);
	ACTION_RETURN_INT(tex ? tex->GetID().GetIndex() : -1);
}

DEFINE_ACTION_FUNCTION(_Blood, GetViewPlayer)
{
	PARAM_PROLOGUE;
	ACTION_RETURN_POINTER(gView);
}

DEFINE_ACTION_FUNCTION(_BloodPlayer, GetHealth)
{
	PARAM_SELF_STRUCT_PROLOGUE(PLAYER);
	ACTION_RETURN_INT(self->actor->xspr.health);
}

DEFINE_ACTION_FUNCTION_NATIVE(_BloodPlayer, powerupCheck, powerupCheck)
{
	PARAM_SELF_STRUCT_PROLOGUE(PLAYER);
	PARAM_INT(pwup);
	ACTION_RETURN_INT(powerupCheck(self, pwup));
}

END_BLD_NS
