#include "ns.h"
#include "wh.h"
#include "mapinfo.h"
#include "gamestate.h"
#include "buildtiles.h"
#include "v_draw.h"
#include "menu.h"
#include "mmulti.h"
#include "raze_music.h"
#include "statistics.h"
#include "version.h"
#include "cheathandler.h"

BEGIN_WH_NS

int followmode, followx, followy, followa;
int followang, followvel, followsvel;

const char *GameInterface::CheckCheatMode()
{
#if 0
	if (sv_cheats && (ud.player_skill == 4 || (isRR() && ud.player_skill > 3) || (isRRRA() && ps[myconnectindex].nocheat)))
	{
		return quoteMgr.GetQuote(QUOTE_CHEATS_DISABLED);
	}
#endif
	return nullptr;
}

static const char *cheatGod(int playerno, int mode)
{
	if (gamestate == GS_LEVEL)
	{
		player[playerno].godMode = mode == -1? !player[playerno].godMode : mode;
		if(player[playerno].godMode)
			return "God mode: On";
		else 
			return "God mode: Off";
	}
	return nullptr;
}

const char* GameInterface::GenericCheat(int playerno, int cheat)
{
	switch (cheat)
	{
	case CHT_GOD:
		return cheatGod(playerno, -1);

	case CHT_GODOFF:
		return cheatGod(playerno, 0);

	case CHT_GODON:
		return cheatGod(playerno, 1);

	case CHT_NOCLIP:
		player[playerno].noclip = !player[playerno].noclip;
		if(player[playerno].noclip)
			return "Noclip mode: On";
		else
			return "Noclip mode: Off";

	default:
		return nullptr;
	}
}

static void GiveWeapons(int pyrn)
{
	PLAYER& plr = player[pyrn];
	plr.weapon[1] = 1; //DAGGER
	plr.weapon[2] = 1; //MORNINGSTAR
	plr.weapon[3] = 1; //SHORT SWORD
	plr.weapon[4] = 1; //BROAD SWORD
	plr.weapon[5] = 1; //BATTLE AXE
	plr.weapon[6] = 1; // BOW
	plr.weapon[7] = 2; //PIKE
	plr.weapon[8] = 1;  //TWO HANDED
	plr.weapon[9] = 1; 
	//plr.currweapon = plr.selectedgun = 4;
}

static void GiveAmmo(int pyrn)
{
	PLAYER& plr = player[pyrn];
	plr.ammo[1] = 45; //DAGGER
	plr.ammo[2] = 55; //MORNINGSTAR
	plr.ammo[3] = 50; //SHORT SWORD
	plr.ammo[4] = 80; //BROAD SWORD
	plr.ammo[5] = 100; //BATTLE AXE
	plr.ammo[6] = 50; // BOW
	plr.ammo[7] = 40; //PIKE
	plr.ammo[8] = 250; //TWO HANDED
	plr.ammo[9] = 50;
}

static void GiveKeys(int pyrn)
{
	PLAYER& plr = player[pyrn];
	plr.treasure[TBRASSKEY] = 1;
	plr.treasure[TBLACKKEY] = 1;
	plr.treasure[TGLASSKEY] = 1;
	plr.treasure[TIVORYKEY] = 1;
}

static void GiveInventory(int pyrn)
{
	PLAYER& plr = player[pyrn];
	for (int i = 0; i < MAXPOTIONS; i++)
		plr.potion[i] = 9;

	for (int i = 0; i < 8; i++) {
		plr.orb[i] = 1;
		plr.orbammo[i] = 9;
	}
}

static void GiveArmor(int pyrn)
{
	PLAYER& plr = player[pyrn];
	plr.armor = 150;
	plr.armortype = 3;
}

static void GiveHealth(int pyrn)
{
	PLAYER& plr = player[pyrn];
	plr.health = 0;
	addhealth(plr, 200);
	plr.lvl = 7;
	plr.maxhealth = 200;
}

