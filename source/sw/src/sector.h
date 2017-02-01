//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#ifndef SECTOR_H

#define SECTOR_H


void SectorSetup(void);
DOOR_AUTO_CLOSEp SetDoorAutoClose(short SectorNum, short Type);
void DoDragging(void);
int MoveDoorVert(short door_sector, short dir, short door_speed);
int MoveDoorUp(short door_sector, short auto_close, short door_speed);
int MoveDoorDown(short door_sector, short dir, short door_speed);
int MoveDoorHoriz(short door_sector, short dir, short door_speed);
void DoDoorsClose(void);
short Switch(short SwitchSector);
void PlayerOperateEnv(PLAYERp pp);
int TeleportToSector(PLAYERp pp, int newsector);
int OperateSector(short sectnum,short player_is_operating);
int OperateSprite(short SpriteNum, short player_is_operating);
int OperateWall(short wallnum, short player_is_operating);
void OperateTripTrigger(PLAYERp pp);

enum SO_SCALE_TYPE
{
    SO_SCALE_NONE,
    SO_SCALE_HOLD,
    SO_SCALE_DEST,
    SO_SCALE_RANDOM,
    SO_SCALE_CYCLE,
    SO_SCALE_RANDOM_POINT
};

#define SCALE_POINT_SPEED (4 + RANDOM_RANGE(8))

typedef struct
{
    int dist;
    short sectnum, wallnum, spritenum;
} NEAR_TAG_INFO, *NEAR_TAG_INFOp;
extern short nti_cnt;

void DoSpawnSpotsForKill(short match);
void DoSpawnSpotsForDamage(short match);
void DoMatchEverything(PLAYERp pp, short match, short state);
SWBOOL ComboSwitchTest(short combo_type,short match);
void DoSoundSpotStopSound(short match);
void DoSector(void);
short AnimateSwitch(SPRITEp sp,short tgt_value);
void ShootableSwitch(short SpriteNum,short Weapon);
SWBOOL TestKillSectorObject(SECTOR_OBJECTp sop);
void WeaponExplodeSectorInRange(short weapon);

void initlava(void);
void movelava(char *dapic);

#endif
