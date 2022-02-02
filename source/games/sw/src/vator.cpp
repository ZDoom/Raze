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
#include "network.h"
#include "tags.h"
#include "sector.h"
#include "interpolate.h"
#include "misc.h"
#include "sprite.h"
#include "weapon.h"
#include "quotemgr.h"

BEGIN_SW_NS

void DoVatorMatch(PLAYER* pp, short match);
bool TestVatorMatchActive(short match);

void ReverseVator(DSWActor* actor)
{
    // if paused go ahead and start it up again
    if (actor->user.Tics)
    {
        actor->user.Tics = 0;
        SetVatorActive(actor);
        return;
    }

    // moving toward to OFF pos
    if (actor->user.z_tgt == actor->user.oz)
    {
        if (actor->int_pos().Z == actor->user.oz)
            actor->user.z_tgt = actor->user.pos.Z;
        else if (actor->user.pos.Z == actor->user.oz)
            actor->user.z_tgt = actor->int_pos().Z;
    }
    else if (actor->user.z_tgt == actor->user.pos.Z)
    {
        if (actor->int_pos().Z == actor->user.oz)
            actor->user.z_tgt = actor->int_pos().Z;
        else if (actor->user.pos.Z == actor->user.oz)
            actor->user.z_tgt = actor->user.pos.Z;
    }

    actor->user.vel_rate = -actor->user.vel_rate;
}

bool VatorSwitch(short match, short setting)
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

void SetVatorActive(DSWActor* actor)
{
    sectortype* sectp = actor->sector();

    if (actor->spr.cstat & (CSTAT_SPRITE_YFLIP))
        StartInterpolation(actor->sector(), Interp_Sect_Ceilingz);
    else
        StartInterpolation(actor->sector(), Interp_Sect_Floorz);

    InterpSectorSprites(actor->sector(), true);

    // play activate sound
    DoSoundSpotMatch(SP_TAG2(actor), 1, SOUND_OBJECT_TYPE);

    actor->user.Flags |= (SPR_ACTIVE);
    actor->user.Tics = 0;

    // moving to the ON position
    if (actor->user.z_tgt == actor->int_pos().Z)
        VatorSwitch(SP_TAG2(actor), true);
    else
    // moving to the OFF position
    if (actor->user.z_tgt == actor->user.pos.Z)
        VatorSwitch(SP_TAG2(actor), false);
}

void SetVatorInactive(DSWActor* actor)
{
    sectortype* sectp = actor->sector();

    if (actor->spr.cstat & (CSTAT_SPRITE_YFLIP))
        StopInterpolation(actor->sector(), Interp_Sect_Ceilingz);
    else
        StopInterpolation(actor->sector(), Interp_Sect_Floorz);

    InterpSectorSprites(actor->sector(), false);

    // play inactivate sound
    DoSoundSpotMatch(SP_TAG2(actor), 2, SOUND_OBJECT_TYPE);

    actor->user.Flags &= ~(SPR_ACTIVE);
}

// called for operation from the space bar
void DoVatorOperate(PLAYER* pp, sectortype* sect)
{
    short match;

    SWSectIterator it(sect);
    while (auto actor = it.Next())
    {
        if (actor->spr.statnum == STAT_VATOR && SP_TAG1(actor) == SECT_VATOR && SP_TAG3(actor) == 0)
        {
            auto fsect = actor->sector();

            // single play only vator
            // bool 8 must be set for message to display
            if (TEST_BOOL4(actor) && (gNet.MultiGameType == MULTI_GAME_COMMBAT || gNet.MultiGameType == MULTI_GAME_AI_BOTS))
            {
                if (pp && TEST_BOOL11(actor)) PutStringInfo(pp,"This only opens in single play.");
                continue;
            }

            match = SP_TAG2(actor);
            if (match > 0)
            {
                if (!TestVatorMatchActive(match))
                    DoVatorMatch(pp, match);
                return;
            }

            if (pp && fsect->hasU() && fsect->stag == SECT_LOCK_DOOR && fsect->number)
            {
                short key_num;

                key_num = fsect->number;

                {
                    PutStringInfo(pp, quoteMgr.GetQuote(QUOTE_DOORMSG + key_num - 1));
                    return;
                }
            }

            SetVatorActive(actor);
            break;
        }
    }
}

// called from switches and triggers
// returns first vator found
void DoVatorMatch(PLAYER* pp, short match)
{
    SWStatIterator it(STAT_VATOR);
    while (auto actor = it.Next())
    {
        if (SP_TAG1(actor) == SECT_VATOR && SP_TAG2(actor) == match)
        {
            // single play only vator
            // bool 8 must be set for message to display
            if (TEST_BOOL4(actor) && (gNet.MultiGameType == MULTI_GAME_COMMBAT || gNet.MultiGameType == MULTI_GAME_AI_BOTS))
            {
                if (pp && TEST_BOOL11(actor)) PutStringInfo(pp, GStrings("TXTS_SPONLY"));
                continue;
            }

            // lock code
            auto fsect = actor->sector();
            if (pp && fsect->hasU() && fsect->stag == SECT_LOCK_DOOR && fsect->number)
            {
                int key_num = fsect->number;

                {
                    PutStringInfo(pp, quoteMgr.GetQuote(QUOTE_DOORMSG + key_num - 1));
                    return;
                }
            }

            // remember the player than activated it
            actor->user.PlayerP = pp;

            if (actor->user.Flags & (SPR_ACTIVE))
            {
                ReverseVator(actor);
                continue;
            }

            SetVatorActive(actor);
        }
    }
}