static void cmd_Give(int player, uint8_t** stream, bool skip)
{
	int type = ReadByte(stream);
	if (skip) return;

	if (numplayers != 1 || gamestate != GS_LEVEL)// || (Player[player].Flags & PF_DEAD))
	{
		Printf("give: Cannot give while dead or not in a single-player game.\n");
		return;
	}

	switch (type)
	{
	case GIVE_ALL:
		GiveWeapons(player);
		GiveAmmo(player);
		GiveKeys(player);
		GiveInventory(player);
		GiveArmor(player);
		GiveHealth(player);
		break;

	case GIVE_HEALTH:
		GiveHealth(player);
		break;

	case GIVE_WEAPONS:
		GiveWeapons(player);
		break;

	case GIVE_AMMO:
		GiveAmmo(player);
		break;

	case GIVE_ARMOR:
		GiveArmor(player);
		break;

	case GIVE_KEYS:
		GiveKeys(player);
		break;

	case GIVE_INVENTORY:
		GiveInventory(player);
		break;

	case GIVE_ITEMS:
		GiveWeapons(player);
		GiveAmmo(player);
		GiveKeys(player);
		GiveInventory(player);
		break;
	}
}

bool CheatScooter(cheatseq_t* c)
{
	if (CheckCheatmode())
	{
		GiveWeapons(pyrn);
		GiveAmmo(pyrn);
		PLAYER& plr = player[pyrn];
		plr.currweapon = plr.selectedgun = 4;
	}
	return true;
}

bool CheatMommy(cheatseq_t* c)
{
	if (CheckCheatmode())
	{
		PLAYER& plr = player[pyrn];
		for (int i = 0; i < MAXPOTIONS; i++)
			plr.potion[i] = 9;
	}
	return true;
}

bool CheatPowerup(cheatseq_t* c)
{
	if (CheckCheatmode())
	{
		PLAYER& plr = player[pyrn];
		weaponpowerup(plr);
	}
	return true;
}

bool CheatWango(cheatseq_t* c)
{
	if (CheckCheatmode())
	{
		PLAYER& plr = player[pyrn];
		for (int i = 0; i < 8; i++) {
			plr.orb[i] = 1;
			plr.orbammo[i] = 9;
		}
		GiveHealth(pyrn);
		GiveAmmo(pyrn);
		GiveKeys(pyrn);
	}
	return true;
}

bool CheatSpells(cheatseq_t* c)
{
    if (CheckCheatmode())
    {
        PLAYER& plr = player[pyrn];
        for (int i = 0; i < 8; i++) {
            plr.orb[i] = 1;
            plr.orbammo[i] = 9;
        }
    }
    return true;
}

static cheatseq_t whcheats[] = {
	{"scooter",   nullptr,     CheatScooter, 0},
	{"mommy",    nullptr,     CheatMommy, 0},
	{"wango",    nullptr,     CheatWango, 0},
	{"powerup",    nullptr,     CheatPowerup, 0},
};

static cheatseq_t wh2cheats[] = {
    {"weapons",   nullptr,     CheatScooter, 0},
    {"potions",    nullptr,     CheatMommy, 0},
    {"spells",    nullptr,     CheatSpells, 0},
    {"powerup",    nullptr,     CheatPowerup, 0},
};

