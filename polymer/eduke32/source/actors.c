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
#include "actors.h"
#include "gamedef.h"
#include "gameexec.h"

#if KRANDDEBUG
# define ACTOR_INLINE
# define ACTOR_STATIC
#else
# define ACTOR_INLINE inline
# define ACTOR_STATIC static
#endif


#define KILLIT(KX) {deletesprite(KX);goto BOLT;}

extern int32_t g_numEnvSoundsPlaying;
extern int32_t g_noEnemies;
extern int32_t ticrandomseed;

inline void G_UpdateInterpolations(void)  //Stick at beginning of G_DoMoveThings
{
    int32_t i=g_numInterpolations-1;
    for (; i>=0; i--) oldipos[i] = *curipos[i];
}

void G_SetInterpolation(int32_t *posptr)
{
    int32_t i=g_numInterpolations-1;

    if (g_numInterpolations >= MAXINTERPOLATIONS) return;
    for (; i>=0; i--)
        if (curipos[i] == posptr) return;
    curipos[g_numInterpolations] = posptr;
    oldipos[g_numInterpolations] = *posptr;
    g_numInterpolations++;
}

void G_StopInterpolation(int32_t *posptr)
{
    int32_t i=g_numInterpolations-1;

    for (; i>=startofdynamicinterpolations; i--)
        if (curipos[i] == posptr)
        {
            g_numInterpolations--;
            oldipos[i] = oldipos[g_numInterpolations];
            bakipos[i] = bakipos[g_numInterpolations];
            curipos[i] = curipos[g_numInterpolations];
        }
}

void G_DoInterpolations(int32_t smoothratio)       //Stick at beginning of drawscreen
{
    int32_t i=g_numInterpolations-1, j = 0, odelta, ndelta = 0;

    if (g_interpolationLock++)
    {
        return;
    }

    for (; i>=0; i--)
    {
        bakipos[i] = *curipos[i];
        odelta = ndelta;
        ndelta = (*curipos[i])-oldipos[i];
        if (odelta != ndelta) j = mulscale16(ndelta,smoothratio);
        *curipos[i] = oldipos[i]+j;
    }
}

inline void G_RestoreInterpolations(void)  //Stick at end of drawscreen
{
    int32_t i=g_numInterpolations-1;

    if (--g_interpolationLock)
        return;

    for (; i>=0; i--) *curipos[i] = bakipos[i];
}

inline int32_t G_CheckForSpaceCeiling(int32_t sectnum)
{
    return ((sector[sectnum].ceilingstat&1) && sector[sectnum].ceilingpal == 0 && (sector[sectnum].ceilingpicnum==MOONSKY1 || sector[sectnum].ceilingpicnum==BIGORBIT1)?1:0);
}

inline int32_t G_CheckForSpaceFloor(int32_t sectnum)
{
    return ((sector[sectnum].floorstat&1) && sector[sectnum].ceilingpal == 0 && ((sector[sectnum].floorpicnum==MOONSKY1)||(sector[sectnum].floorpicnum==BIGORBIT1))?1:0);
}

void A_RadiusDamage(int32_t i, int32_t  r, int32_t  hp1, int32_t  hp2, int32_t  hp3, int32_t  hp4)
{
    spritetype *s=&sprite[i],*sj;
    walltype *wal;
    int32_t d, q, x1, y1;
    int32_t sectcnt, sectend, dasect, startwall, endwall, nextsect;
    int32_t j,k,p,x,nextj;
    int16_t sect=-1;
    char statlist[] = {STAT_DEFAULT,STAT_ACTOR,STAT_STANDABLE,
                       STAT_PLAYER,STAT_FALLER,STAT_ZOMBIEACTOR,STAT_MISC
                      };
    int16_t *tempshort = (int16_t *)tempbuf;

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
                    Sect_DamageCeiling(dasect);
                else
                {
                    d = klabs(wall[wall[wall[sector[dasect].wallptr].point2].point2].x-s->x)+klabs(wall[wall[wall[sector[dasect].wallptr].point2].point2].y-s->y);
                    if (d < r)
                        Sect_DamageCeiling(dasect);
                }
            }

            startwall = sector[dasect].wallptr;
            endwall = startwall+sector[dasect].wallnum;
            for (x=startwall,wal=&wall[startwall]; x<endwall; x++,wal++)
                if ((klabs(wal->x-s->x)+klabs(wal->y-s->y)) < r)
                {
                    nextsect = wal->nextsector;
                    if (nextsect >= 0)
                    {
                        for (dasect=sectend-1; dasect>=0; dasect--)
                            if (tempshort[dasect] == nextsect) break;
                        if (dasect < 0) tempshort[sectend++] = nextsect;
                    }
                    x1 = (((wal->x+wall[wal->point2].x)>>1)+s->x)>>1;
                    y1 = (((wal->y+wall[wal->point2].y)>>1)+s->y)>>1;
                    updatesector(x1,y1,&sect);
                    if (sect >= 0 && cansee(x1,y1,s->z,sect,s->x,s->y,s->z,s->sectnum))
                    {
                        vec3_t tmpvect;

                        Bmemcpy(&tmpvect, wal, sizeof(int32_t) * 2);
                        tmpvect.z = s->z;

                        A_DamageWall(i,x,&tmpvect,s->picnum);
                    }
                }
        }
        while (sectcnt < sectend);
    }

SKIPWALLCHECK:

    q = -(16<<8)+(krand()&((32<<8)-1));

    for (x = 0; x<7; x++)
    {
        j = headspritestat[(uint8_t)statlist[x]];
        while (j >= 0)
        {
            nextj = nextspritestat[j];
            sj = &sprite[j];

            if (x == 0 || x >= 5 || AFLAMABLE(sj->picnum))
            {
                if (s->picnum != SHRINKSPARK || (sj->cstat&257))
                    if (dist(s, sj) < r)
                    {
                        if (A_CheckEnemySprite(sj) && !cansee(sj->x, sj->y,sj->z+q, sj->sectnum, s->x, s->y, s->z+q, s->sectnum))
                            goto BOLT;
                        A_DamageObject(j, i);
                    }
            }
            else if (sj->extra >= 0 && sj != s && (sj->picnum == TRIPBOMB || A_CheckEnemySprite(sj) || sj->picnum == QUEBALL || sj->picnum == STRIPEBALL || (sj->cstat&257) || sj->picnum == DUKELYINGDEAD))
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
                    actor[j].ang = getangle(sj->x-s->x,sj->y-s->y);

                    if (s->picnum == RPG && sj->extra > 0)
                        actor[j].picnum = RPG;
                    else if (A_CheckSpriteFlags(i,SPRITE_PROJECTILE) && SpriteProjectile[i].workslike & PROJECTILE_RADIUS_PICNUM && sj->extra > 0)
                        actor[j].picnum = s->picnum;
                    else
                    {
                        if (s->picnum == SHRINKSPARK)
                            actor[j].picnum = SHRINKSPARK;
                        else actor[j].picnum = RADIUSEXPLOSION;
                    }

                    if (s->picnum != SHRINKSPARK)
                    {
                        k = (r/3);
                        if (d < k)
                        {
                            if (hp4 == hp3) hp4++;
                            actor[j].extra = hp3 + (krand()%(hp4-hp3));
                        }
                        else if (d < (k*2))
                        {
                            if (hp3 == hp2) hp3++;
                            actor[j].extra = hp2 + (krand()%(hp3-hp2));
                        }
                        else if (d < r)
                        {
                            if (hp2 == hp1) hp2++;
                            actor[j].extra = hp1 + (krand()%(hp2-hp1));
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
                            A_DamageObject(j, i);
                    }
                    else if (s->extra == 0) actor[j].extra = 0;

                    if (sj->picnum != RADIUSEXPLOSION &&
                            s->owner >= 0 && sprite[s->owner].statnum < MAXSTATUS)
                    {
                        if (sj->picnum == APLAYER)
                        {
                            p = sj->yvel;
                            if (g_player[p].ps->newowner >= 0)
                            {
                                g_player[p].ps->newowner = -1;
                                g_player[p].ps->pos.x = g_player[p].ps->opos.x;
                                g_player[p].ps->pos.y = g_player[p].ps->opos.y;
                                g_player[p].ps->pos.z = g_player[p].ps->opos.z;
                                g_player[p].ps->ang = g_player[p].ps->oang;
                                updatesector(g_player[p].ps->pos.x,g_player[p].ps->pos.y,&g_player[p].ps->cursectnum);
                                P_UpdateScreenPal(g_player[p].ps);

                                k = headspritestat[STAT_ACTOR];
                                while (k >= 0)
                                {
                                    if (sprite[k].picnum==CAMERA1)
                                        sprite[k].yvel = 0;
                                    k = nextspritestat[k];
                                }
                            }
                        }
                        actor[j].owner = s->owner;
                    }
                }
            }
BOLT:
            j = nextj;
        }
    }
}

int32_t A_MoveSprite(int32_t spritenum, const vec3_t *change, uint32_t cliptype)
{
    spritetype *spr = &sprite[spritenum];
    int32_t retval, daz;
    int16_t dasectnum, cd;
    int32_t bg = A_CheckEnemySprite(spr);
    int32_t oldx = spr->x, oldy = spr->y;
    /*int32_t osectnum = spr->sectnum;*/


    if (spr->statnum == STAT_MISC || (bg && spr->xrepeat < 4))
    {
        spr->x += (change->x*TICSPERFRAME)>>2;
        spr->y += (change->y*TICSPERFRAME)>>2;
        spr->z += (change->z*TICSPERFRAME)>>2;
        if (bg)
            setsprite(spritenum,(vec3_t *)spr);
        return 0;
    }

    dasectnum = spr->sectnum;

    daz = spr->z - ((tilesizy[spr->picnum]*spr->yrepeat)<<1);

    if (bg)
    {
        if (spr->xrepeat > 60)
        {
            int32_t oz = spr->z;
            spr->z = daz;
            retval = clipmove((vec3_t *)spr,&dasectnum,((change->x*TICSPERFRAME)<<11),((change->y*TICSPERFRAME)<<11),1024L,(4<<8),(4<<8),cliptype);
            daz = spr->z;
            spr->z = oz;
        }
        else
        {
            int32_t oz = spr->z;

            if (spr->picnum == LIZMAN)
                cd = 292L;
            else if ((ActorType[spr->picnum]&3))
                cd = spr->clipdist<<2;
            else
                cd = 192L;

            spr->z = daz;
            retval = clipmove((vec3_t *)spr,&dasectnum,((change->x*TICSPERFRAME)<<11),((change->y*TICSPERFRAME)<<11),cd,(4<<8),(4<<8),cliptype);
            daz = spr->z;
            spr->z = oz;
        }

        if (dasectnum < 0 || (dasectnum >= 0 &&
                              ((actor[spritenum].actorstayput >= 0 && actor[spritenum].actorstayput != dasectnum) ||
                               ((spr->picnum == BOSS2) && spr->pal == 0 && sector[dasectnum].lotag != 3) ||
                               ((spr->picnum == BOSS1 || spr->picnum == BOSS2) && sector[dasectnum].lotag == 1) /*||
                               (sector[dasectnum].lotag == 1 && (spr->picnum == LIZMAN || (spr->picnum == LIZTROOP && spr->zvel == 0)))*/
                              ))
           )
        {
            spr->x = oldx;
            spr->y = oldy;
            /*
            if (dasectnum >= 0 && sector[dasectnum].lotag == 1 && spr->picnum == LIZMAN)
            spr->ang = (krand()&2047);
            else if ((Actor[spritenum].t_data[0]&3) == 1 && spr->picnum != COMMANDER)
            spr->ang = (krand()&2047);
            */
            setsprite(spritenum,(vec3_t *)spr);
            if (dasectnum < 0) dasectnum = 0;
            return (16384+dasectnum);
        }
        if ((retval&49152) >= 32768 && (actor[spritenum].cgg==0)) spr->ang += 768;
    }
    else
    {
        int32_t oz = spr->z;
        spr->z = daz;

        if (spr->statnum == STAT_PROJECTILE && (SpriteProjectile[spritenum].workslike & PROJECTILE_REALCLIPDIST) == 0)
            retval =
                clipmove((vec3_t *)&sprite[spritenum],&dasectnum,((change->x*TICSPERFRAME)<<11),((change->y*TICSPERFRAME)<<11),8L,(4<<8),(4<<8),cliptype);
        else
            retval =
                clipmove((vec3_t *)&sprite[spritenum],&dasectnum,((change->x*TICSPERFRAME)<<11),((change->y*TICSPERFRAME)<<11),(int32_t)(spr->clipdist<<2),(4<<8),(4<<8),cliptype);
        daz = spr->z;
        spr->z = oz;
    }

    if (dasectnum == -1)
    {
        dasectnum = spr->sectnum;
        /*OSD_Printf("%s:%d wtf\n",__FILE__,__LINE__);*/
    }

    if ((dasectnum != spr->sectnum))
    {
        changespritesect(spritenum,dasectnum);
        A_GetZLimits(spritenum);
    }

    daz = spr->z + ((change->z*TICSPERFRAME)>>3);

    bg = (tilesizy[spr->picnum]*spr->yrepeat)>>1;
    if ((daz > actor[spritenum].ceilingz) && (daz <= actor[spritenum].floorz)/*
         &&
                (osectnum == dasectnum || cansee(oldx, oldy, spr->z - bg, osectnum, spr->x, spr->y, daz - bg, dasectnum))*/
       )
        spr->z = daz;
    else if (retval == 0) retval = 16384+dasectnum;

    if (retval == (16384+dasectnum))
        if (spr->statnum == STAT_PROJECTILE)
        {
            int32_t i, nexti;

            TRAVERSE_SPRITE_STAT(headspritestat[STAT_TRANSPORT], i, nexti)
            if (sprite[i].sectnum == dasectnum)
            {
                switch (sector[dasectnum].lotag)
                {
                case 1:
                    if (daz >= actor[spritenum].floorz)
                    {
                        if (totalclock > actor[spritenum].lasttransport)
                        {
                            actor[spritenum].lasttransport = totalclock + (TICSPERFRAME<<2);

                            spr->x += (sprite[OW].x-SX);
                            spr->y += (sprite[OW].y-SY);
                            spr->z = sector[sprite[OW].sectnum].ceilingz - daz + sector[sprite[i].sectnum].floorz;

                            Bmemcpy(&actor[spritenum].bposx, &sprite[spritenum], sizeof(vec3_t));
                            changespritesect(spritenum,sprite[OW].sectnum);
                        }

                        return 0;
                    }
                case 2:
                    if (daz <= actor[spritenum].ceilingz)
                    {
                        if (totalclock > actor[spritenum].lasttransport)
                        {
                            actor[spritenum].lasttransport = totalclock + (TICSPERFRAME<<2);
                            spr->x += (sprite[OW].x-SX);
                            spr->y += (sprite[OW].y-SY);
                            spr->z = sector[sprite[OW].sectnum].floorz - daz + sector[sprite[i].sectnum].ceilingz;

                            Bmemcpy(&actor[spritenum].bposx, &sprite[spritenum], sizeof(vec3_t));
                            changespritesect(spritenum,sprite[OW].sectnum);
                        }

                        return 0;
                    }
                }
            }
        }

    return(retval);
}

ACTOR_INLINE int32_t A_SetSprite(int32_t i,uint32_t cliptype)
{
    vec3_t davect = {(sprite[i].xvel*(sintable[(sprite[i].ang+512)&2047]))>>14,
                     (sprite[i].xvel*(sintable[sprite[i].ang&2047]))>>14,
                     sprite[i].zvel
                    };
    return (A_MoveSprite(i,&davect,cliptype)==0);
}

int32_t block_deletesprite = 0;

// all calls to deletesprite() from the game are wrapped by this function
void A_DeleteSprite(int32_t s)
{
    if (block_deletesprite)
    {
        OSD_Printf(OSD_ERROR "A_DeleteSprite(): tried to remove sprite %d in EVENT_EGS\n",s);
        return;
    }

    if (apScriptGameEvent[EVENT_KILLIT])
    {
        int32_t p, pl=A_FindPlayer(&sprite[s],&p);

        aGameVars[g_iReturnVarID].val.lValue = 0;
        VM_OnEvent(EVENT_KILLIT, s, pl, p);
        if (aGameVars[g_iReturnVarID].val.lValue)
            return;
    }

#ifdef POLYMER
    if (getrendermode() == 4 && actor[s].lightptr != NULL)
    {
        polymer_deletelight(actor[s].lightId);
        actor[s].lightId = -1;
        actor[s].lightptr = NULL;
    }
#endif

#undef deletesprite
    deletesprite(s);
#define deletesprite A_DeleteSprite
}

void A_AddToDeleteQueue(int32_t i)
{
    if (g_netClientPeer || g_spriteDeleteQueueSize == 0)
    {
        deletesprite(i);
        return;
    }

    if (SpriteDeletionQueue[g_spriteDeleteQueuePos] >= 0)
        sprite[SpriteDeletionQueue[g_spriteDeleteQueuePos]].xrepeat = 0;
    SpriteDeletionQueue[g_spriteDeleteQueuePos] = i;
    g_spriteDeleteQueuePos = (g_spriteDeleteQueuePos+1)%g_spriteDeleteQueueSize;
}

void A_SpawnMultiple(int32_t sp, int32_t pic, int32_t n)
{
    int32_t j;
    spritetype *s = &sprite[sp];

    for (; n>0; n--)
    {
        j = A_InsertSprite(s->sectnum,s->x,s->y,s->z-(krand()%(47<<8)),pic,-32,8,8,krand()&2047,0,0,sp,5);
        A_Spawn(-1, j);
        sprite[j].cstat = krand()&12;
    }
}

void A_DoGuts(int32_t sp, int32_t gtype, int32_t n)
{
    int32_t gutz,floorz;
    int32_t i,a,j,sx = 32,sy = 32;
//    int32_t pal;
    spritetype *s = &sprite[sp];

    if (A_CheckEnemySprite(s) && s->xrepeat < 16)
        sx = sy = 8;

    gutz = s->z-(8<<8);
    floorz = getflorzofslope(s->sectnum,s->x,s->y);

    if (gutz > (floorz-(8<<8)))
        gutz = floorz-(8<<8);

    if (s->picnum == COMMANDER)
        gutz -= (24<<8);

//    if (A_CheckEnemySprite(s) && s->pal == 6)
//        pal = 6;
//    else pal = 0;

    for (j=n; j>0; j--)
    {
        a = krand()&2047;
        i = A_InsertSprite(s->sectnum,s->x+(krand()&255)-128,s->y+(krand()&255)-128,gutz-(krand()&8191),gtype,-32,sx,sy,a,48+(krand()&31),-512-(krand()&2047),sp,5);
        if (PN == JIBS2)
        {
            sprite[i].xrepeat >>= 2;
            sprite[i].yrepeat >>= 2;
        }
//        if (pal == 6)
//            sprite[i].pal = 6;
        sprite[i].pal = s->pal;
    }
}

void A_DoGutsDir(int32_t sp, int32_t gtype, int32_t n)
{
    int32_t gutz,floorz;
    int32_t i,a,j,sx = 32,sy = 32;
    spritetype *s = &sprite[sp];

    if (A_CheckEnemySprite(s) && s->xrepeat < 16)
        sx = sy = 8;

    gutz = s->z-(8<<8);
    floorz = getflorzofslope(s->sectnum,s->x,s->y);

    if (gutz > (floorz-(8<<8)))
        gutz = floorz-(8<<8);

    if (s->picnum == COMMANDER)
        gutz -= (24<<8);

    for (j=n; j>0; j--)
    {
        a = krand()&2047;
        i = A_InsertSprite(s->sectnum,s->x,s->y,gutz,gtype,-32,sx,sy,a,256+(krand()&127),-512-(krand()&2047),sp,5);
        sprite[i].pal = s->pal;
    }
}

void Sect_SetInterpolation(int32_t i)
{
    int32_t k, j = sector[SECT].wallptr, endwall = j+sector[SECT].wallnum;

    for (; j<endwall; j++)
    {
        G_SetInterpolation(&wall[j].x);
        G_SetInterpolation(&wall[j].y);
        k = wall[j].nextwall;
        if (k >= 0)
        {
            G_SetInterpolation(&wall[k].x);
            G_SetInterpolation(&wall[k].y);
            k = wall[k].point2;
            G_SetInterpolation(&wall[k].x);
            G_SetInterpolation(&wall[k].y);
        }
    }
}

void Sect_ClearInterpolation(int32_t i)
{
    int32_t k, j = sector[SECT].wallptr, endwall = j+sector[SECT].wallnum;

    for (; j<endwall; j++)
    {
        G_StopInterpolation(&wall[j].x);
        G_StopInterpolation(&wall[j].y);
        k = wall[j].nextwall;
        if (k >= 0)
        {
            G_StopInterpolation(&wall[k].x);
            G_StopInterpolation(&wall[k].y);
            k = wall[k].point2;
            G_StopInterpolation(&wall[k].x);
            G_StopInterpolation(&wall[k].y);
        }
    }
}

static void A_MoveSector(int32_t i)
{
    //T1,T2 and T3 are used for all the sector moving stuff!!!

    int32_t tx,ty;
    spritetype *s = &sprite[i];
    int32_t j = T2, k = T3;

    s->x += (s->xvel*(sintable[(s->ang+512)&2047]))>>14;
    s->y += (s->xvel*(sintable[s->ang&2047]))>>14;

    {
        int32_t x = sector[s->sectnum].wallptr, endwall = x+sector[s->sectnum].wallnum;

        for (; x<endwall; x++)
        {
            rotatepoint(0,0,msx[j],msy[j],k&2047,&tx,&ty);
            dragpoint(x,s->x+tx,s->y+ty);

            j++;
        }
    }
}

// this is the same crap as in game.c's tspr manipulation.  puke.
#define LIGHTRAD (s->yrepeat * tilesizy[s->picnum+(T5?(*(intptr_t *)T5) + *(((intptr_t *)T5)+2) * T4:0)])
#define LIGHTRAD2 (((s->yrepeat) + (rand()%(s->yrepeat>>2))) * tilesizy[s->picnum+(T5?(*(intptr_t *)T5) + *(((intptr_t *)T5)+2) * T4:0)])

void G_AddGameLight(int32_t radius, int32_t srcsprite, int32_t zoffset, int32_t range, int32_t color, int32_t priority)
{
#ifdef POLYMER
    spritetype *s = &sprite[srcsprite];

    if (getrendermode() != 4)
        return;

    if (actor[srcsprite].lightptr == NULL)
    {
#pragma pack(push,1)
        _prlight mylight;
#pragma pack(pop)

        mylight.sector = s->sectnum;
        mylight.x = s->x;
        mylight.y = s->y;
        mylight.z = s->z-zoffset;
        mylight.color[0] = color&255;
        mylight.color[1] = (color>>8)&255;
        mylight.color[2] = (color>>16)&255;
        mylight.radius = radius;
        actor[srcsprite].lightmaxrange = mylight.range = range;

        mylight.priority = priority;
        mylight.tilenum = 0;

        actor[srcsprite].lightId = polymer_addlight(&mylight);
        if (actor[srcsprite].lightId >= 0)
            actor[srcsprite].lightptr = &prlights[actor[srcsprite].lightId];
        return;
    }

    s->z -= zoffset;

    if (range < actor[srcsprite].lightmaxrange>>1)
        actor[srcsprite].lightmaxrange = 0;

    if (range > actor[srcsprite].lightmaxrange ||
            priority != actor[srcsprite].lightptr->priority ||
            Bmemcmp(&sprite[srcsprite], actor[srcsprite].lightptr, sizeof(int32_t) * 3))
    {
        if (range > actor[srcsprite].lightmaxrange)
            actor[srcsprite].lightmaxrange = range;

        Bmemcpy(actor[srcsprite].lightptr, &sprite[srcsprite], sizeof(int32_t) * 3);
        actor[srcsprite].lightptr->sector = s->sectnum;
        actor[srcsprite].lightptr->flags.invalidate = 1;
    }

    actor[srcsprite].lightptr->priority = priority;
    actor[srcsprite].lightptr->range = range;
    actor[srcsprite].lightptr->color[0] = color&255;
    actor[srcsprite].lightptr->color[1] = (color>>8)&255;
    actor[srcsprite].lightptr->color[2] = (color>>16)&255;

    s->z += zoffset;

#else
    UNREFERENCED_PARAMETER(radius);
    UNREFERENCED_PARAMETER(srcsprite);
    UNREFERENCED_PARAMETER(zoffset);
    UNREFERENCED_PARAMETER(range);
    UNREFERENCED_PARAMETER(color);
    UNREFERENCED_PARAMETER(priority);
#endif
}

