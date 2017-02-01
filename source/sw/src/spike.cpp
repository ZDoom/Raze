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
#include "tags.h"
#include "sector.h"
#include "sprite.h"

short DoSpikeMatch(PLAYERp pp, short match);
SWBOOL TestSpikeMatchActive(short match);
int DoVatorMove(short SpriteNum, int *lptr);
void InterpSectorSprites(short sectnum, SWBOOL state);

void ReverseSpike(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;

    // if paused go ahead and start it up again
    if (u->Tics)
    {
        u->Tics = 0;
        SetSpikeActive(SpriteNum);
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

SWBOOL
SpikeSwitch(short match, short setting)
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

void SetSpikeActive(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SECTORp sectp = &sector[sp->sectnum];

    if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
        short_setinterpolation(&sectp->ceilingheinum);
    else
        short_setinterpolation(&sectp->floorheinum);

    InterpSectorSprites(sp->sectnum, ON);

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

void SetSpikeInactive(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SECTORp sectp = &sector[sp->sectnum];

    if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
        short_stopinterpolation(&sectp->ceilingheinum);
    else
        short_stopinterpolation(&sectp->floorheinum);

    InterpSectorSprites(sp->sectnum, OFF);

    // play activate sound
    DoSoundSpotMatch(SP_TAG2(sp), 2, SOUND_OBJECT_TYPE);

    RESET(u->Flags, SPR_ACTIVE);
}

// called for operation from the space bar
short DoSpikeOperate(PLAYERp pp, short sectnum)
{
    USERp fu;
    SPRITEp fsp;
    short match;
    short i,nexti;

    TRAVERSE_SPRITE_SECT(headspritesect[sectnum], i, nexti)
    {
        fsp = &sprite[i];

        if (fsp->statnum == STAT_SPIKE && SP_TAG1(fsp) == SECT_SPIKE && SP_TAG3(fsp) == 0)
        {
            fu = User[i];

            sectnum = fsp->sectnum;

            match = SP_TAG2(fsp);
            if (match > 0)
            {
                if (TestSpikeMatchActive(match))
                    return -1;
                else
                    return DoSpikeMatch(pp, match);
            }

            SetSpikeActive(i);
            break;
        }
    }

    return i;
}

// called from switches and triggers
// returns first spike found
short
DoSpikeMatch(PLAYERp pp, short match)
{
    USERp fu;
    SPRITEp fsp;
    short sectnum;
    short first_spike = -1;

    short i,nexti;

    //SpikeSwitch(match, ON);

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SPIKE], i, nexti)
    {
        fsp = &sprite[i];

        if (SP_TAG1(fsp) == SECT_SPIKE && SP_TAG2(fsp) == match)
        {
            fu = User[i];

            if (first_spike == -1)
                first_spike = i;

            sectnum = fsp->sectnum;

            if (TEST(fu->Flags, SPR_ACTIVE))
            {
                ReverseSpike(i);
                continue;
            }

            SetSpikeActive(i);
        }
    }

    return first_spike;
}


SWBOOL
TestSpikeMatchActive(short match)
{
    USERp fu;
    SPRITEp fsp;
    short sectnum;

    short i,nexti;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SPIKE], i, nexti)
    {
        fsp = &sprite[i];

        if (SP_TAG1(fsp) == SECT_SPIKE && SP_TAG2(fsp) == match)
        {
            fu = User[i];

            // door war
            if (TEST_BOOL6(fsp))
                continue;

            if (TEST(fu->Flags, SPR_ACTIVE) || fu->Tics)
                return TRUE;
        }
    }

    return FALSE;
}

int DoSpikeMove(short SpriteNum, int *lptr)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SECTORp sectp = &sector[sp->sectnum];
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

