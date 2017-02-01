//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "build.h"
#include "common.h"

#include "names2.h"
#include "game.h"
#include "tags.h"
#include "common_game.h"
#include "break.h"
#include "quake.h"
#include "sprite.h"

#define QUAKE_Match(sp) (SP_TAG2(sp))
#define QUAKE_Zamt(sp) (SP_TAG3(sp))
#define QUAKE_Radius(sp) (SP_TAG4(sp))
#define QUAKE_Duration(sp) (SP_TAG5(sp))
#define QUAKE_WaitSecs(sp) (SP_TAG6(sp))
#define QUAKE_AngAmt(sp) (SP_TAG7(sp))
#define QUAKE_PosAmt(sp) (SP_TAG8(sp))
#define QUAKE_RandomTest(sp) (SP_TAG9(sp))
#define QUAKE_WaitTics(sp) (SP_TAG13(sp))

#define QUAKE_TestDontTaper(sp) (TEST_BOOL1(sp))
#define QUAKE_KillAfterQuake(sp) (TEST_BOOL2(sp))
// only for timed quakes
#define QUAKE_WaitForTrigger(sp) (TEST_BOOL3(sp))

extern SWBOOL GamePaused;

short CopyQuakeSpotToOn(SPRITEp sp)
{
    short New;
    SPRITEp np;

    New = COVERinsertsprite(sp->sectnum, STAT_QUAKE_SPOT);
    np = &sprite[New];

    memcpy(np, sp, sizeof(SPRITE));

    np->sectnum = sp->sectnum;
    np->statnum = sp->statnum;

    np->cstat = 0;
    np->extra = 0;
    np->owner = -1;

    change_sprite_stat(New, STAT_QUAKE_ON);

    QUAKE_Duration(np) *= 120;

    return New;
}


void DoQuakeMatch(short match)
{
    short i, nexti;
    SPRITEp sp;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_QUAKE_SPOT], i, nexti)
    {
        sp = &sprite[i];

        if (QUAKE_Match(sp) == match)
        {
            if ((int16_t)QUAKE_WaitTics(sp) > 0)
            {
                // its not waiting any more
                RESET_BOOL3(sp);
            }
            else
            {
                CopyQuakeSpotToOn(sp);
                if (QUAKE_KillAfterQuake(sp))
                {
                    KillSprite(i);
                    continue;
                }
            }
        }

    }
}

void ProcessQuakeOn(void)
{
    short i, nexti;
    SPRITEp sp;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_QUAKE_ON], i, nexti)
    {
        sp = &sprite[i];

        // get rid of quake when timer runs out
        QUAKE_Duration(sp) -= synctics*4;
        if (QUAKE_Duration(sp) < 0)
        {
            KillSprite(i);
            continue;
        }
    }
}

void ProcessQuakeSpot(void)
{
    short i, nexti;
    SPRITEp sp;
    int rand_test;

    // check timed quakes and random quakes
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_QUAKE_SPOT], i, nexti)
    {
        sp = &sprite[i];

        // not a timed quake
        if (!QUAKE_WaitSecs(sp))
            continue;

        // don't process unless triggered
        if (QUAKE_WaitForTrigger(sp))
            continue;

        // spawn a quake if time is up
        //QUAKE_WaitTics(sp) -= 4*synctics;
        SET_SP_TAG13(sp, (QUAKE_WaitTics(sp)-4*synctics));
        if ((int16_t)QUAKE_WaitTics(sp) < 0)
        {
            // reset timer - add in Duration of quake
            //QUAKE_WaitTics(sp) = ((QUAKE_WaitSecs(sp)*10L) + QUAKE_Duration(sp)) * 120L;
            SET_SP_TAG13(sp, (((QUAKE_WaitSecs(sp)*10L) + QUAKE_Duration(sp)) * 120L));

            // spawn a quake if condition is met
            rand_test = QUAKE_RandomTest(sp);
            // wrong - all quakes need to happen at the same time on all computerssg
            //if (!rand_test || (rand_test && STD_RANDOM_RANGE(128) < rand_test))
            if (!rand_test || (rand_test && RANDOM_RANGE(128) < rand_test))
            {
                CopyQuakeSpotToOn(sp);
                // kill quake spot if needed
                if (QUAKE_KillAfterQuake(sp))
                {
                    DeleteNoSoundOwner(i);
                    KillSprite(i);
                    continue;
                }
            }
        }
    }
}

