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

#include "compat.h"
#include "build.h"
#include "player.h"

BEGIN_BLD_NS

#define kMessageLogSize 32
#define kMaxMessageTextLength 81

enum MESSAGE_PRIORITY {
    MESSAGE_PRIORITY_PICKUP = -10,
    MESSAGE_PRIORITY_NORMAL = 0,
    MESSAGE_PRIORITY_SECRET = 10,
    MESSAGE_PRIORITY_INI = 20,
    MESSAGE_PRIORITY_SYSTEM = 100
};

enum CHEATCODE
 {
	 kCheatNone = 0,

	 kCheat1 = 1000, // refills ammo, no cheat code for it
	 kCheatGriswold,
	 kCheatSatchel,
	 kCheatEvaGalli,
	 kCheatMpkfa,
	 kCheatCapInMyAss,
	 kCheatNoCapInMyAss,
	 kCheatIdaho,
	 kCheatKevorkian,
	 kCheatMcGee,
	 kCheatEdmark,
	 kCheatKrueger,
	 kCheatSterno,
	 kCheat14, // quake effect, not used
	 kCheatSpork,
	 kCheatGoonies,
	 kCheatClarice,
	 kCheatFrankenstein,
	 kCheatCheeseHead,
	 kCheatTequila,
	 kCheatFunkyShoes,
	 kCheatKeyMaster,
	 kCheatOneRing,
	 kCheatVoorhees,
	 kCheatJoJo,
	 kCheatGateKeeper,
	 kCheatRate,
	 kCheatMario,
	 kCheatLaraCroft,
	 kCheatHongKong,
	 kCheatMontana,
	 kCheatBunz,
	 kCheatCousteau,
	 kCheatForkYou,
	 kCheatLieberMan,
	 kCheatSpielberg,
	 kCheatCalgon,
	 kCheatMax
 };

extern bool bPlayerCheated;
void cheatReset(void);

void SetAmmo(bool stat);
void SetWeapons(bool stat);
void SetToys(bool stat);
void SetArmor(bool stat);
void SetKeys(bool stat);

END_BLD_NS
