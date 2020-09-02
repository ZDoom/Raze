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
#include "common.h"
#include "engine.h"
#include "exhumed.h"
#include "sequence.h"
#include "names.h"
#include "player.h"
#include "ps_input.h"
#include "sound.h"
#include "view.h"
#include "status.h"
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

BEGIN_PS_NS


static const char* GodCheat(int nPlayer, int state)
{
	if (state == -1)
	{
		if (PlayerList[nPlayer].invincibility >= 0)
			PlayerList[nPlayer].invincibility = -1;
		else
			PlayerList[nPlayer].invincibility = 0;
	}
	else PlayerList[nPlayer].invincibility = -state;

	return GStrings(PlayerList[nPlayer].invincibility ? "TXT_EX_DEITYON" : "TXT_EX_DEITYOFF");
}

static const char* SlipCheat()
{
	if (bSlipMode == false)
	{
		bSlipMode = true;
		return GStrings("TXT_EX_SLIPON");
	}
	else
	{
		bSlipMode = false;
		return GStrings("TXT_EX_SLIPOFF");
	}
}



const char* GameInterface::GenericCheat(int player, int cheat)
{
	switch (cheat)
	{
	case CHT_GOD:
		return GodCheat(player, -1);

	case CHT_GODOFF:
		return GodCheat(player, 0);

	case CHT_GODON:
		return GodCheat(player, 1);

	case CHT_NOCLIP:
		return SlipCheat();

	default:
		return nullptr;
	}
}

static bool HollyCheat(cheatseq_t* c)
{
	// Do the closest thing to this cheat that's available.
	C_ToggleConsole();
	return true;
}

static bool KimberlyCheat(cheatseq_t* c)
{
	Printf(PRINT_NOTIFY, "%s\n", GStrings("TXT_EX_SWEETIE"));
	return true;
}

static bool LiteCheat(cheatseq_t* c)
{
	Printf(PRINT_NOTIFY, "%s\n", GStrings("TXT_EX_FLASHES"));
	bDoFlashes = !bDoFlashes;
	g_visibility = 2048 + 512 - g_visibility;	// let there be light - for real!
	return true;
}

static bool SnakeCheat(cheatseq_t* c)
{
	if (!nNetPlayerCount)
	{
		if (bSnakeCam == false)
		{
			bSnakeCam = true;
			Printf(PRINT_NOTIFY, "%s\n", GStrings("TXT_EX_SNAKEON"));
		}
		else {
			bSnakeCam = false;
			Printf(PRINT_NOTIFY, "%s\n", GStrings("TXT_EX_SNAKEOFF"));
		}
	}
	return true;
}

static bool SphereCheat(cheatseq_t* c)
{
	Printf(PRINT_NOTIFY, "%s\n", GStrings("TXT_EX_FULLMAP"));
	GrabMap();
	bShowTowers = true;
	return true;
}

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


void InitCheats()
{
	SetCheats(excheats, countof(excheats));
}

END_PS_NS
