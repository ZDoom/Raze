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
#include "sector.h"
#include "gamedef.h"
#include "gameexec.h"
#include "premap.h"
#include "osd.h"
#include "common_game.h"
#include "input.h"

// PRIMITIVE

static int32_t g_haltSoundHack = 0;

int32_t S_FindMusicSFX(int32_t sn, int32_t *sndptr)
{
    int32_t i;

    for (SPRITES_OF_SECT(sn, i))
    {
        const int32_t snd = sprite[i].lotag;
        EDUKE32_STATIC_ASSERT(MAXSOUNDS >= 1000);

        if (PN == MUSICANDSFX && (unsigned)snd < 1000)  // XXX: in other places, 999
        {
            *sndptr = snd;
            return i;
        }
    }

    *sndptr = -1;
    return -1;
}

// this function activates a sector's MUSICANDSFX sprite
int32_t A_CallSound(int32_t sn, int32_t whatsprite)
{
    int32_t i, snd;

    if (g_haltSoundHack)
    {
        g_haltSoundHack = 0;
        return -1;
    }

    i = S_FindMusicSFX(sn, &snd);

    if (i >= 0)
    {
        if (whatsprite == -1)
            whatsprite = i;

        if (T1 == 0)
        {
            if ((g_sounds[snd].m & SF_GLOBAL) == 0)
            {
                if (snd)
                {
                    A_PlaySound(snd, whatsprite);
                    if (SHT && snd != SHT && SHT < MAXSOUNDS)
                        S_StopEnvSound(SHT,T6);
                    T6 = whatsprite;
                }

                if ((sector[SECT].lotag&0xff) != ST_22_SPLITTING_DOOR)
                    T1 = 1;
            }
        }
        else if (SHT < MAXSOUNDS)
        {
            if (SHT)
                A_PlaySound(SHT, whatsprite);
            if ((g_sounds[snd].m & SF_LOOP) || (SHT && SHT != snd))
                S_StopEnvSound(snd, T6);
            T6 = whatsprite;
            T1 = 0;
        }

        return snd;
    }

    return -1;
}

int32_t G_CheckActivatorMotion(int32_t lotag)
{
    int32_t i = headspritestat[STAT_ACTIVATOR], j;
    spritetype *s;

    while (i >= 0)
    {
        if (sprite[i].lotag == lotag)
        {
            s = &sprite[i];

            for (j = g_animateCount-1; j >= 0; j--)
                if (s->sectnum == animatesect[j])
                    return(1);

            j = headspritestat[STAT_EFFECTOR];
            while (j >= 0)
            {
                if (s->sectnum == sprite[j].sectnum)
                    switch (sprite[j].lotag)
                    {
                    case SE_11_SWINGING_DOOR:
                    case SE_30_TWO_WAY_TRAIN:
                        if (actor[j].t_data[4])
                            return(1);
                        break;
                    case SE_20_STRETCH_BRIDGE:
                    case SE_31_FLOOR_RISE_FALL:
                    case SE_32_CEILING_RISE_FALL:
                    case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
                        if (actor[j].t_data[0])
                            return(1);
                        break;
                    }

                j = nextspritestat[j];
            }
        }
        i = nextspritestat[i];
    }
    return(0);
}

int32_t CheckDoorTile(int32_t dapic)
{
    switch (DYNAMICTILEMAP(dapic))
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
    return 0;
}

int32_t isanunderoperator(int32_t lotag)
{
    switch (lotag&0xff)
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

int32_t isanearoperator(int32_t lotag)
{
    switch (lotag&0xff)
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
    case ST_29_TEETH_DOOR://Toothed door
        return 1;
    }
    return 0;
}

static inline int32_t A_FP_ManhattanDist(const DukePlayer_t *ps, const spritetype *s)
{
    return klabs(ps->opos.x-s->x)
        + klabs(ps->opos.y-s->y)
        + ((klabs(ps->opos.z-s->z+(28<<8)))>>4);
}

int32_t __fastcall A_FindPlayer(const spritetype *s, int32_t *d)
{
    if (!g_netServer && ud.multimode < 2)
    {
        DukePlayer_t *const myps = g_player[myconnectindex].ps;

        if (d)
            *d = A_FP_ManhattanDist(myps, s);
        return myconnectindex;
    }

    {
        int32_t j;
        int32_t closest_player=0, closest=INT32_MAX;

        for (TRAVERSE_CONNECT(j))
        {
            DukePlayer_t *const ps = g_player[j].ps;
            int32_t x = A_FP_ManhattanDist(ps, s);

            if (x < closest && sprite[ps->i].extra > 0)
            {
                closest_player = j;
                closest = x;
            }
        }

        if (d)
            *d = closest;
        return closest_player;
    }
}

void G_DoSectorAnimations(void)
{
    int32_t i, j, a, p, v, dasect;

    for (i=g_animateCount-1; i>=0; i--)
    {
        a = *animateptr[i];
        v = animatevel[i]*TICSPERFRAME;
        dasect = animatesect[i];

        if (a == animategoal[i])
        {
            G_StopInterpolation(animateptr[i]);

            // This fixes a bug where wall or floor sprites contained in
            // elevator sectors (ST 16-19) would jitter vertically after the
            // elevator had stopped.
            if (animateptr[i] == &sector[animatesect[i]].floorz)
                for (j=headspritesect[dasect]; j>=0; j=nextspritesect[j])
                    if (sprite[j].statnum != STAT_EFFECTOR)
                        actor[j].bpos.z = sprite[j].z;

            g_animateCount--;
            animateptr[i] = animateptr[g_animateCount];
            animategoal[i] = animategoal[g_animateCount];
            animatevel[i] = animatevel[g_animateCount];
            animatesect[i] = animatesect[g_animateCount];
            if (sector[animatesect[i]].lotag == ST_18_ELEVATOR_DOWN || sector[animatesect[i]].lotag == ST_19_ELEVATOR_UP)
                if (animateptr[i] == &sector[animatesect[i]].ceilingz)
                    continue;

            if ((sector[dasect].lotag&0xff) != ST_22_SPLITTING_DOOR)
                A_CallSound(dasect,-1);

            continue;
        }

        if (v > 0)
            a = min(a+v, animategoal[i]);
        else
            a = max(a+v, animategoal[i]);

        if (animateptr[i] == &sector[animatesect[i]].floorz)
        {
            for (TRAVERSE_CONNECT(p))
                if (g_player[p].ps->cursectnum == dasect)
                    if ((sector[dasect].floorz-g_player[p].ps->pos.z) < (64<<8))
                        if (sprite[g_player[p].ps->i].owner >= 0)
                        {
                            g_player[p].ps->pos.z += v;
                            g_player[p].ps->vel.z = 0;
                            if (p == myconnectindex)
                            {
                                my.z += v;
                                myvel.z = 0;
                            }
                        }

            for (j=headspritesect[dasect]; j>=0; j=nextspritesect[j])
                if (sprite[j].statnum != STAT_EFFECTOR)
                {
                    actor[j].bpos.z = sprite[j].z;
                    sprite[j].z += v;
                    actor[j].floorz = sector[dasect].floorz+v;
                }
        }

        *animateptr[i] = a;
    }
}

int32_t GetAnimationGoal(const int32_t *animptr)
{
    int32_t i=0;

    for (; i<g_animateCount; i++)
        if (animptr == animateptr[i])
            return i;
    return -1;
}

int32_t SetAnimation(int32_t animsect,int32_t *animptr, int32_t thegoal, int32_t thevel)
{
    int32_t i = 0, j = g_animateCount;

    if (g_animateCount >= MAXANIMATES)
        return -1;

    for (; i<g_animateCount; i++)
        if (animptr == animateptr[i])
        {
            j = i;
            break;
        }

    animatesect[j] = animsect;
    animateptr[j] = animptr;
    animategoal[j] = thegoal;
    animatevel[j] = (thegoal >= *animptr) ? thevel : -thevel;

    if (j == g_animateCount)
        g_animateCount++;

    G_SetInterpolation(animptr);

    return j;
}

static void G_SetupCamTile(int32_t i, int32_t wn, int32_t smoothratio)
{
    vec3_t cam = G_GetCameraPosition(i, smoothratio);

    const int32_t mir = display_mirror;
    //if (waloff[wn] == 0) loadtile(wn);
    setviewtotile(wn, tilesiz[wn].y, tilesiz[wn].x);

    yax_preparedrawrooms();
    drawrooms(cam.x, cam.y, cam.z, SA, 100+sprite[i].shade, SECT);
    yax_drawrooms(G_DoSpriteAnimations, SECT, 0, smoothratio);

    display_mirror = 3;
    G_DoSpriteAnimations(cam.x, cam.y, SA, smoothratio);
    display_mirror = mir;
    drawmasks();

    setviewback();
    squarerotatetile(wn);
    invalidatetile(wn, -1, 255);
}

void G_AnimateCamSprite(int32_t smoothratio)
{
    const int32_t i = g_curViewscreen;

#ifdef DEBUG_VALGRIND_NO_SMC
    return;
#endif
    if (g_curViewscreen < 0)
        return;

    if (totalclock >= T1 + ud.camera_time)
    {
        const DukePlayer_t *const ps = g_player[screenpeek].ps;

        if (ps->newowner >= 0)
            OW = ps->newowner;

        if (OW >= 0 && dist(&sprite[ps->i], &sprite[i]) < VIEWSCREEN_ACTIVE_DISTANCE)
        {
            const int viewscrShift = G_GetViewscreenSizeShift((const tspritetype *)&sprite[i]);
            const int viewscrTile = TILE_VIEWSCR-viewscrShift;

            if (waloff[viewscrTile] == 0)
                allocatepermanenttile(viewscrTile, tilesiz[PN].x<<viewscrShift, tilesiz[PN].y<<viewscrShift);
            else
                walock[viewscrTile] = 255;

            G_SetupCamTile(OW, viewscrTile, smoothratio);
#ifdef POLYMER
            // Force texture update on viewscreen sprite in Polymer!
            if (getrendermode() == REND_POLYMER)
                polymer_invalidatesprite(i);
#endif
        }

        T1 = totalclock;
    }
}

void G_AnimateWalls(void)
{
    int32_t p = g_numAnimWalls-1;

    for (; p>=0; p--)
    {
        const int32_t i = animwall[p].wallnum;

        switch (DYNAMICTILEMAP(wall[i].picnum))
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
                animwall[p].tag = wall[i].picnum;
                wall[i].picnum = SCREENBREAK6;
            }

            continue;

        case SCREENBREAK6__STATIC:
        case SCREENBREAK7__STATIC:
        case SCREENBREAK8__STATIC:

            if (animwall[p].tag >= 0 && wall[i].extra != FEMPIC2 && wall[i].extra != FEMPIC3)
                wall[i].picnum = animwall[p].tag;
            else
            {
                wall[i].picnum++;
                if (wall[i].picnum == (SCREENBREAK6+3))
                    wall[i].picnum = SCREENBREAK6;
            }
            continue;
        }

        if ((wall[i].cstat&16) && G_GetForcefieldPicnum(i)==W_FORCEFIELD)
        {
            const int32_t t = animwall[p].tag;

            if (wall[i].cstat&254)
            {
                wall[i].xpanning -= t>>10; // sintable[(t+512)&2047]>>12;
                wall[i].ypanning -= t>>10; // sintable[t&2047]>>12;

                if (wall[i].extra == 1)
                {
                    wall[i].extra = 0;
                    animwall[p].tag = 0;
                }
                else
                    animwall[p].tag+=128;

                if (animwall[p].tag < (128<<4))
                {
                    if (animwall[p].tag&128)
                        wall[i].overpicnum = W_FORCEFIELD;
                    else wall[i].overpicnum = W_FORCEFIELD+1;
                }
                else
                {
                    if ((krand()&255) < 32)
                        animwall[p].tag = 128<<(krand()&3);
                    else wall[i].overpicnum = W_FORCEFIELD+1;
                }
            }
        }
    }
}

int32_t G_ActivateWarpElevators(int32_t s, int32_t d) //Parm = sectoreffectornum
{
    int32_t i;
    const int32_t sn = sprite[s].sectnum;

    for (SPRITES_OF(STAT_EFFECTOR, i))
        if (SLT == SE_17_WARP_ELEVATOR && SHT == sprite[s].hitag)
        {
            if (klabs(sector[sn].floorz - actor[s].t_data[2]) > SP ||
                    sector[SECT].hitag == sector[sn].hitag - d)
                break;
        }

    if (i == -1)
        return 1; // No find

    A_PlaySound(d ? ELEVATOR_ON : ELEVATOR_OFF, s);

    for (SPRITES_OF(STAT_EFFECTOR, i))
        if (SLT == SE_17_WARP_ELEVATOR && SHT == sprite[s].hitag)
            T1 = T2 = d; //Make all check warp

    return 0;
}

