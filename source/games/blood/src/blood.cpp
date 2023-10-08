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
#include "tilesetbuilder.h"
#include "nnexts.h"
#include "thingdef.h"

BEGIN_BLD_NS


IMPLEMENT_CLASS(DBloodActor, false, true)
IMPLEMENT_POINTERS_START(DBloodActor)
#ifdef NOONE_EXTENSIONS
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
#endif
IMPLEMENT_POINTER(xspr.burnSource)
IMPLEMENT_POINTER(xspr.target)
IMPLEMENT_POINTERS_END

IMPLEMENT_CLASS(DBloodPlayer, false, true)
IMPLEMENT_POINTERS_START(DBloodPlayer)
IMPLEMENT_POINTER(ctfFlagState[0])
IMPLEMENT_POINTER(ctfFlagState[1])
IMPLEMENT_POINTER(aimTarget)
IMPLEMENT_POINTER(fragger)
IMPLEMENT_POINTER(voodooTarget)
IMPLEMENT_POINTER(aimTargets[0])
IMPLEMENT_POINTER(aimTargets[1])
IMPLEMENT_POINTER(aimTargets[2])
IMPLEMENT_POINTER(aimTargets[3])
IMPLEMENT_POINTER(aimTargets[4])
IMPLEMENT_POINTER(aimTargets[5])
IMPLEMENT_POINTER(aimTargets[6])
IMPLEMENT_POINTER(aimTargets[7])
IMPLEMENT_POINTER(aimTargets[8])
IMPLEMENT_POINTER(aimTargets[9])
IMPLEMENT_POINTER(aimTargets[10])
IMPLEMENT_POINTER(aimTargets[11])
IMPLEMENT_POINTER(aimTargets[12])
IMPLEMENT_POINTER(aimTargets[13])
IMPLEMENT_POINTER(aimTargets[14])
IMPLEMENT_POINTER(aimTargets[15])
IMPLEMENT_POINTERS_END

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------
void MarkSprInSect();
void MarkSeq();


size_t DBloodActor::PropagateMark()
{
	if (hit.hit.type == kHitSprite) GC::Mark(hit.hit.hitActor);
	if (hit.ceilhit.type == kHitSprite) GC::Mark(hit.ceilhit.hitActor);
	if (hit.florhit.type == kHitSprite) GC::Mark(hit.florhit.hitActor);
#ifdef NOONE_EXTENSIONS
	condition[0].Mark();
	condition[1].Mark();
#endif
	return Super::PropagateMark();
}

static void markgcroots()
{
#ifdef NOONE_EXTENSIONS
	GC::MarkArray(gProxySpritesList, gProxySpritesCount);
	GC::MarkArray(gSightSpritesList, gSightSpritesCount);
	GC::MarkArray(gPhysSpritesList, gPhysSpritesCount);
	GC::MarkArray(gImpactSpritesList, gImpactSpritesCount);
	for (auto& cond : gConditions)
	{
		for (auto& obj : cond.objects) obj.obj.Mark();
	}
	MarkSprInSect();
#endif
	for (auto& evobj : rxBucket)
	{
		evobj.Mark();
	}
	MarkSeq();
}


void InitCheats();

