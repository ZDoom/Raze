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

#define sector_c_

#include "duke3d.h"
#include "input.h"

// PRIMITIVE

static int g_haltSoundHack = 0;

int S_FindMusicSFX(int sectNum, int *sndptr)
{
    for (bssize_t SPRITES_OF_SECT(sectNum, spriteNum))
    {
        const int32_t snd = sprite[spriteNum].lotag;
        EDUKE32_STATIC_ASSERT(MAXSOUNDS >= 1000);

        if (PN(spriteNum) == MUSICANDSFX && (unsigned)snd < 1000)  // XXX: in other places, 999
        {
            *sndptr = snd;
            return spriteNum;
        }
    }

    *sndptr = -1;
    return -1;
}

// this function activates a sector's MUSICANDSFX sprite
int A_CallSound(int sectNum, int spriteNum)
{
    if (g_haltSoundHack)
    {
        g_haltSoundHack = 0;
        return -1;
    }

    int soundNum;
    int const SFXsprite = S_FindMusicSFX(sectNum, &soundNum);

    if (SFXsprite >= 0)
    {
        if (spriteNum == -1)
            spriteNum = SFXsprite;

        if (T1(SFXsprite) == 0)
        {
            if ((g_sounds[soundNum].m & (SF_GLOBAL|SF_DTAG)) != SF_GLOBAL)
            {
                if (soundNum)
                {
                    A_PlaySound(soundNum, spriteNum);

                    if (SHT(SFXsprite) && soundNum != SHT(SFXsprite) && SHT(SFXsprite) < MAXSOUNDS)
                        S_StopEnvSound(SHT(SFXsprite),T6(SFXsprite));

                    T6(SFXsprite) = spriteNum;
                }

                if ((sector[SECT(SFXsprite)].lotag&0xff) != ST_22_SPLITTING_DOOR)
                    T1(SFXsprite) = 1;
            }
        }
        else if (SHT(SFXsprite) < MAXSOUNDS)
        {
            if (SHT(SFXsprite))
                A_PlaySound(SHT(SFXsprite), spriteNum);

            if ((g_sounds[soundNum].m & SF_LOOP) || (SHT(SFXsprite) && SHT(SFXsprite) != soundNum))
                S_StopEnvSound(soundNum, T6(SFXsprite));

            T6(SFXsprite) = spriteNum;
            T1(SFXsprite) = 0;
        }

        return soundNum;
    }

    return -1;
}

int G_CheckActivatorMotion(int lotag)
{
    int spriteNum = headspritestat[STAT_ACTIVATOR];

    while (spriteNum >= 0)
    {
        if (sprite[spriteNum].lotag == lotag)
        {
            auto const pSprite = &sprite[spriteNum];

            for (bssize_t j = g_animateCnt - 1; j >= 0; j--)
                if (pSprite->sectnum == g_animateSect[j])
                    return 1;

            int sectorEffector = headspritestat[STAT_EFFECTOR];

            while (sectorEffector >= 0)
            {
                if (pSprite->sectnum == sprite[sectorEffector].sectnum)
                {
                    switch (sprite[sectorEffector].lotag)
                    {
                        case SE_11_SWINGING_DOOR:
                        case SE_30_TWO_WAY_TRAIN:
                            if (actor[sectorEffector].t_data[4])
                                return 1;
                            break;
                        case SE_20_STRETCH_BRIDGE:
                        case SE_31_FLOOR_RISE_FALL:
                        case SE_32_CEILING_RISE_FALL:
                        case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
                            if (actor[sectorEffector].t_data[0])
                                return 1;
                            break;
                    }
                }

                sectorEffector = nextspritestat[sectorEffector];
            }
        }
        spriteNum = nextspritestat[spriteNum];
    }
    return 0;
}

int CheckDoorTile(int tileNum)
{
#ifndef EDUKE32_STANDALONE
    switch (DYNAMICTILEMAP(tileNum))
    {
        case DOORTILE1__STATIC:
        case DOORTILE2__STATIC:
        case DOORTILE3__STATIC:
        case DOORTILE4__STATIC:
        case DOORTILE5__STATIC:
        case DOORTILE6__STATIC:
        case DOORTILE7__STATIC:
        case DOORTILE8__STATIC:
        case DOORTILE9__STATIC:
        case DOORTILE10__STATIC:
        case DOORTILE11__STATIC:
        case DOORTILE12__STATIC:
        case DOORTILE14__STATIC:
        case DOORTILE15__STATIC:
        case DOORTILE16__STATIC:
        case DOORTILE17__STATIC:
        case DOORTILE18__STATIC:
        case DOORTILE19__STATIC:
        case DOORTILE20__STATIC:
        case DOORTILE21__STATIC:
        case DOORTILE22__STATIC:
        case DOORTILE23__STATIC:
            return 1;
    }
#else
    UNREFERENCED_PARAMETER(tileNum);
#endif
    return 0;
}

int isanunderoperator(int lotag)
{
    switch (lotag & 0xff)
    {
        case ST_15_WARP_ELEVATOR:
        case ST_16_PLATFORM_DOWN:
        case ST_17_PLATFORM_UP:
        case ST_18_ELEVATOR_DOWN:
        case ST_19_ELEVATOR_UP:
        case ST_22_SPLITTING_DOOR:
        case ST_26_SPLITTING_ST_DOOR:
            return 1;
    }
    return 0;
}

int isanearoperator(int lotag)
{
    switch (lotag & 0xff)
    {
        case ST_9_SLIDING_ST_DOOR:
        case ST_15_WARP_ELEVATOR:
        case ST_16_PLATFORM_DOWN:
        case ST_17_PLATFORM_UP:
        case ST_18_ELEVATOR_DOWN:
        case ST_19_ELEVATOR_UP:
        case ST_20_CEILING_DOOR:
        case ST_21_FLOOR_DOOR:
        case ST_22_SPLITTING_DOOR:
        case ST_23_SWINGING_DOOR:
        case ST_25_SLIDING_DOOR:
        case ST_26_SPLITTING_ST_DOOR:
        case ST_29_TEETH_DOOR:
            return 1;
    }
    return 0;
}

static inline int32_t A_FP_ManhattanDist(const DukePlayer_t *pPlayer, const spritetype *pSprite)
{
    return klabs(pPlayer->opos.x - pSprite->x)
           + klabs(pPlayer->opos.y - pSprite->y)
           + ((klabs(pPlayer->opos.z - pSprite->z + (28 << 8))) >> 4);
}

int __fastcall A_FindPlayer(const spritetype *pSprite, int32_t *dist)
{
    if (!g_netServer && ud.multimode < 2)
    {
        auto const pPlayer = g_player[myconnectindex].ps;

        if (dist)
            *dist = A_FP_ManhattanDist(pPlayer, pSprite);

        return myconnectindex;
    }

    int     closestPlayer     = 0;
    int32_t closestPlayerDist = INT32_MAX;

    for (bssize_t TRAVERSE_CONNECT(j))
    {
        auto const pPlayer    = g_player[j].ps;
        int32_t             playerDist = A_FP_ManhattanDist(pPlayer, pSprite);

        if (playerDist < closestPlayerDist && sprite[pPlayer->i].extra > 0)
        {
            closestPlayer     = j;
            closestPlayerDist = playerDist;
        }
    }

    if (dist)
        *dist = closestPlayerDist;

    return closestPlayer;
}

void G_DoSectorAnimations(void)
{
    for (bssize_t animNum=g_animateCnt-1; animNum>=0; animNum--)
    {
        int       animPos  = *g_animatePtr[animNum];
        int const animVel  = g_animateVel[animNum] * TICSPERFRAME;
        int const animSect = g_animateSect[animNum];

        if (animPos == g_animateGoal[animNum])
        {
            G_StopInterpolation(g_animatePtr[animNum]);

            // This fixes a bug where wall or floor sprites contained in
            // elevator sectors (ST 16-19) would jitter vertically after the
            // elevator had stopped.
            if (g_animatePtr[animNum] == &sector[g_animateSect[animNum]].floorz)
            {
                for (bssize_t j=headspritesect[animSect]; j>=0; j=nextspritesect[j])
                {
                    if (sprite[j].statnum != STAT_EFFECTOR)
                        actor[j].bpos.z = sprite[j].z;
                }
            }

            g_animateCnt--;

            g_animatePtr[animNum]  = g_animatePtr[g_animateCnt];
            g_animateGoal[animNum] = g_animateGoal[g_animateCnt];
            g_animateVel[animNum]  = g_animateVel[g_animateCnt];
            g_animateSect[animNum] = g_animateSect[g_animateCnt];

            if ((sector[g_animateSect[animNum]].lotag == ST_18_ELEVATOR_DOWN || sector[g_animateSect[animNum]].lotag == ST_19_ELEVATOR_UP)
                && (g_animatePtr[animNum] == &sector[g_animateSect[animNum]].ceilingz))
                continue;

            if ((sector[animSect].lotag&0xff) != ST_22_SPLITTING_DOOR)
                A_CallSound(animSect,-1);

            continue;
        }

        animPos = (animVel > 0) ? min(animPos + animVel, g_animateGoal[animNum])
                                : max(animPos + animVel, g_animateGoal[animNum]);

        if (g_animatePtr[animNum] == &sector[g_animateSect[animNum]].floorz)
        {
            for (bssize_t TRAVERSE_CONNECT(playerNum))
            {
                if ((g_player[playerNum].ps->cursectnum == animSect)
                    && ((sector[animSect].floorz - g_player[playerNum].ps->pos.z) < (64 << 8))
                    && (sprite[g_player[playerNum].ps->i].owner >= 0))
                {
                    g_player[playerNum].ps->pos.z += animVel;
                    g_player[playerNum].ps->vel.z = 0;
                    /*
                                                if (p == myconnectindex)
                                                {
                                                    my.z += v;
                                                    myvel.z = 0;
                                                }
                    */
                }
            }

            for (bssize_t j=headspritesect[animSect]; j>=0; j=nextspritesect[j])
            {
                if (sprite[j].statnum != STAT_EFFECTOR)
                {
                    actor[j].bpos.z = sprite[j].z;
                    sprite[j].z += animVel;
                    actor[j].floorz = sector[animSect].floorz+animVel;
                }
            }
        }

        *g_animatePtr[animNum] = animPos;
    }
}

int GetAnimationGoal(const int32_t *animPtr)
{
    for (bssize_t i = 0; i < g_animateCnt; i++)
        if (animPtr == g_animatePtr[i])
            return i;
    return -1;
}

int SetAnimation(int sectNum, int32_t *animPtr, int goalVal, int animVel)
{
    if (g_animateCnt >= MAXANIMATES)
        return -1;

    int animNum = g_animateCnt;

    for (bssize_t i = 0; i < g_animateCnt; i++)
    {
        if (animPtr == g_animatePtr[i])
        {
            animNum = i;
            break;
        }
    }

    g_animateSect[animNum] = sectNum;
    g_animatePtr[animNum]  = animPtr;
    g_animateGoal[animNum] = goalVal;
    g_animateVel[animNum]  = (goalVal >= *animPtr) ? animVel : -animVel;

    if (animNum == g_animateCnt)
        g_animateCnt++;

    G_SetInterpolation(animPtr);

    return animNum;
}

static void G_SetupCamTile(int spriteNum, int tileNum, int smoothRatio)
{
    int const playerNum = screenpeek;

    vec3_t const camera     = G_GetCameraPosition(spriteNum, smoothRatio);
    int const    saveMirror = display_mirror;

    //if (waloff[wn] == 0) loadtile(wn);
    renderSetTarget(tileNum, tilesiz[tileNum].y, tilesiz[tileNum].x);

    int const noDraw = VM_OnEventWithReturn(EVENT_DISPLAYROOMSCAMERATILE, spriteNum, playerNum, 0);

    if (noDraw == 1)
        goto finishTileSetup;
#ifdef DEBUGGINGAIDS
    else if (EDUKE32_PREDICT_FALSE(noDraw != 0)) // event return values other than 0 and 1 are reserved
        OSD_Printf(OSD_ERROR "ERROR: EVENT_DISPLAYROOMSCAMERATILE return value must be 0 or 1, "
                   "other values are reserved.\n");
#endif

    yax_preparedrawrooms();
    drawrooms(camera.x, camera.y, camera.z, SA(spriteNum), 100 + sprite[spriteNum].shade, SECT(spriteNum));
    yax_drawrooms(G_DoSpriteAnimations, SECT(spriteNum), 0, smoothRatio);

    display_mirror = 3;
    G_DoSpriteAnimations(camera.x, camera.y, SA(spriteNum), smoothRatio);
    display_mirror = saveMirror;
    renderDrawMasks();

finishTileSetup:
    renderRestoreTarget();
    squarerotatetile(tileNum);
    tileInvalidate(tileNum, -1, 255);
}

void G_AnimateCamSprite(int smoothRatio)
{
#ifdef DEBUG_VALGRIND_NO_SMC
    return;
#endif

    if (g_curViewscreen < 0)
        return;

    int const spriteNum = g_curViewscreen;

    if (totalclock >= T1(spriteNum) + ud.camera_time)
    {
        auto const pPlayer = g_player[screenpeek].ps;

        if (pPlayer->newowner >= 0)
            OW(spriteNum) = pPlayer->newowner;

        if (OW(spriteNum) >= 0 && dist(&sprite[pPlayer->i], &sprite[spriteNum]) < VIEWSCREEN_ACTIVE_DISTANCE)
        {
            int const viewscrShift = G_GetViewscreenSizeShift((uspriteptr_t)&sprite[spriteNum]);
            int const viewscrTile  = TILE_VIEWSCR - viewscrShift;

            if (waloff[viewscrTile] == 0)
                tileCreate(viewscrTile, tilesiz[PN(spriteNum)].x << viewscrShift, tilesiz[PN(spriteNum)].y << viewscrShift);
            else
                walock[viewscrTile] = 199;

            G_SetupCamTile(OW(spriteNum), viewscrTile, smoothRatio);
#ifdef POLYMER
            // Force texture update on viewscreen sprite in Polymer!
            if (videoGetRenderMode() == REND_POLYMER)
                polymer_invalidatesprite(spriteNum);
#endif
        }

        T1(spriteNum) = (int32_t) totalclock;
    }
}

void G_AnimateWalls(void)
{
    for (bssize_t animwallNum = g_animWallCnt-1; animwallNum>=0; animwallNum--)
    {
        int const wallNum = animwall[animwallNum].wallnum;

        switch (DYNAMICTILEMAP(wall[wallNum].picnum))
        {
        case SCREENBREAK1__STATIC:
        case SCREENBREAK2__STATIC:
        case SCREENBREAK3__STATIC:
        case SCREENBREAK4__STATIC:
        case SCREENBREAK5__STATIC:

        case SCREENBREAK9__STATIC:
        case SCREENBREAK10__STATIC:
        case SCREENBREAK11__STATIC:
        case SCREENBREAK12__STATIC:
        case SCREENBREAK13__STATIC:
        case SCREENBREAK14__STATIC:
        case SCREENBREAK15__STATIC:
        case SCREENBREAK16__STATIC:
        case SCREENBREAK17__STATIC:
        case SCREENBREAK18__STATIC:
        case SCREENBREAK19__STATIC:
            if ((krand()&255) < 16)
            {
                animwall[animwallNum].tag = wall[wallNum].picnum;
                wall[wallNum].picnum = SCREENBREAK6;
            }
            continue;

        case SCREENBREAK6__STATIC:
        case SCREENBREAK7__STATIC:
        case SCREENBREAK8__STATIC:
            if (animwall[animwallNum].tag >= 0 && wall[wallNum].extra != FEMPIC2 && wall[wallNum].extra != FEMPIC3)
                wall[wallNum].picnum = animwall[animwallNum].tag;
            else
            {
                wall[wallNum].picnum++;
                if (wall[wallNum].picnum == (SCREENBREAK6+3))
                    wall[wallNum].picnum = SCREENBREAK6;
            }
            continue;
        }

        if ((wall[wallNum].cstat&16) && G_GetForcefieldPicnum(wallNum)==W_FORCEFIELD)
        {
            int const wallTag = animwall[animwallNum].tag;

            if (wall[wallNum].cstat&254)
            {
                wall[wallNum].xpanning -= wallTag>>10; // sintable[(t+512)&2047]>>12;
                wall[wallNum].ypanning -= wallTag>>10; // sintable[t&2047]>>12;

                if (wall[wallNum].extra == 1)
                {
                    wall[wallNum].extra = 0;
                    animwall[animwallNum].tag     = 0;
                }
                else animwall[animwallNum].tag += 128;

                if (animwall[animwallNum].tag < (128 << 4))
                {
                    wall[wallNum].overpicnum = (animwall[animwallNum].tag & 128) ? W_FORCEFIELD : W_FORCEFIELD + 1;
                }
                else
                {
                    if ((krand()&255) < 32)
                        animwall[animwallNum].tag = 128<<(krand()&3);
                    else wall[wallNum].overpicnum = W_FORCEFIELD+1;
                }
            }
        }
    }
}

