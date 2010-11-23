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

#include <time.h>
#include <stdlib.h>
#include <math.h>

#include "duke3d.h"
#include "gamedef.h"
#include "gameexec.h"
#include "scriplib.h"
#include "savegame.h"
#include "premap.h"
#include "osdcmds.h"
#include "osd.h"
#include "menus.h"

#if KRANDDEBUG
# define GAMEEXEC_INLINE
# define GAMEEXEC_STATIC
#else
# define GAMEEXEC_INLINE inline
# define GAMEEXEC_STATIC static
#endif

void G_RestoreMapState(mapstate_t *save);
void G_SaveMapState(mapstate_t *save);

vmstate_t vm;

int32_t g_errorLineNum;
int32_t g_tw;
extern int32_t ticrandomseed;

GAMEEXEC_STATIC int32_t VM_Execute(int32_t once);

#include "gamestructures.c"

void VM_ScriptInfo(void)
{
    intptr_t *p;

    if (!script)
        return;

    if (insptr)
    {
        for (p=insptr-20; p<insptr+20; p++)
        {
            if (*p>>12&&(*p&0xFFF)<CON_END)
                initprintf("\n%5d: %5d %s ",p-script,*p>>12,keyw[*p&0xFFF]);
            else
                initprintf(" %d",*p);
        }

        initprintf("\n");
    }

    if (vm.g_i)
        initprintf("current actor: %d (%d)\n",vm.g_i,vm.g_sp->picnum);

    initprintf("g_errorLineNum: %d, g_tw: %d\n",g_errorLineNum,g_tw);
}

void VM_OnEvent(register int32_t iEventID, register int32_t iActor, register int32_t iPlayer, register int32_t lDist)
{
    if (iEventID < 0 || iEventID >= MAXGAMEEVENTS || apScriptGameEvent[iEventID] == 0)
        return;

    {
        intptr_t *oinsptr=insptr;
        vmstate_t vm_backup;
        vmstate_t tempvm = { iActor, iPlayer, lDist, &actor[iActor].t_data[0],
                             &sprite[iActor], 0
                           };

        Bmemcpy(&vm_backup, &vm, sizeof(vmstate_t));
        Bmemcpy(&vm, &tempvm, sizeof(vmstate_t));

        insptr = apScriptGameEvent[iEventID];

        VM_Execute(0);

        if (vm.g_flags & VM_KILL)
        {
            // if player was set to squish, first stop that...
            if (vm.g_p >= 0)
            {
                if (g_player[vm.g_p].ps->actorsqu == vm.g_i)
                    g_player[vm.g_p].ps->actorsqu = -1;
            }
            deletesprite(vm.g_i);
        }

        Bmemcpy(&vm, &vm_backup, sizeof(vmstate_t));
        insptr=oinsptr;
    }
}

static int32_t VM_CheckSquished(void)
{
    sectortype *sc = &sector[vm.g_sp->sectnum];
    int32_t squishme = 0;

    if ((vm.g_sp->picnum == APLAYER && ud.clipping) || sc->lotag == 23)
        return 0;

    squishme = (sc->floorz - sc->ceilingz < (12<<8)); // && (sc->lotag&32768) == 0;

    if (vm.g_sp->pal == 1)
        squishme = (sc->floorz - sc->ceilingz < (32<<8) && (sc->lotag&32768) == 0);

    if (!squishme)
        return 0;

    P_DoQuote(10, g_player[vm.g_p].ps);

    if (A_CheckEnemySprite(vm.g_sp)) vm.g_sp->xvel = 0;

    if (vm.g_sp->pal == 1)
    {
        actor[vm.g_i].picnum = SHOTSPARK1;
        actor[vm.g_i].extra = 1;
        return 0;
    }

    return 1;
}

GAMEEXEC_STATIC GAMEEXEC_INLINE void P_ForceAngle(DukePlayer_t *p)
{
    int32_t n = 128-(krand()&255);

    p->horiz += 64;
    p->return_to_center = 9;
    p->look_ang = n>>1;
    p->rotscrnang = n>>1;
}

GAMEEXEC_STATIC int32_t A_Dodge(spritetype *s)
{
    int32_t bx,by,bxvect,byvect,d,i;
    int32_t mx = s->x, my = s->y;
    int32_t mxvect = sintable[(s->ang+512)&2047];
    int32_t myvect = sintable[s->ang&2047];

    if (A_CheckEnemySprite(s) && s->extra <= 0) // hack
        return 0;

    for (i=headspritestat[STAT_PROJECTILE]; i>=0; i=nextspritestat[i]) //weapons list
    {
        if (OW == i/* || SECT != s->sectnum*/)
            continue;

        bx = SX-mx;
        by = SY-my;
        bxvect = sintable[(SA+512)&2047];
        byvect = sintable[SA&2047];

        if (mxvect *bx + myvect *by >= 0)
            if (bxvect*bx + byvect*by < 0)
            {
                d = bxvect*by - byvect*bx;
                if (klabs(d) < 65536*64)
                {
                    s->ang -= 512+(krand()&1024);
                    return 1;
                }
            }
    }
    return 0;
}

int32_t A_GetFurthestAngle(int32_t iActor,int32_t angs)
{
    spritetype *s = &sprite[iActor];

    if (s->picnum != APLAYER && (actor[iActor].t_data[0]&63) > 2)
        return(s->ang + 1024);

    {
        int32_t furthest_angle=0;
        int32_t d;
        int32_t greatestd = -(1<<30);
        int32_t angincs = 2048/angs,j;
        hitdata_t hitinfo;

        for (j=s->ang; j<(2048+s->ang); j+=angincs)
        {
            s->z -= (8<<8);
            hitscan((const vec3_t *)s, s->sectnum,
                    sintable[(j+512)&2047],
                    sintable[j&2047],0,
                    &hitinfo,CLIPMASK1);
            s->z += (8<<8);
            d = klabs(hitinfo.pos.x-s->x) + klabs(hitinfo.pos.y-s->y);

            if (d > greatestd)
            {
                greatestd = d;
                furthest_angle = j;
            }
        }
        return (furthest_angle&2047);
    }
}

int32_t A_FurthestVisiblePoint(int32_t iActor,spritetype *ts,int32_t *dax,int32_t *day)
{
    if ((actor[iActor].t_data[0]&63)) return -1;
    {
        int32_t d, da;//, d, cd, ca,tempx,tempy,cx,cy;
        int32_t j, angincs;
        spritetype *s = &sprite[iActor];
        hitdata_t hitinfo;

        if ((!g_netServer && ud.multimode < 2) && ud.player_skill < 3)
            angincs = 2048/2;
        else angincs = 2048/(1+(krand()&1));

        for (j=ts->ang; j<(2048+ts->ang); j+=(angincs-(krand()&511)))
        {
            ts->z -= (16<<8);
            hitscan((const vec3_t *)ts, ts->sectnum,
                    sintable[(j+512)&2047],
                    sintable[j&2047],16384-(krand()&32767),
                    &hitinfo,CLIPMASK1);

            ts->z += (16<<8);

            d = klabs(hitinfo.pos.x-ts->x)+klabs(hitinfo.pos.y-ts->y);
            da = klabs(hitinfo.pos.x-s->x)+klabs(hitinfo.pos.y-s->y);

            if (d < da && hitinfo.hitsect > -1)
                if (cansee(hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,
                           hitinfo.hitsect,s->x,s->y,s->z-(16<<8),s->sectnum))
                {
                    *dax = hitinfo.pos.x;
                    *day = hitinfo.pos.y;
                    return hitinfo.hitsect;
                }
        }
        return -1;
    }
}

void A_GetZLimits(int32_t iActor)
{
    spritetype *s = &sprite[iActor];

//    if (s->statnum == STAT_PLAYER || s->statnum == STAT_STANDABLE || s->statnum == STAT_ZOMBIEACTOR || s->statnum == STAT_ACTOR || s->statnum == STAT_PROJECTILE)
    {
        int32_t hz,lz,zr = 127L;
        int32_t cstat = s->cstat;

        s->cstat = 0;

        if (s->statnum == STAT_PROJECTILE)
            zr = 4L;

        s->z -= ZOFFSET;
        getzrange((vec3_t *)s,s->sectnum,&actor[iActor].ceilingz,&hz,&actor[iActor].floorz,&lz,zr,CLIPMASK0);
        s->z += ZOFFSET;

        s->cstat = cstat;

        if ((lz&49152) == 49152 && (sprite[lz&(MAXSPRITES-1)].cstat&48) == 0)
        {
            lz &= (MAXSPRITES-1);
            if (A_CheckEnemySprite(&sprite[lz]) && sprite[lz].pal != 1)
            {
                if (s->statnum != 4)
                {
                    actor[iActor].dispicnum = -4; // No shadows on actors
                    s->xvel = -256;
                    A_SetSprite(iActor,CLIPMASK0);
                }
            }
            else if (sprite[lz].picnum == APLAYER && A_CheckEnemySprite(s))
            {
                actor[iActor].dispicnum = -4; // No shadows on actors
                s->xvel = -256;
                A_SetSprite(iActor,CLIPMASK0);
            }
            else if (s->statnum == STAT_PROJECTILE && sprite[lz].picnum == APLAYER)
                if (s->owner == lz)
                {
                    actor[iActor].ceilingz = sector[s->sectnum].ceilingz;
                    actor[iActor].floorz   = sector[s->sectnum].floorz;
                }
        }
    }
    /*
        else
        {
            actor[iActor].ceilingz = sector[s->sectnum].ceilingz;
            actor[iActor].floorz   = sector[s->sectnum].floorz;
        }
    */
}

void A_Fall(int32_t iActor)
{
    spritetype *s = &sprite[iActor];
    int32_t hz,lz,c = g_spriteGravity;

    if (G_CheckForSpaceFloor(s->sectnum))
        c = 0;
    else
    {
        if (G_CheckForSpaceCeiling(s->sectnum) || sector[s->sectnum].lotag == 2)
            c = g_spriteGravity/6;
    }

    if (s->statnum == STAT_ACTOR || s->statnum == STAT_PLAYER || s->statnum == STAT_ZOMBIEACTOR || s->statnum == STAT_STANDABLE)
    {
        int32_t cstat = s->cstat;
        s->cstat = 0;
        s->z -= ZOFFSET;
        getzrange((vec3_t *)s,s->sectnum,&actor[iActor].ceilingz,&hz,&actor[iActor].floorz,&lz,127L,CLIPMASK0);
        s->z += ZOFFSET;
        s->cstat = cstat;
    }
    else
    {
        actor[iActor].ceilingz = sector[s->sectnum].ceilingz;
        actor[iActor].floorz   = sector[s->sectnum].floorz;
    }

    if (s->z < actor[iActor].floorz-(ZOFFSET))
    {
        if (sector[s->sectnum].lotag == 2 && s->zvel > 3122)
            s->zvel = 3144;
        s->z += s->zvel = min(6144, s->zvel+c);
    }
    if (s->z >= actor[iActor].floorz-(ZOFFSET))
    {
        s->z = actor[iActor].floorz - ZOFFSET;
        s->zvel = 0;
    }
}

int32_t G_GetAngleDelta(int32_t a,int32_t na)
{
    a &= 2047;
    na &= 2047;

    if (klabs(a-na) < 1024)
    {
//        OSD_Printf("G_GetAngleDelta() returning %d\n",na-a);
        return (na-a);
    }

    if (na > 1024) na -= 2048;
    if (a > 1024) a -= 2048;

    na -= 2048;
    a -= 2048;
//    OSD_Printf("G_GetAngleDelta() returning %d\n",na-a);
    return (na-a);
}

GAMEEXEC_STATIC GAMEEXEC_INLINE void VM_AlterAng(int32_t a)
{
    intptr_t *moveptr;
    int32_t ticselapsed = (vm.g_t[0])&31;

    if ((moveptr = (intptr_t *)vm.g_t[1]) < &script[0] || moveptr > (&script[0]+g_scriptSize))
    {
        vm.g_t[1] = 0;
        OSD_Printf(OSD_ERROR "bad moveptr for actor %d (%d)!\n", vm.g_i, vm.g_sp->picnum);
        return;
    }

    vm.g_sp->xvel += (*moveptr-vm.g_sp->xvel)/5;
    if (vm.g_sp->zvel < 648) vm.g_sp->zvel += ((*(moveptr+1)<<4)-vm.g_sp->zvel)/5;

    if (A_CheckEnemySprite(vm.g_sp) && vm.g_sp->extra <= 0) // hack
        return;

    if (a&seekplayer)
    {
        int32_t aang = vm.g_sp->ang, angdif, goalang;
        int32_t j = g_player[vm.g_p].ps->holoduke_on;

        // NOTE: looks like 'owner' is set to target sprite ID...

        if (j >= 0 && cansee(sprite[j].x,sprite[j].y,sprite[j].z,sprite[j].sectnum,vm.g_sp->x,vm.g_sp->y,vm.g_sp->z,vm.g_sp->sectnum))
            vm.g_sp->owner = j;
        else vm.g_sp->owner = g_player[vm.g_p].ps->i;

        if (sprite[vm.g_sp->owner].picnum == APLAYER)
            goalang = getangle(actor[vm.g_i].lastvx-vm.g_sp->x,actor[vm.g_i].lastvy-vm.g_sp->y);
        else
            goalang = getangle(sprite[vm.g_sp->owner].x-vm.g_sp->x,sprite[vm.g_sp->owner].y-vm.g_sp->y);

        if (vm.g_sp->xvel && vm.g_sp->picnum != DRONE)
        {
            angdif = G_GetAngleDelta(aang,goalang);

            if (ticselapsed < 2)
            {
                if (klabs(angdif) < 256)
                {
                    j = 128-(krand()&256);
                    vm.g_sp->ang += j;
                    if (A_GetHitscanRange(vm.g_i) < 844)
                        vm.g_sp->ang -= j;
                }
            }
            else if (ticselapsed > 18 && ticselapsed < GAMETICSPERSEC) // choose
            {
                if (klabs(angdif>>2) < 128) vm.g_sp->ang = goalang;
                else vm.g_sp->ang += angdif>>2;
            }
        }
        else vm.g_sp->ang = goalang;
    }

    if (ticselapsed < 1)
    {
        int32_t j = 2;
        if (a&furthestdir)
        {
            vm.g_sp->ang = A_GetFurthestAngle(vm.g_i,j);
            vm.g_sp->owner = g_player[vm.g_p].ps->i;
        }

        if (a&fleeenemy)
        {
            vm.g_sp->ang = A_GetFurthestAngle(vm.g_i,j); // += angdif; //  = G_GetAngleDelta(aang,goalang)>>1;
        }
    }
}

GAMEEXEC_STATIC void VM_Move(void)
{
    int32_t l;
    intptr_t *moveptr;
    int32_t a = vm.g_sp->hitag, goalang, angdif;
    int32_t daxvel;
    int32_t deadflag = (A_CheckEnemySprite(vm.g_sp) && vm.g_sp->extra <= 0);

    if (a == -1) a = 0;

    vm.g_t[0]++;

    if (vm.g_t[1] == 0 || a == 0)
    {
        if (deadflag || (actor[vm.g_i].bposx != vm.g_sp->x) || (actor[vm.g_i].bposy != vm.g_sp->y))
        {
            actor[vm.g_i].bposx = vm.g_sp->x;
            actor[vm.g_i].bposy = vm.g_sp->y;
            setsprite(vm.g_i,(vec3_t *)vm.g_sp);
        }
        return;
    }

    if (a&face_player && !deadflag)
    {
        if (g_player[vm.g_p].ps->newowner >= 0)
            goalang = getangle(g_player[vm.g_p].ps->opos.x-vm.g_sp->x,g_player[vm.g_p].ps->opos.y-vm.g_sp->y);
        else goalang = getangle(g_player[vm.g_p].ps->pos.x-vm.g_sp->x,g_player[vm.g_p].ps->pos.y-vm.g_sp->y);
        angdif = G_GetAngleDelta(vm.g_sp->ang,goalang)>>2;
        if ((angdif > -8 && angdif < 0) || (angdif < 8 && angdif > 0))
            angdif *= 2;
        vm.g_sp->ang += angdif;
    }

    if (a&spin && !deadflag)
        vm.g_sp->ang += sintable[((vm.g_t[0]<<3)&2047)]>>6;

    if (a&face_player_slow && !deadflag)
    {
        if (g_player[vm.g_p].ps->newowner >= 0)
            goalang = getangle(g_player[vm.g_p].ps->opos.x-vm.g_sp->x,g_player[vm.g_p].ps->opos.y-vm.g_sp->y);
        else goalang = getangle(g_player[vm.g_p].ps->pos.x-vm.g_sp->x,g_player[vm.g_p].ps->pos.y-vm.g_sp->y);
        angdif = G_GetAngleDelta(vm.g_sp->ang,goalang)>>4;
        if ((angdif > -8 && angdif < 0) || (angdif < 8 && angdif > 0))
            angdif *= 2;
        vm.g_sp->ang += angdif;
    }

    if (((a&jumptoplayer) == jumptoplayer) && !deadflag)
    {
        if (vm.g_t[0] < 16)
            vm.g_sp->zvel -= (sintable[(512+(vm.g_t[0]<<4))&2047]>>5);
    }

    if (a&face_player_smart && !deadflag)
    {
        int32_t newx = g_player[vm.g_p].ps->pos.x+(g_player[vm.g_p].ps->posvel.x/768);
        int32_t newy = g_player[vm.g_p].ps->pos.y+(g_player[vm.g_p].ps->posvel.y/768);

        goalang = getangle(newx-vm.g_sp->x,newy-vm.g_sp->y);
        angdif = G_GetAngleDelta(vm.g_sp->ang,goalang)>>2;
        if ((angdif > -8 && angdif < 0) || (angdif < 8 && angdif > 0))
            angdif *= 2;
        vm.g_sp->ang += angdif;
    }

    if ((moveptr = (intptr_t *)vm.g_t[1]) >= &script[0] && moveptr <= (&script[0]+g_scriptSize))
    {
        if (a&geth) vm.g_sp->xvel += ((*moveptr)-vm.g_sp->xvel)>>1;
        if (a&getv) vm.g_sp->zvel += ((*(moveptr+1)<<4)-vm.g_sp->zvel)>>1;
    }
    else
    {
        vm.g_t[1] = 0;
        OSD_Printf(OSD_ERROR "bad moveptr for actor %d (%d)!\n", vm.g_i, vm.g_sp->picnum);
        return;
    }

    if (a&dodgebullet && !deadflag)
        A_Dodge(vm.g_sp);

    if (vm.g_sp->picnum != APLAYER)
        VM_AlterAng(a);

    if (vm.g_sp->xvel > -6 && vm.g_sp->xvel < 6) vm.g_sp->xvel = 0;

    a = A_CheckEnemySprite(vm.g_sp);

    if (vm.g_sp->xvel || vm.g_sp->zvel)
    {
        if (a && vm.g_sp->picnum != ROTATEGUN)
        {
            if ((vm.g_sp->picnum == DRONE || vm.g_sp->picnum == COMMANDER) && vm.g_sp->extra > 0)
            {
                if (vm.g_sp->picnum == COMMANDER)
                {
                    actor[vm.g_i].floorz = l = getflorzofslope(vm.g_sp->sectnum,vm.g_sp->x,vm.g_sp->y);
                    if (vm.g_sp->z > (l-(8<<8)))
                    {
                        if (vm.g_sp->z > (l-(8<<8))) vm.g_sp->z = l-(8<<8);
                        vm.g_sp->zvel = 0;
                    }

                    actor[vm.g_i].ceilingz = l = getceilzofslope(vm.g_sp->sectnum,vm.g_sp->x,vm.g_sp->y);
                    if ((vm.g_sp->z-l) < (80<<8))
                    {
                        vm.g_sp->z = l+(80<<8);
                        vm.g_sp->zvel = 0;
                    }
                }
                else
                {
                    if (vm.g_sp->zvel > 0)
                    {
                        actor[vm.g_i].floorz = l = getflorzofslope(vm.g_sp->sectnum,vm.g_sp->x,vm.g_sp->y);
                        if (vm.g_sp->z > (l-(30<<8)))
                            vm.g_sp->z = l-(30<<8);
                    }
                    else
                    {
                        actor[vm.g_i].ceilingz = l = getceilzofslope(vm.g_sp->sectnum,vm.g_sp->x,vm.g_sp->y);
                        if ((vm.g_sp->z-l) < (50<<8))
                        {
                            vm.g_sp->z = l+(50<<8);
                            vm.g_sp->zvel = 0;
                        }
                    }
                }
            }
            else if (vm.g_sp->picnum != ORGANTIC)
            {
                if (vm.g_sp->zvel > 0 && actor[vm.g_i].floorz < vm.g_sp->z)
                    vm.g_sp->z = actor[vm.g_i].floorz;
                if (vm.g_sp->zvel < 0)
                {
                    l = getceilzofslope(vm.g_sp->sectnum,vm.g_sp->x,vm.g_sp->y);
                    if ((vm.g_sp->z-l) < (66<<8))
                    {
                        vm.g_sp->z = l+(66<<8);
                        vm.g_sp->zvel >>= 1;
                    }
                }
            }
        }
        else if (vm.g_sp->picnum == APLAYER)
            if ((vm.g_sp->z-actor[vm.g_i].ceilingz) < (32<<8))
                vm.g_sp->z = actor[vm.g_i].ceilingz+(32<<8);

        daxvel = vm.g_sp->xvel;
        angdif = vm.g_sp->ang;

        if (a && vm.g_sp->picnum != ROTATEGUN)
        {
            if (vm.g_x < 960 && vm.g_sp->xrepeat > 16)
            {

                daxvel = -(1024-vm.g_x);
                angdif = getangle(g_player[vm.g_p].ps->pos.x-vm.g_sp->x,g_player[vm.g_p].ps->pos.y-vm.g_sp->y);

                if (vm.g_x < 512)
                {
                    g_player[vm.g_p].ps->posvel.x = 0;
                    g_player[vm.g_p].ps->posvel.y = 0;
                }
                else
                {
                    g_player[vm.g_p].ps->posvel.x = mulscale(g_player[vm.g_p].ps->posvel.x,g_player[vm.g_p].ps->runspeed-0x2000,16);
                    g_player[vm.g_p].ps->posvel.y = mulscale(g_player[vm.g_p].ps->posvel.y,g_player[vm.g_p].ps->runspeed-0x2000,16);
                }
            }
            else if (vm.g_sp->picnum != DRONE && vm.g_sp->picnum != SHARK && vm.g_sp->picnum != COMMANDER)
            {
                if (actor[vm.g_i].bposz != vm.g_sp->z || ((!g_netServer && ud.multimode < 2) && ud.player_skill < 2))
                {
                    if ((vm.g_t[0]&1) || g_player[vm.g_p].ps->actorsqu == vm.g_i) return;
                    else daxvel <<= 1;
                }
                else
                {
                    if ((vm.g_t[0]&3) || g_player[vm.g_p].ps->actorsqu == vm.g_i) return;
                    else daxvel <<= 2;
                }
            }
        }

        {
            vec3_t tmpvect = { (daxvel*(sintable[(angdif+512)&2047]))>>14,
                               (daxvel*(sintable[angdif&2047]))>>14, vm.g_sp->zvel
                             };

            actor[vm.g_i].movflag = A_MoveSprite(vm.g_i,&tmpvect,CLIPMASK0);
        }
    }

    if (!a) return;

    if (sector[vm.g_sp->sectnum].ceilingstat&1)
        vm.g_sp->shade += (sector[vm.g_sp->sectnum].ceilingshade-vm.g_sp->shade)>>1;
    else vm.g_sp->shade += (sector[vm.g_sp->sectnum].floorshade-vm.g_sp->shade)>>1;

// wtf?
    /*
        if (sector[vm.g_sp->sectnum].floorpicnum == MIRROR)
            deletesprite(vm.g_i);
    */
}

