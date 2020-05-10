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


void breakwall(short newpn, short spr, short dawallnum);
void activatebysector(int s, int sn);
int S_FindMusicSFX(int sectNum, int *sndptr);
void callsound2(int soundNum, int playerNum);
int callsound(int sectNum,int spriteNum);
int A_CheckHitSprite(int spriteNum,int16_t *hitSprite);
inline int hitasprite(int s, int16_t* h)
{
    return A_CheckHitSprite(s, h);
}
void checkhitsprite(int s, int d);
void checkhitwall(int spr, int wal, int x, int y, int z, int w);
inline void A_DamageWall(int spr, int dawallnum, const vec3_t* pos, int weaponNum)
{
    checkhitwall(spr, dawallnum, pos->x, pos->y, pos->z, weaponNum);
}

int findplayer(const spritetype* pSprite, int32_t* dist);
void operatejaildoors(int hitag);
void allignwarpelevators(void);
bool isadoorwall(int tileNum);
bool isablockdoor(int tileNum);
void G_AnimateCamSprite(int smoothRatio);
void animatewalls(void);
bool activatewarpelevators(int s, int w);
int check_activator_motion(int lotag);
void operateactivators(int l, int w);
void operateforcefields(int s,int low);
void operateforcefields_common(int s, int low, const std::initializer_list<int>& tiles);
void operatemasterswitches(int lotag);
void operaterespawns(int lotag);
void operatesectors(int s, int i);
void P_HandleSharedKeys(int playerNum);
int getanimationgoal(const int32_t* animPtr);
bool isanearoperator(int lotag);
bool isanunderoperator(int lotag);
bool checkhitswitch(int playerNum, int wallOrSprite, int nSwitchType);
void checksectors(int playerNum);
bool checkhitceiling(int sec);
int setanimation(short animsect, int* animptr, int thegoal, int thevel);
void dofurniture(int wallNum, int sectNum, int playerNum);
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

enum { SWITCH_WALL, SWITCH_SPRITE };

#endif

END_DUKE_NS

#endif