int G_ActivateWarpElevators(int spriteNum, int warpDir)
{
    bssize_t i;
    int const sectNum = sprite[spriteNum].sectnum;

    for (SPRITES_OF(STAT_EFFECTOR, i))
        if (SLT(i) == SE_17_WARP_ELEVATOR && SHT(i) == sprite[spriteNum].hitag)
        {
            if (klabs(sector[sectNum].floorz - actor[spriteNum].t_data[2]) > SP(i) ||
                    sector[SECT(i)].hitag == sector[sectNum].hitag - warpDir)
                break;
        }

    if (i == -1)
        return 1; // No find

#ifndef EDUKE32_STANDALONE
    if (!FURY)
        A_PlaySound(warpDir ? ELEVATOR_ON : ELEVATOR_OFF, spriteNum);
#endif

    for (SPRITES_OF(STAT_EFFECTOR, i))
        if (SLT(i) == SE_17_WARP_ELEVATOR && SHT(i) == sprite[spriteNum].hitag)
            T1(i) = T2(i) = warpDir; //Make all check warp

    return 0;
}

void G_OperateSectors(int sectNum, int spriteNum)
{
    int32_t j=0;
    int32_t i;
    sectortype *const pSector = &sector[sectNum];

    switch (pSector->lotag & (uint16_t)~49152u)
    {
    case ST_30_ROTATE_RISE_BRIDGE:
        j = sector[sectNum].hitag;

        if (E_SpriteIsValid(j))
        {
            if (actor[j].tempang == 0 || actor[j].tempang == 256)
                A_CallSound(sectNum,spriteNum);
            sprite[j].extra = (sprite[j].extra == 1) ? 3 : 1;
        }
        break;

    case ST_31_TWO_WAY_TRAIN:
        j = sector[sectNum].hitag;

        if (E_SpriteIsValid(j))
        {
            if (actor[j].t_data[4] == 0)
                actor[j].t_data[4] = 1;

            A_CallSound(sectNum,spriteNum);
        }
        break;

    case ST_26_SPLITTING_ST_DOOR: //The split doors
        if (GetAnimationGoal(&pSector->ceilingz) == -1) //if the door has stopped
        {
            g_haltSoundHack = 1;
            pSector->lotag &= 0xFF00u;
            pSector->lotag |= ST_22_SPLITTING_DOOR;
            G_OperateSectors(sectNum,spriteNum);
            pSector->lotag &= 0xFF00u;
            pSector->lotag |= ST_9_SLIDING_ST_DOOR;
            G_OperateSectors(sectNum,spriteNum);
            pSector->lotag &= 0xFF00u;
            pSector->lotag |= ST_26_SPLITTING_ST_DOOR;
        }
        return;

    case ST_9_SLIDING_ST_DOOR:
    {
        int const startWall = pSector->wallptr;
        int const endWall   = startWall + pSector->wallnum - 1;
        int const doorSpeed = pSector->extra >> 4;

        //first find center point by averaging all points

        vec2_t vect = { 0, 0 };

        for (i = startWall; i <= endWall; i++)
        {
            vect.x += wall[i].x;
            vect.y += wall[i].y;
        }

        vect.x = tabledivide32_noinline(vect.x, (endWall-startWall+1));
        vect.y = tabledivide32_noinline(vect.y, (endWall-startWall+1));

        //find any points with either same x or same y coordinate
        //  as center (dax, day) - should be 2 points found.

        int wallfind[2] = { -1, -1 };

        for (i = startWall; i <= endWall; i++)
        {
            if (wall[i].x == vect.x || wall[i].y == vect.y)
            {
                if (wallfind[0] == -1)
                    wallfind[0] = i;
                else
                    wallfind[1] = i;
            }
        }

        if (wallfind[1] == -1)
            return;

        for (j=0; j<2; j++)
        {
            int const foundWall = wallfind[j];

            i = foundWall - 1;

            if (i < startWall)
                i = endWall;

            vec2_t vect2 = { ((wall[i].x + wall[wall[foundWall].point2].x) >> 1) - wall[foundWall].x,
                             ((wall[i].y + wall[wall[foundWall].point2].y) >> 1) - wall[foundWall].y };

            if (wall[foundWall].x == vect.x && wall[foundWall].y == vect.y)
            {
                //find what direction door should open by averaging the
                //  2 neighboring points of wallfind[0] & wallfind[1].
                if (vect2.x != 0)
                {
                    vect2.x = wall[wall[wall[foundWall].point2].point2].x;
                    vect2.x -= wall[wall[foundWall].point2].x;
                    SetAnimation(sectNum, &wall[foundWall].x, wall[foundWall].x + vect2.x, doorSpeed);
                    SetAnimation(sectNum, &wall[i].x, wall[i].x + vect2.x, doorSpeed);
                    SetAnimation(sectNum, &wall[wall[foundWall].point2].x, wall[wall[foundWall].point2].x + vect2.x, doorSpeed);
                    A_CallSound(sectNum, spriteNum);
                }
                else if (vect2.y != 0)
                {
                    vect2.y = wall[wall[wall[foundWall].point2].point2].y;
                    vect2.y -= wall[wall[foundWall].point2].y;
                    SetAnimation(sectNum, &wall[foundWall].y, wall[foundWall].y + vect2.y, doorSpeed);
                    SetAnimation(sectNum, &wall[i].y, wall[i].y + vect2.y, doorSpeed);
                    SetAnimation(sectNum, &wall[wall[foundWall].point2].y, wall[wall[foundWall].point2].y + vect2.y, doorSpeed);
                    A_CallSound(sectNum, spriteNum);
                }
            }
            else
            {
                if (vect2.x != 0)
                {
                    SetAnimation(sectNum, &wall[foundWall].x, vect.x, doorSpeed);
                    SetAnimation(sectNum, &wall[i].x, vect.x + vect2.x, doorSpeed);
                    SetAnimation(sectNum, &wall[wall[foundWall].point2].x, vect.x + vect2.x, doorSpeed);
                    A_CallSound(sectNum, spriteNum);
                }
                else if (vect2.y != 0)
                {
                    SetAnimation(sectNum, &wall[foundWall].y, vect.y, doorSpeed);
                    SetAnimation(sectNum, &wall[i].y, vect.y + vect2.y, doorSpeed);
                    SetAnimation(sectNum, &wall[wall[foundWall].point2].y, vect.y + vect2.y, doorSpeed);
                    A_CallSound(sectNum, spriteNum);
                }
            }
        }
    }
    return;

    case ST_15_WARP_ELEVATOR://Warping elevators

        if (sprite[spriteNum].picnum != APLAYER)
            return;

        for (SPRITES_OF_SECT(sectNum, i))
            if (PN(i)==SECTOREFFECTOR && SLT(i) == SE_17_WARP_ELEVATOR)
                break;

        if (i < 0)
            return;

        if (sprite[spriteNum].sectnum == sectNum)
        {
            if (G_ActivateWarpElevators(i,-1))
                G_ActivateWarpElevators(i,1);
            else if (G_ActivateWarpElevators(i,1))
                G_ActivateWarpElevators(i,-1);
        }
        else
        {
            if (pSector->floorz > SZ(i))
                G_ActivateWarpElevators(i,-1);
            else
                G_ActivateWarpElevators(i,1);
        }

        return;

    case ST_16_PLATFORM_DOWN:
    case ST_17_PLATFORM_UP:

        i = GetAnimationGoal(&pSector->floorz);

        if (i == -1)
        {
            i = nextsectorneighborz(sectNum,pSector->floorz,1,1);
            if (i == -1)
            {
                i = nextsectorneighborz(sectNum,pSector->floorz,1,-1);
                if (i == -1) return;
                j = sector[i].floorz;
                SetAnimation(sectNum,&pSector->floorz,j,pSector->extra);
            }
            else
            {
                j = sector[i].floorz;
                SetAnimation(sectNum,&pSector->floorz,j,pSector->extra);
            }
            A_CallSound(sectNum,spriteNum);
        }

        return;

    case ST_18_ELEVATOR_DOWN:
    case ST_19_ELEVATOR_UP:

        i = GetAnimationGoal(&pSector->floorz);

        if (i==-1)
        {
            i = nextsectorneighborz(sectNum, pSector->floorz, 1, -1);

            if (i == -1)
                i = nextsectorneighborz(sectNum, pSector->floorz, 1, 1);

            if (i == -1)
                return;

            j = sector[i].floorz;

            int const sectorExtra = pSector->extra;
            int const zDiff       = pSector->ceilingz - pSector->floorz;

            SetAnimation(sectNum, &pSector->floorz, j, sectorExtra);
            SetAnimation(sectNum, &pSector->ceilingz, j + zDiff, sectorExtra);
            A_CallSound(sectNum, spriteNum);
        }
        return;

    case ST_29_TEETH_DOOR:

        for (SPRITES_OF(STAT_EFFECTOR, i))
            if (SLT(i) == SE_22_TEETH_DOOR && SHT(i) == pSector->hitag)
            {
                sector[SECT(i)].extra = -sector[SECT(i)].extra;

                T1(i) = sectNum;
                T2(i) = 1;
            }

        A_CallSound(sectNum, spriteNum);

        pSector->lotag ^= 0x8000u;

        if (pSector->lotag & 0x8000u)
        {
            j = nextsectorneighborz(sectNum,pSector->ceilingz,-1,-1);
            if (j == -1) j = nextsectorneighborz(sectNum,pSector->ceilingz,1,1);
            if (j == -1)
            {
                OSD_Printf("WARNING: ST29: null sector!\n");
                return;
            }
            j = sector[j].ceilingz;
        }
        else
        {
            j = nextsectorneighborz(sectNum,pSector->ceilingz,1,1);
            if (j == -1) j = nextsectorneighborz(sectNum,pSector->ceilingz,-1,-1);
            if (j == -1)
            {
                OSD_Printf("WARNING: ST29: null sector!\n");
                return;
            }
            j = sector[j].floorz;
        }

        SetAnimation(sectNum,&pSector->ceilingz,j,pSector->extra);

        return;

    case ST_20_CEILING_DOOR:
REDODOOR:

        if (pSector->lotag & 0x8000u)
        {
            for (SPRITES_OF_SECT(sectNum, i))
                if (sprite[i].statnum == STAT_EFFECTOR && SLT(i)==SE_9_DOWN_OPEN_DOOR_LIGHTS)
                {
                    j = SZ(i);
                    break;
                }

            if (i==-1)
                j = pSector->floorz;
        }
        else
        {
            j = nextsectorneighborz(sectNum,pSector->ceilingz,-1,-1);

            if (j >= 0) j = sector[j].ceilingz;
            else
            {
                pSector->lotag |= 32768u;
                goto REDODOOR;
            }
        }

        pSector->lotag ^= 0x8000u;

        SetAnimation(sectNum,&pSector->ceilingz,j,pSector->extra);
        A_CallSound(sectNum,spriteNum);

        return;

    case ST_21_FLOOR_DOOR:
        i = GetAnimationGoal(&pSector->floorz);
        if (i >= 0)
        {
            if (g_animateGoal[sectNum] == pSector->ceilingz)
                g_animateGoal[i] = sector[nextsectorneighborz(sectNum,pSector->ceilingz,1,1)].floorz;
            else g_animateGoal[i] = pSector->ceilingz;
        }
        else
        {
            if (pSector->ceilingz == pSector->floorz)
                j = sector[nextsectorneighborz(sectNum,pSector->ceilingz,1,1)].floorz;
            else j = pSector->ceilingz;

            pSector->lotag ^= 0x8000u;

            if (SetAnimation(sectNum,&pSector->floorz,j,pSector->extra) >= 0)
                A_CallSound(sectNum,spriteNum);
        }
        return;

    case ST_22_SPLITTING_DOOR:

        if (pSector->lotag & 0x8000u)
        {
            int const q = (pSector->ceilingz + pSector->floorz) >> 1;
            SetAnimation(sectNum, &pSector->floorz, q, pSector->extra);
            SetAnimation(sectNum, &pSector->ceilingz, q, pSector->extra);
        }
        else
        {
            int const floorNeighbor   = nextsectorneighborz(sectNum, pSector->floorz, 1, 1);
            int const ceilingNeighbor = nextsectorneighborz(sectNum, pSector->ceilingz, -1, -1);

            if (floorNeighbor>=0 && ceilingNeighbor>=0)
            {
                SetAnimation(sectNum, &pSector->floorz, sector[floorNeighbor].floorz, pSector->extra);
                SetAnimation(sectNum, &pSector->ceilingz, sector[ceilingNeighbor].ceilingz, pSector->extra);
            }
            else
            {
                OSD_Printf("WARNING: ST_22_SPLITTING_DOOR: null sector: floor neighbor=%d, ceiling neighbor=%d!\n",
                           floorNeighbor, ceilingNeighbor);
                pSector->lotag ^= 0x8000u;
            }
        }

        pSector->lotag ^= 0x8000u;

        A_CallSound(sectNum,spriteNum);

        return;

    case ST_23_SWINGING_DOOR: //Swingdoor
        {
            j = -1;

            for (SPRITES_OF(STAT_EFFECTOR, i))
                if (SLT(i) == SE_11_SWINGING_DOOR && SECT(i) == sectNum && !T5(i))
                {
                    j = i;
                    break;
                }

            if (i < 0)
                return;

            uint16_t const tag = sector[SECT(i)].lotag & 0x8000u;

            if (j >= 0)
            {
                int soundPlayed = 0;

                for (SPRITES_OF(STAT_EFFECTOR, i))
                {
                    if (tag == (sector[SECT(i)].lotag & 0x8000u) && SLT(i) == SE_11_SWINGING_DOOR && sprite[j].hitag == SHT(i) && !T5(i))
                    {
                        if (sector[SECT(i)].lotag & 0x8000u) sector[SECT(i)].lotag &= 0x7fff;
                        else sector[SECT(i)].lotag |= 0x8000u;

                        T5(i) = 1;
                        T4(i) = -T4(i);

                        if (!soundPlayed)
                        {
                            A_CallSound(sectNum, i);
                            soundPlayed = 1;
                        }
                    }
                }
            }
        }
        return;

    case ST_25_SLIDING_DOOR: //Subway type sliding doors

        for (SPRITES_OF(STAT_EFFECTOR, j))
            if (sprite[j].lotag == SE_15_SLIDING_DOOR && sprite[j].sectnum == sectNum)
                break; //Found the sectoreffector.

        if (j < 0)
            return;

        for (SPRITES_OF(STAT_EFFECTOR, i))
            if (SHT(i)==sprite[j].hitag)
            {
                if (SLT(i) == SE_15_SLIDING_DOOR)
                {
                    sector[SECT(i)].lotag ^= 0x8000u; // Toggle the open or close
                    SA(i) += 1024;

                    if (T5(i))
                        A_CallSound(SECT(i),i);
                    A_CallSound(SECT(i),i);

                    T5(i) = (sector[SECT(i)].lotag & 0x8000u) ? 1 : 2;
                }
            }

        return;

    case ST_27_STRETCH_BRIDGE:  //Extended bridge

        for (SPRITES_OF(STAT_EFFECTOR, j))
            if ((sprite[j].lotag&0xff)==SE_20_STRETCH_BRIDGE && sprite[j].sectnum == sectNum)  //Bridge
            {
                sector[sectNum].lotag ^= 0x8000u;
                // Highest bit now set means we're opening.

                actor[j].t_data[0] = (sector[sectNum].lotag & 0x8000u) ? 1 : 2;
                A_CallSound(sectNum,spriteNum);
                break;
            }

        return;

    case ST_28_DROP_FLOOR:
        //activate the rest of them

        for (SPRITES_OF_SECT(sectNum, j))
            if (sprite[j].statnum==STAT_EFFECTOR && (sprite[j].lotag&0xff)==SE_21_DROP_FLOOR)
                break;

        if (j >= 0)  // PK: The matching SE21 might have gone, see SE_21_KILLIT in actors.c
        {
            j = sprite[j].hitag;

            for (bssize_t SPRITES_OF(STAT_EFFECTOR, l))
            {
                if ((sprite[l].lotag&0xff)==SE_21_DROP_FLOOR && !actor[l].t_data[0] &&
                        sprite[l].hitag == j)
                    actor[l].t_data[0] = 1;
            }

            A_CallSound(sectNum,spriteNum);
        }

        return;
    }
}