GAMEEXEC_STATIC GAMEEXEC_INLINE void __fastcall VM_DoConditional(register int32_t condition)
{
    if (condition || ((insptr = (intptr_t *)*(insptr+1)) && (((*insptr) & 0xfff) == CON_ELSE)))
    {
        // skip 'else' pointer.. and...
        insptr += 2;
        VM_Execute(1);
        return;
    }
}

GAMEEXEC_STATIC int32_t VM_Execute(int32_t once)
{
    register int32_t tw = *insptr;

    // jump directly into the loop, saving us from the checks during the first iteration
    goto skip_check;

    while (!once)
    {
        if (vm.g_flags & (VM_RETURN|VM_KILL|VM_NOEXECUTE))
            return 1;

        tw = *insptr;

skip_check:
        //      Bsprintf(g_szBuf,"Parsing: %d",*insptr);
        //      AddLog(g_szBuf);

        g_errorLineNum = tw>>12;
        g_tw = tw &= 0xFFF;

        switch (tw)
        {
        case CON_REDEFINEQUOTE:
            insptr++;
            {
                int32_t q = *insptr++, i = *insptr++;
                if ((ScriptQuotes[q] == NULL || ScriptQuoteRedefinitions[i] == NULL))
                {
                    OSD_Printf(CON_ERROR "%s %d null quote\n",g_errorLineNum,keyw[g_tw],q,i);
                    break;
                }
                Bstrcpy(ScriptQuotes[q],ScriptQuoteRedefinitions[i]);
                continue;
            }

        case CON_GETTHISPROJECTILE:
        case CON_SETTHISPROJECTILE:
            insptr++;
            {
                // syntax [gs]etplayer[<var>].x <VAR>
                // <varid> <xxxid> <varid>
                int32_t lVar1=*insptr++, lLabelID=*insptr++, lVar2=*insptr++;

                VM_AccessActiveProjectile(tw==CON_SETTHISPROJECTILE,lVar1,lLabelID,lVar2);
                continue;
            }

        case CON_IFRND:
            VM_DoConditional(rnd(*(++insptr)));
            continue;

        case CON_IFCANSHOOTTARGET:
        {
            int32_t j;
            if (vm.g_x > 1024)
            {
                int16_t temphit, sclip = 768, angdif = 16;

                j = A_CheckHitSprite(vm.g_i,&temphit);

                if (A_CheckEnemySprite(vm.g_sp) && vm.g_sp->xrepeat > 56)
                {
                    sclip = 3084;
                    angdif = 48;
                }

                if (j == (1<<30))
                {
                    VM_DoConditional(1);
                    break;
                }
                if (j > sclip)
                {
                    if (temphit >= 0 && sprite[temphit].picnum == vm.g_sp->picnum)
                        j = 0;
                    else
                    {
                        vm.g_sp->ang += angdif;
                        j = A_CheckHitSprite(vm.g_i,&temphit);
                        vm.g_sp->ang -= angdif;
                        if (j > sclip)
                        {
                            if (temphit >= 0 && sprite[temphit].picnum == vm.g_sp->picnum)
                                j = 0;
                            else
                            {
                                vm.g_sp->ang -= angdif;
                                j = A_CheckHitSprite(vm.g_i,&temphit);
                                vm.g_sp->ang += angdif;
                                if (j > 768)
                                {
                                    if (temphit >= 0 && sprite[temphit].picnum == vm.g_sp->picnum)
                                        j = 0;
                                    else j = 1;
                                }
                                else j = 0;
                            }
                        }
                        else j = 0;
                    }
                }
                else j =  0;
            }
            else j = 1;

            VM_DoConditional(j);
        }
        continue;

        case CON_IFCANSEETARGET:
        {
            int32_t j = cansee(vm.g_sp->x,vm.g_sp->y,vm.g_sp->z-((krand()&41)<<8),
                               vm.g_sp->sectnum,g_player[vm.g_p].ps->pos.x,g_player[vm.g_p].ps->pos.y,
                               g_player[vm.g_p].ps->pos.z/*-((krand()&41)<<8)*/,sprite[g_player[vm.g_p].ps->i].sectnum);
            VM_DoConditional(j);
            if (j) actor[vm.g_i].timetosleep = SLEEPTIME;
        }
        continue;

        case CON_IFACTORNOTSTAYPUT:
            VM_DoConditional(actor[vm.g_i].actorstayput == -1);
            continue;

        case CON_IFCANSEE:
        {
            spritetype *s = &sprite[g_player[vm.g_p].ps->i];
            int32_t j;

            // select sprite for monster to target
            // if holoduke is on, let them target holoduke first.
            //
            if (g_player[vm.g_p].ps->holoduke_on >= 0)
            {
                s = &sprite[g_player[vm.g_p].ps->holoduke_on];
                j = cansee(vm.g_sp->x,vm.g_sp->y,vm.g_sp->z-(krand()&((32<<8)-1)),vm.g_sp->sectnum,
                           s->x,s->y,s->z,s->sectnum);

                if (j == 0)
                {
                    // they can't see player's holoduke
                    // check for player...
                    s = &sprite[g_player[vm.g_p].ps->i];
                }
            }

            // can they see player, (or player's holoduke)
            j = cansee(vm.g_sp->x,vm.g_sp->y,vm.g_sp->z-(krand()&((47<<8))),vm.g_sp->sectnum,
                       s->x,s->y,s->z-(24<<8),s->sectnum);

            if (j == 0)
            {
                // search around for target player

                // also modifies 'target' x&y if found..

                j = 1;
                if (A_FurthestVisiblePoint(vm.g_i,s,&actor[vm.g_i].lastvx,&actor[vm.g_i].lastvy) == -1)
                    j = 0;
            }
            else
            {
                // else, they did see it.
                // save where we were looking...
                actor[vm.g_i].lastvx = s->x;
                actor[vm.g_i].lastvy = s->y;
            }

            if (j && (vm.g_sp->statnum == STAT_ACTOR || vm.g_sp->statnum == STAT_STANDABLE))
                actor[vm.g_i].timetosleep = SLEEPTIME;

            VM_DoConditional(j);
            continue;
        }

        case CON_IFHITWEAPON:
            VM_DoConditional(A_IncurDamage(vm.g_i) >= 0);
            continue;

        case CON_IFSQUISHED:
            VM_DoConditional(VM_CheckSquished());
            continue;

        case CON_IFDEAD:
            //        j = vm.g_sp->extra;
            //        if (vm.g_sp->picnum == APLAYER)
            //            j--;
            VM_DoConditional(vm.g_sp->extra <= 0);
            continue;

        case CON_AI:
            insptr++;
            //Following changed to use pointersizes
            vm.g_t[5] = *insptr++; // Ai
            vm.g_t[4] = *(intptr_t *)(vm.g_t[5]);       // Action
            if (vm.g_t[5]) vm.g_t[1] = *(((intptr_t *)vm.g_t[5])+1);       // move
            vm.g_sp->hitag = *(((intptr_t *)vm.g_t[5])+2);    // move flags
            vm.g_t[0] = vm.g_t[2] = vm.g_t[3] = 0; // count, actioncount... vm.g_t[3] = ??
            if (A_CheckEnemySprite(vm.g_sp) && vm.g_sp->extra <= 0) // hack
                continue;
            if (vm.g_sp->hitag&random_angle)
                vm.g_sp->ang = krand()&2047;
            continue;

        case CON_ACTION:
            insptr++;
            vm.g_t[2] = vm.g_t[3] = 0;
            vm.g_t[4] = *insptr++;
            continue;

        case CON_IFPDISTL:
            insptr++;
            VM_DoConditional(vm.g_x < *insptr);
            if (vm.g_x > MAXSLEEPDIST && actor[vm.g_i].timetosleep == 0)
                actor[vm.g_i].timetosleep = SLEEPTIME;
            continue;

        case CON_IFPDISTG:
            insptr++;
            VM_DoConditional(vm.g_x > *insptr);
            if (vm.g_x > MAXSLEEPDIST && actor[vm.g_i].timetosleep == 0)
                actor[vm.g_i].timetosleep = SLEEPTIME;
            continue;

        case CON_ELSE:
            insptr = (intptr_t *) *(insptr+1);
            continue;

        case CON_ADDSTRENGTH:
            insptr++;
            vm.g_sp->extra += *insptr++;
            continue;

        case CON_STRENGTH:
            insptr++;
            vm.g_sp->extra = *insptr++;
            continue;

        case CON_IFGOTWEAPONCE:
            insptr++;

            if ((GametypeFlags[ud.coop]&GAMETYPE_WEAPSTAY) && (g_netServer || ud.multimode > 1))
            {
                if (*insptr == 0)
                {
                    int32_t j = 0;
                    for (; j < g_player[vm.g_p].ps->weapreccnt; j++)
                        if (g_player[vm.g_p].ps->weaprecs[j] == vm.g_sp->picnum)
                            break;

                    VM_DoConditional(j < g_player[vm.g_p].ps->weapreccnt && vm.g_sp->owner == vm.g_i);
                    continue;
                }
                else if (g_player[vm.g_p].ps->weapreccnt < MAX_WEAPONS)
                {
                    g_player[vm.g_p].ps->weaprecs[g_player[vm.g_p].ps->weapreccnt++] = vm.g_sp->picnum;
                    VM_DoConditional(vm.g_sp->owner == vm.g_i);
                    continue;
                }
            }
            VM_DoConditional(0);
            continue;

        case CON_GETLASTPAL:
            insptr++;
            if (vm.g_sp->picnum == APLAYER)
                vm.g_sp->pal = g_player[vm.g_sp->yvel].ps->palookup;
            else
            {
                if (vm.g_sp->pal == 1 && vm.g_sp->extra == 0) // hack for frozen
                    vm.g_sp->extra++;
                vm.g_sp->pal = actor[vm.g_i].tempang;
            }
            actor[vm.g_i].tempang = 0;
            continue;

        case CON_TOSSWEAPON:
            insptr++;
            P_DropWeapon(g_player[vm.g_sp->yvel].ps);
            continue;

        case CON_NULLOP:
            insptr++;
            continue;

        case CON_MIKESND:
            insptr++;
            if ((vm.g_sp->yvel<0 || vm.g_sp->yvel>=MAXSOUNDS))
            {
                OSD_Printf(CON_ERROR "Invalid sound %d\n",g_errorLineNum,keyw[g_tw],vm.g_sp->yvel);
                insptr++;
                continue;
            }
            if (!S_CheckSoundPlaying(vm.g_i,vm.g_sp->yvel))
                A_PlaySound(vm.g_sp->yvel,vm.g_i);
            continue;

        case CON_PKICK:
            insptr++;

            if ((g_netServer || ud.multimode > 1) && vm.g_sp->picnum == APLAYER)
            {
                if (g_player[otherp].ps->quick_kick == 0)
                    g_player[otherp].ps->quick_kick = 14;
            }
            else if (vm.g_sp->picnum != APLAYER && g_player[vm.g_p].ps->quick_kick == 0)
                g_player[vm.g_p].ps->quick_kick = 14;
            continue;

        case CON_SIZETO:
            insptr++;

            {
                int32_t j = (*insptr++ - vm.g_sp->xrepeat)<<1;
                vm.g_sp->xrepeat += ksgn(j);

                if ((vm.g_sp->picnum == APLAYER && vm.g_sp->yrepeat < 36) || *insptr < vm.g_sp->yrepeat ||
                        ((vm.g_sp->yrepeat*(tilesizy[vm.g_sp->picnum]+8))<<2) < (actor[vm.g_i].floorz - actor[vm.g_i].ceilingz))
                {
                    j = ((*insptr)-vm.g_sp->yrepeat)<<1;
                    if (klabs(j)) vm.g_sp->yrepeat += ksgn(j);
                }
            }
            insptr++;

            continue;

        case CON_SIZEAT:
            insptr++;
            vm.g_sp->xrepeat = (uint8_t) *insptr++;
            vm.g_sp->yrepeat = (uint8_t) *insptr++;
            continue;

        case CON_SHOOT:
            insptr++;
            A_Shoot(vm.g_i,*insptr++);
            continue;

        case CON_SOUNDONCE:
            insptr++;
            if ((*insptr<0 || *insptr>=MAXSOUNDS))
            {
                OSD_Printf(CON_ERROR "Invalid sound %d\n",g_errorLineNum,keyw[g_tw],*insptr++);
                continue;
            }
            if (!S_CheckSoundPlaying(vm.g_i,*insptr++))
                A_PlaySound(*(insptr-1),vm.g_i);
            continue;

        case CON_IFACTORSOUND:
            insptr++;
            {
                int32_t i = Gv_GetVarX(*insptr++), j = Gv_GetVarX(*insptr++);

                if ((j<0 || j>=MAXSOUNDS))
                {
                    OSD_Printf(CON_ERROR "Invalid sound %d\n",g_errorLineNum,keyw[g_tw],j);
                    insptr++;
                    continue;
                }
                insptr--;
                VM_DoConditional(A_CheckSoundPlaying(i,j));
            }
            continue;

        case CON_IFSOUND:
            insptr++;
            if ((*insptr<0 || *insptr>=MAXSOUNDS))
            {
                OSD_Printf(CON_ERROR "Invalid sound %d\n",g_errorLineNum,keyw[g_tw],*insptr);
                insptr++;
                continue;
            }
            VM_DoConditional(S_CheckSoundPlaying(vm.g_i,*insptr));
            //    VM_DoConditional(SoundOwner[*insptr][0].i == vm.g_i);
            continue;

        case CON_STOPSOUND:
            insptr++;
            if ((*insptr<0 || *insptr>=MAXSOUNDS))
            {
                OSD_Printf(CON_ERROR "Invalid sound %d\n",g_errorLineNum,keyw[g_tw],*insptr);
                insptr++;
                continue;
            }
            if (S_CheckSoundPlaying(vm.g_i,*insptr))
                S_StopSound((int16_t)*insptr);
            insptr++;
            continue;

        case CON_STOPACTORSOUND:
            insptr++;
            {
                int32_t i = Gv_GetVarX(*insptr++), j = Gv_GetVarX(*insptr++);

                if ((j<0 || j>=MAXSOUNDS))
                {
                    OSD_Printf(CON_ERROR "Invalid sound %d\n",g_errorLineNum,keyw[g_tw],j);
                    continue;
                }

                if (A_CheckSoundPlaying(i,j))
                    S_StopEnvSound(j,i);

                continue;
            }

        case CON_GLOBALSOUND:
            insptr++;
            if ((*insptr<0 || *insptr>=MAXSOUNDS))
            {
                OSD_Printf(CON_ERROR "Invalid sound %d\n",g_errorLineNum,keyw[g_tw],*insptr);
                insptr++;
                continue;
            }
            if (vm.g_p == screenpeek || (GametypeFlags[ud.coop]&GAMETYPE_COOPSOUND))
                A_PlaySound(*insptr,g_player[screenpeek].ps->i);
            insptr++;
            continue;

        case CON_SOUND:
            insptr++;
            if ((*insptr<0 || *insptr>=MAXSOUNDS))
            {
                OSD_Printf(CON_ERROR "Invalid sound %d\n",g_errorLineNum,keyw[g_tw],*insptr);
                insptr++;
                continue;
            }
            A_PlaySound(*insptr++,vm.g_i);
            continue;

        case CON_TIP:
            insptr++;
            g_player[vm.g_p].ps->tipincs = GAMETICSPERSEC;
            continue;

        case CON_FALL:
            insptr++;
            vm.g_sp->xoffset = vm.g_sp->yoffset = 0;

            {
                int32_t j = g_spriteGravity;

                if (G_CheckForSpaceCeiling(vm.g_sp->sectnum) || sector[vm.g_sp->sectnum].lotag == 2)
                    j = g_spriteGravity/6;
                else if (G_CheckForSpaceFloor(vm.g_sp->sectnum))
                    j = 0;

                if (!actor[vm.g_i].cgg-- || (sector[vm.g_sp->sectnum].floorstat&2))
                {
                    A_GetZLimits(vm.g_i);
                    actor[vm.g_i].cgg = 3;
                }

                if (vm.g_sp->z < (actor[vm.g_i].floorz-ZOFFSET))
                {
                    vm.g_sp->z += vm.g_sp->zvel = min(6144, vm.g_sp->zvel+j);

                    if (vm.g_sp->z > (actor[vm.g_i].floorz - ZOFFSET))
                        vm.g_sp->z = (actor[vm.g_i].floorz - ZOFFSET);
                    continue;
                }
                vm.g_sp->z = actor[vm.g_i].floorz - ZOFFSET;

                if (A_CheckEnemySprite(vm.g_sp) || (vm.g_sp->picnum == APLAYER && vm.g_sp->owner >= 0))
                {
                    if (vm.g_sp->zvel > 3084 && vm.g_sp->extra <= 1)
                    {
                        // I'm guessing this DRONE check is from a beta version of the game
                        // where they crashed into the ground when killed
                        if (!(vm.g_sp->picnum == APLAYER && vm.g_sp->extra > 0) && vm.g_sp->pal != 1 && vm.g_sp->picnum != DRONE)
                        {
                            A_DoGuts(vm.g_i,JIBS6,15);
                            A_PlaySound(SQUISHED,vm.g_i);
                            A_Spawn(vm.g_i,BLOODPOOL);
                        }
                        actor[vm.g_i].picnum = SHOTSPARK1;
                        actor[vm.g_i].extra = 1;
                        vm.g_sp->zvel = 0;
                    }
                    else if (vm.g_sp->zvel > 2048  && sector[vm.g_sp->sectnum].lotag != 1)
                    {
                        j = vm.g_sp->sectnum;
                        pushmove((vec3_t *)vm.g_sp,(int16_t *)&j,128L,(4L<<8),(4L<<8),CLIPMASK0);
                        if (j != vm.g_sp->sectnum && j >= 0 && j < MAXSECTORS)
                            changespritesect(vm.g_i,j);
                        A_PlaySound(THUD,vm.g_i);
                    }
                }
            }

            if (vm.g_sp->z > (actor[vm.g_i].floorz - ZOFFSET))
            {
                A_GetZLimits(vm.g_i);
                if (actor[vm.g_i].floorz != sector[vm.g_sp->sectnum].floorz)
                    vm.g_sp->z = (actor[vm.g_i].floorz - ZOFFSET);
                continue;
            }
            else if (sector[vm.g_sp->sectnum].lotag == 1)
            {
                switch (DynamicTileMap[vm.g_sp->picnum])
                {
                default:
                    // fix for flying/jumping monsters getting stuck in water
                {
                    intptr_t *moveptr = (intptr_t *)vm.g_t[1];
                    if (vm.g_sp->hitag & jumptoplayer || (actorscrptr[vm.g_sp->picnum] &&
                                                          moveptr >= &script[0] && moveptr <= (&script[0]+g_scriptSize) && *(moveptr+1)))
                    {
                        //                    OSD_Printf("%d\n",*(moveptr+1));
                        break;
                    }
                }
                //                OSD_Printf("hitag: %d\n",vm.g_sp->hitag);
                vm.g_sp->z += (24<<8);
                case OCTABRAIN__STATIC:
                case COMMANDER__STATIC:
                case DRONE__STATIC:
                    break;
                }
                continue;
            }
            vm.g_sp->zvel = 0;
            continue;

        case CON_RETURN:
            vm.g_flags |= VM_RETURN;
        case CON_ENDA:
        case CON_BREAK:
        case CON_ENDS:
            return 1;
        case CON_RIGHTBRACE:
            insptr++;
            return 1;
        case CON_ADDAMMO:
            insptr++;
            if ((*insptr<0 || *insptr>=MAX_WEAPONS))
            {
                OSD_Printf(CON_ERROR "Invalid weapon ID %d\n",g_errorLineNum,keyw[g_tw],*insptr);
                insptr += 2; break;
            }
            if (g_player[vm.g_p].ps->ammo_amount[*insptr] >= g_player[vm.g_p].ps->max_ammo_amount[*insptr])
            {
                vm.g_flags |= VM_NOEXECUTE;
                break;
            }
            P_AddAmmo(*insptr, g_player[vm.g_p].ps, *(insptr+1));
            if (g_player[vm.g_p].ps->curr_weapon == KNEE_WEAPON && g_player[vm.g_p].ps->gotweapon & (1 << *insptr))
            {
                if (!(g_player[vm.g_p].ps->weaponswitch & 1))
                    P_AddWeaponNoSwitch(g_player[vm.g_p].ps, *insptr);
                else P_AddWeapon(g_player[vm.g_p].ps, *insptr);
            }
            insptr += 2;
            continue;

        case CON_MONEY:
            insptr++;
            A_SpawnMultiple(vm.g_i, MONEY, *insptr++);
            continue;

        case CON_MAIL:
            insptr++;
            A_SpawnMultiple(vm.g_i, MAIL, *insptr++);
            continue;

        case CON_SLEEPTIME:
            insptr++;
            actor[vm.g_i].timetosleep = (int16_t)*insptr++;
            continue;

        case CON_PAPER:
            insptr++;
            A_SpawnMultiple(vm.g_i, PAPER, *insptr++);
            continue;

        case CON_ADDKILLS:
            insptr++;
            g_player[vm.g_p].ps->actors_killed += *insptr++;
            actor[vm.g_i].actorstayput = -1;
            continue;

        case CON_LOTSOFGLASS:
            insptr++;
            A_SpawnGlass(vm.g_i,*insptr++);
            continue;

        case CON_KILLIT:
            insptr++;
            vm.g_flags |= VM_KILL;
            continue;

        case CON_ADDWEAPON:
            insptr++;
            if ((*insptr<0 ||*insptr>=MAX_WEAPONS))
            {
                OSD_Printf(CON_ERROR "Invalid weapon ID %d\n",g_errorLineNum,keyw[g_tw],*insptr);
                insptr += 2;
                continue;
            }
            if ((g_player[vm.g_p].ps->gotweapon & (1 << *insptr)) == 0)
            {
                if (!(g_player[vm.g_p].ps->weaponswitch & 1)) P_AddWeaponNoSwitch(g_player[vm.g_p].ps, *insptr);
                else P_AddWeapon(g_player[vm.g_p].ps, *insptr);
            }
            else if (g_player[vm.g_p].ps->ammo_amount[*insptr] >= g_player[vm.g_p].ps->max_ammo_amount[*insptr])
            {
                vm.g_flags |= VM_NOEXECUTE;
                continue;
            }
            P_AddAmmo(*insptr, g_player[vm.g_p].ps, *(insptr+1));
            if (g_player[vm.g_p].ps->curr_weapon == KNEE_WEAPON && g_player[vm.g_p].ps->gotweapon & (1 << *insptr))
            {
                if (!(g_player[vm.g_p].ps->weaponswitch & 1)) P_AddWeaponNoSwitch(g_player[vm.g_p].ps, *insptr);
                else P_AddWeapon(g_player[vm.g_p].ps, *insptr);
            }
            insptr += 2;
            continue;

        case CON_DEBUG:
            insptr++;
            initprintf("%d\n",*insptr++);
            continue;

        case CON_ENDOFGAME:
            insptr++;
            g_player[vm.g_p].ps->timebeforeexit = *insptr++;
            g_player[vm.g_p].ps->customexitsound = -1;
            ud.eog = 1;
            continue;

        case CON_ADDPHEALTH:
            insptr++;

            {
                int32_t j;

                if (g_player[vm.g_p].ps->newowner >= 0)
                {
                    g_player[vm.g_p].ps->newowner = -1;
                    g_player[vm.g_p].ps->pos.x = g_player[vm.g_p].ps->opos.x;
                    g_player[vm.g_p].ps->pos.y = g_player[vm.g_p].ps->opos.y;
                    g_player[vm.g_p].ps->pos.z = g_player[vm.g_p].ps->opos.z;
                    g_player[vm.g_p].ps->ang = g_player[vm.g_p].ps->oang;
                    updatesector(g_player[vm.g_p].ps->pos.x,g_player[vm.g_p].ps->pos.y,&g_player[vm.g_p].ps->cursectnum);
                    P_UpdateScreenPal(g_player[vm.g_p].ps);

                    j = headspritestat[STAT_ACTOR];
                    while (j >= 0)
                    {
                        if (sprite[j].picnum==CAMERA1)
                            sprite[j].yvel = 0;
                        j = nextspritestat[j];
                    }
                }

                j = sprite[g_player[vm.g_p].ps->i].extra;

                if (vm.g_sp->picnum != ATOMICHEALTH)
                {
                    if (j > g_player[vm.g_p].ps->max_player_health && *insptr > 0)
                    {
                        insptr++;
                        continue;
                    }
                    else
                    {
                        if (j > 0)
                            j += *insptr;
                        if (j > g_player[vm.g_p].ps->max_player_health && *insptr > 0)
                            j = g_player[vm.g_p].ps->max_player_health;
                    }
                }
                else
                {
                    if (j > 0)
                        j += *insptr;
                    if (j > (g_player[vm.g_p].ps->max_player_health<<1))
                        j = (g_player[vm.g_p].ps->max_player_health<<1);
                }

                if (j < 0) j = 0;

                if (ud.god == 0)
                {
                    if (*insptr > 0)
                    {
                        if ((j - *insptr) < (g_player[vm.g_p].ps->max_player_health>>2) &&
                                j >= (g_player[vm.g_p].ps->max_player_health>>2))
                            A_PlaySound(DUKE_GOTHEALTHATLOW,g_player[vm.g_p].ps->i);

                        g_player[vm.g_p].ps->last_extra = j;
                    }

                    sprite[g_player[vm.g_p].ps->i].extra = j;
                }
            }

            insptr++;
            continue;

        case CON_STATE:
        {
            intptr_t *tempscrptr=insptr+2;

            insptr = (intptr_t *) *(insptr+1);
            VM_Execute(0);
            insptr = tempscrptr;
        }
        continue;

        case CON_LEFTBRACE:
            insptr++;
            VM_Execute(0);
            continue;

        case CON_MOVE:
            insptr++;
            vm.g_t[0]=0;
            vm.g_t[1] = *insptr++;
            vm.g_sp->hitag = *insptr++;
            if (A_CheckEnemySprite(vm.g_sp) && vm.g_sp->extra <= 0) // hack
                continue;
            if (vm.g_sp->hitag&random_angle)
                vm.g_sp->ang = krand()&2047;
            continue;

        case CON_ADDWEAPONVAR:
            insptr++;
            if ((g_player[vm.g_p].ps->gotweapon & (1 << Gv_GetVarX(*(insptr)))) == 0)
            {
                if (!(g_player[vm.g_p].ps->weaponswitch & 1)) P_AddWeaponNoSwitch(g_player[vm.g_p].ps, Gv_GetVarX(*(insptr)));
                else P_AddWeapon(g_player[vm.g_p].ps, Gv_GetVarX(*(insptr)));
            }
            else if (g_player[vm.g_p].ps->ammo_amount[Gv_GetVarX(*(insptr))] >= g_player[vm.g_p].ps->max_ammo_amount[Gv_GetVarX(*(insptr))])
            {
                vm.g_flags |= VM_NOEXECUTE;
                continue;
            }
            P_AddAmmo(Gv_GetVarX(*(insptr)), g_player[vm.g_p].ps, Gv_GetVarX(*(insptr+1)));
            if (g_player[vm.g_p].ps->curr_weapon == KNEE_WEAPON && (g_player[vm.g_p].ps->gotweapon & (1 << Gv_GetVarX(*(insptr)))))
            {
                if (!(g_player[vm.g_p].ps->weaponswitch & 1)) P_AddWeaponNoSwitch(g_player[vm.g_p].ps, Gv_GetVarX(*(insptr)));
                else P_AddWeapon(g_player[vm.g_p].ps, Gv_GetVarX(*(insptr)));
            }
            insptr += 2;
            continue;

        case CON_ACTIVATEBYSECTOR:
        case CON_OPERATESECTORS:
        case CON_OPERATEACTIVATORS:
        case CON_SETASPECT:
        case CON_SSP:
            insptr++;
            {
                int32_t var1 = Gv_GetVarX(*insptr++), var2;
                if (tw == CON_OPERATEACTIVATORS && *insptr == g_iThisActorID)
                {
                    var2 = vm.g_p;
                    insptr++;
                }
                else var2 = Gv_GetVarX(*insptr++);

                switch (tw)
                {
                case CON_ACTIVATEBYSECTOR:
                    if ((var1<0 || var1>=numsectors)) {OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],var1); break;}
                    activatebysector(var1, var2);
                    break;
                case CON_OPERATESECTORS:
                    if ((var1<0 || var1>=numsectors)) {OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],var1); break;}
                    G_OperateSectors(var1, var2);
                    break;
                case CON_OPERATEACTIVATORS:
                    if ((var2<0 || var2>=playerswhenstarted)) {OSD_Printf(CON_ERROR "Invalid player %d\n",g_errorLineNum,keyw[g_tw],var2); break;}
                    G_OperateActivators(var1, var2);
                    break;
                case CON_SETASPECT:
                    setaspect(var1, var2);
                    break;
                case CON_SSP:
                    if ((var1<0 || var1>=MAXSPRITES)) { OSD_Printf(CON_ERROR "Invalid sprite %d\n",g_errorLineNum,keyw[g_tw],var1); break;}
                    A_SetSprite(var1, var2);
                    break;
                }
                continue;
            }

        case CON_CANSEESPR:
            insptr++;
            {
                int32_t lVar1 = Gv_GetVarX(*insptr++), lVar2 = Gv_GetVarX(*insptr++), res;

                if ((lVar1<0 || lVar1>=MAXSPRITES || lVar2<0 || lVar2>=MAXSPRITES))
                {
                    OSD_Printf(CON_ERROR "Invalid sprite %d\n",g_errorLineNum,keyw[g_tw],lVar1<0||lVar1>=MAXSPRITES?lVar1:lVar2);
                    res=0;
                }
                else res=cansee(sprite[lVar1].x,sprite[lVar1].y,sprite[lVar1].z,sprite[lVar1].sectnum,
                                    sprite[lVar2].x,sprite[lVar2].y,sprite[lVar2].z,sprite[lVar2].sectnum);

                Gv_SetVarX(*insptr++, res);
                continue;
            }

        case CON_OPERATERESPAWNS:
        case CON_OPERATEMASTERSWITCHES:
        case CON_CHECKACTIVATORMOTION:
            insptr++;
            {
                int32_t var1 = Gv_GetVarX(*insptr++);

                switch (tw)
                {
                case CON_OPERATERESPAWNS:
                    G_OperateRespawns(var1);
                    break;
                case CON_OPERATEMASTERSWITCHES:
                    G_OperateMasterSwitches(var1);
                    break;
                case CON_CHECKACTIVATORMOTION:
                    Gv_SetVarX(g_iReturnVarID, G_CheckActivatorMotion(var1));
                    break;
                }
                continue;
            }

        case CON_INSERTSPRITEQ:
            insptr++;
            A_AddToDeleteQueue(vm.g_i);
            continue;

        case CON_QSTRLEN:
            insptr++;
            {
                int32_t i=*insptr++;
                int32_t j=Gv_GetVarX(*insptr++);
                if ((ScriptQuotes[j] == NULL))
                {
                    OSD_Printf(CON_ERROR "null quote %d\n",g_errorLineNum,keyw[g_tw],j);
                    Gv_SetVarX(i,-1);
                    continue;
                }
                Gv_SetVarX(i,Bstrlen(ScriptQuotes[j]));
                continue;
            }

        case CON_HEADSPRITESTAT:
            insptr++;
            {
                int32_t i=*insptr++;
                int32_t j=Gv_GetVarX(*insptr++);
                if ((j < 0 || j > MAXSTATUS))
                {
                    OSD_Printf(CON_ERROR "invalid status list %d\n",g_errorLineNum,keyw[g_tw],j);
                    continue;
                }
                Gv_SetVarX(i,headspritestat[j]);
                continue;
            }

        case CON_PREVSPRITESTAT:
            insptr++;
            {
                int32_t i=*insptr++;
                int32_t j=Gv_GetVarX(*insptr++);
                if ((j < 0 || j >= MAXSPRITES))
                {
                    OSD_Printf(CON_ERROR "invalid sprite ID %d\n",g_errorLineNum,keyw[g_tw],j);
                    continue;
                }
                Gv_SetVarX(i,prevspritestat[j]);
                continue;
            }

        case CON_NEXTSPRITESTAT:
            insptr++;
            {
                int32_t i=*insptr++;
                int32_t j=Gv_GetVarX(*insptr++);
                if ((j < 0 || j >= MAXSPRITES))
                {
                    OSD_Printf(CON_ERROR "invalid sprite ID %d\n",g_errorLineNum,keyw[g_tw],j);
                    continue;
                }
                Gv_SetVarX(i,nextspritestat[j]);
                continue;
            }

        case CON_HEADSPRITESECT:
            insptr++;
            {
                int32_t i=*insptr++;
                int32_t j=Gv_GetVarX(*insptr++);
                if ((j < 0 || j > numsectors))
                {
                    OSD_Printf(CON_ERROR "invalid sector %d\n",g_errorLineNum,keyw[g_tw],j);
                    continue;
                }
                Gv_SetVarX(i,headspritesect[j]);
                continue;
            }

        case CON_PREVSPRITESECT:
            insptr++;
            {
                int32_t i=*insptr++;
                int32_t j=Gv_GetVarX(*insptr++);
                if ((j < 0 || j >= MAXSPRITES))
                {
                    OSD_Printf(CON_ERROR "invalid sprite ID %d\n",g_errorLineNum,keyw[g_tw],j);
                    continue;
                }
                Gv_SetVarX(i,prevspritesect[j]);
                continue;
            }

        case CON_NEXTSPRITESECT:
            insptr++;
            {
                int32_t i=*insptr++;
                int32_t j=Gv_GetVarX(*insptr++);
                if ((j < 0 || j >= MAXSPRITES))
                {
                    OSD_Printf(CON_ERROR "invalid sprite ID %d\n",g_errorLineNum,keyw[g_tw],j);
                    continue;
                }
                Gv_SetVarX(i,nextspritesect[j]);
                continue;
            }

        case CON_GETKEYNAME:
            insptr++;
            {
                int32_t i = Gv_GetVarX(*insptr++),
                        f=Gv_GetVarX(*insptr++);
                int32_t j=Gv_GetVarX(*insptr++);
                if ((i<0 || i>=MAXQUOTES))
                    OSD_Printf(CON_ERROR "invalid quote ID %d\n",g_errorLineNum,keyw[g_tw],i);
                else if ((ScriptQuotes[i] == NULL))
                    OSD_Printf(CON_ERROR "null quote %d\n",g_errorLineNum,keyw[g_tw],i);
                else if ((f<0 || f>=NUMGAMEFUNCTIONS))
                    OSD_Printf(CON_ERROR "invalid function %d\n",g_errorLineNum,keyw[g_tw],f);
                else
                {
                    if (j<2)
                        Bstrcpy(tempbuf,KB_ScanCodeToString(ud.config.KeyboardKeys[f][j]));
                    else
                    {
                        Bstrcpy(tempbuf,KB_ScanCodeToString(ud.config.KeyboardKeys[f][0]));
                        if (!*tempbuf)
                            Bstrcpy(tempbuf,KB_ScanCodeToString(ud.config.KeyboardKeys[f][1]));
                    }
                }

                if (*tempbuf)
                    Bstrcpy(ScriptQuotes[i],tempbuf);
                continue;
            }
        case CON_QSUBSTR:
            insptr++;
            {
                int32_t q1 = Gv_GetVarX(*insptr++);
                int32_t q2 = Gv_GetVarX(*insptr++);
                int32_t st = Gv_GetVarX(*insptr++);
                int32_t ln = Gv_GetVarX(*insptr++);

                if ((q1<0 || q1>=MAXQUOTES))
                {
                    OSD_Printf(CON_ERROR "invalid quote ID %d\n",g_errorLineNum,keyw[g_tw],q1);
                    continue;
                }
                if ((ScriptQuotes[q1] == NULL))
                {
                    OSD_Printf(CON_ERROR "null quote %d\n",g_errorLineNum,keyw[g_tw],q1);
                    continue;
                }
                if ((q2<0 || q2>=MAXQUOTES))
                {
                    OSD_Printf(CON_ERROR "invalid quote ID %d\n",g_errorLineNum,keyw[g_tw],q2);
                    continue;
                }
                if ((ScriptQuotes[q2] == NULL))
                {
                    OSD_Printf(CON_ERROR "null quote %d\n",g_errorLineNum,keyw[g_tw],q2);
                    continue;
                }

                {
                    char *s1 = ScriptQuotes[q1];
                    char *s2 = ScriptQuotes[q2];

                    while (*s2 && st--) s2++;
                    while ((*s1 = *s2) && ln--)
                    {
                        s1++;
                        s2++;
                    }
                    *s1=0;
                }
                continue;
            }

        case CON_GETPNAME:
        case CON_QSTRNCAT:
        case CON_QSTRCAT:
        case CON_QSTRCPY:
        case CON_QGETSYSSTR:
        case CON_CHANGESPRITESECT:
            insptr++;
            {
                int32_t i = Gv_GetVarX(*insptr++), j;
                if (tw == CON_GETPNAME && *insptr == g_iThisActorID)
                {
                    j = vm.g_p;
                    insptr++;
                }
                else j = Gv_GetVarX(*insptr++);

                switch (tw)
                {
                case CON_GETPNAME:
                    if ((ScriptQuotes[i] == NULL))
                    {
                        OSD_Printf(CON_ERROR "null quote %d\n",g_errorLineNum,keyw[g_tw],i);
                        break;
                    }
                    if (g_player[j].user_name[0])
                        Bstrcpy(ScriptQuotes[i],g_player[j].user_name);
                    else Bsprintf(ScriptQuotes[i],"%d",j);
                    break;
                case CON_QGETSYSSTR:
                    if ((ScriptQuotes[i] == NULL))
                    {
                        OSD_Printf(CON_ERROR "null quote %d %d\n",g_errorLineNum,keyw[g_tw],i,j);
                        break;
                    }
                    switch (j)
                    {
                    case STR_MAPNAME:
                        Bstrcpy(ScriptQuotes[i],MapInfo[ud.volume_number*MAXLEVELS + ud.level_number].name);
                        break;
                    case STR_MAPFILENAME:
                        Bstrcpy(ScriptQuotes[i],MapInfo[ud.volume_number*MAXLEVELS + ud.level_number].filename);
                        break;
                    case STR_PLAYERNAME:
                        Bstrcpy(ScriptQuotes[i],g_player[vm.g_p].user_name);
                        break;
                    case STR_VERSION:
                        Bsprintf(tempbuf,HEAD2 " %s",s_buildDate);
                        Bstrcpy(ScriptQuotes[i],tempbuf);
                        break;
                    case STR_GAMETYPE:
                        Bstrcpy(ScriptQuotes[i],GametypeNames[ud.coop]);
                        break;
                    case STR_VOLUMENAME:
                        Bstrcpy(ScriptQuotes[i],EpisodeNames[ud.volume_number]);
                        break;
                    default:
                        OSD_Printf(CON_ERROR "unknown str ID %d %d\n",g_errorLineNum,keyw[g_tw],i,j);
                    }
                    break;
                case CON_QSTRCAT:
                    if ((ScriptQuotes[i] == NULL || ScriptQuotes[j] == NULL)) goto nullquote;
                    Bstrncat(ScriptQuotes[i],ScriptQuotes[j],(MAXQUOTELEN-1)-Bstrlen(ScriptQuotes[i]));
                    break;
                case CON_QSTRNCAT:
                    if ((ScriptQuotes[i] == NULL || ScriptQuotes[j] == NULL)) goto nullquote;
                    Bstrncat(ScriptQuotes[i],ScriptQuotes[j],Gv_GetVarX(*insptr++));
                    break;
                case CON_QSTRCPY:
                    if ((ScriptQuotes[i] == NULL || ScriptQuotes[j] == NULL)) goto nullquote;
                    Bstrcpy(ScriptQuotes[i],ScriptQuotes[j]);
                    break;
                case CON_CHANGESPRITESECT:
                    if ((i<0 || i>=MAXSPRITES))
                    {
                        OSD_Printf(CON_ERROR "Invalid sprite %d\n",g_errorLineNum,keyw[g_tw],i);
                        break;
                    }
                    if ((j<0 || j>=numsectors))
                    {
                        OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],j);
                        break;
                    }
                    changespritesect(i,j);
                    break;
                default:
