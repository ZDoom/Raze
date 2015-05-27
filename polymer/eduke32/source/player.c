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
#include "common_game.h"
#include "osd.h"
#include "player.h"
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

void P_UpdateScreenPal(DukePlayer_t *p)
{
    int32_t intowater = 0;
    const int32_t sect = p->cursectnum;

    if (p->heat_on) p->palette = SLIMEPAL;
    else if (sect < 0) p->palette = BASEPAL;
    else if (sector[sect].ceilingpicnum >= FLOORSLIME && sector[sect].ceilingpicnum <= FLOORSLIME+2)
    {
        p->palette = SLIMEPAL;
        intowater = 1;
    }
    else
    {
        if (sector[p->cursectnum].lotag == ST_2_UNDERWATER) p->palette = WATERPAL;
        else p->palette = BASEPAL;
        intowater = 1;
    }

    g_restorePalette = 1+intowater;
}

static void P_IncurDamage(DukePlayer_t *p)
{
    int32_t damage;

    if (VM_OnEvent(EVENT_INCURDAMAGE, p->i, P_Get(p->i)) != 0)
        return;

    sprite[p->i].extra -= p->extra_extra8>>8;

    damage = sprite[p->i].extra - p->last_extra;

    if (damage >= 0)
        return;

    p->extra_extra8 = 0;

    if (p->inv_amount[GET_SHIELD] > 0)
    {
        int32_t shield_damage =  damage * (20 + (krand()%30)) / 100;
        damage -= shield_damage;

        p->inv_amount[GET_SHIELD] += shield_damage;

        if (p->inv_amount[GET_SHIELD] < 0)
        {
            damage += p->inv_amount[GET_SHIELD];
            p->inv_amount[GET_SHIELD] = 0;
        }
    }

    sprite[p->i].extra = p->last_extra + damage;
}

void P_QuickKill(DukePlayer_t *p)
{
    P_PalFrom(p, 48, 48,48,48);

    sprite[p->i].extra = 0;
    sprite[p->i].cstat |= 32768;

    if (ud.god == 0)
        A_DoGuts(p->i,JIBS6,8);
}

static void A_DoWaterTracers(int32_t x1,int32_t y1,int32_t z1,int32_t x2,int32_t y2,int32_t z2,int32_t n)
{
    int32_t i, xv, yv, zv;
    int16_t sect = -1;

    i = n+1;
    xv = tabledivide32_noinline(x2-x1, i);
    yv = tabledivide32_noinline(y2-y1, i);
    zv = tabledivide32_noinline(z2-z1, i);

    if ((klabs(x1-x2)+klabs(y1-y2)) < 3084)
        return;

    for (i=n; i>0; i--)
    {
        x1 += xv;
        y1 += yv;
        z1 += zv;
        updatesector(x1,y1,&sect);
        if (sect < 0)
            break;

        if (sector[sect].lotag == ST_2_UNDERWATER)
            A_InsertSprite(sect,x1,y1,z1,WATERBUBBLE,-32,4+(krand()&3),4+(krand()&3),krand()&2047,0,0,g_player[0].ps->i,5);
        else
            A_InsertSprite(sect,x1,y1,z1,SMALLSMOKE,-32,14,14,0,0,0,g_player[0].ps->i,5);
    }
}

static inline projectile_t * Proj_GetProjectile(int tile)
{
    return ((unsigned)tile < MAXTILES && g_tile[tile].proj) ? g_tile[tile].proj : &DefaultProjectile;
}

static void A_HitscanProjTrail(const vec3_t *sv, const vec3_t *dv, int32_t ang, int32_t atwith)
{
    int32_t n, j, i;
    int16_t sect = -1;
    vec3_t srcvect;
    vec3_t destvect;

    const projectile_t *const proj = Proj_GetProjectile(atwith);

    Bmemcpy(&destvect, dv, sizeof(vec3_t));

    srcvect.x = sv->x + tabledivide32_noinline(sintable[(348+ang+512)&2047], proj->offset);
    srcvect.y = sv->y + tabledivide32_noinline(sintable[(ang+348)&2047], proj->offset);
    srcvect.z = sv->z + 1024+(proj->toffset<<8);

    n = ((FindDistance2D(srcvect.x-destvect.x,srcvect.y-destvect.y))>>8)+1;

    destvect.x = tabledivide32_noinline((destvect.x-srcvect.x), n);
    destvect.y = tabledivide32_noinline((destvect.y-srcvect.y), n);
    destvect.z = tabledivide32_noinline((destvect.z-srcvect.z), n);

    srcvect.x += destvect.x>>2;
    srcvect.y += destvect.y>>2;
    srcvect.z += (destvect.z>>2);

    for (i=proj->tnum; i>0; i--)
    {
        srcvect.x += destvect.x;
        srcvect.y += destvect.y;
        srcvect.z += destvect.z;
        updatesector(srcvect.x,srcvect.y,&sect);
        if (sect < 0)
            break;
        getzsofslope(sect,srcvect.x,srcvect.y,&n,&j);
        if (srcvect.z > j || srcvect.z < n)
            break;
        j = A_InsertSprite(sect,srcvect.x,srcvect.y,srcvect.z,proj->trail,-32,
                           proj->txrepeat,proj->tyrepeat,ang,0,0,g_player[0].ps->i,0);
        changespritestat(j, STAT_ACTOR);
    }
}

int32_t A_GetHitscanRange(int32_t i)
{
    int32_t zoff = (PN == APLAYER) ? PHEIGHT : 0;
    hitdata_t hit;

    SZ -= zoff;
    hitscan((const vec3_t *)&sprite[i],SECT,
            sintable[(SA+512)&2047],
            sintable[SA&2047],
            0,&hit,CLIPMASK1);
    SZ += zoff;

    return (FindDistance2D(hit.pos.x-SX,hit.pos.y-SY));
}

