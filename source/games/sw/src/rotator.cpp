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

void DoRotatorMatch(PLAYERp pp, short match, bool);
bool TestRotatorMatchActive(short match);
void InterpSectorSprites(short sectnum, bool state);
void DoMatchEverything(PLAYERp pp, short match, short state);
void DoRotatorSetInterp(DSWActor*);
void DoRotatorStopInterp(DSWActor*);

void ReverseRotator(DSWActor* actor)
{
    USERp u = actor->u();
    ROTATORp r;

    r = u->rotator.Data();

    // if paused go ahead and start it up again
    if (u->Tics)
    {
        u->Tics = 0;
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

bool RotatorSwitch(short match, short setting)
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

void SetRotatorActive(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();
    ROTATORp r;

    r = u->rotator.Data();

    DoRotatorSetInterp(actor);

    // play activate sound
    DoSoundSpotMatch(SP_TAG2(sp), 1, SOUND_OBJECT_TYPE);

    SET(u->Flags, SPR_ACTIVE);
    u->Tics = 0;

    // moving to the OFF position
    if (r->tgt == 0)
        VatorSwitch(SP_TAG2(sp), OFF);
    else
        VatorSwitch(SP_TAG2(sp), ON);
}

void SetRotatorInactive(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();

    DoRotatorStopInterp(actor);

    // play inactivate sound
    DoSoundSpotMatch(SP_TAG2(sp), 2, SOUND_OBJECT_TYPE);

    RESET(u->Flags, SPR_ACTIVE);
}

// called for operation from the space bar
void DoRotatorOperate(PLAYERp pp, short sectnum)
{
    short match = sector[sectnum].hitag;

    if (match > 0)
    {
        if (!TestRotatorMatchActive(match))
            DoRotatorMatch(pp, match, true);
    }
}

// called from switches and triggers
// returns first vator found
void DoRotatorMatch(PLAYERp pp, short match, bool manual)
{
    USERp fu;
    SPRITEp fsp;
    short sectnum;
    DSWActor* firstVator = nullptr;

    //RotatorSwitch(match, ON);

    SWStatIterator it(STAT_ROTATOR);
    while (auto actor = it.Next())
    {
        fsp = &actor->s();

        if (SP_TAG1(fsp) == SECT_ROTATOR && SP_TAG2(fsp) == match)
        {
            fu = actor->u();

            // single play only vator
            // bool 8 must be set for message to display
            if (TEST_BOOL4(fsp) && (gNet.MultiGameType == MULTI_GAME_COMMBAT || gNet.MultiGameType == MULTI_GAME_AI_BOTS))
            {
                if (pp && TEST_BOOL11(fsp)) PutStringInfo(pp, GStrings("TXT_SPONLY"));
                continue;
            }

            // switch trigger only
            if (SP_TAG3(fsp) == 1)
            {
                // tried to manually operat a switch/trigger only
                if (manual)
                    continue;
            }

            if (firstVator == nullptr)
                firstVator = actor;

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

            if (TEST(fu->Flags, SPR_ACTIVE))
            {
                ReverseRotator(actor);
                continue;
            }

            SetRotatorActive(actor);
        }
    }
}


bool TestRotatorMatchActive(short match)
{
    USERp fu;
    SPRITEp fsp;

    SWStatIterator it(STAT_ROTATOR);
    while (auto actor = it.Next())
    {
        fsp = &actor->s();

        if (SP_TAG1(fsp) == SECT_ROTATOR && SP_TAG2(fsp) == match)
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


void DoRotatorSetInterp(DSWActor* actor)
{
    SPRITEp sp = &actor->s();
    short w,startwall,endwall;

    startwall = sector[sp->sectnum].wallptr;
    endwall = startwall + sector[sp->sectnum].wallnum - 1;

    // move points
    for (w = startwall; w <= endwall; w++)
    {
        StartInterpolation(w, Interp_Wall_X);
        StartInterpolation(w, Interp_Wall_Y);

        uint16_t const nextwall = wall[w].nextwall;
        if (validWallIndex(nextwall))
        {
            StartInterpolation(wall[nextwall].point2, Interp_Wall_X);
            StartInterpolation(wall[nextwall].point2, Interp_Wall_Y);
        }
    }
}

void DoRotatorStopInterp(DSWActor* actor)
{
    SPRITEp sp = &actor->s();
    short w,startwall,endwall;

    startwall = sector[sp->sectnum].wallptr;
    endwall = startwall + sector[sp->sectnum].wallnum - 1;

    // move points
    for (w = startwall; w <= endwall; w++)
    {
        StopInterpolation(w, Interp_Wall_X);
        StopInterpolation(w, Interp_Wall_Y);

        uint16_t const nextwall = wall[w].nextwall;
        if (validWallIndex(nextwall))
        {
            StopInterpolation(wall[nextwall].point2, Interp_Wall_X);
            StopInterpolation(wall[nextwall].point2, Interp_Wall_Y);
        }
    }
}

int DoRotator(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();
    ROTATORp r;
    short ndx,w,startwall,endwall;
    SPRITEp pivot = nullptr;
    vec2_t nxy;
    int dist,closest;
    bool kill = false;

    r = u->rotator.Data();

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

            if (SP_TAG6(sp))
                DoMatchEverything(nullptr, SP_TAG6(sp), -1);

            // wait a bit and close it
            if (u->WaitTics)
                u->Tics = u->WaitTics;
        }
        else
        // If ang is CLOSED then
        if (r->pos == 0)
        {
            short match = SP_TAG2(sp);

            // new tgt is OPEN (open)
            r->tgt = r->open_dest;
            r->speed = r->orig_speed;
            r->vel = labs(r->vel);

            SetRotatorInactive(actor);

            // set Owner swith back to OFF
            // only if ALL vators are inactive
            if (!TestRotatorMatchActive(match))
            {
                //RotatorSwitch(match, OFF);
            }

            if (SP_TAG6(sp) && TEST_BOOL5(sp))
                DoMatchEverything(nullptr, SP_TAG6(sp), -1);
        }

        if (TEST_BOOL2(sp))
            kill = true;
    }

    closest = 99999;
    SWStatIterator it(STAT_ROTATOR_PIVOT);
    while (auto itActor = it.Next())
    {
        auto itsp = &itActor->s();
        if (itsp->lotag == sp->lotag)
        {
            dist = Distance(sp->x, sp->y, itsp->x, itsp->y);
            if (dist < closest)
            {
                closest = dist;
                pivot = itsp;
            }
        }
    }

    if (!pivot)
        return 0;

    startwall = sector[sp->sectnum].wallptr;
    endwall = startwall + sector[sp->sectnum].wallnum - 1;

    // move points
    for (w = startwall, ndx = 0; w <= endwall; w++)
    {
        vec2_t const orig = { r->origX[ndx], r->origY[ndx] };
        rotatepoint(pivot->pos.vec2, orig, r->pos, &nxy);

        dragpoint(w, nxy.x, nxy.y);
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