void G_OperateSectors(int32_t sn, int32_t ii)
{
    int32_t j=0, l;
    int32_t i;
    sectortype *const sptr = &sector[sn];

    switch (sptr->lotag&(0xffff-49152))
    {
    case ST_30_ROTATE_RISE_BRIDGE:
        j = sector[sn].hitag;

        if (E_SpriteIsValid(j))
        {
            if (actor[j].tempang == 0 || actor[j].tempang == 256)
                A_CallSound(sn,ii);
            sprite[j].extra = (sprite[j].extra == 1) ? 3 : 1;
        }
        break;

    case ST_31_TWO_WAY_TRAIN:
        j = sector[sn].hitag;

        if (E_SpriteIsValid(j))
        {
            if (actor[j].t_data[4] == 0)
                actor[j].t_data[4] = 1;

            A_CallSound(sn,ii);
        }
        break;

    case ST_26_SPLITTING_ST_DOOR: //The split doors
        if (GetAnimationGoal(&sptr->ceilingz) == -1) //if the door has stopped
        {
            g_haltSoundHack = 1;
            sptr->lotag &= 0xff00;
            sptr->lotag |= ST_22_SPLITTING_DOOR;
            G_OperateSectors(sn,ii);
            sptr->lotag &= 0xff00;
            sptr->lotag |= ST_9_SLIDING_ST_DOOR;
            G_OperateSectors(sn,ii);
            sptr->lotag &= 0xff00;
            sptr->lotag |= ST_26_SPLITTING_ST_DOOR;
        }
        return;

    case ST_9_SLIDING_ST_DOOR:
    {
        int32_t dax, day;
        int32_t wallfind[2];

        const int32_t startwall = sptr->wallptr;
        const int32_t endwall = startwall+sptr->wallnum-1;

        const int32_t sp = sptr->extra>>4;

        //first find center point by averaging all points
        dax = 0, day = 0;
        for (i=startwall; i<=endwall; i++)
        {
            dax += wall[i].x;
            day += wall[i].y;
        }

        dax = tabledivide32_noinline(dax, (endwall-startwall+1));
        day = tabledivide32_noinline(day, (endwall-startwall+1));

        //find any points with either same x or same y coordinate
        //  as center (dax, day) - should be 2 points found.
        wallfind[0] = -1;
        wallfind[1] = -1;
        for (i=startwall; i<=endwall; i++)
            if (wall[i].x == dax || wall[i].y == day)
            {
                if (wallfind[0] == -1)
                    wallfind[0] = i;
                else wallfind[1] = i;
            }

        if (wallfind[1] == -1)
            return;

        for (j=0; j<2; j++)
        {
            const int32_t wf = wallfind[j];
            int32_t dax2, day2;

            i = wf-1;
            if (i < startwall) i = endwall;
            dax2 = ((wall[i].x+wall[wall[wf].point2].x)>>1) - wall[wf].x;
            day2 = ((wall[i].y+wall[wall[wf].point2].y)>>1) - wall[wf].y;

            if (wall[wf].x == dax && wall[wf].y == day)
            {
                //find what direction door should open by averaging the
                //  2 neighboring points of wallfind[0] & wallfind[1].
                if (dax2 != 0)
                {
                    dax2 = wall[wall[wall[wf].point2].point2].x;
                    dax2 -= wall[wall[wf].point2].x;
                    SetAnimation(sn,&wall[wf].x,wall[wf].x+dax2,sp);
                    SetAnimation(sn,&wall[i].x,wall[i].x+dax2,sp);
                    SetAnimation(sn,&wall[wall[wf].point2].x,wall[wall[wf].point2].x+dax2,sp);
                    A_CallSound(sn,ii);
                }
                else if (day2 != 0)
                {
                    day2 = wall[wall[wall[wf].point2].point2].y;
                    day2 -= wall[wall[wf].point2].y;
                    SetAnimation(sn,&wall[wf].y,wall[wf].y+day2,sp);
                    SetAnimation(sn,&wall[i].y,wall[i].y+day2,sp);
                    SetAnimation(sn,&wall[wall[wf].point2].y,wall[wall[wf].point2].y+day2,sp);
                    A_CallSound(sn,ii);
                }
            }
            else
            {
                if (dax2 != 0)
                {
                    SetAnimation(sn,&wall[wf].x,dax,sp);
                    SetAnimation(sn,&wall[i].x,dax+dax2,sp);
                    SetAnimation(sn,&wall[wall[wf].point2].x,dax+dax2,sp);
                    A_CallSound(sn,ii);
                }
                else if (day2 != 0)
                {
                    SetAnimation(sn,&wall[wf].y,day,sp);
                    SetAnimation(sn,&wall[i].y,day+day2,sp);
                    SetAnimation(sn,&wall[wall[wf].point2].y,day+day2,sp);
                    A_CallSound(sn,ii);
                }
            }
        }
    }
    return;

    case ST_15_WARP_ELEVATOR://Warping elevators

        if (sprite[ii].picnum != APLAYER)
            return;

        for (SPRITES_OF_SECT(sn, i))
            if (PN==SECTOREFFECTOR && SLT == SE_17_WARP_ELEVATOR)
                break;

        if (i < 0)
            return;

        if (sprite[ii].sectnum == sn)
        {
            if (G_ActivateWarpElevators(i,-1))
                G_ActivateWarpElevators(i,1);
            else if (G_ActivateWarpElevators(i,1))
                G_ActivateWarpElevators(i,-1);
        }
        else
        {
            if (sptr->floorz > SZ)
                G_ActivateWarpElevators(i,-1);
            else
                G_ActivateWarpElevators(i,1);
        }

        return;

    case ST_16_PLATFORM_DOWN:
    case ST_17_PLATFORM_UP:

        i = GetAnimationGoal(&sptr->floorz);

        if (i == -1)
        {
            i = nextsectorneighborz(sn,sptr->floorz,1,1);
            if (i == -1)
            {
                i = nextsectorneighborz(sn,sptr->floorz,1,-1);
                if (i == -1) return;
                j = sector[i].floorz;
                SetAnimation(sn,&sptr->floorz,j,sptr->extra);
            }
            else
            {
                j = sector[i].floorz;
                SetAnimation(sn,&sptr->floorz,j,sptr->extra);
            }
            A_CallSound(sn,ii);
        }

        return;

    case ST_18_ELEVATOR_DOWN:
    case ST_19_ELEVATOR_UP:

        i = GetAnimationGoal(&sptr->floorz);

        if (i==-1)
        {
            int32_t q;

            i = nextsectorneighborz(sn,sptr->floorz,1,-1);
            if (i==-1) i = nextsectorneighborz(sn,sptr->floorz,1,1);
            if (i==-1) return;
            j = sector[i].floorz;
            q = sptr->extra;
            l = sptr->ceilingz-sptr->floorz;
            SetAnimation(sn,&sptr->floorz,j,q);
            SetAnimation(sn,&sptr->ceilingz,j+l,q);
            A_CallSound(sn,ii);
        }
        return;

    case ST_29_TEETH_DOOR:

        for (SPRITES_OF(STAT_EFFECTOR, i))
            if (SLT == SE_22_TEETH_DOOR && SHT == sptr->hitag)
            {
                sector[SECT].extra = -sector[SECT].extra;

                T1 = sn;
                T2 = 1;
            }

        A_CallSound(sn, ii);

        sptr->lotag ^= 0x8000;

        if (sptr->lotag&0x8000)
        {
            j = nextsectorneighborz(sn,sptr->ceilingz,-1,-1);
            if (j == -1) j = nextsectorneighborz(sn,sptr->ceilingz,1,1);
            if (j == -1)
            {
                OSD_Printf("WARNING: ST29: null sector!\n");
                return;
            }
            j = sector[j].ceilingz;
        }
        else
        {
            j = nextsectorneighborz(sn,sptr->ceilingz,1,1);
            if (j == -1) j = nextsectorneighborz(sn,sptr->ceilingz,-1,-1);
            if (j == -1)
            {
                OSD_Printf("WARNING: ST29: null sector!\n");
                return;
            }
            j = sector[j].floorz;
        }

        SetAnimation(sn,&sptr->ceilingz,j,sptr->extra);

        return;

    case ST_20_CEILING_DOOR:
REDODOOR:

        if (sptr->lotag&0x8000)
        {
            for (SPRITES_OF_SECT(sn, i))
                if (sprite[i].statnum == STAT_EFFECTOR && SLT==SE_9_DOWN_OPEN_DOOR_LIGHTS)
                {
                    j = SZ;
                    break;
                }

            if (i==-1)
                j = sptr->floorz;
        }
        else
        {
            j = nextsectorneighborz(sn,sptr->ceilingz,-1,-1);

            if (j >= 0) j = sector[j].ceilingz;
            else
            {
                sptr->lotag |= 32768;
                goto REDODOOR;
            }
        }

        sptr->lotag ^= 0x8000;

        SetAnimation(sn,&sptr->ceilingz,j,sptr->extra);
        A_CallSound(sn,ii);

        return;

    case ST_21_FLOOR_DOOR:
        i = GetAnimationGoal(&sptr->floorz);
        if (i >= 0)
        {
            if (animategoal[sn] == sptr->ceilingz)
                animategoal[i] = sector[nextsectorneighborz(sn,sptr->ceilingz,1,1)].floorz;
            else animategoal[i] = sptr->ceilingz;
        }
        else
        {
            if (sptr->ceilingz == sptr->floorz)
                j = sector[nextsectorneighborz(sn,sptr->ceilingz,1,1)].floorz;
            else j = sptr->ceilingz;

            sptr->lotag ^= 0x8000;

            if (SetAnimation(sn,&sptr->floorz,j,sptr->extra) >= 0)
                A_CallSound(sn,ii);
        }
        return;

    case ST_22_SPLITTING_DOOR:

        if (sptr->lotag&0x8000)
        {
            int32_t q = (sptr->ceilingz+sptr->floorz)>>1;
            j = SetAnimation(sn, &sptr->floorz, q, sptr->extra);
            j = SetAnimation(sn, &sptr->ceilingz, q, sptr->extra);
        }
        else
        {
            int32_t fneigh = nextsectorneighborz(sn, sptr->floorz, 1, 1);
            int32_t cneigh = nextsectorneighborz(sn, sptr->ceilingz, -1, -1);

            if (fneigh>=0 && cneigh>=0)
            {
                j = SetAnimation(sn, &sptr->floorz, sector[fneigh].floorz, sptr->extra);
                j = SetAnimation(sn, &sptr->ceilingz, sector[cneigh].ceilingz, sptr->extra);
            }
            else
            {
                OSD_Printf("WARNING: ST22: null sector: floor neighbor=%d, ceiling neighbor=%d!\n",
                           fneigh, cneigh);
                sptr->lotag ^= 0x8000;
            }
        }

        sptr->lotag ^= 0x8000;

        A_CallSound(sn,ii);

        return;

    case ST_23_SWINGING_DOOR: //Swingdoor

        j = -1;

        for (SPRITES_OF(STAT_EFFECTOR, i))
            if (SLT == SE_11_SWINGING_DOOR && SECT == sn && !T5)
            {
                j = i;
                break;
            }

        if (i < 0)
        {
            OSD_Printf("WARNING: SE23 i<0!\n");
            return;
        }    // JBF

        l = sector[SECT].lotag&0x8000;

        if (j >= 0)
        {
            int32_t playedsnd = 0;

            for (SPRITES_OF(STAT_EFFECTOR, i))
                if (l == (sector[SECT].lotag&0x8000) && SLT == SE_11_SWINGING_DOOR && sprite[j].hitag == SHT && !T5)
                {
                    if (sector[SECT].lotag&0x8000) sector[SECT].lotag &= 0x7fff;
                    else sector[SECT].lotag |= 0x8000;

                    T5 = 1;
                    T4 = -T4;

                    if (!playedsnd)
                    {
                        A_CallSound(sn,i);
                        playedsnd = 1;
                    }
                }
        }
        return;

    case ST_25_SLIDING_DOOR: //Subway type sliding doors

        for (SPRITES_OF(STAT_EFFECTOR, j))
            if (sprite[j].lotag == SE_15_SLIDING_DOOR && sprite[j].sectnum == sn)
                break; //Found the sectoreffector.

        if (j < 0)
            return;

        for (SPRITES_OF(STAT_EFFECTOR, i))
            if (SHT==sprite[j].hitag)
            {
                if (SLT == SE_15_SLIDING_DOOR)
                {
                    sector[SECT].lotag ^= 0x8000; // Toggle the open or close
                    SA += 1024;

                    if (T5)
                        A_CallSound(SECT,i);
                    A_CallSound(SECT,i);

                    T5 = (sector[SECT].lotag&0x8000) ? 1 : 2;
                }
            }

        return;

    case ST_27_STRETCH_BRIDGE:  //Extended bridge

        for (SPRITES_OF(STAT_EFFECTOR, j))
            if ((sprite[j].lotag&0xff)==SE_20_STRETCH_BRIDGE && sprite[j].sectnum == sn)  //Bridge
            {
                sector[sn].lotag ^= 0x8000;
                // Highest bit now set means we're opening.

                actor[j].t_data[0] = (sector[sn].lotag&0x8000) ? 1 : 2;
                A_CallSound(sn,ii);
                break;
            }

        return;

    case ST_28_DROP_FLOOR:
        //activate the rest of them

        for (SPRITES_OF_SECT(sn, j))
            if (sprite[j].statnum==STAT_EFFECTOR && (sprite[j].lotag&0xff)==SE_21_DROP_FLOOR)
                break;

        if (j >= 0)  // PK: The matching SE21 might have gone, see SE_21_KILLIT in actors.c
        {
            j = sprite[j].hitag;

            for (SPRITES_OF(STAT_EFFECTOR, l))
            {
                if ((sprite[l].lotag&0xff)==SE_21_DROP_FLOOR && !actor[l].t_data[0] &&
                        sprite[l].hitag == j)
                    actor[l].t_data[0] = 1;
            }

            A_CallSound(sn,ii);
        }

        return;
    }
}

