//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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
#pragma once
#include "build.h"
#include "common_game.h"

#include "db.h"
#include "eventq.h"
#include "dude.h"
#include "player.h"

BEGIN_BLD_NS
void trTriggerSector(unsigned int nSector, XSECTOR *pXSector, int command);
void trMessageSector(unsigned int nSector, EVENT event);
void trTriggerWall(unsigned int nWall, XWALL *pXWall, int command);
void trMessageWall(unsigned int nWall, EVENT event);
void trTriggerSprite(unsigned int nSprite, XSPRITE *pXSprite, int command);
void trMessageSprite(unsigned int nSprite, EVENT event);
void trProcessBusy(void);
void trInit(void);
void trTextOver(int nId);
char SetSpriteState(int nSprite, XSPRITE* pXSprite, int nState);
char SetWallState(int nWall, XWALL* pXWall, int nState);
void TeleFrag(int nKiller, int nSector);

END_BLD_NS
