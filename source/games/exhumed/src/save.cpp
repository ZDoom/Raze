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
#include <stdio.h>
#include <stdarg.h>
#include "build.h"
#include "raze_music.h"
#include "engine.h"
#include "exhumed.h"
#include "savegamehelp.h"
#include "sound.h"
#include "mapinfo.h"

BEGIN_PS_NS

void SerializeState(FSerializer& arc);
void SerializeAnim(FSerializer& arc);
void SerializeBullet(FSerializer& arc);
void SerializeGun(FSerializer& arc);
void SerializeInit(FSerializer& arc);
void SerializeItems(FSerializer& arc);
void SerializeMove(FSerializer& arc);
void SerializeLighting(FSerializer& arc);
void SerializeObjects(FSerializer& arc);
void SerializePlayer(FSerializer& arc);
void SerializeRa(FSerializer& arc);
void SerializeRand(FSerializer& arc);
void SerializeRunList(FSerializer& arc);
void SerializeSequence(FSerializer& arc);
void SerializeSnake(FSerializer& arc);
void SerializeSwitch(FSerializer& arc);
void SerializeView(FSerializer& arc);

void SerializeQueen(FSerializer& arc);
void SerializeRat(FSerializer& arc);

void GameInterface::SerializeGameState(FSerializer& arc)
{
	if (arc.BeginObject("exhumed"))
	{
		SerializeState(arc);
		SerializeAnim(arc);
		SerializeBullet(arc);
		SerializeGun(arc);
		SerializeInit(arc);
		SerializeItems(arc);
		SerializeMove(arc);
		SerializeLighting(arc);
		SerializeObjects(arc);
		SerializePlayer(arc);
		SerializeRa(arc);
		SerializeRand(arc);
		SerializeRunList(arc);
		SerializeSequence(arc);
		SerializeSnake(arc);
		SerializeSwitch(arc);
		SerializeView(arc);

		SerializeQueen(arc);
		SerializeRat(arc);
	}
	if (arc.isReading())
	{

		// reset the sky in case it hasn't been done yet.
		psky_t* pSky = tileSetupSky(DEFAULTPSKY);
		pSky->tileofs[0] = 0;
		pSky->tileofs[1] = 0;
		pSky->tileofs[2] = 0;
		pSky->tileofs[3] = 0;
		pSky->yoffs = 256;
		pSky->yoffs2 = 256;
		pSky->lognumtiles = 2;
		pSky->horizfrac = 65536;
		pSky->yscale = 65536;
		parallaxtype = 2;
		g_visibility = 1024;

		if (currentLevel->gameflags & LEVEL_EX_ALTSOUND)
		{
			nSwitchSound = 35;
			nStoneSound = 23;
			nElevSound = 51;
			nStopSound = 35;
		}
		else
		{
			nSwitchSound = 33;
			nStoneSound = 23;
			nElevSound = 23;
			nStopSound = 66;
		}

		Mus_ResumeSaved();
	}
}

END_PS_NS