static int32_t A_FindTargetSprite(const spritetype *s, int32_t aang, int32_t atwith)
{
    int32_t gotshrinker,gotfreezer;
    int32_t i, j, a, k, cans;
    static const int32_t aimstats[] = {
        STAT_PLAYER, STAT_DUMMYPLAYER, STAT_ACTOR, STAT_ZOMBIEACTOR
    };
    int32_t dx1, dy1, dx2, dy2, dx3, dy3, smax, sdist;
    int32_t xv, yv;

    const int32_t snum = s->picnum == APLAYER ? P_GetP(s) : -1;

    if (s->picnum == APLAYER)
    {
        if (!g_player[snum].ps->auto_aim)
            return -1;

        if (g_player[snum].ps->auto_aim == 2)
        {
            if (A_CheckSpriteTileFlags(atwith,SFLAG_PROJECTILE) && (Proj_GetProjectile(atwith)->workslike & PROJECTILE_RPG))
                return -1;

            switch (DYNAMICTILEMAP(atwith))
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
    }

    a = s->ang;

    j = -1;

    gotshrinker = (s->picnum == APLAYER && PWEAPON(snum, g_player[snum].ps->curr_weapon, WorksLike) == SHRINKER_WEAPON);
    gotfreezer = (s->picnum == APLAYER && PWEAPON(snum, g_player[snum].ps->curr_weapon, WorksLike) == FREEZE_WEAPON);

    smax = INT32_MAX;

    dx1 = sintable[(a+512-aang)&2047];
    dy1 = sintable[(a-aang)&2047];
    dx2 = sintable[(a+512+aang)&2047];
    dy2 = sintable[(a+aang)&2047];

    dx3 = sintable[(a+512)&2047];
    dy3 = sintable[a&2047];

    for (k=0; k<4; k++)
    {
        if (j >= 0)
            break;
        for (i=headspritestat[aimstats[k]]; i >= 0; i=nextspritestat[i])
            if (sprite[i].xrepeat > 0 && sprite[i].extra >= 0 && (sprite[i].cstat&(257+32768)) == 257)
                if (A_CheckEnemySprite(&sprite[i]) || k < 2)
                {
                    if (A_CheckEnemySprite(&sprite[i]) || PN == APLAYER || PN == SHARK)
                    {
                        if (PN == APLAYER && s->picnum == APLAYER && s != &sprite[i] &&
                                //                        ud.ffire == 0 &&
                                (GTFLAGS(GAMETYPE_PLAYERSFRIENDLY) || (GTFLAGS(GAMETYPE_TDM) &&
                                        g_player[P_Get(i)].ps->team == g_player[snum].ps->team)))
                            continue;

                        if (gotshrinker && sprite[i].xrepeat < 30)
                        {
                            if (PN == SHARK)
                            {
                                if (sprite[i].xrepeat < 20) continue;
                                continue;
                            }
                            else if (!(PN >= GREENSLIME && PN <= GREENSLIME+7))
                                continue;
                        }
                        if (gotfreezer && sprite[i].pal == 1) continue;
                    }

                    xv = (SX-s->x);
                    yv = (SY-s->y);

                    if ((dy1*xv <= dx1*yv) && (dy2*xv >= dx2*yv))
                    {
                        sdist = mulscale(dx3,xv,14) + mulscale(dy3,yv,14);

                        if (sdist > 512 && sdist < smax)
                        {
                            if (s->picnum == APLAYER)
                            {
                                const DukePlayer_t *const ps = g_player[P_GetP(s)].ps;
                                a = (klabs(scale(SZ-s->z,10,sdist)-(ps->horiz+ps->horizoff-100)) < 100);
                            }
                            else a = 1;

                            if (PN == ORGANTIC || PN == ROTATEGUN)
                                cans = cansee(SX,SY,SZ,SECT,s->x,s->y,s->z-(32<<8),s->sectnum);
                            else cans = cansee(SX,SY,SZ-(32<<8),SECT,s->x,s->y,s->z-(32<<8),s->sectnum);

                            if (a && cans)
                            {
                                smax = sdist;
                                j = i;
                            }
                        }
                    }
                }
    }

    return j;
}

static void A_SetHitData(int32_t i, const hitdata_t *hit)
{
    actor[i].t_data[6] = hit->wall;
    actor[i].t_data[7] = hit->sect;
    actor[i].t_data[8] = hit->sprite;
}

static int32_t CheckShootSwitchTile(int32_t pn)
{
    return pn == DIPSWITCH || pn == DIPSWITCH+1 ||
        pn == DIPSWITCH2 || pn == DIPSWITCH2+1 ||
        pn == DIPSWITCH3 || pn == DIPSWITCH3+1 ||
        pn == HANDSWITCH || pn == HANDSWITCH+1;
}

static int32_t safeldist(int32_t spritenum1, const spritetype *s2)
{
    int32_t dst = ldist(&sprite[spritenum1], s2);
    return dst ? dst : 1;
}

// flags:
//  1: do sprite center adjustment (cen-=(8<<8)) for GREENSLIME or ROTATEGUN
//  2: do auto getangle only if not RECON (if clear, do unconditionally)
static int32_t GetAutoAimAngle(int32_t i, int32_t p, int32_t atwith,
                               int32_t cen_add, int32_t flags,
                               const vec3_t *srcvect, int32_t vel,
                               int32_t *zvel, int16_t *sa)
{
    int32_t j = -1;

    Bassert((unsigned)p < MAXPLAYERS);

#ifdef LUNATIC
    g_player[p].ps->autoaimang = g_player[p].ps->auto_aim == 3 ? AUTO_AIM_ANGLE<<1 : AUTO_AIM_ANGLE;
#else
    Gv_SetVar(g_iAimAngleVarID, g_player[p].ps->auto_aim == 3 ? AUTO_AIM_ANGLE<<1 : AUTO_AIM_ANGLE, i, p);
#endif

    VM_OnEvent(EVENT_GETAUTOAIMANGLE, i, p);

    {
#ifdef LUNATIC
        int32_t aimang = g_player[p].ps->autoaimang;
#else
        int32_t aimang = Gv_GetVar(g_iAimAngleVarID, i, p);
#endif
        if (aimang > 0)
            j = A_FindTargetSprite(&sprite[i], aimang, atwith);
    }

    if (j >= 0)
    {
        const spritetype *const spr = &sprite[j];
        int32_t cen = 2*(spr->yrepeat*tilesiz[spr->picnum].y) + cen_add;
        int32_t dst;

        if (flags)
        {
            int32_t pn = spr->picnum;
            if ((pn >= GREENSLIME && pn <= GREENSLIME+7) || spr->picnum==ROTATEGUN)
            {
                cen -= (8<<8);
            }
        }

        dst = safeldist(g_player[p].ps->i, &sprite[j]);
        *zvel = tabledivide32_noinline((spr->z - srcvect->z - cen)*vel, dst);

        if (!(flags&2) || sprite[j].picnum != RECON)
            *sa = getangle(spr->x-srcvect->x, spr->y-srcvect->y);
    }

    return j;
}

static void Proj_MaybeSpawn(int32_t k, int32_t atwith, const hitdata_t *hit)
{
    // atwith < 0 is for hard-coded projectiles
    projectile_t * const proj = Proj_GetProjectile(atwith);
    int32_t spawntile = atwith < 0 ? -atwith : proj->spawns;

    if (spawntile >= 0)
    {
        int32_t wh = A_Spawn(k, spawntile);

        if (atwith >= 0)
        {
            if (proj->sxrepeat > 4)
                sprite[wh].xrepeat = proj->sxrepeat;
            if (proj->syrepeat > 4)
                sprite[wh].yrepeat = proj->syrepeat;
        }

        A_SetHitData(wh, hit);
    }
}

// <extra>: damage that this shotspark does
static int32_t Proj_InsertShotspark(const hitdata_t *hit, int32_t i, int32_t atwith,
                                    int32_t xyrepeat, int32_t ang, int32_t extra)
{
    int32_t k = A_InsertSprite(hit->sect, hit->pos.x, hit->pos.y, hit->pos.z,
                               SHOTSPARK1,-15, xyrepeat,xyrepeat, ang,0,0,i,4);
    sprite[k].extra = extra;
    // This is a hack to allow you to detect which weapon spawned a SHOTSPARK1:
    sprite[k].yvel = atwith;
    A_SetHitData(k, hit);

    return k;
}

static int32_t Proj_GetExtra(int32_t atwith)
{
    projectile_t * const proj = Proj_GetProjectile(atwith);
    int32_t extra = proj->extra;
    if (proj->extra_rand > 0)
        extra += (krand() % proj->extra_rand);
    return extra;
}

static void Proj_MaybeAddSpread(int32_t not_accurate_p, int32_t *zvel, int16_t *sa,
                                int32_t zRange, int32_t angRange)
{
    if (not_accurate_p)
    {
        // Ranges <= 1 mean no spread at all. A range of 1 calls krand() though.
        if (zRange > 0)
            *zvel += zRange/2 - krand()%zRange;
        if (angRange > 0)
            *sa += angRange/2 - krand()%angRange;
    }
}


static int32_t g_overrideShootZvel = 0;  // a boolean
static int32_t g_shootZvel;  // the actual zvel if the above is !=0

static int32_t A_GetShootZvel(int32_t defaultzvel)
{
    return g_overrideShootZvel ? g_shootZvel : defaultzvel;
}

// Prepare hitscan weapon fired from player p.
static void P_PreFireHitscan(int32_t i, int32_t p, int32_t atwith,
                             vec3_t *srcvect, int32_t *zvel, int16_t *sa,
                             int32_t accurate_autoaim_p,
                             int32_t not_accurate_p)
{
    int32_t angRange=32;
    int32_t zRange=256;

    int32_t j = GetAutoAimAngle(i, p, atwith, 5<<8, 0+1, srcvect, 256, zvel, sa);
    DukePlayer_t *const ps = g_player[p].ps;

#ifdef LUNATIC
    ps->angrange = angRange;
    ps->zrange = zRange;
#else
    Gv_SetVar(g_iAngRangeVarID,angRange, i,p);
    Gv_SetVar(g_iZRangeVarID,zRange,i,p);
#endif

    VM_OnEvent(EVENT_GETSHOTRANGE, i, p);

#ifdef LUNATIC
    angRange = ps->angrange;
    zRange = ps->zrange;
#else
    angRange=Gv_GetVar(g_iAngRangeVarID,i,p);
    zRange=Gv_GetVar(g_iZRangeVarID,i,p);
#endif

    if (accurate_autoaim_p)
    {
        if (!ps->auto_aim)
        {
            hitdata_t hit;

            *zvel = A_GetShootZvel((100-ps->horiz-ps->horizoff)<<5);

            hitscan(srcvect, sprite[i].sectnum, sintable[(*sa+512)&2047], sintable[*sa&2047],
                    *zvel<<6,&hit,CLIPMASK1);

            if (hit.sprite != -1)
            {
                const int32_t hitstatnumsbitmap =
                    ((1<<STAT_ACTOR) | (1<<STAT_ZOMBIEACTOR) | (1<<STAT_PLAYER) | (1<<STAT_DUMMYPLAYER));
                const int32_t st = sprite[hit.sprite].statnum;

                if (st>=0 && st<=30 && (hitstatnumsbitmap&(1<<st)))
                    j = hit.sprite;
            }
        }

        if (j == -1)
        {
            *zvel = (100-ps->horiz-ps->horizoff)<<5;
            Proj_MaybeAddSpread(not_accurate_p, zvel, sa, zRange, angRange);
        }
    }
    else
    {
        if (j == -1)  // no target
            *zvel = (100-ps->horiz-ps->horizoff)<<5;
        Proj_MaybeAddSpread(not_accurate_p, zvel, sa, zRange, angRange);
    }

    srcvect->z -= (2<<8);
}

// Hitscan weapon fired from actor (sprite s);
static void A_PreFireHitscan(const spritetype *s, vec3_t *srcvect, int32_t *zvel, int16_t *sa,
                             int32_t not_accurate_p)
{
    const int32_t j = A_FindPlayer(s, NULL);
    const DukePlayer_t *targetps = g_player[j].ps;

    const int32_t d = safeldist(targetps->i, s);
    *zvel = tabledivide32_noinline((targetps->pos.z-srcvect->z)<<8, d);

    srcvect->z -= (4<<8);

    if (s->picnum != BOSS1)
    {
        Proj_MaybeAddSpread(not_accurate_p, zvel, sa, 256, 64);
    }
    else
    {
        *sa = getangle(targetps->pos.x-srcvect->x, targetps->pos.y-srcvect->y);

        Proj_MaybeAddSpread(not_accurate_p, zvel, sa, 256, 128);
    }
}

static int32_t Proj_DoHitscan(int32_t i, int32_t cstatmask,
                              const vec3_t *srcvect, int32_t zvel, int16_t sa,
                              hitdata_t *hit)
{
    spritetype *const s = &sprite[i];

    s->cstat &= ~cstatmask;

    zvel = A_GetShootZvel(zvel);

    hitscan(srcvect, s->sectnum,
            sintable[(sa+512)&2047],
            sintable[sa&2047],
            zvel<<6, hit, CLIPMASK1);

    s->cstat |= cstatmask;

    return (hit->sect < 0);
}

static void Proj_DoRandDecalSize(int32_t spritenum, int32_t atwith)
{
    const projectile_t *const proj = Proj_GetProjectile(atwith);

    if (proj->workslike & PROJECTILE_RANDDECALSIZE)
    {
        int32_t wh = (krand()&proj->xrepeat);
        if (wh < proj->yrepeat)
            wh = proj->yrepeat;
        sprite[spritenum].xrepeat = wh;
        sprite[spritenum].yrepeat = wh;
    }
    else
    {
        sprite[spritenum].xrepeat = proj->xrepeat;
        sprite[spritenum].yrepeat = proj->yrepeat;
    }
}

static int32_t SectorContainsSE13(int32_t sectnum)
{
    int32_t i;
    if (sectnum >= 0)
        for (SPRITES_OF_SECT(sectnum, i))
            if (sprite[i].statnum == STAT_EFFECTOR && sprite[i].lotag == SE_13_EXPLOSIVE)
                return 1;
    return 0;
}

// Maybe handle bit 2 (swap wall bottoms).
// (in that case walltype *hitwal may be stale)
static inline void HandleHitWall(hitdata_t *hit)
{
    const walltype *const hitwal = &wall[hit->wall];

    if ((hitwal->cstat&2) && redwallp(hitwal))
        if (hit->pos.z >= sector[hitwal->nextsector].floorz)
            hit->wall = hitwal->nextwall;
}

// Maybe damage a ceiling or floor as the consequence of projectile impact.
// Returns 1 if projectile hit a parallaxed ceiling.
// NOTE: Compare with Proj_MaybeDamageCF() in actors.c
static int32_t Proj_MaybeDamageCF2(int32_t zvel, int32_t hitsect)
{
    if (zvel < 0)
    {
        Bassert(hitsect >= 0);

        if (sector[hitsect].ceilingstat&1)
            return 1;

        Sect_DamageCeilingOrFloor(0, hitsect);
    }
    else if (zvel > 0)
    {
        Bassert(hitsect >= 0);

        if (sector[hitsect].floorstat&1)
        {
            // Keep original Duke3D behavior: pass projectiles through
            // parallaxed ceilings, but NOT through such floors.
            return 0;
        }

        Sect_DamageCeilingOrFloor(1, hitsect);
    }

    return 0;
}

// Finish shooting hitscan weapon from player <p>. <k> is the inserted SHOTSPARK1.
// * <spawnatimpacttile> is passed to Proj_MaybeSpawn()
// * <decaltile> and <damagewalltile> are for wall impact
// * <damagewalltile> is passed to A_DamageWall()
// * <flags> is for decals upon wall impact:
//    1: handle random decal size (tile <atwith>)
//    2: set cstat to wall-aligned + random x/y flip
//
// TODO: maybe split into 3 cases (hit neither wall nor sprite, hit sprite, hit wall)?
static int32_t P_PostFireHitscan(int32_t p, int32_t k, hitdata_t *hit, int32_t i, int32_t atwith, int32_t zvel,
                                 int32_t spawnatimpacttile, int32_t decaltile, int32_t damagewalltile,
                                 int32_t flags)
{
    if (hit->wall == -1 && hit->sprite == -1)
    {
        if (Proj_MaybeDamageCF2(zvel, hit->sect))
        {
            sprite[k].xrepeat = 0;
            sprite[k].yrepeat = 0;
            return -1;
        }

        Proj_MaybeSpawn(k, spawnatimpacttile, hit);
    }
    else if (hit->sprite >= 0)
    {
        A_DamageObject(hit->sprite, k);

        if (sprite[hit->sprite].picnum == APLAYER &&
            (ud.ffire == 1 || (!GTFLAGS(GAMETYPE_PLAYERSFRIENDLY) && GTFLAGS(GAMETYPE_TDM) &&
                               g_player[P_Get(hit->sprite)].ps->team != g_player[P_Get(i)].ps->team)))
        {
            int32_t l = A_Spawn(k, JIBS6);
            sprite[k].xrepeat = sprite[k].yrepeat = 0;
            sprite[l].z += (4<<8);
            sprite[l].xvel = 16;
            sprite[l].xrepeat = sprite[l].yrepeat = 24;
            sprite[l].ang += 64-(krand()&127);
        }
        else
        {
            Proj_MaybeSpawn(k, spawnatimpacttile, hit);
        }

        if (p >= 0 && CheckShootSwitchTile(sprite[hit->sprite].picnum))
        {
            P_ActivateSwitch(p, hit->sprite, 1);
            return -1;
        }
    }
    else if (hit->wall >= 0)
    {
        const walltype *const hitwal = &wall[hit->wall];

        Proj_MaybeSpawn(k, spawnatimpacttile, hit);

        if (CheckDoorTile(hitwal->picnum) == 1)
            goto SKIPBULLETHOLE;

        if (p >= 0 && CheckShootSwitchTile(hitwal->picnum))
        {
            P_ActivateSwitch(p, hit->wall, 0);
            return -1;
        }

        if (hitwal->hitag != 0 || (hitwal->nextwall >= 0 && wall[hitwal->nextwall].hitag != 0))
            goto SKIPBULLETHOLE;

        if (hit->sect >= 0 && sector[hit->sect].lotag == 0)
            if (hitwal->overpicnum != BIGFORCE && (hitwal->cstat&16) == 0)
                if ((hitwal->nextsector >= 0 && sector[hitwal->nextsector].lotag == 0) ||
                    (hitwal->nextsector == -1 && sector[hit->sect].lotag == 0))
                {
                    int32_t l;

                    if (SectorContainsSE13(hitwal->nextsector))
                        goto SKIPBULLETHOLE;

                    for (SPRITES_OF(STAT_MISC, l))
                        if (sprite[l].picnum == decaltile)
                            if (dist(&sprite[l],&sprite[k]) < (12+(krand()&7)))
                                goto SKIPBULLETHOLE;

                    if (decaltile >= 0)
                    {
                        l = A_Spawn(k, decaltile);

                        if (!A_CheckSpriteFlags(l, SFLAG_DECAL))
                            actor[l].flags |= SFLAG_DECAL;

                        sprite[l].xvel = -1;
                        sprite[l].ang = getangle(hitwal->x-wall[hitwal->point2].x,
                                                 hitwal->y-wall[hitwal->point2].y)+512;
                        if (flags&1)
                            Proj_DoRandDecalSize(l, atwith);

                        if (flags&2)
                            sprite[l].cstat = 16+(krand()&(8+4));

                        sprite[l].x -= sintable[(sprite[l].ang+2560)&2047]>>13;
                        sprite[l].y -= sintable[(sprite[l].ang+2048)&2047]>>13;

                        A_SetSprite(l, CLIPMASK0);

                        // BULLETHOLE already adds itself to the deletion queue in
                        // A_Spawn(). However, some other tiles do as well.
                        if (decaltile != BULLETHOLE)
                            A_AddToDeleteQueue(l);
                    }
                }

SKIPBULLETHOLE:
        HandleHitWall(hit);

        A_DamageWall(k, hit->wall, &hit->pos, damagewalltile);
    }

    return 0;
}

// Finish shooting hitscan weapon from actor (sprite <i>).
static int32_t A_PostFireHitscan(const hitdata_t *hit, int32_t i, int32_t atwith, int32_t sa, int32_t extra,
                                 int32_t spawnatimpacttile, int32_t damagewalltile)
{
    int32_t k = Proj_InsertShotspark(hit, i, atwith, 24, sa, extra);

    if (hit->sprite >= 0)
    {
        A_DamageObject(hit->sprite, k);

        if (sprite[hit->sprite].picnum != APLAYER)
            Proj_MaybeSpawn(k, spawnatimpacttile, hit);
        else
            sprite[k].xrepeat = sprite[k].yrepeat = 0;
    }
    else if (hit->wall >= 0)
        A_DamageWall(k, hit->wall, &hit->pos, damagewalltile);

    return k;
}

// Common "spawn blood?" predicate.
// minzdiff: minimal "step" height for blood to be spawned
static int32_t Proj_CheckBlood(const vec3_t *srcvect, const hitdata_t *hit,
                               int32_t projrange, int32_t minzdiff)
{
    const walltype * hitwal;

    if (hit->wall < 0 || hit->sect < 0)
        return 0;

    hitwal = &wall[hit->wall];

    if (FindDistance2D(srcvect->x-hit->pos.x, srcvect->y-hit->pos.y) < projrange)
        if (hitwal->overpicnum != BIGFORCE && (hitwal->cstat&16) == 0)
            if (sector[hit->sect].lotag == 0)
                if (hitwal->nextsector < 0 ||
                    (sector[hitwal->nextsector].lotag == 0 && sector[hit->sect].lotag == 0 &&
                    sector[hit->sect].floorz-sector[hitwal->nextsector].floorz > minzdiff))
                    return 1;

    return 0;
}

static void Proj_HandleKnee(hitdata_t *hit, int32_t i, int32_t p, int32_t atwith, int32_t sa,
                            const projectile_t *proj, int32_t inserttile,
                            int32_t addrandextra, int32_t spawnatimpacttile, int32_t soundnum)
{
    const DukePlayer_t *const ps = p >= 0 ? g_player[p].ps : NULL;

    int32_t j = A_InsertSprite(hit->sect,hit->pos.x,hit->pos.y,hit->pos.z,
                               inserttile,-15,0,0,sa,32,0,i,4);

    if (proj != NULL)
    {
        // Custom projectiles.
        SpriteProjectile[j].workslike = Proj_GetProjectile(sprite[j].picnum)->workslike;
        sprite[j].extra = proj->extra;
    }

    if (addrandextra > 0)
        sprite[j].extra += (krand()&addrandextra);

    if (p >= 0)
    {
        if (spawnatimpacttile >= 0)
        {
            int32_t k = A_Spawn(j, spawnatimpacttile);
            sprite[k].z -= (8<<8);
            A_SetHitData(k, hit);
        }

        if (soundnum >= 0)
            A_PlaySound(soundnum, j);
    }

    if (p >= 0 && ps->inv_amount[GET_STEROIDS] > 0 && ps->inv_amount[GET_STEROIDS] < 400)
        sprite[j].extra += (ps->max_player_health>>2);

    if (hit->sprite >= 0 && sprite[hit->sprite].picnum != ACCESSSWITCH && sprite[hit->sprite].picnum != ACCESSSWITCH2)
    {
        A_DamageObject(hit->sprite, j);
        if (p >= 0)
            P_ActivateSwitch(p, hit->sprite,1);
    }
    else if (hit->wall >= 0)
    {
        HandleHitWall(hit);

        if (wall[hit->wall].picnum != ACCESSSWITCH && wall[hit->wall].picnum != ACCESSSWITCH2)
        {
            A_DamageWall(j, hit->wall, &hit->pos, atwith);
            if (p >= 0)
                P_ActivateSwitch(p, hit->wall,0);
        }
    }
}

#define MinibossScale(s) (((s)*sprite[i].yrepeat)/80)

static int32_t A_ShootCustom(const int32_t i, const int32_t atwith, int16_t sa, vec3_t * const srcvect)
{
    /* Custom projectiles */
    projectile_t *const proj = Proj_GetProjectile(atwith);
    int32_t j, k = -1, l;
    int32_t vel, zvel = 0;
    hitdata_t hit;
    spritetype *const s = &sprite[i];
    const int16_t sect = s->sectnum;
    const int32_t p = (s->picnum == APLAYER) ? P_GetP(s) : -1;
    DukePlayer_t *const ps = p >= 0 ? g_player[p].ps : NULL;

#ifdef POLYMER
    if (getrendermode() == REND_POLYMER && proj->flashcolor)
    {
        int32_t x = ((sintable[(s->ang + 512) & 2047]) >> 7), y = ((sintable[(s->ang) & 2047]) >> 7);

        s->x += x;
        s->y += y;
        G_AddGameLight(0, i, PHEIGHT, 8192, proj->flashcolor, PR_LIGHT_PRIO_MAX_GAME);
        actor[i].lightcount = 2;
        s->x -= x;
        s->y -= y;
    }
#endif // POLYMER

    if (proj->offset == 0)
        proj->offset = 1;

    switch (proj->workslike & PROJECTILE_TYPE_MASK)
    {
    case PROJECTILE_HITSCAN:
        if (!(proj->workslike & PROJECTILE_NOSETOWNERSHADE) && s->extra >= 0)
            s->shade = proj->shade;

        if (p >= 0)
            P_PreFireHitscan(i, p, atwith, srcvect, &zvel, &sa,
            proj->workslike & PROJECTILE_ACCURATE_AUTOAIM,
            !(proj->workslike & PROJECTILE_ACCURATE));
        else
            A_PreFireHitscan(s, srcvect, &zvel, &sa,
            !(proj->workslike & PROJECTILE_ACCURATE));

        if (Proj_DoHitscan(i, (proj->cstat >= 0) ? proj->cstat : 256 + 1,
            srcvect, zvel, sa, &hit))
            return -1;

        if (proj->range > 0 && klabs(srcvect->x - hit.pos.x) + klabs(srcvect->y - hit.pos.y) > proj->range)
            return -1;

        if (proj->trail >= 0)
            A_HitscanProjTrail(srcvect, &hit.pos, sa, atwith);

        if (proj->workslike & PROJECTILE_WATERBUBBLES)
        {
            if ((krand() & 15) == 0 && sector[hit.sect].lotag == ST_2_UNDERWATER)
                A_DoWaterTracers(hit.pos.x, hit.pos.y, hit.pos.z,
                srcvect->x, srcvect->y, srcvect->z, 8 - (ud.multimode >> 1));
        }

        if (p >= 0)
        {
            k = Proj_InsertShotspark(&hit, i, atwith, 10, sa, Proj_GetExtra(atwith));

            if (P_PostFireHitscan(p, k, &hit, i, atwith, zvel,
                atwith, proj->decal, atwith, 1 + 2) < 0)
                return -1;
        }
        else
        {
            k = A_PostFireHitscan(&hit, i, atwith, sa, Proj_GetExtra(atwith),
                atwith, atwith);
        }

        if ((krand() & 255) < 4 && proj->isound >= 0)
            S_PlaySound3D(proj->isound, k, &hit.pos);

        return -1;

    case PROJECTILE_RPG:
        if (!(proj->workslike & PROJECTILE_NOSETOWNERSHADE) && s->extra >= 0)
            s->shade = proj->shade;

        vel = proj->vel;

        j = -1;

        if (p >= 0)
        {
            // NOTE: j is a SPRITE_INDEX
            j = GetAutoAimAngle(i, p, atwith, 8<<8, 0+2, srcvect, vel, &zvel, &sa);

            if (j < 0)
                zvel = (100-ps->horiz-ps->horizoff)*(proj->vel/8);

            if (proj->sound >= 0)
                A_PlaySound(proj->sound, i);
        }
        else
        {
            if (!(proj->workslike & PROJECTILE_NOAIM))
            {
                // NOTE: j is a player index
                j = A_FindPlayer(s, NULL);
                sa = getangle(g_player[j].ps->opos.x-srcvect->x, g_player[j].ps->opos.y-srcvect->y);

                l = safeldist(g_player[j].ps->i, s);
                zvel = tabledivide32_noinline((g_player[j].ps->opos.z - srcvect->z)*vel, l);

                if (A_CheckEnemySprite(s) && (AC_MOVFLAGS(s, &actor[i]) & face_player_smart))
                    sa = s->ang + (krand() & 31) - 16;
            }
        }

        if (numplayers > 1 && g_netClient) return -1;

        // l may be a SPRITE_INDEX, see above
        l = (p >= 0 && j >= 0) ? j : -1;

        zvel = A_GetShootZvel(zvel);
        j = A_InsertSprite(sect,
            srcvect->x + tabledivide32_noinline(sintable[(348 + sa + 512) & 2047], proj->offset),
            srcvect->y + tabledivide32_noinline(sintable[(sa + 348) & 2047], proj->offset),
            srcvect->z - (1 << 8), atwith, 0, 14, 14, sa, vel, zvel, i, 4);

        sprite[j].xrepeat = proj->xrepeat;
        sprite[j].yrepeat = proj->yrepeat;

        if (proj->extra_rand > 0)
            sprite[j].extra += (krand()&proj->extra_rand);

        if (!(proj->workslike & PROJECTILE_BOUNCESOFFWALLS))
            sprite[j].yvel = l;  // NOT_BOUNCESOFFWALLS_YVEL
        else
        {
            if (proj->bounces >= 1) sprite[j].yvel = proj->bounces;
            else sprite[j].yvel = g_numFreezeBounces;
            sprite[j].zvel -= (2 << 4);
        }

        if (proj->cstat >= 0) sprite[j].cstat = proj->cstat;
        else sprite[j].cstat = 128;

        if (proj->clipdist != 255) sprite[j].clipdist = proj->clipdist;
        else sprite[j].clipdist = 40;

        SpriteProjectile[j] = *Proj_GetProjectile(sprite[j].picnum);

        return j;

    case PROJECTILE_KNEE:
        if (p >= 0)
        {
            zvel = (100 - ps->horiz - ps->horizoff) << 5;
            srcvect->z += (6 << 8);
            sa += 15;
        }
        else if (!(proj->workslike & PROJECTILE_NOAIM))
        {
            int32_t x;
            j = g_player[A_FindPlayer(s, &x)].ps->i;
            zvel = tabledivide32_noinline((sprite[j].z - srcvect->z) << 8, x + 1);
            sa = getangle(sprite[j].x - srcvect->x, sprite[j].y - srcvect->y);
        }

        Proj_DoHitscan(i, 0, srcvect, zvel, sa, &hit);

        if (hit.sect < 0) return -1;

        if (proj->range == 0)
            proj->range = 1024;

        if (proj->range > 0 && klabs(srcvect->x - hit.pos.x) + klabs(srcvect->y - hit.pos.y) > proj->range)
            return -1;

        Proj_HandleKnee(&hit, i, p, atwith, sa,
            proj, atwith,
            proj->extra_rand,
            proj->spawns, proj->sound);

        return -1;

    case PROJECTILE_BLOOD:
        sa += 64 - (krand() & 127);
        if (p < 0) sa += 1024;
        zvel = 1024 - (krand() & 2047);

        Proj_DoHitscan(i, 0, srcvect, zvel, sa, &hit);

        if (proj->range == 0)
            proj->range = 1024;

        if (Proj_CheckBlood(srcvect, &hit, proj->range,
            mulscale3(proj->yrepeat, tilesiz[proj->decal].y) << 8))
        {
            const walltype *const hitwal = &wall[hit.wall];

            if (FindDistance2D(hitwal->x - wall[hitwal->point2].x, hitwal->y - wall[hitwal->point2].y) >
                (mulscale3(proj->xrepeat + 8, tilesiz[proj->decal].x)))
            {
                if (SectorContainsSE13(hitwal->nextsector))
                    return -1;

                if (hitwal->nextwall >= 0 && wall[hitwal->nextwall].hitag != 0)
                    return -1;

                if (hitwal->hitag == 0 && proj->decal >= 0)
                {
                    k = A_Spawn(i, proj->decal);

                    if (!A_CheckSpriteFlags(k, SFLAG_DECAL))
                        actor[k].flags |= SFLAG_DECAL;

                    sprite[k].xvel = -1;
                    sprite[k].ang = getangle(hitwal->x - wall[hitwal->point2].x,
                        hitwal->y - wall[hitwal->point2].y) + 512;
                    Bmemcpy(&sprite[k], &hit.pos, sizeof(vec3_t));

                    Proj_DoRandDecalSize(k, atwith);

                    sprite[k].z += sprite[k].yrepeat << 8;

                    //                                sprite[k].cstat = 16+(krand()&12);
                    sprite[k].cstat = 16;

                    if (krand() & 1)
                        sprite[k].cstat |= 4;

                    if (krand() & 1)
                        sprite[k].cstat |= 8;

                    sprite[k].shade = sector[sprite[k].sectnum].floorshade;

                    sprite[k].x -= sintable[(sprite[k].ang + 2560) & 2047] >> 13;
                    sprite[k].y -= sintable[(sprite[k].ang + 2048) & 2047] >> 13;

                    A_SetSprite(k, CLIPMASK0);
                    A_AddToDeleteQueue(k);
                    changespritestat(k, 5);
                }
            }
        }

        return -1;

    default:
        return -1;
    }
}

int32_t A_ShootWithZvel(int32_t i, int32_t atwith, int32_t override_zvel)
{
    int16_t sa;
    vec3_t srcvect;
    spritetype *const s = &sprite[i];
    const int32_t p = (s->picnum == APLAYER) ? P_GetP(s) : -1;
    DukePlayer_t *const ps = p >= 0 ? g_player[p].ps : NULL;

    Bassert(atwith >= 0);

    if (override_zvel != SHOOT_HARDCODED_ZVEL)
    {
        g_overrideShootZvel = 1;
        g_shootZvel = override_zvel;
    }
    else
        g_overrideShootZvel = 0;

    if (s->picnum == APLAYER)
    {
        Bmemcpy(&srcvect,ps,sizeof(vec3_t));
        srcvect.z += ps->pyoff+(4<<8);
        sa = ps->ang;

        ps->crack_time = 777;
    }
    else
    {
        sa = s->ang;
        Bmemcpy(&srcvect,s,sizeof(vec3_t));
        srcvect.z -= (((s->yrepeat*tilesiz[s->picnum].y)<<1)-(4<<8));

        if (s->picnum != ROTATEGUN)
        {
            srcvect.z -= (7<<8);

            if (A_CheckEnemySprite(s) && PN != COMMANDER)
            {
                srcvect.x += (sintable[(sa+1024+96)&2047]>>7);
                srcvect.y += (sintable[(sa+512+96)&2047]>>7);
            }
        }

#ifdef POLYMER
        switch (DYNAMICTILEMAP(atwith))
        {
        case FIRELASER__STATIC:
        case SHOTGUN__STATIC:
        case SHOTSPARK1__STATIC:
        case CHAINGUN__STATIC:
        case RPG__STATIC:
        case MORTER__STATIC:
            {
                int32_t x = ((sintable[(s->ang+512)&2047])>>7), y = ((sintable[(s->ang)&2047])>>7);
                s->x += x;
                s->y += y;
                G_AddGameLight(0, i, PHEIGHT, 8192, 255+(95<<8), PR_LIGHT_PRIO_MAX_GAME);
                actor[i].lightcount = 2;
                s->x -= x;
                s->y -= y;
            }

            break;
        }
#endif // POLYMER
    }

    if (A_CheckSpriteTileFlags(atwith, SFLAG_PROJECTILE))
        return A_ShootCustom(i, atwith, sa, &srcvect);
    else
    {
        int32_t j, k = -1, l;
        int32_t vel, zvel = 0;
        hitdata_t hit;
        const int16_t sect = s->sectnum;

        switch (DYNAMICTILEMAP(atwith))
        {
        case BLOODSPLAT1__STATIC:
        case BLOODSPLAT2__STATIC:
        case BLOODSPLAT3__STATIC:
        case BLOODSPLAT4__STATIC:
            sa += 64 - (krand()&127);
            if (p < 0) sa += 1024;
            zvel = 1024-(krand()&2047);
            // fall-through
        case KNEE__STATIC:
            if (atwith == KNEE)
            {
                if (p >= 0)
                {
                    zvel = (100-ps->horiz-ps->horizoff)<<5;
                    srcvect.z += (6<<8);
                    sa += 15;
                }
                else
                {
                    int32_t x;
                    j = g_player[A_FindPlayer(s,&x)].ps->i;
                    zvel = tabledivide32_noinline((sprite[j].z-srcvect.z)<<8, x+1);
                    sa = getangle(sprite[j].x-srcvect.x,sprite[j].y-srcvect.y);
                }
            }

            Proj_DoHitscan(i, 0, &srcvect, zvel, sa, &hit);

            if (atwith >= BLOODSPLAT1 && atwith <= BLOODSPLAT4)
            {
                if (Proj_CheckBlood(&srcvect, &hit, 1024, 16<<8))
                {
                    const walltype *const hitwal = &wall[hit.wall];

                    if (SectorContainsSE13(hitwal->nextsector))
                        return -1;

                    if (hitwal->nextwall >= 0 && wall[hitwal->nextwall].hitag != 0)
                        return -1;

                    if (hitwal->hitag == 0)
                    {
                        k = A_Spawn(i,atwith);
                        sprite[k].xvel = -12;
                        sprite[k].ang = getangle(hitwal->x-wall[hitwal->point2].x,
                                                 hitwal->y-wall[hitwal->point2].y)+512;
                        Bmemcpy(&sprite[k], &hit.pos, sizeof(vec3_t));

                        sprite[k].cstat |= (krand()&4);
                        A_SetSprite(k,CLIPMASK0);
                        setsprite(k, (vec3_t *)&sprite[k]);
                        if (PN == OOZFILTER || PN == NEWBEAST)
                            sprite[k].pal = 6;
                    }
                }

                return -1;
            }

            if (hit.sect < 0) break;

            if (klabs(srcvect.x-hit.pos.x)+klabs(srcvect.y-hit.pos.y) < 1024)
                Proj_HandleKnee(&hit, i, p, atwith, sa,
                                NULL, KNEE, 7, SMALLSMOKE, KICK_HIT);
            break;

        case SHOTSPARK1__STATIC:
        case SHOTGUN__STATIC:
        case CHAINGUN__STATIC:
            if (s->extra >= 0) s->shade = -96;

            if (p >= 0)
                P_PreFireHitscan(i, p, atwith, &srcvect, &zvel, &sa,
                                 atwith == SHOTSPARK1__STATIC && !WW2GI && !NAM,
                                 1);
            else
                A_PreFireHitscan(s, &srcvect, &zvel, &sa, 1);

            if (Proj_DoHitscan(i, 256+1, &srcvect, zvel, sa, &hit))
                return -1;

            if ((krand()&15) == 0 && sector[hit.sect].lotag == ST_2_UNDERWATER)
                A_DoWaterTracers(hit.pos.x,hit.pos.y,hit.pos.z,
                                 srcvect.x,srcvect.y,srcvect.z,8-(ud.multimode>>1));

            if (p >= 0)
            {
                k = Proj_InsertShotspark(&hit, i, atwith, 10, sa,
                                         G_InitialActorStrength(atwith) + (krand()%6));

                if (P_PostFireHitscan(p, k, &hit, i, atwith, zvel,
                                      -SMALLSMOKE, BULLETHOLE, SHOTSPARK1, 0) < 0)
                    return -1;
            }
            else
            {
                k = A_PostFireHitscan(&hit, i, atwith, sa, G_InitialActorStrength(atwith),
                                      -SMALLSMOKE, SHOTSPARK1);
            }

            if ((krand()&255) < 4)
                S_PlaySound3D(PISTOL_RICOCHET, k, &hit.pos);

            return -1;

        case GROWSPARK__STATIC:
            if (p >= 0)
                P_PreFireHitscan(i, p, atwith, &srcvect, &zvel, &sa, 1, 1);
            else
                A_PreFireHitscan(s, &srcvect, &zvel, &sa, 1);

            if (Proj_DoHitscan(i, 256 + 1, &srcvect, zvel, sa, &hit))
                return -1;

            j = A_InsertSprite(hit.sect,hit.pos.x,hit.pos.y,hit.pos.z,GROWSPARK,-16,28,28,sa,0,0,i,1);

            sprite[j].pal = 2;
            sprite[j].cstat |= 130;
            sprite[j].xrepeat = sprite[j].yrepeat = 1;
            A_SetHitData(j, &hit);

            if (hit.wall == -1 && hit.sprite == -1 && hit.sect >= 0)
            {
                Proj_MaybeDamageCF2(zvel, hit.sect);
            }
            else if (hit.sprite >= 0) A_DamageObject(hit.sprite,j);
            else if (hit.wall >= 0 && wall[hit.wall].picnum != ACCESSSWITCH && wall[hit.wall].picnum != ACCESSSWITCH2)
                A_DamageWall(j,hit.wall,&hit.pos,atwith);

            break;

        case FIRELASER__STATIC:
        case SPIT__STATIC:
        case COOLEXPLOSION1__STATIC:
        {
            int32_t tsiz;

            if (s->extra >= 0) s->shade = -96;

            switch (atwith)
            {
            case SPIT__STATIC:
                vel = 292;
                break;
            case COOLEXPLOSION1__STATIC:
                if (s->picnum == BOSS2) vel = 644;
                else vel = 348;
                srcvect.z -= (4<<7);
                break;
            case FIRELASER__STATIC:
            default:
                vel = 840;
                srcvect.z -= (4<<7);
                break;
            }

            if (p >= 0)
            {
                j = GetAutoAimAngle(i, p, atwith, -(12<<8), 0, &srcvect, vel, &zvel, &sa);

                if (j < 0)
                    zvel = (100-ps->horiz-ps->horizoff)*98;
            }
            else
            {
                j = A_FindPlayer(s, NULL);
                //                sa = getangle(g_player[j].ps->opos.x-sx,g_player[j].ps->opos.y-sy);
                sa += 16-(krand()&31);
                hit.pos.x = safeldist(g_player[j].ps->i, s);
                zvel = tabledivide32_noinline((g_player[j].ps->opos.z - srcvect.z + (3<<8))*vel, hit.pos.x);
            }

            zvel = A_GetShootZvel(zvel);

            if (atwith == SPIT)
            {
                tsiz = 18;
                srcvect.z -= (10<<8);
            }
            else if (p >= 0)
                tsiz = 7;
            else
            {
                if (atwith == FIRELASER)
                {
                    if (p >= 0)
                        tsiz = 34;
                    else
                        tsiz = 18;
                }
                else
                    tsiz = 18;
            }

            j = A_InsertSprite(sect,srcvect.x,srcvect.y,srcvect.z,
                               atwith,-127,tsiz,tsiz,sa,vel,zvel,i,4);
            sprite[j].extra += (krand()&7);

            if (atwith == COOLEXPLOSION1)
            {
                sprite[j].shade = 0;
                if (PN == BOSS2)
                {
                    l = sprite[j].xvel;
                    sprite[j].xvel = MinibossScale(1024);
                    A_SetSprite(j,CLIPMASK0);
                    sprite[j].xvel = l;
                    sprite[j].ang += 128-(krand()&255);
                }
            }

            sprite[j].cstat = 128;
            sprite[j].clipdist = 4;

            sa = s->ang+32-(krand()&63);
            zvel += 512-(krand()&1023);

            return j;
        }

        case FREEZEBLAST__STATIC:
            srcvect.z += (3<<8);
        case RPG__STATIC:
            // XXX: "CODEDUP"
            if (s->extra >= 0) s->shade = -96;

            vel = 644;

            j = -1;

            if (p >= 0)
            {
                // NOTE: j is a SPRITE_INDEX
                j = GetAutoAimAngle(i, p, atwith, 8<<8, 0+2, &srcvect, vel, &zvel, &sa);

                if (j < 0)
                    zvel = (100-ps->horiz-ps->horizoff)*81;

                if (atwith == RPG)
                    A_PlaySound(RPG_SHOOT,i);
            }
            else
            {
                // NOTE: j is a player index
                j = A_FindPlayer(s, NULL);
                sa = getangle(g_player[j].ps->opos.x-srcvect.x, g_player[j].ps->opos.y-srcvect.y);
                if (PN == BOSS3)
                    srcvect.z -= MinibossScale(32<<8);
                else if (PN == BOSS2)
                {
                    vel += 128;
                    srcvect.z += MinibossScale(24<<8);
                }

                l = safeldist(g_player[j].ps->i, s);
                zvel = tabledivide32_noinline((g_player[j].ps->opos.z - srcvect.z)*vel, l);

                if (A_CheckEnemySprite(s) && (AC_MOVFLAGS(s, &actor[i]) & face_player_smart))
                    sa = s->ang+(krand()&31)-16;
            }

            if (numplayers > 1 && g_netClient)
                return -1;

            // l may be a SPRITE_INDEX, see above
            l = (p >= 0 && j >= 0) ? j : -1;

            zvel = A_GetShootZvel(zvel);
            j = A_InsertSprite(sect,
                               srcvect.x+(sintable[(348+sa+512)&2047]/448),
                               srcvect.y+(sintable[(sa+348)&2047]/448),
                               srcvect.z-(1<<8),atwith,0,14,14,sa,vel,zvel,i,4);

            sprite[j].extra += (krand()&7);
            if (atwith != FREEZEBLAST)
                sprite[j].yvel = l;  // RPG_YVEL
            else
            {
                sprite[j].yvel = g_numFreezeBounces;
                sprite[j].xrepeat >>= 1;
                sprite[j].yrepeat >>= 1;
                sprite[j].zvel -= (2<<4);
            }

            if (p == -1)
            {
                if (PN == BOSS3)
                {
                    if (krand()&1)
                    {
                        sprite[j].x -= MinibossScale(sintable[sa&2047]>>6);
                        sprite[j].y -= MinibossScale(sintable[(sa+1024+512)&2047]>>6);
                        sprite[j].ang -= MinibossScale(8);
                    }
                    else
                    {
                        sprite[j].x += MinibossScale(sintable[sa&2047]>>6);
                        sprite[j].y += MinibossScale(sintable[(sa+1024+512)&2047]>>6);
                        sprite[j].ang += MinibossScale(4);
                    }
                    sprite[j].xrepeat = MinibossScale(42);
                    sprite[j].yrepeat = MinibossScale(42);
                }
                else if (PN == BOSS2)
                {
                    sprite[j].x -= MinibossScale(sintable[sa&2047]/56);
                    sprite[j].y -= MinibossScale(sintable[(sa+1024+512)&2047]/56);
                    sprite[j].ang -= MinibossScale(8)+(krand()&255)-128;
                    sprite[j].xrepeat = 24;
                    sprite[j].yrepeat = 24;
                }
                else if (atwith != FREEZEBLAST)
                {
                    sprite[j].xrepeat = 30;
                    sprite[j].yrepeat = 30;
                    sprite[j].extra >>= 2;
                }
            }
            else if (PWEAPON(p, g_player[p].ps->curr_weapon, WorksLike) == DEVISTATOR_WEAPON)
            {
                sprite[j].extra >>= 2;
                sprite[j].ang += 16-(krand()&31);
                sprite[j].zvel += 256-(krand()&511);

                if (g_player[p].ps->hbomb_hold_delay)
                {
                    sprite[j].x -= sintable[sa&2047]/644;
                    sprite[j].y -= sintable[(sa+1024+512)&2047]/644;
                }
                else
                {
                    sprite[j].x += sintable[sa&2047]>>8;
                    sprite[j].y += sintable[(sa+1024+512)&2047]>>8;
                }
                sprite[j].xrepeat >>= 1;
                sprite[j].yrepeat >>= 1;
            }

            sprite[j].cstat = 128;
            if (atwith == RPG)
                sprite[j].clipdist = 4;
            else
                sprite[j].clipdist = 40;

            return j;

        case HANDHOLDINGLASER__STATIC:
        {
            const int32_t zoff = (p>=0) ? g_player[p].ps->pyoff : 0;
            if (p >= 0)
                zvel = (100-ps->horiz-ps->horizoff)*32;
            else zvel = 0;

            srcvect.z -= zoff;
            Proj_DoHitscan(i, 0, &srcvect, zvel, sa, &hit);
            srcvect.z += zoff;

            j = 0;
            if (hit.sprite >= 0) break;

            if (hit.wall >= 0 && hit.sect >= 0)
                if (((hit.pos.x-srcvect.x)*(hit.pos.x-srcvect.x)+(hit.pos.y-srcvect.y)*(hit.pos.y-srcvect.y)) < (290*290))
                {
                    // ST_2_UNDERWATER
                    if (wall[hit.wall].nextsector >= 0)
                    {
                        if (sector[wall[hit.wall].nextsector].lotag <= 2 && sector[hit.sect].lotag <= 2)
                            j = 1;
                    }
                    else if (sector[hit.sect].lotag <= 2)
                        j = 1;
                }

            if (j == 1)
            {
                int32_t lTripBombControl = (p < 0) ? 0 :
#ifdef LUNATIC
                    g_player[p].ps->tripbombControl;
#else
                    Gv_GetVarByLabel("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, g_player[p].ps->i, p);
#endif
                k = A_InsertSprite(hit.sect,hit.pos.x,hit.pos.y,hit.pos.z,TRIPBOMB,-16,4,5,sa,0,0,i,6);
                if (lTripBombControl & TRIPBOMB_TIMER)
                {
#ifdef LUNATIC
                    int32_t lLifetime = g_player[p].ps->tripbombLifetime;
                    int32_t lLifetimeVar = g_player[p].ps->tripbombLifetimeVar;
#else
                    int32_t lLifetime=Gv_GetVarByLabel("STICKYBOMB_LIFETIME", NAM_GRENADE_LIFETIME, g_player[p].ps->i, p);
                    int32_t lLifetimeVar=Gv_GetVarByLabel("STICKYBOMB_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, g_player[p].ps->i, p);
#endif
                    // set timer.  blows up when at zero....
                    actor[k].t_data[7]=lLifetime
                                       + mulscale(krand(),lLifetimeVar, 14)
                                       - lLifetimeVar;
                    // TIMER_CONTROL
                    actor[k].t_data[6]=1;
                }
                else
                    sprite[k].hitag = k;

                A_PlaySound(LASERTRIP_ONWALL,k);
                sprite[k].xvel = -20;
                A_SetSprite(k,CLIPMASK0);
                sprite[k].cstat = 16;

                {
                    int32_t p2 = wall[hit.wall].point2;
                    int32_t a = getangle(wall[hit.wall].x-wall[p2].x, wall[hit.wall].y-wall[p2].y)-512;
                    actor[k].t_data[5] = sprite[k].ang = a;
                }
            }
            return j?k:-1;
        }

        case BOUNCEMINE__STATIC:
        case MORTER__STATIC:
        {
            int32_t x;

            if (s->extra >= 0) s->shade = -96;

            j = g_player[A_FindPlayer(s, NULL)].ps->i;
            x = ldist(&sprite[j],s);

            zvel = -x>>1;

            if (zvel < -4096)
                zvel = -2048;
            vel = x>>4;

            zvel = A_GetShootZvel(zvel);
            A_InsertSprite(sect,
                           srcvect.x+(sintable[(512+sa+512)&2047]>>8),
                           srcvect.y+(sintable[(sa+512)&2047]>>8),
                           srcvect.z+(6<<8),atwith,-64,32,32,sa,vel,zvel,i,1);
            break;
        }

        case SHRINKER__STATIC:
            if (s->extra >= 0) s->shade = -96;
            if (p >= 0)
            {
                j = GetAutoAimAngle(i, p, atwith, 4<<8, 0, &srcvect, 768, &zvel, &sa);

                if (j < 0)
                    zvel = (100-ps->horiz-ps->horizoff)*98;
            }
            else if (s->statnum != STAT_EFFECTOR)
            {
                j = A_FindPlayer(s, NULL);
                l = safeldist(g_player[j].ps->i, s);
                zvel = tabledivide32_noinline((g_player[j].ps->opos.z-srcvect.z)*512, l);
            }
            else zvel = 0;

            zvel = A_GetShootZvel(zvel);
            j = A_InsertSprite(sect,
                               srcvect.x+(sintable[(512+sa+512)&2047]>>12),
                               srcvect.y+(sintable[(sa+512)&2047]>>12),
                               srcvect.z+(2<<8),SHRINKSPARK,-16,28,28,sa,768,zvel,i,4);

            sprite[j].cstat = 128;
            sprite[j].clipdist = 32;

            return j;
        }
    }

    return -1;
}


//////////////////// HUD WEAPON / MISC. DISPLAY CODE ////////////////////

static void P_DisplaySpit(void)
{
    DukePlayer_t *const ps = g_player[screenpeek].ps;
    const int32_t loogcnt = ps->loogcnt;

    if (loogcnt == 0)
        return;

    if (VM_OnEvent(EVENT_DISPLAYSPIT, ps->i, screenpeek) != 0)
        return;

    const int32_t y = loogcnt<<2;

    for (int32_t i=0; i < ps->numloogs; i++)
    {
        int32_t a = klabs(sintable[((loogcnt+i)<<5)&2047])>>5;
        int32_t z = 4096 + ((loogcnt+i)<<9);
        int32_t x = (-g_player[screenpeek].sync->avel>>1) + (sintable[((loogcnt+i)<<6)&2047]>>10);

        rotatesprite_fs(
            (ps->loogiex[i]+x)<<16, (200+ps->loogiey[i]-y)<<16,
            z-(i<<8), 256-a,
            LOOGIE,0,0,2);
    }
}

int32_t P_GetHudPal(const DukePlayer_t *p)
{
    if (sprite[p->i].pal == 1)
        return 1;

    if (p->cursectnum >= 0)
    {
        int32_t dapal = sector[p->cursectnum].floorpal;
        if (!g_noFloorPal[dapal])
            return dapal;
    }

    return 0;
}

static int32_t P_DisplayFist(int32_t gs)
{
    int32_t looking_arc,fisti,fistpal;
    int32_t fistzoom, fistz;

    int32_t wx[2] = { windowx1, windowx2 };

    const DukePlayer_t *const ps = g_player[screenpeek].ps;

    fisti = ps->fist_incs;
    if (fisti > 32) fisti = 32;
    if (fisti <= 0) return 0;

    switch (VM_OnEvent(EVENT_DISPLAYFIST, ps->i, screenpeek))
    {
        case 1:
            return 1;
        case -1:
            return 0;
    }

    looking_arc = klabs(ps->look_ang)/9;

    fistzoom = 65536 - (sintable[(512+(fisti<<6))&2047]<<2);
    fistzoom = clamp(fistzoom, 40920, 90612);

    fistz = 194 + (sintable[((6+fisti)<<7)&2047]>>9);

    fistpal = P_GetHudPal(ps);

#ifdef SPLITSCREEN_MOD_HACKS
    // XXX: this is outdated, doesn't handle above/below split.
    if (g_fakeMultiMode==2)
        wx[(g_snum==0)] = (wx[0]+wx[1])/2+1;
#endif

    rotatesprite(
        (-fisti+222+(g_player[screenpeek].sync->avel>>5))<<16,
        (looking_arc+fistz)<<16,
        fistzoom,0,FIST,gs,fistpal,2,
        wx[0],windowy1,wx[1],windowy2);

    return 1;
}

#define DRAWEAP_CENTER 262144
#define weapsc(sc) scale(sc, ud.weaponscale, 100)

static int32_t g_dts_yadd;

static void G_DrawTileScaled(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p)
{
    int32_t ang = 0;
    int32_t xoff = 192;

    int32_t wx[2] = { windowx1, windowx2 };
    int32_t wy[2] = { windowy1, windowy2 };
    int32_t yofs = 0;

    switch (hudweap.cur)
    {
    case DEVISTATOR_WEAPON:
    case TRIPBOMB_WEAPON:
        xoff = 160;
        break;
    default:
        if (orientation & DRAWEAP_CENTER)
        {
            xoff = 160;
            orientation &= ~DRAWEAP_CENTER;
        }
        break;
    }

    // bit 4 means "flip x" for G_DrawTileScaled
    if (orientation&4)
        ang = 1024;

#ifdef SPLITSCREEN_MOD_HACKS
    if (g_fakeMultiMode==2)
    {
        const int32_t sidebyside = (ud.screen_size!=0);

        // splitscreen HACK
        orientation &= ~(1024|512|256);
        if (sidebyside)
        {
            orientation &= ~8;
            wx[(g_snum==0)] = (wx[0]+wx[1])/2 + 2;
        }
        else
        {
            orientation |= 8;
            if (g_snum==0)
                yofs = -(100<<16);
            wy[(g_snum==0)] = (wy[0]+wy[1])/2 + 2;
        }
    }
#endif

#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST && usemodels && md_tilehasmodel(tilenum,p) >= 0)
        y += (224-weapsc(224));
#endif
    rotatesprite(weapsc(x<<16) + ((xoff-weapsc(xoff))<<16),
                 weapsc((y<<16) + g_dts_yadd) + ((200-weapsc(200))<<16) + yofs,
                 weapsc(65536L),ang,tilenum,shade,p,(2|orientation),
                 wx[0],wy[0], wx[1],wy[1]);
}

static void G_DrawWeaponTile(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p,
                             uint8_t slot)
{
    static int32_t shadef[2] = { 0, 0 }, palf[2] = { 0, 0 };

    // sanity checking the slot value
    if (slot > 1)
        slot = 1;

    // basic fading between player weapon shades
    if (shadef[slot] != shade && (!p || palf[slot] == p))
    {
        shadef[slot] += (shade - shadef[slot]) >> 2;

        if (!((shade - shadef[slot]) >> 2))
            shadef[slot] = logapproach(shadef[slot], shade);
    }
    else
        shadef[slot] = shade;

    palf[slot] = p;

#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST)
        if (tilenum >= CHAINGUN + 1 && tilenum <= CHAINGUN + 4)
            if (!usemodels || md_tilehasmodel(tilenum, p) < 0)
            {
                // HACK: Draw the upper part of the chaingun two screen
                // pixels (not texels; multiplied by weapon scale) lower
                // first, preventing ugly horizontal seam.
                g_dts_yadd = tabledivide32_noinline(65536 * 2 * 200, ydim);
                G_DrawTileScaled(x, y, tilenum, shadef[slot], orientation, p);
                g_dts_yadd = 0;
            }
#endif
    G_DrawTileScaled(x, y, tilenum, shadef[slot], orientation, p);
}

static inline void G_DrawWeaponTileWithID(int32_t id, int32_t x, int32_t y, int32_t tilenum, int32_t shade,
                                          int32_t orientation, int32_t p, uint8_t slot)
{
    int oldid = guniqhudid;

    guniqhudid = id;
    G_DrawWeaponTile(x, y, tilenum, shade, orientation, p, slot);
    guniqhudid = oldid;
}

static int32_t P_DisplayKnee(int32_t gs)
{
    static const int8_t knee_y[] = {0,-8,-16,-32,-64,-84,-108,-108,-108,-72,-32,-8};

    const DukePlayer_t *const ps = g_player[screenpeek].ps;

    if (ps->knee_incs == 0)
        return 0;

    switch (VM_OnEvent(EVENT_DISPLAYKNEE, ps->i, screenpeek))
    {
        case 1:
            return 1;
        case -1:
            return 0;
    }

    if (ps->knee_incs >= ARRAY_SIZE(knee_y) || sprite[ps->i].extra <= 0)
        return 0;

    int32_t looking_arc, pal;

    looking_arc = knee_y[ps->knee_incs] + klabs(ps->look_ang)/9;

    looking_arc -= (ps->hard_landing<<3);

    pal = P_GetHudPal(ps);
    if (pal == 0)
        pal = ps->palookup;

    G_DrawTileScaled(105+(g_player[screenpeek].sync->avel>>5)-(ps->look_ang>>1)+(knee_y[ps->knee_incs]>>2),
                     looking_arc+280-((ps->horiz-ps->horizoff)>>4),KNEE,gs,4+DRAWEAP_CENTER,pal);

    return 1;
}

static int32_t P_DisplayKnuckles(int32_t gs)
{
    static const int8_t knuckle_frames[] = {0,1,2,2,3,3,3,2,2,1,0};
    const DukePlayer_t *const ps = g_player[screenpeek].ps;

    if (ps->knuckle_incs == 0)
        return 0;

    switch (VM_OnEvent(EVENT_DISPLAYKNUCKLES, ps->i, screenpeek))
    {
        case 1:
            return 1;
        case -1:
            return 0;
    }

    if ((unsigned) (ps->knuckle_incs>>1) >= ARRAY_SIZE(knuckle_frames) || sprite[ps->i].extra <= 0)
        return 0;

    int32_t looking_arc, pal;

    looking_arc = klabs(ps->look_ang)/9;

    looking_arc -= (ps->hard_landing<<3);

    pal = P_GetHudPal(ps);

    G_DrawTileScaled(160+(g_player[screenpeek].sync->avel>>5)-(ps->look_ang>>1),
                     looking_arc+180-((ps->horiz-ps->horizoff)>>4),
                     CRACKKNUCKLES+knuckle_frames[ps->knuckle_incs>>1],gs,4+DRAWEAP_CENTER,pal);

    return 1;
}

#if !defined LUNATIC
// Set C-CON's WEAPON and WORKSLIKE gamevars.
void P_SetWeaponGamevars(int32_t snum, const DukePlayer_t *p)
{
    Gv_SetVar(g_iWeaponVarID, p->curr_weapon, p->i, snum);
    Gv_SetVar(g_iWorksLikeVarID,
              ((unsigned)p->curr_weapon < MAX_WEAPONS) ? PWEAPON(snum, p->curr_weapon, WorksLike) : -1,
              p->i, snum);
}
#endif

static void P_FireWeapon(int32_t snum)
{
    int32_t i;
    DukePlayer_t *const p = g_player[snum].ps;

    if (VM_OnEvent(EVENT_DOFIRE, p->i, snum) || p->weapon_pos != 0)
        return;

    if (PWEAPON(snum, p->curr_weapon, WorksLike) != KNEE_WEAPON)
        p->ammo_amount[p->curr_weapon]--;

    if (PWEAPON(snum, p->curr_weapon, FireSound) > 0)
        A_PlaySound(PWEAPON(snum, p->curr_weapon, FireSound), p->i);

    P_SetWeaponGamevars(snum, p);
    //        OSD_Printf("doing %d %d %d\n",PWEAPON(snum, p->curr_weapon, Shoots),p->curr_weapon,snum);
    A_Shoot(p->i, PWEAPON(snum, p->curr_weapon, Shoots));

    for (i = PWEAPON(snum, p->curr_weapon, ShotsPerBurst) - 1; i > 0; i--)
    {
        if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_FIREEVERYOTHER)
        {
            // this makes the projectiles fire on a delay from player code
            actor[p->i].t_data[7] = (PWEAPON(snum, p->curr_weapon, ShotsPerBurst)) << 1;
        }
        else
        {
            if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_AMMOPERSHOT &&
                PWEAPON(snum, p->curr_weapon, WorksLike) != KNEE_WEAPON)
            {
                if (p->ammo_amount[p->curr_weapon] > 0)
                    p->ammo_amount[p->curr_weapon]--;
                else
                    break;
            }

            A_Shoot(p->i, PWEAPON(snum, p->curr_weapon, Shoots));
        }
    }

    if (!(PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_NOVISIBLE))
    {
#ifdef POLYMER
        spritetype *s = &sprite[p->i];
        int32_t x = ((sintable[(s->ang + 512) & 2047]) >> 7), y = ((sintable[(s->ang) & 2047]) >> 7);

        s->x += x;
        s->y += y;
        G_AddGameLight(0, p->i, PHEIGHT, 8192, PWEAPON(snum, p->curr_weapon, FlashColor), PR_LIGHT_PRIO_MAX_GAME);
        actor[p->i].lightcount = 2;
        s->x -= x;
        s->y -= y;
#endif  // POLYMER
        p->visibility = 0;
    }
}

static void P_DoWeaponSpawn(int32_t snum)
{
    int32_t j;
    const DukePlayer_t *const p = g_player[snum].ps;

    // NOTE: For the 'Spawn' member, 0 means 'none', too (originally so,
    // i.e. legacy). The check for <0 was added to the check because mod
    // authors (rightly) assumed that -1 is the no-op value.
    if (PWEAPON(snum, p->curr_weapon, Spawn) <= 0)  // <=0 : AMC TC beta/RC2 has WEAPONx_SPAWN -1
        return;

    j = A_Spawn(p->i, PWEAPON(snum, p->curr_weapon, Spawn));

    if ((PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_SPAWNTYPE3))
    {
        // like chaingun shells
        sprite[j].ang += 1024;
        sprite[j].ang &= 2047;
        sprite[j].xvel += 32;
        sprite[j].z += (3<<8);
    }

    A_SetSprite(j,CLIPMASK0);

}

void P_DisplayScuba(void)
{
    if (g_player[screenpeek].ps->scuba_on)
    {
        const DukePlayer_t *const ps = g_player[screenpeek].ps;

        if (VM_OnEvent(EVENT_DISPLAYSCUBA, ps->i, screenpeek) != 0)
            return;

        int32_t p = P_GetHudPal(ps);

#ifdef SPLITSCREEN_MOD_HACKS
        g_snum = screenpeek;
#endif
#ifdef USE_OPENGL
        if (getrendermode() >= REND_POLYMOST)
            G_DrawTileScaled(44, (200-tilesiz[SCUBAMASK].y), SCUBAMASK, 0, 2+16+DRAWEAP_CENTER, p);
#endif
        G_DrawTileScaled(43, (200-tilesiz[SCUBAMASK].y), SCUBAMASK, 0, 2+16+DRAWEAP_CENTER, p);
        G_DrawTileScaled(320-43, (200-tilesiz[SCUBAMASK].y), SCUBAMASK, 0, 2+4+16+DRAWEAP_CENTER, p);
    }
}

static const int8_t access_tip_y [] ={
    0, -8, -16, -32, -64, -84, -108, -108, -108, -108, -108, -108, -108, -108, -108, -108, -96, -72, -64, -32, -16,
    /* EDuke32: */ 0, 16, 32, 48,
    // At y coord 64, the hand is already not shown.
};

static int32_t P_DisplayTip(int32_t gs)
{
    const DukePlayer_t *const ps = g_player[screenpeek].ps;

    if (ps->tipincs == 0)
        return 0;

    switch (VM_OnEvent(EVENT_DISPLAYTIP, ps->i, screenpeek))
    {
        case 1:
            return 1;
        case -1:
            return 0;
    }

    // Report that the tipping hand has been drawn so that the otherwise
    // selected weapon is not drawn.
    if ((unsigned)ps->tipincs >= ARRAY_SIZE(access_tip_y))
        return 1;

    int y, looking_arc, p = 0;

    looking_arc = (klabs(ps->look_ang) / 9) - (ps->hard_landing << 3);

    p = P_GetHudPal(ps);

    y = access_tip_y[ps->tipincs] >> 1;

    guniqhudid = 201;

    G_DrawTileScaled(170 + (g_player[screenpeek].sync->avel >> 5) - (ps->look_ang >> 1),
                     y + looking_arc + 240 - ((ps->horiz - ps->horizoff) >> 4), TIP + ((26 - ps->tipincs) >> 4), gs,
                     DRAWEAP_CENTER, p);

    guniqhudid = 0;

    return 1;
}

static int32_t P_DisplayAccess(int32_t gs)
{
    const DukePlayer_t *const ps = g_player[screenpeek].ps;

    if (ps->access_incs == 0)
        return 0;

    switch (VM_OnEvent(EVENT_DISPLAYACCESS, ps->i, screenpeek))
    {
        case 1:
            return 1;
        case -1:
            return 0;
    }

    if ((unsigned)ps->access_incs >= ARRAY_SIZE(access_tip_y)-4 || sprite[ps->i].extra <= 0)
        return 1;

    int y, looking_arc, p = 0;

    looking_arc = access_tip_y[ps->access_incs] + (klabs(ps->look_ang) / 9) - (ps->hard_landing << 3);

    if (ps->access_spritenum >= 0)
        p = sprite[ps->access_spritenum].pal;

    y = access_tip_y[ps->access_incs] >> 2;

    guniqhudid = 200;

    if ((ps->access_incs - 3) > 0 && (ps->access_incs - 3) >> 3)
    {
        G_DrawTileScaled(170 + (g_player[screenpeek].sync->avel >> 5) - (ps->look_ang >> 1) + y,
                         looking_arc + 266 - ((ps->horiz - ps->horizoff) >> 4),
                         HANDHOLDINGLASER + (ps->access_incs >> 3), gs, DRAWEAP_CENTER, p);
    }
    else
    {
        G_DrawTileScaled(170 + (g_player[screenpeek].sync->avel >> 5) - (ps->look_ang >> 1) + y,
                         looking_arc + 266 - ((ps->horiz - ps->horizoff) >> 4), HANDHOLDINGACCESS, gs,
                         4 + DRAWEAP_CENTER, p);
    }

    guniqhudid = 0;

    return 1;
}


static int32_t fistsign;

void P_DisplayWeapon(void)
{
    int32_t gun_pos, looking_arc, cw;
    int32_t weapon_xoffset, i, j;
    int32_t o = 0,pal = 0;
    DukePlayer_t *const p = g_player[screenpeek].ps;
    const uint8_t *const kb = &p->kickback_pic;
    int32_t gs;

#ifdef SPLITSCREEN_MOD_HACKS
    g_snum = screenpeek;
#endif

    looking_arc = klabs(p->look_ang)/9;

    gs = sprite[p->i].shade;
    if (gs > 24) gs = 24;

    if (p->newowner >= 0 || ud.camerasprite >= 0 || p->over_shoulder_on > 0 || (sprite[p->i].pal != 1 && sprite[p->i].extra <= 0))
        return;

    if (P_DisplayFist(gs) || P_DisplayKnuckles(gs) || P_DisplayTip(gs) || P_DisplayAccess(gs))
        goto enddisplayweapon;

    P_DisplayKnee(gs);

    gun_pos = 80-(p->weapon_pos*p->weapon_pos);

    weapon_xoffset = (160)-90;

    if (ud.weaponsway)
    {
        weapon_xoffset -= (sintable[((p->weapon_sway>>1)+512)&2047]/(1024+512));

        if (sprite[p->i].xrepeat < 32)
            gun_pos -= klabs(sintable[(p->weapon_sway<<2)&2047]>>9);
        else gun_pos -= klabs(sintable[(p->weapon_sway>>1)&2047]>>10);
    }
    else gun_pos -= 16;

    weapon_xoffset -= 58 + p->weapon_ang;
    gun_pos -= (p->hard_landing<<3);

    cw = PWEAPON(screenpeek, (p->last_weapon >= 0) ? p->last_weapon : p->curr_weapon, WorksLike);

    hudweap.gunposy=gun_pos;
    hudweap.lookhoriz=looking_arc;
    hudweap.cur=cw;
    hudweap.gunposx=weapon_xoffset;
    hudweap.shade=gs;
    hudweap.count=*kb;
    hudweap.lookhalfang=p->look_ang>>1;

    if (VM_OnEvent(EVENT_DISPLAYWEAPON, p->i, screenpeek) == 0)
    {
        j = 14-p->quick_kick;
        if ((j != 14 || p->last_quick_kick) && ud.drawweapon == 1)
        {
            pal = P_GetHudPal(p);
            if (pal == 0)
                pal = p->palookup;

            guniqhudid = 100;
            if (j < 6 || j > 12)
                G_DrawTileScaled(weapon_xoffset+80-(p->look_ang>>1),
                                 looking_arc+250-gun_pos,KNEE,gs,o|4|DRAWEAP_CENTER,pal);
            else G_DrawTileScaled(weapon_xoffset+160-16-(p->look_ang>>1),
                                  looking_arc+214-gun_pos,KNEE+1,gs,o|4|DRAWEAP_CENTER,pal);
            guniqhudid = 0;
        }

        if (sprite[p->i].xrepeat < 40)
        {
            pal = P_GetHudPal(p);

            if (p->jetpack_on == 0)
            {
                i = sprite[p->i].xvel;
                looking_arc += 32-(i>>3);
                fistsign += i>>3;
            }

            cw = weapon_xoffset;
            weapon_xoffset += sintable[(fistsign)&2047]>>10;
            G_DrawTileScaled(weapon_xoffset+250-(p->look_ang>>1),
                             looking_arc+258-(klabs(sintable[(fistsign)&2047]>>8)),
                             FIST,gs,o, pal);
            weapon_xoffset = cw - (sintable[(fistsign)&2047]>>10);
            G_DrawTileScaled(weapon_xoffset+40-(p->look_ang>>1),
                             looking_arc+200+(klabs(sintable[(fistsign)&2047]>>8)),
                             FIST,gs,o|4, pal);
        }
        else
        {
            switch (ud.drawweapon)
            {
                case 1:
                    break;

                case 2:
                    if ((unsigned)hudweap.cur < MAX_WEAPONS && hudweap.cur != KNEE_WEAPON)
                        rotatesprite_win(160 << 16, (180 + (p->weapon_pos * p->weapon_pos)) << 16, scale(65536, ud.statusbarscale, 100), 0,
                                         hudweap.cur == GROW_WEAPON ? GROWSPRITEICON : WeaponPickupSprites[hudweap.cur], 0,
                                         0, 2);
                default:
                    goto enddisplayweapon;
            }

            const int doanim = !(sprite[p->i].pal == 1 || ud.pause_on || g_player[myconnectindex].ps->gm&MODE_MENU);
            const int hla = p->look_ang >> 1;

            pal = P_GetHudPal(p);

            switch (cw)
            {
            case KNEE_WEAPON:
                if (VM_OnEvent(EVENT_DRAWWEAPON, g_player[screenpeek].ps->i, screenpeek) || *kb == 0)
                    break;

                if (pal == 0)
                    pal = p->palookup;

                guniqhudid = cw;
                if (*kb < 5 || *kb > 9)
                    G_DrawTileScaled(weapon_xoffset + 220 - hla, looking_arc + 250 - gun_pos, KNEE,
                                     gs, o, pal);
                else
                    G_DrawTileScaled(weapon_xoffset + 160 - hla, looking_arc + 214 - gun_pos, KNEE + 1,
                                     gs, o, pal);
                guniqhudid = 0;
                break;

            case TRIPBOMB_WEAPON:
                if (VM_OnEvent(EVENT_DRAWWEAPON, g_player[screenpeek].ps->i, screenpeek))
                    break;

                weapon_xoffset += 8;
                gun_pos -= 10;

                if ((*kb) > 6)
                    looking_arc += ((*kb) << 3);
                else if ((*kb) < 4)
                {
                    G_DrawWeaponTileWithID(cw << 2, weapon_xoffset + 142 - hla,
                                           looking_arc + 234 - gun_pos, HANDHOLDINGLASER + 3, gs, o, pal, 0);
                }

                G_DrawWeaponTileWithID(cw, weapon_xoffset + 130 - hla, looking_arc + 249 - gun_pos,
                                       HANDHOLDINGLASER + ((*kb) >> 2), gs, o, pal, 0);

                G_DrawWeaponTileWithID(cw << 1, weapon_xoffset + 152 - hla,
                                       looking_arc + 249 - gun_pos, HANDHOLDINGLASER + ((*kb) >> 2), gs, o | 4,
                                       pal, 0);
                break;

            case RPG_WEAPON:
                if (VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek))
                    break;

                weapon_xoffset -= sintable[(768 + ((*kb) << 7)) & 2047] >> 11;
                gun_pos += sintable[(768 + ((*kb) << 7)) & 2047] >> 11;

                if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING))
                    o |= 512;

                if (*kb > 0 && *kb < 8)
                {
                    G_DrawWeaponTileWithID(cw << 1, weapon_xoffset + 164, (looking_arc << 1) + 176 - gun_pos,
                                           RPGGUN + ((*kb) >> 1), gs, o, pal, 0);
                }

                G_DrawWeaponTileWithID(cw, weapon_xoffset + 164, (looking_arc << 1) + 176 - gun_pos, RPGGUN, gs,
                                       o, pal, 0);
                break;

            case SHOTGUN_WEAPON:
                if (VM_OnEvent(EVENT_DRAWWEAPON, g_player[screenpeek].ps->i, screenpeek))
                    break;

                weapon_xoffset -= 8;

                switch (*kb)
                {
                    case 1:
                    case 2:
                        G_DrawWeaponTileWithID(cw << 1, weapon_xoffset + 168 - hla, looking_arc + 201 - gun_pos,
                                               SHOTGUN + 2, -128, o, pal, 0);
                    case 0:
                    case 6:
                    case 7:
                    case 8:
                        G_DrawWeaponTileWithID(cw, weapon_xoffset + 146 - hla, looking_arc + 202 - gun_pos,
                                               SHOTGUN, gs, o, pal, 0);
                        break;

                    case 3:
                    case 4:
                        gun_pos -= 40;
                        weapon_xoffset += 20;

                        G_DrawWeaponTileWithID(cw << 1, weapon_xoffset + 178 - hla, looking_arc + 194 - gun_pos,
                                               SHOTGUN + 1 + ((*(kb)-1) >> 1), -128, o, pal, 0);
                    case 5:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                        G_DrawWeaponTileWithID(cw, weapon_xoffset + 158 - hla, looking_arc + 220 - gun_pos,
                                               SHOTGUN + 3, gs, o, pal, 0);
                        break;

                    case 13:
                    case 14:
                    case 15:
                        G_DrawWeaponTileWithID(cw, 32 + weapon_xoffset + 166 - hla, looking_arc + 210 - gun_pos,
                                               SHOTGUN + 4, gs, o, pal, 0);
                        break;

                    case 16:
                    case 17:
                    case 18:
                    case 19:
                    case 24:
                    case 25:
                    case 26:
                    case 27:
                        G_DrawWeaponTileWithID(cw, 64 + weapon_xoffset + 170 - hla, looking_arc + 196 - gun_pos,
                                               SHOTGUN + 5, gs, o, pal, 0);
                        break;

                    case 20:
                    case 21:
                    case 22:
                    case 23:
                        G_DrawWeaponTileWithID(cw, 64 + weapon_xoffset + 176 - hla, looking_arc + 196 - gun_pos,
                                               SHOTGUN + 6, gs, o, pal, 0);
                        break;


                    case 28:
                    case 29:
                    case 30:
                        G_DrawWeaponTileWithID(cw, 32 + weapon_xoffset + 156 - hla, looking_arc + 206 - gun_pos,
                                               SHOTGUN + 4, gs, o, pal, 0);
                        break;
                }
                break;

            case CHAINGUN_WEAPON:
                if (VM_OnEvent(EVENT_DRAWWEAPON, g_player[screenpeek].ps->i, screenpeek))
                    break;

                if (*kb > 0)
                {
                    gun_pos -= sintable[(*kb)<<7]>>12;

                    if (doanim)
                        weapon_xoffset += 1-(rand()&3);
                }

                switch (*kb)
                {
                case 0:
                    G_DrawWeaponTileWithID(cw, weapon_xoffset+178-(p->look_ang>>1),looking_arc+233-gun_pos,
                        CHAINGUN+1,gs,o,pal,0);
                    break;

                default:
                    if (*kb > PWEAPON(screenpeek, CHAINGUN_WEAPON, FireDelay) && *kb < PWEAPON(screenpeek, CHAINGUN_WEAPON, TotalTime))
                    {
                        i = 0;
                        if (doanim) i = rand()&7;
                        G_DrawWeaponTileWithID(cw<<2, i+weapon_xoffset-4+140-(p->look_ang>>1),i+looking_arc-((*kb)>>1)+208-gun_pos,
                            CHAINGUN+5+((*kb-4)/5),gs,o,pal,0);
                        if (doanim) i = rand()&7;
                        G_DrawWeaponTileWithID(cw<<2, i+weapon_xoffset-4+184-(p->look_ang>>1),i+looking_arc-((*kb)>>1)+208-gun_pos,
                            CHAINGUN+5+((*kb-4)/5),gs,o,pal,0);
                    }

                    if (*kb < PWEAPON(screenpeek, CHAINGUN_WEAPON, TotalTime)-4)
                    {
                        i = 0;
                        if (doanim) i = rand()&7;
                        G_DrawWeaponTileWithID(cw<<2, i+weapon_xoffset-4+162-(p->look_ang>>1),i+looking_arc-((*kb)>>1)+208-gun_pos,
                            CHAINGUN+5+((*kb-2)/5),gs,o,pal,0);
                        G_DrawWeaponTileWithID(cw, weapon_xoffset+178-(p->look_ang>>1),looking_arc+233-gun_pos,
                            CHAINGUN+1+((*kb)>>1),gs,o,pal,0);
                    }
                    else G_DrawWeaponTileWithID(cw, weapon_xoffset+178-(p->look_ang>>1),looking_arc+233-gun_pos,
                        CHAINGUN+1,gs,o,pal,0);

                    break;
                }

                G_DrawWeaponTileWithID(cw<<1, weapon_xoffset+168-(p->look_ang>>1),looking_arc+260-gun_pos,
                    CHAINGUN,gs,o,pal,0);
                break;

            case PISTOL_WEAPON:
                if (VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek))
                    break;

                if ((*kb) < PWEAPON(screenpeek, PISTOL_WEAPON, TotalTime)+1)
                {
                    static uint8_t kb_frames [] ={ 0, 1, 2 };
                    int32_t l = 195-12+weapon_xoffset;

                    if ((*kb) == PWEAPON(screenpeek, PISTOL_WEAPON, FireDelay))
                        l -= 3;

                    G_DrawWeaponTileWithID(cw, (l-(p->look_ang>>1)), (looking_arc+244-gun_pos), FIRSTGUN+kb_frames[*kb>2 ? 0 : *kb], gs, 2, pal, 0);

                    break;
                }

                if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING))
                    o |= 512;

                if ((*kb) < PWEAPON(screenpeek, PISTOL_WEAPON, Reload)-17)
                    G_DrawWeaponTileWithID(cw, 194-(p->look_ang>>1), looking_arc+230-gun_pos, FIRSTGUN+4, gs, o, pal, 0);
                else if ((*kb) < PWEAPON(screenpeek, PISTOL_WEAPON, Reload)-12)
                {
                    G_DrawWeaponTileWithID(cw<<1, 244-((*kb)<<3)-(p->look_ang>>1), looking_arc+130-gun_pos+((*kb)<<4), FIRSTGUN+6, gs, o, pal, 0);
                    G_DrawWeaponTileWithID(cw, 224-(p->look_ang>>1), looking_arc+220-gun_pos, FIRSTGUN+5, gs, o, pal, 0);
                }
                else if ((*kb) < PWEAPON(screenpeek, PISTOL_WEAPON, Reload)-7)
                {
                    G_DrawWeaponTileWithID(cw<<1, 124+((*kb)<<1)-(p->look_ang>>1), looking_arc+430-gun_pos-((*kb)<<3), FIRSTGUN+6, gs, o, pal, 0);
                    G_DrawWeaponTileWithID(cw, 224-(p->look_ang>>1), looking_arc+220-gun_pos, FIRSTGUN+5, gs, o, pal, 0);
                }

                else if ((*kb) < PWEAPON(screenpeek, PISTOL_WEAPON, Reload)-4)
                {
                    G_DrawWeaponTileWithID(cw<<2, 184-(p->look_ang>>1), looking_arc+235-gun_pos, FIRSTGUN+8, gs, o, pal, 0);
                    G_DrawWeaponTileWithID(cw, 224-(p->look_ang>>1), looking_arc+210-gun_pos, FIRSTGUN+5, gs, o, pal, 0);
                }
                else if ((*kb) < PWEAPON(screenpeek, PISTOL_WEAPON, Reload)-2)
                {
                    G_DrawWeaponTileWithID(cw<<2, 164-(p->look_ang>>1), looking_arc+245-gun_pos, FIRSTGUN+8, gs, o, pal, 0);
                    G_DrawWeaponTileWithID(cw, 224-(p->look_ang>>1), looking_arc+220-gun_pos, FIRSTGUN+5, gs, o, pal, 0);
                }
                else if ((*kb) < PWEAPON(screenpeek, PISTOL_WEAPON, Reload))
                    G_DrawWeaponTileWithID(cw, 194-(p->look_ang>>1), looking_arc+235-gun_pos, FIRSTGUN+5, gs, o, pal, 0);

                break;

            case HANDBOMB_WEAPON:
                if (VM_OnEvent(EVENT_DRAWWEAPON, g_player[screenpeek].ps->i, screenpeek))
                    break;
                else
                {
                    static uint8_t throw_frames [] ={ 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2 };

                    if (*kb >= PWEAPON(screenpeek, p->curr_weapon, TotalTime) || *kb >= ARRAY_SIZE(throw_frames))
                        break;

                    if (*kb)
                    {
                        if ((*kb) < 7)
                            gun_pos -= 10 * (*kb);  // D
                        else if ((*kb) < 12)
                            gun_pos += 20 * ((*kb) - 10);  // U
                        else if ((*kb) < 20)
                            gun_pos -= 9 * ((*kb) - 14);  // D

                        gun_pos += 10;
                    }

                    G_DrawWeaponTileWithID(cw, weapon_xoffset + 190 - hla, looking_arc + 260 - gun_pos,
                                           HANDTHROW + throw_frames[(*kb)], gs, o, pal, 0);
                }
                break;

            case HANDREMOTE_WEAPON:
                if (VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek))
                    break;
                else
                {
                    static uint8_t remote_frames [] ={ 0, 1, 1, 2, 1, 1, 0, 0, 0, 0, 0 };

                    if (*kb >= ARRAY_SIZE(remote_frames))
                        break;

                    weapon_xoffset = -48;
                    G_DrawWeaponTileWithID(cw, weapon_xoffset + 150 - hla, looking_arc + 258 - gun_pos,
                                           HANDREMOTE + remote_frames[(*kb)], gs, o, pal, 0);
                }
                break;

            case DEVISTATOR_WEAPON:
                if (VM_OnEvent(EVENT_DRAWWEAPON, g_player[screenpeek].ps->i, screenpeek))
                    break;

                if ((*kb) < (PWEAPON(screenpeek, DEVISTATOR_WEAPON, TotalTime) + 1) && (*kb) > 0)
                {
                    static uint8_t cycloidy [] ={ 0, 4, 12, 24, 12, 4, 0 };

                    if (*kb >= ARRAY_SIZE(cycloidy))
                        break;

                    i = ksgn((*kb) >> 2);

                    if (p->hbomb_hold_delay)
                    {
                        G_DrawWeaponTileWithID(
                        cw, (cycloidy[*kb] >> 1) + weapon_xoffset + 268 - hla,
                        cycloidy[*kb] + looking_arc + 238 - gun_pos, DEVISTATOR + i, -32, o, pal, 0);
                        G_DrawWeaponTileWithID(cw << 1, weapon_xoffset + 30 - hla,
                                               looking_arc + 240 - gun_pos, DEVISTATOR, gs, o | 4, pal, 0);
                    }
                    else
                    {
                        G_DrawWeaponTileWithID(cw<<1, -(cycloidy[*kb] >> 1) + weapon_xoffset + 30 - hla,
                                         cycloidy[*kb] + looking_arc + 240 - gun_pos, DEVISTATOR + i, -32, o | 4,
                                         pal, 0);
                        G_DrawWeaponTileWithID(cw, weapon_xoffset + 268 - hla, looking_arc + 238 - gun_pos,
                                         DEVISTATOR, gs, o, pal, 0);
                    }
                }
                else
                {
                    G_DrawWeaponTileWithID(cw, weapon_xoffset + 268 - hla, looking_arc + 238 - gun_pos,
                                     DEVISTATOR, gs, o, pal, 0);
                    G_DrawWeaponTileWithID(cw<<1, weapon_xoffset + 30 - hla, looking_arc + 240 - gun_pos,
                                     DEVISTATOR, gs, o | 4, pal, 0);
                }
                break;

            case FREEZE_WEAPON:
                if (VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek))
                    break;

                if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING))
                    o |= 512;

                if ((*kb) < (PWEAPON(screenpeek, p->curr_weapon, TotalTime)+1) && (*kb) > 0)
                {
                    static uint8_t cat_frames[] = { 0,0,1,1,2,2 };

                    if (*kb%6 >= ARRAY_SIZE(cat_frames))
                        break;

                    if (doanim)
                    {
                        weapon_xoffset += rand()&3;
                        looking_arc += rand()&3;
                    }
                    gun_pos -= 16;
                    G_DrawWeaponTileWithID(cw<<1, weapon_xoffset+210-(p->look_ang>>1),looking_arc+261-gun_pos,FREEZE+2,-32,o,pal,0);
                    G_DrawWeaponTileWithID(cw, weapon_xoffset+210-(p->look_ang>>1),looking_arc+235-gun_pos,FREEZE+3+cat_frames[*kb%6],-32,o,pal,0);
                }
                else
                    G_DrawWeaponTileWithID(cw, weapon_xoffset+210-(p->look_ang>>1),looking_arc+261-gun_pos,FREEZE,gs,o,pal,0);
                break;

            case GROW_WEAPON:
            case SHRINKER_WEAPON:
                if (VM_OnEvent(EVENT_DRAWWEAPON, g_player[screenpeek].ps->i, screenpeek))
                    break;

                weapon_xoffset += 28;
                looking_arc += 18;

                if ((*kb) < PWEAPON(screenpeek, p->curr_weapon, TotalTime) && (*kb) > 0)
                {
                    if (doanim)
                    {
                        weapon_xoffset += rand() & 3;
                        gun_pos += (rand() & 3);
                    }

                    G_DrawWeaponTileWithID(cw << 1, weapon_xoffset + 184 - hla, looking_arc + 240 - gun_pos,
                                           SHRINKER + 3 + ((*kb) & 3), -32, o, cw == GROW_WEAPON ? 2 : 0, 1);
                    G_DrawWeaponTileWithID(cw, weapon_xoffset + 188 - hla, looking_arc + 240 - gun_pos,
                                           cw == GROW_WEAPON ? SHRINKER - 1 : SHRINKER + 1, gs, o, pal, 0);
                }
                else
                {
                    G_DrawWeaponTileWithID(cw << 1, weapon_xoffset + 184 - hla, looking_arc + 240 - gun_pos,
                                           SHRINKER + 2, 16 - (sintable[p->random_club_frame & 2047] >> 10), o,
                                           cw == GROW_WEAPON ? 2 : 0, 1);
                    G_DrawWeaponTileWithID(cw, weapon_xoffset + 188 - hla, looking_arc + 240 - gun_pos,
                                           cw == GROW_WEAPON ? SHRINKER - 2 : SHRINKER, gs, o, pal, 0);
                }
                break;
            }
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
#define MAXANGVEL    255
#define MAXHORIZ     127

