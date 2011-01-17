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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "duke3d.h"
#include "osd.h"
#include "player.h"
#include "demo.h"
#include "enet/enet.h"

int32_t g_currentweapon;
int32_t g_gun_pos;
int32_t g_looking_arc;
int32_t g_weapon_offset;
int32_t g_gs;
int32_t g_kb;
int32_t g_looking_angSR1;
int32_t g_weapon_xoffset;

extern int32_t g_levelTextTime, ticrandomseed;

int32_t g_numObituaries = 0;
int32_t g_numSelfObituaries = 0;

void P_UpdateScreenPal(DukePlayer_t *p)
{
    if (p->heat_on) p->palette = SLIMEPAL;
    else if (p->cursectnum < 0) p->palette = BASEPAL;
    else if ((sector[p->cursectnum].ceilingpicnum >= FLOORSLIME)&&(sector[p->cursectnum].ceilingpicnum <=FLOORSLIME+2))
    {
        p->palette = SLIMEPAL;
    }
    else
    {
        if (sector[p->cursectnum].lotag == 2) p->palette = WATERPAL;
        else p->palette = BASEPAL;
    }
    g_restorePalette = 1;
}

static void P_IncurDamage(DukePlayer_t *p)
{
    int32_t damage;

    aGameVars[g_iReturnVarID].val.lValue = 0;
    if (apScriptGameEvent[EVENT_INCURDAMAGE])
        VM_OnEvent(EVENT_INCURDAMAGE, p->i, sprite[p->i].yvel, -1);

    if (aGameVars[g_iReturnVarID].val.lValue == 0)
    {
        sprite[p->i].extra -= p->extra_extra8>>8;

        damage = sprite[p->i].extra - p->last_extra;

        if (damage < 0)
        {
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

    }

}

void P_QuickKill(DukePlayer_t *p)
{
    p->pals.r = 48;
    p->pals.g = 48;
    p->pals.b = 48;
    p->pals.f = 48;

    sprite[p->i].extra = 0;
    sprite[p->i].cstat |= 32768;
    if (ud.god == 0)
        A_DoGuts(p->i,JIBS6,8);
    return;
}

static void A_DoWaterTracers(int32_t x1,int32_t y1,int32_t z1,int32_t x2,int32_t y2,int32_t z2,int32_t n)
{
    int32_t i, xv, yv, zv;
    int16_t sect = -1;

    i = n+1;
    xv = (x2-x1)/i;
    yv = (y2-y1)/i;
    zv = (z2-z1)/i;

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

        if (sector[sect].lotag == 2)
            A_InsertSprite(sect,x1,y1,z1,WATERBUBBLE,-32,4+(krand()&3),4+(krand()&3),krand()&2047,0,0,g_player[0].ps->i,5);
        else
            A_InsertSprite(sect,x1,y1,z1,SMALLSMOKE,-32,14,14,0,0,0,g_player[0].ps->i,5);
    }
}

static void A_HitscanProjTrail(const vec3_t *sv, const vec3_t *dv, int32_t ang, int32_t atwith)
{
    int32_t n, j, i;
    int16_t sect = -1;
    vec3_t srcvect;
    vec3_t destvect;

    Bmemcpy(&destvect, dv, sizeof(vec3_t));

    srcvect.x = sv->x + (sintable[(348+ang+512)&2047]/ProjectileData[atwith].offset);
    srcvect.y = sv->y + (sintable[(ang+348)&2047]/ProjectileData[atwith].offset);
    srcvect.z = sv->z + 1024+(ProjectileData[atwith].toffset<<8);

    n = ((FindDistance2D(srcvect.x-destvect.x,srcvect.y-destvect.y))>>8)+1;

    destvect.x = ((destvect.x-srcvect.x)/n);
    destvect.y = ((destvect.y-srcvect.y)/n);
    destvect.z = ((destvect.z-srcvect.z)/n);

    srcvect.x += destvect.x>>2;
    srcvect.y += destvect.y>>2;
    srcvect.z += (destvect.z>>2);

    for (i=ProjectileData[atwith].tnum; i>0; i--)
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
        j = A_InsertSprite(sect,srcvect.x,srcvect.y,srcvect.z,ProjectileData[atwith].trail,-32,
                           ProjectileData[atwith].txrepeat,ProjectileData[atwith].tyrepeat,ang,0,0,g_player[0].ps->i,0);
        changespritestat(j,1);
    }
}

int32_t A_GetHitscanRange(int32_t i)
{
    int32_t zoff = (PN == APLAYER) ? PHEIGHT : 0;
    hitdata_t hitinfo;

    SZ -= zoff;
    hitscan((const vec3_t *)&sprite[i],SECT,
            sintable[(SA+512)&2047],
            sintable[SA&2047],
            0,&hitinfo,CLIPMASK1);
    SZ += zoff;

    return (FindDistance2D(hitinfo.pos.x-SX,hitinfo.pos.y-SY));
}