void QuakeViewChange(PLAYERp pp, int *z_diff, int *x_diff, int *y_diff, short *ang_diff)
{
    short i, nexti;
    SPRITEp sp;
    SPRITEp save_sp = NULL;
    int dist,save_dist = 999999;
    int dist_diff, scale_value;
    int ang_amt;
    int radius;
    int pos_amt;

    *z_diff = 0;
    *x_diff = 0;
    *y_diff = 0;
    *ang_diff = 0;

    if (GamePaused)
        return;

    // find the closest quake - should be a strength value
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_QUAKE_ON], i, nexti)
    {
        sp = &sprite[i];

        dist = FindDistance3D(pp->posx - sp->x, pp->posy - sp->y, (pp->posz - sp->z)>>4);

        // shake whole level
        if (QUAKE_TestDontTaper(sp))
        {
            save_dist = dist;
            save_sp = sp;
            break;
        }

        if (dist < save_dist)
        {
            save_dist = dist;
            save_sp = sp;
        }
    }

    if (!save_sp)
        return;
    else
        sp = save_sp;

    radius = QUAKE_Radius(sp) * 8L;
    if (save_dist > radius)
        return;

    *z_diff = Z(STD_RANDOM_RANGE(QUAKE_Zamt(sp)) - (QUAKE_Zamt(sp)/2));

    ang_amt = QUAKE_AngAmt(sp) * 4L;
    *ang_diff = STD_RANDOM_RANGE(ang_amt) - (ang_amt/2);

    pos_amt = QUAKE_PosAmt(sp) * 4L;
    *x_diff = STD_RANDOM_RANGE(pos_amt) - (pos_amt/2);
    *y_diff = STD_RANDOM_RANGE(pos_amt) - (pos_amt/2);

    if (!QUAKE_TestDontTaper(sp))
    {
        // scale values from epicenter
        dist_diff = radius - save_dist;
        scale_value = divscale16(dist_diff, radius);

        *z_diff = mulscale16(*z_diff, scale_value);
        *ang_diff = mulscale16(*ang_diff, scale_value);
        *x_diff = mulscale16(*x_diff, scale_value);
        *y_diff = mulscale16(*y_diff, scale_value);
    }
}

int SpawnQuake(short sectnum, int x, int y, int z,
               short tics, short amt, int radius)
{
    short SpriteNum;
    SPRITEp sp;

    SpriteNum = COVERinsertsprite(sectnum, STAT_QUAKE_ON);
    sp = &sprite[SpriteNum];

    ASSERT(SpriteNum >= 0);

    sp->x = x;
    sp->y = y;
    sp->z = z;
    sp->cstat = 0;
    sp->owner = -1;
    sp->extra = 0;

    QUAKE_Match(sp) = -1;
    QUAKE_Zamt(sp) = amt;
    QUAKE_Radius(sp) = radius/8;
    QUAKE_Duration(sp) = tics;
    QUAKE_AngAmt(sp) = 8;
    QUAKE_PosAmt(sp) = 0;

    PlaySound(DIGI_ERUPTION, &sp->x, &sp->y, &sp->z, v3df_follow|v3df_dontpan);
    Set3DSoundOwner(SpriteNum);

    return SpriteNum;
}

SWBOOL
SetQuake(PLAYERp pp, short tics, short amt)
{
    SpawnQuake(pp->cursectnum, pp->posx, pp->posy, pp->posz,  tics, amt, 30000);
    return FALSE;
}

int
SetExpQuake(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];

    SpawnQuake(sp->sectnum, sp->x, sp->y, sp->z,  40, 4, 20000); // !JIM! was 8, 40000
    return 0;
}

int
SetGunQuake(int16_t SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    SpawnQuake(sp->sectnum, sp->x, sp->y, sp->z,  40, 8, 40000);

    return 0;
}

int
SetPlayerQuake(PLAYERp pp)
{
    SpawnQuake(pp->cursectnum, pp->posx, pp->posy, pp->posz,  40, 8, 40000);

    return 0;
}

int
SetNuclearQuake(int16_t Weapon)
{
    SPRITEp sp = &sprite[Weapon];

    SpawnQuake(sp->sectnum, sp->x, sp->y, sp->z, 400, 8, 64000);
    return 0;
}

int
SetSumoQuake(int16_t SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];

    SpawnQuake(sp->sectnum, sp->x, sp->y, sp->z,  120, 4, 20000);
    return 0;
}

int
SetSumoFartQuake(int16_t SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];

    SpawnQuake(sp->sectnum, sp->x, sp->y, sp->z,  60, 4, 4000);
    return 0;
}


#include "saveable.h"

static saveable_code saveable_quake_code[] =
{
    SAVE_CODE(CopyQuakeSpotToOn),
    SAVE_CODE(DoQuakeMatch),
    SAVE_CODE(ProcessQuakeOn),
    SAVE_CODE(ProcessQuakeSpot),
    SAVE_CODE(QuakeViewChange),
    SAVE_CODE(SpawnQuake),
    SAVE_CODE(SetQuake),
    SAVE_CODE(SetExpQuake),
    SAVE_CODE(SetGunQuake),
    SAVE_CODE(SetPlayerQuake),
    SAVE_CODE(SetNuclearQuake),
    SAVE_CODE(SetSumoQuake),
    SAVE_CODE(SetSumoFartQuake),
};

saveable_module saveable_quake =
{
    // code
    saveable_quake_code,
    SIZ(saveable_quake_code),

    // data
    NULL,0
};

