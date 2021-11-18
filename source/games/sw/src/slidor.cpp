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
#include "quotemgr.h"

BEGIN_SW_NS

void ReverseSlidor(DSWActor* actor)
{
    USERp u = actor->u();
    ROTATORp r;

    r = u->rotator.Data();

    // if paused go ahead and start it up again
    if (u->Tics)
    {
        u->Tics = 0;
        SetSlidorActive(actor);
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


bool SlidorSwitch(short match, short setting)
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

void SetSlidorActive(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();
    ROTATORp r;

    r = u->rotator.Data();

    DoSlidorInterp(actor, StartInterpolation);

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

void SetSlidorInactive(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();

    DoSlidorInterp(actor, StopInterpolation);

    // play inactivate sound
    DoSoundSpotMatch(SP_TAG2(sp), 2, SOUND_OBJECT_TYPE);

    RESET(u->Flags, SPR_ACTIVE);
}

// called for operation from the space bar
void DoSlidorOperate(PLAYERp pp, short sectnum)
{
    short match;

    match = sector[sectnum].hitag;


    if (match > 0)
    {
        if (!TestSlidorMatchActive(match))
            DoSlidorMatch(pp, match, true);
    }
}

// called from switches and triggers
// returns first vator found
void DoSlidorMatch(PLAYERp pp, short match, bool manual)
{
    USERp fu;
    SPRITEp fsp;
    short sectnum;

    SWStatIterator it(STAT_SLIDOR);
    while (auto actor = it.Next())
    {
        fsp = &actor->s();

        if (SP_TAG1(fsp) == SECT_SLIDOR && SP_TAG2(fsp) == match)
        {
            fu = actor->u();

            // single play only vator
            // bool 8 must be set for message to display
            if (TEST_BOOL4(fsp) && (gNet.MultiGameType == MULTI_GAME_COMMBAT || gNet.MultiGameType == MULTI_GAME_AI_BOTS))
            {
                if (pp && TEST_BOOL11(fsp)) PutStringInfo(pp, GStrings("TXTS_SPONLY"));
                continue;
            }

            // switch trigger only
            if (SP_TAG3(fsp) == 1)
            {
                // tried to manually operat a switch/trigger only
                if (manual)
                    continue;
            }

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
                ReverseSlidor(actor);
                continue;
            }

            SetSlidorActive(actor);
        }
    }
}


bool TestSlidorMatchActive(short match)
{
    USERp fu;
    SPRITEp fsp;

    SWStatIterator it(STAT_SLIDOR);
    while (auto actor = it.Next())
    {
        fsp = &actor->s();

        if (SP_TAG1(fsp) == SECT_SLIDOR && SP_TAG2(fsp) == match)
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

void DoSlidorInterp(DSWActor* actor, INTERP_FUNC interp_func)
{
    auto sp = &actor->s();
    short w, pw, startwall, endwall;

    w = startwall = sector[sp->sectnum].wallptr;
    endwall = startwall + sector[sp->sectnum].wallnum - 1;

    do
    {
        switch (wall[w].lotag)
        {
        case TAG_WALL_SLIDOR_LEFT:
        {
            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            uint16_t const nextwall = wall[w].nextwall;
            if (!validWallIndex(nextwall))
            {
                // white wall - move 4 points
                interp_func(w, Interp_Wall_X);
                interp_func(pw, Interp_Wall_X);
                interp_func(wall[w].point2, Interp_Wall_X);
                interp_func(wall[wall[w].point2].point2, Interp_Wall_X);
            }
            else
            {
                // red wall - move 2 points
                interp_func(w, Interp_Wall_X);
                interp_func(wall[nextwall].point2, Interp_Wall_X);
                interp_func(wall[w].point2, Interp_Wall_X);
                interp_func(wall[wall[wall[w].point2].nextwall].point2, Interp_Wall_X);
            }

            break;
        }

        case TAG_WALL_SLIDOR_RIGHT:
        {
            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            uint16_t const nextwall = wall[w].nextwall;
            if (!validWallIndex(nextwall))
            {
                // white wall - move 4 points
                interp_func(w, Interp_Wall_X);
                interp_func(pw, Interp_Wall_X);
                interp_func(wall[w].point2, Interp_Wall_X);
                interp_func(wall[wall[w].point2].point2, Interp_Wall_X);
            }
            else
            {
                // red wall - move 2 points
                interp_func(w, Interp_Wall_X);
                interp_func(wall[nextwall].point2, Interp_Wall_X);
                interp_func(wall[w].point2, Interp_Wall_X);
                interp_func(wall[wall[wall[w].point2].nextwall].point2, Interp_Wall_X);
            }

            break;
        }

        case TAG_WALL_SLIDOR_UP:
        {
            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            uint16_t const nextwall = wall[w].nextwall;
            if (!validWallIndex(nextwall))
            {
                interp_func(w, Interp_Wall_Y);
                interp_func(pw, Interp_Wall_Y);
                interp_func(wall[w].point2, Interp_Wall_Y);
                interp_func(wall[wall[w].point2].point2, Interp_Wall_Y);
            }
            else
            {
                interp_func(w, Interp_Wall_Y);
                interp_func(wall[nextwall].point2, Interp_Wall_Y);
                interp_func(wall[w].point2, Interp_Wall_Y);
                interp_func(wall[wall[wall[w].point2].nextwall].point2, Interp_Wall_Y);
            }

            break;
        }

        case TAG_WALL_SLIDOR_DOWN:
        {
            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            uint16_t const nextwall = wall[w].nextwall;
            if (!validWallIndex(nextwall))
            {
                interp_func(w, Interp_Wall_Y);
                interp_func(pw, Interp_Wall_Y);
                interp_func(wall[w].point2, Interp_Wall_Y);
                interp_func(wall[wall[w].point2].point2, Interp_Wall_Y);
            }
            else
            {
                interp_func(w, Interp_Wall_Y);
                interp_func(wall[nextwall].point2, Interp_Wall_Y);
                interp_func(wall[w].point2, Interp_Wall_Y);
                interp_func(wall[wall[wall[w].point2].nextwall].point2, Interp_Wall_Y);
            }

            break;
        }
        }

        w = wall[w].point2;
    }
    while (w != startwall);
}

int DoSlidorMoveWalls(DSWActor* actor, int amt)
{
    auto sp = &actor->s();
    short w, pw, startwall, endwall;

    w = startwall = sector[sp->sectnum].wallptr;
    endwall = startwall + sector[sp->sectnum].wallnum - 1;

    do
    {
        switch (wall[w].lotag)
        {
        case TAG_WALL_SLIDOR_LEFT:

            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            if (!validWallIndex(wall[w].nextwall))
            {
                // white wall - move 4 points
                wall[w].x -= amt;
                wall[pw].x -= amt;
                wall[wall[w].point2].x -= amt;
                wall[wall[wall[w].point2].point2].x -= amt;
            }
            else
            {
                // red wall - move 2 points
                dragpoint(w, wall[w].x - amt, wall[w].y);
                dragpoint(wall[w].point2, wall[wall[w].point2].x - amt, wall[wall[w].point2].y);
            }

            break;

        case TAG_WALL_SLIDOR_RIGHT:

            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            if (!validWallIndex(wall[w].nextwall))
            {
                // white wall - move 4 points
                wall[w].x += amt;
                wall[pw].x += amt;
                wall[wall[w].point2].x += amt;
                wall[wall[wall[w].point2].point2].x += amt;
            }
            else
            {
                // red wall - move 2 points
                dragpoint(w, wall[w].x + amt, wall[w].y);
                dragpoint(wall[w].point2, wall[wall[w].point2].x + amt, wall[wall[w].point2].y);
            }

            break;

        case TAG_WALL_SLIDOR_UP:

            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            if (!validWallIndex(wall[w].nextwall))
            {
                wall[w].y -= amt;
                wall[pw].y -= amt;
                wall[wall[w].point2].y -= amt;
                wall[wall[wall[w].point2].point2].y -= amt;
            }
            else
            {
                dragpoint(w, wall[w].x, wall[w].y - amt);
                dragpoint(wall[w].point2, wall[wall[w].point2].x, wall[wall[w].point2].y - amt);
            }

            break;

        case TAG_WALL_SLIDOR_DOWN:

            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            if (!validWallIndex(wall[w].nextwall))
            {
                wall[w].y += amt;
                wall[pw].y += amt;
                wall[wall[w].point2].y += amt;
                wall[wall[wall[w].point2].point2].y += amt;
            }
            else
            {
                dragpoint(w, wall[w].x, wall[w].y + amt);
                dragpoint(wall[w].point2, wall[wall[w].point2].x, wall[wall[w].point2].y + amt);
            }


            break;
        }

        w = wall[w].point2;
    }
    while (w != startwall);

    return 0;
}

int DoSlidorInstantClose(DSWActor* actor)
{
    SPRITEp sp = &actor->s();
    short w, startwall;
    int diff;

    w = startwall = sector[sp->sectnum].wallptr;

    do
    {
        switch (wall[w].lotag)
        {
        case TAG_WALL_SLIDOR_LEFT:
            diff = wall[w].x - sp->x;
            DoSlidorMoveWalls(actor, diff);
            break;

        case TAG_WALL_SLIDOR_RIGHT:
            diff = wall[w].x - sp->x;
            DoSlidorMoveWalls(actor, -diff);
            break;

        case TAG_WALL_SLIDOR_UP:
            diff = wall[w].y - sp->y;
            DoSlidorMoveWalls(actor, diff);
            break;

        case TAG_WALL_SLIDOR_DOWN:
            diff = wall[w].y - sp->y;
            DoSlidorMoveWalls(actor, -diff);
            break;
        }

        w = wall[w].point2;
    }
    while (w != startwall);

    return 0;
}


int DoSlidor(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();
    ROTATORp r;
    int old_pos;
    bool kill = false;

    r = u->rotator.Data();

    // Example - ang pos moves from 0 to 512 <<OR>> from 0 to -512

    old_pos = r->pos;

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
            SetSlidorInactive(actor);

            if (SP_TAG6(sp) && !TEST_BOOL8(sp))
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
            r->speed = r->orig_speed;
            r->vel = labs(r->vel);

            r->tgt = r->open_dest;
            SetSlidorInactive(actor);

            RESET_BOOL8(sp);

            // set Owner swith back to OFF
            // only if ALL vators are inactive
            if (!TestSlidorMatchActive(match))
            {
                //SlidorSwitch(match, OFF);
            }

            if (SP_TAG6(sp) && TEST_BOOL8(sp))
                DoMatchEverything(nullptr, SP_TAG6(sp), -1);
        }

        if (TEST_BOOL2(sp))
            kill = true;
    }
    else
    {
        // if heading for the OFF (original) position and should NOT CRUSH
        if (TEST_BOOL3(sp) && r->tgt == 0)
        {
            SPRITEp bsp;
            USERp bu;
            bool found = false;

            SWSectIterator it(sp->sectnum);
            while (auto itActor = it.Next())
            {
                bsp = &itActor->s();
                bu = itActor->u();

                if (bu && TEST(bsp->cstat, CSTAT_SPRITE_BLOCK) && TEST(bsp->extra, SPRX_PLAYER_OR_ENEMY))
                {
                    // found something blocking so reverse to ON position
                    ReverseSlidor(actor);
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
                        ReverseSlidor(actor);

                        u->vel_rate = -u->vel_rate;
                        found = true;
                    }
                }
            }
        }
    }


    DoSlidorMoveWalls(actor, r->pos - old_pos);

    if (kill)
    {
        SetSlidorInactive(actor);
        KillActor(actor);
        return 0;
    }

    return 0;
}

#include "saveable.h"

static saveable_code saveable_slidor_code[] =
{
    SAVE_CODE(DoSlidor),
};

saveable_module saveable_slidor =
{
    // code
    saveable_slidor_code,
    SIZ(saveable_slidor_code),

    // data
    nullptr,0
};
END_SW_NS
