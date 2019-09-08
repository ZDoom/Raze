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

#include "duke3d.h"
#include "demo.h"
#include "enet/enet.h"

#ifdef __ANDROID__
#include "android.h"
#endif

int32_t lastvisinc;
hudweapon_t hudweap;

#ifdef SPLITSCREEN_MOD_HACKS
static int32_t g_snum;
#endif

extern int32_t g_levelTextTime, ticrandomseed;

int32_t g_numObituaries = 0;
int32_t g_numSelfObituaries = 0;


int const icon_to_inv[ICON_MAX] = { GET_FIRSTAID, GET_FIRSTAID, GET_STEROIDS, GET_HOLODUKE,
                                    GET_JETPACK,  GET_HEATS,    GET_SCUBA,    GET_BOOTS };

int const inv_to_icon[GET_MAX] = { ICON_STEROIDS, ICON_NONE,  ICON_SCUBA, ICON_HOLODUKE, ICON_JETPACK, ICON_NONE,
                                   ICON_NONE,     ICON_HEATS, ICON_NONE,  ICON_FIRSTAID, ICON_BOOTS };

void P_AddKills(DukePlayer_t * const pPlayer, uint16_t kills)
{
    pPlayer->actors_killed += kills;
}

void P_UpdateScreenPal(DukePlayer_t * const pPlayer)
{
    int       inWater       = 0;
    int const playerSectnum = pPlayer->cursectnum;

    if (pPlayer->heat_on)
        pPlayer->palette = SLIMEPAL;
    else if (playerSectnum < 0)
        pPlayer->palette = BASEPAL;
    else if (sector[playerSectnum].ceilingpicnum >= FLOORSLIME && sector[playerSectnum].ceilingpicnum <= FLOORSLIME + 2)
    {
        pPlayer->palette = SLIMEPAL;
        inWater          = 1;
    }
    else
    {
        pPlayer->palette     = (sector[pPlayer->cursectnum].lotag == ST_2_UNDERWATER) ? WATERPAL : BASEPAL;
        inWater              = 1;
    }

    g_restorePalette = 1+inWater;
}

static void P_IncurDamage(DukePlayer_t * const pPlayer)
{
    if (VM_OnEvent(EVENT_INCURDAMAGE, pPlayer->i, P_Get(pPlayer->i)) != 0)
        return;

    sprite[pPlayer->i].extra -= pPlayer->extra_extra8>>8;

    int playerDamage = sprite[pPlayer->i].extra - pPlayer->last_extra;

    if (playerDamage >= 0)
        return;

    pPlayer->extra_extra8 = 0;

    if (pPlayer->inv_amount[GET_SHIELD] > 0)
    {
        int const shieldDamage = playerDamage * (20 + (krand()%30)) / 100;

        playerDamage                     -= shieldDamage;
        pPlayer->inv_amount[GET_SHIELD] += shieldDamage;

        if (pPlayer->inv_amount[GET_SHIELD] < 0)
        {
            playerDamage += pPlayer->inv_amount[GET_SHIELD];
            pPlayer->inv_amount[GET_SHIELD] = 0;
        }
    }

    sprite[pPlayer->i].extra = pPlayer->last_extra + playerDamage;
}

void P_QuickKill(DukePlayer_t * const pPlayer)
{
    P_PalFrom(pPlayer, 48, 48,48,48);

    sprite[pPlayer->i].extra = 0;
    sprite[pPlayer->i].cstat |= 32768;

#ifndef EDUKE32_STANDALONE
    if (!FURY && ud.god == 0)
        A_DoGuts(pPlayer->i,JIBS6,8);
#endif
}

static void Proj_DoWaterTracers(vec3_t startPos, vec3_t const *endPos, int n, int16_t sectNum)
{
    if ((klabs(startPos.x - endPos->x) + klabs(startPos.y - endPos->y)) < 3084)
        return;

    vec3_t const v_inc = { tabledivide32_noinline(endPos->x - startPos.x, n + 1), tabledivide32_noinline(endPos->y - startPos.y, n + 1),
                           tabledivide32_noinline(endPos->z - startPos.z, n + 1) };

    for (bssize_t i=n; i>0; i--)
    {
        startPos.x += v_inc.x;
        startPos.y += v_inc.y;
        startPos.z += v_inc.z;

        updatesector(startPos.x, startPos.y, &sectNum);

        if (sectNum < 0)
            break;

        A_InsertSprite(sectNum, startPos.x, startPos.y, startPos.z, WATERBUBBLE, -32, 4 + (krand() & 3), 4 + (krand() & 3), krand() & 2047, 0, 0,
                       g_player[0].ps->i, 5);
    }
}

static inline projectile_t *Proj_GetProjectile(int tile)
{
    return ((unsigned)tile < MAXTILES && g_tile[tile].proj) ? g_tile[tile].proj : &DefaultProjectile;
}

static void A_HitscanProjTrail(const vec3_t *startPos, const vec3_t *endPos, int projAng, int tileNum, int16_t sectNum)
{
    const projectile_t *const pProj = Proj_GetProjectile(tileNum);

    vec3_t        spawnPos = { startPos->x + tabledivide32_noinline(sintable[(348 + projAng + 512) & 2047], pProj->offset),
                               startPos->y + tabledivide32_noinline(sintable[(projAng + 348) & 2047], pProj->offset),
                               startPos->z + 1024 + (pProj->toffset << 8) };

    int32_t      n         = ((FindDistance2D(spawnPos.x - endPos->x, spawnPos.y - endPos->y)) >> 8) + 1;

    vec3_t const increment = { tabledivide32_noinline((endPos->x - spawnPos.x), n),
                               tabledivide32_noinline((endPos->y - spawnPos.y), n),
                               tabledivide32_noinline((endPos->z - spawnPos.z), n) };

    spawnPos.x += increment.x >> 2;
    spawnPos.y += increment.y >> 2;
    spawnPos.z += increment.z >> 2;

    int32_t j;

    for (bssize_t i = pProj->tnum; i > 0; --i)
    {
        spawnPos.x += increment.x;
        spawnPos.y += increment.y;
        spawnPos.z += increment.z;

        updatesectorz(spawnPos.x, spawnPos.y, spawnPos.z, &sectNum);

        if (sectNum < 0)
            break;

        getzsofslope(sectNum, spawnPos.x, spawnPos.y, &n, &j);

        if (spawnPos.z > j || spawnPos.z < n)
            break;

        j = A_InsertSprite(sectNum, spawnPos.x, spawnPos.y, spawnPos.z, pProj->trail, -32,
                           pProj->txrepeat, pProj->tyrepeat, projAng, 0, 0, g_player[0].ps->i, 0);
        changespritestat(j, STAT_ACTOR);
    }
}

int32_t A_GetHitscanRange(int spriteNum)
{
    int const zOffset = (PN(spriteNum) == APLAYER) ? PHEIGHT : 0;
    hitdata_t hitData;

    SZ(spriteNum) -= zOffset;
    hitscan((const vec3_t *)&sprite[spriteNum], SECT(spriteNum), sintable[(SA(spriteNum) + 512) & 2047],
            sintable[SA(spriteNum) & 2047], 0, &hitData, CLIPMASK1);
    SZ(spriteNum) += zOffset;

    return (FindDistance2D(hitData.pos.x - SX(spriteNum), hitData.pos.y - SY(spriteNum)));
}

static int A_FindTargetSprite(const spritetype *pSprite, int projAng, int projecTile)
{
    static int const aimstats[] = {
        STAT_PLAYER, STAT_DUMMYPLAYER, STAT_ACTOR, STAT_ZOMBIEACTOR
    };

    int const playerNum = pSprite->picnum == APLAYER ? P_GetP(pSprite) : -1;

    if (playerNum != -1)
    {
        if (!g_player[playerNum].ps->auto_aim)
            return -1;

        if (g_player[playerNum].ps->auto_aim == 2)
        {
            if (A_CheckSpriteTileFlags(projecTile,SFLAG_PROJECTILE) && (Proj_GetProjectile(projecTile)->workslike & PROJECTILE_RPG))
                return -1;

#ifndef EDUKE32_STANDALONE
            if (!FURY)
            {
                switch (DYNAMICTILEMAP(projecTile))
                {
                    case TONGUE__STATIC:
                    case FREEZEBLAST__STATIC:
                    case SHRINKSPARK__STATIC:
                    case SHRINKER__STATIC:
                    case RPG__STATIC:
                    case FIRELASER__STATIC:
                    case SPIT__STATIC:
                    case COOLEXPLOSION1__STATIC:
                        return -1;
                    default:
                        break;
                }
            }
#endif
        }
    }

    int const spriteAng = pSprite->ang;

#ifndef EDUKE32_STANDALONE
    int const isShrinker = (pSprite->picnum == APLAYER && PWEAPON(playerNum, g_player[playerNum].ps->curr_weapon, WorksLike) == SHRINKER_WEAPON);
    int const isFreezer  = (pSprite->picnum == APLAYER && PWEAPON(playerNum, g_player[playerNum].ps->curr_weapon, WorksLike) == FREEZE_WEAPON);
#endif

    vec2_t const d1 = { sintable[(spriteAng + 512 - projAng) & 2047], sintable[(spriteAng - projAng) & 2047] };
    vec2_t const d2 = { sintable[(spriteAng + 512 + projAng) & 2047], sintable[(spriteAng + projAng) & 2047] };
    vec2_t const d3 = { sintable[(spriteAng + 512) & 2047], sintable[spriteAng & 2047] };

    int lastDist   = INT32_MAX;
    int bestSprite = -1;

    for (bssize_t k=0; k<4; k++)
    {
        if (bestSprite >= 0)
            break;

        for (bssize_t spriteNum=headspritestat[aimstats[k]]; spriteNum >= 0; spriteNum=nextspritestat[spriteNum])
        {
            if ((sprite[spriteNum].xrepeat > 0 && sprite[spriteNum].extra >= 0 &&
                 (sprite[spriteNum].cstat & (257 + 32768)) == 257) &&
                (A_CheckEnemySprite(&sprite[spriteNum]) || k < 2))
            {
                if (A_CheckEnemySprite(&sprite[spriteNum]) || PN(spriteNum) == APLAYER)
                {
                    if (PN(spriteNum) == APLAYER && pSprite->picnum == APLAYER && pSprite != &sprite[spriteNum] &&
                        (GTFLAGS(GAMETYPE_PLAYERSFRIENDLY) ||
                         (GTFLAGS(GAMETYPE_TDM) && g_player[P_Get(spriteNum)].ps->team == g_player[playerNum].ps->team)))
                        continue;

#ifndef EDUKE32_STANDALONE
                    if (!FURY && ((isShrinker && sprite[spriteNum].xrepeat < 30
                        && (PN(spriteNum) == SHARK || !(PN(spriteNum) >= GREENSLIME && PN(spriteNum) <= GREENSLIME + 7)))
                        || (isFreezer && sprite[spriteNum].pal == 1)))
                        continue;
#endif
                }

                vec2_t const vd = { (SX(spriteNum) - pSprite->x), (SY(spriteNum) - pSprite->y) };

                if ((d1.y * vd.x <= d1.x * vd.y) && (d2.y * vd.x >= d2.x * vd.y))
                {
                    int const spriteDist = mulscale14(d3.x, vd.x) + mulscale14(d3.y, vd.y);

                    if (spriteDist > 512 && spriteDist < lastDist)
                    {
                        int onScreen = 1;

                        if (pSprite->picnum == APLAYER)
                        {
                            auto const ps = g_player[P_GetP(pSprite)].ps;
                            onScreen = (klabs(scale(SZ(spriteNum)-pSprite->z,10,spriteDist)-fix16_to_int(ps->q16horiz+ps->q16horizoff-F16(100))) < 100);
                        }

#ifndef EDUKE32_STANDALONE
                        int const zOffset = (!FURY && (PN(spriteNum) == ORGANTIC || PN(spriteNum) == ROTATEGUN)) ? 0 : ZOFFSET5;
#else
                        int const zOffset = ZOFFSET5;
#endif
                        int const canSee = cansee(SX(spriteNum), SY(spriteNum), SZ(spriteNum) - zOffset, SECT(spriteNum),
                                                  pSprite->x, pSprite->y, pSprite->z - ZOFFSET5, pSprite->sectnum);

                        if (onScreen && canSee)
                        {
                            lastDist   = spriteDist;
                            bestSprite = spriteNum;
                        }
                    }
                }
            }
        }
    }

    return bestSprite;
}

static void A_SetHitData(int spriteNum, const hitdata_t *hitData)
{
    actor[spriteNum].t_data[6] = hitData->wall;
    actor[spriteNum].t_data[7] = hitData->sect;
    actor[spriteNum].t_data[8] = hitData->sprite;
}

#ifndef EDUKE32_STANDALONE
static int CheckShootSwitchTile(int tileNum)
{
    if (FURY)
        return 0;

    return tileNum == DIPSWITCH || tileNum == DIPSWITCH + 1 || tileNum == DIPSWITCH2 || tileNum == DIPSWITCH2 + 1 ||
           tileNum == DIPSWITCH3 || tileNum == DIPSWITCH3 + 1 || tileNum == HANDSWITCH || tileNum == HANDSWITCH + 1;
}
#endif

static int32_t safeldist(int32_t spriteNum, const void *pSprite)
{
    int32_t distance = ldist(&sprite[spriteNum], pSprite);
    return distance ? distance : 1;
}

// flags:
//  1: do sprite center adjustment (cen-=(8<<8)) for GREENSLIME or ROTATEGUN
//  2: do auto getangle only if not RECON (if clear, do unconditionally)
static int GetAutoAimAng(int spriteNum, int playerNum, int projecTile, int zAdjust, int aimFlags,
                               const vec3_t *startPos, int projVel, int32_t *pZvel, int *pAng)
{
    int returnSprite = -1;

    Bassert((unsigned)playerNum < MAXPLAYERS);

#ifdef LUNATIC
    g_player[playerNum].ps->autoaimang = g_player[playerNum].ps->auto_aim == 3 ? AUTO_AIM_ANGLE<<1 : AUTO_AIM_ANGLE;
#else
    Gv_SetVar(g_aimAngleVarID, g_player[playerNum].ps->auto_aim == 3 ? AUTO_AIM_ANGLE<<1 : AUTO_AIM_ANGLE, spriteNum, playerNum);
#endif

    VM_OnEvent(EVENT_GETAUTOAIMANGLE, spriteNum, playerNum);

#ifdef LUNATIC
    int aimang = g_player[playerNum].ps->autoaimang;
#else
    int aimang = Gv_GetVar(g_aimAngleVarID, spriteNum, playerNum);
#endif
    if (aimang > 0)
        returnSprite = A_FindTargetSprite(&sprite[spriteNum], aimang, projecTile);

    if (returnSprite >= 0)
    {
        auto const pSprite = (uspriteptr_t)&sprite[returnSprite];
        int        zCenter = 2 * (pSprite->yrepeat * tilesiz[pSprite->picnum].y) + zAdjust;

#ifndef EDUKE32_STANDALONE
        if (!FURY && aimFlags &&
            ((pSprite->picnum >= GREENSLIME && pSprite->picnum <= GREENSLIME + 7) || pSprite->picnum == ROTATEGUN || pSprite->cstat & CSTAT_SPRITE_YCENTER))
#else
        if (aimFlags && pSprite->cstat & CSTAT_SPRITE_YCENTER)
#endif
            zCenter -= ZOFFSET3;

        int spriteDist = safeldist(g_player[playerNum].ps->i, &sprite[returnSprite]);
        *pZvel         = tabledivide32_noinline((pSprite->z - startPos->z - zCenter) * projVel, spriteDist);

        if (!(aimFlags&2) || sprite[returnSprite].picnum != RECON)
            *pAng = getangle(pSprite->x-startPos->x, pSprite->y-startPos->y);
    }

    return returnSprite;
}

static void Proj_MaybeSpawn(int spriteNum, int projecTile, const hitdata_t *hitData)
{
    // atwith < 0 is for hard-coded projectiles
    projectile_t *const pProj      = Proj_GetProjectile(projecTile);
    int                 spawnTile  = projecTile < 0 ? -projecTile : pProj->spawns;

    if (spawnTile >= 0)
    {
        int spawned = A_Spawn(spriteNum, spawnTile);

        if (projecTile >= 0)
        {
            if (pProj->sxrepeat > 4)
                sprite[spawned].xrepeat = pProj->sxrepeat;

            if (pProj->syrepeat > 4)
                sprite[spawned].yrepeat = pProj->syrepeat;
        }

        A_SetHitData(spawned, hitData);
    }
}

// <extra>: damage that this shotspark does
static int Proj_InsertShotspark(const hitdata_t *hitData, int spriteNum, int projecTile, int sparkSize, int sparkAng, int damage)
{
    int returnSprite = A_InsertSprite(hitData->sect, hitData->pos.x, hitData->pos.y, hitData->pos.z, SHOTSPARK1, -15,
                                     sparkSize, sparkSize, sparkAng, 0, 0, spriteNum, 4);

    sprite[returnSprite].extra = damage;
    sprite[returnSprite].yvel  = projecTile;  // This is a hack to allow you to detect which weapon spawned a SHOTSPARK1

    A_SetHitData(returnSprite, hitData);

    return returnSprite;
}

int Proj_GetDamage(projectile_t const *pProj)
{
    Bassert(pProj);

    int damage = pProj->extra;

    if (pProj->extra_rand > 0)
        damage += (krand() % pProj->extra_rand);

    return damage;
}

static void Proj_MaybeAddSpread(int doSpread, int32_t *zvel, int *shootAng, int zRange, int angRange)
{
    if (doSpread)
    {
        // Ranges <= 1 mean no spread at all. A range of 1 calls krand() though.
        if (zRange > 0)
            *zvel += (zRange >> 1) - krand() % zRange;

        if (angRange > 0)
            *shootAng += (angRange >> 1) - krand() % angRange;
    }
}

static int g_overrideShootZvel = 0;  // a boolean
static int g_shootZvel;  // the actual zvel if the above is !=0

static int A_GetShootZvel(int defaultZvel)
{
    return g_overrideShootZvel ? g_shootZvel : defaultZvel;
}

// Prepare hitscan weapon fired from player p.
static void P_PreFireHitscan(int spriteNum, int playerNum, int projecTile, vec3_t *srcVect, int32_t *zvel, int *shootAng,
                             int accurateAim, int doSpread)
{
    int angRange  = 32;
    int zRange    = 256;
    int aimSprite = GetAutoAimAng(spriteNum, playerNum, projecTile, 5 << 8, 0 + 1, srcVect, 256, zvel, shootAng);

    auto const pPlayer = g_player[playerNum].ps;

#ifdef LUNATIC
    pPlayer->angrange = angRange;
    pPlayer->zrange = zRange;
#else
    Gv_SetVar(g_angRangeVarID, angRange, spriteNum, playerNum);
    Gv_SetVar(g_zRangeVarID, zRange, spriteNum, playerNum);
#endif

    VM_OnEvent(EVENT_GETSHOTRANGE, spriteNum, playerNum);

#ifdef LUNATIC
    angRange = pPlayer->angrange;
    zRange   = pPlayer->zrange;
#else
    angRange = Gv_GetVar(g_angRangeVarID, spriteNum, playerNum);
    zRange   = Gv_GetVar(g_zRangeVarID, spriteNum, playerNum);
#endif

    if (accurateAim)
    {
        if (!pPlayer->auto_aim)
        {
            hitdata_t hitData;

            *zvel = A_GetShootZvel(fix16_to_int(F16(100)-pPlayer->q16horiz-pPlayer->q16horizoff)<<5);

            hitscan(srcVect, sprite[spriteNum].sectnum, sintable[(*shootAng + 512) & 2047],
                    sintable[*shootAng & 2047], *zvel << 6, &hitData, CLIPMASK1);

            if (hitData.sprite != -1)
            {
                int const statNumMap = ((1 << STAT_ACTOR) | (1 << STAT_ZOMBIEACTOR) | (1 << STAT_PLAYER) | (1 << STAT_DUMMYPLAYER));
                int const statNum    = sprite[hitData.sprite].statnum;

                if ((unsigned)statNum <= 30 && (statNumMap & (1 << statNum)))
                    aimSprite = hitData.sprite;
            }
        }

        if (aimSprite == -1)
            goto notarget;
    }
    else
    {
        if (aimSprite == -1)  // no target
        {
notarget:
            *zvel = fix16_to_int(F16(100)-pPlayer->q16horiz-pPlayer->q16horizoff)<<5;
        }

        Proj_MaybeAddSpread(doSpread, zvel, shootAng, zRange, angRange);
    }

    // ZOFFSET6 is added to this position at the same time as the player's pyoff in A_ShootWithZvel()
    srcVect->z -= ZOFFSET6;
}

// Hitscan weapon fired from actor (sprite s);
static void A_PreFireHitscan(const spritetype *pSprite, vec3_t * const srcVect, int32_t * const zvel, int * const shootAng, int const doSpread)
{
    int const  playerNum  = A_FindPlayer(pSprite, NULL);
    auto const pPlayer    = g_player[playerNum].ps;
    int const  playerDist = safeldist(pPlayer->i, pSprite);

    *zvel = tabledivide32_noinline((pPlayer->pos.z - srcVect->z) << 8, playerDist);

    srcVect->z -= ZOFFSET6;

    if (pSprite->picnum == BOSS1)
        *shootAng = getangle(pPlayer->pos.x - srcVect->x, pPlayer->pos.y - srcVect->y);

    Proj_MaybeAddSpread(doSpread, zvel, shootAng, 256, 128 >> (uint8_t)(pSprite->picnum != BOSS1));
}

static int Proj_DoHitscan(int spriteNum, int32_t const cstatmask, const vec3_t * const srcVect, int zvel, int const shootAng, hitdata_t * const hitData)
{
    auto const pSprite = &sprite[spriteNum];

    pSprite->cstat &= ~cstatmask;
    zvel = A_GetShootZvel(zvel);
    hitscan(srcVect, pSprite->sectnum, sintable[(shootAng + 512) & 2047], sintable[shootAng & 2047], zvel << 6, hitData, CLIPMASK1);
    pSprite->cstat |= cstatmask;

    return (hitData->sect < 0);
}

static void Proj_DoRandDecalSize(int const spriteNum, int const projecTile)
{
    const projectile_t *const proj    = Proj_GetProjectile(projecTile);
    auto const         pSprite = &sprite[spriteNum];

    if (proj->workslike & PROJECTILE_RANDDECALSIZE)
        pSprite->xrepeat = pSprite->yrepeat = clamp((krand() & proj->xrepeat), pSprite->yrepeat, pSprite->xrepeat);
    else
    {
        pSprite->xrepeat = proj->xrepeat;
        pSprite->yrepeat = proj->yrepeat;
    }
}

static int SectorContainsSE13(int const sectNum)
{
    if (sectNum >= 0)
    {
        for (bssize_t SPRITES_OF_SECT(sectNum, i))
        {
            if (sprite[i].statnum == STAT_EFFECTOR && sprite[i].lotag == SE_13_EXPLOSIVE)
                return 1;
        }
    }
    return 0;
}

// Maybe handle bit 2 (swap wall bottoms).
// (in that case walltype *hitwal may be stale)
static inline void HandleHitWall(hitdata_t *hitData)
{
    auto const hitWall = (uwallptr_t)&wall[hitData->wall];

    if ((hitWall->cstat & 2) && redwallp(hitWall) && (hitData->pos.z >= sector[hitWall->nextsector].floorz))
        hitData->wall = hitWall->nextwall;
}

// Maybe damage a ceiling or floor as the consequence of projectile impact.
// Returns 1 if projectile hit a parallaxed ceiling.
// NOTE: Compare with Proj_MaybeDamageCF() in actors.c
static int Proj_MaybeDamageCF2(int const spriteNum, int const zvel, int const hitSect)
{
    Bassert(hitSect >= 0);

    if (zvel < 0)
    {
        if (sector[hitSect].ceilingstat&1)
            return 1;

        Sect_DamageCeiling(spriteNum, hitSect);
    }
    else if (zvel > 0)
    {
        if (sector[hitSect].floorstat&1)
        {
            // Keep original Duke3D behavior: pass projectiles through
            // parallaxed ceilings, but NOT through such floors.
            return 0;
        }

        Sect_DamageFloor(spriteNum, hitSect);
    }

    return 0;
}

// Finish shooting hitscan weapon from player <p>. <k> is the inserted SHOTSPARK1.
// * <spawnObject> is passed to Proj_MaybeSpawn()
// * <decalTile> and <wallDamage> are for wall impact
// * <wallDamage> is passed to A_DamageWall()
// * <decalFlags> is for decals upon wall impact:
//    1: handle random decal size (tile <atwith>)
//    2: set cstat to wall-aligned + random x/y flip
//
// TODO: maybe split into 3 cases (hit neither wall nor sprite, hit sprite, hit wall)?
static int P_PostFireHitscan(int playerNum, int const spriteNum, hitdata_t *const hitData, int const spriteOwner,
                             int const projecTile, int const zvel, int const spawnTile, int const decalTile, int const wallDamage,
                             int const decalFlags)
{
#ifdef EDUKE32_STANDALONE
    UNREFERENCED_PARAMETER(playerNum);
#endif
    if (hitData->wall == -1 && hitData->sprite == -1)
    {
        if (Proj_MaybeDamageCF2(spriteNum, zvel, hitData->sect))
        {
            sprite[spriteNum].xrepeat = 0;
            sprite[spriteNum].yrepeat = 0;
            return -1;
        }

        Proj_MaybeSpawn(spriteNum, spawnTile, hitData);
    }
    else if (hitData->sprite >= 0)
    {
        A_DamageObject(hitData->sprite, spriteNum);

        if (!FURY && sprite[hitData->sprite].picnum == APLAYER &&
            (ud.ffire == 1 || (!GTFLAGS(GAMETYPE_PLAYERSFRIENDLY) && GTFLAGS(GAMETYPE_TDM) &&
                               g_player[P_Get(hitData->sprite)].ps->team != g_player[P_Get(spriteOwner)].ps->team)))
        {
#ifndef EDUKE32_STANDALONE
            int jibSprite = A_Spawn(spriteNum, JIBS6);

            sprite[spriteNum].xrepeat = sprite[spriteNum].yrepeat = 0;
            sprite[jibSprite].z += ZOFFSET6;
            sprite[jibSprite].xvel    = 16;
            sprite[jibSprite].xrepeat = sprite[jibSprite].yrepeat = 24;
            sprite[jibSprite].ang += 64 - (krand() & 127);
#endif
        }
        else
        {
            Proj_MaybeSpawn(spriteNum, spawnTile, hitData);
        }
#ifndef EDUKE32_STANDALONE
        if (!FURY && playerNum >= 0 && CheckShootSwitchTile(sprite[hitData->sprite].picnum))
        {
            P_ActivateSwitch(playerNum, hitData->sprite, 1);
            return -1;
        }
#endif
    }
    else if (hitData->wall >= 0)
    {
        auto const hitWall = (uwallptr_t)&wall[hitData->wall];

        Proj_MaybeSpawn(spriteNum, spawnTile, hitData);

        if (CheckDoorTile(hitWall->picnum) == 1)
            goto SKIPBULLETHOLE;

#ifndef EDUKE32_STANDALONE
        if (!FURY && playerNum >= 0 && CheckShootSwitchTile(hitWall->picnum))
        {
            P_ActivateSwitch(playerNum, hitData->wall, 0);
            return -1;
        }
#endif

        if (hitWall->hitag != 0 || (hitWall->nextwall >= 0 && wall[hitWall->nextwall].hitag != 0))
            goto SKIPBULLETHOLE;

        if ((hitData->sect >= 0 && sector[hitData->sect].lotag == 0) &&
            (hitWall->overpicnum != BIGFORCE && (hitWall->cstat & 16) == 0) &&
            ((hitWall->nextsector >= 0 && sector[hitWall->nextsector].lotag == 0) || (hitWall->nextsector == -1 && sector[hitData->sect].lotag == 0)))
        {
            int decalSprite;

            if (SectorContainsSE13(hitWall->nextsector))
                goto SKIPBULLETHOLE;

            for (SPRITES_OF(STAT_MISC, decalSprite))
                if (sprite[decalSprite].picnum == decalTile && dist(&sprite[decalSprite], &sprite[spriteNum]) < (12 + (krand() & 7)))
                    goto SKIPBULLETHOLE;

            if (decalTile >= 0)
            {
                decalSprite = A_Spawn(spriteNum, decalTile);

                auto const decal = &sprite[decalSprite];

                A_SetHitData(decalSprite, hitData);

                if (!A_CheckSpriteFlags(decalSprite, SFLAG_DECAL))
                    actor[decalSprite].flags |= SFLAG_DECAL;

                int32_t diffZ;
                spriteheightofs(decalSprite, &diffZ, 0);

                decal->z += diffZ >> 1;
                decal->ang = (getangle(hitWall->x - wall[hitWall->point2].x, hitWall->y - wall[hitWall->point2].y) + 1536) & 2047;

                if (decalFlags & 1)
                    Proj_DoRandDecalSize(decalSprite, projecTile);

                if (decalFlags & 2)
                    decal->cstat = 16 + (krand() & (8 + 4));

                A_SetSprite(decalSprite, CLIPMASK0);

                // BULLETHOLE already adds itself to the deletion queue in
                // A_Spawn(). However, some other tiles do as well.
                if (decalTile != BULLETHOLE)
                    A_AddToDeleteQueue(decalSprite);
            }
        }

SKIPBULLETHOLE:
        HandleHitWall(hitData);
        A_DamageWall(spriteNum, hitData->wall, hitData->pos, wallDamage);
    }

    return 0;
}

