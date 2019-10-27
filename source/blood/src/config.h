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

#ifndef config_public_h_
#define config_public_h_

#include "keyboard.h"
#include "control.h"
#include "_control.h"
#include "gamedefs.h"
#include "hash.h"
#include "gamecvars.h"
#include "gamecontrol.h"

BEGIN_BLD_NS

#define MAXPLAYERNAME 16

extern int32_t scripthandle;
extern int32_t setupread;
extern int32_t configversion;
extern int32_t CheckForUpdates;
extern int32_t LastUpdateCheck;
extern char szPlayerName[MAXPLAYERNAME];
extern int32_t gTurnSpeed;
extern int32_t gDetail;
extern int32_t gAutoRun;
extern int32_t gFollowMap;
extern int32_t gOverlayMap;
extern int32_t gRotateMap;
extern int32_t gMessageCount;
extern int32_t gMessageTime;
extern int32_t gMessageFont;
extern int32_t gbAdultContent;
extern char gzAdultPassword[9];
extern int32_t gMouseSensitivity;
extern bool gNoClip;
extern bool gInfiniteAmmo;
extern bool gFullMap;
extern int32_t gUpscaleFactor;
extern int32_t gDeliriumBlur;

///////
extern int gWeaponsV10x;
//////

int  CONFIG_ReadSetup(void);
void CONFIG_SetDefaults(void);

int32_t CONFIG_GetMapBestTime(char const *mapname, uint8_t const *mapmd4);
int     CONFIG_SetMapBestTime(uint8_t const *mapmd4, int32_t tm);

END_BLD_NS

#endif