void G_OperateRespawns(int32_t low)
{
    int32_t i, nexti;

    for (SPRITES_OF_STAT_SAFE(STAT_FX, i, nexti))
    {
        spritetype *respr = &sprite[i];

        if (respr->lotag == low && respr->picnum == RESPAWN)
        {
            if (!ud.monsters_off || !A_CheckEnemyTile(respr->hitag))
            {
                int32_t j = A_Spawn(i, TRANSPORTERSTAR);
                sprite[j].z -= (32<<8);

                // Just a way to killit (see G_MoveFX(): RESPAWN__STATIC)
                respr->extra = 66-12;
            }
        }
    }
}

void G_OperateActivators(int32_t low, int32_t snum)
{
    int32_t i, nexti, j, k;
    walltype *wal;

    for (i=g_numCyclers-1; i>=0; i--)
    {
        int16_t *const p = &cyclers[i][0];

        if (p[4] == low)
        {
            p[5] = !p[5];

            sector[p[0]].floorshade = sector[p[0]].ceilingshade = p[3];
            wal = &wall[sector[p[0]].wallptr];
            for (j=sector[p[0]].wallnum; j > 0; j--,wal++)
                wal->shade = p[3];
        }
    }

    k = -1;

    for (SPRITES_OF_STAT_SAFE(STAT_ACTIVATOR, i, nexti))
    {
        if (sprite[i].lotag == low)
        {
            if (sprite[i].picnum == ACTIVATORLOCKED)
            {
                sector[SECT].lotag ^= 16384;

                if (snum >= 0 && snum < ud.multimode)
                    P_DoQuote((sector[SECT].lotag&16384) ? QUOTE_LOCKED : QUOTE_UNLOCKED,
                              g_player[snum].ps);
            }
            else
            {
                switch (SHT)
                {
                case 1:
                    if (sector[SECT].floorz != sector[SECT].ceilingz)
                        continue;
                    break;
                case 2:
                    if (sector[SECT].floorz == sector[SECT].ceilingz)
                        continue;
                    break;
                }

                // ST_2_UNDERWATER
                if (sector[sprite[i].sectnum].lotag < 3)
                {
                    for (SPRITES_OF_SECT(sprite[i].sectnum, j))
                        if (sprite[j].statnum == STAT_EFFECTOR)
                            switch (sprite[j].lotag)
                            {
                            case SE_36_PROJ_SHOOTER:
                            case SE_31_FLOOR_RISE_FALL:
                            case SE_32_CEILING_RISE_FALL:
                            case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
                                actor[j].t_data[0] = 1-actor[j].t_data[0];
                                A_CallSound(SECT,j);
                                break;
                            }
                }

                if (k == -1 && (sector[SECT].lotag&0xff) == ST_22_SPLITTING_DOOR)
                    k = A_CallSound(SECT,i);

                G_OperateSectors(SECT,i);
            }
        }
    }

    G_OperateRespawns(low);
}

void G_OperateMasterSwitches(int32_t low)
{
    int32_t i;

    for (SPRITES_OF(STAT_STANDABLE, i))
        if (PN == MASTERSWITCH && SLT == low && SP == 0)
            SP = 1;
}

void G_OperateForceFields(int32_t s, int32_t low)
{
    int32_t p=g_numAnimWalls;

    for (; p>=0; p--)
    {
        const int32_t i = animwall[p].wallnum;

        if (low == wall[i].lotag || low == -1)
            if (G_GetForcefieldPicnum(i) == W_FORCEFIELD
                    || (wall[i].overpicnum == BIGFORCE))
            {
                animwall[p].tag = 0;

                if (wall[i].cstat)
                {
                    wall[i].cstat = 0;

                    if (s >= 0 && sprite[s].picnum == SECTOREFFECTOR && sprite[s].lotag == SE_30_TWO_WAY_TRAIN)
                        wall[i].lotag = 0;
                }
                else
                    wall[i].cstat = FORCEFIELD_CSTAT;
            }
    }
}

// List of switches that function like dip (combination lock) switches.
#define DIPSWITCH_LIKE_CASES \
         DIPSWITCH__STATIC: \
    case TECHSWITCH__STATIC: \
    case ALIENSWITCH__STATIC

// List of access switches.
#define ACCESSSWITCH_CASES \
         ACCESSSWITCH__STATIC: \
    case ACCESSSWITCH2__STATIC

// List of switches that don't fit the two preceding categories, and are not
// the MULTISWITCH. 13 cases.
#define REST_SWITCH_CASES \
         DIPSWITCH2__STATIC: \
    case DIPSWITCH3__STATIC: \
    case FRANKENSTINESWITCH__STATIC: \
    case HANDSWITCH__STATIC: \
    case LIGHTSWITCH2__STATIC: \
    case LIGHTSWITCH__STATIC: \
    case LOCKSWITCH1__STATIC: \
    case POWERSWITCH1__STATIC: \
    case POWERSWITCH2__STATIC: \
    case PULLSWITCH__STATIC: \
    case SLOTDOOR__STATIC: \
    case SPACEDOORSWITCH__STATIC: \
    case SPACELIGHTSWITCH__STATIC

// Returns:
//  0: is not a dipswitch-like switch
//  1: is one, off
//  2: is one, on
static int32_t G_IsLikeDipswitch(int32_t picnum)
{
    int32_t i;

    for (i=0; i<2; i++)
        if (picnum == DIPSWITCH+i || picnum == TECHSWITCH+i || picnum == ALIENSWITCH+i)
            return 1+i;

    return 0;
}

// Get base (unpressed) tile number for switch.
static int32_t G_GetBaseSwitch(int32_t picnum)
{
    if (picnum==DIPSWITCH+1
            || picnum==TECHSWITCH+1
            || picnum==ALIENSWITCH+1
            || picnum==DIPSWITCH2+1
            || picnum==DIPSWITCH3+1
            || picnum==PULLSWITCH+1
            || picnum==HANDSWITCH+1
            || picnum==SLOTDOOR+1
            || picnum==LIGHTSWITCH+1
            || picnum==SPACELIGHTSWITCH+1
            || picnum==SPACEDOORSWITCH+1
            || picnum==FRANKENSTINESWITCH+1
            || picnum==LIGHTSWITCH2+1
            || picnum==POWERSWITCH1+1
            || picnum==LOCKSWITCH1+1
            || picnum==POWERSWITCH2+1
        )
    {
        return picnum-1;
    }

    if (picnum > MULTISWITCH && picnum <= MULTISWITCH+3)
    {
        return MULTISWITCH;
    }

    return picnum;
}

int32_t P_ActivateSwitch(int32_t snum, int32_t w, int32_t switchissprite)
{
    int32_t lotag, hitag, picnum, switchpal;
    vec3_t davector;

    int32_t switchpicnum;
    int32_t i, correctdips = 1, numdips = 0;

    if (w < 0)
        return 0;

    if (switchissprite == 1) // A wall sprite
    {
        if (actor[w].lasttransport == totalclock)
            return 0;

        actor[w].lasttransport = totalclock;

        lotag = sprite[w].lotag;
        if (lotag == 0)
            return 0;

        hitag = sprite[w].hitag;

        Bmemcpy(&davector, &sprite[w], sizeof(vec3_t));
        picnum = sprite[w].picnum;
        switchpal = sprite[w].pal;
    }
    else
    {
        lotag = wall[w].lotag;
        if (lotag == 0)
            return 0;

        hitag = wall[w].hitag;

        Bmemcpy(&davector, &wall[w], sizeof(int32_t) * 2);
        davector.z = g_player[snum].ps->pos.z;
        picnum = wall[w].picnum;
        switchpal = wall[w].pal;
    }

//    initprintf("P_ActivateSwitch called picnum=%i switchissprite=%i\n",picnum,switchissprite);

    switchpicnum = G_GetBaseSwitch(picnum);

    switch (DYNAMICTILEMAP(switchpicnum))
    {
    case DIPSWITCH_LIKE_CASES:
        break;

    case ACCESSSWITCH_CASES:
        if (g_player[snum].ps->access_incs == 0)
        {
            int32_t pp;

            static const int32_t key_switchpal[3] = { 0, 21, 23 };
            static const int32_t need_key_quote[3] = {
                QUOTE_NEED_BLUE_KEY, QUOTE_NEED_RED_KEY, QUOTE_NEED_YELLOW_KEY
            };

            for (pp=0; pp<3; pp++)
                if (switchpal == key_switchpal[pp])
                {
                    if (g_player[snum].ps->got_access & (1<<pp))
                        g_player[snum].ps->access_incs = 1;
                    else
                        P_DoQuote(need_key_quote[pp], g_player[snum].ps);

                    break;
                }

            if (g_player[snum].ps->access_incs == 1)
            {
                if (switchissprite == 0)
                    g_player[snum].ps->access_wallnum = w;
                else
                    g_player[snum].ps->access_spritenum = w;
            }

            return 0;
        }
        /* fall-through (XXX: intended?) */
    case MULTISWITCH__STATIC:
    case REST_SWITCH_CASES:
        if (G_CheckActivatorMotion(lotag))
            return 0;
        break;

    default:
        if (CheckDoorTile(picnum) == 0)
            return 0;
        break;
    }

    for (SPRITES_OF(STAT_DEFAULT, i))
    {
        if (lotag == SLT)
        {
            // Put the tile number into a variable so later switches don't
            // trigger on the result of changes:
            const int32_t switchpic = PN;

            if (switchpic >= MULTISWITCH && switchpic <= MULTISWITCH+3)
            {
                sprite[i].picnum++;
                if (sprite[i].picnum > MULTISWITCH+3)
                    sprite[i].picnum = MULTISWITCH;
            }

            switch (DYNAMICTILEMAP(switchpic))
            {
            case DIPSWITCH_LIKE_CASES:
                if (switchissprite == 1 && w == i)
                    PN++;
                else if (SHT == 0)
                    correctdips++;
                numdips++;
                break;

            case ACCESSSWITCH_CASES:
            case REST_SWITCH_CASES:
                sprite[i].picnum++;
                break;

            default:
                if (switchpic <= 0)  // oob safety
                    break;

                switch (DYNAMICTILEMAP(switchpic-1))
                {
                case DIPSWITCH_LIKE_CASES:
                    if (switchissprite == 1 && w == i)
                        PN--;
                    else if (SHT == 1)
                        correctdips++;
                    numdips++;
                    break;

                case REST_SWITCH_CASES:
                    sprite[i].picnum--;
                    break;
                }
                break;
            }
        }
    }

    for (i=numwalls-1; i>=0; i--)
    {
        const int32_t x = i;

        if (lotag == wall[x].lotag)
        {
            if (wall[x].picnum >= MULTISWITCH && wall[x].picnum <= MULTISWITCH+3)
            {
                wall[x].picnum++;
                if (wall[x].picnum > MULTISWITCH+3)
                    wall[x].picnum = MULTISWITCH;
            }

            switch (DYNAMICTILEMAP(wall[x].picnum))
            {
            case DIPSWITCH_LIKE_CASES:
                if (switchissprite == 0 && i == w)
                    wall[x].picnum++;
                else if (wall[x].hitag == 0)
                    correctdips++;
                numdips++;
                break;

            case ACCESSSWITCH_CASES:
            case REST_SWITCH_CASES:
                wall[x].picnum++;
                break;

            default:
                if (wall[x].picnum <= 0)  // oob safety
                    break;

                switch (DYNAMICTILEMAP(wall[x].picnum-1))
                {
                case DIPSWITCH_LIKE_CASES:
                    if (switchissprite == 0 && i == w)
                        wall[x].picnum--;
                    else if (wall[x].hitag == 1)
                        correctdips++;
                    numdips++;
                    break;

                case REST_SWITCH_CASES:
                    wall[x].picnum--;
                    break;
                }
                break;
            }
        }
    }

    if (lotag == 65535)
    {
        g_player[myconnectindex].ps->gm = MODE_EOL;

        if (ud.from_bonus)
        {
            ud.level_number = ud.from_bonus;
            ud.m_level_number = ud.level_number;
            ud.from_bonus = 0;
        }
        else
        {
            ud.level_number++;
            if (ud.level_number > MAXLEVELS-1)
                ud.level_number = 0;
            ud.m_level_number = ud.level_number;
        }

        return 1;
    }

    switchpicnum = G_GetBaseSwitch(picnum);

    switch (DYNAMICTILEMAP(switchpicnum))
    {
    default:
        if (CheckDoorTile(picnum) == 0)
            break;
        /* fall-through */
    case DIPSWITCH_LIKE_CASES:
        if (G_IsLikeDipswitch(picnum))
        {
            if (picnum == ALIENSWITCH || picnum == ALIENSWITCH+1)
            {
                if (switchissprite == 1)
                    S_PlaySound3D(ALIEN_SWITCH1, w, &davector);
                else
                    S_PlaySound3D(ALIEN_SWITCH1, g_player[snum].ps->i, &davector);
            }
            else
            {
                if (switchissprite == 1)
                    S_PlaySound3D(SWITCH_ON, w, &davector);
                else
                    S_PlaySound3D(SWITCH_ON, g_player[snum].ps->i, &davector);
            }

            if (numdips != correctdips)
                break;

            S_PlaySound3D(END_OF_LEVEL_WARN, g_player[snum].ps->i, &davector);
        }
        /* fall-through */
    case ACCESSSWITCH_CASES:
    case MULTISWITCH__STATIC:
    case REST_SWITCH_CASES:
    {
        int32_t x;

        if (picnum >= MULTISWITCH && picnum <= MULTISWITCH+3)
            lotag += picnum - MULTISWITCH;

        for (SPRITES_OF(STAT_EFFECTOR, x))
        {
            if (sprite[x].hitag == lotag)
            {
                switch (sprite[x].lotag)
                {
                case SE_12_LIGHT_SWITCH:
                    sector[sprite[x].sectnum].floorpal = 0;
                    actor[x].t_data[0]++;
                    if (actor[x].t_data[0] == 2)
                        actor[x].t_data[0]++;
                    break;

                case SE_24_CONVEYOR:
                case SE_34:
                case SE_25_PISTON:
                    actor[x].t_data[4] = !actor[x].t_data[4];
                    P_DoQuote(actor[x].t_data[4] ? QUOTE_DEACTIVATED : QUOTE_ACTIVATED,
                              g_player[snum].ps);
                    break;

                case SE_21_DROP_FLOOR:
                    P_DoQuote(QUOTE_ACTIVATED, g_player[screenpeek].ps);
                    break;
                }
            }
        }

        G_OperateActivators(lotag,snum);
        G_OperateForceFields(g_player[snum].ps->i, lotag);
        G_OperateMasterSwitches(lotag);

        if (G_IsLikeDipswitch(picnum))
            return 1;

        if (hitag == 0 && CheckDoorTile(picnum) == 0)
        {
            if (switchissprite == 1)
                S_PlaySound3D(SWITCH_ON, w, &davector);
            else
                S_PlaySound3D(SWITCH_ON, g_player[snum].ps->i, &davector);
        }
        else if (hitag != 0)
        {
            if (switchissprite == 1 && (g_sounds[hitag].m & SF_TALK) == 0)
                S_PlaySound3D(hitag, w, &davector);
            else
                A_PlaySound(hitag, g_player[snum].ps->i);
        }

        return 1;
    }
    }

    return 0;
}

