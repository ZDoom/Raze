//-----------------------------------------------------------------------------
//
// Copyright 1999-2016 Randy Heit
// Copyright 2002-2016 Christoph Oelckers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//		Generic Cheat code.
//
//-----------------------------------------------------------------------------

#include "c_cvars.h"
#include "c_dispatch.h"
#include "d_event.h"
#include "cheathandler.h"
#include "printf.h"
#include "gamestruct.h"
#include "utf8.h"

static cheatseq_t *cheatlist;
static int numcheats;

void SetCheats(cheatseq_t *cht, int count)
{
	cheatlist = cht;
	numcheats = count;
}

CVAR(Bool, nocheats, false, CVAR_ARCHIVE)

//--------------------------------------------------------------------------
//
// FUNC CheatAddkey
//
// Returns true if the added key completed the cheat, false otherwise.
//
//--------------------------------------------------------------------------

static bool CheatAddKey (cheatseq_t *cheat, uint8_t key, bool *eat)
{
	if (cheat->Pos == NULL)
	{
		cheat->Pos = cheat->Sequence;
		cheat->CurrentArg = 0;
	}
	if (*cheat->Pos == '#' && key >= '0' && key <= '9')
	{
		*eat = true;
		cheat->Args[cheat->CurrentArg++] = key;
		cheat->Pos++;
	}
	else if (upperforlower[key] == upperforlower[*cheat->Pos])
	{
		cheat->Pos++;
	}
	else
	{
		cheat->Pos = cheat->Sequence;
		cheat->CurrentArg = 0;
	}
	if (*cheat->Pos == 0)
	{
		cheat->Pos = cheat->Sequence;
		cheat->CurrentArg = 0;
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------
//
// Respond to keyboard input events, intercept cheats.
// [RH] Cheats eat the last keypress used to trigger them
//
//--------------------------------------------------------------------------

bool Cheat_Responder (event_t *ev)
{
	bool eat = false;

	if (nocheats)
	{
		return false;
	}
	if (ev->type == EV_KeyDown)
	{
		int i;

		auto cheats = cheatlist;
		for (i = 0; i < numcheats; i++, cheats++)
		{
			if (CheatAddKey (cheats, (uint8_t)ev->data2, &eat))
			{
				if (cheats->DontCheck || CheckCheatmode ())
				{
					eat |= cheats->Handler (cheats);
				}
			}
			else if (cheats->Pos - cheats->Sequence > 2)
			{ // If more than two characters into the sequence,
			  // eat the keypress.
				eat = true;
			}
		}
	}
	return eat;
}

void PlaybackCheat(const char *p)
{
	if (!gi->CheatAllowed(false))
	{
		event_t ev = { EV_KeyDown, 0, 0, -1 };
		Cheat_Responder(&ev);   // Reset the parser by passing a non-existent key.
		for (; *p; p++)
		{
			// just play the cheat command through the event parser
			ev.data2 = *p;
			Cheat_Responder(&ev);
		}
		ev.data2 = -1;
		Cheat_Responder(&ev);
	}
	else
		Printf("activatecheat: Cheats not allowed.\n");

}

CCMD(activatecheat)
{
	if (argv.argc() != 1)
		Printf("activatecheat <string>: activates a classic cheat code\n");
	else
		PlaybackCheat(argv[0]);
}