#if 0 // to do
void
checkcheat(void)
{

    int  i, j, y = 24;
    struct player* plr;

    plr = &player[pyrn];

    strupr(displaybuf);

    else if (strcmp(displaybuf, "MARKETING") == 0) {
        if (godmode == 1) {
            godmode = 0;
        }
        else {
            godmode = 1;
            plr->health = 0;
            healthpic(200);
#if 0
            for (i = 0; i < MAXWEAPONS; i++) {
                plr->weapon[i] = 3;
            }
#endif
            GiveAmmo(player);
            GiveArmor(player);
            currweapon = selectedgun = 4;
            for (i = 0; i < MAXNUMORBS; i++) {
                plr->orb[i] = 1;
                plr->orbammo[i] = 9;
            }
            for (i = 0; i < MAXPOTIONS; i++) {
                plr->potion[i] = 9;
            }
            plr->armor = 150;
            plr->armortype = 3;
            plr->lvl = 7;
            plr->maxhealth = 200;
            for (i = 0; i < MAXTREASURES; i++) {
                plr->treasure[i] = 1;
            }
            nobreakflag = 1;
            updatepics();
        }
    }
    else if (strcmp(displaybuf, "KILLME") == 0) {
        plr->health = 0;
        plr->potion[0] = 0;
        playerdead(plr);
        updatepics();
    }
    else if (strcmp(displaybuf, "SCARE") == 0) {
        currentorb = 0;
        plr->orbammo[currentorb] += 1;
        orbshot = 1;
        activatedaorb(plr);
        updatepics();
    }
    else if (strcmp(displaybuf, "NIGHTVISION") == 0) {
        currentorb = 1;
        plr->orbammo[currentorb] += 1;
        orbshot = 1;
        activatedaorb(plr);
        updatepics();
    }
    else if (strcmp(displaybuf, "FREEZE") == 0) {
        currentorb = 2;
        plr->orbammo[currentorb] += 1;
        orbshot = 1;
        activatedaorb(plr);
        updatepics();
    }
    else if (strcmp(displaybuf, "MAGICARROW") == 0) {
        currentorb = 3;
        plr->orbammo[currentorb] += 1;
        orbshot = 1;
        activatedaorb(plr);
        updatepics();
    }
    else if (strcmp(displaybuf, "OPENDOOR") == 0) {
        currentorb = 4;
        plr->orbammo[currentorb] += 1;
        orbshot = 1;
        activatedaorb(plr);
        updatepics();
    }
    else if (strcmp(displaybuf, "FLY") == 0) {
        currentorb = 5;
        plr->orbammo[currentorb] += 1;
        orbshot = 1;
        activatedaorb(plr);
        updatepics();
    }
    else if (strcmp(displaybuf, "FIREBALL") == 0) {
        currentorb = 6;
        plr->orbammo[currentorb] += 1;
        orbshot = 1;
        activatedaorb(plr);
        updatepics();
    }
    else if (strcmp(displaybuf, "NUKE") == 0) {
        currentorb = 7;
        plr->orbammo[currentorb] += 1;
        orbshot = 1;
        activatedaorb(plr);
        updatepics();
    }
    else if (strcmp(displaybuf, "HEALTH") == 0) {
        i = currentpotion;
        currentpotion = 0;
        plr->potion[currentpotion] += 1;
        usapotion(plr);
        currentpotion = i;
        updatepics();
    }
    else if (strcmp(displaybuf, "STRENGTH") == 0) {
        i = currentpotion;
        currentpotion = 1;
        plr->potion[currentpotion] += 1;
        usapotion(plr);
        currentpotion = i;
        updatepics();
    }
    else if (strcmp(displaybuf, "CUREPOISON") == 0) {
        i = currentpotion;
        currentpotion = 2;
        plr->potion[currentpotion] += 1;
        usapotion(plr);
        currentpotion = i;
        updatepics();
    }
    else if (strcmp(displaybuf, "RESISTFIRE") == 0) {
        i = currentpotion;
        currentpotion = 3;
        plr->potion[currentpotion] += 1;
        usapotion(plr);
        currentpotion = i;
        updatepics();
    }
    else if (strcmp(displaybuf, "INVIS") == 0) {
        i = currentpotion;
        currentpotion = 4;
        plr->potion[currentpotion] += 1;
        usapotion(plr);
        currentpotion = i;
        updatepics();
    }
    else if (strcmp(displaybuf, "KEYS") == 0) {
        plr->treasure[14] = 1;
        plr->treasure[15] = 1;
        plr->treasure[16] = 1;
        plr->treasure[17] = 1;
        updatepics();
    }
    else if (strcmp(displaybuf, "PENTAGRAM") == 0) {
        plr->treasure[8] = 1;
    }
    else if (strcmp(displaybuf, "ARMOR") == 0) {
        plr->armortype = 3;
        armorpic(150);
        updatepics();
    }
    else if (strcmp(displaybuf, "HEROTIME") == 0) {
        helmettime = 7200;
    }
    else if (strcmp(displaybuf, "SHIELD") == 0) {
        droptheshield = 0;
        shieldtype = 1;
        shieldpoints = 100;
    }
    else if (strcmp(displaybuf, "SHIELD2") == 0) {
        droptheshield = 0;
        shieldtype = 2;
        shieldpoints = 200;
    }
    else if (strcmp(displaybuf, "NOBREAK") == 0) {
        nobreakflag ^= 1;
    }
    else if (strcmp(displaybuf, "SHOWOBJECTS") == 0) {
        show2dobjectsflag ^= 1;
    }
    else if (strcmp(displaybuf, "SHOWMAP") == 0) {
        show2dmapflag ^= 1;
    }
    else if (strcmp(displaybuf, "ENCHANT") == 0) {
        for (i = 0; i < MAXWEAPONS; i++) {
            plr->weapon[i] = 3;
        }
    }
    else if (strcmp(displaybuf, "INTRACORP") == 0) {
        if (svga == 1) {
            keystatus[0x39] = 0;
            keystatus[1] = 0;
            SND_Sound(S_PICKUPFLAG);
            permanentwritesprite(0, 0, STHEORDER, 0, 0, 0, 639, 239, 0);
            permanentwritesprite(0, 240, STHEORDER + 1, 0, 0, 240, 639, 479, 0);
            nextpage();
            i = 0;
            while (!i) {
                if (keystatus[0x39] > 0 || keystatus[1] > 0)
                    i = 1;
            }
            keystatus[0x39] = 0;
            keystatus[1] = 0;
        }
        else {
            keystatus[0x39] = 0;
            keystatus[1] = 0;
            SND_Sound(S_PICKUPFLAG);
            itemtoscreen(0L, 0L, THEORDER, 0, 0);
            nextpage();
            i = 0;
            while (!i) {
                if (keystatus[0x39] > 0 || keystatus[1] > 0)
                    i = 1;
            }
            keystatus[0x39] = 0;
            keystatus[1] = 0;
        }
        mflag = 1;
    }
    else if (strcmp(displaybuf, "SPIKEME") == 0) {
        spikeme = 1;
    }
    else if (strcmp(displaybuf, "EXPERIENCE") == 0) {
        score(10000);
    }
    else if (strcmp(displaybuf, "SCAREME") == 0) {
        scarytime = 180;
        scarysize = 30;
        SND_PlaySound(S_SCARYDUDE, 0, 0, 0, 0);
    }
    strcpy(displaybuf, "");
}
#endif

