/*
** c_dispatch.cpp
** Functions for executing console commands and aliases
**
**---------------------------------------------------------------------------
** Copyright 1998-2016 Randy Heit
** Copyright 2003-2019 Christoph Oelckers
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
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/ 

#include "c_bind.h"
#include "d_event.h"
#include "c_console.h"
#include "d_gui.h"
#include "inputstate.h"
#include "razemenu.h"
#include "gamestate.h"
#include "gamecontrol.h"
#include "uiinput.h"
#include "automap.h"
#include "screenjob.h"

//==========================================================================
//
// G_Responder
// Process the event for the game
//
//==========================================================================

bool G_Responder (event_t *ev)
{
	if (gamestate == GS_INTRO || gamestate == GS_CUTSCENE)
	{
		return ScreenJobResponder(ev);
	}

	if (CT_Responder(ev))
		return true;					// chat ate the event
	if (Cheat_Responder(ev))
		return true;

	if (gamestate == GS_LEVEL && automapMode != am_off && AM_Responder(ev, false)) return true;

	switch (ev->type)
	{
	case EV_KeyDown:
		if (C_DoKey (ev, &Bindings, &DoubleBindings))
			return true;
		break;

	case EV_KeyUp:
		C_DoKey (ev, &Bindings, &DoubleBindings);
		break;

	}

	// This won't work as expected with Build's overlay automap.
#if 0
	// [RH] If the view is active, give the automap a chance at
	// the events *last* so that any bound keys get precedence.
	if (gamestate == GS_LEVEL && automapMode == am_overlay)
		return AM_Responder (ev, true);
#endif

	return (ev->type == EV_KeyDown ||
			ev->type == EV_Mouse);
}


