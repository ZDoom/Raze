//-------------------------------------------------------------------------
/*
Copyright (C) 2005 - EDuke32 team

This file is part of EDuke32

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

// Savage Baggage Masters

#include "duke3d.h"
#include "osd.h"

long g_currentweapon;
long g_gun_pos;
long g_looking_arc;
long g_weapon_offset;
long g_gs;
long g_kb;
long g_looking_angSR1;
long g_weapon_xoffset;

int32 turnheldtime; //MED
int32 lastcontroltime; //MED

void setpal(struct player_struct *p)
{
    if(p->heat_on) p->palette = slimepal;
    else if ((sector[p->cursectnum].ceilingpicnum >= FLOORSLIME)&&(sector[p->cursectnum].ceilingpicnum <=FLOORSLIME+2)) {
        p->palette = slimepal;
    } else {
        if(sector[p->cursectnum].lotag == 2) p->palette = waterpal;
        else p->palette = palette;

    }
    restorepalette = 1;
}

void fadepal(int r, int g, int b, int start, int end, int step)
{
    if (getrendermode() >= 3) return;
    if (step > 0) for (; start < end; start += step) palto(r,g,b,start);
    else for (; start >= end; start += step) palto(r,g,b,start);
}

void incur_damage(struct player_struct *p)
{
    long damage = 0L, shield_damage = 0L;

    SetGameVarID(g_iReturnVarID,0,p->i,sprite[p->i].yvel);
    OnEvent(EVENT_INCURDAMAGE, p->i, sprite[p->i].yvel, -1);

    if(GetGameVarID(g_iReturnVarID,p->i,sprite[p->i].yvel) == 0)

    {
        sprite[p->i].extra -= p->extra_extra8>>8;

        damage = sprite[p->i].extra - p->last_extra;

        if ( damage < 0 )
        {
            p->extra_extra8 = 0;

            if ( p->shield_amount > 0 )
            {
                shield_damage =  damage * (20 + (TRAND%30)) / 100;
                damage -= shield_damage;

                p->shield_amount += shield_damage;

                if ( p->shield_amount < 0 )
                {
                    damage += p->shield_amount;
                    p->shield_amount = 0;
                }
            }

            sprite[p->i].extra = p->last_extra + damage;
        }

    }

}

void quickkill(struct player_struct *p)
{
    p->pals[0] = 48;
    p->pals[1] = 48;
    p->pals[2] = 48;
    p->pals_time = 48;

    sprite[p->i].extra = 0;
    sprite[p->i].cstat |= 32768;
    if(ud.god == 0) guts(&sprite[p->i],JIBS6,8,myconnectindex);
    return;
}

void forceplayerangle(struct player_struct *p)
{
    short n;

    n = 128-(TRAND&255);

    p->horiz += 64;
    p->return_to_center = 9;
    p->look_ang = n>>1;
    p->rotscrnang = n>>1;
}

void tracers(long x1,long y1,long z1,long x2,long y2,long z2,long n)
{
    long i, xv, yv, zv;
    short sect = -1;

    i = n+1;
    xv = (x2-x1)/i;
    yv = (y2-y1)/i;
    zv = (z2-z1)/i;

    if( ( klabs(x1-x2)+klabs(y1-y2) ) < 3084 )
        return;

    for(i=n;i>0;i--)
    {
        x1 += xv;
        y1 += yv;
        z1 += zv;
        updatesector(x1,y1,&sect);
        if(sect >= 0)
        {
            if(sector[sect].lotag == 2)
                EGS(sect,x1,y1,z1,WATERBUBBLE,-32,4+(TRAND&3),4+(TRAND&3),TRAND&2047,0,0,ps[0].i,5);
            else
                EGS(sect,x1,y1,z1,SMALLSMOKE,-32,14,14,0,0,0,ps[0].i,5);
        }
    }
}

void hitscantrail(long x1, long y1, long z1, long x2, long y2, long z2, short ang, short atwith)
{
    long xv, yv, zv, n, j, i;
    short sect = -1;

    x1 += (sintable[(348+ang+512)&2047]/projectile[atwith].offset);
    y1 += (sintable[(ang+348)&2047]/projectile[atwith].offset);

    n = ((FindDistance2D(x1-x2,y1-y2))>>8)+1;

    z1 += (projectile[atwith].toffset<<8)+1024;

    xv = (x2-x1)/n;
    yv = (y2-y1)/n;
    zv = (z2-z1)/n;

    x1 += xv>>2;
    y1 += yv>>2;
    z1 += zv>>2;

    for(i=0;i<projectile[atwith].tnum;i++)
    {
        x1 += xv;
        y1 += yv;
        z1 += zv;
        updatesector(x1,y1,&sect);
        if(sect >= 0)
        {
            j = EGS(sect,x1,y1,z1,projectile[atwith].trail,-32,projectile[atwith].txrepeat,projectile[atwith].tyrepeat,ang,0,0,ps[0].i,0);
            changespritestat(j,1);
        }
        else continue;
    }
}

long hits(short i)
{
    long sx,sy,sz;
    short sect,hw,hs;
    long zoff;

    if(PN == APLAYER) zoff = (40<<8);
    else zoff = 0;

    hitscan(SX,SY,SZ-zoff,SECT,
            sintable[(SA+512)&2047],
            sintable[SA&2047],
            0,&sect,&hw,&hs,&sx,&sy,&sz,CLIPMASK1);

    return ( FindDistance2D( sx-SX,sy-SY ) );
}

long hitasprite(short i,short *hitsp)
{
    long sx,sy,sz,zoff;
    short sect,hw;

    if(badguy(&sprite[i]) )
        zoff = (42<<8);
    else if(PN == APLAYER) zoff = (39<<8);
    else zoff = 0;

    hitscan(SX,SY,SZ-zoff,SECT,
            sintable[(SA+512)&2047],
            sintable[SA&2047],
            0,&sect,&hw,hitsp,&sx,&sy,&sz,CLIPMASK1);

    if(hw >= 0 && (wall[hw].cstat&16) && badguy(&sprite[i]) )
        return((1<<30));

    return ( FindDistance2D(sx-SX,sy-SY) );
}

/*
long hitaspriteandwall(short i,short *hitsp,short *hitw,short *x, short *y)
{
    long sz;
    short sect;

    hitscan(SX,SY,SZ,SECT,
        sintable[(SA+512)&2047],
        sintable[SA&2047],
        0,&sect,hitw,hitsp,x,y,&sz,CLIPMASK1);

    return ( FindDistance2D(*x-SX,*y-SY) );
}
*/

long hitawall(struct player_struct *p,short *hitw)
{
    long sx,sy,sz;
    short sect,hs;

    hitscan(p->posx,p->posy,p->posz,p->cursectnum,
            sintable[(p->ang+512)&2047],
            sintable[p->ang&2047],
            0,&sect,hitw,&hs,&sx,&sy,&sz,CLIPMASK0);

    return ( FindDistance2D(sx-p->posx,sy-p->posy) );
}