void InitCheats()
{
	if (!isWh2()) SetCheats(whcheats, countof(whcheats));
	else SetCheats(wh2cheats, countof(wh2cheats));
	Net_SetCommandHandler(DEM_GIVE, cmd_Give);
}


void MakeLevel(int i, int game)
{
	auto mi = AllocateMap();
	mi->fileName.Format("level%d.map", i);
	mi->labelName.Format("LEVEL%d", i);
	mi->name.Format("$TXT_WH%d_MAP%02d", game, i);
	mi->levelNumber = i;
}

void InitOriginalEpisodes()
{
	if(isWh2()) {
		/*
		const char *wh2names[] = {
				 "Antechamber of Asmodeus",
				 "Halls of Ragnoth",
				 "Lokis Tomb",
				 "Forsaken Realm",
				 "Eye of Midian",
				 "Dungeon of Disembowelment",
				 "Stronghold of Chaos",
				 "Jaws of Venom",
				 "Descent into Doom",
				 "Hack'n Sniff",
				 "Straits of Perdition",
				 "Plateau of Insanity",
				 "Crypt of Decay",
				 "Mausoleum of Madness",
				 "Gateway into Oblivion",
				 "Lungs of Hell",
			};
			*/
		for (int i = 1; i <= 15; i++)
			MakeLevel(i, 2);
		for (int i = 30; i <= 34; i++)
			MakeLevel(i, 2);
	} else {
		for (int i = 1; i <= 25; i++)
			MakeLevel(i, 1);
		for (int i = 30; i <= 35; i++)
			MakeLevel(i, 1);
	}
}