nullquote:
                    OSD_Printf(CON_ERROR "null quote %d\n",g_errorLineNum,keyw[g_tw],ScriptQuotes[i] ? j : i);
                    break;
                }
                continue;
            }

        case CON_CHANGESPRITESTAT:
            insptr++;
            {
                int32_t i = Gv_GetVarX(*insptr++);
                int32_t j = Gv_GetVarX(*insptr++);

                if ((i<0 || i>=MAXSPRITES))
                {
                    OSD_Printf(CON_ERROR "Invalid sprite: %d\n",g_errorLineNum,keyw[g_tw],i);
                    continue;
                }
                if ((j<0 || j>=MAXSTATUS))
                {
                    OSD_Printf(CON_ERROR "Invalid statnum: %d\n",g_errorLineNum,keyw[g_tw],j);
                    continue;
                }
                if (sprite[i].statnum == j) continue;

                /* initialize actor data when changing to an actor statnum because there's usually
                garbage left over from being handled as a hard coded object */

                if (sprite[i].statnum > STAT_ZOMBIEACTOR && (j == STAT_ACTOR || j == STAT_ZOMBIEACTOR))
                {
                    actor[i].lastvx = 0;
                    actor[i].lastvy = 0;
                    actor[i].timetosleep = 0;
                    actor[i].cgg = 0;
                    actor[i].movflag = 0;
                    actor[i].tempang = 0;
                    actor[i].dispicnum = 0;
                    T1=T2=T3=T4=T5=T6=T7=T8=T9=0;
                    actor[i].flags = 0;
                    sprite[i].hitag = 0;

                    // pointers
                    if (actorscrptr[sprite[i].picnum])
                    {
                        T5 = *(actorscrptr[sprite[i].picnum]+1);
                        T2 = *(actorscrptr[sprite[i].picnum]+2);
                        sprite[i].hitag = *(actorscrptr[sprite[i].picnum]+3);
                    }
                }
                changespritestat(i,j);
                continue;
            }

        case CON_STARTLEVEL:
            insptr++; // skip command
            {
                // from 'level' cheat in game.c (about line 6250)
                int32_t volnume=Gv_GetVarX(*insptr++), levnume=Gv_GetVarX(*insptr++);

                if ((volnume > MAXVOLUMES-1 || volnume < 0))
                {
                    OSD_Printf(CON_ERROR "invalid volume (%d)\n",g_errorLineNum,keyw[g_tw],volnume);
                    continue;
                }

                if ((levnume > MAXLEVELS-1 || levnume < 0))
                {
                    OSD_Printf(CON_ERROR "invalid level (%d)\n",g_errorLineNum,keyw[g_tw],levnume);
                    continue;
                }

                ud.m_volume_number = ud.volume_number = volnume;
                ud.m_level_number = ud.level_number = levnume;
                if (numplayers > 1 && g_netServer)
                    Net_NewGame(volnume,levnume);
                else
                {
                    g_player[myconnectindex].ps->gm |= MODE_EOL;
                    ud.display_bonus_screen = 0;
                } // MODE_RESTART;

                continue;
            }

        case CON_MYOSX:
        case CON_MYOSPALX:
        case CON_MYOS:
        case CON_MYOSPAL:
            insptr++;
            {
                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++), tilenum=Gv_GetVarX(*insptr++);
                int32_t shade=Gv_GetVarX(*insptr++), orientation=Gv_GetVarX(*insptr++);

                switch (tw)
                {
                case CON_MYOS:
                    G_DrawTile(x,y,tilenum,shade,orientation);
                    break;
                case CON_MYOSPAL:
                {
                    int32_t pal=Gv_GetVarX(*insptr++);
                    G_DrawTilePal(x,y,tilenum,shade,orientation,pal);
                    break;
                }
                case CON_MYOSX:
                    G_DrawTileSmall(x,y,tilenum,shade,orientation);
                    break;
                case CON_MYOSPALX:
                {
                    int32_t pal=Gv_GetVarX(*insptr++);
                    G_DrawTilePalSmall(x,y,tilenum,shade,orientation,pal);
                    break;
                }
                }
                continue;
            }

        case CON_SWITCH:
            insptr++; // p-code
            {
                // command format:
                // variable ID to check
                // script offset to 'end'
                // count of case statements
                // script offset to default case (null if none)
                // For each case: value, ptr to code
                //AddLog("Processing Switch...");
                int32_t lValue=Gv_GetVarX(*insptr++), lEnd=*insptr++, lCases=*insptr++;
                intptr_t *lpDefault=insptr++, *lpCases=insptr, *lTempInsPtr;
                int32_t bMatched=0, lCheckCase;
                int32_t left,right;
                insptr += lCases*2;
                lTempInsPtr=insptr;
                //Bsprintf(g_szBuf,"lEnd= %d *lpDefault=%d",lEnd,*lpDefault);
                //AddLog(g_szBuf);

                //Bsprintf(g_szBuf,"Checking %d cases for %d",lCases, lValue);
                //AddLog(g_szBuf);
                left=0; right=lCases-1;
                while (!bMatched)
                {
                    //Bsprintf(g_szBuf,"Checking #%d Value= %d",lCheckCase, lpCases[lCheckCase*2]);
                    //AddLog(g_szBuf);
                    lCheckCase=(left+right)/2;
                    //                initprintf("(%2d..%2d..%2d) [%2d..%2d..%2d]==%2d\n",left,lCheckCase,right,lpCases[left*2],lpCases[lCheckCase*2],lpCases[right*2],lValue);
                    if (lpCases[lCheckCase*2] > lValue)
                        right=lCheckCase-1;
                    else if (lpCases[lCheckCase*2] < lValue)
                        left =lCheckCase+1;
                    else if (lpCases[lCheckCase*2] == lValue)
                    {
                        //AddLog("Found Case Match");
                        //Bsprintf(g_szBuf,"insptr=%d. lCheckCase=%d, offset=%d, &script[0]=%d",
                        //            (int32_t)insptr,(int32_t)lCheckCase,lpCases[lCheckCase*2+1],(int32_t)&script[0]);
                        //AddLog(g_szBuf);
                        // fake a 2-d Array
                        insptr=(intptr_t *)(lpCases[lCheckCase*2+1] + &script[0]);
                        //Bsprintf(g_szBuf,"insptr=%d. ",     (int32_t)insptr);
                        //AddLog(g_szBuf);
                        VM_Execute(0);
                        //AddLog("Done Executing Case");
                        bMatched=1;
                    }
                    if (right-left < 0)
                        break;
                }
                if (!bMatched)
                {
                    if (*lpDefault)
                    {
                        //AddLog("No Matching Case: Using Default");
                        insptr=(intptr_t *)(*lpDefault + &script[0]);
                        VM_Execute(0);
                    }
                    else
                    {
                        //AddLog("No Matching Case: No Default to use");
                    }
                }
                insptr=(intptr_t *)(lEnd + (intptr_t)&script[0]);
                //Bsprintf(g_szBuf,"insptr=%d. ",     (int32_t)insptr);
                //AddLog(g_szBuf);
                //AddLog("Done Processing Switch");

                continue;
            }

        case CON_ENDSWITCH:
        case CON_ENDEVENT:
            insptr++;
            return 1;

        case CON_DISPLAYRAND:
            insptr++;
            Gv_SetVarX(*insptr++, rand());
            continue;

        case CON_DRAGPOINT:
            insptr++;
            {
                int32_t wallnum = Gv_GetVarX(*insptr++), newx = Gv_GetVarX(*insptr++), newy = Gv_GetVarX(*insptr++);

                if ((wallnum<0 || wallnum>=numwalls))
                {
                    OSD_Printf(CON_ERROR "Invalid wall %d\n",g_errorLineNum,keyw[g_tw],wallnum);
                    continue;
                }
                dragpoint(wallnum,newx,newy);
                continue;
            }

        case CON_LDIST:
            insptr++;
            {
                int32_t distvar = *insptr++, xvar = Gv_GetVarX(*insptr++), yvar = Gv_GetVarX(*insptr++);

                if ((xvar < 0 || yvar < 0 || xvar >= MAXSPRITES || yvar >= MAXSPRITES))
                {
                    OSD_Printf(CON_ERROR "invalid sprite\n",g_errorLineNum,keyw[g_tw]);
                    continue;
                }

                Gv_SetVarX(distvar, ldist(&sprite[xvar],&sprite[yvar]));
                continue;
            }

        case CON_DIST:
            insptr++;
            {
                int32_t distvar = *insptr++, xvar = Gv_GetVarX(*insptr++), yvar = Gv_GetVarX(*insptr++);

                if ((xvar < 0 || yvar < 0 || xvar >= MAXSPRITES || yvar >= MAXSPRITES))
                {
                    OSD_Printf(CON_ERROR "invalid sprite\n",g_errorLineNum,keyw[g_tw]);
                    continue;
                }

                Gv_SetVarX(distvar, dist(&sprite[xvar],&sprite[yvar]));
                continue;
            }

        case CON_GETANGLE:
            insptr++;
            {
                int32_t angvar = *insptr++;
                int32_t xvar = Gv_GetVarX(*insptr++);
                int32_t yvar = Gv_GetVarX(*insptr++);

                Gv_SetVarX(angvar, getangle(xvar,yvar));
                continue;
            }

        case CON_GETINCANGLE:
            insptr++;
            {
                int32_t angvar = *insptr++;
                int32_t xvar = Gv_GetVarX(*insptr++);
                int32_t yvar = Gv_GetVarX(*insptr++);

                Gv_SetVarX(angvar, G_GetAngleDelta(xvar,yvar));
                continue;
            }

        case CON_MULSCALE:
            insptr++;
            {
                int32_t var1 = *insptr++, var2 = Gv_GetVarX(*insptr++);
                int32_t var3 = Gv_GetVarX(*insptr++), var4 = Gv_GetVarX(*insptr++);

                Gv_SetVarX(var1, mulscale(var2, var3, var4));
                continue;
            }

        case CON_INITTIMER:
            insptr++;
            {
                int32_t j = Gv_GetVarX(*insptr++);

                if (g_timerTicsPerSecond == j)
                    continue;

                uninittimer();
                inittimer((g_timerTicsPerSecond = j));
            }
            continue;

        case CON_TIME:
            insptr += 2;
            continue;

        case CON_ESPAWNVAR:
        case CON_EQSPAWNVAR:
        case CON_QSPAWNVAR:
            insptr++;
            {
                int32_t lIn=Gv_GetVarX(*insptr++);
                int32_t j;
                if ((vm.g_sp->sectnum < 0 || vm.g_sp->sectnum >= numsectors))
                {
                    OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],vm.g_sp->sectnum);
                    continue;
                }
                j = A_Spawn(vm.g_i, lIn);
                switch (tw)
                {
                case CON_EQSPAWNVAR:
                    if (j != -1)
                        A_AddToDeleteQueue(j);
                case CON_ESPAWNVAR:
                    aGameVars[g_iReturnVarID].val.lValue = j;
                    break;
                case CON_QSPAWNVAR:
                    if (j != -1)
                        A_AddToDeleteQueue(j);
                    break;
                }
                continue;
            }

        case CON_ESPAWN:
        case CON_EQSPAWN:
        case CON_QSPAWN:
            insptr++;

            {
                int32_t j;

                if ((vm.g_sp->sectnum < 0 || vm.g_sp->sectnum >= numsectors))
                {
                    OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],vm.g_sp->sectnum);
                    insptr++;
                    continue;
                }

                j = A_Spawn(vm.g_i,*insptr++);

                switch (tw)
                {
                case CON_EQSPAWN:
                    if (j != -1)
                        A_AddToDeleteQueue(j);
                case CON_ESPAWN:
                    aGameVars[g_iReturnVarID].val.lValue = j;
                    break;
                case CON_QSPAWN:
                    if (j != -1)
                        A_AddToDeleteQueue(j);
                    break;
                }
            }
            continue;

        case CON_ESHOOT:
        case CON_EZSHOOT:
        case CON_ZSHOOT:
            insptr++;

            {
                int32_t j;

                if (tw != CON_ESHOOT)
                {
                    actor[vm.g_i].shootzvel = Gv_GetVarX(*insptr++);
                    if (actor[vm.g_i].shootzvel == 0)
                        actor[vm.g_i].shootzvel = 1;
                }

                if ((vm.g_sp->sectnum < 0 || vm.g_sp->sectnum >= numsectors))
                {
                    OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],vm.g_sp->sectnum);
                    insptr++;
                    actor[vm.g_i].shootzvel=0;
                    continue;
                }

                j = A_Shoot(vm.g_i,*insptr++);

                if (tw != CON_ZSHOOT)
                    aGameVars[g_iReturnVarID].val.lValue = j;

                actor[vm.g_i].shootzvel=0;
            }
            continue;

        case CON_SHOOTVAR:
        case CON_ESHOOTVAR:
            insptr++;
            {
                int32_t j=Gv_GetVarX(*insptr++);

                if ((vm.g_sp->sectnum < 0 || vm.g_sp->sectnum >= numsectors))
                {
                    OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],vm.g_sp->sectnum);
                    actor[vm.g_i].shootzvel=0;
                    continue;
                }

                j = A_Shoot(vm.g_i, j);
                if (tw == CON_ESHOOTVAR)
                    aGameVars[g_iReturnVarID].val.lValue = j;
                actor[vm.g_i].shootzvel=0;
                continue;
            }

        case CON_EZSHOOTVAR:
        case CON_ZSHOOTVAR:
            insptr++;

            actor[vm.g_i].shootzvel = Gv_GetVarX(*insptr++);

            if (actor[vm.g_i].shootzvel == 0)
                actor[vm.g_i].shootzvel = 1;

            {
                int32_t j=Gv_GetVarX(*insptr++);

                if ((vm.g_sp->sectnum < 0 || vm.g_sp->sectnum >= numsectors))
                {
                    OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],vm.g_sp->sectnum);
                    actor[vm.g_i].shootzvel=0;
                    continue;
                }

                j = A_Shoot(vm.g_i, j);
                if (tw == CON_EZSHOOTVAR)
                    aGameVars[g_iReturnVarID].val.lValue = j;
                actor[vm.g_i].shootzvel=0;
                continue;
            }

        case CON_CMENU:
            insptr++;
            ChangeToMenu(Gv_GetVarX(*insptr++));
            continue;

        case CON_SOUNDVAR:
        case CON_STOPSOUNDVAR:
        case CON_SOUNDONCEVAR:
        case CON_GLOBALSOUNDVAR:
            insptr++;
            {
                int32_t j=Gv_GetVarX(*insptr++);

                if (j<0 || j>=MAXSOUNDS)
                {
                    OSD_Printf(CON_ERROR "Invalid sound %d\n",g_errorLineNum,keyw[g_tw],j);
                    continue;
                }

                switch (tw)
                {
                case CON_SOUNDONCEVAR:
                    if (!S_CheckSoundPlaying(vm.g_i,j))
                        A_PlaySound((int16_t)j,vm.g_i);
                    continue;
                case CON_GLOBALSOUNDVAR:
                    A_PlaySound((int16_t)j,g_player[screenpeek].ps->i);
                    continue;
                case CON_STOPSOUNDVAR:
                    if (S_CheckSoundPlaying(vm.g_i,j))
                        S_StopSound((int16_t)j);
                    continue;
                case CON_SOUNDVAR:
                    A_PlaySound((int16_t)j,vm.g_i);
                    continue;
                }
            }
            continue;

        case CON_GUNIQHUDID:
            insptr++;
            {
                int32_t j=Gv_GetVarX(*insptr++);
                if (j >= 0 && j < MAXUNIQHUDID-1)
                    guniqhudid = j;
                else
                    OSD_Printf(CON_ERROR "Invalid ID %d\n",g_errorLineNum,keyw[g_tw],j);
                continue;
            }

        case CON_SAVEGAMEVAR:
        case CON_READGAMEVAR:
        {
            int32_t i=0;
            insptr++;
            if (ud.config.scripthandle < 0)
            {
                insptr++;
                continue;
            }
            switch (tw)
            {
            case CON_SAVEGAMEVAR:
                i=Gv_GetVarX(*insptr);
                SCRIPT_PutNumber(ud.config.scripthandle, "Gamevars",aGameVars[*insptr++].szLabel,i,FALSE,FALSE);
                break;
            case CON_READGAMEVAR:
                SCRIPT_GetNumber(ud.config.scripthandle, "Gamevars",aGameVars[*insptr].szLabel,&i);
                Gv_SetVarX(*insptr++, i);
                break;
            }
            continue;
        }

        case CON_SHOWVIEW:
            insptr++;
            {
                int32_t x=Gv_GetVarX(*insptr++);
                int32_t y=Gv_GetVarX(*insptr++);
                int32_t z=Gv_GetVarX(*insptr++);
                int32_t a=Gv_GetVarX(*insptr++);
                int32_t horiz=Gv_GetVarX(*insptr++);
                int32_t sect=Gv_GetVarX(*insptr++);
                int32_t x1=scale(Gv_GetVarX(*insptr++),xdim,320);
                int32_t y1=scale(Gv_GetVarX(*insptr++),ydim,200);
                int32_t x2=scale(Gv_GetVarX(*insptr++),xdim,320);
                int32_t y2=scale(Gv_GetVarX(*insptr++),ydim,200);
                int32_t smoothratio = min(max((totalclock - ototalclock) * (65536 / 4),0),65536);
#if defined(USE_OPENGL) && defined(POLYMOST)
                int32_t j;
#endif

                if (g_screenCapture) continue;

                if (x1 > x2) swaplong(&x1,&x2);
                if (y1 > y2) swaplong(&y1,&y2);

                if ((x1 < 0 || y1 < 0 || x2 > xdim-1 || y2 > ydim-1 || x2-x1 < 2 || y2-y1 < 2))
                {
                    OSD_Printf(CON_ERROR "incorrect coordinates\n",g_errorLineNum,keyw[g_tw]);
                    continue;
                }
                if ((sect<0 || sect>=numsectors))
                {
                    OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],sect);
                    continue;
                }

