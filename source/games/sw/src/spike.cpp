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

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "sector.h"
#include "sprite.h"
#include "interpolate.h"

BEGIN_SW_NS

bool TestSpikeMatchActive(short match);
void InterpSectorSprites(short sectnum, bool state);

void ReverseSpike(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();

    // if paused go ahead and start it up again
    if (u->Tics)
    {
        u->Tics = 0;
        SetSpikeActive(actor);
        return;
    }

    // moving toward to OFF pos
    if (u->z_tgt == u->oz)
    {
        if (sp->z == u->oz)
            u->z_tgt = u->sz;
        else if (u->sz == u->oz)
            u->z_tgt = sp->z;
    }
    else if (u->z_tgt == u->sz)
    {
        if (sp->z == u->oz)
            u->z_tgt = sp->z;
        else if (u->sz == u->oz)
            u->z_tgt = u->sz;
    }

    u->vel_rate = -u->vel_rate;
}

bool SpikeSwitch(short match, short setting)
{
    SPRITEp sp;
    bool found = false;

    SWStatIterator it(STAT_DEFAULT);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (sp->lotag == TAG_SPRITE_SWITCH_VATOR && sp->hitag == match)
        {
            found = true;
            AnimateSwitch(sp, setting);
        }
    }

    return found;
}

void SetSpikeActive(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();
    SECTORp sectp = &sector[sp->sectnum];

    if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
        StartInterpolation(sp->sectnum, Interp_Sect_Ceilingheinum);
    else
        StartInterpolation(sp->sectnum, Interp_Sect_Floorheinum);

    InterpSectorSprites(sp->sectnum, true);

    // play activate sound
    DoSoundSpotMatch(SP_TAG2(sp), 1, SOUND_OBJECT_TYPE);

    SET(u->Flags, SPR_ACTIVE);
    u->Tics = 0;

    // moving to the ON position
    if (u->z_tgt == sp->z)
        VatorSwitch(SP_TAG2(sp), ON);
    else
    // moving to the OFF position
    if (u->z_tgt == u->sz)
        VatorSwitch(SP_TAG2(sp), OFF);
}

void SetSpikeInactive(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();
    SECTORp sectp = &sector[sp->sectnum];

    if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
        StopInterpolation(sp->sectnum, Interp_Sect_Ceilingheinum);
    else
        StopInterpolation(sp->sectnum, Interp_Sect_Floorheinum);

    InterpSectorSprites(sp->sectnum, false);

    // play activate sound
    DoSoundSpotMatch(SP_TAG2(sp), 2, SOUND_OBJECT_TYPE);

    RESET(u->Flags, SPR_ACTIVE);
}

// called for operation from the space bar
void DoSpikeOperate(short sectnum)
{
    SPRITEp fsp;
    short match;

    SWSectIterator it(sectnum);
    while (auto actor = it.Next())
    {
        fsp = &actor->s();

        if (fsp->statnum == STAT_SPIKE && SP_TAG1(fsp) == SECT_SPIKE && SP_TAG3(fsp) == 0)
        {
            sectnum = fsp->sectnum;

            match = SP_TAG2(fsp);
            if (match > 0)
            {
                if (!TestSpikeMatchActive(match))
                    DoSpikeMatch(match);
                return;
            }

            SetSpikeActive(actor);
            break;
        }
    }
}

// called from switches and triggers
// returns first spike found
void DoSpikeMatch(short match)
{
    USERp fu;
    SPRITEp fsp;

    SWStatIterator it(STAT_SPIKE);
    while (auto actor = it.Next())
    {
        fsp = &actor->s();

        if (SP_TAG1(fsp) == SECT_SPIKE && SP_TAG2(fsp) == match)
        {
            fu = actor->u();

            if (TEST(fu->Flags, SPR_ACTIVE))
            {
                ReverseSpike(actor);
                continue;
            }

            SetSpikeActive(actor);
        }
    }
}


bool TestSpikeMatchActive(short match)
{
    USERp fu;
    SPRITEp fsp;

    SWStatIterator it(STAT_SPIKE);
    while (auto actor = it.Next())
    {
        fsp = &actor->s();

        if (SP_TAG1(fsp) == SECT_SPIKE && SP_TAG2(fsp) == match)
        {
            fu = actor->u();

            // door war
            if (TEST_BOOL6(fsp))
                continue;

            if (TEST(fu->Flags, SPR_ACTIVE) || fu->Tics)
                return true;
        }
    }

    return false;
}

int DoSpikeMove(DSWActor* actor, int *lptr)
{
    USERp u = actor->u();
    int zval;

    zval = *lptr;

    // if LESS THAN goal
    if (zval < u->z_tgt)
    {
        // move it DOWN
        zval += (synctics * u->jump_speed);

        u->jump_speed += u->vel_rate * synctics;

        // if the other way make it equal
        if (zval > u->z_tgt)
            zval = u->z_tgt;
    }

    // if GREATER THAN goal
    if (zval > u->z_tgt)
    {
        // move it UP
        zval -= (synctics * u->jump_speed);

        u->jump_speed += u->vel_rate * synctics;

        if (zval < u->z_tgt)
            zval = u->z_tgt;
    }

    *lptr = zval;

    return 0;
}