int32_t g_myAimMode = 0, g_myAimStat = 0, g_oldAimStat = 0;
int32_t mouseyaxismode = -1;
int32_t g_emuJumpTics = 0;

void P_GetInput(int32_t snum)
{
    int32_t j;
    static ControlInfo info[2];
    static int32_t turnheldtime; //MED
    static int32_t lastcontroltime; //MED

    int32_t tics, running;
    int32_t turnamount;
    int32_t keymove;
    DukePlayer_t *p = g_player[snum].ps;
    static input_t in;

    if ((p->gm & (MODE_MENU|MODE_TYPE)) || (ud.pause_on && !KB_KeyPressed(sc_Pause)))
    {
        if (!(p->gm&MODE_MENU))
            CONTROL_GetInput(&info[0]);

        Bmemset(&info[1], 0, sizeof(input_t));
        Bmemset(&loc, 0, sizeof(input_t));
        loc.bits = (((int32_t)g_gameQuit)<<SK_GAMEQUIT);
        loc.extbits = (g_player[snum].pteam != g_player[snum].ps->team)<<6;
        loc.extbits |= (1<<7);

        return;
    }

    if (ud.mouseaiming)
        g_myAimMode = BUTTON(gamefunc_Mouse_Aiming);
    else
    {
        g_oldAimStat = g_myAimStat;
        g_myAimStat = BUTTON(gamefunc_Mouse_Aiming);
        if (g_myAimStat > g_oldAimStat)
        {
            g_myAimMode ^= 1;
            P_DoQuote(QUOTE_MOUSE_AIMING_OFF+g_myAimMode,p);
        }
    }

    j = (g_myAimMode) ? analog_lookingupanddown : ud.config.MouseAnalogueAxes[1];

    if (j != mouseyaxismode)
    {
        CONTROL_MapAnalogAxis(1, j, controldevice_mouse);
        mouseyaxismode = j;
    }

    CONTROL_GetInput(&info[0]);

    if (ud.config.MouseDeadZone)
    {
        if (info[0].dpitch > 0)
        {
            if (info[0].dpitch > ud.config.MouseDeadZone)
                info[0].dpitch -= ud.config.MouseDeadZone;
            else info[0].dpitch = 0;
        }
        else if (info[0].dpitch < 0)
        {
            if (info[0].dpitch < -ud.config.MouseDeadZone)
                info[0].dpitch += ud.config.MouseDeadZone;
            else info[0].dpitch = 0;
        }
        if (info[0].dyaw > 0)
        {
            if (info[0].dyaw > ud.config.MouseDeadZone)
                info[0].dyaw -= ud.config.MouseDeadZone;
            else info[0].dyaw = 0;
        }
        else if (info[0].dyaw < 0)
        {
            if (info[0].dyaw < -ud.config.MouseDeadZone)
                info[0].dyaw += ud.config.MouseDeadZone;
            else info[0].dyaw = 0;
        }
    }

    if (ud.config.MouseBias)
    {
        if (klabs(info[0].dyaw) > klabs(info[0].dpitch))
            info[0].dpitch = tabledivide32_noinline(info[0].dpitch, ud.config.MouseBias);
        else info[0].dyaw = tabledivide32_noinline(info[0].dyaw, ud.config.MouseBias);
    }

    tics = totalclock-lastcontroltime;
    lastcontroltime = totalclock;

    // JBF: Run key behaviour is selectable
    running = (ud.runkey_mode) ? (BUTTON(gamefunc_Run) | ud.auto_run) : (ud.auto_run ^ BUTTON(gamefunc_Run));

    in.svel = in.fvel = in.avel = in.horz = 0;

    if (BUTTON(gamefunc_Strafe))
    {
        in.svel = -(info[0].dyaw+info[1].dyaw)/8;
        info[1].dyaw = (info[1].dyaw+info[0].dyaw) % 8;
    }
    else
    {
        in.avel = (info[0].dyaw+info[1].dyaw)/32;
        info[1].dyaw = (info[1].dyaw+info[0].dyaw) % 32;
    }

    if (ud.mouseflip)
        in.horz = -(info[0].dpitch+info[1].dpitch)/(314-128);
    else in.horz = (info[0].dpitch+info[1].dpitch)/(314-128);

    info[1].dpitch = (info[1].dpitch+info[0].dpitch) % (314-128);

    in.svel -= info[0].dx;
    info[1].dz = info[0].dz % (1<<6);
    in.fvel = -info[0].dz>>6;

//     OSD_Printf("running: %d\n", running);
    if (running)
    {
        turnamount = NORMALTURN<<1;
        keymove = NORMALKEYMOVE<<1;
    }
    else
    {
        turnamount = NORMALTURN;
        keymove = NORMALKEYMOVE;
    }

    if (BUTTON(gamefunc_Strafe))
    {
        if (BUTTON(gamefunc_Turn_Left) && !(g_player[snum].ps->movement_lock&4))
            in.svel -= -keymove;
        if (BUTTON(gamefunc_Turn_Right) && !(g_player[snum].ps->movement_lock&8))
            in.svel -= keymove;
    }
    else
    {
        if (BUTTON(gamefunc_Turn_Left))
        {
            turnheldtime += tics;
            in.avel -= (turnheldtime>=TURBOTURNTIME) ? (turnamount<<1) : (PREAMBLETURN<<1);
        }
        else if (BUTTON(gamefunc_Turn_Right))
        {
            turnheldtime += tics;
            in.avel += (turnheldtime>=TURBOTURNTIME) ? (turnamount<<1) : (PREAMBLETURN<<1);
        }
        else
            turnheldtime=0;
    }

    if (BUTTON(gamefunc_Strafe_Left) && !(g_player[snum].ps->movement_lock&4))
        in.svel += keymove;
    if (BUTTON(gamefunc_Strafe_Right) && !(g_player[snum].ps->movement_lock&8))
        in.svel += -keymove;
    if (BUTTON(gamefunc_Move_Forward) && !(g_player[snum].ps->movement_lock&1))
        in.fvel += keymove;
    if (BUTTON(gamefunc_Move_Backward) && !(g_player[snum].ps->movement_lock&2))
        in.fvel += -keymove;

    in.fvel = clamp(in.fvel, -MAXVEL, MAXVEL);
    in.svel = clamp(in.svel, -MAXSVEL, MAXSVEL);
    in.avel = clamp(in.avel, -MAXANGVEL, MAXANGVEL);
    in.horz = clamp(in.horz, -MAXHORIZ, MAXHORIZ);

    for (j = gamefunc_Weapon_10; j >= gamefunc_Weapon_1; j--)
    {
        if (BUTTON(j))
        {
            j -= (gamefunc_Weapon_1 - 1);
            break;
        }
    }

    if (j == gamefunc_Weapon_1-1)
        j = 0;

    if (BUTTON(gamefunc_Previous_Weapon) || (BUTTON(gamefunc_Dpad_Select) && in.fvel < 0))
        j = 11;
    if (BUTTON(gamefunc_Next_Weapon) || (BUTTON(gamefunc_Dpad_Select) && in.fvel > 0))
        j = 12;

    if (BUTTON(gamefunc_Jump) && p->on_ground)
        g_emuJumpTics = 4;

    loc.bits = (g_emuJumpTics > 0 || BUTTON(gamefunc_Jump))<<SK_JUMP;

    if (g_emuJumpTics > 0)
        g_emuJumpTics--;

    loc.bits |=   BUTTON(gamefunc_Crouch)<<SK_CROUCH;
    loc.bits |=   BUTTON(gamefunc_Fire)<<SK_FIRE;
    loc.bits |= (BUTTON(gamefunc_Aim_Up) || (BUTTON(gamefunc_Dpad_Aiming) && in.fvel > 0))<<SK_AIM_UP;
    loc.bits |= (BUTTON(gamefunc_Aim_Down) || (BUTTON(gamefunc_Dpad_Aiming) && in.fvel < 0))<<SK_AIM_DOWN;
    loc.bits |= ((ud.runkey_mode) ? (ud.auto_run | BUTTON(gamefunc_Run)) : (BUTTON(gamefunc_Run) ^ ud.auto_run))<<SK_RUN;
    loc.bits |=   BUTTON(gamefunc_Look_Left)<<SK_LOOK_LEFT;
    loc.bits |=   BUTTON(gamefunc_Look_Right)<<SK_LOOK_RIGHT;
    loc.bits |=   j<<SK_WEAPON_BITS;
    loc.bits |=   BUTTON(gamefunc_Steroids)<<SK_STEROIDS;
    loc.bits |=   BUTTON(gamefunc_Look_Up)<<SK_LOOK_UP;
    loc.bits |=   BUTTON(gamefunc_Look_Down)<<SK_LOOK_DOWN;
    loc.bits |=   BUTTON(gamefunc_NightVision)<<SK_NIGHTVISION;
    loc.bits |=   BUTTON(gamefunc_MedKit)<<SK_MEDKIT;
    loc.bits |=   BUTTON(gamefunc_Center_View)<<SK_CENTER_VIEW;
    loc.bits |=   BUTTON(gamefunc_Holster_Weapon)<<SK_HOLSTER;
    loc.bits |= (BUTTON(gamefunc_Inventory_Left) || (BUTTON(gamefunc_Dpad_Select) && (in.svel > 0 || in.avel < 0))) <<SK_INV_LEFT;
    loc.bits |=   KB_KeyPressed(sc_Pause)<<SK_PAUSE;
    loc.bits |=   BUTTON(gamefunc_Quick_Kick)<<SK_QUICK_KICK;
    loc.bits |=   g_myAimMode<<SK_AIMMODE;
    loc.bits |=   BUTTON(gamefunc_Holo_Duke)<<SK_HOLODUKE;
    loc.bits |=   BUTTON(gamefunc_Jetpack)<<SK_JETPACK;
    loc.bits |= (g_gameQuit<<SK_GAMEQUIT);
    loc.bits |= (BUTTON(gamefunc_Inventory_Right) || (BUTTON(gamefunc_Dpad_Select) && (in.svel < 0 || in.avel > 0))) <<SK_INV_RIGHT;
    loc.bits |=   BUTTON(gamefunc_TurnAround)<<SK_TURNAROUND;
    loc.bits |=   BUTTON(gamefunc_Open)<<SK_OPEN;
    loc.bits |=   BUTTON(gamefunc_Inventory)<<SK_INVENTORY;
    loc.bits |=   ((uint32_t)KB_KeyPressed(sc_Escape))<<SK_ESCAPE;

    if (BUTTON(gamefunc_Dpad_Select))
        in.fvel = in.svel = in.avel = 0;

    if (BUTTON(gamefunc_Dpad_Aiming))
        in.fvel = 0;

    if (PWEAPON(snum, g_player[snum].ps->curr_weapon, Flags) & WEAPON_SEMIAUTO && BUTTON(gamefunc_Fire))
        CONTROL_ClearButton(gamefunc_Fire);

    loc.extbits = (BUTTON(gamefunc_Move_Forward) || (in.fvel > 0));
    loc.extbits |= (BUTTON(gamefunc_Move_Backward) || (in.fvel < 0))<<1;
    loc.extbits |= (BUTTON(gamefunc_Strafe_Left) || (in.svel > 0))<<2;
    loc.extbits |= (BUTTON(gamefunc_Strafe_Right) || (in.svel < 0))<<3;

    if (VM_HaveEvent(EVENT_PROCESSINPUT) || VM_HaveEvent(EVENT_TURNLEFT))
        loc.extbits |= BUTTON(gamefunc_Turn_Left)<<4;

    if (VM_HaveEvent(EVENT_PROCESSINPUT) || VM_HaveEvent(EVENT_TURNRIGHT))
        loc.extbits |= BUTTON(gamefunc_Turn_Right)<<5;

    // used for changing team
    loc.extbits |= (g_player[snum].pteam != g_player[snum].ps->team)<<6;

    if (ud.scrollmode && ud.overhead_on)
    {
        ud.folfvel = in.fvel;
        ud.folavel = in.avel;
        loc.fvel = loc.svel = loc.avel = loc.horz = 0;
        return;
    }

    loc.fvel =
    mulscale9(in.fvel, sintable[(p->ang + 2560) & 2047]) + (mulscale9(in.svel, sintable[(p->ang + 2048) & 2047]));
    loc.svel =
    mulscale9(in.fvel, sintable[(p->ang + 2048) & 2047]) + (mulscale9(in.svel, sintable[(p->ang + 1536) & 2047]));

    loc.avel = in.avel;
    loc.horz = in.horz;
}