bool TestVatorMatchActive(short match)
{
    SWStatIterator it(STAT_VATOR);
    while (auto actor = it.Next())
    {
        if (SP_TAG1(actor) == SECT_VATOR && SP_TAG2(actor) == match)
        {
            // Does not have to be inactive to be operated
            if (TEST_BOOL6(actor))
                continue;

            if (actor->user.Flags & (SPR_ACTIVE) || actor->user.Tics)
                return true;
        }
    }

    return false;
}

void InterpSectorSprites(sectortype* sect, bool state)
{
    SWSectIterator it(sect);
    while (auto actor = it.Next())
    {
        if (actor->hasU())
        {
            if (actor->user.Flags & (SPR_SKIP4) && actor->spr.statnum <= STAT_SKIP4_INTERP_END)
                continue;

            if (actor->user.Flags & (SPR_SKIP2) && actor->spr.statnum <= STAT_SKIP2_INTERP_END)
                continue;
        }

        if (state)
            StartInterpolation(actor, Interp_Sprite_Z);
        else
            StopInterpolation(actor, Interp_Sprite_Z);
    }
}

void MoveSpritesWithSector(sectortype* sect, int z_amt, bool type)
{
    bool both = false;
    if ( sect->hasU())
        both = !!(sect->flags & SECTFU_VATOR_BOTH);

    SWSectIterator it(sect);
    while (auto actor = it.Next())
    {
        if (actor->hasU())
        {
            switch (actor->spr.statnum)
            {
            case STAT_ITEM:
            case STAT_NO_STATE:
            case STAT_MINE_STUCK:
            case STAT_WALLBLOOD_QUEUE:
            case STAT_FLOORBLOOD_QUEUE:
            case STAT_STATIC_FIRE:
                break;
            default:
                continue;
            }
        }
        else
        {
            switch (actor->spr.statnum)
            {
            case STAT_STAR_QUEUE:
            case STAT_HOLE_QUEUE:
//              case STAT_WALLBLOOD_QUEUE:
//              case STAT_FLOORBLOOD_QUEUE:
                continue;
            }
        }

        if ((actor->spr.extra & SPRX_STAY_PUT_VATOR))
            continue;

        if (both)
        {
            // sprite started close to floor
            if (actor->spr.cstat & (CSTAT_SPRITE_CLOSE_FLOOR))
            {
                // this is a ceiling
                if (type == 1)
                    continue;
            }
            else
            {
                // this is a floor
                if (type == 0)
                    continue;
            }
        }

        actor->add_int_z(z_amt);
    }
}

int DoVatorMove(DSWActor* actor, int *lptr)
{
    int zval;
    int move_amt;

    zval = *lptr;

    // if LESS THAN goal
    if (zval < actor->user.z_tgt)
    {
        // move it DOWN
        zval += (synctics * actor->user.jump_speed);

        actor->user.jump_speed += actor->user.vel_rate * synctics;

        // if the other way make it equal
        if (zval > actor->user.z_tgt)
            zval = actor->user.z_tgt;
    }

    // if GREATER THAN goal
    if (zval > actor->user.z_tgt)
    {
        // move it UP
        zval -= (synctics * actor->user.jump_speed);

        actor->user.jump_speed += actor->user.vel_rate * synctics;

        if (zval < actor->user.z_tgt)
            zval = actor->user.z_tgt;
    }

    move_amt = zval - *lptr;
    *lptr = zval;

    return move_amt;
}