void G_ActivateBySector(int32_t sect, int32_t j)
{
    int32_t i;
    int32_t didit = 0;

    for (SPRITES_OF_SECT(sect, i))
        if (PN == ACTIVATOR)
        {
            G_OperateActivators(SLT,-1);
            didit = 1;
        }

    if (didit == 0)
        G_OperateSectors(sect,j);
}

static void BreakWall(int32_t newpn, int32_t spr, int32_t dawallnum)
{
    wall[dawallnum].picnum = newpn;
    A_PlaySound(VENT_BUST,spr);
    A_PlaySound(GLASS_HEAVYBREAK,spr);
    A_SpawnWallGlass(spr,dawallnum,10);
}

void A_DamageWall(int32_t spr, int32_t dawallnum, const vec3_t *pos, int32_t atwith)
{
    int16_t sn = -1;
    int32_t j, i, darkestwall;
    walltype *wal = &wall[dawallnum];

    if (wal->overpicnum == MIRROR && wal->pal != 4 &&
            A_CheckSpriteTileFlags(atwith,SFLAG_PROJECTILE) &&
            (SpriteProjectile[spr].workslike & PROJECTILE_RPG))
    {
        if (wal->nextwall == -1 || wall[wal->nextwall].pal != 4)
        {
            A_SpawnWallGlass(spr,dawallnum,70);
            wal->cstat &= ~16;
            wal->overpicnum = MIRRORBROKE;
            A_PlaySound(GLASS_HEAVYBREAK,spr);
            return;
        }
    }

    if (wal->overpicnum == MIRROR && wal->pal != 4)
    {
        switch (DYNAMICTILEMAP(atwith))
        {
        case HEAVYHBOMB__STATIC:
        case RADIUSEXPLOSION__STATIC:
        case RPG__STATIC:
        case HYDRENT__STATIC:
        case SEENINE__STATIC:
        case OOZFILTER__STATIC:
        case EXPLODINGBARREL__STATIC:
            if (wal->nextwall == -1 || wall[wal->nextwall].pal != 4)
            {
                A_SpawnWallGlass(spr,dawallnum,70);
                wal->cstat &= ~16;
                wal->overpicnum = MIRRORBROKE;
                A_PlaySound(GLASS_HEAVYBREAK,spr);
                return;
            }
        }
    }

    if (((wal->cstat&16) || wal->overpicnum == BIGFORCE) && wal->nextsector >= 0)
        if (sector[wal->nextsector].floorz > pos->z)
            if (sector[wal->nextsector].floorz != sector[wal->nextsector].ceilingz)
            {
                int32_t switchpicnum = G_GetForcefieldPicnum(dawallnum);

                switch (DYNAMICTILEMAP(switchpicnum))
                {
                case W_FORCEFIELD__STATIC:
                    wal->extra = 1; // tell the forces to animate
                    /* fall-through */
                case BIGFORCE__STATIC:
                    updatesector(pos->x, pos->y, &sn);
                    if (sn < 0)
                        return;

                    {
                        int32_t xr=32, yr=32;

                        if (atwith == -1)
                            xr = yr = 8;
                        else if (atwith == CHAINGUN)
                        {
                            xr = 16+sprite[spr].xrepeat;
                            yr = 16+sprite[spr].yrepeat;
                        }

                        i = A_InsertSprite(sn, pos->x,pos->y,pos->z, FORCERIPPLE,
                                           -127, xr,yr, 0,0,0,spr,5);
                    }

                    CS |= 18+128;
                    SA = getangle(wal->x-wall[wal->point2].x,
                                  wal->y-wall[wal->point2].y)-512;

                    A_PlaySound(SOMETHINGHITFORCE,i);
                    return;

                case FANSPRITE__STATIC:
                    wal->overpicnum = FANSPRITEBROKE;
                    wal->cstat &= 65535-65;
                    if (wal->nextwall >= 0)
                    {
                        wall[wal->nextwall].overpicnum = FANSPRITEBROKE;
                        wall[wal->nextwall].cstat &= 65535-65;
                    }
                    A_PlaySound(VENT_BUST,spr);
                    A_PlaySound(GLASS_BREAKING,spr);
                    return;

                case GLASS__STATIC:
                    updatesector(pos->x,pos->y,&sn);
                    if (sn < 0)
                        return;
                    wal->overpicnum=GLASS2;
                    A_SpawnWallGlass(spr,dawallnum,10);
                    wal->cstat = 0;

                    if (wal->nextwall >= 0)
                        wall[wal->nextwall].cstat = 0;

                    i = A_InsertSprite(sn,pos->x,pos->y,pos->z,SECTOREFFECTOR,0,0,0,g_player[0].ps->ang,0,0,spr,3);
                    SLT = 128;
                    T2 = 5;
                    T3 = dawallnum;
                    A_PlaySound(GLASS_BREAKING,i);
                    return;

                case STAINGLASS1__STATIC:
                    updatesector(pos->x,pos->y,&sn);
                    if (sn < 0)
                        return;
                    A_SpawnRandomGlass(spr,dawallnum,80);
                    wal->cstat = 0;
                    if (wal->nextwall >= 0)
                        wall[wal->nextwall].cstat = 0;
                    A_PlaySound(VENT_BUST,spr);
                    A_PlaySound(GLASS_BREAKING,spr);
                    return;
                }
            }

    switch (DYNAMICTILEMAP(wal->picnum))
    {
    case COLAMACHINE__STATIC:
    case VENDMACHINE__STATIC:
        BreakWall(wal->picnum+2,spr,dawallnum);
        A_PlaySound(VENT_BUST,spr);
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
        A_SpawnWallGlass(spr,dawallnum,30);
        wal->picnum=W_SCREENBREAK+(krand()%3);
        A_PlaySound(GLASS_HEAVYBREAK,spr);
        return;

    case W_TECHWALL5__STATIC:
    case W_TECHWALL6__STATIC:
    case W_TECHWALL7__STATIC:
    case W_TECHWALL8__STATIC:
    case W_TECHWALL9__STATIC:
        BreakWall(wal->picnum+1,spr,dawallnum);
        return;

    case W_MILKSHELF__STATIC:
        BreakWall(W_MILKSHELFBROKE,spr,dawallnum);
        return;

    case W_TECHWALL10__STATIC:
        BreakWall(W_HITTECHWALL10,spr,dawallnum);
        return;

    case W_TECHWALL1__STATIC:
    case W_TECHWALL11__STATIC:
    case W_TECHWALL12__STATIC:
    case W_TECHWALL13__STATIC:
    case W_TECHWALL14__STATIC:
        BreakWall(W_HITTECHWALL1,spr,dawallnum);
        return;

    case W_TECHWALL15__STATIC:
        BreakWall(W_HITTECHWALL15,spr,dawallnum);
        return;

    case W_TECHWALL16__STATIC:
        BreakWall(W_HITTECHWALL16,spr,dawallnum);
        return;

    case W_TECHWALL2__STATIC:
        BreakWall(W_HITTECHWALL2,spr,dawallnum);
        return;

    case W_TECHWALL3__STATIC:
        BreakWall(W_HITTECHWALL3,spr,dawallnum);
        return;

    case W_TECHWALL4__STATIC:
        BreakWall(W_HITTECHWALL4,spr,dawallnum);
        return;

    case ATM__STATIC:
        wal->picnum = ATMBROKE;
        A_SpawnMultiple(spr, MONEY, 1+(krand()&7));
        A_PlaySound(GLASS_HEAVYBREAK,spr);
        break;

    case WALLLIGHT1__STATIC:
    case WALLLIGHT2__STATIC:
    case WALLLIGHT3__STATIC:
    case WALLLIGHT4__STATIC:
    case TECHLIGHT2__STATIC:
    case TECHLIGHT4__STATIC:
        A_PlaySound(rnd(128) ? GLASS_HEAVYBREAK : GLASS_BREAKING, spr);
        A_SpawnWallGlass(spr,dawallnum,30);

        if (wal->picnum == WALLLIGHT1)
            wal->picnum = WALLLIGHTBUST1;

        if (wal->picnum == WALLLIGHT2)
            wal->picnum = WALLLIGHTBUST2;

        if (wal->picnum == WALLLIGHT3)
            wal->picnum = WALLLIGHTBUST3;

        if (wal->picnum == WALLLIGHT4)
            wal->picnum = WALLLIGHTBUST4;

        if (wal->picnum == TECHLIGHT2)
            wal->picnum = TECHLIGHTBUST2;

        if (wal->picnum == TECHLIGHT4)
            wal->picnum = TECHLIGHTBUST4;

        if (wal->lotag == 0)
            return;

        sn = wal->nextsector;
        if (sn < 0)
            return;
        darkestwall = 0;

        wal = &wall[sector[sn].wallptr];
        for (i=sector[sn].wallnum; i > 0; i--,wal++)
            if (wal->shade > darkestwall)
                darkestwall=wal->shade;

        j = krand()&1;

        for (SPRITES_OF(STAT_EFFECTOR, i))
            if (SHT == wall[dawallnum].lotag && SLT == SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT)
            {
                T3 = j;
                T4 = darkestwall;
                T5 = 1;
            }

        break;
    }
}

