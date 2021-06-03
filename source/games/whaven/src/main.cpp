#include "ns.h"
#include "wh.h"
#include "mapinfo.h"
#include "gamestate.h"
#include "gamefuncs.h"
#include "buildtiles.h"
#include "v_draw.h"
#include "menu.h"
#include "d_net.h"
#include "raze_music.h"
#include "statistics.h"
#include "version.h"
#include "cheathandler.h"
#include "screenjob_.h"
#include "vm.h"

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

void GameInterface::loadPalette()
{
	paletteLoadFromDisk();
	readpalettetable();

	if (isWh2()) {
		tileDelete(FLOORMIRROR);
		uint8_t tempbuf[256];
		for (int j = 0; j < 256; j++)
			tempbuf[j] = (byte)((j + 32) & 0xFF);     // remap colors for screwy palette sectors
		lookups.makeTable(16, tempbuf, 0, 0, 0, 1);
		for (int j = 0; j < 256; j++)
			tempbuf[j] = (byte)j;
		lookups.makeTable(17, tempbuf, 24, 24, 24, 1);

		for (int j = 0; j < 256; j++)
			tempbuf[j] = (byte)j;          // (j&31)+32;
		lookups.makeTable(18, tempbuf, 8, 8, 48, 1);
	}

	// Remap table for the menu font.
	uint8_t remapbuf[256];
	for (int i = 0; i < 256; i++) remapbuf[i] = i;
	for (int i = 242; i < 252; i++) //yellow to green
		remapbuf[i] = (uint8_t)(368 - i);
	//for(int i = 117; i < 127; i++) //green to yellow
		//remapbuf[i] = (uint8_t) (368 - i);
	lookups.makeTable(20, remapbuf, 0, 0, 0, true);

}