bool bNoDemo = false;
int gNetPlayers;
int gChokeCounter = 0;
int blood_globalflags;
PlayerSave gPlayerTemp[kMaxPlayers];
int gHealthTemp[kMaxPlayers];
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
	gViewPos = viewFirstPerson;
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
		auto actor = InsertSprite(sprt->sectp, sprt->statnum, GetSpawnType(sprt->lotag));
		spawns[j++] = actor;
		actor->time = i;
		actor->initFromSprite(&sprites.sprites[i]);
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
		switch (actor->GetType())
		{
		case kMarkerOff:
		case kMarkerAxis:
		case kMarkerWarpDest:
		{
			int nOwner = actor->spr.intowner;
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
			int nOwner = actor->spr.intowner;
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
	DVector3 startpos;
	dbLoadMap(currentLevel->fileName.GetChars(), startpos, &startang, &startsector, nullptr, sprites);
	auto startangle = mapangle(startang);
	SECRET_SetMapName(currentLevel->DisplayName(), currentLevel->name.GetChars());
	STAT_NewLevel(currentLevel->fileName.GetChars());
	TITLE_InformName(currentLevel->name.GetChars());
	wsrand(dbReadMapCRC(currentLevel->LabelName()));
	gHitInfo.hitSector = nullptr;
	gHitInfo.hitWall = nullptr;
	Level.clearStats();
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
		gStartZone[i].pos = startpos;
		gStartZone[i].sector = startsector;
		gStartZone[i].angle = startangle;

#ifdef NOONE_EXTENSIONS
		// Create spawn zones for players in teams mode.
		if (gModernMap && i <= kMaxPlayers / 2) {
			gStartZoneTeam1[i].pos = startpos;
			gStartZoneTeam1[i].sector = startsector;
			gStartZoneTeam1[i].angle = startangle;

			gStartZoneTeam2[i].pos = startpos;
			gStartZoneTeam2[i].sector = startsector;
			gStartZoneTeam2[i].angle = startangle;
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
			DBloodPlayer* pPlayer = getPlayer(i);
			pPlayer->GetActor()->xspr.health &= 0xf000;
			pPlayer->GetActor()->xspr.health |= gHealthTemp[i];
			gPlayerTemp[i].CopyToPlayer(pPlayer);
		}
	}
	PreloadCache();
	InitMirrors();
	trInit(actorlist);
	if (!getPlayer(myconnectindex)->packSlots[1].isActive) // if diving suit is not active, turn off reverb sound effect
		sfxSetReverb(0);
	ambInit();
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
	BloodSpriteIterator it;
	while (DBloodActor* act = it.Next()) act->interpolated = false;

	ClearMovementInterpolations();
	UpdateInterpolations();

	if (!(paused || (gGameOptions.nGameType == 0 && M_Active())))
	{
		thinktime.Reset();
		thinktime.Clock();

		DBloodPlayer* pPlayer = getPlayer(myconnectindex);

		// disable synchronised input if set by game.
		gameInput.ResetInputSync();

		for (int i = connecthead; i >= 0; i = connectpoint2[i])
		{
			getPlayer(i)->Angles.resetCameraAngles();
			viewBackupView(i);
			playerProcess(getPlayer(i));
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
		ambProcess(pPlayer);
		viewUpdateDelirium(pPlayer);
		gi->UpdateSounds();
		if (pPlayer->hand == 1)
		{
			const int CHOKERATE = 8;
			const int COUNTRATE = 30;
			gChokeCounter += CHOKERATE;
			while (gChokeCounter >= COUNTRATE)
			{
				gChoke.callback(pPlayer);
				gChokeCounter -= COUNTRATE;
			}
		}
		thinktime.Unclock();

		// update console player's viewzoffset at the end of the tic.
		pPlayer->GetActor()->oviewzoffset = pPlayer->GetActor()->viewzoffset;
		pPlayer->GetActor()->viewzoffset = pPlayer->zView - pPlayer->GetActor()->spr.pos.Z;

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
	DrawTexture(twod, TexMan.GetGameTexture(aTexIds[kTexTitlescreen]), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, TAG_DONE);
}


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
		auto pal = fileSystem.ReadFileFullName(PAL[i]);
		if (pal.GetSize() < 768) I_FatalError("%s: file too small", PAL[i]);
		paletteSetColorTable(i, (const uint8_t*)pal.GetMem(), false, false);
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
		auto data = fileSystem.ReadFile(lump);
		if (data.GetSize() != 64 * 256)
		{
			if (i < 15) I_FatalError("%s: Incorrect PLU size", PLU[i]);
			else continue;
		}
		lookups.setTable(i, (const uint8_t*)data.GetMem());
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
	// Initialise player array.
	for (unsigned i = 0; i < MAXPLAYERS; i++)
	{
		PlayerArray[i] = Create<DBloodPlayer>(i);
		GC::WriteBarrier(PlayerArray[i]);
	}
	RegisterClasses();

	mirrortile = tileGetTextureID(504);
	InitTextureIDs();

	GC::AddMarkerFunc(markgcroots);

	InitCheats();
	memcpy(&gGameOptions, &gSingleGameOptions, sizeof(GAMEOPTIONS));
	gGameOptions.nMonsterSettings = !userConfig.nomonsters;
	ReadAllRFS();

	levelLoadDefaults();

	//---------
	C_InitConback(aTexIds[kTexBACKTILE], true, 0.25);

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
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void gameInit()
{
	gViewIndex = myconnectindex;

	UpdateNetworkMenus();
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

void GameInterface::FreeLevelData()
{
	EndLevel();
	::GameInterface::FreeLevelData();
}


::GameInterface* CreateInterface()
{
	return new GameInterface;
}

void GameInterface::FinalizeSetup()
{
	// assign spawn types to actor classes. Some code will need them.
	SpawnMap::Iterator it(spawnMap);
	SpawnMap::Pair* pair;
	// this is only reliable for unambiguous assignments, which applies to everything aside from BloodActor itself.
	while (it.NextPair(pair))
	{
		auto cls = pair->Value.cls;
		if (cls != RUNTIME_CLASS(DBloodActor))
		{
			auto actorinfo = static_cast<PClassActor*>(cls)->ActorInfo();
			actorinfo->TypeNum = pair->Key;
		}
	}
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

inline DUDEINFO* getDudeInfo(DBloodActor* actor)
{
	return getDudeInfo(actor->GetType());
}

inline DBloodPlayer* getPlayer(DBloodActor* actor)
{
	return getPlayer(actor->GetType() - kDudePlayer1);
}


// Register all internally used classes at game startup so that we can find naming errors right away without having them cause bugs later.
void RegisterClasses()
{
#define xx(n) { #n, &n##Class},
	static std::pair<const char*, PClassActor**> classreg[] = {
	#include "classnames.h"
	};
#undef xx

	int error = 0;
	for (auto& classdef : classreg)
	{
		auto cls = PClass::FindActor(classdef.first);
		if (cls == nullptr || !cls->IsDescendantOf(RUNTIME_CLASS(DBloodActor)))
		{
			Printf(TEXTCOLOR_RED "%s: Attempt to register unknown actor class\n", classdef.first);
			error++;
		}

		*classdef.second = cls;
	}
	if (error > 0)
	{
		I_FatalError("Unable to register %d actor classes", error);
	}
}

DEFINE_PROPERTY(dmgcontrol, IIIIIII, BloodActor)
{
	for (int i = 0; i < kDamageMax; i++)
	{
		PROP_INT_PARM(j, i);
		defaults->dmgControl[i] = (int16_t)clamp(j, -32768, 32767);
	}
}

// the state parser with its special semantics cannot be extended to handle this right. :(
DEFINE_PROPERTY(aistate, SSIIGGGGs, CoreActor)
{
	PROP_STRING_PARM(label, 0);
	PROP_STRING_PARM(seq, 1); // either a name, an absolute ID with #000 or a relative ID with +000. Empty string means nothing
	PROP_INT_PARM(type, 2);
	PROP_INT_PARM(duration, 3);
	PROP_FUNC_PARM(action, 4);
	PROP_FUNC_PARM(enter, 5);
	PROP_FUNC_PARM(move, 6);
	PROP_FUNC_PARM(tick, 7);
	const char* next = nullptr;
	if (PROP_PARM_COUNT > 8)
	{
		PROP_STRING_PARM(_next, 8);
		next = _next;
	}
	bag.Info->ActorInfo()->AIStates.Reserve(1);
	auto& state = bag.Info->ActorInfo()->AIStates.Last();

	char* endp = (char*)"";
	if (*seq == 0) state.sprite = 0;
	else if (*seq == '#') state.sprite = (int)strtoull(seq + 1, &endp, 10) | 0x10000000;
	else if (*seq == '+') state.sprite = (int)strtoull(seq + 1, &endp, 10) | 0x20000000;
	else state.sprite = FName(seq).GetIndex();

	state.Label = label;
	state.Type = type;
	state.Tics = duration;
	state.ActionFunc = action;
	state.EnterFunc = enter;
	state.MoveFunc = move;
	state.TickFunc = tick;
	state.NextStaten = next ? FName(next) : FName(NAME_None);
}

END_BLD_NS