#if defined(USE_OPENGL) && defined(POLYMOST)
                j = glprojectionhacks;
                glprojectionhacks = 0;
#endif
                setview(x1,y1,x2,y2);

#if 0
                if (!ud.pause_on && ((ud.show_help == 0 && (!net_server && ud.multimode < 2) && !(g_player[myconnectindex].ps->gm&MODE_MENU))
                                     || (net_server || ud.multimode > 1) || ud.recstat == 2))
                    smoothratio = min(max((totalclock-ototalclock)*(65536L/TICSPERFRAME),0),65536);
#endif
                G_DoInterpolations(smoothratio);

                if (((gotpic[MIRROR>>3]&(1<<(MIRROR&7))) > 0)
#if defined(POLYMER) && defined(USE_OPENGL)
                        && (getrendermode() != 4)
#endif
                   )
                {
                    int32_t j, i = 0, k, dst = 0x7fffffff;

                    for (k=g_mirrorCount-1; k>=0; k--)
                    {
                        j = klabs(wall[g_mirrorWall[k]].x-x);
                        j += klabs(wall[g_mirrorWall[k]].y-y);
                        if (j < dst) dst = j, i = k;
                    }

                    if (wall[g_mirrorWall[i]].overpicnum == MIRROR)
                    {
                        int32_t tposx,tposy;
                        int16_t tang;

                        preparemirror(x,y,z,a,horiz,g_mirrorWall[i],g_mirrorSector[i],&tposx,&tposy,&tang);

                        j = visibility;
                        visibility = (j>>1) + (j>>2);

                        drawrooms(tposx,tposy,z,tang,horiz,g_mirrorSector[i]+MAXSECTORS);

                        display_mirror = 1;
                        G_DoSpriteAnimations(tposx,tposy,tang,smoothratio);
                        display_mirror = 0;

                        drawmasks();
                        completemirror();   //Reverse screen x-wise in this function
                        visibility = j;
                    }
                    gotpic[MIRROR>>3] &= ~(1<<(MIRROR&7));
                }

#ifdef POLYMER
                if (getrendermode() == 4)
                    polymer_setanimatesprites(G_DoSpriteAnimations, x,y,a,smoothratio);
#endif
                drawrooms(x,y,z,a,horiz,sect);
                display_mirror = 2;
                G_DoSpriteAnimations(x,y,a,smoothratio);
                display_mirror = 0;
                drawmasks();
                G_RestoreInterpolations();
                G_UpdateScreenArea();
#if defined(USE_OPENGL) && defined(POLYMOST)
                glprojectionhacks = j;
