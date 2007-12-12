//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2004, 2007 - EDuke32 developers

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
#include "duke3d.h"

extern int numenvsnds;
extern int actor_tog;

void updateinterpolations()  //Stick at beginning of domovethings
{
    int i=numinterpolations-1;

    for (;i>=0;i--) oldipos[i] = *curipos[i];
}

void setinterpolation(int *posptr)
{
    int i;

    if (numinterpolations >= MAXINTERPOLATIONS) return;
    for (i=numinterpolations-1;i>=0;i--)
        if (curipos[i] == posptr) return;
    curipos[numinterpolations] = posptr;
    oldipos[numinterpolations] = *posptr;
    numinterpolations++;
}

void stopinterpolation(int *posptr)
{
    int i;

    for (i=numinterpolations-1;i>=startofdynamicinterpolations;i--)
        if (curipos[i] == posptr)
        {
            numinterpolations--;
            oldipos[i] = oldipos[numinterpolations];
            bakipos[i] = bakipos[numinterpolations];
            curipos[i] = curipos[numinterpolations];
        }
}

void dointerpolations(int smoothratio)       //Stick at beginning of drawscreen
{
    int i=numinterpolations-1, j = 0, odelta, ndelta = 0;

    for (;i>=0;i--)
    {
        bakipos[i] = *curipos[i];
        odelta = ndelta;
        ndelta = (*curipos[i])-oldipos[i];
        if (odelta != ndelta) j = mulscale16(ndelta,smoothratio);
        *curipos[i] = oldipos[i]+j;
    }
}

void restoreinterpolations()  //Stick at end of drawscreen
{
    int i=numinterpolations-1;

    for (;i>=0;i--) *curipos[i] = bakipos[i];
}

int ceilingspace(int sectnum)
{
    if ((sector[sectnum].ceilingstat&1) && sector[sectnum].ceilingpal == 0 && (sector[sectnum].ceilingpicnum==MOONSKY1 || sector[sectnum].ceilingpicnum==BIGORBIT1))
        return 1;
    return 0;
}

int floorspace(int sectnum)
{
    if ((sector[sectnum].floorstat&1) && sector[sectnum].ceilingpal == 0)
    {
        if ((sector[sectnum].floorpicnum==MOONSKY1)||(sector[sectnum].floorpicnum==BIGORBIT1)) return 1;
    }
    return 0;
}

void addammo(int weapon,player_struct *p,int amount)
{
    p->ammo_amount[weapon] += amount;

    if (p->ammo_amount[weapon] > p->max_ammo_amount[weapon])
        p->ammo_amount[weapon] = p->max_ammo_amount[weapon];
}

void addweaponnoswitch(player_struct *p, int weapon)
{
    if (p->gotweapon[weapon] == 0)
    {
        p->gotweapon[weapon] = 1;
        if (weapon == SHRINKER_WEAPON)
            p->gotweapon[GROW_WEAPON] = 1;
    }
    switch (p->curr_weapon)
    {
    case KNEE_WEAPON:
    case TRIPBOMB_WEAPON:
    case HANDREMOTE_WEAPON:
    case HANDBOMB_WEAPON:
        break;
    case SHOTGUN_WEAPON:
        stopspritesound(SHOTGUN_COCK,p->i);
        break;
    case PISTOL_WEAPON:
        stopspritesound(INSERT_CLIP,p->i);
        break;
    default:
        stopspritesound(SELECT_WEAPON,p->i);
        break;
    }
    switch (weapon)
    {
    case KNEE_WEAPON:
    case TRIPBOMB_WEAPON:
    case HANDREMOTE_WEAPON:
    case HANDBOMB_WEAPON:
        break;
    case SHOTGUN_WEAPON:
        spritesound(SHOTGUN_COCK,p->i);
        break;
    case PISTOL_WEAPON:
        spritesound(INSERT_CLIP,p->i);
        break;
    default:
        spritesound(SELECT_WEAPON,p->i);
        break;
    }
}

void addweapon(player_struct *p,int weapon)
{
    int snum = sprite[p->i].yvel;

    addweaponnoswitch(p,weapon);

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

    if (p->curr_weapon != weapon)
        OnEvent(EVENT_CHANGEWEAPON,p->i, snum, -1);

    p->curr_weapon = weapon;

    SetGameVarID(g_iWeaponVarID,p->curr_weapon, p->i, snum);
    if (p->curr_weapon>=0)
    {
        SetGameVarID(g_iWorksLikeVarID,aplWeaponWorksLike[p->curr_weapon][snum], p->i, snum);
    }
    else
    {
        SetGameVarID(g_iWorksLikeVarID,-1, p->i, snum);
    }
}

void checkavailinven(player_struct *p)
{

    if (p->firstaid_amount > 0)
        p->inven_icon = 1;
    else if (p->steroids_amount > 0)
        p->inven_icon = 2;
    else if (p->holoduke_amount > 0)
        p->inven_icon = 3;
    else if (p->jetpack_amount > 0)
        p->inven_icon = 4;
    else if (p->heat_amount > 0)
        p->inven_icon = 5;
    else if (p->scuba_amount > 0)
        p->inven_icon = 6;
    else if (p->boot_amount > 0)
        p->inven_icon = 7;
    else p->inven_icon = 0;
}

void checkavailweapon(player_struct *p)
{
    short i,snum;
    int32 weap;

    if (p->reloading) return;

    if (p->wantweaponfire >= 0)
    {
        weap = p->wantweaponfire;
        p->wantweaponfire = -1;

        if (weap == p->curr_weapon) return;
        else if (p->gotweapon[weap] && p->ammo_amount[weap] > 0)
        {
            addweapon(p,weap);
            return;
        }
    }

    weap = p->curr_weapon;
    if (p->gotweapon[weap] && p->ammo_amount[weap] > 0)
        return;
    if (p->gotweapon[weap] && !(p->weaponswitch & 2))
        return;

    snum = sprite[p->i].yvel;

    for (i=0;i<10;i++)
    {
        weap = g_player[snum].wchoice[i];
        if (VOLUMEONE && weap > 6) continue;

        if (weap == 0) weap = 9;
        else weap--;

        if (weap == 0 || (p->gotweapon[weap] && p->ammo_amount[weap] > 0))
            break;
    }

    if (i == 10) weap = 0;

    // Found the weapon

    p->last_weapon  = p->curr_weapon;
    p->random_club_frame = 0;
    p->curr_weapon  = weap;
    SetGameVarID(g_iWeaponVarID,p->curr_weapon, p->i, snum);
    if (p->curr_weapon>=0)
    {
        SetGameVarID(g_iWorksLikeVarID,aplWeaponWorksLike[p->curr_weapon][snum], p->i, snum);
    }
    else
    {
        SetGameVarID(g_iWorksLikeVarID,-1, p->i, snum);
    }
    OnEvent(EVENT_CHANGEWEAPON,p->i, snum, -1);
    p->kickback_pic = 0;
    if (p->holster_weapon == 1)
    {
        p->holster_weapon = 0;
        p->weapon_pos = 10;
    }
    else p->weapon_pos   = -1;
}

void hitradius(int i, int  r, int  hp1, int  hp2, int  hp3, int  hp4)
{
    spritetype *s=&sprite[i],*sj;
    walltype *wal;
    int d, q, x1, y1;
    int sectcnt, sectend, dasect, startwall, endwall, nextsect;
    int j,k,p,x,nextj;
    short sect=-1;
    char statlist[] = {0,1,6,10,12,2,5};
    short *tempshort = (short *)tempbuf;

    if (s->picnum == RPG && s->xrepeat < 11) goto SKIPWALLCHECK;

    if (s->picnum != SHRINKSPARK)
    {
        tempshort[0] = s->sectnum;
        dasect = s->sectnum;
        sectcnt = 0;
        sectend = 1;

        do
        {
            dasect = tempshort[sectcnt++];
            if (((sector[dasect].ceilingz-s->z)>>8) < r)
            {
                d = klabs(wall[sector[dasect].wallptr].x-s->x)+klabs(wall[sector[dasect].wallptr].y-s->y);
                if (d < r)
                    checkhitceiling(dasect);
                else
                {
                    d = klabs(wall[wall[wall[sector[dasect].wallptr].point2].point2].x-s->x)+klabs(wall[wall[wall[sector[dasect].wallptr].point2].point2].y-s->y);
                    if (d < r)
                        checkhitceiling(dasect);
                }
            }

            startwall = sector[dasect].wallptr;
            endwall = startwall+sector[dasect].wallnum;
            for (x=startwall,wal=&wall[startwall];x<endwall;x++,wal++)
                if ((klabs(wal->x-s->x)+klabs(wal->y-s->y)) < r)
                {
                    nextsect = wal->nextsector;
                    if (nextsect >= 0)
                    {
                        for (dasect=sectend-1;dasect>=0;dasect--)
                            if (tempshort[dasect] == nextsect) break;
                        if (dasect < 0) tempshort[sectend++] = nextsect;
                    }
                    x1 = (((wal->x+wall[wal->point2].x)>>1)+s->x)>>1;
                    y1 = (((wal->y+wall[wal->point2].y)>>1)+s->y)>>1;
                    updatesector(x1,y1,&sect);
                    if (sect >= 0 && cansee(x1,y1,s->z,sect,s->x,s->y,s->z,s->sectnum))
                        checkhitwall(i,x,wal->x,wal->y,s->z,s->picnum);
                }
        }
        while (sectcnt < sectend);
    }

SKIPWALLCHECK:

    q = -(16<<8)+(TRAND&((32<<8)-1));

    for (x = 0;x<7;x++)
    {
        j = headspritestat[(unsigned char)statlist[x]];
        while (j >= 0)
        {
            nextj = nextspritestat[j];
            sj = &sprite[j];

            if (x == 0 || x >= 5 || AFLAMABLE(sj->picnum))
            {
                if (s->picnum != SHRINKSPARK || (sj->cstat&257))
                    if (dist(s, sj) < r)
                    {
                        if (badguy(sj) && !cansee(sj->x, sj->y,sj->z+q, sj->sectnum, s->x, s->y, s->z+q, s->sectnum))
                            goto BOLT;
                        checkhitsprite(j, i);
                    }
            }
            else if (sj->extra >= 0 && sj != s && (sj->picnum == TRIPBOMB || badguy(sj) || sj->picnum == QUEBALL || sj->picnum == STRIPEBALL || (sj->cstat&257) || sj->picnum == DUKELYINGDEAD))
            {
                if (s->picnum == SHRINKSPARK && sj->picnum != SHARK && (j == s->owner || sj->xrepeat < 24))
                {
                    j = nextj;
                    continue;
                }
                if (s->picnum == MORTER && j == s->owner)
                {
                    j = nextj;
                    continue;
                }

                if (sj->picnum == APLAYER) sj->z -= PHEIGHT;
                d = dist(s, sj);
                if (sj->picnum == APLAYER) sj->z += PHEIGHT;

                if (d < r && cansee(sj->x, sj->y, sj->z-(8<<8), sj->sectnum, s->x, s->y, s->z-(12<<8), s->sectnum))
                {
                    hittype[j].ang = getangle(sj->x-s->x,sj->y-s->y);

                    if (s->picnum == RPG && sj->extra > 0)
                        hittype[j].picnum = RPG;
                    else if (checkspriteflags(i,SPRITE_FLAG_PROJECTILE) && hittype[i].projectile.workslike & PROJECTILE_FLAG_RADIUS_PICNUM && sj->extra > 0)
                        hittype[j].picnum = s->picnum;
                    else
                    {
                        if (s->picnum == SHRINKSPARK)
                            hittype[j].picnum = SHRINKSPARK;
                        else hittype[j].picnum = RADIUSEXPLOSION;
                    }

                    if (s->picnum != SHRINKSPARK)
                    {
                        if (d < r/3)
                        {
                            if (hp4 == hp3) hp4++;
                            hittype[j].extra = hp3 + (TRAND%(hp4-hp3));
                        }
                        else if (d < 2*r/3)
                        {
                            if (hp3 == hp2) hp3++;
                            hittype[j].extra = hp2 + (TRAND%(hp3-hp2));
                        }
                        else if (d < r)
                        {
                            if (hp2 == hp1) hp2++;
                            hittype[j].extra = hp1 + (TRAND%(hp2-hp1));
                        }

                        if (sprite[j].picnum != TANK && sprite[j].picnum != ROTATEGUN && sprite[j].picnum != RECON && sprite[j].picnum != BOSS1 && sprite[j].picnum != BOSS2 && sprite[j].picnum != BOSS3 && sprite[j].picnum != BOSS4)
                        {
                            if (sj->xvel < 0) sj->xvel = 0;
                            sj->xvel += (s->extra<<2);
                        }

                        if (sj->picnum == PODFEM1 || sj->picnum == FEM1 ||
                                sj->picnum == FEM2 || sj->picnum == FEM3 ||
                                sj->picnum == FEM4 || sj->picnum == FEM5 ||
                                sj->picnum == FEM6 || sj->picnum == FEM7 ||
                                sj->picnum == FEM8 || sj->picnum == FEM9 ||
                                sj->picnum == FEM10 || sj->picnum == STATUE ||
                                sj->picnum == STATUEFLASH || sj->picnum == SPACEMARINE || sj->picnum == QUEBALL || sj->picnum == STRIPEBALL)
                            checkhitsprite(j, i);
                    }
                    else if (s->extra == 0) hittype[j].extra = 0;

                    if (sj->picnum != RADIUSEXPLOSION &&
                            s->owner >= 0 && sprite[s->owner].statnum < MAXSTATUS)
                    {
                        if (sj->picnum == APLAYER)
                        {
                            p = sj->yvel;
                            if (g_player[p].ps->newowner >= 0)
                            {
                                g_player[p].ps->newowner = -1;
                                g_player[p].ps->posx = g_player[p].ps->oposx;
                                g_player[p].ps->posy = g_player[p].ps->oposy;
                                g_player[p].ps->posz = g_player[p].ps->oposz;
                                g_player[p].ps->ang = g_player[p].ps->oang;
                                updatesector(g_player[p].ps->posx,g_player[p].ps->posy,&g_player[p].ps->cursectnum);
                                setpal(g_player[p].ps);

                                k = headspritestat[1];
                                while (k >= 0)
                                {
                                    if (sprite[k].picnum==CAMERA1)
                                        sprite[k].yvel = 0;
                                    k = nextspritestat[k];
                                }
                            }
                        }
                        hittype[j].owner = s->owner;
                    }
                }
            }
BOLT:
            j = nextj;
        }
    }
}

int movesprite(int spritenum, int xchange, int ychange, int zchange, unsigned int cliptype)
{
    int daz, oldx, oldy;
    int retval;
    short dasectnum, cd;
    int bg = badguy(&sprite[spritenum]);

    if (sprite[spritenum].statnum == 5 || (bg && sprite[spritenum].xrepeat < 4))
    {
        sprite[spritenum].x += (xchange*TICSPERFRAME)>>2;
        sprite[spritenum].y += (ychange*TICSPERFRAME)>>2;
        sprite[spritenum].z += (zchange*TICSPERFRAME)>>2;
        if (bg)
            setsprite(spritenum,sprite[spritenum].x,sprite[spritenum].y,sprite[spritenum].z);
        return 0;
    }

    dasectnum = sprite[spritenum].sectnum;

    daz = sprite[spritenum].z - ((tilesizy[sprite[spritenum].picnum]*sprite[spritenum].yrepeat)<<1);

    if (bg)
    {
        oldx = sprite[spritenum].x;
        oldy = sprite[spritenum].y;

        if (sprite[spritenum].xrepeat > 60)
            retval = clipmove(&sprite[spritenum].x,&sprite[spritenum].y,&daz,&dasectnum,((xchange*TICSPERFRAME)<<11),((ychange*TICSPERFRAME)<<11),1024L,(4<<8),(4<<8),cliptype);
        else
        {
            if (sprite[spritenum].picnum == LIZMAN)
                cd = 292L;
            else if ((actortype[sprite[spritenum].picnum]&3))
                cd = sprite[spritenum].clipdist<<2;
            else
                cd = 192L;

            retval = clipmove(&sprite[spritenum].x,&sprite[spritenum].y,&daz,&dasectnum,((xchange*TICSPERFRAME)<<11),((ychange*TICSPERFRAME)<<11),cd,(4<<8),(4<<8),cliptype);
        }

        if (dasectnum < 0 || (dasectnum >= 0 &&
                              ((hittype[spritenum].actorstayput >= 0 && hittype[spritenum].actorstayput != dasectnum) ||
                               ((sprite[spritenum].picnum == BOSS2) && sprite[spritenum].pal == 0 && sector[dasectnum].lotag != 3) ||
                               ((sprite[spritenum].picnum == BOSS1 || sprite[spritenum].picnum == BOSS2) && sector[dasectnum].lotag == 1) ||
                               (sector[dasectnum].lotag == 1 && (sprite[spritenum].picnum == LIZMAN || (sprite[spritenum].picnum == LIZTROOP && sprite[spritenum].zvel == 0)))
                              ))
           )
        {
            sprite[spritenum].x = oldx;
            sprite[spritenum].y = oldy;
            if (sector[dasectnum].lotag == 1 && sprite[spritenum].picnum == LIZMAN)
                sprite[spritenum].ang = (TRAND&2047);
            else if ((hittype[spritenum].temp_data[0]&3) == 1 && sprite[spritenum].picnum != COMMANDER)
                sprite[spritenum].ang = (TRAND&2047);
            setsprite(spritenum,oldx,oldy,sprite[spritenum].z);
            if (dasectnum < 0) dasectnum = 0;
            return (16384+dasectnum);
        }
        if ((retval&49152) >= 32768 && (hittype[spritenum].cgg==0)) sprite[spritenum].ang += 768;
    }
    else
    {
        if (sprite[spritenum].statnum == 4)
            retval =
                clipmove(&sprite[spritenum].x,&sprite[spritenum].y,&daz,&dasectnum,((xchange*TICSPERFRAME)<<11),((ychange*TICSPERFRAME)<<11),8L,(4<<8),(4<<8),cliptype);
        else
            retval =
                clipmove(&sprite[spritenum].x,&sprite[spritenum].y,&daz,&dasectnum,((xchange*TICSPERFRAME)<<11),((ychange*TICSPERFRAME)<<11),(int)(sprite[spritenum].clipdist<<2),(4<<8),(4<<8),cliptype);
    }

    if (dasectnum >= 0)
        if ((dasectnum != sprite[spritenum].sectnum))
            changespritesect(spritenum,dasectnum);
    daz = sprite[spritenum].z + ((zchange*TICSPERFRAME)>>3);
    if ((daz > hittype[spritenum].ceilingz) && (daz <= hittype[spritenum].floorz))
        sprite[spritenum].z = daz;
    else
        if (retval == 0)
            return(16384+dasectnum);

    return(retval);
}

int ssp(int i,unsigned int cliptype) //The set sprite function
{
    spritetype *s= &sprite[i];
    int movetype;

    movetype = movesprite(i,
                          (s->xvel*(sintable[(s->ang+512)&2047]))>>14,
                          (s->xvel*(sintable[s->ang&2047]))>>14,s->zvel,
                          cliptype);

    return (movetype==0);
}

void insertspriteq(int i)
{
    if (spriteqamount > 0)
    {
        if (spriteq[spriteqloc] >= 0)
            sprite[spriteq[spriteqloc]].xrepeat = 0;
        spriteq[spriteqloc] = i;
        spriteqloc = (spriteqloc+1)%spriteqamount;
    }
    //    else sprite[i].xrepeat = sprite[i].yrepeat = 0;
    else deletesprite(i);
}

void lotsofmoney(int sp, int n)
{
    int i=n ,j;
    spritetype *s = &sprite[sp];

    for (;i>0;i--)
    {
        j = EGS(s->sectnum,s->x,s->y,s->z-(TRAND%(47<<8)),MONEY,-32,8,8,TRAND&2047,0,0,sp,5);
        sprite[j].cstat = TRAND&12;
    }
}

void lotsofmail(int sp, int n)
{
    int i=n ,j;
    spritetype *s = &sprite[sp];

    for (;i>0;i--)
    {
        j = EGS(s->sectnum,s->x,s->y,s->z-(TRAND%(47<<8)),MAIL,-32,8,8,TRAND&2047,0,0,sp,5);
        sprite[j].cstat = TRAND&12;
    }
}

void lotsofpaper(int sp, int n)
{
    int i=n ,j;
    spritetype *s = &sprite[sp];

    for (;i>0;i--)
    {
        j = EGS(s->sectnum,s->x,s->y,s->z-(TRAND%(47<<8)),PAPER,-32,8,8,TRAND&2047,0,0,sp,5);
        sprite[j].cstat = TRAND&12;
    }
}

void guts(int sp, int gtype, int n, int p)
{
    int gutz,floorz;
    int i,a,j,sx,sy,pal;
    spritetype *s = &sprite[sp];

    if (badguy(s) && s->xrepeat < 16)
        sx = sy = 8;
    else sx = sy = 32;

    gutz = s->z-(8<<8);
    floorz = getflorzofslope(s->sectnum,s->x,s->y);

    if (gutz > (floorz-(8<<8)))
        gutz = floorz-(8<<8);

    if (s->picnum == COMMANDER)
        gutz -= (24<<8);

    if (badguy(s) && s->pal == 6)
        pal = 6;
    else pal = 0;

    for (j=0;j<n;j++)
    {
        a = TRAND&2047;
        i = EGS(s->sectnum,s->x+(TRAND&255)-128,s->y+(TRAND&255)-128,gutz-(TRAND&8191),gtype,-32,sx,sy,a,48+(TRAND&31),-512-(TRAND&2047),sp,5);
        if (PN == JIBS2)
        {
            sprite[i].xrepeat >>= 2;
            sprite[i].yrepeat >>= 2;
        }
        if (pal == 6)
            sprite[i].pal = 6;
    }
}

void gutsdir(int sp, int gtype, int n, int p)
{
    int gutz,floorz;
    int i,a,j,sx,sy;
    spritetype *s = &sprite[sp];

    if (badguy(s) && s->xrepeat < 16)
        sx = sy = 8;
    else sx = sy = 32;

    gutz = s->z-(8<<8);
    floorz = getflorzofslope(s->sectnum,s->x,s->y);

    if (gutz > (floorz-(8<<8)))
        gutz = floorz-(8<<8);

    if (s->picnum == COMMANDER)
        gutz -= (24<<8);

    for (j=0;j<n;j++)
    {
        a = TRAND&2047;
        i = EGS(s->sectnum,s->x,s->y,gutz,gtype,-32,sx,sy,a,256+(TRAND&127),-512-(TRAND&2047),sp,5);
    }
}

void setsectinterpolate(int i)
{
    int k, j = sector[SECT].wallptr,endwall = j+sector[SECT].wallnum;

    for (;j<endwall;j++)
    {
        setinterpolation(&wall[j].x);
        setinterpolation(&wall[j].y);
        k = wall[j].nextwall;
        if (k >= 0)
        {
            setinterpolation(&wall[k].x);
            setinterpolation(&wall[k].y);
            k = wall[k].point2;
            setinterpolation(&wall[k].x);
            setinterpolation(&wall[k].y);
        }
    }
}

void clearsectinterpolate(int i)
{
    int j = sector[SECT].wallptr,endwall = j+sector[SECT].wallnum;

    for (;j<endwall;j++)
    {
        stopinterpolation(&wall[j].x);
        stopinterpolation(&wall[j].y);
        if (wall[j].nextwall >= 0)
        {
            stopinterpolation(&wall[wall[j].nextwall].x);
            stopinterpolation(&wall[wall[j].nextwall].y);
        }
    }
}

static void ms(int i)
{
    //T1,T2 and T3 are used for all the sector moving stuff!!!

    int tx,ty;
    spritetype *s = &sprite[i];
    int j = T2, k = T3;

    s->x += (s->xvel*(sintable[(s->ang+512)&2047]))>>14;
    s->y += (s->xvel*(sintable[s->ang&2047]))>>14;

    {
        int x = sector[s->sectnum].wallptr, endwall = x+sector[s->sectnum].wallnum;

        for (;x<endwall;x++)
        {
            rotatepoint(0,0,msx[j],msy[j],k&2047,&tx,&ty);
            dragpoint(x,s->x+tx,s->y+ty);

            j++;
        }
    }
}

static void movefta(void)
{
    int x, px, py, sx, sy;
    int i = headspritestat[2], j, p, nexti;
    short psect, ssect;
    spritetype *s;

    while (i >= 0)
    {
        nexti = nextspritestat[i];

        s = &sprite[i];
        p = findplayer(s,&x);

        ssect = psect = s->sectnum;

        if (sprite[g_player[p].ps->i].extra > 0)
        {
            if (x < 30000)
            {
                hittype[i].timetosleep++;
                if (hittype[i].timetosleep >= (x>>8))
                {
                    if (badguy(s))
                    {
                        px = g_player[p].ps->oposx+64-(TRAND&127);
                        py = g_player[p].ps->oposy+64-(TRAND&127);
                        updatesector(px,py,&psect);
                        if (psect == -1)
                        {
                            i = nexti;
                            continue;
                        }
                        sx = s->x+64-(TRAND&127);
                        sy = s->y+64-(TRAND&127);
                        updatesector(px,py,&ssect);
                        if (ssect == -1)
                        {
                            i = nexti;
                            continue;
                        }
                        j = cansee(sx,sy,s->z-(TRAND%(52<<8)),s->sectnum,px,py,g_player[p].ps->oposz-(TRAND%(32<<8)),g_player[p].ps->cursectnum);
                    }
                    else
                        j = cansee(s->x,s->y,s->z-((TRAND&31)<<8),s->sectnum,g_player[p].ps->oposx,g_player[p].ps->oposy,g_player[p].ps->oposz-((TRAND&31)<<8),g_player[p].ps->cursectnum);

                    //             j = 1;

                    if (j) switch (dynamictostatic[s->picnum])
                        {
                        case RUBBERCAN__STATIC:
                        case EXPLODINGBARREL__STATIC:
                        case WOODENHORSE__STATIC:
                        case HORSEONSIDE__STATIC:
                        case CANWITHSOMETHING__STATIC:
                        case CANWITHSOMETHING2__STATIC:
                        case CANWITHSOMETHING3__STATIC:
                        case CANWITHSOMETHING4__STATIC:
                        case FIREBARREL__STATIC:
                        case FIREVASE__STATIC:
                        case NUKEBARREL__STATIC:
                        case NUKEBARRELDENTED__STATIC:
                        case NUKEBARRELLEAKED__STATIC:
                        case TRIPBOMB__STATIC:
                            if (sector[s->sectnum].ceilingstat&1 && checkspriteflags(j,SPRITE_FLAG_NOSHADE) == 0)
                                s->shade = sector[s->sectnum].ceilingshade;
                            else s->shade = sector[s->sectnum].floorshade;

                            hittype[i].timetosleep = 0;
                            changespritestat(i,6);
                            break;
                        default:
                            hittype[i].timetosleep = 0;
                            check_fta_sounds(i);
                            changespritestat(i,1);
                            break;
                        }
                    else hittype[i].timetosleep = 0;
                }
            }
            if (badguy(s) && checkspriteflags(i,SPRITE_FLAG_NOSHADE) == 0)
            {
                if (sector[s->sectnum].ceilingstat&1)
                    s->shade = sector[s->sectnum].ceilingshade;
                else s->shade = sector[s->sectnum].floorshade;
            }
        }
        i = nexti;
    }
}

int ifhitsectors(int sectnum)
{
    int i = headspritestat[5];
    while (i >= 0)
    {
        if (PN == EXPLOSION2 && sectnum == SECT)
            return i;
        i = nextspritestat[i];
    }
    return -1;
}