static int32_t P_DoCounters(int32_t snum)
{
    DukePlayer_t *const p = g_player[snum].ps;

//        j = g_player[snum].sync->avel;
//        p->weapon_ang = -(j/5);

    if (p->invdisptime > 0)
        p->invdisptime--;

    if (p->tipincs > 0)
        p->tipincs--;

    if (p->last_pissed_time > 0)
    {
        switch (--p->last_pissed_time)
        {
        case GAMETICSPERSEC*219:
            {
                A_PlaySound(FLUSH_TOILET,p->i);
                if (snum == screenpeek || GTFLAGS(GAMETYPE_COOPSOUND))
                    A_PlaySound(DUKE_PISSRELIEF,p->i);
            }
            break;
        case GAMETICSPERSEC*218:
            {
                p->holster_weapon = 0;
                p->weapon_pos = WEAPON_POS_RAISE;
            }
            break;
        }
    }

    if (p->crack_time > 0)
    {
        if (--p->crack_time == 0)
        {
            p->knuckle_incs = 1;
            p->crack_time = 777;
        }
    }

    if (p->inv_amount[GET_STEROIDS] > 0 && p->inv_amount[GET_STEROIDS] < 400)
    {
        if (--p->inv_amount[GET_STEROIDS] == 0)
            P_SelectNextInvItem(p);

        if (!(p->inv_amount[GET_STEROIDS]&7))
            if (snum == screenpeek || GTFLAGS(GAMETYPE_COOPSOUND))
                A_PlaySound(DUKE_HARTBEAT,p->i);
    }

    if (p->heat_on && p->inv_amount[GET_HEATS] > 0)
    {
        if (--p->inv_amount[GET_HEATS] == 0)
        {
            p->heat_on = 0;
            P_SelectNextInvItem(p);
            A_PlaySound(NITEVISION_ONOFF,p->i);
            P_UpdateScreenPal(p);
        }
    }

    if (p->holoduke_on >= 0)
    {
        if (--p->inv_amount[GET_HOLODUKE] <= 0)
        {
            A_PlaySound(TELEPORTER,p->i);
            p->holoduke_on = -1;
            P_SelectNextInvItem(p);
        }
    }

    if (p->jetpack_on && p->inv_amount[GET_JETPACK] > 0)
    {
        if (--p->inv_amount[GET_JETPACK] <= 0)
        {
            p->jetpack_on = 0;
            P_SelectNextInvItem(p);
            A_PlaySound(DUKE_JETPACK_OFF,p->i);
            S_StopEnvSound(DUKE_JETPACK_IDLE,p->i);
            S_StopEnvSound(DUKE_JETPACK_ON,p->i);
        }
    }

    if (p->quick_kick > 0 && sprite[p->i].pal != 1)
    {
        p->last_quick_kick = p->quick_kick+1;

        if (--p->quick_kick == 8)
            A_Shoot(p->i,KNEE);
    }
    else if (p->last_quick_kick > 0) p->last_quick_kick--;

    if (p->access_incs && sprite[p->i].pal != 1)
    {
        p->access_incs++;
        if (sprite[p->i].extra <= 0)
            p->access_incs = 12;

        if (p->access_incs == 12)
        {
            if (p->access_spritenum >= 0)
            {
                P_ActivateSwitch(snum,p->access_spritenum,1);
                switch (sprite[p->access_spritenum].pal)
                {
                case 0:
                    p->got_access &= (0xffff-0x1);
                    break;
                case 21:
                    p->got_access &= (0xffff-0x2);
                    break;
                case 23:
                    p->got_access &= (0xffff-0x4);
                    break;
                }
                p->access_spritenum = -1;
            }
            else
            {
                P_ActivateSwitch(snum,p->access_wallnum,0);
                switch (wall[p->access_wallnum].pal)
                {
                case 0:
                    p->got_access &= (0xffff-0x1);
                    break;
                case 21:
                    p->got_access &= (0xffff-0x2);
                    break;
                case 23:
                    p->got_access &= (0xffff-0x4);
                    break;
                }
            }
        }

        if (p->access_incs > 20)
        {
            p->access_incs = 0;
            p->weapon_pos = WEAPON_POS_RAISE;
            p->kickback_pic = 0;
        }
    }

    if (p->cursectnum >= 0 && p->scuba_on == 0 && sector[p->cursectnum].lotag == ST_2_UNDERWATER)
    {
        if (p->inv_amount[GET_SCUBA] > 0)
        {
            p->scuba_on = 1;
            p->inven_icon = ICON_SCUBA;
            P_DoQuote(QUOTE_SCUBA_ON,p);
        }
        else
        {
            if (p->airleft > 0)
                p->airleft--;
            else
            {
                p->extra_extra8 += 32;
                if (p->last_extra < (p->max_player_health>>1) && (p->last_extra&3) == 0)
                    A_PlaySound(DUKE_LONGTERM_PAIN,p->i);
            }
        }
    }
    else if (p->inv_amount[GET_SCUBA] > 0 && p->scuba_on)
    {
        p->inv_amount[GET_SCUBA]--;
        if (p->inv_amount[GET_SCUBA] == 0)
        {
            p->scuba_on = 0;
            P_SelectNextInvItem(p);
        }
    }

    if (p->knuckle_incs)
    {
        if (++p->knuckle_incs == 10)
        {
            if (totalclock > 1024)
                if (snum == screenpeek || GTFLAGS(GAMETYPE_COOPSOUND))
                {

                    if (rand()&1)
                        A_PlaySound(DUKE_CRACK,p->i);
                    else A_PlaySound(DUKE_CRACK2,p->i);

                }

            A_PlaySound(DUKE_CRACK_FIRST,p->i);

        }
        else if (p->knuckle_incs == 22 || TEST_SYNC_KEY(g_player[snum].sync->bits, SK_FIRE))
            p->knuckle_incs=0;

        return 1;
    }
    return 0;
}

