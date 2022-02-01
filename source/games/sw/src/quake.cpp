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
#include "ns.h"

#include "build.h"

#include "gamecontrol.h"

#include "names2.h"
#include "game.h"
#include "tags.h"
#include "break.h"
#include "sprite.h"

BEGIN_SW_NS

inline int16_t& QUAKE_Match(DSWActor* actor) { return SP_TAG2(actor); }
inline uint8_t& QUAKE_Zamt(DSWActor* actor) { return SP_TAG3(actor); }
#define QUAKE_Radius(actor) (SP_TAG4(actor))
#define QUAKE_Duration(actor) (SP_TAG5(actor))
#define QUAKE_WaitSecs(actor) (SP_TAG6(actor))
#define QUAKE_AngAmt(actor) (SP_TAG7(actor))
#define QUAKE_PosAmt(actor) (SP_TAG8(actor))
#define QUAKE_RandomTest(actor) (SP_TAG9(actor))
#define QUAKE_WaitTics(actor) (SP_TAG13(actor))

#define QUAKE_TestDontTaper(actor) (TEST_BOOL1(actor))
#define QUAKE_KillAfterQuake(actor) (TEST_BOOL2(actor))
// only for timed quakes
#define QUAKE_WaitForTrigger(actor) (TEST_BOOL3(actor))

void CopyQuakeSpotToOn(DSWActor* actor)
{
    auto actorNew = insertActor(actor->sector(), STAT_QUAKE_SPOT);

    actorNew->spr = actor->spr;
    actorNew->spr.cstat = 0;
    actorNew->spr.extra = 0;

    change_actor_stat(actorNew, STAT_QUAKE_ON);

    QUAKE_Duration(actorNew) *= 120;
}


void DoQuakeMatch(short match)
{
    SWStatIterator it(STAT_QUAKE_SPOT);
    while (auto actor = it.Next())
    {
        if (QUAKE_Match(actor) == match)
        {
            if ((int16_t)QUAKE_WaitTics(actor) > 0)
            {
                // its not waiting any more
                RESET_BOOL3(actor);
            }
            else
            {
                CopyQuakeSpotToOn(actor);
                if (QUAKE_KillAfterQuake(actor))
                {
                    KillActor(actor);
                    continue;
                }
            }
        }

    }
}

void ProcessQuakeOn(void)
{
    SWStatIterator it(STAT_QUAKE_ON);
	while (auto actor = it.Next())
	{
        // get rid of quake when timer runs out
        QUAKE_Duration(actor) -= synctics*4;
        if (QUAKE_Duration(actor) < 0)
        {
            KillActor(actor);
            continue;
        }
    }
}

void ProcessQuakeSpot(void)
{
    int rand_test;

    // check timed quakes and random quakes
    SWStatIterator it(STAT_QUAKE_SPOT);
    while (auto actor = it.Next())
    {
        // not a timed quake
        if (!QUAKE_WaitSecs(actor))
            continue;

        // don't process unless triggered
        if (QUAKE_WaitForTrigger(actor))
            continue;

        // spawn a quake if time is up
        SET_SP_TAG13(actor, (QUAKE_WaitTics(actor)-4*synctics));
        if ((int16_t)QUAKE_WaitTics(actor) < 0)
        {
            // reset timer - add in Duration of quake
            SET_SP_TAG13(actor, (((QUAKE_WaitSecs(actor)*10) + QUAKE_Duration(actor)) * 120));

            // spawn a quake if condition is met
            rand_test = QUAKE_RandomTest(actor);
            // wrong - all quakes need to happen at the same time on all computerssg
            //if (!rand_test || (rand_test && StdRandomRange(128) < rand_test))
            if (!rand_test || (rand_test && RandomRange(128) < rand_test))
            {
                CopyQuakeSpotToOn(actor);
                // kill quake spot if needed
                if (QUAKE_KillAfterQuake(actor))
                {
                    DeleteNoSoundOwner(actor);
                    KillActor(actor);
                    continue;
                }
            }
        }
    }
}