// Finish shooting hitscan weapon from actor (sprite <i>).
static int A_PostFireHitscan(const hitdata_t *hitData, int const spriteNum, int const projecTile, int const zvel, int const shootAng,
                             int const extra, int const spawnTile, int const wallDamage)
{
    int const returnSprite = Proj_InsertShotspark(hitData, spriteNum, projecTile, 24, shootAng, extra);

    if (hitData->sprite >= 0)
    {
        A_DamageObject(hitData->sprite, returnSprite);

        if (sprite[hitData->sprite].picnum != APLAYER)
            Proj_MaybeSpawn(returnSprite, spawnTile, hitData);
        else
            sprite[returnSprite].xrepeat = sprite[returnSprite].yrepeat = 0;
    }
    else if (hitData->wall >= 0)
    {
        A_DamageWall(returnSprite, hitData->wall, hitData->pos, wallDamage);
        Proj_MaybeSpawn(returnSprite, spawnTile, hitData);
    }
    else
    {
        if (Proj_MaybeDamageCF2(returnSprite, zvel, hitData->sect))
        {
            sprite[returnSprite].xrepeat = 0;
            sprite[returnSprite].yrepeat = 0;
        }
        else Proj_MaybeSpawn(returnSprite, spawnTile, hitData);
    }

    return returnSprite;
}

// Common "spawn blood?" predicate.
// minzdiff: minimal "step" height for blood to be spawned
static int Proj_CheckBlood(vec3_t const *const srcVect, hitdata_t const *const hitData, int const bloodRange, int const minZdiff)
{
    if (hitData->wall < 0 || hitData->sect < 0)
        return 0;

    auto const hitWall = (uwallptr_t)&wall[hitData->wall];

    if ((FindDistance2D(srcVect->x - hitData->pos.x, srcVect->y - hitData->pos.y) < bloodRange)
        && (hitWall->overpicnum != BIGFORCE && (hitWall->cstat & 16) == 0)
        && (sector[hitData->sect].lotag == 0)
        && (hitWall->nextsector < 0 || (sector[hitWall->nextsector].lotag == 0 && sector[hitData->sect].lotag == 0
                                        && sector[hitData->sect].floorz - sector[hitWall->nextsector].floorz > minZdiff)))
        return 1;

    return 0;
}

static void Proj_HandleKnee(hitdata_t *const hitData, int const spriteNum, int const playerNum, int const projecTile, int const shootAng,
                            const projectile_t *const proj, int const inserttile, int const randomDamage, int const spawnTile,
                            int const soundNum)
{
    auto const pPlayer = playerNum >= 0 ? g_player[playerNum].ps : NULL;

    int kneeSprite = A_InsertSprite(hitData->sect,hitData->pos.x,hitData->pos.y,hitData->pos.z,
                                    inserttile,-15,0,0,shootAng,32,0,spriteNum,4);

    if (proj != NULL)
    {
        // Custom projectiles.
        SpriteProjectile[kneeSprite].workslike = Proj_GetProjectile(sprite[kneeSprite].picnum)->workslike;
        sprite[kneeSprite].extra = proj->extra;
    }

    if (randomDamage > 0)
        sprite[kneeSprite].extra += (krand()&randomDamage);

    if (playerNum >= 0)
    {
        if (spawnTile >= 0)
        {
            int k = A_Spawn(kneeSprite, spawnTile);
            sprite[k].z -= ZOFFSET3;
            A_SetHitData(k, hitData);
        }

        if (soundNum >= 0)
            A_PlaySound(soundNum, kneeSprite);
    }

    if (pPlayer != NULL && pPlayer->inv_amount[GET_STEROIDS] > 0 && pPlayer->inv_amount[GET_STEROIDS] < 400)
        sprite[kneeSprite].extra += (pPlayer->max_player_health>>2);

    if (hitData->sprite >= 0 && sprite[hitData->sprite].picnum != ACCESSSWITCH && sprite[hitData->sprite].picnum != ACCESSSWITCH2)
    {
        A_DamageObject(hitData->sprite, kneeSprite);
        if (playerNum >= 0)
            P_ActivateSwitch(playerNum, hitData->sprite,1);
    }
    else if (hitData->wall >= 0)
    {
        HandleHitWall(hitData);

        if (wall[hitData->wall].picnum != ACCESSSWITCH && wall[hitData->wall].picnum != ACCESSSWITCH2)
        {
            A_DamageWall(kneeSprite, hitData->wall, hitData->pos, projecTile);
            if (playerNum >= 0)
                P_ActivateSwitch(playerNum, hitData->wall,0);
        }
    }
}

#define MinibossScale(i, s) (((s)*sprite[i].yrepeat)/80)

static int A_ShootCustom(int const spriteNum, int const projecTile, int shootAng, vec3_t * const startPos)
{
    /* Custom projectiles */
    hitdata_t           hitData;
    projectile_t *const pProj     = Proj_GetProjectile(projecTile);
    auto const   pSprite   = &sprite[spriteNum];
    int const           playerNum = (pSprite->picnum == APLAYER) ? P_GetP(pSprite) : -1;
    auto const pPlayer   = playerNum >= 0 ? g_player[playerNum].ps : NULL;

#ifdef POLYMER
    if (videoGetRenderMode() == REND_POLYMER && pProj->flashcolor)
    {
        int32_t x = ((sintable[(pSprite->ang + 512) & 2047]) >> 7), y = ((sintable[(pSprite->ang) & 2047]) >> 7);

        pSprite->x += x;
        pSprite->y += y;
        G_AddGameLight(0, spriteNum, PHEIGHT, 8192, pProj->flashcolor, PR_LIGHT_PRIO_MAX_GAME);
        actor[spriteNum].lightcount = 2;
        pSprite->x -= x;
        pSprite->y -= y;
    }
#endif // POLYMER

    if (pProj->offset == 0)
        pProj->offset = 1;

    int     otherSprite = -1;
    int32_t zvel = 0;

    switch (pProj->workslike & PROJECTILE_TYPE_MASK)
    {
    case PROJECTILE_HITSCAN:
        if (!(pProj->workslike & PROJECTILE_NOSETOWNERSHADE) && pSprite->extra >= 0)
            pSprite->shade = pProj->shade;

        if (playerNum >= 0)
            P_PreFireHitscan(spriteNum, playerNum, projecTile, startPos, &zvel, &shootAng,
                             pProj->workslike & PROJECTILE_ACCURATE_AUTOAIM, !(pProj->workslike & PROJECTILE_ACCURATE));
        else
            A_PreFireHitscan(pSprite, startPos, &zvel, &shootAng, !(pProj->workslike & PROJECTILE_ACCURATE));

        if (Proj_DoHitscan(spriteNum, (pProj->cstat >= 0) ? pProj->cstat : 256 + 1, startPos, zvel, shootAng, &hitData))
            return -1;

        if (pProj->range > 0 && klabs(startPos->x - hitData.pos.x) + klabs(startPos->y - hitData.pos.y) > pProj->range)
            return -1;

        if (pProj->trail >= 0)
            A_HitscanProjTrail(startPos, &hitData.pos, shootAng, projecTile, pSprite->sectnum);

        if (pProj->workslike & PROJECTILE_WATERBUBBLES)
        {
            if ((krand() & 15) == 0 && sector[hitData.sect].lotag == ST_2_UNDERWATER)
                Proj_DoWaterTracers(hitData.pos, startPos, 8 - (ud.multimode >> 1), pSprite->sectnum);
        }

        if (playerNum >= 0)
        {
            otherSprite = Proj_InsertShotspark(&hitData, spriteNum, projecTile, 10, shootAng, Proj_GetDamage(pProj));

            if (P_PostFireHitscan(playerNum, otherSprite, &hitData, spriteNum, projecTile, zvel, projecTile, pProj->decal,
                                  projecTile, 1 + 2) < 0)
                return -1;
        }
        else
        {
            otherSprite =
            A_PostFireHitscan(&hitData, spriteNum, projecTile, zvel, shootAng, Proj_GetDamage(pProj), projecTile, projecTile);
        }

        if ((krand() & 255) < 4 && pProj->isound >= 0)
            S_PlaySound3D(pProj->isound, otherSprite, &hitData.pos);

        return -1;

    case PROJECTILE_RPG:
        if (!(pProj->workslike & PROJECTILE_NOSETOWNERSHADE) && pSprite->extra >= 0)
            pSprite->shade = pProj->shade;

        if (pPlayer != NULL)
        {
            // NOTE: j is a SPRITE_INDEX
            otherSprite = GetAutoAimAng(spriteNum, playerNum, projecTile, 8<<8, 0+2, startPos, pProj->vel, &zvel, &shootAng);

            if (otherSprite < 0)
                zvel = fix16_to_int(F16(100)-pPlayer->q16horiz-pPlayer->q16horizoff)*(pProj->vel/8);

            if (pProj->sound >= 0)
                A_PlaySound(pProj->sound, spriteNum);
        }
        else
        {
            if (!(pProj->workslike & PROJECTILE_NOAIM))
            {
                int const otherPlayer     = A_FindPlayer(pSprite, NULL);
                int const otherPlayerDist = safeldist(g_player[otherPlayer].ps->i, pSprite);

                shootAng = getangle(g_player[otherPlayer].ps->opos.x - startPos->x,
                                      g_player[otherPlayer].ps->opos.y - startPos->y);

                zvel = tabledivide32_noinline((g_player[otherPlayer].ps->opos.z - startPos->z) * pProj->vel, otherPlayerDist);

                if (A_CheckEnemySprite(pSprite) && (AC_MOVFLAGS(pSprite, &actor[spriteNum]) & face_player_smart))
                    shootAng = pSprite->ang + (krand() & 31) - 16;
            }
        }

        if (numplayers > 1 && g_netClient) return -1;
        else
        {
            // l may be a SPRITE_INDEX, see above
            int const l = (playerNum >= 0 && otherSprite >= 0) ? otherSprite : -1;

            zvel = A_GetShootZvel(zvel);
            otherSprite = A_InsertSprite(pSprite->sectnum,
                startPos->x + tabledivide32_noinline(sintable[(348 + shootAng + 512) & 2047], pProj->offset),
                startPos->y + tabledivide32_noinline(sintable[(shootAng + 348) & 2047], pProj->offset),
                startPos->z - (1 << 8), projecTile, 0, 14, 14, shootAng, pProj->vel, zvel, spriteNum, 4);

            sprite[otherSprite].extra = Proj_GetDamage(pProj);

            if (!(pProj->workslike & PROJECTILE_BOUNCESOFFWALLS))
                sprite[otherSprite].yvel = l;  // NOT_BOUNCESOFFWALLS_YVEL
            else
            {
                sprite[otherSprite].yvel = (pProj->bounces >= 1) ? pProj->bounces : g_numFreezeBounces;
                sprite[otherSprite].zvel -= (2 << 4);
            }

            sprite[otherSprite].pal       = (pProj->pal >= 0) ? pProj->pal : 0;
            sprite[otherSprite].xrepeat   = pProj->xrepeat;
            sprite[otherSprite].yrepeat   = pProj->yrepeat;
            sprite[otherSprite].cstat     = (pProj->cstat >= 0) ? pProj->cstat : 128;
            sprite[otherSprite].clipdist  = (pProj->clipdist != 255) ? pProj->clipdist : 40;
            SpriteProjectile[otherSprite] = *Proj_GetProjectile(sprite[otherSprite].picnum);

            return otherSprite;
        }

    case PROJECTILE_KNEE:
        if (playerNum >= 0)
        {
            zvel = fix16_to_int(F16(100) - pPlayer->q16horiz - pPlayer->q16horizoff) << 5;
            startPos->z += (6 << 8);
            shootAng += 15;
        }
        else if (!(pProj->workslike & PROJECTILE_NOAIM))
        {
            int32_t playerDist;
            otherSprite = g_player[A_FindPlayer(pSprite, &playerDist)].ps->i;
            zvel = tabledivide32_noinline((sprite[otherSprite].z - startPos->z) << 8, playerDist + 1);
            shootAng = getangle(sprite[otherSprite].x - startPos->x, sprite[otherSprite].y - startPos->y);
        }

        Proj_DoHitscan(spriteNum, 0, startPos, zvel, shootAng, &hitData);

        if (hitData.sect < 0) return -1;

        if (pProj->range == 0)
            pProj->range = 1024;

        if (pProj->range > 0 && klabs(startPos->x - hitData.pos.x) + klabs(startPos->y - hitData.pos.y) > pProj->range)
            return -1;

        Proj_HandleKnee(&hitData, spriteNum, playerNum, projecTile, shootAng,
                        pProj, projecTile, pProj->extra_rand, pProj->spawns, pProj->sound);

        return -1;

    case PROJECTILE_BLOOD:
        shootAng += 64 - (krand() & 127);

        if (playerNum < 0)
            shootAng += 1024;

        zvel = 1024 - (krand() & 2047);

        Proj_DoHitscan(spriteNum, 0, startPos, zvel, shootAng, &hitData);

        if (pProj->range == 0)
            pProj->range = 1024;

        if (Proj_CheckBlood(startPos, &hitData, pProj->range, mulscale3(pProj->yrepeat, tilesiz[pProj->decal].y) << 8))
        {
            uwallptr_t const hitWall = (uwallptr_t)&wall[hitData.wall];

            if (FindDistance2D(hitWall->x - wall[hitWall->point2].x, hitWall->y - wall[hitWall->point2].y) >
                (mulscale3(pProj->xrepeat + 8, tilesiz[pProj->decal].x)))
            {
                if (SectorContainsSE13(hitWall->nextsector))
                    return -1;

                if (hitWall->nextwall >= 0 && wall[hitWall->nextwall].hitag != 0)
                    return -1;

                if (hitWall->hitag == 0 && pProj->decal >= 0)
                {
                    otherSprite = A_Spawn(spriteNum, pProj->decal);

                    A_SetHitData(otherSprite, &hitData);

                    if (!A_CheckSpriteFlags(otherSprite, SFLAG_DECAL))
                        actor[otherSprite].flags |= SFLAG_DECAL;

                    sprite[otherSprite].ang = getangle(hitWall->x - wall[hitWall->point2].x,
                        hitWall->y - wall[hitWall->point2].y) + 512;
                    Bmemcpy(&sprite[otherSprite], &hitData.pos, sizeof(vec3_t));

                    Proj_DoRandDecalSize(otherSprite, projecTile);

                    sprite[otherSprite].z += sprite[otherSprite].yrepeat << 8;

                    //                                sprite[spawned].cstat = 16+(krand()&12);
                    sprite[otherSprite].cstat = 16;

                    if (krand() & 1)
                        sprite[otherSprite].cstat |= 4;

                    if (krand() & 1)
                        sprite[otherSprite].cstat |= 8;

                    sprite[otherSprite].shade = sector[sprite[otherSprite].sectnum].floorshade;

                    A_SetSprite(otherSprite, CLIPMASK0);
                    A_AddToDeleteQueue(otherSprite);
                    changespritestat(otherSprite, 5);
                }
            }
        }

        return -1;

    default:
        return -1;
    }
}

