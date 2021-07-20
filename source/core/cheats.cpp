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
#include "gstrings.h"
#include "gamecontrol.h"
#include "screenjob.h"
#include "mapinfo.h"
#include "statistics.h"

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
        Printf(PRINT_NOTIFY, "%s\n", msg);
    else
    {
        FString message = GStrings("TXT_X_CHEATS");
        //message.Substitute("%s", player->userinfo.GetName()); // fixme - make globally accessible
        Printf(PRINT_NOTIFY, "%s: %s\n", message.GetChars(), msg);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CCMD(fly)
{
	if (!CheckCheatmode(true, true))
	{
		Net_WriteByte(DEM_GENERICCHEAT);
		Net_WriteByte(CHT_FLY);
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CCMD(give)
{
	static const char* type[] = { "ALL","AMMO","ARMOR","HEALTH","INVENTORY","ITEMS","KEYS","WEAPONS" };
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
		return;
	}
	if (!CheckCheatmode(true, true))
	{
		Net_WriteByte(DEM_GIVE);
		Net_WriteByte((uint8_t)found);
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

void endScreenJob(int player, uint8_t** stream, bool skip)
{
	if (!skip) gameaction = ga_endscreenjob;
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

void DeferredStartGame(MapRecord* map, int skill, bool nostopsound)
{
	g_nextmap = map;
	g_nextskill = skill;
	gameaction = nostopsound? ga_newgamenostopsound : ga_newgame;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static MapRecord* levelwarp_common(FCommandLine& argv, const char *cmdname, const char *t2)
{
	int numparm = g_gameType & (GAMEFLAG_SW | GAMEFLAG_PSEXHUMED) ? 1 : 2;	// Handle games with episodic and non-episodic level order.
	if (numparm == 2 && argv.argc() == 2) numparm = 1;
	if (argv.argc() <= numparm)
	{
		if (numparm == 2) Printf(PRINT_BOLD,  "%s <e> <m>: %s episode 'e' and map 'm'\n", cmdname, t2);
		else Printf(PRINT_BOLD, "%s <m>: %s map 'm'\n", cmdname, t2);
		return nullptr;
	}
	// Values are one-based.
	int e = numparm == 2 ? atoi(argv[1]) : 1;
	int m = atoi(numparm == 2 ? argv[2] : argv[1]);
	if (e <= 0 || m <= 0)
	{
		Printf(PRINT_BOLD, "Invalid level! Numbers must be > 0\n");
		return nullptr;
	}
	auto map = FindMapByIndex(e, m);
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
	if (!gi->CanSave())
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
		ChangeLevel(map, g_nextskill);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CCMD(levelstart)
{
	if (netgame)
	{
		Printf("Use " TEXTCOLOR_BOLD "levelwarp" TEXTCOLOR_NORMAL " instead. " TEXTCOLOR_BOLD "levelstart"
			TEXTCOLOR_NORMAL " is for single-player only.\n");
		return;
	}
	auto map = levelwarp_common(argv, "start game", "start new game at");
	if (map)
	{
		DeferredStartGame(map, g_nextskill);
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
	if (!gi->CanSave())
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

	ChangeLevel(map, g_nextskill);
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
		map = SetupUserMap(mapfilename, g_gameType & GAMEFLAG_DUKE? "dethtoll.mid" : nullptr);
	}
	if (map)
	{
		if (fileSystem.FindFile(map->fileName) < 0)
		{
			Printf(PRINT_BOLD, "%s: map file not found\n", map->fileName.GetChars());
		}

		DeferredStartGame(map, g_nextskill);
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
	ChangeLevel(currentLevel, g_nextskill);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CUSTOM_CVAR(Float, i_timescale, 1.0f, CVAR_NOINITCALL)
{
	if (netgame)
	{
		Printf("Time scale cannot be changed in net games.\n");
		self = 1.0f;
	}
	else if (self >= 0.05f)
	{
		I_FreezeTime(true);
		TimeScale = self;
		I_FreezeTime(false);
	}
	else
	{
		Printf("Time scale must be at least 0.05!\n");
	}
}

CCMD(endofgame)
{
	STAT_Update(true);
	ChangeLevel(nullptr, g_nextskill);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CCMD(skill)
{
	if (gamestate == GS_LEVEL)
	{
		auto argsCount = argv.argc();

		if (argsCount < 2)
		{
			auto currentSkill = gi->GetCurrentSkill();
			if (currentSkill >= 0)
			{
				Printf("Current skill is %d (%s)\n", currentSkill, GStrings.localize(gSkillNames[currentSkill]));
			}
			else if (currentSkill == -1)
			{
				Printf("Current skill is not set (%d)\n");
			}
			else if (currentSkill == -2)
			{
				Printf("This game has no skill settings.\n");
			}
			else
			{
				Printf("Current skill is an unknown/unsupported value (%d)\n");
			}
		}
		else if (argsCount == 2)
		{
			auto newSkill = atoi(argv[1]);
			if (newSkill >= 0 and newSkill <  MAXSKILLS)
			{
				g_nextskill = newSkill;
				Printf("Skill will be changed for next game.\n");
			}
			else
			{
				Printf("Please specify a skill level between 0 and %d\n", MAXSKILLS - 1);
			}
		}
		else if (argsCount > 2)
		{
			Printf(PRINT_BOLD, "skill <newskill>: returns the current skill level, and optionally sets the skill level for the next game.\n");
		}
	}
	else
	{
		Printf("Currently not in a game.\n");
	}
}
