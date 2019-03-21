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

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "network.h"
#include "tags.h"
#include "slidor.h"
#include "sector.h"
#include "interp.h"
#include "text.h"
#include "sprite.h"

void ReverseSlidor(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    ROTATORp r;

    r = u->rotator;

    // if paused go ahead and start it up again
    if (u->Tics)
    {
        u->Tics = 0;
        SetSlidorActive(SpriteNum);
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


SWBOOL
SlidorSwitch(short match, short setting)
{
    SPRITEp sp;
    short i,nexti;
    SWBOOL found = FALSE;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_DEFAULT], i, nexti)
    {
        sp = &sprite[i];

        if (sp->lotag == TAG_SPRITE_SWITCH_VATOR && sp->hitag == match)
        {
            found = TRUE;
            AnimateSwitch(sp, setting);
        }
    }

    return found;
}

void SetSlidorActive(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SECTORp sectp = &sector[sp->sectnum];
    ROTATORp r;

    r = u->rotator;

    DoSlidorInterp(SpriteNum, setinterpolation);

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

void SetSlidorInactive(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SECTORp sectp = &sector[sp->sectnum];

    DoSlidorInterp(SpriteNum, stopinterpolation);

    // play inactivate sound
    DoSoundSpotMatch(SP_TAG2(sp), 2, SOUND_OBJECT_TYPE);

    RESET(u->Flags, SPR_ACTIVE);
}

// called for operation from the space bar
short DoSlidorOperate(PLAYERp pp, short sectnum)
{
    USERp fu;
    SPRITEp fsp;
    short match;
    short i,nexti;

    match = sector[sectnum].hitag;


    if (match > 0)
    {
        if (TestSlidorMatchActive(match))
            return -1;
        else
            return DoSlidorMatch(pp, match, TRUE);
    }

    return -1;
}

// called from switches and triggers
// returns first vator found
short
DoSlidorMatch(PLAYERp pp, short match, SWBOOL manual)
{
    USERp fu;
    SPRITEp fsp;
    short sectnum;
    short first_vator = -1;

    short i,nexti;

    //SlidorSwitch(match, ON);

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SLIDOR], i, nexti)
    {
        fsp = &sprite[i];

        if (SP_TAG1(fsp) == SECT_SLIDOR && SP_TAG2(fsp) == match)
        {
            fu = User[i];

            // single play only vator
            // SWBOOL 8 must be set for message to display
            if (TEST_BOOL4(fsp) && (gNet.MultiGameType == MULTI_GAME_COMMBAT || gNet.MultiGameType == MULTI_GAME_AI_BOTS))
            {
                if (pp && TEST_BOOL11(fsp)) PutStringInfo(pp,"This only opens in single play.");
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

            if (pp && SectUser[sectnum] && SectUser[sectnum]->stag == SECT_LOCK_DOOR && SectUser[sectnum]->number)
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
                    PutStringInfo(pp, KeyDoorMessage[key_num - 1]);
                    return -1;
                }
            }

            if (TEST(fu->Flags, SPR_ACTIVE))
            {
                ReverseSlidor(i);
                continue;
            }

            SetSlidorActive(i);
        }
    }

    return first_vator;
}


SWBOOL
TestSlidorMatchActive(short match)
{
    USERp fu;
    SPRITEp fsp;
    short sectnum;

    short i,nexti;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SLIDOR], i, nexti)
    {
        fsp = &sprite[i];

        if (SP_TAG1(fsp) == SECT_SLIDOR && SP_TAG2(fsp) == match)
        {
            fu = User[i];

            // Does not have to be inactive to be operated
            if (TEST_BOOL6(fsp))
                continue;

            if (TEST(fu->Flags, SPR_ACTIVE) || fu->Tics)
                return TRUE;
        }
    }

    return FALSE;
}

