/*
** cheathandler.cpp
**		Generic Cheat code.
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

#include "c_cvars.h"
#include "c_dispatch.h"
#include "d_event.h"
#include "cheathandler.h"
#include "printf.h"
#include "gamestruct.h"
#include "utf8.h"
#include "d_protocol.h"
#include "d_net.h"

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
	if (*cheat->Pos == '#' && ((key >= '0' && key <= '9') || key == ' '))
	{
		*eat = true;
		cheat->Args[cheat->CurrentArg++] = key == ' ' ? '0' : key;
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
				if (cheats->DontCheck || !CheckCheatmode ())
				{
					if (cheats->Handler)
						eat |= cheats->Handler (cheats);
					else if (cheats->ccmd)
					{
						eat = true;
						C_DoCommand(cheats->ccmd);
					}
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

bool SendGenericCheat(cheatseq_t* cheat)
{
	Net_WriteByte(DEM_GENERICCHEAT);
	Net_WriteByte(cheat->Param);
	return true;
}

void PlaybackCheat(const char *p)
{
	if (!CheckCheatmode(true))
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
}

CCMD(activatecheat)
{
	if (argv.argc() < 2)
		Printf("activatecheat <string>: activates a classic cheat code\n");
	else
		PlaybackCheat(argv[1]);
}
