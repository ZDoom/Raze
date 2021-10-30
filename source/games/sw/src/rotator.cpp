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

short DoRotatorMatch(PLAYERp pp, short match, bool);
bool TestRotatorMatchActive(short match);
void InterpSectorSprites(short sectnum, bool state);
void DoMatchEverything(PLAYERp pp, short match, short state);
void DoRotatorSetInterp(short SpriteNum);
void DoRotatorStopInterp(short SpriteNum);

void ReverseRotator(short SpriteNum)
{
    USERp u = User[SpriteNum].Data();
    ROTATORp r;

    r = u->rotator.Data();

    // if paused go ahead and start it up again
    if (u->Tics)
    {
        u->Tics = 0;
        SetRotatorActive(SpriteNum);
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

bool
RotatorSwitch(short match, short setting)
{
    SPRITEp sp;
    int i;
    bool found = false;

    StatIterator it(STAT_DEFAULT);
    while ((i = it.NextIndex()) >= 0)
    {
        sp = &sprite[i];

        if (sp->lotag == TAG_SPRITE_SWITCH_VATOR && sp->hitag == match)
        {
            found = true;
            AnimateSwitch(sp, setting);
        }
    }

    return found;
}

void SetRotatorActive(short SpriteNum)
{
    USERp u = User[SpriteNum].Data();
    SPRITEp sp = u->SpriteP;
    ROTATORp r;

    r = u->rotator.Data();

    DoRotatorSetInterp(SpriteNum);

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

void SetRotatorInactive(short SpriteNum)
{
    USERp u = User[SpriteNum].Data();
    SPRITEp sp = u->SpriteP;

    DoRotatorStopInterp(SpriteNum);

    // play inactivate sound
    DoSoundSpotMatch(SP_TAG2(sp), 2, SOUND_OBJECT_TYPE);

    RESET(u->Flags, SPR_ACTIVE);
}

// called for operation from the space bar
short DoRotatorOperate(PLAYERp pp, short sectnum)
{
    short match;

    match = sector[sectnum].hitag;

    if (match > 0)
    {
        if (TestRotatorMatchActive(match))
            return -1;
        else
            return DoRotatorMatch(pp, match, true);
    }

    return -1;
}

// called from switches and triggers
// returns first vator found
short
DoRotatorMatch(PLAYERp pp, short match, bool manual)
{
    USERp fu;
    SPRITEp fsp;
    short sectnum;
    short first_vator = -1;

    int i;

    //RotatorSwitch(match, ON);

    StatIterator it(STAT_ROTATOR);
    while ((i = it.NextIndex()) >= 0)
    {
        fsp = &sprite[i];

        if (SP_TAG1(fsp) == SECT_ROTATOR && SP_TAG2(fsp) == match)
        {
            fu = User[i].Data();

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

            if (first_vator == -1)
                first_vator = i;

            sectnum = fsp->sectnum;

            if (pp && SectUser[sectnum].Data() && SectUser[sectnum]->stag == SECT_LOCK_DOOR && SectUser[sectnum]->number)
            {
                short key_num;

                key_num = SectUser[sectnum]->number;

#if 0
                if (pp->HasKey[key_num - 1])
                {
                    int i;
                    for (i=0; i<numsectors; i++)
                    {
                        if (SectUser[i] && SectUser[i]->stag == SECT_LOCK_DOOR && SectUser[i]->number == key_num)
                            SectUser[i]->number = 0;  // unlock all doors of this type
                    }
                    UnlockKeyLock(key_num);
                }
                else
#endif
                {
                    PutStringInfo(pp, quoteMgr.GetQuote(QUOTE_DOORMSG + key_num - 1));
                    return -1;
                }
            }

            if (TEST(fu->Flags, SPR_ACTIVE))
            {
                ReverseRotator(i);
                continue;
            }

            SetRotatorActive(i);
        }
    }

    return first_vator;
}


bool
TestRotatorMatchActive(short match)
{
    USERp fu;
    SPRITEp fsp;

    int i;

    StatIterator it(STAT_ROTATOR);
    while ((i = it.NextIndex()) >= 0)
    {
        fsp = &sprite[i];

        if (SP_TAG1(fsp) == SECT_ROTATOR && SP_TAG2(fsp) == match)
        {
            fu = User[i].Data();

            // Does not have to be inactive to be operated
            if (TEST_BOOL6(fsp))
                continue;

            if (TEST(fu->Flags, SPR_ACTIVE) || fu->Tics)
                return true;
        }
    }

    return false;
}


void DoRotatorSetInterp(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
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

void DoRotatorStopInterp(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
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

int DoRotatorMove(short SpriteNum)
{
    auto actor = &swActors[SpriteNum];
    USERp u = actor->u();
    SPRITEp sp = &actor->s();
    ROTATORp r;
    short ndx,w,startwall,endwall;
    SPRITEp pivot = nullptr;
    int i;
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
            SetRotatorInactive(SpriteNum);

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

            SetRotatorInactive(SpriteNum);

            // set owner swith back to OFF
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
    StatIterator it(STAT_ROTATOR_PIVOT);
    while ((i = it.NextIndex()) >= 0)
    {
        if (sprite[i].lotag == sp->lotag)
        {
            dist = Distance(sp->x, sp->y, sprite[i].x, sprite[i].y);
            if (dist < closest)
            {
                closest = dist;
                pivot = &sprite[i];
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

        dragpoint(w, nxy.x, nxy.y, 0);
        ndx++;
    }

    if (kill)
    {
        SetRotatorInactive(SpriteNum);
        KillActor(actor);
        return 0;
    }

    return 0;
}

int DoRotator(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;

    // could move this inside sprite control
    DoRotatorMove(SpriteNum);

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