void GameInterface::app_init()
{
	InitFonts();

	GameTicRate = TIMERRATE / TICSPERFRAME;
	InitNames();
	engineInit();

	TileFiles.tileMakeWritable(ANILAVA);
	TileFiles.tileMakeWritable(HEALTHWATER);
	initlava();
	initwater();
	//ConsoleInit();
	g_visibility=1024;
	connectpoint2[0] = -1;
	 
	setupmidi();
	sfxInit();
	//sndInit();
	//initpaletteshifts();

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
	PlayLogos(ga_mainmenu, ga_mainmenu, true);
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
	spikeanimation(plr);

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
	Mus_Stop();

	SummaryInfo info{};

	if (kills > killcnt) kills = killcnt;
	int killp = (kills * 100) / (killcnt + 1);
	if (treasuresfound > treasurescnt) treasuresfound = treasurescnt;
	int treap = (treasuresfound * 100) / (treasurescnt + 1);
	int rating = (killp + treap) / 2;
	if (rating >= 95) rating = 3;
	else if (rating >= 70) rating = 2;
	else if (rating >= 40) rating = 1;
	else rating = 0;

	info.kills = kills;
	info.maxkills = killcnt;
	info.secrets = treasuresfound;
	info.maxsecrets = treasurescnt;
	info.supersecrets = rating;
	info.time = PlayClock * GameTicRate / 120;
	info.endofgame = map == nullptr;

	info.bonus = rating * 500;
	player[pyrn].score += info.bonus;
	info.score = info.bonus + expgained;

	ShowIntermission(currentLevel, map, &info, [=](bool)
		{
			soundEngine->StopAllChannels();
			auto pplr = &player[pyrn];
			SND_Sound(S_CHAINDOOR1);
			playertorch = 0;
			SND_Sound(S_WARP);
			gameaction = map ? ga_nextlevel : ga_mainmenu;
		});
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


void GameInterface::NewGame(MapRecord* map, int skill, bool) 
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
	STAT_NewLevel(currentLevel->labelName);
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

void GameInterface::ToggleThirdPerson()
{
	if (gamestate == GS_LEVEL)
	{
		auto pplr = &player[pyrn];

		if (pplr->over_shoulder_on)
		{
			pplr->over_shoulder_on = false;
		}
		else
		{
			pplr->over_shoulder_on = true;
			cameradist = 0;
			cameraclock = INT_MIN;
		}
	}
}

::GameStats GameInterface::getStats()
{
	return { kills, killcnt, treasuresfound, treasurescnt, PlayClock / 30, 0 };
}

::GameInterface* CreateInterface()
{
	return new GameInterface;
}


DEFINE_FIELD_X(WhPlayer,PLAYER,spellnum);
DEFINE_FIELD_X(WhPlayer,PLAYER,x);
DEFINE_FIELD_X(WhPlayer,PLAYER,y);
DEFINE_FIELD_X(WhPlayer,PLAYER,z);
DEFINE_FIELD_X(WhPlayer,PLAYER,height);
DEFINE_FIELD_X(WhPlayer,PLAYER,hvel);
DEFINE_FIELD_X(WhPlayer,PLAYER, sector);
DEFINE_FIELD_X(WhPlayer,PLAYER, oldsector);
DEFINE_FIELD_X(WhPlayer,PLAYER, spritenum);
DEFINE_FIELD_X(WhPlayer,PLAYER, keytoggle);
DEFINE_FIELD_X(WhPlayer,PLAYER,flags);
DEFINE_FIELD_X(WhPlayer,PLAYER,weapon);
DEFINE_FIELD_X(WhPlayer, PLAYER, preenchantedweapon);
DEFINE_FIELD_X(WhPlayer,PLAYER,ammo);
DEFINE_FIELD_X(WhPlayer, PLAYER, preenchantedammo);
DEFINE_FIELD_X(WhPlayer,PLAYER,orbammo);
DEFINE_FIELD_X(WhPlayer,PLAYER,treasure);
DEFINE_FIELD_X(WhPlayer,PLAYER,orbactive);
DEFINE_FIELD_X(WhPlayer,PLAYER,orb);
DEFINE_FIELD_X(WhPlayer,PLAYER,potion);
DEFINE_FIELD_X(WhPlayer,PLAYER,lvl);
DEFINE_FIELD_X(WhPlayer,PLAYER,score);
DEFINE_FIELD_X(WhPlayer,PLAYER,health);
DEFINE_FIELD_X(WhPlayer,PLAYER,maxhealth);
DEFINE_FIELD_X(WhPlayer,PLAYER,armor);
DEFINE_FIELD_X(WhPlayer,PLAYER,armortype);
DEFINE_FIELD_X(WhPlayer,PLAYER,onsomething);
DEFINE_FIELD_X(WhPlayer,PLAYER,fallz);
DEFINE_FIELD_X(WhPlayer,PLAYER, dead);
DEFINE_FIELD_X(WhPlayer,PLAYER,shadowtime);
DEFINE_FIELD_X(WhPlayer,PLAYER,helmettime);
DEFINE_FIELD_X(WhPlayer,PLAYER,scoretime);
DEFINE_FIELD_X(WhPlayer,PLAYER,vampiretime);
DEFINE_FIELD_X(WhPlayer,PLAYER,selectedgun);
DEFINE_FIELD_X(WhPlayer,PLAYER,currweapon);
DEFINE_FIELD_X(WhPlayer,PLAYER,currweapontics);
DEFINE_FIELD_X(WhPlayer,PLAYER,currweaponanim);
DEFINE_FIELD_X(WhPlayer,PLAYER,currweaponframe);
DEFINE_FIELD_X(WhPlayer,PLAYER,currweaponfired);
DEFINE_FIELD_X(WhPlayer,PLAYER,currweaponattackstyle);
DEFINE_FIELD_X(WhPlayer,PLAYER,currweaponflip);
DEFINE_FIELD_X(WhPlayer,PLAYER,hasshot);
DEFINE_FIELD_X(WhPlayer,PLAYER,currentpotion);
DEFINE_FIELD_X(WhPlayer,PLAYER,strongtime);
DEFINE_FIELD_X(WhPlayer,PLAYER,manatime);
DEFINE_FIELD_X(WhPlayer,PLAYER,invisibletime);
DEFINE_FIELD_X(WhPlayer,PLAYER,orbshot);
DEFINE_FIELD_X(WhPlayer,PLAYER,spellbooktics);
DEFINE_FIELD_X(WhPlayer,PLAYER,spellbook);
DEFINE_FIELD_X(WhPlayer,PLAYER,spellbookframe);
DEFINE_FIELD_X(WhPlayer,PLAYER,spellbookflip);
DEFINE_FIELD_X(WhPlayer,PLAYER,nightglowtime);
DEFINE_FIELD_X(WhPlayer,PLAYER,showbook);
DEFINE_FIELD_X(WhPlayer,PLAYER,showbooktype);
DEFINE_FIELD_X(WhPlayer,PLAYER,showbookflip);
DEFINE_FIELD_X(WhPlayer,PLAYER,showbookanim);
DEFINE_FIELD_X(WhPlayer,PLAYER,currentorb);
DEFINE_FIELD_X(WhPlayer,PLAYER,spelltime);
DEFINE_FIELD_X(WhPlayer,PLAYER,shieldpoints);
DEFINE_FIELD_X(WhPlayer,PLAYER,shieldtype);
DEFINE_FIELD_X(WhPlayer,PLAYER,poisoned);
DEFINE_FIELD_X(WhPlayer,PLAYER,poisontime);
DEFINE_FIELD_X(WhPlayer,PLAYER,shockme);
DEFINE_FIELD_X(WhPlayer,PLAYER,invincibletime);
DEFINE_FIELD_X(WhPlayer,PLAYER,spiked);
DEFINE_FIELD_X(WhPlayer,PLAYER,spiketics);
DEFINE_FIELD_X(WhPlayer,PLAYER,spikeframe);
DEFINE_FIELD_X(WhPlayer,PLAYER,currspikeframe);
DEFINE_FIELD_X(WhPlayer,PLAYER, godMode);
DEFINE_FIELD_X(WhPlayer,PLAYER, noclip);
DEFINE_FIELD_X(WhPlayer,PLAYER, over_shoulder_on);
DEFINE_FIELD_X(WhPlayer, PLAYER, justwarpedfx);
DEFINE_FIELD_X(WhPlayer, PLAYER, justwarpedcnt);

DEFINE_FIELD_X(WhWeaponInf, WEAPONINF, daweapontics);
DEFINE_FIELD_X(WhWeaponInf, WEAPONINF, daweaponframe);
DEFINE_FIELD_X(WhWeaponInf, WEAPONINF, currx);
DEFINE_FIELD_X(WhWeaponInf, WEAPONINF, curry);

DEFINE_ACTION_FUNCTION(_WhPlayer, GetSpellbookAnim)
{
	PARAM_SELF_STRUCT_PROLOGUE(PLAYER);
	ACTION_RETURN_POINTER(&sspellbookanim[self->currentorb][8]);
}

DEFINE_ACTION_FUNCTION(_Witchaven, GetViewPlayer)
{
	ACTION_RETURN_POINTER(&player[myconnectindex]);
}
END_WH_NS
