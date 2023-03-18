#pragma once
//-------------------------------------------------------------------------
/*
Copyright (C) 2020 Christoph Oelckers & Mitchell Richters

This file is part of Raze.

Raze is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "ns.h"	// Must come before everything else!

#include "blood.h"
#include "gamestate.h"
#include "inputstate.h"
#include "gamestruct.h"
#include "razemenu.h"

BEGIN_BLD_NS

static InputPacket gInput;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::GetInput(const double scaleAdjust, InputPacket* packet)
{
	if (paused || M_Active())
	{
		gInput = {};
		return;
	}

	HIDInput hidInput;
	getHidInput(&hidInput);

	PLAYER* pPlayer = &gPlayer[myconnectindex];
	InputPacket input{};

	ApplyGlobalInput(&hidInput, &gInput);
	processMovement(&hidInput, &gInput, &input, scaleAdjust);

	// Perform unsynchronised angle/horizon if not dead.
	if (!SyncInput() && gamestate == GS_LEVEL)
	{
		pPlayer->Angles.CameraAngles.Yaw += DAngle::fromDeg(input.avel);
		pPlayer->Angles.CameraAngles.Pitch += DAngle::fromDeg(input.horz);
	}

	if (packet)
	{
		*packet = gInput;
		gInput = {};
	}
}

//---------------------------------------------------------------------------
//
// This is called from InputState::ClearAllInput and resets all static state being used here.
//
//---------------------------------------------------------------------------

void GameInterface::clearlocalinputstate()
{
	gInput = {};
}

END_BLD_NS