int ifhitbyweapon(int sn)
{
    int j,p;
    spritetype *npc;

    if (hittype[sn].extra >= 0)
    {
        if (sprite[sn].extra >= 0)
        {
            npc = &sprite[sn];

            if (npc->picnum == APLAYER)
            {
                if (ud.god && hittype[sn].picnum != SHRINKSPARK) return -1;

                p = npc->yvel;
                j = hittype[sn].owner;

                if (j >= 0 &&
                        sprite[j].picnum == APLAYER &&
                        (gametype_flags[ud.coop] & GAMETYPE_FLAG_PLAYERSFRIENDLY) &&
                        ud.ffire == 0)
                    return -1;

                if (j >= 0 &&
                        sprite[j].picnum == APLAYER &&
                        (gametype_flags[ud.coop] & GAMETYPE_FLAG_TDM) &&
                        g_player[p].ps->team == g_player[sprite[j].yvel].ps->team &&
                        ud.ffire == 0)
                    return -1;

                npc->extra -= hittype[sn].extra;

                if (j >= 0)
                {
                    if (npc->extra <= 0 && hittype[sn].picnum != FREEZEBLAST)
                    {
                        npc->extra = 0;

                        g_player[p].ps->wackedbyactor = j;

                        if (sprite[hittype[sn].owner].picnum == APLAYER && p != sprite[hittype[sn].owner].yvel)
                            g_player[p].ps->frag_ps = sprite[j].yvel;

                        hittype[sn].owner = g_player[p].ps->i;
                    }
                }

                if (checkspriteflagsp(hittype[sn].picnum,SPRITE_FLAG_PROJECTILE) && (hittype[sn].projectile.workslike & PROJECTILE_FLAG_RPG))
                {
                    g_player[p].ps->posxv +=
                        hittype[sn].extra*(sintable[(hittype[sn].ang+512)&2047])<<2;
                    g_player[p].ps->posyv +=
                        hittype[sn].extra*(sintable[hittype[sn].ang&2047])<<2;
                }
                else if (checkspriteflagsp(hittype[sn].picnum,SPRITE_FLAG_PROJECTILE))
                {
                    g_player[p].ps->posxv +=
                        hittype[sn].extra*(sintable[(hittype[sn].ang+512)&2047])<<1;
                    g_player[p].ps->posyv +=
                        hittype[sn].extra*(sintable[hittype[sn].ang&2047])<<1;
                }

                switch (dynamictostatic[hittype[sn].picnum])
                {
                case RADIUSEXPLOSION__STATIC:
                case RPG__STATIC:
                case HYDRENT__STATIC:
                case HEAVYHBOMB__STATIC:
                case SEENINE__STATIC:
                case OOZFILTER__STATIC:
                case EXPLODINGBARREL__STATIC:
                    g_player[p].ps->posxv +=
                        hittype[sn].extra*(sintable[(hittype[sn].ang+512)&2047])<<2;
                    g_player[p].ps->posyv +=
                        hittype[sn].extra*(sintable[hittype[sn].ang&2047])<<2;
                    break;
                default:
                    g_player[p].ps->posxv +=
                        hittype[sn].extra*(sintable[(hittype[sn].ang+512)&2047])<<1;
                    g_player[p].ps->posyv +=
                        hittype[sn].extra*(sintable[hittype[sn].ang&2047])<<1;
                    break;
                }
            }
            else
            {
                if (hittype[sn].extra == 0)
                    if (hittype[sn].picnum == SHRINKSPARK && npc->xrepeat < 24)
                        return -1;

                npc->extra -= hittype[sn].extra;
                if (npc->picnum != RECON && npc->owner >= 0 && sprite[npc->owner].statnum < MAXSTATUS)
                    npc->owner = hittype[sn].owner;
            }

            hittype[sn].extra = -1;
            return hittype[sn].picnum;
        }
    }

    hittype[sn].extra = -1;
    return -1;
}

void movecyclers(void)
{
    int q, j, x, t, s, cshade;
    short *c;
    walltype *wal;

    for (q=numcyclers-1;q>=0;q--)
    {

        c = &cyclers[q][0];
        s = c[0];

        t = c[3];
        j = t+(sintable[c[1]&2047]>>10);
        cshade = c[2];

        if (j < cshade) j = cshade;
        else if (j > t)  j = t;

        c[1] += sector[s].extra;
        if (c[5])
        {
            wal = &wall[sector[s].wallptr];
            for (x = sector[s].wallnum;x>0;x--,wal++)
                if (wal->hitag != 1)
                {
                    wal->shade = j;

                    if ((wal->cstat&2) && wal->nextwall >= 0)
                        wall[wal->nextwall].shade = j;

                }
            sector[s].floorshade = sector[s].ceilingshade = j;
        }
    }
}

void movedummyplayers(void)
{
    int i = headspritestat[13], p, nexti;

    while (i >= 0)
    {
        nexti = nextspritestat[i];

        p = sprite[OW].yvel;

        if (g_player[p].ps->on_crane >= 0 || sector[g_player[p].ps->cursectnum].lotag != 1 || sprite[g_player[p].ps->i].extra <= 0)
        {
            g_player[p].ps->dummyplayersprite = -1;
            KILLIT(i);
        }
        else
        {
            if (g_player[p].ps->on_ground && g_player[p].ps->on_warping_sector == 1 && sector[g_player[p].ps->cursectnum].lotag == 1)
            {
                CS = 257;
                SZ = sector[SECT].ceilingz+(27<<8);
                SA = g_player[p].ps->ang;
                if (T1 == 8)
                    T1 = 0;
                else T1++;
            }
            else
            {
                if (sector[SECT].lotag != 2) SZ = sector[SECT].floorz;
                CS = (short) 32768;
            }
        }

        SX += (g_player[p].ps->posx-g_player[p].ps->oposx);
        SY += (g_player[p].ps->posy-g_player[p].ps->oposy);
        setsprite(i,SX,SY,SZ);

BOLT:

        i = nexti;
    }
}

int otherp;

static void moveplayers(void) //Players
{
    int i = headspritestat[10], nexti;
    int otherx;
    spritetype *s;
    player_struct *p;

    while (i >= 0)
    {
        nexti = nextspritestat[i];

        s = &sprite[i];
        p = g_player[s->yvel].ps;
        if (s->owner >= 0)
        {
            if (p->newowner >= 0)  //Looking thru the camera
            {
                s->x = p->oposx;
                s->y = p->oposy;
                hittype[i].bposz = s->z = p->oposz+PHEIGHT;
                s->ang = p->oang;
                setsprite(i,s->x,s->y,s->z);
            }
            else
            {
                if (ud.multimode > 1)
                    otherp = findotherplayer(s->yvel,&otherx);
                else
                {
                    otherp = s->yvel;
                    otherx = 0;
                }

                execute(i,s->yvel,otherx);

                if (ud.multimode > 1)
                    if (sprite[g_player[otherp].ps->i].extra > 0)
                    {
                        if (s->yrepeat > 32 && sprite[g_player[otherp].ps->i].yrepeat < 32)
                        {
                            if (otherx < 1400 && p->knee_incs == 0)
                            {
                                p->knee_incs = 1;
                                p->weapon_pos = -1;
                                p->actorsqu = g_player[otherp].ps->i;
                            }
                        }
                    }
                if (ud.god)
                {
                    s->extra = p->max_player_health;
                    s->cstat = 257;
                    p->jetpack_amount =     1599;
                }


                if (s->extra > 0)
                {
                    hittype[i].owner = i;

                    if (ud.god == 0)
                        if (ceilingspace(s->sectnum) || floorspace(s->sectnum))
                            quickkill(p);
                }
                else
                {

                    p->posx = s->x;
                    p->posy = s->y;
                    p->posz = s->z-(20<<8);

                    p->newowner = -1;

                    if (p->wackedbyactor >= 0 && sprite[p->wackedbyactor].statnum < MAXSTATUS)
                    {
                        p->ang += getincangle(p->ang,getangle(sprite[p->wackedbyactor].x-p->posx,sprite[p->wackedbyactor].y-p->posy))>>1;
                        p->ang &= 2047;
                    }

                }
                s->ang = p->ang;
            }
        }
        else
        {
            if (p->holoduke_on == -1)
                KILLIT(i);

            hittype[i].bposx = s->x;
            hittype[i].bposy = s->y;
            hittype[i].bposz = s->z;

            s->cstat = 0;

            if (s->xrepeat < 42)
            {
                s->xrepeat += 4;
                s->cstat |= 2;
            }
            else s->xrepeat = 42;
            if (s->yrepeat < 36)
                s->yrepeat += 4;
            else
            {
                s->yrepeat = 36;
                if (sector[s->sectnum].lotag != 2)
                    makeitfall(i);
                if (s->zvel == 0 && sector[s->sectnum].lotag == 1)
                    s->z += (32<<8);
            }

            if (s->extra < 8)
            {
                s->xvel = 128;
                s->ang = p->ang;
                s->extra++;
                ssp(i,CLIPMASK0);
            }
            else
            {
                s->ang = 2047-p->ang;
                setsprite(i,s->x,s->y,s->z);
            }
        }

        if (sector[s->sectnum].ceilingstat&1)
            s->shade += (sector[s->sectnum].ceilingshade-s->shade)>>1;
        else
            s->shade += (sector[s->sectnum].floorshade-s->shade)>>1;

BOLT:
        i = nexti;
    }
}

static void movefx(void)
{
    int i = headspritestat[11], j, nexti, p;
    int x, ht;
    spritetype *s;

    while (i >= 0)
    {
        s = &sprite[i];

        nexti = nextspritestat[i];

        switch (dynamictostatic[s->picnum])
        {
        case RESPAWN__STATIC:
            if (sprite[i].extra == 66)
            {
                j = spawn(i,SHT);
                //                    sprite[j].pal = sprite[i].pal;
                KILLIT(i);
            }
            else if (sprite[i].extra > (66-13))
                sprite[i].extra++;
            break;

        case MUSICANDSFX__STATIC:

            ht = s->hitag;

            if (T2 != ud.config.SoundToggle)
            {
                T2 = ud.config.SoundToggle;
                T1 = 0;
            }

            if (s->lotag >= 1000 && s->lotag < 2000)
            {
                x = ldist(&sprite[g_player[screenpeek].ps->i],s);
                if (x < ht && T1 == 0)
                {
                    FX_SetReverb(s->lotag - 1000);
                    T1 = 1;
                }
                if (x >= ht && T1 == 1)
                {
                    FX_SetReverb(0);
                    FX_SetReverbDelay(0);
                    T1 = 0;
                }
            }
            else if (s->lotag < 999 && (unsigned)sector[s->sectnum].lotag < 9 && ud.config.AmbienceToggle && sector[SECT].floorz != sector[SECT].ceilingz)
            {
                if ((g_sounds[s->lotag].m&2))
                {
                    x = dist(&sprite[g_player[screenpeek].ps->i],s);
                    if (x < ht && T1 == 0 && FX_VoiceAvailable(g_sounds[s->lotag].pr-1))
                    {
                        if (numenvsnds == ud.config.NumVoices)
                        {
                            j = headspritestat[11];
                            while (j >= 0)
                            {
                                if (PN == MUSICANDSFX && j != i && sprite[j].lotag < 999 && hittype[j].temp_data[0] == 1 && dist(&sprite[j],&sprite[g_player[screenpeek].ps->i]) > x)
                                {
                                    stopenvsound(sprite[j].lotag,j);
                                    break;
                                }
                                j = nextspritestat[j];
                            }
                            if (j == -1) goto BOLT;
                        }
                        spritesound(s->lotag,i);
                        T1 = 1;
                    }
                    if (x >= ht && T1 == 1)
                    {
                        T1 = 0;
                        stopenvsound(s->lotag,i);
                    }
                }
                if ((g_sounds[s->lotag].m&16))
                {
                    if (T5 > 0) T5--;
                    else for (p=connecthead;p>=0;p=connectpoint2[p])
                            if (p == myconnectindex && g_player[p].ps->cursectnum == s->sectnum)
                            {
                                j = s->lotag+((unsigned)global_random%(s->hitag+1));
                                sound(j);
                                T5 =  26*40 + (global_random%(26*40));
                            }
                }
            }
            break;
        }
BOLT:
        i = nexti;
    }
}

