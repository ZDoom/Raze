//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

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

#define actors_c_

#include "global.h"

BEGIN_DUKE_NS

#if KRANDDEBUG
# define ACTOR_STATIC
#else
# define ACTOR_STATIC static
#endif

#define DELETE_SPRITE_AND_CONTINUE(KX) do { A_DeleteSprite(KX); goto next_sprite; } while (0)

void G_ClearCameraView(DukePlayer_t *ps)
{
    ps->newowner = -1;
    ps->pos = ps->opos;
    ps->q16ang = ps->oq16ang;

    updatesector(ps->pos.x, ps->pos.y, &ps->cursectnum);
    P_UpdateScreenPal(ps);

    for (bssize_t SPRITES_OF(STAT_ACTOR, k))
        if (sprite[k].picnum==TILE_CAMERA1)
            sprite[k].yvel = 0;
}

// deletesprite() game wrapper
void A_DeleteSprite(int spriteNum)
{
#ifdef POLYMER
    if (actor[spriteNum].lightptr != NULL && videoGetRenderMode() == REND_POLYMER)
        A_DeleteLight(spriteNum);
#endif

    // AMBIENT_SFX_PLAYING
    if (sprite[spriteNum].picnum == TILE_MUSICANDSFX && actor[spriteNum].t_data[0] == 1)
        S_StopEnvSound(sprite[spriteNum].lotag, spriteNum);

    deletesprite(spriteNum);
}

void insertspriteq(int i);

static int32_t G_ToggleWallInterpolation(int32_t wallNum, int32_t setInterpolation)
{
    if (setInterpolation)
    {
        return G_SetInterpolation(&wall[wallNum].x) || G_SetInterpolation(&wall[wallNum].y);
    }
    else
    {
        G_StopInterpolation(&wall[wallNum].x);
        G_StopInterpolation(&wall[wallNum].y);
        return 0;
    }
}

void Sect_ToggleInterpolation(int sectNum, int setInterpolation)
{
    for (bssize_t j = sector[sectNum].wallptr, endwall = sector[sectNum].wallptr + sector[sectNum].wallnum; j < endwall; j++)
    {
        G_ToggleWallInterpolation(j, setInterpolation);

        int const nextWall = wall[j].nextwall;

        if (nextWall >= 0)
        {
            G_ToggleWallInterpolation(nextWall, setInterpolation);
            G_ToggleWallInterpolation(wall[nextWall].point2, setInterpolation);
        }
    }
}

int g_canSeePlayer = 0;

int G_WakeUp(spritetype *const pSprite, int const playerNum)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;
    if (!pPlayer->make_noise)
        return 0;
    int const radius = pPlayer->noise_radius;

    if (pSprite->pal == 30 || pSprite->pal == 32 || pSprite->pal == 33 || (RRRA && pSprite->pal == 8))
        return 0;

    return (pPlayer->noise_x - radius < pSprite->x && pPlayer->noise_x + radius > pSprite->x
        && pPlayer->noise_y - radius < pSprite->y && pPlayer->noise_y + radius > pSprite->y);
}


// sleeping monsters, etc

 int A_FindLocator(int const tag, int const sectNum)
{
    for (bssize_t SPRITES_OF(STAT_LOCATOR, spriteNum))
    {
        if ((sectNum == -1 || sectNum == SECT(spriteNum)) && tag == SLT(spriteNum))
            return spriteNum;
    }

    return -1;
}


TileInfo tileinfo[MAXTILES];


void movefta_d(void);
void movefallers_d();
void movestandables_d();
void moveweapons_d();
void movetransports_d(void);
void moveactors_d();
void moveexplosions_d();
void moveeffectors_d();

void movefta_r(void);
void moveplayers();
void movefx();
void movefallers_r();
void movestandables_r();
void moveweapons_r();
void movetransports_r(void);
void moveactors_r();
void thunder();
void moveexplosions_r();
void moveeffectors_r();

void doanimations(void);

void G_MoveWorld_d(void)
{
    extern double g_moveActorsTime, g_moveWorldTime;
    const double worldTime = timerGetHiTicks();

    movefta_d();     //ST 2
    moveweapons_d();          //ST 4
    movetransports_d();       //ST 9

    moveplayers();          //ST 10
    movefallers_d();          //ST 12
    moveexplosions_d();             //ST 5

    const double actorsTime = timerGetHiTicks();

    moveactors_d();           //ST 1

    g_moveActorsTime = (1-0.033)*g_moveActorsTime + 0.033*(timerGetHiTicks()-actorsTime);

    // XXX: Has to be before effectors, in particular movers?
    // TODO: lights in moving sectors ought to be interpolated
    //G_DoEffectorLights();
    moveeffectors_d();        //ST 3
    movestandables_d();       //ST 6

    //G_RefreshLights();
    doanimations();
    movefx();               //ST 11

    g_moveWorldTime = (1-0.033)*g_moveWorldTime + 0.033*(timerGetHiTicks()-worldTime);
}

void G_MoveWorld_r(void)
{
    extern double g_moveActorsTime, g_moveWorldTime;
    const double worldTime = timerGetHiTicks();

    if (!DEER)
    {
        movefta_r();     //ST 2
        moveweapons_r();          //ST 4
        movetransports_r();       //ST 9
    }

    moveplayers();          //ST 10
    movefallers_r();          //ST 12
    if (!DEER)
        moveexplosions_r();             //ST 5

    const double actorsTime = timerGetHiTicks();

    moveactors_r();           //ST 1

    g_moveActorsTime = (1 - 0.033) * g_moveActorsTime + 0.033 * (timerGetHiTicks() - actorsTime);

    // XXX: Has to be before effectors, in particular movers?
    // TODO: lights in moving sectors ought to be interpolated
    // G_DoEffectorLights();
    if (!DEER)
    {
        moveeffectors_r();        //ST 3
        movestandables_r();       //ST 6
    }

    //G_RefreshLights();
    doanimations();
    if (!DEER)
        movefx();               //ST 11

    if (numplayers < 2 && g_thunderOn)
        thunder();

    g_moveWorldTime = (1 - 0.033) * g_moveWorldTime + 0.033 * (timerGetHiTicks() - worldTime);
}

void G_MoveWorld(void)
{
    if (!isRR()) G_MoveWorld_d();
    else G_MoveWorld_r();
}

END_DUKE_NS

