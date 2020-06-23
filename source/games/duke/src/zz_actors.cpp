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
    // AMBIENT_SFX_PLAYING
    if (sprite[spriteNum].picnum == MUSICANDSFX && actor[spriteNum].t_data[0] == 1)
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


TileInfo tileinfo[MAXTILES];

END_DUKE_NS