#ifndef EDUKE32_STANDALONE
static int32_t A_ShootHardcoded(int spriteNum, int projecTile, int shootAng, vec3_t startPos,
                                spritetype *pSprite, int const playerNum, DukePlayer_t * const pPlayer)
{
    hitdata_t hitData;
    int const spriteSectnum = pSprite->sectnum;
    int32_t Zvel;
    int vel;

    switch (DYNAMICTILEMAP(projecTile))
    {
        case BLOODSPLAT1__STATIC:
        case BLOODSPLAT2__STATIC:
        case BLOODSPLAT3__STATIC:
        case BLOODSPLAT4__STATIC:
            shootAng += 64 - (krand() & 127);
            if (playerNum < 0)
                shootAng += 1024;
            Zvel = 1024 - (krand() & 2047);
            fallthrough__;
        case KNEE__STATIC:
            if (projecTile == KNEE)
            {
                if (playerNum >= 0)
                {
                    Zvel = fix16_to_int(F16(100) - pPlayer->q16horiz - pPlayer->q16horizoff) << 5;
                    startPos.z += (6 << 8);
                    shootAng += 15;
                }
                else
                {
                    int32_t   playerDist;
                    int const playerSprite = g_player[A_FindPlayer(pSprite, &playerDist)].ps->i;
                    Zvel                   = tabledivide32_noinline((sprite[playerSprite].z - startPos.z) << 8, playerDist + 1);
                    shootAng             = getangle(sprite[playerSprite].x - startPos.x, sprite[playerSprite].y - startPos.y);
                }
            }

            Proj_DoHitscan(spriteNum, 0, &startPos, Zvel, shootAng, &hitData);

            if (projecTile >= BLOODSPLAT1 && projecTile <= BLOODSPLAT4)
            {
                if (Proj_CheckBlood(&startPos, &hitData, 1024, 16 << 8))
                {
                    uwallptr_t const hitwal = (uwallptr_t)&wall[hitData.wall];

                    if (SectorContainsSE13(hitwal->nextsector))
                        return -1;

                    if (hitwal->nextwall >= 0 && wall[hitwal->nextwall].hitag != 0)
                        return -1;

                    if (hitwal->hitag == 0)
                    {
                        int const spawnedSprite = A_Spawn(spriteNum, projecTile);
                        sprite[spawnedSprite].ang
                        = (getangle(hitwal->x - wall[hitwal->point2].x, hitwal->y - wall[hitwal->point2].y) + 1536) & 2047;
                        sprite[spawnedSprite].pos = hitData.pos;
                        sprite[spawnedSprite].cstat |= (krand() & 4);
                        A_SetSprite(spawnedSprite, CLIPMASK0);
                        setsprite(spawnedSprite, &sprite[spawnedSprite].pos);
                        if (PN(spriteNum) == OOZFILTER || PN(spriteNum) == NEWBEAST)
                            sprite[spawnedSprite].pal = 6;
                    }
                }

                return -1;
            }

            if (hitData.sect < 0)
                break;

            if (klabs(startPos.x - hitData.pos.x) + klabs(startPos.y - hitData.pos.y) < 1024)
                Proj_HandleKnee(&hitData, spriteNum, playerNum, projecTile, shootAng, NULL, KNEE, 7, SMALLSMOKE, KICK_HIT);
            break;

        case SHOTSPARK1__STATIC:
        case SHOTGUN__STATIC:
        case CHAINGUN__STATIC:
        {
            if (pSprite->extra >= 0)
                pSprite->shade = -96;

            if (playerNum >= 0)
                P_PreFireHitscan(spriteNum, playerNum, projecTile, &startPos, &Zvel, &shootAng,
                    projecTile == SHOTSPARK1__STATIC && !WW2GI, 1);
            else
                A_PreFireHitscan(pSprite, &startPos, &Zvel, &shootAng, 1);

            if (Proj_DoHitscan(spriteNum, 256 + 1, &startPos, Zvel, shootAng, &hitData))
                return -1;

            if ((krand() & 15) == 0 && sector[hitData.sect].lotag == ST_2_UNDERWATER)
                Proj_DoWaterTracers(hitData.pos, &startPos, 8 - (ud.multimode >> 1), pSprite->sectnum);

            int spawnedSprite;

            if (playerNum >= 0)
            {
                spawnedSprite = Proj_InsertShotspark(&hitData, spriteNum, projecTile, 10, shootAng, G_DefaultActorHealth(projecTile) + (krand() % 6));

                if (P_PostFireHitscan(playerNum, spawnedSprite, &hitData, spriteNum, projecTile, Zvel, -SMALLSMOKE, BULLETHOLE, SHOTSPARK1, 0) < 0)
                    return -1;
            }
            else
            {
                spawnedSprite = A_PostFireHitscan(&hitData, spriteNum, projecTile, Zvel, shootAng, G_DefaultActorHealth(projecTile), -SMALLSMOKE,
                    SHOTSPARK1);
            }

            if ((krand() & 255) < 4)
                S_PlaySound3D(PISTOL_RICOCHET, spawnedSprite, &hitData.pos);

            return -1;
        }

        case GROWSPARK__STATIC:
        {
            if (playerNum >= 0)
                P_PreFireHitscan(spriteNum, playerNum, projecTile, &startPos, &Zvel, &shootAng, 1, 1);
            else
                A_PreFireHitscan(pSprite, &startPos, &Zvel, &shootAng, 1);

            if (Proj_DoHitscan(spriteNum, 256 + 1, &startPos, Zvel, shootAng, &hitData))
                return -1;

            int const otherSprite = A_InsertSprite(hitData.sect, hitData.pos.x, hitData.pos.y, hitData.pos.z, GROWSPARK, -16, 28, 28,
                                                   shootAng, 0, 0, spriteNum, 1);

            sprite[otherSprite].pal = 2;
            sprite[otherSprite].cstat |= 130;
            sprite[otherSprite].xrepeat = sprite[otherSprite].yrepeat = 1;
            A_SetHitData(otherSprite, &hitData);

            if (hitData.wall == -1 && hitData.sprite == -1 && hitData.sect >= 0)
            {
                Proj_MaybeDamageCF2(otherSprite, Zvel, hitData.sect);
            }
            else if (hitData.sprite >= 0)
                A_DamageObject(hitData.sprite, otherSprite);
            else if (hitData.wall >= 0 && wall[hitData.wall].picnum != ACCESSSWITCH && wall[hitData.wall].picnum != ACCESSSWITCH2)
                A_DamageWall(otherSprite, hitData.wall, hitData.pos, projecTile);
        }
        break;

        case FIRELASER__STATIC:
        case SPIT__STATIC:
        case COOLEXPLOSION1__STATIC:
        {
            if (pSprite->extra >= 0)
                pSprite->shade = -96;

            switch (projecTile)
            {
                case SPIT__STATIC: vel = 292; break;
                case COOLEXPLOSION1__STATIC:
                    vel = (pSprite->picnum == BOSS2) ? 644 : 348;
                    startPos.z -= (4 << 7);
                    break;
                case FIRELASER__STATIC:
                default:
                    vel = 840;
                    startPos.z -= (4 << 7);
                    break;
            }

            if (playerNum >= 0)
            {
                if (GetAutoAimAng(spriteNum, playerNum, projecTile, -ZOFFSET4, 0, &startPos, vel, &Zvel, &shootAng) < 0)
                    Zvel = fix16_to_int(F16(100) - pPlayer->q16horiz - pPlayer->q16horizoff) * 98;
            }
            else
            {
                int const otherPlayer = A_FindPlayer(pSprite, NULL);
                shootAng           += 16 - (krand() & 31);
                hitData.pos.x         = safeldist(g_player[otherPlayer].ps->i, pSprite);
                Zvel                  = tabledivide32_noinline((g_player[otherPlayer].ps->opos.z - startPos.z + (3 << 8)) * vel, hitData.pos.x);
            }

            Zvel = A_GetShootZvel(Zvel);

            int spriteSize = (playerNum >= 0) ? 7 : 18;

            if (projecTile == SPIT)
            {
                spriteSize = 18;
                startPos.z -= (10 << 8);
            }

            int const returnSprite = A_InsertSprite(spriteSectnum, startPos.x, startPos.y, startPos.z, projecTile, -127, spriteSize, spriteSize,
                                                    shootAng, vel, Zvel, spriteNum, 4);

            sprite[returnSprite].extra += (krand() & 7);

            if (projecTile == COOLEXPLOSION1)
            {
                sprite[returnSprite].shade = 0;

                if (PN(spriteNum) == BOSS2)
                {
                    int const saveXvel        = sprite[returnSprite].xvel;
                    sprite[returnSprite].xvel = MinibossScale(spriteNum, 1024);
                    A_SetSprite(returnSprite, CLIPMASK0);
                    sprite[returnSprite].xvel = saveXvel;
                    sprite[returnSprite].ang += 128 - (krand() & 255);
                }
            }

            sprite[returnSprite].cstat    = 128;
            sprite[returnSprite].clipdist = 4;

            return returnSprite;
        }

        case FREEZEBLAST__STATIC:
            startPos.z += (3 << 8);
            fallthrough__;
        case RPG__STATIC:
        {
            // XXX: "CODEDUP"
            if (pSprite->extra >= 0)
                pSprite->shade = -96;

            vel = 644;

            int j = -1;

            if (playerNum >= 0)
            {
                // NOTE: j is a SPRITE_INDEX
                j = GetAutoAimAng(spriteNum, playerNum, projecTile, 8 << 8, 0 + 2, &startPos, vel, &Zvel, &shootAng);

                if (j < 0)
                    Zvel = fix16_to_int(F16(100) - pPlayer->q16horiz - pPlayer->q16horizoff) * 81;

                if (projecTile == RPG)
                    A_PlaySound(RPG_SHOOT, spriteNum);
            }
            else
            {
                // NOTE: j is a player index
                j          = A_FindPlayer(pSprite, NULL);
                shootAng = getangle(g_player[j].ps->opos.x - startPos.x, g_player[j].ps->opos.y - startPos.y);
                if (PN(spriteNum) == BOSS3)
                    startPos.z -= MinibossScale(spriteNum, ZOFFSET5);
                else if (PN(spriteNum) == BOSS2)
                {
                    vel += 128;
                    startPos.z += MinibossScale(spriteNum, 24 << 8);
                }

                Zvel = tabledivide32_noinline((g_player[j].ps->opos.z - startPos.z) * vel, safeldist(g_player[j].ps->i, pSprite));

                if (A_CheckEnemySprite(pSprite) && (AC_MOVFLAGS(pSprite, &actor[spriteNum]) & face_player_smart))
                    shootAng = pSprite->ang + (krand() & 31) - 16;
            }

            if (numplayers > 1 && g_netClient)
                return -1;

            Zvel                   = A_GetShootZvel(Zvel);
            int const returnSprite = A_InsertSprite(spriteSectnum, startPos.x + (sintable[(348 + shootAng + 512) & 2047] / 448),
                                                    startPos.y + (sintable[(shootAng + 348) & 2047] / 448), startPos.z - (1 << 8),
                                                    projecTile, 0, 14, 14, shootAng, vel, Zvel, spriteNum, 4);
            auto const pReturn = &sprite[returnSprite];

            pReturn->extra += (krand() & 7);
            if (projecTile != FREEZEBLAST)
                pReturn->yvel = (playerNum >= 0 && j >= 0) ? j : -1;  // RPG_YVEL
            else
            {
                pReturn->yvel = g_numFreezeBounces;
                pReturn->xrepeat >>= 1;
                pReturn->yrepeat >>= 1;
                pReturn->zvel -= (2 << 4);
            }

            if (playerNum == -1)
            {
                if (PN(spriteNum) == BOSS3)
                {
                    if (krand() & 1)
                    {
                        pReturn->x -= MinibossScale(spriteNum, sintable[shootAng & 2047] >> 6);
                        pReturn->y -= MinibossScale(spriteNum, sintable[(shootAng + 1024 + 512) & 2047] >> 6);
                        pReturn->ang -= MinibossScale(spriteNum, 8);
                    }
                    else
                    {
                        pReturn->x += MinibossScale(spriteNum, sintable[shootAng & 2047] >> 6);
                        pReturn->y += MinibossScale(spriteNum, sintable[(shootAng + 1024 + 512) & 2047] >> 6);
                        pReturn->ang += MinibossScale(spriteNum, 4);
                    }
                    pReturn->xrepeat = MinibossScale(spriteNum, 42);
                    pReturn->yrepeat = MinibossScale(spriteNum, 42);
                }
                else if (PN(spriteNum) == BOSS2)
                {
                    pReturn->x -= MinibossScale(spriteNum, sintable[shootAng & 2047] / 56);
                    pReturn->y -= MinibossScale(spriteNum, sintable[(shootAng + 1024 + 512) & 2047] / 56);
                    pReturn->ang -= MinibossScale(spriteNum, 8) + (krand() & 255) - 128;
                    pReturn->xrepeat = 24;
                    pReturn->yrepeat = 24;
                }
                else if (projecTile != FREEZEBLAST)
                {
                    pReturn->xrepeat = 30;
                    pReturn->yrepeat = 30;
                    pReturn->extra >>= 2;
                }
            }
            else if (PWEAPON(playerNum, g_player[playerNum].ps->curr_weapon, WorksLike) == DEVISTATOR_WEAPON)
            {
                pReturn->extra >>= 2;
                pReturn->ang += 16 - (krand() & 31);
                pReturn->zvel += 256 - (krand() & 511);

                if (g_player[playerNum].ps->hbomb_hold_delay)
                {
                    pReturn->x -= sintable[shootAng & 2047] / 644;
                    pReturn->y -= sintable[(shootAng + 1024 + 512) & 2047] / 644;
                }
                else
                {
                    pReturn->x += sintable[shootAng & 2047] >> 8;
                    pReturn->y += sintable[(shootAng + 1024 + 512) & 2047] >> 8;
                }
                pReturn->xrepeat >>= 1;
                pReturn->yrepeat >>= 1;
            }

            pReturn->cstat    = 128;
            pReturn->clipdist = (projecTile == RPG) ? 4 : 40;

            return returnSprite;
        }

        case HANDHOLDINGLASER__STATIC:
        {
            int const zOffset     = (playerNum >= 0) ? g_player[playerNum].ps->pyoff : 0;
            Zvel                  = (playerNum >= 0) ? fix16_to_int(F16(100) - pPlayer->q16horiz - pPlayer->q16horizoff) * 32 : 0;

            startPos.z -= zOffset;
            Proj_DoHitscan(spriteNum, 0, &startPos, Zvel, shootAng, &hitData);
            startPos.z += zOffset;

            int placeMine = 0;
            if (hitData.sprite >= 0)
                break;

            if (hitData.wall >= 0 && hitData.sect >= 0)
                if (((hitData.pos.x - startPos.x) * (hitData.pos.x - startPos.x)
                     + (hitData.pos.y - startPos.y) * (hitData.pos.y - startPos.y))
                    < (290 * 290))
                {
                    // ST_2_UNDERWATER
                    if (wall[hitData.wall].nextsector >= 0)
                    {
                        if (sector[wall[hitData.wall].nextsector].lotag <= 2 && sector[hitData.sect].lotag <= 2)
                            placeMine = 1;
                    }
                    else if (sector[hitData.sect].lotag <= 2)
                        placeMine = 1;
                }

            if (placeMine == 1)
            {
                int const tripBombMode = (playerNum < 0) ? 0 :
#ifdef LUNATIC
                                                           g_player[playerNum].ps->tripbombControl;
#else
                                                           Gv_GetVarByLabel("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE,
                                                                            g_player[playerNum].ps->i, playerNum);
#endif
                int const spawnedSprite = A_InsertSprite(hitData.sect, hitData.pos.x, hitData.pos.y, hitData.pos.z, TRIPBOMB, -16, 4, 5,
                                                         shootAng, 0, 0, spriteNum, 6);
                if (tripBombMode & TRIPBOMB_TIMER)
                {
#ifdef LUNATIC
                    int32_t lLifetime    = g_player[playerNum].ps->tripbombLifetime;
                    int32_t lLifetimeVar = g_player[playerNum].ps->tripbombLifetimeVar;
#else
                    int32_t lLifetime = Gv_GetVarByLabel("STICKYBOMB_LIFETIME", NAM_GRENADE_LIFETIME, g_player[playerNum].ps->i, playerNum);
                    int32_t lLifetimeVar
                    = Gv_GetVarByLabel("STICKYBOMB_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, g_player[playerNum].ps->i, playerNum);
#endif
                    // set timer.  blows up when at zero....
                    actor[spawnedSprite].t_data[7] = lLifetime + mulscale14(krand(), lLifetimeVar) - lLifetimeVar;
                    // TIMER_CONTROL
                    actor[spawnedSprite].t_data[6] = 1;
                }
                else
                    sprite[spawnedSprite].hitag = spawnedSprite;

                A_PlaySound(LASERTRIP_ONWALL, spawnedSprite);
                sprite[spawnedSprite].xvel = -20;
                A_SetSprite(spawnedSprite, CLIPMASK0);
                sprite[spawnedSprite].cstat = 16;

                int const p2      = wall[hitData.wall].point2;
                int const wallAng = getangle(wall[hitData.wall].x - wall[p2].x, wall[hitData.wall].y - wall[p2].y) - 512;

                actor[spawnedSprite].t_data[5] = sprite[spawnedSprite].ang = wallAng;

                return spawnedSprite;
            }
            return -1;
        }

        case BOUNCEMINE__STATIC:
        case MORTER__STATIC:
        {
            if (pSprite->extra >= 0)
                pSprite->shade = -96;

            int const playerSprite = g_player[A_FindPlayer(pSprite, NULL)].ps->i;
            int const playerDist   = ldist(&sprite[playerSprite], pSprite);

            Zvel = -playerDist >> 1;

            if (Zvel < -4096)
                Zvel = -2048;

            vel  = playerDist >> 4;
            Zvel = A_GetShootZvel(Zvel);

            A_InsertSprite(spriteSectnum, startPos.x + (sintable[(512 + shootAng + 512) & 2047] >> 8),
                           startPos.y + (sintable[(shootAng + 512) & 2047] >> 8), startPos.z + (6 << 8), projecTile, -64, 32, 32,
                           shootAng, vel, Zvel, spriteNum, 1);
            break;
        }

        case SHRINKER__STATIC:
        {
            if (pSprite->extra >= 0)
                pSprite->shade = -96;

            if (playerNum >= 0)
            {
                if (GetAutoAimAng(spriteNum, playerNum, projecTile, ZOFFSET6, 0, &startPos, 768, &Zvel, &shootAng) < 0)
                    Zvel = fix16_to_int(F16(100) - pPlayer->q16horiz - pPlayer->q16horizoff) * 98;
            }
            else if (pSprite->statnum != STAT_EFFECTOR)
            {
                int const otherPlayer = A_FindPlayer(pSprite, NULL);
                Zvel                  = tabledivide32_noinline((g_player[otherPlayer].ps->opos.z - startPos.z) * 512,
                                              safeldist(g_player[otherPlayer].ps->i, pSprite));
            }
            else
                Zvel = 0;

            Zvel                   = A_GetShootZvel(Zvel);
            int const returnSprite = A_InsertSprite(spriteSectnum, startPos.x + (sintable[(512 + shootAng + 512) & 2047] >> 12),
                                                    startPos.y + (sintable[(shootAng + 512) & 2047] >> 12), startPos.z + (2 << 8),
                                                    SHRINKSPARK, -16, 28, 28, shootAng, 768, Zvel, spriteNum, 4);
            sprite[returnSprite].cstat    = 128;
            sprite[returnSprite].clipdist = 32;

            return returnSprite;
        }
    }

    return -1;
}
#endif

int A_ShootWithZvel(int const spriteNum, int const projecTile, int const forceZvel)
{
    Bassert(projecTile >= 0);

    auto const   pSprite   = &sprite[spriteNum];
    int const           playerNum = (pSprite->picnum == APLAYER) ? P_GetP(pSprite) : -1;
    auto const pPlayer   = playerNum >= 0 ? g_player[playerNum].ps : NULL;

    if (forceZvel != SHOOT_HARDCODED_ZVEL)
    {
        g_overrideShootZvel = 1;
        g_shootZvel = forceZvel;
    }
    else
        g_overrideShootZvel = 0;

    int    shootAng;
    vec3_t startPos;

    if (pPlayer != NULL)
    {
        startPos = pPlayer->pos;
        startPos.z += pPlayer->pyoff + ZOFFSET6;
        shootAng = fix16_to_int(pPlayer->q16ang);

        pPlayer->crack_time = PCRACKTIME;
    }
    else
    {
        shootAng = pSprite->ang;
        startPos = pSprite->pos;
        startPos.z -= (((pSprite->yrepeat * tilesiz[pSprite->picnum].y)<<1) - ZOFFSET6);

        if (pSprite->picnum != ROTATEGUN)
        {
            startPos.z -= (7<<8);

            if (A_CheckEnemySprite(pSprite) && PN(spriteNum) != COMMANDER)
            {
                startPos.x += (sintable[(shootAng+1024+96)&2047]>>7);
                startPos.y += (sintable[(shootAng+512+96)&2047]>>7);
            }
        }

#ifndef EDUKE32_STANDALONE
#ifdef POLYMER
        switch (DYNAMICTILEMAP(projecTile))
        {
            case FIRELASER__STATIC:
            case SHOTGUN__STATIC:
            case SHOTSPARK1__STATIC:
            case CHAINGUN__STATIC:
            case RPG__STATIC:
            case MORTER__STATIC:
                {
                    vec2_t const v = { ((sintable[(pSprite->ang + 512) & 2047]) >> 7),
                                       ((sintable[(pSprite->ang) & 2047]) >> 7) };

                    pSprite->x += v.x;
                    pSprite->y += v.y;
                    G_AddGameLight(0, spriteNum, PHEIGHT, 8192, 255 + (95 << 8), PR_LIGHT_PRIO_MAX_GAME);
                    actor[spriteNum].lightcount = 2;
                    pSprite->x -= v.x;
                    pSprite->y -= v.y;
                }

                break;
            }
#endif // POLYMER
#endif // !EDUKE32_STANDALONE
    }

#ifdef EDUKE32_STANDALONE
    return A_CheckSpriteTileFlags(projecTile, SFLAG_PROJECTILE) ? A_ShootCustom(spriteNum, projecTile, shootAng, &startPos) : -1;
#else
    return A_CheckSpriteTileFlags(projecTile, SFLAG_PROJECTILE)
           ? A_ShootCustom(spriteNum, projecTile, shootAng, &startPos)
           : !FURY ? A_ShootHardcoded(spriteNum, projecTile, shootAng, startPos, pSprite, playerNum, pPlayer) : -1;
#endif
}


//////////////////// HUD WEAPON / MISC. DISPLAY CODE ////////////////////

static void P_DisplaySpit(void)
{
    auto const pPlayer     = g_player[screenpeek].ps;
    int const           loogCounter = pPlayer->loogcnt;

    if (loogCounter == 0)
        return;

    if (VM_OnEvent(EVENT_DISPLAYSPIT, pPlayer->i, screenpeek) != 0)
        return;

    int const rotY = loogCounter<<2;

    for (bssize_t i=0; i < pPlayer->numloogs; i++)
    {
        int const rotAng = klabs(sintable[((loogCounter + i) << 5) & 2047]) >> 5;
        int const rotZoom  = 4096 + ((loogCounter + i) << 9);
        int const rotX     = (-fix16_to_int(g_player[screenpeek].input->q16avel) >> 1) + (sintable[((loogCounter + i) << 6) & 2047] >> 10);

        rotatesprite_fs((pPlayer->loogiex[i] + rotX) << 16, (200 + pPlayer->loogiey[i] - rotY) << 16, rotZoom - (i << 8),
                        256 - rotAng, LOOGIE, 0, 0, 2);
    }
}

int P_GetHudPal(const DukePlayer_t *p)
{
    if (sprite[p->i].pal == 1)
        return 1;

    if (p->cursectnum >= 0)
    {
        int const hudPal = sector[p->cursectnum].floorpal;
        if (!g_noFloorPal[hudPal])
            return hudPal;
    }

    return 0;
}

int P_GetKneePal(DukePlayer_t const * pPlayer)
{
    return P_GetKneePal(pPlayer, P_GetHudPal(pPlayer));
}

int P_GetKneePal(DukePlayer_t const * pPlayer, int const hudPal)
{
    return hudPal == 0 ? pPlayer->palookup : hudPal;
}

int P_GetOverheadPal(DukePlayer_t const * pPlayer)
{
    return sprite[pPlayer->i].pal;
}

static int P_DisplayFist(int const fistShade)
{
    DukePlayer_t const *const pPlayer = g_player[screenpeek].ps;
    int fistInc = pPlayer->fist_incs;

    if (fistInc > 32)
        fistInc = 32;

    if (fistInc <= 0)
        return 0;

    switch (VM_OnEvent(EVENT_DISPLAYFIST, pPlayer->i, screenpeek))
    {
        case 1: return 1;
        case -1: return 0;
    }

    int const fistY       = klabs(pPlayer->look_ang) / 9;
    int const fistZoom    = clamp(65536 - (sintable[(512 + (fistInc << 6)) & 2047] << 2), 40920, 90612);
    int const fistYOffset = 194 + (sintable[((6 + fistInc) << 7) & 2047] >> 9);
    int const fistPal     = P_GetHudPal(pPlayer);
    int       wx[2]       = { windowxy1.x, windowxy2.x };

#ifdef SPLITSCREEN_MOD_HACKS
    // XXX: this is outdated, doesn't handle above/below split.
    if (g_fakeMultiMode==2)
        wx[(g_snum==0)] = (wx[0]+wx[1])/2+1;
#endif

    rotatesprite((-fistInc + 222 + (fix16_to_int(g_player[screenpeek].input->q16avel) >> 5)) << 16, (fistY + fistYOffset) << 16,
                 fistZoom, 0, FIST, fistShade, fistPal, 2, wx[0], windowxy1.y, wx[1], windowxy2.y);

    return 1;
}

#define DRAWEAP_CENTER 262144
#define weapsc(sc) scale(sc, ud.weaponscale, 100)

static int32_t g_dts_yadd;

static void G_DrawTileScaled(int drawX, int drawY, int tileNum, int drawShade, int drawBits, int drawPal)
{
    int32_t wx[2] = { windowxy1.x, windowxy2.x };
    int32_t wy[2] = { windowxy1.y, windowxy2.y };

    int drawYOffset = 0;
    int drawXOffset = 192<<16;

    switch (hudweap.cur)
    {
        case DEVISTATOR_WEAPON:
        case TRIPBOMB_WEAPON:
            drawXOffset = 160<<16;
            break;
        default:
            if (drawBits & DRAWEAP_CENTER)
            {
                drawXOffset = 160<<16;
                drawBits &= ~DRAWEAP_CENTER;
            }
            break;
    }

    // bit 4 means "flip x" for G_DrawTileScaled
    int const drawAng = (drawBits & 4) ? 1024 : 0;

#ifdef SPLITSCREEN_MOD_HACKS
    if (g_fakeMultiMode==2)
    {
        int const sideBySide = (ud.screen_size!=0);

        // splitscreen HACK
        drawBits &= ~(1024|512|256);
        if (sideBySide)
        {
            drawBits &= ~8;
            wx[(g_snum==0)] = (wx[0]+wx[1])/2 + 2;
        }
        else
        {
            drawBits |= 8;
            if (g_snum==0)
                drawYOffset = -(100<<16);
            wy[(g_snum==0)] = (wy[0]+wy[1])/2 + 2;
        }
    }
#endif

#ifdef USE_OPENGL
    if (videoGetRenderMode() >= REND_POLYMOST && usemodels && md_tilehasmodel(tileNum,drawPal) >= 0)
        drawYOffset += (224<<16)-weapsc(224<<16);
#endif
    rotatesprite(weapsc(drawX<<16) + (drawXOffset-weapsc(drawXOffset)),
                 weapsc((drawY<<16) + g_dts_yadd) + ((200<<16)-weapsc(200<<16)) + drawYOffset,
                 weapsc(65536L),drawAng,tileNum,drawShade,drawPal,(2|drawBits),
                 wx[0],wy[0], wx[1],wy[1]);
}

static void G_DrawWeaponTile(int weaponX, int weaponY, int weaponTile, int weaponShade, int weaponBits, int weaponPal)
{
    static int shadef = 0;
    static int palf = 0;

    // basic fading between player weapon shades
    if (shadef != weaponShade && (!weaponPal || palf == weaponPal))
    {
        shadef += (weaponShade - shadef) >> 2;

        if (!((weaponShade - shadef) >> 2))
            shadef = logapproach(shadef, weaponShade);
    }
    else
        shadef = weaponShade;

    palf = weaponPal;

#ifdef USE_OPENGL
    if (videoGetRenderMode() >= REND_POLYMOST)
    {
        if (weaponTile >= CHAINGUN + 1 && weaponTile <= CHAINGUN + 4)
        {
            if (!usemodels || md_tilehasmodel(weaponTile, weaponPal) < 0)
            {
                // HACK: Draw the upper part of the chaingun two screen
                // pixels (not texels; multiplied by weapon scale) lower
                // first, preventing ugly horizontal seam.
                g_dts_yadd = tabledivide32_noinline(65536 * 2 * 200, ydim);
                G_DrawTileScaled(weaponX, weaponY, weaponTile, shadef, weaponBits, weaponPal);
                g_dts_yadd = 0;
            }
        }
    }
#endif

    G_DrawTileScaled(weaponX, weaponY, weaponTile, shadef, weaponBits, weaponPal);
}

static inline void G_DrawWeaponTileWithID(int uniqueID, int weaponX, int weaponY, int weaponTile, int weaponShade,
                                          int weaponBits, int p)
{
    int lastUniqueID = guniqhudid;
    guniqhudid       = uniqueID;

    G_DrawWeaponTile(weaponX, weaponY, weaponTile, weaponShade, weaponBits, p);

    guniqhudid       = lastUniqueID;
}

static inline void G_DrawWeaponTileUnfadedWithID(int uniqueID, int weaponX, int weaponY, int weaponTile, int weaponShade,
                                          int weaponBits, int p)
{
    int lastUniqueID = guniqhudid;
    guniqhudid       = uniqueID;

    G_DrawTileScaled(weaponX, weaponY, weaponTile, weaponShade, weaponBits, p); // skip G_DrawWeaponTile

    guniqhudid       = lastUniqueID;
}

static int P_DisplayKnee(int kneeShade)
{
    static int8_t const       knee_y[] = { 0, -8, -16, -32, -64, -84, -108, -108, -108, -72, -32, -8 };
    auto const ps = g_player[screenpeek].ps;

    if (ps->knee_incs == 0)
        return 0;

    switch (VM_OnEvent(EVENT_DISPLAYKNEE, ps->i, screenpeek))
    {
        case 1: return 1;
        case -1: return 0;
    }

    if (ps->knee_incs >= ARRAY_SIZE(knee_y) || sprite[ps->i].extra <= 0)
        return 0;

    int const kneeY   = knee_y[ps->knee_incs] + (klabs(ps->look_ang) / 9) - (ps->hard_landing << 3);
    int const kneePal = P_GetKneePal(ps);

    G_DrawTileScaled(105+(fix16_to_int(g_player[screenpeek].input->q16avel)>>5)-(ps->look_ang>>1)+(knee_y[ps->knee_incs]>>2),
                     kneeY+280-(fix16_to_int(ps->q16horiz-ps->q16horizoff)>>4),KNEE,kneeShade,4+DRAWEAP_CENTER,kneePal);

    return 1;
}

static int P_DisplayKnuckles(int knuckleShade)
{
    if (WW2GI)
        return 0;

    auto const pPlayer = g_player[screenpeek].ps;

    if (pPlayer->knuckle_incs == 0)
        return 0;

    static int8_t const knuckleFrames[] = { 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0 };

    switch (VM_OnEvent(EVENT_DISPLAYKNUCKLES, pPlayer->i, screenpeek))
    {
        case 1: return 1;
        case -1: return 0;
    }

    if ((unsigned) (pPlayer->knuckle_incs>>1) >= ARRAY_SIZE(knuckleFrames) || sprite[pPlayer->i].extra <= 0)
        return 0;

    int const knuckleY   = (klabs(pPlayer->look_ang) / 9) - (pPlayer->hard_landing << 3);
    int const knucklePal = P_GetHudPal(pPlayer);

    G_DrawTileScaled(160 + (fix16_to_int(g_player[screenpeek].input->q16avel) >> 5) - (pPlayer->look_ang >> 1),
                     knuckleY + 180 - (fix16_to_int(pPlayer->q16horiz - pPlayer->q16horizoff) >> 4),
                     CRACKKNUCKLES + knuckleFrames[pPlayer->knuckle_incs >> 1], knuckleShade, 4 + DRAWEAP_CENTER,
                     knucklePal);

    return 1;
}

#if !defined LUNATIC
// Set C-CON's WEAPON and WORKSLIKE gamevars.
void P_SetWeaponGamevars(int playerNum, const DukePlayer_t * const pPlayer)
{
    Gv_SetVar(g_weaponVarID, pPlayer->curr_weapon, pPlayer->i, playerNum);
    Gv_SetVar(g_worksLikeVarID,
              ((unsigned)pPlayer->curr_weapon < MAX_WEAPONS) ? PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) : -1,
              pPlayer->i, playerNum);
}
#endif

static void P_FireWeapon(int playerNum)
{
    auto const pPlayer = g_player[playerNum].ps;

    if (VM_OnEvent(EVENT_DOFIRE, pPlayer->i, playerNum) || pPlayer->weapon_pos != 0)
        return;

    if (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) != KNEE_WEAPON)
        pPlayer->ammo_amount[pPlayer->curr_weapon]--;

    if (PWEAPON(playerNum, pPlayer->curr_weapon, FireSound) > 0)
        A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, FireSound), pPlayer->i);

    P_SetWeaponGamevars(playerNum, pPlayer);
    //        OSD_Printf("doing %d %d %d\n",PWEAPON(snum, p->curr_weapon, Shoots),p->curr_weapon,snum);
    A_Shoot(pPlayer->i, PWEAPON(playerNum, pPlayer->curr_weapon, Shoots));

    for (bssize_t burstFire = PWEAPON(playerNum, pPlayer->curr_weapon, ShotsPerBurst) - 1; burstFire > 0; --burstFire)
    {
        if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_FIREEVERYOTHER)
        {
            // devastator hack to make the projectiles fire on a delay from player code
            actor[pPlayer->i].t_data[7] = (PWEAPON(playerNum, pPlayer->curr_weapon, ShotsPerBurst)) << 1;
        }
        else
        {
            if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_AMMOPERSHOT &&
                PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) != KNEE_WEAPON)
            {
                if (pPlayer->ammo_amount[pPlayer->curr_weapon] > 0)
                    pPlayer->ammo_amount[pPlayer->curr_weapon]--;
                else
                    break;
            }

            A_Shoot(pPlayer->i, PWEAPON(playerNum, pPlayer->curr_weapon, Shoots));
        }
    }

    if (!(PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_NOVISIBLE))
    {
#ifdef POLYMER
        spritetype *s = &sprite[pPlayer->i];
        int32_t     x = ((sintable[(s->ang + 512) & 2047]) >> 7), y = ((sintable[(s->ang) & 2047]) >> 7);

        s->x += x;
        s->y += y;
        G_AddGameLight(0, pPlayer->i, PHEIGHT, 8192, PWEAPON(playerNum, pPlayer->curr_weapon, FlashColor),
                       PR_LIGHT_PRIO_MAX_GAME);
        actor[pPlayer->i].lightcount = 2;
        s->x -= x;
        s->y -= y;
#endif  // POLYMER
        pPlayer->visibility = 0;
    }

    if (WW2GI)
    {
        if (/*!(PWEAPON(playerNum, p->curr_weapon, Flags) & WEAPON_CHECKATRELOAD) && */ pPlayer->reloading == 1 ||
                (PWEAPON(playerNum, pPlayer->curr_weapon, Reload) > PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime) && pPlayer->ammo_amount[pPlayer->curr_weapon] > 0
                 && (PWEAPON(playerNum, pPlayer->curr_weapon, Clip)) && (((pPlayer->ammo_amount[pPlayer->curr_weapon]%(PWEAPON(playerNum, pPlayer->curr_weapon, Clip)))==0))))
        {
            pPlayer->kickback_pic = PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime);
        }
    }
}

static void P_DoWeaponSpawn(int playerNum)
{
    auto const pPlayer = g_player[playerNum].ps;

    // NOTE: For the 'Spawn' member, 0 means 'none', too (originally so,
    // i.e. legacy). The check for <0 was added to the check because mod
    // authors (rightly) assumed that -1 is the no-op value.
    if (PWEAPON(playerNum, pPlayer->curr_weapon, Spawn) <= 0)  // <=0 : AMC TC beta/RC2 has WEAPONx_SPAWN -1
        return;

    int newSprite = A_Spawn(pPlayer->i, PWEAPON(playerNum, pPlayer->curr_weapon, Spawn));

    if ((PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_SPAWNTYPE3))
    {
        // like chaingun shells
        sprite[newSprite].ang += 1024;
        sprite[newSprite].ang &= 2047;
        sprite[newSprite].xvel += 32;
        sprite[newSprite].z += (3<<8);
    }

    A_SetSprite(newSprite,CLIPMASK0);
}

void P_DisplayScuba(void)
{
    if (g_player[screenpeek].ps->scuba_on)
    {
        auto const pPlayer = g_player[screenpeek].ps;

        if (VM_OnEvent(EVENT_DISPLAYSCUBA, pPlayer->i, screenpeek) != 0)
            return;

        int const scubaPal = P_GetHudPal(pPlayer);
        int scubaY = 200 - tilesiz[SCUBAMASK].y;
        if (ud.screen_size > 4 && ud.statusbarmode == 0)
            // Scale the offset of 8px with the status bar, otherwise the bottom of the tile is cut
            scubaY -= scale(8, ud.statusbarscale, 100);

#ifdef SPLITSCREEN_MOD_HACKS
        g_snum = screenpeek;
#endif

        // this is a hack to hide the seam that appears between the two halves of the mask in GL
#ifdef USE_OPENGL
        if (videoGetRenderMode() >= REND_POLYMOST)
            G_DrawTileScaled(44, scubaY, SCUBAMASK, 0, 2 + 16 + DRAWEAP_CENTER, scubaPal);
#endif
        G_DrawTileScaled(43, scubaY, SCUBAMASK, 0, 2 + 16 + DRAWEAP_CENTER, scubaPal);
        G_DrawTileScaled(320 - 43, scubaY, SCUBAMASK, 0, 2 + 4 + 16 + DRAWEAP_CENTER, scubaPal);
    }
}

static int8_t const access_tip_y [] = {
    0, -8, -16, -32, -64, -84, -108, -108, -108, -108, -108, -108, -108, -108, -108, -108, -96, -72, -64, -32, -16,
    /* EDuke32: */ 0, 16, 32, 48,
    // At y coord 64, the hand is already not shown.
};

static int P_DisplayTip(int tipShade)
{
    auto const pPlayer = g_player[screenpeek].ps;

    if (pPlayer->tipincs == 0)
        return 0;

    switch (VM_OnEvent(EVENT_DISPLAYTIP, pPlayer->i, screenpeek))
    {
        case 1: return 1;
        case -1: return 0;
    }

    // Report that the tipping hand has been drawn so that the otherwise
    // selected weapon is not drawn.
    if ((unsigned)pPlayer->tipincs >= ARRAY_SIZE(access_tip_y))
        return 1;

    int const tipY       = (klabs(pPlayer->look_ang) / 9) - (pPlayer->hard_landing << 3);
    int const tipPal     = P_GetHudPal(pPlayer);
    int const tipYOffset = access_tip_y[pPlayer->tipincs] >> 1;

    guniqhudid = 201;

    G_DrawTileScaled(170 + (fix16_to_int(g_player[screenpeek].input->q16avel) >> 5) - (pPlayer->look_ang >> 1),
                     tipYOffset + tipY + 240 - (fix16_to_int(pPlayer->q16horiz - pPlayer->q16horizoff) >> 4),
                     TIP + ((26 - pPlayer->tipincs) >> 4), tipShade, DRAWEAP_CENTER, tipPal);

    guniqhudid = 0;

    return 1;
}

static int P_DisplayAccess(int accessShade)
{
    auto const pSprite = g_player[screenpeek].ps;

    if (pSprite->access_incs == 0)
        return 0;

    switch (VM_OnEvent(EVENT_DISPLAYACCESS, pSprite->i, screenpeek))
    {
        case 1: return 1;
        case -1: return 0;
    }

    if ((unsigned)pSprite->access_incs >= ARRAY_SIZE(access_tip_y)-4 || sprite[pSprite->i].extra <= 0)
        return 1;

    int const accessX   = access_tip_y[pSprite->access_incs] >> 2;
    int const accessY   = access_tip_y[pSprite->access_incs] + (klabs(pSprite->look_ang) / 9) - (pSprite->hard_landing << 3);
    int const accessPal = (pSprite->access_spritenum >= 0) ? sprite[pSprite->access_spritenum].pal : 0;

    guniqhudid = 200;

    if ((pSprite->access_incs - 3) > 0 && (pSprite->access_incs - 3) >> 3)
    {
        G_DrawTileScaled(170 + (fix16_to_int(g_player[screenpeek].input->q16avel) >> 5) - (pSprite->look_ang >> 1) + accessX,
                         accessY + 266 - (fix16_to_int(pSprite->q16horiz - pSprite->q16horizoff) >> 4),
                         HANDHOLDINGLASER + (pSprite->access_incs >> 3), accessShade, DRAWEAP_CENTER, accessPal);
    }
    else
    {
        G_DrawTileScaled(170 + (fix16_to_int(g_player[screenpeek].input->q16avel) >> 5) - (pSprite->look_ang >> 1) + accessX,
                         accessY + 266 - (fix16_to_int(pSprite->q16horiz - pSprite->q16horizoff) >> 4), HANDHOLDINGACCESS, accessShade,
                         4 + DRAWEAP_CENTER, accessPal);
    }

    guniqhudid = 0;

    return 1;
}