#endif
                continue;
            }

        case CON_ROTATESPRITE16:
        case CON_ROTATESPRITE:
            insptr++;
            {
                int32_t x=Gv_GetVarX(*insptr++),   y=Gv_GetVarX(*insptr++),           z=Gv_GetVarX(*insptr++);
                int32_t a=Gv_GetVarX(*insptr++),   tilenum=Gv_GetVarX(*insptr++),     shade=Gv_GetVarX(*insptr++);
                int32_t pal=Gv_GetVarX(*insptr++), orientation=Gv_GetVarX(*insptr++);
                int32_t x1=Gv_GetVarX(*insptr++),  y1=Gv_GetVarX(*insptr++);
                int32_t x2=Gv_GetVarX(*insptr++),  y2=Gv_GetVarX(*insptr++);

                if (tw == CON_ROTATESPRITE)
                {
                    x<<=16;
                    y<<=16;
                }

                if (x < (-320)<<16 || x >= (640<<16) || y < (-200)<<16 || y >= (400<<16))
                {
                    OSD_Printf(CON_ERROR "invalid coordinates: %d, %d\n",g_errorLineNum, keyw[g_tw], x, y);
                    continue;
                }

                rotatesprite(x,y,z,a,tilenum,shade,pal,2|orientation,x1,y1,x2,y2);
                continue;
            }

        case CON_GAMETEXT:
        case CON_GAMETEXTZ:
            insptr++;
            {
                int32_t tilenum = Gv_GetVarX(*insptr++);
                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++), q=Gv_GetVarX(*insptr++);
                int32_t shade=Gv_GetVarX(*insptr++), pal=Gv_GetVarX(*insptr++);
                int32_t orientation=Gv_GetVarX(*insptr++);
                int32_t x1=Gv_GetVarX(*insptr++), y1=Gv_GetVarX(*insptr++);
                int32_t x2=Gv_GetVarX(*insptr++), y2=Gv_GetVarX(*insptr++);
                int32_t z = (tw == CON_GAMETEXTZ) ? Gv_GetVarX(*insptr++) : 65536;

                if ((ScriptQuotes[q] == NULL))
                {
                    OSD_Printf(CON_ERROR "null quote %d\n",g_errorLineNum,keyw[g_tw],q);
                    if (tw == CON_GAMETEXTZ)
                        Gv_GetVarX(*insptr++);
                    continue;
                }

                G_PrintGameText(0,tilenum,x>>1,y,ScriptQuotes[q],shade,pal,orientation,x1,y1,x2,y2,z);
                continue;
            }

        case CON_DIGITALNUMBER:
        case CON_DIGITALNUMBERZ:
            insptr++;
            {
                int32_t tilenum = Gv_GetVarX(*insptr++);
                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++), q=Gv_GetVarX(*insptr++);
                int32_t shade=Gv_GetVarX(*insptr++), pal=Gv_GetVarX(*insptr++);
                int32_t orientation=Gv_GetVarX(*insptr++);
                int32_t x1=Gv_GetVarX(*insptr++), y1=Gv_GetVarX(*insptr++);
                int32_t x2=Gv_GetVarX(*insptr++), y2=Gv_GetVarX(*insptr++);
                int32_t z = (tw == CON_DIGITALNUMBERZ) ? Gv_GetVarX(*insptr++) : 65536;

                G_DrawTXDigiNumZ(tilenum,x,y,q,shade,pal,orientation,x1,y1,x2,y2,z);
                continue;
            }

        case CON_MINITEXT:
            insptr++;
            {
                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++), q=Gv_GetVarX(*insptr++);
                int32_t shade=Gv_GetVarX(*insptr++), pal=Gv_GetVarX(*insptr++);

                if ((ScriptQuotes[q] == NULL))
                {
                    OSD_Printf(CON_ERROR "null quote %d\n",g_errorLineNum,keyw[g_tw],q);
                    continue;
                }
                minitextshade(x,y,ScriptQuotes[q],shade,pal,26);
                continue;
            }

        case CON_ANGOFF:
            insptr++;
            spriteext[vm.g_i].angoff=*insptr++;
            continue;

        case CON_GETZRANGE:
            insptr++;
            {
                vec3_t vect;

                vect.x = Gv_GetVarX(*insptr++);
                vect.y = Gv_GetVarX(*insptr++);
                vect.z = Gv_GetVarX(*insptr++);

                {
                    int32_t sectnum=Gv_GetVarX(*insptr++);
                    int32_t ceilzvar=*insptr++, ceilhitvar=*insptr++, florzvar=*insptr++, florhitvar=*insptr++;
                    int32_t walldist=Gv_GetVarX(*insptr++), clipmask=Gv_GetVarX(*insptr++);
                    int32_t ceilz, ceilhit, florz, florhit;


                    if ((sectnum<0 || sectnum>=numsectors))
                    {
                        OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],sectnum);
                        continue;
                    }
                    getzrange(&vect, sectnum, &ceilz, &ceilhit, &florz, &florhit, walldist, clipmask);
                    Gv_SetVarX(ceilzvar, ceilz);
                    Gv_SetVarX(ceilhitvar, ceilhit);
                    Gv_SetVarX(florzvar, florz);
                    Gv_SetVarX(florhitvar, florhit);
                }
                continue;
            }

        case CON_SECTSETINTERPOLATION:
        case CON_SECTCLEARINTERPOLATION:
            insptr++;
            {
                int32_t sectnum = Gv_GetVarX(*insptr++), osectnum;

                if ((sectnum<0 || sectnum>=numsectors))
                {
                    OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],sectnum);
                    continue;
                }

                osectnum = sprite[MAXSPRITES-1].sectnum;
                sprite[MAXSPRITES-1].sectnum = sectnum;
                if (tw==CON_SECTSETINTERPOLATION)
                    Sect_SetInterpolation(MAXSPRITES-1);
                else
                    Sect_ClearInterpolation(MAXSPRITES-1);
                sprite[MAXSPRITES-1].sectnum = osectnum;

                continue;
            }

        case CON_CALCHYPOTENUSE:
            insptr++;
            {
                int32_t retvar=*insptr++;
                int64_t dax=Gv_GetVarX(*insptr++), day=Gv_GetVarX(*insptr++);
                int64_t hypsq = dax*dax + day*day;

                if (hypsq > (int64_t)INT_MAX)
                    Gv_SetVarX(retvar, (int32_t)sqrt((double)hypsq));
                else
                    Gv_SetVarX(retvar, ksqrt((int32_t)hypsq));

                continue;
            }

        case CON_LINEINTERSECT:
        case CON_RAYINTERSECT:
            insptr++;
            {
                int32_t x1=Gv_GetVarX(*insptr++), y1=Gv_GetVarX(*insptr++), z1=Gv_GetVarX(*insptr++);
                int32_t x2=Gv_GetVarX(*insptr++), y2=Gv_GetVarX(*insptr++), z2=Gv_GetVarX(*insptr++);
                int32_t x3=Gv_GetVarX(*insptr++), y3=Gv_GetVarX(*insptr++), x4=Gv_GetVarX(*insptr++), y4=Gv_GetVarX(*insptr++);
                int32_t intxvar=*insptr++, intyvar=*insptr++, intzvar=*insptr++, retvar=*insptr++;
                int32_t intx, inty, intz, ret;

                if (tw==CON_LINEINTERSECT)
                    ret = lineintersect(x1, y1, z1, x2, y2, z2, x3, y3, x4, y4, &intx, &inty, &intz);
                else
                    ret = rayintersect(x1, y1, z1, x2, y2, z2, x3, y3, x4, y4, &intx, &inty, &intz);

                Gv_SetVarX(retvar, ret);
                if (ret)
                {
                    Gv_SetVarX(intxvar, intx);
                    Gv_SetVarX(intyvar, inty);
                    Gv_SetVarX(intzvar, intz);
                }

                continue;
            }

        case CON_CLIPMOVE:
        case CON_CLIPMOVENOSLIDE:
            insptr++;
            {
                vec3_t vect;
                int32_t retvar=*insptr++, xvar=*insptr++, yvar=*insptr++, z=Gv_GetVarX(*insptr++), sectnumvar=*insptr++;
                int32_t xvect=Gv_GetVarX(*insptr++), yvect=Gv_GetVarX(*insptr++);
                int32_t walldist=Gv_GetVarX(*insptr++), floordist=Gv_GetVarX(*insptr++), ceildist=Gv_GetVarX(*insptr++);
                int32_t clipmask=Gv_GetVarX(*insptr++), oclipmoveboxtracenum=clipmoveboxtracenum;
                int16_t sectnum;

                vect.x = Gv_GetVarX(xvar);
                vect.y = Gv_GetVarX(yvar);
                vect.z = z;
                sectnum = Gv_GetVarX(sectnumvar);

                if ((sectnum<0 || sectnum>=numsectors))
                {
                    OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],sectnum);
                    Gv_SetVarX(retvar, 0);
                    continue;
                }

                if (tw==CON_CLIPMOVENOSLIDE)
                    clipmoveboxtracenum = 1;
                Gv_SetVarX(retvar, clipmove(&vect, &sectnum, xvect, yvect, walldist, floordist, ceildist, clipmask));
                if (tw==CON_CLIPMOVENOSLIDE)
                    clipmoveboxtracenum = oclipmoveboxtracenum;
                Gv_SetVarX(sectnumvar, sectnum);
                Gv_SetVarX(xvar, vect.x);
                Gv_SetVarX(yvar, vect.y);

                continue;
            }

        case CON_HITSCAN:
            insptr++;
            {
                vec3_t vect;
                hitdata_t hitinfo;

                vect.x = Gv_GetVarX(*insptr++);
                vect.y = Gv_GetVarX(*insptr++);
                vect.z = Gv_GetVarX(*insptr++);

                {
                    int32_t sectnum=Gv_GetVarX(*insptr++);
                    int32_t vx=Gv_GetVarX(*insptr++), vy=Gv_GetVarX(*insptr++), vz=Gv_GetVarX(*insptr++);
                    int32_t hitsectvar=*insptr++, hitwallvar=*insptr++, hitspritevar=*insptr++;
                    int32_t hitxvar=*insptr++, hityvar=*insptr++, hitzvar=*insptr++, cliptype=Gv_GetVarX(*insptr++);

                    if ((sectnum<0 || sectnum>=numsectors))
                    {
                        OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],sectnum);
                        continue;
                    }
                    hitscan((const vec3_t *)&vect, sectnum, vx, vy, vz, &hitinfo, cliptype);
                    Gv_SetVarX(hitsectvar, hitinfo.hitsect);
                    Gv_SetVarX(hitwallvar, hitinfo.hitwall);
                    Gv_SetVarX(hitspritevar, hitinfo.hitsprite);
                    Gv_SetVarX(hitxvar, hitinfo.pos.x);
                    Gv_SetVarX(hityvar, hitinfo.pos.y);
                    Gv_SetVarX(hitzvar, hitinfo.pos.z);
                }
                continue;
            }

        case CON_CANSEE:
            insptr++;
            {
                int32_t x1=Gv_GetVarX(*insptr++), y1=Gv_GetVarX(*insptr++), z1=Gv_GetVarX(*insptr++);
                int32_t sect1=Gv_GetVarX(*insptr++);
                int32_t x2=Gv_GetVarX(*insptr++), y2=Gv_GetVarX(*insptr++), z2=Gv_GetVarX(*insptr++);
                int32_t sect2=Gv_GetVarX(*insptr++), rvar=*insptr++;

                if ((sect1<0 || sect1>=numsectors || sect2<0 || sect2>=numsectors))
                {
                    OSD_Printf(CON_ERROR "Invalid sector\n",g_errorLineNum,keyw[g_tw]);
                    Gv_SetVarX(rvar, 0);
                }

                Gv_SetVarX(rvar, cansee(x1,y1,z1,sect1,x2,y2,z2,sect2));
                continue;
            }

        case CON_ROTATEPOINT:
            insptr++;
            {
                int32_t xpivot=Gv_GetVarX(*insptr++), ypivot=Gv_GetVarX(*insptr++);
                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++), daang=Gv_GetVarX(*insptr++);
                int32_t x2var=*insptr++, y2var=*insptr++;
                int32_t x2, y2;

                rotatepoint(xpivot,ypivot,x,y,daang,&x2,&y2);
                Gv_SetVarX(x2var, x2);
                Gv_SetVarX(y2var, y2);
                continue;
            }

        case CON_NEARTAG:
            insptr++;
            {
                //             neartag(int32_t x, int32_t y, int32_t z, short sectnum, short ang,  //Starting position & angle
                //                     short *neartagsector,   //Returns near sector if sector[].tag != 0
                //                     short *neartagwall,     //Returns near wall if wall[].tag != 0
                //                     short *neartagsprite,   //Returns near sprite if sprite[].tag != 0
                //                     int32_t *neartaghitdist,   //Returns actual distance to object (scale: 1024=largest grid size)
                //                     int32_t neartagrange,      //Choose maximum distance to scan (scale: 1024=largest grid size)
                //                     char tagsearch)         //1-lotag only, 2-hitag only, 3-lotag&hitag

                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++), z=Gv_GetVarX(*insptr++);
                int32_t sectnum=Gv_GetVarX(*insptr++), ang=Gv_GetVarX(*insptr++);
                int32_t neartagsectorvar=*insptr++, neartagwallvar=*insptr++, neartagspritevar=*insptr++, neartaghitdistvar=*insptr++;
                int32_t neartagrange=Gv_GetVarX(*insptr++), tagsearch=Gv_GetVarX(*insptr++);

                if ((sectnum<0 || sectnum>=numsectors))
                {
                    OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],sectnum);
                    continue;
                }
                neartag(x, y, z, sectnum, ang, &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, neartagrange, tagsearch);

                Gv_SetVarX(neartagsectorvar, neartagsector);
                Gv_SetVarX(neartagwallvar, neartagwall);
                Gv_SetVarX(neartagspritevar, neartagsprite);
                Gv_SetVarX(neartaghitdistvar, neartaghitdist);
                continue;
            }

        case CON_GETTIMEDATE:
            insptr++;
            {
                int32_t v1=*insptr++,v2=*insptr++,v3=*insptr++,v4=*insptr++,v5=*insptr++,v6=*insptr++,v7=*insptr++,v8=*insptr++;
                time_t rawtime;
                struct tm *ti;

                time(&rawtime);
                ti=localtime(&rawtime);
                // initprintf("Time&date: %s\n",asctime (ti));

                Gv_SetVarX(v1, ti->tm_sec);
                Gv_SetVarX(v2, ti->tm_min);
                Gv_SetVarX(v3, ti->tm_hour);
                Gv_SetVarX(v4, ti->tm_mday);
                Gv_SetVarX(v5, ti->tm_mon);
                Gv_SetVarX(v6, ti->tm_year+1900);
                Gv_SetVarX(v7, ti->tm_wday);
                Gv_SetVarX(v8, ti->tm_yday);
                continue;
            }

        case CON_MOVESPRITE:
        case CON_SETSPRITE:
            insptr++;
            {
                int32_t spritenum = Gv_GetVarX(*insptr++);
                vec3_t davector;

                davector.x = Gv_GetVarX(*insptr++);
                davector.y = Gv_GetVarX(*insptr++);
                davector.z = Gv_GetVarX(*insptr++);

                if (tw == CON_SETSPRITE)
                {
                    if ((spritenum < 0 || spritenum >= MAXSPRITES))
                    {
                        OSD_Printf(CON_ERROR "invalid sprite ID %d\n",g_errorLineNum,keyw[g_tw],spritenum);
                        continue;
                    }
                    setsprite(spritenum, &davector);
                    continue;
                }

                {
                    int32_t cliptype = Gv_GetVarX(*insptr++);

                    if ((spritenum < 0 && spritenum >= MAXSPRITES))
                    {
                        OSD_Printf(CON_ERROR "invalid sprite ID %d\n",g_errorLineNum,keyw[g_tw],spritenum);
                        insptr++;
                        continue;
                    }
                    Gv_SetVarX(*insptr++, A_MoveSprite(spritenum, &davector, cliptype));
                    continue;
                }
            }

        case CON_GETFLORZOFSLOPE:
        case CON_GETCEILZOFSLOPE:
            insptr++;
            {
                int32_t sectnum = Gv_GetVarX(*insptr++), x = Gv_GetVarX(*insptr++), y = Gv_GetVarX(*insptr++);
                if ((sectnum<0 || sectnum>=numsectors))
                {
                    OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],sectnum);
                    insptr++;
                    continue;
                }

                if (tw == CON_GETFLORZOFSLOPE)
                {
                    Gv_SetVarX(*insptr++, getflorzofslope(sectnum,x,y));
                    continue;
                }
                Gv_SetVarX(*insptr++, getceilzofslope(sectnum,x,y));
                continue;
            }

        case CON_UPDATESECTOR:
        case CON_UPDATESECTORZ:
            insptr++;
            {
                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++);
                int32_t z=(tw==CON_UPDATESECTORZ)?Gv_GetVarX(*insptr++):0;
                int32_t var=*insptr++;
                int16_t w=sprite[vm.g_i].sectnum;

                if (tw==CON_UPDATESECTOR) updatesector(x,y,&w);
                else updatesectorz(x,y,z,&w);

                Gv_SetVarX(var, w);
                continue;
            }

        case CON_SPAWN:
            insptr++;
            if (vm.g_sp->sectnum >= 0 && vm.g_sp->sectnum < MAXSECTORS)
                A_Spawn(vm.g_i,*insptr);
            insptr++;
            continue;

        case CON_IFWASWEAPON:
            insptr++;
            VM_DoConditional(actor[vm.g_i].picnum == *insptr);
            continue;

        case CON_IFAI:
            insptr++;
            VM_DoConditional(vm.g_t[5] == *insptr);
            continue;

        case CON_IFACTION:
            insptr++;
            VM_DoConditional(vm.g_t[4] == *insptr);
            continue;

        case CON_IFACTIONCOUNT:
            insptr++;
            VM_DoConditional(vm.g_t[2] >= *insptr);
            continue;

        case CON_RESETACTIONCOUNT:
            insptr++;
            vm.g_t[2] = 0;
            continue;

        case CON_DEBRIS:
            insptr++;
            {
                int32_t dnum = *insptr++;
                int32_t s, l, j;

                if (vm.g_sp->sectnum >= 0 && vm.g_sp->sectnum < MAXSECTORS)
                    for (j=(*insptr)-1; j>=0; j--)
                    {
                        if (vm.g_sp->picnum == BLIMP && dnum == SCRAP1)
                            s = 0;
                        else s = (krand()%3);

                        l = A_InsertSprite(vm.g_sp->sectnum,
                                           vm.g_sp->x+(krand()&255)-128,vm.g_sp->y+(krand()&255)-128,vm.g_sp->z-(8<<8)-(krand()&8191),
                                           dnum+s,vm.g_sp->shade,32+(krand()&15),32+(krand()&15),
                                           krand()&2047,(krand()&127)+32,
                                           -(krand()&2047),vm.g_i,5);
                        if (vm.g_sp->picnum == BLIMP && dnum == SCRAP1)
                            sprite[l].yvel = BlimpSpawnSprites[j%14];
                        else sprite[l].yvel = -1;
                        sprite[l].pal = vm.g_sp->pal;
                    }
                insptr++;
            }
            continue;

        case CON_COUNT:
            insptr++;
            vm.g_t[0] = (int16_t) *insptr++;
            continue;

        case CON_CSTATOR:
            insptr++;
            vm.g_sp->cstat |= (int16_t) *insptr++;
            continue;

        case CON_CLIPDIST:
            insptr++;
            vm.g_sp->clipdist = (int16_t) *insptr++;
            continue;

        case CON_CSTAT:
            insptr++;
            vm.g_sp->cstat = (int16_t) *insptr++;
            continue;

        case CON_SAVENN:
        case CON_SAVE:
            insptr++;
            {
                time_t curtime;

                g_lastSaveSlot = *insptr++;

                if (g_lastSaveSlot > 9)
                    continue;

                if (tw == CON_SAVE || ud.savegame[g_lastSaveSlot][0] == 0)
                {
                    curtime = time(NULL);
                    Bstrcpy(tempbuf,asctime(localtime(&curtime)));
                    clearbufbyte(ud.savegame[g_lastSaveSlot],sizeof(ud.savegame[g_lastSaveSlot]),0);
                    Bsprintf(ud.savegame[g_lastSaveSlot],"Auto");
                    //            for (j=0;j<13;j++)
                    //                Bmemcpy(&ud.savegame[g_lastSaveSlot][j+4],&tempbuf[j+3],sizeof(tempbuf[j+3]));
                    //            ud.savegame[g_lastSaveSlot][j+4] = '\0';
                    Bmemcpy(&ud.savegame[g_lastSaveSlot][4],&tempbuf[3],sizeof(tempbuf[0])*13);
                    ud.savegame[g_lastSaveSlot][17] = '\0';
                }

                OSD_Printf("Saving to slot %d\n",g_lastSaveSlot);

                KB_FlushKeyboardQueue();

                g_screenCapture = 1;
                G_DrawRooms(myconnectindex,65536);
                g_screenCapture = 0;
                if ((g_netServer || ud.multimode > 1))
                    G_SavePlayer(-1-(g_lastSaveSlot));
                else G_SavePlayer(g_lastSaveSlot);

                continue;
            }

        case CON_QUAKE:
            insptr++;
            g_earthquakeTime = (uint8_t)Gv_GetVarX(*insptr++);
            A_PlaySound(EARTHQUAKE,g_player[screenpeek].ps->i);
            continue;

        case CON_IFMOVE:
            insptr++;
            VM_DoConditional(vm.g_t[1] == *insptr);
            continue;

        case CON_RESETPLAYER:
        {
            insptr++;

            //AddLog("resetplayer");
            if ((!g_netServer && ud.multimode < 2))
            {
                if (g_lastSaveSlot >= 0 && ud.recstat != 2)
                {
                    g_player[vm.g_p].ps->gm |= MODE_MENU;
                    KB_ClearKeyDown(sc_Space);
                    ChangeToMenu(15000);
                }
                else g_player[vm.g_p].ps->gm = MODE_RESTART;
                vm.g_flags |= VM_NOEXECUTE;
            }
            else
            {
                if (vm.g_p == myconnectindex)
                {
                    g_cameraDistance = 0;
                    g_cameraClock = totalclock;
                }

                if (g_netServer)
                {
                    P_ResetPlayer(vm.g_p);

                    packbuf[0] = PACKET_PLAYER_SPAWN;
                    packbuf[1] = vm.g_p;
                    packbuf[2] = 0;

                    enet_host_broadcast(g_netServer, CHAN_GAMESTATE , enet_packet_create(packbuf, 3, ENET_PACKET_FLAG_RELIABLE));
                }
            }
            P_UpdateScreenPal(g_player[vm.g_p].ps);
            //AddLog("EOF: resetplayer");
        }
        continue;

        case CON_IFONWATER:
            VM_DoConditional(sector[vm.g_sp->sectnum].lotag == 1 && klabs(vm.g_sp->z-sector[vm.g_sp->sectnum].floorz) < (32<<8));
            continue;

        case CON_IFINWATER:
            VM_DoConditional(sector[vm.g_sp->sectnum].lotag == 2);
            continue;

        case CON_IFCOUNT:
            insptr++;
            VM_DoConditional(vm.g_t[0] >= *insptr);
            continue;

        case CON_IFACTOR:
            insptr++;
            VM_DoConditional(vm.g_sp->picnum == *insptr);
            continue;

        case CON_RESETCOUNT:
            insptr++;
            vm.g_t[0] = 0;
            continue;

        case CON_ADDINVENTORY:
            insptr += 2;
            switch (*(insptr-1))
            {
            case GET_STEROIDS:
                g_player[vm.g_p].ps->inv_amount[GET_STEROIDS] = *insptr;
                g_player[vm.g_p].ps->inven_icon = 2;
                break;

            case GET_SHIELD:
                g_player[vm.g_p].ps->inv_amount[GET_SHIELD] += *insptr;// 100;
                if (g_player[vm.g_p].ps->inv_amount[GET_SHIELD] > g_player[vm.g_p].ps->max_shield_amount)
                    g_player[vm.g_p].ps->inv_amount[GET_SHIELD] = g_player[vm.g_p].ps->max_shield_amount;
                break;

            case GET_SCUBA:
                g_player[vm.g_p].ps->inv_amount[GET_SCUBA] = *insptr;// 1600;
                g_player[vm.g_p].ps->inven_icon = 6;
                break;

            case GET_HOLODUKE:
                g_player[vm.g_p].ps->inv_amount[GET_HOLODUKE] = *insptr;// 1600;
                g_player[vm.g_p].ps->inven_icon = 3;
                break;

            case GET_JETPACK:
                g_player[vm.g_p].ps->inv_amount[GET_JETPACK] = *insptr;// 1600;
                g_player[vm.g_p].ps->inven_icon = 4;
                break;

            case GET_ACCESS:
                switch (vm.g_sp->pal)
                {
                case  0:
                    g_player[vm.g_p].ps->got_access |= 1;
                    break;
                case 21:
                    g_player[vm.g_p].ps->got_access |= 2;
                    break;
                case 23:
                    g_player[vm.g_p].ps->got_access |= 4;
                    break;
                }
                break;

            case GET_HEATS:
                g_player[vm.g_p].ps->inv_amount[GET_HEATS] = *insptr;
                g_player[vm.g_p].ps->inven_icon = 5;
                break;

            case GET_FIRSTAID:
                g_player[vm.g_p].ps->inven_icon = 1;
                g_player[vm.g_p].ps->inv_amount[GET_FIRSTAID] = *insptr;
                break;

            case GET_BOOTS:
                g_player[vm.g_p].ps->inven_icon = 7;
                g_player[vm.g_p].ps->inv_amount[GET_BOOTS] = *insptr;
                break;
            default:
                OSD_Printf(CON_ERROR "Invalid inventory ID %d\n",g_errorLineNum,keyw[g_tw],*(insptr-1));
                break;
            }
            insptr++;
            continue;

        case CON_HITRADIUSVAR:
            insptr++;
            {
                int32_t v1=Gv_GetVarX(*insptr++),v2=Gv_GetVarX(*insptr++),v3=Gv_GetVarX(*insptr++);
                int32_t v4=Gv_GetVarX(*insptr++),v5=Gv_GetVarX(*insptr++);
                A_RadiusDamage(vm.g_i,v1,v2,v3,v4,v5);
            }
            continue;

        case CON_HITRADIUS:
            A_RadiusDamage(vm.g_i,*(insptr+1),*(insptr+2),*(insptr+3),*(insptr+4),*(insptr+5));
            insptr += 6;
            continue;

        case CON_IFP:
        {
            int32_t l = *(++insptr);
            int32_t j = 0;
            int32_t s = sprite[g_player[vm.g_p].ps->i].xvel;

            if ((l&8) && g_player[vm.g_p].ps->on_ground && TEST_SYNC_KEY(g_player[vm.g_p].sync->bits, SK_CROUCH))
                j = 1;
            else if ((l&16) && g_player[vm.g_p].ps->jumping_counter == 0 && !g_player[vm.g_p].ps->on_ground &&
                     g_player[vm.g_p].ps->posvel.z > 2048)
                j = 1;
            else if ((l&32) && g_player[vm.g_p].ps->jumping_counter > 348)
                j = 1;
            else if ((l&1) && s >= 0 && s < 8)
                j = 1;
            else if ((l&2) && s >= 8 && !TEST_SYNC_KEY(g_player[vm.g_p].sync->bits, SK_RUN))
                j = 1;
            else if ((l&4) && s >= 8 && TEST_SYNC_KEY(g_player[vm.g_p].sync->bits, SK_RUN))
                j = 1;
            else if ((l&64) && g_player[vm.g_p].ps->pos.z < (vm.g_sp->z-(48<<8)))
                j = 1;
            else if ((l&128) && s <= -8 && !TEST_SYNC_KEY(g_player[vm.g_p].sync->bits, SK_RUN))
                j = 1;
            else if ((l&256) && s <= -8 && TEST_SYNC_KEY(g_player[vm.g_p].sync->bits, SK_RUN))
                j = 1;
            else if ((l&512) && (g_player[vm.g_p].ps->quick_kick > 0 || (g_player[vm.g_p].ps->curr_weapon == KNEE_WEAPON && g_player[vm.g_p].ps->kickback_pic > 0)))
                j = 1;
            else if ((l&1024) && sprite[g_player[vm.g_p].ps->i].xrepeat < 32)
                j = 1;
            else if ((l&2048) && g_player[vm.g_p].ps->jetpack_on)
                j = 1;
            else if ((l&4096) && g_player[vm.g_p].ps->inv_amount[GET_STEROIDS] > 0 && g_player[vm.g_p].ps->inv_amount[GET_STEROIDS] < 400)
                j = 1;
            else if ((l&8192) && g_player[vm.g_p].ps->on_ground)
                j = 1;
            else if ((l&16384) && sprite[g_player[vm.g_p].ps->i].xrepeat > 32 && sprite[g_player[vm.g_p].ps->i].extra > 0 && g_player[vm.g_p].ps->timebeforeexit == 0)
                j = 1;
            else if ((l&32768) && sprite[g_player[vm.g_p].ps->i].extra <= 0)
                j = 1;
            else if ((l&65536L))
            {
                if (vm.g_sp->picnum == APLAYER && (g_netServer || ud.multimode > 1))
                    j = G_GetAngleDelta(g_player[otherp].ps->ang,getangle(g_player[vm.g_p].ps->pos.x-g_player[otherp].ps->pos.x,g_player[vm.g_p].ps->pos.y-g_player[otherp].ps->pos.y));
                else
                    j = G_GetAngleDelta(g_player[vm.g_p].ps->ang,getangle(vm.g_sp->x-g_player[vm.g_p].ps->pos.x,vm.g_sp->y-g_player[vm.g_p].ps->pos.y));

                if (j > -128 && j < 128)
                    j = 1;
                else
                    j = 0;
            }
            VM_DoConditional((intptr_t) j);
        }
        continue;

        case CON_IFSTRENGTH:
            insptr++;
            VM_DoConditional(vm.g_sp->extra <= *insptr);
            continue;

        case CON_GUTS:
            A_DoGuts(vm.g_i,*(insptr+1),*(insptr+2));
            insptr += 3;
            continue;

        case CON_IFSPAWNEDBY:
            insptr++;
            VM_DoConditional(actor[vm.g_i].picnum == *insptr);
            continue;

        case CON_WACKPLAYER:
            insptr++;
            P_ForceAngle(g_player[vm.g_p].ps);
            continue;

        case CON_FLASH:
            insptr++;
            sprite[vm.g_i].shade = -127;
            g_player[vm.g_p].ps->visibility = -127;
            lastvisinc = totalclock+32;
            continue;

        case CON_SAVEMAPSTATE:
            if (MapInfo[ud.volume_number *MAXLEVELS+ud.level_number].savedstate == NULL)
                MapInfo[ud.volume_number *MAXLEVELS+ud.level_number].savedstate = Bcalloc(1,sizeof(mapstate_t));
            G_SaveMapState(MapInfo[ud.volume_number*MAXLEVELS+ud.level_number].savedstate);
            insptr++;
            continue;

        case CON_LOADMAPSTATE:
            if (MapInfo[ud.volume_number*MAXLEVELS+ud.level_number].savedstate)
                G_RestoreMapState(MapInfo[ud.volume_number*MAXLEVELS+ud.level_number].savedstate);
            insptr++;
            continue;

        case CON_CLEARMAPSTATE:
            insptr++;
            {
                int32_t j = Gv_GetVarX(*insptr++);
                if ((j < 0 || j >= MAXVOLUMES*MAXLEVELS))
                {
                    OSD_Printf(CON_ERROR "Invalid map number: %d\n",g_errorLineNum,keyw[g_tw],j);
                    continue;
                }
                if (MapInfo[j].savedstate)
                    G_FreeMapState(j);
            }
            continue;

        case CON_STOPALLSOUNDS:
            insptr++;
            if (screenpeek == vm.g_p)
                FX_StopAllSounds();
            continue;

        case CON_IFGAPZL:
            insptr++;
            VM_DoConditional(((actor[vm.g_i].floorz - actor[vm.g_i].ceilingz) >> 8) < *insptr);
            continue;

        case CON_IFHITSPACE:
            VM_DoConditional(TEST_SYNC_KEY(g_player[vm.g_p].sync->bits, SK_OPEN));
            continue;

        case CON_IFOUTSIDE:
            VM_DoConditional(sector[vm.g_sp->sectnum].ceilingstat&1);
            continue;

        case CON_IFMULTIPLAYER:
            VM_DoConditional((g_netServer || g_netClient || ud.multimode > 1));
            continue;

        case CON_IFCLIENT:
            VM_DoConditional(g_netClient != NULL);
            continue;

        case CON_IFSERVER:
            VM_DoConditional(g_netServer != NULL);
            continue;

        case CON_OPERATE:
            insptr++;
            if (sector[vm.g_sp->sectnum].lotag == 0)
            {
                neartag(vm.g_sp->x,vm.g_sp->y,vm.g_sp->z-(32<<8),vm.g_sp->sectnum,vm.g_sp->ang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,768L,1);
                if (neartagsector >= 0 && isanearoperator(sector[neartagsector].lotag))
                    if ((sector[neartagsector].lotag&0xff) == 23 || sector[neartagsector].floorz == sector[neartagsector].ceilingz)
                        if ((sector[neartagsector].lotag&16384) == 0)
                            if ((sector[neartagsector].lotag&32768) == 0)
                            {
                                int32_t j = headspritesect[neartagsector];
                                while (j >= 0)
                                {
                                    if (sprite[j].picnum == ACTIVATOR)
                                        break;
                                    j = nextspritesect[j];
                                }
                                if (j == -1)
                                    G_OperateSectors(neartagsector,vm.g_i);
                            }
            }
            continue;

        case CON_IFINSPACE:
            VM_DoConditional(G_CheckForSpaceCeiling(vm.g_sp->sectnum));
            continue;

        case CON_SPRITEPAL:
            insptr++;
            if (vm.g_sp->picnum != APLAYER)
                actor[vm.g_i].tempang = vm.g_sp->pal;
            vm.g_sp->pal = *insptr++;
            continue;

        case CON_CACTOR:
            insptr++;
            vm.g_sp->picnum = *insptr++;
            continue;

        case CON_IFBULLETNEAR:
            VM_DoConditional(A_Dodge(vm.g_sp) == 1);
            continue;

        case CON_IFRESPAWN:
            if (A_CheckEnemySprite(vm.g_sp))
                VM_DoConditional(ud.respawn_monsters);
            else if (A_CheckInventorySprite(vm.g_sp))
                VM_DoConditional(ud.respawn_inventory);
            else
                VM_DoConditional(ud.respawn_items);
            continue;

        case CON_IFFLOORDISTL:
            insptr++;
            VM_DoConditional((actor[vm.g_i].floorz - vm.g_sp->z) <= ((*insptr)<<8));
            continue;

        case CON_IFCEILINGDISTL:
            insptr++;
            VM_DoConditional((vm.g_sp->z - actor[vm.g_i].ceilingz) <= ((*insptr)<<8));
            continue;

        case CON_PALFROM:
            insptr++;
            {
                int32_t j = 2;
                g_player[vm.g_p].ps->pals.f = *insptr++;
                for (; j>=0; j--)
                    *((char *)(&g_player[vm.g_p].ps->pals.r)+2-j) = *insptr++;
            }
            continue;

        case CON_SECTOROFWALL:
            insptr++;
            {
                int32_t j = *insptr++;

                Gv_SetVarX(j, sectorofwall(Gv_GetVarX(*insptr++)));
            }
            continue;

        case CON_QSPRINTF:
            insptr++;
            {
                int32_t dq = Gv_GetVarX(*insptr++), sq = Gv_GetVarX(*insptr++);
                if ((ScriptQuotes[sq] == NULL || ScriptQuotes[dq] == NULL))
                {
                    OSD_Printf(CON_ERROR "null quote %d\n",g_errorLineNum,keyw[g_tw],ScriptQuotes[sq] ? dq : sq);

                    while ((*insptr & 0xFFF) != CON_NULLOP)
                        Gv_GetVarX(*insptr++);

                    insptr++; // skip the NOP
                    continue;
                }

                {
                    int32_t arg[32], i = 0, j = 0, k = 0;
                    int32_t len = Bstrlen(ScriptQuotes[sq]);
                    char tempbuf[MAXQUOTELEN];

                    while ((*insptr & 0xFFF) != CON_NULLOP && i < 32)
                        arg[i++] = Gv_GetVarX(*insptr++);

                    insptr++; // skip the NOP

                    i = 0;

                    do
                    {
                        while (k < len && j < MAXQUOTELEN && ScriptQuotes[sq][k] != '%')
                            tempbuf[j++] = ScriptQuotes[sq][k++];

                        if (ScriptQuotes[sq][k] == '%')
                        {
                            k++;
                            switch (ScriptQuotes[sq][k])
                            {
                            case 'l':
                                if (ScriptQuotes[sq][k+1] != 'd')
                                {
                                    // write the % and l
                                    tempbuf[j++] = ScriptQuotes[sq][k-1];
                                    tempbuf[j++] = ScriptQuotes[sq][k++];
                                    break;
                                }
                                k++;
                            case 'd':
                            {
                                char buf[16];
                                int32_t ii = 0;

                                Bsprintf(buf, "%d", arg[i++]);

                                ii = Bstrlen(buf);
                                Bmemcpy(&tempbuf[j], buf, ii);
                                j += ii;
                                k++;
                            }
                            break;

                            case 's':
                            {
                                int32_t ii = Bstrlen(ScriptQuotes[arg[i]]);

                                Bmemcpy(&tempbuf[j], ScriptQuotes[arg[i]], ii);
                                j += ii;
                                k++;
                            }
                            break;

                            default:
                                tempbuf[j++] = ScriptQuotes[sq][k-1];
                                break;
                            }
                        }
                    }
                    while (k < len && j < MAXQUOTELEN);

                    tempbuf[j] = '\0';
                    Bstrcpy(ScriptQuotes[dq], tempbuf);
                    continue;
                }
            }

        case CON_ADDLOG:
        {
            insptr++;

            OSD_Printf(OSDTEXT_GREEN "CONLOG: L=%d\n",g_errorLineNum);
            continue;
        }

        case CON_ADDLOGVAR:
            insptr++;
            {
                int32_t m=1;
                char szBuf[256];
                int32_t lVarID = *insptr;

                if ((lVarID >= g_gameVarCount) || lVarID < 0)
                {
                    if (*insptr==MAXGAMEVARS) // addlogvar for a constant?  Har.
                        insptr++;
                    //                else if (*insptr > g_gameVarCount && (*insptr < (MAXGAMEVARS<<1)+MAXGAMEVARS+1+MAXGAMEARRAYS))
                    else if (*insptr&(MAXGAMEVARS<<2))
                    {
                        int32_t index;

                        lVarID ^= (MAXGAMEVARS<<2);

                        if (lVarID&(MAXGAMEVARS<<1))
                        {
                            m = -m;
                            lVarID ^= (MAXGAMEVARS<<1);
                        }

                        insptr++;

                        index=Gv_GetVarX(*insptr++);
                        if ((index < aGameArrays[lVarID].size)&&(index>=0))
                        {
                            OSD_Printf(OSDTEXT_GREEN "%s: L=%d %s[%d] =%d\n",keyw[g_tw],g_errorLineNum,
                                       aGameArrays[lVarID].szLabel,index,m*aGameArrays[lVarID].plValues[index]);
                            continue;
                        }
                        else
                        {
                            OSD_Printf(CON_ERROR "invalid array index\n",g_errorLineNum,keyw[g_tw]);
                            continue;
                        }
                    }
                    else if (*insptr&(MAXGAMEVARS<<3))
                    {
                        //                    FIXME FIXME FIXME
                        if ((lVarID & (MAXGAMEVARS-1)) == g_iActorVarID)
                        {
                            intptr_t *oinsptr = insptr++;
                            int32_t index = Gv_GetVarX(*insptr++);
                            insptr = oinsptr;
                            if (index < 0 || index >= MAXSPRITES-1)
                            {
                                OSD_Printf(CON_ERROR "invalid array index\n",g_errorLineNum,keyw[g_tw]);
                                Gv_GetVarX(*insptr++);
                                continue;
                            }
                            OSD_Printf(OSDTEXT_GREEN "%s: L=%d %d %d\n",keyw[g_tw],g_errorLineNum,index,Gv_GetVar(*insptr++,index,vm.g_p));
                            continue;
                        }
                    }
                    else if (*insptr&(MAXGAMEVARS<<1))
                    {
                        m = -m;
                        lVarID ^= (MAXGAMEVARS<<1);
                    }
                    else
                    {
                        // invalid varID
                        insptr++;
                        OSD_Printf(CON_ERROR "invalid variable\n",g_errorLineNum,keyw[g_tw]);
                        continue;  // out of switch
                    }
                }
                Bsprintf(szBuf,"CONLOGVAR: L=%d %s ",g_errorLineNum, aGameVars[lVarID].szLabel);
                strcpy(g_szBuf,szBuf);

                if (aGameVars[lVarID].dwFlags & GAMEVAR_READONLY)
                {
                    Bsprintf(szBuf," (read-only)");
                    strcat(g_szBuf,szBuf);
                }
                if (aGameVars[lVarID].dwFlags & GAMEVAR_PERPLAYER)
                {
                    Bsprintf(szBuf," (Per Player. Player=%d)",vm.g_p);
                }
                else if (aGameVars[lVarID].dwFlags & GAMEVAR_PERACTOR)
                {
                    Bsprintf(szBuf," (Per Actor. Actor=%d)",vm.g_i);
                }
                else
                {
                    Bsprintf(szBuf," (Global)");
                }
                Bstrcat(g_szBuf,szBuf);
                Bsprintf(szBuf," =%d\n", Gv_GetVarX(lVarID)*m);
                Bstrcat(g_szBuf,szBuf);
                OSD_Printf(OSDTEXT_GREEN "%s",g_szBuf);
                insptr++;
                continue;
            }

        case CON_SETSECTOR:
        case CON_GETSECTOR:
            insptr++;
            {
                // syntax [gs]etsector[<var>].x <VAR>
                // <varid> <xxxid> <varid>
                int32_t lVar1=*insptr++, lLabelID=*insptr++, lVar2=*insptr++;

                VM_AccessSector(tw==CON_SETSECTOR, lVar1, lLabelID, lVar2);
                continue;
            }

        case CON_SQRT:
            insptr++;
            {
                // syntax sqrt <invar> <outvar>
                int32_t lInVarID=*insptr++, lOutVarID=*insptr++;

                Gv_SetVarX(lOutVarID, ksqrt(Gv_GetVarX(lInVarID)));
                continue;
            }

        case CON_FINDNEARACTOR:
        case CON_FINDNEARSPRITE:
        case CON_FINDNEARACTOR3D:
        case CON_FINDNEARSPRITE3D:
            insptr++;
            {
                // syntax findnearactorvar <type> <maxdist> <getvar>
                // gets the sprite ID of the nearest actor within max dist
                // that is of <type> into <getvar>
                // -1 for none found
                // <type> <maxdist> <varid>
                int32_t lType=*insptr++, lMaxDist=*insptr++, lVarID=*insptr++;
                int32_t lFound=-1, j, k = MAXSTATUS-1;

                if (tw == CON_FINDNEARACTOR || tw == CON_FINDNEARACTOR3D)
                    k = 1;
                do
                {
                    j=headspritestat[k];    // all sprites
                    if (tw==CON_FINDNEARSPRITE3D || tw==CON_FINDNEARACTOR3D)
                    {
                        while (j>=0)
                        {
                            if (sprite[j].picnum == lType && j != vm.g_i && dist(&sprite[vm.g_i], &sprite[j]) < lMaxDist)
                            {
                                lFound=j;
                                j = MAXSPRITES;
                                break;
                            }
                            j = nextspritestat[j];
                        }
                        if (j == MAXSPRITES || tw == CON_FINDNEARACTOR3D)
                            break;
                        continue;
                    }

                    while (j>=0)
                    {
                        if (sprite[j].picnum == lType && j != vm.g_i && ldist(&sprite[vm.g_i], &sprite[j]) < lMaxDist)
                        {
                            lFound=j;
                            j = MAXSPRITES;
                            break;
                        }
                        j = nextspritestat[j];
                    }

                    if (j == MAXSPRITES || tw == CON_FINDNEARACTOR)
                        break;
                }
                while (k--);
                Gv_SetVarX(lVarID, lFound);
                continue;
            }

        case CON_FINDNEARACTORVAR:
        case CON_FINDNEARSPRITEVAR:
        case CON_FINDNEARACTOR3DVAR:
        case CON_FINDNEARSPRITE3DVAR:
            insptr++;
            {
                // syntax findnearactorvar <type> <maxdistvar> <getvar>
                // gets the sprite ID of the nearest actor within max dist
                // that is of <type> into <getvar>
                // -1 for none found
                // <type> <maxdistvarid> <varid>
                int32_t lType=*insptr++, lMaxDist=Gv_GetVarX(*insptr++), lVarID=*insptr++;
                int32_t lFound=-1, j, k = 1;

                if (tw == CON_FINDNEARSPRITEVAR || tw == CON_FINDNEARSPRITE3DVAR)
                    k = MAXSTATUS-1;

                do
                {
                    j=headspritestat[k];    // all sprites
                    if (tw==CON_FINDNEARACTOR3DVAR || tw==CON_FINDNEARSPRITE3DVAR)
                    {
                        while (j >= 0)
                        {
                            if (sprite[j].picnum == lType && j != vm.g_i && dist(&sprite[vm.g_i], &sprite[j]) < lMaxDist)
                            {
                                lFound=j;
                                j = MAXSPRITES;
                                break;
                            }
                            j = nextspritestat[j];
                        }
                        if (j == MAXSPRITES || tw==CON_FINDNEARACTOR3DVAR)
                            break;
                        continue;
                    }

                    while (j >= 0)
                    {
                        if (sprite[j].picnum == lType && j != vm.g_i && ldist(&sprite[vm.g_i], &sprite[j]) < lMaxDist)
                        {
                            lFound=j;
                            j = MAXSPRITES;
                            break;
                        }
                        j = nextspritestat[j];
                    }

                    if (j == MAXSPRITES || tw==CON_FINDNEARACTORVAR)
                        break;
                }
                while (k--);
                Gv_SetVarX(lVarID, lFound);
                continue;
            }

        case CON_FINDNEARACTORZVAR:
        case CON_FINDNEARSPRITEZVAR:
            insptr++;
            {
                // syntax findnearactorvar <type> <maxdistvar> <getvar>
                // gets the sprite ID of the nearest actor within max dist
                // that is of <type> into <getvar>
                // -1 for none found
                // <type> <maxdistvarid> <varid>
                int32_t lType=*insptr++, lMaxDist=Gv_GetVarX(*insptr++);
                int32_t lMaxZDist=Gv_GetVarX(*insptr++);
                int32_t lVarID=*insptr++, lFound=-1, lTemp, lTemp2, j, k=MAXSTATUS-1;
                do
                {
                    j=headspritestat[tw==CON_FINDNEARACTORZVAR?1:k];    // all sprites
                    if (j == -1) continue;
                    do
                    {
                        if (sprite[j].picnum == lType && j != vm.g_i)
                        {
                            lTemp=ldist(&sprite[vm.g_i], &sprite[j]);
                            if (lTemp < lMaxDist)
                            {
                                lTemp2=klabs(sprite[vm.g_i].z-sprite[j].z);
                                if (lTemp2 < lMaxZDist)
                                {
                                    lFound=j;
                                    j = MAXSPRITES;
                                    break;
                                }
                            }
                        }
                        j = nextspritestat[j];
                    }
                    while (j>=0);
                    if (tw==CON_FINDNEARACTORZVAR || j == MAXSPRITES)
                        break;
                }
                while (k--);
                Gv_SetVarX(lVarID, lFound);

                continue;
            }

        case CON_FINDNEARACTORZ:
        case CON_FINDNEARSPRITEZ:
            insptr++;
            {
                // syntax findnearactorvar <type> <maxdist> <getvar>
                // gets the sprite ID of the nearest actor within max dist
                // that is of <type> into <getvar>
                // -1 for none found
                // <type> <maxdist> <varid>
                int32_t lType=*insptr++, lMaxDist=*insptr++, lMaxZDist=*insptr++, lVarID=*insptr++;
                int32_t lTemp, lTemp2, lFound=-1, j, k=MAXSTATUS-1;
                do
                {
                    j=headspritestat[tw==CON_FINDNEARACTORZ?1:k];    // all sprites
                    if (j == -1) continue;
                    do
                    {
                        if (sprite[j].picnum == lType && j != vm.g_i)
                        {
                            lTemp=ldist(&sprite[vm.g_i], &sprite[j]);
                            if (lTemp < lMaxDist)
                            {
                                lTemp2=klabs(sprite[vm.g_i].z-sprite[j].z);
                                if (lTemp2 < lMaxZDist)
                                {
                                    lFound=j;
                                    j = MAXSPRITES;
                                    break;
                                }
                            }
                        }
                        j = nextspritestat[j];
                    }
                    while (j>=0);

                    if (tw==CON_FINDNEARACTORZ || j == MAXSPRITES)
                        break;
                }
                while (k--);
                Gv_SetVarX(lVarID, lFound);
                continue;
            }

        case CON_FINDPLAYER:
            insptr++;
            {
                int32_t j;
                //            Gv_SetVarX(g_iReturnVarID, A_FindPlayer(&sprite[vm.g_i],&j));
                aGameVars[g_iReturnVarID].val.lValue = A_FindPlayer(&sprite[vm.g_i],&j);
                Gv_SetVarX(*insptr++, j);
            }
            continue;

        case CON_FINDOTHERPLAYER:
            insptr++;
            {
                int32_t j;
                //            Gv_SetVarX(g_iReturnVarID, P_FindOtherPlayer(vm.g_p,&j));
                aGameVars[g_iReturnVarID].val.lValue = P_FindOtherPlayer(vm.g_p,&j);
                Gv_SetVarX(*insptr++, j);
            }
            continue;

        case CON_SETPLAYER:
            insptr++;
            {
                // syntax [gs]etplayer[<var>].x <VAR>
                // <varid> <xxxid> <varid>
                int32_t lVar1=*insptr++, lLabelID=*insptr++, lParm2 = 0, lVar2;
                // HACK: need to have access to labels structure at run-time...

                if (PlayerLabels[lLabelID].flags & LABEL_HASPARM2)
                    lParm2=Gv_GetVarX(*insptr++);
                lVar2=*insptr++;

                VM_SetPlayer(lVar1, lLabelID, lVar2, lParm2);
                continue;
            }


        case CON_GETPLAYER:
            insptr++;
            {
                // syntax [gs]etplayer[<var>].x <VAR>
                // <varid> <xxxid> <varid>
                int32_t lVar1=*insptr++, lLabelID=*insptr++, lParm2 = 0, lVar2;
                // HACK: need to have access to labels structure at run-time...

                if (PlayerLabels[lLabelID].flags & LABEL_HASPARM2)
                    lParm2=Gv_GetVarX(*insptr++);
                lVar2=*insptr++;

                VM_GetPlayer(lVar1, lLabelID, lVar2, lParm2);
                continue;
            }

        case CON_SETINPUT:
        case CON_GETINPUT:
            insptr++;
            {
                // syntax [gs]etplayer[<var>].x <VAR>
                // <varid> <xxxid> <varid>
                int32_t lVar1=*insptr++, lLabelID=*insptr++, lVar2=*insptr++;

                VM_AccessPlayerInput(tw==CON_SETINPUT, lVar1, lLabelID, lVar2);
                continue;
            }

        case CON_GETUSERDEF:
        case CON_SETUSERDEF:
            insptr++;
            {
                // syntax [gs]etuserdef.xxx <VAR>
                //  <xxxid> <varid>
                int32_t lLabelID=*insptr++, lVar2=*insptr++;

                VM_AccessUserdef(tw==CON_SETUSERDEF, lLabelID, lVar2);
                continue;
            }

        case CON_GETPROJECTILE:
        case CON_SETPROJECTILE:
            insptr++;
            {
                // syntax [gs]etplayer[<var>].x <VAR>
                // <varid> <xxxid> <varid>
                int32_t lVar1=Gv_GetVarX(*insptr++), lLabelID=*insptr++, lVar2=*insptr++;

                VM_AccessProjectile(tw==CON_SETPROJECTILE,lVar1,lLabelID,lVar2);
                continue;
            }

        case CON_SETWALL:
        case CON_GETWALL:
            insptr++;
            {
                // syntax [gs]etwall[<var>].x <VAR>
                // <varid> <xxxid> <varid>
                int32_t lVar1=*insptr++, lLabelID=*insptr++, lVar2=*insptr++;

                VM_AccessWall(tw==CON_SETWALL, lVar1, lLabelID, lVar2);
                continue;
            }

        case CON_SETACTORVAR:
        case CON_GETACTORVAR:
            insptr++;
            {
                // syntax [gs]etactorvar[<var>].<varx> <VAR>
                // gets the value of the per-actor variable varx into VAR
                // <var> <varx> <VAR>
                int32_t lSprite=Gv_GetVarX(*insptr++), lVar1=*insptr++;
                int32_t lVar2=*insptr++;

                if ((lSprite < 0 || lSprite >= MAXSPRITES))
                {
                    OSD_Printf(CON_ERROR "invalid sprite ID %d\n",g_errorLineNum,keyw[g_tw],lSprite);
                    if (lVar1 == MAXGAMEVARS || lVar1 & ((MAXGAMEVARS<<2)|(MAXGAMEVARS<<3))) insptr++;
                    if (lVar2 == MAXGAMEVARS || lVar2 & ((MAXGAMEVARS<<2)|(MAXGAMEVARS<<3))) insptr++;
                    continue;
                }

                if (tw == CON_SETACTORVAR)
                {
                    Gv_SetVar(lVar1, Gv_GetVarX(lVar2), lSprite, vm.g_p);
                    continue;
                }
                Gv_SetVarX(lVar2, Gv_GetVar(lVar1, lSprite, vm.g_p));
                continue;
            }

        case CON_SETPLAYERVAR:
        case CON_GETPLAYERVAR:
            insptr++;
            {
                int32_t iPlayer = vm.g_p;

                if (*insptr != g_iThisActorID)
                    iPlayer=Gv_GetVarX(*insptr);

                insptr++;
                {
                    int32_t lVar1=*insptr++, lVar2=*insptr++;

                    if ((iPlayer < 0 || iPlayer >= playerswhenstarted))
                    {
                        OSD_Printf(CON_ERROR "invalid player ID %d\n",g_errorLineNum,keyw[g_tw],iPlayer);
                        if (lVar1 == MAXGAMEVARS || lVar1 & ((MAXGAMEVARS<<2)|(MAXGAMEVARS<<3))) insptr++;
                        if (lVar2 == MAXGAMEVARS || lVar2 & ((MAXGAMEVARS<<2)|(MAXGAMEVARS<<3))) insptr++;
                        continue;
                    }

                    if (tw == CON_SETPLAYERVAR)
                    {
                        Gv_SetVar(lVar1, Gv_GetVarX(lVar2), vm.g_i, iPlayer);
                        continue;
                    }
                    Gv_SetVarX(lVar2, Gv_GetVar(lVar1, vm.g_i, iPlayer));
                    continue;
                }
            }

        case CON_SETACTOR:
            insptr++;
            {
                // syntax [gs]etactor[<var>].x <VAR>
                // <varid> <xxxid> <varid>

                int32_t lVar1=*insptr++, lLabelID=*insptr++, lParm2 = 0;

                if (ActorLabels[lLabelID].flags & LABEL_HASPARM2)
                    lParm2=Gv_GetVarX(*insptr++);

                {
                    int32_t lVar2=*insptr++;

                    VM_SetSprite(lVar1, lLabelID, lVar2, lParm2);
                }
                continue;
            }

        case CON_GETACTOR:
            insptr++;
            {
                // syntax [gs]etactor[<var>].x <VAR>
                // <varid> <xxxid> <varid>

                int32_t lVar1=*insptr++, lLabelID=*insptr++, lParm2 = 0;

                if (ActorLabels[lLabelID].flags & LABEL_HASPARM2)
                    lParm2=Gv_GetVarX(*insptr++);

                {
                    int32_t lVar2=*insptr++;

                    VM_GetSprite(lVar1, lLabelID, lVar2, lParm2);
                }
                continue;
            }

        case CON_SETTSPR:
        case CON_GETTSPR:
            insptr++;
            {
                // syntax [gs]etactor[<var>].x <VAR>
                // <varid> <xxxid> <varid>

                int32_t lVar1=*insptr++, lLabelID=*insptr++, lVar2=*insptr++;

                VM_AccessTsprite(tw==CON_SETTSPR, lVar1, lLabelID, lVar2);
                continue;
            }

        case CON_GETANGLETOTARGET:
            insptr++;
            // Actor[vm.g_i].lastvx and lastvy are last known location of target.
            Gv_SetVarX(*insptr++, getangle(actor[vm.g_i].lastvx-vm.g_sp->x,actor[vm.g_i].lastvy-vm.g_sp->y));
            continue;

        case CON_ANGOFFVAR:
            insptr++;
            spriteext[vm.g_i].angoff=Gv_GetVarX(*insptr++);
            continue;

        case CON_LOCKPLAYER:
            insptr++;
            g_player[vm.g_p].ps->transporter_hold=Gv_GetVarX(*insptr++);
            continue;

        case CON_CHECKAVAILWEAPON:
        case CON_CHECKAVAILINVEN:
            insptr++;
            {
                int32_t j = vm.g_p;

                if (*insptr != g_iThisActorID)
                    j=Gv_GetVarX(*insptr);

                insptr++;

                if ((j < 0 || j >= playerswhenstarted))
                {
                    OSD_Printf(CON_ERROR "Invalid player ID %d\n",g_errorLineNum,keyw[g_tw],j);
                    continue;
                }

                if (tw == CON_CHECKAVAILWEAPON)
                    P_CheckWeapon(g_player[j].ps);
                else P_SelectNextInvItem(g_player[j].ps);
            }
            continue;

        case CON_GETPLAYERANGLE:
            insptr++;
            Gv_SetVarX(*insptr++, g_player[vm.g_p].ps->ang);
            continue;

        case CON_SETPLAYERANGLE:
            insptr++;
            g_player[vm.g_p].ps->ang=Gv_GetVarX(*insptr++);
            g_player[vm.g_p].ps->ang &= 2047;
            continue;

        case CON_GETACTORANGLE:
            insptr++;
            Gv_SetVarX(*insptr++, vm.g_sp->ang);
            continue;

        case CON_SETACTORANGLE:
            insptr++;
            vm.g_sp->ang=Gv_GetVarX(*insptr++);
            vm.g_sp->ang &= 2047;
            continue;

        case CON_SETVAR:
            insptr++;
            Gv_SetVarX(*insptr, *(insptr+1));
            insptr += 2;
            continue;

        case CON_SETARRAY:
            insptr++;
            {
                int32_t j=*insptr++;
                int32_t index = Gv_GetVarX(*insptr++);
                int32_t value = Gv_GetVarX(*insptr++);

                if (j<0 || j >= g_gameArrayCount || index >= aGameArrays[j].size || index < 0)
                {
                    OSD_Printf(OSD_ERROR "Gv_SetVar(): tried to set invalid array ID (%d) or index out of bounds from sprite %d (%d), player %d\n",j,vm.g_i,sprite[vm.g_i].picnum,vm.g_p);
                    continue;
                }
                aGameArrays[j].plValues[index]=value;
                continue;
            }
        case CON_WRITEARRAYTOFILE:
        case CON_READARRAYFROMFILE:
            insptr++;
            {
                int32_t j=*insptr++;
                {
                    int q = *insptr++;

                    if (ScriptQuotes[q] == NULL)
                    {
                        OSD_Printf(CON_ERROR "null quote %d\n",g_errorLineNum,keyw[g_tw],q);
                        continue;
                    }

                    if (tw == CON_READARRAYFROMFILE)
                    {
                        int32_t fil = kopen4loadfrommod(ScriptQuotes[q], 0);
                        int32_t asize;

                        if (fil < 0)
                            continue;

                        asize = kfilelength(fil);

                        if (asize > 0)
                        {
                            /*OSD_Printf(OSDTEXT_GREEN "CON_RESIZEARRAY: resizing array %s from %d to %d\n",
                                aGameArrays[j].szLabel, aGameArrays[j].size, asize / sizeof(int32_t));*/
                            aGameArrays[j].plValues=Brealloc(aGameArrays[j].plValues, asize);
                            aGameArrays[j].size = asize / sizeof(int32_t);
                            kread(fil, aGameArrays[j].plValues, asize);
                        }

                        kclose(fil);
                        continue;
                    }

                    {
                        FILE *fil;
                        char temp[BMAX_PATH];

                        if (g_modDir[0] != '/')
                            Bsprintf(temp,"%s/%s",g_modDir,ScriptQuotes[q]);
                        else Bsprintf(temp,"%s",ScriptQuotes[q]);

                        if ((fil = fopen(temp,"wb")) == 0) continue;

                        fwrite(aGameArrays[j].plValues,1,sizeof(int) * aGameArrays[j].size,fil);
                        fclose(fil);
                    }

                    continue;
                }
            }

        case CON_GETARRAYSIZE:
            insptr++;
            {
                int32_t j=*insptr++;
                Gv_SetVarX(*insptr++,aGameArrays[j].size);
            }
            continue;

        case CON_RESIZEARRAY:
            insptr++;
            {
                int32_t j=*insptr++;
                int32_t asize = Gv_GetVarX(*insptr++);

                if (asize > 0)
                {
                    /*OSD_Printf(OSDTEXT_GREEN "CON_RESIZEARRAY: resizing array %s from %d to %d\n", aGameArrays[j].szLabel, aGameArrays[j].size, asize);*/
                    aGameArrays[j].plValues=Brealloc(aGameArrays[j].plValues, sizeof(int32_t) * asize);
                    aGameArrays[j].size = asize;
                }
                continue;
            }

        case CON_COPY:
            insptr++;
            {
                int32_t j=*insptr++;
                int32_t index = Gv_GetVar(*insptr++, vm.g_i, vm.g_p);
                int32_t j1=*insptr++;
                int32_t index1 = Gv_GetVar(*insptr++, vm.g_i, vm.g_p);
                int32_t value = Gv_GetVar(*insptr++, vm.g_i, vm.g_p);

                if (index > aGameArrays[j].size || index1 > aGameArrays[j1].size) continue;
                if ((index+value)>aGameArrays[j].size) value=aGameArrays[j].size-index;
                if ((index1+value)>aGameArrays[j1].size) value=aGameArrays[j1].size-index1;
                Bmemcpy(aGameArrays[j1].plValues+index1, aGameArrays[j].plValues+index, value * sizeof(intptr_t));
                continue;
            }

        case CON_RANDVAR:
            insptr++;
            Gv_SetVarX(*insptr, mulscale16(krand(), *(insptr+1)+1));
            insptr += 2;
            continue;

        case CON_DISPLAYRANDVAR:
            insptr++;
            Gv_SetVarX(*insptr, mulscale15((uint16_t)rand(), *(insptr+1)+1));
            insptr += 2;
            continue;

        case CON_INV:
            Gv_SetVarX(*(insptr+1), -Gv_GetVarX(*(insptr+1)));
            insptr += 2;
            continue;

        case CON_MULVAR:
            insptr++;
            Gv_SetVarX(*insptr, Gv_GetVarX(*insptr) * *(insptr+1));
            insptr += 2;
            continue;

        case CON_DIVVAR:
            insptr++;
            if (*(insptr+1) == 0)
            {
                OSD_Printf(CON_ERROR "Divide by zero.\n",g_errorLineNum,keyw[g_tw]);
                insptr += 2;
                continue;
            }
            Gv_SetVarX(*insptr, Gv_GetVarX(*insptr) / *(insptr+1));
            insptr += 2;
            continue;

        case CON_MODVAR:
            insptr++;
            if (*(insptr+1) == 0)
            {
                OSD_Printf(CON_ERROR "Mod by zero.\n",g_errorLineNum,keyw[g_tw]);
                insptr += 2;
                continue;
            }
            Gv_SetVarX(*insptr,Gv_GetVarX(*insptr)%*(insptr+1));
            insptr += 2;
            continue;

        case CON_ANDVAR:
            insptr++;
            Gv_SetVarX(*insptr,Gv_GetVarX(*insptr) & *(insptr+1));
            insptr += 2;
            continue;

        case CON_ORVAR:
            insptr++;
            Gv_SetVarX(*insptr,Gv_GetVarX(*insptr) | *(insptr+1));
            insptr += 2;
            continue;

        case CON_XORVAR:
            insptr++;
            Gv_SetVarX(*insptr,Gv_GetVarX(*insptr) ^ *(insptr+1));
            insptr += 2;
            continue;

        case CON_SETVARVAR:
            insptr++;
            {
                int32_t j=*insptr++;
                Gv_SetVarX(j, Gv_GetVarX(*insptr++));
            }
            continue;

        case CON_RANDVARVAR:
            insptr++;
            {
                int32_t j=*insptr++;
                Gv_SetVarX(j,mulscale(krand(), Gv_GetVarX(*insptr++)+1, 16));
            }
            continue;

        case CON_DISPLAYRANDVARVAR:
            insptr++;
            {
                int32_t j=*insptr++;
                Gv_SetVarX(j,mulscale((uint16_t)rand(), Gv_GetVarX(*insptr++)+1, 15));
            }
            continue;

        case CON_GMAXAMMO:
            insptr++;
            {
                int32_t j=Gv_GetVarX(*insptr++);
                if ((j<0 || j>=MAX_WEAPONS))
                {
                    OSD_Printf(CON_ERROR "Invalid weapon ID %d\n",g_errorLineNum,keyw[g_tw],j);
                    insptr++;
                    continue;
                }
                Gv_SetVarX(*insptr++, g_player[vm.g_p].ps->max_ammo_amount[j]);
            }
            continue;

        case CON_SMAXAMMO:
            insptr++;
            {
                int32_t j=Gv_GetVarX(*insptr++);
                if ((j<0 || j>=MAX_WEAPONS))
                {
                    OSD_Printf(CON_ERROR "Invalid weapon ID %d\n",g_errorLineNum,keyw[g_tw],j);
                    insptr++;
                    continue;
                }
                g_player[vm.g_p].ps->max_ammo_amount[j]=Gv_GetVarX(*insptr++);
            }
            continue;

        case CON_MULVARVAR:
            insptr++;
            {
                int32_t j=*insptr++;
                Gv_SetVarX(j, Gv_GetVarX(j)*Gv_GetVarX(*insptr++));
            }
            continue;

        case CON_DIVVARVAR:
            insptr++;
            {
                int32_t j=*insptr++;
                int32_t l2=Gv_GetVarX(*insptr++);

                if (l2==0)
                {
                    OSD_Printf(CON_ERROR "Divide by zero.\n",g_errorLineNum,keyw[g_tw]);
                    continue;
                }
                Gv_SetVarX(j, Gv_GetVarX(j)/l2);
                continue;
            }

        case CON_MODVARVAR:
            insptr++;
            {
                int32_t j=*insptr++;
                int32_t l2=Gv_GetVarX(*insptr++);

                if (l2==0)
                {
                    OSD_Printf(CON_ERROR "Mod by zero.\n",g_errorLineNum,keyw[g_tw]);
                    continue;
                }

                Gv_SetVarX(j, Gv_GetVarX(j) % l2);
                continue;
            }

        case CON_ANDVARVAR:
            insptr++;
            {
                int32_t j=*insptr++;
                Gv_SetVarX(j, Gv_GetVarX(j) & Gv_GetVarX(*insptr++));
            }
            continue;

        case CON_XORVARVAR:
            insptr++;
            {
                int32_t j=*insptr++;
                Gv_SetVarX(j, Gv_GetVarX(j) ^ Gv_GetVarX(*insptr++));
            }
            continue;

        case CON_ORVARVAR:
            insptr++;
            {
                int32_t j=*insptr++;
                Gv_SetVarX(j, Gv_GetVarX(j) | Gv_GetVarX(*insptr++));
            }
            continue;

        case CON_SUBVAR:
            insptr++;
            Gv_SetVarX(*insptr, Gv_GetVarX(*insptr) - *(insptr+1));
            insptr += 2;
            continue;

        case CON_SUBVARVAR:
            insptr++;
            {
                int32_t j=*insptr++;
                Gv_SetVarX(j, Gv_GetVarX(j) - Gv_GetVarX(*insptr++));
            }
            continue;

        case CON_ADDVAR:
            insptr++;
            Gv_SetVarX(*insptr, Gv_GetVarX(*insptr) + *(insptr+1));
            insptr += 2;
            continue;

        case CON_SHIFTVARL:
            insptr++;
            Gv_SetVarX(*insptr, Gv_GetVarX(*insptr) << *(insptr+1));
            insptr += 2;
            continue;

        case CON_SHIFTVARR:
            insptr++;
            Gv_SetVarX(*insptr, Gv_GetVarX(*insptr) >> *(insptr+1));
            insptr += 2;
            continue;

        case CON_SIN:
            insptr++;
            Gv_SetVarX(*insptr, sintable[Gv_GetVarX(*(insptr+1))&2047]);
            insptr += 2;
            continue;

        case CON_COS:
            insptr++;
            Gv_SetVarX(*insptr, sintable[(Gv_GetVarX(*(insptr+1))+512)&2047]);
            insptr += 2;
            continue;

        case CON_ADDVARVAR:
            insptr++;
            {
                int32_t j=*insptr++;
                Gv_SetVarX(j, Gv_GetVarX(j) + Gv_GetVarX(*insptr++));
            }
            continue;

        case CON_SPGETLOTAG:
            insptr++;
            Gv_SetVarX(g_iLoTagID, vm.g_sp->lotag);
            continue;

        case CON_SPGETHITAG:
            insptr++;
            Gv_SetVarX(g_iHiTagID, vm.g_sp->hitag);
            continue;

        case CON_SECTGETLOTAG:
            insptr++;
            Gv_SetVarX(g_iLoTagID, sector[vm.g_sp->sectnum].lotag);
            continue;

        case CON_SECTGETHITAG:
            insptr++;
            Gv_SetVarX(g_iHiTagID, sector[vm.g_sp->sectnum].hitag);
            continue;

        case CON_GETTEXTUREFLOOR:
            insptr++;
            Gv_SetVarX(g_iTextureID, sector[vm.g_sp->sectnum].floorpicnum);
            continue;

        case CON_STARTTRACK:
        case CON_STARTTRACKVAR:
            insptr++;
            if (tw == CON_STARTTRACK) g_musicIndex=(ud.volume_number*MAXLEVELS)+(*(insptr++));
            else g_musicIndex=(ud.volume_number*MAXLEVELS)+(Gv_GetVarX(*(insptr++)));
            if (MapInfo[(uint8_t)g_musicIndex].musicfn == NULL)
            {
                OSD_Printf(CON_ERROR "null music for map %d\n",g_errorLineNum,keyw[g_tw],g_musicIndex);
                continue;
            }
            S_PlayMusic(&MapInfo[(uint8_t)g_musicIndex].musicfn[0],g_musicIndex);
            continue;

        case CON_ACTIVATECHEAT:
            insptr++;
            {
                int32_t j=Gv_GetVarX(*(insptr++));
                if (numplayers != 1 || !(g_player[myconnectindex].ps->gm & MODE_GAME))
                {
                    OSD_Printf(CON_ERROR "not in a single-player game.\n",g_errorLineNum,keyw[g_tw]);
                    continue;
                }
                osdcmd_cheatsinfo_stat.cheatnum = j;
            }
            continue;

        case CON_SETGAMEPALETTE:
            insptr++;
            switch (Gv_GetVarX(*(insptr++)))
            {
            default:
            case 0:P_SetGamePalette(g_player[vm.g_p].ps,palette  ,0); break;
            case 1:P_SetGamePalette(g_player[vm.g_p].ps,waterpal ,0); break;
            case 2:P_SetGamePalette(g_player[vm.g_p].ps,slimepal ,0); break;
            case 3:P_SetGamePalette(g_player[vm.g_p].ps,drealms  ,0); break;
            case 4:P_SetGamePalette(g_player[vm.g_p].ps,titlepal ,0); break;
            case 5:P_SetGamePalette(g_player[vm.g_p].ps,endingpal,0); break;
            case 6:P_SetGamePalette(g_player[vm.g_p].ps,animpal  ,0); break;
            }
            continue;

        case CON_GETTEXTURECEILING:
            insptr++;
            Gv_SetVarX(g_iTextureID, sector[vm.g_sp->sectnum].ceilingpicnum);
            continue;

        case CON_IFVARVARAND:
            insptr++;
            {
                int32_t j = Gv_GetVarX(*insptr++);
                j &= Gv_GetVarX(*insptr++);
                insptr--;
                VM_DoConditional(j);
            }
            continue;

        case CON_IFVARVAROR:
            insptr++;
            {
                int32_t j = Gv_GetVarX(*insptr++);
                j |= Gv_GetVarX(*insptr++);
                insptr--;
                VM_DoConditional(j);
            }
            continue;

        case CON_IFVARVARXOR:
            insptr++;
            {
                int32_t j = Gv_GetVarX(*insptr++);
                j ^= Gv_GetVarX(*insptr++);
                insptr--;
                VM_DoConditional(j);
            }
            continue;

        case CON_IFVARVAREITHER:
            insptr++;
            {
                int32_t j = Gv_GetVarX(*insptr++);
                int32_t l = Gv_GetVarX(*insptr++);
                insptr--;
                VM_DoConditional(j || l);
            }
            continue;

        case CON_IFVARVARN:
            insptr++;
            {
                int32_t j = Gv_GetVarX(*insptr++);
                j = (j != Gv_GetVarX(*insptr++));
                insptr--;
                VM_DoConditional(j);
            }
            continue;

        case CON_IFVARVARE:
            insptr++;
            {
                int32_t j = Gv_GetVarX(*insptr++);
                j = (j == Gv_GetVarX(*insptr++));
                insptr--;
                VM_DoConditional(j);
            }
            continue;

        case CON_IFVARVARG:
            insptr++;
            {
                int32_t j = Gv_GetVarX(*insptr++);
                j = (j > Gv_GetVarX(*insptr++));
                insptr--;
                VM_DoConditional(j);
            }
            continue;

        case CON_IFVARVARL:
            insptr++;
            {
                int32_t j = Gv_GetVarX(*insptr++);
                j = (j < Gv_GetVarX(*insptr++));
                insptr--;
                VM_DoConditional(j);
            }
            continue;

        case CON_IFVARE:
            insptr++;
            {
                int32_t j=Gv_GetVarX(*insptr++);
                VM_DoConditional(j == *insptr);
            }
            continue;

        case CON_IFVARN:
            insptr++;
            {
                int32_t j=Gv_GetVarX(*insptr++);
                VM_DoConditional(j != *insptr);
            }
            continue;

        case CON_WHILEVARN:
        {
            intptr_t *savedinsptr=insptr+2;
            int32_t j;
            do
            {
                insptr=savedinsptr;
                j = (Gv_GetVarX(*(insptr-1)) != *insptr);
                VM_DoConditional(j);
            }
            while (j);
            continue;
        }

        case CON_WHILEVARVARN:
        {
            int32_t j;
            intptr_t *savedinsptr=insptr+2;
            do
            {
                insptr=savedinsptr;
                j = Gv_GetVarX(*(insptr-1));
                j = (j != Gv_GetVarX(*insptr++));
                insptr--;
                VM_DoConditional(j);
            }
            while (j);
            continue;
        }

        case CON_IFVARAND:
            insptr++;
            {
                int32_t j=Gv_GetVarX(*insptr++);
                VM_DoConditional(j & *insptr);
            }
            continue;

        case CON_IFVAROR:
            insptr++;
            {
                int32_t j=Gv_GetVarX(*insptr++);
                VM_DoConditional(j | *insptr);
            }
            continue;

        case CON_IFVARXOR:
            insptr++;
            {
                int32_t j=Gv_GetVarX(*insptr++);
                VM_DoConditional(j ^ *insptr);
            }
            continue;

        case CON_IFVAREITHER:
            insptr++;
            {
                int32_t j=Gv_GetVarX(*insptr++);
                VM_DoConditional(j || *insptr);
            }
            continue;

        case CON_IFVARG:
            insptr++;
            {
                int32_t j=Gv_GetVarX(*insptr++);
                VM_DoConditional(j > *insptr);
            }
            continue;

        case CON_IFVARL:
            insptr++;
            {
                int32_t j=Gv_GetVarX(*insptr++);
                VM_DoConditional(j < *insptr);
            }
            continue;

        case CON_IFPHEALTHL:
            insptr++;
            VM_DoConditional(sprite[g_player[vm.g_p].ps->i].extra < *insptr);
            continue;

        case CON_IFPINVENTORY:
            insptr++;
            {
                int32_t j = 0;
                switch (*insptr++)
                {
                case GET_STEROIDS:
                    if (g_player[vm.g_p].ps->inv_amount[GET_STEROIDS] != *insptr)
                        j = 1;
                    break;
                case GET_SHIELD:
                    if (g_player[vm.g_p].ps->inv_amount[GET_SHIELD] != g_player[vm.g_p].ps->max_shield_amount)
                        j = 1;
                    break;
                case GET_SCUBA:
                    if (g_player[vm.g_p].ps->inv_amount[GET_SCUBA] != *insptr) j = 1;
                    break;
                case GET_HOLODUKE:
                    if (g_player[vm.g_p].ps->inv_amount[GET_HOLODUKE] != *insptr) j = 1;
                    break;
                case GET_JETPACK:
                    if (g_player[vm.g_p].ps->inv_amount[GET_JETPACK] != *insptr) j = 1;
                    break;
                case GET_ACCESS:
                    switch (vm.g_sp->pal)
                    {
                    case  0:
                        if (g_player[vm.g_p].ps->got_access&1) j = 1;
                        break;
                    case 21:
                        if (g_player[vm.g_p].ps->got_access&2) j = 1;
                        break;
                    case 23:
                        if (g_player[vm.g_p].ps->got_access&4) j = 1;
                        break;
                    }
                    break;
                case GET_HEATS:
                    if (g_player[vm.g_p].ps->inv_amount[GET_HEATS] != *insptr) j = 1;
                    break;
                case GET_FIRSTAID:
                    if (g_player[vm.g_p].ps->inv_amount[GET_FIRSTAID] != *insptr) j = 1;
                    break;
                case GET_BOOTS:
                    if (g_player[vm.g_p].ps->inv_amount[GET_BOOTS] != *insptr) j = 1;
                    break;
                default:
                    OSD_Printf(CON_ERROR "invalid inventory ID: %d\n",g_errorLineNum,keyw[g_tw],*(insptr-1));
                }

                VM_DoConditional(j);
                continue;
            }

        case CON_PSTOMP:
            insptr++;
            if (g_player[vm.g_p].ps->knee_incs == 0 && sprite[g_player[vm.g_p].ps->i].xrepeat >= 40)
                if (cansee(vm.g_sp->x,vm.g_sp->y,vm.g_sp->z-(4<<8),vm.g_sp->sectnum,g_player[vm.g_p].ps->pos.x,
                           g_player[vm.g_p].ps->pos.y,g_player[vm.g_p].ps->pos.z+(16<<8),sprite[g_player[vm.g_p].ps->i].sectnum))
                {
                    int32_t j = playerswhenstarted-1;
                    for (; j>=0; j--)
                    {
                        if (g_player[j].ps->actorsqu == vm.g_i)
                            break;
                    }
                    if (j == -1)
                    {
                        g_player[vm.g_p].ps->knee_incs = 1;
                        if (g_player[vm.g_p].ps->weapon_pos == 0)
                            g_player[vm.g_p].ps->weapon_pos = -1;
                        g_player[vm.g_p].ps->actorsqu = vm.g_i;
                    }
                }
            continue;

        case CON_IFAWAYFROMWALL:
        {
            int16_t s1=vm.g_sp->sectnum;
            int32_t j = 0;

            updatesector(vm.g_sp->x+108,vm.g_sp->y+108,&s1);
            if (s1 == vm.g_sp->sectnum)
            {
                updatesector(vm.g_sp->x-108,vm.g_sp->y-108,&s1);
                if (s1 == vm.g_sp->sectnum)
                {
                    updatesector(vm.g_sp->x+108,vm.g_sp->y-108,&s1);
                    if (s1 == vm.g_sp->sectnum)
                    {
                        updatesector(vm.g_sp->x-108,vm.g_sp->y+108,&s1);
                        if (s1 == vm.g_sp->sectnum)
                            j = 1;
                    }
                }
            }
            VM_DoConditional(j);
        }
        continue;

        case CON_QUOTE:
            insptr++;

            if ((ScriptQuotes[*insptr] == NULL))
            {
                OSD_Printf(CON_ERROR "null quote %d\n",g_errorLineNum,keyw[g_tw],*insptr);
                insptr++;
                continue;
            }

            if ((vm.g_p < 0 || vm.g_p >= MAXPLAYERS))
            {
                OSD_Printf(CON_ERROR "bad player for quote %d: (%d)\n",g_errorLineNum,keyw[g_tw],*insptr,vm.g_p);
                insptr++;
                continue;
            }

            P_DoQuote(*(insptr++)|MAXQUOTES,g_player[vm.g_p].ps);
            continue;

        case CON_USERQUOTE:
            insptr++;
            {
                int32_t i=Gv_GetVarX(*insptr++);

                if ((ScriptQuotes[i] == NULL))
                {
                    OSD_Printf(CON_ERROR "null quote %d\n",g_errorLineNum,keyw[g_tw],i);
                    continue;
                }
                G_AddUserQuote(ScriptQuotes[i]);
            }
            continue;

        case CON_IFINOUTERSPACE:
            VM_DoConditional(G_CheckForSpaceFloor(vm.g_sp->sectnum));
            continue;

        case CON_IFNOTMOVING:
            VM_DoConditional((actor[vm.g_i].movflag&49152) > 16384);
            continue;

        case CON_RESPAWNHITAG:
            insptr++;
            switch (DynamicTileMap[vm.g_sp->picnum])
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
                if (vm.g_sp->yvel) G_OperateRespawns(vm.g_sp->yvel);
                break;
            default:
                if (vm.g_sp->hitag >= 0) G_OperateRespawns(vm.g_sp->hitag);
                break;
            }
            continue;

        case CON_IFSPRITEPAL:
            insptr++;
            VM_DoConditional(vm.g_sp->pal == *insptr);
            continue;

        case CON_IFANGDIFFL:
            insptr++;
            {
                int32_t j = klabs(G_GetAngleDelta(g_player[vm.g_p].ps->ang,vm.g_sp->ang));
                VM_DoConditional(j <= *insptr);
            }
            continue;

        case CON_IFNOSOUNDS:
        {
            int32_t j = MAXSOUNDS-1;
            for (; j>=0; j--)
            {
                int32_t k = 0;

                for (; k<MAXSOUNDINSTANCES; k++)
                {
                    if (g_sounds[j].SoundOwner[k].i == vm.g_i)
                        break;
                }

                if (k != MAXSOUNDINSTANCES)
                    break;
            }

            VM_DoConditional(j < 0);
        }
        continue;

        case CON_SPRITEFLAGS:
            insptr++;
            actor[vm.g_i].flags = Gv_GetVarX(*insptr++);
            continue;

        case CON_GETTICKS:
            insptr++;
            {
                int32_t j=*insptr++;
                Gv_SetVarX(j, getticks());
            }
            continue;

        case CON_GETCURRADDRESS:
            insptr++;
            {
                int32_t j=*insptr++;
                Gv_SetVarX(j, (intptr_t)(insptr-script));
            }
            continue;

        case CON_JUMP:
            insptr++;
            {
                int32_t j = Gv_GetVarX(*insptr++);
                insptr = (intptr_t *)(j+script);
            }
            continue;

        default:
            VM_ScriptInfo();

            G_GameExit("An error has occurred in the EDuke32 virtual machine.\n\n"
                       "If you are an end user, please e-mail the file eduke32.log\n"
                       "along with links to any mods you're using to terminx@gmail.com.\n\n"
                       "If you are a mod developer, please attach all of your CON files\n"
                       "along with instructions on how to reproduce this error.\n\n"
                       "Thank you!");
            break;
        }
    }

    return 0;
}

