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

BEGIN_BLD_NS
void sleeveStopBouncing(DBloodActor* pSprite);

enum CALLBACK_ID {
	kCallbackNone = -1,
	kCallbackFXFlameLick = 0,
	kCallbackRemove = 1,
	kCallbackFXFlareBurst = 2,
	kCallbackFXFlareSpark = 3,
	kCallbackFXFlareSparkLite = 4,
	kCallbackFXZombieSpurt = 5,
	kCallbackFXBloodSpurt = 6,
	kCallbackFXArcSpark = 7,
	kCallbackFXDynPuff = 8,
	kCallbackRespawn = 9,
	kCallbackPlayerBubble = 10,
	kCallbackEnemeyBubble = 11,
	kCallbackCounterCheck = 12,
	kCallbackFinishHim = 13,
	kCallbackFXBloodBits = 14,
	kCallbackFXTeslaAlt = 15,
	kCallbackFXBouncingSleeve = 16,
	kCallbackReturnFlag = 17,
	kCallbackFXPodBloodSpray = 18,
	kCallbackFXPodBloodSplat = 19,
	kCallbackLeechStateTimer = 20,
	kCallbackDropVoodoo = 21, // unused
#ifdef NOONE_EXTENSIONS
	kCallbackMissileBurst = 22,
	kCallbackMissileSpriteBlock = 23,
	kCallbackGenDudeUpdate = 24,
	kCallbackCondition = 25,
#endif
	kCallbackMax,
};

extern void(*gCallback[kCallbackMax])(DBloodActor*, sectortype*);

END_BLD_NS
