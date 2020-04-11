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
#include "ns.h"	// Must come before everything else!

#define sector_c_

#include "duke3d.h"

#include "secrets.h"
#include "v_video.h"

BEGIN_RR_NS

// PRIMITIVE

static int g_haltSoundHack = 0;

uint8_t g_shadedSector[MAXSECTORS];

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

void A_CallSound2(int soundNum, int playerNum)
{
    A_PlaySound(soundNum, g_player[playerNum].ps->i);
}

// this function activates a sector's MUSICANDSFX sprite
int A_CallSound(int sectNum, int spriteNum)
{
    if (!RR && g_haltSoundHack)
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

        auto flags = S_GetUserFlags(soundNum);
        if (T1(SFXsprite) == 0)
        {
            if ((flags & (SF_GLOBAL | SF_DTAG)) != SF_GLOBAL)
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

            if ((flags & SF_LOOP) || (SHT(SFXsprite) && SHT(SFXsprite) != soundNum))
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
            spritetype *const pSprite = &sprite[spriteNum];

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
                        case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
                            if (RRRA)
                                break;
                            fallthrough__;
                        case SE_20_STRETCH_BRIDGE:
                        case SE_31_FLOOR_RISE_FALL:
                        case SE_32_CEILING_RISE_FALL:
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
    switch (DYNAMICTILEMAP(tileNum))
    {
        case DOORTILE23__STATIC:
            return !RR;
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
        case RRTILE1856__STATICRR:
        case RRTILE1877__STATICRR:
            return 1;
    }
    return 0;
}

int CheckBlockDoorTile(int tileNum)
{
    if (tileNum == RRTILE3643)
        return 0;
    if (tileNum >= RRTILE3643+2 && tileNum <= RRTILE3643+3)
        tileNum = RRTILE3643;

    switch (DYNAMICTILEMAP(tileNum))
    {
        case RRTILE1996__STATICRR:
        case RRTILE2382__STATICRR:
        case RRTILE2961__STATICRR:
        case RRTILE3804__STATICRR:
        case RRTILE7430__STATICRR:
        case RRTILE7467__STATICRR:
        case RRTILE7469__STATICRR:
        case RRTILE7470__STATICRR:
        case RRTILE7475__STATICRR:
        case RRTILE7566__STATICRR:
        case RRTILE7576__STATICRR:
        case RRTILE7716__STATICRR:
        case RRTILE8063__STATICRR:
        case RRTILE8067__STATICRR:
        case RRTILE8076__STATICRR:
        case RRTILE8106__STATICRR:
        case RRTILE8379__STATICRR:
        case RRTILE8380__STATICRR:
        case RRTILE8565__STATICRR:
        case RRTILE8605__STATICRR:
            return RRRA;
        case RRTILE1792__STATICRR:
        case RRTILE1801__STATICRR:
        case RRTILE1805__STATICRR:
        case RRTILE1807__STATICRR:
        case RRTILE1808__STATICRR:
        case RRTILE1812__STATICRR:
        case RRTILE1821__STATICRR:
        case RRTILE1826__STATICRR:
        case RRTILE1850__STATICRR:
        case RRTILE1851__STATICRR:
        case RRTILE1856__STATICRR:
        case RRTILE1877__STATICRR:
        case RRTILE1938__STATICRR:
        case RRTILE1942__STATICRR:
        case RRTILE1944__STATICRR:
        case RRTILE1945__STATICRR:
        case RRTILE1951__STATICRR:
        case RRTILE1961__STATICRR:
        case RRTILE1964__STATICRR:
        case RRTILE1985__STATICRR:
        case RRTILE1995__STATICRR:
        case RRTILE2022__STATICRR:
        case RRTILE2052__STATICRR:
        case RRTILE2053__STATICRR:
        case RRTILE2060__STATICRR:
        case RRTILE2074__STATICRR:
        case RRTILE2132__STATICRR:
        case RRTILE2136__STATICRR:
        case RRTILE2139__STATICRR:
        case RRTILE2150__STATICRR:
        case RRTILE2178__STATICRR:
        case RRTILE2186__STATICRR:
        case RRTILE2319__STATICRR:
        case RRTILE2321__STATICRR:
        case RRTILE2326__STATICRR:
        case RRTILE2329__STATICRR:
        case RRTILE2578__STATICRR:
        case RRTILE2581__STATICRR:
        case RRTILE2610__STATICRR:
        case RRTILE2613__STATICRR:
        case RRTILE2621__STATICRR:
        case RRTILE2622__STATICRR:
        case RRTILE2676__STATICRR:
        case RRTILE2732__STATICRR:
        case RRTILE2831__STATICRR:
        case RRTILE2832__STATICRR:
        case RRTILE2842__STATICRR:
        case RRTILE2940__STATICRR:
        case RRTILE2970__STATICRR:
        case RRTILE3083__STATICRR:
        case RRTILE3100__STATICRR:
        case RRTILE3155__STATICRR:
        case RRTILE3195__STATICRR:
        case RRTILE3232__STATICRR:
        case RRTILE3600__STATICRR:
        case RRTILE3631__STATICRR:
        case RRTILE3635__STATICRR:
        case RRTILE3637__STATICRR:
        case RRTILE3643__STATICRR:
        case RRTILE3647__STATICRR:
        case RRTILE3652__STATICRR:
        case RRTILE3653__STATICRR:
        case RRTILE3671__STATICRR:
        case RRTILE3673__STATICRR:
        case RRTILE3684__STATICRR:
        case RRTILE3708__STATICRR:
        case RRTILE3714__STATICRR:
        case RRTILE3716__STATICRR:
        case RRTILE3723__STATICRR:
        case RRTILE3725__STATICRR:
        case RRTILE3737__STATICRR:
        case RRTILE3754__STATICRR:
        case RRTILE3762__STATICRR:
        case RRTILE3763__STATICRR:
        case RRTILE3764__STATICRR:
        case RRTILE3765__STATICRR:
        case RRTILE3767__STATICRR:
        case RRTILE3793__STATICRR:
        case RRTILE3814__STATICRR:
        case RRTILE3815__STATICRR:
        case RRTILE3819__STATICRR:
        case RRTILE3827__STATICRR:
        case RRTILE3837__STATICRR:
            return 1;
    }
    return 0;
}

int isanunderoperator(int lotag)
{
    switch (lotag & 0xff)
    {
        case ST_22_SPLITTING_DOOR:
            if (RR) return 0;
            fallthrough__;
        case ST_15_WARP_ELEVATOR:
        case ST_16_PLATFORM_DOWN:
        case ST_17_PLATFORM_UP:
        case ST_18_ELEVATOR_DOWN:
        case ST_19_ELEVATOR_UP:
        case ST_26_SPLITTING_ST_DOOR:
            return 1;
    }
    return 0;
}

int isanearoperator(int lotag)
{
    switch (lotag & 0xff)
    {
        case 41:
            if (!RR) return 0;
            fallthrough__;
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
        DukePlayer_t *const pPlayer = g_player[myconnectindex].ps;

        if (dist)
            *dist = A_FP_ManhattanDist(pPlayer, pSprite);

        return myconnectindex;
    }

    int     closestPlayer     = 0;
    int32_t closestPlayerDist = INT32_MAX;

    for (bssize_t TRAVERSE_CONNECT(j))
    {
        DukePlayer_t *const pPlayer    = g_player[j].ps;
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
    vec3_t const camera     = G_GetCameraPosition(spriteNum, smoothRatio);
    int const    saveMirror = display_mirror;

    renderSetTarget(tileNum, tilesiz[tileNum].y, tilesiz[tileNum].x);
    screen->BeginScene();

    yax_preparedrawrooms();
    drawrooms(camera.x, camera.y, camera.z, SA(spriteNum), 100 + sprite[spriteNum].shade, SECT(spriteNum));
    yax_drawrooms(G_DoSpriteAnimations, SECT(spriteNum), 0, smoothRatio);

    display_mirror = 3;
    G_DoSpriteAnimations(camera.x, camera.y, camera.z, SA(spriteNum), smoothRatio);
    display_mirror = saveMirror;
    renderDrawMasks();
    screen->FinishScene();

    renderRestoreTarget();
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
        DukePlayer_t const *const pPlayer = g_player[screenpeek].ps;

        if (pPlayer->newowner >= 0)
            OW(spriteNum) = pPlayer->newowner;

        if (OW(spriteNum) >= 0 && dist(&sprite[pPlayer->i], &sprite[spriteNum]) < VIEWSCREEN_ACTIVE_DISTANCE)
        {
            int const viewscrTile  = TILE_VIEWSCR;
			TileFiles.MakeCanvas(viewscrTile, tilesiz[PN(spriteNum)].x, tilesiz[PN(spriteNum)].y);

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
    if (RRRA && g_player[screenpeek].ps->sea_sick_stat == 1)
    {
        for (bssize_t i = 0; i < MAXWALLS; i++)
        {
            if (wall[i].picnum == RRTILE7873 || wall[i].picnum == RRTILE7870)
                wall[i].xpanning += 6;
        }
    }
    for (bssize_t animwallNum = 0; animwallNum < g_animWallCnt; animwallNum++)
    {
        int const wallNum = animwall[animwallNum].wallnum;

        switch (DYNAMICTILEMAP(wall[wallNum].picnum))
        {
        case SCREENBREAK14__STATIC:
        case SCREENBREAK15__STATIC:
        case SCREENBREAK16__STATIC:
        case SCREENBREAK17__STATIC:
        case SCREENBREAK18__STATIC:
        case SCREENBREAK19__STATIC:
            if (RR) continue;
            fallthrough__;
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
            if ((krand2()&255) < 16)
            {
                animwall[animwallNum].tag = wall[wallNum].picnum;
                wall[wallNum].picnum = SCREENBREAK6;
            }
            continue;

        case SCREENBREAK6__STATIC:
        case SCREENBREAK7__STATIC:
        case SCREENBREAK8__STATIC:
            if (animwall[animwallNum].tag >= 0 && (RR || (wall[wallNum].extra != FEMPIC2 && wall[wallNum].extra != FEMPIC3)))
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
                    if ((krand2()&255) < 32)
                        animwall[animwallNum].tag = 128<<(krand2()&3);
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
        if ((SLT(i) == SE_17_WARP_ELEVATOR || (RRRA && SLT(i) == 18)) && SHT(i) == sprite[spriteNum].hitag)
        {
            if (klabs(sector[sectNum].floorz - actor[spriteNum].t_data[2]) > SP(i) ||
                    sector[SECT(i)].hitag == sector[sectNum].hitag - warpDir)
                break;
        }

    if (i == -1)
        return 1; // No find

    A_PlaySound(warpDir ? ELEVATOR_ON : ELEVATOR_OFF, spriteNum);

    for (SPRITES_OF(STAT_EFFECTOR, i))
        if ((SLT(i) == SE_17_WARP_ELEVATOR || (RRRA && SLT(i) == 18)) && SHT(i) == sprite[spriteNum].hitag)
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
    case 41:
        if (!RR) break;
        for (bsize_t i = 0; i < g_jailDoorCnt; i++)
        {
            if (g_jailDoorSecHitag[i] == pSector->hitag)
            {
                if (g_jailDoorOpen[i] == 0)
                {
                    g_jailDoorOpen[i] = 1;
                    g_jailDoorDrag[i] = g_jailDoorDist[i];
                    if (!RRRA || g_jailDoorSound[i] != 0)
                        A_CallSound2(g_jailDoorSound[i], screenpeek);
                }
                if (g_jailDoorOpen[i] == 2)
                {
                    g_jailDoorOpen[i] = 3;
                    g_jailDoorDrag[i] = g_jailDoorDist[i];
                    if (!RRRA || g_jailDoorSound[i] != 0)
                        A_CallSound2(g_jailDoorSound[i], screenpeek);
                }
            }
        }
        break;
    case 7:
    {
        if (!RR) break;
        int const startwall = pSector->wallptr;
        int const endwall = startwall + pSector->wallnum;
        for (bssize_t j = startwall; j < endwall; j++)
        {
            SetAnimation(sectNum, &wall[j].x, wall[j].x + 1024, 4);
            SetAnimation(sectNum, &wall[wall[j].nextwall].x, wall[wall[j].nextwall].x + 1024, 4);
        }
        break;
    }
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
            //if (j == -1) j = nextsectorneighborz(sectNum,pSector->ceilingz,1,1);
            if (j == -1)
            {
                Printf("WARNING: ST29: null sector!\n");
                return;
            }
            j = sector[j].ceilingz;
        }
        else
        {
            j = nextsectorneighborz(sectNum,pSector->ceilingz,1,1);
            //if (j == -1) j = nextsectorneighborz(sectNum,pSector->ceilingz,-1,-1);
            if (j == -1)
            {
                Printf("WARNING: ST29: null sector!\n");
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
                Printf("WARNING: ST_22_SPLITTING_DOOR: null sector: floor neighbor=%d, ceiling neighbor=%d!\n",
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
        spritetype * const pSprite = &sprite[spriteNum];

        if (pSprite->lotag == lotag && pSprite->picnum == RESPAWN)
        {
            if (!ud.monsters_off || !A_CheckEnemyTile(pSprite->hitag))
            {
                int const j = A_Spawn(spriteNum, TRANSPORTERSTAR);
                sprite[j].z -= ZOFFSET5;

                // Just a way to killit (see G_MoveFX(): RESPAWN__STATIC)
                pSprite->extra = 66-12;
            }
        }
        if (RRRA && pSprite->lotag == lotag && pSprite->picnum == RRTILE7424)
        {
            if (!ud.monsters_off)
                changespritestat(spriteNum, 119);
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

            for (bsize_t j = sector[pCycler[0]].wallnum; j > 0; j--, pWall++)
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
                                case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
                                    if (RRRA) break;
                                    fallthrough__;
                                case SE_36_PROJ_SHOOTER:
                                case SE_31_FLOOR_RISE_FALL:
                                case SE_32_CEILING_RISE_FALL:
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
            && ((!RR && G_GetForcefieldPicnum(wallNum) == W_FORCEFIELD) || (wall[wallNum].overpicnum == BIGFORCE)))
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
    case SPACELIGHTSWITCH__STATIC:                                                                                          \
    case RRTILE2697__STATICRR:                                                                                              \
    case RRTILE2707__STATICRR


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
    
    if (RRRA && switchPic > MULTISWITCH2 && switchPic <= MULTISWITCH2+3)
        return MULTISWITCH2;
    
    if (RR)
    {
        if (switchPic == NUKEBUTTON+1 || switchPic == RRTILE2697+1 || switchPic == RRTILE2707+1)
            return switchPic-1;
    }

    return ((RR && (switchPic == NUKEBUTTON+1 || switchPic == RRTILE2697+1 || switchPic == RRTILE2707+1)) ||
        switchPic == DIPSWITCH + 1          || switchPic == DIPSWITCH2 + 1   || switchPic == DIPSWITCH3 + 1 ||
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
        if (sprite[wallOrSprite].lotag == 0)
            return 0;

        lotag         = sprite[wallOrSprite].lotag;
        hitag         = sprite[wallOrSprite].hitag;
        davector      = *(vec3_t *)&sprite[wallOrSprite];
        nSwitchPicnum = sprite[wallOrSprite].picnum;
        nSwitchPal    = sprite[wallOrSprite].pal;
    }
    else
    {
        if (wall[wallOrSprite].lotag == 0)
            return 0;

        lotag         = wall[wallOrSprite].lotag;
        hitag         = wall[wallOrSprite].hitag;
        davector      = *(vec3_t *)&wall[wallOrSprite];
        davector.z    = g_player[playerNum].ps->pos.z;
        nSwitchPicnum = wall[wallOrSprite].picnum;
        nSwitchPal    = wall[wallOrSprite].pal;
    }

    //    Printf("P_ActivateSwitch called picnum=%i switchissprite=%i\n",picnum,switchissprite);

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
                    if ((!RR && (g_player[playerNum].ps->got_access & (1 << nKeyPal)))
                        || (RR && g_player[playerNum].ps->keys[nKeyPal+1]))
                        g_player[playerNum].ps->access_incs = 1;
                    else
                    {
                        P_DoQuote(need_key_quote[nKeyPal], g_player[playerNum].ps);
                        if (RRRA)
                        {
                            if (switchType == SWITCH_SPRITE)
                                A_PlaySound(99, wallOrSprite);
                            else
                                A_PlaySound(99, g_player[playerNum].ps->i);
                        }
                    }

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
    case RRTILE2214__STATICRR:
    case RRTILE8464__STATICRR:
    case RRTILE8660__STATICRR:
    case NUKEBUTTON__STATIC:
    case MULTISWITCH__STATIC:
    case MULTISWITCH2__STATICRR:
    case REST_SWITCH_CASES:
        if (!RR && nSwitchPicnum == NUKEBUTTON) goto default_case;
        if (RR && !RRRA && (nSwitchPicnum == MULTISWITCH2 || nSwitchPicnum == RRTILE8464 || nSwitchPicnum == RRTILE8660)) goto default_case;
        if (G_CheckActivatorMotion(lotag))
            return 0;
        break;

    default:
default_case:
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

            if (RRRA && spritePic >= MULTISWITCH2 && spritePic <= MULTISWITCH2+3)
            {
                sprite[spriteNum].picnum++;
                if (sprite[spriteNum].picnum > MULTISWITCH2+3)
                    sprite[spriteNum].picnum = MULTISWITCH2;
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

            case RRTILE2214__STATICRR:
                if (ud.level_number > 6)
                    ud.level_number = 0;
                sprite[spriteNum].picnum++;
                break;

            case ACCESSSWITCH_CASES:
            case REST_SWITCH_CASES:
            case NUKEBUTTON__STATIC:
            case RRTILE8660__STATICRR:
                if (!RR && spritePic == NUKEBUTTON)
                    break;
                if (RR && !RRRA && spritePic == RRTILE8660)
                    break;
                if (RR)
                {
                    if (PN(spriteNum) == DIPSWITCH3 && SHT(spriteNum) == 999)
                    {
                        int j = headspritestat[107];
                        while (j >= 0)
                        {
                            int const nextj = nextspritestat[j];
                            if (sprite[j].picnum == RRTILE3410)
                            {
                                sprite[j].picnum++;
                                sprite[j].hitag = 100;
                                sprite[j].extra = 0;
                                A_PlaySound(474, j);
                            }
                            else if (sprite[j].picnum == RRTILE295)
                                A_DeleteSprite(j);
                            j = nextj;
                        }
                        sprite[spriteNum].picnum++;
                        break;
                    }
                    if (PN(spriteNum) == NUKEBUTTON)
                        g_chickenPlant = 0;
                    if (RRRA && PN(spriteNum) == RRTILE8660)
                    {
                        g_bellTime = 132;
                        g_bellSprite = spriteNum;
                    }
                }
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
                    case NUKEBUTTON__STATIC:
                        if (!RR && spritePic == NUKEBUTTON+1)
                            break;
                        if (RR && PN(spriteNum) == NUKEBUTTON+1)
                            g_chickenPlant = 1;
                        if (!RR || SHT(spriteNum) != 999)
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
            if (RRRA && wall[wallNum].picnum >= MULTISWITCH2 && wall[wallNum].picnum <= MULTISWITCH2+3)
            {
                wall[wallNum].picnum++;
                if (wall[wallNum].picnum > MULTISWITCH2+3)
                    wall[wallNum].picnum = MULTISWITCH2;
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
                case RRTILE8464__STATICRR:
                case RRTILE8660__STATICRR:
                    if (RR && !RRRA && wall[wallNum].picnum == RRTILE8660) break;
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
        case MULTISWITCH2__STATICRR:
        case REST_SWITCH_CASES:
        case RRTILE8464__STATICRR:
        case RRTILE8660__STATICRR:
        {
            if (RR && !RRRA && (basePicnum == MULTISWITCH2 || basePicnum == RRTILE8464 || basePicnum == RRTILE8660)) break;
            if (RRRA && switchType == SWITCH_SPRITE)
            {
                if (nSwitchPicnum == RRTILE8660)
                {
                    g_bellTime = 132;
                    g_bellSprite = wallOrSprite;
                    sprite[wallOrSprite].picnum++;
                }
                else if (nSwitchPicnum == RRTILE8464)
                {
                    sprite[wallOrSprite].picnum = sprite[wallOrSprite].picnum+1;
                    if (hitag == 10001)
                    {
                        if (g_player[playerNum].ps->sea_sick == 0)
                            g_player[playerNum].ps->sea_sick = 350;
                        G_OperateActivators(668, playerNum);
                        G_OperateMasterSwitches(668);
                        A_PlaySound(328,g_player[playerNum].ps->i);
                        return 1;
                    }
                }
                else if (hitag == 10000)
                {
                    if( nSwitchPicnum == MULTISWITCH || nSwitchPicnum == (MULTISWITCH+1) ||
                        nSwitchPicnum == (MULTISWITCH+2) || nSwitchPicnum == (MULTISWITCH+3) ||
                        nSwitchPicnum == MULTISWITCH2 || nSwitchPicnum == (MULTISWITCH2+1) ||
                        nSwitchPicnum == (MULTISWITCH2+2) || nSwitchPicnum == (MULTISWITCH2+3) )
                    {
                        int var6c[3], var54;
                        int jpn, jht;
                        var54 = 0;
                        S_PlaySound3D(SWITCH_ON, wallOrSprite, &davector);
                        for (bssize_t j = 0; j < MAXSPRITES; j++)
                        {
                            jpn = sprite[j].picnum;
                            jht = sprite[j].hitag;
                            if ((jpn == MULTISWITCH || jpn == MULTISWITCH2) && jht == 10000)
                            {
                                if (var54 < 3)
                                {
                                    var6c[var54] = j;
                                    var54++;
                                }
                            }
                        }
                        if (var54 == 3)
                        {
                            S_PlaySound3D(SWITCH_ON, wallOrSprite, &davector);
                            for (bssize_t j = 0; j < var54; j++)
                            {
                                sprite[var6c[j]].hitag = 0;
                                if (nSwitchPicnum >= MULTISWITCH2)
                                    sprite[var6c[j]].picnum = MULTISWITCH2+3;
                                else
                                    sprite[var6c[j]].picnum = MULTISWITCH+3;
                                P_ActivateSwitch(playerNum,var6c[j],1);
                            }
                        }
                        return 1;
                    }
                }
            }
            if (nSwitchPicnum >= MULTISWITCH && nSwitchPicnum <= MULTISWITCH + 3)
                lotag += nSwitchPicnum - MULTISWITCH;

            if (RRRA && nSwitchPicnum >= MULTISWITCH2 && nSwitchPicnum <= MULTISWITCH2 + 3)
                lotag += nSwitchPicnum - MULTISWITCH2;

            for (bssize_t SPRITES_OF(STAT_EFFECTOR, spriteNum))
            {
                if (sprite[spriteNum].hitag == lotag)
                {
                    switch (sprite[spriteNum].lotag)
                    {
                        case 46:
                        case 47:
                        case 48:
                            if (!RRRA) break;
                            fallthrough__;
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
                auto flags = S_GetUserFlags(hitag);

                if (switchType == SWITCH_SPRITE && (flags & SF_TALK) == 0)
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

    if ((!RR && !activatedSectors) || (RR && sector[sectNum].lotag != SE_22_TEETH_DOOR))
        G_OperateSectors(sectNum, spriteNum);
}

static void G_BreakWall(int tileNum, int spriteNum, int wallNum)
{
    wall[wallNum].picnum = tileNum;
    A_PlaySound(VENT_BUST,spriteNum);
    A_PlaySound(GLASS_HEAVYBREAK,spriteNum);
    A_SpawnWallGlass(spriteNum,wallNum,10);
}

void A_DamageWall(int spriteNum, int wallNum, const vec3_t *vPos, int weaponNum)
{
    int16_t sectNum = -1;
    walltype *pWall = &wall[wallNum];

    if (pWall->overpicnum == MIRROR)
    {
        switch (DYNAMICTILEMAP(weaponNum))
        {
            case RPG2__STATICRR:
                if (!RRRA) break;
                fallthrough__;
            case RADIUSEXPLOSION__STATIC:
            case SEENINE__STATIC:
            case HEAVYHBOMB__STATIC:
            case RPG__STATIC:
            case HYDRENT__STATIC:
            case OOZFILTER__STATIC:
            case EXPLODINGBARREL__STATIC:
                A_SpawnWallGlass(spriteNum, wallNum, 70);
                A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
                pWall->cstat &= ~16;
                pWall->overpicnum = MIRRORBROKE;
                return;
                
        }
    }

    if ((((pWall->cstat & 16) || pWall->overpicnum == BIGFORCE) && pWall->nextsector >= 0) &&
        (sector[pWall->nextsector].floorz > vPos->z) &&
        (sector[pWall->nextsector].floorz != sector[pWall->nextsector].ceilingz))
    {
        int const switchPic = G_GetForcefieldPicnum(wallNum);

        switch (DYNAMICTILEMAP(switchPic))
        {
            case W_FORCEFIELD__STATIC:
                pWall->extra = 1;  // tell the forces to animate
                fallthrough__;
            case BIGFORCE__STATIC:
            {
                if (RR) break;
                updatesector(vPos->x, vPos->y, &sectNum);
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

                int const i = A_InsertSprite(sectNum, vPos->x, vPos->y, vPos->z, FORCERIPPLE, -127, xRepeat, yRepeat, 0,
                                   0, 0, spriteNum, 5);

                CS(i) |= 18 + 128;
                SA(i) = getangle(pWall->x - wall[pWall->point2].x, pWall->y - wall[pWall->point2].y) - 512;

                A_PlaySound(SOMETHINGHITFORCE, i);
            }
                return;
                
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

            case RRTILE1973__STATICRR:
                updatesector(vPos->x, vPos->y, &sectNum);
                if (sectNum < 0)
                    return;
                pWall->overpicnum = GLASS2;
                A_SpawnWallPopcorn(spriteNum, wallNum, 64);
                pWall->cstat = 0;

                if (pWall->nextwall >= 0)
                    wall[pWall->nextwall].cstat = 0;

                {
                    int const i = A_InsertSprite(sectNum, vPos->x, vPos->y, vPos->z, SECTOREFFECTOR, 0, 0, 0,
                        fix16_to_int(g_player[0].ps->q16ang), 0, 0, spriteNum, 3);
                    SLT(i) = 128;
                    T2(i)  = 2;
                    T3(i)  = wallNum;
                    A_PlaySound(GLASS_BREAKING, i);
                }
                return;

            case GLASS__STATIC:
                updatesector(vPos->x, vPos->y, &sectNum);
                if (sectNum < 0)
                    return;
                pWall->overpicnum = GLASS2;
                A_SpawnWallGlass(spriteNum, wallNum, 10);
                pWall->cstat = 0;

                if (pWall->nextwall >= 0)
                    wall[pWall->nextwall].cstat = 0;

                {
                    int const i = A_InsertSprite(sectNum, vPos->x, vPos->y, vPos->z, SECTOREFFECTOR, 0, 0, 0,
                        fix16_to_int(g_player[0].ps->q16ang), 0, 0, spriteNum, 3);
                    SLT(i) = 128;
                    T2(i)  = RR ? 2 : 5;
                    T3(i)  = wallNum;
                    A_PlaySound(GLASS_BREAKING, i);
                }
                return;

            case STAINGLASS1__STATIC:
                updatesector(vPos->x, vPos->y, &sectNum);
                if (sectNum < 0)
                    return;
                A_SpawnRandomGlass(spriteNum, wallNum, 80);
                pWall->cstat = 0;
                if (pWall->nextwall >= 0)
                    wall[pWall->nextwall].cstat = 0;
                A_PlaySound(VENT_BUST, spriteNum);
                A_PlaySound(GLASS_BREAKING, spriteNum);
                return;
        }
    }

    int wallPicnum = pWall->picnum;

    if (RR && wallPicnum >= RRTILE3643 && wallPicnum <= RRTILE3643+3)
        wallPicnum = RRTILE3643;

    switch (DYNAMICTILEMAP(wallPicnum))
    {
        case RRTILE3643__STATICRR:
            {
                int jj = headspritesect[wall[pWall->nextwall].nextsector];
                while (jj != -1)
                {
                    int const nextjj = nextspritesect[jj];
                    spritetype *pSprite = &sprite[jj];
                    if (pSprite->lotag == 6)
                    {
                        for (bssize_t j = 0; j < 16; j++) RANDOMSCRAP(pSprite,jj);
                        g_spriteExtra[jj]++;
                        if (g_spriteExtra[jj] == 25)
                        {
                            int const startwall = sector[pSprite->sectnum].wallptr;
                            int const endwall = startwall+sector[pSprite->sectnum].wallnum;
                            for(bssize_t i=startwall;i<endwall;i++)
                                sector[wall[i].nextsector].lotag = 0;
                            sector[pSprite->sectnum].lotag = 0;
                            S_StopSound(sprite[jj].lotag);
                            A_PlaySound(400,jj);
                            A_DeleteSprite(jj);
                        }
                    }
                    jj = nextjj;
                }
                return;
            }
        case RRTILE7555__STATICRR:
            if (!RRRA) break;
            pWall->picnum = SBMOVE;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE7441__STATICRR:
            if (!RRRA) break;
            pWall->picnum = RRTILE5016;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE7559__STATICRR:
            if (!RRRA) break;
            pWall->picnum = RRTILE5017;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE7433__STATICRR:
            if (!RRRA) break;
            pWall->picnum = RRTILE5018;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE7557__STATICRR:
            if (!RRRA) break;
            pWall->picnum = RRTILE5019;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE7553__STATICRR:
            if (!RRRA) break;
            pWall->picnum = RRTILE5020;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE7552__STATICRR:
            if (!RRRA) break;
            pWall->picnum = RRTILE5021;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE7568__STATICRR:
            if (!RRRA) break;
            pWall->picnum = RRTILE5022;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE7540__STATICRR:
            if (!RRRA) break;
            pWall->picnum = RRTILE5023;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE7558__STATICRR:
            if (!RRRA) break;
            pWall->picnum = RRTILE5024;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE7554__STATICRR:
            if (!RRRA) break;
            pWall->picnum = RRTILE5025;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE7579__STATICRR:
            if (!RRRA) break;
            pWall->picnum = RRTILE5026;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE7561__STATICRR:
            if (!RRRA) break;
            pWall->picnum = RRTILE5027;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE7580__STATICRR:
            if (!RRRA) break;
            pWall->picnum = RRTILE5037;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE8227__STATICRR:
            if (!RRRA) break;
            pWall->picnum = RRTILE5070;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE8503__STATICRR:
            if (!RRRA) break;
            pWall->picnum = RRTILE5079;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE8567__STATICRR:
        case RRTILE8568__STATICRR:
        case RRTILE8569__STATICRR:
        case RRTILE8570__STATICRR:
        case RRTILE8571__STATICRR:
            pWall->picnum = RRTILE5082;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE7859__STATICRR:
            pWall->picnum = RRTILE5081;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE8496__STATICRR:
            pWall->picnum = RRTILE5061;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE8617__STATICRR:
            if (numplayers < 2)
            {
                pWall->picnum = RRTILE8618;
                A_PlaySound(47, spriteNum);
            }
            return;
        case RRTILE8620__STATICRR:
            pWall->picnum = RRTILE8621;
            A_PlaySound(47, spriteNum);
            return;
        case RRTILE8622__STATICRR:
            pWall->picnum = RRTILE8623;
            A_PlaySound(495, spriteNum);
            return;
        case RRTILE7657__STATICRR:
            pWall->picnum = RRTILE7659;
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            return;
        case RRTILE8497__STATICRR:
            pWall->picnum = RRTILE5076;
            A_PlaySound(495, spriteNum);
            return;
        case RRTILE7533__STATICRR:
            pWall->picnum = RRTILE5035;
            A_PlaySound(495, spriteNum);
            return;
        case COLAMACHINE__STATIC:
        case VENDMACHINE__STATIC:
            G_BreakWall(pWall->picnum + 2, spriteNum, wallNum);
            A_PlaySound(RR ? GLASS_BREAKING : VENT_BUST, spriteNum);
            return;

        case FEMPIC2__STATIC:
        case FEMPIC3__STATIC:

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
            if (RR) break;
            fallthrough__;
        case OJ__STATIC:

        case SCREENBREAK6__STATIC:
        case SCREENBREAK7__STATIC:
        case SCREENBREAK8__STATIC:
            A_SpawnWallGlass(spriteNum, wallNum, 30);
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            pWall->picnum = W_SCREENBREAK + (krand2() % (RRRA ? 2: 3));
            return;

        case W_TECHWALL5__STATIC:
        case W_TECHWALL6__STATIC:
        case W_TECHWALL7__STATIC:
        case W_TECHWALL8__STATIC:
        case W_TECHWALL9__STATIC:
            if (RR) break;
            G_BreakWall(pWall->picnum + 1, spriteNum, wallNum);
            return;

        case W_MILKSHELF__STATIC:
            if (RR) break;
            G_BreakWall(W_MILKSHELFBROKE, spriteNum, wallNum);
            return;

        case W_TECHWALL10__STATIC:
            if (RR) break;
            G_BreakWall(W_HITTECHWALL10, spriteNum, wallNum);
            return;

        case W_TECHWALL1__STATIC:
        case W_TECHWALL11__STATIC:
        case W_TECHWALL12__STATIC:
        case W_TECHWALL13__STATIC:
        case W_TECHWALL14__STATIC:
            if (RR) break;
            G_BreakWall(W_HITTECHWALL1, spriteNum, wallNum);
            return;

        case W_TECHWALL15__STATIC:
            if (RR) break;
            G_BreakWall(W_HITTECHWALL15, spriteNum, wallNum);
            return;

        case W_TECHWALL16__STATIC:
            if (RR) break;
            G_BreakWall(W_HITTECHWALL16, spriteNum, wallNum);
            return;

        case W_TECHWALL2__STATIC:
            if (RR) break;
            G_BreakWall(W_HITTECHWALL2, spriteNum, wallNum);
            return;

        case W_TECHWALL3__STATIC:
            if (RR) break;
            G_BreakWall(W_HITTECHWALL3, spriteNum, wallNum);
            return;

        case W_TECHWALL4__STATIC:
            if (RR) break;
            G_BreakWall(W_HITTECHWALL4, spriteNum, wallNum);
            return;

        case ATM__STATIC:
            pWall->picnum = ATMBROKE;
            A_SpawnMultiple(spriteNum, MONEY, 1 + (krand2() & 7));
            A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
            break;

        case WALLLIGHT2__STATIC:
            if (RR) break;
            fallthrough__;
        case WALLLIGHT1__STATIC:
        case WALLLIGHT3__STATIC:
        case WALLLIGHT4__STATIC:
        case TECHLIGHT2__STATIC:
        case TECHLIGHT4__STATIC:
        case RRTILE1814__STATICRR:
        case RRTILE1939__STATICRR:
        case RRTILE1986__STATICRR:
        case RRTILE1988__STATICRR:
        case RRTILE2123__STATICRR:
        case RRTILE2125__STATICRR:
        case RRTILE2636__STATICRR:
        case RRTILE2878__STATICRR:
        case RRTILE2898__STATICRR:
        case RRTILE3200__STATICRR:
        case RRTILE3202__STATICRR:
        case RRTILE3204__STATICRR:
        case RRTILE3206__STATICRR:
        case RRTILE3208__STATICRR:
        {
            A_PlaySound(rnd(128) ? GLASS_HEAVYBREAK : GLASS_BREAKING, spriteNum);
            A_SpawnWallGlass(spriteNum, wallNum, 30);

            if (RR)
            {
                if (pWall->picnum == RRTILE1814)
                    pWall->picnum = RRTILE1817;

                if (pWall->picnum == RRTILE1986)
                    pWall->picnum = RRTILE1987;

                if (pWall->picnum == RRTILE1939)
                    pWall->picnum = RRTILE2004;

                if (pWall->picnum == RRTILE1988)
                    pWall->picnum = RRTILE2005;

                if (pWall->picnum == RRTILE2898)
                    pWall->picnum = RRTILE2899;

                if (pWall->picnum == RRTILE2878)
                    pWall->picnum = RRTILE2879;

                if (pWall->picnum == RRTILE2123)
                    pWall->picnum = RRTILE2124;

                if (pWall->picnum == RRTILE2125)
                    pWall->picnum = RRTILE2126;

                if (pWall->picnum == RRTILE3200)
                    pWall->picnum = RRTILE3201;

                if (pWall->picnum == RRTILE3202)
                    pWall->picnum = RRTILE3203;

                if (pWall->picnum == RRTILE3204)
                    pWall->picnum = RRTILE3205;

                if (pWall->picnum == RRTILE3206)
                    pWall->picnum = RRTILE3207;

                if (pWall->picnum == RRTILE3208)
                    pWall->picnum = RRTILE3209;

                if (pWall->picnum == RRTILE2636)
                    pWall->picnum = RRTILE2637;
            }

            if (pWall->picnum == WALLLIGHT1)
                pWall->picnum = WALLLIGHTBUST1;

            if (!RR && pWall->picnum == WALLLIGHT2)
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

            int const random = krand2() & 1;

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

void Sect_DamageCeiling(int const sectNum)
{
    int16_t * const pPicnum = &sector[sectNum].ceilingpicnum;

    switch (DYNAMICTILEMAP(*pPicnum))
    {
        case RRTILE1939__STATICRR: *pPicnum = RRTILE2004; goto GLASSBREAK_CODE;
        case RRTILE1986__STATICRR: *pPicnum = RRTILE1987; goto GLASSBREAK_CODE;
        case RRTILE1988__STATICRR: *pPicnum = RRTILE2005; goto GLASSBREAK_CODE;
        case RRTILE2123__STATICRR: *pPicnum = RRTILE2124; goto GLASSBREAK_CODE;
        case RRTILE2125__STATICRR: *pPicnum = RRTILE2126; goto GLASSBREAK_CODE;
        case RRTILE2878__STATICRR: *pPicnum = RRTILE2879; goto GLASSBREAK_CODE;
        case RRTILE2898__STATICRR: *pPicnum = RRTILE2899; goto GLASSBREAK_CODE;
        case WALLLIGHT1__STATIC: *pPicnum = WALLLIGHTBUST1; goto GLASSBREAK_CODE;
        case WALLLIGHT2__STATIC: if (RR) break; *pPicnum = WALLLIGHTBUST2; goto GLASSBREAK_CODE;
        case WALLLIGHT3__STATIC: *pPicnum = WALLLIGHTBUST3; goto GLASSBREAK_CODE;
        case WALLLIGHT4__STATIC: *pPicnum = WALLLIGHTBUST4; goto GLASSBREAK_CODE;
        case TECHLIGHT2__STATIC: *pPicnum = TECHLIGHTBUST2; goto GLASSBREAK_CODE;
        case TECHLIGHT4__STATIC: *pPicnum = TECHLIGHTBUST4;
    GLASSBREAK_CODE:
            A_SpawnCeilingGlass(g_player[myconnectindex].ps->i, sectNum, 10);
            A_PlaySound(GLASS_BREAKING, g_player[screenpeek].ps->i);
            if (sector[sectNum].hitag == 0)
            {
                for (bssize_t SPRITES_OF_SECT(sectNum, i))
                {
                    if (PN(i) == SECTOREFFECTOR && (SLT(i) == SE_12_LIGHT_SWITCH || (RRRA && (SLT(i) == 47 || SLT(i) == 48))))
                    {
                        for (bssize_t SPRITES_OF(STAT_EFFECTOR, j))
                            if (sprite[j].hitag == SHT(i))
                                actor[j].t_data[3] = 1;
                        break;
                    }
                }
            }

            int j = krand2() & 1;

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

// hard coded props... :(
void A_DamageObject(int spriteNum, int const dmgSrc)
{
    //if (g_netClient)
    //    return;

    spriteNum &= (MAXSPRITES-1);

    int spritePicnum = PN(spriteNum);
    if (RR)
    {
        if (spritePicnum == HENSTAND+1)
            spritePicnum = HENSTAND;
        if (spritePicnum == RRTILE3440+1)
            spritePicnum = RRTILE3440;
    }

    if (spritePicnum > WATERFOUNTAIN && spritePicnum <= WATERFOUNTAIN+3)
        spritePicnum = WATERFOUNTAIN;

    switch (DYNAMICTILEMAP(spritePicnum))
    {
    case RRTILE8487__STATICRR:
    case RRTILE8489__STATICRR:
        if (!RRRA) goto default_case;
        A_PlaySound(471, spriteNum);
        PN(spriteNum)++;
        break;
    case RRTILE7638__STATICRR:
    case RRTILE7644__STATICRR:
    case RRTILE7646__STATICRR:
    case RRTILE7650__STATICRR:
    case RRTILE7653__STATICRR:
    case RRTILE7655__STATICRR:
    case RRTILE7691__STATICRR:
    case RRTILE7876__STATICRR:
    case RRTILE7881__STATICRR:
    case RRTILE7883__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum)++;
        A_PlaySound(VENT_BUST, spriteNum);
        break;
    case RRTILE7879__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum)++;
        A_PlaySound(495, spriteNum);
        A_RadiusDamage(spriteNum, 10, 0, 0, 1, 1);
        break;
    case RRTILE7648__STATICRR:
    case RRTILE7694__STATICRR:
    case RRTILE7700__STATICRR:
    case RRTILE7702__STATICRR:
    case RRTILE7711__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum)++;
        A_PlaySound(47, spriteNum);
        break;
    case RRTILE7636__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) += 3;
        A_PlaySound(VENT_BUST, spriteNum);
        break;
    case RRTILE7875__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) += 3;
        A_PlaySound(VENT_BUST, spriteNum);
        break;
    case RRTILE7640__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) += 2;
        A_PlaySound(VENT_BUST, spriteNum);
        break;
    case RRTILE7595__STATICRR:
    case RRTILE7704__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE7705;
        A_PlaySound(495, spriteNum);
        break;
    case RRTILE8579__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5014;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE7441__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5016;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE7534__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5029;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE7545__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5030;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE7547__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5031;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE7574__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5032;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE7575__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5033;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE7578__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5034;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE7478__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5035;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8525__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5036;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8537__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5062;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8215__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5064;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8216__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5065;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8217__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5066;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8218__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5067;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8220__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5068;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8221__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5069;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8312__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5071;
        A_PlaySound(472, spriteNum);
        break;
    case RRTILE8395__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5072;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8423__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5073;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE3462__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5074;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case UWHIP__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5075;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8608__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5083;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8609__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5084;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8567__STATICRR:
    case RRTILE8568__STATICRR:
    case RRTILE8569__STATICRR:
    case RRTILE8570__STATICRR:
    case RRTILE8571__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5082;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8640__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5085;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8611__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5086;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case TECHLIGHTBUST2__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = TECHLIGHTBUST4;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8497__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5076;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8162__STATICRR:
    case RRTILE8163__STATICRR:
    case RRTILE8164__STATICRR:
    case RRTILE8165__STATICRR:
    case RRTILE8166__STATICRR:
    case RRTILE8167__STATICRR:
    case RRTILE8168__STATICRR:
        if (!RRRA) goto default_case;
        changespritestat(spriteNum, 5);
        PN(spriteNum) = RRTILE5063;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8589__STATICRR:
    case RRTILE8590__STATICRR:
    case RRTILE8591__STATICRR:
    case RRTILE8592__STATICRR:
    case RRTILE8593__STATICRR:
    case RRTILE8594__STATICRR:
    case RRTILE8595__STATICRR:
        if (!RRRA) goto default_case;
        changespritestat(spriteNum, 5);
        PN(spriteNum) = RRTILE8588;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE3497__STATICRR:
        PN(spriteNum) = RRTILE5076;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE3498__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5077;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE3499__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5078;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8503__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5079;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE7901__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5080;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE7696__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE7697;
        A_PlaySound(47, spriteNum);
        break;
    case RRTILE7806__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5043;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE7885__STATICRR:
    case RRTILE7890__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5045;
        A_PlaySound(495, spriteNum);
        A_RadiusDamage(spriteNum, 10, 0, 0, 1, 1);
        break;
    case RRTILE7886__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5046;
        A_PlaySound(495, spriteNum);
        A_RadiusDamage(spriteNum, 10, 0, 0, 1, 1);
        break;
    case RRTILE7887__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5044;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        A_RadiusDamage(spriteNum, 10, 0, 0, 1, 1);
        break;
    case RRTILE7900__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5047;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE7906__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5048;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE7912__STATICRR:
    case RRTILE7913__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5049;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8047__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5050;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8596__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE8598;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8059__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5051;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8060__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5052;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8222__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5053;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8223__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5054;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8224__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5055;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8370__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5056;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8371__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5057;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8372__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5058;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8373__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5059;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8396__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5038;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8397__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5039;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8398__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5040;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8399__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5041;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8385__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE8386;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8387__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE8388;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8389__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE8390;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8391__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE8392;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE7553__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5035;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8475__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5075;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8498__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5077;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8499__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5078;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE2445__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE2450;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE2123__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE2124;
        A_PlaySound(GLASS_BREAKING, spriteNum);
        A_SpawnWallGlass(spriteNum, -1, 10);
        break;
    case RRTILE3773__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE8651;
        A_PlaySound(GLASS_BREAKING, spriteNum);
        A_SpawnWallGlass(spriteNum, -1, 10);
        break;
    case RRTILE7533__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5035;
        A_PlaySound(495, spriteNum);
        A_RadiusDamage(spriteNum, 10, 0, 0, 1, 1);
        break;
    case RRTILE8394__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5072;
        A_PlaySound(495, spriteNum);
        break;
    case RRTILE8461__STATICRR:
    case RRTILE8462__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE5074;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8679__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE8680;
        A_PlaySound(47, spriteNum);
        A_RadiusDamage(spriteNum, 10, 0, 0, 1, 1);
        if (SLT(spriteNum) != 0)
        {
            for (bssize_t j = 0; j < MAXSPRITES; j++)
            {
                if (sprite[j].picnum == RRTILE8679 && sprite[j].pal == 4)
                {
                    if (sprite[j].lotag == SLT(spriteNum))
                        sprite[j].picnum = RRTILE8680;
                }
            }
        }
        break;
    case RRTILE3584__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE8681;
        A_PlaySound(495, spriteNum);
        A_RadiusDamage(spriteNum, 250, 0, 0, 1, 1);
        break;
    case RRTILE8682__STATICRR:
        if (!RRRA) goto default_case;
        PN(spriteNum) = RRTILE8683;
        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);
        break;
    case RRTILE8099__STATICRR:
        if (!RRRA) goto default_case;
        if (SLT(spriteNum) == 5)
        {
            SLT(spriteNum) = 0;
            PN(spriteNum) = RRTILE5087;
            A_PlaySound(340, spriteNum);
            for (bssize_t j = 0; j < MAXSPRITES; j++)
            {
                if (sprite[j].picnum == RRTILE8094)
                    sprite[j].picnum = RRTILE5088;
            }
        }
        break;
    case RRTILE2431__STATICRR:
        if (!RRRA) goto default_case;
        if (sprite[spriteNum].pal != 4)
        {
            PN(spriteNum) = RRTILE2451;
            if (SLT(spriteNum) != 0)
            {
                for (bssize_t j = 0; j < MAXSPRITES; j++)
                {
                    if (sprite[j].picnum == RRTILE2431 && sprite[j].pal == 4)
                    {
                        if (SLT(spriteNum) == sprite[j].lotag)
                            sprite[j].picnum = RRTILE2451;
                    }
                }
            }
        }
        break;
    case RRTILE2443__STATICRR:
        if (!RRRA) goto default_case;
        if (sprite[spriteNum].pal != 19)
            PN(spriteNum) = RRTILE2455;
        break;
    case RRTILE2455__STATICRR:
        if (!RRRA) goto default_case;
        A_PlaySound(SQUISHED, spriteNum);
        A_DoGuts(spriteNum, RRTILE2465, 3);
        deletesprite(spriteNum);
        break;
    case RRTILE2451__STATICRR:
        if (!RRRA) goto default_case;
        if (sprite[spriteNum].pal != 4)
        {
            A_PlaySound(SQUISHED, spriteNum);
            if (SLT(spriteNum) != 0)
            {
                for (bssize_t j = 0; j < MAXSPRITES; j++)
                {
                    if (sprite[j].picnum == RRTILE2451 && sprite[j].pal == 4)
                    {
                        if (SLT(spriteNum) == sprite[j].lotag)
                        {
                            A_DoGuts(spriteNum, RRTILE2460, 12);
                            A_DoGuts(spriteNum, RRTILE2465, 3);
                            sprite[j].xrepeat = 0;
                            sprite[j].yrepeat = 0;
                            sprite[spriteNum].xrepeat = 0;
                            sprite[spriteNum].yrepeat = 0;
                        }
                    }
                }
            }
            else
            {
                A_DoGuts(spriteNum, RRTILE2460, 12);
                A_DoGuts(spriteNum, RRTILE2465, 3);
                sprite[spriteNum].xrepeat = 0;
                sprite[spriteNum].yrepeat = 0;
            }
        }
        break;
    case RRTILE2437__STATICRR:
        if (!RRRA) goto default_case;
        A_PlaySound(439, spriteNum);
        break;
    case RRTILE3114__STATICRR:
        PN(spriteNum) = RRTILE3117;
        break;
    case RRTILE2876__STATICRR:
        PN(spriteNum) = RRTILE2990;
        break;
    case RRTILE3152__STATICRR:
        PN(spriteNum) = RRTILE3218;
        break;
    case RRTILE3153__STATICRR:
        PN(spriteNum) = RRTILE3219;
        break;
    case RRTILE2030__STATICRR:
        PN(spriteNum) = RRTILE2034;
        A_PlaySound(GLASS_BREAKING, spriteNum);
        A_SpawnWallGlass(spriteNum, -1, 10);
        break;
    case RRTILE2893__STATICRR:
    case RRTILE2915__STATICRR:
    case RRTILE3115__STATICRR:
    case RRTILE3171__STATICRR:
        switch (DYNAMICTILEMAP(PN(spriteNum)))
        {
        case RRTILE2915__STATICRR:
            PN(spriteNum) = RRTILE2977;
            break;
        case RRTILE2893__STATICRR:
            PN(spriteNum) = RRTILE2978;
            break;
        case RRTILE3115__STATICRR:
            PN(spriteNum) = RRTILE3116;
            break;
        case RRTILE3171__STATICRR:
            PN(spriteNum) = RRTILE3216;
            break;
        }
        A_PlaySound(GLASS_BREAKING, spriteNum);
        A_SpawnWallGlass(spriteNum, -1, 10);
        break;
    case RRTILE2156__STATICRR:
    case RRTILE2158__STATICRR:
    case RRTILE2160__STATICRR:
    case RRTILE2175__STATICRR:
        PN(spriteNum)++;
        A_PlaySound(GLASS_BREAKING, spriteNum);
        A_SpawnWallGlass(spriteNum, -1, 10);
        break;
    case RRTILE2137__STATICRR:
    case RRTILE2151__STATICRR:
    case RRTILE2152__STATICRR:
        A_PlaySound(GLASS_BREAKING, spriteNum);
        A_SpawnWallGlass(spriteNum, -1, 10);
        PN(spriteNum)++;
        for (int k = 0; k < 6; k++)
        {
            int32_t const r1 = krand2(), r2 = krand2(), r3 = krand2(), r4 = krand2();
            A_InsertSprite(SECT(spriteNum), SX(spriteNum), SY(spriteNum), SZ(spriteNum) - ZOFFSET3, SCRAP6 + (r4 & 15), -8, 48, 48, r3 & 2047, (r2 & 63) + 64, -(r1 & 4095) - (sprite[spriteNum].zvel >> 2), spriteNum, STAT_MISC);
        }
        break;
    case BOWLINGBALL__STATICRR:
        sprite[dmgSrc].xvel = (sprite[spriteNum].xvel >> 1) + (sprite[spriteNum].xvel >> 2);
        sprite[dmgSrc].ang -= (krand2() & 16);
        A_PlaySound(355, spriteNum);
        break;
    case OCEANSPRITE1__STATIC:
    case OCEANSPRITE2__STATIC:
    case OCEANSPRITE3__STATIC:
    case OCEANSPRITE4__STATIC:
    case OCEANSPRITE5__STATIC:
        if (RR) goto default_case;
        A_Spawn(spriteNum,SMALLSMOKE);
        A_DeleteSprite(spriteNum);
        break;

    case QUEBALL__STATIC:
    case STRIPEBALL__STATIC:
    case RRTILE3440__STATICRR:
    case HENSTAND__STATICRR:
        if (sprite[dmgSrc].picnum == QUEBALL || sprite[dmgSrc].picnum == STRIPEBALL)
        {
            sprite[dmgSrc].xvel = (sprite[spriteNum].xvel>>1)+(sprite[spriteNum].xvel>>2);
            sprite[dmgSrc].ang -= (SA(spriteNum)<<1)+1024;
            SA(spriteNum) = getangle(SX(spriteNum)-sprite[dmgSrc].x,SY(spriteNum)-sprite[dmgSrc].y)-512;
            if (S_CheckSoundPlaying(POOLBALLHIT) < 2)
                A_PlaySound(POOLBALLHIT, spriteNum);
        }
        else if (RR && (sprite[dmgSrc].picnum == RRTILE3440 || sprite[dmgSrc].picnum == RRTILE3440+1))
        {
            sprite[dmgSrc].xvel = (sprite[spriteNum].xvel>>1)+(sprite[spriteNum].xvel>>2);
            sprite[dmgSrc].ang -= ((SA(spriteNum)<<1)+krand2())&64;
            SA(spriteNum) = (SA(spriteNum)+krand2())&16;
            A_PlaySound(355,spriteNum);
        }
        else if (RR && (sprite[dmgSrc].picnum == HENSTAND || sprite[dmgSrc].picnum == HENSTAND+1))
        {
            sprite[dmgSrc].xvel = (sprite[spriteNum].xvel>>1)+(sprite[spriteNum].xvel>>2);
            sprite[dmgSrc].ang -= ((SA(spriteNum)<<1)+krand2())&16;
            SA(spriteNum) = (SA(spriteNum)+krand2())&16;
            A_PlaySound(355,spriteNum);
        }
        else
        {
            if (krand2()&3)
            {
                sprite[spriteNum].xvel = 164;
                sprite[spriteNum].ang = sprite[dmgSrc].ang;
            }
            else if (!RR)
            {
                A_SpawnWallGlass(spriteNum,-1,3);
                A_DeleteSprite(spriteNum);
            }
        }
        break;

    case CONE__STATIC:
        if (RR) goto default_case;
        fallthrough__;
    case TREE1__STATIC:
    case TREE2__STATIC:
    case TIRE__STATIC:
    case BOX__STATIC:
    {
        switch (DYNAMICTILEMAP(sprite[dmgSrc].picnum))
        {
        case RPG2__STATICRR:
            if (!RRRA) break;
            fallthrough__;
        case OWHIP__STATICRR:
        case UWHIP__STATICRR:
        case TRIPBOMBSPRITE__STATIC:
        case COOLEXPLOSION1__STATIC:
            if (!RR) break;
            fallthrough__;
        case RADIUSEXPLOSION__STATIC:
        case RPG__STATIC:
        case FIRELASER__STATIC:
        case HYDRENT__STATIC:
        case HEAVYHBOMB__STATIC:
            if (T1(spriteNum) == 0)
            {
                CS(spriteNum) &= ~257;
                T1(spriteNum) = 1;
                A_Spawn(spriteNum,BURNING);
            }
            break;
        }
        break;
    }

    case CACTUS__STATIC:
    {
        switch (DYNAMICTILEMAP(sprite[dmgSrc].picnum))
        {
        case RPG2__STATICRR:
            if (!RRRA) break;
            fallthrough__;
        case OWHIP__STATICRR:
        case UWHIP__STATICRR:
        case TRIPBOMBSPRITE__STATIC:
        case COOLEXPLOSION1__STATIC:
            if (!RR) break;
            fallthrough__;
        case RADIUSEXPLOSION__STATIC:
        case RPG__STATIC:
        case FIRELASER__STATIC:
        case HYDRENT__STATIC:
        case HEAVYHBOMB__STATIC:
            for (bssize_t k=64; k>0; k--)
            {
                int32_t const r1 = krand2(), r2 = krand2(), r3 = krand2(), r4 = krand2(), r5 = krand2();
                int newSprite =
                    A_InsertSprite(SECT(spriteNum), SX(spriteNum), SY(spriteNum), SZ(spriteNum) - (r5 % (48 << 8)), SCRAP3 + (r4 & 3), -8, 48, 48,
                        r3 & 2047, (r2 & 63) + 64, -(r1 & 4095) - (sprite[spriteNum].zvel >> 2), spriteNum, 5);
                sprite[newSprite].pal = 8;
            }
            //        case CACTUSBROKE:
            if (PN(spriteNum) == CACTUS)
                PN(spriteNum) = CACTUSBROKE;
            CS(spriteNum) &= ~257;
            break;
        }
        break;
    }

    case HANGLIGHT__STATIC:
    case GENERICPOLE2__STATIC:
        if (RR) goto default_case;
        for (bssize_t k=6; k>0; k--)
        {
            int32_t const r1 = krand2(), r2 = krand2(), r3 = krand2(), r4 = krand2();
            A_InsertSprite(SECT(spriteNum),SX(spriteNum),SY(spriteNum),SZ(spriteNum)-ZOFFSET3,SCRAP1+(r4&15),-8,48,48,r3&2047,(r2&63)+64,-(r1&4095)-(sprite[spriteNum].zvel>>2),spriteNum,5);
        }
        A_PlaySound(GLASS_HEAVYBREAK,spriteNum);
        A_DeleteSprite(spriteNum);
        break;
        
    case FANSPRITE__STATIC:
        PN(spriteNum) = FANSPRITEBROKE;
        CS(spriteNum) &= (65535-257);
        if (!RR && sector[SECT(spriteNum)].floorpicnum == FANSHADOW)
            sector[SECT(spriteNum)].floorpicnum = FANSHADOWBROKE;

        A_PlaySound(GLASS_HEAVYBREAK, spriteNum);

        for (bssize_t j=16; j>0; j--)
        {
            spritetype * const pSprite = &sprite[spriteNum];
            RANDOMSCRAP(pSprite, spriteNum);
        }
        break;

    case WATERFOUNTAIN__STATIC:
        //    case WATERFOUNTAIN+1:
        //    case WATERFOUNTAIN+2:
        if (!RR)
            PN(spriteNum) = WATERFOUNTAINBROKE;
        A_Spawn(spriteNum,TOILETWATER);
        break;

    case SATELITE__STATIC:
    case FUELPOD__STATIC:
    case SOLARPANNEL__STATIC:
    case ANTENNA__STATIC:
        if (sprite[dmgSrc].extra != G_DefaultActorHealth(SHOTSPARK1))
        {
            for (bssize_t j=0; j<15; j++)
            {
                int32_t const r1 = krand2(), r2 = krand2(), r3 = krand2(), r4 = krand2();
                A_InsertSprite(SECT(spriteNum),SX(spriteNum),SY(spriteNum),sector[SECT(spriteNum)].floorz-ZOFFSET4-(j<<9),SCRAP1+(r4&15),-8,64,64,
                               r3&2047,(r2&127)+64,-(r1&511)-256,spriteNum,5);
            }
            A_Spawn(spriteNum,EXPLOSION2);
            A_DeleteSprite(spriteNum);
        }
        break;

    case WATERFOUNTAINBROKE__STATIC:
        if (RR) goto default_case;
        fallthrough__;
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
    case RRTILE1824__STATICRR:
        if (RR && !RRRA && PN(spriteNum) == RRTILE1824) goto default_case;
        if (PN(spriteNum) == BOTTLE10)
            A_SpawnMultiple(spriteNum, MONEY, 4+(krand2()&3));
        else if (PN(spriteNum) == STATUE || PN(spriteNum) == STATUEFLASH)
        {
            A_SpawnRandomGlass(spriteNum,-1,40);
            A_PlaySound(GLASS_HEAVYBREAK,spriteNum);
        }
        else if (PN(spriteNum) == VASE)
            A_SpawnWallGlass(spriteNum,-1,40);

        A_PlaySound(GLASS_BREAKING,spriteNum);
        SA(spriteNum) = krand2()&2047;
        A_SpawnWallGlass(spriteNum,-1,8);
        A_DeleteSprite(spriteNum);
        break;

    case FETUS__STATIC:
        if (RR) goto default_case;
        PN(spriteNum) = FETUSBROKE;
        A_PlaySound(GLASS_BREAKING,spriteNum);
        A_SpawnWallGlass(spriteNum,-1,10);
        break;

    case FETUSBROKE__STATIC:
        if (RR) goto default_case;
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
    case RRTILE2654__STATICRR:
    case RRTILE2656__STATICRR:
    case RRTILE3172__STATICRR:
        if (!RRRA) goto default_case;
        A_PlaySound(GLASS_BREAKING,spriteNum);
        A_SpawnWallGlass(spriteNum,-1,10);
        A_DeleteSprite(spriteNum);
        break;

    case HYDROPLANT__STATIC:
        if (RR) goto default_case;
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
        if (RR) goto default_case;
        if (CS(spriteNum)&1)
        {
            A_PlaySound(GLASS_BREAKING,spriteNum);
            SZ(spriteNum) += ZOFFSET2;
            CS(spriteNum) = 0;
            A_SpawnWallGlass(spriteNum,-1,5);
        }
        break;

    case TOILET__STATIC:
        PN(spriteNum) = TOILETBROKE;
        CS(spriteNum) |= (krand2()&1)<<2;
        CS(spriteNum) &= ~257;
        A_Spawn(spriteNum,TOILETWATER);
        A_PlaySound(GLASS_BREAKING,spriteNum);
        break;

    case STALL__STATIC:
        PN(spriteNum) = STALLBROKE;
        CS(spriteNum) |= (krand2()&1)<<2;
        CS(spriteNum) &= ~257;
        A_Spawn(spriteNum,TOILETWATER);
        A_PlaySound(GLASS_HEAVYBREAK,spriteNum);
        break;

    case HYDRENT__STATIC:
        PN(spriteNum) = BROKEFIREHYDRENT;
        A_Spawn(spriteNum,TOILETWATER);

        //            for(k=0;k<5;k++)
        //          {
        //            j = A_InsertSprite(SECT,SX,SY,SZ-(krand2()%(48<<8)),SCRAP3+(krand2()&3),-8,48,48,krand2()&2047,(krand2()&63)+64,-(krand2()&4095)-(sprite[i].zvel>>2),i,5);
        //          sprite[j].pal = 2;
        //    }
        A_PlaySound(GLASS_HEAVYBREAK,spriteNum);
        break;
        
    case GRATE1__STATIC:
        PN(spriteNum) = BGRATE1;
        CS(spriteNum) &= (65535-256-1);
        A_PlaySound(VENT_BUST, spriteNum);
        break;

    case CIRCLEPANNEL__STATIC:
        PN(spriteNum) = CIRCLEPANNELBROKE;
        CS(spriteNum) &= (65535-256-1);
        A_PlaySound(VENT_BUST,spriteNum);
        break;

    case PANNEL1__STATIC:
    case PANNEL2__STATIC:
        if (RR) goto default_case;
        PN(spriteNum) = BPANNEL1;
        CS(spriteNum) &= (65535-256-1);
        A_PlaySound(VENT_BUST,spriteNum);
        break;

    case PANNEL3__STATIC:
        if (RR) goto default_case;
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
        if (RR) goto default_case;
        A_PlaySound(SLT(spriteNum),spriteNum);
        A_Spawn(spriteNum,SHT(spriteNum));
        fallthrough__;
    case SPACEMARINE__STATIC:
        if (RR) goto default_case;
        sprite[spriteNum].extra -= sprite[dmgSrc].extra;
        if (sprite[spriteNum].extra > 0) break;
        SA(spriteNum) = krand2()&2047;
        A_Shoot(spriteNum,BLOODSPLAT1);
        SA(spriteNum) = krand2()&2047;
        A_Shoot(spriteNum,BLOODSPLAT2);
        SA(spriteNum) = krand2()&2047;
        A_Shoot(spriteNum,BLOODSPLAT3);
        SA(spriteNum) = krand2()&2047;
        A_Shoot(spriteNum,BLOODSPLAT4);
        SA(spriteNum) = krand2()&2047;
        A_Shoot(spriteNum,BLOODSPLAT1);
        SA(spriteNum) = krand2()&2047;
        A_Shoot(spriteNum,BLOODSPLAT2);
        SA(spriteNum) = krand2()&2047;
        A_Shoot(spriteNum,BLOODSPLAT3);
        SA(spriteNum) = krand2()&2047;
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

    case TRIPODCAMERA__STATIC:
        if (RR) goto default_case;
        fallthrough__;
    case CHAIR3__STATIC:
    case MOVIECAMERA__STATIC:
    case SCALE__STATIC:
    case VACUUM__STATIC:
    case CAMERALIGHT__STATIC:
    case IVUNIT__STATIC:
    case POT1__STATIC:
    case POT2__STATIC:
    case POT3__STATIC:
        A_PlaySound(GLASS_HEAVYBREAK,spriteNum);
        for (bssize_t j=16; j>0; j--)
        {
            spritetype * const pSprite = &sprite[spriteNum];
            RANDOMSCRAP(pSprite, spriteNum);
        }
        A_DeleteSprite(spriteNum);
        break;

    case PLAYERONWATER__STATIC:
        spriteNum = OW(spriteNum);
        fallthrough__;
    default:
default_case:
        if ((sprite[spriteNum].cstat&16) && SHT(spriteNum) == 0 && SLT(spriteNum) == 0 && sprite[spriteNum].statnum == STAT_DEFAULT)
            break;

        if (((RR && sprite[dmgSrc].picnum == SHRINKSPARK) || sprite[dmgSrc].picnum == FREEZEBLAST || sprite[dmgSrc].owner != spriteNum) && sprite[spriteNum].statnum != STAT_PROJECTILE)
        {
            if (A_CheckEnemySprite(&sprite[spriteNum]) == 1)
            {
                if (sprite[dmgSrc].picnum == RPG || (RRRA && sprite[dmgSrc].picnum == RPG2))
                    sprite[dmgSrc].extra <<= 1;

                if ((PN(spriteNum) != DRONE) && (RR || ((PN(spriteNum) != ROTATEGUN) && (PN(spriteNum) != COMMANDER) && (PN(spriteNum) < GREENSLIME || PN(spriteNum) > GREENSLIME+7))))
                    if (sprite[dmgSrc].picnum != FREEZEBLAST)
                        if (!A_CheckSpriteFlags(spriteNum, SFLAG_BADGUY))
                        {
                            int const newSprite       = A_Spawn(dmgSrc, JIBS6);
                            sprite[newSprite].z      += ZOFFSET6;
                            if (sprite[dmgSrc].pal == 6)
                                sprite[newSprite].pal = 6;
                            sprite[newSprite].xvel    = 16;
                            sprite[newSprite].xrepeat = sprite[newSprite].yrepeat = 24;
                            sprite[newSprite].ang    += 32 - (krand2() & 63);
                        }

                int const damageOwner = sprite[dmgSrc].owner;

                if (damageOwner >= 0 && sprite[damageOwner].picnum == APLAYER && (RR || PN(spriteNum) != ROTATEGUN) && PN(spriteNum) != DRONE)
                    if (g_player[P_Get(damageOwner)].ps->curr_weapon == SHOTGUN_WEAPON)
                    {
                        A_Shoot(spriteNum, BLOODSPLAT3);
                        A_Shoot(spriteNum, BLOODSPLAT1);
                        A_Shoot(spriteNum, BLOODSPLAT2);
                        A_Shoot(spriteNum, BLOODSPLAT4);
                    }

                if (!RR && !A_CheckSpriteFlags(spriteNum, SFLAG_NODAMAGEPUSH))
                {
                    if ((sprite[spriteNum].cstat & 48) == 0)
                        SA(spriteNum)          = (sprite[dmgSrc].ang + 1024) & 2047;
                    sprite[spriteNum].xvel  = -(sprite[dmgSrc].extra << 2);
                    int16_t sectNum = SECT(spriteNum);
                    if ((unsigned)sectNum < MAXSECTORS)
                    {
                        pushmove((vec3_t *)&sprite[spriteNum], &sectNum, 128L, (4L << 8), (4L << 8), CLIPMASK0);
                        if (sectNum != SECT(spriteNum) && (unsigned)sectNum < MAXSECTORS)
                            changespritesect(spriteNum, sectNum);
                    }
                }

                if (sprite[spriteNum].statnum == STAT_ZOMBIEACTOR)
                {
                    changespritestat(spriteNum, STAT_ACTOR);
                    actor[spriteNum].timetosleep = SLEEPTIME;
                }

                if (!RR && (sprite[spriteNum].xrepeat < 24 || PN(spriteNum) == SHARK) && sprite[dmgSrc].picnum == SHRINKSPARK)
                    return;
            }

            if (sprite[spriteNum].statnum != STAT_ZOMBIEACTOR)
            {
                if (sprite[dmgSrc].picnum == FREEZEBLAST && ((PN(spriteNum) == APLAYER && sprite[spriteNum].pal == 1) || (g_freezerSelfDamage == 0 && sprite[dmgSrc].owner == spriteNum)))
                    return;
                actor[spriteNum].picnum = sprite[dmgSrc].picnum;
                actor[spriteNum].extra += sprite[dmgSrc].extra;
                if (!RR || PN(spriteNum) != COW)
                    actor[spriteNum].ang    = sprite[dmgSrc].ang;
                actor[spriteNum].owner  = sprite[dmgSrc].owner;
            }

            if (sprite[spriteNum].statnum == STAT_PLAYER)
            {
                DukePlayer_t *ps = g_player[P_Get(spriteNum)].ps;

                if (ps->newowner >= 0)
                    G_ClearCameraView(ps);

                if (!RR && sprite[spriteNum].xrepeat < 24 && sprite[dmgSrc].picnum == SHRINKSPARK)
                    return;

                if (sprite[actor[spriteNum].owner].picnum != APLAYER)
                    if (ud.player_skill >= 3)
                        sprite[dmgSrc].extra += (sprite[dmgSrc].extra>>1);
            }
        }

        break;
    }
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

void P_HandleSharedKeys(int playerNum)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    if (pPlayer->cheat_phase == 1) return;

    uint32_t playerBits = g_player[playerNum].input->bits;
    int32_t weaponNum;

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

    if (RR)
    {
        if (TEST_SYNC_KEY(playerBits, SK_QUICK_KICK) && pPlayer->last_pissed_time == 0
            && (!RRRA || sprite[pPlayer->i].extra > 0))
        {
            pPlayer->last_pissed_time = 4000;
            if (!adult_lockout)
                A_PlaySound(437, pPlayer->i);
            if (sprite[pPlayer->i].extra <= pPlayer->max_player_health - pPlayer->max_player_health / 10)
            {
                sprite[pPlayer->i].extra += 2;
                pPlayer->last_extra = sprite[pPlayer->i].extra;
            }
            else if (sprite[pPlayer->i].extra < pPlayer->max_player_health)
                sprite[pPlayer->i].extra = pPlayer->max_player_health;
        }
    }
    else
    {

        if (TEST_SYNC_KEY(playerBits, SK_QUICK_KICK) && pPlayer->quick_kick == 0)
            if (pPlayer->curr_weapon != KNEE_WEAPON || pPlayer->kickback_pic == 0)
            {
                if (VM_OnEvent(EVENT_QUICKKICK,g_player[playerNum].ps->i,playerNum) == 0)
                {
                    pPlayer->quick_kick = 14;
                    if (pPlayer->fta == 0 || pPlayer->ftq == 80)
                        P_DoQuote(QUOTE_MIGHTY_FOOT,pPlayer);
                }
            }
    }

    if (!(playerBits & ((15u<<SK_WEAPON_BITS)|BIT(SK_STEROIDS)|BIT(SK_NIGHTVISION)|BIT(SK_MEDKIT)|BIT(SK_QUICK_KICK)| \
                   BIT(SK_HOLSTER)|BIT(SK_INV_LEFT)|BIT(SK_PAUSE)|BIT(SK_HOLODUKE)|BIT(SK_JETPACK)|BIT(SK_INV_RIGHT)| \
                   BIT(SK_TURNAROUND)|BIT(SK_OPEN)|BIT(SK_INVENTORY)|BIT(SK_ESCAPE))))
        pPlayer->interface_toggle_flag = 0;
    else if (pPlayer->interface_toggle_flag == 0)
    {
        pPlayer->interface_toggle_flag = 1;

        if (TEST_SYNC_KEY(playerBits, SK_PAUSE))
        {
            inputState.ClearKeyStatus(sc_Pause);
            if (ud.pause_on)
                ud.pause_on = 0;
            else ud.pause_on = 1+SHIFTS_IS_PRESSED;
            if (ud.pause_on)
            {
                Mus_SetPaused(true);
                S_PauseSounds(true);
            }
            else
            {
                Mus_SetPaused(false);

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

        if (!RR && TEST_SYNC_KEY(playerBits, SK_NIGHTVISION))
        {
            if (VM_OnEvent(EVENT_USENIGHTVISION,g_player[playerNum].ps->i,playerNum) == 0
                    &&  pPlayer->inv_amount[GET_HEATS] > 0)
            {
                pPlayer->heat_on = !pPlayer->heat_on;
                P_UpdateScreenPal(pPlayer);
                pPlayer->inven_icon = ICON_HEATS;
                A_PlaySound(NITEVISION_ONOFF,pPlayer->i);
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
                    A_PlaySound(DUKE_TAKEPILLS,pPlayer->i);
                    P_DoQuote(QUOTE_USED_STEROIDS,pPlayer);
                }
                if (pPlayer->inv_amount[GET_STEROIDS] > 0)
                    pPlayer->inven_icon = ICON_STEROIDS;
            }
            return;		// is there significance to returning?
        }
        if (WW2GI && pPlayer->refresh_inventory)
            playerBits |= BIT(SK_INV_LEFT);   // emulate move left...

        if (pPlayer->newowner == -1 && (TEST_SYNC_KEY(playerBits, SK_INV_LEFT) || TEST_SYNC_KEY(playerBits, SK_INV_RIGHT)) || (!WW2GI && pPlayer->refresh_inventory))
        {
            pPlayer->invdisptime = GAMETICSPERSEC*2;

            int const inventoryRight = !!(TEST_SYNC_KEY(playerBits, SK_INV_RIGHT));

            if (pPlayer->refresh_inventory) pPlayer->refresh_inventory = 0;
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

                if (inventoryIcon || pPlayer->inv_amount[GET_FIRSTAID])
                {
                    static const int32_t invQuotes[7] = { QUOTE_MEDKIT, QUOTE_STEROIDS, QUOTE_HOLODUKE,
                        QUOTE_JETPACK, QUOTE_NVG, QUOTE_SCUBA, QUOTE_BOOTS };
                    if (inventoryIcon-1 < ARRAY_SSIZE(invQuotes))
                        P_DoQuote(invQuotes[inventoryIcon-1], pPlayer);
                }
            }
        }

        weaponNum = ((playerBits&(15<<SK_WEAPON_BITS))>>SK_WEAPON_BITS) - 1;
        if (weaponNum > 0 && pPlayer->kickback_pic > 0)
        {
            pPlayer->wantweaponfire = weaponNum;
        }

        if (pPlayer->last_pissed_time <= (GAMETICSPERSEC * 218) && pPlayer->show_empty_weapon == 0 &&
            pPlayer->kickback_pic == 0 && pPlayer->quick_kick == 0 && sprite[pPlayer->i].xrepeat > (RR ? 8 :32) && pPlayer->access_incs == 0 &&
            pPlayer->knee_incs == 0)
        {
            if(  (pPlayer->weapon_pos == 0 || (pPlayer->holster_weapon && pPlayer->weapon_pos == WEAPON_POS_LOWER ) ))
            {
                if (weaponNum == 10 || weaponNum == 11)
                {
                    int currentWeapon = pPlayer->curr_weapon;

                    if (RRRA)
                    {
                        if (currentWeapon == CHICKEN_WEAPON) currentWeapon = RPG_WEAPON;
                        else if (currentWeapon == GROW_WEAPON) currentWeapon = SHRINKER_WEAPON;
                        else if (currentWeapon == SLINGBLADE_WEAPON) currentWeapon = KNEE_WEAPON;
                    }

                    weaponNum = (weaponNum == 10 ? -1 : 1);  // JBF: prev (-1) or next (1) weapon choice
                    int i = 0;

                    while ((currentWeapon >= 0 && currentWeapon < 10) || (!RR && currentWeapon == GROW_WEAPON && (pPlayer->subweapon&(1 << GROW_WEAPON))))
                    {
                        if (!RR)
                        {
                            if (currentWeapon == GROW_WEAPON)
                            {
                                if (weaponNum == -1)
                                    currentWeapon = HANDBOMB_WEAPON;
                                else currentWeapon = DEVISTATOR_WEAPON;

                            }
                            else
                            {
                                currentWeapon += weaponNum;
                                if (currentWeapon == SHRINKER_WEAPON && pPlayer->subweapon&(1 << GROW_WEAPON))
                                    currentWeapon = GROW_WEAPON;
                            }
                        }
                        else
                            currentWeapon += weaponNum;

                        if (currentWeapon == -1) currentWeapon = FREEZE_WEAPON;
                        else if (currentWeapon == 10) currentWeapon = KNEE_WEAPON;

                        if ((pPlayer->gotweapon & (1<<currentWeapon)) && pPlayer->ammo_amount[currentWeapon] > 0)
                        {
                            if (!RR && currentWeapon == SHRINKER_WEAPON && pPlayer->subweapon&(1<<GROW_WEAPON))
                                currentWeapon = GROW_WEAPON;
                            weaponNum = currentWeapon;
                            break;
                        }
                        else if (!RR && currentWeapon == GROW_WEAPON && pPlayer->ammo_amount[GROW_WEAPON] == 0
                            && (pPlayer->gotweapon & (1<<SHRINKER_WEAPON)) && pPlayer->ammo_amount[SHRINKER_WEAPON] > 0)
                        {
                            weaponNum = SHRINKER_WEAPON;
                            pPlayer->subweapon &= ~(1<<GROW_WEAPON);
                            break;
                        }
                        else if (!RR && currentWeapon == SHRINKER_WEAPON && pPlayer->ammo_amount[SHRINKER_WEAPON] == 0
                            && (pPlayer->gotweapon & (1<<SHRINKER_WEAPON)) && pPlayer->ammo_amount[GROW_WEAPON] > 0)
                        {
                            weaponNum = GROW_WEAPON;
                            pPlayer->subweapon |= (1<<GROW_WEAPON);
                            break;
                        }

                        i++;

                        if (i == currentWeapon) // absolutely no weapons, so use foot
                        {
                            weaponNum = KNEE_WEAPON;
                            break;
                        }
                    }
                }

                if (weaponNum == HANDBOMB_WEAPON && pPlayer->ammo_amount[HANDBOMB_WEAPON] == 0)
                {
                    int spriteNum = headspritestat[1];
                    while (spriteNum >= 0)
                    {
                        if (sprite[spriteNum].picnum == HEAVYHBOMB && sprite[spriteNum].owner == pPlayer->i)
                        {
                            pPlayer->gotweapon |= 1<<HANDREMOTE_WEAPON;
                            weaponNum = HANDREMOTE_WEAPON;
                            break;
                        }
                        spriteNum = nextspritestat[spriteNum];
                    }
                }
                else if (RRRA)
                {
                    if (weaponNum == KNEE_WEAPON)
                    {
                        if(screenpeek == playerNum) pus = NUMPAGES;

                        if (pPlayer->curr_weapon == KNEE_WEAPON)
                        {
                            pPlayer->subweapon = 2;
                            weaponNum = SLINGBLADE_WEAPON;
                        }
                        else if(pPlayer->subweapon&2)
                        {
                            pPlayer->subweapon = 0;
                            weaponNum = KNEE_WEAPON;
                        }
                    }
                    else if (weaponNum == RPG_WEAPON)
                    {
                        if(screenpeek == playerNum) pus = NUMPAGES;

                        if (pPlayer->curr_weapon == RPG_WEAPON || pPlayer->ammo_amount[RPG_WEAPON] == 0)
                        {
                            if (pPlayer->ammo_amount[CHICKEN_WEAPON] == 0)
                                return;
                            pPlayer->subweapon = 4;
                            weaponNum = CHICKEN_WEAPON;
                        }
                        else if((pPlayer->subweapon&4) || pPlayer->ammo_amount[CHICKEN_WEAPON] == 0)
                        {
                            pPlayer->subweapon = 0;
                            weaponNum = RPG_WEAPON;
                        }
                    }
                }
                if (RR)
                {
                    if(weaponNum == SHRINKER_WEAPON)
                    {
                        if(screenpeek == playerNum) pus = NUMPAGES;

                        if (pPlayer->curr_weapon == SHRINKER_WEAPON || pPlayer->ammo_amount[SHRINKER_WEAPON] == 0)
                        {
                            pPlayer->subweapon = (1<<GROW_WEAPON);
                            weaponNum = GROW_WEAPON;
                        }
                        else if((pPlayer->subweapon&(1<<GROW_WEAPON)) || pPlayer->ammo_amount[GROW_WEAPON] == 0)
                        {
                            pPlayer->subweapon = 0;
                            weaponNum = SHRINKER_WEAPON;
                        }
                    }
                    else if(weaponNum == TRIPBOMB_WEAPON)
                    {
                        if(screenpeek == playerNum) pus = NUMPAGES;

                        if (pPlayer->curr_weapon == TRIPBOMB_WEAPON || pPlayer->ammo_amount[TRIPBOMB_WEAPON] == 0)
                        {
                            pPlayer->subweapon = (1<<BOWLINGBALL_WEAPON);
                            weaponNum = BOWLINGBALL_WEAPON;
                        }
                        else if((pPlayer->subweapon&(1<<BOWLINGBALL_WEAPON)) || pPlayer->ammo_amount[BOWLINGBALL_WEAPON] == 0)
                        {
                            pPlayer->subweapon = 0;
                            weaponNum = TRIPBOMB_WEAPON;
                        }
                    }
                }

                if (!RR && weaponNum == SHRINKER_WEAPON)
                {
                    if (screenpeek == playerNum) pus = NUMPAGES;

                    if (pPlayer->curr_weapon != GROW_WEAPON && pPlayer->curr_weapon != SHRINKER_WEAPON)
                    {
                        if (pPlayer->ammo_amount[GROW_WEAPON] > 0)
                        {
                            if ((pPlayer->subweapon&(1 << GROW_WEAPON)) == (1 << GROW_WEAPON))
                                weaponNum = GROW_WEAPON;
                            else if (pPlayer->ammo_amount[SHRINKER_WEAPON] == 0)
                            {
                                weaponNum = GROW_WEAPON;
                                pPlayer->subweapon |= (1 << GROW_WEAPON);
                            }
                        }
                        else if (pPlayer->ammo_amount[SHRINKER_WEAPON] > 0)
                            pPlayer->subweapon &= ~(1 << GROW_WEAPON);
                    }
                    else if (pPlayer->curr_weapon == SHRINKER_WEAPON)
                    {
                        pPlayer->subweapon |= (1 << GROW_WEAPON);
                        weaponNum = GROW_WEAPON;
                    }
                    else
                        pPlayer->subweapon &= ~(1 << GROW_WEAPON);
                }

                if (pPlayer->holster_weapon)
                {
                    playerBits |= BIT(SK_HOLSTER);
                    pPlayer->weapon_pos = WEAPON_POS_LOWER;
                }
                else if ((uint32_t)weaponNum < MAX_WEAPONS && (pPlayer->gotweapon & (1<<weaponNum)) && pPlayer->curr_weapon != weaponNum)
                    switch (DYNAMICWEAPONMAP(weaponNum))
                    {
                    case SLINGBLADE_WEAPON__STATIC:
                        if (!RRRA) break;
                        A_PlaySound(496,g_player[screenpeek].ps->i);
                        P_AddWeapon(pPlayer, weaponNum);
                        break;
                    case CHICKEN_WEAPON__STATIC:
                        if (!RRRA) break;
                        fallthrough__;
                    case BOWLINGBALL_WEAPON__STATIC:
                        if (!RR) break;
                        fallthrough__;
                    case PISTOL_WEAPON__STATIC:
                    case SHOTGUN_WEAPON__STATIC:
                    case CHAINGUN_WEAPON__STATIC:
                    case RPG_WEAPON__STATIC:
                    case DEVISTATOR_WEAPON__STATIC:
                    case FREEZE_WEAPON__STATIC:
                    case GROW_WEAPON__STATIC:
                    case SHRINKER_WEAPON__STATIC:
rrtripbomb_case:
                        if (pPlayer->ammo_amount[weaponNum] == 0 && pPlayer->show_empty_weapon == 0)
                        {
                            pPlayer->last_full_weapon = pPlayer->curr_weapon;
                            pPlayer->show_empty_weapon = 32;
                        }
                        fallthrough__;
                    case KNEE_WEAPON__STATIC:
                        P_AddWeapon(pPlayer, weaponNum);
                        break;
                    case HANDREMOTE_WEAPON__STATIC:
                        pPlayer->curr_weapon = HANDREMOTE_WEAPON;
                        pPlayer->last_weapon = -1;
                        pPlayer->weapon_pos = WEAPON_POS_RAISE;
                        break;
                    case HANDBOMB_WEAPON__STATIC:
                    case TRIPBOMB_WEAPON__STATIC:
                        if (RR && weaponNum == TRIPBOMB) goto rrtripbomb_case;
                        if (pPlayer->ammo_amount[weaponNum] > 0 && (pPlayer->gotweapon & (1<<weaponNum)))
                            P_AddWeapon(pPlayer, weaponNum);
                        break;
                    case MOTORCYCLE_WEAPON__STATIC:
                    case BOAT_WEAPON__STATIC:
                        if (!RRRA) break;
                        if (pPlayer->ammo_amount[weaponNum] == 0 && pPlayer->show_empty_weapon == 0)
                            pPlayer->show_empty_weapon = 32;
                        P_AddWeapon(pPlayer, weaponNum);
                        break;
                    }
            }

            if (TEST_SYNC_KEY(playerBits, SK_HOLSTER))
            {
                if (pPlayer->curr_weapon > KNEE_WEAPON)
                {
                    if (pPlayer->holster_weapon == 0 && pPlayer->weapon_pos == 0)
                    {
                        pPlayer->holster_weapon = 1;
                        pPlayer->weapon_pos = -1;
                        P_DoQuote(QUOTE_WEAPON_LOWERED, pPlayer);
                    }
                    else if (pPlayer->holster_weapon == 1 && pPlayer->weapon_pos == WEAPON_POS_LOWER)
                    {
                        pPlayer->holster_weapon = 0;
                        pPlayer->weapon_pos = WEAPON_POS_RAISE;
                        P_DoQuote(QUOTE_WEAPON_RAISED, pPlayer);
                    }
                }
            }
        }

        if (TEST_SYNC_KEY(playerBits, SK_HOLODUKE) && (RR || pPlayer->newowner == -1))
        {
            if (RR)
            {
                if (pPlayer->inv_amount[GET_HOLODUKE] > 0 && sprite[pPlayer->i].extra < pPlayer->max_player_health)
                {
                    pPlayer->inv_amount[GET_HOLODUKE] -= 400;
                    sprite[pPlayer->i].extra += 5;
                    if (sprite[pPlayer->i].extra > pPlayer->max_player_health)
                        sprite[pPlayer->i].extra = pPlayer->max_player_health;

                    pPlayer->drink_amt += 5;
                    pPlayer->inven_icon = 3;
                    if (pPlayer->inv_amount[GET_HOLODUKE] == 0)
                        P_SelectNextInvItem(pPlayer);

                    if (pPlayer->drink_amt < 99)
                        if (!A_CheckSoundPlaying(pPlayer->i, 425))
                            A_PlaySound(425, pPlayer->i);
                }
            }
            else
            {
                if (pPlayer->holoduke_on == -1)
                {
                    if (VM_OnEvent(EVENT_HOLODUKEON, g_player[playerNum].ps->i, playerNum) == 0)
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
                                A_PlaySound(TELEPORTER,pPlayer->holoduke_on);
                            }
                        }
                        else P_DoQuote(QUOTE_HOLODUKE_NOT_FOUND,pPlayer);
                    }
                }
                else
                {
                    if (VM_OnEvent(EVENT_HOLODUKEOFF,g_player[playerNum].ps->i,playerNum) == 0)
                    {
                        A_PlaySound(TELEPORTER,pPlayer->holoduke_on);
                        pPlayer->holoduke_on = -1;
                        P_DoQuote(QUOTE_HOLODUKE_OFF,pPlayer);
                    }
                }
            }
        }

        if (RR && TEST_SYNC_KEY(playerBits, SK_NIGHTVISION) && pPlayer->newowner == -1 && pPlayer->yehaa_timer == 0)
        {
            pPlayer->yehaa_timer = 126;
            A_PlaySound(390, pPlayer->i);
            pPlayer->noise_radius = 16384;
            P_MadeNoise(playerNum);
            if (sector[pPlayer->cursectnum].lotag == 857)
            {
                if (sprite[pPlayer->i].extra <= pPlayer->max_player_health)
                {
                    sprite[pPlayer->i].extra += 10;
                    if (sprite[pPlayer->i].extra >= pPlayer->max_player_health)
                        sprite[pPlayer->i].extra = pPlayer->max_player_health;
                }
            }
            else
            {
                if (sprite[pPlayer->i].extra + 1 <= pPlayer->max_player_health)
                {
                    sprite[pPlayer->i].extra++;
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

                    if (RR) healthDiff = 10;

                    if (pPlayer->inv_amount[GET_FIRSTAID] > healthDiff)
                    {
                        pPlayer->inv_amount[GET_FIRSTAID] -= healthDiff;
                        if (RR)
                            sprite[pPlayer->i].extra += healthDiff;
                        if (!RR || sprite[pPlayer->i].extra > pPlayer->max_player_health)
                            sprite[pPlayer->i].extra = pPlayer->max_player_health;
                        pPlayer->inven_icon = ICON_FIRSTAID;
                    }
                    else
                    {
                        sprite[pPlayer->i].extra += pPlayer->inv_amount[GET_FIRSTAID];
                        pPlayer->inv_amount[GET_FIRSTAID] = 0;
                        P_SelectNextInvItem(pPlayer);
                    }
                    if (RR)
                    {
                        if (sprite[pPlayer->i].extra > pPlayer->max_player_health)
                            sprite[pPlayer->i].extra = pPlayer->max_player_health;
                        pPlayer->drink_amt += 10;
                    }
                    if (!RR || (pPlayer->drink_amt <= 100 && !A_CheckSoundPlaying(pPlayer->i, DUKE_USEMEDKIT)))
                        A_PlaySound(DUKE_USEMEDKIT,pPlayer->i);
                }
            }
        }

        if ((pPlayer->newowner == -1 || RR) && TEST_SYNC_KEY(playerBits, SK_JETPACK))
        {
            if (RR)
            {
                if (VM_OnEvent(EVENT_USEJETPACK,g_player[playerNum].ps->i,playerNum) == 0)
                {
                    if (pPlayer->inv_amount[GET_JETPACK] > 0 && sprite[pPlayer->i].extra < pPlayer->max_player_health)
                    {
                        if (!A_CheckSoundPlaying(pPlayer->i, 429))
                            A_PlaySound(429, pPlayer->i);

                        pPlayer->inv_amount[GET_JETPACK] -= 100;
                        if (pPlayer->drink_amt > 0)
                        {
                            pPlayer->drink_amt -= 5;
                            if (pPlayer->drink_amt < 0)
                                pPlayer->drink_amt = 0;
                        }

                        if (pPlayer->eat_amt < 100)
                        {
                            pPlayer->eat_amt += 5;
                            if (pPlayer->eat_amt > 100)
                                pPlayer->eat_amt = 100;
                        }

                        sprite[pPlayer->i].extra += 5;

                        pPlayer->inven_icon = 4;

                        if (sprite[pPlayer->i].extra > pPlayer->max_player_health)
                            sprite[pPlayer->i].extra = pPlayer->max_player_health;

                        if (pPlayer->inv_amount[GET_JETPACK] <= 0)
                            P_SelectNextInvItem(pPlayer);
                    }
                }
            }
            else
            {
                if (pPlayer->inv_amount[GET_JETPACK] > 0)
                {
                    pPlayer->jetpack_on = !pPlayer->jetpack_on;
                    if (pPlayer->jetpack_on)
                    {
                        pPlayer->inven_icon = ICON_JETPACK;
                        S_StopEnvSound(-1, pPlayer->i, CHAN_VOICE);

                        A_PlaySound(DUKE_JETPACK_ON,pPlayer->i);

                        P_DoQuote(QUOTE_JETPACK_ON,pPlayer);
                    }
                    else
                    {
                        pPlayer->hard_landing = 0;
                        pPlayer->vel.z = 0;
                        A_PlaySound(DUKE_JETPACK_OFF,pPlayer->i);
                        S_StopEnvSound(DUKE_JETPACK_IDLE,pPlayer->i);
                        S_StopEnvSound(DUKE_JETPACK_ON,pPlayer->i);
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
static int32_t our_neartag_blacklist(int32_t UNUSED(spriteNum))
{
    return 0;
}

static void G_ClearCameras(DukePlayer_t *p)
{
    G_ClearCameraView(p);
}

void P_CheckSectors(int playerNum)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    if (pPlayer->cursectnum > -1)
    {
        sectortype *const pSector = &sector[pPlayer->cursectnum];
        switch ((uint16_t)pSector->lotag)
        {
            case 32767:
                pSector->lotag = 0;
                if (RR && !RRRA)
                    g_canSeePlayer = 0;
                P_DoQuote(QUOTE_FOUND_SECRET, pPlayer);
				SECRET_Trigger(pPlayer->cursectnum);
				pPlayer->secret_rooms++;
                return;

            case UINT16_MAX:
                pSector->lotag = 0;
                for (bssize_t TRAVERSE_CONNECT(playerNum))
                    g_player[playerNum].ps->gm = MODE_EOL;

                if (!RRRA || !g_RAendLevel)
                {
                    if (ud.from_bonus)
                    {
                        ud.level_number = ud.from_bonus;
                        m_level_number = ud.level_number;
                        ud.from_bonus = 0;
                    }
                    else
                    {
                        if (RRRA && ud.level_number == 6 && ud.volume_number == 0)
                            g_RAendEpisode = 1;
                        ud.level_number = (++ud.level_number < MAXLEVELS) ? ud.level_number : 0;
                        m_level_number = ud.level_number;
                    }
                    g_RAendLevel = 1;
                }
                return;

            case UINT16_MAX-1:
                pSector->lotag           = 0;
                pPlayer->timebeforeexit  = GAMETICSPERSEC * 8;
                pPlayer->customexitsound = pSector->hitag;
                return;

            default:
                if (pSector->lotag >= 10000 && (RR || pSector->lotag < 16383))
                {
                    if (playerNum == screenpeek || (g_gametypeFlags[ud.coop] & GAMETYPE_COOPSOUND))
                    {
                        if (RR && !RRRA)
                            g_canSeePlayer = -1;
                        A_PlaySound(pSector->lotag - 10000, pPlayer->i);
                    }
                    pSector->lotag = 0;
                }
                break;
        }
    }

    //After this point the the player effects the map with space

    if (pPlayer->gm &MODE_TYPE || sprite[pPlayer->i].extra <= 0)
        return;

    if (ud.cashman && TEST_SYNC_KEY(g_player[playerNum].input->bits, SK_OPEN))
    {
        if (RR && !RRRA)
            g_canSeePlayer = -1;
        A_SpawnMultiple(pPlayer->i, MONEY, 2);
    }

    if (!RR && pPlayer->newowner >= 0)
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

        if (!RR && TEST_SYNC_KEY(g_player[playerNum].input->bits, SK_ESCAPE))
        {
            if (pPlayer->newowner >= 0)
                G_ClearCameras(pPlayer);
            return;
        }

        nearSprite = -1;
        pPlayer->toggle_key_flag = 1;
        foundWall = -1;

        if (RR && !RRRA)
        {
            hitdata_t hitData;
            hitscan((const vec3_t *)pPlayer, pPlayer->cursectnum, sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047],
                    sintable[fix16_to_int(pPlayer->q16ang) & 2047], 0, &hitData, CLIPMASK0);
            g_canSeePlayer &= ~0xffff;
            g_canSeePlayer |= hitData.sect;
        }

        int wallDist = P_FindWall(pPlayer, &foundWall);

        if (RRRA)
        {
            if (foundWall >= 0 && wall[foundWall].overpicnum == MIRROR && playerNum == screenpeek)
                if (!g_netServer && numplayers == 1)
                {
                    if (A_CheckSoundPlaying(pPlayer->i,27) == 0 && A_CheckSoundPlaying(pPlayer->i,28) == 0 && A_CheckSoundPlaying(pPlayer->i,29) == 0
                        && A_CheckSoundPlaying(pPlayer->i,257) == 0 && A_CheckSoundPlaying(pPlayer->i,258) == 0)
                    {
                        int snd = krand2() % 5;
                        if (snd == 0)
                            A_PlaySound(27, pPlayer->i);
                        else if (snd == 1)
                            A_PlaySound(28, pPlayer->i);
                        else if (snd == 2)
                            A_PlaySound(29, pPlayer->i);
                        else if (snd == 3)
                            A_PlaySound(257, pPlayer->i);
                        else if (snd == 4)
                            A_PlaySound(258, pPlayer->i);
                    }
                    return;
                }
        }
        else if (foundWall >= 0 && (RR || wallDist < 1280) && wall[foundWall].overpicnum == MIRROR)
            if (wall[foundWall].lotag > 0 && !A_CheckSoundPlaying(pPlayer->i,wall[foundWall].lotag) && playerNum == screenpeek)
            {
                if (RR)
                    g_canSeePlayer = -1;
                A_PlaySound(wall[foundWall].lotag,pPlayer->i);
                return;
            }

        if (foundWall >= 0 && (wall[foundWall].cstat&16))
        {
            if (RRRA)
                g_canSeePlayer = foundWall*32;
            if (wall[foundWall].lotag)
                return;
        }

        int const intang = fix16_to_int(pPlayer->oq16ang);

        if (RRRA)
        {
            if (pPlayer->on_motorcycle)
            {
                if (pPlayer->moto_speed < 20)
                    G_OffMotorcycle(pPlayer);
                return;
            }
            if (pPlayer->on_boat)
            {
                if (pPlayer->moto_speed < 20)
                    G_OffBoat(pPlayer);
                return;
            }
            neartag(pPlayer->opos.x, pPlayer->opos.y, pPlayer->opos.z, sprite[pPlayer->i].sectnum, intang, &nearSector,
                &nearWall, &nearSprite, &nearDist, 1280, 1, our_neartag_blacklist);
        }

        if (RR && !RRRA)
            g_canSeePlayer = -1;

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
                        case PODFEM1__STATIC:
                        case FEM1__STATIC:
                        case FEM2__STATIC:
                        case FEM3__STATIC:
                        case FEM4__STATIC:
                        case FEM5__STATIC:
                        case FEM6__STATIC:
                        case FEM7__STATIC:
                        case FEM8__STATIC:
                        case FEM9__STATIC:
                            if (RR) break;
                            fallthrough__;
                        case FEM10__STATIC:
                        case NAKED1__STATIC:
                        case STATUE__STATIC:
                        case TOUGHGAL__STATIC: return;
                        case COW__STATICRR:
                            g_spriteExtra[nearSprite] = 1;
                            return;
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
            if (RR && !RRRA)
                g_canSeePlayer = playerNum;
            if (P_ActivateSwitch(playerNum, nearSprite, 1))
                return;

            switch (DYNAMICTILEMAP(sprite[nearSprite].picnum))
            {
            case RRTILE8448__STATICRR:
                if (!RRRA) break;
                if (!A_CheckSoundPlaying(nearSprite, 340))
                    A_PlaySound(340, nearSprite);
                return;
            case RRTILE8704__STATICRR:
                if (!RRRA) break;
                if (!g_netServer && numplayers == 1)
                {
                    static int soundPlayed = 0;
                    if (S_CheckSoundPlaying(nearSprite, 445) == 0 && soundPlayed == 0)
                    {
                        A_PlaySound(445, nearSprite);
                        soundPlayed = 1;
                    }
                    else if (S_CheckSoundPlaying(nearSprite, 445) == 0 && S_CheckSoundPlaying(nearSprite, 446) == 0
                        && S_CheckSoundPlaying(nearSprite, 447) == 0 && soundPlayed == 0)
                    {
                        if ((krand2()%2) == 1)
                            A_PlaySound(446, nearSprite);
                        else
                            A_PlaySound(447, nearSprite);
                    }
                }
                return;
            case EMPTYBIKE__STATICRR:
                if (!RRRA) break;
                G_OnMotorcycle(pPlayer, nearSprite);
                return;
            case EMPTYBOAT__STATICRR:
                if (!RRRA) break;
                G_OnBoat(pPlayer, nearSprite);
                return;
            case RRTILE8164__STATICRR:
            case RRTILE8165__STATICRR:
            case RRTILE8166__STATICRR:
            case RRTILE8167__STATICRR:
            case RRTILE8168__STATICRR:
            case RRTILE8591__STATICRR:
            case RRTILE8592__STATICRR:
            case RRTILE8593__STATICRR:
            case RRTILE8594__STATICRR:
            case RRTILE8595__STATICRR:
                if (!RRRA) break;
                sprite[nearSprite].extra = 60;
                A_PlaySound(235, nearSprite);
                return;
            case TOILET__STATIC:
            case STALL__STATIC:
            case RRTILE2121__STATICRR:
            case RRTILE2122__STATICRR:
                if (pPlayer->last_pissed_time == 0)
                {
                    if (adult_lockout == 0)
                        A_PlaySound(RR ? 435 : DUKE_URINATE, pPlayer->i);

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
                else if (!A_CheckSoundPlaying(nearSprite,RR ? DUKE_GRUNT : FLUSH_TOILET))
                {
                    if (RR && !RRRA)
                        g_canSeePlayer = -1;

                    A_PlaySound(RR ? DUKE_GRUNT : FLUSH_TOILET,nearSprite);
                }
                return;

            case NUKEBUTTON__STATIC:
            {
                if (RR) break;
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
                        if (RR && !RRRA)
                            g_canSeePlayer = -1;
                        A_PlaySound(DUKE_DRINKING,pPlayer->i);
                    }
                }
                return;

            case PLUG__STATIC:
                if (RR && !RRRA)
                    g_canSeePlayer = -1;
                A_PlaySound(SHORT_CIRCUIT, pPlayer->i);
                sprite[pPlayer->i].extra -= 2+(krand2()&3);

                P_PalFrom(pPlayer, 32, 48,48,64);
                break;

            case VIEWSCREEN__STATIC:
            case VIEWSCREEN2__STATIC:
                if (RR) break;
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

                        //P_UpdatePosWhenViewingCam(pPlayer);

                        return;
                    }
                }

                G_ClearCameras(pPlayer);
                return;
            }  // switch
        }

        if (TEST_SYNC_KEY(g_player[playerNum].input->bits, SK_OPEN) == 0)
            return;

        if (!RR && pPlayer->newowner >= 0)
        {
            G_ClearCameras(pPlayer);
            return;
        }
        if (RR && !RRRA && nearWall == -1 && nearSector == -1 && nearSprite == -1)
            g_canSeePlayer = playerNum;

        if (nearWall == -1 && nearSector == -1 && nearSprite == -1)
        {
            if (klabs(A_GetHitscanRange(pPlayer->i)) < 512)
            {
                if (RR && !RRRA)
                    g_canSeePlayer = -1;
                A_PlaySound(((krand2()&255) < 16) ? DUKE_SEARCH2 : DUKE_SEARCH, pPlayer->i);
                return;
            }
        }

        if (nearWall >= 0)
        {
            if (wall[nearWall].lotag > 0 && CheckDoorTile(wall[nearWall].picnum))
            {
                if (foundWall == nearWall || foundWall == -1)
                {
                    if (RR && !RRRA)
                        g_canSeePlayer = playerNum;
                    P_ActivateSwitch(playerNum,nearWall,0);
                }
                return;
            }
            else if (!RR && pPlayer->newowner >= 0)
            {
                G_ClearCameras(pPlayer);
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

            if (!RR || P_HasKey(nearSector, playerNum))
            {
                if (RR && !RRRA)
                    g_canSeePlayer = -1;
                G_OperateSectors(nearSector,pPlayer->i);
            }
            else if (RR)
            {
                if (g_sectorExtra[nearSector] > 3)
                    A_PlaySound(99,pPlayer->i);
                else
                    A_PlaySound(419,pPlayer->i);
                if (RR && !RRRA)
                    g_canSeePlayer = -1;
                P_DoQuote(41,pPlayer);
            }
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
                
                if (!RR || P_HasKey(sprite[pPlayer->i].sectnum, playerNum))
                {
                    if (RR && !RRRA)
                        g_canSeePlayer = -1;
                    G_OperateSectors(sprite[pPlayer->i].sectnum,pPlayer->i);
                }
                else if (RR)
                {
                    if (g_sectorExtra[sprite[pPlayer->i].sectnum] > 3)
                        A_PlaySound(99,pPlayer->i);
                    else
                        A_PlaySound(419,pPlayer->i);
                    if (RR && !RRRA)
                        g_canSeePlayer = -1;
                    P_DoQuote(41,pPlayer);
                }
            }
            else P_ActivateSwitch(playerNum,nearWall,0);
        }
    }
}

void G_DoFurniture(int wallNum, int sectNum, int playerNum)
{
    int startwall, endwall;
    int insideCheck, i;
    int32_t max_x, min_x, max_y, min_y, speed;
    startwall = sector[wall[wallNum].nextsector].wallptr;
    endwall = startwall+sector[wall[wallNum].nextsector].wallnum;

    insideCheck = 1;
    max_x = max_y = -(2<<16);
    min_x = min_y = 2<<16;
    speed = sector[sectNum].hitag;
    if (speed > 16)
        speed = 16;
    else if (speed == 0)
        speed = 4;
    for (i = startwall; i < endwall; i++)
    {
        if (max_x < wall[i].x)
            max_x = wall[i].x;
        if (max_y < wall[i].y)
            max_y = wall[i].y;
        if (min_x > wall[i].x)
            min_x = wall[i].x;
        if (min_y > wall[i].y)
            min_y = wall[i].y;
    }
    max_x += speed+1;
    max_y += speed+1;
    min_x -= speed+1;
    min_y -= speed+1;
    if (!inside(max_x, max_y, sectNum))
        insideCheck = 0;
    if (!inside(max_x, min_y, sectNum))
        insideCheck = 0;
    if (!inside(min_x, min_y, sectNum))
        insideCheck = 0;
    if (!inside(min_x, max_y, sectNum))
        insideCheck = 0;
    if (insideCheck)
    {
        if (!S_CheckSoundPlaying(g_player[playerNum].ps->i, 389))
            A_PlaySound(389, g_player[playerNum].ps->i);
        for (i = startwall; i < endwall; i++)
        {
            int32_t x, y;
            x = wall[i].x;
            y = wall[i].y;
            switch (wall[wallNum].lotag)
            {
                case 42:
                    dragpoint(i,x,y+speed,0);
                    break;
                case 41:
                    dragpoint(i,x-speed,y,0);
                    break;
                case 40:
                    dragpoint(i,x,y-speed,0);
                    break;
                case 43:
                    dragpoint(i,x+speed,y,0);
                    break;
            }
        }
    }
    else
    {
        speed -= 2;
        for (i = startwall; i < endwall; i++)
        {
            int32_t x, y;
            x = wall[i].x;
            y = wall[i].y;
            switch (wall[wallNum].lotag)
            {
                case 42:
                    dragpoint(i,x,y-speed,0);
                    break;
                case 41:
                    dragpoint(i,x+speed,y,0);
                    break;
                case 40:
                    dragpoint(i,x,y+speed,0);
                    break;
                case 43:
                    dragpoint(i,x-speed,y,0);
                    break;
            }
        }
    }
}

void G_DoTorch(void)
{
    int j;
    int startWall, endWall;
    int randNum = rand()&8;
    for (bsize_t i = 0; i < g_torchCnt; i++)
    {
        int shade = g_torchSectorShade[i] - randNum;
        switch (g_torchType[i])
        {
        case 0:
            sector[g_torchSector[i]].floorshade = shade;
            sector[g_torchSector[i]].ceilingshade = shade;
            break;
        case 1:
            sector[g_torchSector[i]].ceilingshade = shade;
            break;
        case 2:
            sector[g_torchSector[i]].floorshade = shade;
            break;
        case 4:
            sector[g_torchSector[i]].ceilingshade = shade;
            break;
        case 5:
            sector[g_torchSector[i]].floorshade = shade;
            break;
        }
        startWall = sector[g_torchSector[i]].wallptr;
        endWall = startWall + sector[g_torchSector[i]].wallnum - 1;
        for (j = startWall; j <= endWall; j++)
        {
            if (wall[j].lotag != 1)
            {
                switch (g_torchType[i])
                {
                    case 0:
                        wall[j].shade = shade;
                        break;
                    case 1:
                        wall[j].shade = shade;
                        break;
                    case 2:
                        wall[j].shade = shade;
                        break;
                    case 3:
                        wall[j].shade = shade;
                        break;
                }
            }
        }
    }
}

void G_DoJailDoor(void)
{
    int j;
    int32_t speed;
    int startWall, endWall;
    for (bsize_t i = 0; i < g_jailDoorCnt; i++)
    {
        speed = g_jailDoorSpeed[i];
        if (speed < 2)
            speed = 2;

        if (g_jailDoorOpen[i] == 1)
        {
            g_jailDoorDrag[i] -= speed;
            if (g_jailDoorDrag[i] <= 0)
            {
                g_jailDoorDrag[i] = 0;
                g_jailDoorOpen[i] = 2;
                switch (g_jailDoorDir[i])
                {
                case 10:
                    g_jailDoorDir[i] = 30;
                    break;
                case 20:
                    g_jailDoorDir[i] = 40;
                    break;
                case 30:
                    g_jailDoorDir[i] = 10;
                    break;
                case 40:
                    g_jailDoorDir[i] = 20;
                    break;
                }
            }
            else
            {
                startWall = sector[g_jailDoorSect[i]].wallptr;
                endWall = startWall + sector[g_jailDoorSect[i]].wallnum - 1;
                for (j = startWall; j <= endWall; j++)
                {
                    int32_t x, y;
                    x = wall[j].x;
                    y = wall[j].y;
                    switch (g_jailDoorDir[i])
                    {
                    case 10:
                        y += speed;
                        break;
                    case 20:
                        x -= speed;
                        break;
                    case 30:
                        y -= speed;
                        break;
                    case 40:
                        x += speed;
                        break;
                    }
                    dragpoint(j, x, y, 0);
                }
            }
        }
        if (g_jailDoorOpen[i] == 3)
        {
            g_jailDoorDrag[i] -= speed;
            if (g_jailDoorDrag[i] <= 0)
            {
                g_jailDoorDrag[i] = 0;
                g_jailDoorOpen[i] = 0;
                switch (g_jailDoorDir[i])
                {
                case 10:
                    g_jailDoorDir[i] = 30;
                    break;
                case 20:
                    g_jailDoorDir[i] = 40;
                    break;
                case 30:
                    g_jailDoorDir[i] = 10;
                    break;
                case 40:
                    g_jailDoorDir[i] = 20;
                    break;
                }
            }
            else
            {
                startWall = sector[g_jailDoorSect[i]].wallptr;
                endWall = startWall + sector[g_jailDoorSect[i]].wallnum - 1;
                for (j = startWall; j <= endWall; j++)
                {
                    int32_t x, y;
                    x = wall[j].x;
                    y = wall[j].y;
                    switch (g_jailDoorDir[i])
                    {
                    case 10:
                        y += speed;
                        break;
                    case 20:
                        x -= speed;
                        break;
                    case 30:
                        y -= speed;
                        break;
                    case 40:
                        x += speed;
                        break;
                    }
                    dragpoint(j, x, y, 0);
                }
            }
        }
    }
}

void G_MoveMineCart(void)
{
    int j, nextj;
    int startWall, endWall;
    int32_t speed;
    int32_t max_x, min_x, max_y, min_y, cx, cy;
    for (bsize_t i = 0; i < g_mineCartCnt; i++)
    {
        speed = g_mineCartSpeed[i];
        if (speed < 2)
            speed = 2;

        if (g_mineCartOpen[i] == 1)
        {
            g_mineCartDrag[i] -= speed;
            if (g_mineCartDrag[i] <= 0)
            {
                g_mineCartDrag[i] = g_mineCartDist[i];
                g_mineCartOpen[i] = 2;
                switch (g_mineCartDir[i])
                {
                case 10:
                    g_mineCartDir[i] = 30;
                    break;
                case 20:
                    g_mineCartDir[i] = 40;
                    break;
                case 30:
                    g_mineCartDir[i] = 10;
                    break;
                case 40:
                    g_mineCartDir[i] = 20;
                    break;
                }
            }
            else
            {
                startWall = sector[g_mineCartSect[i]].wallptr;
                endWall = startWall + sector[g_mineCartSect[i]].wallnum - 1;
                for (j = startWall; j <= endWall; j++)
                {
                    int32_t x, y;
                    x = wall[j].x;
                    y = wall[j].y;
                    switch (g_mineCartDir[i])
                    {
                    case 10:
                        y += speed;
                        break;
                    case 20:
                        x -= speed;
                        break;
                    case 30:
                        y -= speed;
                        break;
                    case 40:
                        x += speed;
                        break;
                    }
                    dragpoint(j, x, y, 0);
                }
            }
        }
        if (g_mineCartOpen[i] == 2)
        {
            g_mineCartDrag[i] -= speed;
            if (g_mineCartDrag[i] <= 0)
            {
                g_mineCartDrag[i] = g_mineCartDist[i];
                g_mineCartOpen[i] = 1;
                switch (g_mineCartDir[i])
                {
                case 10:
                    g_mineCartDir[i] = 30;
                    break;
                case 20:
                    g_mineCartDir[i] = 40;
                    break;
                case 30:
                    g_mineCartDir[i] = 10;
                    break;
                case 40:
                    g_mineCartDir[i] = 20;
                    break;
                }
            }
            else
            {
                startWall = sector[g_mineCartSect[i]].wallptr;
                endWall = startWall + sector[g_mineCartSect[i]].wallnum - 1;
                for (j = startWall; j <= endWall; j++)
                {
                    int32_t x, y;
                    x = wall[j].x;
                    y = wall[j].y;
                    switch (g_mineCartDir[i])
                    {
                    case 10:
                        y += speed;
                        break;
                    case 20:
                        x -= speed;
                        break;
                    case 30:
                        y -= speed;
                        break;
                    case 40:
                        x += speed;
                        break;
                    }
                    dragpoint(j, x, y, 0);
                }
            }
        }
        startWall = sector[g_mineCartChildSect[i]].wallptr;
        endWall = startWall + sector[g_mineCartChildSect[i]].wallnum - 1;
        max_x = max_y = -(2<<16);
        min_x = min_y = 2<<16;
        for (j = startWall; j <= endWall; j++)
        {
            if (max_x < wall[j].x)
                max_x = wall[j].x;
            if (max_y < wall[j].y)
                max_y = wall[j].y;
            if (min_x > wall[j].x)
                min_x = wall[j].x;
            if (min_y > wall[j].y)
                min_y = wall[j].y;
        }
        cx = (max_x + min_x) >> 1;
        cy = (max_y + min_y) >> 1;
        j = headspritesect[g_mineCartChildSect[i]];
        vec3_t pos;
        pos.x = cx;
        pos.y = cy;
        while (j != -1)
        {
            nextj = nextspritesect[j];
            pos.z = sprite[j].z;
            if (A_CheckEnemySprite(&sprite[j]))
                setsprite(j,&pos);
            j = nextj;
        }
    }
}


void G_Thunder(void)
{
    static int32_t brightness;
    int j;
    int startWall, endWall;
    uint8_t shade;
    bsize_t i = 0;
    if (!g_thunderFlash)
    {
        if ((gotpic[RRTILE2577>>3]&(1<<(RRTILE2577&7))))
        {
            gotpic[RRTILE2577>>3] &= ~(1<<(RRTILE2577&7));
            if (tilePtr(RRTILE2577) != nullptr)	// why does this on texture load state???
            {
                g_visibility = 256;
                if (krand2() > 65000)
                {
                    g_thunderTime = 256;
                    g_thunderFlash = 1;
                    S_PlaySound(351+(rand()%3));
                }
            }
        }
        else
        {
            brightness = 0;
            g_visibility = g_player[screenpeek].ps->visibility;
        }
    }
    else
    {
        g_thunderTime -= 4;
        if (g_thunderTime < 0)
        {
            brightness = 0;
            g_thunderFlash = 0;
            videoSetPalette(0,g_player[screenpeek].ps->palette,Pal_SceneBrightness);
            g_visibility = g_player[screenpeek].ps->visibility;
        }
    }
    if (!g_winderFlash)
    {
        if ((gotpic[RRTILE2562>>3]&(1<<(RRTILE2562&7))))
        {
            gotpic[RRTILE2562>>3] &= ~(1<<(RRTILE2562&7));
            if (tilePtr(RRTILE2562) != nullptr)	// why does this on texture load state???
            {
                if (krand2() > 65000)
                {
                    g_winderTime = 128;
                    g_winderFlash = 1;
                    S_PlaySound(351+(rand()%3));
                }
            }
        }
    }
    else
    {
        g_winderFlash -= 4;
        if (g_winderTime < 0)
        {
            g_winderFlash = 0;
            for (i = 0; i < g_lightninCnt; i++)
            {
                sector[g_lightninSector[i]].floorshade = g_lightninSectorShade[i];
                sector[g_lightninSector[i]].ceilingshade = g_lightninSectorShade[i];
                startWall = sector[g_lightninSector[i]].wallptr;
                endWall = startWall + sector[g_lightninSector[i]].wallnum - 1;
                for (j = startWall; j <= endWall; j++)
                    wall[j].shade = g_lightninSectorShade[i];
            }
        }
    }
    if (g_thunderFlash == 1)
    {
        brightness += krand2()&4;
        g_visibility = 2048;
        if (brightness > 8)
            brightness = 0;
        videoSetPalette(brightness,g_player[screenpeek].ps->palette,Pal_SceneBrightness);
    }
    if (g_winderFlash == 1)
    {
        if (i >= MAXTORCHSECTORS)
            i = MAXTORCHSECTORS - 1;
        shade = g_torchSectorShade[i]+(krand2()&8);
        for (i = 0; i < g_lightninCnt; i++)
        {
            sector[g_lightninSector[i]].floorshade = g_lightninSectorShade[i] - shade;
            sector[g_lightninSector[i]].ceilingshade = g_lightninSectorShade[i] - shade;
            startWall = sector[g_lightninSector[i]].wallptr;
            endWall = startWall + sector[g_lightninSector[i]].wallnum - 1;
            for (j = startWall; j <= endWall; j++)
                wall[j].shade = g_lightninSectorShade[i] - shade;
        }
    }
}

END_RR_NS