int16_t WeaponPickupSprites[MAX_WEAPONS] = { KNEE__STATIC, FIRSTGUNSPRITE__STATIC, SHOTGUNSPRITE__STATIC,
        CHAINGUNSPRITE__STATIC, RPGSPRITE__STATIC, HEAVYHBOMB__STATIC, SHRINKERSPRITE__STATIC, DEVISTATORSPRITE__STATIC,
        TRIPBOMBSPRITE__STATIC, FREEZESPRITE__STATIC, HEAVYHBOMB__STATIC, SHRINKERSPRITE__STATIC
                                           };
// this is used for player deaths
void P_DropWeapon(int32_t snum)
{
    const DukePlayer_t *const p = g_player[snum].ps;
    int32_t cw = PWEAPON(snum, p->curr_weapon, WorksLike);

    if ((unsigned)cw >= MAX_WEAPONS)
        return;
      
    if (krand()&1)
        A_Spawn(p->i, WeaponPickupSprites[cw]);
    else switch (cw)
        {
        case RPG_WEAPON:
        case HANDBOMB_WEAPON:
            A_Spawn(p->i, EXPLOSION2);
            break;
        }
}

void P_AddAmmo(int32_t weapon,DukePlayer_t *p,int32_t amount)
{
    p->ammo_amount[weapon] += amount;

    if (p->ammo_amount[weapon] > p->max_ammo_amount[weapon])
        p->ammo_amount[weapon] = p->max_ammo_amount[weapon];
}

static void P_AddWeaponNoSwitch(DukePlayer_t *p, int32_t weapon)
{
    int32_t snum = P_Get(p->i);  // PASS_SNUM?

    if ((p->gotweapon & (1<<weapon)) == 0)
    {
        p->gotweapon |= (1<<weapon);

        if (weapon == SHRINKER_WEAPON)
            p->gotweapon |= (1<<GROW_WEAPON);
    }

    if (PWEAPON(snum, p->curr_weapon, SelectSound) > 0)
        S_StopEnvSound(PWEAPON(snum, p->curr_weapon, SelectSound),p->i);

    if (PWEAPON(snum, weapon, SelectSound) > 0)
        A_PlaySound(PWEAPON(snum, weapon, SelectSound),p->i);
}

static void P_ChangeWeapon(DukePlayer_t *p, int32_t weapon)
{
    int32_t i = 0, snum = P_Get(p->i);  // PASS_SNUM?
    const int8_t curr_weapon = p->curr_weapon;

    if (p->reloading)
        return;

    if (p->curr_weapon != weapon && VM_HaveEvent(EVENT_CHANGEWEAPON))
        i = VM_OnEventWithReturn(EVENT_CHANGEWEAPON,p->i, snum, weapon);

    if (i == -1)
        return;

    if (i != -2)
        p->curr_weapon = weapon;


    p->random_club_frame = 0;

    if (p->weapon_pos == 0)
    {
        p->weapon_pos = -1;
        p->last_weapon = curr_weapon;
    }
    else if ((unsigned)p->weapon_pos < WEAPON_POS_RAISE)
    {
        p->weapon_pos = -p->weapon_pos;
        p->last_weapon = curr_weapon;
    }
    else if (p->last_weapon == weapon)
    {
        p->last_weapon = -1;
        p->weapon_pos = -p->weapon_pos;
    }

    if (p->holster_weapon)
    {
        p->weapon_pos = WEAPON_POS_RAISE;
        p->holster_weapon = 0;
        p->last_weapon = -1;
    }

#ifdef __ANDROID__
    if (curr_weapon != p->curr_weapon &&
//        p->last_weapon != -1 &&
        !(PWEAPON(snum, curr_weapon, WorksLike) == HANDREMOTE_WEAPON && PWEAPON(snum, p->curr_weapon, WorksLike) == HANDBOMB_WEAPON) &&
        !(PWEAPON(snum, curr_weapon, WorksLike) == HANDBOMB_WEAPON && PWEAPON(snum, p->curr_weapon, WorksLike) == HANDREMOTE_WEAPON))
        CONTROL_Android_SetLastWeapon(PWEAPON(snum, curr_weapon, WorksLike) == HANDREMOTE_WEAPON ? (int)HANDBOMB_WEAPON : curr_weapon);
#endif

    p->kickback_pic = 0;

    P_SetWeaponGamevars(snum, p);
}

void P_AddWeapon(DukePlayer_t *p, int32_t weapon, int32_t doswitch)
{
    P_AddWeaponNoSwitch(p, weapon);
    if (doswitch)
        P_ChangeWeapon(p, weapon);
}