void P_DisplayWeapon(void)
{
    auto const pPlayer     = g_player[screenpeek].ps;
    auto const weaponFrame = &pPlayer->kickback_pic;

    int currentWeapon;

#ifdef SPLITSCREEN_MOD_HACKS
    g_snum = screenpeek;
#endif

    if (pPlayer->newowner >= 0 || ud.camerasprite >= 0 || pPlayer->over_shoulder_on > 0
        || (sprite[pPlayer->i].pal != 1 && sprite[pPlayer->i].extra <= 0))
        return;

    int weaponX       = (160) - 90;
    int weaponY       = klabs(pPlayer->look_ang) / 9;
    int weaponYOffset = 80 - (pPlayer->weapon_pos * pPlayer->weapon_pos);
    int weaponShade   = sprite[pPlayer->i].shade <= 24 ? sprite[pPlayer->i].shade : 24;

    int32_t weaponBits = 0;
    UNREFERENCED_PARAMETER(weaponBits);

    if (P_DisplayFist(weaponShade) || P_DisplayKnuckles(weaponShade) || P_DisplayTip(weaponShade) || P_DisplayAccess(weaponShade))
        goto enddisplayweapon;

    P_DisplayKnee(weaponShade);

    if (ud.weaponsway)
    {
        weaponX -= (sintable[((pPlayer->weapon_sway>>1)+512)&2047]/(1024+512));
        weaponYOffset -= (sprite[pPlayer->i].xrepeat < 32) ? klabs(sintable[(pPlayer->weapon_sway << 2) & 2047] >> 9)
                                                           : klabs(sintable[(pPlayer->weapon_sway >> 1) & 2047] >> 10);
    }
    else weaponYOffset -= 16;

    weaponX -= 58 + pPlayer->weapon_ang;
    weaponYOffset -= (pPlayer->hard_landing << 3);

    currentWeapon       = PWEAPON(screenpeek, (pPlayer->last_weapon >= 0) ? pPlayer->last_weapon : pPlayer->curr_weapon, WorksLike);
    hudweap.gunposy     = weaponYOffset;
    hudweap.lookhoriz   = weaponY;
    hudweap.cur         = currentWeapon;
    hudweap.gunposx     = weaponX;
    hudweap.shade       = weaponShade;
    hudweap.count       = *weaponFrame;
    hudweap.lookhalfang = pPlayer->look_ang >> 1;

    if (VM_OnEvent(EVENT_DISPLAYWEAPON, pPlayer->i, screenpeek) == 0)
    {
#ifndef EDUKE32_STANDALONE
        int const quickKickFrame = 14 - pPlayer->quick_kick;

        if (!FURY && (quickKickFrame != 14 || pPlayer->last_quick_kick) && ud.drawweapon == 1)
        {
            int const weaponPal = P_GetKneePal(pPlayer);

            guniqhudid = 100;

            if (quickKickFrame < 6 || quickKickFrame > 12)
                G_DrawTileScaled(weaponX + 80 - (pPlayer->look_ang >> 1), weaponY + 250 - weaponYOffset, KNEE, weaponShade,
                                 weaponBits | 4 | DRAWEAP_CENTER, weaponPal);
            else
                G_DrawTileScaled(weaponX + 160 - 16 - (pPlayer->look_ang >> 1), weaponY + 214 - weaponYOffset, KNEE + 1,
                                 weaponShade, weaponBits | 4 | DRAWEAP_CENTER, weaponPal);
            guniqhudid = 0;
        }

        if (!FURY && sprite[pPlayer->i].xrepeat < 40)
        {
            static int32_t fistPos;

            int const weaponPal = P_GetHudPal(pPlayer);

            if (pPlayer->jetpack_on == 0)
            {
                int const playerXvel = sprite[pPlayer->i].xvel;
                weaponY += 32 - (playerXvel >> 3);
                fistPos += playerXvel >> 3;
            }

            currentWeapon = weaponX;
            weaponX += sintable[(fistPos)&2047] >> 10;
            G_DrawTileScaled(weaponX + 250 - (pPlayer->look_ang >> 1), weaponY + 258 - (klabs(sintable[(fistPos)&2047] >> 8)),
                FIST, weaponShade, weaponBits, weaponPal);
            weaponX = currentWeapon - (sintable[(fistPos)&2047] >> 10);
            G_DrawTileScaled(weaponX + 40 - (pPlayer->look_ang >> 1), weaponY + 200 + (klabs(sintable[(fistPos)&2047] >> 8)), FIST,
                weaponShade, weaponBits | 4, weaponPal);
        }
        else
#endif
        {
            switch (ud.drawweapon)
            {
                case 1: break;
#ifndef EDUKE32_STANDALONE
                case 2:
                    if (!FURY && (unsigned)hudweap.cur < MAX_WEAPONS && hudweap.cur != KNEE_WEAPON)
                        rotatesprite_win(160 << 16, (180 + (pPlayer->weapon_pos * pPlayer->weapon_pos)) << 16, divscale16(ud.statusbarscale, 100), 0,
                                         hudweap.cur == GROW_WEAPON ? GROWSPRITEICON : WeaponPickupSprites[hudweap.cur], 0,
                                         0, 2);
#endif
                default: goto enddisplayweapon;
            }

            if (VM_OnEvent(EVENT_DRAWWEAPON, g_player[screenpeek].ps->i, screenpeek)||(currentWeapon == KNEE_WEAPON && *weaponFrame == 0))
                goto enddisplayweapon;

#ifndef EDUKE32_STANDALONE
            int const doAnim      = !(sprite[pPlayer->i].pal == 1 || ud.pause_on || g_player[myconnectindex].ps->gm & MODE_MENU);
            int const halfLookAng = pPlayer->look_ang >> 1;

            int const weaponPal = P_GetHudPal(pPlayer);

            if (!FURY)
            switch (currentWeapon)
            {
            case KNEE_WEAPON:
            {
                int const kneePal = P_GetKneePal(pPlayer, weaponPal);

                guniqhudid = currentWeapon;
                if (*weaponFrame < 5 || *weaponFrame > 9)
                    G_DrawTileScaled(weaponX + 220 - halfLookAng, weaponY + 250 - weaponYOffset, KNEE,
                                     weaponShade, weaponBits, kneePal);
                else
                    G_DrawTileScaled(weaponX + 160 - halfLookAng, weaponY + 214 - weaponYOffset, KNEE + 1,
                                     weaponShade, weaponBits, kneePal);
                guniqhudid = 0;
                break;
            }

            case TRIPBOMB_WEAPON:
                weaponX += 8;
                weaponYOffset -= 10;

                if ((*weaponFrame) > 6)
                    weaponY += ((*weaponFrame) << 3);
                else if ((*weaponFrame) < 4)
                    G_DrawWeaponTileWithID(currentWeapon << 2, weaponX + 142 - halfLookAng,
                                           weaponY + 234 - weaponYOffset, HANDHOLDINGLASER + 3, weaponShade, weaponBits, weaponPal);

                G_DrawWeaponTileWithID(currentWeapon, weaponX + 130 - halfLookAng, weaponY + 249 - weaponYOffset,
                                       HANDHOLDINGLASER + ((*weaponFrame) >> 2), weaponShade, weaponBits, weaponPal);

                G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 152 - halfLookAng,
                                       weaponY + 249 - weaponYOffset, HANDHOLDINGLASER + ((*weaponFrame) >> 2), weaponShade, weaponBits | 4,
                                       weaponPal);
                break;

            case RPG_WEAPON:
                weaponX -= sintable[(768 + ((*weaponFrame) << 7)) & 2047] >> 11;
                weaponYOffset += sintable[(768 + ((*weaponFrame) << 7)) & 2047] >> 11;

                if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING))
                    weaponBits |= 512;

                if (*weaponFrame > 0)
                {
                    int totalTime;
                    if (*weaponFrame < (WW2GI ? (totalTime = PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime)) : 8))
                        G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 164, (weaponY << 1) + 176 - weaponYOffset,
                            RPGGUN + ((*weaponFrame) >> 1), weaponShade, weaponBits, weaponPal);
                    else if (WW2GI)
                    {
                        totalTime = PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime);
                        int const reloadTime = PWEAPON(screenpeek, pPlayer->curr_weapon, Reload);

                        weaponYOffset -= (*weaponFrame < ((reloadTime - totalTime) / 2 + totalTime))
                                          ? 10 * ((*weaponFrame) - totalTime)   // down
                                          : 10 * (reloadTime - (*weaponFrame)); // up
                    }
                }

                G_DrawWeaponTileWithID(currentWeapon, weaponX + 164, (weaponY << 1) + 176 - weaponYOffset, RPGGUN, weaponShade,
                                       weaponBits, weaponPal);
                break;

            case SHOTGUN_WEAPON:
                weaponX -= 8;

                if (WW2GI)
                {
                    int const totalTime  = PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime);
                    int const reloadTime = PWEAPON(screenpeek, pPlayer->curr_weapon, Reload);

                    if (*weaponFrame > 0)
                        weaponYOffset -= sintable[(*weaponFrame)<<7]>>12;

                    if (*weaponFrame > 0 && doAnim)
                        weaponX += 1-(krand()&3);

                    if (*weaponFrame == 0)
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 146 - halfLookAng, weaponY + 202 - weaponYOffset,
                                               SHOTGUN, weaponShade, weaponBits, weaponPal);
                    }
                    else if (*weaponFrame <= totalTime)
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 146 - halfLookAng, weaponY + 202 - weaponYOffset,
                                               SHOTGUN + 1, weaponShade, weaponBits, weaponPal);
                    }
                    // else we are in 'reload time'
                    else
                    {
                        weaponYOffset -= (*weaponFrame < ((reloadTime - totalTime) / 2 + totalTime))
                                         ? 10 * ((*weaponFrame) - totalTime)    // D
                                         : 10 * (reloadTime - (*weaponFrame));  // U

                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 146 - halfLookAng, weaponY + 202 - weaponYOffset,
                                               SHOTGUN, weaponShade, weaponBits, weaponPal);
                    }

                    break;
                }

                switch (*weaponFrame)
                {
                    case 1:
                    case 2:
                        G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 168 - halfLookAng, weaponY + 201 - weaponYOffset,
                                               SHOTGUN + 2, -128, weaponBits, weaponPal);
                        fallthrough__;
                    case 0:
                    case 6:
                    case 7:
                    case 8:
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 146 - halfLookAng, weaponY + 202 - weaponYOffset,
                                               SHOTGUN, weaponShade, weaponBits, weaponPal);
                        break;

                    case 3:
                    case 4:
                        weaponYOffset -= 40;
                        weaponX += 20;

                        G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 178 - halfLookAng, weaponY + 194 - weaponYOffset,
                                               SHOTGUN + 1 + ((*(weaponFrame)-1) >> 1), -128, weaponBits, weaponPal);
                        fallthrough__;
                    case 5:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 158 - halfLookAng, weaponY + 220 - weaponYOffset,
                                               SHOTGUN + 3, weaponShade, weaponBits, weaponPal);
                        break;

                    case 13:
                    case 14:
                    case 15:
                        G_DrawWeaponTileWithID(currentWeapon, 32 + weaponX + 166 - halfLookAng, weaponY + 210 - weaponYOffset,
                                               SHOTGUN + 4, weaponShade, weaponBits, weaponPal);
                        break;

                    case 16:
                    case 17:
                    case 18:
                    case 19:
                    case 24:
                    case 25:
                    case 26:
                    case 27:
                        G_DrawWeaponTileWithID(currentWeapon, 64 + weaponX + 170 - halfLookAng, weaponY + 196 - weaponYOffset,
                                               SHOTGUN + 5, weaponShade, weaponBits, weaponPal);
                        break;

                    case 20:
                    case 21:
                    case 22:
                    case 23:
                        G_DrawWeaponTileWithID(currentWeapon, 64 + weaponX + 176 - halfLookAng, weaponY + 196 - weaponYOffset,
                                               SHOTGUN + 6, weaponShade, weaponBits, weaponPal);
                        break;


                    case 28:
                    case 29:
                    case 30:
                        G_DrawWeaponTileWithID(currentWeapon, 32 + weaponX + 156 - halfLookAng, weaponY + 206 - weaponYOffset,
                                               SHOTGUN + 4, weaponShade, weaponBits, weaponPal);
                        break;
                }
                break;

            case CHAINGUN_WEAPON:
                if (*weaponFrame > 0)
                {
                    weaponYOffset -= sintable[(*weaponFrame)<<7]>>12;

                    if (doAnim)
                        weaponX += 1-(rand()&3);
                }

                if (WW2GI)
                {
                    int const totalTime = PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime);
                    int const reloadTime = PWEAPON(screenpeek, pPlayer->curr_weapon, Reload);

                    if (*weaponFrame == 0)
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 178 - halfLookAng,weaponY+233-weaponYOffset,
                            CHAINGUN+1,weaponShade,weaponBits,weaponPal);
                    }
                    else if (*weaponFrame <= totalTime)
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 188 - halfLookAng,weaponY+243-weaponYOffset,
                            CHAINGUN+2,weaponShade,weaponBits,weaponPal);
                    }
                    // else we are in 'reload time'
                    // divide reload time into fifths..
                    // 1) move weapon up/right, hand on clip (CHAINGUN - 17)
                    // 2) move weapon up/right, hand removing clip (CHAINGUN - 18)
                    // 3) hold weapon up/right, hand removed clip (CHAINGUN - 19)
                    // 4) hold weapon up/right, hand inserting clip (CHAINGUN - 18)
                    // 5) move weapon down/left, clip inserted (CHAINGUN - 17)
                    else
                    {
                        int iFifths = (reloadTime - totalTime) / 5;
                        if (iFifths < 1)
                            iFifths = 1;

                        if (*weaponFrame < iFifths + totalTime)
                        {
                            // first segment
                            int const weaponOffset = 80 - 10 * (totalTime + iFifths - (*weaponFrame));
                            weaponYOffset += weaponOffset;
                            weaponX += weaponOffset;
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 168 - halfLookAng, weaponY + 260 - weaponYOffset, CHAINGUN - 17,
                                                   weaponShade, weaponBits, weaponPal);
                        }
                        else if (*weaponFrame < (iFifths * 2 + totalTime))
                        {
                            // second segment
                            weaponYOffset += 80; // D
                            weaponX += 80;
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 168 - halfLookAng, weaponY + 260 - weaponYOffset, CHAINGUN - 18,
                                                   weaponShade, weaponBits, weaponPal);
                        }
                        else if (*weaponFrame < (iFifths * 3 + totalTime))
                        {
                            // third segment
                            // up
                            weaponYOffset += 80;
                            weaponX += 80;
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 168 - halfLookAng, weaponY + 260 - weaponYOffset, CHAINGUN - 19,
                                                   weaponShade, weaponBits, weaponPal);
                        }
                        else if (*weaponFrame < (iFifths * 4 + totalTime))
                        {
                            // fourth segment
                            // down
                            weaponYOffset += 80; // D
                            weaponX += 80;
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 168 - halfLookAng, weaponY + 260 - weaponYOffset, CHAINGUN - 18,
                                                   weaponShade, weaponBits, weaponPal);
                        }
                        else
                        {
                            // up and left
                            int const weaponOffset = 10 * (reloadTime - (*weaponFrame));
                            weaponYOffset += weaponOffset; // U
                            weaponX += weaponOffset;
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 168 - halfLookAng, weaponY + 260 - weaponYOffset, CHAINGUN - 17,
                                                   weaponShade, weaponBits, weaponPal);
                        }
                    }

                    break;
                }

                switch (*weaponFrame)
                {
                case 0:
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 178 - (pPlayer->look_ang >> 1), weaponY + 233 - weaponYOffset,
                                           CHAINGUN + 1, weaponShade, weaponBits, weaponPal);
                    break;

                default:
                    if (*weaponFrame > PWEAPON(screenpeek, CHAINGUN_WEAPON, FireDelay) &&
                        *weaponFrame < PWEAPON(screenpeek, CHAINGUN_WEAPON, TotalTime))
                    {
                        int randomOffset = doAnim ? rand()&7 : 0;
                        G_DrawWeaponTileWithID(currentWeapon << 2, randomOffset + weaponX - 4 + 140 - (pPlayer->look_ang >> 1),
                                               randomOffset + weaponY - ((*weaponFrame) >> 1) + 208 - weaponYOffset,
                                               CHAINGUN + 5 + ((*weaponFrame - 4) / 5), weaponShade, weaponBits, weaponPal);
                        if (doAnim) randomOffset = rand()&7;
                        G_DrawWeaponTileWithID(currentWeapon << 2, randomOffset + weaponX - 4 + 184 - (pPlayer->look_ang >> 1),
                                               randomOffset + weaponY - ((*weaponFrame) >> 1) + 208 - weaponYOffset,
                                               CHAINGUN + 5 + ((*weaponFrame - 4) / 5), weaponShade, weaponBits, weaponPal);
                    }

                    if (*weaponFrame < PWEAPON(screenpeek, CHAINGUN_WEAPON, TotalTime)-4)
                    {
                        int const randomOffset = doAnim ? rand()&7 : 0;
                        G_DrawWeaponTileWithID(currentWeapon << 2, randomOffset + weaponX - 4 + 162 - (pPlayer->look_ang >> 1),
                            randomOffset + weaponY - ((*weaponFrame) >> 1) + 208 - weaponYOffset,
                                               CHAINGUN + 5 + ((*weaponFrame - 2) / 5), weaponShade, weaponBits, weaponPal);
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 178 - (pPlayer->look_ang >> 1), weaponY + 233 - weaponYOffset,
                                               CHAINGUN + 1 + ((*weaponFrame) >> 1), weaponShade, weaponBits, weaponPal);
                    }
                    else
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 178 - (pPlayer->look_ang >> 1), weaponY + 233 - weaponYOffset,
                                               CHAINGUN + 1, weaponShade, weaponBits, weaponPal);

                    break;
                }

                G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 168 - (pPlayer->look_ang >> 1), weaponY + 260 - weaponYOffset,
                                       CHAINGUN, weaponShade, weaponBits, weaponPal);
                break;

            case PISTOL_WEAPON:
                if ((*weaponFrame) < PWEAPON(screenpeek, PISTOL_WEAPON, TotalTime)+1)
                {
                    static uint8_t pistolFrames[] = { 0, 1, 2 };
                    int pistolOffset = 195-12+weaponX;

                    if ((*weaponFrame) == PWEAPON(screenpeek, PISTOL_WEAPON, FireDelay))
                        pistolOffset -= 3;

                    G_DrawWeaponTileWithID(currentWeapon, (pistolOffset - (pPlayer->look_ang >> 1)), (weaponY + 244 - weaponYOffset),
                                           FIRSTGUN + pistolFrames[*weaponFrame > 2 ? 0 : *weaponFrame], weaponShade, 2,
                                           weaponPal);

                    break;
                }

                if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING) && DUKE)
                    weaponBits |= 512;

                if ((*weaponFrame) < PWEAPON(screenpeek, PISTOL_WEAPON, Reload) - (NAM_WW2GI ? 40 : 17))
                    G_DrawWeaponTileWithID(currentWeapon, 194 - (pPlayer->look_ang >> 1), weaponY + 230 - weaponYOffset, FIRSTGUN + 4,
                                           weaponShade, weaponBits, weaponPal);
                else if ((*weaponFrame) < PWEAPON(screenpeek, PISTOL_WEAPON, Reload) - (NAM_WW2GI ? 35 : 12))
                {
                    G_DrawWeaponTileWithID(currentWeapon << 1, 244 - ((*weaponFrame) << 3) - (pPlayer->look_ang >> 1),
                                           weaponY + 130 - weaponYOffset + ((*weaponFrame) << 4), FIRSTGUN + 6, weaponShade,
                                           weaponBits, weaponPal);
                    G_DrawWeaponTileWithID(currentWeapon, 224 - (pPlayer->look_ang >> 1), weaponY + 220 - weaponYOffset, FIRSTGUN + 5,
                                           weaponShade, weaponBits, weaponPal);
                }
                else if ((*weaponFrame) < PWEAPON(screenpeek, PISTOL_WEAPON, Reload) - (NAM_WW2GI ? 30 : 7))
                {
                    G_DrawWeaponTileWithID(currentWeapon << 1, 124 + ((*weaponFrame) << 1) - (pPlayer->look_ang >> 1),
                                           weaponY + 430 - weaponYOffset - ((*weaponFrame) << 3), FIRSTGUN + 6, weaponShade,
                                           weaponBits, weaponPal);
                    G_DrawWeaponTileWithID(currentWeapon, 224 - (pPlayer->look_ang >> 1), weaponY + 220 - weaponYOffset, FIRSTGUN + 5,
                                           weaponShade, weaponBits, weaponPal);
                }

                else if ((*weaponFrame) < PWEAPON(screenpeek, PISTOL_WEAPON, Reload) - (NAM_WW2GI ? 12 : 4))
                {
                    G_DrawWeaponTileWithID(currentWeapon << 2, 184 - (pPlayer->look_ang >> 1), weaponY + 235 - weaponYOffset,
                                           FIRSTGUN + 8, weaponShade, weaponBits, weaponPal);
                    G_DrawWeaponTileWithID(currentWeapon, 224 - (pPlayer->look_ang >> 1), weaponY + 210 - weaponYOffset, FIRSTGUN + 5,
                                           weaponShade, weaponBits, weaponPal);
                }
                else if ((*weaponFrame) < PWEAPON(screenpeek, PISTOL_WEAPON, Reload) - (NAM_WW2GI ? 6 : 2))
                {
                    G_DrawWeaponTileWithID(currentWeapon << 2, 164 - (pPlayer->look_ang >> 1), weaponY + 245 - weaponYOffset,
                                           FIRSTGUN + 8, weaponShade, weaponBits, weaponPal);
                    G_DrawWeaponTileWithID(currentWeapon, 224 - (pPlayer->look_ang >> 1), weaponY + 220 - weaponYOffset, FIRSTGUN + 5,
                                           weaponShade, weaponBits, weaponPal);
                }
                else if ((*weaponFrame) < PWEAPON(screenpeek, PISTOL_WEAPON, Reload))
                    G_DrawWeaponTileWithID(currentWeapon, 194 - (pPlayer->look_ang >> 1), weaponY + 235 - weaponYOffset, FIRSTGUN + 5,
                                           weaponShade, weaponBits, weaponPal);

                break;

            case HANDBOMB_WEAPON:
                {
                    static uint8_t pipebombFrames [] = { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2 };

                    if (*weaponFrame >= PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime) || *weaponFrame >= ARRAY_SIZE(pipebombFrames))
                        break;

                    if (*weaponFrame)
                    {
                        if (WW2GI)
                        {
                            int const fireDelay = PWEAPON(screenpeek, pPlayer->curr_weapon, FireDelay);
                            int const totalTime = PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime);

                            if (*weaponFrame <= fireDelay)
                            {
                                // it holds here
                                weaponYOffset -= 5 * (*weaponFrame);  // D
                            }
                            else if (*weaponFrame < ((totalTime - fireDelay) / 2 + fireDelay))
                            {
                                // up and left
                                int const weaponOffset = (*weaponFrame) - fireDelay;
                                weaponYOffset += 10 * weaponOffset;  // U
                                weaponX += 80 * weaponOffset;
                            }
                            else if (*weaponFrame < totalTime)
                            {
                                // start high
                                weaponYOffset += 240;
                                weaponYOffset -= 12 * ((*weaponFrame) - fireDelay);  // D
                                // move left
                                weaponX += 90 - 5 * (totalTime - (*weaponFrame));
                            }
                        }
                        else
                        {
                            if (*weaponFrame < 7)       weaponYOffset -= 10 * (*weaponFrame);  // D
                            else if (*weaponFrame < 12) weaponYOffset += 20 * ((*weaponFrame) - 10);  // U
                            else if (*weaponFrame < 20) weaponYOffset -= 9  * ((*weaponFrame) - 14);  // D
                        }

                        weaponYOffset += 10;
                    }

                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 190 - halfLookAng, weaponY + 260 - weaponYOffset,
                                           HANDTHROW + pipebombFrames[(*weaponFrame)], weaponShade, weaponBits, weaponPal);
                }
                break;

            case HANDREMOTE_WEAPON:
                {
                    static uint8_t remoteFrames[] = { 0, 1, 1, 2, 1, 1, 0, 0, 0, 0, 0 };

                    if (*weaponFrame >= ARRAY_SIZE(remoteFrames))
                        break;

                    weaponX = -48;
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 150 - halfLookAng, weaponY + 258 - weaponYOffset,
                                           HANDREMOTE + remoteFrames[(*weaponFrame)], weaponShade, weaponBits, weaponPal);
                }
                break;

            case DEVISTATOR_WEAPON:
                if (WW2GI)
                {
                    if (*weaponFrame)
                    {
                        int32_t const totalTime = PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime);
                        int32_t const reloadTime = PWEAPON(screenpeek, pPlayer->curr_weapon, Reload);

                        if (*weaponFrame < totalTime)
                        {
                            int const tileOffset = ksgn((*weaponFrame) >> 2);

                            if (pPlayer->ammo_amount[pPlayer->curr_weapon] & 1)
                            {
                                G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 30 - halfLookAng, weaponY + 240 - weaponYOffset,
                                                       DEVISTATOR, weaponShade, weaponBits | 4, weaponPal);
                                G_DrawWeaponTileWithID(currentWeapon, weaponX + 268 - halfLookAng, weaponY + 238 - weaponYOffset,
                                                       DEVISTATOR + tileOffset, -32, weaponBits, weaponPal);
                            }
                            else
                            {
                                G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 30 - halfLookAng, weaponY + 240 - weaponYOffset,
                                                       DEVISTATOR + tileOffset, -32, weaponBits | 4, weaponPal);
                                G_DrawWeaponTileWithID(currentWeapon, weaponX + 268 - halfLookAng, weaponY + 238 - weaponYOffset, DEVISTATOR,
                                                       weaponShade, weaponBits, weaponPal);
                            }
                        }
                        // else we are in 'reload time'
                        else
                        {
                            weaponYOffset -= (*weaponFrame < ((reloadTime - totalTime) / 2 + totalTime))
                                             ? 10 * ((*weaponFrame) - totalTime)
                                             : 10 * (reloadTime - (*weaponFrame));

                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 268 - halfLookAng, weaponY + 238 - weaponYOffset, DEVISTATOR,
                                                   weaponShade, weaponBits, weaponPal);
                            G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 30 - halfLookAng, weaponY + 240 - weaponYOffset, DEVISTATOR,
                                                   weaponShade, weaponBits | 4, weaponPal);
                        }
                    }
                    else
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 268 - halfLookAng, weaponY + 238 - weaponYOffset, DEVISTATOR,
                                               weaponShade, weaponBits, weaponPal);
                        G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 30 - halfLookAng, weaponY + 240 - weaponYOffset, DEVISTATOR,
                                               weaponShade, weaponBits | 4, weaponPal);
                    }
                    break;
                }

                if (*weaponFrame <= PWEAPON(screenpeek, DEVISTATOR_WEAPON, TotalTime) && *weaponFrame > 0)
                {
                    static uint8_t const devastatorFrames[] = { 0, 4, 12, 24, 12, 4, 0 };

                    if (*weaponFrame >= ARRAY_SIZE(devastatorFrames))
                        break;

                    int const tileOffset = ksgn((*weaponFrame) >> 2);

                    if (pPlayer->hbomb_hold_delay)
                    {
                        G_DrawWeaponTileWithID(currentWeapon, (devastatorFrames[*weaponFrame] >> 1) + weaponX + 268 - halfLookAng,
                                               devastatorFrames[*weaponFrame] + weaponY + 238 - weaponYOffset,
                                               DEVISTATOR + tileOffset, -32, weaponBits, weaponPal);
                        G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 30 - halfLookAng, weaponY + 240 - weaponYOffset, DEVISTATOR,
                                               weaponShade, weaponBits | 4, weaponPal);
                    }
                    else
                    {
                        G_DrawWeaponTileWithID(currentWeapon << 1, -(devastatorFrames[*weaponFrame] >> 1) + weaponX + 30 - halfLookAng,
                                               devastatorFrames[*weaponFrame] + weaponY + 240 - weaponYOffset,
                                               DEVISTATOR + tileOffset, -32, weaponBits | 4, weaponPal);
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 268 - halfLookAng, weaponY + 238 - weaponYOffset, DEVISTATOR,
                                               weaponShade, weaponBits, weaponPal);
                    }
                }
                else
                {
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 268 - halfLookAng, weaponY + 238 - weaponYOffset, DEVISTATOR, weaponShade,
                                           weaponBits, weaponPal);
                    G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 30 - halfLookAng, weaponY + 240 - weaponYOffset, DEVISTATOR,
                                           weaponShade, weaponBits | 4, weaponPal);
                }
                break;

            case FREEZE_WEAPON:
                if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING) && DUKE)
                    weaponBits |= 512;

                if ((*weaponFrame) < (PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime) + 1) && (*weaponFrame) > 0)
                {
                    static uint8_t freezerFrames[] = { 0, 0, 1, 1, 2, 2 };

                    if (doAnim)
                    {
                        weaponX += rand() & 3;
                        weaponY += rand() & 3;
                    }
                    weaponYOffset -= 16;
                    G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 210 - (pPlayer->look_ang >> 1), weaponY + 261 - weaponYOffset,
                                           FREEZE + 2, -32, weaponBits, weaponPal);
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 210 - (pPlayer->look_ang >> 1), weaponY + 235 - weaponYOffset,
                                           FREEZE + 3 + freezerFrames[*weaponFrame % 6], -32, weaponBits, weaponPal);
                }
                else
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 210 - (pPlayer->look_ang >> 1), weaponY + 261 - weaponYOffset,
                                           FREEZE, weaponShade, weaponBits, weaponPal);
                break;

            case GROW_WEAPON:
            case SHRINKER_WEAPON:
                weaponX += 28;
                weaponY += 18;

                if (WW2GI)
                {
                    if (*weaponFrame == 0)
                    {
                        // the 'at rest' display
                        if (currentWeapon == GROW_WEAPON)
                        {
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 188 - halfLookAng, weaponY + 240 - weaponYOffset, SHRINKER - 2,
                                                   weaponShade, weaponBits, weaponPal);
                            break;
                        }
                        else if (pPlayer->ammo_amount[currentWeapon] > 0)
                        {
                            G_DrawWeaponTileUnfadedWithID(currentWeapon << 1, weaponX + 184 - halfLookAng, weaponY + 240 - weaponYOffset, SHRINKER + 2,
                                                          16 - (sintable[pPlayer->random_club_frame & 2047] >> 10), weaponBits, 0);
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 188 - halfLookAng, weaponY + 240 - weaponYOffset, SHRINKER,
                                                   weaponShade, weaponBits, weaponPal);
                            break;
                        }
                    }
                    else
                    {
                        // the 'active' display.
                        if (doAnim)
                        {
                            weaponX += rand() & 3;
                            weaponYOffset += rand() & 3;
                        }

                        int const totalTime = PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime);
                        int const reloadTime = PWEAPON(screenpeek, pPlayer->curr_weapon, Reload);

                        if (*weaponFrame < totalTime)
                        {
                            if (*weaponFrame >= PWEAPON(screenpeek, pPlayer->curr_weapon, FireDelay))
                            {
                                // after fire time.
                                // lower weapon to reload cartridge (not clip)
                                weaponYOffset -= (currentWeapon == GROW_WEAPON ? 15 : 10) * (totalTime - (*weaponFrame));
                            }
                        }
                        // else we are in 'reload time'
                        else
                        {
                            weaponYOffset -= (*weaponFrame < ((reloadTime - totalTime) / 2 + totalTime))
                                             ? (currentWeapon == GROW_WEAPON ? 5 : 10) * ((*weaponFrame) - totalTime) // D
                                             : 10 * (reloadTime - (*weaponFrame)); // U
                        }
                    }

                    G_DrawWeaponTileUnfadedWithID(currentWeapon << 1, weaponX + 184 - halfLookAng, weaponY + 240 - weaponYOffset,
                                                  SHRINKER + 3 + ((*weaponFrame) & 3), -32, weaponBits, currentWeapon == GROW_WEAPON ? 2 : 0);

                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 188 - halfLookAng, weaponY + 240 - weaponYOffset,
                                           SHRINKER + (currentWeapon == GROW_WEAPON ? -1 : 1), weaponShade, weaponBits, weaponPal);

                    break;
                }

                if ((*weaponFrame) < PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime) && (*weaponFrame) > 0)
                {
                    if (doAnim)
                    {
                        weaponX += rand() & 3;
                        weaponYOffset += (rand() & 3);
                    }

                    G_DrawWeaponTileUnfadedWithID(currentWeapon << 1, weaponX + 184 - halfLookAng, weaponY + 240 - weaponYOffset,
                                                  SHRINKER + 3 + ((*weaponFrame) & 3), -32, weaponBits, currentWeapon == GROW_WEAPON ? 2 : 0);
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 188 - halfLookAng, weaponY + 240 - weaponYOffset,
                                           currentWeapon == GROW_WEAPON ? SHRINKER - 1 : SHRINKER + 1, weaponShade, weaponBits, weaponPal);
                }
                else
                {
                    G_DrawWeaponTileUnfadedWithID(currentWeapon << 1, weaponX + 184 - halfLookAng, weaponY + 240 - weaponYOffset,
                                                  SHRINKER + 2, 16 - (sintable[pPlayer->random_club_frame & 2047] >> 10), weaponBits,
                                                  currentWeapon == GROW_WEAPON ? 2 : 0);
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 188 - halfLookAng, weaponY + 240 - weaponYOffset,
                                           currentWeapon == GROW_WEAPON ? SHRINKER - 2 : SHRINKER, weaponShade, weaponBits, weaponPal);
                }
                break;
            }
