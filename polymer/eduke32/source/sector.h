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

#include "gamevars.h"
#include "actors.h"  // actor_t
#include "player.h"  // playerspawn_t
#include "namesdyn.h"  // for G_GetForcefieldPicnum()

#ifdef __cplusplus
extern "C" {
#endif

#define MAXCYCLERS      1024
#define MAXANIMATES     256
#define MAXANIMWALLS    512

#define VIEWSCREEN_ACTIVE_DISTANCE 8192

typedef struct {
    int16_t wallnum, tag;
} animwalltype;

typedef struct {
    // this needs to have a copy of everything related to the map/actor state
    // see savegame.c
    int32_t animategoal[MAXANIMATES], animatevel[MAXANIMATES], g_animateCount;
    intptr_t animateptr[MAXANIMATES];
    int32_t lockclock;
    int32_t msx[2048], msy[2048];
    int32_t randomseed, g_globalRandom;
    int32_t pskyidx;

    int16_t SpriteDeletionQueue[1024],g_spriteDeleteQueuePos;
    int16_t animatesect[MAXANIMATES];
    int16_t cyclers[MAXCYCLERS][6];
    int16_t g_mirrorWall[64], g_mirrorSector[64], g_mirrorCount;
    int16_t g_numAnimWalls;
    int16_t g_numClouds,clouds[256],cloudx,cloudy;
    int16_t g_numCyclers;

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
    int8_t g_numPlayerSprites;

    uint8_t show2dsector[(MAXSECTORS+7)>>3];

    actor_t actor[MAXSPRITES];
    playerspawn_t g_playerSpawnPoints[MAXPLAYERS];
    animwalltype animwall[MAXANIMWALLS];
    tsectortype sector[MAXSECTORS];
    spriteext_t spriteext[MAXSPRITES];
    tspritetype sprite[MAXSPRITES];
    twalltype wall[MAXWALLS];
#if !defined LUNATIC
    intptr_t *vars[MAXGAMEVARS];
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

//extern map_t MapInfo[(MAXVOLUMES+1)*MAXLEVELS]; // +1 volume for "intro", "briefing" music

void G_ActivateBySector(int32_t sect,int32_t j);
int32_t S_FindMusicSFX(int32_t sn, int32_t *sndptr);
int32_t A_CallSound(int32_t sn,int32_t whatsprite);
int32_t A_CheckHitSprite(int32_t i,int16_t *hitsp);
void A_DamageObject(int32_t i,int32_t sn);
void A_DamageWall(int32_t spr,int32_t dawallnum,const vec3_t *pos,int32_t atwith);
int32_t __fastcall A_FindPlayer(const spritetype *s,int32_t *d);
void G_AlignWarpElevators(void);
int32_t CheckDoorTile(int32_t dapic);
void G_AnimateCamSprite(int32_t smoothratio);
void G_AnimateWalls(void);
int32_t G_ActivateWarpElevators(int32_t s,int32_t d);
int32_t G_CheckActivatorMotion(int32_t lotag);
void G_DoSectorAnimations(void);
void G_OperateActivators(int32_t low,int32_t snum);
void G_OperateForceFields(int32_t s,int32_t low);
void G_OperateMasterSwitches(int32_t low);
void G_OperateRespawns(int32_t low);
void G_OperateSectors(int32_t sn,int32_t ii);
void P_HandleSharedKeys(int32_t snum);
int32_t GetAnimationGoal(const int32_t *animptr);
int32_t isanearoperator(int32_t lotag);
int32_t isanunderoperator(int32_t lotag);
int32_t P_ActivateSwitch(int32_t snum,int32_t w,int32_t switchissprite);
void P_CheckSectors(int32_t snum);
int32_t Sect_DamageCeilingOrFloor(int32_t floorp, int32_t sn);
int32_t SetAnimation(int32_t animsect,int32_t *animptr,int32_t thegoal,int32_t thevel);

#define FORCEFIELD_CSTAT (64+16+4+1)

// Returns W_FORCEFIELD if wall has a forcefield overpicnum, its overpicnum else.
static inline int32_t G_GetForcefieldPicnum(int32_t wallnum)
{
    int32_t picnum = wall[wallnum].overpicnum;
    if (picnum == W_FORCEFIELD+1)
        picnum = W_FORCEFIELD;
    return picnum;
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

#ifdef __cplusplus
}
#endif

#include "sector_inline.h"

#endif