void A_LoadActor(int32_t iActor)
{
    vm.g_i = iActor;    // Sprite ID
    vm.g_p = -1; // iPlayer;    // Player ID
    vm.g_x = -1; // lDist;    // ?
    vm.g_sp = &sprite[vm.g_i];    // Pointer to sprite structure
    vm.g_t = &actor[vm.g_i].t_data[0];   // Sprite's 'extra' data

    if (actorLoadEventScrptr[vm.g_sp->picnum] == 0) return;

    insptr = actorLoadEventScrptr[vm.g_sp->picnum];

    vm.g_flags &= ~(VM_RETURN|VM_KILL|VM_NOEXECUTE);

    if (vm.g_sp->sectnum < 0 || vm.g_sp->sectnum >= MAXSECTORS)
    {
        //      if(A_CheckEnemySprite(vm.g_sp))
        //          g_player[vm.g_p].ps->actors_killed++;
        deletesprite(vm.g_i);
        return;
    }

    VM_Execute(0);

    if (vm.g_flags & VM_KILL)
        deletesprite(vm.g_i);
}

void A_Execute(int32_t iActor,int32_t iPlayer,int32_t lDist)
{
    vmstate_t tempvm = { iActor, iPlayer, lDist, &actor[iActor].t_data[0],
                         &sprite[iActor], 0
                       };

    if (g_netClient && A_CheckSpriteFlags(iActor, SPRITE_NULL))
    {
        deletesprite(iActor);
        return;
    }

    if (g_netServer || g_netClient)
        randomseed = ticrandomseed;

    Bmemcpy(&vm, &tempvm, sizeof(vmstate_t));

    insptr = 4 + (actorscrptr[vm.g_sp->picnum]);

    if (vm.g_sp->sectnum < 0 || vm.g_sp->sectnum >= MAXSECTORS)
    {
        if (A_CheckEnemySprite(vm.g_sp))
            g_player[vm.g_p].ps->actors_killed++;
        deletesprite(vm.g_i);
        return;
    }

    /* Qbix: Changed variables to be aware of the sizeof *insptr
     * (whether it is int32_t vs intptr_t), Although it is specifically cast to intptr_t*
     * which might be corrected if the code is converted to use offsets */

    if (vm.g_t[4] > (intptr_t)&script[0] && vm.g_t[4] < (intptr_t)&script[g_scriptSize])
    {
        vm.g_sp->lotag += TICSPERFRAME;

        if (vm.g_sp->lotag > *(intptr_t *)(vm.g_t[4]+4*sizeof(*insptr)))
        {
            vm.g_t[2]++;
            vm.g_sp->lotag = 0;
            vm.g_t[3] +=  *(intptr_t *)(vm.g_t[4]+3*sizeof(*insptr));
        }

        if (klabs(vm.g_t[3]) >= klabs(*(intptr_t *)(vm.g_t[4]+sizeof(*insptr)) * *(intptr_t *)(vm.g_t[4]+3*sizeof(*insptr))))
            vm.g_t[3] = 0;
    }

    VM_Execute(0);

    if (vm.g_flags & VM_KILL)
    {
        // if player was set to squish, first stop that...
        if (g_player[vm.g_p].ps->actorsqu == vm.g_i)
            g_player[vm.g_p].ps->actorsqu = -1;
        deletesprite(vm.g_i);
        return;
    }

    VM_Move();

    /*        if (ud.angleinterpolation)
            {
                temp = (vm.g_sp->ang & 2047) - sprpos[vm.g_i].ang;
                sprpos[vm.g_i].oldang = sprpos[vm.g_i].ang;
                if (temp)
                {
                    temp2 = temp/klabs(temp);
                    if (klabs(temp) > 1024) temp2 = -(temp2);
                    sprpos[vm.g_i].angdir = temp2;
                    sprpos[vm.g_i].angdif = min(ud.angleinterpolation,klabs(temp));
                    sprpos[vm.g_i].ang += sprpos[vm.g_i].angdif * sprpos[vm.g_i].angdir;
                    sprpos[vm.g_i].ang &= 2047;
                }
            }
      */
    if (vm.g_sp->statnum == STAT_STANDABLE)
        switch (DynamicTileMap[vm.g_sp->picnum])
        {
        case RUBBERCAN__STATIC:
        case EXPLODINGBARREL__STATIC:
        case WOODENHORSE__STATIC:
        case HORSEONSIDE__STATIC:
        case CANWITHSOMETHING__STATIC:
        case FIREBARREL__STATIC:
        case NUKEBARREL__STATIC:
        case NUKEBARRELDENTED__STATIC:
        case NUKEBARRELLEAKED__STATIC:
        case TRIPBOMB__STATIC:
        case EGG__STATIC:
            if (actor[vm.g_i].timetosleep > 1)
                actor[vm.g_i].timetosleep--;
            else if (actor[vm.g_i].timetosleep == 1)
                changespritestat(vm.g_i,STAT_ZOMBIEACTOR);
        default:
            return;
        }

    if (vm.g_sp->statnum != 1)
        return;

    if (A_CheckEnemySprite(vm.g_sp))
    {
        if (vm.g_sp->xrepeat > 60) return;
        if (ud.respawn_monsters == 1 && vm.g_sp->extra <= 0) return;
    }
    else if (ud.respawn_items == 1 && (vm.g_sp->cstat&32768)) return;

    if (A_CheckSpriteFlags(vm.g_i, SPRITE_USEACTIVATOR) && sector[vm.g_sp->sectnum].lotag & 16384)
        changespritestat(vm.g_i,STAT_ZOMBIEACTOR);
    else if (actor[vm.g_i].timetosleep > 1)
        actor[vm.g_i].timetosleep--;
    else if (actor[vm.g_i].timetosleep == 1)
        changespritestat(vm.g_i,STAT_ZOMBIEACTOR);
}

