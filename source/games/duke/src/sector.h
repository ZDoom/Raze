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
#include "mathutil.h"

BEGIN_DUKE_NS

#define MAXCYCLERS      1024
#define MAXANIMATES     1024
#define MAXANIMWALLS    512
#define MAXANIMPOINTS   2048

#define VIEWSCREEN_ACTIVE_DISTANCE 8192

extern uint8_t g_shadedSector[MAXSECTORS];

typedef struct {
    int16_t wallnum, tag;
} animwalltype;

typedef struct {
    // this needs to have a copy of everything related to the map/actor state
    // see savegame.c
    int32_t g_animateGoal[MAXANIMATES], g_animateVel[MAXANIMATES], g_animateCnt;
    intptr_t g_animatePtr[MAXANIMATES];
    int32_t lockclock;
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

    FixedBitArray<MAXSECTORS> show2dsector;

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
    intptr_t *vars[MAXGAMEVARS];
#ifdef YAX_ENABLE
    int32_t numyaxbunches;
# if !defined NEW_MAP_FORMAT
    int16_t yax_bunchnum[MAXSECTORS][2];
    int16_t yax_nextwall[MAXWALLS][2];
# endif
#endif
} mapstate_t;

typedef struct {
    mapstate_t *savedstate;
} map_t;


void G_ActivateBySector(int sect,int spriteNum);
inline void activatebysector(int s, int sn)
{
    G_ActivateBySector(s, sn);
}
int S_FindMusicSFX(int sectNum, int *sndptr);
void callsound2(int soundNum, int playerNum);
int callsound(int sectNum,int spriteNum);
int A_CheckHitSprite(int spriteNum,int16_t *hitSprite);
inline int hitasprite(int s, int16_t* h)
{
    return A_CheckHitSprite(s, h);
}
void A_DamageObject(int spriteNum,int dmgSrc);
inline void checkhitsprite(int s, int d)
{
    A_DamageObject(s, d);
}
void A_DamageWall(int spr,int dawallnum,const vec3_t *pos,int weaponNum);
inline void checkhitwall(int spr, int wal, int x, int y, int z, int w)
{
    vec3_t vec{ x, y, z };
    A_DamageWall(spr, wal, &vec, w);
}
int __fastcall A_FindPlayer(const spritetype *pSprite,int32_t *dist);
inline int findplayer(const spritetype* pSprite, int32_t* dist)
{
    return A_FindPlayer(pSprite, dist);
}
void G_AlignWarpElevators(void);
bool isadoorwall(int tileNum);
bool isablockdoor(int tileNum);
void G_AnimateCamSprite(int smoothRatio);
void G_AnimateWalls(void);
int G_ActivateWarpElevators(int s,int warpDir);
inline int activatewarpelevators(int s, int w)
{
    return G_ActivateWarpElevators(s, w);
}
int check_activator_motion(int lotag);
void G_DoSectorAnimations(void);
void G_OperateActivators(int lotag, int playerNum);
inline void operateactivators(int l, int w)
{
    G_OperateActivators(l, w);
}
void G_OperateForceFields(int spriteNum,int wallTag);
inline void operateforcefields(int s, int w)
{
    G_OperateForceFields(s, w);
}
void G_OperateMasterSwitches(int lotag);
inline void operatemasterswitches(int l)
{
    return G_OperateMasterSwitches(l);
}
void G_OperateRespawns(int lotag);
void G_OperateSectors(int sectNum,int spriteNum);
inline void operatesectors(int s, int i)
{
    G_OperateSectors(s, i);
}
void P_HandleSharedKeys(int playerNum);
int GetAnimationGoal(const int32_t *animPtr);
inline int getanimationgoal(const int32_t* animPtr)
{
    return GetAnimationGoal(animPtr);
}
bool isanearoperator(int lotag);
bool isanunderoperator(int lotag);
int P_ActivateSwitch(int playerNum, int wallOrSprite, int nSwitchType);
void P_CheckSectors(int playerNum);
void Sect_DamageCeiling(int sectNum);
inline void checkhitceiling(int sec) { Sect_DamageCeiling(sec); }
int SetAnimation(int sectNum,int32_t *animPtr,int goalVal,int animVel);
inline int setanimation(int sectNum, int32_t* animPtr, int goalVal, int animVel)
{
    return SetAnimation(sectNum, animPtr, goalVal, animVel);
}
void G_DoFurniture(int wallNum, int sectNum, int playerNum);
void dotorch();

#define FORCEFIELD_CSTAT (64+16+4+1)

// Returns TILE_W_FORCEFIELD if wall has a forcefield overpicnum, its overpicnum else.
static inline int G_GetForcefieldPicnum(int wallNum)
{
    int tileNum = wall[wallNum].overpicnum;
    if (tileNum == TILE_W_FORCEFIELD + 1 || tileNum == TILE_W_FORCEFIELD + 2)
        tileNum = TILE_W_FORCEFIELD;
    return tileNum;
}

// Returns the interpolated position of the camera that the player is looking
// through (using a viewscreen). <i> should be the player's ->newowner member.
static inline vec3_t G_GetCameraPosition(int32_t i, int32_t smoothratio)
{
    const spritetype *const cs = &sprite[i];
    const actor_t *const ca = &actor[i];

    vec3_t cam = { ca->bpos.x + mulscale16(cs->x - ca->bpos.x, smoothratio),
                   ca->bpos.y + mulscale16(cs->y - ca->bpos.y, smoothratio),
                   ca->bpos.z + mulscale16(cs->z - ca->bpos.z, smoothratio)
                 };
    return cam;
}

EXTERN_INLINE_HEADER int32_t G_CheckPlayerInSector(int32_t sect);

#if defined sector_c_ || !defined DISABLE_INLINING

EXTERN_INLINE int32_t G_CheckPlayerInSector(int32_t sect)
{
    int32_t i;
    for (TRAVERSE_CONNECT(i))
        if ((unsigned)g_player[i].ps->i < MAXSPRITES && sprite[g_player[i].ps->i].sectnum == sect)
            return i;
    return -1;
}

inline int checkcursectnums(int se)
{
    return G_CheckPlayerInSector(se);
}

inline int ldist(const spritetype* s1, const spritetype* s2)
{
    int vx, vy;
    vx = s1->x - s2->x;
    vy = s1->y - s2->y;
    return(FindDistance2D(vx, vy) + 1);
}

inline int dist(const spritetype* s1, const spritetype* s2)
{
    int vx, vy, vz;
    vx = s1->x - s2->x;
    vy = s1->y - s2->y;
    vz = s1->z - s2->z;
    return(FindDistance3D(vx, vy, vz >> 4));
}


#endif

END_DUKE_NS

#endif