// sleeping monsters, etc
ACTOR_STATIC void G_MoveZombieActors(void)
{
    int32_t x, px, py, sx, sy;
    int32_t i = headspritestat[STAT_ZOMBIEACTOR], j, p, nexti;
    int16_t psect, ssect;
    spritetype *s;

    while (i >= 0)
    {
        nexti = nextspritestat[i];

        s = &sprite[i];
        p = A_FindPlayer(s,&x);

        ssect = psect = s->sectnum;

        if (sprite[g_player[p].ps->i].extra > 0)
        {
            if (x < 30000)
            {
                actor[i].timetosleep++;
                if (actor[i].timetosleep >= (x>>8))
                {
                    if (A_CheckEnemySprite(s))
                    {
                        px = g_player[p].ps->opos.x+64-(krand()&127);
                        py = g_player[p].ps->opos.y+64-(krand()&127);
                        updatesector(px,py,&psect);
                        if (psect == -1)
                        {
                            i = nexti;
                            continue;
                        }
                        sx = s->x+64-(krand()&127);
                        sy = s->y+64-(krand()&127);
                        updatesector(px,py,&ssect);
                        if (ssect == -1)
                        {
                            i = nexti;
                            continue;
                        }
                        j = cansee(sx,sy,s->z-(krand()%(52<<8)),s->sectnum,px,py,g_player[p].ps->opos.z-(krand()%(32<<8)),g_player[p].ps->cursectnum);
                    }
                    else
                        j = cansee(s->x,s->y,s->z-((krand()&31)<<8),s->sectnum,g_player[p].ps->opos.x,g_player[p].ps->opos.y,g_player[p].ps->opos.z-((krand()&31)<<8),g_player[p].ps->cursectnum);

                    //             j = 1;

                    if (j) switch (DynamicTileMap[s->picnum])
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
                            if (sector[s->sectnum].ceilingstat&1 && A_CheckSpriteFlags(j,SPRITE_NOSHADE) == 0)
                                s->shade = sector[s->sectnum].ceilingshade;
                            else s->shade = sector[s->sectnum].floorshade;

                            actor[i].timetosleep = 0;
                            changespritestat(i,6);
                            break;
                        case RECON__STATIC:
                            CS |= 257;
                        default:
                            if (A_CheckSpriteFlags(i, SPRITE_USEACTIVATOR) && sector[sprite[i].sectnum].lotag & 16384)
                                break;
                            actor[i].timetosleep = 0;
                            A_PlayAlertSound(i);
                            changespritestat(i, STAT_ACTOR);
                            break;
                        }
                    else actor[i].timetosleep = 0;
                }
            }
            if (A_CheckEnemySprite(s) && A_CheckSpriteFlags(i,SPRITE_NOSHADE) == 0)
            {
                if (sector[s->sectnum].ceilingstat&1)
                    s->shade = sector[s->sectnum].ceilingshade;
                else s->shade = sector[s->sectnum].floorshade;
            }
        }
        i = nexti;
    }
}

static inline int32_t ifhitsectors(int32_t sectnum)
{
    int32_t i = headspritestat[STAT_MISC];
    while (i >= 0)
    {
        if (PN == EXPLOSION2 && sectnum == SECT)
            return i;
        i = nextspritestat[i];
    }
    return -1;
}

#define IFHITSECT j=ifhitsectors(s->sectnum);if(j >= 0)

int32_t A_IncurDamage(int32_t sn)
{
    int32_t j,p;
    spritetype *npc;

    if (actor[sn].extra >= 0)
    {
        if (sprite[sn].extra >= 0)
        {
            npc = &sprite[sn];

            if (npc->picnum == APLAYER)
            {
                if (ud.god && actor[sn].picnum != SHRINKSPARK) return -1;

                p = npc->yvel;
                j = actor[sn].owner;

                if (j >= 0 &&
                        sprite[j].picnum == APLAYER &&
                        (GametypeFlags[ud.coop] & GAMETYPE_PLAYERSFRIENDLY) &&
                        ud.ffire == 0)
                    return -1;

                if (j >= 0 &&
                        sprite[j].picnum == APLAYER &&
                        (GametypeFlags[ud.coop] & GAMETYPE_TDM) &&
                        g_player[p].ps->team == g_player[sprite[j].yvel].ps->team &&
                        ud.ffire == 0)
                    return -1;

                npc->extra -= actor[sn].extra;

                if (j >= 0)
                {
                    if (npc->extra <= 0 && actor[sn].picnum != FREEZEBLAST)
                    {
                        npc->extra = 0;

                        g_player[p].ps->wackedbyactor = j;

                        if (sprite[actor[sn].owner].picnum == APLAYER && p != sprite[actor[sn].owner].yvel)
                            g_player[p].ps->frag_ps = sprite[j].yvel;

                        actor[sn].owner = g_player[p].ps->i;
                    }
                }

                if (A_CheckSpriteTileFlags(actor[sn].picnum,SPRITE_PROJECTILE) && (SpriteProjectile[sn].workslike & PROJECTILE_RPG))
                {
                    g_player[p].ps->posvel.x +=
                        actor[sn].extra*(sintable[(actor[sn].ang+512)&2047])<<2;
                    g_player[p].ps->posvel.y +=
                        actor[sn].extra*(sintable[actor[sn].ang&2047])<<2;
                }
                else if (A_CheckSpriteTileFlags(actor[sn].picnum,SPRITE_PROJECTILE))
                {
                    g_player[p].ps->posvel.x +=
                        actor[sn].extra*(sintable[(actor[sn].ang+512)&2047])<<1;
                    g_player[p].ps->posvel.y +=
                        actor[sn].extra*(sintable[actor[sn].ang&2047])<<1;
                }

                switch (DynamicTileMap[actor[sn].picnum])
                {
                case RADIUSEXPLOSION__STATIC:
                case RPG__STATIC:
                case HYDRENT__STATIC:
                case HEAVYHBOMB__STATIC:
                case SEENINE__STATIC:
                case OOZFILTER__STATIC:
                case EXPLODINGBARREL__STATIC:
                    g_player[p].ps->posvel.x +=
                        actor[sn].extra*(sintable[(actor[sn].ang+512)&2047])<<2;
                    g_player[p].ps->posvel.y +=
                        actor[sn].extra*(sintable[actor[sn].ang&2047])<<2;
                    break;
                default:
                    g_player[p].ps->posvel.x +=
                        actor[sn].extra*(sintable[(actor[sn].ang+512)&2047])<<1;
                    g_player[p].ps->posvel.y +=
                        actor[sn].extra*(sintable[actor[sn].ang&2047])<<1;
                    break;
                }
            }
            else
            {
                if (actor[sn].extra == 0)
                    if (actor[sn].picnum == SHRINKSPARK && npc->xrepeat < 24)
                        return -1;

                npc->extra -= actor[sn].extra;
                if (npc->picnum != RECON && npc->owner >= 0 && sprite[npc->owner].statnum < MAXSTATUS)
                    npc->owner = actor[sn].owner;
            }

            actor[sn].extra = -1;
            return actor[sn].picnum;
        }
    }

    actor[sn].extra = -1;
    return -1;
}

void A_MoveCyclers(void)
{
    int32_t q, j, x, t, s, cshade;
    int16_t *c;
    walltype *wal;

    for (q=g_numCyclers-1; q>=0; q--)
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
            for (x = sector[s].wallnum; x>0; x--,wal++)
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

void A_MoveDummyPlayers(void)
{
    int32_t i = headspritestat[STAT_DUMMYPLAYER], p, nexti;

    while (i >= 0)
    {
        nexti = nextspritestat[i];

        p = sprite[OW].yvel;

        if (g_player[p].ps->on_crane >= 0 || (g_player[p].ps->cursectnum >= 0 && sector[g_player[p].ps->cursectnum].lotag != 1) || sprite[g_player[p].ps->i].extra <= 0)
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
                CS = (int16_t) 32768;
            }
        }

        SX += (g_player[p].ps->pos.x-g_player[p].ps->opos.x);
        SY += (g_player[p].ps->pos.y-g_player[p].ps->opos.y);
        setsprite(i,(vec3_t *)&sprite[i]);

BOLT:

        i = nexti;
    }
}

int32_t otherp;