short aim(spritetype *s,short aang,short atwith)
{
    char gotshrinker,gotfreezer;
    short i, j, a, k, cans;
    short aimstats[] = {10,13,1,2};
    long dx1, dy1, dx2, dy2, dx3, dy3, smax, sdist;
    long xv, yv;

    if(s->picnum == APLAYER) {
        if(!ps[s->yvel].auto_aim)
            return -1;
        if(ps[s->yvel].auto_aim == 2)
        {
            if(checkspriteflagsp(atwith,SPRITE_FLAG_PROJECTILE) && (projectile[atwith].workslike & PROJECTILE_FLAG_RPG))
                return -1;
            else switch(dynamictostatic[atwith]) {
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

    gotshrinker = s->picnum == APLAYER && *aplWeaponWorksLike[ps[s->yvel].curr_weapon] == SHRINKER_WEAPON;
    gotfreezer = s->picnum == APLAYER && *aplWeaponWorksLike[ps[s->yvel].curr_weapon] == FREEZE_WEAPON;

    smax = 0x7fffffff;

    dx1 = sintable[(a+512-aang)&2047];
    dy1 = sintable[(a-aang)&2047];
    dx2 = sintable[(a+512+aang)&2047];
    dy2 = sintable[(a+aang)&2047];

    dx3 = sintable[(a+512)&2047];
    dy3 = sintable[a&2047];

    for(k=0;k<4;k++)
    {
        if( j >= 0 )
            break;
        for(i=headspritestat[aimstats[k]];i >= 0;i=nextspritestat[i])
            if( sprite[i].xrepeat > 0 && sprite[i].extra >= 0 && (sprite[i].cstat&(257+32768)) == 257)
                if( badguy(&sprite[i]) || k < 2 )
                {
                    if(badguy(&sprite[i]) || PN == APLAYER || PN == SHARK)
                    {
                        if( PN == APLAYER &&
                                //                        ud.ffire == 0 &&
                                (gametype_flags[ud.coop]& GAMETYPE_FLAG_PLAYERSFRIENDLY ) &&
                                s->picnum == APLAYER &&
                                s != &sprite[i])
                            continue;

                        if(gotshrinker && sprite[i].xrepeat < 30 )
                        {
                            if (PN == SHARK) {
                                if(sprite[i].xrepeat < 20) continue;
                                continue;
                            } else if ((PN >= GREENSLIME)&&(PN <= GREENSLIME+7)) {
                            } else {
                                continue;
                            }
                        }
                        if(gotfreezer && sprite[i].pal == 1) continue;
                    }

                    xv = (SX-s->x);
                    yv = (SY-s->y);

                    if( (dy1*xv) <= (dx1*yv) )
                        if( ( dy2*xv ) >= (dx2*yv) )
                        {
                            sdist = mulscale(dx3,xv,14) + mulscale(dy3,yv,14);
                            if( sdist > 512 && sdist < smax )
                            {
                                if(s->picnum == APLAYER)
                                    a = (klabs(scale(SZ-s->z,10,sdist)-(ps[s->yvel].horiz+ps[s->yvel].horizoff-100)) < 100);
                                else a = 1;

                                if(PN == ORGANTIC || PN == ROTATEGUN )
                                    cans = cansee(SX,SY,SZ,SECT,s->x,s->y,s->z-(32<<8),s->sectnum);
                                else cans = cansee(SX,SY,SZ-(32<<8),SECT,s->x,s->y,s->z-(32<<8),s->sectnum);

                                if( a && cans )
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

short shoot(short i,short atwith)
{
    short sect, hitsect, hitspr, hitwall, l, sa, p, j, k=-1, wh, scount;
    long sx, sy, sz, vel, zvel, hitx, hity, hitz, x, oldzvel, dal;
    unsigned char sizx,sizy;
    spritetype *s;

    s = &sprite[i];
    sect = s->sectnum;
    zvel = 0;

    if( s->picnum == APLAYER )
    {
        p = s->yvel;

        sx = ps[p].posx;
        sy = ps[p].posy;
        sz = ps[p].posz+ps[p].pyoff+(4<<8);
        sa = ps[p].ang;

        ps[p].crack_time = 777;
    }
    else
    {
        p = -1;
        sa = s->ang;
        sx = s->x;
        sy = s->y;
        sz = s->z-((s->yrepeat*tilesizy[s->picnum])<<1)+(4<<8);
        if(s->picnum != ROTATEGUN)
        {
            sz -= (7<<8);
            if(badguy(s) && PN != COMMANDER)
            {
                sx += (sintable[(sa+1024+96)&2047]>>7);
                sy += (sintable[(sa+512+96)&2047]>>7);
            }
        }
    }

    if(checkspriteflagsp(atwith,SPRITE_FLAG_PROJECTILE))
    {
        /* Custom projectiles.  This is a big hack. */

        if (projectile[atwith].offset == 0) projectile[atwith].offset = 1;

        //            writestring(sx,sy,sz,sect,sintable[(sa+512)&2047],sintable[sa&2047],zvel<<6);
        if(projectile[atwith].workslike & PROJECTILE_FLAG_BLOOD || projectile[atwith].workslike & PROJECTILE_FLAG_KNEE)
        {

            if(projectile[atwith].workslike & PROJECTILE_FLAG_BLOOD)
            {
                if(p >= 0)
                    sa += 64 - (TRAND&127);
                else sa += 1024 + 64 - (TRAND&127);
                zvel = 1024-(TRAND&2047);
            }

            if(projectile[atwith].workslike & PROJECTILE_FLAG_KNEE)
            {
                if(p >= 0)
                {
                    zvel = (100-ps[p].horiz-ps[p].horizoff)<<5;
                    sz += (6<<8);
                    sa += 15;
                }
                else if (!(projectile[atwith].workslike & PROJECTILE_FLAG_NOAIM))
                {
                    j = ps[findplayer(s,&x)].i;
                    zvel = ( (sprite[j].z-sz)<<8 ) / (x+1);
                    sa = getangle(sprite[j].x-sx,sprite[j].y-sy);
                }
            }

            if (hittype[i].temp_data[9]) zvel = hittype[i].temp_data[9];
            hitscan(sx,sy,sz,sect,
                    sintable[(sa+512)&2047],
                    sintable[sa&2047],zvel<<6,
                    &hitsect,&hitwall,&hitspr,&hitx,&hity,&hitz,CLIPMASK1);

            if(projectile[atwith].workslike & PROJECTILE_FLAG_BLOOD)
            {

                if (projectile[atwith].range == 0)
                    projectile[atwith].range = 1024;

                if( FindDistance2D(sx-hitx,sy-hity) < projectile[atwith].range )
                    if (FindDistance2D(wall[hitwall].x-wall[wall[hitwall].point2].x,wall[hitwall].y-wall[wall[hitwall].point2].y) > (mulscale(projectile[atwith].xrepeat+8,tilesizx[projectile[atwith].decal],3)))
                        if( hitwall >= 0 && wall[hitwall].overpicnum != BIGFORCE )
                            if( (  wall[hitwall].nextsector >= 0 && hitsect >= 0 &&
                                    sector[wall[hitwall].nextsector].lotag == 0 &&
                                    sector[hitsect].lotag == 0 &&
                                    sector[wall[hitwall].nextsector].lotag == 0 &&
                                    (sector[hitsect].floorz-sector[wall[hitwall].nextsector].floorz) > (mulscale(projectile[atwith].yrepeat,tilesizy[projectile[atwith].decal],3)<<8) ) ||
                                    ( wall[hitwall].nextsector == -1 && sector[hitsect].lotag == 0 ) )
                                if( (wall[hitwall].cstat&16) == 0)
                                {
                                    if(wall[hitwall].nextsector >= 0)
                                    {
                                        k = headspritesect[wall[hitwall].nextsector];
                                        while(k >= 0)
                                        {
                                            if(sprite[k].statnum == 3 && sprite[k].lotag == 13)
                                                return -1;
                                            k = nextspritesect[k];
                                        }
                                    }

                                    if( wall[hitwall].nextwall >= 0 &&
                                            wall[wall[hitwall].nextwall].hitag != 0 )
                                        return -1;

                                    if(wall[hitwall].hitag == 0)
                                    {
                                        if(projectile[atwith].decal >= 0)
                                        {
                                            k = spawn(i,projectile[atwith].decal);
                                            /*
                                                                                    sprite[k].xvel = -12;
                                                                                    sprite[k].ang = getangle(wall[hitwall].x-wall[wall[hitwall].point2].x,
                                                                                                             wall[hitwall].y-wall[wall[hitwall].point2].y)+512;
                                                                                    sprite[k].x = hitx;
                                                                                    sprite[k].y = hity;
                                                                                    sprite[k].z = hitz;
                                                                                    sprite[k].cstat |= (TRAND&4);
                                                                    sprite[k].xrepeat = projectile[atwith].xrepeat;
                                                                sprite[k].yrepeat = projectile[atwith].yrepeat;
                                                                    sprite[k].cstat = 16+(krand()&12);
                                                                                    ssp(k,CLIPMASK0);
                                                                                    setsprite(k,sprite[k].x,sprite[k].y,sprite[k].z);
                                                                    insertspriteq(k);
                                            */

                                            if(!spriteflags[projectile[atwith].decal] & SPRITE_FLAG_DECAL)
                                                spriteflags[projectile[atwith].decal] |= SPRITE_FLAG_DECAL;

                                            k = spawn(i,projectile[atwith].decal);
                                            sprite[k].xvel = -1;
                                            sprite[k].ang = getangle(wall[hitwall].x-wall[wall[hitwall].point2].x,
                                                                     wall[hitwall].y-wall[wall[hitwall].point2].y)+512;
                                            sprite[k].x = hitx;
                                            sprite[k].y = hity;
                                            sprite[k].z = hitz;
                                            if(projectile[atwith].workslike & PROJECTILE_FLAG_RANDDECALSIZE)
                                            {
                                                wh = (TRAND&projectile[atwith].xrepeat);
                                                if (wh < projectile[atwith].yrepeat)
                                                    wh = projectile[atwith].yrepeat;
                                                sprite[k].xrepeat = wh;
                                                sprite[k].yrepeat = wh;
                                            }
                                            else
                                            {
                                                sprite[k].xrepeat = projectile[atwith].xrepeat;
                                                sprite[k].yrepeat = projectile[atwith].yrepeat;
                                            }
                                            sprite[k].z += sprite[k].yrepeat<<8;
                                            //                                        sprite[k].cstat = 16+(krand()&12);
                                            sprite[k].cstat = 16;

                                            wh = (TRAND&1);
                                            if (wh == 1)
                                                sprite[k].cstat |= 4;

                                            wh = (TRAND&1);
                                            if (wh == 1)
                                                sprite[k].cstat |= 8;

                                            wh = sprite[k].sectnum;
                                            sprite[k].shade = sector[wh].floorshade;
                                            sprite[k].x -= mulscale13(1,sintable[(sprite[k].ang+2560)&2047]);
                                            sprite[k].y -= mulscale13(1,sintable[(sprite[k].ang+2048)&2047]);

                                            ssp(k,CLIPMASK0);
                                            insertspriteq(k);
                                            changespritestat(k,5);

                                        }
                                        //                                    if( PN == OOZFILTER || PN == NEWBEAST )
                                        //                                        sprite[k].pal = 6;
                                    }
                                }
                return -1;
            }

            if(hitsect < 0) return -1;

            if ((projectile[atwith].range == 0) && (projectile[atwith].workslike & PROJECTILE_FLAG_KNEE))
                projectile[atwith].range = 1024;

            if( (projectile[atwith].range > 0) && (( klabs(sx-hitx)+klabs(sy-hity) ) > projectile[atwith].range) )
                return -1;
            else
            {
                if(hitwall >= 0 || hitspr >= 0)
                {
                    j = EGS(hitsect,hitx,hity,hitz,atwith,-15,0,0,sa,32,0,i,4);
                    thisprojectile[j].workslike = projectile[sprite[j].picnum].workslike;
                    sprite[j].extra = projectile[atwith].extra;
                    if(projectile[atwith].extra_rand > 0)
                        sprite[j].extra += (TRAND&projectile[atwith].extra_rand);
                    if(p >= 0)
                    {
                        if(projectile[atwith].spawns >= 0)
                        {
                            k = spawn(j,projectile[atwith].spawns);
                            sprite[k].z -= (8<<8);
                        }
                        if (projectile[atwith].sound > -1) spritesound(projectile[atwith].sound,j);
                    }

                    if ( p >= 0 && ps[p].steroids_amount > 0 && ps[p].steroids_amount < 400 )
                        sprite[j].extra += (max_player_health>>2);

                    if( hitspr >= 0 && sprite[hitspr].picnum != ACCESSSWITCH && sprite[hitspr].picnum != ACCESSSWITCH2 )
                    {
                        checkhitsprite(hitspr,j);
                        if(p >= 0) checkhitswitch(p,hitspr,1);
                    }

                    else if( hitwall >= 0 )
                    {
                        if( wall[hitwall].cstat&2 )
                            if(wall[hitwall].nextsector >= 0)
                                if(hitz >= (sector[wall[hitwall].nextsector].floorz) )
                                    hitwall = wall[hitwall].nextwall;

                        if( hitwall >= 0 && wall[hitwall].picnum != ACCESSSWITCH && wall[hitwall].picnum != ACCESSSWITCH2 )
                        {
                            checkhitwall(j,hitwall,hitx,hity,hitz,atwith);
                            if(p >= 0) checkhitswitch(p,hitwall,0);
                        }
                    }
                }
                else if(p >= 0 && zvel > 0 && sector[hitsect].lotag == 1)
                {
                    j = spawn(ps[p].i,WATERSPLASH2);
                    sprite[j].x = hitx;
                    sprite[j].y = hity;
                    sprite[j].ang = ps[p].ang; // Total tweek
                    sprite[j].xvel = 32;
                    ssp(i,CLIPMASK0);
                    sprite[j].xvel = 0;

                }
            }
            return -1;
        }

        if(projectile[atwith].workslike & PROJECTILE_FLAG_HITSCAN)
        {
            if( s->extra >= 0 ) s->shade = projectile[atwith].shade;

            if(p >= 0)
            {
                int angRange;
                int zRange;

                SetGameVarID(g_iAimAngleVarID,AUTO_AIM_ANGLE,i,p);
                OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
                j=-1;
                if( GetGameVarID(g_iAimAngleVarID,i,p) > 0 )
                {
                    j = aim( s, GetGameVarID(g_iAimAngleVarID,i,p),atwith);
                }
                if(j >= 0)
                {
                    dal = ((sprite[j].xrepeat*tilesizy[sprite[j].picnum])<<1)+(5<<8);
                    if (((sprite[j].picnum >= GREENSLIME)&&(sprite[j].picnum <= GREENSLIME+7))||(sprite[j].picnum ==ROTATEGUN) )
                    {
                        dal -= (8<<8);
                        return -1;
                    }
                    zvel = ( ( sprite[j].z-sz-dal )<<8 ) / ldist(&sprite[ps[p].i], &sprite[j]) ;
                    sa = getangle(sprite[j].x-sx,sprite[j].y-sy);
                }

                angRange=32;
                zRange=256;
                SetGameVarID(g_iAngRangeVarID,32, i,p);
                SetGameVarID(g_iZRangeVarID,256,i,p);
                OnEvent(EVENT_GETSHOTRANGE, i,p, -1);
                angRange=GetGameVarID(g_iAngRangeVarID,i,p);
                zRange=GetGameVarID(g_iZRangeVarID,i,p);

                sa += (angRange/2)-(TRAND&(angRange-1));
                if(j == -1)
                {
                    // no target
                    zvel = (100-ps[p].horiz-ps[p].horizoff)<<5;
                }
                zvel += (zRange/2)-(TRAND&(zRange-1));
                sz -= (2<<8);
            }
            else
            {
                j = findplayer(s,&x);
                sz -= (4<<8);
                zvel = ( (ps[j].posz-sz) <<8 ) / (ldist(&sprite[ps[j].i], s ) );
                if(s->picnum != BOSS1)
                {
                    zvel += 128-(TRAND&255);
                    sa += 32-(TRAND&63);
                }
                else
                {
                    zvel += 128-(TRAND&255);
                    sa = getangle(ps[j].posx-sx,ps[j].posy-sy)+64-(TRAND&127);
                }
            }

            if (projectile[atwith].cstat >= 0) s->cstat &= ~projectile[atwith].cstat;
            else s->cstat &= ~257;

            if (hittype[i].temp_data[9]) zvel = hittype[i].temp_data[9];
            hitscan(sx,sy,sz,sect,
                    sintable[(sa+512)&2047],
                    sintable[sa&2047],
                    zvel<<6,&hitsect,&hitwall,&hitspr,&hitx,&hity,&hitz,CLIPMASK1);


            if (projectile[atwith].cstat >= 0) s->cstat |= projectile[atwith].cstat;
            else s->cstat |= 257;

            if(hitsect < 0) return -1;

            if( (projectile[atwith].range > 0) && (( klabs(sx-hitx)+klabs(sy-hity) ) > projectile[atwith].range) ) return -1;

            if (projectile[atwith].trail > -1)
                hitscantrail(sx,sy,sz,hitx,hity,hitz,sa,atwith);

            if (projectile[atwith].workslike & PROJECTILE_FLAG_WATERBUBBLES)
            {
                if( (TRAND&15) == 0 && sector[hitsect].lotag == 2 )
                    tracers(hitx,hity,hitz,sx,sy,sz,8-(ud.multimode>>1));
            }

            if(p >= 0)
            {
                k = EGS(hitsect,hitx,hity,hitz,SHOTSPARK1,-15,10,10,sa,0,0,i,4);
                sprite[k].extra = projectile[atwith].extra;
                sprite[k].yvel = atwith; // this is a hack to allow you to detect which weapon spawned a SHOTSPARK1
                hittype[k].temp_data[6] = hitwall;
                hittype[k].temp_data[7] = hitsect;
                hittype[k].temp_data[8] = hitspr;

                if(projectile[atwith].extra_rand > 0)
                    sprite[k].extra += (TRAND%projectile[atwith].extra_rand);

                if( hitwall == -1 && hitspr == -1)
                {
                    if( zvel < 0 )
                    {
                        if( sector[hitsect].ceilingstat&1 )
                        {
                            sprite[k].xrepeat = 0;
                            sprite[k].yrepeat = 0;
                            return -1;
                        }
                        else
                            checkhitceiling(hitsect);
                    }
                    if (projectile[atwith].spawns >= 0)
                    {
                        wh=spawn(k,projectile[atwith].spawns);
                        if (projectile[atwith].sxrepeat > 4) sprite[wh].xrepeat=projectile[atwith].sxrepeat;
                        if (projectile[atwith].syrepeat > 4) sprite[wh].yrepeat=projectile[atwith].syrepeat;
                    }
                }

                if(hitspr >= 0)
                {
                    checkhitsprite(hitspr,k);
                    if( sprite[hitspr].picnum == APLAYER && (!(gametype_flags[ud.coop] & GAMETYPE_FLAG_PLAYERSFRIENDLY) || ud.ffire == 1) )
                    {
                        l = spawn(k,JIBS6);
                        sprite[k].xrepeat = sprite[k].yrepeat = 0;
                        sprite[l].z += (4<<8);
                        sprite[l].xvel = 16;
                        sprite[l].xrepeat = sprite[l].yrepeat = 24;
                        sprite[l].ang += 64-(TRAND&127);
                    }
                    else {
                        if (projectile[atwith].spawns >= 0)
                        {
                            wh=spawn(k,projectile[atwith].spawns);
                            if (projectile[atwith].sxrepeat > 4) sprite[wh].xrepeat=projectile[atwith].sxrepeat;
                            if (projectile[atwith].syrepeat > 4) sprite[wh].yrepeat=projectile[atwith].syrepeat;
                        }
                    }
                    if(p >= 0 && (
                                sprite[hitspr].picnum == DIPSWITCH ||
                                sprite[hitspr].picnum == DIPSWITCH+1 ||
                                sprite[hitspr].picnum == DIPSWITCH2 ||
                                sprite[hitspr].picnum == DIPSWITCH2+1 ||
                                sprite[hitspr].picnum == DIPSWITCH3 ||
                                sprite[hitspr].picnum == DIPSWITCH3+1 ||
                                sprite[hitspr].picnum == HANDSWITCH ||
                                sprite[hitspr].picnum == HANDSWITCH+1) )
                    {
                        checkhitswitch(p,hitspr,1);
                        return -1;
                    }
                }
                else if( hitwall >= 0 )
                {
                    if (projectile[atwith].spawns >= 0)
                    {
                        wh=spawn(k,projectile[atwith].spawns);
                        if (projectile[atwith].sxrepeat > 4) sprite[wh].xrepeat=projectile[atwith].sxrepeat;
                        if (projectile[atwith].syrepeat > 4) sprite[wh].yrepeat=projectile[atwith].syrepeat;
                    }
                    if( isadoorwall(wall[hitwall].picnum) == 1 )
                        goto DOSKIPBULLETHOLE;
                    if(p >= 0 && (
                                wall[hitwall].picnum == DIPSWITCH ||
                                wall[hitwall].picnum == DIPSWITCH+1 ||
                                wall[hitwall].picnum == DIPSWITCH2 ||
                                wall[hitwall].picnum == DIPSWITCH2+1 ||
                                wall[hitwall].picnum == DIPSWITCH3 ||
                                wall[hitwall].picnum == DIPSWITCH3+1 ||
                                wall[hitwall].picnum == HANDSWITCH ||
                                wall[hitwall].picnum == HANDSWITCH+1) )
                    {
                        checkhitswitch(p,hitwall,0);
                        return -1;
                    }

                    if(wall[hitwall].hitag != 0 || ( wall[hitwall].nextwall >= 0 && wall[wall[hitwall].nextwall].hitag != 0 ) )
                        goto DOSKIPBULLETHOLE;

                    if( hitsect >= 0 && sector[hitsect].lotag == 0 )
                        if( wall[hitwall].overpicnum != BIGFORCE )
                            if( (wall[hitwall].nextsector >= 0 && sector[wall[hitwall].nextsector].lotag == 0 ) ||
                                    ( wall[hitwall].nextsector == -1 && sector[hitsect].lotag == 0 ) )
                                if( (wall[hitwall].cstat&16) == 0)
                                {
                                    if(wall[hitwall].nextsector >= 0)
                                    {
                                        l = headspritesect[wall[hitwall].nextsector];
                                        while(l >= 0)
                                        {
                                            if(sprite[l].statnum == 3 && sprite[l].lotag == 13)
                                                goto DOSKIPBULLETHOLE;
                                            l = nextspritesect[l];
                                        }
                                    }

                                    l = headspritestat[5];
                                    while(l >= 0)
                                    {
                                        if(sprite[l].picnum == projectile[atwith].decal)
                                            if(dist(&sprite[l],&sprite[k]) < (12+(TRAND&7)) )
                                                goto DOSKIPBULLETHOLE;
                                        l = nextspritestat[l];
                                    }
                                    if (projectile[atwith].decal >= 0)
                                    {
                                        l = spawn(k,projectile[atwith].decal);
                                        sprite[l].xvel = -1;
                                        sprite[l].ang = getangle(wall[hitwall].x-wall[wall[hitwall].point2].x,
                                                                 wall[hitwall].y-wall[wall[hitwall].point2].y)+512;
                                        if(projectile[atwith].workslike & PROJECTILE_FLAG_RANDDECALSIZE)
                                        {
                                            wh = (TRAND&projectile[atwith].xrepeat);
                                            if (wh < projectile[atwith].yrepeat)
                                                wh = projectile[atwith].yrepeat;
                                            sprite[l].xrepeat = wh;
                                            sprite[l].yrepeat = wh;
                                        }
                                        else
                                        {
                                            sprite[l].xrepeat = projectile[atwith].xrepeat;
                                            sprite[l].yrepeat = projectile[atwith].yrepeat;
                                        }
                                        sprite[l].cstat = 16+(krand()&12);
                                        sprite[l].x -= mulscale13(1,sintable[(sprite[l].ang+2560)&2047]);
                                        sprite[l].y -= mulscale13(1,sintable[(sprite[l].ang+2048)&2047]);

                                        ssp(l,CLIPMASK0);
                                        insertspriteq(l);
                                    }
                                }

DOSKIPBULLETHOLE:

                    if( wall[hitwall].cstat&2 )
                        if(wall[hitwall].nextsector >= 0)
                            if(hitz >= (sector[wall[hitwall].nextsector].floorz) )
                                hitwall = wall[hitwall].nextwall;

                    checkhitwall(k,hitwall,hitx,hity,hitz,atwith);
                }
            }
            else
            {
                k = EGS(hitsect,hitx,hity,hitz,SHOTSPARK1,-15,24,24,sa,0,0,i,4);
                sprite[k].extra = projectile[atwith].extra;
                sprite[k].yvel = atwith; // this is a hack to allow you to detect which weapon spawned a SHOTSPARK1
                hittype[k].temp_data[6] = hitwall;
                hittype[k].temp_data[7] = hitsect;
                hittype[k].temp_data[8] = hitspr;

                if( hitspr >= 0 )
                {
                    checkhitsprite(hitspr,k);
                    if( sprite[hitspr].picnum != APLAYER )
                    {
                        if (projectile[atwith].spawns >= 0)
                        {
                            wh=spawn(k,projectile[atwith].spawns);
                            if (projectile[atwith].sxrepeat > 4) sprite[wh].xrepeat=projectile[atwith].sxrepeat;
                            if (projectile[atwith].syrepeat > 4) sprite[wh].yrepeat=projectile[atwith].syrepeat;
                        }
                    }
                    else sprite[k].xrepeat = sprite[k].yrepeat = 0;
                }
                else if( hitwall >= 0 )
                    checkhitwall(k,hitwall,hitx,hity,hitz,atwith);
            }

            if( (TRAND&255) < 4 )
                if (projectile[atwith].isound >= 0)
                    xyzsound(projectile[atwith].isound,k,hitx,hity,hitz);

            return -1;
        }

        if(projectile[atwith].workslike & PROJECTILE_FLAG_RPG)
        {

            /*            if(projectile[atwith].workslike & PROJECTILE_FLAG_FREEZEBLAST)
                            sz += (3<<8);*/

            if( s->extra >= 0 ) s->shade = projectile[atwith].shade;

            scount = 1;
            vel = projectile[atwith].vel;

            j = -1;

            if(p >= 0)
            {
                //            j = aim( s, AUTO_AIM_ANGLE ); // 48
                SetGameVarID(g_iAimAngleVarID,AUTO_AIM_ANGLE,i,p);
                OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
                j=-1;
                if( GetGameVarID(g_iAimAngleVarID,i,p) > 0 )
                {
                    j = aim( s, GetGameVarID(g_iAimAngleVarID,i,p),atwith);
                }
                if(j >= 0)
                {
                    dal = ((sprite[j].xrepeat*tilesizy[sprite[j].picnum])<<1)+(8<<8);
                    zvel = ( (sprite[j].z-sz-dal)*vel ) / ldist(&sprite[ps[p].i], &sprite[j]);
                    if( sprite[j].picnum != RECON )
                        sa = getangle(sprite[j].x-sx,sprite[j].y-sy);
                }
                //                else zvel = (100-ps[p].horiz-ps[p].horizoff)*81;
                else zvel = ((100-ps[p].horiz-ps[p].horizoff)*(projectile[atwith].vel/8));
                if (projectile[atwith].sound > -1) spritesound(projectile[atwith].sound,i);
            }
            else
            {
                if (!(projectile[atwith].workslike & PROJECTILE_FLAG_NOAIM))
                {
                    j = findplayer(s,&x);
                    sa = getangle(ps[j].oposx-sx,ps[j].oposy-sy);

                    l = ldist(&sprite[ps[j].i],s);
                    zvel = ( (ps[j].oposz-sz)*vel) / l;

                    if( badguy(s) && (s->hitag&face_player_smart) )
                        sa = s->ang+(TRAND&31)-16;
                }
            }



            if( p >= 0 && j >= 0)
                l = j;
            else l = -1;

            /*                        j = EGS(sect,
                                            sx+(sintable[(348+sa+512)&2047]/448),
                                            sy+(sintable[(sa+348)&2047]/448),
                                            sz-(1<<8),atwith,0,14,14,sa,vel,zvel,i,4);*/
            if (hittype[i].temp_data[9]) zvel = hittype[i].temp_data[9];
            j = EGS(sect,
                    sx+(sintable[(348+sa+512)&2047]/projectile[atwith].offset),
                    sy+(sintable[(sa+348)&2047]/projectile[atwith].offset),
                    sz-(1<<8),atwith,0,14,14,sa,vel,zvel,i,4);

            sprite[j].xrepeat=projectile[atwith].xrepeat;
            sprite[j].yrepeat=projectile[atwith].yrepeat;


            if(projectile[atwith].extra_rand > 0)
                sprite[j].extra += (TRAND&projectile[atwith].extra_rand);
            if(!(projectile[atwith].workslike & PROJECTILE_FLAG_BOUNCESOFFWALLS))
                sprite[j].yvel = l;
            else
            {
                if (projectile[atwith].bounces >= 1) sprite[j].yvel = projectile[atwith].bounces;
                else sprite[j].yvel = numfreezebounces;
                //                sprite[j].xrepeat >>= 1;
                //                sprite[j].yrepeat >>= 1;
                sprite[j].zvel -= (2<<4);
            }
            /*
                        if(p == -1)
                        {
                            if(!(projectile[atwith].workslike & PROJECTILE_FLAG_BOUNCESOFFWALLS))
                            {
                                sprite[j].xrepeat = projectile[atwith].xrepeat; // 30
                                sprite[j].yrepeat = projectile[atwith].yrepeat;
                                sprite[j].extra >>= 2;
                            }
                        }
            */
            if (projectile[atwith].cstat >= 0) sprite[j].cstat = projectile[atwith].cstat;
            else sprite[j].cstat = 128;
            if (projectile[atwith].clipdist >= 0) sprite[j].clipdist = projectile[atwith].clipdist;
            else sprite[j].clipdist = 40;

            Bmemcpy(&thisprojectile[j], &projectile[sprite[j].picnum], sizeof(projectile[sprite[j].picnum]));

            //            sa = s->ang+32-(TRAND&63);
            //            zvel = oldzvel+512-(TRAND&1023);

            return j;
        }

    }

    else

    {
        switch(dynamictostatic[atwith])
        {
        case BLOODSPLAT1__STATIC:
        case BLOODSPLAT2__STATIC:
        case BLOODSPLAT3__STATIC:
        case BLOODSPLAT4__STATIC:

            if(p >= 0)
                sa += 64 - (TRAND&127);
            else sa += 1024 + 64 - (TRAND&127);
            zvel = 1024-(TRAND&2047);
        case KNEE__STATIC:
            if(atwith == KNEE )
            {
                if(p >= 0)
                {
                    zvel = (100-ps[p].horiz-ps[p].horizoff)<<5;
                    sz += (6<<8);
                    sa += 15;
                }
                else
                {
                    j = ps[findplayer(s,&x)].i;
                    zvel = ( (sprite[j].z-sz)<<8 ) / (x+1);
                    sa = getangle(sprite[j].x-sx,sprite[j].y-sy);
                }
            }

            //            writestring(sx,sy,sz,sect,sintable[(sa+512)&2047],sintable[sa&2047],zvel<<6);
            if (hittype[i].temp_data[9]) zvel = hittype[i].temp_data[9];
            hitscan(sx,sy,sz,sect,
                    sintable[(sa+512)&2047],
                    sintable[sa&2047],zvel<<6,
                    &hitsect,&hitwall,&hitspr,&hitx,&hity,&hitz,CLIPMASK1);

            if( atwith == BLOODSPLAT1 || atwith == BLOODSPLAT2 || atwith == BLOODSPLAT3 || atwith == BLOODSPLAT4 )
            {
                if( FindDistance2D(sx-hitx,sy-hity) < 1024 )
                    if( hitwall >= 0 && wall[hitwall].overpicnum != BIGFORCE )
                        if( ( wall[hitwall].nextsector >= 0 && hitsect >= 0 &&
                                sector[wall[hitwall].nextsector].lotag == 0 &&
                                sector[hitsect].lotag == 0 &&
                                sector[wall[hitwall].nextsector].lotag == 0 &&
                                (sector[hitsect].floorz-sector[wall[hitwall].nextsector].floorz) > (16<<8) ) ||
                                ( wall[hitwall].nextsector == -1 && sector[hitsect].lotag == 0 ) )
                            if( (wall[hitwall].cstat&16) == 0)
                            {
                                if(wall[hitwall].nextsector >= 0)
                                {
                                    k = headspritesect[wall[hitwall].nextsector];
                                    while(k >= 0)
                                    {
                                        if(sprite[k].statnum == 3 && sprite[k].lotag == 13)
                                            return -1;
                                        k = nextspritesect[k];
                                    }
                                }

                                if( wall[hitwall].nextwall >= 0 &&
                                        wall[wall[hitwall].nextwall].hitag != 0 )
                                    return -1;

                                if(wall[hitwall].hitag == 0)
                                {
                                    k = spawn(i,atwith);
                                    sprite[k].xvel = -12;
                                    sprite[k].ang = getangle(wall[hitwall].x-wall[wall[hitwall].point2].x,
                                                             wall[hitwall].y-wall[wall[hitwall].point2].y)+512;
                                    sprite[k].x = hitx;
                                    sprite[k].y = hity;
                                    sprite[k].z = hitz;
                                    sprite[k].cstat |= (TRAND&4);
                                    ssp(k,CLIPMASK0);
                                    setsprite(k,sprite[k].x,sprite[k].y,sprite[k].z);
                                    if( PN == OOZFILTER || PN == NEWBEAST )
                                        sprite[k].pal = 6;
                                }
                            }
                return -1;
            }

            if(hitsect < 0) break;

            if( ( klabs(sx-hitx)+klabs(sy-hity) ) < 1024 )
            {
                if(hitwall >= 0 || hitspr >= 0)
                {
                    j = EGS(hitsect,hitx,hity,hitz,KNEE,-15,0,0,sa,32,0,i,4);
                    sprite[j].extra += (TRAND&7);
                    if(p >= 0)
                    {
                        k = spawn(j,SMALLSMOKE);
                        sprite[k].z -= (8<<8);
                        spritesound(KICK_HIT,j);
                    }

                    if ( p >= 0 && ps[p].steroids_amount > 0 && ps[p].steroids_amount < 400 )
                        sprite[j].extra += (max_player_health>>2);

                    if( hitspr >= 0 && sprite[hitspr].picnum != ACCESSSWITCH && sprite[hitspr].picnum != ACCESSSWITCH2 )
                    {
                        checkhitsprite(hitspr,j);
                        if(p >= 0) checkhitswitch(p,hitspr,1);
                    }

                    else if( hitwall >= 0 )
                    {
                        if( wall[hitwall].cstat&2 )
                            if(wall[hitwall].nextsector >= 0)
                                if(hitz >= (sector[wall[hitwall].nextsector].floorz) )
                                    hitwall = wall[hitwall].nextwall;

                        if( hitwall >= 0 && wall[hitwall].picnum != ACCESSSWITCH && wall[hitwall].picnum != ACCESSSWITCH2 )
                        {
                            checkhitwall(j,hitwall,hitx,hity,hitz,atwith);
                            if(p >= 0) checkhitswitch(p,hitwall,0);
                        }
                    }
                }
                else if(p >= 0 && zvel > 0 && sector[hitsect].lotag == 1)
                {
                    j = spawn(ps[p].i,WATERSPLASH2);
                    sprite[j].x = hitx;
                    sprite[j].y = hity;
                    sprite[j].ang = ps[p].ang; // Total tweek
                    sprite[j].xvel = 32;
                    ssp(i,CLIPMASK0);
                    sprite[j].xvel = 0;

                }
            }

            break;

        case SHOTSPARK1__STATIC:
        case SHOTGUN__STATIC:
        case CHAINGUN__STATIC:

            if( s->extra >= 0 ) s->shade = -96;

            if(p >= 0)
            {
                int angRange;
                int zRange;

                SetGameVarID(g_iAimAngleVarID,AUTO_AIM_ANGLE,i,p);
                OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
                j=-1;
                if( GetGameVarID(g_iAimAngleVarID,i,p) > 0 )
                {
                    j = aim( s, GetGameVarID(g_iAimAngleVarID,i,p),atwith);
                }
                if(j >= 0)
                {
                    dal = ((sprite[j].xrepeat*tilesizy[sprite[j].picnum])<<1)+(5<<8);
                    if (((sprite[j].picnum>=GREENSLIME)&&(sprite[j].picnum<=GREENSLIME+7))||(sprite[j].picnum==ROTATEGUN)) {

                        dal -= (8<<8);

                    }
                    zvel = ( ( sprite[j].z-sz-dal )<<8 ) / ldist(&sprite[ps[p].i], &sprite[j]) ;
                    sa = getangle(sprite[j].x-sx,sprite[j].y-sy);
                }

                angRange=32;
                zRange=256;
                SetGameVarID(g_iAngRangeVarID,32, i,p);
                SetGameVarID(g_iZRangeVarID,256,i,p);
                OnEvent(EVENT_GETSHOTRANGE, i,p, -1);
                angRange=GetGameVarID(g_iAngRangeVarID,i,p);
                zRange=GetGameVarID(g_iZRangeVarID,i,p);

                sa += (angRange/2)-(TRAND&(angRange-1));
                if(j == -1)
                {
                    // no target
                    zvel = (100-ps[p].horiz-ps[p].horizoff)<<5;
                }
                zvel += (zRange/2)-(TRAND&(zRange-1));
                sz -= (2<<8);
            }
            else
            {
                j = findplayer(s,&x);
                sz -= (4<<8);
                zvel = ( (ps[j].posz-sz) <<8 ) / (ldist(&sprite[ps[j].i], s ) );
                if(s->picnum != BOSS1)
                {
                    zvel += 128-(TRAND&255);
                    sa += 32-(TRAND&63);
                }
                else
                {
                    zvel += 128-(TRAND&255);
                    sa = getangle(ps[j].posx-sx,ps[j].posy-sy)+64-(TRAND&127);
                }
            }

            s->cstat &= ~257;
            if (hittype[i].temp_data[9]) zvel = hittype[i].temp_data[9];
            hitscan(sx,sy,sz,sect,
                    sintable[(sa+512)&2047],
                    sintable[sa&2047],
                    zvel<<6,&hitsect,&hitwall,&hitspr,&hitx,&hity,&hitz,CLIPMASK1);
            s->cstat |= 257;

            if(hitsect < 0) return -1;

            if( (TRAND&15) == 0 && sector[hitsect].lotag == 2 )
                tracers(hitx,hity,hitz,sx,sy,sz,8-(ud.multimode>>1));

            if(p >= 0)
            {
                k = EGS(hitsect,hitx,hity,hitz,SHOTSPARK1,-15,10,10,sa,0,0,i,4);
                sprite[k].extra = *actorscrptr[atwith];
                sprite[k].extra += (TRAND%6);
                sprite[k].yvel = atwith; // this is a hack to allow you to detect which weapon spawned a SHOTSPARK1
                hittype[k].temp_data[6] = hitwall;
                hittype[k].temp_data[7] = hitsect;
                hittype[k].temp_data[8] = hitspr;


                if( hitwall == -1 && hitspr == -1)
                {
                    if( zvel < 0 )
                    {
                        if( sector[hitsect].ceilingstat&1 )
                        {
                            sprite[k].xrepeat = 0;
                            sprite[k].yrepeat = 0;
                            return -1;
                        }
                        else
                            checkhitceiling(hitsect);
                    }
                    spawn(k,SMALLSMOKE);
                }

                if(hitspr >= 0)
                {
                    checkhitsprite(hitspr,k);
                    if( sprite[hitspr].picnum == APLAYER && (!(gametype_flags[ud.coop]&GAMETYPE_FLAG_PLAYERSFRIENDLY ) || ud.ffire == 1) )
                    {
                        l = spawn(k,JIBS6);
                        sprite[k].xrepeat = sprite[k].yrepeat = 0;
                        sprite[l].z += (4<<8);
                        sprite[l].xvel = 16;
                        sprite[l].xrepeat = sprite[l].yrepeat = 24;
                        sprite[l].ang += 64-(TRAND&127);
                    }
                    else spawn(k,SMALLSMOKE);

                    if(p >= 0 && (
                                sprite[hitspr].picnum == DIPSWITCH ||
                                sprite[hitspr].picnum == DIPSWITCH+1 ||
                                sprite[hitspr].picnum == DIPSWITCH2 ||
                                sprite[hitspr].picnum == DIPSWITCH2+1 ||
                                sprite[hitspr].picnum == DIPSWITCH3 ||
                                sprite[hitspr].picnum == DIPSWITCH3+1 ||
                                sprite[hitspr].picnum == HANDSWITCH ||
                                sprite[hitspr].picnum == HANDSWITCH+1) )
                    {
                        checkhitswitch(p,hitspr,1);
                        return -1;
                    }
                }
                else if( hitwall >= 0 )
                {
                    spawn(k,SMALLSMOKE);

                    if( isadoorwall(wall[hitwall].picnum) == 1 )
                        goto SKIPBULLETHOLE;
                    if(p >= 0 && (
                                wall[hitwall].picnum == DIPSWITCH ||
                                wall[hitwall].picnum == DIPSWITCH+1 ||
                                wall[hitwall].picnum == DIPSWITCH2 ||
                                wall[hitwall].picnum == DIPSWITCH2+1 ||
                                wall[hitwall].picnum == DIPSWITCH3 ||
                                wall[hitwall].picnum == DIPSWITCH3+1 ||
                                wall[hitwall].picnum == HANDSWITCH ||
                                wall[hitwall].picnum == HANDSWITCH+1) )
                    {
                        checkhitswitch(p,hitwall,0);
                        return -1;
                    }

                    if(wall[hitwall].hitag != 0 || ( wall[hitwall].nextwall >= 0 && wall[wall[hitwall].nextwall].hitag != 0 ) )
                        goto SKIPBULLETHOLE;

                    if( hitsect >= 0 && sector[hitsect].lotag == 0 )
                        if( wall[hitwall].overpicnum != BIGFORCE )
                            if( (wall[hitwall].nextsector >= 0 && sector[wall[hitwall].nextsector].lotag == 0 ) ||
                                    ( wall[hitwall].nextsector == -1 && sector[hitsect].lotag == 0 ) )
                                if( (wall[hitwall].cstat&16) == 0)
                                {
                                    if(wall[hitwall].nextsector >= 0)
                                    {
                                        l = headspritesect[wall[hitwall].nextsector];
                                        while(l >= 0)
                                        {
                                            if(sprite[l].statnum == 3 && sprite[l].lotag == 13)
                                                goto SKIPBULLETHOLE;
                                            l = nextspritesect[l];
                                        }
                                    }

                                    l = headspritestat[5];
                                    while(l >= 0)
                                    {
                                        if(sprite[l].picnum == BULLETHOLE)
                                            if(dist(&sprite[l],&sprite[k]) < (12+(TRAND&7)) )
                                                goto SKIPBULLETHOLE;
                                        l = nextspritestat[l];
                                    }
                                    l = spawn(k,BULLETHOLE);
                                    sprite[l].xvel = -1;
                                    sprite[l].x = hitx;
                                    sprite[l].y = hity;
                                    sprite[l].z = hitz;

                                    sprite[l].ang = getangle(wall[hitwall].x-wall[wall[hitwall].point2].x,
                                                             wall[hitwall].y-wall[wall[hitwall].point2].y)+512;

                                    sprite[l].x -= mulscale13(1,sintable[(sprite[l].ang+2560)&2047]);
                                    sprite[l].y -= mulscale13(1,sintable[(sprite[l].ang+2048)&2047]);
                                    ssp(l,CLIPMASK0);
                                }

SKIPBULLETHOLE:

                    if( wall[hitwall].cstat&2 )
                        if(wall[hitwall].nextsector >= 0)
                            if(hitz >= (sector[wall[hitwall].nextsector].floorz) )
                                hitwall = wall[hitwall].nextwall;

                    checkhitwall(k,hitwall,hitx,hity,hitz,SHOTSPARK1);
                }
            }
            else
            {
                k = EGS(hitsect,hitx,hity,hitz,SHOTSPARK1,-15,24,24,sa,0,0,i,4);
                sprite[k].extra = *actorscrptr[atwith];
                sprite[k].yvel = atwith; // this is a hack to allow you to detect which weapon spawned a SHOTSPARK1
                hittype[k].temp_data[6] = hitwall;
                hittype[k].temp_data[7] = hitsect;
                hittype[k].temp_data[8] = hitspr;

                if( hitspr >= 0 )
                {
                    checkhitsprite(hitspr,k);
                    if( sprite[hitspr].picnum != APLAYER )
                        spawn(k,SMALLSMOKE);
                    else sprite[k].xrepeat = sprite[k].yrepeat = 0;
                }
                else if( hitwall >= 0 )
                    checkhitwall(k,hitwall,hitx,hity,hitz,SHOTSPARK1);
            }

            if( (TRAND&255) < 4 )
                xyzsound(PISTOL_RICOCHET,k,hitx,hity,hitz);

            return -1;

        case FIRELASER__STATIC:
        case SPIT__STATIC:
        case COOLEXPLOSION1__STATIC:

            if( s->extra >= 0 ) s->shade = -96;

            scount = 1;
            if(atwith == SPIT) vel = 292;
            else
            {
                if(atwith == COOLEXPLOSION1)
                {
                    if(s->picnum == BOSS2) vel = 644;
                    else vel = 348;
                    sz -= (4<<7);
                }
                else
                {
                    vel = 840;
                    sz -= (4<<7);
                }
            }

            if(p >= 0)
            {
                //            j = aim( s, AUTO_AIM_ANGLE );
                SetGameVarID(g_iAimAngleVarID,AUTO_AIM_ANGLE,i,p);
                OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
                j=-1;
                if( GetGameVarID(g_iAimAngleVarID,i,p) > 0 )
                {
                    j = aim( s, GetGameVarID(g_iAimAngleVarID,i,p),atwith);
                }

                if(j >= 0)
                {
                    dal = ((sprite[j].xrepeat*tilesizy[sprite[j].picnum])<<1)-(12<<8);
                    zvel = ((sprite[j].z-sz-dal)*vel ) / ldist(&sprite[ps[p].i], &sprite[j]) ;
                    sa = getangle(sprite[j].x-sx,sprite[j].y-sy);
                }
                else
                    zvel = (100-ps[p].horiz-ps[p].horizoff)*98;
            }
            else
            {
                j = findplayer(s,&x);
                //                sa = getangle(ps[j].oposx-sx,ps[j].oposy-sy);
                sa += 16-(TRAND&31);
                zvel = ( ( (ps[j].oposz - sz + (3<<8) ) )*vel ) / ldist(&sprite[ps[j].i],s);
            }
            if (hittype[i].temp_data[9]) zvel = hittype[i].temp_data[9];
            oldzvel = zvel;

        if(atwith == SPIT) { sizx = 18;sizy = 18,sz -= (10<<8); }
            else
            {
                if( atwith == FIRELASER )
                {
                    if(p >= 0)
                    {

                        sizx = 34;
                        sizy = 34;
                    }
                    else
                    {
                        sizx = 18;
                        sizy = 18;
                    }
                }
                else
                {
                    sizx = 18;
                    sizy = 18;
                }
            }

            if(p >= 0) sizx = 7,sizy = 7;

            while(scount > 0)
            {
                j = EGS(sect,sx,sy,sz,atwith,-127,sizx,sizy,sa,vel,zvel,i,4);
                sprite[j].extra += (TRAND&7);

                if(atwith == COOLEXPLOSION1)
                {
                    sprite[j].shade = 0;
                    if(PN == BOSS2)
                    {
                        l = sprite[j].xvel;
                        sprite[j].xvel = 1024;
                        ssp(j,CLIPMASK0);
                        sprite[j].xvel = l;
                        sprite[j].ang += 128-(TRAND&255);
                    }
                }

                sprite[j].cstat = 128;
                sprite[j].clipdist = 4;

                sa = s->ang+32-(TRAND&63);
                zvel = oldzvel+512-(TRAND&1023);

                scount--;
            }

            return j;

        case FREEZEBLAST__STATIC:
            sz += (3<<8);
        case RPG__STATIC:

            if( s->extra >= 0 ) s->shade = -96;

            scount = 1;
            vel = 644;

            j = -1;

            if(p >= 0)
            {
                //            j = aim( s, AUTO_AIM_ANGLE ); // 48
                SetGameVarID(g_iAimAngleVarID,AUTO_AIM_ANGLE,i,p);
                OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
                j=-1;
                if( GetGameVarID(g_iAimAngleVarID,i,p) > 0 )
                {
                    j = aim( s, GetGameVarID(g_iAimAngleVarID,i,p),atwith);
                }

                if(j >= 0)
                {
                    dal = ((sprite[j].xrepeat*tilesizy[sprite[j].picnum])<<1)+(8<<8);
                    zvel = ( (sprite[j].z-sz-dal)*vel ) / ldist(&sprite[ps[p].i], &sprite[j]);
                    if( sprite[j].picnum != RECON )
                        sa = getangle(sprite[j].x-sx,sprite[j].y-sy);
                }
                else zvel = (100-ps[p].horiz-ps[p].horizoff)*81;
                if(atwith == RPG)
                    spritesound(RPG_SHOOT,i);

            }
            else
            {
                j = findplayer(s,&x);
                sa = getangle(ps[j].oposx-sx,ps[j].oposy-sy);
                if(PN == BOSS3)
                    sz -= (32<<8);
                else if(PN == BOSS2)
                {
                    vel += 128;
                    sz += 24<<8;
                }

                l = ldist(&sprite[ps[j].i],s);
                zvel = ( (ps[j].oposz-sz)*vel) / l;

                if( badguy(s) && (s->hitag&face_player_smart) )
                    sa = s->ang+(TRAND&31)-16;
            }

            if( p >= 0 && j >= 0)
                l = j;
            else l = -1;
            if (hittype[i].temp_data[9]) zvel = hittype[i].temp_data[9];
            j = EGS(sect,
                    sx+(sintable[(348+sa+512)&2047]/448),
                    sy+(sintable[(sa+348)&2047]/448),
                    sz-(1<<8),atwith,0,14,14,sa,vel,zvel,i,4);

            sprite[j].extra += (TRAND&7);
            if(atwith != FREEZEBLAST)
                sprite[j].yvel = l;
            else
            {
                sprite[j].yvel = numfreezebounces;
                sprite[j].xrepeat >>= 1;
                sprite[j].yrepeat >>= 1;
                sprite[j].zvel -= (2<<4);
            }

            if(p == -1)
            {
                if(PN == BOSS3)
                {
                    if(TRAND&1)
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
                else if(PN == BOSS2)
                {
                    sprite[j].x -= sintable[sa&2047]/56;
                    sprite[j].y -= sintable[(sa+1024+512)&2047]/56;
                    sprite[j].ang -= 8+(TRAND&255)-128;
                    sprite[j].xrepeat = 24;
                    sprite[j].yrepeat = 24;
                }
                else if(atwith != FREEZEBLAST)
                {
                    sprite[j].xrepeat = 30;
                    sprite[j].yrepeat = 30;
                    sprite[j].extra >>= 2;
                }
            }

            else if(*aplWeaponWorksLike[ps[p].curr_weapon] == DEVISTATOR_WEAPON)
            {
                sprite[j].extra >>= 2;
                sprite[j].ang += 16-(TRAND&31);
                sprite[j].zvel += 256-(TRAND&511);

                if( ps[p].hbomb_hold_delay )
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
            if(atwith == RPG)
                sprite[j].clipdist = 4;
            else
                sprite[j].clipdist = 40;

            break;

        case HANDHOLDINGLASER__STATIC:

            if(p >= 0)
                zvel = (100-ps[p].horiz-ps[p].horizoff)*32;
            else zvel = 0;
            if (hittype[i].temp_data[9]) zvel = hittype[i].temp_data[9];
            hitscan(sx,sy,sz-ps[p].pyoff,sect,
                    sintable[(sa+512)&2047],
                    sintable[sa&2047],
                    zvel<<6,&hitsect,&hitwall,&hitspr,&hitx,&hity,&hitz,CLIPMASK1);

            j = 0;
            if(hitspr >= 0) break;

            if(hitwall >= 0 && hitsect >= 0)
                if( ((hitx-sx)*(hitx-sx)+(hity-sy)*(hity-sy)) < (290*290) )
                {
                    if( wall[hitwall].nextsector >= 0)
                    {
                        if( sector[wall[hitwall].nextsector].lotag <= 2 && sector[hitsect].lotag <= 2 )
                            j = 1;
                    }
                    else if( sector[hitsect].lotag <= 2 )
                        j = 1;
                }

            if(j == 1)
            {
                long lTripBombControl=GetGameVar("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, ps[p].i, p);
                k = EGS(hitsect,hitx,hity,hitz,TRIPBOMB,-16,4,5,sa,0,0,i,6);
                if(lTripBombControl & TRIPBOMB_TIMER)
                {
                    long lLifetime=GetGameVar("STICKYBOMB_LIFETIME", NAM_GRENADE_LIFETIME, ps[p].i, p);
                    long lLifetimeVar=GetGameVar("STICKYBOMB_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, ps[p].i, p);
                    // set timer.  blows up when at zero....
                    hittype[k].temp_data[7]=lLifetime
                                            + mulscale(krand(),lLifetimeVar, 14)
                                            - lLifetimeVar;
                    hittype[k].temp_data[6]=1;
                }
                else

                    sprite[k].hitag = k;
                spritesound(LASERTRIP_ONWALL,k);
                sprite[k].xvel = -20;
                ssp(k,CLIPMASK0);
                sprite[k].cstat = 16;
                hittype[k].temp_data[5] = sprite[k].ang = getangle(wall[hitwall].x-wall[wall[hitwall].point2].x,wall[hitwall].y-wall[wall[hitwall].point2].y)-512;


            }
            return j?k:-1;

        case BOUNCEMINE__STATIC:
        case MORTER__STATIC:

            if( s->extra >= 0 ) s->shade = -96;

            j = ps[findplayer(s,&x)].i;
            x = ldist(&sprite[j],s);

            zvel = -x>>1;

            if(zvel < -4096)
                zvel = -2048;
            vel = x>>4;
            if (hittype[i].temp_data[9]) zvel = hittype[i].temp_data[9];
            EGS(sect,
                sx+(sintable[(512+sa+512)&2047]>>8),
                sy+(sintable[(sa+512)&2047]>>8),
                sz+(6<<8),atwith,-64,32,32,sa,vel,zvel,i,1);
            break;

        case GROWSPARK__STATIC:

            if(p >= 0)
            {
                //            j = aim( s, AUTO_AIM_ANGLE );
                SetGameVarID(g_iAimAngleVarID,AUTO_AIM_ANGLE,i,p);
                OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
                j=-1;
                if( GetGameVarID(g_iAimAngleVarID,i,p) > 0 )
                {
                    j = aim( s, GetGameVarID(g_iAimAngleVarID,i,p),atwith);
                }

                if(j >= 0)
                {
                    dal = ((sprite[j].xrepeat*tilesizy[sprite[j].picnum])<<1)+(5<<8);
                    if (((sprite[j].picnum >= GREENSLIME)&&(sprite[j].picnum <= GREENSLIME+7))||(sprite[j].picnum ==ROTATEGUN) ) {
                        dal -= (8<<8);

                    }
                    zvel = ( ( sprite[j].z-sz-dal )<<8 ) / (ldist(&sprite[ps[p].i], &sprite[j]) );
                    sa = getangle(sprite[j].x-sx,sprite[j].y-sy);
                }
                else
                {
                    sa += 16-(TRAND&31);
                    zvel = (100-ps[p].horiz-ps[p].horizoff)<<5;
                    zvel += 128-(TRAND&255);
                }

                sz -= (2<<8);
            }
            else
            {
                j = findplayer(s,&x);
                sz -= (4<<8);
                zvel = ( (ps[j].posz-sz) <<8 ) / (ldist(&sprite[ps[j].i], s ) );
                zvel += 128-(TRAND&255);
                sa += 32-(TRAND&63);
            }

            k = 0;

            //            RESHOOTGROW:

            s->cstat &= ~257;
            if (hittype[i].temp_data[9]) zvel = hittype[i].temp_data[9];
            hitscan(sx,sy,sz,sect,
                    sintable[(sa+512)&2047],
                    sintable[sa&2047],
                    zvel<<6,&hitsect,&hitwall,&hitspr,&hitx,&hity,&hitz,CLIPMASK1);

            s->cstat |= 257;

            j = EGS(sect,hitx,hity,hitz,GROWSPARK,-16,28,28,sa,0,0,i,1);

            sprite[j].pal = 2;
            sprite[j].cstat |= 130;
            sprite[j].xrepeat = sprite[j].yrepeat = 1;

            if( hitwall == -1 && hitspr == -1 && hitsect >= 0)
            {
                if( zvel < 0 && (sector[hitsect].ceilingstat&1) == 0)
                    checkhitceiling(hitsect);
            }
            else if(hitspr >= 0) checkhitsprite(hitspr,j);
            else if(hitwall >= 0 && wall[hitwall].picnum != ACCESSSWITCH && wall[hitwall].picnum != ACCESSSWITCH2 )
            {
                /*    if(wall[hitwall].overpicnum == MIRROR && k == 0)
                    {
                        l = getangle(
                            wall[wall[hitwall].point2].x-wall[hitwall].x,
                            wall[wall[hitwall].point2].y-wall[hitwall].y);

                        sx = hitx;
                        sy = hity;
                        sz = hitz;
                        sect = hitsect;
                        sa = ((l<<1) - sa)&2047;
                        sx += sintable[(sa+512)&2047]>>12;
                        sy += sintable[sa&2047]>>12;

                        k++;
                        goto RESHOOTGROW;
                    }
                    else */
                checkhitwall(j,hitwall,hitx,hity,hitz,atwith);
            }

            break;

        case SHRINKER__STATIC:
            if( s->extra >= 0 ) s->shade = -96;
            if(p >= 0)
            {
                //            j = aim( s, AUTO_AIM_ANGLE );
                SetGameVarID(g_iAimAngleVarID,AUTO_AIM_ANGLE,i,p);
                OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
                j=-1;
                if( GetGameVarID(g_iAimAngleVarID,i,p) > 0 )
                {
                    j = aim( s, GetGameVarID(g_iAimAngleVarID,i,p),atwith);
                }

                if(j >= 0)
                {
                    dal = ((sprite[j].xrepeat*tilesizy[sprite[j].picnum])<<1);
                    zvel = ( (sprite[j].z-sz-dal-(4<<8))*768) / (ldist( &sprite[ps[p].i], &sprite[j]));
                    sa = getangle(sprite[j].x-sx,sprite[j].y-sy);
                }
                else zvel = (100-ps[p].horiz-ps[p].horizoff)*98;
            }
            else if(s->statnum != 3)
            {
                j = findplayer(s,&x);
                l = ldist(&sprite[ps[j].i],s);
                zvel = ( (ps[j].oposz-sz)*512) / l ;
            }
            else zvel = 0;
            if (hittype[i].temp_data[9]) zvel = hittype[i].temp_data[9];
            j = EGS(sect,
                    sx+(sintable[(512+sa+512)&2047]>>12),
                    sy+(sintable[(sa+512)&2047]>>12),
                    sz+(2<<8),SHRINKSPARK,-16,28,28,sa,768,zvel,i,4);

            sprite[j].cstat = 128;
            sprite[j].clipdist = 32;


            return j;
        }
    }
    return -1;
}

void displayloogie(short snum)
{
    long i, a, x, y, z;

    if(ps[snum].loogcnt == 0) return;

    y = (ps[snum].loogcnt<<2);
    for(i=0;i<ps[snum].numloogs;i++)
    {
        a = klabs(sintable[((ps[snum].loogcnt+i)<<5)&2047])>>5;
        z = 4096+((ps[snum].loogcnt+i)<<9);
        x = (-sync[snum].avel)+(sintable[((ps[snum].loogcnt+i)<<6)&2047]>>10);

        rotatesprite(
            (ps[snum].loogiex[i]+x)<<16,(200+ps[snum].loogiey[i]-y)<<16,z-(i<<8),256-a,
            LOOGIE,0,0,2,0,0,xdim-1,ydim-1);
    }
}

char animatefist(short gs,short snum)
{
    short looking_arc,fisti,fistpal;
    long fistzoom, fistz;

    fisti = ps[snum].fist_incs;
    if(fisti > 32) fisti = 32;
    if(fisti <= 0) return 0;

    looking_arc = klabs(ps[snum].look_ang)/9;

    fistzoom = 65536L - (sintable[(512+(fisti<<6))&2047]<<2);
    if(fistzoom > 90612L)
        fistzoom = 90612L;
    if(fistzoom < 40920)
        fistzoom = 40290;
    fistz = 194 + (sintable[((6+fisti)<<7)&2047]>>9);

    if(sprite[ps[snum].i].pal == 1)
        fistpal = 1;
    else
        fistpal = sector[ps[snum].cursectnum].floorpal;

    rotatesprite(
        (-fisti+222+(sync[snum].avel>>4))<<16,
        (looking_arc+fistz)<<16,
        fistzoom,0,FIST,gs,fistpal,2,0,0,xdim-1,ydim-1);

    return 1;
}

char animateknee(short gs,short snum)
{
    short knee_y[] = {0,-8,-16,-32,-64,-84,-108,-108,-108,-72,-32,-8};
    short looking_arc, pal;

    if(ps[snum].knee_incs > 11 || ps[snum].knee_incs == 0 || sprite[ps[snum].i].extra <= 0) return 0;

    looking_arc = knee_y[ps[snum].knee_incs] + klabs(ps[snum].look_ang)/9;

    looking_arc -= (ps[snum].hard_landing<<3);

    if(sprite[ps[snum].i].pal == 1)
        pal = 1;
    else
    {
        pal = sector[ps[snum].cursectnum].floorpal;
        if(pal == 0)
            pal = ps[snum].palookup;
    }

    myospal(105+(sync[snum].avel>>4)-(ps[snum].look_ang>>1)+(knee_y[ps[snum].knee_incs]>>2),looking_arc+280-((ps[snum].horiz-ps[snum].horizoff)>>4),KNEE,gs,4,pal);

    return 1;
}

char animateknuckles(short gs,short snum)
{
    short knuckle_frames[] = {0,1,2,2,3,3,3,2,2,1,0};
    short looking_arc, pal;

    if(ps[snum].knuckle_incs == 0 || sprite[ps[snum].i].extra <= 0) return 0;

    looking_arc = klabs(ps[snum].look_ang)/9;

    looking_arc -= (ps[snum].hard_landing<<3);

    if(sprite[ps[snum].i].pal == 1)
        pal = 1;
    else
        pal = sector[ps[snum].cursectnum].floorpal;

    myospal(160+(sync[snum].avel>>4)-(ps[snum].look_ang>>1),looking_arc+180-((ps[snum].horiz-ps[snum].horizoff)>>4),CRACKKNUCKLES+knuckle_frames[ps[snum].knuckle_incs>>1],gs,4,pal);

    return 1;
}

long lastvisinc;

void DoFire(struct player_struct *p)
{
    int i;

    short snum = sprite[p->i].yvel;

    SetGameVarID(g_iReturnVarID,0,p->i,snum);
    OnEvent(EVENT_DOFIRE, p->i, snum, -1);

    if(GetGameVarID(g_iReturnVarID,p->i,snum) == 0)
    {
        if (p->weapon_pos != 0) return;

        if(aplWeaponWorksLike[p->curr_weapon][snum]!=KNEE_WEAPON)
            p->ammo_amount[p->curr_weapon]--;

        if(aplWeaponFireSound[p->curr_weapon][snum])
        {
            spritesound(aplWeaponFireSound[p->curr_weapon][snum],p->i);
        }

        SetGameVarID(g_iWeaponVarID,p->curr_weapon,p->i,snum);
        SetGameVarID(g_iWorksLikeVarID,aplWeaponWorksLike[p->curr_weapon][snum], p->i, snum);
        shoot(p->i,aplWeaponShoots[p->curr_weapon][snum]);
        for(i=1;i<aplWeaponShotsPerBurst[p->curr_weapon][snum];i++)
        {
            if(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_AMMOPERSHOT)
            {
                if(p->ammo_amount[p->curr_weapon] > 0)
                {
                    p->ammo_amount[p->curr_weapon]--;
                    shoot(p->i,aplWeaponShoots[p->curr_weapon][snum]);
                }
            }
            else
                shoot(p->i,aplWeaponShoots[p->curr_weapon][snum]);
        }

        if(! (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_NOVISIBLE ))
        {
            lastvisinc = totalclock+32;
            p->visibility = 0;
        }

        /*        if( //!(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_CHECKATRELOAD) &&
                    aplWeaponReload[p->curr_weapon][snum] > aplWeaponTotalTime[p->curr_weapon][snum]
                    && p->ammo_amount[p->curr_weapon] > 0
                    && (aplWeaponClip[p->curr_weapon][snum])
                    && ((p->ammo_amount[p->curr_weapon]%(aplWeaponClip[p->curr_weapon][snum]))==0))
                {
                    p->kickback_pic=aplWeaponTotalTime[p->curr_weapon][snum];
                } */
    }
}

void DoSpawn(struct player_struct *p)
{
    int j;

    short snum = sprite[p->i].yvel;

    if(!aplWeaponSpawn[p->curr_weapon][snum])
        return;

    j = spawn(p->i, aplWeaponSpawn[p->curr_weapon][snum]);

    if((aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_SPAWNTYPE3 ) )
    {
        // like chaingun shells
        sprite[j].ang += 1024;
        sprite[j].ang &= 2047;
        sprite[j].xvel += 32;
        sprite[j].z += (3<<8);
    }

    ssp(j,CLIPMASK0);

}

void displaymasks(short snum)
{
    short p;

    if(sprite[ps[snum].i].pal == 1)
        p = 1;
    else
        p = sector[ps[snum].cursectnum].floorpal;

    if(ps[snum].scuba_on)
    {
        if(ud.screen_size > 4)
        {
            rotatesprite(43<<16,(200-8-tilesizy[SCUBAMASK])<<16,65536,0,SCUBAMASK,0,p,2+16,windowx1,windowy1,windowx2,windowy2);
            rotatesprite((320-43)<<16,(200-8-tilesizy[SCUBAMASK])<<16,65536,1024,SCUBAMASK,0,p,2+4+16,windowx1,windowy1,windowx2,windowy2);
        }
        else
        {
            rotatesprite(43<<16,(200-tilesizy[SCUBAMASK])<<16,65536,0,SCUBAMASK,0,p,2+16,windowx1,windowy1,windowx2,windowy2);
            rotatesprite((320-43)<<16,(200-tilesizy[SCUBAMASK])<<16,65536,1024,SCUBAMASK,0,p,2+4+16,windowx1,windowy1,windowx2,windowy2);
        }
    }
}

char animatetip(short gs,short snum)
{
    short p,looking_arc;
    short tip_y[] = {0,-8,-16,-32,-64,-84,-108,-108,-108,-108,-108,-108,-108,-108,-108,-108,-96,-72,-64,-32,-16};

    if(ps[snum].tipincs == 0) return 0;

    looking_arc = klabs(ps[snum].look_ang)/9;
    looking_arc -= (ps[snum].hard_landing<<3);

    if(sprite[ps[snum].i].pal == 1)
        p = 1;
    else
        p = sector[ps[snum].cursectnum].floorpal;

    /*    if(ps[snum].access_spritenum >= 0)
            p = sprite[ps[snum].access_spritenum].pal;
        else
            p = wall[ps[snum].access_wallnum].pal;
      */
    myospal(170+(sync[snum].avel>>4)-(ps[snum].look_ang>>1),
            (tip_y[ps[snum].tipincs]>>1)+looking_arc+240-((ps[snum].horiz-ps[snum].horizoff)>>4),TIP+((26-ps[snum].tipincs)>>4),gs,0,p);

    return 1;
}

char animateaccess(short gs,short snum)
{
    short access_y[] = {0,-8,-16,-32,-64,-84,-108,-108,-108,-108,-108,-108,-108,-108,-108,-108,-96,-72,-64,-32,-16};
    short looking_arc;
    char p;

    if(ps[snum].access_incs == 0 || sprite[ps[snum].i].extra <= 0) return 0;

    looking_arc = access_y[ps[snum].access_incs] + klabs(ps[snum].look_ang)/9;
    looking_arc -= (ps[snum].hard_landing<<3);

    if(ps[snum].access_spritenum >= 0)
        p = sprite[ps[snum].access_spritenum].pal;
    else p = 0;
    //    else
    //        p = wall[ps[snum].access_wallnum].pal;

    if((ps[snum].access_incs-3) > 0 && (ps[snum].access_incs-3)>>3)
        myospal(170+(sync[snum].avel>>4)-(ps[snum].look_ang>>1)+(access_y[ps[snum].access_incs]>>2),looking_arc+266-((ps[snum].horiz-ps[snum].horizoff)>>4),HANDHOLDINGLASER+(ps[snum].access_incs>>3),gs,0,p);
    else
        myospal(170+(sync[snum].avel>>4)-(ps[snum].look_ang>>1)+(access_y[ps[snum].access_incs]>>2),looking_arc+266-((ps[snum].horiz-ps[snum].horizoff)>>4),HANDHOLDINGACCESS,gs,4,p);

    return 1;
}

void myospalw(long x, long y, short tilenum, signed char shade, char orientation, char p)
{
    if(!ud.drawweapon)
        return;
    else if(ud.drawweapon == 1)
        myospal(x,y,tilenum,shade,orientation,p);
    else if(ud.drawweapon == 2)
    {
        switch(g_currentweapon)
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
            rotatesprite(160<<16,(180+(ps[screenpeek].weapon_pos*ps[screenpeek].weapon_pos))<<16,scale(65536,ud.statusbarscale,100),0,g_currentweapon==GROW_WEAPON?GROWSPRITEICON:weapon_sprites[g_currentweapon],0,0,2,windowx1,windowy1,windowx2,windowy2);
            break;
        }
    }
}

short fistsign;

char last_quick_kick[MAXPLAYERS];

void displayweapon(short snum)
{
    long gun_pos, looking_arc, cw;
    long weapon_xoffset, i, j;
    char o,pal;
    signed char gs;
    struct player_struct *p;
    short *kb;

    p = &ps[snum];
    kb = &p->kickback_pic;

    o = 0;

    looking_arc = klabs(p->look_ang)/9;

    gs = sprite[p->i].shade;
    if(gs > 24) gs = 24;

    if(p->newowner >= 0 || ud.camerasprite >= 0 || p->over_shoulder_on > 0 || (sprite[p->i].pal != 1 && sprite[p->i].extra <= 0) || animatefist(gs,snum) || animateknuckles(gs,snum) || animatetip(gs,snum) || animateaccess(gs,snum) )
        return;

    animateknee(gs,snum);

    gun_pos = 80-(p->weapon_pos*p->weapon_pos);

    weapon_xoffset =  (160)-90;
    weapon_xoffset -= (sintable[((p->weapon_sway>>1)+512)&2047]/(1024+512));
    weapon_xoffset -= 58 + p->weapon_ang;
    if( sprite[p->i].xrepeat < 32 )
        gun_pos -= klabs(sintable[(p->weapon_sway<<2)&2047]>>9);
    else gun_pos -= klabs(sintable[(p->weapon_sway>>1)&2047]>>10);

    gun_pos -= (p->hard_landing<<3);

    if(p->last_weapon >= 0)
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

    SetGameVarID(g_iReturnVarID,0,p->i,snum);
    OnEvent(EVENT_DISPLAYWEAPON, p->i, screenpeek, -1);

    if(GetGameVarID(g_iReturnVarID,p->i,snum) == 0)
    {
        j = 14-p->quick_kick;
        if(j != 14 || last_quick_kick[snum])
        {
            if(sprite[p->i].pal == 1)
                pal = 1;
            else
            {
                pal = sector[p->cursectnum].floorpal;
                if(pal == 0)
                    pal = p->palookup;
            }


            if( j < 6 || j > 12 )
                myospal(weapon_xoffset+80-(p->look_ang>>1),
                        looking_arc+250-gun_pos,KNEE,gs,o|4,pal);
            else myospal(weapon_xoffset+160-16-(p->look_ang>>1),
                             looking_arc+214-gun_pos,KNEE+1,gs,o|4,pal);
        }

        if( sprite[p->i].xrepeat < 40 )
        {
            if(p->jetpack_on == 0 )
            {
                i = sprite[p->i].xvel;
                looking_arc += 32-(i>>1);
                fistsign += i>>1;
            }
            cw = weapon_xoffset;
            weapon_xoffset += sintable[(fistsign)&2047]>>10;
            myos(weapon_xoffset+250-(p->look_ang>>1),
                 looking_arc+258-(klabs(sintable[(fistsign)&2047]>>8)),
                 FIST,gs,o);
            weapon_xoffset = cw;
            weapon_xoffset -= sintable[(fistsign)&2047]>>10;
            myos(weapon_xoffset+40-(p->look_ang>>1),
                 looking_arc+200+(klabs(sintable[(fistsign)&2047]>>8)),
                 FIST,gs,o|4);
        }
        else switch(cw)

            {

            case KNEE_WEAPON:

                SetGameVarID(g_iReturnVarID,0,ps[screenpeek].i,screenpeek);
                OnEvent(EVENT_DRAWWEAPON,ps[screenpeek].i,screenpeek, -1);
                if(GetGameVarID(g_iReturnVarID,ps[screenpeek].i,screenpeek) == 0)
                {
                    if( (*kb) > 0 )
                    {
                        if(sprite[p->i].pal == 1)
                            pal = 1;
                        else
                        {
                            pal = sector[p->cursectnum].floorpal;
                            if(pal == 0)
                                pal = p->palookup;
                        }

                        if( (*kb) < 5 || (*kb) > 9 )
                            myospal(weapon_xoffset+220-(p->look_ang>>1),
                                    looking_arc+250-gun_pos,KNEE,gs,o,pal);
                        else
                            myospal(weapon_xoffset+160-(p->look_ang>>1),
                                    looking_arc+214-gun_pos,KNEE+1,gs,o,pal);
                    }
                }
                break;

            case TRIPBOMB_WEAPON:

                SetGameVarID(g_iReturnVarID,0,ps[screenpeek].i,screenpeek);
                OnEvent(EVENT_DRAWWEAPON,ps[screenpeek].i,screenpeek, -1);
                if(GetGameVarID(g_iReturnVarID,ps[screenpeek].i,screenpeek) == 0)
                {
                    if(sprite[p->i].pal == 1)
                        pal = 1;
                    else
                        pal = sector[p->cursectnum].floorpal;

                    weapon_xoffset += 8;
                    gun_pos -= 10;

                    if((*kb) > 6)
                        looking_arc += ((*kb)<<3);
                    else if((*kb) < 4)
                        myospalw(weapon_xoffset+142-(p->look_ang>>1),
                                 looking_arc+234-gun_pos,HANDHOLDINGLASER+3,gs,o,pal);

                    myospalw(weapon_xoffset+130-(p->look_ang>>1),
                             looking_arc+249-gun_pos,
                             HANDHOLDINGLASER+((*kb)>>2),gs,o,pal);
                    myospalw(weapon_xoffset+152-(p->look_ang>>1),
                             looking_arc+249-gun_pos,
                             HANDHOLDINGLASER+((*kb)>>2),gs,o|4,pal);
                }
                break;

            case RPG_WEAPON:

                SetGameVarID(g_iReturnVarID,0,ps[screenpeek].i,screenpeek);
                OnEvent(EVENT_DRAWWEAPON,ps[screenpeek].i,screenpeek, -1);
                if(GetGameVarID(g_iReturnVarID,ps[screenpeek].i,screenpeek) == 0)
                {
                    if(sprite[p->i].pal == 1)
                        pal = 1;
                    else pal = sector[p->cursectnum].floorpal;

                    weapon_xoffset -= sintable[(768+((*kb)<<7))&2047]>>11;
                    gun_pos += sintable[(768+((*kb)<<7))&2047]>>11;

                    if(*kb > 0)
                    {
                        if(*kb < 8)
                        {
                            myospalw(weapon_xoffset+164,(looking_arc<<1)+176-gun_pos,
                                     RPGGUN+((*kb)>>1),gs,o,pal);
                        }
                    }

                    myospalw(weapon_xoffset+164,(looking_arc<<1)+176-gun_pos,
                             RPGGUN,gs,o,pal);
                }
                break;

            case SHOTGUN_WEAPON:

                SetGameVarID(g_iReturnVarID,0,ps[screenpeek].i,screenpeek);
                OnEvent(EVENT_DRAWWEAPON,ps[screenpeek].i,screenpeek, -1);
                if(GetGameVarID(g_iReturnVarID,ps[screenpeek].i,screenpeek) == 0)
                {
                    if(sprite[p->i].pal == 1)
                        pal = 1;
                    else
                        pal = sector[p->cursectnum].floorpal;


                    weapon_xoffset -= 8;

                    switch(*kb)
                    {
                    case 1:
                    case 2:
                        myospalw(weapon_xoffset+168-(p->look_ang>>1),looking_arc+201-gun_pos,
                                 SHOTGUN+2,-128,o,pal);
                    case 0:
                    case 6:
                    case 7:
                    case 8:
                        myospalw(weapon_xoffset+146-(p->look_ang>>1),looking_arc+202-gun_pos,
                                 SHOTGUN,gs,o,pal);
                        break;
                    case 3:
                    case 4:
                    case 5:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                        if( *kb > 1 && *kb < 5 )
                        {
                            gun_pos -= 40;
                            weapon_xoffset += 20;

                            myospalw(weapon_xoffset+178-(p->look_ang>>1),looking_arc+194-gun_pos,
                                     SHOTGUN+1+((*(kb)-1)>>1),-128,o,pal);
                        }

                        myospalw(weapon_xoffset+158-(p->look_ang>>1),looking_arc+220-gun_pos,
                                 SHOTGUN+3,gs,o,pal);

                        break;
                    case 13:
                    case 14:
                    case 15:
                        myospalw(32+weapon_xoffset+166-(p->look_ang>>1),looking_arc+210-gun_pos,
                                 SHOTGUN+4,gs,o,pal);
                        break;
                    case 16:
                    case 17:
                    case 18:
                    case 19:
                        myospalw(64+weapon_xoffset+170-(p->look_ang>>1),looking_arc+196-gun_pos,
                                 SHOTGUN+5,gs,o,pal);
                        break;
                    case 20:
                    case 21:
                    case 22:
                    case 23:
                        myospalw(64+weapon_xoffset+176-(p->look_ang>>1),looking_arc+196-gun_pos,
                                 SHOTGUN+6,gs,o,pal);
                        break;
                    case 24:
                    case 25:
                    case 26:
                    case 27:
                        myospalw(64+weapon_xoffset+170-(p->look_ang>>1),looking_arc+196-gun_pos,
                                 SHOTGUN+5,gs,o,pal);
                        break;
                    case 28:
                    case 29:
                    case 30:
                        myospalw(32+weapon_xoffset+156-(p->look_ang>>1),looking_arc+206-gun_pos,
                                 SHOTGUN+4,gs,o,pal);
                        break;
                    }
                }
                break;


            case CHAINGUN_WEAPON:

                SetGameVarID(g_iReturnVarID,0,ps[screenpeek].i,screenpeek);
                OnEvent(EVENT_DRAWWEAPON,ps[screenpeek].i,screenpeek, -1);
                if(GetGameVarID(g_iReturnVarID,ps[screenpeek].i,screenpeek) == 0)
                {
                    if(sprite[p->i].pal == 1)
                        pal = 1;
                    else
                        pal = sector[p->cursectnum].floorpal;

                    if(*kb > 0)
                        gun_pos -= sintable[(*kb)<<7]>>12;

                    if(*kb > 0 && sprite[p->i].pal != 1) weapon_xoffset += 1-(rand()&3);

                    myospalw(weapon_xoffset+168-(p->look_ang>>1),looking_arc+260-gun_pos,
                             CHAINGUN,gs,o,pal);
                    switch(*kb)
                    {
                    case 0:
                        myospalw(weapon_xoffset+178-(p->look_ang>>1),looking_arc+233-gun_pos,
                                 CHAINGUN+1,gs,o,pal);
                        break;
                    default:
                        if(*kb > *aplWeaponFireDelay[CHAINGUN_WEAPON] && *kb < *aplWeaponTotalTime[CHAINGUN_WEAPON])
                        {
                            i = 0;
                            if(sprite[p->i].pal != 1) i = rand()&7;
                            myospalw(i+weapon_xoffset-4+140-(p->look_ang>>1),i+looking_arc-((*kb)>>1)+208-gun_pos,
                                     CHAINGUN+5+((*kb-4)/5),gs,o,pal);
                            if(sprite[p->i].pal != 1) i = rand()&7;
                            myospalw(i+weapon_xoffset-4+184-(p->look_ang>>1),i+looking_arc-((*kb)>>1)+208-gun_pos,
                                     CHAINGUN+5+((*kb-4)/5),gs,o,pal);
                        }
                        if(*kb < *aplWeaponTotalTime[CHAINGUN_WEAPON]-4)
                        {
                            i = rand()&7;
                            myospalw(i+weapon_xoffset-4+162-(p->look_ang>>1),i+looking_arc-((*kb)>>1)+208-gun_pos,
                                     CHAINGUN+5+((*kb-2)/5),gs,o,pal);
                            myospalw(weapon_xoffset+178-(p->look_ang>>1),looking_arc+233-gun_pos,
                                     CHAINGUN+1+((*kb)>>1),gs,o,pal);
                        }
                        else myospalw(weapon_xoffset+178-(p->look_ang>>1),looking_arc+233-gun_pos,
                                          CHAINGUN+1,gs,o,pal);
                        break;
                    }
                }
                break;

            case PISTOL_WEAPON:

                SetGameVarID(g_iReturnVarID,0,ps[screenpeek].i,screenpeek);
                OnEvent(EVENT_DRAWWEAPON,ps[screenpeek].i,screenpeek, -1);
                if(GetGameVarID(g_iReturnVarID,ps[screenpeek].i,screenpeek) == 0)
                {
                    if(sprite[p->i].pal == 1)
                        pal = 1;
                    else
                        pal = sector[p->cursectnum].floorpal;

                    if( (*kb) < *aplWeaponTotalTime[PISTOL_WEAPON]+1)
                    {
                        short kb_frames[] = {0,1,2},l;

                        l = 195-12+weapon_xoffset;

                        if((*kb) == *aplWeaponFireDelay[PISTOL_WEAPON])
                            l -= 3;

                        myospalw((l-(p->look_ang>>1)),(looking_arc+244-gun_pos),FIRSTGUN+kb_frames[*kb>2?0:*kb],gs,2,pal);
                    }
                    else
                    {

                        if((*kb) < *aplWeaponReload[PISTOL_WEAPON]-17)
                            myospalw(194-(p->look_ang>>1),looking_arc+230-gun_pos,FIRSTGUN+4,gs,o,pal);
                        else if((*kb) < *aplWeaponReload[PISTOL_WEAPON]-12)
                        {
                            myospalw(244-((*kb)<<3)-(p->look_ang>>1),looking_arc+130-gun_pos+((*kb)<<4),FIRSTGUN+6,gs,o,pal);
                            myospalw(224-(p->look_ang>>1),looking_arc+220-gun_pos,FIRSTGUN+5,gs,o,pal);
                        }
                        else if((*kb) < *aplWeaponReload[PISTOL_WEAPON]-7)
                        {
                            myospalw(124+((*kb)<<1)-(p->look_ang>>1),looking_arc+430-gun_pos-((*kb)<<3),FIRSTGUN+6,gs,o,pal);
                            myospalw(224-(p->look_ang>>1),looking_arc+220-gun_pos,FIRSTGUN+5,gs,o,pal);
                        }

                        else if((*kb) < *aplWeaponReload[PISTOL_WEAPON]-4)
                        {
                            myospalw(184-(p->look_ang>>1),looking_arc+235-gun_pos,FIRSTGUN+8,gs,o,pal);
                            myospalw(224-(p->look_ang>>1),looking_arc+210-gun_pos,FIRSTGUN+5,gs,o,pal);
                        }
                        else if((*kb) < *aplWeaponReload[PISTOL_WEAPON]-2)
                        {
                            myospalw(164-(p->look_ang>>1),looking_arc+245-gun_pos,FIRSTGUN+8,gs,o,pal);
                            myospalw(224-(p->look_ang>>1),looking_arc+220-gun_pos,FIRSTGUN+5,gs,o,pal);
                        }
                        else if((*kb) < *aplWeaponReload[PISTOL_WEAPON])
                            myospalw(194-(p->look_ang>>1),looking_arc+235-gun_pos,FIRSTGUN+5,gs,o,pal);

                    }
                }

                break;
            case HANDBOMB_WEAPON:
                {
                    SetGameVarID(g_iReturnVarID,0,ps[screenpeek].i,screenpeek);
                    OnEvent(EVENT_DRAWWEAPON,ps[screenpeek].i,screenpeek, -1);
                    if(GetGameVarID(g_iReturnVarID,ps[screenpeek].i,screenpeek) == 0)
                    {
                        if(sprite[p->i].pal == 1)
                            pal = 1;
                        else
                            pal = sector[p->cursectnum].floorpal;

                        if((*kb))
                        {
                            if((*kb) < (*aplWeaponTotalTime[p->curr_weapon]))
                            {

                                char throw_frames[]
                                = {0,0,0,0,0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2};

                                if((*kb) < 7)
                                    gun_pos -= 10*(*kb);        //D
                                else if((*kb) < 12)
                                    gun_pos += 20*((*kb)-10); //U
                                else if((*kb) < 20)
                                    gun_pos -= 9*((*kb)-14);  //D

                                myospalw(weapon_xoffset+190-(p->look_ang>>1),looking_arc+250-gun_pos,HANDTHROW+throw_frames[(*kb)],gs,o,pal);
                            }
                        }
                        else
                            myospalw(weapon_xoffset+190-(p->look_ang>>1),looking_arc+260-gun_pos,HANDTHROW,gs,o,pal);
                    }
                }
                break;

            case HANDREMOTE_WEAPON:
                {
                    SetGameVarID(g_iReturnVarID,0,ps[screenpeek].i,screenpeek);
                    OnEvent(EVENT_DRAWWEAPON,ps[screenpeek].i,screenpeek, -1);
                    if(GetGameVarID(g_iReturnVarID,ps[screenpeek].i,screenpeek) == 0)
                    {
                        signed char remote_frames[] = {0,1,1,2,1,1,0,0,0,0,0};
                        if(sprite[p->i].pal == 1)
                            pal = 1;
                        else
                            pal = sector[p->cursectnum].floorpal;

                        weapon_xoffset = -48;
                        if((*kb))
                            myospalw(weapon_xoffset+150-(p->look_ang>>1),looking_arc+258-gun_pos,HANDREMOTE+remote_frames[(*kb)],gs,o,pal);
                        else
                            myospalw(weapon_xoffset+150-(p->look_ang>>1),looking_arc+258-gun_pos,HANDREMOTE,gs,o,pal);
                    }
                }
                break;

            case DEVISTATOR_WEAPON:

                SetGameVarID(g_iReturnVarID,0,ps[screenpeek].i,screenpeek);
                OnEvent(EVENT_DRAWWEAPON,ps[screenpeek].i,screenpeek, -1);
                if(GetGameVarID(g_iReturnVarID,ps[screenpeek].i,screenpeek) == 0)
                {
                    if(sprite[p->i].pal == 1)
                        pal = 1;
                    else
                        pal = sector[p->cursectnum].floorpal;

                    if((*kb) < (*aplWeaponTotalTime[DEVISTATOR_WEAPON]+1) && (*kb) > 0)
                    {
                        char cycloidy[] = {0,4,12,24,12,4,0};

                        i = ksgn((*kb)>>2);

                        if(p->hbomb_hold_delay)
                        {
                            myospalw( (cycloidy[*kb]>>1)+weapon_xoffset+268-(p->look_ang>>1),cycloidy[*kb]+looking_arc+238-gun_pos,DEVISTATOR+i,-32,o,pal);
                            myospalw(weapon_xoffset+30-(p->look_ang>>1),looking_arc+240-gun_pos,DEVISTATOR,gs,o|4,pal);
                        }
                        else
                        {
                            myospalw( -(cycloidy[*kb]>>1)+weapon_xoffset+30-(p->look_ang>>1),cycloidy[*kb]+looking_arc+240-gun_pos,DEVISTATOR+i,-32,o|4,pal);
                            myospalw(weapon_xoffset+268-(p->look_ang>>1),looking_arc+238-gun_pos,DEVISTATOR,gs,o,pal);
                        }
                    }
                    else
                    {
                        myospalw(weapon_xoffset+268-(p->look_ang>>1),looking_arc+238-gun_pos,DEVISTATOR,gs,o,pal);
                        myospalw(weapon_xoffset+30-(p->look_ang>>1),looking_arc+240-gun_pos,DEVISTATOR,gs,o|4,pal);
                    }
                }
                break;

            case FREEZE_WEAPON:

                SetGameVarID(g_iReturnVarID,0,ps[screenpeek].i,screenpeek);
                OnEvent(EVENT_DRAWWEAPON,ps[screenpeek].i,screenpeek, -1);
                if(GetGameVarID(g_iReturnVarID,ps[screenpeek].i,screenpeek) == 0)
                {
                    if(sprite[p->i].pal == 1)
                        pal = 1;
                    else
                        pal = sector[p->cursectnum].floorpal;

                    if((*kb) < (aplWeaponTotalTime[p->curr_weapon][snum]+1) && (*kb) > 0)
                    {
                        char cat_frames[] = { 0,0,1,1,2,2 };

                        if(sprite[p->i].pal != 1)
                        {
                            weapon_xoffset += rand()&3;
                            looking_arc += rand()&3;
                        }
                        gun_pos -= 16;
                        myospalw(weapon_xoffset+210-(p->look_ang>>1),looking_arc+261-gun_pos,FREEZE+2,-32,o,pal);
                        myospalw(weapon_xoffset+210-(p->look_ang>>1),looking_arc+235-gun_pos,FREEZE+3+cat_frames[*kb%6],-32,o,pal);
                    }
                    else myospalw(weapon_xoffset+210-(p->look_ang>>1),looking_arc+261-gun_pos,FREEZE,gs,o,pal);
                }
                break;

            case GROW_WEAPON:

                SetGameVarID(g_iReturnVarID,0,ps[screenpeek].i,screenpeek);
                OnEvent(EVENT_DRAWWEAPON,ps[screenpeek].i,screenpeek, -1);
                if(GetGameVarID(g_iReturnVarID,ps[screenpeek].i,screenpeek) == 0)
                {
                    weapon_xoffset += 28;
                    looking_arc += 18;
                    if(sprite[p->i].pal == 1)
                        pal = 1;
                    else
                        pal = sector[p->cursectnum].floorpal;
                    {
                        if((*kb) < aplWeaponTotalTime[p->curr_weapon][snum] && (*kb) > 0)
                        {
                            if(sprite[p->i].pal != 1)
                            {
                                weapon_xoffset += rand()&3;
                                gun_pos += (rand()&3);
                            }

                            myospalw(weapon_xoffset+184-(p->look_ang>>1),
                                     looking_arc+240-gun_pos,SHRINKER+3+((*kb)&3),-32,
                                     o,2);

                            myospalw(weapon_xoffset+188-(p->look_ang>>1),
                                     looking_arc+240-gun_pos,SHRINKER-1,gs,o,pal);
                        }
                        else
                        {
                            myospalw(weapon_xoffset+184-(p->look_ang>>1),
                                     looking_arc+240-gun_pos,SHRINKER+2,
                                     16-(sintable[p->random_club_frame&2047]>>10),
                                     o,2);

                            myospalw(weapon_xoffset+188-(p->look_ang>>1),
                                     looking_arc+240-gun_pos,SHRINKER-2,gs,o,pal);
                        }
                    }
                }
                break;

            case SHRINKER_WEAPON:

                SetGameVarID(g_iReturnVarID,0,ps[screenpeek].i,screenpeek);
                OnEvent(EVENT_DRAWWEAPON,ps[screenpeek].i,screenpeek, -1);
                if(GetGameVarID(g_iReturnVarID,ps[screenpeek].i,screenpeek) == 0)
                {
                    weapon_xoffset += 28;
                    looking_arc += 18;
                    if(sprite[p->i].pal == 1)
                        pal = 1;
                    else
                        pal = sector[p->cursectnum].floorpal;
                    if(((*kb) > 0) && ((*kb) < aplWeaponTotalTime[p->curr_weapon][snum]))
                    {
                        if(sprite[p->i].pal != 1)
                        {
                            weapon_xoffset += rand()&3;
                            gun_pos += (rand()&3);
                        }

                        myospalw(weapon_xoffset+184-(p->look_ang>>1),
                                 looking_arc+240-gun_pos,SHRINKER+3+((*kb)&3),-32,
                                 o,0);

                        myospalw(weapon_xoffset+188-(p->look_ang>>1),
                                 looking_arc+240-gun_pos,SHRINKER+1,gs,o,pal);

                    }
                    else
                    {
                        myospalw(weapon_xoffset+184-(p->look_ang>>1),
                                 looking_arc+240-gun_pos,SHRINKER+2,
                                 16-(sintable[p->random_club_frame&2047]>>10),
                                 o,0);

                        myospalw(weapon_xoffset+188-(p->look_ang>>1),
                                 looking_arc+240-gun_pos,SHRINKER,gs,o,pal);
                    }
                }
                break;

            }
    }
    displayloogie(snum);

}

#define TURBOTURNTIME (TICRATE/8) // 7
#define NORMALTURN   15
#define PREAMBLETURN 5
#define NORMALKEYMOVE 40
#define MAXVEL       ((NORMALKEYMOVE*2)+10)
#define MAXSVEL      ((NORMALKEYMOVE*2)+10)
#define MAXANGVEL    127
#define MAXHORIZ     127

long myaimmode = 0, myaimstat = 0, omyaimstat = 0;

int32 mouseyaxismode = -1;
static ControlInfo lastinfo = { 0,0,0,0,0,0 };

static int jump_input = 0;

void getinput(short snum)
{
    short j, daang;
    ControlInfo info;
    int32 tics;
    boolean running;
    int32 turnamount;
    int32 keymove;
    int32 momx,momy;
    struct player_struct *p;

    momx = momy = 0;
    p = &ps[snum];

    if( (p->gm&MODE_MENU) || (p->gm&MODE_TYPE) || (ud.pause_on && !KB_KeyPressed(sc_Pause)) )
    {
        CONTROL_GetInput( &info );
        memset(&lastinfo, 0, sizeof(lastinfo));
        loc.fvel = vel = 0;
        loc.svel = svel = 0;
        loc.avel = angvel = 0;
        loc.horz = horiz = 0;
        loc.bits = (((long)gamequit)<<26);
        return;
    }

    if (ud.mouseaiming)
        myaimmode = BUTTON(gamefunc_Mouse_Aiming);
    else
    {
        omyaimstat = myaimstat; myaimstat = BUTTON(gamefunc_Mouse_Aiming);
        if (myaimstat > omyaimstat)
        {
            myaimmode ^= 1;
            FTA(44+myaimmode,p);
        }
    }

    {
        int32 i;
        if (myaimmode) i = analog_lookingupanddown;
        else i = MouseAnalogueAxes[1];

        if (i != mouseyaxismode) {
            CONTROL_MapAnalogAxis(1, i, controldevice_mouse);
            mouseyaxismode = i;
        }
    }

    CONTROL_GetInput( &info );

    if(MouseFilter)
    {
        if(info.dpitch > 0)
        {
            info.dpitch -= MouseFilter;
            if(info.dpitch < 0) info.dpitch = 0;
        }
        else if(info.dpitch < 0)
        {
            info.dpitch += MouseFilter;
            if(info.dpitch > 0) info.dpitch = 0;
        }
        if(info.dyaw > 0)
        {
            info.dyaw -= MouseFilter;
            if(info.dyaw < 0) info.dyaw = 0;
        }
        else if(info.dyaw < 0)
        {
            info.dyaw += MouseFilter;
            if(info.dyaw > 0) info.dyaw = 0;
        }
    }

    tics = totalclock-lastcontroltime;
    lastcontroltime = totalclock;

    if(multiflag == 1)
    {
        loc.bits =   1<<17;
        loc.bits |=   multiwhat<<18;
        loc.bits |=   multipos<<19;
        multiflag = 0;
        return;
    }

    if(BUTTON(gamefunc_Jump))
        jump_input = 2;

    loc.bits =   (jump_input > 0); //BUTTON(gamefunc_Jump);
    loc.bits |=   BUTTON(gamefunc_Crouch)<<1;
    loc.bits |=   BUTTON(gamefunc_Fire)<<2;
    loc.bits |=   BUTTON(gamefunc_Aim_Up)<<3;
    loc.bits |=   BUTTON(gamefunc_Aim_Down)<<4;
    if (ud.runkey_mode) loc.bits |=   (ud.auto_run | BUTTON(gamefunc_Run))<<5;
    else loc.bits |=   (BUTTON(gamefunc_Run) ^ ud.auto_run)<<5;
    loc.bits |=   BUTTON(gamefunc_Look_Left)<<6;
    loc.bits |=   BUTTON(gamefunc_Look_Right)<<7;

    if ( aplWeaponFlags[ps[snum].curr_weapon][snum] & WEAPON_FLAG_SEMIAUTO && BUTTON( gamefunc_Fire ) )
        CONTROL_ClearButton(gamefunc_Fire);

    if(jump_input > 0)
        jump_input--;

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
    if (BUTTON(gamefunc_Previous_Weapon))
        j = 11;
    if (BUTTON(gamefunc_Next_Weapon))
        j = 12;

    loc.bits |=   j<<8;
    loc.bits |=   BUTTON(gamefunc_Steroids)<<12;
    loc.bits |=   BUTTON(gamefunc_Look_Up)<<13;
    loc.bits |=   BUTTON(gamefunc_Look_Down)<<14;
    loc.bits |=   BUTTON(gamefunc_NightVision)<<15;
    loc.bits |=   BUTTON(gamefunc_MedKit)<<16;
    loc.bits |=   BUTTON(gamefunc_Center_View)<<18;
    loc.bits |=   BUTTON(gamefunc_Holster_Weapon)<<19;
    loc.bits |=   BUTTON(gamefunc_Inventory_Left)<<20;
    loc.bits |=   KB_KeyPressed(sc_Pause)<<21;
    loc.bits |=   BUTTON(gamefunc_Quick_Kick)<<22;
    loc.bits |=   myaimmode<<23;
    loc.bits |=   BUTTON(gamefunc_Holo_Duke)<<24;
    loc.bits |=   BUTTON(gamefunc_Jetpack)<<25;
    loc.bits |=   (((long)gamequit)<<26);
    loc.bits |=   BUTTON(gamefunc_Inventory_Right)<<27;
    loc.bits |=   BUTTON(gamefunc_TurnAround)<<28;
    loc.bits |=   BUTTON(gamefunc_Open)<<29;
    loc.bits |=   BUTTON(gamefunc_Inventory)<<30;
    loc.bits |=   KB_KeyPressed(sc_Escape)<<31;

    //    running = BUTTON(gamefunc_Run)|ud.auto_run;
    // JBF: Run key behaviour is selectable
    if (ud.runkey_mode)
        running = BUTTON(gamefunc_Run)|ud.auto_run; // classic
    else
        running = ud.auto_run^BUTTON(gamefunc_Run); // modern

    svel = vel = angvel = horiz = 0;

    if(SmoothInput)
    {
        if( BUTTON(gamefunc_Strafe) ) {
            svel = -(info.dyaw+lastinfo.dyaw)/8;
            lastinfo.dyaw = (lastinfo.dyaw+info.dyaw) % 8;
        } else {
            angvel = (info.dyaw+lastinfo.dyaw)/64;
            lastinfo.dyaw = (lastinfo.dyaw+info.dyaw) % 64;
        }

        if(ud.mouseflip)
            horiz = -(info.dpitch+lastinfo.dpitch)/(314-128);
        else horiz = (info.dpitch+lastinfo.dpitch)/(314-128);

        lastinfo.dpitch = (lastinfo.dpitch+info.dpitch) % (314-128);
    }
    else
    {
        if( BUTTON(gamefunc_Strafe) ) {
            svel = -info.dyaw/8;
        } else {
            angvel = info.dyaw/64;
        }

        if(ud.mouseflip)
            horiz -= info.dpitch/(314-128);
        else horiz += info.dpitch/(314-128);
    }

    svel -= info.dx;
    lastinfo.dz = info.dz % (1<<6);
    vel = -info.dz>>6;

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
        if ( BUTTON(gamefunc_Turn_Left) && !(ps[snum].movement_lock&4))
            svel -= -keymove;
        if ( BUTTON(gamefunc_Turn_Right) && !(ps[snum].movement_lock&8))
            svel -= keymove;
    }
    else
    {
        if ( BUTTON(gamefunc_Turn_Left))
        {
            turnheldtime += tics;
            if (turnheldtime>=TURBOTURNTIME)
                angvel -= turnamount;
            else
                angvel -= PREAMBLETURN;
        }
        else if ( BUTTON(gamefunc_Turn_Right))
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

    if ( BUTTON(gamefunc_Strafe_Left) && !(ps[snum].movement_lock&4))
        svel += keymove;
    if ( BUTTON(gamefunc_Strafe_Right) && !(ps[snum].movement_lock&8))
        svel += -keymove;
    if ( BUTTON(gamefunc_Move_Forward) && !(ps[snum].movement_lock&1))
        vel += keymove;
    if ( BUTTON(gamefunc_Move_Backward) && !(ps[snum].movement_lock&2))
        vel += -keymove;

    if(vel < -MAXVEL) vel = -MAXVEL;
    if(vel > MAXVEL) vel = MAXVEL;
    if(svel < -MAXSVEL) svel = -MAXSVEL;
    if(svel > MAXSVEL) svel = MAXSVEL;
    if(angvel < -MAXANGVEL) angvel = -MAXANGVEL;
    if(angvel > MAXANGVEL) angvel = MAXANGVEL;
    if(horiz < -MAXHORIZ) horiz = -MAXHORIZ;
    if(horiz > MAXHORIZ) horiz = MAXHORIZ;

    loc.bits2 = BUTTON(gamefunc_Move_Forward);
    loc.bits2 |= BUTTON(gamefunc_Move_Backward)<<1;
    loc.bits2 |= BUTTON(gamefunc_Strafe_Left)<<2;
    loc.bits2 |= BUTTON(gamefunc_Strafe_Right)<<3;
    loc.bits2 |= BUTTON(gamefunc_Turn_Left)<<4;
    loc.bits2 |= BUTTON(gamefunc_Turn_Right)<<5;

    if(ud.scrollmode && ud.overhead_on)
    {
        ud.folfvel = vel;
        ud.folavel = angvel;
        loc.fvel = 0;
        loc.svel = 0;
        loc.avel = 0;
        loc.horz = 0;
        return;
    }

    if( numplayers > 1 )
        daang = myang;
    else daang = p->ang;

    momx = mulscale9(vel,sintable[(daang+2560)&2047]);
    momy = mulscale9(vel,sintable[(daang+2048)&2047]);

    momx += mulscale9(svel,sintable[(daang+2048)&2047]);
    momy += mulscale9(svel,sintable[(daang+1536)&2047]);

    momx += fricxv;
    momy += fricyv;

    if (momx == 0) momx = 1; // HACK; the game seems to "forget" about the rest of the data if we aren't moving
    if (momy == 0) momy = 1;

    loc.fvel = momx;
    loc.svel = momy;

    loc.avel = angvel;
    loc.horz = horiz;
}

char doincrements(struct player_struct *p)
{
    short snum;

    snum = sprite[p->i].yvel;
    //    j = sync[snum].avel;
    //    p->weapon_ang = -(j/5);

    if(p->invdisptime > 0)
        p->invdisptime--;

    if(p->tipincs > 0) p->tipincs--;

    if(p->last_pissed_time > 0 )
    {
        p->last_pissed_time--;

        if( p->last_pissed_time == (26*219) )
        {
            spritesound(FLUSH_TOILET,p->i);
            if(snum == screenpeek || (gametype_flags[ud.coop] & GAMETYPE_FLAG_COOPSOUND))
                spritesound(DUKE_PISSRELIEF,p->i);
        }

        if( p->last_pissed_time == (26*218) )
        {
            p->holster_weapon = 0;
            p->weapon_pos = 10;
        }
    }

    if(p->crack_time > 0)
    {
        p->crack_time--;
        if(p->crack_time == 0)
        {
            p->knuckle_incs = 1;
            p->crack_time = 777;
        }
    }

    if( p->steroids_amount > 0 && p->steroids_amount < 400)
    {
        p->steroids_amount--;
        if(p->steroids_amount == 0)
            checkavailinven(p);
        if( !(p->steroids_amount&7) )
            if(snum == screenpeek || (gametype_flags[ud.coop] & GAMETYPE_FLAG_COOPSOUND))
                spritesound(DUKE_HARTBEAT,p->i);
    }

    if(p->heat_on && p->heat_amount > 0)
    {
        p->heat_amount--;
        if( p->heat_amount == 0 )
        {
            p->heat_on = 0;
            checkavailinven(p);
            spritesound(NITEVISION_ONOFF,p->i);
            setpal(p);
        }
    }

    if( p->holoduke_on >= 0 )
    {
        p->holoduke_amount--;
        if(p->holoduke_amount <= 0)
        {
            spritesound(TELEPORTER,p->i);
            p->holoduke_on = -1;
            checkavailinven(p);
        }
    }

    if( p->jetpack_on && p->jetpack_amount > 0 )
    {
        p->jetpack_amount--;
        if(p->jetpack_amount <= 0)
        {
            p->jetpack_on = 0;
            checkavailinven(p);
            spritesound(DUKE_JETPACK_OFF,p->i);
            stopspritesound(DUKE_JETPACK_IDLE,p->i);
            stopspritesound(DUKE_JETPACK_ON,p->i);
        }
    }

    if(p->quick_kick > 0 && sprite[p->i].pal != 1)
    {
        last_quick_kick[snum] = p->quick_kick+1;
        p->quick_kick--;
        if( p->quick_kick == 8 )
            shoot(p->i,KNEE);
    } else if(last_quick_kick[snum] > 0) last_quick_kick[snum]--;

    if(p->access_incs && sprite[p->i].pal != 1)
    {
        p->access_incs++;
        if(sprite[p->i].extra <= 0)
            p->access_incs = 12;
        if(p->access_incs == 12)
        {
            if(p->access_spritenum >= 0)
            {
                checkhitswitch(snum,p->access_spritenum,1);
                switch(sprite[p->access_spritenum].pal)
                {
                case 0:p->got_access &= (0xffff-0x1);break;
                case 21:p->got_access &= (0xffff-0x2);break;
                case 23:p->got_access &= (0xffff-0x4);break;
                }
                p->access_spritenum = -1;
            }
            else
            {
                checkhitswitch(snum,p->access_wallnum,0);
                switch(wall[p->access_wallnum].pal)
                {
                case 0:p->got_access &= (0xffff-0x1);break;
                case 21:p->got_access &= (0xffff-0x2);break;
                case 23:p->got_access &= (0xffff-0x4);break;
                }
            }
        }

        if(p->access_incs > 20)
        {
            p->access_incs = 0;
            p->weapon_pos = 10;
            p->kickback_pic = 0;
        }
    }

    if(p->scuba_on == 0 && sector[p->cursectnum].lotag == 2)
    {
        if(p->scuba_amount > 0)
        {
            p->scuba_on = 1;
            p->inven_icon = 6;
            FTA(76,p);
        }
        else
        {
            if(p->airleft > 0)
                p->airleft--;
            else
            {
                p->extra_extra8 += 32;
                if(p->last_extra < (max_player_health>>1) && (p->last_extra&3) == 0)
                    spritesound(DUKE_LONGTERM_PAIN,p->i);
            }
        }
    }
    else if(p->scuba_amount > 0 && p->scuba_on)
    {
        p->scuba_amount--;
        if(p->scuba_amount == 0)
        {
            p->scuba_on = 0;
            checkavailinven(p);
        }
    }

    if(p->knuckle_incs)
    {
        p->knuckle_incs ++;
        if(p->knuckle_incs==10)
        {
            if(totalclock > 1024)
                if(snum == screenpeek || (gametype_flags[ud.coop] & GAMETYPE_FLAG_COOPSOUND))
                {

                    if(rand()&1)
                        spritesound(DUKE_CRACK,p->i);
                    else spritesound(DUKE_CRACK2,p->i);

                }

            spritesound(DUKE_CRACK_FIRST,p->i);

        }
        else if( p->knuckle_incs == 22 || (sync[snum].bits&(1<<2)))
            p->knuckle_incs=0;

        return 1;
    }
    return 0;
}

short weapon_sprites[MAX_WEAPONS] = { KNEE__STATIC, FIRSTGUNSPRITE__STATIC, SHOTGUNSPRITE__STATIC,
                                      CHAINGUNSPRITE__STATIC, RPGSPRITE__STATIC, HEAVYHBOMB__STATIC, SHRINKERSPRITE__STATIC, DEVISTATORSPRITE__STATIC,
                                      TRIPBOMBSPRITE__STATIC, FREEZESPRITE__STATIC, HEAVYHBOMB__STATIC, SHRINKERSPRITE__STATIC};

void checkweapons(struct player_struct *p)
{
    short cw;
    short snum = sprite[p->i].yvel;

    cw = aplWeaponWorksLike[p->curr_weapon][snum];

    if(cw < 1 || cw >= MAX_WEAPONS) return;

    if(cw)
    {
        if(TRAND&1)
            spawn(p->i,weapon_sprites[cw]);
        else switch(cw)
            {
            case RPG_WEAPON:
            case HANDBOMB_WEAPON:
                spawn(p->i,EXPLOSION2);
                break;
            }
    }
}

void processinput(short snum)
{
    long j, i, k, doubvel, fz, cz, hz, lz, truefdist, x, y;
    char shrunk;
    unsigned long sb_snum;
    short psect, psectlotag,*kb, tempsect, pi;
    struct player_struct *p;
    spritetype *s;

    p = &ps[snum];
    pi = p->i;
    s = &sprite[pi];

    kb = &p->kickback_pic;

    p->player_par++;

    OnEvent(EVENT_PROCESSINPUT, pi, snum, -1);

    if(p->cheat_phase <= 0) sb_snum = sync[snum].bits;
    else sb_snum = 0;

    if((sb_snum&(1<<2)))
    {
        SetGameVarID(g_iReturnVarID,0,pi,snum);
        OnEvent(EVENT_PRESSEDFIRE, pi, snum, -1);
        if(GetGameVarID(g_iReturnVarID,pi,snum) != 0)
            sb_snum &= ~(1<<2);
    }

    psect = p->cursectnum;
    if(psect == -1)
    {
        if(s->extra > 0 && ud.clipping == 0)
        {
            quickkill(p);
            spritesound(SQUISHED,pi);
        }
        psect = 0;
    }

    psectlotag = sector[psect].lotag;
    p->spritebridge = 0;
    p->sbs = 0;

    shrunk = (s->yrepeat < 32);
    getzrange(p->posx,p->posy,p->posz,psect,&cz,&hz,&fz,&lz,163L,CLIPMASK0);

    j = getflorzofslope(psect,p->posx,p->posy);

    p->truefz = j;
    p->truecz = getceilzofslope(psect,p->posx,p->posy);

    truefdist = klabs(p->posz-j);
    if( (lz&49152) == 16384 && psectlotag == 1 && truefdist > PHEIGHT+(16<<8) )
        psectlotag = 0;

    hittype[pi].floorz = fz;
    hittype[pi].ceilingz = cz;

    p->ohoriz = p->horiz;
    p->ohorizoff = p->horizoff;

    if( p->aim_mode == 0 && p->on_ground && psectlotag != 2 && (sector[psect].floorstat&2) )
    {
        x = p->posx+(sintable[(p->ang+512)&2047]>>5);
        y = p->posy+(sintable[p->ang&2047]>>5);
        tempsect = psect;
        updatesector(x,y,&tempsect);
        if (tempsect >= 0)
        {
            k = getflorzofslope(psect,x,y);
            if (psect == tempsect)
                p->horizoff += mulscale16(j-k,160);
            else if (klabs(getflorzofslope(tempsect,x,y)-k) <= (4<<8))
                p->horizoff += mulscale16(j-k,160);
        }
    }

    if (p->horizoff > 0) p->horizoff -= ((p->horizoff>>3)+1);
    else if (p->horizoff < 0) p->horizoff += (((-p->horizoff)>>3)+1);

    if( hz >= 0 && (hz&49152) == 49152)
    {
        hz &= (MAXSPRITES-1);

        if(sprite[hz].statnum == 1 && sprite[hz].extra >= 0)
        {
            hz = 0;
            cz = p->truecz;
        }
    }

    if(lz >= 0 && (lz&49152) == 49152)
    {
        j = lz&(MAXSPRITES-1);

        if( (sprite[j].cstat&33) == 33 )
        {
            psectlotag = 0;
            p->footprintcount = 0;
            p->spritebridge = 1;
            p->sbs = j;
        }
        else if(badguy(&sprite[j]) && sprite[j].xrepeat > 24 && klabs(s->z-sprite[j].z) < (84<<8) )
        {
            j = getangle(sprite[j].x-p->posx,sprite[j].y-p->posy);
            p->posxv -= sintable[(j+512)&2047]<<4;
            p->posyv -= sintable[j&2047]<<4;
        }
    }


    if ( s->extra > 0 ) incur_damage( p );
    else
    {
        s->extra = 0;
        p->shield_amount = 0;
    }

    p->last_extra = s->extra;

    if(p->loogcnt > 0) p->loogcnt--;
    else p->loogcnt = 0;

    if(p->fist_incs)
    {
        // the fist puching the end-of-level thing...
        p->fist_incs++;
        if(p->fist_incs == 28)
        {
            if(ud.recstat == 1) closedemowrite();
            sound(PIPEBOMB_EXPLODE);
            p->pals[0] = 64;
            p->pals[1] = 64;
            p->pals[2] = 64;
            p->pals_time = 48;
        }
        if(p->fist_incs > 42)
        {
            if(p->buttonpalette && ud.from_bonus == 0)
            {
                ud.from_bonus = ud.level_number+1;
                if(ud.secretlevel > 0 && ud.secretlevel < 12) ud.level_number = ud.secretlevel-1;
                ud.m_level_number = ud.level_number;
            }
            else
            {
                if(ud.from_bonus)
                {
                    ud.level_number = ud.from_bonus;
                    ud.m_level_number = ud.level_number;
                    ud.from_bonus = 0;
                }
                else
                {
                    if(ud.level_number == ud.secretlevel && ud.from_bonus > 0 )
                        ud.level_number = ud.from_bonus;
                    else ud.level_number++;

                    if(ud.level_number > 10) ud.level_number = 0;
                    ud.m_level_number = ud.level_number;

                }
            }
            for(i=connecthead;i>=0;i=connectpoint2[i])
                ps[i].gm = MODE_EOL;
            p->fist_incs = 0;

            return;
        }
    }

    if(p->timebeforeexit > 1 && p->last_extra > 0)
    {
        p->timebeforeexit--;
        if(p->timebeforeexit == 26*5)
        {
            FX_StopAllSounds();
            clearsoundlocks();
            if(p->customexitsound >= 0)
            {
                sound(p->customexitsound);
                FTA(102,p);
            }
        }
        else if(p->timebeforeexit == 1)
        {
            for(i=connecthead;i>=0;i=connectpoint2[i])
                ps[i].gm = MODE_EOL;
            if(ud.from_bonus)
            {
                ud.level_number = ud.from_bonus;
                ud.m_level_number = ud.level_number;
                ud.from_bonus = 0;
            }
            else
            {
                ud.level_number++;
                ud.m_level_number = ud.level_number;
            }
            return;
        }
    }
    /*
        if(p->select_dir)
        {
            if(psectlotag != 15 || (sb_snum&(1<<31)) )
                p->select_dir = 0;
            else
            {
                if(sync[snum].fvel > 127)
                {
                    p->select_dir = 0;
                    activatewarpelevators(pi,-1);
                }
                else if(sync[snum].fvel <= -127)
                {
                    p->select_dir = 0;
                    activatewarpelevators(pi,1);
                }
                return;
            }
        }
      */

    if(p->pals_time >= 0)
        p->pals_time--;

    if(p->fta > 0)
    {
        p->fta--;
        if(p->fta == 0)
        {
            pub = NUMPAGES;
            pus = NUMPAGES;
            p->ftq = 0;
        }
    }

    if( s->extra <= 0 )
    {
        if(p->dead_flag == 0)
        {
            if(s->pal != 1)
            {
                p->pals[0] = 63;
                p->pals[1] = 0;
                p->pals[2] = 0;
                p->pals_time = 63;
                p->posz -= (16<<8);
                s->z -= (16<<8);
            }

            if(ud.recstat == 1 && ud.multimode < 2)
                closedemowrite();

            if(s->pal != 1)
            {
                p->dead_flag = (512-((TRAND&1)<<10)+(TRAND&255)-512)&2047;
                if(p->dead_flag == 0)
                    p->dead_flag++;
            }

            p->jetpack_on = 0;
            p->holoduke_on = -1;

            stopspritesound(DUKE_JETPACK_IDLE,p->i);
            if(p->scream_voice > FX_Ok)
            {
                FX_StopSound(p->scream_voice);
                testcallback(DUKE_SCREAM);
                p->scream_voice = FX_Ok;
            }

            if( s->pal != 1 && (s->cstat&32768) == 0) s->cstat = 0;

            if( ud.multimode > 1 && ( s->pal != 1 || (s->cstat&32768) ) )
            {
                if(p->frag_ps != snum)
                {
                    ps[p->frag_ps].frag++;
                    frags[p->frag_ps][snum]++;

                    if(snum == screenpeek)
                    {
                        Bsprintf(fta_quotes[115],"KILLED BY %s",&ud.user_name[p->frag_ps][0]);
                        FTA(115,p);
                    }
                    else
                    {
                        Bsprintf(fta_quotes[116],"KILLED %s",&ud.user_name[snum][0]);
                        FTA(116,&ps[p->frag_ps]);
                    }

                    {
                        char name1[32],name2[32];

                        if(gametype_flags[ud.coop] & GAMETYPE_FLAG_PLAYERSFRIENDLY)
                            i = 9;
                        else
                        {
                            switch(dynamictostatic[hittype[p->i].picnum])
                            {
                            case KNEE__STATIC:
                                if(hittype[p->i].temp_data[1] == 1)
                                    i = 7;
                                else i = 0;
                                break;
                            case SHOTSPARK1__STATIC:
                                switch(ps[p->frag_ps].curr_weapon)
                                {
                                default:
                                case PISTOL_WEAPON:
                                    i = 1;
                                    break;
                                case SHOTGUN_WEAPON:
                                    i = 2;
                                    break;
                                case CHAINGUN_WEAPON:
                                    i = 3;
                                    break;
                                }
                                break;
                            case RPG__STATIC: i = 4; break;
                            case RADIUSEXPLOSION__STATIC: i = 5; break;
                            case SHRINKSPARK__STATIC: i = 6; break;
                            case GROWSPARK__STATIC: i = 8; break;
                            default: i = 0; break;
                            }
                        }
                        Bstrcpy(name1,strip_color_codes(&ud.user_name[snum][0]));
                        Bstrcpy(name2,strip_color_codes(&ud.user_name[p->frag_ps][0]));

                        Bsprintf(tempbuf,fta_quotes[16300+i+(mulscale(krand(), 3, 16)*10)],name1,name2);
                        if(ScreenWidth >= 800)
                            adduserquote(tempbuf);
                        else OSD_Printf("%s\n",strip_color_codes(tempbuf));
                    }
                }
                else
                {
                    p->fraggedself++;

                    if(badguypic(sprite[p->wackedbyactor].picnum))
                        i = 2;
                    else if(hittype[p->i].picnum == NUKEBUTTON)
                        i = 1;
                    else i = 0;

                    Bsprintf(tempbuf,fta_quotes[16350+i],strip_color_codes(&ud.user_name[snum][0]));
                    if(ScreenWidth >= 800)
                        adduserquote(tempbuf);
                    else OSD_Printf("%s\n",strip_color_codes(tempbuf));
                }

                if(myconnectindex == connecthead)
                {
                    Bsprintf(tempbuf,"frag %d killed %d\n",p->frag_ps+1,snum+1);
                    sendscore(tempbuf);
                    //                    printf(tempbuf);
                }

                p->frag_ps = snum;
                pus = NUMPAGES;
            }
        }

        if( psectlotag == 2 )
        {
            if(p->on_warping_sector == 0)
            {
                if( klabs(p->posz-fz) > (PHEIGHT>>1))
                    p->posz += 348;
            }
            else
            {
                s->z -= 512;
                s->zvel = -348;
            }

            clipmove(&p->posx,&p->posy,
                     &p->posz,&p->cursectnum,
                     0,0,164L,(4L<<8),(4L<<8),CLIPMASK0);
            //                        p->bobcounter += 32;
        }

        p->oposx = p->posx;
        p->oposy = p->posy;
        p->oposz = p->posz;
        p->oang = p->ang;
        p->opyoff = p->pyoff;

        p->horiz = 100;
        p->horizoff = 0;

        updatesector(p->posx,p->posy,&p->cursectnum);

        pushmove(&p->posx,&p->posy,&p->posz,&p->cursectnum,128L,(4L<<8),(20L<<8),CLIPMASK0);

        if( fz > cz+(16<<8) && s->pal != 1)
            p->rotscrnang = (p->dead_flag + ( (fz+p->posz)>>7))&2047;

        p->on_warping_sector = 0;

        return;
    }

    if(p->transporter_hold > 0)
    {
        p->transporter_hold--;
        if(p->transporter_hold == 0 && p->on_warping_sector)
            p->transporter_hold = 2;
    }
    if(p->transporter_hold < 0)
        p->transporter_hold++;

    if(p->newowner >= 0)
    {
        i = p->newowner;
        p->posx = SX;
        p->posy = SY;
        p->posz = SZ;
        p->ang =  SA;
        p->posxv = p->posyv = s->xvel = 0;
        p->look_ang = 0;
        p->rotscrnang = 0;

        doincrements(p);

        if(*aplWeaponWorksLike[p->curr_weapon] == HANDREMOTE_WEAPON)
            goto SHOOTINCODE;

        return;
    }

    doubvel = TICSPERFRAME;

    if (p->rotscrnang > 0) p->rotscrnang -= ((p->rotscrnang>>1)+1);
    else if (p->rotscrnang < 0) p->rotscrnang += (((-p->rotscrnang)>>1)+1);

    p->look_ang -= (p->look_ang>>2);

    if( sb_snum&(1<<6) )
    {
        // look_left
        SetGameVarID(g_iReturnVarID,0,pi,snum);
        OnEvent(EVENT_LOOKLEFT,pi,snum, -1);
        if(GetGameVarID(g_iReturnVarID,pi,snum) == 0)
        {
            p->look_ang -= 152;
            p->rotscrnang += 24;
        }
    }

    if( sb_snum&(1<<7) )
    {
        // look_right
        SetGameVarID(g_iReturnVarID,0,pi,snum);
        OnEvent(EVENT_LOOKRIGHT,pi,snum, -1);
        if(GetGameVarID(g_iReturnVarID,pi,snum) == 0)
        {
            p->look_ang += 152;
            p->rotscrnang -= 24;
        }
    }

    if(p->on_crane >= 0)
        goto HORIZONLY;

    j = ksgn(sync[snum].avel);
    /*
    if( j && ud.screen_tilting == 2)
    {
        k = 4;
        if(sb_snum&(1<<5)) k <<= 2;
        p->rotscrnang -= k*j;
        p->look_ang += k*j;
    }
    */

    if( s->xvel < 32 || p->on_ground == 0 || p->bobcounter == 1024 )
    {
        if( (p->weapon_sway&2047) > (1024+96) )
            p->weapon_sway -= 96;
        else if( (p->weapon_sway&2047) < (1024-96) )
            p->weapon_sway += 96;
        else p->weapon_sway = 1024;
    }
    else p->weapon_sway = p->bobcounter;

    s->xvel =
        ksqrt( (p->posx-p->bobposx)*(p->posx-p->bobposx)+(p->posy-p->bobposy)*(p->posy-p->bobposy));
    if(p->on_ground) p->bobcounter += sprite[p->i].xvel>>1;

    if( ud.clipping == 0 && ( sector[p->cursectnum].floorpicnum == MIRROR || p->cursectnum < 0 || p->cursectnum >= MAXSECTORS) )
    {
        p->posx = p->oposx;
        p->posy = p->oposy;
    }
    else
    {
        p->oposx = p->posx;
        p->oposy = p->posy;
    }

    p->bobposx = p->posx;
    p->bobposy = p->posy;

    p->oposz = p->posz;
    p->opyoff = p->pyoff;
    p->oang = p->ang;

    if(p->one_eighty_count < 0)
    {
        p->one_eighty_count += 128;
        p->ang += 128;
    }

    // Shrinking code

    i = 40;

    if( psectlotag == 2)
    {
        // under water
        p->jumping_counter = 0;

        p->pycount += 32;
        p->pycount &= 2047;
        p->pyoff = sintable[p->pycount]>>7;

        if(!isspritemakingsound(pi,DUKE_UNDERWATER))
            spritesound(DUKE_UNDERWATER,pi);

        if ( sb_snum&1 )
        {
            SetGameVarID(g_iReturnVarID,0,pi,snum);
            OnEvent(EVENT_SWIMUP,pi,snum, -1);
            if(GetGameVarID(g_iReturnVarID,pi,snum) == 0)
            {
                // jump
                if(p->poszv > 0) p->poszv = 0;
                p->poszv -= 348;
                if(p->poszv < -(256*6)) p->poszv = -(256*6);
            }
        }
        else if (sb_snum&(1<<1))
        {
            SetGameVarID(g_iReturnVarID,0,pi,snum);
            OnEvent(EVENT_SWIMDOWN,pi,snum, -1);
            if(GetGameVarID(g_iReturnVarID,pi,snum) == 0)
            {
                // crouch
                if(p->poszv < 0) p->poszv = 0;
                p->poszv += 348;
                if(p->poszv > (256*6)) p->poszv = (256*6);
            }
        }
        else
        {
            // normal view
            if(p->poszv < 0)
            {
                p->poszv += 256;
                if(p->poszv > 0)
                    p->poszv = 0;
            }
            if(p->poszv > 0)
            {
                p->poszv -= 256;
                if(p->poszv < 0)
                    p->poszv = 0;
            }
        }

        if(p->poszv > 2048)
            p->poszv >>= 1;

        p->posz += p->poszv;

        if(p->posz > (fz-(15<<8)) )
            p->posz += ((fz-(15<<8))-p->posz)>>1;

        if(p->posz < (cz+(4<<8)) )
        {
            p->posz = cz+(4<<8);
            p->poszv = 0;
        }

        if( p->scuba_on && (TRAND&255) < 8 )
        {
            j = spawn(pi,WATERBUBBLE);
            sprite[j].x +=
                sintable[(p->ang+512+64-(global_random&128))&2047]>>6;
            sprite[j].y +=
                sintable[(p->ang+64-(global_random&128))&2047]>>6;
            sprite[j].xrepeat = 3;
            sprite[j].yrepeat = 2;
            sprite[j].z = p->posz+(8<<8);
        }
    }

    else if(p->jetpack_on)
    {
        p->on_ground = 0;
        p->jumping_counter = 0;
        p->hard_landing = 0;
        p->falling_counter = 0;

        p->pycount += 32;
        p->pycount &= 2047;
        p->pyoff = sintable[p->pycount]>>7;

        if(p->jetpack_on < 11)
        {
            p->jetpack_on++;
            p->posz -= (p->jetpack_on<<7); //Goin up
        }
        else if(p->jetpack_on == 11 && !isspritemakingsound(pi,DUKE_JETPACK_IDLE))
            spritesound(DUKE_JETPACK_IDLE,pi);

        if(shrunk) j = 512;
        else j = 2048;

        if ( sb_snum&1 )                            //A (soar high)
        {
            // jump
            SetGameVarID(g_iReturnVarID,0,pi,snum);
            OnEvent(EVENT_SOARUP,pi,snum, -1);
            if(GetGameVarID(g_iReturnVarID,pi,snum) == 0)
            {
                p->posz -= j;
                p->crack_time = 777;
            }
        }

        if (sb_snum&(1<<1))                            //Z (soar low)
        {
            // crouch
            SetGameVarID(g_iReturnVarID,0,pi,snum);
            OnEvent(EVENT_SOARDOWN,pi,snum, -1);
            if(GetGameVarID(g_iReturnVarID,pi,snum) == 0)
            {
                p->posz += j;
                p->crack_time = 777;
            }
        }

        if( shrunk == 0 && (psectlotag == 0 || psectlotag == 2)) k = 32;
        else k = 16;

        if( psectlotag != 2 && p->scuba_on == 1 )
            p->scuba_on = 0;

        if(p->posz > (fz-(k<<8)) )
            p->posz += ((fz-(k<<8))-p->posz)>>1;
        if(p->posz < (hittype[pi].ceilingz+(18<<8)) )
            p->posz = hittype[pi].ceilingz+(18<<8);

    }
    else if( psectlotag != 2 )
    {
        if(p->airleft != 15*26)
            p->airleft = 15*26; //Aprox twenty seconds.

        if(p->scuba_on == 1)
            p->scuba_on = 0;

        if( psectlotag == 1 && p->spritebridge == 0)
        {
            if(shrunk == 0)
            {
                i = 34;
                p->pycount += 32;
                p->pycount &= 2047;
                p->pyoff = sintable[p->pycount]>>6;
            }
            else i = 12;

            if(shrunk == 0 && truefdist <= PHEIGHT)
            {
                if(p->on_ground == 1)
                {
                    if( p->dummyplayersprite == -1 )
                        p->dummyplayersprite =
                            spawn(pi,PLAYERONWATER);
                    sprite[p->dummyplayersprite].pal = sprite[p->i].pal;
                    p->footprintcount = 6;
                    if(sector[p->cursectnum].floorpicnum == FLOORSLIME)
                        p->footprintpal = 8;
                    else p->footprintpal = 0;
                    p->footprintshade = 0;
                }
            }
        }
        else
        {
            if(p->footprintcount > 0 && p->on_ground)
                if( (sector[p->cursectnum].floorstat&2) != 2 )
                {
                    for(j=headspritesect[psect];j>=0;j=nextspritesect[j])
                        if( sprite[j].picnum == FOOTPRINTS || sprite[j].picnum == FOOTPRINTS2 || sprite[j].picnum == FOOTPRINTS3 || sprite[j].picnum == FOOTPRINTS4 )
                            if (klabs(sprite[j].x-p->posx) < 384)
                                if (klabs(sprite[j].y-p->posy) < 384)
                                    break;
                    if(j < 0)
                    {
                        p->footprintcount--;
                        if( sector[p->cursectnum].lotag == 0 && sector[p->cursectnum].hitag == 0 )
                        {
                            switch(TRAND&3)
                            {
                            case 0:  j = spawn(pi,FOOTPRINTS); break;
                            case 1:  j = spawn(pi,FOOTPRINTS2); break;
                            case 2:  j = spawn(pi,FOOTPRINTS3); break;
                            default: j = spawn(pi,FOOTPRINTS4); break;
                            }
                            sprite[j].pal = p->footprintpal;
                            sprite[j].shade = p->footprintshade;
                        }
                    }
                }
        }

        if(p->posz < (fz-(i<<8)) ) //falling
        {

            // not jumping or crouching
            if( (sb_snum&3) == 0 && p->on_ground && (sector[psect].floorstat&2) && p->posz >= (fz-(i<<8)-(16<<8) ) )
                p->posz = fz-(i<<8);
            else
            {
                p->on_ground = 0;
                p->poszv += (gc+80); // (TICSPERFRAME<<6);
                if(p->poszv >= (4096+2048)) p->poszv = (4096+2048);
                if(p->poszv > 2400 && p->falling_counter < 255)
                {
                    p->falling_counter++;
                    if( p->falling_counter == 38 )
                        p->scream_voice = spritesound(DUKE_SCREAM,pi);
                }

                if( (p->posz+p->poszv) >= (fz-(i<<8)) ) // hit the ground
                    if(sector[p->cursectnum].lotag != 1)
                    {
                        if( p->falling_counter > 62 ) quickkill(p);

                        else if( p->falling_counter > 9 )
                        {
                            j = p->falling_counter;
                            s->extra -= j-(TRAND&3);
                            if(s->extra <= 0)
                            {
                                spritesound(SQUISHED,pi);
                                p->pals[0] = 63;
                                p->pals[1] = 0;
                                p->pals[2] = 0;
                                p->pals_time = 63;
                            }
                            else
                            {
                                spritesound(DUKE_LAND,pi);
                                spritesound(DUKE_LAND_HURT,pi);
                            }

                            p->pals[0] = 16;
                            p->pals[1] = 0;
                            p->pals[2] = 0;
                            p->pals_time = 32;
                        }
                        else if(p->poszv > 2048) spritesound(DUKE_LAND,pi);
                    }
            }
        }

        else
        {
            p->falling_counter = 0;
            if(p->scream_voice > FX_Ok)
            {
                FX_StopSound(p->scream_voice);
                p->scream_voice = FX_Ok;
            }

            if(psectlotag != 1 && psectlotag != 2 && p->on_ground == 0 && p->poszv > (6144>>1))
                p->hard_landing = p->poszv>>10;

            p->on_ground = 1;

            if( i==40 )
            {
                //Smooth on the ground

                k = ((fz-(i<<8))-p->posz)>>1;
                if( klabs(k) < 256 ) k = 0;
                p->posz += k;
                p->poszv -= 768;
                if(p->poszv < 0) p->poszv = 0;
            }
            else if(p->jumping_counter == 0)
            {
                p->posz += ((fz-(i<<7))-p->posz)>>1; //Smooth on the water
                if(p->on_warping_sector == 0 && p->posz > fz-(16<<8))
                {
                    p->posz = fz-(16<<8);
                    p->poszv >>= 1;
                }
            }

            p->on_warping_sector = 0;

            if( (sb_snum&2) )
            {
                // crouching
                SetGameVarID(g_iReturnVarID,0,pi,snum);
                OnEvent(EVENT_CROUCH,pi,snum, -1);
                if(GetGameVarID(g_iReturnVarID,pi,snum) == 0)
                {
                    p->posz += (2048+768);
                    p->crack_time = 777;
                }
            }

            // jumping
            if( (sb_snum&1) == 0 && p->jumping_toggle == 1)
                p->jumping_toggle = 0;

            else if( (sb_snum&1) && p->jumping_toggle == 0 )
            {
                if( p->jumping_counter == 0 )
                    if( (fz-cz) > (56<<8) )
                    {
                        SetGameVarID(g_iReturnVarID,0,pi,snum);
                        OnEvent(EVENT_JUMP,pi,snum, -1);
                        if(GetGameVarID(g_iReturnVarID,pi,snum) == 0)
                        {
                            p->jumping_counter = 1;
                            p->jumping_toggle = 1;
                        }
                    }
            }

            if( p->jumping_counter && (sb_snum&1) == 0 )
                p->jumping_toggle = 0;
        }

        if(p->jumping_counter)
        {
            if( (sb_snum&1) == 0 && p->jumping_toggle == 1)
                p->jumping_toggle = 0;

            if( p->jumping_counter < (1024+256) )
            {
                if(psectlotag == 1 && p->jumping_counter > 768)
                {
                    p->jumping_counter = 0;
                    p->poszv = -512;
                }
                else
                {
                    p->poszv -= (sintable[(2048-128+p->jumping_counter)&2047])/12;
                    p->jumping_counter += 180;
                    p->on_ground = 0;
                }
            }
            else
            {
                p->jumping_counter = 0;
                p->poszv = 0;
            }
        }

        p->posz += p->poszv;

        if(p->posz < (cz+(4<<8)))
        {
            p->jumping_counter = 0;
            if(p->poszv < 0)
                p->posxv = p->posyv = 0;
            p->poszv = 128;
            p->posz = cz+(4<<8);
        }
    }

    //Do the quick lefts and rights

    if ( p->fist_incs ||
            p->transporter_hold > 2 ||
            p->hard_landing ||
            p->access_incs > 0 ||
            p->knee_incs > 0 ||
            (*aplWeaponWorksLike[p->curr_weapon] == TRIPBOMB_WEAPON &&
             *kb > 1 &&
             *kb < 4 ) )
    {
        doubvel = 0;
        p->posxv = 0;
        p->posyv = 0;
    }
    else if ( sync[snum].avel )          //p->ang += syncangvel * constant
    {                         //ENGINE calculates angvel for you
        long tempang;

        tempang = sync[snum].avel<<1;

        if( psectlotag == 2 ) p->angvel =(tempang-(tempang>>3))*ksgn(doubvel);
        else p->angvel = tempang*ksgn(doubvel);

        p->ang += p->angvel;
        p->ang &= 2047;
        p->crack_time = 777;
    }

    if(p->spritebridge == 0)
    {
        j = sector[s->sectnum].floorpicnum;

        if( j == PURPLELAVA || sector[s->sectnum].ceilingpicnum == PURPLELAVA )
        {
            if(p->boot_amount > 0)
            {
                p->boot_amount--;
                p->inven_icon = 7;
                if(p->boot_amount <= 0)
                    checkavailinven(p);
            }
            else
            {
                if(!isspritemakingsound(pi,DUKE_LONGTERM_PAIN))
                    spritesound(DUKE_LONGTERM_PAIN,pi);
                p->pals[0] = 0; p->pals[1] = 8; p->pals[2] = 0;
                p->pals_time = 32;
                s->extra--;
            }
        }

        k = 0;

        if(p->on_ground && truefdist <= PHEIGHT+(16<<8))
        {
            switch(dynamictostatic[j])
            {
            case HURTRAIL__STATIC:
                if( rnd(32) )
                {
                    if(p->boot_amount > 0)
                        k = 1;
                    else
                    {
                        if(!isspritemakingsound(pi,DUKE_LONGTERM_PAIN))
                            spritesound(DUKE_LONGTERM_PAIN,pi);
                        p->pals[0] = 64; p->pals[1] = 64; p->pals[2] = 64;
                        p->pals_time = 32;
                        s->extra -= 1+(TRAND&3);
                        if(!isspritemakingsound(pi,SHORT_CIRCUIT))
                            spritesound(SHORT_CIRCUIT,pi);
                    }
                }
                break;
            case FLOORSLIME__STATIC:
                if( rnd(16) )
                {
                    if(p->boot_amount > 0)
                        k = 1;
                    else
                    {
                        if(!isspritemakingsound(pi,DUKE_LONGTERM_PAIN))
                            spritesound(DUKE_LONGTERM_PAIN,pi);
                        p->pals[0] = 0; p->pals[1] = 8; p->pals[2] = 0;
                        p->pals_time = 32;
                        s->extra -= 1+(TRAND&3);
                    }
                }
                break;
            case FLOORPLASMA__STATIC:
                if( rnd(32) )
                {
                    if( p->boot_amount > 0 )
                        k = 1;
                    else
                    {
                        if(!isspritemakingsound(pi,DUKE_LONGTERM_PAIN))
                            spritesound(DUKE_LONGTERM_PAIN,pi);
                        p->pals[0] = 8; p->pals[1] = 0; p->pals[2] = 0;
                        p->pals_time = 32;
                        s->extra -= 1+(TRAND&3);
                    }
                }
                break;
            }
        }

        if( k )
        {
            FTA(75,p);
            p->boot_amount -= 2;
            if(p->boot_amount <= 0)
                checkavailinven(p);
        }
    }

    if ( p->posxv || p->posyv || sync[snum].fvel || sync[snum].svel )
    {
        p->crack_time = 777;

        k = sintable[p->bobcounter&2047]>>12;

        if((truefdist < PHEIGHT+(8<<8) ) && ( k == 1 || k == 3 ))
        {
            if(p->spritebridge == 0 && p->walking_snd_toggle == 0 && p->on_ground)
            {
                switch( psectlotag )
                {
                case 0:

                    if(lz >= 0 && (lz&(MAXSPRITES-1))==49152 )
                        j = sprite[lz&(MAXSPRITES-1)].picnum;
                    else j = sector[psect].floorpicnum;

                    switch(dynamictostatic[j])
                    {
                    case PANNEL1__STATIC:
                    case PANNEL2__STATIC:
                        spritesound(DUKE_WALKINDUCTS,pi);
                        p->walking_snd_toggle = 1;
                        break;
                    }
                    break;
                case 1:
                    if((TRAND&1) == 0)
                        spritesound(DUKE_ONWATER,pi);
                    p->walking_snd_toggle = 1;
                    break;
                }
            }
        }
        else if(p->walking_snd_toggle > 0)
            p->walking_snd_toggle --;

        if(p->jetpack_on == 0 && p->steroids_amount > 0 && p->steroids_amount < 400)
            doubvel <<= 1;

        /*
            loc.bits2 = BUTTON(gamefunc_Move_Forward);
            loc.bits2 |= BUTTON(gamefunc_Move_Backward)<<1;
            loc.bits2 |= BUTTON(gamefunc_Strafe_Left)<<2;
            loc.bits2 |= BUTTON(gamefunc_Strafe_Right)<<3;
            loc.bits2 |= BUTTON(gamefunc_Turn_Left)<<4;
            loc.bits2 |= BUTTON(gamefunc_Turn_Right)<<5;
        */
        if (sync[snum].bits2&(1))
            OnEvent(EVENT_MOVEFORWARD,pi,snum, -1);

        if (sync[snum].bits2&(1<<1))
            OnEvent(EVENT_MOVEBACKWARD,pi,snum, -1);

        if (sync[snum].bits2&(1<<2))
            OnEvent(EVENT_STRAFELEFT,pi,snum, -1);

        if (sync[snum].bits2&(1<<3))
            OnEvent(EVENT_STRAFERIGHT,pi,snum, -1);

        if (sync[snum].bits2&(1<<4) || sync[snum].avel < 0)
            OnEvent(EVENT_TURNLEFT,pi,snum, -1);

        if (sync[snum].bits2&(1<<5) || sync[snum].avel > 0)
            OnEvent(EVENT_TURNRIGHT,pi,snum, -1);

        p->posxv += ((sync[snum].fvel*doubvel)<<6);
        p->posyv += ((sync[snum].svel*doubvel)<<6);

        if( ( aplWeaponWorksLike[p->curr_weapon] == KNEE_WEAPON && *kb > 10 && p->on_ground ) || ( p->on_ground && (sb_snum&2) ) )
        {
            p->posxv = mulscale(p->posxv,p->runspeed-0x2000,16);
            p->posyv = mulscale(p->posyv,p->runspeed-0x2000,16);
        }
        else
        {
            if(psectlotag == 2)
            {
                p->posxv = mulscale(p->posxv,p->runspeed-0x1400,16);
                p->posyv = mulscale(p->posyv,p->runspeed-0x1400,16);
            }
            else
            {
                p->posxv = mulscale(p->posxv,p->runspeed,16);
                p->posyv = mulscale(p->posyv,p->runspeed,16);
            }
        }

        if( abs(p->posxv) < 2048 && abs(p->posyv) < 2048 )
            p->posxv = p->posyv = 0;

        if( shrunk )
        {
            p->posxv =
                mulscale16(p->posxv,p->runspeed-(p->runspeed>>1)+(p->runspeed>>2));
            p->posyv =
                mulscale16(p->posyv,p->runspeed-(p->runspeed>>1)+(p->runspeed>>2));
        }
    }

HORIZONLY:

    if(psectlotag == 1 || p->spritebridge == 1) i = (4L<<8);
    else i = (20L<<8);

    if(sector[p->cursectnum].lotag == 2) k = 0;
    else k = 1;

    if(ud.clipping)
    {
        j = 0;
        p->posx += p->posxv>>14;
        p->posy += p->posyv>>14;
        updatesector(p->posx,p->posy,&p->cursectnum);
        changespritesect(pi,p->cursectnum);
    }
    else
        j = clipmove(&p->posx,&p->posy,
                     &p->posz,&p->cursectnum,
                     p->posxv,p->posyv,164L,(4L<<8),i,CLIPMASK0);

    if(p->jetpack_on == 0 && psectlotag != 2 && psectlotag != 1 && shrunk)
        p->posz += 32<<8;

    if(j)
        checkplayerhurt(p,j);

    if(p->jetpack_on == 0)
    {
        if( s->xvel > 16 )
        {
            if( psectlotag != 1 && psectlotag != 2 && p->on_ground )
            {
                p->pycount += 52;
                p->pycount &= 2047;
                p->pyoff =
                    klabs(s->xvel*sintable[p->pycount])/1596;
            }
        }
        else if( psectlotag != 2 && psectlotag != 1 )
            p->pyoff = 0;
    }

    // RBG***
    setsprite(pi,p->posx,p->posy,p->posz+PHEIGHT);

    if( psectlotag < 3 )
    {
        psect = s->sectnum;
        if( ud.clipping == 0 && sector[psect].lotag == 31)
        {
            if( sprite[sector[psect].hitag].xvel && hittype[sector[psect].hitag].temp_data[0] == 0)
            {
                quickkill(p);
                return;
            }
        }
    }

    if(truefdist < PHEIGHT && p->on_ground && psectlotag != 1 && shrunk == 0 && sector[p->cursectnum].lotag == 1)
        if(!isspritemakingsound(pi,DUKE_ONWATER))
            spritesound(DUKE_ONWATER,pi);

    if (p->cursectnum != s->sectnum)
        changespritesect(pi,p->cursectnum);

    if(ud.clipping == 0)
        j = ( pushmove(&p->posx,&p->posy,&p->posz,&p->cursectnum,164L,(4L<<8),(4L<<8),CLIPMASK0) < 0 && furthestangle(pi,8) < 512 );
    else j = 0;

    if(ud.clipping == 0)
    {
        if( klabs(hittype[pi].floorz-hittype[pi].ceilingz) < (48<<8) || j )
        {
            if ( !(sector[s->sectnum].lotag&0x8000) && ( isanunderoperator(sector[s->sectnum].lotag) ||
                    isanearoperator(sector[s->sectnum].lotag) ) )
                activatebysector(s->sectnum,pi);
            if(j)
            {
                quickkill(p);
                return;
            }
        }
        else if( klabs(fz-cz) < (32<<8) && isanunderoperator(sector[psect].lotag) )
            activatebysector(psect,pi);
    }

    // center_view
    i = 0;
    if( sb_snum&(1<<18) || p->hard_landing)
    {
        SetGameVarID(g_iReturnVarID,0,pi,snum);
        OnEvent(EVENT_RETURNTOCENTER,pi,snum, -1);
        if(GetGameVarID(g_iReturnVarID,pi,snum) == 0)
        {
            p->return_to_center = 9;
        }
    }

    if( sb_snum&(1<<13) )
    {
        // look_up
        SetGameVarID(g_iReturnVarID,0,pi,snum);
        OnEvent(EVENT_LOOKUP,pi,snum, -1);
        if(GetGameVarID(g_iReturnVarID,pi,snum) == 0)
        {
            p->return_to_center = 9;
            if( sb_snum&(1<<5) ) p->horiz += 12;    // running
            p->horiz += 12;
            i++;
        }
    }

    else if( sb_snum&(1<<14) )
    {
        // look_down
        SetGameVarID(g_iReturnVarID,0,pi,snum);
        OnEvent(EVENT_LOOKDOWN,pi,snum, -1);
        if(GetGameVarID(g_iReturnVarID,pi,snum) == 0)
        {
            p->return_to_center = 9;
            if( sb_snum&(1<<5) ) p->horiz -= 12;
            p->horiz -= 12;
            i++;
        }
    }

    else if( sb_snum&(1<<3) )
    {   // aim_up
        SetGameVarID(g_iReturnVarID,0,pi,snum);
        OnEvent(EVENT_AIMUP,pi,snum, -1);
        if(GetGameVarID(g_iReturnVarID,pi,snum) == 0)
        {
            // running
            if( sb_snum&(1<<5) ) p->horiz += 6;
            p->horiz += 6;
            i++;
        }
    }

    else if( sb_snum&(1<<4) )
    {   // aim_down
        SetGameVarID(g_iReturnVarID,0,pi,snum);
        OnEvent(EVENT_AIMDOWN,pi,snum, -1);
        if(GetGameVarID(g_iReturnVarID,pi,snum) == 0)
        {
            // running
            if( sb_snum&(1<<5) ) p->horiz -= 6;
            p->horiz -= 6;
            i++;
        }
    }
    if(p->return_to_center > 0)
        if( (sb_snum&(1<<13)) == 0 && (sb_snum&(1<<14)) == 0 )
        {
            p->return_to_center--;
            p->horiz += 33-(p->horiz/3);
            i++;
        }

    if(p->hard_landing > 0)
    {
        p->hard_landing--;
        p->horiz -= (p->hard_landing<<4);
    }

    if(i)
    {
        if( p->horiz > 95 && p->horiz < 105) p->horiz = 100;
        if( p->horizoff > -5 && p->horizoff < 5) p->horizoff = 0;
    }
    p->horiz += sync[snum].horz;

    if(p->horiz > 299) p->horiz = 299;
    else if(p->horiz < -99) p->horiz = -99;

    //Shooting code/changes

    if(p->show_empty_weapon > 0)
    {
        p->show_empty_weapon--;
        if(p->show_empty_weapon == 0 && (p->weaponswitch & 2) && p->ammo_amount[p->curr_weapon] <= 0)
        {
            if(p->last_full_weapon == GROW_WEAPON)
                p->subweapon |= (1<<GROW_WEAPON);
            else if(p->last_full_weapon == SHRINKER_WEAPON)
                p->subweapon &= ~(1<<GROW_WEAPON);
            addweapon( p, p->last_full_weapon );
            return;
        }
    }

    if(p->knee_incs > 0)
    {
        p->knee_incs++;
        p->horiz -= 48;
        p->return_to_center = 9;
        if(p->knee_incs > 15)
        {
            p->knee_incs = 0;
            p->holster_weapon = 0;
            if(p->weapon_pos < 0)
                p->weapon_pos = -p->weapon_pos;
            if(p->actorsqu >= 0 && dist(&sprite[pi],&sprite[p->actorsqu]) < 1400 && sprite[p->actorsqu].statnum != MAXSTATUS)
            {
                guts(&sprite[p->actorsqu],JIBS6,7,myconnectindex);
                spawn(p->actorsqu,BLOODPOOL);
                spritesound(SQUISHED,p->actorsqu);
                switch(dynamictostatic[sprite[p->actorsqu].picnum])
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
                    if(sprite[p->actorsqu].yvel)
                        operaterespawns(sprite[p->actorsqu].yvel);
                    break;
                }

                if(sprite[p->actorsqu].picnum == APLAYER)
                {
                    quickkill(&ps[sprite[p->actorsqu].yvel]);
                    ps[sprite[p->actorsqu].yvel].frag_ps = snum;
                }
                else if(badguy(&sprite[p->actorsqu]))
                {
                    deletesprite(p->actorsqu);
                    p->actors_killed++;
                }
                else deletesprite(p->actorsqu);
            }
            p->actorsqu = -1;
        }
        else if(p->actorsqu >= 0)
            p->ang += getincangle(p->ang,getangle(sprite[p->actorsqu].x-p->posx,sprite[p->actorsqu].y-p->posy))>>2;
    }

    if( doincrements(p) ) return;

    if(p->weapon_pos != 0)
    {
        if(p->weapon_pos == -9)
        {
            if(p->last_weapon >= 0)
            {
                p->weapon_pos = 10;
                //                if(p->curr_weapon == KNEE_WEAPON) *kb = 1;
                p->last_weapon = -1;
            }
            else if(p->holster_weapon == 0)
                p->weapon_pos = 10;
        }
        else p->weapon_pos--;
    }

    // HACKS

SHOOTINCODE:
    if( sb_snum & (1<<19) ) // 'Holster Weapon
    {
        SetGameVarID(g_iReturnVarID,0,pi,snum);
        SetGameVarID(g_iWeaponVarID,p->curr_weapon,pi,snum);
        SetGameVarID(g_iWorksLikeVarID,aplWeaponWorksLike[p->curr_weapon][snum],pi,snum);
        OnEvent(EVENT_HOLSTER, pi, snum, -1);
        if(GetGameVarID(g_iReturnVarID,pi,snum) == 0)
        {
            if( aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_HOLSTER_CLEARS_CLIP)
            {
                if( p->ammo_amount[p->curr_weapon] > aplWeaponClip[p->curr_weapon][snum]
                        && (p->ammo_amount[p->curr_weapon] % aplWeaponClip[p->curr_weapon][snum]) != 0 )
                {
                    p->ammo_amount[p->curr_weapon]-=
                        p->ammo_amount[p->curr_weapon] % aplWeaponClip[p->curr_weapon][snum] ;
                    (*kb) = aplWeaponTotalTime[p->curr_weapon][snum];
                    sb_snum &= ~(1<<2); // not firing...
                }
                return;
            }
        }
    }

    if( aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_GLOWS)
        p->random_club_frame += 64; // Glowing

    if(p->rapid_fire_hold == 1)
    {
        if( sb_snum&(1<<2) ) return;
        p->rapid_fire_hold = 0;
    }
    if(shrunk || p->tipincs || p->access_incs)
        sb_snum &= ~(1<<2);
    else if ( shrunk == 0 && (sb_snum&(1<<2)) && (*kb) == 0 && p->fist_incs == 0 &&
              p->last_weapon == -1 && ( p->weapon_pos == 0 || p->holster_weapon == 1 ) )
    {
        p->crack_time = 777;

        if(p->holster_weapon == 1)
        {
            if( p->last_pissed_time <= (26*218) && p->weapon_pos == -9)
            {
                p->holster_weapon = 0;
                p->weapon_pos = 10;
                FTA(74,p);
            }
        }
        else
        {
            SetGameVarID(g_iReturnVarID,0,pi,snum);
            SetGameVarID(g_iWeaponVarID,p->curr_weapon,pi,snum);
            SetGameVarID(g_iWorksLikeVarID,aplWeaponWorksLike[p->curr_weapon][snum],pi,snum);
            OnEvent(EVENT_FIRE, pi, snum, -1);
            if(GetGameVarID(g_iReturnVarID,pi,snum) == 0)
            {
                switch(aplWeaponWorksLike[p->curr_weapon][snum])
                {
                case HANDBOMB_WEAPON:
                    OnEvent(EVENT_FIREWEAPON, p->i, snum, -1);

                    p->hbomb_hold_delay = 0;
                    if( p->ammo_amount[p->curr_weapon] > 0 )
                    {
                        (*kb)=1;
                        if(aplWeaponInitialSound[p->curr_weapon][snum])
                        {
                            spritesound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
                        }
                    }
                    break;

                case HANDREMOTE_WEAPON:
                    OnEvent(EVENT_FIREWEAPON, p->i, snum, -1);
                    p->hbomb_hold_delay = 0;
                    (*kb) = 1;
                    if(aplWeaponInitialSound[p->curr_weapon][snum])
                    {
                        spritesound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
                    }
                    break;

                case SHOTGUN_WEAPON:
                    OnEvent(EVENT_FIREWEAPON, p->i, snum, -1);
                    if( p->ammo_amount[p->curr_weapon] > 0 && p->random_club_frame == 0 )
                    {
                        (*kb)=1;
                        if(aplWeaponInitialSound[p->curr_weapon][snum])
                        {
                            spritesound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
                        }
                    }
                    break;

                case TRIPBOMB_WEAPON:
                    if ( p->ammo_amount[p->curr_weapon] > 0 )
                    {
                        long sx,sy,sz;
                        short sect,hw,hitsp;

                        hitscan( p->posx, p->posy, p->posz,
                                 p->cursectnum, sintable[(p->ang+512)&2047],
                                 sintable[p->ang&2047], (100-p->horiz-p->horizoff)*32,
                                 &sect, &hw, &hitsp, &sx, &sy, &sz,CLIPMASK1);

                        if(sect < 0 || hitsp >= 0)
                            break;

                        if( hw >= 0 && sector[sect].lotag > 2 )
                            break;

                        if(hw >= 0 && wall[hw].overpicnum >= 0)
                            if(wall[hw].overpicnum == BIGFORCE)
                                break;

                        j = headspritesect[sect];
                        while(j >= 0)
                        {
                            if( sprite[j].picnum == TRIPBOMB &&
                                    klabs(sprite[j].z-sz) < (12<<8) && ((sprite[j].x-sx)*(sprite[j].x-sx)+(sprite[j].y-sy)*(sprite[j].y-sy)) < (290*290) )
                                break;
                            j = nextspritesect[j];
                        }

                        if(j == -1 && hw >= 0 && (wall[hw].cstat&16) == 0 )
                            if( ( wall[hw].nextsector >= 0 && sector[wall[hw].nextsector].lotag <= 2 ) || ( wall[hw].nextsector == -1 && sector[sect].lotag <= 2 ) )
                                if( ( (sx-p->posx)*(sx-p->posx) + (sy-p->posy)*(sy-p->posy) ) < (290*290) )
                                {
                                    p->posz = p->oposz;
                                    p->poszv = 0;
                                    (*kb) = 1;
                                    if(aplWeaponInitialSound[p->curr_weapon][snum])
                                    {
                                        spritesound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
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
                    OnEvent(EVENT_FIREWEAPON, p->i, snum, -1);
                    if ( p->ammo_amount[p->curr_weapon] > 0)
                    {
                        (*kb) = 1;
                        if(aplWeaponInitialSound[p->curr_weapon][snum])
                        {
                            spritesound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
                        }
                    }
                    break;

                case DEVISTATOR_WEAPON:
                    OnEvent(EVENT_FIREWEAPON, p->i, snum, -1);
                    if( p->ammo_amount[p->curr_weapon] > 0 )
                    {
                        (*kb) = 1;
                        p->hbomb_hold_delay = !p->hbomb_hold_delay;
                        if(aplWeaponInitialSound[p->curr_weapon][snum])
                        {
                            spritesound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
                        }
                    }
                    break;

                case KNEE_WEAPON:
                    OnEvent(EVENT_FIREWEAPON, p->i, snum, -1);
                    if(p->quick_kick == 0)
                    {
                        (*kb) = 1;
                        if(aplWeaponInitialSound[p->curr_weapon][snum])
                        {
                            spritesound(aplWeaponInitialSound[p->curr_weapon][snum], pi);
                        }
                    }
                    break;
                }
            }
        }
    }
    else if((*kb))
    {
        if(aplWeaponWorksLike[p->curr_weapon][snum] == HANDBOMB_WEAPON)
        {
            if( aplWeaponHoldDelay[p->curr_weapon][snum] && ((*kb) == aplWeaponFireDelay[p->curr_weapon][snum]) && (sb_snum&(1<<2)))
            {
                p->rapid_fire_hold = 1;
                return;
            }
            (*kb)++;
            if((*kb)==aplWeaponHoldDelay[p->curr_weapon][snum])
            {
                long lPipeBombControl;

                p->ammo_amount[p->curr_weapon]--;

                if(p->on_ground && (sb_snum&2) )
                {
                    k = 15;
                    i = ((p->horiz+p->horizoff-100)*20);
                }
                else
                {
                    k = 140;
                    i = -512-((p->horiz+p->horizoff-100)*20);
                }

                j = EGS(p->cursectnum,
                        p->posx+(sintable[(p->ang+512)&2047]>>6),
                        p->posy+(sintable[p->ang&2047]>>6),
                        p->posz,aplWeaponShoots[p->curr_weapon][snum],-16,9,9,
                        p->ang,(k+(p->hbomb_hold_delay<<5)),i,pi,1);

                lPipeBombControl=GetGameVar("PIPEBOMB_CONTROL", PIPEBOMB_REMOTE, -1, snum);

                if(lPipeBombControl & PIPEBOMB_TIMER)
                {
                    long lGrenadeLifetime=GetGameVar("GRENADE_LIFETIME", NAM_GRENADE_LIFETIME, -1, snum);
                    long lGrenadeLifetimeVar=GetGameVar("GRENADE_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, -1, snum);
                    hittype[j].temp_data[7]=lGrenadeLifetime
                                            + mulscale(krand(),lGrenadeLifetimeVar, 14)
                                            - lGrenadeLifetimeVar;
                    hittype[j].temp_data[6]=1;
                }
                else
                    hittype[j].temp_data[6]=2;

                if(k == 15)
                {
                    sprite[j].yvel = 3;
                    sprite[j].z += (8<<8);
                }

                k = hits(pi);
                if( k < 512 )
                {
                    sprite[j].ang += 1024;
                    sprite[j].zvel /= 3;
                    sprite[j].xvel /= 3;
                }
                p->hbomb_on = 1;
            }
            else if( (*kb) < aplWeaponHoldDelay[p->curr_weapon][snum] && (sb_snum&(1<<2)) )
            {
                p->hbomb_hold_delay++;
            }
            else if( (*kb) > aplWeaponTotalTime[p->curr_weapon][snum] )
            {
                long lPipeBombControl=GetGameVar("PIPEBOMB_CONTROL", PIPEBOMB_REMOTE, -1, snum);

                (*kb) = 0;

                if(lPipeBombControl == PIPEBOMB_REMOTE)
                {
                    p->curr_weapon = HANDREMOTE_WEAPON;
                    p->last_weapon = -1;
                    p->weapon_pos = 10;
                }
                else
                    checkavailweapon(p);
            }
        }
        else if(aplWeaponWorksLike[p->curr_weapon][snum] == HANDREMOTE_WEAPON)
        {
            (*kb)++;

            if((*kb) == aplWeaponFireDelay[p->curr_weapon][snum])
            {
                if( aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_BOMB_TRIGGER)
                {
                    p->hbomb_on = 0;
                }
                if(aplWeaponShoots[p->curr_weapon][snum] != 0)
                {
                    if(! (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_NOVISIBLE ))
                    {
                        lastvisinc = totalclock+32;
                        p->visibility = 0;
                    }
                    SetGameVarID(g_iWeaponVarID,p->curr_weapon,p->i,snum);
                    SetGameVarID(g_iWorksLikeVarID,aplWeaponWorksLike[p->curr_weapon][snum], p->i, snum);
                    shoot(pi, aplWeaponShoots[p->curr_weapon][snum]);
                }
            }

            if((*kb) >= aplWeaponTotalTime[p->curr_weapon][snum])
            {
                long lPipeBombControl=GetGameVar("PIPEBOMB_CONTROL", PIPEBOMB_REMOTE, -1, snum);
                (*kb) = 0;
                if((p->ammo_amount[HANDBOMB_WEAPON] > 0) && lPipeBombControl == PIPEBOMB_REMOTE)
                    addweapon(p,HANDBOMB_WEAPON);
                else
                    checkavailweapon(p);
            }
        }
        else
        {
            // the basic weapon...
            (*kb)++;

            if(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_CHECKATRELOAD)
            {
                if (aplWeaponWorksLike[p->curr_weapon][snum] == TRIPBOMB_WEAPON)
                {
                    if((*kb) >= aplWeaponTotalTime[p->curr_weapon][snum])
                    {
                        (*kb) = 0;
                        checkavailweapon(p);
                        p->weapon_pos = -9;
                    }
                }
                else if(*kb >= aplWeaponReload[p->curr_weapon][snum])
                    checkavailweapon(p);
            }
            else if(aplWeaponWorksLike[p->curr_weapon][snum]!=KNEE_WEAPON && *kb >= aplWeaponFireDelay[p->curr_weapon][snum])
                checkavailweapon(p);

            if( aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_STANDSTILL
                    && *kb < (aplWeaponFireDelay[p->curr_weapon][snum]+1) )
            {
                p->posz = p->oposz;
                p->poszv = 0;
            }
            if(*kb == aplWeaponSound2Time[p->curr_weapon][snum])
            {
                if(aplWeaponSound2Sound[p->curr_weapon][snum])
                {
                    spritesound(aplWeaponSound2Sound[p->curr_weapon][snum],pi);
                }
            }
            if(*kb == aplWeaponSpawnTime[p->curr_weapon][snum])
                DoSpawn(p);

            if ((*kb) >= aplWeaponTotalTime[p->curr_weapon][snum])
            {
                if(/*!(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_CHECKATRELOAD) && */ p->reloading == 1 ||
                        (aplWeaponReload[p->curr_weapon][snum] > aplWeaponTotalTime[p->curr_weapon][snum] && p->ammo_amount[p->curr_weapon] > 0
                         && (aplWeaponClip[p->curr_weapon][snum]) && (((p->ammo_amount[p->curr_weapon]%(aplWeaponClip[p->curr_weapon][snum]))==0))))
                {
                    int i = aplWeaponReload[p->curr_weapon][snum] - aplWeaponTotalTime[p->curr_weapon][snum];

                    p->reloading = 1;

                    if( (*kb) != (aplWeaponTotalTime[p->curr_weapon][snum]))
                    {
                        if( (*kb) == (aplWeaponTotalTime[p->curr_weapon][snum]+1))
                        {
                            if(aplWeaponReloadSound1[p->curr_weapon][snum])
                                spritesound(aplWeaponReloadSound1[p->curr_weapon][snum],pi);
                        }
                        else if( ((*kb) == (aplWeaponReload[p->curr_weapon][snum] - (i/3)) && !(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_RELOAD_TIMING)) ||
                                 ((*kb) == (aplWeaponReload[p->curr_weapon][snum] - i+4) && (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_RELOAD_TIMING)) )
                        {
                            if(aplWeaponReloadSound2[p->curr_weapon][snum])
                                spritesound(aplWeaponReloadSound2[p->curr_weapon][snum],pi);
                        }
                        else if( (*kb) >= (aplWeaponReload[p->curr_weapon][snum]) )
                        {
                            *kb=0;
                            p->reloading = 0;
                        }
                    }
                }
                else
                {
                    if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_AUTOMATIC &&
                            (aplWeaponWorksLike[p->curr_weapon][snum]==KNEE_WEAPON?1:p->ammo_amount[p->curr_weapon] > 0))
                    {
                        if( sb_snum&(1<<2) )
                        {
                            if(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_RANDOMRESTART)
                                *kb = 1+(TRAND&3);
                            else *kb=1;
                        }
                        else *kb = 0;
                    }
                    else *kb = 0;

                    if( aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_RESET &&
                            ((aplWeaponWorksLike[p->curr_weapon][snum] == KNEE_WEAPON)?1:p->ammo_amount[p->curr_weapon] > 0))
                    {
                        if( sb_snum&(1<<2) ) *kb = 1;
                        else *kb = 0;
                    }
                }
            }
            else if ( *kb >= aplWeaponFireDelay[p->curr_weapon][snum] && (*kb) < aplWeaponTotalTime[p->curr_weapon][snum]
                      && ((aplWeaponWorksLike[p->curr_weapon][snum] == KNEE_WEAPON)?1:p->ammo_amount[p->curr_weapon] > 0))
            {
                if ( aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_AUTOMATIC )
                {
                    if(!(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_SEMIAUTO))
                    {
                        if(( sb_snum&(1<<2) ) == 0 && aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_RESET)
                            *kb = 0;
                        if ( aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_FIREEVERYTHIRD)
                        {
                            if( ((*(kb))%3) == 0 )
                            {
                                DoFire(p);
                                DoSpawn(p);
                            }
                        }
                        else if( aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_FIREEVERYOTHER)
                        {
                            DoFire(p);
                            DoSpawn(p);
                        }
                        else
                        {
                            if(*kb == aplWeaponFireDelay[p->curr_weapon][snum])
                            {
                                DoFire(p);
                                DoSpawn(p);
                            }
                        }
                        if(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_RESET &&
                                (*kb) > aplWeaponTotalTime[p->curr_weapon][snum]-aplWeaponHoldDelay[p->curr_weapon][snum] &&
                                ((aplWeaponWorksLike[p->curr_weapon][snum] == KNEE_WEAPON)?1:p->ammo_amount[p->curr_weapon] > 0))
                        {
                            if( sb_snum&(1<<2) ) *kb = 1;
                            else *kb = 0;
                        }
                    }
                    else
                    {
                        if( aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_FIREEVERYOTHER)
                        {
                            DoFire(p);
                            DoSpawn(p);
                        }
                        else
                        {
                            if(*kb == aplWeaponFireDelay[p->curr_weapon][snum])
                            {
                                DoFire(p);
                                DoSpawn(p);
                            }
                        }
                    }
                }
                else if(*kb == aplWeaponFireDelay[p->curr_weapon][snum])
                    DoFire(p);
            }
        }
    }
}

//UPDATE THIS FILE OVER THE OLD GETSPRITESCORE/COMPUTERGETINPUT FUNCTIONS
int getspritescore(long snum, long dapicnum)
{
    switch(dynamictostatic[dapicnum])
    {
    case FIRSTGUNSPRITE__STATIC: return(20);
    case CHAINGUNSPRITE__STATIC: return(50);
    case RPGSPRITE__STATIC: return(200);
    case FREEZESPRITE__STATIC: return(25);
    case SHRINKERSPRITE__STATIC: return(80);
    case HEAVYHBOMB__STATIC: return(60);
    case TRIPBOMBSPRITE__STATIC: return(50);
    case SHOTGUNSPRITE__STATIC: return(120);
    case DEVISTATORSPRITE__STATIC: return(120);

    case FREEZEAMMO__STATIC: if (ps[snum].ammo_amount[FREEZE_WEAPON] < max_ammo_amount[FREEZE_WEAPON]) return(10); else return(0);
    case AMMO__STATIC: if (ps[snum].ammo_amount[PISTOL_WEAPON] < max_ammo_amount[PISTOL_WEAPON]) return(10); else return(0);
    case BATTERYAMMO__STATIC: if (ps[snum].ammo_amount[CHAINGUN_WEAPON] < max_ammo_amount[CHAINGUN_WEAPON]) return(20); else return(0);
    case DEVISTATORAMMO__STATIC: if (ps[snum].ammo_amount[DEVISTATOR_WEAPON] < max_ammo_amount[DEVISTATOR_WEAPON]) return(25); else return(0);
    case RPGAMMO__STATIC: if (ps[snum].ammo_amount[RPG_WEAPON] < max_ammo_amount[RPG_WEAPON]) return(50); else return(0);
    case CRYSTALAMMO__STATIC: if (ps[snum].ammo_amount[SHRINKER_WEAPON] < max_ammo_amount[SHRINKER_WEAPON]) return(10); else return(0);
    case HBOMBAMMO__STATIC: if (ps[snum].ammo_amount[HANDBOMB_WEAPON] < max_ammo_amount[HANDBOMB_WEAPON]) return(30); else return(0);
    case SHOTGUNAMMO__STATIC: if (ps[snum].ammo_amount[SHOTGUN_WEAPON] < max_ammo_amount[SHOTGUN_WEAPON]) return(25); else return(0);

    case COLA__STATIC: if (sprite[ps[snum].i].extra < 100) return(10); else return(0);
    case SIXPAK__STATIC: if (sprite[ps[snum].i].extra < 100) return(30); else return(0);
    case FIRSTAID__STATIC: if (ps[snum].firstaid_amount < 100) return(100); else return(0);
    case SHIELD__STATIC: if (ps[snum].shield_amount < 100) return(50); else return(0);
    case STEROIDS__STATIC: if (ps[snum].steroids_amount < 400) return(30); else return(0);
    case AIRTANK__STATIC: if (ps[snum].scuba_amount < 6400) return(30); else return(0);
    case JETPACK__STATIC: if (ps[snum].jetpack_amount < 1600) return(100); else return(0);
    case HEATSENSOR__STATIC: if (ps[snum].heat_amount < 1200) return(5); else return(0);
    case ACCESSCARD__STATIC: return(1);
    case BOOTS__STATIC: if (ps[snum].boot_amount < 200) return(15); else return(0);
    case ATOMICHEALTH__STATIC: if (sprite[ps[snum].i].extra < max_player_health<<1) return(50); else return(0);
    case HOLODUKE__STATIC: if (ps[snum].holoduke_amount < 2400) return(5); else return(0);

    case SECTOREFFECTOR__STATIC: return(1);
    case TOUCHPLATE__STATIC: return(1);
    case MUSICANDSFX__STATIC: return(1);
    }
    return(0);
}

static long fdmatrix[12][12] =
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

static long goalx[MAXPLAYERS], goaly[MAXPLAYERS], goalz[MAXPLAYERS];
static long goalsect[MAXPLAYERS], goalwall[MAXPLAYERS], goalsprite[MAXPLAYERS];
static long goalplayer[MAXPLAYERS], clipmovecount[MAXPLAYERS];
short searchsect[MAXSECTORS], searchparent[MAXSECTORS];
char dashow2dsector[(MAXSECTORS+7)>>3];
void computergetinput(long snum, input *syn)
{
    long i, j, k, l, x1, y1, z1, x2, y2, z2, x3, y3, z3, dx, dy;
    long dist, daang, zang, fightdist, damyang, damysect;
    long startsect, endsect, splc, send, startwall, endwall;
    short dasect, dawall, daspr;
    struct player_struct *p;
    walltype *wal;

    p = &ps[snum];
    syn->fvel = 0;
    syn->svel = 0;
    syn->avel = 0;
    syn->horz = 0;
    syn->bits = 0;

    x1 = sprite[p->i].x;
    y1 = sprite[p->i].y;
    z1 = sprite[p->i].z;
    damyang = sprite[p->i].ang;
    damysect = sprite[p->i].sectnum;
    if ((numplayers >= 2) && (snum == myconnectindex))
    { x1 = myx; y1 = myy; z1 = myz+PHEIGHT; damyang = myang; damysect = mycursectnum; }

    if (!(numframes&7))
    {
        x2 = sprite[ps[goalplayer[snum]].i].x;
        y2 = sprite[ps[goalplayer[snum]].i].y;
        z2 = sprite[ps[goalplayer[snum]].i].z;

        if (!cansee(x1,y1,z1-(48<<8),damysect,x2,y2,z2-(48<<8),sprite[ps[goalplayer[snum]].i].sectnum))
            goalplayer[snum] = snum;
    }

    if ((goalplayer[snum] == snum) || (ps[goalplayer[snum]].dead_flag != 0))
    {
        j = 0x7fffffff;
        for(i=connecthead;i>=0;i=connectpoint2[i])
            if (i != snum)
            {
                dist = ksqrt((sprite[ps[i].i].x-x1)*(sprite[ps[i].i].x-x1)+(sprite[ps[i].i].y-y1)*(sprite[ps[i].i].y-y1));

                x2 = sprite[ps[i].i].x;
                y2 = sprite[ps[i].i].y;
                z2 = sprite[ps[i].i].z;
                if (!cansee(x1,y1,z1-(48<<8),damysect,x2,y2,z2-(48<<8),sprite[ps[i].i].sectnum))
                    dist <<= 1;

            if (dist < j) { j = dist; goalplayer[snum] = i; }
            }
    }

    x2 = sprite[ps[goalplayer[snum]].i].x;
    y2 = sprite[ps[goalplayer[snum]].i].y;
    z2 = sprite[ps[goalplayer[snum]].i].z;

    if (p->dead_flag) syn->bits |= (1<<29);
    if ((p->firstaid_amount > 0) && (p->last_extra < 100))
        syn->bits |= (1<<16);

    for(j=headspritestat[4];j>=0;j=nextspritestat[j])
    {
        switch (dynamictostatic[sprite[j].picnum])
        {
        case TONGUE__STATIC: k = 4; break;
        case FREEZEBLAST__STATIC: k = 4; break;
        case SHRINKSPARK__STATIC: k = 16; break;
        case RPG__STATIC: k = 16; break;
        default: k = 0; break;
        }
        if (k)
        {
            x3 = sprite[j].x;
            y3 = sprite[j].y;
            z3 = sprite[j].z;
            for(l=0;l<=8;l++)
            {
                if (tmulscale11(x3-x1,x3-x1,y3-y1,y3-y1,(z3-z1)>>4,(z3-z1)>>4) < 3072)
                {
                    dx = sintable[(sprite[j].ang+512)&2047];
                    dy = sintable[sprite[j].ang&2047];
                    if ((x1-x3)*dy > (y1-y3)*dx) i = -k*512; else i = k*512;
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
                    hitscan(sprite[j].x,sprite[j].y,sprite[j].z,sprite[j].sectnum,
                            mulscale14(sprite[j].xvel,sintable[(sprite[j].ang+512)&2047]),
                            mulscale14(sprite[j].xvel,sintable[sprite[j].ang&2047]),
                            (long)sprite[j].zvel,
                            &dasect,&dawall,&daspr,&x3,&y3,&z3,CLIPMASK1);
                }
            }
        }
    }

    if ((ps[goalplayer[snum]].dead_flag == 0) &&
            ((cansee(x1,y1,z1,damysect,x2,y2,z2,sprite[ps[goalplayer[snum]].i].sectnum)) ||
             (cansee(x1,y1,z1-(24<<8),damysect,x2,y2,z2-(24<<8),sprite[ps[goalplayer[snum]].i].sectnum)) ||
             (cansee(x1,y1,z1-(48<<8),damysect,x2,y2,z2-(48<<8),sprite[ps[goalplayer[snum]].i].sectnum))))
    {
        syn->bits |= (1<<2);

        if ((p->curr_weapon == HANDBOMB_WEAPON) && (!(rand()&7)))
            syn->bits &= ~(1<<2);

        if (p->curr_weapon == TRIPBOMB_WEAPON)
            syn->bits |= ((rand()%MAX_WEAPONS)<<8);

        if (p->curr_weapon == RPG_WEAPON)
        {
            hitscan(x1,y1,z1-PHEIGHT,damysect,sintable[(damyang+512)&2047],sintable[damyang&2047],
                    (100-p->horiz-p->horizoff)*32,&dasect,&dawall,&daspr,&x3,&y3,&z3,CLIPMASK1);
            if ((x3-x1)*(x3-x1)+(y3-y1)*(y3-y1) < 2560*2560) syn->bits &= ~(1<<2);
        }


        fightdist = fdmatrix[p->curr_weapon][ps[goalplayer[snum]].curr_weapon];
        if (fightdist < 128) fightdist = 128;
        dist = ksqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)); if (dist == 0) dist = 1;
        daang = getangle(x2+(ps[goalplayer[snum]].posxv>>14)-x1,y2+(ps[goalplayer[snum]].posyv>>14)-y1);
        zang = 100-((z2-z1)*8)/dist;
        fightdist = max(fightdist,(klabs(z2-z1)>>4));

        if (sprite[ps[goalplayer[snum]].i].yrepeat < 32)
        { fightdist = 0; syn->bits &= ~(1<<2); }
        if (sprite[ps[goalplayer[snum]].i].pal == 1)
        { fightdist = 0; syn->bits &= ~(1<<2); }

        if (dist < 256) syn->bits |= (1<<22);

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
            dx = sintable[(sprite[ps[goalplayer[snum]].i].ang+512)&2047];
            dy = sintable[sprite[ps[goalplayer[snum]].i].ang&2047];
            if ((x1-x2)*dy > (y1-y2)*dx) i += 8192; else i -= 8192;
            syn->fvel += ((sintable[(daang+1024)&2047]*i)>>17);
            syn->svel += ((sintable[(daang+512)&2047]*i)>>17);
        }

        syn->avel = min(max((((daang+1024-damyang)&2047)-1024)>>1,-127),127);
        syn->horz = min(max((zang-p->horiz),-MAXHORIZ),MAXHORIZ);
        syn->bits |= (1<<23);
        return;
    }

    goalsect[snum] = -1;
    if (goalsect[snum] < 0)
    {
        goalwall[snum] = -1;
        startsect = sprite[p->i].sectnum;
        endsect = sprite[ps[goalplayer[snum]].i].sectnum;

        clearbufbyte(dashow2dsector,(MAXSECTORS+7)>>3,0L);
        searchsect[0] = startsect;
        searchparent[0] = -1;
        dashow2dsector[startsect>>3] |= (1<<(startsect&7));
        for(splc=0,send=1;splc<send;splc++)
        {
            startwall = sector[searchsect[splc]].wallptr;
            endwall = startwall + sector[searchsect[splc]].wallnum;
            for(i=startwall,wal=&wall[startwall];i<endwall;i++,wal++)
            {
                j = wal->nextsector; if (j < 0) continue;

                dx = ((wall[wal->point2].x+wal->x)>>1);
                dy = ((wall[wal->point2].y+wal->y)>>1);
                if ((getceilzofslope(j,dx,dy) > getflorzofslope(j,dx,dy)-(28<<8)) && ((sector[j].lotag < 15) || (sector[j].lotag > 22)))
                    continue;
                if (getflorzofslope(j,dx,dy) < getflorzofslope(searchsect[splc],dx,dy)-(72<<8))
                    continue;
                if ((dashow2dsector[j>>3]&(1<<(j&7))) == 0)
                {
                    dashow2dsector[j>>3] |= (1<<(j&7));
                    searchsect[send] = (short)j;
                    searchparent[send] = (short)splc;
                    send++;
                    if (j == endsect)
                    {
                        clearbufbyte(dashow2dsector,(MAXSECTORS+7)>>3,0L);
                        for(k=send-1;k>=0;k=searchparent[k])
                            dashow2dsector[searchsect[k]>>3] |= (1<<(searchsect[k]&7));

                        for(k=send-1;k>=0;k=searchparent[k])
                            if (!searchparent[k]) break;

                        goalsect[snum] = searchsect[k];
                        startwall = sector[goalsect[snum]].wallptr;
                        endwall = startwall+sector[goalsect[snum]].wallnum;
                        x3 = y3 = 0;
                        for(i=startwall;i<endwall;i++)
                        {
                            x3 += wall[i].x;
                            y3 += wall[i].y;
                        }
                        x3 /= (endwall-startwall);
                        y3 /= (endwall-startwall);

                        startwall = sector[startsect].wallptr;
                        endwall = startwall+sector[startsect].wallnum;
                        l = 0; k = startwall;
                        for(i=startwall;i<endwall;i++)
                        {
                            if (wall[i].nextsector != goalsect[snum]) continue;
                            dx = wall[wall[i].point2].x-wall[i].x;
                            dy = wall[wall[i].point2].y-wall[i].y;

                            //if (dx*(y1-wall[i].y) <= dy*(x1-wall[i].x))
                            //   if (dx*(y2-wall[i].y) >= dy*(x2-wall[i].x))
                            if ((x3-x1)*(wall[i].y-y1) <= (y3-y1)*(wall[i].x-x1))
                                if ((x3-x1)*(wall[wall[i].point2].y-y1) >= (y3-y1)*(wall[wall[i].point2].x-x1))
                                { k = i; break; }

                            dist = ksqrt(dx*dx+dy*dy);
                            if (dist > l) { l = dist; k = i; }
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

            for(i=headspritesect[searchsect[splc]];i>=0;i=nextspritesect[i])
                if (sprite[i].lotag == 7)
                {
                    j = sprite[sprite[i].owner].sectnum;
                    if ((dashow2dsector[j>>3]&(1<<(j&7))) == 0)
                    {
                        dashow2dsector[j>>3] |= (1<<(j&7));
                        searchsect[send] = (short)j;
                        searchparent[send] = (short)splc;
                        send++;
                        if (j == endsect)
                        {
                            clearbufbyte(dashow2dsector,(MAXSECTORS+7)>>3,0L);
                            for(k=send-1;k>=0;k=searchparent[k])
                                dashow2dsector[searchsect[k]>>3] |= (1<<(searchsect[k]&7));

                            for(k=send-1;k>=0;k=searchparent[k])
                                if (!searchparent[k]) break;

                            goalsect[snum] = searchsect[k];
                            startwall = sector[startsect].wallptr;
                            endwall = startwall+sector[startsect].wallnum;
                            l = 0; k = startwall;
                            for(i=startwall;i<endwall;i++)
                            {
                                dx = wall[wall[i].point2].x-wall[i].x;
                                dy = wall[wall[i].point2].y-wall[i].y;
                                dist = ksqrt(dx*dx+dy*dy);
                                if ((wall[i].nextsector == goalsect[snum]) && (dist > l))
                                { l = dist; k = i; }
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
        if (goalsprite[snum] < 0)
        {
            for(k=0;k<4;k++)
            {
                i = (rand()%numsectors);
                for(j=headspritesect[i];j>=0;j=nextspritesect[j])
                {
                    if ((sprite[j].xrepeat <= 0) || (sprite[j].yrepeat <= 0)) continue;
                    if (getspritescore(snum,sprite[j].picnum) <= 0) continue;
                    if (cansee(x1,y1,z1-(32<<8),damysect,sprite[j].x,sprite[j].y,sprite[j].z-(4<<8),i))
                    { goalx[snum] = sprite[j].x; goaly[snum] = sprite[j].y; goalz[snum] = sprite[j].z; goalsprite[snum] = j; break; }
                }
            }
        }
        x2 = goalx[snum];
        y2 = goaly[snum];
        dist = ksqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)); if (!dist) return;
        daang = getangle(x2-x1,y2-y1);
        syn->fvel += (x2-x1)*2047/dist;
        syn->svel += (y2-y1)*2047/dist;
        syn->avel = min(max((((daang+1024-damyang)&2047)-1024)>>3,-127),127);
    }
    else
        goalsprite[snum] = -1;

    x3 = p->posx; y3 = p->posy; z3 = p->posz; dasect = p->cursectnum;
    i = clipmove(&x3,&y3,&z3,&dasect,p->posxv,p->posyv,164L,4L<<8,4L<<8,CLIPMASK0);
    if (!i)
    {
        x3 = p->posx; y3 = p->posy; z3 = p->posz+(24<<8); dasect = p->cursectnum;
        i = clipmove(&x3,&y3,&z3,&dasect,p->posxv,p->posyv,164L,4L<<8,4L<<8,CLIPMASK0);
    }
    if (i)
    {
        clipmovecount[snum]++;

        j = 0;
        if ((i&0xc000) == 32768)  //Hit a wall (49152 for sprite)
            if (wall[i&(MAXWALLS-1)].nextsector >= 0)
            {
                if (getflorzofslope(wall[i&(MAXWALLS-1)].nextsector,p->posx,p->posy) <= p->posz+(24<<8)) j |= 1;
                if (getceilzofslope(wall[i&(MAXWALLS-1)].nextsector,p->posx,p->posy) >= p->posz-(24<<8)) j |= 2;
            }
        if ((i&0xc000) == 49152) j = 1;
        if (j&1) if (clipmovecount[snum] == 4) syn->bits |= (1<<0);
        if (j&2) syn->bits |= (1<<1);

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

        if ((clipmovecount[snum]&31) == 2) syn->bits |= (1<<29);
        if ((clipmovecount[snum]&31) == 17) syn->bits |= (1<<22);
    if (clipmovecount[snum] > 32) { goalsect[snum] = -1; goalwall[snum] = -1; clipmovecount[snum] = 0; }

        goalsprite[snum] = -1;
    }
    else
        clipmovecount[snum] = 0;

    if ((goalsect[snum] >= 0) && (goalwall[snum] >= 0))
    {
        x2 = goalx[snum];
        y2 = goaly[snum];
        dist = ksqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)); if (!dist) return;
        daang = getangle(x2-x1,y2-y1);
        if ((goalwall[snum] >= 0) && (dist < 4096))
            daang = ((getangle(wall[wall[goalwall[snum]].point2].x-wall[goalwall[snum]].x,wall[wall[goalwall[snum]].point2].y-wall[goalwall[snum]].y)+1536)&2047);
        syn->fvel += (x2-x1)*2047/dist;
        syn->svel += (y2-y1)*2047/dist;
        syn->avel = min(max((((daang+1024-damyang)&2047)-1024)>>3,-127),127);
    }
}