void P_SelectNextInvItem(DukePlayer_t *p)
{
    if (p->inv_amount[GET_FIRSTAID] > 0)
        p->inven_icon = ICON_FIRSTAID;
    else if (p->inv_amount[GET_STEROIDS] > 0)
        p->inven_icon = ICON_STEROIDS;
    else if (p->inv_amount[GET_JETPACK] > 0)
        p->inven_icon = ICON_JETPACK;
    else if (p->inv_amount[GET_HOLODUKE] > 0)
        p->inven_icon = ICON_HOLODUKE;
    else if (p->inv_amount[GET_HEATS] > 0)
        p->inven_icon = ICON_HEATS;
    else if (p->inv_amount[GET_SCUBA] > 0)
        p->inven_icon = ICON_SCUBA;
    else if (p->inv_amount[GET_BOOTS] > 0)
        p->inven_icon = ICON_BOOTS;
    else p->inven_icon = ICON_NONE;
}

void P_CheckWeapon(DukePlayer_t *p)
{
    int32_t i, snum, weapon;

    if (p->reloading)
        return;

    if (p->wantweaponfire >= 0)
    {
        weapon = p->wantweaponfire;
        p->wantweaponfire = -1;

        if (weapon == p->curr_weapon)
            return;

        if ((p->gotweapon & (1<<weapon)) && p->ammo_amount[weapon] > 0)
        {
            P_AddWeapon(p, weapon, 1);
            return;
        }
    }

    weapon = p->curr_weapon;

    if ((p->gotweapon & (1<<weapon)) && (p->ammo_amount[weapon] > 0 || !(p->weaponswitch & 2)))
        return;

    snum = P_Get(p->i);

    for (i=0; i<=FREEZE_WEAPON; i++)
    {
        weapon = g_player[snum].wchoice[i];
        if (VOLUMEONE && weapon > SHRINKER_WEAPON)
            continue;

        if (weapon == KNEE_WEAPON)
            weapon = FREEZE_WEAPON;
        else weapon--;

        if (weapon == KNEE_WEAPON || ((p->gotweapon & (1<<weapon)) && p->ammo_amount[weapon] > 0))
            break;
    }

    if (i == HANDREMOTE_WEAPON)
        weapon = KNEE_WEAPON;

    // Found the weapon

    P_ChangeWeapon(p, weapon);
}

#ifdef LUNATIC
void P_CheckWeaponI(int32_t snum)
{
    P_CheckWeapon(g_player[snum].ps);
}
#endif

static void DoWallTouchDamage(const DukePlayer_t *p, int32_t obj)
{
    vec3_t davect;

    davect.x = p->pos.x + (sintable[(p->ang+512)&2047]>>9);
    davect.y = p->pos.y + (sintable[p->ang&2047]>>9);
    davect.z = p->pos.z;

    A_DamageWall(p->i, obj, &davect, -1);
}

static void P_CheckTouchDamage(DukePlayer_t *p, int32_t obj)
{
    if ((obj = VM_OnEventWithReturn(EVENT_CHECKTOUCHDAMAGE, p->i, P_Get(p->i), obj)) == -1)
        return;

    if ((obj&49152) == 49152)
    {
        obj &= MAXSPRITES-1;

        if (sprite[obj].picnum == CACTUS)
        {
            if (p->hurt_delay < 8)
            {
                sprite[p->i].extra -= 5;

                p->hurt_delay = 16;
                P_PalFrom(p, 32, 32,0,0);
                A_PlaySound(DUKE_LONGTERM_PAIN, p->i);
            }
        }
        return;
    }

    if ((obj&49152) != 32768)
        return;

    obj &= (MAXWALLS-1);

    if (p->hurt_delay > 0)
    {
        p->hurt_delay--;
    }
    else if (wall[obj].cstat & FORCEFIELD_CSTAT)
    {
        int32_t switchpicnum = G_GetForcefieldPicnum(obj);

        switch (DYNAMICTILEMAP(switchpicnum))
        {
        case W_FORCEFIELD__STATIC:
            sprite[p->i].extra -= 5;

            p->hurt_delay = 16;
            P_PalFrom(p, 32, 32,0,0);

            p->vel.x = -(sintable[(p->ang+512)&2047]<<8);
            p->vel.y = -(sintable[(p->ang)&2047]<<8);
            A_PlaySound(DUKE_LONGTERM_PAIN,p->i);

            DoWallTouchDamage(p, obj);
            break;

        case BIGFORCE__STATIC:
            p->hurt_delay = GAMETICSPERSEC;
            DoWallTouchDamage(p, obj);
            break;
        }
    }
}

static int32_t P_CheckFloorDamage(DukePlayer_t *p, int32_t tex)
{
    spritetype *s = &sprite[p->i];

    if ((unsigned)(tex = VM_OnEventWithReturn(EVENT_CHECKFLOORDAMAGE, p->i, P_Get(p->i), tex)) >= MAXTILES)
        return 0;

    switch (DYNAMICTILEMAP(tex))
    {
    case HURTRAIL__STATIC:
        if (rnd(32))
        {
            if (p->inv_amount[GET_BOOTS] > 0)
                return 1;
            else
            {
                if (!A_CheckSoundPlaying(p->i,DUKE_LONGTERM_PAIN))
                    A_PlaySound(DUKE_LONGTERM_PAIN,p->i);

                P_PalFrom(p, 32, 64,64,64);

                s->extra -= 1+(krand()&3);
                if (!A_CheckSoundPlaying(p->i,SHORT_CIRCUIT))
                    A_PlaySound(SHORT_CIRCUIT,p->i);

                return 0;
            }
        }
        break;
    case FLOORSLIME__STATIC:
        if (rnd(16))
        {
            if (p->inv_amount[GET_BOOTS] > 0)
                return 1;
            else
            {
                if (!A_CheckSoundPlaying(p->i,DUKE_LONGTERM_PAIN))
                    A_PlaySound(DUKE_LONGTERM_PAIN,p->i);

                P_PalFrom(p, 32, 0,8,0);
                s->extra -= 1+(krand()&3);

                return 0;
            }
        }
        break;
    case FLOORPLASMA__STATIC:
        if (rnd(32))
        {
            if (p->inv_amount[GET_BOOTS] > 0)
                return 1;
            else
            {
                if (!A_CheckSoundPlaying(p->i,DUKE_LONGTERM_PAIN))
                    A_PlaySound(DUKE_LONGTERM_PAIN,p->i);

                P_PalFrom(p, 32, 8,0,0);
                s->extra -= 1+(krand()&3);

                return 0;
            }
        }
        break;
    }

    return 0;
}


int32_t P_FindOtherPlayer(int32_t p, int32_t *d)
{
    int32_t j, closest_player = p;
    int32_t x, closest = INT32_MAX;

    for (TRAVERSE_CONNECT(j))
        if (p != j && sprite[g_player[j].ps->i].extra > 0)
        {
            x = klabs(g_player[j].ps->opos.x-g_player[p].ps->pos.x) +
                klabs(g_player[j].ps->opos.y-g_player[p].ps->pos.y) +
                (klabs(g_player[j].ps->opos.z-g_player[p].ps->pos.z)>>4);

            if (x < closest)
            {
                closest_player = j;
                closest = x;
            }
        }

    *d = closest;
    return closest_player;
}

void P_FragPlayer(int32_t snum)
{
    DukePlayer_t *p = g_player[snum].ps;
    spritetype *s = &sprite[p->i];

    if (g_netServer || g_netClient)
        randomseed = ticrandomseed;

    if (s->pal != 1)
    {
        P_PalFrom(p, 63, 63,0,0);

        p->pos.z -= (16<<8);
        s->z -= (16<<8);

        p->dead_flag = (512-((krand()&1)<<10)+(krand()&255)-512)&2047;
        if (p->dead_flag == 0)
            p->dead_flag++;
#ifndef NETCODE_DISABLE
        if (g_netServer)
        {
            packbuf[0] = PACKET_FRAG;
            packbuf[1] = snum;
            packbuf[2] = p->frag_ps;
            packbuf[3] = actor[p->i].picnum;
            B_BUF32(&packbuf[4], ticrandomseed);
            packbuf[8] = myconnectindex;

            enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, 9, ENET_PACKET_FLAG_RELIABLE));
        }
#endif
    }

    p->jetpack_on = 0;
    p->holoduke_on = -1;

    S_StopEnvSound(DUKE_JETPACK_IDLE,p->i);
    if (p->scream_voice > FX_Ok)
    {
        FX_StopSound(p->scream_voice);
        S_Cleanup();
        //                S_TestSoundCallback(DUKE_SCREAM);
        p->scream_voice = -1;
    }

    if (s->pal != 1 && (s->cstat&32768) == 0) s->cstat = 0;

    if ((g_netServer || ud.multimode > 1) && (s->pal != 1 || (s->cstat&32768)))
    {
        if (p->frag_ps != snum)
        {
            if (GTFLAGS(GAMETYPE_TDM) && g_player[p->frag_ps].ps->team == g_player[snum].ps->team)
                g_player[p->frag_ps].ps->fraggedself++;
            else
            {
                g_player[p->frag_ps].ps->frag++;
                g_player[p->frag_ps].frags[snum]++;
                g_player[snum].frags[snum]++; // deaths
            }

            if (snum == screenpeek)
            {
                Bsprintf(ScriptQuotes[QUOTE_RESERVED],"Killed by %s",&g_player[p->frag_ps].user_name[0]);
                P_DoQuote(QUOTE_RESERVED,p);
            }
            else
            {
                Bsprintf(ScriptQuotes[QUOTE_RESERVED2],"Killed %s",&g_player[snum].user_name[0]);
                P_DoQuote(QUOTE_RESERVED2,g_player[p->frag_ps].ps);
            }

            if (ud.obituaries)
            {
                Bsprintf(tempbuf,ScriptQuotes[OBITQUOTEINDEX+(krand()%g_numObituaries)],
                         &g_player[p->frag_ps].user_name[0],
                         &g_player[snum].user_name[0]);
                G_AddUserQuote(tempbuf);
            }
            else krand();
        }
        else
        {
            if (actor[p->i].picnum != APLAYERTOP)
            {
                p->fraggedself++;
                if ((unsigned)p->wackedbyactor < MAXTILES && A_CheckEnemyTile(sprite[p->wackedbyactor].picnum))
                    Bsprintf(tempbuf,ScriptQuotes[OBITQUOTEINDEX+(krand()%g_numObituaries)],"A monster",&g_player[snum].user_name[0]);
                else if (actor[p->i].picnum == NUKEBUTTON)
                    Bsprintf(tempbuf,"^02%s^02 tried to leave",&g_player[snum].user_name[0]);
                else
                {
                    // random suicide death string
                    Bsprintf(tempbuf,ScriptQuotes[SUICIDEQUOTEINDEX+(krand()%g_numSelfObituaries)],&g_player[snum].user_name[0]);
                }
            }
            else Bsprintf(tempbuf,"^02%s^02 switched to team %d",&g_player[snum].user_name[0],p->team+1);

            if (ud.obituaries)
                G_AddUserQuote(tempbuf);
        }
        p->frag_ps = snum;
        pus = NUMPAGES;
    }
}

#ifdef LUNATIC
# define PIPEBOMB_CONTROL(snum) (g_player[snum].ps->pipebombControl)
#else
# define PIPEBOMB_CONTROL(snum) (Gv_GetVarByLabel("PIPEBOMB_CONTROL", PIPEBOMB_REMOTE, -1, snum))
#endif

static void P_ProcessWeapon(int32_t snum)
{
    DukePlayer_t *const p = g_player[snum].ps;
    uint8_t *const kb = &p->kickback_pic;
    const int32_t shrunk = (sprite[p->i].yrepeat < 32);
    uint32_t sb_snum = g_player[snum].sync->bits;
    int32_t i, j, k;

    switch (p->weapon_pos)
    {
    case WEAPON_POS_LOWER:
        if (p->last_weapon >= 0)
        {
            p->weapon_pos = WEAPON_POS_RAISE;
            p->last_weapon = -1;
        }
        else if (p->holster_weapon == 0)
            p->weapon_pos = WEAPON_POS_RAISE;
        break;
    case 0:
        break;
    default:
        p->weapon_pos--;
        break;
    }

    if (TEST_SYNC_KEY(sb_snum, SK_FIRE))
    {
        P_SetWeaponGamevars(snum, p);
        
        if (VM_OnEvent(EVENT_PRESSEDFIRE, p->i, snum) != 0)
            sb_snum &= ~BIT(SK_FIRE);
    }

    if (TEST_SYNC_KEY(sb_snum, SK_HOLSTER))   // 'Holster Weapon
    {
        P_SetWeaponGamevars(snum, p);
        
        if (VM_OnEvent(EVENT_HOLSTER, p->i, snum) == 0)
        {
            if (PWEAPON(snum, p->curr_weapon, WorksLike) != KNEE_WEAPON)
            {
                if (p->holster_weapon == 0 && p->weapon_pos == 0)
                {
                    p->holster_weapon = 1;
                    p->weapon_pos = -1;
                    P_DoQuote(QUOTE_WEAPON_LOWERED,p);
                }
                else if (p->holster_weapon == 1 && p->weapon_pos == WEAPON_POS_LOWER)
                {
                    p->holster_weapon = 0;
                    p->weapon_pos = WEAPON_POS_RAISE;
                    P_DoQuote(QUOTE_WEAPON_RAISED,p);
                }
            }

            if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_HOLSTER_CLEARS_CLIP)
            {
                const int32_t cw=p->curr_weapon, clipcnt = PWEAPON(snum, cw, Clip);

                if (p->ammo_amount[cw] > clipcnt && (p->ammo_amount[cw] % clipcnt) != 0)
                {
                    p->ammo_amount[cw] -= p->ammo_amount[cw] % clipcnt;
                    (*kb) = PWEAPON(snum, cw, TotalTime);
                    sb_snum &= ~BIT(SK_FIRE); // not firing...
                }

                return;
            }
        }
    }

    if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_GLOWS)
    {
        p->random_club_frame += 64; // Glowing

        if (p->kickback_pic == 0)
        {
            spritetype *s = &sprite[p->i];
            int32_t x = ((sintable[(s->ang+512)&2047])>>7), y = ((sintable[(s->ang)&2047])>>7);
            int32_t r = 1024+(sintable[p->random_club_frame&2047]>>3);

            s->x += x;
            s->y += y;
            G_AddGameLight(0, p->i, PHEIGHT, max(r, 0), PWEAPON(snum, p->curr_weapon, FlashColor),PR_LIGHT_PRIO_HIGH_GAME);
            actor[p->i].lightcount = 2;
            s->x -= x;
            s->y -= y;
        }

    }

    // this is a hack for WEAPON_FIREEVERYOTHER
    if (actor[p->i].t_data[7])
    {
        actor[p->i].t_data[7]--;
        if (p->last_weapon == -1 && actor[p->i].t_data[7] != 0 && ((actor[p->i].t_data[7] & 1) == 0))
        {
            if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_AMMOPERSHOT)
            {
                if (p->ammo_amount[p->curr_weapon] > 0)
                    p->ammo_amount[p->curr_weapon]--;
                else
                {
                    actor[p->i].t_data[7] = 0;
                    P_CheckWeapon(p);
                }
            }

            if (actor[p->i].t_data[7] != 0)
                A_Shoot(p->i,PWEAPON(snum, p->curr_weapon, Shoots));
        }
    }

    if (p->rapid_fire_hold == 1)
    {
        if (TEST_SYNC_KEY(sb_snum, SK_FIRE)) return;
        p->rapid_fire_hold = 0;
    }

    if (shrunk || p->tipincs || p->access_incs)
        sb_snum &= ~BIT(SK_FIRE);
    else if (shrunk == 0 && (sb_snum&(1<<2)) && (*kb) == 0 && p->fist_incs == 0 &&
             p->last_weapon == -1 && (p->weapon_pos == 0 || p->holster_weapon == 1))
    {
        p->crack_time = 777;

        if (p->holster_weapon == 1)
        {
            if (p->last_pissed_time <= (GAMETICSPERSEC*218) && p->weapon_pos == WEAPON_POS_LOWER)
            {
                p->holster_weapon = 0;
                p->weapon_pos = WEAPON_POS_RAISE;
                P_DoQuote(QUOTE_WEAPON_RAISED,p);
            }
        }
        else
        {
            P_SetWeaponGamevars(snum, p);

            if (VM_OnEvent(EVENT_FIRE, p->i, snum) == 0)
            {
                // this event is deprecated
                VM_OnEvent(EVENT_FIREWEAPON, p->i, snum);

                switch (PWEAPON(snum, p->curr_weapon, WorksLike))
                {
                case HANDBOMB_WEAPON:
                    p->hbomb_hold_delay = 0;
                    if (p->ammo_amount[p->curr_weapon] > 0)
                    {
                        (*kb)=1;
                        if (PWEAPON(snum, p->curr_weapon, InitialSound) > 0)
                            A_PlaySound(PWEAPON(snum, p->curr_weapon, InitialSound), p->i);
                    }
                    break;

                case HANDREMOTE_WEAPON:
                    p->hbomb_hold_delay = 0;
                    (*kb) = 1;
                    if (PWEAPON(snum, p->curr_weapon, InitialSound) > 0)
                        A_PlaySound(PWEAPON(snum, p->curr_weapon, InitialSound), p->i);
                    break;

                case SHOTGUN_WEAPON:
                    if (p->ammo_amount[p->curr_weapon] > 0 && p->random_club_frame == 0)
                    {
                        (*kb)=1;
                        if (PWEAPON(snum, p->curr_weapon, InitialSound) > 0)
                            A_PlaySound(PWEAPON(snum, p->curr_weapon, InitialSound), p->i);
                    }
                    break;

                case TRIPBOMB_WEAPON:
                    if (p->ammo_amount[p->curr_weapon] > 0)
                    {
                        hitdata_t hit;
                        hitscan((const vec3_t *)p,
                                p->cursectnum, sintable[(p->ang+512)&2047],
                                sintable[p->ang&2047], (100-p->horiz-p->horizoff)*32,
                                &hit,CLIPMASK1);

                        if (hit.sect < 0 || hit.sprite >= 0)
                            break;

                        // ST_2_UNDERWATER
                        if (hit.wall >= 0 && sector[hit.sect].lotag > 2)
                            break;

                        if (hit.wall >= 0 && wall[hit.wall].overpicnum >= 0)
                            if (wall[hit.wall].overpicnum == BIGFORCE)
                                break;

                        j = headspritesect[hit.sect];
                        while (j >= 0)
                        {
                            if (sprite[j].picnum == TRIPBOMB &&
                                    klabs(sprite[j].z-hit.pos.z) < (12<<8) &&
                                    ((sprite[j].x-hit.pos.x)*(sprite[j].x-hit.pos.x)+
                                     (sprite[j].y-hit.pos.y)*(sprite[j].y-hit.pos.y)) < (290*290))
                                break;
                            j = nextspritesect[j];
                        }

                        // ST_2_UNDERWATER
                        if (j == -1 && hit.wall >= 0 && (wall[hit.wall].cstat&16) == 0)
                            if ((wall[hit.wall].nextsector >= 0 &&
                                    sector[wall[hit.wall].nextsector].lotag <= 2) ||
                                    (wall[hit.wall].nextsector == -1 && sector[hit.sect].lotag <= 2))
                                if (((hit.pos.x-p->pos.x)*(hit.pos.x-p->pos.x) +
                                        (hit.pos.y-p->pos.y)*(hit.pos.y-p->pos.y)) < (290*290))
                                {
                                    p->pos.z = p->opos.z;
                                    p->vel.z = 0;
                                    (*kb) = 1;
                                    if (PWEAPON(snum, p->curr_weapon, InitialSound) > 0)
                                    {
                                        A_PlaySound(PWEAPON(snum, p->curr_weapon, InitialSound), p->i);
                                    }
                                }
                    }
                    break;

                case PISTOL_WEAPON:
                case CHAINGUN_WEAPON:
                case SHRINKER_WEAPON:
                case GROW_WEAPON:
                case FREEZE_WEAPON:
                case RPG_WEAPON:
                    if (p->ammo_amount[p->curr_weapon] > 0)
                    {
                        (*kb) = 1;
                        if (PWEAPON(snum, p->curr_weapon, InitialSound) > 0)
                            A_PlaySound(PWEAPON(snum, p->curr_weapon, InitialSound), p->i);
                    }
                    break;

                case DEVISTATOR_WEAPON:
                    if (p->ammo_amount[p->curr_weapon] > 0)
                    {
                        (*kb) = 1;
                        p->hbomb_hold_delay = !p->hbomb_hold_delay;
                        if (PWEAPON(snum, p->curr_weapon, InitialSound) > 0)
                            A_PlaySound(PWEAPON(snum, p->curr_weapon, InitialSound), p->i);
                    }
                    break;

                case KNEE_WEAPON:
                    if (p->quick_kick == 0)
                    {
                        (*kb) = 1;
                        if (PWEAPON(snum, p->curr_weapon, InitialSound) > 0)
                            A_PlaySound(PWEAPON(snum, p->curr_weapon, InitialSound), p->i);
                    }
                    break;
                }
            }
        }
    }
    else if (*kb)
    {
        if (PWEAPON(snum, p->curr_weapon, WorksLike) == HANDBOMB_WEAPON)
        {
            if (PWEAPON(snum, p->curr_weapon, HoldDelay) && ((*kb) == PWEAPON(snum, p->curr_weapon, FireDelay)) && TEST_SYNC_KEY(sb_snum, SK_FIRE))
            {
                p->rapid_fire_hold = 1;
                return;
            }

            if (++(*kb) == PWEAPON(snum, p->curr_weapon, HoldDelay))
            {
                p->ammo_amount[p->curr_weapon]--;

                if (numplayers < 2 || g_netServer)
                {
                    int32_t lPipeBombControl;

                    if (p->on_ground && TEST_SYNC_KEY(sb_snum, SK_CROUCH))
                    {
                        k = 15;
                        i = ((p->horiz+p->horizoff-100)*20);
                    }
                    else
                    {
                        k = 140;
                        i = -512-((p->horiz+p->horizoff-100)*20);
                    }

                    j = A_InsertSprite(p->cursectnum,
                                       p->pos.x+(sintable[(p->ang+512)&2047]>>6),
                                       p->pos.y+(sintable[p->ang&2047]>>6),
                                       p->pos.z,PWEAPON(snum, p->curr_weapon, Shoots),-16,9,9,
                                       p->ang,(k+(p->hbomb_hold_delay<<5)),i,p->i,1);

                    lPipeBombControl = PIPEBOMB_CONTROL(snum);

                    if (lPipeBombControl & PIPEBOMB_TIMER)
                    {
#ifdef LUNATIC
                        int32_t ltime = g_player[snum].ps->pipebombLifetime;
                        int32_t lv = g_player[snum].ps->pipebombLifetimeVar;
#else
                        int32_t ltime = Gv_GetVarByLabel("GRENADE_LIFETIME", NAM_GRENADE_LIFETIME, -1, snum);
                        int32_t lv=Gv_GetVarByLabel("GRENADE_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, -1, snum);
#endif
                        actor[j].t_data[7]= ltime
                                            + mulscale(krand(),lv, 14)
                                            - lv;
                        // TIMER_CONTROL
                        actor[j].t_data[6]=1;
                    }
                    else actor[j].t_data[6]=2;

                    if (k == 15)
                    {
                        sprite[j].yvel = 3;
                        sprite[j].z += (8<<8);
                    }

                    if (A_GetHitscanRange(p->i) < 512)
                    {
                        sprite[j].ang += 1024;
                        sprite[j].zvel /= 3;
                        sprite[j].xvel /= 3;
                    }
                }

                p->hbomb_on = 1;
            }
            else if ((*kb) < PWEAPON(snum, p->curr_weapon, HoldDelay) && TEST_SYNC_KEY(sb_snum, SK_FIRE))
                p->hbomb_hold_delay++;
            else if ((*kb) > PWEAPON(snum, p->curr_weapon, TotalTime))
            {
                (*kb) = 0;
                p->weapon_pos = WEAPON_POS_RAISE;
                if (PIPEBOMB_CONTROL(snum) == PIPEBOMB_REMOTE)
                {
                    p->curr_weapon = HANDREMOTE_WEAPON;
                    p->last_weapon = -1;
                }
                else P_CheckWeapon(p);
            }
        }
        else if (PWEAPON(snum, p->curr_weapon, WorksLike) == HANDREMOTE_WEAPON)
        {
            if (++(*kb) == PWEAPON(snum, p->curr_weapon, FireDelay))
            {
                if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_BOMB_TRIGGER)
                    p->hbomb_on = 0;

                if (PWEAPON(snum, p->curr_weapon, Shoots) != 0)
                {
                    if (!(PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_NOVISIBLE))
                    {
                        lastvisinc = totalclock+32;
                        p->visibility = 0;
                    }

                    P_SetWeaponGamevars(snum, p);
                    A_Shoot(p->i, PWEAPON(snum, p->curr_weapon, Shoots));
                }
            }

            if ((*kb) >= PWEAPON(snum, p->curr_weapon, TotalTime))
            {
                (*kb) = 0;
                if ((p->ammo_amount[HANDBOMB_WEAPON] > 0) && PIPEBOMB_CONTROL(snum) == PIPEBOMB_REMOTE)
                    P_AddWeapon(p, HANDBOMB_WEAPON, 1);
                else P_CheckWeapon(p);
            }
        }
        else
        {
            // the basic weapon...
            (*kb)++;

            if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_CHECKATRELOAD)
            {
                if (PWEAPON(snum, p->curr_weapon, WorksLike) == TRIPBOMB_WEAPON)
                {
                    if ((*kb) >= PWEAPON(snum, p->curr_weapon, TotalTime))
                    {
                        (*kb) = 0;
                        P_CheckWeapon(p);
                        p->weapon_pos = WEAPON_POS_LOWER;
                    }
                }
                else if (*kb >= PWEAPON(snum, p->curr_weapon, Reload))
                    P_CheckWeapon(p);
            }
            else if (PWEAPON(snum, p->curr_weapon, WorksLike)!=KNEE_WEAPON && *kb >= PWEAPON(snum, p->curr_weapon, FireDelay))
                P_CheckWeapon(p);

            if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_STANDSTILL
                    && *kb < (PWEAPON(snum, p->curr_weapon, FireDelay)+1))
            {
                p->pos.z = p->opos.z;
                p->vel.z = 0;
            }

            if (*kb == PWEAPON(snum, p->curr_weapon, Sound2Time))
                if (PWEAPON(snum, p->curr_weapon, Sound2Sound) > 0)
                    A_PlaySound(PWEAPON(snum, p->curr_weapon, Sound2Sound),p->i);

            if (*kb == PWEAPON(snum, p->curr_weapon, SpawnTime))
                P_DoWeaponSpawn(snum);

            if ((*kb) >= PWEAPON(snum, p->curr_weapon, TotalTime))
            {
                if (/*!(PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_CHECKATRELOAD) && */ p->reloading == 1 ||
                        (PWEAPON(snum, p->curr_weapon, Reload) > PWEAPON(snum, p->curr_weapon, TotalTime) && p->ammo_amount[p->curr_weapon] > 0
                         && (PWEAPON(snum, p->curr_weapon, Clip)) && (((p->ammo_amount[p->curr_weapon]%(PWEAPON(snum, p->curr_weapon, Clip)))==0))))
                {
                    int32_t i = PWEAPON(snum, p->curr_weapon, Reload) - PWEAPON(snum, p->curr_weapon, TotalTime);

                    p->reloading = 1;

                    if ((*kb) != (PWEAPON(snum, p->curr_weapon, TotalTime)))
                    {
                        if ((*kb) == (PWEAPON(snum, p->curr_weapon, TotalTime)+1))
                        {
                            if (PWEAPON(snum, p->curr_weapon, ReloadSound1) > 0)
                                A_PlaySound(PWEAPON(snum, p->curr_weapon, ReloadSound1),p->i);
                        }
                        else if (((*kb) == (PWEAPON(snum, p->curr_weapon, Reload) - (i/3)) &&
                                  !(PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_RELOAD_TIMING)) ||
                                 ((*kb) == (PWEAPON(snum, p->curr_weapon, Reload) - i+4) &&
                                  (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_RELOAD_TIMING)))
                        {
                            if (PWEAPON(snum, p->curr_weapon, ReloadSound2) > 0)
                                A_PlaySound(PWEAPON(snum, p->curr_weapon, ReloadSound2),p->i);
                        }
                        else if ((*kb) >= (PWEAPON(snum, p->curr_weapon, Reload)))
                        {
                            *kb=0;
                            p->reloading = 0;
                        }
                    }
                }
                else
                {
                    if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_AUTOMATIC &&
                            (PWEAPON(snum, p->curr_weapon, WorksLike)==KNEE_WEAPON?1:p->ammo_amount[p->curr_weapon] > 0))
                    {
                        if (TEST_SYNC_KEY(sb_snum, SK_FIRE))
                        {
                            if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_RANDOMRESTART)
                                *kb = 1+(krand()&3);
                            else *kb=1;
                        }
                        else *kb = 0;
                    }
                    else *kb = 0;

                    if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_RESET &&
                            ((PWEAPON(snum, p->curr_weapon, WorksLike) == KNEE_WEAPON)?1:p->ammo_amount[p->curr_weapon] > 0))
                    {
                        if (TEST_SYNC_KEY(sb_snum, SK_FIRE)) *kb = 1;
                        else *kb = 0;
                    }
                }
            }
            else if (*kb >= PWEAPON(snum, p->curr_weapon, FireDelay) && (*kb) < PWEAPON(snum, p->curr_weapon, TotalTime)
                     && ((PWEAPON(snum, p->curr_weapon, WorksLike) == KNEE_WEAPON)?1:p->ammo_amount[p->curr_weapon] > 0))
            {
                if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_AUTOMATIC)
                {
                    if (!(PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_SEMIAUTO))
                    {
                        if (TEST_SYNC_KEY(sb_snum, SK_FIRE) == 0 && PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_RESET)
                            *kb = 0;
                        if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_FIREEVERYTHIRD)
                        {
                            if (((*(kb))%3) == 0)
                            {
                                P_FireWeapon(snum);
                                P_DoWeaponSpawn(snum);
                            }
                        }
                        else if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_FIREEVERYOTHER)
                        {
                            P_FireWeapon(snum);
                            P_DoWeaponSpawn(snum);
                        }
                        else
                        {
                            if (*kb == PWEAPON(snum, p->curr_weapon, FireDelay))
                            {
                                P_FireWeapon(snum);
//                                P_DoWeaponSpawn(snum);
                            }
                        }
                        if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_RESET &&
                                (*kb) > PWEAPON(snum, p->curr_weapon, TotalTime)-PWEAPON(snum, p->curr_weapon, HoldDelay) &&
                                ((PWEAPON(snum, p->curr_weapon, WorksLike) == KNEE_WEAPON) || p->ammo_amount[p->curr_weapon] > 0))
                        {
                            if (TEST_SYNC_KEY(sb_snum, SK_FIRE)) *kb = 1;
                            else *kb = 0;
                        }
                    }
                    else
                    {
                        if (PWEAPON(snum, p->curr_weapon, Flags) & WEAPON_FIREEVERYOTHER)
                        {
                            P_FireWeapon(snum);
                            P_DoWeaponSpawn(snum);
                        }
                        else
                        {
                            if (*kb == PWEAPON(snum, p->curr_weapon, FireDelay))
                            {
                                P_FireWeapon(snum);
//                                P_DoWeaponSpawn(snum);
                            }
                        }
                    }
                }
                else if (*kb == PWEAPON(snum, p->curr_weapon, FireDelay))
                    P_FireWeapon(snum);
            }
        }
    }
}

