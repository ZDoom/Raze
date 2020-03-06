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

BEGIN_DUKE_NS
// (+ 40 16 16 4 8 6 8 6 4 20)
#pragma pack(push, 1)

// note: fields in this struct DO NOT have to be in this order,
// however if you add something to this struct, please make sure
// a field gets added to ActorFields[], otherwise the field
// won't get synced over the network!
//
// I don't think the ActorFields array needs to be in the same order,
// need to verify this...
typedef struct netactor_s
{
    // actor fields
    //--------------------------------------------
    int32_t
        t_data_0,
        t_data_1,
        t_data_2,
        t_data_3,
        t_data_4,
        t_data_5,
        t_data_6,
        t_data_7,
        t_data_8,
        t_data_9;

    int32_t
        flags;


    int32_t
        bpos_x,
        bpos_y,
        bpos_z;

    int32_t
        floorz,
        ceilingz,
        lastvx,
        lastvy,

        lasttransport,

        picnum,
        ang,
        extra,
        owner,

        movflag,
        tempang,
        timetosleep,

        stayput,
        dispicnum;


    // note: lightId, lightcount, lightmaxrange are not synchronized between client and server

    int32_t
        cgg;


    // sprite fields
    //-----------------------------

    int32_t
        spr_x,
        spr_y,
        spr_z,

        spr_cstat,

        spr_picnum,

        spr_shade,

        spr_pal,
        spr_clipdist,
        spr_blend,

        spr_xrepeat,
        spr_yrepeat,

        spr_xoffset,
        spr_yoffset,

        spr_sectnum,
        spr_statnum,

        spr_ang,
        spr_owner,
        spr_xvel,
        spr_yvel,
        spr_zvel,

        spr_lotag,
        spr_hitag,

        spr_extra;

    //---------------------------------------------
    //spriteext fields

    int32_t
        ext_mdanimtims,

        ext_mdanimcur,
        ext_angoff,
        ext_pitch,
        ext_roll,

        ext_pivot_offset_x,
        ext_pivot_offset_y,
        ext_pivot_offset_z,

        ext_position_offset_x,
        ext_position_offset_y,
        ext_position_offset_z,

        ext_flags,
        ext_xpanning,
        ext_ypanning;

    float       ext_alpha;

    // DON'T send tsprites over the internet

    //--------------------------------------------
    //spritesmooth fields

    float       sm_smoothduration;

    int32_t
        sm_mdcurframe,
        sm_mdoldframe,

        sm_mdsmooth;

    //--------------------------------------------
    // SpriteProjectile fields

    // may want to put projectile fields here
    int32_t netIndex;

} netactor_t;
#pragma pack(pop)

extern tiledata_t   g_tile[MAXTILES];
extern actor_t      actor[MAXSPRITES];
extern int32_t      block_deletesprite;
extern int32_t      g_noEnemies;
extern int32_t      otherp;
extern int32_t      ticrandomseed;
extern projectile_t SpriteProjectile[MAXSPRITES];
extern uint8_t      g_radiusDmgStatnums[(MAXSTATUS+7)>>3];

int  A_CheckNoSE7Water(uspriteptr_t pSprite, int sectNum, int sectLotag, int32_t *pOther);
int  A_CheckSwitchTile(int spriteNum);
int A_IncurDamage(int spriteNum);
void A_AddToDeleteQueue(int spriteNum);
void A_DeleteSprite(int spriteNum);
void A_DoGuts(int spriteNum, int tileNum, int spawnCnt);
void A_DoGutsDir(int spriteNum, int tileNum, int spawnCnt);
int A_GetClipdist(int spriteNum, int clipDist);
void A_MoveCyclers(void);
void A_MoveDummyPlayers(void);
void A_MoveSector(int spriteNum);
void A_PlayAlertSound(int spriteNum);
void A_RadiusDamage(int spriteNum, int blastRadius, int dmg1, int dmg2, int dmg3, int dmg4);
void A_SpawnMultiple(int spriteNum, int tileNum, int spawnCnt);

int  G_SetInterpolation(int32_t *posptr);
void G_AddGameLight(int lightRadius, int spriteNum, int zOffset, int lightRange, int lightColor, int lightPrio);
void G_ClearCameraView(DukePlayer_t *ps);
void G_DoInterpolations(int smoothRatio);
void G_MoveWorld(void);
void G_RefreshLights(void);
void G_StopInterpolation(const int32_t *posptr);

inline int A_CheckEnemyTile(int tileNum);
inline int A_SetSprite(int spriteNum, uint32_t cliptype);
inline int32_t A_MoveSprite(int spriteNum, vec3_t const* change, uint32_t cliptype);

inline int G_CheckForSpaceCeiling(int sectnum);
inline int G_CheckForSpaceFloor(int sectnum);

inline int A_CheckEnemySprite(void const* s);

// PK 20110701: changed input argument: int32_t i (== sprite, whose sectnum...) --> sectnum directly
void                Sect_ToggleInterpolation(int sectnum, int setInterpolation);
inline void   Sect_ClearInterpolation(int sectnum) { Sect_ToggleInterpolation(sectnum, 0); }
inline void   Sect_SetInterpolation(int sectnum) { Sect_ToggleInterpolation(sectnum, 1); }

extern int32_t A_MoveSpriteClipdist(int32_t spritenum, vec3_t const * change, uint32_t cliptype, int32_t clipdist);


inline int A_CheckEnemyTile(int const tileNum)
{
    return ((g_tile[tileNum].flags & (SFLAG_HARDCODED_BADGUY | SFLAG_BADGUY)) != 0);
}

inline int A_SetSprite(int const spriteNum, uint32_t cliptype)
{
    vec3_t const davect = { (sprite[spriteNum].xvel * (sintable[(sprite[spriteNum].ang + 512) & 2047])) >> 14,
                      (sprite[spriteNum].xvel * (sintable[sprite[spriteNum].ang & 2047])) >> 14, sprite[spriteNum].zvel };
    return (A_MoveSprite(spriteNum, &davect, cliptype) == 0);
}

inline int A_SetSpriteNoZ(int const spriteNum, uint32_t cliptype)
{
    vec3_t const davect = { (sprite[spriteNum].xvel * (sintable[(sprite[spriteNum].ang + 512) & 2047])) >> 14,
                      (sprite[spriteNum].xvel * (sintable[sprite[spriteNum].ang & 2047])) >> 14, 0 };
    return (A_MoveSprite(spriteNum, &davect, cliptype) == 0);
}

inline int32_t A_MoveSprite(int const spriteNum, vec3_t const * const change, uint32_t cliptype)
{
    return A_MoveSpriteClipdist(spriteNum, change, cliptype, -1);
}

inline int G_CheckForSpaceCeiling(int const sectnum)
{
    return ((sector[sectnum].ceilingstat&1) && sector[sectnum].ceilingpal == 0 &&
            (sector[sectnum].ceilingpicnum==MOONSKY1 || sector[sectnum].ceilingpicnum==BIGORBIT1));
}

inline int G_CheckForSpaceFloor(int const sectnum)
{
    return ((sector[sectnum].floorstat&1) && sector[sectnum].ceilingpal == 0 &&
            (sector[sectnum].floorpicnum==MOONSKY1 || sector[sectnum].floorpicnum==BIGORBIT1));
}

inline int A_CheckEnemySprite(void const * const pSprite)
{
    return A_CheckEnemyTile(((uspriteptr_t) pSprite)->picnum);
}


END_DUKE_NS

#endif
