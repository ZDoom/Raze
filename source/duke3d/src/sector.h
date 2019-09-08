//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
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

#ifndef sector_h_
#define sector_h_

#include "actors.h"  // actor_t
#include "gamevars.h"
#include "macros.h"
#include "namesdyn.h"  // for G_GetForcefieldPicnum()
#include "player.h"  // playerspawn_t

#ifdef __cplusplus
extern "C" {
#endif

#define MAXCYCLERS      1024
#define MAXANIMATES     1024
#define MAXANIMWALLS    512
#define MAXANIMPOINTS   2048

#define VIEWSCREEN_ACTIVE_DISTANCE 8192

typedef struct {
    int16_t wallnum, tag;
} animwalltype;

typedef struct {
    // this needs to have a copy of everything related to the map/actor state
    // see savegame.c
    int32_t g_animateGoal[MAXANIMATES], g_animateVel[MAXANIMATES], g_animateCnt;
    intptr_t g_animatePtr[MAXANIMATES];
    int32_t filler;
    vec2_t origins[MAXANIMPOINTS];
    int32_t randomseed, g_globalRandom;
    int32_t pskyidx;

    int16_t SpriteDeletionQueue[1024],g_spriteDeleteQueuePos;
    int16_t g_animateSect[MAXANIMATES];
    int16_t g_cyclers[MAXCYCLERS][6];
    int16_t g_mirrorWall[64], g_mirrorSector[64], g_mirrorCount;
    int16_t g_animWallCnt;
    int16_t g_cloudCnt,g_cloudSect[256],g_cloudX,g_cloudY;
    int16_t g_cyclerCnt;

    int32_t numsprites;
    int16_t tailspritefree;
    int16_t headspritesect[MAXSECTORS+1];
    int16_t headspritestat[MAXSTATUS+1];
    int16_t nextspritesect[MAXSPRITES];
    int16_t nextspritestat[MAXSPRITES];
    int16_t numsectors;
    int16_t numwalls;
    int16_t prevspritesect[MAXSPRITES];
    int16_t prevspritestat[MAXSPRITES];

    uint16_t g_earthquakeTime;
    int8_t g_playerSpawnCnt;

    uint8_t show2dsector[(MAXSECTORS+7)>>3];

    actor_t actor[MAXSPRITES];
    playerspawn_t g_playerSpawnPoints[MAXPLAYERS];
    animwalltype animwall[MAXANIMWALLS];
    usectortype sector[MAXSECTORS];
    spriteext_t spriteext[MAXSPRITES];
    uspritetype sprite[MAXSPRITES];
    uwalltype wall[MAXWALLS];
#ifndef NEW_MAP_FORMAT
    wallext_t wallext[MAXWALLS];
#endif
#if !defined LUNATIC
    intptr_t *vars[MAXGAMEVARS];
    intptr_t *arrays[MAXGAMEARRAYS];
    int32_t arraysiz[MAXGAMEARRAYS];
#else
    char *savecode;
#endif
#ifdef YAX_ENABLE
    int32_t numyaxbunches;
# if !defined NEW_MAP_FORMAT
    int16_t yax_bunchnum[MAXSECTORS][2];
    int16_t yax_nextwall[MAXWALLS][2];
# endif
#endif
} mapstate_t;

extern void G_SaveMapState();
extern void G_RestoreMapState();

typedef struct {
    int32_t partime, designertime;
    char *name, *filename, *musicfn;
    mapstate_t *savedstate;
} map_t;

//extern map_t g_mapInfo[(MAXVOLUMES+1)*MAXLEVELS]; // +1 volume for "intro", "briefing" music

void G_ActivateBySector(int sect,int spriteNum);
int S_FindMusicSFX(int sectNum, int *sndptr);
int A_CallSound(int sectNum,int spriteNum);
int A_CheckHitSprite(int spriteNum,int16_t *hitSprite);
void A_DamageObject_Duke3D(int spriteNum, int dmgSrc);
void A_DamageObject_Generic(int spriteNum, int dmgSrc);
void A_DamageObject(int spriteNum,int dmgSrc);
void A_DamageWall_Internal(int spr, int dawallnum, const vec3_t &pos, int weaponNum);
void A_DamageWall(int spr,int dawallnum,const vec3_t &pos,int weaponNum);
int __fastcall A_FindPlayer(spritetype const *pSprite,int32_t *dist);
void G_AlignWarpElevators(void);
int CheckDoorTile(int tileNum);
void G_AnimateCamSprite(int smoothRatio);
void G_AnimateWalls(void);
int G_ActivateWarpElevators(int s,int warpDir);
int G_CheckActivatorMotion(int lotag);
void G_DoSectorAnimations(void);
void G_OperateActivators(int lotag, int playerNum);
void G_OperateForceFields(int spriteNum,int wallTag);
void G_OperateMasterSwitches(int lotag);
void G_OperateRespawns(int lotag);
void G_OperateSectors(int sectNum,int spriteNum);
void P_HandleSharedKeys(int playerNum);
int GetAnimationGoal(const int32_t *animPtr);
int isanearoperator(int lotag);
int isanunderoperator(int lotag);
int P_ActivateSwitch(int playerNum, int wallOrSprite, int nSwitchType);
void P_CheckSectors(int playerNum);
void Sect_DamageFloor_Internal(int spriteNum, int sectNum);
void Sect_DamageFloor(int spriteNum, int sectNum);
void Sect_DamageCeiling_Internal(int spriteNum, int sectNum);
void Sect_DamageCeiling(int spriteNum, int sectNum);
int SetAnimation(int sectNum,int32_t *animPtr,int goalVal,int animVel);

#define FORCEFIELD_CSTAT (64+16+4+1)

// Returns W_FORCEFIELD if wall has a forcefield overpicnum, its overpicnum else.
static FORCE_INLINE int G_GetForcefieldPicnum(int const wallNum)
{
    int const tileNum = wall[wallNum].overpicnum;
    return tileNum == W_FORCEFIELD + 1 ? W_FORCEFIELD : tileNum;
}

// Returns the interpolated position of the camera that the player is looking
// through (using a viewscreen). <i> should be the player's ->newowner member.
static inline vec3_t G_GetCameraPosition(int32_t const i, int32_t const smoothratio)
{
    auto const cs = (uspriteptr_t)&sprite[i];
    const actor_t *const ca = &actor[i];

    return { ca->bpos.x + mulscale16(cs->x - ca->bpos.x, smoothratio),
             ca->bpos.y + mulscale16(cs->y - ca->bpos.y, smoothratio),
             ca->bpos.z + mulscale16(cs->z - ca->bpos.z, smoothratio) };
}

EXTERN_INLINE_HEADER int32_t G_CheckPlayerInSector(int32_t const sect);

#ifdef __cplusplus
}
#endif

#if defined sector_c_ || !defined DISABLE_INLINING

EXTERN_INLINE int32_t G_CheckPlayerInSector(int32_t const sect)
{
    for (int TRAVERSE_CONNECT(i))
        if ((unsigned)g_player[i].ps->i < MAXSPRITES && sprite[g_player[i].ps->i].sectnum == sect)
            return i;
    return -1;
}

#endif

#endif