static int32_t A_FindTargetSprite(spritetype *s,int32_t aang,int32_t atwith)
{
    int32_t gotshrinker,gotfreezer;
    int32_t i, j, a, k, cans;
    static int32_t aimstats[] = { 10, 13, 1, 2 };
    int32_t dx1, dy1, dx2, dy2, dx3, dy3, smax, sdist;
    int32_t xv, yv;

    if (s->picnum == APLAYER)
    {
        if (!g_player[s->yvel].ps->auto_aim)
            return -1;
        if (g_player[s->yvel].ps->auto_aim == 2)
        {
            if (A_CheckSpriteTileFlags(atwith,SPRITE_PROJECTILE) && (ProjectileData[atwith].workslike & PROJECTILE_RPG))
                return -1;
            else switch (DynamicTileMap[atwith])
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

    gotshrinker = (s->picnum == APLAYER && *aplWeaponWorksLike[g_player[s->yvel].ps->curr_weapon] == SHRINKER_WEAPON);
    gotfreezer = (s->picnum == APLAYER && *aplWeaponWorksLike[g_player[s->yvel].ps->curr_weapon] == FREEZE_WEAPON);

    smax = 0x7fffffff;

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
                        if (PN == APLAYER &&
                                //                        ud.ffire == 0 &&
                                (GTFLAGS(GAMETYPE_PLAYERSFRIENDLY) || (GTFLAGS(GAMETYPE_TDM) &&
                                        g_player[sprite[i].yvel].ps->team == g_player[s->yvel].ps->team)) &&
                                s->picnum == APLAYER &&
                                s != &sprite[i])
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

                    if ((dy1*xv) <= (dx1*yv))
                        if ((dy2*xv) >= (dx2*yv))
                        {
                            sdist = mulscale(dx3,xv,14) + mulscale(dy3,yv,14);
                            if (sdist > 512 && sdist < smax)
                            {
                                if (s->picnum == APLAYER)
                                    a = (klabs(scale(SZ-s->z,10,sdist)-(g_player[s->yvel].ps->horiz+g_player[s->yvel].ps->horizoff-100)) < 100);
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

int32_t A_Shoot(int32_t i,int32_t atwith)
{
    int16_t l, sa, p, j, k=-1;
    int32_t vel, zvel = 0, x, oldzvel, dal;
    hitdata_t hitinfo;
    vec3_t srcvect;
    char sizx,sizy;
    spritetype *s = &sprite[i];
    int16_t sect = s->sectnum;

    if (s->picnum == APLAYER)
    {
        p = s->yvel;

        Bmemcpy(&srcvect,g_player[p].ps,sizeof(vec3_t));
        srcvect.z += g_player[p].ps->pyoff+(4<<8);
        sa = g_player[p].ps->ang;

        g_player[p].ps->crack_time = 777;
    }
    else
    {
        p = -1;
        sa = s->ang;
        Bmemcpy(&srcvect,s,sizeof(vec3_t));
        srcvect.z -= (((s->yrepeat*tilesizy[s->picnum])<<1)-(4<<8));

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
        switch (DynamicTileMap[atwith])
        {
        case FIRELASER__STATIC:
        case SHOTGUN__STATIC:
        case SHOTSPARK1__STATIC:
        case CHAINGUN__STATIC:
        case RPG__STATIC:
        case MORTER__STATIC:
        {
            int32_t x = ((sintable[(s->ang+512)&2047])>>7), y = ((sintable[(s->ang)&2047])>>7);
            s-> x += x;
            s-> y += y;
            G_AddGameLight(0, i, PHEIGHT, 8192, 255+(95<<8),PR_LIGHT_PRIO_MAX_GAME);
            actor[i].lightcount = 2;
            s-> x -= x;
            s-> y -= y;
        }

        break;
        }
#endif // POLYMER
    }

    if (A_CheckSpriteTileFlags(atwith,SPRITE_PROJECTILE))
    {
        /* Custom projectiles.  This is a big hack. */

#ifdef POLYMER
        if (ProjectileData[atwith].flashcolor)
        {
            int32_t x = ((sintable[(s->ang+512)&2047])>>7), y = ((sintable[(s->ang)&2047])>>7);

            s-> x += x;
            s-> y += y;
            G_AddGameLight(0, i, PHEIGHT, 8192, ProjectileData[atwith].flashcolor,PR_LIGHT_PRIO_MAX_GAME);
            actor[i].lightcount = 2;
            s-> x -= x;
            s-> y -= y;
        }
#endif // POLYMER

        if (ProjectileData[atwith].offset == 0) ProjectileData[atwith].offset = 1;

        if (ProjectileData[atwith].workslike & PROJECTILE_BLOOD || ProjectileData[atwith].workslike & PROJECTILE_KNEE)
        {

            if (ProjectileData[atwith].workslike & PROJECTILE_BLOOD)
            {
                sa += 64 - (krand()&127);
                if (p < 0) sa += 1024;
                zvel = 1024-(krand()&2047);
            }

            if (ProjectileData[atwith].workslike & PROJECTILE_KNEE)
            {
                if (p >= 0)
                {
                    zvel = (100-g_player[p].ps->horiz-g_player[p].ps->horizoff)<<5;
                    srcvect.z += (6<<8);
                    sa += 15;
                }
                else if (!(ProjectileData[atwith].workslike & PROJECTILE_NOAIM))
                {
                    j = g_player[A_FindPlayer(s,&x)].ps->i;
                    zvel = ((sprite[j].z-srcvect.z)<<8) / (x+1);
                    sa = getangle(sprite[j].x-srcvect.x,sprite[j].y-srcvect.y);
                }
            }

            if (actor[i].shootzvel) zvel = actor[i].shootzvel;
            hitscan((const vec3_t *)&srcvect,sect,
                    sintable[(sa+512)&2047],
                    sintable[sa&2047],zvel<<6,
                    &hitinfo,CLIPMASK1);

            if (ProjectileData[atwith].workslike & PROJECTILE_BLOOD)
            {
                if (ProjectileData[atwith].range == 0)
                    ProjectileData[atwith].range = 1024;

                if (FindDistance2D(srcvect.x-hitinfo.pos.x,srcvect.y-hitinfo.pos.y) < ProjectileData[atwith].range)
                    if (FindDistance2D(wall[hitinfo.hitwall].x-wall[wall[hitinfo.hitwall].point2].x,wall[hitinfo.hitwall].y-wall[wall[hitinfo.hitwall].point2].y) >
                            (mulscale(ProjectileData[atwith].xrepeat+8,tilesizx[ProjectileData[atwith].decal],3)))
                        if (hitinfo.hitwall >= 0 && wall[hitinfo.hitwall].overpicnum != BIGFORCE)
                            if ((wall[hitinfo.hitwall].nextsector >= 0 && hitinfo.hitsect >= 0 &&
                                    sector[wall[hitinfo.hitwall].nextsector].lotag == 0 &&
                                    sector[hitinfo.hitsect].lotag == 0 &&
                                    sector[wall[hitinfo.hitwall].nextsector].lotag == 0 &&
                                    (sector[hitinfo.hitsect].floorz-sector[wall[hitinfo.hitwall].nextsector].floorz) >
                                    (mulscale(ProjectileData[atwith].yrepeat,tilesizy[ProjectileData[atwith].decal],3)<<8)) ||
                                    (wall[hitinfo.hitwall].nextsector == -1 && sector[hitinfo.hitsect].lotag == 0))
                                if ((wall[hitinfo.hitwall].cstat&16) == 0)
                                {
                                    if (wall[hitinfo.hitwall].nextsector >= 0)
                                    {
                                        k = headspritesect[wall[hitinfo.hitwall].nextsector];
                                        while (k >= 0)
                                        {
                                            if (sprite[k].statnum == 3 && sprite[k].lotag == 13)
                                                return -1;
                                            k = nextspritesect[k];
                                        }
                                    }

                                    if (wall[hitinfo.hitwall].nextwall >= 0 &&
                                            wall[wall[hitinfo.hitwall].nextwall].hitag != 0)
                                        return -1;

                                    if (wall[hitinfo.hitwall].hitag == 0)
                                    {
                                        if (ProjectileData[atwith].decal >= 0)
                                        {
                                            k = A_Spawn(i,ProjectileData[atwith].decal);

                                            if (!A_CheckSpriteFlags(k , SPRITE_DECAL))
                                                actor[k].flags |= SPRITE_DECAL;

                                            sprite[k].xvel = -1;
                                            sprite[k].ang = getangle(wall[hitinfo.hitwall].x-wall[wall[hitinfo.hitwall].point2].x,
                                                                     wall[hitinfo.hitwall].y-wall[wall[hitinfo.hitwall].point2].y)+512;
                                            Bmemcpy(&sprite[k],&hitinfo.pos,sizeof(vec3_t));
                                            /*
                                                                                        sprite[k].x = hitinfo.pos.x;
                                                                                        sprite[k].y = hitinfo.pos.y;
                                                                                        sprite[k].z = hitinfo.pos.z;
                                            */
                                            if (ProjectileData[atwith].workslike & PROJECTILE_RANDDECALSIZE)
                                            {
                                                int32_t wh = (krand()&ProjectileData[atwith].xrepeat);
                                                if (wh < ProjectileData[atwith].yrepeat)
                                                    wh = ProjectileData[atwith].yrepeat;
                                                sprite[k].xrepeat = wh;
                                                sprite[k].yrepeat = wh;
                                            }
                                            else
                                            {
                                                sprite[k].xrepeat = ProjectileData[atwith].xrepeat;
                                                sprite[k].yrepeat = ProjectileData[atwith].yrepeat;
                                            }
                                            sprite[k].z += sprite[k].yrepeat<<8;
                                            //                                        sprite[k].cstat = 16+(krand()&12);
                                            sprite[k].cstat = 16;

                                            {
                                                int32_t wh = (krand()&1);
                                                if (wh == 1)
                                                    sprite[k].cstat |= 4;

                                                wh = (krand()&1);
                                                if (wh == 1)
                                                    sprite[k].cstat |= 8;

                                                wh = sprite[k].sectnum;
                                                sprite[k].shade = sector[wh].floorshade;
                                            }
                                            sprite[k].x -= mulscale13(1,sintable[(sprite[k].ang+2560)&2047]);
                                            sprite[k].y -= mulscale13(1,sintable[(sprite[k].ang+2048)&2047]);

                                            A_SetSprite(k,CLIPMASK0);
                                            A_AddToDeleteQueue(k);
                                            changespritestat(k,5);

                                        }
                                        //                                    if( PN == OOZFILTER || PN == NEWBEAST )
                                        //                                        sprite[k].pal = 6;
                                    }
                                }
                return -1;
            }

            if (hitinfo.hitsect < 0) return -1;

            if ((ProjectileData[atwith].range == 0) && (ProjectileData[atwith].workslike & PROJECTILE_KNEE))
                ProjectileData[atwith].range = 1024;

            if ((ProjectileData[atwith].range > 0) && ((klabs(srcvect.x-hitinfo.pos.x)+klabs(srcvect.y-hitinfo.pos.y)) > ProjectileData[atwith].range))
                return -1;
            else
            {
                if (hitinfo.hitwall >= 0 || hitinfo.hitsprite >= 0)
                {
                    j = A_InsertSprite(hitinfo.hitsect,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,atwith,-15,0,0,sa,32,0,i,4);
                    SpriteProjectile[j].workslike = ProjectileData[sprite[j].picnum].workslike;
                    sprite[j].extra = ProjectileData[atwith].extra;
                    if (ProjectileData[atwith].extra_rand > 0)
                        sprite[j].extra += (krand()&ProjectileData[atwith].extra_rand);
                    if (p >= 0)
                    {
                        if (ProjectileData[atwith].spawns >= 0)
                        {
                            k = A_Spawn(j,ProjectileData[atwith].spawns);
                            sprite[k].z -= (8<<8);
                            actor[k].t_data[6] = hitinfo.hitwall;
                            actor[k].t_data[7] = hitinfo.hitsect;
                            actor[k].t_data[8] = hitinfo.hitsprite;
                        }
                        if (ProjectileData[atwith].sound >= 0) A_PlaySound(ProjectileData[atwith].sound,j);
                    }

                    if (p >= 0 && g_player[p].ps->inv_amount[GET_STEROIDS] > 0 && g_player[p].ps->inv_amount[GET_STEROIDS] < 400)
                        sprite[j].extra += (g_player[p].ps->max_player_health>>2);

                    if (hitinfo.hitsprite >= 0 && sprite[hitinfo.hitsprite].picnum != ACCESSSWITCH && sprite[hitinfo.hitsprite].picnum != ACCESSSWITCH2)
                    {
                        A_DamageObject(hitinfo.hitsprite,j);
                        if (p >= 0) P_ActivateSwitch(p,hitinfo.hitsprite,1);
                    }

                    else if (hitinfo.hitwall >= 0)
                    {
                        if (wall[hitinfo.hitwall].cstat&2)
                            if (wall[hitinfo.hitwall].nextsector >= 0)
                                if (hitinfo.pos.z >= (sector[wall[hitinfo.hitwall].nextsector].floorz))
                                    hitinfo.hitwall = wall[hitinfo.hitwall].nextwall;

                        if (hitinfo.hitwall >= 0 && wall[hitinfo.hitwall].picnum != ACCESSSWITCH && wall[hitinfo.hitwall].picnum != ACCESSSWITCH2)
                        {
                            A_DamageWall(j,hitinfo.hitwall,&hitinfo.pos,atwith);
                            if (p >= 0) P_ActivateSwitch(p,hitinfo.hitwall,0);
                        }
                    }
                }
                else if (p >= 0 && zvel > 0 && sector[hitinfo.hitsect].lotag == 1)
                {
                    j = A_Spawn(g_player[p].ps->i,WATERSPLASH2);
                    sprite[j].x = hitinfo.pos.x;
                    sprite[j].y = hitinfo.pos.y;
                    sprite[j].ang = g_player[p].ps->ang; // Total tweek
                    sprite[j].xvel = 32;
                    A_SetSprite(i,CLIPMASK0);
                    sprite[j].xvel = 0;

                }
            }
            return -1;
        }

        if (ProjectileData[atwith].workslike & PROJECTILE_HITSCAN)
        {
            if (s->extra >= 0) s->shade = ProjectileData[atwith].shade;

            if (p >= 0)
            {
                int32_t angRange=32;
                int32_t zRange=256;

                Gv_SetVar(g_iAimAngleVarID,AUTO_AIM_ANGLE,i,p);
                if (apScriptGameEvent[EVENT_GETAUTOAIMANGLE])
                    VM_OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
                j=-1;
                if (Gv_GetVar(g_iAimAngleVarID,i,p) > 0)
                {
                    j = A_FindTargetSprite(s, Gv_GetVar(g_iAimAngleVarID,i,p),atwith);
                }
                if (j >= 0)
                {
                    dal = ((sprite[j].yrepeat*tilesizy[sprite[j].picnum])<<1)+(5<<8);
                    if (((sprite[j].picnum >= GREENSLIME)&&(sprite[j].picnum <= GREENSLIME+7))||(sprite[j].picnum ==ROTATEGUN))
                    {
                        dal -= (8<<8);
                        //                        return -1;
                    }
                    hitinfo.pos.x = ldist(&sprite[g_player[p].ps->i], &sprite[j]);
                    if (hitinfo.pos.x == 0) hitinfo.pos.x++;
                    zvel = ((sprite[j].z-srcvect.z-dal)<<8) / hitinfo.pos.x;
                    sa = getangle(sprite[j].x-srcvect.x,sprite[j].y-srcvect.y);
                }

                Gv_SetVar(g_iAngRangeVarID,angRange, i,p);
                Gv_SetVar(g_iZRangeVarID,zRange,i,p);
                VM_OnEvent(EVENT_GETSHOTRANGE, i,p, -1);
                angRange=Gv_GetVar(g_iAngRangeVarID,i,p);
                zRange=Gv_GetVar(g_iZRangeVarID,i,p);

                if (ProjectileData[atwith].workslike & PROJECTILE_ACCURATE_AUTOAIM)
                {
                    if (!g_player[p].ps->auto_aim)
                    {
                        zvel = (100-g_player[p].ps->horiz-g_player[p].ps->horizoff)<<5;
                        if (actor[i].shootzvel) zvel = actor[i].shootzvel;
                        hitscan((const vec3_t *)&srcvect,sect,sintable[(sa+512)&2047],sintable[sa&2047],
                                zvel<<6,&hitinfo,CLIPMASK1);
                        if (hitinfo.hitsprite != -1)
                        {
                            if (sprite[hitinfo.hitsprite].statnum == STAT_ACTOR || sprite[hitinfo.hitsprite].statnum == STAT_ZOMBIEACTOR || sprite[hitinfo.hitsprite].statnum == STAT_PLAYER || sprite[hitinfo.hitsprite].statnum == STAT_DUMMYPLAYER)
                                j = hitinfo.hitsprite;
                        }
                    }

                    if (j == -1)
                    {
                        zvel = (100-g_player[p].ps->horiz-g_player[p].ps->horizoff)<<5;
                        if (!(ProjectileData[atwith].workslike & PROJECTILE_ACCURATE))
                        {
                            sa += (angRange/2)-(krand()&(angRange-1));
                            zvel += (zRange/2)-(krand()&(zRange-1));
                        }
                    }
                }
                else
                {
                    if (j == -1)
                    {
                        // no target
                        zvel = (100-g_player[p].ps->horiz-g_player[p].ps->horizoff)<<5;
                    }
                    if (!(ProjectileData[atwith].workslike & PROJECTILE_ACCURATE))
                    {
                        sa += (angRange/2)-(krand()&(angRange-1));
                        zvel += (zRange/2)-(krand()&(zRange-1));
                    }
                }
                srcvect.z -= (2<<8);
            }
            else
            {
                j = A_FindPlayer(s,&x);
                srcvect.z -= (4<<8);
                hitinfo.pos.x = ldist(&sprite[g_player[j].ps->i], s);
                if (hitinfo.pos.x == 0)
                    hitinfo.pos.x++;
                zvel = ((g_player[j].ps->pos.z-srcvect.z) <<8) / hitinfo.pos.x;
                if (s->picnum != BOSS1)
                {
                    if (!(ProjectileData[atwith].workslike & PROJECTILE_ACCURATE))
                    {
                        zvel += 128-(krand()&255);
                        sa += 32-(krand()&63);
                    }
                }
                else
                {
                    sa = getangle(g_player[j].ps->pos.x-srcvect.x,g_player[j].ps->pos.y-srcvect.y);

                    if (!(ProjectileData[atwith].workslike & PROJECTILE_ACCURATE))
                    {
                        zvel += 128-(krand()&255);
                        sa += 64-(krand()&127);
                    }
                }
            }

            if (ProjectileData[atwith].cstat >= 0) s->cstat &= ~ProjectileData[atwith].cstat;
            else s->cstat &= ~257;

            if (actor[i].shootzvel) zvel = actor[i].shootzvel;
            hitscan((const vec3_t *)&srcvect,sect,
                    sintable[(sa+512)&2047],
                    sintable[sa&2047],
                    zvel<<6,&hitinfo,CLIPMASK1);


            if (ProjectileData[atwith].cstat >= 0) s->cstat |= ProjectileData[atwith].cstat;
            else s->cstat |= 257;

            if (hitinfo.hitsect < 0) return -1;

            if ((ProjectileData[atwith].range > 0) &&
                    ((klabs(srcvect.x-hitinfo.pos.x)+klabs(srcvect.y-hitinfo.pos.y)) > ProjectileData[atwith].range))
                return -1;

            if (ProjectileData[atwith].trail >= 0)
                A_HitscanProjTrail(&srcvect,&hitinfo.pos,sa,atwith);

            if (ProjectileData[atwith].workslike & PROJECTILE_WATERBUBBLES)
            {
                if ((krand()&15) == 0 && sector[hitinfo.hitsect].lotag == 2)
                    A_DoWaterTracers(hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,srcvect.x,srcvect.y,srcvect.z,8-(ud.multimode>>1));
            }

            if (p >= 0)
            {
                k = A_InsertSprite(hitinfo.hitsect,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,SHOTSPARK1,-15,10,10,sa,0,0,i,4);
                sprite[k].extra = ProjectileData[atwith].extra;
                if (ProjectileData[atwith].extra_rand > 0)
                    sprite[k].extra += (krand()%ProjectileData[atwith].extra_rand);
                sprite[k].yvel = atwith; // this is a hack to allow you to detect which weapon spawned a SHOTSPARK1
                actor[k].t_data[6] = hitinfo.hitwall;
                actor[k].t_data[7] = hitinfo.hitsect;
                actor[k].t_data[8] = hitinfo.hitsprite;

                if (hitinfo.hitwall == -1 && hitinfo.hitsprite == -1)
                {
                    if (zvel < 0)
                    {
                        if (sector[hitinfo.hitsect].ceilingstat&1)
                        {
                            sprite[k].xrepeat = 0;
                            sprite[k].yrepeat = 0;
                            return -1;
                        }
                        else
                            Sect_DamageCeiling(hitinfo.hitsect);
                    }
                    if (ProjectileData[atwith].spawns >= 0)
                    {
                        int32_t wh=A_Spawn(k,ProjectileData[atwith].spawns);
                        if (ProjectileData[atwith].sxrepeat > 4) sprite[wh].xrepeat=ProjectileData[atwith].sxrepeat;
                        if (ProjectileData[atwith].syrepeat > 4) sprite[wh].yrepeat=ProjectileData[atwith].syrepeat;
                        actor[wh].t_data[6] = hitinfo.hitwall;
                        actor[wh].t_data[7] = hitinfo.hitsect;
                        actor[wh].t_data[8] = hitinfo.hitsprite;
                    }
                }

                if (hitinfo.hitsprite >= 0)
                {
                    A_DamageObject(hitinfo.hitsprite,k);
                    if (sprite[hitinfo.hitsprite].picnum == APLAYER &&
                            (ud.ffire == 1 || (!GTFLAGS(GAMETYPE_PLAYERSFRIENDLY) && GTFLAGS(GAMETYPE_TDM) &&
                                               g_player[sprite[hitinfo.hitsprite].yvel].ps->team != g_player[sprite[i].yvel].ps->team)))
                    {
                        l = A_Spawn(k,JIBS6);
                        sprite[k].xrepeat = sprite[k].yrepeat = 0;
                        sprite[l].z += (4<<8);
                        sprite[l].xvel = 16;
                        sprite[l].xrepeat = sprite[l].yrepeat = 24;
                        sprite[l].ang += 64-(krand()&127);
                    }
                    else
                    {
                        if (ProjectileData[atwith].spawns >= 0)
                        {
                            int32_t wh=A_Spawn(k,ProjectileData[atwith].spawns);
                            if (ProjectileData[atwith].sxrepeat > 4) sprite[wh].xrepeat=ProjectileData[atwith].sxrepeat;
                            if (ProjectileData[atwith].syrepeat > 4) sprite[wh].yrepeat=ProjectileData[atwith].syrepeat;
                            actor[wh].t_data[6] = hitinfo.hitwall;
                            actor[wh].t_data[7] = hitinfo.hitsect;
                            actor[wh].t_data[8] = hitinfo.hitsprite;
                        }
                    }
                    if (p >= 0 && (
                                sprite[hitinfo.hitsprite].picnum == DIPSWITCH ||
                                sprite[hitinfo.hitsprite].picnum == DIPSWITCH+1 ||
                                sprite[hitinfo.hitsprite].picnum == DIPSWITCH2 ||
                                sprite[hitinfo.hitsprite].picnum == DIPSWITCH2+1 ||
                                sprite[hitinfo.hitsprite].picnum == DIPSWITCH3 ||
                                sprite[hitinfo.hitsprite].picnum == DIPSWITCH3+1 ||
                                sprite[hitinfo.hitsprite].picnum == HANDSWITCH ||
                                sprite[hitinfo.hitsprite].picnum == HANDSWITCH+1))
                    {
                        P_ActivateSwitch(p,hitinfo.hitsprite,1);
                        return -1;
                    }
                }
                else if (hitinfo.hitwall >= 0)
                {
                    if (ProjectileData[atwith].spawns >= 0)
                    {
                        int32_t wh=A_Spawn(k,ProjectileData[atwith].spawns);
                        if (ProjectileData[atwith].sxrepeat > 4) sprite[wh].xrepeat=ProjectileData[atwith].sxrepeat;
                        if (ProjectileData[atwith].syrepeat > 4) sprite[wh].yrepeat=ProjectileData[atwith].syrepeat;
                        actor[wh].t_data[6] = hitinfo.hitwall;
                        actor[wh].t_data[7] = hitinfo.hitsect;
                        actor[wh].t_data[8] = hitinfo.hitsprite;
                    }
                    if (CheckDoorTile(wall[hitinfo.hitwall].picnum) == 1)
                        goto DOSKIPBULLETHOLE;
                    if (p >= 0 && (
                                wall[hitinfo.hitwall].picnum == DIPSWITCH ||
                                wall[hitinfo.hitwall].picnum == DIPSWITCH+1 ||
                                wall[hitinfo.hitwall].picnum == DIPSWITCH2 ||
                                wall[hitinfo.hitwall].picnum == DIPSWITCH2+1 ||
                                wall[hitinfo.hitwall].picnum == DIPSWITCH3 ||
                                wall[hitinfo.hitwall].picnum == DIPSWITCH3+1 ||
                                wall[hitinfo.hitwall].picnum == HANDSWITCH ||
                                wall[hitinfo.hitwall].picnum == HANDSWITCH+1))
                    {
                        P_ActivateSwitch(p,hitinfo.hitwall,0);
                        return -1;
                    }

                    if (wall[hitinfo.hitwall].hitag != 0 || (wall[hitinfo.hitwall].nextwall >= 0 && wall[wall[hitinfo.hitwall].nextwall].hitag != 0))
                        goto DOSKIPBULLETHOLE;

                    if (hitinfo.hitsect >= 0 && sector[hitinfo.hitsect].lotag == 0)
                        if (wall[hitinfo.hitwall].overpicnum != BIGFORCE)
                            if ((wall[hitinfo.hitwall].nextsector >= 0 && sector[wall[hitinfo.hitwall].nextsector].lotag == 0) ||
                                    (wall[hitinfo.hitwall].nextsector == -1 && sector[hitinfo.hitsect].lotag == 0))
                                if ((wall[hitinfo.hitwall].cstat&16) == 0)
                                {
                                    if (wall[hitinfo.hitwall].nextsector >= 0)
                                    {
                                        l = headspritesect[wall[hitinfo.hitwall].nextsector];
                                        while (l >= 0)
                                        {
                                            if (sprite[l].statnum == 3 && sprite[l].lotag == 13)
                                                goto DOSKIPBULLETHOLE;
                                            l = nextspritesect[l];
                                        }
                                    }

                                    l = headspritestat[STAT_MISC];
                                    while (l >= 0)
                                    {
                                        if (sprite[l].picnum == ProjectileData[atwith].decal)
                                            if (dist(&sprite[l],&sprite[k]) < (12+(krand()&7)))
                                                goto DOSKIPBULLETHOLE;
                                        l = nextspritestat[l];
                                    }
                                    if (ProjectileData[atwith].decal >= 0)
                                    {
                                        l = A_Spawn(k,ProjectileData[atwith].decal);

                                        if (!A_CheckSpriteFlags(l , SPRITE_DECAL))
                                            actor[l].flags |= SPRITE_DECAL;

                                        sprite[l].xvel = -1;
                                        sprite[l].ang = getangle(wall[hitinfo.hitwall].x-wall[wall[hitinfo.hitwall].point2].x,
                                                                 wall[hitinfo.hitwall].y-wall[wall[hitinfo.hitwall].point2].y)+512;
                                        if (ProjectileData[atwith].workslike & PROJECTILE_RANDDECALSIZE)
                                        {
                                            int32_t wh = (krand()&ProjectileData[atwith].xrepeat);
                                            if (wh < ProjectileData[atwith].yrepeat)
                                                wh = ProjectileData[atwith].yrepeat;
                                            sprite[l].xrepeat = wh;
                                            sprite[l].yrepeat = wh;
                                        }
                                        else
                                        {
                                            sprite[l].xrepeat = ProjectileData[atwith].xrepeat;
                                            sprite[l].yrepeat = ProjectileData[atwith].yrepeat;
                                        }
                                        sprite[l].cstat = 16+(krand()&12);
                                        sprite[l].x -= mulscale13(1,sintable[(sprite[l].ang+2560)&2047]);
                                        sprite[l].y -= mulscale13(1,sintable[(sprite[l].ang+2048)&2047]);

                                        A_SetSprite(l,CLIPMASK0);
                                        A_AddToDeleteQueue(l);
                                    }
                                }

DOSKIPBULLETHOLE:

                    if (wall[hitinfo.hitwall].cstat&2)
                        if (wall[hitinfo.hitwall].nextsector >= 0)
                            if (hitinfo.pos.z >= (sector[wall[hitinfo.hitwall].nextsector].floorz))
                                hitinfo.hitwall = wall[hitinfo.hitwall].nextwall;

                    A_DamageWall(k,hitinfo.hitwall,&hitinfo.pos,atwith);
                }
            }
            else
            {
                k = A_InsertSprite(hitinfo.hitsect,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,SHOTSPARK1,-15,24,24,sa,0,0,i,4);
                sprite[k].extra = ProjectileData[atwith].extra;
                if (ProjectileData[atwith].extra_rand > 0)
                    sprite[k].extra += (krand()%ProjectileData[atwith].extra_rand);
                sprite[k].yvel = atwith; // this is a hack to allow you to detect which weapon spawned a SHOTSPARK1
                actor[k].t_data[6] = hitinfo.hitwall;
                actor[k].t_data[7] = hitinfo.hitsect;
                actor[k].t_data[8] = hitinfo.hitsprite;

                if (hitinfo.hitsprite >= 0)
                {
                    A_DamageObject(hitinfo.hitsprite,k);
                    if (sprite[hitinfo.hitsprite].picnum != APLAYER)
                    {
                        if (ProjectileData[atwith].spawns >= 0)
                        {
                            int32_t wh=A_Spawn(k,ProjectileData[atwith].spawns);
                            if (ProjectileData[atwith].sxrepeat > 4) sprite[wh].xrepeat=ProjectileData[atwith].sxrepeat;
                            if (ProjectileData[atwith].syrepeat > 4) sprite[wh].yrepeat=ProjectileData[atwith].syrepeat;
                            actor[wh].t_data[6] = hitinfo.hitwall;
                            actor[wh].t_data[7] = hitinfo.hitsect;
                            actor[wh].t_data[8] = hitinfo.hitsprite;
                        }
                    }
                    else sprite[k].xrepeat = sprite[k].yrepeat = 0;
                }
                else if (hitinfo.hitwall >= 0)
                    A_DamageWall(k,hitinfo.hitwall,&hitinfo.pos,atwith);
            }

            if ((krand()&255) < 4 && ProjectileData[atwith].isound >= 0)
                S_PlaySound3D(ProjectileData[atwith].isound,k,&hitinfo.pos);

            return -1;
        }

        if (ProjectileData[atwith].workslike & PROJECTILE_RPG)
        {

            /*            if(ProjectileData[atwith].workslike & PROJECTILE_FREEZEBLAST)
            sz += (3<<8);*/

            if (s->extra >= 0) s->shade = ProjectileData[atwith].shade;

            vel = ProjectileData[atwith].vel;

            j = -1;

            if (p >= 0)
            {
                //            j = A_FindTargetSprite( s, AUTO_AIM_ANGLE ); // 48
                Gv_SetVar(g_iAimAngleVarID,AUTO_AIM_ANGLE,i,p);
                if (apScriptGameEvent[EVENT_GETAUTOAIMANGLE])
                    VM_OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
                j=-1;

                if (Gv_GetVar(g_iAimAngleVarID,i,p) > 0)
                    j = A_FindTargetSprite(s, Gv_GetVar(g_iAimAngleVarID,i,p),atwith);

                if (j >= 0)
                {
                    dal = ((sprite[j].yrepeat*tilesizy[sprite[j].picnum])<<1)+(8<<8);
                    hitinfo.pos.x = ldist(&sprite[g_player[p].ps->i], &sprite[j]);

                    if (hitinfo.pos.x == 0)
                        hitinfo.pos.x++;

                    zvel = ((sprite[j].z-srcvect.z-dal)*vel) / hitinfo.pos.x;

                    if (sprite[j].picnum != RECON)
                        sa = getangle(sprite[j].x-srcvect.x,sprite[j].y-srcvect.y);
                }
                //                else zvel = (100-g_player[p].ps->horiz-g_player[p].ps->horizoff)*81;
                else zvel = (100-g_player[p].ps->horiz-g_player[p].ps->horizoff)*(ProjectileData[atwith].vel/8);

                if (ProjectileData[atwith].sound >= 0)
                    A_PlaySound(ProjectileData[atwith].sound,i);
            }
            else
            {
                if (!(ProjectileData[atwith].workslike & PROJECTILE_NOAIM))
                {
                    j = A_FindPlayer(s,&x);
                    sa = getangle(g_player[j].ps->opos.x-srcvect.x,g_player[j].ps->opos.y-srcvect.y);

                    l = ldist(&sprite[g_player[j].ps->i],s);
                    if (l == 0)
                        l++;
                    zvel = ((g_player[j].ps->opos.z-srcvect.z)*vel) / l;

                    if (A_CheckEnemySprite(s) && (s->hitag&face_player_smart))
                        sa = s->ang+(krand()&31)-16;
                }
            }



            if (p >= 0 && j >= 0)
                l = j;
            else l = -1;

            if (numplayers > 1 && g_netClient) return -1;

            /*                        j = A_InsertSprite(sect,
            sx+(sintable[(348+sa+512)&2047]/448),
            sy+(sintable[(sa+348)&2047]/448),
            sz-(1<<8),atwith,0,14,14,sa,vel,zvel,i,4);*/
            if (actor[i].shootzvel) zvel = actor[i].shootzvel;
            j = A_InsertSprite(sect,
                               srcvect.x+(sintable[(348+sa+512)&2047]/ProjectileData[atwith].offset),
                               srcvect.y+(sintable[(sa+348)&2047]/ProjectileData[atwith].offset),
                               srcvect.z-(1<<8),atwith,0,14,14,sa,vel,zvel,i,4);

            sprite[j].xrepeat=ProjectileData[atwith].xrepeat;
            sprite[j].yrepeat=ProjectileData[atwith].yrepeat;


            if (ProjectileData[atwith].extra_rand > 0)
                sprite[j].extra += (krand()&ProjectileData[atwith].extra_rand);
            if (!(ProjectileData[atwith].workslike & PROJECTILE_BOUNCESOFFWALLS))
                sprite[j].yvel = l;
            else
            {
                if (ProjectileData[atwith].bounces >= 1) sprite[j].yvel = ProjectileData[atwith].bounces;
                else sprite[j].yvel = g_numFreezeBounces;
                //                sprite[j].xrepeat >>= 1;
                //                sprite[j].yrepeat >>= 1;
                sprite[j].zvel -= (2<<4);
            }
            /*
            if(p == -1)
            {
            if(!(ProjectileData[atwith].workslike & PROJECTILE_BOUNCESOFFWALLS))
            {
            sprite[j].xrepeat = ProjectileData[atwith].xrepeat; // 30
            sprite[j].yrepeat = ProjectileData[atwith].yrepeat;
            sprite[j].extra >>= 2;
            }
            }
            */
            if (ProjectileData[atwith].cstat >= 0) sprite[j].cstat = ProjectileData[atwith].cstat;
            else sprite[j].cstat = 128;
            if (ProjectileData[atwith].clipdist != 255) sprite[j].clipdist = ProjectileData[atwith].clipdist;
            else sprite[j].clipdist = 40;

            Bmemcpy(&SpriteProjectile[j], &ProjectileData[sprite[j].picnum], sizeof(ProjectileData[sprite[j].picnum]));

            //            sa = s->ang+32-(krand()&63);
            //            zvel = oldzvel+512-(krand()&1023);

            return j;
        }

    }
    else
    {
        switch (DynamicTileMap[atwith])
        {
        case BLOODSPLAT1__STATIC:
        case BLOODSPLAT2__STATIC:
        case BLOODSPLAT3__STATIC:
        case BLOODSPLAT4__STATIC:
            sa += 64 - (krand()&127);
            if (p < 0) sa += 1024;
            zvel = 1024-(krand()&2047);
        case KNEE__STATIC:
            if (atwith == KNEE)
            {
                if (p >= 0)
                {
                    zvel = (100-g_player[p].ps->horiz-g_player[p].ps->horizoff)<<5;
                    srcvect.z += (6<<8);
                    sa += 15;
                }
                else
                {
                    j = g_player[A_FindPlayer(s,&x)].ps->i;
                    zvel = ((sprite[j].z-srcvect.z)<<8) / (x+1);
                    sa = getangle(sprite[j].x-srcvect.x,sprite[j].y-srcvect.y);
                }
            }

            if (actor[i].shootzvel) zvel = actor[i].shootzvel;
            hitscan((const vec3_t *)&srcvect,sect,
                    sintable[(sa+512)&2047],
                    sintable[sa&2047],zvel<<6,
                    &hitinfo,CLIPMASK1);

            if (atwith >= BLOODSPLAT1 && atwith <= BLOODSPLAT4)
            {
                if (FindDistance2D(srcvect.x-hitinfo.pos.x,srcvect.y-hitinfo.pos.y) < 1024)
                    if (hitinfo.hitwall >= 0 && wall[hitinfo.hitwall].overpicnum != BIGFORCE)
                        if ((wall[hitinfo.hitwall].nextsector >= 0 && hitinfo.hitsect >= 0 &&
                                sector[wall[hitinfo.hitwall].nextsector].lotag == 0 &&
                                sector[hitinfo.hitsect].lotag == 0 &&
                                sector[wall[hitinfo.hitwall].nextsector].lotag == 0 &&
                                (sector[hitinfo.hitsect].floorz-sector[wall[hitinfo.hitwall].nextsector].floorz) > (16<<8)) ||
                                (wall[hitinfo.hitwall].nextsector == -1 && sector[hitinfo.hitsect].lotag == 0))
                            if ((wall[hitinfo.hitwall].cstat&16) == 0)
                            {
                                if (wall[hitinfo.hitwall].nextsector >= 0)
                                {
                                    k = headspritesect[wall[hitinfo.hitwall].nextsector];
                                    while (k >= 0)
                                    {
                                        if (sprite[k].statnum == 3 && sprite[k].lotag == 13)
                                            return -1;
                                        k = nextspritesect[k];
                                    }
                                }

                                if (wall[hitinfo.hitwall].nextwall >= 0 &&
                                        wall[wall[hitinfo.hitwall].nextwall].hitag != 0)
                                    return -1;

                                if (wall[hitinfo.hitwall].hitag == 0)
                                {
                                    k = A_Spawn(i,atwith);
                                    sprite[k].xvel = -12;
                                    sprite[k].ang = getangle(wall[hitinfo.hitwall].x-wall[wall[hitinfo.hitwall].point2].x,
                                                             wall[hitinfo.hitwall].y-wall[wall[hitinfo.hitwall].point2].y)+512;
                                    sprite[k].x = hitinfo.pos.x;
                                    sprite[k].y = hitinfo.pos.y;
                                    sprite[k].z = hitinfo.pos.z;
                                    sprite[k].cstat |= (krand()&4);
                                    A_SetSprite(k,CLIPMASK0);
                                    setsprite(k,(vec3_t *)&sprite[k]);
                                    if (PN == OOZFILTER || PN == NEWBEAST)
                                        sprite[k].pal = 6;
                                }
                            }
                return -1;
            }

            if (hitinfo.hitsect < 0) break;

            if ((klabs(srcvect.x-hitinfo.pos.x)+klabs(srcvect.y-hitinfo.pos.y)) < 1024)
            {
                if (hitinfo.hitwall >= 0 || hitinfo.hitsprite >= 0)
                {
                    j = A_InsertSprite(hitinfo.hitsect,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,KNEE,-15,0,0,sa,32,0,i,4);
                    sprite[j].extra += (krand()&7);
                    if (p >= 0)
                    {
                        k = A_Spawn(j,SMALLSMOKE);
                        sprite[k].z -= (8<<8);
                        A_PlaySound(KICK_HIT,j);
                        actor[k].t_data[6] = hitinfo.hitwall;
                        actor[k].t_data[7] = hitinfo.hitsect;
                        actor[k].t_data[8] = hitinfo.hitsprite;
                    }

                    if (p >= 0 && g_player[p].ps->inv_amount[GET_STEROIDS] > 0 && g_player[p].ps->inv_amount[GET_STEROIDS] < 400)
                        sprite[j].extra += (g_player[p].ps->max_player_health>>2);

                    if (hitinfo.hitsprite >= 0 && sprite[hitinfo.hitsprite].picnum != ACCESSSWITCH && sprite[hitinfo.hitsprite].picnum != ACCESSSWITCH2)
                    {
                        A_DamageObject(hitinfo.hitsprite,j);
                        if (p >= 0) P_ActivateSwitch(p,hitinfo.hitsprite,1);
                    }

                    else if (hitinfo.hitwall >= 0)
                    {
                        if (wall[hitinfo.hitwall].cstat&2)
                            if (wall[hitinfo.hitwall].nextsector >= 0)
                                if (hitinfo.pos.z >= (sector[wall[hitinfo.hitwall].nextsector].floorz))
                                    hitinfo.hitwall = wall[hitinfo.hitwall].nextwall;

                        if (hitinfo.hitwall >= 0 && wall[hitinfo.hitwall].picnum != ACCESSSWITCH && wall[hitinfo.hitwall].picnum != ACCESSSWITCH2)
                        {
                            A_DamageWall(j,hitinfo.hitwall,&hitinfo.pos,atwith);
                            if (p >= 0) P_ActivateSwitch(p,hitinfo.hitwall,0);
                        }
                    }
                }
                else if (p >= 0 && zvel > 0 && sector[hitinfo.hitsect].lotag == 1)
                {
                    j = A_Spawn(g_player[p].ps->i,WATERSPLASH2);
                    sprite[j].x = hitinfo.pos.x;
                    sprite[j].y = hitinfo.pos.y;
                    sprite[j].ang = g_player[p].ps->ang; // Total tweek
                    sprite[j].xvel = 32;
                    A_SetSprite(i,CLIPMASK0);
                    sprite[j].xvel = 0;

                }
            }
            break;

        case SHOTSPARK1__STATIC:
        case SHOTGUN__STATIC:
        case CHAINGUN__STATIC:

            if (s->extra >= 0) s->shade = -96;

            if (p >= 0)
            {
                int32_t angRange=32;
                int32_t zRange=256;

                Gv_SetVar(g_iAimAngleVarID,AUTO_AIM_ANGLE,i,p);
                if (apScriptGameEvent[EVENT_GETAUTOAIMANGLE])
                    VM_OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
                j=-1;
                if (Gv_GetVar(g_iAimAngleVarID,i,p) > 0)
                {
                    j = A_FindTargetSprite(s, Gv_GetVar(g_iAimAngleVarID,i,p),atwith);
                }
                if (j >= 0)
                {
                    dal = ((sprite[j].yrepeat*tilesizy[sprite[j].picnum])<<1)+(5<<8);
                    if (((sprite[j].picnum>=GREENSLIME)&&(sprite[j].picnum<=GREENSLIME+7))||(sprite[j].picnum==ROTATEGUN))
                    {

                        dal -= (8<<8);

                    }
                    hitinfo.pos.x = ldist(&sprite[g_player[p].ps->i], &sprite[j]);
                    if (hitinfo.pos.x == 0) hitinfo.pos.x++;
                    zvel = ((sprite[j].z-srcvect.z-dal)<<8) / hitinfo.pos.x;
                    sa = getangle(sprite[j].x-srcvect.x,sprite[j].y-srcvect.y);
                }

                Gv_SetVar(g_iAngRangeVarID,angRange, i,p);
                Gv_SetVar(g_iZRangeVarID,zRange,i,p);

                VM_OnEvent(EVENT_GETSHOTRANGE, i,p, -1);

                angRange=Gv_GetVar(g_iAngRangeVarID,i,p);
                zRange=Gv_GetVar(g_iZRangeVarID,i,p);

                if (atwith == SHOTSPARK1__STATIC && !WW2GI && !NAM)
                {
                    if (!g_player[p].ps->auto_aim)
                    {
                        zvel = (100-g_player[p].ps->horiz-g_player[p].ps->horizoff)<<5;
                        if (actor[i].shootzvel) zvel = actor[i].shootzvel;
                        hitscan((const vec3_t *)&srcvect,sect,sintable[(sa+512)&2047],sintable[sa&2047],
                                zvel<<6,&hitinfo,CLIPMASK1);
                        if (hitinfo.hitsprite != -1)
                        {
                            if (sprite[hitinfo.hitsprite].statnum == STAT_ACTOR || sprite[hitinfo.hitsprite].statnum == STAT_ZOMBIEACTOR ||
                                    sprite[hitinfo.hitsprite].statnum == STAT_PLAYER || sprite[hitinfo.hitsprite].statnum == STAT_DUMMYPLAYER)
                                j = hitinfo.hitsprite;
                        }
                    }

                    if (j == -1)
                    {
                        sa += (angRange/2)-(krand()&(angRange-1));
                        zvel = (100-g_player[p].ps->horiz-g_player[p].ps->horizoff)<<5;
                        zvel += (zRange/2)-(krand()&(zRange-1));
                    }
                }
                else
                {
                    sa += (angRange/2)-(krand()&(angRange-1));
                    if (j == -1)
                    {
                        // no target
                        zvel = (100-g_player[p].ps->horiz-g_player[p].ps->horizoff)<<5;
                    }
                    zvel += (zRange/2)-(krand()&(zRange-1));
                }

                srcvect.z -= (2<<8);
            }
            else
            {
                j = A_FindPlayer(s,&x);
                srcvect.z -= (4<<8);
                hitinfo.pos.x = ldist(&sprite[g_player[j].ps->i], s);
                if (hitinfo.pos.x == 0)
                    hitinfo.pos.x++;
                zvel = ((g_player[j].ps->pos.z-srcvect.z) <<8) / hitinfo.pos.x;
                if (s->picnum != BOSS1)
                {
                    zvel += 128-(krand()&255);
                    sa += 32-(krand()&63);
                }
                else
                {
                    zvel += 128-(krand()&255);
                    sa = getangle(g_player[j].ps->pos.x-srcvect.x,g_player[j].ps->pos.y-srcvect.y)+64-(krand()&127);
                }
            }

            s->cstat &= ~257;
            if (actor[i].shootzvel) zvel = actor[i].shootzvel;
            hitscan((const vec3_t *)&srcvect,sect,
                    sintable[(sa+512)&2047],
                    sintable[sa&2047],
                    zvel<<6,&hitinfo,CLIPMASK1);
            s->cstat |= 257;

            if (hitinfo.hitsect < 0) return -1;

            if ((krand()&15) == 0 && sector[hitinfo.hitsect].lotag == 2)
                A_DoWaterTracers(hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,
                                 srcvect.x,srcvect.y,srcvect.z,8-(ud.multimode>>1));

            if (p >= 0)
            {
                k = A_InsertSprite(hitinfo.hitsect,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,SHOTSPARK1,-15,10,10,sa,0,0,i,4);
                sprite[k].extra = *actorscrptr[atwith];
                sprite[k].extra += (krand()%6);
                sprite[k].yvel = atwith; // this is a hack to allow you to detect which weapon spawned a SHOTSPARK1
                actor[k].t_data[6] = hitinfo.hitwall;
                actor[k].t_data[7] = hitinfo.hitsect;
                actor[k].t_data[8] = hitinfo.hitsprite;


                if (hitinfo.hitwall == -1 && hitinfo.hitsprite == -1)
                {
                    if (zvel < 0)
                    {
                        if (sector[hitinfo.hitsect].ceilingstat&1)
                        {
                            sprite[k].xrepeat = 0;
                            sprite[k].yrepeat = 0;
                            return -1;
                        }
                        else
                            Sect_DamageCeiling(hitinfo.hitsect);
                    }
                    l = A_Spawn(k,SMALLSMOKE);
                    actor[l].t_data[6] = hitinfo.hitwall;
                    actor[l].t_data[7] = hitinfo.hitsect;
                    actor[l].t_data[8] = hitinfo.hitsprite;
                }

                if (hitinfo.hitsprite >= 0)
                {
                    A_DamageObject(hitinfo.hitsprite,k);
                    if (sprite[hitinfo.hitsprite].picnum == APLAYER && (ud.ffire == 1 || (!GTFLAGS(GAMETYPE_PLAYERSFRIENDLY) &&
                            GTFLAGS(GAMETYPE_TDM) && g_player[sprite[hitinfo.hitsprite].yvel].ps->team != g_player[sprite[i].yvel].ps->team)))
                    {
                        l = A_Spawn(k,JIBS6);
                        sprite[k].xrepeat = sprite[k].yrepeat = 0;
                        sprite[l].z += (4<<8);
                        sprite[l].xvel = 16;
                        sprite[l].xrepeat = sprite[l].yrepeat = 24;
                        sprite[l].ang += 64-(krand()&127);
                    }
                    else
                    {
                        l = A_Spawn(k,SMALLSMOKE);
                        actor[l].t_data[6] = hitinfo.hitwall;
                        actor[l].t_data[7] = hitinfo.hitsect;
                        actor[l].t_data[8] = hitinfo.hitsprite;
                    }

                    if (p >= 0 && (
                                sprite[hitinfo.hitsprite].picnum == DIPSWITCH ||
                                sprite[hitinfo.hitsprite].picnum == DIPSWITCH+1 ||
                                sprite[hitinfo.hitsprite].picnum == DIPSWITCH2 ||
                                sprite[hitinfo.hitsprite].picnum == DIPSWITCH2+1 ||
                                sprite[hitinfo.hitsprite].picnum == DIPSWITCH3 ||
                                sprite[hitinfo.hitsprite].picnum == DIPSWITCH3+1 ||
                                sprite[hitinfo.hitsprite].picnum == HANDSWITCH ||
                                sprite[hitinfo.hitsprite].picnum == HANDSWITCH+1))
                    {
                        P_ActivateSwitch(p,hitinfo.hitsprite,1);
                        return -1;
                    }
                }
                else if (hitinfo.hitwall >= 0)
                {
                    l = A_Spawn(k,SMALLSMOKE);
                    actor[l].t_data[6] = hitinfo.hitwall;
                    actor[l].t_data[7] = hitinfo.hitsect;
                    actor[l].t_data[8] = hitinfo.hitsprite;

                    if (CheckDoorTile(wall[hitinfo.hitwall].picnum) == 1)
                        goto SKIPBULLETHOLE;
                    if (p >= 0 && (
                                wall[hitinfo.hitwall].picnum == DIPSWITCH ||
                                wall[hitinfo.hitwall].picnum == DIPSWITCH+1 ||
                                wall[hitinfo.hitwall].picnum == DIPSWITCH2 ||
                                wall[hitinfo.hitwall].picnum == DIPSWITCH2+1 ||
                                wall[hitinfo.hitwall].picnum == DIPSWITCH3 ||
                                wall[hitinfo.hitwall].picnum == DIPSWITCH3+1 ||
                                wall[hitinfo.hitwall].picnum == HANDSWITCH ||
                                wall[hitinfo.hitwall].picnum == HANDSWITCH+1))
                    {
                        P_ActivateSwitch(p,hitinfo.hitwall,0);
                        return -1;
                    }

                    if (wall[hitinfo.hitwall].hitag != 0 || (wall[hitinfo.hitwall].nextwall >= 0 && wall[wall[hitinfo.hitwall].nextwall].hitag != 0))
                        goto SKIPBULLETHOLE;

                    if (hitinfo.hitsect >= 0 && sector[hitinfo.hitsect].lotag == 0)
                        if (wall[hitinfo.hitwall].overpicnum != BIGFORCE)
                            if ((wall[hitinfo.hitwall].nextsector >= 0 && sector[wall[hitinfo.hitwall].nextsector].lotag == 0) ||
                                    (wall[hitinfo.hitwall].nextsector == -1 && sector[hitinfo.hitsect].lotag == 0))
                                if ((wall[hitinfo.hitwall].cstat&16) == 0)
                                {
                                    if (wall[hitinfo.hitwall].nextsector >= 0)
                                    {
                                        l = headspritesect[wall[hitinfo.hitwall].nextsector];
                                        while (l >= 0)
                                        {
                                            if (sprite[l].statnum == 3 && sprite[l].lotag == 13)
                                                goto SKIPBULLETHOLE;
                                            l = nextspritesect[l];
                                        }
                                    }

                                    l = headspritestat[STAT_MISC];
                                    while (l >= 0)
                                    {
                                        if (sprite[l].picnum == BULLETHOLE)
                                            if (dist(&sprite[l],&sprite[k]) < (12+(krand()&7)))
                                                goto SKIPBULLETHOLE;
                                        l = nextspritestat[l];
                                    }
                                    l = A_Spawn(k,BULLETHOLE);

                                    if (!A_CheckSpriteFlags(l , SPRITE_DECAL))
                                        actor[l].flags |= SPRITE_DECAL;

                                    sprite[l].xvel = -1;
                                    sprite[l].x = hitinfo.pos.x;
                                    sprite[l].y = hitinfo.pos.y;
                                    sprite[l].z = hitinfo.pos.z;

                                    sprite[l].ang = getangle(wall[hitinfo.hitwall].x-wall[wall[hitinfo.hitwall].point2].x,
                                                             wall[hitinfo.hitwall].y-wall[wall[hitinfo.hitwall].point2].y)+512;

                                    sprite[l].x -= mulscale13(1,sintable[(sprite[l].ang+2560)&2047]);
                                    sprite[l].y -= mulscale13(1,sintable[(sprite[l].ang+2048)&2047]);
                                    A_SetSprite(l,CLIPMASK0);
                                }

SKIPBULLETHOLE:

                    if (wall[hitinfo.hitwall].cstat&2)
                        if (wall[hitinfo.hitwall].nextsector >= 0)
                            if (hitinfo.pos.z >= (sector[wall[hitinfo.hitwall].nextsector].floorz))
                                hitinfo.hitwall = wall[hitinfo.hitwall].nextwall;

                    A_DamageWall(k,hitinfo.hitwall,&hitinfo.pos,SHOTSPARK1);
                }
            }
            else
            {
                k = A_InsertSprite(hitinfo.hitsect,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,SHOTSPARK1,-15,24,24,sa,0,0,i,4);
                sprite[k].extra = *actorscrptr[atwith];
                sprite[k].yvel = atwith; // this is a hack to allow you to detect which weapon spawned a SHOTSPARK1
                actor[k].t_data[6] = hitinfo.hitwall;
                actor[k].t_data[7] = hitinfo.hitsect;
                actor[k].t_data[8] = hitinfo.hitsprite;

                if (hitinfo.hitsprite >= 0)
                {
                    A_DamageObject(hitinfo.hitsprite,k);
                    if (sprite[hitinfo.hitsprite].picnum != APLAYER)
                    {
                        l = A_Spawn(k,SMALLSMOKE);
                        actor[l].t_data[6] = hitinfo.hitwall;
                        actor[l].t_data[7] = hitinfo.hitsect;
                        actor[l].t_data[8] = hitinfo.hitsprite;
                    }
                    else sprite[k].xrepeat = sprite[k].yrepeat = 0;
                }
                else if (hitinfo.hitwall >= 0)
                    A_DamageWall(k,hitinfo.hitwall,&hitinfo.pos,SHOTSPARK1);
            }

            if ((krand()&255) < 4)
                S_PlaySound3D(PISTOL_RICOCHET,k, &hitinfo.pos);

            return -1;

        case FIRELASER__STATIC:
        case SPIT__STATIC:
        case COOLEXPLOSION1__STATIC:

            if (s->extra >= 0) s->shade = -96;

            if (atwith == SPIT) vel = 292;
            else
            {
                if (atwith == COOLEXPLOSION1)
                {
                    if (s->picnum == BOSS2) vel = 644;
                    else vel = 348;
                    srcvect.z -= (4<<7);
                }
                else
                {
                    vel = 840;
                    srcvect.z -= (4<<7);
                }
            }

            if (p >= 0)
            {
                //            j = A_FindTargetSprite( s, AUTO_AIM_ANGLE );
                Gv_SetVar(g_iAimAngleVarID,AUTO_AIM_ANGLE,i,p);
                if (apScriptGameEvent[EVENT_GETAUTOAIMANGLE])
                    VM_OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
                j=-1;
                if (Gv_GetVar(g_iAimAngleVarID,i,p) > 0)
                {
                    j = A_FindTargetSprite(s, Gv_GetVar(g_iAimAngleVarID,i,p),atwith);
                }

                if (j >= 0)
                {
                    dal = ((sprite[j].yrepeat*tilesizy[sprite[j].picnum])<<1)-(12<<8);
                    hitinfo.pos.x = ldist(&sprite[g_player[p].ps->i], &sprite[j]);
                    if (hitinfo.pos.x == 0) hitinfo.pos.x++;
                    zvel = ((sprite[j].z-srcvect.z-dal)*vel) / hitinfo.pos.x;
                    sa = getangle(sprite[j].x-srcvect.x,sprite[j].y-srcvect.y);
                }
                else
                    zvel = (100-g_player[p].ps->horiz-g_player[p].ps->horizoff)*98;
            }
            else
            {
                j = A_FindPlayer(s,&x);
                //                sa = getangle(g_player[j].ps->opos.x-sx,g_player[j].ps->opos.y-sy);
                sa += 16-(krand()&31);
                hitinfo.pos.x = ldist(&sprite[g_player[j].ps->i],s);
                if (hitinfo.pos.x == 0) hitinfo.pos.x++;
                zvel = ((g_player[j].ps->opos.z - srcvect.z + (3<<8))*vel) / hitinfo.pos.x;
            }
            if (actor[i].shootzvel) zvel = actor[i].shootzvel;
            oldzvel = zvel;

            if (atwith == SPIT)
            {
                sizx = sizy = 18;
                srcvect.z -= (10<<8);
            }
            else if (p >= 0)
                sizx = sizy = 7;
            else
            {
                if (atwith == FIRELASER)
                {
                    if (p >= 0)
                        sizx = sizy = 34;
                    else
                        sizx = sizy = 18;
                }
                else
                    sizx = sizy = 18;
            }

            j = A_InsertSprite(sect,srcvect.x,srcvect.y,srcvect.z,
                               atwith,-127,sizx,sizy,sa,vel,zvel,i,4);
            sprite[j].extra += (krand()&7);

            if (atwith == COOLEXPLOSION1)
            {
                sprite[j].shade = 0;
                if (PN == BOSS2)
                {
                    l = sprite[j].xvel;
                    sprite[j].xvel = 1024;
                    A_SetSprite(j,CLIPMASK0);
                    sprite[j].xvel = l;
                    sprite[j].ang += 128-(krand()&255);
                }
            }

            sprite[j].cstat = 128;
            sprite[j].clipdist = 4;

            sa = s->ang+32-(krand()&63);
            zvel = oldzvel+512-(krand()&1023);

            return j;

        case FREEZEBLAST__STATIC:
            srcvect.z += (3<<8);
        case RPG__STATIC:

            if (s->extra >= 0) s->shade = -96;

            vel = 644;

            j = -1;

            if (p >= 0)
            {
                //            j = A_FindTargetSprite( s, AUTO_AIM_ANGLE ); // 48
                Gv_SetVar(g_iAimAngleVarID,AUTO_AIM_ANGLE,i,p);
                if (apScriptGameEvent[EVENT_GETAUTOAIMANGLE])
                    VM_OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
                j=-1;
                if (Gv_GetVar(g_iAimAngleVarID,i,p) > 0)
                {
                    j = A_FindTargetSprite(s, Gv_GetVar(g_iAimAngleVarID,i,p),atwith);
                }

                if (j >= 0)
                {
                    dal = ((sprite[j].yrepeat*tilesizy[sprite[j].picnum])<<1)+(8<<8);
                    hitinfo.pos.x = ldist(&sprite[g_player[p].ps->i], &sprite[j]);
                    if (hitinfo.pos.x == 0) hitinfo.pos.x++;
                    zvel = ((sprite[j].z-srcvect.z-dal)*vel) / hitinfo.pos.x;
                    if (sprite[j].picnum != RECON)
                        sa = getangle(sprite[j].x-srcvect.x,sprite[j].y-srcvect.y);
                }
                else zvel = (100-g_player[p].ps->horiz-g_player[p].ps->horizoff)*81;
                if (atwith == RPG)
                    A_PlaySound(RPG_SHOOT,i);
            }
            else
            {
                j = A_FindPlayer(s,&x);
                sa = getangle(g_player[j].ps->opos.x-srcvect.x,g_player[j].ps->opos.y-srcvect.y);
                if (PN == BOSS3)
                    srcvect.z -= (32<<8);
                else if (PN == BOSS2)
                {
                    vel += 128;
                    srcvect.z += 24<<8;
                }

                l = ldist(&sprite[g_player[j].ps->i],s);
                if (l == 0)
                    l++;
                zvel = ((g_player[j].ps->opos.z-srcvect.z)*vel) / l;

                if (A_CheckEnemySprite(s) && (s->hitag&face_player_smart))
                    sa = s->ang+(krand()&31)-16;
            }

            if (p >= 0 && j >= 0)
                l = j;
            else l = -1;

            if (numplayers > 1 && g_netClient) return -1;

            if (actor[i].shootzvel) zvel = actor[i].shootzvel;
            j = A_InsertSprite(sect,
                               srcvect.x+(sintable[(348+sa+512)&2047]/448),
                               srcvect.y+(sintable[(sa+348)&2047]/448),
                               srcvect.z-(1<<8),atwith,0,14,14,sa,vel,zvel,i,4);

            sprite[j].extra += (krand()&7);
            if (atwith != FREEZEBLAST)
                sprite[j].yvel = l;
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
                        sprite[j].x -= sintable[sa&2047]>>6;
                        sprite[j].y -= sintable[(sa+1024+512)&2047]>>6;
                        sprite[j].ang -= 8;
                    }
                    else
                    {
                        sprite[j].x += sintable[sa&2047]>>6;
                        sprite[j].y += sintable[(sa+1024+512)&2047]>>6;
                        sprite[j].ang += 4;
                    }
                    sprite[j].xrepeat = 42;
                    sprite[j].yrepeat = 42;
                }
                else if (PN == BOSS2)
                {
                    sprite[j].x -= sintable[sa&2047]/56;
                    sprite[j].y -= sintable[(sa+1024+512)&2047]/56;
                    sprite[j].ang -= 8+(krand()&255)-128;
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

            else if (*aplWeaponWorksLike[g_player[p].ps->curr_weapon] == DEVISTATOR_WEAPON)
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

            if (p >= 0)
                zvel = (100-g_player[p].ps->horiz-g_player[p].ps->horizoff)*32;
            else zvel = 0;
            if (actor[i].shootzvel) zvel = actor[i].shootzvel;

            srcvect.z -= g_player[p].ps->pyoff;
            hitscan((const vec3_t *)&srcvect,sect,
                    sintable[(sa+512)&2047],
                    sintable[sa&2047],
                    zvel<<6,&hitinfo,CLIPMASK1);

            srcvect.z += g_player[p].ps->pyoff;
            j = 0;
            if (hitinfo.hitsprite >= 0) break;

            if (hitinfo.hitwall >= 0 && hitinfo.hitsect >= 0)
                if (((hitinfo.pos.x-srcvect.x)*(hitinfo.pos.x-srcvect.x)+(hitinfo.pos.y-srcvect.y)*(hitinfo.pos.y-srcvect.y)) < (290*290))
                {
                    if (wall[hitinfo.hitwall].nextsector >= 0)
                    {
                        if (sector[wall[hitinfo.hitwall].nextsector].lotag <= 2 && sector[hitinfo.hitsect].lotag <= 2)
                            j = 1;
                    }
                    else if (sector[hitinfo.hitsect].lotag <= 2)
                        j = 1;
                }

            if (j == 1)
            {
                int32_t lTripBombControl=Gv_GetVarByLabel("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, g_player[p].ps->i, p);
                k = A_InsertSprite(hitinfo.hitsect,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,TRIPBOMB,-16,4,5,sa,0,0,i,6);
                if (lTripBombControl & TRIPBOMB_TIMER)
                {
                    int32_t lLifetime=Gv_GetVarByLabel("STICKYBOMB_LIFETIME", NAM_GRENADE_LIFETIME, g_player[p].ps->i, p);
                    int32_t lLifetimeVar=Gv_GetVarByLabel("STICKYBOMB_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, g_player[p].ps->i, p);
                    // set timer.  blows up when at zero....
                    actor[k].t_data[7]=lLifetime
                                       + mulscale(krand(),lLifetimeVar, 14)
                                       - lLifetimeVar;
                    actor[k].t_data[6]=1;
                }
                else

                    sprite[k].hitag = k;
                A_PlaySound(LASERTRIP_ONWALL,k);
                sprite[k].xvel = -20;
                A_SetSprite(k,CLIPMASK0);
                sprite[k].cstat = 16;
                actor[k].t_data[5] = sprite[k].ang = getangle(wall[hitinfo.hitwall].x-wall[wall[hitinfo.hitwall].point2].x,wall[hitinfo.hitwall].y-wall[wall[hitinfo.hitwall].point2].y)-512;


            }
            return j?k:-1;

        case BOUNCEMINE__STATIC:
        case MORTER__STATIC:

            if (s->extra >= 0) s->shade = -96;

            j = g_player[A_FindPlayer(s,&x)].ps->i;
            x = ldist(&sprite[j],s);

            zvel = -x>>1;

            if (zvel < -4096)
                zvel = -2048;
            vel = x>>4;
            if (actor[i].shootzvel) zvel = actor[i].shootzvel;
            A_InsertSprite(sect,
                           srcvect.x+(sintable[(512+sa+512)&2047]>>8),
                           srcvect.y+(sintable[(sa+512)&2047]>>8),
                           srcvect.z+(6<<8),atwith,-64,32,32,sa,vel,zvel,i,1);
            break;

        case GROWSPARK__STATIC:

            if (p >= 0)
            {
                //            j = A_FindTargetSprite( s, AUTO_AIM_ANGLE );
                Gv_SetVar(g_iAimAngleVarID,AUTO_AIM_ANGLE,i,p);
                if (apScriptGameEvent[EVENT_GETAUTOAIMANGLE])
                    VM_OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
                j=-1;
                if (Gv_GetVar(g_iAimAngleVarID,i,p) > 0)
                {
                    j = A_FindTargetSprite(s, Gv_GetVar(g_iAimAngleVarID,i,p),atwith);
                }

                if (j >= 0)
                {
                    dal = ((sprite[j].yrepeat*tilesizy[sprite[j].picnum])<<1)+(5<<8);
                    if (((sprite[j].picnum >= GREENSLIME)&&(sprite[j].picnum <= GREENSLIME+7))||(sprite[j].picnum ==ROTATEGUN))
                    {
                        dal -= (8<<8);

                    }
                    hitinfo.pos.x = ldist(&sprite[g_player[p].ps->i], &sprite[j]);
                    if (hitinfo.pos.x == 0)
                        hitinfo.pos.x++;
                    zvel = ((sprite[j].z-srcvect.z-dal)<<8) / hitinfo.pos.x;
                    sa = getangle(sprite[j].x-srcvect.x,sprite[j].y-srcvect.y);
                }
                else
                {
                    sa += 16-(krand()&31);
                    zvel = (100-g_player[p].ps->horiz-g_player[p].ps->horizoff)<<5;
                    zvel += 128-(krand()&255);
                }

                srcvect.z -= (2<<8);
            }
            else
            {
                j = A_FindPlayer(s,&x);
                srcvect.z -= (4<<8);
                hitinfo.pos.x = ldist(&sprite[g_player[j].ps->i], s);
                if (hitinfo.pos.x == 0)
                    hitinfo.pos.x++;
                zvel = ((g_player[j].ps->pos.z-srcvect.z) <<8) / hitinfo.pos.x;
                zvel += 128-(krand()&255);
                sa += 32-(krand()&63);
            }

            k = 0;

            //            RESHOOTGROW:
            if (sect < 0) break;

            s->cstat &= ~257;
            if (actor[i].shootzvel) zvel = actor[i].shootzvel;
            hitscan((const vec3_t *)&srcvect,sect,
                    sintable[(sa+512)&2047],
                    sintable[sa&2047],
                    zvel<<6,&hitinfo,CLIPMASK1);

            s->cstat |= 257;

            j = A_InsertSprite(sect,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,GROWSPARK,-16,28,28,sa,0,0,i,1);

            sprite[j].pal = 2;
            sprite[j].cstat |= 130;
            sprite[j].xrepeat = sprite[j].yrepeat = 1;

            if (hitinfo.hitwall == -1 && hitinfo.hitsprite == -1 && hitinfo.hitsect >= 0)
            {
                if (zvel < 0 && (sector[hitinfo.hitsect].ceilingstat&1) == 0)
                    Sect_DamageCeiling(hitinfo.hitsect);
            }
            else if (hitinfo.hitsprite >= 0) A_DamageObject(hitinfo.hitsprite,j);
            else if (hitinfo.hitwall >= 0 && wall[hitinfo.hitwall].picnum != ACCESSSWITCH && wall[hitinfo.hitwall].picnum != ACCESSSWITCH2)
            {
                /*    if(wall[hitinfo.hitwall].overpicnum == MIRROR && k == 0)
                {
                l = getangle(
                wall[wall[hitinfo.hitwall].point2].x-wall[hitinfo.hitwall].x,
                wall[wall[hitinfo.hitwall].point2].y-wall[hitinfo.hitwall].y);

                sx = hitinfo.pos.x;
                sy = hitinfo.pos.y;
                srcvect.z = hitinfo.pos.z;
                sect = hitinfo.hitsect;
                sa = ((l<<1) - sa)&2047;
                sx += sintable[(sa+512)&2047]>>12;
                sy += sintable[sa&2047]>>12;

                k++;
                goto RESHOOTGROW;
                }
                else */
                A_DamageWall(j,hitinfo.hitwall,&hitinfo.pos,atwith);
            }

            break;

        case SHRINKER__STATIC:
            if (s->extra >= 0) s->shade = -96;
            if (p >= 0)
            {
                //            j = A_FindTargetSprite( s, AUTO_AIM_ANGLE );
                Gv_SetVar(g_iAimAngleVarID,AUTO_AIM_ANGLE,i,p);
                if (apScriptGameEvent[EVENT_GETAUTOAIMANGLE])
                    VM_OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
                j=-1;
                if (Gv_GetVar(g_iAimAngleVarID,i,p) > 0)
                {
                    j = A_FindTargetSprite(s, Gv_GetVar(g_iAimAngleVarID,i,p),atwith);
                }

                if (j >= 0)
                {
                    dal = ((sprite[j].yrepeat*tilesizy[sprite[j].picnum])<<1);
                    hitinfo.pos.x = ldist(&sprite[g_player[p].ps->i], &sprite[j]);
                    if (hitinfo.pos.x == 0)
                        hitinfo.pos.x++;
                    zvel = ((sprite[j].z-srcvect.z-dal-(4<<8))*768) / hitinfo.pos.x;
                    sa = getangle(sprite[j].x-srcvect.x,sprite[j].y-srcvect.y);
                }
                else zvel = (100-g_player[p].ps->horiz-g_player[p].ps->horizoff)*98;
            }
            else if (s->statnum != 3)
            {
                j = A_FindPlayer(s,&x);
                l = ldist(&sprite[g_player[j].ps->i],s);
                if (l == 0)
                    l++;
                zvel = ((g_player[j].ps->opos.z-srcvect.z)*512) / l ;
            }
            else zvel = 0;
            if (actor[i].shootzvel) zvel = actor[i].shootzvel;
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

static void P_DisplaySpit(int32_t snum)
{
    int32_t i, a, x, y, z;

    if (g_player[snum].ps->loogcnt == 0) return;

    y = (g_player[snum].ps->loogcnt<<2);
    for (i=0; i<g_player[snum].ps->numloogs; i++)
    {
        a = klabs(sintable[((g_player[snum].ps->loogcnt+i)<<5)&2047])>>5;
        z = 4096+((g_player[snum].ps->loogcnt+i)<<9);
        x = (-g_player[snum].sync->avel)+(sintable[((g_player[snum].ps->loogcnt+i)<<6)&2047]>>10);

        rotatesprite(
            (g_player[snum].ps->loogiex[i]+x)<<16,(200+g_player[snum].ps->loogiey[i]-y)<<16,z-(i<<8),256-a,
            LOOGIE,0,0,2,0,0,xdim-1,ydim-1);
    }
}

static int32_t P_DisplayFist(int32_t gs,int32_t snum)
{
    int32_t looking_arc,fisti,fistpal;
    int32_t fistzoom, fistz;

    fisti = g_player[snum].ps->fist_incs;
    if (fisti > 32) fisti = 32;
    if (fisti <= 0) return 0;

    looking_arc = klabs(g_player[snum].ps->look_ang)/9;

    fistzoom = 65536L - (sintable[(512+(fisti<<6))&2047]<<2);
    if (fistzoom > 90612L)
        fistzoom = 90612L;
    if (fistzoom < 40920)
        fistzoom = 40290;
    fistz = 194 + (sintable[((6+fisti)<<7)&2047]>>9);

    if (sprite[g_player[snum].ps->i].pal == 1)
        fistpal = 1;
    else if (g_player[snum].ps->cursectnum >= 0)
        fistpal = sector[g_player[snum].ps->cursectnum].floorpal;
    else fistpal = 0;

    rotatesprite(
        (-fisti+222+(g_player[snum].sync->avel>>4))<<16,
        (looking_arc+fistz)<<16,
        fistzoom,0,FIST,gs,fistpal,2,0,0,xdim-1,ydim-1);

    return 1;
}

#define weapsc(sc) scale(sc,ud.weaponscale,100)

static void G_DrawTileScaled(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p)
{
    int32_t a = 0;
    int32_t xoff = 192;

    switch (g_currentweapon)
    {
    case DEVISTATOR_WEAPON:
    case TRIPBOMB_WEAPON:
        xoff = 160;
        break;
    default:
        if (orientation & 262144)
        {
            xoff = 160;
            orientation &= ~262144;
        }
        break;
    }

    if (orientation&4)
        a = 1024;

#if defined(POLYMOST) && defined(USE_OPENGL)
    if (getrendermode() >= 3 && usemodels && md_tilehasmodel(tilenum,p) > 0)
        y += (224-weapsc(224));
#endif
    rotatesprite(weapsc((orientation&1024)?x:(x<<16))+((xoff-weapsc(xoff))<<16),
                 weapsc((orientation&1024)?y:(y<<16))+((200-weapsc(200))<<16),
                 weapsc(65536L),a,tilenum,shade,p,(2|orientation),windowx1,windowy1,windowx2,windowy2);
}

static void G_DrawWeaponTile(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p)
{
    static int32_t shadef = 0, palf = 0;

    // basic fading between player weapon shades
    if (shadef != shade && (!p || palf == p))
    {
        shadef += (shade-shadef)>>2;

        if (!((shade-shadef)>>2))
        {
            shadef += (shade-shadef)>>1;
            if (!((shade-shadef)>>1))
                shadef = shade;
        }
    }
    else
        shadef = shade;

    palf = p;

    switch (ud.drawweapon)
    {
    default:
        return;
    case 1:
        G_DrawTileScaled(x,y,tilenum,shadef,orientation,p);
        return;
    case 2:
        switch (g_currentweapon)
        {
        case PISTOL_WEAPON:
        case CHAINGUN_WEAPON:
        case RPG_WEAPON:
        case FREEZE_WEAPON:
        case SHRINKER_WEAPON:
        case GROW_WEAPON:
        case DEVISTATOR_WEAPON:
        case TRIPBOMB_WEAPON:
        case HANDREMOTE_WEAPON:
        case HANDBOMB_WEAPON:
        case SHOTGUN_WEAPON:
            rotatesprite(160<<16,(180+(g_player[screenpeek].ps->weapon_pos*g_player[screenpeek].ps->weapon_pos))<<16,
                         scale(65536,ud.statusbarscale,100),0,g_currentweapon==GROW_WEAPON?GROWSPRITEICON:WeaponPickupSprites[g_currentweapon],
                         0,0,2,windowx1,windowy1,windowx2,windowy2);
            return;
        default:
            return;
        }
    }
}

static int32_t P_DisplayKnee(int32_t gs,int32_t snum)
{
    static int8_t knee_y[] = {0,-8,-16,-32,-64,-84,-108,-108,-108,-72,-32,-8};
    int32_t looking_arc, pal = g_player[snum].ps->palookup;

    if (g_player[snum].ps->knee_incs > 11 || g_player[snum].ps->knee_incs == 0 || sprite[g_player[snum].ps->i].extra <= 0) return 0;

    looking_arc = knee_y[g_player[snum].ps->knee_incs] + klabs(g_player[snum].ps->look_ang)/9;

    looking_arc -= (g_player[snum].ps->hard_landing<<3);

    if (sprite[g_player[snum].ps->i].pal == 1)
        pal = 1;
    else if (g_player[snum].ps->cursectnum >= 0)
    {
        pal = sector[g_player[snum].ps->cursectnum].floorpal;
        if (pal == 0)
            pal = g_player[snum].ps->palookup;
    }

    G_DrawTileScaled(105+(g_player[snum].sync->avel>>4)-(g_player[snum].ps->look_ang>>1)+(knee_y[g_player[snum].ps->knee_incs]>>2),
                     looking_arc+280-((g_player[snum].ps->horiz-g_player[snum].ps->horizoff)>>4),KNEE,gs,4+262144,pal);

    return 1;
}

static int32_t P_DisplayKnuckles(int32_t gs,int32_t snum)
{
    static int8_t knuckle_frames[] = {0,1,2,2,3,3,3,2,2,1,0};
    int32_t looking_arc, pal = 0;

    if (g_player[snum].ps->knuckle_incs == 0 || sprite[g_player[snum].ps->i].extra <= 0) return 0;

    looking_arc = klabs(g_player[snum].ps->look_ang)/9;

    looking_arc -= (g_player[snum].ps->hard_landing<<3);

    if (sprite[g_player[snum].ps->i].pal == 1)
        pal = 1;
    else if (g_player[snum].ps->cursectnum >= 0)
        pal = sector[g_player[snum].ps->cursectnum].floorpal;

    G_DrawTileScaled(160+(g_player[snum].sync->avel>>4)-(g_player[snum].ps->look_ang>>1),
                     looking_arc+180-((g_player[snum].ps->horiz-g_player[snum].ps->horizoff)>>4),
                     CRACKKNUCKLES+knuckle_frames[g_player[snum].ps->knuckle_incs>>1],gs,4+262144,pal);

    return 1;
}

int32_t lastvisinc;

void P_FireWeapon(DukePlayer_t *p)
{
    int32_t i, snum = sprite[p->i].yvel;

    aGameVars[g_iReturnVarID].val.lValue = 0;

    if (apScriptGameEvent[EVENT_DOFIRE])
        VM_OnEvent(EVENT_DOFIRE, p->i, snum, -1);

    if (aGameVars[g_iReturnVarID].val.lValue == 0)
    {
        if (p->weapon_pos != 0) return;

        if (aplWeaponWorksLike[p->curr_weapon][snum] != KNEE_WEAPON)
            p->ammo_amount[p->curr_weapon]--;

        if (aplWeaponFireSound[p->curr_weapon][snum] > 0)
            A_PlaySound(aplWeaponFireSound[p->curr_weapon][snum],p->i);

        Gv_SetVar(g_iWeaponVarID,p->curr_weapon,p->i,snum);
        Gv_SetVar(g_iWorksLikeVarID,aplWeaponWorksLike[p->curr_weapon][snum], p->i, snum);
//        OSD_Printf("doing %d %d %d\n",aplWeaponShoots[p->curr_weapon][snum],p->curr_weapon,snum);
        A_Shoot(p->i,aplWeaponShoots[p->curr_weapon][snum]);

        for (i=aplWeaponShotsPerBurst[p->curr_weapon][snum]-1; i > 0; i--)
        {
            if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FIREEVERYOTHER)
            {
                // this makes the projectiles fire on a delay from player code
                actor[p->i].t_data[7] = (aplWeaponShotsPerBurst[p->curr_weapon][snum])<<1;
            }
            else
            {
                if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_AMMOPERSHOT &&
                        aplWeaponWorksLike[p->curr_weapon][snum] != KNEE_WEAPON)
                {
                    if (p->ammo_amount[p->curr_weapon] > 0)
                        p->ammo_amount[p->curr_weapon]--;
                    else break;
                }
                A_Shoot(p->i,aplWeaponShoots[p->curr_weapon][snum]);
            }
        }

        if (!(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_NOVISIBLE))
        {
#ifdef POLYMER
            spritetype *s = &sprite[p->i];
            int32_t x = ((sintable[(s->ang+512)&2047])>>7), y = ((sintable[(s->ang)&2047])>>7);

            s->x += x;
            s->y += y;
            G_AddGameLight(0, p->i, PHEIGHT, 8192, aplWeaponFlashColor[p->curr_weapon][snum],PR_LIGHT_PRIO_MAX_GAME);
            actor[p->i].lightcount = 2;
            s->x -= x;
            s->y -= y;
#endif // POLYMER
            lastvisinc = totalclock+32;
            p->visibility = 0;
        }
    }
}

void P_DoWeaponSpawn(DukePlayer_t *p)
{
    int32_t j, snum = sprite[p->i].yvel;

    if (!aplWeaponSpawn[p->curr_weapon][snum])
        return;

    j = A_Spawn(p->i, aplWeaponSpawn[p->curr_weapon][snum]);

    if ((aplWeaponFlags[p->curr_weapon][snum] & WEAPON_SPAWNTYPE3))
    {
        // like chaingun shells
        sprite[j].ang += 1024;
        sprite[j].ang &= 2047;
        sprite[j].xvel += 32;
        sprite[j].z += (3<<8);
    }

    A_SetSprite(j,CLIPMASK0);

}

void P_DisplayScuba(int32_t snum)
{
    int32_t p;

    if (sprite[g_player[snum].ps->i].pal == 1)
        p = 1;
    else if (g_player[snum].ps->cursectnum >= 0)
        p = sector[g_player[snum].ps->cursectnum].floorpal;
    else p = 0;

    if (g_player[snum].ps->scuba_on)
    {
        rotatesprite(43<<16,(200-tilesizy[SCUBAMASK])<<16,65536,0,SCUBAMASK,0,p,2+16,windowx1,windowy1,windowx2,windowy2);
        rotatesprite((320-43)<<16,(200-tilesizy[SCUBAMASK])<<16,65536,1024,SCUBAMASK,0,p,2+4+16,windowx1,windowy1,windowx2,windowy2);
    }
}

static int32_t P_DisplayTip(int32_t gs,int32_t snum)
{
    int32_t p,looking_arc;
    static int16_t tip_y[] = {0,-8,-16,-32,-64,-84,-108,-108,-108,-108,-108,-108,-108,-108,-108,-108,-96,-72,-64,-32,-16};

    if (g_player[snum].ps->tipincs == 0) return 0;

    looking_arc = klabs(g_player[snum].ps->look_ang)/9;
    looking_arc -= (g_player[snum].ps->hard_landing<<3);

    if (sprite[g_player[snum].ps->i].pal == 1)
        p = 1;
    else
        p = sector[g_player[snum].ps->cursectnum].floorpal;

    /*    if(g_player[snum].ps->access_spritenum >= 0)
            p = sprite[g_player[snum].ps->access_spritenum].pal;
        else
            p = wall[g_player[snum].ps->access_wallnum].pal;
      */
    G_DrawTileScaled(170+(g_player[snum].sync->avel>>4)-(g_player[snum].ps->look_ang>>1),
                     (tip_y[g_player[snum].ps->tipincs]>>1)+looking_arc+240-((g_player[snum].ps->horiz-g_player[snum].ps->horizoff)>>4),TIP+((26-g_player[snum].ps->tipincs)>>4),gs,262144,p);

    return 1;
}

static int32_t P_DisplayAccess(int32_t gs,int32_t snum)
{
    static int16_t access_y[] = {0,-8,-16,-32,-64,-84,-108,-108,-108,-108,-108,-108,-108,-108,-108,-108,-96,-72,-64,-32,-16};
    int32_t looking_arc, p = 0;

    if (g_player[snum].ps->access_incs == 0 || sprite[g_player[snum].ps->i].extra <= 0) return 0;

    looking_arc = access_y[g_player[snum].ps->access_incs] + klabs(g_player[snum].ps->look_ang)/9 -
                  (g_player[snum].ps->hard_landing<<3);

    if (g_player[snum].ps->access_spritenum >= 0)
        p = sprite[g_player[snum].ps->access_spritenum].pal;

    //    else
    //        p = wall[g_player[snum].ps->access_wallnum].pal;

    if ((g_player[snum].ps->access_incs-3) > 0 && (g_player[snum].ps->access_incs-3)>>3)
        G_DrawTileScaled(170+(g_player[snum].sync->avel>>4)-(g_player[snum].ps->look_ang>>1)+(access_y[g_player[snum].ps->access_incs]>>2),
                         looking_arc+266-((g_player[snum].ps->horiz-g_player[snum].ps->horizoff)>>4),HANDHOLDINGLASER+(g_player[snum].ps->access_incs>>3),
                         gs,262144,p);
    else
        G_DrawTileScaled(170+(g_player[snum].sync->avel>>4)-(g_player[snum].ps->look_ang>>1)+(access_y[g_player[snum].ps->access_incs]>>2),
                         looking_arc+266-((g_player[snum].ps->horiz-g_player[snum].ps->horizoff)>>4),HANDHOLDINGACCESS,gs,4+262144,p);

    return 1;
}


static int32_t fistsign;

void P_DisplayWeapon(int32_t snum)
{
    int32_t gun_pos, looking_arc, cw;
    int32_t weapon_xoffset, i, j;
    int32_t o = 0,pal = 0;
    DukePlayer_t *p = g_player[snum].ps;
    uint8_t *kb = &p->kickback_pic;
    int32_t gs;

    looking_arc = klabs(p->look_ang)/9;

    gs = sprite[p->i].shade;
    if (gs > 24) gs = 24;

    if (p->newowner >= 0 || ud.camerasprite >= 0 || p->over_shoulder_on > 0 || (sprite[p->i].pal != 1 && sprite[p->i].extra <= 0) ||
            P_DisplayFist(gs,snum) || P_DisplayKnuckles(gs,snum) || P_DisplayTip(gs,snum) || P_DisplayAccess(gs,snum))
        return;

    P_DisplayKnee(gs,snum);

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

    if (p->last_weapon >= 0)
        cw = aplWeaponWorksLike[p->last_weapon][snum];
    else
        cw = aplWeaponWorksLike[p->curr_weapon][snum];

    g_gun_pos=gun_pos;
    g_looking_arc=looking_arc;
    g_currentweapon=cw;
    g_weapon_xoffset=weapon_xoffset;
    g_gs=gs;
    g_kb=*kb;
    g_looking_angSR1=p->look_ang>>1;

    aGameVars[g_iReturnVarID].val.lValue = 0;
    if (apScriptGameEvent[EVENT_DISPLAYWEAPON])
        VM_OnEvent(EVENT_DISPLAYWEAPON, p->i, screenpeek, -1);

    if (aGameVars[g_iReturnVarID].val.lValue == 0)
    {
        j = 14-p->quick_kick;
        if (j != 14 || p->last_quick_kick)
        {
            if (sprite[p->i].pal == 1)
                pal = 1;
            else
            {
                if (p->cursectnum >= 0)
                    pal = sector[p->cursectnum].floorpal;
                if (pal == 0)
                    pal = p->palookup;
            }

            guniqhudid = 100;
            if (j < 6 || j > 12)
                G_DrawTileScaled(weapon_xoffset+80-(p->look_ang>>1),
                                 looking_arc+250-gun_pos,KNEE,gs,o|4|262144,pal);
            else G_DrawTileScaled(weapon_xoffset+160-16-(p->look_ang>>1),
                                      looking_arc+214-gun_pos,KNEE+1,gs,o|4|262144,pal);
            guniqhudid = 0;
        }

        if (sprite[p->i].xrepeat < 40)
        {
            if (p->jetpack_on == 0)
            {
                i = sprite[p->i].xvel;
                looking_arc += 32-(i>>1);
                fistsign += i>>1;
            }
            cw = weapon_xoffset;
            weapon_xoffset += sintable[(fistsign)&2047]>>10;
            G_DrawTile(weapon_xoffset+250-(p->look_ang>>1),
                       looking_arc+258-(klabs(sintable[(fistsign)&2047]>>8)),
                       FIST,gs,o);
            weapon_xoffset = cw - (sintable[(fistsign)&2047]>>10);
            G_DrawTile(weapon_xoffset+40-(p->look_ang>>1),
                       looking_arc+200+(klabs(sintable[(fistsign)&2047]>>8)),
                       FIST,gs,o|4);
        }
        else switch (cw)
            {
            case KNEE_WEAPON:

                aGameVars[g_iReturnVarID].val.lValue = 0;
                if (apScriptGameEvent[EVENT_DRAWWEAPON])
                    VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek, -1);
                if (aGameVars[g_iReturnVarID].val.lValue == 0)
                {
                    if ((*kb) > 0)
                    {
                        if (sprite[p->i].pal == 1)
                            pal = 1;
                        else
                        {
                            if (p->cursectnum >= 0)
                                pal = sector[p->cursectnum].floorpal;
                            if (pal == 0)
                                pal = p->palookup;
                        }
                        guniqhudid = cw;
                        if ((*kb) < 5 || (*kb) > 9)
                            G_DrawTileScaled(weapon_xoffset+220-(p->look_ang>>1),
                                             looking_arc+250-gun_pos,KNEE,gs,o,pal);
                        else
                            G_DrawTileScaled(weapon_xoffset+160-(p->look_ang>>1),
                                             looking_arc+214-gun_pos,KNEE+1,gs,o,pal);
                        guniqhudid = 0;
                    }
                }
                break;

            case TRIPBOMB_WEAPON:

                aGameVars[g_iReturnVarID].val.lValue = 0;
                if (apScriptGameEvent[EVENT_DRAWWEAPON])
                    VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek, -1);
                if (aGameVars[g_iReturnVarID].val.lValue == 0)
                {
                    if (sprite[p->i].pal == 1)
                        pal = 1;
                    else if (p->cursectnum >= 0)
                        pal = sector[p->cursectnum].floorpal;
                    else pal = 0;

                    weapon_xoffset += 8;
                    gun_pos -= 10;

                    if ((*kb) > 6)
                        looking_arc += ((*kb)<<3);
                    else if ((*kb) < 4)
                    {
                        guniqhudid = cw<<2;
                        G_DrawWeaponTile(weapon_xoffset+142-(p->look_ang>>1),
                                         looking_arc+234-gun_pos,HANDHOLDINGLASER+3,gs,o,pal);
                    }

                    guniqhudid = cw;
                    G_DrawWeaponTile(weapon_xoffset+130-(p->look_ang>>1),
                                     looking_arc+249-gun_pos,
                                     HANDHOLDINGLASER+((*kb)>>2),gs,o,pal);

                    guniqhudid = cw<<1;
                    G_DrawWeaponTile(weapon_xoffset+152-(p->look_ang>>1),
                                     looking_arc+249-gun_pos,
                                     HANDHOLDINGLASER+((*kb)>>2),gs,o|4,pal);
                    guniqhudid = 0;
                }
                break;

            case RPG_WEAPON:

                aGameVars[g_iReturnVarID].val.lValue = 0;
                if (apScriptGameEvent[EVENT_DRAWWEAPON])
                    VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek, -1);
                if (aGameVars[g_iReturnVarID].val.lValue == 0)
                {
                    if (sprite[p->i].pal == 1)
                        pal = 1;
                    else if (p->cursectnum >= 0)
                        pal = sector[p->cursectnum].floorpal;
                    else pal = 0;

                    weapon_xoffset -= sintable[(768+((*kb)<<7))&2047]>>11;
                    gun_pos += sintable[(768+((*kb)<<7))&2047]>>11;

                    if (*kb > 0)
                    {
                        if (*kb < 8)
                        {
                            G_DrawWeaponTile(weapon_xoffset+164,(looking_arc<<1)+176-gun_pos,
                                             RPGGUN+((*kb)>>1),gs,o|512,pal);
                        }
                    }

                    G_DrawWeaponTile(weapon_xoffset+164,(looking_arc<<1)+176-gun_pos,
                                     RPGGUN,gs,o|512,pal);
                }
                break;

            case SHOTGUN_WEAPON:

                aGameVars[g_iReturnVarID].val.lValue = 0;
                if (apScriptGameEvent[EVENT_DRAWWEAPON])
                    VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek, -1);
                if (aGameVars[g_iReturnVarID].val.lValue == 0)
                {
                    if (sprite[p->i].pal == 1)
                        pal = 1;
                    else if (p->cursectnum >= 0)
                        pal = sector[p->cursectnum].floorpal;
                    else pal = 0;


                    weapon_xoffset -= 8;

                    switch (*kb)
                    {
                    case 1:
                    case 2:
                        guniqhudid = cw<<1;
                        G_DrawWeaponTile(weapon_xoffset+168-(p->look_ang>>1),looking_arc+201-gun_pos,
                                         SHOTGUN+2,-128,o,pal);
                    case 0:
                    case 6:
                    case 7:
                    case 8:
                        guniqhudid = cw;
                        G_DrawWeaponTile(weapon_xoffset+146-(p->look_ang>>1),looking_arc+202-gun_pos,
                                         SHOTGUN,gs,o,pal);
                        guniqhudid = 0;
                        break;
                    case 3:
                    case 4:
                    case 5:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                        if (*kb > 1 && *kb < 5)
                        {
                            gun_pos -= 40;
                            weapon_xoffset += 20;

                            guniqhudid = cw<<1;
                            G_DrawWeaponTile(weapon_xoffset+178-(p->look_ang>>1),looking_arc+194-gun_pos,
                                             SHOTGUN+1+((*(kb)-1)>>1),-128,o,pal);
                        }
                        guniqhudid = cw;
                        G_DrawWeaponTile(weapon_xoffset+158-(p->look_ang>>1),looking_arc+220-gun_pos,
                                         SHOTGUN+3,gs,o,pal);
                        guniqhudid = 0;
                        break;
                    case 13:
                    case 14:
                    case 15:
                        guniqhudid = cw;
                        G_DrawWeaponTile(32+weapon_xoffset+166-(p->look_ang>>1),looking_arc+210-gun_pos,
                                         SHOTGUN+4,gs,o,pal);
                        guniqhudid = 0;
                        break;
                    case 16:
                    case 17:
                    case 18:
                    case 19:
                        guniqhudid = cw;
                        G_DrawWeaponTile(64+weapon_xoffset+170-(p->look_ang>>1),looking_arc+196-gun_pos,
                                         SHOTGUN+5,gs,o,pal);
                        guniqhudid = 0;
                        break;
                    case 20:
                    case 21:
                    case 22:
                    case 23:
                        guniqhudid = cw;
                        G_DrawWeaponTile(64+weapon_xoffset+176-(p->look_ang>>1),looking_arc+196-gun_pos,
                                         SHOTGUN+6,gs,o,pal);
                        guniqhudid = 0;
                        break;
                    case 24:
                    case 25:
                    case 26:
                    case 27:
                        guniqhudid = cw;
                        G_DrawWeaponTile(64+weapon_xoffset+170-(p->look_ang>>1),looking_arc+196-gun_pos,
                                         SHOTGUN+5,gs,o,pal);
                        guniqhudid = 0;
                        break;
                    case 28:
                    case 29:
                    case 30:
                        guniqhudid = cw;
                        G_DrawWeaponTile(32+weapon_xoffset+156-(p->look_ang>>1),looking_arc+206-gun_pos,
                                         SHOTGUN+4,gs,o,pal);
                        guniqhudid = 0;
                        break;
                    }
                }
                break;


            case CHAINGUN_WEAPON:

                aGameVars[g_iReturnVarID].val.lValue = 0;
                if (apScriptGameEvent[EVENT_DRAWWEAPON])
                    VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek, -1);
                if (aGameVars[g_iReturnVarID].val.lValue == 0)
                {
                    if (sprite[p->i].pal == 1)
                        pal = 1;
                    else if (p->cursectnum >= 0)
                        pal = sector[p->cursectnum].floorpal;
                    else pal = 0;

                    if (*kb > 0)
                        gun_pos -= sintable[(*kb)<<7]>>12;

                    if (*kb > 0 && sprite[p->i].pal != 1) weapon_xoffset += 1-(rand()&3);

                    G_DrawWeaponTile(weapon_xoffset+168-(p->look_ang>>1),looking_arc+260-gun_pos,
                                     CHAINGUN,gs,o,pal);
                    switch (*kb)
                    {
                    case 0:
                        G_DrawWeaponTile(weapon_xoffset+178-(p->look_ang>>1),looking_arc+233-gun_pos,
                                         CHAINGUN+1,gs,o,pal);
                        break;
                    default:
                        if (*kb > *aplWeaponFireDelay[CHAINGUN_WEAPON] && *kb < *aplWeaponTotalTime[CHAINGUN_WEAPON])
                        {
                            i = 0;
                            if (sprite[p->i].pal != 1) i = rand()&7;
                            G_DrawWeaponTile(i+weapon_xoffset-4+140-(p->look_ang>>1),i+looking_arc-((*kb)>>1)+208-gun_pos,
                                             CHAINGUN+5+((*kb-4)/5),gs,o,pal);
                            if (sprite[p->i].pal != 1) i = rand()&7;
                            G_DrawWeaponTile(i+weapon_xoffset-4+184-(p->look_ang>>1),i+looking_arc-((*kb)>>1)+208-gun_pos,
                                             CHAINGUN+5+((*kb-4)/5),gs,o,pal);
                        }
                        if (*kb < *aplWeaponTotalTime[CHAINGUN_WEAPON]-4)
                        {
                            i = rand()&7;
                            G_DrawWeaponTile(i+weapon_xoffset-4+162-(p->look_ang>>1),i+looking_arc-((*kb)>>1)+208-gun_pos,
                                             CHAINGUN+5+((*kb-2)/5),gs,o,pal);
                            G_DrawWeaponTile(weapon_xoffset+178-(p->look_ang>>1),looking_arc+233-gun_pos,
                                             CHAINGUN+1+((*kb)>>1),gs,o,pal);
                        }
                        else G_DrawWeaponTile(weapon_xoffset+178-(p->look_ang>>1),looking_arc+233-gun_pos,
                                                  CHAINGUN+1,gs,o,pal);
                        break;
                    }
                }
                break;

            case PISTOL_WEAPON:

                aGameVars[g_iReturnVarID].val.lValue = 0;
                if (apScriptGameEvent[EVENT_DRAWWEAPON])
                    VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek, -1);
                if (aGameVars[g_iReturnVarID].val.lValue == 0)
                {
                    if (sprite[p->i].pal == 1)
                        pal = 1;
                    else if (p->cursectnum >= 0)
                        pal = sector[p->cursectnum].floorpal;
                    else pal = 0;

                    if ((*kb) < *aplWeaponTotalTime[PISTOL_WEAPON]+1)
                    {
                        static uint8_t kb_frames[] = { 0, 1, 2 };
                        int32_t l = 195-12+weapon_xoffset;

                        if ((*kb) == *aplWeaponFireDelay[PISTOL_WEAPON])
                            l -= 3;

                        guniqhudid = cw;
                        G_DrawWeaponTile((l-(p->look_ang>>1)),(looking_arc+244-gun_pos),FIRSTGUN+kb_frames[*kb>2?0:*kb],gs,2,pal);
                        guniqhudid = 0;
                    }
                    else
                    {

                        if ((*kb) < *aplWeaponReload[PISTOL_WEAPON]-17)
                        {
                            guniqhudid = cw;
                            G_DrawWeaponTile(194-(p->look_ang>>1),looking_arc+230-gun_pos,FIRSTGUN+4,gs,o|512,pal);
                            guniqhudid = 0;
                        }
                        else if ((*kb) < *aplWeaponReload[PISTOL_WEAPON]-12)
                        {
                            G_DrawWeaponTile(244-((*kb)<<3)-(p->look_ang>>1),looking_arc+130-gun_pos+((*kb)<<4),FIRSTGUN+6,gs,o|512,pal);
                            guniqhudid = cw;
                            G_DrawWeaponTile(224-(p->look_ang>>1),looking_arc+220-gun_pos,FIRSTGUN+5,gs,o|512,pal);
                            guniqhudid = 0;
                        }
                        else if ((*kb) < *aplWeaponReload[PISTOL_WEAPON]-7)
                        {
                            G_DrawWeaponTile(124+((*kb)<<1)-(p->look_ang>>1),looking_arc+430-gun_pos-((*kb)<<3),FIRSTGUN+6,gs,o|512,pal);
                            guniqhudid = cw;
                            G_DrawWeaponTile(224-(p->look_ang>>1),looking_arc+220-gun_pos,FIRSTGUN+5,gs,o|512,pal);
                            guniqhudid = 0;
                        }

                        else if ((*kb) < *aplWeaponReload[PISTOL_WEAPON]-4)
                        {
                            G_DrawWeaponTile(184-(p->look_ang>>1),looking_arc+235-gun_pos,FIRSTGUN+8,gs,o|512,pal);
                            guniqhudid = cw;
                            G_DrawWeaponTile(224-(p->look_ang>>1),looking_arc+210-gun_pos,FIRSTGUN+5,gs,o|512,pal);
                            guniqhudid = 0;
                        }
                        else if ((*kb) < *aplWeaponReload[PISTOL_WEAPON]-2)
                        {
                            G_DrawWeaponTile(164-(p->look_ang>>1),looking_arc+245-gun_pos,FIRSTGUN+8,gs,o|512,pal);
                            guniqhudid = cw;
                            G_DrawWeaponTile(224-(p->look_ang>>1),looking_arc+220-gun_pos,FIRSTGUN+5,gs,o|512,pal);
                            guniqhudid = 0;
                        }
                        else if ((*kb) < *aplWeaponReload[PISTOL_WEAPON])
                        {
                            guniqhudid = cw;
                            G_DrawWeaponTile(194-(p->look_ang>>1),looking_arc+235-gun_pos,FIRSTGUN+5,gs,o|512,pal);
                            guniqhudid = 0;
                        }

                    }
                }

                break;
            case HANDBOMB_WEAPON:
            {
                aGameVars[g_iReturnVarID].val.lValue = 0;
                if (apScriptGameEvent[EVENT_DRAWWEAPON])
                    VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek, -1);
                if (aGameVars[g_iReturnVarID].val.lValue == 0)
                {
                    if (sprite[p->i].pal == 1)
                        pal = 1;
                    else if (p->cursectnum >= 0)
                        pal = sector[p->cursectnum].floorpal;
                    else pal = 0;
                    guniqhudid = cw;
                    if ((*kb))
                    {
                        if ((*kb) < (*aplWeaponTotalTime[p->curr_weapon]))
                        {

                            static uint8_t throw_frames[] = {0,0,0,0,0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2};

                            if ((*kb) < 7)
                                gun_pos -= 10*(*kb);        //D
                            else if ((*kb) < 12)
                                gun_pos += 20*((*kb)-10); //U
                            else if ((*kb) < 20)
                                gun_pos -= 9*((*kb)-14);  //D

                            G_DrawWeaponTile(weapon_xoffset+190-(p->look_ang>>1),looking_arc+250-gun_pos,HANDTHROW+throw_frames[(*kb)],gs,o,pal);
                        }
                    }
                    else
                        G_DrawWeaponTile(weapon_xoffset+190-(p->look_ang>>1),looking_arc+260-gun_pos,HANDTHROW,gs,o,pal);
                    guniqhudid = 0;
                }
            }
            break;

            case HANDREMOTE_WEAPON:
            {
                aGameVars[g_iReturnVarID].val.lValue = 0;
                if (apScriptGameEvent[EVENT_DRAWWEAPON])
                    VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek, -1);
                if (aGameVars[g_iReturnVarID].val.lValue == 0)
                {
                    static uint8_t remote_frames[] = {0,1,1,2,1,1,0,0,0,0,0};
                    if (sprite[p->i].pal == 1)
                        pal = 1;
                    else if (p->cursectnum >= 0)
                        pal = sector[p->cursectnum].floorpal;
                    else pal = 0;

                    weapon_xoffset = -48;
                    guniqhudid = cw;
                    if ((*kb))
                        G_DrawWeaponTile(weapon_xoffset+150-(p->look_ang>>1),looking_arc+258-gun_pos,HANDREMOTE+remote_frames[(*kb)],gs,o,pal);
                    else
                        G_DrawWeaponTile(weapon_xoffset+150-(p->look_ang>>1),looking_arc+258-gun_pos,HANDREMOTE,gs,o,pal);
                    guniqhudid = 0;
                }
            }
            break;

            case DEVISTATOR_WEAPON:

                aGameVars[g_iReturnVarID].val.lValue = 0;
                if (apScriptGameEvent[EVENT_DRAWWEAPON])
                    VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek, -1);
                if (aGameVars[g_iReturnVarID].val.lValue == 0)
                {
                    if (sprite[p->i].pal == 1)
                        pal = 1;
                    else if (p->cursectnum >= 0)
                        pal = sector[p->cursectnum].floorpal;
                    else pal = 0;

                    if ((*kb) < (*aplWeaponTotalTime[DEVISTATOR_WEAPON]+1) && (*kb) > 0)
                    {
                        static uint8_t cycloidy[] = {0,4,12,24,12,4,0};

                        i = ksgn((*kb)>>2);

                        if (p->hbomb_hold_delay)
                        {
                            guniqhudid = cw;
                            G_DrawWeaponTile((cycloidy[*kb]>>1)+weapon_xoffset+268-(p->look_ang>>1),cycloidy[*kb]+looking_arc+238-gun_pos,DEVISTATOR+i,-32,o,pal);
                            guniqhudid = cw<<1;
                            G_DrawWeaponTile(weapon_xoffset+30-(p->look_ang>>1),looking_arc+240-gun_pos,DEVISTATOR,gs,o|4,pal);
                            guniqhudid = 0;
                        }
                        else
                        {
                            guniqhudid = cw<<1;
                            G_DrawWeaponTile(-(cycloidy[*kb]>>1)+weapon_xoffset+30-(p->look_ang>>1),cycloidy[*kb]+looking_arc+240-gun_pos,DEVISTATOR+i,-32,o|4,pal);
                            guniqhudid = cw;
                            G_DrawWeaponTile(weapon_xoffset+268-(p->look_ang>>1),looking_arc+238-gun_pos,DEVISTATOR,gs,o,pal);
                            guniqhudid = 0;
                        }
                    }
                    else
                    {
                        guniqhudid = cw;
                        G_DrawWeaponTile(weapon_xoffset+268-(p->look_ang>>1),looking_arc+238-gun_pos,DEVISTATOR,gs,o,pal);
                        guniqhudid = cw<<1;
                        G_DrawWeaponTile(weapon_xoffset+30-(p->look_ang>>1),looking_arc+240-gun_pos,DEVISTATOR,gs,o|4,pal);
                        guniqhudid = 0;
                    }
                }
                break;

            case FREEZE_WEAPON:

                aGameVars[g_iReturnVarID].val.lValue = 0;
                if (apScriptGameEvent[EVENT_DRAWWEAPON])
                    VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek, -1);
                if (aGameVars[g_iReturnVarID].val.lValue == 0)
                {
                    if (sprite[p->i].pal == 1)
                        pal = 1;
                    else if (p->cursectnum >= 0)
                        pal = sector[p->cursectnum].floorpal;
                    else pal = 0;
                    if ((*kb) < (aplWeaponTotalTime[p->curr_weapon][snum]+1) && (*kb) > 0)
                    {
                        static uint8_t cat_frames[] = { 0,0,1,1,2,2 };

                        if (sprite[p->i].pal != 1)
                        {
                            weapon_xoffset += rand()&3;
                            looking_arc += rand()&3;
                        }
                        gun_pos -= 16;
                        guniqhudid = 0;
                        G_DrawWeaponTile(weapon_xoffset+210-(p->look_ang>>1),looking_arc+261-gun_pos,FREEZE+2,-32,o|512,pal);
                        guniqhudid = cw;
                        G_DrawWeaponTile(weapon_xoffset+210-(p->look_ang>>1),looking_arc+235-gun_pos,FREEZE+3+cat_frames[*kb%6],-32,o|512,pal);
                        guniqhudid = 0;
                    }
                    else
                    {
                        guniqhudid = cw;
                        G_DrawWeaponTile(weapon_xoffset+210-(p->look_ang>>1),looking_arc+261-gun_pos,FREEZE,gs,o|512,pal);
                        guniqhudid = 0;
                    }
                }
                break;

            case GROW_WEAPON:

                aGameVars[g_iReturnVarID].val.lValue = 0;
                if (apScriptGameEvent[EVENT_DRAWWEAPON])
                    VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek, -1);
                if (aGameVars[g_iReturnVarID].val.lValue == 0)
                {
                    weapon_xoffset += 28;
                    looking_arc += 18;
                    if (sprite[p->i].pal == 1)
                        pal = 1;
                    else if (p->cursectnum >= 0)
                        pal = sector[p->cursectnum].floorpal;
                    else pal = 0;
                    {
                        if ((*kb) < aplWeaponTotalTime[p->curr_weapon][snum] && (*kb) > 0)
                        {
                            if (sprite[p->i].pal != 1)
                            {
                                weapon_xoffset += rand()&3;
                                gun_pos += (rand()&3);
                            }

                            guniqhudid = cw<<1;
                            G_DrawWeaponTile(weapon_xoffset+184-(p->look_ang>>1),
                                             looking_arc+240-gun_pos,SHRINKER+3+((*kb)&3),-32,
                                             o,2);

                            guniqhudid = cw;
                            G_DrawWeaponTile(weapon_xoffset+188-(p->look_ang>>1),
                                             looking_arc+240-gun_pos,SHRINKER-1,gs,o,pal);
                            guniqhudid = 0;
                        }
                        else
                        {
                            guniqhudid = cw<<1;
                            G_DrawWeaponTile(weapon_xoffset+184-(p->look_ang>>1),
                                             looking_arc+240-gun_pos,SHRINKER+2,
                                             16-(sintable[p->random_club_frame&2047]>>10),
                                             o,2);

                            guniqhudid = cw;
                            G_DrawWeaponTile(weapon_xoffset+188-(p->look_ang>>1),
                                             looking_arc+240-gun_pos,SHRINKER-2,gs,o,pal);
                            guniqhudid = 0;
                        }
                    }
                }
                break;

            case SHRINKER_WEAPON:

                aGameVars[g_iReturnVarID].val.lValue = 0;
                if (apScriptGameEvent[EVENT_DRAWWEAPON])
                    VM_OnEvent(EVENT_DRAWWEAPON,g_player[screenpeek].ps->i,screenpeek, -1);
                if (aGameVars[g_iReturnVarID].val.lValue == 0)
                {
                    weapon_xoffset += 28;
                    looking_arc += 18;
                    if (sprite[p->i].pal == 1)
                        pal = 1;
                    else if (p->cursectnum >= 0)
                        pal = sector[p->cursectnum].floorpal;
                    else pal = 0;
                    if (((*kb) > 0) && ((*kb) < aplWeaponTotalTime[p->curr_weapon][snum]))
                    {
                        if (sprite[p->i].pal != 1)
                        {
                            weapon_xoffset += rand()&3;
                            gun_pos += (rand()&3);
                        }
                        guniqhudid = cw<<1;
                        G_DrawWeaponTile(weapon_xoffset+184-(p->look_ang>>1),
                                         looking_arc+240-gun_pos,SHRINKER+3+((*kb)&3),-32,
                                         o,0);
                        guniqhudid = cw;
                        G_DrawWeaponTile(weapon_xoffset+188-(p->look_ang>>1),
                                         looking_arc+240-gun_pos,SHRINKER+1,gs,o,pal);
                        guniqhudid = 0;

                    }
                    else
                    {
                        guniqhudid = cw<<1;
                        G_DrawWeaponTile(weapon_xoffset+184-(p->look_ang>>1),
                                         looking_arc+240-gun_pos,SHRINKER+2,
                                         16-(sintable[p->random_club_frame&2047]>>10),
                                         o,0);
                        guniqhudid = cw;
                        G_DrawWeaponTile(weapon_xoffset+188-(p->look_ang>>1),
                                         looking_arc+240-gun_pos,SHRINKER,gs,o,pal);
                        guniqhudid = 0;
                    }
                }
                break;

            }
    }
    P_DisplaySpit(snum);
}

