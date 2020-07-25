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

#include "ns.h"	// Must come before everything else!


#include "baselayer.h"
#include "common_game.h"
#include "build.h"

#include "baselayer.h"
#include "gamecontrol.h"
#include "blood.h"
#include "config.h"
#include "globals.h"
#include "screen.h"
#include "sound.h"
#include "view.h"

// we load this in to get default button and key assignments
// as well as setting up function mappings

BEGIN_BLD_NS


int32_t gTurnSpeed = 92;
int32_t gDetail = 4;
int32_t cl_weaponswitch;
int32_t gFollowMap = 1;
int32_t gOverlayMap = 0;
int32_t gRotateMap = 0;
int32_t gMessageCount = 4;
int32_t gMessageTime = 5;
int32_t gMessageFont = 0;
int32_t gMouseSensitivity;
bool gNoClip;
bool gInfiniteAmmo;
int32_t gDeliriumBlur = 1;

//////////
int gWeaponsV10x;
/////////


END_BLD_NS