static void movefallers(void)
{
    int i = headspritestat[12], nexti, sect, j;
    spritetype *s;
    int x;

    while (i >= 0)
    {
        nexti = nextspritestat[i];
        s = &sprite[i];

        sect = s->sectnum;

        if (T1 == 0)
        {
            s->z -= (16<<8);
            T2 = s->ang;
            x = s->extra;
            IFHIT
            {
                if (j == FIREEXT || j == RPG || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
                {
                    if (s->extra <= 0)
                    {
                        T1 = 1;
                        j = headspritestat[12];
                        while (j >= 0)
                        {
                            if (sprite[j].hitag == SHT)
                            {
                                hittype[j].temp_data[0] = 1;
                                sprite[j].cstat &= (65535-64);
                                if (sprite[j].picnum == CEILINGSTEAM || sprite[j].picnum == STEAM)
                                    sprite[j].cstat |= 32768;
                            }
                            j = nextspritestat[j];
                        }
                    }
                }
                else
                {
                    hittype[i].extra = 0;
                    s->extra = x;
                }
            }
            s->ang = T2;
            s->z += (16<<8);
        }
        else if (T1 == 1)
        {
            if (s->lotag > 0)
            {
                s->lotag-=3;
                if (s->lotag <= 0)
                {
                    s->xvel = (32+(TRAND&63));
                    s->zvel = -(1024+(TRAND&1023));
                }
            }
            else
            {
                if (s->xvel > 0)
                {
                    s->xvel -= 8;
                    ssp(i,CLIPMASK0);
                }

                if (floorspace(s->sectnum)) x = 0;
                else
                {
                    if (ceilingspace(s->sectnum))
                        x = gc/6;
                    else
                        x = gc;
                }

                if (s->z < (sector[sect].floorz-FOURSLEIGHT))
                {
                    s->zvel += x;
                    if (s->zvel > 6144)
                        s->zvel = 6144;
                    s->z += s->zvel;
                }
                if ((sector[sect].floorz-s->z) < (16<<8))
                {
                    j = 1+(TRAND&7);
                    for (x=0;x<j;x++) RANDOMSCRAP;
                    KILLIT(i);
                }
            }
        }

BOLT:
        i = nexti;
    }
}

static void movestandables(void)
{
    int i = headspritestat[6], j, k, nexti, nextj, p=0, sect, switchpicnum;
    int l=0, x, *t;
    spritetype *s;
    short m;

    while (i >= 0)
    {
        nexti = nextspritestat[i];

        t = &hittype[i].temp_data[0];
        s = &sprite[i];
        sect = s->sectnum;

        if (sect < 0) KILLIT(i);

        hittype[i].bposx = s->x;
        hittype[i].bposy = s->y;
        hittype[i].bposz = s->z;

        IFWITHIN(CRANE,CRANE+3)
        {
            //t[0] = state
            //t[1] = checking sector number

            if (s->xvel) getglobalz(i);

            if (t[0] == 0)   //Waiting to check the sector
            {
                j = headspritesect[t[1]];
                while (j>=0)
                {
                    nextj = nextspritesect[j];
                    switch (sprite[j].statnum)
                    {
                    case 1:
                    case 2:
                    case 6:
                    case 10:
                        s->ang = getangle(msx[t[4]+1]-s->x,msy[t[4]+1]-s->y);
                        setsprite(j,msx[t[4]+1],msy[t[4]+1],sprite[j].z);
                        t[0]++;
                        goto BOLT;
                    }
                    j = nextj;
                }
            }

            else if (t[0]==1)
            {
                if (s->xvel < 184)
                {
                    s->picnum = CRANE+1;
                    s->xvel += 8;
                }
                ssp(i,CLIPMASK0);
                if (sect == t[1])
                    t[0]++;
            }
            else if (t[0]==2 || t[0]==7)
            {
                s->z += (1024+512);

                if (t[0]==2)
                {
                    if ((sector[sect].floorz - s->z) < (64<<8))
                        if (s->picnum > CRANE) s->picnum--;

                    if ((sector[sect].floorz - s->z) < (4096+1024))
                        t[0]++;
                }
                if (t[0]==7)
                {
                    if ((sector[sect].floorz - s->z) < (64<<8))
                    {
                        if (s->picnum > CRANE) s->picnum--;
                        else
                        {
                            if (s->owner==-2)
                            {
                                p = findplayer(s,&x);
                                spritesound(DUKE_GRUNT,g_player[p].ps->i);
                                if (g_player[p].ps->on_crane == i)
                                    g_player[p].ps->on_crane = -1;
                            }
                            t[0]++;
                            s->owner = -1;
                        }
                    }
                }
            }
            else if (t[0]==3)
            {
                s->picnum++;
                if (s->picnum == (CRANE+2))
                {
                    p = checkcursectnums(t[1]);
                    if (p >= 0 && g_player[p].ps->on_ground)
                    {
                        s->owner = -2;
                        g_player[p].ps->on_crane = i;
                        spritesound(DUKE_GRUNT,g_player[p].ps->i);
                        g_player[p].ps->ang = s->ang+1024;
                    }
                    else
                    {
                        j = headspritesect[t[1]];
                        while (j>=0)
                        {
                            switch (sprite[j].statnum)
                            {
                            case 1:
                            case 6:
                                s->owner = j;
                                break;
                            }
                            j = nextspritesect[j];
                        }
                    }

                    t[0]++;//Grabbed the sprite
                    t[2]=0;
                    goto BOLT;
                }
            }
            else if (t[0]==4) //Delay before going up
            {
                t[2]++;
                if (t[2] > 10)
                    t[0]++;
            }
            else if (t[0]==5 || t[0] == 8)
            {
                if (t[0]==8 && s->picnum < (CRANE+2))
                    if ((sector[sect].floorz-s->z) > 8192)
                        s->picnum++;

                if (s->z < msx[t[4]+2])
                {
                    t[0]++;
                    s->xvel = 0;
                }
                else
                    s->z -= (1024+512);
            }
            else if (t[0]==6)
            {
                if (s->xvel < 192)
                    s->xvel += 8;
                s->ang = getangle(msx[t[4]]-s->x,msy[t[4]]-s->y);
                ssp(i,CLIPMASK0);
                if (((s->x-msx[t[4]])*(s->x-msx[t[4]])+(s->y-msy[t[4]])*(s->y-msy[t[4]])) < (128*128))
                    t[0]++;
            }

            else if (t[0]==9)
                t[0] = 0;

            setsprite(msy[t[4]+2],s->x,s->y,s->z-(34<<8));

            if (s->owner != -1)
            {
                p = findplayer(s,&x);

                IFHIT
                {
                    if (s->owner == -2)
                        if (g_player[p].ps->on_crane == i)
                            g_player[p].ps->on_crane = -1;
                    s->owner = -1;
                    s->picnum = CRANE;
                    goto BOLT;
                }

                if (s->owner >= 0)
                {
                    setsprite(s->owner,s->x,s->y,s->z);

                    hittype[s->owner].bposx = s->x;
                    hittype[s->owner].bposy = s->y;
                    hittype[s->owner].bposz = s->z;

                    s->zvel = 0;
                }
                else if (s->owner == -2)
                {
                    g_player[p].ps->oposx = g_player[p].ps->posx = s->x-(sintable[(g_player[p].ps->ang+512)&2047]>>6);
                    g_player[p].ps->oposy = g_player[p].ps->posy = s->y-(sintable[g_player[p].ps->ang&2047]>>6);
                    g_player[p].ps->oposz = g_player[p].ps->posz = s->z+(2<<8);
                    setsprite(g_player[p].ps->i,g_player[p].ps->posx,g_player[p].ps->posy,g_player[p].ps->posz);
                    g_player[p].ps->cursectnum = sprite[g_player[p].ps->i].sectnum;
                }
            }

            goto BOLT;
        }

        IFWITHIN(WATERFOUNTAIN,WATERFOUNTAIN+3)
        {
            if (t[0] > 0)
            {
                if (t[0] < 20)
                {
                    t[0]++;

                    s->picnum++;

                    if (s->picnum == (WATERFOUNTAIN+3))
                        s->picnum = WATERFOUNTAIN+1;
                }
                else
                {
                    p = findplayer(s,&x);

                    if (x > 512)
                    {
                        t[0] = 0;
                        s->picnum = WATERFOUNTAIN;
                    }
                    else t[0] = 1;
                }
            }
            goto BOLT;
        }

        if (AFLAMABLE(s->picnum))
        {
            if (T1 == 1)
            {
                T2++;
                if ((T2&3) > 0) goto BOLT;

                if (s->picnum == TIRE && T2 == 32)
                {
                    s->cstat = 0;
                    j = spawn(i,BLOODPOOL);
                    sprite[j].shade = 127;
                }
                else
                {
                    if (s->shade < 64) s->shade++;
                    else KILLIT(i);
                }

                j = s->xrepeat-(TRAND&7);
                if (j < 10)
                {
                    KILLIT(i);
                }

                s->xrepeat = j;

                j = s->yrepeat-(TRAND&7);
                if (j < 4)
                {
                    KILLIT(i);
                }
                s->yrepeat = j;
            }
            if (s->picnum == BOX)
            {
                makeitfall(i);
                hittype[i].ceilingz = sector[s->sectnum].ceilingz;
            }
            goto BOLT;
        }

        if (s->picnum == TRIPBOMB)
        {
            //            int lTripBombControl=GetGameVar("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, -1, -1);
            //            if(lTripBombControl & TRIPBOMB_TIMER)
            if (hittype[i].temp_data[6] == 1)
            {

                if (hittype[i].temp_data[7] >= 1)
                {
                    hittype[i].temp_data[7]--;
                }

                if (hittype[i].temp_data[7] <= 0)
                {
                    //                    s->extra = *actorscrptr[s->picnum];
                    T3=16;
                    hittype[i].temp_data[6]=3;
                    spritesound(LASERTRIP_ARMING,i);
                }
                // we're on a timer....
            }
            if (T3 > 0 && hittype[i].temp_data[6] == 3)
            {
                T3--;
                if (T3 == 8)
                {
                    spritesound(LASERTRIP_EXPLODE,i);
                    for (j=0;j<5;j++) RANDOMSCRAP;
                    x = s->extra;
                    hitradius(i, tripbombblastradius, x>>2,x>>1,x-(x>>2),x);

                    j = spawn(i,EXPLOSION2);
                    sprite[j].ang = s->ang;
                    sprite[j].xvel = 348;
                    ssp(j,CLIPMASK0);

                    j = headspritestat[5];
                    while (j >= 0)
                    {
                        if (sprite[j].picnum == LASERLINE && s->hitag == sprite[j].hitag)
                            sprite[j].xrepeat = sprite[j].yrepeat = 0;
                        j = nextspritestat[j];
                    }
                    KILLIT(i);
                }
                goto BOLT;
            }
            else
            {
                x = s->extra;
                s->extra = 1;
                l = s->ang;
                IFHIT { hittype[i].temp_data[6] = 3;
                        T3 = 16;
                      }
                s->extra = x;
                s->ang = l;
            }

            if (T1 < 32)
            {
                p = findplayer(s,&x);
                if (x > 768) T1++;
                else if (T1 > 16) T1++;
            }
            if (T1 == 32)
            {
                l = s->ang;
                s->ang = T6;

                T4 = s->x;
                T5 = s->y;
                s->x += sintable[(T6+512)&2047]>>9;
                s->y += sintable[(T6)&2047]>>9;
                s->z -= (3<<8);
                setsprite(i,s->x,s->y,s->z);

                x = hitasprite(i,&m);

                hittype[i].lastvx = x;

                s->ang = l;

                k = 0;

                //                if(lTripBombControl & TRIPBOMB_TRIPWIRE)
                if (hittype[i].temp_data[6] != 1)
                {
                    // we're on a trip wire

                    while (x > 0)
                    {
                        j = spawn(i,LASERLINE);
                        setsprite(j,sprite[j].x,sprite[j].y,sprite[j].z);
                        sprite[j].hitag = s->hitag;
                        hittype[j].temp_data[1] = sprite[j].z;

                        s->x += sintable[(T6+512)&2047]>>4;
                        s->y += sintable[(T6)&2047]>>4;

                        if (x < 1024)
                        {
                            sprite[j].xrepeat = x>>5;
                            break;
                        }
                        x -= 1024;
                    }
                }
                T1++;
                s->x = T4;
                s->y = T5;
                s->z += (3<<8);
                setsprite(i,s->x,s->y,s->z);
                T4 = 0;
                //                if( m >= 0 && lTripBombControl & TRIPBOMB_TRIPWIRE)
                if (m >= 0 && hittype[i].temp_data[6] != 1)
                {
                    hittype[i].temp_data[6] = 3;
                    T3 = 13;
                    spritesound(LASERTRIP_ARMING,i);
                }
                else T3 = 0;
            }
            if (T1 == 33)
            {
                T2++;


                T4 = s->x;
                T5 = s->y;
                s->x += sintable[(T6+512)&2047]>>9;
                s->y += sintable[(T6)&2047]>>9;
                s->z -= (3<<8);
                setsprite(i,s->x,s->y,s->z);

                x = hitasprite(i,&m);

                s->x = T4;
                s->y = T5;
                s->z += (3<<8);
                setsprite(i,s->x,s->y,s->z);

                //                if( hittype[i].lastvx != x && lTripBombControl & TRIPBOMB_TRIPWIRE)
                if (hittype[i].lastvx != x && hittype[i].temp_data[6] != 1)
                {
                    hittype[i].temp_data[6] = 3;
                    T3 = 13;
                    spritesound(LASERTRIP_ARMING,i);
                }
            }
            goto BOLT;
        }


        if (s->picnum >= CRACK1 && s->picnum <= CRACK4)
        {
            if (s->hitag > 0)
            {
                t[0] = s->cstat;
                t[1] = s->ang;
                j = ifhitbyweapon(i);
                if (j == FIREEXT || j == RPG || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
                {
                    j = headspritestat[6];
                    while (j >= 0)
                    {
                        if (s->hitag == sprite[j].hitag && (sprite[j].picnum == OOZFILTER || sprite[j].picnum == SEENINE))
                            if (sprite[j].shade != -32)
                                sprite[j].shade = -32;
                        j = nextspritestat[j];
                    }

                    goto DETONATE;
                }
                else
                {
                    s->cstat = t[0];
                    s->ang = t[1];
                    s->extra = 0;
                }
            }
            goto BOLT;
        }

        if (s->picnum == FIREEXT)
        {
            j = ifhitbyweapon(i);
            if (j == -1) goto BOLT;

            for (k=0;k<16;k++)
            {
                j = EGS(SECT,SX,SY,SZ-(TRAND%(48<<8)),SCRAP3+(TRAND&3),-8,48,48,TRAND&2047,(TRAND&63)+64,-(TRAND&4095)-(sprite[i].zvel>>2),i,5);
                sprite[j].pal = 2;
            }

            spawn(i,EXPLOSION2);
            spritesound(PIPEBOMB_EXPLODE,i);
            spritesound(GLASS_HEAVYBREAK,i);

            if (s->hitag > 0)
            {
                j = headspritestat[6];
                while (j >= 0)
                {
                    if (s->hitag == sprite[j].hitag && (sprite[j].picnum == OOZFILTER || sprite[j].picnum == SEENINE))
                        if (sprite[j].shade != -32)
                            sprite[j].shade = -32;
                    j = nextspritestat[j];
                }

                x = s->extra;
                spawn(i,EXPLOSION2);
                hitradius(i, pipebombblastradius,x>>2, x-(x>>1),x-(x>>2), x);
                spritesound(PIPEBOMB_EXPLODE,i);

                goto DETONATE;
            }
            else
            {
                hitradius(i,seenineblastradius,10,15,20,25);
                KILLIT(i);
            }
            goto BOLT;
        }

        if (s->picnum == OOZFILTER || s->picnum == SEENINE || s->picnum == SEENINEDEAD || s->picnum == (SEENINEDEAD+1))
        {
            if (s->shade != -32 && s->shade != -33)
            {
                if (s->xrepeat)
                    j = (ifhitbyweapon(i) >= 0);
                else
                    j = 0;

                if (j || s->shade == -31)
                {
                    if (j) s->lotag = 0;

                    t[3] = 1;

                    j = headspritestat[6];
                    while (j >= 0)
                    {
                        if (s->hitag == sprite[j].hitag && (sprite[j].picnum == SEENINE || sprite[j].picnum == OOZFILTER))
                            sprite[j].shade = -32;
                        j = nextspritestat[j];
                    }
                }
            }
            else
            {
                if (s->shade == -32)
                {
                    if (s->lotag > 0)
                    {
                        s->lotag-=3;
                        if (s->lotag <= 0) s->lotag = -99;
                    }
                    else
                        s->shade = -33;
                }
                else
                {
                    if (s->xrepeat > 0)
                    {
                        T3++;
                        if (T3 == 3)
                        {
                            if (s->picnum == OOZFILTER)
                            {
                                T3 = 0;
                                goto DETONATE;
                            }
                            if (s->picnum != (SEENINEDEAD+1))
                            {
                                T3 = 0;

                                if (s->picnum == SEENINEDEAD) s->picnum++;
                                else if (s->picnum == SEENINE)
                                    s->picnum = SEENINEDEAD;
                            }
                            else goto DETONATE;
                        }
                        goto BOLT;
                    }

DETONATE:

                    earthquaketime = 16;

                    j = headspritestat[3];
                    while (j >= 0)
                    {
                        if (s->hitag == sprite[j].hitag)
                        {
                            if (sprite[j].lotag == 13)
                            {
                                if (hittype[j].temp_data[2] == 0)
                                    hittype[j].temp_data[2] = 1;
                            }
                            else if (sprite[j].lotag == 8)
                                hittype[j].temp_data[4] = 1;
                            else if (sprite[j].lotag == 18)
                            {
                                if (hittype[j].temp_data[0] == 0)
                                    hittype[j].temp_data[0] = 1;
                            }
                            else if (sprite[j].lotag == 21)
                                hittype[j].temp_data[0] = 1;
                        }
                        j = nextspritestat[j];
                    }

                    s->z -= (32<<8);

                    if ((t[3] == 1 && s->xrepeat) || s->lotag == -99)
                    {
                        x = s->extra;
                        spawn(i,EXPLOSION2);
                        hitradius(i,seenineblastradius,x>>2, x-(x>>1),x-(x>>2), x);
                        spritesound(PIPEBOMB_EXPLODE,i);
                    }

                    if (s->xrepeat)
                        for (x=0;x<8;x++) RANDOMSCRAP;

                    KILLIT(i);
                }
            }
            goto BOLT;
        }

        if (s->picnum == MASTERSWITCH)
        {
            if (s->yvel == 1)
            {
                s->hitag--;
                if (s->hitag <= 0)
                {
                    operatesectors(sect,i);

                    j = headspritesect[sect];
                    while (j >= 0)
                    {
                        if (sprite[j].statnum == 3)
                        {
                            switch (sprite[j].lotag)
                            {
                            case 2:
                            case 21:
                            case 31:
                            case 32:
                            case 36:
                                hittype[j].temp_data[0] = 1;
                                break;
                            case 3:
                                hittype[j].temp_data[4] = 1;
                                break;
                            }
                        }
                        else if (sprite[j].statnum == 6)
                        {
                            switch (dynamictostatic[sprite[j].picnum])
                            {
                            case SEENINE__STATIC:
                            case OOZFILTER__STATIC:
                                sprite[j].shade = -31;
                                break;
                            }
                        }
                        j = nextspritesect[j];
                    }
                    KILLIT(i);
                }
            }
            goto BOLT;
        }
        switchpicnum = s->picnum;
        if ((s->picnum > SIDEBOLT1) && (s->picnum <= SIDEBOLT1+3))
        {
            switchpicnum = SIDEBOLT1;
        }
        if ((s->picnum > BOLT1) && (s->picnum <= BOLT1+3))
        {
            switchpicnum = BOLT1;
        }

        switch (dynamictostatic[switchpicnum])
        {
        case VIEWSCREEN__STATIC:
        case VIEWSCREEN2__STATIC:

            if (s->xrepeat == 0) KILLIT(i);

            p = findplayer(s, &x);

            if (x < 2048)
            {
                if (SP == 1)
                    camsprite = i;
            }
            else if (camsprite != -1 && T1 == 1)
            {
                camsprite = -1;
                T1 = 0;
                //loadtile(s->picnum);
                //invalidatetile(s->picnum,-1,255);
                walock[TILE_VIEWSCR] = 199;
            }

            goto BOLT;

        case TRASH__STATIC:

            if (s->xvel == 0) s->xvel = 1;
            IFMOVING
            {
                makeitfall(i);
                if (TRAND&1) s->zvel -= 256;
                if (klabs(s->xvel) < 48)
                    s->xvel += (TRAND&3);
            }
            else KILLIT(i);
            break;

        case SIDEBOLT1__STATIC:
            //        case SIDEBOLT1+1:
            //        case SIDEBOLT1+2:
            //        case SIDEBOLT1+3:
            p = findplayer(s, &x);
            if (x > 20480) goto BOLT;

CLEAR_THE_BOLT2:
            if (t[2])
            {
                t[2]--;
                goto BOLT;
            }
            if ((s->xrepeat|s->yrepeat) == 0)
            {
                s->xrepeat=t[0];
                s->yrepeat=t[1];
            }
            if ((TRAND&8) == 0)
            {
                t[0]=s->xrepeat;
                t[1]=s->yrepeat;
                t[2] = global_random&4;
                s->xrepeat=s->yrepeat=0;
                goto CLEAR_THE_BOLT2;
            }
            s->picnum++;

            if (l&1) s->cstat ^= 2;

            if ((TRAND&1) && sector[sect].floorpicnum == HURTRAIL)
                spritesound(SHORT_CIRCUIT,i);

            if (s->picnum == SIDEBOLT1+4) s->picnum = SIDEBOLT1;

            goto BOLT;

        case BOLT1__STATIC:
            //        case BOLT1+1:
            //        case BOLT1+2:
            //        case BOLT1+3:
            p = findplayer(s, &x);
            if (x > 20480) goto BOLT;

            if (t[3] == 0)
                t[3]=sector[sect].floorshade;

CLEAR_THE_BOLT:
            if (t[2])
            {
                t[2]--;
                sector[sect].floorshade = 20;
                sector[sect].ceilingshade = 20;
                goto BOLT;
            }
            if ((s->xrepeat|s->yrepeat) == 0)
            {
                s->xrepeat=t[0];
                s->yrepeat=t[1];
            }
            else if ((TRAND&8) == 0)
            {
                t[0]=s->xrepeat;
                t[1]=s->yrepeat;
                t[2] = global_random&4;
                s->xrepeat=s->yrepeat=0;
                goto CLEAR_THE_BOLT;
            }
            s->picnum++;

            l = global_random&7;
            s->xrepeat=l+8;

            if (l&1) s->cstat ^= 2;

            if (s->picnum == (BOLT1+1) && (TRAND&7) == 0 && sector[sect].floorpicnum == HURTRAIL)
                spritesound(SHORT_CIRCUIT,i);

            if (s->picnum==BOLT1+4) s->picnum=BOLT1;

            if (s->picnum&1)
            {
                sector[sect].floorshade = 0;
                sector[sect].ceilingshade = 0;
            }
            else
            {
                sector[sect].floorshade = 20;
                sector[sect].ceilingshade = 20;
            }
            goto BOLT;

        case WATERDRIP__STATIC:

            if (t[1])
            {
                t[1]--;
                if (t[1] == 0)
                    s->cstat &= 32767;
            }
            else
            {
                makeitfall(i);
                ssp(i,CLIPMASK0);
                if (s->xvel > 0) s->xvel -= 2;

                if (s->zvel == 0)
                {
                    s->cstat |= 32768;

                    if (s->pal != 2 && s->hitag == 0)
                        spritesound(SOMETHING_DRIPPING,i);

                    if (sprite[s->owner].picnum != WATERDRIP)
                    {
                        KILLIT(i);
                    }
                    else
                    {
                        hittype[i].bposz = s->z = t[0];
                        t[1] = 48+(TRAND&31);
                    }
                }
            }


            goto BOLT;

        case DOORSHOCK__STATIC:
            j = klabs(sector[sect].ceilingz-sector[sect].floorz)>>9;
            s->yrepeat = j+4;
            s->xrepeat = 16;
            s->z = sector[sect].floorz;
            goto BOLT;

        case TOUCHPLATE__STATIC:
            if (t[1] == 1 && s->hitag >= 0)  //Move the sector floor
            {
                x = sector[sect].floorz;

                if (t[3] == 1)
                {
                    if (x >= t[2])
                    {
                        sector[sect].floorz = x;
                        t[1] = 0;
                    }
                    else
                    {
                        sector[sect].floorz += sector[sect].extra;
                        p = checkcursectnums(sect);
                        if (p >= 0) g_player[p].ps->posz += sector[sect].extra;
                    }
                }
                else
                {
                    if (x <= s->z)
                    {
                        sector[sect].floorz = s->z;
                        t[1] = 0;
                    }
                    else
                    {
                        sector[sect].floorz -= sector[sect].extra;
                        p = checkcursectnums(sect);
                        if (p >= 0)
                            g_player[p].ps->posz -= sector[sect].extra;
                    }
                }
                goto BOLT;
            }

            if (t[5] == 1) goto BOLT;

            p = checkcursectnums(sect);
            if (p >= 0 && (g_player[p].ps->on_ground || s->ang == 512))
            {
                if (t[0] == 0 && !check_activator_motion(s->lotag))
                {
                    t[0] = 1;
                    t[1] = 1;
                    t[3] = !t[3];
                    operatemasterswitches(s->lotag);
                    operateactivators(s->lotag,p);
                    if (s->hitag > 0)
                    {
                        s->hitag--;
                        if (s->hitag == 0) t[5] = 1;
                    }
                }
            }
            else t[0] = 0;

            if (t[1] == 1)
            {
                j = headspritestat[6];
                while (j >= 0)
                {
                    if (j != i && sprite[j].picnum == TOUCHPLATE && sprite[j].lotag == s->lotag)
                    {
                        hittype[j].temp_data[1] = 1;
                        hittype[j].temp_data[3] = t[3];
                    }
                    j = nextspritestat[j];
                }
            }
            goto BOLT;

        case CANWITHSOMETHING__STATIC:
        case CANWITHSOMETHING2__STATIC:
        case CANWITHSOMETHING3__STATIC:
        case CANWITHSOMETHING4__STATIC:
            makeitfall(i);
            IFHIT
            {
                spritesound(VENT_BUST,i);
                for (j=0;j<10;j++)
                    RANDOMSCRAP;

                if (s->lotag) spawn(i,s->lotag);

                KILLIT(i);
            }
            goto BOLT;

        case EXPLODINGBARREL__STATIC:
        case WOODENHORSE__STATIC:
        case HORSEONSIDE__STATIC:
        case FLOORFLAME__STATIC:
        case FIREBARREL__STATIC:
        case FIREVASE__STATIC:
        case NUKEBARREL__STATIC:
        case NUKEBARRELDENTED__STATIC:
        case NUKEBARRELLEAKED__STATIC:
        case TOILETWATER__STATIC:
        case RUBBERCAN__STATIC:
        case STEAM__STATIC:
        case CEILINGSTEAM__STATIC:
            p = findplayer(s, &x);
            execute(i,p,x);
            goto BOLT;
        case WATERBUBBLEMAKER__STATIC:
            p = findplayer(s, &x);
            execute(i,p,x);
            goto BOLT;
        }

BOLT:
        i = nexti;
    }
}

static void bounce(int i)
{
    int daang, dax, day, daz, xvect, yvect, zvect;
    spritetype *s = &sprite[i];
    int hitsect = s->sectnum;
    int k = sector[hitsect].wallptr;
    int l = wall[k].point2;

    xvect = mulscale10(s->xvel,sintable[(s->ang+512)&2047]);
    yvect = mulscale10(s->xvel,sintable[s->ang&2047]);
    zvect = s->zvel;

    daang = getangle(wall[l].x-wall[k].x,wall[l].y-wall[k].y);

    if (s->z < (hittype[i].floorz+hittype[i].ceilingz)>>1)
        k = sector[hitsect].ceilingheinum;
    else
        k = sector[hitsect].floorheinum;

    dax = mulscale14(k,sintable[(daang)&2047]);
    day = mulscale14(k,sintable[(daang+1536)&2047]);
    daz = 4096;

    k = xvect*dax+yvect*day+zvect*daz;
    l = dax*dax+day*day+daz*daz;
    if ((klabs(k)>>14) < l)
    {
        k = divscale17(k,l);
        xvect -= mulscale16(dax,k);
        yvect -= mulscale16(day,k);
        zvect -= mulscale16(daz,k);
    }

    s->zvel = zvect;
    s->xvel = ksqrt(dmulscale8(xvect,xvect,yvect,yvect));
    s->ang = getangle(xvect,yvect);
}

static void moveweapons(void)
{
    int i = headspritestat[4], j=0, k, f, nexti, p, q;
    int dax,day,daz, x, ll;
    unsigned int qq;
    spritetype *s;

    while (i >= 0)
    {
        nexti = nextspritestat[i];
        s = &sprite[i];

        if (s->sectnum < 0) KILLIT(i);

        hittype[i].bposx = s->x;
        hittype[i].bposy = s->y;
        hittype[i].bposz = s->z;
        // here

        if (checkspriteflags(i,SPRITE_FLAG_PROJECTILE))
        {
            /* Custom projectiles.  This is a big hack. */

            if (hittype[i].projectile.pal >= 0)
                s->pal=hittype[i].projectile.pal;

            if (hittype[i].projectile.workslike & PROJECTILE_FLAG_KNEE)
                KILLIT(i);

            if (hittype[i].projectile.workslike & PROJECTILE_FLAG_RPG)
            {
                //  if (hittype[i].projectile.workslike & COOLEXPLOSION1)
                //                if( g_sounds[WIERDSHOT_FLY].num == 0 )
                //                    spritesound(WIERDSHOT_FLY,i);

                p = -1;

                if (hittype[i].projectile.workslike & PROJECTILE_FLAG_COOLEXPLOSION1)
                {
                    s->shade++;
                    if (s->shade >= 40) KILLIT(i);
                }

                if (hittype[i].projectile.drop)
                    s->zvel -= hittype[i].projectile.drop;

                if (hittype[i].projectile.workslike & PROJECTILE_FLAG_SPIT)
                    if (s->zvel < 6144)
                        s->zvel += gc-112;

                k = s->xvel;
                ll = s->zvel;

                if (sector[s->sectnum].lotag == 2)
                {
                    k = s->xvel>>1;
                    ll = s->zvel>>1;
                }

                dax = s->x;
                day = s->y;
                daz = s->z;

                getglobalz(i);
                qq = CLIPMASK1;

                if (hittype[i].projectile.trail > -1)
                {
                    for (f=0;f<=hittype[i].projectile.tnum;f++)
                    {
                        j = spawn(i,hittype[i].projectile.trail);
                        if (hittype[i].projectile.toffset != 0)
                            sprite[j].z += (hittype[i].projectile.toffset<<8);
                        if (hittype[i].projectile.txrepeat >= 0)
                            sprite[j].xrepeat=hittype[i].projectile.txrepeat;
                        if (hittype[i].projectile.tyrepeat >= 0)
                            sprite[j].yrepeat=hittype[i].projectile.tyrepeat;
                    }
                }

                for (f=1;f<=hittype[i].projectile.velmult;f++)
                    j = movesprite(i,
                                   (k*(sintable[(s->ang+512)&2047]))>>14,
                                   (k*(sintable[s->ang&2047]))>>14,ll,qq);


                if (!(hittype[i].projectile.workslike & PROJECTILE_FLAG_BOUNCESOFFWALLS) && s->yvel >= 0)
                    if (FindDistance2D(s->x-sprite[s->yvel].x,s->y-sprite[s->yvel].y) < 256)
                        j = 49152|s->yvel;

                if (s->sectnum < 0)
                {
                    KILLIT(i);
                }

                if (hittype[i].projectile.workslike & PROJECTILE_FLAG_TIMED && hittype[i].projectile.range > 0)
                {
                    if (!(hittype[i].temp_data[8]))
                        hittype[i].temp_data[8] = 1;
                    else
                        hittype[i].temp_data[8]++;

                    if (hittype[i].temp_data[8] > hittype[i].projectile.range)
                    {
                        if (hittype[i].projectile.workslike & PROJECTILE_FLAG_EXPLODEONTIMER)
                        {
                            if (hittype[i].projectile.spawns >= 0)
                            {
                                k = spawn(i,hittype[i].projectile.spawns);
                                sprite[k].x = dax;
                                sprite[k].y = day;
                                sprite[k].z = daz;

                                if (hittype[i].projectile.sxrepeat > 4) sprite[k].xrepeat=hittype[i].projectile.sxrepeat;
                                if (hittype[i].projectile.syrepeat > 4) sprite[k].yrepeat=hittype[i].projectile.syrepeat;
                            }
                            if (hittype[i].projectile.isound > -1)
                                spritesound(hittype[i].projectile.isound,i);

                            s->extra=hittype[i].projectile.extra;

                            if (hittype[i].projectile.extra_rand > 0)
                                s->extra += (TRAND&hittype[i].projectile.extra_rand);

                            x = s->extra;
                            hitradius(i,hittype[i].projectile.hitradius, x>>2,x>>1,x-(x>>2),x);
                        }
                        KILLIT(i);
                    }
                }

                if (hittype[i].projectile.workslike & PROJECTILE_FLAG_BOUNCESOFFWALLS)
                {
                    /*                    if(s->yvel < 1 || s->extra < 2 || (s->xvel|s->zvel) == 0)
                                Did this cause the bug with prematurely exploding projectiles? */
                    if (s->yvel < 1)
                    {

                        if (hittype[i].projectile.spawns >= 0)
                        {
                            k = spawn(i,hittype[i].projectile.spawns);
                            sprite[k].x = dax;
                            sprite[k].y = day;
                            sprite[k].z = daz;

                            if (hittype[i].projectile.sxrepeat > 4) sprite[k].xrepeat=hittype[i].projectile.sxrepeat;
                            if (hittype[i].projectile.syrepeat > 4) sprite[k].yrepeat=hittype[i].projectile.syrepeat;
                        }
                        if (hittype[i].projectile.isound > -1)
                            spritesound(hittype[i].projectile.isound,i);

                        s->extra=hittype[i].projectile.extra;

                        if (hittype[i].projectile.extra_rand > 0)
                            s->extra += (TRAND&hittype[i].projectile.extra_rand);

                        x = s->extra;
                        hitradius(i,hittype[i].projectile.hitradius, x>>2,x>>1,x-(x>>2),x);

                        KILLIT(i);
                    }

                }

                if ((j&49152) != 49152)
                    if (!(hittype[i].projectile.workslike & PROJECTILE_FLAG_BOUNCESOFFWALLS))
                    {
                        if (s->z < hittype[i].ceilingz)
                        {
                            j = 16384|(s->sectnum);
                            s->zvel = -1;
                        }
                        else
                            if ((s->z > hittype[i].floorz && sector[s->sectnum].lotag != 1) ||
                                    (s->z > hittype[i].floorz+(16<<8) && sector[s->sectnum].lotag == 1))
                            {
                                j = 16384|(s->sectnum);
                                if (sector[s->sectnum].lotag != 1)
                                    s->zvel = 1;
                            }
                    }

                /*                if(hittype[i].projectile.workslike & 8192)
                                {
                                    for(k=-3;k<2;k++)
                                    {

                                        x = EGS(s->sectnum,
                                                s->x+((k*sintable[(s->ang+512)&2047])>>9),
                                                s->y+((k*sintable[s->ang&2047])>>9),
                                                s->z+((k*ksgn(s->zvel))*klabs(s->zvel/24)),s->picnum,-40+(k<<2), // FIRELASER
                                                s->xrepeat,s->yrepeat,0,0,0,s->owner,5);

                                        sprite[x].cstat = 128;
                                        sprite[x].pal = s->pal;

                                    }
                                }
                                else */

                if (hittype[i].projectile.workslike & PROJECTILE_FLAG_WATERBUBBLES && sector[s->sectnum].lotag == 2 && rnd(140))
                    spawn(i,WATERBUBBLE);

                if (j != 0)
                {
                    if (hittype[i].projectile.workslike & PROJECTILE_FLAG_COOLEXPLOSION1)
                    {
                        /*                        if( (j&49152) == 49152 && sprite[j&(MAXSPRITES-1)].picnum != APLAYER)
                                                    goto BOLT; */
                        s->xvel = 0;
                        s->zvel = 0;
                    }

                    if ((j&49152) == 49152)
                    {
                        j &= (MAXSPRITES-1);

                        /*                        if(hittype[i].projectile.workslike & PROJECTILE_FLAG_FREEZEBLAST && sprite[j].pal == 1 )
                                                    if( badguy(&sprite[j]) || sprite[j].picnum == APLAYER )
                                                    {
                                                        j = spawn(i,TRANSPORTERSTAR);
                                                        sprite[j].pal = 1;
                                                        sprite[j].xrepeat = 32;
                                                        sprite[j].yrepeat = 32;

                                                        KILLIT(i);
                                                    }*/

                        if (hittype[i].projectile.workslike & PROJECTILE_FLAG_BOUNCESOFFSPRITES)
                        {
                            s->yvel--;

                            k = getangle(sprite[j].x-s->x,sprite[j].y-s->y)+(sprite[j].cstat&16?0:512);
                            s->ang = ((k<<1) - s->ang)&2047;

                            if (hittype[i].projectile.bsound > -1)
                                spritesound(hittype[i].projectile.bsound,i);

                            if (hittype[i].projectile.workslike & PROJECTILE_FLAG_LOSESVELOCITY)
                            {
                                s->xvel=s->xvel>>1;
                                s->zvel=s->zvel>>1;
                            }
                            goto BOLT;
                        }

                        checkhitsprite(j,i);

                        if (sprite[j].picnum == APLAYER)
                        {
                            p = sprite[j].yvel;
                            spritesound(PISTOL_BODYHIT,j);

                            if (hittype[i].projectile.workslike & PROJECTILE_FLAG_SPIT)
                            {
                                g_player[p].ps->horiz += 32;
                                g_player[p].ps->return_to_center = 8;

                                if (g_player[p].ps->loogcnt == 0)
                                {
                                    if (!isspritemakingsound(g_player[p].ps->i, DUKE_LONGTERM_PAIN))
                                        spritesound(DUKE_LONGTERM_PAIN,g_player[p].ps->i);

                                    j = 3+(TRAND&3);
                                    g_player[p].ps->numloogs = j;
                                    g_player[p].ps->loogcnt = 24*4;
                                    for (x=0;x < j;x++)
                                    {
                                        g_player[p].ps->loogiex[x] = TRAND%xdim;
                                        g_player[p].ps->loogiey[x] = TRAND%ydim;
                                    }
                                }
                            }
                        }

                        if (hittype[i].projectile.workslike & PROJECTILE_FLAG_RPG_IMPACT)
                        {

                            hittype[j].owner = s->owner;
                            hittype[j].picnum = s->picnum;
                            hittype[j].extra += hittype[i].projectile.extra;

                            if (hittype[i].projectile.spawns >= 0)
                            {
                                k = spawn(i,hittype[i].projectile.spawns);
                                sprite[k].x = dax;
                                sprite[k].y = day;
                                sprite[k].z = daz;

                                if (hittype[i].projectile.sxrepeat > 4) sprite[k].xrepeat=hittype[i].projectile.sxrepeat;
                                if (hittype[i].projectile.syrepeat > 4) sprite[k].yrepeat=hittype[i].projectile.syrepeat;
                            }

                            if (hittype[i].projectile.isound > -1)
                                spritesound(hittype[i].projectile.isound,i);

                            KILLIT(i);

                        }

                    }
                    else if ((j&49152) == 32768)
                    {
                        j &= (MAXWALLS-1);

                        if (hittype[i].projectile.workslike & PROJECTILE_FLAG_BOUNCESOFFMIRRORS && (wall[j].overpicnum == MIRROR || wall[j].picnum == MIRROR))
                        {
                            k = getangle(
                                    wall[wall[j].point2].x-wall[j].x,
                                    wall[wall[j].point2].y-wall[j].y);
                            s->ang = ((k<<1) - s->ang)&2047;
                            s->owner = i;
                            spawn(i,TRANSPORTERSTAR);
                            goto BOLT;
                        }
                        else
                        {
                            setsprite(i,dax,day,daz);
                            checkhitwall(i,j,s->x,s->y,s->z,s->picnum);

                            if (hittype[i].projectile.workslike & PROJECTILE_FLAG_BOUNCESOFFWALLS)
                            {
                                if (wall[j].overpicnum != MIRROR && wall[j].picnum != MIRROR)
                                    s->yvel--;

                                k = getangle(
                                        wall[wall[j].point2].x-wall[j].x,
                                        wall[wall[j].point2].y-wall[j].y);
                                s->ang = ((k<<1) - s->ang)&2047;

                                if (hittype[i].projectile.bsound > -1)
                                    spritesound(hittype[i].projectile.bsound,i);

                                if (hittype[i].projectile.workslike & PROJECTILE_FLAG_LOSESVELOCITY)
                                {
                                    s->xvel=s->xvel>>1;
                                    s->zvel=s->zvel>>1;
                                }
                                goto BOLT;
                            }
                        }
                    }
                    else if ((j&49152) == 16384)
                    {
                        setsprite(i,dax,day,daz);

                        if (s->zvel < 0)
                        {
                            if (sector[s->sectnum].ceilingstat&1)
                                if (sector[s->sectnum].ceilingpal == 0)
                                    KILLIT(i);

                            checkhitceiling(s->sectnum);
                        }

                        if (hittype[i].projectile.workslike & PROJECTILE_FLAG_BOUNCESOFFWALLS)
                        {
                            bounce(i);
                            ssp(i,qq);

                            /*                            if(s->xrepeat > 8)
                                                            s->xrepeat -= 2;
                                                        if(s->yrepeat > 8)
                                                            s->yrepeat -= 2;*/
                            s->yvel--;

                            if (hittype[i].projectile.bsound > -1)
                                spritesound(hittype[i].projectile.bsound,i);

                            if (hittype[i].projectile.workslike & PROJECTILE_FLAG_LOSESVELOCITY)
                            {
                                s->xvel=s->xvel>>1;
                                s->zvel=s->zvel>>1;
                            }

                            goto BOLT;
                        }
                    }

                    if (hittype[i].projectile.workslike & PROJECTILE_FLAG_RPG)
                    {
                        if (hittype[i].projectile.spawns > 0)
                        {
                            k = spawn(i,hittype[i].projectile.spawns);
                            sprite[k].x = dax;
                            sprite[k].y = day;
                            sprite[k].z = daz;

                            if (hittype[i].projectile.sxrepeat > 4) sprite[k].xrepeat=hittype[i].projectile.sxrepeat;
                            if (hittype[i].projectile.syrepeat > 4) sprite[k].yrepeat=hittype[i].projectile.syrepeat;
                        }
                        /*                            if(s->xrepeat < 10)
                                                    {
                                                        sprite[k].xrepeat = 6;
                                                        sprite[k].yrepeat = 6;
                                                    }*/
                        /*                        else if( (j&49152) == 16384)
                                                {
                                                    if( s->zvel > 0)
                                                        spawn(i,EXPLOSION2BOT);
                                                    else
                                                    {
                                                        sprite[k].cstat |= 8;
                                                        sprite[k].z += (48<<8);
                                                    }
                                                }
                        */
                    }


                    if (hittype[i].projectile.workslike & PROJECTILE_FLAG_HITSCAN)
                    {
                        p = findplayer(s,&x);
                        execute(i,p,x);
                        goto BOLT;
                    }


                    if (hittype[i].projectile.workslike & PROJECTILE_FLAG_RPG)
                    {
                        if (hittype[i].projectile.isound > -1)
                            spritesound(hittype[i].projectile.isound,i);

                        /*                            if(s->xrepeat >= 10)
                                                    {*/
                        s->extra=hittype[i].projectile.extra;
                        if (hittype[i].projectile.extra_rand > 0)
                            s->extra += (TRAND&hittype[i].projectile.extra_rand);

                        x = s->extra;
                        hitradius(i,hittype[i].projectile.hitradius, x>>2,x>>1,x-(x>>2),x);
                        /*                            }
                                                    else
                                                    {
                                                        x = s->extra+(global_random&3);
                                                        hitradius( i,(hittype[i].projectile.hitradius>>1),x>>2,x>>1,x-(x>>2),x);
                                                    }*/
                        //                        if (!(hittype[i].projectile.workslike & PROJECTILE_FLAG_COOLEXPLOSION1))
                        KILLIT(i);
                    }
                }


                goto BOLT;


            }
        }
        else

        {

            // here
            switch (dynamictostatic[s->picnum])
            {
            case RADIUSEXPLOSION__STATIC:
            case KNEE__STATIC:
                KILLIT(i);
            case TONGUE__STATIC:
                T1 = sintable[(T2)&2047]>>9;
                T2 += 32;
                if (T2 > 2047) KILLIT(i);

                if (sprite[s->owner].statnum == MAXSTATUS)
                    if (badguy(&sprite[s->owner]) == 0)
                        KILLIT(i);

                s->ang = sprite[s->owner].ang;
                s->x = sprite[s->owner].x;
                s->y = sprite[s->owner].y;
                if (sprite[s->owner].picnum == APLAYER)
                    s->z = sprite[s->owner].z-(34<<8);
                for (k=0;k<T1;k++)
                {
                    q = EGS(s->sectnum,
                            s->x+((k*sintable[(s->ang+512)&2047])>>9),
                            s->y+((k*sintable[s->ang&2047])>>9),
                            s->z+((k*ksgn(s->zvel))*klabs(s->zvel/12)),TONGUE,-40+(k<<1),
                            8,8,0,0,0,i,5);
                    sprite[q].cstat = 128;
                    sprite[q].pal = 8;
                }
                q = EGS(s->sectnum,
                        s->x+((k*sintable[(s->ang+512)&2047])>>9),
                        s->y+((k*sintable[s->ang&2047])>>9),
                        s->z+((k*ksgn(s->zvel))*klabs(s->zvel/12)),INNERJAW,-40,
                        32,32,0,0,0,i,5);
                sprite[q].cstat = 128;
                if (T2 > 512 && T2 < (1024))
                    sprite[q].picnum = INNERJAW+1;

                goto BOLT;

            case FREEZEBLAST__STATIC:
                if (s->yvel < 1 || s->extra < 2 || (s->xvel|s->zvel) == 0)
                {
                    j = spawn(i,TRANSPORTERSTAR);
                    sprite[j].pal = 1;
                    sprite[j].xrepeat = 32;
                    sprite[j].yrepeat = 32;
                    KILLIT(i);
                }
            case SHRINKSPARK__STATIC:
            case RPG__STATIC:
            case FIRELASER__STATIC:
            case SPIT__STATIC:
            case COOLEXPLOSION1__STATIC:

                if (s->picnum == COOLEXPLOSION1)
                    if (!issoundplaying(i,WIERDSHOT_FLY))
                        spritesound(WIERDSHOT_FLY,i);

                p = -1;

                k = s->xvel;
                ll = s->zvel;

                if (s->picnum == RPG && sector[s->sectnum].lotag == 2)
                {
                    k = s->xvel>>1;
                    ll = s->zvel>>1;
                }

                dax = s->x;
                day = s->y;
                daz = s->z;

                getglobalz(i);
                qq = CLIPMASK1;

                switch (dynamictostatic[s->picnum])
                {
                case RPG__STATIC:
                    if (hittype[i].picnum != BOSS2 && s->xrepeat >= 10 && sector[s->sectnum].lotag != 2)
                    {
                        j = spawn(i,SMALLSMOKE);
                        sprite[j].z += (1<<8);
                    }
                    break;
                }

                j = movesprite(i,(k*(sintable[(s->ang+512)&2047]))>>14,(k*(sintable[s->ang&2047]))>>14,ll,qq);

                if (s->picnum == RPG && s->yvel >= 0)
                    if (FindDistance2D(s->x-sprite[s->yvel].x,s->y-sprite[s->yvel].y) < 256)
                        j = 49152|s->yvel;

                if (s->sectnum < 0)
                    KILLIT(i);

                if ((j&49152) != 49152)
                    if (s->picnum != FREEZEBLAST)
                    {
                        if (s->z < hittype[i].ceilingz)
                        {
                            j = 16384|(s->sectnum);
                            s->zvel = -1;
                        }
                        else
                            if ((s->z > hittype[i].floorz && sector[s->sectnum].lotag != 1) ||
                                    (s->z > hittype[i].floorz+(16<<8) && sector[s->sectnum].lotag == 1))
                            {
                                j = 16384|(s->sectnum);
                                if (sector[s->sectnum].lotag != 1)
                                    s->zvel = 1;
                            }
                    }

                if (s->picnum == FIRELASER)
                {
                    for (k=-3;k<2;k++)
                    {
                        x = EGS(s->sectnum,
                                s->x+((k*sintable[(s->ang+512)&2047])>>9),
                                s->y+((k*sintable[s->ang&2047])>>9),
                                s->z+((k*ksgn(s->zvel))*klabs(s->zvel/24)),FIRELASER,-40+(k<<2),
                                s->xrepeat,s->yrepeat,0,0,0,s->owner,5);

                        sprite[x].cstat = 128;
                        sprite[x].pal = s->pal;
                    }
                }
                else if (s->picnum == SPIT) if (s->zvel < 6144)
                        s->zvel += gc-112;

                if (j != 0)
                {
                    if (s->picnum == COOLEXPLOSION1)
                    {
                        if ((j&49152) == 49152 && sprite[j&(MAXSPRITES-1)].picnum != APLAYER)
                            goto BOLT;
                        s->xvel = 0;
                        s->zvel = 0;
                    }

                    if ((j&49152) == 49152)
                    {
                        j &= (MAXSPRITES-1);

                        if (s->picnum == FREEZEBLAST && sprite[j].pal == 1)
                            if (badguy(&sprite[j]) || sprite[j].picnum == APLAYER)
                            {
                                j = spawn(i,TRANSPORTERSTAR);
                                sprite[j].pal = 1;
                                sprite[j].xrepeat = 32;
                                sprite[j].yrepeat = 32;

                                KILLIT(i);
                            }

                        checkhitsprite(j,i);

                        if (sprite[j].picnum == APLAYER)
                        {
                            p = sprite[j].yvel;
                            spritesound(PISTOL_BODYHIT,j);

                            if (s->picnum == SPIT)
                            {
                                g_player[p].ps->horiz += 32;
                                g_player[p].ps->return_to_center = 8;

                                if (g_player[p].ps->loogcnt == 0)
                                {
                                    if (!isspritemakingsound(g_player[p].ps->i, DUKE_LONGTERM_PAIN))
                                        spritesound(DUKE_LONGTERM_PAIN,g_player[p].ps->i);

                                    j = 3+(TRAND&3);
                                    g_player[p].ps->numloogs = j;
                                    g_player[p].ps->loogcnt = 24*4;
                                    for (x=0;x < j;x++)
                                    {
                                        g_player[p].ps->loogiex[x] = TRAND%xdim;
                                        g_player[p].ps->loogiey[x] = TRAND%ydim;
                                    }
                                }
                            }
                        }
                    }
                    else if ((j&49152) == 32768)
                    {
                        j &= (MAXWALLS-1);

                        if (s->picnum != RPG && s->picnum != FREEZEBLAST && s->picnum != SPIT && (wall[j].overpicnum == MIRROR || wall[j].picnum == MIRROR))
                        {
                            k = getangle(
                                    wall[wall[j].point2].x-wall[j].x,
                                    wall[wall[j].point2].y-wall[j].y);
                            s->ang = ((k<<1) - s->ang)&2047;
                            s->owner = i;
                            spawn(i,TRANSPORTERSTAR);
                            goto BOLT;
                        }
                        else
                        {
                            setsprite(i,dax,day,daz);
                            checkhitwall(i,j,s->x,s->y,s->z,s->picnum);

                            if (s->picnum == FREEZEBLAST)
                            {
                                if (wall[j].overpicnum != MIRROR && wall[j].picnum != MIRROR)
                                {
                                    s->extra >>= 1;
                                    s->yvel--;
                                }

                                k = getangle(
                                        wall[wall[j].point2].x-wall[j].x,
                                        wall[wall[j].point2].y-wall[j].y);
                                s->ang = ((k<<1) - s->ang)&2047;
                                goto BOLT;
                            }
                        }
                    }
                    else if ((j&49152) == 16384)
                    {
                        setsprite(i,dax,day,daz);

                        if (s->zvel < 0)
                        {
                            if (sector[s->sectnum].ceilingstat&1)
                                if (sector[s->sectnum].ceilingpal == 0)
                                    KILLIT(i);

                            checkhitceiling(s->sectnum);
                        }

                        if (s->picnum == FREEZEBLAST)
                        {
                            bounce(i);
                            ssp(i,qq);
                            s->extra >>= 1;
                            if (s->xrepeat > 8)
                                s->xrepeat -= 2;
                            if (s->yrepeat > 8)
                                s->yrepeat -= 2;
                            s->yvel--;
                            goto BOLT;
                        }
                    }

                    if (s->picnum != SPIT)
                    {
                        if (s->picnum == RPG)
                        {
                            k = spawn(i,EXPLOSION2);
                            sprite[k].x = dax;
                            sprite[k].y = day;
                            sprite[k].z = daz;

                            if (s->xrepeat < 10)
                            {
                                sprite[k].xrepeat = 6;
                                sprite[k].yrepeat = 6;
                            }
                            else if ((j&49152) == 16384)
                            {
                                if (s->zvel > 0)
                                    spawn(i,EXPLOSION2BOT);
                                else
                                {
                                    sprite[k].cstat |= 8;
                                    sprite[k].z += (48<<8);
                                }

                            }
                            spritesound(RPG_EXPLODE,i);

                            if (s->xrepeat >= 10)
                            {
                                x = s->extra;
                                hitradius(i,rpgblastradius, x>>2,x>>1,x-(x>>2),x);
                            }
                            else
                            {
                                x = s->extra+(global_random&3);
                                hitradius(i,(rpgblastradius>>1),x>>2,x>>1,x-(x>>2),x);
                            }
                        }
                        else if (s->picnum == SHRINKSPARK)
                        {
                            spawn(i,SHRINKEREXPLOSION);
                            spritesound(SHRINKER_HIT,i);
                            hitradius(i,shrinkerblastradius,0,0,0,0);
                        }
                        else if (s->picnum != COOLEXPLOSION1 && s->picnum != FREEZEBLAST && s->picnum != FIRELASER)
                        {
                            k = spawn(i,EXPLOSION2);
                            sprite[k].xrepeat = sprite[k].yrepeat = s->xrepeat>>1;
                            if ((j&49152) == 16384)
                            {
                                if (s->zvel < 0)
                                {
                                    sprite[k].cstat |= 8;
                                    sprite[k].z += (72<<8);
                                }

                            }
                        }
                    }
                    if (s->picnum != COOLEXPLOSION1) KILLIT(i);
                }
                if (s->picnum == COOLEXPLOSION1)
                {
                    s->shade++;
                    if (s->shade >= 40) KILLIT(i);
                }
                else if (s->picnum == RPG && sector[s->sectnum].lotag == 2 && s->xrepeat >= 10 && rnd(140))
                    spawn(i,WATERBUBBLE);

                goto BOLT;

            case SHOTSPARK1__STATIC:
                p = findplayer(s,&x);
                execute(i,p,x);
                goto BOLT;
            }
        }
BOLT:
        i = nexti;
    }
}

static void movetransports(void)
{
    int warpspriteto;
    int i = headspritestat[9], j, k, l, p, sect, sectlotag, nexti, nextj;
    int ll,onfloorz,q;

    while (i >= 0)
    {
        sect = SECT;
        sectlotag = sector[sect].lotag;

        nexti = nextspritestat[i];

        if (OW == i)
        {
            i = nexti;
            continue;
        }

        onfloorz = T5;

        if (T1 > 0) T1--;

        j = headspritesect[sect];
        while (j >= 0)
        {
            nextj = nextspritesect[j];

            switch (sprite[j].statnum)
            {
            case 10:    // Player

                if (sprite[j].owner != -1)
                {
                    p = sprite[j].yvel;

                    g_player[p].ps->on_warping_sector = 1;

                    if (g_player[p].ps->transporter_hold == 0 && g_player[p].ps->jumping_counter == 0)
                    {
                        if (g_player[p].ps->on_ground && sectlotag == 0 && onfloorz && g_player[p].ps->jetpack_on == 0)
                        {
                            if (sprite[i].pal == 0)
                            {
                                spawn(i,TRANSPORTERBEAM);
                                spritesound(TELEPORTER,i);
                            }

                            for (k=connecthead;k>=0;k=connectpoint2[k])
                                if (g_player[k].ps->cursectnum == sprite[OW].sectnum)
                                {
                                    g_player[k].ps->frag_ps = p;
                                    sprite[g_player[k].ps->i].extra = 0;
                                }

                            g_player[p].ps->ang = sprite[OW].ang;

                            if (sprite[OW].owner != OW)
                            {
                                T1 = 13;
                                hittype[OW].temp_data[0] = 13;
                                g_player[p].ps->transporter_hold = 13;
                            }

                            g_player[p].ps->bobposx = g_player[p].ps->oposx = g_player[p].ps->posx = sprite[OW].x;
                            g_player[p].ps->bobposy = g_player[p].ps->oposy = g_player[p].ps->posy = sprite[OW].y;
                            g_player[p].ps->oposz = g_player[p].ps->posz = sprite[OW].z-PHEIGHT;

                            changespritesect(j,sprite[OW].sectnum);
                            g_player[p].ps->cursectnum = sprite[j].sectnum;

                            if (sprite[i].pal == 0)
                            {
                                k = spawn(OW,TRANSPORTERBEAM);
                                spritesound(TELEPORTER,k);
                            }

                            break;
                        }
                    }
                    else if (!(sectlotag == 1 && g_player[p].ps->on_ground == 1)) break;

                    if (onfloorz == 0 && klabs(SZ-g_player[p].ps->posz) < 6144)
                        if ((g_player[p].ps->jetpack_on == 0) || (g_player[p].ps->jetpack_on && (g_player[p].sync->bits&1)) ||
                                (g_player[p].ps->jetpack_on && (g_player[p].sync->bits&2)))
                        {
                            g_player[p].ps->oposx = g_player[p].ps->posx += sprite[OW].x-SX;
                            g_player[p].ps->oposy = g_player[p].ps->posy += sprite[OW].y-SY;

                            if (g_player[p].ps->jetpack_on && ((g_player[p].sync->bits&1) || g_player[p].ps->jetpack_on < 11))
                                g_player[p].ps->posz = sprite[OW].z-6144;
                            else g_player[p].ps->posz = sprite[OW].z+6144;
                            g_player[p].ps->oposz = g_player[p].ps->posz;

                            hittype[g_player[p].ps->i].bposx = g_player[p].ps->posx;
                            hittype[g_player[p].ps->i].bposy = g_player[p].ps->posy;
                            hittype[g_player[p].ps->i].bposz = g_player[p].ps->posz;

                            changespritesect(j,sprite[OW].sectnum);
                            g_player[p].ps->cursectnum = sprite[OW].sectnum;

                            break;
                        }

                    k = 0;

                    if (onfloorz && sectlotag == 1 && g_player[p].ps->on_ground && g_player[p].ps->posz > (sector[sect].floorz-1080) && ((g_player[p].sync->bits&2) || g_player[p].ps->poszv > 2048))
                        //                        if( onfloorz && sectlotag == 1 && g_player[p].ps->posz > (sector[sect].floorz-(6<<8)) )
                    {
                        k = 1;
                        if (screenpeek == p)
                        {
                            FX_StopAllSounds();
                            clearsoundlocks();
                        }
                        if (sprite[g_player[p].ps->i].extra > 0)
                            spritesound(DUKE_UNDERWATER,j);
                        g_player[p].ps->oposz = g_player[p].ps->posz =
                                                    sector[sprite[OW].sectnum].ceilingz;

                        g_player[p].ps->posxv = 4096-(TRAND&8192);
                        g_player[p].ps->posyv = 4096-(TRAND&8192);
                        //          g_player[p].ps->poszv += 1080;
                    }

                    if (onfloorz && sectlotag == 2 && g_player[p].ps->posz < (sector[sect].ceilingz+1080) && g_player[p].ps->poszv == 0)
                    {
                        k = 1;
                        //                            if( sprite[j].extra <= 0) break;
                        if (screenpeek == p)
                        {
                            FX_StopAllSounds();
                            clearsoundlocks();
                        }
                        spritesound(DUKE_GASP,j);

                        g_player[p].ps->oposz = g_player[p].ps->posz =
                                                    sector[sprite[OW].sectnum].floorz;

                        g_player[p].ps->jumping_toggle = 1;
                        g_player[p].ps->jumping_counter = 0;
                        //                        g_player[p].ps->poszv += 1024;
                    }

                    if (k == 1)
                    {
                        g_player[p].ps->oposx = g_player[p].ps->posx += sprite[OW].x-SX;
                        g_player[p].ps->oposy = g_player[p].ps->posy += sprite[OW].y-SY;

                        if (sprite[OW].owner != OW)
                            g_player[p].ps->transporter_hold = -2;
                        g_player[p].ps->cursectnum = sprite[OW].sectnum;

                        changespritesect(j,sprite[OW].sectnum);
                        setsprite(g_player[p].ps->i,g_player[p].ps->posx,g_player[p].ps->posy,g_player[p].ps->posz+PHEIGHT);

                        setpal(g_player[p].ps);

                        if ((TRAND&255) < 32)
                            spawn(j,WATERSPLASH2);

                        if (sectlotag == 1)
                            for (l = 0;l < 9;l++)
                            {
                                q = spawn(g_player[p].ps->i,WATERBUBBLE);
                                sprite[q].z += TRAND&16383;
                            }
                    }
                }
                break;

            case 1:
                if ((sprite[j].picnum == SHARK) || (sprite[j].picnum == COMMANDER) || (sprite[j].picnum == OCTABRAIN)
                        || ((sprite[j].picnum >= GREENSLIME) && (sprite[j].picnum >= GREENSLIME+7)))
                {
                    if (sprite[j].extra > 0)
                        goto JBOLT;
                }
            case 4:
            case 5:
            case 12:
            case 13:

                ll = klabs(sprite[j].zvel);

                {
                    warpspriteto = 0;
                    if (ll && sectlotag == 2 && sprite[j].z < (sector[sect].ceilingz+ll))
                        warpspriteto = 1;

                    if (ll && sectlotag == 1 && sprite[j].z > (sector[sect].floorz-ll))
                        warpspriteto = 1;

                    if (sectlotag == 0 && (onfloorz || klabs(sprite[j].z-SZ) < 4096))
                    {
                        if (sprite[OW].owner != OW && onfloorz && T1 > 0 && sprite[j].statnum != 5)
                        {
                            T1++;
                            goto BOLT;
                        }
                        warpspriteto = 1;
                    }

                    if (warpspriteto && checkspriteflags(j,SPRITE_FLAG_DECAL)) goto JBOLT;

                    if (warpspriteto) switch (dynamictostatic[sprite[j].picnum])
                        {
                        case TRANSPORTERSTAR__STATIC:
                        case TRANSPORTERBEAM__STATIC:
                        case TRIPBOMB__STATIC:
                        case BULLETHOLE__STATIC:
                        case WATERSPLASH2__STATIC:
                        case BURNING__STATIC:
                        case BURNING2__STATIC:
                        case FIRE__STATIC:
                        case FIRE2__STATIC:
                        case TOILETWATER__STATIC:
                        case LASERLINE__STATIC:
                            goto JBOLT;
                        case PLAYERONWATER__STATIC:
                            if (sectlotag == 2)
                            {
                                sprite[j].cstat &= 32767;
                                break;
                            }
                        default:
                            if (sprite[j].statnum == 5 && !(sectlotag == 1 || sectlotag == 2))
                                break;

                        case WATERBUBBLE__STATIC:
                            //                                if( rnd(192) && sprite[j].picnum == WATERBUBBLE)
                            //                                 break;

                            if (sectlotag > 0)
                            {
                                k = spawn(j,WATERSPLASH2);
                                if (sectlotag == 1 && sprite[j].statnum == 4)
                                {
                                    sprite[k].xvel = sprite[j].xvel>>1;
                                    sprite[k].ang = sprite[j].ang;
                                    ssp(k,CLIPMASK0);
                                }
                            }

                            switch (sectlotag)
                            {
                            case 0:
                                if (onfloorz)
                                {
                                    if (sprite[j].statnum == 4 || (checkcursectnums(sect) == -1 && checkcursectnums(sprite[OW].sectnum)  == -1))
                                    {
                                        sprite[j].x += (sprite[OW].x-SX);
                                        sprite[j].y += (sprite[OW].y-SY);
                                        sprite[j].z -= SZ - sector[sprite[OW].sectnum].floorz;
                                        sprite[j].ang = sprite[OW].ang;

                                        hittype[j].bposx = sprite[j].x;
                                        hittype[j].bposy = sprite[j].y;
                                        hittype[j].bposz = sprite[j].z;

                                        if (sprite[i].pal == 0)
                                        {
                                            k = spawn(i,TRANSPORTERBEAM);
                                            spritesound(TELEPORTER,k);

                                            k = spawn(OW,TRANSPORTERBEAM);
                                            spritesound(TELEPORTER,k);
                                        }

                                        if (sprite[OW].owner != OW)
                                        {
                                            T1 = 13;
                                            hittype[OW].temp_data[0] = 13;
                                        }

                                        changespritesect(j,sprite[OW].sectnum);
                                    }
                                }
                                else
                                {
                                    sprite[j].x += (sprite[OW].x-SX);
                                    sprite[j].y += (sprite[OW].y-SY);
                                    sprite[j].z = sprite[OW].z+4096;

                                    hittype[j].bposx = sprite[j].x;
                                    hittype[j].bposy = sprite[j].y;
                                    hittype[j].bposz = sprite[j].z;

                                    changespritesect(j,sprite[OW].sectnum);
                                }
                                break;
                            case 1:
                                sprite[j].x += (sprite[OW].x-SX);
                                sprite[j].y += (sprite[OW].y-SY);
                                sprite[j].z = sector[sprite[OW].sectnum].ceilingz+ll;

                                hittype[j].bposx = sprite[j].x;
                                hittype[j].bposy = sprite[j].y;
                                hittype[j].bposz = sprite[j].z;

                                changespritesect(j,sprite[OW].sectnum);

                                break;
                            case 2:
                                sprite[j].x += (sprite[OW].x-SX);
                                sprite[j].y += (sprite[OW].y-SY);
                                sprite[j].z = sector[sprite[OW].sectnum].floorz-ll;

                                hittype[j].bposx = sprite[j].x;
                                hittype[j].bposy = sprite[j].y;
                                hittype[j].bposz = sprite[j].z;

                                changespritesect(j,sprite[OW].sectnum);

                                break;
                            }

                            break;
                        }
                }
                break;

            }
JBOLT:
            j = nextj;
        }
BOLT:
        i = nexti;
    }
}

static short LocateTheLocator(int n,int sn)
{
    int i = headspritestat[7];

    while (i >= 0)
    {
        if ((sn == -1 || sn == SECT) && n == SLT)
            return i;
        i = nextspritestat[i];
    }
    return -1;
}

static void moveactors(void)
{
    int x, m, l, *t;
    int a, j, nexti, nextj, sect, p, switchpicnum, k;
    spritetype *s;
    int i = headspritestat[1];

    while (i >= 0)
    {
        nexti = nextspritestat[i];

        s = &sprite[i];

        sect = s->sectnum;

        if (s->xrepeat == 0 || sect < 0 || sect >= MAXSECTORS)
            KILLIT(i);

        t = &hittype[i].temp_data[0];

        hittype[i].bposx = s->x;
        hittype[i].bposy = s->y;
        hittype[i].bposz = s->z;
        switchpicnum=s->picnum;
        if ((s->picnum > GREENSLIME)&&(s->picnum <= GREENSLIME+7))
        {
            switchpicnum = GREENSLIME;
        }
        switch (dynamictostatic[switchpicnum])
        {
        case DUCK__STATIC:
        case TARGET__STATIC:
            if (s->cstat&32)
            {
                t[0]++;
                if (t[0] > 60)
                {
                    t[0] = 0;
                    s->cstat = 128+257+16;
                    s->extra = 1;
                }
            }
            else
            {
                j = ifhitbyweapon(i);
                if (j >= 0)
                {
                    s->cstat = 32+128;
                    k = 1;

                    j = headspritestat[1];
                    while (j >= 0)
                    {
                        if (sprite[j].lotag == s->lotag &&
                                sprite[j].picnum == s->picnum)
                        {
                            if ((sprite[j].hitag && !(sprite[j].cstat&32)) ||
                                    (!sprite[j].hitag && (sprite[j].cstat&32))
                               )
                            {
                                k = 0;
                                break;
                            }
                        }

                        j = nextspritestat[j];
                    }

                    if (k == 1)
                    {
                        operateactivators(s->lotag,-1);
                        operateforcefields(i,s->lotag);
                        operatemasterswitches(s->lotag);
                    }
                }
            }
            goto BOLT;

        case RESPAWNMARKERRED__STATIC:
        case RESPAWNMARKERYELLOW__STATIC:
        case RESPAWNMARKERGREEN__STATIC:
            T1++;
            if (T1 > respawnitemtime)
            {
                KILLIT(i);
            }
            if (T1 >= (respawnitemtime>>1) && T1 < ((respawnitemtime>>1)+(respawnitemtime>>2)))
                PN = RESPAWNMARKERYELLOW;
            else if (T1 > ((respawnitemtime>>1)+(respawnitemtime>>2)))
                PN = RESPAWNMARKERGREEN;
            makeitfall(i);
            break;

        case HELECOPT__STATIC:
        case DUKECAR__STATIC:

            s->z += s->zvel;
            t[0]++;

            if (t[0] == 4) spritesound(WAR_AMBIENCE2,i);

            if (t[0] > (26*8))
            {
                sound(RPG_EXPLODE);
                for (j=0;j<32;j++) RANDOMSCRAP;
                earthquaketime = 16;
                KILLIT(i);
            }
            else if ((t[0]&3) == 0)
                spawn(i,EXPLOSION2);
            ssp(i,CLIPMASK0);
            break;
        case RAT__STATIC:
            makeitfall(i);
            IFMOVING
            {
                if ((TRAND&255) < 3) spritesound(RATTY,i);
                s->ang += (TRAND&31)-15+(sintable[(t[0]<<8)&2047]>>11);
            }
            else
            {
                T1++;
                if (T1 > 1)
                {
                    KILLIT(i);
                }
                else s->ang = (TRAND&2047);
            }
            if (s->xvel < 128)
                s->xvel+=2;
            s->ang += (TRAND&3)-6;
            break;
        case QUEBALL__STATIC:
        case STRIPEBALL__STATIC:
            if (s->xvel)
            {
                j = headspritestat[0];
                while (j >= 0)
                {
                    nextj = nextspritestat[j];
                    if (sprite[j].picnum == POCKET && ldist(&sprite[j],s) < 52) KILLIT(i);
                    j = nextj;
                }

                j = clipmove(&s->x,&s->y,&s->z,&s->sectnum,
                             (((s->xvel*(sintable[(s->ang+512)&2047]))>>14)*TICSPERFRAME)<<11,
                             (((s->xvel*(sintable[s->ang&2047]))>>14)*TICSPERFRAME)<<11,
                             24L,(4<<8),(4<<8),CLIPMASK1);

                if (j&49152)
                {
                    if ((j&49152) == 32768)
                    {
                        j &= (MAXWALLS-1);
                        k = getangle(
                                wall[wall[j].point2].x-wall[j].x,
                                wall[wall[j].point2].y-wall[j].y);
                        s->ang = ((k<<1) - s->ang)&2047;
                    }
                    else if ((j&49152) == 49152)
                    {
                        j &= (MAXSPRITES-1);
                        checkhitsprite(i,j);
                    }
                }
                s->xvel --;
                if (s->xvel < 0) s->xvel = 0;
                if (s->picnum == STRIPEBALL)
                {
                    s->cstat = 257;
                    s->cstat |= 4&s->xvel;
                    s->cstat |= 8&s->xvel;
                }
            }
            else
            {
                p = findplayer(s,&x);

                if (x < 1596)
                {

                    //                        if(s->pal == 12)
                    {
                        j = getincangle(g_player[p].ps->ang,getangle(s->x-g_player[p].ps->posx,s->y-g_player[p].ps->posy));
                        if (j > -64 && j < 64 && (g_player[p].sync->bits&(1<<29)))
                            if (g_player[p].ps->toggle_key_flag == 1)
                            {
                                a = headspritestat[1];
                                while (a >= 0)
                                {
                                    if (sprite[a].picnum == QUEBALL || sprite[a].picnum == STRIPEBALL)
                                    {
                                        j = getincangle(g_player[p].ps->ang,getangle(sprite[a].x-g_player[p].ps->posx,sprite[a].y-g_player[p].ps->posy));
                                        if (j > -64 && j < 64)
                                        {
                                            findplayer(&sprite[a],&l);
                                            if (x > l) break;
                                        }
                                    }
                                    a = nextspritestat[a];
                                }
                                if (a == -1)
                                {
                                    if (s->pal == 12)
                                        s->xvel = 164;
                                    else s->xvel = 140;
                                    s->ang = g_player[p].ps->ang;
                                    g_player[p].ps->toggle_key_flag = 2;
                                }
                            }
                    }
                }
                if (x < 512 && s->sectnum == g_player[p].ps->cursectnum)
                {
                    s->ang = getangle(s->x-g_player[p].ps->posx,s->y-g_player[p].ps->posy);
                    s->xvel = 48;
                }
            }

            break;
        case FORCESPHERE__STATIC:

            if (s->yvel == 0)
            {
                s->yvel = 1;

                for (l=512;l<(2048-512);l+= 128)
                    for (j=0;j<2048;j += 128)
                    {
                        k = spawn(i,FORCESPHERE);
                        sprite[k].cstat = 257+128;
                        sprite[k].clipdist = 64;
                        sprite[k].ang = j;
                        sprite[k].zvel = sintable[l&2047]>>5;
                        sprite[k].xvel = sintable[(l+512)&2047]>>9;
                        sprite[k].owner = i;
                    }
            }

            if (t[3] > 0)
            {
                if (s->zvel < 6144)
                    s->zvel += 192;
                s->z += s->zvel;
                if (s->z > sector[sect].floorz)
                    s->z = sector[sect].floorz;
                t[3]--;
                if (t[3] == 0)
                    KILLIT(i);
            }
            else if (t[2] > 10)
            {
                j = headspritestat[5];
                while (j >= 0)
                {
                    if (sprite[j].owner == i && sprite[j].picnum == FORCESPHERE)
                        hittype[j].temp_data[1] = 1+(TRAND&63);
                    j = nextspritestat[j];
                }
                t[3] = 64;
            }

            goto BOLT;

        case RECON__STATIC:

            getglobalz(i);

            if (sector[s->sectnum].ceilingstat&1)
                s->shade += (sector[s->sectnum].ceilingshade-s->shade)>>1;
            else s->shade += (sector[s->sectnum].floorshade-s->shade)>>1;

            if (s->z < sector[sect].ceilingz+(32<<8))
                s->z = sector[sect].ceilingz+(32<<8);

            if (ud.multimode < 2)
            {
                if (actor_tog == 1)
                {
                    s->cstat = (short)32768;
                    goto BOLT;
                }
                else if (actor_tog == 2) s->cstat = 257;
            }
            IFHIT
            {
                if (s->extra < 0 && t[0] != -1)
                {
                    t[0] = -1;
                    s->extra = 0;
                }
                spritesound(RECO_PAIN,i);
                RANDOMSCRAP;
            }

            if (t[0] == -1)
            {
                s->z += 1024;
                t[2]++;
                if ((t[2]&3) == 0) spawn(i,EXPLOSION2);
                getglobalz(i);
                s->ang += 96;
                s->xvel = 128;
                j = ssp(i,CLIPMASK0);
                if (j != 1 || s->z > hittype[i].floorz)
                {
                    for (l=0;l<16;l++)
                        RANDOMSCRAP;
                    spritesound(LASERTRIP_EXPLODE,i);
                    spawn(i,PIGCOP);
                    g_player[myconnectindex].ps->actors_killed++;
                    KILLIT(i);
                }
                goto BOLT;
            }
            else
            {
                if (s->z > hittype[i].floorz-(48<<8))
                    s->z = hittype[i].floorz-(48<<8);
            }

            p = findplayer(s,&x);
            j = s->owner;

            // 3 = findplayerz, 4 = shoot

            if (t[0] >= 4)
            {
                t[2]++;
                if ((t[2]&15) == 0)
                {
                    a = s->ang;
                    s->ang = hittype[i].tempang;
                    spritesound(RECO_ATTACK,i);
                    shoot(i,FIRELASER);
                    s->ang = a;
                }
                if (t[2] > (26*3) || !cansee(s->x,s->y,s->z-(16<<8),s->sectnum, g_player[p].ps->posx,g_player[p].ps->posy,g_player[p].ps->posz,g_player[p].ps->cursectnum))
                {
                    t[0] = 0;
                    t[2] = 0;
                }
                else hittype[i].tempang +=
                        getincangle(hittype[i].tempang,getangle(g_player[p].ps->posx-s->x,g_player[p].ps->posy-s->y))/3;
            }
            else if (t[0] == 2 || t[0] == 3)
            {
                t[3] = 0;
                if (s->xvel > 0) s->xvel -= 16;
                else s->xvel = 0;

                if (t[0] == 2)
                {
                    l = g_player[p].ps->posz-s->z;
                    if (klabs(l) < (48<<8)) t[0] = 3;
                    else s->z += ksgn(g_player[p].ps->posz-s->z)<<10;
                }
                else
                {
                    t[2]++;
                    if (t[2] > (26*3) || !cansee(s->x,s->y,s->z-(16<<8),s->sectnum, g_player[p].ps->posx,g_player[p].ps->posy,g_player[p].ps->posz,g_player[p].ps->cursectnum))
                    {
                        t[0] = 1;
                        t[2] = 0;
                    }
                    else if ((t[2]&15) == 0)
                    {
                        spritesound(RECO_ATTACK,i);
                        shoot(i,FIRELASER);
                    }
                }
                s->ang += getincangle(s->ang,getangle(g_player[p].ps->posx-s->x,g_player[p].ps->posy-s->y))>>2;
            }

            if (t[0] != 2 && t[0] != 3)
            {
                l = ldist(&sprite[j],s);
                if (l <= 1524)
                {
                    a = s->ang;
                    s->xvel >>= 1;
                }
                else a = getangle(sprite[j].x-s->x,sprite[j].y-s->y);

                if (t[0] == 1 || t[0] == 4) // Found a locator and going with it
                {
                    l = dist(&sprite[j],s);

                    if (l <= 1524)
                    {
                        if (t[0] == 1) t[0] = 0;
                        else t[0] = 5;
                    }
                    else
                    {
                        // Control speed here
                        if (l > 1524)
                        {
                            if (s->xvel < 256) s->xvel += 32;
                        }
                        else
                        {
                            if (s->xvel > 0) s->xvel -= 16;
                            else s->xvel = 0;
                        }
                    }

                    if (t[0] < 2) t[2]++;

                    if (x < 6144 && t[0] < 2 && t[2] > (26*4))
                    {
                        t[0] = 2+(TRAND&2);
                        t[2] = 0;
                        hittype[i].tempang = s->ang;
                    }
                }

                if (t[0] == 0 || t[0] == 5)
                {
                    if (t[0] == 0)
                        t[0] = 1;
                    else t[0] = 4;
                    j = s->owner = LocateTheLocator(s->hitag,-1);
                    if (j == -1)
                    {
                        s->hitag = j = hittype[i].temp_data[5];
                        s->owner = LocateTheLocator(j,-1);
                        j = s->owner;
                        if (j == -1) KILLIT(i);
                    }
                    else s->hitag++;
                }

                t[3] = getincangle(s->ang,a);
                s->ang += t[3]>>3;

                if (s->z < sprite[j].z)
                    s->z += 1024;
                else s->z -= 1024;
            }

            if (!isspritemakingsound(i,RECO_ROAM))
                spritesound(RECO_ROAM,i);

            ssp(i,CLIPMASK0);

            goto BOLT;

        case OOZ__STATIC:
        case OOZ2__STATIC:

            getglobalz(i);

            j = (hittype[i].floorz-hittype[i].ceilingz)>>9;
            if (j > 255) j = 255;

            x = 25-(j>>1);
            if (x < 8) x = 8;
            else if (x > 48) x = 48;

            s->yrepeat = j;
            s->xrepeat = x;
            s->z = hittype[i].floorz;

            goto BOLT;

        case GREENSLIME__STATIC:
            //        case GREENSLIME+1:
            //        case GREENSLIME+2:
            //        case GREENSLIME+3:
            //        case GREENSLIME+4:
            //        case GREENSLIME+5:
            //        case GREENSLIME+6:
            //        case GREENSLIME+7:

            // #ifndef VOLUMEONE
            if (ud.multimode < 2)
            {
                if (actor_tog == 1)
                {
                    s->cstat = (short)32768;
                    goto BOLT;
                }
                else if (actor_tog == 2) s->cstat = 257;
            }
            // #endif

            t[1]+=128;

            if (sector[sect].floorstat&1)
                KILLIT(i);

            p = findplayer(s,&x);

            if (x > 20480)
            {
                hittype[i].timetosleep++;
                if (hittype[i].timetosleep > SLEEPTIME)
                {
                    hittype[i].timetosleep = 0;
                    changespritestat(i,2);
                    goto BOLT;
                }
            }

            if (t[0] == -5) // FROZEN
            {
                t[3]++;
                if (t[3] > 280)
                {
                    s->pal = 0;
                    t[0] = 0;
                    goto BOLT;
                }
                makeitfall(i);
                s->cstat = 257;
                s->picnum = GREENSLIME+2;
                s->extra = 1;
                s->pal = 1;
                IFHIT
                {
                    if (j == FREEZEBLAST) goto BOLT;
                    for (j=16; j >= 0 ;j--)
                    {
                        k = EGS(SECT,SX,SY,SZ,GLASSPIECES+(j%3),-32,36,36,TRAND&2047,32+(TRAND&63),1024-(TRAND&1023),i,5);
                        sprite[k].pal = 1;
                    }
                    spritesound(GLASS_BREAKING,i);
                    KILLIT(i);
                }
                else if (x < 1024 && g_player[p].ps->quick_kick == 0)
                {
                    j = getincangle(g_player[p].ps->ang,getangle(SX-g_player[p].ps->posx,SY-g_player[p].ps->posy));
                    if (j > -128 && j < 128)
                        g_player[p].ps->quick_kick = 14;
                }

                goto BOLT;
            }

            if (x < 1596)
                s->cstat = 0;
            else s->cstat = 257;

            if (t[0] == -4) //On the player
            {
                if (sprite[g_player[p].ps->i].extra < 1)
                {
                    t[0] = 0;
                    goto BOLT;
                }

                setsprite(i,s->x,s->y,s->z);

                s->ang = g_player[p].ps->ang;

                if (((g_player[p].sync->bits&4) || (g_player[p].ps->quick_kick > 0)) && sprite[g_player[p].ps->i].extra > 0)
                    if (g_player[p].ps->quick_kick > 0 || (g_player[p].ps->curr_weapon != HANDREMOTE_WEAPON && g_player[p].ps->curr_weapon != HANDBOMB_WEAPON && g_player[p].ps->curr_weapon != TRIPBOMB_WEAPON && g_player[p].ps->ammo_amount[g_player[p].ps->curr_weapon] >= 0))
                    {
                        for (x=0;x<8;x++)
                        {
                            j = EGS(sect,s->x,s->y,s->z-(8<<8),SCRAP3+(TRAND&3),-8,48,48,TRAND&2047,(TRAND&63)+64,-(TRAND&4095)-(s->zvel>>2),i,5);
                            sprite[j].pal = 6;
                        }

                        spritesound(SLIM_DYING,i);
                        spritesound(SQUISHED,i);
                        if ((TRAND&255) < 32)
                        {
                            j = spawn(i,BLOODPOOL);
                            sprite[j].pal = 0;
                        }
                        g_player[p].ps->actors_killed ++;
                        t[0] = -3;
                        if (g_player[p].ps->somethingonplayer == i)
                            g_player[p].ps->somethingonplayer = -1;
                        KILLIT(i);
                    }

                s->z = g_player[p].ps->posz+g_player[p].ps->pyoff-t[2]+(8<<8);

                s->z += (100-g_player[p].ps->horiz)<<4;

                if (t[2] > 512)
                    t[2] -= 128;

                if (t[2] < 348)
                    t[2] += 128;

                if (g_player[p].ps->newowner >= 0)
                {
                    g_player[p].ps->newowner = -1;
                    g_player[p].ps->posx = g_player[p].ps->oposx;
                    g_player[p].ps->posy = g_player[p].ps->oposy;
                    g_player[p].ps->posz = g_player[p].ps->oposz;
                    g_player[p].ps->ang = g_player[p].ps->oang;

                    updatesector(g_player[p].ps->posx,g_player[p].ps->posy,&g_player[p].ps->cursectnum);
                    setpal(g_player[p].ps);

                    j = headspritestat[1];
                    while (j >= 0)
                    {
                        if (sprite[j].picnum==CAMERA1) sprite[j].yvel = 0;
                        j = nextspritestat[j];
                    }
                }

                if (t[3]>0)
                {
                    short frames[] = {5,5,6,6,7,7,6,5};

                    s->picnum = GREENSLIME+frames[t[3]];

                    if (t[3] == 5)
                    {
                        sprite[g_player[p].ps->i].extra += -(5+(TRAND&3));
                        spritesound(SLIM_ATTACK,i);
                    }

                    if (t[3] < 7) t[3]++;
                    else t[3] = 0;

                }
                else
                {
                    s->picnum = GREENSLIME+5;
                    if (rnd(32))
                        t[3] = 1;
                }

                s->xrepeat = 20+(sintable[t[1]&2047]>>13);
                s->yrepeat = 15+(sintable[t[1]&2047]>>13);

                s->x = g_player[p].ps->posx + (sintable[(g_player[p].ps->ang+512)&2047]>>7);
                s->y = g_player[p].ps->posy + (sintable[g_player[p].ps->ang&2047]>>7);

                goto BOLT;
            }

            else if (s->xvel < 64 && x < 768)
            {
                if (g_player[p].ps->somethingonplayer == -1)
                {
                    g_player[p].ps->somethingonplayer = i;
                    if (t[0] == 3 || t[0] == 2) //Falling downward
                        t[2] = (12<<8);
                    else t[2] = -(13<<8); //Climbing up duke
                    t[0] = -4;
                }
            }

            IFHIT
            {
                spritesound(SLIM_DYING,i);

                g_player[p].ps->actors_killed ++;
                if (g_player[p].ps->somethingonplayer == i)
                    g_player[p].ps->somethingonplayer = -1;

                if (j == FREEZEBLAST)
                {
                    spritesound(SOMETHINGFROZE,i);
                    t[0] = -5 ;
                    t[3] = 0 ;
                    goto BOLT;
                }

                if ((TRAND&255) < 32)
                {
                    j = spawn(i,BLOODPOOL);
                    sprite[j].pal = 0;
                }

                for (x=0;x<8;x++)
                {
                    j = EGS(sect,s->x,s->y,s->z-(8<<8),SCRAP3+(TRAND&3),-8,48,48,TRAND&2047,(TRAND&63)+64,-(TRAND&4095)-(s->zvel>>2),i,5);
                    sprite[j].pal = 6;
                }
                t[0] = -3;
                KILLIT(i);
            }
            // All weap
            if (t[0] == -1) //Shrinking down
            {
                makeitfall(i);

                s->cstat &= 65535-8;
                s->picnum = GREENSLIME+4;

                //                    if(s->yrepeat > 62)
                //                      guts(s,JIBS6,5,myconnectindex);

                if (s->xrepeat > 32) s->xrepeat -= TRAND&7;
                if (s->yrepeat > 16) s->yrepeat -= TRAND&7;
                else
                {
                    s->xrepeat = 40;
                    s->yrepeat = 16;
                    t[5] = -1;
                    t[0] = 0;
                }

                goto BOLT;
            }
            else if (t[0] != -2) getglobalz(i);

            if (t[0] == -2) //On top of somebody
            {
                makeitfall(i);
                sprite[t[5]].xvel = 0;

                l = sprite[t[5]].ang;

                s->z = sprite[t[5]].z;
                s->x = sprite[t[5]].x+(sintable[(l+512)&2047]>>11);
                s->y = sprite[t[5]].y+(sintable[l&2047]>>11);

                s->picnum =  GREENSLIME+2+(global_random&1);

                if (s->yrepeat < 64) s->yrepeat+=2;
                else
                {
                    if (s->xrepeat < 32) s->xrepeat += 4;
                    else
                    {
                        t[0] = -1;
                        x = ldist(s,&sprite[t[5]]);
                        if (x < 768)
                        {
                            sprite[t[5]].xrepeat = 0;

                            // JBF 20041129: a slimer eating another enemy really ought
                            // to decrease the maximum kill count by one.
                            if (sprite[t[5]].extra > 0) g_player[myconnectindex].ps->max_actors_killed--;
                        }
                    }
                }

                goto BOLT;
            }

            //Check randomly to see of there is an actor near
            if (rnd(32))
            {
                j = headspritesect[sect];
                while (j>=0)
                {
                    switch (dynamictostatic[sprite[j].picnum])
                    {
                    case LIZTROOP__STATIC:
                    case LIZMAN__STATIC:
                    case PIGCOP__STATIC:
                    case NEWBEAST__STATIC:
                        if (ldist(s,&sprite[j]) < 768 && (klabs(s->z-sprite[j].z)<8192))   //Gulp them
                        {
                            t[5] = j;
                            t[0] = -2;
                            t[1] = 0;
                            goto BOLT;
                        }
                    }

                    j = nextspritesect[j];
                }
            }

            //Moving on the ground or ceiling

            if (t[0] == 0 || t[0] == 2)
            {
                s->picnum = GREENSLIME;

                if ((TRAND&511) == 0)
                    spritesound(SLIM_ROAM,i);

                if (t[0]==2)
                {
                    s->zvel = 0;
                    s->cstat &= (65535-8);

                    if ((sector[sect].ceilingstat&1) || (hittype[i].ceilingz+6144) < s->z)
                    {
                        s->z += 2048;
                        t[0] = 3;
                        goto BOLT;
                    }
                }
                else
                {
                    s->cstat |= 8;
                    makeitfall(i);
                }

                if (everyothertime&1) ssp(i,CLIPMASK0);

                if (s->xvel > 96)
                {
                    s->xvel -= 2;
                    goto BOLT;
                }
                else
                {
                    if (s->xvel < 32) s->xvel += 4;
                    s->xvel = 64 - (sintable[(t[1]+512)&2047]>>9);

                    s->ang += getincangle(s->ang,
                                          getangle(g_player[p].ps->posx-s->x,g_player[p].ps->posy-s->y))>>3;
                    // TJR
                }

                s->xrepeat = 36 + (sintable[(t[1]+512)&2047]>>11);
                s->yrepeat = 16 + (sintable[t[1]&2047]>>13);

                if (rnd(4) && (sector[sect].ceilingstat&1) == 0 &&
                        klabs(hittype[i].floorz-hittype[i].ceilingz)
                        < (192<<8))
                {
                    s->zvel = 0;
                    t[0]++;
                }

            }

            if (t[0]==1)
            {
                s->picnum = GREENSLIME;
                if (s->yrepeat < 40) s->yrepeat+=8;
                if (s->xrepeat > 8) s->xrepeat-=4;
                if (s->zvel > -(2048+1024))
                    s->zvel -= 348;
                s->z += s->zvel;
                if (s->z < hittype[i].ceilingz+4096)
                {
                    s->z = hittype[i].ceilingz+4096;
                    s->xvel = 0;
                    t[0] = 2;
                }
            }

            if (t[0]==3)
            {
                s->picnum = GREENSLIME+1;

                makeitfall(i);

                if (s->z > hittype[i].floorz-(8<<8))
                {
                    s->yrepeat-=4;
                    s->xrepeat+=2;
                }
                else
                {
                    if (s->yrepeat < (40-4)) s->yrepeat+=8;
                    if (s->xrepeat > 8) s->xrepeat-=4;
                }

                if (s->z > hittype[i].floorz-2048)
                {
                    s->z = hittype[i].floorz-2048;
                    t[0] = 0;
                    s->xvel = 0;
                }
            }
            goto BOLT;

        case BOUNCEMINE__STATIC:
        case MORTER__STATIC:
            j = spawn(i,(PLUTOPAK?FRAMEEFFECT1:FRAMEEFFECT1_13));
            hittype[j].temp_data[0] = 3;

        case HEAVYHBOMB__STATIC:

            if ((s->cstat&32768))
            {
                t[2]--;
                if (t[2] <= 0)
                {
                    spritesound(TELEPORTER,i);
                    spawn(i,TRANSPORTERSTAR);
                    s->cstat = 257;
                }
                goto BOLT;
            }

            p = findplayer(s,&x);

            if (x < 1220) s->cstat &= ~257;
            else s->cstat |= 257;

            if (t[3] == 0)
            {
                j = ifhitbyweapon(i);
                if (j >= 0)
                {
                    t[3] = 1;
                    t[4] = 0;
                    l = 0;
                    s->xvel = 0;
                    goto DETONATEB;
                }
            }

            if (s->picnum != BOUNCEMINE)
            {
                makeitfall(i);

                if ((sector[sect].lotag != 1 || hittype[i].floorz != sector[sect].floorz) && s->z >= hittype[i].floorz-(FOURSLEIGHT) && s->yvel < 3)
                {
                    if (s->yvel > 0 || (s->yvel == 0 && hittype[i].floorz == sector[sect].floorz))
                        spritesound(PIPEBOMB_BOUNCE,i);
                    s->zvel = -((4-s->yvel)<<8);
                    if (sector[s->sectnum].lotag== 2)
                        s->zvel >>= 2;
                    s->yvel++;
                }
                if (s->z < hittype[i].ceilingz)   // && sector[sect].lotag != 2 )
                {
                    s->z = hittype[i].ceilingz+(3<<8);
                    s->zvel = 0;
                }
            }

            j = movesprite(i,
                           (s->xvel*(sintable[(s->ang+512)&2047]))>>14,
                           (s->xvel*(sintable[s->ang&2047]))>>14,
                           s->zvel,CLIPMASK0);

            if (sector[SECT].lotag == 1 && s->zvel == 0 && hittype[i].floorz == sector[sect].floorz)
            {
                s->z += (32<<8);
                if (t[5] == 0)
                {
                    t[5] = 1;
                    spawn(i,WATERSPLASH2);
                }
            }
            else t[5] = 0;

            if (t[3] == 0 && (s->picnum == BOUNCEMINE || s->picnum == MORTER) && (j || x < 844))
            {
                t[3] = 1;
                t[4] = 0;
                l = 0;
                s->xvel = 0;
                goto DETONATEB;
            }

            if (sprite[s->owner].picnum == APLAYER)
                l = sprite[s->owner].yvel;
            else l = -1;

            if (s->xvel > 0)
            {
                s->xvel -= 5;
                if (sector[sect].lotag == 2)
                    s->xvel -= 10;

                if (s->xvel < 0)
                    s->xvel = 0;
                if (s->xvel&8) s->cstat ^= 4;
            }

            if ((j&49152) == 32768)
            {
                j &= (MAXWALLS-1);

                checkhitwall(i,j,s->x,s->y,s->z,s->picnum);

                k = getangle(
                        wall[wall[j].point2].x-wall[j].x,
                        wall[wall[j].point2].y-wall[j].y);

                s->ang = ((k<<1) - s->ang)&2047;
                s->xvel >>= 1;
            }

            //      int lPipeBombControl=GetGameVar("PIPEBOMB_CONTROL", PIPEBOMB_REMOTE, -1, -1);

DETONATEB:

            //  if(lPipeBombControl & PIPEBOMB_TIMER)
            //       {

            if (s->picnum == HEAVYHBOMB && hittype[i].temp_data[6] == 1)
            {
                /*                if(s->extra >= 1)
                                {
                                    s->extra--;
                                }

                                if(s->extra <= 0)
                                    s->lotag=911;
                */

                if (hittype[i].temp_data[7] >= 1)
                {
                    hittype[i].temp_data[7]--;
                }

                if (hittype[i].temp_data[7] <= 0)
                {
                    hittype[i].temp_data[6] = 3;
                    //                    s->extra = *actorscrptr[s->picnum];
                }
            }
            //      }

            if ((l >= 0 && g_player[l].ps->hbomb_on == 0 && hittype[i].temp_data[6] == 2) || t[3] == 1)
                hittype[i].temp_data[6] = 3;

            if (hittype[i].temp_data[6] == 3)

            {
                t[4]++;

                if (t[4] == 2)
                {
                    x = s->extra;
                    m = 0;
                    switch (dynamictostatic[s->picnum])
                    {
                    case HEAVYHBOMB__STATIC:
                        m = pipebombblastradius;
                        break;
                    case MORTER__STATIC:
                        m = morterblastradius;
                        break;
                    case BOUNCEMINE__STATIC:
                        m = bouncemineblastradius;
                        break;
                    }

                    hitradius(i, m,x>>2,x>>1,x-(x>>2),x);
                    spawn(i,EXPLOSION2);
                    if (s->zvel == 0)
                        spawn(i,EXPLOSION2BOT);
                    spritesound(PIPEBOMB_EXPLODE,i);
                    for (x=0;x<8;x++)
                        RANDOMSCRAP;
                }

                if (s->yrepeat)
                {
                    s->yrepeat = 0;
                    goto BOLT;
                }

                if (t[4] > 20)
                {
                    if (s->owner != i || ud.respawn_items == 0)
                    {
                        KILLIT(i);
                    }
                    else
                    {
                        t[2] = respawnitemtime;
                        spawn(i,RESPAWNMARKERRED);
                        s->cstat = (short) 32768;
                        s->yrepeat = 9;
                        goto BOLT;
                    }
                }
            }
            else if (s->picnum == HEAVYHBOMB && x < 788 && t[0] > 7 && s->xvel == 0)
                if (cansee(s->x,s->y,s->z-(8<<8),s->sectnum,g_player[p].ps->posx,g_player[p].ps->posy,g_player[p].ps->posz,g_player[p].ps->cursectnum))
                    if (g_player[p].ps->ammo_amount[HANDBOMB_WEAPON] < g_player[p].ps->max_ammo_amount[HANDBOMB_WEAPON])
                    {
                        if ((gametype_flags[ud.coop] & GAMETYPE_FLAG_WEAPSTAY) && s->owner == i)
                        {
                            for (j=0;j<g_player[p].ps->weapreccnt;j++)
                                if (g_player[p].ps->weaprecs[j] == s->picnum)
                                    goto BOLT;

                            if (g_player[p].ps->weapreccnt < 255)
                                g_player[p].ps->weaprecs[g_player[p].ps->weapreccnt++] = s->picnum;
                        }

                        addammo(HANDBOMB_WEAPON,g_player[p].ps,1);
                        spritesound(DUKE_GET,g_player[p].ps->i);

                        if (g_player[p].ps->gotweapon[HANDBOMB_WEAPON] == 0 || s->owner == g_player[p].ps->i)
                        {
                            /* addweapon(g_player[p].ps,HANDBOMB_WEAPON); */
                            if (!(g_player[p].ps->weaponswitch & 1) && *aplWeaponWorksLike[g_player[p].ps->curr_weapon] != HANDREMOTE_WEAPON)
                                addweaponnoswitch(g_player[p].ps,HANDBOMB_WEAPON);
                            else addweapon(g_player[p].ps,HANDBOMB_WEAPON);
                        }

                        if (sprite[s->owner].picnum != APLAYER)
                        {
                            g_player[p].ps->pals[0] = 0;
                            g_player[p].ps->pals[1] = 32;
                            g_player[p].ps->pals[2] = 0;
                            g_player[p].ps->pals_time = 32;
                        }

                        if (s->owner != i || ud.respawn_items == 0)
                        {
                            if (s->owner == i && (gametype_flags[ud.coop] & GAMETYPE_FLAG_WEAPSTAY))
                                goto BOLT;
                            KILLIT(i);
                        }
                        else
                        {
                            t[2] = respawnitemtime;
                            spawn(i,RESPAWNMARKERRED);
                            s->cstat = (short) 32768;
                        }
                    }

            if (t[0] < 8) t[0]++;
            goto BOLT;

        case REACTORBURNT__STATIC:
        case REACTOR2BURNT__STATIC:
            goto BOLT;

        case REACTOR__STATIC:
        case REACTOR2__STATIC:

            if (t[4] == 1)
            {
                j = headspritesect[sect];
                while (j >= 0)
                {
                    switch (dynamictostatic[sprite[j].picnum])
                    {
                    case SECTOREFFECTOR__STATIC:
                        if (sprite[j].lotag == 1)
                        {
                            sprite[j].lotag = (short) 65535;
                            sprite[j].hitag = (short) 65535;
                        }
                        break;
                    case REACTOR__STATIC:
                        sprite[j].picnum = REACTORBURNT;
                        break;
                    case REACTOR2__STATIC:
                        sprite[j].picnum = REACTOR2BURNT;
                        break;
                    case REACTORSPARK__STATIC:
                    case REACTOR2SPARK__STATIC:
                        sprite[j].cstat = (short) 32768;
                        break;
                    }
                    j = nextspritesect[j];
                }
                goto BOLT;
            }

            if (t[1] >= 20)
            {
                t[4] = 1;
                goto BOLT;
            }

            p = findplayer(s,&x);

            t[2]++;
            if (t[2] == 4) t[2]=0;

            if (x < 4096)
            {
                if ((TRAND&255) < 16)
                {
                    if (!isspritemakingsound(g_player[p].ps->i, DUKE_LONGTERM_PAIN))
                        spritesound(DUKE_LONGTERM_PAIN,g_player[p].ps->i);

                    spritesound(SHORT_CIRCUIT,i);

                    sprite[g_player[p].ps->i].extra --;
                    g_player[p].ps->pals_time = 32;
                    g_player[p].ps->pals[0] = 32;
                    g_player[p].ps->pals[1] = 0;
                    g_player[p].ps->pals[2] = 0;
                }
                t[0] += 128;
                if (t[3] == 0)
                    t[3] = 1;
            }
            else t[3] = 0;

            if (t[1])
            {
                t[1]++;

                t[4] = s->z;
                s->z = sector[sect].floorz-(TRAND%(sector[sect].floorz-sector[sect].ceilingz));

                switch (t[1])
                {
                case 3:
                    //Turn on all of those flashing sectoreffector.
                    hitradius(i, 4096,
                              impact_damage<<2,
                              impact_damage<<2,
                              impact_damage<<2,
                              impact_damage<<2);
                    /*
                                                j = headspritestat[3];
                                                while(j>=0)
                                                {
                                                    if( sprite[j].lotag  == 3 )
                                                        hittype[j].temp_data[4]=1;
                                                    else if(sprite[j].lotag == 12)
                                                    {
                                                        hittype[j].temp_data[4] = 1;
                                                        sprite[j].lotag = 3;
                                                        sprite[j].owner = 0;
                                                        hittype[j].temp_data[0] = s->shade;
                                                    }
                                                    j = nextspritestat[j];
                                                }
                    */
                    j = headspritestat[6];
                    while (j >= 0)
                    {
                        if (sprite[j].picnum == MASTERSWITCH)
                            if (sprite[j].hitag == s->hitag)
                                if (sprite[j].yvel == 0)
                                    sprite[j].yvel = 1;
                        j = nextspritestat[j];
                    }
                    break;

                case 4:
                case 7:
                case 10:
                case 15:
                    j = headspritesect[sect];
                    while (j >= 0)
                    {
                        l = nextspritesect[j];

                        if (j != i)
                        {
                            deletesprite(j);
                            break;
                        }
                        j = l;
                    }
                    break;
                }
                for (x=0;x<16;x++)
                    RANDOMSCRAP;

                s->z = t[4];
                t[4] = 0;

            }
            else
            {
                IFHIT
                {
                    for (x=0;x<32;x++)
                        RANDOMSCRAP;
                    if (s->extra < 0)
                        t[1] = 1;
                }
            }
            goto BOLT;

        case CAMERA1__STATIC:

            if (t[0] == 0)
            {
                t[1]+=8;
                if (camerashitable)
                {
                    IFHIT
                    {
                        t[0] = 1; // static
                        s->cstat = (short)32768;
                        for (x=0;x<5;x++) RANDOMSCRAP;
                        goto BOLT;
                    }
                }

                if (s->hitag > 0)
                {
                    if (t[1]<s->hitag)
                        s->ang+=8;
                    else if (t[1]<(s->hitag*3))
                        s->ang-=8;
                    else if (t[1] < (s->hitag<<2))
                        s->ang+=8;
                    else
                    {
                        t[1]=8;
                        s->ang+=16;
                    }
                }
            }
            goto BOLT;
        }


        // #ifndef VOLOMEONE
        if (ud.multimode < 2 && badguy(s))
        {
            if (actor_tog == 1)
            {
                s->cstat = (short)32768;
                goto BOLT;
            }
            else if (actor_tog == 2)
            {
                s->cstat = 0;
                if (s->extra)
                    s->cstat = 257;
            }
        }
        // #endif

        p = findplayer(s,&x);

        execute(i,p,x);

BOLT:

        i = nexti;
    }

}

static void moveexplosions(void)  // STATNUM 5
{
    short i, j, nexti, sect, p;
    int l, x, *t;
    spritetype *s;
    int switchpicnum;

    i = headspritestat[5];
    while (i >= 0)
    {
        nexti = nextspritestat[i];

        t = &hittype[i].temp_data[0];
        s = &sprite[i];
        sect = s->sectnum;

        if (sect < 0 || s->xrepeat == 0) KILLIT(i);

        hittype[i].bposx = s->x;
        hittype[i].bposy = s->y;
        hittype[i].bposz = s->z;
        switchpicnum = s->picnum;
        if ((s->picnum > NUKEBUTTON)&&(s->picnum <= NUKEBUTTON+3))
        {
            switchpicnum = NUKEBUTTON;
        }
        if ((s->picnum > GLASSPIECES)&&(s->picnum <= GLASSPIECES+2))
        {
            switchpicnum = GLASSPIECES;
        }
        if (s->picnum ==INNERJAW+1)
        {
            switchpicnum--;
        }
        if ((s->picnum == MONEY+1) || (s->picnum == MAIL+1) || (s->picnum == PAPER+1))
            hittype[i].floorz = s->z = getflorzofslope(s->sectnum,s->x,s->y);
        else switch (dynamictostatic[switchpicnum])
            {
            case NEON1__STATIC:
            case NEON2__STATIC:
            case NEON3__STATIC:
            case NEON4__STATIC:
            case NEON5__STATIC:
            case NEON6__STATIC:

                if ((global_random/(s->lotag+1)&31) > 4) s->shade = -127;
                else s->shade = 127;
                goto BOLT;

            case BLOODSPLAT1__STATIC:
            case BLOODSPLAT2__STATIC:
            case BLOODSPLAT3__STATIC:
            case BLOODSPLAT4__STATIC:

                if (t[0] == 7*26) goto BOLT;
                s->z += 16+(TRAND&15);
                t[0]++;
                if ((t[0]%9) == 0) s->yrepeat++;
                goto BOLT;

            case NUKEBUTTON__STATIC:
                //        case NUKEBUTTON+1:
                //        case NUKEBUTTON+2:
                //        case NUKEBUTTON+3:

                if (t[0])
                {
                    t[0]++;
                    if (t[0] == 8) s->picnum = NUKEBUTTON+1;
                    else if (t[0] == 16)
                    {
                        s->picnum = NUKEBUTTON+2;
                        g_player[sprite[s->owner].yvel].ps->fist_incs = 1;
                    }
                    if (g_player[sprite[s->owner].yvel].ps->fist_incs == 26)
                        s->picnum = NUKEBUTTON+3;
                }
                goto BOLT;

            case FORCESPHERE__STATIC:

                l = s->xrepeat;
                if (t[1] > 0)
                {
                    t[1]--;
                    if (t[1] == 0)
                    {
                        KILLIT(i);
                    }
                }
                if (hittype[s->owner].temp_data[1] == 0)
                {
                    if (t[0] < 64)
                    {
                        t[0]++;
                        l += 3;
                    }
                }
                else
                    if (t[0] > 64)
                    {
                        t[0]--;
                        l -= 3;
                    }

                s->x = sprite[s->owner].x;
                s->y = sprite[s->owner].y;
                s->z = sprite[s->owner].z;
                s->ang += hittype[s->owner].temp_data[0];

                if (l > 64) l = 64;
                else if (l < 1) l = 1;

                s->xrepeat = l;
                s->yrepeat = l;
                s->shade = (l>>1)-48;

                for (j=t[0];j > 0;j--)
                    ssp(i,CLIPMASK0);
                goto BOLT;
            case WATERSPLASH2__STATIC:

                t[0]++;
                if (t[0] == 1)
                {
                    if (sector[sect].lotag != 1 && sector[sect].lotag != 2)
                        KILLIT(i);
                    /*                    else
                                        {
                                            l = getflorzofslope(sect,s->x,s->y)-s->z;
                                            if( l > (16<<8) ) KILLIT(i);
                                        }
                                        else */
                    if (!issoundplaying(i,ITEM_SPLASH))
                        spritesound(ITEM_SPLASH,i);
                }
                if (t[0] == 3)
                {
                    t[0] = 0;
                    t[1]++;
                }
                if (t[1] == 5)
                    deletesprite(i);
                goto BOLT;
            case FRAMEEFFECT1_13__STATIC:
                if (PLUTOPAK) goto BOLT;	// JBF: ideally this should never happen...
            case FRAMEEFFECT1__STATIC:

                if (s->owner >= 0)
                {
                    t[0]++;

                    if (t[0] > 7)
                    {
                        KILLIT(i);
                    }
                    else if (t[0] > 4)
                        s->cstat |= 512+2;
                    else if (t[0] > 2)
                        s->cstat |= 2;
                    s->xoffset = sprite[s->owner].xoffset;
                    s->yoffset = sprite[s->owner].yoffset;
                }
                goto BOLT;
            case INNERJAW__STATIC:
                //        case INNERJAW+1:

                p = findplayer(s,&x);
                if (x < 512)
                {
                    g_player[p].ps->pals_time = 32;
                    g_player[p].ps->pals[0] = 32;
                    g_player[p].ps->pals[1] = 0;
                    g_player[p].ps->pals[2] = 0;
                    sprite[g_player[p].ps->i].extra -= 4;
                }

            case FIRELASER__STATIC:
                if (s->extra != 999)
                    s->extra = 999;
                else KILLIT(i);
                break;
            case TONGUE__STATIC:
                KILLIT(i);

            case MONEY__STATIC:
            case MAIL__STATIC:
            case PAPER__STATIC:

                s->xvel = (TRAND&7)+(sintable[T1&2047]>>9);
                T1 += (TRAND&63);
                if ((T1&2047) > 512 && (T1&2047) < 1596)
                {
                    if (sector[sect].lotag == 2)
                    {
                        if (s->zvel < 64)
                            s->zvel += (gc>>5)+(TRAND&7);
                    }
                    else
                        if (s->zvel < 144)
                            s->zvel += (gc>>5)+(TRAND&7);
                }

                ssp(i,CLIPMASK0);

                if ((TRAND&3) == 0)
                    setsprite(i,s->x,s->y,s->z);

                if (s->sectnum == -1) KILLIT(i);
                l = getflorzofslope(s->sectnum,s->x,s->y);

                if (s->z > l)
                {
                    s->z = l;

                    insertspriteq(i);
                    PN ++;

                    j = headspritestat[5];
                    while (j >= 0)
                    {
                        if (sprite[j].picnum == BLOODPOOL)
                            if (ldist(s,&sprite[j]) < 348)
                            {
                                s->pal = 2;
                                break;
                            }
                        j = nextspritestat[j];
                    }
                }

                break;

            case JIBS1__STATIC:
            case JIBS2__STATIC:
            case JIBS3__STATIC:
            case JIBS4__STATIC:
            case JIBS5__STATIC:
            case JIBS6__STATIC:
            case HEADJIB1__STATIC:
            case ARMJIB1__STATIC:
            case LEGJIB1__STATIC:
            case LIZMANHEAD1__STATIC:
            case LIZMANARM1__STATIC:
            case LIZMANLEG1__STATIC:
            case DUKETORSO__STATIC:
            case DUKEGUN__STATIC:
            case DUKELEG__STATIC:

                if (s->xvel > 0) s->xvel--;
                else s->xvel = 0;

                if (t[5] < 30*10)
                    t[5]++;
                else
                {
                    KILLIT(i);
                }


                if (s->zvel > 1024 && s->zvel < 1280)
                {
                    setsprite(i,s->x,s->y,s->z);
                    sect = s->sectnum;
                }

                l = getflorzofslope(sect,s->x,s->y);
                x = getceilzofslope(sect,s->x,s->y);
                if (x == l || sect < 0 || sect >= MAXSECTORS) KILLIT(i);

                if (s->z < l-(2<<8))
                {
                    if (t[1] < 2) t[1]++;
                    else if (sector[sect].lotag != 2)
                    {
                        t[1] = 0;
                        if (s->picnum == DUKELEG || s->picnum == DUKETORSO || s->picnum == DUKEGUN)
                        {
                            if (t[0] > 6) t[0] = 0;
                            else t[0]++;
                        }
                        else
                        {
                            if (t[0] > 2)
                                t[0] = 0;
                            else t[0]++;
                        }
                    }

                    if (s->zvel < 6144)
                    {
                        if (sector[sect].lotag == 2)
                        {
                            if (s->zvel < 1024)
                                s->zvel += 48;
                            else s->zvel = 1024;
                        }
                        else s->zvel += gc-50;
                    }

                    s->x += (s->xvel*sintable[(s->ang+512)&2047])>>14;
                    s->y += (s->xvel*sintable[s->ang&2047])>>14;
                    s->z += s->zvel;

                }
                else
                {
                    if (t[2] == 0)
                    {
                        if (s->sectnum == -1)
                        {
                            KILLIT(i);
                        }
                        if ((sector[s->sectnum].floorstat&2))
                        {
                            KILLIT(i);
                        }
                        t[2]++;
                    }
                    l = getflorzofslope(s->sectnum,s->x,s->y);

                    s->z = l-(2<<8);
                    s->xvel = 0;

                    if (s->picnum == JIBS6)
                    {
                        t[1]++;
                        if ((t[1]&3) == 0 && t[0] < 7)
                            t[0]++;
                        if (t[1] > 20) KILLIT(i);
                    }
                    else
                    {
                        s->picnum = JIBS6;
                        t[0] = 0;
                        t[1] = 0;
                    }

                }
                goto BOLT;

            case BLOODPOOL__STATIC:
            case PUKE__STATIC:

                if (t[0] == 0)
                {
                    t[0] = 1;
                    if (sector[sect].floorstat&2)
                    {
                        KILLIT(i);
                    }
                    else insertspriteq(i);
                }

                makeitfall(i);

                p = findplayer(s,&x);

                s->z = hittype[i].floorz-(FOURSLEIGHT);

                if (t[2] < 32)
                {
                    t[2]++;
                    if (hittype[i].picnum == TIRE)
                    {
                        if (s->xrepeat < 64 && s->yrepeat < 64)
                        {
                            s->xrepeat += TRAND&3;
                            s->yrepeat += TRAND&3;
                        }
                    }
                    else
                    {
                        if (s->xrepeat < 32 && s->yrepeat < 32)
                        {
                            s->xrepeat += TRAND&3;
                            s->yrepeat += TRAND&3;
                        }
                    }
                }

                if (x < 844 && s->xrepeat > 6 && s->yrepeat > 6)
                {
                    if (s->pal == 0 && (TRAND&255) < 16 && s->picnum != PUKE)
                    {
                        if (g_player[p].ps->boot_amount > 0)
                            g_player[p].ps->boot_amount--;
                        else
                        {
                            if (!isspritemakingsound(g_player[p].ps->i,DUKE_LONGTERM_PAIN))
                                spritesound(DUKE_LONGTERM_PAIN,g_player[p].ps->i);
                            sprite[g_player[p].ps->i].extra --;
                            g_player[p].ps->pals_time = 32;
                            g_player[p].ps->pals[0] = 16;
                            g_player[p].ps->pals[1] = 0;
                            g_player[p].ps->pals[2] = 0;
                        }
                    }

                    if (t[1] == 1) goto BOLT;
                    t[1] = 1;

                    if (hittype[i].picnum == TIRE)
                        g_player[p].ps->footprintcount = 10;
                    else g_player[p].ps->footprintcount = 3;

                    g_player[p].ps->footprintpal = s->pal;
                    g_player[p].ps->footprintshade = s->shade;

                    if (t[2] == 32)
                    {
                        s->xrepeat -= 6;
                        s->yrepeat -= 6;
                    }
                }
                else t[1] = 0;
                goto BOLT;

            case BURNING__STATIC:
            case BURNING2__STATIC:
            case FECES__STATIC:
            case WATERBUBBLE__STATIC:
            case SMALLSMOKE__STATIC:
            case EXPLOSION2__STATIC:
            case SHRINKEREXPLOSION__STATIC:
            case EXPLOSION2BOT__STATIC:
            case BLOOD__STATIC:
            case LASERSITE__STATIC:
            case FORCERIPPLE__STATIC:
            case TRANSPORTERSTAR__STATIC:
            case TRANSPORTERBEAM__STATIC:
                p = findplayer(s,&x);
                execute(i,p,x);
                goto BOLT;

            case SHELL__STATIC:
            case SHOTGUNSHELL__STATIC:

                ssp(i,CLIPMASK0);

                if (sect < 0 || (sector[sect].floorz + 256) < s->z) KILLIT(i);

                if (sector[sect].lotag == 2)
                {
                    t[1]++;
                    if (t[1] > 8)
                    {
                        t[1] = 0;
                        t[0]++;
                        t[0] &= 3;
                    }
                    if (s->zvel < 128) s->zvel += (gc/13); // 8
                    else s->zvel -= 64;
                    if (s->xvel > 0)
                        s->xvel -= 4;
                    else s->xvel = 0;
                }
                else
                {
                    t[1]++;
                    if (t[1] > 3)
                    {
                        t[1] = 0;
                        t[0]++;
                        t[0] &= 3;
                    }
                    if (s->zvel < 512) s->zvel += (gc/3); // 52;
                    if (s->xvel > 0)
                        s->xvel --;
                    //                else KILLIT(i);
                }

                goto BOLT;

            case GLASSPIECES__STATIC:
                //        case GLASSPIECES+1:
                //        case GLASSPIECES+2:

                makeitfall(i);

                if (s->zvel > 4096) s->zvel = 4096;
                if (sect < 0) KILLIT(i);

                if (s->z == hittype[i].floorz-(FOURSLEIGHT) && t[0] < 3)
                {
                    s->zvel = -((3-t[0])<<8)-(TRAND&511);
                    if (sector[sect].lotag == 2)
                        s->zvel >>= 1;
                    s->xrepeat >>= 1;
                    s->yrepeat >>= 1;
                    if (rnd(96))
                        setsprite(i,s->x,s->y,s->z);
                    t[0]++;//Number of bounces
                }
                else if (t[0] == 3) KILLIT(i);

                if (s->xvel > 0)
                {
                    s->xvel -= 2;
                    s->cstat = ((s->xvel&3)<<2);
                }
                else s->xvel = 0;

                ssp(i,CLIPMASK0);

                goto BOLT;
            }

        IFWITHIN(SCRAP6,SCRAP5+3)
        {
            if (s->xvel > 0)
                s->xvel--;
            else s->xvel = 0;

            if (s->zvel > 1024 && s->zvel < 1280)
            {
                setsprite(i,s->x,s->y,s->z);
                sect = s->sectnum;
            }

            if (s->z < sector[sect].floorz-(2<<8))
            {
                if (t[1] < 1) t[1]++;
                else
                {
                    t[1] = 0;

                    if (s->picnum < SCRAP6+8)
                    {
                        if (t[0] > 6)
                            t[0] = 0;
                        else t[0]++;
                    }
                    else
                    {
                        if (t[0] > 2)
                            t[0] = 0;
                        else t[0]++;
                    }
                }
                if (s->zvel < 4096) s->zvel += gc-50;
                s->x += (s->xvel*sintable[(s->ang+512)&2047])>>14;
                s->y += (s->xvel*sintable[s->ang&2047])>>14;
                s->z += s->zvel;
            }
            else
            {
                if (s->picnum == SCRAP1 && s->yvel > 0)
                {
                    j = spawn(i,s->yvel);
                    setsprite(j,s->x,s->y,s->z);
                    getglobalz(j);
                    sprite[j].hitag = sprite[j].lotag = 0;
                }
                KILLIT(i);
            }
            goto BOLT;
        }

BOLT:
        i = nexti;
    }
}

static void moveeffectors(void)   //STATNUM 3
{
    int q=0, l, m, x, st, j, *t;
    int i = headspritestat[3], nexti, nextk, p, sh, nextj;
    short k;
    spritetype *s;
    sectortype *sc;
    walltype *wal;

    fricxv = fricyv = 0;

    while (i >= 0)
    {
        nexti = nextspritestat[i];
        s = &sprite[i];

        sc = &sector[s->sectnum];
        st = s->lotag;
        sh = s->hitag;

        t = &hittype[i].temp_data[0];

        switch (st)
        {
        case 0:
        {
            int zchange = 0;

            zchange = 0;

            j = s->owner;

            if (sprite[j].lotag == (short) 65535)
                KILLIT(i);

            q = sc->extra>>3;
            l = 0;

            if (sc->lotag == 30)
            {
                q >>= 2;

                if (sprite[i].extra == 1)
                {
                    if (hittype[i].tempang < 256)
                    {
                        hittype[i].tempang += 4;
                        if (hittype[i].tempang >= 256)
                            callsound(s->sectnum,i);
                        if (s->clipdist) l = 1;
                        else l = -1;
                    }
                    else hittype[i].tempang = 256;

                    if (sc->floorz > s->z)   //z's are touching
                    {
                        sc->floorz -= 512;
                        zchange = -512;
                        if (sc->floorz < s->z)
                            sc->floorz = s->z;
                    }

                    else if (sc->floorz < s->z)   //z's are touching
                    {
                        sc->floorz += 512;
                        zchange = 512;
                        if (sc->floorz > s->z)
                            sc->floorz = s->z;
                    }
                }
                else if (sprite[i].extra == 3)
                {
                    if (hittype[i].tempang > 0)
                    {
                        hittype[i].tempang -= 4;
                        if (hittype[i].tempang <= 0)
                            callsound(s->sectnum,i);
                        if (s->clipdist) l = -1;
                        else l = 1;
                    }
                    else hittype[i].tempang = 0;

                    if (sc->floorz > T4)   //z's are touching
                    {
                        sc->floorz -= 512;
                        zchange = -512;
                        if (sc->floorz < T4)
                            sc->floorz = T4;
                    }

                    else if (sc->floorz < T4)   //z's are touching
                    {
                        sc->floorz += 512;
                        zchange = 512;
                        if (sc->floorz > T4)
                            sc->floorz = T4;
                    }
                }

                s->ang += (l*q);
                t[2] += (l*q);
            }
            else
            {
                if (hittype[j].temp_data[0] == 0) break;
                if (hittype[j].temp_data[0] == 2) KILLIT(i);

                if (sprite[j].ang > 1024)
                    l = -1;
                else l = 1;
                if (t[3] == 0)
                    t[3] = ldist(s,&sprite[j]);
                s->xvel = t[3];
                s->x = sprite[j].x;
                s->y = sprite[j].y;
                s->ang += (l*q);
                t[2] += (l*q);
            }

            if (l && (sc->floorstat&64))
            {
                for (p=connecthead;p>=0;p=connectpoint2[p])
                {
                    if (g_player[p].ps->cursectnum == s->sectnum && g_player[p].ps->on_ground == 1)
                    {

                        g_player[p].ps->ang += (l*q);
                        g_player[p].ps->ang &= 2047;

                        g_player[p].ps->posz += zchange;

                        rotatepoint(sprite[j].x,sprite[j].y,g_player[p].ps->posx,g_player[p].ps->posy,(q*l),&m,&x);

                        g_player[p].ps->bobposx += m-g_player[p].ps->posx;
                        g_player[p].ps->bobposy += x-g_player[p].ps->posy;

                        g_player[p].ps->posx = m;
                        g_player[p].ps->posy = x;

                        if (sprite[g_player[p].ps->i].extra <= 0)
                        {
                            sprite[g_player[p].ps->i].x = m;
                            sprite[g_player[p].ps->i].y = x;
                        }
                    }
                }

                p = headspritesect[s->sectnum];
                while (p >= 0)
                {
                    if (sprite[p].statnum != 3 && sprite[p].statnum != 4)
                        if (sprite[p].picnum != LASERLINE)
                        {
                            if (sprite[p].picnum == APLAYER && sprite[p].owner >= 0)
                            {
                                p = nextspritesect[p];
                                continue;
                            }

                            sprite[p].ang += (l*q);
                            sprite[p].ang &= 2047;

                            sprite[p].z += zchange;

                            rotatepoint(sprite[j].x,sprite[j].y,sprite[p].x,sprite[p].y,(q*l),&sprite[p].x,&sprite[p].y);
                        }
                    p = nextspritesect[p];
                }

            }

            ms(i);
        }

        break;
        case 1: //Nothing for now used as the pivot
            if (s->owner == -1) //Init
            {
                s->owner = i;

                j = headspritestat[3];
                while (j >= 0)
                {
                    if (sprite[j].lotag == 19 && sprite[j].hitag == sh)
                    {
                        t[0] = 0;
                        break;
                    }
                    j = nextspritestat[j];
                }
            }

            break;
        case 6:
            k = sc->extra;

            if (t[4] > 0)
            {
                t[4]--;
                if (t[4] >= (k-(k>>3)))
                    s->xvel -= (k>>5);
                if (t[4] > ((k>>1)-1) && t[4] < (k-(k>>3)))
                    s->xvel = 0;
                if (t[4] < (k>>1))
                    s->xvel += (k>>5);
                if (t[4] < ((k>>1)-(k>>3)))
                {
                    t[4] = 0;
                    s->xvel = k;
                }
            }
            else s->xvel = k;

            j = headspritestat[3];
            while (j >= 0)
            {
                if ((sprite[j].lotag == 14) && (sh == sprite[j].hitag) && (hittype[j].temp_data[0] == t[0]))
                {
                    sprite[j].xvel = s->xvel;
                    //                        if( t[4] == 1 )
                    {
                        if (hittype[j].temp_data[5] == 0)
                            hittype[j].temp_data[5] = dist(&sprite[j],s);
                        x = ksgn(dist(&sprite[j],s)-hittype[j].temp_data[5]);
                        if (sprite[j].extra)
                            x = -x;
                        s->xvel += x;
                    }
                    hittype[j].temp_data[4] = t[4];
                }
                j = nextspritestat[j];
            }
            x = 0;


        case 14:
            if (s->owner==-1)
                s->owner = LocateTheLocator((short)t[3],(short)t[0]);

            if (s->owner == -1)
            {
                Bsprintf(tempbuf,"Could not find any locators for SE# 6 and 14 with a hitag of %d.\n",t[3]);
                gameexit(tempbuf);
            }

            j = ldist(&sprite[s->owner],s);

            if (j < 1024L)
            {
                if (st==6)
                    if (sprite[s->owner].hitag&1)
                        t[4]=sc->extra; //Slow it down
                t[3]++;
                s->owner = LocateTheLocator(t[3],t[0]);
                if (s->owner==-1)
                {
                    t[3]=0;
                    s->owner = LocateTheLocator(0,t[0]);
                }
            }

            if (s->xvel)
            {
                x = getangle(sprite[s->owner].x-s->x,sprite[s->owner].y-s->y);
                q = getincangle(s->ang,x)>>3;

                t[2] += q;
                s->ang += q;

                if (s->xvel == sc->extra)
                {
                    if ((sc->floorstat&1) == 0 && (sc->ceilingstat&1) == 0)
                    {
                        if (!issoundplaying(i,hittype[i].lastvx))
                            spritesound(hittype[i].lastvx,i);
                    }
                    else if (ud.monsters_off == 0 && sc->floorpal == 0 && (sc->floorstat&1) && rnd(8))
                    {
                        p = findplayer(s,&x);
                        if (x < 20480)
                        {
                            j = s->ang;
                            s->ang = getangle(s->x-g_player[p].ps->posx,s->y-g_player[p].ps->posy);
                            shoot(i,RPG);
                            s->ang = j;
                        }
                    }
                }

                if (s->xvel <= 64 && (sc->floorstat&1) == 0 && (sc->ceilingstat&1) == 0)
                    stopspritesound(hittype[i].lastvx,i);

                if ((sc->floorz-sc->ceilingz) < (108<<8))
                {
                    if (ud.clipping == 0 && s->xvel >= 192)
                        for (p=connecthead;p>=0;p=connectpoint2[p])
                            if (sprite[g_player[p].ps->i].extra > 0)
                            {
                                k = g_player[p].ps->cursectnum;
                                updatesector(g_player[p].ps->posx,g_player[p].ps->posy,&k);
                                if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && g_player[p].ps->cursectnum != s->sectnum))
                                {
                                    g_player[p].ps->posx = s->x;
                                    g_player[p].ps->posy = s->y;
                                    g_player[p].ps->cursectnum = s->sectnum;

                                    setsprite(g_player[p].ps->i,s->x,s->y,s->z);
                                    quickkill(g_player[p].ps);
                                }
                            }
                }

                m = (s->xvel*sintable[(s->ang+512)&2047])>>14;
                x = (s->xvel*sintable[s->ang&2047])>>14;

                for (p = connecthead;p >= 0;p=connectpoint2[p])
                    if (sector[g_player[p].ps->cursectnum].lotag != 2)
                    {
                        if (g_PlayerSpawnPoints[p].os == s->sectnum)
                        {
                            g_PlayerSpawnPoints[p].ox += m;
                            g_PlayerSpawnPoints[p].oy += x;
                        }

                        if (s->sectnum == sprite[g_player[p].ps->i].sectnum)
                        {
                            rotatepoint(s->x,s->y,g_player[p].ps->posx,g_player[p].ps->posy,q,&g_player[p].ps->posx,&g_player[p].ps->posy);

                            g_player[p].ps->posx += m;
                            g_player[p].ps->posy += x;

                            g_player[p].ps->bobposx += m;
                            g_player[p].ps->bobposy += x;

                            g_player[p].ps->ang += q;

                            if (numplayers > 1)
                            {
                                g_player[p].ps->oposx = g_player[p].ps->posx;
                                g_player[p].ps->oposy = g_player[p].ps->posy;
                            }
                            if (sprite[g_player[p].ps->i].extra <= 0)
                            {
                                sprite[g_player[p].ps->i].x = g_player[p].ps->posx;
                                sprite[g_player[p].ps->i].y = g_player[p].ps->posy;
                            }
                        }
                    }
                j = headspritesect[s->sectnum];
                while (j >= 0)
                {
                    if (sprite[j].statnum != 10 && sector[sprite[j].sectnum].lotag != 2 && sprite[j].picnum != SECTOREFFECTOR && sprite[j].picnum != LOCATORS)
                    {
                        rotatepoint(s->x,s->y,sprite[j].x,sprite[j].y,q,&sprite[j].x,&sprite[j].y);

                        sprite[j].x+= m;
                        sprite[j].y+= x;

                        sprite[j].ang+=q;

                        if (numplayers > 1)
                        {
                            hittype[j].bposx = sprite[j].x;
                            hittype[j].bposy = sprite[j].y;
                        }
                    }
                    j = nextspritesect[j];
                }

                ms(i);
                setsprite(i,s->x,s->y,s->z);

                if ((sc->floorz-sc->ceilingz) < (108<<8))
                {
                    if (ud.clipping == 0 && s->xvel >= 192)
                        for (p=connecthead;p>=0;p=connectpoint2[p])
                            if (sprite[g_player[p].ps->i].extra > 0)
                            {
                                k = g_player[p].ps->cursectnum;
                                updatesector(g_player[p].ps->posx,g_player[p].ps->posy,&k);
                                if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && g_player[p].ps->cursectnum != s->sectnum))
                                {
                                    g_player[p].ps->oposx = g_player[p].ps->posx = s->x;
                                    g_player[p].ps->oposy = g_player[p].ps->posy = s->y;
                                    g_player[p].ps->cursectnum = s->sectnum;

                                    setsprite(g_player[p].ps->i,s->x,s->y,s->z);
                                    quickkill(g_player[p].ps);
                                }
                            }

                    j = headspritesect[sprite[OW].sectnum];
                    while (j >= 0)
                    {
                        l = nextspritesect[j];
                        if (sprite[j].statnum == 1 && badguy(&sprite[j]) && sprite[j].picnum != SECTOREFFECTOR && sprite[j].picnum != LOCATORS)
                        {
                            k = sprite[j].sectnum;
                            updatesector(sprite[j].x,sprite[j].y,&k);
                            if (sprite[j].extra >= 0 && k == s->sectnum)
                            {
                                gutsdir(j,JIBS6,72,myconnectindex);
                                spritesound(SQUISHED,i);
                                deletesprite(j);
                            }
                        }
                        j = l;
                    }
                }
            }

            break;

        case 30:
            if (s->owner == -1)
            {
                t[3] = !t[3];
                s->owner = LocateTheLocator(t[3],t[0]);
            }
            else
            {

                if (t[4] == 1) // Starting to go
                {
                    if (ldist(&sprite[s->owner],s) < (2048-128))
                        t[4] = 2;
                    else
                    {
                        if (s->xvel == 0)
                            operateactivators(s->hitag+(!t[3]),-1);
                        if (s->xvel < 256)
                            s->xvel += 16;
                    }
                }
                if (t[4] == 2)
                {
                    l = FindDistance2D(sprite[s->owner].x-s->x,sprite[s->owner].y-s->y);

                    if (l <= 128)
                        s->xvel = 0;

                    if (s->xvel > 0)
                        s->xvel -= 16;
                    else
                    {
                        s->xvel = 0;
                        operateactivators(s->hitag+(short)t[3],-1);
                        s->owner = -1;
                        s->ang += 1024;
                        t[4] = 0;
                        operateforcefields(i,s->hitag);

                        j = headspritesect[s->sectnum];
                        while (j >= 0)
                        {
                            if (sprite[j].picnum != SECTOREFFECTOR && sprite[j].picnum != LOCATORS)
                            {
                                hittype[j].bposx = sprite[j].x;
                                hittype[j].bposy = sprite[j].y;
                            }
                            j = nextspritesect[j];
                        }

                    }
                }
            }

            if (s->xvel)
            {
                l = (s->xvel*sintable[(s->ang+512)&2047])>>14;
                x = (s->xvel*sintable[s->ang&2047])>>14;

                if ((sc->floorz-sc->ceilingz) < (108<<8))
                    if (ud.clipping == 0)
                        for (p=connecthead;p>=0;p=connectpoint2[p])
                            if (sprite[g_player[p].ps->i].extra > 0)
                            {
                                k = g_player[p].ps->cursectnum;
                                updatesector(g_player[p].ps->posx,g_player[p].ps->posy,&k);
                                if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && g_player[p].ps->cursectnum != s->sectnum))
                                {
                                    g_player[p].ps->posx = s->x;
                                    g_player[p].ps->posy = s->y;
                                    g_player[p].ps->cursectnum = s->sectnum;

                                    setsprite(g_player[p].ps->i,s->x,s->y,s->z);
                                    quickkill(g_player[p].ps);
                                }
                            }

                for (p = connecthead;p >= 0;p = connectpoint2[p])
                {
                    if (sprite[g_player[p].ps->i].sectnum == s->sectnum)
                    {
                        g_player[p].ps->posx += l;
                        g_player[p].ps->posy += x;

                        if (numplayers > 1)
                        {
                            g_player[p].ps->oposx = g_player[p].ps->posx;
                            g_player[p].ps->oposy = g_player[p].ps->posy;
                        }

                        g_player[p].ps->bobposx += l;
                        g_player[p].ps->bobposy += x;
                    }

                    if (g_PlayerSpawnPoints[p].os == s->sectnum)
                    {
                        g_PlayerSpawnPoints[p].ox += l;
                        g_PlayerSpawnPoints[p].oy += x;
                    }
                }

                j = headspritesect[s->sectnum];
                while (j >= 0)
                {
                    if (sprite[j].picnum != SECTOREFFECTOR && sprite[j].picnum != LOCATORS)
                    {
                        if (numplayers < 2)
                        {
                            hittype[j].bposx = sprite[j].x;
                            hittype[j].bposy = sprite[j].y;
                        }

                        sprite[j].x += l;
                        sprite[j].y += x;

                        if (numplayers > 1)
                        {
                            hittype[j].bposx = sprite[j].x;
                            hittype[j].bposy = sprite[j].y;
                        }
                    }
                    j = nextspritesect[j];
                }

                ms(i);
                setsprite(i,s->x,s->y,s->z);

                if ((sc->floorz-sc->ceilingz) < (108<<8))
                {
                    if (ud.clipping == 0)
                        for (p=connecthead;p>=0;p=connectpoint2[p])
                            if (sprite[g_player[p].ps->i].extra > 0)
                            {
                                k = g_player[p].ps->cursectnum;
                                updatesector(g_player[p].ps->posx,g_player[p].ps->posy,&k);
                                if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && g_player[p].ps->cursectnum != s->sectnum))
                                {
                                    g_player[p].ps->posx = s->x;
                                    g_player[p].ps->posy = s->y;

                                    g_player[p].ps->oposx = g_player[p].ps->posx;
                                    g_player[p].ps->oposy = g_player[p].ps->posy;

                                    g_player[p].ps->cursectnum = s->sectnum;

                                    setsprite(g_player[p].ps->i,s->x,s->y,s->z);
                                    quickkill(g_player[p].ps);
                                }
                            }

                    j = headspritesect[sprite[OW].sectnum];
                    while (j >= 0)
                    {
                        l = nextspritesect[j];
                        if (sprite[j].statnum == 1 && badguy(&sprite[j]) && sprite[j].picnum != SECTOREFFECTOR && sprite[j].picnum != LOCATORS)
                        {
                            //                    if(sprite[j].sectnum != s->sectnum)
                            {
                                k = sprite[j].sectnum;
                                updatesector(sprite[j].x,sprite[j].y,&k);
                                if (sprite[j].extra >= 0 && k == s->sectnum)
                                {
                                    gutsdir(j,JIBS6,24,myconnectindex);
                                    spritesound(SQUISHED,j);
                                    deletesprite(j);
                                }
                            }

                        }
                        j = l;
                    }
                }
            }

            break;


        case 2://Quakes
            if (t[4] > 0 && t[0] == 0)
            {
                if (t[4] < sh)
                    t[4]++;
                else t[0] = 1;
            }

            if (t[0] > 0)
            {
                t[0]++;

                s->xvel = 3;

                if (t[0] > 96)
                {
                    t[0] = -1; //Stop the quake
                    t[4] = -1;
                    KILLIT(i);
                }
                else
                {
                    if ((t[0]&31) ==  8)
                    {
                        earthquaketime = 48;
                        spritesound(EARTHQUAKE,g_player[screenpeek].ps->i);
                    }

                    if (klabs(sc->floorheinum-t[5]) < 8)
                        sc->floorheinum = t[5];
                    else sc->floorheinum += (ksgn(t[5]-sc->floorheinum)<<4);
                }

                m = (s->xvel*sintable[(s->ang+512)&2047])>>14;
                x = (s->xvel*sintable[s->ang&2047])>>14;


                for (p=connecthead;p>=0;p=connectpoint2[p])
                    if (g_player[p].ps->cursectnum == s->sectnum && g_player[p].ps->on_ground)
                    {
                        g_player[p].ps->posx += m;
                        g_player[p].ps->posy += x;

                        g_player[p].ps->bobposx += m;
                        g_player[p].ps->bobposy += x;
                    }

                j = headspritesect[s->sectnum];
                while (j >= 0)
                {
                    nextj = nextspritesect[j];

                    if (sprite[j].picnum != SECTOREFFECTOR)
                    {
                        sprite[j].x+=m;
                        sprite[j].y+=x;
                        setsprite(j,sprite[j].x,sprite[j].y,sprite[j].z);
                    }
                    j = nextj;
                }
                ms(i);
                setsprite(i,s->x,s->y,s->z);
            }
            break;

            //Flashing sector lights after reactor EXPLOSION2

        case 3:

            if (t[4] == 0) break;
            p = findplayer(s,&x);

            //    if(t[5] > 0) { t[5]--; break; }

            if ((global_random/(sh+1)&31) < 4 && !t[2])
            {
                //       t[5] = 4+(global_random&7);
                sc->ceilingpal = s->owner>>8;
                sc->floorpal = s->owner&0xff;
                t[0] = s->shade + (global_random&15);
            }
            else
            {
                //       t[5] = 4+(global_random&3);
                sc->ceilingpal = s->pal;
                sc->floorpal = s->pal;
                t[0] = t[3];
            }

            sc->ceilingshade = t[0];
            sc->floorshade = t[0];

            wal = &wall[sc->wallptr];

            for (x=sc->wallnum;x > 0;x--,wal++)
            {
                if (wal->hitag != 1)
                {
                    wal->shade = t[0];
                    if ((wal->cstat&2) && wal->nextwall >= 0)
                    {
                        wall[wal->nextwall].shade = wal->shade;
                    }
                }
            }

            break;

        case 4:

            if ((global_random/(sh+1)&31) < 4)
            {
                t[1] = s->shade + (global_random&15);//Got really bright
                t[0] = s->shade + (global_random&15);
                sc->ceilingpal = s->owner>>8;
                sc->floorpal = s->owner&0xff;
                j = 1;
            }
            else
            {
                t[1] = t[2];
                t[0] = t[3];

                sc->ceilingpal = s->pal;
                sc->floorpal = s->pal;

                j = 0;
            }

            sc->floorshade = t[1];
            sc->ceilingshade = t[1];

            wal = &wall[sc->wallptr];

            for (x=sc->wallnum;x > 0; x--,wal++)
            {
                if (j) wal->pal = (s->owner&0xff);
                else wal->pal = s->pal;

                if (wal->hitag != 1)
                {
                    wal->shade = t[0];
                    if ((wal->cstat&2) && wal->nextwall >= 0)
                        wall[wal->nextwall].shade = wal->shade;
                }
            }

            j = headspritesect[SECT];
            while (j >= 0)
            {
                if (sprite[j].cstat&16 && checkspriteflags(j,SPRITE_FLAG_NOSHADE) == 0)
                {
                    if (sc->ceilingstat&1)
                        sprite[j].shade = sc->ceilingshade;
                    else sprite[j].shade = sc->floorshade;
                }

                j = nextspritesect[j];
            }

            if (t[4]) KILLIT(i);

            break;

            //BOSS
        case 5:
            p = findplayer(s,&x);
            if (x < 8192)
            {
                j = s->ang;
                s->ang = getangle(s->x-g_player[p].ps->posx,s->y-g_player[p].ps->posy);
                shoot(i,FIRELASER);
                s->ang = j;
            }

            if (s->owner==-1) //Start search
            {
                t[4]=0;
                l = 0x7fffffff;
                while (1) //Find the shortest dist
                {
                    s->owner = LocateTheLocator((short)t[4],-1); //t[0] hold sectnum

                    if (s->owner==-1) break;

                    m = ldist(&sprite[g_player[p].ps->i],&sprite[s->owner]);

                    if (l > m)
                    {
                        q = s->owner;
                        l = m;
                    }

                    t[4]++;
                }

                s->owner = q;
                s->zvel = ksgn(sprite[q].z-s->z)<<4;
            }

            if (ldist(&sprite[s->owner],s) < 1024)
            {
                short ta;
                ta = s->ang;
                s->ang = getangle(g_player[p].ps->posx-s->x,g_player[p].ps->posy-s->y);
                s->ang = ta;
                s->owner = -1;
                goto BOLT;

            }
            else s->xvel=256;

            x = getangle(sprite[s->owner].x-s->x,sprite[s->owner].y-s->y);
            q = getincangle(s->ang,x)>>3;
            s->ang += q;

            if (rnd(32))
            {
                t[2]+=q;
                sc->ceilingshade = 127;
            }
            else
            {
                t[2] +=
                    getincangle(t[2]+512,getangle(g_player[p].ps->posx-s->x,g_player[p].ps->posy-s->y))>>2;
                sc->ceilingshade = 0;
            }
            IFHIT
            {
                t[3]++;
                if (t[3] == 5)
                {
                    s->zvel += 1024;
                    FTA(7,g_player[myconnectindex].ps);
                }
            }

            s->z += s->zvel;
            sc->ceilingz += s->zvel;
            sector[t[0]].ceilingz += s->zvel;
            ms(i);
            setsprite(i,s->x,s->y,s->z);
            break;


        case 8:
        case 9:

            // work only if its moving

            j = -1;

            if (hittype[i].temp_data[4])
            {
                hittype[i].temp_data[4]++;
                if (hittype[i].temp_data[4] > 8) KILLIT(i);
                j = 1;
            }
            else j = getanimationgoal(&sc->ceilingz);

            if (j >= 0)
            {
                short sn;

                if ((sc->lotag&0x8000) || hittype[i].temp_data[4])
                    x = -t[3];
                else
                    x = t[3];

                if (st == 9) x = -x;

                j = headspritestat[3];
                while (j >= 0)
                {
                    if (((sprite[j].lotag) == st) && (sprite[j].hitag) == sh)
                    {
                        sn = sprite[j].sectnum;
                        m = sprite[j].shade;

                        wal = &wall[sector[sn].wallptr];

                        for (l=sector[sn].wallnum;l>0;l--,wal++)
                        {
                            if (wal->hitag != 1)
                            {
                                wal->shade+=x;

                                if (wal->shade < m)
                                    wal->shade = m;
                                else if (wal->shade > hittype[j].temp_data[2])
                                    wal->shade = hittype[j].temp_data[2];

                                if (wal->nextwall >= 0)
                                    if (wall[wal->nextwall].hitag != 1)
                                        wall[wal->nextwall].shade = wal->shade;
                            }
                        }

                        sector[sn].floorshade   += x;
                        sector[sn].ceilingshade += x;

                        if (sector[sn].floorshade < m)
                            sector[sn].floorshade = m;
                        else if (sector[sn].floorshade > hittype[j].temp_data[0])
                            sector[sn].floorshade = hittype[j].temp_data[0];

                        if (sector[sn].ceilingshade < m)
                            sector[sn].ceilingshade = m;
                        else if (sector[sn].ceilingshade > hittype[j].temp_data[1])
                            sector[sn].ceilingshade = hittype[j].temp_data[1];

                    }
                    j = nextspritestat[j];
                }
            }
            break;
        case 10:

            if ((sc->lotag&0xff) == 27 || (sc->floorz > sc->ceilingz && (sc->lotag&0xff) != 23) || sc->lotag == (short) 32791)
            {
                j = 1;

                if ((sc->lotag&0xff) != 27)
                    for (p=connecthead;p>=0;p=connectpoint2[p])
                        if (sc->lotag != 30 && sc->lotag != 31 && sc->lotag != 0)
                            if (s->sectnum == sprite[g_player[p].ps->i].sectnum)
                                j = 0;

                if (j == 1)
                {
                    if (t[0] > sh)
                        switch (sector[s->sectnum].lotag)
                        {
                        case 20:
                        case 21:
                        case 22:
                        case 26:
                            if (getanimationgoal(&sector[s->sectnum].ceilingz) >= 0)
                                break;
                        default:
                            activatebysector(s->sectnum,i);
                            t[0] = 0;
                            break;
                        }
                    else t[0]++;
                }
            }
            else t[0]=0;
            break;
        case 11: //Swingdoor

            if (t[5] > 0)
            {
                t[5]--;
                break;
            }

            if (t[4])
            {
                int endwall = sc->wallptr+sc->wallnum;

                for (j=sc->wallptr;j<endwall;j++)
                {
                    k = headspritestat[1];
                    while (k >= 0)
                    {
                        if (sprite[k].extra > 0 && badguy(&sprite[k]) && clipinsidebox(sprite[k].x,sprite[k].y,j,256L) == 1)
                            goto BOLT;
                        k = nextspritestat[k];
                    }

                    k = headspritestat[10];
                    while (k >= 0)
                    {
                        if (sprite[k].owner >= 0 && clipinsidebox(sprite[k].x,sprite[k].y,j,144L) == 1)
                        {
                            t[5] = 8; // Delay
                            k = (SP>>3)*t[3];
                            t[2]-=k;
                            t[4]-=k;
                            ms(i);
                            setsprite(i,s->x,s->y,s->z);
                            goto BOLT;
                        }
                        k = nextspritestat[k];
                    }
                }

                k = (SP>>3)*t[3];
                t[2]+=k;
                t[4]+=k;
                ms(i);
                setsprite(i,s->x,s->y,s->z);

                if (t[4] <= -511 || t[4] >= 512)
                {
                    t[4] = 0;
                    t[2] &= 0xffffff00;
                    ms(i);
                    setsprite(i,s->x,s->y,s->z);
                    break;
                }
            }
            break;
        case 12:
            if (t[0] == 3 || t[3] == 1)   //Lights going off
            {
                sc->floorpal = 0;
                sc->ceilingpal = 0;

                wal = &wall[sc->wallptr];
                for (j = sc->wallnum;j > 0; j--, wal++)
                    if (wal->hitag != 1)
                    {
                        wal->shade = t[1];
                        wal->pal = 0;
                    }

                sc->floorshade = t[1];
                sc->ceilingshade = t[2];
                t[0]=0;

                j = headspritesect[SECT];
                while (j >= 0)
                {
                    if (sprite[j].cstat&16 && checkspriteflags(j,SPRITE_FLAG_NOSHADE) == 0)
                    {
                        if (sc->ceilingstat&1)
                            sprite[j].shade = sc->ceilingshade;
                        else sprite[j].shade = sc->floorshade;
                    }
                    j = nextspritesect[j];

                }

                if (t[3] == 1) KILLIT(i);
            }
            if (t[0] == 1)   //Lights flickering on
            {
                if (sc->floorshade > s->shade)
                {
                    sc->floorpal = s->pal;
                    sc->ceilingpal = s->pal;

                    sc->floorshade -= 2;
                    sc->ceilingshade -= 2;

                    wal = &wall[sc->wallptr];
                    for (j=sc->wallnum;j>0;j--,wal++)
                        if (wal->hitag != 1)
                        {
                            wal->pal = s->pal;
                            wal->shade -= 2;
                        }
                }
                else t[0] = 2;

                j = headspritesect[SECT];
                while (j >= 0)
                {
                    if (sprite[j].cstat&16)
                    {
                        if (sc->ceilingstat&1 && checkspriteflags(j,SPRITE_FLAG_NOSHADE) == 0)
                            sprite[j].shade = sc->ceilingshade;
                        else sprite[j].shade = sc->floorshade;
                    }
                    j = nextspritesect[j];
                }
            }
            break;


        case 13:
            if (t[2])
            {
                j = (SP<<5)|1;

                if (s->ang == 512)
                {
                    if (s->owner)
                    {
                        if (klabs(t[0]-sc->ceilingz) >= j)
                            sc->ceilingz += ksgn(t[0]-sc->ceilingz)*j;
                        else sc->ceilingz = t[0];
                    }
                    else
                    {
                        if (klabs(t[1]-sc->floorz) >= j)
                            sc->floorz += ksgn(t[1]-sc->floorz)*j;
                        else sc->floorz = t[1];
                    }
                }
                else
                {
                    if (klabs(t[1]-sc->floorz) >= j)
                        sc->floorz += ksgn(t[1]-sc->floorz)*j;
                    else sc->floorz = t[1];
                    if (klabs(t[0]-sc->ceilingz) >= j)
                        sc->ceilingz += ksgn(t[0]-sc->ceilingz)*j;
                    sc->ceilingz = t[0];
                }

                if (t[3] == 1)
                {
                    //Change the shades

                    t[3]++;
                    sc->ceilingstat ^= 1;

                    if (s->ang == 512)
                    {
                        wal = &wall[sc->wallptr];
                        for (j=sc->wallnum;j>0;j--,wal++)
                            wal->shade = s->shade;

                        sc->floorshade = s->shade;

                        if (g_player[0].ps->one_parallax_sectnum >= 0)
                        {
                            sc->ceilingpicnum =
                                sector[g_player[0].ps->one_parallax_sectnum].ceilingpicnum;
                            sc->ceilingshade  =
                                sector[g_player[0].ps->one_parallax_sectnum].ceilingshade;
                        }
                    }
                }
                t[2]++;
                if (t[2] > 256)
                    KILLIT(i);
            }


            if (t[2] == 4 && s->ang != 512)
                for (x=0;x<7;x++) RANDOMSCRAP;
            break;


        case 15:

            if (t[4])
            {
                s->xvel = 16;

                if (t[4] == 1) //Opening
                {
                    if (t[3] >= (SP>>3))
                    {
                        t[4] = 0; //Turn off the sliders
                        callsound(s->sectnum,i);
                        break;
                    }
                    t[3]++;
                }
                else if (t[4] == 2)
                {
                    if (t[3]<1)
                    {
                        t[4] = 0;
                        callsound(s->sectnum,i);
                        break;
                    }
                    t[3]--;
                }

                ms(i);
                setsprite(i,s->x,s->y,s->z);
            }
            break;

        case 16: //Reactor

            t[2]+=32;
            if (sc->floorz<sc->ceilingz) s->shade=0;

            else if (sc->ceilingz < t[3])
            {

                //The following code check to see if
                //there is any other sprites in the sector.
                //If there isn't, then kill this sectoreffector
                //itself.....

                j = headspritesect[s->sectnum];
                while (j >= 0)
                {
                    if (sprite[j].picnum == REACTOR || sprite[j].picnum == REACTOR2)
                        break;
                    j = nextspritesect[j];
                }
                if (j == -1)
                {
                    KILLIT(i);
                }
                else s->shade=1;
            }

            if (s->shade) sc->ceilingz+=1024;
            else sc->ceilingz-=512;

            ms(i);
            setsprite(i,s->x,s->y,s->z);

            break;

        case 17:

            q = t[0]*(SP<<2);

            sc->ceilingz += q;
            sc->floorz += q;

            j = headspritesect[s->sectnum];
            while (j >= 0)
            {
                if (sprite[j].statnum == 10 && sprite[j].owner >= 0)
                {
                    p = sprite[j].yvel;
                    if (numplayers < 2)
                        g_player[p].ps->oposz = g_player[p].ps->posz;
                    g_player[p].ps->posz += q;
                    g_player[p].ps->truefz += q;
                    g_player[p].ps->truecz += q;
                    if (numplayers > 1)
                        g_player[p].ps->oposz = g_player[p].ps->posz;
                }
                if (sprite[j].statnum != 3)
                {
                    hittype[j].bposz = sprite[j].z;
                    sprite[j].z += q;
                }

                hittype[j].floorz = sc->floorz;
                hittype[j].ceilingz = sc->ceilingz;

                j = nextspritesect[j];
            }

            if (t[0])                if (t[0])   //If in motion
                {
                    if (klabs(sc->floorz-t[2]) <= SP)
                    {
                        activatewarpelevators(i,0);
                        break;
                    }

                    if (t[0]==-1)
                    {
                        if (sc->floorz > t[3])
                            break;
                    }
                    else if (sc->ceilingz < t[4]) break;

                    if (t[1] == 0) break;
                    t[1] = 0;

                    j = headspritestat[3];
                    while (j >= 0)
                    {
                        if (i != j && (sprite[j].lotag) == 17)
                            if ((sc->hitag-t[0]) ==
                                    (sector[sprite[j].sectnum].hitag)
                                    && sh == (sprite[j].hitag))
                                break;
                        j = nextspritestat[j];
                    }

                    if (j == -1) break;

                    k = headspritesect[s->sectnum];
                    while (k >= 0)
                    {
                        nextk = nextspritesect[k];

                        if (sprite[k].statnum == 10 && sprite[k].owner >= 0)
                        {
                            p = sprite[k].yvel;

                            g_player[p].ps->posx += sprite[j].x-s->x;
                            g_player[p].ps->posy += sprite[j].y-s->y;
                            g_player[p].ps->posz = sector[sprite[j].sectnum].floorz-(sc->floorz-g_player[p].ps->posz);

                            hittype[k].floorz = sector[sprite[j].sectnum].floorz;
                            hittype[k].ceilingz = sector[sprite[j].sectnum].ceilingz;

                            g_player[p].ps->bobposx = g_player[p].ps->oposx = g_player[p].ps->posx;
                            g_player[p].ps->bobposy = g_player[p].ps->oposy = g_player[p].ps->posy;
                            g_player[p].ps->oposz = g_player[p].ps->posz;

                            g_player[p].ps->truefz = hittype[k].floorz;
                            g_player[p].ps->truecz = hittype[k].ceilingz;
                            g_player[p].ps->bobcounter = 0;

                            changespritesect(k,sprite[j].sectnum);
                            g_player[p].ps->cursectnum = sprite[j].sectnum;
                        }
                        else if (sprite[k].statnum != 3)
                        {
                            sprite[k].x +=
                                sprite[j].x-s->x;
                            sprite[k].y +=
                                sprite[j].y-s->y;
                            sprite[k].z = sector[sprite[j].sectnum].floorz-
                                          (sc->floorz-sprite[k].z);

                            hittype[k].bposx = sprite[k].x;
                            hittype[k].bposy = sprite[k].y;
                            hittype[k].bposz = sprite[k].z;

                            changespritesect(k,sprite[j].sectnum);
                            setsprite(k,sprite[k].x,sprite[k].y,sprite[k].z);

                            hittype[k].floorz = sector[sprite[j].sectnum].floorz;
                            hittype[k].ceilingz = sector[sprite[j].sectnum].ceilingz;

                        }
                        k = nextk;
                    }
                }
            break;

        case 18:
            if (t[0])
            {
                if (s->pal)
                {
                    if (s->ang == 512)
                    {
                        sc->ceilingz -= sc->extra;
                        if (sc->ceilingz <= t[1])
                        {
                            sc->ceilingz = t[1];
                            KILLIT(i);
                        }
                    }
                    else
                    {
                        sc->floorz += sc->extra;
                        j = headspritesect[s->sectnum];
                        while (j >= 0)
                        {
                            if (sprite[j].picnum == APLAYER && sprite[j].owner >= 0)
                                if (g_player[sprite[j].yvel].ps->on_ground == 1)
                                    g_player[sprite[j].yvel].ps->posz += sc->extra;
                            if (sprite[j].zvel == 0 && sprite[j].statnum != 3 && sprite[j].statnum != 4)
                            {
                                hittype[j].bposz = sprite[j].z += sc->extra;
                                hittype[j].floorz = sc->floorz;
                            }
                            j = nextspritesect[j];
                        }
                        if (sc->floorz >= t[1])
                        {
                            sc->floorz = t[1];
                            KILLIT(i);
                        }
                    }
                }
                else
                {
                    if (s->ang == 512)
                    {
                        sc->ceilingz += sc->extra;
                        if (sc->ceilingz >= s->z)
                        {
                            sc->ceilingz = s->z;
                            KILLIT(i);
                        }
                    }
                    else
                    {
                        sc->floorz -= sc->extra;
                        j = headspritesect[s->sectnum];
                        while (j >= 0)
                        {
                            if (sprite[j].picnum == APLAYER && sprite[j].owner >= 0)
                                if (g_player[sprite[j].yvel].ps->on_ground == 1)
                                    g_player[sprite[j].yvel].ps->posz -= sc->extra;
                            if (sprite[j].zvel == 0 && sprite[j].statnum != 3 && sprite[j].statnum != 4)
                            {
                                hittype[j].bposz = sprite[j].z -= sc->extra;
                                hittype[j].floorz = sc->floorz;
                            }
                            j = nextspritesect[j];
                        }
                        if (sc->floorz <= s->z)
                        {
                            sc->floorz = s->z;
                            KILLIT(i);
                        }
                    }
                }

                t[2]++;
                if (t[2] >= s->hitag)
                {
                    t[2] = 0;
                    t[0] = 0;
                }
            }
            break;

        case 19: //Battlestar galactia shields

            if (t[0])
            {
                if (t[0] == 1)
                {
                    t[0]++;
                    x = sc->wallptr;
                    q = x+sc->wallnum;
                    for (j=x;j<q;j++)
                        if (wall[j].overpicnum == BIGFORCE)
                        {
                            wall[j].cstat &= (128+32+8+4+2);
                            wall[j].overpicnum = 0;
                            if (wall[j].nextwall >= 0)
                            {
                                wall[wall[j].nextwall].overpicnum = 0;
                                wall[wall[j].nextwall].cstat &= (128+32+8+4+2);
                            }
                        }
                }

                if (sc->ceilingz < sc->floorz)
                    sc->ceilingz += SP;
                else
                {
                    sc->ceilingz = sc->floorz;

                    j = headspritestat[3];
                    while (j >= 0)
                    {
                        if (sprite[j].lotag == 0 && sprite[j].hitag==sh)
                        {
                            q = sprite[sprite[j].owner].sectnum;
                            sector[sprite[j].sectnum].floorpal = sector[sprite[j].sectnum].ceilingpal =
                                                                     sector[q].floorpal;
                            sector[sprite[j].sectnum].floorshade = sector[sprite[j].sectnum].ceilingshade =
                                                                       sector[q].floorshade;

                            hittype[sprite[j].owner].temp_data[0] = 2;
                        }
                        j = nextspritestat[j];
                    }
                    KILLIT(i);
                }
            }
            else //Not hit yet
            {
                IFHITSECT
                {
                    FTA(8,g_player[myconnectindex].ps);

                    l = headspritestat[3];
                    while (l >= 0)
                    {
                        x = sprite[l].lotag&0x7fff;
                        switch (x)
                        {
                        case 0:
                            if (sprite[l].hitag == sh)
                            {
                                q = sprite[l].sectnum;
                                sector[q].floorshade =
                                    sector[q].ceilingshade =
                                        sprite[sprite[l].owner].shade;
                                sector[q].floorpal =
                                    sector[q].ceilingpal =
                                        sprite[sprite[l].owner].pal;
                            }
                            break;

                        case 1:
                        case 12:
                            //                                case 18:
                        case 19:

                            if (sh == sprite[l].hitag)
                                if (hittype[l].temp_data[0] == 0)
                                {
                                    hittype[l].temp_data[0] = 1; //Shut them all on
                                    sprite[l].owner = i;
                                }

                            break;
                        }
                        l = nextspritestat[l];
                    }
                }
            }

            break;

        case 20: //Extend-o-bridge

            if (t[0] == 0) break;
            if (t[0] == 1) s->xvel = 8;
            else s->xvel = -8;

            if (s->xvel)   //Moving
            {
                x = (s->xvel*sintable[(s->ang+512)&2047])>>14;
                l = (s->xvel*sintable[s->ang&2047])>>14;

                t[3] += s->xvel;

                s->x += x;
                s->y += l;

                if (t[3] <= 0 || (t[3]>>6) >= (SP>>6))
                {
                    s->x -= x;
                    s->y -= l;
                    t[0] = 0;
                    callsound(s->sectnum,i);
                    break;
                }

                j = headspritesect[s->sectnum];
                while (j >= 0)
                {
                    nextj = nextspritesect[j];

                    if (sprite[j].statnum != 3 && sprite[j].zvel == 0)
                    {
                        sprite[j].x += x;
                        sprite[j].y += l;
                        setsprite(j,sprite[j].x,sprite[j].y,sprite[j].z);
                        if (sector[sprite[j].sectnum].floorstat&2)
                            if (sprite[j].statnum == 2)
                                makeitfall(j);
                    }
                    j = nextj;
                }

                dragpoint((short)t[1],wall[t[1]].x+x,wall[t[1]].y+l);
                dragpoint((short)t[2],wall[t[2]].x+x,wall[t[2]].y+l);

                for (p=connecthead;p>=0;p=connectpoint2[p])
                    if (g_player[p].ps->cursectnum == s->sectnum && g_player[p].ps->on_ground)
                    {
                        g_player[p].ps->posx += x;
                        g_player[p].ps->posy += l;

                        g_player[p].ps->oposx = g_player[p].ps->posx;
                        g_player[p].ps->oposy = g_player[p].ps->posy;

                        setsprite(g_player[p].ps->i,g_player[p].ps->posx,g_player[p].ps->posy,g_player[p].ps->posz+PHEIGHT);
                    }

                sc->floorxpanning-=x>>3;
                sc->floorypanning-=l>>3;

                sc->ceilingxpanning-=x>>3;
                sc->ceilingypanning-=l>>3;
            }

            break;

        case 21: // Cascading effect

            if (t[0] == 0) break;

            if (s->ang == 1536)
                l = (int) &sc->ceilingz;
            else
                l = (int) &sc->floorz;

            if (t[0] == 1)   //Decide if the s->sectnum should go up or down
            {
                s->zvel = ksgn(s->z-*(int *)l) * (SP<<4);
                t[0]++;
            }

            if (sc->extra == 0)
            {
                *(int *)l += s->zvel;

                if (klabs(*(int *)l-s->z) < 1024)
                {
                    *(int *)l = s->z;
                    KILLIT(i); //All done
                }
            }
            else sc->extra--;
            break;

        case 22:

            if (t[1])
            {
                if (getanimationgoal(&sector[t[0]].ceilingz) >= 0)
                    sc->ceilingz += sc->extra*9;
                else t[1] = 0;
            }
            break;

        case 24:
        case 34:

            if (t[4]) break;

            x = (SP*sintable[(s->ang+512)&2047])>>18;
            l = (SP*sintable[s->ang&2047])>>18;

            k = 0;

            j = headspritesect[s->sectnum];
            while (j >= 0)
            {
                nextj = nextspritesect[j];
                if (sprite[j].zvel >= 0)
                    switch (sprite[j].statnum)
                    {
                    case 5:
                        switch (dynamictostatic[sprite[j].picnum])
                        {
                        case BLOODPOOL__STATIC:
                        case PUKE__STATIC:
                        case FOOTPRINTS__STATIC:
                        case FOOTPRINTS2__STATIC:
                        case FOOTPRINTS3__STATIC:
                        case FOOTPRINTS4__STATIC:
                        case BULLETHOLE__STATIC:
                        case BLOODSPLAT1__STATIC:
                        case BLOODSPLAT2__STATIC:
                        case BLOODSPLAT3__STATIC:
                        case BLOODSPLAT4__STATIC:
                            sprite[j].xrepeat = sprite[j].yrepeat = 0;
                            j = nextj;
                            continue;
                        case LASERLINE__STATIC:
                            j = nextj;
                            continue;
                        }
                    case 6:
                        if (sprite[j].picnum == TRIPBOMB) break;
                    case 1:
                    case 0:
                        if (
                            sprite[j].picnum == BOLT1 ||
                            sprite[j].picnum == BOLT1+1 ||
                            sprite[j].picnum == BOLT1+2 ||
                            sprite[j].picnum == BOLT1+3 ||
                            sprite[j].picnum == SIDEBOLT1 ||
                            sprite[j].picnum == SIDEBOLT1+1 ||
                            sprite[j].picnum == SIDEBOLT1+2 ||
                            sprite[j].picnum == SIDEBOLT1+3 ||
                            wallswitchcheck(j)
                        )
                            break;

                        if (!(sprite[j].picnum >= CRANE && sprite[j].picnum <= (CRANE+3)))
                        {
                            if (sprite[j].z > (hittype[j].floorz-(16<<8)))
                            {
                                hittype[j].bposx = sprite[j].x;
                                hittype[j].bposy = sprite[j].y;

                                sprite[j].x += x>>2;
                                sprite[j].y += l>>2;

                                setsprite(j,sprite[j].x,sprite[j].y,sprite[j].z);

                                if (sector[sprite[j].sectnum].floorstat&2)
                                    if (sprite[j].statnum == 2)
                                        makeitfall(j);
                            }
                        }
                        break;
                    }
                j = nextj;
            }

            p = myconnectindex;
            if (g_player[p].ps->cursectnum == s->sectnum && g_player[p].ps->on_ground)
                if (klabs(g_player[p].ps->posz-g_player[p].ps->truefz) < PHEIGHT+(9<<8))
                {
                    fricxv += x<<3;
                    fricyv += l<<3;
                }

            sc->floorxpanning += SP>>7;

            break;

        case 35:
            if (sc->ceilingz > s->z)
                for (j = 0;j < 8;j++)
                {
                    s->ang += TRAND&511;
                    k = spawn(i,SMALLSMOKE);
                    sprite[k].xvel = 96+(TRAND&127);
                    ssp(k,CLIPMASK0);
                    setsprite(k,sprite[k].x,sprite[k].y,sprite[k].z);
                    if (rnd(16))
                        spawn(i,EXPLOSION2);
                }

            switch (t[0])
            {
            case 0:
                sc->ceilingz += s->yvel;
                if (sc->ceilingz > sc->floorz)
                    sc->floorz = sc->ceilingz;
                if (sc->ceilingz > s->z+(32<<8))
                    t[0]++;
                break;
            case 1:
                sc->ceilingz-=(s->yvel<<2);
                if (sc->ceilingz < t[4])
                {
                    sc->ceilingz = t[4];
                    t[0] = 0;
                }
                break;
            }
            break;

        case 25: //PISTONS

            if (t[4] == 0) break;

            if (sc->floorz <= sc->ceilingz)
                s->shade = 0;
            else if (sc->ceilingz <= t[3])
                s->shade = 1;

            if (s->shade)
            {
                sc->ceilingz += SP<<4;
                if (sc->ceilingz > sc->floorz)
                    sc->ceilingz = sc->floorz;
            }
            else
            {
                sc->ceilingz   -= SP<<4;
                if (sc->ceilingz < t[3])
                    sc->ceilingz = t[3];
            }

            break;

        case 26:

            s->xvel = 32;
            l = (s->xvel*sintable[(s->ang+512)&2047])>>14;
            x = (s->xvel*sintable[s->ang&2047])>>14;

            s->shade++;
            if (s->shade > 7)
            {
                s->x = t[3];
                s->y = t[4];
                sc->floorz -= ((s->zvel*s->shade)-s->zvel);
                s->shade = 0;
            }
            else
                sc->floorz += s->zvel;

            j = headspritesect[s->sectnum];
            while (j >= 0)
            {
                nextj = nextspritesect[j];
                if (sprite[j].statnum != 3 && sprite[j].statnum != 10)
                {
                    hittype[j].bposx = sprite[j].x;
                    hittype[j].bposy = sprite[j].y;

                    sprite[j].x += l;
                    sprite[j].y += x;

                    sprite[j].z += s->zvel;
                    setsprite(j,sprite[j].x,sprite[j].y,sprite[j].z);
                }
                j = nextj;
            }

            p = myconnectindex;
            if (sprite[g_player[p].ps->i].sectnum == s->sectnum && g_player[p].ps->on_ground)
            {
                fricxv += l<<5;
                fricyv += x<<5;
            }

            for (p = connecthead;p >= 0;p = connectpoint2[p])
                if (sprite[g_player[p].ps->i].sectnum == s->sectnum && g_player[p].ps->on_ground)
                    g_player[p].ps->posz += s->zvel;

            ms(i);
            setsprite(i,s->x,s->y,s->z);

            break;


        case 27:

            if (ud.recstat == 0 || !ud.democams) break;

            hittype[i].tempang = s->ang;

            p = findplayer(s,&x);
            if (sprite[g_player[p].ps->i].extra > 0 && myconnectindex == screenpeek)
            {
                if (t[0] < 0)
                {
                    ud.camerasprite = i;
                    t[0]++;
                }
                else if (ud.recstat == 2 && g_player[p].ps->newowner == -1)
                {
                    if (cansee(s->x,s->y,s->z,SECT,g_player[p].ps->posx,g_player[p].ps->posy,g_player[p].ps->posz,g_player[p].ps->cursectnum))
                    {
                        if (x < (int)((unsigned)sh))
                        {
                            ud.camerasprite = i;
                            t[0] = 999;
                            s->ang += getincangle(s->ang,getangle(g_player[p].ps->posx-s->x,g_player[p].ps->posy-s->y))>>3;
                            SP = 100+((s->z-g_player[p].ps->posz)/257);

                        }
                        else if (t[0] == 999)
                        {
                            if (ud.camerasprite == i)
                                t[0] = 0;
                            else t[0] = -10;
                            ud.camerasprite = i;

                        }
                    }
                    else
                    {
                        s->ang = getangle(g_player[p].ps->posx-s->x,g_player[p].ps->posy-s->y);

                        if (t[0] == 999)
                        {
                            if (ud.camerasprite == i)
                                t[0] = 0;
                            else t[0] = -20;
                            ud.camerasprite = i;
                        }
                    }
                }
            }
            break;
        case 28:
            if (t[5] > 0)
            {
                t[5]--;
                break;
            }

            if (T1 == 0)
            {
                p = findplayer(s,&x);
                if (x > 15500)
                    break;
                T1 = 1;
                T2 = 64 + (TRAND&511);
                T3 = 0;
            }
            else
            {
                T3++;
                if (T3 > T2)
                {
                    T1 = 0;
                    g_player[screenpeek].ps->visibility = ud.const_visibility;
                    break;
                }
                else if (T3 == (T2>>1))
                    spritesound(THUNDER,i);
                else if (T3 == (T2>>3))
                    spritesound(LIGHTNING_SLAP,i);
                else if (T3 == (T2>>2))
                {
                    j = headspritestat[0];
                    while (j >= 0)
                    {
                        if (sprite[j].picnum == NATURALLIGHTNING && sprite[j].hitag == s->hitag)
                            sprite[j].cstat |= 32768;
                        j = nextspritestat[j];
                    }
                }
                else if (T3 > (T2>>3) && T3 < (T2>>2))
                {
                    if (cansee(s->x,s->y,s->z,s->sectnum,g_player[screenpeek].ps->posx,g_player[screenpeek].ps->posy,g_player[screenpeek].ps->posz,g_player[screenpeek].ps->cursectnum))
                        j = 1;
                    else j = 0;

                    if (rnd(192) && (T3&1))
                    {
                        if (j)
                            g_player[screenpeek].ps->visibility = 0;
                    }
                    else if (j)
                        g_player[screenpeek].ps->visibility = ud.const_visibility;

                    j = headspritestat[0];
                    while (j >= 0)
                    {
                        if (sprite[j].picnum == NATURALLIGHTNING && sprite[j].hitag == s->hitag)
                        {
                            if (rnd(32) && (T3&1))
                            {
                                sprite[j].cstat &= 32767;
                                spawn(j,SMALLSMOKE);

                                p = findplayer(s,&x);
                                x = ldist(&sprite[g_player[p].ps->i], &sprite[j]);
                                if (x < 768)
                                {
                                    if (!isspritemakingsound(g_player[p].ps->i,DUKE_LONGTERM_PAIN))
                                        spritesound(DUKE_LONGTERM_PAIN,g_player[p].ps->i);
                                    spritesound(SHORT_CIRCUIT,g_player[p].ps->i);
                                    sprite[g_player[p].ps->i].extra -= 8+(TRAND&7);
                                    g_player[p].ps->pals_time = 32;
                                    g_player[p].ps->pals[0] = 16;
                                    g_player[p].ps->pals[1] = 0;
                                    g_player[p].ps->pals[2] = 0;
                                }
                                break;
                            }
                            else sprite[j].cstat |= 32768;
                        }

                        j = nextspritestat[j];
                    }
                }
            }
            break;
        case 29:
            s->hitag += 64;
            l = mulscale12((int)s->yvel,sintable[s->hitag&2047]);
            sc->floorz = s->z + l;
            break;
        case 31: // True Drop Floor
            if (t[0] == 1)
            {
                // Choose dir

                if (t[3] > 0)
                {
                    t[3]--;
                    break;
                }

                if (t[2] == 1) // Retract
                {
                    if (SA != 1536)
                    {
                        if (klabs(sc->floorz - s->z) < SP)
                        {
                            sc->floorz = s->z;
                            t[2] = 0;
                            t[0] = 0;
                            t[3] = s->hitag;
                            callsound(s->sectnum,i);
                        }
                        else
                        {
                            l = ksgn(s->z-sc->floorz)*SP;
                            sc->floorz += l;

                            j = headspritesect[s->sectnum];
                            while (j >= 0)
                            {
                                if (sprite[j].picnum == APLAYER && sprite[j].owner >= 0)
                                    if (g_player[sprite[j].yvel].ps->on_ground == 1)
                                        g_player[sprite[j].yvel].ps->posz += l;
                                if (sprite[j].zvel == 0 && sprite[j].statnum != 3 && sprite[j].statnum != 4)
                                {
                                    hittype[j].bposz = sprite[j].z += l;
                                    hittype[j].floorz = sc->floorz;
                                }
                                j = nextspritesect[j];
                            }
                        }
                    }
                    else
                    {
                        if (klabs(sc->floorz - t[1]) < SP)
                        {
                            sc->floorz = t[1];
                            callsound(s->sectnum,i);
                            t[2] = 0;
                            t[0] = 0;
                            t[3] = s->hitag;
                        }
                        else
                        {
                            l = ksgn(t[1]-sc->floorz)*SP;
                            sc->floorz += l;

                            j = headspritesect[s->sectnum];
                            while (j >= 0)
                            {
                                if (sprite[j].picnum == APLAYER && sprite[j].owner >= 0)
                                    if (g_player[sprite[j].yvel].ps->on_ground == 1)
                                        g_player[sprite[j].yvel].ps->posz += l;
                                if (sprite[j].zvel == 0 && sprite[j].statnum != 3 && sprite[j].statnum != 4)
                                {
                                    hittype[j].bposz = sprite[j].z += l;
                                    hittype[j].floorz = sc->floorz;
                                }
                                j = nextspritesect[j];
                            }
                        }
                    }
                    break;
                }

                if ((s->ang&2047) == 1536)
                {
                    if (klabs(s->z-sc->floorz) < SP)
                    {
                        callsound(s->sectnum,i);
                        t[0] = 0;
                        t[2] = 1;
                        t[3] = s->hitag;
                    }
                    else
                    {
                        l = ksgn(s->z-sc->floorz)*SP;
                        sc->floorz += l;

                        j = headspritesect[s->sectnum];
                        while (j >= 0)
                        {
                            if (sprite[j].picnum == APLAYER && sprite[j].owner >= 0)
                                if (g_player[sprite[j].yvel].ps->on_ground == 1)
                                    g_player[sprite[j].yvel].ps->posz += l;
                            if (sprite[j].zvel == 0 && sprite[j].statnum != 3 && sprite[j].statnum != 4)
                            {
                                hittype[j].bposz = sprite[j].z += l;
                                hittype[j].floorz = sc->floorz;
                            }
                            j = nextspritesect[j];
                        }
                    }
                }
                else
                {
                    if (klabs(sc->floorz-t[1]) < SP)
                    {
                        t[0] = 0;
                        callsound(s->sectnum,i);
                        t[2] = 1;
                        t[3] = s->hitag;
                    }
                    else
                    {
                        l = ksgn(s->z-t[1])*SP;
                        sc->floorz -= l;

                        j = headspritesect[s->sectnum];
                        while (j >= 0)
                        {
                            if (sprite[j].picnum == APLAYER && sprite[j].owner >= 0)
                                if (g_player[sprite[j].yvel].ps->on_ground == 1)
                                    g_player[sprite[j].yvel].ps->posz -= l;
                            if (sprite[j].zvel == 0 && sprite[j].statnum != 3 && sprite[j].statnum != 4)
                            {
                                hittype[j].bposz = sprite[j].z -= l;
                                hittype[j].floorz = sc->floorz;
                            }
                            j = nextspritesect[j];
                        }
                    }
                }
            }
            break;

        case 32: // True Drop Ceiling
            if (t[0] == 1)
            {
                // Choose dir

                if (t[2] == 1) // Retract
                {
                    if (SA != 1536)
                    {
                        if (klabs(sc->ceilingz - s->z) <
                                (SP<<1))
                        {
                            sc->ceilingz = s->z;
                            callsound(s->sectnum,i);
                            t[2] = 0;
                            t[0] = 0;
                        }
                        else sc->ceilingz +=
                                ksgn(s->z-sc->ceilingz)*SP;
                    }
                    else
                    {
                        if (klabs(sc->ceilingz - t[1]) <
                                (SP<<1))
                        {
                            sc->ceilingz = t[1];
                            callsound(s->sectnum,i);
                            t[2] = 0;
                            t[0] = 0;
                        }
                        else sc->ceilingz +=
                                ksgn(t[1]-sc->ceilingz)*SP;
                    }
                    break;
                }

                if ((s->ang&2047) == 1536)
                {
                    if (klabs(sc->ceilingz-s->z) <
                            (SP<<1))
                    {
                        t[0] = 0;
                        t[2] = !t[2];
                        callsound(s->sectnum,i);
                        sc->ceilingz = s->z;
                    }
                    else sc->ceilingz +=
                            ksgn(s->z-sc->ceilingz)*SP;
                }
                else
                {
                    if (klabs(sc->ceilingz-t[1]) < (SP<<1))
                    {
                        t[0] = 0;
                        t[2] = !t[2];
                        callsound(s->sectnum,i);
                    }
                    else sc->ceilingz -= ksgn(s->z-t[1])*SP;
                }
            }
            break;

        case 33:
            if (earthquaketime > 0 && (TRAND&7) == 0)
                RANDOMSCRAP;
            break;
        case 36:

            if (t[0])
            {
                if (t[0] == 1)
                    shoot(i,sc->extra);
                else if (t[0] == 26*5)
                    t[0] = 0;
                t[0]++;
            }
            break;

        case 128: //SE to control glass breakage

            wal = &wall[t[2]];

            if (wal->cstat|32)
            {
                wal->cstat &= (255-32);
                wal->cstat |= 16;
                if (wal->nextwall >= 0)
                {
                    wall[wal->nextwall].cstat &= (255-32);
                    wall[wal->nextwall].cstat |= 16;
                }
            }
            else break;

            wal->overpicnum++;
            if (wal->nextwall >= 0)
                wall[wal->nextwall].overpicnum++;

            if (t[0] < t[1]) t[0]++;
            else
            {
                wal->cstat &= (128+32+8+4+2);
                if (wal->nextwall >= 0)
                    wall[wal->nextwall].cstat &= (128+32+8+4+2);
                KILLIT(i);
            }
            break;

        case 130:
            if (t[0] > 80)
            {
                KILLIT(i);
            }
            else t[0]++;

            x = sc->floorz-sc->ceilingz;

            if (rnd(64))
            {
                k = spawn(i,EXPLOSION2);
                sprite[k].xrepeat = sprite[k].yrepeat = 2+(TRAND&7);
                sprite[k].z = sc->floorz-(TRAND%x);
                sprite[k].ang += 256-(TRAND%511);
                sprite[k].xvel = TRAND&127;
                ssp(k,CLIPMASK0);
            }
            break;
        case 131:
            if (t[0] > 40)
            {
                KILLIT(i);
            }
            else t[0]++;

            x = sc->floorz-sc->ceilingz;

            if (rnd(32))
            {
                k = spawn(i,EXPLOSION2);
                sprite[k].xrepeat = sprite[k].yrepeat = 2+(TRAND&3);
                sprite[k].z = sc->floorz-(TRAND%x);
                sprite[k].ang += 256-(TRAND%511);
                sprite[k].xvel = TRAND&127;
                ssp(k,CLIPMASK0);
            }
            break;
        }
BOLT:
        i = nexti;
    }

    //Sloped sin-wave floors!
    for (i=headspritestat[3];i>=0;i=nextspritestat[i])
    {
        s = &sprite[i];
        if (s->lotag != 29) continue;
        sc = &sector[s->sectnum];
        if (sc->wallnum != 4) continue;
        wal = &wall[sc->wallptr+2];
        alignflorslope(s->sectnum,wal->x,wal->y,sector[wal->nextsector].floorz);
    }
}

void moveobjects(void)
{
    int k = 0;

    movefta();              //ST 2
    moveweapons();          //ST 4
    movetransports();       //ST 9

    moveplayers();          //ST 10
    movefallers();          //ST 12
    moveexplosions();       //ST 5

    moveactors();           //ST 1
    moveeffectors();        //ST 3

    movestandables();       //ST 6

    for (;k<MAXSTATUS;k++)
    {
        int i = headspritestat[k];
        while (i >= 0)
        {
            int p, j = nextspritestat[i];
            OnEvent(EVENT_GAME,i, findplayer(&sprite[i],(int *)&p), p);
            i = j;
        }
    }

    doanimations();
    movefx();               //ST 11
}