#define TURBOTURNTIME (TICRATE/8) // 7
#define NORMALTURN   15
#define PREAMBLETURN 5
#define NORMALKEYMOVE 40
#define MAXVEL       ((NORMALKEYMOVE*2)+10)
#define MAXSVEL      ((NORMALKEYMOVE*2)+10)
#define MAXANGVEL    127
#define MAXHORIZ     127

int32_t g_myAimMode = 0, g_myAimStat = 0, g_oldAimStat = 0;
int32_t mouseyaxismode = -1;
int32_t jump_timer = 0;

void getinput(int32_t snum)
{
    int32_t j, daang;
    static ControlInfo info[2];
    static int32_t turnheldtime; //MED
    static int32_t lastcontroltime; //MED

    int32_t tics, running;
    int32_t turnamount;
    int32_t keymove;
    int32_t momx = 0,momy = 0;
    DukePlayer_t *p = g_player[snum].ps;

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
            P_DoQuote(44+g_myAimMode,p);
        }
    }

    if (g_myAimMode) j = analog_lookingupanddown;
    else j = ud.config.MouseAnalogueAxes[1];

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
            info[0].dpitch /= ud.config.MouseBias;
        else info[0].dyaw /= ud.config.MouseBias;
    }

    tics = totalclock-lastcontroltime;
    lastcontroltime = totalclock;

    //    running = BUTTON(gamefunc_Run)|ud.auto_run;
    // JBF: Run key behaviour is selectable
    if (ud.runkey_mode)
        running = BUTTON(gamefunc_Run)|ud.auto_run; // classic
    else
        running = ud.auto_run^BUTTON(gamefunc_Run); // modern

    svel = vel = angvel = horiz = 0;

    if (BUTTON(gamefunc_Strafe))
    {
        svel = -(info[0].dyaw+info[1].dyaw)/8;
        info[1].dyaw = (info[1].dyaw+info[0].dyaw) % 8;
    }
    else
    {
        angvel = (info[0].dyaw+info[1].dyaw)/64;
        info[1].dyaw = (info[1].dyaw+info[0].dyaw) % 64;
    }

    if (ud.mouseflip)
        horiz = -(info[0].dpitch+info[1].dpitch)/(314-128);
    else horiz = (info[0].dpitch+info[1].dpitch)/(314-128);

    info[1].dpitch = (info[1].dpitch+info[0].dpitch) % (314-128);

    svel -= info[0].dx;
    info[1].dz = info[0].dz % (1<<6);
    vel = -info[0].dz>>6;

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
            svel -= -keymove;
        if (BUTTON(gamefunc_Turn_Right) && !(g_player[snum].ps->movement_lock&8))
            svel -= keymove;
    }
    else
    {
        if (BUTTON(gamefunc_Turn_Left))
        {
            turnheldtime += tics;
            if (turnheldtime>=TURBOTURNTIME)
                angvel -= turnamount;
            else
                angvel -= PREAMBLETURN;
        }
        else if (BUTTON(gamefunc_Turn_Right))
        {
            turnheldtime += tics;
            if (turnheldtime>=TURBOTURNTIME)
                angvel += turnamount;
            else
                angvel += PREAMBLETURN;
        }
        else
            turnheldtime=0;
    }

    if (BUTTON(gamefunc_Strafe_Left) && !(g_player[snum].ps->movement_lock&4))
        svel += keymove;
    if (BUTTON(gamefunc_Strafe_Right) && !(g_player[snum].ps->movement_lock&8))
        svel += -keymove;
    if (BUTTON(gamefunc_Move_Forward) && !(g_player[snum].ps->movement_lock&1))
        vel += keymove;
    if (BUTTON(gamefunc_Move_Backward) && !(g_player[snum].ps->movement_lock&2))
        vel += -keymove;

    if (vel < -MAXVEL) vel = -MAXVEL;
    if (vel > MAXVEL) vel = MAXVEL;
    if (svel < -MAXSVEL) svel = -MAXSVEL;
    if (svel > MAXSVEL) svel = MAXSVEL;
    if (angvel < -MAXANGVEL) angvel = -MAXANGVEL;
    if (angvel > MAXANGVEL) angvel = MAXANGVEL;
    if (horiz < -MAXHORIZ) horiz = -MAXHORIZ;
    if (horiz > MAXHORIZ) horiz = MAXHORIZ;

    j=0;

    if (BUTTON(gamefunc_Weapon_1))
        j = 1;
    if (BUTTON(gamefunc_Weapon_2))
        j = 2;
    if (BUTTON(gamefunc_Weapon_3))
        j = 3;
    if (BUTTON(gamefunc_Weapon_4))
        j = 4;
    if (BUTTON(gamefunc_Weapon_5))
        j = 5;
    if (BUTTON(gamefunc_Weapon_6))
        j = 6;
    if (BUTTON(gamefunc_Weapon_7))
        j = 7;
    if (BUTTON(gamefunc_Weapon_8))
        j = 8;
    if (BUTTON(gamefunc_Weapon_9))
        j = 9;
    if (BUTTON(gamefunc_Weapon_10))
        j = 10;
    if (BUTTON(gamefunc_Previous_Weapon) || (BUTTON(gamefunc_Dpad_Select) && vel < 0))
        j = 11;
    if (BUTTON(gamefunc_Next_Weapon) || (BUTTON(gamefunc_Dpad_Select) && vel > 0))
        j = 12;

    if (BUTTON(gamefunc_Jump) && p->on_ground)
        jump_timer = 4;

    loc.bits = (jump_timer > 0 || BUTTON(gamefunc_Jump))<<SK_JUMP;

    if (jump_timer > 0)
        jump_timer--;

    loc.bits |=   BUTTON(gamefunc_Crouch)<<SK_CROUCH;
    loc.bits |=   BUTTON(gamefunc_Fire)<<SK_FIRE;
    loc.bits |= (BUTTON(gamefunc_Aim_Up) || (BUTTON(gamefunc_Dpad_Aiming) && vel > 0))<<SK_AIM_UP;
    loc.bits |= (BUTTON(gamefunc_Aim_Down) || (BUTTON(gamefunc_Dpad_Aiming) && vel < 0))<<SK_AIM_DOWN;
    if (ud.runkey_mode) loc.bits |= (ud.auto_run | BUTTON(gamefunc_Run))<<SK_RUN;
    else loc.bits |= (BUTTON(gamefunc_Run) ^ ud.auto_run)<<SK_RUN;
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
    loc.bits |= (BUTTON(gamefunc_Inventory_Left) || (BUTTON(gamefunc_Dpad_Select) && (svel > 0 || angvel < 0))) <<SK_INV_LEFT;
    loc.bits |=   KB_KeyPressed(sc_Pause)<<SK_PAUSE;
    loc.bits |=   BUTTON(gamefunc_Quick_Kick)<<SK_QUICK_KICK;
    loc.bits |=   g_myAimMode<<SK_AIMMODE;
    loc.bits |=   BUTTON(gamefunc_Holo_Duke)<<SK_HOLODUKE;
    loc.bits |=   BUTTON(gamefunc_Jetpack)<<SK_JETPACK;
    loc.bits |= (((int32_t)g_gameQuit)<<SK_GAMEQUIT);
    loc.bits |= (BUTTON(gamefunc_Inventory_Right) || (BUTTON(gamefunc_Dpad_Select) && (svel < 0 || angvel > 0))) <<SK_INV_RIGHT;
    loc.bits |=   BUTTON(gamefunc_TurnAround)<<SK_TURNAROUND;
    loc.bits |=   BUTTON(gamefunc_Open)<<SK_OPEN;
    loc.bits |=   BUTTON(gamefunc_Inventory)<<SK_INVENTORY;
    loc.bits |=   KB_KeyPressed(sc_Escape)<<SK_ESCAPE;

    if (BUTTON(gamefunc_Dpad_Select))
        vel = svel = angvel = 0;

    if (BUTTON(gamefunc_Dpad_Aiming))
        vel = 0;

    if (aplWeaponFlags[g_player[snum].ps->curr_weapon][snum] & WEAPON_SEMIAUTO && BUTTON(gamefunc_Fire))
        CONTROL_ClearButton(gamefunc_Fire);

    loc.extbits = 0;