#endif
        }
    }

enddisplayweapon:
    P_DisplaySpit();
}

#define TURBOTURNTIME (TICRATE/8) // 7
#define NORMALTURN   15
#define PREAMBLETURN 5
#define NORMALKEYMOVE 40
#define MAXVEL       ((NORMALKEYMOVE*2)+10)
#define MAXSVEL      ((NORMALKEYMOVE*2)+10)
#define MAXANGVEL    1024
#define MAXHORIZ     256

int32_t g_myAimMode = 0, g_myAimStat = 0, g_oldAimStat = 0;
int32_t mouseyaxismode = -1;

void P_GetInput(int const playerNum)
{
    auto const pPlayer = g_player[playerNum].ps;
    ControlInfo info;

    if ((pPlayer->gm & (MODE_MENU|MODE_TYPE)) || (ud.pause_on && !KB_KeyPressed(sc_Pause)))
    {
        if (!(pPlayer->gm&MODE_MENU))
            CONTROL_GetInput(&info);

        Bmemset(&localInput, 0, sizeof(input_t));

        localInput.bits    = (((int32_t)g_gameQuit) << SK_GAMEQUIT);
        localInput.extbits |= (1<<7);

        return;
    }

    CONTROL_ProcessBinds();

    if (ud.mouseaiming)
        g_myAimMode = BUTTON(gamefunc_Mouse_Aiming);
    else
    {
        g_oldAimStat = g_myAimStat;
        g_myAimStat  = BUTTON(gamefunc_Mouse_Aiming);

        if (g_myAimStat > g_oldAimStat)
        {
            g_myAimMode ^= 1;
            P_DoQuote(QUOTE_MOUSE_AIMING_OFF + g_myAimMode, pPlayer);
        }
    }

    CONTROL_GetInput(&info);

    if (ud.config.MouseDeadZone)
    {
        if (info.mousey > 0)
            info.mousey = max(info.mousey - ud.config.MouseDeadZone, 0);
        else if (info.mousey < 0)
            info.mousey = min(info.mousey + ud.config.MouseDeadZone, 0);

        if (info.mousex > 0)
            info.mousex = max(info.mousex - ud.config.MouseDeadZone, 0);
        else if (info.mousex < 0)
            info.mousex = min(info.mousex + ud.config.MouseDeadZone, 0);
    }

    if (ud.config.MouseBias)
    {
        if (klabs(info.mousex) > klabs(info.mousey))
            info.mousey = tabledivide32_noinline(info.mousey, ud.config.MouseBias);
        else
            info.mousex = tabledivide32_noinline(info.mousex, ud.config.MouseBias);
    }

    // JBF: Run key behaviour is selectable
    int const playerRunning = (ud.runkey_mode) ? (BUTTON(gamefunc_Run) | ud.auto_run) : (ud.auto_run ^ BUTTON(gamefunc_Run));
    int const turnAmount = playerRunning ? (NORMALTURN << 1) : NORMALTURN;
    constexpr int const analogTurnAmount = (NORMALTURN << 1);
    int const keyMove    = playerRunning ? (NORMALKEYMOVE << 1) : NORMALKEYMOVE;
    constexpr int const analogExtent = 32767; // KEEPINSYNC sdlayer.cpp

    input_t input {};

    if (BUTTON(gamefunc_Strafe))
    {
        static int strafeyaw;

        input.svel = -(info.mousex + strafeyaw) >> 3;
        strafeyaw  = (info.mousex + strafeyaw) % 8;

        input.svel -= info.dyaw * keyMove / analogExtent;
    }
    else
    {
        input.q16avel = fix16_div(fix16_from_int(info.mousex), F16(32));
        input.q16avel += fix16_from_int(info.dyaw) / analogExtent * (analogTurnAmount << 1);
    }

    if (g_myAimMode)
        input.q16horz = fix16_div(fix16_from_int(info.mousey), F16(64));
    else
        input.fvel = -(info.mousey >> 6);

    if (ud.mouseflip) input.q16horz = -input.q16horz;

    input.q16horz -= fix16_from_int(info.dpitch) / analogExtent * analogTurnAmount;
    input.svel -= info.dx * keyMove / analogExtent;
    input.fvel -= info.dz * keyMove / analogExtent;

    if (BUTTON(gamefunc_Strafe))
    {
        if (BUTTON(gamefunc_Turn_Left) && !(pPlayer->movement_lock&4))
            input.svel -= -keyMove;

        if (BUTTON(gamefunc_Turn_Right) && !(pPlayer->movement_lock&8))
            input.svel -= keyMove;
    }
    else
    {
        static int32_t turnHeldTime   = 0;
        static int32_t lastInputClock = 0;  // MED
        int32_t const  elapsedTics    = (int32_t) totalclock - lastInputClock;

        lastInputClock = (int32_t) totalclock;

        if (BUTTON(gamefunc_Turn_Left))
        {
            turnHeldTime += elapsedTics;
            input.q16avel -= fix16_from_int((turnHeldTime >= TURBOTURNTIME) ? (turnAmount << 1) : (PREAMBLETURN << 1));
        }
        else if (BUTTON(gamefunc_Turn_Right))
        {
            turnHeldTime += elapsedTics;
            input.q16avel += fix16_from_int((turnHeldTime >= TURBOTURNTIME) ? (turnAmount << 1) : (PREAMBLETURN << 1));
        }
        else
            turnHeldTime=0;
    }

    if (BUTTON(gamefunc_Strafe_Left) && !(pPlayer->movement_lock & 4))
        input.svel += keyMove;

    if (BUTTON(gamefunc_Strafe_Right) && !(pPlayer->movement_lock & 8))
        input.svel += -keyMove;

    if (BUTTON(gamefunc_Move_Forward) && !(pPlayer->movement_lock & 1))
        input.fvel += keyMove;

    if (BUTTON(gamefunc_Move_Backward) && !(pPlayer->movement_lock & 2))
        input.fvel += -keyMove;

    input.fvel = clamp(input.fvel, -MAXVEL, MAXVEL);
    input.svel = clamp(input.svel, -MAXSVEL, MAXSVEL);

    input.q16avel = fix16_clamp(input.q16avel, F16(-MAXANGVEL), F16(MAXANGVEL));
    input.q16horz = fix16_clamp(input.q16horz, F16(-MAXHORIZ), F16(MAXHORIZ));

    int weaponSelection;

    for (weaponSelection = gamefunc_Weapon_10; weaponSelection >= gamefunc_Weapon_1; --weaponSelection)
    {
        if (BUTTON(weaponSelection))
        {
            weaponSelection -= (gamefunc_Weapon_1 - 1);
            break;
        }
    }

    if (BUTTON(gamefunc_Last_Weapon))
        weaponSelection = 14;
    else if (BUTTON(gamefunc_Alt_Weapon))
        weaponSelection = 13;
    else if (BUTTON(gamefunc_Next_Weapon) || (BUTTON(gamefunc_Dpad_Select) && input.fvel > 0))
        weaponSelection = 12;
    else if (BUTTON(gamefunc_Previous_Weapon) || (BUTTON(gamefunc_Dpad_Select) && input.fvel < 0))
        weaponSelection = 11;
    else if (weaponSelection == gamefunc_Weapon_1-1)
        weaponSelection = 0;

    localInput.bits = (weaponSelection << SK_WEAPON_BITS) | (BUTTON(gamefunc_Fire) << SK_FIRE);
    localInput.bits |= (BUTTON(gamefunc_Open) << SK_OPEN);

    int const sectorLotag = pPlayer->cursectnum != -1 ? sector[pPlayer->cursectnum].lotag : 0;
    int const crouchable = sectorLotag != 2 && (sectorLotag != 1 || pPlayer->spritebridge);

    if (BUTTON(gamefunc_Toggle_Crouch))
    {
        pPlayer->crouch_toggle = !pPlayer->crouch_toggle && crouchable;

        if (crouchable)
            CONTROL_ClearButton(gamefunc_Toggle_Crouch);
    }

    if (BUTTON(gamefunc_Crouch) || BUTTON(gamefunc_Jump) || pPlayer->jetpack_on || (!crouchable && pPlayer->on_ground))
        pPlayer->crouch_toggle = 0;

    int const crouching = BUTTON(gamefunc_Crouch) || BUTTON(gamefunc_Toggle_Crouch) || pPlayer->crouch_toggle;

    localInput.bits |= (BUTTON(gamefunc_Jump) << SK_JUMP) | (crouching << SK_CROUCH);

    localInput.bits |= (BUTTON(gamefunc_Aim_Up) || (BUTTON(gamefunc_Dpad_Aiming) && input.fvel > 0)) << SK_AIM_UP;
    localInput.bits |= (BUTTON(gamefunc_Aim_Down) || (BUTTON(gamefunc_Dpad_Aiming) && input.fvel < 0)) << SK_AIM_DOWN;
    localInput.bits |= (BUTTON(gamefunc_Center_View) << SK_CENTER_VIEW);

    localInput.bits |= (BUTTON(gamefunc_Look_Left) << SK_LOOK_LEFT) | (BUTTON(gamefunc_Look_Right) << SK_LOOK_RIGHT);
    localInput.bits |= (BUTTON(gamefunc_Look_Up) << SK_LOOK_UP) | (BUTTON(gamefunc_Look_Down) << SK_LOOK_DOWN);

    localInput.bits |= (playerRunning << SK_RUN);

    localInput.bits |= (BUTTON(gamefunc_Inventory_Left) || (BUTTON(gamefunc_Dpad_Select) && (input.svel > 0 || input.q16avel < 0))) << SK_INV_LEFT;
    localInput.bits |= (BUTTON(gamefunc_Inventory_Right) || (BUTTON(gamefunc_Dpad_Select) && (input.svel < 0 || input.q16avel > 0))) << SK_INV_RIGHT;
    localInput.bits |= (BUTTON(gamefunc_Inventory) << SK_INVENTORY);

    localInput.bits |= (BUTTON(gamefunc_Steroids) << SK_STEROIDS) | (BUTTON(gamefunc_NightVision) << SK_NIGHTVISION);
    localInput.bits |= (BUTTON(gamefunc_MedKit) << SK_MEDKIT) | (BUTTON(gamefunc_Holo_Duke) << SK_HOLODUKE);
    localInput.bits |= (BUTTON(gamefunc_Jetpack) << SK_JETPACK);

    localInput.bits |= BUTTON(gamefunc_Holster_Weapon) << SK_HOLSTER;
    localInput.bits |= BUTTON(gamefunc_Quick_Kick) << SK_QUICK_KICK;
    localInput.bits |= BUTTON(gamefunc_TurnAround) << SK_TURNAROUND;

    localInput.bits |= (g_myAimMode << SK_AIMMODE);
    localInput.bits |= (g_gameQuit << SK_GAMEQUIT);
    localInput.bits |= KB_KeyPressed(sc_Pause) << SK_PAUSE;
    localInput.bits |= ((uint32_t)KB_KeyPressed(sc_Escape)) << SK_ESCAPE;

    if (BUTTON(gamefunc_Dpad_Select))
    {
        input.fvel = 0;
        input.svel = 0;
        input.q16avel = 0;
    }
    else if (BUTTON(gamefunc_Dpad_Aiming))
        input.fvel = 0;

    if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_SEMIAUTO && BUTTON(gamefunc_Fire))
        CONTROL_ClearButton(gamefunc_Fire);

    localInput.extbits = (BUTTON(gamefunc_Move_Forward) || (input.fvel > 0));
    localInput.extbits |= (BUTTON(gamefunc_Move_Backward) || (input.fvel < 0)) << 1;
    localInput.extbits |= (BUTTON(gamefunc_Strafe_Left) || (input.svel > 0)) << 2;
    localInput.extbits |= (BUTTON(gamefunc_Strafe_Right) || (input.svel < 0)) << 3;
    localInput.extbits |= BUTTON(gamefunc_Turn_Left)<<4;
    localInput.extbits |= BUTTON(gamefunc_Turn_Right)<<5;
    localInput.extbits |= BUTTON(gamefunc_Alt_Fire)<<6;

    if (ud.scrollmode && ud.overhead_on)
    {
        ud.folfvel = input.fvel;
        ud.folavel = fix16_to_int(input.q16avel);

        localInput.fvel = 0;
        localInput.svel = 0;

        localInput.q16avel = 0;
        localInput.q16horz = 0;

        return;
    }

    int16_t const q16ang = fix16_to_int(pPlayer->q16ang);

    localInput.fvel = mulscale9(input.fvel, sintable[(q16ang + 2560) & 2047]) +
                      mulscale9(input.svel, sintable[(q16ang + 2048) & 2047]) +
                      (FURY ? 0 : pPlayer->fric.x);

    localInput.svel = mulscale9(input.fvel, sintable[(q16ang + 2048) & 2047]) +
                      mulscale9(input.svel, sintable[(q16ang + 1536) & 2047]) +
                      (FURY ? 0 : pPlayer->fric.y);

    localInput.q16avel = input.q16avel;
    localInput.q16horz = input.q16horz;
}

static int32_t P_DoCounters(int playerNum)
{
    auto const pPlayer = g_player[playerNum].ps;

#ifndef EDUKE32_STANDALONE
    if (FURY)
        goto access_incs; // I'm sorry

    if (pPlayer->invdisptime > 0)
        pPlayer->invdisptime--;

    if (pPlayer->tipincs > 0)
        pPlayer->tipincs--;

    if (pPlayer->last_pissed_time > 0)
    {
        switch (--pPlayer->last_pissed_time)
        {
            case GAMETICSPERSEC * 219:
            {
                A_PlaySound(FLUSH_TOILET, pPlayer->i);
                if (playerNum == screenpeek || GTFLAGS(GAMETYPE_COOPSOUND))
                    A_PlaySound(DUKE_PISSRELIEF, pPlayer->i);
            }
            break;
            case GAMETICSPERSEC * 218:
            {
                pPlayer->holster_weapon = 0;
                pPlayer->weapon_pos     = WEAPON_POS_RAISE;
            }
            break;
        }
    }

    if (pPlayer->crack_time > 0)
    {
        if (--pPlayer->crack_time == 0)
        {
            pPlayer->knuckle_incs = 1;
            pPlayer->crack_time   = PCRACKTIME;
        }
    }

    if (pPlayer->inv_amount[GET_STEROIDS] > 0 && pPlayer->inv_amount[GET_STEROIDS] < 400)
    {
        if (--pPlayer->inv_amount[GET_STEROIDS] == 0)
            P_SelectNextInvItem(pPlayer);

        if (!(pPlayer->inv_amount[GET_STEROIDS] & 7))
            if (playerNum == screenpeek || GTFLAGS(GAMETYPE_COOPSOUND))
                A_PlaySound(DUKE_HARTBEAT, pPlayer->i);
    }

    if (pPlayer->heat_on && pPlayer->inv_amount[GET_HEATS] > 0)
    {
        if (--pPlayer->inv_amount[GET_HEATS] == 0)
        {
            pPlayer->heat_on = 0;
            P_SelectNextInvItem(pPlayer);
            A_PlaySound(NITEVISION_ONOFF, pPlayer->i);
            P_UpdateScreenPal(pPlayer);
        }
    }

    if (pPlayer->holoduke_on >= 0)
    {
        if (--pPlayer->inv_amount[GET_HOLODUKE] <= 0)
        {
            A_PlaySound(TELEPORTER, pPlayer->i);
            pPlayer->holoduke_on = -1;
            P_SelectNextInvItem(pPlayer);
        }
    }

    if (pPlayer->jetpack_on && pPlayer->inv_amount[GET_JETPACK] > 0)
    {
        if (--pPlayer->inv_amount[GET_JETPACK] <= 0)
        {
            pPlayer->jetpack_on = 0;
            P_SelectNextInvItem(pPlayer);
            A_PlaySound(DUKE_JETPACK_OFF, pPlayer->i);
            S_StopEnvSound(DUKE_JETPACK_IDLE, pPlayer->i);
            S_StopEnvSound(DUKE_JETPACK_ON, pPlayer->i);
        }
    }

    if (pPlayer->quick_kick > 0 && sprite[pPlayer->i].pal != 1)
    {
        pPlayer->last_quick_kick = pPlayer->quick_kick + 1;

        if (--pPlayer->quick_kick == 8)
            A_Shoot(pPlayer->i, KNEE);
    }
    else if (pPlayer->last_quick_kick > 0)
        --pPlayer->last_quick_kick;

access_incs:
#endif

    if (pPlayer->access_incs && sprite[pPlayer->i].pal != 1)
    {
        ++pPlayer->access_incs;

        if (sprite[pPlayer->i].extra <= 0)
            pPlayer->access_incs = 12;

        if (pPlayer->access_incs == 12)
        {
            if (pPlayer->access_spritenum >= 0)
            {
                P_ActivateSwitch(playerNum, pPlayer->access_spritenum, 1);
                switch (sprite[pPlayer->access_spritenum].pal)
                {
                    case 0: pPlayer->got_access  &= (0xffff - 0x1); break;
                    case 21: pPlayer->got_access &= (0xffff - 0x2); break;
                    case 23: pPlayer->got_access &= (0xffff - 0x4); break;
                }
                pPlayer->access_spritenum = -1;
            }
            else
            {
                P_ActivateSwitch(playerNum,pPlayer->access_wallnum,0);
                switch (wall[pPlayer->access_wallnum].pal)
                {
                    case 0: pPlayer->got_access  &= (0xffff - 0x1); break;
                    case 21: pPlayer->got_access &= (0xffff - 0x2); break;
                    case 23: pPlayer->got_access &= (0xffff - 0x4); break;
                }
            }
        }

        if (pPlayer->access_incs > 20)
        {
            pPlayer->access_incs  = 0;
            pPlayer->weapon_pos   = WEAPON_POS_RAISE;
            pPlayer->kickback_pic = 0;
        }
    }

    if (pPlayer->cursectnum >= 0 && pPlayer->scuba_on == 0 && sector[pPlayer->cursectnum].lotag == ST_2_UNDERWATER)
    {
        if (pPlayer->inv_amount[GET_SCUBA] > 0)
        {
            pPlayer->scuba_on   = 1;
            pPlayer->inven_icon = ICON_SCUBA;
            P_DoQuote(QUOTE_SCUBA_ON, pPlayer);
        }
        else
        {
            if (pPlayer->airleft > 0)
                --pPlayer->airleft;
            else
            {
                pPlayer->extra_extra8 += 32;
                if (pPlayer->last_extra < (pPlayer->max_player_health >> 1) && (pPlayer->last_extra & 3) == 0)
                    A_PlaySound(DUKE_LONGTERM_PAIN, pPlayer->i);
            }
        }
    }
    else if (pPlayer->inv_amount[GET_SCUBA] > 0 && pPlayer->scuba_on)
    {
        pPlayer->inv_amount[GET_SCUBA]--;
        if (pPlayer->inv_amount[GET_SCUBA] == 0)
        {
            pPlayer->scuba_on = 0;
            P_SelectNextInvItem(pPlayer);
        }
    }

#ifndef EDUKE32_STANDALONE
    if (!FURY && pPlayer->knuckle_incs)
    {
        if (++pPlayer->knuckle_incs == 10)
        {
            if (!WW2GI)
            {
                if (totalclock > 1024)
                    if (playerNum == screenpeek || GTFLAGS(GAMETYPE_COOPSOUND))
                    {
                        if (rand()&1)
                            A_PlaySound(DUKE_CRACK,pPlayer->i);
                        else A_PlaySound(DUKE_CRACK2,pPlayer->i);
                    }

                A_PlaySound(DUKE_CRACK_FIRST,pPlayer->i);
            }
        }
        else if (pPlayer->knuckle_incs == 22 || TEST_SYNC_KEY(g_player[playerNum].input->bits, SK_FIRE))
            pPlayer->knuckle_incs=0;

        return 1;
    }
#endif

    return 0;
}

int16_t WeaponPickupSprites[MAX_WEAPONS] = { KNEE__STATIC, FIRSTGUNSPRITE__STATIC, SHOTGUNSPRITE__STATIC,
        CHAINGUNSPRITE__STATIC, RPGSPRITE__STATIC, HEAVYHBOMB__STATIC, SHRINKERSPRITE__STATIC, DEVISTATORSPRITE__STATIC,
        TRIPBOMBSPRITE__STATIC, FREEZESPRITE__STATIC, HEAVYHBOMB__STATIC, SHRINKERSPRITE__STATIC
                                           };
// this is used for player deaths
void P_DropWeapon(int const playerNum)
{
    auto const pPlayer       = g_player[playerNum].ps;
    int const                 currentWeapon = PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike);

    if ((unsigned)currentWeapon >= MAX_WEAPONS)
        return;

    if (krand() & 1)
        A_Spawn(pPlayer->i, WeaponPickupSprites[currentWeapon]);
#ifndef EDUKE32_STANDALONE
    else if (!FURY)
        switch (PWEAPON(playerNum, currentWeapon, WorksLike))
        {
            case RPG_WEAPON:
            case HANDBOMB_WEAPON: A_Spawn(pPlayer->i, EXPLOSION2); break;
        }
#endif
}

void P_AddAmmo(DukePlayer_t * const pPlayer, int const weaponNum, int const addAmount)
{
    pPlayer->ammo_amount[weaponNum] += addAmount;

    if (pPlayer->ammo_amount[weaponNum] > pPlayer->max_ammo_amount[weaponNum])
        pPlayer->ammo_amount[weaponNum] = pPlayer->max_ammo_amount[weaponNum];
}

static void P_AddWeaponNoSwitch(DukePlayer_t * const p, int const weaponNum)
{
    int const playerNum = P_Get(p->i);  // PASS_SNUM?

    if ((p->gotweapon & (1<<weaponNum)) == 0)
    {
        p->gotweapon |= (1<<weaponNum);

#ifndef EDUKE32_STANDALONE
        if (!FURY && weaponNum == SHRINKER_WEAPON)
            p->gotweapon |= (1<<GROW_WEAPON);
#endif
    }

    if (PWEAPON(playerNum, p->curr_weapon, SelectSound) > 0)
        S_StopEnvSound(PWEAPON(playerNum, p->curr_weapon, SelectSound), p->i);

    if (PWEAPON(playerNum, weaponNum, SelectSound) > 0)
        A_PlaySound(PWEAPON(playerNum, weaponNum, SelectSound), p->i);
}

static void P_ChangeWeapon(DukePlayer_t * const pPlayer, int const weaponNum)
{
    int const    playerNum     = P_Get(pPlayer->i);  // PASS_SNUM?
    int8_t const currentWeapon = pPlayer->curr_weapon;

    if (pPlayer->reloading)
        return;

    int eventReturn = 0;

    if (pPlayer->curr_weapon != weaponNum && VM_HaveEvent(EVENT_CHANGEWEAPON))
        eventReturn = VM_OnEventWithReturn(EVENT_CHANGEWEAPON,pPlayer->i, playerNum, weaponNum);

    if (eventReturn == -1)
        return;

    if (eventReturn != -2)
        pPlayer->curr_weapon = weaponNum;

    pPlayer->random_club_frame = 0;

    if (pPlayer->weapon_pos == 0)
    {
        pPlayer->weapon_pos = -1;
        pPlayer->last_weapon = currentWeapon;
    }
    else if ((unsigned)pPlayer->weapon_pos < WEAPON_POS_RAISE)
    {
        pPlayer->weapon_pos = -pPlayer->weapon_pos;
        pPlayer->last_weapon = currentWeapon;
    }
    else if (pPlayer->last_weapon == weaponNum)
    {
        pPlayer->last_weapon = -1;
        pPlayer->weapon_pos = -pPlayer->weapon_pos;
    }

    if (pPlayer->holster_weapon)
    {
        pPlayer->weapon_pos = WEAPON_POS_RAISE;
        pPlayer->holster_weapon = 0;
        pPlayer->last_weapon = -1;
    }

    if (currentWeapon != pPlayer->curr_weapon &&
        !(PWEAPON(playerNum, currentWeapon, WorksLike) == HANDREMOTE_WEAPON && PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == HANDBOMB_WEAPON) &&
        !(PWEAPON(playerNum, currentWeapon, WorksLike) == HANDBOMB_WEAPON && PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == HANDREMOTE_WEAPON))
    {
        pPlayer->last_used_weapon = currentWeapon;
    }

    pPlayer->kickback_pic = 0;

    P_SetWeaponGamevars(playerNum, pPlayer);
}

void P_AddWeapon(DukePlayer_t *pPlayer, int weaponNum, int switchWeapon)
{
    P_AddWeaponNoSwitch(pPlayer, weaponNum);

    if (switchWeapon)
        P_ChangeWeapon(pPlayer, weaponNum);
}

void P_SelectNextInvItem(DukePlayer_t *pPlayer)
{
    if (pPlayer->inv_amount[GET_FIRSTAID] > 0)
        pPlayer->inven_icon = ICON_FIRSTAID;
    else if (pPlayer->inv_amount[GET_STEROIDS] > 0)
        pPlayer->inven_icon = ICON_STEROIDS;
    else if (pPlayer->inv_amount[GET_JETPACK] > 0)
        pPlayer->inven_icon = ICON_JETPACK;
    else if (pPlayer->inv_amount[GET_HOLODUKE] > 0)
        pPlayer->inven_icon = ICON_HOLODUKE;
    else if (pPlayer->inv_amount[GET_HEATS] > 0)
        pPlayer->inven_icon = ICON_HEATS;
    else if (pPlayer->inv_amount[GET_SCUBA] > 0)
        pPlayer->inven_icon = ICON_SCUBA;
    else if (pPlayer->inv_amount[GET_BOOTS] > 0)
        pPlayer->inven_icon = ICON_BOOTS;
    else
        pPlayer->inven_icon = ICON_NONE;
}

void P_CheckWeapon(DukePlayer_t *pPlayer)
{
    if (pPlayer->reloading || (unsigned)pPlayer->curr_weapon >= MAX_WEAPONS)
        return;

    int playerNum, weaponNum;

    if (pPlayer->wantweaponfire >= 0)
    {
        weaponNum = pPlayer->wantweaponfire;
        pPlayer->wantweaponfire = -1;

        if (weaponNum == pPlayer->curr_weapon)
            return;

        if ((pPlayer->gotweapon & (1<<weaponNum)) && pPlayer->ammo_amount[weaponNum] > 0)
        {
            P_AddWeapon(pPlayer, weaponNum, 1);
            return;
        }
    }

    weaponNum = pPlayer->curr_weapon;

    if ((pPlayer->gotweapon & (1<<weaponNum)) && (pPlayer->ammo_amount[weaponNum] > 0 || !(pPlayer->weaponswitch & 2)))
        return;

    playerNum  = P_Get(pPlayer->i);

    int wpnInc = 0;

    for (wpnInc = 0; wpnInc <= FREEZE_WEAPON; ++wpnInc)
    {
        weaponNum = g_player[playerNum].wchoice[wpnInc];
        if (VOLUMEONE && weaponNum > SHRINKER_WEAPON)
            continue;

        if (weaponNum == KNEE_WEAPON)
            weaponNum = FREEZE_WEAPON;
        else weaponNum--;

        if (weaponNum == KNEE_WEAPON || ((pPlayer->gotweapon & (1<<weaponNum)) && pPlayer->ammo_amount[weaponNum] > 0))
            break;
    }

    if (wpnInc == HANDREMOTE_WEAPON)
        weaponNum = KNEE_WEAPON;

    // Found the weapon

    P_ChangeWeapon(pPlayer, weaponNum);
}

#ifdef LUNATIC
void P_CheckWeaponI(int playerNum)
{
    P_CheckWeapon(g_player[playerNum].ps);
}
#endif

static void DoWallTouchDamage(const DukePlayer_t *pPlayer, int32_t wallNum)
{
    vec3_t const davect = { pPlayer->pos.x + (sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047] >> 9),
                      pPlayer->pos.y + (sintable[fix16_to_int(pPlayer->q16ang) & 2047] >> 9), pPlayer->pos.z };

    A_DamageWall(pPlayer->i, wallNum, davect, -1);
}

