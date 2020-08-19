//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020 - Christoph Oelckers

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//------------------------------------------------------------------------- 

// all code related to startup, up to entering the main loop.

#include "ns.h"	// Must come before everything else!

#include "duke3d.h"
#include "baselayer.h"
#include "m_argv.h"
#include "mapinfo.h"
#include "texturemanager.h"
#include "statusbar.h"
#include "st_start.h"
#include "i_interface.h"
#include "prediction.h"
#include "glbackend/glbackend.h"
#include "gamestate.h"

BEGIN_DUKE_NS

void SetDispatcher();
void InitCheats();
int registerosdcommands(void);
void registerinputcommands(void);

//---------------------------------------------------------------------------
//
// game specific command line args go here. 
//
//---------------------------------------------------------------------------

static void checkcommandline()
{
	auto val = Args->CheckValue("-skill");
	if (!val) val = Args->CheckValue("-s");
	if (val)
	{
		ud.m_player_skill = ud.player_skill = clamp((int)strtol(val, nullptr, 0), 0, 5);
		if (ud.m_player_skill == 4) ud.m_respawn_monsters = ud.respawn_monsters = 1;
	}
	val = Args->CheckValue("-respawn");
	if (!val) val = Args->CheckValue("-t");
	if (val)
	{
		if (*val == '1') ud.m_respawn_monsters = 1;
		else if (*val == '2') ud.m_respawn_items = 1;
		else if (*val == '3') ud.m_respawn_inventory = 1;
		else
		{
			ud.m_respawn_monsters = 1;
			ud.m_respawn_items = 1;
			ud.m_respawn_inventory = 1;
		}
		Printf("Respawn on.\n");
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void genspriteremaps(void)
{
	int j;

	auto fr = fileSystem.OpenFileReader("lookup.dat");
	if (!fr.isOpen())
		return;

	j = lookups.loadTable(fr);

	if (j < 0)
	{
		if (j == -1)
			Printf("ERROR loading \"lookup.dat\": failed reading enough data.\n");

		return;
	}

	uint8_t paldata[768];

	for (j = 1; j <= 5; j++)
	{
		if (fr.Read(paldata, 768) != 768)
			return;

		for (int k = 0; k < 768; k++) // Build uses 6 bit VGA palettes.
			paldata[k] = (paldata[k] << 2) | (paldata[k] >> 6);

		paletteSetColorTable(j, paldata, j == DREALMSPAL || j == ENDINGPAL, j > SLIMEPAL);
	}

	for (int i = 0; i < 256; i++)
	{
		// swap red and blue channels.
		paldata[i * 3] = GPalette.BaseColors[i].b;
		paldata[i * 3 + 1] = GPalette.BaseColors[i].g;
		paldata[i * 3 + 2] = GPalette.BaseColors[i].r;
	}
	paletteSetColorTable(DRUGPAL, paldata, false, false); // todo: implement this as a shader effect (swap R and B in postprocessing.)

	if (isRR())
	{
		uint8_t table[256];
		for (j = 0; j < 256; j++)
			table[j] = j;
		for (j = 0; j < 32; j++)
			table[j] = j + 32;

		lookups.makeTable(7, table, 0, 0, 0, 0);

		for (j = 0; j < 256; j++)
			table[j] = j;
		lookups.makeTable(30, table, 0, 0, 0, 0);
		lookups.makeTable(31, table, 0, 0, 0, 0);
		lookups.makeTable(32, table, 0, 0, 0, 0);
		lookups.makeTable(33, table, 0, 0, 0, 0);
		if (isRRRA())
			lookups.makeTable(105, table, 0, 0, 0, 0);

		int unk = 63;
		for (j = 64; j < 80; j++)
		{
			unk--;
			table[j] = unk;
			table[j + 16] = j - 24;
		}
		table[80] = 80;
		table[81] = 81;
		for (j = 0; j < 32; j++)
		{
			table[j] = j + 32;
		}
		lookups.makeTable(34, table, 0, 0, 0, 0);
		for (j = 0; j < 256; j++)
			table[j] = j;
		for (j = 0; j < 16; j++)
			table[j] = j + 129;
		for (j = 16; j < 32; j++)
			table[j] = j + 192;
		lookups.makeTable(35, table, 0, 0, 0, 0);
		if (isRRRA())
		{
			lookups.makeTable(50, nullptr, 12 * 4, 12 * 4, 12 * 4, 0);
			lookups.makeTable(51, nullptr, 12 * 4, 12 * 4, 12 * 4, 0);
			lookups.makeTable(54, lookups.getTable(8), 32 * 4, 32 * 4, 32 * 4, 0);
		}
	}
}

//---------------------------------------------------------------------------
//
// Define sky layouts.
// This one's easy - the other games are a total mess
//
//---------------------------------------------------------------------------

static void setupbackdrop()
{
	static const uint16_t pskyoff[8] = {};
	static const uint16_t moonoff[8] = { 0, 2, 3, 0, 2, 0, 1, 0 };
	static const uint16_t orbitoff[8] = { 0, 0, 4, 0, 0, 1, 2, 3 };
	static const uint16_t laoff[8] = { 1, 2, 1, 3, 4, 0, 2, 3 };
	static const uint16_t defoff[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	static const uint16_t defoff1[8] = { 1, 2, 3, 4, 5, 6, 7, 0 };
	static const uint16_t defoff4[8] = { 4, 5, 6, 7, 0, 1, 2, 3 };
	static const uint16_t defoff7[8] = { 7, 0, 1, 2, 3, 4, 5, 6 };

	defineSky(DEFAULTPSKY, 32768, 3, pskyoff);
	defineSky(TILE_CLOUDYOCEAN, 65536, 3, pskyoff);
	defineSky(TILE_MOONSKY1, 32768, 3, moonoff);
	defineSky(TILE_BIGORBIT1, 32768, 3, orbitoff);
	defineSky(TILE_LA, 16384 + 1024, 3, laoff);
	if (isWorldTour())
	{
		defineSky(5284, 65536, 3, defoff);
		defineSky(5412, 65536, 3, defoff, 48);
		defineSky(5420, 65536, 3, defoff, 48);
		defineSky(5450, 65536, 3, defoff7, 48);
		defineSky(5548, 65536, 3, defoff, 48);
		defineSky(5556, 65536, 3, defoff1, 48);
		defineSky(5720, 65536, 3, defoff4, 48);
		defineSky(5814, 65536, 3, defoff, 48);
	}

	// Ugh... Since we do not know up front which of these tiles are skies we have to set them all...
	if (isRRRA())
	{
		for (int i = 0; i < MAXUSERTILES; i++)
		{
			if (tilesiz[i].x == 512)
			{
				defineSky(i, 32768, 1, pskyoff);
			}
			else if (tilesiz[i].x == 1024)
			{
				defineSky(i, 32768, 0, pskyoff);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void SetupGameButtons()
{
	static const char* actions[] = {
		"Move_Forward",
		"Move_Backward",
		"Turn_Left",
		"Turn_Right",
		"Strafe",
		"Fire",
		"Open",
		"Run",
		"Jump",
		"Crouch",
		"Look_Up",
		"Look_Down",
		"Look_Left",
		"Look_Right",
		"Strafe_Left",
		"Strafe_Right",
		"Aim_Up",
		"Aim_Down",
		"Map",
		"Shrink_Screen",
		"Enlarge_Screen",
		"Show_Opponents_Weapon",
		"Map_Follow_Mode",
		"See_Coop_View",
		"Mouse_Aiming",
		"Toggle_Crosshair",
		"Quick_Kick",
		"Dpad_Select",
		"Dpad_Aiming",
		"Third_Person_View",
		"Toggle_Crouch",
	};
	buttonMap.SetButtons(actions, NUM_ACTIONS);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void ticker(void)
{
	S_Update();

	// we need CONTROL_GetInput in order to pick up joystick button presses
	if (gamestate != GS_LEVEL || (paused && !System_WantGuiCapture()))
	{
		ControlInfo noshareinfo;
		CONTROL_GetInput(&noshareinfo);
		C_RunDelayedCommands();
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void loaddefs()
{
	const char* defsfile = G_DefFile();
	cycle_t deftimer;
	deftimer.Reset();
	deftimer.Clock();
	if (!loaddefinitionsfile(defsfile))
	{
		deftimer.Unclock();
		Printf("Definitions file \"%s\" loaded in %.3f ms.\n", defsfile, deftimer.TimeMS());
	}
	userConfig.AddDefs.reset();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void initTiles()
{
	if (TileFiles.artLoadFiles("tiles%03i.art") < 0)
		I_FatalError("Failed loading art.");

	tileDelete(TILE_MIRROR);
	skiptile = TILE_W_FORCEFIELD + 1;

	if (isRR())
		tileDelete(0);

	tileDelete(FOF);
}

//---------------------------------------------------------------------------
//
// set up the game module's state
//
//---------------------------------------------------------------------------

static void Startup(void)
{
	if (isRR()) C_SetNotifyFontScale(2);
	ud.god = 0;
	ud.m_respawn_items = 0;
	ud.m_respawn_monsters = 0;
	ud.m_respawn_inventory = 0;
	ud.cashman = 0;
	ud.m_player_skill = ud.player_skill = 2;
	ud.wchoice[0][0] = 3;
	ud.wchoice[0][1] = 4;
	ud.wchoice[0][2] = 5;
	ud.wchoice[0][3] = 7;
	ud.wchoice[0][4] = 8;
	ud.wchoice[0][5] = 6;
	ud.wchoice[0][6] = 0;
	ud.wchoice[0][7] = 2;
	ud.wchoice[0][8] = 9;
	ud.wchoice[0][9] = 1;
	ud.multimode = 1;
	ud.m_monsters_off = userConfig.nomonsters;
	ps[0].aim_mode = 1;
	ud.camerasprite = -1;

	if (fileSystem.FileExists("DUKESW.BIN"))
		g_gameType |= GAMEFLAG_SHAREWARE;

	numplayers = 1;
	playerswhenstarted = ud.multimode;

	connectpoint2[0] = -1;

	SetDispatcher();
	S_InitSound();

	timerInit(TICRATE);
	timerSetCallback(ticker);

	loadcons();
	fi.initactorflags();

	OnEvent(EVENT_INIT);

	enginecompatibility_mode = ENGINECOMPATIBILITY_19961112;

	if (engineInit())
		G_FatalEngineError();

	setupbackdrop();
	//Net_SendClientInfo();

	initTiles();
	fi.InitFonts();
	genspriteremaps();
	TileFiles.PostLoadSetup();
	SetupGameButtons();
	InitCheats();
	checkcommandline();
	registerosdcommands();
	registerinputcommands();

	screenpeek = myconnectindex;
	ps[myconnectindex].palette = BASEPAL;

	for (int j = numplayers; j < ud.multimode; j++)
	{
		mysnprintf(ud.user_name[j], sizeof(ud.user_name[j]), "%s %d", GStrings("PLAYER"), j + 1);
		ps[j].auto_aim = 0;
	}

	loaddefs();

	if (ud.multimode > 1)
	{
		ud.m_monsters_off = 1;
		ud.m_player_skill = 0;
	}

	ud.last_level = -1;
}

//---------------------------------------------------------------------------
//
// main entry point, sets up the game module and the engine, then enters the main loop
//
//---------------------------------------------------------------------------

void app_loop();
int GameInterface::app_main()
{
	Startup();
	enginePostInit();
	videoInit();
	app_loop();
	return 0;
}


END_DUKE_NS