void G_OperateRespawns(int lotag)
{
    for (bssize_t nextSprite, SPRITES_OF_STAT_SAFE(STAT_FX, spriteNum, nextSprite))
    {
        auto const pSprite = &sprite[spriteNum];

        if (pSprite->lotag == lotag && pSprite->picnum == RESPAWN)
        {
            if (!ud.monsters_off || !A_CheckEnemyTile(pSprite->hitag))
            {
#ifndef EDUKE32_STANDALONE
                if (!FURY)
                {
                    int const j = A_Spawn(spriteNum, TRANSPORTERSTAR);
                    sprite[j].z -= ZOFFSET5;
                }
#endif
                // Just a way to killit (see G_MoveFX(): RESPAWN__STATIC)
                pSprite->extra = 66-12;
            }
        }
    }
}

void G_OperateActivators(int lotag, int playerNum)
{
    for (bssize_t spriteNum=g_cyclerCnt-1; spriteNum>=0; spriteNum--)
    {
        int16_t *const pCycler = &g_cyclers[spriteNum][0];

        if (pCycler[4] == lotag)
        {
            pCycler[5]                      = !pCycler[5];
            sector[pCycler[0]].floorshade   = pCycler[3];
            sector[pCycler[0]].ceilingshade = pCycler[3];
            walltype *pWall                 = &wall[sector[pCycler[0]].wallptr];

            for (int j = sector[pCycler[0]].wallnum; j > 0; j--, pWall++)
                pWall->shade = pCycler[3];
        }
    }

    int soundPlayed = -1;

    for (bssize_t nextSprite, SPRITES_OF_STAT_SAFE(STAT_ACTIVATOR, spriteNum, nextSprite))
    {
        if (sprite[spriteNum].lotag == lotag)
        {
            if (sprite[spriteNum].picnum == ACTIVATORLOCKED)
            {
                sector[SECT(spriteNum)].lotag ^= 16384;

                if (playerNum >= 0 && playerNum < ud.multimode)
                    P_DoQuote((sector[SECT(spriteNum)].lotag & 16384) ? QUOTE_LOCKED : QUOTE_UNLOCKED, g_player[playerNum].ps);
            }
            else
            {
                switch (SHT(spriteNum))
                {
                    case 1:
                        if (sector[SECT(spriteNum)].floorz != sector[SECT(spriteNum)].ceilingz)
                            continue;
                        break;
                    case 2:
                        if (sector[SECT(spriteNum)].floorz == sector[SECT(spriteNum)].ceilingz)
                            continue;
                        break;
                }

                // ST_2_UNDERWATER
                if (sector[sprite[spriteNum].sectnum].lotag < 3)
                {
                    for (bssize_t SPRITES_OF_SECT(sprite[spriteNum].sectnum, foundSprite))
                    {
                        if (sprite[foundSprite].statnum == STAT_EFFECTOR)
                        {
                            switch (sprite[foundSprite].lotag)
                            {
                                case SE_36_PROJ_SHOOTER:
                                case SE_31_FLOOR_RISE_FALL:
                                case SE_32_CEILING_RISE_FALL:
                                case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
                                    actor[foundSprite].t_data[0] = 1 - actor[foundSprite].t_data[0];
                                    A_CallSound(SECT(spriteNum), foundSprite);
                                    break;
                            }
                        }
                    }
                }

                if (soundPlayed == -1 && (sector[SECT(spriteNum)].lotag&0xff) == ST_22_SPLITTING_DOOR)
                    soundPlayed = A_CallSound(SECT(spriteNum),spriteNum);

                G_OperateSectors(SECT(spriteNum),spriteNum);
            }
        }
    }

    G_OperateRespawns(lotag);
}

void G_OperateMasterSwitches(int lotag)
{
    for (bssize_t SPRITES_OF(STAT_STANDABLE, i))
        if (PN(i) == MASTERSWITCH && SLT(i) == lotag && SP(i) == 0)
            SP(i) = 1;
}

void G_OperateForceFields(int spriteNum, int wallTag)
{
    for (bssize_t animwallNum = 0; animwallNum < g_animWallCnt; ++animwallNum)
    {
        int const wallNum = animwall[animwallNum].wallnum;

        if ((wallTag == wall[wallNum].lotag || wallTag == -1)
            && (G_GetForcefieldPicnum(wallNum) == W_FORCEFIELD || (wall[wallNum].overpicnum == BIGFORCE)))
        {
            animwall[animwallNum].tag = 0;

            if (wall[wallNum].cstat)
            {
                wall[wallNum].cstat = 0;

                if (spriteNum >= 0 && sprite[spriteNum].picnum == SECTOREFFECTOR && sprite[spriteNum].lotag == SE_30_TWO_WAY_TRAIN)
                    wall[wallNum].lotag = 0;
            }
            else
                wall[wallNum].cstat = FORCEFIELD_CSTAT;
        }
    }
}

// List of switches that function like dip (combination lock) switches.
#define DIPSWITCH_LIKE_CASES                                                                                                \
    DIPSWITCH__STATIC:                                                                                                      \
    case TECHSWITCH__STATIC:                                                                                                \
    case ALIENSWITCH__STATIC

// List of access switches.
#define ACCESSSWITCH_CASES                                                                                                  \
    ACCESSSWITCH__STATIC:                                                                                                   \
    case ACCESSSWITCH2__STATIC

// List of switches that don't fit the two preceding categories, and are not
// the MULTISWITCH. 13 cases.
#define REST_SWITCH_CASES                                                                                                   \
    DIPSWITCH2__STATIC:                                                                                                     \
    case DIPSWITCH3__STATIC:                                                                                                \
    case FRANKENSTINESWITCH__STATIC:                                                                                        \
    case HANDSWITCH__STATIC:                                                                                                \
    case LIGHTSWITCH2__STATIC:                                                                                              \
    case LIGHTSWITCH__STATIC:                                                                                               \
    case LOCKSWITCH1__STATIC:                                                                                               \
    case POWERSWITCH1__STATIC:                                                                                              \
    case POWERSWITCH2__STATIC:                                                                                              \
    case PULLSWITCH__STATIC:                                                                                                \
    case SLOTDOOR__STATIC:                                                                                                  \
    case SPACEDOORSWITCH__STATIC:                                                                                           \
    case SPACELIGHTSWITCH__STATIC

// Returns:
//  0: is not a dipswitch-like switch
//  1: is one, off
//  2: is one, on
static int G_IsLikeDipswitch(int switchPic)
{
    for (bssize_t i=0; i<2; i++)
        if (switchPic == DIPSWITCH+i || switchPic == TECHSWITCH+i || switchPic == ALIENSWITCH+i)
            return 1+i;

    return 0;
}

// Get base (unpressed) tile number for switch.
static int G_GetBaseSwitch(int switchPic)
{
    if (switchPic > MULTISWITCH && switchPic <= MULTISWITCH+3)
        return MULTISWITCH;

    return (switchPic == DIPSWITCH + 1      || switchPic == DIPSWITCH2 + 1   || switchPic == DIPSWITCH3 + 1 ||
        switchPic == TECHSWITCH + 1         || switchPic == ALIENSWITCH + 1  || switchPic == PULLSWITCH + 1 ||
        switchPic == HANDSWITCH + 1         || switchPic == SLOTDOOR + 1     || switchPic == SPACEDOORSWITCH + 1 ||
        switchPic == SPACELIGHTSWITCH + 1   || switchPic == LIGHTSWITCH + 1  || switchPic == LIGHTSWITCH2 + 1 ||
        switchPic == FRANKENSTINESWITCH + 1 || switchPic == POWERSWITCH1 + 1 || switchPic == POWERSWITCH2 + 1 ||
        switchPic == LOCKSWITCH1 + 1) ?
        switchPic-1 : switchPic;
}

enum { SWITCH_WALL, SWITCH_SPRITE };

int P_ActivateSwitch(int playerNum, int wallOrSprite, int switchType)
{
    if (wallOrSprite < 0)
        return 0;

    vec3_t davector;
    int16_t lotag, hitag;
    int16_t nSwitchPicnum;
    uint8_t nSwitchPal;

    if (switchType == SWITCH_SPRITE) // A wall sprite
    {
        if (actor[wallOrSprite].lasttransport == ((int32_t) totalclock & UINT8_MAX))
            return 0;

        actor[wallOrSprite].lasttransport = ((int32_t) totalclock & UINT8_MAX);

        if (sprite[wallOrSprite].lotag == 0)
            return 0;

        lotag         = sprite[wallOrSprite].lotag;
        hitag         = sprite[wallOrSprite].hitag;
        davector      = sprite[wallOrSprite].pos;
        nSwitchPicnum = sprite[wallOrSprite].picnum;
        nSwitchPal    = sprite[wallOrSprite].pal;
    }
    else
    {
        if (wall[wallOrSprite].lotag == 0)
            return 0;

        lotag         = wall[wallOrSprite].lotag;
        hitag         = wall[wallOrSprite].hitag;
        davector      = { wall[wallOrSprite].x, wall[wallOrSprite].y, g_player[playerNum].ps->pos.z };
        nSwitchPicnum = wall[wallOrSprite].picnum;
        nSwitchPal    = wall[wallOrSprite].pal;
    }

    //    initprintf("P_ActivateSwitch called picnum=%i switchissprite=%i\n",picnum,switchissprite);

    int basePicnum   = G_GetBaseSwitch(nSwitchPicnum);
    int correctDips  = 1;
    int numDips      = 0;

    switch (DYNAMICTILEMAP(basePicnum))
    {
    case DIPSWITCH_LIKE_CASES:
        break;

    case ACCESSSWITCH_CASES:
        if (g_player[playerNum].ps->access_incs == 0)
        {
            static const int32_t key_switchpal[3]  = { 0, 21, 23 };
            static const int32_t need_key_quote[3] = { QUOTE_NEED_BLUE_KEY, QUOTE_NEED_RED_KEY, QUOTE_NEED_YELLOW_KEY };

            for (bssize_t nKeyPal = 0; nKeyPal < 3; nKeyPal++)
            {
                if (nSwitchPal == key_switchpal[nKeyPal])
                {
                    if (g_player[playerNum].ps->got_access & (1 << nKeyPal))
                        g_player[playerNum].ps->access_incs = 1;
                    else
                        P_DoQuote(need_key_quote[nKeyPal], g_player[playerNum].ps);

                    break;
                }
            }

            if (g_player[playerNum].ps->access_incs == 1)
            {
                if (switchType == SWITCH_WALL)
                    g_player[playerNum].ps->access_wallnum = wallOrSprite;
                else
                    g_player[playerNum].ps->access_spritenum = wallOrSprite;
            }

            return 0;
        }
        fallthrough__;
    case MULTISWITCH__STATIC:
    case REST_SWITCH_CASES:
        if (G_CheckActivatorMotion(lotag))
            return 0;
        break;

    default:
        if (CheckDoorTile(nSwitchPicnum) == 0)
            return 0;
        break;
    }

    for (bssize_t SPRITES_OF(STAT_DEFAULT, spriteNum))
    {
        if (lotag == SLT(spriteNum))
        {
            // Put the tile number into a variable so later switches don't
            // trigger on the result of changes:
            int const spritePic = PN(spriteNum);

            if (spritePic >= MULTISWITCH && spritePic <= MULTISWITCH+3)
            {
                sprite[spriteNum].picnum++;
                if (sprite[spriteNum].picnum > MULTISWITCH+3)
                    sprite[spriteNum].picnum = MULTISWITCH;
            }

            switch (DYNAMICTILEMAP(spritePic))
            {
            case DIPSWITCH_LIKE_CASES:
                if (switchType == SWITCH_SPRITE && wallOrSprite == spriteNum)
                    PN(spriteNum)++;
                else if (SHT(spriteNum) == 0)
                    correctDips++;
                numDips++;
                break;

            case ACCESSSWITCH_CASES:
            case REST_SWITCH_CASES:
                sprite[spriteNum].picnum++;
                break;

            default:
                if (spritePic <= 0)  // oob safety
                    break;

                switch (DYNAMICTILEMAP(spritePic - 1))
                {
                    case DIPSWITCH_LIKE_CASES:
                        if (switchType == SWITCH_SPRITE && wallOrSprite == spriteNum)
                            PN(spriteNum)--;
                        else if (SHT(spriteNum) == 1)
                            correctDips++;
                        numDips++;
                        break;

                    case REST_SWITCH_CASES:
                        sprite[spriteNum].picnum--;
                        break;
                }
                break;
            }
        }
    }

    for (bssize_t wallNum=numwalls-1; wallNum>=0; wallNum--)
    {
        if (lotag == wall[wallNum].lotag)
        {
            if (wall[wallNum].picnum >= MULTISWITCH && wall[wallNum].picnum <= MULTISWITCH+3)
            {
                wall[wallNum].picnum++;
                if (wall[wallNum].picnum > MULTISWITCH+3)
                    wall[wallNum].picnum = MULTISWITCH;
            }

            switch (DYNAMICTILEMAP(wall[wallNum].picnum))
            {
                case DIPSWITCH_LIKE_CASES:
                    if (switchType == SWITCH_WALL && wallNum == wallOrSprite)
                        wall[wallNum].picnum++;
                    else if (wall[wallNum].hitag == 0)
                        correctDips++;
                    numDips++;
                    break;

                case ACCESSSWITCH_CASES:
                case REST_SWITCH_CASES:
                    wall[wallNum].picnum++;
                    break;

                default:
                    if (wall[wallNum].picnum <= 0)  // oob safety
                        break;

                    switch (DYNAMICTILEMAP(wall[wallNum].picnum - 1))
                    {
                        case DIPSWITCH_LIKE_CASES:
                            if (switchType == SWITCH_WALL && wallNum == wallOrSprite)
                                wall[wallNum].picnum--;
                            else if (wall[wallNum].hitag == 1)
                                correctDips++;
                            numDips++;
                            break;

                        case REST_SWITCH_CASES:
                            wall[wallNum].picnum--;
                            break;
                    }
                    break;
            }
        }
    }

    if ((uint16_t)lotag == UINT16_MAX)
    {
        P_EndLevel();
        return 1;
    }

    basePicnum = G_GetBaseSwitch(nSwitchPicnum);

    switch (DYNAMICTILEMAP(basePicnum))
    {
        default:
            if (CheckDoorTile(nSwitchPicnum) == 0)
                break;
            fallthrough__;
        case DIPSWITCH_LIKE_CASES:
            if (G_IsLikeDipswitch(nSwitchPicnum))
            {
                S_PlaySound3D((nSwitchPicnum == ALIENSWITCH || nSwitchPicnum == ALIENSWITCH + 1) ? ALIEN_SWITCH1 : SWITCH_ON,
                              (switchType == SWITCH_SPRITE) ? wallOrSprite : g_player[playerNum].ps->i, &davector);

                if (numDips != correctDips)
                    break;

                S_PlaySound3D(END_OF_LEVEL_WARN, g_player[playerNum].ps->i, &davector);
            }
            fallthrough__;
        case ACCESSSWITCH_CASES:
        case MULTISWITCH__STATIC:
        case REST_SWITCH_CASES:
        {
            if (nSwitchPicnum >= MULTISWITCH && nSwitchPicnum <= MULTISWITCH + 3)
                lotag += nSwitchPicnum - MULTISWITCH;

            for (bssize_t SPRITES_OF(STAT_EFFECTOR, spriteNum))
            {
                if (sprite[spriteNum].hitag == lotag)
                {
                    switch (sprite[spriteNum].lotag)
                    {
                        case SE_12_LIGHT_SWITCH:
                            sector[sprite[spriteNum].sectnum].floorpal = 0;
                            actor[spriteNum].t_data[0]++;
                            if (actor[spriteNum].t_data[0] == 2)
                                actor[spriteNum].t_data[0]++;
                            break;

                        case SE_24_CONVEYOR:
                        case SE_34:
                        case SE_25_PISTON:
                            actor[spriteNum].t_data[4] = !actor[spriteNum].t_data[4];
                            P_DoQuote(actor[spriteNum].t_data[4] ? QUOTE_DEACTIVATED : QUOTE_ACTIVATED, g_player[playerNum].ps);
                            break;

                        case SE_21_DROP_FLOOR:
                            P_DoQuote(QUOTE_ACTIVATED, g_player[screenpeek].ps);
                            break;
                    }
                }
            }

            G_OperateActivators(lotag, playerNum);
            G_OperateForceFields(g_player[playerNum].ps->i, lotag);
            G_OperateMasterSwitches(lotag);

            if (G_IsLikeDipswitch(nSwitchPicnum))
                return 1;

            if (!hitag && CheckDoorTile(nSwitchPicnum) == 0)
                S_PlaySound3D(SWITCH_ON, (switchType == SWITCH_SPRITE) ? wallOrSprite : g_player[playerNum].ps->i, &davector);
            else if (hitag)
            {
                if (switchType == SWITCH_SPRITE && (g_sounds[hitag].m & SF_TALK) == 0)
                    S_PlaySound3D(hitag, wallOrSprite, &davector);
                else
                    A_PlaySound(hitag, g_player[playerNum].ps->i);
            }

            return 1;
        }
    }

    return 0;
}