FString GameInterface::GetCoordString()
{
	return FStringf("Player X: %d, Y: %d, Z: %d", player[0].x, player[0].y, player[0].z);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void readpalettetable(void)
{
	auto fr = fileSystem.OpenFileReader("lookup.dat");
	if (!fr.isOpen())
		return;

	int j = lookups.loadTable(fr);

	if (j < 0)
	{
		if (j == -1)
			Printf("ERROR loading \"lookup.dat\": failed reading enough data.\n");

		return;
	}
}


void GameInterface::app_init()
{
	GameTicRate = TIMERRATE / TICSPERFRAME;
	InitNames();
	engineInit();

	TileFiles.LoadArtSet("tiles%03d.art");
	TileFiles.tileMakeWritable(ANILAVA);
	TileFiles.tileMakeWritable(HEALTHWATER);
	initlava();
	initwater();
	//ConsoleInit();
	g_visibility=1024;
	readpalettetable();
	TileFiles.SetBackup();
	InitFonts();
	connectpoint2[0] = -1;

	if(isWh2()) {
		tileDelete(FLOORMIRROR);
		uint8_t tempbuf[256];
		for (int j = 0; j < 256; j++)
			tempbuf[j] = (byte) ((j + 32) & 0xFF);     // remap colors for screwy palette sectors
		lookups.makeTable(16, tempbuf, 0, 0, 0, 1);
		for (int j = 0; j < 256; j++)
			tempbuf[j] = (byte) j;
		lookups.makeTable(17, tempbuf, 24, 24, 24, 1);

		for (int j = 0; j < 256; j++)
			tempbuf[j] = (byte) j;          // (j&31)+32;
		lookups.makeTable(18, tempbuf, 8, 8, 48, 1);
	}
	 
	FadeInit();
	setupmidi();
	sfxInit();
	//sndInit();
	//initpaletteshifts();
	InitOriginalEpisodes();

	numplayers = 1; 
	myconnectindex = 0;
	connecthead = 0; 
	connectpoint2[0] = -1;

	psky_t* pSky = tileSetupSky(0);
	pSky->tileofs[0] = 0;
	pSky->tileofs[1] = 0;
	pSky->tileofs[2] = 0;
	pSky->tileofs[3] = 0;
	pSky->yoffs = 256;
	pSky->lognumtiles = 2;
	pSky->horizfrac = 65536;
	pSky->yscale = 65536;
	parallaxtype = 2;
	g_visibility = 2048;
	enginecompatibility_mode = ENGINECOMPATIBILITY_19950829;
	initAI();
	InitItems();
	wepdatainit();
	InitCheats();
}

void GameInterface::Startup()
{
	if (userConfig.CommandMap.IsNotEmpty())
	{
	}
	else
	{
		IntroMovie([](bool)
			{
				gameaction = ga_mainmenu;
			});
	}
}

bool GameInterface::CanSave()
{
	return !player[pyrn].dead && numplayers == 1 && gamestate == GS_LEVEL;
}


void GameInterface::DrawBackground()
{
	if (isWh2()) 
	{
		DrawTexture(twod, tileGetTexture(VMAINBLANK), 0, 0, DTA_Fullscreen, FSMode_ScaleToFit43, TAG_DONE);
	}
	else
	{
		bool showmainmenu = CurrentMenu && CurrentMenu->GetClass()->TypeName == FName("WHMainMenu");
		// This only shows the low res menu, the larger one is rather poor because it places the actual menu into a postage stamp size window.
		DrawTexture(twod, tileGetTexture(showmainmenu ? MAINMENU : TITLEPIC), 0, 0, DTA_Fullscreen, FSMode_ScaleToFit43, TAG_DONE);
	}
}

bool playrunning()
{
	return (paused == 0 || multiplayer/* || demoplay/record*/);
}

static void recordoldspritepos()
{
	for (int i = 0; i < MAXSPRITES; i++)
	{
		sprite[i].backuploc();
	}
}

void GameInterface::Ticker() 
{
	// Make copies so that the originals do not have to be modified.
	for (int i = 0; i < MAXPLAYERS; i++)
	{
		auto oldactions = player[i].plInput.actions;
		player[i].plInput = playercmds[i].ucmd;
		if (oldactions & SB_CENTERVIEW) player[i].plInput.actions |= SB_CENTERVIEW;
	}

	if (!playrunning())
	{
		r_NoInterpolate = true;
		return;
	}

	for (int i = connecthead; i >= 0; i = connectpoint2[i])
		player[i].oldsector = player[i].sector;

	PLAYER &plr = player[pyrn];
	viewBackupPlayerLoc(pyrn);

	recordoldspritepos();

	processinput(pyrn);
	updateviewmap(plr);
	updatepaletteshifts();

	processobjs(plr);
	animateobjs(plr);
	animatetags(pyrn);
	doanimations();
	dodelayitems(TICSPERFRAME);
	dofx();
	speelbookprocess(plr);
	timerprocess(plr);
	weaponsprocess(pyrn);

	if (followmode) {
		followa += followang;

		followx += MulScale(-bsin(followa), followvel, 10);
		followy += MulScale(-bcos(followa), followvel, 10);

		followx += MulScale(-bcos(followa), followsvel, 10);
		followy -= MulScale(-bsin(followa), followsvel, 10);
	}

	PlayClock += TICSPERFRAME;
	r_NoInterpolate = false;
}

void GameInterface::LevelCompleted(MapRecord* map, int skill)
{
	if (map)
	{
		STAT_Update(false);
		auto pplr = &player[pyrn];
		auto completion = [=](bool)
		{
			spritesound(S_CHAINDOOR1, &sprite[pplr->spritenum]);
			playertorch = 0;
			spritesound(S_WARP, &sprite[pplr->spritenum]);
		};
		if (isWh2()) {
			showStatisticsScreen(completion);
			return;
		}
		completion(false);
	}
	else if (!isWh2())
	{
		STAT_Update(true);
		showVictoryScreen([=](bool)
			{
				gameaction = ga_mainmenu;
			});
	}
	else
	{
		STAT_Update(true);
		startWh2Ending([](bool) {
			gameaction = ga_mainmenu;
			});
	}
}

void GameInterface::NextLevel(MapRecord* map, int skill)
{
	mapon = map->levelNumber;
	currentLevel = map;
	difficulty = skill;
	nextlevel = true;
	prepareboard(currentLevel->fileName);
	STAT_NewLevel(currentLevel->labelName);
}


void GameInterface::NewGame(MapRecord* map, int skill) 
{
	//pNet.ready2send = false;
	//game.nNetMode = NetMode.Single;

	mapon = map->levelNumber;
	currentLevel = map;
	difficulty = skill;
	justteleported = false;
	nextlevel = false;
	Mus_Stop();
	prepareboard(currentLevel->fileName);
	STAT_StartNewGame(isWh2() ? "Witchaven2" : "Witchaven", skill);
	STAT_NewLevel(currentLevel->labelName);
}

bool GameInterface::StartGame(FNewGameStartup& gs)
{
	auto map = FindMapByLevelNum(1);
	DeferedStartGame(map, gs.Skill);
	return true;

}


void GameInterface::MenuSound(EMenuSounds snd)
{
	if (!isWh2()) SND_Sound(85);
	else SND_Sound(59);
}

void GameInterface::MenuOpened()
{
	if (!isWh2()) SND_Sound(85);
	else SND_Sound(59);
}

FSavegameInfo GameInterface::GetSaveSig()
{
	return { SAVESIG_DN3D, MINSAVEVER_DN3D, SAVEVER_DN3D };
}

void GameInterface::QuitToTitle()
{
	Mus_Stop();
	gameaction = ga_mainmenu;
}

::GameStats GameInterface::getStats()
{
	return { kills, killcnt, treasuresfound, treasurescnt, PlayClock / 30, 0 };
}

::GameInterface* CreateInterface()
{
	return new GameInterface;
}


END_WH_NS
