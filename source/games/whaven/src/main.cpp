#include "ns.h"
#include "wh.h"
#include "mapinfo.h"
#include "gamestate.h"
#include "buildtiles.h"
#include "v_draw.h"
#include "menu.h"

BEGIN_WH_NS 

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

#if 0
		Console.RegisterCvar(new OSDCOMMAND("scooter",
			"", new OSDCVARFUNC() {
				public void execute() {
					if (isCurrentScreen(gGameScreen)) {
						PLAYER plr = player[pyrn];
						plr.weapon[1]=1;plr.ammo[1]=45; //DAGGER
						plr.weapon[2]=1;plr.ammo[2]=55; //MORNINGSTAR
						plr.weapon[3]=1;plr.ammo[3]=50; //SHORT SWORD
						plr.weapon[4]=1;plr.ammo[4]=80; //BROAD SWORD
						plr.weapon[5]=1;plr.ammo[5]=100; //BATTLE AXE
						plr.weapon[6]=1;plr.ammo[6]=50; // BOW
						plr.weapon[7]=2;plr.ammo[7]=40; //PIKE
						plr.weapon[8]=1;plr.ammo[8]=250; //TWO HANDED
						plr.weapon[9]=1;plr.ammo[9]=50;
						plr.currweapon=plr.selectedgun=4;
					} else
						Console.Println("scooter: not in a single-player game");
				}
			}));
		
		Console.RegisterCvar(new OSDCOMMAND("mommy",
			"", new OSDCVARFUNC() {
				public void execute() {
					if (isCurrentScreen(gGameScreen)) {
						PLAYER plr = player[pyrn];
						for(int i=0;i<MAXPOTIONS;i++) 
							plr.potion[i]=9;
					} else
						Console.Println("mommy: not in a single-player game");
				}
			}));
		Console.RegisterCvar(new OSDCOMMAND("wango",
			"", new OSDCVARFUNC() {
				public void execute() {
					if (isCurrentScreen(gGameScreen)) {
						PLAYER plr = player[pyrn];
						for(int i=0;i<8;i++) {
							plr.orb[i]=1;
							plr.orbammo[i]=9;
						}
						plr.health=0;
						addhealth(plr, 200);
						plr.armor=150;
						plr.armortype=3;
						plr.lvl=7;
						plr.maxhealth=200;
						plr.treasure[TBRASSKEY]=1;
						plr.treasure[TBLACKKEY]=1;
						plr.treasure[TGLASSKEY]=1;
						plr.treasure[TIVORYKEY]=1;
					} else
						Console.Println("wango: not in a single-player game");
				}
			}));
		
		Console.RegisterCvar(new OSDCOMMAND("powerup",
				"", new OSDCVARFUNC() {
					public void execute() {
						if (isCurrentScreen(gGameScreen)) {
							PLAYER plr = player[pyrn];
							weaponpowerup(plr);
						} else
							Console.Println("powerup: not in a single-player game");
					}
				}));
				
	}
#endif

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
	return false;// !bRecord && !bPlayback && !paused && !bInDemo && nTotalPlayers == 1;
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

::GameInterface* CreateInterface()
{
	return new GameInterface;
}

END_WH_NS