void G_ActivateBySector(int sectNum, int spriteNum)
{
    int activatedSectors = 0;

    for (bssize_t SPRITES_OF_SECT(sectNum, i))
        if (PN(i) == ACTIVATOR)
        {
            G_OperateActivators(SLT(i),-1);
            ++activatedSectors;
        }

    if (!activatedSectors)
        G_OperateSectors(sectNum, spriteNum);
}

static void G_BreakWall(int tileNum, int spriteNum, int wallNum)
{
    wall[wallNum].picnum = tileNum;
#ifndef EDUKE32_STANDALONE
    A_PlaySound(VENT_BUST,spriteNum);
    A_PlaySound(GLASS_HEAVYBREAK,spriteNum);
    A_SpawnWallGlass(spriteNum,wallNum,10);
#else
    UNREFERENCED_PARAMETER(spriteNum);
#endif
}

void A_DamageWall_Internal(int spriteNum, int wallNum, const vec3_t &vPos, int weaponNum)
{
    int16_t sectNum = -1;
    walltype *pWall = &wall[wallNum];

    if ((g_tile[pWall->overpicnum].flags & SFLAG_DAMAGEEVENT) || (g_tile[pWall->picnum].flags & SFLAG_DAMAGEEVENT))
    {
        if (VM_OnEventWithReturn(EVENT_DAMAGEWALL, spriteNum, -1, wallNum) < 0)
            return;
    }

    if (pWall->overpicnum == MIRROR && pWall->pal != 4 &&
        A_CheckSpriteFlags(spriteNum, SFLAG_PROJECTILE) &&
        (SpriteProjectile[spriteNum].workslike & PROJECTILE_RPG))
    {
        if (pWall->nextwall == -1 || wall[pWall->nextwall].pal != 4)
        {
#ifndef EDUKE32_STANDALONE
            A_SpawnWallGlass(spriteNum, wallNum, 70);
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
#endif
            pWall->cstat &= ~16;
            pWall->overpicnum = MIRRORBROKE;
            return;
        }
    }

    if (pWall->overpicnum == MIRROR && pWall->pal != 4)
    {
        switch (DYNAMICTILEMAP(weaponNum))
        {
            case RADIUSEXPLOSION__STATIC:
            case SEENINE__STATIC:
#ifndef EDUKE32_STANDALONE
            case HEAVYHBOMB__STATIC:
            case RPG__STATIC:
            case HYDRENT__STATIC:
            case OOZFILTER__STATIC:
            case EXPLODINGBARREL__STATIC:
#endif
                if (pWall->nextwall == -1 || wall[pWall->nextwall].pal != 4)
                {
#ifndef EDUKE32_STANDALONE
                    A_SpawnWallGlass(spriteNum, wallNum, 70);
                    A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
#endif
                    pWall->cstat &= ~16;
                    pWall->overpicnum = MIRRORBROKE;
                    return;
                }
        }
    }

    if ((((pWall->cstat & 16) || pWall->overpicnum == BIGFORCE) && pWall->nextsector >= 0) &&
        (sector[pWall->nextsector].floorz > vPos.z) &&
        (sector[pWall->nextsector].floorz != sector[pWall->nextsector].ceilingz))
    {
        int const switchPic = G_GetForcefieldPicnum(wallNum);

        switch (DYNAMICTILEMAP(switchPic))
        {
            case FANSPRITE__STATIC:
                pWall->overpicnum = FANSPRITEBROKE;
                pWall->cstat &= 65535 - 65;
                if (pWall->nextwall >= 0)
                {
                    wall[pWall->nextwall].overpicnum = FANSPRITEBROKE;
                    wall[pWall->nextwall].cstat &= 65535 - 65;
                }
                A_PlaySound(VENT_BUST, spriteNum);
                A_PlaySound(GLASS_BREAKING, spriteNum);
                return;

#ifndef EDUKE32_STANDALONE
            case W_FORCEFIELD__STATIC:
                pWall->extra = 1;  // tell the forces to animate
                fallthrough__;
            case BIGFORCE__STATIC:
            {
                updatesector(vPos.x, vPos.y, &sectNum);
                if (sectNum < 0)
                    return;

                int xRepeat = 32;
                int yRepeat = 32;

                if (weaponNum == -1)
                    xRepeat = yRepeat = 8;
                else if (weaponNum == CHAINGUN)
                {
                    xRepeat = 16 + sprite[spriteNum].xrepeat;
                    yRepeat = 16 + sprite[spriteNum].yrepeat;
                }

                int const i = A_InsertSprite(sectNum, vPos.x, vPos.y, vPos.z, FORCERIPPLE, -127, xRepeat, yRepeat, 0,
                                   0, 0, spriteNum, 5);

                CS(i) |= 18 + 128;
                SA(i) = getangle(pWall->x - wall[pWall->point2].x, pWall->y - wall[pWall->point2].y) - 512;

                A_PlaySound(SOMETHINGHITFORCE, i);
            }
                return;

            case GLASS__STATIC:
                updatesector(vPos.x, vPos.y, &sectNum);
                if (sectNum < 0)
                    return;
                pWall->overpicnum = GLASS2;
                A_SpawnWallGlass(spriteNum, wallNum, 10);
                pWall->cstat = 0;

                if (pWall->nextwall >= 0)
                    wall[pWall->nextwall].cstat = 0;

                {
                    int const i = A_InsertSprite(sectNum, vPos.x, vPos.y, vPos.z, SECTOREFFECTOR, 0, 0, 0,
                        fix16_to_int(g_player[0].ps->q16ang), 0, 0, spriteNum, 3);
                    SLT(i) = 128;
                    T2(i)  = 5;
                    T3(i)  = wallNum;
                    A_PlaySound(GLASS_BREAKING, i);
                }
                return;

            case STAINGLASS1__STATIC:
                updatesector(vPos.x, vPos.y, &sectNum);
                if (sectNum < 0)
                    return;
                A_SpawnRandomGlass(spriteNum, wallNum, 80);
                pWall->cstat = 0;
                if (pWall->nextwall >= 0)
                    wall[pWall->nextwall].cstat = 0;
                A_PlaySound(VENT_BUST, spriteNum);
                A_PlaySound(GLASS_BREAKING, spriteNum);
                return;
#endif
        }
    }

    switch (DYNAMICTILEMAP(pWall->picnum))
    {
        case COLAMACHINE__STATIC:
        case VENDMACHINE__STATIC:
            G_BreakWall(pWall->picnum + 2, spriteNum, wallNum);
            A_PlaySound(VENT_BUST, spriteNum);
            return;

        case OJ__STATIC:
        case FEMPIC2__STATIC:
        case FEMPIC3__STATIC:

        case SCREENBREAK6__STATIC:
        case SCREENBREAK7__STATIC:
        case SCREENBREAK8__STATIC:

        case SCREENBREAK1__STATIC:
        case SCREENBREAK2__STATIC:
        case SCREENBREAK3__STATIC:
        case SCREENBREAK4__STATIC:
        case SCREENBREAK5__STATIC:

        case SCREENBREAK9__STATIC:
        case SCREENBREAK10__STATIC:
        case SCREENBREAK11__STATIC:
        case SCREENBREAK12__STATIC:
        case SCREENBREAK13__STATIC:
        case SCREENBREAK14__STATIC:
        case SCREENBREAK15__STATIC:
        case SCREENBREAK16__STATIC:
        case SCREENBREAK17__STATIC:
        case SCREENBREAK18__STATIC:
        case SCREENBREAK19__STATIC:
        case BORNTOBEWILDSCREEN__STATIC:
#ifndef EDUKE32_STANDALONE
            A_SpawnWallGlass(spriteNum, wallNum, 30);
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
#endif
            pWall->picnum = W_SCREENBREAK + (krand() % 3);
            return;

        case W_TECHWALL5__STATIC:
        case W_TECHWALL6__STATIC:
        case W_TECHWALL7__STATIC:
        case W_TECHWALL8__STATIC:
        case W_TECHWALL9__STATIC:
            G_BreakWall(pWall->picnum + 1, spriteNum, wallNum);
            return;

        case W_MILKSHELF__STATIC:
            G_BreakWall(W_MILKSHELFBROKE, spriteNum, wallNum);
            return;

        case W_TECHWALL10__STATIC:
            G_BreakWall(W_HITTECHWALL10, spriteNum, wallNum);
            return;

        case W_TECHWALL1__STATIC:
        case W_TECHWALL11__STATIC:
        case W_TECHWALL12__STATIC:
        case W_TECHWALL13__STATIC:
        case W_TECHWALL14__STATIC:
            G_BreakWall(W_HITTECHWALL1, spriteNum, wallNum);
            return;

        case W_TECHWALL15__STATIC:
            G_BreakWall(W_HITTECHWALL15, spriteNum, wallNum);
            return;

        case W_TECHWALL16__STATIC:
            G_BreakWall(W_HITTECHWALL16, spriteNum, wallNum);
            return;

        case W_TECHWALL2__STATIC:
            G_BreakWall(W_HITTECHWALL2, spriteNum, wallNum);
            return;

        case W_TECHWALL3__STATIC:
            G_BreakWall(W_HITTECHWALL3, spriteNum, wallNum);
            return;

        case W_TECHWALL4__STATIC:
            G_BreakWall(W_HITTECHWALL4, spriteNum, wallNum);
            return;

        case ATM__STATIC:
            pWall->picnum = ATMBROKE;
            A_SpawnMultiple(spriteNum, MONEY, 1 + (krand() & 7));
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            break;

        case WALLLIGHT1__STATIC:
        case WALLLIGHT2__STATIC:
        case WALLLIGHT3__STATIC:
        case WALLLIGHT4__STATIC:
        case TECHLIGHT2__STATIC:
        case TECHLIGHT4__STATIC:
        {
#ifndef EDUKE32_STANDALONE
            A_PlaySound(rnd(128) ? GLASS_HEAVYBREAK : GLASS_BREAKING, spriteNum);
            A_SpawnWallGlass(spriteNum, wallNum, 30);
#endif

            if (pWall->picnum == WALLLIGHT1)
                pWall->picnum = WALLLIGHTBUST1;

            if (pWall->picnum == WALLLIGHT2)
                pWall->picnum = WALLLIGHTBUST2;

            if (pWall->picnum == WALLLIGHT3)
                pWall->picnum = WALLLIGHTBUST3;

            if (pWall->picnum == WALLLIGHT4)
                pWall->picnum = WALLLIGHTBUST4;

            if (pWall->picnum == TECHLIGHT2)
                pWall->picnum = TECHLIGHTBUST2;

            if (pWall->picnum == TECHLIGHT4)
                pWall->picnum = TECHLIGHTBUST4;

            if (pWall->lotag == 0)
                return;

            sectNum = pWall->nextsector;

            if (sectNum < 0)
                return;

            int darkestWall = 0;

            pWall = &wall[sector[sectNum].wallptr];

            for (bssize_t i = sector[sectNum].wallnum; i > 0; i--, pWall++)
                if (pWall->shade > darkestWall)
                    darkestWall = pWall->shade;

            int const random = krand() & 1;

            for (bssize_t SPRITES_OF(STAT_EFFECTOR, i))
                if (SHT(i) == wall[wallNum].lotag && SLT(i) == SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT)
                {
                    T3(i) = random;
                    T4(i) = darkestWall;
                    T5(i) = 1;
                }

            break;
        }
    }
}

void A_DamageWall(int spriteNum, int wallNum, const vec3_t &vPos, int weaponNum)
{
    ud.returnvar[0] = -1;
    A_DamageWall_Internal(spriteNum, wallNum, vPos, weaponNum);
}

void Sect_DamageFloor_Internal(int const spriteNum, int const sectNum)
{
    int16_t tileNum = sector[sectNum].floorpicnum;
    if (g_tile[tileNum].flags & SFLAG_DAMAGEEVENT)
    {
        if (VM_OnEventWithReturn(EVENT_DAMAGEFLOOR, spriteNum, -1, sectNum) < 0)
            return;
    }

    // NOTE: pass RETURN in the dist argument, too.
    int const     RETURN_in = 131072 + sectNum;
    /* int32_t const returnValue = */ VM_OnEvent(EVENT_DAMAGEHPLANE, -1, -1, RETURN_in, RETURN_in);

#if 0
    // No hard-coded floor damage effects.
    if (returnValue < 0)
        return;
#endif
}

void Sect_DamageFloor(int const spriteNum, int const sectNum)
{
    ud.returnvar[0] = -1;
    Sect_DamageFloor_Internal(spriteNum, sectNum);
}

