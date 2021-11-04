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

void DoVatorMatch(PLAYERp pp, short match);
bool TestVatorMatchActive(short match);
void InterpSectorSprites(short sectnum, bool state);

void ReverseVator(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();

    // if paused go ahead and start it up again
    if (u->Tics)
    {
        u->Tics = 0;
        SetVatorActive(actor);
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

bool VatorSwitch(short match, short setting)
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

void SetVatorActive(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();
    SECTORp sectp = &sector[sp->sectnum];

    if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
        StartInterpolation(sp->sectnum, Interp_Sect_Ceilingz);
    else
        StartInterpolation(sp->sectnum, Interp_Sect_Floorz);

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

void SetVatorInactive(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();
    SECTORp sectp = &sector[sp->sectnum];

    if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
        StopInterpolation(sp->sectnum, Interp_Sect_Ceilingz);
    else
        StopInterpolation(sp->sectnum, Interp_Sect_Floorz);

    InterpSectorSprites(sp->sectnum, false);

    // play inactivate sound
    DoSoundSpotMatch(SP_TAG2(sp), 2, SOUND_OBJECT_TYPE);

    RESET(u->Flags, SPR_ACTIVE);
}

// called for operation from the space bar
void DoVatorOperate(PLAYERp pp, short sectnum)
{
    SPRITEp fsp;
    short match;

    SWSectIterator it(sectnum);
    while (auto actor = it.Next())
    {
        fsp = &actor->s();

        if (fsp->statnum == STAT_VATOR && SP_TAG1(fsp) == SECT_VATOR && SP_TAG3(fsp) == 0)
        {
            sectnum = fsp->sectnum;

            // single play only vator
            // bool 8 must be set for message to display
            if (TEST_BOOL4(fsp) && (gNet.MultiGameType == MULTI_GAME_COMMBAT || gNet.MultiGameType == MULTI_GAME_AI_BOTS))
            {
                if (pp && TEST_BOOL11(fsp)) PutStringInfo(pp,"This only opens in single play.");
                continue;
            }

            match = SP_TAG2(fsp);
            if (match > 0)
            {
                if (!TestVatorMatchActive(match))
                    DoVatorMatch(pp, match);
                return;
            }

            if (pp && SectUser[sectnum].Data() && SectUser[sectnum]->stag == SECT_LOCK_DOOR && SectUser[sectnum]->number)
            {
                short key_num;

                key_num = SectUser[sectnum]->number;

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
void DoVatorMatch(PLAYERp pp, short match)
{
    USERp fu;
    SPRITEp fsp;
    short sectnum;

    SWStatIterator it(STAT_VATOR);
    while (auto actor = it.Next())
    {
        fsp = &actor->s();

        if (SP_TAG1(fsp) == SECT_VATOR && SP_TAG2(fsp) == match)
        {
            fu = actor->u();

            // single play only vator
            // bool 8 must be set for message to display
            if (TEST_BOOL4(fsp) && (gNet.MultiGameType == MULTI_GAME_COMMBAT || gNet.MultiGameType == MULTI_GAME_AI_BOTS))
            {
                if (pp && TEST_BOOL11(fsp)) PutStringInfo(pp, GStrings("TXTS_SPONLY"));
                continue;
            }

            // lock code
            sectnum = fsp->sectnum;
            if (pp && SectUser[sectnum].Data() && SectUser[sectnum]->stag == SECT_LOCK_DOOR && SectUser[sectnum]->number)
            {
                short key_num;

                key_num = SectUser[sectnum]->number;

                {
                    PutStringInfo(pp, quoteMgr.GetQuote(QUOTE_DOORMSG + key_num - 1));
                    return;
                }
            }

            // remember the player than activated it
            fu->PlayerP = pp;

            if (TEST(fu->Flags, SPR_ACTIVE))
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
    USERp fu;
    SPRITEp fsp;

    SWStatIterator it(STAT_VATOR);
    while (auto actor = it.Next())
    {
        fsp = &actor->s();

        if (SP_TAG1(fsp) == SECT_VATOR && SP_TAG2(fsp) == match)
        {
            fu = actor->u();

            // Does not have to be inactive to be operated
            if (TEST_BOOL6(fsp))
                continue;

            if (TEST(fu->Flags, SPR_ACTIVE) || fu->Tics)
                return true;
        }
    }

    return false;
}

void InterpSectorSprites(short sectnum, bool state)
{
    SPRITEp sp;

    SWSectIterator it(sectnum);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (actor->hasU())
        {
            auto u = actor->u();
            if (TEST(u->Flags, SPR_SKIP4) && sp->statnum <= STAT_SKIP4_INTERP_END)
                continue;

            if (TEST(u->Flags, SPR_SKIP2) && sp->statnum <= STAT_SKIP2_INTERP_END)
                continue;
        }

        if (state)
            StartInterpolation(actor->GetSpriteIndex(), Interp_Sprite_Z);
        else
            StopInterpolation(actor->GetSpriteIndex(), Interp_Sprite_Z);
    }
}

void MoveSpritesWithSector(short sectnum, int z_amt, bool type)
{
    SPRITEp sp;
    bool both = false;

    if (SectUser[sectnum].Data())
        both = !!TEST(SectUser[sectnum]->flags, SECTFU_VATOR_BOTH);

    SWSectIterator it(sectnum);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (actor->hasU())
        {
            switch (sp->statnum)
            {
            case STAT_ITEM:
            case STAT_NO_STATE:
            case STAT_MINE_STUCK:
            case STAT_WALLBLOOD_QUEUE:
            case STAT_FLOORBLOOD_QUEUE:
            case STAT_STATIC_FIRE:
                break;
            default:
                goto cont;
            }
        }
        else
        {
            switch (sp->statnum)
            {
            case STAT_STAR_QUEUE:
            case STAT_HOLE_QUEUE:
//              case STAT_WALLBLOOD_QUEUE:
//              case STAT_FLOORBLOOD_QUEUE:
                goto cont;
            }
        }

        if (TEST(sp->extra, SPRX_STAY_PUT_VATOR))
            continue;

        if (both)
        {
            // sprite started close to floor
            if (TEST(sp->cstat, CSTAT_SPRITE_CLOSE_FLOOR))
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

        sp->z += z_amt;

cont:
        continue;
    }
}

int DoVatorMove(DSWActor* actor, int *lptr)
{
    USERp u = actor->u();
    int zval;
    int move_amt;

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

    move_amt = zval - *lptr;
    *lptr = zval;

    return move_amt;
}


int DoVator(DSWActor* actor)
{
    USER* u = actor->u();
    SPRITEp sp = &actor->s();
    SECTORp sectp = &sector[sp->sectnum];
    int *lptr;
    int amt;

    // u->sz        - where the sector z started
    // u->z_tgt     - current target z
    // u->oz        - original z - where it initally starts off
    // sp->z        - z of the sprite
    // u->vel_rate  - velocity

    if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
    {
        lptr = &sectp->ceilingz;
        amt = DoVatorMove(actor, lptr);
        MoveSpritesWithSector(sp->sectnum, amt, true); // ceiling
    }
    else
    {
        lptr = &sectp->floorz;
        amt = DoVatorMove(actor, lptr);
        MoveSpritesWithSector(sp->sectnum, amt, false); // floor
    }

    // EQUAL this entry has finished
    if (*lptr == u->z_tgt)
    {
        // in the ON position
        if (u->z_tgt == sp->z)
        {
            // change target
            u->z_tgt = u->sz;
            u->vel_rate = -u->vel_rate;

            SetVatorInactive(actor);

            // if tag6 and nothing blocking door
            if (SP_TAG6(sp) && !TEST_BOOL8(sp))
                DoMatchEverything(u->PlayerP, SP_TAG6(sp), -1);
        }
        else
        // in the OFF position
        if (u->z_tgt == u->sz)
        {
            short match = SP_TAG2(sp);

            // change target
            u->jump_speed = u->vel_tgt;
            u->vel_rate = short(abs(u->vel_rate));
            u->z_tgt = sp->z;

            RESET_BOOL8(sp);
            SetVatorInactive(actor);

            // set Owner swith back to OFF
            // only if ALL vators are inactive
            if (!TestVatorMatchActive(match))
            {
                //VatorSwitch(match, OFF);
            }

            if (SP_TAG6(sp) && TEST_BOOL5(sp))
                DoMatchEverything(u->PlayerP, SP_TAG6(sp), -1);
        }

        // operate only once
        if (TEST_BOOL2(sp))
        {
            SetVatorInactive(actor);
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
            int i;
            SPRITEp bsp;
            USERp bu;
            bool found = false;

            SWSectIterator it(sp->sectnum);
            while (auto itActor = it.Next())
            {
                bsp = &itActor->s();
                bu = itActor->u();

                if (bsp->statnum == STAT_ENEMY)
                {
                    if (labs(sectp->ceilingz - sectp->floorz) < SPRITEp_SIZE_Z(bsp))
                    {
                        InitBloodSpray(itActor, true, -1);
                        UpdateSinglePlayKills(itActor);
                        KillActor(itActor);
                        continue;
                    }
                }

                if (bu && TEST(bsp->cstat, CSTAT_SPRITE_BLOCK) && TEST(bsp->extra, SPRX_PLAYER_OR_ENEMY))
                {
                    // found something blocking so reverse to ON position
                    ReverseVator(actor);
                    SET_BOOL8(sp); // tell vator that something blocking door
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
                        ReverseVator(actor);

                        u->vel_rate = -u->vel_rate;
                        found = true;
                    }
                }
            }
        }
        else
        {
            SPRITEp bsp;

            SWSectIterator it(sp->sectnum);
            while (auto itActor = it.Next())
            {
                bsp = &itActor->s();

                if (bsp->statnum == STAT_ENEMY)
                {
                    if (labs(sectp->ceilingz - sectp->floorz) < SPRITEp_SIZE_Z(bsp))
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
    USER* u = actor->u();
    SPRITEp sp = &actor->s();
    SECTORp sectp = &sector[sp->sectnum];
    int *lptr;
    int amt;

    if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
    {
        lptr = &sectp->ceilingz;
        amt = DoVatorMove(actor, lptr);
        MoveSpritesWithSector(sp->sectnum, amt, true); // ceiling
    }
    else
    {
        lptr = &sectp->floorz;
        amt = DoVatorMove(actor, lptr);
        MoveSpritesWithSector(sp->sectnum, amt, false); // floor
    }

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
                DoMatchEverything(u->PlayerP, SP_TAG6(sp), -1);
        }
        else
        // in the DOWN position
        if (u->z_tgt == u->sz)
        {
            // change target
            u->jump_speed = u->vel_tgt;
            u->vel_rate = short(abs(u->vel_rate));
            u->z_tgt = sp->z;
            u->Tics = u->WaitTics;

            if (SP_TAG6(sp) && TEST_BOOL5(sp))
                DoMatchEverything(nullptr, SP_TAG6(sp), -1);
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