static void P_CheckTouchDamage(DukePlayer_t *pPlayer, int touchObject)
{
    if ((touchObject = VM_OnEventWithReturn(EVENT_CHECKTOUCHDAMAGE, pPlayer->i, P_Get(pPlayer->i), touchObject)) == -1)
        return;

    if ((touchObject & 49152) == 49152)
    {
#ifndef EDUKE32_STANDALONE
        int const touchSprite = touchObject & (MAXSPRITES - 1);

        if (!FURY && sprite[touchSprite].picnum == CACTUS)
        {
            if (pPlayer->hurt_delay < 8)
            {
                sprite[pPlayer->i].extra -= 5;

                pPlayer->hurt_delay = 16;
                P_PalFrom(pPlayer, 32, 32, 0, 0);
                A_PlaySound(DUKE_LONGTERM_PAIN, pPlayer->i);
            }
        }
#endif
        return;
    }

    if ((touchObject & 49152) != 32768)
        return;

    int const touchWall = touchObject & (MAXWALLS-1);

    if (pPlayer->hurt_delay > 0)
        pPlayer->hurt_delay--;
    else if (wall[touchWall].cstat & FORCEFIELD_CSTAT)
    {
        int const forcePic = G_GetForcefieldPicnum(touchWall);

        switch (DYNAMICTILEMAP(forcePic))
        {
        case W_FORCEFIELD__STATIC:
            sprite[pPlayer->i].extra -= 5;

            pPlayer->hurt_delay = 16;
            P_PalFrom(pPlayer, 32, 32,0,0);

            pPlayer->vel.x = -(sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]<<8);
            pPlayer->vel.y = -(sintable[(fix16_to_int(pPlayer->q16ang))&2047]<<8);

#ifndef EDUKE32_STANDALONE
            if (!FURY)
                A_PlaySound(DUKE_LONGTERM_PAIN,pPlayer->i);
#endif
            DoWallTouchDamage(pPlayer, touchWall);
            break;

        case BIGFORCE__STATIC:
            pPlayer->hurt_delay = GAMETICSPERSEC;
            DoWallTouchDamage(pPlayer, touchWall);
            break;
        }
    }
}

static int P_CheckFloorDamage(DukePlayer_t *pPlayer, int floorTexture)
{
    auto const pSprite = &sprite[pPlayer->i];

    if ((unsigned)(floorTexture = VM_OnEventWithReturn(EVENT_CHECKFLOORDAMAGE, pPlayer->i, P_Get(pPlayer->i), floorTexture)) >= MAXTILES)
        return 0;

    switch (DYNAMICTILEMAP(floorTexture))
    {
        case HURTRAIL__STATIC:
            if (rnd(32))
            {
                if (pPlayer->inv_amount[GET_BOOTS] > 0)
                    return 1;
                else
                {
#ifndef EDUKE32_STANDALONE
                    if (!FURY)
                    {
                        if (!A_CheckSoundPlaying(pPlayer->i, DUKE_LONGTERM_PAIN))
                            A_PlaySound(DUKE_LONGTERM_PAIN, pPlayer->i);

                        if (!A_CheckSoundPlaying(pPlayer->i, SHORT_CIRCUIT))
                            A_PlaySound(SHORT_CIRCUIT, pPlayer->i);
                    }
#endif

                    P_PalFrom(pPlayer, 32, 64, 64, 64);
                    pSprite->extra -= 1 + (krand() & 3);

                    return 0;
                }
            }
            break;

        case FLOORSLIME__STATIC:
            if (rnd(16))
            {
                if (pPlayer->inv_amount[GET_BOOTS] > 0)
                    return 1;
                else
                {
#ifndef EDUKE32_STANDALONE
                    if (!FURY && !A_CheckSoundPlaying(pPlayer->i, DUKE_LONGTERM_PAIN))
                        A_PlaySound(DUKE_LONGTERM_PAIN, pPlayer->i);
#endif

                    P_PalFrom(pPlayer, 32, 0, 8, 0);
                    pSprite->extra -= 1 + (krand() & 3);

                    return 0;
                }
            }
            break;

#ifndef EDUKE32_STANDALONE
        case FLOORPLASMA__STATIC:
            if (!FURY && rnd(32))
            {
                if (pPlayer->inv_amount[GET_BOOTS] > 0)
                    return 1;
                else
                {
                    if (!A_CheckSoundPlaying(pPlayer->i, DUKE_LONGTERM_PAIN))
                        A_PlaySound(DUKE_LONGTERM_PAIN, pPlayer->i);

                    P_PalFrom(pPlayer, 32, 8, 0, 0);
                    pSprite->extra -= 1 + (krand() & 3);

                    return 0;
                }
            }
            break;
#endif
    }

    return 0;
}


int P_FindOtherPlayer(int playerNum, int32_t *pDist)
{
    int closestPlayer     = playerNum;
    int closestPlayerDist = INT32_MAX;

    for (bssize_t TRAVERSE_CONNECT(otherPlayer))
    {
        if (playerNum != otherPlayer && sprite[g_player[otherPlayer].ps->i].extra > 0)
        {
            int otherPlayerDist = klabs(g_player[otherPlayer].ps->opos.x - g_player[playerNum].ps->pos.x) +
                                  klabs(g_player[otherPlayer].ps->opos.y - g_player[playerNum].ps->pos.y) +
                                  (klabs(g_player[otherPlayer].ps->opos.z - g_player[playerNum].ps->pos.z) >> 4);

            if (otherPlayerDist < closestPlayerDist)
            {
                closestPlayer     = otherPlayer;
                closestPlayerDist = otherPlayerDist;
            }
        }
    }

    *pDist = closestPlayerDist;

    return closestPlayer;
}

void P_FragPlayer(int playerNum)
{
    auto const pPlayer = g_player[playerNum].ps;
    auto const pSprite = &sprite[pPlayer->i];

    if (g_netClient) // [75] The server should not overwrite its own randomseed
        randomseed = ticrandomseed;

    if (pSprite->pal != 1)
    {
        P_PalFrom(pPlayer, 63, 63, 0, 0);

        pPlayer->pos.z -= ZOFFSET2;
        pSprite->z -= ZOFFSET2;

        pPlayer->dead_flag = (512 - ((krand() & 1) << 10) + (krand() & 255) - 512) & 2047;

        if (pPlayer->dead_flag == 0)
            pPlayer->dead_flag++;

#ifndef NETCODE_DISABLE
        if (g_netServer)
        {
            // this packet might not be needed anymore with the new snapshot code
            packbuf[0] = PACKET_FRAG;
            packbuf[1] = playerNum;
            packbuf[2] = pPlayer->frag_ps;
            packbuf[3] = actor[pPlayer->i].picnum;
            B_BUF32(&packbuf[4], ticrandomseed);
            packbuf[8] = myconnectindex;

            enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(&packbuf[0], 9, ENET_PACKET_FLAG_RELIABLE));
        }
#endif
    }

#ifndef EDUKE32_STANDALONE
    if (!FURY)
    {
        pPlayer->jetpack_on  = 0;
        pPlayer->holoduke_on = -1;

        S_StopEnvSound(DUKE_JETPACK_IDLE, pPlayer->i);

        if (pPlayer->scream_voice > FX_Ok)
        {
            FX_StopSound(pPlayer->scream_voice);
            S_Cleanup();
            pPlayer->scream_voice = -1;
        }
    }
#endif

    if (pSprite->pal != 1 && (pSprite->cstat & 32768) == 0)
        pSprite->cstat = 0;

    if ((g_netServer || ud.multimode > 1) && (pSprite->pal != 1 || (pSprite->cstat & 32768)))
    {
        if (pPlayer->frag_ps != playerNum)
        {
            if (GTFLAGS(GAMETYPE_TDM) && g_player[pPlayer->frag_ps].ps->team == g_player[playerNum].ps->team)
                g_player[pPlayer->frag_ps].ps->fraggedself++;
            else
            {
                g_player[pPlayer->frag_ps].ps->frag++;
                g_player[pPlayer->frag_ps].frags[playerNum]++;
                g_player[playerNum].frags[playerNum]++;  // deaths
            }

            if (playerNum == screenpeek)
            {
                Bsprintf(apStrings[QUOTE_RESERVED], "Killed by %s", &g_player[pPlayer->frag_ps].user_name[0]);
                P_DoQuote(QUOTE_RESERVED, pPlayer);
            }
            else
            {
                Bsprintf(apStrings[QUOTE_RESERVED2], "Killed %s", &g_player[playerNum].user_name[0]);
                P_DoQuote(QUOTE_RESERVED2, g_player[pPlayer->frag_ps].ps);
            }

            if (ud.obituaries)
            {
                Bsprintf(tempbuf, apStrings[OBITQUOTEINDEX + (krand() % g_numObituaries)],
                         &g_player[pPlayer->frag_ps].user_name[0], &g_player[playerNum].user_name[0]);
                G_AddUserQuote(tempbuf);
            }
            else
                krand();
        }
        else
        {
            if (actor[pPlayer->i].picnum != APLAYERTOP)
            {
                pPlayer->fraggedself++;
                if ((unsigned)pPlayer->wackedbyactor < MAXTILES && A_CheckEnemyTile(sprite[pPlayer->wackedbyactor].picnum))
                    Bsprintf(tempbuf, apStrings[OBITQUOTEINDEX + (krand() % g_numObituaries)], "A monster",
                             &g_player[playerNum].user_name[0]);
                else if (actor[pPlayer->i].picnum == NUKEBUTTON)
                    Bsprintf(tempbuf, "^02%s^02 tried to leave", &g_player[playerNum].user_name[0]);
                else
                {
                    // random suicide death string
                    Bsprintf(tempbuf, apStrings[SUICIDEQUOTEINDEX + (krand() % g_numSelfObituaries)],
                             &g_player[playerNum].user_name[0]);
                }
            }
            else
                Bsprintf(tempbuf, "^02%s^02 switched to team %d", &g_player[playerNum].user_name[0], pPlayer->team + 1);

            if (ud.obituaries)
                G_AddUserQuote(tempbuf);
        }
        pPlayer->frag_ps = playerNum;
        pus              = NUMPAGES;
    }
}

#ifdef LUNATIC
# define PIPEBOMB_CONTROL(playerNum) (g_player[playerNum].ps->pipebombControl)
#else
# define PIPEBOMB_CONTROL(playerNum) (Gv_GetVarByLabel("PIPEBOMB_CONTROL", PIPEBOMB_REMOTE, -1, playerNum))
#endif

static void P_ProcessWeapon(int playerNum)
{
    auto const     pPlayer      = g_player[playerNum].ps;
    uint8_t *const weaponFrame  = &pPlayer->kickback_pic;
    int const      playerShrunk = (sprite[pPlayer->i].yrepeat < 32);
    uint32_t       playerBits   = g_player[playerNum].input->bits;

    switch (pPlayer->weapon_pos)
    {
        case WEAPON_POS_LOWER:
            if (pPlayer->last_weapon >= 0)
            {
                pPlayer->weapon_pos  = WEAPON_POS_RAISE;
                pPlayer->last_weapon = -1;
            }
            else if (pPlayer->holster_weapon == 0)
                pPlayer->weapon_pos = WEAPON_POS_RAISE;
            break;
        case 0: break;
        default: pPlayer->weapon_pos--; break;
    }

    if (TEST_SYNC_KEY(playerBits, SK_FIRE))
    {
        P_SetWeaponGamevars(playerNum, pPlayer);

        if (VM_OnEvent(EVENT_PRESSEDFIRE, pPlayer->i, playerNum) != 0)
            playerBits &= ~BIT(SK_FIRE);
    }

    if (TEST_SYNC_KEY(playerBits, SK_HOLSTER))   // 'Holster Weapon
    {
        P_SetWeaponGamevars(playerNum, pPlayer);

        if (VM_OnEvent(EVENT_HOLSTER, pPlayer->i, playerNum) == 0)
        {
            if (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) != KNEE_WEAPON)
            {
                if (pPlayer->holster_weapon == 0 && pPlayer->weapon_pos == 0)
                {
                    pPlayer->holster_weapon = 1;
                    pPlayer->weapon_pos     = -1;
                    P_DoQuote(QUOTE_WEAPON_LOWERED, pPlayer);
                }
                else if (pPlayer->holster_weapon == 1 && pPlayer->weapon_pos == WEAPON_POS_LOWER)
                {
                    pPlayer->holster_weapon = 0;
                    pPlayer->weapon_pos = WEAPON_POS_RAISE;
                    P_DoQuote(QUOTE_WEAPON_RAISED,pPlayer);
                }
            }

            if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_HOLSTER_CLEARS_CLIP)
            {
                int const weap = pPlayer->curr_weapon, clipcnt = PWEAPON(playerNum, weap, Clip);

                if (pPlayer->ammo_amount[weap] > clipcnt && (pPlayer->ammo_amount[weap] % clipcnt) != 0)
                {
                    pPlayer->ammo_amount[weap] -= pPlayer->ammo_amount[weap] % clipcnt;
                    *weaponFrame                = PWEAPON(playerNum, weap, TotalTime);
                    playerBits                 &= ~BIT(SK_FIRE);  // not firing...
                }

                return;
            }
        }
    }

    if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_GLOWS)
    {
        pPlayer->random_club_frame += 64; // Glowing

#ifdef POLYMER
        if (pPlayer->kickback_pic == 0)
        {
            auto const pSprite     = &sprite[pPlayer->i];
            int const         glowXOffset = ((sintable[(pSprite->ang + 512) & 2047]) >> 7);
            int const         glowYOffset = ((sintable[(pSprite->ang) & 2047]) >> 7);
            int const         glowRange   = 1024 + (sintable[pPlayer->random_club_frame & 2047] >> 3);

            pSprite->x += glowXOffset;
            pSprite->y += glowYOffset;

            G_AddGameLight(0, pPlayer->i, PHEIGHT, max(glowRange, 0),
                           PWEAPON(playerNum, pPlayer->curr_weapon, FlashColor), PR_LIGHT_PRIO_HIGH_GAME);

            actor[pPlayer->i].lightcount = 2;

            pSprite->x -= glowXOffset;
            pSprite->y -= glowYOffset;
        }
#endif
    }

    // this is a hack for WEAPON_FIREEVERYOTHER
    if (actor[pPlayer->i].t_data[7])
    {
        actor[pPlayer->i].t_data[7]--;
        if (pPlayer->last_weapon == -1 && actor[pPlayer->i].t_data[7] != 0 && ((actor[pPlayer->i].t_data[7] & 1) == 0))
        {
            if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_AMMOPERSHOT)
            {
                if (pPlayer->ammo_amount[pPlayer->curr_weapon] > 0)
                    pPlayer->ammo_amount[pPlayer->curr_weapon]--;
                else
                {
                    actor[pPlayer->i].t_data[7] = 0;
                    P_CheckWeapon(pPlayer);
                }
            }

            if (actor[pPlayer->i].t_data[7] != 0)
                A_Shoot(pPlayer->i,PWEAPON(playerNum, pPlayer->curr_weapon, Shoots));
        }
    }

    if (pPlayer->rapid_fire_hold == 1)
    {
        if (TEST_SYNC_KEY(playerBits, SK_FIRE))
            return;
        pPlayer->rapid_fire_hold = 0;
    }

    bool const doFire    = (playerBits & BIT(SK_FIRE) && (*weaponFrame) == 0);
    bool const doAltFire = g_player[playerNum].input->extbits & (1 << 6);

    if (doAltFire)
    {
        P_SetWeaponGamevars(playerNum, pPlayer);
        VM_OnEvent(EVENT_ALTFIRE, pPlayer->i, playerNum);
    }

    if (playerShrunk || pPlayer->tipincs || pPlayer->access_incs)
        playerBits &= ~BIT(SK_FIRE);
    else if (doFire && pPlayer->fist_incs == 0 &&
             pPlayer->last_weapon == -1 && (pPlayer->weapon_pos == 0 || pPlayer->holster_weapon == 1))
    {
        pPlayer->crack_time = PCRACKTIME;

        if (pPlayer->holster_weapon == 1)
        {
            if (pPlayer->last_pissed_time <= (GAMETICSPERSEC * 218) && pPlayer->weapon_pos == WEAPON_POS_LOWER)
            {
                pPlayer->holster_weapon = 0;
                pPlayer->weapon_pos     = WEAPON_POS_RAISE;
                P_DoQuote(QUOTE_WEAPON_RAISED, pPlayer);
            }
        }
        else
        {
            P_SetWeaponGamevars(playerNum, pPlayer);

            if (doFire && VM_OnEvent(EVENT_FIRE, pPlayer->i, playerNum) == 0)
            {
                // this event is deprecated
                VM_OnEvent(EVENT_FIREWEAPON, pPlayer->i, playerNum);

                switch (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike))
                {
                    case HANDBOMB_WEAPON:
                        pPlayer->hbomb_hold_delay = 0;
                        if (pPlayer->ammo_amount[pPlayer->curr_weapon] > 0)
                        {
                            (*weaponFrame) = 1;
                            if (PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                                A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                        }
                        break;

                    case HANDREMOTE_WEAPON:
                        pPlayer->hbomb_hold_delay = 0;
                        (*weaponFrame)            = 1;
                        if (PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                            A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                        break;


                    case TRIPBOMB_WEAPON:
                        if (pPlayer->ammo_amount[pPlayer->curr_weapon] > 0)
                        {
                            hitdata_t hitData;

                            hitscan((const vec3_t *)pPlayer, pPlayer->cursectnum, sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047],
                                    sintable[fix16_to_int(pPlayer->q16ang) & 2047], fix16_to_int(F16(100) - pPlayer->q16horiz - pPlayer->q16horizoff) * 32, &hitData,
                                    CLIPMASK1);

                            if ((hitData.sect < 0 || hitData.sprite >= 0) ||
                                (hitData.wall >= 0 && sector[hitData.sect].lotag > 2))
                                break;

                            if (hitData.wall >= 0 && wall[hitData.wall].overpicnum >= 0)
                                if (wall[hitData.wall].overpicnum == BIGFORCE)
                                    break;

                            int spriteNum = headspritesect[hitData.sect];
                            while (spriteNum >= 0)
                            {
                                if (sprite[spriteNum].picnum == TRIPBOMB && klabs(sprite[spriteNum].z - hitData.pos.z) < ZOFFSET4 &&
                                    ((sprite[spriteNum].x - hitData.pos.x) * (sprite[spriteNum].x - hitData.pos.x) +
                                     (sprite[spriteNum].y - hitData.pos.y) * (sprite[spriteNum].y - hitData.pos.y)) < (290 * 290))
                                    break;
                                spriteNum = nextspritesect[spriteNum];
                            }

                            // ST_2_UNDERWATER
                            if (spriteNum == -1 && hitData.wall >= 0 && (wall[hitData.wall].cstat & 16) == 0)
                                if ((wall[hitData.wall].nextsector >= 0 && sector[wall[hitData.wall].nextsector].lotag <= 2) ||
                                    (wall[hitData.wall].nextsector == -1 && sector[hitData.sect].lotag <= 2))
                                    if (((hitData.pos.x - pPlayer->pos.x) * (hitData.pos.x - pPlayer->pos.x) +
                                         (hitData.pos.y - pPlayer->pos.y) * (hitData.pos.y - pPlayer->pos.y)) < (290 * 290))
                                    {
                                        pPlayer->pos.z = pPlayer->opos.z;
                                        pPlayer->vel.z = 0;
                                        (*weaponFrame) = 1;
                                        if (PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                                        {
                                            A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                                        }
                                    }
                        }
                        break;

                    case PISTOL_WEAPON:
                    case SHOTGUN_WEAPON:
                    case CHAINGUN_WEAPON:
                    case SHRINKER_WEAPON:
                    case GROW_WEAPON:
                    case FREEZE_WEAPON:
                    case RPG_WEAPON:
                        if (pPlayer->ammo_amount[pPlayer->curr_weapon] > 0)
                        {
                            (*weaponFrame) = 1;
                            if (PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                                A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                        }
                        break;

                    case DEVISTATOR_WEAPON:
                        if (pPlayer->ammo_amount[pPlayer->curr_weapon] > 0)
                        {
                            (*weaponFrame)            = 1;
                            pPlayer->hbomb_hold_delay = !pPlayer->hbomb_hold_delay;
                            if (PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                                A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                        }
                        break;

                    case KNEE_WEAPON:
                        if (pPlayer->quick_kick == 0)
                        {
                            (*weaponFrame) = 1;
                            if (PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                                A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                        }
                        break;
                }
            }
        }
    }
    else if (*weaponFrame)
    {
        if (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == HANDBOMB_WEAPON)
        {
            if (PWEAPON(playerNum, pPlayer->curr_weapon, HoldDelay) && ((*weaponFrame) == PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay)) && TEST_SYNC_KEY(playerBits, SK_FIRE))
            {
                pPlayer->rapid_fire_hold = 1;
                return;
            }

            if (++(*weaponFrame) == PWEAPON(playerNum, pPlayer->curr_weapon, HoldDelay))
            {
                pPlayer->ammo_amount[pPlayer->curr_weapon]--;

                if (numplayers < 2 || g_netServer)
                {
                    int pipeBombType;
                    int pipeBombZvel;
                    int pipeBombFwdVel;

                    if (pPlayer->on_ground && TEST_SYNC_KEY(playerBits, SK_CROUCH))
                    {
                        pipeBombFwdVel = 15;
                        pipeBombZvel   = (fix16_to_int(pPlayer->q16horiz + pPlayer->q16horizoff - F16(100)) * 20);
                    }
                    else
                    {
                        pipeBombFwdVel = 140;
                        pipeBombZvel   = -512 - (fix16_to_int(pPlayer->q16horiz + pPlayer->q16horizoff - F16(100)) * 20);
                    }

                    int pipeSpriteNum = A_InsertSprite(pPlayer->cursectnum,
                                       pPlayer->pos.x+(sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]>>6),
                                       pPlayer->pos.y+(sintable[fix16_to_int(pPlayer->q16ang)&2047]>>6),
                                       pPlayer->pos.z,PWEAPON(playerNum, pPlayer->curr_weapon, Shoots),-16,9,9,
                                       fix16_to_int(pPlayer->q16ang),(pipeBombFwdVel+(pPlayer->hbomb_hold_delay<<5)),pipeBombZvel,pPlayer->i,1);

                    pipeBombType = PIPEBOMB_CONTROL(playerNum);

                    if (pipeBombType & PIPEBOMB_TIMER)
                    {
#ifdef LUNATIC
                        int pipeLifeTime     = g_player[playerNum].ps->pipebombLifetime;
                        int pipeLifeVariance = g_player[playerNum].ps->pipebombLifetimeVar;
#else
                        int pipeLifeTime     = Gv_GetVarByLabel("GRENADE_LIFETIME", NAM_GRENADE_LIFETIME, -1, playerNum);
                        int pipeLifeVariance = Gv_GetVarByLabel("GRENADE_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, -1, playerNum);
#endif
                        actor[pipeSpriteNum].t_data[7]= pipeLifeTime
                                            + mulscale14(krand(), pipeLifeVariance)
                                            - pipeLifeVariance;
                        // TIMER_CONTROL
                        actor[pipeSpriteNum].t_data[6]=1;
                    }
                    else actor[pipeSpriteNum].t_data[6]=2;

                    if (pipeBombFwdVel == 15)
                    {
                        sprite[pipeSpriteNum].yvel = 3;
                        sprite[pipeSpriteNum].z += ZOFFSET3;
                    }

                    if (A_GetHitscanRange(pPlayer->i) < 512)
                    {
                        sprite[pipeSpriteNum].ang += 1024;
                        sprite[pipeSpriteNum].zvel /= 3;
                        sprite[pipeSpriteNum].xvel /= 3;
                    }
                }

                pPlayer->hbomb_on = 1;
            }
            else if ((*weaponFrame) < PWEAPON(playerNum, pPlayer->curr_weapon, HoldDelay) && TEST_SYNC_KEY(playerBits, SK_FIRE))
                pPlayer->hbomb_hold_delay++;
            else if ((*weaponFrame) > PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime))
            {
                (*weaponFrame) = 0;
                pPlayer->weapon_pos = WEAPON_POS_RAISE;
                if (PIPEBOMB_CONTROL(playerNum) == PIPEBOMB_REMOTE)
                {
                    pPlayer->curr_weapon = HANDREMOTE_WEAPON;
                    pPlayer->last_weapon = -1;
                }
                else P_CheckWeapon(pPlayer);
            }
        }
        else if (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == HANDREMOTE_WEAPON)
        {
            if (++(*weaponFrame) == PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay))
            {
                if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_BOMB_TRIGGER)
                    pPlayer->hbomb_on = 0;

                if (PWEAPON(playerNum, pPlayer->curr_weapon, Shoots) != 0)
                {
                    if (!(PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_NOVISIBLE))
                    {
                        lastvisinc = (int32_t) totalclock+32;
                        pPlayer->visibility = 0;
                    }

                    P_SetWeaponGamevars(playerNum, pPlayer);
                    A_Shoot(pPlayer->i, PWEAPON(playerNum, pPlayer->curr_weapon, Shoots));
                }
            }

            if ((*weaponFrame) >= PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime))
            {
                (*weaponFrame) = 0;
                if ((pPlayer->ammo_amount[HANDBOMB_WEAPON] > 0) && PIPEBOMB_CONTROL(playerNum) == PIPEBOMB_REMOTE)
                    P_AddWeapon(pPlayer, HANDBOMB_WEAPON, 1);
                else P_CheckWeapon(pPlayer);
            }
        }
        else
        {
            // the basic weapon...
            (*weaponFrame)++;

            if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_CHECKATRELOAD)
            {
                if (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == TRIPBOMB_WEAPON)
                {
                    if ((*weaponFrame) >= PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime))
                    {
                        (*weaponFrame) = 0;
                        P_CheckWeapon(pPlayer);
                        pPlayer->weapon_pos = WEAPON_POS_LOWER;
                    }
                }
                else if (*weaponFrame >= PWEAPON(playerNum, pPlayer->curr_weapon, Reload))
                    P_CheckWeapon(pPlayer);
            }
            else if (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike)!=KNEE_WEAPON && *weaponFrame >= PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay))
                P_CheckWeapon(pPlayer);

            if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_STANDSTILL
                    && *weaponFrame < (PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay)+1))
            {
                pPlayer->pos.z = pPlayer->opos.z;
                pPlayer->vel.z = 0;
            }

            if (*weaponFrame == PWEAPON(playerNum, pPlayer->curr_weapon, Sound2Time))
                if (PWEAPON(playerNum, pPlayer->curr_weapon, Sound2Sound) > 0)
                    A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, Sound2Sound),pPlayer->i);

            if (*weaponFrame == PWEAPON(playerNum, pPlayer->curr_weapon, SpawnTime))
                P_DoWeaponSpawn(playerNum);

            if ((*weaponFrame) >= PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime))
            {
                if (/*!(PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_CHECKATRELOAD) && */ pPlayer->reloading == 1 ||
                        (PWEAPON(playerNum, pPlayer->curr_weapon, Reload) > PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime) && pPlayer->ammo_amount[pPlayer->curr_weapon] > 0
                         && (PWEAPON(playerNum, pPlayer->curr_weapon, Clip)) && (((pPlayer->ammo_amount[pPlayer->curr_weapon]%(PWEAPON(playerNum, pPlayer->curr_weapon, Clip)))==0))))
                {
                    int const weaponReloadTime = PWEAPON(playerNum, pPlayer->curr_weapon, Reload)
                                               - PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime);

                    pPlayer->reloading = 1;

                    if ((*weaponFrame) != (PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime)))
                    {
                        if ((*weaponFrame) == (PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime)+1))
                        {
                            if (PWEAPON(playerNum, pPlayer->curr_weapon, ReloadSound1) > 0)
                                A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, ReloadSound1), pPlayer->i);
                        }
                        else if (((*weaponFrame) ==
                                  (PWEAPON(playerNum, pPlayer->curr_weapon, Reload) - (weaponReloadTime / 3)) &&
                                  !(PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_RELOAD_TIMING)) ||
                                 ((*weaponFrame) ==
                                  (PWEAPON(playerNum, pPlayer->curr_weapon, Reload) - weaponReloadTime + 4) &&
                                  (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_RELOAD_TIMING)))
                        {
                            if (PWEAPON(playerNum, pPlayer->curr_weapon, ReloadSound2) > 0)
                                A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, ReloadSound2), pPlayer->i);
                        }
                        else if ((*weaponFrame) >= (PWEAPON(playerNum, pPlayer->curr_weapon, Reload)))
                        {
                            *weaponFrame       = 0;
                            pPlayer->reloading = 0;
                        }
                    }
                }
                else
                {
                    if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_AUTOMATIC &&
                            (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike)==KNEE_WEAPON || pPlayer->ammo_amount[pPlayer->curr_weapon] > 0))
                    {
                        if (TEST_SYNC_KEY(playerBits, SK_FIRE))
                        {
                            *weaponFrame =
                            (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_RANDOMRESTART) ? 1 + (krand() & 3) : 1;
                        }
                        else *weaponFrame = 0;
                    }
                    else *weaponFrame = 0;

                    if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_RESET &&
                        ((PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == KNEE_WEAPON)
                         || pPlayer->ammo_amount[pPlayer->curr_weapon] > 0))
                    {
                        *weaponFrame = !!(TEST_SYNC_KEY(playerBits, SK_FIRE));
                    }
                }
            }
            else if (*weaponFrame >= PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay)
                     && ((PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == KNEE_WEAPON) || pPlayer->ammo_amount[pPlayer->curr_weapon] > 0))
            {
                if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_AUTOMATIC)
                {
                    if (!(PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_SEMIAUTO))
                    {
                        if (TEST_SYNC_KEY(playerBits, SK_FIRE) == 0 && (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_RESET || WW2GI))
                            *weaponFrame = PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime);
                        if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_FIREEVERYTHIRD)
                        {
                            if (((*(weaponFrame))%3) == 0)
                            {
                                P_FireWeapon(playerNum);
                                P_DoWeaponSpawn(playerNum);
                            }
                        }
                        else if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_FIREEVERYOTHER)
                        {
                            P_FireWeapon(playerNum);
                            P_DoWeaponSpawn(playerNum);
                        }
                        else
                        {
                            if (*weaponFrame == PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay))
                            {
                                P_FireWeapon(playerNum);
//                                P_DoWeaponSpawn(snum);
                            }
                        }
                        if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_RESET
                            && (*weaponFrame) > PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime)
                                                - PWEAPON(playerNum, pPlayer->curr_weapon, HoldDelay)
                            && ((PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == KNEE_WEAPON)
                                || pPlayer->ammo_amount[pPlayer->curr_weapon] > 0))
                        {
                            *weaponFrame = !!(TEST_SYNC_KEY(playerBits, SK_FIRE));
                        }
                    }
                    else
                    {
                        if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_FIREEVERYOTHER)
                        {
                            P_FireWeapon(playerNum);
                            P_DoWeaponSpawn(playerNum);
                        }
                        else
                        {
                            if (*weaponFrame == PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay))
                                P_FireWeapon(playerNum);
                        }
                    }
                }
                else if (*weaponFrame == PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay))
                    P_FireWeapon(playerNum);
            }
        }
    }
}

