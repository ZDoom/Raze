/*
** cheats.cpp
**	Common cheat code
**
**---------------------------------------------------------------------------
// Copyright 1999-2016 Randy Heit
// Copyright 2002-2020 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OFf
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "build.h"
#include "gamestruct.h"
#include "printf.h"
#include "c_cvars.h"
#include "cheathandler.h"
#include "c_dispatch.h"
#include "d_net.h"
#include "gamestate.h"
#include "mmulti.h"
#include "gstrings.h"
#include "gamecontrol.h"
#include "mapinfo.h"

CVAR(Bool, sv_cheats, true, CVAR_ARCHIVE|CVAR_SERVERINFO)
CVAR(Bool, cl_blockcheats, false, 0)

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool CheckCheatmode (bool printmsg, bool sponly)
{
	if ((sponly && netgame) || gamestate != GS_LEVEL)
	{
		if (printmsg) Printf ("Not in a singleplayer game.\n");
		return true;
	}
	else if ((netgame /*|| deathmatch*/) && (!sv_cheats))
	{
		if (printmsg) Printf ("sv_cheats must be true to enable this command.\n");
		return true;
	}
	else if (cl_blockcheats != 0)
	{
		if (printmsg && cl_blockcheats == 1) Printf ("cl_blockcheats is turned on and disabled this command.\n");
		return true;
	}
	else
	{
		const char *gamemsg = gi->CheckCheatMode(); // give the game anopportuity to add its own blocks.
		if (printmsg && gamemsg)
		{
			Printf("%s\n", gamemsg);
			return true;
		}
		return false;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void genericCheat(int player, uint8_t** stream, bool skip)
{
    int cheat = ReadByte(stream);
    if (skip) return;
	const char *msg = gi->GenericCheat(player, cheat);
    if (!msg || !*msg)              // Don't print blank lines.
        return;

    if (player == myconnectindex)
        Printf("%s\n", msg);
    else
    {
        FString message = GStrings("TXT_X_CHEATS");
        //message.Substitute("%s", player->userinfo.GetName()); // fixme - make globally accessible
        Printf("%s: %s\n", message.GetChars(), msg);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CCMD(god)
{
	if (!CheckCheatmode(true, true))	// Right now the god cheat is a global setting in some games and not a player property. This should be changed.
	{
		Net_WriteByte(DEM_GENERICCHEAT);
		Net_WriteByte(CHT_GOD);
	}
}

CCMD(godon)
{
	if (!CheckCheatmode(true, true))
	{
		Net_WriteByte(DEM_GENERICCHEAT);
		Net_WriteByte(CHT_GODON);
	}
}

CCMD(godoff)
{
	if (!CheckCheatmode(true, true))
	{
		Net_WriteByte(DEM_GENERICCHEAT);
		Net_WriteByte(CHT_GODOFF);
	}
}

CCMD(noclip)
{
	if (!CheckCheatmode(true, true))
	{
		Net_WriteByte(DEM_GENERICCHEAT);
		Net_WriteByte(CHT_NOCLIP);
	}
}

CCMD(allmap)
{
	if (!CheckCheatmode(true, false))
	{
		gFullMap = !gFullMap;
		Printf("%s\n", GStrings(gFullMap ? "SHOW MAP: ON" : "SHOW MAP: OFF"));
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CCMD(give)
{
	static const char* type[] = { "ALL","AMMO","ARMOR","HEALTH","INVENTORY","ITEMS","KEYS","WEAPONS",nullptr };
	if (argv.argc() < 2)
	{
		Printf("give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item\n");
		return;
	}
	size_t found = -1;
	for (size_t i = 0; i < countof(type); i++)
	{
		if (!stricmp(argv[1], type[i]))
		{
			found = i;
			break;
		}
	}
	if (found == -1)
	{
		Printf("Unable to give %s\n", argv[1]);
	}
	if (!CheckCheatmode(true, true))
	{
		Net_WriteByte(DEM_GIVE);
		Net_WriteByte(found);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void CompleteLevel(MapRecord* map)
{
	gameaction = ga_completed;
	g_nextmap = !currentLevel || !(currentLevel->flags & MI_FORCEEOG)? map : nullptr;
	g_nextskill = -1;	// This does not change the skill
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void changeMap(int player, uint8_t** stream, bool skip)
{
	int skill = (int8_t)ReadByte(stream);
	auto mapname = ReadStringConst(stream);
	if (skip) return;
	auto map = FindMapByName(mapname);
	if (map || *mapname == 0)	// mapname = "" signals end of episode
	{
		gameaction = ga_completed;
		g_nextmap = map;
		g_nextskill = skill;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ChangeLevel(MapRecord* map, int skill)
{
	Net_WriteByte(DEM_CHANGEMAP);
	Net_WriteByte(skill);
	Net_WriteString(map? map->labelName : nullptr);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DeferedStartGame(MapRecord* map, int skill)
{
	g_nextmap = map;
	g_nextskill = skill;
	gameaction = ga_newgame;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static MapRecord* levelwarp_common(FCommandLine& argv, const char *cmdname, const char *t2)
{
	int numparm = g_gameType & (GAMEFLAG_SW | GAMEFLAG_PSEXHUMED) ? 1 : 2;	// Handle games with episodic and non-episodic level order.
	if (argv.argc() <= numparm)
	{
		if (numparm == 2) Printf(PRINT_BOLD,  "%s <e> <m>: %s episode 'e' and map 'm'\n", cmdname, t2);
		else Printf(PRINT_BOLD, "%s <m>: %s map 'm'\n", cmdname, t2);
		return nullptr;
	}
	// Values are one-based.
	int e = numparm == 2 ? atoi(argv[1]) : 0;
	int m = atoi(numparm == 2 ? argv[2] : argv[1]);
	if (e <= 0 || m <= 0)
	{
		Printf(PRINT_BOLD, "Invalid level! Numbers must be > 0\n");
		return nullptr;
	}
	auto map = FindMapByLevelNum(levelnum(e - 1, m - 1));
	if (!map)
	{
		if (numparm == 2) Printf(PRINT_BOLD, "Level E%s L%s not found!\n", argv[1], argv[2]);
		else Printf(PRINT_BOLD, "Level %s not found!\n", argv[1]);
		return nullptr;
	}
	if (fileSystem.FindFile(map->fileName) < 0)
	{
		Printf(PRINT_BOLD, "%s: map file not found\n", map->fileName.GetChars());
	}
	return map;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CCMD(levelwarp)
{
	if (gamestate != GS_LEVEL)
	{
		Printf("Use the startgame command when not in a game.\n");
		return;
	}

#if 0
	if (/*!players[consoleplayer].settings_controller &&*/ netgame)
	{
		Printf("Only setting controllers can change the map.\n");
		return;
	}
#endif

	auto map = levelwarp_common(argv, "levelwarp", "warp to");
	if (map)
	{
		ChangeLevel(map, -1);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CCMD(startgame)
{
	if (netgame)
	{
		Printf("Use " TEXTCOLOR_BOLD "levelwarp" TEXTCOLOR_NORMAL " instead. " TEXTCOLOR_BOLD "startgame"
			TEXTCOLOR_NORMAL " is for single-player only.\n");
		return;
	}
	auto map = levelwarp_common(argv, "start game", "start new game at");
	if (map)
	{
		DeferedStartGame(map, -1);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CCMD(changemap)
{
	if (argv.argc() < 2)
	{
		Printf(PRINT_BOLD, "changemap <mapname>: warp to the given map, identified by its name.\n");
		return;
	}
	if (gamestate != GS_LEVEL)
	{
		Printf("Use the map command when not in a game.\n");
		return;
	}

#if 0
	if (/*!players[consoleplayer].settings_controller &&*/ netgame)
	{
		Printf("Only setting controllers can change the map.\n");
		return;
	}
#endif

	FString mapname = argv[1];
	auto map = FindMapByName(mapname);
	if (map == nullptr)
	{
		// got a user map
		Printf(PRINT_BOLD, "%s: Map not defined.\n", mapname.GetChars());
		return;
	}
	if (map->flags & MI_USERMAP)
	{
		// got a user map
		Printf(PRINT_BOLD, "%s: Cannot warp to user maps.\n", mapname.GetChars());
		return;
	}
	if (fileSystem.FindFile(map->fileName) < 0)
	{
		Printf(PRINT_BOLD, "%s: map file not found\n", map->fileName.GetChars());
	}

	ChangeLevel(map, -1);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CCMD(map)
{
	if (argv.argc() < 2)
	{
		Printf(PRINT_BOLD, "map <mapname>: start new game at the given map, identified by its name.\n");
		return;
	}
	if (netgame)
	{
		Printf("Use " TEXTCOLOR_BOLD "changemap" TEXTCOLOR_NORMAL " instead. " TEXTCOLOR_BOLD "map"
			TEXTCOLOR_NORMAL " is for single-player only.\n");
		return;
	}

	FString mapname = argv[1];
	FString mapfilename = mapname;
	DefaultExtension(mapfilename, ".map");

	// Check if the map is already defined.
	auto map = FindMapByName(mapname);
	if (map == nullptr)
	{
		// got a user map
		if (g_gameType & GAMEFLAG_SHAREWARE)
		{
			Printf(PRINT_BOLD, "Cannot use user maps in shareware.\n");
			return;
		}
		map = SetupUserMap(mapfilename, g_gameType & GAMEFLAG_DUKE? "dethtoll.mid" : nullptr);
	}
	if (map)
	{
		if (fileSystem.FindFile(map->fileName) < 0)
		{
			Printf(PRINT_BOLD, "%s: map file not found\n", map->fileName.GetChars());
		}

		DeferedStartGame(map, -1);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CCMD(restartmap)
{
	if (gamestate != GS_LEVEL || currentLevel == nullptr)
	{
		Printf("Must be in a game to restart a level.\n");
		return;
	}
	ChangeLevel(currentLevel, -1);
}