static int32_t P_DoFist(DukePlayer_t *p)
{
    // the fist punching NUKEBUTTON

    if (++p->fist_incs == 28)
    {
        if (ud.recstat == 1) G_CloseDemoWrite();
        S_PlaySound(PIPEBOMB_EXPLODE);

        P_PalFrom(p, 48, 64,64,64);
    }

    if (p->fist_incs > 42)
    {
        int32_t i;

        for (TRAVERSE_CONNECT(i))
            g_player[i].ps->gm = MODE_EOL;

        if (p->buttonpalette && ud.from_bonus == 0)
        {
            ud.from_bonus = ud.level_number+1;
            if (ud.secretlevel > 0 && ud.secretlevel <= MAXLEVELS)
                ud.level_number = ud.secretlevel-1;
            ud.m_level_number = ud.level_number;
        }
        else
        {
            if (ud.from_bonus)
            {
                ud.m_level_number = ud.level_number = ud.from_bonus;
                ud.from_bonus = 0;
            }
            else
            {
                if (ud.level_number == ud.secretlevel && ud.from_bonus > 0)
                    ud.level_number = ud.from_bonus;
                else ud.level_number++;

                if (ud.level_number > MAXLEVELS-1)
                    ud.level_number = 0;
                ud.m_level_number = ud.level_number;
            }
        }

        p->fist_incs = 0;

        return 1;
    }

    return 0;
}

#ifdef YAX_ENABLE
static void getzsofslope_player(int16_t sectnum, int32_t dax, int32_t day, int32_t *ceilz, int32_t *florz)
{
    int32_t i, didceil=0, didflor=0;

    if ((sector[sectnum].ceilingstat&512)==0)
    {
        i = yax_getneighborsect(dax, day, sectnum, YAX_CEILING);
        if (i >= 0)
        {
            *ceilz = getceilzofslope(i, dax,day);
            didceil = 1;
        }
    }

    if ((sector[sectnum].floorstat&512)==0)
    {
        i = yax_getneighborsect(dax, day, sectnum, YAX_FLOOR);
        if (i >= 0)
        {
            *florz = getflorzofslope(i, dax,day);
            didflor = 1;
        }
    }

    if (!didceil || !didflor)
    {
        int32_t cz, fz;
        getzsofslope(sectnum, dax, day, &cz, &fz);

        if (!didceil)
            *ceilz = cz;
        if (!didflor)
            *florz = fz;
    }
}
#endif

void P_UpdatePosWhenViewingCam(DukePlayer_t *p)
{
    int32_t i = p->newowner;

    Bmemcpy(&p->pos, &sprite[i], sizeof(vec3_t));
    p->ang =  SA;
    p->vel.x = p->vel.y = sprite[p->i].xvel = 0;
    p->look_ang = 0;
    p->rotscrnang = 0;
}