//    if (apScriptGameEvent[EVENT_PROCESSINPUT] || apScriptGameEvent[EVENT_MOVEFORWARD])
    loc.extbits |= BUTTON(gamefunc_Move_Forward) || (vel > 0);
//    if (apScriptGameEvent[EVENT_PROCESSINPUT] || apScriptGameEvent[EVENT_MOVEBACKWARD])
    loc.extbits |= (BUTTON(gamefunc_Move_Backward) || (vel < 0))<<1;
//    if (apScriptGameEvent[EVENT_PROCESSINPUT] || apScriptGameEvent[EVENT_STRAFELEFT])
    loc.extbits |= (BUTTON(gamefunc_Strafe_Left) || (svel > 0))<<2;
//    if (apScriptGameEvent[EVENT_PROCESSINPUT] || apScriptGameEvent[EVENT_STRAFERIGHT])
    loc.extbits |= (BUTTON(gamefunc_Strafe_Right) || (svel < 0))<<3;
    if (apScriptGameEvent[EVENT_PROCESSINPUT] || apScriptGameEvent[EVENT_TURNLEFT])
        loc.extbits |= BUTTON(gamefunc_Turn_Left)<<4;
    if (apScriptGameEvent[EVENT_PROCESSINPUT] || apScriptGameEvent[EVENT_TURNRIGHT])
        loc.extbits |= BUTTON(gamefunc_Turn_Right)<<5;
    // used for changing team
    loc.extbits |= (g_player[snum].pteam != g_player[snum].ps->team)<<6;

    if (ud.scrollmode && ud.overhead_on)
    {
        ud.folfvel = vel;
        ud.folavel = angvel;
        loc.fvel = loc.svel = loc.avel = loc.horz = 0;
        return;
    }

    daang = p->ang;

    momx = mulscale9(vel,sintable[(daang+2560)&2047]);
    momy = mulscale9(vel,sintable[(daang+2048)&2047]);

    momx += mulscale9(svel,sintable[(daang+2048)&2047]);
    momy += mulscale9(svel,sintable[(daang+1536)&2047]);

    momx += fricxv;
    momy += fricyv;

    loc.fvel = momx;
    loc.svel = momy;

    loc.avel = angvel;
    loc.horz = horiz;
}

