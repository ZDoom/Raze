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

CVAR(Bool, sv_cheats, true, CVAR_ARCHIVE|CVAR_SERVERINFO)
CVAR(Bool, cl_blockcheats, false, 0)

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