void QuakeViewChange(PLAYER* pp, int *z_diff, int *x_diff, int *y_diff, short *ang_diff)
{
    int i;
    DSWActor* save_act = nullptr;
    int dist,save_dist = 999999;
    int dist_diff, scale_value;
    int ang_amt;
    int radius;
    int pos_amt;

    *z_diff = 0;
    *x_diff = 0;
    *y_diff = 0;
    *ang_diff = 0;

    if (paused)
        return;

    // find the closest quake - should be a strength value
    DSWActor* actor = nullptr;
    SWStatIterator it(STAT_QUAKE_ON);
	while ((actor = it.Next()))
	{
        dist = FindDistance3D(pp->pos - actor->int_pos());

        // shake whole level
        if (QUAKE_TestDontTaper(actor))
        {
            save_dist = dist;
            save_act = actor;
            break;
        }

        if (dist < save_dist)
        {
            save_dist = dist;
            save_act = actor;
        }
    }

    if (!save_act)
        return;
    else
        actor = save_act;

    radius = QUAKE_Radius(actor) * 8L;
    if (save_dist > radius)
        return;

    *z_diff = Z(StdRandomRange(QUAKE_Zamt(actor)) - (QUAKE_Zamt(actor)/2));

    ang_amt = QUAKE_AngAmt(actor) * 4L;
    *ang_diff = StdRandomRange(ang_amt) - (ang_amt/2);

    pos_amt = QUAKE_PosAmt(actor) * 4L;
    *x_diff = StdRandomRange(pos_amt) - (pos_amt/2);
    *y_diff = StdRandomRange(pos_amt) - (pos_amt/2);

    if (!QUAKE_TestDontTaper(actor))
    {
        // scale values from epicenter
        dist_diff = radius - save_dist;
        scale_value = DivScale(dist_diff, radius, 16);

        *z_diff = MulScale(*z_diff, scale_value, 16);
        *ang_diff = MulScale(*ang_diff, scale_value, 16);
        *x_diff = MulScale(*x_diff, scale_value, 16);
        *y_diff = MulScale(*y_diff, scale_value, 16);
    }
}

void SpawnQuake(sectortype* sect, int x, int y, int z,
               short tics, short amt, int radius)
{

    auto actorNew = insertActor(sect, STAT_QUAKE_ON);

    actorNew->set_int_pos({ x, y, z });
    actorNew->spr.cstat = 0;
    actorNew->spr.extra = 0;

    QUAKE_Match(actorNew) = -1;
    QUAKE_Zamt(actorNew) = uint8_t(amt);
    QUAKE_Radius(actorNew) = radius/8;
    QUAKE_Duration(actorNew) = tics;
    QUAKE_AngAmt(actorNew) = 8;
    QUAKE_PosAmt(actorNew) = 0;

    PlaySound(DIGI_ERUPTION, actorNew, v3df_follow|v3df_dontpan);
}

bool SetQuake(PLAYER* pp, short tics, short amt)
{
    SpawnQuake(pp->cursector, pp->pos.X, pp->pos.Y, pp->pos.Z,  tics, amt, 30000);
    return false;
}

int SetExpQuake(DSWActor* actor)
{
    SpawnQuake(actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z,  40, 4, 20000); // !JIM! was 8, 40000
    return 0;
}

int SetGunQuake(DSWActor* actor)
{
    SpawnQuake(actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z,  40, 8, 40000);
    return 0;
}

int SetPlayerQuake(PLAYER* pp)
{
    SpawnQuake(pp->cursector, pp->pos.X, pp->pos.Y, pp->pos.Z,  40, 8, 40000);
    return 0;
}

int SetNuclearQuake(DSWActor* actor)
{
    SpawnQuake(actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 400, 8, 64000);
    return 0;
}

int SetSumoQuake(DSWActor* actor)
{
    SpawnQuake(actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z,  120, 4, 20000);
    return 0;
}

int SetSumoFartQuake(DSWActor* actor)
{
    SpawnQuake(actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z,  60, 4, 4000);
    return 0;
}


END_SW_NS