void G_SaveMapState(mapstate_t *save)
{
    if (save != NULL)
    {
        int32_t i;
        intptr_t j;

        Bmemcpy(&save->numwalls,&numwalls,sizeof(numwalls));
        Bmemcpy(&save->wall[0],&wall[0],sizeof(walltype)*MAXWALLS);
        Bmemcpy(&save->numsectors,&numsectors,sizeof(numsectors));
        Bmemcpy(&save->sector[0],&sector[0],sizeof(sectortype)*MAXSECTORS);
        Bmemcpy(&save->sprite[0],&sprite[0],sizeof(spritetype)*MAXSPRITES);
        Bmemcpy(&save->spriteext[0],&spriteext[0],sizeof(spriteext_t)*MAXSPRITES);
        Bmemcpy(&save->headspritesect[0],&headspritesect[0],sizeof(headspritesect));
        Bmemcpy(&save->prevspritesect[0],&prevspritesect[0],sizeof(prevspritesect));
        Bmemcpy(&save->nextspritesect[0],&nextspritesect[0],sizeof(nextspritesect));
        Bmemcpy(&save->headspritestat[0],&headspritestat[0],sizeof(headspritestat));
        Bmemcpy(&save->prevspritestat[0],&prevspritestat[0],sizeof(prevspritestat));
        Bmemcpy(&save->nextspritestat[0],&nextspritestat[0],sizeof(nextspritestat));

        for (i=MAXSPRITES-1; i>=0; i--)
        {
            save->scriptptrs[i] = 0;

            if (actorscrptr[PN] == 0) continue;

            j = (intptr_t)&script[0];

            if (T2 >= j && T2 < (intptr_t)(&script[g_scriptSize]))
            {
                save->scriptptrs[i] |= 1;
                T2 -= j;
            }
            if (T5 >= j && T5 < (intptr_t)(&script[g_scriptSize]))
            {
                save->scriptptrs[i] |= 2;
                T5 -= j;
            }
            if (T6 >= j && T6 < (intptr_t)(&script[g_scriptSize]))
            {
                save->scriptptrs[i] |= 4;
                T6 -= j;
            }
        }

        Bmemcpy(&save->actor[0],&actor[0],sizeof(actor_t)*MAXSPRITES);

        for (i=MAXSPRITES-1; i>=0; i--)
        {
            if (actorscrptr[PN] == 0) continue;
            j = (intptr_t)&script[0];

            if (save->scriptptrs[i]&1)
                T2 += j;
            if (save->scriptptrs[i]&2)
                T5 += j;
            if (save->scriptptrs[i]&4)
                T6 += j;
        }

        Bmemcpy(&save->g_numCyclers,&g_numCyclers,sizeof(g_numCyclers));
        Bmemcpy(&save->cyclers[0][0],&cyclers[0][0],sizeof(cyclers));
        Bmemcpy(&save->g_playerSpawnPoints[0],&g_playerSpawnPoints[0],sizeof(g_playerSpawnPoints));
        Bmemcpy(&save->g_numAnimWalls,&g_numAnimWalls,sizeof(g_numAnimWalls));
        Bmemcpy(&save->SpriteDeletionQueue[0],&SpriteDeletionQueue[0],sizeof(SpriteDeletionQueue));
        Bmemcpy(&save->g_spriteDeleteQueuePos,&g_spriteDeleteQueuePos,sizeof(g_spriteDeleteQueuePos));
        Bmemcpy(&save->animwall[0],&animwall[0],sizeof(animwall));
        Bmemcpy(&save->msx[0],&msx[0],sizeof(msx));
        Bmemcpy(&save->msy[0],&msy[0],sizeof(msy));
        Bmemcpy(&save->g_mirrorWall[0],&g_mirrorWall[0],sizeof(g_mirrorWall));
        Bmemcpy(&save->g_mirrorSector[0],&g_mirrorSector[0],sizeof(g_mirrorSector));
        Bmemcpy(&save->g_mirrorCount,&g_mirrorCount,sizeof(g_mirrorCount));
        Bmemcpy(&save->show2dsector[0],&show2dsector[0],sizeof(show2dsector));
        Bmemcpy(&save->g_numClouds,&g_numClouds,sizeof(g_numClouds));
        Bmemcpy(&save->clouds[0],&clouds[0],sizeof(clouds));
        Bmemcpy(&save->cloudx[0],&cloudx[0],sizeof(cloudx));
        Bmemcpy(&save->cloudy[0],&cloudy[0],sizeof(cloudy));
        Bmemcpy(&save->pskyoff[0],&pskyoff[0],sizeof(pskyoff));
        Bmemcpy(&save->pskybits,&pskybits,sizeof(pskybits));
        Bmemcpy(&save->animategoal[0],&animategoal[0],sizeof(animategoal));
        Bmemcpy(&save->animatevel[0],&animatevel[0],sizeof(animatevel));
        Bmemcpy(&save->g_animateCount,&g_animateCount,sizeof(g_animateCount));
        Bmemcpy(&save->animatesect[0],&animatesect[0],sizeof(animatesect));
        for (i = g_animateCount-1; i>=0; i--) animateptr[i] = (int32_t *)((intptr_t)animateptr[i]-(intptr_t)(&sector[0]));
        Bmemcpy(&save->animateptr[0],&animateptr[0],sizeof(animateptr));
        for (i = g_animateCount-1; i>=0; i--) animateptr[i] = (int32_t *)((intptr_t)animateptr[i]+(intptr_t)(&sector[0]));
        Bmemcpy(&save->g_numPlayerSprites,&g_numPlayerSprites,sizeof(g_numPlayerSprites));
        Bmemcpy(&save->g_earthquakeTime,&g_earthquakeTime,sizeof(g_earthquakeTime));
        Bmemcpy(&save->lockclock,&lockclock,sizeof(lockclock));
        Bmemcpy(&save->randomseed,&randomseed,sizeof(randomseed));
        Bmemcpy(&save->g_globalRandom,&g_globalRandom,sizeof(g_globalRandom));

        for (i=g_gameVarCount-1; i>=0; i--)
        {
            if (aGameVars[i].dwFlags & GAMEVAR_NORESET) continue;
            if (aGameVars[i].dwFlags & GAMEVAR_PERPLAYER)
            {
                if (!save->vars[i])
                    save->vars[i] = Bcalloc(MAXPLAYERS,sizeof(intptr_t));
                Bmemcpy(&save->vars[i][0],&aGameVars[i].val.plValues[0],sizeof(intptr_t) * MAXPLAYERS);
            }
            else if (aGameVars[i].dwFlags & GAMEVAR_PERACTOR)
            {
                if (!save->vars[i])
                    save->vars[i] = Bcalloc(MAXSPRITES,sizeof(intptr_t));
                Bmemcpy(&save->vars[i][0],&aGameVars[i].val.plValues[0],sizeof(intptr_t) * MAXSPRITES);
            }
            else save->vars[i] = (intptr_t *)aGameVars[i].val.lValue;
        }

        ototalclock = totalclock;
    }
}