void DoSlidorInterp(short SpriteNum, INTERP_FUNCp interp_func)
{
    short w, pw, startwall, endwall;

    w = startwall = sector[sprite[SpriteNum].sectnum].wallptr;
    endwall = startwall + sector[sprite[SpriteNum].sectnum].wallnum - 1;

    do
    {
        switch (wall[w].lotag)
        {
        case TAG_WALL_SLIDOR_LEFT:

            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            if (wall[w].nextwall < 0)
            {
                // white wall - move 4 points
                interp_func(&wall[w].x);
                interp_func(&wall[pw].x);
                interp_func(&wall[wall[w].point2].x);
                interp_func(&wall[wall[wall[w].point2].point2].x);
            }
            else
            {
                // red wall - move 2 points
                interp_func(&wall[w].x);
                interp_func(&wall[DRAG_WALL(w)].x);
                interp_func(&wall[wall[w].point2].x);
                interp_func(&wall[DRAG_WALL(wall[w].point2)].x);
            }

            break;

        case TAG_WALL_SLIDOR_RIGHT:

            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            if (wall[w].nextwall < 0)
            {
                // white wall - move 4 points
                interp_func(&wall[w].x);
                interp_func(&wall[pw].x);
                interp_func(&wall[wall[w].point2].x);
                interp_func(&wall[wall[wall[w].point2].point2].x);
            }
            else
            {
                // red wall - move 2 points
                interp_func(&wall[w].x);
                interp_func(&wall[DRAG_WALL(w)].x);
                interp_func(&wall[wall[w].point2].x);
                interp_func(&wall[DRAG_WALL(wall[w].point2)].x);
            }

            break;

        case TAG_WALL_SLIDOR_UP:

            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            if (wall[w].nextwall < 0)
            {
                interp_func(&wall[w].y);
                interp_func(&wall[pw].y);
                interp_func(&wall[wall[w].point2].y);
                interp_func(&wall[wall[wall[w].point2].point2].y);
            }
            else
            {
                interp_func(&wall[w].y);
                interp_func(&wall[DRAG_WALL(w)].y);
                interp_func(&wall[wall[w].point2].y);
                interp_func(&wall[DRAG_WALL(wall[w].point2)].y);
            }

            break;

        case TAG_WALL_SLIDOR_DOWN:

            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            if (wall[w].nextwall < 0)
            {
                interp_func(&wall[w].y);
                interp_func(&wall[pw].y);
                interp_func(&wall[wall[w].point2].y);
                interp_func(&wall[wall[wall[w].point2].point2].y);
            }
            else
            {
                interp_func(&wall[w].y);
                interp_func(&wall[DRAG_WALL(w)].y);
                interp_func(&wall[wall[w].point2].y);
                interp_func(&wall[DRAG_WALL(wall[w].point2)].y);
            }


            break;
        }

        w = wall[w].point2;
    }
    while (w != startwall);
}

int DoSlidorMoveWalls(short SpriteNum, int amt)
{
    short w, pw, startwall, endwall;

    w = startwall = sector[sprite[SpriteNum].sectnum].wallptr;
    endwall = startwall + sector[sprite[SpriteNum].sectnum].wallnum - 1;

    do
    {
        switch (wall[w].lotag)
        {
        case TAG_WALL_SLIDOR_LEFT:

            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            if (wall[w].nextwall < 0)
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
                dragpoint(w, wall[w].x - amt, wall[w].y, 0);
                dragpoint(wall[w].point2, wall[wall[w].point2].x - amt, wall[wall[w].point2].y, 0);
            }

            break;

        case TAG_WALL_SLIDOR_RIGHT:

            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            if (wall[w].nextwall < 0)
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
                dragpoint(w, wall[w].x + amt, wall[w].y, 0);
                dragpoint(wall[w].point2, wall[wall[w].point2].x + amt, wall[wall[w].point2].y, 0);
            }

            break;

        case TAG_WALL_SLIDOR_UP:

            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            if (wall[w].nextwall < 0)
            {
                wall[w].y -= amt;
                wall[pw].y -= amt;
                wall[wall[w].point2].y -= amt;
                wall[wall[wall[w].point2].point2].y -= amt;
            }
            else
            {
                dragpoint(w, wall[w].x, wall[w].y - amt, 0);
                dragpoint(wall[w].point2, wall[wall[w].point2].x, wall[wall[w].point2].y - amt, 0);
            }

            break;

        case TAG_WALL_SLIDOR_DOWN:

            // prev wall
            pw = w - 1;
            if (w < startwall)
                pw = endwall;

            if (wall[w].nextwall < 0)
            {
                wall[w].y += amt;
                wall[pw].y += amt;
                wall[wall[w].point2].y += amt;
                wall[wall[wall[w].point2].point2].y += amt;
            }
            else
            {
                dragpoint(w, wall[w].x, wall[w].y + amt, 0);
                dragpoint(wall[w].point2, wall[wall[w].point2].x, wall[wall[w].point2].y + amt, 0);
            }


            break;
        }

        w = wall[w].point2;
    }
    while (w != startwall);

    return 0;
}

