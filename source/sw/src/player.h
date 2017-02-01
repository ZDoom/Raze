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

#ifndef PLAYER_H

#define PLAYER_H

#define PLAYER_HORIZ_MAX 299    // !JIM! was 199 and 5
#define PLAYER_HORIZ_MIN -99    // Had to make plax sky pan up/down like in Duke
// But this is MUCH better!

#define MIN_SWIM_DEPTH 15

// Player view height
#define PLAYER_HEIGHT Z(58)
#define PLAYER_CRAWL_HEIGHT Z(36)
#define PLAYER_SWIM_HEIGHT Z(26)
#define PLAYER_DIVE_HEIGHT Z(26)
#define PLAYER_DIE_DOWN_HEIGHT Z(4)
#define PLAYER_DIE_UP_HEIGHT Z(8)

// step heights - effects floor_dist's
#define PLAYER_STEP_HEIGHT Z(30)
//#define PLAYER_STEP_HEIGHT Z(34)
//#define PLAYER_STEP_HEIGHT Z(38)

#define PLAYER_CRAWL_STEP_HEIGHT Z(8)
#define PLAYER_SWIM_STEP_HEIGHT Z(8)
#define PLAYER_DIVE_STEP_HEIGHT Z(8)

//#define PLAYER_JUMP_STEP_HEIGHT Z(16)
//#define PLAYER_FALL_STEP_HEIGHT Z(16)

//#define PLAYER_JUMP_STEP_HEIGHT Z(34)
//#define PLAYER_FALL_STEP_HEIGHT Z(24)

#define PLAYER_JUMP_STEP_HEIGHT Z(48)
#define PLAYER_FALL_STEP_HEIGHT Z(24)

// FLOOR_DIST variables are the difference in the Players view and the sector floor.
// Must be at LEAST this distance or you cannot move onto sector.
#define PLAYER_RUN_FLOOR_DIST (PLAYER_HEIGHT - PLAYER_STEP_HEIGHT)
#define PLAYER_CRAWL_FLOOR_DIST (PLAYER_CRAWL_HEIGHT - PLAYER_CRAWL_STEP_HEIGHT)
#define PLAYER_WADE_FLOOR_DIST (PLAYER_HEIGHT - PLAYER_STEP_HEIGHT)
#define PLAYER_JUMP_FLOOR_DIST (PLAYER_HEIGHT - PLAYER_JUMP_STEP_HEIGHT)
#define PLAYER_FALL_FLOOR_DIST (PLAYER_HEIGHT - PLAYER_FALL_STEP_HEIGHT)
#define PLAYER_SWIM_FLOOR_DIST (PLAYER_SWIM_HEIGHT - PLAYER_SWIM_STEP_HEIGHT)
#define PLAYER_DIVE_FLOOR_DIST (PLAYER_DIVE_HEIGHT - PLAYER_DIVE_STEP_HEIGHT)


// FLOOR_DIST variables are the difference in the Players view and the sector floor.
// Must be at LEAST this distance or you cannot move onto sector.
#define PLAYER_RUN_CEILING_DIST Z(10)
#define PLAYER_SWIM_CEILING_DIST (Z(12))
#define PLAYER_DIVE_CEILING_DIST (Z(22))
#define PLAYER_CRAWL_CEILING_DIST (Z(12))
#define PLAYER_JUMP_CEILING_DIST Z(4)
#define PLAYER_FALL_CEILING_DIST Z(4)
#define PLAYER_WADE_CEILING_DIST Z(4)

//
// DIVE
//

#define PLAYER_DIVE_MAX_SPEED       (1700)
#define PLAYER_DIVE_INC             (600)
#define PLAYER_DIVE_BOB_AMT (Z(8))

#define PLAYER_DIVE_TIME (12*120)       // time before damage is taken
#define PLAYER_DIVE_DAMAGE_AMOUNT (-1)  // amount of damage accessed
#define PLAYER_DIVE_DAMAGE_TIME (50) // time between damage accessment

//
// FLY
//

#define PLAYER_FLY_MAX_SPEED       (2560)
#define PLAYER_FLY_INC             (1000)
#define PLAYER_FLY_BOB_AMT (Z(12))

// Height from which Player will actually call DoPlayerBeginFall()
//#define PLAYER_FALL_HEIGHT Z(16)
#define PLAYER_FALL_HEIGHT Z(28)
#define PLAYER_FALL_DAMAGE_AMOUNT (10)

//
// DEATH
//

// dead head height - used in DeathFall
#define PLAYER_DEATH_HEIGHT (Z(16))
#define PLAYER_DEAD_HEAD_FLOORZ_OFFSET (Z(7))

//#define PLAYER_NINJA_XREPEAT (56)
//#define PLAYER_NINJA_YREPEAT (56)
#define PLAYER_NINJA_XREPEAT (47)
#define PLAYER_NINJA_YREPEAT (33)

int SetVisHigh(void);
int SetVisNorm(void);
void DoWeapon(void);
void HeadBobStateControl(void);
int DoPickTarget(SPRITEp sp, uint32_t max_delta_ang, SWBOOL skip_targets);
void DoPlayer(void);
void domovethings(void);
void InitAllPlayers(void);
void InitMultiPlayerInfo(void);
void MoveScrollMode2D(PLAYERp pp);
void DoPlayerDivePalette(PLAYERp pp);
void DoPlayerNightVisionPalette(PLAYERp pp);
void DoPlayerStopDiveNoWarp(PLAYERp pp);
void DoPlayerResetMovement(PLAYERp pp);
void DoPlayerZrange(PLAYERp pp);
void DoPlayerSpriteThrow(PLAYERp pp);
int DoPlayerWadeSuperJump(PLAYERp pp);
void DoPlayerWarpTeleporter(PLAYERp pp);
void UpdatePlayerSprite(PLAYERp pp);
void PlaySOsound(short sectnum,short sound_num);
void DoSpawnTeleporterEffectPlace(SPRITEp sp);
void FindMainSector(SECTOR_OBJECTp sop);

#endif