void SpikeAlign(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;

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
    SECTORp sectp = &sector[sectnum];
    SPRITEp sp;
    short i,nexti;
    int cz,fz;

    TRAVERSE_SPRITE_SECT(headspritesect[sectnum], i, nexti)
    {
        sp = &sprite[i];

        if (User[i])
            continue;

        if (TEST(sp->extra, SPRX_STAY_PUT_VATOR))
            continue;

        getzsofslope(sectnum, sp->x, sp->y, &cz, &fz);
        sp->z = fz;
    }
}

int DoSpike(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SECTORp sectp = &sector[sp->sectnum];
    int *lptr;
    int amt;

    // zclip = floor or ceiling z
    // oz = original z
    // z_tgt = target z - on pos
    // sz = starting z - off pos

    lptr = &u->zclip;

    DoSpikeMove(SpriteNum, lptr);
    MoveSpritesWithSpike(sp->sectnum);
    SpikeAlign(SpriteNum);

    // EQUAL this entry has finished
    if (*lptr == u->z_tgt)
    {
        // in the ON position
        if (u->z_tgt == sp->z)
        {
            // change target
            u->z_tgt = u->sz;
            u->vel_rate = -u->vel_rate;

            SetSpikeInactive(SpriteNum);

            if (SP_TAG6(sp))
                DoMatchEverything(NULL, SP_TAG6(sp), -1);
        }
        else
        // in the OFF position
        if (u->z_tgt == u->sz)
        {
            short match = SP_TAG2(sp);

            // change target
            u->jump_speed = u->vel_tgt;
            u->vel_rate = labs(u->vel_rate);
            u->z_tgt = sp->z;

            SetSpikeInactive(SpriteNum);

            // set owner swith back to OFF
            // only if ALL spikes are inactive
            if (!TestSpikeMatchActive(match))
            {
                //SpikeSwitch(match, OFF);
            }

            if (SP_TAG6(sp) && TEST_BOOL5(sp))
                DoMatchEverything(NULL, SP_TAG6(sp), -1);
        }

        // operate only once
        if (TEST_BOOL2(sp))
        {
            SetSpikeInactive(SpriteNum);
            KillSprite(SpriteNum);
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
                    ReverseSpike(SpriteNum);
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
                        ReverseSpike(SpriteNum);
                        found = TRUE;
                    }
                }
            }
        }
    }

    return 0;
}

int DoSpikeAuto(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SECTORp sectp = &sector[sp->sectnum];
    int zval;
    int *lptr;
    int amt;

    lptr = &u->zclip;

    DoSpikeMove(SpriteNum, lptr);
    MoveSpritesWithSpike(sp->sectnum);
    SpikeAlign(SpriteNum);

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
                DoMatchEverything(NULL, SP_TAG6(sp), -1);
        }
        else
        // in the DOWN position
        if (u->z_tgt == u->sz)
        {
            // change target
            u->jump_speed = u->vel_tgt;
            u->vel_rate = labs(u->vel_rate);
            u->z_tgt = sp->z;
            u->Tics = u->WaitTics;

            if (SP_TAG6(sp) && TEST_BOOL5(sp))
                DoMatchEverything(NULL, SP_TAG6(sp), -1);
        }
    }

    return 0;
}


#include "saveable.h"

static saveable_code saveable_spike_code[] =
{
    SAVE_CODE(ReverseSpike),
    SAVE_CODE(SpikeSwitch),
    SAVE_CODE(SetSpikeActive),
    SAVE_CODE(SetSpikeInactive),
    SAVE_CODE(DoSpikeOperate),
    SAVE_CODE(DoSpikeMatch),
    SAVE_CODE(TestSpikeMatchActive),
    SAVE_CODE(DoSpikeMove),
    SAVE_CODE(SpikeAlign),
    SAVE_CODE(MoveSpritesWithSpike),
    SAVE_CODE(DoSpike),
    SAVE_CODE(DoSpikeAuto),
};

saveable_module saveable_spike =
{
    // code
    saveable_spike_code,
    SIZ(saveable_spike_code),

    // data
    NULL,0
};