ACTOR_STATIC void G_MovePlayers(void)
{
    int32_t i = headspritestat[STAT_PLAYER], nexti;
    int32_t otherx;
    spritetype *s;
    DukePlayer_t *p;

    while (i >= 0)
    {
        nexti = nextspritestat[i];

        s = &sprite[i];
        p = g_player[s->yvel].ps;
        if (s->owner >= 0)
        {
            if (p->newowner >= 0)  //Looking thru the camera
            {
                s->x = p->opos.x;
                s->y = p->opos.y;
                actor[i].bposz = s->z = p->opos.z+PHEIGHT;
                s->ang = p->oang;
                setsprite(i,(vec3_t *)s);
            }
            else
            {
                if (g_netServer || (g_netServer || ud.multimode > 1))
                    otherp = P_FindOtherPlayer(s->yvel,&otherx);
                else
                {
                    otherp = s->yvel;
                    otherx = 0;
                }

                if (actorscrptr[sprite[i].picnum])
                    A_Execute(i,s->yvel,otherx);

                if (g_netServer || (g_netServer || ud.multimode > 1))
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
                    p->inv_amount[GET_JETPACK] =     1599;
                }


                if (s->extra > 0)
                {
                    actor[i].owner = i;

                    if (ud.god == 0)
                        if (G_CheckForSpaceCeiling(s->sectnum) || G_CheckForSpaceFloor(s->sectnum))
                            P_QuickKill(p);
                }
                else
                {

                    p->pos.x = s->x;
                    p->pos.y = s->y;
                    p->pos.z = s->z-(20<<8);

                    p->newowner = -1;

                    if (p->wackedbyactor >= 0 && sprite[p->wackedbyactor].statnum < MAXSTATUS)
                    {
                        p->ang += G_GetAngleDelta(p->ang,getangle(sprite[p->wackedbyactor].x-p->pos.x,sprite[p->wackedbyactor].y-p->pos.y))>>1;
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

            Bmemcpy(&actor[i].bposx, s, sizeof(vec3_t));
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
                    A_Fall(i);
                if (s->zvel == 0 && sector[s->sectnum].lotag == 1)
                    s->z += (32<<8);
            }

            if (s->extra < 8)
            {
                s->xvel = 128;
                s->ang = p->ang;
                s->extra++;
                A_SetSprite(i,CLIPMASK0);
            }
            else
            {
                s->ang = 2047-p->ang;
                setsprite(i,(vec3_t *)s);
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

ACTOR_STATIC void G_MoveFX(void)
{
    int32_t i = headspritestat[STAT_FX], j, nexti, p;
    int32_t x, ht;
    spritetype *s;

    while (i >= 0)
    {
        s = &sprite[i];

        nexti = nextspritestat[i];

        switch (DynamicTileMap[s->picnum])
        {
        case RESPAWN__STATIC:
            if (sprite[i].extra == 66)
            {
                j = A_Spawn(i,SHT);
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
                        if (g_numEnvSoundsPlaying == ud.config.NumVoices)
                        {
                            j = headspritestat[STAT_FX];
                            while (j >= 0)
                            {
                                if (PN == MUSICANDSFX && j != i && sprite[j].lotag < 999 && actor[j].t_data[0] == 1 && dist(&sprite[j],&sprite[g_player[screenpeek].ps->i]) > x)
                                {
                                    S_StopEnvSound(sprite[j].lotag,j);
                                    break;
                                }
                                j = nextspritestat[j];
                            }
                            if (j == -1) goto BOLT;
                        }
                        A_PlaySound(s->lotag,i);
                        T1 = 1;
                    }
                    if (x >= ht && T1 == 1)
                    {
                        // T1 = 0;
                        S_StopEnvSound(s->lotag,i);
                    }
                }
                if ((g_sounds[s->lotag].m&16))
                {
                    if (T5 > 0) T5--;
                    else
                        TRAVERSE_CONNECT(p)
                        if (p == myconnectindex && g_player[p].ps->cursectnum == s->sectnum)
                        {
                            j = s->lotag+((unsigned)g_globalRandom%(s->hitag+1));
                            S_PlaySound(j);
                            T5 =  GAMETICSPERSEC*40 + (g_globalRandom%(GAMETICSPERSEC*40));
                        }
                }
            }
            break;
        }
BOLT:
        i = nexti;
    }
}

ACTOR_STATIC void G_MoveFallers(void)
{
    int32_t i = headspritestat[STAT_FALLER], nexti, sect, j;
    spritetype *s;
    int32_t x;

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
                        j = headspritestat[STAT_FALLER];
                        while (j >= 0)
                        {
                            if (sprite[j].hitag == SHT)
                            {
                                actor[j].t_data[0] = 1;
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
                    actor[i].extra = 0;
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
                    s->xvel = (32+(krand()&63));
                    s->zvel = -(1024+(krand()&1023));
                }
            }
            else
            {
                if (s->xvel > 0)
                {
                    s->xvel -= 8;
                    A_SetSprite(i,CLIPMASK0);
                }

                if (G_CheckForSpaceFloor(s->sectnum))
                    x = 0;
                else
                {
                    if (G_CheckForSpaceCeiling(s->sectnum))
                        x = g_spriteGravity/6;
                    else
                        x = g_spriteGravity;
                }

                if (s->z < (sector[sect].floorz-ZOFFSET))
                {
                    s->zvel += x;
                    if (s->zvel > 6144)
                        s->zvel = 6144;
                    s->z += s->zvel;
                }
                if ((sector[sect].floorz-s->z) < (16<<8))
                {
                    j = 1+(krand()&7);
                    for (x=0; x<j; x++) RANDOMSCRAP;
                    KILLIT(i);
                }
            }
        }

BOLT:
        i = nexti;
    }
}

ACTOR_STATIC void G_MoveStandables(void)
{
    int32_t i = headspritestat[STAT_STANDABLE], j, k, nexti, nextj, p=0, sect, switchpicnum;
    int32_t l=0, x;
    intptr_t *t;
    spritetype *s;
    int16_t m;

    while (i >= 0)
    {
        nexti = nextspritestat[i];

        t = &actor[i].t_data[0];
        s = &sprite[i];
        sect = s->sectnum;

        if (sect < 0) KILLIT(i);

        Bmemcpy(&actor[i].bposx, s, sizeof(vec3_t));

        IFWITHIN(CRANE,CRANE+3)
        {
            //t[0] = state
            //t[1] = checking sector number

            if (s->xvel) A_GetZLimits(i);

            if (t[0] == 0)   //Waiting to check the sector
            {
                j = headspritesect[t[1]];
                while (j>=0)
                {
                    nextj = nextspritesect[j];
                    switch (sprite[j].statnum)
                    {
                    case STAT_ACTOR:
                    case STAT_ZOMBIEACTOR:
                    case STAT_STANDABLE:
                    case STAT_PLAYER:
                    {
                        vec3_t vect;

                        vect.x = msx[t[4]+1];
                        vect.y = msy[t[4]+1];
                        vect.z = sprite[j].z;
                        s->ang = getangle(msx[t[4]+1]-s->x,msy[t[4]+1]-s->y);
                        setsprite(j,&vect);
                        t[0]++;
                        goto BOLT;

                    }
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
                A_SetSprite(i,CLIPMASK0);
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
                                p = A_FindPlayer(s,&x);
                                A_PlaySound(DUKE_GRUNT,g_player[p].ps->i);
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
                    p = G_CheckPlayerInSector(t[1]);
                    if (p >= 0 && g_player[p].ps->on_ground)
                    {
                        s->owner = -2;
                        g_player[p].ps->on_crane = i;
                        A_PlaySound(DUKE_GRUNT,g_player[p].ps->i);
                        g_player[p].ps->ang = s->ang+1024;
                    }
                    else
                    {
                        j = headspritesect[t[1]];
                        while (j>=0)
                        {
                            switch (sprite[j].statnum)
                            {
                            case STAT_ACTOR:
                            case STAT_STANDABLE:
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
                A_SetSprite(i,CLIPMASK0);
                if (((s->x-msx[t[4]])*(s->x-msx[t[4]])+(s->y-msy[t[4]])*(s->y-msy[t[4]])) < (128*128))
                    t[0]++;
            }

            else if (t[0]==9)
                t[0] = 0;

            {
                vec3_t vect;
                Bmemcpy(&vect,s,sizeof(vec3_t));
                vect.z -= (34<<8);
                setsprite(msy[t[4]+2],&vect);
            }


            if (s->owner != -1)
            {
                p = A_FindPlayer(s,&x);

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
                    setsprite(s->owner,(vec3_t *)s);

                    Bmemcpy(&actor[s->owner].bposx, s, sizeof(vec3_t));

                    s->zvel = 0;
                }
                else if (s->owner == -2)
                {
                    g_player[p].ps->opos.x = g_player[p].ps->pos.x = s->x-(sintable[(g_player[p].ps->ang+512)&2047]>>6);
                    g_player[p].ps->opos.y = g_player[p].ps->pos.y = s->y-(sintable[g_player[p].ps->ang&2047]>>6);
                    g_player[p].ps->opos.z = g_player[p].ps->pos.z = s->z+(2<<8);

                    setsprite(g_player[p].ps->i,(vec3_t *)g_player[p].ps);
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
                    p = A_FindPlayer(s,&x);

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
                    j = A_Spawn(i,BLOODPOOL);
                    sprite[j].shade = 127;
                }
                else
                {
                    if (s->shade < 64) s->shade++;
                    else KILLIT(i);
                }

                j = s->xrepeat-(krand()&7);
                if (j < 10)
                {
                    KILLIT(i);
                }

                s->xrepeat = j;

                j = s->yrepeat-(krand()&7);
                if (j < 4)
                {
                    KILLIT(i);
                }
                s->yrepeat = j;
            }
            if (s->picnum == BOX)
            {
                A_Fall(i);
                actor[i].ceilingz = sector[s->sectnum].ceilingz;
            }
            goto BOLT;
        }

        if (s->picnum == TRIPBOMB)
        {
            if (actor[i].t_data[6] == 1)
            {

                if (actor[i].t_data[7] >= 1)
                {
                    actor[i].t_data[7]--;
                }

                if (actor[i].t_data[7] <= 0)
                {
                    T3=16;
                    actor[i].t_data[6]=3;
                    A_PlaySound(LASERTRIP_ARMING,i);
                }
                // we're on a timer....
            }
            if (T3 > 0 && actor[i].t_data[6] == 3)
            {
                T3--;

                if (T3 == 8)
                {
                    for (j=0; j<5; j++) RANDOMSCRAP;
                    x = s->extra;
                    A_RadiusDamage(i, g_tripbombBlastRadius, x>>2,x>>1,x-(x>>2),x);

                    j = A_Spawn(i,EXPLOSION2);
                    A_PlaySound(LASERTRIP_EXPLODE,j);
                    sprite[j].ang = s->ang;
                    sprite[j].xvel = 348;
                    A_SetSprite(j,CLIPMASK0);

                    j = headspritestat[STAT_MISC];
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
                IFHIT { actor[i].t_data[6] = 3;
                        T3 = 16;
                      }
                s->extra = x;
                s->ang = l;
            }

            switch (T1)
            {
            default:
                p = A_FindPlayer(s,&x);
                if (x > 768 || T1 > 16) T1++;
                break;

            case 32:
                l = s->ang;
                s->ang = T6;

                T4 = s->x;
                T5 = s->y;

                s->x += sintable[(T6+512)&2047]>>9;
                s->y += sintable[(T6)&2047]>>9;
                s->z -= (3<<8);

                setsprite(i,(vec3_t *)s);

                x = A_CheckHitSprite(i,&m);

                actor[i].lastvx = x;

                s->ang = l;

                k = 0;

                //                if(lTripBombControl & TRIPBOMB_TRIPWIRE)
                if (actor[i].t_data[6] != 1)
                {
                    // we're on a trip wire

                    while (x > 0)
                    {
                        j = A_Spawn(i,LASERLINE);
                        setsprite(j,(vec3_t *)&sprite[j]);
                        sprite[j].hitag = s->hitag;
                        actor[j].t_data[1] = sprite[j].z;

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

                setsprite(i,(vec3_t *)s);
                T4 = T3 = 0;

                if (m >= 0 && actor[i].t_data[6] != 1)
                {
                    actor[i].t_data[6] = 3;
                    T3 = 13;
                    A_PlaySound(LASERTRIP_ARMING,i);
                }
                break;

            case 33:
                T2++;

                T4 = s->x;
                T5 = s->y;

                s->x += sintable[(T6+512)&2047]>>9;
                s->y += sintable[(T6)&2047]>>9;
                s->z -= (3<<8);

                setsprite(i,(vec3_t *)s);

                x = A_CheckHitSprite(i,&m);

                s->x = T4;
                s->y = T5;
                s->z += (3<<8);
                setsprite(i,(vec3_t *)s);

                //                if( Actor[i].lastvx != x && lTripBombControl & TRIPBOMB_TRIPWIRE)
                if (actor[i].lastvx != x && actor[i].t_data[6] != 1)
                {
                    actor[i].t_data[6] = 3;
                    T3 = 13;
                    A_PlaySound(LASERTRIP_ARMING,i);
                }
                break;
            }

            goto BOLT;
        }

        if (s->picnum >= CRACK1 && s->picnum <= CRACK4)
        {
            if (s->hitag > 0)
            {
                t[0] = s->cstat;
                t[1] = s->ang;
                j = A_IncurDamage(i);
                if (j == FIREEXT || j == RPG || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
                {
                    j = headspritestat[STAT_STANDABLE];
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
            j = A_IncurDamage(i);
            if (j == -1) goto BOLT;

            for (k=0; k<16; k++)
            {
                j = A_InsertSprite(SECT,SX,SY,SZ-(krand()%(48<<8)),SCRAP3+(krand()&3),-8,48,48,krand()&2047,(krand()&63)+64,-(krand()&4095)-(sprite[i].zvel>>2),i,5);
                sprite[j].pal = 2;
            }

            j = A_Spawn(i,EXPLOSION2);
            A_PlaySound(PIPEBOMB_EXPLODE,j);
            A_PlaySound(GLASS_HEAVYBREAK,j);

            if (s->hitag > 0)
            {
                j = headspritestat[STAT_STANDABLE];
                while (j >= 0)
                {
                    if (s->hitag == sprite[j].hitag && (sprite[j].picnum == OOZFILTER || sprite[j].picnum == SEENINE))
                        if (sprite[j].shade != -32)
                            sprite[j].shade = -32;
                    j = nextspritestat[j];
                }

                x = s->extra;
                A_RadiusDamage(i, g_pipebombBlastRadius,x>>2, x-(x>>1),x-(x>>2), x);
                j = A_Spawn(i,EXPLOSION2);
                A_PlaySound(PIPEBOMB_EXPLODE,j);

                goto DETONATE;
            }
            else
            {
                A_RadiusDamage(i,g_seenineBlastRadius,10,15,20,25);
                KILLIT(i);
            }
            goto BOLT;
        }

        if (s->picnum == OOZFILTER || s->picnum == SEENINE || s->picnum == SEENINEDEAD || s->picnum == (SEENINEDEAD+1))
        {
            if (s->shade != -32 && s->shade != -33)
            {
                if (s->xrepeat)
                    j = (A_IncurDamage(i) >= 0);
                else
                    j = 0;

                if (j || s->shade == -31)
                {
                    if (j) s->lotag = 0;

                    t[3] = 1;

                    j = headspritestat[STAT_STANDABLE];
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

                    g_earthquakeTime = 16;

                    j = headspritestat[STAT_EFFECTOR];
                    while (j >= 0)
                    {
                        if (s->hitag == sprite[j].hitag)
                        {
                            if (sprite[j].lotag == 13)
                            {
                                if (actor[j].t_data[2] == 0)
                                    actor[j].t_data[2] = 1;
                            }
                            else if (sprite[j].lotag == 8)
                                actor[j].t_data[4] = 1;
                            else if (sprite[j].lotag == 18)
                            {
                                if (actor[j].t_data[0] == 0)
                                    actor[j].t_data[0] = 1;
                            }
                            else if (sprite[j].lotag == 21)
                                actor[j].t_data[0] = 1;
                        }
                        j = nextspritestat[j];
                    }

                    s->z -= (32<<8);

                    if (s->xrepeat)
                        for (x=0; x<8; x++) RANDOMSCRAP;

                    if ((t[3] == 1 && s->xrepeat) || s->lotag == -99)
                    {
                        int32_t j = A_Spawn(i,EXPLOSION2);
                        x = s->extra;
                        A_RadiusDamage(i,g_seenineBlastRadius,x>>2, x-(x>>1),x-(x>>2), x);
                        A_PlaySound(PIPEBOMB_EXPLODE,j);
                    }

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
                    G_OperateSectors(sect,i);

                    j = headspritesect[sect];
                    while (j >= 0)
                    {
                        if (sprite[j].statnum == STAT_EFFECTOR)
                        {
                            switch (sprite[j].lotag)
                            {
                            case 2:
                            case 21:
                            case 31:
                            case 32:
                            case 36:
                                actor[j].t_data[0] = 1;
                                break;
                            case 3:
                                actor[j].t_data[4] = 1;
                                break;
                            }
                        }
                        else if (sprite[j].statnum == STAT_STANDABLE)
                        {
                            switch (DynamicTileMap[sprite[j].picnum])
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

        switch (DynamicTileMap[switchpicnum])
        {
        case VIEWSCREEN__STATIC:
        case VIEWSCREEN2__STATIC:

            if (s->xrepeat == 0) KILLIT(i);

            p = A_FindPlayer(s, &x);

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
                A_Fall(i);
                if (krand()&1) s->zvel -= 256;
                if (klabs(s->xvel) < 48)
                    s->xvel += (krand()&3);
            }
            else KILLIT(i);
            break;

        case SIDEBOLT1__STATIC:
            //        case SIDEBOLT1+1:
            //        case SIDEBOLT1+2:
            //        case SIDEBOLT1+3:
            p = A_FindPlayer(s, &x);
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
            if ((krand()&8) == 0)
            {
                t[0]=s->xrepeat;
                t[1]=s->yrepeat;
                t[2] = g_globalRandom&4;
                s->xrepeat=s->yrepeat=0;
                goto CLEAR_THE_BOLT2;
            }
            s->picnum++;

            if (l&1) s->cstat ^= 2;

            if ((krand()&1) && sector[sect].floorpicnum == HURTRAIL)
                A_PlaySound(SHORT_CIRCUIT,i);

            if (s->picnum == SIDEBOLT1+4) s->picnum = SIDEBOLT1;

            goto BOLT;

        case BOLT1__STATIC:
            //        case BOLT1+1:
            //        case BOLT1+2:
            //        case BOLT1+3:
            p = A_FindPlayer(s, &x);
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
            else if ((krand()&8) == 0)
            {
                t[0]=s->xrepeat;
                t[1]=s->yrepeat;
                t[2] = g_globalRandom&4;
                s->xrepeat=s->yrepeat=0;
                goto CLEAR_THE_BOLT;
            }
            s->picnum++;

            l = g_globalRandom&7;
            s->xrepeat=l+8;

            if (l&1) s->cstat ^= 2;

            if (s->picnum == (BOLT1+1) && (krand()&7) == 0 && sector[sect].floorpicnum == HURTRAIL)
                A_PlaySound(SHORT_CIRCUIT,i);

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
                if (--t[1] == 0)
                    s->cstat &= 32767;
            }
            else
            {
                A_Fall(i);
                A_SetSprite(i,CLIPMASK0);
                if (s->xvel > 0) s->xvel -= 2;

                if (s->zvel == 0)
                {
                    s->cstat |= 32768;

                    if (s->pal != 2 && s->hitag == 0)
                        A_PlaySound(SOMETHING_DRIPPING,i);

                    if (sprite[s->owner].picnum != WATERDRIP)
                    {
                        KILLIT(i);
                    }
                    else
                    {
                        actor[i].bposz = s->z = t[0];
                        t[1] = 48+(krand()&31);
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
                        p = G_CheckPlayerInSector(sect);
                        if (p >= 0) g_player[p].ps->pos.z += sector[sect].extra;
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
                        p = G_CheckPlayerInSector(sect);
                        if (p >= 0)
                            g_player[p].ps->pos.z -= sector[sect].extra;
                    }
                }
                goto BOLT;
            }

            if (t[5] == 1) goto BOLT;

            p = G_CheckPlayerInSector(sect);
            if (p >= 0 && (g_player[p].ps->on_ground || s->ang == 512))
            {
                if (t[0] == 0 && !G_CheckActivatorMotion(s->lotag))
                {
                    t[0] = 1;
                    t[1] = 1;
                    t[3] = !t[3];
                    G_OperateMasterSwitches(s->lotag);
                    G_OperateActivators(s->lotag,p);
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
                j = headspritestat[STAT_STANDABLE];
                while (j >= 0)
                {
                    if (j != i && sprite[j].picnum == TOUCHPLATE && sprite[j].lotag == s->lotag)
                    {
                        actor[j].t_data[1] = 1;
                        actor[j].t_data[3] = t[3];
                    }
                    j = nextspritestat[j];
                }
            }
            goto BOLT;

        case CANWITHSOMETHING__STATIC:
        case CANWITHSOMETHING2__STATIC:
        case CANWITHSOMETHING3__STATIC:
        case CANWITHSOMETHING4__STATIC:
            A_Fall(i);
            IFHIT
            {
                A_PlaySound(VENT_BUST,i);

                for (j=9; j>=0; j--)
                    RANDOMSCRAP;

                if (s->lotag) A_Spawn(i,s->lotag);

                KILLIT(i);
            }
            goto BOLT;

        case FLOORFLAME__STATIC:
        case FIREBARREL__STATIC:
        case FIREVASE__STATIC:
        case EXPLODINGBARREL__STATIC:
        case WOODENHORSE__STATIC:
        case HORSEONSIDE__STATIC:
        case NUKEBARREL__STATIC:
        case NUKEBARRELDENTED__STATIC:
        case NUKEBARRELLEAKED__STATIC:
        case TOILETWATER__STATIC:
        case RUBBERCAN__STATIC:
        case STEAM__STATIC:
        case CEILINGSTEAM__STATIC:
            if (!actorscrptr[sprite[i].picnum])
                goto BOLT;
            p = A_FindPlayer(s, &x);
            A_Execute(i,p,x);
            goto BOLT;
        case WATERBUBBLEMAKER__STATIC:
            if (!actorscrptr[sprite[i].picnum])
                goto BOLT;
            p = A_FindPlayer(s, &x);
            A_Execute(i,p,x);
            goto BOLT;
        }

BOLT:
        i = nexti;
    }
}

ACTOR_STATIC void A_DoProjectileBounce(int32_t i)
{
    int32_t dax, day, daz = 4096;
    spritetype *s = &sprite[i];
    int32_t hitsect = s->sectnum;
    int32_t k = sector[hitsect].wallptr;
    int32_t l = wall[k].point2;

    int32_t xvect = mulscale10(s->xvel,sintable[(s->ang+512)&2047]);
    int32_t yvect = mulscale10(s->xvel,sintable[s->ang&2047]);
    int32_t zvect = s->zvel;

    int32_t daang = getangle(wall[l].x-wall[k].x,wall[l].y-wall[k].y);

    if (s->z < (actor[i].floorz+actor[i].ceilingz)>>1)
        k = sector[hitsect].ceilingheinum;
    else
        k = sector[hitsect].floorheinum;

    dax = mulscale14(k,sintable[(daang)&2047]);
    day = mulscale14(k,sintable[(daang+1536)&2047]);

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

ACTOR_STATIC void G_MoveWeapons(void)
{
    int32_t i = headspritestat[STAT_PROJECTILE], j=0, k, f, nexti, p, q;
    vec3_t davect;
    int32_t x, ll;
    uint32_t qq;
    spritetype *s;

    while (i >= 0)
    {
        nexti = nextspritestat[i];
        s = &sprite[i];

        if (s->sectnum < 0) KILLIT(i);

        Bmemcpy(&actor[i].bposx, s, sizeof(vec3_t));
        // here

        if (A_CheckSpriteFlags(i,SPRITE_PROJECTILE))
        {
            /* Custom projectiles.  This is a big hack. */

            if (SpriteProjectile[i].pal >= 0)
                s->pal=SpriteProjectile[i].pal;

            if (SpriteProjectile[i].workslike & PROJECTILE_KNEE)
                KILLIT(i);

            if (SpriteProjectile[i].workslike & PROJECTILE_RPG)
            {
                //  if (SpriteProjectile[i].workslike & COOLEXPLOSION1)
                //                if( g_sounds[WIERDSHOT_FLY].num == 0 )
                //                    A_PlaySound(WIERDSHOT_FLY,i);

                Bmemcpy(&davect,s,sizeof(vec3_t));

                if (SpriteProjectile[i].flashcolor)
                    G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), 2048, SpriteProjectile[i].flashcolor,PR_LIGHT_PRIO_LOW_GAME);

                if (SpriteProjectile[i].workslike & PROJECTILE_BOUNCESOFFWALLS)
                {
                    /*                    if(s->yvel < 1 || s->extra < 2 || (s->xvel|s->zvel) == 0)
                    Did this cause the bug with prematurely exploding projectiles? */
                    if (s->yvel < 1)
                    {

                        if (SpriteProjectile[i].spawns >= 0)
                        {
                            k = A_Spawn(i,SpriteProjectile[i].spawns);

//                            Bmemcpy(&sprite[k],&davect,sizeof(vec3_t));
                            /*
                            sprite[k].x = dax;
                            sprite[k].y = day;
                            sprite[k].z = daz;
                            */

                            if (SpriteProjectile[i].sxrepeat > 4)
                                sprite[k].xrepeat=SpriteProjectile[i].sxrepeat;
                            if (SpriteProjectile[i].syrepeat > 4)
                                sprite[k].yrepeat=SpriteProjectile[i].syrepeat;
                        }
                        if (SpriteProjectile[i].isound >= 0)
                            A_PlaySound(SpriteProjectile[i].isound,i);

                        s->extra=SpriteProjectile[i].extra;

                        if (SpriteProjectile[i].extra_rand > 0)
                            s->extra += (krand()&SpriteProjectile[i].extra_rand);

                        x = s->extra;
                        A_RadiusDamage(i,SpriteProjectile[i].hitradius, x>>2,x>>1,x-(x>>2),x);

                        KILLIT(i);
                    }
                }

                p = -1;

                if (SpriteProjectile[i].workslike & PROJECTILE_COOLEXPLOSION1)
                {
                    s->shade++;
                    if (s->shade >= 40) KILLIT(i);
                }

                if (SpriteProjectile[i].drop)
                    s->zvel -= SpriteProjectile[i].drop;

                if (SpriteProjectile[i].workslike & PROJECTILE_SPIT)
                    if (s->zvel < 6144)
                        s->zvel += g_spriteGravity-112;

                k = s->xvel;
                ll = s->zvel;

                if (sector[s->sectnum].lotag == 2)
                {
                    k = s->xvel>>1;
                    ll = s->zvel>>1;
                }

                A_GetZLimits(i);
                qq = CLIPMASK1;

                if (SpriteProjectile[i].trail >= 0)
                {
                    for (f=0; f<=SpriteProjectile[i].tnum; f++)
                    {
                        j = A_Spawn(i,SpriteProjectile[i].trail);
                        if (SpriteProjectile[i].toffset != 0)
                            sprite[j].z += (SpriteProjectile[i].toffset<<8);
                        if (SpriteProjectile[i].txrepeat >= 0)
                            sprite[j].xrepeat=SpriteProjectile[i].txrepeat;
                        if (SpriteProjectile[i].tyrepeat >= 0)
                            sprite[j].yrepeat=SpriteProjectile[i].tyrepeat;
                    }
                }

                for (f=1; f<=SpriteProjectile[i].velmult; f++)
                {
                    vec3_t tmpvect;
                    Bmemcpy(&davect,s,sizeof(vec3_t));

                    tmpvect.x = (k*(sintable[(s->ang+512)&2047]))>>14;
                    tmpvect.y = (k*(sintable[s->ang&2047]))>>14;
                    tmpvect.z = ll;

                    j = A_MoveSprite(i,&tmpvect,qq);
                    if (j)
                        break;
                }

                if (!(SpriteProjectile[i].workslike & PROJECTILE_BOUNCESOFFWALLS) &&
                        s->yvel >= 0 && sprite[s->yvel].sectnum < MAXSECTORS)
                    if (FindDistance2D(s->x-sprite[s->yvel].x,s->y-sprite[s->yvel].y) < 256)
                        j = 49152|s->yvel;

                actor[i].movflag = j;

                if (s->sectnum < 0)
                {
                    KILLIT(i);
                }

                if (SpriteProjectile[i].workslike & PROJECTILE_TIMED && SpriteProjectile[i].range > 0)
                {
                    if (!(actor[i].t_data[8]))
                        actor[i].t_data[8] = 1;
                    else
                        actor[i].t_data[8]++;

                    if (actor[i].t_data[8] > SpriteProjectile[i].range)
                    {
                        if (SpriteProjectile[i].workslike & PROJECTILE_EXPLODEONTIMER)
                        {
                            if (SpriteProjectile[i].spawns >= 0)
                            {
                                k = A_Spawn(i,SpriteProjectile[i].spawns);

                                Bmemcpy(&sprite[k],&davect,sizeof(vec3_t));

                                if (SpriteProjectile[i].sxrepeat > 4)
                                    sprite[k].xrepeat=SpriteProjectile[i].sxrepeat;
                                if (SpriteProjectile[i].syrepeat > 4)
                                    sprite[k].yrepeat=SpriteProjectile[i].syrepeat;
                            }
                            if (SpriteProjectile[i].isound >= 0)
                                A_PlaySound(SpriteProjectile[i].isound,i);

                            s->extra=SpriteProjectile[i].extra;

                            if (SpriteProjectile[i].extra_rand > 0)
                                s->extra += (krand()&SpriteProjectile[i].extra_rand);

                            x = s->extra;
                            A_RadiusDamage(i,SpriteProjectile[i].hitradius, x>>2,x>>1,x-(x>>2),x);
                        }
                        KILLIT(i);
                    }
                }

                if ((j&49152) != 49152)
                    if (!(SpriteProjectile[i].workslike & PROJECTILE_BOUNCESOFFWALLS))
                    {
                        if (s->z < actor[i].ceilingz)
                        {
                            j = 16384|(s->sectnum);
                            s->zvel = -1;
                        }
                        else if ((s->z > actor[i].floorz && sector[s->sectnum].lotag != 1) ||
                                 (s->z > actor[i].floorz+(16<<8) && sector[s->sectnum].lotag == 1))
                        {
                            j = 16384|(s->sectnum);
                            if (sector[s->sectnum].lotag != 1)
                                s->zvel = 1;
                        }
                    }

                if (SpriteProjectile[i].workslike & PROJECTILE_WATERBUBBLES &&
                        sector[s->sectnum].lotag == 2 && rnd(140))
                    A_Spawn(i,WATERBUBBLE);

                if (j != 0)
                {
                    if (SpriteProjectile[i].workslike & PROJECTILE_COOLEXPLOSION1)
                    {
                        s->xvel = 0;
                        s->zvel = 0;
                    }

                    if ((j&49152) == 49152)
                    {
                        j &= (MAXSPRITES-1);

                        if (SpriteProjectile[i].workslike & PROJECTILE_BOUNCESOFFSPRITES)
                        {
                            s->yvel--;

                            k = getangle(sprite[j].x-s->x,sprite[j].y-s->y)+(sprite[j].cstat&16?0:512);
                            s->ang = ((k<<1) - s->ang)&2047;

                            if (SpriteProjectile[i].bsound >= 0)
                                A_PlaySound(SpriteProjectile[i].bsound,i);

                            if (SpriteProjectile[i].workslike & PROJECTILE_LOSESVELOCITY)
                            {
                                s->xvel=s->xvel>>1;
                                s->zvel=s->zvel>>1;
                            }
                            if (!(SpriteProjectile[i].workslike & PROJECTILE_FORCEIMPACT))goto BOLT;
                        }

                        A_DamageObject(j,i);

                        if (sprite[j].picnum == APLAYER)
                        {
                            p = sprite[j].yvel;
                            A_PlaySound(PISTOL_BODYHIT,j);

                            if (SpriteProjectile[i].workslike & PROJECTILE_SPIT)
                            {
                                g_player[p].ps->horiz += 32;
                                g_player[p].ps->return_to_center = 8;

                                if (g_player[p].ps->loogcnt == 0)
                                {
                                    if (!A_CheckSoundPlaying(g_player[p].ps->i, DUKE_LONGTERM_PAIN))
                                        A_PlaySound(DUKE_LONGTERM_PAIN,g_player[p].ps->i);

                                    j = 3+(krand()&3);
                                    g_player[p].ps->numloogs = j;
                                    g_player[p].ps->loogcnt = 24*4;
                                    for (x=0; x < j; x++)
                                    {
                                        g_player[p].ps->loogiex[x] = krand()%xdim;
                                        g_player[p].ps->loogiey[x] = krand()%ydim;
                                    }
                                }
                            }
                        }

                        if (SpriteProjectile[i].workslike & PROJECTILE_RPG_IMPACT)
                        {

                            actor[j].owner = s->owner;
                            actor[j].picnum = s->picnum;
                            actor[j].extra += SpriteProjectile[i].extra;

                            if (SpriteProjectile[i].spawns >= 0)
                            {
                                k = A_Spawn(i,SpriteProjectile[i].spawns);
                                Bmemcpy(&sprite[k],&davect,sizeof(vec3_t));

                                if (SpriteProjectile[i].sxrepeat > 4)
                                    sprite[k].xrepeat=SpriteProjectile[i].sxrepeat;
                                if (SpriteProjectile[i].syrepeat > 4)
                                    sprite[k].yrepeat=SpriteProjectile[i].syrepeat;
                            }

                            if (SpriteProjectile[i].isound >= 0)
                                A_PlaySound(SpriteProjectile[i].isound,i);

                            if (!(SpriteProjectile[i].workslike & PROJECTILE_FORCEIMPACT))
                                KILLIT(i);

                        }
                        if (SpriteProjectile[i].workslike & PROJECTILE_FORCEIMPACT)
                            goto BOLT;

                    }
                    else if ((j&49152) == 32768)
                    {
                        j &= (MAXWALLS-1);

                        if (SpriteProjectile[i].workslike & PROJECTILE_BOUNCESOFFMIRRORS &&
                                (wall[j].overpicnum == MIRROR || wall[j].picnum == MIRROR))
                        {
                            k = getangle(
                                    wall[wall[j].point2].x-wall[j].x,
                                    wall[wall[j].point2].y-wall[j].y);
                            s->ang = ((k<<1) - s->ang)&2047;
                            s->owner = i;
                            A_Spawn(i,TRANSPORTERSTAR);
                            goto BOLT;
                        }
                        else
                        {
                            setsprite(i,&davect);
                            A_DamageWall(i,j,(vec3_t *)s,s->picnum);

                            if (SpriteProjectile[i].workslike & PROJECTILE_BOUNCESOFFWALLS)
                            {
                                if (wall[j].overpicnum != MIRROR && wall[j].picnum != MIRROR)
                                    s->yvel--;

                                k = getangle(
                                        wall[wall[j].point2].x-wall[j].x,
                                        wall[wall[j].point2].y-wall[j].y);
                                s->ang = ((k<<1) - s->ang)&2047;

                                if (SpriteProjectile[i].bsound >= 0)
                                    A_PlaySound(SpriteProjectile[i].bsound,i);

                                if (SpriteProjectile[i].workslike & PROJECTILE_LOSESVELOCITY)
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
                        setsprite(i,&davect);

                        if (s->zvel < 0)
                        {
                            if (sector[s->sectnum].ceilingstat&1 && sector[s->sectnum].ceilingpal == 0)
                                KILLIT(i);

                            Sect_DamageCeiling(s->sectnum);
                        }

                        if (SpriteProjectile[i].workslike & PROJECTILE_BOUNCESOFFWALLS)
                        {
                            A_DoProjectileBounce(i);
                            A_SetSprite(i,qq);

                            s->yvel--;

                            if (SpriteProjectile[i].bsound >= 0)
                                A_PlaySound(SpriteProjectile[i].bsound,i);

                            if (SpriteProjectile[i].workslike & PROJECTILE_LOSESVELOCITY)
                            {
                                s->xvel=s->xvel>>1;
                                s->zvel=s->zvel>>1;
                            }

                            goto BOLT;
                        }
                    }

                    if (SpriteProjectile[i].workslike & PROJECTILE_HITSCAN)
                    {
                        if (!actorscrptr[sprite[i].picnum])
                            goto BOLT;
                        p = A_FindPlayer(s,&x);
                        A_Execute(i,p,x);
                        goto BOLT;
                    }

                    if (SpriteProjectile[i].workslike & PROJECTILE_RPG)
                    {
                        if (SpriteProjectile[i].spawns > 0)
                        {
                            k = A_Spawn(i,SpriteProjectile[i].spawns);
                            Bmemcpy(&sprite[k],&davect,sizeof(vec3_t));

                            if (SpriteProjectile[i].sxrepeat > 4)
                                sprite[k].xrepeat=SpriteProjectile[i].sxrepeat;
                            if (SpriteProjectile[i].syrepeat > 4)
                                sprite[k].yrepeat=SpriteProjectile[i].syrepeat;
                        }

                        if (SpriteProjectile[i].isound >= 0)
                            A_PlaySound(SpriteProjectile[i].isound,i);

                        s->extra=SpriteProjectile[i].extra;
                        if (SpriteProjectile[i].extra_rand > 0)
                            s->extra += (krand()&SpriteProjectile[i].extra_rand);

                        A_RadiusDamage(i,SpriteProjectile[i].hitradius,
                                       s->extra>>2,s->extra>>1,s->extra-(s->extra>>2),s->extra);
                        KILLIT(i);
                    }
                }
                goto BOLT;
            }
        }
        else
        {

            // here
            switch (DynamicTileMap[s->picnum])
            {
            case RADIUSEXPLOSION__STATIC:
            case KNEE__STATIC:
                KILLIT(i);
            case TONGUE__STATIC:
                T1 = sintable[(T2)&2047]>>9;
                T2 += 32;
                if (T2 > 2047) KILLIT(i);

                if (sprite[s->owner].statnum == MAXSTATUS)
                    if (A_CheckEnemySprite(&sprite[s->owner]) == 0)
                        KILLIT(i);

                s->ang = sprite[s->owner].ang;
                s->x = sprite[s->owner].x;
                s->y = sprite[s->owner].y;
                if (sprite[s->owner].picnum == APLAYER)
                    s->z = sprite[s->owner].z-(34<<8);
                for (k=0; k<T1; k++)
                {
                    q = A_InsertSprite(s->sectnum,
                                       s->x+((k*sintable[(s->ang+512)&2047])>>9),
                                       s->y+((k*sintable[s->ang&2047])>>9),
                                       s->z+((k*ksgn(s->zvel))*klabs(s->zvel/12)),TONGUE,-40+(k<<1),
                                       8,8,0,0,0,i,5);
                    sprite[q].cstat = 128;
                    sprite[q].pal = 8;
                }
                q = A_InsertSprite(s->sectnum,
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
                    j = A_Spawn(i,TRANSPORTERSTAR);
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
                    if (!S_CheckSoundPlaying(i,WIERDSHOT_FLY))
                        A_PlaySound(WIERDSHOT_FLY,i);

                p = -1;

                k = s->xvel;
                ll = s->zvel;

                if (s->picnum == RPG && sector[s->sectnum].lotag == 2)
                {
                    k = s->xvel>>1;
                    ll = s->zvel>>1;
                }

                Bmemcpy(&davect,s,sizeof(vec3_t));

                A_GetZLimits(i);
                qq = CLIPMASK1;

                switch (DynamicTileMap[s->picnum])
                {
                case RPG__STATIC:
                    if (DynamicTileMap[s->picnum] == RPG__STATIC && actor[i].picnum != BOSS2 &&
                            s->xrepeat >= 10 && sector[s->sectnum].lotag != 2)
                    {
                        j = A_Spawn(i,SMALLSMOKE);
                        sprite[j].z += (1<<8);
                    }
                    break;
                }

                {
                    vec3_t tmpvect;

                    tmpvect.x = (k*(sintable[(s->ang+512)&2047]))>>14;
                    tmpvect.y = (k*(sintable[s->ang&2047]))>>14;
                    tmpvect.z = ll;
                    j = A_MoveSprite(i,&tmpvect,qq);
                }


                if (s->picnum == RPG && s->yvel >= 0)
                    if (FindDistance2D(s->x-sprite[s->yvel].x,s->y-sprite[s->yvel].y) < 256)
                        j = 49152|s->yvel;

                actor[i].movflag = j;

                if (s->sectnum < 0)
                    KILLIT(i);

                if ((j&49152) != 49152)
                    if (s->picnum != FREEZEBLAST)
                    {
                        if (s->z < actor[i].ceilingz)
                        {
                            j = 16384|(s->sectnum);
                            s->zvel = -1;
                        }
                        else if ((s->z > actor[i].floorz && sector[s->sectnum].lotag != 1) ||
                                 (s->z > actor[i].floorz+(16<<8) && sector[s->sectnum].lotag == 1))
                        {
                            j = 16384|(s->sectnum);
                            if (sector[s->sectnum].lotag != 1)
                                s->zvel = 1;
                        }
                    }

                if (s->picnum == FIRELASER)
                {
                    for (k=-3; k<2; k++)
                    {
                        x = A_InsertSprite(s->sectnum,
                                           s->x+((k*sintable[(s->ang+512)&2047])>>9),
                                           s->y+((k*sintable[s->ang&2047])>>9),
                                           s->z+((k*ksgn(s->zvel))*klabs(s->zvel/24)),FIRELASER,-40+(k<<2),
                                           s->xrepeat,s->yrepeat,0,0,0,s->owner,5);

                        sprite[x].cstat = 128;
                        sprite[x].pal = s->pal;
                    }
                }
                else if (s->picnum == SPIT) if (s->zvel < 6144)
                        s->zvel += g_spriteGravity-112;

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
                            if (A_CheckEnemySprite(&sprite[j]) || sprite[j].picnum == APLAYER)
                            {
                                j = A_Spawn(i,TRANSPORTERSTAR);
                                sprite[j].pal = 1;
                                sprite[j].xrepeat = 32;
                                sprite[j].yrepeat = 32;

                                KILLIT(i);
                            }

                        A_DamageObject(j,i);

                        if (sprite[j].picnum == APLAYER)
                        {
                            p = sprite[j].yvel;
                            A_PlaySound(PISTOL_BODYHIT,j);

                            if (s->picnum == SPIT)
                            {
                                g_player[p].ps->horiz += 32;
                                g_player[p].ps->return_to_center = 8;

                                if (g_player[p].ps->loogcnt == 0)
                                {
                                    if (!A_CheckSoundPlaying(g_player[p].ps->i, DUKE_LONGTERM_PAIN))
                                        A_PlaySound(DUKE_LONGTERM_PAIN,g_player[p].ps->i);

                                    j = 3+(krand()&3);
                                    g_player[p].ps->numloogs = j;
                                    g_player[p].ps->loogcnt = 24*4;
                                    for (x=0; x < j; x++)
                                    {
                                        g_player[p].ps->loogiex[x] = krand()%xdim;
                                        g_player[p].ps->loogiey[x] = krand()%ydim;
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
                            A_Spawn(i,TRANSPORTERSTAR);
                            goto BOLT;
                        }
                        else
                        {
                            setsprite(i,&davect);
                            A_DamageWall(i,j,(vec3_t *)s,s->picnum);

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
                        setsprite(i,&davect);

                        if (s->zvel < 0)
                        {
                            if (sector[s->sectnum].ceilingstat&1)
                                if (sector[s->sectnum].ceilingpal == 0)
                                    KILLIT(i);

                            Sect_DamageCeiling(s->sectnum);
                        }

                        if (s->picnum == FREEZEBLAST)
                        {
                            A_DoProjectileBounce(i);
                            A_SetSprite(i,qq);
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
                            k = A_Spawn(i,EXPLOSION2);
                            A_PlaySound(RPG_EXPLODE,k);
                            Bmemcpy(&sprite[k],&davect,sizeof(vec3_t));

                            if (s->xrepeat < 10)
                            {
                                sprite[k].xrepeat = 6;
                                sprite[k].yrepeat = 6;
                            }
                            else if ((j&49152) == 16384)
                            {
                                if (s->zvel > 0)
                                    A_Spawn(i,EXPLOSION2BOT);
                                else
                                {
                                    sprite[k].cstat |= 8;
                                    sprite[k].z += (48<<8);
                                }

                            }

                            if (s->xrepeat >= 10)
                            {
                                x = s->extra;
                                A_RadiusDamage(i,g_rpgBlastRadius, x>>2,x>>1,x-(x>>2),x);
                            }
                            else
                            {
                                x = s->extra+(g_globalRandom&3);
                                A_RadiusDamage(i,(g_rpgBlastRadius>>1),x>>2,x>>1,x-(x>>2),x);
                            }
                        }
                        else if (s->picnum == SHRINKSPARK)
                        {
                            A_Spawn(i,SHRINKEREXPLOSION);
                            A_PlaySound(SHRINKER_HIT,i);
                            A_RadiusDamage(i,g_shrinkerBlastRadius,0,0,0,0);
                        }
                        else if (s->picnum != COOLEXPLOSION1 && s->picnum != FREEZEBLAST && s->picnum != FIRELASER)
                        {
                            k = A_Spawn(i,EXPLOSION2);
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
                    A_Spawn(i,WATERBUBBLE);

                goto BOLT;

            case SHOTSPARK1__STATIC:
                if (!actorscrptr[sprite[i].picnum])
                    goto BOLT;
                p = A_FindPlayer(s,&x);
                A_Execute(i,p,x);
                goto BOLT;
            }
        }
BOLT:
        i = nexti;
    }
}

ACTOR_STATIC void G_MoveTransports(void)
{
    int32_t warpspriteto;
    int32_t i = headspritestat[STAT_TRANSPORT], j, k, l, p, sect, sectlotag, nexti, nextj;
    int32_t ll,onfloorz,q;

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
            case STAT_PLAYER:

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
                                A_Spawn(i,TRANSPORTERBEAM);
                                A_PlaySound(TELEPORTER,i);
                            }

                            TRAVERSE_CONNECT(k)
                            if (g_player[k].ps->cursectnum == sprite[OW].sectnum)
                            {
                                g_player[k].ps->frag_ps = p;
                                sprite[g_player[k].ps->i].extra = 0;
                            }

                            g_player[p].ps->ang = sprite[OW].ang;

                            if (sprite[OW].owner != OW)
                            {
                                T1 = 13;
                                actor[OW].t_data[0] = 13;
                                g_player[p].ps->transporter_hold = 13;
                            }

                            g_player[p].ps->bobposx = g_player[p].ps->opos.x = g_player[p].ps->pos.x = sprite[OW].x;
                            g_player[p].ps->bobposy = g_player[p].ps->opos.y = g_player[p].ps->pos.y = sprite[OW].y;
                            g_player[p].ps->opos.z = g_player[p].ps->pos.z = sprite[OW].z-PHEIGHT;

                            changespritesect(j,sprite[OW].sectnum);
                            g_player[p].ps->cursectnum = sprite[j].sectnum;

                            if (sprite[i].pal == 0)
                            {
                                k = A_Spawn(OW,TRANSPORTERBEAM);
                                A_PlaySound(TELEPORTER,k);
                            }

                            break;
                        }
                    }
                    else if (!(sectlotag == 1 && g_player[p].ps->on_ground == 1)) break;

                    if (onfloorz == 0 && klabs(SZ-g_player[p].ps->pos.z) < 6144)
                        if ((g_player[p].ps->jetpack_on == 0) || (g_player[p].ps->jetpack_on && TEST_SYNC_KEY(g_player[p].sync->bits, SK_JUMP)) ||
                                (g_player[p].ps->jetpack_on && TEST_SYNC_KEY(g_player[p].sync->bits, SK_CROUCH)))
                        {
                            g_player[p].ps->opos.x = g_player[p].ps->pos.x += sprite[OW].x-SX;
                            g_player[p].ps->opos.y = g_player[p].ps->pos.y += sprite[OW].y-SY;

                            if (g_player[p].ps->jetpack_on && (TEST_SYNC_KEY(g_player[p].sync->bits, SK_JUMP) || g_player[p].ps->jetpack_on < 11))
                                g_player[p].ps->pos.z = sprite[OW].z-6144;
                            else g_player[p].ps->pos.z = sprite[OW].z+6144;
                            g_player[p].ps->opos.z = g_player[p].ps->pos.z;

                            actor[g_player[p].ps->i].bposx = g_player[p].ps->pos.x;
                            actor[g_player[p].ps->i].bposy = g_player[p].ps->pos.y;
                            actor[g_player[p].ps->i].bposz = g_player[p].ps->pos.z;

                            changespritesect(j,sprite[OW].sectnum);
                            g_player[p].ps->cursectnum = sprite[OW].sectnum;

                            break;
                        }

                    k = 0;

                    if (onfloorz && sectlotag == 1 && g_player[p].ps->on_ground &&
                            g_player[p].ps->pos.z >= sector[sect].floorz &&
                            (TEST_SYNC_KEY(g_player[p].sync->bits, SK_CROUCH) || g_player[p].ps->posvel.z > 2048))
                        //                        if( onfloorz && sectlotag == 1 && g_player[p].ps->pos.z > (sector[sect].floorz-(6<<8)) )
                    {
                        k = 1;
                        if (screenpeek == p)
                        {
                            FX_StopAllSounds();
                            S_ClearSoundLocks();
                        }
                        if (sprite[g_player[p].ps->i].extra > 0)
                            A_PlaySound(DUKE_UNDERWATER,j);
                        g_player[p].ps->opos.z = g_player[p].ps->pos.z =
                                                     sector[sprite[OW].sectnum].ceilingz;

                        /*
                                                g_player[p].ps->posvel.x = 4096-(krand()&8192);
                                                g_player[p].ps->posvel.y = 4096-(krand()&8192);
                        */
                        if (TEST_SYNC_KEY(g_player[p].sync->bits, SK_CROUCH))
                            g_player[p].ps->posvel.z += 512;
                    }

                    if (onfloorz && sectlotag == 2 && g_player[p].ps->pos.z <= sector[sect].ceilingz /*&& g_player[p].ps->posvel.z == 0*/)
                    {
                        k = 1;
                        //                            if( sprite[j].extra <= 0) break;
                        if (screenpeek == p)
                        {
                            FX_StopAllSounds();
                            S_ClearSoundLocks();
                        }
                        A_PlaySound(DUKE_GASP,j);

                        g_player[p].ps->opos.z = g_player[p].ps->pos.z =
                                                     sector[sprite[OW].sectnum].floorz;

                        g_player[p].ps->jumping_toggle = 1;
                        g_player[p].ps->jumping_counter = 0;
                        g_player[p].ps->posvel.z = 0;
                        //                        g_player[p].ps->posvel.z += 1024;
                    }

                    if (k == 1)
                    {
                        vec3_t vect;
                        g_player[p].ps->bobposx = g_player[p].ps->opos.x = g_player[p].ps->pos.x += sprite[OW].x-SX;
                        g_player[p].ps->bobposy = g_player[p].ps->opos.y = g_player[p].ps->pos.y += sprite[OW].y-SY;

                        if (sprite[OW].owner != OW)
                            g_player[p].ps->transporter_hold = -2;
                        g_player[p].ps->cursectnum = sprite[OW].sectnum;

                        changespritesect(j,sprite[OW].sectnum);

                        vect.x = g_player[p].ps->pos.x;
                        vect.y = g_player[p].ps->pos.y;
                        vect.z = g_player[p].ps->pos.z+PHEIGHT;

                        setsprite(g_player[p].ps->i,&vect);

                        P_UpdateScreenPal(g_player[p].ps);

                        if ((krand()&255) < 32)
                            A_Spawn(j,WATERSPLASH2);

                        if (sectlotag == 1)
                            for (l = 0; l < 9; l++)
                            {
                                q = A_Spawn(g_player[p].ps->i,WATERBUBBLE);
                                sprite[q].z += krand()&16383;
                            }
                    }
                }
                break;

            case STAT_PROJECTILE:
                if (sectlotag != 0) goto JBOLT;
            case STAT_ACTOR:
                if ((sprite[j].picnum == SHARK) || (sprite[j].picnum == COMMANDER) || (sprite[j].picnum == OCTABRAIN)
                        || ((sprite[j].picnum >= GREENSLIME) && (sprite[j].picnum <= GREENSLIME+7)))
                {
                    if (sprite[j].extra > 0)
                        goto JBOLT;
                }
            case STAT_MISC:
            case STAT_FALLER:
            case STAT_DUMMYPLAYER:

                ll = klabs(sprite[j].zvel);

                if (totalclock > actor[j].lasttransport)
                {
                    warpspriteto = 0;
                    if (ll && sectlotag == 2 && sprite[j].z < (sector[sect].ceilingz+ll))
                        warpspriteto = 1;

                    if (ll && sectlotag == 1 && sprite[j].z > (sector[sect].floorz-ll))
                        warpspriteto = 1;

                    if (sectlotag == 0 && (onfloorz || klabs(sprite[j].z-SZ) < 4096))
                    {
                        if (sprite[OW].owner != OW && onfloorz && T1 > 0 && sprite[j].statnum != STAT_MISC)
                        {
                            T1++;
                            goto BOLT;
                        }
                        warpspriteto = 1;
                    }

                    if (warpspriteto && A_CheckSpriteFlags(j,SPRITE_DECAL)) goto JBOLT;

                    if (warpspriteto) switch (DynamicTileMap[sprite[j].picnum])
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
                                sprite[j].cstat &= 32768;
                                break;
                            }
                        default:
                            if (sprite[j].statnum == STAT_MISC && !(sectlotag == 1 || sectlotag == 2))
                                break;

                        case WATERBUBBLE__STATIC:
                            //                                if( rnd(192) && sprite[j].picnum == WATERBUBBLE)
                            //                                 break;

                            if (sectlotag > 0)
                            {
                                k = A_Spawn(j,WATERSPLASH2);
                                if (sectlotag == 1 && sprite[j].statnum == STAT_PROJECTILE)
                                {
                                    sprite[k].xvel = sprite[j].xvel>>1;
                                    sprite[k].ang = sprite[j].ang;
                                    A_SetSprite(k,CLIPMASK0);
                                }
                            }

                            switch (sectlotag)
                            {
                            case 0:
                                if (onfloorz)
                                {
                                    if (sprite[j].statnum == STAT_PROJECTILE ||
                                            (G_CheckPlayerInSector(sect) == -1 && G_CheckPlayerInSector(sprite[OW].sectnum)  == -1))
                                    {
                                        sprite[j].x += (sprite[OW].x-SX);
                                        sprite[j].y += (sprite[OW].y-SY);
                                        sprite[j].z -= SZ - sector[sprite[OW].sectnum].floorz;
                                        sprite[j].ang = sprite[OW].ang;

                                        Bmemcpy(&actor[j].bposx, &sprite[j], sizeof(vec3_t));

                                        if (sprite[i].pal == 0)
                                        {
                                            k = A_Spawn(i,TRANSPORTERBEAM);
                                            A_PlaySound(TELEPORTER,k);

                                            k = A_Spawn(OW,TRANSPORTERBEAM);
                                            A_PlaySound(TELEPORTER,k);
                                        }

                                        if (sprite[OW].owner != OW)
                                        {
                                            T1 = 13;
                                            actor[OW].t_data[0] = 13;
                                        }

                                        changespritesect(j,sprite[OW].sectnum);
                                    }
                                }
                                else
                                {
                                    sprite[j].x += (sprite[OW].x-SX);
                                    sprite[j].y += (sprite[OW].y-SY);
                                    sprite[j].z = sprite[OW].z+4096;

                                    Bmemcpy(&actor[j].bposx, &sprite[j], sizeof(vec3_t));

                                    changespritesect(j,sprite[OW].sectnum);
                                }
                                break;
                            case 1:
                                actor[j].lasttransport = totalclock + (TICSPERFRAME<<2);

                                sprite[j].x += (sprite[OW].x-SX);
                                sprite[j].y += (sprite[OW].y-SY);
                                sprite[j].z = sector[sprite[OW].sectnum].ceilingz;


                                Bmemcpy(&actor[j].bposx, &sprite[j], sizeof(vec3_t));

                                changespritesect(j,sprite[OW].sectnum);

                                break;
                            case 2:
                                actor[j].lasttransport = totalclock + (TICSPERFRAME<<2);
                                sprite[j].x += (sprite[OW].x-SX);
                                sprite[j].y += (sprite[OW].y-SY);
                                sprite[j].z = sector[sprite[OW].sectnum].floorz;

                                Bmemcpy(&actor[j].bposx, &sprite[j], sizeof(vec3_t));

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

static int16_t A_FindLocator(int32_t n,int32_t sn)
{
    int32_t i = headspritestat[STAT_LOCATOR];

    while (i >= 0)
    {
        if ((sn == -1 || sn == SECT) && n == SLT)
            return i;
        i = nextspritestat[i];
    }
    return -1;
}

ACTOR_STATIC void G_MoveActors(void)
{
    int32_t x, m, l;
    intptr_t *t;
    int32_t a, j, nexti, nextj, sect, p, switchpicnum, k;
    spritetype *s;
    int32_t i = headspritestat[STAT_ACTOR];

    while (i >= 0)
    {
        nexti = nextspritestat[i];

        s = &sprite[i];

        sect = s->sectnum;

        if (s->xrepeat == 0 || sect < 0 || sect >= MAXSECTORS)
            KILLIT(i);

        t = &actor[i].t_data[0];

        Bmemcpy(&actor[i].bposx, s, sizeof(vec3_t));

        switchpicnum=s->picnum;
        if ((s->picnum > GREENSLIME)&&(s->picnum <= GREENSLIME+7))
        {
            switchpicnum = GREENSLIME;
        }
        switch (DynamicTileMap[switchpicnum])
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
                j = A_IncurDamage(i);
                if (j >= 0)
                {
                    s->cstat = 32+128;
                    k = 1;

                    j = headspritestat[STAT_ACTOR];
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
                        G_OperateActivators(s->lotag,-1);
                        G_OperateForceFields(i,s->lotag);
                        G_OperateMasterSwitches(s->lotag);
                    }
                }
            }
            goto BOLT;

        case RESPAWNMARKERRED__STATIC:
        case RESPAWNMARKERYELLOW__STATIC:
        case RESPAWNMARKERGREEN__STATIC:
            if (++T1 > g_itemRespawnTime)
                KILLIT(i);

            if (T1 >= (g_itemRespawnTime>>1) && T1 < ((g_itemRespawnTime>>1)+(g_itemRespawnTime>>2)))
                PN = RESPAWNMARKERYELLOW;
            else if (T1 > ((g_itemRespawnTime>>1)+(g_itemRespawnTime>>2)))
                PN = RESPAWNMARKERGREEN;

            A_Fall(i);
            break;

        case HELECOPT__STATIC:
        case DUKECAR__STATIC:

            s->z += s->zvel;
            t[0]++;

            if (t[0] == 4) A_PlaySound(WAR_AMBIENCE2,i);

            if (t[0] > (GAMETICSPERSEC*8))
            {
                S_PlaySound(RPG_EXPLODE);
                for (j=0; j<32; j++) RANDOMSCRAP;
                g_earthquakeTime = 16;
                KILLIT(i);
            }
            else if ((t[0]&3) == 0)
                A_Spawn(i,EXPLOSION2);
            A_SetSprite(i,CLIPMASK0);
            break;
        case RAT__STATIC:
            A_Fall(i);
            IFMOVING
            {
                if ((krand()&255) < 3) A_PlaySound(RATTY,i);
                s->ang += (krand()&31)-15+(sintable[(t[0]<<8)&2047]>>11);
            }
            else
            {
                T1++;
                if (T1 > 1)
                {
                    KILLIT(i);
                }
                else s->ang = (krand()&2047);
            }
            if (s->xvel < 128)
                s->xvel+=2;
            s->ang += (krand()&3)-6;
            break;
        case QUEBALL__STATIC:
        case STRIPEBALL__STATIC:
            if (s->xvel)
            {
                j = headspritestat[STAT_DEFAULT];
                while (j >= 0)
                {
                    nextj = nextspritestat[j];
                    if (sprite[j].picnum == POCKET && ldist(&sprite[j],s) < 52) KILLIT(i);
                    j = nextj;
                }

                j = clipmove((vec3_t *)s,&s->sectnum,
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
                        A_DamageObject(i,j);
                    }
                }
                s->xvel --;
                if (s->xvel < 0) s->xvel = 0;
                if (s->picnum == STRIPEBALL)
                {
                    s->cstat = 257;
                    s->cstat |= (4 & s->xvel) | (8 & s->xvel);
                }
            }
            else
            {
                p = A_FindPlayer(s,&x);

                if (x < 1596)
                {
                    //                        if(s->pal == 12)
                    {
                        j = G_GetAngleDelta(g_player[p].ps->ang,getangle(s->x-g_player[p].ps->pos.x,s->y-g_player[p].ps->pos.y));
                        if (j > -64 && j < 64 && TEST_SYNC_KEY(g_player[p].sync->bits, SK_OPEN))
                            if (g_player[p].ps->toggle_key_flag == 1)
                            {
                                a = headspritestat[STAT_ACTOR];
                                while (a >= 0)
                                {
                                    if (sprite[a].picnum == QUEBALL || sprite[a].picnum == STRIPEBALL)
                                    {
                                        j = G_GetAngleDelta(g_player[p].ps->ang,getangle(sprite[a].x-g_player[p].ps->pos.x,sprite[a].y-g_player[p].ps->pos.y));
                                        if (j > -64 && j < 64)
                                        {
                                            A_FindPlayer(&sprite[a],&l);
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
                    s->ang = getangle(s->x-g_player[p].ps->pos.x,s->y-g_player[p].ps->pos.y);
                    s->xvel = 48;
                }
            }

            break;
        case FORCESPHERE__STATIC:

            if (s->yvel == 0)
            {
                s->yvel = 1;

                for (l=512; l<(2048-512); l+= 128)
                    for (j=0; j<2048; j += 128)
                    {
                        k = A_Spawn(i,FORCESPHERE);
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
                j = headspritestat[STAT_MISC];
                while (j >= 0)
                {
                    if (sprite[j].owner == i && sprite[j].picnum == FORCESPHERE)
                        actor[j].t_data[1] = 1+(krand()&63);
                    j = nextspritestat[j];
                }
                t[3] = 64;
            }

            goto BOLT;

        case RECON__STATIC:

            A_GetZLimits(i);

            if (sector[s->sectnum].ceilingstat&1)
                s->shade += (sector[s->sectnum].ceilingshade-s->shade)>>1;
            else s->shade += (sector[s->sectnum].floorshade-s->shade)>>1;

            if (s->z < sector[sect].ceilingz+(32<<8))
                s->z = sector[sect].ceilingz+(32<<8);

#ifdef POLYMER
            /*
                        gamelights[gamelightcount&(PR_MAXLIGHTS-1)].sector = s->sectnum;
                        gamelights[gamelightcount&(PR_MAXLIGHTS-1)].x = s->x;
                        gamelights[gamelightcount&(PR_MAXLIGHTS-1)].y = s->y;
                        gamelights[gamelightcount&(PR_MAXLIGHTS-1)].z = s->z + 10248;
                        gamelights[gamelightcount&(PR_MAXLIGHTS-1)].range = 8192;

                        gamelights[gamelightcount&(PR_MAXLIGHTS-1)].angle = s->ang;
                        gamelights[gamelightcount&(PR_MAXLIGHTS-1)].horiz = 100;
                        gamelights[gamelightcount&(PR_MAXLIGHTS-1)].radius = 256;
                        gamelights[gamelightcount&(PR_MAXLIGHTS-1)].faderadius = 200;

                        gamelights[gamelightcount&(PR_MAXLIGHTS-1)].color[0] = 255;
                        gamelights[gamelightcount&(PR_MAXLIGHTS-1)].color[1] = 255;
                        gamelights[gamelightcount&(PR_MAXLIGHTS-1)].color[2] = 255;

                        gamelights[gamelightcount&(PR_MAXLIGHTS-1)].priority = PR_LIGHT_PRIO_MAX_GAME;

                        if (gamelightcount < PR_MAXLIGHTS)
                            gamelightcount++;
            */
#endif // POLYMER

            if (!g_netServer && ud.multimode < 2)
            {
                if (g_noEnemies == 1)
                {
                    s->cstat = (int16_t)32768;
                    goto BOLT;
                }
                else if (g_noEnemies == 2) s->cstat = 257;
            }
            IFHIT
            {
                if (s->extra < 0 && t[0] != -1)
                {
                    t[0] = -1;
                    s->extra = 0;
                }
                A_PlaySound(RECO_PAIN,i);
                RANDOMSCRAP;
            }

            if (t[0] == -1)
            {
                s->z += 1024;
                t[2]++;
                if ((t[2]&3) == 0) A_Spawn(i,EXPLOSION2);
                A_GetZLimits(i);
                s->ang += 96;
                s->xvel = 128;
                j = A_SetSprite(i,CLIPMASK0);
                if (j != 1 || s->z > actor[i].floorz)
                {
                    for (l=0; l<16; l++)
                        RANDOMSCRAP;
                    j = A_Spawn(i,EXPLOSION2);
                    A_PlaySound(LASERTRIP_EXPLODE,j);
                    A_Spawn(i,PIGCOP);
                    g_player[myconnectindex].ps->actors_killed++;
                    KILLIT(i);
                }
                goto BOLT;
            }
            else
            {
                if (s->z > actor[i].floorz-(48<<8))
                    s->z = actor[i].floorz-(48<<8);
            }

            p = A_FindPlayer(s,&x);
            j = s->owner;

            // 3 = findplayerz, 4 = shoot

            if (t[0] >= 4)
            {
                t[2]++;
                if ((t[2]&15) == 0)
                {
                    a = s->ang;
                    s->ang = actor[i].tempang;
                    A_PlaySound(RECO_ATTACK,i);
                    A_Shoot(i,FIRELASER);
                    s->ang = a;
                }
                if (t[2] > (GAMETICSPERSEC*3) || !cansee(s->x,s->y,s->z-(16<<8),s->sectnum, g_player[p].ps->pos.x,g_player[p].ps->pos.y,g_player[p].ps->pos.z,g_player[p].ps->cursectnum))
                {
                    t[0] = 0;
                    t[2] = 0;
                }
                else actor[i].tempang +=
                        G_GetAngleDelta(actor[i].tempang,getangle(g_player[p].ps->pos.x-s->x,g_player[p].ps->pos.y-s->y))/3;
            }
            else if (t[0] == 2 || t[0] == 3)
            {
                t[3] = 0;
                if (s->xvel > 0) s->xvel -= 16;
                else s->xvel = 0;

                if (t[0] == 2)
                {
                    l = g_player[p].ps->pos.z-s->z;
                    if (klabs(l) < (48<<8)) t[0] = 3;
                    else s->z += ksgn(g_player[p].ps->pos.z-s->z)<<10;
                }
                else
                {
                    t[2]++;
                    if (t[2] > (GAMETICSPERSEC*3) || !cansee(s->x,s->y,s->z-(16<<8),s->sectnum, g_player[p].ps->pos.x,g_player[p].ps->pos.y,g_player[p].ps->pos.z,g_player[p].ps->cursectnum))
                    {
                        t[0] = 1;
                        t[2] = 0;
                    }
                    else if ((t[2]&15) == 0)
                    {
                        A_PlaySound(RECO_ATTACK,i);
                        A_Shoot(i,FIRELASER);
                    }
                }
                s->ang += G_GetAngleDelta(s->ang,getangle(g_player[p].ps->pos.x-s->x,g_player[p].ps->pos.y-s->y))>>2;
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

                    if (x < 6144 && t[0] < 2 && t[2] > (GAMETICSPERSEC*4))
                    {
                        t[0] = 2+(krand()&2);
                        t[2] = 0;
                        actor[i].tempang = s->ang;
                    }
                }

                if (t[0] == 0 || t[0] == 5)
                {
                    if (t[0] == 0)
                        t[0] = 1;
                    else t[0] = 4;
                    j = s->owner = A_FindLocator(s->hitag,-1);
                    if (j == -1)
                    {
                        s->hitag = j = actor[i].t_data[5];
                        s->owner = A_FindLocator(j,-1);
                        j = s->owner;
                        if (j == -1) KILLIT(i);
                    }
                    else s->hitag++;
                }

                t[3] = G_GetAngleDelta(s->ang,a);
                s->ang += t[3]>>3;

                if ((s->z - sprite[j].z) < -512)
                    s->z += 512;
                else if ((s->z - sprite[j].z) > 512)
                    s->z -= 512;
                else s->z = sprite[j].z;
            }

            if (!A_CheckSoundPlaying(i,RECO_ROAM))
                A_PlaySound(RECO_ROAM,i);

            A_SetSprite(i,CLIPMASK0);

            goto BOLT;

        case OOZ__STATIC:
        case OOZ2__STATIC:

            A_GetZLimits(i);

            j = (actor[i].floorz-actor[i].ceilingz)>>9;
            if (j > 255) j = 255;

            x = 25-(j>>1);
            if (x < 8) x = 8;
            else if (x > 48) x = 48;

            s->yrepeat = j;
            s->xrepeat = x;
            s->z = actor[i].floorz;

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
            if (g_netServer && (!g_netServer && ud.multimode < 2))
            {
                if (g_noEnemies == 1)
                {
                    s->cstat = (int16_t)32768;
                    goto BOLT;
                }
                else if (g_noEnemies == 2) s->cstat = 257;
            }
            // #endif

            t[1]+=128;

            if (sector[sect].floorstat&1)
                KILLIT(i);

            p = A_FindPlayer(s,&x);

            if (x > 20480)
            {
                actor[i].timetosleep++;
                if (actor[i].timetosleep > SLEEPTIME)
                {
                    actor[i].timetosleep = 0;
                    changespritestat(i, STAT_ZOMBIEACTOR);
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
                A_Fall(i);
                s->cstat = 257;
                s->picnum = GREENSLIME+2;
                s->extra = 1;
                s->pal = 1;
                IFHIT
                {
                    if (j == FREEZEBLAST) goto BOLT;
                    for (j=16; j >= 0 ; j--)
                    {
                        k = A_InsertSprite(SECT,SX,SY,SZ,GLASSPIECES+(j%3),-32,36,36,krand()&2047,32+(krand()&63),1024-(krand()&1023),i,5);
                        sprite[k].pal = 1;
                    }
                    A_PlaySound(GLASS_BREAKING,i);
                    KILLIT(i);
                }
                else if (x < 1024 && g_player[p].ps->quick_kick == 0)
                {
                    j = G_GetAngleDelta(g_player[p].ps->ang,getangle(SX-g_player[p].ps->pos.x,SY-g_player[p].ps->pos.y));
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

                setsprite(i,(vec3_t *)s);

                s->ang = g_player[p].ps->ang;

                if ((TEST_SYNC_KEY(g_player[p].sync->bits, SK_FIRE) || (g_player[p].ps->quick_kick > 0)) && sprite[g_player[p].ps->i].extra > 0)
                    if (g_player[p].ps->quick_kick > 0 || (g_player[p].ps->curr_weapon != HANDREMOTE_WEAPON && g_player[p].ps->curr_weapon != HANDBOMB_WEAPON && g_player[p].ps->curr_weapon != TRIPBOMB_WEAPON && g_player[p].ps->ammo_amount[g_player[p].ps->curr_weapon] >= 0))
                    {
                        for (x=0; x<8; x++)
                        {
                            j = A_InsertSprite(sect,s->x,s->y,s->z-(8<<8),SCRAP3+(krand()&3),-8,48,48,krand()&2047,(krand()&63)+64,-(krand()&4095)-(s->zvel>>2),i,5);
                            sprite[j].pal = 6;
                        }

                        A_PlaySound(SLIM_DYING,i);
                        A_PlaySound(SQUISHED,i);
                        if ((krand()&255) < 32)
                        {
                            j = A_Spawn(i,BLOODPOOL);
                            sprite[j].pal = 0;
                        }
                        g_player[p].ps->actors_killed ++;
                        t[0] = -3;
                        if (g_player[p].ps->somethingonplayer == i)
                            g_player[p].ps->somethingonplayer = -1;
                        KILLIT(i);
                    }

                s->z = g_player[p].ps->pos.z+g_player[p].ps->pyoff-t[2]+(8<<8);

                s->z += (100-g_player[p].ps->horiz)<<4;

                if (t[2] > 512)
                    t[2] -= 128;

                if (t[2] < 348)
                    t[2] += 128;

                if (g_player[p].ps->newowner >= 0)
                {
                    g_player[p].ps->newowner = -1;
                    g_player[p].ps->pos.x = g_player[p].ps->opos.x;
                    g_player[p].ps->pos.y = g_player[p].ps->opos.y;
                    g_player[p].ps->pos.z = g_player[p].ps->opos.z;
                    g_player[p].ps->ang = g_player[p].ps->oang;

                    updatesector(g_player[p].ps->pos.x,g_player[p].ps->pos.y,&g_player[p].ps->cursectnum);
                    P_UpdateScreenPal(g_player[p].ps);

                    j = headspritestat[STAT_ACTOR];
                    while (j >= 0)
                    {
                        if (sprite[j].picnum==CAMERA1) sprite[j].yvel = 0;
                        j = nextspritestat[j];
                    }
                }

                if (t[3]>0)
                {
                    static char frames[] = {5,5,6,6,7,7,6,5};

                    s->picnum = GREENSLIME+frames[t[3]];

                    if (t[3] == 5)
                    {
                        sprite[g_player[p].ps->i].extra += -(5+(krand()&3));
                        A_PlaySound(SLIM_ATTACK,i);
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

                s->x = g_player[p].ps->pos.x + (sintable[(g_player[p].ps->ang+512)&2047]>>7);
                s->y = g_player[p].ps->pos.y + (sintable[g_player[p].ps->ang&2047]>>7);

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
                A_PlaySound(SLIM_DYING,i);

                g_player[p].ps->actors_killed ++;
                if (g_player[p].ps->somethingonplayer == i)
                    g_player[p].ps->somethingonplayer = -1;

                if (j == FREEZEBLAST)
                {
                    A_PlaySound(SOMETHINGFROZE,i);
                    t[0] = -5 ;
                    t[3] = 0 ;
                    goto BOLT;
                }

                if ((krand()&255) < 32)
                {
                    j = A_Spawn(i,BLOODPOOL);
                    sprite[j].pal = 0;
                }

                for (x=0; x<8; x++)
                {
                    j = A_InsertSprite(sect,s->x,s->y,s->z-(8<<8),SCRAP3+(krand()&3),-8,48,48,krand()&2047,(krand()&63)+64,-(krand()&4095)-(s->zvel>>2),i,5);
                    sprite[j].pal = 6;
                }
                t[0] = -3;
                KILLIT(i);
            }
            // All weap
            if (t[0] == -1) //Shrinking down
            {
                A_Fall(i);

                s->cstat &= 65535-8;
                s->picnum = GREENSLIME+4;

                //                    if(s->yrepeat > 62)
                //                      A_DoGuts(s,JIBS6,5,myconnectindex);

                if (s->xrepeat > 32) s->xrepeat -= krand()&7;
                if (s->yrepeat > 16) s->yrepeat -= krand()&7;
                else
                {
                    s->xrepeat = 40;
                    s->yrepeat = 16;
                    t[5] = -1;
                    t[0] = 0;
                }

                goto BOLT;
            }
            else if (t[0] != -2) A_GetZLimits(i);

            if (t[0] == -2) //On top of somebody
            {
                A_Fall(i);
                sprite[t[5]].xvel = 0;

                l = sprite[t[5]].ang;

                s->z = sprite[t[5]].z;
                s->x = sprite[t[5]].x+(sintable[(l+512)&2047]>>11);
                s->y = sprite[t[5]].y+(sintable[l&2047]>>11);

                s->picnum =  GREENSLIME+2+(g_globalRandom&1);

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
                    switch (DynamicTileMap[sprite[j].picnum])
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

                if ((krand()&511) == 0)
                    A_PlaySound(SLIM_ROAM,i);

                if (t[0]==2)
                {
                    s->zvel = 0;
                    s->cstat &= (65535-8);

                    if ((sector[sect].ceilingstat&1) || (actor[i].ceilingz+6144) < s->z)
                    {
                        s->z += 2048;
                        t[0] = 3;
                        goto BOLT;
                    }
                }
                else
                {
                    s->cstat |= 8;
                    A_Fall(i);
                }

                if (everyothertime&1) A_SetSprite(i,CLIPMASK0);

                if (s->xvel > 96)
                {
                    s->xvel -= 2;
                    goto BOLT;
                }
                else
                {
                    if (s->xvel < 32) s->xvel += 4;
                    s->xvel = 64 - (sintable[(t[1]+512)&2047]>>9);

                    s->ang += G_GetAngleDelta(s->ang,
                                              getangle(g_player[p].ps->pos.x-s->x,g_player[p].ps->pos.y-s->y))>>3;
                    // TJR
                }

                s->xrepeat = 36 + (sintable[(t[1]+512)&2047]>>11);
                s->yrepeat = 16 + (sintable[t[1]&2047]>>13);

                if (rnd(4) && (sector[sect].ceilingstat&1) == 0 &&
                        klabs(actor[i].floorz-actor[i].ceilingz)
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
                if (s->z < actor[i].ceilingz+4096)
                {
                    s->z = actor[i].ceilingz+4096;
                    s->xvel = 0;
                    t[0] = 2;
                }
            }

            if (t[0]==3)
            {
                s->picnum = GREENSLIME+1;

                A_Fall(i);

                if (s->z > actor[i].floorz-(8<<8))
                {
                    s->yrepeat-=4;
                    s->xrepeat+=2;
                }
                else
                {
                    if (s->yrepeat < (40-4)) s->yrepeat+=8;
                    if (s->xrepeat > 8) s->xrepeat-=4;
                }

                if (s->z > actor[i].floorz-2048)
                {
                    s->z = actor[i].floorz-2048;
                    t[0] = 0;
                    s->xvel = 0;
                }
            }
            goto BOLT;

        case BOUNCEMINE__STATIC:
        case MORTER__STATIC:
            j = A_Spawn(i,(PLUTOPAK?FRAMEEFFECT1:FRAMEEFFECT1_13));
            actor[j].t_data[0] = 3;

        case HEAVYHBOMB__STATIC:

            if ((s->cstat&32768))
            {
                t[2]--;
                if (t[2] <= 0)
                {
                    A_PlaySound(TELEPORTER,i);
                    A_Spawn(i,TRANSPORTERSTAR);
                    s->cstat = 257;
                }
                goto BOLT;
            }

            p = A_FindPlayer(s,&x);

            if (x < 1220) s->cstat &= ~257;
            else s->cstat |= 257;

            if (t[3] == 0)
            {
                j = A_IncurDamage(i);
                if (j >= 0)
                {
                    t[3] = 1;
                    t[2] = 0;
                    l = 0;
                    s->xvel = 0;
                    goto DETONATEB;
                }
            }

            if (s->picnum != BOUNCEMINE)
            {
                A_Fall(i);

                if ((sector[sect].lotag != 1 || actor[i].floorz != sector[sect].floorz) && s->z >= actor[i].floorz-(ZOFFSET) && s->yvel < 3)
                {
                    if (s->yvel > 0 || (s->yvel == 0 && actor[i].floorz == sector[sect].floorz))
                        A_PlaySound(PIPEBOMB_BOUNCE,i);
                    s->zvel = -((4-s->yvel)<<8);
                    if (sector[s->sectnum].lotag== 2)
                        s->zvel >>= 2;
                    s->yvel++;
                }
                if (s->z < actor[i].ceilingz)   // && sector[sect].lotag != 2 )
                {
                    s->z = actor[i].ceilingz+(3<<8);
                    s->zvel = 0;
                }
            }

            {
                vec3_t tmpvect;

                tmpvect.x = (s->xvel*(sintable[(s->ang+512)&2047]))>>14;
                tmpvect.y = (s->xvel*(sintable[s->ang&2047]))>>14;
                tmpvect.z = s->zvel;
                j = A_MoveSprite(i,&tmpvect,CLIPMASK0);
            }

            actor[i].movflag = j;

            if (sector[SECT].lotag == 1 && s->zvel == 0 && actor[i].floorz == sector[sect].floorz)
            {
                s->z += (32<<8);
                if (t[5] == 0)
                {
                    t[5] = 1;
                    A_Spawn(i,WATERSPLASH2);
                }
            }
            else t[5] = 0;

            if (t[3] == 0 && (s->picnum == BOUNCEMINE || s->picnum == MORTER) && (j || x < 844))
            {
                t[3] = 1;
                t[2] = 0;
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
                vec3_t davect;

                j &= (MAXWALLS-1);

                Bmemcpy(&davect, s, sizeof(vec3_t));
                A_DamageWall(i,j,&davect,s->picnum);

                k = getangle(
                        wall[wall[j].point2].x-wall[j].x,
                        wall[wall[j].point2].y-wall[j].y);

                s->ang = ((k<<1) - s->ang)&2047;
                s->xvel >>= 1;
            }

            //      int32_t lPipeBombControl=Gv_GetVarByLabel("PIPEBOMB_CONTROL", PIPEBOMB_REMOTE, -1, -1);

DETONATEB:

            //  if(lPipeBombControl & PIPEBOMB_TIMER)
            //       {

            if (s->picnum == HEAVYHBOMB && t[6] == 1)
            {
                /*                if(s->extra >= 1)
                                {
                                    s->extra--;
                                }

                                if(s->extra <= 0)
                                    s->lotag=911;
                */

                if (t[7] > 0)
                    t[7]--;

                if (t[7] == 0)
                    t[6] = 3;
            }
            //      }

            if ((l >= 0 && g_player[l].ps->hbomb_on == 0 && t[6] == 2) || t[3] == 1)
                t[6] = 3;

            if (t[6] == 3)
            {
                t[2]++;

                if (t[2] == 2)
                {
                    int32_t j;

                    x = s->extra;
                    m = 0;
                    switch (DynamicTileMap[s->picnum])
                    {
                    case HEAVYHBOMB__STATIC:
                        m = g_pipebombBlastRadius;
                        break;
                    case MORTER__STATIC:
                        m = g_morterBlastRadius;
                        break;
                    case BOUNCEMINE__STATIC:
                        m = g_bouncemineBlastRadius;
                        break;
                    }

                    A_RadiusDamage(i, m,x>>2,x>>1,x-(x>>2),x);
                    j = A_Spawn(i,EXPLOSION2);
                    A_PlaySound(PIPEBOMB_EXPLODE,j);
                    if (s->zvel == 0)
                        A_Spawn(i,EXPLOSION2BOT);
                    for (x=0; x<8; x++)
                        RANDOMSCRAP;
                }

                if (s->yrepeat)
                {
                    s->yrepeat = 0;
                    goto BOLT;
                }

                if (t[2] > 20)
                {
                    if (s->owner != i || ud.respawn_items == 0)
                    {
                        KILLIT(i);
                    }
                    else
                    {
                        t[2] = g_itemRespawnTime;
                        A_Spawn(i,RESPAWNMARKERRED);
                        s->cstat = (int16_t) 32768;
                        s->yrepeat = 9;
                        goto BOLT;
                    }
                }
            }
            else if (s->picnum == HEAVYHBOMB && x < 788 && t[0] > 7 && s->xvel == 0)
                if (cansee(s->x,s->y,s->z-(8<<8),s->sectnum,g_player[p].ps->pos.x,g_player[p].ps->pos.y,g_player[p].ps->pos.z,g_player[p].ps->cursectnum))
                    if (g_player[p].ps->ammo_amount[HANDBOMB_WEAPON] < g_player[p].ps->max_ammo_amount[HANDBOMB_WEAPON])
                    {
                        if ((GametypeFlags[ud.coop] & GAMETYPE_WEAPSTAY) && s->owner == i)
                        {
                            for (j=0; j<g_player[p].ps->weapreccnt; j++)
                                if (g_player[p].ps->weaprecs[j] == s->picnum)
                                    goto BOLT;

                            if (g_player[p].ps->weapreccnt < MAX_WEAPONS)
                                g_player[p].ps->weaprecs[g_player[p].ps->weapreccnt++] = s->picnum;
                        }

                        P_AddAmmo(HANDBOMB_WEAPON,g_player[p].ps,1);
                        A_PlaySound(DUKE_GET,g_player[p].ps->i);

                        if ((g_player[p].ps->gotweapon & (1<<HANDBOMB_WEAPON)) == 0 || s->owner == g_player[p].ps->i)
                        {
                            /* P_AddWeapon(g_player[p].ps,HANDBOMB_WEAPON); */
                            if (!(g_player[p].ps->weaponswitch & 1) && *aplWeaponWorksLike[g_player[p].ps->curr_weapon] != HANDREMOTE_WEAPON)
                                P_AddWeaponNoSwitch(g_player[p].ps,HANDBOMB_WEAPON);
                            else P_AddWeapon(g_player[p].ps,HANDBOMB_WEAPON);
                        }

                        if (sprite[s->owner].picnum != APLAYER)
                        {
                            g_player[p].ps->pals.r = 0;
                            g_player[p].ps->pals.g = 32;
                            g_player[p].ps->pals.b = 0;
                            g_player[p].ps->pals.f = 32;
                        }

                        if (s->owner != i || ud.respawn_items == 0)
                        {
                            if (s->owner == i && (GametypeFlags[ud.coop] & GAMETYPE_WEAPSTAY))
                                goto BOLT;
                            KILLIT(i);
                        }
                        else
                        {
                            t[2] = g_itemRespawnTime;
                            A_Spawn(i,RESPAWNMARKERRED);
                            s->cstat = (int16_t) 32768;
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
                    switch (DynamicTileMap[sprite[j].picnum])
                    {
                    case SECTOREFFECTOR__STATIC:
                        if (sprite[j].lotag == 1)
                        {
                            sprite[j].lotag = (int16_t) 65535;
                            sprite[j].hitag = (int16_t) 65535;
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
                        sprite[j].cstat = (int16_t) 32768;
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

            p = A_FindPlayer(s,&x);

            t[2]++;
            if (t[2] == 4) t[2]=0;

            if (x < 4096)
            {
                if ((krand()&255) < 16)
                {
                    if (!A_CheckSoundPlaying(g_player[p].ps->i, DUKE_LONGTERM_PAIN))
                        A_PlaySound(DUKE_LONGTERM_PAIN,g_player[p].ps->i);

                    A_PlaySound(SHORT_CIRCUIT,i);

                    sprite[g_player[p].ps->i].extra --;
                    g_player[p].ps->pals.f = 32;
                    g_player[p].ps->pals.r = 32;
                    g_player[p].ps->pals.g = 0;
                    g_player[p].ps->pals.b = 0;
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
                s->z = sector[sect].floorz-(krand()%(sector[sect].floorz-sector[sect].ceilingz));

                switch (t[1])
                {
                case 3:
                    //Turn on all of those flashing sectoreffector.
                    A_RadiusDamage(i, 4096,
                                   g_impactDamage<<2,
                                   g_impactDamage<<2,
                                   g_impactDamage<<2,
                                   g_impactDamage<<2);
                    /*
                                                j = headspritestat[STAT_EFFECTOR];
                                                while(j>=0)
                                                {
                                                    if( sprite[j].lotag  == 3 )
                                                        Actor[j].t_data[4]=1;
                                                    else if(sprite[j].lotag == 12)
                                                    {
                                                        Actor[j].t_data[4] = 1;
                                                        sprite[j].lotag = 3;
                                                        sprite[j].owner = 0;
                                                        Actor[j].t_data[0] = s->shade;
                                                    }
                                                    j = nextspritestat[j];
                                                }
                    */
                    j = headspritestat[STAT_STANDABLE];
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
                for (x=0; x<16; x++)
                    RANDOMSCRAP;

                s->z = t[4];
                t[4] = 0;

            }
            else
            {
                IFHIT
                {
                    for (x=0; x<32; x++)
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
                if (g_damageCameras)
                {
                    IFHIT
                    {
                        t[0] = 1; // static
                        s->cstat = (int16_t)32768;
                        for (x=0; x<5; x++) RANDOMSCRAP;
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

        if (!g_netServer && (!g_netServer && ud.multimode < 2) && A_CheckEnemySprite(s))
        {
            if (g_noEnemies == 1)
            {
                s->cstat = (int16_t)32768;
                goto BOLT;
            }
            else if (g_noEnemies == 2)
            {
                s->cstat = 0;
                if (s->extra)
                    s->cstat = 257;
            }
        }

        if (!actorscrptr[sprite[i].picnum])
            goto BOLT;
        p = A_FindPlayer(s,&x);
        A_Execute(i,p,x);
BOLT:
        i = nexti;
    }

}

ACTOR_STATIC void G_MoveMisc(void)  // STATNUM 5
{
    int16_t i, j, nexti, sect, p;
    int32_t l, x;
    intptr_t *t;
    spritetype *s;
    int32_t switchpicnum;

    i = headspritestat[STAT_MISC];
    while (i >= 0)
    {
        nexti = nextspritestat[i];

        t = &actor[i].t_data[0];
        s = &sprite[i];
        sect = s->sectnum;

        if (sect < 0 || s->xrepeat == 0) KILLIT(i);

        Bmemcpy(&actor[i].bposx, s, sizeof(vec3_t));

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
            actor[i].floorz = s->z = getflorzofslope(s->sectnum,s->x,s->y);
        else switch (DynamicTileMap[switchpicnum])
            {
            case APLAYER__STATIC:
                s->cstat = 32768;
                goto BOLT;
            case NEON1__STATIC:
            case NEON2__STATIC:
            case NEON3__STATIC:
            case NEON4__STATIC:
            case NEON5__STATIC:
            case NEON6__STATIC:

                if ((g_globalRandom/(s->lotag+1)&31) > 4) s->shade = -127;
                else s->shade = 127;
                goto BOLT;

            case BLOODSPLAT1__STATIC:
            case BLOODSPLAT2__STATIC:
            case BLOODSPLAT3__STATIC:
            case BLOODSPLAT4__STATIC:

                if (t[0] == 7*GAMETICSPERSEC) goto BOLT;
                s->z += 16+(krand()&15);
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
                    if (g_player[sprite[s->owner].yvel].ps->fist_incs == GAMETICSPERSEC)
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
                if (actor[s->owner].t_data[1] == 0)
                {
                    if (t[0] < 64)
                    {
                        t[0]++;
                        l += 3;
                    }
                }
                else if (t[0] > 64)
                {
                    t[0]--;
                    l -= 3;
                }

                s->x = sprite[s->owner].x;
                s->y = sprite[s->owner].y;
                s->z = sprite[s->owner].z;
                s->ang += actor[s->owner].t_data[0];

                if (l > 64) l = 64;
                else if (l < 1) l = 1;

                s->xrepeat = l;
                s->yrepeat = l;
                s->shade = (l>>1)-48;

                for (j=t[0]; j > 0; j--)
                    A_SetSprite(i,CLIPMASK0);
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
                    if (!S_CheckSoundPlaying(i,ITEM_SPLASH))
                        A_PlaySound(ITEM_SPLASH,i);
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

                p = A_FindPlayer(s,&x);
                if (x < 512)
                {
                    g_player[p].ps->pals.f = 32;
                    g_player[p].ps->pals.r = 32;
                    g_player[p].ps->pals.g = 0;
                    g_player[p].ps->pals.b = 0;
                    sprite[g_player[p].ps->i].extra -= 4;
                }

            case FIRELASER__STATIC:
                if (s->extra != 5)
                    s->extra = 5;
                else KILLIT(i);
                break;
            case TONGUE__STATIC:
                KILLIT(i);

            case MONEY__STATIC:
            case MAIL__STATIC:
            case PAPER__STATIC:

                s->xvel = (krand()&7)+(sintable[T1&2047]>>9);
                T1 += (krand()&63);
                if ((T1&2047) > 512 && (T1&2047) < 1596)
                {
                    if (sector[sect].lotag == 2)
                    {
                        if (s->zvel < 64)
                            s->zvel += (g_spriteGravity>>5)+(krand()&7);
                    }
                    else if (s->zvel < 144)
                        s->zvel += (g_spriteGravity>>5)+(krand()&7);
                }

                A_SetSprite(i,CLIPMASK0);

                if ((krand()&3) == 0)
                    setsprite(i,(vec3_t *)s);

                if (s->sectnum == -1) KILLIT(i);
                l = getflorzofslope(s->sectnum,s->x,s->y);

                if (s->z > l)
                {
                    s->z = l;

                    A_AddToDeleteQueue(i);
                    PN ++;

                    j = headspritestat[STAT_MISC];
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

                if (++t[5] == (30*10))
                    KILLIT(i);

                if (s->zvel > 1024 && s->zvel < 1280)
                {
                    setsprite(i,(vec3_t *)s);
                    sect = s->sectnum;
                }

                getzsofslope(sect,s->x,s->y,&x,&l);
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
                        else s->zvel += g_spriteGravity-50;
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
                    else A_AddToDeleteQueue(i);
                }

                A_Fall(i);

                p = A_FindPlayer(s,&x);

                s->z = actor[i].floorz-(ZOFFSET);

                if (t[2] < 32)
                {
                    t[2]++;
                    if (actor[i].picnum == TIRE)
                    {
                        if (s->xrepeat < 64 && s->yrepeat < 64)
                        {
                            s->xrepeat += krand()&3;
                            s->yrepeat += krand()&3;
                        }
                    }
                    else
                    {
                        if (s->xrepeat < 32 && s->yrepeat < 32)
                        {
                            s->xrepeat += krand()&3;
                            s->yrepeat += krand()&3;
                        }
                    }
                }

                if (x < 844 && s->xrepeat > 6 && s->yrepeat > 6)
                {
                    if (s->pal == 0 && (krand()&255) < 16 && s->picnum != PUKE)
                    {
                        if (g_player[p].ps->inv_amount[GET_BOOTS] > 0)
                            g_player[p].ps->inv_amount[GET_BOOTS]--;
                        else
                        {
                            if (!A_CheckSoundPlaying(g_player[p].ps->i,DUKE_LONGTERM_PAIN))
                                A_PlaySound(DUKE_LONGTERM_PAIN,g_player[p].ps->i);
                            sprite[g_player[p].ps->i].extra --;
                            g_player[p].ps->pals.f = 32;
                            g_player[p].ps->pals.r = 16;
                            g_player[p].ps->pals.g = 0;
                            g_player[p].ps->pals.b = 0;
                        }
                    }

                    if (t[1] == 1) goto BOLT;
                    t[1] = 1;

                    if (actor[i].picnum == TIRE)
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
            {
                if (!actorscrptr[sprite[i].picnum])
                    goto BOLT;
                p = A_FindPlayer(s,&x);
                A_Execute(i,p,x);
                goto BOLT;
            }

            case SHELL__STATIC:
            case SHOTGUNSHELL__STATIC:

                A_SetSprite(i,CLIPMASK0);

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
                    if (s->zvel < 128) s->zvel += (g_spriteGravity/13); // 8
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
                    if (s->zvel < 512) s->zvel += (g_spriteGravity/3); // 52;
                    if (s->xvel > 0)
                        s->xvel --;
                    //                else KILLIT(i);
                }

                goto BOLT;

            case GLASSPIECES__STATIC:
                //        case GLASSPIECES+1:
                //        case GLASSPIECES+2:

                A_Fall(i);

                if (s->zvel > 4096) s->zvel = 4096;
                if (sect < 0) KILLIT(i);

                if (s->z == actor[i].floorz-(ZOFFSET) && t[0] < 3)
                {
                    s->zvel = -((3-t[0])<<8)-(krand()&511);
                    if (sector[sect].lotag == 2)
                        s->zvel >>= 1;
                    s->xrepeat >>= 1;
                    s->yrepeat >>= 1;
                    if (rnd(96))
                        setsprite(i,(vec3_t *)s);
                    t[0]++;//Number of bounces
                }
                else if (t[0] == 3) KILLIT(i);

                if (s->xvel > 0)
                {
                    s->xvel -= 2;
                    s->cstat = ((s->xvel&3)<<2);
                }
                else s->xvel = 0;

                A_SetSprite(i,CLIPMASK0);

                goto BOLT;
            }

        IFWITHIN(SCRAP6,SCRAP5+3)
        {
            if (s->xvel > 0)
                s->xvel--;
            else s->xvel = 0;

            if (s->zvel > 1024 && s->zvel < 1280)
            {
                setsprite(i,(vec3_t *)s);
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
                if (s->zvel < 4096) s->zvel += g_spriteGravity-50;
                s->x += (s->xvel*sintable[(s->ang+512)&2047])>>14;
                s->y += (s->xvel*sintable[s->ang&2047])>>14;
                s->z += s->zvel;
            }
            else
            {
                if (s->picnum == SCRAP1 && s->yvel > 0)
                {
                    j = A_Spawn(i,s->yvel);
                    setsprite(j,(vec3_t *)s);
                    A_GetZLimits(j);
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

ACTOR_STATIC void G_MoveEffectors(void)   //STATNUM 3
{
    int32_t q=0,  m, x, st, j;
    intptr_t *t,l;
    int32_t i = headspritestat[STAT_EFFECTOR], nexti, nextk, p, sh, nextj;
    int16_t k;
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

        t = &actor[i].t_data[0];

        switch (st)
        {
        case 0:
        {
            int32_t zchange = 0;

            zchange = 0;

            j = s->owner;

            if (sprite[j].lotag == (int16_t) 65535)
                KILLIT(i);

            q = sc->extra>>3;
            l = 0;

            if (sc->lotag == 30)
            {
                q >>= 2;

                if (sprite[i].extra == 1)
                {
                    if (actor[i].tempang < 256)
                    {
                        actor[i].tempang += 4;
                        if (actor[i].tempang >= 256)
                            A_CallSound(s->sectnum,i);
                        if (s->clipdist) l = 1;
                        else l = -1;
                    }
                    else actor[i].tempang = 256;

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
                    if (actor[i].tempang > 0)
                    {
                        actor[i].tempang -= 4;
                        if (actor[i].tempang <= 0)
                            A_CallSound(s->sectnum,i);
                        if (s->clipdist) l = -1;
                        else l = 1;
                    }
                    else actor[i].tempang = 0;

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
                if (actor[j].t_data[0] == 0) break;
                if (actor[j].t_data[0] == 2) KILLIT(i);

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
                TRAVERSE_CONNECT(p)
                {
                    if (g_player[p].ps->cursectnum == s->sectnum && g_player[p].ps->on_ground == 1)
                    {

                        g_player[p].ps->ang += (l*q);
                        g_player[p].ps->ang &= 2047;

                        g_player[p].ps->pos.z += zchange;

                        rotatepoint(sprite[j].x,sprite[j].y,g_player[p].ps->pos.x,g_player[p].ps->pos.y,(q*l),&m,&x);

                        g_player[p].ps->bobposx += m-g_player[p].ps->pos.x;
                        g_player[p].ps->bobposy += x-g_player[p].ps->pos.y;

                        g_player[p].ps->pos.x = m;
                        g_player[p].ps->pos.y = x;

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
                    if (sprite[p].statnum != STAT_EFFECTOR && sprite[p].statnum != STAT_PROJECTILE)
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

            A_MoveSector(i);
        }

        break;
        case 1: //Nothing for now used as the pivot
            if (s->owner == -1) //Init
            {
                s->owner = i;

                j = headspritestat[STAT_EFFECTOR];
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

            j = headspritestat[STAT_EFFECTOR];
            while (j >= 0)
            {
                if ((sprite[j].lotag == 14) && (sh == sprite[j].hitag) && (actor[j].t_data[0] == t[0]))
                {
                    sprite[j].xvel = s->xvel;
                    //                        if( t[4] == 1 )
                    {
                        if (actor[j].t_data[5] == 0)
                            actor[j].t_data[5] = dist(&sprite[j],s);
                        x = ksgn(dist(&sprite[j],s)-actor[j].t_data[5]);
                        if (sprite[j].extra)
                            x = -x;
                        s->xvel += x;
                    }
                    actor[j].t_data[4] = t[4];
                }
                j = nextspritestat[j];
            }
            x = 0;


        case 14:
            if (s->owner==-1)
                s->owner = A_FindLocator((int16_t)t[3],(int16_t)t[0]);

            if (s->owner == -1)
            {
                Bsprintf(tempbuf,"Could not find any locators for SE# 6 and 14 with a hitag of %" PRIdPTR ".\n",t[3]);
                G_GameExit(tempbuf);
            }

            j = ldist(&sprite[s->owner],s);

            if (j < 1024L)
            {
                if (st==6)
                    if (sprite[s->owner].hitag&1)
                        t[4]=sc->extra; //Slow it down
                t[3]++;
                s->owner = A_FindLocator(t[3],t[0]);
                if (s->owner==-1)
                {
                    t[3]=0;
                    s->owner = A_FindLocator(0,t[0]);
                }
            }

            if (s->xvel)
            {
                x = getangle(sprite[s->owner].x-s->x,sprite[s->owner].y-s->y);
                q = G_GetAngleDelta(s->ang,x)>>3;

                t[2] += q;
                s->ang += q;

                if (s->xvel == sc->extra)
                {
                    if ((sc->floorstat&1) == 0 && (sc->ceilingstat&1) == 0)
                    {
                        if (!S_CheckSoundPlaying(i,actor[i].lastvx))
                            A_PlaySound(actor[i].lastvx,i);
                    }
                    else if (ud.monsters_off == 0 && sc->floorpal == 0 && (sc->floorstat&1) && rnd(8))
                    {
                        p = A_FindPlayer(s,&x);
                        if (x < 20480)
                        {
                            j = s->ang;
                            s->ang = getangle(s->x-g_player[p].ps->pos.x,s->y-g_player[p].ps->pos.y);
                            A_Shoot(i,RPG);
                            s->ang = j;
                        }
                    }
                }

                if (s->xvel <= 64 && (sc->floorstat&1) == 0 && (sc->ceilingstat&1) == 0)
                    S_StopEnvSound(actor[i].lastvx,i);

                if ((sc->floorz-sc->ceilingz) < (108<<8))
                {
                    if (ud.clipping == 0 && s->xvel >= 192)
                        TRAVERSE_CONNECT(p)
                        if (sprite[g_player[p].ps->i].extra > 0)
                        {
                            k = g_player[p].ps->cursectnum;
                            updatesector(g_player[p].ps->pos.x,g_player[p].ps->pos.y,&k);
                            if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && g_player[p].ps->cursectnum != s->sectnum))
                            {
                                g_player[p].ps->pos.x = s->x;
                                g_player[p].ps->pos.y = s->y;
                                g_player[p].ps->cursectnum = s->sectnum;

                                setsprite(g_player[p].ps->i,(vec3_t *)s);
                                P_QuickKill(g_player[p].ps);
                            }
                        }
                }

                m = (s->xvel*sintable[(s->ang+512)&2047])>>14;
                x = (s->xvel*sintable[s->ang&2047])>>14;

                TRAVERSE_CONNECT(p)
                if (sector[g_player[p].ps->cursectnum].lotag != 2)
                {
                    if (g_playerSpawnPoints[p].os == s->sectnum)
                    {
                        g_playerSpawnPoints[p].ox += m;
                        g_playerSpawnPoints[p].oy += x;
                    }

                    if (s->sectnum == sprite[g_player[p].ps->i].sectnum)
                    {
                        rotatepoint(s->x,s->y,g_player[p].ps->pos.x,g_player[p].ps->pos.y,q,&g_player[p].ps->pos.x,&g_player[p].ps->pos.y);

                        g_player[p].ps->pos.x += m;
                        g_player[p].ps->pos.y += x;

                        g_player[p].ps->bobposx += m;
                        g_player[p].ps->bobposy += x;

                        g_player[p].ps->ang += q;

                        if (g_netServer || numplayers > 1)
                        {
                            g_player[p].ps->opos.x = g_player[p].ps->pos.x;
                            g_player[p].ps->opos.y = g_player[p].ps->pos.y;
                        }
                        if (sprite[g_player[p].ps->i].extra <= 0)
                        {
                            sprite[g_player[p].ps->i].x = g_player[p].ps->pos.x;
                            sprite[g_player[p].ps->i].y = g_player[p].ps->pos.y;
                        }
                    }
                }
                j = headspritesect[s->sectnum];
                while (j >= 0)
                {
                    if (sprite[j].statnum != STAT_PLAYER && sector[sprite[j].sectnum].lotag != 2 &&
                            (sprite[j].picnum != SECTOREFFECTOR ||
                             (sprite[j].picnum == SECTOREFFECTOR && (sprite[j].lotag == 49||sprite[j].lotag == 50)))
                            && sprite[j].picnum != LOCATORS)
                    {
                        rotatepoint(s->x,s->y,sprite[j].x,sprite[j].y,q,&sprite[j].x,&sprite[j].y);

                        sprite[j].x+= m;
                        sprite[j].y+= x;

                        sprite[j].ang+=q;

                        if (g_netServer || numplayers > 1)
                        {
                            actor[j].bposx = sprite[j].x;
                            actor[j].bposy = sprite[j].y;
                        }
                    }
                    j = nextspritesect[j];
                }

                A_MoveSector(i);
                setsprite(i,(vec3_t *)s);

                if ((sc->floorz-sc->ceilingz) < (108<<8))
                {
                    if (ud.clipping == 0 && s->xvel >= 192)
                        TRAVERSE_CONNECT(p)
                        if (sprite[g_player[p].ps->i].extra > 0)
                        {
                            k = g_player[p].ps->cursectnum;
                            updatesector(g_player[p].ps->pos.x,g_player[p].ps->pos.y,&k);
                            if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && g_player[p].ps->cursectnum != s->sectnum))
                            {
                                g_player[p].ps->opos.x = g_player[p].ps->pos.x = s->x;
                                g_player[p].ps->opos.y = g_player[p].ps->pos.y = s->y;
                                g_player[p].ps->cursectnum = s->sectnum;

                                setsprite(g_player[p].ps->i,(vec3_t *)s);
                                P_QuickKill(g_player[p].ps);
                            }
                        }

                    j = headspritesect[sprite[OW].sectnum];
                    while (j >= 0)
                    {
                        l = nextspritesect[j];
                        if (sprite[j].statnum == STAT_ACTOR && A_CheckEnemySprite(&sprite[j]) &&
                                sprite[j].picnum != SECTOREFFECTOR && sprite[j].picnum != LOCATORS)
                        {
                            k = sprite[j].sectnum;
                            updatesector(sprite[j].x,sprite[j].y,&k);
                            if (sprite[j].extra >= 0 && k == s->sectnum)
                            {
                                A_DoGutsDir(j,JIBS6,72);
                                A_PlaySound(SQUISHED,i);
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
                s->owner = A_FindLocator(t[3],t[0]);
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
                            G_OperateActivators(s->hitag+(!t[3]),-1);
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
                        G_OperateActivators(s->hitag+(int16_t)t[3],-1);
                        s->owner = -1;
                        s->ang += 1024;
                        t[4] = 0;
                        G_OperateForceFields(i,s->hitag);

                        j = headspritesect[s->sectnum];
                        while (j >= 0)
                        {
                            if (sprite[j].picnum != SECTOREFFECTOR && sprite[j].picnum != LOCATORS)
                            {
                                actor[j].bposx = sprite[j].x;
                                actor[j].bposy = sprite[j].y;
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
                        TRAVERSE_CONNECT(p)
                        if (sprite[g_player[p].ps->i].extra > 0)
                        {
                            k = g_player[p].ps->cursectnum;
                            updatesector(g_player[p].ps->pos.x,g_player[p].ps->pos.y,&k);
                            if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && g_player[p].ps->cursectnum != s->sectnum))
                            {
                                g_player[p].ps->pos.x = s->x;
                                g_player[p].ps->pos.y = s->y;
                                g_player[p].ps->cursectnum = s->sectnum;

                                setsprite(g_player[p].ps->i,(vec3_t *)s);
                                P_QuickKill(g_player[p].ps);
                            }
                        }

                TRAVERSE_CONNECT(p)
                {
                    if (sprite[g_player[p].ps->i].sectnum == s->sectnum)
                    {
                        g_player[p].ps->pos.x += l;
                        g_player[p].ps->pos.y += x;

                        if (g_netServer || numplayers > 1)
                        {
                            g_player[p].ps->opos.x = g_player[p].ps->pos.x;
                            g_player[p].ps->opos.y = g_player[p].ps->pos.y;
                        }

                        g_player[p].ps->bobposx += l;
                        g_player[p].ps->bobposy += x;
                    }

                    if (g_playerSpawnPoints[p].os == s->sectnum)
                    {
                        g_playerSpawnPoints[p].ox += l;
                        g_playerSpawnPoints[p].oy += x;
                    }
                }

                j = headspritesect[s->sectnum];
                while (j >= 0)
                {
                    if (sprite[j].picnum != SECTOREFFECTOR && sprite[j].picnum != LOCATORS)
                    {
                        if (numplayers < 2 && !g_netServer)
                        {
                            actor[j].bposx = sprite[j].x;
                            actor[j].bposy = sprite[j].y;
                        }

                        sprite[j].x += l;
                        sprite[j].y += x;

                        if (g_netServer || numplayers > 1)
                        {
                            actor[j].bposx = sprite[j].x;
                            actor[j].bposy = sprite[j].y;
                        }
                    }
                    j = nextspritesect[j];
                }

                A_MoveSector(i);
                setsprite(i,(vec3_t *)s);

                if ((sc->floorz-sc->ceilingz) < (108<<8))
                {
                    if (ud.clipping == 0)
                        TRAVERSE_CONNECT(p)
                        if (sprite[g_player[p].ps->i].extra > 0)
                        {
                            k = g_player[p].ps->cursectnum;
                            updatesector(g_player[p].ps->pos.x,g_player[p].ps->pos.y,&k);
                            if ((k == -1 && ud.clipping == 0) || (k == s->sectnum && g_player[p].ps->cursectnum != s->sectnum))
                            {
                                g_player[p].ps->pos.x = s->x;
                                g_player[p].ps->pos.y = s->y;

                                g_player[p].ps->opos.x = g_player[p].ps->pos.x;
                                g_player[p].ps->opos.y = g_player[p].ps->pos.y;

                                g_player[p].ps->cursectnum = s->sectnum;

                                setsprite(g_player[p].ps->i,(vec3_t *)s);
                                P_QuickKill(g_player[p].ps);
                            }
                        }

                    j = headspritesect[sprite[OW].sectnum];
                    while (j >= 0)
                    {
                        l = nextspritesect[j];
                        if (sprite[j].statnum == STAT_ACTOR && A_CheckEnemySprite(&sprite[j]) &&
                                sprite[j].picnum != SECTOREFFECTOR && sprite[j].picnum != LOCATORS)
                        {
                            //                    if(sprite[j].sectnum != s->sectnum)
                            {
                                k = sprite[j].sectnum;
                                updatesector(sprite[j].x,sprite[j].y,&k);
                                if (sprite[j].extra >= 0 && k == s->sectnum)
                                {
                                    A_DoGutsDir(j,JIBS6,24);
                                    A_PlaySound(SQUISHED,j);
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
                        g_earthquakeTime = 48;
                        A_PlaySound(EARTHQUAKE,g_player[screenpeek].ps->i);
                    }

                    if (klabs(sc->floorheinum-t[5]) < 8)
                        sc->floorheinum = t[5];
                    else sc->floorheinum += (ksgn(t[5]-sc->floorheinum)<<4);
                }

                m = (s->xvel*sintable[(s->ang+512)&2047])>>14;
                x = (s->xvel*sintable[s->ang&2047])>>14;


                TRAVERSE_CONNECT(p)
                if (g_player[p].ps->cursectnum == s->sectnum && g_player[p].ps->on_ground)
                {
                    g_player[p].ps->pos.x += m;
                    g_player[p].ps->pos.y += x;

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
                        setsprite(j,(vec3_t *)&sprite[j]);
                    }
                    j = nextj;
                }
                A_MoveSector(i);
                setsprite(i,(vec3_t *)s);
            }
            break;

            //Flashing sector lights after reactor EXPLOSION2

        case 3:

            if (t[4] == 0) break;
            p = A_FindPlayer(s,&x);

            //    if(t[5] > 0) { t[5]--; break; }

            if ((g_globalRandom/(sh+1)&31) < 4 && !t[2])
            {
                //       t[5] = 4+(g_globalRandom&7);
                sc->ceilingpal = s->owner>>8;
                sc->floorpal = s->owner&0xff;
                t[0] = s->shade + (g_globalRandom&15);
            }
            else
            {
                //       t[5] = 4+(g_globalRandom&3);
                sc->ceilingpal = s->pal;
                sc->floorpal = s->pal;
                t[0] = t[3];
            }

            sc->ceilingshade = t[0];
            sc->floorshade = t[0];

            wal = &wall[sc->wallptr];

            for (x=sc->wallnum; x > 0; x--,wal++)
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

            if ((g_globalRandom/(sh+1)&31) < 4)
            {
                t[1] = s->shade + (g_globalRandom&15);//Got really bright
                t[0] = s->shade + (g_globalRandom&15);
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

            for (x=sc->wallnum; x > 0; x--,wal++)
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
                if (sprite[j].cstat&16 && A_CheckSpriteFlags(j,SPRITE_NOSHADE) == 0)
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
            p = A_FindPlayer(s,&x);
            if (x < 8192)
            {
                j = s->ang;
                s->ang = getangle(s->x-g_player[p].ps->pos.x,s->y-g_player[p].ps->pos.y);
                A_Shoot(i,FIRELASER);
                s->ang = j;
            }

            if (s->owner==-1) //Start search
            {
                t[4]=0;
                l = 0x7fffffff;
                while (1) //Find the shortest dist
                {
                    s->owner = A_FindLocator((int16_t)t[4],-1); //t[0] hold sectnum

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
                int16_t ta;
                ta = s->ang;
                s->ang = getangle(g_player[p].ps->pos.x-s->x,g_player[p].ps->pos.y-s->y);
                s->ang = ta;
                s->owner = -1;
                goto BOLT;

            }
            else s->xvel=256;

            x = getangle(sprite[s->owner].x-s->x,sprite[s->owner].y-s->y);
            q = G_GetAngleDelta(s->ang,x)>>3;
            s->ang += q;

            if (rnd(32))
            {
                t[2]+=q;
                sc->ceilingshade = 127;
            }
            else
            {
                t[2] +=
                    G_GetAngleDelta(t[2]+512,getangle(g_player[p].ps->pos.x-s->x,g_player[p].ps->pos.y-s->y))>>2;
                sc->ceilingshade = 0;
            }
            IFHIT
            {
                t[3]++;
                if (t[3] == 5)
                {
                    s->zvel += 1024;
                    P_DoQuote(7,g_player[myconnectindex].ps);
                }
            }

            s->z += s->zvel;
            sc->ceilingz += s->zvel;
            sector[t[0]].ceilingz += s->zvel;
            A_MoveSector(i);
            setsprite(i,(vec3_t *)s);
            break;


        case 8:
        case 9:

            // work only if its moving

            j = -1;

            if (actor[i].t_data[4])
            {
                actor[i].t_data[4]++;
                if (actor[i].t_data[4] > 8) KILLIT(i);
                j = 1;
            }
            else j = GetAnimationGoal(&sc->ceilingz);

            if (j >= 0)
            {
                int16_t sn;

                if ((sc->lotag&0x8000) || actor[i].t_data[4])
                    x = -t[3];
                else
                    x = t[3];

                if (st == 9) x = -x;

                j = headspritestat[STAT_EFFECTOR];
                while (j >= 0)
                {
                    if (((sprite[j].lotag) == st) && (sprite[j].hitag) == sh)
                    {
                        sn = sprite[j].sectnum;
                        m = sprite[j].shade;

                        wal = &wall[sector[sn].wallptr];

                        for (l=sector[sn].wallnum; l>0; l--,wal++)
                        {
                            if (wal->hitag != 1)
                            {
                                wal->shade+=x;

                                if (wal->shade < m)
                                    wal->shade = m;
                                else if (wal->shade > actor[j].t_data[2])
                                    wal->shade = actor[j].t_data[2];

                                if (wal->nextwall >= 0)
                                    if (wall[wal->nextwall].hitag != 1)
                                        wall[wal->nextwall].shade = wal->shade;
                            }
                        }

                        sector[sn].floorshade   += x;
                        sector[sn].ceilingshade += x;

                        if (sector[sn].floorshade < m)
                            sector[sn].floorshade = m;
                        else if (sector[sn].floorshade > actor[j].t_data[0])
                            sector[sn].floorshade = actor[j].t_data[0];

                        if (sector[sn].ceilingshade < m)
                            sector[sn].ceilingshade = m;
                        else if (sector[sn].ceilingshade > actor[j].t_data[1])
                            sector[sn].ceilingshade = actor[j].t_data[1];

                    }
                    j = nextspritestat[j];
                }
            }
            break;
        case 10:

            if ((sc->lotag&0xff) == 27 || (sc->floorz > sc->ceilingz && (sc->lotag&0xff) != 23) || sc->lotag == (int16_t) 32791)
            {
                j = 1;

                if ((sc->lotag&0xff) != 27)
                    TRAVERSE_CONNECT(p)
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
                            if (GetAnimationGoal(&sector[s->sectnum].ceilingz) >= 0)
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
                int32_t endwall = sc->wallptr+sc->wallnum;

                for (j=sc->wallptr; j<endwall; j++)
                {
                    k = headspritestat[STAT_ACTOR];
                    while (k >= 0)
                    {
                        if (sprite[k].extra > 0 && A_CheckEnemySprite(&sprite[k]) && clipinsidebox(sprite[k].x,sprite[k].y,j,256L) == 1)
                            goto BOLT;
                        k = nextspritestat[k];
                    }

                    k = headspritestat[STAT_PLAYER];
                    while (k >= 0)
                    {
                        if (sprite[k].owner >= 0 && clipinsidebox(sprite[k].x,sprite[k].y,j,144L) == 1)
                        {
                            t[5] = 8; // Delay
                            k = (SP>>3)*t[3];
                            t[2]-=k;
                            t[4]-=k;
                            A_MoveSector(i);
                            setsprite(i,(vec3_t *)s);
                            goto BOLT;
                        }
                        k = nextspritestat[k];
                    }
                }

                k = (SP>>3)*t[3];
                t[2]+=k;
                t[4]+=k;
                A_MoveSector(i);
                setsprite(i,(vec3_t *)s);

                if (t[4] <= -511 || t[4] >= 512)
                {
                    t[4] = 0;
                    t[2] &= 0xffffff00;
                    A_MoveSector(i);
                    setsprite(i,(vec3_t *)s);
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
                for (j = sc->wallnum; j > 0; j--, wal++)
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
                    if (sprite[j].cstat&16 && A_CheckSpriteFlags(j,SPRITE_NOSHADE) == 0)
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
                    for (j=sc->wallnum; j>0; j--,wal++)
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
                        if (sc->ceilingstat&1 && A_CheckSpriteFlags(j,SPRITE_NOSHADE) == 0)
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
                        for (j=sc->wallnum; j>0; j--,wal++)
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
                for (x=0; x<7; x++) RANDOMSCRAP;
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
                        A_CallSound(s->sectnum,i);
                        break;
                    }
                    t[3]++;
                }
                else if (t[4] == 2)
                {
                    if (t[3]<1)
                    {
                        t[4] = 0;
                        A_CallSound(s->sectnum,i);
                        break;
                    }
                    t[3]--;
                }

                A_MoveSector(i);
                setsprite(i,(vec3_t *)s);
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

            A_MoveSector(i);
            setsprite(i,(vec3_t *)s);

            break;

        case 17:

            q = t[0]*(SP<<2);

            sc->ceilingz += q;
            sc->floorz += q;

            j = headspritesect[s->sectnum];
            while (j >= 0)
            {
                if (sprite[j].statnum == STAT_PLAYER && sprite[j].owner >= 0)
                {
                    p = sprite[j].yvel;
                    if (numplayers < 2 && !g_netServer)
                        g_player[p].ps->opos.z = g_player[p].ps->pos.z;
                    g_player[p].ps->pos.z += q;
                    g_player[p].ps->truefz += q;
                    g_player[p].ps->truecz += q;
                    if (g_netServer || numplayers > 1)
                        g_player[p].ps->opos.z = g_player[p].ps->pos.z;
                }
                if (sprite[j].statnum != STAT_EFFECTOR)
                {
                    actor[j].bposz = sprite[j].z;
                    sprite[j].z += q;
                }

                actor[j].floorz = sc->floorz;
                actor[j].ceilingz = sc->ceilingz;

                j = nextspritesect[j];
            }

            if (t[0]) //If in motion
            {
                if (klabs(sc->floorz-t[2]) <= SP)
                {
                    G_ActivateWarpElevators(i,0);
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

                j = headspritestat[STAT_EFFECTOR];
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

                    if (sprite[k].statnum == STAT_PLAYER && sprite[k].owner >= 0)
                    {
                        p = sprite[k].yvel;

                        g_player[p].ps->pos.x += sprite[j].x-s->x;
                        g_player[p].ps->pos.y += sprite[j].y-s->y;
                        g_player[p].ps->pos.z = sector[sprite[j].sectnum].floorz-(sc->floorz-g_player[p].ps->pos.z);

                        actor[k].floorz = sector[sprite[j].sectnum].floorz;
                        actor[k].ceilingz = sector[sprite[j].sectnum].ceilingz;

                        g_player[p].ps->bobposx = g_player[p].ps->opos.x = g_player[p].ps->pos.x;
                        g_player[p].ps->bobposy = g_player[p].ps->opos.y = g_player[p].ps->pos.y;
                        g_player[p].ps->opos.z = g_player[p].ps->pos.z;

                        g_player[p].ps->truefz = actor[k].floorz;
                        g_player[p].ps->truecz = actor[k].ceilingz;
                        g_player[p].ps->bobcounter = 0;

                        changespritesect(k,sprite[j].sectnum);
                        g_player[p].ps->cursectnum = sprite[j].sectnum;
                    }
                    else if (sprite[k].statnum != STAT_EFFECTOR)
                    {
                        sprite[k].x +=
                            sprite[j].x-s->x;
                        sprite[k].y +=
                            sprite[j].y-s->y;
                        sprite[k].z = sector[sprite[j].sectnum].floorz-
                                      (sc->floorz-sprite[k].z);

                        Bmemcpy(&actor[k].bposx, &sprite[k], sizeof(vec3_t));

                        changespritesect(k,sprite[j].sectnum);
                        setsprite(k,(vec3_t *)&sprite[k]);

                        actor[k].floorz = sector[sprite[j].sectnum].floorz;
                        actor[k].ceilingz = sector[sprite[j].sectnum].ceilingz;

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
                                    g_player[sprite[j].yvel].ps->pos.z += sc->extra;
                            if (sprite[j].zvel == 0 && sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PROJECTILE)
                            {
                                actor[j].bposz = sprite[j].z += sc->extra;
                                actor[j].floorz = sc->floorz;
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
                                    g_player[sprite[j].yvel].ps->pos.z -= sc->extra;
                            if (sprite[j].zvel == 0 && sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PROJECTILE)
                            {
                                actor[j].bposz = sprite[j].z -= sc->extra;
                                actor[j].floorz = sc->floorz;
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
                    for (j=x; j<q; j++)
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

                    j = headspritestat[STAT_EFFECTOR];
                    while (j >= 0)
                    {
                        if (sprite[j].lotag == 0 && sprite[j].hitag==sh)
                        {
                            q = sprite[sprite[j].owner].sectnum;
                            sector[sprite[j].sectnum].floorpal = sector[sprite[j].sectnum].ceilingpal =
                                    sector[q].floorpal;
                            sector[sprite[j].sectnum].floorshade = sector[sprite[j].sectnum].ceilingshade =
                                    sector[q].floorshade;

                            actor[sprite[j].owner].t_data[0] = 2;
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
                    P_DoQuote(8,g_player[myconnectindex].ps);

                    l = headspritestat[STAT_EFFECTOR];
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
                                if (actor[l].t_data[0] == 0)
                                {
                                    actor[l].t_data[0] = 1; //Shut them all on
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
                    A_CallSound(s->sectnum,i);
                    break;
                }

                j = headspritesect[s->sectnum];
                while (j >= 0)
                {
                    nextj = nextspritesect[j];

                    if (sprite[j].statnum != STAT_EFFECTOR && sprite[j].zvel == 0)
                    {
                        sprite[j].x += x;
                        sprite[j].y += l;
                        setsprite(j,(vec3_t *)&sprite[j]);
                        if (sector[sprite[j].sectnum].floorstat&2)
                            if (sprite[j].statnum == STAT_ZOMBIEACTOR)
                                A_Fall(j);
                    }
                    j = nextj;
                }

                dragpoint((int16_t)t[1],wall[t[1]].x+x,wall[t[1]].y+l);
                dragpoint((int16_t)t[2],wall[t[2]].x+x,wall[t[2]].y+l);

                TRAVERSE_CONNECT(p)
                if (g_player[p].ps->cursectnum == s->sectnum && g_player[p].ps->on_ground)
                {
                    g_player[p].ps->pos.x += x;
                    g_player[p].ps->pos.y += l;

                    g_player[p].ps->opos.x = g_player[p].ps->pos.x;
                    g_player[p].ps->opos.y = g_player[p].ps->pos.y;

                    g_player[p].ps->pos.z += PHEIGHT;
                    setsprite(g_player[p].ps->i,(vec3_t *)g_player[p].ps);
                    g_player[p].ps->pos.z -= PHEIGHT;
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
                l = (intptr_t) &sc->ceilingz;
            else
                l = (intptr_t) &sc->floorz;

            if (t[0] == 1)   //Decide if the s->sectnum should go up or down
            {
                s->zvel = ksgn(s->z-*(int32_t *)l) * (SP<<4);
                t[0]++;
            }

            if (sc->extra == 0)
            {
                *(int32_t *)l += s->zvel;

                if (klabs(*(int32_t *)l-s->z) < 1024)
                {
                    *(int32_t *)l = s->z;
                    KILLIT(i); //All done
                }
            }
            else sc->extra--;
            break;

        case 22:

            if (t[1])
            {
                if (GetAnimationGoal(&sector[t[0]].ceilingz) >= 0)
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
                    case STAT_MISC:
                        switch (DynamicTileMap[sprite[j].picnum])
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
                    case STAT_STANDABLE:
                        if (sprite[j].picnum == TRIPBOMB) break;
                    case STAT_ACTOR:
                    case STAT_DEFAULT:
                        if (
                            sprite[j].picnum == BOLT1 ||
                            sprite[j].picnum == BOLT1+1 ||
                            sprite[j].picnum == BOLT1+2 ||
                            sprite[j].picnum == BOLT1+3 ||
                            sprite[j].picnum == SIDEBOLT1 ||
                            sprite[j].picnum == SIDEBOLT1+1 ||
                            sprite[j].picnum == SIDEBOLT1+2 ||
                            sprite[j].picnum == SIDEBOLT1+3 ||
                            A_CheckSwitchTile(j)
                        )
                            break;

                        if (!(sprite[j].picnum >= CRANE && sprite[j].picnum <= (CRANE+3)))
                        {
                            if (sprite[j].z > (actor[j].floorz-(16<<8)))
                            {
                                actor[j].bposx = sprite[j].x;
                                actor[j].bposy = sprite[j].y;

                                sprite[j].x += x>>2;
                                sprite[j].y += l>>2;

                                setsprite(j,(vec3_t *)&sprite[j]);

                                if (sector[sprite[j].sectnum].floorstat&2)
                                    if (sprite[j].statnum == STAT_ZOMBIEACTOR)
                                        A_Fall(j);
                            }
                        }
                        break;
                    }
                j = nextj;
            }

            p = myconnectindex;
            if (g_player[p].ps->cursectnum == s->sectnum && g_player[p].ps->on_ground)
                if (klabs(g_player[p].ps->pos.z-g_player[p].ps->truefz) < PHEIGHT+(9<<8))
                {
                    fricxv += x<<3;
                    fricyv += l<<3;
                }

            sc->floorxpanning += SP>>7;

            break;

        case 35:
            if (sc->ceilingz > s->z)
                for (j = 0; j < 8; j++)
                {
                    s->ang += krand()&511;
                    k = A_Spawn(i,SMALLSMOKE);
                    sprite[k].xvel = 96+(krand()&127);
                    A_SetSprite(k,CLIPMASK0);
                    setsprite(k,(vec3_t *)&sprite[k]);
                    if (rnd(16))
                        A_Spawn(i,EXPLOSION2);
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
                if (sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PLAYER)
                {
                    actor[j].bposx = sprite[j].x;
                    actor[j].bposy = sprite[j].y;

                    sprite[j].x += l;
                    sprite[j].y += x;

                    sprite[j].z += s->zvel;
                    setsprite(j,(vec3_t *)&sprite[j]);
                }
                j = nextj;
            }

            p = myconnectindex;
            if (sprite[g_player[p].ps->i].sectnum == s->sectnum && g_player[p].ps->on_ground)
            {
                fricxv += l<<5;
                fricyv += x<<5;
            }

            TRAVERSE_CONNECT(p)
            if (sprite[g_player[p].ps->i].sectnum == s->sectnum && g_player[p].ps->on_ground)
                g_player[p].ps->pos.z += s->zvel;

            A_MoveSector(i);
            setsprite(i,(vec3_t *)s);

            break;


        case 27:

            if (ud.recstat == 0 || !ud.democams) break;

            actor[i].tempang = s->ang;

            p = A_FindPlayer(s,&x);
            if (sprite[g_player[p].ps->i].extra > 0 && myconnectindex == screenpeek)
            {
                if (t[0] < 0)
                {
                    ud.camerasprite = i;
                    t[0]++;
                }
                else if (ud.recstat == 2 && g_player[p].ps->newowner == -1)
                {
                    if (cansee(s->x,s->y,s->z,SECT,g_player[p].ps->pos.x,g_player[p].ps->pos.y,g_player[p].ps->pos.z,g_player[p].ps->cursectnum))
                    {
                        if (x < (int32_t)((unsigned)sh))
                        {
                            ud.camerasprite = i;
                            t[0] = 999;
                            s->ang += G_GetAngleDelta(s->ang,getangle(g_player[p].ps->pos.x-s->x,g_player[p].ps->pos.y-s->y))>>3;
                            SP = 100+((s->z-g_player[p].ps->pos.z)/257);

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
                        s->ang = getangle(g_player[p].ps->pos.x-s->x,g_player[p].ps->pos.y-s->y);

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
                p = A_FindPlayer(s,&x);
                if (x > 15500)
                    break;
                T1 = 1;
                T2 = 64 + (krand()&511);
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
                    A_PlaySound(THUNDER,i);
                else if (T3 == (T2>>3))
                    A_PlaySound(LIGHTNING_SLAP,i);
                else if (T3 == (T2>>2))
                {
                    j = headspritestat[STAT_DEFAULT];
                    while (j >= 0)
                    {
                        if (sprite[j].picnum == NATURALLIGHTNING && sprite[j].hitag == s->hitag)
                            sprite[j].cstat |= 32768;
                        j = nextspritestat[j];
                    }
                }
                else if (T3 > (T2>>3) && T3 < (T2>>2))
                {
                    if (cansee(s->x,s->y,s->z,s->sectnum,g_player[screenpeek].ps->pos.x,g_player[screenpeek].ps->pos.y,g_player[screenpeek].ps->pos.z,g_player[screenpeek].ps->cursectnum))
                        j = 1;
                    else j = 0;

                    if (rnd(192) && (T3&1))
                    {
                        if (j)
                            g_player[screenpeek].ps->visibility = 0;
                    }
                    else if (j)
                        g_player[screenpeek].ps->visibility = ud.const_visibility;

                    j = headspritestat[STAT_DEFAULT];
                    while (j >= 0)
                    {
                        if (sprite[j].picnum == NATURALLIGHTNING && sprite[j].hitag == s->hitag)
                        {
                            if (rnd(32) && (T3&1))
                            {
                                sprite[j].cstat &= 32767;
                                A_Spawn(j,SMALLSMOKE);

                                p = A_FindPlayer(s,&x);
                                x = ldist(&sprite[g_player[p].ps->i], &sprite[j]);
                                if (x < 768)
                                {
                                    if (!A_CheckSoundPlaying(g_player[p].ps->i,DUKE_LONGTERM_PAIN))
                                        A_PlaySound(DUKE_LONGTERM_PAIN,g_player[p].ps->i);
                                    A_PlaySound(SHORT_CIRCUIT,g_player[p].ps->i);
                                    sprite[g_player[p].ps->i].extra -= 8+(krand()&7);
                                    g_player[p].ps->pals.f = 32;
                                    g_player[p].ps->pals.r = 16;
                                    g_player[p].ps->pals.g = 0;
                                    g_player[p].ps->pals.b = 0;
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
            l = mulscale12((int32_t)s->yvel,sintable[s->hitag&2047]);
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
                            A_CallSound(s->sectnum,i);
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
                                        g_player[sprite[j].yvel].ps->pos.z += l;
                                if (sprite[j].zvel == 0 && sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PROJECTILE)
                                {
                                    actor[j].bposz = sprite[j].z += l;
                                    actor[j].floorz = sc->floorz;
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
                            A_CallSound(s->sectnum,i);
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
                                        g_player[sprite[j].yvel].ps->pos.z += l;
                                if (sprite[j].zvel == 0 && sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PROJECTILE)
                                {
                                    actor[j].bposz = sprite[j].z += l;
                                    actor[j].floorz = sc->floorz;
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
                        A_CallSound(s->sectnum,i);
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
                                    g_player[sprite[j].yvel].ps->pos.z += l;
                            if (sprite[j].zvel == 0 && sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PROJECTILE)
                            {
                                actor[j].bposz = sprite[j].z += l;
                                actor[j].floorz = sc->floorz;
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
                        A_CallSound(s->sectnum,i);
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
                                    g_player[sprite[j].yvel].ps->pos.z -= l;
                            if (sprite[j].zvel == 0 && sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PROJECTILE)
                            {
                                actor[j].bposz = sprite[j].z -= l;
                                actor[j].floorz = sc->floorz;
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
                            A_CallSound(s->sectnum,i);
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
                            A_CallSound(s->sectnum,i);
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
                        A_CallSound(s->sectnum,i);
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
                        A_CallSound(s->sectnum,i);
                    }
                    else sc->ceilingz -= ksgn(s->z-t[1])*SP;
                }
            }
            break;

        case 33:
            if (g_earthquakeTime > 0 && (krand()&7) == 0)
                RANDOMSCRAP;
            break;
        case 36:

            if (t[0])
            {
                if (t[0] == 1)
                    A_Shoot(i,sc->extra);
                else if (t[0] == GAMETICSPERSEC*5)
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
                k = A_Spawn(i,EXPLOSION2);
                sprite[k].xrepeat = sprite[k].yrepeat = 2+(krand()&7);
                sprite[k].z = sc->floorz-(krand()%x);
                sprite[k].ang += 256-(krand()%511);
                sprite[k].xvel = krand()&127;
                A_SetSprite(k,CLIPMASK0);
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
                k = A_Spawn(i,EXPLOSION2);
                sprite[k].xrepeat = sprite[k].yrepeat = 2+(krand()&3);
                sprite[k].z = sc->floorz-(krand()%x);
                sprite[k].ang += 256-(krand()%511);
                sprite[k].xvel = krand()&127;
                A_SetSprite(k,CLIPMASK0);
            }
            break;
#ifdef POLYMER
        case 49:
        {
            if (!A_CheckSpriteFlags(i, SPRITE_NOLIGHT) && getrendermode() == 4 &&
                    !(A_CheckSpriteFlags(i, SPRITE_USEACTIVATOR) && sector[sprite[i].sectnum].lotag & 16384))
            {
                if (actor[i].lightptr == NULL)
                {
#pragma pack(push,1)
                    _prlight mylight;
#pragma pack(pop)
                    mylight.sector = SECT;
                    Bmemcpy(&mylight, &sprite[i], sizeof(int32_t) * 3);
                    mylight.range = SHT;
                    mylight.color[0] = sprite[i].xvel;
                    mylight.color[1] = sprite[i].yvel;
                    mylight.color[2] = sprite[i].zvel;
                    mylight.radius = 0;
                    mylight.angle = SA;
                    mylight.horiz = SH;
                    mylight.minshade = sprite[i].xoffset;
                    mylight.maxshade = sprite[i].yoffset;
                    mylight.tilenum = 0;

                    if (CS & 2)
                    {
                        if (CS & 512)
                            mylight.priority = PR_LIGHT_PRIO_LOW;
                        else
                            mylight.priority = PR_LIGHT_PRIO_HIGH;
                    }
                    else
                        mylight.priority = PR_LIGHT_PRIO_MAX;

                    actor[i].lightId = polymer_addlight(&mylight);
                    if (actor[i].lightId >= 0)
                        actor[i].lightptr = &prlights[actor[i].lightId];
                    break;
                }

                if (Bmemcmp(&sprite[i], actor[i].lightptr, sizeof(int32_t) * 3))
                {
                    Bmemcpy(actor[i].lightptr, &sprite[i], sizeof(int32_t) * 3);
                    actor[i].lightptr->sector = sprite[i].sectnum;
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if (SHT != actor[i].lightptr->range)
                {
                    actor[i].lightptr->range = SHT;
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if ((sprite[i].xvel != actor[i].lightptr->color[0]) ||
                        (sprite[i].yvel != actor[i].lightptr->color[1]) ||
                        (sprite[i].zvel != actor[i].lightptr->color[2]))
                {
                    actor[i].lightptr->color[0] = sprite[i].xvel;
                    actor[i].lightptr->color[1] = sprite[i].yvel;
                    actor[i].lightptr->color[2] = sprite[i].zvel;
                }
            }
            break;
        }
        case 50:
        {
            if (!A_CheckSpriteFlags(i, SPRITE_NOLIGHT) && getrendermode() == 4 &&
                    !(A_CheckSpriteFlags(i, SPRITE_USEACTIVATOR) && sector[sprite[i].sectnum].lotag & 16384))
            {
                if (actor[i].lightptr == NULL)
                {
#pragma pack(push,1)
                    _prlight mylight;
#pragma pack(pop)

                    mylight.sector = SECT;
                    Bmemcpy(&mylight, &sprite[i], sizeof(int32_t) * 3);
                    mylight.range = SHT;
                    mylight.color[0] = sprite[i].xvel;
                    mylight.color[1] = sprite[i].yvel;
                    mylight.color[2] = sprite[i].zvel;
                    mylight.radius = (256-(SS+128))<<1;
                    mylight.faderadius = (int16_t)(mylight.radius * 0.75f);
                    mylight.angle = SA;
                    mylight.horiz = SH;
                    mylight.minshade = sprite[i].xoffset;
                    mylight.maxshade = sprite[i].yoffset;
                    mylight.tilenum = actor[i].picnum;

                    if (CS & 2)
                    {
                        if (CS & 512)
                            mylight.priority = PR_LIGHT_PRIO_LOW;
                        else
                            mylight.priority = PR_LIGHT_PRIO_HIGH;
                    }
                    else
                        mylight.priority = PR_LIGHT_PRIO_MAX;

                    actor[i].lightId = polymer_addlight(&mylight);
                    if (actor[i].lightId >= 0)
                        actor[i].lightptr = &prlights[actor[i].lightId];
                    break;
                }

                if (Bmemcmp(&sprite[i], actor[i].lightptr, sizeof(int32_t) * 3))
                {
                    Bmemcpy(actor[i].lightptr, &sprite[i], sizeof(int32_t) * 3);
                    actor[i].lightptr->sector = sprite[i].sectnum;
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if (SHT != actor[i].lightptr->range)
                {
                    actor[i].lightptr->range = SHT;
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if ((sprite[i].xvel != actor[i].lightptr->color[0]) ||
                        (sprite[i].yvel != actor[i].lightptr->color[1]) ||
                        (sprite[i].zvel != actor[i].lightptr->color[2]))
                {
                    actor[i].lightptr->color[0] = sprite[i].xvel;
                    actor[i].lightptr->color[1] = sprite[i].yvel;
                    actor[i].lightptr->color[2] = sprite[i].zvel;
                }
                if (((256-(SS+128))<<1) != actor[i].lightptr->radius)
                {
                    actor[i].lightptr->radius = (256-(SS+128))<<1;
                    actor[i].lightptr->faderadius = (int16_t)(actor[i].lightptr->radius * 0.75f);
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if (SA != actor[i].lightptr->angle)
                {
                    actor[i].lightptr->angle = SA;
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if (SH != actor[i].lightptr->horiz)
                {
                    actor[i].lightptr->horiz = SH;
                    actor[i].lightptr->flags.invalidate = 1;
                }
                actor[i].lightptr->tilenum = actor[i].picnum;
            }

            break;
        }
#endif // POLYMER

        }
BOLT:
        i = nexti;
    }

    //Sloped sin-wave floors!
    for (i=headspritestat[STAT_EFFECTOR]; i>=0; i=nextspritestat[i])
    {
        s = &sprite[i];
        if (s->lotag != 29) continue;
        sc = &sector[s->sectnum];
        if (sc->wallnum != 4) continue;
        wal = &wall[sc->wallptr+2];
        alignflorslope(s->sectnum,wal->x,wal->y,sector[wal->nextsector].floorz);
    }
}

void A_PlayAlertSound(int32_t i)
{
    if (sprite[i].extra > 0)
        switch (DynamicTileMap[PN])
        {
        case LIZTROOPONTOILET__STATIC:
        case LIZTROOPJUSTSIT__STATIC:
        case LIZTROOPSHOOT__STATIC:
        case LIZTROOPJETPACK__STATIC:
        case LIZTROOPDUCKING__STATIC:
        case LIZTROOPRUNNING__STATIC:
        case LIZTROOP__STATIC:
            A_PlaySound(PRED_RECOG,i);
            break;
        case LIZMAN__STATIC:
        case LIZMANSPITTING__STATIC:
        case LIZMANFEEDING__STATIC:
        case LIZMANJUMP__STATIC:
            A_PlaySound(CAPT_RECOG,i);
            break;
        case PIGCOP__STATIC:
        case PIGCOPDIVE__STATIC:
            A_PlaySound(PIG_RECOG,i);
            break;
        case RECON__STATIC:
            A_PlaySound(RECO_RECOG,i);
            break;
        case DRONE__STATIC:
            A_PlaySound(DRON_RECOG,i);
            break;
        case COMMANDER__STATIC:
        case COMMANDERSTAYPUT__STATIC:
            A_PlaySound(COMM_RECOG,i);
            break;
        case ORGANTIC__STATIC:
            A_PlaySound(TURR_RECOG,i);
            break;
        case OCTABRAIN__STATIC:
        case OCTABRAINSTAYPUT__STATIC:
            A_PlaySound(OCTA_RECOG,i);
            break;
        case BOSS1__STATIC:
            S_PlaySound(BOS1_RECOG);
            break;
        case BOSS2__STATIC:
            if (sprite[i].pal == 1)
                S_PlaySound(BOS2_RECOG);
            else S_PlaySound(WHIPYOURASS);
            break;
        case BOSS3__STATIC:
            if (sprite[i].pal == 1)
                S_PlaySound(BOS3_RECOG);
            else S_PlaySound(RIPHEADNECK);
            break;
        case BOSS4__STATIC:
        case BOSS4STAYPUT__STATIC:
            if (sprite[i].pal == 1)
                S_PlaySound(BOS4_RECOG);
            S_PlaySound(BOSS4_FIRSTSEE);
            break;
        case GREENSLIME__STATIC:
            A_PlaySound(SLIM_RECOG,i);
            break;
        }
}

int32_t A_CheckEnemyTile(int32_t pn)
{
    //this case can't be handled by the dynamictostatic system because it adds
    //stuff to the value from names.h so handling separately
    if ((pn >= GREENSLIME) && (pn <= GREENSLIME+7)) return 1;

    if (A_CheckSpriteTileFlags(pn,SPRITE_BADGUY)) return 1;

    if (ActorType[pn]) return 1;

    switch (DynamicTileMap[pn])
    {
    case SHARK__STATIC:
    case RECON__STATIC:
    case DRONE__STATIC:
    case LIZTROOPONTOILET__STATIC:
    case LIZTROOPJUSTSIT__STATIC:
    case LIZTROOPSTAYPUT__STATIC:
    case LIZTROOPSHOOT__STATIC:
    case LIZTROOPJETPACK__STATIC:
    case LIZTROOPDUCKING__STATIC:
    case LIZTROOPRUNNING__STATIC:
    case LIZTROOP__STATIC:
    case OCTABRAIN__STATIC:
    case COMMANDER__STATIC:
    case COMMANDERSTAYPUT__STATIC:
    case PIGCOP__STATIC:
    case EGG__STATIC:
    case PIGCOPSTAYPUT__STATIC:
    case PIGCOPDIVE__STATIC:
    case LIZMAN__STATIC:
    case LIZMANSPITTING__STATIC:
    case LIZMANFEEDING__STATIC:
    case LIZMANJUMP__STATIC:
    case ORGANTIC__STATIC:
    case BOSS1__STATIC:
    case BOSS2__STATIC:
    case BOSS3__STATIC:
    case BOSS4__STATIC:
        //case GREENSLIME:
        //case GREENSLIME+1:
        //case GREENSLIME+2:
        //case GREENSLIME+3:
        //case GREENSLIME+4:
        //case GREENSLIME+5:
        //case GREENSLIME+6:
        //case GREENSLIME+7:
    case RAT__STATIC:
    case ROTATEGUN__STATIC:
        return 1;
    }
    return 0;
}

inline int32_t A_CheckEnemySprite(spritetype *s)
{
    return(A_CheckEnemyTile(s->picnum));
}

int32_t A_CheckSwitchTile(int32_t i)
{
    int32_t j;
    //MULTISWITCH has 4 states so deal with it separately
    if ((PN >= MULTISWITCH) && (PN <=MULTISWITCH+3)) return 1;
    // ACCESSSWITCH and ACCESSSWITCH2 are only active in 1 state so deal with them separately
    if ((PN == ACCESSSWITCH) || (PN == ACCESSSWITCH2)) return 1;
    //loop to catch both states of switches
    for (j=1; j>=0; j--)
    {
        switch (DynamicTileMap[PN-j])
        {
        case HANDPRINTSWITCH__STATIC:
            //case HANDPRINTSWITCH+1:
        case ALIENSWITCH__STATIC:
            //case ALIENSWITCH+1:
        case MULTISWITCH__STATIC:
            //case MULTISWITCH+1:
            //case MULTISWITCH+2:
            //case MULTISWITCH+3:
            //case ACCESSSWITCH:
            //case ACCESSSWITCH2:
        case PULLSWITCH__STATIC:
            //case PULLSWITCH+1:
        case HANDSWITCH__STATIC:
            //case HANDSWITCH+1:
        case SLOTDOOR__STATIC:
            //case SLOTDOOR+1:
        case LIGHTSWITCH__STATIC:
            //case LIGHTSWITCH+1:
        case SPACELIGHTSWITCH__STATIC:
            //case SPACELIGHTSWITCH+1:
        case SPACEDOORSWITCH__STATIC:
            //case SPACEDOORSWITCH+1:
        case FRANKENSTINESWITCH__STATIC:
            //case FRANKENSTINESWITCH+1:
        case LIGHTSWITCH2__STATIC:
            //case LIGHTSWITCH2+1:
        case POWERSWITCH1__STATIC:
            //case POWERSWITCH1+1:
        case LOCKSWITCH1__STATIC:
            //case LOCKSWITCH1+1:
        case POWERSWITCH2__STATIC:
            //case POWERSWITCH2+1:
        case DIPSWITCH__STATIC:
            //case DIPSWITCH+1:
        case DIPSWITCH2__STATIC:
            //case DIPSWITCH2+1:
        case TECHSWITCH__STATIC:
            //case TECHSWITCH+1:
        case DIPSWITCH3__STATIC:
            //case DIPSWITCH3+1:
            return 1;
        }
    }
    return 0;
}

void G_MoveWorld(void)
{
    G_MoveZombieActors();     //ST 2
    G_MoveWeapons();          //ST 4
    G_MoveTransports();       //ST 9

    G_MovePlayers();          //ST 10
    G_MoveFallers();          //ST 12
    G_MoveMisc();             //ST 5

    G_MoveActors();           //ST 1
    G_MoveEffectors();        //ST 3

    G_MoveStandables();       //ST 6

    {
        int32_t i, p, j, k = MAXSTATUS-1, pl;

        do
        {
            i = headspritestat[k];

            while (i >= 0)
            {
#ifdef POLYMER
                if (getrendermode() == 4)
                {
                    spritetype *s = &sprite[i];

                    if ((sprite[i].picnum != SECTOREFFECTOR && (s->cstat & 32768)) || A_CheckSpriteFlags(i, SPRITE_NOLIGHT) ||
                            (A_CheckSpriteFlags(i, SPRITE_USEACTIVATOR) && sector[sprite[i].sectnum].lotag & 16384))
                    {
                        if (actor[i].lightptr != NULL)
                        {
                            polymer_deletelight(actor[i].lightId);
                            actor[i].lightId = -1;
                            actor[i].lightptr = NULL;
                        }
                    }
                    else
                    {

                        if (actor[i].lightptr != NULL && actor[i].lightcount)
                        {
                            if (!(--actor[i].lightcount))
                            {
                                polymer_deletelight(actor[i].lightId);
                                actor[i].lightId = -1;
                                actor[i].lightptr = NULL;
                            }
                        }

                        switch (DynamicTileMap[sprite[i].picnum-1])
                        {
                        case DIPSWITCH__STATIC:
                        case DIPSWITCH2__STATIC:
                        case DIPSWITCH3__STATIC:
                        case PULLSWITCH__STATIC:
                        case SLOTDOOR__STATIC:
                        case LIGHTSWITCH__STATIC:
                        case SPACELIGHTSWITCH__STATIC:
                        case SPACEDOORSWITCH__STATIC:
                        case FRANKENSTINESWITCH__STATIC:
                        case POWERSWITCH1__STATIC:
                        case LOCKSWITCH1__STATIC:
                        case POWERSWITCH2__STATIC:
                        case TECHSWITCH__STATIC:
                        case ACCESSSWITCH__STATIC:
                        case ACCESSSWITCH2__STATIC:
                        {
                            int32_t x, y;

                            if ((s->cstat & 32768) || A_CheckSpriteFlags(i, SPRITE_NOLIGHT) ||
                                    !inside(s->x+((sintable[(s->ang+512)&2047])>>9), s->y+((sintable[(s->ang)&2047])>>9), s->sectnum))
                            {
                                if (actor[i].lightptr != NULL)
                                {
                                    polymer_deletelight(actor[i].lightId);
                                    actor[i].lightId = -1;
                                    actor[i].lightptr = NULL;
                                }
                                break;
                            }

                            x = ((sintable[(s->ang+512)&2047])>>7);
                            y = ((sintable[(s->ang)&2047])>>7);

                            s->x += x;
                            s->y += y;

                            G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), 1024, 48+(255<<8)+(48<<16),PR_LIGHT_PRIO_LOW);
                            s->x -= x;
                            s->y -= y;
                        }
                        break;
                        }

                        switch (DynamicTileMap[sprite[i].picnum])
                        {
                        case ATOMICHEALTH__STATIC:
                            G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), LIGHTRAD2 * 3, 128+(128<<8)+(255<<16),PR_LIGHT_PRIO_HIGH_GAME);
                            break;

                        case FIRE__STATIC:
                        case FIRE2__STATIC:
                        case BURNING__STATIC:
                        case BURNING2__STATIC:
                            /*
                            if (Actor[i].floorz - Actor[i].ceilingz < 128) break;
                            if (s->z > Actor[i].floorz+2048) break;
                            */
                            G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), LIGHTRAD2, 255+(95<<8),PR_LIGHT_PRIO_HIGH_GAME);
                            break;

                        case OOZFILTER__STATIC:
                            if (s->xrepeat > 4)
                                G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), 4096, 128+(255<<8)+(128<<16),PR_LIGHT_PRIO_HIGH_GAME);
                            break;
                        case FLOORFLAME__STATIC:
                        case FIREBARREL__STATIC:
                        case FIREVASE__STATIC:
                            G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), LIGHTRAD, 255+(95<<8),PR_LIGHT_PRIO_HIGH_GAME);
                            break;

                        case EXPLOSION2__STATIC:
                            if (!actor[i].lightcount)
                            {
                                int32_t x = ((sintable[(s->ang+512)&2047])>>6);
                                int32_t y = ((sintable[(s->ang)&2047])>>6);

                                s->x -= x;
                                s->y -= y;

                                G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), LIGHTRAD, 255+(95<<8),
                                               s->yrepeat > 32 ? PR_LIGHT_PRIO_HIGH_GAME : PR_LIGHT_PRIO_LOW_GAME);

                                s->x += x;
                                s->y += y;
                            }
                            break;
                        case FORCERIPPLE__STATIC:
                        case TRANSPORTERBEAM__STATIC:
                            G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), LIGHTRAD, 80+(80<<8)+(255<<16),PR_LIGHT_PRIO_LOW_GAME);
                            break;
                        case GROWSPARK__STATIC:
                        {
                            int32_t x = ((sintable[(s->ang+512)&2047])>>6);
                            int32_t y = ((sintable[(s->ang)&2047])>>6);

                            s->x -= x;
                            s->y -= y;

                            G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), 2048, 255+(95<<8),PR_LIGHT_PRIO_HIGH_GAME);

                            s->x += x;
                            s->y += y;
                        }
                        break;
                        case SHRINKEREXPLOSION__STATIC:
                        {
                            int32_t x = ((sintable[(s->ang+512)&2047])>>6);
                            int32_t y = ((sintable[(s->ang)&2047])>>6);

                            s->x -= x;
                            s->y -= y;

                            G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), 2048, 128+(255<<8)+(128<<16),PR_LIGHT_PRIO_HIGH_GAME);

                            s->x += x;
                            s->y += y;
                        }
                        break;
                        case FREEZEBLAST__STATIC:
                            G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), LIGHTRAD<<2, 128+(128<<8)+(255<<16),PR_LIGHT_PRIO_HIGH_GAME);
                            break;
                        case COOLEXPLOSION1__STATIC:
                            G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), LIGHTRAD<<2, 128+(0<<8)+(255<<16),PR_LIGHT_PRIO_HIGH_GAME);
                            break;
                        case SHRINKSPARK__STATIC:
                            G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), LIGHTRAD, 128+(255<<8)+(128<<16),PR_LIGHT_PRIO_HIGH_GAME);
                            break;
                        case FIRELASER__STATIC:
                            G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), 64 * s->yrepeat, 255+(95<<8),PR_LIGHT_PRIO_LOW_GAME);
                            break;
                        case RPG__STATIC:
                            G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), 128 * s->yrepeat, 255+(95<<8),PR_LIGHT_PRIO_LOW_GAME);
                            break;
                        case SHOTSPARK1__STATIC:
                            if (actor[i].t_data[2] == 0) // check for first frame of action
                            {
                                int32_t x = ((sintable[(s->ang+512)&2047])>>7);
                                int32_t y = ((sintable[(s->ang)&2047])>>7);

                                s->x -= x;
                                s->y -= y;

                                G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), 16 * s->yrepeat, 255+(95<<8),PR_LIGHT_PRIO_LOW_GAME);
                                actor[i].lightcount = 1;

                                s->x += x;
                                s->y += y;
                            }
                            break;
                        case DIPSWITCH__STATIC:
                        case DIPSWITCH2__STATIC:
                        case DIPSWITCH3__STATIC:
                        case PULLSWITCH__STATIC:
                        case SLOTDOOR__STATIC:
                        case LIGHTSWITCH__STATIC:
                        case SPACELIGHTSWITCH__STATIC:
                        case SPACEDOORSWITCH__STATIC:
                        case FRANKENSTINESWITCH__STATIC:
                        case POWERSWITCH1__STATIC:
                        case LOCKSWITCH1__STATIC:
                        case POWERSWITCH2__STATIC:
                        case TECHSWITCH__STATIC:
                        case ACCESSSWITCH__STATIC:
                        case ACCESSSWITCH2__STATIC:
                        {
                            int32_t x, y;

                            if ((s->cstat & 32768) || A_CheckSpriteFlags(i, SPRITE_NOLIGHT) ||
                                    !inside(s->x+((sintable[(s->ang+512)&2047])>>9), s->y+((sintable[(s->ang)&2047])>>9), s->sectnum))
                            {
                                if (actor[i].lightptr != NULL)
                                {
                                    polymer_deletelight(actor[i].lightId);
                                    actor[i].lightId = -1;
                                    actor[i].lightptr = NULL;
                                }
                                break;
                            }

                            x = ((sintable[(s->ang+512)&2047])>>7);
                            y = ((sintable[(s->ang)&2047])>>7);

                            s->x += x;
                            s->y += y;

                            G_AddGameLight(0, i, ((s->yrepeat*tilesizy[s->picnum])<<1), 768, 255+(48<<8)+(48<<16),PR_LIGHT_PRIO_LOW);
                            s->x -= x;
                            s->y -= y;
                        }
                        break;
                        }
                    }
                }
#endif
                if (!apScriptGameEvent[EVENT_GAME] || A_CheckSpriteFlags(i, SPRITE_NOEVENTCODE))
                {
                    i = nextspritestat[i];
                    continue;
                }

                j = nextspritestat[i];

                pl = A_FindPlayer(&sprite[i],&p);
                VM_OnEvent(EVENT_GAME,i, pl, p);
                i = j;
            }
        }
        while (k--);
    }

    G_DoSectorAnimations();
    G_MoveFX();               //ST 11
}