extern void Gv_RefreshPointers(void);

void G_RestoreMapState(mapstate_t *save)
{
    if (save != NULL)
    {
        int32_t i, k, x;
        intptr_t j;
        char phealth[MAXPLAYERS];

        for (i=0; i<playerswhenstarted; i++)
            phealth[i] = sprite[g_player[i].ps->i].extra;

        pub = NUMPAGES;
        pus = NUMPAGES;
        G_UpdateScreenArea();

        Bmemcpy(&numwalls,&save->numwalls,sizeof(numwalls));
        Bmemcpy(&wall[0],&save->wall[0],sizeof(walltype)*MAXWALLS);
        Bmemcpy(&numsectors,&save->numsectors,sizeof(numsectors));
        Bmemcpy(&sector[0],&save->sector[0],sizeof(sectortype)*MAXSECTORS);
        Bmemcpy(&sprite[0],&save->sprite[0],sizeof(spritetype)*MAXSPRITES);
        Bmemcpy(&spriteext[0],&save->spriteext[0],sizeof(spriteext_t)*MAXSPRITES);
        Bmemcpy(&headspritesect[0],&save->headspritesect[0],sizeof(headspritesect));
        Bmemcpy(&prevspritesect[0],&save->prevspritesect[0],sizeof(prevspritesect));
        Bmemcpy(&nextspritesect[0],&save->nextspritesect[0],sizeof(nextspritesect));
        Bmemcpy(&headspritestat[0],&save->headspritestat[0],sizeof(headspritestat));
        Bmemcpy(&prevspritestat[0],&save->prevspritestat[0],sizeof(prevspritestat));
        Bmemcpy(&nextspritestat[0],&save->nextspritestat[0],sizeof(nextspritestat));
        Bmemcpy(&actor[0],&save->actor[0],sizeof(actor_t)*MAXSPRITES);

        for (i=MAXSPRITES-1; i>=0; i--)
        {
            j = (intptr_t)(&script[0]);
            if (save->scriptptrs[i]&1) T2 += j;
            if (save->scriptptrs[i]&2) T5 += j;
            if (save->scriptptrs[i]&4) T6 += j;
        }

        Bmemcpy(&g_numCyclers,&save->g_numCyclers,sizeof(g_numCyclers));
        Bmemcpy(&cyclers[0][0],&save->cyclers[0][0],sizeof(cyclers));
        Bmemcpy(&g_playerSpawnPoints[0],&save->g_playerSpawnPoints[0],sizeof(g_playerSpawnPoints));
        Bmemcpy(&g_numAnimWalls,&save->g_numAnimWalls,sizeof(g_numAnimWalls));
        Bmemcpy(&SpriteDeletionQueue[0],&save->SpriteDeletionQueue[0],sizeof(SpriteDeletionQueue));
        Bmemcpy(&g_spriteDeleteQueuePos,&save->g_spriteDeleteQueuePos,sizeof(g_spriteDeleteQueuePos));
        Bmemcpy(&animwall[0],&save->animwall[0],sizeof(animwall));
        Bmemcpy(&msx[0],&save->msx[0],sizeof(msx));
        Bmemcpy(&msy[0],&save->msy[0],sizeof(msy));
        Bmemcpy(&g_mirrorWall[0],&save->g_mirrorWall[0],sizeof(g_mirrorWall));
        Bmemcpy(&g_mirrorSector[0],&save->g_mirrorSector[0],sizeof(g_mirrorSector));
        Bmemcpy(&g_mirrorCount,&save->g_mirrorCount,sizeof(g_mirrorCount));
        Bmemcpy(&show2dsector[0],&save->show2dsector[0],sizeof(show2dsector));
        Bmemcpy(&g_numClouds,&save->g_numClouds,sizeof(g_numClouds));
        Bmemcpy(&clouds[0],&save->clouds[0],sizeof(clouds));
        Bmemcpy(&cloudx[0],&save->cloudx[0],sizeof(cloudx));
        Bmemcpy(&cloudy[0],&save->cloudy[0],sizeof(cloudy));
        Bmemcpy(&pskyoff[0],&save->pskyoff[0],sizeof(pskyoff));
        Bmemcpy(&pskybits,&save->pskybits,sizeof(pskybits));
        Bmemcpy(&animategoal[0],&save->animategoal[0],sizeof(animategoal));
        Bmemcpy(&animatevel[0],&save->animatevel[0],sizeof(animatevel));
        Bmemcpy(&g_animateCount,&save->g_animateCount,sizeof(g_animateCount));
        Bmemcpy(&animatesect[0],&save->animatesect[0],sizeof(animatesect));
        Bmemcpy(&animateptr[0],&save->animateptr[0],sizeof(animateptr));
        for (i = g_animateCount-1; i>=0; i--) animateptr[i] = (int32_t *)((intptr_t)animateptr[i]+(intptr_t)(&sector[0]));
        Bmemcpy(&g_numPlayerSprites,&save->g_numPlayerSprites,sizeof(g_numPlayerSprites));
        Bmemcpy(&g_earthquakeTime,&save->g_earthquakeTime,sizeof(g_earthquakeTime));
        Bmemcpy(&lockclock,&save->lockclock,sizeof(lockclock));
        Bmemcpy(&randomseed,&save->randomseed,sizeof(randomseed));
        Bmemcpy(&g_globalRandom,&save->g_globalRandom,sizeof(g_globalRandom));

        for (i=g_gameVarCount-1; i>=0; i--)
        {
            if (aGameVars[i].dwFlags & GAMEVAR_NORESET) continue;
            if (aGameVars[i].dwFlags & GAMEVAR_PERPLAYER)
            {
                if (!save->vars[i]) continue;
                Bmemcpy(&aGameVars[i].val.plValues[0],&save->vars[i][0],sizeof(intptr_t) * MAXPLAYERS);
            }
            else if (aGameVars[i].dwFlags & GAMEVAR_PERACTOR)
            {
                if (!save->vars[i]) continue;
                Bmemcpy(&aGameVars[i].val.plValues[0],&save->vars[i][0],sizeof(intptr_t) * MAXSPRITES);
            }
            else aGameVars[i].val.lValue = (intptr_t)save->vars[i];
        }

        Gv_RefreshPointers();

        for (i=0; i<playerswhenstarted; i++)
            sprite[g_player[i].ps->i].extra = phealth[i];

        if (g_player[myconnectindex].ps->over_shoulder_on != 0)
        {
            g_cameraDistance = 0;
            g_cameraClock = 0;
            g_player[myconnectindex].ps->over_shoulder_on = 1;
        }

        screenpeek = myconnectindex;

        if (ud.lockout == 0)
        {
            for (x=g_numAnimWalls-1; x>=0; x--)
                if (wall[animwall[x].wallnum].extra >= 0)
                    wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
        }
        else
        {
            for (x=g_numAnimWalls-1; x>=0; x--)
                switch (DynamicTileMap[wall[animwall[x].wallnum].picnum])
                {
                case FEMPIC1__STATIC:
                    wall[animwall[x].wallnum].picnum = BLANKSCREEN;
                    break;
                case FEMPIC2__STATIC:
                case FEMPIC3__STATIC:
                    wall[animwall[x].wallnum].picnum = SCREENBREAK6;
                    break;
                }
        }

        g_numInterpolations = 0;
        startofdynamicinterpolations = 0;

        k = headspritestat[STAT_EFFECTOR];
        while (k >= 0)
        {
            switch (sprite[k].lotag)
            {
            case 31:
                G_SetInterpolation(&sector[sprite[k].sectnum].floorz);
                break;
            case 32:
                G_SetInterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 25:
                G_SetInterpolation(&sector[sprite[k].sectnum].floorz);
                G_SetInterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 17:
                G_SetInterpolation(&sector[sprite[k].sectnum].floorz);
                G_SetInterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 0:
            case 5:
            case 6:
            case 11:
            case 14:
            case 15:
            case 16:
            case 26:
            case 30:
                Sect_SetInterpolation(k);
                break;
            }

            k = nextspritestat[k];
        }

        for (i=g_numInterpolations-1; i>=0; i--) bakipos[i] = *curipos[i];
        for (i = g_animateCount-1; i>=0; i--)
            G_SetInterpolation(animateptr[i]);

        Net_ResetPrediction();

        clearfifo();
        G_ResetTimers();
    }
}
