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

#ifndef WEAPON_H
#define WEAPON_H

#define NEW_ELECTRO 1
#define HORIZ_MULT 128L

#define ANG2PLAYER(pp,sp) (getangle((pp)->posx - (sp)->x, (pp)->posy - (sp)->y))
#define ANG2SPRITE(sp,op) (getangle((sp)->x - (op)->x, (sp)->y - (op)->y))

#define MAX_HOLE_QUEUE 64
#define MAX_STAR_QUEUE 32
#define MAX_WALLBLOOD_QUEUE  32
#define MAX_FLOORBLOOD_QUEUE 32
#define MAX_GENERIC_QUEUE 32
#define MAX_LOWANGS_QUEUE 16

//#define NUKE_RADIUS 16384
#define NUKE_RADIUS 30000
#define RAIL_RADIUS 3500

// This is how many bullet shells have been spawned since the beginning of the game.
extern FOOT_TYPE FootMode;
extern int ShellCount;
#define MAXSHELLS 32

extern short StarQueueHead;
extern short StarQueue[MAX_STAR_QUEUE];
extern short HoleQueueHead;
extern short HoleQueue[MAX_HOLE_QUEUE];
extern short WallBloodQueueHead;
extern short WallBloodQueue[MAX_WALLBLOOD_QUEUE];
extern short FloorBloodQueueHead;
extern short FloorBloodQueue[MAX_FLOORBLOOD_QUEUE];
extern short GenericQueueHead;
extern short GenericQueue[MAX_GENERIC_QUEUE];
extern short LoWangsQueueHead;
extern short LoWangsQueue[MAX_LOWANGS_QUEUE];

void ChangeState(short SpriteNum, STATEp statep);
void DoPlayerBeginRecoil(PLAYERp pp, short pix_amt);
SECTOR_OBJECTp DetectSectorObject(SECTORp);
SECTOR_OBJECTp DetectSectorObjectByWall(WALLp);
void ScaleSpriteVector(short SpriteNum, int scale);
int QueueHole(short ang, short hit_sect, short hit_wall, int hit_x, int hit_y, int hit_z);
int QueueWallBlood(short hit_sprite, short ang);
SWBOOL SlopeBounce(short SpriteNum, SWBOOL *hit_wall);
SWBOOL HitscanSpriteAdjust(short SpriteNum, short hit_wall);
int SpawnSwordSparks(PLAYERp pp, short hit_sect, short hit_wall, int hit_x, int hit_y, int hit_z, short hit_ang);
int SpawnBubble(short SpriteNum);
int SpawnFireballExp(int16_t Weapon);
int SpawnFireballFlames(int16_t SpriteNum,int16_t enemy);
int SpawnRadiationCloud(short SpriteNum);
int SpawnGrenadeExp(int16_t Weapon);
int SpawnSectorExp(int16_t Weapon);
int DoShrapVelocity(int16_t SpriteNum);
int ShrapKillSprite(short SpriteNum);
SWBOOL MissileSetPos(short Weapon,ANIMATORp DoWeapon,int dist);
int ActorPain(short SpriteNum);

//
// Damage Amounts defined in damage.h
//
extern DAMAGE_DATA DamageData[];

// Damage Times - takes damage after this many tics
#define DAMAGE_BLADE_TIME       (10)

// Player Missile Speeds
#define STAR_VELOCITY           (1800)
#define BOLT_VELOCITY           (900)
#define ROCKET_VELOCITY         (1350)
#define BOLT_SEEKER_VELOCITY    (820)
#define FIREBALL_VELOCITY       (2000)
#define ELECTRO_VELOCITY        (800)
#define PLASMA_VELOCITY         (1000)
#define UZI_BULLET_VELOCITY     (2500)
#define TRACER_VELOCITY         (1200)
#define TANK_SHELL_VELOCITY     (1200)
#define GRENADE_VELOCITY        (900)
#define MINE_VELOCITY           (520)   // Was 420
#define CHEMBOMB_VELOCITY       (420)

// Player Spell Missile Speeds
#define BLOOD_WORM_VELOCITY   (800)
#define NAPALM_VELOCITY         (800)
#define MIRV_VELOCITY          (600)
#define SPIRAL_VELOCITY         (600)

// Trap Speeds
#define BOLT_TRAP_VELOCITY      (950)
#define SPEAR_TRAP_VELOCITY     (650)
#define FIREBALL_TRAP_VELOCITY  (750)

// NPC Missile Speeds
#define NINJA_STAR_VELOCITY     (1800)
#define NINJA_BOLT_VELOCITY     (500)
#define GORO_FIREBALL_VELOCITY  (800)
#define SKEL_ELECTRO_VELOCITY   (850)
#define COOLG_FIRE_VELOCITY     (400)