static int32_t P_DoCounters(DukePlayer_t *p)
{
    int32_t snum = sprite[p->i].yvel;

//        j = g_player[snum].sync->avel;
//        p->weapon_ang = -(j/5);

    if (p->invdisptime > 0)
        p->invdisptime--;

    if (p->tipincs > 0) p->tipincs--;

    if (p->last_pissed_time > 0)
    {
        p->last_pissed_time--;

        if (p->last_pissed_time == (GAMETICSPERSEC*219))
        {
            A_PlaySound(FLUSH_TOILET,p->i);
            if (snum == screenpeek || GTFLAGS(GAMETYPE_COOPSOUND))
                A_PlaySound(DUKE_PISSRELIEF,p->i);
        }

        if (p->last_pissed_time == (GAMETICSPERSEC*218))
        {
            p->holster_weapon = 0;
            p->weapon_pos = 10;
        }
    }

    if (p->crack_time > 0)
    {
        p->crack_time--;
        if (p->crack_time == 0)
        {
            p->knuckle_incs = 1;
            p->crack_time = 777;
        }
    }

    if (p->inv_amount[GET_STEROIDS] > 0 && p->inv_amount[GET_STEROIDS] < 400)
    {
        p->inv_amount[GET_STEROIDS]--;
        if (p->inv_amount[GET_STEROIDS] == 0)
            P_SelectNextInvItem(p);
        if (!(p->inv_amount[GET_STEROIDS]&7))
            if (snum == screenpeek || GTFLAGS(GAMETYPE_COOPSOUND))
                A_PlaySound(DUKE_HARTBEAT,p->i);
    }

    if (p->heat_on && p->inv_amount[GET_HEATS] > 0)
    {
        p->inv_amount[GET_HEATS]--;
        if (p->inv_amount[GET_HEATS] == 0)
        {
            p->heat_on = 0;
            P_SelectNextInvItem(p);
            A_PlaySound(NITEVISION_ONOFF,p->i);
            P_UpdateScreenPal(p);
        }
    }

    if (p->holoduke_on >= 0)
    {
        p->inv_amount[GET_HOLODUKE]--;
        if (p->inv_amount[GET_HOLODUKE] <= 0)
        {
            A_PlaySound(TELEPORTER,p->i);
            p->holoduke_on = -1;
            P_SelectNextInvItem(p);
        }
    }

    if (p->jetpack_on && p->inv_amount[GET_JETPACK] > 0)
    {
        p->inv_amount[GET_JETPACK]--;
        if (p->inv_amount[GET_JETPACK] <= 0)
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
        p->quick_kick--;
        if (p->quick_kick == 8)
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
            p->weapon_pos = 10;
            p->kickback_pic = 0;
        }
    }

    if (p->cursectnum >= 0 && p->scuba_on == 0 && sector[p->cursectnum].lotag == 2)
    {
        if (p->inv_amount[GET_SCUBA] > 0)
        {
            p->scuba_on = 1;
            p->inven_icon = 6;
            P_DoQuote(76,p);
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
        p->knuckle_incs++;
        if (p->knuckle_incs==10)
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
void P_DropWeapon(DukePlayer_t *p)
{
    int32_t snum = sprite[p->i].yvel,
            cw = aplWeaponWorksLike[p->curr_weapon][snum];

    if (cw < 1 || cw >= MAX_WEAPONS) return;

    if (krand()&1)
        A_Spawn(p->i,WeaponPickupSprites[cw]);
    else switch (cw)
        {
        case RPG_WEAPON:
        case HANDBOMB_WEAPON:
            A_Spawn(p->i,EXPLOSION2);
            break;
        }
}

void P_AddAmmo(int32_t weapon,DukePlayer_t *p,int32_t amount)
{
    p->ammo_amount[weapon] += amount;

    if (p->ammo_amount[weapon] > p->max_ammo_amount[weapon])
        p->ammo_amount[weapon] = p->max_ammo_amount[weapon];
}

void P_AddWeaponNoSwitch(DukePlayer_t *p, int32_t weapon)
{
    int32_t snum = sprite[p->i].yvel;

    if ((p->gotweapon & (1<<weapon)) == 0)
    {
        p->gotweapon |= (1<<weapon);
        if (weapon == SHRINKER_WEAPON)
            p->gotweapon |= (1<<GROW_WEAPON);
    }

    if (aplWeaponSelectSound[p->curr_weapon][snum] > 0)
        S_StopEnvSound(aplWeaponSelectSound[p->curr_weapon][snum],p->i);

    if (aplWeaponSelectSound[weapon][snum] > 0)
        A_PlaySound(aplWeaponSelectSound[weapon][snum],p->i);
}

void P_AddWeapon(DukePlayer_t *p,int32_t weapon)
{
    int32_t snum = sprite[p->i].yvel;

    P_AddWeaponNoSwitch(p,weapon);

    if (p->reloading) return;

    p->random_club_frame = 0;

    if (p->holster_weapon == 0)
    {
        if (p->weapon_pos == 0)
            p->weapon_pos = -1;
        else p->weapon_pos = -9;
        p->last_weapon = p->curr_weapon;
    }
    else
    {
        p->weapon_pos = 10;
        p->holster_weapon = 0;
        p->last_weapon = -1;
    }

    p->kickback_pic = 0;

    if (p->curr_weapon != weapon && apScriptGameEvent[EVENT_CHANGEWEAPON])
        VM_OnEvent(EVENT_CHANGEWEAPON,p->i, snum, -1);

    p->curr_weapon = weapon;

    Gv_SetVar(g_iWeaponVarID,p->curr_weapon, p->i, snum);
    Gv_SetVar(g_iWorksLikeVarID,
              (p->curr_weapon>=0) ? aplWeaponWorksLike[p->curr_weapon][snum] : -1,
              p->i, snum);
}

void P_SelectNextInvItem(DukePlayer_t *p)
{
    if (p->inv_amount[GET_FIRSTAID] > 0)
        p->inven_icon = 1;
    else if (p->inv_amount[GET_STEROIDS] > 0)
        p->inven_icon = 2;
    else if (p->inv_amount[GET_HOLODUKE] > 0)
        p->inven_icon = 3;
    else if (p->inv_amount[GET_JETPACK] > 0)
        p->inven_icon = 4;
    else if (p->inv_amount[GET_HEATS] > 0)
        p->inven_icon = 5;
    else if (p->inv_amount[GET_SCUBA] > 0)
        p->inven_icon = 6;
    else if (p->inv_amount[GET_BOOTS] > 0)
        p->inven_icon = 7;
    else p->inven_icon = 0;
}

void P_CheckWeapon(DukePlayer_t *p)
{
    int32_t i, snum, weap;

    if (p->reloading) return;

    if (p->wantweaponfire >= 0)
    {
        weap = p->wantweaponfire;
        p->wantweaponfire = -1;

        if (weap == p->curr_weapon) return;
        if ((p->gotweapon & (1<<weap)) && p->ammo_amount[weap] > 0)
        {
            P_AddWeapon(p,weap);
            return;
        }
    }

    weap = p->curr_weapon;
    if ((p->gotweapon & (1<<weap)) && p->ammo_amount[weap] > 0)
        return;
    if ((p->gotweapon & (1<<weap)) && !(p->weaponswitch & 2))
        return;

    snum = sprite[p->i].yvel;

    for (i=0; i<10; i++)
    {
        weap = g_player[snum].wchoice[i];
        if (VOLUMEONE && weap > 6) continue;

        if (weap == 0) weap = 9;
        else weap--;

        if (weap == 0 || ((p->gotweapon & (1<<weap)) && p->ammo_amount[weap] > 0))
            break;
    }

    if (i == 10) weap = 0;

    // Found the weapon

    p->last_weapon  = p->curr_weapon;
    p->random_club_frame = 0;
    p->curr_weapon  = weap;
    Gv_SetVar(g_iWeaponVarID,p->curr_weapon, p->i, snum);
    Gv_SetVar(g_iWorksLikeVarID, p->curr_weapon >= 0 ? aplWeaponWorksLike[p->curr_weapon][snum] : -1, p->i, snum);

    if (apScriptGameEvent[EVENT_CHANGEWEAPON])
        VM_OnEvent(EVENT_CHANGEWEAPON,p->i, snum, -1);
    p->kickback_pic = 0;
    if (p->holster_weapon == 1)
    {
        p->holster_weapon = 0;
        p->weapon_pos = 10;
    }
    else p->weapon_pos   = -1;
}

void P_CheckTouchDamage(DukePlayer_t *p,int32_t j)
{
    if ((j&49152) == 49152)
    {
        j &= (MAXSPRITES-1);

        if (sprite[j].picnum == CACTUS)
        {
            if (p->hurt_delay < 8)
            {
                sprite[p->i].extra -= 5;

                p->hurt_delay = 16;
                p->pals.f = 32;
                p->pals.r = 32;
                p->pals.g = 0;
                p->pals.b = 0;
                A_PlaySound(DUKE_LONGTERM_PAIN,p->i);
            }
        }
        return;
    }

    if ((j&49152) != 32768) return;
    j &= (MAXWALLS-1);

    if (p->hurt_delay > 0) p->hurt_delay--;
    else if (wall[j].cstat&85)
    {
        int32_t switchpicnum = wall[j].overpicnum;
        if ((switchpicnum>W_FORCEFIELD)&&(switchpicnum<=W_FORCEFIELD+2))
            switchpicnum=W_FORCEFIELD;

        switch (DynamicTileMap[switchpicnum])
        {
        case W_FORCEFIELD__STATIC:
            //        case W_FORCEFIELD+1:
            //        case W_FORCEFIELD+2:
            sprite[p->i].extra -= 5;

            p->hurt_delay = 16;
            p->pals.f = 32;
            p->pals.r = 32;
            p->pals.g = 0;
            p->pals.b = 0;

            p->posvel.x = -(sintable[(p->ang+512)&2047]<<8);
            p->posvel.y = -(sintable[(p->ang)&2047]<<8);
            A_PlaySound(DUKE_LONGTERM_PAIN,p->i);

            {
                vec3_t davect;

                davect.x = p->pos.x+(sintable[(p->ang+512)&2047]>>9);
                davect.y = p->pos.y+(sintable[p->ang&2047]>>9);
                davect.z = p->pos.z;
                A_DamageWall(p->i,j,&davect,-1);
            }

            break;

        case BIGFORCE__STATIC:
            p->hurt_delay = GAMETICSPERSEC;
            {
                vec3_t davect;

                davect.x = p->pos.x+(sintable[(p->ang+512)&2047]>>9);
                davect.y = p->pos.y+(sintable[p->ang&2047]>>9);
                davect.z = p->pos.z;
                A_DamageWall(p->i,j,&davect,-1);
            }
            break;

        }
    }
}

int32_t P_CheckFloorDamage(DukePlayer_t *p, int32_t j)
{
    int32_t ret = 0;
    spritetype *s = &sprite[p->i];

    switch (DynamicTileMap[j])
    {
    case HURTRAIL__STATIC:
        if (rnd(32))
        {
            if (p->inv_amount[GET_BOOTS] > 0)
                ret++;
            else
            {
                if (!A_CheckSoundPlaying(p->i,DUKE_LONGTERM_PAIN))
                    A_PlaySound(DUKE_LONGTERM_PAIN,p->i);
                p->pals.r = 64;
                p->pals.g = 64;
                p->pals.b = 64;
                p->pals.f = 32;
                s->extra -= 1+(krand()&3);
                if (!A_CheckSoundPlaying(p->i,SHORT_CIRCUIT))
                    A_PlaySound(SHORT_CIRCUIT,p->i);
            }
        }
        break;
    case FLOORSLIME__STATIC:
        if (rnd(16))
        {
            if (p->inv_amount[GET_BOOTS] > 0)
                ret++;
            else
            {
                if (!A_CheckSoundPlaying(p->i,DUKE_LONGTERM_PAIN))
                    A_PlaySound(DUKE_LONGTERM_PAIN,p->i);
                p->pals.r = 0;
                p->pals.g = 8;
                p->pals.b = 0;
                p->pals.f = 32;
                s->extra -= 1+(krand()&3);
            }
        }
        break;
    case FLOORPLASMA__STATIC:
        if (rnd(32))
        {
            if (p->inv_amount[GET_BOOTS] > 0)
                ret++;
            else
            {
                if (!A_CheckSoundPlaying(p->i,DUKE_LONGTERM_PAIN))
                    A_PlaySound(DUKE_LONGTERM_PAIN,p->i);
                p->pals.r = 8;
                p->pals.g = 0;
                p->pals.b = 0;
                p->pals.f = 32;
                s->extra -= 1+(krand()&3);
            }
        }
        break;
    }

    return ret;
}


int32_t P_FindOtherPlayer(int32_t p,int32_t *d)
{
    int32_t j, closest_player = p;
    int32_t x, closest = 0x7fffffff;

    TRAVERSE_CONNECT(j)
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
        p->pals.r = 63;
        p->pals.g = 0;
        p->pals.b = 0;
        p->pals.f = 63;
        p->pos.z -= (16<<8);
        s->z -= (16<<8);

        p->dead_flag = (512-((krand()&1)<<10)+(krand()&255)-512)&2047;
        if (p->dead_flag == 0)
            p->dead_flag++;

        if (g_netServer)
        {
            packbuf[0] = PACKET_FRAG;
            packbuf[1] = snum;
            packbuf[2] = p->frag_ps;
            packbuf[3] = actor[p->i].picnum;
            *(int32_t *)&packbuf[4] = ticrandomseed;
            packbuf[8] = myconnectindex;

            enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, 9, ENET_PACKET_FLAG_RELIABLE));
        }
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
                Bsprintf(ScriptQuotes[115],"KILLED BY %s",&g_player[p->frag_ps].user_name[0]);
                P_DoQuote(115,p);
            }
            else
            {
                Bsprintf(ScriptQuotes[116],"KILLED %s",&g_player[snum].user_name[0]);
                P_DoQuote(116,g_player[p->frag_ps].ps);
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
                if (A_CheckEnemyTile(sprite[p->wackedbyactor].picnum))
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

void P_ProcessWeapon(int32_t snum)
{
    DukePlayer_t *p = g_player[snum].ps;
    uint8_t *kb = &p->kickback_pic;
    int32_t shrunk = (sprite[p->i].yrepeat < 32);
    uint32_t sb_snum = g_player[snum].sync->bits;
    int32_t i, j, k;

    switch (p->weapon_pos)
    {
    case -9:
        if (p->last_weapon >= 0)
        {
            p->weapon_pos = 10;
            p->last_weapon = -1;
        }
        else if (p->holster_weapon == 0)
            p->weapon_pos = 10;
        break;
    case 0:
        break;
    default:
        p->weapon_pos--;
        break;
    }

    if (TEST_SYNC_KEY(sb_snum, SK_FIRE))
    {
        Gv_SetVar(g_iWeaponVarID,p->curr_weapon,p->i,snum);
        Gv_SetVar(g_iWorksLikeVarID,aplWeaponWorksLike[p->curr_weapon][snum],p->i,snum);
        aGameVars[g_iReturnVarID].val.lValue = 0;
        VM_OnEvent(EVENT_PRESSEDFIRE, p->i, snum, -1);
        if (aGameVars[g_iReturnVarID].val.lValue != 0)
            sb_snum &= ~BIT(SK_FIRE);
    }

    if (TEST_SYNC_KEY(sb_snum, SK_HOLSTER))   // 'Holster Weapon
    {
        aGameVars[g_iReturnVarID].val.lValue = 0;
        Gv_SetVar(g_iWeaponVarID,p->curr_weapon,p->i,snum);
        Gv_SetVar(g_iWorksLikeVarID,aplWeaponWorksLike[p->curr_weapon][snum],p->i,snum);
        VM_OnEvent(EVENT_HOLSTER, p->i, snum, -1);
        if (aGameVars[g_iReturnVarID].val.lValue == 0)
        {
            if (*aplWeaponWorksLike[p->curr_weapon] != KNEE_WEAPON)
            {
                if (p->holster_weapon == 0 && p->weapon_pos == 0)
                {
                    p->holster_weapon = 1;
                    p->weapon_pos = -1;
                    P_DoQuote(73,p);
                }
                else if (p->holster_weapon == 1 && p->weapon_pos == -9)
                {
                    p->holster_weapon = 0;
                    p->weapon_pos = 10;
                    P_DoQuote(74,p);
                }
            }

            if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_HOLSTER_CLEARS_CLIP)
            {
                if (p->ammo_amount[p->curr_weapon] > aplWeaponClip[p->curr_weapon][snum]
                        && (p->ammo_amount[p->curr_weapon] % aplWeaponClip[p->curr_weapon][snum]) != 0)
                {
                    p->ammo_amount[p->curr_weapon]-=
                        p->ammo_amount[p->curr_weapon] % aplWeaponClip[p->curr_weapon][snum] ;
                    (*kb) = aplWeaponTotalTime[p->curr_weapon][snum];
                    sb_snum &= ~BIT(SK_FIRE); // not firing...
                }
                return;
            }
        }
    }

    if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_GLOWS)
    {
        p->random_club_frame += 64; // Glowing

        if (p->kickback_pic == 0)
        {
            spritetype *s = &sprite[p->i];
            int32_t x = ((sintable[(s->ang+512)&2047])>>7), y = ((sintable[(s->ang)&2047])>>7);
            int32_t r = 1024+(sintable[p->random_club_frame&2047]>>3);

            s->x += x;
            s->y += y;
            G_AddGameLight(0, p->i, PHEIGHT, max(r, 0), aplWeaponFlashColor[p->curr_weapon][snum],PR_LIGHT_PRIO_HIGH_GAME);
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
            if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_AMMOPERSHOT)
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
                A_Shoot(p->i,aplWeaponShoots[p->curr_weapon][snum]);
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
            if (p->last_pissed_time <= (GAMETICSPERSEC*218) && p->weapon_pos == -9)
            {
                p->holster_weapon = 0;
                p->weapon_pos = 10;
                P_DoQuote(74,p);
            }
        }
        else
        {
            aGameVars[g_iReturnVarID].val.lValue = 0;
            Gv_SetVar(g_iWeaponVarID,p->curr_weapon,p->i,snum);
            Gv_SetVar(g_iWorksLikeVarID,aplWeaponWorksLike[p->curr_weapon][snum],p->i,snum);

            if (apScriptGameEvent[EVENT_FIRE])
                VM_OnEvent(EVENT_FIRE, p->i, snum, -1);

            if (aGameVars[g_iReturnVarID].val.lValue == 0)
            {
                if (apScriptGameEvent[EVENT_FIREWEAPON]) // this event is deprecated
                    VM_OnEvent(EVENT_FIREWEAPON, p->i, snum, -1);

                switch (aplWeaponWorksLike[p->curr_weapon][snum])
                {
                case HANDBOMB_WEAPON:
                    p->hbomb_hold_delay = 0;
                    if (p->ammo_amount[p->curr_weapon] > 0)
                    {
                        (*kb)=1;
                        if (aplWeaponInitialSound[p->curr_weapon][snum] > 0)
                            A_PlaySound(aplWeaponInitialSound[p->curr_weapon][snum], p->i);
                    }
                    break;

                case HANDREMOTE_WEAPON:
                    p->hbomb_hold_delay = 0;
                    (*kb) = 1;
                    if (aplWeaponInitialSound[p->curr_weapon][snum] > 0)
                        A_PlaySound(aplWeaponInitialSound[p->curr_weapon][snum], p->i);
                    break;

                case SHOTGUN_WEAPON:
                    if (p->ammo_amount[p->curr_weapon] > 0 && p->random_club_frame == 0)
                    {
                        (*kb)=1;
                        if (aplWeaponInitialSound[p->curr_weapon][snum] > 0)
                            A_PlaySound(aplWeaponInitialSound[p->curr_weapon][snum], p->i);
                    }
                    break;

                case TRIPBOMB_WEAPON:
                    if (p->ammo_amount[p->curr_weapon] > 0)
                    {
                        hitdata_t hitinfo;
                        hitscan((const vec3_t *)p,
                                p->cursectnum, sintable[(p->ang+512)&2047],
                                sintable[p->ang&2047], (100-p->horiz-p->horizoff)*32,
                                &hitinfo,CLIPMASK1);

                        if (hitinfo.hitsect < 0 || hitinfo.hitsprite >= 0)
                            break;

                        if (hitinfo.hitwall >= 0 && sector[hitinfo.hitsect].lotag > 2)
                            break;

                        if (hitinfo.hitwall >= 0 && wall[hitinfo.hitwall].overpicnum >= 0)
                            if (wall[hitinfo.hitwall].overpicnum == BIGFORCE)
                                break;

                        j = headspritesect[hitinfo.hitsect];
                        while (j >= 0)
                        {
                            if (sprite[j].picnum == TRIPBOMB &&
                                    klabs(sprite[j].z-hitinfo.pos.z) < (12<<8) &&
                                    ((sprite[j].x-hitinfo.pos.x)*(sprite[j].x-hitinfo.pos.x)+
                                     (sprite[j].y-hitinfo.pos.y)*(sprite[j].y-hitinfo.pos.y)) < (290*290))
                                break;
                            j = nextspritesect[j];
                        }

                        if (j == -1 && hitinfo.hitwall >= 0 && (wall[hitinfo.hitwall].cstat&16) == 0)
                            if ((wall[hitinfo.hitwall].nextsector >= 0 &&
                                    sector[wall[hitinfo.hitwall].nextsector].lotag <= 2) ||
                                    (wall[hitinfo.hitwall].nextsector == -1 && sector[hitinfo.hitsect].lotag <= 2))
                                if (((hitinfo.pos.x-p->pos.x)*(hitinfo.pos.x-p->pos.x) +
                                        (hitinfo.pos.y-p->pos.y)*(hitinfo.pos.y-p->pos.y)) < (290*290))
                                {
                                    p->pos.z = p->opos.z;
                                    p->posvel.z = 0;
                                    (*kb) = 1;
                                    if (aplWeaponInitialSound[p->curr_weapon][snum] > 0)
                                    {
                                        A_PlaySound(aplWeaponInitialSound[p->curr_weapon][snum], p->i);
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
                        if (aplWeaponInitialSound[p->curr_weapon][snum] > 0)
                            A_PlaySound(aplWeaponInitialSound[p->curr_weapon][snum], p->i);
                    }
                    break;

                case DEVISTATOR_WEAPON:
                    if (p->ammo_amount[p->curr_weapon] > 0)
                    {
                        (*kb) = 1;
                        p->hbomb_hold_delay = !p->hbomb_hold_delay;
                        if (aplWeaponInitialSound[p->curr_weapon][snum] > 0)
                            A_PlaySound(aplWeaponInitialSound[p->curr_weapon][snum], p->i);
                    }
                    break;

                case KNEE_WEAPON:
                    if (p->quick_kick == 0)
                    {
                        (*kb) = 1;
                        if (aplWeaponInitialSound[p->curr_weapon][snum] > 0)
                            A_PlaySound(aplWeaponInitialSound[p->curr_weapon][snum], p->i);
                    }
                    break;
                }
            }
        }
    }
    else if (*kb)
    {
        if (aplWeaponWorksLike[p->curr_weapon][snum] == HANDBOMB_WEAPON)
        {
            if (aplWeaponHoldDelay[p->curr_weapon][snum] && ((*kb) == aplWeaponFireDelay[p->curr_weapon][snum]) && TEST_SYNC_KEY(sb_snum, SK_FIRE))
            {
                p->rapid_fire_hold = 1;
                return;
            }

            if (++(*kb) == aplWeaponHoldDelay[p->curr_weapon][snum])
            {
                int32_t lPipeBombControl;

                p->ammo_amount[p->curr_weapon]--;

                if (numplayers < 2 || g_netServer)
                {
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
                                       p->pos.z,aplWeaponShoots[p->curr_weapon][snum],-16,9,9,
                                       p->ang,(k+(p->hbomb_hold_delay<<5)),i,p->i,1);

                    lPipeBombControl=Gv_GetVarByLabel("PIPEBOMB_CONTROL", PIPEBOMB_REMOTE, -1, snum);

                    if (lPipeBombControl & PIPEBOMB_TIMER)
                    {
                        int32_t lv=Gv_GetVarByLabel("GRENADE_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, -1, snum);

                        actor[j].t_data[7]= Gv_GetVarByLabel("GRENADE_LIFETIME", NAM_GRENADE_LIFETIME, -1, snum)
                                            + mulscale(krand(),lv, 14)
                                            - lv;
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
            else if ((*kb) < aplWeaponHoldDelay[p->curr_weapon][snum] && TEST_SYNC_KEY(sb_snum, SK_FIRE))
                p->hbomb_hold_delay++;
            else if ((*kb) > aplWeaponTotalTime[p->curr_weapon][snum])
            {
                (*kb) = 0;
                p->weapon_pos = 10;

                if (Gv_GetVarByLabel("PIPEBOMB_CONTROL", PIPEBOMB_REMOTE, -1, snum) == PIPEBOMB_REMOTE)
                {
                    p->curr_weapon = HANDREMOTE_WEAPON;
                    p->last_weapon = -1;
                }
                else P_CheckWeapon(p);
            }
        }
        else if (aplWeaponWorksLike[p->curr_weapon][snum] == HANDREMOTE_WEAPON)
        {
            if (++(*kb) == aplWeaponFireDelay[p->curr_weapon][snum])
            {
                if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_BOMB_TRIGGER)
                    p->hbomb_on = 0;

                if (aplWeaponShoots[p->curr_weapon][snum] != 0)
                {
                    if (!(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_NOVISIBLE))
                    {
                        lastvisinc = totalclock+32;
                        p->visibility = 0;
                    }
                    Gv_SetVar(g_iWeaponVarID,p->curr_weapon,p->i,snum);
                    Gv_SetVar(g_iWorksLikeVarID,aplWeaponWorksLike[p->curr_weapon][snum], p->i, snum);
                    A_Shoot(p->i, aplWeaponShoots[p->curr_weapon][snum]);
                }
            }

            if ((*kb) >= aplWeaponTotalTime[p->curr_weapon][snum])
            {
                (*kb) = 0;
                if ((p->ammo_amount[HANDBOMB_WEAPON] > 0) &&
                        Gv_GetVarByLabel("PIPEBOMB_CONTROL", PIPEBOMB_REMOTE, -1, snum) == PIPEBOMB_REMOTE)
                    P_AddWeapon(p,HANDBOMB_WEAPON);
                else P_CheckWeapon(p);
            }
        }
        else
        {
            // the basic weapon...
            (*kb)++;

            if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_CHECKATRELOAD)
            {
                if (aplWeaponWorksLike[p->curr_weapon][snum] == TRIPBOMB_WEAPON)
                {
                    if ((*kb) >= aplWeaponTotalTime[p->curr_weapon][snum])
                    {
                        (*kb) = 0;
                        P_CheckWeapon(p);
                        p->weapon_pos = -9;
                    }
                }
                else if (*kb >= aplWeaponReload[p->curr_weapon][snum])
                    P_CheckWeapon(p);
            }
            else if (aplWeaponWorksLike[p->curr_weapon][snum]!=KNEE_WEAPON && *kb >= aplWeaponFireDelay[p->curr_weapon][snum])
                P_CheckWeapon(p);

            if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_STANDSTILL
                    && *kb < (aplWeaponFireDelay[p->curr_weapon][snum]+1))
            {
                p->pos.z = p->opos.z;
                p->posvel.z = 0;
            }

            if (*kb == aplWeaponSound2Time[p->curr_weapon][snum])
                if (aplWeaponSound2Sound[p->curr_weapon][snum] > 0)
                    A_PlaySound(aplWeaponSound2Sound[p->curr_weapon][snum],p->i);

            if (*kb == aplWeaponSpawnTime[p->curr_weapon][snum])
                P_DoWeaponSpawn(p);

            if ((*kb) >= aplWeaponTotalTime[p->curr_weapon][snum])
            {
                if (/*!(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_CHECKATRELOAD) && */ p->reloading == 1 ||
                        (aplWeaponReload[p->curr_weapon][snum] > aplWeaponTotalTime[p->curr_weapon][snum] && p->ammo_amount[p->curr_weapon] > 0
                         && (aplWeaponClip[p->curr_weapon][snum]) && (((p->ammo_amount[p->curr_weapon]%(aplWeaponClip[p->curr_weapon][snum]))==0))))
                {
                    int32_t i = aplWeaponReload[p->curr_weapon][snum] - aplWeaponTotalTime[p->curr_weapon][snum];

                    p->reloading = 1;

                    if ((*kb) != (aplWeaponTotalTime[p->curr_weapon][snum]))
                    {
                        if ((*kb) == (aplWeaponTotalTime[p->curr_weapon][snum]+1))
                        {
                            if (aplWeaponReloadSound1[p->curr_weapon][snum] > 0)
                                A_PlaySound(aplWeaponReloadSound1[p->curr_weapon][snum],p->i);
                        }
                        else if (((*kb) == (aplWeaponReload[p->curr_weapon][snum] - (i/3)) &&
                                  !(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_RELOAD_TIMING)) ||

                                 ((*kb) == (aplWeaponReload[p->curr_weapon][snum] - i+4) &&
                                  (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_RELOAD_TIMING)))
                        {
                            if (aplWeaponReloadSound2[p->curr_weapon][snum] > 0)
                                A_PlaySound(aplWeaponReloadSound2[p->curr_weapon][snum],p->i);
                        }
                        else if ((*kb) >= (aplWeaponReload[p->curr_weapon][snum]))
                        {
                            *kb=0;
                            p->reloading = 0;
                        }
                    }
                }
                else
                {
                    if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_AUTOMATIC &&
                            (aplWeaponWorksLike[p->curr_weapon][snum]==KNEE_WEAPON?1:p->ammo_amount[p->curr_weapon] > 0))
                    {
                        if (TEST_SYNC_KEY(sb_snum, SK_FIRE))
                        {
                            if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_RANDOMRESTART)
                                *kb = 1+(krand()&3);
                            else *kb=1;
                        }
                        else *kb = 0;
                    }
                    else *kb = 0;

                    if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_RESET &&
                            ((aplWeaponWorksLike[p->curr_weapon][snum] == KNEE_WEAPON)?1:p->ammo_amount[p->curr_weapon] > 0))
                    {
                        if (TEST_SYNC_KEY(sb_snum, SK_FIRE)) *kb = 1;
                        else *kb = 0;
                    }
                }
            }
            else if (*kb >= aplWeaponFireDelay[p->curr_weapon][snum] && (*kb) < aplWeaponTotalTime[p->curr_weapon][snum]
                     && ((aplWeaponWorksLike[p->curr_weapon][snum] == KNEE_WEAPON)?1:p->ammo_amount[p->curr_weapon] > 0))
            {
                if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_AUTOMATIC)
                {
                    if (!(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_SEMIAUTO))
                    {
                        if (TEST_SYNC_KEY(sb_snum, SK_FIRE) == 0 && aplWeaponFlags[p->curr_weapon][snum] & WEAPON_RESET)
                            *kb = 0;
                        if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FIREEVERYTHIRD)
                        {
                            if (((*(kb))%3) == 0)
                            {
                                P_FireWeapon(p);
                                P_DoWeaponSpawn(p);
                            }
                        }
                        else if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FIREEVERYOTHER)
                        {
                            P_FireWeapon(p);
                            P_DoWeaponSpawn(p);
                        }
                        else
                        {
                            if (*kb == aplWeaponFireDelay[p->curr_weapon][snum])
                            {
                                P_FireWeapon(p);
                                //                                P_DoWeaponSpawn(p);
                            }
                        }
                        if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_RESET &&
                                (*kb) > aplWeaponTotalTime[p->curr_weapon][snum]-aplWeaponHoldDelay[p->curr_weapon][snum] &&
                                ((aplWeaponWorksLike[p->curr_weapon][snum] == KNEE_WEAPON) || p->ammo_amount[p->curr_weapon] > 0))
                        {
                            if (TEST_SYNC_KEY(sb_snum, SK_FIRE)) *kb = 1;
                            else *kb = 0;
                        }
                    }
                    else
                    {
                        if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FIREEVERYOTHER)
                        {
                            P_FireWeapon(p);
                            P_DoWeaponSpawn(p);
                        }
                        else
                        {
                            if (*kb == aplWeaponFireDelay[p->curr_weapon][snum])
                            {
                                P_FireWeapon(p);
                                //                                P_DoWeaponSpawn(p);
                            }
                        }
                    }
                }
                else if (*kb == aplWeaponFireDelay[p->curr_weapon][snum])
                    P_FireWeapon(p);
            }
        }
    }
}