void Sect_DamageCeiling_Internal(int const spriteNum, int const sectNum)
{
    int16_t tileNum = sector[sectNum].ceilingpicnum;
    if (g_tile[tileNum].flags & SFLAG_DAMAGEEVENT)
    {
        if (VM_OnEventWithReturn(EVENT_DAMAGECEILING, spriteNum, -1, sectNum) < 0)
            return;
    }

    // NOTE: pass RETURN in the dist argument, too.
    int const     RETURN_in = 65536 + sectNum;
    int32_t const returnValue = VM_OnEvent(EVENT_DAMAGEHPLANE, -1, -1, RETURN_in, RETURN_in);

    if (returnValue < 0)
        return;

#ifndef EDUKE32_STANDALONE
    int16_t * const pPicnum = &sector[sectNum].ceilingpicnum;
#endif

    if (returnValue == (1<<20))
    {
        // Execute the hard-coded stuff without changing picnum (expected to
        // have been done by the event).
        goto GLASSBREAK_CODE;
    }

#ifndef EDUKE32_STANDALONE
    switch (DYNAMICTILEMAP(*pPicnum))
    {
        case WALLLIGHT1__STATIC: *pPicnum = WALLLIGHTBUST1; goto GLASSBREAK_CODE;
        case WALLLIGHT2__STATIC: *pPicnum = WALLLIGHTBUST2; goto GLASSBREAK_CODE;
        case WALLLIGHT3__STATIC: *pPicnum = WALLLIGHTBUST3; goto GLASSBREAK_CODE;
        case WALLLIGHT4__STATIC: *pPicnum = WALLLIGHTBUST4; goto GLASSBREAK_CODE;
        case TECHLIGHT2__STATIC: *pPicnum = TECHLIGHTBUST2; goto GLASSBREAK_CODE;
        case TECHLIGHT4__STATIC: *pPicnum = TECHLIGHTBUST4;
#else
    if (0)
    {
#endif
    GLASSBREAK_CODE:
#ifndef EDUKE32_STANDALONE
            A_SpawnCeilingGlass(g_player[myconnectindex].ps->i, sectNum, 10);
            A_PlaySound(GLASS_BREAKING, g_player[screenpeek].ps->i);
#endif
            if (sector[sectNum].hitag == 0)
            {
                for (bssize_t SPRITES_OF_SECT(sectNum, i))
                {
                    if (PN(i) == SECTOREFFECTOR && SLT(i) == SE_12_LIGHT_SWITCH)
                    {
                        for (bssize_t SPRITES_OF(STAT_EFFECTOR, j))
                            if (sprite[j].hitag == SHT(i))
                                actor[j].t_data[3] = 1;
                        break;
                    }
                }
            }

            int j = krand() & 1;

            for (bssize_t SPRITES_OF(STAT_EFFECTOR, i))
            {
                if (SHT(i) == sector[sectNum].hitag && SLT(i) == SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT)
                {
                    T3(i) = j;
                    T5(i) = 1;
                }
            }
    }
}

void Sect_DamageCeiling(int const spriteNum, int const sectNum)
{
    ud.returnvar[0] = -1;
    Sect_DamageCeiling_Internal(spriteNum, sectNum);
}

// hard coded props... :(
#ifndef EDUKE32_STANDALONE
void A_DamageObject_Duke3D(int spriteNum, int const dmgSrc)
{
    if (g_netClient)
        return;

    if (A_CheckSpriteFlags(spriteNum, SFLAG_DAMAGEEVENT))
    {
        if (VM_OnEventWithReturn(EVENT_DAMAGESPRITE, dmgSrc, -1, spriteNum) < 0)
            return;
    }

    spriteNum &= (MAXSPRITES-1);

    int radiusDamage = 0;

    if (A_CheckSpriteFlags(dmgSrc,SFLAG_PROJECTILE))
        if (SpriteProjectile[dmgSrc].workslike & PROJECTILE_RPG)
            radiusDamage = 1;

    switch (DYNAMICTILEMAP(PN(spriteNum)))
    {
    case GRATE1__STATIC:
        PN(spriteNum) = BGRATE1;
        CS(spriteNum) &= (65535-256-1);
        A_PlaySound(VENT_BUST, spriteNum);
        break;

    case FANSPRITE__STATIC:
        PN(spriteNum) = FANSPRITEBROKE;
        CS(spriteNum) &= (65535-257);

        if (sector[SECT(spriteNum)].floorpicnum == FANSHADOW)
            sector[SECT(spriteNum)].floorpicnum = FANSHADOWBROKE;

        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);

        for (bssize_t j=16; j>0; j--)
        {
            auto const pSprite = &sprite[spriteNum];
            RANDOMSCRAP(pSprite, spriteNum);
        }
        break;

    case OCEANSPRITE1__STATIC:
    case OCEANSPRITE2__STATIC:
    case OCEANSPRITE3__STATIC:
    case OCEANSPRITE4__STATIC:
    case OCEANSPRITE5__STATIC:
        A_Spawn(spriteNum,SMALLSMOKE);
        A_DeleteSprite(spriteNum);
        break;

    case QUEBALL__STATIC:
    case STRIPEBALL__STATIC:
        if (sprite[dmgSrc].picnum == QUEBALL || sprite[dmgSrc].picnum == STRIPEBALL)
        {
            sprite[dmgSrc].xvel = (sprite[spriteNum].xvel>>1)+(sprite[spriteNum].xvel>>2);
            sprite[dmgSrc].ang -= (SA(spriteNum)<<1)+1024;
            SA(spriteNum) = getangle(SX(spriteNum)-sprite[dmgSrc].x,SY(spriteNum)-sprite[dmgSrc].y)-512;
            if (g_sounds[POOLBALLHIT].num < 2)
                A_PlaySound(POOLBALLHIT, spriteNum);
        }
        else
        {
            if (krand()&3)
            {
                sprite[spriteNum].xvel = 164;
                sprite[spriteNum].ang = sprite[dmgSrc].ang;
            }
            else
            {
                A_SpawnWallGlass(spriteNum,-1,3);
                A_DeleteSprite(spriteNum);
            }
        }
        break;

    case TREE1__STATIC:
    case TREE2__STATIC:
    case TIRE__STATIC:
    case CONE__STATIC:
    case BOX__STATIC:
    {
        switch (DYNAMICTILEMAP(sprite[dmgSrc].picnum))
        {
        case RADIUSEXPLOSION__STATIC:
        case RPG__STATIC:
        case FIRELASER__STATIC:
        case HYDRENT__STATIC:
        case HEAVYHBOMB__STATIC:
            radiusDamage = 1;
            break;
        }

        if (radiusDamage == 1)
            if (T1(spriteNum) == 0)
            {
                CS(spriteNum) &= ~257;
                T1(spriteNum) = 1;
                A_Spawn(spriteNum,BURNING);
            }
        break;
    }

    case CACTUS__STATIC:
    {
        switch (DYNAMICTILEMAP(sprite[dmgSrc].picnum))
        {
        case RADIUSEXPLOSION__STATIC:
        case RPG__STATIC:
        case FIRELASER__STATIC:
        case HYDRENT__STATIC:
        case HEAVYHBOMB__STATIC:
            radiusDamage = 1;
            break;
        }

        if (radiusDamage == 1)
        {
            for (bssize_t k=64; k>0; k--)
            {
                int newSprite =
                    A_InsertSprite(SECT(spriteNum), SX(spriteNum), SY(spriteNum), SZ(spriteNum) - (krand() % (48 << 8)), SCRAP3 + (krand() & 3), -8, 48, 48,
                        krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (sprite[spriteNum].zvel >> 2), spriteNum, 5);
                sprite[newSprite].pal = 8;
            }
            //        case CACTUSBROKE:
            if (PN(spriteNum) == CACTUS)
                PN(spriteNum) = CACTUSBROKE;
            CS(spriteNum) &= ~257;
        }
        break;
    }

    case HANGLIGHT__STATIC:
    case GENERICPOLE2__STATIC:
        for (bssize_t k=6; k>0; k--)
            A_InsertSprite(SECT(spriteNum),SX(spriteNum),SY(spriteNum),SZ(spriteNum)-ZOFFSET3,SCRAP1+(krand()&15),-8,48,48,krand()&2047,(krand()&63)+64,-(krand()&4095)-(sprite[spriteNum].zvel>>2),spriteNum,5);
        A_PlaySound(GLASS_HEAVYBREAK,spriteNum);
        A_DeleteSprite(spriteNum);
        break;

    case WATERFOUNTAIN__STATIC:
        //    case WATERFOUNTAIN+1:
        //    case WATERFOUNTAIN+2:
        PN(spriteNum) = WATERFOUNTAINBROKE;
        A_Spawn(spriteNum,TOILETWATER);
        break;

    case SATELITE__STATIC:
    case FUELPOD__STATIC:
    case SOLARPANNEL__STATIC:
    case ANTENNA__STATIC:
        if (sprite[dmgSrc].extra != G_DefaultActorHealth(SHOTSPARK1))
        {
            for (bssize_t j=15; j>0; j--)
                A_InsertSprite(SECT(spriteNum),SX(spriteNum),SY(spriteNum),sector[SECT(spriteNum)].floorz-ZOFFSET4-(j<<9),SCRAP1+(krand()&15),-8,64,64,
                               krand()&2047,(krand()&127)+64,-(krand()&511)-256,spriteNum,5);
            A_Spawn(spriteNum,EXPLOSION2);
            A_DeleteSprite(spriteNum);
        }
        break;

    case BOTTLE1__STATIC:
    case BOTTLE2__STATIC:
    case BOTTLE3__STATIC:
    case BOTTLE4__STATIC:
    case BOTTLE5__STATIC:
    case BOTTLE6__STATIC:
    case BOTTLE8__STATIC:
    case BOTTLE10__STATIC:
    case BOTTLE11__STATIC:
    case BOTTLE12__STATIC:
    case BOTTLE13__STATIC:
    case BOTTLE14__STATIC:
    case BOTTLE15__STATIC:
    case BOTTLE16__STATIC:
    case BOTTLE17__STATIC:
    case BOTTLE18__STATIC:
    case BOTTLE19__STATIC:
    case WATERFOUNTAINBROKE__STATIC:
    case DOMELITE__STATIC:
    case SUSHIPLATE1__STATIC:
    case SUSHIPLATE2__STATIC:
    case SUSHIPLATE3__STATIC:
    case SUSHIPLATE4__STATIC:
    case SUSHIPLATE5__STATIC:
    case WAITTOBESEATED__STATIC:
    case VASE__STATIC:
    case STATUEFLASH__STATIC:
    case STATUE__STATIC:
        if (PN(spriteNum) == BOTTLE10)
            A_SpawnMultiple(spriteNum, MONEY, 4+(krand()&3));
        else if (PN(spriteNum) == STATUE || PN(spriteNum) == STATUEFLASH)
        {
            A_SpawnRandomGlass(spriteNum,-1,40);
            A_PlaySound(GLASS_HEAVYBREAK,spriteNum);
        }
        else if (PN(spriteNum) == VASE)
            A_SpawnWallGlass(spriteNum,-1,40);

        A_PlaySound(GLASS_BREAKING,spriteNum);
        SA(spriteNum) = krand()&2047;
        A_SpawnWallGlass(spriteNum,-1,8);
        A_DeleteSprite(spriteNum);
        break;

    case FETUS__STATIC:
        PN(spriteNum) = FETUSBROKE;
        A_PlaySound(GLASS_BREAKING,spriteNum);
        A_SpawnWallGlass(spriteNum,-1,10);
        break;

    case FETUSBROKE__STATIC:
        for (bssize_t j=48; j>0; j--)
        {
            A_Shoot(spriteNum,BLOODSPLAT1);
            SA(spriteNum) += 333;
        }
        A_PlaySound(GLASS_HEAVYBREAK,spriteNum);
        A_PlaySound(SQUISHED,spriteNum);
        fallthrough__;
    case BOTTLE7__STATIC:
        A_PlaySound(GLASS_BREAKING,spriteNum);
        A_SpawnWallGlass(spriteNum,-1,10);
        A_DeleteSprite(spriteNum);
        break;

    case HYDROPLANT__STATIC:
        PN(spriteNum) = BROKEHYDROPLANT;
        A_PlaySound(GLASS_BREAKING,spriteNum);
        A_SpawnWallGlass(spriteNum,-1,10);
        break;

    case FORCESPHERE__STATIC:
        sprite[spriteNum].xrepeat = 0;
        actor[OW(spriteNum)].t_data[0] = 32;
        actor[OW(spriteNum)].t_data[1] = !actor[OW(spriteNum)].t_data[1];
        actor[OW(spriteNum)].t_data[2] ++;
        A_Spawn(spriteNum,EXPLOSION2);
        break;

    case BROKEHYDROPLANT__STATIC:
        A_PlaySound(GLASS_BREAKING,spriteNum);
        A_SpawnWallGlass(spriteNum,-1,5);
        A_DeleteSprite(spriteNum);
        break;

    case TOILET__STATIC:
        PN(spriteNum) = TOILETBROKE;
        CS(spriteNum) |= (krand()&1)<<2;
        CS(spriteNum) &= ~257;
        A_Spawn(spriteNum,TOILETWATER);
        A_PlaySound(GLASS_BREAKING,spriteNum);
        break;

    case STALL__STATIC:
        PN(spriteNum) = STALLBROKE;
        CS(spriteNum) |= (krand()&1)<<2;
        CS(spriteNum) &= ~257;
        A_Spawn(spriteNum,TOILETWATER);
        A_PlaySound(GLASS_HEAVYBREAK,spriteNum);
        break;

    case HYDRENT__STATIC:
        PN(spriteNum) = BROKEFIREHYDRENT;
        A_Spawn(spriteNum,TOILETWATER);

        //            for(k=0;k<5;k++)
        //          {
        //            j = A_InsertSprite(SECT,SX,SY,SZ-(krand()%(48<<8)),SCRAP3+(krand()&3),-8,48,48,krand()&2047,(krand()&63)+64,-(krand()&4095)-(sprite[i].zvel>>2),i,5);
        //          sprite[j].pal = 2;
        //    }
        A_PlaySound(GLASS_HEAVYBREAK,spriteNum);
        break;

    case CIRCLEPANNEL__STATIC:
        PN(spriteNum) = CIRCLEPANNELBROKE;
        CS(spriteNum) &= (65535-256-1);
        A_PlaySound(VENT_BUST,spriteNum);
        break;

    case PANNEL1__STATIC:
    case PANNEL2__STATIC:
        PN(spriteNum) = BPANNEL1;
        CS(spriteNum) &= (65535-256-1);
        A_PlaySound(VENT_BUST,spriteNum);
        break;

    case PANNEL3__STATIC:
        PN(spriteNum) = BPANNEL3;
        CS(spriteNum) &= (65535-256-1);
        A_PlaySound(VENT_BUST,spriteNum);
        break;

    case PIPE1__STATIC:
    case PIPE2__STATIC:
    case PIPE3__STATIC:
    case PIPE4__STATIC:
    case PIPE5__STATIC:
    case PIPE6__STATIC:
    {
        switch (DYNAMICTILEMAP(PN(spriteNum)))
        {
        case PIPE1__STATIC:
            PN(spriteNum)=PIPE1B;
            break;
        case PIPE2__STATIC:
            PN(spriteNum)=PIPE2B;
            break;
        case PIPE3__STATIC:
            PN(spriteNum)=PIPE3B;
            break;
        case PIPE4__STATIC:
            PN(spriteNum)=PIPE4B;
            break;
        case PIPE5__STATIC:
            PN(spriteNum)=PIPE5B;
            break;
        case PIPE6__STATIC:
            PN(spriteNum)=PIPE6B;
            break;
        }

        int newSprite = A_Spawn(spriteNum, STEAM);
        sprite[newSprite].z = sector[SECT(spriteNum)].floorz-ZOFFSET5;
        break;
    }

    case MONK__STATIC:
    case LUKE__STATIC:
    case INDY__STATIC:
    case JURYGUY__STATIC:
        A_PlaySound(SLT(spriteNum),spriteNum);
        A_Spawn(spriteNum,SHT(spriteNum));
        fallthrough__;
    case SPACEMARINE__STATIC:
        sprite[spriteNum].extra -= sprite[dmgSrc].extra;
        if (sprite[spriteNum].extra > 0) break;
        SA(spriteNum) = krand()&2047;
        A_Shoot(spriteNum,BLOODSPLAT1);
        SA(spriteNum) = krand()&2047;
        A_Shoot(spriteNum,BLOODSPLAT2);
        SA(spriteNum) = krand()&2047;
        A_Shoot(spriteNum,BLOODSPLAT3);
        SA(spriteNum) = krand()&2047;
        A_Shoot(spriteNum,BLOODSPLAT4);
        SA(spriteNum) = krand()&2047;
        A_Shoot(spriteNum,BLOODSPLAT1);
        SA(spriteNum) = krand()&2047;
        A_Shoot(spriteNum,BLOODSPLAT2);
        SA(spriteNum) = krand()&2047;
        A_Shoot(spriteNum,BLOODSPLAT3);
        SA(spriteNum) = krand()&2047;
        A_Shoot(spriteNum,BLOODSPLAT4);
        A_DoGuts(spriteNum,JIBS1,1);
        A_DoGuts(spriteNum,JIBS2,2);
        A_DoGuts(spriteNum,JIBS3,3);
        A_DoGuts(spriteNum,JIBS4,4);
        A_DoGuts(spriteNum,JIBS5,1);
        A_DoGuts(spriteNum,JIBS3,6);
        S_PlaySound(SQUISHED);
        A_DeleteSprite(spriteNum);
        break;

    case CHAIR1__STATIC:
    case CHAIR2__STATIC:
        PN(spriteNum) = BROKENCHAIR;
        CS(spriteNum) = 0;
        break;

    case CHAIR3__STATIC:
    case MOVIECAMERA__STATIC:
    case SCALE__STATIC:
    case VACUUM__STATIC:
    case CAMERALIGHT__STATIC:
    case IVUNIT__STATIC:
    case POT1__STATIC:
    case POT2__STATIC:
    case POT3__STATIC:
    case TRIPODCAMERA__STATIC:
        A_PlaySound(GLASS_HEAVYBREAK,spriteNum);
        for (bssize_t j=16; j>0; j--)
        {
            auto const pSprite = &sprite[spriteNum];
            RANDOMSCRAP(pSprite, spriteNum);
        }
        A_DeleteSprite(spriteNum);
        break;

    case PLAYERONWATER__STATIC:
        spriteNum = OW(spriteNum);
        fallthrough__;
    default:
        if ((sprite[spriteNum].cstat&16) && SHT(spriteNum) == 0 && SLT(spriteNum) == 0 && sprite[spriteNum].statnum == STAT_DEFAULT)
            break;

        if ((sprite[dmgSrc].picnum == FREEZEBLAST || sprite[dmgSrc].owner != spriteNum) && sprite[spriteNum].statnum != STAT_PROJECTILE)
        {
            if (A_CheckEnemySprite(&sprite[spriteNum]) == 1)
            {
                if (sprite[dmgSrc].picnum == RPG)
                    sprite[dmgSrc].extra <<= 1;

                if ((PN(spriteNum) != DRONE) && (PN(spriteNum) != ROTATEGUN) && (PN(spriteNum) != COMMANDER)
                    && (PN(spriteNum) < GREENSLIME || PN(spriteNum) > GREENSLIME + 7))
                    if (sprite[dmgSrc].picnum != FREEZEBLAST)
                        if (!A_CheckSpriteFlags(spriteNum, SFLAG_BADGUY) || A_CheckSpriteFlags(spriteNum, SFLAG_HURTSPAWNBLOOD))
                        {
                            int const newSprite = A_Spawn(dmgSrc, JIBS6);
                            sprite[newSprite].z += ZOFFSET6;
                            if (sprite[dmgSrc].pal == 6)
                                sprite[newSprite].pal = 6;
                            sprite[newSprite].xvel    = 16;
                            sprite[newSprite].xrepeat = sprite[newSprite].yrepeat = 24;
                            sprite[newSprite].ang += 32 - (krand() & 63);
                        }

                int const damageOwner = sprite[dmgSrc].owner;

                if (damageOwner >= 0 && sprite[damageOwner].picnum == APLAYER && PN(spriteNum) != ROTATEGUN && PN(spriteNum) != DRONE)
                    if (g_player[P_Get(damageOwner)].ps->curr_weapon == SHOTGUN_WEAPON)
                        if (!A_CheckSpriteFlags(spriteNum, SFLAG_BADGUY) || A_CheckSpriteFlags(spriteNum, SFLAG_HURTSPAWNBLOOD))
                        {
                            A_Shoot(spriteNum, BLOODSPLAT3);
                            A_Shoot(spriteNum, BLOODSPLAT1);
                            A_Shoot(spriteNum, BLOODSPLAT2);
                            A_Shoot(spriteNum, BLOODSPLAT4);
                        }

                if (!A_CheckSpriteFlags(spriteNum, SFLAG_NODAMAGEPUSH))
                {
                    if (sprite[spriteNum].extra > 0)
                    {
                        if ((sprite[spriteNum].cstat & 48) == 0)
                            SA(spriteNum)          = (sprite[dmgSrc].ang + 1024) & 2047;
                        sprite[spriteNum].xvel  = -(sprite[dmgSrc].extra << 2);
                        int16_t sectNum = SECT(spriteNum);
                        pushmove(&sprite[spriteNum].pos, &sectNum, 128L, (4L << 8), (4L << 8), CLIPMASK0);
                        if (sectNum != SECT(spriteNum) && (unsigned)sectNum < MAXSECTORS)
                            changespritesect(spriteNum, sectNum);
                    }
                }

                if (sprite[spriteNum].statnum == STAT_ZOMBIEACTOR)
                {
                    changespritestat(spriteNum, STAT_ACTOR);
                    actor[spriteNum].timetosleep = SLEEPTIME;
                }
                if ((sprite[spriteNum].xrepeat < 24 || PN(spriteNum) == SHARK) && sprite[dmgSrc].picnum == SHRINKSPARK)
                    return;
            }

            if (sprite[spriteNum].statnum != STAT_ZOMBIEACTOR)
            {
                if (sprite[dmgSrc].picnum == FREEZEBLAST && ((PN(spriteNum) == APLAYER && sprite[spriteNum].pal == 1) || (g_freezerSelfDamage == 0 && sprite[dmgSrc].owner == spriteNum)))
                    return;
                actor[spriteNum].picnum = sprite[dmgSrc].picnum;
                actor[spriteNum].extra += sprite[dmgSrc].extra;
                actor[spriteNum].ang    = sprite[dmgSrc].ang;
                actor[spriteNum].owner  = sprite[dmgSrc].owner;

                if(A_CheckSpriteFlags(spriteNum, SFLAG_DAMAGEEVENT))
                    VM_OnEventWithReturn(EVENT_POSTDAMAGESPRITE, dmgSrc, -1, spriteNum);
            }

            if (sprite[spriteNum].statnum == STAT_PLAYER)
            {
                auto ps = g_player[P_Get(spriteNum)].ps;

                if (ps->newowner >= 0)
                    G_ClearCameraView(ps);

                if (sprite[spriteNum].xrepeat < 24 && sprite[dmgSrc].picnum == SHRINKSPARK)
                    return;

                if (sprite[actor[spriteNum].owner].picnum != APLAYER)
                    if (ud.player_skill >= 3)
                        sprite[dmgSrc].extra += (sprite[dmgSrc].extra>>1);
            }
        }

        break;
    }
}
#endif

