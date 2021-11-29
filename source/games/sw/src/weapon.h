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

BEGIN_SW_NS

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

enum
{
    UZI_SHELL = 2152,
    SHOT_SHELL = 2180
};


extern short StarQueueHead;
extern DSWActor* StarQueue[MAX_STAR_QUEUE];
extern short HoleQueueHead;
extern DSWActor* HoleQueue[MAX_HOLE_QUEUE];
extern short WallBloodQueueHead;
extern DSWActor* WallBloodQueue[MAX_WALLBLOOD_QUEUE];
extern short FloorBloodQueueHead;
extern DSWActor* FloorBloodQueue[MAX_FLOORBLOOD_QUEUE];
extern short GenericQueueHead;
extern DSWActor* GenericQueue[MAX_GENERIC_QUEUE];
extern short LoWangsQueueHead;
extern DSWActor* LoWangsQueue[MAX_LOWANGS_QUEUE];

void ChangeState(DSWActor* actor, STATEp statep);
void DoPlayerBeginRecoil(PLAYERp pp, short pix_amt);
SECTOR_OBJECTp DetectSectorObject(SECTORp);
SECTOR_OBJECTp DetectSectorObjectByWall(WALLp);
void ScaleSpriteVector(DSWActor* actor, int scale);
void QueueHole(short hit_sect, short hit_wall, int hit_x, int hit_y, int hit_z);
DSWActor* QueueWallBlood(DSWActor* hit, short ang);
bool SlopeBounce(DSWActor*, bool *hit_wall);
int SpawnSwordSparks(PLAYERp pp, short hit_sect, short hit_wall, int hit_x, int hit_y, int hit_z, short hit_ang);
DSWActor* SpawnBubble(DSWActor*);
void SpawnFireballExp(DSWActor*);
void SpawnFireballFlames(DSWActor* actor, DSWActor* enemyActor);
int SpawnRadiationCloud(DSWActor* actor);
void SpawnGrenadeExp(DSWActor*);
DSWActor* SpawnSectorExp(DSWActor*);
int DoShrapVelocity(DSWActor*);
int ShrapKillSprite(DSWActor*);
bool MissileSetPos(DSWActor*,ANIMATORp DoWeapon,int dist);
int ActorPain(DSWActor*);
int SpawnBreakFlames(DSWActor*);
bool PlayerTakeDamage(PLAYERp pp, DSWActor* weapActor);
const char *DeathString(DSWActor*);

//
// Damage Amounts defined in damage.h
//

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

inline int CLOSE_RANGE_DIST_FUDGE(SPRITEp sp1, SPRITEp sp2, int fudge)
{
    return (((int)sp1->clipdist << 2) + ((int)sp2->clipdist << 2) + fudge);
}

inline int CLOSE_RANGE_DIST(SPRITEp sp1, SPRITEp sp2)
{
    return CLOSE_RANGE_DIST_FUDGE(sp1, sp2, 400);
}


extern short target_ang;

bool SpriteOverlap(short, short);

DSWActor* SpawnShotgunSparks(PLAYERp pp, short hit_sect, short hit_wall, int hit_x, int hit_y, int hit_z, short hit_ang);
int DoActorBeginSlide(DSWActor* actor, short ang, short vel, short dec);
int GetOverlapSector(int x, int y, short *over, short *under);
bool MissileHitDiveArea(DSWActor*);

int DoDamageTest(DSWActor*);

extern short StatDamageList[STAT_DAMAGE_LIST_SIZE];

#define RADIATION_CLOUD 3258
#define MUSHROOM_CLOUD 3280
extern STATE s_NukeMushroom[];

void WallBounce(DSWActor*, short ang);

#define PUFF 1748
#define CALTROPS 2218
#define PHOSPHORUS 1397

int PlayerInitChemBomb(PLAYERp pp);
int InitChemBomb(DSWActor*);
int PlayerInitCaltrops(PLAYERp pp);
int InitBloodSpray(DSWActor* actor, bool dogib, short velocity);
int SpawnBunnyExp(DSWActor* actor);
int InitBunnyRocket(PLAYERp pp);

int GetDamage(DSWActor*, DSWActor*, int DamageNdx);
int DoFlamesDamageTest(DSWActor*);

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

int SetSuicide(DSWActor*);
void UpdateSinglePlayKills(DSWActor* actor);
int InitPlasmaFountain(DSWActor* wActor, DSWActor* sActor);
int InitCoolgDrip(DSWActor*);
int InitFireball(PLAYERp pp);
void InitSpellRing(PLAYERp pp);
void InitSpellNapalm(PLAYERp pp);
int DoStaticFlamesDamage(DSWActor*);
int InitUzi(PLAYERp pp);
int InitSobjGun(PLAYERp pp);
void InitFireballTrap(DSWActor* actor);
void InitBoltTrap(DSWActor* actor);
void InitSpearTrap(DSWActor*);
int InitTurretMgun(SECTOR_OBJECTp sop);
void InitVulcanBoulder(DSWActor* actor);
int DoBladeDamage(DSWActor*);
int DoFindGround(DSWActor*);
int DoFindGroundPoint(DSWActor* actor);
void SpriteQueueDelete(DSWActor* actor);
int HelpMissileLateral(DSWActor*, int dist);
void AddSpriteToSectorObject(DSWActor*,SECTOR_OBJECTp sop);
void QueueReset(void);
int PlayerCheckDeath(PLAYERp pp,DSWActor*);
bool SpriteWarpToUnderwater(DSWActor* actor);
int PlayerDamageSlide(PLAYERp pp,int damage,short ang);
bool VehicleMoveHit(DSWActor*);
int SpawnSplash(DSWActor*);
void SpawnMineExp(DSWActor*);
void SpawnLittleExp(DSWActor*);
DSWActor* SpawnLargeExp(DSWActor*);
void SpawnNuclearExp(DSWActor* actor);
void SpawnBoltExp(DSWActor* actor);
void SpawnTracerExp(DSWActor* Weapon);
void SpawnGoroFireballExp(DSWActor* Weapon);
bool MissileHitMatch(DSWActor* weapActor, int WeaponNum, DSWActor* hitActor);
int DoItemFly(DSWActor*);
int SpawnVehicleSmoke(DSWActor* actor);
short PrevWall(short wall_num);
int DoDamage(DSWActor*, DSWActor*);

END_SW_NS

#endif