int DoSlidorInstantClose(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    short w, pw, startwall, endwall;
    int diff;

    w = startwall = sector[sprite[SpriteNum].sectnum].wallptr;
    endwall = startwall + sector[sprite[SpriteNum].sectnum].wallnum - 1;

    do
    {
        switch (wall[w].lotag)
        {
        case TAG_WALL_SLIDOR_LEFT:
            diff = wall[w].x - sp->x;
            DoSlidorMoveWalls(SpriteNum, diff);
            break;

        case TAG_WALL_SLIDOR_RIGHT:
            diff = wall[w].x - sp->x;
            DoSlidorMoveWalls(SpriteNum, -diff);
            break;

        case TAG_WALL_SLIDOR_UP:
            diff = wall[w].y - sp->y;
            DoSlidorMoveWalls(SpriteNum, diff);
            break;

        case TAG_WALL_SLIDOR_DOWN:
            diff = wall[w].y - sp->y;
            DoSlidorMoveWalls(SpriteNum, -diff);
            break;
        }

        w = wall[w].point2;
    }
    while (w != startwall);

    return 0;
}


int DoSlidorMove(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    ROTATORp r;
    short ndx,w,startwall,endwall;
    int i, nexti;
    int nx,ny;
    int dist,closest;
    int old_pos;
    SWBOOL kill = FALSE;

    r = u->rotator;

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
            SetSlidorInactive(SpriteNum);

            if (SP_TAG6(sp) && !TEST_BOOL8(sp))
                DoMatchEverything(NULL, SP_TAG6(sp), -1);

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
            SetSlidorInactive(SpriteNum);

            RESET_BOOL8(sp);

            // set owner swith back to OFF
            // only if ALL vators are inactive
            if (!TestSlidorMatchActive(match))
            {
                //SlidorSwitch(match, OFF);
            }

            if (SP_TAG6(sp) && TEST_BOOL8(sp))
                DoMatchEverything(NULL, SP_TAG6(sp), -1);
        }

        if (TEST_BOOL2(sp))
            kill = TRUE;
    }
    else
    {
        // if heading for the OFF (original) position and should NOT CRUSH
        if (TEST_BOOL3(sp) && r->tgt == 0)
        {
            int i,nexti;
            SPRITEp bsp;
            USERp bu;
            SWBOOL found = FALSE;

            TRAVERSE_SPRITE_SECT(headspritesect[sp->sectnum], i, nexti)
            {
                bsp = &sprite[i];
                bu = User[i];

                if (bu && TEST(bsp->cstat, CSTAT_SPRITE_BLOCK) && TEST(bsp->extra, SPRX_PLAYER_OR_ENEMY))
                {
                    // found something blocking so reverse to ON position
                    ReverseSlidor(SpriteNum);
                    SET_BOOL8(sp); // tell vator that something blocking door
                    found = TRUE;
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
                        ReverseSlidor(SpriteNum);

                        u->vel_rate = -u->vel_rate;
                        found = TRUE;
                    }
                }
            }
        }
    }


    DoSlidorMoveWalls(SpriteNum, r->pos - old_pos);

    if (kill)
    {
        SetSlidorInactive(SpriteNum);
        KillSprite(SpriteNum);
        return 0;
    }

    return 0;
}

int DoSlidor(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SECTORp sectp = &sector[sp->sectnum];
    int *lptr;
    int amt;

    DoSlidorMove(SpriteNum);

    return 0;
}


#include "saveable.h"

static saveable_code saveable_slidor_code[] =
{
    SAVE_CODE(ReverseSlidor),
    SAVE_CODE(SlidorSwitch),
    SAVE_CODE(SetSlidorActive),
    SAVE_CODE(SetSlidorInactive),
    SAVE_CODE(DoSlidorOperate),
    SAVE_CODE(DoSlidorMatch),
    SAVE_CODE(TestSlidorMatchActive),
    SAVE_CODE(DoSlidorInterp),
    SAVE_CODE(DoSlidorMoveWalls),
    SAVE_CODE(DoSlidorInstantClose),
    SAVE_CODE(DoSlidorMove),
    SAVE_CODE(DoSlidor),
};

saveable_module saveable_slidor =
{
    // code
    saveable_slidor_code,
    SIZ(saveable_slidor_code),

    // data
    NULL,0
};