void A_DamageObject_Generic(int spriteNum, int const dmgSrc)
{
    if (g_netClient)
        return;

    if (A_CheckSpriteFlags(spriteNum, SFLAG_DAMAGEEVENT))
    {
        if (VM_OnEventWithReturn(EVENT_DAMAGESPRITE, dmgSrc, -1, spriteNum) < 0)
            return;
    }

    spriteNum &= (MAXSPRITES-1);

    switch (DYNAMICTILEMAP(PN(spriteNum)))
    {
    case GRATE1__STATIC:
        PN(spriteNum) = BGRATE1;
        CS(spriteNum) &= (65535-256-1);
        break;

    case FANSPRITE__STATIC:
        PN(spriteNum) = FANSPRITEBROKE;
        CS(spriteNum) &= (65535-257);
        break;

    case PLAYERONWATER__STATIC:
        spriteNum = OW(spriteNum);
        fallthrough__;
    default:
        if ((sprite[spriteNum].cstat&16) && SHT(spriteNum) == 0 && SLT(spriteNum) == 0 && sprite[spriteNum].statnum == STAT_DEFAULT)
            break;

        if (sprite[dmgSrc].owner != spriteNum && sprite[spriteNum].statnum != STAT_PROJECTILE)
        {
            if (A_CheckEnemySprite(&sprite[spriteNum]))
            {
                if (!A_CheckSpriteFlags(spriteNum, SFLAG_NODAMAGEPUSH))
                {
                    if (sprite[spriteNum].extra > 0)
                    {
                        if ((sprite[spriteNum].cstat & 48) == 0)
                            SA(spriteNum) = (sprite[dmgSrc].ang + 1024) & 2047;
                        sprite[spriteNum].xvel  = -(sprite[dmgSrc].extra << 2);
                        int16_t sectNum = SECT(spriteNum);
                        pushmove(&sprite[spriteNum].pos, &sectNum, 128L, (4L << 8), (4L << 8), CLIPMASK0);
                        if (sectNum != SECT(spriteNum) && (unsigned)sectNum < MAXSECTORS)
                            changespritesect(spriteNum, sectNum);
                    }
                }

                if (sprite[spriteNum].statnum == STAT_ZOMBIEACTOR)
                {
                    changespritestat(spriteNum, STAT_ACTOR);
                    actor[spriteNum].timetosleep = SLEEPTIME;
                }
            }

            if (sprite[spriteNum].statnum != STAT_ZOMBIEACTOR)
            {
                actor[spriteNum].picnum = sprite[dmgSrc].picnum;
                actor[spriteNum].extra += sprite[dmgSrc].extra;
                actor[spriteNum].ang    = sprite[dmgSrc].ang;
                actor[spriteNum].owner  = sprite[dmgSrc].owner;

                if(A_CheckSpriteFlags(spriteNum, SFLAG_DAMAGEEVENT))
                    VM_OnEventWithReturn(EVENT_POSTDAMAGESPRITE, dmgSrc, -1, spriteNum);
            }

            if (sprite[spriteNum].statnum == STAT_PLAYER)
            {
                auto ps = g_player[P_Get(spriteNum)].ps;

                if (ps->newowner >= 0)
                    G_ClearCameraView(ps);

                if (sprite[actor[spriteNum].owner].picnum != APLAYER)
                    if (ud.player_skill >= 3)
                        sprite[dmgSrc].extra += (sprite[dmgSrc].extra>>1);
            }
        }

        break;
    }
}

void A_DamageObject(int spriteNum, int const dmgSrc)
{
    ud.returnvar[0] = -1;

#ifndef EDUKE32_STANDALONE
    if (!FURY)
        A_DamageObject_Duke3D(spriteNum, dmgSrc);
    else
#endif
        A_DamageObject_Generic(spriteNum, dmgSrc);
}

void G_AlignWarpElevators(void)
{
    for (bssize_t SPRITES_OF(STAT_EFFECTOR, i))
    {
        if (SLT(i) == SE_17_WARP_ELEVATOR && SS(i) > 16)
        {
            for (bssize_t SPRITES_OF(STAT_EFFECTOR, j))
            {
                if (i != j && sprite[j].lotag == SE_17_WARP_ELEVATOR && SHT(i) == sprite[j].hitag)
                {
                    sector[sprite[j].sectnum].floorz   = sector[SECT(i)].floorz;
                    sector[sprite[j].sectnum].ceilingz = sector[SECT(i)].ceilingz;
                }
            }
        }
    }
}


static int P_CheckDetonatorSpecialCase(DukePlayer_t *const pPlayer, int weaponNum)
{
    if (weaponNum == HANDBOMB_WEAPON && pPlayer->ammo_amount[HANDBOMB_WEAPON] == 0)
    {
        int spriteNum = headspritestat[STAT_ACTOR];

        while (spriteNum >= 0)
        {
            if (sprite[spriteNum].picnum == HEAVYHBOMB && sprite[spriteNum].owner == pPlayer->i)
                return 1;

            spriteNum = nextspritestat[spriteNum];
        }
    }

    return 0;
}