void P_EndLevel(void)
{
    for (bssize_t TRAVERSE_CONNECT(playerNum))
        g_player[playerNum].ps->gm = MODE_EOL;

    if (ud.from_bonus)
    {
        ud.level_number   = ud.from_bonus;
        ud.m_level_number = ud.level_number;
        ud.from_bonus     = 0;
    }
    else
    {
        ud.level_number   = (++ud.level_number < MAXLEVELS) ? ud.level_number : 0;
        ud.m_level_number = ud.level_number;
    }
}

static int P_DoFist(DukePlayer_t *pPlayer)
{
    // the fist punching NUKEBUTTON

#ifndef EDUKE32_STANDALONE
    if (FURY)
        return 0;

    if (++(pPlayer->fist_incs) == 28)
    {
        if (ud.recstat == 1)
            G_CloseDemoWrite();

        S_PlaySound(PIPEBOMB_EXPLODE);
        P_PalFrom(pPlayer, 48, 64, 64, 64);
    }

    if (pPlayer->fist_incs > 42)
    {
        if (pPlayer->buttonpalette && ud.from_bonus == 0)
        {
            for (bssize_t TRAVERSE_CONNECT(playerNum))
                g_player[playerNum].ps->gm = MODE_EOL;

            ud.from_bonus = ud.level_number + 1;

            if ((unsigned)ud.secretlevel <= MAXLEVELS)
                ud.level_number = ud.secretlevel - 1;

            ud.m_level_number = ud.level_number;
        }
        else
            P_EndLevel();

        pPlayer->fist_incs = 0;

        return 1;
    }
#else
    UNREFERENCED_PARAMETER(pPlayer);
#endif

    return 0;
}

#ifdef YAX_ENABLE
static void getzsofslope_player(int sectNum, int playerX, int playerY, int32_t *pCeilZ, int32_t *pFloorZ)
{
    int didCeiling = 0;

    if ((sector[sectNum].ceilingstat & 512) == 0)
    {
        int const neighborSect = yax_getneighborsect(playerX, playerY, sectNum, YAX_CEILING);

        if (neighborSect >= 0)
        {
            *pCeilZ    = getceilzofslope(neighborSect, playerX, playerY);
            didCeiling = 1;
        }
    }

    int didFloor   = 0;

    if ((sector[sectNum].floorstat & 512) == 0)
    {
        int const neighborSect = yax_getneighborsect(playerX, playerY, sectNum, YAX_FLOOR);

        if (neighborSect >= 0)
        {
            *pFloorZ = getflorzofslope(neighborSect, playerX, playerY);
            didFloor = 1;
        }
    }

    if (!didCeiling || !didFloor)
    {
        int32_t ceilingZ, floorZ;
        getzsofslope(sectNum, playerX, playerY, &ceilingZ, &floorZ);

        if (!didCeiling)
            *pCeilZ = ceilingZ;

        if (!didFloor)
            *pFloorZ = floorZ;
    }
}
#endif

void P_UpdatePosWhenViewingCam(DukePlayer_t *pPlayer)
{
    int const newOwner      = pPlayer->newowner;
    pPlayer->pos            = sprite[newOwner].pos;
    pPlayer->q16ang         = fix16_from_int(SA(newOwner));
    pPlayer->vel.x          = 0;
    pPlayer->vel.y          = 0;
    sprite[pPlayer->i].xvel = 0;
    pPlayer->look_ang       = 0;
    pPlayer->rotscrnang     = 0;
}

static void P_DoWater(int const playerNum, int const playerBits, int const floorZ, int const ceilZ)
{
    auto const pPlayer = g_player[playerNum].ps;

    // under water
    pPlayer->pycount        += 32;
    pPlayer->pycount        &= 2047;
    pPlayer->jumping_counter = 0;
    pPlayer->pyoff           = sintable[pPlayer->pycount] >> 7;

    if (!A_CheckSoundPlaying(pPlayer->i, DUKE_UNDERWATER))
        A_PlaySound(DUKE_UNDERWATER, pPlayer->i);

    if (TEST_SYNC_KEY(playerBits, SK_JUMP))
    {
        if (VM_OnEvent(EVENT_SWIMUP, pPlayer->i, playerNum) == 0)
            pPlayer->vel.z = max(min(-348, pPlayer->vel.z - 348), -(256 * 6));
    }
    else if (TEST_SYNC_KEY(playerBits, SK_CROUCH))
    {
        if (VM_OnEvent(EVENT_SWIMDOWN, pPlayer->i, playerNum) == 0)
            pPlayer->vel.z = min(max(348, pPlayer->vel.z + 348), (256 * 6));
    }
    else
    {
        // normal view
        if (pPlayer->vel.z < 0)
            pPlayer->vel.z = min(0, pPlayer->vel.z + 256);

        if (pPlayer->vel.z > 0)
            pPlayer->vel.z = max(0, pPlayer->vel.z - 256);
    }

    if (pPlayer->vel.z > 2048)
        pPlayer->vel.z >>= 1;

    pPlayer->pos.z += pPlayer->vel.z;

    if (pPlayer->pos.z > (floorZ-(15<<8)))
        pPlayer->pos.z += ((floorZ-(15<<8))-pPlayer->pos.z)>>1;

    if (pPlayer->pos.z < ceilZ)
    {
        pPlayer->pos.z = ceilZ;
        pPlayer->vel.z = 0;
    }

    if ((pPlayer->on_warping_sector == 0 || ceilZ != pPlayer->truecz) && pPlayer->pos.z < ceilZ + PMINHEIGHT)
    {
        pPlayer->pos.z = ceilZ + PMINHEIGHT;
        pPlayer->vel.z = 0;
    }

    if (pPlayer->scuba_on && (krand()&255) < 8)
    {
        int const spriteNum = A_Spawn(pPlayer->i, WATERBUBBLE);
        int const q16ang      = fix16_to_int(pPlayer->q16ang);

        sprite[spriteNum].x      += sintable[(q16ang + 512 + 64 - (g_globalRandom & 128)) & 2047] >> 6;
        sprite[spriteNum].y      += sintable[(q16ang + 64 - (g_globalRandom & 128)) & 2047] >> 6;
        sprite[spriteNum].xrepeat = 3;
        sprite[spriteNum].yrepeat = 2;
        sprite[spriteNum].z       = pPlayer->pos.z + ZOFFSET3;
    }
}
static void P_DoJetpack(int const playerNum, int const playerBits, int const playerShrunk, int const sectorLotag, int const floorZ)
{
    auto const pPlayer = g_player[playerNum].ps;

    pPlayer->on_ground       = 0;
    pPlayer->jumping_counter = 0;
    pPlayer->hard_landing    = 0;
    pPlayer->falling_counter = 0;
    pPlayer->pycount        += 32;
    pPlayer->pycount        &= 2047;
    pPlayer->pyoff           = sintable[pPlayer->pycount] >> 7;

    if (pPlayer->jetpack_on < 11)
    {
        pPlayer->jetpack_on++;
        pPlayer->pos.z -= (pPlayer->jetpack_on<<7); //Goin up
    }
    else if (pPlayer->jetpack_on == 11 && !A_CheckSoundPlaying(pPlayer->i, DUKE_JETPACK_IDLE))
        A_PlaySound(DUKE_JETPACK_IDLE, pPlayer->i);

    int const zAdjust = playerShrunk ? 512 : 2048;

    if (TEST_SYNC_KEY(playerBits, SK_JUMP))  // jumping, flying up
    {
        if (VM_OnEvent(EVENT_SOARUP, pPlayer->i, playerNum) == 0)
        {
            pPlayer->pos.z -= zAdjust;
            pPlayer->crack_time = PCRACKTIME;
        }
    }

    if (TEST_SYNC_KEY(playerBits, SK_CROUCH))  // crouching, flying down
    {
        if (VM_OnEvent(EVENT_SOARDOWN, pPlayer->i, playerNum) == 0)
        {
            pPlayer->pos.z += zAdjust;
            pPlayer->crack_time = PCRACKTIME;
        }
    }

    int const Zdiff = (playerShrunk == 0 && (sectorLotag == 0 || sectorLotag == ST_2_UNDERWATER)) ? 32 : 16;

    if (sectorLotag != ST_2_UNDERWATER && pPlayer->scuba_on == 1)
        pPlayer->scuba_on = 0;

    if (pPlayer->pos.z > (floorZ - (Zdiff << 8)))
        pPlayer->pos.z += ((floorZ - (Zdiff << 8)) - pPlayer->pos.z) >> 1;

    if (pPlayer->pos.z < (actor[pPlayer->i].ceilingz + (18 << 8)))
        pPlayer->pos.z = actor[pPlayer->i].ceilingz + (18 << 8);
}

static void P_Dead(int const playerNum, int const sectorLotag, int const floorZ, int const ceilZ)
{
    auto const pPlayer = g_player[playerNum].ps;
    auto const pSprite = &sprite[pPlayer->i];

    if (ud.recstat == 1 && (!g_netServer && ud.multimode < 2))
        G_CloseDemoWrite();

    if ((numplayers < 2 || g_netServer) && pPlayer->dead_flag == 0)
        P_FragPlayer(playerNum);

    if (sectorLotag == ST_2_UNDERWATER)
    {
        if (pPlayer->on_warping_sector == 0)
        {
            if (klabs(pPlayer->pos.z-floorZ) >(PHEIGHT>>1))
                pPlayer->pos.z += 348;
        }
        else
        {
            pSprite->z -= 512;
            pSprite->zvel = -348;
        }

        clipmove(&pPlayer->pos, &pPlayer->cursectnum,
            0, 0, pPlayer->clipdist, (4L<<8), (4L<<8), CLIPMASK0);
        //                        p->bobcounter += 32;
    }

    Bmemcpy(&pPlayer->opos, &pPlayer->pos, sizeof(vec3_t));
    pPlayer->oq16ang = pPlayer->q16ang;
    pPlayer->opyoff = pPlayer->pyoff;

    pPlayer->q16horiz = F16(100);
    pPlayer->q16horizoff = 0;

    updatesector(pPlayer->pos.x, pPlayer->pos.y, &pPlayer->cursectnum);

    pushmove(&pPlayer->pos, &pPlayer->cursectnum, 128L, (4L<<8), (20L<<8), CLIPMASK0);

    if (floorZ > ceilZ + ZOFFSET2 && pSprite->pal != 1)
        pPlayer->rotscrnang = (pPlayer->dead_flag + ((floorZ+pPlayer->pos.z)>>7))&2047;

    pPlayer->on_warping_sector = 0;
}


static void P_HandlePal(DukePlayer_t *const pPlayer)
{
#if !defined LUNATIC
    pPlayer->pals.f--;
#else
    if (pPlayer->palsfadespeed > 0)
    {
        // <palsfadespeed> is the tint fade speed is in
        // decrements/P_ProcessInput() calls.
        pPlayer->pals.f = max(pPlayer->pals.f - pPlayer->palsfadespeed, 0);
    }
    else
    {
        // <palsfadespeed> is a negated count of how many times we
        // (P_ProcessInput()) should be called before decrementing the tint
        // fading by one. <palsfadenext> is the live counter.
        if (pPlayer->palsfadenext < 0)
            pPlayer->palsfadenext++;

        if (pPlayer->palsfadenext == 0)
        {
            pPlayer->palsfadenext = pPlayer->palsfadespeed;
            pPlayer->pals.f--;
        }
    }
#endif
}


static void P_ClampZ(DukePlayer_t* const pPlayer, int const sectorLotag, int32_t const ceilZ, int32_t const floorZ)
{
    if ((sectorLotag != ST_2_UNDERWATER || ceilZ != pPlayer->truecz) && pPlayer->pos.z < ceilZ + PMINHEIGHT)
        pPlayer->pos.z = ceilZ + PMINHEIGHT;

    if (sectorLotag != ST_1_ABOVE_WATER && pPlayer->pos.z > floorZ - PMINHEIGHT)
        pPlayer->pos.z = floorZ - PMINHEIGHT;
}

#define GETZRANGECLIPDISTOFFSET 8

void P_ProcessInput(int playerNum)
{
    if (g_player[playerNum].playerquitflag == 0)
        return;

    auto const pPlayer = g_player[playerNum].ps;
    auto const pSprite = &sprite[pPlayer->i];

    ++pPlayer->player_par;

    VM_OnEvent(EVENT_PROCESSINPUT, pPlayer->i, playerNum);

    uint32_t playerBits = g_player[playerNum].input->bits;

    if (pPlayer->cheat_phase > 0)
        playerBits = 0;

    if (pPlayer->cursectnum == -1)
    {
        if (pSprite->extra > 0 && ud.noclip == 0)
        {
            OSD_Printf(OSD_ERROR "%s: player killed by cursectnum == -1!\n", EDUKE32_FUNCTION);
            P_QuickKill(pPlayer);
            A_PlaySound(SQUISHED, pPlayer->i);
        }

        pPlayer->cursectnum = 0;
    }

    int sectorLotag = sector[pPlayer->cursectnum].lotag;
    // sectorLotag can be set to 0 later on, but the same block sets spritebridge to 1
    int stepHeight = (sectorLotag == ST_1_ABOVE_WATER && pPlayer->spritebridge != 1) ? pPlayer->autostep_sbw : pPlayer->autostep;

    int32_t floorZ, ceilZ, highZhit, lowZhit, dummy;

    if (sectorLotag != ST_2_UNDERWATER)
    {
        if (pPlayer->pos.z + stepHeight > actor[pPlayer->i].floorz - PMINHEIGHT)
            stepHeight -= (pPlayer->pos.z + stepHeight) - (actor[pPlayer->i].floorz - PMINHEIGHT);
        else if (!pPlayer->on_ground)
            stepHeight -= (pPlayer->jumping_counter << 1) + (pPlayer->jumping_counter >> 1);

        stepHeight = max(stepHeight, 0);
    }
    else stepHeight = 0;

    pPlayer->pos.z += stepHeight;
    getzrange(&pPlayer->pos, pPlayer->cursectnum, &ceilZ, &highZhit, &floorZ, &lowZhit, pPlayer->clipdist - GETZRANGECLIPDISTOFFSET, CLIPMASK0);
    pPlayer->pos.z -= stepHeight;

    int32_t ceilZ2 = ceilZ;
    getzrange(&pPlayer->pos, pPlayer->cursectnum, &ceilZ, &highZhit, &dummy, &dummy, pPlayer->clipdist - GETZRANGECLIPDISTOFFSET, CSTAT_SPRITE_ALIGNMENT_FLOOR << 16);

    if ((highZhit & 49152) == 49152 && (sprite[highZhit & (MAXSPRITES - 1)].cstat & CSTAT_SPRITE_BLOCK) != CSTAT_SPRITE_BLOCK)
        ceilZ = ceilZ2;

    pPlayer->spritebridge = 0;
    pPlayer->sbs          = 0;

#ifdef YAX_ENABLE
    getzsofslope_player(pPlayer->cursectnum, pPlayer->pos.x, pPlayer->pos.y, &pPlayer->truecz, &pPlayer->truefz);
#else
    getzsofslope(pPlayer->cursectnum, pPlayer->pos.x, pPlayer->pos.y, &pPlayer->truecz, &pPlayer->truefz);
#endif
    int const trueFloorZ    = pPlayer->truefz;
    int const trueFloorDist = klabs(pPlayer->pos.z - trueFloorZ);

    if ((lowZhit & 49152) == 16384 && sectorLotag == 1 && trueFloorDist > PHEIGHT + ZOFFSET2)
        sectorLotag = 0;

    actor[pPlayer->i].floorz   = floorZ;
    actor[pPlayer->i].ceilingz = ceilZ;

    pPlayer->oq16horiz    = pPlayer->q16horiz;
    pPlayer->oq16horizoff = pPlayer->q16horizoff;

    // calculates automatic view angle for playing without a mouse
    if (pPlayer->aim_mode == 0 && pPlayer->on_ground && sectorLotag != ST_2_UNDERWATER
        && (sector[pPlayer->cursectnum].floorstat & 2))
    {
        vec2_t const adjustedPlayer = { pPlayer->pos.x + (sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047] >> 5),
                                        pPlayer->pos.y + (sintable[fix16_to_int(pPlayer->q16ang) & 2047] >> 5) };
        int16_t curSectNum = pPlayer->cursectnum;

        updatesector(adjustedPlayer.x, adjustedPlayer.y, &curSectNum);

        if (curSectNum >= 0)
        {
            int const slopeZ = getflorzofslope(pPlayer->cursectnum, adjustedPlayer.x, adjustedPlayer.y);
            if ((pPlayer->cursectnum == curSectNum) ||
                (klabs(getflorzofslope(curSectNum, adjustedPlayer.x, adjustedPlayer.y) - slopeZ) <= ZOFFSET6))
                pPlayer->q16horizoff += fix16_from_int(mulscale16(trueFloorZ - slopeZ, 160));
        }
    }

    if (pPlayer->q16horizoff > 0)
    {
        pPlayer->q16horizoff -= ((pPlayer->q16horizoff >> 3) + fix16_one);
        pPlayer->q16horizoff = max(pPlayer->q16horizoff, 0);
    }
    else if (pPlayer->q16horizoff < 0)
    {
        pPlayer->q16horizoff += (((-pPlayer->q16horizoff) >> 3) + fix16_one);
        pPlayer->q16horizoff = min(pPlayer->q16horizoff, 0);
    }

    if ((highZhit & 49152) == 49152)
    {
        int const spriteNum = highZhit & (MAXSPRITES-1);

        if (sprite[spriteNum].statnum == STAT_ACTOR && sprite[spriteNum].extra >= 0)
        {
            highZhit = 0;
            ceilZ    = pPlayer->truecz;
        }
    }

    if ((lowZhit & 49152) == 49152)
    {
        int spriteNum = lowZhit&(MAXSPRITES-1);

        if ((sprite[spriteNum].cstat&33) == 33 || (sprite[spriteNum].cstat&17) == 17 ||
                clipshape_idx_for_sprite((uspriteptr_t)&sprite[spriteNum], -1) >= 0)
        {
            // EDuke32 extension: xvel of 1 makes a sprite be never regarded as a bridge.

            if (sectorLotag != ST_2_UNDERWATER && (sprite[spriteNum].xvel & 1) == 0)
            {
                sectorLotag             = 0;
                pPlayer->footprintcount = 0;
                pPlayer->spritebridge   = 1;
                pPlayer->sbs            = spriteNum;
            }
        }
        else if (A_CheckEnemySprite(&sprite[spriteNum]) && sprite[spriteNum].xrepeat > 24
                 && klabs(pSprite->z - sprite[spriteNum].z) < (84 << 8))
        {
            // TX: I think this is what makes the player slide off enemies... might
            // be a good sprite flag to add later.
            // Helix: there's also SLIDE_ABOVE_ENEMY.
            int spriteAng = getangle(sprite[spriteNum].x - pPlayer->pos.x,
                                       sprite[spriteNum].y - pPlayer->pos.y);
            pPlayer->vel.x -= sintable[(spriteAng + 512) & 2047] << 4;
            pPlayer->vel.y -= sintable[spriteAng & 2047] << 4;
        }
    }

    if (pSprite->extra > 0)
        P_IncurDamage(pPlayer);
    else
    {
        pSprite->extra                  = 0;
        pPlayer->inv_amount[GET_SHIELD] = 0;
    }

    pPlayer->last_extra = pSprite->extra;
    pPlayer->loogcnt    = (pPlayer->loogcnt > 0) ? pPlayer->loogcnt - 1 : 0;

    if (pPlayer->fist_incs && P_DoFist(pPlayer)) return;

    if (pPlayer->timebeforeexit > 1 && pPlayer->last_extra > 0)
    {
        if (--pPlayer->timebeforeexit == GAMETICSPERSEC*5)
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();

            if (pPlayer->customexitsound >= 0)
            {
                S_PlaySound(pPlayer->customexitsound);
                P_DoQuote(QUOTE_WEREGONNAFRYYOURASS,pPlayer);
            }
        }
        else if (pPlayer->timebeforeexit == 1)
        {
            P_EndLevel();
            return;
        }
    }

    if (pPlayer->pals.f > 0)
        P_HandlePal(pPlayer);

    if (pPlayer->fta > 0 && --pPlayer->fta == 0)
    {
        pub = pus = NUMPAGES;
        pPlayer->ftq = 0;
    }

    if (g_levelTextTime > 0)
        g_levelTextTime--;

    if (pSprite->extra <= 0)
    {
        P_Dead(playerNum, sectorLotag, floorZ, ceilZ);
        return;
    }

    if (pPlayer->transporter_hold > 0)
    {
        pPlayer->transporter_hold--;
        if (pPlayer->transporter_hold == 0 && pPlayer->on_warping_sector)
            pPlayer->transporter_hold = 2;
    }
    else if (pPlayer->transporter_hold < 0)
        pPlayer->transporter_hold++;

    if (pPlayer->newowner >= 0)
    {
        P_UpdatePosWhenViewingCam(pPlayer);
        P_DoCounters(playerNum);

        if (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == HANDREMOTE_WEAPON)
            P_ProcessWeapon(playerNum);

        return;
    }

    pPlayer->rotscrnang -= (pPlayer->rotscrnang >> 1);

    if (pPlayer->rotscrnang && !(pPlayer->rotscrnang >> 1))
        pPlayer->rotscrnang -= ksgn(pPlayer->rotscrnang);

    pPlayer->look_ang -= (pPlayer->look_ang >> 2);

    if (pPlayer->look_ang && !(pPlayer->look_ang >> 2))
        pPlayer->look_ang -= ksgn(pPlayer->look_ang);

    if (TEST_SYNC_KEY(playerBits, SK_LOOK_LEFT))
    {
        // look_left
        if (VM_OnEvent(EVENT_LOOKLEFT,pPlayer->i,playerNum) == 0)
        {
            pPlayer->look_ang -= 152;
            pPlayer->rotscrnang += 24;
        }
    }

    if (TEST_SYNC_KEY(playerBits, SK_LOOK_RIGHT))
    {
        // look_right
        if (VM_OnEvent(EVENT_LOOKRIGHT,pPlayer->i,playerNum) == 0)
        {
            pPlayer->look_ang += 152;
            pPlayer->rotscrnang -= 24;
        }
    }

    int                  velocityModifier = TICSPERFRAME;
    const uint8_t *const weaponFrame      = &pPlayer->kickback_pic;
    int                  floorZOffset     = 40;
    int const            playerShrunk     = (pSprite->yrepeat < 32);

    if (pPlayer->on_crane >= 0)
        goto HORIZONLY;

    pPlayer->weapon_sway = (pSprite->xvel < 32 || pPlayer->on_ground == 0 || pPlayer->bobcounter == 1024)
                           ? (((pPlayer->weapon_sway & 2047) > (1024 + 96))
                           ? (pPlayer->weapon_sway - 96)
                           : (((pPlayer->weapon_sway & 2047) < (1024 - 96)))
                           ? (pPlayer->weapon_sway + 96)
                           : 1024)
                           : pPlayer->bobcounter;

    // NOTE: This silently wraps if the difference is too great, e.g. used to do
    // that when teleported by silent SE7s.
    pSprite->xvel = ksqrt(uhypsq(pPlayer->pos.x - pPlayer->bobpos.x, pPlayer->pos.y - pPlayer->bobpos.y));

    if (pPlayer->on_ground)
        pPlayer->bobcounter += sprite[pPlayer->i].xvel>>1;

    if (ud.noclip == 0 && ((uint16_t)pPlayer->cursectnum >= MAXSECTORS || sector[pPlayer->cursectnum].floorpicnum == MIRROR))
    {
        pPlayer->pos.x = pPlayer->opos.x;
        pPlayer->pos.y = pPlayer->opos.y;
    }
    else
    {
        pPlayer->opos.x = pPlayer->pos.x;
        pPlayer->opos.y = pPlayer->pos.y;
    }

    pPlayer->bobpos.x = pPlayer->pos.x;
    pPlayer->bobpos.y = pPlayer->pos.y;
    pPlayer->opos.z   = pPlayer->pos.z;
    pPlayer->opyoff   = pPlayer->pyoff;
    pPlayer->oq16ang    = pPlayer->q16ang;

    if (pPlayer->one_eighty_count < 0)
    {
        pPlayer->one_eighty_count += 128;
        pPlayer->q16ang += F16(128);
    }

    // Shrinking code

    if (sectorLotag == ST_2_UNDERWATER)
        P_DoWater(playerNum, playerBits, floorZ, ceilZ);
    else if (pPlayer->jetpack_on)
        P_DoJetpack(playerNum, playerBits, playerShrunk, sectorLotag, floorZ);
    else
    {
        pPlayer->airleft  = 15 * GAMETICSPERSEC;  // 13 seconds
        pPlayer->scuba_on = 0;

        if (sectorLotag == ST_1_ABOVE_WATER && pPlayer->spritebridge == 0)
        {
            floorZOffset = 12;

            if (playerShrunk == 0)
            {
                floorZOffset      = 34;
                pPlayer->pycount += 32;
                pPlayer->pycount &= 2047;
                pPlayer->pyoff    = sintable[pPlayer->pycount] >> 6;
            }

            if (playerShrunk == 0 && trueFloorDist <= PHEIGHT)
            {
                if (pPlayer->on_ground == 1)
                {
                    if (pPlayer->dummyplayersprite < 0)
                        pPlayer->dummyplayersprite = A_Spawn(pPlayer->i,PLAYERONWATER);

                    sprite[pPlayer->dummyplayersprite].cstat |= 32768;
                    sprite[pPlayer->dummyplayersprite].pal = sprite[pPlayer->i].pal;
                    pPlayer->footprintpal                  = (sector[pPlayer->cursectnum].floorpicnum == FLOORSLIME) ? 8 : 0;
                    pPlayer->footprintshade                = 0;
                }
            }
        }
        else if (pPlayer->footprintcount > 0 && pPlayer->on_ground)
        {
            if (pPlayer->cursectnum >= 0 && (sector[pPlayer->cursectnum].floorstat & 2) != 2)
            {
                int spriteNum = -1;

                for (spriteNum = headspritesect[pPlayer->cursectnum]; spriteNum >= 0; spriteNum = nextspritesect[spriteNum])
                {
                    if (sprite[spriteNum].picnum == FOOTPRINTS || sprite[spriteNum].picnum == FOOTPRINTS2 ||
                        sprite[spriteNum].picnum == FOOTPRINTS3 || sprite[spriteNum].picnum == FOOTPRINTS4)
                    {
                        if (klabs(sprite[spriteNum].x - pPlayer->pos.x) < 384 &&
                            klabs(sprite[spriteNum].y - pPlayer->pos.y) < 384)
                            break;
                    }
                }

                if (spriteNum < 0)
                {
                    if (sector[pPlayer->cursectnum].lotag == 0 &&
                        sector[pPlayer->cursectnum].hitag == 0)
#ifdef YAX_ENABLE
                        if (yax_getbunch(pPlayer->cursectnum, YAX_FLOOR) < 0 || (sector[pPlayer->cursectnum].floorstat & 512))
#endif
                        {
                            switch (krand() & 3)
                            {
                                case 0: spriteNum  = A_Spawn(pPlayer->i, FOOTPRINTS); break;
                                case 1: spriteNum  = A_Spawn(pPlayer->i, FOOTPRINTS2); break;
                                case 2: spriteNum  = A_Spawn(pPlayer->i, FOOTPRINTS3); break;
                                default: spriteNum = A_Spawn(pPlayer->i, FOOTPRINTS4); break;
                            }
                            sprite[spriteNum].pal   = pPlayer->footprintpal;
                            sprite[spriteNum].shade = pPlayer->footprintshade;
                            pPlayer->footprintcount--;
                        }
                }
            }
        }

        if (pPlayer->pos.z < (floorZ-(floorZOffset<<8)))  //falling
        {
            // not jumping or crouching
            if ((!TEST_SYNC_KEY(playerBits, SK_JUMP) && !(TEST_SYNC_KEY(playerBits, SK_CROUCH))) && pPlayer->on_ground &&
                (sector[pPlayer->cursectnum].floorstat & 2) && pPlayer->pos.z >= (floorZ - (floorZOffset << 8) - ZOFFSET2))
                pPlayer->pos.z = floorZ - (floorZOffset << 8);
            else
            {
                pPlayer->on_ground = 0;
                pPlayer->vel.z    += (g_spriteGravity + 80);  // (TICSPERFRAME<<6);

                if (pPlayer->vel.z >= (4096 + 2048))
                    pPlayer->vel.z = (4096 + 2048);

                if (pPlayer->vel.z > 2400 && pPlayer->falling_counter < 255)
                {
                    pPlayer->falling_counter++;
                    if (pPlayer->falling_counter >= 38 && pPlayer->scream_voice <= FX_Ok)
                    {
                        int32_t voice = A_PlaySound(DUKE_SCREAM,pPlayer->i);
                        if (voice <= 127)  // XXX: p->scream_voice is an int8_t
                            pPlayer->scream_voice = voice;
                    }
                }

                if ((pPlayer->pos.z + pPlayer->vel.z) >= (floorZ - (floorZOffset << 8)) && pPlayer->cursectnum >= 0)  // hit the ground
                {
                    if (sector[pPlayer->cursectnum].lotag != ST_1_ABOVE_WATER)
                    {
                        if (pPlayer->falling_counter > 62)
                            P_QuickKill(pPlayer);
                        else if (pPlayer->falling_counter > 9)
                        {
                            // Falling damage.
                            pSprite->extra -= pPlayer->falling_counter - (krand() & 3);

#ifndef EDUKE32_STANDALONE
                            if (!FURY)
                            {
                                if (pSprite->extra <= 0)
                                    A_PlaySound(SQUISHED, pPlayer->i);
                                else
                                {
                                    A_PlaySound(DUKE_LAND, pPlayer->i);
                                    A_PlaySound(DUKE_LAND_HURT, pPlayer->i);
                                }
                            }
#endif
                            P_PalFrom(pPlayer, 32, 16, 0, 0);
                        }
#ifndef EDUKE32_STANDALONE
                        else if (!FURY && pPlayer->vel.z > 2048)
                            A_PlaySound(DUKE_LAND, pPlayer->i);
#endif
                    }
                }
            }
        }
        else
        {
            pPlayer->falling_counter = 0;

            if (pPlayer->scream_voice > FX_Ok)
            {
                FX_StopSound(pPlayer->scream_voice);
                S_Cleanup();
                pPlayer->scream_voice = -1;
            }

            if ((sectorLotag != ST_1_ABOVE_WATER && sectorLotag != ST_2_UNDERWATER) &&
                (pPlayer->on_ground == 0 && pPlayer->vel.z > (6144 >> 1)))
                pPlayer->hard_landing = pPlayer->vel.z>>10;

            pPlayer->on_ground = 1;

            if (floorZOffset==40)
            {
                //Smooth on the ground
                int Zdiff = ((floorZ - (floorZOffset << 8)) - pPlayer->pos.z) >> 1;

                if (klabs(Zdiff) < 256)
                    Zdiff = 0;
                else  pPlayer->pos.z += (floorZ - (floorZOffset << 8) - pPlayer->pos.z) >> 1;
                pPlayer->vel.z -= 768;

                if (pPlayer->vel.z < 0)
                    pPlayer->vel.z = 0;
            }
            else if (pPlayer->jumping_counter == 0)
            {
                pPlayer->pos.z += ((floorZ - (floorZOffset << 7)) - pPlayer->pos.z) >> 1;  // Smooth on the water

                if (pPlayer->on_warping_sector == 0 && pPlayer->pos.z > floorZ - PCROUCHHEIGHT)
                {
                    pPlayer->pos.z = floorZ - PCROUCHHEIGHT;
                    pPlayer->vel.z >>= 1;
                }
            }

            if (TEST_SYNC_KEY(playerBits, SK_CROUCH))
            {
                // crouching
                if (VM_OnEvent(EVENT_CROUCH,pPlayer->i,playerNum) == 0)
                {
                    if (pPlayer->jumping_toggle == 0)
                    {
                        pPlayer->pos.z += PCROUCHINCREMENT;
                        pPlayer->crack_time = PCRACKTIME;
                    }
                }
            }

            // jumping
            if (!TEST_SYNC_KEY(playerBits, SK_JUMP) && pPlayer->jumping_toggle)
                pPlayer->jumping_toggle--;
            else if (TEST_SYNC_KEY(playerBits, SK_JUMP) && pPlayer->jumping_toggle == 0)
            {
                int32_t floorZ2, ceilZ2;
                getzrange(&pPlayer->pos, pPlayer->cursectnum, &ceilZ2, &dummy, &floorZ2, &dummy, pPlayer->clipdist - GETZRANGECLIPDISTOFFSET, CLIPMASK0);

                if ((floorZ2-ceilZ2) > (48<<8))
                {
                    if (VM_OnEvent(EVENT_JUMP,pPlayer->i,playerNum) == 0)
                    {
                        pPlayer->jumping_toggle = 1;

                        if (!TEST_SYNC_KEY(playerBits, SK_CROUCH))
                            pPlayer->jumping_counter = 1;
                        else
                        {
                            pPlayer->jumping_toggle = 2;

                            if (myconnectindex == playerNum)
                                CONTROL_ClearButton(gamefunc_Jump);
                        }
                    }
                }
            }
        }

        if (pPlayer->jumping_counter)
        {
            if (!TEST_SYNC_KEY(playerBits, SK_JUMP) && pPlayer->jumping_toggle)
                pPlayer->jumping_toggle--;

            if (pPlayer->jumping_counter < (1024+256))
            {
                if (sectorLotag == ST_1_ABOVE_WATER && pPlayer->jumping_counter > 768)
                {
                    pPlayer->jumping_counter = 0;
                    pPlayer->vel.z = -512;
                }
                else
                {
                    pPlayer->vel.z -= (sintable[(2048-128+pPlayer->jumping_counter)&2047])/12;
                    pPlayer->jumping_counter += 180;
                    pPlayer->on_ground = 0;
                }
            }
            else
            {
                pPlayer->jumping_counter = 0;
                pPlayer->vel.z = 0;
            }
        }

        if ((sectorLotag != ST_2_UNDERWATER || ceilZ != pPlayer->truecz) && pPlayer->jumping_counter && pPlayer->pos.z <= (ceilZ + PMINHEIGHT + 128))
        {
            pPlayer->jumping_counter = 0;
            if (pPlayer->vel.z < 0)
                pPlayer->vel.x = pPlayer->vel.y = 0;
            pPlayer->vel.z = 128;
        }
    }

    if (pPlayer->fist_incs || pPlayer->transporter_hold > 2 || pPlayer->hard_landing || pPlayer->access_incs > 0 ||
        pPlayer->knee_incs > 0 || (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == TRIPBOMB_WEAPON &&
                                   *weaponFrame > 1 && *weaponFrame < PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay)))
    {
        velocityModifier = 0;
        pPlayer->vel.x   = 0;
        pPlayer->vel.y   = 0;
    }
    else if (g_player[playerNum].input->q16avel)            //p->ang += syncangvel * constant
    {
        fix16_t const inputAng  = g_player[playerNum].input->q16avel;

        pPlayer->q16angvel     = (sectorLotag == ST_2_UNDERWATER) ? fix16_mul(inputAng - (inputAng >> 3), fix16_from_int(ksgn(velocityModifier)))
                                                               : fix16_mul(inputAng, fix16_from_int(ksgn(velocityModifier)));
        pPlayer->q16ang       += pPlayer->q16angvel;
        pPlayer->q16ang       &= 0x7FFFFFF;
        pPlayer->crack_time = PCRACKTIME;
    }

    if (pPlayer->spritebridge == 0)
    {
        int const floorPicnum = sector[pSprite->sectnum].floorpicnum;

#ifndef EDUKE32_STANDALONE
        if (!FURY && (floorPicnum == PURPLELAVA || sector[pSprite->sectnum].ceilingpicnum == PURPLELAVA))
        {
            if (pPlayer->inv_amount[GET_BOOTS] > 0)
            {
                pPlayer->inv_amount[GET_BOOTS]--;
                pPlayer->inven_icon = ICON_BOOTS;
                if (pPlayer->inv_amount[GET_BOOTS] <= 0)
                    P_SelectNextInvItem(pPlayer);
            }
            else
            {
                if (!A_CheckSoundPlaying(pPlayer->i,DUKE_LONGTERM_PAIN))
                    A_PlaySound(DUKE_LONGTERM_PAIN,pPlayer->i);
                P_PalFrom(pPlayer, 32, 0, 8, 0);
                pSprite->extra--;
            }
        }
#endif
        if (pPlayer->on_ground && trueFloorDist <= PHEIGHT+ZOFFSET2 && P_CheckFloorDamage(pPlayer, floorPicnum))
        {
            P_DoQuote(QUOTE_BOOTS_ON, pPlayer);
            pPlayer->inv_amount[GET_BOOTS] -= 2;
            if (pPlayer->inv_amount[GET_BOOTS] <= 0)
            {
                pPlayer->inv_amount[GET_BOOTS] = 0;
                P_SelectNextInvItem(pPlayer);
            }
        }
    }

    if (g_player[playerNum].input->extbits & (1))      VM_OnEvent(EVENT_MOVEFORWARD,  pPlayer->i, playerNum);
    if (g_player[playerNum].input->extbits & (1 << 1)) VM_OnEvent(EVENT_MOVEBACKWARD, pPlayer->i, playerNum);
    if (g_player[playerNum].input->extbits & (1 << 2)) VM_OnEvent(EVENT_STRAFELEFT,   pPlayer->i, playerNum);
    if (g_player[playerNum].input->extbits & (1 << 3)) VM_OnEvent(EVENT_STRAFERIGHT,  pPlayer->i, playerNum);

    if (g_player[playerNum].input->extbits & (1 << 4) || g_player[playerNum].input->q16avel < 0)
        VM_OnEvent(EVENT_TURNLEFT, pPlayer->i, playerNum);

    if (g_player[playerNum].input->extbits & (1 << 5) || g_player[playerNum].input->q16avel > 0)
        VM_OnEvent(EVENT_TURNRIGHT, pPlayer->i, playerNum);

    if (pPlayer->vel.x || pPlayer->vel.y || g_player[playerNum].input->fvel || g_player[playerNum].input->svel)
    {
        pPlayer->crack_time = PCRACKTIME;

#ifndef EDUKE32_STANDALONE
        if (!FURY)
        {
            int const checkWalkSound = sintable[pPlayer->bobcounter & 2047] >> 12;

            if (trueFloorDist < PHEIGHT + ZOFFSET3)
            {
                if (checkWalkSound == 1 || checkWalkSound == 3)
                {
                    if (pPlayer->walking_snd_toggle == 0 && pPlayer->on_ground)
                    {
                        switch (sectorLotag)
                        {
                            case 0:
                            {
                                int const walkPicnum = (lowZhit >= 0 && (lowZhit & 49152) == 49152)
                                                       ? TrackerCast(sprite[lowZhit & (MAXSPRITES - 1)].picnum)
                                                       : TrackerCast(sector[pPlayer->cursectnum].floorpicnum);

                                switch (DYNAMICTILEMAP(walkPicnum))
                                {
                                    case PANNEL1__STATIC:
                                    case PANNEL2__STATIC:
                                        A_PlaySound(DUKE_WALKINDUCTS, pPlayer->i);
                                        pPlayer->walking_snd_toggle = 1;
                                        break;
                                }
                            }
                            break;

                            case ST_1_ABOVE_WATER:
                                if (!pPlayer->spritebridge)
                                {
                                    if ((krand() & 1) == 0)
                                        A_PlaySound(DUKE_ONWATER, pPlayer->i);
                                    pPlayer->walking_snd_toggle = 1;
                                }
                                break;
                        }
                    }
                }
                else if (pPlayer->walking_snd_toggle > 0)
                    pPlayer->walking_snd_toggle--;
            }

            if (pPlayer->jetpack_on == 0 && pPlayer->inv_amount[GET_STEROIDS] > 0 && pPlayer->inv_amount[GET_STEROIDS] < 400)
                velocityModifier <<= 1;
        }
#endif

        pPlayer->vel.x += (((g_player[playerNum].input->fvel) * velocityModifier) << 6);
        pPlayer->vel.y += (((g_player[playerNum].input->svel) * velocityModifier) << 6);

        int playerSpeedReduction = 0;

        if (sectorLotag == ST_2_UNDERWATER)
            playerSpeedReduction = 0x1400;
        else if (((pPlayer->on_ground && TEST_SYNC_KEY(playerBits, SK_CROUCH))
                  || (*weaponFrame > 10 && PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == KNEE_WEAPON)))
            playerSpeedReduction = 0x2000;

        pPlayer->vel.x = mulscale16(pPlayer->vel.x, pPlayer->runspeed - playerSpeedReduction);
        pPlayer->vel.y = mulscale16(pPlayer->vel.y, pPlayer->runspeed - playerSpeedReduction);

        if (klabs(pPlayer->vel.x) < 2048 && klabs(pPlayer->vel.y) < 2048)
            pPlayer->vel.x = pPlayer->vel.y = 0;

#ifndef EDUKE32_STANDALONE
        if (!FURY && playerShrunk)
        {
            pPlayer->vel.x = mulscale16(pPlayer->vel.x, pPlayer->runspeed - (pPlayer->runspeed >> 1) + (pPlayer->runspeed >> 2));
            pPlayer->vel.y = mulscale16(pPlayer->vel.y, pPlayer->runspeed - (pPlayer->runspeed >> 1) + (pPlayer->runspeed >> 2));
        }
#endif
    }

    // This makes the player view lower when shrunk. This needs to happen before clipmove().
    if (!FURY && pPlayer->jetpack_on == 0 && sectorLotag != ST_2_UNDERWATER && sectorLotag != ST_1_ABOVE_WATER && playerShrunk)
        pPlayer->pos.z += ZOFFSET5 - (sprite[pPlayer->i].yrepeat<<8);

