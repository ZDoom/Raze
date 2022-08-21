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

void ReverseSpike(DSWActor* actor)
{
    // if paused go ahead and start it up again
    if (actor->user.Tics)
    {
        actor->user.Tics = 0;
        SetSpikeActive(actor);
        return;
    }

    // moving toward to OFF pos
    if (actor->user.z_tgt == actor->user.oz)
    {
        if (actor->spr.pos.Z == actor->user.oz)
            actor->user.z_tgt = actor->user.int_upos().Z * zinttoworld;
        else if (actor->user.int_upos().Z == actor->user.int_oz())
            actor->user.z_tgt = actor->spr.pos.Z;
    }
    else if (actor->user.int_z_tgt() == actor->user.int_upos().Z)
    {
        if (actor->spr.pos.Z == actor->user.oz)
            actor->user.z_tgt = actor->spr.pos.Z;
        else if (actor->user.int_upos().Z == actor->user.int_oz())
            actor->user.z_tgt = actor->user.int_upos().Z * zinttoworld;
    }

    actor->user.vel_rate = -actor->user.vel_rate;
}

bool SpikeSwitch(short match, short setting)
{
    bool found = false;

    SWStatIterator it(STAT_DEFAULT);
    while (auto actor = it.Next())
    {
        if (actor->spr.lotag == TAG_SPRITE_SWITCH_VATOR && actor->spr.hitag == match)
        {
            found = true;
            AnimateSwitch(actor, setting);
        }
    }

    return found;
}

void SetSpikeActive(DSWActor* actor)
{
    sectortype* sectp = actor->sector();

    if (actor->spr.cstat & (CSTAT_SPRITE_YFLIP))
        StartInterpolation(actor->sector(), Interp_Sect_Ceilingheinum);
    else
        StartInterpolation(actor->sector(), Interp_Sect_Floorheinum);

    InterpSectorSprites(actor->sector(), true);

    // play activate sound
    DoSoundSpotMatch(SP_TAG2(actor), 1, SOUND_OBJECT_TYPE);

    actor->user.Flags |= (SPR_ACTIVE);
    actor->user.Tics = 0;

    // moving to the ON position
    if (actor->user.z_tgt == actor->spr.pos.Z)
        VatorSwitch(SP_TAG2(actor), true);
    else
    // moving to the OFF position
    if (actor->user.int_z_tgt() == actor->user.int_upos().Z)
        VatorSwitch(SP_TAG2(actor), false);
}

void SetSpikeInactive(DSWActor* actor)
{
    sectortype* sectp = actor->sector();

    if (actor->spr.cstat & (CSTAT_SPRITE_YFLIP))
        StopInterpolation(sectp, Interp_Sect_Ceilingheinum);
    else
        StopInterpolation(sectp, Interp_Sect_Floorheinum);

    InterpSectorSprites(sectp, false);

    // play activate sound
    DoSoundSpotMatch(SP_TAG2(actor), 2, SOUND_OBJECT_TYPE);

    actor->user.Flags &= ~(SPR_ACTIVE);
}

