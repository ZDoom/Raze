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
#include "build.h"
#include "common.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "common_game.h"
#include "break.h"
#include "quake.h"
#include "net.h"
#include "pal.h"
#include "vis.h"

#include "ai.h"
#include "weapon.h"
#include "anim.h"
#include "sector.h"
#include "sprite.h"
#include "actor.h"
#include "track.h"
#include "player.h"

//
// Damage Amounts defined in damage.h
//
#define DAMAGE_TABLE
DAMAGE_DATA DamageData[] =
{
#include "damage.h"
};
#undef  DAMAGE_TABLE

short ADJUST=120;
int ADJUSTV=1<<16;

FOOT_TYPE FootMode=WATER_FOOT;
SWBOOL left_foot = FALSE;
int FinishTimer = 0;

// This is how many bullet shells have been spawned since the beginning of the game.
int ShellCount = 0;

//short Zombies = 0;
short StarQueueHead=0;
short StarQueue[MAX_STAR_QUEUE];
short HoleQueueHead=0;
short HoleQueue[MAX_HOLE_QUEUE];
short WallBloodQueueHead=0;
short WallBloodQueue[MAX_WALLBLOOD_QUEUE];
short FloorBloodQueueHead=0;
short FloorBloodQueue[MAX_FLOORBLOOD_QUEUE];
short GenericQueueHead=0;
short GenericQueue[MAX_GENERIC_QUEUE];
short LoWangsQueueHead=0;
short LoWangsQueue[MAX_LOWANGS_QUEUE];
int SpawnBreakStaticFlames(short);

SWBOOL GlobalSkipZrange = FALSE;

int WeaponIsAmmo = BIT(WPN_STAR) | BIT(WPN_SWORD) | BIT(WPN_MINE) | BIT(WPN_FIST);

short target_ang;

ANIMATOR NullAnimator;
ANIMATOR DoStar;
ANIMATOR DoCrossBolt;
ANIMATOR DoSuicide, DoUziSmoke;
ANIMATOR DoShrapJumpFall;
ANIMATOR DoFastShrapJumpFall;

int SpawnSmokePuff(short SpriteNum);
SWBOOL WarpToUnderwater(short *sectnum, int *x, int *y, int *z);
SWBOOL WarpToSurface(short *sectnum, int *x, int *y, int *z);
short ElectroFindClosestEnemy(short SpriteNum);
int InitElectroJump(SPRITEp wp, SPRITEp sp);
SWBOOL TestDontStickSector(short hit_sect);
int SpawnShrapX(short SpriteNum);
SWBOOL WeaponMoveHit(short SpriteNum);
int HelpMissileLateral(int16_t Weapon, int dist);
void SpawnMidSplash(short SpriteNum);

int SopDamage(SECTOR_OBJECTp sop,short amt);
int SopCheckKill(SECTOR_OBJECTp sop);
int QueueStar(short SpriteNum);
int DoBlurExtend(int16_t Weapon,int16_t interval,int16_t blur_num);
int SpawnDemonFist(short Weapon);
int SpawnTankShellExp(int16_t Weapon);
int SpawnMicroExp(int16_t Weapon);
void SpawnExpZadjust(short Weapon, SPRITEp exp, int upper_zsize, int lower_zsize);
int BulletHitSprite(SPRITEp sp,short hit_sprite,short hit_sect,short hit_wall,int hit_x,int hit_y,int hit_z,short ID);
int SpawnSplashXY(int hit_x,int hit_y,int hit_z,short);
int SpawnBoatSparks(PLAYERp pp,short hit_sect,short hit_wall,int hit_x,int hit_y,int hit_z,short hit_ang);

short StatDamageList[STAT_DAMAGE_LIST_SIZE] =
{
    STAT_SO_SP_CHILD,
    STAT_ENEMY,
    STAT_ENEMY_SKIP4,
    STAT_PLAYER0,
    STAT_PLAYER1,
    STAT_PLAYER2,
    STAT_PLAYER3,
    STAT_PLAYER4,
    STAT_PLAYER5,
    STAT_PLAYER6,
    STAT_PLAYER7,
    STAT_PLAYER_UNDER0,
    STAT_PLAYER_UNDER1,
    STAT_PLAYER_UNDER2,
    STAT_PLAYER_UNDER3,
    STAT_PLAYER_UNDER4,
    STAT_PLAYER_UNDER5,
    STAT_PLAYER_UNDER6,
    STAT_PLAYER_UNDER7,
    // MINE MUST BE LAST
    STAT_MINE_STUCK
};

//////////////////////
//
// SPECIAL STATES
//
//////////////////////

// state for sprites that are not restored
STATE s_NotRestored[] =
{
    {2323, 100, NullAnimator, &s_NotRestored[0]}
};

STATE s_Suicide[] =
{
    {1, 100, DoSuicide, &s_Suicide[0]}
};

STATE s_DeadLoWang[] =
{
    {1160, 100, NullAnimator, &s_DeadLoWang[0]},
};

//////////////////////
//
// BREAKABLE STATES
//
//////////////////////

ANIMATOR DoDefaultStat;

#define BREAK_LIGHT_RATE 18
STATE s_BreakLight[] =
{
    {BREAK_LIGHT_ANIM + 0, BREAK_LIGHT_RATE, NullAnimator, &s_BreakLight[1]},
    {BREAK_LIGHT_ANIM + 1, BREAK_LIGHT_RATE, NullAnimator, &s_BreakLight[2]},
    {BREAK_LIGHT_ANIM + 2, BREAK_LIGHT_RATE, NullAnimator, &s_BreakLight[3]},
    {BREAK_LIGHT_ANIM + 3, BREAK_LIGHT_RATE, NullAnimator, &s_BreakLight[4]},
    {BREAK_LIGHT_ANIM + 4, BREAK_LIGHT_RATE, NullAnimator, &s_BreakLight[5]},
    {BREAK_LIGHT_ANIM + 5, BREAK_LIGHT_RATE, NullAnimator, &s_BreakLight[5]}
};

#define BREAK_BARREL_RATE 18
STATE s_BreakBarrel[] =
{
    {BREAK_BARREL + 4, BREAK_BARREL_RATE, NullAnimator, &s_BreakBarrel[1]},
    {BREAK_BARREL + 5, BREAK_BARREL_RATE, NullAnimator, &s_BreakBarrel[2]},
    {BREAK_BARREL + 6, BREAK_BARREL_RATE, NullAnimator, &s_BreakBarrel[3]},
    {BREAK_BARREL + 7, BREAK_BARREL_RATE, NullAnimator, &s_BreakBarrel[4]},
    {BREAK_BARREL + 8, BREAK_BARREL_RATE, NullAnimator, &s_BreakBarrel[5]},
    {BREAK_BARREL + 9, BREAK_BARREL_RATE, DoDefaultStat, &s_BreakBarrel[5]},
};

#define BREAK_PEDISTAL_RATE 28
STATE s_BreakPedistal[] =
{
    {BREAK_PEDISTAL + 1, BREAK_PEDISTAL_RATE, NullAnimator, &s_BreakPedistal[1]},
    {BREAK_PEDISTAL + 2, BREAK_PEDISTAL_RATE, NullAnimator, &s_BreakPedistal[2]},
    {BREAK_PEDISTAL + 3, BREAK_PEDISTAL_RATE, NullAnimator, &s_BreakPedistal[3]},
    {BREAK_PEDISTAL + 4, BREAK_PEDISTAL_RATE, NullAnimator, &s_BreakPedistal[3]},
};

#define BREAK_BOTTLE1_RATE 18
STATE s_BreakBottle1[] =
{
    {BREAK_BOTTLE1 + 1, BREAK_BOTTLE1_RATE, NullAnimator, &s_BreakBottle1[1]},
    {BREAK_BOTTLE1 + 2, BREAK_BOTTLE1_RATE, NullAnimator, &s_BreakBottle1[2]},
    {BREAK_BOTTLE1 + 3, BREAK_BOTTLE1_RATE, NullAnimator, &s_BreakBottle1[3]},
    {BREAK_BOTTLE1 + 4, BREAK_BOTTLE1_RATE, NullAnimator, &s_BreakBottle1[4]},
    {BREAK_BOTTLE1 + 5, BREAK_BOTTLE1_RATE, NullAnimator, &s_BreakBottle1[5]},
    {BREAK_BOTTLE1 + 6, BREAK_BOTTLE1_RATE, NullAnimator, &s_BreakBottle1[5]},
};

#define BREAK_BOTTLE2_RATE 18
STATE s_BreakBottle2[] =
{
    {BREAK_BOTTLE2 + 1, BREAK_BOTTLE2_RATE, NullAnimator, &s_BreakBottle2[1]},
    {BREAK_BOTTLE2 + 2, BREAK_BOTTLE2_RATE, NullAnimator, &s_BreakBottle2[2]},
    {BREAK_BOTTLE2 + 3, BREAK_BOTTLE2_RATE, NullAnimator, &s_BreakBottle2[3]},
    {BREAK_BOTTLE2 + 4, BREAK_BOTTLE2_RATE, NullAnimator, &s_BreakBottle2[4]},
    {BREAK_BOTTLE2 + 5, BREAK_BOTTLE2_RATE, NullAnimator, &s_BreakBottle2[5]},
    {BREAK_BOTTLE2 + 6, BREAK_BOTTLE2_RATE, NullAnimator, &s_BreakBottle2[5]},
};

#define PUFF_RATE 8
ANIMATOR DoPuff;
STATE s_Puff[] =
{
    {PUFF + 0, PUFF_RATE, DoPuff, &s_Puff[1]},
    {PUFF + 1, PUFF_RATE, DoPuff, &s_Puff[2]},
    {PUFF + 2, PUFF_RATE, DoPuff, &s_Puff[3]},
    {PUFF + 3, PUFF_RATE, DoPuff, &s_Puff[4]},
    {PUFF + 4, PUFF_RATE, DoPuff, &s_Puff[5]},
    {PUFF + 5, PUFF_RATE, DoPuff, &s_Puff[6]},
    {PUFF + 5, 100, DoSuicide, &s_Puff[0]}
};

#define RAIL_PUFF_R0 3969
#define RAIL_PUFF_R1 3985
#define RAIL_PUFF_R2 4001
#define RAIL_PUFF_RATE 6

ANIMATOR DoRailPuff;

STATE s_RailPuff[3][17] =
{
    {
        {RAIL_PUFF_R0 + 0, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][1]},
        {RAIL_PUFF_R0 + 1, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][2]},
        {RAIL_PUFF_R0 + 2, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][3]},
        {RAIL_PUFF_R0 + 3, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][4]},
        {RAIL_PUFF_R0 + 4, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][5]},
        {RAIL_PUFF_R0 + 5, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][6]},
        {RAIL_PUFF_R0 + 6, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][7]},
        {RAIL_PUFF_R0 + 7, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][8]},
        {RAIL_PUFF_R0 + 8, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][9]},
        {RAIL_PUFF_R0 + 9, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][10]},
        {RAIL_PUFF_R0 + 10, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][11]},
        {RAIL_PUFF_R0 + 11, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][12]},
        {RAIL_PUFF_R0 + 12, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][13]},
        {RAIL_PUFF_R0 + 13, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][14]},
        {RAIL_PUFF_R0 + 14, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][15]},
        {RAIL_PUFF_R0 + 15, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][16]},
        {RAIL_PUFF_R0 + 15, 100, DoSuicide, &s_RailPuff[0][0]},
    },
    {
        {RAIL_PUFF_R1 + 0, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][1]},
        {RAIL_PUFF_R1 + 1, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][2]},
        {RAIL_PUFF_R1 + 2, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][3]},
        {RAIL_PUFF_R1 + 3, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][4]},
        {RAIL_PUFF_R1 + 4, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][5]},
        {RAIL_PUFF_R1 + 5, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][6]},
        {RAIL_PUFF_R1 + 6, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][7]},
        {RAIL_PUFF_R1 + 7, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][8]},
        {RAIL_PUFF_R1 + 8, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][9]},
        {RAIL_PUFF_R1 + 9, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][10]},
        {RAIL_PUFF_R1 + 10, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][11]},
        {RAIL_PUFF_R1 + 11, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][12]},
        {RAIL_PUFF_R1 + 12, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][13]},
        {RAIL_PUFF_R1 + 13, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][14]},
        {RAIL_PUFF_R1 + 14, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][15]},
        {RAIL_PUFF_R1 + 15, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][16]},
        {RAIL_PUFF_R1 + 15, 100, DoSuicide, &s_RailPuff[1][0]},
    },
    {
        {RAIL_PUFF_R2 + 0, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][1]},
        {RAIL_PUFF_R2 + 1, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][2]},
        {RAIL_PUFF_R2 + 2, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][3]},
        {RAIL_PUFF_R2 + 3, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][4]},
        {RAIL_PUFF_R2 + 4, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][5]},
        {RAIL_PUFF_R2 + 5, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][6]},
        {RAIL_PUFF_R2 + 6, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][7]},
        {RAIL_PUFF_R2 + 7, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][8]},
        {RAIL_PUFF_R2 + 8, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][9]},
        {RAIL_PUFF_R2 + 9, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][10]},
        {RAIL_PUFF_R2 + 10, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][11]},
        {RAIL_PUFF_R2 + 11, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][12]},
        {RAIL_PUFF_R2 + 12, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][13]},
        {RAIL_PUFF_R2 + 13, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][14]},
        {RAIL_PUFF_R2 + 14, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][15]},
        {RAIL_PUFF_R2 + 15, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][16]},
        {RAIL_PUFF_R2 + 15, 100, DoSuicide, &s_RailPuff[2][0]},
    }
};


STATEp sg_RailPuff[] =
{
    &s_RailPuff[0][0],
    &s_RailPuff[1][0],
    &s_RailPuff[2][0],
    &s_RailPuff[1][0],
    &s_RailPuff[0][0]
};

#define LASER_PUFF 3201
#define LASER_PUFF_RATE 8
STATE s_LaserPuff[] =
{
    {LASER_PUFF + 0, LASER_PUFF_RATE, NULL, &s_LaserPuff[1]},
    //{LASER_PUFF + 1, LASER_PUFF_RATE, NULL, &s_LaserPuff[2]},
    //{LASER_PUFF + 2, LASER_PUFF_RATE, NULL, &s_LaserPuff[3]},
    {LASER_PUFF + 0, 100, DoSuicide, &s_LaserPuff[0]}
};

#define TRACER 3201
#define TRACER_RATE 6
ANIMATOR DoTracer;
STATE s_Tracer[] =
{
    {TRACER + 0, TRACER_RATE, DoTracer, &s_Tracer[1]},
    {TRACER + 1, TRACER_RATE, DoTracer, &s_Tracer[2]},
    {TRACER + 2, TRACER_RATE, DoTracer, &s_Tracer[3]},
    {TRACER + 3, TRACER_RATE, DoTracer, &s_Tracer[4]},
    {TRACER + 4, TRACER_RATE, DoTracer, &s_Tracer[5]},
    {TRACER + 5, TRACER_RATE, DoTracer, &s_Tracer[0]}
};

#define EMP 2058
#define EMP_RATE 6
ANIMATOR DoEMP;
STATE s_EMP[] =
{
    {EMP + 0, EMP_RATE, DoEMP, &s_EMP[1]},
    {EMP + 1, EMP_RATE, DoEMP, &s_EMP[2]},
    {EMP + 2, EMP_RATE, DoEMP, &s_EMP[0]}
};

ANIMATOR DoEMPBurst;
STATE s_EMPBurst[] =
{
    {EMP + 0, EMP_RATE, DoEMPBurst, &s_EMPBurst[1]},
    {EMP + 1, EMP_RATE, DoEMPBurst, &s_EMPBurst[2]},
    {EMP + 2, EMP_RATE, DoEMPBurst, &s_EMPBurst[0]}
};

STATE s_EMPShrap[] =
{
    {EMP + 0, EMP_RATE, DoFastShrapJumpFall, &s_EMPShrap[1]},
    {EMP + 1, EMP_RATE, DoFastShrapJumpFall, &s_EMPShrap[2]},
    {EMP + 2, EMP_RATE, DoFastShrapJumpFall, &s_EMPShrap[0]},
};


#define TANK_SHELL 3201
#define TANK_SHELL_RATE 6
ANIMATOR DoTankShell;
STATE s_TankShell[] =
{
    {TRACER + 0, 200, DoTankShell, &s_TankShell[0]}
};

ANIMATOR DoVehicleSmoke, SpawnVehicleSmoke;
#define VEHICLE_SMOKE_RATE 18
STATE s_VehicleSmoke[] =
{
    {PUFF + 0, VEHICLE_SMOKE_RATE, DoVehicleSmoke, &s_VehicleSmoke[1]},
    {PUFF + 1, VEHICLE_SMOKE_RATE, DoVehicleSmoke, &s_VehicleSmoke[2]},
    {PUFF + 2, VEHICLE_SMOKE_RATE, DoVehicleSmoke, &s_VehicleSmoke[3]},
    {PUFF + 3, VEHICLE_SMOKE_RATE, DoVehicleSmoke, &s_VehicleSmoke[4]},
    {PUFF + 4, VEHICLE_SMOKE_RATE, DoVehicleSmoke, &s_VehicleSmoke[5]},
    {PUFF + 5, VEHICLE_SMOKE_RATE, DoVehicleSmoke, &s_VehicleSmoke[6]},
    {PUFF + 5, 100, DoSuicide, &s_VehicleSmoke[6]}
};

ANIMATOR DoWaterSmoke, SpawnWaterSmoke;
#define WATER_SMOKE_RATE 18
STATE s_WaterSmoke[] =
{
    {PUFF + 0, WATER_SMOKE_RATE, DoWaterSmoke, &s_WaterSmoke[1]},
    {PUFF + 1, WATER_SMOKE_RATE, DoWaterSmoke, &s_WaterSmoke[2]},
    {PUFF + 2, WATER_SMOKE_RATE, DoWaterSmoke, &s_WaterSmoke[3]},
    {PUFF + 3, WATER_SMOKE_RATE, DoWaterSmoke, &s_WaterSmoke[4]},
    {PUFF + 4, WATER_SMOKE_RATE, DoWaterSmoke, &s_WaterSmoke[5]},
    {PUFF + 5, WATER_SMOKE_RATE, DoWaterSmoke, &s_WaterSmoke[6]},
    {PUFF + 5, 100, DoSuicide, &s_WaterSmoke[6]}
};

#define UZI_SPARK_REPEAT 24
#define UZI_SMOKE_REPEAT 24 // Was 32
#define UZI_SMOKE_RATE 16 // Was 9
STATE s_UziSmoke[] =
{
    {UZI_SMOKE + 0, UZI_SMOKE_RATE, DoUziSmoke, &s_UziSmoke[1]},
    {UZI_SMOKE + 1, UZI_SMOKE_RATE, DoUziSmoke, &s_UziSmoke[2]},
    {UZI_SMOKE + 2, UZI_SMOKE_RATE, DoUziSmoke, &s_UziSmoke[3]},
    {UZI_SMOKE + 3, UZI_SMOKE_RATE, DoUziSmoke, &s_UziSmoke[4]},
    {UZI_SMOKE + 3, 100, DoSuicide, &s_UziSmoke[0]},
};

#define SHOTGUN_SMOKE_RATE 16
#define SHOTGUN_SMOKE_REPEAT 18 // Was 32
#define SHOTGUN_SMOKE UZI_SMOKE+1
ANIMATOR DoShotgunSmoke;
STATE s_ShotgunSmoke[] =
{
    {UZI_SMOKE + 0, SHOTGUN_SMOKE_RATE, DoShotgunSmoke, &s_ShotgunSmoke[1]},
    {UZI_SMOKE + 1, SHOTGUN_SMOKE_RATE, DoShotgunSmoke, &s_ShotgunSmoke[2]},
    {UZI_SMOKE + 2, SHOTGUN_SMOKE_RATE, DoShotgunSmoke, &s_ShotgunSmoke[3]},
    {UZI_SMOKE + 3, SHOTGUN_SMOKE_RATE, DoShotgunSmoke, &s_ShotgunSmoke[4]},
    {UZI_SMOKE + 3, 100, DoSuicide, &s_ShotgunSmoke[0]},
};

#define UZI_BULLET_RATE 100
#define UZI_BULLET 717 // actually a bubble
ANIMATOR DoUziBullet;

STATE s_UziBullet[] =
{
    {UZI_BULLET + 0, UZI_BULLET_RATE, DoUziBullet, &s_UziBullet[0]},
};

#define UZI_SPARK_RATE 8

STATE s_UziSpark[] =
{
    {UZI_SPARK + 0, UZI_SPARK_RATE, NullAnimator, &s_UziSpark[1]},
    {UZI_SPARK + 1, UZI_SPARK_RATE, NullAnimator, &s_UziSpark[2]},
    {UZI_SPARK + 2, UZI_SPARK_RATE, NullAnimator, &s_UziSpark[3]},
    {UZI_SPARK + 3, UZI_SPARK_RATE, NullAnimator, &s_UziSpark[4]},
    {UZI_SPARK + 4, UZI_SPARK_RATE, NullAnimator, &s_UziSpark[5]},
    {UZI_SPARK + 4, 100, DoSuicide, &s_UziSpark[0]},
};

STATE s_UziPowerSpark[] =
{
    {UZI_SPARK + 0, UZI_SPARK_RATE, DoUziSmoke, &s_UziSpark[1]},
    {UZI_SPARK + 1, UZI_SPARK_RATE, DoUziSmoke, &s_UziSpark[2]},
    {UZI_SPARK + 2, UZI_SPARK_RATE, DoUziSmoke, &s_UziSpark[3]},
    {UZI_SPARK + 3, UZI_SPARK_RATE, DoUziSmoke, &s_UziSpark[4]},
    {UZI_SPARK + 4, UZI_SPARK_RATE, DoUziSmoke, &s_UziSpark[5]},
    {UZI_SPARK + 4, 100, DoSuicide, &s_UziSpark[0]},
};

#define BUBBLE 716
#define BUBBLE_RATE 100
ANIMATOR DoBubble;

STATE s_Bubble[] =
{
    {BUBBLE + 0, BUBBLE_RATE, DoBubble, &s_Bubble[0]}
};


//#define SPLASH 2190
#define SPLASH 772
#define SPLASH_RATE 10

STATE s_Splash[] =
{
    {SPLASH + 0, SPLASH_RATE, NullAnimator, &s_Splash[1]},
    {SPLASH + 1, SPLASH_RATE, NullAnimator, &s_Splash[2]},
    {SPLASH + 2, SPLASH_RATE, NullAnimator, &s_Splash[3]},
    {SPLASH + 3, SPLASH_RATE, NullAnimator, &s_Splash[4]},
    {SPLASH + 4, SPLASH_RATE, NullAnimator, &s_Splash[5]},
    {SPLASH + 4, 100, DoSuicide, &s_Splash[0]}
};

#define CROSSBOLT 2230
#define CROSSBOLT_RATE 100
STATE s_CrossBolt[5][1] =
{
    {
        {CROSSBOLT + 0, CROSSBOLT_RATE, DoCrossBolt, &s_CrossBolt[0][0]},
    },
    {
        {CROSSBOLT + 2, CROSSBOLT_RATE, DoCrossBolt, &s_CrossBolt[1][0]},
    },
    {
        {CROSSBOLT + 3, CROSSBOLT_RATE, DoCrossBolt, &s_CrossBolt[2][0]},
    },
    {
        {CROSSBOLT + 4, CROSSBOLT_RATE, DoCrossBolt, &s_CrossBolt[3][0]},
    },
    {
        {CROSSBOLT + 1, CROSSBOLT_RATE, DoCrossBolt, &s_CrossBolt[4][0]},
    }
};

STATEp sg_CrossBolt[] =
{
    &s_CrossBolt[0][0],
    &s_CrossBolt[1][0],
    &s_CrossBolt[2][0],
    &s_CrossBolt[3][0],
    &s_CrossBolt[4][0]
};

#undef STAR
#define STAR 2102
#define STAR_RATE 6
STATE s_Star[] =
{
    {STAR + 0, STAR_RATE, DoStar, &s_Star[1]},
    {STAR + 1, STAR_RATE, DoStar, &s_Star[2]},
    {STAR + 2, STAR_RATE, DoStar, &s_Star[3]},
    {STAR + 3, STAR_RATE, DoStar, &s_Star[0]}
};

STATE s_StarStuck[] =
{
    {STAR + 0, STAR_RATE, NullAnimator, &s_StarStuck[0]},
};

#define STAR_DOWN 2066
STATE s_StarDown[] =
{
    {STAR_DOWN + 0, STAR_RATE, DoStar, &s_StarDown[1]},
    {STAR_DOWN + 1, STAR_RATE, DoStar, &s_StarDown[2]},
    {STAR_DOWN + 2, STAR_RATE, DoStar, &s_StarDown[3]},
    {STAR_DOWN + 3, STAR_RATE, DoStar, &s_StarDown[0]}
};

STATE s_StarDownStuck[] =
{
    {STAR + 0, STAR_RATE, NullAnimator, &s_StarDownStuck[0]},
};

//////////////////////
//
// LAVA BOSS
//
//////////////////////

#define LAVA_BOULDER_RATE 6
ANIMATOR DoLavaBoulder,DoShrapDamage,DoVulcanBoulder;

STATE s_LavaBoulder[] =
{
    {LAVA_BOULDER + 1, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[1]},
    {LAVA_BOULDER + 2, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[2]},
    {LAVA_BOULDER + 3, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[3]},
    {LAVA_BOULDER + 4, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[4]},
    {LAVA_BOULDER + 5, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[5]},
    {LAVA_BOULDER + 6, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[6]},
    {LAVA_BOULDER + 7, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[7]},
    {LAVA_BOULDER + 8, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[0]},
};

#define LAVA_SHARD (LAVA_BOULDER+1)

STATE s_LavaShard[] =
{
    {LAVA_BOULDER + 1, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[1]},
    {LAVA_BOULDER + 2, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[2]},
    {LAVA_BOULDER + 3, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[3]},
    {LAVA_BOULDER + 4, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[4]},
    {LAVA_BOULDER + 5, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[5]},
    {LAVA_BOULDER + 6, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[6]},
    {LAVA_BOULDER + 7, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[7]},
    {LAVA_BOULDER + 8, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[0]},
};

STATE s_VulcanBoulder[] =
{
    {LAVA_BOULDER + 1, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[1]},
    {LAVA_BOULDER + 2, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[2]},
    {LAVA_BOULDER + 3, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[3]},
    {LAVA_BOULDER + 4, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[4]},
    {LAVA_BOULDER + 5, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[5]},
    {LAVA_BOULDER + 6, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[6]},
    {LAVA_BOULDER + 7, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[7]},
    {LAVA_BOULDER + 8, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[0]},
};

//////////////////////
//
// GRENADE
//
//////////////////////

#if 0
ANIMATOR DoGrenade;
#undef GRENADE
#define GRENADE 5000
#define GRENADE_RATE 8

STATE s_Grenade[] =
{
    {FIREBALL + 0, GRENADE_RATE, DoGrenade, &s_Grenade[1]},
    {FIREBALL + 1, GRENADE_RATE, DoGrenade, &s_Grenade[2]},
    {FIREBALL + 2, GRENADE_RATE, DoGrenade, &s_Grenade[3]},
    {FIREBALL + 3, GRENADE_RATE, DoGrenade, &s_Grenade[0]}
};
#else
#define GRENADE_FRAMES 1
#define GRENADE_R0 2110
#define GRENADE_R1 GRENADE_R0 + (GRENADE_FRAMES * 1)
#define GRENADE_R2 GRENADE_R0 + (GRENADE_FRAMES * 2)
#define GRENADE_R3 GRENADE_R0 + (GRENADE_FRAMES * 3)
#define GRENADE_R4 GRENADE_R0 + (GRENADE_FRAMES * 4)

#undef GRENADE
#define GRENADE GRENADE_R0
#define GRENADE_RATE 8
ANIMATOR DoGrenade;

STATE s_Grenade[5][1] =
{
    {
        {GRENADE_R0 + 0, GRENADE_RATE, DoGrenade, &s_Grenade[0][0]},
    },
    {
        {GRENADE_R1 + 0, GRENADE_RATE, DoGrenade, &s_Grenade[1][0]},
    },
    {
        {GRENADE_R2 + 0, GRENADE_RATE, DoGrenade, &s_Grenade[2][0]},
    },
    {
        {GRENADE_R3 + 0, GRENADE_RATE, DoGrenade, &s_Grenade[3][0]},
    },
    {
        {GRENADE_R4 + 0, GRENADE_RATE, DoGrenade, &s_Grenade[4][0]},
    }
};


STATEp sg_Grenade[] =
{
    &s_Grenade[0][0],
    &s_Grenade[1][0],
    &s_Grenade[2][0],
    &s_Grenade[3][0],
    &s_Grenade[4][0]
};
#endif

//////////////////////
//
// MINE
//
//////////////////////

ANIMATOR DoMine,DoMineStuck;
#undef MINE
#define MINE 2223
#define MINE_SHRAP 5011
#define MINE_RATE 16

STATE s_MineStuck[] =
{
    {MINE + 0, MINE_RATE, DoMineStuck, &s_MineStuck[0]},
};

STATE s_Mine[] =
{
    {MINE + 0, MINE_RATE, DoMine, &s_Mine[1]},
    {MINE + 1, MINE_RATE, DoMine, &s_Mine[0]},
};

ANIMATOR DoMineSpark;
STATE s_MineSpark[] =
{
    {UZI_SPARK + 0, UZI_SPARK_RATE, DoMineSpark, &s_MineSpark[1]},
    {UZI_SPARK + 1, UZI_SPARK_RATE, DoMineSpark, &s_MineSpark[2]},
    {UZI_SPARK + 2, UZI_SPARK_RATE, DoMineSpark, &s_MineSpark[3]},
    {UZI_SPARK + 3, UZI_SPARK_RATE, DoMineSpark, &s_MineSpark[4]},
    {UZI_SPARK + 4, UZI_SPARK_RATE, DoMineSpark, &s_MineSpark[5]},
    {UZI_SPARK + 4, 100, DoSuicide, &s_MineSpark[0]},
};

//////////////////////
//
// METEOR
//
//////////////////////

#define METEOR_R0 2098
#define METEOR_R1 2090
#define METEOR_R2 2094
#define METEOR_R3 2090
#define METEOR_R4 2098


#define METEOR STAR
#define METEOR_RATE 8
ANIMATOR DoMeteor;

STATE s_Meteor[5][4] =
{
    {
        {METEOR_R0 + 0, METEOR_RATE, DoMeteor, &s_Meteor[0][1]},
        {METEOR_R0 + 1, METEOR_RATE, DoMeteor, &s_Meteor[0][2]},
        {METEOR_R0 + 2, METEOR_RATE, DoMeteor, &s_Meteor[0][3]},
        {METEOR_R0 + 3, METEOR_RATE, DoMeteor, &s_Meteor[0][0]},
    },
    {
        {METEOR_R1 + 0, METEOR_RATE, DoMeteor, &s_Meteor[1][1]},
        {METEOR_R1 + 1, METEOR_RATE, DoMeteor, &s_Meteor[1][2]},
        {METEOR_R1 + 2, METEOR_RATE, DoMeteor, &s_Meteor[1][3]},
        {METEOR_R1 + 3, METEOR_RATE, DoMeteor, &s_Meteor[1][0]},
    },
    {
        {METEOR_R2 + 0, METEOR_RATE, DoMeteor, &s_Meteor[2][1]},
        {METEOR_R2 + 1, METEOR_RATE, DoMeteor, &s_Meteor[2][2]},
        {METEOR_R2 + 2, METEOR_RATE, DoMeteor, &s_Meteor[2][3]},
        {METEOR_R2 + 3, METEOR_RATE, DoMeteor, &s_Meteor[2][0]},
    },
    {
        {METEOR_R3 + 0, METEOR_RATE, DoMeteor, &s_Meteor[3][1]},
        {METEOR_R3 + 1, METEOR_RATE, DoMeteor, &s_Meteor[3][2]},
        {METEOR_R3 + 2, METEOR_RATE, DoMeteor, &s_Meteor[3][3]},
        {METEOR_R3 + 3, METEOR_RATE, DoMeteor, &s_Meteor[3][0]},
    },
    {
        {METEOR_R4 + 0, METEOR_RATE, DoMeteor, &s_Meteor[4][1]},
        {METEOR_R4 + 1, METEOR_RATE, DoMeteor, &s_Meteor[4][2]},
        {METEOR_R4 + 2, METEOR_RATE, DoMeteor, &s_Meteor[4][3]},
        {METEOR_R4 + 3, METEOR_RATE, DoMeteor, &s_Meteor[4][0]},
    }
};


STATEp sg_Meteor[] =
{
    &s_Meteor[0][0],
    &s_Meteor[1][0],
    &s_Meteor[2][0],
    &s_Meteor[3][0],
    &s_Meteor[4][0]
};

#define METEOR_EXP 2115
#define METEOR_EXP_RATE 7

STATE s_MeteorExp[] =
{
    {METEOR_EXP + 0, METEOR_EXP_RATE, NullAnimator, &s_MeteorExp[1]},
    {METEOR_EXP + 1, METEOR_EXP_RATE, NullAnimator, &s_MeteorExp[2]},
    {METEOR_EXP + 2, METEOR_EXP_RATE, NullAnimator, &s_MeteorExp[3]},
    {METEOR_EXP + 3, METEOR_EXP_RATE, NullAnimator, &s_MeteorExp[4]},
    {METEOR_EXP + 4, METEOR_EXP_RATE, NullAnimator, &s_MeteorExp[5]},
    {METEOR_EXP + 5, METEOR_EXP_RATE, NullAnimator, &s_MeteorExp[6]},
    {METEOR_EXP + 5, METEOR_EXP_RATE, DoSuicide, &s_MeteorExp[6]}
};

#define MIRV_METEOR METEOR_R0
ANIMATOR DoMirvMissile;
STATE s_MirvMeteor[5][4] =
{
    {
        {METEOR_R0 + 0, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[0][1]},
        {METEOR_R0 + 1, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[0][2]},
        {METEOR_R0 + 2, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[0][3]},
        {METEOR_R0 + 3, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[0][0]},
    },
    {
        {METEOR_R1 + 0, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[1][1]},
        {METEOR_R1 + 1, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[1][2]},
        {METEOR_R1 + 2, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[1][3]},
        {METEOR_R1 + 3, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[1][0]},
    },
    {
        {METEOR_R2 + 0, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[2][1]},
        {METEOR_R2 + 1, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[2][2]},
        {METEOR_R2 + 2, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[2][3]},
        {METEOR_R2 + 3, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[2][0]},
    },
    {
        {METEOR_R3 + 0, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[3][1]},
        {METEOR_R3 + 1, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[3][2]},
        {METEOR_R3 + 2, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[3][3]},
        {METEOR_R3 + 3, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[3][0]},
    },
    {
        {METEOR_R4 + 0, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[4][1]},
        {METEOR_R4 + 1, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[4][2]},
        {METEOR_R4 + 2, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[4][3]},
        {METEOR_R4 + 3, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[4][0]},
    }
};


STATEp sg_MirvMeteor[] =
{
    &s_MirvMeteor[0][0],
    &s_MirvMeteor[1][0],
    &s_MirvMeteor[2][0],
    &s_MirvMeteor[3][0],
    &s_MirvMeteor[4][0]
};

STATE s_MirvMeteorExp[] =
{
    {METEOR_EXP + 0, METEOR_EXP_RATE, NullAnimator, &s_MirvMeteorExp[1]},
    {METEOR_EXP + 1, METEOR_EXP_RATE, NullAnimator, &s_MirvMeteorExp[2]},
    {METEOR_EXP + 2, METEOR_EXP_RATE, NullAnimator, &s_MirvMeteorExp[3]},
    {METEOR_EXP + 3, METEOR_EXP_RATE, NullAnimator, &s_MirvMeteorExp[4]},
    {METEOR_EXP + 4, METEOR_EXP_RATE, NullAnimator, &s_MirvMeteorExp[5]},
    {METEOR_EXP + 5, METEOR_EXP_RATE, NullAnimator, &s_MirvMeteorExp[6]},
    {METEOR_EXP + 5, METEOR_EXP_RATE, DoSuicide, &s_MirvMeteorExp[6]}
};

#define SERP_METEOR METEOR_R0+1
ANIMATOR DoSerpMeteor;
STATE s_SerpMeteor[5][4] =
{
    {
        {2031 + 0, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[0][1]},
        {2031 + 1, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[0][2]},
        {2031 + 2, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[0][3]},
        {2031 + 3, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[0][0]},
    },
    {
        {2031 + 0, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[1][1]},
        {2031 + 1, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[1][2]},
        {2031 + 2, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[1][3]},
        {2031 + 3, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[1][0]},
    },
    {
        {2031 + 0, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[2][1]},
        {2031 + 1, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[2][2]},
        {2031 + 2, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[2][3]},
        {2031 + 3, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[2][0]},
    },
    {
        {2031 + 0, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[3][1]},
        {2031 + 1, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[3][2]},
        {2031 + 2, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[3][3]},
        {2031 + 3, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[3][0]},
    },
    {
        {2031 + 0, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[4][1]},
        {2031 + 1, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[4][2]},
        {2031 + 2, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[4][3]},
        {2031 + 3, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[4][0]},
    }
};


STATEp sg_SerpMeteor[] =
{
    &s_SerpMeteor[0][0],
    &s_SerpMeteor[1][0],
    &s_SerpMeteor[2][0],
    &s_SerpMeteor[3][0],
    &s_SerpMeteor[4][0]
};


STATE s_SerpMeteorExp[] =
{
    {METEOR_EXP + 0, METEOR_EXP_RATE, NullAnimator, &s_SerpMeteorExp[1]},
    {METEOR_EXP + 1, METEOR_EXP_RATE, NullAnimator, &s_SerpMeteorExp[2]},
    {METEOR_EXP + 2, METEOR_EXP_RATE, NullAnimator, &s_SerpMeteorExp[3]},
    {METEOR_EXP + 3, METEOR_EXP_RATE, NullAnimator, &s_SerpMeteorExp[4]},
    {METEOR_EXP + 4, METEOR_EXP_RATE, NullAnimator, &s_SerpMeteorExp[5]},
    {METEOR_EXP + 5, METEOR_EXP_RATE, NullAnimator, &s_SerpMeteorExp[6]},
    {METEOR_EXP + 5, METEOR_EXP_RATE, DoSuicide, &s_SerpMeteorExp[6]}
};

//////////////////////
//
// SPEAR
//
//////////////////////

#define SPEAR_RATE 8
ANIMATOR DoSpear;

STATE s_Spear[5][1] =
{
    {
        {SPEAR_R0 + 0, SPEAR_RATE, DoSpear, s_Spear[0]},
    },
    {
        {SPEAR_R1 + 0, SPEAR_RATE, DoSpear, s_Spear[1]},
    },
    {
        {SPEAR_R2 + 0, SPEAR_RATE, DoSpear, s_Spear[2]},
    },
    {
        {SPEAR_R3 + 0, SPEAR_RATE, DoSpear, s_Spear[3]},
    },
    {
        {SPEAR_R4 + 0, SPEAR_RATE, DoSpear, s_Spear[4]},
    }
};

STATEp sg_Spear[] =
{
    s_Spear[0],
    s_Spear[1],
    s_Spear[2],
    s_Spear[3],
    s_Spear[4]
};

//////////////////////
//
// ROCKET
//
//////////////////////

#define ROCKET_FRAMES 1
#define ROCKET_R0 2206
#define ROCKET_R1 ROCKET_R0 + (ROCKET_FRAMES * 2)
#define ROCKET_R2 ROCKET_R0 + (ROCKET_FRAMES * 3)
#define ROCKET_R3 ROCKET_R0 + (ROCKET_FRAMES * 4)
#define ROCKET_R4 ROCKET_R0 + (ROCKET_FRAMES * 1)

ANIMATOR DoRocket;
#define ROCKET_RATE 8

STATE s_Rocket[5][1] =
{
    {
        {ROCKET_R0 + 0, ROCKET_RATE, DoRocket, &s_Rocket[0][0]},
    },
    {
        {ROCKET_R1 + 0, ROCKET_RATE, DoRocket, &s_Rocket[1][0]},
    },
    {
        {ROCKET_R2 + 0, ROCKET_RATE, DoRocket, &s_Rocket[2][0]},
    },
    {
        {ROCKET_R3 + 0, ROCKET_RATE, DoRocket, &s_Rocket[3][0]},
    },
    {
        {ROCKET_R4 + 0, ROCKET_RATE, DoRocket, &s_Rocket[4][0]},
    }
};

STATEp sg_Rocket[] =
{
    &s_Rocket[0][0],
    &s_Rocket[1][0],
    &s_Rocket[2][0],
    &s_Rocket[3][0],
    &s_Rocket[4][0]
};

//////////////////////
//
// BUNNY ROCKET
//
//////////////////////

#define BUNNYROCKET_FRAMES 5
#define BUNNYROCKET_R0 4550
#define BUNNYROCKET_R1 BUNNYROCKET_R0 + (BUNNYROCKET_FRAMES * 1)
#define BUNNYROCKET_R2 BUNNYROCKET_R0 + (BUNNYROCKET_FRAMES * 2)
#define BUNNYROCKET_R3 BUNNYROCKET_R0 + (BUNNYROCKET_FRAMES * 3)
#define BUNNYROCKET_R4 BUNNYROCKET_R0 + (BUNNYROCKET_FRAMES * 4)

ANIMATOR DoRocket;
#define BUNNYROCKET_RATE 8

STATE s_BunnyRocket[5][1] =
{
    {
        {BUNNYROCKET_R0 + 2, BUNNYROCKET_RATE, DoRocket, &s_BunnyRocket[0][0]},
    },
    {
        {BUNNYROCKET_R1 + 2, BUNNYROCKET_RATE, DoRocket, &s_BunnyRocket[1][0]},
    },
    {
        {BUNNYROCKET_R2 + 2, BUNNYROCKET_RATE, DoRocket, &s_BunnyRocket[2][0]},
    },
    {
        {BUNNYROCKET_R3 + 2, BUNNYROCKET_RATE, DoRocket, &s_BunnyRocket[3][0]},
    },
    {
        {BUNNYROCKET_R4 + 2, BUNNYROCKET_RATE, DoRocket, &s_BunnyRocket[4][0]},
    }
};

STATEp sg_BunnyRocket[] =
{
    &s_BunnyRocket[0][0],
    &s_BunnyRocket[1][0],
    &s_BunnyRocket[2][0],
    &s_BunnyRocket[3][0],
    &s_BunnyRocket[4][0]
};

ANIMATOR DoRail;
#define RAIL_RATE 8

STATE s_Rail[5][1] =
{
    {
        {ROCKET_R0 + 0, RAIL_RATE, DoRail, &s_Rail[0][0]},
    },
    {
        {ROCKET_R1 + 0, RAIL_RATE, DoRail, &s_Rail[1][0]},
    },
    {
        {ROCKET_R2 + 0, RAIL_RATE, DoRail, &s_Rail[2][0]},
    },
    {
        {ROCKET_R3 + 0, RAIL_RATE, DoRail, &s_Rail[3][0]},
    },
    {
        {ROCKET_R4 + 0, RAIL_RATE, DoRail, &s_Rail[4][0]},
    }
};

STATEp sg_Rail[] =
{
    &s_Rail[0][0],
    &s_Rail[1][0],
    &s_Rail[2][0],
    &s_Rail[3][0],
    &s_Rail[4][0]
};

ANIMATOR DoLaser;
#define LASER_RATE 8

STATE s_Laser[] =
{
    {ROCKET_R0 + 0, LASER_RATE, DoLaser, &s_Laser[0]}
};

//////////////////////
//
// MICRO
//
//////////////////////

#define MICRO_FRAMES 1
#define MICRO_R0 2206
#define MICRO_R1 MICRO_R0 + (MICRO_FRAMES * 2)
#define MICRO_R2 MICRO_R0 + (MICRO_FRAMES * 3)
#define MICRO_R3 MICRO_R0 + (MICRO_FRAMES * 4)
#define MICRO_R4 MICRO_R0 + (MICRO_FRAMES * 1)

ANIMATOR DoMicro;
#define MICRO_RATE 8

#if 0
#define PUFF 1748
#define PUFF_RATE 8
ANIMATOR DoMicroPuffSuicide;
STATE s_MicroPuff[] =
{
    {PUFF + 0, PUFF_RATE, DoPuff, &s_MicroPuff[1]},
    {PUFF + 1, PUFF_RATE, DoPuff, &s_MicroPuff[2]},
    {PUFF + 2, PUFF_RATE, DoPuff, &s_MicroPuff[3]},
    {PUFF + 3, PUFF_RATE, DoPuff, &s_MicroPuff[4]},
    {PUFF + 4, PUFF_RATE, DoPuff, &s_MicroPuff[5]},
    {PUFF + 5, PUFF_RATE, DoPuff, &s_MicroPuff[6]},
    {PUFF + 5, 100, DoMicroPuffSuicide, &s_MicroPuff[6]}
};
#endif

STATE s_Micro[5][1] =
{
    {
        {MICRO_R0 + 0, MICRO_RATE, DoMicro, &s_Micro[0][0]},
    },
    {
        {MICRO_R1 + 0, MICRO_RATE, DoMicro, &s_Micro[1][0]},
    },
    {
        {MICRO_R2 + 0, MICRO_RATE, DoMicro, &s_Micro[2][0]},
    },
    {
        {MICRO_R3 + 0, MICRO_RATE, DoMicro, &s_Micro[3][0]},
    },
    {
        {MICRO_R4 + 0, MICRO_RATE, DoMicro, &s_Micro[4][0]},
    }
};

STATEp sg_Micro[] =
{
    &s_Micro[0][0],
    &s_Micro[1][0],
    &s_Micro[2][0],
    &s_Micro[3][0],
    &s_Micro[4][0]
};

ANIMATOR DoMicroMini;
STATE s_MicroMini[5][1] =
{
    {
        {MICRO_R0 + 0, MICRO_RATE, DoMicroMini, &s_MicroMini[0][0]},
    },
    {
        {MICRO_R1 + 0, MICRO_RATE, DoMicroMini, &s_MicroMini[1][0]},
    },
    {
        {MICRO_R2 + 0, MICRO_RATE, DoMicroMini, &s_MicroMini[2][0]},
    },
    {
        {MICRO_R3 + 0, MICRO_RATE, DoMicroMini, &s_MicroMini[3][0]},
    },
    {
        {MICRO_R4 + 0, MICRO_RATE, DoMicroMini, &s_MicroMini[4][0]},
    }
};

STATEp sg_MicroMini[] =
{
    &s_MicroMini[0][0],
    &s_MicroMini[1][0],
    &s_MicroMini[2][0],
    &s_MicroMini[3][0],
    &s_MicroMini[4][0]
};

//////////////////////
//
// BOLT THINMAN
//
//////////////////////

#define BOLT_THINMAN_RATE 8
ANIMATOR DoBoltThinMan;

STATE s_BoltThinMan[5][1] =
{
    {
        {BOLT_THINMAN_R0 + 0, BOLT_THINMAN_RATE, DoBoltThinMan, &s_BoltThinMan[0][0]},
    },
    {
        {BOLT_THINMAN_R1 + 0, BOLT_THINMAN_RATE, DoBoltThinMan, &s_BoltThinMan[1][0]},
    },
    {
        {BOLT_THINMAN_R2 + 0, BOLT_THINMAN_RATE, DoBoltThinMan, &s_BoltThinMan[2][0]},
    },
    {
        {BOLT_THINMAN_R3 + 0, BOLT_THINMAN_RATE, DoBoltThinMan, &s_BoltThinMan[3][0]},
    },
    {
        {BOLT_THINMAN_R4 + 0, BOLT_THINMAN_RATE, DoBoltThinMan, &s_BoltThinMan[4][0]},
    }
};

STATEp sg_BoltThinMan[] =
{
    &s_BoltThinMan[0][0],
    &s_BoltThinMan[1][0],
    &s_BoltThinMan[2][0],
    &s_BoltThinMan[3][0],
    &s_BoltThinMan[4][0]
};

#define BOLT_SEEKER_RATE 8
ANIMATOR DoBoltSeeker;

STATE s_BoltSeeker[5][1] =
{
    {
        {BOLT_THINMAN_R0 + 0, BOLT_SEEKER_RATE, DoBoltSeeker, &s_BoltSeeker[0][0]},
    },
    {
        {BOLT_THINMAN_R1 + 0, BOLT_SEEKER_RATE, DoBoltSeeker, &s_BoltSeeker[1][0]},
    },
    {
        {BOLT_THINMAN_R2 + 0, BOLT_SEEKER_RATE, DoBoltSeeker, &s_BoltSeeker[2][0]},
    },
    {
        {BOLT_THINMAN_R3 + 0, BOLT_SEEKER_RATE, DoBoltSeeker, &s_BoltSeeker[3][0]},
    },
    {
        {BOLT_THINMAN_R4 + 0, BOLT_SEEKER_RATE, DoBoltSeeker, &s_BoltSeeker[4][0]},
    }
};

STATEp sg_BoltSeeker[] =
{
    &s_BoltSeeker[0][0],
    &s_BoltSeeker[1][0],
    &s_BoltSeeker[2][0],
    &s_BoltSeeker[3][0],
    &s_BoltSeeker[4][0]
};


#define BOLT_FATMAN STAR
#define BOLT_FATMAN_RATE 8
ANIMATOR DoBoltFatMan;

STATE s_BoltFatMan[] =
{
    {BOLT_FATMAN + 0, BOLT_FATMAN_RATE, DoBoltFatMan, &s_BoltFatMan[1]},
    {BOLT_FATMAN + 1, BOLT_FATMAN_RATE, DoBoltFatMan, &s_BoltFatMan[2]},
    {BOLT_FATMAN + 2, BOLT_FATMAN_RATE, DoBoltFatMan, &s_BoltFatMan[3]},
    {BOLT_FATMAN + 3, BOLT_FATMAN_RATE, DoBoltFatMan, &s_BoltFatMan[0]}
};


#define BOLT_SHRAPNEL STAR
#define BOLT_SHRAPNEL_RATE 8
ANIMATOR DoBoltShrapnel;

STATE s_BoltShrapnel[] =
{
    {BOLT_SHRAPNEL + 0, BOLT_SHRAPNEL_RATE, DoBoltShrapnel, &s_BoltShrapnel[1]},
    {BOLT_SHRAPNEL + 1, BOLT_SHRAPNEL_RATE, DoBoltShrapnel, &s_BoltShrapnel[2]},
    {BOLT_SHRAPNEL + 2, BOLT_SHRAPNEL_RATE, DoBoltShrapnel, &s_BoltShrapnel[3]},
    {BOLT_SHRAPNEL + 3, BOLT_SHRAPNEL_RATE, DoBoltShrapnel, &s_BoltShrapnel[0]}
};

#define COOLG_FIRE 2430
//#define COOLG_FIRE 1465
#define COOLG_FIRE_RATE 8
ANIMATOR DoCoolgFire;

STATE s_CoolgFire[] =
{
    {2031 + 0, COOLG_FIRE_RATE, DoCoolgFire, &s_CoolgFire[1]},
    {2031 + 1, COOLG_FIRE_RATE, DoCoolgFire, &s_CoolgFire[2]},
    {2031 + 2, COOLG_FIRE_RATE, DoCoolgFire, &s_CoolgFire[3]},
    {2031 + 3, COOLG_FIRE_RATE, DoCoolgFire, &s_CoolgFire[0]}
};

#define COOLG_FIRE_DONE 2410
//#define COOLG_FIRE_DONE 1466
//#define COOLG_FIRE_DONE_RATE 8
#define COOLG_FIRE_DONE_RATE 3

STATE s_CoolgFireDone[] =
{
    {COOLG_FIRE_DONE + 0, COOLG_FIRE_DONE_RATE, NullAnimator, &s_CoolgFireDone[1]},
    {COOLG_FIRE_DONE + 1, COOLG_FIRE_DONE_RATE, NullAnimator, &s_CoolgFireDone[2]},
    {COOLG_FIRE_DONE + 2, COOLG_FIRE_DONE_RATE, NullAnimator, &s_CoolgFireDone[3]},
    {COOLG_FIRE_DONE + 3, COOLG_FIRE_DONE_RATE, NullAnimator, &s_CoolgFireDone[4]},
    {COOLG_FIRE_DONE + 4, COOLG_FIRE_DONE_RATE, NullAnimator, &s_CoolgFireDone[5]},
    {COOLG_FIRE_DONE + 4, COOLG_FIRE_DONE_RATE, DoSuicide, &s_CoolgFireDone[5]}
};

ANIMATOR DoCoolgDrip;
#define COOLG_DRIP 1720
STATE s_CoolgDrip[] =
{
    {COOLG_DRIP + 0, 100, DoCoolgDrip, &s_CoolgDrip[0]}
};

#define GORE_FLOOR_SPLASH_RATE 8
#define GORE_FLOOR_SPLASH 1710
STATE s_GoreFloorSplash[] =
{
    {GORE_FLOOR_SPLASH + 0, GORE_FLOOR_SPLASH_RATE, NullAnimator, &s_GoreFloorSplash[1]},
    {GORE_FLOOR_SPLASH + 1, GORE_FLOOR_SPLASH_RATE, NullAnimator, &s_GoreFloorSplash[2]},
    {GORE_FLOOR_SPLASH + 2, GORE_FLOOR_SPLASH_RATE, NullAnimator, &s_GoreFloorSplash[3]},
    {GORE_FLOOR_SPLASH + 3, GORE_FLOOR_SPLASH_RATE, NullAnimator, &s_GoreFloorSplash[4]},
    {GORE_FLOOR_SPLASH + 4, GORE_FLOOR_SPLASH_RATE, NullAnimator, &s_GoreFloorSplash[5]},
    {GORE_FLOOR_SPLASH + 5, GORE_FLOOR_SPLASH_RATE, NullAnimator, &s_GoreFloorSplash[6]},
    {GORE_FLOOR_SPLASH + 5, GORE_FLOOR_SPLASH_RATE, DoSuicide, &s_GoreFloorSplash[6]}
};

#define GORE_SPLASH_RATE 8
#define GORE_SPLASH 2410
STATE s_GoreSplash[] =
{
    {GORE_SPLASH + 0, GORE_SPLASH_RATE, NullAnimator, &s_GoreSplash[1]},
    {GORE_SPLASH + 1, GORE_SPLASH_RATE, NullAnimator, &s_GoreSplash[2]},
    {GORE_SPLASH + 2, GORE_SPLASH_RATE, NullAnimator, &s_GoreSplash[3]},
    {GORE_SPLASH + 3, GORE_SPLASH_RATE, NullAnimator, &s_GoreSplash[4]},
    {GORE_SPLASH + 4, GORE_SPLASH_RATE, NullAnimator, &s_GoreSplash[5]},
    {GORE_SPLASH + 5, GORE_SPLASH_RATE, NullAnimator, &s_GoreSplash[6]},
    {GORE_SPLASH + 5, GORE_SPLASH_RATE, DoSuicide, &s_GoreSplash[6]}
};

//////////////////////////////////////////////
//
//  HEART ATTACK & PLASMA
//
//////////////////////////////////////////////

#define PLASMA 1562 //2058
#define PLASMA_FOUNTAIN 2058+1
#define PLASMA_RATE 8
#define PLASMA_FOUNTAIN_TIME (3*120);

ANIMATOR DoPlasma;

// regular bolt from heart
STATE s_Plasma[] =
{
    {PLASMA + 0, PLASMA_RATE, DoPlasma, &s_Plasma[1]},
    {PLASMA + 1, PLASMA_RATE, DoPlasma, &s_Plasma[2]},
    {PLASMA + 2, PLASMA_RATE, DoPlasma, &s_Plasma[0]}
};

ANIMATOR DoPlasmaFountain;

// follows actor spewing blood
#define PLASMA_Drip 1562 //2420
STATE s_PlasmaFountain[] =
{
    {PLASMA_Drip + 0, PLASMA_RATE, DoPlasmaFountain, &s_PlasmaFountain[1]},
    {PLASMA_Drip + 1, PLASMA_RATE, DoPlasmaFountain, &s_PlasmaFountain[2]},
    {PLASMA_Drip + 2, PLASMA_RATE, DoPlasmaFountain, &s_PlasmaFountain[3]},
    {PLASMA_Drip + 3, PLASMA_RATE, DoPlasmaFountain, &s_PlasmaFountain[0]},
};

#define PLASMA_Drip_RATE 12
STATE s_PlasmaDrip[] =
{
    {PLASMA_Drip + 0, PLASMA_Drip_RATE, DoShrapJumpFall, &s_PlasmaDrip[1]},
    {PLASMA_Drip + 1, PLASMA_Drip_RATE, DoShrapJumpFall, &s_PlasmaDrip[2]},
    {PLASMA_Drip + 2, PLASMA_Drip_RATE, DoShrapJumpFall, &s_PlasmaDrip[3]},
    {PLASMA_Drip + 3, PLASMA_Drip_RATE, DoShrapJumpFall, &s_PlasmaDrip[4]},
    {PLASMA_Drip + 4, PLASMA_Drip_RATE, DoShrapJumpFall, &s_PlasmaDrip[5]},
    {PLASMA_Drip + 5, PLASMA_Drip_RATE, DoShrapJumpFall, &s_PlasmaDrip[6]},
    {PLASMA_Drip + 7, PLASMA_Drip_RATE, DoSuicide, &s_PlasmaDrip[6]}
};

#define PLASMA_DONE 2061
#define PLASMA_DONE_RATE 15

ANIMATOR DoPlasmaDone;

STATE s_PlasmaDone[] =
{
    {PLASMA + 0, PLASMA_DONE_RATE, DoPlasmaDone, &s_PlasmaDone[1]},
    {PLASMA + 2, PLASMA_DONE_RATE, DoPlasmaDone, &s_PlasmaDone[2]},
    {PLASMA + 1, PLASMA_DONE_RATE, DoPlasmaDone, &s_PlasmaDone[0]}
};

#define TELEPORT_EFFECT 3240
#define TELEPORT_EFFECT_RATE 6
ANIMATOR DoTeleportEffect;
STATE s_TeleportEffect[] =
{
    {TELEPORT_EFFECT + 0, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[1]},
    {TELEPORT_EFFECT + 1, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[2]},
    {TELEPORT_EFFECT + 2, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[3]},
    {TELEPORT_EFFECT + 3, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[4]},
    {TELEPORT_EFFECT + 4, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[5]},
    {TELEPORT_EFFECT + 5, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[6]},
    {TELEPORT_EFFECT + 6, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[7]},
    {TELEPORT_EFFECT + 7, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[8]},
    {TELEPORT_EFFECT + 8, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[9]},
    {TELEPORT_EFFECT + 9, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[10]},
    {TELEPORT_EFFECT + 10, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[11]},
    {TELEPORT_EFFECT + 11, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[12]},
    {TELEPORT_EFFECT + 12, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[13]},
    {TELEPORT_EFFECT + 13, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[14]},
    {TELEPORT_EFFECT + 14, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[15]},
    {TELEPORT_EFFECT + 15, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[16]},
    {TELEPORT_EFFECT + 16, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[17]},
    {TELEPORT_EFFECT + 17, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[18]},
    {TELEPORT_EFFECT + 17, TELEPORT_EFFECT_RATE, DoSuicide, &s_TeleportEffect[18]},
};

// Spawn a RIPPER teleport effect
ANIMATOR DoTeleRipper;
STATE s_TeleportEffect2[] =
{
    {TELEPORT_EFFECT + 0, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[1]},
    {TELEPORT_EFFECT + 1, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[2]},
    {TELEPORT_EFFECT + 2, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[3]},
    {TELEPORT_EFFECT + 3, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[4]},
    {TELEPORT_EFFECT + 4, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[5]},
    {TELEPORT_EFFECT + 5, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[6]},
    {TELEPORT_EFFECT + 6, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[7]},
    {TELEPORT_EFFECT + 7, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[8]},
    {TELEPORT_EFFECT + 8, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[9]},
    {TELEPORT_EFFECT + 9, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[10]},
    {TELEPORT_EFFECT + 10, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[11]},
    {TELEPORT_EFFECT + 11, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[12]},
    {TELEPORT_EFFECT + 12, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[13]},
    {TELEPORT_EFFECT + 13, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[14]},
    {TELEPORT_EFFECT + 14, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[15]},
    {TELEPORT_EFFECT + 15, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[16]},
    {TELEPORT_EFFECT + 16, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[17]},
    {TELEPORT_EFFECT + 17, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[18]},
    {TELEPORT_EFFECT + 17, SF_QUICK_CALL, DoTeleRipper, &s_TeleportEffect2[19]},
    {TELEPORT_EFFECT + 17, TELEPORT_EFFECT_RATE, DoSuicide, &s_TeleportEffect2[19]},
};


ANIMATOR DoElectro;
#define ELECTRO_SNAKE 2073
#define ELECTRO_PLAYER (ELECTRO)
#define ELECTRO_ENEMY (ELECTRO + 1)
#define ELECTRO_SHARD (ELECTRO + 2)

STATE s_Electro[] =
{
    {ELECTRO + 0, 12, DoElectro, &s_Electro[1]},
    {ELECTRO + 1, 12, DoElectro, &s_Electro[2]},
    {ELECTRO + 2, 12, DoElectro, &s_Electro[3]},
    {ELECTRO + 3, 12, DoElectro, &s_Electro[0]}
};

STATE s_ElectroShrap[] =
{
    {ELECTRO + 0, 12, DoShrapDamage, &s_ElectroShrap[1]},
    {ELECTRO + 1, 12, DoShrapDamage, &s_ElectroShrap[2]},
    {ELECTRO + 2, 12, DoShrapDamage, &s_ElectroShrap[3]},
    {ELECTRO + 3, 12, DoShrapDamage, &s_ElectroShrap[0]}
};

//////////////////////
//
// EXPS
//
//////////////////////
#define GRENADE_EXP 3121
#define MINE_EXP GRENADE_EXP+1
#define GRENADE_EXP_RATE 6

#if 0
STATE s_GrenadeExp[] =
{
    {GRENADE_EXP + 0, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[1]},
    {GRENADE_EXP + 1, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[2]},
    {GRENADE_EXP + 2, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[3]},
    {GRENADE_EXP + 3, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[4]},
    {GRENADE_EXP + 4, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[5]},
    {GRENADE_EXP + 5, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[6]},
    {GRENADE_EXP + 6, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[7]},
    {GRENADE_EXP + 7, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[8]},
    {GRENADE_EXP + 8, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[9]},
    {GRENADE_EXP + 9, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[10]},
    {GRENADE_EXP + 10, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[11]},
    {GRENADE_EXP + 11, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[12]},
    {GRENADE_EXP + 12, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[13]},
    {GRENADE_EXP + 13, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[14]},
    {GRENADE_EXP + 14, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[15]},
    {GRENADE_EXP + 15, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[16]},
    {GRENADE_EXP + 16, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[17]},
    {GRENADE_EXP + 17, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[18]},
    {GRENADE_EXP + 18, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[19]},
    {GRENADE_EXP + 19, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[20]},
    {GRENADE_EXP + 20, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[21]},
    {GRENADE_EXP + 21, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[22]},
    {GRENADE_EXP + 21, 100, DoSuicide, &s_GrenadeExp[22]}
};
#else
STATE s_GrenadeSmallExp[] =
{
    {GRENADE_EXP + 0, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[1]},
    {GRENADE_EXP + 1, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[2]},
    {GRENADE_EXP + 2, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[3]},
    {GRENADE_EXP + 3, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[4]},
    {GRENADE_EXP + 4, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[5]},
    {GRENADE_EXP + 5, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[6]},
    {GRENADE_EXP + 6, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[7]},
    {GRENADE_EXP + 7, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[8]},
    {GRENADE_EXP + 8, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[9]},
    {GRENADE_EXP + 9, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[10]},
    {GRENADE_EXP + 10, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[11]},
    {GRENADE_EXP + 11, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[12]},
    {GRENADE_EXP + 12, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[13]},
    {GRENADE_EXP + 13, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[14]},
    {GRENADE_EXP + 14, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[15]},
    {GRENADE_EXP + 15, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[16]},
    {GRENADE_EXP + 16, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[17]},
    {GRENADE_EXP + 17, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[18]},
    {GRENADE_EXP + 18, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[19]},
    {GRENADE_EXP + 19, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[20]},
    {GRENADE_EXP + 20, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[21]},
    {GRENADE_EXP + 21, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[22]},
    {GRENADE_EXP + 21, 100, DoSuicide, &s_GrenadeSmallExp[22]}
};

ANIMATOR SpawnGrenadeSmallExp;
STATE s_GrenadeExp[] =
{
    {GRENADE_EXP + 0, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[1]},
    {GRENADE_EXP + 1, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[2]},
    {GRENADE_EXP + 2, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[3]},
    {GRENADE_EXP + 3, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[4]},
    {GRENADE_EXP + 4, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[5]},
    {GRENADE_EXP + 5, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[6]},
    {GRENADE_EXP + 6, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[7]},
    {GRENADE_EXP + 6, SF_QUICK_CALL,     SpawnGrenadeSmallExp, &s_GrenadeExp[8]},
    {GRENADE_EXP + 7, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[9]},
    {GRENADE_EXP + 8, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[10]},
    {GRENADE_EXP + 9, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[11]},
    {GRENADE_EXP + 10, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[12]},
    {GRENADE_EXP + 11, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[13]},
    {GRENADE_EXP + 12, SF_QUICK_CALL,     SpawnGrenadeSmallExp, &s_GrenadeExp[14]},
    {GRENADE_EXP + 12, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[15]},
    {GRENADE_EXP + 13, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[16]},
    {GRENADE_EXP + 14, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[17]},
    {GRENADE_EXP + 15, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[18]},
    {GRENADE_EXP + 16, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[19]},
    {GRENADE_EXP + 17, SF_QUICK_CALL,     SpawnGrenadeSmallExp, &s_GrenadeExp[20]},
    {GRENADE_EXP + 17, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[21]},
    {GRENADE_EXP + 18, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[22]},
    {GRENADE_EXP + 19, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[23]},
    {GRENADE_EXP + 20, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[24]},
    {GRENADE_EXP + 21, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[25]},
    {GRENADE_EXP + 21, 100, DoSuicide, &s_GrenadeExp[25]}
};
#endif

#define MINE_EXP GRENADE_EXP+1
ANIMATOR DoMineExp, DoMineExpMine;
STATE s_MineExp[] =
{
    {GRENADE_EXP + 0, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[1]},
    {GRENADE_EXP + 1, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[2]},
    {GRENADE_EXP + 2, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[3]},
    {GRENADE_EXP + 3, 0|SF_QUICK_CALL, DoMineExp, &s_MineExp[4]},
    {GRENADE_EXP + 3, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[5]},
    {GRENADE_EXP + 4, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[6]},
    {GRENADE_EXP + 5, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[7]},
    {GRENADE_EXP + 6, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[8]},
    {GRENADE_EXP + 7, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[9]},
    {GRENADE_EXP + 8, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[10]},
    {GRENADE_EXP + 9, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[11]},
    {GRENADE_EXP + 10, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[12]},
    {GRENADE_EXP + 11, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[13]},
    {GRENADE_EXP + 12, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[14]},
    {GRENADE_EXP + 13, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[15]},
    {GRENADE_EXP + 14, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[16]},
    {GRENADE_EXP + 15, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[17]},
    {GRENADE_EXP + 16, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[18]},
    {GRENADE_EXP + 17, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[19]},
    {GRENADE_EXP + 17, 0|SF_QUICK_CALL, DoMineExpMine, &s_MineExp[20]},
    {GRENADE_EXP + 18, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[21]},
    {GRENADE_EXP + 19, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[22]},
    {GRENADE_EXP + 20, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[23]},
    {GRENADE_EXP + 21, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[24]},
    {GRENADE_EXP + 21, 100, DoSuicide, &s_MineExp[24]}
};

#define EXP_RATE 7
#define BOLT_EXP EXP
#define FIREBALL_EXP EXP+1
#define BASIC_EXP EXP+2
#define SECTOR_EXP EXP+3
#define MICRO_EXP EXP+5
#define TRACER_EXP EXP+6
#define TANK_SHELL_EXP EXP+7

STATE s_BasicExp[] =
{
    {EXP + 0, EXP_RATE, NullAnimator, &s_BasicExp[1]},
    {EXP + 1, EXP_RATE, NullAnimator, &s_BasicExp[2]},
    {EXP + 2, EXP_RATE, NullAnimator, &s_BasicExp[3]},
    {EXP + 3, EXP_RATE, NullAnimator, &s_BasicExp[4]},
    {EXP + 4, EXP_RATE, NullAnimator, &s_BasicExp[5]},
    {EXP + 5, EXP_RATE, NullAnimator, &s_BasicExp[6]},
    {EXP + 6, EXP_RATE, NullAnimator, &s_BasicExp[7]},
    {EXP + 7, EXP_RATE, NullAnimator, &s_BasicExp[8]},
    {EXP + 8, EXP_RATE, NullAnimator, &s_BasicExp[9]},
    {EXP + 9, EXP_RATE, NullAnimator, &s_BasicExp[10]},
    {EXP + 10, EXP_RATE, NullAnimator, &s_BasicExp[11]},
    {EXP + 11, EXP_RATE, NullAnimator, &s_BasicExp[12]},
    {EXP + 12, EXP_RATE, NullAnimator, &s_BasicExp[13]},
    {EXP + 13, EXP_RATE, NullAnimator, &s_BasicExp[14]},
    {EXP + 14, EXP_RATE, NullAnimator, &s_BasicExp[15]},
    {EXP + 15, EXP_RATE, NullAnimator, &s_BasicExp[16]},
    {EXP + 16, EXP_RATE, NullAnimator, &s_BasicExp[17]},
    {EXP + 17, EXP_RATE, NullAnimator, &s_BasicExp[18]},
    {EXP + 18, EXP_RATE, NullAnimator, &s_BasicExp[19]},
    {EXP + 19, EXP_RATE, NullAnimator, &s_BasicExp[20]},
    {EXP + 20, 100, DoSuicide, &s_BasicExp[0]}
};

#define MICRO_EXP_RATE 3
ANIMATOR DoExpDamageTest;

STATE s_MicroExp[] =
{
    {EXP + 0, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[1]},
    {EXP + 0, SF_QUICK_CALL,  DoExpDamageTest,   &s_MicroExp[2]},
    {EXP + 1, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[3]},
    {EXP + 2, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[4]},
    {EXP + 3, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[5]},
    {EXP + 4, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[6]},
    {EXP + 5, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[7]},
    {EXP + 6, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[8]},
    {EXP + 7, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[9]},
    {EXP + 8, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[10]},
    {EXP + 9, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[11]},
    {EXP + 10, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[12]},
    {EXP + 11, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[13]},
    {EXP + 12, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[14]},
    {EXP + 13, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[15]},
    {EXP + 14, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[16]},
    {EXP + 15, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[17]},
    {EXP + 16, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[18]},
    {EXP + 17, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[19]},
    {EXP + 18, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[20]},
    {EXP + 19, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[21]},
    {EXP + 20, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[22]},
    {EXP + 20, 100, DoSuicide, &s_MicroExp[22]}
};


#define BIG_GUN_FLAME_RATE 15
STATE s_BigGunFlame[] =
{
    // first 3 frames
    {EXP + 0, BIG_GUN_FLAME_RATE, NullAnimator, &s_BigGunFlame[1]},
    {EXP + 1, BIG_GUN_FLAME_RATE, NullAnimator, &s_BigGunFlame[2]},
    {EXP + 2, BIG_GUN_FLAME_RATE, NullAnimator, &s_BigGunFlame[3]},
    // last 4 frames frames
    {EXP + 17, BIG_GUN_FLAME_RATE, NullAnimator, &s_BigGunFlame[4]},
    {EXP + 18, BIG_GUN_FLAME_RATE, NullAnimator, &s_BigGunFlame[5]},
    {EXP + 19, BIG_GUN_FLAME_RATE, NullAnimator, &s_BigGunFlame[6]},
    {EXP + 20, BIG_GUN_FLAME_RATE, NullAnimator, &s_BigGunFlame[7]},
    {EXP + 20, 100, DoSuicide, &s_BigGunFlame[0]}
};

STATE s_BoltExp[] =
{
    {EXP + 0, EXP_RATE, NullAnimator, &s_BoltExp[1]},
    {EXP + 0, SF_QUICK_CALL,  NullAnimator,  &s_BoltExp[2]},
    {EXP + 0, SF_QUICK_CALL,  SpawnShrapX,   &s_BoltExp[3]},
    {EXP + 1, EXP_RATE, NullAnimator, &s_BoltExp[4]},
    {EXP + 2, EXP_RATE, NullAnimator, &s_BoltExp[5]},
    {EXP + 3, EXP_RATE, NullAnimator, &s_BoltExp[6]},
    {EXP + 4, EXP_RATE, NullAnimator, &s_BoltExp[7]},
    {EXP + 5, EXP_RATE, NullAnimator, &s_BoltExp[8]},
    {EXP + 6, EXP_RATE, NullAnimator, &s_BoltExp[9]},
    {EXP + 7, EXP_RATE, NullAnimator, &s_BoltExp[10]},
    {EXP + 7, SF_QUICK_CALL,  SpawnShrapX,   &s_BoltExp[11]},
    {EXP + 8, EXP_RATE, NullAnimator, &s_BoltExp[12]},
    {EXP + 9, EXP_RATE, NullAnimator, &s_BoltExp[13]},
    {EXP + 10, EXP_RATE, NullAnimator, &s_BoltExp[14]},
    {EXP + 11, EXP_RATE, NullAnimator, &s_BoltExp[15]},
    {EXP + 12, EXP_RATE, NullAnimator, &s_BoltExp[16]},
    {EXP + 13, EXP_RATE, NullAnimator, &s_BoltExp[17]},
    {EXP + 14, EXP_RATE, NullAnimator, &s_BoltExp[18]},
    {EXP + 15, EXP_RATE, NullAnimator, &s_BoltExp[19]},
    {EXP + 16, EXP_RATE, NullAnimator, &s_BoltExp[20]},
    {EXP + 17, EXP_RATE, NullAnimator, &s_BoltExp[21]},
    {EXP + 18, EXP_RATE, NullAnimator, &s_BoltExp[22]},
    {EXP + 19, EXP_RATE, NullAnimator, &s_BoltExp[23]},
    {EXP + 20, EXP_RATE, NullAnimator, &s_BoltExp[24]},
    {EXP + 20, 100, DoSuicide, &s_BoltExp[0]}
};

STATE s_TankShellExp[] =
{
    {EXP + 0, EXP_RATE, NullAnimator, &s_TankShellExp[1]},
    {EXP + 0, SF_QUICK_CALL,  NullAnimator,  &s_TankShellExp[2]},
    {EXP + 0, SF_QUICK_CALL,  SpawnShrapX,   &s_TankShellExp[3]},
    {EXP + 1, EXP_RATE, NullAnimator, &s_TankShellExp[4]},
    {EXP + 2, EXP_RATE, NullAnimator, &s_TankShellExp[5]},
    {EXP + 3, EXP_RATE, NullAnimator, &s_TankShellExp[6]},
    {EXP + 4, EXP_RATE, NullAnimator, &s_TankShellExp[7]},
    {EXP + 5, EXP_RATE, NullAnimator, &s_TankShellExp[8]},
    {EXP + 6, EXP_RATE, NullAnimator, &s_TankShellExp[9]},
    {EXP + 7, EXP_RATE, NullAnimator, &s_TankShellExp[10]},
    {EXP + 7, SF_QUICK_CALL,  SpawnShrapX,   &s_TankShellExp[11]},
    {EXP + 8, EXP_RATE, NullAnimator, &s_TankShellExp[12]},
    {EXP + 9, EXP_RATE, NullAnimator, &s_TankShellExp[13]},
    {EXP + 10, EXP_RATE, NullAnimator, &s_TankShellExp[14]},
    {EXP + 11, EXP_RATE, NullAnimator, &s_TankShellExp[15]},
    {EXP + 12, EXP_RATE, NullAnimator, &s_TankShellExp[16]},
    {EXP + 13, EXP_RATE, NullAnimator, &s_TankShellExp[17]},
    {EXP + 14, EXP_RATE, NullAnimator, &s_TankShellExp[18]},
    {EXP + 15, EXP_RATE, NullAnimator, &s_TankShellExp[19]},
    {EXP + 16, EXP_RATE, NullAnimator, &s_TankShellExp[20]},
    {EXP + 17, EXP_RATE, NullAnimator, &s_TankShellExp[21]},
    {EXP + 18, EXP_RATE, NullAnimator, &s_TankShellExp[22]},
    {EXP + 19, EXP_RATE, NullAnimator, &s_TankShellExp[23]},
    {EXP + 20, EXP_RATE, NullAnimator, &s_TankShellExp[24]},
    {EXP + 20, 100, DoSuicide, &s_TankShellExp[0]}
};

#define TRACER_EXP_RATE 4
STATE s_TracerExp[] =
{
    {EXP + 0, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[1]},
    {EXP + 0, SF_QUICK_CALL,  NullAnimator,  &s_TracerExp[2]},
    {EXP + 0, SF_QUICK_CALL,  NullAnimator,   &s_TracerExp[3]},
    {EXP + 1, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[4]},
    {EXP + 2, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[5]},
    {EXP + 3, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[6]},
    {EXP + 4, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[7]},
    {EXP + 5, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[8]},
    {EXP + 6, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[9]},
    {EXP + 7, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[10]},
    {EXP + 7, SF_QUICK_CALL,  NullAnimator,   &s_TracerExp[11]},
    {EXP + 8, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[12]},
    {EXP + 9, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[13]},
    {EXP + 10, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[14]},
    {EXP + 11, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[15]},
    {EXP + 12, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[16]},
    {EXP + 13, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[17]},
    {EXP + 14, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[18]},
    {EXP + 15, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[19]},
    {EXP + 16, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[20]},
    {EXP + 17, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[21]},
    {EXP + 18, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[22]},
    {EXP + 19, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[23]},
    {EXP + 20, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[24]},
    {EXP + 20, 100, DoSuicide, &s_TracerExp[0]}
};

#define EXP_RATE 7
ANIMATOR SpawnShrapX;
ANIMATOR DoSectorExp;

STATE s_SectorExp[] =
{
    {EXP + 0, EXP_RATE, DoSectorExp, &s_SectorExp[1]},
    {EXP + 0, SF_QUICK_CALL,  SpawnShrapX,  &s_SectorExp[2]},
    {EXP + 0, SF_QUICK_CALL,  DoSectorExp,   &s_SectorExp[3]},
    {EXP + 1, EXP_RATE, DoSectorExp, &s_SectorExp[4]},
    {EXP + 2, EXP_RATE, DoSectorExp, &s_SectorExp[5]},
    {EXP + 3, EXP_RATE, DoSectorExp, &s_SectorExp[6]},
    {EXP + 4, EXP_RATE, DoSectorExp, &s_SectorExp[7]},
    {EXP + 5, EXP_RATE, DoSectorExp, &s_SectorExp[8]},
    {EXP + 6, EXP_RATE, DoSectorExp, &s_SectorExp[9]},
    {EXP + 7, EXP_RATE, DoSectorExp, &s_SectorExp[10]},
    {EXP + 7, SF_QUICK_CALL,  DoSectorExp,  &s_SectorExp[11]},
    {EXP + 8, EXP_RATE, DoSectorExp, &s_SectorExp[12]},
    {EXP + 9, EXP_RATE, DoSectorExp, &s_SectorExp[13]},
    {EXP + 10, EXP_RATE, DoSectorExp, &s_SectorExp[14]},
    {EXP + 11, EXP_RATE, DoSectorExp, &s_SectorExp[15]},
    {EXP + 12, EXP_RATE, DoSectorExp, &s_SectorExp[16]},
    {EXP + 13, EXP_RATE, DoSectorExp, &s_SectorExp[17]},
    {EXP + 14, EXP_RATE, DoSectorExp, &s_SectorExp[18]},
    {EXP + 15, EXP_RATE, DoSectorExp, &s_SectorExp[19]},
    {EXP + 16, EXP_RATE, DoSectorExp, &s_SectorExp[20]},
    {EXP + 17, EXP_RATE, DoSectorExp, &s_SectorExp[21]},
    {EXP + 18, EXP_RATE, DoSectorExp, &s_SectorExp[22]},
    {EXP + 19, EXP_RATE, DoSectorExp, &s_SectorExp[23]},
    {EXP + 20, EXP_RATE, DoSectorExp, &s_SectorExp[24]},
    {EXP + 20, 100, DoSuicide, &s_SectorExp[0]}
};


#define FIREBALL_DISS 3196
#define FIREBALL_DISS_RATE 8
STATE s_FireballExp[] =
{
    {FIREBALL_DISS + 0, FIREBALL_DISS_RATE, NullAnimator, &s_FireballExp[1]},
    {FIREBALL_DISS + 1, FIREBALL_DISS_RATE, NullAnimator, &s_FireballExp[2]},
    {FIREBALL_DISS + 2, FIREBALL_DISS_RATE, NullAnimator, &s_FireballExp[3]},
    {FIREBALL_DISS + 3, FIREBALL_DISS_RATE, NullAnimator, &s_FireballExp[4]},
    {FIREBALL_DISS + 4, FIREBALL_DISS_RATE, NullAnimator, &s_FireballExp[5]},
    {FIREBALL_DISS + 4, 100, DoSuicide, &s_FireballExp[0]}
};

#define NAP_EXP (3072)
#define NAP_EXP_RATE 6

STATE s_NapExp[] =
{
    {NAP_EXP + 0,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[1]},
    {NAP_EXP + 0,     0 | SF_QUICK_CALL,       DoDamageTest,  &s_NapExp[2]},
    {NAP_EXP + 1,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[3]},
    {NAP_EXP + 2,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[4]},
    {NAP_EXP + 3,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[5]},
    {NAP_EXP + 4,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[6]},
    {NAP_EXP + 5,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[7]},
    {NAP_EXP + 6,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[8]},
    {NAP_EXP + 7,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[9]},
    {NAP_EXP + 8,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[10]},
    {NAP_EXP + 9,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[11]},
    {NAP_EXP + 10,    NAP_EXP_RATE,      NullAnimator,  &s_NapExp[12]},
    {NAP_EXP + 11,    NAP_EXP_RATE,      NullAnimator,  &s_NapExp[13]},
    {NAP_EXP + 12,    NAP_EXP_RATE,      NullAnimator,  &s_NapExp[14]},
    {NAP_EXP + 13,    NAP_EXP_RATE,      NullAnimator,  &s_NapExp[15]},
    {NAP_EXP + 14,    NAP_EXP_RATE,      NullAnimator,  &s_NapExp[16]},
    {NAP_EXP + 15,    NAP_EXP_RATE-2,    NullAnimator,  &s_NapExp[17]},
    {NAP_EXP + 16,    NAP_EXP_RATE-2,    NullAnimator,  &s_NapExp[18]},
    {NAP_EXP + 17,    NAP_EXP_RATE-2,    NullAnimator,  &s_NapExp[19]},
    {NAP_EXP + 18,    NAP_EXP_RATE-2,    NullAnimator,  &s_NapExp[20]},
    {NAP_EXP + 19,    NAP_EXP_RATE-2,    NullAnimator,  &s_NapExp[21]},
    {NAP_EXP + 20,    NAP_EXP_RATE-2,    NullAnimator,  &s_NapExp[22]},
    {NAP_EXP + 21,    NAP_EXP_RATE-2,    NullAnimator,  &s_NapExp[23]},
    {NAP_EXP + 21,    NAP_EXP_RATE-2,    DoSuicide,     &s_NapExp[23]}
};

ANIMATOR DoFireballFlames;
#define FLAME_RATE 6

STATE s_FireballFlames[] =
{
    {FIREBALL_FLAMES + 0, FLAME_RATE, DoFireballFlames, &s_FireballFlames[1]},
    {FIREBALL_FLAMES + 1, FLAME_RATE, DoFireballFlames, &s_FireballFlames[2]},
    {FIREBALL_FLAMES + 2, FLAME_RATE, DoFireballFlames, &s_FireballFlames[3]},
    {FIREBALL_FLAMES + 3, FLAME_RATE, DoFireballFlames, &s_FireballFlames[4]},
    {FIREBALL_FLAMES + 4, FLAME_RATE, DoFireballFlames, &s_FireballFlames[5]},
    {FIREBALL_FLAMES + 5, FLAME_RATE, DoFireballFlames, &s_FireballFlames[6]},
    {FIREBALL_FLAMES + 6, FLAME_RATE, DoFireballFlames, &s_FireballFlames[7]},
    {FIREBALL_FLAMES + 7, FLAME_RATE, DoFireballFlames, &s_FireballFlames[8]},
    {FIREBALL_FLAMES + 8, FLAME_RATE, DoFireballFlames, &s_FireballFlames[9]},
    {FIREBALL_FLAMES + 9, FLAME_RATE, DoFireballFlames, &s_FireballFlames[10]},
    {FIREBALL_FLAMES +10, FLAME_RATE, DoFireballFlames, &s_FireballFlames[11]},
    {FIREBALL_FLAMES +11, FLAME_RATE, DoFireballFlames, &s_FireballFlames[12]},
    {FIREBALL_FLAMES +12, FLAME_RATE, DoFireballFlames, &s_FireballFlames[13]},
    {FIREBALL_FLAMES +13, FLAME_RATE, DoFireballFlames, &s_FireballFlames[0]},
};

ANIMATOR DoBreakFlames;
#define FLAME_RATE 6

STATE s_BreakFlames[] =
{
    {FIREBALL_FLAMES + 0, FLAME_RATE, DoBreakFlames, &s_BreakFlames[1]},
    {FIREBALL_FLAMES + 1, FLAME_RATE, DoBreakFlames, &s_BreakFlames[2]},
    {FIREBALL_FLAMES + 2, FLAME_RATE, DoBreakFlames, &s_BreakFlames[3]},
    {FIREBALL_FLAMES + 3, FLAME_RATE, DoBreakFlames, &s_BreakFlames[4]},
    {FIREBALL_FLAMES + 4, FLAME_RATE, DoBreakFlames, &s_BreakFlames[5]},
    {FIREBALL_FLAMES + 5, FLAME_RATE, DoBreakFlames, &s_BreakFlames[6]},
    {FIREBALL_FLAMES + 6, FLAME_RATE, DoBreakFlames, &s_BreakFlames[7]},
    {FIREBALL_FLAMES + 7, FLAME_RATE, DoBreakFlames, &s_BreakFlames[8]},
    {FIREBALL_FLAMES + 8, FLAME_RATE, DoBreakFlames, &s_BreakFlames[9]},
    {FIREBALL_FLAMES + 9, FLAME_RATE, DoBreakFlames, &s_BreakFlames[10]},
    {FIREBALL_FLAMES +10, FLAME_RATE, DoBreakFlames, &s_BreakFlames[11]},
    {FIREBALL_FLAMES +11, FLAME_RATE, DoBreakFlames, &s_BreakFlames[12]},
    {FIREBALL_FLAMES +12, FLAME_RATE, DoBreakFlames, &s_BreakFlames[13]},
    {FIREBALL_FLAMES +13, FLAME_RATE, DoBreakFlames, &s_BreakFlames[0]},
};

//////////////////////
//
// FIREBALL
//
//////////////////////

#if 1
ANIMATOR DoFireball;
#define FIREBALL_RATE 8
#define GORO_FIREBALL FIREBALL+1

STATE s_Fireball[] =
{
    {FIREBALL + 0, 12, DoFireball, &s_Fireball[1]},
    {FIREBALL + 1, 12, DoFireball, &s_Fireball[2]},
    {FIREBALL + 2, 12, DoFireball, &s_Fireball[3]},
    {FIREBALL + 3, 12, DoFireball, &s_Fireball[0]}
};

#else
#define GORO_FIREBALL FIREBALL_R0
#define FIREBALL_RATE 6
ANIMATOR DoFireball;

STATE s_Fireball[5][4] =
{
    {
        {FIREBALL_R0 + 0, FIREBALL_RATE, DoFireball, &s_Fireball[0][1]},
        {FIREBALL_R0 + 1, FIREBALL_RATE, DoFireball, &s_Fireball[0][2]},
        {FIREBALL_R0 + 2, FIREBALL_RATE, DoFireball, &s_Fireball[0][3]},
        {FIREBALL_R0 + 3, FIREBALL_RATE, DoFireball, &s_Fireball[0][0]},
    },
    {
        {FIREBALL_R1 + 0, FIREBALL_RATE, DoFireball, &s_Fireball[1][1]},
        {FIREBALL_R1 + 1, FIREBALL_RATE, DoFireball, &s_Fireball[1][2]},
        {FIREBALL_R1 + 2, FIREBALL_RATE, DoFireball, &s_Fireball[1][3]},
        {FIREBALL_R1 + 3, FIREBALL_RATE, DoFireball, &s_Fireball[1][0]},
    },
    {
        {FIREBALL_R2 + 0, FIREBALL_RATE, DoFireball, &s_Fireball[2][1]},
        {FIREBALL_R2 + 1, FIREBALL_RATE, DoFireball, &s_Fireball[2][2]},
        {FIREBALL_R2 + 2, FIREBALL_RATE, DoFireball, &s_Fireball[2][3]},
        {FIREBALL_R2 + 3, FIREBALL_RATE, DoFireball, &s_Fireball[2][0]},
    },
    {
        {FIREBALL_R3 + 0, FIREBALL_RATE, DoFireball, &s_Fireball[3][1]},
        {FIREBALL_R3 + 1, FIREBALL_RATE, DoFireball, &s_Fireball[3][2]},
        {FIREBALL_R3 + 2, FIREBALL_RATE, DoFireball, &s_Fireball[3][3]},
        {FIREBALL_R3 + 3, FIREBALL_RATE, DoFireball, &s_Fireball[3][0]},
    },
    {
        {FIREBALL_R4 + 0, FIREBALL_RATE, DoFireball, &s_Fireball[4][1]},
        {FIREBALL_R4 + 1, FIREBALL_RATE, DoFireball, &s_Fireball[4][2]},
        {FIREBALL_R4 + 2, FIREBALL_RATE, DoFireball, &s_Fireball[4][3]},
        {FIREBALL_R4 + 3, FIREBALL_RATE, DoFireball, &s_Fireball[4][0]},
    }
};

STATEp sg_Fireball[] =
{
    s_Fireball[0],
    s_Fireball[1],
    s_Fireball[2],
    s_Fireball[3],
    s_Fireball[4]
};
#endif

#if 0
ANIMATOR DoRing;

STATE s_Ring[5][4] =
{
    {
        {FIREBALL_R0 + 0, FIREBALL_RATE, DoRing, &s_Ring[0][1]},
        {FIREBALL_R0 + 1, FIREBALL_RATE, DoRing, &s_Ring[0][2]},
        {FIREBALL_R0 + 2, FIREBALL_RATE, DoRing, &s_Ring[0][3]},
        {FIREBALL_R0 + 3, FIREBALL_RATE, DoRing, &s_Ring[0][0]},
    },
    {
        {FIREBALL_R1 + 0, FIREBALL_RATE, DoRing, &s_Ring[1][1]},
        {FIREBALL_R1 + 1, FIREBALL_RATE, DoRing, &s_Ring[1][2]},
        {FIREBALL_R1 + 2, FIREBALL_RATE, DoRing, &s_Ring[1][3]},
        {FIREBALL_R1 + 3, FIREBALL_RATE, DoRing, &s_Ring[1][0]},
    },
    {
        {FIREBALL_R2 + 0, FIREBALL_RATE, DoRing, &s_Ring[2][1]},
        {FIREBALL_R2 + 1, FIREBALL_RATE, DoRing, &s_Ring[2][2]},
        {FIREBALL_R2 + 2, FIREBALL_RATE, DoRing, &s_Ring[2][3]},
        {FIREBALL_R2 + 3, FIREBALL_RATE, DoRing, &s_Ring[2][0]},
    },
    {
        {FIREBALL_R3 + 0, FIREBALL_RATE, DoRing, &s_Ring[3][1]},
        {FIREBALL_R3 + 1, FIREBALL_RATE, DoRing, &s_Ring[3][2]},
        {FIREBALL_R3 + 2, FIREBALL_RATE, DoRing, &s_Ring[3][3]},
        {FIREBALL_R3 + 3, FIREBALL_RATE, DoRing, &s_Ring[3][0]},
    },
    {
        {FIREBALL_R4 + 0, FIREBALL_RATE, DoRing, &s_Ring[4][1]},
        {FIREBALL_R4 + 1, FIREBALL_RATE, DoRing, &s_Ring[4][2]},
        {FIREBALL_R4 + 2, FIREBALL_RATE, DoRing, &s_Ring[4][3]},
        {FIREBALL_R4 + 3, FIREBALL_RATE, DoRing, &s_Ring[4][0]},
    }
};

STATEp sg_Ring[] =
{
    s_Ring[0],
    s_Ring[1],
    s_Ring[2],
    s_Ring[3],
    s_Ring[4]
};
#else
ANIMATOR DoRing;

STATE s_Ring[] =
{
    {FIREBALL + 0, 12, DoRing, &s_Ring[1]},
    {FIREBALL + 1, 12, DoRing, &s_Ring[2]},
    {FIREBALL + 2, 12, DoRing, &s_Ring[3]},
    {FIREBALL + 3, 12, DoRing, &s_Ring[0]}
};
#endif

STATE s_Ring2[] =
{
    {2031 + 0, 12, DoRing, &s_Ring2[1]},
    {2031 + 1, 12, DoRing, &s_Ring2[2]},
    {2031 + 2, 12, DoRing, &s_Ring2[3]},
    {2031 + 3, 12, DoRing, &s_Ring2[0]}
};

ANIMATOR DoNapalm;

STATE s_Napalm[] =
{
    {FIREBALL + 0, 12, DoNapalm, &s_Napalm[1]},
    {FIREBALL + 1, 12, DoNapalm, &s_Napalm[2]},
    {FIREBALL + 2, 12, DoNapalm, &s_Napalm[3]},
    {FIREBALL + 3, 12, DoNapalm, &s_Napalm[0]}
};


ANIMATOR DoBloodWorm;

#if 1
#define BLOOD_WORM 2106
STATE s_BloodWorm[] =
{
    {BLOOD_WORM + 0, 12, DoBloodWorm, &s_BloodWorm[1]},
    {BLOOD_WORM + 1, 12, DoBloodWorm, &s_BloodWorm[2]},
    {BLOOD_WORM + 2, 12, DoBloodWorm, &s_BloodWorm[3]},
    {BLOOD_WORM + 3, 12, DoBloodWorm, &s_BloodWorm[4]},
    {BLOOD_WORM + 2, 12, DoBloodWorm, &s_BloodWorm[5]},
    {BLOOD_WORM + 1, 12, DoBloodWorm, &s_BloodWorm[0]}
};
#else
#define BLOOD_WORM FIREBALL+5
STATE s_BloodWorm[] =
{
    {FIREBALL + 0, 12, DoBloodWorm, &s_BloodWorm[1]},
    {FIREBALL + 1, 12, DoBloodWorm, &s_BloodWorm[2]},
    {FIREBALL + 2, 12, DoBloodWorm, &s_BloodWorm[3]},
    {FIREBALL + 3, 12, DoBloodWorm, &s_BloodWorm[0]}
};
#endif

#if 1
#define PLASMA_EXP BLOOD_WORM+1
#define PLASMA_EXP_RATE 8

STATE s_PlasmaExp[] =
{
    {BLOOD_WORM + 0, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[1]},
    {BLOOD_WORM + 1, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[2]},
    {BLOOD_WORM + 2, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[3]},
    {BLOOD_WORM + 3, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[4]},
    {BLOOD_WORM + 2, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[5]},
    {BLOOD_WORM + 1, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[6]},
    {BLOOD_WORM + 0, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[7]},
    {BLOOD_WORM + 1, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[8]},
    {BLOOD_WORM + 2, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[9]},
    {BLOOD_WORM + 3, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[10]},
    {BLOOD_WORM + 2, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[11]},
    {BLOOD_WORM + 1, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[12]},
    {BLOOD_WORM + 0, PLASMA_EXP_RATE, DoSuicide, &s_PlasmaExp[12]},
};

#else
#define PLASMA_EXP (NAP_EXP+1)
#define PLASMA_EXP_RATE 4

STATE s_PlasmaExp[] =
{
    {PLASMA_EXP + 0,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[1]},
    {PLASMA_EXP + 0,     0 | SF_QUICK_CALL,    DoDamageTest,  &s_PlasmaExp[2]},
    {PLASMA_EXP + 1,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[3]},
    {PLASMA_EXP + 2,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[4]},
    {PLASMA_EXP + 3,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[5]},
    {PLASMA_EXP + 4,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[6]},
    {PLASMA_EXP + 5,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[7]},
    {PLASMA_EXP + 6,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[8]},
    {PLASMA_EXP + 7,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[9]},
    {PLASMA_EXP + 8,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[10]},
    {PLASMA_EXP + 9,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[11]},
    {PLASMA_EXP + 10,    PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[12]},
    {PLASMA_EXP + 11,    PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[13]},
    {PLASMA_EXP + 9,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[14]},
    {PLASMA_EXP + 8,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[15]},
    {PLASMA_EXP + 7,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[16]},
    {PLASMA_EXP + 6,     PLASMA_EXP_RATE-2,    NullAnimator,  &s_PlasmaExp[17]},
    {PLASMA_EXP + 5,     PLASMA_EXP_RATE-2,    NullAnimator,  &s_PlasmaExp[18]},
    {PLASMA_EXP + 4,     PLASMA_EXP_RATE-2,    NullAnimator,  &s_PlasmaExp[19]},
    {PLASMA_EXP + 3,     PLASMA_EXP_RATE-2,    NullAnimator,  &s_PlasmaExp[20]},
    {PLASMA_EXP + 2,     PLASMA_EXP_RATE-2,    NullAnimator,  &s_PlasmaExp[21]},
    {PLASMA_EXP + 1,     PLASMA_EXP_RATE-2,    NullAnimator,  &s_PlasmaExp[22]},
    {PLASMA_EXP + 1,     PLASMA_EXP_RATE-2,    NullAnimator,  &s_PlasmaExp[23]},
    {PLASMA_EXP + 1,     PLASMA_EXP_RATE-2,    DoSuicide,     &s_PlasmaExp[23]}
};
#endif


ANIMATOR DoMirv;

STATE s_Mirv[] =
{
    {FIREBALL + 0, 12, DoMirv, &s_Mirv[1]},
    {FIREBALL + 1, 12, DoMirv, &s_Mirv[2]},
    {FIREBALL + 2, 12, DoMirv, &s_Mirv[3]},
    {FIREBALL + 3, 12, DoMirv, &s_Mirv[0]}
};

ANIMATOR DoMirvMissile;

STATE s_MirvMissile[] =
{
    {FIREBALL + 0, 12, DoMirvMissile, &s_MirvMissile[1]},
    {FIREBALL + 1, 12, DoMirvMissile, &s_MirvMissile[2]},
    {FIREBALL + 2, 12, DoMirvMissile, &s_MirvMissile[3]},
    {FIREBALL + 3, 12, DoMirvMissile, &s_MirvMissile[0]}
};

//#define Vomit1 1740
//#define Vomit2 1741
#define Vomit1 1719
#define Vomit2 1721
//#define VomitSplash 1742
#define VomitSplash 1711
#define Vomit_RATE 16
ANIMATOR DoVomit,DoVomitSplash;

STATE s_Vomit1[] =
{
    {Vomit1 + 0, Vomit_RATE, DoVomit, &s_Vomit1[0]}
};

STATE s_Vomit2[] =
{
    {Vomit2 + 0, Vomit_RATE, DoVomit, &s_Vomit2[0]}
};

STATE s_VomitSplash[] =
{
    {VomitSplash + 0, Vomit_RATE, DoVomitSplash, &s_VomitSplash[0]}
};

#define GORE_Head 1670
#define GORE_Head_RATE 16

STATE s_GoreHead[] =
{
    {GORE_Head + 0, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[1]},
    {GORE_Head + 1, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[2]},
    {GORE_Head + 2, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[3]},
    {GORE_Head + 3, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[4]},
    {GORE_Head + 4, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[5]},
    {GORE_Head + 5, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[6]},
    {GORE_Head + 6, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[7]},
    {GORE_Head + 7, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[8]},
    {GORE_Head + 8, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[9]},
    {GORE_Head + 9, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[10]},
    {GORE_Head + 10, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[11]},
    {GORE_Head + 11, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[0]},
};

#define GORE_Leg 1689
#define GORE_Leg_RATE 16

STATE s_GoreLeg[] =
{
    {GORE_Leg + 0, GORE_Leg_RATE, DoShrapJumpFall, &s_GoreLeg[1]},
    {GORE_Leg + 1, GORE_Leg_RATE, DoShrapJumpFall, &s_GoreLeg[2]},
    {GORE_Leg + 2, GORE_Leg_RATE, DoShrapJumpFall, &s_GoreLeg[0]},
};

#define GORE_Eye 1692
#define GORE_Eye_RATE 16

STATE s_GoreEye[] =
{
    {GORE_Eye + 0, GORE_Eye_RATE, DoShrapJumpFall, &s_GoreEye[1]},
    {GORE_Eye + 1, GORE_Eye_RATE, DoShrapJumpFall, &s_GoreEye[2]},
    {GORE_Eye + 2, GORE_Eye_RATE, DoShrapJumpFall, &s_GoreEye[3]},
    {GORE_Eye + 3, GORE_Eye_RATE, DoShrapJumpFall, &s_GoreEye[0]},
};

#define GORE_Torso 1696
#define GORE_Torso_RATE 16

STATE s_GoreTorso[] =
{
    {GORE_Torso + 0, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[1]},
    {GORE_Torso + 1, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[2]},
    {GORE_Torso + 2, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[3]},
    {GORE_Torso + 3, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[4]},
    {GORE_Torso + 4, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[5]},
    {GORE_Torso + 5, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[6]},
    {GORE_Torso + 6, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[7]},
    {GORE_Torso + 7, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[0]},
};


#define GORE_Arm 1550
#define GORE_Arm_RATE 16

STATE s_GoreArm[] =
{
    {GORE_Arm + 0, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[1]},
    {GORE_Arm + 1, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[2]},
    {GORE_Arm + 2, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[3]},
    {GORE_Arm + 3, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[4]},
    {GORE_Arm + 4, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[5]},
    {GORE_Arm + 5, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[6]},
    {GORE_Arm + 6, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[7]},
    {GORE_Arm + 7, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[8]},
    {GORE_Arm + 8, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[9]},
    {GORE_Arm + 9, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[10]},
    {GORE_Arm + 10, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[11]},
    {GORE_Arm + 11, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[0]},
};

#define GORE_Lung 903
#define GORE_Lung_RATE 16

STATE s_GoreLung[] =
{
    {GORE_Lung + 0, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[1]},
    {GORE_Lung + 1, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[2]},
    {GORE_Lung + 2, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[3]},
    {GORE_Lung + 3, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[4]},
    {GORE_Lung + 4, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[5]},
    {GORE_Lung + 5, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[6]},
    {GORE_Lung + 6, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[7]},
    {GORE_Lung + 7, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[8]},
    {GORE_Lung + 8, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[9]},
    {GORE_Lung + 9, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[10]},
    {GORE_Lung + 10, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[11]},
    {GORE_Lung + 11, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[0]},
};

#define GORE_Liver 918
#define GORE_Liver_RATE 16

STATE s_GoreLiver[] =
{
    {GORE_Liver + 0, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[1]},
    {GORE_Liver + 1, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[2]},
    {GORE_Liver + 2, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[3]},
    {GORE_Liver + 3, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[4]},
    {GORE_Liver + 4, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[5]},
    {GORE_Liver + 5, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[6]},
    {GORE_Liver + 6, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[7]},
    {GORE_Liver + 7, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[8]},
    {GORE_Liver + 8, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[9]},
    {GORE_Liver + 9, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[10]},
    {GORE_Liver + 10, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[11]},
    {GORE_Liver + 11, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[0]},
};

#define GORE_SkullCap 933
#define GORE_SkullCap_RATE 16

STATE s_GoreSkullCap[] =
{
    {GORE_SkullCap + 0, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[1]},
    {GORE_SkullCap + 1, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[2]},
    {GORE_SkullCap + 2, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[3]},
    {GORE_SkullCap + 3, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[4]},
    {GORE_SkullCap + 4, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[5]},
    {GORE_SkullCap + 5, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[6]},
    {GORE_SkullCap + 6, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[7]},
    {GORE_SkullCap + 7, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[8]},
    {GORE_SkullCap + 8, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[9]},
    {GORE_SkullCap + 9, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[10]},
    {GORE_SkullCap + 10, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[11]},
    {GORE_SkullCap + 11, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[0]},
};


#define GORE_ChunkS 2430
#define GORE_ChunkS_RATE 16

STATE s_GoreChunkS[] =
{
    {GORE_ChunkS + 0, GORE_ChunkS_RATE, DoShrapJumpFall, &s_GoreChunkS[1]},
    {GORE_ChunkS + 1, GORE_ChunkS_RATE, DoShrapJumpFall, &s_GoreChunkS[2]},
    {GORE_ChunkS + 2, GORE_ChunkS_RATE, DoShrapJumpFall, &s_GoreChunkS[3]},
    {GORE_ChunkS + 3, GORE_ChunkS_RATE, DoShrapJumpFall, &s_GoreChunkS[0]},
};

#define GORE_Drip 1562 //2430
#define GORE_Drip_RATE 16

STATE s_GoreDrip[] =
{
    {GORE_Drip + 0, GORE_Drip_RATE, DoShrapJumpFall, &s_GoreDrip[1]},
    {GORE_Drip + 1, GORE_Drip_RATE, DoShrapJumpFall, &s_GoreDrip[2]},
    {GORE_Drip + 2, GORE_Drip_RATE, DoShrapJumpFall, &s_GoreDrip[3]},
    {GORE_Drip + 3, GORE_Drip_RATE, DoShrapJumpFall, &s_GoreDrip[0]},
};

STATE s_FastGoreDrip[] =
{
    {GORE_Drip + 0, GORE_Drip_RATE, DoFastShrapJumpFall, &s_FastGoreDrip[1]},
    {GORE_Drip + 1, GORE_Drip_RATE, DoFastShrapJumpFall, &s_FastGoreDrip[2]},
    {GORE_Drip + 2, GORE_Drip_RATE, DoFastShrapJumpFall, &s_FastGoreDrip[3]},
    {GORE_Drip + 3, GORE_Drip_RATE, DoFastShrapJumpFall, &s_FastGoreDrip[0]},
};

///////////////////////////////////////////////
//
// This GORE mostly for the Accursed Heads
//
///////////////////////////////////////////////

#define GORE_Flame 847
#define GORE_Flame_RATE 8

STATE s_GoreFlame[] =
{
    {GORE_Flame + 0, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[1]},
    {GORE_Flame + 1, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[2]},
    {GORE_Flame + 2, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[3]},
    {GORE_Flame + 3, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[4]},
    {GORE_Flame + 4, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[5]},
    {GORE_Flame + 5, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[6]},
    {GORE_Flame + 6, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[7]},
    {GORE_Flame + 7, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[0]},
};

ANIMATOR DoTracerShrap;
STATE s_TracerShrap[] =
{
    {GORE_Flame + 0, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[1]},
    {GORE_Flame + 1, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[2]},
    {GORE_Flame + 2, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[3]},
    {GORE_Flame + 3, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[4]},
    {GORE_Flame + 4, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[5]},
    {GORE_Flame + 5, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[6]},
    {GORE_Flame + 6, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[7]},
    {GORE_Flame + 7, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[0]},
};

#define UZI_SHELL 2152
#define UZISHELL_RATE 8
//ANIMATOR DoShellShrap;
STATE s_UziShellShrap[] =
{
    {UZI_SHELL + 0, UZISHELL_RATE, DoShrapJumpFall, &s_UziShellShrap[1]},
    {UZI_SHELL + 1, UZISHELL_RATE, DoShrapJumpFall, &s_UziShellShrap[2]},
    {UZI_SHELL + 2, UZISHELL_RATE, DoShrapJumpFall, &s_UziShellShrap[3]},
    {UZI_SHELL + 3, UZISHELL_RATE, DoShrapJumpFall, &s_UziShellShrap[4]},
    {UZI_SHELL + 4, UZISHELL_RATE, DoShrapJumpFall, &s_UziShellShrap[5]},
    {UZI_SHELL + 5, UZISHELL_RATE, DoShrapJumpFall, &s_UziShellShrap[0]},
};

STATE s_UziShellShrapStill1[] =
{
    {UZI_SHELL + 0, UZISHELL_RATE, NullAnimator, &s_UziShellShrapStill1[0]}
};
STATE s_UziShellShrapStill2[] =
{
    {UZI_SHELL + 1, UZISHELL_RATE, NullAnimator, &s_UziShellShrapStill2[0]}
};
STATE s_UziShellShrapStill3[] =
{
    {UZI_SHELL + 2, UZISHELL_RATE, NullAnimator, &s_UziShellShrapStill3[0]}
};
STATE s_UziShellShrapStill4[] =
{
    {UZI_SHELL + 3, UZISHELL_RATE, NullAnimator, &s_UziShellShrapStill4[0]}
};
STATE s_UziShellShrapStill5[] =
{
    {UZI_SHELL + 4, UZISHELL_RATE, NullAnimator, &s_UziShellShrapStill5[0]}
};
STATE s_UziShellShrapStill6[] =
{
    {UZI_SHELL + 5, UZISHELL_RATE, NullAnimator, &s_UziShellShrapStill6[0]}
};

#define SHOT_SHELL 2180
#define SHOTSHELL_RATE 8
STATE s_ShotgunShellShrap[] =
{
    {SHOT_SHELL + 0, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[1]},
    {SHOT_SHELL + 1, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[2]},
    {SHOT_SHELL + 2, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[3]},
    {SHOT_SHELL + 3, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[4]},
    {SHOT_SHELL + 4, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[5]},
    {SHOT_SHELL + 5, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[6]},
    {SHOT_SHELL + 6, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[7]},
    {SHOT_SHELL + 7, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[0]},
};

STATE s_ShotgunShellShrapStill1[] =
{
    {SHOT_SHELL + 1, SHOTSHELL_RATE, NullAnimator, &s_ShotgunShellShrapStill1[0]}
};
STATE s_ShotgunShellShrapStill2[] =
{
    {SHOT_SHELL + 3, SHOTSHELL_RATE, NullAnimator, &s_ShotgunShellShrapStill2[0]}
};
STATE s_ShotgunShellShrapStill3[] =
{
    {SHOT_SHELL + 7, SHOTSHELL_RATE, NullAnimator, &s_ShotgunShellShrapStill3[0]}
};

#define GORE_FlameChunkA 839
#define GORE_FlameChunkA_RATE 8

STATE s_GoreFlameChunkA[] =
{
    {GORE_FlameChunkA + 0, GORE_FlameChunkA_RATE, DoShrapJumpFall, &s_GoreFlameChunkA[1]},
    {GORE_FlameChunkA + 1, GORE_FlameChunkA_RATE, DoShrapJumpFall, &s_GoreFlameChunkA[2]},
    {GORE_FlameChunkA + 2, GORE_FlameChunkA_RATE, DoShrapJumpFall, &s_GoreFlameChunkA[3]},
    {GORE_FlameChunkA + 3, GORE_FlameChunkA_RATE, DoShrapJumpFall, &s_GoreFlameChunkA[0]},
};

#define GORE_FlameChunkB 843
#define GORE_FlameChunkB_RATE 8

STATE s_GoreFlameChunkB[] =
{
    {GORE_FlameChunkB + 0, GORE_FlameChunkB_RATE, DoShrapJumpFall, &s_GoreFlameChunkB[1]},
    {GORE_FlameChunkB + 1, GORE_FlameChunkB_RATE, DoShrapJumpFall, &s_GoreFlameChunkB[2]},
    {GORE_FlameChunkB + 2, GORE_FlameChunkB_RATE, DoShrapJumpFall, &s_GoreFlameChunkB[3]},
    {GORE_FlameChunkB + 3, GORE_FlameChunkB_RATE, DoShrapJumpFall, &s_GoreFlameChunkB[0]},
};

/////////////////////////////////////////////////////////////////////
//
// General Breaking Shrapnel
//
/////////////////////////////////////////////////////////////////////

#define COIN_SHRAP 2530
#define CoinShrap_RATE 12

STATE s_CoinShrap[] =
{
    {COIN_SHRAP + 0, CoinShrap_RATE, DoShrapJumpFall, &s_CoinShrap[1]},
    {COIN_SHRAP + 1, CoinShrap_RATE, DoShrapJumpFall, &s_CoinShrap[2]},
    {COIN_SHRAP + 2, CoinShrap_RATE, DoShrapJumpFall, &s_CoinShrap[3]},
    {COIN_SHRAP + 3, CoinShrap_RATE, DoShrapJumpFall, &s_CoinShrap[0]},
};

#define MARBEL 5096
#define Marbel_RATE 12

STATE s_Marbel[] =
{
    {MARBEL, Marbel_RATE, DoShrapJumpFall, &s_Marbel[0]},
};

//
// Glass
//

#define GLASS_SHRAP_A 3864
#define GlassShrapA_RATE 12

STATE s_GlassShrapA[] =
{
    {GLASS_SHRAP_A + 0, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[1]},
    {GLASS_SHRAP_A + 1, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[2]},
    {GLASS_SHRAP_A + 2, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[3]},
    {GLASS_SHRAP_A + 3, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[4]},
    {GLASS_SHRAP_A + 4, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[5]},
    {GLASS_SHRAP_A + 5, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[6]},
    {GLASS_SHRAP_A + 6, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[7]},
    {GLASS_SHRAP_A + 7, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[0]},
};

#define GLASS_SHRAP_B 3872
#define GlassShrapB_RATE 12

STATE s_GlassShrapB[] =
{
    {GLASS_SHRAP_B + 0, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[1]},
    {GLASS_SHRAP_B + 1, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[2]},
    {GLASS_SHRAP_B + 2, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[3]},
    {GLASS_SHRAP_B + 3, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[4]},
    {GLASS_SHRAP_B + 4, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[5]},
    {GLASS_SHRAP_B + 5, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[6]},
    {GLASS_SHRAP_B + 6, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[7]},
    {GLASS_SHRAP_B + 7, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[0]},
};

#define GLASS_SHRAP_C 3880
#define GlassShrapC_RATE 12

STATE s_GlassShrapC[] =
{
    {GLASS_SHRAP_C + 0, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[1]},
    {GLASS_SHRAP_C + 1, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[2]},
    {GLASS_SHRAP_C + 2, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[3]},
    {GLASS_SHRAP_C + 3, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[4]},
    {GLASS_SHRAP_C + 4, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[5]},
    {GLASS_SHRAP_C + 5, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[6]},
    {GLASS_SHRAP_C + 6, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[7]},
    {GLASS_SHRAP_C + 7, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[0]},
};

//
// Wood
//

#define WOOD_SHRAP_A 3924
#define WoodShrapA_RATE 12

STATE s_WoodShrapA[] =
{
    {WOOD_SHRAP_A + 0, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[1]},
    {WOOD_SHRAP_A + 1, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[2]},
    {WOOD_SHRAP_A + 2, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[3]},
    {WOOD_SHRAP_A + 3, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[4]},
    {WOOD_SHRAP_A + 4, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[5]},
    {WOOD_SHRAP_A + 5, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[6]},
    {WOOD_SHRAP_A + 6, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[7]},
    {WOOD_SHRAP_A + 7, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[0]},
};

#define WOOD_SHRAP_B 3932
#define WoodShrapB_RATE 12

STATE s_WoodShrapB[] =
{
    {WOOD_SHRAP_B + 0, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[1]},
    {WOOD_SHRAP_B + 1, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[2]},
    {WOOD_SHRAP_B + 2, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[3]},
    {WOOD_SHRAP_B + 3, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[4]},
    {WOOD_SHRAP_B + 4, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[5]},
    {WOOD_SHRAP_B + 5, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[6]},
    {WOOD_SHRAP_B + 6, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[7]},
    {WOOD_SHRAP_B + 7, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[0]},
};

#define WOOD_SHRAP_C 3941
#define WoodShrapC_RATE 12

STATE s_WoodShrapC[] =
{
    {WOOD_SHRAP_C + 0, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[1]},
    {WOOD_SHRAP_C + 1, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[2]},
    {WOOD_SHRAP_C + 2, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[3]},
    {WOOD_SHRAP_C + 3, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[4]},
    {WOOD_SHRAP_C + 4, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[5]},
    {WOOD_SHRAP_C + 5, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[6]},
    {WOOD_SHRAP_C + 6, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[7]},
    {WOOD_SHRAP_C + 7, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[0]},
};

//
// Stone
//

#define STONE_SHRAP_A 3840
#define StoneShrapA_RATE 12

STATE s_StoneShrapA[] =
{
    {STONE_SHRAP_A + 0, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[1]},
    {STONE_SHRAP_A + 1, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[2]},
    {STONE_SHRAP_A + 2, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[3]},
    {STONE_SHRAP_A + 3, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[4]},
    {STONE_SHRAP_A + 4, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[5]},
    {STONE_SHRAP_A + 5, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[6]},
    {STONE_SHRAP_A + 6, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[7]},
    {STONE_SHRAP_A + 7, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[0]},
};

#define STONE_SHRAP_B 3848
#define StoneShrapB_RATE 12

STATE s_StoneShrapB[] =
{
    {STONE_SHRAP_B + 0, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[1]},
    {STONE_SHRAP_B + 1, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[2]},
    {STONE_SHRAP_B + 2, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[3]},
    {STONE_SHRAP_B + 3, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[4]},
    {STONE_SHRAP_B + 4, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[5]},
    {STONE_SHRAP_B + 5, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[6]},
    {STONE_SHRAP_B + 6, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[7]},
    {STONE_SHRAP_B + 7, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[0]},
};

#define STONE_SHRAP_C 3856
#define StoneShrapC_RATE 12

STATE s_StoneShrapC[] =
{
    {STONE_SHRAP_C + 0, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[1]},
    {STONE_SHRAP_C + 1, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[2]},
    {STONE_SHRAP_C + 2, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[3]},
    {STONE_SHRAP_C + 3, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[4]},
    {STONE_SHRAP_C + 4, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[5]},
    {STONE_SHRAP_C + 5, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[6]},
    {STONE_SHRAP_C + 6, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[7]},
    {STONE_SHRAP_C + 7, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[0]},
};

//
// Metal
//

#define METAL_SHRAP_A 3888
#define MetalShrapA_RATE 12

STATE s_MetalShrapA[] =
{
    {METAL_SHRAP_A + 0, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[1]},
    {METAL_SHRAP_A + 1, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[2]},
    {METAL_SHRAP_A + 2, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[3]},
    {METAL_SHRAP_A + 3, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[4]},
    {METAL_SHRAP_A + 4, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[5]},
    {METAL_SHRAP_A + 5, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[6]},
    {METAL_SHRAP_A + 6, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[7]},
    {METAL_SHRAP_A + 7, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[0]},
};

#define METAL_SHRAP_B 3896
#define MetalShrapB_RATE 12

STATE s_MetalShrapB[] =
{
    {METAL_SHRAP_B + 0, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[1]},
    {METAL_SHRAP_B + 1, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[2]},
    {METAL_SHRAP_B + 2, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[3]},
    {METAL_SHRAP_B + 3, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[4]},
    {METAL_SHRAP_B + 4, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[5]},
    {METAL_SHRAP_B + 5, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[6]},
    {METAL_SHRAP_B + 6, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[7]},
    {METAL_SHRAP_B + 7, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[0]},
};

#define METAL_SHRAP_C 3904
#define MetalShrapC_RATE 12

STATE s_MetalShrapC[] =
{
    {METAL_SHRAP_C + 0, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[1]},
    {METAL_SHRAP_C + 1, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[2]},
    {METAL_SHRAP_C + 2, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[3]},
    {METAL_SHRAP_C + 3, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[4]},
    {METAL_SHRAP_C + 4, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[5]},
    {METAL_SHRAP_C + 5, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[6]},
    {METAL_SHRAP_C + 6, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[7]},
    {METAL_SHRAP_C + 7, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[0]},
};

//
// Paper
//

#define PAPER_SHRAP_A 3924
#define PaperShrapA_RATE 12

STATE s_PaperShrapA[] =
{
    {PAPER_SHRAP_A + 0, PaperShrapA_RATE, DoShrapJumpFall, &s_PaperShrapA[1]},
    {PAPER_SHRAP_A + 1, PaperShrapA_RATE, DoShrapJumpFall, &s_PaperShrapA[2]},
    {PAPER_SHRAP_A + 2, PaperShrapA_RATE, DoShrapJumpFall, &s_PaperShrapA[3]},
    {PAPER_SHRAP_A + 3, PaperShrapA_RATE, DoShrapJumpFall, &s_PaperShrapA[0]},
};

#define PAPER_SHRAP_B 3932
#define PaperShrapB_RATE 12

STATE s_PaperShrapB[] =
{
    {PAPER_SHRAP_B + 0, PaperShrapB_RATE, DoShrapJumpFall, &s_PaperShrapB[1]},
    {PAPER_SHRAP_B + 1, PaperShrapB_RATE, DoShrapJumpFall, &s_PaperShrapB[2]},
    {PAPER_SHRAP_B + 2, PaperShrapB_RATE, DoShrapJumpFall, &s_PaperShrapB[3]},
    {PAPER_SHRAP_B + 3, PaperShrapB_RATE, DoShrapJumpFall, &s_PaperShrapB[0]},
};

#define PAPER_SHRAP_C 3941
#define PaperShrapC_RATE 12

STATE s_PaperShrapC[] =
{
    {PAPER_SHRAP_C + 0, PaperShrapC_RATE, DoShrapJumpFall, &s_PaperShrapC[1]},
    {PAPER_SHRAP_C + 1, PaperShrapC_RATE, DoShrapJumpFall, &s_PaperShrapC[2]},
    {PAPER_SHRAP_C + 2, PaperShrapC_RATE, DoShrapJumpFall, &s_PaperShrapC[3]},
    {PAPER_SHRAP_C + 3, PaperShrapC_RATE, DoShrapJumpFall, &s_PaperShrapC[0]},
};

#if 1
SWBOOL MissileHitMatch(short Weapon, short WeaponNum, short hit_sprite)
{
    SPRITEp hsp = &sprite[hit_sprite];
    SPRITEp wp = &sprite[Weapon];
    USERp wu = User[Weapon];

    if (WeaponNum <= -1)
    {
        ASSERT(Weapon >= 0);
        WeaponNum = wu->WeaponNum;

        // can be hit by SO only
        if (SP_TAG7(hsp) == 4)
        {
            if (TEST(wu->Flags2, SPR2_SO_MISSILE))
            {
                DoMatchEverything(NULL, hsp->hitag, -1);
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
    }

    if (SP_TAG7(hsp) == 0)
    {
        switch (WeaponNum)
        {
        case WPN_RAIL:
        case WPN_MICRO:
        case WPN_NAPALM:
        case WPN_ROCKET:
            DoMatchEverything(NULL, hsp->hitag, -1);
            return TRUE;
        }
    }
    else if (SP_TAG7(hsp) == 1)
    {
        switch (WeaponNum)
        {
        case WPN_MICRO:
        case WPN_RAIL:
        case WPN_HOTHEAD:
        case WPN_NAPALM:
        case WPN_ROCKET:
            DoMatchEverything(NULL, hsp->hitag, -1);
            return TRUE;
        }
    }
    else if (SP_TAG7(hsp) == 2)
    {
        switch (WeaponNum)
        {
        case WPN_MICRO:
        case WPN_RAIL:
        case WPN_HOTHEAD:
        case WPN_NAPALM:
        case WPN_ROCKET:
        case WPN_UZI:
        case WPN_SHOTGUN:
            DoMatchEverything(NULL, hsp->hitag, -1);
            return TRUE;
        }
    }
    else if (SP_TAG7(hsp) == 3)
    {
        switch (WeaponNum)
        {
        case WPN_STAR:
        case WPN_SWORD:
        case WPN_FIST:
        case WPN_MICRO:
        case WPN_RAIL:
        case WPN_HOTHEAD:
        case WPN_NAPALM:
        case WPN_ROCKET:
        case WPN_UZI:
        case WPN_SHOTGUN:
            DoMatchEverything(NULL, hsp->hitag, -1);
            return TRUE;
        }
    }

    return FALSE;

#if 0
    WPN_STAR
    WPN_UZI
    WPN_SHOTGUN
    WPN_MICRO
    WPN_GRENADE
    WPN_MINE
    WPN_RAIL
    WPN_HEART
    WPN_HOTHEAD
    WPN_NAPALM
    WPN_RING
    WPN_ROCKET
    WPN_SWORD
        WPN_FIST
#endif
}
#endif

int SpawnShrapX(short SpriteNum)
{
    //For shrap that has no Weapon to send over
    SpawnShrap(SpriteNum, -1);
    return 0;
}

int DoLavaErupt(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    short i,nexti,pnum;
    PLAYERp pp;
    SPRITEp tsp;
    SWBOOL found = FALSE;

    if (TEST_BOOL1(sp))
    {
        TRAVERSE_CONNECT(pnum)
        {
            pp = Player + pnum;
            if (TEST(sector[pp->cursectnum].extra, SECTFX_TRIGGER))
            {
                TRAVERSE_SPRITE_SECT(headspritesect[pp->cursectnum],i,nexti)
                {
                    tsp = &sprite[i];

                    if (tsp->statnum == STAT_TRIGGER && SP_TAG7(tsp) == 0 && SP_TAG5(tsp) == 1)
                    {
                        found = TRUE;
                        break;
                    }
                }
                if (found)
                    break;
            }
        }

        if (!found)
            return 0;
    }

    if (!TEST(u->Flags, SPR_ACTIVE))
    {
        // inactive
        if ((u->WaitTics -= synctics) <= 0)
        {
            SET(u->Flags, SPR_ACTIVE);
            u->Counter = 0;
            u->WaitTics = SP_TAG9(sp) * 120L;
        }
    }
    else
    {
        // active
        if ((u->WaitTics -= synctics) <= 0)
        {
            // Stop for this long
            RESET(u->Flags, SPR_ACTIVE);
            u->Counter = 0;
            u->WaitTics = SP_TAG10(sp) * 120L;
        }

        // Counter controls the volume of lava erupting
        // starts out slow and increases to a max
        u->Counter += synctics;
        if (u->Counter > SP_TAG2(sp))
            u->Counter = SP_TAG2(sp);

        if ((RANDOM_P2(1024<<6)>>6) < u->Counter)
        {
            switch (SP_TAG3(sp))
            {
            case 0:
                SpawnShrapX(SpriteNum);
                break;
            case 1:
                InitVulcanBoulder(SpriteNum);
                break;
            }
        }
    }

    return 0;
}


void UserShrapSetup(SHRAPp shrap, STATEp state, int num_shrap)
{
    shrap->state = state;
    shrap->num = num_shrap;
}

#if 0
STATEp UserStateSetup(short base_tile, short num_tiles)
{
    STATEp base_state, stp;
    short tile;

#define USER_PIC 0
#define USER_RATE 12

    base_state = CallocMem(sizeof(STATE) * num_tiles, 1);

    for (stp = base_state, tile = 0; stp->state || tile < num_tiles; stp++, tile++)
    {
        stp->Pic = base_tile + tile;
        stp->Tics = USER_RATE;
        stp->Animator = DoShrapJumpFall;

        // if last tile
        if (tile == num_tiles - 1)
            stp->NextState = base_state;
        else
            stp->NextState = stp + 1;
    }

    return base_state;
}
#endif

int
SpawnShrap(short ParentNum, short Secondary)
{
    SPRITEp parent = &sprite[ParentNum];
    SPRITEp sp;
    USERp u, pu = User[ParentNum];
    short SpriteNum;
    short i;

    /////////////////////////////////////////
    //
    // BREAK shrap types
    //
    /////////////////////////////////////////

    // Individual shraps can be copied to this and then values can be changed
    static SHRAP CustomShrap[20];

    static SHRAP CoinShrap[] =
    {
        {s_CoinShrap, COIN_SHRAP,      5, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_MetalShrapA, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_MetalShrapB, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_MetalShrapC, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP GlassShrap[] =
    {
        {s_GlassShrapA, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_GlassShrapB, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_GlassShrapC, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP WoodShrap[] =
    {
        {s_WoodShrapA, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_WoodShrapB, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_WoodShrapC, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP StoneShrap[] =
    {
        {s_StoneShrapA, STONE_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_StoneShrapB, STONE_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_StoneShrapC, STONE_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP PaperShrap[] =
    {
        {s_PaperShrapA, PAPER_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_PaperShrapB, PAPER_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_PaperShrapC, PAPER_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP MetalShrap[] =
    {
        {s_MetalShrapA, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_MetalShrapB, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_MetalShrapC, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP MetalMix[] =
    {
        {s_GlassShrapA, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_GlassShrapB, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_GlassShrapC, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_MetalShrapA, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_MetalShrapB, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_MetalShrapC, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP WoodMix[] =
    {
        {s_WoodShrapA, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_WoodShrapB, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_WoodShrapC, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_MetalShrapA, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_MetalShrapB, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_MetalShrapC, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP PaperMix[] =
    {
        {s_WoodShrapA, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_WoodShrapB, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_WoodShrapC, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_PaperShrapA, PAPER_SHRAP_A, 2, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_PaperShrapB, PAPER_SHRAP_A, 2, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_PaperShrapC, PAPER_SHRAP_A, 2, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP Marbels[] =
    {
        {s_Marbel,      MARBEL,        5, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_GlassShrapA, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_GlassShrapB, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {s_GlassShrapC, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

#if 0
    static SHRAP BloodShrap[] =
    {
        {s_BloodShrap, BLOOD_SHRAP, 8, Z_MID, 200, 600, 100, 500, TRUE, 2048},
        {NULL},
    };
#endif

    ////
    // END - BREAK shrap types
    ////

    static SHRAP EMPShrap[] =
    {
        {s_EMPShrap, EMP, 1, Z_MID, 500, 1100, 300, 600, FALSE, 128},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP StdShrap[] =
    {
        {s_GoreHead, GORE_Head, 1, Z_TOP, 400, 700, 20, 40, TRUE, 2048},
        {s_GoreLung, GORE_Lung, 2, Z_TOP, 500, 800, 100, 300, TRUE, 2048},
        {s_GoreLiver, GORE_Liver, 1, Z_MID, 300, 500, 100, 150, TRUE, 2048},
        {s_GoreArm, GORE_Arm, 1, Z_MID, 300, 500, 250, 500, TRUE, 2048},
        {s_GoreSkullCap, GORE_SkullCap, 1, Z_TOP, 300, 500, 250, 500, TRUE, 2048},
        {s_FastGoreDrip, GORE_Drip, 8, Z_BOT, 600, 800, 50, 70, FALSE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP HeartAttackShrap[] = // fewer gibs because of the plasma fountain sprites
    {
        {s_GoreLung,     GORE_Lung,       2, Z_TOP, 500, 1100, 300, 600, TRUE, 2048},
        {s_GoreLiver,   GORE_Liver,     1, Z_MID, 500, 1100, 300, 500, TRUE, 2048},
        {s_GoreArm,     GORE_Arm,       2, Z_MID, 500, 1100, 350, 600, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP SkelGore[] =
    {
        {s_GoreHead, GORE_Head, 1, Z_TOP, 400, 700, 20, 40, TRUE, 2048},
        {s_GoreLung, GORE_Lung, 2, Z_TOP, 500, 800, 100, 300, TRUE, 2048},
        {s_GoreLiver, GORE_Liver, 1, Z_MID, 300, 500, 100, 150, TRUE, 2048},
        {s_GoreSkullCap, GORE_SkullCap, 1, Z_TOP, 300, 500, 100, 150, TRUE, 2048},
        {s_GoreArm, GORE_Arm, 1, Z_MID, 300, 500, 250, 500, TRUE, 2048},
        {s_GoreLeg, GORE_Leg, 2, Z_BOT, 200, 400, 250, 500, TRUE, 2048},
        {s_GoreChunkS, GORE_ChunkS, 4, Z_BOT, 200, 400, 250, 400, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP UpperGore[] =
    {
        {s_GoreHead, GORE_Head, 1, Z_TOP, 400, 700, 20, 40, TRUE, 2048},
        {s_GoreLung, GORE_Lung, 2, Z_TOP, 500, 800, 100, 300, TRUE, 2048},
        {s_GoreLiver, GORE_Liver, 1, Z_MID, 300, 500, 100, 150, TRUE, 2048},
        {s_GoreSkullCap, GORE_SkullCap, 1, Z_TOP, 300, 500, 100, 150, TRUE, 2048},
        {s_GoreArm, GORE_Arm, 1, Z_MID, 300, 500, 250, 500, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP LowerGore[] =
    {
        {s_GoreLeg, GORE_Leg, 4, Z_BOT, 300, 500, 100, 200, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP SmallGore[] =
    {
        {s_GoreDrip, GORE_Drip, 3, Z_TOP, 600, 800, 50, 70, FALSE, 2048},
        {s_FastGoreDrip, GORE_Drip, 3, Z_BOT, 600, 800, 70, 100, FALSE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP FlamingGore[] =
    {
        {s_GoreFlame, GORE_Drip, 2, Z_TOP, 600, 800, 100, 200, FALSE, 2048},
        {s_GoreFlameChunkB, GORE_Drip, 4, Z_MID, 300, 500, 100, 200, FALSE, 2048},
        {s_GoreFlame, GORE_Drip, 2, Z_BOT, 100, 200, 100, 200, FALSE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP BoltExpShrap[] =
    {
        {s_GoreFlame, GORE_Drip, 4, Z_MID, 300, 700, 300, 600, TRUE, 2048},
        {s_GoreFlame, GORE_Drip, 4, Z_BOT, 300, 700, 300, 600, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP TracerExpShrap[] =
    {
        {s_TracerShrap, GORE_Drip, 3, Z_MID, 300, 700, 300, 600, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP FireballExpShrap1[] =
    {
        {s_GoreFlame, GORE_Drip, 1, Z_MID, 100, 300, 100, 200, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP FireballExpShrap2[] =
    {
        {s_GoreFlame, GORE_Drip, 2, Z_MID, 100, 300, 100, 200, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAPp FireballExpShrap[] =
    {
        FireballExpShrap1,
        FireballExpShrap2
    };

    // state, id, num, zlevel, min_jspeed, max_jspeed, min_vel, max_vel,
    // random_disperse, ang_range;
    static SHRAP ElectroShrap[] =
    {
        {s_ElectroShrap, ELECTRO_SHARD, 12, Z_TOP, 200, 600, 100, 500, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    // state, id, num, zlevel, min_jspeed, max_jspeed, min_vel, max_vel,
    // random_disperse, ang_range;
    static SHRAP LavaShrap1[] =
    {
        {s_GoreFlame, GORE_Drip, 1, Z_TOP, 400, 1400, 100, 400, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP LavaShrap2[] =
    {
        {s_GoreFlameChunkB, GORE_Drip, 1, Z_TOP, 400, 1400, 100, 400, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP *LavaShrapTable[] =
    {
        LavaShrap1,
        LavaShrap2
    };

    static SHRAP LavaBoulderShrap[] =
    {
        {s_LavaShard, LAVA_SHARD, 16, Z_MID, 400, 900, 200, 600, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP SectorSquishGore[] =
    {
        {s_FastGoreDrip,    GORE_Drip,   24, Z_MID, -400, -200, 600, 800, FALSE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };



    //
    // PLAYER SHRAP
    //

    // state, id, num, zlevel, min_jspeed, max_jspeed, min_vel, max_vel,
    // random_disperse, ang_range;
    static SHRAP PlayerGoreFall[] =
    {
        {s_GoreSkullCap,GORE_SkullCap,  1, Z_TOP, 200, 300, 100, 200, TRUE, 2048},
        {s_GoreLiver,   GORE_Liver,     1, Z_MID, 200, 300, 100, 200, TRUE, 2048},
        {s_GoreLung,   GORE_Lung,       1, Z_MID, 200, 300, 100, 200, TRUE, 2048},
        {s_GoreDrip,    GORE_Drip,      10, Z_MID, 200, 300, 100, 200, FALSE, 2048},
        {s_GoreArm,     GORE_Arm,       1, Z_MID, 200, 300, 100, 200, TRUE, 2048},
        {s_FastGoreDrip,    GORE_Drip,      10, Z_BOT, 200, 300, 100, 200, FALSE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP PlayerGoreFly[] =
    {
        {s_GoreSkullCap,GORE_SkullCap,  1, Z_TOP, 500, 1100, 300, 600, TRUE, 2048},
        {s_GoreTorso,   GORE_Torso,     1, Z_MID, 500, 1100, 300, 500, TRUE, 2048},
        {s_GoreLiver,   GORE_Liver,     1, Z_MID, 200, 300, 100, 200, TRUE, 2048},
        {s_GoreArm,     GORE_Arm,       1, Z_MID, 500, 1100, 350, 600, TRUE, 2048},
        {s_FastGoreDrip,    GORE_Drip,      16, Z_MID, 500, 1100, 350, 600, FALSE, 2048},
        {s_FastGoreDrip,    GORE_Drip,      16, Z_BOT, 500, 1100, 350, 600, FALSE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP PlayerDeadHead[] =
    {
        {s_GoreDrip, GORE_Drip, 2, Z_TOP, 150, 400, 40, 80, TRUE, 2048},
        {s_GoreDrip, GORE_Drip, 2, Z_MID, 150, 400, 40, 80, TRUE, 2048},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    // state, id, num, zlevel, min_jspeed, max_jspeed, min_vel, max_vel,
    // random_disperse, ang_range;
    static SHRAP PlayerHeadHurl1[] =
    {
        {s_Vomit1, Vomit1, 1, Z_BOT, 250, 400, 100, 200, TRUE, 256},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

#define WALL_FLOOR_SHRAP 4097
    ANIMATOR DoShrapWallFloor;
    static SHRAP SectorExpShrap[] =
    {
        {NULL, WALL_FLOOR_SHRAP, 1, Z_BOT, 550, 800, 200, 400, TRUE, 512},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    int hz[3];
    short dang = 0;

    //SHRAPp p = StdShrap;
    SHRAPp p = SmallGore;
    short shrap_shade = -15;
    short shrap_xsize = 48, shrap_ysize = 48;
    short retval = TRUE;
    short shrap_pal = PALETTE_DEFAULT;
    int shrap_floor_dist = Z(2);
    int shrap_ceiling_dist = Z(2);
    int shrap_z = 0;
    int nx,ny;
    short jump_grav = ACTOR_GRAVITY;
    short start_ang = 0;
    short shrap_owner = -1;
    int shrap_bounce = FALSE;
    short WaitTics = 64; // for FastShrap
    short shrap_type;
    int shrap_rand_zamt = 0;
    short shrap_ang = parent->ang;
    short shrap_delta_size = 0;
    short shrap_amt = 0;
    extern BREAK_INFOp GlobBreakInfo;

    if (Prediction)
        return 0;

    // Don't spawn shrapnel in invalid sectors gosh dern it!
    if (parent->sectnum < 0 || parent->sectnum >= MAXSECTORS)
    {
        //DSPRINTF(ds,"SpawnShrap: Invalid sector %d, picnum=%d\n",parent->sectnum,parent->picnum);
        MONO_PRINT(ds);
        return 0;
    }

    if (GlobBreakInfo)
    {
        shrap_type = GlobBreakInfo->shrap_type;
        shrap_amt = GlobBreakInfo->shrap_amt;
        GlobBreakInfo = NULL;
        goto AutoShrap;
    }
    else if (TEST(parent->extra, SPRX_BREAKABLE))
    {
        // if no user
        if (!User[parent - sprite])
        {
            // Jump to shrap type
            shrap_type = SP_TAG8(parent);
            goto UserShrap;
        }
        else
        {
            // has a user - is programmed
            change_sprite_stat(parent - sprite, STAT_MISC);
            RESET(parent->extra, SPRX_BREAKABLE);
            RESET(parent->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        }
    }

    ASSERT(pu);

    switch (pu->ID)
    {
    case ST1:
        switch (parent->hitag)
        {
        case SPAWN_SPOT:
        {
            if (pu->LastDamage)
                shrap_type = SP_TAG3(parent);
            else
                shrap_type = SP_TAG6(parent);

UserShrap:

            shrap_delta_size = (signed char)SP_TAG10(parent);
            shrap_rand_zamt = SP_TAG9(parent);
            // Hey, better limit this in case mappers go crazy, like I did. :)
            // Kills frame rate!
            shrap_amt = SP_TAG8(parent);
            if (shrap_amt > 5)
                shrap_amt = 5;

AutoShrap:

            switch (shrap_type)
            {
            case SHRAP_NONE:
                return FALSE;

            case SHRAP_GLASS:
                PlaySound(DIGI_BREAKGLASS,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
                p = GlassShrap;
                //BreakShrapSetup(GlassShrap, sizeof(GlassShrap), CustomShrap, shrap_amt);
                if (shrap_amt)
                {
                    memcpy(CustomShrap, GlassShrap, sizeof(GlassShrap));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = TRUE;
                break;

            case SHRAP_GENERIC:
            case SHRAP_STONE:
                PlaySound(DIGI_BREAKSTONES,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
                p = StoneShrap;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, StoneShrap, sizeof(StoneShrap));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 8 + shrap_delta_size;
                shrap_bounce = TRUE;
                break;

            case SHRAP_WOOD:
                PlaySound(DIGI_BREAKINGWOOD,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
                p = WoodShrap;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, WoodShrap, sizeof(WoodShrap));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = TRUE;
                break;

            case SHRAP_BLOOD:
                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                break;

            case SHRAP_GIBS:
                PlaySound(DIGI_GIBS1,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
                p = SmallGore;
                shrap_xsize = shrap_ysize = 34;
                shrap_bounce = FALSE;
                break;

            case SHRAP_TREE_BARK:
                PlaySound(DIGI_BREAKINGWOOD,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
                p = WoodShrap;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, WoodShrap, sizeof(WoodShrap));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = TRUE;
                break;

            case SHRAP_PAPER:
                p = PaperShrap;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, PaperShrap, sizeof(PaperShrap));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                break;

            case SHRAP_METAL:
                PlaySound(DIGI_BREAKMETAL,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
                p = MetalShrap;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, MetalShrap, sizeof(MetalShrap));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = TRUE;
                break;


            case SHRAP_COIN:
                PlaySound(DIGI_COINS,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
                p = CoinShrap;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, CoinShrap, sizeof(CoinShrap));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = TRUE;
                break;

            case SHRAP_METALMIX:
                PlaySound(DIGI_BREAKMETAL,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
                p = MetalMix;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, MetalMix, sizeof(MetalMix));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = TRUE;
                break;

            case SHRAP_MARBELS:
            {
#if 0
                short spnum;
                short size;
                SPRITEp ep;
                USERp eu;
                int SpawnLittleExp(int16_t Weapon);

                spnum = SpawnLittleExp(ParentNum);
                ASSERT(spnum >= 0);
                ep = &sprite[spnum];
                eu = User[spnum];

                //eu->xchange = MOVEx(92, ep->ang);
                //eu->ychange = MOVEy(92, ep->ang);

                size = ep->xrepeat/2;
                ep->xrepeat = ep->yrepeat = size + shrap_delta_size;
#endif

                //PlaySound(DIGI_BREAKMARBELS,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
                p = Marbels;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, Marbels, sizeof(Marbels));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 10 + shrap_delta_size;
                shrap_bounce = TRUE;
            }
            break;

            case SHRAP_WOODMIX:
                PlaySound(DIGI_BREAKINGWOOD,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
                p = WoodMix;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, WoodMix, sizeof(WoodMix));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = TRUE;
                break;

            case SHRAP_PAPERMIX:
                PlaySound(DIGI_BREAKINGWOOD,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
                p = PaperMix;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, PaperMix, sizeof(PaperMix));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = FALSE;
                break;

            case SHRAP_SO_SMOKE:
                return FALSE;

            case SHRAP_EXPLOSION:
            {
                short spnum;
                short size;
                SPRITEp ep;
                USERp eu;

                spnum = SpawnLargeExp(ParentNum);
                ASSERT(spnum >= 0);
                //spnum = SpawnSectorExp(ParentNum);
                ep = &sprite[spnum];
                eu = User[spnum];

                //eu->xchange = MOVEx(92, ep->ang);
                //eu->ychange = MOVEy(92, ep->ang);

                size = ep->xrepeat;
                ep->xrepeat = ep->yrepeat = size + shrap_delta_size;

                return FALSE;
            }

            case SHRAP_LARGE_EXPLOSION:
            {
                short spnum;
                short size;
                SPRITEp ep;
                USERp eu;

                //spnum = SpawnSectorExp(ParentNum);
                spnum = SpawnLargeExp(ParentNum);
                ASSERT(spnum >= 0);
                ep = &sprite[spnum];
                eu = User[spnum];

                //eu->xchange = MOVEx(92, ep->ang);
                //eu->ychange = MOVEy(92, ep->ang);

                size = ep->xrepeat;
                ep->xrepeat = ep->yrepeat = size + shrap_delta_size;

                InitPhosphorus(spnum);

                return FALSE;
            }

            default:
            {
                return FALSE;
            }
            }
            break;
        }

        default:
            p = LavaShrapTable[RANDOM_P2(2<<8)>>8];
        }
        break;

    case BREAK_BARREL:
        PlaySound(DIGI_BREAKDEBRIS,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
        p = WoodShrap;
        shrap_xsize = shrap_ysize = 24;
        shrap_bounce = TRUE;
        ChangeState(parent - sprite, s_BreakBarrel);
        break;
    case BREAK_LIGHT:
        PlaySound(DIGI_BREAKGLASS,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
        p = GlassShrap;
        shrap_xsize = shrap_ysize = 24;
        shrap_bounce = TRUE;
        ChangeState(parent - sprite, s_BreakLight);
        break;
    case BREAK_PEDISTAL:
        PlaySound(DIGI_BREAKSTONES,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
        p = StoneShrap;
        shrap_xsize = shrap_ysize = 24;
        shrap_bounce = TRUE;
        ChangeState(parent - sprite, s_BreakPedistal);
        break;
    case BREAK_BOTTLE1:
        PlaySound(DIGI_BREAKGLASS,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
        p = GlassShrap;
        shrap_xsize = shrap_ysize = 8;
        shrap_bounce = TRUE;
        ChangeState(parent - sprite, s_BreakBottle1);
        break;
    case BREAK_BOTTLE2:
        PlaySound(DIGI_BREAKGLASS,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
        p = GlassShrap;
        shrap_xsize = shrap_ysize = 8;
        shrap_bounce = TRUE;
        ChangeState(parent - sprite, s_BreakBottle2);
        break;
    case BREAK_MUSHROOM:
        PlaySound(DIGI_BREAKDEBRIS,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
        p = StoneShrap;
        shrap_xsize = shrap_ysize = 4;
        shrap_bounce = TRUE;
        SetSuicide(parent - sprite); // kill next iteration
        break;
    case BOLT_EXP:
        return FALSE;
//        p = BoltExpShrap;
//        break;
    case TANK_SHELL_EXP:
        return FALSE;
//        p = BoltExpShrap;
//        break;
    case TRACER_EXP:
        return FALSE;
//        p = TracerExpShrap;
//        shrap_xsize = shrap_ysize = 20;
//        WaitTics = 10;
//        break;
    case BOLT_THINMAN_R1:
        p = MetalShrap;
        if (shrap_amt)
        {
            memcpy(CustomShrap, MetalShrap, sizeof(MetalShrap));
            CustomShrap->num = 1;
            p = CustomShrap;
        }

        shrap_xsize = shrap_ysize = 10;
        break;
    case LAVA_BOULDER:
        PlaySound(DIGI_BREAKSTONES,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
        p = LavaBoulderShrap;
        shrap_owner = parent->owner;
        shrap_xsize = shrap_ysize = 24;
        shrap_bounce = TRUE;
        break;
    case SECTOR_EXP:
        //p = SectorExpShrap;
        //break;
        return FALSE;
    case GRENADE_EXP:
        //p = SectorExpShrap;
        //break;
        return FALSE;
    case FIREBALL_EXP:
        return FALSE;
//        p = FireballExpShrap[RANDOM_P2(2<<8)>>8];
//        shrap_pal = pu->spal;
        break;
    case ELECTRO_PLAYER:
    case ELECTRO_ENEMY:
        shrap_owner = parent->owner;
        p = ElectroShrap;
        shrap_xsize = shrap_ysize = 20;
        break;
    case COOLIE_RUN_R0:
        if (Secondary == WPN_NM_SECTOR_SQUISH)
            break;
//        return (FALSE);
        break;
    case NINJA_DEAD:
        return FALSE;
        break;
    case NINJA_Head_R0:
    {
        extern STATEp sg_PlayerHeadHurl[];

        if (pu->Rot == sg_PlayerHeadHurl)
        {
            p = PlayerHeadHurl1;
        }
        else
        {
            p = PlayerDeadHead;
            shrap_xsize = shrap_ysize = 16+8;
            shrap_bounce = TRUE;
        }
        break;
    }
    case GIRLNINJA_RUN_R0:
        p = StdShrap;
        break;
    case NINJA_RUN_R0:
    {
        p = StdShrap;
        if (pu->PlayerP)
        {
            PLAYERp pp = pu->PlayerP;

            if (pp->DeathType == PLAYER_DEATH_CRUMBLE)
                p = PlayerGoreFall;
            else
                p = PlayerGoreFly;
        }
        break;
    }
    case GORO_RUN_R0:
        p = StdShrap;
        shrap_xsize = shrap_ysize = 64;
        break;
    case COOLG_RUN_R0:
        p = UpperGore;
        break;
    case RIPPER_RUN_R0:
        p = StdShrap;
        if (pu->spal != 0)
            shrap_xsize = shrap_ysize = 64;
        else
            shrap_xsize = shrap_ysize = 32;
        break;
    case RIPPER2_RUN_R0:
        p = StdShrap;
        if (pu->spal != 0)
            shrap_xsize = shrap_ysize = 64;
        else
            shrap_xsize = shrap_ysize = 32;
        break;
    case SERP_RUN_R0:
        p = StdShrap;
        //return (FALSE);
        break;
    case SUMO_RUN_R0:
        p = StdShrap;
        break;
    case SKEL_RUN_R0:
        p = SkelGore;
        shrap_pal = PALETTE_SKEL_GORE;
        break;
    case HORNET_RUN_R0:
        p = SmallGore;
        shrap_pal = PALETTE_SKEL_GORE;
        break;
    case SKULL_R0:
    case SKULL_R0 + 1:
        p = FlamingGore;
        break;
    case SKULL_SERP:
        return FALSE;
    case BETTY_R0:
    case TRASHCAN:
    case PACHINKO1:
    case PACHINKO2:
    case PACHINKO3:
    case PACHINKO4:
    case 623:
        PlaySound(DIGI_BREAKGLASS,&parent->x,&parent->y,&parent->z,v3df_dontpan|v3df_doppler);
        p = MetalShrap;
        shrap_xsize = shrap_ysize = 10;
        break;
    case ZILLA_RUN_R0:
        p = MetalShrap;
        shrap_xsize = shrap_ysize = 10;
        break;
    case EMP:
        p = EMPShrap;
        shrap_xsize = shrap_ysize = 8;
        shrap_bounce = FALSE;
        break;
    }

    // second sprite involved
    // most of the time is is the weapon
    if (Secondary >= 0)
    {
        SPRITEp wp = &sprite[Secondary];
        USERp wu = User[Secondary];

        if (wu->PlayerP && wu->PlayerP->sop_control)
        {
            p = StdShrap;
        }
        else
            switch (wu->ID)
            {
            case PLASMA_FOUNTAIN:
                p = HeartAttackShrap;
                break;
            }
    }

    hz[Z_TOP] = SPRITEp_TOS(parent);        // top
    hz[Z_BOT] = SPRITEp_BOS(parent);        // bottom
    hz[Z_MID] = DIV2(hz[0] + hz[2]);        // mid

    for (; p->id; p++)
    {
        if (!p->random_disperse)
        {
            //dang = (2048 / p->num);
            start_ang = NORM_ANGLE(shrap_ang - DIV2(p->ang_range));
            dang = (p->ang_range / p->num);
        }

        for (i = 0; i < p->num; i++)
        {
            SpriteNum = SpawnSprite(STAT_SKIP4, p->id, p->state, parent->sectnum,
                                    parent->x, parent->y, hz[p->zlevel], shrap_ang, 512);

            sp = &sprite[SpriteNum];
            u = User[SpriteNum];

            if (p->random_disperse)
            {
                sp->ang = shrap_ang + (RANDOM_P2(p->ang_range<<5)>>5) - DIV2(p->ang_range);
                sp->ang = NORM_ANGLE(sp->ang);
            }
            else
            {
                sp->ang = start_ang + (i * dang);
                sp->ang = NORM_ANGLE(sp->ang);
            }

            // for FastShrap
            u->zchange = labs(u->jump_speed*4) - RANDOM_RANGE(labs(u->jump_speed)*8)*2;
            u->WaitTics = WaitTics + RANDOM_RANGE(WaitTics/2);

            switch (u->ID)
            {
            case GORE_Drip:
                shrap_bounce = FALSE;
                break;
            case GORE_Lung:
                shrap_xsize = 20;
                shrap_ysize = 20;
                shrap_bounce = FALSE;
                break;
            case GORE_Liver:
                shrap_xsize = 20;
                shrap_ysize = 20;
                shrap_bounce = FALSE;
                break;
            case GORE_SkullCap:
                shrap_xsize = 24;
                shrap_ysize = 24;
                shrap_bounce = TRUE;
                break;
            case GORE_Arm:
                shrap_xsize = 21;
                shrap_ysize = 21;
                shrap_bounce = FALSE;
                break;
            case GORE_Head:
                shrap_xsize = 26;
                shrap_ysize = 30;
                shrap_bounce = TRUE;
                break;
            case Vomit1:
                shrap_bounce = FALSE;
                sp->z -= Z(4);
                shrap_xsize = u->sx = 12 + (RANDOM_P2(32<<8)>>8);
                shrap_ysize = u->sy = 12 + (RANDOM_P2(32<<8)>>8);
                u->Counter = (RANDOM_P2(2048<<5)>>5);

                nx = sintable[NORM_ANGLE(sp->ang+512)]>>6;
                ny = sintable[sp->ang]>>6;
                move_missile(SpriteNum, nx, ny, 0, Z(8), Z(8), CLIPMASK_MISSILE, MISSILEMOVETICS);

                if (RANDOM_P2(1024)<700)
                    u->ID = 0;

                break;
            case EMP:
                shrap_bounce = FALSE;
                sp->z -= Z(4);
                //sp->ang = NORM_ANGLE(sp->ang + 1024);
                shrap_xsize = u->sx = 5 + (RANDOM_P2(4<<8)>>8);
                shrap_ysize = u->sy = 5 + (RANDOM_P2(4<<8)>>8);
                break;
            }

            sp->shade = shrap_shade;
            sp->xrepeat = shrap_xsize;
            sp->yrepeat = shrap_ysize;
            sp->clipdist = 16L >> 2;

            if (shrap_owner >= 0)
            {
                SetOwner(shrap_owner, SpriteNum);
            }

            if (shrap_rand_zamt)
            {
                sp->z += Z(RANDOM_RANGE(shrap_rand_zamt) - (shrap_rand_zamt/2));
            }

            sp->pal = u->spal = shrap_pal;

            sp->xvel = p->min_vel*2;
            sp->xvel += RANDOM_RANGE(p->max_vel - p->min_vel);

            u->floor_dist = shrap_floor_dist;
            u->ceiling_dist = shrap_ceiling_dist;
            u->jump_speed = p->min_jspeed;
            u->jump_speed += RANDOM_RANGE(p->max_jspeed - p->min_jspeed);
            u->jump_speed = -u->jump_speed;

            DoBeginJump(SpriteNum);
            u->jump_grav = jump_grav;

            u->xchange = MOVEx(sp->xvel, sp->ang);
            u->ychange = MOVEy(sp->xvel, sp->ang);

            if (!shrap_bounce)
                SET(u->Flags, SPR_BOUNCE);
        }
    }

    return retval;
}

int
DoShrapMove(int16_t SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    u->ret = move_missile(SpriteNum, u->xchange, u->ychange, 0, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS*2);

    return 0;
}

#if 1
int
DoVomit(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    u->Counter = NORM_ANGLE(u->Counter + (30*MISSILEMOVETICS));
    sp->xrepeat = u->sx + ((12 * sintable[NORM_ANGLE(u->Counter+512)]) >> 14);
    sp->yrepeat = u->sy + ((12 * sintable[u->Counter]) >> 14);

    if (TEST(u->Flags, SPR_JUMPING))
    {
        DoJump(SpriteNum);
        DoJump(SpriteNum);
        DoShrapMove(SpriteNum);
    }
    else if (TEST(u->Flags, SPR_FALLING))
    {
        DoFall(SpriteNum);
        DoFall(SpriteNum);
        DoShrapMove(SpriteNum);
    }
    else
    {
        ChangeState(SpriteNum, s_VomitSplash);
        DoFindGroundPoint(SpriteNum);
        MissileWaterAdjust(SpriteNum);
        sp->z = u->loz;
        u->WaitTics = 60;
        u->sx = sp->xrepeat;
        u->sy = sp->yrepeat;
        return 0;
    }

    if (TEST(u->ret, HIT_MASK) == HIT_SPRITE)
    {
        short hit_sprite = NORM_SPRITE(u->ret);
        if (TEST(sprite[hit_sprite].extra, SPRX_PLAYER_OR_ENEMY))
        {
            DoDamage(hit_sprite, SpriteNum);
            //KillSprite(SpriteNum);
            return 0;
        }
    }

    //if (u->ID)
    //    DoDamageTest(SpriteNum);

    return 0;
}
#else
int
DoVomit(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    u->Counter = NORM_ANGLE(u->Counter + (30*MISSILEMOVETICS));
    sp->xrepeat = u->sx + ((12 * sintable[NORM_ANGLE(u->Counter+512)]) >> 14);
    sp->yrepeat = u->sy + ((12 * sintable[u->Counter]) >> 14);

    if (TEST(u->Flags, SPR_JUMPING))
    {
        DoShrapVelocity(SpriteNum);
    }
    else if (TEST(u->Flags, SPR_FALLING))
    {
        DoShrapVelocity(SpriteNum);
    }
    else
    {
        ChangeState(SpriteNum, s_VomitSplash);
        DoFindGroundPoint(SpriteNum);
        MissileWaterAdjust(SpriteNum);
        sp->z = u->loz;
        u->WaitTics = 60;
        u->sx = sp->xrepeat;
        u->sy = sp->yrepeat;
        return 0;
    }

    if (TEST(u->ret, HIT_MASK) == HIT_SPRITE)
    {
        short hit_sprite = NORM_SPRITE(u->ret);
        if (TEST(sprite[hit_sprite].extra, SPRX_PLAYER_OR_ENEMY))
        {
            DoDamage(hit_sprite, SpriteNum);
            //KillSprite(SpriteNum);
            return 0;
        }
    }

    return 0;
}
#endif


int
DoVomitSplash(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if ((u->WaitTics-=MISSILEMOVETICS) < 0)
    {
        KillSprite(SpriteNum);
        return 0;
    }

    return 0;
}

int
DoFastShrapJumpFall(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    sp->x += u->xchange*2;
    sp->y += u->ychange*2;
    sp->z += u->zchange*2;

    u->WaitTics -= MISSILEMOVETICS;
    if (u->WaitTics <= 0)
        KillSprite(SpriteNum);

    return 0;
}

int
DoTracerShrap(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    sp->x += u->xchange;
    sp->y += u->ychange;
    sp->z += u->zchange;

    u->WaitTics -= MISSILEMOVETICS;
    if (u->WaitTics <= 0)
        KillSprite(SpriteNum);

    return 0;
}

//#define ShrapKillSprite(num) _Shrap_Kill_Sprite(num, __FILE__, __LINE__) //#define DoShrapVelocity(num) _Do_Shrap_Velocity(num, __FILE__, __LINE__)

int
DoShrapJumpFall(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    int16_t rnd_num;

    if (TEST(u->Flags, SPR_JUMPING))
    {
        DoShrapVelocity(SpriteNum);
    }
    else if (TEST(u->Flags, SPR_FALLING))
    {
        DoShrapVelocity(SpriteNum);
    }
    else
    {
        if (!TEST(u->Flags, SPR_BOUNCE))
        {
            DoShrapVelocity(SpriteNum);
            return 0;
        }

        if (u->ID == GORE_Drip)
            ChangeState(SpriteNum, s_GoreFloorSplash);
        else
            ShrapKillSprite(SpriteNum);
    }


    return 0;
}

int
DoShrapDamage(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (TEST(u->Flags, SPR_JUMPING))
    {
        DoJump(SpriteNum);
        DoShrapMove(SpriteNum);
    }
    else if (TEST(u->Flags, SPR_FALLING))
    {
        DoFall(SpriteNum);
        DoShrapMove(SpriteNum);
    }
    else
    {
        if (!TEST(u->Flags, SPR_BOUNCE))
        {
            SET(u->Flags, SPR_BOUNCE);
            u->jump_speed = -300;
            sp->xvel >>= 2;
            DoBeginJump(SpriteNum);
            return 0;
        }

        KillSprite(SpriteNum);
        return 0;
    }

    if (u->ret)
    {
        switch (TEST(u->ret, HIT_MASK))
        {
        case HIT_SPRITE:
        {
            WeaponMoveHit(SpriteNum);
            KillSprite(SpriteNum);
            return 0;
        }
        }
    }

    return 0;
}

int
SpawnBlood(short SpriteNum, short Weapon, short hit_ang, int hit_x, int hit_y, int hit_z)
{
    SPRITEp sp = &sprite[SpriteNum];
    SPRITEp np;
    USERp nu, u;
    short i,New;


    // state, id, num, zlevel, min_jspeed, max_jspeed, min_vel, max_vel,
    // random_disperse, ang_range;

    static SHRAP UziBlood[] =
    {
        {s_GoreDrip, GORE_Drip, 1, Z_MID, 100, 250, 10, 20, TRUE, 512},  // 70,200 vels
        //{s_GoreSplash, PLASMA_Drip, 1, Z_BOT, 0, 0, 0, 0, FALSE, 512},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP SmallBlood[] =
    {
        {s_GoreDrip, GORE_Drip, 1, Z_TOP, 100, 250, 10, 20, TRUE, 512},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP PlasmaFountainBlood[] =
    {
        {s_PlasmaDrip, PLASMA_Drip, 1, Z_TOP, 200, 500, 100, 300, TRUE, 16},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP SomeBlood[] =
    {
        {s_GoreDrip, GORE_Drip, 1, Z_TOP, 100, 250, 10, 20, TRUE, 512},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP MoreBlood[] =
    {
        {s_GoreDrip, GORE_Drip, 2, Z_TOP, 100, 250, 10, 20, TRUE, 512},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP ExtraBlood[] =
    {
        {s_GoreDrip, GORE_Drip, 4, Z_TOP, 100, 250, 10, 20, TRUE, 512},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP HariKariBlood[] =
    {
        {s_FastGoreDrip, GORE_Drip, 32, Z_TOP, 200, 650, 70, 100, TRUE, 1024},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP SwordPowerup[] =
    {
        {s_ElectroShrap, ELECTRO_SHARD, 16, Z_TOP, 75, 200, 70, 150, TRUE, 512},
        {NULL,0,0,0,0,0,0,0,0,0},
    };

    int hz;
    short dang = 0;

    SHRAPp p = UziBlood;
    short shrap_shade = -15;
    short shrap_xsize = 20, shrap_ysize = 20;
    short retval = TRUE;
    short shrap_pal = PALETTE_DEFAULT;
    int shrap_z = 0;
    short start_ang = 0;

    u = User[SpriteNum];

    switch (u->ID)
    {
    case TRASHCAN:
    case PACHINKO1:
    case PACHINKO2:
    case PACHINKO3:
    case PACHINKO4:
    case 623:
    case ZILLA_RUN_R0:
        return 0;   // Don't put blood on trashcan
    }

    // second sprite involved
    // most of the time is the weapon
    if (Weapon >= 0)
    {
        SPRITEp wp = &sprite[Weapon];
        USERp wu = User[Weapon];

        switch (wu->ID)
        {
        case NINJA_RUN_R0: //sword
            // if sprite and weapon are the same it must be HARIII-KARIII
            if (sp == wp)
            {
                p = HariKariBlood;
                hit_ang = sp->ang;
                hit_x = sp->x;
                hit_y = sp->y;
                hit_z = SPRITEp_TOS(wp) + DIV16(SPRITEp_SIZE_Z(wp));
            }
            else
            {
                p = ExtraBlood;
                hit_ang = NORM_ANGLE(wp->ang + 1024);
                hit_x = sp->x;
                hit_y = sp->y;
                hit_z = SPRITEp_TOS(wp) + DIV4(SPRITEp_SIZE_Z(wp));

                //ASSERT(wu->PlayerP);
            }
            break;
        case SERP_RUN_R0:
            p = ExtraBlood;
            hit_ang = NORM_ANGLE(wp->ang + 1024);
            hit_x = sp->x;
            hit_y = sp->y;
            hit_z = SPRITEp_TOS(sp) + DIV4(SPRITEp_SIZE_Z(sp));
            break;
        case BLADE1:
        case BLADE2:
        case BLADE3:
        case 5011:
            p = SmallBlood;
            hit_ang = NORM_ANGLE(ANG2SPRITE(sp, wp) + 1024);
            hit_x = sp->x;
            hit_y = sp->y;
            hit_z = wp->z - DIV2(SPRITEp_SIZE_Z(wp));
            break;
        case STAR1:
        case CROSSBOLT:
            p = SomeBlood;
            hit_ang = NORM_ANGLE(wp->ang + 1024);
            hit_x = sp->x;
            hit_y = sp->y;
            hit_z = wp->z;
            break;
        case PLASMA_FOUNTAIN:
            p = PlasmaFountainBlood;
            hit_ang = wp->ang;
            hit_x = sp->x;
            hit_y = sp->y;
            hit_z = SPRITEp_TOS(sp) + DIV4(SPRITEp_SIZE_Z(sp));
            break;
        default:
            p = SomeBlood;
            hit_ang = NORM_ANGLE(wp->ang + 1024);
            hit_x = sp->x;
            hit_y = sp->y;
            hit_z = SPRITEp_TOS(wp) + DIV4(SPRITEp_SIZE_Z(wp));
            break;
        }
    }
    else
    {
        hit_ang = NORM_ANGLE(hit_ang + 1024);
    }

    for (; p->state; p++)
    {
        if (!p->random_disperse)
        {
            start_ang = NORM_ANGLE(hit_ang - DIV2(p->ang_range)+1024);
            dang = (p->ang_range / p->num);
        }

        for (i = 0; i < p->num; i++)
        {
            New = SpawnSprite(STAT_SKIP4, p->id, p->state, sp->sectnum,
                              hit_x, hit_y, hit_z, hit_ang, 0);
            np = &sprite[New];
            nu = User[New];

            switch (nu->ID)
            {
            case ELECTRO_SHARD:
                shrap_xsize = 7;
                shrap_ysize = 7;
                break;

            case PLASMA_Drip:
                // Don't do central blood splats for every hitscan
                if (RANDOM_P2(1024) < 950)
                {
                    np->xrepeat = np->yrepeat = 0;
                }
                if (RANDOM_P2(1024) < 512)
                    SET(np->cstat, CSTAT_SPRITE_XFLIP);
                //shrap_xsize = 96;
                //shrap_ysize = 75;
                //shrap_xsize = 10;
                //shrap_ysize = 10;
                break;
            }

            if (p->random_disperse)
            {
                np->ang = hit_ang + (RANDOM_P2(p->ang_range<<5)>>5) - DIV2(p->ang_range);
                np->ang = NORM_ANGLE(np->ang);
            }
            else
            {
                np->ang = start_ang + (i * dang);
                np->ang = NORM_ANGLE(np->ang);
            }

            SET(nu->Flags, SPR_BOUNCE);

            np->shade = shrap_shade;
            np->xrepeat = shrap_xsize;
            np->yrepeat = shrap_ysize;
            np->clipdist = 16L >> 2;

            np->pal = nu->spal = shrap_pal;

            np->xvel = p->min_vel;
            np->xvel += RANDOM_RANGE(p->max_vel - p->min_vel);

            // special case
            // blood coming off of actors should have the acceleration of the actor
            // so add it in
            np->xvel += sp->xvel;

            nu->ceiling_dist = nu->floor_dist = Z(2);
            nu->jump_speed = p->min_jspeed;
            nu->jump_speed += RANDOM_RANGE(p->max_jspeed - p->min_jspeed);
            nu->jump_speed = -nu->jump_speed;

            //setspritez(New, (vec3_t *)np);
            nu->xchange = MOVEx(np->xvel, np->ang);
            nu->ychange = MOVEy(np->xvel, np->ang);

            // for FastShrap
            nu->zchange = labs(nu->jump_speed*4) - RANDOM_RANGE(labs(nu->jump_speed)*8);
            nu->WaitTics = 64 + RANDOM_P2(32);

            SET(u->Flags, SPR_BOUNCE);

            DoBeginJump(New);
        }
    }

    return retval;
}


SWBOOL
VehicleMoveHit(short SpriteNum)
{
    USERp u = User[SpriteNum],cu;
    SPRITEp sp = User[SpriteNum]->SpriteP,cp;
    SECTOR_OBJECTp sop;
    SECTOR_OBJECTp hsop;
    SWBOOL TestKillSectorObject(SECTOR_OBJECTp);
    short controller;

    if (!u->ret)
        return FALSE;

    sop = u->sop_parent;

    // sprite controlling sop
    cp = sop->controller;
    controller = cp - sprite;
    cu = User[controller];

    switch (TEST(u->ret, HIT_MASK))
    {
    case HIT_SECTOR:
    {
        short hit_sect = NORM_SECTOR(u->ret);
        SECTORp sectp = &sector[hit_sect];

        if (TEST(sectp->extra, SECTFX_SECTOR_OBJECT))
        {
            // shouldn't ever really happen
        }

        return TRUE;
    }

    case HIT_SPRITE:
    {
        short hit_sprite = NORM_SPRITE(u->ret);
        SPRITEp hsp = &sprite[hit_sprite];

        if (TEST(hsp->extra, SPRX_BREAKABLE))
        {
            HitBreakSprite(hit_sprite, u->ID);
            return TRUE;
        }

        if (TEST(hsp->extra, SPRX_PLAYER_OR_ENEMY))
        {
            if (hit_sprite != cp->owner)
            {
                DoDamage(hit_sprite, controller);
                return TRUE;
            }
        }
        else
        {
            if (hsp->statnum == STAT_MINE_STUCK)
            {
                DoDamage(hit_sprite, SpriteNum);
                return TRUE;
            }
        }

        return TRUE;
    }

    case HIT_WALL:
    {
        short hit_wall = NORM_WALL(u->ret);
        WALLp wph = &wall[hit_wall];

        if (TEST(wph->extra, WALLFX_SECTOR_OBJECT))
        {
            // sector object collision
            if ((hsop = DetectSectorObjectByWall(wph)))
            {
#if 1
                SopDamage(hsop, sop->ram_damage);
                if (hsop->max_damage <= 0)
                    SopCheckKill(hsop);
                else
                    DoSpawnSpotsForDamage(hsop->match_event);
#else
                // drivable damage is DIE_HARD type
                if (hsop->max_damage != -9999 && sop->ram_damage)
                {
                    hsop->max_damage -= sop->ram_damage;
                    if (hsop->max_damage <= 0)
                    {
                        TestKillSectorObject(hsop);
                    }
                    else
                    {
                        DoSpawnSpotsForDamage(hsop->match_event);
                    }
                }
#endif
            }
        }

        return TRUE;
    }
    }

    return FALSE;
}


SWBOOL
WeaponMoveHit(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;


    if (!u->ret)
        return FALSE;

    switch (TEST(u->ret, HIT_MASK))
    {
    case HIT_PLAX_WALL:
        SetSuicide(SpriteNum);
        return TRUE;

    case HIT_SECTOR:
    {
        short hit_sect;
        SECTORp sectp;
        SECTOR_OBJECTp sop;

        hit_sect = NORM_SECTOR(u->ret);
        sectp = &sector[hit_sect];

        ASSERT(sectp->extra != -1);

        // hit floor - closer to floor than ceiling
        if (sp->z > DIV2(u->hiz + u->loz))
        {
            // hit a floor sprite
            if (u->lo_sp)
            {

                if (u->lo_sp->lotag == TAG_SPRITE_HIT_MATCH)
                {
                    if (MissileHitMatch(SpriteNum, -1, u->lo_sp - sprite))
                        return TRUE;
                    //DoMatchEverything(NULL, u->lo_sp->hitag, -1);
                    //return(TRUE);
                }

                return TRUE;
            }

            if (SectUser[hit_sect] && SectUser[hit_sect]->depth > 0)
            {
                SpawnSplash(SpriteNum);
                //SetSuicide(SpriteNum);
                return TRUE;
            }

        }
        // hit ceiling
        else
        {
            // hit a floor sprite
            if (u->hi_sp)
            {
                if (u->hi_sp->lotag == TAG_SPRITE_HIT_MATCH)
                {
                    if (MissileHitMatch(SpriteNum, -1, u->hi_sp - sprite))
                        return TRUE;
                    //DoMatchEverything(NULL, u->hi_sp->hitag, -1);
                    //return(TRUE);
                }
            }
        }


        if (TEST(sectp->extra, SECTFX_SECTOR_OBJECT))
        {
            if ((sop = DetectSectorObject(sectp)))
            {
                //if (sop->max_damage != -9999)
                DoDamage(sop->sp_child - sprite, SpriteNum);
                return TRUE;
            }
        }

        if (TEST(sectp->ceilingstat, CEILING_STAT_PLAX) && sectp->ceilingpicnum != FAF_MIRROR_PIC)
        {
            if (labs(sp->z - sectp->ceilingz) < SPRITEp_SIZE_Z(sp))
            {
                SetSuicide(SpriteNum);
                return TRUE;
            }
        }

        return TRUE;
    }

    case HIT_SPRITE:
    {
        SPRITEp hsp;
        USERp hu;
        short hit_sprite;

        hit_sprite = NORM_SPRITE(u->ret);
        hsp = &sprite[hit_sprite];
        hu = User[hit_sprite];

        if (hsp->statnum == STAT_ENEMY)
        {
            ////DSPRINTF(ds, "Bullet Hit Monster %d, stat %d ", hsp-sprite, (short)hsp->statnum);
            //MONO_PRINT(ds);
        }

        ASSERT(hsp->extra != -1);

        if (TEST(hsp->extra, SPRX_BREAKABLE))
        {
            HitBreakSprite(hit_sprite, u->ID);
            return TRUE;
        }

        if (TEST(hsp->extra, SPRX_PLAYER_OR_ENEMY))
        {
            // make sure you didn't hit the owner of the missile
            //if (sp->owner != -1 && hit_sprite != sp->owner)
            if (hit_sprite != sp->owner)
            {
                if (u->ID == STAR1)
                {
                    extern STATE s_TrashCanPain[];
                    switch (hu->ID)
                    {
                    case TRASHCAN:
                        PlaySound(DIGI_TRASHLID, &sp->x, &sp->y, &sp->z, v3df_none);
                        PlaySound(DIGI_STARCLINK, &sp->x, &sp->y, &sp->z, v3df_none);
                        if (hu->WaitTics <= 0)
                        {
                            hu->WaitTics = SEC(2);
                            ChangeState(hit_sprite,s_TrashCanPain);
                        }
                        break;
                    case PACHINKO1:
                    case PACHINKO2:
                    case PACHINKO3:
                    case PACHINKO4:
                    case ZILLA_RUN_R0:
                    case 623:
                    {
                        PlaySound(DIGI_STARCLINK, &sp->x, &sp->y, &sp->z, v3df_none);
                    }
                    break;
                    }
                }
                DoDamage(hit_sprite, SpriteNum);
                return TRUE;
            }
        }
        else
        {
            if (hsp->statnum == STAT_MINE_STUCK)
            {
                DoDamage(hit_sprite, SpriteNum);
                return TRUE;
            }
        }

        if (hsp->lotag == TAG_SPRITE_HIT_MATCH)
        {
            if (MissileHitMatch(SpriteNum, -1, hit_sprite))
                return TRUE;
            //DoMatchEverything(NULL, hsp->hitag, -1);
            //return(TRUE);
        }

        if (TEST(hsp->cstat, CSTAT_SPRITE_WALL))
        {
            if (hsp->lotag || hsp->hitag)
            {
                ShootableSwitch(hit_sprite, SpriteNum);
                return TRUE;
            }
        }

        return TRUE;
    }

    case HIT_WALL:
    {
        hitdata_t hitinfo = { { 0, 0, 0 }, -2, NORM_WALL(u->ret), -2 };
        WALLp wph = &wall[hitinfo.wall];
        SECTOR_OBJECTp sop;

        ASSERT(wph->extra != -1);

        if (TEST(wph->extra, WALLFX_SECTOR_OBJECT))
        {
            if ((sop = DetectSectorObjectByWall(wph)))
            {
                if (sop->max_damage != -999)
                    DoDamage(sop->sp_child - sprite, SpriteNum);
                return TRUE;
            }
        }

        if (wph->lotag == TAG_WALL_BREAK)
        {
            HitBreakWall(&wall[hitinfo.wall], sp->x, sp->y, sp->z, sp->ang, u->ID);
            u->ret = 0;
            return TRUE;
        }

        // clipmove does not correctly return the sprite for WALL sprites
        // on walls, so look with hitscan

        hitscan((vec3_t *)sp, sp->sectnum,   // Start position
                sintable[NORM_ANGLE(sp->ang + 512)],    // X vector of 3D ang
                sintable[NORM_ANGLE(sp->ang)],  // Y vector of 3D ang
                sp->zvel,               // Z vector of 3D ang
                &hitinfo, CLIPMASK_MISSILE);

        if (hitinfo.sect < 0)
        {
            return FALSE;
        }

        if (hitinfo.sprite >= 0)
        {
            SPRITEp hsp = &sprite[hitinfo.sprite];

            if (hsp->lotag == TAG_SPRITE_HIT_MATCH)
            {
                if (MissileHitMatch(SpriteNum, -1, hitinfo.sprite))
                    return TRUE;
            }

            if (TEST(hsp->cstat, CSTAT_SPRITE_WALL))
            {
                if (hsp->lotag || hsp->hitag)
                {
                    ShootableSwitch(hitinfo.sprite,SpriteNum);
                    return TRUE;
                }
            }
        }

        return TRUE;
    }
    }

    return FALSE;
}

int
DoUziSmoke(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    short pnum;
    PLAYERp pp;

    //if (sp->picnum != NULL)
    //    DoDamageTest(SpriteNum);
    sp->z -= 200; // !JIM! Make them float up

    return 0;
}

int
DoShotgunSmoke(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    short pnum;
    PLAYERp pp;

    //if (sp->picnum != NULL)
    //    DoDamageTest(SpriteNum);
    sp->z -= 200; // !JIM! Make them float up

    return 0;
}

int
DoMineSpark(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    short pnum;
    PLAYERp pp;

    if (sp->picnum != 0)
    {
        DoDamageTest(SpriteNum);
    }

    return 0;
}

int
DoFireballFlames(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum],ap;
    USERp u = User[SpriteNum];
    SWBOOL jumping = FALSE;

    // if no owner then stay where you are
    if (u->Attach >= 0)
    {
        ap = &sprite[u->Attach];

        sp->x = ap->x;
        sp->y = ap->y;

        sp->z = DIV2(SPRITEp_TOS(ap) + SPRITEp_BOS(ap));

        if (TEST(ap->extra, SPRX_BURNABLE))
        {
            if (MOD2(u->Counter2) == 0)
            {
                ap->shade++;
                if (ap->shade > 10)
                    ap->shade = 10;
            }
        }
    }
    else
    {
        if (TEST(u->Flags, SPR_JUMPING))
        {
            DoJump(SpriteNum);
            jumping = TRUE;
            //u->ret = move_missile(SpriteNum, dax, day, daz, Z(16), Z(16), CLIPMASK_MISSILE, MISSILEMOVETICS);
        }
        else if (TEST(u->Flags, SPR_FALLING))
        {
            DoFall(SpriteNum);
            jumping = TRUE;
            //u->ret = move_missile(SpriteNum, dax, day, daz, Z(16), Z(16), CLIPMASK_MISSILE, MISSILEMOVETICS);
        }
        else
        {
            if (SectUser[sp->sectnum] && SectUser[sp->sectnum]->depth > 0)
            {
                if (labs(sector[sp->sectnum].floorz - sp->z) <= Z(4))
                {
                    KillSprite(SpriteNum);
                    return 0;
                }
            }

            if (TestDontStickSector(sp->sectnum))
            {
                KillSprite(SpriteNum);
                return 0;
            }
        }
    }

    if (!jumping)
    {
        if ((u->WaitTics += MISSILEMOVETICS) > 4 * 120)
        {
            // shrink and go away
            sp->xrepeat--;
            sp->yrepeat--;

            if (((signed char)sp->xrepeat) == 0)
            {
                if (u->Attach >= 0)
                    User[u->Attach]->flame = -1;
                KillSprite(SpriteNum);
                return 0;
            }
        }
        else
        {
            // grow until the right size
            if (sp->xrepeat <= u->Counter)
            {
                sp->xrepeat += 3;
                sp->yrepeat += 3;
            }
        }
    }

    u->Counter2++;
    if (u->Counter2 > 9)
    {
        u->Counter2 = 0;
        DoFlamesDamageTest(SpriteNum);
    }

    return 0;
}

int
DoBreakFlames(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum],ap;
    USERp u = User[SpriteNum];
    SWBOOL jumping = FALSE;

    if (TEST(u->Flags, SPR_JUMPING))
    {
        DoJump(SpriteNum);
        jumping = TRUE;
    }
    else if (TEST(u->Flags, SPR_FALLING))
    {
        DoFall(SpriteNum);
        jumping = TRUE;
    }
    else
    {
        if (SectUser[sp->sectnum] && SectUser[sp->sectnum]->depth > 0)
        {
            if (labs(sector[sp->sectnum].floorz - sp->z) <= Z(4))
            {
                KillSprite(SpriteNum);
                return 0;
            }
        }

        if (TestDontStickSector(sp->sectnum))
        {
            KillSprite(SpriteNum);
            return 0;
        }
    }

    if (!jumping)
    {
        if ((u->WaitTics += MISSILEMOVETICS) > 4 * 120)
        {
            // shrink and go away
            sp->xrepeat--;
            sp->yrepeat--;

            if (((signed char)sp->xrepeat) == 0)
            {
                if (u->Attach >= 0)
                    User[u->Attach]->flame = -1;
                KillSprite(SpriteNum);
                return 0;
            }
        }
        else
        {
            // grow until the right size
            if (sp->xrepeat <= u->Counter)
            {
                sp->xrepeat += 3;
                sp->yrepeat += 3;
            }

            if (u->WaitTics + MISSILEMOVETICS > 4 * 120)
            {
                SpawnBreakStaticFlames(SpriteNum);
            }
        }
    }

    u->Counter2++;
    if (u->Counter2 > 9)
    {
        u->Counter2 = 0;
        DoFlamesDamageTest(SpriteNum);
    }

    return 0;
}

int
SetSuicide(short SpriteNum)
{
#if 1
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    SET(u->Flags, SPR_SUICIDE);
    u->RotNum = 0;
    ChangeState(SpriteNum, s_Suicide);
#else
    // this will NOT work because
    // TRAVERSE_SPRITE_STAT(headspritestat[STAT_MISSILE], i, nexti)
    // nexti will still be valid but will be on a different list
    // and will have a different nextspritestat[nexti] result
    changespritestat(SpriteNum, STAT_SUICIDE);
#endif
    return 0;
}

int
DoActorScale(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = &sprite[SpriteNum];

    u->scale_speed = 70;
    u->scale_value = sp->xrepeat << 8;
    u->scale_tgt = sp->xrepeat + 25;

    if (u->scale_tgt > 256)
    {
        u->scale_speed = 0;
        u->scale_tgt = 256;
    }

    return 0;
}

int
DoRipperGrow(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = &sprite[SpriteNum];

    u->scale_speed = 70;
    u->scale_value = sp->xrepeat << 8;
    u->scale_tgt = sp->xrepeat + 20;

    if (u->scale_tgt > 128)
    {
        u->scale_speed = 0;
        u->scale_tgt = 128;
    }

    return 0;
}

void UpdateSinglePlayKills(short SpriteNum)
{
    // single play and coop kill count
    if (gNet.MultiGameType != MULTI_GAME_COMMBAT)
    {
#if DEBUG
        extern SWBOOL DebugSecret;
        if (DebugSecret)
        {
            SPRITEp sp = &sprite[SpriteNum];
            sprintf(ds,"KILLED: spnum %d, pic %d, x %d, y %d",SpriteNum,sp->picnum,sp->x,sp->y);
            DebugWriteString(ds);
        }
#endif

        ASSERT(User[SpriteNum]);

        if (TEST(User[SpriteNum]->Flags, SPR_SUICIDE))
        {
            return;
        }

        switch (User[SpriteNum]->ID)
        {
        case COOLIE_RUN_R0:
        case NINJA_RUN_R0:
        case NINJA_CRAWL_R0:
        case GORO_RUN_R0:
        case 1441:
        case COOLG_RUN_R0:
        case EEL_RUN_R0:
        case SUMO_RUN_R0:
        case ZILLA_RUN_R0:
        //case TOILETGIRL_R0:
        //case WASHGIRL_R0:
        //case BUNNY_RUN_R0:
        case RIPPER_RUN_R0:
        case RIPPER2_RUN_R0:
        case SERP_RUN_R0:
        case LAVA_RUN_R0:
        case SKEL_RUN_R0:
        case HORNET_RUN_R0:
        case GIRLNINJA_RUN_R0:
        case SKULL_R0:
        case BETTY_R0:
            // always give kills to the first player
            Player->Kills++;
            break;
        }
    }
}


int
ActorChooseDeath(short SpriteNum, short Weapon)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SPRITEp wp = &sprite[Weapon];
    USERp wu = User[Weapon];

    ASSERT(u);

    if (u->Health > 0)
        return FALSE;

    UpdateSinglePlayKills(SpriteNum);

    ASSERT(Weapon >= 0);

#if 0
    if (Weapon < 0)
    {
        switch (Weapon)
        {
        default:
            DoActorDie(SpriteNum, Weapon);
            UpdateSinglePlayKills(SpriteNum);
            break;
        }
        return FALSE;
    }
#endif

    if (u->Attrib)
        PlaySpriteSound(SpriteNum,attr_die,v3df_follow);

    switch (u->ID)
    {
    case PACHINKO1:
    case PACHINKO2:
    case PACHINKO3:
    case PACHINKO4:
    case 623:
    case TOILETGIRL_R0:
    case WASHGIRL_R0:
    case CARGIRL_R0:
    case MECHANICGIRL_R0:
    case SAILORGIRL_R0:
    case PRUNEGIRL_R0:
    case TRASHCAN:
    case GORO_RUN_R0:
    case RIPPER2_RUN_R0:
        break;
    case BUNNY_RUN_R0:
    {
        extern short Bunny_Count;
        Bunny_Count--;  // Bunny died, decrease the population
    }
    break;
    default:
        ActorCoughItem(SpriteNum);
        //UpdateSinglePlayKills(SpriteNum);
        break;
    }

    switch (u->ID)
    {
    case BETTY_R0:
    {
        ANIMATOR DoBettyBeginDeath;
        DoBettyBeginDeath(SpriteNum);
        break;
    }
    case SKULL_R0:
    {
        ANIMATOR DoSkullBeginDeath;
        DoSkullBeginDeath(SpriteNum);
        break;
    }
    case TOILETGIRL_R0:
    case WASHGIRL_R0:
    case CARGIRL_R0:
    case MECHANICGIRL_R0:
    case SAILORGIRL_R0:
    case PRUNEGIRL_R0:
    case TRASHCAN:
    case PACHINKO1:
    case PACHINKO2:
    case PACHINKO3:
    case PACHINKO4:
    case 623:
    {
        if ((u->ID == TOILETGIRL_R0 ||
             u->ID == CARGIRL_R0 || u->ID == MECHANICGIRL_R0 || u->ID == SAILORGIRL_R0 || u->ID == PRUNEGIRL_R0 ||
             u->ID == WASHGIRL_R0) && wu->ID == NINJA_RUN_R0 && wu->PlayerP)
        {
            PLAYERp pp = wu->PlayerP;
            if (pp && !TEST(pp->Flags, PF_DIVING))  // JBF: added null test
                pp->Bloody = TRUE;
            PlaySound(DIGI_TOILETGIRLSCREAM, &sp->x, &sp->y, &sp->z, v3df_none);
        }
        if (SpawnShrap(SpriteNum, Weapon))
            SetSuicide(SpriteNum);
        break;
    }

    // These are player zombies
    case ZOMBIE_RUN_R0:
        InitBloodSpray(SpriteNum,TRUE,105);
        InitBloodSpray(SpriteNum,TRUE,105);
        if (SpawnShrap(SpriteNum, Weapon))
            SetSuicide(SpriteNum);
        break;

    default:

        switch (wu->ID)
        {
        case NINJA_RUN_R0: //sword
            if (wu->PlayerP)
            {
                PLAYERp pp = wu->PlayerP;

                if (wu->WeaponNum == WPN_FIST && STD_RANDOM_RANGE(1000)>500 && pp == Player+myconnectindex)
                {
                    char choosesnd = 0;

                    choosesnd=STD_RANDOM_RANGE(6);

                    if (choosesnd == 0)
                        PlayerSound(DIGI_KUNGFU,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
                    else if (choosesnd == 1)
                        PlayerSound(DIGI_PAYINGATTENTION,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
                    else if (choosesnd == 2)
                        PlayerSound(DIGI_EATTHIS,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
                    else if (choosesnd == 3)
                        PlayerSound(DIGI_TAUNTAI4,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
                    else if (choosesnd == 4)
                        PlayerSound(DIGI_TAUNTAI5,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
                    else if (choosesnd == 5)
                        PlayerSound(DIGI_HOWYOULIKEMOVE,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
                    //PlayerSound(TauntAIVocs[choosesnd],&pp->posx,
                    //    &pp->posy,&pp->posy,v3df_dontpan|v3df_follow,pp);
                }
                else if (wu->WeaponNum == WPN_SWORD && STD_RANDOM_RANGE(1000)>500 && pp == Player+myconnectindex)
                {
                    short choose_snd;

                    choose_snd = STD_RANDOM_RANGE(1000);
                    if (choose_snd > 750)
                        PlayerSound(DIGI_SWORDGOTU1,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
                    else if (choose_snd > 575)
                        PlayerSound(DIGI_SWORDGOTU2,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
                    else if (choose_snd > 250)
                        PlayerSound(DIGI_SWORDGOTU3,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
                    else
                        PlayerSound(DIGI_CANBEONLYONE,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
                }
                if (!TEST(pp->Flags, PF_DIVING))
                    pp->Bloody = TRUE;
            }

            if (u->WeaponNum == WPN_FIST)
                DoActorDie(SpriteNum, Weapon);
            else if (u->ID == NINJA_RUN_R0 || RANDOM_RANGE(1000) < 500)
                DoActorDie(SpriteNum, Weapon);
            else
            {
                // Can't gib bosses!
                if (u->ID == SERP_RUN_R0 || u->ID == SUMO_RUN_R0 || u->ID == ZILLA_RUN_R0)
                {
                    DoActorDie(SpriteNum, Weapon);
                    break;
                }

                // Gib out the ones you can't cut in half
                // Blood fountains
                InitBloodSpray(SpriteNum,TRUE,-1);

                if (SpawnShrap(SpriteNum, Weapon))
                {
                    SetSuicide(SpriteNum);
                }
                else
                    DoActorDie(SpriteNum, Weapon);

            }
            break;
#if 0
        case PLASMA_FOUNTAIN:
            if (SpawnShrap(SpriteNum, Weapon))
                SetSuicide(SpriteNum);
            else
                DoActorDie(SpriteNum, Weapon);
            break;
#endif
        case BOLT_THINMAN_R0:
        case BOLT_THINMAN_R1:
        case BOLT_THINMAN_R2:
        case NAP_EXP:
        case BOLT_EXP:
        case TANK_SHELL_EXP:
        case SECTOR_EXP:
        case FIREBALL_EXP:
        case GRENADE_EXP:
        case MINE_EXP:
        case SKULL_R0:
        case BETTY_R0:
#if 0
            if (RANDOM_P2(1024) < 256 && wu->Radius != NUKE_RADIUS)
            {
                if (wu->ID == BOLT_THINMAN_R1 && wu->Radius == RAIL_RADIUS)
                {
                    SpawnShrapX(Weapon);    // Do rail gun shrap
                }
                DoActorDie(SpriteNum, Weapon);

            }
            else
#endif
            {
                int choosesnd = 0;

                // For the Nuke, do residual radiation if he gibs
                if (wu->Radius == NUKE_RADIUS)
                    SpawnFireballFlames(SpriteNum, -1);

                // Random chance of taunting the AI's here
                if (RANDOM_RANGE(1000) > 400)
                {
                    PLAYERp pp;

                    if (wp && wp->owner >= 0)
                    {
                        if (User[wp->owner]->PlayerP)
                        {

                            pp = User[wp->owner]->PlayerP;


                            ASSERT(pp >= Player && pp <= Player+MAX_SW_PLAYERS);
                            choosesnd=STD_RANDOM_RANGE(MAX_TAUNTAI<<8)>>8;

                            if (pp && pp == Player+myconnectindex)
                                PlayerSound(TauntAIVocs[choosesnd],&pp->posx,
                                            &pp->posy,&pp->posy,v3df_dontpan|v3df_follow,pp);
                        }
                    }
                }

                // These guys cough items only if gibbed
                if (u->ID == GORO_RUN_R0 || u->ID == RIPPER2_RUN_R0)
                    ActorCoughItem(SpriteNum);

                // Blood fountains
                InitBloodSpray(SpriteNum,TRUE,-1);

                // Bosses do not gib
                if (u->ID == SERP_RUN_R0 || u->ID == SUMO_RUN_R0 || u->ID == ZILLA_RUN_R0)
                {
                    DoActorDie(SpriteNum, Weapon);
                    return TRUE;
                }

                if (SpawnShrap(SpriteNum, Weapon))
                {
                    SetSuicide(SpriteNum);
                }
                else
                    DoActorDie(SpriteNum, Weapon);

            }

            break;
        default:
            DoActorDie(SpriteNum, Weapon);
            break;
        }

        break;
    }

    return TRUE;

}

int
ActorHealth(short SpriteNum, short amt)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    extern SWBOOL FinishAnim;
    extern SWBOOL FinishedLevel;

    if (u->ID == TRASHCAN && amt > -75)
    {
        u->LastDamage = 100;
        return TRUE;
    }

    SET(u->Flags, SPR_ATTACKED);

    u->Health += amt;

    if (u->ID == SERP_RUN_R0 && sp->pal != 16 && Level == 4)
    {
        if (u->Health < u->MaxHealth/2)
        {
            ExitLevel = TRUE;
            FinishAnim = ANIM_SERP;
            FinishedLevel = TRUE;
            return TRUE;
        }
    }

    if (u->ID == SUMO_RUN_R0 && Level == 11)
    {
        if (u->Health <= 0)
        {
            FinishTimer = 7*120;
            FinishAnim = ANIM_SUMO;
        }
    }

    if (u->ID == ZILLA_RUN_R0 && Level == 20)
    {
        if (u->Health <= 0)
        //if (u->Health < u->MaxHealth)
        {
            FinishTimer = 15*120;
            FinishAnim = ANIM_ZILLA;
        }
    }

    if (u->Attrib && RANDOM_P2(1024) > 850)
        PlaySpriteSound(SpriteNum,attr_pain,v3df_follow|v3df_dontpan);

    // keep track of the last damage
    if (amt < 0)
        u->LastDamage = -amt;

    // Do alternate Death2 if it exists
    if (u->ActorActionSet && u->ActorActionSet->Death2) // JBF: added null check
    {
#define DEATH2_HEALTH_VALUE 15

        if (u->Health  <= DEATH2_HEALTH_VALUE)
        {
            // If he's dead, possibly choose a special death type
#if 1 // Problematic code, REMOVED.
            switch (u->ID)
            {
            case NINJA_RUN_R0:
            {
                extern STATEp sg_NinjaGrabThroat[];

                if (TEST(u->Flags2, SPR2_DYING)) return TRUE;
                if (TEST(u->Flags, SPR_FALLING | SPR_JUMPING | SPR_CLIMBING)) return TRUE;

                if (!TEST(u->Flags2, SPR2_DYING))
                {
                    short rnd;

                    rnd = RANDOM_P2(1024<<4)>>4;
                    if (rnd < 950)
                        return TRUE;
                    SET(u->Flags2, SPR2_DYING); // Only let it check this once!
                    u->WaitTics = SEC(1) + SEC(RANDOM_RANGE(2));
                    u->Health = 60;
                    PlaySound(DIGI_NINJACHOKE,&sp->x,&sp->y,&sp->z,v3df_follow);
                    InitPlasmaFountain(NULL, sp);
                    InitBloodSpray(SpriteNum,FALSE,105);
                    sp->ang = NORM_ANGLE(getangle(u->tgt_sp->x - sp->x, u->tgt_sp->y - sp->y) + 1024);
                    RESET(sp->cstat, CSTAT_SPRITE_YFLIP);
                    NewStateGroup(SpriteNum, sg_NinjaGrabThroat);
                }
                break;
            }
            }
#endif
        }
    }

    return TRUE;
}

int
SopDamage(SECTOR_OBJECTp sop, short amt)
{
    SPRITEp sp = sop->sp_child;
    USERp u = User[sp - sprite];

    // does not have damage
    if (sop->max_damage == -9999)
        return FALSE;

    sop->max_damage += amt;

    // keep track of the last damage
    if (amt < 0)
        u->LastDamage = -amt;

    return TRUE;
}

int
SopCheckKill(SECTOR_OBJECTp sop)
{
    SPRITEp sp = sop->sp_child;
    USERp u = User[sp - sprite];
    SWBOOL killed = FALSE;
    void VehicleSetSmoke(SECTOR_OBJECTp sop, ANIMATORp animator);

    if (TEST(sop->flags, SOBJ_BROKEN))
        return FALSE;

    // does not have damage
    if (sop->max_damage == -9999)
        return killed;

    if (sop->max_damage <= 0)
    {
        // returns whether so was collapsed(killed)
        killed = TestKillSectorObject(sop);
        if (!killed)
        {
            VehicleSetSmoke(sop, SpawnVehicleSmoke);
            SET(sop->flags, SOBJ_BROKEN);
        }
    }

    return killed;
}

int
ActorPain(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    //if (u->LastDamage < u->PainThreshold)  // This doesn't work well at all because of
    // uzi/shotgun damages
    switch (u->ID)
    {
    case TOILETGIRL_R0:
    case WASHGIRL_R0:
    case CARGIRL_R0:
    case MECHANICGIRL_R0:
    case SAILORGIRL_R0:
    case PRUNEGIRL_R0:
        u->FlagOwner = 1;
        break;
    }

    if (RANDOM_RANGE(1000) < 875 || u->WaitTics > 0)
        return FALSE;

    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
    {
        if (u->ActorActionSet && u->ActorActionSet->Pain)
        {
            ActorLeaveTrack(SpriteNum);
            u->WaitTics = 60;
            NewStateGroup(SpriteNum, u->ActorActionSet->Pain);
            return TRUE;
        }
    }

    return FALSE;
}

int
ActorPainPlasma(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (!TEST(u->Flags, SPR_JUMPING | SPR_FALLING | SPR_ELECTRO_TOLERANT))
    {
        if (u->ActorActionSet && u->ActorActionSet->Pain)
        {
            u->WaitTics = PLASMA_FOUNTAIN_TIME;
            NewStateGroup(SpriteNum, u->ActorActionSet->Pain);
            return TRUE;
        }
        else
        {
            u->Vis = PLASMA_FOUNTAIN_TIME;
            InitActorPause(SpriteNum);
        }
    }

    return FALSE;
}

int
ActorStdMissile(short SpriteNum, short Weapon)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SPRITEp wp = &sprite[Weapon];
    USERp wu = User[Weapon];

    ASSERT(Weapon >= 0);

    // Attack the player that is attacking you
    // Only if hes still alive
    if (wp->owner >= 0)
    {
// Commented out line will make actors attack each other, it's been removed as feature
//        if (User[wp->owner]->PlayerP || User[wp->owner]->SpriteP)
        if (User[wp->owner]->PlayerP && sp->owner != wp->owner)
        {
            u->tgt_sp = &sprite[wp->owner];
        }
    }

    // Reset the weapons target before dying
    if (wu->WpnGoal >= 0)
    {
        // attempt to see if it was killed
        ASSERT(sprite[wu->WpnGoal].sectnum >= 0);
        ASSERT(User[Weapon]);
        // what if the target has been killed?  This will crash!
        RESET(User[wu->WpnGoal]->Flags, SPR_TARGETED);
    }

    return 0;
}
int
ActorDamageSlide(short SpriteNum, short damage, short ang)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    int slide_vel,slide_dec;

    if (TEST(u->Flags, SPR_CLIMBING))
        return FALSE;

    damage = labs(damage);

    if (!damage)
        return FALSE;

    if (damage <= 10)
    {
        DoActorBeginSlide(SpriteNum, ang, 64, 5);
        return TRUE;
    }
    else if (damage <= 20)
    {
        DoActorBeginSlide(SpriteNum, ang, 128, 5);
        return TRUE;
    }
    else
    {
        slide_vel = (damage * 6) - (u->MaxHealth);

        //DSPRINTF(ds,"slide_vel = %ld",slide_vel);
        MONO_PRINT(ds);

        if (slide_vel < -1000) slide_vel = -1000;
        slide_dec = 5;

        DoActorBeginSlide(SpriteNum, ang, slide_vel, slide_dec);

        return TRUE;
    }
}

int
PlayerDamageSlide(PLAYERp pp, short damage, short ang)
{
    int slide_vel;
    USERp u = User[pp->PlayerSprite];

    damage = labs(damage);

    if (!damage)
        return FALSE;

    if (damage <= 5)
    {
        //nudge
        //pp->slide_xvect = MOVEx(4, ang)<<15;
        //pp->slide_yvect = MOVEy(4, ang)<<15;
        //return(TRUE);
        return FALSE;
    }
    else if (damage <= 10)
    {
        //nudge
        pp->slide_xvect = MOVEx(16, ang)<<15;
        pp->slide_yvect = MOVEy(16, ang)<<15;
        return TRUE;
    }
    else if (damage <= 20)
    {
        //bigger nudge
        pp->slide_xvect = MOVEx(64, ang)<<15;
        pp->slide_yvect = MOVEy(64, ang)<<15;
        return TRUE;
    }
    else
    {
        extern char PlayerGravity;

        slide_vel = (damage * 6);

        pp->slide_xvect = MOVEx(slide_vel, ang)<<15;
        pp->slide_yvect = MOVEy(slide_vel, ang)<<15;

        return TRUE;
    }
}


int
GetDamage(short SpriteNum, short Weapon, short DamageNdx)
{
    DAMAGE_DATAp d = &DamageData[DamageNdx];

    ASSERT(SpriteNum >= 0 && Weapon >= 0);

    // if ndx does radius
    if (d->radius > 0)
    {
        SPRITEp sp = &sprite[SpriteNum];
        USERp u = User[SpriteNum];
        SPRITEp wp = &sprite[Weapon];
        USERp wu = User[Weapon];
        int dist,a,b,c;
        int damage_per_pixel, damage_force, damage_amt;


        DISTANCE(wp->x,wp->y,sp->x,sp->y,dist,a,b,c);

        // take off the box around the player or else you'll never get
        // the max_damage;
        dist -= ((int)sp->clipdist)<<(2);

        if (dist < 0) dist = 0;

        if ((unsigned)dist < d->radius)
        {
            damage_per_pixel = (d->damage_hi<<16)/d->radius;

            //the closer your distance is to 0 the more damage
            damage_force = (d->radius - dist);
            damage_amt = -((damage_force * damage_per_pixel)>>16);

            //return(damage_amt);
            // formula: damage_amt = 75% + random(25%)
            return DIV2(damage_amt) + DIV4(damage_amt) + RANDOM_RANGE(DIV4(damage_amt));
        }
        else
        {
            return 0;
        }
    }

    return -(d->damage_lo + RANDOM_RANGE(d->damage_hi - d->damage_lo));
}

int
RadiusGetDamage(short SpriteNum, short Weapon, int max_damage)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SPRITEp wp = &sprite[Weapon];
    USERp wu = User[Weapon];
    int dist,a,b,c;
    int damage_per_pixel, damage_force, damage_amt;

    max_damage = labs(max_damage);

    DISTANCE(wp->x,wp->y,sp->x,sp->y,dist,a,b,c);

    // take off the box around the player or else you'll never get
    // the max_damage;
    dist -= ((int)sp->clipdist)<<(2);
    if (dist < 0) dist = 0;

    if ((unsigned)dist < wu->Radius)
    {
        damage_per_pixel = (max_damage<<16)/wu->Radius;

        //the closer your distance is to 0 the more damage
        damage_force = (wu->Radius - dist);
        // fudge factor
        //damage_force += 400;

        damage_amt = -((damage_force * damage_per_pixel)>>16);
    }
    else
    {
        damage_amt = 0;
    }


    return damage_amt;
}


int
PlayerCheckDeath(PLAYERp pp, short Weapon)
{
    SPRITEp sp = pp->SpriteP;
    USERp u = User[pp->PlayerSprite];
    SPRITEp wp = &sprite[Weapon];
    USERp   wu = User[Weapon];
    int SpawnZombie(PLAYERp pp, short);


    // Store off what player was struck by
    pp->HitBy = Weapon;

    if (u->Health <= 0 && !TEST(pp->Flags, PF_DEAD))
    {
        void DoPlayerBeginDie(PLAYERp);

        // pick a death type
        if (u->LastDamage >= PLAYER_DEATH_EXPLODE_DAMMAGE_AMT)
            pp->DeathType = PLAYER_DEATH_EXPLODE;
        else if (u->LastDamage >= PLAYER_DEATH_CRUMBLE_DAMMAGE_AMT)
            pp->DeathType = PLAYER_DEATH_CRUMBLE;
        else
            pp->DeathType = PLAYER_DEATH_FLIP;

        if (Weapon < 0)
        {
            pp->Killer = -1;
            DoPlayerBeginDie(pp);
            return TRUE;
        }

        if (Weapon > -1 && (wu->ID == RIPPER_RUN_R0 || wu->ID == RIPPER2_RUN_R0))
            pp->DeathType = PLAYER_DEATH_RIPPER;

        if (Weapon > -1 && wu->ID == CALTROPS)
            pp->DeathType = PLAYER_DEATH_FLIP;

        if (Weapon > -1 && wu->ID == NINJA_RUN_R0 && wu->PlayerP)
        {
            PLAYERp wp = wu->PlayerP;

            pp->DeathType = PLAYER_DEATH_FLIP;
            wu->PlayerP->Bloody = TRUE;
        }

        // keep track of who killed you for death purposes
        // need to check all Killer variables when an enemy dies
        if (pp->Killer < 0)
        {
            if (wp->owner >= 0)
                pp->Killer = wp->owner;
            else if (TEST(wp->extra, SPRX_PLAYER_OR_ENEMY))
                pp->Killer = Weapon;
        }

        // start the death process
        DoPlayerBeginDie(pp);

        // for death direction
        //u->slide_ang = wp->ang;
        u->slide_ang = getangle(sp->x - wp->x, sp->y - wp->y);
        // for death velocity
        u->slide_vel = u->LastDamage * 5;

        return TRUE;
    }

    return FALSE;
}

SWBOOL
PlayerTakeDamage(PLAYERp pp, short Weapon)
{
    SPRITEp sp = pp->SpriteP;
    USERp u = User[pp->PlayerSprite];
    SPRITEp wp = &sprite[Weapon];
    USERp   wu = User[Weapon];

    if (Weapon < 0)
        return TRUE;

    if (gNet.MultiGameType == MULTI_GAME_NONE)
    {
        // ZOMBIE special case for single play
        if (wu->ID == ZOMBIE_RUN_R0)
        {
            // if weapons owner the player
            if (wp->owner == pp->PlayerSprite)
                return FALSE;
        }

        return TRUE;
    }

    if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
    {
        // everything hurts you
        if (gNet.HurtTeammate)
            return TRUE;

        // if weapon IS the YOURSELF take damage
        if (wu->PlayerP == pp)
            return TRUE;

        // if the weapons owner is YOURSELF take damage
        if (wp->owner >= 0 && User[wp->owner] && User[wp->owner]->PlayerP && User[wp->owner]->PlayerP == pp)
            return TRUE;

        // if weapon IS the player no damage
        if (wu->PlayerP)
            return FALSE;

        // if the weapons owner is a player
        if (wp->owner >= 0 && User[wp->owner] && User[wp->owner]->PlayerP)
            return FALSE;
    }
    else if (gNet.MultiGameType == MULTI_GAME_COMMBAT && gNet.TeamPlay)
    {
        // everything hurts you
        if (gNet.HurtTeammate)
            return TRUE;

        // if weapon IS the YOURSELF take damage
        if (wu->PlayerP == pp)
            return TRUE;

        // if the weapons owner is YOURSELF take damage
        if (wp->owner >= 0 && User[wp->owner] && User[wp->owner]->PlayerP && User[wp->owner]->PlayerP == pp)
            return TRUE;

        if (wu->PlayerP)
        {
            // if both on the same team then no damage
            if (wu->spal == u->spal)
                return FALSE;
        }

        // if the weapons owner is a player
        if (wp->owner >= 0 && User[wp->owner] && User[wp->owner]->PlayerP)
        {
            // if both on the same team then no damage
            if (User[wp->owner]->spal == u->spal)
                return FALSE;
        }
    }

    return TRUE;
}


int
StarBlood(short SpriteNum, short Weapon)
{
    USERp u = User[SpriteNum];
    short blood_num = 1;
    short i;

    if (u->Health <= 0)
        blood_num = 4;

    for (i = 0; i < blood_num; i++)
        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);
    return 0;
}


/*

!AIC KEY - This is where damage is assesed when missiles hit actors and other
objects.

*/

int
DoDamage(short SpriteNum, short Weapon)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SPRITEp wp;
    USERp wu;
    int dist;
    int damage=0;

    ASSERT(u);

    // don't hit a dead player
    if (u->PlayerP && TEST(u->PlayerP->Flags, PF_DEAD))
    {
        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);
        return 0;
    }

    if (TEST(u->Flags, SPR_SUICIDE))
        return 0;

    wp = &sprite[Weapon];
    wu = User[Weapon];

    ASSERT(wu);

    if (TEST(wu->Flags, SPR_SUICIDE))
        return 0;

    ASSERT(u);
    if (u->Attrib && RANDOM_P2(1024) > 850)
        PlaySpriteSound(SpriteNum,attr_pain,v3df_follow);

    if (TEST(u->Flags, SPR_DEAD))
    {
        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);
        return 0;
    }


    // special case for shooting mines
    if (sp->statnum == STAT_MINE_STUCK)
    {
        SpawnMineExp(SpriteNum);
        KillSprite(SpriteNum);
        return 0;
    }

    // weapon is drivable object manned by player
    if (wu->PlayerP && wu->PlayerP->sop)
    {
        switch (wu->PlayerP->sop->track)
        {
        case SO_TANK:
            damage = -200;

            if (u->sop_parent)
            {
                break;
            }
            else if (u->PlayerP)
            {
                PlayerDamageSlide(u->PlayerP, damage, wp->ang);
                if (PlayerTakeDamage(u->PlayerP, Weapon))
                {
                    PlayerUpdateHealth(u->PlayerP, damage);
                    PlayerCheckDeath(u->PlayerP, Weapon);
                }
            }
            else
            {
                PLAYERp pp = Player + screenpeek;

                ActorHealth(SpriteNum, damage);
                if (u->Health <= 0)
                {
                    int choosesnd=0;
                    // Random chance of taunting the AI's here
                    if (STD_RANDOM_RANGE(1024) > 512 && pp == Player+myconnectindex)
                    {
                        choosesnd=RANDOM_RANGE(MAX_TAUNTAI);
                        PlayerSound(TauntAIVocs[choosesnd],&pp->posx,
                                    &pp->posy,&pp->posy,v3df_dontpan|v3df_follow,pp);
                    }
                    SpawnShrap(SpriteNum, Weapon);
                    SetSuicide(SpriteNum);
                    return 0;
                }
            }
            break;

        case SO_SPEED_BOAT:
            damage = -100;

            if (u->sop_parent)
            {
                break;
            }
            else if (u->PlayerP)
            {
                PlayerDamageSlide(u->PlayerP, damage, wp->ang);
                if (PlayerTakeDamage(u->PlayerP, Weapon))
                {
                    PlayerUpdateHealth(u->PlayerP, damage);
                    PlayerCheckDeath(u->PlayerP, Weapon);
                }
            }
            else
            {
                ActorHealth(SpriteNum, damage);
                ActorPain(SpriteNum);
                ActorChooseDeath(SpriteNum, Weapon);
            }

            SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);
            break;
        }
    }

    if (Player[0].PlayerSprite == Weapon)
    {
        //DSPRINTF(ds,"got here 4, %d", User[Player[0].PlayerSprite]->ID);
        MONO_PRINT(ds);
    }


    // weapon is the actor - no missile used - example swords, axes, etc
    switch (wu->ID)
    {
    case NINJA_RUN_R0:

        ASSERT(wu->PlayerP);

        if (wu->WeaponNum == WPN_SWORD)
            damage = GetDamage(SpriteNum, Weapon, WPN_SWORD);
        else
        {
            damage = GetDamage(SpriteNum, Weapon, WPN_FIST);
            // Add damage for stronger attacks!
            switch (wu->PlayerP->WpnKungFuMove)
            {
            case 1:
                damage -= 2 - RANDOM_RANGE(7);
                break;
            case 2:
                damage -= 5 - RANDOM_RANGE(10);
                break;
            }
            PlaySound(DIGI_CGTHIGHBONE,&wp->x,&wp->y,&wp->z,v3df_follow|v3df_dontpan);
        }

        ////DSPRINTF(ds,"got here 3, %d",damage);
        //MONO_PRINT(ds);

        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            // Is the player blocking?
            if (u->PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
            if (u->PlayerP->Armor)
                PlaySound(DIGI_ARMORHIT,&u->PlayerP->posx,&u->PlayerP->posy,&u->PlayerP->posz,v3df_dontpan|v3df_follow|v3df_doppler);
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);

        break;

    case SKEL_RUN_R0:
    case COOLG_RUN_R0:
    case GORO_RUN_R0:

        damage = GetDamage(SpriteNum, Weapon, DMG_SKEL_SLASH);
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            // Is the player blocking?
            if (u->PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);

        break;

    case HORNET_RUN_R0:
        PlaySound(DIGI_HORNETSTING,&sp->x,&sp->y,&sp->z,v3df_follow|v3df_dontpan);
        damage = GetDamage(SpriteNum, Weapon, DMG_HORNET_STING);
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            // Is the player blocking?
            if (u->PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        //SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);

        break;

    case EEL_RUN_R0:
        damage = GetDamage(SpriteNum, Weapon, DMG_RIPPER_SLASH);
        damage /= 3;
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            // Is the player blocking?
            if (u->PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);

        break;

    case RIPPER_RUN_R0:
        damage = GetDamage(SpriteNum, Weapon, DMG_RIPPER_SLASH);
        damage /= 3; // Little rippers aren't as tough.
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            // Is the player blocking?
            if (u->PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                if (PlayerCheckDeath(u->PlayerP, Weapon))
                {
                    PlaySound(DIGI_RIPPERHEARTOUT,&u->PlayerP->posx,&u->PlayerP->posy,
                              &u->PlayerP->posz,v3df_dontpan|v3df_doppler);

                    DoRipperRipHeart(Weapon);
                }
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);

        break;

    case RIPPER2_RUN_R0:
        damage = GetDamage(SpriteNum, Weapon, DMG_RIPPER_SLASH);
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            // Is the player blocking?
            if (u->PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                if (PlayerCheckDeath(u->PlayerP, Weapon))
                {
                    PlaySound(DIGI_RIPPERHEARTOUT,&u->PlayerP->posx,&u->PlayerP->posy,
                              &u->PlayerP->posz,v3df_dontpan|v3df_doppler);

                    DoRipper2RipHeart(Weapon);
                }
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);

        break;

    case BUNNY_RUN_R0:
        damage = GetDamage(SpriteNum, Weapon, DMG_RIPPER_SLASH);
        damage /= 3;
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            extern int DoBunnyRipHeart(short SpriteNum);
            // Is the player blocking?
            if (u->PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                if (PlayerCheckDeath(u->PlayerP, Weapon))
                {
                    DoBunnyRipHeart(Weapon);
                }
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);

        break;

    case SERP_RUN_R0:
        damage = GetDamage(SpriteNum, Weapon, DMG_SERP_SLASH);
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            if (u->PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(u->PlayerP, damage/4, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);
        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);
        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);

        break;

    case BLADE1:
    case BLADE2:
    case BLADE3:
    case 5011:

        if (wu->ID == 5011)
            damage = -(3 + (RANDOM_RANGE(4<<8)>>8));
        else
            damage = GetDamage(SpriteNum, Weapon, DMG_BLADE);
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            if (u->PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            if ((u->BladeDamageTics -= synctics) < 0)
            {
                u->BladeDamageTics = DAMAGE_BLADE_TIME;
                PlayerDamageSlide(u->PlayerP, damage, ANG2PLAYER(u->PlayerP, wp));
                if (PlayerTakeDamage(u->PlayerP, Weapon))
                {
                    PlayerUpdateHealth(u->PlayerP, damage);
                    PlayerCheckDeath(u->PlayerP, Weapon);
                }
            }
        }
        else
        {
            if ((u->BladeDamageTics -= ACTORMOVETICS) < 0)
            {
                u->BladeDamageTics = DAMAGE_BLADE_TIME;
                ActorHealth(SpriteNum, damage);
            }

            ActorChooseDeath(SpriteNum, Weapon);
        }

        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);

        break;

    case STAR1:
    case CROSSBOLT:
        damage = GetDamage(SpriteNum, Weapon, WPN_STAR);

        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            MONO_PRINT("Stat Hit Actor");
            // Is the player blocking?
            if (u->PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
            if (u->PlayerP->Armor)
                PlaySound(DIGI_ARMORHIT,&u->PlayerP->posx,&u->PlayerP->posy,&u->PlayerP->posz,v3df_dontpan|v3df_follow|v3df_doppler);
        }
        else
        {
            MONO_PRINT("Star Hit Actor");
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorStdMissile(SpriteNum, Weapon);
            ActorDamageSlide(SpriteNum, damage, wp->ang);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        StarBlood(SpriteNum, Weapon);

        wu->ID = 0;
        SetSuicide(Weapon);
        break;

    case SPEAR_R0:
        damage = GetDamage(SpriteNum, Weapon, DMG_SPEAR_TRAP);

        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
            if (u->PlayerP->Armor)
                PlaySound(DIGI_ARMORHIT,&u->PlayerP->posx,&u->PlayerP->posy,&u->PlayerP->posz,v3df_dontpan|v3df_follow|v3df_doppler);
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorStdMissile(SpriteNum, Weapon);
            ActorDamageSlide(SpriteNum, damage, wp->ang);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);

        wu->ID = 0;
        SetSuicide(Weapon);
        break;

    case LAVA_BOULDER:
        damage = GetDamage(SpriteNum, Weapon, DMG_LAVA_BOULDER);

        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorStdMissile(SpriteNum, Weapon);
            ActorDamageSlide(SpriteNum, damage, wp->ang);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);

        wu->ID = 0;
        SetSuicide(Weapon);
        break;

    case LAVA_SHARD:
        damage = GetDamage(SpriteNum, Weapon, DMG_LAVA_SHARD);

        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorStdMissile(SpriteNum, Weapon);
            ActorDamageSlide(SpriteNum, damage, wp->ang);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);

        wu->ID = 0;
        SetSuicide(Weapon);
        break;

    case UZI_SMOKE:
    case UZI_SMOKE+2:
        if (wu->ID == UZI_SMOKE)
            damage = GetDamage(SpriteNum, Weapon, WPN_UZI);
        else
            damage = GetDamage(SpriteNum, Weapon, WPN_UZI)/3; // Enemy Uzi, 1/3 damage

        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            //PlayerDamageSlide(u->PlayerP, damage, ANG2PLAYER(u->PlayerP, wp));
            PlayerDamageSlide(u->PlayerP, damage/2, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
            if (u->PlayerP->Armor)
                PlaySound(DIGI_ARMORHIT,&u->PlayerP->posx,&u->PlayerP->posy,&u->PlayerP->posz,v3df_dontpan|v3df_follow|v3df_doppler);
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorStdMissile(SpriteNum, Weapon);
            ActorDamageSlide(SpriteNum, damage, wp->ang);
            ActorChooseDeath(SpriteNum, Weapon);
            switch (u->ID)
            {
            case TRASHCAN:
            case PACHINKO1:
            case PACHINKO2:
            case PACHINKO3:
            case PACHINKO4:
            case 623:
            case ZILLA_RUN_R0:
                break;
            default:
                if (RANDOM_RANGE(1000) > 900)
                    InitBloodSpray(SpriteNum,FALSE,105);
                if (RANDOM_RANGE(1000) > 900)
                    SpawnMidSplash(SpriteNum);
                break;
            }
        }

        //SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);
        // reset id so no more damage is taken
        wu->ID = 0;
        break;

    case SHOTGUN_SMOKE:
        damage = GetDamage(SpriteNum, Weapon, WPN_SHOTGUN);

        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
            if (u->PlayerP->Armor)
                PlaySound(DIGI_ARMORHIT,&u->PlayerP->posx,&u->PlayerP->posy,&u->PlayerP->posz,v3df_dontpan|v3df_follow|v3df_doppler);
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorStdMissile(SpriteNum, Weapon);
            ActorDamageSlide(SpriteNum, damage, wp->ang);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        //SpawnBlood(SpriteNum, Weapon, 0, 0, 0, 0);
        switch (u->ID)
        {
        case TRASHCAN:
        case PACHINKO1:
        case PACHINKO2:
        case PACHINKO3:
        case PACHINKO4:
        case 623:
        case ZILLA_RUN_R0:
            break;
        default:
            if (RANDOM_RANGE(1000) > 950)
                SpawnMidSplash(SpriteNum);
            break;
        }

        // reset id so no more damage is taken
        wu->ID = 0;
        break;

    case MIRV_METEOR:

        //damage = -DAMAGE_MIRV_METEOR;
        damage = GetDamage(SpriteNum, Weapon, DMG_MIRV_METEOR);
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorStdMissile(SpriteNum, Weapon);
        }

        SetSuicide(Weapon);
        break;

    case SERP_METEOR:

        //damage = -DAMAGE_SERP_METEOR;
        damage = GetDamage(SpriteNum, Weapon, DMG_SERP_METEOR);
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorStdMissile(SpriteNum, Weapon);
        }

        SetSuicide(Weapon);
        break;

    case BOLT_THINMAN_R0:
        damage = GetDamage(SpriteNum, Weapon, WPN_ROCKET);

        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorStdMissile(SpriteNum, Weapon);
        }

        if (wu->Radius == NUKE_RADIUS)
            SpawnNuclearExp(Weapon);
        else
            SpawnBoltExp(Weapon);
        SetSuicide(Weapon);
        break;

    case BOLT_THINMAN_R1:
        //damage =  -(2000 + (65 + RANDOM_RANGE(40))); // -2000 makes armor not count
        damage =  -(65 + RANDOM_RANGE(40));

        if (u->sop_parent)
        {
            if (TEST(u->sop_parent->flags, SOBJ_DIE_HARD))
                break;
            SopDamage(u->sop_parent, damage);
            SopCheckKill(u->sop_parent);
            break;
        }
        else if (u->PlayerP)
        {
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
            if (u->PlayerP->Armor)
                PlaySound(DIGI_ARMORHIT,&u->PlayerP->posx,&u->PlayerP->posy,&u->PlayerP->posz,v3df_dontpan|v3df_follow|v3df_doppler);
        }
        else
        {
            // this is special code to prevent the Zombie from taking out the Bosses to quick
            // if rail gun weapon owner is not player
            if (wp->owner >= 0 && User[wp->owner] && !User[wp->owner]->PlayerP)
            {
                // if actor is a boss
                if (u->ID == ZILLA_RUN_R0 || u->ID == SERP_RUN_R0 || u->ID == SUMO_RUN_R0)
                    damage /= 2;
            }

            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorStdMissile(SpriteNum, Weapon);
            ActorDamageSlide(SpriteNum, damage>>1, wp->ang);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        wu->ID = 0; // No more damage
        SpawnTracerExp(Weapon);
        SetSuicide(Weapon);
        break;

    case BOLT_THINMAN_R2:
        damage = (GetDamage(SpriteNum, Weapon, WPN_ROCKET)/2);

        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorStdMissile(SpriteNum, Weapon);
        }

        if (wu->Radius == NUKE_RADIUS)
            SpawnNuclearExp(Weapon);
        else
            SpawnBoltExp(Weapon);
        SetSuicide(Weapon);
        break;

    case BOLT_THINMAN_R4:
        damage = GetDamage(SpriteNum, Weapon, DMG_GRENADE_EXP);

        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, ANG2PLAYER(u->PlayerP, wp));
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorDamageSlide(SpriteNum, damage, ANG2SPRITE(sp, wp));
            ActorChooseDeath(SpriteNum, Weapon);
        }

        SpawnBunnyExp(Weapon);
        //InitBloodSpray(Weapon,TRUE,-1);
        //InitBloodSpray(Weapon,TRUE,-1);
        //InitBloodSpray(Weapon,TRUE,-1);
        SetSuicide(Weapon);
        break;

    case SUMO_RUN_R0:
        damage = GetDamage(SpriteNum, Weapon, DMG_FLASHBOMB);

        damage /= 3;
        if (u->sop_parent)
        {
            if (TEST(u->sop_parent->flags, SOBJ_DIE_HARD))
                break;
            SopDamage(u->sop_parent, damage);
            SopCheckKill(u->sop_parent);
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, ANG2PLAYER(u->PlayerP, wp));
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorDamageSlide(SpriteNum, damage, ANG2SPRITE(sp, wp));
            ActorChooseDeath(SpriteNum, Weapon);
        }

        break;

    case BOLT_EXP:
        damage = GetDamage(SpriteNum, Weapon, DMG_BOLT_EXP);
//      //DSPRINTF(ds,"Damage Bolt: %d\n",damage);
//      MONO_PRINT(ds);
        if (u->sop_parent)
        {
            if (TEST(u->sop_parent->flags, SOBJ_DIE_HARD))
                break;
            SopDamage(u->sop_parent, damage);
            SopCheckKill(u->sop_parent);
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, ANG2PLAYER(u->PlayerP, wp));
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorDamageSlide(SpriteNum, damage, ANG2SPRITE(sp, wp));
            ActorChooseDeath(SpriteNum, Weapon);
        }

        break;

    case BLOOD_WORM:

        // Don't hurt blood worm zombies!
        if (u->ID == ZOMBIE_RUN_R0)
            break;

        damage = GetDamage(SpriteNum, Weapon, WPN_HEART);

        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            //PlayerDamageSlide(u->PlayerP, damage, ANG2PLAYER(u->PlayerP, wp));
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                if (PlayerCheckDeath(u->PlayerP, Weapon))
                {
                    // degrade blood worm life
                    wu->Counter3 += (4*120)/MISSILEMOVETICS;
                }
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorDamageSlide(SpriteNum, damage, ANG2SPRITE(sp, wp));
            ActorChooseDeath(SpriteNum, Weapon);
        }

        // degrade blood worm life
        wu->Counter3 += (2*120)/MISSILEMOVETICS;

        break;


    case TANK_SHELL_EXP:
        damage = GetDamage(SpriteNum, Weapon, DMG_TANK_SHELL_EXP);
        if (u->sop_parent)
        {
            if (TEST(u->sop_parent->flags, SOBJ_DIE_HARD))
                break;
            SopDamage(u->sop_parent, damage);
            SopCheckKill(u->sop_parent);
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, ANG2PLAYER(u->PlayerP, wp));
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorDamageSlide(SpriteNum, damage, ANG2SPRITE(sp, wp));
            ActorChooseDeath(SpriteNum, Weapon);
        }

        break;

    case MUSHROOM_CLOUD:
    case GRENADE_EXP:
        if (wu->Radius == NUKE_RADIUS) // Special Nuke stuff
            damage = (GetDamage(SpriteNum, Weapon, DMG_NUCLEAR_EXP));
        else
            damage = GetDamage(SpriteNum, Weapon, DMG_GRENADE_EXP);

        //DSPRINTF(ds,"MUSHROOM: damage = %d, wu->radius = %d\n",damage,wu->Radius);
        MONO_PRINT(ds);

        if (u->sop_parent)
        {
            if (TEST(u->sop_parent->flags, SOBJ_DIE_HARD))
                break;
            SopDamage(u->sop_parent, damage);
            SopCheckKill(u->sop_parent);
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, ANG2PLAYER(u->PlayerP, wp));
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            // Don't let it hurt the SUMO
            if (wp->owner >=0 && User[wp->owner] && User[wp->owner]->ID == SUMO_RUN_R0) break;  // JBF: added sanity check for wp->owner and User[wp->owner]
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorDamageSlide(SpriteNum, damage, ANG2SPRITE(sp, wp));
            ActorChooseDeath(SpriteNum, Weapon);
        }

        break;

    case MICRO_EXP:

        damage = GetDamage(SpriteNum, Weapon, DMG_MINE_EXP);

//      //DSPRINTF(ds,"Damage Micro: %d\n",damage);
//      MONO_PRINT(ds);

        if (u->sop_parent)
        {
            if (TEST(u->sop_parent->flags, SOBJ_DIE_HARD))
                break;
            SopDamage(u->sop_parent, damage);
            SopCheckKill(u->sop_parent);
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, ANG2PLAYER(u->PlayerP, wp));
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorDamageSlide(SpriteNum, damage, ANG2SPRITE(sp, wp));
            ActorChooseDeath(SpriteNum, Weapon);
        }
        break;

    case MINE_EXP:
        damage = GetDamage(SpriteNum, Weapon, DMG_MINE_EXP);
        if (wp->owner >= 0 && User[wp->owner] && User[wp->owner]->ID == SERP_RUN_R0)
        {
            damage /= 6;
        }

        if (u->sop_parent)
        {
            if (TEST(u->sop_parent->flags, SOBJ_DIE_HARD))
                break;
            SopDamage(u->sop_parent, damage);
            SopCheckKill(u->sop_parent);
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, ANG2PLAYER(u->PlayerP, wp));
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            if (wp->owner >= 0)
            {
                // Don't let serp skulls hurt the Serpent God
                if (User[wp->owner]->ID == SERP_RUN_R0) break;
                // Don't let it hurt the SUMO
                if (User[wp->owner]->ID == SUMO_RUN_R0) break;
            }
            if (u->ID == TRASHCAN)
                ActorHealth(SpriteNum, -500);
            else
                ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorDamageSlide(SpriteNum, damage, ANG2SPRITE(sp, wp));
            ActorChooseDeath(SpriteNum, Weapon);
        }

        // reset id so no more damage is taken
        wu->ID = 0;
        break;

#if 0
    case MINE_SHRAP:

        damage = GetDamage(SpriteNum, Weapon, DMG_MINE_SHRAP);
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorStdMissile(SpriteNum, Weapon);
            ActorDamageSlide(SpriteNum, damage, wp->ang);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        // reset id so no more damage is taken
        wu->ID = 0;
        break;
#endif

    case NAP_EXP:

        damage = GetDamage(SpriteNum, Weapon, DMG_NAPALM_EXP);

        // Sumo Nap does less
        if (wp->owner >= 0 && User[wp->owner] && User[wp->owner]->ID == SUMO_RUN_R0)
            damage /= 4;

        if (u->sop_parent)
        {
            if (TEST(u->sop_parent->flags, SOBJ_DIE_HARD))
                break;
            SopDamage(u->sop_parent, damage);
            SopCheckKill(u->sop_parent);
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, ANG2PLAYER(u->PlayerP, wp));
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            // Don't let it hurt the SUMO
            if (User[wp->owner]->ID == SUMO_RUN_R0) break;
            ActorHealth(SpriteNum, damage);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        SetSuicide(Weapon);
        break;

    case Vomit1:
    case Vomit2:

        damage = GetDamage(SpriteNum, Weapon, DMG_VOMIT);
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        SetSuicide(Weapon);
        break;

    case COOLG_FIRE:
        damage = GetDamage(SpriteNum, Weapon, DMG_COOLG_FIRE);
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorChooseDeath(SpriteNum, Weapon);
        }

//        u->ID = 0;
        SetSuicide(Weapon);
        break;

    // Skull Exp
    case SKULL_R0:
    case BETTY_R0:

        damage = GetDamage(SpriteNum, Weapon, DMG_SKULL_EXP);

        if (u->sop_parent)
        {
            if (TEST(u->sop_parent->flags, SOBJ_DIE_HARD))
                break;
            SopDamage(u->sop_parent, damage);
            SopCheckKill(u->sop_parent);
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, ANG2PLAYER(u->PlayerP, wp));
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorDamageSlide(SpriteNum, damage, ANG2SPRITE(sp, wp));
            ActorChooseDeath(SpriteNum, Weapon);
        }

        break;

    // Serp ring of skull
    case SKULL_SERP:
        //DoSkullBeginDeath(Weapon);
        break;

    case FIREBALL1:
        ASSERT(SpriteNum >= 0 && Weapon >= 0);
        damage = GetDamage(SpriteNum, Weapon, WPN_HOTHEAD);
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorStdMissile(SpriteNum, Weapon);
            ActorDamageSlide(SpriteNum, damage, wp->ang);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        if (wp->owner >= 0) // For SerpGod Ring
            User[wp->owner]->Counter--;
        SpawnFireballFlames(Weapon, SpriteNum);
        SetSuicide(Weapon);
        break;

    case FIREBALL:
    case GORO_FIREBALL:

        ASSERT(SpriteNum >= 0 && Weapon >= 0);
        damage = GetDamage(SpriteNum, Weapon, DMG_GORO_FIREBALL);
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            PlayerDamageSlide(u->PlayerP, damage, wp->ang);
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
            if (u->PlayerP->Armor)
                PlaySound(DIGI_ARMORHIT,&u->PlayerP->posx,&u->PlayerP->posy,&u->PlayerP->posz,v3df_dontpan|v3df_follow|v3df_doppler);
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorStdMissile(SpriteNum, Weapon);
            ActorDamageSlide(SpriteNum, damage, wp->ang);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        SpawnGoroFireballExp(Weapon);
        SetSuicide(Weapon);
        break;

    case FIREBALL_FLAMES:

        damage = -DamageData[DMG_FIREBALL_FLAMES].damage_lo;

        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        //SpawnFireballFlames(Weapon, SpriteNum);

        break;

    case RADIATION_CLOUD:

        damage = GetDamage(SpriteNum, Weapon, DMG_RADIATION_CLOUD);
//      //DSPRINTF(ds,"Radiation damage = %d\n",damage);
//      MONO_PRINT(ds);

        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                PLAYERp pp = u->PlayerP;

                PlayerSound(DIGI_GASHURT,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow|v3df_doppler,pp);
                PlayerUpdateHealth(u->PlayerP, damage-1000);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            // Don't let it hurt the SUMO
            if (wp->owner >= 0 && User[wp->owner]->ID == SUMO_RUN_R0) break;    // JBF: added owner check
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorChooseDeath(SpriteNum, Weapon);
        }

//        u->ID = 0;
        wu->ID = 0;
        break;

    case PLASMA:

        //damage = GetDamage(SpriteNum, Weapon, WPN_HEART);
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            //PlayerUpdateHealth(u->PlayerP, damage);
            //PlayerCheckDeath(u->PlayerP, Weapon);
        }
        else
        {
            if (u->ID == SKULL_R0 || u->ID == BETTY_R0)
            {
                ActorHealth(SpriteNum, damage);
                ActorStdMissile(SpriteNum, Weapon);
                ActorChooseDeath(SpriteNum, Weapon);
                SetSuicide(Weapon);
                break;
            }
            else if (u->ID == RIPPER_RUN_R0)
            {
                ANIMATOR DoRipperGrow;
                DoRipperGrow(SpriteNum);
                break;
            }

            ActorPainPlasma(SpriteNum);
        }

        InitPlasmaFountain(wp, sp);

        SetSuicide(Weapon);

        break;

    case CALTROPS:

        ASSERT(SpriteNum >= 0 && Weapon >= 0);
        damage = GetDamage(SpriteNum, Weapon, DMG_MINE_SHRAP);
        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            if (PlayerTakeDamage(u->PlayerP, Weapon))
            {
                if (RANDOM_P2(1024<<4)>>4 < 800)
                    PlayerSound(DIGI_STEPONCALTROPS, &sp->x, &sp->y, &sp->z, v3df_follow|v3df_dontpan, u->PlayerP);
                PlayerUpdateHealth(u->PlayerP, damage);
                PlayerCheckDeath(u->PlayerP, Weapon);
            }
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorPain(SpriteNum);
            ActorStdMissile(SpriteNum, Weapon);
            ActorChooseDeath(SpriteNum, Weapon);
        }

        SetSuicide(Weapon);
        break;

#if 0
    case PLASMA_FOUNTAIN:

        //damage = GetDamage(SpriteNum, Weapon, WPN_HEART);
        damage = -300;

        if (u->sop_parent)
        {
            break;
        }
        else if (u->PlayerP)
        {
            PlayerUpdateHealth(u->PlayerP, damage);
#if 0
            if (PlayerCheckDeath(u->PlayerP, Weapon))
            {
                // Spawn a couple of rippers
                RipperHatch(Weapon);
            }
#endif
        }
        else
        {
            ActorHealth(SpriteNum, damage);
            ActorStdMissile(SpriteNum, Weapon);
#if 0
            if (ActorChooseDeath(SpriteNum, Weapon))
            {
                RipperHatch(Weapon);
            }
#endif
        }

        break;
#endif

    }

    // If player take alot of damage, make him yell
    if (u && u->PlayerP)
    {
        if (damage <= -40 && RANDOM_RANGE(1000) > 700)
            PlayerSound(DIGI_SONOFABITCH, &sp->x, &sp->y, &sp->z, v3df_dontpan|v3df_follow, u->PlayerP);
        else if (damage <= -40 && RANDOM_RANGE(1000) > 700)
            PlayerSound(DIGI_PAINFORWEAK, &sp->x, &sp->y, &sp->z, v3df_dontpan|v3df_follow, u->PlayerP);
        else if (damage <= -10)
            PlayerSound(PlayerPainVocs[RANDOM_RANGE(MAX_PAIN)], &sp->x, &sp->y, &sp->z, v3df_dontpan|v3df_follow, u->PlayerP);
    }

    return 0;
}

#if 1
// Select death text based on ID
char *DeathString(short SpriteNum)
{
    USERp ku = User[SpriteNum];

    switch (ku->ID)
    {
    case NINJA_RUN_R0:
        return " ";
    case ZOMBIE_RUN_R0:
        return "Zombie";
    case BLOOD_WORM:
        return "Blood Worm";
    case SKEL_RUN_R0:
        return "Skeletor Priest";
    case COOLG_RUN_R0:
        return "Coolie Ghost";
    case GORO_RUN_R0:
        return "Guardian";
    case HORNET_RUN_R0:
        return "Hornet";
    case RIPPER_RUN_R0:
        return "Ripper Hatchling";
    case RIPPER2_RUN_R0:
        return "Ripper";
    case BUNNY_RUN_R0:
        return "Killer Rabbit";
    case SERP_RUN_R0:
        return "Serpent god";
    case GIRLNINJA_RUN_R0:
        return "Girl Ninja";
    case BLADE1:
    case BLADE2:
    case BLADE3:
    case 5011:
        return "blade";
    case STAR1:
        if (useDarts) return "dart";
        else return "shuriken";
    case CROSSBOLT:
        return "crossbow bolt";
    case SPEAR_R0:
        return "spear";
    case LAVA_BOULDER:
    case LAVA_SHARD:
        return "lava boulder";
    case UZI_SMOKE:
        return "Uzi";
    case UZI_SMOKE+2:
        return "Evil Ninja Uzi";
    case SHOTGUN_SMOKE:
        return "shotgun";
    case MIRV_METEOR:
    case SERP_METEOR:
        return "meteor";
    case BOLT_THINMAN_R0:
        return "rocket";
    case BOLT_THINMAN_R1:
        return "rail gun";
    case BOLT_THINMAN_R2:
        return "enemy rocket";
    case BOLT_THINMAN_R4:  // BunnyRocket
        return "bunny rocket";
    case BOLT_EXP:
        return "explosion";
    case TANK_SHELL_EXP:
        return "tank shell";
    case MUSHROOM_CLOUD:
        return "nuclear bomb";
    case GRENADE_EXP:
        return "40mm grenade";
    case MICRO_EXP:
        return "micro missile";
    case MINE_EXP:
        //case MINE_SHRAP:
        return "sticky bomb";
    case NAP_EXP:
        return "napalm";
    case Vomit1:
    case Vomit2:
        return "vomit";
    case COOLG_FIRE:
        return "Coolie Ghost phlem";
    case SKULL_R0:
        return "Accursed Head";
    case BETTY_R0:
        return "Bouncing Betty";
    case SKULL_SERP:
        return "Serpent god Protector";
    case FIREBALL1:
    case FIREBALL:
    case GORO_FIREBALL:
    case FIREBALL_FLAMES:
        return "flames";
    case RADIATION_CLOUD:
        return "radiation";
    case CALTROPS:
        return "caltrops";
    }
    return "";
}
#endif

int
DoDamageTest(short Weapon)
{
    SPRITEp wp = &sprite[Weapon];
    USERp wu = User[Weapon];

    USERp u;
    SPRITEp sp;
    short i, nexti;
    unsigned stat;
    int dist, tx, ty;
    int tmin;

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            sp = &sprite[i];
            u = User[i];


            DISTANCE(sp->x, sp->y, wp->x, wp->y, dist, tx, ty, tmin);
            if ((unsigned)dist > wu->Radius + u->Radius)
                continue;

            if (sp == wp)
                continue;

            if (!TEST(sp->cstat, CSTAT_SPRITE_BLOCK))
                continue;

            // !JIM! Put in a cansee so that you don't take damage through walls and such
            // For speed's sake, try limiting check only to radius weapons!
            if (wu->Radius > 200)
            {
                if (!FAFcansee(sp->x,sp->y, SPRITEp_UPPER(sp), sp->sectnum,wp->x,wp->y,wp->z,wp->sectnum))
                    continue;
            }

            if (wp->owner != i && SpriteOverlap(Weapon, i))
            {
                DoDamage(i, Weapon);
            }
        }
    }

    return 0;
}

int
DoHitscanDamage(short Weapon, short hit_sprite)
{
    SPRITEp wp = &sprite[Weapon];
    USERp wu = User[Weapon];
    unsigned stat;

    // this routine needs some sort of sprite generated from the hitscan
    // such as a smoke or spark sprite - reason is because of DoDamage()

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        if (sprite[hit_sprite].statnum == StatDamageList[stat])
        {
            DoDamage(hit_sprite, Weapon);
            break;
        }
    }

    return 0;
}

int
DoFlamesDamageTest(short Weapon)
{
    SPRITEp wp = &sprite[Weapon];
    USERp wu = User[Weapon];

    USERp u;
    SPRITEp sp;
    short i, nexti;
    unsigned stat;
    int dist, tx, ty;
    int tmin;

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            sp = &sprite[i];
            u = User[i];

            switch (u->ID)
            {
            case TRASHCAN:
            case PACHINKO1:
            case PACHINKO2:
            case PACHINKO3:
            case PACHINKO4:
            case 623:
                continue;
            }

            DISTANCE(sp->x, sp->y, wp->x, wp->y, dist, tx, ty, tmin);

//          //DSPRINTF(ds,"radius = %ld, distance = %ld",wu->Radius+u->Radius,dist);
//          MONO_PRINT(ds);

            if ((unsigned)dist > wu->Radius + u->Radius)
                continue;

            if (sp == wp)
                continue;

            if (!TEST(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN))
                continue;

            if (TEST(wp->cstat, CSTAT_SPRITE_INVISIBLE))
                continue;

            if (wu->Radius > 200) // Note: No weaps have bigger radius than 200 cept explosion stuff
            {
                if (FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,wp->x,wp->y,SPRITEp_MID(wp),wp->sectnum))
                {
                    DoDamage(i, Weapon);
                }
            }
            else if (SpriteOverlap(Weapon, i))
            {
                DoDamage(i, Weapon);
            }

        }
    }

    return 0;
}

short PrevWall(short wall_num)
{
    short start_wall,prev_wall;

    start_wall = wall_num;

    do
    {
        prev_wall = wall_num;
        wall_num = wall[wall_num].point2;
    }
    while (wall_num != start_wall);

    return prev_wall;
}


short StatBreakList[] =
{
    STAT_DEFAULT,
    STAT_BREAKABLE,
    STAT_NO_STATE,
    STAT_DEAD_ACTOR,
};

void TraverseBreakableWalls(short start_sect, int x, int y, int z, short ang, int radius)
{
    int WallBreakPosition(short hit_wall, short *sectnum, int *x, int *y, int *z, short *ang);
    int i, j, k;
    short sectlist[MAXSECTORS]; // !JIM! Frank, 512 was not big enough for $dozer, was asserting out!
    short sectlistplc, sectlistend, sect, startwall, endwall, nextsector;
    int xmid,ymid;
    int dist;
    short break_count;

    short sectnum,wall_ang;
    int hit_x,hit_y,hit_z;

    sectlist[0] = start_sect;
    sectlistplc = 0; sectlistend = 1;

    // limit radius
    if (radius > 2000)
        radius = 2000;

    break_count = 0;
    while (sectlistplc < sectlistend)
    {
        sect = sectlist[sectlistplc++];

        ASSERT(sectlistplc < SIZ(sectlist));

        startwall = sector[sect].wallptr;
        endwall = startwall + sector[sect].wallnum;

        for (j = startwall; j < endwall - 1; j++)
        {
            // see if this wall should be broken
            if (wall[j].lotag == TAG_WALL_BREAK)
            {
                // find midpoint
                xmid = DIV2(wall[j].x + wall[j+1].x);
                ymid = DIV2(wall[j].y + wall[j+1].y);

                // don't need to go further if wall is too far out

                dist = Distance(xmid, ymid, x, y);
                if (dist > radius)
                    continue;

                if (WallBreakPosition(j, &sectnum, &hit_x, &hit_y, &hit_z, &wall_ang))
                {
                    if (hit_x != MAXLONG && sectnum >= 0 && FAFcansee(x, y, z, start_sect, hit_x, hit_y, hit_z, sectnum))
                    {
                        //HitBreakWall(&wall[j], x, y, z, ang, 0);
                        HitBreakWall(&wall[j], MAXLONG, MAXLONG, MAXLONG, ang, 0);

                        break_count++;
                        if (break_count > 4)
                            return;
                    }
                }
            }

            nextsector = wall[j].nextsector;

            if (nextsector < 0)
                continue;

            // make sure its not on the list
            for (k = sectlistend - 1; k >= 0; k--)
            {
                if (sectlist[k] == nextsector)
                    break;
            }

            // if its not on the list add it to the end
            if (k < 0)
            {
                sectlist[sectlistend++] = nextsector;
                ASSERT(sectlistend < SIZ(sectlist));
            }
        }

    }
}


int DoExpDamageTest(short Weapon)
{
    SPRITEp wp = &sprite[Weapon];
    USERp wu = User[Weapon];

    USERp u;
    SPRITEp sp;
    short i, nexti, stat;
    int dist, tx, ty;
    int tmin;
    int max_stat;
    short break_count;

    SPRITEp found_sp = NULL;
    int found_dist = 999999;
    int DoWallMoveMatch(short match);

    // crack sprites
    if (wu->ID != MUSHROOM_CLOUD)
        WeaponExplodeSectorInRange(Weapon);

    // Just like DoDamageTest() except that it doesn't care about the owner

    max_stat = SIZ(StatDamageList);
    // don't check for mines if the weapon is a mine
    if (wp->statnum == STAT_MINE_STUCK)
        max_stat--;

    for (stat = 0; stat < max_stat; stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            sp = &sprite[i];
            u = User[i];

            DISTANCE(sp->x, sp->y, wp->x, wp->y, dist, tx, ty, tmin);

            if ((unsigned)dist > wu->Radius + u->Radius)
                continue;

            if (sp == wp)
                continue;

            if (StatDamageList[stat] == STAT_SO_SP_CHILD)
            {
                DoDamage(i, Weapon);
            }
            else
            {
                int zdist=0;

                if ((unsigned)FindDistance3D(sp->x - wp->x, sp->y - wp->y, (sp->z - wp->z)>>4) > wu->Radius + u->Radius)
                    continue;

                // added hitscan block because mines no long clip against actors/players
                if (!TEST(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN))
                    continue;

                // Second parameter MUST have blocking bits set or cansee won't work
                // added second check for FAF water - hitscans were hitting ceiling
                if (!FAFcansee(wp->x, wp->y, wp->z, wp->sectnum, sp->x, sp->y, SPRITEp_UPPER(sp), sp->sectnum) &&
                    !FAFcansee(wp->x, wp->y, wp->z, wp->sectnum, sp->x, sp->y, SPRITEp_LOWER(sp), sp->sectnum))
                    continue;

                DoDamage(i, Weapon);
            }
        }
    }

    if (wu->ID == MUSHROOM_CLOUD) return 0;   // Central Nuke doesn't break stuff
    // Only secondaries do that

    TraverseBreakableWalls(wp->sectnum, wp->x, wp->y, wp->z, wp->ang, wu->Radius);

    break_count = 0;
    max_stat = SIZ(StatBreakList);
    // Breakable stuff
    for (stat = 0; stat < max_stat; stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatBreakList[stat]], i, nexti)
        {
            sp = &sprite[i];
            u = User[i];

            DISTANCE(sp->x, sp->y, wp->x, wp->y, dist, tx, ty, tmin);
            if ((unsigned)dist > wu->Radius)
                continue;

            dist = FindDistance3D(sp->x - wp->x, sp->y - wp->y, (SPRITEp_MID(sp) - wp->z)>>4);
            if ((unsigned)dist > wu->Radius)
                continue;

            if (!FAFcansee(sp->x, sp->y, SPRITEp_MID(sp), sp->sectnum, wp->x, wp->y, wp->z, wp->sectnum))
                continue;

            if (TEST(sp->extra, SPRX_BREAKABLE))
            {
                HitBreakSprite(i, wu->ID);
                break_count++;
                if (break_count > 6)
                    break;
            }
        }
    }

    if (wu->ID == BLOOD_WORM)
        return 0;

    // wall damaging
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_WALL_MOVE], i, nexti)
    {
        sp = &sprite[i];

        DISTANCE(sp->x, sp->y, wp->x, wp->y, dist, tx, ty, tmin);
        if ((unsigned)dist > wu->Radius/4)
            continue;

        if (TEST_BOOL1(sp))
            continue;

        if (!CanSeeWallMove(wp, SP_TAG2(sp)))
            continue;

        if (dist < found_dist)
        {
            found_dist = dist;
            found_sp = sp;
        }
    }



    if (found_sp)
    {
        if (SP_TAG2(found_sp) == 0)
        {
            // just do one
            DoWallMove(found_sp);
        }
        else
        {
            SWBOOL once = FALSE;

            if (DoWallMoveMatch(SP_TAG2(found_sp)))
            {
                DoSpawnSpotsForDamage(SP_TAG2(found_sp));
            }
        }
    }

    return 0;
}


int DoMineExpMine(short Weapon)
{
    SPRITEp wp = &sprite[Weapon];
    USERp wu = User[Weapon];

    USERp u;
    SPRITEp sp;
    short i, nexti, stat;
    int dist, tx, ty;
    int tmin;
    int zdist;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_MINE_STUCK], i, nexti)
    {
        sp = &sprite[i];
        u = User[i];

        DISTANCE(sp->x, sp->y, wp->x, wp->y, dist, tx, ty, tmin);
        if ((unsigned)dist > wu->Radius + u->Radius)
            continue;

        if (sp == wp)
            continue;

        //if (!TEST(sp->cstat, CSTAT_SPRITE_BLOCK))
        //    continue;
        if (!TEST(sp->cstat, CSTAT_SPRITE_BLOCK_HITSCAN))
            continue;

        // Explosions are spherical, not planes, so let's check that way, well cylindrical at least.
        zdist = abs(sp->z - wp->z)>>4;
        if (SpriteOverlap(Weapon, i) || (unsigned)zdist < wu->Radius + u->Radius)
        {
            DoDamage(i, Weapon);
            // only explode one mine at a time
            break;
        }
    }

    return 0;
}

int
DoStar(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int32_t enemy, next_enemy, dax, day, daz, nextj;
    short NewWeapon, pnum;
    USERp su;
    PLAYERp pp;
    short ret;
    int vel;

//    if (!TEST(u->Flags, SPR_BOUNCE))
//        DoBlurExtend(Weapon, 0, 2);

    if (TEST(u->Flags, SPR_UNDERWATER))
    {
        u->motion_blur_num = 0;
        ScaleSpriteVector(Weapon, 54000);

        vel = ksqrt(SQ(u->xchange) + SQ(u->ychange));

        if (vel > 100)
        {
            if ((RANDOM_P2(1024 << 4) >> 4) < 128)
                SpawnBubble(Weapon);
        }

        sp->z += 128 * MISSILEMOVETICS;

        DoActorZrange(Weapon);
        MissileWaterAdjust(Weapon);

        if (sp->z > u->loz)
        {
            KillSprite(Weapon);
            return TRUE;
        }
    }
    else
    {
        vel = ksqrt(SQ(u->xchange) + SQ(u->ychange));


        if (vel < 800)
        {
            u->Counter += 50;
            u->zchange += u->Counter;
        }
    }

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);
    //DoDamageTest(Weapon);

    if (u->ret && !TEST(u->Flags, SPR_UNDERWATER))
    {
        switch (TEST(u->ret, HIT_MASK))
        {
        case HIT_PLAX_WALL:
            break;

        case HIT_WALL:
        {
            short hit_wall,nw,wall_ang,dang;
            WALLp wph;

            hit_wall = NORM_WALL(u->ret);
            wph = &wall[hit_wall];

#define STAR_STICK_RNUM 400
#define STAR_BOUNCE_RNUM 600

            if (wph->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(&wall[hit_wall], sp->x, sp->y, sp->z, sp->ang, u->ID);
                u->ret = 0;
                break;
            }

            // special case with MissileSetPos - don't queue star
            // from this routine
            if (TEST(u->Flags, SPR_SET_POS_DONT_KILL))
                break;

            // chance of sticking
            if (!TEST(u->Flags, SPR_BOUNCE) && RANDOM_P2(1024) < STAR_STICK_RNUM)
            {
                u->motion_blur_num = 0;
                ChangeState(Weapon, s_StarStuck);
                sp->xrepeat -= 16;
                sp->yrepeat -= 16;
                RESET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
                //RESET(sp->cstat, CSTAT_SPRITE_BLOCK_HITSCAN);
                //SET(sp->cstat, CSTAT_SPRITE_BLOCK);
                sp->clipdist = 16L >> 2;
                u->ceiling_dist = Z(2);
                u->floor_dist = Z(2);
                // treat this just like a KillSprite but don't kill
                QueueStar(Weapon);
                return 0;
            }

            // chance of bouncing
            if (RANDOM_P2(1024) < STAR_BOUNCE_RNUM)
                break;

            nw = wall[hit_wall].point2;
            wall_ang = NORM_ANGLE(getangle(wall[nw].x - wph->x, wall[nw].y - wph->y)+512);

            WallBounce(Weapon, wall_ang);
            ScaleSpriteVector(Weapon, 36000);
            SET(u->Flags, SPR_BOUNCE);
            u->motion_blur_num = 0;
            u->ret = 0;
            break;
        }

        case HIT_SECTOR:
        {
            SWBOOL did_hit_wall;
            short hit_sect = NORM_SECTOR(u->ret);

            if (sp->z > DIV2(u->hiz + u->loz))
            {
                if (SectUser[hit_sect] && SectUser[hit_sect]->depth > 0)
                {
                    SpawnSplash(Weapon);
                    KillSprite(Weapon);
                    return TRUE;
                    // hit water - will be taken care of in WeaponMoveHit
                    //break;
                }
            }

            if (u->lo_sp)
                if (u->lo_sp->lotag == TAG_SPRITE_HIT_MATCH)
                    break;
            if (u->hi_sp)
                if (u->hi_sp->lotag == TAG_SPRITE_HIT_MATCH)
                    break;

            ScaleSpriteVector(Weapon, 58000);

            vel = ksqrt(SQ(u->xchange) + SQ(u->ychange));

            if (vel < 500)
                break; // will be killed below - u->ret != 0

            // 32000 to 96000
            u->xchange = mulscale16(u->xchange, 64000 + (RANDOM_RANGE(64000) - 32000));
            u->ychange = mulscale16(u->ychange, 64000 + (RANDOM_RANGE(64000) - 32000));

            if (sp->z > DIV2(u->hiz + u->loz))
                u->zchange = mulscale16(u->zchange, 50000); // floor
            else
                u->zchange = mulscale16(u->zchange, 40000); // ceiling

            if (SlopeBounce(Weapon, &did_hit_wall))
            {
                if (did_hit_wall)
                {
                    // chance of sticking
                    if (RANDOM_P2(1024) < STAR_STICK_RNUM)
                        break;

                    // chance of bouncing
                    if (RANDOM_P2(1024) < STAR_BOUNCE_RNUM)
                        break;

                    SET(u->Flags, SPR_BOUNCE);
                    u->motion_blur_num = 0;
                    u->ret = 0;

                }
                else
                {
                    // hit a sloped sector < 45 degrees
                    SET(u->Flags, SPR_BOUNCE);
                    u->motion_blur_num = 0;
                    u->ret = 0;
                }

                // BREAK HERE - LOOOK !!!!!!!!!!!!!!!!!!!!!!!!
                break; // hit a slope
            }

            SET(u->Flags, SPR_BOUNCE);
            u->motion_blur_num = 0;
            u->ret = 0;
            u->zchange = -u->zchange;

            // 32000 to 96000
            u->xchange = mulscale16(u->xchange, 64000 + (RANDOM_RANGE(64000) - 32000));
            u->ychange = mulscale16(u->ychange, 64000 + (RANDOM_RANGE(64000) - 32000));
            if (sp->z > DIV2(u->hiz + u->loz))
                u->zchange = mulscale16(u->zchange, 50000); // floor
            else
                u->zchange = mulscale16(u->zchange, 40000); // ceiling

            break;
        }
        }
    }

    if (u->ret)
    {
        short hit_sprite = NORM_SPRITE(u->ret);
        if (hit_sprite != -1)
        {
            su = User[hit_sprite];
            if (su && (su->ID == TRASHCAN || su->ID == ZILLA_RUN_R0))   // JBF: added null test
                PlaySound(DIGI_STARCLINK, &sp->x, &sp->y, &sp->z, v3df_none);
        }

        if (TEST(u->ret, HIT_MASK) != HIT_SPRITE) // Don't clank on sprites
            PlaySound(DIGI_STARCLINK, &sp->x, &sp->y, &sp->z, v3df_none);

        if (WeaponMoveHit(Weapon))
        {
            KillSprite(Weapon);
            return TRUE;
        }
    }


    return FALSE;

}

int
DoCrossBolt(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int32_t dax, day, daz;
    short NewWeapon;

    int offset;

    u = User[Weapon];

    ASSERT(Weapon >= 0);

    DoBlurExtend(Weapon, 0, 2);

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, Z(16), Z(16), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);

    if (u->ret)
    {
        if (WeaponMoveHit(Weapon))
        {
            switch (TEST(u->ret, HIT_MASK))
            {
            case HIT_SPRITE:
            {
                break;
            }
            }

            KillSprite(Weapon);

            return TRUE;
        }
    }

    return FALSE;

}


int
DoPlasmaDone(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];

    sp->xrepeat += u->Counter;
    sp->yrepeat -= 4;
    u->Counter += 2;

    if (sp->yrepeat < 6)
    {
        KillSprite(Weapon);
        return 0;
    }

    return 0;
}

int PickEnemyTarget(SPRITEp sp, short aware_range)
{
    TARGET_SORTp ts;

    DoPickTarget(sp, aware_range, FALSE);

    for (ts = TargetSort; ts < &TargetSort[TargetSortCount]; ts++)
    {
        if (ts->sprite_num >= 0)
        {
            if (ts->sprite_num == sp->owner || sprite[ts->sprite_num].owner == sp->owner)
                continue;

            return ts->sprite_num;
        }
    }

    return -1;
}

int
MissileSeek(int16_t Weapon, int16_t delay_tics, int16_t aware_range, int16_t dang_shift, int16_t turn_limit, int16_t z_limit)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];

    int dist;
    int zh;
    short ang2tgt, delta_ang, ang_adj;
    int ozvel;

    SPRITEp hp;

    if (u->WaitTics <= delay_tics)
        u->WaitTics += MISSILEMOVETICS;

    if (u->WpnGoal == -1)
    {
        if (u->WaitTics > delay_tics)
        {
            short hit_sprite;

            if (TEST(u->Flags2, SPR2_DONT_TARGET_OWNER))
            {
                if ((hit_sprite = PickEnemyTarget(sp, aware_range)) != -1)
                {
                    SPRITEp hp = &sprite[hit_sprite];
                    USERp hu = User[hit_sprite];

                    u->WpnGoal = hit_sprite;
                    SET(hu->Flags, SPR_TARGETED);
                    SET(hu->Flags, SPR_ATTACKED);
                }
            }
            else if ((hit_sprite = DoPickTarget(sp, aware_range, FALSE)) != -1)
            {
                SPRITEp hp = &sprite[hit_sprite];
                USERp hu = User[hit_sprite];

                u->WpnGoal = hit_sprite;
                SET(hu->Flags, SPR_TARGETED);
                SET(hu->Flags, SPR_ATTACKED);
            }
        }
    }

    if (u->WpnGoal >= 0)
    {
        hp = &sprite[User[Weapon]->WpnGoal];

        // move to correct angle
        ang2tgt = getangle(hp->x - sp->x, hp->y - sp->y);

        delta_ang = GetDeltaAngle(sp->ang, ang2tgt);

        if (labs(delta_ang) > 32)
        {
            if (delta_ang > 0)
                delta_ang = 32;
            else
                delta_ang = -32;
        }

        sp->ang -= delta_ang;

        zh = SPRITEp_TOS(hp) + DIV4(SPRITEp_SIZE_Z(hp));

        delta_ang = (zh - sp->z)>>1;

        if (labs(delta_ang) > Z(16))
        {
            if (delta_ang > 0)
                delta_ang = Z(16);
            else
                delta_ang = -Z(16);
        }

        sp->zvel = delta_ang;

        u->xchange = MOVEx(sp->xvel, sp->ang);
        u->ychange = MOVEy(sp->xvel, sp->ang);
        u->zchange = sp->zvel;
    }
    return 0;
}

// combination of vector manipulation
int
ComboMissileSeek(int16_t Weapon, int16_t delay_tics, int16_t aware_range, int16_t dang_shift, int16_t turn_limit, int16_t z_limit)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];

    int dist;
    int zh;
    short ang2tgt, delta_ang, ang_adj;
    int ozvel;

    SPRITEp hp;

    if (u->WaitTics <= delay_tics)
        u->WaitTics += MISSILEMOVETICS;

    if (u->WpnGoal == -1)
    {
        if (u->WaitTics > delay_tics)
        {
            short hit_sprite;

            if ((hit_sprite = DoPickTarget(sp, aware_range, FALSE)) != -1)
            {
                SPRITEp hp = &sprite[hit_sprite];
                USERp hu = User[hit_sprite];

                u->WpnGoal = hit_sprite;
                SET(hu->Flags, SPR_TARGETED);
                SET(hu->Flags, SPR_ATTACKED);
            }
        }
    }

    if (u->WpnGoal >= 0)
    {
        int oz;
        hp = &sprite[User[Weapon]->WpnGoal];

        // move to correct angle
        ang2tgt = getangle(hp->x - sp->x, hp->y - sp->y);

        delta_ang = GetDeltaAngle(sp->ang, ang2tgt);

        if (labs(delta_ang) > 32)
        {
            if (delta_ang > 0)
                delta_ang = 32;
            else
                delta_ang = -32;
        }

        sp->ang -= delta_ang;

        u->xchange = MOVEx(sp->xvel, sp->ang);
        u->ychange = MOVEy(sp->xvel, sp->ang);

        zh = SPRITEp_TOS(hp) + DIV4(SPRITEp_SIZE_Z(hp));

        dist = ksqrt(SQ(sp->x - hp->x) + SQ(sp->y - hp->y) + (SQ(sp->z - zh)>>8));

        oz = u->zchange;

        u->zchange = scale(sp->xvel, zh - sp->z, dist);
        u->zchange = (u->zchange + oz*15)/16;
    }
    return 0;
}

// completely vector manipulation
int
VectorMissileSeek(int16_t Weapon, int16_t delay_tics, int16_t turn_speed, int16_t aware_range1, int16_t aware_range2)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];

    int dist;
    int zh;
    short ang2tgt, delta_ang, ang_adj;
    int ozchange;
    int zdiff;

    SPRITEp hp;

    if (u->WaitTics <= delay_tics)
        u->WaitTics += MISSILEMOVETICS;

    if (u->WpnGoal == -1)
    {
        if (u->WaitTics > delay_tics)
        {
            short hit_sprite;

            if (TEST(u->Flags2, SPR2_DONT_TARGET_OWNER))
            {
                if ((hit_sprite = PickEnemyTarget(sp, aware_range1)) != -1)
                {
                    SPRITEp hp = &sprite[hit_sprite];
                    USERp hu = User[hit_sprite];

                    u->WpnGoal = hit_sprite;
                    SET(hu->Flags, SPR_TARGETED);
                    SET(hu->Flags, SPR_ATTACKED);
                }
                else if ((hit_sprite = PickEnemyTarget(sp, aware_range2)) != -1)
                {
                    SPRITEp hp = &sprite[hit_sprite];
                    USERp hu = User[hit_sprite];

                    u->WpnGoal = hit_sprite;
                    SET(hu->Flags, SPR_TARGETED);
                    SET(hu->Flags, SPR_ATTACKED);
                }
            }
            else
            {
                if ((hit_sprite = DoPickTarget(sp, aware_range1, FALSE)) != -1)
                {
                    SPRITEp hp = &sprite[hit_sprite];
                    USERp hu = User[hit_sprite];

                    u->WpnGoal = hit_sprite;
                    SET(hu->Flags, SPR_TARGETED);
                    SET(hu->Flags, SPR_ATTACKED);
                }
                else if ((hit_sprite = DoPickTarget(sp, aware_range2, FALSE)) != -1)
                {
                    SPRITEp hp = &sprite[hit_sprite];
                    USERp hu = User[hit_sprite];

                    u->WpnGoal = hit_sprite;
                    SET(hu->Flags, SPR_TARGETED);
                    SET(hu->Flags, SPR_ATTACKED);
                }
            }
        }
    }

    if (u->WpnGoal >= 0)
    {
        int ox,oy,oz;
        hp = &sprite[User[Weapon]->WpnGoal];

        if (!hp) return 0;

        zh = SPRITEp_TOS(hp) + DIV4(SPRITEp_SIZE_Z(hp));

        dist = ksqrt(SQ(sp->x - hp->x) + SQ(sp->y - hp->y) + (SQ(sp->z - zh)>>8));

        ox = u->xchange;
        oy = u->ychange;
        oz = u->zchange;

        u->xchange = scale(sp->xvel, hp->x - sp->x, dist);
        u->ychange = scale(sp->xvel, hp->y - sp->y, dist);
        u->zchange = scale(sp->xvel, zh - sp->z, dist);

        // the large turn_speed is the slower the turn

        u->xchange = (u->xchange + ox*(turn_speed-1))/turn_speed;
        u->ychange = (u->ychange + oy*(turn_speed-1))/turn_speed;
        u->zchange = (u->zchange + oz*(turn_speed-1))/turn_speed;

        sp->ang = getangle(u->xchange, u->ychange);
    }

    return 0;
}

// completely vector manipulation
int
VectorWormSeek(int16_t Weapon, int16_t delay_tics, int16_t aware_range1, int16_t aware_range2)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];

    int dist;
    int zh;
    short ang2tgt, delta_ang, ang_adj;
    int ozchange;
    int zdiff;

    SPRITEp hp;

    if (u->WaitTics <= delay_tics)
        u->WaitTics += MISSILEMOVETICS;

    if (u->WpnGoal == -1)
    {
        if (u->WaitTics > delay_tics)
        {
            short hit_sprite;

            if ((hit_sprite = DoPickTarget(sp, aware_range1, FALSE)) != -1)
            {
                SPRITEp hp = &sprite[hit_sprite];
                USERp hu = User[hit_sprite];

                u->WpnGoal = hit_sprite;
                SET(hu->Flags, SPR_TARGETED);
                SET(hu->Flags, SPR_ATTACKED);
            }
            else if ((hit_sprite = DoPickTarget(sp, aware_range2, FALSE)) != -1)
            {
                SPRITEp hp = &sprite[hit_sprite];
                USERp hu = User[hit_sprite];

                u->WpnGoal = hit_sprite;
                SET(hu->Flags, SPR_TARGETED);
                SET(hu->Flags, SPR_ATTACKED);
            }
        }
    }

    if (u->WpnGoal >= 0)
    {
        int ox,oy,oz;
        hp = &sprite[User[Weapon]->WpnGoal];

        zh = SPRITEp_TOS(hp) + DIV4(SPRITEp_SIZE_Z(hp));

        dist = ksqrt(SQ(sp->x - hp->x) + SQ(sp->y - hp->y) + (SQ(sp->z - zh)>>8));

        ox = u->xchange;
        oy = u->ychange;
        oz = u->zchange;

        u->xchange = scale(sp->xvel, hp->x - sp->x, dist);
        u->ychange = scale(sp->xvel, hp->y - sp->y, dist);
        u->zchange = scale(sp->xvel, zh - sp->z, dist);

        u->xchange = (u->xchange + ox*7)/8;
        u->ychange = (u->ychange + oy*7)/8;
        u->zchange = (u->zchange + oz*7)/8;

        sp->ang = getangle(u->xchange, u->ychange);
    }

    return 0;
}

int
DoBlurExtend(int16_t Weapon, int16_t interval, int16_t blur_num)
{
    USERp u = User[Weapon];

    if (u->motion_blur_num >= blur_num)
        return 0;

    u->Counter2++;
    if (u->Counter2 > interval)
        u->Counter2 = 0;

    if (!u->Counter2)
    {
        u->motion_blur_num++;
        if (u->motion_blur_num > blur_num)
            u->motion_blur_num = blur_num;
    }

    return 0;
}

int
InitPlasmaFountain(SPRITEp wp, SPRITEp sp)
{
    USERp u = User[sp - sprite];
    SPRITEp np;
    USERp nu;
    short SpriteNum;

    SpriteNum = SpawnSprite(STAT_MISSILE, PLASMA_FOUNTAIN, s_PlasmaFountain, sp->sectnum,
                            sp->x, sp->y, SPRITEp_BOS(sp), sp->ang, 0);

    np = &sprite[SpriteNum];
    nu = User[SpriteNum];

    np->shade = -40;
    if (wp)
        SetOwner(wp->owner, SpriteNum);
    SetAttach(sp - sprite, SpriteNum);
    np->yrepeat = 0;
    np->clipdist = 8>>2;

    // start off on a random frame
    //nu->WaitTics = PLASMA_FOUNTAIN_TIME;
    nu->WaitTics = 120+60;
    nu->Radius = 50;
    return 0;
}

int
DoPlasmaFountain(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    SPRITEp ap;
    USERp u = User[Weapon];
    short bak_cstat;

    // if no owner then die
    if (u->Attach < 0)
    {
        KillSprite(Weapon);
        return 0;
    }
    else
    {
        ap = &sprite[u->Attach];

        // move with sprite
        setspritez(Weapon, (vec3_t *)ap);
        sp->ang = ap->ang;

        u->Counter++;
        if (u->Counter > 3)
            u->Counter = 0;

        if (!u->Counter)
        {
            SpawnBlood(ap-sprite, Weapon, 0, 0, 0, 0);
            if (RANDOM_RANGE(1000) > 600)
                InitBloodSpray(ap-sprite, FALSE, 105);
        }
    }

    // kill the fountain
    if ((u->WaitTics-=MISSILEMOVETICS) <= 0)
    {
        u->WaitTics = 0;

        bak_cstat = sp->cstat;
        RESET(sp->cstat, CSTAT_SPRITE_BLOCK);
        //DoDamageTest(Weapon); // fountain not doing the damage an more
        sp->cstat = bak_cstat;

        KillSprite(Weapon);
    }
    return 0;
}

int
DoPlasma(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int32_t dax, day, daz;
    int ox,oy,oz;

    ox = sp->x;
    oy = sp->y;
    oz = sp->z;

//MissileSeek(int16_t Weapon, int16_t delay_tics, int16_t aware_range, int16_t dang_shift, int16_t turn_limit, int16_t z_limit)
    //MissileSeek(Weapon, 20, 1024, 6, 80, 6);
    DoBlurExtend(Weapon, 0, 4);

    dax = MOVEx(sp->xvel, sp->ang);
    day = MOVEy(sp->xvel, sp->ang);
    daz = sp->zvel;

    u->ret = move_missile(Weapon, dax, day, daz, Z(16), Z(16), CLIPMASK_MISSILE, MISSILEMOVETICS);

    if (u->ret)
    {
        // this sprite is supposed to go through players/enemys
        // if hit a player/enemy back up and do it again with blocking reset
        if (TEST(u->ret, HIT_MASK) == HIT_SPRITE)
        {
            short hit_sprite = NORM_SPRITE(u->ret);
            SPRITEp hsp = &sprite[hit_sprite];
            USERp hu = User[hit_sprite];

            if (TEST(hsp->cstat, CSTAT_SPRITE_BLOCK) && !TEST(hsp->cstat, CSTAT_SPRITE_WALL))
            {
                short hcstat = hsp->cstat;

                if (hu && hit_sprite != u->WpnGoal)
                {
                    sp->x = ox;
                    sp->y = oy;
                    sp->z = oz;

                    RESET(hsp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
                    u->ret = move_missile(Weapon, dax, day, daz, Z(16), Z(16), CLIPMASK_MISSILE, MISSILEMOVETICS);
                    hsp->cstat = hcstat;
                }
            }
        }
    }


    MissileHitDiveArea(Weapon);
    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);
    //DoDamageTest(Weapon);

    if (u->ret)
    {
        if (WeaponMoveHit(Weapon))
        {
            if (TEST(u->Flags, SPR_SUICIDE))
            {
                KillSprite(Weapon);
                return TRUE;
            }
            else
            {
                u->Counter = 4;
                ChangeState(Weapon, s_PlasmaDone);
            }

            return TRUE;
        }
    }

    return FALSE;
}


int
DoCoolgFire(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);
    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);

    // !JIM! //!FRANK WHY???
    //DoDamageTest(Weapon);

    if (u->ret)
    {
        if (WeaponMoveHit(Weapon))
        {
            PlaySound(DIGI_CGMAGICHIT,&sp->x,&sp->y,&sp->z,v3df_follow);
            ChangeState(Weapon, s_CoolgFireDone);
            if (sp->owner >= 0 && User[sp->owner] && User[sp->owner]->ID != RIPPER_RUN_R0)  // JBF: added range check
                SpawnDemonFist(Weapon); // Just a red magic circle flash
            return TRUE;
        }
    }

    return FALSE;
}

int
DoEelFire(short Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];

    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);

    //DoDamageTest(Weapon);

    return FALSE;
}


void ScaleSpriteVector(short SpriteNum, int scale)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;

    u->xchange = mulscale16(u->xchange, scale);
    u->ychange = mulscale16(u->ychange, scale);
    u->zchange = mulscale16(u->zchange, scale);
}

void WallBounce(short SpriteNum, short ang)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    int old_ang;
    //short hit_wall;
    //short nextwall;
    int k,l;
    //short ang;
    int dax, day;

    //hit_wall = NORM_WALL(u->ret);

    //nextwall = wall[hit_wall].point2;
    //ang = getangle(wall[nextwall].x - wall[hit_wall].x, wall[nextwall].y - wall[hit_wall].y)+512;

    u->bounce++;

    //k = cos(ang) * sin(ang) * 2
    k = mulscale13(sintable[NORM_ANGLE(ang+512)], sintable[ang]);
    //l = cos(ang * 2)
    l = sintable[NORM_ANGLE((ang*2)+512)];

    dax = -u->xchange;
    day = -u->ychange;

    u->xchange = dmulscale14(day, k, dax, l);
    u->ychange = dmulscale14(dax, k, -day, l);

    old_ang = sp->ang;
    sp->ang = getangle(u->xchange, u->ychange);

    // hack to prevent missile from sticking to a wall
    //
    if (old_ang == sp->ang)
    {
        u->xchange = -u->xchange;
        u->ychange = -u->ychange;
        sp->ang = getangle(u->xchange, u->ychange);
    }
}


SWBOOL SlopeBounce(short SpriteNum, SWBOOL *hit_wall)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    int k,l;
    int hiz,loz;
    int slope;
    int dax,day,daz;
    short hit_sector;
    short daang;

    hit_sector = NORM_SECTOR(u->ret);

    getzsofslope(hit_sector, sp->x, sp->y, &hiz, &loz);

    // detect the ceiling and the hit_wall
    if (sp->z < DIV2(hiz+loz))
    {
        if (!TEST(sector[hit_sector].ceilingstat, CEILING_STAT_SLOPE))
            slope = 0;
        else
            slope = sector[hit_sector].ceilingheinum;
    }
    else
    {
        if (!TEST(sector[hit_sector].floorstat, FLOOR_STAT_SLOPE))
            slope = 0;
        else
            slope = sector[hit_sector].floorheinum;
    }

    if (!slope)
        return FALSE;

    // if greater than a 45 degree angle
    if (labs(slope) > 4096)
        *hit_wall = TRUE;
    else
        *hit_wall = FALSE;

    // get angle of the first wall of the sector
    k = sector[hit_sector].wallptr;
    l = wall[k].point2;
    daang = getangle(wall[l].x - wall[k].x, wall[l].y - wall[k].y);

    // k is now the slope of the ceiling or floor

    // normal vector of the slope
    dax = mulscale14(slope, sintable[(daang)&2047]);
    day = mulscale14(slope, sintable[(daang+1536)&2047]);
    daz = 4096; // 4096 = 45 degrees

    // reflection code
    k = ((u->xchange*dax) + (u->ychange*day)) + mulscale4(u->zchange, daz);
    l = (dax*dax) + (day*day) + (daz*daz);

    // make sure divscale doesn't overflow
    if ((klabs(k)>>14) < l)
    {
        k = divscale17(k, l);
        u->xchange -= mulscale16(dax, k);
        u->ychange -= mulscale16(day, k);
        u->zchange -= mulscale12(daz, k);

        sp->ang = getangle(u->xchange, u->ychange);
    }

    return TRUE;
}

extern STATE s_Phosphorus[];

int
DoGrenade(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int32_t dax, day, daz;
    short i;

    if (TEST(u->Flags, SPR_UNDERWATER))
    {
        ScaleSpriteVector(Weapon, 50000);

        u->Counter += 20;
        u->zchange += u->Counter;
    }
    else
    {
        u->Counter += 20;
        u->zchange += u->Counter;
    }

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange,
                          u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);

    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);

    ////DSPRINTF(ds, "dist %d, u->ret %d", FindDistance3D(u->xchange, u->ychange, u->zchange>>4), u->ret);
    //MONO_PRINT(ds);

    if (u->ret)
    {
        switch (TEST(u->ret, HIT_MASK))
        {
        case HIT_PLAX_WALL:
            KillSprite(Weapon);
            return TRUE;
        case HIT_SPRITE:
        {
            short wall_ang, dang;
            short hit_sprite = -2;
            SPRITEp hsp;

            PlaySound(DIGI_40MMBNCE, &sp->x, &sp->y, &sp->z, v3df_dontpan);

            hit_sprite = NORM_SPRITE(u->ret);
            hsp = &sprite[hit_sprite];

            // special case so grenade can ring gong
            if (hsp->lotag == TAG_SPRITE_HIT_MATCH)
            {
                if (TEST(SP_TAG8(hsp), BIT(3)))
                    DoMatchEverything(NULL, hsp->hitag, -1);
            }

            if (TEST(hsp->cstat, CSTAT_SPRITE_WALL))
            {
                wall_ang = NORM_ANGLE(hsp->ang);
                WallBounce(Weapon, wall_ang);
                ScaleSpriteVector(Weapon, 32000);
            }
            else
            {
                if (u->Counter2 == TRUE) // It's a phosphorus grenade!
                {
                    for (i=0; i<5; i++)
                    {
                        sp->ang = NORM_ANGLE(RANDOM_RANGE(2048));
                        InitPhosphorus(Weapon);
                    }
                }
                SpawnGrenadeExp(Weapon);
                KillSprite((short) Weapon);
                return TRUE;
            }


            break;
        }

        case HIT_WALL:
        {
            short hit_wall,nw,wall_ang,dang;
            WALLp wph;

            hit_wall = NORM_WALL(u->ret);
            wph = &wall[hit_wall];

            if (wph->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(wph, sp->x, sp->y, sp->z, sp->ang, u->ID);
                u->ret = 0;
                break;
            }

            PlaySound(DIGI_40MMBNCE, &sp->x, &sp->y, &sp->z, v3df_dontpan);

            nw = wall[hit_wall].point2;
            wall_ang = NORM_ANGLE(getangle(wall[nw].x - wph->x, wall[nw].y - wph->y)+512);

            //sp->ang = NORM_ANGLE(sp->ang + 1);
            WallBounce(Weapon, wall_ang);
            ScaleSpriteVector(Weapon, 22000);

            break;
        }

        case HIT_SECTOR:
        {
            SWBOOL did_hit_wall;
            if (SlopeBounce(Weapon, &did_hit_wall))
            {
                if (did_hit_wall)
                {
                    // hit a wall
                    ScaleSpriteVector(Weapon, 22000); // 28000
                    u->ret = 0;
                    u->Counter = 0;
                }
                else
                {
                    // hit a sector
                    if (sp->z > DIV2(u->hiz + u->loz))
                    {
                        // hit a floor
                        if (!TEST(u->Flags, SPR_BOUNCE))
                        {
                            SET(u->Flags, SPR_BOUNCE);
                            ScaleSpriteVector(Weapon, 40000); // 18000
                            u->ret = 0;
                            u->zchange /= 4;
                            u->Counter = 0;
                        }
                        else
                        {
                            if (u->Counter2 == TRUE) // It's a phosphorus grenade!
                            {
                                for (i=0; i<5; i++)
                                {
                                    sp->ang = NORM_ANGLE(RANDOM_RANGE(2048));
                                    InitPhosphorus(Weapon);
                                }
                            }
                            SpawnGrenadeExp(Weapon);
                            KillSprite((short) Weapon);
                            return TRUE;
                        }
                    }
                    else
                    {
                        // hit a ceiling
                        ScaleSpriteVector(Weapon, 22000);
                    }
                }
            }
            else
            {
                // hit floor
                if (sp->z > DIV2(u->hiz + u->loz))
                {
                    if (TEST(u->Flags, SPR_UNDERWATER))
                        SET(u->Flags, SPR_BOUNCE); // no bouncing underwater

                    if (u->lo_sectp && SectUser[sp->sectnum] && SectUser[sp->sectnum]->depth)
                        SET(u->Flags, SPR_BOUNCE); // no bouncing on shallow water

                    if (!TEST(u->Flags, SPR_BOUNCE))
                    {
                        SET(u->Flags, SPR_BOUNCE);
                        u->ret = 0;
                        u->Counter = 0;
                        u->zchange = -u->zchange;
                        ScaleSpriteVector(Weapon, 40000); // 18000
                        u->zchange /= 4;
                        PlaySound(DIGI_40MMBNCE, &sp->x, &sp->y, &sp->z, v3df_dontpan);
                    }
                    else
                    {
                        if (u->Counter2 == TRUE) // It's a phosphorus grenade!
                        {
                            for (i=0; i<5; i++)
                            {
                                sp->ang = NORM_ANGLE(RANDOM_RANGE(2048));
                                InitPhosphorus(Weapon);
                            }
                        }
                        //WeaponMoveHit(Weapon);
                        SpawnGrenadeExp(Weapon);
                        KillSprite((short) Weapon);
                        return TRUE;
                    }
                }
                else
                // hit something above
                {
                    u->zchange = -u->zchange;
                    ScaleSpriteVector(Weapon, 22000);
                    PlaySound(DIGI_40MMBNCE, &sp->x, &sp->y, &sp->z, v3df_dontpan);
                }
            }
            break;
        }
        }
    }

    if (u->bounce > 10)
    {
        SpawnGrenadeExp(Weapon);
        KillSprite(Weapon);
        return TRUE;
    }

    // if you haven't bounced or your going slow do some puffs
    if (!TEST(u->Flags, SPR_BOUNCE|SPR_UNDERWATER))
    {
        SPRITEp np;
        USERp nu;
        short New;

        New = SpawnSprite(STAT_MISSILE, PUFF, s_Puff, sp->sectnum,
                          sp->x, sp->y, sp->z, sp->ang, 100);

        np = &sprite[New];
        nu = User[New];

        SetOwner(Weapon, New);
        np->shade = -40;
        np->xrepeat = 40;
        np->yrepeat = 40;
        nu->ox = u->ox;
        nu->oy = u->oy;
        nu->oz = u->oz;
        SET(np->cstat, CSTAT_SPRITE_YCENTER);
        RESET(np->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

        nu->xchange = u->xchange;
        nu->ychange = u->ychange;
        nu->zchange = u->zchange;

        ScaleSpriteVector(New, 22000);

        if (TEST(u->Flags, SPR_UNDERWATER))
            SET(nu->Flags, SPR_UNDERWATER);
    }

    return FALSE;
}

int
DoVulcanBoulder(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int32_t dax, day, daz;

    u->Counter += 40;
    u->zchange += u->Counter;

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange,
                          u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    vel = ksqrt(SQ(u->xchange) + SQ(u->ychange));

    if (vel < 30)
    {
        SpawnLittleExp(Weapon);
        KillSprite(Weapon);
        return TRUE;
    }

    if (u->ret)
    {
        switch (TEST(u->ret, HIT_MASK))
        {
        case HIT_PLAX_WALL:
            KillSprite(Weapon);
            return TRUE;
        case HIT_SPRITE:
        {
            short wall_ang, dang;
            short hit_sprite = -2;
            SPRITEp hsp;

//                PlaySound(DIGI_DHCLUNK, &sp->x, &sp->y, &sp->z, v3df_dontpan);

            hit_sprite = NORM_SPRITE(u->ret);
            hsp = &sprite[hit_sprite];

            if (TEST(hsp->cstat, CSTAT_SPRITE_WALL))
            {
                wall_ang = NORM_ANGLE(hsp->ang);
                WallBounce(Weapon, wall_ang);
                ScaleSpriteVector(Weapon, 40000);
            }
            else
            {
                // hit an actor
                SpawnLittleExp(Weapon);
                KillSprite((short) Weapon);
                return TRUE;
            }


            break;
        }

        case HIT_WALL:
        {
            short hit_wall,nw,wall_ang,dang;
            WALLp wph;

            hit_wall = NORM_WALL(u->ret);
            wph = &wall[hit_wall];

            if (wph->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(wph, sp->x, sp->y, sp->z, sp->ang, u->ID);
                u->ret = 0;
                break;
            }

//                PlaySound(DIGI_DHCLUNK, &sp->x, &sp->y, &sp->z, v3df_dontpan);

            nw = wall[hit_wall].point2;
            wall_ang = NORM_ANGLE(getangle(wall[nw].x - wph->x, wall[nw].y - wph->y)+512);

            WallBounce(Weapon, wall_ang);
            ScaleSpriteVector(Weapon, 40000);
            break;
        }

        case HIT_SECTOR:
        {
            SWBOOL did_hit_wall;

            if (SlopeBounce(Weapon, &did_hit_wall))
            {
                if (did_hit_wall)
                {
                    // hit a sloped sector - treated as a wall because of large slope
                    ScaleSpriteVector(Weapon, 30000);
                    u->ret = 0;
                    u->Counter = 0;
                }
                else
                {
                    // hit a sloped sector
                    if (sp->z > DIV2(u->hiz + u->loz))
                    {
                        // hit a floor
                        u->xchange = mulscale16(u->xchange, 30000);
                        u->ychange = mulscale16(u->ychange, 30000);
                        u->zchange = mulscale16(u->zchange, 12000);
                        u->ret = 0;
                        u->Counter = 0;

                        // limit to a reasonable bounce value
                        if (u->zchange > Z(32))
                            u->zchange = Z(32);
                    }
                    else
                    {
                        // hit a sloped ceiling
                        u->ret = 0;
                        u->Counter = 0;
                        ScaleSpriteVector(Weapon, 30000);
                    }
                }
            }
            else
            {
                // hit unsloped floor
                if (sp->z > DIV2(u->hiz + u->loz))
                {
                    u->ret = 0;
                    u->Counter = 0;

                    u->xchange = mulscale16(u->xchange, 20000);
                    u->ychange = mulscale16(u->ychange, 20000);
                    u->zchange = mulscale16(u->zchange, 32000);

                    // limit to a reasonable bounce value
                    if (u->zchange > Z(24))
                        u->zchange = Z(24);

                    u->zchange = -u->zchange;

                }
                else
                // hit unsloped ceiling
                {
                    u->zchange = -u->zchange;
                    ScaleSpriteVector(Weapon, 30000);
                }
            }

            break;
        }
        }
    }

    return FALSE;
}

SWBOOL
OwnerIsPlayer(short Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon],uo;

    if (!u || !sp || (unsigned)sp->owner >= (unsigned)MAXSPRITES) return FALSE;
    uo = User[sp->owner];
    if (uo && uo->PlayerP) return TRUE;

    return FALSE;
}

int
DoMineRangeTest(short Weapon, short range)
{
    SPRITEp wp = &sprite[Weapon];
    USERp wu = User[Weapon];

    USERp u;
    SPRITEp sp;
    short i, nexti;
    unsigned stat;
    int dist, tx, ty;
    int tmin;
    SWBOOL ownerisplayer = FALSE;

    ownerisplayer = OwnerIsPlayer(Weapon);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            sp = &sprite[i];
            u = User[i];

            // don't detect the owner or the owners bottom half
            //if (wp->owner == i || (u->PlayerP && (wp->owner == u->PlayerP->PlayerSprite)))
            //    continue;

            DISTANCE(sp->x, sp->y, wp->x, wp->y, dist, tx, ty, tmin);
            if (dist > range)
                continue;

            if (sp == wp)
                continue;

            if (!TEST(sp->cstat, CSTAT_SPRITE_BLOCK))
                continue;

            if (!TEST(sp->extra, SPRX_PLAYER_OR_ENEMY))
                continue;

            if (u->ID == GIRLNINJA_RUN_R0 && !ownerisplayer)
                continue;

            dist = FindDistance3D(wp->x - sp->x, wp->y - sp->y, (wp->z - sp->z)>>4);
            if (dist > range)
                continue;

            if (!FAFcansee(sp->x,sp->y,SPRITEp_UPPER(sp),sp->sectnum,wp->x,wp->y,wp->z,wp->sectnum))
                continue;

            return TRUE;
        }
    }

    return FALSE;
}


int
DoMineStuck(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
#define MINE_DETONATE_STATE 99

    // if no owner then die
    if (u->Attach >= 0)
    {
        SPRITEp ap = &sprite[u->Attach];
        USERp au = User[u->Attach];

        ASSERT(au);

        // Is it attached to a dead actor? Blow it up if so.
        if (TEST(au->Flags, SPR_DEAD) && u->Counter2 < MINE_DETONATE_STATE)
        {
            u->Counter2 = MINE_DETONATE_STATE;
            u->WaitTics = SEC(1)/2;
        }

        setspritez_old(Weapon, ap->x, ap->y, ap->z - u->sz);
        sp->z = ap->z - DIV2(SPRITEp_SIZE_Z(ap));
    }

    // not activated yet
    if (!TEST(u->Flags, SPR_ACTIVE))
    {
        if ((u->WaitTics -= (MISSILEMOVETICS*2)) > 0)
            return FALSE;

        // activate it
        //u->WaitTics = 65536;
        u->WaitTics = 32767;
        u->Counter2 = 0;
        SET(u->Flags, SPR_ACTIVE);
    }

    // limit the number of times DoMineRangeTest is called
    u->Counter++;
    if (u->Counter > 1)
        u->Counter = 0;

    if (u->Counter2 != MINE_DETONATE_STATE)
    {
        if ((u->Counter2++) > 30)
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->WaitTics = 32767;  // Keep reseting tics to make it stay forever
            u->Counter2 = 0;
        }
    }

    if (!u->Counter)
    {
        // not already in detonate state
        if (u->Counter2 < MINE_DETONATE_STATE)
        {
            // if something came into range - detonate
            if (DoMineRangeTest(Weapon, 3000))
            {
                // move directly to detonate state
                u->Counter2 = MINE_DETONATE_STATE;
                u->WaitTics = SEC(1)/2;
            }
        }
    }

    u->WaitTics -= (MISSILEMOVETICS * 2);


    // start beeping with pauses
    // quick and dirty beep countdown code
    switch (u->Counter2)
    {
#if 0
    case 0:
        if (u->WaitTics < SEC(45))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2++;
        }
        break;
    case 1:
        if (u->WaitTics < SEC(38))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2++;
        }
        break;
    case 2:
        if (u->WaitTics < SEC(30))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2++;
        }
        break;
    case 3:
        if (u->WaitTics < SEC(20))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2++;
        }
        break;
    case 4:
        if (u->WaitTics < SEC(15))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2++;
        }
        break;
    case 5:
        if (u->WaitTics < SEC(12))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2++;
        }
        break;
    case 6:
        if (u->WaitTics < SEC(10))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2++;
        }
        break;
    case 7:
        if (u->WaitTics < SEC(8))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2++;
        }
        break;
#endif
    case 30:
        if (u->WaitTics < SEC(6))
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            u->Counter2 = MINE_DETONATE_STATE;
        }
        break;
//        case MINE_DETONATE_STATE:
//            if (u->WaitTics < SEC(5))
//                {
    // start frantic beeping
//                PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
//                u->Counter2++;
//                }
//            break;
    case MINE_DETONATE_STATE:
        if (u->WaitTics < 0)
        {
            PlaySound(DIGI_MINEBEEP, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            SpawnMineExp(Weapon);
            KillSprite(Weapon);
            return FALSE;
        }
        break;
    }

    return FALSE;
}

int
SetMineStuck(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];

    // stuck
    SET(u->Flags, SPR_BOUNCE);
    // not yet active for 1 sec
    RESET(u->Flags, SPR_ACTIVE);
    u->WaitTics = SEC(3);
    //SET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    SET(sp->cstat, CSTAT_SPRITE_BLOCK_HITSCAN);
    u->Counter = 0;
    change_sprite_stat(Weapon, STAT_MINE_STUCK);
    ChangeState(Weapon, s_MineStuck);
    return 0;
}

int
DoMine(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];

    if (TEST(u->Flags, SPR_UNDERWATER))
    {
        // decrease velocity
        ScaleSpriteVector(Weapon, 50000);

        u->Counter += 20;
        u->zchange += u->Counter;
    }
    else
    {
        //u->Counter += 75;
        u->Counter += 40;
        u->zchange += u->Counter;
    }

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange,
                          u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);

    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);

    if (u->ret)
    {
        // check to see if you hit a sprite
        switch (TEST(u->ret, HIT_MASK))
        {
        case HIT_PLAX_WALL:
            KillSprite(Weapon);
            return 0;
        case HIT_SPRITE:
        {
            short hit_sprite = NORM_SPRITE(u->ret);
            SPRITEp hsp = &sprite[hit_sprite];
            USERp hu = User[hit_sprite];

            SetMineStuck(Weapon);
            // Set the Z position
            sp->z = hsp->z - DIV2(SPRITEp_SIZE_Z(hsp));

            // If it's not alive, don't stick it
            if (hu && hu->Health <= 0) return FALSE;    // JBF: added null check

            // check to see if sprite is player or enemy
            if (TEST(hsp->extra, SPRX_PLAYER_OR_ENEMY))
            {
                USERp uo;
                PLAYERp pp;

                // attach weapon to sprite
                SetAttach(hit_sprite, Weapon);
                u->sz = sprite[hit_sprite].z - sp->z;

                if (sp->owner >= 0)
                {
                    uo = User[sp->owner];

                    if (uo && uo->PlayerP)
                    {
                        pp = uo->PlayerP;

                        if (RANDOM_RANGE(1000) > 800)
                            PlayerSound(DIGI_STICKYGOTU1,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
                        else if (RANDOM_RANGE(1000) > 800)
                            PlayerSound(DIGI_STICKYGOTU2,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
                        else if (RANDOM_RANGE(1000) > 800)
                            PlayerSound(DIGI_STICKYGOTU3,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
                        else if (RANDOM_RANGE(1000) > 800)
                            PlayerSound(DIGI_STICKYGOTU4,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
                    }
                }
            }
            else
            {
                if (TEST(hsp->cstat, CSTAT_SPRITE_WALL))
                {
                    SET(u->Flags2, SPR2_ATTACH_WALL);
                }
                else if (TEST(hsp->cstat, CSTAT_SPRITE_FLOOR))
                {
                    // hit floor
                    if (sp->z > DIV2(u->hiz + u->loz))
                        SET(u->Flags2, SPR2_ATTACH_FLOOR);
                    else
                        SET(u->Flags2, SPR2_ATTACH_CEILING);
                }
                else
                {
                    SpawnMineExp(Weapon);
                    KillSprite(Weapon);
                    return FALSE;
                }
            }

            break;
        }

        case HIT_WALL:
        {
            short hit_wall = NORM_WALL(u->ret);

            if (wall[hit_wall].lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(&wall[hit_wall], sp->x, sp->y, sp->z, sp->ang, u->ID);
                u->ret = 0;
                break;
            }

            SetMineStuck(Weapon);

            SET(u->Flags2, SPR2_ATTACH_WALL);

            if (TEST(wall[hit_wall].extra, WALLFX_SECTOR_OBJECT))
            {
            }

            if (TEST(wall[hit_wall].extra, WALLFX_DONT_STICK))
            {
                SpawnMineExp(Weapon);
                KillSprite(Weapon);
                return FALSE;
            }

            break;
        }

        case HIT_SECTOR:
        {
            short hit_sect = NORM_SECTOR(u->ret);

            SetMineStuck(Weapon);

            // hit floor
            if (sp->z > DIV2(u->hiz + u->loz))
                SET(u->Flags2, SPR2_ATTACH_FLOOR);
            else
                SET(u->Flags2, SPR2_ATTACH_CEILING);


            if (TEST(sector[hit_sect].extra, SECTFX_SECTOR_OBJECT))
            {
                SpawnMineExp(Weapon);
                KillSprite(Weapon);
                return FALSE;
            }

            break;
        }
        }

        u->ret = 0;
    }

    return FALSE;
}

int
DoPuff(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    sp->x += u->xchange;
    sp->y += u->ychange;
    sp->z += u->zchange;

    return 0;
}

int
DoRailPuff(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    sp->xrepeat += 4;
    sp->yrepeat += 4;

    return 0;
}

int
DoBoltThinMan(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int32_t dax, day, daz;
    short NewWeapon;

    DoBlurExtend(Weapon, 0, 4);

    dax = MOVEx(sp->xvel, sp->ang);
    day = MOVEy(sp->xvel, sp->ang);
    daz = sp->zvel;

    u->ret = move_missile(Weapon, dax, day, daz, CEILING_DIST, FLOOR_DIST, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);

    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);

    //DoDamageTest(Weapon);

    if (TEST(u->Flags, SPR_SUICIDE))
        return TRUE;

    if (u->ret)
    {
        if (WeaponMoveHit(Weapon))
        {
            SpawnBoltExp(Weapon);
            KillSprite(Weapon);
            return TRUE;
        }
    }


    return FALSE;
}

int
DoTracer(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp np;
    USERp nu;
    short New;
    short spawn_count = 0;
    short i;

    for (i = 0; i < 4; i++)
    {
        u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

        MissileHitDiveArea(Weapon);

        if (u->ret)
        {
            if (WeaponMoveHit(Weapon))
            {
                KillSprite(Weapon);
                return TRUE;
            }
        }
    }

    RESET(sp->cstat, CSTAT_SPRITE_INVISIBLE);

    return FALSE;
}

int
DoEMP(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp np;
    USERp nu;
    short New;
    short spawn_count = 0;
    short i;

    for (i = 0; i < 4; i++)
    {
        u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

        MissileHitDiveArea(Weapon);

        if (RANDOM_RANGE(1000) > 500)
        {
            sp->xrepeat = 52;
            sp->yrepeat = 10;
        }
        else
        {
            sp->xrepeat = 8;
            sp->yrepeat = 38;
        }

        if (u->ret)
        {
            if (WeaponMoveHit(Weapon))
            {
                KillSprite(Weapon);
                return TRUE;
            }
        }
    }

    RESET(sp->cstat, CSTAT_SPRITE_INVISIBLE);

    return FALSE;
}

int
DoEMPBurst(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];

    if (u->Attach >= 0)
    {
        SPRITEp ap = &sprite[u->Attach];
        USERp au = User[u->Attach];

        ASSERT(au);

        setspritez_old(Weapon, ap->x, ap->y, ap->z - u->sz);
        sp->ang = NORM_ANGLE(ap->ang+1024);
    }

    // not activated yet
    if (!TEST(u->Flags, SPR_ACTIVE))
    {
        // activate it
        u->WaitTics = SEC(7);
        SET(u->Flags, SPR_ACTIVE);
    }

    if (RANDOM_RANGE(1000) > 500)
    {
        sp->xrepeat = 52;
        sp->yrepeat = 10;
    }
    else
    {
        sp->xrepeat = 8;
        sp->yrepeat = 38;
    }

    if ((RANDOM_P2(1024<<6)>>6) < 700)
    {
        SpawnShrapX(Weapon);
    }

    u->WaitTics -= (MISSILEMOVETICS * 2);

    if (u->WaitTics < 0)
    {
        //SpawnMineExp(Weapon);
        // Spawn a big radius burst of sparks here and check for final damage amount
        KillSprite(Weapon);
        return FALSE;
    }

    return FALSE;
}

int
DoTankShell(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp np;
    USERp nu;
    short New;
    short spawn_count = 0;
    short i;

    for (i = 0; i < 4; i++)
    {
        u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

        MissileHitDiveArea(Weapon);

        if (u->ret)
        {
            if (WeaponMoveHit(Weapon))
            {
                SpawnTankShellExp(Weapon);
                //SetExpQuake(exp);
                KillSprite(Weapon);
                return TRUE;
            }
        }
    }

    return FALSE;
}

int
DoTracerStart(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp np;
    USERp nu;
    short New;
    short spawn_count = 0;
    short i;

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange,
                          u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);

    if (u->ret)
    {
        if (WeaponMoveHit(Weapon))
        {
            KillSprite(Weapon);
            return TRUE;
        }
    }

    return FALSE;
}

int
DoLaser(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp np;
    USERp nu;
    short New;
    short spawn_count = 0;

    if (SW_SHAREWARE) return FALSE; // JBF: verify

    while (TRUE)
    {
        u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

        MissileHitDiveArea(Weapon);

        if (u->ret)
        {
            if (WeaponMoveHit(Weapon))
            {
                SpawnBoltExp(Weapon);
                KillSprite((short) Weapon);
                return TRUE;
            }
        }

        spawn_count++;
        if (spawn_count < 256)
        {
            New = SpawnSprite(STAT_MISSILE, PUFF, s_LaserPuff, sp->sectnum,
                              sp->x, sp->y, sp->z, sp->ang, 0);
            np = &sprite[New];
            nu = User[New];

            np->shade = -40;
            np->xrepeat = 16;
            np->yrepeat = 16;
            np->pal = nu->spal = PALETTE_RED_LIGHTING;

            SET(np->cstat, CSTAT_SPRITE_YCENTER);
            RESET(np->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

            nu->xchange = nu->ychange = nu->zchange = 0;
        }
    }
}

int
DoLaserStart(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp np;
    USERp nu;

    if (SW_SHAREWARE) return FALSE; // JBF: verify

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);

    if (u->ret)
    {
        if (WeaponMoveHit(Weapon))
        {
            SpawnBoltExp(Weapon);
            KillSprite((short) Weapon);
            return TRUE;
        }
    }

    return 0;
}

int
DoRail(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int32_t dax, day, daz;
    short NewWeapon;
    SPRITEp np;
    USERp nu;
    short New;
    short spawn_count = 0;

    if (SW_SHAREWARE) return FALSE; // JBF: verify

    while (TRUE)
    {
        u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

        MissileHitDiveArea(Weapon);

        if (u->ret)
        {
            if (WeaponMoveHit(Weapon) && u->ret)
            {
                if (TEST(u->ret, HIT_MASK) == HIT_SPRITE)
                {
                    short hit_sprite;
                    hit_sprite = NORM_SPRITE(u->ret);

                    if (TEST(sprite[hit_sprite].extra, SPRX_PLAYER_OR_ENEMY))
                    {
                        short cstat_save = sprite[hit_sprite].cstat;

                        RESET(sprite[hit_sprite].cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN|CSTAT_SPRITE_BLOCK_MISSILE);
                        DoRail(Weapon);
                        sprite[hit_sprite].cstat = cstat_save;
                        return TRUE;
                    }
                    else
                    {
                        SpawnTracerExp(Weapon);
                        SpawnShrapX(Weapon);
                        KillSprite((short) Weapon);
                        return TRUE;
                    }
                }
                else
                {
                    SpawnTracerExp(Weapon);
                    SpawnShrapX(Weapon);
                    KillSprite((short) Weapon);
                    return TRUE;
                }
            }
        }

        spawn_count++;
        if (spawn_count < 128)
        {
            New = SpawnSprite(STAT_MISSILE, PUFF, &s_RailPuff[0][0], sp->sectnum,
                              sp->x, sp->y, sp->z, sp->ang, 20);

            np = &sprite[New];
            nu = User[New];

            np->xvel += (RANDOM_RANGE(140)-RANDOM_RANGE(140));
            np->yvel += (RANDOM_RANGE(140)-RANDOM_RANGE(140));
            np->zvel += (RANDOM_RANGE(140)-RANDOM_RANGE(140));

            nu->RotNum = 5;
            NewStateGroup(New, sg_RailPuff);

            np->shade = -40;
            np->xrepeat = 10;
            np->yrepeat = 10;
            nu->ox = u->ox;
            nu->oy = u->oy;
            nu->oz = u->oz;
            SET(np->cstat, CSTAT_SPRITE_YCENTER);
            RESET(np->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

            nu->xchange = u->xchange;
            nu->ychange = u->ychange;
            nu->zchange = u->zchange;

            ScaleSpriteVector(New, 1500);

            if (TEST(u->Flags, SPR_UNDERWATER))
                SET(nu->Flags, SPR_UNDERWATER);
        }
    }
}

int
DoRailStart(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int32_t dax, day, daz;
    SPRITEp np;
    USERp nu;
    short New;

    if (SW_SHAREWARE) return FALSE; // JBF: verify

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);

    if (u->ret)
    {
        if (WeaponMoveHit(Weapon))
        {
            SpawnTracerExp(Weapon);
            SpawnShrapX(Weapon);
            KillSprite((short) Weapon);
            return TRUE;
        }
    }

    return 0;
}

int
DoRocket(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int32_t dax, day, daz;
    short NewWeapon;
    int dist,a,b,c;


    if ((u->FlagOwner -= ACTORMOVETICS)<=0 && u->spal == 20)
    {
        DISTANCE(sp->x, sp->y, u->tgt_sp->x, u->tgt_sp->y, dist, a, b, c);
        u->FlagOwner = dist>>6;
        // Special warn sound attached to each seeker spawned
        PlaySound(DIGI_MINEBEEP,&sp->x,&sp->y,&sp->z,v3df_follow);
    }

    if (TEST(u->Flags, SPR_FIND_PLAYER))
    {
        //MissileSeek(Weapon, 10, 768, 3, 48, 6);
        VectorMissileSeek(Weapon, 30, 16, 128, 768);
    }

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);

    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);

    //DoDamageTest(Weapon);

    if (TEST(u->Flags, SPR_SUICIDE))
        return TRUE;

    if (u->ret)
    {
        if (WeaponMoveHit(Weapon) && u->ret)
        {
            if (u->ID == BOLT_THINMAN_R4)
            {
                SpawnBunnyExp(Weapon);
            }
            else if (u->Radius == NUKE_RADIUS)
                SpawnNuclearExp(Weapon);
            else
                SpawnBoltExp(Weapon);

            KillSprite((short) Weapon);
            return TRUE;
        }
    }

    if (!u->Counter)
    {
        SPRITEp np;
        USERp nu;
        short New;

        New = SpawnSprite(STAT_MISSILE, PUFF, s_Puff, sp->sectnum,
                          u->ox, u->oy, u->oz, sp->ang, 100);

        np = &sprite[New];
        nu = User[New];

        SetOwner(Weapon, New);
        np->shade = -40;
        np->xrepeat = 40;
        np->yrepeat = 40;
        nu->ox = u->ox;
        nu->oy = u->oy;
        nu->oz = u->oz;
        SET(np->cstat, CSTAT_SPRITE_YCENTER);
        RESET(np->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

        nu->xchange = u->xchange;
        nu->ychange = u->ychange;
        nu->zchange = u->zchange;

        ScaleSpriteVector(New, 20000);

        //nu->zchange -= Z(8);

        if (TEST(u->Flags, SPR_UNDERWATER))
            SET(nu->Flags, SPR_UNDERWATER);
    }

    return FALSE;
}

int
DoMicroMini(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp np;
    USERp nu;
    short New;
    short spawn_count = 0;
    short i;

    for (i = 0; i < 3; i++)
    {
        u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

        MissileHitDiveArea(Weapon);

        if (u->ret)
        {
            if (WeaponMoveHit(Weapon))
            {
                SpawnMicroExp(Weapon);
                KillSprite((short) Weapon);
                return TRUE;
            }
        }
    }

    return FALSE;
}

int
SpawnExtraMicroMini(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp wp;
    USERp wu;
    short w;

    w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R0, &s_Micro[0][0], sp->sectnum,
                    sp->x, sp->y, sp->z, sp->ang, sp->xvel);

    wp = &sprite[w];
    wu = User[w];

    SetOwner(sp->owner, w);
    wp->yrepeat = wp->xrepeat = sp->xrepeat;
    //wp->xrepeat = 64;
    wp->shade = sp->shade;
    wp->clipdist = sp->clipdist;

    wu->RotNum = 5;
    NewStateGroup(w, &sg_MicroMini[0]);
    wu->WeaponNum = u->WeaponNum;
    wu->Radius = u->Radius;
    wu->ceiling_dist = u->ceiling_dist;
    wu->floor_dist = u->floor_dist;
    wp->cstat = sp->cstat;

    wp->ang = NORM_ANGLE(wp->ang + RANDOM_RANGE(64) - 32);
    wp->zvel = sp->zvel;
    wp->zvel += RANDOM_RANGE(Z(16)) - Z(8);

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;
    return 0;
}

int
DoMicro(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int32_t dax, day, daz;
    short New;

    if (SW_SHAREWARE) return FALSE; // JBF: verify

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange,
                          u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);

    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);

    //DoDamageTest(Weapon);

    if (!u->Counter)
    {
        SPRITEp np;
        USERp nu;

        New = SpawnSprite(STAT_MISSILE, PUFF, s_Puff, sp->sectnum,
                          sp->x, sp->y, sp->z, sp->ang, 100);

        np = &sprite[New];
        nu = User[New];

        SetOwner(sp->owner, New);
        np->shade = -40;
        np->xrepeat = 20;
        np->yrepeat = 20;
        nu->ox = u->ox;
        nu->oy = u->oy;
        nu->oz = u->oz;
        np->zvel = sp->zvel;
        SET(np->cstat, CSTAT_SPRITE_YCENTER);
        RESET(np->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

        nu->xchange = u->xchange;
        nu->ychange = u->ychange;
        nu->zchange = u->zchange;

        ScaleSpriteVector(New, 20000);

        if (TEST(u->Flags, SPR_UNDERWATER))
            SET(nu->Flags, SPR_UNDERWATER);

        // last smoke
        if ((u->WaitTics -= MISSILEMOVETICS) <= 0)
        {
            setspritez(New, (vec3_t *)np);
            NewStateGroup(Weapon, &sg_MicroMini[0]);
            sp->xrepeat = sp->yrepeat = 10;
            RESET(sp->cstat, CSTAT_SPRITE_INVISIBLE);
            SpawnExtraMicroMini(Weapon);
            return TRUE;
        }
    }


    // hit something
    if (u->ret)
    {
        if (WeaponMoveHit(Weapon))
        {
            SpawnMicroExp(Weapon);
            KillSprite(Weapon);
            return TRUE;
        }
    }

    return FALSE;
}

int
DoUziBullet(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon], newp;
    USERp u = User[Weapon];
    int32_t dax, day, daz;
    int sx,sy;
    short NewWeapon;
    short i;

    // call move_sprite twice for each movement
    // otherwize the moves are in too big an increment
    for (i = 0; i < 2; i++)
    {
        dax = MOVEx((sp->xvel >> 1), sp->ang);
        day = MOVEy((sp->xvel >> 1), sp->ang);
        daz = sp->zvel >> 1;

        sx = sp->x;
        sy = sp->y;
        u->ret = move_missile(Weapon, dax, day, daz, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);
        u->Dist += Distance(sx, sy, sp->x, sp->y);

        MissileHitDiveArea(Weapon);

        if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 128)
            SpawnBubble(Weapon);

        if (u->ret)
        {
            SPRITEp wp;
            short j;

            WeaponMoveHit(Weapon);

            j = SpawnSprite(STAT_MISSILE, UZI_SMOKE, s_UziSmoke, sp->sectnum, sp->x, sp->y, sp->z, sp->ang, 0);
            wp = &sprite[j];
            wp->shade = -40;
            wp->xrepeat = UZI_SMOKE_REPEAT;
            wp->yrepeat = UZI_SMOKE_REPEAT;
            SetOwner(sp->owner, j);
            wp->ang = sp->ang;
            wp->clipdist = 128 >> 2;
            SET(wp->cstat, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);

            if (!TEST(u->Flags, SPR_UNDERWATER))
            {
                j = SpawnSprite(STAT_MISSILE, UZI_SPARK, s_UziSpark, wp->sectnum, wp->x, wp->y, wp->z, 0, 0);
                wp = &sprite[j];
                wp->shade = -40;
                wp->xrepeat = UZI_SPARK_REPEAT;
                wp->yrepeat = UZI_SPARK_REPEAT;
                //wp->owner = sp->owner;
                SetOwner(sp->owner, j);
                wp->ang = sp->ang;
                SET(wp->cstat, CSTAT_SPRITE_YCENTER);
            }

            KillSprite(Weapon);

            return TRUE;
        }
        else if (u->Dist > 8000)
        {
            KillSprite((short) Weapon);
            return 0;
        }
    }

    return FALSE;
}


int
DoBoltSeeker(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon], newp;
    USERp u = User[Weapon];
    int32_t dax, day, daz;
    short NewWeapon;

    MissileSeek(Weapon, 30, 768, 4, 48, 6);
    DoBlurExtend(Weapon, 0, 4);

    dax = MOVEx(sp->xvel, sp->ang);
    day = MOVEy(sp->xvel, sp->ang);
    daz = sp->zvel;

    u->ret = move_missile(Weapon, dax, day, daz, CEILING_DIST, FLOOR_DIST, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);
    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);
    //DoDamageTest(Weapon);

    if (u->ret)
    {
        if (WeaponMoveHit(Weapon))
        {
            SpawnBoltExp(Weapon);
            KillSprite((short) Weapon);
            return TRUE;
        }
    }


    return FALSE;
}

int
DoBoltShrapnel(int16_t Weapon)
{
    return 0;
}

int
DoBoltFatMan(int16_t Weapon)
{
    return 0;
}

int
DoElectro(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon], newp;
    USERp u = User[Weapon];
    int32_t dax, day, daz;

    DoBlurExtend(Weapon, 0, 4);

    // only seek on Electro's after a hit on an actor
    if (u->Counter > 0)
        MissileSeek(Weapon, 30, 512, 3, 52, 2);

    dax = MOVEx(sp->xvel, sp->ang);
    day = MOVEy(sp->xvel, sp->ang);
    daz = sp->zvel;

    u->ret = move_missile(Weapon, dax, day, daz, CEILING_DIST, FLOOR_DIST, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);
    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);
    //DoDamageTest(Weapon);

    if (TEST(u->Flags, SPR_SUICIDE))
        return TRUE;

    if (u->ret)
    {
        if (WeaponMoveHit(Weapon))
        {
            switch (TEST(u->ret, HIT_MASK))
            {
            case HIT_SPRITE:
            {
                SPRITEp hsp = &sprite[NORM_SPRITE(u->ret)];
                USERp hu = User[NORM_SPRITE(u->ret)];

                if (!TEST(hsp->extra, SPRX_PLAYER_OR_ENEMY) || hu->ID == SKULL_R0 || hu->ID == BETTY_R0)
                    SpawnShrap(Weapon, -1);
                break;
            }

            default:
                SpawnShrap(Weapon, -1);
                break;
            }

            //SpawnShrap(Weapon, -1);
            KillSprite(Weapon);
            return TRUE;
        }
    }

    return FALSE;
}

int
DoLavaBoulder(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon], newp;
    USERp u = User[Weapon];
    int32_t dax, day, daz;

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange,
                          u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);
    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);
    //DoDamageTest(Weapon);

    if (TEST(u->Flags, SPR_SUICIDE))
        return TRUE;

    if (u->ret)
    {
        if (WeaponMoveHit(Weapon))
        {
            SpawnShrap(Weapon, -1);
            KillSprite(Weapon);
            return TRUE;
        }
    }

    return FALSE;
}

int
DoSpear(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon], newp;
    USERp u = User[Weapon];
    int32_t dax, day, daz;

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange,
                          u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);

    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);

    //DoDamageTest(Weapon);

    if (TEST(u->Flags, SPR_SUICIDE))
        return TRUE;

    if (u->ret)
    {
        if (WeaponMoveHit(Weapon))
        {
            //SpawnShrap(Weapon, -1);
            KillSprite(Weapon);
            return TRUE;
        }
    }

    return FALSE;
}

int SpawnCoolieExp(short SpriteNum)
{
    USERp u = User[SpriteNum], eu;
    SPRITEp sp = &sprite[SpriteNum];
    extern STATE s_BasicExp[];

    short explosion;
    SPRITEp exp;
    int zh,nx,ny;

    ASSERT(u);

    u->Counter = RANDOM_RANGE(120);  // This is the wait til birth time!

    zh = sp->z - SPRITEp_SIZE_Z(sp) + DIV4(SPRITEp_SIZE_Z(sp));
    nx = sp->x + MOVEx(64, sp->ang+1024);
    ny = sp->y + MOVEy(64, sp->ang+1024);

    PlaySound(DIGI_COOLIEEXPLODE, &sp->x, &sp->y, &sp->z, v3df_none);

    explosion = SpawnSprite(STAT_MISSILE, BOLT_EXP, s_BoltExp, sp->sectnum,
                            nx, ny, zh, sp->ang, 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    SetOwner(SpriteNum, explosion);
    exp->shade = -40;
    exp->pal = eu->spal = u->spal;
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    eu->Radius = DamageData[DMG_BOLT_EXP].radius;

    DoExpDamageTest(explosion);

    return explosion;
}


int
SpawnBasicExp(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;

    ASSERT(u);

    if (TEST(u->Flags, SPR_SUICIDE))
        return -1;

    PlaySound(DIGI_MEDIUMEXP, &sp->x, &sp->y, &sp->z, v3df_none);

    explosion = SpawnSprite(STAT_MISSILE, BASIC_EXP, s_BasicExp, sp->sectnum,
                            sp->x, sp->y, sp->z, sp->ang, 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    exp->xrepeat = 24;
    exp->yrepeat = 24;
    SetOwner(sp->owner, explosion);
    exp->shade = -40;
    exp->pal = eu->spal = u->spal;
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    eu->Radius = DamageData[DMG_BASIC_EXP].radius;

    //
    // All this stuff assures that explosions do not go into floors &
    // ceilings
    //

    SpawnExpZadjust(Weapon, exp, Z(15), Z(15));
    DoExpDamageTest(explosion);
    SpawnVis(-1, exp->sectnum, exp->x, exp->y, exp->z, 16);

    return explosion;
}

int
SpawnFireballFlames(int16_t SpriteNum, int16_t enemy)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SPRITEp ep = &sprite[enemy];
    USERp eu = User[enemy];
    SPRITEp np;
    USERp nu;
    short New;

    if (TEST(u->Flags, SPR_UNDERWATER))
        return -1;

    if (enemy >= 0)
    {
        // test for already burned
        if (TEST(ep->extra, SPRX_BURNABLE) && ep->shade > 40)
            return -1;

        if (!eu)
        {
            ASSERT(TRUE == FALSE);
        }

        if (eu->flame >= 0)
        {
            int sizez = SPRITEp_SIZE_Z(ep) + DIV4(SPRITEp_SIZE_Z(ep));
            np = &sprite[eu->flame];
            nu = User[eu->flame];

            if (TEST(ep->extra, SPRX_BURNABLE))
                return eu->flame;

            if (nu->Counter >= SPRITEp_SIZE_Z_2_YREPEAT(np, sizez))
            {
                // keep flame only slightly bigger than the enemy itself
                nu->Counter = SPRITEp_SIZE_Z_2_YREPEAT(np, sizez);
            }
            else
            {
                //increase max size
                nu->Counter += SPRITEp_SIZE_Z_2_YREPEAT(np, 8<<8);
            }

            // Counter is max size
            if (nu->Counter >= 230)
            {
                // this is far too big
                nu->Counter = 230;
            }

            if (nu->WaitTics < 2*120)
                nu->WaitTics = 2*120;  // allow it to grow again

            return eu->flame;
        }
        else
        {
            if (eu->PlayerP)
            {
                void SpawnOnFire(PLAYERp pp);
                //SpawnOnFire(eu->PlayerP);  //Nobody likes the panel fire
            }
        }
    }

    New = SpawnSprite(STAT_MISSILE, FIREBALL_FLAMES, s_FireballFlames, sp->sectnum,
                      sp->x, sp->y, sp->z, sp->ang, 0);
    np = &sprite[New];
    nu = User[New];

    np->hitag = LUMINOUS; //Always full brightness

    if (enemy >= 0)
        eu->flame = New;

    np->xrepeat = 16;
    np->yrepeat = 16;
    if (enemy >= 0)
    {
        // large flame for trees and such
        if (TEST(ep->extra, SPRX_BURNABLE))
        {
            int sizez = SPRITEp_SIZE_Z(ep) + DIV4(SPRITEp_SIZE_Z(ep));
            nu->Counter = SPRITEp_SIZE_Z_2_YREPEAT(np, sizez);
        }
        else
        {
            nu->Counter = SPRITEp_SIZE_Z_2_YREPEAT(np, SPRITEp_SIZE_Z(ep)>>1);
        }
    }
    else
    {
        nu->Counter = 48; // max flame size
    }

    SetOwner(sp->owner, New);
    np->shade = -40;
    np->pal = nu->spal = u->spal;
    SET(np->cstat, CSTAT_SPRITE_YCENTER);
    RESET(np->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    //nu->Radius = DamageData[DMG_FIREBALL_FLAMES].radius;
    nu->Radius = 200;

    if (enemy >= 0)
    {
        SetAttach(enemy, New);
    }
    else
    {
        if (TestDontStickSector(np->sectnum))
        {
            KillSprite(New);
            return -1;
        }

        nu->floor_dist = nu->ceiling_dist = 0;

        DoFindGround(New);
        nu->jump_speed = 0;
        DoBeginJump(New);
    }

    PlaySound(DIGI_FIRE1,&np->x,&np->y,&np->z,v3df_dontpan|v3df_doppler);
    Set3DSoundOwner(New);

    return New;
}


int
SpawnBreakFlames(int16_t SpriteNum, int16_t enemy)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SPRITEp ep = &sprite[enemy];
    USERp eu = User[enemy];
    SPRITEp np;
    USERp nu;
    short New;

    New = SpawnSprite(STAT_MISSILE, FIREBALL_FLAMES+1, s_BreakFlames, sp->sectnum,
                      sp->x, sp->y, sp->z, sp->ang, 0);
    np = &sprite[New];
    nu = User[New];

    np->hitag = LUMINOUS; //Always full brightness

    np->xrepeat = 16;
    np->yrepeat = 16;
    nu->Counter = 48; // max flame size

    //SetOwner(sp->owner, New);
    np->shade = -40;
    if (u)
        np->pal = nu->spal = u->spal;
    SET(np->cstat, CSTAT_SPRITE_YCENTER);
    RESET(np->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    nu->Radius = 200;

    nu->floor_dist = nu->ceiling_dist = 0;

    DoFindGround(New);
    nu->jump_speed = 0;
    DoBeginJump(New);

    PlaySound(DIGI_FIRE1,&np->x,&np->y,&np->z,v3df_dontpan|v3df_doppler);
    Set3DSoundOwner(New);

    return New;
}


int
SpawnBreakStaticFlames(int16_t SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SPRITEp np;
    USERp nu;
    short New;

    New = SpawnSprite(STAT_STATIC_FIRE, FIREBALL_FLAMES, NULL, sp->sectnum,
                      sp->x, sp->y, sp->z, sp->ang, 0);
    np = &sprite[New];
    nu = User[New];

    if (RANDOM_RANGE(1000) > 500)
        np->picnum = 3143;
    else
        np->picnum = 3157;

    np->hitag = LUMINOUS; //Always full brightness

    //np->xrepeat = 64;
    //np->yrepeat = 64;
    np->xrepeat = 32;
    np->yrepeat = 32;
    //nu->Counter = 48; // max flame size

    //SetOwner(sp->owner, New);
    np->shade = -40;
    np->pal = nu->spal = u->spal;
    //SET(np->cstat, CSTAT_SPRITE_YCENTER);
    RESET(np->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    nu->Radius = 200;

    nu->floor_dist = nu->ceiling_dist = 0;

    np->z = getflorzofslope(np->sectnum,np->x,np->y);

    //DoFindGround(New);
    //nu->jump_speed = 0;
    //DoBeginJump(New);

    PlaySound(DIGI_FIRE1,&np->x,&np->y,&np->z,v3df_dontpan|v3df_doppler);
    Set3DSoundOwner(New);

    return New;
}


int
SpawnFireballExp(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;

    ASSERT(u);

    if (TEST(u->Flags, SPR_SUICIDE))
        return -1;

    PlaySound(DIGI_SMALLEXP, &sp->x, &sp->y, &sp->z, v3df_none);

    explosion = SpawnSprite(STAT_MISSILE, FIREBALL_EXP, s_FireballExp, sp->sectnum,
                            sp->x, sp->y, sp->z, sp->ang, 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    exp->xrepeat = 52;
    exp->yrepeat = 52;
    SetOwner(sp->owner, explosion);
    exp->shade = -40;
    exp->pal = eu->spal = u->spal;
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    SET(eu->Flags, TEST(u->Flags,SPR_UNDERWATER));
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    //
    // All this stuff assures that explosions do not go into floors &
    // ceilings
    //

    SpawnExpZadjust(Weapon, exp, Z(15), Z(15));

    if (RANDOM_P2(1024) < 150)
        SpawnFireballFlames(explosion,-1);
    return explosion;
}

int
SpawnGoroFireballExp(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;

    ASSERT(u);

    if (TEST(u->Flags, SPR_SUICIDE))
        return -1;

    PlaySound(DIGI_MEDIUMEXP, &sp->x, &sp->y, &sp->z, v3df_none);

    explosion = SpawnSprite(STAT_MISSILE, 0, s_FireballExp, sp->sectnum,
                            sp->x, sp->y, sp->z, sp->ang, 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    exp->xrepeat = 16;
    exp->yrepeat = 16;
    SetOwner(sp->owner, explosion);
    exp->shade = -40;
    exp->pal = eu->spal = u->spal;
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    //
    // All this stuff assures that explosions do not go into floors &
    // ceilings
    //

    SpawnExpZadjust(Weapon, exp, Z(15), Z(15));
    return explosion;
}

int
SpawnBoltExp(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;

    ASSERT(u);

    if (u && TEST(u->Flags, SPR_SUICIDE))
        return -1;

    PlaySound(DIGI_BOLTEXPLODE, &sp->x, &sp->y, &sp->z, v3df_none);

    explosion = SpawnSprite(STAT_MISSILE, BOLT_EXP, s_BoltExp, sp->sectnum,
                            sp->x, sp->y, sp->z, sp->ang, 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    SetOwner(sp->owner, explosion);
    exp->shade = -40;
    exp->xrepeat = 76;
    exp->yrepeat = 76;
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    if (RANDOM_P2(1024) > 512)
        SET(exp->cstat, CSTAT_SPRITE_XFLIP);
    eu->Radius = DamageData[DMG_BOLT_EXP].radius;

    SpawnExpZadjust(Weapon, exp, Z(40), Z(40));

    DoExpDamageTest(explosion);

    SetExpQuake(explosion); // !JIM! made rocket launcher shake things
    SpawnVis(-1, exp->sectnum, exp->x, exp->y, exp->z, 16);

    return explosion;
}

int
SpawnBunnyExp(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;

    ASSERT(u);

    if (u && TEST(u->Flags, SPR_SUICIDE))
        return -1;

    PlaySound(DIGI_BUNNYDIE3, &sp->x, &sp->y, &sp->z, v3df_none);

    u->ID = BOLT_EXP; // Change id
    InitBloodSpray(Weapon,TRUE,-1);
    InitBloodSpray(Weapon,TRUE,-1);
    InitBloodSpray(Weapon,TRUE,-1);
    DoExpDamageTest(Weapon);

    return 0;
}

int
SpawnTankShellExp(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;

    ASSERT(u);

    if (u && TEST(u->Flags, SPR_SUICIDE))
        return -1;

    PlaySound(DIGI_BOLTEXPLODE, &sp->x, &sp->y, &sp->z, v3df_none);

    explosion = SpawnSprite(STAT_MISSILE, TANK_SHELL_EXP, s_TankShellExp, sp->sectnum,
                            sp->x, sp->y, sp->z, sp->ang, 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    SetOwner(sp->owner, explosion);
    exp->shade = -40;
    exp->xrepeat = 64+32;
    exp->yrepeat = 64+32;
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    if (RANDOM_P2(1024) > 512)
        SET(exp->cstat, CSTAT_SPRITE_XFLIP);
    eu->Radius = DamageData[DMG_TANK_SHELL_EXP].radius;

    SpawnExpZadjust(Weapon, exp, Z(40), Z(40));
    DoExpDamageTest(explosion);
    SpawnVis(-1, exp->sectnum, exp->x, exp->y, exp->z, 16);

    return explosion;
}


int
SpawnNuclearSecondaryExp(int16_t Weapon, short ang)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;

    ASSERT(u);

    explosion = SpawnSprite(STAT_MISSILE, GRENADE_EXP, s_GrenadeExp, sp->sectnum,
                            sp->x, sp->y, sp->z, sp->ang, 512);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    SetOwner(sp->owner, explosion);
    exp->shade = -128;
    exp->xrepeat = 218;
    exp->yrepeat = 152;
    exp->clipdist = sp->clipdist;
    eu->ceiling_dist = Z(16);
    eu->floor_dist = Z(16);
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    //ang = RANDOM_P2(2048);
    vel = (2048+128) + RANDOM_RANGE(2048);
    eu->xchange = MOVEx(vel, ang);
    eu->ychange = MOVEy(vel, ang);
    eu->Radius = 200; // was NUKE_RADIUS
    eu->ret = move_missile(explosion, eu->xchange, eu->ychange, 0,
                           eu->ceiling_dist, eu->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    if (FindDistance3D(exp->x - sp->x, exp->y - sp->y, (exp->z - sp->z)>>4) < 1024)
    {
        KillSprite(explosion);
        return -1;
    }

    SpawnExpZadjust(Weapon, exp, Z(50), Z(10));

    InitChemBomb(explosion);

    return explosion;
}

int
SpawnNuclearExp(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion,ang=0;
    PLAYERp pp = NULL;
    short rnd_rng;

    ASSERT(u);
    if (u && TEST(u->Flags, SPR_SUICIDE))
        return -1;

    PlaySound(DIGI_NUCLEAREXP, &sp->x, &sp->y, &sp->z, v3df_dontpan | v3df_doppler);

    if (sp->owner)
    {
        pp = User[sp->owner]->PlayerP;
        rnd_rng = RANDOM_RANGE(1000);

        if (rnd_rng > 990)
            PlayerSound(DIGI_LIKEHIROSHIMA,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
        else if (rnd_rng > 980)
            PlayerSound(DIGI_LIKENAGASAKI,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
        else if (rnd_rng > 970)
            PlayerSound(DIGI_LIKEPEARL,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
    }

    // Spawn big mushroom cloud
    explosion = SpawnSprite(STAT_MISSILE, MUSHROOM_CLOUD, s_NukeMushroom, sp->sectnum,
                            sp->x, sp->y, sp->z, sp->ang, 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    SetOwner(sp->owner, explosion);
    exp->shade = -128;
    exp->xrepeat = 255;
    exp->yrepeat = 255;
    exp->clipdist = sp->clipdist;
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    eu->spal = exp->pal = PALETTE_PLAYER1;  // Set nuke puff to gray

    InitChemBomb(explosion);


    // Do central explosion
    explosion = SpawnSprite(STAT_MISSILE, MUSHROOM_CLOUD, s_GrenadeExp, sp->sectnum,
                            sp->x, sp->y, sp->z, sp->ang, 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    SetOwner(sp->owner, explosion);
    exp->shade = -128;
    exp->xrepeat = 218;
    exp->yrepeat = 152;
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    if (RANDOM_P2(1024) > 512)
        SET(exp->cstat, CSTAT_SPRITE_XFLIP);

    eu->Radius = NUKE_RADIUS;

    SpawnExpZadjust(Weapon, exp, Z(30), Z(30));

    DoExpDamageTest(explosion);

    // Nuclear effects
    SetNuclearQuake(explosion);

//  if (pp->NightVision)
//      {
//      SetFadeAmt(pp, -300, 1); // Idiot had night vision on in nuke flash
//      PlayerUpdateHealth(pp,-25);  // Just burned your eyes out of their sockets!
//      }
//    else
    SetFadeAmt(pp, -80, 1); // Nuclear flash

    // Secondary blasts
    ang = RANDOM_P2(2048);
    SpawnNuclearSecondaryExp(explosion, ang);
    ang = ang + 512 + RANDOM_P2(256);
    SpawnNuclearSecondaryExp(explosion, ang);
    ang = ang + 512 + RANDOM_P2(256);
    SpawnNuclearSecondaryExp(explosion, ang);
    ang = ang + 512 + RANDOM_P2(256);
    SpawnNuclearSecondaryExp(explosion, ang);

    return explosion;
}

int
SpawnTracerExp(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;

    ASSERT(u);
    if (u && TEST(u->Flags, SPR_SUICIDE))
        return -1;

    if (u->ID == BOLT_THINMAN_R1)
        explosion = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R1, s_TracerExp, sp->sectnum,
                                sp->x, sp->y, sp->z, sp->ang, 0);
    else
        explosion = SpawnSprite(STAT_MISSILE, TRACER_EXP, s_TracerExp, sp->sectnum,
                                sp->x, sp->y, sp->z, sp->ang, 0);

    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    SetOwner(sp->owner, explosion);
    exp->shade = -40;
    exp->xrepeat = 4;
    exp->yrepeat = 4;
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    if (RANDOM_P2(1024) > 512)
        SET(exp->cstat, CSTAT_SPRITE_XFLIP);

    if (u->ID == BOLT_THINMAN_R1)
    {
        eu->Radius = DamageData[DMG_BASIC_EXP].radius;
        DoExpDamageTest(explosion);
    }
    else
        eu->Radius = DamageData[DMG_BOLT_EXP].radius;


    return explosion;
}

int
SpawnMicroExp(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;

    ASSERT(u);
    if (u && TEST(u->Flags, SPR_SUICIDE))
        return -1;

//    PlaySound(DIGI_MISSLEXP, &sp->x, &sp->y, &sp->z, v3df_none);

    explosion = SpawnSprite(STAT_MISSILE, MICRO_EXP, s_MicroExp, sp->sectnum,
                            sp->x, sp->y, sp->z, sp->ang, 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    SetOwner(sp->owner, explosion);
    exp->shade = -40;
    exp->xrepeat = 32;
    exp->yrepeat = 32;
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    if (RANDOM_P2(1024) > 512)
        SET(exp->cstat, CSTAT_SPRITE_XFLIP);
    if (RANDOM_P2(1024) > 512)
        SET(exp->cstat, CSTAT_SPRITE_YFLIP);
    eu->Radius = DamageData[DMG_BOLT_EXP].radius;

    //
    // All this stuff assures that explosions do not go into floors &
    // ceilings
    //

    SpawnExpZadjust(Weapon, exp, Z(20), Z(20));
    SpawnVis(-1, exp->sectnum, exp->x, exp->y, exp->z, 16);

    return explosion;
}

int
AddSpriteToSectorObject(short SpriteNum, SECTOR_OBJECTp sop)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    unsigned sn;

    // make sure it has a user
    if (!u)
    {
        u = SpawnUser(SpriteNum, 0, NULL);
    }

    // find a free place on this list
    for (sn = 0; sn < SIZ(sop->sp_num); sn++)
    {
        if (sop->sp_num[sn] == -1)
            break;
    }

    ASSERT(sn < SIZ(sop->sp_num) - 1);
    sop->sp_num[sn] = SpriteNum;

    SET(u->Flags, SPR_ON_SO_SECTOR|SPR_SO_ATTACHED);

    u->sx = sop->xmid - sp->x;
    u->sy = sop->ymid - sp->y;
    u->sz = sector[sop->mid_sector].floorz - sp->z;

    u->sang = sp->ang;
    return 0;
}

int
SpawnBigGunFlames(int16_t Weapon, int16_t Operator, SECTOR_OBJECTp sop)
{
    SPRITEp sp,shp;
    USERp u,shu;
    SPRITEp exp;
    USERp eu;
    short explosion,shoot_pt;
    unsigned sn;
    SWBOOL smallflames = FALSE;

    if (Weapon < 0)
    {
        Weapon  = abs(Weapon);
        smallflames = TRUE;
    }

    sp = &sprite[Weapon];
    u = User[Weapon];

    explosion = SpawnSprite(STAT_MISSILE, MICRO_EXP, s_BigGunFlame, sp->sectnum,
                            sp->x, sp->y, sp->z, sp->ang, 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    SetOwner(Operator, explosion);
    exp->shade = -40;
    if (smallflames)
    {
        exp->xrepeat = 12;
        exp->yrepeat = 12;
    }
    else
    {
        exp->xrepeat = 34;
        exp->yrepeat = 34;
    }
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    if (RANDOM_P2(1024) > 512)
        SET(exp->cstat, CSTAT_SPRITE_XFLIP);
    if (RANDOM_P2(1024) > 512)
        SET(exp->cstat, CSTAT_SPRITE_YFLIP);

    // place all sprites on list
    for (sn = 0; sn < SIZ(sop->sp_num); sn++)
    {
        if (sop->sp_num[sn] == -1)
            break;
    }

    ASSERT(sn < SIZ(sop->sp_num) - 1);
    sop->sp_num[sn] = explosion;

    // Place sprite exactly where shoot point is
    //exp->x = eu->ox = sop->xmid - u->sx;
    //exp->y = eu->oy = sop->ymid - u->sy;

    SET(eu->Flags, TEST(u->Flags, SPR_ON_SO_SECTOR|SPR_SO_ATTACHED));

    if (TEST(u->Flags, SPR_ON_SO_SECTOR))
    {
        // move with sector its on
        exp->z = eu->oz = sector[sp->sectnum].floorz - u->sz;
    }
    else
    {
        // move with the mid sector
        exp->z = eu->oz = sector[sop->mid_sector].floorz - u->sz;
    }

    eu->sx = u->sx;
    eu->sy = u->sy;
    eu->sz = u->sz;

    return explosion;
}

int
SpawnGrenadeSecondaryExp(int16_t Weapon, short ang)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;
    int vel;


    ASSERT(u);
    explosion = SpawnSprite(STAT_MISSILE, GRENADE_EXP, s_GrenadeSmallExp, sp->sectnum,
                            sp->x, sp->y, sp->z, sp->ang, 1024);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    SetOwner(sp->owner, explosion);
    exp->shade = -40;
    exp->xrepeat = 32;
    exp->yrepeat = 32;
    exp->clipdist = sp->clipdist;
    eu->ceiling_dist = Z(16);
    eu->floor_dist = Z(16);
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    //ang = RANDOM_P2(2048);
    vel = (1024+512) + RANDOM_RANGE(1024);
    eu->xchange = MOVEx(vel, ang);
    eu->ychange = MOVEy(vel, ang);

    eu->ret = move_missile(explosion, eu->xchange, eu->ychange, 0,
                           eu->ceiling_dist, eu->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    if (FindDistance3D(exp->x - sp->x, exp->y - sp->y, (exp->z - sp->z)>>4) < 1024)
    {
        KillSprite(explosion);
        return -1;
    }

    SpawnExpZadjust(Weapon, exp, Z(50), Z(10));

    eu->ox = exp->x;
    eu->oy = exp->y;
    eu->oz = exp->z;

    return explosion;
}

int
SpawnGrenadeSmallExp(int16_t Weapon)
{
    short ang;

    ang = RANDOM_P2(2048);
    SpawnGrenadeSecondaryExp(Weapon, ang);
    return 0;
}

int
SpawnGrenadeExp(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;
    ANIMATOR InitMineShrap;
    short ang;
    int dx,dy,dz;

    ASSERT(u);
    if (u && TEST(u->Flags, SPR_SUICIDE))
        return -1;

    PlaySound(DIGI_30MMEXPLODE, &sp->x, &sp->y, &sp->z, v3df_none);

    if (RANDOM_RANGE(1000) > 990)
    {
        if (sp->owner >= 0 && User[sp->owner] && User[sp->owner]->PlayerP)
        {
            PLAYERp pp = User[sp->owner]->PlayerP;
            PlayerSound(DIGI_LIKEFIREWORKS,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
        }
    }

    dx = sp->x;
    dy = sp->y;
    dz = sp->z;

    if (u->ID == ZILLA_RUN_R0)
    {
        dx += RANDOM_RANGE(1000)-RANDOM_RANGE(1000);
        dy += RANDOM_RANGE(1000)-RANDOM_RANGE(1000);
        dz = SPRITEp_MID(sp) + RANDOM_RANGE(1000)-RANDOM_RANGE(1000);
    }

    explosion = SpawnSprite(STAT_MISSILE, GRENADE_EXP, s_GrenadeExp, sp->sectnum,
                            dx, dy, dz, sp->ang, 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    SetOwner(sp->owner, explosion);
    exp->shade = -40;
    exp->xrepeat = 64 + 32;
    exp->yrepeat = 64 + 32;
    exp->clipdist = sp->clipdist;
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    eu->Radius = DamageData[DMG_GRENADE_EXP].radius;

    //
    // All this stuff assures that explosions do not go into floors &
    // ceilings
    //

    SpawnExpZadjust(Weapon, exp, Z(100), Z(30));

    DoExpDamageTest(explosion);
    //InitMineShrap(explosion);

    SetExpQuake(explosion);
    SpawnVis(-1, exp->sectnum, exp->x, exp->y, exp->z, 0);

#if 0
    ang = RANDOM_P2(2048);
    SpawnGrenadeSecondaryExp(explosion, ang);
    ang = ang + 512 + RANDOM_P2(256);
    SpawnGrenadeSecondaryExp(explosion, ang);
    ang = ang + 512 + RANDOM_P2(256);
    SpawnGrenadeSecondaryExp(explosion, ang);
    ang = ang + 512 + RANDOM_P2(256);
    SpawnGrenadeSecondaryExp(explosion, ang);
#endif

    return explosion;
}

void SpawnExpZadjust(short Weapon, SPRITEp exp, int upper_zsize, int lower_zsize)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    USERp eu = User[exp - sprite];
    int tos_z, bos_z;

    ASSERT(eu);

    if (u)
    {
        tos_z = exp->z - upper_zsize;
        bos_z = exp->z + lower_zsize;

        if (tos_z <= u->hiz + Z(4))
        {
            exp->z = u->hiz + upper_zsize;
            SET(exp->cstat, CSTAT_SPRITE_YFLIP);
        }
        else if (bos_z > u->loz)
        {
            exp->z = u->loz - lower_zsize;
        }
    }
    else
    {
        int cz,fz;

        getzsofslope(exp->sectnum, exp->x, exp->y, &cz, &fz);

        tos_z = exp->z - upper_zsize;
        bos_z = exp->z + lower_zsize;

        if (tos_z <= cz + Z(4))
        {
            exp->z = cz + upper_zsize;
            SET(exp->cstat, CSTAT_SPRITE_YFLIP);
        }
        else if (bos_z > fz)
        {
            exp->z = fz - lower_zsize;
        }
    }

    eu->oz = exp->z;
}
int
SpawnMineExp(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;

    ASSERT(u);
    if (u && TEST(u->Flags, SPR_SUICIDE))
        return -1;

    change_sprite_stat(Weapon, STAT_MISSILE);

    PlaySound(DIGI_MINEBLOW, &sp->x, &sp->y, &sp->z, v3df_none);

    explosion = SpawnSprite(STAT_MISSILE, MINE_EXP, s_MineExp, sp->sectnum,
                            sp->x, sp->y, sp->z, sp->ang, 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    SetOwner(sp->owner, explosion);
    exp->shade = -40;
    exp->xrepeat = 64 + 44;
    exp->yrepeat = 64 + 44;
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    eu->Radius = DamageData[DMG_MINE_EXP].radius;

    //
    // All this stuff assures that explosions do not go into floors &
    // ceilings
    //

    SpawnExpZadjust(Weapon, exp, Z(100), Z(20));
    SpawnVis(-1, exp->sectnum, exp->x, exp->y, exp->z, 16);

    SetExpQuake(explosion);

    //DoExpDamageTest(explosion);

    return explosion;
}


int
InitMineShrap(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SPRITEp wp, hsp;
    USERp wu;
    short ang, w, i;
    hitdata_t hitinfo;
    int daz, nz;
    int nx,ny;

    for (i = 0; i < 18; i++)
    {
        ang = RANDOM_P2(2048);
        daz = Z(RANDOM_P2(48))<<3;
        daz -= DIV2(Z(48)<<3);

        FAFhitscan(sp->x, sp->y, sp->z - Z(30), sp->sectnum,   // Start position
                   sintable[NORM_ANGLE(ang + 512)],        // X vector of 3D ang
                   sintable[NORM_ANGLE(ang)],              // Y vector of 3D ang
                   daz,                                    // Z vector of 3D ang
                   &hitinfo, CLIPMASK_MISSILE);

        if (hitinfo.sect < 0)
            continue;


        // check to see what you hit
        if (hitinfo.sprite < 0 && hitinfo.wall < 0)
        {
        }

#define MINE_SHRAP_DIST_MAX 20000
        if (Distance(hitinfo.pos.x, hitinfo.pos.y, sp->x, sp->y) > MINE_SHRAP_DIST_MAX)
            continue;

        // hit a sprite?
        if (hitinfo.sprite >= 0)
        {
            hsp = &sprite[hitinfo.sprite];
        }

        w = SpawnSprite(STAT_MISSILE, MINE_SHRAP, s_MineSpark, hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, ang, 0);
        wp = &sprite[w];
        wp->shade = -40;
        wp->hitag = LUMINOUS; //Always full brightness
        SetOwner(sp->owner, w);
        SET(wp->cstat, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);

        wp->clipdist = 32 >> 2;

        HitscanSpriteAdjust(w, hitinfo.wall);
    }

    return 0;
}

int DoMineExp(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    DoExpDamageTest(SpriteNum);
    //InitMineShrap(SpriteNum);

    return 0;
}

int
DoSectorExp(int16_t SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    sp->x += u->xchange;
    sp->y += u->ychange;

    return 0;
}

int
SpawnSectorExp(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;

    ASSERT(u);
    if (TEST(u->Flags, SPR_SUICIDE))
        return -1;

    PlaySound(DIGI_30MMEXPLODE, &sp->x, &sp->y, &sp->z, v3df_none);

    explosion = SpawnSprite(STAT_MISSILE, GRENADE_EXP, s_SectorExp, sp->sectnum,
                            sp->x, sp->y, sp->z, sp->ang, 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    exp->owner = -1;
    exp->shade = -40;
    exp->xrepeat = 90; // was 40,40
    exp->yrepeat = 90;
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    eu->Radius = DamageData[DMG_SECTOR_EXP].radius;

    DoExpDamageTest(explosion);
    SetExpQuake(explosion);
    SpawnVis(-1, exp->sectnum, exp->x, exp->y, exp->z, 16);

    return explosion;
}

// called from SpawnShrap
int
SpawnLargeExp(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;

    PlaySound(DIGI_30MMEXPLODE, &sp->x, &sp->y, &sp->z, v3df_none);

    explosion = SpawnSprite(STAT_MISSILE, GRENADE_EXP, s_SectorExp, sp->sectnum,
                            sp->x, sp->y, sp->z, sp->ang, 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    exp->owner = -1;
    exp->shade = -40;
    exp->xrepeat = 90; // was 40,40
    exp->yrepeat = 90;
    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    eu->Radius = DamageData[DMG_SECTOR_EXP].radius;

    SpawnExpZadjust(Weapon, exp, Z(50), Z(50));

    // Should not cause other sectors to explode
    DoExpDamageTest(explosion);
    SetExpQuake(explosion);
    SpawnVis(-1, exp->sectnum, exp->x, exp->y, exp->z, 16);

    return explosion;
}

int
SpawnMeteorExp(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;

    ASSERT(u);
    if (TEST(u->Flags, SPR_SUICIDE))
        return -1;

    if (u->spal == 25)    // Serp ball
    {
        explosion = SpawnSprite(STAT_MISSILE, METEOR_EXP, s_TeleportEffect2, sp->sectnum,
                                sp->x, sp->y, sp->z, sp->ang, 0);
    }
    else
    {
        PlaySound(DIGI_MEDIUMEXP, &sp->x, &sp->y, &sp->z, v3df_none);
        explosion = SpawnSprite(STAT_MISSILE, METEOR_EXP, s_MeteorExp, sp->sectnum,
                                sp->x, sp->y, sp->z, sp->ang, 0);
    }

    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    exp->owner = -1;
    exp->shade = -40;
    if (sp->yrepeat < 64)
    {
        // small
        exp->xrepeat = 64;
        exp->yrepeat = 64;
    }
    else
    {
        // large - boss
        exp->xrepeat = 80;
        exp->yrepeat = 80;
    }

    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    eu->Radius = DamageData[DMG_BASIC_EXP].radius;

    return explosion;
}

int
SpawnLittleExp(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;

    PlaySound(DIGI_HEADSHOTHIT, &sp->x, &sp->y, &sp->z, v3df_none);
    explosion = SpawnSprite(STAT_MISSILE, BOLT_EXP, s_SectorExp, sp->sectnum,
                            sp->x, sp->y, sp->z, sp->ang, 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    exp->owner = -1;
    exp->shade = -127;

    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    eu->Radius = DamageData[DMG_BASIC_EXP].radius;
    DoExpDamageTest(explosion);
    SpawnVis(-1, exp->sectnum, exp->x, exp->y, exp->z, 16);

    return explosion;
}

int
DoFireball(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int32_t dax, day, daz;
    short NewWeapon;

    int offset;

    u = User[Weapon];

    ASSERT(Weapon >= 0);

    if (TEST(u->Flags, SPR_UNDERWATER))
    {
        sp->xrepeat = sp->yrepeat -= 1;
        if (sp->xrepeat <= 37)
        {
            SpawnSmokePuff(Weapon);
            KillSprite(Weapon);
            return TRUE;
        }
    }

#if 0
    ox = sp->x;
    oy = sp->y;
    oz = sp->z;
    os = sp->sectnum
#endif

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

#if 0
    if (u->ret)
    {
        u->ret = 0;
        xv = u->xchange/2;
        yv = u->ychange/2;
        zv = u->zchange/2;

        u->ret = move_missile(Weapon, xv, yv, zv, Z(16), Z(16), CLIPMASK_MISSILE, MISSILEMOVETICS);
    }
#endif

    MissileHitDiveArea(Weapon);

    if (u->ret)
    {
        char hit_burn = 0;

        if (WeaponMoveHit(Weapon))
        {
            switch (TEST(u->ret, HIT_MASK))
            {
            case HIT_SPRITE:
            {
                SPRITEp hsp;
                USERp hu;

                hsp = &sprite[NORM_SPRITE(u->ret)];
                hu = User[NORM_SPRITE(u->ret)];

                if (TEST(hsp->extra, SPRX_BURNABLE))
                {
                    if (!hu)
                        hu = SpawnUser(hsp - sprite, hsp->picnum, NULL);
                    SpawnFireballFlames(Weapon, hsp - sprite);
                    hit_burn = TRUE;
                }

                break;
            }
            }

            if (!hit_burn)
            {
                if (u->ID == GORO_FIREBALL)
                    SpawnGoroFireballExp(Weapon);
                else
                    SpawnFireballExp(Weapon);
            }

            KillSprite(Weapon);

            return TRUE;
        }
    }

    return FALSE;

}

int
DoFindGround(int16_t SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], hsp;
    USERp u = User[SpriteNum];
    int ceilhit, florhit;
    short save_cstat;
    SWBOOL found = FALSE;
    short bak_cstat;

    // recursive routine to find the ground - either sector or floor sprite
    // skips over enemy and other types of sprites

    save_cstat = sp->cstat;
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    FAFgetzrange(sp->x, sp->y, sp->z, sp->sectnum, &u->hiz, &ceilhit, &u->loz, &florhit, (((int) sp->clipdist) << 2) - GETZRANGE_CLIP_ADJ, CLIPMASK_PLAYER);
    sp->cstat = save_cstat;

    ASSERT(TEST(florhit, HIT_SPRITE | HIT_SECTOR));

    switch (TEST(florhit, HIT_MASK))
    {
    case HIT_SPRITE:
    {
        hsp = &sprite[NORM_SPRITE(florhit)];

        if (TEST(hsp->cstat, CSTAT_SPRITE_FLOOR))
        {
            // found a sprite floor
            u->lo_sp = hsp;
            u->lo_sectp = NULL;
            return TRUE;
        }
        else
        {
            // reset the blocking bit of what you hit and try again -
            // recursive
            bak_cstat = hsp->cstat;
            RESET(hsp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
            DoFindGround(SpriteNum);
            hsp->cstat = bak_cstat;
        }

        return FALSE;
    }
    case HIT_SECTOR:
    {
        u->lo_sectp = &sector[NORM_SECTOR(florhit)];
        u->lo_sp = NULL;
        return TRUE;
    }

    default:
        ASSERT(TRUE == FALSE);
        break;
    }

    return FALSE;
}

int
DoFindGroundPoint(int16_t SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], hsp;
    USERp u = User[SpriteNum];
    int ceilhit, florhit;
    short save_cstat;
    SWBOOL found = FALSE;
    short bak_cstat;

    // recursive routine to find the ground - either sector or floor sprite
    // skips over enemy and other types of sprites

    save_cstat = sp->cstat;
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    FAFgetzrangepoint(sp->x, sp->y, sp->z, sp->sectnum, &u->hiz, &ceilhit, &u->loz, &florhit);
    sp->cstat = save_cstat;

    ASSERT(TEST(florhit, HIT_SPRITE | HIT_SECTOR));

    switch (TEST(florhit, HIT_MASK))
    {
    case HIT_SPRITE:
    {
        hsp = &sprite[NORM_SPRITE(florhit)];

        if (TEST(hsp->cstat, CSTAT_SPRITE_FLOOR))
        {
            // found a sprite floor
            u->lo_sp = hsp;
            u->lo_sectp = NULL;
            return TRUE;
        }
        else
        {
            // reset the blocking bit of what you hit and try again -
            // recursive
            bak_cstat = hsp->cstat;
            RESET(hsp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
            DoFindGroundPoint(SpriteNum);
            hsp->cstat = bak_cstat;
        }

        return FALSE;
    }
    case HIT_SECTOR:
    {
        u->lo_sectp = &sector[NORM_SECTOR(florhit)];
        u->lo_sp = NULL;
        return TRUE;
    }

    default:
        ASSERT(TRUE == FALSE);
        break;
    }

    return FALSE;
}

int
DoNapalm(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon], exp;
    USERp u = User[Weapon];
    short explosion;
    int offset;
    int ox, oy, oz;

    DoBlurExtend(Weapon, 1, 7);

    u = User[Weapon];

    if (TEST(u->Flags, SPR_UNDERWATER))
    {
        sp->xrepeat = sp->yrepeat -= 1;
        if (sp->xrepeat <= 30)
        {
            SpawnSmokePuff(Weapon);
            KillSprite(Weapon);
            return TRUE;
        }
    }

    ox = sp->x;
    oy = sp->y;
    oz = sp->z;

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);

    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);

    if (u->ret)
    {
        // this sprite is suPlayerosed to go through players/enemys
        // if hit a player/enemy back up and do it again with blocking reset
        if (TEST(u->ret, HIT_MASK) == HIT_SPRITE)
        {
            SPRITEp hsp = &sprite[NORM_SPRITE(u->ret)];

            if (TEST(hsp->cstat, CSTAT_SPRITE_BLOCK) && !TEST(hsp->cstat, CSTAT_SPRITE_WALL))
            {
                short hcstat = hsp->cstat;

                sp->x = ox;
                sp->y = oy;
                sp->z = oz;

                RESET(hsp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
                u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);
                hsp->cstat = hcstat;
            }
        }
    }

    u->Counter++;
    if (u->Counter > 2)
        u->Counter = 0;

    if (!u->Counter)
    {
        USERp eu;

        PlaySound(DIGI_NAPPUFF, &sp->x, &sp->y, &sp->z, v3df_none);

        explosion = SpawnSprite(STAT_MISSILE, NAP_EXP, s_NapExp, sp->sectnum,
                                sp->x, sp->y, sp->z, sp->ang, 0);

        exp = &sprite[explosion];
        eu = User[explosion];

        exp->hitag = LUMINOUS; //Always full brightness
        //exp->owner = sp->owner;
        SetOwner(sp->owner, explosion);
        exp->shade = -40;
        exp->cstat = sp->cstat;
        exp->xrepeat = 48;
        exp->yrepeat = 64;
        SET(exp->cstat, CSTAT_SPRITE_YCENTER);
        if (RANDOM_P2(1024) < 512)
            SET(exp->cstat, CSTAT_SPRITE_XFLIP);
        RESET(exp->cstat, CSTAT_SPRITE_TRANSLUCENT);
        eu->Radius = 1500;

        DoFindGroundPoint(explosion);
        MissileWaterAdjust(explosion);
        exp->z = eu->loz;
        eu->oz = exp->z;

        if (TEST(u->Flags, SPR_UNDERWATER))
            SET(eu->Flags, SPR_UNDERWATER);

        ASSERT(exp->picnum == 3072);
        ASSERT(eu->Tics == 0);
    }

    // DoDamageTest(Weapon);

    if (u->ret)
    {
        if (WeaponMoveHit(Weapon))
        {
            KillSprite((short) Weapon);

            return TRUE;
        }
    }

    return FALSE;
}

#define WORM 1

#if WORM == 1
int
DoBloodWorm(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon], exp;
    USERp u = User[Weapon];
    int offset;
    int move_ground_missile(short spritenum, int xchange, int ychange, int zchange, int ceildist, int flordist, uint32_t cliptype, int numtics);
    short ang;
    int x,y,z,xvect,yvect;
    int bx,by;
    int amt;
    int SpawnZombie2(short);
    short sectnum;

    u = User[Weapon];

    u->ret = move_ground_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    if (u->ret)
    {
        u->xchange = -u->xchange;
        u->ychange = -u->ychange;
        u->ret = 0;
        sp->ang = NORM_ANGLE(sp->ang + 1024);
        return TRUE;
    }

    MissileHitDiveArea(Weapon);

    if (!u->z_tgt)
    {
        // stay alive for 10 seconds
        if (++u->Counter3 > 3)
        {
            SPRITEp tsp;
            USERp tu;
            int i,nexti;

            InitBloodSpray(Weapon, FALSE, 1);
            InitBloodSpray(Weapon, FALSE, 1);
            InitBloodSpray(Weapon, FALSE, 1);

            // Kill any old zombies you own
            TRAVERSE_SPRITE_STAT(headspritestat[STAT_ENEMY], i, nexti)
            {
                tsp = &sprite[i];
                tu = User[i];

                ASSERT(tu);

                if (tu->ID == ZOMBIE_RUN_R0 && tsp->owner == sp->owner)
                {
                    InitBloodSpray(i,TRUE,105);
                    InitBloodSpray(i,TRUE,105);
                    SetSuicide(i);
                    break;
                }
            }

            SpawnZombie2(Weapon);
            KillSprite(Weapon);
            return TRUE;
        }
    }

    ang = NORM_ANGLE(sp->ang + 512);

    xvect = sintable[NORM_ANGLE(ang+512)];
    yvect = sintable[ang];

    bx = sp->x;
    by = sp->y;

    amt = RANDOM_P2(2048) - 1024;
    sp->x += mulscale15(amt,xvect);
    sp->y += mulscale15(amt,yvect);

    sectnum = sp->sectnum;
    updatesectorz(sp->x, sp->y, sp->z, &sectnum);
    if (sectnum >= 0)
    {
        GlobalSkipZrange = TRUE;
        InitBloodSpray(Weapon, FALSE, 1);
        GlobalSkipZrange = FALSE;
    }

    sp->x = bx;
    sp->y = by;

    return FALSE;
}
#endif

#if WORM == 2
int
DoBloodWorm(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon], exp;
    USERp u = User[Weapon];
    int offset;
    int move_ground_missile(short spritenum, int xchange, int ychange, int zchange, int ceildist, int flordist, uint32_t cliptype, int numtics);
    short ang;
    int x,y,z,xvect,yvect;
    int bx,by;
    int amt;
    int SpawnZombie2(short);
    short sectnum;

    u = User[Weapon];

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    // stay on ground
    sp->z = u->loz - Z(32);

    if (u->ret)
    {
        switch (TEST(u->ret, HIT_MASK))
        {

        case HIT_PLAX_WALL:
            KillSprite(Weapon);
            return TRUE;

        case HIT_SPRITE:
        case HIT_WALL:
        {
            u->xchange = -u->xchange;
            u->ychange = -u->ychange;
            u->ret = 0;
            sp->ang = NORM_ANGLE(sp->ang + 1024);
            break;
        }

        case HIT_SECTOR:
        {
            sp->z -= Z(32);

            u->xchange = -u->xchange;
            u->ychange = -u->ychange;
            u->ret = 0;
            sp->ang = NORM_ANGLE(sp->ang + 1024);
            break;
        }

        }
    }

    if (++u->Counter3 > 3)
    {
        SPRITEp tsp;
        USERp tu;
        int i,nexti;

        InitBloodSpray(Weapon, FALSE, 1);
        InitBloodSpray(Weapon, FALSE, 1);

        // Kill any old zombies you own
        TRAVERSE_SPRITE_STAT(headspritestat[STAT_ENEMY], i, nexti)
        {
            tsp = &sprite[i];
            tu = User[i];

            ASSERT(tu);

            if (tu->ID == ZOMBIE_RUN_R0 && tsp->owner == sp->owner)
            {
                InitBloodSpray(i,TRUE,105);
                InitBloodSpray(i,TRUE,105);
                SetSuicide(i);
                break;
            }
        }

        SpawnZombie2(Weapon);
        KillSprite(Weapon);
        return TRUE;
    }

    ang = NORM_ANGLE(sp->ang + 512);

    xvect = sintable[NORM_ANGLE(ang+512)];
    yvect = sintable[ang];

    bx = sp->x;
    by = sp->y;

    amt = RANDOM_P2(2048) - 1024;
    sp->x += mulscale15(amt,xvect);
    sp->y += mulscale15(amt,yvect);

    sectnum = sp->sectnum;
    updatesectorz(sp->x, sp->y, sp->z, &sectnum);
    if (sectnum >= 0)
    {
        //GlobalSkipZrange = TRUE;
        InitBloodSpray(Weapon, FALSE, 1);
        //GlobalSkipZrange = FALSE;
    }

    if (RANDOM_P2(2048) < 512)
    {
        sp->x = bx;
        sp->y = by;

        amt = RANDOM_P2(2048) - 1024;
        sp->x += mulscale15(amt,xvect);
        sp->y += mulscale15(amt,yvect);

        sectnum = sp->sectnum;
        updatesectorz(sp->x, sp->y, sp->z, &sectnum);
        if (sectnum >= 0)
        {
            //GlobalSkipZrange = TRUE;
            InitBloodSpray(Weapon, FALSE, 1);
            //GlobalSkipZrange = FALSE;
        }
    }

    sp->x = bx;
    sp->y = by;

    return FALSE;
}
#endif

int
DoMeteor(int16_t Weapon)
{
    return FALSE;
}

int
DoSerpMeteor(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon], exp;
    USERp u = User[Weapon];
    short explosion;
    int offset;
    int ox, oy, oz;

    ox = sp->x;
    oy = sp->y;
    oz = sp->z;

    sp->xrepeat += MISSILEMOVETICS * 2;
    if (sp->xrepeat > 80)
        sp->xrepeat = 80;

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    if (u->ret)
    {
        // this sprite is supposed to go through players/enemys
        // if hit a player/enemy back up and do it again with blocking reset
        if (TEST(u->ret, HIT_MASK) == HIT_SPRITE)
        {
            SPRITEp hsp = &sprite[NORM_SPRITE(u->ret)];
            USERp hu = User[NORM_SPRITE(u->ret)];

            if (hu && hu->ID >= SKULL_R0 && hu->ID <= SKULL_SERP)
            {
                short hcstat = hsp->cstat;

                sp->x = ox;
                sp->y = oy;
                sp->z = oz;

                RESET(hsp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
                u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);
                hsp->cstat = hcstat;
            }
        }
    }

    if (u->ret)
    {
        if (WeaponMoveHit(Weapon))
        {
            SpawnMeteorExp(Weapon);

            KillSprite((short) Weapon);

            return TRUE;
        }
    }

    return FALSE;

}


int
DoMirvMissile(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon], exp;
    USERp u = User[Weapon];
    short explosion;
    int offset;

    u = User[Weapon];

    sp->xrepeat += MISSILEMOVETICS * 2;
    if (sp->xrepeat > 80)
        sp->xrepeat = 80;

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);

    if (u->ret)
    {
        if (WeaponMoveHit(Weapon))
        {
            SpawnMeteorExp(Weapon);

            KillSprite((short) Weapon);

            return TRUE;
        }
    }

    return FALSE;

}

int
DoMirv(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon], np;
    USERp u = User[Weapon], nu;
    short New;
    int offset;
    int ox, oy, oz;

    u = User[Weapon];

    ox = sp->x;
    oy = sp->y;
    oz = sp->z;

    u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(Weapon);

    if (TEST(u->Flags, SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(Weapon);

#if 0
    if (u->ret)
    {
        // this sprite is supposed to go through players/enemys
        // if hit a player/enemy back up and do it again with blocking reset
        if (TEST(u->ret, HIT_MASK) == HIT_SPRITE)
        {
            SPRITEp hsp = &sprite[NORM_SPRITE(u->ret)];

            if (TEST(hsp->cstat, CSTAT_SPRITE_BLOCK) && !TEST(hsp->cstat, CSTAT_SPRITE_WALL))
            {
                short hcstat = hsp->cstat;

                sp->x = ox;
                sp->y = oy;
                sp->z = oz;

                RESET(hsp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
                u->ret = move_missile(Weapon, u->xchange, u->ychange, u->zchange, u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS);
                hsp->cstat = hcstat;
            }
        }
    }
#endif

    u->Counter++;
    u->Counter &= 1;

    if (!u->Counter)
    {
        int i;
        static short angs[] =
        {
            512,
            -512
        };

        for (i = 0; i < 2; i++)
        {
            New = SpawnSprite(STAT_MISSILE, MIRV_METEOR, &sg_MirvMeteor[0][0], sp->sectnum,
                              sp->x, sp->y, sp->z, NORM_ANGLE(sp->ang + angs[i]), 800);

            np = &sprite[New];
            nu = User[New];

            nu->RotNum = 5;
            NewStateGroup(New, &sg_MirvMeteor[0]);
            nu->StateEnd = s_MirvMeteorExp;

            //np->owner = Weapon;
            SetOwner(Weapon, New);
            np->shade = -40;
            np->xrepeat = 40;
            np->yrepeat = 40;
            np->clipdist = 32L >> 2;
            np->zvel = 0;
            SET(np->cstat, CSTAT_SPRITE_YCENTER);

            nu->ceiling_dist = Z(16);
            nu->floor_dist = Z(16);
            nu->Dist = 200;
            //nu->Dist = 0;

            nu->xchange = MOVEx(np->xvel, np->ang);
            nu->ychange = MOVEy(np->xvel, np->ang);
            nu->zchange = np->zvel;

            if (TEST(u->Flags, SPR_UNDERWATER))
                SET(nu->Flags, SPR_UNDERWATER);
        }

    }

    if (u->ret)
    {
        SpawnMeteorExp(Weapon);
        //SpawnBasicExp(Weapon);
        KillSprite((short) Weapon);
        return TRUE;
    }

    return FALSE;
}

SWBOOL
MissileSetPos(short Weapon, ANIMATORp DoWeapon, int dist)
{
    SPRITEp wp = &sprite[Weapon];
    USERp wu = User[Weapon];
    int oldspeed, oldvel, oldzvel;
    int oldxc, oldyc, oldzc;
    char retval = FALSE;

    // backup values
    oldxc = wu->xchange;
    oldyc = wu->ychange;
    oldzc = wu->zchange;
    oldvel = wp->xvel;
    oldzvel = wp->zvel;

    // make missile move in smaller increments
    wp->xvel = (dist * 6L) / MISSILEMOVETICS;
    //wp->zvel = (wp->zvel*4) / MISSILEMOVETICS;
    wp->zvel = (wp->zvel*6L) / MISSILEMOVETICS;

    // some Weapon Animators use this
    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    SET(wu->Flags, SPR_SET_POS_DONT_KILL);
    if ((*DoWeapon)(Weapon))
        retval = TRUE;
    RESET(wu->Flags, SPR_SET_POS_DONT_KILL);

    // reset values
    wu->xchange = oldxc;
    wu->ychange = oldyc;
    wu->zchange = oldzc;
    wp->xvel = oldvel;
    wp->zvel = oldzvel;

    // update for interpolation
    wu->ox = wp->x;
    wu->oy = wp->y;
    wu->oz = wp->z;

    return retval;
}

SWBOOL
TestMissileSetPos(short Weapon, ANIMATORp DoWeapon, int dist, int zvel)
{
    SPRITEp wp = &sprite[Weapon];
    USERp wu = User[Weapon];
    int oldspeed, oldvel, oldzvel;
    int oldxc, oldyc, oldzc;
    char retval = FALSE;

    // backup values
    oldxc = wu->xchange;
    oldyc = wu->ychange;
    oldzc = wu->zchange;
    oldvel = wp->xvel;
    oldzvel = wp->zvel;

    // make missile move in smaller increments
    wp->xvel = (dist * 6L) / MISSILEMOVETICS;
    //wp->zvel = (wp->zvel*4) / MISSILEMOVETICS;
    zvel = (zvel*6L) / MISSILEMOVETICS;

    // some Weapon Animators use this
    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = zvel;

    SET(wu->Flags, SPR_SET_POS_DONT_KILL);
    if ((*DoWeapon)(Weapon))
        retval = TRUE;
    RESET(wu->Flags, SPR_SET_POS_DONT_KILL);

    // reset values
    wu->xchange = oldxc;
    wu->ychange = oldyc;
    wu->zchange = oldzc;
    wp->xvel = oldvel;
    wp->zvel = oldzvel;

    // update for interpolation
    wu->ox = wp->x;
    wu->oy = wp->y;
    wu->oz = wp->z;

    return retval;
}

int
DoRing(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    PLAYERp pp = User[sp->owner]->PlayerP;
    SPRITEp so = &sprite[sp->owner];
    short sect;
    int cz,fz;

#define RINGMOVETICS (MISSILEMOVETICS * 2)
#define RING_OUTER_DIST 3200
#define RING_INNER_DIST 800

    ASSERT(sp->owner != -1);

    if (TEST(u->Flags, SPR_UNDERWATER))
    {
        sp->xrepeat = sp->yrepeat -= 1;
        if (sp->xrepeat <= 30)
        {
            SpawnSmokePuff(Weapon);
            KillSprite(Weapon);
            return TRUE;
        }
    }

    // move the center with the player
    sp->x = sprite[sp->owner].x;
    sp->y = sprite[sp->owner].y;
    if (User[sp->owner]->PlayerP)
        sp->z = pp->posz + Z(20);
    else
        sp->z = SPRITEp_MID(so) + Z(30);

    // go out until its time to come back in
    if (u->Counter2 == FALSE)
    {
        u->Dist += 8 * RINGMOVETICS;

        if (u->Dist > RING_OUTER_DIST)
        {
            u->Counter2 = TRUE;
        }
    }
    else
    {
        u->Dist -= 8 * RINGMOVETICS;

        if (u->Dist <= RING_INNER_DIST)
        {
            if (!User[sp->owner]->PlayerP)
                User[sp->owner]->Counter--;
            KillSprite(Weapon);
            return 0;
        }
    }

    //sp->ang = NORM_ANGLE(sp->ang - 512);

    // rotate the ring
    sp->ang = NORM_ANGLE(sp->ang + (4 * RINGMOVETICS) + RINGMOVETICS);

    // put it out there
    sp->x += ((int) u->Dist * (int) sintable[NORM_ANGLE(sp->ang + 512)]) >> 14;
    sp->y += ((int) u->Dist * (int) sintable[sp->ang]) >> 14;
    if (User[sp->owner]->PlayerP)
        sp->z += (u->Dist * ((100 - pp->horiz) * HORIZ_MULT)) >> 9;

    //sp->ang = NORM_ANGLE(sp->ang + 512);
    //updatesector(sp->x, sp->y);

    setsprite(Weapon, (vec3_t *)sp);
    sect = sp->sectnum;

    ASSERT(sp->sectnum >= 0);

    getzsofslope(sp->sectnum, sp->x, sp->y, &cz, &fz);

    // bound the sprite by the sectors ceiling and floor
    if (sp->z > fz)
    {
        sp->z = fz;
    }

    if (sp->z < cz + SPRITEp_SIZE_Z(sp))
    {
        sp->z = cz + SPRITEp_SIZE_Z(sp);
    }

    // Done last - check for damage
    DoDamageTest(Weapon);

    return 0;
}



void
InitSpellRing(PLAYERp pp)
{
    short ang, ang_diff, ang_start, ang_finish, SpriteNum, missiles;
    SPRITEp sp;
    USERp u, pu = User[pp->PlayerSprite];
    short max_missiles = 16;
    short ammo;

    ammo = NAPALM_MIN_AMMO;

    if (pp->WpnAmmo[WPN_HOTHEAD] < ammo)
        return;
    else
        PlayerUpdateAmmo(pp, WPN_HOTHEAD, -ammo);

    ang_diff = 2048 / max_missiles;

    ang_start = NORM_ANGLE(pp->pang - DIV2(2048));

    if (!SW_SHAREWARE)
        PlaySound(DIGI_RFWIZ, &pp->posx, &pp->posy, &pp->posz, v3df_none);

    for (missiles = 0, ang = ang_start; missiles < max_missiles; ang += ang_diff, missiles++)
    {
        SpriteNum = SpawnSprite(STAT_MISSILE_SKIP4, FIREBALL1, s_Ring, pp->cursectnum, pp->posx, pp->posy, pp->posz, ang, 0);

        sp = &sprite[SpriteNum];

        sp->hitag = LUMINOUS; //Always full brightness
        sp->xvel = 500;
        //sp->owner = pp->SpriteP - sprite;
        SetOwner(pp->SpriteP - sprite, SpriteNum);
        sp->shade = -40;
        sp->xrepeat = 32;
        sp->yrepeat = 32;
        sp->zvel = 0;

        u = User[SpriteNum];

        u->sz = Z(20);
        u->Dist = RING_INNER_DIST;
        u->Counter = max_missiles;
        u->Counter2 = 0;
        u->ceiling_dist = Z(10);
        u->floor_dist = Z(10);

        //u->RotNum = 5;
        //NewStateGroup(SpriteNum, &sg_Ring);
        //SET(u->Flags, SPR_XFLIP_TOGGLE);

        // put it out there
        sp->x += ((int) u->Dist * (int) sintable[NORM_ANGLE(sp->ang + 512)]) >> 14;
        sp->y += ((int) u->Dist * (int) sintable[sp->ang]) >> 14;
        sp->z = pp->posz + Z(20) + ((u->Dist * ((100 - pp->horiz) * HORIZ_MULT)) >> 9);

        sp->ang = NORM_ANGLE(sp->ang + 512);

        u->ox = sp->x;
        u->oy = sp->y;
        u->oz = sp->z;

        if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(sp))
            SET(u->Flags, SPR_UNDERWATER);
    }
}

int
DoSerpRing(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    USERp ou = User[sp->owner];
    short sect;
    int dist,a,b,c;
    int cz,fz;

    // if owner does not exist or he's dead on the floor
    // kill off all of his skull children
    if (sp->owner == -1 || ou->RotNum < 5)
    {
        UpdateSinglePlayKills(Weapon);
        DoSkullBeginDeath(Weapon);
        // +2 does not spawn shrapnel
        u->ID = SKULL_SERP;
        return 0;
    }

    // move the center with the player
    sp->x = sprite[sp->owner].x;
    sp->y = sprite[sp->owner].y;

    sp->z += sp->zvel;
    if (sp->z > sprite[sp->owner].z - u->sz)
        sp->z = sprite[sp->owner].z - u->sz;

    // go out until its time to come back in
    if (u->Counter2 == FALSE)
    {
        u->Dist += 8 * RINGMOVETICS;

        if (u->Dist > u->TargetDist)
            u->Counter2 = TRUE;
    }

    // rotate the ring
    u->slide_ang = NORM_ANGLE(u->slide_ang + sp->yvel);

    // rotate the heads
    if (TEST(u->Flags, SPR_BOUNCE))
        sp->ang = NORM_ANGLE(sp->ang + (28 * RINGMOVETICS));
    else
        sp->ang = NORM_ANGLE(sp->ang - (28 * RINGMOVETICS));

    // put it out there
    sp->x += ((int) u->Dist * (int) sintable[NORM_ANGLE(u->slide_ang + 512)]) >> 14;
    sp->y += ((int) u->Dist * (int) sintable[u->slide_ang]) >> 14;

    setsprite(Weapon, (vec3_t *)sp);
    sect = sp->sectnum;

    ASSERT(sp->sectnum >= 0);

    getzsofslope(sp->sectnum, sp->x, sp->y, &cz, &fz);

    // bound the sprite by the sectors ceiling and floor
    if (sp->z > fz)
    {
        sp->z = fz;
    }

    if (sp->z < cz + SPRITEp_SIZE_Z(sp))
    {
        sp->z = cz + SPRITEp_SIZE_Z(sp);
    }

    if (u->Counter2 > 0)
    {
        if (!User[ou->tgt_sp-sprite] ||
            !User[ou->tgt_sp-sprite]->PlayerP ||
            !TEST(User[ou->tgt_sp-sprite]->PlayerP->Flags, PF_DEAD))
        {
            u->tgt_sp = ou->tgt_sp;
            DISTANCE(sp->x, sp->y, u->tgt_sp->x, u->tgt_sp->y, dist, a,b,c);

            // if ((dist ok and random ok) OR very few skulls left)
            if ((dist < 18000 && (RANDOM_P2(2048<<5)>>5) < 16) || User[sp->owner]->Counter < 4)
            {
                short sectnum = sp->sectnum;
                COVERupdatesector(sp->x,sp->y,&sectnum);

                // if (valid sector and can see target)
                if (sectnum != -1 && CanSeePlayer(Weapon))
                {
                    extern STATEp sg_SkullJump[];
                    u->ID = SKULL_R0;
                    sp->ang = getangle(u->tgt_sp->x - sp->x, u->tgt_sp->y - sp->y);
                    sp->xvel = dist>>5;
                    sp->xvel += DIV2(sp->xvel);
                    sp->xvel += (RANDOM_P2(128<<8)>>8);
                    u->jump_speed = -800;
                    change_sprite_stat(Weapon, STAT_ENEMY);
                    NewStateGroup(Weapon, sg_SkullJump);
                    DoBeginJump(Weapon);
                    // tell owner that one is gone
                    // User[sp->owner]->Counter--;
                    return 0;
                }
            }
        }
    }

    // Done last - check for damage
    //DoDamageTest(Weapon);

    // if its exploded
#if 0
    if (u->RotNum == 0)
    {
        // tell owner that one is gone
        User[sp->owner]->Counter--;
    }
#endif

    return 0;
}

int
InitLavaFlame(short SpriteNum)
{
    return 0;
}

int
InitLavaThrow(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    int nx, ny, nz, dist, nang;
    short w;

    //PlaySound(DIGI_NINJAROCKETATTACK, &sp->x, &sp->y, &sp->z, v3df_none);

    // get angle to player and also face player when attacking
    sp->ang = nang = getangle(u->tgt_sp->x - sp->x, u->tgt_sp->y - sp->y);

    nx = sp->x;
    ny = sp->y;
    nz = SPRITEp_TOS(sp) + DIV4(SPRITEp_SIZE_Z(sp));

    // Spawn a shot
    w = SpawnSprite(STAT_MISSILE, LAVA_BOULDER, s_LavaBoulder, sp->sectnum,
                    nx, ny, nz, nang, NINJA_BOLT_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    SetOwner(SpriteNum, w);
    wp->hitag = LUMINOUS; //Always full brightness
    wp->yrepeat = 72;
    wp->xrepeat = 72;
    wp->shade = -15;
    wp->zvel = 0;
    wp->ang = nang;

    if (RANDOM_P2(1024) > 512)
        SET(wp->cstat, CSTAT_SPRITE_XFLIP);
    if (RANDOM_P2(1024) > 512)
        SET(wp->cstat, CSTAT_SPRITE_YFLIP);

    wu->Radius = 200;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    wp->clipdist = 256>>2;
    wu->ceiling_dist = Z(14);
    wu->floor_dist = Z(14);

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    MissileSetPos(w, DoLavaBoulder, 1200);

    // find the distance to the target (player)
    dist = Distance(wp->x, wp->y, u->tgt_sp->x, u->tgt_sp->y);

    if (dist != 0)
        wu->zchange = wp->zvel = (wp->xvel * (SPRITEp_UPPER(u->tgt_sp) - wp->z)) / dist;

    return w;
}

int
InitVulcanBoulder(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    int nx, ny, nz, dist, nang;
    short w;
    int zsize;
    int zvel, zvel_rand;
    short delta;
    short vel;

    nx = sp->x;
    ny = sp->y;
    nz = sp->z - Z(40);

    if (SP_TAG7(sp))
    {
        delta = SP_TAG5(sp);
        nang = sp->ang + (RANDOM_RANGE(delta) - DIV2(delta));
        nang = NORM_ANGLE(nang);
    }
    else
    {
        nang = RANDOM_P2(2048);
    }

    if (SP_TAG6(sp))
        vel = SP_TAG6(sp);
    else
        vel = 800;

    // Spawn a shot
    w = SpawnSprite(STAT_MISSILE, LAVA_BOULDER, s_VulcanBoulder, sp->sectnum,
                    nx, ny, nz, nang, (vel/2 + vel/4) + RANDOM_RANGE(vel/4));

    wp = &sprite[w];
    wu = User[w];

    SetOwner(SpriteNum, w);
    wp->xrepeat = wp->yrepeat = 8 + RANDOM_RANGE(72);
    wp->shade = -40;
    wp->ang = nang;
    wu->Counter = 0;

    zsize = SPRITEp_SIZE_Z(wp);

    wu->Radius = 200;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    wp->clipdist = 256>>2;
    wu->ceiling_dist = zsize/2;
    wu->floor_dist = zsize/2;
    if (RANDOM_P2(1024) > 512)
        SET(wp->cstat, CSTAT_SPRITE_XFLIP);
    if (RANDOM_P2(1024) > 512)
        SET(wp->cstat, CSTAT_SPRITE_YFLIP);

    if (SP_TAG7(sp))
    {
        zvel = SP_TAG7(sp);
        zvel_rand = SP_TAG8(sp);
    }
    else
    {
        zvel = 50;
        zvel_rand = 40;
    }

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = -Z(zvel) + -Z(RANDOM_RANGE(zvel_rand));

    return w;
}

#if 0
int
InitSerpRing(short SpriteNum)
{
    SPRITEp sp = User[SpriteNum]->SpriteP, np;
    USERp u = User[SpriteNum], nu;
    short ang, ang_diff, ang_start, ang_finish, missiles, NewSprite;
    short max_missiles = 16;

    u->Counter = max_missiles;

    ang_diff = 2048 / max_missiles;

    ang_start = NORM_ANGLE(sp->ang - DIV2(2048));

    if (!SW_SHAREWARE)
        PlaySound(DIGI_RFWIZ, &sp->x, &sp->y, &sp->z, v3df_none);

    for (missiles = 0, ang = ang_start; missiles < max_missiles; ang += ang_diff, missiles++)
    {
        NewSprite = SpawnSprite(STAT_MISSILE_SKIP4, FIREBALL1, s_Ring2, sp->sectnum, sp->x, sp->y, SPRITEp_TOS(sp)+Z(20), ang, 0);

        np = &sprite[NewSprite];

        np->hitag = LUMINOUS; //Always full brightness
        np->xvel = 500;
        SetOwner(SpriteNum, NewSprite);
        np->shade = -40;
        np->xrepeat = 32;
        np->yrepeat = 32;
        np->zvel = 0;

        nu = User[NewSprite];

        nu->sz = Z(20);
        nu->Dist = 800;
        nu->Counter = max_missiles;
        nu->Counter2 = 0;
        nu->ceiling_dist = Z(10);
        nu->floor_dist = Z(10);
        nu->spal = np->pal = 27; // Bright Green

        // put it out there
        np->x += ((int) nu->Dist * (int) sintable[NORM_ANGLE(np->ang + 512)]) >> 14;
        np->y += ((int) nu->Dist * (int) sintable[np->ang]) >> 14;
        np->z = SPRITEp_TOS(sp) + Z(20);

        np->ang = NORM_ANGLE(np->ang + 512);

        nu->ox = np->x;
        nu->oy = np->y;
        nu->oz = np->z;

        if (SpriteInUnderwaterArea(sp))
            SET(nu->Flags, SPR_UNDERWATER);
    }
}
#else
int
InitSerpRing(short SpriteNum)
{
    SPRITEp sp = User[SpriteNum]->SpriteP, np;
    USERp u = User[SpriteNum], nu;
    short ang, ang_diff, ang_start, ang_finish, missiles, New;
    short max_missiles;

#define SERP_RING_DIST 2800 // Was 3500

    extern STATE s_SkullExplode[];
    extern STATE s_SkullRing[5][1];
    extern STATEp sg_SkullRing[];

    max_missiles = 12;

    u->Counter = max_missiles;

    ang_diff = 2048 / max_missiles;

    ang_start = NORM_ANGLE(sp->ang - DIV2(2048));

    PlaySound(DIGI_SERPSUMMONHEADS, &sp->x, &sp->y, &sp->z, v3df_none);

    for (missiles = 0, ang = ang_start; missiles < max_missiles; ang += ang_diff, missiles++)
    {
        New = SpawnSprite(STAT_SKIP4, SKULL_SERP, &s_SkullRing[0][0], sp->sectnum, sp->x, sp->y, sp->z, ang, 0);

        np = &sprite[New];
        nu = User[New];

        np->xvel = 500;
        //np->owner = SpriteNum;
        SetOwner(SpriteNum, New);
        np->shade = -20;
        np->xrepeat = 64;
        np->yrepeat = 64;
        np->yvel = 2*RINGMOVETICS;
        np->zvel = Z(3);
        np->pal = 0;

        np->z = SPRITEp_TOS(sp) - Z(20);
        nu->sz = Z(50);

        // ang around the serp is now slide_ang
        nu->slide_ang = np->ang;
        // randomize the head turning angle
        np->ang = RANDOM_P2(2048<<5)>>5;

        // control direction of spinning
        FLIP(u->Flags, SPR_BOUNCE);
        SET(nu->Flags, TEST(u->Flags, SPR_BOUNCE));

        nu->Dist = 600;
        nu->TargetDist = SERP_RING_DIST;
        nu->Counter2 = 0;

        nu->StateEnd = s_SkullExplode;
        nu->Rot = sg_SkullRing;

        // defaults do change the statnum
        EnemyDefaults(New, NULL, NULL);
        change_sprite_stat(New, STAT_SKIP4);
        RESET(np->extra, SPRX_PLAYER_OR_ENEMY);

        np->clipdist = (128+64) >> 2;
        SET(nu->Flags, SPR_XFLIP_TOGGLE);
        SET(np->cstat, CSTAT_SPRITE_YCENTER);

        nu->Radius = 400;
    }
    return 0;
}
#endif

#if 0
int
InitSerpRing2(short SpriteNum)
{
    SPRITEp sp = User[SpriteNum]->SpriteP, np;
    USERp u = User[SpriteNum], nu;
    short ang, ang_diff, ang_start, ang_finish, missiles, New;
    short max_missiles;
    short i;

    extern STATE s_SkullExplode[];
    extern STATE s_SkullRing[5][1];
    extern STATEp sg_SkullRing[];

#define NUM_SERP_RING 2

    static int zpos[NUM_SERP_RING] = {Z(25), Z(130)};

    PlaySound(DIGI_SERPSUMMONHEADS, &sp->x, &sp->y, &sp->z, v3df_none);

    for (i = 0; i < NUM_SERP_RING; i++)
    {
        max_missiles = 12;

        ang_diff = 2048 / max_missiles;

        ang_start = i * (2048/NUM_SERP_RING); //NORM_ANGLE(sp->ang - DIV2(2048));

        for (missiles = 0, ang = ang_start; missiles < max_missiles; ang += ang_diff, missiles++)
        {
            New = SpawnSprite(STAT_MISSILE_SKIP4, SKULL_SERP, &s_SkullRing[0][0], sp->sectnum, sp->x, sp->y, sp->z, ang, 0);

            np = &sprite[New];
            nu = User[New];

            //np->owner = SpriteNum;
            SetOwner(SpriteNum, New);
            np->shade = -20;
            np->xrepeat = 64;
            np->yrepeat = 64;
            np->zvel = Z(10);
            np->yvel = 4*RINGMOVETICS;
            np->pal = 0;

            np->z = SPRITEp_TOS(sp) - Z(20);
            nu->sz = zpos[i]; //Z(20) + (i * (SPRITEp_SIZE_Z(sp)/NUM_SERP_RING));

            // ang around the serp is now slide_ang
            nu->slide_ang = np->ang;
            // randomize the head turning angle
            np->ang = RANDOM_P2(2048<<5)>>5;

            // control direction of spinning
            FLIP(u->Flags, SPR_BOUNCE);
            SET(nu->Flags, TEST(u->Flags, SPR_BOUNCE));

            nu->Dist = 400;
            nu->TargetDist = 2500;
            nu->Counter2 = 0;

            nu->StateEnd = s_SkullExplode;
            nu->Rot = sg_SkullRing;

            // defaults do change the statnum
            EnemyDefaults(New, NULL, NULL);
            RESET(np->extra, SPRX_PLAYER_OR_ENEMY);
            change_sprite_stat(New, STAT_MISSILE_SKIP4);

            np->clipdist = (128+64) >> 2;
            SET(nu->Flags, SPR_XFLIP_TOGGLE);
            SET(np->cstat, CSTAT_SPRITE_YCENTER);

            nu->Radius = 400;
        }
    }
    return 0;
}
#endif

void
InitSpellNapalm(PLAYERp pp)
{
    short SpriteNum;
    SPRITEp sp;
    USERp u;
    unsigned i;
    short oclipdist;
    short ammo;
    USERp pu = User[pp->PlayerSprite];

    typedef struct
    {
        int dist_over, dist_out;
        short ang;
    } MISSILE_PLACEMENT;

    static MISSILE_PLACEMENT mp[] =
    {
        {600 * 6, 400, 512},
        {0, 1100, 0},
        {600 * 6, 400, -512},
    };

    ammo = NAPALM_MIN_AMMO;

    if (pp->WpnAmmo[WPN_HOTHEAD] < ammo)
        return;
    else
        PlayerUpdateAmmo(pp, WPN_HOTHEAD, -ammo);

    PlaySound(DIGI_NAPFIRE, &pp->posx, &pp->posy, &pp->posz, v3df_none);

    for (i = 0; i < SIZ(mp); i++)
    {
        SpriteNum = SpawnSprite(STAT_MISSILE, FIREBALL1, s_Napalm, pp->cursectnum,
                                pp->posx, pp->posy, pp->posz + Z(12), pp->pang, NAPALM_VELOCITY*2);

        sp = &sprite[SpriteNum];
        u = User[SpriteNum];

        sp->hitag = LUMINOUS; //Always full brightness

        if (i==0) // Only attach sound to first projectile
        {
            PlaySound(DIGI_NAPWIZ, &sp->x, &sp->y, &sp->z, v3df_follow);
            Set3DSoundOwner(SpriteNum);
        }

        //sp->owner = pp->SpriteP - sprite;
        SetOwner(pp->SpriteP - sprite, SpriteNum);
        sp->shade = -40;
        sp->xrepeat = 32;
        sp->yrepeat = 32;
        sp->clipdist = 0;
        sp->zvel = ((100 - pp->horiz) * HORIZ_MULT);
        SET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
        RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
        SET(u->Flags2, SPR2_BLUR_TAPER_FAST);

        u->floor_dist = Z(1);
        u->ceiling_dist = Z(1);
        u->Dist = 200;

        oclipdist = pp->SpriteP->clipdist;
        pp->SpriteP->clipdist = 1;

        if (mp[i].dist_over != 0)
        {
            sp->ang = NORM_ANGLE(sp->ang + mp[i].ang);
            HelpMissileLateral(SpriteNum, mp[i].dist_over);
            sp->ang = NORM_ANGLE(sp->ang - mp[i].ang);
        }

        u->xchange = MOVEx(sp->xvel, sp->ang);
        u->ychange = MOVEy(sp->xvel, sp->ang);
        u->zchange = sp->zvel;

        if (MissileSetPos(SpriteNum, DoNapalm, mp[i].dist_out))
        {
            pp->SpriteP->clipdist = oclipdist;
            KillSprite(SpriteNum);
            continue;
        }

        if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(sp))
            SET(u->Flags, SPR_UNDERWATER);

        pp->SpriteP->clipdist = oclipdist;

        u->Counter = 0;

    }
}

int
InitEnemyNapalm(short SpriteNum)
{
    short w;
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    short dist;
    unsigned i;
    short oclipdist;

    typedef struct
    {
        int dist_over, dist_out;
        short ang;
    } MISSILE_PLACEMENT;

    static MISSILE_PLACEMENT mp[] =
    {
        {600 * 6, 400, 512},
        {0, 1100, 0},
        {600 * 6, 400, -512},
    };

    PlaySound(DIGI_NAPFIRE, &sp->x, &sp->y, &sp->z, v3df_none);

    for (i = 0; i < SIZ(mp); i++)
    {
        w = SpawnSprite(STAT_MISSILE, FIREBALL1, s_Napalm, sp->sectnum,
                        sp->x, sp->y, SPRITEp_TOS(sp) + DIV4(SPRITEp_SIZE_Z(sp)), sp->ang, NAPALM_VELOCITY);

        wp = &sprite[w];
        wu = User[w];

        wp->hitag = LUMINOUS; //Always full brightness
        if (i==0) // Only attach sound to first projectile
        {
            PlaySound(DIGI_NAPWIZ, &wp->x, &wp->y, &wp->z, v3df_follow);
            Set3DSoundOwner(w);
        }

        if (u->ID == ZOMBIE_RUN_R0)
            SetOwner(sp->owner, w);
        else
            SetOwner(SpriteNum, w);

        wp->shade = -40;
        wp->xrepeat = 32;
        wp->yrepeat = 32;
        wp->clipdist = 0;
        SET(wp->cstat, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
        RESET(wp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
        SET(wu->Flags2, SPR2_BLUR_TAPER_FAST);

        wu->floor_dist = Z(1);
        wu->ceiling_dist = Z(1);
        wu->Dist = 200;

        oclipdist = sp->clipdist;
        sp->clipdist = 1;

        if (mp[i].dist_over != 0)
        {
            wp->ang = NORM_ANGLE(wp->ang + mp[i].ang);
            HelpMissileLateral(w, mp[i].dist_over);
            wp->ang = NORM_ANGLE(wp->ang - mp[i].ang);
        }

        // find the distance to the target (player)
        dist = Distance(wp->x, wp->y, u->tgt_sp->x, u->tgt_sp->y);

        if (dist != 0)
            wp->zvel = (wp->xvel * (SPRITEp_UPPER(u->tgt_sp) - wp->z)) / dist;

        wu->xchange = MOVEx(wp->xvel, wp->ang);
        wu->ychange = MOVEy(wp->xvel, wp->ang);
        wu->zchange = wp->zvel;

        MissileSetPos(w, DoNapalm, mp[i].dist_out);

        sp->clipdist = oclipdist;

        u->Counter = 0;

    }
    return 0;
}

int
InitSpellMirv(PLAYERp pp)
{
    short SpriteNum;
    SPRITEp sp;
    USERp u;
    short i;
    short oclipdist;

    PlaySound(DIGI_MIRVFIRE, &pp->posx, &pp->posy, &pp->posz, v3df_none);

    SpriteNum = SpawnSprite(STAT_MISSILE, FIREBALL1, s_Mirv, pp->cursectnum,
                            pp->posx, pp->posy, pp->posz + Z(12), pp->pang, MIRV_VELOCITY);

    sp = &sprite[SpriteNum];
    u = User[SpriteNum];

    PlaySound(DIGI_MIRVWIZ, &sp->x, &sp->y, &sp->z, v3df_follow);
    Set3DSoundOwner(SpriteNum);

    //sp->owner = pp->SpriteP - sprite;
    SetOwner(pp->SpriteP - sprite, SpriteNum);
    sp->shade = -40;
    sp->xrepeat = 72;
    sp->yrepeat = 72;
    sp->clipdist = 32L >> 2;
    sp->zvel = ((100 - pp->horiz) * HORIZ_MULT);
    SET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    u->floor_dist = Z(16);
    u->ceiling_dist = Z(16);
    u->Dist = 200;

    oclipdist = pp->SpriteP->clipdist;
    pp->SpriteP->clipdist = 0;

    u->xchange = MOVEx(sp->xvel, sp->ang);
    u->ychange = MOVEy(sp->xvel, sp->ang);
    u->zchange = sp->zvel;

    MissileSetPos(SpriteNum, DoMirv, 600);
    pp->SpriteP->clipdist = oclipdist;

    u->Counter = 0;
    return 0;
}

int
InitEnemyMirv(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    short w;
    short i;
    short oclipdist;
    int dist;

    PlaySound(DIGI_MIRVFIRE, &sp->x, &sp->y, &sp->z, v3df_none);

    w = SpawnSprite(STAT_MISSILE, MIRV_METEOR, s_Mirv, sp->sectnum,
                    sp->x, sp->y, SPRITEp_TOS(sp) + DIV4(SPRITEp_SIZE_Z(sp)), sp->ang, MIRV_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    PlaySound(DIGI_MIRVWIZ, &wp->x, &wp->y, &wp->z, v3df_follow);
    Set3DSoundOwner(w);

    SetOwner(SpriteNum, w);
    wp->shade = -40;
    wp->xrepeat = 72;
    wp->yrepeat = 72;
    wp->clipdist = 32L >> 2;

    SET(wp->cstat, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
    RESET(wp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    wu->floor_dist = Z(16);
    wu->ceiling_dist = Z(16);
    wu->Dist = 200;

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    MissileSetPos(w, DoMirv, 600);

    // find the distance to the target (player)
    dist = Distance(wp->x, wp->y, u->tgt_sp->x, u->tgt_sp->y);

    if (dist != 0)
        wu->zchange = wp->zvel = (wp->xvel * (SPRITEp_UPPER(u->tgt_sp) - wp->z)) / dist;
    return 0;
}


int
InitSwordAttack(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite],tu;
    SPRITEp sp = NULL;
    short i, nexti;
    unsigned stat;
    int dist;
    short reach,face;

    PlaySound(DIGI_SWORDSWOOSH, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_doppler);

    if (TEST(pp->Flags, PF_DIVING))
    {
        short bubble;
        USERp bu;
        SPRITEp bp;
        int nx,ny;
        short random_amt;

        static int16_t dangs[] =
        {
            -256, -128, 0, 128, 256
        };

        for (i = 0; i < (int)SIZ(dangs); i++)
        {
            if (RANDOM_RANGE(1000) < 500) continue; // Don't spawn bubbles every time
            bubble = SpawnBubble(pp->SpriteP - sprite);
            if (bubble >= 0)
            {
                bu = User[bubble];
                bp = &sprite[bubble];

                bp->ang = pp->pang;

                random_amt = (RANDOM_P2(32<<8)>>8) - 16;

                // back it up a bit to get it out of your face
                nx = MOVEx((1024+256)*3, NORM_ANGLE(bp->ang + dangs[i] + random_amt));
                ny = MOVEy((1024+256)*3, NORM_ANGLE(bp->ang + dangs[i] + random_amt));

                move_missile(bubble, nx, ny, 0L, u->ceiling_dist, u->floor_dist, CLIPMASK_PLAYER, 1);
            }
        }
    }

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            sp = &sprite[i];

            //if (pp->SpriteP == sp) // UnderSprite was getting hit
            //    break;
            if (User[i]->PlayerP == pp)
                break;

            if (!TEST(sp->extra, SPRX_PLAYER_OR_ENEMY))
                continue;

            dist = Distance(pp->posx, pp->posy, sp->x, sp->y);

            reach = 1000; // !JIM! was 800
            face = 200;

            if (dist < CLOSE_RANGE_DIST_FUDGE(sp, pp->SpriteP, reach) && PLAYER_FACING_RANGE(pp, sp, face))
            {
                if (SpriteOverlapZ(pp->PlayerSprite, i, Z(20)))
                {
                    if (FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,pp->SpriteP->x,pp->SpriteP->y,SPRITEp_MID(pp->SpriteP),pp->SpriteP->sectnum))
                        DoDamage(i, pp->PlayerSprite);
                }
            }
        }
    }

    // all this is to break glass
    {
        hitdata_t hitinfo;
        short daang;
        int daz;

        daang = pp->pang;
        daz = ((100 - pp->horiz) * 2000) + (RANDOM_RANGE(24000) - 12000);

        FAFhitscan(pp->posx, pp->posy, pp->posz, pp->cursectnum,       // Start position
                   sintable[NORM_ANGLE(daang + 512)],      // X vector of 3D ang
                   sintable[NORM_ANGLE(daang)],    // Y vector of 3D ang
                   daz,                            // Z vector of 3D ang
                   &hitinfo, CLIPMASK_MISSILE);

        if (hitinfo.sect < 0)
            return 0;

        if (FindDistance3D(pp->posx - hitinfo.pos.x, pp->posy - hitinfo.pos.y, (pp->posz - hitinfo.pos.z)>>4) < 700)
        {

            if (hitinfo.sprite >= 0)
            {
                extern STATE s_TrashCanPain[];
                SPRITEp hsp = &sprite[hitinfo.sprite];
                tu = User[hitinfo.sprite];

                if (tu) // JBF: added null check
                    switch (tu->ID)
                    {
                    case ZILLA_RUN_R0:
                        SpawnSwordSparks(pp, hitinfo.sect, -1, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang);
                        PlaySound(DIGI_SWORDCLANK, &hitinfo.pos.x, &hitinfo.pos.y, &hitinfo.pos.z, v3df_none);
                        break;
                    case TRASHCAN:
                        if (tu->WaitTics <= 0)
                        {
                            tu->WaitTics = SEC(2);
                            ChangeState(hitinfo.sprite,s_TrashCanPain);
                        }
                        SpawnSwordSparks(pp, hitinfo.sect, -1, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang);
                        PlaySound(DIGI_SWORDCLANK, &hitinfo.pos.x, &hitinfo.pos.y, &hitinfo.pos.z, v3df_none);
                        PlaySound(DIGI_TRASHLID, &sp->x, &sp->y, &sp->z, v3df_none);
                        break;
                    case PACHINKO1:
                    case PACHINKO2:
                    case PACHINKO3:
                    case PACHINKO4:
                    case 623:
                        SpawnSwordSparks(pp, hitinfo.sect, -1, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang);
                        PlaySound(DIGI_SWORDCLANK, &hitinfo.pos.x, &hitinfo.pos.y, &hitinfo.pos.z, v3df_none);
                        break;
                    }

                if (sprite[hitinfo.sprite].lotag == TAG_SPRITE_HIT_MATCH)
                {
                    if (MissileHitMatch(-1, WPN_STAR, hitinfo.sprite))
                        return 0;
                }

                if (TEST(hsp->extra, SPRX_BREAKABLE))
                {
                    HitBreakSprite(hitinfo.sprite,0);
                }

                // hit a switch?
                if (TEST(hsp->cstat, CSTAT_SPRITE_WALL) && (hsp->lotag || hsp->hitag))
                {
                    ShootableSwitch(hitinfo.sprite,-1);
                }

            }

            if (hitinfo.wall >= 0)
            {
                if (wall[hitinfo.wall].nextsector >= 0)
                {
                    if (TEST(sector[wall[hitinfo.wall].nextsector].ceilingstat, CEILING_STAT_PLAX))
                    {
                        if (hitinfo.pos.z < sector[wall[hitinfo.wall].nextsector].ceilingz)
                        {
                            return 0;
                        }
                    }
                }

                if (wall[hitinfo.wall].lotag == TAG_WALL_BREAK)
                {
                    HitBreakWall(&wall[hitinfo.wall], hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang, u->ID);
                }
                // hit non breakable wall - do sound and puff
                else
                {
                    SpawnSwordSparks(pp, hitinfo.sect, hitinfo.wall, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang);
                    PlaySound(DIGI_SWORDCLANK, &hitinfo.pos.x, &hitinfo.pos.y, &hitinfo.pos.z, v3df_none);
                }
            }
        }
    }

    return 0;
}

int
InitFistAttack(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite],tu;
    SPRITEp sp = NULL;
    short i, nexti;
    unsigned stat;
    int dist;
    short reach,face;

    PlaySound(DIGI_STAR, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_doppler);

    if (TEST(pp->Flags, PF_DIVING))
    {
        short bubble;
        USERp bu;
        SPRITEp bp;
        int nx,ny;
        short random_amt;

        static int16_t dangs[] =
        {
            -128,128
        };

        for (i = 0; i < (int)SIZ(dangs); i++)
        {
            bubble = SpawnBubble(pp->SpriteP - sprite);
            if (bubble >= 0)
            {
                bu = User[bubble];
                bp = &sprite[bubble];

                bp->ang = pp->pang;

                random_amt = (RANDOM_P2(32<<8)>>8) - 16;

                // back it up a bit to get it out of your face
                nx = MOVEx((1024+256)*3, NORM_ANGLE(bp->ang + dangs[i] + random_amt));
                ny = MOVEy((1024+256)*3, NORM_ANGLE(bp->ang + dangs[i] + random_amt));

                move_missile(bubble, nx, ny, 0L, u->ceiling_dist, u->floor_dist, CLIPMASK_PLAYER, 1);
            }
        }
    }

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            sp = &sprite[i];

            //if (pp->SpriteP == sp) // UnderSprite was getting hit
            //    break;
            if (User[i]->PlayerP == pp)
                break;

            if (!TEST(sp->extra, SPRX_PLAYER_OR_ENEMY))
                continue;

            dist = Distance(pp->posx, pp->posy, sp->x, sp->y);

            if (pp->InventoryActive[2]) // Shadow Bombs give you demon fist
            {
                face = 190;
                reach = 2300;
            }
            else
            {
                reach = 1000;
                face = 200;
            }

            if (dist < CLOSE_RANGE_DIST_FUDGE(sp, pp->SpriteP, reach) && PLAYER_FACING_RANGE(pp, sp, face))
            {
                if (SpriteOverlapZ(pp->PlayerSprite, i, Z(20)) || face == 190)
                {
                    if (FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,pp->SpriteP->x,pp->SpriteP->y,SPRITEp_MID(pp->SpriteP),pp->SpriteP->sectnum))
                        DoDamage(i, pp->PlayerSprite);
                    if (face == 190)
                    {
                        SpawnDemonFist(i);
                    }
                }
            }
        }
    }


    // all this is to break glass
    {
        hitdata_t hitinfo;
        short daang;
        int daz;

        daang = pp->pang;
        daz = ((100 - pp->horiz) * 2000) + (RANDOM_RANGE(24000) - 12000);

        FAFhitscan(pp->posx, pp->posy, pp->posz, pp->cursectnum,       // Start position
                   sintable[NORM_ANGLE(daang + 512)],      // X vector of 3D ang
                   sintable[NORM_ANGLE(daang)],    // Y vector of 3D ang
                   daz,                            // Z vector of 3D ang
                   &hitinfo, CLIPMASK_MISSILE);

        if (hitinfo.sect < 0)
            return 0;

        if (FindDistance3D(pp->posx - hitinfo.pos.x, pp->posy - hitinfo.pos.y, (pp->posz - hitinfo.pos.z)>>4) < 700)
        {

            if (hitinfo.sprite >= 0)
            {
                extern STATE s_TrashCanPain[];
                SPRITEp hsp = &sprite[hitinfo.sprite];
                tu = User[hitinfo.sprite];

                if (tu)     // JBF: added null check
                    switch (tu->ID)
                    {
                    case ZILLA_RUN_R0:
                        SpawnSwordSparks(pp, hitinfo.sect, -1, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang);
                        PlaySound(DIGI_ARMORHIT, &hitinfo.pos.x, &hitinfo.pos.y, &hitinfo.pos.z, v3df_none);
                        break;
                    case TRASHCAN:
                        if (tu->WaitTics <= 0)
                        {
                            tu->WaitTics = SEC(2);
                            ChangeState(hitinfo.sprite,s_TrashCanPain);
                        }
                        SpawnSwordSparks(pp, hitinfo.sect, -1, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang);
                        PlaySound(DIGI_ARMORHIT, &hitinfo.pos.x, &hitinfo.pos.y, &hitinfo.pos.z, v3df_none);
                        PlaySound(DIGI_TRASHLID, &sp->x, &sp->y, &sp->z, v3df_none);
                        break;
                    case PACHINKO1:
                    case PACHINKO2:
                    case PACHINKO3:
                    case PACHINKO4:
                    case 623:
                        SpawnSwordSparks(pp, hitinfo.sect, -1, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang);
                        PlaySound(DIGI_ARMORHIT, &hitinfo.pos.x, &hitinfo.pos.y, &hitinfo.pos.z, v3df_none);
                        break;
                    }

                if (sprite[hitinfo.sprite].lotag == TAG_SPRITE_HIT_MATCH)
                {
                    if (MissileHitMatch(-1, WPN_STAR, hitinfo.sprite))
                        return 0;
                }

                if (TEST(hsp->extra, SPRX_BREAKABLE))
                {
                    HitBreakSprite(hitinfo.sprite,0);
                }

                // hit a switch?
                if (TEST(hsp->cstat, CSTAT_SPRITE_WALL) && (hsp->lotag || hsp->hitag))
                {
                    ShootableSwitch(hitinfo.sprite,-1);
                }

                switch (hsp->picnum)
                {
                case 5062:
                case 5063:
                case 4947:
                    SpawnSwordSparks(pp, hitinfo.sect, -1, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang);
                    PlaySound(DIGI_ARMORHIT, &hitinfo.pos.x, &hitinfo.pos.y, &hitinfo.pos.z, v3df_none);
                    if (RANDOM_RANGE(1000) > 700)
                        PlayerUpdateHealth(pp,1); // Give some health
                    SET(hsp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
                    break;
                }
            }


            if (hitinfo.wall >= 0)
            {
                if (wall[hitinfo.wall].nextsector >= 0)
                {
                    if (TEST(sector[wall[hitinfo.wall].nextsector].ceilingstat, CEILING_STAT_PLAX))
                    {
                        if (hitinfo.pos.z < sector[wall[hitinfo.wall].nextsector].ceilingz)
                        {
                            return 0;
                        }
                    }
                }

                if (wall[hitinfo.wall].lotag == TAG_WALL_BREAK)
                {
                    HitBreakWall(&wall[hitinfo.wall], hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang, u->ID);
                }
                // hit non breakable wall - do sound and puff
                else
                {
                    SpawnSwordSparks(pp, hitinfo.sect, hitinfo.wall, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang);
                    PlaySound(DIGI_ARMORHIT, &hitinfo.pos.x, &hitinfo.pos.y, &hitinfo.pos.z, v3df_none);
                    if (PlayerTakeDamage(pp, -1))
                    {
                        PlayerUpdateHealth(pp, -(RANDOM_RANGE(2<<8)>>8));
                        PlayerCheckDeath(pp, -1);
                    }
                }
            }
        }

        return 0;
    }
}

int
InitSumoNapalm(short SpriteNum)
{
    short w;
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    short dist;
    short i,j,ang;
    short oclipdist;

    typedef struct
    {
        int dist_over, dist_out;
        short ang;
    } MISSILE_PLACEMENT;

    static MISSILE_PLACEMENT mp[] =
    {
        {0, 1100, 0},
    };

    PlaySound(DIGI_NAPFIRE, &sp->x, &sp->y, &sp->z, v3df_none);

    ang = sp->ang;
    for (j=0; j<4; j++)
    {
        for (i = 0; i < (int)SIZ(mp); i++)
        {
            w = SpawnSprite(STAT_MISSILE, FIREBALL1, s_Napalm, sp->sectnum,
                            sp->x, sp->y, SPRITEp_TOS(sp), ang, NAPALM_VELOCITY);

            wp = &sprite[w];
            wu = User[w];

            wp->hitag = LUMINOUS; //Always full brightness
            if (i==0) // Only attach sound to first projectile
            {
                PlaySound(DIGI_NAPWIZ, &wp->x, &wp->y, &wp->z, v3df_follow);
                Set3DSoundOwner(w);
            }

            SetOwner(SpriteNum, w);
            wp->shade = -40;
            wp->xrepeat = 32;
            wp->yrepeat = 32;
            wp->clipdist = 0;
            SET(wp->cstat, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
            RESET(wp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
            SET(wu->Flags2, SPR2_BLUR_TAPER_FAST);

            wu->floor_dist = Z(1);
            wu->ceiling_dist = Z(1);
            wu->Dist = 200;

            oclipdist = sp->clipdist;
            sp->clipdist = 1;

            if (mp[i].dist_over != 0)
            {
                wp->ang = NORM_ANGLE(wp->ang + mp[i].ang);
                HelpMissileLateral(w, mp[i].dist_over);
                wp->ang = NORM_ANGLE(wp->ang - mp[i].ang);
            }

            // find the distance to the target (player)
            dist = Distance(wp->x, wp->y, u->tgt_sp->x, u->tgt_sp->y);

            if (dist != 0)
                wp->zvel = (wp->xvel * (SPRITEp_UPPER(u->tgt_sp) - wp->z)) / dist;

            wu->xchange = MOVEx(wp->xvel, wp->ang);
            wu->ychange = MOVEy(wp->xvel, wp->ang);
            wu->zchange = wp->zvel;

            MissileSetPos(w, DoNapalm, mp[i].dist_out);

            sp->clipdist = oclipdist;

            u->Counter = 0;

        }
        ang += 512;
    }
    return 0;
}

int
InitSumoSkull(short SpriteNum)
{
    SPRITEp sp = User[SpriteNum]->SpriteP, np;
    USERp u = User[SpriteNum], nu;
    short ang, ang_diff, ang_start, ang_finish, missiles, New;
    short max_missiles;

    extern STATE s_SkullExplode[];
    extern STATE s_SkullWait[5][1];
    extern STATEp sg_SkullWait[];
    extern ATTRIBUTE SkullAttrib;


    PlaySound(DIGI_SERPSUMMONHEADS, &sp->x, &sp->y, &sp->z, v3df_none);

    New = SpawnSprite(STAT_ENEMY, SKULL_R0, &s_SkullWait[0][0], sp->sectnum, sp->x, sp->y, SPRITEp_MID(sp), sp->ang, 0);

    np = &sprite[New];
    nu = User[New];

    np->xvel = 500;
    SetOwner(SpriteNum, New);
    np->shade = -20;
    np->xrepeat = 64;
    np->yrepeat = 64;
    np->pal = 0;

    // randomize the head turning angle
    np->ang = RANDOM_P2(2048<<5)>>5;

    // control direction of spinning
    FLIP(u->Flags, SPR_BOUNCE);
    SET(nu->Flags, TEST(u->Flags, SPR_BOUNCE));

    nu->StateEnd = s_SkullExplode;
    nu->Rot = sg_SkullWait;

    nu->Attrib = &SkullAttrib;
    DoActorSetSpeed(SpriteNum, NORM_SPEED);
    nu->Counter = RANDOM_P2(2048);
    nu->sz = np->z;
    nu->Health = 100;

    // defaults do change the statnum
    EnemyDefaults(New, NULL, NULL);
    //change_sprite_stat(New, STAT_SKIP4);
    SET(np->extra, SPRX_PLAYER_OR_ENEMY);

    np->clipdist = (128+64) >> 2;
    SET(nu->Flags, SPR_XFLIP_TOGGLE);
    SET(np->cstat, CSTAT_SPRITE_YCENTER);

    nu->Radius = 400;
    return 0;
}

int
InitSumoStompAttack(short SpriteNum)
{
    USERp u = User[SpriteNum],tu;
    SPRITEp sp = &sprite[SpriteNum],tsp;
    short i, nexti;
    unsigned stat;
    int dist;
    short reach,face;


    PlaySound(DIGI_30MMEXPLODE, &sp->x, &sp->y, &sp->z, v3df_dontpan|v3df_doppler);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            tsp = &sprite[i];

            if (tsp != u->tgt_sp)
                break;

            if (!TEST(tsp->extra, SPRX_PLAYER_OR_ENEMY))
                continue;

            dist = Distance(sp->x, sp->y, tsp->x, tsp->y);

            reach = 16384;

            if (dist < CLOSE_RANGE_DIST_FUDGE(tsp, sp, reach))
            {
                if (FAFcansee(tsp->x,tsp->y,SPRITEp_MID(tsp),tsp->sectnum,sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum))
                    DoDamage(i, SpriteNum);
            }
        }
    }


    return 0;
}

int
InitMiniSumoClap(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    int nx, ny, nz, dist;
    short w;
    short reach;


    if (!u->tgt_sp) return 0;

    dist = Distance(sp->x, sp->y, u->tgt_sp->x, u->tgt_sp->y);

    reach = 10000;

    if (dist < CLOSE_RANGE_DIST_FUDGE(u->tgt_sp, sp, 1000))
    {
        if (SpriteOverlapZ(SpriteNum, u->tgt_sp - sprite, Z(20)))
        {
            if (FAFcansee(u->tgt_sp->x,u->tgt_sp->y,SPRITEp_MID(u->tgt_sp),u->tgt_sp->sectnum,sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum))
            {
                PlaySound(DIGI_CGTHIGHBONE,&sp->x,&sp->y,&sp->z,v3df_follow|v3df_dontpan);
                DoDamage(u->tgt_sp - sprite, SpriteNum);
            }
        }
    }
    else if (dist < CLOSE_RANGE_DIST_FUDGE(u->tgt_sp, sp, reach))
    {
        if (FAFcansee(u->tgt_sp->x,u->tgt_sp->y,SPRITEp_MID(u->tgt_sp),u->tgt_sp->sectnum,sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum))
        {
            PlaySound(DIGI_30MMEXPLODE, &sp->x, &sp->y, &sp->z, v3df_none);
            SpawnFireballFlames(SpriteNum, u->tgt_sp - sprite);
        }
    }

    return 0;
}

int
WeaponAutoAim(SPRITEp sp, short Missile, short ang, SWBOOL test)
{
    USERp wu = User[Missile];
    USERp u = User[sp - sprite];
    SPRITEp wp = &sprite[Missile];
    short hit_sprite = -1;
    int dist;
    int zh;

    //return(-1);

#if 0
    //formula for leading a player
    dist = Distance(sp->x, sp->y, hp->x, hp->y);
    time_to_target = dist/wp->xvel;
    lead_dist = time_to_target*hp->vel;
#endif

    if (u && u->PlayerP)
    {
        if (!TEST(u->PlayerP->Flags, PF_AUTO_AIM))
        {
            return -1;
        }
    }

    if ((hit_sprite = DoPickTarget(sp, ang, test)) != -1)
    {
        SPRITEp hp = &sprite[hit_sprite];
        USERp hu = User[hit_sprite];

        wu->WpnGoal = hit_sprite;
        SET(hu->Flags, SPR_TARGETED);
        SET(hu->Flags, SPR_ATTACKED);

        wp->ang = NORM_ANGLE(getangle(hp->x - wp->x, hp->y - wp->y));
        //dist = FindDistance2D(wp->x, wp->y, hp->x, hp->y);
        dist = FindDistance2D(wp->x - hp->x, wp->y - hp->y);

        if (dist != 0)
        {
#if 1
            int tos, diff, siz;

            tos = SPRITEp_TOS(hp);
            diff = wp->z - tos;
            siz = SPRITEp_SIZE_Z(hp);

            // hit_sprite is below
            if (diff < -Z(50))
                zh = tos + DIV2(siz);
            else
            // hit_sprite is above
            if (diff > Z(50))
                zh = tos + DIV8(siz);
            else
                zh = tos + DIV4(siz);

            wp->zvel = (wp->xvel * (zh - wp->z)) / dist;
#else
            zh = SPRITEp_TOS(hp) + DIV4(SPRITEp_SIZE_Z(hp));
            wp->zvel = (wp->xvel * (zh - wp->z)) / dist;
#endif
        }
    }

    return hit_sprite;
}

int
WeaponAutoAimZvel(SPRITEp sp, short Missile, int *zvel, short ang, SWBOOL test)
{
    USERp wu = User[Missile];
    USERp u = User[sp - sprite];
    SPRITEp wp = &sprite[Missile];
    short hit_sprite = -1;
    int dist;
    int zh;

    //return(-1);

#if 0
    //formula for leading a player
    dist = Distance(sp->x, sp->y, hp->x, hp->y);
    time_to_target = dist/wp->xvel;
    lead_dist = time_to_target*hp->vel;
#endif

    if (u && u->PlayerP)
    {
        if (!TEST(u->PlayerP->Flags, PF_AUTO_AIM))
        {
            return -1;
        }
    }

    if ((hit_sprite = DoPickTarget(sp, ang, test)) != -1)
    {
        SPRITEp hp = &sprite[hit_sprite];
        USERp hu = User[hit_sprite];

        wu->WpnGoal = hit_sprite;
        SET(hu->Flags, SPR_TARGETED);
        SET(hu->Flags, SPR_ATTACKED);

        wp->ang = NORM_ANGLE(getangle(hp->x - wp->x, hp->y - wp->y));
        //dist = FindDistance2D(wp->x, wp->y, hp->x, hp->y);
        dist = FindDistance2D(wp->x - hp->x, wp->y - hp->y);

        if (dist != 0)
        {
#if 1
            int tos, diff, siz;

            tos = SPRITEp_TOS(hp);
            diff = wp->z - tos;
            siz = SPRITEp_SIZE_Z(hp);

            // hit_sprite is below
            if (diff < -Z(50))
                zh = tos + DIV2(siz);
            else
            // hit_sprite is above
            if (diff > Z(50))
                zh = tos + DIV8(siz);
            else
                zh = tos + DIV4(siz);

            *zvel = (wp->xvel * (zh - wp->z)) / dist;
#else
            zh = SPRITEp_TOS(hp) + DIV4(SPRITEp_SIZE_Z(hp));
            wp->zvel = (wp->xvel * (zh - wp->z)) / dist;
#endif
        }
    }

    return hit_sprite;
}


int
AimHitscanToTarget(SPRITEp sp, int *z, short *ang, int z_ratio)
{
    USERp u = User[sp - sprite];
    short hit_sprite = -1;
    int dist;
    int zh;
    int xvect;
    int yvect;
    SPRITEp hp;
    USERp hu;

    if (!u->tgt_sp)
        return -1;

    hit_sprite = u->tgt_sp - sprite;
    hp = &sprite[hit_sprite];
    hu = User[hit_sprite];

    SET(hu->Flags, SPR_TARGETED);
    SET(hu->Flags, SPR_ATTACKED);

    // Global set by DoPickTarget
    *ang = getangle(hp->x - sp->x, hp->y - sp->y);

    // find the distance to the target
    dist = ksqrt(SQ(sp->x - hp->x) + SQ(sp->y - hp->y));

    if (dist != 0)
    {
        zh = SPRITEp_UPPER(hp);

        xvect = sintable[NORM_ANGLE(*ang + 512)];
        yvect = sintable[NORM_ANGLE(*ang)];

        if (hp->x - sp->x != 0)
            //*z = xvect * ((zh - *z)/(hp->x - sp->x));
            *z = scale(xvect,zh - *z,hp->x - sp->x);
        else if (hp->y - sp->y != 0)
            //*z = yvect * ((zh - *z)/(hp->y - sp->y));
            *z = scale(yvect,zh - *z,hp->y - sp->y);
        else
            *z = 0;

        // so actors won't shoot straight up at you
        // need to be a bit of a distance away
        // before they have a valid shot
        if (labs(*z / dist) > z_ratio)
        {
            return -1;
        }
    }


    return hit_sprite;
}

int
WeaponAutoAimHitscan(SPRITEp sp, int *z, short *ang, SWBOOL test)
{
    USERp u = User[sp - sprite];
    short hit_sprite = -1;
    int dist;
    int zh;
    int xvect;
    int yvect;

    if (u && u->PlayerP)
    {
        if (!TEST(u->PlayerP->Flags, PF_AUTO_AIM))
        {
            return -1;
        }
    }

    if ((hit_sprite = DoPickTarget(sp, *ang, test)) != -1)
    {
        SPRITEp hp = &sprite[hit_sprite];
        USERp hu = User[hit_sprite];

        SET(hu->Flags, SPR_TARGETED);
        SET(hu->Flags, SPR_ATTACKED);

        // Global set by DoPickTarget
        //*ang = target_ang;
        *ang = NORM_ANGLE(getangle(hp->x - sp->x, hp->y - sp->y));

        // find the distance to the target
        dist = ksqrt(SQ(sp->x - hp->x) + SQ(sp->y - hp->y));

        if (dist != 0)
        {
            zh = SPRITEp_TOS(hp) + DIV4(SPRITEp_SIZE_Z(hp));

            xvect = sintable[NORM_ANGLE(*ang + 512)];
            yvect = sintable[NORM_ANGLE(*ang)];

            if (hp->x - sp->x != 0)
                //*z = xvect * ((zh - *z)/(hp->x - sp->x));
                *z = scale(xvect,zh - *z,hp->x - sp->x);
            else if (hp->y - sp->y != 0)
                //*z = yvect * ((zh - *z)/(hp->y - sp->y));
                *z = scale(yvect,zh - *z,hp->y - sp->y);
            else
                *z = 0;
        }
    }

    return hit_sprite;
}

void
WeaponHitscanShootFeet(SPRITEp sp, SPRITEp hp, int *zvect)
{
    int dist;
    int zh;
    int xvect;
    int yvect;
    int z;
    short ang;

    // Global set by DoPickTarget
    //*ang = target_ang;
    ang = NORM_ANGLE(getangle(hp->x - sp->x, hp->y - sp->y));

    // find the distance to the target
    dist = ksqrt(SQ(sp->x - hp->x) + SQ(sp->y - hp->y));

    if (dist != 0)
    {
        zh = SPRITEp_BOS(hp) + Z(20);
        z = sp->z;

        xvect = sintable[NORM_ANGLE(ang + 512)];
        yvect = sintable[NORM_ANGLE(ang)];

        if (hp->x - sp->x != 0)
            //*z = xvect * ((zh - *z)/(hp->x - sp->x));
            *zvect = scale(xvect,zh - z, hp->x - sp->x);
        else if (hp->y - sp->y != 0)
            //*z = yvect * ((zh - *z)/(hp->y - sp->y));
            *zvect = scale(yvect,zh - z, hp->y - sp->y);
        else
            *zvect = 0;
    }
}

int
InitStar(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz, oldx, oldy, oldspeed;
    short Weapon, w;
    int oclipdist;
    int zvel;

    static short dang[] = {-12, 12};
//    static short dang[] = {-0, 0};
    unsigned char i;
    SPRITEp np;
    USERp nu;
    short nw;
    SWBOOL AutoAim = FALSE;
#define STAR_REPEAT  26
#define STAR_HORIZ_ADJ 100L
    //#define STAR_HORIZ_ADJ 0

    PlayerUpdateAmmo(pp, u->WeaponNum, -3);

    nx = pp->posx;
    ny = pp->posy;

    nz = pp->posz + pp->bob_z + Z(8);

    PlaySound(DIGI_STAR, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_doppler);

    // Spawn a shot
    // Inserting and setting up variables

    w = SpawnSprite(STAT_MISSILE, STAR1, s_Star, pp->cursectnum, nx, ny, nz, pp->pang, STAR_VELOCITY);
    wp = &sprite[w];
    wu = User[w];

    // Attach sound to moving shuriken  (* Screw off, yucky sound! *)
//    PlaySound(DIGI_STARWIZ, &wp->x, &wp->y, &wp->z, v3df_follow);
//    Set3DSoundOwner(w);

    //SET(wp->cstat, CSTAT_SPRITE_WALL);
    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = wp->xrepeat = STAR_REPEAT;
    wp->shade = -25;
    wp->clipdist = 32L >> 2;
    // wp->zvel was overflowing with this calculation - had to move to a local
    // long var
    zvel = ((100 - pp->horiz) * (HORIZ_MULT+STAR_HORIZ_ADJ));

    wu->ceiling_dist = Z(1);
    wu->floor_dist = Z(1);
    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 100;
    wu->Counter = 0;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    //SET(wu->Flags2, SPR2_BLUR_TAPER_SLOW);

    // zvel had to be tweaked alot for this weapon
    // MissileSetPos seemed to be pushing the sprite too far up or down when
    // the horizon was tilted.  Never figured out why.
    wp->zvel = zvel >> 1;
    if (MissileSetPos(w, DoStar, 1000))
    {
        KillSprite(w);
        return 0;
    }

    if (WeaponAutoAim(pp->SpriteP, w, 32, FALSE) != -1)
    {
        AutoAim = TRUE;
        zvel = wp->zvel;
    }

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = zvel;

    if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    wu->ox = wp->x;
    wu->oy = wp->y;
    wu->oz = wp->z;

    for (i = 0; i < (int)SIZ(dang); i++)
    {
        nw = SpawnSprite(STAT_MISSILE, STAR1, s_Star, pp->cursectnum, nx, ny, nz, NORM_ANGLE(wp->ang + dang[i]), wp->xvel);
        np = &sprite[nw];
        nu = User[nw];

        SetOwner(wp->owner, nw);
        np->yrepeat = np->xrepeat = STAR_REPEAT;
        np->shade = wp->shade;

        np->extra = wp->extra;
        np->clipdist = wp->clipdist;
        nu->WeaponNum = wu->WeaponNum;
        nu->Radius = wu->Radius;
        nu->ceiling_dist = wu->ceiling_dist;
        nu->floor_dist = wu->floor_dist;
        nu->Flags2 = wu->Flags2;

        if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(np))
            SET(nu->Flags, SPR_UNDERWATER);

        zvel = ((100 - pp->horiz) * (HORIZ_MULT+STAR_HORIZ_ADJ));
        np->zvel = zvel >> 1;

        if (MissileSetPos(nw, DoStar, 1000))
        {
            KillSprite(nw);
            return 0;
        }

        // move the same as middle star
        zvel = wu->zchange;

        nu->xchange = MOVEx(np->xvel, np->ang);
        nu->ychange = MOVEy(np->xvel, np->ang);
        nu->zchange = zvel;

        nu->ox = np->x;
        nu->oy = np->y;
        nu->oz = np->z;
    }

    return 0;
}

#if WORM == 1
void
InitHeartAttack(PLAYERp pp)
{
    short SpriteNum;
    SPRITEp sp;
    USERp u;
    short i = 0;
    short oclipdist;
    USERp pu = User[pp->PlayerSprite];

    typedef struct
    {
        int dist_over, dist_out;
        short ang;
    } MISSILE_PLACEMENT;

    static MISSILE_PLACEMENT mp[] =
    {
        {0, 1100, 0},
    };

    PlayerUpdateAmmo(pp, WPN_HEART, -1);

    SpriteNum = SpawnSprite(STAT_MISSILE_SKIP4, BLOOD_WORM, s_BloodWorm, pp->cursectnum,
                            pp->posx, pp->posy, pp->posz + Z(12), pp->pang, BLOOD_WORM_VELOCITY*2);

    sp = &sprite[SpriteNum];
    u = User[SpriteNum];

    sp->hitag = LUMINOUS; //Always full brightness

    //sp->owner = pp->SpriteP - sprite;
    SetOwner(pp->SpriteP - sprite, SpriteNum);
    sp->shade = -10;
    sp->xrepeat = 52;
    sp->yrepeat = 52;
    sp->clipdist = 0;
    sp->zvel = ((100 - pp->horiz) * HORIZ_MULT);
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    SET(u->Flags2, SPR2_DONT_TARGET_OWNER);
    SET(sp->cstat, CSTAT_SPRITE_INVISIBLE);

    u->floor_dist = Z(1);
    u->ceiling_dist = Z(1);
    u->Dist = 200;

    oclipdist = pp->SpriteP->clipdist;
    pp->SpriteP->clipdist = 1;

    u->xchange = MOVEx(sp->xvel, sp->ang);
    u->ychange = MOVEy(sp->xvel, sp->ang);
    u->zchange = sp->zvel;

#if 0
    if (MissileSetPos(SpriteNum, DoBloodWorm, mp[i].dist_out))
    {
        pp->SpriteP->clipdist = oclipdist;
        KillSprite(SpriteNum);
        continue;
    }
#endif

    MissileSetPos(SpriteNum, DoBloodWorm, mp[i].dist_out);

    pp->SpriteP->clipdist = oclipdist;
    u->Counter = 0;
    u->Counter2 = 0;
    u->Counter3 = 0;
    u->WaitTics = 0;
}
#endif

#if WORM == 2
void
InitHeartAttack(PLAYERp pp)
{
    short SpriteNum;
    SPRITEp sp;
    USERp u;
    short i = 0;
    short oclipdist;
    USERp pu = User[pp->PlayerSprite];

    typedef struct
    {
        int dist_over, dist_out;
        short ang;
    } MISSILE_PLACEMENT;

    static MISSILE_PLACEMENT mp[] =
    {
        {0, 1100, 0},
    };

    PlayerUpdateAmmo(pp, WPN_HEART, -1);

    SpriteNum = SpawnSprite(STAT_MISSILE_SKIP4, BLOOD_WORM, s_BloodWorm, pp->cursectnum,
                            pp->posx, pp->posy, pp->posz + Z(12), pp->pang, BLOOD_WORM_VELOCITY*2);

    sp = &sprite[SpriteNum];
    u = User[SpriteNum];

    sp->hitag = LUMINOUS; //Always full brightness

    //sp->owner = pp->SpriteP - sprite;
    SetOwner(pp->SpriteP - sprite, SpriteNum);
    sp->shade = -10;
    sp->xrepeat = 52;
    sp->yrepeat = 52;
    sp->clipdist = (256) >> 2; // same size a zombie
    sp->zvel = 0;
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    SET(u->Flags2, SPR2_DONT_TARGET_OWNER);
    SET(sp->cstat, CSTAT_SPRITE_INVISIBLE);

    u->floor_dist = Z(1);
    u->ceiling_dist = Z(1);
    u->Dist = 200;

    oclipdist = pp->SpriteP->clipdist;
    pp->SpriteP->clipdist = 1;

    u->xchange = MOVEx(sp->xvel, sp->ang);
    u->ychange = MOVEy(sp->xvel, sp->ang);
    u->zchange = sp->zvel;

    MissileSetPos(SpriteNum, DoBloodWorm, 1100);

    pp->SpriteP->clipdist = oclipdist;
    u->Counter = 0;
    u->Counter2 = 0;
    u->Counter3 = 0;
    u->WaitTics = 0;
}
#endif

int ContinueHitscan(PLAYERp pp, short sectnum, int x, int y, int z, short ang, int xvect, int yvect, int zvect)
{
    short j;
    hitdata_t hitinfo;
    USERp u = User[pp->PlayerSprite];

    FAFhitscan(x, y, z, sectnum,
               xvect, yvect, zvect,
               &hitinfo, CLIPMASK_MISSILE);

    if (hitinfo.sect < 0)
        return 0;

    if (hitinfo.sprite < 0 && hitinfo.wall < 0)
    {
        if (labs(hitinfo.pos.z - sector[hitinfo.sect].ceilingz) <= Z(1))
        {
            hitinfo.pos.z += Z(16);
            if (TEST(sector[hitinfo.sect].ceilingstat, CEILING_STAT_PLAX))
                return 0;
        }
        else if (labs(hitinfo.pos.z - sector[hitinfo.sect].floorz) <= Z(1))
        {
        }
    }

    if (hitinfo.wall >= 0)
    {
        if (wall[hitinfo.wall].nextsector >= 0)
        {
            if (TEST(sector[wall[hitinfo.wall].nextsector].ceilingstat, CEILING_STAT_PLAX))
            {
                if (hitinfo.pos.z < sector[wall[hitinfo.wall].nextsector].ceilingz)
                {
                    return 0;
                }
            }
        }

        if (wall[hitinfo.wall].lotag == TAG_WALL_BREAK)
        {
            HitBreakWall(&wall[hitinfo.wall], hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, ang, u->ID);
            return 0;
        }

        QueueHole(ang,hitinfo.sect,hitinfo.wall,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z);
    }

    // hit a sprite?
    if (hitinfo.sprite >= 0)
    {
        SPRITEp hsp = &sprite[hitinfo.sprite];

        if (hsp->lotag == TAG_SPRITE_HIT_MATCH)
        {
            if (MissileHitMatch(-1, WPN_SHOTGUN, hitinfo.sprite))
                return 0;
        }

        if (TEST(hsp->extra, SPRX_BREAKABLE))
        {
            HitBreakSprite(hitinfo.sprite,0);
            return 0;
        }

        if (BulletHitSprite(pp->SpriteP, hitinfo.sprite, hitinfo.sect, hitinfo.wall, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, 0))
            return 0;

        // hit a switch?
        if (TEST(hsp->cstat, CSTAT_SPRITE_WALL) && (hsp->lotag || hsp->hitag))
        {
            ShootableSwitch(hitinfo.sprite,-1);
        }
    }

    j = SpawnShotgunSparks(pp, hitinfo.sect, hitinfo.wall, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, ang);
    DoHitscanDamage(j, hitinfo.sprite);

    return 0;
}

int
InitShotgun(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    SPRITEp wp, hsp;
    USERp wu;
    short daang,ndaang, i, j, exp;
    hitdata_t hitinfo;
    short nsect;
    int daz, ndaz;
    int nx,ny,nz;
    int xvect,yvect,zvect;
    short cstat = 0;
    SPRITEp sp,ep;

    PlayerUpdateAmmo(pp, u->WeaponNum, -1);

    PlaySound(DIGI_RIOTFIRE2, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    u->Vis = 128;

    if (pp->WpnShotgunAuto)
    {
        switch (pp->WpnShotgunType)
        {
        case 1:
            pp->WpnShotgunAuto--;
        }
    }

    nx = pp->posx;
    ny = pp->posy;
    daz = nz = pp->posz + pp->bob_z;
    nsect = pp->cursectnum;
    sp = pp->SpriteP;

    daang = 64;
    if (WeaponAutoAimHitscan(sp, &daz, &daang, FALSE) != -1)
    {
    }
    else
    {
        daz = (100 - pp->horiz) * 2000;
        daang = pp->pang;
    }

    for (i = 0; i < 12; i++)
    {
        if (pp->WpnShotgunType == 0)
        {
            ndaz = daz + (RANDOM_RANGE(Z(120)) - Z(45));
            ndaang = NORM_ANGLE(daang + (RANDOM_RANGE(30) - 15));
        }
        else
        {
            ndaz = daz + (RANDOM_RANGE(Z(200)) - Z(65));
            ndaang = NORM_ANGLE(daang + (RANDOM_RANGE(70) - 30));
        }

        xvect = sintable[NORM_ANGLE(ndaang + 512)];
        yvect = sintable[NORM_ANGLE(ndaang)];
        zvect = ndaz;
        FAFhitscan(nx, ny, nz, nsect,               // Start position
                   xvect, yvect, zvect,
                   &hitinfo, CLIPMASK_MISSILE);

        if (hitinfo.sect < 0)
        {
            ////DSPRINTF(ds,"PROBLEM! - FAFhitscan returned a bad hitinfo.sect");
            //MONO_PRINT(ds);
            continue;
        }

        if (hitinfo.sprite < 0 && hitinfo.wall < 0)
        {
            if (labs(hitinfo.pos.z - sector[hitinfo.sect].ceilingz) <= Z(1))
            {
                hitinfo.pos.z += Z(16);
                SET(cstat, CSTAT_SPRITE_YFLIP);

                if (TEST(sector[hitinfo.sect].ceilingstat, CEILING_STAT_PLAX))
                    continue;

                if (SectorIsUnderwaterArea(hitinfo.sect))
                {
                    WarpToSurface(&hitinfo.sect, &hitinfo.pos.x, &hitinfo.pos.y, &hitinfo.pos.z);
                    ContinueHitscan(pp, hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, ndaang, xvect, yvect, zvect);
                    continue;
                }
            }
            else if (labs(hitinfo.pos.z - sector[hitinfo.sect].floorz) <= Z(1))
            {
                if (TEST(sector[hitinfo.sect].extra, SECTFX_LIQUID_MASK) != SECTFX_LIQUID_NONE)
                {
                    SpawnSplashXY(hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,hitinfo.sect);

                    if (SectorIsDiveArea(hitinfo.sect))
                    {
                        WarpToUnderwater(&hitinfo.sect, &hitinfo.pos.x, &hitinfo.pos.y, &hitinfo.pos.z);
                        ContinueHitscan(pp, hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, ndaang, xvect, yvect, zvect);
                    }

                    continue;
                }
            }
        }

        if (hitinfo.wall >= 0)
        {
            if (wall[hitinfo.wall].nextsector >= 0)
            {
                if (TEST(sector[wall[hitinfo.wall].nextsector].ceilingstat, CEILING_STAT_PLAX))
                {
                    if (hitinfo.pos.z < sector[wall[hitinfo.wall].nextsector].ceilingz)
                    {
                        continue;
                    }
                }
            }

            if (wall[hitinfo.wall].lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(&wall[hitinfo.wall], hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, ndaang, u->ID);
                continue;
            }

            QueueHole(ndaang,hitinfo.sect,hitinfo.wall,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z);
        }

        // hit a sprite?
        if (hitinfo.sprite >= 0)
        {
            SPRITEp hsp = &sprite[hitinfo.sprite];
            USERp hu = User[hitinfo.sprite];

            if (hu && hu->ID == TRASHCAN)   // JBF: added null check
            {
                extern STATE s_TrashCanPain[];

                PlaySound(DIGI_TRASHLID, &sp->x, &sp->y, &sp->z, v3df_none);
                if (hu->WaitTics <= 0)
                {
                    hu->WaitTics = SEC(2);
                    ChangeState(hitinfo.sprite,s_TrashCanPain);
                }
            }

            if (hsp->lotag == TAG_SPRITE_HIT_MATCH)
            {
                if (MissileHitMatch(-1, WPN_SHOTGUN, hitinfo.sprite))
                    continue;
            }

            if (TEST(hsp->extra, SPRX_BREAKABLE))
            {
                HitBreakSprite(hitinfo.sprite,0);
                continue;
            }

            if (BulletHitSprite(pp->SpriteP, hitinfo.sprite, hitinfo.sect, hitinfo.wall, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, SHOTGUN_SMOKE))
                continue;

            // hit a switch?
            if (TEST(hsp->cstat, CSTAT_SPRITE_WALL) && (hsp->lotag || hsp->hitag))
            {
                ShootableSwitch(hitinfo.sprite,-1);
            }
        }

        j = SpawnShotgunSparks(pp, hitinfo.sect, hitinfo.wall, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, ndaang);
        DoHitscanDamage(j, hitinfo.sprite);
    }

    DoPlayerBeginRecoil(pp, SHOTGUN_RECOIL_AMT);
    return 0;
}

int
InitLaser(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w;
    short oclipdist;

    DoPlayerBeginRecoil(pp, RAIL_RECOIL_AMT);

    PlayerUpdateAmmo(pp, u->WeaponNum, -1);

    PlaySound(DIGI_RIOTFIRE, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_doppler);

    nx = pp->posx;
    ny = pp->posy;

    nz = pp->posz + pp->bob_z + Z(8);

    // Spawn a shot
    // Inserting and setting up variables

    w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R0, s_Laser, pp->cursectnum,
                    nx, ny, nz, pp->pang, 300);

    wp = &sprite[w];
    wu = User[w];

    wp->hitag = LUMINOUS; //Always full brightness
    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = 52;
    wp->xrepeat = 52;
    wp->shade = -15;
    wp->clipdist = 64L>>2;

    // the slower the missile travels the less of a zvel it needs
    wp->zvel = ((100 - pp->horiz) * HORIZ_MULT);
    wp->zvel /= 4;

    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 200;
    wu->ceiling_dist = Z(1);
    wu->floor_dist = Z(1);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    // at certain angles the clipping box was big enough to block the
    // initial positioning of the fireball.
    oclipdist = pp->SpriteP->clipdist;
    pp->SpriteP->clipdist = 0;

    wp->ang = NORM_ANGLE(wp->ang + 512);
    HelpMissileLateral(w, 900);
    wp->ang = NORM_ANGLE(wp->ang - 512);

    if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    // the slower the missile travels the less of a zvel it needs
    // move it 1200 dist in increments - works better
    if (MissileSetPos(w, DoLaserStart, 300))
    {
        pp->SpriteP->clipdist = oclipdist;
        KillSprite(w);
        return 0;
    }
    if (MissileSetPos(w, DoLaserStart, 300))
    {
        pp->SpriteP->clipdist = oclipdist;
        KillSprite(w);
        return 0;
    }
    if (MissileSetPos(w, DoLaserStart, 300))
    {
        pp->SpriteP->clipdist = oclipdist;
        KillSprite(w);
        return 0;
    }
    if (MissileSetPos(w, DoLaserStart, 300))
    {
        pp->SpriteP->clipdist = oclipdist;
        KillSprite(w);
        return 0;
    }

    pp->SpriteP->clipdist = oclipdist;

    if (WeaponAutoAim(pp->SpriteP, w, 32, FALSE) == -1)
    {
        wp->ang = NORM_ANGLE(wp->ang - 5);
    }

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    return 0;
}



int
InitRail(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w;
    short oclipdist;
    int zvel;

    if (SW_SHAREWARE) return FALSE; // JBF: verify

    DoPlayerBeginRecoil(pp, RAIL_RECOIL_AMT);

    PlayerUpdateAmmo(pp, u->WeaponNum, -1);

    PlaySound(DIGI_RAILFIRE, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    u->Vis = 128;

    nx = pp->posx;
    ny = pp->posy;

    nz = pp->posz + pp->bob_z + Z(11);

    // Spawn a shot
    // Inserting and setting up variables

    w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R1, &s_Rail[0][0], pp->cursectnum,
                    nx, ny, nz, pp->pang, 1200);

    wp = &sprite[w];
    wu = User[w];

    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = 52;
    wp->xrepeat = 52;
    wp->shade = -15;
    zvel = ((100 - pp->horiz) * (HORIZ_MULT+17));

    wu->RotNum = 5;
    NewStateGroup(w, &sg_Rail[0]);

    wu->WeaponNum = u->WeaponNum;
    wu->Radius = RAIL_RADIUS;
    wu->ceiling_dist = Z(1);
    wu->floor_dist = Z(1);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER|CSTAT_SPRITE_INVISIBLE);
    SET(wp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    // at certain angles the clipping box was big enough to block the
    // initial positioning
    oclipdist = pp->SpriteP->clipdist;
    pp->SpriteP->clipdist = 0;
    wp->clipdist = 32L>>2;

    wp->ang = NORM_ANGLE(wp->ang + 512);
    HelpMissileLateral(w, 700);
    wp->ang = NORM_ANGLE(wp->ang - 512);

    if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    if (TestMissileSetPos(w, DoRailStart, 1200, zvel))
    {
        pp->SpriteP->clipdist = oclipdist;
        KillSprite(w);
        return 0;
    }

    pp->SpriteP->clipdist = oclipdist;

    wp->zvel = zvel >> 1;
    if (WeaponAutoAim(pp->SpriteP, w, 32, FALSE) == -1)
    {
        wp->ang = NORM_ANGLE(wp->ang - 4);
    }
    else
        zvel = wp->zvel;  // Let autoaiming set zvel now

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = zvel;

    return 0;
}

int
InitZillaRail(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w;
    short oclipdist;
    int zvel;

    if (SW_SHAREWARE) return FALSE; // JBF: verify

    PlaySound(DIGI_RAILFIRE, &sp->x, &sp->y, &sp->z, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    u->Vis = 128;

    nx = sp->x;
    ny = sp->y;

    nz = SPRITEp_TOS(sp);

    // Spawn a shot
    // Inserting and setting up variables

    w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R1, &s_Rail[0][0], sp->sectnum,
                    nx, ny, nz, sp->ang, 1200);

    wp = &sprite[w];
    wu = User[w];

    SetOwner(SpriteNum, w);
    wp->yrepeat = 52;
    wp->xrepeat = 52;
    wp->shade = -15;
    zvel = (100 * (HORIZ_MULT+17));

    wu->RotNum = 5;
    NewStateGroup(w, &sg_Rail[0]);

    wu->WeaponNum = u->WeaponNum;
    wu->Radius = RAIL_RADIUS;
    wu->ceiling_dist = Z(1);
    wu->floor_dist = Z(1);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER|CSTAT_SPRITE_INVISIBLE);
    SET(wp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    // at certain angles the clipping box was big enough to block the
    // initial positioning
    oclipdist = sp->clipdist;
    sp->clipdist = 0;
    wp->clipdist = 32L>>2;

    wp->ang = NORM_ANGLE(wp->ang + 512);
    HelpMissileLateral(w, 700);
    wp->ang = NORM_ANGLE(wp->ang - 512);

    if (SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    if (TestMissileSetPos(w, DoRailStart, 1200, zvel))
    {
        sp->clipdist = oclipdist;
        KillSprite(w);
        return 0;
    }

    sp->clipdist = oclipdist;

    wp->zvel = zvel >> 1;
    if (WeaponAutoAim(sp, w, 32, FALSE) == -1)
    {
        wp->ang = NORM_ANGLE(wp->ang - 4);
    }
    else
        zvel = wp->zvel;  // Let autoaiming set zvel now

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = zvel;

    return 0;
}

int
InitRocket(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w;
    short oclipdist;
    int zvel;

#if 0
    static short lat_dist[3] =
    {
        //800,1000,600
        600,900,300
    };

    static short z_off[3] =
    {
        //Z(8), Z(14), Z(14)
        Z(6), Z(12), Z(12)
    };
#endif

    DoPlayerBeginRecoil(pp, ROCKET_RECOIL_AMT);

    PlayerUpdateAmmo(pp, u->WeaponNum, -1);

    PlaySound(DIGI_RIOTFIRE, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    u->Vis = 128;

    nx = pp->posx;
    ny = pp->posy;

    // Spawn a shot
    // Inserting and setting up variables
    //nz = pp->posz + pp->bob_z + Z(12);
    nz = pp->posz + pp->bob_z + Z(8);
    w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R0, &s_Rocket[0][0], pp->cursectnum,
                    nx, ny, nz, pp->pang, ROCKET_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    //wp->owner = pp->PlayerSprite;
    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = 90;
    wp->xrepeat = 90;
    wp->shade = -15;
    zvel = ((100 - pp->horiz) * (HORIZ_MULT+35));

    wp->clipdist = 64L>>2;

    wu->RotNum = 5;
    NewStateGroup(w, &sg_Rocket[0]);

    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 2000;
    wu->ceiling_dist = Z(3);
    wu->floor_dist = Z(3);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    // Set default palette
    wp->pal = wu->spal = 17; // White

    if (pp->WpnRocketHeat)
    {
        switch (pp->WpnRocketType)
        {
        case 1:
            pp->WpnRocketHeat--;
            SET(wu->Flags, SPR_FIND_PLAYER);
            wp->pal = wu->spal = 20; // Yellow
            break;
        }
    }

    // at certain angles the clipping box was big enough to block the
    // initial positioning of the fireball.
    oclipdist = pp->SpriteP->clipdist;
    pp->SpriteP->clipdist = 0;

    wp->ang = NORM_ANGLE(wp->ang + 512);
    HelpMissileLateral(w, 900);
    wp->ang = NORM_ANGLE(wp->ang - 512);

    if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    // cancel smoke trail
    wu->Counter = 1;
    if (TestMissileSetPos(w, DoRocket, 1200, zvel))
    {
        pp->SpriteP->clipdist = oclipdist;
        KillSprite(w);
        return 0;
    }
    // inable smoke trail
    wu->Counter = 0;

    pp->SpriteP->clipdist = oclipdist;

    wp->zvel = zvel >> 1;
    if (WeaponAutoAim(pp->SpriteP, w, 32, FALSE) == -1)
    {
        wp->ang = NORM_ANGLE(wp->ang - 5);
    }
    else
        zvel = wp->zvel;  // Let autoaiming set zvel now

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = zvel;

    return 0;
}

int
InitBunnyRocket(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w;
    short oclipdist;
    int zvel;

#if 0
    static short lat_dist[3] =
    {
        //800,1000,600
        600,900,300
    };

    static short z_off[3] =
    {
        //Z(8), Z(14), Z(14)
        Z(6), Z(12), Z(12)
    };
#endif

    DoPlayerBeginRecoil(pp, ROCKET_RECOIL_AMT);

    PlayerUpdateAmmo(pp, u->WeaponNum, -1);

    PlaySound(DIGI_BUNNYATTACK, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_doppler);

    nx = pp->posx;
    ny = pp->posy;

    // Spawn a shot
    // Inserting and setting up variables
    //nz = pp->posz + pp->bob_z + Z(12);
    nz = pp->posz + pp->bob_z + Z(8);
    w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R4, &s_BunnyRocket[0][0], pp->cursectnum,
                    nx, ny, nz, pp->pang, ROCKET_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    //wp->owner = pp->PlayerSprite;
    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = 64;
    wp->xrepeat = 64;
    wp->shade = -15;
    zvel = ((100 - pp->horiz) * (HORIZ_MULT+35));

    wp->clipdist = 64L>>2;

    wu->RotNum = 5;
    NewStateGroup(w, &sg_BunnyRocket[0]);

    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 2000;
    wu->ceiling_dist = Z(3);
    wu->floor_dist = Z(3);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    SET(wu->Flags, SPR_XFLIP_TOGGLE);

    if (pp->WpnRocketHeat)
    {
        switch (pp->WpnRocketType)
        {
        case 1:
            pp->WpnRocketHeat--;
            SET(wu->Flags, SPR_FIND_PLAYER);
        }
    }

    // at certain angles the clipping box was big enough to block the
    // initial positioning of the fireball.
    oclipdist = pp->SpriteP->clipdist;
    pp->SpriteP->clipdist = 0;

    wp->ang = NORM_ANGLE(wp->ang + 512);
    HelpMissileLateral(w, 900);
    wp->ang = NORM_ANGLE(wp->ang - 512);

    if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    // cancel smoke trail
    wu->Counter = 1;
    if (TestMissileSetPos(w, DoRocket, 1200, zvel))
    {
        pp->SpriteP->clipdist = oclipdist;
        KillSprite(w);
        return 0;
    }
    // inable smoke trail
    wu->Counter = 0;

    pp->SpriteP->clipdist = oclipdist;

    wp->zvel = zvel >> 1;
    if (WeaponAutoAim(pp->SpriteP, w, 32, FALSE) == -1)
    {
        wp->ang = NORM_ANGLE(wp->ang - 5);
    }
    else
        zvel = wp->zvel;  // Let autoaiming set zvel now

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = zvel;
    wu->spal = wp->pal = PALETTE_PLAYER1;

    return 0;
}

int
InitNuke(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w;
    short oclipdist;
    int zvel;


//    PlayerUpdateAmmo(pp, u->WeaponNum, -1);
    if (pp->WpnRocketNuke > 0)
        pp->WpnRocketNuke = 0;  // Bye Bye little nukie.
    else
        return 0;

    DoPlayerBeginRecoil(pp, ROCKET_RECOIL_AMT*12);


    PlaySound(DIGI_RIOTFIRE, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    u->Vis = 128;

    nx = pp->posx;
    ny = pp->posy;

    // Spawn a shot
    // Inserting and setting up variables
    //nz = pp->posz + pp->bob_z + Z(12);
    nz = pp->posz + pp->bob_z + Z(8);
    w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R0, &s_Rocket[0][0], pp->cursectnum,
                    nx, ny, nz, pp->pang, 700);

    wp = &sprite[w];
    wu = User[w];

    //wp->owner = pp->PlayerSprite;
    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = 128;
    wp->xrepeat = 128;
    wp->shade = -15;
    zvel = ((100 - pp->horiz) * (HORIZ_MULT-36));
    wp->clipdist = 64L>>2;

    // Set to red palette
    wp->pal = wu->spal = 19;

    wu->RotNum = 5;
    NewStateGroup(w, &sg_Rocket[0]);

    wu->WeaponNum = u->WeaponNum;
    wu->Radius = NUKE_RADIUS;
    wu->ceiling_dist = Z(3);
    wu->floor_dist = Z(3);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    // at certain angles the clipping box was big enough to block the
    // initial positioning of the fireball.
    oclipdist = pp->SpriteP->clipdist;
    pp->SpriteP->clipdist = 0;

    wp->ang = NORM_ANGLE(wp->ang + 512);
    HelpMissileLateral(w, 900);
    wp->ang = NORM_ANGLE(wp->ang - 512);

    if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    // cancel smoke trail
    wu->Counter = 1;
    if (TestMissileSetPos(w, DoRocket, 1200, zvel))
    {
        pp->SpriteP->clipdist = oclipdist;
        KillSprite(w);
        return 0;
    }
    // inable smoke trail
    wu->Counter = 0;

    pp->SpriteP->clipdist = oclipdist;

    wp->zvel = zvel >> 1;
    if (WeaponAutoAim(pp->SpriteP, w, 32, FALSE) == -1)
    {
        wp->ang = NORM_ANGLE(wp->ang - 5);
    }
    else
        zvel = wp->zvel;  // Let autoaiming set zvel now

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = zvel;

    PlayerDamageSlide(pp, -40, NORM_ANGLE(pp->pang+1024)); // Recoil slide

    return 0;
}

int
InitEnemyNuke(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w;
    short oclipdist;
    int zvel;


    PlaySound(DIGI_RIOTFIRE, &sp->x, &sp->y, &sp->z, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    u->Vis = 128;

    nx = sp->x;
    ny = sp->y;

    // Spawn a shot
    // Inserting and setting up variables
    //nz = pp->posz + pp->bob_z + Z(12);
    nz = sp->z + Z(40);
    w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R0, &s_Rocket[0][0], sp->sectnum,
                    nx, ny, nz, sp->ang, 700);

    wp = &sprite[w];
    wu = User[w];

    if (u->ID == ZOMBIE_RUN_R0)
        SetOwner(sp->owner, w);
    else
        SetOwner(SpriteNum, w);

    wp->yrepeat = 128;
    wp->xrepeat = 128;
    wp->shade = -15;
    zvel = (100 * (HORIZ_MULT-36));
    wp->clipdist = 64L>>2;

    // Set to red palette
    wp->pal = wu->spal = 19;

    wu->RotNum = 5;
    NewStateGroup(w, &sg_Rocket[0]);

    wu->WeaponNum = u->WeaponNum;
    wu->Radius = NUKE_RADIUS;
    wu->ceiling_dist = Z(3);
    wu->floor_dist = Z(3);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    wp->ang = NORM_ANGLE(wp->ang + 512);
    HelpMissileLateral(w, 500);
    wp->ang = NORM_ANGLE(wp->ang - 512);

    if (SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    // cancel smoke trail
    wu->Counter = 1;
    if (TestMissileSetPos(w, DoRocket, 1200, zvel))
    {
        KillSprite(w);
        return 0;
    }

    // enable smoke trail
    wu->Counter = 0;

    wp->zvel = zvel >> 1;
    if (WeaponAutoAim(sp, w, 32, FALSE) == -1)
    {
        wp->ang = NORM_ANGLE(wp->ang - 5);
    }
    else
        zvel = wp->zvel;  // Let autoaiming set zvel now

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = zvel;

    return 0;
}

int
InitMicro(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    USERp wu,hu;
    SPRITEp wp,hp;
    int nx, ny, nz, dist;
    short w;
    short oclipdist;
    short i,ang;
    TARGET_SORTp ts = TargetSort;

    //DoPlayerBeginRecoil(pp, MICRO_RECOIL_AMT);
    //PlayerUpdateAmmo(pp, u->WeaponNum, -1);

    nx = pp->posx;
    ny = pp->posy;

#define MAX_MICRO 1

    DoPickTarget(pp->SpriteP, 256, FALSE);

    if (TargetSortCount > MAX_MICRO)
        TargetSortCount = MAX_MICRO;

    for (i = 0; i < MAX_MICRO; i++)
    {
        if (ts < &TargetSort[TargetSortCount] && ts->sprite_num >= 0)
        {
            hp = &sprite[ts->sprite_num];
            hu = User[ts->sprite_num];

            ang = getangle(hp->x - nx, hp->y - ny);

            ts++;
        }
        else
        {
            hp = NULL;
            hu = NULL;
            ang = pp->pang;
        }

        nz = pp->posz + pp->bob_z + Z(14);
        nz += Z(RANDOM_RANGE(20)) - Z(10);

        // Spawn a shot
        // Inserting and setting up variables

        w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R0, &s_Micro[0][0], pp->cursectnum,
                        nx, ny, nz, ang, 1200);

        wp = &sprite[w];
        wu = User[w];

        SetOwner(pp->PlayerSprite, w);
        wp->yrepeat = 24;
        wp->xrepeat = 24;
        wp->shade = -15;
        wp->zvel = ((100 - pp->horiz) * HORIZ_MULT);
        wp->clipdist = 64L>>2;

        // randomize zvelocity
        wp->zvel += RANDOM_RANGE(Z(8)) - Z(5);

        wu->RotNum = 5;
        NewStateGroup(w, &sg_Micro[0]);

        wu->WeaponNum = u->WeaponNum;
        wu->Radius = 200;
        wu->ceiling_dist = Z(2);
        wu->floor_dist = Z(2);
        RESET(wp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        SET(wp->cstat, CSTAT_SPRITE_YCENTER);
        SET(wp->cstat, CSTAT_SPRITE_INVISIBLE);

        wu->WaitTics = 10 + RANDOM_RANGE(40);

        // at certain angles the clipping box was big enough to block the
        // initial positioning of the fireball.
        oclipdist = pp->SpriteP->clipdist;
        pp->SpriteP->clipdist = 0;

        wp->ang = NORM_ANGLE(wp->ang + 512);
#define MICRO_LATERAL 5000
        HelpMissileLateral(w, 1000 + (RANDOM_RANGE(MICRO_LATERAL) - DIV2(MICRO_LATERAL)));
        wp->ang = NORM_ANGLE(wp->ang - 512);

        if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(wp))
            SET(wu->Flags, SPR_UNDERWATER);

        // cancel smoke trail
        wu->Counter = 1;
        if (MissileSetPos(w, DoMicro, 700))
        {
            pp->SpriteP->clipdist = oclipdist;
            KillSprite(w);
            continue;
        }
        // inable smoke trail
        wu->Counter = 0;

        pp->SpriteP->clipdist = oclipdist;

#define MICRO_ANG 400

        if (hp)
        {
            dist = Distance(wp->x, wp->y, hp->x, hp->y);
            if (dist != 0)
            {
                int zh;
                zh = SPRITEp_TOS(hp) + DIV4(SPRITEp_SIZE_Z(hp));
                wp->zvel = (wp->xvel * (zh - wp->z)) / dist;
            }

            wu->WpnGoal = ts->sprite_num;
            SET(hu->Flags, SPR_TARGETED);
            SET(hu->Flags, SPR_ATTACKED);
        }
        else
        {
            wp->ang = NORM_ANGLE(wp->ang + (RANDOM_RANGE(MICRO_ANG) - DIV2(MICRO_ANG)) - 16);
        }

        wu->xchange = MOVEx(wp->xvel, wp->ang);
        wu->ychange = MOVEy(wp->xvel, wp->ang);
        wu->zchange = wp->zvel;
    }

    return 0;
}

int
InitRipperSlash(short SpriteNum)
{
    USERp u = User[SpriteNum], hu;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SPRITEp hp;
    short i, nexti;
    unsigned stat;
    int dist, a, b, c;

    PlaySound(DIGI_RIPPER2ATTACK, &sp->x, &sp->y, &sp->z, v3df_none);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            hp = &sprite[i];
            hu = User[i];

            if (i == SpriteNum)
                break;

            if ((unsigned)FindDistance3D(sp->x - hp->x, sp->y - hp->y, (sp->z - hp->z)>>4) > hu->Radius + u->Radius)
                continue;

            DISTANCE(hp->x, hp->y, sp->x, sp->y, dist, a, b, c);

            if (dist < CLOSE_RANGE_DIST_FUDGE(sp, hp, 600) && FACING_RANGE(hp, sp, 150))
            {
//                  PlaySound(PlayerPainVocs[RANDOM_RANGE(MAX_PAIN)], &sp->x, &sp->y, &sp->z, v3df_none);
                DoDamage(i, SpriteNum);
            }
        }
    }

    return 0;
}

int
InitBunnySlash(short SpriteNum)
{
    USERp u = User[SpriteNum], hu;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SPRITEp hp;
    short i, nexti;
    unsigned stat;
    int dist, a, b, c;

    PlaySound(DIGI_BUNNYATTACK, &sp->x, &sp->y, &sp->z, v3df_none);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            hp = &sprite[i];
            hu = User[i];

            if (i == SpriteNum)
                break;

            DISTANCE(hp->x, hp->y, sp->x, sp->y, dist, a, b, c);

            if (dist < CLOSE_RANGE_DIST_FUDGE(sp, hp, 600) && FACING_RANGE(hp, sp, 150))
            {
                DoDamage(i, SpriteNum);
            }
        }
    }

    return 0;
}


int
InitSerpSlash(short SpriteNum)
{
    USERp u = User[SpriteNum], hu;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SPRITEp hp;
    short i, nexti;
    unsigned stat;
    int dist, a, b, c;

    PlaySound(DIGI_SERPSWORDATTACK, &sp->x, &sp->y, &sp->z, v3df_none);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            hp = &sprite[i];
            hu = User[i];

            if (i == SpriteNum)
                break;

            DISTANCE(hp->x, hp->y, sp->x, sp->y, dist, a, b, c);

            if (dist < CLOSE_RANGE_DIST_FUDGE(sp, hp, 800) && FACING_RANGE(hp, sp, 150))
            {
//                PlaySound(PlayerPainVocs[RANDOM_RANGE(MAX_PAIN)], &sp->x, &sp->y, &sp->z, v3df_none);
                DoDamage(i, SpriteNum);
            }
        }
    }

    return 0;
}

SWBOOL
WallSpriteInsideSprite(SPRITEp wsp, SPRITEp sp)
{
    int x1, y1, x2, y2;
    int xoff, l, k;
    int dax, day;
    int xsiz, mid_dist;

    x1 = wsp->x;
    y1 = wsp->y;

    xoff = (int) TILE_XOFF(wsp->picnum) + (int) wsp->xoffset;

    if (TEST(wsp->cstat, CSTAT_SPRITE_XFLIP))
        xoff = -xoff;

    // x delta
    dax = sintable[wsp->ang] * wsp->xrepeat;
    // y delta
    day = sintable[NORM_ANGLE(wsp->ang + 1024 + 512)] * wsp->xrepeat;

    xsiz = tilesiz[wsp->picnum].x;
    mid_dist = DIV2(xsiz) + xoff;

    // starting from the center find the first point
    x1 -= mulscale16(dax, mid_dist);
    // starting from the first point find the end point
    x2 = x1 + mulscale16(dax, xsiz);

    y1 -= mulscale16(day, mid_dist);
    y2 = y1 + mulscale16(day, xsiz);

    return clipinsideboxline(sp->x, sp->y, x1, y1, x2, y2, ((int) sp->clipdist) << 2);
}


int
DoBladeDamage(short SpriteNum)
{
    USERp u = User[SpriteNum], hu;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SPRITEp hp;
    short i, nexti;
    unsigned stat;
    int dist, a, b, c;

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            hp = &sprite[i];

            if (i == SpriteNum)
                break;

            if (!TEST(hp->extra, SPRX_PLAYER_OR_ENEMY))
                continue;

            DISTANCE(hp->x, hp->y, sp->x, sp->y, dist, a, b, c);

            if (dist > 2000)
                continue;

            dist = FindDistance3D(sp->x - hp->x, sp->y - hp->y, (sp->z - hp->z)>>4);

            if (dist > 2000)
                continue;

            if (WallSpriteInsideSprite(sp, hp))
            {
                DoDamage(i, SpriteNum);
//                    PlaySound(PlayerPainVocs[RANDOM_RANGE(MAX_PAIN)], &sp->x, &sp->y, &sp->z, v3df_none);
            }
        }
    }

    return 0;
}

int
DoStaticFlamesDamage(short SpriteNum)
{
    USERp u = User[SpriteNum], hu;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SPRITEp hp;
    short i, nexti;
    unsigned stat;
    int dist, a, b, c;

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            hp = &sprite[i];

            if (i == SpriteNum)
                break;

            if (!TEST(hp->extra, SPRX_PLAYER_OR_ENEMY))
                continue;

            DISTANCE(hp->x, hp->y, sp->x, sp->y, dist, a, b, c);

            if (dist > 2000)
                continue;

            dist = FindDistance3D(sp->x - hp->x, sp->y - hp->y, (sp->z - hp->z)>>4);

            if (dist > 2000)
                continue;

            if (SpriteOverlap(SpriteNum, i))  // If sprites are overlapping, cansee will fail!
                DoDamage(i, SpriteNum);
            else if (u->Radius > 200)
            {
                if (FAFcansee(sp->x,sp->y,SPRITEp_MID(sp),sp->sectnum,hp->x,hp->y,SPRITEp_MID(hp),hp->sectnum))
                    DoDamage(i, SpriteNum);
            }
        }
    }

    return 0;
}

int
InitCoolgBash(short SpriteNum)
{
    USERp u = User[SpriteNum],hu;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SPRITEp hp;
    short i, nexti;
    unsigned stat;
    int dist, a, b, c;

    PlaySound(DIGI_CGTHIGHBONE, &sp->x, &sp->y, &sp->z, v3df_none);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            hp = &sprite[i];

            if (i == SpriteNum)
                break;

            // don't set off mine
            if (!TEST(hp->extra, SPRX_PLAYER_OR_ENEMY))
                continue;

            DISTANCE(hp->x, hp->y, sp->x, sp->y, dist, a, b, c);

            if (dist < CLOSE_RANGE_DIST_FUDGE(sp, hp, 600) && FACING_RANGE(hp, sp, 150))
            {
//                PlaySound(PlayerPainVocs[RANDOM_RANGE(MAX_PAIN)], &sp->x, &sp->y, &sp->z, v3df_none);
                DoDamage(i, SpriteNum);
            }
        }
    }

    return 0;
}

int
InitSkelSlash(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SPRITEp hp;
    short i, nexti;
    unsigned stat;
    int dist, a, b, c;

    PlaySound(DIGI_SPBLADE, &sp->x, &sp->y, &sp->z, v3df_none);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            hp = &sprite[i];

            if (i == SpriteNum)
                break;

            DISTANCE(hp->x, hp->y, sp->x, sp->y, dist, a, b, c);

            if (dist < CLOSE_RANGE_DIST_FUDGE(sp, hp, 600) && FACING_RANGE(hp, sp, 150))
            {
//                PlaySound(PlayerPainVocs[RANDOM_RANGE(MAX_PAIN)], &sp->x, &sp->y, &sp->z, v3df_none);
                DoDamage(i, SpriteNum);
            }
        }
    }

    return 0;
}

int
InitGoroChop(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SPRITEp hp;
    short i, nexti;
    unsigned stat;
    int dist, a, b, c;

    PlaySound(DIGI_GRDSWINGAXE, &sp->x, &sp->y, &sp->z, v3df_none);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            hp = &sprite[i];

            if (i == SpriteNum)
                break;

            DISTANCE(hp->x, hp->y, sp->x, sp->y, dist, a, b, c);

            if (dist < CLOSE_RANGE_DIST_FUDGE(sp, hp, 700) && FACING_RANGE(hp, sp, 150))
            {
                PlaySound(DIGI_GRDAXEHIT, &sp->x, &sp->y, &sp->z, v3df_none);
                DoDamage(i, SpriteNum);
            }
        }
    }

    return 0;
}

int
InitHornetSting(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    short HitSprite = NORM_SPRITE(u->ret);

    DoDamage(HitSprite, SpriteNum);
    InitActorReposition(SpriteNum);

    return 0;
}

int
InitSerpSpell(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], np;
    USERp u = User[SpriteNum], nu;
    int dist;
    short New, save_ang, i;
    short oclipdist;

    static short lat_ang[] =
    {
        512, -512
    };

    static short delta_ang[] =
    {
        -10, 10
    };

    for (i = 0; i < 2; i++)
    {
        sp->ang = getangle(u->tgt_sp->x - sp->x, u->tgt_sp->y - sp->y);

        New = SpawnSprite(STAT_MISSILE, SERP_METEOR, &sg_SerpMeteor[0][0], sp->sectnum,
                          sp->x, sp->y, sp->z, sp->ang, 1500);

        np = &sprite[New];
        nu = User[New];

        np->z = SPRITEp_TOS(sp);

        nu->RotNum = 5;
        NewStateGroup(New, &sg_SerpMeteor[0]);
        nu->StateEnd = s_MirvMeteorExp;

        //np->owner = SpriteNum;
        SetOwner(SpriteNum, New);
        np->shade = -40;
        PlaySound(DIGI_SERPMAGICLAUNCH, &sp->x, &sp->y, &sp->z, v3df_none);
        nu->spal = np->pal = 27; // Bright Green
        np->xrepeat = 64;
        np->yrepeat = 64;
        np->clipdist = 32L >> 2;
        np->zvel = 0;
        SET(np->cstat, CSTAT_SPRITE_YCENTER);

        nu->ceiling_dist = Z(16);
        nu->floor_dist = Z(16);
        nu->Dist = 200;

        oclipdist = sp->clipdist;
        sp->clipdist = 1;

        np->ang = NORM_ANGLE(np->ang + lat_ang[i]);
        HelpMissileLateral(New, 4200L);
        np->ang = NORM_ANGLE(np->ang - lat_ang[i]);

        // find the distance to the target (player)
        dist = Distance(np->x, np->y, u->tgt_sp->x, u->tgt_sp->y);
        if (dist != 0)
            np->zvel = (np->xvel * (SPRITEp_UPPER(u->tgt_sp) - np->z)) / dist;

        np->ang = NORM_ANGLE(np->ang + delta_ang[i]);

        nu->xchange = MOVEx(np->xvel, np->ang);
        nu->ychange = MOVEy(np->xvel, np->ang);
        nu->zchange = np->zvel;

        MissileSetPos(New, DoMirvMissile, 400);
        sp->clipdist = oclipdist;

        if (TEST(u->Flags, SPR_UNDERWATER))
            SET(nu->Flags, SPR_UNDERWATER);
    }

    return 0;
}

int
SpawnDemonFist(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    SPRITEp exp;
    USERp eu;
    short explosion;

    ASSERT(u);
    if (TEST(u->Flags, SPR_SUICIDE))
        return -1;

    //PlaySound(DIGI_ITEM_SPAWN, &sp->x, &sp->y, &sp->z, v3df_none);
    explosion = SpawnSprite(STAT_MISSILE, 0, s_TeleportEffect, sp->sectnum,
                            sp->x, sp->y, SPRITEp_MID(sp), sp->ang, 0);

    exp = &sprite[explosion];
    eu = User[explosion];

    exp->hitag = LUMINOUS; //Always full brightness
    exp->owner = -1;
    exp->shade = -40;
    exp->xrepeat = 32;
    exp->yrepeat = 32;
    eu->spal = exp->pal = 25;

    SET(exp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(exp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    eu->Radius = DamageData[DMG_BASIC_EXP].radius;

    if (RANDOM_P2(1024<<8)>>8 > 600)
        SET(exp->cstat, CSTAT_SPRITE_XFLIP);
    if (RANDOM_P2(1024<<8)>>8 > 600)
        SET(exp->cstat, CSTAT_SPRITE_YFLIP);

    return explosion;
}

int
InitSerpMonstSpell(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], np;
    USERp u = User[SpriteNum], nu;
    int dist;
    short New, save_ang, i;
    short oclipdist;

    static short lat_ang[] =
    {
        512, -512
    };

    static short delta_ang[] =
    {
        -10, 10
    };

    PlaySound(DIGI_MISSLFIRE, &sp->x, &sp->y, &sp->z, v3df_none);

    for (i = 0; i < 1; i++)
    {
        sp->ang = getangle(u->tgt_sp->x - sp->x, u->tgt_sp->y - sp->y);

        New = SpawnSprite(STAT_MISSILE, SERP_METEOR, &sg_SerpMeteor[0][0], sp->sectnum,
                          sp->x, sp->y, sp->z, sp->ang, 500);

        np = &sprite[New];
        nu = User[New];

        nu->spal = np->pal = 25; // Bright Red
        np->z = SPRITEp_TOS(sp);

        nu->RotNum = 5;
        NewStateGroup(New, &sg_SerpMeteor[0]);
        //nu->StateEnd = s_MirvMeteorExp;
        nu->StateEnd = s_TeleportEffect2;

        SetOwner(SpriteNum, New);
        np->shade = -40;
        np->xrepeat = 122;
        np->yrepeat = 116;
        np->clipdist = 32L >> 2;
        np->zvel = 0;
        SET(np->cstat, CSTAT_SPRITE_YCENTER);

        nu->ceiling_dist = Z(16);
        nu->floor_dist = Z(16);

        nu->Dist = 200;

        oclipdist = sp->clipdist;
        sp->clipdist = 1;

        np->ang = NORM_ANGLE(np->ang + lat_ang[i]);
        HelpMissileLateral(New, 4200L);
        np->ang = NORM_ANGLE(np->ang - lat_ang[i]);

        // find the distance to the target (player)
        dist = Distance(np->x, np->y, u->tgt_sp->x, u->tgt_sp->y);
        if (dist != 0)
            np->zvel = (np->xvel * (SPRITEp_UPPER(u->tgt_sp) - np->z)) / dist;

        np->ang = NORM_ANGLE(np->ang + delta_ang[i]);

        nu->xchange = MOVEx(np->xvel, np->ang);
        nu->ychange = MOVEy(np->xvel, np->ang);
        nu->zchange = np->zvel;

        MissileSetPos(New, DoMirvMissile, 400);
        sp->clipdist = oclipdist;

        if (TEST(u->Flags, SPR_UNDERWATER))
            SET(nu->Flags, SPR_UNDERWATER);
    }

    return 0;
}

int
DoTeleRipper(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    extern void Ripper2Hatch(short Weapon);

    PlaySound(DIGI_ITEM_SPAWN,&sp->x,&sp->y,&sp->z,v3df_none);
    Ripper2Hatch(SpriteNum);

    return 0;
}


int
InitEnemyRocket(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    int nx, ny, nz, dist, nang;
    short w;


    PlaySound(DIGI_NINJARIOTATTACK, &sp->x, &sp->y, &sp->z, v3df_none);

    // get angle to player and also face player when attacking
    sp->ang = nang = getangle(u->tgt_sp->x - sp->x, u->tgt_sp->y - sp->y);

    nx = sp->x;
    ny = sp->y;
    nz = sp->z - DIV2(SPRITEp_SIZE_Z(sp))-Z(8);

    // Spawn a shot
    // wp = &sprite[w = SpawnSprite(STAT_MISSILE, STAR1, s_Star, sp->sectnum,
    // nx, ny, nz, u->tgt_sp->ang, 250)];
    w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R2, &s_Rocket[0][0], sp->sectnum,
                    nx, ny, nz-Z(8), u->tgt_sp->ang, NINJA_BOLT_VELOCITY);
    wp = &sprite[w];
    wu = User[w];

    // Set default palette
    wp->pal = wu->spal = 17; // White

    if (u->ID == ZOMBIE_RUN_R0)
        SetOwner(sp->owner, w);
    else
        SetOwner(SpriteNum, w);
    wp->yrepeat = 28;
    wp->xrepeat = 28;
    wp->shade = -15;
    wp->zvel = 0;
    wp->ang = nang;
    wp->clipdist = 64L>>2;

    wu->RotNum = 5;
    NewStateGroup(w, &sg_Rocket[0]);
    wu->Radius = 200;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    if (u->spal == PAL_XLAT_LT_TAN)
    {
        SET(wu->Flags, SPR_FIND_PLAYER);
        wp->pal = wu->spal = 20; // Yellow
    }

    MissileSetPos(w, DoBoltThinMan, 400);

    // find the distance to the target (player)
    dist = Distance(wp->x, wp->y, u->tgt_sp->x, u->tgt_sp->y);

    if (dist != 0)
        wu->zchange = wp->zvel = (wp->xvel * (SPRITEp_UPPER(u->tgt_sp) - wp->z)) / dist;

    return w;
}

int
InitEnemyRail(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz, dist, nang;
    short w;
    short oclipdist,pnum=0;

    if (SW_SHAREWARE) return FALSE; // JBF: verify

    // if co-op don't hurt teammate
    if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE && u->ID == ZOMBIE_RUN_R0)
    {
        PLAYERp pp;
        SPRITEp psp;

        // Check all players
        TRAVERSE_CONNECT(pnum)
        {
            pp = &Player[pnum];
            psp = &sprite[pp->PlayerSprite];

            if (u->tgt_sp == psp)
                return 0;
        }
    }

    PlaySound(DIGI_RAILFIRE, &sp->x, &sp->y, &sp->z, v3df_dontpan|v3df_doppler);

    // get angle to player and also face player when attacking
    sp->ang = nang = getangle(u->tgt_sp->x - sp->x, u->tgt_sp->y - sp->y);

    // add a bit of randomness
    if (RANDOM_P2(1024) < 512)
        sp->ang = NORM_ANGLE(sp->ang + RANDOM_P2(128) - 64);

    nx = sp->x;
    ny = sp->y;
    nz = sp->z - DIV2(SPRITEp_SIZE_Z(sp))-Z(8);

    // Spawn a shot
    // Inserting and setting up variables

    w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R1, &s_Rail[0][0], sp->sectnum,
                    nx, ny, nz, sp->ang, 1200);

    wp = &sprite[w];
    wu = User[w];

    if (u->ID == ZOMBIE_RUN_R0)
        SetOwner(sp->owner, w);
    else
        SetOwner(SpriteNum, w);

    wp->yrepeat = 52;
    wp->xrepeat = 52;
    wp->shade = -15;
    wp->zvel = 0;

    wu->RotNum = 5;
    NewStateGroup(w, &sg_Rail[0]);

    wu->Radius = 200;
    wu->ceiling_dist = Z(1);
    wu->floor_dist = Z(1);
    SET(wu->Flags2, SPR2_SO_MISSILE);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER|CSTAT_SPRITE_INVISIBLE);
    SET(wp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    wp->clipdist = 64L>>2;

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    if (TestMissileSetPos(w, DoRailStart, 600, wp->zvel))
    {
        //sprite->clipdist = oclipdist;
        KillSprite(w);
        return 0;
    }

    // find the distance to the target (player)
    dist = Distance(wp->x, wp->y, u->tgt_sp->x, u->tgt_sp->y);

    if (dist != 0)
        wu->zchange = wp->zvel = (wp->xvel * (SPRITEp_UPPER(u->tgt_sp) - wp->z)) / dist;

    return w;
}


int
InitZillaRocket(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    int nx, ny, nz, dist, nang;
    short w, i;

    typedef struct
    {
        int dist_over, dist_out;
        short ang;
    } MISSILE_PLACEMENT;

    static MISSILE_PLACEMENT mp[] =
    {
        {600 * 6, 400, 512},
        {900 * 6, 400, 512},
        {1100 * 6, 400, 512},
        {600 * 6, 400, -512},
        {900 * 6, 400, -512},
        {1100 * 6, 400, -512},
    };

    PlaySound(DIGI_NINJARIOTATTACK, &sp->x, &sp->y, &sp->z, v3df_none);

    // get angle to player and also face player when attacking
    sp->ang = nang = getangle(u->tgt_sp->x - sp->x, u->tgt_sp->y - sp->y);

    for (i = 0; i < (int)SIZ(mp); i++)
    {
        nx = sp->x;
        ny = sp->y;
        nz = sp->z - DIV2(SPRITEp_SIZE_Z(sp))-Z(8);

        // Spawn a shot
        // wp = &sprite[w = SpawnSprite(STAT_MISSILE, STAR1, s_Star, sp->sectnum,
        // nx, ny, nz, u->tgt_sp->ang, 250)];
        w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R2, &s_Rocket[0][0], sp->sectnum,
                        nx, ny, nz-Z(8), u->tgt_sp->ang, NINJA_BOLT_VELOCITY);
        wp = &sprite[w];
        wu = User[w];

        SetOwner(SpriteNum, w);
        wp->yrepeat = 28;
        wp->xrepeat = 28;
        wp->shade = -15;
        wp->zvel = 0;
        wp->ang = nang;
        wp->clipdist = 64L>>2;

        wu->RotNum = 5;
        NewStateGroup(w, &sg_Rocket[0]);
        wu->Radius = 200;
        SET(wp->cstat, CSTAT_SPRITE_YCENTER);

        wu->xchange = MOVEx(wp->xvel, wp->ang);
        wu->ychange = MOVEy(wp->xvel, wp->ang);
        wu->zchange = wp->zvel;

        // Zilla has seekers!
        if (i != 1 && i != 4)
            wp->pal = wu->spal = 17; // White
        else
        {
            SET(wu->Flags, SPR_FIND_PLAYER);
            wp->pal = wu->spal = 20; // Yellow
        }

        if (mp[i].dist_over != 0)
        {
            wp->ang = NORM_ANGLE(wp->ang + mp[i].ang);
            HelpMissileLateral(w, mp[i].dist_over);
            wp->ang = NORM_ANGLE(wp->ang - mp[i].ang);
        }

        MissileSetPos(w, DoBoltThinMan, mp[i].dist_out);

        // find the distance to the target (player)
        dist = Distance(wp->x, wp->y, u->tgt_sp->x, u->tgt_sp->y);

        if (dist != 0)
            wu->zchange = wp->zvel = (wp->xvel * (SPRITEp_UPPER(u->tgt_sp) - wp->z)) / dist;
    }

    return w;
}

int
InitEnemyStar(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    int nx, ny, nz, dist, nang;
    short w;

    // get angle to player and also face player when attacking
    sp->ang = nang = NORM_ANGLE(getangle(u->tgt_sp->x - sp->x, u->tgt_sp->y - sp->y));

    nx = sp->x;
    ny = sp->y;
    nz = SPRITEp_MID(sp);

    // Spawn a shot
    wp = &sprite[w = SpawnSprite(STAT_MISSILE, STAR1, s_Star, sp->sectnum,
                                 nx, ny, nz, u->tgt_sp->ang, NINJA_STAR_VELOCITY)];

    wu = User[w];

    //wp->owner = SpriteNum;
    SetOwner(SpriteNum, w);
    wp->yrepeat = 16;
    wp->xrepeat = 16;
    wp->shade = -25;
    wp->zvel = 0;
    wp->ang = nang;
    wp->clipdist = 64L>>2;

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    MissileSetPos(w, DoStar, 400);

    // find the distance to the target (player)
    dist = Distance(wp->x, wp->y, u->tgt_sp->x, u->tgt_sp->y);

    if (dist != 0)
        wu->zchange = wp->zvel = (wp->xvel * (SPRITEp_UPPER(u->tgt_sp) - wp->z)) / dist;

    //
    // Star Power Up Code
    //

#if 0
    if (sp->pal == PALETTE_PLAYER0)
    {
        static short dang[] = {-28, 28};
        char i;
        SPRITEp np;
        USERp nu;
        short sn;

        PlaySound(DIGI_STAR, &sp->x, &sp->y, &sp->z, v3df_none);
        for (i = 0; i < SIZ(dang); i++)
        {
            sn = SpawnSprite(STAT_MISSILE, STAR1, s_Star, sp->sectnum, wp->x, wp->y, wp->z, NORM_ANGLE(wp->ang + dang[i]), wp->xvel);
            np = &sprite[sn];
            nu = User[sn];

            SetOwner(wp->owner, sn);
            np->yrepeat = wp->yrepeat;
            np->xrepeat = wp->xrepeat;
            np->shade = wp->shade;
            nu->WeaponNum = wu->WeaponNum;
            nu->Radius = wu->Radius;
            nu->xchange = wu->xchange;
            nu->ychange = wu->ychange;
            nu->zchange = 0;
            np->zvel = 0;

            MissileSetPos(sn, DoStar, 400);

            nu->zchange = wu->zchange;
            np->zvel = wp->zvel;
        }
    }
    else
#endif

    PlaySound(DIGI_STAR, &sp->x, &sp->y, &sp->z, v3df_none);



    return w;
}

int
InitEnemyCrossbow(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    int nx, ny, nz, dist, nang;
    short w;

    // get angle to player and also face player when attacking
    sp->ang = nang = NORM_ANGLE(getangle(u->tgt_sp->x - sp->x, u->tgt_sp->y - sp->y));

    nx = sp->x;
    ny = sp->y;
    nz = SPRITEp_MID(sp)-Z(14);

    // Spawn a shot
    wp = &sprite[w = SpawnSprite(STAT_MISSILE, CROSSBOLT, &s_CrossBolt[0][0], sp->sectnum,
                                 nx, ny, nz, u->tgt_sp->ang, 800)];

    wu = User[w];

    //wp->owner = SpriteNum;
    SetOwner(SpriteNum, w);
    wp->xrepeat = 16;
    wp->yrepeat = 26;
    wp->shade = -25;
    wp->zvel = 0;
    wp->ang = nang;
    wp->clipdist = 64L>>2;

    wu->RotNum = 5;
    NewStateGroup(w, &sg_CrossBolt[0]);

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    SET(wu->Flags, SPR_XFLIP_TOGGLE);

    MissileSetPos(w, DoStar, 400);

    // find the distance to the target (player)
    dist = Distance(wp->x, wp->y, u->tgt_sp->x, u->tgt_sp->y);

    if (dist != 0)
        wu->zchange = wp->zvel = (wp->xvel * (SPRITEp_UPPER(u->tgt_sp) - wp->z)) / dist;

    //
    // Star Power Up Code
    //

#if 0
    if (sp->pal == PALETTE_PLAYER0)
    {
        static short dang[] = {-28, 28};
        char i;
        SPRITEp np;
        USERp nu;
        short sn;

        PlaySound(DIGI_STAR, &sp->x, &sp->y, &sp->z, v3df_none);
        for (i = 0; i < SIZ(dang); i++)
        {
            sn = SpawnSprite(STAT_MISSILE, CROSSBOLT, s_CrossBolt, sp->sectnum, wp->x, wp->y, wp->z, NORM_ANGLE(wp->ang + dang[i]), wp->xvel);
            np = &sprite[sn];
            nu = User[sn];

            SetOwner(wp->owner, sn);
            np->yrepeat = wp->yrepeat;
            np->xrepeat = wp->xrepeat;
            np->shade = wp->shade;
            nu->WeaponNum = wu->WeaponNum;
            nu->Radius = wu->Radius;
            nu->xchange = wu->xchange;
            nu->ychange = wu->ychange;
            nu->zchange = 0;
            np->zvel = 0;

            MissileSetPos(sn, DoStar, 400);

            nu->zchange = wu->zchange;
            np->zvel = wp->zvel;
        }
    }
    else
#endif

    PlaySound(DIGI_STAR, &sp->x, &sp->y, &sp->z, v3df_none);



    return w;
}


int
InitSkelSpell(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    int nx, ny, nz, dist, nang;
    short w;

    PlaySound(DIGI_SPELEC, &sp->x, &sp->y, &sp->z, v3df_none);

    // get angle to player and also face player when attacking
    sp->ang = nang = NORM_ANGLE(getangle(u->tgt_sp->x - sp->x, u->tgt_sp->y - sp->y));

    nx = sp->x;
    ny = sp->y;
    nz = sp->z - DIV2(SPRITEp_SIZE_Z(sp));

    // Spawn a shot
    w = SpawnSprite(STAT_MISSILE, ELECTRO_ENEMY, s_Electro, sp->sectnum,
                    nx, ny, nz, u->tgt_sp->ang, SKEL_ELECTRO_VELOCITY);
    wp = &sprite[w];
    wu = User[w];

    //wp->owner = SpriteNum;
    SetOwner(SpriteNum, w);
    wp->xrepeat -= 20;
    wp->yrepeat -= 20;
    wp->shade = -40;
    wp->zvel = 0;
    wp->ang = nang;
    wp->clipdist = 64L>>2;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);

    // find the distance to the target (player)
    dist = Distance(nx, ny, u->tgt_sp->x, u->tgt_sp->y);

    if (dist != 0)
        wp->zvel = (wp->xvel * (SPRITEp_UPPER(u->tgt_sp) - nz)) / dist;

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    MissileSetPos(w, DoElectro, 400);

    return w;
}


int
InitCoolgFire(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    int nx, ny, nz, dist, nang;
    short w;

    // get angle to player and also face player when attacking
    sp->ang = nang = NORM_ANGLE(getangle(u->tgt_sp->x - sp->x, u->tgt_sp->y - sp->y));

    nx = sp->x;
    ny = sp->y;

    nz = sp->z - Z(16);

    // Spawn a shot
    // Inserting and setting up variables

    PlaySound(DIGI_CGMAGIC, &sp->x, &sp->y, &sp->z, v3df_follow);

    w = SpawnSprite(STAT_MISSILE, COOLG_FIRE, s_CoolgFire, sp->sectnum,
                    nx, ny, nz, u->tgt_sp->ang, COOLG_FIRE_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    //wp->owner = SpriteNum;
    SetOwner(SpriteNum, w);
    wp->hitag = LUMINOUS;
    wp->yrepeat = 18;
    wp->xrepeat = 18;
    wp->shade = -40;
    wp->zvel = 0;
    wp->ang = nang;
    wp->clipdist = 32L>>2;
    wu->ceiling_dist = Z(4);
    wu->floor_dist = Z(4);
    if (u->ID == RIPPER_RUN_R0)
        wu->spal = wp->pal = 27; // Bright Green
    else
        wu->spal = wp->pal = 25; // Bright Red

    PlaySound(DIGI_MAGIC1, &wp->x, &wp->y, &wp->z, v3df_follow|v3df_doppler);

    // find the distance to the target (player)
    dist = Distance(nx, ny, u->tgt_sp->x, u->tgt_sp->y);

    if (dist != 0)
        // (velocity * difference between the target and the throwing star) /
        // distance
        wp->zvel = (wp->xvel * (SPRITEp_UPPER(u->tgt_sp) - nz)) / dist;

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    nx = ((int) 728 * (int) sintable[NORM_ANGLE(nang + 512)]) >> 14;
    ny = ((int) 728 * (int) sintable[nang]) >> 14;

    move_missile(w, nx, ny, 0L, wu->ceiling_dist, wu->floor_dist, 0, 3L);

    return w;
}

int DoCoolgDrip(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    u->Counter += 220;
    sp->z += u->Counter;

    if (sp->z > u->loz - u->floor_dist)
    {
        sp->z = u->loz - u->floor_dist;
        sp->yrepeat = sp->xrepeat = 32;
        ChangeState(SpriteNum, s_GoreFloorSplash);
        if (u->spal == PALETTE_BLUE_LIGHTING)
            PlaySound(DIGI_DRIP, &sp->x, &sp->y, &sp->z, v3df_none);
    }
    return 0;
}

int
InitCoolgDrip(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    int nx, ny, nz, dist, nang;
    short w;

    nx = sp->x;
    ny = sp->y;
    nz = sp->z;

    w = SpawnSprite(STAT_MISSILE, COOLG_DRIP, s_CoolgDrip, sp->sectnum,
                    nx, ny, nz, sp->ang, 0);

    wp = &sprite[w];
    wu = User[w];

    //wp->owner = SpriteNum;
    SetOwner(SpriteNum, w);
    wp->yrepeat = wp->xrepeat = 20;
    wp->shade = -5;
    wp->zvel = 0;
    wp->clipdist = 16L>>2;
    wu->ceiling_dist = Z(4);
    wu->floor_dist = Z(4);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);

    DoFindGroundPoint(SpriteNum);

    return w;
}

int
GenerateDrips(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    int nx, ny, nz, dist, nang;
    short w = 0;

    if ((u->WaitTics-=ACTORMOVETICS) <= 0)
    {
        if (sp->lotag == 0)
            u->WaitTics = RANDOM_P2(256<<8)>>8;
        else
            u->WaitTics = (sp->lotag * 120) + SEC(RANDOM_RANGE(3<<8)>>8);

        if (TEST_BOOL2(sp))
        {
            w = SpawnBubble(SpriteNum);
            return w;
        }

        nx = sp->x;
        ny = sp->y;
        nz = sp->z;

        w = SpawnSprite(STAT_SHRAP, COOLG_DRIP, s_CoolgDrip, sp->sectnum,
                        nx, ny, nz, sp->ang, 0);

        wp = &sprite[w];
        wu = User[w];

        //wp->owner = SpriteNum;
        SetOwner(SpriteNum, w);
        wp->yrepeat = wp->xrepeat = 20;
        wp->shade = -10;
        wp->zvel = 0;
        wp->clipdist = 16L>>2;
        wu->ceiling_dist = Z(4);
        wu->floor_dist = Z(4);
        SET(wp->cstat, CSTAT_SPRITE_YCENTER);
        if (TEST_BOOL1(sp))
            wu->spal = wp->pal = PALETTE_BLUE_LIGHTING;

        DoFindGroundPoint(SpriteNum);
    }
    return w;
}

int
InitEelFire(short SpriteNum)
{
    USERp u = User[SpriteNum], hu;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SPRITEp hp;
    short i, nexti;
    unsigned stat;
    int dist, a, b, c;

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[StatDamageList[stat]], i, nexti)
        {
            hp = &sprite[i];
            hu = User[i];

            if (i == SpriteNum)
                continue;

            if (hp != u->tgt_sp)
                continue;

            if ((unsigned)FindDistance3D(sp->x - hp->x, sp->y - hp->y, (sp->z - hp->z)>>4) > hu->Radius + u->Radius)
                continue;

            DISTANCE(hp->x, hp->y, sp->x, sp->y, dist, a, b, c);

            if (dist < CLOSE_RANGE_DIST_FUDGE(sp, hp, 600) && FACING_RANGE(hp, sp, 150))
            {
                PlaySound(DIGI_GIBS1, &sp->x, &sp->y, &sp->z, v3df_none);
                DoDamage(i, SpriteNum);
            }
            else
                InitActorReposition(SpriteNum);
        }
    }

    return 0;
}

int
InitFireballTrap(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    int nx, ny, nz, dist;
    short w;

    PlaySound(DIGI_FIREBALL1, &sp->x, &sp->y, &sp->z, v3df_none);

    nx = sp->x;
    ny = sp->y;
    nz = sp->z - SPRITEp_SIZE_Z(sp);

    // Spawn a shot
    w = SpawnSprite(STAT_MISSILE, FIREBALL, s_Fireball, sp->sectnum, nx, ny, nz,
                    sp->ang, FIREBALL_TRAP_VELOCITY);
    wp = &sprite[w];
    wu = User[w];

    //wp->owner = SpriteNum;
    wp->hitag = LUMINOUS; //Always full brightness
    SetOwner(SpriteNum, w);
    wp->xrepeat -= 20;
    wp->yrepeat -= 20;
    wp->shade = -40;
    wp->clipdist = 32>>2;
    wp->zvel = 0;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    wu->WeaponNum = WPN_HOTHEAD;

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    return w;
}

int
InitBoltTrap(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    int nx, ny, nz, dist;
    short w;

    PlaySound(DIGI_RIOTFIRE, &sp->x, &sp->y, &sp->z, v3df_none);

    nx = sp->x;
    ny = sp->y;
    nz = sp->z - SPRITEp_SIZE_Z(sp);

    // Spawn a shot
    w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R0, &s_Rocket[0][0], sp->sectnum, nx, ny, nz,
                    sp->ang, BOLT_TRAP_VELOCITY);
    wp = &sprite[w];
    wu = User[w];

    //wp->owner = SpriteNum;
    SetOwner(SpriteNum, w);
    wp->yrepeat = 32;
    wp->xrepeat = 32;
    wp->shade = -15;
    wp->zvel = 0;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);

    wu->RotNum = 5;
    NewStateGroup(w, &sg_Rocket[0]);
    wu->Radius = 200;

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    return w;
}

#if 0
int
InitEnemyCrossbow(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    int nx, ny, nz, dist, nang;
    short w;

    // get angle to player and also face player when attacking
    sp->ang = nang = NORM_ANGLE(getangle(u->tgt_sp->x - sp->x, u->tgt_sp->y - sp->y));

    nx = sp->x;
    ny = sp->y;
    nz = SPRITEp_MID(sp);

    // Spawn a shot
    wp = &sprite[w = SpawnSprite(STAT_MISSILE, CROSSBOLT, s_CrossBolt, sp->sectnum,
                                 nx, ny, nz, u->tgt_sp->ang, 800)];

    wu = User[w];

    //wp->owner = SpriteNum;
    SetOwner(SpriteNum, w);
    wp->xrepeat = 16;
    wp->yrepeat = 26;
    wp->shade = -25;
    wp->zvel = 0;
    wp->ang = nang;
    wp->clipdist = 64L>>2;

    wu->RotNum = 5;
    NewStateGroup(w, &sg_CrossBolt);

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    SET(wu->Flags, SPR_XFLIP_TOGGLE);

    MissileSetPos(w, DoStar, 400);

    // find the distance to the target (player)
    dist = Distance(wp->x, wp->y, u->tgt_sp->x, u->tgt_sp->y);

    if (dist != 0)
        wu->zchange = wp->zvel = (wp->xvel * (SPRITEp_UPPER(u->tgt_sp) - wp->z)) / dist;

    PlaySound(DIGI_STAR, &sp->x, &sp->y, &sp->z, v3df_none);

    return w;
}
#endif

int
InitSpearTrap(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], wp;
    USERp u = User[SpriteNum], wu;
    int nx, ny, nz, dist;
    short w;
    //short nang;

    nx = sp->x;
    ny = sp->y;
    nz = SPRITEp_MID(sp);

    // Spawn a shot
    w = SpawnSprite(STAT_MISSILE, CROSSBOLT, &s_CrossBolt[0][0], sp->sectnum, nx, ny, nz, sp->ang, 750);

    wp = &sprite[w];
    wu = User[w];

    //wp->owner = SpriteNum;
    SetOwner(SpriteNum, w);
    wp->xrepeat = 16;
    wp->yrepeat = 26;
    wp->shade = -25;
    //wp->zvel = 0;
    //wp->ang = nang;
    wp->clipdist = 64L>>2;

    wu->RotNum = 5;
    NewStateGroup(w, &sg_CrossBolt[0]);

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    SET(wu->Flags, SPR_XFLIP_TOGGLE);

    //MissileSetPos(w, DoStar, 400);

    // find the distance to the target (player)
    //dist = Distance(wp->x, wp->y, u->tgt_sp->x, u->tgt_sp->y);

    //if (dist != 0)
    //wu->zchange = wp->zvel = (wp->xvel * (SPRITEp_UPPER(u->tgt_sp) - wp->z)) / dist;

    PlaySound(DIGI_STAR, &sp->x, &sp->y, &sp->z, v3df_none);
    return w;
}

int
DoSuicide(short SpriteNum)
{
    KillSprite(SpriteNum);
    return 0;
}

int
DoDefaultStat(short SpriteNum)
{
    change_sprite_stat(SpriteNum, STAT_DEFAULT);
    return 0;
}


int
InitTracerUzi(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    SPRITEp wp, hsp;
    USERp wu;

    int nx, ny, nz, dist, nang;
    short w;
    int oclipdist;

    short lat_dist[] = {800,-800};

    nx = pp->posx;
    ny = pp->posy;
    //nz = pp->posz + pp->bob_z + Z(8);
    //nz = pp->posz + pp->bob_z + Z(8) + ((100 - pp->horiz) * 72);
    nz = pp->posz + Z(8) + ((100 - pp->horiz) * 72);

    // Spawn a shot
    // Inserting and setting up variables
    w = SpawnSprite(STAT_MISSILE, 0, s_Tracer, pp->cursectnum,
                    nx, ny, nz, pp->pang, TRACER_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    wp->hitag = LUMINOUS; //Always full brightness
    //wp->owner = pp->PlayerSprite;
    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = 10;
    wp->xrepeat = 10;
    wp->shade = -40;
    wp->zvel = 0;
    //wp->zvel = ((100 - pp->horiz) * HORIZ_MULT);
    wp->clipdist = 32 >> 2;

    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 50;
    wu->ceiling_dist = Z(3);
    wu->floor_dist = Z(3);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, CSTAT_SPRITE_INVISIBLE);

    oclipdist = pp->SpriteP->clipdist;
    pp->SpriteP->clipdist = 0;

    wp->ang = NORM_ANGLE(wp->ang + 512);
    if (TEST(pp->Flags, PF_TWO_UZI) && pp->WpnUziType == 0)
        HelpMissileLateral(w, lat_dist[RANDOM_P2(2<<8)>>8]);
    else
        HelpMissileLateral(w, lat_dist[0]);
    wp->ang = NORM_ANGLE(wp->ang - 512);

    if (MissileSetPos(w, DoTracerStart, 800))
    {
        pp->SpriteP->clipdist = oclipdist;
        KillSprite(w);
        return 0;
    }

    wp->zvel = ((100 - pp->horiz) * (wp->xvel/8));

    pp->SpriteP->clipdist = oclipdist;

    WeaponAutoAim(pp->SpriteP, w, 32, FALSE);

    // a bit of randomness
    wp->ang = NORM_ANGLE(wp->ang + RANDOM_RANGE(30) - 15);

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    return 0;
}

int
InitTracerTurret(short SpriteNum, short Operator, int horiz)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SPRITEp wp, hsp;
    USERp wu;

    int nx, ny, nz, dist, nang;
    short w;
    int oclipdist;

    nx = sp->x;
    ny = sp->y;
    nz = sp->z + ((100 - horiz) * 72);

    // Spawn a shot
    // Inserting and setting up variables

    w = SpawnSprite(STAT_MISSILE, 0, s_Tracer, sp->sectnum,
                    nx, ny, nz, sp->ang, TRACER_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    wp->hitag = LUMINOUS; //Always full brightness
    if (Operator >= 0)
        SetOwner(Operator, w);
    wp->yrepeat = 10;
    wp->xrepeat = 10;
    wp->shade = -40;
    wp->zvel = 0;
    wp->clipdist = 8 >> 2;

    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 50;
    wu->ceiling_dist = Z(1);
    wu->floor_dist = Z(1);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, CSTAT_SPRITE_INVISIBLE);

    wp->zvel = ((100 - horiz) * (wp->xvel/8));

    WeaponAutoAim(sp, w, 32, FALSE);

    // a bit of randomness
    wp->ang = NORM_ANGLE(wp->ang + RANDOM_RANGE(30) - 15);

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    if (SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    return 0;
}

#if 1
int
InitTracerAutoTurret(short SpriteNum, short Operator, int xchange, int ychange, int zchange)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SPRITEp wp, hsp;
    USERp wu;

    int nx, ny, nz, dist, nang;
    short w;
    int oclipdist;

    nx = sp->x;
    ny = sp->y;
    nz = sp->z;

    // Spawn a shot
    // Inserting and setting up variables

    w = SpawnSprite(STAT_MISSILE, 0, s_Tracer, sp->sectnum,
                    nx, ny, nz, sp->ang, TRACER_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    wp->hitag = LUMINOUS; //Always full brightness
    if (Operator >= 0)
        SetOwner(Operator, w);
    wp->yrepeat = 10;
    wp->xrepeat = 10;
    wp->shade = -40;
    wp->zvel = 0;
    wp->clipdist = 8 >> 2;

    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 50;
    wu->ceiling_dist = Z(1);
    wu->floor_dist = Z(1);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, CSTAT_SPRITE_INVISIBLE);

    wu->xchange = xchange;
    wu->ychange = ychange;
    wu->zchange = zchange;

    if (SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    return 0;
}
#endif

int
BulletHitSprite(SPRITEp sp, short hit_sprite, short hit_sect, short hit_wall, int hit_x, int hit_y, int hit_z, short ID)
{
    vec3_t hit_pos = { hit_x, hit_y, hit_z };
    SPRITEp hsp = &sprite[hit_sprite];
    USERp hu = User[hit_sprite];
    SPRITEp wp;
    USERp wu;
    short New;
    short id;

    // hit a NPC or PC?
    if (TEST(hsp->extra, SPRX_PLAYER_OR_ENEMY))
    {
        // spawn a red splotch
        // !FRANK! this if was incorrect - its not who is HIT, its who is SHOOTING
        //if(!hu->PlayerP)
        if (User[sp - sprite]->PlayerP)
            id = UZI_SMOKE;
        else if (TEST(User[sp - sprite]->Flags, SPR_SO_ATTACHED))
            id = UZI_SMOKE;
        else // Spawn NPC uzi with less damage
            id = UZI_SMOKE+2;

        if (ID>0) id = ID;

        New = SpawnSprite(STAT_MISSILE, id, s_UziSmoke, 0, hit_x, hit_y, hit_z, sp->ang, 0);
        wp = &sprite[New];
        wu = User[New];
        wp->shade = -40;

        if (hu->PlayerP)
            SET(wp->cstat, CSTAT_SPRITE_INVISIBLE);

        switch (hu->ID)
        {
        case TRASHCAN:
        case PACHINKO1:
        case PACHINKO2:
        case PACHINKO3:
        case PACHINKO4:
        case 623:
        case ZILLA_RUN_R0:
            wp->xrepeat = UZI_SMOKE_REPEAT;
            wp->yrepeat = UZI_SMOKE_REPEAT;
            if (RANDOM_P2(1024) > 800)
                SpawnShrapX(hit_sprite);
            break;
        default:
            wp->xrepeat = UZI_SMOKE_REPEAT/3;
            wp->yrepeat = UZI_SMOKE_REPEAT/3;
            SET(wp->cstat, CSTAT_SPRITE_INVISIBLE);
            //wu->spal = wp->pal = PALETTE_RED_LIGHTING;
            break;
        }

        SetOwner(sp - sprite, New);
        wp->ang = sp->ang;

        setspritez(New, &hit_pos);
        SET(wp->cstat, CSTAT_SPRITE_YCENTER);
        RESET(wp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

        if ((RANDOM_P2(1024<<5)>>5) < 512+128)
        {
            if (!hu->PlayerP)
                SpawnBlood(hit_sprite, -1, sp->ang, hit_x, hit_y, hit_z);
            else
                SpawnBlood(hit_sprite, -1, sp->ang, hit_x, hit_y, hit_z+Z(20));

            // blood comes out the other side?
            if ((RANDOM_P2(1024<<5)>>5) < 256)
            {
                if (!hu->PlayerP)
                    SpawnBlood(hit_sprite, -1, NORM_ANGLE(sp->ang+1024),hit_x, hit_y, hit_z);
                if (hu->ID != TRASHCAN && hu->ID != ZILLA_RUN_R0)
                    QueueWallBlood(hit_sprite, sp->ang);  //QueueWallBlood needs bullet angle.
            }
        }

        DoHitscanDamage(New, hit_sprite);

        return TRUE;
    }

    return FALSE;
}

int SpawnWallHole(short hit_sect, short hit_wall, int hit_x, int hit_y, int hit_z)
{
    short w,nw,wall_ang,dang;
    short SpriteNum;
    int nx,ny;
    SPRITEp sp;

    SpriteNum = COVERinsertsprite(hit_sect, STAT_DEFAULT);
    sp = &sprite[SpriteNum];
    sp->owner = -1;
    sp->xrepeat = sp->yrepeat = 16;
    sp->cstat = sp->pal = sp->shade = sp->extra = sp->clipdist = sp->xoffset = sp->yoffset = 0;
    sp->x = hit_x;
    sp->y = hit_y;
    sp->z = hit_z;
    sp->picnum = 2151;

    //SET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT|CSTAT_SPRITE_WALL);
    SET(sp->cstat, CSTAT_SPRITE_WALL);
    SET(sp->cstat, CSTAT_SPRITE_ONE_SIDE);

    w = hit_wall;
    nw = wall[w].point2;
    wall_ang = NORM_ANGLE(getangle(wall[nw].x - wall[w].x, wall[nw].y - wall[w].y)-512);

    sp->ang = NORM_ANGLE(wall_ang + 1024);

    //nx = (sintable[(512 + Player[0].pang) & 2047] >> 7);
    //ny = (sintable[Player[0].pang] >> 7);
    //sp->x -= nx;
    //sp->y -= ny;

    return SpriteNum;
}

SWBOOL
HitscanSpriteAdjust(short SpriteNum, short hit_wall)
{
    SPRITEp sp = &sprite[SpriteNum];
    short w, nw, ang = sp->ang, wall_ang;
    int xvect,yvect;
    short sectnum;

#if 1
    w = hit_wall;
    nw = wall[w].point2;
    wall_ang = NORM_ANGLE(getangle(wall[nw].x - wall[w].x, wall[nw].y - wall[w].y));

    if (hit_wall >= 0)
        ang = sp->ang = NORM_ANGLE(wall_ang + 512);
    else
        ang = sp->ang;
#endif

#if 0
    xvect = (sintable[(512 + ang) & 2047] >> 7);
    yvect = (sintable[ang] >> 7);
    move_missile(SpriteNum, xvect, yvect, 0L, CEILING_DIST, FLOOR_DIST, CLIPMASK_MISSILE, 4L);
#else
    xvect = sintable[(512 + ang) & 2047] << 4;
    yvect = sintable[ang] << 4;

    clipmoveboxtracenum = 1;

    // must have this
    sectnum = sp->sectnum;
    clipmove((vec3_t *)sp, &sectnum, xvect, yvect,
             4L, 4L<<8, 4L<<8, CLIPMASK_MISSILE);
    clipmoveboxtracenum = 3;

    if (sp->sectnum != sectnum)
        changespritesect(SpriteNum, sectnum);
#endif

    return TRUE;
}

int
InitUzi(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    SPRITEp wp, hsp;
    USERp wu;
    short daang, j;
    hitdata_t hitinfo;
    int daz, nz;
    int nx,ny;
    int xvect,yvect,zvect;
    short cstat = 0;
    uint8_t pal = 0;
    //static char alternate=0;
    static int uziclock=0;
    int clockdiff=0;
    SWBOOL FireSnd = FALSE;
#define UZIFIRE_WAIT 20

    void InitUziShell(PLAYERp);


    PlayerUpdateAmmo(pp, u->WeaponNum, -1);

    if (uziclock > totalclock)
    {
        uziclock = totalclock;
        FireSnd = TRUE;
    }

    clockdiff = totalclock - uziclock;
    if (clockdiff > UZIFIRE_WAIT)
    {
        uziclock = totalclock;
        FireSnd = TRUE;
    }

    if (FireSnd)
        PlaySound(DIGI_UZIFIRE, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    u->Vis = 128;

    if (RANDOM_P2(1024) < 400)
        InitTracerUzi(pp);

    nz = pp->posz + pp->bob_z;
    daz = pp->posz + pp->bob_z;
    daang = 32;
    if (WeaponAutoAimHitscan(pp->SpriteP, &daz, &daang, FALSE) != -1)
    {
        daang += RANDOM_RANGE(24) - 12;
        daang = NORM_ANGLE(daang);
        daz += RANDOM_RANGE(10000) - 5000;
    }
    else
    {
        //daang = NORM_ANGLE(pp->pang + (RANDOM_RANGE(50) - 25));
        daang = NORM_ANGLE(pp->pang + (RANDOM_RANGE(24) - 12));
        daz = ((100 - pp->horiz) * 2000) + (RANDOM_RANGE(24000) - 12000);
    }


    xvect = sintable[NORM_ANGLE(daang + 512)];
    yvect = sintable[NORM_ANGLE(daang)];
    zvect = daz;
    FAFhitscan(pp->posx, pp->posy, nz, pp->cursectnum,       // Start position
               xvect,yvect,zvect,
               &hitinfo, CLIPMASK_MISSILE);

    if (hitinfo.sect < 0)
    {
        return 0;
    }

    SetVisHigh();

    // check to see what you hit
    if (hitinfo.sprite < 0 && hitinfo.wall < 0)
    {
        if (labs(hitinfo.pos.z - sector[hitinfo.sect].ceilingz) <= Z(1))
        {
            hitinfo.pos.z += Z(16);
            SET(cstat, CSTAT_SPRITE_YFLIP);

            if (TEST(sector[hitinfo.sect].ceilingstat, CEILING_STAT_PLAX))
                return 0;

            if (SectorIsUnderwaterArea(hitinfo.sect))
            {
                WarpToSurface(&hitinfo.sect, &hitinfo.pos.x, &hitinfo.pos.y, &hitinfo.pos.z);
                ContinueHitscan(pp, hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang, xvect, yvect, zvect);
                return 0;
            }
        }
        else if (labs(hitinfo.pos.z - sector[hitinfo.sect].floorz) <= Z(1))
        {
            if (TEST(sector[hitinfo.sect].extra, SECTFX_LIQUID_MASK) != SECTFX_LIQUID_NONE)
            {
                SpawnSplashXY(hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,hitinfo.sect);

                if (SectorIsDiveArea(hitinfo.sect))
                {
                    WarpToUnderwater(&hitinfo.sect, &hitinfo.pos.x, &hitinfo.pos.y, &hitinfo.pos.z);
                    ContinueHitscan(pp, hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang, xvect, yvect, zvect);
                    return 0;
                }

                return 0;
            }
        }
    }

    if (hitinfo.wall >= 0)
    {
        if (wall[hitinfo.wall].nextsector >= 0)
        {
            if (TEST(sector[wall[hitinfo.wall].nextsector].ceilingstat, CEILING_STAT_PLAX))
            {
                if (hitinfo.pos.z < sector[wall[hitinfo.wall].nextsector].ceilingz)
                {
                    return 0;
                }
            }
        }


        if (wall[hitinfo.wall].lotag == TAG_WALL_BREAK)
        {
            HitBreakWall(&wall[hitinfo.wall], hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang, u->ID);
            return 0;
        }

        QueueHole(daang,hitinfo.sect,hitinfo.wall,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z);
    }

    // hit a sprite?
    if (hitinfo.sprite >= 0)
    {
        USERp hu = User[hitinfo.sprite];
        hsp = &sprite[hitinfo.sprite];

        if (hu) // JBF: added null check
            if (hu->ID == TRASHCAN)
            {
                extern STATE s_TrashCanPain[];

                PlaySound(DIGI_TRASHLID, &hsp->x, &hsp->y, &hsp->z, v3df_none);
                if (hu->WaitTics <= 0)
                {
                    hu->WaitTics = SEC(2);
                    ChangeState(hitinfo.sprite,s_TrashCanPain);
                }
            }

        if (hsp->lotag == TAG_SPRITE_HIT_MATCH)
        {
            if (MissileHitMatch(-1, WPN_UZI, hitinfo.sprite))
                return 0;
        }

        if (TEST(hsp->extra, SPRX_BREAKABLE) && HitBreakSprite(hitinfo.sprite,0))
        {
            return 0;
        }

        if (BulletHitSprite(pp->SpriteP, hitinfo.sprite, hitinfo.sect, hitinfo.wall, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, 0))
            return 0;

        // hit a switch?
        if (TEST(hsp->cstat, CSTAT_SPRITE_WALL) && (hsp->lotag || hsp->hitag))
        {
            ShootableSwitch(hitinfo.sprite,-1);
        }
    }


    j = SpawnSprite(STAT_MISSILE, UZI_SMOKE, s_UziSmoke, hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang, 0);
    wp = &sprite[j];
    wp->shade = -40;
    wp->xrepeat = UZI_SMOKE_REPEAT;
    wp->yrepeat = UZI_SMOKE_REPEAT;
    SetOwner(pp->PlayerSprite, j);
    //SET(wp->cstat, cstat | CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, cstat | CSTAT_SPRITE_YCENTER);
    wp->clipdist = 8 >> 2;

    HitscanSpriteAdjust(j, hitinfo.wall);
    DoHitscanDamage(j, hitinfo.sprite);

    j = SpawnSprite(STAT_MISSILE, UZI_SPARK, s_UziSpark, hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang, 0);
    wp = &sprite[j];
    wu = User[j];
    wp->shade = -40;
    wp->xrepeat = UZI_SPARK_REPEAT;
    wp->yrepeat = UZI_SPARK_REPEAT;
    SetOwner(pp->PlayerSprite, j);
    wu->spal = wp->pal = pal;
    SET(wp->cstat, cstat | CSTAT_SPRITE_YCENTER);
    wp->clipdist = 8 >> 2;

    HitscanSpriteAdjust(j, hitinfo.wall);

    if (RANDOM_P2(1024) < 100)
    {
        PlaySound(DIGI_RICHOCHET1,&wp->x, &wp->y, &wp->z, v3df_none);
    }
    else if (RANDOM_P2(1024) < 100)
        PlaySound(DIGI_RICHOCHET2,&wp->x, &wp->y, &wp->z, v3df_none);

    return 0;
}

// Electro Magnetic Pulse gun
int
InitEMP(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    SPRITEp wp, hsp=NULL;
    USERp wu;
    short daang, j;
    hitdata_t hitinfo;
    int daz, nz;
    int nx,ny;
    short cstat = 0;
    uint8_t pal = 0;

    if (SW_SHAREWARE) return FALSE; // JBF: verify


    PlayerUpdateAmmo(pp, u->WeaponNum, -1);

    PlaySound(DIGI_RAILFIRE, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_doppler);

    InitTracerUzi(pp);

    //daz = nz = pp->posz + Z(8) + ((100 - pp->horiz) * 72);
    //daang = NORM_ANGLE(pp->pang + (RANDOM_RANGE(50) - 25));

    daz = nz = pp->posz + pp->bob_z;
    daang = 64;
    if (WeaponAutoAimHitscan(pp->SpriteP, &daz, &daang, FALSE) != -1)
    {
    }
    else
    {
        daz = (100 - pp->horiz) * 2000;
        daang = pp->pang;
    }

    FAFhitscan(pp->posx, pp->posy, nz, pp->cursectnum,       // Start position
               sintable[NORM_ANGLE(daang + 512)],      // X vector of 3D ang
               sintable[NORM_ANGLE(daang)],    // Y vector of 3D ang
               daz,                            // Z vector of 3D ang
               &hitinfo, CLIPMASK_MISSILE);

    j = SpawnSprite(STAT_MISSILE, EMP, s_EMPBurst, hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang, 0);

    wp = &sprite[j];
    wu = User[j];

    wu->WaitTics = SEC(7);
    wp->shade = -127;
    SetOwner(pp->PlayerSprite, j);
    SET(wp->cstat, cstat | CSTAT_SPRITE_YCENTER);
    wp->clipdist = 8 >> 2;

    if (hitinfo.sect < 0)
    {
        ////DSPRINTF(ds,"PROBLEM! - FAFhitscan returned a bad hitinfo.sect");
        //MONO_PRINT(ds);
        return 0;
    }

#if 0
    if (TEST(pp->Flags, PF_DIVING) ||
        (hitinfo.wall < 0 && hitinfo.sprite < 0 && SectorIsDiveArea(hitinfo.sect))
        )
    {
        InitUziBullet(pp);
        return 0;
    }
#endif

    SetVisHigh();

    // check to see what you hit
    if (hitinfo.sprite < 0 && hitinfo.wall < 0)
    {
        if (labs(hitinfo.pos.z - sector[hitinfo.sect].ceilingz) <= Z(1))
        {
            hitinfo.pos.z += Z(16);
            SET(cstat, CSTAT_SPRITE_YFLIP);

            if (TEST(sector[hitinfo.sect].ceilingstat, CEILING_STAT_PLAX))
                return 0;
        }
        else if (labs(hitinfo.pos.z - sector[hitinfo.sect].floorz) <= Z(1))
        {
            if (TEST(sector[hitinfo.sect].extra, SECTFX_LIQUID_MASK) != SECTFX_LIQUID_NONE)
            {
                SpawnSplashXY(hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,hitinfo.sect);
                return 0;
            }
        }
    }

    if (hitinfo.wall >= 0)
    {
        if (wall[hitinfo.wall].nextsector >= 0)
        {
            if (TEST(sector[wall[hitinfo.wall].nextsector].ceilingstat, CEILING_STAT_PLAX))
            {
                if (hitinfo.pos.z < sector[wall[hitinfo.wall].nextsector].ceilingz)
                {
                    return 0;
                }
            }
        }

        HitscanSpriteAdjust(j, hitinfo.wall);
        if (wall[hitinfo.wall].lotag == TAG_WALL_BREAK)
        {
            HitBreakWall(&wall[hitinfo.wall], hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang, u->ID);
            return 0;
        }
    }

    // hit a sprite?
    if (hitinfo.sprite >= 0)
    {
        hsp = &sprite[hitinfo.sprite];

        if (hsp->lotag == TAG_SPRITE_HIT_MATCH)
        {
            if (MissileHitMatch(-1, WPN_UZI, hitinfo.sprite))
                return 0;
        }

        if (TEST(hsp->extra, SPRX_BREAKABLE))
        {
            HitBreakSprite(hitinfo.sprite,0);
            //return(0);
        }

        if (BulletHitSprite(pp->SpriteP, hitinfo.sprite, hitinfo.sect, hitinfo.wall, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z,0))
            //return(0);

            // hit a switch?
            if (TEST(hsp->cstat, CSTAT_SPRITE_WALL) && (hsp->lotag || hsp->hitag))
            {
                ShootableSwitch(hitinfo.sprite,-1);
            }

        if (TEST(hsp->extra, SPRX_PLAYER_OR_ENEMY))
        {
            // attach weapon to sprite
            SetAttach(hitinfo.sprite, j);
            wu->sz = sprite[hitinfo.sprite].z - wp->z;
            if (RANDOM_RANGE(1000) > 500)
                PlayerSound(DIGI_YOULOOKSTUPID,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
        }
        else
        {
            if (TEST(hsp->cstat, CSTAT_SPRITE_WALL))
            {
                SET(wu->Flags2, SPR2_ATTACH_WALL);
            }
            else if (TEST(hsp->cstat, CSTAT_SPRITE_FLOOR))
            {
                // hit floor
                if (wp->z > DIV2(wu->hiz + wu->loz))
                    SET(wu->Flags2, SPR2_ATTACH_FLOOR);
                else
                    SET(wu->Flags2, SPR2_ATTACH_CEILING);
            }
            else
            {
                KillSprite(j);
                return FALSE;
            }
        }

    }
    return 0;
}

int
InitTankShell(short SpriteNum, PLAYERp pp)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SPRITEp wp, hsp;
    USERp wu;
    int dist, nang;
    short w;

    if (!SW_SHAREWARE)
        PlaySound(DIGI_CANNON, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_doppler);

    w = SpawnSprite(STAT_MISSILE, 0, s_TankShell, sp->sectnum,
                    sp->x, sp->y, sp->z, sp->ang, TANK_SHELL_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = 8;
    wp->xrepeat = 8;
    wp->shade = -40;
    wp->zvel = 0;
    wp->clipdist = 32 >> 2;

    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 50;
    wu->ceiling_dist = Z(4);
    wu->floor_dist = Z(4);
    SET(wu->Flags2, SPR2_SO_MISSILE);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, CSTAT_SPRITE_INVISIBLE);

    wp->zvel = ((100 - pp->horiz) * (wp->xvel/8));

    WeaponAutoAim(sp, w, 64, FALSE);
    // a bit of randomness
    wp->ang += RANDOM_RANGE(30) - 15;
    wp->ang = NORM_ANGLE(wp->ang);

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    if (SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);


    return 0;
}

int
InitTurretMicro(short SpriteNum, PLAYERp pp)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;

    USERp pu = User[pp->PlayerSprite];
    USERp wu,hu;
    SPRITEp wp,hp;
    int nx, ny, nz, dist;
    short w;
    short oclipdist;
    short i,ang;
    TARGET_SORTp ts = TargetSort;

    if (SW_SHAREWARE) return FALSE; // JBF: verify


    nx = sp->x;
    ny = sp->y;

#define MAX_TURRET_MICRO 10

    DoPickTarget(pp->SpriteP, 256, FALSE);

    if (TargetSortCount > MAX_TURRET_MICRO)
        TargetSortCount = MAX_TURRET_MICRO;

    for (i = 0; i < MAX_TURRET_MICRO; i++)
    {
        if (ts < &TargetSort[TargetSortCount] && ts->sprite_num >= 0)
        {
            hp = &sprite[ts->sprite_num];
            hu = User[ts->sprite_num];

            ang = getangle(hp->x - nx, hp->y - ny);

            ts++;
        }
        else
        {
            hp = NULL;
            hu = NULL;
            ang = sp->ang;
        }

        nz = sp->z;
        nz += Z(RANDOM_RANGE(20)) - Z(10);

        // Spawn a shot
        // Inserting and setting up variables

        w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R0, &s_Micro[0][0], sp->sectnum,
                        nx, ny, nz, ang, 1200);

        wp = &sprite[w];
        wu = User[w];

        SetOwner(pp->PlayerSprite, w);
        wp->yrepeat = 24;
        wp->xrepeat = 24;
        wp->shade = -15;
        wp->zvel = ((100 - pp->horiz) * HORIZ_MULT);
        wp->clipdist = 64L>>2;

        // randomize zvelocity
        wp->zvel += RANDOM_RANGE(Z(8)) - Z(5);

        wu->RotNum = 5;
        NewStateGroup(w, &sg_Micro[0]);

        wu->WeaponNum = pu->WeaponNum;
        wu->Radius = 200;
        wu->ceiling_dist = Z(2);
        wu->floor_dist = Z(2);
        RESET(wp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        SET(wp->cstat, CSTAT_SPRITE_YCENTER);
        SET(wp->cstat, CSTAT_SPRITE_INVISIBLE);

        wu->WaitTics = 10 + RANDOM_RANGE(40);

#define MICRO_ANG 400

        if (hp)
        {
            dist = Distance(wp->x, wp->y, hp->x, hp->y);
            if (dist != 0)
            {
                int zh;
                zh = SPRITEp_TOS(hp) + DIV4(SPRITEp_SIZE_Z(hp));
                wp->zvel = (wp->xvel * (zh - wp->z)) / dist;
            }

            wu->WpnGoal = ts->sprite_num;
            SET(hu->Flags, SPR_TARGETED);
            SET(hu->Flags, SPR_ATTACKED);
        }
        else
        {
            wp->ang = NORM_ANGLE(wp->ang + (RANDOM_RANGE(MICRO_ANG) - DIV2(MICRO_ANG)) - 16);
        }

        wu->xchange = MOVEx(wp->xvel, wp->ang);
        wu->ychange = MOVEy(wp->xvel, wp->ang);
        wu->zchange = wp->zvel;
    }

    return 0;
}


int
InitTurretRocket(short SpriteNum, PLAYERp pp)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SPRITEp wp, hsp;
    USERp wu;
    int dist, nang;
    short w;

    if (SW_SHAREWARE) return FALSE; // JBF: verify


    w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R0, &s_Rocket[0][0], sp->sectnum,
                    sp->x, sp->y, sp->z, sp->ang, ROCKET_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = 40;
    wp->xrepeat = 40;
    wp->shade = -40;
    wp->zvel = 0;
    wp->clipdist = 32 >> 2;

    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 50;
    wu->ceiling_dist = Z(4);
    wu->floor_dist = Z(4);
    SET(wu->Flags2, SPR2_SO_MISSILE);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);

    wp->zvel = ((100 - pp->horiz) * (wp->xvel/8));

    WeaponAutoAim(sp, w, 64, FALSE);
    // a bit of randomness
    //wp->ang += RANDOM_RANGE(30) - 15;

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    if (SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    return 0;
}

int
InitTurretFireball(short SpriteNum, PLAYERp pp)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SPRITEp wp, hsp;
    USERp wu;
    int dist, nang;
    short w;

    if (SW_SHAREWARE) return FALSE; // JBF: verify

    w = SpawnSprite(STAT_MISSILE, FIREBALL, s_Fireball, sp->sectnum,
                    sp->x, sp->y, sp->z, sp->ang, FIREBALL_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = 40;
    wp->xrepeat = 40;
    wp->shade = -40;
    wp->zvel = 0;
    wp->clipdist = 32 >> 2;

    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 50;
    wu->ceiling_dist = Z(4);
    wu->floor_dist = Z(4);
    SET(wu->Flags2, SPR2_SO_MISSILE);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);

    wp->zvel = ((100 - pp->horiz) * (wp->xvel/8));

    WeaponAutoAim(sp, w, 64, FALSE);
    // a bit of randomness
    wp->ang += RANDOM_RANGE(30) - 15;
    wp->ang = NORM_ANGLE(wp->ang);

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    if (SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    return 0;
}

int
InitTurretRail(short SpriteNum, PLAYERp pp)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w;
    short oclipdist;

    if (SW_SHAREWARE) return FALSE; // JBF: verify

    nx = sp->x;
    ny = sp->y;
    nz = sp->z;

    // Spawn a shot
    // Inserting and setting up variables

    w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R1, &s_Rail[0][0], pp->cursectnum,
                    nx, ny, nz, sp->ang, 1200);

    wp = &sprite[w];
    wu = User[w];

    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = 52;
    wp->xrepeat = 52;
    wp->shade = -15;
    wp->zvel = ((100 - pp->horiz) * HORIZ_MULT);

    wu->RotNum = 5;
    NewStateGroup(w, &sg_Rail[0]);

    wu->Radius = 200;
    wu->ceiling_dist = Z(1);
    wu->floor_dist = Z(1);
    SET(wu->Flags2, SPR2_SO_MISSILE);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER|CSTAT_SPRITE_INVISIBLE);
    SET(wp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    wp->clipdist = 64L>>2;

    if (WeaponAutoAim(pp->SpriteP, w, 32, FALSE) == -1)
    {
        wp->ang = NORM_ANGLE(wp->ang);
    }

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    return 0;
}

int
InitTurretLaser(short SpriteNum, PLAYERp pp)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w;
    short oclipdist;

    if (SW_SHAREWARE) return FALSE; // JBF: verify


    nx = sp->x;
    ny = sp->y;
    nz = sp->z;

    // Spawn a shot
    // Inserting and setting up variables

    w = SpawnSprite(STAT_MISSILE, BOLT_THINMAN_R0, s_Laser, pp->cursectnum,
                    nx, ny, nz, sp->ang, 300);

    wp = &sprite[w];
    wu = User[w];

    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = 52;
    wp->xrepeat = 52;
    wp->shade = -15;

    // the slower the missile travels the less of a zvel it needs
    wp->zvel = ((100 - pp->horiz) * HORIZ_MULT);
    wp->zvel /= 4;

    wu->Radius = 200;
    wu->ceiling_dist = Z(1);
    wu->floor_dist = Z(1);
    SET(wu->Flags2, SPR2_SO_MISSILE);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    wp->clipdist = 64L>>2;

    if (WeaponAutoAim(sp, w, 32, FALSE) == -1)
    {
        wp->ang = NORM_ANGLE(wp->ang);
    }

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    return 0;
}

int
InitSobjMachineGun(short SpriteNum, PLAYERp pp)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SPRITEp wp, hsp;
    USERp wu;
    short daang, i, j, exp;
    hitdata_t hitinfo;
    short nsect;
    int daz;
    int nx,ny,nz;
    short cstat = 0;
    short spark;
    //static sound = 0;

    //sound = (++sound)&1;
    //if (sound == 0)
    PlaySound(DIGI_BOATFIRE, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_doppler);

    nx = sp->x;
    ny = sp->y;
    daz = nz = sp->z;
    nsect = sp->sectnum;

    if (RANDOM_P2(1024) < 200)
        InitTracerTurret(sp - sprite, pp->PlayerSprite, pp->horiz);

    daang = 64;
    if (WeaponAutoAimHitscan(sp, &daz, &daang, FALSE) != -1)
    {
        daz += RANDOM_RANGE(Z(30)) - Z(15);
        //daz += 0;
    }
    else
    {
        int horiz;
        horiz = pp->horiz;
        if (horiz < 75)
            horiz = 75;

        daz = ((100 - horiz) * 2000) + (RANDOM_RANGE(Z(80)) - Z(40));
        daang = sp->ang;
    }

    FAFhitscan(nx, ny, nz, nsect,       // Start position
               sintable[NORM_ANGLE(daang + 512)],      // X vector of 3D ang
               sintable[NORM_ANGLE(daang)],    // Y vector of 3D ang
               daz,                            // Z vector of 3D ang
               &hitinfo, CLIPMASK_MISSILE);

    if (hitinfo.sect < 0)
    {
        //DSPRINTF(ds,"PROBLEM! - FAFhitscan returned a bad hitinfo.sect");
        MONO_PRINT(ds);
        return 0;
    }

    if (hitinfo.sprite < 0 && hitinfo.wall < 0)
    {
        if (labs(hitinfo.pos.z - sector[hitinfo.sect].ceilingz) <= Z(1))
        {
            hitinfo.pos.z += Z(16);
            SET(cstat, CSTAT_SPRITE_YFLIP);

            if (TEST(sector[hitinfo.sect].ceilingstat, CEILING_STAT_PLAX))
                return 0;
        }
        else if (labs(hitinfo.pos.z - sector[hitinfo.sect].floorz) <= Z(1))
        {
            if (TEST(sector[hitinfo.sect].extra, SECTFX_LIQUID_MASK) != SECTFX_LIQUID_NONE)
            {
                SpawnSplashXY(hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,hitinfo.sect);
                return 0;
            }
        }

    }

    // hit a sprite?
    if (hitinfo.sprite >= 0)
    {
        SPRITEp hsp = &sprite[hitinfo.sprite];

        if (sprite[hitinfo.sprite].lotag == TAG_SPRITE_HIT_MATCH)
        {
            // spawn sparks here and pass the sprite as SO_MISSILE
            spark = SpawnBoatSparks(pp, hitinfo.sect, hitinfo.wall, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang);
            SET(User[spark]->Flags2, SPR2_SO_MISSILE);
            if (MissileHitMatch(spark, -1, hitinfo.sprite))
                return 0;
            return 0;
        }

        if (TEST(hsp->extra, SPRX_BREAKABLE))
        {
            HitBreakSprite(hitinfo.sprite,0);
            return 0;
        }

        if (BulletHitSprite(pp->SpriteP, hitinfo.sprite, hitinfo.sect, hitinfo.wall, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, 0))
            return 0;

        // hit a switch?
        if (TEST(hsp->cstat, CSTAT_SPRITE_WALL) && (hsp->lotag || hsp->hitag))
        {
            ShootableSwitch(hitinfo.sprite,-1);
        }
    }

    spark = SpawnBoatSparks(pp, hitinfo.sect, hitinfo.wall, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang);
    DoHitscanDamage(spark, hitinfo.sprite);

    return 0;
}

int
InitSobjGun(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    short i;
    SPRITEp sp;
    SWBOOL first = FALSE;

    for (i = 0; pp->sop->sp_num[i] != -1; i++)
    {
        if (sprite[pp->sop->sp_num[i]].statnum == STAT_SO_SHOOT_POINT)
        {
            sp = &sprite[pp->sop->sp_num[i]];

            // match when firing
            if (SP_TAG2(sp))
            {
                DoMatchEverything(pp, SP_TAG2(sp), -1);
                if (TEST_BOOL1(sp))
                {
                    SP_TAG2(sp) = 0;
                }
            }

            // inert shoot point
            if ((uint8_t)SP_TAG3(sp) == 255)
                return 0;

            if (!first)
            {
                first = TRUE;
                if (SP_TAG6(sp))
                    DoSoundSpotMatch(SP_TAG6(sp), 1, SOUND_OBJECT_TYPE);
            }

            switch (SP_TAG3(sp))
            {
            case 32:
            case 0:
                SpawnVis(sp - sprite, -1, -1, -1, -1, 8);
                SpawnBigGunFlames(sp - sprite, pp->PlayerSprite, pp->sop);
                SetGunQuake(sp - sprite);
                InitTankShell(sp - sprite, pp);
                if (!SP_TAG5(sp))
                    pp->FirePause = 80;
                else
                    pp->FirePause = SP_TAG5(sp);
                break;
            case 1:
                SpawnVis(sp - sprite, -1, -1, -1, -1, 32);
                SpawnBigGunFlames(-(sp - sprite), pp->PlayerSprite, pp->sop);
                InitSobjMachineGun(sp - sprite, pp);
                if (!SP_TAG5(sp))
                    pp->FirePause = 10;
                else
                    pp->FirePause = SP_TAG5(sp);
                break;
            case 2:
                if (SW_SHAREWARE) break;
                SpawnVis(sp - sprite, -1, -1, -1, -1, 32);
                InitTurretLaser(sp - sprite, pp);
                if (!SP_TAG5(sp))
                    pp->FirePause = 120;
                else
                    pp->FirePause = SP_TAG5(sp);
                break;
            case 3:
                if (SW_SHAREWARE) break;
                SpawnVis(sp - sprite, -1, -1, -1, -1, 32);
                InitTurretRail(sp - sprite, pp);
                if (!SP_TAG5(sp))
                    pp->FirePause = 120;
                else
                    pp->FirePause = SP_TAG5(sp);
                break;
            case 4:
                if (SW_SHAREWARE) break;
                SpawnVis(sp - sprite, -1, -1, -1, -1, 32);
                InitTurretFireball(sp - sprite, pp);
                if (!SP_TAG5(sp))
                    pp->FirePause = 20;
                else
                    pp->FirePause = SP_TAG5(sp);
                break;
            case 5:
                if (SW_SHAREWARE) break;
                SpawnVis(sp - sprite, -1, -1, -1, -1, 32);
                InitTurretRocket(sp - sprite, pp);
                if (!SP_TAG5(sp))
                    pp->FirePause = 100;
                else
                    pp->FirePause = SP_TAG5(sp);
                break;
            case 6:
                if (SW_SHAREWARE) break;
                SpawnVis(sp - sprite, -1, -1, -1, -1, 32);
                InitTurretMicro(sp - sprite, pp);
                if (!SP_TAG5(sp))
                    pp->FirePause = 100;
                else
                    pp->FirePause = SP_TAG5(sp);
                break;
            }
        }
    }

    return 0;
}

int
SpawnBoatSparks(PLAYERp pp, short hit_sect, short hit_wall, int hit_x, int hit_y, int hit_z, short hit_ang)
{
    USERp u = User[pp->PlayerSprite];
    short j;
    int nx,ny;
    SPRITEp wp;
    USERp wu;

    j = SpawnSprite(STAT_MISSILE, UZI_SMOKE, s_UziSmoke, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);
    wp = &sprite[j];
    wp->shade = -40;
    wp->xrepeat = UZI_SMOKE_REPEAT + 12;
    wp->yrepeat = UZI_SMOKE_REPEAT + 12;
    SetOwner(pp->PlayerSprite, j);
    SET(wp->cstat, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);

    wp->hitag = LUMINOUS; //Always full brightness
    // Sprite starts out with center exactly on wall.
    // This moves it back enough to see it at all angles.

    wp->clipdist = 32 >> 2;

    HitscanSpriteAdjust(j, hit_wall);

    j = SpawnSprite(STAT_MISSILE, UZI_SPARK, s_UziSpark, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);
    wp = &sprite[j];
    wu = User[j];
    wp->shade = -40;
    wp->xrepeat = UZI_SPARK_REPEAT + 10;
    wp->yrepeat = UZI_SPARK_REPEAT + 10;
    SetOwner(pp->PlayerSprite, j);
    wu->spal = wp->pal = PALETTE_DEFAULT;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);

    wp->clipdist = 32 >> 2;

    HitscanSpriteAdjust(j, hit_wall);

    if (RANDOM_P2(1024) < 100)
        PlaySound(DIGI_RICHOCHET1,&wp->x, &wp->y, &wp->z, v3df_none);

    return j;
}

int
SpawnSwordSparks(PLAYERp pp, short hit_sect, short hit_wall, int hit_x, int hit_y, int hit_z, short hit_ang)
{
    USERp u = User[pp->PlayerSprite];
    short j;
    int nx,ny;
    SPRITEp wp;
    USERp wu;

    j = SpawnSprite(STAT_MISSILE, UZI_SMOKE, s_UziSmoke, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);
    wp = &sprite[j];
    wp->shade = -40;
    wp->xrepeat = wp->yrepeat = 20;
    SetOwner(pp->PlayerSprite, j);
    SET(wp->cstat, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
    wp->hitag = LUMINOUS; //Always full brightness

    // Sprite starts out with center exactly on wall.
    // This moves it back enough to see it at all angles.

    wp->clipdist = 32 >> 2;

    if (hit_wall != -1)
        HitscanSpriteAdjust(j, hit_wall);

    j = SpawnSprite(STAT_MISSILE, UZI_SPARK, s_UziSpark, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);
    wp = &sprite[j];
    wu = User[j];
    wp->shade = -40;
    wp->xrepeat = wp->yrepeat = 20;
    SetOwner(pp->PlayerSprite, j);
    wu->spal = wp->pal = PALETTE_DEFAULT;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    if (u->WeaponNum == WPN_FIST)
        SET(wp->cstat, CSTAT_SPRITE_INVISIBLE);

    wp->clipdist = 32 >> 2;

    if (hit_wall != -1)
        HitscanSpriteAdjust(j, hit_wall);

    return 0;
}

int
SpawnTurretSparks(SPRITEp sp, short hit_sect, short hit_wall, int hit_x, int hit_y, int hit_z, short hit_ang)
{
    //USERp u = User[sp - sprite];
    short j;
    int nx,ny;
    SPRITEp wp;
    USERp wu;

    j = SpawnSprite(STAT_MISSILE, UZI_SMOKE, s_UziSmoke, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);
    wp = &sprite[j];
    wp->shade = -40;
    wp->xrepeat = UZI_SMOKE_REPEAT + 12;
    wp->yrepeat = UZI_SMOKE_REPEAT + 12;
    SET(wp->cstat, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
    wp->hitag = LUMINOUS; //Always full brightness

    // Sprite starts out with center exactly on wall.
    // This moves it back enough to see it at all angles.

    wp->clipdist = 32 >> 2;
    HitscanSpriteAdjust(j, hit_wall);

    j = SpawnSprite(STAT_MISSILE, UZI_SPARK, s_UziSpark, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);
    wp = &sprite[j];
    wu = User[j];
    wp->shade = -40;
    wp->xrepeat = UZI_SPARK_REPEAT + 10;
    wp->yrepeat = UZI_SPARK_REPEAT + 10;
    wu->spal = wp->pal = PALETTE_DEFAULT;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);

    wp->clipdist = 32 >> 2;
    HitscanSpriteAdjust(j, hit_wall);

    if (RANDOM_P2(1024) < 100)
        PlaySound(DIGI_RICHOCHET1, &wp->x, &wp->y, &wp->z, v3df_none);

    return j;
}

int
SpawnShotgunSparks(PLAYERp pp, short hit_sect, short hit_wall, int hit_x, int hit_y, int hit_z, short hit_ang)
{
    USERp u = User[pp->PlayerSprite];
    short j;
    int nx,ny;
    SPRITEp wp;
    USERp wu;

    j = SpawnSprite(STAT_MISSILE, UZI_SPARK, s_UziSpark, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);
    wp = &sprite[j];
    wu = User[j];
    wp->shade = -40;
    wp->xrepeat = UZI_SPARK_REPEAT;
    wp->yrepeat = UZI_SPARK_REPEAT;
    SetOwner(pp->PlayerSprite, j);
    wu->spal = wp->pal = PALETTE_DEFAULT;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);

    wp->clipdist = 32 >> 2;

    HitscanSpriteAdjust(j, hit_wall);

    j = SpawnSprite(STAT_MISSILE, SHOTGUN_SMOKE, s_ShotgunSmoke, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);
    wp = &sprite[j];
    wp->shade = -40;
    wp->xrepeat = SHOTGUN_SMOKE_REPEAT;
    wp->yrepeat = SHOTGUN_SMOKE_REPEAT;
    SetOwner(pp->PlayerSprite, j);
    SET(wp->cstat, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);

    wp->hitag = LUMINOUS; //Always full brightness
    // Sprite starts out with center exactly on wall.
    // This moves it back enough to see it at all angles.

    wp->clipdist = 32 >> 2;

    HitscanSpriteAdjust(j, hit_wall);

    return j;
}

int
InitTurretMgun(SECTOR_OBJECTp sop)
{
    SPRITEp wp, hsp;
    USERp wu;
    short daang, i, j, exp;
    hitdata_t hitinfo;
    short nsect;
    int daz;
    int nx,ny,nz;
    short cstat = 0;
    short delta;
    SPRITEp sp,ep;
    int xvect,yvect,zvect;

    PlaySound(DIGI_BOATFIRE, &sop->xmid, &sop->ymid, &sop->zmid, v3df_dontpan|v3df_doppler);

    for (i = 0; sop->sp_num[i] != -1; i++)
    {
        if (sprite[sop->sp_num[i]].statnum == STAT_SO_SHOOT_POINT)
        {
            sp = &sprite[sop->sp_num[i]];

            nx = sp->x;
            ny = sp->y;
            daz = nz = sp->z;
            nsect = sp->sectnum;

            // if its not operated by a player
            if (sop->Animator)
            {
                // only auto aim for Z
                daang = 512;
                if ((hitinfo.sprite = WeaponAutoAimHitscan(sp, &daz, &daang, FALSE)) != -1)
                {
                    delta = labs(GetDeltaAngle(daang, sp->ang));
                    if (delta > 128)
                    {
                        // don't shoot if greater than 128
                        return 0;
                    }
                    else if (delta > 24)
                    {
                        // always shoot the ground when tracking
                        // and not close
                        WeaponHitscanShootFeet(sp, &sprite[hitinfo.sprite], &daz);

                        daang = sp->ang;
                        daang = NORM_ANGLE(daang + RANDOM_P2(32) - 16);
                    }
                    else
                    {
                        // randomize the z for shots
                        daz += RANDOM_RANGE(Z(120)) - Z(60);
                        // never auto aim the angle
                        daang = sp->ang;
                        daang = NORM_ANGLE(daang + RANDOM_P2(64) - 32);
                    }
                }
            }
            else
            {
                daang = 64;
                if (WeaponAutoAimHitscan(sp, &daz, &daang, FALSE) != -1)
                {
                    daz += RANDOM_RANGE(Z(30)) - Z(15);
                }
            }

            xvect = sintable[NORM_ANGLE(daang + 512)];
            yvect = sintable[NORM_ANGLE(daang)];
            zvect = daz;

            FAFhitscan(nx, ny, nz, nsect,       // Start position
                       xvect, yvect, zvect,
                       &hitinfo, CLIPMASK_MISSILE);

            if (RANDOM_P2(1024) < 400)
            {
                InitTracerAutoTurret(sop->sp_num[i], -1,
                                     xvect>>4, yvect>>4, zvect>>4);
            }

            if (hitinfo.sect < 0)
                continue;

            if (hitinfo.sprite < 0 && hitinfo.wall < 0)
            {
                if (labs(hitinfo.pos.z - sector[hitinfo.sect].ceilingz) <= Z(1))
                {
                    hitinfo.pos.z += Z(16);
                    SET(cstat, CSTAT_SPRITE_YFLIP);

                    if (TEST(sector[hitinfo.sect].ceilingstat, CEILING_STAT_PLAX))
                        continue;
                }
                else if (labs(hitinfo.pos.z - sector[hitinfo.sect].floorz) <= Z(1))
                {
                    if (TEST(sector[hitinfo.sect].extra, SECTFX_LIQUID_MASK) != SECTFX_LIQUID_NONE)
                    {
                        SpawnSplashXY(hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,hitinfo.sect);
                        continue;
                    }
                }

            }

            if (hitinfo.wall >= 0)
            {
                if (wall[hitinfo.wall].nextsector >= 0)
                {
                    if (TEST(sector[wall[hitinfo.wall].nextsector].ceilingstat, CEILING_STAT_PLAX))
                    {
                        if (hitinfo.pos.z < sector[wall[hitinfo.wall].nextsector].ceilingz)
                        {
                            return 0;
                        }
                    }
                }

                if (wall[hitinfo.wall].lotag == TAG_WALL_BREAK)
                {
                    HitBreakWall(&wall[hitinfo.wall], hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang, 0);
                    continue;
                }

                QueueHole(daang,hitinfo.sect,hitinfo.wall,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z);
            }

            // hit a sprite?
            if (hitinfo.sprite >= 0)
            {
                hsp = &sprite[hitinfo.sprite];

                if (hsp->lotag == TAG_SPRITE_HIT_MATCH)
                {
                    if (MissileHitMatch(-1, WPN_UZI, hitinfo.sprite))
                        continue;
                }

                if (TEST(hsp->extra, SPRX_BREAKABLE))
                {
                    HitBreakSprite(hitinfo.sprite,0);
                    continue;
                }

                if (BulletHitSprite(sp, hitinfo.sprite, hitinfo.sect, hitinfo.wall, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, 0))
                    continue;

                // hit a switch?
                if (TEST(hsp->cstat, CSTAT_SPRITE_WALL) && (hsp->lotag || hsp->hitag))
                {
                    ShootableSwitch(hitinfo.sprite,-1);
                }
            }


            j = SpawnTurretSparks(sp, hitinfo.sect, hitinfo.wall, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang);
            DoHitscanDamage(j, hitinfo.sprite);
        }
    }

    return 0;
}


int
InitEnemyUzi(short SpriteNum)
{
    SPRITEp sp = User[SpriteNum]->SpriteP,wp;
    USERp u = User[SpriteNum];
    USERp wu;
    short daang, j, sectnum;
    hitdata_t hitinfo = { { -2, -2, -2 }, -2, -2, -2 };
    int ret = -2;
    int daz;
    int zh;
    void InitUziShell(PLAYERp);
    static short alternate;


    // Make sprite shade brighter
    u->Vis = 128;

    setspritez(SpriteNum, (vec3_t *)sp);

    if (u->ID == ZILLA_RUN_R0)
    {
        zh = SPRITEp_TOS(sp);
        zh += Z(20);
    }
    else
    {
        zh = SPRITEp_SIZE_Z(sp);
        zh -= DIV4(zh);
    }
    daz = sp->z - zh;

    if (AimHitscanToTarget(sp, &daz, &daang, 200) != -1)
    {
        // set angle to player and also face player when attacking
        sp->ang = daang;
        daang += RANDOM_RANGE(24) - 12;
        daang = NORM_ANGLE(daang);
        daz += RANDOM_RANGE(Z(40)) - Z(20);
    }
    else
    {
        // couldn't shoot target for some reason

        // don't bother wasting processing 50% of the time
        if (RANDOM_P2(1024) < 512)
            return 0;

        daz = 0;
        daang = NORM_ANGLE(sp->ang + (RANDOM_P2(128)) - 64);
    }

    FAFhitscan(sp->x, sp->y, sp->z - zh, sp->sectnum,      // Start position
               sintable[NORM_ANGLE(daang + 512)],      // X vector of 3D ang
               sintable[NORM_ANGLE(daang)],    // Y vector of 3D ang
               daz,                            // Z vector of 3D ang
               &hitinfo, CLIPMASK_MISSILE);

    if (hitinfo.sect < 0)
        return 0;

    if (RANDOM_P2(1024<<4)>>4 > 700)
    {
        if (u->ID == TOILETGIRL_R0 || u->ID == WASHGIRL_R0 || u->ID == CARGIRL_R0)
            SpawnShell(SpriteNum,-3);
        else
            SpawnShell(SpriteNum,-2); // Enemy Uzi shell
    }

    if ((alternate++)>2) alternate = 0;
    if (!alternate)
    {
        if (sp->pal == PALETTE_PLAYER3 || sp->pal == PALETTE_PLAYER5 ||
            sp->pal == PAL_XLAT_LT_GREY || sp->pal == PAL_XLAT_LT_TAN)
            PlaySound(DIGI_M60, &sp->x, &sp->y, &sp->z, v3df_none);
        else
            PlaySound(DIGI_NINJAUZIATTACK, &sp->x, &sp->y, &sp->z, v3df_none);
    }

    if (hitinfo.wall >= 0)
    {
        if (wall[hitinfo.wall].nextsector >= 0)
        {
            if (TEST(sector[wall[hitinfo.wall].nextsector].ceilingstat, CEILING_STAT_PLAX))
            {
                if (hitinfo.pos.z < sector[wall[hitinfo.wall].nextsector].ceilingz)
                {
                    return 0;
                }
            }
        }

        if (wall[hitinfo.wall].lotag == TAG_WALL_BREAK)
        {
            HitBreakWall(&wall[hitinfo.wall], hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang, u->ID);
            return 0;
        }

        QueueHole(daang,hitinfo.sect,hitinfo.wall,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z);
    }

    if (hitinfo.sprite >= 0)
    {
        if (BulletHitSprite(sp, hitinfo.sprite, hitinfo.sect, hitinfo.wall, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, 0))
            return 0;
    }

    j = SpawnSprite(STAT_MISSILE, UZI_SMOKE+2, s_UziSmoke, hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang, 0);
    wp = &sprite[j];
    wp->shade = -40;
    wp->xrepeat = UZI_SMOKE_REPEAT;
    wp->yrepeat = UZI_SMOKE_REPEAT;

    if (u->ID == ZOMBIE_RUN_R0)
        SetOwner(sp->owner, j);
    else
        SetOwner(SpriteNum, j);

    User[j]->WaitTics = 63;
    //SET(wp->cstat, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);

    wp->clipdist = 32L >> 2;
    //HitscanSpriteAdjust(j, hitinfo.wall);

    j = SpawnSprite(STAT_MISSILE, UZI_SMOKE, s_UziSmoke, hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang, 0);
    wp = &sprite[j];
    wp->shade = -40;
    wp->xrepeat = UZI_SMOKE_REPEAT;
    wp->yrepeat = UZI_SMOKE_REPEAT;
    SetOwner(SpriteNum, j);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    wp->clipdist = 8 >> 2;

    HitscanSpriteAdjust(j, hitinfo.wall);
    DoHitscanDamage(j, hitinfo.sprite);

    j = SpawnSprite(STAT_MISSILE, UZI_SPARK, s_UziSpark, hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, daang, 0);
    wp = &sprite[j];
    wu = User[j];
    wp->shade = -40;
    wp->xrepeat = UZI_SPARK_REPEAT;
    wp->yrepeat = UZI_SPARK_REPEAT;
    SetOwner(SpriteNum, j);
    wu->spal = wp->pal;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    wp->clipdist = 8 >> 2;

    HitscanSpriteAdjust(j, hitinfo.wall);

    if (RANDOM_P2(1024) < 100)
    {
        PlaySound(DIGI_RICHOCHET1,&wp->x, &wp->y, &wp->z, v3df_none);
    }
    else if (RANDOM_P2(1024) < 100)
        PlaySound(DIGI_RICHOCHET2,&wp->x, &wp->y, &wp->z, v3df_none);

    return 0;
}


int
InitGrenade(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w;
    short oclipdist;
    int dist;
    int zvel;
    SWBOOL auto_aim = FALSE;

    DoPlayerBeginRecoil(pp, GRENADE_RECOIL_AMT);

    PlayerUpdateAmmo(pp, u->WeaponNum, -1);

    PlaySound(DIGI_30MMFIRE, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    u->Vis = 128;

    nx = pp->posx;
    ny = pp->posy;
    nz = pp->posz + pp->bob_z + Z(8);

    // Spawn a shot
    // Inserting and setting up variables
    w = SpawnSprite(STAT_MISSILE, GRENADE, &s_Grenade[0][0], pp->cursectnum,
                    nx, ny, nz, pp->pang, GRENADE_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    // don't throw it as far if crawling
    if (TEST(pp->Flags, PF_CRAWLING))
    {
        wp->xvel -= DIV4(wp->xvel);
    }

    wu->RotNum = 5;
    NewStateGroup(w, &sg_Grenade[0]);
    SET(wu->Flags, SPR_XFLIP_TOGGLE);

    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = 32;
    wp->xrepeat = 32;
    wp->shade = -15;
    //wp->clipdist = 80L>>2;
    wp->clipdist = 32L>>2;
    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 200;
    wu->ceiling_dist = Z(3);
    wu->floor_dist = Z(3);
    wu->Counter = 0;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, CSTAT_SPRITE_BLOCK);

    if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    wp->zvel = ((100 - pp->horiz) * HORIZ_MULT);

    ////DSPRINTF(ds,"horiz %d, ho %d, ho+ho %d",pp->horiz, pp->horizoff, pp->horizoff + pp->horiz);
    //MONO_PRINT(ds);

    oclipdist = pp->SpriteP->clipdist;
    pp->SpriteP->clipdist = 0;

    wp->ang = NORM_ANGLE(wp->ang + 512);
    HelpMissileLateral(w, 800);
    wp->ang = NORM_ANGLE(wp->ang - 512);

    // don't do smoke for this movement
    SET(wu->Flags, SPR_BOUNCE);
    MissileSetPos(w, DoGrenade, 1000);
    RESET(wu->Flags, SPR_BOUNCE);

    pp->SpriteP->clipdist = oclipdist;

    //dist = FindDistance2D(pp->xvect, pp->yvect)>>12;
    //dist = dist - (dist/2);

    zvel = wp->zvel;
    if (WeaponAutoAim(pp->SpriteP, w, 32, FALSE) >= 0)
    {
        auto_aim = TRUE;
    }
    wp->zvel = zvel;

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    if (!auto_aim)
    {
        // adjust xvel according to player velocity
        wu->xchange += pp->xvect>>14;
        wu->ychange += pp->yvect>>14;
    }

    wu->Counter2 = TRUE;  // Phosphorus Grenade

    return 0;
}

int
InitSpriteGrenade(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w;
    short oclipdist;
    int dist;


    PlaySound(DIGI_30MMFIRE, &sp->x, &sp->y, &sp->z, v3df_dontpan|v3df_doppler);

    nx = sp->x;
    ny = sp->y;
    nz = sp->z - Z(40);

    // Spawn a shot
    // Inserting and setting up variables
    w = SpawnSprite(STAT_MISSILE, GRENADE, &s_Grenade[0][0], sp->sectnum,
                    nx, ny, nz, sp->ang, GRENADE_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    wu->RotNum = 5;
    NewStateGroup(w, &sg_Grenade[0]);
    SET(wu->Flags, SPR_XFLIP_TOGGLE);

    if (u->ID == ZOMBIE_RUN_R0)
        SetOwner(sp->owner, w);
    else
        SetOwner(SpriteNum, w);
    wp->yrepeat = 32;
    wp->xrepeat = 32;
    wp->shade = -15;
    //wp->clipdist = 80L>>2;
    wp->clipdist = 32L>>2;
    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 200;
    wu->ceiling_dist = Z(3);
    wu->floor_dist = Z(3);
    wu->Counter = 0;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    SET(wp->cstat, CSTAT_SPRITE_BLOCK);


    //wp->zvel = (-RANDOM_RANGE(100) * HORIZ_MULT);
    wp->zvel = -2000;

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = wp->zvel;

    wp->ang = NORM_ANGLE(wp->ang + 512);
    HelpMissileLateral(w, 800);
    wp->ang = NORM_ANGLE(wp->ang - 512);

    // don't do smoke for this movement
    SET(wu->Flags, SPR_BOUNCE);
    MissileSetPos(w, DoGrenade, 400);
    RESET(wu->Flags, SPR_BOUNCE);

    return 0;
}

int
InitMine(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w;
    int dist;
    int dot;

    PlayerUpdateAmmo(pp, u->WeaponNum, -1);

    PlaySound(DIGI_MINETHROW, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_doppler);

    nx = pp->posx;
    ny = pp->posy;
    nz = pp->posz + pp->bob_z + Z(8);

    // Spawn a shot
    // Inserting and setting up variables
    w = SpawnSprite(STAT_MISSILE, MINE, s_Mine, pp->cursectnum,
                    nx, ny, nz, pp->pang, MINE_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    SetOwner(pp->PlayerSprite, w);
    wp->yrepeat = 32;
    wp->xrepeat = 32;
    wp->shade = -15;
    wp->clipdist = 128L>>2;
    wp->zvel = ((100 - pp->horiz) * HORIZ_MULT);
    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 200;
    wu->ceiling_dist = Z(5);
    wu->floor_dist = Z(5);
    wu->Counter = 0;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(wp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    wu->spal = wp->pal = User[pp->PlayerSprite]->spal; // Set sticky color

    if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    //wp->zvel = ((100 - pp->horiz) * HORIZ_MULT);

    MissileSetPos(w, DoMine, 800);

    wu->zchange = wp->zvel>>1;
    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);

    dot = DOT_PRODUCT_2D(pp->xvect, pp->yvect, sintable[NORM_ANGLE(pp->pang+512)], sintable[pp->pang]);

    // don't adjust for strafing
    if (labs(dot) > 10000)
    {
        // adjust xvel according to player velocity
        wu->xchange += pp->xvect>>13;
        wu->ychange += pp->yvect>>13;
    }

    return 0;
}

int
InitEnemyMine(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    USERp wu;
    SPRITEp wp;
    int nx, ny, nz;
    short w;
    int dist;
    int dot;
    int dx,dy,dz;
    int tvel;


    PlaySound(DIGI_MINETHROW, &sp->x, &sp->y, &sp->z, v3df_dontpan|v3df_doppler);

    nx = sp->x;
    ny = sp->y;
    nz = sp->z - Z(40);

    // Spawn a shot
    // Inserting and setting up variables
    w = SpawnSprite(STAT_MISSILE, MINE, s_Mine, sp->sectnum,
                    nx, ny, nz, sp->ang, MINE_VELOCITY);

    wp = &sprite[w];
    wu = User[w];

    SetOwner(SpriteNum, w);
    wp->yrepeat = 32;
    wp->xrepeat = 32;
    wp->shade = -15;
    wp->clipdist = 128L>>2;

    // set dx,dy,dz up for finding the z magnitude
    dx = sp->x;
    dy = sp->y;
    dz = sp->z;

    // find the distance to the target (player)
    dist = DIST(dx, dy, u->tgt_sp->x, u->tgt_sp->y);

    // (velocity * difference between the target and the object) /
    // distance
    tvel = -((((MINE_VELOCITY) *(wp->z - dz)) / dist)*128);

    wu->WeaponNum = u->WeaponNum;
    wu->Radius = 200;
    wu->ceiling_dist = Z(5);
    wu->floor_dist = Z(5);
    wu->Counter = 0;
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(wp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    wu->spal = wp->pal = User[SpriteNum]->spal; // Set sticky color

    if (SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    MissileSetPos(w, DoMine, 300);
    wp->ang = NORM_ANGLE(wp->ang-512);
    MissileSetPos(w, DoMine, 300);
    wp->ang = NORM_ANGLE(wp->ang+512);

    wu->zchange = -5000;
    //CON_Message("change = %ld",wu->zchange);
    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);

    return 0;
}

int
HelpMissileLateral(int16_t Weapon, int dist)
{
    SPRITEp sp = &sprite[Weapon];
    USERp u = User[Weapon];
    int x, y;
    int xchange, ychange;
    short old_xvel;
    short old_clipdist;

    old_xvel = sp->xvel;
    old_clipdist = sp->clipdist;

    sp->xvel = dist;
    xchange = MOVEx(sp->xvel, sp->ang);
    ychange = MOVEy(sp->xvel, sp->ang);

    sp->clipdist = 32L >> 2;
    x = sp->x;
    y = sp->y;

    //u->ret = move_missile(Weapon, xchange, ychange, 0, Z(16), Z(16), CLIPMASK_MISSILE, 1);
    u->ret = move_missile(Weapon, xchange, ychange, 0, Z(16), Z(16), 0, 1);

// !JIM! Should this be fatal, it seems to fail check alot, but doesn't crash game or anything
// when I commented it out.
//    ASSERT(!u->ret);

    sp->xvel = old_xvel;
    sp->clipdist = old_clipdist;

    u->ox = sp->x;
    u->oy = sp->y;
    u->oz = sp->z;
    return 0;
}


int
InitFireball(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    SPRITEp wp;
    int nx = 0, ny = 0, nz, oldx, oldy, oldspeed;
    short new_angle, w;
    USERp wu;
    short oclipdist;
    int zvel;

    PlayerUpdateAmmo(pp, WPN_HOTHEAD, -1);

    PlaySound(DIGI_HEADFIRE, &pp->posx, &pp->posy, &pp->posz, v3df_none);

    // Make sprite shade brighter
    u->Vis = 128;

    nx += pp->posx;
    ny += pp->posy;

    nz = pp->posz + pp->bob_z + Z(15);

    w = SpawnSprite(STAT_MISSILE, FIREBALL1, s_Fireball, pp->cursectnum, nx, ny, nz, pp->pang, FIREBALL_VELOCITY);
    wp = &sprite[w];
    wu = User[w];

    wp->hitag = LUMINOUS; //Always full brightness
    wp->xrepeat = 40;
    wp->yrepeat = 40;
    wp->shade = -40;
    wp->clipdist = 32>>2;
    SetOwner(pp->PlayerSprite, w);
    SET(wp->cstat, CSTAT_SPRITE_YCENTER);
    wu->Radius = 100;

    wu->ceiling_dist = Z(6);
    wu->floor_dist = Z(6);
    //zvel = ((100 - pp->horiz) * (100+ADJUST));
    zvel = ((100 - pp->horiz) * (240L));

    //wu->RotNum = 5;
    //NewStateGroup(w, &sg_Fireball);
    //SET(wu->Flags, SPR_XFLIP_TOGGLE);

    // at certain angles the clipping box was big enough to block the
    // initial positioning of the fireball.
    oclipdist = pp->SpriteP->clipdist;
    pp->SpriteP->clipdist = 0;

    wp->ang = NORM_ANGLE(wp->ang + 512);
    HelpMissileLateral(w, 2100);
    wp->ang = NORM_ANGLE(wp->ang - 512);

    if (TEST(pp->Flags, PF_DIVING) || SpriteInUnderwaterArea(wp))
        SET(wu->Flags, SPR_UNDERWATER);

    if (TestMissileSetPos(w, DoFireball, 1200, mulscale16(zvel,44000)))
    {
        pp->SpriteP->clipdist = oclipdist;
        KillSprite(w);
        return 0;
    }

    pp->SpriteP->clipdist = oclipdist;

    wp->zvel = zvel >> 1;
    if (WeaponAutoAimZvel(pp->SpriteP, w, &zvel, 32, FALSE) == -1)
    {
        wp->ang = NORM_ANGLE(wp->ang - 9);
    }

    wu->xchange = MOVEx(wp->xvel, wp->ang);
    wu->ychange = MOVEy(wp->xvel, wp->ang);
    wu->zchange = zvel;

    return 0;
}

int
InitEnemyFireball(short SpriteNum)
{
    SPRITEp sp = User[SpriteNum]->SpriteP, fp = NULL;
    USERp u = User[SpriteNum], fu;
    SPRITEp wp;
    int nz, dist;
    int size_z;
    short new_angle, w;
    USERp wu;
    SPRITEp tsp;
    int i, targ_z, zvel=0, xchange, ychange, zchange;

    static short lat_ang[] =
    {
        512, -512
    };

    ASSERT(SpriteNum >= 0);

    tsp = u->tgt_sp;

    PlaySound(DIGI_FIREBALL1, &sp->x, &sp->y, &sp->z, v3df_none);

    // get angle to player and also face player when attacking
    sp->ang = NORM_ANGLE(getangle(tsp->x - sp->x, tsp->y - sp->y));

    size_z = Z(SPRITEp_SIZE_Y(sp));
    //nz = sp->z - size_z + DIV4(size_z) + DIV8(size_z);
    nz = sp->z - size_z + DIV4(size_z) + DIV8(size_z) + Z(4);

    xchange = MOVEx(GORO_FIREBALL_VELOCITY, sp->ang);
    ychange = MOVEy(GORO_FIREBALL_VELOCITY, sp->ang);

    for (i = 0; i < 2; i++)
    {
        w = SpawnSprite(STAT_MISSILE, GORO_FIREBALL, s_Fireball, sp->sectnum,
                        sp->x, sp->y, nz, sp->ang, GORO_FIREBALL_VELOCITY);

        wp = &sprite[w];
        wu = User[w];

        wp->hitag = LUMINOUS; //Always full brightness
        wp->xrepeat = 20;
        wp->yrepeat = 20;
        wp->shade = -40;
        //wp->owner = SpriteNum;
        SetOwner(SpriteNum, w);
        wp->zvel = 0;
        wp->clipdist = 16>>2;

        wp->ang = NORM_ANGLE(wp->ang + lat_ang[i]);
        HelpMissileLateral(w, 500L);
        wp->ang = NORM_ANGLE(wp->ang - lat_ang[i]);

        wu->xchange = xchange;
        wu->ychange = ychange;

        //MissileSetPos(w, DoFireball, 700);
        MissileSetPos(w, DoFireball, 700);

        if (i == 0)
        {
            // back up first one
            fp = wp;
            fu = wu;

            // find the distance to the target (player)
            dist = ksqrt(SQ(wp->x - tsp->x) + SQ(wp->y - tsp->y));
            //dist = Distance(wp->x, wp->y, tsp->x, tsp->y);

            // Determine target Z value
            //targ_z = tsp->z - Z(SPRITEp_SIZE_Y(sp)) + Z(DIV2(SPRITEp_SIZE_Y(sp)));
            //targ_z = tsp->z;
            targ_z = tsp->z - DIV2(Z(SPRITEp_SIZE_Y(sp)));

            // (velocity * difference between the target and the throwing star) /
            // distance
            if (dist != 0)
                wu->zchange = wp->zvel = (GORO_FIREBALL_VELOCITY * (targ_z - wp->z)) / dist;
        }
        else
        {
            // use the first calculations so the balls stay together
            wu->zchange = wp->zvel = fp->zvel;
        }
    }

    return 0;

}

///////////////////////////////////////////////////////////////////////////////
// for hitscans or other uses
///////////////////////////////////////////////////////////////////////////////

SWBOOL
WarpToUnderwater(short *sectnum, int *x, int *y, int *z)
{
    short i, nexti;
    SECT_USERp sectu = SectUser[*sectnum];
    SPRITEp under_sp = NULL, over_sp = NULL;
    char Found = FALSE;
    short over, under;
    int sx, sy;

    // 0 not valid for water match tags
    if (sectu->number == 0)
        return FALSE;

    // search for DIVE_AREA "over" sprite for reference point
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_DIVE_AREA], i, nexti)
    {
        over_sp = &sprite[i];

        if (TEST(sector[over_sp->sectnum].extra, SECTFX_DIVE_AREA) &&
            SectUser[over_sp->sectnum] &&
            SectUser[over_sp->sectnum]->number == sectu->number)
        {
            Found = TRUE;
            break;
        }
    }

    ASSERT(Found == TRUE);
    Found = FALSE;

    // search for UNDERWATER "under" sprite for reference point
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_UNDERWATER], i, nexti)
    {
        under_sp = &sprite[i];

        if (TEST(sector[under_sp->sectnum].extra, SECTFX_UNDERWATER) &&
            SectUser[under_sp->sectnum] &&
            SectUser[under_sp->sectnum]->number == sectu->number)
        {
            Found = TRUE;
            break;
        }
    }

    ASSERT(Found == TRUE);

    // get the offset from the sprite
    sx = over_sp->x - *x;
    sy = over_sp->y - *y;

    // update to the new x y position
    *x = under_sp->x - sx;
    *y = under_sp->y - sy;

    over = over_sp->sectnum;
    under = under_sp->sectnum;

    if (GetOverlapSector(*x, *y, &over, &under) == 2)
    {
        *sectnum = under;
    }
    else
    {
        *sectnum = under;
    }

    *z = sector[under_sp->sectnum].ceilingz + Z(1);

    return TRUE;
}

SWBOOL
WarpToSurface(short *sectnum, int *x, int *y, int *z)
{
    short i, nexti;
    SECT_USERp sectu = SectUser[*sectnum];
    short over, under;
    int sx, sy;

    SPRITEp under_sp = NULL, over_sp = NULL;
    char Found = FALSE;

    // 0 not valid for water match tags
    if (sectu->number == 0)
        return FALSE;

    // search for UNDERWATER "under" sprite for reference point
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_UNDERWATER], i, nexti)
    {
        under_sp = &sprite[i];

        if (TEST(sector[under_sp->sectnum].extra, SECTFX_UNDERWATER) &&
            SectUser[under_sp->sectnum] &&
            SectUser[under_sp->sectnum]->number == sectu->number)
        {
            Found = TRUE;
            break;
        }
    }

    ASSERT(Found == TRUE);
    Found = FALSE;

    // search for DIVE_AREA "over" sprite for reference point
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_DIVE_AREA], i, nexti)
    {
        over_sp = &sprite[i];

        if (TEST(sector[over_sp->sectnum].extra, SECTFX_DIVE_AREA) &&
            SectUser[over_sp->sectnum] &&
            SectUser[over_sp->sectnum]->number == sectu->number)
        {
            Found = TRUE;
            break;
        }
    }

    ASSERT(Found == TRUE);

    // get the offset from the under sprite
    sx = under_sp->x - *x;
    sy = under_sp->y - *y;

    // update to the new x y position
    *x = over_sp->x - sx;
    *y = over_sp->y - sy;

    over = over_sp->sectnum;
    under = under_sp->sectnum;

    if (GetOverlapSector(*x, *y, &over, &under))
    {
        *sectnum = over;
    }

    *z = sector[over_sp->sectnum].floorz - Z(2);

    //MissileWaterAdjust(sp - sprite);

    return TRUE;
}


SWBOOL
SpriteWarpToUnderwater(SPRITEp sp)
{
    USERp u = User[sp - sprite];
    short i, nexti;
    SECT_USERp sectu = SectUser[sp->sectnum];
    SPRITEp under_sp = NULL, over_sp = NULL;
    char Found = FALSE;
    short over, under;
    int sx, sy;

    // 0 not valid for water match tags
    if (sectu->number == 0)
        return FALSE;

    // search for DIVE_AREA "over" sprite for reference point
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_DIVE_AREA], i, nexti)
    {
        over_sp = &sprite[i];

        if (TEST(sector[over_sp->sectnum].extra, SECTFX_DIVE_AREA) &&
            SectUser[over_sp->sectnum] &&
            SectUser[over_sp->sectnum]->number == sectu->number)
        {
            Found = TRUE;
            break;
        }
    }

    ASSERT(Found == TRUE);
    Found = FALSE;

    // search for UNDERWATER "under" sprite for reference point
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_UNDERWATER], i, nexti)
    {
        under_sp = &sprite[i];

        if (TEST(sector[under_sp->sectnum].extra, SECTFX_UNDERWATER) &&
            SectUser[under_sp->sectnum] &&
            SectUser[under_sp->sectnum]->number == sectu->number)
        {
            Found = TRUE;
            break;
        }
    }

    ASSERT(Found == TRUE);

    // get the offset from the sprite
    sx = over_sp->x - sp->x;
    sy = over_sp->y - sp->y;

    // update to the new x y position
    sp->x = under_sp->x - sx;
    sp->y = under_sp->y - sy;

    over = over_sp->sectnum;
    under = under_sp->sectnum;

    if (GetOverlapSector(sp->x, sp->y, &over, &under) == 2)
    {
        changespritesect(sp - sprite, under);
    }
    else
    {
        changespritesect(sp - sprite, over);
    }

    //sp->z = sector[under_sp->sectnum].ceilingz + Z(6);
    sp->z = sector[under_sp->sectnum].ceilingz + u->ceiling_dist+Z(1);

    u->ox = sp->x;
    u->oy = sp->y;
    u->oz = sp->z;

    return TRUE;
}

SWBOOL
SpriteWarpToSurface(SPRITEp sp)
{
    USERp u = User[sp - sprite];
    short i, nexti;
    SECT_USERp sectu = SectUser[sp->sectnum];
    short over, under;
    int sx, sy;

    SPRITEp under_sp = NULL, over_sp = NULL;
    char Found = FALSE;

    // 0 not valid for water match tags
    if (sectu->number == 0)
        return FALSE;

    // search for UNDERWATER "under" sprite for reference point
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_UNDERWATER], i, nexti)
    {
        under_sp = &sprite[i];

        if (TEST(sector[under_sp->sectnum].extra, SECTFX_UNDERWATER) &&
            SectUser[under_sp->sectnum] &&
            SectUser[under_sp->sectnum]->number == sectu->number)
        {
            Found = TRUE;
            break;
        }
    }

    ASSERT(Found == TRUE);

    if (under_sp->lotag == 0)
        return FALSE;

    Found = FALSE;

    // search for DIVE_AREA "over" sprite for reference point
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_DIVE_AREA], i, nexti)
    {
        over_sp = &sprite[i];

        if (TEST(sector[over_sp->sectnum].extra, SECTFX_DIVE_AREA) &&
            SectUser[over_sp->sectnum] &&
            SectUser[over_sp->sectnum]->number == sectu->number)
        {
            Found = TRUE;
            break;
        }
    }

    ASSERT(Found == TRUE);

    // get the offset from the under sprite
    sx = under_sp->x - sp->x;
    sy = under_sp->y - sp->y;

    // update to the new x y position
    sp->x = over_sp->x - sx;
    sp->y = over_sp->y - sy;

    over = over_sp->sectnum;
    under = under_sp->sectnum;

    if (GetOverlapSector(sp->x, sp->y, &over, &under))
    {
        changespritesect(sp - sprite, over);
    }

    sp->z = sector[over_sp->sectnum].floorz - Z(2);

    // set z range and wade depth so we know how high to set view
    DoActorZrange(sp - sprite);
    MissileWaterAdjust(sp - sprite);


    u->ox = sp->x;
    u->oy = sp->y;
    u->oz = sp->z;

    return TRUE;
}


int
SpawnSplash(short SpriteNum)
{
    USERp u = User[SpriteNum], wu;
    SPRITEp sp = User[SpriteNum]->SpriteP, wp;
    short w;

    SECT_USERp sectu = SectUser[sp->sectnum];
    SECTORp sectp = &sector[sp->sectnum];

    if (Prediction)
        return 0;

    if (sectu && (TEST(sectp->extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_NONE))
        return 0;

    if (sectu && TEST(sectp->floorstat, FLOOR_STAT_PLAX))
        return 0;

    PlaySound(DIGI_SPLASH1, &sp->x, &sp->y, &sp->z, v3df_none);

    DoActorZrange(SpriteNum);
    MissileWaterAdjust(SpriteNum);

    w = SpawnSprite(STAT_MISSILE, SPLASH, s_Splash, sp->sectnum, sp->x, sp->y, u->loz, sp->ang, 0);
    wp = &sprite[w];
    wu = User[w];

    if (sectu && TEST(sectp->extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
        wu->spal = wp->pal = PALETTE_RED_LIGHTING;

    wp->xrepeat = 45;
    wp->yrepeat = 42;
    wp->shade = sector[sp->sectnum].floorshade - 10;

    return 0;
}

int
SpawnSplashXY(int hit_x, int hit_y, int hit_z, short sectnum)
{
    USERp wu;
    SPRITEp wp;
    short w;
    //short sectnum=0;

    SECT_USERp sectu;
    SECTORp sectp;

    if (Prediction)
        return 0;

    sectu = SectUser[sectnum];
    sectp = &sector[sectnum];

    if (sectu && (TEST(sectp->extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_NONE))
        return 0;

    if (sectu && TEST(sectp->floorstat, FLOOR_STAT_PLAX))
        return 0;

    w = SpawnSprite(STAT_MISSILE, SPLASH, s_Splash, sectnum, hit_x, hit_y, hit_z, 0, 0);
    wp = &sprite[w];
    wu = User[w];

    if (sectu && TEST(sectp->extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
        wu->spal = wp->pal = PALETTE_RED_LIGHTING;

    wp->xrepeat = 45;
    wp->yrepeat = 42;
    wp->shade = sector[wp->sectnum].floorshade - 10;

    return 0;
}

int
SpawnUnderSplash(short SpriteNum)
{
    USERp u = User[SpriteNum], wu;
    SPRITEp sp = User[SpriteNum]->SpriteP, wp;
    short w;

    SECT_USERp sectu = SectUser[sp->sectnum];
    SECTORp sectp = &sector[sp->sectnum];

    return 0;

    if (Prediction)
        return 0;

    if (sectu && (TEST(sectp->extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_NONE))
        return 0;

    DoActorZrange(SpriteNum);
    MissileWaterAdjust(SpriteNum);

    w = SpawnSprite(STAT_MISSILE, SPLASH, s_Splash, sp->sectnum, sp->x, sp->y, u->hiz, sp->ang, 0);
    wp = &sprite[w];
    wu = User[w];

    if (sectu && TEST(sectp->extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
        wu->spal = wp->pal = PALETTE_RED_LIGHTING;

    wp->xrepeat = 72;
    wp->yrepeat = 90;
    wp->shade = sector[sp->sectnum].floorshade - 10;
    SET(wp->cstat, CSTAT_SPRITE_YFLIP);

    return 0;
}

SWBOOL
MissileHitDiveArea(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    SECTORp sectp;

    // correctly set underwater bit for missiles
    // in Stacked water areas.
    if (FAF_ConnectArea(sp->sectnum))
    {
        if (SectorIsUnderwaterArea(sp->sectnum))
            SET(u->Flags, SPR_UNDERWATER);
        else
            RESET(u->Flags, SPR_UNDERWATER);
    }

    if (!u->ret)
        return FALSE;

    switch (TEST(u->ret, HIT_MASK))
    {
    case HIT_SECTOR:
    {
        short hit_sect = NORM_SECTOR(u->ret);

        if (SpriteInDiveArea(sp))
        {
            // make sure you are close to the floor
            if (sp->z < DIV2(u->hiz + u->loz))
                return FALSE;

            // Check added by Jim because of sprite bridge over water
            if (sp->z < (sector[hit_sect].floorz-Z(20)))
                return FALSE;

            SET(u->Flags, SPR_UNDERWATER);
            SpawnSplash(sp - sprite);
            SpriteWarpToUnderwater(sp);
            //SpawnUnderSplash(sp - sprite);
            u->ret = 0;
            PlaySound(DIGI_PROJECTILEWATERHIT, &sp->x, &sp->y, &sp->z, v3df_none);
            return TRUE;
        }
        else if (SpriteInUnderwaterArea(sp))
        {
            // make sure you are close to the ceiling
            if (sp->z > DIV2(u->hiz + u->loz))
                return FALSE;

            RESET(u->Flags, SPR_UNDERWATER);
            if (!SpriteWarpToSurface(sp))
            {
                return FALSE;
            }
            SpawnSplash(sp - sprite);
            u->ret = 0;
            return TRUE;
        }

        break;
    }
    }

    return FALSE;
}

int
SpawnBubble(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum], bp;
    USERp u = User[SpriteNum], bu;
    short b;

    if (Prediction)
        return -1;

    b = SpawnSprite(STAT_MISSILE, BUBBLE, s_Bubble, sp->sectnum, sp->x, sp->y, sp->z, sp->ang, 0);
    bp = &sprite[b];
    bu = User[b];

    //PlaySound(DIGI_BUBBLES, &sp->x, &sp->y, &sp->z, v3df_none);

    bp->xrepeat = 8 + (RANDOM_P2(8 << 8) >> 8);
    bp->yrepeat = bp->xrepeat;
    bu->sx = bp->xrepeat;
    bu->sy = bp->yrepeat;
    bu->ceiling_dist = Z(1);
    bu->floor_dist = Z(1);
    bp->shade = sector[sp->sectnum].floorshade - 10;
    bu->WaitTics = 120 * 120;
    bp->zvel = 512;
    bp->clipdist = 12 >> 2;
    SET(bp->cstat, CSTAT_SPRITE_YCENTER);
    SET(bu->Flags, SPR_UNDERWATER);
    bp->shade = -60; // Make em brighter

    return b;
}

int
DoVehicleSmoke(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    sp->z -= sp->zvel;

    sp->x += u->xchange;
    sp->y += u->ychange;


    return FALSE;
}

int
DoWaterSmoke(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    sp->z -= sp->zvel;

    return FALSE;
}

int
SpawnVehicleSmoke(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum],np;
    USERp u = User[SpriteNum],nu;
    short New;

    if (MoveSkip2 != 0)
        return FALSE;

    New = SpawnSprite(STAT_MISSILE, PUFF, s_VehicleSmoke, sp->sectnum,
                      sp->x, sp->y, sp->z - RANDOM_P2(Z(8)), sp->ang, 0);

    np = &sprite[New];
    nu = User[New];

    nu->WaitTics = 1*120;
    np->shade = -40;
    np->xrepeat = 64;
    np->yrepeat = 64;
    SET(np->cstat, CSTAT_SPRITE_YCENTER);
    RESET(np->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    if (RANDOM_P2(1024) < 512)
        SET(np->cstat, CSTAT_SPRITE_XFLIP);
    if (RANDOM_P2(1024) < 512)
        SET(np->cstat, CSTAT_SPRITE_YFLIP);

    np->ang = RANDOM_P2(2048);
    np->xvel = RANDOM_P2(32);
    nu->xchange = MOVEx(np->xvel, np->ang);
    nu->ychange = MOVEy(np->xvel, np->ang);
    np->zvel = Z(4) + RANDOM_P2(Z(4));

    return FALSE;
}

int
SpawnSmokePuff(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum],np;
    USERp u = User[SpriteNum],nu;
    short New;

    New = SpawnSprite(STAT_MISSILE, PUFF, s_WaterSmoke, sp->sectnum,
                      sp->x, sp->y, sp->z - RANDOM_P2(Z(8)), sp->ang, 0);

    np = &sprite[New];
    nu = User[New];

    nu->WaitTics = 1*120;
    np->shade = -40;
    np->xrepeat = 64;
    np->yrepeat = 64;
    SET(np->cstat, CSTAT_SPRITE_YCENTER);
    RESET(np->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    if (RANDOM_P2(1024) < 512)
        SET(np->cstat, CSTAT_SPRITE_XFLIP);
    if (RANDOM_P2(1024) < 512)
        SET(np->cstat, CSTAT_SPRITE_YFLIP);

    np->ang = RANDOM_P2(2048);
    np->xvel = RANDOM_P2(32);
    nu->xchange = MOVEx(np->xvel, np->ang);
    nu->ychange = MOVEy(np->xvel, np->ang);
    //np->zvel = Z(4) + RANDOM_P2(Z(4));
    np->zvel = Z(1) + RANDOM_P2(Z(2));

    return FALSE;
}


int
DoBubble(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    sp->z -= sp->zvel;
    sp->zvel += 32;

    if (sp->zvel > 768)
        sp->zvel = 768;

    u->sx += 1;
    u->sy += 1;

    if (u->sx > 32)
    {
        u->sx = 32;
        u->sy = 32;
    }

    sp->xrepeat = u->sx + (RANDOM_P2(8 << 8) >> 8) - 4;
    sp->yrepeat = u->sy + (RANDOM_P2(8 << 8) >> 8) - 4;

    if (sp->z < sector[sp->sectnum].ceilingz)
    {
        if (SectorIsUnderwaterArea(u->hi_sectp - sector))
        {
            if (!SpriteWarpToSurface(sp))
            {
                KillSprite(SpriteNum);
                return TRUE;
            }

            RESET(u->Flags, SPR_UNDERWATER);
            // stick around above water for this long
            u->WaitTics = (RANDOM_P2(64 << 8) >> 8);
        }
        else
        {
            KillSprite(SpriteNum);
            return TRUE;
        }
    }

    if (!TEST(u->Flags, SPR_UNDERWATER))
    {
        if ((u->WaitTics -= MISSILEMOVETICS) <= 0)
        {
            KillSprite(SpriteNum);
            return TRUE;
        }
    }
    else
    // just in case its stuck somewhere kill it after a while
    {
        if ((u->WaitTics -= MISSILEMOVETICS) <= 0)
        {
            KillSprite(SpriteNum);
            return TRUE;
        }
    }

    return FALSE;
}

// this needs to be called before killsprite
// whenever killing a sprite that you aren't completely sure what it is, like
// with the drivables, copy sectors, break sprites, etc
void
SpriteQueueDelete(short SpriteNum)
{
    int i;

    for (i = 0; i < MAX_STAR_QUEUE; i++)
        if (StarQueue[i] == SpriteNum)
            StarQueue[i] = -1;

    for (i = 0; i < MAX_HOLE_QUEUE; i++)
        if (HoleQueue[i] == SpriteNum)
            HoleQueue[i] = -1;

    for (i = 0; i < MAX_WALLBLOOD_QUEUE; i++)
        if (WallBloodQueue[i] == SpriteNum)
            WallBloodQueue[i] = -1;

    for (i = 0; i < MAX_FLOORBLOOD_QUEUE; i++)
        if (FloorBloodQueue[i] == SpriteNum)
            FloorBloodQueue[i] = -1;

    for (i = 0; i < MAX_GENERIC_QUEUE; i++)
        if (GenericQueue[i] == SpriteNum)
            GenericQueue[i] = -1;

    for (i = 0; i < MAX_LOWANGS_QUEUE; i++)
        if (LoWangsQueue[i] == SpriteNum)
            LoWangsQueue[i] = -1;
}


void QueueReset(void)
{
    short i;
    StarQueueHead=0;
    HoleQueueHead=0;
    WallBloodQueueHead=0;
    FloorBloodQueueHead=0;
    GenericQueueHead=0;
    LoWangsQueueHead=0;


    for (i = 0; i < MAX_STAR_QUEUE; i++)
        StarQueue[i] = -1;

    for (i = 0; i < MAX_HOLE_QUEUE; i++)
        HoleQueue[i] = -1;

    for (i = 0; i < MAX_WALLBLOOD_QUEUE; i++)
        WallBloodQueue[i] = -1;

    for (i = 0; i < MAX_FLOORBLOOD_QUEUE; i++)
        FloorBloodQueue[i] = -1;

    for (i = 0; i < MAX_GENERIC_QUEUE; i++)
        GenericQueue[i] = -1;

    for (i = 0; i < MAX_LOWANGS_QUEUE; i++)
        LoWangsQueue[i] = -1;
}

SWBOOL TestDontStick(short SpriteNum, short hit_sect, short hit_wall, int hit_z)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    WALLp wp;

    if (hit_wall < 0)
    {
        ASSERT(SpriteNum>=0);
        hit_wall = NORM_WALL(u->ret);
        hit_sect = sp->sectnum;
    }

    wp = &wall[hit_wall];

    if (TEST(wp->extra, WALLFX_DONT_STICK))
        return TRUE;

    // if blocking red wallo
    if (TEST(wp->cstat, CSTAT_WALL_BLOCK) && wp->nextwall >= 0)
        return TRUE;

    return FALSE;
}

SWBOOL TestDontStickSector(short hit_sect)
{
    if (TEST(sector[hit_sect].extra, SECTFX_DYNAMIC_AREA|SECTFX_SECTOR_OBJECT))
        return TRUE;

    return FALSE;
}

int QueueStar(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    SPRITEp osp;

    if (TestDontStick(SpriteNum, -1, -1, sp->z))
    {
        KillSprite(SpriteNum);
        return -1;
    }

    // can and should kill the user portion of the star
    if (StarQueue[StarQueueHead] == -1)
    {
        // new star
        if (User[SpriteNum])
        {
            FreeMem(User[SpriteNum]);
            User[SpriteNum] = NULL;
        }
        change_sprite_stat(SpriteNum, STAT_STAR_QUEUE);
        StarQueue[StarQueueHead] = SpriteNum;
    }
    else
    {
        // move old star to new stars place
        osp = &sprite[StarQueue[StarQueueHead]];
        osp->x = sp->x;
        osp->y = sp->y;
        osp->z = sp->z;
        changespritesect(StarQueue[StarQueueHead], sp->sectnum);
        KillSprite(SpriteNum);
        SpriteNum = StarQueue[StarQueueHead];
        ASSERT(sprite[SpriteNum].statnum != MAXSTATUS);
    }

    StarQueueHead = (StarQueueHead+1) & (MAX_STAR_QUEUE-1);

    return SpriteNum;
}

int QueueHole(short ang, short hit_sect, short hit_wall, int hit_x, int hit_y, int hit_z)
{
    short w,nw,wall_ang,dang;
    short SpriteNum;
    int nx,ny;
    SPRITEp sp;
    short sectnum;


    if (TestDontStick(-1,hit_sect,hit_wall,hit_z))
        return -1;

    if (HoleQueue[HoleQueueHead] == -1)
        HoleQueue[HoleQueueHead] = SpriteNum = COVERinsertsprite(hit_sect, STAT_HOLE_QUEUE);
    else
        SpriteNum = HoleQueue[HoleQueueHead];

    HoleQueueHead = (HoleQueueHead+1) & (MAX_HOLE_QUEUE-1);

    sp = &sprite[SpriteNum];
    sp->owner = -1;
    sp->xrepeat = sp->yrepeat = 16;
    sp->cstat = sp->pal = sp->shade = sp->extra = sp->clipdist = sp->xoffset = sp->yoffset = 0;
    sp->x = hit_x;
    sp->y = hit_y;
    sp->z = hit_z;
    sp->picnum = 2151;
    changespritesect(SpriteNum, hit_sect);

    ASSERT(sp->statnum != MAXSTATUS);

    SET(sp->cstat, CSTAT_SPRITE_WALL);
    SET(sp->cstat, CSTAT_SPRITE_ONE_SIDE);
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    w = hit_wall;
    nw = wall[w].point2;
    wall_ang = NORM_ANGLE(getangle(wall[nw].x - wall[w].x, wall[nw].y - wall[w].y)+512);
    sp->ang = wall_ang;

    // move it back some
    nx = sintable[(512 + sp->ang) & 2047]<<4;
    ny = sintable[sp->ang]<<4;

    sectnum = sp->sectnum;

    clipmoveboxtracenum = 1;
    clipmove((vec3_t *)sp, &sectnum, nx, ny,
             0L, 0L, 0L, CLIPMASK_MISSILE);
    clipmoveboxtracenum = 3;

    if (sp->sectnum != sectnum)
        changespritesect(SpriteNum, sectnum);

    return SpriteNum;
}

#define FLOORBLOOD1 389
#define FLOORBLOOD_RATE 30
ANIMATOR DoFloorBlood;
STATE s_FloorBlood1[] =
{
    {FLOORBLOOD1, SF_QUICK_CALL,   DoFloorBlood, &s_FloorBlood1[1]},
    {FLOORBLOOD1, FLOORBLOOD_RATE, NullAnimator, &s_FloorBlood1[0]},
};

int QueueFloorBlood(short hit_sprite)
{
    SPRITEp hsp = &sprite[hit_sprite];
    short w,nw,wall_ang,dang;
    short SpriteNum;
    int nx,ny;
    SPRITEp sp;
    short sectnum;
    USERp u = User[hit_sprite];
    SECTORp sectp = &sector[hsp->sectnum];


    if (TEST(sectp->extra, SECTFX_SINK)||TEST(sectp->extra, SECTFX_CURRENT))
        return -1;   // No blood in water or current areas

    if (TEST(u->Flags, SPR_UNDERWATER) || SpriteInUnderwaterArea(hsp) || SpriteInDiveArea(hsp))
        return -1;   // No blood underwater!

    if (TEST(sector[hsp->sectnum].extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_WATER)
        return -1;   // No prints liquid areas!

    if (TEST(sector[hsp->sectnum].extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
        return -1;   // Not in lave either

    if (TestDontStickSector(hsp->sectnum))
        return -1;   // Not on special sectors you don't

    if (FloorBloodQueue[FloorBloodQueueHead] != -1)
        KillSprite(FloorBloodQueue[FloorBloodQueueHead]);

    FloorBloodQueue[FloorBloodQueueHead] = SpriteNum =
                                               SpawnSprite(STAT_SKIP4, FLOORBLOOD1, s_FloorBlood1, hsp->sectnum, hsp->x, hsp->y, hsp->z, hsp->ang, 0);

    FloorBloodQueueHead = (FloorBloodQueueHead+1) & (MAX_FLOORBLOOD_QUEUE-1);

    sp = &sprite[SpriteNum];
    sp->owner = -1;
    // Stupid hack to fix the blood under the skull to not show through
    // x,y repeat of floor blood MUST be smaller than the sprite above it or clipping probs.
    if (u->ID == GORE_Head)
        sp->hitag = 9995;
    else
        sp->hitag = 0;
    sp->xrepeat = sp->yrepeat = 8;
    sp->cstat = sp->pal = sp->shade = sp->extra = sp->clipdist = sp->xoffset = sp->yoffset = 0;
    sp->x = hsp->x;
    sp->y = hsp->y;
    sp->z = hsp->z + Z(1);
    sp->ang = RANDOM_P2(2048); // Just make it any old angle
    sp->shade -= 5;  // Brighten it up just a bit

    SET(sp->cstat, CSTAT_SPRITE_FLOOR);
    SET(sp->cstat, CSTAT_SPRITE_ONE_SIDE);
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    RESET(u->Flags, SPR_SHADOW);

    return SpriteNum;
}

#define FOOTPRINT1 2490
#define FOOTPRINT2 2491
#define FOOTPRINT3 2492
#define FOOTPRINT_RATE 30
ANIMATOR DoFootPrint;
STATE s_FootPrint1[] =
{
    {FOOTPRINT1, FOOTPRINT_RATE, NullAnimator, &s_FootPrint1[0]},
};
STATE s_FootPrint2[] =
{
    {FOOTPRINT2, FOOTPRINT_RATE, NullAnimator, &s_FootPrint2[0]},
};
STATE s_FootPrint3[] =
{
    {FOOTPRINT3, FOOTPRINT_RATE, NullAnimator, &s_FootPrint3[0]},
};

int QueueFootPrint(short hit_sprite)
{
    SPRITEp hsp = &sprite[hit_sprite];
    short w,nw,wall_ang,dang;
    short SpriteNum;
    int nx,ny;
    SPRITEp sp;
    short sectnum;
    USERp u = User[hit_sprite];
    USERp nu;
    short rnd_num=0;
    SWBOOL Found=FALSE;
    SPRITEp under_sp;
    short i, nexti;
    SECT_USERp sectu = SectUser[hsp->sectnum];
    SECTORp sectp = &sector[hsp->sectnum];


    if (TEST(sectp->extra, SECTFX_SINK)||TEST(sectp->extra, SECTFX_CURRENT))
        return -1;   // No blood in water or current areas

    if (u->PlayerP)
    {
        if (TEST(u->PlayerP->Flags, PF_DIVING))
            Found = TRUE;

        // Stupid masked floor stuff!  Damn your weirdness!
        if (TEST(sector[u->PlayerP->cursectnum].ceilingstat, CEILING_STAT_PLAX))
            Found = TRUE;
        if (TEST(sector[u->PlayerP->cursectnum].floorstat, CEILING_STAT_PLAX))
            Found = TRUE;
    }

    if (TEST(u->Flags, SPR_UNDERWATER) || SpriteInUnderwaterArea(hsp) || Found || SpriteInDiveArea(hsp))
        return -1;   // No prints underwater!

    if (TEST(sector[hsp->sectnum].extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_WATER)
        return -1;   // No prints liquid areas!

    if (TEST(sector[hsp->sectnum].extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
        return -1;   // Not in lave either

    if (TestDontStickSector(hsp->sectnum))
        return -1;   // Not on special sectors you don't

    // So, are we like, done checking now!?
    if (FloorBloodQueue[FloorBloodQueueHead] != -1)
        KillSprite(FloorBloodQueue[FloorBloodQueueHead]);

    rnd_num = RANDOM_RANGE(1024);

    if (rnd_num > 683)
        FloorBloodQueue[FloorBloodQueueHead] = SpriteNum =
                                                   SpawnSprite(STAT_WALLBLOOD_QUEUE, FOOTPRINT1, s_FootPrint1, hsp->sectnum, hsp->x, hsp->y, hsp->z, hsp->ang, 0);
    else if (rnd_num > 342)
        FloorBloodQueue[FloorBloodQueueHead] = SpriteNum =
                                                   SpawnSprite(STAT_WALLBLOOD_QUEUE, FOOTPRINT2, s_FootPrint2, hsp->sectnum, hsp->x, hsp->y, hsp->z, hsp->ang, 0);
    else
        FloorBloodQueue[FloorBloodQueueHead] = SpriteNum =
                                                   SpawnSprite(STAT_WALLBLOOD_QUEUE, FOOTPRINT3, s_FootPrint3, hsp->sectnum, hsp->x, hsp->y, hsp->z, hsp->ang, 0);

    FloorBloodQueueHead = (FloorBloodQueueHead+1) & (MAX_FLOORBLOOD_QUEUE-1);

    // Decrease footprint count
    if (u->PlayerP)
        u->PlayerP->NumFootPrints--;


    sp = &sprite[SpriteNum];
    nu = User[SpriteNum];
    sp->hitag = 0;
    sp->owner = -1;
    sp->xrepeat = 48;
    sp->yrepeat = 54;
    sp->cstat = sp->pal = sp->shade = sp->extra = sp->clipdist = sp->xoffset = sp->yoffset = 0;
    sp->x = hsp->x;
    sp->y = hsp->y;
    sp->z = hsp->z;
    sp->ang = hsp->ang;
    RESET(nu->Flags, SPR_SHADOW);
    switch (FootMode)
    {
    case BLOOD_FOOT:
        nu->spal = sp->pal = PALETTE_PLAYER3; // Turn blue to blood red
        break;
    default:
        nu->spal = sp->pal = PALETTE_PLAYER1; // Gray water
        break;
    }


    // Alternate the feet
    left_foot = !left_foot;
    if (left_foot)
        SET(sp->cstat, CSTAT_SPRITE_XFLIP);
    SET(sp->cstat, CSTAT_SPRITE_FLOOR);
    SET(sp->cstat, CSTAT_SPRITE_ONE_SIDE);
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    return SpriteNum;
}

#define WALLBLOOD1  2500
#define WALLBLOOD2  2501
#define WALLBLOOD3  2502
#define WALLBLOOD4  2503
#define WALLBLOOD_RATE 30
ANIMATOR DoWallBlood;
STATE s_WallBlood1[] =
{
    {WALLBLOOD1, SF_QUICK_CALL, DoWallBlood, &s_WallBlood1[1]},
    {WALLBLOOD1, WALLBLOOD_RATE, NullAnimator, &s_WallBlood1[0]},
};
STATE s_WallBlood2[] =
{
    {WALLBLOOD2, SF_QUICK_CALL, DoWallBlood, &s_WallBlood2[1]},
    {WALLBLOOD2, WALLBLOOD_RATE, NullAnimator, &s_WallBlood2[0]},
};
STATE s_WallBlood3[] =
{
    {WALLBLOOD3, SF_QUICK_CALL, DoWallBlood, &s_WallBlood3[1]},
    {WALLBLOOD3, WALLBLOOD_RATE, NullAnimator, &s_WallBlood3[0]},
};
STATE s_WallBlood4[] =
{
    {WALLBLOOD4, SF_QUICK_CALL, DoWallBlood, &s_WallBlood4[1]},
    {WALLBLOOD4, WALLBLOOD_RATE, NullAnimator, &s_WallBlood4[0]},
};


int QueueWallBlood(short hit_sprite, short ang)
{
    SPRITEp hsp = &sprite[hit_sprite];
    short w,nw,wall_ang,dang;
    short SpriteNum;
    int nx,ny;
    SPRITEp sp;
    short sectnum;
    short rndnum;
    int daz;
    hitdata_t hitinfo;
    USERp u = User[hit_sprite];


    if (TEST(u->Flags, SPR_UNDERWATER) || SpriteInUnderwaterArea(hsp) || SpriteInDiveArea(hsp))
        return -1;   // No blood underwater!

    daz = Z(RANDOM_P2(128))<<3;
    daz -= DIV2(Z(128)<<3);
    dang = (ang+(RANDOM_P2(128<<5) >> 5)) - DIV2(128);

    FAFhitscan(hsp->x, hsp->y, hsp->z - Z(30), hsp->sectnum,    // Start position
               sintable[NORM_ANGLE(dang + 512)],  // X vector of 3D ang
               sintable[NORM_ANGLE(dang)],                             // Y vector of 3D ang
               daz,                                                    // Z vector of 3D ang
               &hitinfo, CLIPMASK_MISSILE);

    if (hitinfo.sect < 0)
        return -1;

#define WALLBLOOD_DIST_MAX 2500
    if (Distance(hitinfo.pos.x, hitinfo.pos.y, hsp->x, hsp->y) > WALLBLOOD_DIST_MAX)
        return -1;

    // hit a sprite?
    if (hitinfo.sprite >= 0)
        return 0;   // Don't try to put blood on a sprite

    if (hitinfo.wall >= 0)   // Don't check if blood didn't hit a wall, otherwise the ASSERT fails!
    {
        if (TestDontStick(-1, hitinfo.sect, hitinfo.wall, hitinfo.pos.z))
            return -1;
    }
    else
        return -1;


    if (WallBloodQueue[WallBloodQueueHead] != -1)
        KillSprite(WallBloodQueue[WallBloodQueueHead]);

    // Randomly choose a wall blood sprite
    rndnum = RANDOM_RANGE(1024);
    if (rndnum > 768)
    {
        WallBloodQueue[WallBloodQueueHead] = SpriteNum =
                                                 SpawnSprite(STAT_WALLBLOOD_QUEUE, WALLBLOOD1, s_WallBlood1, hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, ang, 0);
    }
    else if (rndnum > 512)
    {
        WallBloodQueue[WallBloodQueueHead] = SpriteNum =
                                                 SpawnSprite(STAT_WALLBLOOD_QUEUE, WALLBLOOD2, s_WallBlood2, hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, ang, 0);
    }
    else if (rndnum > 128)
    {
        WallBloodQueue[WallBloodQueueHead] = SpriteNum =
                                                 SpawnSprite(STAT_WALLBLOOD_QUEUE, WALLBLOOD3, s_WallBlood3, hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, ang, 0);
    }
    else
    {
        WallBloodQueue[WallBloodQueueHead] = SpriteNum =
                                                 SpawnSprite(STAT_WALLBLOOD_QUEUE, WALLBLOOD4, s_WallBlood4, hitinfo.sect, hitinfo.pos.x, hitinfo.pos.y, hitinfo.pos.z, ang, 0);
    }

    WallBloodQueueHead = (WallBloodQueueHead+1) & (MAX_WALLBLOOD_QUEUE-1);

    sp = &sprite[SpriteNum];
    sp->owner = -1;
    sp->xrepeat = 30;
    sp->yrepeat = 40;   // yrepeat will grow towards 64, it's default size
    sp->cstat = sp->pal = sp->shade = sp->extra = sp->clipdist = sp->xoffset = sp->yoffset = 0;
    sp->x = hitinfo.pos.x;
    sp->y = hitinfo.pos.y;
    sp->z = hitinfo.pos.z;
    sp->shade -= 5;  // Brighten it up just a bit
    sp->yvel = hitinfo.wall; // pass hitinfo.wall in yvel

    SET(sp->cstat, CSTAT_SPRITE_WALL);
    SET(sp->cstat, CSTAT_SPRITE_ONE_SIDE);
    SET(sp->cstat, CSTAT_SPRITE_YCENTER);
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    w = hitinfo.wall;
    nw = wall[w].point2;
    wall_ang = NORM_ANGLE(getangle(wall[nw].x - wall[w].x, wall[nw].y - wall[w].y)+512);
    sp->ang = wall_ang;

    // move it back some
    nx = sintable[(512 + sp->ang) & 2047]<<4;
    ny = sintable[sp->ang]<<4;

    sectnum = sp->sectnum;

    clipmoveboxtracenum = 1;
    clipmove((vec3_t *)sp, &sectnum, nx, ny,
             0L, 0L, 0L, CLIPMASK_MISSILE);
    clipmoveboxtracenum = 3;

    if (sp->sectnum != sectnum)
        changespritesect(SpriteNum, sectnum);

    return SpriteNum;
}

#define FEET_IN_BLOOD_DIST 300
int
DoFloorBlood(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    SPRITEp psp = User[SpriteNum]->SpriteP;
    int dist, near_dist = FEET_IN_BLOOD_DIST, a,b,c;
    short pnum;
    PLAYERp pp;
    short xsiz,ysiz;


    if (sp->hitag == 9995)
    {
        xsiz = 12;
        ysiz = 12;
    }
    else
    {
        xsiz = 40;
        ysiz = 40;
    }

    // Make pool of blood seem to grow
    if (sp->xrepeat < xsiz && sp->xrepeat != 4)
    {
        sp->xrepeat++;
    }

    if (sp->yrepeat < ysiz && sp->xrepeat != xsiz && sp->xrepeat != 4)
    {
        sp->yrepeat++;
    }

    // See if any players stepped in blood
    if (sp->xrepeat != 4 && sp->yrepeat > 4)
    {
        TRAVERSE_CONNECT(pnum)
        {
            pp = &Player[pnum];

            DISTANCE(psp->x, psp->y, pp->posx, pp->posy, dist, a, b, c);

//      //DSPRINTF(ds,"dist = %ld\n",dist);
//      MONO_PRINT(ds);

            if (dist < near_dist)
            {
                if (pp->NumFootPrints <= 0 || FootMode != BLOOD_FOOT)
                {
                    pp->NumFootPrints = RANDOM_RANGE(10)+3;
                    FootMode = BLOOD_FOOT;
                }

                // If blood has already grown to max size, we can shrink it
                if (sp->xrepeat == 40 && sp->yrepeat > 10)
                {
                    sp->yrepeat -= 10;
                    if (sp->yrepeat <= 10)  // Shrink it down and don't use it anymore
                        sp->xrepeat = sp->yrepeat = 4;
                }
//          //DSPRINTF(ds,"pp->NumFootPrints = %d\n",pp->NumFootPrints);
//          MONO_PRINT(ds);
            }
        }
    }

    return 0;
}

int
DoWallBlood(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    // Make blood drip down the wall
    if (sp->yrepeat < 80)
    {
        sp->yrepeat++;
        sp->z += 128;
    }

    return 0;
}

// This is the FAST queue, it doesn't call any animator functions or states
int QueueGeneric(short SpriteNum, short pic)
{
    SPRITEp sp = &sprite[SpriteNum];
    SPRITEp osp;
    short xrepeat,yrepeat;

    if (TEST(sector[sp->sectnum].extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_WATER)
    {
        KillSprite(SpriteNum);
        return -1;
    }

    if (TEST(sector[sp->sectnum].extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
    {
        KillSprite(SpriteNum);
        return -1;
    }

    if (TestDontStickSector(sp->sectnum))
    {
        KillSprite(SpriteNum);
        return -1;
    }

    xrepeat = sp->xrepeat;
    yrepeat = sp->yrepeat;

    // can and should kill the user portion
    if (GenericQueue[GenericQueueHead] == -1)
    {
        if (User[SpriteNum])
        {
            FreeMem(User[SpriteNum]);
            User[SpriteNum] = NULL;
        }
        change_sprite_stat(SpriteNum, STAT_GENERIC_QUEUE);
        GenericQueue[GenericQueueHead] = SpriteNum;
    }
    else
    {
        // move old sprite to new sprite's place
        osp = &sprite[GenericQueue[GenericQueueHead]];
        //setspritez(GenericQueue[GenericQueueHead], (vec3_t *)sp);
        osp->x = sp->x;
        osp->y = sp->y;
        osp->z = sp->z;
        changespritesect(GenericQueue[GenericQueueHead], sp->sectnum);
        KillSprite(SpriteNum);
        SpriteNum = GenericQueue[GenericQueueHead];
        ASSERT(sprite[SpriteNum].statnum != MAXSTATUS);
    }

    sp = &sprite[SpriteNum];
    ASSERT(sp);
    sp->picnum = pic;
    sp->xrepeat = xrepeat;
    sp->yrepeat = yrepeat;
    sp->cstat = 0;
    switch (sp->picnum)
    {
    case 900:
    case 901:
    case 902:
    case 915:
    case 916:
    case 917:
    case 930:
    case 931:
    case 932:
    case GORE_Head:
        change_sprite_stat(SpriteNum,STAT_DEFAULT); // Breakable
        SET(sp->cstat, CSTAT_SPRITE_BREAKABLE);
        SET(sp->extra, SPRX_BREAKABLE);
        break;
    default:
        RESET(sp->cstat, CSTAT_SPRITE_BREAKABLE);
        RESET(sp->extra, SPRX_BREAKABLE);
        break;
    }

    GenericQueueHead = (GenericQueueHead+1) & (MAX_GENERIC_QUEUE-1);

    return SpriteNum;
}

#if 0
int
DoShellShrap(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    // If the shell doesn't fall in the allowable range, kill it.
    if (u->ShellNum < (ShellCount-MAXSHELLS))
    {
        KillSprite(SpriteNum);
        return 0;
    }

    // Get rid of shell if they fall in non-divable liquid areas
    if (TEST(sector[sp->sectnum].extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_WATER)
    {
        KillSprite(SpriteNum);
        return 0;
    }

    if (TEST(sector[sp->sectnum].extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
    {
        KillSprite(SpriteNum);
        return 0;
    }

    return 0;
}
#endif

int
SpawnShell(short SpriteNum, short ShellNum)
{
    extern int InitShell(int16_t SpriteNum, int16_t ShellNum);

    //ShellCount++;
    //SpawnShrap(SpriteNum,ShellNum);         // -2 signifies right Uzi shell
    // -3 is left Uzi
    // -4 is Shotgun shell

    InitShell(SpriteNum, ShellNum);
    return TRUE;
}


int
DoShrapVelocity(int16_t SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    int32_t dax, day, daz;

    if (TEST(u->Flags, SPR_UNDERWATER) || SpriteInUnderwaterArea(sp))
    {
        ScaleSpriteVector(SpriteNum, 20000);

        u->Counter += 8*4;      // These are MoveSkip4 now
        u->zchange += u->Counter;
    }
    else
    {
        u->Counter += 60*4;
        u->zchange += u->Counter;
    }

    u->ret = move_missile(SpriteNum, u->xchange, u->ychange, u->zchange,
                          u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS*2);

    MissileHitDiveArea(SpriteNum);

    if (u->ret)
    {
        switch (TEST(u->ret, HIT_MASK))
        {
        case HIT_PLAX_WALL:
            KillSprite(SpriteNum);
            return TRUE;
        case HIT_SPRITE:
        {
            short wall_ang, dang;
            short hit_sprite = -2;
            SPRITEp hsp;
//                PlaySound(DIGI_DHCLUNK, &sp->x, &sp->y, &sp->z, v3df_dontpan);

            hit_sprite = NORM_SPRITE(u->ret);
            hsp = &sprite[hit_sprite];

            wall_ang = NORM_ANGLE(hsp->ang);
            WallBounce(SpriteNum, wall_ang);
            ScaleSpriteVector(SpriteNum, 32000);

            break;
        }

        case HIT_WALL:
        {
            short hit_wall,nw,wall_ang,dang;
            WALLp wph;


            hit_wall = NORM_WALL(u->ret);
            wph = &wall[hit_wall];


//                PlaySound(DIGI_DHCLUNK, &sp->x, &sp->y, &sp->z, v3df_dontpan);

            nw = wall[hit_wall].point2;
            wall_ang = NORM_ANGLE(getangle(wall[nw].x - wph->x, wall[nw].y - wph->y)+512);

            WallBounce(SpriteNum, wall_ang);
            ScaleSpriteVector(SpriteNum, 32000);
            break;
        }

        case HIT_SECTOR:
        {
            SWBOOL did_hit_wall;

            if (SlopeBounce(SpriteNum, &did_hit_wall))
            {
                if (did_hit_wall)
                {
                    // hit a wall
                    ScaleSpriteVector(SpriteNum, 28000);
                    u->ret = 0;
                    u->Counter = 0;
                }
                else
                {
                    // hit a sector
                    if (sp->z > DIV2(u->hiz + u->loz))
                    {
                        // hit a floor
                        if (!TEST(u->Flags, SPR_BOUNCE))
                        {
                            SET(u->Flags, SPR_BOUNCE);
                            ScaleSpriteVector(SpriteNum, 18000);
                            u->ret = 0;
                            u->Counter = 0;
                        }
                        else
                        {
                            if (u->ID == GORE_Drip)
                                ChangeState(SpriteNum, s_GoreFloorSplash);
                            else
                                ShrapKillSprite(SpriteNum);
                            return TRUE;
                        }
                    }
                    else
                    {
                        // hit a ceiling
                        ScaleSpriteVector(SpriteNum, 22000);
                    }
                }
            }
            else
            {
                // hit floor
                if (sp->z > DIV2(u->hiz + u->loz))
                {
                    sp->z = u->loz;
                    if (TEST(u->Flags, SPR_UNDERWATER))
                        SET(u->Flags, SPR_BOUNCE); // no bouncing underwater

                    if (u->lo_sectp && SectUser[sp->sectnum] && SectUser[sp->sectnum]->depth)
                        SET(u->Flags, SPR_BOUNCE); // no bouncing on shallow water

                    if (!TEST(u->Flags, SPR_BOUNCE))
                    {
                        SET(u->Flags, SPR_BOUNCE);
                        u->ret = 0;
                        u->Counter = 0;
                        u->zchange = -u->zchange;
                        ScaleSpriteVector(SpriteNum, 18000);
                        switch (u->ID)
                        {
                        case UZI_SHELL:
                            PlaySound(DIGI_SHELL, &sp->x, &sp->y, &sp->z, v3df_none);
                            break;
                        case SHOT_SHELL:
                            PlaySound(DIGI_SHOTSHELLSPENT, &sp->x, &sp->y, &sp->z, v3df_none);
                            break;
                        }
                    }
                    else
                    {
                        if (u->ID == GORE_Drip)
                            ChangeState(SpriteNum, s_GoreFloorSplash);
                        else
                            ShrapKillSprite(SpriteNum);
                        return TRUE;
                    }
                }
                else
                // hit something above
                {
                    u->zchange = -u->zchange;
                    ScaleSpriteVector(SpriteNum, 22000);
                }
            }
            break;
        }
        }
    }

    // just outright kill it if its boucing around alot
    if (u->bounce > 10)
    {
        KillSprite(SpriteNum);
        return TRUE;
    }

    return FALSE;
}


int
ShrapKillSprite(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    short rnd_num;

    rnd_num = RANDOM_RANGE(1024);

    ASSERT(u);

    switch (u->ID)
    {
    case UZI_SHELL:
        if (rnd_num > 854)
        {
            QueueGeneric(SpriteNum,UZI_SHELL + 0);
        }
        else if (rnd_num > 684)
        {
            QueueGeneric(SpriteNum,UZI_SHELL + 1);
        }
        else if (rnd_num > 514)
        {
            QueueGeneric(SpriteNum,UZI_SHELL + 2);
        }
        else if (rnd_num > 344)
        {
            QueueGeneric(SpriteNum,UZI_SHELL + 3);
        }
        else if (rnd_num > 174)
        {
            QueueGeneric(SpriteNum,UZI_SHELL + 4);
        }
        else
        {
            QueueGeneric(SpriteNum,UZI_SHELL + 5);
        }

        return 0;
        break;
    case SHOT_SHELL:
        if (rnd_num > 683)
        {
            QueueGeneric(SpriteNum,SHOT_SHELL + 1);
        }
        else if (rnd_num > 342)
        {
            QueueGeneric(SpriteNum,SHOT_SHELL + 3);
        }
        else
        {
            QueueGeneric(SpriteNum,SHOT_SHELL + 7);
        }
        return 0;
        break;
    case GORE_Lung:
        if (RANDOM_RANGE(1000) > 500) break;
        sp->clipdist = SPRITEp_SIZE_X(sp);
        SpawnFloorSplash(SpriteNum);
        if (RANDOM_RANGE(1000) < 500)
            PlaySound(DIGI_GIBS1,&sp->x,&sp->y,&sp->z,v3df_none);
        else
            PlaySound(DIGI_GIBS2,&sp->x,&sp->y,&sp->z,v3df_none);
        if (rnd_num > 683)
        {
            QueueGeneric(SpriteNum,900);
        }
        else if (rnd_num > 342)
        {
            QueueGeneric(SpriteNum,901);
        }
        else
        {
            QueueGeneric(SpriteNum,902);
        }
        return 0;
        break;
    case GORE_Liver:
        if (RANDOM_RANGE(1000) > 500) break;
        sp->clipdist = SPRITEp_SIZE_X(sp);
        SpawnFloorSplash(SpriteNum);
        if (RANDOM_RANGE(1000) < 500)
            PlaySound(DIGI_GIBS1,&sp->x,&sp->y,&sp->z,v3df_none);
        else
            PlaySound(DIGI_GIBS2,&sp->x,&sp->y,&sp->z,v3df_none);
        if (rnd_num > 683)
        {
            QueueGeneric(SpriteNum,915);
        }
        else if (rnd_num > 342)
        {
            QueueGeneric(SpriteNum,916);
        }
        else
        {
            QueueGeneric(SpriteNum,917);
        }
        return 0;
        break;
    case GORE_SkullCap:
        if (RANDOM_RANGE(1000) > 500) break;
        sp->clipdist = SPRITEp_SIZE_X(sp);
        SpawnFloorSplash(SpriteNum);
        if (rnd_num > 683)
        {
            QueueGeneric(SpriteNum,930);
        }
        else if (rnd_num > 342)
        {
            QueueGeneric(SpriteNum,931);
        }
        else
        {
            QueueGeneric(SpriteNum,932);
        }
        return 0;
        break;
    case GORE_Head:
        if (RANDOM_RANGE(1000) > 500) break;
        sp->clipdist = SPRITEp_SIZE_X(sp);
        QueueFloorBlood(SpriteNum);
        QueueGeneric(SpriteNum,GORE_Head);
        return 0;
        break;
    }

    // If it wasn't in the switch statement, kill it.
    KillSprite(SpriteNum);

    return 0;
}

SWBOOL CheckBreakToughness(BREAK_INFOp break_info, short ID)
{
    ////DSPRINTF(ds,"CheckBreakToughness called with %d",ID);
    //CON_Message(ds);
    //MONO_PRINT(ds);

    if (TEST(break_info->flags, BF_TOUGH))
    {
        switch (ID)
        {
        case LAVA_BOULDER:
        case MIRV_METEOR:
        case SERP_METEOR:
        case BOLT_THINMAN_R0:
        case BOLT_THINMAN_R1:
        case BOLT_THINMAN_R2:
        case BOLT_EXP:
        case TANK_SHELL_EXP:
        case GRENADE_EXP:
        case MICRO_EXP:
        case MINE_EXP:
        case NAP_EXP:
        case SKULL_R0:
        case BETTY_R0:
        case SKULL_SERP:
        case FIREBALL1:
        case GORO_FIREBALL:
            return TRUE;   // All the above stuff will break tough things
        }
        return FALSE;   // False means it won't break with current weapon
    }

    return TRUE;   // It wasn't tough, go ahead and break it
}

int
DoItemFly(int16_t SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    int32_t dax, day, daz;

    if (TEST(u->Flags, SPR_UNDERWATER))
    {
        ScaleSpriteVector(SpriteNum, 50000);

        u->Counter += 20*2;
        u->zchange += u->Counter;
    }
    else
    {
        u->Counter += 60*2;
        u->zchange += u->Counter;
    }

    u->ret = move_missile(SpriteNum, u->xchange, u->ychange, u->zchange,
                          u->ceiling_dist, u->floor_dist, CLIPMASK_MISSILE, MISSILEMOVETICS*2);

    MissileHitDiveArea(SpriteNum);

    if (u->ret)
    {
        switch (TEST(u->ret, HIT_MASK))
        {
        case HIT_SPRITE:
        {
            short wall_ang, dang;
            short hit_sprite = -2;
            SPRITEp hsp;

            hit_sprite = NORM_SPRITE(u->ret);
            hsp = &sprite[hit_sprite];

            if (TEST(hsp->cstat, CSTAT_SPRITE_WALL))
            {
                wall_ang = NORM_ANGLE(hsp->ang);
                WallBounce(SpriteNum, wall_ang);
                ScaleSpriteVector(SpriteNum, 32000);
            }
            else
            {
                u->xchange = -u->xchange;
                u->ychange = -u->ychange;
            }

            break;
        }

        case HIT_WALL:
        {
            short hit_wall,nw,wall_ang,dang;
            WALLp wph;

            hit_wall = NORM_WALL(u->ret);
            wph = &wall[hit_wall];

            nw = wall[hit_wall].point2;
            wall_ang = NORM_ANGLE(getangle(wall[nw].x - wph->x, wall[nw].y - wph->y)+512);

            WallBounce(SpriteNum, wall_ang);
            ScaleSpriteVector(SpriteNum, 32000);
            break;
        }

        case HIT_SECTOR:
        {
            // hit floor
            if (sp->z > DIV2(u->hiz + u->loz))
            {
                sp->z = u->loz;
                u->Counter = 0;
                sp->xvel = 0;
                u->zchange = u->xchange = u->ychange = 0;
                return FALSE;
            }
            else
            // hit something above
            {
                u->zchange = -u->zchange;
                ScaleSpriteVector(SpriteNum, 22000);
            }
            break;
        }
        }
    }

    return TRUE;
}

// This is the FAST queue, it doesn't call any animator functions or states
int QueueLoWangs(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum],ps;
    USERp u;
    SPRITEp osp;
    short NewSprite;


    if (TEST(sector[sp->sectnum].extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_WATER)
    {
        return -1;
    }

    if (TEST(sector[sp->sectnum].extra, SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
    {
        return -1;
    }

    if (TestDontStickSector(sp->sectnum))
    {
        return -1;
    }

    if (LoWangsQueue[LoWangsQueueHead] == -1)
    {
        LoWangsQueue[LoWangsQueueHead] = NewSprite =
                                             SpawnSprite(STAT_GENERIC_QUEUE, sp->picnum, s_DeadLoWang, sp->sectnum,
                                                         sp->x, sp->y, sp->z, sp->ang, 0);
    }
    else
    {
        // move old sprite to new sprite's place
        osp = &sprite[LoWangsQueue[LoWangsQueueHead]];
        setspritez(LoWangsQueue[LoWangsQueueHead], (vec3_t *)sp);
        NewSprite = LoWangsQueue[LoWangsQueueHead];
        ASSERT(sprite[NewSprite].statnum != MAXSTATUS);
    }

    // Point passed in sprite to ps
    ps = sp;
    sp = &sprite[NewSprite];
    u = User[NewSprite];
    ASSERT(sp);
    sp->owner = -1;
    sp->cstat = 0;
    sp->xrepeat = ps->xrepeat;
    sp->yrepeat = ps->yrepeat;
    sp->shade = ps->shade;
    u->spal = sp->pal = ps->pal;
    change_sprite_stat(NewSprite,STAT_DEFAULT); // Breakable
    SET(sp->cstat, CSTAT_SPRITE_BREAKABLE);
    SET(sp->extra, SPRX_BREAKABLE);
    SET(sp->cstat, CSTAT_SPRITE_BLOCK_HITSCAN);

    LoWangsQueueHead = (LoWangsQueueHead+1) & (MAX_LOWANGS_QUEUE-1);

    return SpriteNum;
}


#include "saveable.h"

static saveable_code saveable_weapon_code[] =
{
    SAVE_CODE(MissileHitMatch),
    SAVE_CODE(SpawnShrapX),
    SAVE_CODE(DoLavaErupt),
    SAVE_CODE(UserShrapSetup),
    SAVE_CODE(SpawnShrap),
    SAVE_CODE(DoShrapMove),
    SAVE_CODE(DoVomit),
    SAVE_CODE(DoVomit),
    SAVE_CODE(DoVomitSplash),
    SAVE_CODE(DoFastShrapJumpFall),
    SAVE_CODE(DoTracerShrap),
    SAVE_CODE(DoShrapJumpFall),
    SAVE_CODE(DoShrapDamage),
    SAVE_CODE(SpawnBlood),
    SAVE_CODE(VehicleMoveHit),
    SAVE_CODE(WeaponMoveHit),
    SAVE_CODE(DoUziSmoke),
    SAVE_CODE(DoShotgunSmoke),
    SAVE_CODE(DoMineSpark),
    SAVE_CODE(DoFireballFlames),
    SAVE_CODE(DoBreakFlames),
    SAVE_CODE(SetSuicide),
    SAVE_CODE(DoActorScale),
    SAVE_CODE(DoRipperGrow),
    SAVE_CODE(ActorChooseDeath),
    SAVE_CODE(ActorHealth),
    SAVE_CODE(SopDamage),
    SAVE_CODE(SopCheckKill),
    SAVE_CODE(ActorPain),
    SAVE_CODE(ActorPainPlasma),
    SAVE_CODE(ActorStdMissile),
    SAVE_CODE(ActorDamageSlide),
    SAVE_CODE(PlayerDamageSlide),
    SAVE_CODE(GetDamage),
    SAVE_CODE(RadiusGetDamage),
    SAVE_CODE(PlayerCheckDeath),
    SAVE_CODE(PlayerTakeDamage),
    SAVE_CODE(StarBlood),
    SAVE_CODE(DoDamage),
    SAVE_CODE(DoDamageTest),
    SAVE_CODE(DoHitscanDamage),
    SAVE_CODE(DoFlamesDamageTest),
    SAVE_CODE(DoStar),
    SAVE_CODE(DoCrossBolt),
    SAVE_CODE(DoPlasmaDone),
    SAVE_CODE(MissileSeek),
    SAVE_CODE(ComboMissileSeek),
    SAVE_CODE(VectorMissileSeek),
    SAVE_CODE(VectorWormSeek),
    SAVE_CODE(DoBlurExtend),
    SAVE_CODE(InitPlasmaFountain),
    SAVE_CODE(DoPlasmaFountain),
    SAVE_CODE(DoPlasma),
    SAVE_CODE(DoCoolgFire),
    SAVE_CODE(DoEelFire),
    SAVE_CODE(DoGrenade),
    SAVE_CODE(DoVulcanBoulder),
    SAVE_CODE(OwnerIsPlayer),
    SAVE_CODE(DoMineRangeTest),
    SAVE_CODE(DoMineStuck),
    SAVE_CODE(SetMineStuck),
    SAVE_CODE(DoMine),
    SAVE_CODE(DoPuff),
    SAVE_CODE(DoRailPuff),
    SAVE_CODE(DoBoltThinMan),
    SAVE_CODE(DoTracer),
    SAVE_CODE(DoEMP),
    SAVE_CODE(DoEMPBurst),
    SAVE_CODE(DoTankShell),
    SAVE_CODE(DoTracerStart),
    SAVE_CODE(DoLaser),
    SAVE_CODE(DoLaserStart),
    SAVE_CODE(DoRail),
    SAVE_CODE(DoRailStart),
    SAVE_CODE(DoRocket),
    SAVE_CODE(DoMicroMini),
    SAVE_CODE(SpawnExtraMicroMini),
    SAVE_CODE(DoMicro),
    SAVE_CODE(DoUziBullet),
    SAVE_CODE(DoBoltSeeker),
    SAVE_CODE(DoBoltShrapnel),
    SAVE_CODE(DoBoltFatMan),
    SAVE_CODE(DoElectro),
    SAVE_CODE(DoLavaBoulder),
    SAVE_CODE(DoSpear),
    SAVE_CODE(SpawnBasicExp),
    SAVE_CODE(SpawnFireballFlames),
    SAVE_CODE(SpawnBreakFlames),
    SAVE_CODE(SpawnBreakStaticFlames),
    SAVE_CODE(SpawnFireballExp),
    SAVE_CODE(SpawnGoroFireballExp),
    SAVE_CODE(SpawnBoltExp),
    SAVE_CODE(SpawnBunnyExp),
    SAVE_CODE(SpawnTankShellExp),
    SAVE_CODE(SpawnNuclearSecondaryExp),
    SAVE_CODE(SpawnNuclearExp),
    SAVE_CODE(SpawnTracerExp),
    SAVE_CODE(SpawnMicroExp),
    SAVE_CODE(AddSpriteToSectorObject),
    SAVE_CODE(SpawnBigGunFlames),
    SAVE_CODE(SpawnGrenadeSecondaryExp),
    SAVE_CODE(SpawnGrenadeSmallExp),
    SAVE_CODE(SpawnGrenadeExp),
    SAVE_CODE(SpawnMineExp),
    SAVE_CODE(InitMineShrap),
    SAVE_CODE(DoSectorExp),
    SAVE_CODE(SpawnSectorExp),
    SAVE_CODE(SpawnLargeExp),
    SAVE_CODE(SpawnMeteorExp),
    SAVE_CODE(SpawnLittleExp),
    SAVE_CODE(DoFireball),
    SAVE_CODE(DoFindGround),
    SAVE_CODE(DoFindGroundPoint),
    SAVE_CODE(DoNapalm),
    SAVE_CODE(DoBloodWorm),
    SAVE_CODE(DoBloodWorm),
    SAVE_CODE(DoMeteor),
    SAVE_CODE(DoSerpMeteor),
    SAVE_CODE(DoMirvMissile),
    SAVE_CODE(DoMirv),
    SAVE_CODE(MissileSetPos),
    SAVE_CODE(TestMissileSetPos),
    SAVE_CODE(DoRing),
    SAVE_CODE(InitSpellRing),
    SAVE_CODE(DoSerpRing),
    SAVE_CODE(InitLavaFlame),
    SAVE_CODE(InitLavaThrow),
    SAVE_CODE(InitVulcanBoulder),
    SAVE_CODE(InitSerpRing),
    SAVE_CODE(InitSerpRing),
    //SAVE_CODE(InitSerpRing2),
    SAVE_CODE(InitSpellNapalm),
    SAVE_CODE(InitEnemyNapalm),
    SAVE_CODE(InitSpellMirv),
    SAVE_CODE(InitEnemyMirv),
    SAVE_CODE(InitSwordAttack),
    SAVE_CODE(InitFistAttack),
    SAVE_CODE(InitSumoNapalm),
    SAVE_CODE(InitSumoSkull),
    SAVE_CODE(InitSumoStompAttack),
    SAVE_CODE(InitMiniSumoClap),
    SAVE_CODE(WeaponAutoAim),
    SAVE_CODE(WeaponAutoAimZvel),
    SAVE_CODE(AimHitscanToTarget),
    SAVE_CODE(WeaponAutoAimHitscan),
    SAVE_CODE(WeaponHitscanShootFeet),
    SAVE_CODE(InitStar),
    SAVE_CODE(InitHeartAttack),
    SAVE_CODE(InitHeartAttack),
    SAVE_CODE(InitShotgun),
    SAVE_CODE(InitLaser),
    SAVE_CODE(InitRail),
    SAVE_CODE(InitZillaRail),
    SAVE_CODE(InitRocket),
    SAVE_CODE(InitBunnyRocket),
    SAVE_CODE(InitNuke),
    SAVE_CODE(InitEnemyNuke),
    SAVE_CODE(InitMicro),
    SAVE_CODE(InitRipperSlash),
    SAVE_CODE(InitBunnySlash),
    SAVE_CODE(InitSerpSlash),
    SAVE_CODE(WallSpriteInsideSprite),
    SAVE_CODE(DoBladeDamage),
    SAVE_CODE(DoStaticFlamesDamage),
    SAVE_CODE(InitCoolgBash),
    SAVE_CODE(InitSkelSlash),
    SAVE_CODE(InitGoroChop),
    SAVE_CODE(InitHornetSting),
    SAVE_CODE(InitSerpSpell),
    SAVE_CODE(SpawnDemonFist),
    SAVE_CODE(InitSerpMonstSpell),
    SAVE_CODE(DoTeleRipper),
    SAVE_CODE(InitEnemyRocket),
    SAVE_CODE(InitEnemyRail),
    SAVE_CODE(InitZillaRocket),
    SAVE_CODE(InitEnemyStar),
    SAVE_CODE(InitEnemyCrossbow),
    SAVE_CODE(InitSkelSpell),
    SAVE_CODE(InitCoolgFire),
    SAVE_CODE(InitCoolgDrip),
    SAVE_CODE(GenerateDrips),
    SAVE_CODE(InitEelFire),
    SAVE_CODE(InitFireballTrap),
    SAVE_CODE(InitBoltTrap),
    SAVE_CODE(InitEnemyCrossbow),
    SAVE_CODE(InitSpearTrap),
    SAVE_CODE(DoSuicide),
    SAVE_CODE(DoDefaultStat),
    SAVE_CODE(InitTracerUzi),
    SAVE_CODE(InitTracerTurret),
    SAVE_CODE(InitTracerAutoTurret),
    SAVE_CODE(BulletHitSprite),
    SAVE_CODE(HitscanSpriteAdjust),
    SAVE_CODE(InitUzi),
    SAVE_CODE(InitEMP),
    SAVE_CODE(InitTankShell),
    SAVE_CODE(InitTurretMicro),
    SAVE_CODE(InitTurretRocket),
    SAVE_CODE(InitTurretFireball),
    SAVE_CODE(InitTurretRail),
    SAVE_CODE(InitTurretLaser),
    SAVE_CODE(InitSobjMachineGun),
    SAVE_CODE(InitSobjGun),
    SAVE_CODE(SpawnBoatSparks),
    SAVE_CODE(SpawnSwordSparks),
    SAVE_CODE(SpawnTurretSparks),
    SAVE_CODE(SpawnShotgunSparks),
    SAVE_CODE(InitTurretMgun),
    SAVE_CODE(InitEnemyUzi),
    SAVE_CODE(InitGrenade),
    SAVE_CODE(InitSpriteGrenade),
    SAVE_CODE(InitMine),
    SAVE_CODE(InitEnemyMine),
    SAVE_CODE(HelpMissileLateral),
    SAVE_CODE(InitFireball),
    SAVE_CODE(InitEnemyFireball),
    SAVE_CODE(WarpToUnderwater),
    SAVE_CODE(WarpToSurface),
    SAVE_CODE(SpriteWarpToUnderwater),
    SAVE_CODE(SpriteWarpToSurface),
    SAVE_CODE(SpawnSplash),
    SAVE_CODE(SpawnSplashXY),
    SAVE_CODE(SpawnUnderSplash),
    SAVE_CODE(MissileHitDiveArea),
    SAVE_CODE(SpawnBubble),
    SAVE_CODE(DoVehicleSmoke),
    SAVE_CODE(DoWaterSmoke),
    SAVE_CODE(SpawnVehicleSmoke),
    SAVE_CODE(SpawnSmokePuff),
    SAVE_CODE(DoBubble),
    SAVE_CODE(SpriteQueueDelete),
    SAVE_CODE(DoFloorBlood),
    SAVE_CODE(DoWallBlood),
    //SAVE_CODE(DoShellShrap),
    SAVE_CODE(SpawnShell),
    SAVE_CODE(DoShrapVelocity),
    SAVE_CODE(ShrapKillSprite),
    SAVE_CODE(DoItemFly),
};

static saveable_data saveable_weapon_data[] =
{
    SAVE_DATA(s_NotRestored),
    SAVE_DATA(s_Suicide),
    SAVE_DATA(s_DeadLoWang),
    SAVE_DATA(s_BreakLight),
    SAVE_DATA(s_BreakBarrel),
    SAVE_DATA(s_BreakPedistal),
    SAVE_DATA(s_BreakBottle1),
    SAVE_DATA(s_BreakBottle2),
    SAVE_DATA(s_Puff),
    SAVE_DATA(s_RailPuff),
    SAVE_DATA(sg_RailPuff),
    SAVE_DATA(s_LaserPuff),
    SAVE_DATA(s_Tracer),
    SAVE_DATA(s_EMP),
    SAVE_DATA(s_EMPBurst),
    SAVE_DATA(s_EMPShrap),
    SAVE_DATA(s_TankShell),
    SAVE_DATA(s_VehicleSmoke),
    SAVE_DATA(s_WaterSmoke),
    SAVE_DATA(s_UziSmoke),
    SAVE_DATA(s_ShotgunSmoke),
    SAVE_DATA(s_UziBullet),
    SAVE_DATA(s_UziSpark),
    SAVE_DATA(s_UziPowerSpark),
    SAVE_DATA(s_Bubble),
    SAVE_DATA(s_Splash),
    SAVE_DATA(s_CrossBolt),
    SAVE_DATA(sg_CrossBolt),
    SAVE_DATA(s_Star),
    SAVE_DATA(s_StarStuck),
    SAVE_DATA(s_StarDown),
    SAVE_DATA(s_StarDownStuck),
    SAVE_DATA(s_LavaBoulder),
    SAVE_DATA(s_LavaShard),
    SAVE_DATA(s_VulcanBoulder),
    SAVE_DATA(s_Grenade),
    SAVE_DATA(s_Grenade),
    SAVE_DATA(sg_Grenade),
    SAVE_DATA(s_MineStuck),
    SAVE_DATA(s_Mine),
    SAVE_DATA(s_MineSpark),
    SAVE_DATA(s_Meteor),
    SAVE_DATA(sg_Meteor),
    SAVE_DATA(s_MeteorExp),
    SAVE_DATA(s_MirvMeteor),
    SAVE_DATA(sg_MirvMeteor),
    SAVE_DATA(s_MirvMeteorExp),
    SAVE_DATA(s_SerpMeteor),
    SAVE_DATA(sg_SerpMeteor),
    SAVE_DATA(s_SerpMeteorExp),
    SAVE_DATA(s_Spear),
    SAVE_DATA(sg_Spear),
    SAVE_DATA(s_Rocket),
    SAVE_DATA(sg_Rocket),
    SAVE_DATA(s_BunnyRocket),
    SAVE_DATA(sg_BunnyRocket),
    SAVE_DATA(s_Rail),
    SAVE_DATA(sg_Rail),
    SAVE_DATA(s_Laser),
    //SAVE_DATA(s_MicroPuff),
    SAVE_DATA(s_Micro),
    SAVE_DATA(sg_Micro),
    SAVE_DATA(s_MicroMini),
    SAVE_DATA(sg_MicroMini),
    SAVE_DATA(s_BoltThinMan),
    SAVE_DATA(sg_BoltThinMan),
    SAVE_DATA(s_BoltSeeker),
    SAVE_DATA(sg_BoltSeeker),
    SAVE_DATA(s_BoltFatMan),
    SAVE_DATA(s_BoltShrapnel),
    SAVE_DATA(s_CoolgFire),
    SAVE_DATA(s_CoolgFireDone),
    SAVE_DATA(s_CoolgDrip),
    SAVE_DATA(s_GoreFloorSplash),
    SAVE_DATA(s_GoreSplash),
    SAVE_DATA(s_Plasma),
    SAVE_DATA(s_PlasmaFountain),
    SAVE_DATA(s_PlasmaDrip),
    SAVE_DATA(s_PlasmaDone),
    SAVE_DATA(s_TeleportEffect),
    SAVE_DATA(s_TeleportEffect2),
    SAVE_DATA(s_Electro),
    SAVE_DATA(s_ElectroShrap),
    SAVE_DATA(s_GrenadeExp),
    SAVE_DATA(s_GrenadeSmallExp),
    SAVE_DATA(s_GrenadeExp),
    SAVE_DATA(s_MineExp),
    SAVE_DATA(s_BasicExp),
    SAVE_DATA(s_MicroExp),
    SAVE_DATA(s_BigGunFlame),
    SAVE_DATA(s_BoltExp),
    SAVE_DATA(s_TankShellExp),
    SAVE_DATA(s_TracerExp),
    SAVE_DATA(s_SectorExp),
    SAVE_DATA(s_FireballExp),
    SAVE_DATA(s_NapExp),
    SAVE_DATA(s_FireballFlames),
    SAVE_DATA(s_BreakFlames),
    SAVE_DATA(s_Fireball),
    SAVE_DATA(s_Fireball),
    //SAVE_DATA(sg_Fireball),
    SAVE_DATA(s_Ring),
    //SAVE_DATA(sg_Ring),
    SAVE_DATA(s_Ring),
    SAVE_DATA(s_Ring2),
    SAVE_DATA(s_Napalm),
    SAVE_DATA(s_BloodWorm),
    SAVE_DATA(s_BloodWorm),
    SAVE_DATA(s_PlasmaExp),
    SAVE_DATA(s_PlasmaExp),
    SAVE_DATA(s_Mirv),
    SAVE_DATA(s_MirvMissile),
    SAVE_DATA(s_Vomit1),
    SAVE_DATA(s_Vomit2),
    SAVE_DATA(s_VomitSplash),
    SAVE_DATA(s_GoreHead),
    SAVE_DATA(s_GoreLeg),
    SAVE_DATA(s_GoreEye),
    SAVE_DATA(s_GoreTorso),
    SAVE_DATA(s_GoreArm),
    SAVE_DATA(s_GoreLung),
    SAVE_DATA(s_GoreLiver),
    SAVE_DATA(s_GoreSkullCap),
    SAVE_DATA(s_GoreChunkS),
    SAVE_DATA(s_GoreDrip),
    SAVE_DATA(s_FastGoreDrip),
    SAVE_DATA(s_GoreFlame),
    SAVE_DATA(s_TracerShrap),
    SAVE_DATA(s_UziShellShrap),
    SAVE_DATA(s_UziShellShrapStill1),
    SAVE_DATA(s_UziShellShrapStill2),
    SAVE_DATA(s_UziShellShrapStill3),
    SAVE_DATA(s_UziShellShrapStill4),
    SAVE_DATA(s_UziShellShrapStill5),
    SAVE_DATA(s_UziShellShrapStill6),
    SAVE_DATA(s_ShotgunShellShrap),
    SAVE_DATA(s_ShotgunShellShrapStill1),
    SAVE_DATA(s_ShotgunShellShrapStill2),
    SAVE_DATA(s_ShotgunShellShrapStill3),
    SAVE_DATA(s_GoreFlameChunkA),
    SAVE_DATA(s_GoreFlameChunkB),
    SAVE_DATA(s_CoinShrap),
    SAVE_DATA(s_Marbel),
    SAVE_DATA(s_GlassShrapA),
    SAVE_DATA(s_GlassShrapB),
    SAVE_DATA(s_GlassShrapC),
    SAVE_DATA(s_WoodShrapA),
    SAVE_DATA(s_WoodShrapB),
    SAVE_DATA(s_WoodShrapC),
    SAVE_DATA(s_StoneShrapA),
    SAVE_DATA(s_StoneShrapB),
    SAVE_DATA(s_StoneShrapC),
    SAVE_DATA(s_MetalShrapA),
    SAVE_DATA(s_MetalShrapB),
    SAVE_DATA(s_MetalShrapC),
    SAVE_DATA(s_PaperShrapA),
    SAVE_DATA(s_PaperShrapB),
    SAVE_DATA(s_PaperShrapC),
    SAVE_DATA(s_FloorBlood1),
    SAVE_DATA(s_FootPrint1),
    SAVE_DATA(s_FootPrint2),
    SAVE_DATA(s_FootPrint3),
    SAVE_DATA(s_WallBlood1),
    SAVE_DATA(s_WallBlood2),
    SAVE_DATA(s_WallBlood3),
    SAVE_DATA(s_WallBlood4),
};

saveable_module saveable_weapon =
{
    // code
    saveable_weapon_code,
    SIZ(saveable_weapon_code),

    // data
    saveable_weapon_data,
    SIZ(saveable_weapon_data)
};
