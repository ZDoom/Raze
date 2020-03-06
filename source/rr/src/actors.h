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

#ifndef actors_h_
#define actors_h_

#include "player.h"
# include "dukerr/namesdyn.h"
#include "dukerr/actors.h"

BEGIN_RR_NS

// (+ 40 16 16 4 8 6 8 6 4 20)
#pragma pack(push, 1)
// this struct needs to match the beginning of actor_t above
typedef struct
{
    int32_t t_data[10];  // 40b sometimes used to hold offsets to con code

    int32_t flags;                             // 4b
    vec3_t  bpos;                              // 12b
    int32_t floorz, ceilingz;                  // 8b
    vec2_t lastv;                              // 8b
    int16_t picnum, ang, extra, owner;         // 8b
    int16_t movflag, tempang, timetosleep;     // 6b
    int16_t stayput;

    uint8_t cgg, lasttransport;

    spritetype sprite;
    int16_t    netIndex;
} netactor_t;
#pragma pack(pop)
extern tiledata_t   g_tile[MAXTILES];
extern actor_t      actor[MAXSPRITES];
extern int32_t      block_deletesprite;
extern int32_t      g_noEnemies;
extern int32_t      otherp;
extern int32_t      ticrandomseed;
extern int g_canSeePlayer;


int  A_CheckNoSE7Water(uspritetype const *pSprite, int sectNum, int sectLotag, int32_t *pOther);
int  A_CheckSwitchTile(int spriteNum);
int A_IncurDamage(int spriteNum);
void A_AddToDeleteQueue(int spriteNum);
void A_DeleteSprite(int spriteNum);
void A_DoGuts(int spriteNum, int tileNum, int spawnCnt);
void A_DoGutsDir(int spriteNum, int tileNum, int spawnCnt);
void A_MoveCyclers(void);
void A_MoveDummyPlayers(void);
void A_MoveSector(int spriteNum);
void A_PlayAlertSound(int spriteNum);
void A_RadiusDamage(int spriteNum, int blastRadius, int dmg1, int dmg2, int dmg3, int dmg4);
void A_SpawnMultiple(int spriteNum, int tileNum, int spawnCnt);
void A_ResetLanePics(void);

int  G_SetInterpolation(int32_t *posptr);
void G_AddGameLight(int lightRadius, int spriteNum, int zOffset, int lightRange, int lightColor, int lightPrio);
void G_ClearCameraView(DukePlayer_t *ps);
void G_DoInterpolations(int smoothRatio);
void G_MoveWorld(void);
void G_RefreshLights(void);
void G_StopInterpolation(const int32_t *posptr);

// PK 20110701: changed input argument: int32_t i (== sprite, whose sectnum...) --> sectnum directly
void                Sect_ToggleInterpolation(int sectnum, int setInterpolation);
static FORCE_INLINE void   Sect_ClearInterpolation(int sectnum) { Sect_ToggleInterpolation(sectnum, 0); }
static FORCE_INLINE void   Sect_SetInterpolation(int sectnum) { Sect_ToggleInterpolation(sectnum, 1); }

#if KRANDDEBUG
# define ACTOR_INLINE __fastcall
# define ACTOR_INLINE_HEADER extern __fastcall
#else
# define ACTOR_INLINE EXTERN_INLINE
# define ACTOR_INLINE_HEADER EXTERN_INLINE_HEADER
#endif

extern int32_t A_MoveSprite(int32_t spritenum, vec3_t const * change, uint32_t cliptype);
ACTOR_INLINE_HEADER int A_CheckEnemyTile(int tileNum);
ACTOR_INLINE_HEADER int A_SetSprite(int spriteNum, uint32_t cliptype);

EXTERN_INLINE_HEADER int G_CheckForSpaceCeiling(int sectnum);
EXTERN_INLINE_HEADER int G_CheckForSpaceFloor(int sectnum);

EXTERN_INLINE_HEADER int A_CheckEnemySprite(void const * s);

#if defined actors_c_ || !defined DISABLE_INLINING

# if !KRANDDEBUG || (KRANDDEBUG && defined actors_c_)

ACTOR_INLINE int A_CheckEnemyTile(int const tileNum)
{
    return ((g_tile[tileNum].flags & (SFLAG_BADGUY_TILE | SFLAG_BADGUY)) != 0);
}

ACTOR_INLINE int A_SetSprite(int const spriteNum, uint32_t cliptype)
{
    vec3_t davect = { (sprite[spriteNum].xvel * (sintable[(sprite[spriteNum].ang + 512) & 2047])) >> 14,
                      (sprite[spriteNum].xvel * (sintable[sprite[spriteNum].ang & 2047])) >> 14, sprite[spriteNum].zvel };
    return (A_MoveSprite(spriteNum, &davect, cliptype) == 0);
}

# endif


EXTERN_INLINE int G_CheckForSpaceCeiling(int const sectnum)
{
    return ((sector[sectnum].ceilingstat&1) && sector[sectnum].ceilingpal == 0 &&
            (sector[sectnum].ceilingpicnum==MOONSKY1 || sector[sectnum].ceilingpicnum==BIGORBIT1));
}

EXTERN_INLINE int G_CheckForSpaceFloor(int const sectnum)
{
    return ((sector[sectnum].floorstat&1) && sector[sectnum].ceilingpal == 0 &&
            (sector[sectnum].floorpicnum==MOONSKY1 || sector[sectnum].floorpicnum==BIGORBIT1));
}

EXTERN_INLINE int A_CheckEnemySprite(void const * const pSprite)
{
    return A_CheckEnemyTile(((uspritetype const *) pSprite)->picnum);
}

#endif

END_RR_NS

#endif
