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
#include "misc.h"
#include "network.h"
#include "tags.h"
#include "sector.h"
#include "interpolate.h"
#include "sprite.h"
#include "quotemgr.h"

BEGIN_SW_NS

void DoRotatorMatch(PLAYER* pp, short match, bool);
bool TestRotatorMatchActive(short match);
void DoMatchEverything(PLAYER* pp, short match, short state);
void DoRotatorSetInterp(DSWActor*);
void DoRotatorStopInterp(DSWActor*);

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ReverseRotator(DSWActor* actor)
{
    ROTATOR* r;

    r = actor->user.rotator.Data();

    // if paused go ahead and start it up again
    if (actor->user.Tics)
    {
        actor->user.Tics = 0;
        SetRotatorActive(actor);
        return;
    }

    // moving toward to OFF pos
    if (r->tgt == 0)
    {
        r->tgt = r->open_dest;
    }
    else if (r->tgt == r->open_dest)
    {
        r->tgt = 0;
    }

    r->vel = -r->vel;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool RotatorSwitch(short match, short setting)
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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SetRotatorActive(DSWActor* actor)
{
    ROTATOR* r;

    r = actor->user.rotator.Data();

    DoRotatorSetInterp(actor);

    // play activate sound
    DoSoundSpotMatch(SP_TAG2(actor), 1, SOUND_OBJECT_TYPE);

    actor->user.Flags |= (SPR_ACTIVE);
    actor->user.Tics = 0;

    // moving to the OFF position
    if (r->tgt == 0)
        VatorSwitch(SP_TAG2(actor), false);
    else
        VatorSwitch(SP_TAG2(actor), true);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SetRotatorInactive(DSWActor* actor)
{
    DoRotatorStopInterp(actor);

    // play inactivate sound
    DoSoundSpotMatch(SP_TAG2(actor), 2, SOUND_OBJECT_TYPE);

    actor->user.Flags &= ~(SPR_ACTIVE);
}

//---------------------------------------------------------------------------
//
// called for operation from the space bar
//
//---------------------------------------------------------------------------

void DoRotatorOperate(PLAYER* pp, sectortype* sect)
{
    short match = sect->hitag;

    if (match > 0)
    {
        if (!TestRotatorMatchActive(match))
            DoRotatorMatch(pp, match, true);
    }
}

//---------------------------------------------------------------------------
//
// called from switches and triggers
// returns first vator found
//
//---------------------------------------------------------------------------

void DoRotatorMatch(PLAYER* pp, short match, bool manual)
{
    DSWActor* firstVator = nullptr;

    //RotatorSwitch(match, ON);

    SWStatIterator it(STAT_ROTATOR);
    while (auto actor = it.Next())
    {
        if (SP_TAG1(actor) == SECT_ROTATOR && SP_TAG2(actor) == match)
        {
            // single play only vator
            // bool 8 must be set for message to display
            if (TEST_BOOL4(actor) && (gNet.MultiGameType == MULTI_GAME_COMMBAT || gNet.MultiGameType == MULTI_GAME_AI_BOTS))
            {
                if (pp && TEST_BOOL11(actor)) PutStringInfo(pp, GStrings("TXT_SPONLY"));
                continue;
            }

            // switch trigger only
            if (SP_TAG3(actor) == 1)
            {
                // tried to manually operat a switch/trigger only
                if (manual)
                    continue;
            }

            if (firstVator == nullptr)
                firstVator = actor;

            auto sect = actor->sector();

            if (pp && sect->hasU() && sect->stag == SECT_LOCK_DOOR && sect->number)
            {
                int key_num = sect->number;

                {
                    PutStringInfo(pp, quoteMgr.GetQuote(QUOTE_DOORMSG + key_num - 1));
                    return;
                }
            }

            if (actor->user.Flags & (SPR_ACTIVE))
            {
                ReverseRotator(actor);
                continue;
            }

            SetRotatorActive(actor);
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool TestRotatorMatchActive(short match)
{
    SWStatIterator it(STAT_ROTATOR);
    while (auto actor = it.Next())
    {
        if (SP_TAG1(actor) == SECT_ROTATOR && SP_TAG2(actor) == match)
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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoRotatorSetInterp(DSWActor* actor)
{
    for(auto& wal : wallsofsector(actor->sector()))
    {
        StartInterpolation(&wal, Interp_Wall_X);
        StartInterpolation(&wal, Interp_Wall_Y);

        if (wal.twoSided())
        {
            auto w2 = wal.nextWall()->point2Wall();
            StartInterpolation(w2, Interp_Wall_X);
            StartInterpolation(w2, Interp_Wall_Y);
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoRotatorStopInterp(DSWActor* actor)
{
    for (auto& wal : wallsofsector(actor->sector()))
    {
        StopInterpolation(&wal, Interp_Wall_X);
        StopInterpolation(&wal, Interp_Wall_Y);

        if (wal.twoSided())
        {
            auto w2 = wal.nextWall()->point2Wall();
            StopInterpolation(w2, Interp_Wall_X);
            StopInterpolation(w2, Interp_Wall_Y);
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoRotator(DSWActor* actor)
{
    ROTATOR* r;
    short ndx,w,startwall,endwall;
    DSWActor* pivot = nullptr;
    bool kill = false;

    r = actor->user.rotator.Data();

    // Example - ang pos moves from 0 to 512 <<OR>> from 0 to -512

    // control SPEED of swinging
    if (r->pos < r->tgt)
    {
        // Increment swing angle
        r->pos += r->speed;
        r->speed += r->vel;

        // if the other way make it equal
        if (r->pos > r->tgt)
            r->pos = r->tgt;
    }

    if (r->pos > r->tgt)
    {
        // Increment swing angle
        r->pos -= r->speed;
        r->speed += r->vel;

        // if the other way make it equal
        if (r->pos < r->tgt)
            r->pos = r->tgt;
    }

    if (r->pos == r->tgt)
    {
        // If ang is OPEN
        if (r->pos == r->open_dest)
        {
            // new tgt is CLOSED (0)
            r->tgt = 0;
            r->vel = -r->vel;
            SetRotatorInactive(actor);

            if (SP_TAG6(actor))
                DoMatchEverything(nullptr, SP_TAG6(actor), -1);

            // wait a bit and close it
            if (actor->user.WaitTics)
                actor->user.Tics = actor->user.WaitTics;
        }
        else
        // If ang is CLOSED then
        if (r->pos == 0)
        {
            short match = SP_TAG2(actor);

            // new tgt is OPEN (open)
            r->tgt = r->open_dest;
            r->speed = r->orig_speed;
            r->vel = abs(r->vel);

            SetRotatorInactive(actor);

            // set Owner swith back to OFF
            // only if ALL vators are inactive
            if (!TestRotatorMatchActive(match))
            {
                //RotatorSwitch(match, OFF);
            }

            if (SP_TAG6(actor) && TEST_BOOL5(actor))
                DoMatchEverything(nullptr, SP_TAG6(actor), -1);
        }

        if (TEST_BOOL2(actor))
            kill = true;
    }

    double closest = 99999;
    SWStatIterator it(STAT_ROTATOR_PIVOT);
    while (auto itActor = it.Next())
    {
        if (itActor->spr.lotag == actor->spr.lotag)
        {
			double dist = (actor->spr.pos.XY() - itActor->spr.pos.XY()).Length();
            if (dist < closest)
            {
                closest = dist;
                pivot = itActor;
            }
        }
    }

    if (!pivot)
        return 0;

    // move points
    ndx = 0;
    for(auto& wal : wallsofsector(actor->sector()))
    {
        auto nxy = rotatepoint(pivot->spr.pos, r->orig[ndx], DAngle::fromBuild(r->pos));

        dragpoint(&wal, nxy);
        ndx++;
    }

    if (kill)
    {
        SetRotatorInactive(actor);
        KillActor(actor);
        return 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

#include "saveable.h"

static saveable_code saveable_rotator_code[] =
{
    SAVE_CODE(DoRotator)
};

saveable_module saveable_rotator =
{
    // code
    saveable_rotator_code,
    SIZ(saveable_rotator_code),

    // data
    nullptr,0
};

END_SW_NS
