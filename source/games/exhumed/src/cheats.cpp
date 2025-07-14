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
#include "automap.h"
#include "engine.h"
#include "exhumed.h"
#include "sequence.h"
#include "names.h"
#include "player.h"
#include "sound.h"
#include "view.h"
#include "version.h"
#include "gamecvars.h"
#include "savegamehelp.h"
#include "c_dispatch.h"
#include "raze_sound.h"
#include "gamestate.h"
#include "c_console.h"
#include "cheathandler.h"
#include "inputstate.h"
#include "d_protocol.h"
#include "gstrings.h"
#include "aistuff.h"
#include "d_net.h"
#include "i_protocol.h"

BEGIN_PS_NS

enum {
    kButtonCheatGuns = 0x20,
    kButtonCheatGodMode = 0x40,
    kButtonCheatKeys = 0x80,
    kButtonCheatItems = 0x100,
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static const char* GodCheat(DExhumedPlayer* const pPlayer, int state)
{
	if (state == -1)
	{
		if (pPlayer->invincibility >= 0)
			pPlayer->invincibility = -1;
		else
			pPlayer->invincibility = 0;
	}
	else pPlayer->invincibility = -state;

	return GStrings.GetString(pPlayer->invincibility ? "TXT_EX_DEITYON" : "TXT_EX_DEITYOFF");
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static const char* SlipCheat()
{
	if (bSlipMode == false)
	{
		bSlipMode = true;
		return GStrings.GetString("TXT_EX_SLIPON");
	}
	else
	{
		bSlipMode = false;
		return GStrings.GetString("TXT_EX_SLIPOFF");
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

const char* GameInterface::GenericCheat(int player, int cheat)
{
	const auto pPlayer = getPlayer(player);

	switch (cheat)
	{
	case CHT_GOD:
		return GodCheat(pPlayer, -1);

	case CHT_GODOFF:
		return GodCheat(pPlayer, 0);

	case CHT_GODON:
		return GodCheat(pPlayer, 1);

	case CHT_NOCLIP:
		return SlipCheat();

	default:
		return nullptr;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool HollyCheat(cheatseq_t* c)
{
	// Do the closest thing to this cheat that's available.
	C_ToggleConsole();
	return true;
}

static bool KimberlyCheat(cheatseq_t* c)
{
	Printf(PRINT_NOTIFY, "%s\n", GStrings.GetString("TXT_EX_SWEETIE"));
	return true;
}

static bool LiteCheat(cheatseq_t* c)
{
	Printf(PRINT_NOTIFY, "%s\n", GStrings.GetString("TXT_EX_FLASHES"));
	bDoFlashes = !bDoFlashes;
	g_visibility = 1024 + 512 - g_visibility;	// let there be light - for real!
	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool SnakeCheat(cheatseq_t* c)
{
	if (!nNetPlayerCount)
	{
		if (bSnakeCam == false)
		{
			bSnakeCam = true;
			Printf(PRINT_NOTIFY, "%s\n", GStrings.GetString("TXT_EX_SNAKEON"));
		}
		else {
			bSnakeCam = false;
			Printf(PRINT_NOTIFY, "%s\n", GStrings.GetString("TXT_EX_SNAKEOFF"));
		}
	}
	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool SphereCheat(cheatseq_t* c)
{
	Printf(PRINT_NOTIFY, "%s\n", GStrings.GetString("TXT_EX_FULLMAP"));
	gFullMap = !gFullMap; // only set the cheat flag so it can be toggled.
	bShowTowers = gFullMap;
	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static cheatseq_t excheats[] = {
	{"holly",       nullptr,   HollyCheat, 0},
	{"kimberly",    nullptr,   KimberlyCheat, 0},
	{"lobocop",     "give weapons" },
	{"lobodeity",   "god" },
	{"lobolite",    nullptr,   LiteCheat, 0},
	{"lobopick",    "give keys" },
	{"loboslip",    "noclip" },
	{"lobosnake",   nullptr,   SnakeCheat },
	{"lobosphere",  nullptr,   SphereCheat, 0},
	{"loboswag",    "give inventory" },
	{"loboxy",      "stat printcoords",   nullptr, true},
};


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void cmd_Give(int player, TArrayView<uint8_t>& stream, bool skip)
{
	int type = ReadInt8(stream);
	if (skip) return;

	const auto pPlayer = getPlayer(player);

	if (pPlayer->nHealth <= 0 || nNetPlayerCount || gamestate != GS_LEVEL)
	{
		Printf("give: Cannot give while dead or not in a single-player game.\n");
		return;
	}
	int buttons = 0;

	switch (type)
	{
	case GIVE_ALL:
		buttons |= kButtonCheatGuns | kButtonCheatItems | kButtonCheatKeys;
		break;

	case GIVE_HEALTH:
		pPlayer->nHealth = 800;
		return;

	case GIVE_WEAPONS:
	case GIVE_AMMO:
		buttons |= kButtonCheatGuns;
		break;

	case GIVE_ARMOR:
		// not implemented
		break;

	case GIVE_KEYS:
		buttons |= kButtonCheatKeys;
		break;

	case GIVE_INVENTORY:
		buttons |= kButtonCheatItems;
		break;

	case GIVE_ITEMS:
		buttons |= kButtonCheatItems | kButtonCheatKeys;
		break;
	}

	if (buttons & kButtonCheatGuns) // LOBOCOP cheat
	{
		FillWeapons(pPlayer);
		if (player == myconnectindex) Printf(PRINT_NOTIFY, "%s\n", GStrings.GetString("TXT_EX_WEAPONS"));
	}
	if (buttons & kButtonCheatKeys) // LOBOPICK cheat
	{
		pPlayer->keys = 0xFFFF;
		if (player == myconnectindex) Printf(PRINT_NOTIFY, "%s\n", GStrings.GetString("TXT_EX_KEYS"));
	}
	if (buttons & kButtonCheatItems) // LOBOSWAG cheat
	{
		FillItems(pPlayer);
		if (player == myconnectindex) Printf(PRINT_NOTIFY, "%s\n", GStrings.GetString("TXT_EX_ITEMS"));
	}

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitCheats()
{
	SetCheats(excheats, countof(excheats));
	Net_SetCommandHandler(DEM_GIVE, cmd_Give);
}

END_PS_NS