int32_t P_DoFist(DukePlayer_t *p)
{
    // the fist punching NUKEBUTTON

    if (++p->fist_incs == 28)
    {
        if (ud.recstat == 1) G_CloseDemoWrite();
        S_PlaySound(PIPEBOMB_EXPLODE);
        p->pals.r = 64;
        p->pals.g = 64;
        p->pals.b = 64;
        p->pals.f = 48;
    }

    if (p->fist_incs > 42)
    {
        int32_t i;

        TRAVERSE_CONNECT(i)
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

void P_ProcessInput(int32_t snum)
{
    DukePlayer_t *p = g_player[snum].ps;
    spritetype *s = &sprite[p->i];

    uint32_t sb_snum = g_player[snum].sync->bits;

    int32_t j, i, k, doubvel = TICSPERFRAME, shrunk;
    int32_t fz, cz, hz, lz, truefdist, x, y, psectlotag;
    uint8_t *kb = &p->kickback_pic;
    int16_t tempsect;

    p->player_par++;

    VM_OnEvent(EVENT_PROCESSINPUT, p->i, snum, -1);

    if (p->cheat_phase > 0) sb_snum = 0;

    if (p->cursectnum == -1)
    {
        if (s->extra > 0 && ud.clipping == 0)
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

    getzsofslope(p->cursectnum,p->pos.x,p->pos.y,&p->truecz,&p->truefz);
    j = p->truefz;

    truefdist = klabs(p->pos.z-j);

    if ((lz&49152) == 16384 && psectlotag == 1 && truefdist > PHEIGHT+(16<<8))
        psectlotag = 0;

    actor[p->i].floorz = fz;
    actor[p->i].ceilingz = cz;

    p->ohoriz = p->horiz;
    p->ohorizoff = p->horizoff;

    // calculates automatic view angle for playing without a mouse
    if (p->aim_mode == 0 && p->on_ground && psectlotag != 2 && (sector[p->cursectnum].floorstat&2))
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

        if ((sprite[j].cstat&33) == 33 || (sprite[j].cstat&17) == 17)
        {
            psectlotag = 0;
            p->footprintcount = 0;
            p->spritebridge = 1;
            p->sbs = j;
        }
        else if (A_CheckEnemySprite(&sprite[j]) && sprite[j].xrepeat > 24 && klabs(s->z-sprite[j].z) < (84<<8))
        {
            // I think this is what makes the player slide off enemies... might be a good sprite flag to add later
            j = getangle(sprite[j].x-p->pos.x,sprite[j].y-p->pos.y);
            p->posvel.x -= sintable[(j+512)&2047]<<4;
            p->posvel.y -= sintable[j&2047]<<4;
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
                P_DoQuote(102,p);
            }
        }
        else if (p->timebeforeexit == 1)
        {
            TRAVERSE_CONNECT(i)
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
        p->pals.f--;

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

        if (psectlotag == 2)
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

        Bmemcpy(&p->opos.x, &p->pos.x, sizeof(vec3_t));
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
        i = p->newowner;
        p->pos.x = SX;
        p->pos.y = SY;
        p->pos.z = SZ;
        p->ang =  SA;
        p->posvel.x = p->posvel.y = s->xvel = 0;
        p->look_ang = 0;
        p->rotscrnang = 0;

        P_DoCounters(p);

        if (*aplWeaponWorksLike[p->curr_weapon] == HANDREMOTE_WEAPON)
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
        aGameVars[g_iReturnVarID].val.lValue = 0;
        VM_OnEvent(EVENT_LOOKLEFT,p->i,snum, -1);
        if (aGameVars[g_iReturnVarID].val.lValue == 0)
        {
            p->look_ang -= 152;
            p->rotscrnang += 24;
        }
    }

    if (TEST_SYNC_KEY(sb_snum, SK_LOOK_RIGHT))
    {
        // look_right
        aGameVars[g_iReturnVarID].val.lValue = 0;
        VM_OnEvent(EVENT_LOOKRIGHT,p->i,snum, -1);
        if (aGameVars[g_iReturnVarID].val.lValue == 0)
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

    s->xvel = ksqrt((p->pos.x-p->bobposx)*(p->pos.x-p->bobposx)+(p->pos.y-p->bobposy)*(p->pos.y-p->bobposy));

    if (p->on_ground)
        p->bobcounter += sprite[p->i].xvel>>1;

    if (ud.clipping == 0 && (sector[p->cursectnum].floorpicnum == MIRROR || p->cursectnum < 0 || p->cursectnum >= MAXSECTORS))
    {
        p->pos.x = p->opos.x;
        p->pos.y = p->opos.y;
    }
    else
    {
        p->opos.x = p->pos.x;
        p->opos.y = p->pos.y;
    }

    p->bobposx = p->pos.x;
    p->bobposy = p->pos.y;

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

    if (psectlotag == 2)
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
            aGameVars[g_iReturnVarID].val.lValue = 0;
            VM_OnEvent(EVENT_SWIMUP,p->i,snum, -1);
            if (aGameVars[g_iReturnVarID].val.lValue == 0)
            {
                // jump
                if (p->posvel.z > 0) p->posvel.z = 0;
                p->posvel.z -= 348;
                if (p->posvel.z < -(256*6)) p->posvel.z = -(256*6);
            }
        }
        else if (TEST_SYNC_KEY(sb_snum, SK_CROUCH))
        {
            aGameVars[g_iReturnVarID].val.lValue = 0;
            VM_OnEvent(EVENT_SWIMDOWN,p->i,snum, -1);
            if (aGameVars[g_iReturnVarID].val.lValue == 0)
            {
                // crouch
                if (p->posvel.z < 0) p->posvel.z = 0;
                p->posvel.z += 348;
                if (p->posvel.z > (256*6)) p->posvel.z = (256*6);
            }
        }
        else
        {
            // normal view
            if (p->posvel.z < 0)
            {
                p->posvel.z += 256;
                if (p->posvel.z > 0)
                    p->posvel.z = 0;
            }
            if (p->posvel.z > 0)
            {
                p->posvel.z -= 256;
                if (p->posvel.z < 0)
                    p->posvel.z = 0;
            }
        }

        if (p->posvel.z > 2048)
            p->posvel.z >>= 1;

        p->pos.z += p->posvel.z;

        if (p->pos.z > (fz-(15<<8)))
            p->pos.z += ((fz-(15<<8))-p->pos.z)>>1;

        if (p->pos.z < cz)
        {
            p->pos.z = cz;
            p->posvel.z = 0;
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
            aGameVars[g_iReturnVarID].val.lValue = 0;
            VM_OnEvent(EVENT_SOARUP,p->i,snum, -1);
            if (aGameVars[g_iReturnVarID].val.lValue == 0)
            {
                p->pos.z -= j;
                p->crack_time = 777;
            }
        }

        if (TEST_SYNC_KEY(sb_snum, SK_CROUCH))   //Z (soar low)
        {
            // crouch
            aGameVars[g_iReturnVarID].val.lValue = 0;
            VM_OnEvent(EVENT_SOARDOWN,p->i,snum, -1);
            if (aGameVars[g_iReturnVarID].val.lValue == 0)
            {
                p->pos.z += j;
                p->crack_time = 777;
            }
        }

        if (shrunk == 0 && (psectlotag == 0 || psectlotag == 2)) k = 32;
        else k = 16;

        if (psectlotag != 2 && p->scuba_on == 1)
            p->scuba_on = 0;

        if (p->pos.z > (fz-(k<<8)))
            p->pos.z += ((fz-(k<<8))-p->pos.z)>>1;
        if (p->pos.z < (actor[p->i].ceilingz+(18<<8)))
            p->pos.z = actor[p->i].ceilingz+(18<<8);

    }
    else if (psectlotag != 2)
    {
        p->airleft = 15 * GAMETICSPERSEC; // 13 seconds

        if (p->scuba_on == 1)
            p->scuba_on = 0;

        if (psectlotag == 1 && p->spritebridge == 0)
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
                    if (p->dummyplayersprite == -1)
                        p->dummyplayersprite =
                            A_Spawn(p->i,PLAYERONWATER);
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
                p->posvel.z += (g_spriteGravity+80); // (TICSPERFRAME<<6);
                if (p->posvel.z >= (4096+2048)) p->posvel.z = (4096+2048);
                if (p->posvel.z > 2400 && p->falling_counter < 255)
                {
                    p->falling_counter++;
                    if (p->falling_counter >= 38 && p->scream_voice <= FX_Ok)
                        p->scream_voice = A_PlaySound(DUKE_SCREAM,p->i);
                }

                if ((p->pos.z+p->posvel.z) >= (fz-(i<<8)) && p->cursectnum >= 0)   // hit the ground
                    if (sector[p->cursectnum].lotag != 1)
                    {
                        if (p->falling_counter > 62)
                            P_QuickKill(p);
                        else if (p->falling_counter > 9)
                        {
                            s->extra -= p->falling_counter-(krand()&3);
                            if (s->extra <= 0)
                            {
                                A_PlaySound(SQUISHED,p->i);
                                p->pals.r = 63;
                                p->pals.g = 0;
                                p->pals.b = 0;
                                p->pals.f = 63;
                            }
                            else
                            {
                                A_PlaySound(DUKE_LAND,p->i);
                                A_PlaySound(DUKE_LAND_HURT,p->i);
                            }

                            p->pals.r = 16;
                            p->pals.g = 0;
                            p->pals.b = 0;
                            p->pals.f = 32;
                        }
                        else if (p->posvel.z > 2048)
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

            if (psectlotag != 1 && psectlotag != 2 && p->on_ground == 0 && p->posvel.z > (6144>>1))
                p->hard_landing = p->posvel.z>>10;

            p->on_ground = 1;

            if (i==40)
            {
                //Smooth on the ground

                k = ((fz-(i<<8))-p->pos.z)>>1;
                if (klabs(k) < 256) k = 0;
                p->pos.z += k;
                p->posvel.z -= 768;
                if (p->posvel.z < 0) p->posvel.z = 0;
            }
            else if (p->jumping_counter == 0)
            {
                p->pos.z += ((fz-(i<<7))-p->pos.z)>>1; //Smooth on the water
                if (p->on_warping_sector == 0 && p->pos.z > fz-(16<<8))
                {
                    p->pos.z = fz-(16<<8);
                    p->posvel.z >>= 1;
                }
            }

            p->on_warping_sector = 0;

            if (TEST_SYNC_KEY(sb_snum, SK_CROUCH))
            {
                // crouching
                aGameVars[g_iReturnVarID].val.lValue = 0;
                VM_OnEvent(EVENT_CROUCH,p->i,snum, -1);
                if (aGameVars[g_iReturnVarID].val.lValue == 0)
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
                        aGameVars[g_iReturnVarID].val.lValue = 0;
                        VM_OnEvent(EVENT_JUMP,p->i,snum, -1);
                        if (aGameVars[g_iReturnVarID].val.lValue == 0)
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
                if (psectlotag == 1 && p->jumping_counter > 768)
                {
                    p->jumping_counter = 0;
                    p->posvel.z = -512;
                }
                else
                {
                    p->posvel.z -= (sintable[(2048-128+p->jumping_counter)&2047])/12;
                    p->jumping_counter += 180;
                    p->on_ground = 0;
                }
            }
            else
            {
                p->jumping_counter = 0;
                p->posvel.z = 0;
            }
        }

        p->pos.z += p->posvel.z;

        if ((psectlotag != 2 || cz != sector[p->cursectnum].ceilingz) && p->pos.z < (cz+(4<<8)))
        {
            p->jumping_counter = 0;
            if (p->posvel.z < 0)
                p->posvel.x = p->posvel.y = 0;
            p->posvel.z = 128;
            p->pos.z = cz+(4<<8);
        }
    }

    if (p->fist_incs || p->transporter_hold > 2 || p->hard_landing || p->access_incs > 0 || p->knee_incs > 0 ||
            (*aplWeaponWorksLike[p->curr_weapon] == TRIPBOMB_WEAPON &&
             *kb > 1 && *kb < *aplWeaponFireDelay[p->curr_weapon]))
    {
        doubvel = 0;
        p->posvel.x = 0;
        p->posvel.y = 0;
    }
    else if (g_player[snum].sync->avel)            //p->ang += syncangvel * constant
    {
        int32_t tempang = g_player[snum].sync->avel<<1;

        if (psectlotag == 2) p->angvel =(tempang-(tempang>>3))*ksgn(doubvel);
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
                p->inven_icon = 7;
                if (p->inv_amount[GET_BOOTS] <= 0)
                    P_SelectNextInvItem(p);
            }
            else
            {
                if (!A_CheckSoundPlaying(p->i,DUKE_LONGTERM_PAIN))
                    A_PlaySound(DUKE_LONGTERM_PAIN,p->i);
                p->pals.r = 0;
                p->pals.g = 8;
                p->pals.b = 0;
                p->pals.f = 32;
                s->extra--;
            }
        }

        if (p->on_ground && truefdist <= PHEIGHT+(16<<8) && P_CheckFloorDamage(p, j))
        {
            P_DoQuote(75, p);
            p->inv_amount[GET_BOOTS] -= 2;
            if (p->inv_amount[GET_BOOTS] <= 0)
            {
                p->inv_amount[GET_BOOTS] = 0;
                P_SelectNextInvItem(p);
            }
        }
    }

    if (g_player[snum].sync->extbits&(1))
        VM_OnEvent(EVENT_MOVEFORWARD,p->i,snum, -1);

    if (g_player[snum].sync->extbits&(1<<1))
        VM_OnEvent(EVENT_MOVEBACKWARD,p->i,snum, -1);

    if (g_player[snum].sync->extbits&(1<<2))
        VM_OnEvent(EVENT_STRAFELEFT,p->i,snum, -1);

    if (g_player[snum].sync->extbits&(1<<3))
        VM_OnEvent(EVENT_STRAFERIGHT,p->i,snum, -1);

    if (g_player[snum].sync->extbits&(1<<4) || g_player[snum].sync->avel < 0)
        VM_OnEvent(EVENT_TURNLEFT,p->i,snum, -1);

    if (g_player[snum].sync->extbits&(1<<5) || g_player[snum].sync->avel > 0)
        VM_OnEvent(EVENT_TURNRIGHT,p->i,snum, -1);

    if (p->posvel.x || p->posvel.y || g_player[snum].sync->fvel || g_player[snum].sync->svel)
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

                    switch (DynamicTileMap[j])
                    {
                    case PANNEL1__STATIC:
                    case PANNEL2__STATIC:
                        A_PlaySound(DUKE_WALKINDUCTS,p->i);
                        p->walking_snd_toggle = 1;
                        break;
                    }
                    break;

                case 1:
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

        p->posvel.x += ((g_player[snum].sync->fvel*doubvel)<<6);
        p->posvel.y += ((g_player[snum].sync->svel*doubvel)<<6);

        j = 0;

        if (psectlotag == 2)
            j = 0x1400;
        else if (p->on_ground && (TEST_SYNC_KEY(sb_snum, SK_CROUCH) || (*kb > 10 && aplWeaponWorksLike[p->curr_weapon][snum] == KNEE_WEAPON)))
            j = 0x2000;

        p->posvel.x = mulscale16(p->posvel.x,p->runspeed-j);
        p->posvel.y = mulscale16(p->posvel.y,p->runspeed-j);

        if (klabs(p->posvel.x) < 2048 && klabs(p->posvel.y) < 2048)
            p->posvel.x = p->posvel.y = 0;

        if (shrunk)
        {
            p->posvel.x = mulscale16(p->posvel.x,p->runspeed-(p->runspeed>>1)+(p->runspeed>>2));
            p->posvel.y = mulscale16(p->posvel.y,p->runspeed-(p->runspeed>>1)+(p->runspeed>>2));
        }
    }