void P_ProcessInput(int32_t snum)
{
    DukePlayer_t *const p = g_player[snum].ps;
    spritetype *const s = &sprite[p->i];

    uint32_t sb_snum = g_player[snum].sync->bits;

    int32_t j, i, k, doubvel = TICSPERFRAME, shrunk;
    int32_t fz, cz, hz, lz, truefdist, x, y, psectlotag;
    const uint8_t *const kb = &p->kickback_pic;
    int16_t tempsect;

    if (g_player[snum].playerquitflag == 0)
        return;

    p->player_par++;

    VM_OnEvent(EVENT_PROCESSINPUT, p->i, snum);

    if (p->cheat_phase > 0) sb_snum = 0;

    if (p->cursectnum == -1)
    {
        if (s->extra > 0 && ud.noclip == 0)
        {
            P_QuickKill(p);
            A_PlaySound(SQUISHED,p->i);
        }
        p->cursectnum = 0;
    }

    psectlotag = sector[p->cursectnum].lotag;
    p->spritebridge = p->sbs = 0;

    shrunk = (s->yrepeat < 32);
    getzrange((vec3_t *)p,p->cursectnum,&cz,&hz,&fz,&lz,163L,CLIPMASK0);

#ifdef YAX_ENABLE
    getzsofslope_player(p->cursectnum,p->pos.x,p->pos.y,&p->truecz,&p->truefz);
#else
    getzsofslope(p->cursectnum,p->pos.x,p->pos.y,&p->truecz,&p->truefz);
#endif
    j = p->truefz;

    truefdist = klabs(p->pos.z-j);

    if ((lz&49152) == 16384 && psectlotag == 1 && truefdist > PHEIGHT+(16<<8))
        psectlotag = 0;

    actor[p->i].floorz = fz;
    actor[p->i].ceilingz = cz;

    p->ohoriz = p->horiz;
    p->ohorizoff = p->horizoff;

    // calculates automatic view angle for playing without a mouse
    if (p->aim_mode == 0 && p->on_ground && psectlotag != ST_2_UNDERWATER && (sector[p->cursectnum].floorstat&2))
    {
        x = p->pos.x+(sintable[(p->ang+512)&2047]>>5);
        y = p->pos.y+(sintable[p->ang&2047]>>5);
        tempsect = p->cursectnum;
        updatesector(x,y,&tempsect);
        if (tempsect >= 0)
        {
            k = getflorzofslope(p->cursectnum,x,y);
            if (p->cursectnum == tempsect)
                p->horizoff += mulscale16(j-k,160);
            else if (klabs(getflorzofslope(tempsect,x,y)-k) <= (4<<8))
                p->horizoff += mulscale16(j-k,160);
        }
    }

    if (p->horizoff > 0) p->horizoff -= ((p->horizoff>>3)+1);
    else if (p->horizoff < 0) p->horizoff += (((-p->horizoff)>>3)+1);

    if (hz >= 0 && (hz&49152) == 49152)
    {
        hz &= (MAXSPRITES-1);

        if (sprite[hz].statnum == STAT_ACTOR && sprite[hz].extra >= 0)
        {
            hz = 0;
            cz = p->truecz;
        }
    }

    if (lz >= 0 && (lz&49152) == 49152)
    {
        j = lz&(MAXSPRITES-1);

        if ((sprite[j].cstat&33) == 33 || (sprite[j].cstat&17) == 17 ||
                clipshape_idx_for_sprite(&sprite[j], -1) >= 0)
        {
            // EDuke32 extension: xvel of 1 makes a sprite be never regarded as a bridge.
            if ((sprite[j].xvel&1) == 0)
            {
                psectlotag = 0;
                p->footprintcount = 0;
                p->spritebridge = 1;
                p->sbs = j;
            }
        }
        else if (A_CheckEnemySprite(&sprite[j]) && sprite[j].xrepeat > 24 && klabs(s->z-sprite[j].z) < (84<<8))
        {
            // TX: I think this is what makes the player slide off enemies... might
            // be a good sprite flag to add later.
            // Helix: there's also SLIDE_ABOVE_ENEMY.
            j = getangle(sprite[j].x-p->pos.x,sprite[j].y-p->pos.y);
            p->vel.x -= sintable[(j+512)&2047]<<4;
            p->vel.y -= sintable[j&2047]<<4;
        }
    }

    if (s->extra > 0)
        P_IncurDamage(p);
    else
    {
        s->extra = 0;
        p->inv_amount[GET_SHIELD] = 0;
    }

    p->last_extra = s->extra;

    if (p->loogcnt > 0) p->loogcnt--;
    else p->loogcnt = 0;

    if (p->fist_incs && P_DoFist(p)) return;

    if (p->timebeforeexit > 1 && p->last_extra > 0)
    {
        if (--p->timebeforeexit == GAMETICSPERSEC*5)
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();

            if (p->customexitsound >= 0)
            {
                S_PlaySound(p->customexitsound);
                P_DoQuote(QUOTE_WEREGONNAFRYYOURASS,p);
            }
        }
        else if (p->timebeforeexit == 1)
        {
            for (TRAVERSE_CONNECT(i))
                g_player[i].ps->gm = MODE_EOL;

            ud.m_level_number = ud.level_number++;

            if (ud.from_bonus)
            {
                ud.m_level_number = ud.level_number = ud.from_bonus;
                ud.from_bonus = 0;
            }
            return;
        }
    }

    if (p->pals.f > 0)
    {
#if !defined LUNATIC
        p->pals.f--;
#else
        if (p->palsfadespeed > 0)
        {
            // <palsfadespeed> is the tint fade speed is in
            // decrements/P_ProcessInput() calls.
            p->pals.f = max(p->pals.f - p->palsfadespeed, 0);
        }
        else
        {
            // <palsfadespeed> is a negated count of how many times we
            // (P_ProcessInput()) should be called before decrementing the tint
            // fading by one. <palsfadenext> is the live counter.
            if (p->palsfadenext < 0)
                p->palsfadenext++;

            if (p->palsfadenext == 0)
            {
                p->palsfadenext = p->palsfadespeed;
                p->pals.f--;
            }
        }
#endif
    }

    if (p->fta > 0 && --p->fta == 0)
    {
        pub = pus = NUMPAGES;
        p->ftq = 0;
    }

    if (g_levelTextTime > 0)
        g_levelTextTime--;

    if (s->extra <= 0)
    {
        if (ud.recstat == 1 && (!g_netServer && ud.multimode < 2))
            G_CloseDemoWrite();

        if ((numplayers < 2 || g_netServer) && p->dead_flag == 0)
            P_FragPlayer(snum);

        if (psectlotag == ST_2_UNDERWATER)
        {
            if (p->on_warping_sector == 0)
            {
                if (klabs(p->pos.z-fz) > (PHEIGHT>>1))
                    p->pos.z += 348;
            }
            else
            {
                s->z -= 512;
                s->zvel = -348;
            }

            clipmove((vec3_t *)p,&p->cursectnum,
                     0,0,164L,(4L<<8),(4L<<8),CLIPMASK0);
            //                        p->bobcounter += 32;
        }

        Bmemcpy(&p->opos, &p->pos, sizeof(vec3_t));
        p->oang = p->ang;
        p->opyoff = p->pyoff;

        p->horiz = 100;
        p->horizoff = 0;

        updatesector(p->pos.x,p->pos.y,&p->cursectnum);

        pushmove((vec3_t *)p,&p->cursectnum,128L,(4L<<8),(20L<<8),CLIPMASK0);

        if (fz > cz+(16<<8) && s->pal != 1)
            p->rotscrnang = (p->dead_flag + ((fz+p->pos.z)>>7))&2047;

        p->on_warping_sector = 0;

        return;
    }

    if (p->transporter_hold > 0)
    {
        p->transporter_hold--;
        if (p->transporter_hold == 0 && p->on_warping_sector)
            p->transporter_hold = 2;
    }
    else if (p->transporter_hold < 0)
        p->transporter_hold++;

    if (p->newowner >= 0)
    {
        P_UpdatePosWhenViewingCam(p);
        P_DoCounters(snum);

        if (PWEAPON(snum, p->curr_weapon, WorksLike) == HANDREMOTE_WEAPON)
            P_ProcessWeapon(snum);

        return;
    }

    p->rotscrnang -= (p->rotscrnang>>1);

    if (p->rotscrnang && !(p->rotscrnang>>1))
        p->rotscrnang -= ksgn(p->rotscrnang);

    p->look_ang -= (p->look_ang>>2);

    if (p->look_ang && !(p->look_ang>>2))
        p->look_ang -= ksgn(p->look_ang);

    if (TEST_SYNC_KEY(sb_snum, SK_LOOK_LEFT))
    {
        // look_left
        if (VM_OnEvent(EVENT_LOOKLEFT,p->i,snum) == 0)
        {
            p->look_ang -= 152;
            p->rotscrnang += 24;
        }
    }

    if (TEST_SYNC_KEY(sb_snum, SK_LOOK_RIGHT))
    {
        // look_right
        if (VM_OnEvent(EVENT_LOOKRIGHT,p->i,snum) == 0)
        {
            p->look_ang += 152;
            p->rotscrnang -= 24;
        }
    }

    if (p->on_crane >= 0)
        goto HORIZONLY;

    j = ksgn(g_player[snum].sync->avel);

    if (s->xvel < 32 || p->on_ground == 0 || p->bobcounter == 1024)
    {
        if ((p->weapon_sway&2047) > (1024+96))
            p->weapon_sway -= 96;
        else if ((p->weapon_sway&2047) < (1024-96))
            p->weapon_sway += 96;
        else p->weapon_sway = 1024;
    }
    else p->weapon_sway = p->bobcounter;

    // NOTE: This silently wraps if the difference is too great, e.g. used to do
    // that when teleported by silent SE7s.
    s->xvel = ksqrt(uhypsq(p->pos.x-p->bobpos.x, p->pos.y-p->bobpos.y));

    if (p->on_ground)
        p->bobcounter += sprite[p->i].xvel>>1;

    if (ud.noclip == 0 && ((uint16_t)p->cursectnum >= MAXSECTORS || sector[p->cursectnum].floorpicnum == MIRROR))
    {
        p->pos.x = p->opos.x;
        p->pos.y = p->opos.y;
    }
    else
    {
        p->opos.x = p->pos.x;
        p->opos.y = p->pos.y;
    }

    p->bobpos.x = p->pos.x;
    p->bobpos.y = p->pos.y;

    p->opos.z = p->pos.z;
    p->opyoff = p->pyoff;
    p->oang = p->ang;

    if (p->one_eighty_count < 0)
    {
        p->one_eighty_count += 128;
        p->ang += 128;
    }

    // Shrinking code

    i = 40;

    if (psectlotag == ST_2_UNDERWATER)
    {
        // under water
        p->jumping_counter = 0;

        p->pycount += 32;
        p->pycount &= 2047;
        p->pyoff = sintable[p->pycount]>>7;

        if (!A_CheckSoundPlaying(p->i,DUKE_UNDERWATER))
            A_PlaySound(DUKE_UNDERWATER,p->i);

        if (TEST_SYNC_KEY(sb_snum, SK_JUMP))
        {
            if (VM_OnEvent(EVENT_SWIMUP,p->i,snum) == 0)
            {
                // jump
                if (p->vel.z > 0) p->vel.z = 0;
                p->vel.z -= 348;
                if (p->vel.z < -(256*6)) p->vel.z = -(256*6);
            }
        }
        else if (TEST_SYNC_KEY(sb_snum, SK_CROUCH))
        {
            if (VM_OnEvent(EVENT_SWIMDOWN,p->i,snum) == 0)
            {
                // crouch
                if (p->vel.z < 0) p->vel.z = 0;
                p->vel.z += 348;
                if (p->vel.z > (256*6)) p->vel.z = (256*6);
            }
        }
        else
        {
            // normal view
            if (p->vel.z < 0)
            {
                p->vel.z += 256;
                if (p->vel.z > 0)
                    p->vel.z = 0;
            }
            if (p->vel.z > 0)
            {
                p->vel.z -= 256;
                if (p->vel.z < 0)
                    p->vel.z = 0;
            }
        }

        if (p->vel.z > 2048)
            p->vel.z >>= 1;

        p->pos.z += p->vel.z;

        if (p->pos.z > (fz-(15<<8)))
            p->pos.z += ((fz-(15<<8))-p->pos.z)>>1;

        if (p->pos.z < cz)
        {
            p->pos.z = cz;
            p->vel.z = 0;
        }

        if (p->scuba_on && (krand()&255) < 8)
        {
            j = A_Spawn(p->i,WATERBUBBLE);
            sprite[j].x +=
                sintable[(p->ang+512+64-(g_globalRandom&128))&2047]>>6;
            sprite[j].y +=
                sintable[(p->ang+64-(g_globalRandom&128))&2047]>>6;
            sprite[j].xrepeat = 3;
            sprite[j].yrepeat = 2;
            sprite[j].z = p->pos.z+(8<<8);
        }
    }
    else if (p->jetpack_on)
    {
        p->on_ground = 0;
        p->jumping_counter = 0;
        p->hard_landing = 0;
        p->falling_counter = 0;

        p->pycount += 32;
        p->pycount &= 2047;
        p->pyoff = sintable[p->pycount]>>7;

        if (p->jetpack_on < 11)
        {
            p->jetpack_on++;
            p->pos.z -= (p->jetpack_on<<7); //Goin up
        }
        else if (p->jetpack_on == 11 && !A_CheckSoundPlaying(p->i,DUKE_JETPACK_IDLE))
            A_PlaySound(DUKE_JETPACK_IDLE,p->i);

        if (shrunk) j = 512;
        else j = 2048;

        if (TEST_SYNC_KEY(sb_snum, SK_JUMP))         //A (soar high)
        {
            // jump
            if (VM_OnEvent(EVENT_SOARUP,p->i,snum) == 0)
            {
                p->pos.z -= j;
                p->crack_time = 777;
            }
        }

        if (TEST_SYNC_KEY(sb_snum, SK_CROUCH))   //Z (soar low)
        {
            // crouch
            if (VM_OnEvent(EVENT_SOARDOWN,p->i,snum) == 0)
            {
                p->pos.z += j;
                p->crack_time = 777;
            }
        }

        if (shrunk == 0 && (psectlotag == 0 || psectlotag == ST_2_UNDERWATER)) k = 32;
        else k = 16;

        if (psectlotag != ST_2_UNDERWATER && p->scuba_on == 1)
            p->scuba_on = 0;

        if (p->pos.z > (fz-(k<<8)))
            p->pos.z += ((fz-(k<<8))-p->pos.z)>>1;
        if (p->pos.z < (actor[p->i].ceilingz+(18<<8)))
            p->pos.z = actor[p->i].ceilingz+(18<<8);
    }
    else if (psectlotag != ST_2_UNDERWATER)
    {
        p->airleft = 15 * GAMETICSPERSEC; // 13 seconds

        if (p->scuba_on == 1)
            p->scuba_on = 0;

        if (psectlotag == ST_1_ABOVE_WATER && p->spritebridge == 0)
        {
            if (shrunk == 0)
            {
                i = 34;
                p->pycount += 32;
                p->pycount &= 2047;
                p->pyoff = sintable[p->pycount]>>6;
            }
            else i = 12;

            if (shrunk == 0 && truefdist <= PHEIGHT)
            {
                if (p->on_ground == 1)
                {
                    if (p->dummyplayersprite < 0)
                        p->dummyplayersprite = A_Spawn(p->i,PLAYERONWATER);
                    sprite[p->dummyplayersprite].pal = sprite[p->i].pal;
                    sprite[p->dummyplayersprite].cstat |= 32768;

                    p->footprintcount = 6;
                    if (sector[p->cursectnum].floorpicnum == FLOORSLIME)
                        p->footprintpal = 8;
                    else p->footprintpal = 0;
                    p->footprintshade = 0;
                }
            }
        }
        else
        {
            if (p->footprintcount > 0 && p->on_ground)
                if (p->cursectnum >= 0 && (sector[p->cursectnum].floorstat&2) != 2)
                {
                    for (j=headspritesect[p->cursectnum]; j>=0; j=nextspritesect[j])
                        if (sprite[j].picnum == FOOTPRINTS || sprite[j].picnum == FOOTPRINTS2 ||
                                sprite[j].picnum == FOOTPRINTS3 || sprite[j].picnum == FOOTPRINTS4)
                            if (klabs(sprite[j].x-p->pos.x) < 384 && klabs(sprite[j].y-p->pos.y) < 384)
                                break;

                    if (j < 0)
                    {
                        if (p->cursectnum >= 0 && sector[p->cursectnum].lotag == 0 && sector[p->cursectnum].hitag == 0)
#ifdef YAX_ENABLE
                            if (yax_getbunch(p->cursectnum, YAX_FLOOR) < 0 || (sector[p->cursectnum].floorstat&512))
#endif
                        {
                            switch (krand()&3)
                            {
                            case 0:
                                j = A_Spawn(p->i,FOOTPRINTS);
                                break;
                            case 1:
                                j = A_Spawn(p->i,FOOTPRINTS2);
                                break;
                            case 2:
                                j = A_Spawn(p->i,FOOTPRINTS3);
                                break;
                            default:
                                j = A_Spawn(p->i,FOOTPRINTS4);
                                break;
                            }
                            sprite[j].pal = p->footprintpal;
                            sprite[j].shade = p->footprintshade;
                            p->footprintcount--;
                        }
                    }
                }
        }

        if (p->pos.z < (fz-(i<<8)))  //falling
        {
            // not jumping or crouching

            if (!TEST_SYNC_KEY(sb_snum, SK_JUMP) && !TEST_SYNC_KEY(sb_snum, SK_CROUCH) &&
                    p->on_ground && (sector[p->cursectnum].floorstat&2) && p->pos.z >= (fz-(i<<8)-(16<<8)))
                p->pos.z = fz-(i<<8);
            else
            {
                p->on_ground = 0;
                p->vel.z += (g_spriteGravity+80); // (TICSPERFRAME<<6);
                if (p->vel.z >= (4096+2048)) p->vel.z = (4096+2048);
                if (p->vel.z > 2400 && p->falling_counter < 255)
                {
                    p->falling_counter++;
                    if (p->falling_counter >= 38 && p->scream_voice <= FX_Ok)
                    {
                        int32_t voice = A_PlaySound(DUKE_SCREAM,p->i);
                        if (voice <= 127)  // XXX: p->scream_voice is an int8_t
                            p->scream_voice = voice;
                    }
                }

                if ((p->pos.z+p->vel.z) >= (fz-(i<<8)) && p->cursectnum >= 0)   // hit the ground
                    if (sector[p->cursectnum].lotag != ST_1_ABOVE_WATER)
                    {
                        if (p->falling_counter > 62)
                            P_QuickKill(p);
                        else if (p->falling_counter > 9)
                        {
                            // Falling damage.
                            s->extra -= p->falling_counter-(krand()&3);

                            if (s->extra <= 0)
                            {
                                A_PlaySound(SQUISHED,p->i);

//                                P_PalFrom(p, 63, 63,0,0);
                            }
                            else
                            {
                                A_PlaySound(DUKE_LAND,p->i);
                                A_PlaySound(DUKE_LAND_HURT,p->i);
                            }

                            P_PalFrom(p, 32, 16,0,0);
                        }
                        else if (p->vel.z > 2048)
                            A_PlaySound(DUKE_LAND,p->i);
                    }
            }
        }
        else
        {
            p->falling_counter = 0;

            if (p->scream_voice > FX_Ok)
            {
                FX_StopSound(p->scream_voice);
                S_Cleanup();
                p->scream_voice = -1;
            }

            if (psectlotag != ST_1_ABOVE_WATER && psectlotag != ST_2_UNDERWATER && p->on_ground == 0 && p->vel.z > (6144>>1))
                p->hard_landing = p->vel.z>>10;

            p->on_ground = 1;

            if (i==40)
            {
                //Smooth on the ground

                k = ((fz-(i<<8))-p->pos.z)>>1;
                if (klabs(k) < 256) k = 0;
                p->pos.z += k;
                p->vel.z -= 768;
                if (p->vel.z < 0) p->vel.z = 0;
            }
            else if (p->jumping_counter == 0)
            {
                p->pos.z += ((fz-(i<<7))-p->pos.z)>>1; //Smooth on the water
                if (p->on_warping_sector == 0 && p->pos.z > fz-(16<<8))
                {
                    p->pos.z = fz-(16<<8);
                    p->vel.z >>= 1;
                }
            }

            p->on_warping_sector = 0;

            if (TEST_SYNC_KEY(sb_snum, SK_CROUCH))
            {
                // crouching
                if (VM_OnEvent(EVENT_CROUCH,p->i,snum) == 0)
                {
                    p->pos.z += (2048+768);
                    p->crack_time = 777;
                }
            }

            // jumping
            if (!TEST_SYNC_KEY(sb_snum, SK_JUMP) && p->jumping_toggle == 1)
                p->jumping_toggle = 0;
            else if (TEST_SYNC_KEY(sb_snum, SK_JUMP) && p->jumping_toggle == 0)
            {
                if (p->jumping_counter == 0)
                    if ((fz-cz) > (56<<8))
                    {
                        if (VM_OnEvent(EVENT_JUMP,p->i,snum) == 0)
                        {
                            p->jumping_counter = 1;
                            p->jumping_toggle = 1;
                        }
                    }
            }

            if (p->jumping_counter && !TEST_SYNC_KEY(sb_snum, SK_JUMP))
                p->jumping_toggle = 0;
        }

        if (p->jumping_counter)
        {
            if (!TEST_SYNC_KEY(sb_snum, SK_JUMP) && p->jumping_toggle == 1)
                p->jumping_toggle = 0;

            if (p->jumping_counter < (1024+256))
            {
                if (psectlotag == ST_1_ABOVE_WATER && p->jumping_counter > 768)
                {
                    p->jumping_counter = 0;
                    p->vel.z = -512;
                }
                else
                {
                    p->vel.z -= (sintable[(2048-128+p->jumping_counter)&2047])/12;
                    p->jumping_counter += 180;
                    p->on_ground = 0;
                }
            }
            else
            {
                p->jumping_counter = 0;
                p->vel.z = 0;
            }
        }

        p->pos.z += p->vel.z;

        if ((psectlotag != ST_2_UNDERWATER || cz != sector[p->cursectnum].ceilingz) && p->pos.z < (cz+(4<<8)))
        {
            p->jumping_counter = 0;
            if (p->vel.z < 0)
                p->vel.x = p->vel.y = 0;
            p->vel.z = 128;
            p->pos.z = cz+(4<<8);
        }
    }

    if (p->fist_incs || p->transporter_hold > 2 || p->hard_landing || p->access_incs > 0 || p->knee_incs > 0 ||
            (PWEAPON(snum, p->curr_weapon, WorksLike) == TRIPBOMB_WEAPON &&
             *kb > 1 && *kb < PWEAPON(snum, p->curr_weapon, FireDelay)))
    {
        doubvel = 0;
        p->vel.x = 0;
        p->vel.y = 0;
    }
    else if (g_player[snum].sync->avel)            //p->ang += syncangvel * constant
    {
        int32_t tempang = g_player[snum].sync->avel;

        if (psectlotag == ST_2_UNDERWATER) p->angvel =(tempang-(tempang>>3))*ksgn(doubvel);
        else p->angvel = tempang*ksgn(doubvel);

        p->ang += p->angvel;
        p->ang &= 2047;
        p->crack_time = 777;
    }

    if (p->spritebridge == 0)
    {
        j = sector[s->sectnum].floorpicnum;

        if (j == PURPLELAVA || sector[s->sectnum].ceilingpicnum == PURPLELAVA)
        {
            if (p->inv_amount[GET_BOOTS] > 0)
            {
                p->inv_amount[GET_BOOTS]--;
                p->inven_icon = ICON_BOOTS;
                if (p->inv_amount[GET_BOOTS] <= 0)
                    P_SelectNextInvItem(p);
            }
            else
            {
                if (!A_CheckSoundPlaying(p->i,DUKE_LONGTERM_PAIN))
                    A_PlaySound(DUKE_LONGTERM_PAIN,p->i);

                P_PalFrom(p, 32, 0,8,0);
                s->extra--;
            }
        }

        if (p->on_ground && truefdist <= PHEIGHT+(16<<8) && P_CheckFloorDamage(p, j))
        {
            P_DoQuote(QUOTE_BOOTS_ON, p);
            p->inv_amount[GET_BOOTS] -= 2;
            if (p->inv_amount[GET_BOOTS] <= 0)
            {
                p->inv_amount[GET_BOOTS] = 0;
                P_SelectNextInvItem(p);
            }
        }
    }

    if (g_player[snum].sync->extbits&(1))
        VM_OnEvent(EVENT_MOVEFORWARD,p->i,snum);

    if (g_player[snum].sync->extbits&(1<<1))
        VM_OnEvent(EVENT_MOVEBACKWARD,p->i,snum);

    if (g_player[snum].sync->extbits&(1<<2))
        VM_OnEvent(EVENT_STRAFELEFT,p->i,snum);

    if (g_player[snum].sync->extbits&(1<<3))
        VM_OnEvent(EVENT_STRAFERIGHT,p->i,snum);

    if (g_player[snum].sync->extbits&(1<<4) || g_player[snum].sync->avel < 0)
        VM_OnEvent(EVENT_TURNLEFT,p->i,snum);

    if (g_player[snum].sync->extbits&(1<<5) || g_player[snum].sync->avel > 0)
        VM_OnEvent(EVENT_TURNRIGHT,p->i,snum);

    if (p->vel.x || p->vel.y || g_player[snum].sync->fvel || g_player[snum].sync->svel)
    {
        p->crack_time = 777;

        k = sintable[p->bobcounter&2047]>>12;

        if ((truefdist < PHEIGHT+(8<<8)) && (k == 1 || k == 3))
        {
            if (p->walking_snd_toggle == 0 && p->on_ground)
            {
                switch (psectlotag)
                {
                case 0:
                    if (lz >= 0 && (lz&49152) == 49152)
                        j = sprite[lz&(MAXSPRITES-1)].picnum;
                    else j = sector[p->cursectnum].floorpicnum;

                    switch (DYNAMICTILEMAP(j))
                    {
                    case PANNEL1__STATIC:
                    case PANNEL2__STATIC:
                        A_PlaySound(DUKE_WALKINDUCTS,p->i);
                        p->walking_snd_toggle = 1;
                        break;
                    }
                    break;

                case ST_1_ABOVE_WATER:
                    if (!p->spritebridge)
                    {
                        if ((krand()&1) == 0)
                            A_PlaySound(DUKE_ONWATER,p->i);
                        p->walking_snd_toggle = 1;
                    }
                    break;
                }
            }
        }
        else if (p->walking_snd_toggle > 0)
            p->walking_snd_toggle--;

        if (p->jetpack_on == 0 && p->inv_amount[GET_STEROIDS] > 0 && p->inv_amount[GET_STEROIDS] < 400)
            doubvel <<= 1;

        p->vel.x += (((g_player[snum].sync->fvel) * doubvel) << 6);
        p->vel.y += (((g_player[snum].sync->svel) * doubvel) << 6);

        j = 0;

        if (psectlotag == ST_2_UNDERWATER)
            j = 0x1400;
        else if (p->on_ground && (TEST_SYNC_KEY(sb_snum, SK_CROUCH) || (*kb > 10 && PWEAPON(snum, p->curr_weapon, WorksLike) == KNEE_WEAPON)))
            j = 0x2000;

        p->vel.x = mulscale16(p->vel.x, p->runspeed - j);
        p->vel.y = mulscale16(p->vel.y, p->runspeed - j);

        if (klabs(p->vel.x) < 2048 && klabs(p->vel.y) < 2048)
            p->vel.x = p->vel.y = 0;

        if (shrunk)
        {
            p->vel.x = mulscale16(p->vel.x,p->runspeed-(p->runspeed>>1)+(p->runspeed>>2));
            p->vel.y = mulscale16(p->vel.y,p->runspeed-(p->runspeed>>1)+(p->runspeed>>2));
        }
    }

HORIZONLY:
    if (psectlotag == ST_1_ABOVE_WATER || p->spritebridge == 1) i = p->autostep_sbw;
    else i = p->autostep;

#ifdef EDUKE32_TOUCH_DEVICES
    if (TEST_SYNC_KEY(sb_snum, SK_CROUCH))
        i = p->autostep_sbw;
#endif

    if (p->cursectnum >= 0 && sector[p->cursectnum].lotag == ST_2_UNDERWATER) k = 0;
    else k = 1;

    if (ud.noclip)
    {
        p->pos.x += p->vel.x>>14;
        p->pos.y += p->vel.y>>14;
        updatesector(p->pos.x,p->pos.y,&p->cursectnum);
        changespritesect(p->i,p->cursectnum);
    }
    else
    {
#ifdef YAX_ENABLE
        int32_t sect = p->cursectnum;
        int16_t cb, fb;

        if (sect >= 0)
            yax_getbunches(sect, &cb, &fb);

        // This updatesectorz conflicts with Duke3D's way of teleporting through water,
        // so make it a bit conditional... OTOH, this way we have an ugly z jump when
        // changing from above water to underwater
        if (sect >= 0 && !(sector[sect].lotag==ST_1_ABOVE_WATER && p->on_ground && fb>=0))
        {
            if ((fb>=0 && !(sector[sect].floorstat&512)) || (cb>=0 && !(sector[sect].ceilingstat&512)))
            {
                p->cursectnum += MAXSECTORS;  // skip initial z check, restored by updatesectorz
                updatesectorz(p->pos.x,p->pos.y,p->pos.z,&p->cursectnum);
            }
        }
#endif
        if ((j = clipmove((vec3_t *)p, &p->cursectnum, p->vel.x + (p->fric.x << 9), p->vel.y + (p->fric.y << 9), 164L,
                          (4L << 8), i, CLIPMASK0)))
            P_CheckTouchDamage(p, j);

        p->fric.x = p->fric.y = 0;
    }

    // This makes the player view lower when shrunk.  NOTE that it can get the
    // view below the sector floor (and does, when on the ground).
    if (p->jetpack_on == 0 && psectlotag != ST_2_UNDERWATER && psectlotag != ST_1_ABOVE_WATER && shrunk)
        p->pos.z += 32<<8;

    if (p->jetpack_on == 0)
    {
        if (s->xvel > 16)
        {
            if (psectlotag != ST_1_ABOVE_WATER && psectlotag != ST_2_UNDERWATER && p->on_ground)
            {
                p->pycount += 52;
                p->pycount &= 2047;
                p->pyoff =
                    klabs(s->xvel*sintable[p->pycount])/1596;
            }
        }
        else if (psectlotag != ST_2_UNDERWATER && psectlotag != ST_1_ABOVE_WATER)
            p->pyoff = 0;
    }

    // RBG***

    p->pos.z += PHEIGHT;
    setsprite(p->i,(vec3_t *)&p->pos.x);
    p->pos.z -= PHEIGHT;

    // ST_2_UNDERWATER
    if (p->cursectnum >= 0 && psectlotag < 3)
    {
        const sectortype *sec = &sector[p->cursectnum];
//        p->cursectnum = s->sectnum;

        if (!ud.noclip && sec->lotag == ST_31_TWO_WAY_TRAIN)
        {
            // TRAIN_SECTOR_TO_SE_INDEX
            if ((unsigned)sec->hitag < MAXSPRITES && sprite[sec->hitag].xvel
                    && actor[sec->hitag].t_data[0] == 0)
            {
                P_QuickKill(p);
                return;
            }
        }
    }

    if (p->cursectnum >= 0 && truefdist < PHEIGHT && p->on_ground &&
            psectlotag != ST_1_ABOVE_WATER && shrunk == 0 && sector[p->cursectnum].lotag == ST_1_ABOVE_WATER)
        if (!A_CheckSoundPlaying(p->i,DUKE_ONWATER))
            A_PlaySound(DUKE_ONWATER,p->i);

    if (p->cursectnum >=0 && p->cursectnum != s->sectnum)
        changespritesect(p->i, p->cursectnum);

    if (p->cursectnum >= 0 && ud.noclip == 0)
    {
        j = (pushmove((vec3_t *)p,&p->cursectnum,164L,(4L<<8),(4L<<8),CLIPMASK0) < 0 && A_GetFurthestAngle(p->i,8) < 512);

        if (klabs(actor[p->i].floorz-actor[p->i].ceilingz) < (48<<8) || j)
        {
            if (!(sector[s->sectnum].lotag&0x8000) && (isanunderoperator(sector[s->sectnum].lotag) ||
                    isanearoperator(sector[s->sectnum].lotag)))
                G_ActivateBySector(s->sectnum,p->i);
            if (j)
            {
                P_QuickKill(p);
                return;
            }
        }
        else if (klabs(fz-cz) < (32<<8) && isanunderoperator(sector[p->cursectnum].lotag))
            G_ActivateBySector(p->cursectnum,p->i);
    }

    i = 0;
    if (TEST_SYNC_KEY(sb_snum, SK_CENTER_VIEW) || p->hard_landing)
        if (VM_OnEvent(EVENT_RETURNTOCENTER,p->i,snum) == 0)
            p->return_to_center = 9;

    if (TEST_SYNC_KEY(sb_snum, SK_LOOK_UP))
    {
        if (VM_OnEvent(EVENT_LOOKUP,p->i,snum) == 0)
        {
            p->return_to_center = 9;
            if (TEST_SYNC_KEY(sb_snum, SK_RUN)) p->horiz += 12;
            p->horiz += 12;
            i++;
        }
    }

    if (TEST_SYNC_KEY(sb_snum, SK_LOOK_DOWN))
    {
        if (VM_OnEvent(EVENT_LOOKDOWN,p->i,snum) == 0)
        {
            p->return_to_center = 9;
            if (TEST_SYNC_KEY(sb_snum, SK_RUN)) p->horiz -= 12;
            p->horiz -= 12;
            i++;
        }
    }

    if (TEST_SYNC_KEY(sb_snum, SK_AIM_UP))
    {
        if (VM_OnEvent(EVENT_AIMUP,p->i,snum) == 0)
        {
            if (TEST_SYNC_KEY(sb_snum, SK_RUN)) p->horiz += 6;
            p->horiz += 6;
            i++;
        }
    }

    if (TEST_SYNC_KEY(sb_snum, SK_AIM_DOWN))
    {
        if (VM_OnEvent(EVENT_AIMDOWN,p->i,snum) == 0)
        {
            if (TEST_SYNC_KEY(sb_snum, SK_RUN)) p->horiz -= 6;
            p->horiz -= 6;
            i++;
        }
    }

    if (p->return_to_center > 0 && !TEST_SYNC_KEY(sb_snum, SK_LOOK_UP) && !TEST_SYNC_KEY(sb_snum, SK_LOOK_DOWN))
    {
        p->return_to_center--;
        p->horiz += 33-(p->horiz/3);
        i++;
    }

    if (p->hard_landing > 0)
    {
        p->hard_landing--;
        p->horiz -= (p->hard_landing<<4);
    }

    if (i)
    {
        if (p->horiz > 95 && p->horiz < 105) p->horiz = 100;
        if (p->horizoff > -5 && p->horizoff < 5) p->horizoff = 0;
    }

    p->horiz += g_player[snum].sync->horz;

    if (p->horiz > HORIZ_MAX) p->horiz = HORIZ_MAX;
    else if (p->horiz < HORIZ_MIN) p->horiz = HORIZ_MIN;

    //Shooting code/changes

    if (p->show_empty_weapon > 0)
    {
        p->show_empty_weapon--;
        if (p->show_empty_weapon == 0 && (p->weaponswitch & 2) && p->ammo_amount[p->curr_weapon] <= 0)
        {
            if (p->last_full_weapon == GROW_WEAPON)
                p->subweapon |= (1<<GROW_WEAPON);
            else if (p->last_full_weapon == SHRINKER_WEAPON)
                p->subweapon &= ~(1<<GROW_WEAPON);
            P_AddWeapon(p, p->last_full_weapon, 1);
            return;
        }
    }

    if (p->knee_incs > 0)
    {
        p->horiz -= 48;
        p->return_to_center = 9;

        if (++p->knee_incs > 15)
        {
            p->knee_incs = 0;
            p->holster_weapon = 0;
            p->weapon_pos = klabs(p->weapon_pos);

            if (p->actorsqu >= 0 && sprite[p->actorsqu].statnum != MAXSTATUS && dist(&sprite[p->i],&sprite[p->actorsqu]) < 1400)
            {
                A_DoGuts(p->actorsqu,JIBS6,7);
                A_Spawn(p->actorsqu,BLOODPOOL);
                A_PlaySound(SQUISHED,p->actorsqu);

                switch (DYNAMICTILEMAP(sprite[p->actorsqu].picnum))
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
                    if (sprite[p->actorsqu].yvel)
                        G_OperateRespawns(sprite[p->actorsqu].yvel);
                    A_DeleteSprite(p->actorsqu);
                    break;
                case APLAYER__STATIC:
                {
                    int32_t snum = P_Get(p->actorsqu);
                    P_QuickKill(g_player[snum].ps);
                    g_player[snum].ps->frag_ps = snum;
                    break;
                }
                default:
                    if (A_CheckEnemySprite(&sprite[p->actorsqu]))
                        p->actors_killed++;
                    A_DeleteSprite(p->actorsqu);
                    break;
                }
            }
            p->actorsqu = -1;
        }
        else if (p->actorsqu >= 0)
            p->ang += G_GetAngleDelta(p->ang,getangle(sprite[p->actorsqu].x-p->pos.x,sprite[p->actorsqu].y-p->pos.y))>>2;
    }

    if (P_DoCounters(snum))
        return;

    P_ProcessWeapon(snum);
}