int DoVator(DSWActor* actor)
{
    sectortype* sectp = actor->sector();
    int zval;
    int amt;

    // actor->user.sz        - where the sector z started
    // actor->user.z_tgt     - current target z
    // actor->user.oz        - original z - where it initally starts off
    // actor->spr.z        - z of the sprite
    // actor->user.vel_rate  - velocity

    if (actor->spr.cstat & (CSTAT_SPRITE_YFLIP))
    {
        zval = sectp->int_ceilingz();
        amt = DoVatorMove(actor, &zval);
        sectp->set_int_ceilingz(zval);
        MoveSpritesWithSector(actor->sector(), amt, true); // ceiling
    }
    else
    {
        zval = sectp->int_floorz();
        amt = DoVatorMove(actor, &zval);
        sectp->set_int_floorz(zval);
        MoveSpritesWithSector(actor->sector(), amt, false); // floor
    }

    // EQUAL this entry has finished
    if (zval == actor->user.z_tgt)
    {
        // in the ON position
        if (actor->user.z_tgt == actor->int_pos().Z)
        {
            // change target
            actor->user.z_tgt = actor->user.pos.Z;
            actor->user.vel_rate = -actor->user.vel_rate;

            SetVatorInactive(actor);

            // if tag6 and nothing blocking door
            if (SP_TAG6(actor) && !TEST_BOOL8(actor))
                DoMatchEverything(actor->user.PlayerP, SP_TAG6(actor), -1);
        }
        else
        // in the OFF position
        if (actor->user.z_tgt == actor->user.pos.Z)
        {
            short match = SP_TAG2(actor);

            // change target
            actor->user.jump_speed = actor->user.vel_tgt;
            actor->user.vel_rate = short(abs(actor->user.vel_rate));
            actor->user.z_tgt = actor->int_pos().Z;

            RESET_BOOL8(actor);
            SetVatorInactive(actor);

            // set Owner swith back to OFF
            // only if ALL vators are inactive
            if (!TestVatorMatchActive(match))
            {
                //VatorSwitch(match, OFF);
            }

            if (SP_TAG6(actor) && TEST_BOOL5(actor))
                DoMatchEverything(actor->user.PlayerP, SP_TAG6(actor), -1);
        }

        // operate only once
        if (TEST_BOOL2(actor))
        {
            SetVatorInactive(actor);
            KillActor(actor);
            return 0;
        }

        // setup to go back to the original z
        if (zval != actor->user.oz)
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
            int i;
            bool found = false;

            SWSectIterator it(actor->sector());
            while (auto itActor = it.Next())
            {
                if (itActor->spr.statnum == STAT_ENEMY)
                {
                    if (labs(sectp->int_ceilingz() - sectp->int_floorz()) < ActorSizeZ(itActor))
                    {
                        InitBloodSpray(itActor, true, -1);
                        UpdateSinglePlayKills(itActor);
                        KillActor(itActor);
                        continue;
                    }
                }

                if (itActor->hasU() && (itActor->spr.cstat & CSTAT_SPRITE_BLOCK) && (itActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
                {
                    // found something blocking so reverse to ON position
                    ReverseVator(actor);
                    SET_BOOL8(actor); // tell vator that something blocking door
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
                        ReverseVator(actor);

                        actor->user.vel_rate = -actor->user.vel_rate;
                        found = true;
                    }
                }
            }
        }
        else
        {
            SWSectIterator it(actor->sector());
            while (auto itActor = it.Next())
            {
                if (itActor->spr.statnum == STAT_ENEMY)
                {
                    if (labs(sectp->int_ceilingz() - sectp->int_floorz()) < ActorSizeZ(itActor))
                    {
                        InitBloodSpray(itActor, true, -1);
                        UpdateSinglePlayKills(itActor);
                        KillActor(itActor);
                        continue;
                    }
                }
            }
        }
    }



    return 0;
}

int DoVatorAuto(DSWActor* actor)
{
    sectortype* sectp = actor->sector();
    int zval;
    int amt;

    if (actor->spr.cstat & (CSTAT_SPRITE_YFLIP))
    {
        zval = sectp->int_ceilingz();
        amt = DoVatorMove(actor, &zval);
        sectp->set_int_ceilingz(zval);
        MoveSpritesWithSector(actor->sector(), amt, true); // ceiling
    }
    else
    {
        zval = sectp->int_floorz();
        amt = DoVatorMove(actor, &zval);
        sectp->set_int_floorz(zval);
        MoveSpritesWithSector(actor->sector(), amt, false); // floor
    }

    // EQUAL this entry has finished
    if (zval == actor->user.z_tgt)
    {
        // in the UP position
        if (actor->user.z_tgt == actor->int_pos().Z)
        {
            // change target
            actor->user.z_tgt = actor->user.pos.Z;
            actor->user.vel_rate = -actor->user.vel_rate;
            actor->user.Tics = actor->user.WaitTics;

            if (SP_TAG6(actor))
                DoMatchEverything(actor->user.PlayerP, SP_TAG6(actor), -1);
        }
        else
        // in the DOWN position
        if (actor->user.z_tgt == actor->user.pos.Z)
        {
            // change target
            actor->user.jump_speed = actor->user.vel_tgt;
            actor->user.vel_rate = short(abs(actor->user.vel_rate));
            actor->user.z_tgt = actor->int_pos().Z;
            actor->user.Tics = actor->user.WaitTics;

            if (SP_TAG6(actor) && TEST_BOOL5(actor))
                DoMatchEverything(nullptr, SP_TAG6(actor), -1);
        }
    }

    return 0;
}


#include "saveable.h"

static saveable_code saveable_vator_code[] =
{
    SAVE_CODE(DoVator),
    SAVE_CODE(DoVatorAuto),
};

saveable_module saveable_vator =
{
    // code
    saveable_vator_code,
    SIZ(saveable_vator_code),

    // data
    nullptr,0
};
END_SW_NS