// NOTE: return value never examined in any of the callers.
int32_t Sect_DamageCeilingOrFloor(int32_t floorp, int32_t sn)
{
    int32_t i, j;

    const int32_t RETURN_in = floorp ? 131072+sn : 65536+sn;
    // NOTE: pass RETURN in the dist argument, too.
    int32_t ret = VM_OnEventWithBoth(EVENT_DAMAGEHPLANE, g_player[screenpeek].ps->i, screenpeek,
                             RETURN_in, RETURN_in);

    if (ret < 0)
        return 0;

    if (floorp)
        return 0;

    if (ret == (1<<20))
    {
        // Execute the hard-coded stuff without changing picnum (expected to
        // have been done by the event).
        goto GLASSBREAK_CODE;
    }

    switch (DYNAMICTILEMAP(sector[sn].ceilingpicnum))
    {
    case WALLLIGHT1__STATIC:
    case WALLLIGHT2__STATIC:
    case WALLLIGHT3__STATIC:
    case WALLLIGHT4__STATIC:
    case TECHLIGHT2__STATIC:
    case TECHLIGHT4__STATIC:

        if (sector[sn].ceilingpicnum == WALLLIGHT1)
            sector[sn].ceilingpicnum = WALLLIGHTBUST1;

        if (sector[sn].ceilingpicnum == WALLLIGHT2)
            sector[sn].ceilingpicnum = WALLLIGHTBUST2;

        if (sector[sn].ceilingpicnum == WALLLIGHT3)
            sector[sn].ceilingpicnum = WALLLIGHTBUST3;

        if (sector[sn].ceilingpicnum == WALLLIGHT4)
            sector[sn].ceilingpicnum = WALLLIGHTBUST4;

        if (sector[sn].ceilingpicnum == TECHLIGHT2)
            sector[sn].ceilingpicnum = TECHLIGHTBUST2;

        if (sector[sn].ceilingpicnum == TECHLIGHT4)
            sector[sn].ceilingpicnum = TECHLIGHTBUST4;

    GLASSBREAK_CODE:
        A_SpawnCeilingGlass(g_player[myconnectindex].ps->i,sn,10);
        A_PlaySound(GLASS_BREAKING,g_player[screenpeek].ps->i);

        if (sector[sn].hitag == 0)
        {
            for (SPRITES_OF_SECT(sn, i))
            {
                if (PN == SECTOREFFECTOR && SLT == SE_12_LIGHT_SWITCH)
                {
                    for (SPRITES_OF(STAT_EFFECTOR, j))
                        if (sprite[j].hitag == SHT)
                            actor[j].t_data[3] = 1;
                    break;
                }
            }
        }

        j = krand()&1;

        for (SPRITES_OF(STAT_EFFECTOR, i))
            if (SHT == sector[sn].hitag && SLT == SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT)
            {
                T3 = j;
                T5 = 1;
            }

        return 1;
    }

    return 0;
}