// called for operation from the space bar
void DoSpikeOperate(sectortype* sect)
{
    short match;

    SWSectIterator it(sect);
    while (auto actor = it.Next())
    {
        if (actor->spr.statnum == STAT_SPIKE && SP_TAG1(actor) == SECT_SPIKE && SP_TAG3(actor) == 0)
        {
            match = SP_TAG2(actor);
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
    SWStatIterator it(STAT_SPIKE);
    while (auto actor = it.Next())
    {
        if (SP_TAG1(actor) == SECT_SPIKE && SP_TAG2(actor) == match)
        {
            if (actor->user.Flags & SPR_ACTIVE)
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
    SWStatIterator it(STAT_SPIKE);
    while (auto actor = it.Next())
    {
        if (SP_TAG1(actor) == SECT_SPIKE && SP_TAG2(actor) == match)
        {
            // door war
            if (TEST_BOOL6(actor))
                continue;

            if (actor->user.Flags & (SPR_ACTIVE) || actor->user.Tics)
                return true;
        }
    }

    return false;
}

int DoSpikeMove(DSWActor* actor, double *lptr)
{
    double zval;

    zval = *lptr;

    // if LESS THAN goal
    if (zval < actor->user.z_tgt)
    {
        // move it DOWN
        zval += (synctics * actor->user.jump_speed) * zinttoworld;

        actor->user.jump_speed += actor->user.vel_rate * synctics;

        // if the other way make it equal
        if (zval > actor->user.z_tgt)
            zval = actor->user.z_tgt;
    }

    // if GREATER THAN goal
    if (zval > actor->user.z_tgt)
    {
        // move it UP
        zval -= (synctics * actor->user.jump_speed) * zinttoworld;

        actor->user.jump_speed += actor->user.vel_rate * synctics;

        if (zval < actor->user.z_tgt)
            zval = actor->user.z_tgt;
    }

    *lptr = zval;

    return 0;
}

void SpikeAlign(DSWActor* actor)
{
    // either work on single sector or all tagged in SOBJ
    if ((int8_t)SP_TAG7(actor) < 0)
    {
        if (actor->spr.cstat & (CSTAT_SPRITE_YFLIP))
            alignceilslope(actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->user.int_zclip());
        else
            alignflorslope(actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->user.int_zclip());
    }
    else
    {
        if (actor->spr.cstat & (CSTAT_SPRITE_YFLIP))
            SOBJ_AlignCeilingToPoint(&SectorObject[SP_TAG7(actor)], actor->int_pos().X, actor->int_pos().Y, actor->user.int_zclip());
        else
            SOBJ_AlignFloorToPoint(&SectorObject[SP_TAG7(actor)], actor->int_pos().X, actor->int_pos().Y, actor->user.int_zclip());
    }
}

void MoveSpritesWithSpike(sectortype* sect)
{
    int cz,fz;

    SWSectIterator it(sect);
    while (auto actor = it.Next())
    {
        if (actor->hasU())
            continue;

        if ((actor->spr.extra & SPRX_STAY_PUT_VATOR))
            continue;

        getzsofslopeptr(sect, actor->int_pos().X, actor->int_pos().Y, &cz, &fz);
        actor->set_int_z(fz);
    }
}

int DoSpike(DSWActor* actor)
{
    // zclip = floor or ceiling z
    // oz = original z
    // z_tgt = target z - on pos
    // sz = starting z - off pos

    DoSpikeMove(actor, &actor->user.zclip);
    MoveSpritesWithSpike(actor->sector());
    SpikeAlign(actor);

    // EQUAL this entry has finished
    if (actor->user.zclip == actor->user.z_tgt)
    {
        // in the ON position
        if (actor->user.z_tgt == actor->spr.pos.Z)
        {
            // change target
            actor->user.z_tgt = actor->user.int_upos().Z * zinttoworld;
            actor->user.vel_rate = -actor->user.vel_rate;

            SetSpikeInactive(actor);

            if (SP_TAG6(actor))
                DoMatchEverything(nullptr, SP_TAG6(actor), -1);
        }
        else
        // in the OFF position
        if (actor->user.int_z_tgt() == actor->user.int_upos().Z)
        {
            short match = SP_TAG2(actor);

            // change target
            actor->user.jump_speed = actor->user.vel_tgt;
            actor->user.vel_rate = (short)abs(actor->user.vel_rate);
            actor->user.z_tgt = actor->spr.pos.Z;

            SetSpikeInactive(actor);

            // set Owner swith back to OFF
            // only if ALL spikes are inactive
            if (!TestSpikeMatchActive(match))
            {
                //SpikeSwitch(match, OFF);
            }

            if (SP_TAG6(actor) && TEST_BOOL5(actor))
                DoMatchEverything(nullptr, SP_TAG6(actor), -1);
        }

        // operate only once
        if (TEST_BOOL2(actor))
        {
            SetSpikeInactive(actor);
            KillActor(actor);
            return 0;
        }

        // setup to go back to the original z
        if (actor->user.zclip != actor->user.oz)
        {
            if (actor->user.WaitTics)
                actor->user.Tics = actor->user.WaitTics;
        }
    }
    else // if (*lptr == actor->user.z_tgt)
    {
        // if heading for the OFF (original) position and should NOT CRUSH
        if (TEST_BOOL3(actor) && actor->user.z_tgt == actor->user.oz)
        {
            bool found = false;

            SWSectIterator it(actor->sector());
            while (auto itActor = it.Next())
            {
                if (actor->hasU() && (actor->spr.cstat & CSTAT_SPRITE_BLOCK) && (actor->spr.extra & SPRX_PLAYER_OR_ENEMY))
                {
                    ReverseSpike(actor);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                short pnum;
                PLAYER* pp;
                // go ahead and look for players clip box bounds
                TRAVERSE_CONNECT(pnum)
                {
                    pp = Player + pnum;

                    if (pp->lo_sectp == actor->sector() ||
                        pp->hi_sectp == actor->sector())
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
    DoSpikeMove(actor, &actor->user.zclip);
    MoveSpritesWithSpike(actor->sector());
    SpikeAlign(actor);

    // EQUAL this entry has finished
    if (actor->user.zclip == actor->user.z_tgt)
    {
        // in the UP position
        if (actor->user.z_tgt == actor->spr.pos.Z)
        {
            // change target
            actor->user.z_tgt = actor->user.int_upos().Z * zinttoworld;
            actor->user.vel_rate = -actor->user.vel_rate;
            actor->user.Tics = actor->user.WaitTics;

            if (SP_TAG6(actor))
                DoMatchEverything(nullptr, SP_TAG6(actor), -1);
        }
        else
        // in the DOWN position
        if (actor->user.int_z_tgt() == actor->user.int_upos().Z)
        {
            // change target
            actor->user.jump_speed = actor->user.vel_tgt;
            actor->user.vel_rate = (short)abs(actor->user.vel_rate);
            actor->user.z_tgt = actor->spr.pos.Z;
            actor->user.Tics = actor->user.WaitTics;

            if (SP_TAG6(actor) && TEST_BOOL5(actor))
                DoMatchEverything(nullptr, SP_TAG6(actor), -1);
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