HORIZONLY:
    if (psectlotag == 1 || p->spritebridge == 1) i = (4L<<8);
    else i = (20L<<8);

    if (p->cursectnum >= 0 && sector[p->cursectnum].lotag == 2) k = 0;
    else k = 1;

    if (ud.clipping)
    {
        p->pos.x += p->posvel.x>>14;
        p->pos.y += p->posvel.y>>14;
        updatesector(p->pos.x,p->pos.y,&p->cursectnum);
        changespritesect(p->i,p->cursectnum);
    }
    else if ((j = clipmove((vec3_t *)p,&p->cursectnum, p->posvel.x,p->posvel.y,164L,(4L<<8),i,CLIPMASK0)))
        P_CheckTouchDamage(p, j);

    if (p->jetpack_on == 0 && psectlotag != 2 && psectlotag != 1 && shrunk)
        p->pos.z += 32<<8;

    if (p->jetpack_on == 0)
    {
        if (s->xvel > 16)
        {
            if (psectlotag != 1 && psectlotag != 2 && p->on_ground)
            {
                p->pycount += 52;
                p->pycount &= 2047;
                p->pyoff =
                    klabs(s->xvel*sintable[p->pycount])/1596;
            }
        }
        else if (psectlotag != 2 && psectlotag != 1)
            p->pyoff = 0;
    }

    // RBG***

    p->pos.z += PHEIGHT;
    setsprite(p->i,(vec3_t *)&p->pos.x);
    p->pos.z -= PHEIGHT;

    if (psectlotag < 3)
    {
//        p->cursectnum = s->sectnum;

        if (!ud.clipping && sector[p->cursectnum].lotag == 31)
        {
            if (sprite[sector[p->cursectnum].hitag].xvel && actor[sector[p->cursectnum].hitag].t_data[0] == 0)
            {
                P_QuickKill(p);
                return;
            }
        }
    }

    if (p->cursectnum >= 0 && truefdist < PHEIGHT && p->on_ground && psectlotag != 1 && shrunk == 0 && sector[p->cursectnum].lotag == 1)
        if (!A_CheckSoundPlaying(p->i,DUKE_ONWATER))
            A_PlaySound(DUKE_ONWATER,p->i);

    if (p->cursectnum >=0 && p->cursectnum != s->sectnum)
        changespritesect(p->i, p->cursectnum);

    if (p->cursectnum >= 0 && ud.clipping == 0)
    {
        j = (pushmove((vec3_t *)p,&p->cursectnum,164L,(4L<<8),(4L<<8),CLIPMASK0) < 0 && A_GetFurthestAngle(p->i,8) < 512);

        if (klabs(actor[p->i].floorz-actor[p->i].ceilingz) < (48<<8) || j)
        {
            if (!(sector[s->sectnum].lotag&0x8000) && (isanunderoperator(sector[s->sectnum].lotag) ||
                    isanearoperator(sector[s->sectnum].lotag)))
                activatebysector(s->sectnum,p->i);
            if (j)
            {
                P_QuickKill(p);
                return;
            }
        }
        else if (klabs(fz-cz) < (32<<8) && isanunderoperator(sector[p->cursectnum].lotag))
            activatebysector(p->cursectnum,p->i);
    }

    i = 0;
    if (TEST_SYNC_KEY(sb_snum, SK_CENTER_VIEW) || p->hard_landing)
    {
        aGameVars[g_iReturnVarID].val.lValue = 0;
        VM_OnEvent(EVENT_RETURNTOCENTER,p->i,snum, -1);
        if (aGameVars[g_iReturnVarID].val.lValue == 0)
            p->return_to_center = 9;
    }

    if (TEST_SYNC_KEY(sb_snum, SK_LOOK_UP))
    {
        aGameVars[g_iReturnVarID].val.lValue = 0;
        VM_OnEvent(EVENT_LOOKUP,p->i,snum, -1);
        if (aGameVars[g_iReturnVarID].val.lValue == 0)
        {
            p->return_to_center = 9;
            if (TEST_SYNC_KEY(sb_snum, SK_RUN)) p->horiz += 12;
            p->horiz += 12;
            i++;
        }
    }

    if (TEST_SYNC_KEY(sb_snum, SK_LOOK_DOWN))
    {
        aGameVars[g_iReturnVarID].val.lValue = 0;
        VM_OnEvent(EVENT_LOOKDOWN,p->i,snum, -1);
        if (aGameVars[g_iReturnVarID].val.lValue == 0)
        {
            p->return_to_center = 9;
            if (TEST_SYNC_KEY(sb_snum, SK_RUN)) p->horiz -= 12;
            p->horiz -= 12;
            i++;
        }
    }

    if (TEST_SYNC_KEY(sb_snum, SK_AIM_UP))
    {
        aGameVars[g_iReturnVarID].val.lValue = 0;
        VM_OnEvent(EVENT_AIMUP,p->i,snum, -1);
        if (aGameVars[g_iReturnVarID].val.lValue == 0)
        {
            if (TEST_SYNC_KEY(sb_snum, SK_RUN)) p->horiz += 6;
            p->horiz += 6;
            i++;
        }
    }

    if (TEST_SYNC_KEY(sb_snum, SK_AIM_DOWN))
    {
        aGameVars[g_iReturnVarID].val.lValue = 0;
        VM_OnEvent(EVENT_AIMDOWN,p->i,snum, -1);
        if (aGameVars[g_iReturnVarID].val.lValue == 0)
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
            P_AddWeapon(p, p->last_full_weapon);
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

                switch (DynamicTileMap[sprite[p->actorsqu].picnum])
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
                    deletesprite(p->actorsqu);
                    break;
                case APLAYER__STATIC:
                    P_QuickKill(g_player[sprite[p->actorsqu].yvel].ps);
                    g_player[sprite[p->actorsqu].yvel].ps->frag_ps = snum;
                    break;
                default:
                    if (A_CheckEnemySprite(&sprite[p->actorsqu]))
                        p->actors_killed++;
                    deletesprite(p->actorsqu);
                    break;
                }
            }
            p->actorsqu = -1;
        }
        else if (p->actorsqu >= 0)
            p->ang += G_GetAngleDelta(p->ang,getangle(sprite[p->actorsqu].x-p->pos.x,sprite[p->actorsqu].y-p->pos.y))>>2;
    }

    if (P_DoCounters(p)) return;

    P_ProcessWeapon(snum);
}