// hard coded props... :(
void A_DamageObject(int32_t i,int32_t sn)
{
    int16_t j;
    int32_t k, rpg=0;
    spritetype *s;

    if (g_netClient)
    {
        return;
    }

    i &= (MAXSPRITES-1);

    if (A_CheckSpriteFlags(sn,SFLAG_PROJECTILE))
        if (SpriteProjectile[sn].workslike & PROJECTILE_RPG)
            rpg = 1;

    switch (DYNAMICTILEMAP(PN))
    {
    case OCEANSPRITE1__STATIC:
    case OCEANSPRITE2__STATIC:
    case OCEANSPRITE3__STATIC:
    case OCEANSPRITE4__STATIC:
    case OCEANSPRITE5__STATIC:
        A_Spawn(i,SMALLSMOKE);
        A_DeleteSprite(i);
        break;

    case QUEBALL__STATIC:
    case STRIPEBALL__STATIC:
        if (sprite[sn].picnum == QUEBALL || sprite[sn].picnum == STRIPEBALL)
        {
            sprite[sn].xvel = (sprite[i].xvel>>1)+(sprite[i].xvel>>2);
            sprite[sn].ang -= (SA<<1)+1024;
            SA = getangle(SX-sprite[sn].x,SY-sprite[sn].y)-512;
            if (S_CheckSoundPlaying(i,POOLBALLHIT) < 2)
                A_PlaySound(POOLBALLHIT,i);
        }
        else
        {
            if (krand()&3)
            {
                sprite[i].xvel = 164;
                sprite[i].ang = sprite[sn].ang;
            }
            else
            {
                A_SpawnWallGlass(i,-1,3);
                A_DeleteSprite(i);
            }
        }
        break;

    case TREE1__STATIC:
    case TREE2__STATIC:
    case TIRE__STATIC:
    case CONE__STATIC:
    case BOX__STATIC:
    {
        if (rpg == 1)
            if (T1 == 0)
            {
                CS &= ~257;
                T1 = 1;
                A_Spawn(i,BURNING);
            }
        switch (DYNAMICTILEMAP(sprite[sn].picnum))
        {
        case RADIUSEXPLOSION__STATIC:
        case RPG__STATIC:
        case FIRELASER__STATIC:
        case HYDRENT__STATIC:
        case HEAVYHBOMB__STATIC:
            if (T1 == 0)
            {
                CS &= ~257;
                T1 = 1;
                A_Spawn(i,BURNING);
            }
            break;
        }
        break;
    }

    case CACTUS__STATIC:
    {
        if (rpg == 1)
            for (k=64; k>0; k--)
            {
                j = A_InsertSprite(SECT,SX,SY,SZ-(krand()%(48<<8)),SCRAP3+(krand()&3),-8,48,48,krand()&2047,(krand()&63)+64,-(krand()&4095)-(sprite[i].zvel>>2),i,5);
                sprite[j].pal = 8;
            }
        //        case CACTUSBROKE:
        switch (DYNAMICTILEMAP(sprite[sn].picnum))
        {
        case RADIUSEXPLOSION__STATIC:
        case RPG__STATIC:
        case FIRELASER__STATIC:
        case HYDRENT__STATIC:
        case HEAVYHBOMB__STATIC:
            for (k=64; k>0; k--)
            {
                j = A_InsertSprite(SECT,SX,SY,SZ-(krand()%(48<<8)),SCRAP3+(krand()&3),-8,48,48,krand()&2047,(krand()&63)+64,-(krand()&4095)-(sprite[i].zvel>>2),i,5);
                sprite[j].pal = 8;
            }

            if (PN == CACTUS)
                PN = CACTUSBROKE;
            CS &= ~257;
            //       else A_DeleteSprite(i);
            break;
        }
        break;
    }

    case HANGLIGHT__STATIC:
    case GENERICPOLE2__STATIC:
        for (k=6; k>0; k--)
            A_InsertSprite(SECT,SX,SY,SZ-(8<<8),SCRAP1+(krand()&15),-8,48,48,krand()&2047,(krand()&63)+64,-(krand()&4095)-(sprite[i].zvel>>2),i,5);
        A_PlaySound(GLASS_HEAVYBREAK,i);
        A_DeleteSprite(i);
        break;

    case FANSPRITE__STATIC:
        PN = FANSPRITEBROKE;
        CS &= (65535-257);
        if (sector[SECT].floorpicnum == FANSHADOW)
            sector[SECT].floorpicnum = FANSHADOWBROKE;

        A_PlaySound(GLASS_HEAVYBREAK,i);
        s = &sprite[i];
        for (j=16; j>0; j--)
            RANDOMSCRAP;

        break;

    case WATERFOUNTAIN__STATIC:
        //    case WATERFOUNTAIN+1:
        //    case WATERFOUNTAIN+2:
        PN = WATERFOUNTAINBROKE;
        A_Spawn(i,TOILETWATER);
        break;

    case SATELITE__STATIC:
    case FUELPOD__STATIC:
    case SOLARPANNEL__STATIC:
    case ANTENNA__STATIC:
        if (sprite[sn].extra != G_InitialActorStrength(SHOTSPARK1))
        {
            for (j=15; j>0; j--)
                A_InsertSprite(SECT,SX,SY,sector[SECT].floorz-(12<<8)-(j<<9),SCRAP1+(krand()&15),-8,64,64,
                               krand()&2047,(krand()&127)+64,-(krand()&511)-256,i,5);
            A_Spawn(i,EXPLOSION2);
            A_DeleteSprite(i);
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
        if (PN == BOTTLE10)
            A_SpawnMultiple(i, MONEY, 4+(krand()&3));
        else if (PN == STATUE || PN == STATUEFLASH)
        {
            A_SpawnRandomGlass(i,-1,40);
            A_PlaySound(GLASS_HEAVYBREAK,i);
        }
        else if (PN == VASE)
            A_SpawnWallGlass(i,-1,40);

        A_PlaySound(GLASS_BREAKING,i);
        SA = krand()&2047;
        A_SpawnWallGlass(i,-1,8);
        A_DeleteSprite(i);
        break;

    case FETUS__STATIC:
        PN = FETUSBROKE;
        A_PlaySound(GLASS_BREAKING,i);
        A_SpawnWallGlass(i,-1,10);
        break;

    case FETUSBROKE__STATIC:
        for (j=48; j>0; j--)
        {
            A_Shoot(i,BLOODSPLAT1);
            SA += 333;
        }
        A_PlaySound(GLASS_HEAVYBREAK,i);
        A_PlaySound(SQUISHED,i);
        /* fall-through */
    case BOTTLE7__STATIC:
        A_PlaySound(GLASS_BREAKING,i);
        A_SpawnWallGlass(i,-1,10);
        A_DeleteSprite(i);
        break;

    case HYDROPLANT__STATIC:
        PN = BROKEHYDROPLANT;
        A_PlaySound(GLASS_BREAKING,i);
        A_SpawnWallGlass(i,-1,10);
        break;

    case FORCESPHERE__STATIC:
        sprite[i].xrepeat = 0;
        actor[OW].t_data[0] = 32;
        actor[OW].t_data[1] = !actor[OW].t_data[1];
        actor[OW].t_data[2] ++;
        A_Spawn(i,EXPLOSION2);
        break;

    case BROKEHYDROPLANT__STATIC:
        A_PlaySound(GLASS_BREAKING,i);
        A_SpawnWallGlass(i,-1,5);
        A_DeleteSprite(i);
        break;

    case TOILET__STATIC:
        PN = TOILETBROKE;
        CS |= (krand()&1)<<2;
        CS &= ~257;
        A_Spawn(i,TOILETWATER);
        A_PlaySound(GLASS_BREAKING,i);
        break;

    case STALL__STATIC:
        PN = STALLBROKE;
        CS |= (krand()&1)<<2;
        CS &= ~257;
        A_Spawn(i,TOILETWATER);
        A_PlaySound(GLASS_HEAVYBREAK,i);
        break;

    case HYDRENT__STATIC:
        PN = BROKEFIREHYDRENT;
        A_Spawn(i,TOILETWATER);

        //            for(k=0;k<5;k++)
        //          {
        //            j = A_InsertSprite(SECT,SX,SY,SZ-(krand()%(48<<8)),SCRAP3+(krand()&3),-8,48,48,krand()&2047,(krand()&63)+64,-(krand()&4095)-(sprite[i].zvel>>2),i,5);
        //          sprite[j].pal = 2;
        //    }
        A_PlaySound(GLASS_HEAVYBREAK,i);
        break;

    case GRATE1__STATIC:
        PN = BGRATE1;
        CS &= (65535-256-1);
        A_PlaySound(VENT_BUST,i);
        break;

    case CIRCLEPANNEL__STATIC:
        PN = CIRCLEPANNELBROKE;
        CS &= (65535-256-1);
        A_PlaySound(VENT_BUST,i);
        break;

    case PANNEL1__STATIC:
    case PANNEL2__STATIC:
        PN = BPANNEL1;
        CS &= (65535-256-1);
        A_PlaySound(VENT_BUST,i);
        break;

    case PANNEL3__STATIC:
        PN = BPANNEL3;
        CS &= (65535-256-1);
        A_PlaySound(VENT_BUST,i);
        break;

    case PIPE1__STATIC:
    case PIPE2__STATIC:
    case PIPE3__STATIC:
    case PIPE4__STATIC:
    case PIPE5__STATIC:
    case PIPE6__STATIC:
        switch (DYNAMICTILEMAP(PN))
        {
        case PIPE1__STATIC:
            PN=PIPE1B;
            break;
        case PIPE2__STATIC:
            PN=PIPE2B;
            break;
        case PIPE3__STATIC:
            PN=PIPE3B;
            break;
        case PIPE4__STATIC:
            PN=PIPE4B;
            break;
        case PIPE5__STATIC:
            PN=PIPE5B;
            break;
        case PIPE6__STATIC:
            PN=PIPE6B;
            break;
        }

        j = A_Spawn(i,STEAM);
        sprite[j].z = sector[SECT].floorz-(32<<8);
        break;

    case MONK__STATIC:
    case LUKE__STATIC:
    case INDY__STATIC:
    case JURYGUY__STATIC:
        A_PlaySound(SLT,i);
        A_Spawn(i,SHT);
        /* fall-through */
    case SPACEMARINE__STATIC:
        sprite[i].extra -= sprite[sn].extra;
        if (sprite[i].extra > 0) break;
        SA = krand()&2047;
        A_Shoot(i,BLOODSPLAT1);
        SA = krand()&2047;
        A_Shoot(i,BLOODSPLAT2);
        SA = krand()&2047;
        A_Shoot(i,BLOODSPLAT3);
        SA = krand()&2047;
        A_Shoot(i,BLOODSPLAT4);
        SA = krand()&2047;
        A_Shoot(i,BLOODSPLAT1);
        SA = krand()&2047;
        A_Shoot(i,BLOODSPLAT2);
        SA = krand()&2047;
        A_Shoot(i,BLOODSPLAT3);
        SA = krand()&2047;
        A_Shoot(i,BLOODSPLAT4);
        A_DoGuts(i,JIBS1,1);
        A_DoGuts(i,JIBS2,2);
        A_DoGuts(i,JIBS3,3);
        A_DoGuts(i,JIBS4,4);
        A_DoGuts(i,JIBS5,1);
        A_DoGuts(i,JIBS3,6);
        S_PlaySound(SQUISHED);
        A_DeleteSprite(i);
        break;

    case CHAIR1__STATIC:
    case CHAIR2__STATIC:
        PN = BROKENCHAIR;
        CS = 0;
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
        A_PlaySound(GLASS_HEAVYBREAK,i);
        s = &sprite[i];
        for (j=16; j>0; j--)
            RANDOMSCRAP;
        A_DeleteSprite(i);
        break;

    case PLAYERONWATER__STATIC:
        i = OW;
        /* fall-through */
    default:
        if ((sprite[i].cstat&16) && SHT == 0 && SLT == 0 && sprite[i].statnum == STAT_DEFAULT)
            break;

        if ((sprite[sn].picnum == FREEZEBLAST || sprite[sn].owner != i) && sprite[i].statnum != STAT_PROJECTILE)
        {
            if (A_CheckEnemySprite(&sprite[i]) == 1)
            {
                if (sprite[sn].picnum == RPG)
                    sprite[sn].extra <<= 1;

                if ((PN != DRONE) && (PN != ROTATEGUN) && (PN != COMMANDER) && (PN < GREENSLIME || PN > GREENSLIME+7))
                    if (sprite[sn].picnum != FREEZEBLAST)
                        if (!A_CheckSpriteTileFlags(PN, SFLAG_BADGUY) || A_CheckSpriteTileFlags(PN, SFLAG_HURTSPAWNBLOOD))
                        {
                            j = A_Spawn(sn,JIBS6);
                            if (sprite[sn].pal == 6)
                                sprite[j].pal = 6;
                            sprite[j].z += (4<<8);
                            sprite[j].xvel = 16;
                            sprite[j].xrepeat = sprite[j].yrepeat = 24;
                            sprite[j].ang += 32-(krand()&63);
                        }

                j = sprite[sn].owner;

                if (j >= 0 && sprite[j].picnum == APLAYER && PN != ROTATEGUN && PN != DRONE)
                    if (g_player[P_Get(j)].ps->curr_weapon == SHOTGUN_WEAPON)
                        if (!A_CheckSpriteTileFlags(PN, SFLAG_BADGUY) || A_CheckSpriteTileFlags(PN, SFLAG_HURTSPAWNBLOOD))
                        {
                            A_Shoot(i,BLOODSPLAT3);
                            A_Shoot(i,BLOODSPLAT1);
                            A_Shoot(i,BLOODSPLAT2);
                            A_Shoot(i,BLOODSPLAT4);
                        }

                if (!A_CheckSpriteFlags(i, SFLAG_NODAMAGEPUSH))
                {
                    if (sprite[i].extra > 0)
                    {
                        if ((sprite[i].cstat&48) == 0)
                            SA = (sprite[sn].ang+1024)&2047;
                        sprite[i].xvel = -(sprite[sn].extra<<2);
                        j = SECT;
                        pushmove((vec3_t *)&sprite[i],&j,128L,(4L<<8),(4L<<8),CLIPMASK0);
                        if (j != SECT && (unsigned)j < MAXSECTORS)
                            changespritesect(i,j);
                    }
                }

                if (sprite[i].statnum == STAT_ZOMBIEACTOR)
                {
                    changespritestat(i, STAT_ACTOR);
                    actor[i].timetosleep = SLEEPTIME;
                }

                if ((sprite[i].xrepeat < 24 || PN == SHARK) && sprite[sn].picnum == SHRINKSPARK)
                    return;
            }

            if (sprite[i].statnum != STAT_ZOMBIEACTOR)
            {
                if (sprite[sn].picnum == FREEZEBLAST && ((PN == APLAYER && sprite[i].pal == 1) || (g_freezerSelfDamage == 0 && sprite[sn].owner == i)))
                    return;

                actor[i].picnum = sprite[sn].picnum;
                actor[i].extra += sprite[sn].extra;
                actor[i].ang = sprite[sn].ang;
                actor[i].owner = sprite[sn].owner;
            }

            if (sprite[i].statnum == STAT_PLAYER)
            {
                DukePlayer_t *ps = g_player[P_Get(i)].ps;

                if (ps->newowner >= 0)
                    G_ClearCameraView(ps);

                if (sprite[i].xrepeat < 24 && sprite[sn].picnum == SHRINKSPARK)
                    return;

                if (sprite[actor[i].owner].picnum != APLAYER)
                    if (ud.player_skill >= 3)
                        sprite[sn].extra += (sprite[sn].extra>>1);
            }
        }

        break;
    }
}

void G_AlignWarpElevators(void)
{
    int32_t i, j;

    for (SPRITES_OF(STAT_EFFECTOR, i))
    {
        if (SLT == SE_17_WARP_ELEVATOR && SS > 16)
        {
            for (SPRITES_OF(STAT_EFFECTOR, j))
            {
                if (i != j && sprite[j].lotag == SE_17_WARP_ELEVATOR && SHT == sprite[j].hitag)
                {
                    sector[sprite[j].sectnum].floorz = sector[SECT].floorz;
                    sector[sprite[j].sectnum].ceilingz = sector[SECT].ceilingz;
                }
            }
        }
    }
}


static int32_t P_CheckDetonatorSpecialCase(DukePlayer_t *const p, int32_t weapid)
{
    if (weapid == HANDBOMB_WEAPON && p->ammo_amount[HANDBOMB_WEAPON] == 0)
    {
        int32_t k = headspritestat[STAT_ACTOR];
        while (k >= 0)
        {
            if (sprite[k].picnum == HEAVYHBOMB && sprite[k].owner == p->i)
                return 1;

            k = nextspritestat[k];
        }
    }

    return 0;
}

void P_HandleSharedKeys(int32_t snum)
{
    int32_t i, k = 0, dainv;
    uint32_t sb_snum = g_player[snum].sync->bits, j;
    DukePlayer_t *const p = g_player[snum].ps;

    if (p->cheat_phase == 1) return;

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

    i = p->aim_mode;
    p->aim_mode = (sb_snum>>SK_AIMMODE)&1;
    if (p->aim_mode < i)
        p->return_to_center = 9;

    if (TEST_SYNC_KEY(sb_snum, SK_QUICK_KICK) && p->quick_kick == 0)
        if (PWEAPON(snum, p->curr_weapon, WorksLike) != KNEE_WEAPON || p->kickback_pic == 0)
        {
            if (VM_OnEvent(EVENT_QUICKKICK,g_player[snum].ps->i,snum) == 0)
            {
                p->quick_kick = 14;
                if (p->fta == 0 || p->ftq == 80)
                    P_DoQuote(QUOTE_MIGHTY_FOOT,p);
            }
        }

    j = sb_snum & ((15u<<SK_WEAPON_BITS)|BIT(SK_STEROIDS)|BIT(SK_NIGHTVISION)|BIT(SK_MEDKIT)|BIT(SK_QUICK_KICK)| \
                   BIT(SK_HOLSTER)|BIT(SK_INV_LEFT)|BIT(SK_PAUSE)|BIT(SK_HOLODUKE)|BIT(SK_JETPACK)|BIT(SK_INV_RIGHT)| \
                   BIT(SK_TURNAROUND)|BIT(SK_OPEN)|BIT(SK_INVENTORY)|BIT(SK_ESCAPE));
    sb_snum = j & ~p->interface_toggle_flag;
    p->interface_toggle_flag |= sb_snum | ((sb_snum&0xf00)?0xf00:0);
    p->interface_toggle_flag &= j | ((j&0xf00)?0xf00:0);

    if (sb_snum && TEST_SYNC_KEY(sb_snum, SK_MULTIFLAG) == 0)
    {
        if (TEST_SYNC_KEY(sb_snum, SK_PAUSE))
        {
            KB_ClearKeyDown(sc_Pause);
            if (ud.pause_on)
                ud.pause_on = 0;
            else ud.pause_on = 1+SHIFTS_IS_PRESSED;
            if (ud.pause_on)
            {
                S_PauseMusic(1);
                FX_StopAllSounds();
                S_ClearSoundLocks();
            }
            else
            {
                if (ud.config.MusicToggle) S_PauseMusic(0);
                pub = NUMPAGES;
                pus = NUMPAGES;
            }
        }

        if (ud.pause_on) return;

        if (sprite[p->i].extra <= 0) return;		// if dead...

        if (TEST_SYNC_KEY(sb_snum, SK_INVENTORY) && p->newowner == -1)	// inventory button generates event for selected item
        {
            if (VM_OnEvent(EVENT_INVENTORY,g_player[snum].ps->i,snum) == 0)
            {
                switch (p->inven_icon)
                {
                case ICON_JETPACK:
                    sb_snum |= BIT(SK_JETPACK);
                    break;
                case ICON_HOLODUKE:
                    sb_snum |= BIT(SK_HOLODUKE);
                    break;
                case ICON_HEATS:
                    sb_snum |= BIT(SK_NIGHTVISION);
                    break;
                case ICON_FIRSTAID:
                    sb_snum |= BIT(SK_MEDKIT);
                    break;
                case ICON_STEROIDS:
                    sb_snum |= BIT(SK_STEROIDS);
                    break;
                }
            }
        }

        if (TEST_SYNC_KEY(sb_snum, SK_NIGHTVISION))
        {
            if (VM_OnEvent(EVENT_USENIGHTVISION,g_player[snum].ps->i,snum) == 0
                    &&  p->inv_amount[GET_HEATS] > 0)
            {
                p->heat_on = !p->heat_on;
                P_UpdateScreenPal(p);
                p->inven_icon = ICON_HEATS;
                A_PlaySound(NITEVISION_ONOFF,p->i);
                P_DoQuote(QUOTE_NVG_OFF-!!p->heat_on,p);
            }
        }

        if (TEST_SYNC_KEY(sb_snum, SK_STEROIDS))
        {
            if (VM_OnEvent(EVENT_USESTEROIDS,g_player[snum].ps->i,snum) == 0)
            {
                if (p->inv_amount[GET_STEROIDS] == 400)
                {
                    p->inv_amount[GET_STEROIDS]--;
                    A_PlaySound(DUKE_TAKEPILLS,p->i);
                    P_DoQuote(QUOTE_USED_STEROIDS,p);
                }
                if (p->inv_amount[GET_STEROIDS] > 0)
                    p->inven_icon = ICON_STEROIDS;
            }
            return;		// is there significance to returning?
        }
        if (p->refresh_inventory)
            sb_snum |= BIT(SK_INV_LEFT);   // emulate move left...

        if (p->newowner == -1 && (TEST_SYNC_KEY(sb_snum, SK_INV_LEFT) || TEST_SYNC_KEY(sb_snum, SK_INV_RIGHT)))
        {
            p->invdisptime = GAMETICSPERSEC*2;

            if (TEST_SYNC_KEY(sb_snum, SK_INV_RIGHT)) k = 1;
            else k = 0;

            if (p->refresh_inventory) p->refresh_inventory = 0;
            dainv = p->inven_icon;

            i = 0;

CHECKINV1:
            if (i < 9)
            {
                i++;

                switch (dainv)
                {
                case ICON_JETPACK:
                    if (p->inv_amount[GET_JETPACK] > 0 && i > 1)
                        break;
                    if (k) dainv++;
                    else dainv--;
                    goto CHECKINV1;
                case ICON_SCUBA:
                    if (p->inv_amount[GET_SCUBA] > 0 && i > 1)
                        break;
                    if (k) dainv++;
                    else dainv--;
                    goto CHECKINV1;
                case ICON_STEROIDS:
                    if (p->inv_amount[GET_STEROIDS] > 0 && i > 1)
                        break;
                    if (k) dainv++;
                    else dainv--;
                    goto CHECKINV1;
                case ICON_HOLODUKE:
                    if (p->inv_amount[GET_HOLODUKE] > 0 && i > 1)
                        break;
                    if (k) dainv++;
                    else dainv--;
                    goto CHECKINV1;
                case ICON_NONE:
                case ICON_FIRSTAID:
                    if (p->inv_amount[GET_FIRSTAID] > 0 && i > 1)
                        break;
                    if (k) dainv = 2;
                    else dainv = 7;
                    goto CHECKINV1;
                case ICON_HEATS:
                    if (p->inv_amount[GET_HEATS] > 0 && i > 1)
                        break;
                    if (k) dainv++;
                    else dainv--;
                    goto CHECKINV1;
                case ICON_BOOTS:
                    if (p->inv_amount[GET_BOOTS] > 0 && i > 1)
                        break;
                    if (k) dainv = 1;
                    else dainv = 6;
                    goto CHECKINV1;
                }
            }
            else dainv = 0;

            if (TEST_SYNC_KEY(sb_snum, SK_INV_LEFT))   // Inventory_Left
            {
                /*Gv_SetVar(g_iReturnVarID,dainv,g_player[snum].ps->i,snum);*/
                dainv = VM_OnEventWithReturn(EVENT_INVENTORYLEFT,g_player[snum].ps->i,snum, dainv);
            }
            else if (TEST_SYNC_KEY(sb_snum, SK_INV_RIGHT))   // Inventory_Right
            {
                /*Gv_SetVar(g_iReturnVarID,dainv,g_player[snum].ps->i,snum);*/
                dainv = VM_OnEventWithReturn(EVENT_INVENTORYRIGHT,g_player[snum].ps->i,snum, dainv);
            }

            if (dainv >= 1)
            {
                p->inven_icon = dainv;

                if (dainv || p->inv_amount[GET_FIRSTAID])
                {
                    static const int32_t i[8] = { QUOTE_MEDKIT, QUOTE_STEROIDS, QUOTE_HOLODUKE,
                        QUOTE_JETPACK, QUOTE_NVG, QUOTE_SCUBA, QUOTE_BOOTS, 0 };
                    if (dainv>=1 && dainv<=9)
                        P_DoQuote(i[dainv-1], p);
                }
            }
        }

        j = ((sb_snum&(15<<SK_WEAPON_BITS))>>SK_WEAPON_BITS) - 1;

        switch ((int32_t)j)
        {
        case -1:
            break;
        default:
            j = VM_OnEventWithReturn(EVENT_WEAPKEY1+j,p->i,snum, j);
            break;
        case 10:
            j = VM_OnEventWithReturn(EVENT_PREVIOUSWEAPON,p->i,snum, j);
            break;
        case 11:
            j = VM_OnEventWithReturn(EVENT_NEXTWEAPON,p->i,snum, j);
            break;
        }

        // NOTE: it is assumed that the above events return either -1 or a
        // valid weapon index. Presumably, neither other negative numbers nor
        // positive ones >= MAX_WEAPONS are allowed. However, the code below is
        // a bit inconsistent in checking "j".

        if (p->reloading == 1)
            j = -1;
        else if ((uint32_t)j < MAX_WEAPONS && p->kickback_pic == 1 && p->weapon_pos == 1)
        {
            p->wantweaponfire = j;
            p->kickback_pic = 0;
        }

        if ((int32_t)j != -1 && p->last_pissed_time <= (GAMETICSPERSEC * 218) && p->show_empty_weapon == 0 &&
            p->kickback_pic == 0 && p->quick_kick == 0 && sprite[p->i].xrepeat > 32 && p->access_incs == 0 &&
            p->knee_incs == 0)
        {
            //            if(  ( p->weapon_pos == 0 || ( p->holster_weapon && p->weapon_pos == WEAPON_POS_LOWER ) ))
            {
                if (j == 10 || j == 11)
                {
                    k = p->curr_weapon;
                    j = (j == 10 ? -1 : 1);     // JBF: prev (-1) or next (1) weapon choice
                    i = k;

                    while ((k >= 0 && k < 11) || (PLUTOPAK && k == GROW_WEAPON))
                    {
                        // this accounts for the expander when handling next/previous

                        switch (k)
                        {
                            case DEVISTATOR_WEAPON:
                                if ((int32_t) j == -1)
                                {
                                    if (PLUTOPAK)
                                        k = GROW_WEAPON;
                                    else
                                        k--;
                                }
                                else
                                    k++;
                                break;

                            case GROW_WEAPON:
                                if ((int32_t)j == -1)
                                    k = SHRINKER_WEAPON;
                                else
                                    k = DEVISTATOR_WEAPON;
                                break;

                            case SHRINKER_WEAPON:
                                if ((int32_t)j == 1)
                                {
                                    if (PLUTOPAK)
                                        k = GROW_WEAPON;
                                    else
                                        k++;
                                }
                                else
                                    k--;
                                break;

                            case HANDREMOTE_WEAPON:
                                i = k = HANDBOMB_WEAPON;
                            default:
                                k += j;
                                break;
                        }

                        if (k == -1) k = FREEZE_WEAPON;
                        else if (k == 10) k = KNEE_WEAPON;

                        if (((p->gotweapon & (1<<k)) && p->ammo_amount[k] > 0) || P_CheckDetonatorSpecialCase(p, k))
                        {
                            j = k;
                            break;
                        }

                        if (i == k) // absolutely no weapons, so use foot
                        {
                            j = KNEE_WEAPON;
                            break;
                        }
                    }

                    if (j == SHRINKER_WEAPON)
                        p->subweapon &= ~(1 << GROW_WEAPON);
                    else if (j == GROW_WEAPON)
                        p->subweapon |= (1<<GROW_WEAPON);
                }

                P_SetWeaponGamevars(snum, p);

                j = VM_OnEventWithReturn(EVENT_SELECTWEAPON,p->i,snum, j);

                // XXX: any signifcance to "<= MAX_WEAPONS" instead of "<"?
                if ((int32_t)j != -1 && j <= MAX_WEAPONS)
                {
                    if (P_CheckDetonatorSpecialCase(p, j))
                    {
                        p->gotweapon |= (1<<HANDBOMB_WEAPON);
                        j = HANDREMOTE_WEAPON;
                    }

                    if (j == SHRINKER_WEAPON && PLUTOPAK)   // JBF 20040116: so we don't select the grower with v1.3d
                    {
                        if (screenpeek == snum) pus = NUMPAGES;

                        if (p->curr_weapon != GROW_WEAPON && p->curr_weapon != SHRINKER_WEAPON)
                        {
                            if (p->ammo_amount[GROW_WEAPON] > 0)
                            {
                                if ((p->subweapon&(1<<GROW_WEAPON)) == (1<<GROW_WEAPON))
                                    j = GROW_WEAPON;
                                else if (p->ammo_amount[SHRINKER_WEAPON] == 0)
                                {
                                    j = GROW_WEAPON;
                                    p->subweapon |= (1<<GROW_WEAPON);
                                }
                            }
                            else if (p->ammo_amount[SHRINKER_WEAPON] > 0)
                                p->subweapon &= ~(1<<GROW_WEAPON);
                        }
                        else if (p->curr_weapon == SHRINKER_WEAPON)
                        {
                            p->subweapon |= (1<<GROW_WEAPON);
                            j = GROW_WEAPON;
                        }
                        else
                            p->subweapon &= ~(1<<GROW_WEAPON);
                    }

                    if (p->holster_weapon)
                    {
                        sb_snum |= BIT(SK_HOLSTER);
                        p->weapon_pos = WEAPON_POS_LOWER;
                    }
                    else if ((uint32_t)j < MAX_WEAPONS && (p->gotweapon & (1<<j)) && (uint32_t)p->curr_weapon != j)
                        switch (j)
                        {
                        case PISTOL_WEAPON:
                        case SHOTGUN_WEAPON:
                        case CHAINGUN_WEAPON:
                        case RPG_WEAPON:
                        case DEVISTATOR_WEAPON:
                        case FREEZE_WEAPON:
                        case GROW_WEAPON:
                        case SHRINKER_WEAPON:
                            if (p->ammo_amount[j] == 0 && p->show_empty_weapon == 0)
                            {
                                p->last_full_weapon = p->curr_weapon;
                                p->show_empty_weapon = 32;
                            }
                        case KNEE_WEAPON:
                        case HANDREMOTE_WEAPON:
                            P_AddWeapon(p, j, 1);
                            break;
                        case HANDBOMB_WEAPON:
                        case TRIPBOMB_WEAPON:
                            if (p->ammo_amount[j] > 0 && (p->gotweapon & (1<<j)))
                                P_AddWeapon(p, j, 1);
                            break;
                        }
                }
            }
        }

        if (TEST_SYNC_KEY(sb_snum, SK_HOLODUKE) && p->newowner == -1)
        {
            if (p->holoduke_on == -1)
            {
                if (VM_OnEvent(EVENT_HOLODUKEON,g_player[snum].ps->i,snum) == 0)
                {
                    if (p->inv_amount[GET_HOLODUKE] > 0)
                    {
                        p->inven_icon = ICON_HOLODUKE;

                        if (p->cursectnum > -1)
                        {
                            p->holoduke_on = i = A_InsertSprite(p->cursectnum,p->pos.x,p->pos.y,
                                                                p->pos.z+(30<<8),APLAYER,-64,0,0,p->ang,0,0,-1,10);
                            T4 = T5 = 0;
                            sprite[i].yvel = snum;
                            sprite[i].extra = 0;
                            P_DoQuote(QUOTE_HOLODUKE_ON,p);
                            A_PlaySound(TELEPORTER,p->holoduke_on);
                        }
                    }
                    else P_DoQuote(QUOTE_HOLODUKE_NOT_FOUND,p);
                }
            }
            else
            {
                if (VM_OnEvent(EVENT_HOLODUKEOFF,g_player[snum].ps->i,snum) == 0)
                {
                    A_PlaySound(TELEPORTER,p->holoduke_on);
                    p->holoduke_on = -1;
                    P_DoQuote(QUOTE_HOLODUKE_OFF,p);
                }
            }
        }

        if (TEST_SYNC_KEY(sb_snum, SK_MEDKIT))
        {
            if (VM_OnEvent(EVENT_USEMEDKIT,g_player[snum].ps->i,snum) == 0)
            {
                if (p->inv_amount[GET_FIRSTAID] > 0 && sprite[p->i].extra < p->max_player_health)
                {
                    j = p->max_player_health-sprite[p->i].extra;

                    if ((uint32_t)p->inv_amount[GET_FIRSTAID] > j)
                    {
                        p->inv_amount[GET_FIRSTAID] -= j;
                        sprite[p->i].extra = p->max_player_health;
                        p->inven_icon = ICON_FIRSTAID;
                    }
                    else
                    {
                        sprite[p->i].extra += p->inv_amount[GET_FIRSTAID];
                        p->inv_amount[GET_FIRSTAID] = 0;
                        P_SelectNextInvItem(p);
                    }
                    A_PlaySound(DUKE_USEMEDKIT,p->i);
                }
            }
        }

        if (p->newowner == -1 && TEST_SYNC_KEY(sb_snum, SK_JETPACK))
        {
            if (VM_OnEvent(EVENT_USEJETPACK,g_player[snum].ps->i,snum) == 0)
            {
                if (p->inv_amount[GET_JETPACK] > 0)
                {
                    p->jetpack_on = !p->jetpack_on;
                    if (p->jetpack_on)
                    {
                        p->inven_icon = ICON_JETPACK;
                        if (p->scream_voice > FX_Ok)
                        {
                            FX_StopSound(p->scream_voice);
                            p->scream_voice = -1;
                        }

                        A_PlaySound(DUKE_JETPACK_ON,p->i);

                        P_DoQuote(QUOTE_JETPACK_ON,p);
                    }
                    else
                    {
                        p->hard_landing = 0;
                        p->vel.z = 0;
                        A_PlaySound(DUKE_JETPACK_OFF,p->i);
                        S_StopEnvSound(DUKE_JETPACK_IDLE,p->i);
                        S_StopEnvSound(DUKE_JETPACK_ON,p->i);
                        P_DoQuote(QUOTE_JETPACK_OFF,p);
                    }
                }
                else P_DoQuote(QUOTE_JETPACK_NOT_FOUND,p);
            }
        }

        if (TEST_SYNC_KEY(sb_snum, SK_TURNAROUND) && p->one_eighty_count == 0)
            if (VM_OnEvent(EVENT_TURNAROUND,p->i,snum) == 0)
                p->one_eighty_count = -1024;
    }
}

int32_t A_CheckHitSprite(int32_t i, int16_t *hitsp)
{
    hitdata_t hit;
    int32_t zoff = 0;

    if (A_CheckEnemySprite(&sprite[i]))
        zoff = (42<<8);
    else if (PN == APLAYER)
        zoff = (39<<8);

    SZ -= zoff;
    hitscan((const vec3_t *)&sprite[i],SECT,
            sintable[(SA+512)&2047],
            sintable[SA&2047],
            0,&hit,CLIPMASK1);
    SZ += zoff;

    if (hitsp)
        *hitsp = hit.sprite;

    if (hit.wall >= 0 && (wall[hit.wall].cstat&16) && A_CheckEnemySprite(&sprite[i]))
        return 1<<30;

    return FindDistance2D(hit.pos.x-SX,hit.pos.y-SY);
}

static int32_t P_FindWall(DukePlayer_t *p, int16_t *hitw)
{
    hitdata_t hit;

    hitscan((const vec3_t *)p,p->cursectnum,
            sintable[(p->ang+512)&2047],
            sintable[p->ang&2047],
            0,&hit,CLIPMASK0);

    *hitw = hit.wall;
    if (hit.wall < 0)
        return INT32_MAX;

    return FindDistance2D(hit.pos.x-p->pos.x,hit.pos.y-p->pos.y);
}

// returns 1 if sprite i should not be considered by neartag
static int32_t our_neartag_blacklist(int32_t i)
{
    return sprite[i].picnum >= SECTOREFFECTOR__STATIC && sprite[i].picnum <= GPSPEED__STATIC;
}

static void G_ClearCameras(DukePlayer_t *p)
{
    G_ClearCameraView(p);

    if (I_EscapeTrigger())
        I_EscapeTriggerClear();
}

void P_CheckSectors(int32_t snum)
{
    int32_t i = -1;
    DukePlayer_t *const p = g_player[snum].ps;

    if (p->cursectnum > -1)
        switch (sector[p->cursectnum].lotag)
        {
        case 32767:
            sector[p->cursectnum].lotag = 0;
            P_DoQuote(QUOTE_FOUND_SECRET,p);
            p->secret_rooms++;
            return;

        case UINT16_MAX:
            for (TRAVERSE_CONNECT(i))
                g_player[i].ps->gm = MODE_EOL;
            sector[p->cursectnum].lotag = 0;
            if (ud.from_bonus)
            {
                ud.level_number = ud.from_bonus;
                ud.m_level_number = ud.level_number;
                ud.from_bonus = 0;
            }
            else
            {
                ud.level_number++;
                if (ud.level_number > MAXLEVELS-1)
                    ud.level_number = 0;
                ud.m_level_number = ud.level_number;
            }
            return;

        case UINT16_MAX-1:
            sector[p->cursectnum].lotag = 0;
            p->timebeforeexit = GAMETICSPERSEC*8;
            p->customexitsound = sector[p->cursectnum].hitag;
            return;

        default:
            if (sector[p->cursectnum].lotag >= 10000 && sector[p->cursectnum].lotag < 16383)
            {
                if (snum == screenpeek || (GametypeFlags[ud.coop]&GAMETYPE_COOPSOUND))
                    A_PlaySound(sector[p->cursectnum].lotag-10000,p->i);
                sector[p->cursectnum].lotag = 0;
            }
            break;
        }

    //After this point the the player effects the map with space

    if (p->gm &MODE_TYPE || sprite[p->i].extra <= 0)
        return;

    if (TEST_SYNC_KEY(g_player[snum].sync->bits, SK_OPEN))
    {
        if (VM_OnEvent(EVENT_USE, p->i, snum) != 0)
            g_player[snum].sync->bits &= ~BIT(SK_OPEN);
    }

    if (ud.cashman && TEST_SYNC_KEY(g_player[snum].sync->bits, SK_OPEN))
        A_SpawnMultiple(p->i, MONEY, 2);

    if (p->newowner >= 0)
    {
        if (klabs(g_player[snum].sync->svel) > 768 || klabs(g_player[snum].sync->fvel) > 768)
        {
            G_ClearCameras(p);
            return;
        }
    }

    if (!TEST_SYNC_KEY(g_player[snum].sync->bits, SK_OPEN) && !TEST_SYNC_KEY(g_player[snum].sync->bits, SK_ESCAPE))
        p->toggle_key_flag = 0;
    else if (!p->toggle_key_flag)
    {
        int16_t hitscanwall;

        int16_t neartagsector, neartagwall, neartagsprite;
        int32_t neartaghitdist;

        if (TEST_SYNC_KEY(g_player[snum].sync->bits, SK_ESCAPE))
        {
            if (p->newowner >= 0)
                G_ClearCameras(p);
            return;
        }

        neartagsprite = -1;
        p->toggle_key_flag = 1;
        hitscanwall = -1;

        i = P_FindWall(p,&hitscanwall);

        if (hitscanwall >= 0 && i < 1280 && wall[hitscanwall].overpicnum == MIRROR)
            if (wall[hitscanwall].lotag > 0 && !A_CheckSoundPlaying(p->i,wall[hitscanwall].lotag) && snum == screenpeek)
            {
                A_PlaySound(wall[hitscanwall].lotag,p->i);
                return;
            }

        if (hitscanwall >= 0 && (wall[hitscanwall].cstat&16))
            if (wall[hitscanwall].lotag)
                return;

        if (p->newowner >= 0)
            neartag(p->opos.x,p->opos.y,p->opos.z,sprite[p->i].sectnum,p->oang,&neartagsector,
                    &neartagwall,&neartagsprite,&neartaghitdist, 1280, 1, our_neartag_blacklist);
        else
        {
            neartag(p->pos.x,p->pos.y,p->pos.z,sprite[p->i].sectnum,p->oang,&neartagsector,
                    &neartagwall,&neartagsprite,&neartaghitdist, 1280, 1, our_neartag_blacklist);
            if (neartagsprite == -1 && neartagwall == -1 && neartagsector == -1)
                neartag(p->pos.x,p->pos.y,p->pos.z+(8<<8),sprite[p->i].sectnum,p->oang,&neartagsector,
                        &neartagwall,&neartagsprite,&neartaghitdist, 1280, 1, our_neartag_blacklist);
            if (neartagsprite == -1 && neartagwall == -1 && neartagsector == -1)
                neartag(p->pos.x,p->pos.y,p->pos.z+(16<<8),sprite[p->i].sectnum,p->oang,&neartagsector,
                        &neartagwall,&neartagsprite,&neartaghitdist, 1280, 1, our_neartag_blacklist);
            if (neartagsprite == -1 && neartagwall == -1 && neartagsector == -1)
            {
                neartag(p->pos.x,p->pos.y,p->pos.z+(16<<8),sprite[p->i].sectnum,p->oang,&neartagsector,
                        &neartagwall,&neartagsprite,&neartaghitdist, 1280, 3, our_neartag_blacklist);
                if (neartagsprite >= 0)
                {
                    switch (DYNAMICTILEMAP(sprite[neartagsprite].picnum))
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
                    case TOUGHGAL__STATIC:
                        return;
                    }
                }

                neartagsprite = -1;
                neartagwall = -1;
                neartagsector = -1;
            }
        }

        if (p->newowner == -1 && neartagsprite == -1 && neartagsector == -1 && neartagwall == -1)
            if (isanunderoperator(sector[sprite[p->i].sectnum].lotag))
                neartagsector = sprite[p->i].sectnum;

        if (neartagsector >= 0 && (sector[neartagsector].lotag&16384))
            return;

        if (neartagsprite == -1 && neartagwall == -1)
            if (p->cursectnum >= 0 && sector[p->cursectnum].lotag == 2)
            {
                int32_t oldz = A_CheckHitSprite(p->i,&neartagsprite);
                if (oldz > 1280)
                    neartagsprite = -1;
            }

        if (neartagsprite >= 0)
        {
            if (P_ActivateSwitch(snum, neartagsprite, 1))
                return;

            switch (DYNAMICTILEMAP(sprite[neartagsprite].picnum))
            {
            case TOILET__STATIC:
            case STALL__STATIC:
                if (p->last_pissed_time == 0)
                {
                    if (ud.lockout == 0) A_PlaySound(DUKE_URINATE,p->i);

                    p->last_pissed_time = GAMETICSPERSEC*220;
                    p->transporter_hold = 29*2;
                    if (p->holster_weapon == 0)
                    {
                        p->holster_weapon = 1;
                        p->weapon_pos = -1;
                    }
                    if (sprite[p->i].extra <= (p->max_player_health-(p->max_player_health/10)))
                    {
                        sprite[p->i].extra += p->max_player_health/10;
                        p->last_extra = sprite[p->i].extra;
                    }
                    else if (sprite[p->i].extra < p->max_player_health)
                        sprite[p->i].extra = p->max_player_health;
                }
                else if (!A_CheckSoundPlaying(neartagsprite,FLUSH_TOILET))
                    A_PlaySound(FLUSH_TOILET,neartagsprite);
                return;

            case NUKEBUTTON__STATIC:
            {
                int16_t j;

                P_FindWall(p, &j);

                if (j >= 0 && wall[j].overpicnum == 0)
                    if (actor[neartagsprite].t_data[0] == 0)
                    {
                        if (ud.noexits && (g_netServer || ud.multimode > 1))
                        {
                            // NUKEBUTTON frags the player
                            actor[p->i].picnum = NUKEBUTTON;
                            actor[p->i].extra = 250;
                        }
                        else
                        {
                            actor[neartagsprite].t_data[0] = 1;
                            sprite[neartagsprite].owner = p->i;
                            ud.secretlevel =
                                (p->buttonpalette = sprite[neartagsprite].pal) ? sprite[neartagsprite].lotag : 0;
                        }
                    }
                return;
            }

            case WATERFOUNTAIN__STATIC:
                if (actor[neartagsprite].t_data[0] != 1)
                {
                    actor[neartagsprite].t_data[0] = 1;
                    sprite[neartagsprite].owner = p->i;

                    if (sprite[p->i].extra < p->max_player_health)
                    {
                        sprite[p->i].extra++;
                        A_PlaySound(DUKE_DRINKING,p->i);
                    }
                }
                return;

            case PLUG__STATIC:
                A_PlaySound(SHORT_CIRCUIT,p->i);
                sprite[p->i].extra -= 2+(krand()&3);

                P_PalFrom(p, 32, 48,48,64);
                break;

            case VIEWSCREEN__STATIC:
            case VIEWSCREEN2__STATIC:
                // Try to find a camera sprite for the viewscreen.
                for (SPRITES_OF(STAT_ACTOR, i))
                {
                    if (PN == CAMERA1 && SP == 0 && sprite[neartagsprite].hitag == SLT)
                    {
                        sprite[i].yvel = 1;  // Using this camera
                        A_PlaySound(MONITOR_ACTIVE,p->i);

                        sprite[neartagsprite].owner = i;
                        sprite[neartagsprite].yvel = 1;  // VIEWSCREEN_YVEL
                        g_curViewscreen = neartagsprite;

                        {
                            int32_t j = p->cursectnum;
                            p->cursectnum = SECT;
                            P_UpdateScreenPal(p);
                            p->cursectnum = j;
                        }

                        // parallaxtype = 2;
                        p->newowner = i;

                        P_UpdatePosWhenViewingCam(p);

                        return;
                    }
                }

                G_ClearCameras(p);
                return;
            }  // switch
        }

        if (TEST_SYNC_KEY(g_player[snum].sync->bits, SK_OPEN) == 0)
            return;

        if (p->newowner >= 0)
        {
            G_ClearCameras(p);
            return;
        }

        if (neartagwall == -1 && neartagsector == -1 && neartagsprite == -1)
            if (klabs(A_GetHitscanRange(p->i)) < 512)
            {
                if ((krand()&255) < 16)
                    A_PlaySound(DUKE_SEARCH2,p->i);
                else A_PlaySound(DUKE_SEARCH,p->i);
                return;
            }

        if (neartagwall >= 0)
        {
            if (wall[neartagwall].lotag > 0 && CheckDoorTile(wall[neartagwall].picnum))
            {
                if (hitscanwall == neartagwall || hitscanwall == -1)
                    P_ActivateSwitch(snum,neartagwall,0);
                return;
            }
            else if (p->newowner >= 0)
            {
                G_ClearCameras(p);
                return;
            }
        }

        if (neartagsector >= 0 && (sector[neartagsector].lotag&16384) == 0 &&
                isanearoperator(sector[neartagsector].lotag))
        {
            for (SPRITES_OF_SECT(neartagsector, i))
            {
                if (PN == ACTIVATOR || PN == MASTERSWITCH)
                    return;
            }

            G_OperateSectors(neartagsector,p->i);
        }
        else if ((sector[sprite[p->i].sectnum].lotag&16384) == 0)
        {
            if (isanunderoperator(sector[sprite[p->i].sectnum].lotag))
            {
                for (SPRITES_OF_SECT(sprite[p->i].sectnum, i))
                {
                    if (PN == ACTIVATOR || PN == MASTERSWITCH)
                        return;
                }

                G_OperateSectors(sprite[p->i].sectnum,p->i);
            }
            else P_ActivateSwitch(snum,neartagwall,0);
        }
    }
}