#define GRENADE_RECOIL_AMT      (12)
#define ROCKET_RECOIL_AMT       (7)
#define RAIL_RECOIL_AMT       (7)
#define SHOTGUN_RECOIL_AMT      (12)
//#define MICRO_RECOIL_AMT        (15)

// Damage amounts that determine the type of player death
// The standard flip over death is default
#define PLAYER_DEATH_CRUMBLE_DAMMAGE_AMT   (25)
#define PLAYER_DEATH_EXPLODE_DAMMAGE_AMT   (65)

// electro weapon
#define ELECTRO_MAX_JUMP_DIST 25000

extern int WeaponIsAmmo;

#define MISSILEMOVETICS 6

#define CLOSE_RANGE_DIST_FUDGE(sp1, sp2, fudge) \
    (((int)(sp1)->clipdist<<2) + ((int)(sp2)->clipdist<<2) + (fudge))

#define CLOSE_RANGE_DIST(sp1, sp2) CLOSE_RANGE_DIST_FUDGE(sp1, sp2, 400)


extern short target_ang;

SWBOOL SpriteOverlap(short, short);

int SpawnShotgunSparks(PLAYERp pp, short hit_sect, short hit_wall, int hit_x, int hit_y, int hit_z, short hit_ang);
int DoActorBeginSlide(short SpriteNum, short ang, short vel, short dec);
int GetOverlapSector(int x, int y, short *over, short *under);
SWBOOL MissileHitDiveArea(short SpriteNum);

int DoDamageTest(short);

extern short StatDamageList[STAT_DAMAGE_LIST_SIZE];

#define RADIATION_CLOUD 3258
#define MUSHROOM_CLOUD 3280
extern STATE s_NukeMushroom[];

void WallBounce(short SpriteNum, short ang);

#define PUFF 1748
#define CALTROPS 2218
#define PHOSPHORUS 1397

int PlayerInitChemBomb(PLAYERp pp);
int InitChemBomb(short SpriteNum);
int PlayerInitCaltrops(PLAYERp pp);
int InitBloodSpray(int16_t SpriteNum, SWBOOL dogib, short velocity);
int SpawnBunnyExp(int16_t Weapon);
int InitBunnyRocket(PLAYERp pp);

int GetDamage(short SpriteNum, short Weapon, short DamageNdx);
int DoFlamesDamageTest(short Weapon);

void DoActorSpawnIcon(int16_t SpriteNum);

typedef struct
{
    STATEp state;
    short id, num, zlevel, min_jspeed, max_jspeed, min_vel, max_vel, random_disperse, ang_range;
    // state, id, num, min_jspeed, max_jspeed, min_vel, max_vel, size,
    // random_disperse, ang_range
} SHRAP, *SHRAPp;

enum ShrapPos
{
    Z_TOP,
    Z_MID,
    Z_BOT
};

int SetSuicide(short SpriteNum);
void UpdateSinglePlayKills(short SpriteNum);
int InitPlasmaFountain(SPRITEp wp, SPRITEp sp);
int InitCoolgDrip(short SpriteNum);
int InitFireball(PLAYERp pp);
void InitSpellRing(PLAYERp pp);
void InitSpellNapalm(PLAYERp pp);
int InitUzi(PLAYERp pp);
int InitSobjGun(PLAYERp pp);
int InitBoltTrap(short SpriteNum);
int InitSpearTrap(short SpriteNum);
int InitTurretMgun(SECTOR_OBJECTp sop);
int InitVulcanBoulder(short SpriteNum);
int DoBladeDamage(short SpriteNum);
int DoFindGround(int16_t SpriteNum);
int DoFindGroundPoint(int16_t SpriteNum);
void SpriteQueueDelete(short SpriteNum);
int HelpMissileLateral(int16_t Weapon,int dist);
int AddSpriteToSectorObject(short SpriteNum,SECTOR_OBJECTp sop);
void QueueReset(void);
int PlayerCheckDeath(PLAYERp pp,short Weapon);
SWBOOL SpriteWarpToUnderwater(SPRITEp sp);
int PlayerDamageSlide(PLAYERp pp,short damage,short ang);
SWBOOL VehicleMoveHit(short SpriteNum);
int SpawnSplash(short SpriteNum);
int SpawnMineExp(int16_t Weapon);
int SpawnLittleExp(int16_t Weapon);
int SpawnLargeExp(int16_t Weapon);
int SpawnNuclearExp(int16_t Weapon);
int SpawnBoltExp(int16_t Weapon);
int SpawnTracerExp(int16_t Weapon);
int SpawnGoroFireballExp(int16_t Weapon);
SWBOOL MissileHitMatch(short Weapon,short WeaponNum,short hit_sprite);
int DoItemFly(int16_t SpriteNum);
int SpawnVehicleSmoke(short SpriteNum);
short PrevWall(short wall_num);
int DoDamage(short SpriteNum,short Weapon);

#endif