//UPDATE THIS FILE OVER THE OLD GETSPRITESCORE/COMPUTERGETINPUT FUNCTIONS
int32_t getspritescore(int32_t snum, int32_t dapicnum)
{
    switch (DynamicTileMap[dapicnum])
    {
    case FIRSTGUNSPRITE__STATIC:
        return(20);
    case CHAINGUNSPRITE__STATIC:
        return(50);
    case RPGSPRITE__STATIC:
        return(200);
    case FREEZESPRITE__STATIC:
        return(25);
    case SHRINKERSPRITE__STATIC:
        return(80);
    case HEAVYHBOMB__STATIC:
        return(60);
    case TRIPBOMBSPRITE__STATIC:
        return(50);
    case SHOTGUNSPRITE__STATIC:
        return(120);
    case DEVISTATORSPRITE__STATIC:
        return(120);

    case FREEZEAMMO__STATIC:
        if (g_player[snum].ps->ammo_amount[FREEZE_WEAPON] < g_player[snum].ps->max_ammo_amount[FREEZE_WEAPON]) return(10);
        return(1);
    case AMMO__STATIC:
        if (g_player[snum].ps->ammo_amount[PISTOL_WEAPON] < g_player[snum].ps->max_ammo_amount[PISTOL_WEAPON]) return(10);
        return(1);
    case BATTERYAMMO__STATIC:
        if (g_player[snum].ps->ammo_amount[CHAINGUN_WEAPON] < g_player[snum].ps->max_ammo_amount[CHAINGUN_WEAPON]) return(20);
        return(1);
    case DEVISTATORAMMO__STATIC:
        if (g_player[snum].ps->ammo_amount[DEVISTATOR_WEAPON] < g_player[snum].ps->max_ammo_amount[DEVISTATOR_WEAPON]) return(25);
        return(1);
    case RPGAMMO__STATIC:
        if (g_player[snum].ps->ammo_amount[RPG_WEAPON] < g_player[snum].ps->max_ammo_amount[RPG_WEAPON]) return(50);
        return(1);
    case CRYSTALAMMO__STATIC:
        if (g_player[snum].ps->ammo_amount[SHRINKER_WEAPON] < g_player[snum].ps->max_ammo_amount[SHRINKER_WEAPON]) return(10);
        return(1);
    case HBOMBAMMO__STATIC:
        if (g_player[snum].ps->ammo_amount[HANDBOMB_WEAPON] < g_player[snum].ps->max_ammo_amount[HANDBOMB_WEAPON]) return(30);
        return(1);
    case SHOTGUNAMMO__STATIC:
        if (g_player[snum].ps->ammo_amount[SHOTGUN_WEAPON] < g_player[snum].ps->max_ammo_amount[SHOTGUN_WEAPON]) return(25);
        return(1);

    case COLA__STATIC:
        if (sprite[g_player[snum].ps->i].extra < g_player[snum].ps->max_player_health) return(10);
        return(1);
    case SIXPAK__STATIC:
        if (sprite[g_player[snum].ps->i].extra < g_player[snum].ps->max_player_health) return(30);
        return(1);
    case FIRSTAID__STATIC:
        if (g_player[snum].ps->inv_amount[GET_FIRSTAID] < g_player[snum].ps->max_player_health) return(100);
        return(1);
    case SHIELD__STATIC:
        if (g_player[snum].ps->inv_amount[GET_SHIELD] < g_player[snum].ps->max_shield_amount) return(50);
        return(1);
    case STEROIDS__STATIC:
        if (g_player[snum].ps->inv_amount[GET_STEROIDS] < 400) return(30);
        return(1);
    case AIRTANK__STATIC:
        if (g_player[snum].ps->inv_amount[GET_SCUBA] < 6400) return(30);
        return(1);
    case JETPACK__STATIC:
        if (g_player[snum].ps->inv_amount[GET_JETPACK] < 1600) return(100);
        return(1);
    case HEATSENSOR__STATIC:
        if (g_player[snum].ps->inv_amount[GET_HEATS] < 1200) return(5);
        return(1);
    case ACCESSCARD__STATIC:
        return(1);
    case BOOTS__STATIC:
        if (g_player[snum].ps->inv_amount[GET_BOOTS] < 200) return(15);
        return(1);
    case ATOMICHEALTH__STATIC:
        if (sprite[g_player[snum].ps->i].extra < g_player[snum].ps->max_player_health<<1) return(50);
        return(1);
    case HOLODUKE__STATIC:
        if (g_player[snum].ps->inv_amount[GET_HOLODUKE] < 2400) return(5);
        return(1);
    case MUSICANDSFX__STATIC:
        return(1);
    }
    return(0);
}

static int32_t fdmatrix[12][12] =
{
    //KNEE PIST SHOT CHAIN RPG PIPE SHRI DEVI WALL FREE HAND EXPA
    {  128,  -1,  -1,  -1, 128,  -1,  -1,  -1, 128,  -1, 128,  -1 },   //KNEE
    { 1024,1024,1024,1024,2560, 128,2560,2560,1024,2560,2560,2560 },   //PIST
    {  512, 512, 512, 512,2560, 128,2560,2560,1024,2560,2560,2560 },   //SHOT
    {  512, 512, 512, 512,2560, 128,2560,2560,1024,2560,2560,2560 },   //CHAIN
    { 2560,2560,2560,2560,2560,2560,2560,2560,2560,2560,2560,2560 },   //RPG
    {  512, 512, 512, 512,2048, 512,2560,2560, 512,2560,2560,2560 },   //PIPE
    {  128, 128, 128, 128,2560, 128,2560,2560, 128, 128, 128, 128 },   //SHRI
    { 1536,1536,1536,1536,2560,1536,1536,1536,1536,1536,1536,1536 },   //DEVI
    {   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },   //WALL
    {  128, 128, 128, 128,2560, 128,2560,2560, 128, 128, 128, 128 },   //FREE
    { 2560,2560,2560,2560,2560,2560,2560,2560,2560,2560,2560,2560 },   //HAND
    {  128, 128, 128, 128,2560, 128,2560,2560, 128, 128, 128, 128 }    //EXPA
};

static int32_t goalx[MAXPLAYERS], goaly[MAXPLAYERS], goalz[MAXPLAYERS];
static int32_t goalsect[MAXPLAYERS], goalwall[MAXPLAYERS], goalsprite[MAXPLAYERS], goalspritescore[MAXPLAYERS];
static int32_t goalplayer[MAXPLAYERS], clipmovecount[MAXPLAYERS];
int16_t searchsect[MAXSECTORS], searchparent[MAXSECTORS];
char dashow2dsector[(MAXSECTORS+7)>>3];
void computergetinput(int32_t snum, input_t *syn)
{
    int32_t i, j, k, l, x1, y1, z1, x2, y2, z2, x3, y3, z3, dx, dy;
    int32_t dist, daang, zang, fightdist, damyang, damysect;
    int32_t startsect, endsect, splc, send, startwall, endwall;
    hitdata_t hitinfo;
    DukePlayer_t *p = g_player[snum].ps;
    walltype *wal;

    syn->fvel = 0;
    syn->svel = 0;
    syn->avel = 0;
    syn->horz = 0;
    syn->bits = 0;
    syn->extbits = 0;

    x1 = sprite[p->i].x;
    y1 = sprite[p->i].y;
    z1 = sprite[p->i].z;
    damyang = sprite[p->i].ang;
    damysect = sprite[p->i].sectnum;
    if ((numplayers >= 2) && (snum == myconnectindex))
    {
        x1 = my.x;
        y1 = my.y;
        z1 = my.z+PHEIGHT;
        damyang = myang;
        damysect = mycursectnum;
    }

    if (!(numframes&7))
    {
        x2 = sprite[g_player[goalplayer[snum]].ps->i].x;
        y2 = sprite[g_player[goalplayer[snum]].ps->i].y;
        z2 = sprite[g_player[goalplayer[snum]].ps->i].z;

        if (!cansee(x1,y1,z1-(48<<8),damysect,x2,y2,z2-(48<<8),sprite[g_player[goalplayer[snum]].ps->i].sectnum))
            goalplayer[snum] = snum;
    }

    if ((goalplayer[snum] == snum) || (g_player[goalplayer[snum]].ps->dead_flag != 0))
    {
        j = 0x7fffffff;
        TRAVERSE_CONNECT(i)
        if (i != snum && !(GTFLAGS(GAMETYPE_TDM) && g_player[snum].ps->team == g_player[i].ps->team))
        {
            dist = ksqrt((sprite[g_player[i].ps->i].x-x1)*(sprite[g_player[i].ps->i].x-x1)+(sprite[g_player[i].ps->i].y-y1)*(sprite[g_player[i].ps->i].y-y1));

            x2 = sprite[g_player[i].ps->i].x;
            y2 = sprite[g_player[i].ps->i].y;
            z2 = sprite[g_player[i].ps->i].z;
            if (!cansee(x1,y1,z1-(48<<8),damysect,x2,y2,z2-(48<<8),sprite[g_player[i].ps->i].sectnum))
                dist <<= 1;

            if (dist < j)
            {
                j = dist;
                goalplayer[snum] = i;
            }

        }
    }

    x2 = sprite[g_player[goalplayer[snum]].ps->i].x;
    y2 = sprite[g_player[goalplayer[snum]].ps->i].y;
    z2 = sprite[g_player[goalplayer[snum]].ps->i].z;

    if (p->dead_flag) syn->bits |= BIT(SK_OPEN);
    if ((p->inv_amount[GET_FIRSTAID] > 0) && (p->last_extra < 100))
        syn->bits |= BIT(SK_MEDKIT);

    for (j=headspritestat[STAT_PROJECTILE]; j>=0; j=nextspritestat[j])
    {
        switch (DynamicTileMap[sprite[j].picnum])
        {
        case TONGUE__STATIC:
            k = 4;
            break;
        case FREEZEBLAST__STATIC:
            k = 4;
            break;
        case SHRINKSPARK__STATIC:
            k = 16;
            break;
        case RPG__STATIC:
            k = 16;
            break;
        default:
            k = 0;
            break;
        }
        if (k)
        {
            x3 = sprite[j].x;
            y3 = sprite[j].y;
            z3 = sprite[j].z;
            for (l=0; l<=8; l++)
            {
                if (tmulscale11(x3-x1,x3-x1,y3-y1,y3-y1,(z3-z1)>>4,(z3-z1)>>4) < 3072)
                {
                    dx = sintable[(sprite[j].ang+512)&2047];
                    dy = sintable[sprite[j].ang&2047];
                    if ((x1-x3)*dy > (y1-y3)*dx) i = -k*512;
                    else i = k*512;
                    syn->fvel -= mulscale17(dy,i);
                    syn->svel += mulscale17(dx,i);
                }
                if (l < 7)
                {
                    x3 += (mulscale14(sprite[j].xvel,sintable[(sprite[j].ang+512)&2047])<<2);
                    y3 += (mulscale14(sprite[j].xvel,sintable[sprite[j].ang&2047])<<2);
                    z3 += (sprite[j].zvel<<2);
                }
                else
                {
                    hitscan((const vec3_t *)&sprite[j],sprite[j].sectnum,
                            mulscale14(sprite[j].xvel,sintable[(sprite[j].ang+512)&2047]),
                            mulscale14(sprite[j].xvel,sintable[sprite[j].ang&2047]),
                            (int32_t)sprite[j].zvel,
                            &hitinfo,CLIPMASK1);
                    x3 = hitinfo.pos.x;
                    y3 = hitinfo.pos.y;
                    z3 = hitinfo.pos.z;
                }
            }
        }
    }

    if ((g_player[goalplayer[snum]].ps->dead_flag == 0) &&
            ((cansee(x1,y1,z1,damysect,x2,y2,z2,sprite[g_player[goalplayer[snum]].ps->i].sectnum)) ||
             (cansee(x1,y1,z1-(24<<8),damysect,x2,y2,z2-(24<<8),sprite[g_player[goalplayer[snum]].ps->i].sectnum)) ||
             (cansee(x1,y1,z1-(48<<8),damysect,x2,y2,z2-(48<<8),sprite[g_player[goalplayer[snum]].ps->i].sectnum))))
    {
        syn->bits |= BIT(SK_FIRE);

        if ((p->curr_weapon == HANDBOMB_WEAPON) && (!(rand()&7)))
            syn->bits &= ~BIT(SK_FIRE);

        if (p->curr_weapon == TRIPBOMB_WEAPON)
            syn->bits |= ((rand()%MAX_WEAPONS)<<SK_WEAPON_BITS);

        if (p->curr_weapon == RPG_WEAPON)
        {
            vec3_t vect;
            vect.x = x1;
            vect.y = y1;
            vect.z = z1-PHEIGHT;
            hitscan((const vec3_t *)&vect,damysect,sintable[(damyang+512)&2047],sintable[damyang&2047],
                    (100-p->horiz-p->horizoff)*32,&hitinfo,CLIPMASK1);
            x3 = hitinfo.pos.x;
            y3 = hitinfo.pos.y;
            z3 = hitinfo.pos.z;
            if ((x3-x1)*(x3-x1)+(y3-y1)*(y3-y1) < 2560*2560) syn->bits &= ~BIT(SK_FIRE);
        }


        fightdist = fdmatrix[p->curr_weapon][g_player[goalplayer[snum]].ps->curr_weapon];
        if (fightdist < 128) fightdist = 128;
        dist = ksqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
        if (dist == 0) dist = 1;
        daang = getangle(x2+(g_player[goalplayer[snum]].ps->posvel.x>>14)-x1,y2+(g_player[goalplayer[snum]].ps->posvel.y>>14)-y1);
        zang = 100-((z2-z1)*8)/dist;
        fightdist = max(fightdist,(klabs(z2-z1)>>4));

        if (sprite[g_player[goalplayer[snum]].ps->i].yrepeat < 32)
        {
            fightdist = 0;
            syn->bits &= ~BIT(SK_FIRE);
        }
        if (sprite[g_player[goalplayer[snum]].ps->i].pal == 1)
        {
            fightdist = 0;
            syn->bits &= ~BIT(SK_FIRE);
        }

        if (dist < 256) syn->bits |= BIT(SK_QUICK_KICK);

        x3 = x2+((x1-x2)*fightdist/dist);
        y3 = y2+((y1-y2)*fightdist/dist);
        syn->fvel += (x3-x1)*2047/dist;
        syn->svel += (y3-y1)*2047/dist;

        //Strafe attack
        if (fightdist)
        {
            j = totalclock+snum*13468;
            i = sintable[(j<<6)&2047];
            i += sintable[((j+4245)<<5)&2047];
            i += sintable[((j+6745)<<4)&2047];
            i += sintable[((j+15685)<<3)&2047];
            dx = sintable[(sprite[g_player[goalplayer[snum]].ps->i].ang+512)&2047];
            dy = sintable[sprite[g_player[goalplayer[snum]].ps->i].ang&2047];
            if ((x1-x2)*dy > (y1-y2)*dx) i += 8192;
            else i -= 8192;
            syn->fvel += ((sintable[(daang+1024)&2047]*i)>>17);
            syn->svel += ((sintable[(daang+512)&2047]*i)>>17);
        }

        syn->avel = min(max((((daang+1024-damyang)&2047)-1024)>>1,-127),127);
        syn->horz = min(max((zang-p->horiz),-MAXHORIZ),MAXHORIZ);
        syn->bits |= BIT(SK_AIMMODE);
        return;
    }

    goalsect[snum] = -1;
    if (goalsect[snum] < 0)
    {
        goalwall[snum] = -1;
        startsect = sprite[p->i].sectnum;
        endsect = sprite[g_player[goalplayer[snum]].ps->i].sectnum;

        clearbufbyte(dashow2dsector,(MAXSECTORS+7)>>3,0L);
        searchsect[0] = startsect;
        searchparent[0] = -1;
        dashow2dsector[startsect>>3] |= (1<<(startsect&7));
        for (splc=0,send=1; splc<send; splc++)
        {
            startwall = sector[searchsect[splc]].wallptr;
            endwall = startwall + sector[searchsect[splc]].wallnum;
            for (i=startwall,wal=&wall[startwall]; i<endwall; i++,wal++)
            {
                j = wal->nextsector;
                if (j < 0) continue;

                dx = ((wall[wal->point2].x+wal->x)>>1);
                dy = ((wall[wal->point2].y+wal->y)>>1);
                if ((getceilzofslope(j,dx,dy) > getflorzofslope(j,dx,dy)-(28<<8)) && ((sector[j].lotag < 15) || (sector[j].lotag > 22)))
                    continue;
                if (getflorzofslope(j,dx,dy) < getflorzofslope(searchsect[splc],dx,dy)-(72<<8))
                    continue;
                if ((dashow2dsector[j>>3]&(1<<(j&7))) == 0)
                {
                    dashow2dsector[j>>3] |= (1<<(j&7));
                    searchsect[send] = (int16_t)j;
                    searchparent[send] = (int16_t)splc;
                    send++;
                    if (j == endsect)
                    {
                        clearbufbyte(dashow2dsector,(MAXSECTORS+7)>>3,0L);
                        for (k=send-1; k>=0; k=searchparent[k])
                            dashow2dsector[searchsect[k]>>3] |= (1<<(searchsect[k]&7));

                        for (k=send-1; k>=0; k=searchparent[k])
                            if (!searchparent[k]) break;

                        goalsect[snum] = searchsect[k];
                        startwall = sector[goalsect[snum]].wallptr;
                        endwall = startwall+sector[goalsect[snum]].wallnum;
                        x3 = y3 = 0;
                        for (i=startwall; i<endwall; i++)
                        {
                            x3 += wall[i].x;
                            y3 += wall[i].y;
                        }
                        x3 /= (endwall-startwall);
                        y3 /= (endwall-startwall);

                        startwall = sector[startsect].wallptr;
                        endwall = startwall+sector[startsect].wallnum;
                        l = 0;
                        k = startwall;
                        for (i=startwall; i<endwall; i++)
                        {
                            if (wall[i].nextsector != goalsect[snum]) continue;
                            dx = wall[wall[i].point2].x-wall[i].x;
                            dy = wall[wall[i].point2].y-wall[i].y;

                            //if (dx*(y1-wall[i].y) <= dy*(x1-wall[i].x))
                            //   if (dx*(y2-wall[i].y) >= dy*(x2-wall[i].x))
                            if ((x3-x1)*(wall[i].y-y1) <= (y3-y1)*(wall[i].x-x1))
                                if ((x3-x1)*(wall[wall[i].point2].y-y1) >= (y3-y1)*(wall[wall[i].point2].x-x1))
                                {
                                    k = i;
                                    break;
                                }

                            dist = ksqrt(dx*dx+dy*dy);
                            if (dist > l)
                            {
                                l = dist;
                                k = i;
                            }

                        }
                        goalwall[snum] = k;
                        daang = ((getangle(wall[wall[k].point2].x-wall[k].x,wall[wall[k].point2].y-wall[k].y)+1536)&2047);
                        goalx[snum] = ((wall[k].x+wall[wall[k].point2].x)>>1)+(sintable[(daang+512)&2047]>>8);
                        goaly[snum] = ((wall[k].y+wall[wall[k].point2].y)>>1)+(sintable[daang&2047]>>8);
                        goalz[snum] = sector[goalsect[snum]].floorz-(32<<8);
                        break;
                    }
                }
            }

            for (i=headspritesect[searchsect[splc]]; i>=0; i=nextspritesect[i])
                if (sprite[i].lotag == 7)
                {
                    j = sprite[sprite[i].owner].sectnum;
                    if ((dashow2dsector[j>>3]&(1<<(j&7))) == 0)
                    {
                        dashow2dsector[j>>3] |= (1<<(j&7));
                        searchsect[send] = (int16_t)j;
                        searchparent[send] = (int16_t)splc;
                        send++;
                        if (j == endsect)
                        {
                            clearbufbyte(dashow2dsector,(MAXSECTORS+7)>>3,0L);
                            for (k=send-1; k>=0; k=searchparent[k])
                                dashow2dsector[searchsect[k]>>3] |= (1<<(searchsect[k]&7));

                            for (k=send-1; k>=0; k=searchparent[k])
                                if (!searchparent[k]) break;

                            goalsect[snum] = searchsect[k];
                            startwall = sector[startsect].wallptr;
                            endwall = startwall+sector[startsect].wallnum;
                            l = 0;
                            k = startwall;
                            for (i=startwall; i<endwall; i++)
                            {
                                dx = wall[wall[i].point2].x-wall[i].x;
                                dy = wall[wall[i].point2].y-wall[i].y;
                                dist = ksqrt(dx*dx+dy*dy);
                                if ((wall[i].nextsector == goalsect[snum]) && (dist > l))
                                {
                                    l = dist;
                                    k = i;
                                }

                            }
                            goalwall[snum] = k;
                            daang = ((getangle(wall[wall[k].point2].x-wall[k].x,wall[wall[k].point2].y-wall[k].y)+1536)&2047);
                            goalx[snum] = ((wall[k].x+wall[wall[k].point2].x)>>1)+(sintable[(daang+512)&2047]>>8);
                            goaly[snum] = ((wall[k].y+wall[wall[k].point2].y)>>1)+(sintable[daang&2047]>>8);
                            goalz[snum] = sector[goalsect[snum]].floorz-(32<<8);
                            break;
                        }
                    }
                }
            if (goalwall[snum] >= 0) break;
        }
    }

    if ((goalsect[snum] < 0) || (goalwall[snum] < 0))
    {
        if (goalsprite[snum] < 0 || !cansee(x1,y1,z1-(32<<8),damysect,sprite[goalsprite[snum]].x,sprite[goalsprite[snum]].y,sprite[goalsprite[snum]].z-(4<<8),i))
        {
            int32_t bestsprite = -1, spritescore = 0;

            for (k=0; k<16; k++)
            {
                i = (rand()%numsectors);
                for (j=headspritesect[i]; j>=0; j=nextspritesect[j])
                {
                    if ((sprite[j].xrepeat <= 0) || (sprite[j].yrepeat <= 0)) continue;
                    if (getspritescore(snum,sprite[j].picnum) <= 0) continue;
                    if (cansee(x1,y1,z1-(32<<8),damysect,sprite[j].x,sprite[j].y,sprite[j].z-(4<<8),i))
                    {
                        if (getspritescore(snum,sprite[j].picnum) > spritescore)
                        {
                            spritescore = getspritescore(snum,sprite[j].picnum);
                            bestsprite = j;
                        }
//                        break;
                    }
                }
            }
            if (bestsprite != -1 && (goalsprite[snum] < 0 || spritescore > goalspritescore[snum]))
            {
                goalx[snum] = sprite[bestsprite].x;
                goaly[snum] = sprite[bestsprite].y;
                goalz[snum] = sprite[bestsprite].z;
                goalsprite[snum] = bestsprite;
                goalspritescore[snum] = spritescore;
            }
        }
        x2 = goalx[snum];
        y2 = goaly[snum];
        dist = ksqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
        if (!dist) return;
        daang = getangle(x2-x1,y2-y1);
        syn->fvel += (x2-x1)*2047/dist;
        syn->svel += (y2-y1)*2047/dist;
        syn->avel = min(max((((daang+1024-damyang)&2047)-1024)>>3,-127),127);
    }
    else if (goalsprite[snum] != -1)
    {
        if (!cansee(x1,y1,z1-(32<<8),damysect,sprite[goalsprite[snum]].x,sprite[goalsprite[snum]].y,sprite[goalsprite[snum]].z-(4<<8),i))
        {
            goalspritescore[snum] = 0;
            goalsprite[snum] = -1;
        }
    }

    {
        vec3_t vect;
        int16_t dasect = p->cursectnum;
        Bmemcpy(&vect,p,sizeof(vec3_t));

        i = clipmove(&vect,&dasect,p->posvel.x,p->posvel.y,164L,4L<<8,4L<<8,CLIPMASK0);
        if (!i)
        {
            Bmemcpy(&vect,p,sizeof(vec3_t));
            vect.z += (24<<8);
            dasect = p->cursectnum;
            i = clipmove(&vect,&dasect,p->posvel.x,p->posvel.y,164L,4L<<8,4L<<8,CLIPMASK0);
        }
    }
    if (i)
    {
        clipmovecount[snum]++;

        j = 0;
        if ((i&0xc000) == 32768)  //Hit a wall (49152 for sprite)
            if (wall[i&(MAXWALLS-1)].nextsector >= 0)
            {
                if (getflorzofslope(wall[i&(MAXWALLS-1)].nextsector,p->pos.x,p->pos.y) <= p->pos.z+(24<<8)) j |= 1;
                if (getceilzofslope(wall[i&(MAXWALLS-1)].nextsector,p->pos.x,p->pos.y) >= p->pos.z-(24<<8)) j |= 2;
            }
        if ((i&0xc000) == 49152) j = 1;
        if (j&1) if (clipmovecount[snum] == 4) syn->bits |= BIT(SK_JUMP);
        if (j&2) syn->bits |= BIT(SK_CROUCH);

        //Strafe attack
        daang = getangle(x2-x1,y2-y1);
        if ((i&0xc000) == 32768)
            daang = getangle(wall[wall[i&(MAXWALLS-1)].point2].x-wall[i&(MAXWALLS-1)].x,wall[wall[i&(MAXWALLS-1)].point2].y-wall[i&(MAXWALLS-1)].y);
        j = totalclock+snum*13468;
        i = sintable[(j<<6)&2047];
        i += sintable[((j+4245)<<5)&2047];
        i += sintable[((j+6745)<<4)&2047];
        i += sintable[((j+15685)<<3)&2047];
        syn->fvel += ((sintable[(daang+1024)&2047]*i)>>17);
        syn->svel += ((sintable[(daang+512)&2047]*i)>>17);

        if ((clipmovecount[snum]&31) == 2) syn->bits |= BIT(SK_OPEN);
        if ((clipmovecount[snum]&31) == 17) syn->bits |= BIT(SK_QUICK_KICK);
        if (clipmovecount[snum] > 32)
        {
            goalsect[snum] = -1;
            goalwall[snum] = -1;
            clipmovecount[snum] = 0;
        }

        if (goalsprite[snum] != -1)
        {
            if (!cansee(x1,y1,z1-(32<<8),damysect,sprite[goalsprite[snum]].x,sprite[goalsprite[snum]].y,sprite[goalsprite[snum]].z-(4<<8),i))
            {
                goalsprite[snum] = -1;
                goalspritescore[snum] = 0;
            }
        }
    }
    else
        clipmovecount[snum] = 0;

    if ((goalsect[snum] >= 0) && (goalwall[snum] >= 0))
    {
        x2 = goalx[snum];
        y2 = goaly[snum];
        dist = ksqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
        if (!dist) return;
        daang = getangle(x2-x1,y2-y1);
        if ((goalwall[snum] >= 0) && (dist < 4096))
            daang = ((getangle(wall[wall[goalwall[snum]].point2].x-wall[goalwall[snum]].x,wall[wall[goalwall[snum]].point2].y-wall[goalwall[snum]].y)+1536)&2047);
        syn->fvel += (x2-x1)*2047/dist;
        syn->svel += (y2-y1)*2047/dist;
        syn->avel = min(max((((daang+1024-damyang)&2047)-1024)>>3,-127),127);
    }
}

