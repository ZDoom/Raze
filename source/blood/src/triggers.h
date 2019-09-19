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
#include "common.h"
#include "common_game.h"

#include "blood.h"
#include "db.h"
#include "eventq.h"
#include "dude.h"

void trTriggerSector(unsigned int nSector, XSECTOR *pXSector, int a3);
void trMessageSector(unsigned int nSector, EVENT a2);
void trTriggerWall(unsigned int nWall, XWALL *pXWall, int a3);
void trMessageWall(unsigned int nWall, EVENT a2);
void trTriggerSprite(unsigned int nSprite, XSPRITE *pXSprite, int a3);
void trMessageSprite(unsigned int nSprite, EVENT a2);
void trProcessBusy(void);
void trInit(void);
void trTextOver(int nId);

// By NoOne: functions required for new features
// -------------------------------------------------------
void pastePropertiesInObj(int type, int nDest, EVENT event);
void trDamageSprite(int type, int nDest, EVENT event);
spritetype* getTargetInRange(spritetype* pSprite, int minDist, int maxDist, short data, short teamMode);
bool isMateOf(XSPRITE* pXDude, XSPRITE* pXSprite);
spritetype* targetIsPlayer(XSPRITE* pXSprite);
bool isTargetAimsDude(XSPRITE* pXTarget, spritetype* pDude);
spritetype* getMateTargets(XSPRITE* pXSprite);
bool isMatesHaveSameTarget(XSPRITE* pXLeader, spritetype* pTarget, int allow);
bool isActive(int nSprite);
bool dudeCanSeeTarget(XSPRITE* pXDude, DUDEINFO* pDudeInfo, spritetype* pTarget);
void disturbDudesInSight(spritetype* pSprite, int max);
int getTargetDist(spritetype* pSprite, DUDEINFO* pDudeInfo, spritetype* pTarget);
int getFineTargetDist(spritetype* pSprite, spritetype* pTarget);
bool IsBurningDude(spritetype* pSprite);
bool IsKillableDude(spritetype* pSprite, bool locked);
bool isAnnoyingUnit(spritetype* pDude);
bool unitCanFly(spritetype* pDude);
bool isMeleeUnit(spritetype* pDude);
void activateDudes(int rx);
bool affectedByTargetChg(XSPRITE* pXDude);
int getDataFieldOfObject(int objType, int objIndex, int dataIndex);
bool setDataValueOfObject(int objType, int objIndex, int dataIndex, int value);
bool goalValueIsReached(XSPRITE* pXSprite);
bool getDudesForTargetChg(XSPRITE* pXSprite);
void stopWindOnSectors(XSPRITE* pXSource);
void useSectorWindGen(XSPRITE* pXSource, sectortype* pSector);
void useEffectGen(XSPRITE* pXSource, spritetype* pSprite);
void useSeqSpawnerGen(XSPRITE* pXSource, spritetype* pSprite);
// -------------------------------------------------------