HORIZONLY:;
    if (ud.noclip)
    {
        pPlayer->pos.x += pPlayer->vel.x >> 14;
        pPlayer->pos.y += pPlayer->vel.y >> 14;
        updatesector(pPlayer->pos.x, pPlayer->pos.y, &pPlayer->cursectnum);
        changespritesect(pPlayer->i, pPlayer->cursectnum);
    }
    else
    {
#ifdef YAX_ENABLE
        int const playerSectNum = pPlayer->cursectnum;
        int16_t   ceilingBunch, floorBunch;

        if (playerSectNum >= 0)
            yax_getbunches(playerSectNum, &ceilingBunch, &floorBunch);

        // This updatesectorz conflicts with Duke3D's way of teleporting through water,
        // so make it a bit conditional... OTOH, this way we have an ugly z jump when
        // changing from above water to underwater

        if ((playerSectNum >= 0 && !(sector[playerSectNum].lotag == ST_1_ABOVE_WATER && pPlayer->on_ground && floorBunch >= 0))
            && ((floorBunch >= 0 && !(sector[playerSectNum].floorstat & 512))
                || (ceilingBunch >= 0 && !(sector[playerSectNum].ceilingstat & 512))))
        {
            pPlayer->cursectnum += MAXSECTORS;  // skip initial z check, restored by updatesectorz
            updatesectorz(pPlayer->pos.x, pPlayer->pos.y, pPlayer->pos.z, &pPlayer->cursectnum);
        }
#endif

        P_ClampZ(pPlayer, sectorLotag, ceilZ, floorZ);

        int const touchObject = FURY ? clipmove(&pPlayer->pos, &pPlayer->cursectnum, pPlayer->vel.x + (pPlayer->fric.x << 9),
                                                   pPlayer->vel.y + (pPlayer->fric.y << 9), pPlayer->clipdist, (4L << 8), stepHeight, CLIPMASK0)
                                        : clipmove(&pPlayer->pos, &pPlayer->cursectnum, pPlayer->vel.x, pPlayer->vel.y, pPlayer->clipdist,
                                                   (4L << 8), stepHeight, CLIPMASK0);

        if (touchObject)
            P_CheckTouchDamage(pPlayer, touchObject);

        if (FURY)
            pPlayer->fric.x = pPlayer->fric.y = 0;
    }

    if (pPlayer->jetpack_on == 0)
    {
        if (pSprite->xvel > 16)
        {
            if (sectorLotag != ST_1_ABOVE_WATER && sectorLotag != ST_2_UNDERWATER && pPlayer->on_ground)
            {
                pPlayer->pycount += 52;
                pPlayer->pycount &= 2047;
                pPlayer->pyoff = klabs(pSprite->xvel * sintable[pPlayer->pycount]) / 1536;
            }
        }
        else if (sectorLotag != ST_2_UNDERWATER && sectorLotag != ST_1_ABOVE_WATER)
            pPlayer->pyoff = 0;

        if (sectorLotag != ST_2_UNDERWATER)
            pPlayer->pos.z += pPlayer->vel.z;
    }

    P_ClampZ(pPlayer, sectorLotag, ceilZ, floorZ);

    if (pPlayer->cursectnum >= 0)
    {
        pPlayer->pos.z += PHEIGHT;
        sprite[pPlayer->i].pos = pPlayer->pos;
        pPlayer->pos.z -= PHEIGHT;

        changespritesect(pPlayer->i, pPlayer->cursectnum);
    }

    // ST_2_UNDERWATER
    if (pPlayer->cursectnum >= 0 && sectorLotag < 3)
    {
        auto const pSector = (usectorptr_t)&sector[pPlayer->cursectnum];

        // TRAIN_SECTOR_TO_SE_INDEX
        if ((!ud.noclip && pSector->lotag == ST_31_TWO_WAY_TRAIN) &&
            ((unsigned)pSector->hitag < MAXSPRITES && sprite[pSector->hitag].xvel && actor[pSector->hitag].t_data[0] == 0))
        {
            P_QuickKill(pPlayer);
            return;
        }
    }

#ifndef EDUKE32_STANDALONE
    if (!FURY && (pPlayer->cursectnum >= 0 && trueFloorDist < PHEIGHT && pPlayer->on_ground && sectorLotag != ST_1_ABOVE_WATER &&
         playerShrunk == 0 && sector[pPlayer->cursectnum].lotag == ST_1_ABOVE_WATER) && (!A_CheckSoundPlaying(pPlayer->i, DUKE_ONWATER)))
            A_PlaySound(DUKE_ONWATER, pPlayer->i);
#endif

    pPlayer->on_warping_sector = 0;

    if (pPlayer->cursectnum >= 0 && ud.noclip == 0)
    {
        int const  pushResult    = pushmove(&pPlayer->pos, &pPlayer->cursectnum, pPlayer->clipdist - 1, (4L << 8), (4L << 8), CLIPMASK0);
        int const  furthestAngle = A_GetFurthestAngle(pPlayer->i, 32);
        int const  angleDelta    = G_GetAngleDelta(fix16_to_int(pPlayer->q16ang), furthestAngle);
        bool const squishPlayer  = pushResult < 0 && !angleDelta;

        if (squishPlayer || klabs(actor[pPlayer->i].floorz-actor[pPlayer->i].ceilingz) < (48<<8))
        {
            if (!(sector[pSprite->sectnum].lotag & 0x8000u) &&
                (isanunderoperator(sector[pSprite->sectnum].lotag) || isanearoperator(sector[pSprite->sectnum].lotag)))
                G_ActivateBySector(pSprite->sectnum, pPlayer->i);

            if (squishPlayer)
            {
                OSD_Printf(OSD_ERROR "%s: player killed by pushmove()!\n", EDUKE32_FUNCTION);
                P_QuickKill(pPlayer);
                return;
            }
        }
        else if (klabs(floorZ - ceilZ) < ZOFFSET5 && isanunderoperator(sector[pPlayer->cursectnum].lotag))
            G_ActivateBySector(pPlayer->cursectnum, pPlayer->i);
    }

    int centerHoriz = 0;

    if (TEST_SYNC_KEY(playerBits, SK_CENTER_VIEW) || pPlayer->hard_landing)
        if (VM_OnEvent(EVENT_RETURNTOCENTER,pPlayer->i,playerNum) == 0)
            pPlayer->return_to_center = 9;

    // A horiz diff of 128 equal 45 degrees,
    // so we convert horiz to 1024 angle units

    float horizAngle = atan2f(pPlayer->q16horiz - F16(100), F16(128)) * (512.f / fPI) + fix16_to_float(g_player[playerNum].input->q16horz);

    if (TEST_SYNC_KEY(playerBits, SK_LOOK_UP))
    {
        if (VM_OnEvent(EVENT_LOOKUP,pPlayer->i,playerNum) == 0)
        {
            pPlayer->return_to_center = 9;
            horizAngle += float(12<<(int)(TEST_SYNC_KEY(playerBits, SK_RUN)));
            centerHoriz++;
        }
    }

    if (TEST_SYNC_KEY(playerBits, SK_LOOK_DOWN))
    {
        if (VM_OnEvent(EVENT_LOOKDOWN,pPlayer->i,playerNum) == 0)
        {
            pPlayer->return_to_center = 9;
            horizAngle -= float(12<<(int)(TEST_SYNC_KEY(playerBits, SK_RUN)));
            centerHoriz++;
        }
    }

    if (TEST_SYNC_KEY(playerBits, SK_AIM_UP))
    {
        if (VM_OnEvent(EVENT_AIMUP,pPlayer->i,playerNum) == 0)
        {
            horizAngle += float(6<<(int)(TEST_SYNC_KEY(playerBits, SK_RUN)));
            centerHoriz++;
        }
    }

    if (TEST_SYNC_KEY(playerBits, SK_AIM_DOWN))
    {
        if (VM_OnEvent(EVENT_AIMDOWN,pPlayer->i,playerNum) == 0)
        {
            horizAngle -= float(6<<(int)(TEST_SYNC_KEY(playerBits, SK_RUN)));
            centerHoriz++;
        }
    }

    pPlayer->q16horiz = F16(100) + Blrintf(F16(128) * tanf(horizAngle * (fPI / 512.f)));

    if (pPlayer->return_to_center > 0 && !TEST_SYNC_KEY(playerBits, SK_LOOK_UP) && !TEST_SYNC_KEY(playerBits, SK_LOOK_DOWN))
    {
        pPlayer->return_to_center--;
        pPlayer->q16horiz += F16(33)-fix16_div(pPlayer->q16horiz, F16(3));
        centerHoriz++;
    }

    if (pPlayer->hard_landing > 0)
    {
        pPlayer->hard_landing--;
        pPlayer->q16horiz -= fix16_from_int(pPlayer->hard_landing<<4);
    }

    if (centerHoriz)
    {
        if (pPlayer->q16horiz > F16(95) && pPlayer->q16horiz < F16(105)) pPlayer->q16horiz = F16(100);
        if (pPlayer->q16horizoff > F16(-5) && pPlayer->q16horizoff < F16(5)) pPlayer->q16horizoff = 0;
    }

    pPlayer->q16horiz = fix16_clamp(pPlayer->q16horiz, F16(HORIZ_MIN), F16(HORIZ_MAX));

    //Shooting code/changes

    if (pPlayer->show_empty_weapon > 0)
    {
        --pPlayer->show_empty_weapon;

        if (pPlayer->show_empty_weapon == 0 && (pPlayer->weaponswitch & 2) && pPlayer->ammo_amount[pPlayer->curr_weapon] <= 0)
        {
#ifndef EDUKE32_STANDALONE
            if (!FURY)
            {
                if (pPlayer->last_full_weapon == GROW_WEAPON)
                    pPlayer->subweapon |= (1 << GROW_WEAPON);
                else if (pPlayer->last_full_weapon == SHRINKER_WEAPON)
                    pPlayer->subweapon &= ~(1 << GROW_WEAPON);
            }
#endif
            P_AddWeapon(pPlayer, pPlayer->last_full_weapon, 1);
            return;
        }
    }

#ifndef EDUKE32_STANDALONE
    if (!FURY && pPlayer->knee_incs > 0)
    {
        pPlayer->q16horiz -= F16(48);
        pPlayer->return_to_center = 9;

        if (++pPlayer->knee_incs > 15)
        {
            pPlayer->knee_incs      = 0;
            pPlayer->holster_weapon = 0;
            pPlayer->weapon_pos     = klabs(pPlayer->weapon_pos);

            if (pPlayer->actorsqu >= 0 && sprite[pPlayer->actorsqu].statnum != MAXSTATUS &&
                dist(&sprite[pPlayer->i], &sprite[pPlayer->actorsqu]) < 1400)
            {
                A_DoGuts(pPlayer->actorsqu, JIBS6, 7);
                A_Spawn(pPlayer->actorsqu, BLOODPOOL);
                A_PlaySound(SQUISHED, pPlayer->actorsqu);
                switch (DYNAMICTILEMAP(sprite[pPlayer->actorsqu].picnum))
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
                        if (sprite[pPlayer->actorsqu].yvel)
                            G_OperateRespawns(sprite[pPlayer->actorsqu].yvel);
                        A_DeleteSprite(pPlayer->actorsqu);
                        break;
                    case APLAYER__STATIC:
                    {
                        const int playerSquished = P_Get(pPlayer->actorsqu);
                        P_QuickKill(g_player[playerSquished].ps);
                        g_player[playerSquished].ps->frag_ps = playerNum;
                        break;
                    }
                    default:
                        if (A_CheckEnemySprite(&sprite[pPlayer->actorsqu]))
                            P_AddKills(pPlayer, 1);
                        A_DeleteSprite(pPlayer->actorsqu);
                        break;
                }
            }
            pPlayer->actorsqu = -1;
        }
        else if (pPlayer->actorsqu >= 0)
            pPlayer->q16ang += fix16_from_int(
            G_GetAngleDelta(fix16_to_int(pPlayer->q16ang),
                            getangle(sprite[pPlayer->actorsqu].x - pPlayer->pos.x, sprite[pPlayer->actorsqu].y - pPlayer->pos.y))
            >> 2);
    }
#endif

    if (P_DoCounters(playerNum))
        return;

    P_ProcessWeapon(playerNum);
}


#define SJSON_IMPLEMENT
#include "sjson.h"

int portableBackupSave(const char * path, const char * name, int volume, int level)
{
    if (!FURY)
        return 0;

    char fn[BMAX_PATH];

    if (G_ModDirSnprintf(fn, sizeof(fn), "%s.ext", path))
    {
        return 1;
    }

    sjson_context * ctx = sjson_create_context(0, 0, NULL);
    if (!ctx)
    {
        buildprint("Could not create sjson_context\n");
        return 1;
    }

    sjson_node * root = sjson_mkobject(ctx);

    sjson_put_string(ctx, root, "name", name);
    // sjson_put_string(ctx, root, "map", currentboardfilename);
    sjson_put_int(ctx, root, "volume", volume);
    sjson_put_int(ctx, root, "level", level);
    sjson_put_int(ctx, root, "skill", ud.player_skill);

    {
        sjson_node * players = sjson_mkarray(ctx);
        sjson_append_member(ctx, root, "players", players);

        for (int TRAVERSE_CONNECT(p))
        {
            playerdata_t const * playerData = &g_player[p];
            DukePlayer_t const * ps = playerData->ps;
            auto pSprite = (uspritetype const *)&sprite[ps->i];

            sjson_node * player = sjson_mkobject(ctx);
            sjson_append_element(players, player);

            sjson_put_int(ctx, player, "extra", pSprite->extra);
            sjson_put_int(ctx, player, "max_player_health", ps->max_player_health);

            sjson_node * gotweapon = sjson_put_array(ctx, player, "gotweapon");
            for (int w = 0; w < MAX_WEAPONS; ++w)
                sjson_append_element(gotweapon, sjson_mkbool(ctx, !!(ps->gotweapon & (1<<w))));

            sjson_put_int16s(ctx, player, "ammo_amount", ps->ammo_amount, MAX_WEAPONS);
            sjson_put_int16s(ctx, player, "max_ammo_amount", ps->max_ammo_amount, MAX_WEAPONS);
            sjson_put_int16s(ctx, player, "inv_amount", ps->inv_amount, GET_MAX);

            sjson_put_int(ctx, player, "max_shield_amount", ps->max_shield_amount);

            sjson_put_int(ctx, player, "curr_weapon", ps->curr_weapon);
            sjson_put_int(ctx, player, "subweapon", ps->subweapon);
            sjson_put_int(ctx, player, "inven_icon", ps->inven_icon);

            sjson_node* vars = sjson_mkobject(ctx);
            sjson_append_member(ctx, player, "vars", vars);

            for (int j=0; j<g_gameVarCount; j++)
            {
                gamevar_t & var = aGameVars[j];

                if (!(var.flags & GAMEVAR_SERIALIZE))
                    continue;

                if ((var.flags & (GAMEVAR_PERPLAYER|GAMEVAR_PERACTOR)) != GAMEVAR_PERPLAYER)
                    continue;

                sjson_put_int(ctx, vars, var.szLabel, Gv_GetVar(j, ps->i, p));
            }
        }
    }

    {
        sjson_node * vars = sjson_mkobject(ctx);
        sjson_append_member(ctx, root, "vars", vars);

        for (int j=0; j<g_gameVarCount; j++)
        {
            gamevar_t & var = aGameVars[j];

            if (!(var.flags & GAMEVAR_SERIALIZE))
                continue;

            if (var.flags & (GAMEVAR_PERPLAYER|GAMEVAR_PERACTOR))
                continue;

            sjson_put_int(ctx, vars, var.szLabel, Gv_GetVar(j));
        }
    }

    char errmsg[256];
    if (!sjson_check(root, errmsg))
    {
        buildprint(errmsg, "\n");
        sjson_destroy_context(ctx);
        return 1;
    }

    char * encoded = sjson_stringify(ctx, root, "  ");

    buildvfs_FILE fil = buildvfs_fopen_write(fn);
    if (!fil)
    {
        sjson_destroy_context(ctx);
        return 1;
    }

    buildvfs_fwrite(encoded, strlen(encoded), 1, fil);
    buildvfs_fclose(fil);

    sjson_free_string(ctx, encoded);
    sjson_destroy_context(ctx);

    return 0;
}