void P_HandleSharedKeys(int playerNum)
{
    auto const pPlayer = g_player[playerNum].ps;

    if (pPlayer->cheat_phase == 1) return;

    uint32_t playerBits = g_player[playerNum].input->bits, weaponNum;

    // 1<<0  =  jump
    // 1<<1  =  crouch
    // 1<<2  =  fire
    // 1<<3  =  aim up
    // 1<<4  =  aim down
    // 1<<5  =  run
    // 1<<6  =  look left
    // 1<<7  =  look right
    // 15<<8 = !weapon selection (bits 8-11)
    // 1<<12 = !steroids
    // 1<<13 =  look up
    // 1<<14 =  look down
    // 1<<15 = !nightvis
    // 1<<16 = !medkit
    // 1<<17 =  (multiflag==1) ? changes meaning of bits 18 and 19
    // 1<<18 =  centre view
    // 1<<19 = !holster weapon
    // 1<<20 = !inventory left
    // 1<<21 = !pause
    // 1<<22 = !quick kick
    // 1<<23 =  aim mode
    // 1<<24 = !holoduke
    // 1<<25 = !jetpack
    // 1<<26 =  g_gameQuit
    // 1<<27 = !inventory right
    // 1<<28 = !turn around
    // 1<<29 = !open
    // 1<<30 = !inventory
    // 1<<31 = !escape

    int const aimMode = pPlayer->aim_mode;

    pPlayer->aim_mode = (playerBits>>SK_AIMMODE)&1;
    if (pPlayer->aim_mode < aimMode)
        pPlayer->return_to_center = 9;

    if (TEST_SYNC_KEY(playerBits, SK_QUICK_KICK) && pPlayer->quick_kick == 0)
        if (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) != KNEE_WEAPON || pPlayer->kickback_pic == 0)
        {
            if (VM_OnEvent(EVENT_QUICKKICK,g_player[playerNum].ps->i,playerNum) == 0)
            {
                pPlayer->quick_kick = 14;
                if (pPlayer->fta == 0 || pPlayer->ftq == 80)
                    P_DoQuote(QUOTE_MIGHTY_FOOT,pPlayer);
            }
        }

    weaponNum = playerBits & ((15u<<SK_WEAPON_BITS)|BIT(SK_STEROIDS)|BIT(SK_NIGHTVISION)|BIT(SK_MEDKIT)|BIT(SK_QUICK_KICK)| \
                   BIT(SK_HOLSTER)|BIT(SK_INV_LEFT)|BIT(SK_PAUSE)|BIT(SK_HOLODUKE)|BIT(SK_JETPACK)|BIT(SK_INV_RIGHT)| \
                   BIT(SK_TURNAROUND)|BIT(SK_OPEN)|BIT(SK_INVENTORY)|BIT(SK_ESCAPE));
    playerBits = weaponNum & ~pPlayer->interface_toggle;
    pPlayer->interface_toggle |= playerBits | ((playerBits&0xf00)?0xf00:0);
    pPlayer->interface_toggle &= weaponNum | ((weaponNum&0xf00)?0xf00:0);

    if (playerBits && TEST_SYNC_KEY(playerBits, SK_MULTIFLAG) == 0)
    {
        if (TEST_SYNC_KEY(playerBits, SK_PAUSE))
        {
            KB_ClearKeyDown(sc_Pause);
            if (ud.pause_on)
                ud.pause_on = 0;
            else ud.pause_on = 1+SHIFTS_IS_PRESSED;
            if (ud.pause_on)
            {
                S_PauseMusic(true);
                S_PauseSounds(true);
            }
            else
            {
                if (ud.config.MusicToggle) S_PauseMusic(false);

                S_PauseSounds(false);

                pub = NUMPAGES;
                pus = NUMPAGES;
            }
        }

        if (ud.pause_on) return;

        if (sprite[pPlayer->i].extra <= 0) return;		// if dead...

        if (TEST_SYNC_KEY(playerBits, SK_INVENTORY) && pPlayer->newowner == -1)	// inventory button generates event for selected item
        {
            if (VM_OnEvent(EVENT_INVENTORY,g_player[playerNum].ps->i,playerNum) == 0)
            {
                switch (pPlayer->inven_icon)
                {
                    case ICON_JETPACK: playerBits |= BIT(SK_JETPACK); break;
                    case ICON_HOLODUKE: playerBits |= BIT(SK_HOLODUKE); break;
                    case ICON_HEATS: playerBits |= BIT(SK_NIGHTVISION); break;
                    case ICON_FIRSTAID: playerBits |= BIT(SK_MEDKIT); break;
                    case ICON_STEROIDS: playerBits |= BIT(SK_STEROIDS); break;
                }
            }
        }

        if (TEST_SYNC_KEY(playerBits, SK_NIGHTVISION))
        {
            if (VM_OnEvent(EVENT_USENIGHTVISION,g_player[playerNum].ps->i,playerNum) == 0
                    &&  pPlayer->inv_amount[GET_HEATS] > 0)
            {
                pPlayer->heat_on = !pPlayer->heat_on;
                P_UpdateScreenPal(pPlayer);
                pPlayer->inven_icon = ICON_HEATS;
#ifndef EDUKE32_STANDALONE
                A_PlaySound(NITEVISION_ONOFF,pPlayer->i);
#endif
                P_DoQuote(QUOTE_NVG_OFF-!!pPlayer->heat_on,pPlayer);
            }
        }

        if (TEST_SYNC_KEY(playerBits, SK_STEROIDS))
        {
            if (VM_OnEvent(EVENT_USESTEROIDS,g_player[playerNum].ps->i,playerNum) == 0)
            {
                if (pPlayer->inv_amount[GET_STEROIDS] == 400)
                {
                    pPlayer->inv_amount[GET_STEROIDS]--;
#ifndef EDUKE32_STANDALONE
                    A_PlaySound(DUKE_TAKEPILLS,pPlayer->i);
#endif
                    P_DoQuote(QUOTE_USED_STEROIDS,pPlayer);
                }
                if (pPlayer->inv_amount[GET_STEROIDS] > 0)
                    pPlayer->inven_icon = ICON_STEROIDS;
            }
            return;		// is there significance to returning?
        }

        if (pPlayer->newowner == -1 && (TEST_SYNC_KEY(playerBits, SK_INV_LEFT) || TEST_SYNC_KEY(playerBits, SK_INV_RIGHT)))
        {
            pPlayer->invdisptime = GAMETICSPERSEC*2;

            int const inventoryRight = !!(TEST_SYNC_KEY(playerBits, SK_INV_RIGHT));

            int32_t inventoryIcon = pPlayer->inven_icon;

            int i = 0;

CHECKINV1:
            if (i < 9)
            {
                i++;

                switch (inventoryIcon)
                {
                    case ICON_JETPACK:
                    case ICON_SCUBA:
                    case ICON_STEROIDS:
                    case ICON_HOLODUKE:
                    case ICON_HEATS:
                        if (pPlayer->inv_amount[icon_to_inv[inventoryIcon]] > 0 && i > 1)
                            break;
                        if (inventoryRight)
                            inventoryIcon++;
                        else
                            inventoryIcon--;
                        goto CHECKINV1;
                    case ICON_NONE:
                    case ICON_FIRSTAID:
                        if (pPlayer->inv_amount[GET_FIRSTAID] > 0 && i > 1)
                            break;
                        inventoryIcon = inventoryRight ? 2 : 7;
                        goto CHECKINV1;
                    case ICON_BOOTS:
                        if (pPlayer->inv_amount[GET_BOOTS] > 0 && i > 1)
                            break;
                        inventoryIcon = inventoryRight ? 1 : 6;
                        goto CHECKINV1;
                }
            }
            else inventoryIcon = 0;

            if (TEST_SYNC_KEY(playerBits, SK_INV_LEFT))   // Inventory_Left
            {
                /*Gv_SetVar(g_iReturnVarID,dainv,g_player[snum].ps->i,snum);*/
                inventoryIcon = VM_OnEventWithReturn(EVENT_INVENTORYLEFT,g_player[playerNum].ps->i,playerNum, inventoryIcon);
            }
            else if (TEST_SYNC_KEY(playerBits, SK_INV_RIGHT))   // Inventory_Right
            {
                /*Gv_SetVar(g_iReturnVarID,dainv,g_player[snum].ps->i,snum);*/
                inventoryIcon = VM_OnEventWithReturn(EVENT_INVENTORYRIGHT,g_player[playerNum].ps->i,playerNum, inventoryIcon);
            }

            if (inventoryIcon >= 1)
            {
                pPlayer->inven_icon = inventoryIcon;

                static const int32_t invQuotes[8] = { QUOTE_MEDKIT, QUOTE_STEROIDS, QUOTE_HOLODUKE,
                    QUOTE_JETPACK, QUOTE_NVG, QUOTE_SCUBA, QUOTE_BOOTS, 0 };

                if (inventoryIcon-1 < ARRAY_SSIZE(invQuotes))
                    P_DoQuote(invQuotes[inventoryIcon-1], pPlayer);
            }
        }

        weaponNum = ((playerBits&(15<<SK_WEAPON_BITS))>>SK_WEAPON_BITS) - 1;

        switch ((int32_t)weaponNum)
        {
        case -1:
            break;
        default:
            weaponNum = VM_OnEventWithReturn(EVENT_WEAPKEY1+weaponNum,pPlayer->i,playerNum, weaponNum);
            break;
        case 10:
            weaponNum = VM_OnEventWithReturn(EVENT_PREVIOUSWEAPON,pPlayer->i,playerNum, weaponNum);
            break;
        case 11:
            weaponNum = VM_OnEventWithReturn(EVENT_NEXTWEAPON,pPlayer->i,playerNum, weaponNum);
            break;
        case 12:
            weaponNum = VM_OnEventWithReturn(EVENT_ALTWEAPON,pPlayer->i,playerNum, weaponNum);
            break;
        case 13:
            weaponNum = VM_OnEventWithReturn(EVENT_LASTWEAPON,pPlayer->i,playerNum, weaponNum);
            break;
        }

        // NOTE: it is assumed that the above events return either -1 or a
        // valid weapon index. Presumably, neither other negative numbers nor
        // positive ones >= MAX_WEAPONS are allowed. However, the code below is
        // a bit inconsistent in checking "j".

        if (pPlayer->reloading == 1)
            weaponNum = -1;
        else if ((uint32_t)weaponNum < MAX_WEAPONS && pPlayer->kickback_pic == 1 && pPlayer->weapon_pos == 1)
        {
            pPlayer->wantweaponfire = weaponNum;
            pPlayer->kickback_pic = 0;
        }

        if ((int32_t)weaponNum != -1 && pPlayer->last_pissed_time <= (GAMETICSPERSEC * 218) && pPlayer->show_empty_weapon == 0 &&
            pPlayer->kickback_pic == 0 && pPlayer->quick_kick == 0 && sprite[pPlayer->i].xrepeat > 32 && pPlayer->access_incs == 0 &&
            pPlayer->knee_incs == 0)
        {
            //            if(  ( p->weapon_pos == 0 || ( p->holster_weapon && p->weapon_pos == WEAPON_POS_LOWER ) ))
            {
                if (weaponNum == 10 || weaponNum == 11)
                {
                    int currentWeapon = pPlayer->curr_weapon;

                    weaponNum = (weaponNum == 10 ? -1 : 1);  // JBF: prev (-1) or next (1) weapon choice
                    int i = currentWeapon;

                    while ((currentWeapon >= 0 && currentWeapon < 11) || (PLUTOPAK && currentWeapon == GROW_WEAPON))
                    {
                        // this accounts for the expander when handling next/previous

                        switch (currentWeapon)
                        {
                            case DEVISTATOR_WEAPON:
                                if ((int32_t) weaponNum == -1)
                                {
                                    if (PLUTOPAK)
                                        currentWeapon = GROW_WEAPON;
                                    else
                                        currentWeapon--;
                                }
                                else
                                    currentWeapon++;
                                break;

                            case GROW_WEAPON:
                                currentWeapon = ((int32_t) weaponNum == -1) ? SHRINKER_WEAPON : DEVISTATOR_WEAPON;
                                break;

                            case SHRINKER_WEAPON:
                                if ((int32_t)weaponNum == 1)
                                {
                                    if (PLUTOPAK)
                                        currentWeapon = GROW_WEAPON;
                                    else
                                        currentWeapon++;
                                }
                                else
                                    currentWeapon--;
                                break;

                            case HANDREMOTE_WEAPON:
                                i = currentWeapon = HANDBOMB_WEAPON;
                                fallthrough__;
                            default:
                                currentWeapon += weaponNum;
                                break;
                        }

                        if (currentWeapon == -1) currentWeapon = FREEZE_WEAPON;
                        else if (currentWeapon == 10) currentWeapon = KNEE_WEAPON;

                        if (((pPlayer->gotweapon & (1<<currentWeapon)) && pPlayer->ammo_amount[currentWeapon] > 0) || P_CheckDetonatorSpecialCase(pPlayer, currentWeapon))
                        {
                            weaponNum = currentWeapon;
                            break;
                        }

                        if (i == currentWeapon) // absolutely no weapons, so use foot
                        {
                            weaponNum = KNEE_WEAPON;
                            break;
                        }
                    }

                    if (weaponNum == SHRINKER_WEAPON)
                        pPlayer->subweapon &= ~(1 << GROW_WEAPON);
                    else if (weaponNum == GROW_WEAPON)
                        pPlayer->subweapon |= (1<<GROW_WEAPON);
                }

                // last used weapon will depend on subweapon
                if (weaponNum >= 12) // alt weapon, last used weapon
                {
                    uint32_t const weaponNumSwitch = weaponNum == 13 ? pPlayer->last_used_weapon : pPlayer->curr_weapon;
                    switch (weaponNumSwitch)
                    {
                        case HANDREMOTE_WEAPON:
                            weaponNum = HANDBOMB_WEAPON;
                            break;
                        case GROW_WEAPON:
                            weaponNum = SHRINKER_WEAPON;
                            break;
                        default:
                            weaponNum = weaponNumSwitch;
                            break;
                    }
                }

                P_SetWeaponGamevars(playerNum, pPlayer);

                weaponNum = VM_OnEventWithReturn(EVENT_SELECTWEAPON,pPlayer->i,playerNum, weaponNum);

                // XXX: any signifcance to "<= MAX_WEAPONS" instead of "<"?
                if ((int32_t)weaponNum != -1 && weaponNum <= MAX_WEAPONS)
                {
                    if (P_CheckDetonatorSpecialCase(pPlayer, weaponNum))
                    {
                        pPlayer->gotweapon |= (1<<HANDBOMB_WEAPON);
                        weaponNum = HANDREMOTE_WEAPON;
                    }

                    if (weaponNum == SHRINKER_WEAPON && PLUTOPAK)   // JBF 20040116: so we don't select the grower with v1.3d
                    {
                        if (screenpeek == playerNum) pus = NUMPAGES;

                        if (pPlayer->curr_weapon != GROW_WEAPON && pPlayer->curr_weapon != SHRINKER_WEAPON)
                        {
                            if (pPlayer->ammo_amount[GROW_WEAPON] > 0)
                            {
                                if ((pPlayer->subweapon&(1<<GROW_WEAPON)) == (1<<GROW_WEAPON))
                                    weaponNum = GROW_WEAPON;
                                else if (pPlayer->ammo_amount[SHRINKER_WEAPON] == 0)
                                {
                                    weaponNum = GROW_WEAPON;
                                    pPlayer->subweapon |= (1<<GROW_WEAPON);
                                }
                            }
                            else if (pPlayer->ammo_amount[SHRINKER_WEAPON] > 0)
                                pPlayer->subweapon &= ~(1<<GROW_WEAPON);
                        }
                        else if (pPlayer->curr_weapon == SHRINKER_WEAPON)
                        {
                            pPlayer->subweapon |= (1<<GROW_WEAPON);
                            weaponNum = GROW_WEAPON;
                        }
                        else
                            pPlayer->subweapon &= ~(1<<GROW_WEAPON);
                    }

                    if (pPlayer->holster_weapon)
                    {
                        playerBits |= BIT(SK_HOLSTER);
                        pPlayer->weapon_pos = WEAPON_POS_LOWER;
                    }
                    else if ((uint32_t)weaponNum < MAX_WEAPONS && (pPlayer->gotweapon & (1<<weaponNum)) && (uint32_t)pPlayer->curr_weapon != weaponNum)
                        switch (weaponNum)
                        {
                        case PISTOL_WEAPON:
                        case SHOTGUN_WEAPON:
                        case CHAINGUN_WEAPON:
                        case RPG_WEAPON:
                        case DEVISTATOR_WEAPON:
                        case FREEZE_WEAPON:
                        case GROW_WEAPON:
                        case SHRINKER_WEAPON:
                            if (pPlayer->ammo_amount[weaponNum] == 0 && pPlayer->show_empty_weapon == 0)
                            {
                                pPlayer->last_full_weapon = pPlayer->curr_weapon;
                                pPlayer->show_empty_weapon = 32;
                            }
                            fallthrough__;
                        case KNEE_WEAPON:
                        case HANDREMOTE_WEAPON:
                            P_AddWeapon(pPlayer, weaponNum, 1);
                            break;
                        case HANDBOMB_WEAPON:
                        case TRIPBOMB_WEAPON:
                            if (pPlayer->ammo_amount[weaponNum] > 0 && (pPlayer->gotweapon & (1<<weaponNum)))
                                P_AddWeapon(pPlayer, weaponNum, 1);
                            break;
                        }
                }
            }
        }

        if (TEST_SYNC_KEY(playerBits, SK_HOLODUKE) && (pPlayer->newowner == -1 || pPlayer->holoduke_on != -1))
        {
            if (pPlayer->holoduke_on == -1)
            {
                if (VM_OnEvent(EVENT_HOLODUKEON,g_player[playerNum].ps->i,playerNum) == 0)
                {
                    if (pPlayer->inv_amount[GET_HOLODUKE] > 0)
                    {
                        pPlayer->inven_icon = ICON_HOLODUKE;

                        if (pPlayer->cursectnum > -1)
                        {
                            int const i = A_InsertSprite(pPlayer->cursectnum, pPlayer->pos.x, pPlayer->pos.y,
                                pPlayer->pos.z+(30<<8), APLAYER, -64, 0, 0, fix16_to_int(pPlayer->q16ang), 0, 0, -1, 10);
                            pPlayer->holoduke_on = i;
                            T4(i) = T5(i) = 0;
                            sprite[i].yvel = playerNum;
                            sprite[i].extra = 0;
                            P_DoQuote(QUOTE_HOLODUKE_ON,pPlayer);
#ifndef EDUKE32_STANDALONE
                            if (!FURY)
                                A_PlaySound(TELEPORTER,pPlayer->holoduke_on);
#endif
                        }
                    }
                    else P_DoQuote(QUOTE_HOLODUKE_NOT_FOUND,pPlayer);
                }
            }
            else
            {
                if (VM_OnEvent(EVENT_HOLODUKEOFF,g_player[playerNum].ps->i,playerNum) == 0)
                {
#ifndef EDUKE32_STANDALONE
                    if (!FURY)
                        A_PlaySound(TELEPORTER,pPlayer->holoduke_on);
#endif
                    pPlayer->holoduke_on = -1;
                    P_DoQuote(QUOTE_HOLODUKE_OFF,pPlayer);
                }
            }
        }

        if (TEST_SYNC_KEY(playerBits, SK_MEDKIT))
        {
            if (VM_OnEvent(EVENT_USEMEDKIT,g_player[playerNum].ps->i,playerNum) == 0)
            {
                if (pPlayer->inv_amount[GET_FIRSTAID] > 0 && sprite[pPlayer->i].extra < pPlayer->max_player_health)
                {
                    int healthDiff = pPlayer->max_player_health-sprite[pPlayer->i].extra;

                    if (pPlayer->inv_amount[GET_FIRSTAID] > healthDiff)
                    {
                        pPlayer->inv_amount[GET_FIRSTAID] -= healthDiff;
                        sprite[pPlayer->i].extra = pPlayer->max_player_health;
                        pPlayer->inven_icon = ICON_FIRSTAID;
                    }
                    else
                    {
                        sprite[pPlayer->i].extra += pPlayer->inv_amount[GET_FIRSTAID];
                        pPlayer->inv_amount[GET_FIRSTAID] = 0;
                        P_SelectNextInvItem(pPlayer);
                    }
#ifndef EDUKE32_STANDALONE
                    if (!FURY)
                        A_PlaySound(DUKE_USEMEDKIT,pPlayer->i);
#endif
                }
            }
        }

        if ((pPlayer->newowner == -1 || pPlayer->jetpack_on) && TEST_SYNC_KEY(playerBits, SK_JETPACK))
        {
            if (VM_OnEvent(EVENT_USEJETPACK,g_player[playerNum].ps->i,playerNum) == 0)
            {
                if (pPlayer->inv_amount[GET_JETPACK] > 0)
                {
                    pPlayer->jetpack_on = !pPlayer->jetpack_on;
                    if (pPlayer->jetpack_on)
                    {
                        pPlayer->inven_icon = ICON_JETPACK;
                        if (pPlayer->scream_voice > FX_Ok)
                        {
                            FX_StopSound(pPlayer->scream_voice);
                            pPlayer->scream_voice = -1;
                        }
#ifndef EDUKE32_STANDALONE
                        if (!FURY)
                            A_PlaySound(DUKE_JETPACK_ON,pPlayer->i);
#endif
                        P_DoQuote(QUOTE_JETPACK_ON,pPlayer);
                    }
                    else
                    {
                        pPlayer->hard_landing = 0;
                        pPlayer->vel.z = 0;
#ifndef EDUKE32_STANDALONE
                        if (!FURY)
                        {
                            A_PlaySound(DUKE_JETPACK_OFF, pPlayer->i);
                            S_StopEnvSound(DUKE_JETPACK_IDLE, pPlayer->i);
                            S_StopEnvSound(DUKE_JETPACK_ON, pPlayer->i);
                        }
#endif
                        P_DoQuote(QUOTE_JETPACK_OFF,pPlayer);
                    }
                }
                else P_DoQuote(QUOTE_JETPACK_NOT_FOUND,pPlayer);
            }
        }

        if (TEST_SYNC_KEY(playerBits, SK_TURNAROUND) && pPlayer->one_eighty_count == 0)
            if (VM_OnEvent(EVENT_TURNAROUND,pPlayer->i,playerNum) == 0)
                pPlayer->one_eighty_count = -1024;
    }
}