void SpikeAlign(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();

    // either work on single sector or all tagged in SOBJ
    if ((int8_t)SP_TAG7(sp) < 0)
    {
        if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
            alignceilslope(sp->sectnum, sp->x, sp->y, u->zclip);
        else
            alignflorslope(sp->sectnum, sp->x, sp->y, u->zclip);
    }
    else
    {
        if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
            SOBJ_AlignCeilingToPoint(&SectorObject[SP_TAG7(sp)], sp->x, sp->y, u->zclip);
        else
            SOBJ_AlignFloorToPoint(&SectorObject[SP_TAG7(sp)], sp->x, sp->y, u->zclip);
    }
}

void MoveSpritesWithSpike(short sectnum)
{
    SPRITEp sp;
    int cz,fz;

    SWSectIterator it(sectnum);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (actor->hasU())
            continue;

        if (TEST(sp->extra, SPRX_STAY_PUT_VATOR))
            continue;

        getzsofslope(sectnum, sp->x, sp->y, &cz, &fz);
        sp->z = fz;
    }
}

int DoSpike(DSWActor* actor)
{
    USER* u = actor->u();
    SPRITEp sp = &actor->s();
    int *lptr;

    // zclip = floor or ceiling z
    // oz = original z
    // z_tgt = target z - on pos
    // sz = starting z - off pos

    lptr = &u->zclip;

    DoSpikeMove(actor, lptr);
    MoveSpritesWithSpike(sp->sectnum);
    SpikeAlign(actor);

    // EQUAL this entry has finished
    if (*lptr == u->z_tgt)
    {
        // in the ON position
        if (u->z_tgt == sp->z)
        {
            // change target
            u->z_tgt = u->sz;
            u->vel_rate = -u->vel_rate;

            SetSpikeInactive(actor);

            if (SP_TAG6(sp))
                DoMatchEverything(nullptr, SP_TAG6(sp), -1);
        }
        else
        // in the OFF position
        if (u->z_tgt == u->sz)
        {
            short match = SP_TAG2(sp);

            // change target
            u->jump_speed = u->vel_tgt;
            u->vel_rate = (short)abs(u->vel_rate);
            u->z_tgt = sp->z;

            SetSpikeInactive(actor);

            // set Owner swith back to OFF
            // only if ALL spikes are inactive
            if (!TestSpikeMatchActive(match))
            {
                //SpikeSwitch(match, OFF);
            }

            if (SP_TAG6(sp) && TEST_BOOL5(sp))
                DoMatchEverything(nullptr, SP_TAG6(sp), -1);
        }

        // operate only once
        if (TEST_BOOL2(sp))
        {
            SetSpikeInactive(actor);
            KillActor(actor);
            return 0;
        }

        // setup to go back to the original z
        if (*lptr != u->oz)
        {
            if (u->WaitTics)
                u->Tics = u->WaitTics;
        }
    }
    else // if (*lptr == u->z_tgt)
    {
        // if heading for the OFF (original) position and should NOT CRUSH
        if (TEST_BOOL3(sp) && u->z_tgt == u->oz)
        {
           SPRITEp bsp;
            USERp bu;
            bool found = false;

            SWSectIterator it(sp->sectnum);
            while (auto itActor = it.Next())
            {
                bsp = &actor->s();
                bu = actor->u();

                if (bu && TEST(bsp->cstat, CSTAT_SPRITE_BLOCK) && TEST(bsp->extra, SPRX_PLAYER_OR_ENEMY))
                {
                    ReverseSpike(actor);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                short pnum;
                PLAYERp pp;
                // go ahead and look for players clip box bounds
                TRAVERSE_CONNECT(pnum)
                {
                    pp = Player + pnum;

                    if (pp->lo_sectp == &sector[sp->sectnum] ||
                        pp->hi_sectp == &sector[sp->sectnum])
                    {
                        ReverseSpike(actor);
                        found = true;
                    }
                }
            }
        }
    }

    return 0;
}

int DoSpikeAuto(DSWActor* actor)
{
    USER* u = actor->u();
    SPRITEp sp = &actor->s();
    int *lptr;

    lptr = &u->zclip;

    DoSpikeMove(actor, lptr);
    MoveSpritesWithSpike(sp->sectnum);
    SpikeAlign(actor);

    // EQUAL this entry has finished
    if (*lptr == u->z_tgt)
    {
        // in the UP position
        if (u->z_tgt == sp->z)
        {
            // change target
            u->z_tgt = u->sz;
            u->vel_rate = -u->vel_rate;
            u->Tics = u->WaitTics;

            if (SP_TAG6(sp))
                DoMatchEverything(nullptr, SP_TAG6(sp), -1);
        }
        else
        // in the DOWN position
        if (u->z_tgt == u->sz)
        {
            // change target
            u->jump_speed = u->vel_tgt;
            u->vel_rate = (short)abs(u->vel_rate);
            u->z_tgt = sp->z;
            u->Tics = u->WaitTics;

            if (SP_TAG6(sp) && TEST_BOOL5(sp))
                DoMatchEverything(nullptr, SP_TAG6(sp), -1);
        }
    }

    return 0;
}


#include "saveable.h"

static saveable_code saveable_spike_code[] =
{
    SAVE_CODE(DoSpike),
    SAVE_CODE(DoSpikeAuto),
};

saveable_module saveable_spike =
{
    // code
    saveable_spike_code,
    SIZ(saveable_spike_code),

    // data
    nullptr,0
};
END_SW_NS
