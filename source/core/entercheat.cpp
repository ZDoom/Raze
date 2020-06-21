//-----------------------------------------------------------------------------
//
// Copyright 1993-1996 id Software
// Copyright 1994-1996 Raven Software
// Copyright 1999-2016 Randy Heit
// Copyright 2002-2020 Christoph Oelckers
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
//		Cheat code. See *_sbar.cpp for status bars.
//
//-----------------------------------------------------------------------------

#include "gstrings.h"
#include "c_cvars.h"
#include "c_dispatch.h"
#include "d_event.h"
#include "baselayer.h"
#include "gamecontrol.h"

struct cheatseq_t
{
	const uint8_t *Sequence;
	const uint8_t *Pos;
	uint8_t CurrentArg;
	uint8_t Args[5];
};

static TArray<cheatseq_t> cheats;

static bool CheatCheckList (event_t *ev);
static bool CheatAddKey (cheatseq_t *cheat, uint8_t key, bool *eat);


CVAR(Bool, allcheats, false, CVAR_ARCHIVE)
CVAR(Bool, nocheats, false, CVAR_ARCHIVE)

// Respond to keyboard input events, intercept cheats.
// [RH] Cheats eat the last keypress used to trigger them
bool Cheat_Responder (event_t *ev)
{
	bool eat = false;
	
	if (cheats.Size() == 0)
	{
#if 0
		auto gcheats = gi->GetCheats();
		if (gcheats)
		{
			for (int i = 0; gcheats[i]; i++)
			{
				cheatseq_t cht = { (const uint8_t*)gcheats[i], nullptr };
				cheats.Push(cht);
			}
		}
#endif
	}

	if (nocheats)
	{
		return false;
	}
	else 
	{
		return CheatCheckList(ev);
	}
	return false;
}

static bool CheatCheckList (event_t *ev)
{
	bool eat = false;

	if (ev->type == EV_KeyDown)
	{
		for (auto &cht :cheats)
		{
			if (CheatAddKey (&cht, (uint8_t)ev->data2, &eat))
			{
				int processed = gi->CheckCheat((const char*)cht.Sequence, (const char*)cht.Args);
				if (processed = 1) cht.Pos = nullptr;
				eat |= processed != 0;
			}
			else if (cht.Pos - cht.Sequence > 2)
			{ // If more than two characters into the sequence,
			  // eat the keypress, to reduce interference with game controls.
				eat = true;
			}
		}
	}
	return eat;
}

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
		cheat->Args[0] = 0;
	}
	if (*cheat->Pos == '#' && key >= '0' && key <= '9')
	{
		*eat = true;
		cheat->Args[cheat->CurrentArg++] = key;
		cheat->Args[cheat->CurrentArg] = 0;
		cheat->Pos++;
		return true;
	}
	else if (key == *cheat->Pos)
	{
		cheat->Pos++;
	}
	else
	{
		cheat->Pos = cheat->Sequence;
	}
	if (*cheat->Pos == 0)
	{
		cheat->Pos = cheat->Sequence;
		return true;
	}
	return false;
}