int A_CheckHitSprite(int spriteNum, int16_t *hitSprite)
{
    hitdata_t hitData;
    int32_t   zOffset = 0;

    if (A_CheckEnemySprite(&sprite[spriteNum]))
        zOffset = (42 << 8);
    else if (PN(spriteNum) == APLAYER)
        zOffset = (39 << 8);

    SZ(spriteNum) -= zOffset;
    hitscan((const vec3_t *)&sprite[spriteNum], SECT(spriteNum), sintable[(SA(spriteNum) + 512) & 2047],
            sintable[SA(spriteNum) & 2047], 0, &hitData, CLIPMASK1);
    SZ(spriteNum) += zOffset;

    if (hitSprite)
        *hitSprite = hitData.sprite;

    if (hitData.wall >= 0 && (wall[hitData.wall].cstat&16) && A_CheckEnemySprite( &sprite[spriteNum]))
        return 1<<30;

    return FindDistance2D(hitData.pos.x-SX(spriteNum),hitData.pos.y-SY(spriteNum));
}

static int P_FindWall(DukePlayer_t *pPlayer, int *hitWall)
{
    hitdata_t hitData;

    hitscan((const vec3_t *)pPlayer, pPlayer->cursectnum, sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047],
            sintable[fix16_to_int(pPlayer->q16ang) & 2047], 0, &hitData, CLIPMASK0);

    *hitWall = hitData.wall;

    if (hitData.wall < 0)
        return INT32_MAX;

    return FindDistance2D(hitData.pos.x - pPlayer->pos.x, hitData.pos.y - pPlayer->pos.y);
}

// returns 1 if sprite i should not be considered by neartag
static int32_t our_neartag_blacklist(int32_t spriteNum)
{
    return sprite[spriteNum].picnum >= SECTOREFFECTOR__STATIC && sprite[spriteNum].picnum <= GPSPEED__STATIC;
}

static void G_ClearCameras(DukePlayer_t *p)
{
    G_ClearCameraView(p);

    if (I_EscapeTrigger())
        I_EscapeTriggerClear();
}

void P_CheckSectors(int playerNum)
{
    auto const pPlayer = g_player[playerNum].ps;

    if (pPlayer->cursectnum > -1)
    {
        sectortype *const pSector = &sector[pPlayer->cursectnum];
        switch ((uint16_t)pSector->lotag)
        {
            case 32767:
                pSector->lotag = 0;
                P_DoQuote(QUOTE_FOUND_SECRET, pPlayer);
                pPlayer->secret_rooms++;
                return;

            case UINT16_MAX:
                pSector->lotag = 0;
                P_EndLevel();
                return;

            case UINT16_MAX-1:
                pSector->lotag           = 0;
                pPlayer->timebeforeexit  = GAMETICSPERSEC * 8;
                pPlayer->customexitsound = pSector->hitag;
                return;

            default:
                if (pSector->lotag >= 10000 && pSector->lotag < 16383)
                {
                    if (playerNum == screenpeek || (g_gametypeFlags[ud.coop] & GAMETYPE_COOPSOUND))
                        A_PlaySound(pSector->lotag - 10000, pPlayer->i);
                    pSector->lotag = 0;
                }
                break;
        }
    }

    //After this point the the player effects the map with space

    if (pPlayer->gm &MODE_TYPE || sprite[pPlayer->i].extra <= 0)
        return;

    if (TEST_SYNC_KEY(g_player[playerNum].input->bits, SK_OPEN))
    {
        if (VM_OnEvent(EVENT_USE, pPlayer->i, playerNum) != 0)
            g_player[playerNum].input->bits &= ~BIT(SK_OPEN);
    }

    if (ud.cashman && TEST_SYNC_KEY(g_player[playerNum].input->bits, SK_OPEN))
        A_SpawnMultiple(pPlayer->i, MONEY, 2);

    if (pPlayer->newowner >= 0)
    {
        if (klabs(g_player[playerNum].input->svel) > 768 || klabs(g_player[playerNum].input->fvel) > 768)
        {
            G_ClearCameras(pPlayer);
            return;
        }
    }

    if (!TEST_SYNC_KEY(g_player[playerNum].input->bits, SK_OPEN) && !TEST_SYNC_KEY(g_player[playerNum].input->bits, SK_ESCAPE))
        pPlayer->toggle_key_flag = 0;
    else if (!pPlayer->toggle_key_flag)
    {
        int foundWall;

        int16_t nearSector, nearWall, nearSprite;
        int32_t nearDist;

        if (TEST_SYNC_KEY(g_player[playerNum].input->bits, SK_ESCAPE))
        {
            if (pPlayer->newowner >= 0)
                G_ClearCameras(pPlayer);
            return;
        }

        nearSprite = -1;
        pPlayer->toggle_key_flag = 1;
        foundWall = -1;

        int wallDist = P_FindWall(pPlayer, &foundWall);

        if (foundWall >= 0 && wallDist < 1280 && wall[foundWall].overpicnum == MIRROR)
            if (wall[foundWall].lotag > 0 && !A_CheckSoundPlaying(pPlayer->i,wall[foundWall].lotag) && playerNum == screenpeek)
            {
                A_PlaySound(wall[foundWall].lotag,pPlayer->i);
                return;
            }

        if (foundWall >= 0 && (wall[foundWall].cstat&16))
            if (wall[foundWall].lotag)
                return;

        int const intang = fix16_to_int(pPlayer->oq16ang);

        if (pPlayer->newowner >= 0)
            neartag(pPlayer->opos.x, pPlayer->opos.y, pPlayer->opos.z, sprite[pPlayer->i].sectnum, intang, &nearSector,
                &nearWall, &nearSprite, &nearDist, 1280, 1, our_neartag_blacklist);
        else
        {
            neartag(pPlayer->pos.x, pPlayer->pos.y, pPlayer->pos.z, sprite[pPlayer->i].sectnum, intang, &nearSector,
                &nearWall, &nearSprite, &nearDist, 1280, 1, our_neartag_blacklist);
            if (nearSprite == -1 && nearWall == -1 && nearSector == -1)
                neartag(pPlayer->pos.x, pPlayer->pos.y, pPlayer->pos.z+ZOFFSET3, sprite[pPlayer->i].sectnum, intang, &nearSector,
                    &nearWall, &nearSprite, &nearDist, 1280, 1, our_neartag_blacklist);
            if (nearSprite == -1 && nearWall == -1 && nearSector == -1)
                neartag(pPlayer->pos.x, pPlayer->pos.y, pPlayer->pos.z+ZOFFSET2, sprite[pPlayer->i].sectnum, intang, &nearSector,
                    &nearWall, &nearSprite, &nearDist, 1280, 1, our_neartag_blacklist);
            if (nearSprite == -1 && nearWall == -1 && nearSector == -1)
            {
                neartag(pPlayer->pos.x, pPlayer->pos.y, pPlayer->pos.z+ZOFFSET2, sprite[pPlayer->i].sectnum, intang, &nearSector,
                    &nearWall, &nearSprite, &nearDist, 1280, 3, our_neartag_blacklist);
                if (nearSprite >= 0)
                {
                    switch (DYNAMICTILEMAP(sprite[nearSprite].picnum))
                    {
                        case FEM1__STATIC:
                        case FEM2__STATIC:
                        case FEM3__STATIC:
                        case FEM4__STATIC:
                        case FEM5__STATIC:
                        case FEM6__STATIC:
                        case FEM7__STATIC:
                        case FEM8__STATIC:
                        case FEM9__STATIC:
                        case FEM10__STATIC:
                        case PODFEM1__STATIC:
                        case NAKED1__STATIC:
                        case STATUE__STATIC:
                        case TOUGHGAL__STATIC: return;
                    }
                }

                nearSprite = -1;
                nearWall   = -1;
                nearSector = -1;
            }
        }

        if (pPlayer->newowner == -1 && nearSprite == -1 && nearSector == -1 && nearWall == -1)
        {
            if (isanunderoperator(sector[sprite[pPlayer->i].sectnum].lotag))
                nearSector = sprite[pPlayer->i].sectnum;
        }

        if (nearSector >= 0 && (sector[nearSector].lotag&16384))
            return;

        if (nearSprite == -1 && nearWall == -1)
        {
            if (pPlayer->cursectnum >= 0 && sector[pPlayer->cursectnum].lotag == 2)
            {
                if (A_CheckHitSprite(pPlayer->i, &nearSprite) > 1280)
                    nearSprite = -1;
            }
        }

        if (nearSprite >= 0)
        {
            if (P_ActivateSwitch(playerNum, nearSprite, 1))
                return;

            switch (DYNAMICTILEMAP(sprite[nearSprite].picnum))
            {
#ifndef EDUKE32_STANDALONE
            case TOILET__STATIC:
            case STALL__STATIC:
                if (pPlayer->last_pissed_time == 0)
                {
                    if (ud.lockout == 0)
                        A_PlaySound(DUKE_URINATE, pPlayer->i);

                    pPlayer->last_pissed_time = GAMETICSPERSEC * 220;
                    pPlayer->transporter_hold = 29 * 2;

                    if (pPlayer->holster_weapon == 0)
                    {
                        pPlayer->holster_weapon = 1;
                        pPlayer->weapon_pos     = -1;
                    }

                    if (sprite[pPlayer->i].extra <= (pPlayer->max_player_health - (pPlayer->max_player_health / 10)))
                    {
                        sprite[pPlayer->i].extra += pPlayer->max_player_health / 10;
                        pPlayer->last_extra = sprite[pPlayer->i].extra;
                    }
                    else if (sprite[pPlayer->i].extra < pPlayer->max_player_health)
                        sprite[pPlayer->i].extra = pPlayer->max_player_health;
                }
                else if (!A_CheckSoundPlaying(nearSprite,FLUSH_TOILET))
                    A_PlaySound(FLUSH_TOILET,nearSprite);
                return;

            case NUKEBUTTON__STATIC:
            {
                int wallNum;

                P_FindWall(pPlayer, &wallNum);

                if (wallNum >= 0 && wall[wallNum].overpicnum == 0)
                {
                    if (actor[nearSprite].t_data[0] == 0)
                    {
                        if (ud.noexits && (g_netServer || ud.multimode > 1))
                        {
                            // NUKEBUTTON frags the player
                            actor[pPlayer->i].picnum = NUKEBUTTON;
                            actor[pPlayer->i].extra  = 250;
                        }
                        else
                        {
                            actor[nearSprite].t_data[0] = 1;
                            sprite[nearSprite].owner    = pPlayer->i;
                            // assignment of buttonpalette here is not a bug
                            ud.secretlevel =
                            (pPlayer->buttonpalette = sprite[nearSprite].pal) ? sprite[nearSprite].lotag : 0;
                        }
                    }
                }
                return;
            }

            case WATERFOUNTAIN__STATIC:
                if (actor[nearSprite].t_data[0] != 1)
                {
                    actor[nearSprite].t_data[0] = 1;
                    sprite[nearSprite].owner    = pPlayer->i;

                    if (sprite[pPlayer->i].extra < pPlayer->max_player_health)
                    {
                        sprite[pPlayer->i].extra++;
                        A_PlaySound(DUKE_DRINKING,pPlayer->i);
                    }
                }
                return;

            case PLUG__STATIC:
                A_PlaySound(SHORT_CIRCUIT, pPlayer->i);
                sprite[pPlayer->i].extra -= 2+(krand()&3);

                P_PalFrom(pPlayer, 32, 48,48,64);
                break;
#endif

            case VIEWSCREEN__STATIC:
            case VIEWSCREEN2__STATIC:
                // Try to find a camera sprite for the viewscreen.
                for (bssize_t SPRITES_OF(STAT_ACTOR, spriteNum))
                {
                    if (PN(spriteNum) == CAMERA1 && SP(spriteNum) == 0 && sprite[nearSprite].hitag == SLT(spriteNum))
                    {
                        sprite[spriteNum].yvel   = 1;  // Using this camera
                        A_PlaySound(MONITOR_ACTIVE, pPlayer->i);
                        sprite[nearSprite].owner = spriteNum;
                        sprite[nearSprite].yvel  = 1;  // VIEWSCREEN_YVEL
                        g_curViewscreen          = nearSprite;

                        int const playerSectnum = pPlayer->cursectnum;
                        pPlayer->cursectnum     = SECT(spriteNum);
                        P_UpdateScreenPal(pPlayer);
                        pPlayer->cursectnum     = playerSectnum;
                        pPlayer->newowner       = spriteNum;

                        P_UpdatePosWhenViewingCam(pPlayer);

                        return;
                    }
                }

                G_ClearCameras(pPlayer);
                return;
            }  // switch
        }

        if (TEST_SYNC_KEY(g_player[playerNum].input->bits, SK_OPEN) == 0)
            return;

        if (pPlayer->newowner >= 0)
        {
            G_ClearCameras(pPlayer);
            return;
        }

        if (nearWall == -1 && nearSector == -1 && nearSprite == -1)
        {
            if (klabs(A_GetHitscanRange(pPlayer->i)) < 512)
            {
#ifndef EDUKE32_STANDALONE
                if (!FURY)
                    A_PlaySound(((krand()&255) < 16) ? DUKE_SEARCH2 : DUKE_SEARCH, pPlayer->i);
#endif
                return;
            }
        }

        if (nearWall >= 0)
        {
            if (wall[nearWall].lotag > 0 && CheckDoorTile(wall[nearWall].picnum))
            {
                if (foundWall == nearWall || foundWall == -1)
                    P_ActivateSwitch(playerNum,nearWall,0);
                return;
            }
        }

        if (nearSector >= 0 && (sector[nearSector].lotag&16384) == 0 &&
                isanearoperator(sector[nearSector].lotag))
        {
            for (bssize_t SPRITES_OF_SECT(nearSector, spriteNum))
            {
                if (PN(spriteNum) == ACTIVATOR || PN(spriteNum) == MASTERSWITCH)
                    return;
            }

            G_OperateSectors(nearSector,pPlayer->i);
        }
        else if ((sector[sprite[pPlayer->i].sectnum].lotag&16384) == 0)
        {
            if (isanunderoperator(sector[sprite[pPlayer->i].sectnum].lotag))
            {
                for (bssize_t SPRITES_OF_SECT(sprite[pPlayer->i].sectnum, spriteNum))
                {
                    if (PN(spriteNum) == ACTIVATOR || PN(spriteNum) == MASTERSWITCH)
                        return;
                }

                G_OperateSectors(sprite[pPlayer->i].sectnum,pPlayer->i);
            }
            else P_ActivateSwitch(playerNum,nearWall,0);
        }
    }
}

