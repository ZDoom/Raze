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
#include "net.h"
#include "tags.h"
#include "sector.h"
#include "interp.h"
#include "text.h"
#include "sprite.h"
#include "weapon.h"


short DoVatorMatch(PLAYERp pp, short match);
SWBOOL TestVatorMatchActive(short match);
void InterpSectorSprites(short sectnum, SWBOOL state);
int InitBloodSpray(short, SWBOOL, short);

void ReverseVator(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;

    // if paused go ahead and start it up again
    if (u->Tics)
    {
        u->Tics = 0;
        SetVatorActive(SpriteNum);
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
VatorSwitch(short match, short setting)
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

void SetVatorActive(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SECTORp sectp = &sector[sp->sectnum];

    if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
        setinterpolation(&sectp->ceilingz);
    else
        setinterpolation(&sectp->floorz);

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

void SetVatorInactive(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SECTORp sectp = &sector[sp->sectnum];

    if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
        stopinterpolation(&sectp->ceilingz);
    else
        stopinterpolation(&sectp->floorz);

    InterpSectorSprites(sp->sectnum, OFF);

    // play inactivate sound
    DoSoundSpotMatch(SP_TAG2(sp), 2, SOUND_OBJECT_TYPE);

    RESET(u->Flags, SPR_ACTIVE);
}

// called for operation from the space bar
short DoVatorOperate(PLAYERp pp, short sectnum)
{
    USERp fu;
    SPRITEp fsp;
    short match;
    short i,nexti;

    TRAVERSE_SPRITE_SECT(headspritesect[sectnum], i, nexti)
    {
        fsp = &sprite[i];

        if (fsp->statnum == STAT_VATOR && SP_TAG1(fsp) == SECT_VATOR && SP_TAG3(fsp) == 0)
        {
            fu = User[i];

            sectnum = fsp->sectnum;

            // single play only vator
            // SWBOOL 8 must be set for message to display
            if (TEST_BOOL4(fsp) && (gNet.MultiGameType == MULTI_GAME_COMMBAT || gNet.MultiGameType == MULTI_GAME_AI_BOTS))
            {
                if (pp && TEST_BOOL11(fsp)) PutStringInfo(pp,"This only opens in single play.");
                continue;
            }

            match = SP_TAG2(fsp);
            if (match > 0)
            {
                if (TestVatorMatchActive(match))
                    return -1;
                else
                    return DoVatorMatch(pp, match);
            }

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
                    return FALSE;
                }
            }

            SetVatorActive(i);
            break;
        }
    }

    return i;
}

// called from switches and triggers
// returns first vator found
short
DoVatorMatch(PLAYERp pp, short match)
{
    USERp fu;
    SPRITEp fsp;
    short sectnum;
    short first_vator = -1;

    short i,nexti;

    //VatorSwitch(match, ON);

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_VATOR], i, nexti)
    {
        fsp = &sprite[i];

        if (SP_TAG1(fsp) == SECT_VATOR && SP_TAG2(fsp) == match)
        {
            fu = User[i];

            if (first_vator == -1)
                first_vator = i;

            // single play only vator
            // SWBOOL 8 must be set for message to display
            if (TEST_BOOL4(fsp) && (gNet.MultiGameType == MULTI_GAME_COMMBAT || gNet.MultiGameType == MULTI_GAME_AI_BOTS))
            {
                if (pp && TEST_BOOL11(fsp)) PutStringInfo(pp,"This only opens in single play.");
                continue;
            }

            // lock code
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

            // remember the player than activated it
            fu->PlayerP = pp;

            if (TEST(fu->Flags, SPR_ACTIVE))
            {
                ReverseVator(i);
                continue;
            }

            SetVatorActive(i);
        }
    }

    return first_vator;
}


SWBOOL
TestVatorMatchActive(short match)
{
    USERp fu;
    SPRITEp fsp;
    short sectnum;

    short i,nexti;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_VATOR], i, nexti)
    {
        fsp = &sprite[i];

        if (SP_TAG1(fsp) == SECT_VATOR && SP_TAG2(fsp) == match)
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

void InterpSectorSprites(short sectnum, SWBOOL state)
{
    SPRITEp sp;
    short i,nexti;

    TRAVERSE_SPRITE_SECT(headspritesect[sectnum], i, nexti)
    {
        sp = &sprite[i];

        if (User[i])
        {
            if (TEST(User[i]->Flags, SPR_SKIP4) && sp->statnum <= STAT_SKIP4_INTERP_END)
                continue;

            if (TEST(User[i]->Flags, SPR_SKIP2) && sp->statnum <= STAT_SKIP2_INTERP_END)
                continue;
        }

        if (state)
            setinterpolation(&sp->z);
        else
            stopinterpolation(&sp->z);
    }
}

void MoveSpritesWithSector(short sectnum, int z_amt, SWBOOL type)
{
    SECTORp sectp = &sector[sectnum];
    SPRITEp sp;
    short i,nexti;
    SWBOOL both = FALSE;

    if (SectUser[sectnum])
        both = !!TEST(SectUser[sectnum]->flags, SECTFU_VATOR_BOTH);

    TRAVERSE_SPRITE_SECT(headspritesect[sectnum], i, nexti)
    {
        sp = &sprite[i];

        if (User[i])
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

int DoVatorMove(short SpriteNum, int *lptr)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SECTORp sectp = &sector[sp->sectnum];
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


int DoVator(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
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
        amt = DoVatorMove(SpriteNum, lptr);
        MoveSpritesWithSector(sp->sectnum, amt, 1); // ceiling
    }
    else
    {
        lptr = &sectp->floorz;
        amt = DoVatorMove(SpriteNum, lptr);
        MoveSpritesWithSector(sp->sectnum, amt, 0); // floor
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

            SetVatorInactive(SpriteNum);

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
            u->vel_rate = labs(u->vel_rate);
            u->z_tgt = sp->z;

            RESET_BOOL8(sp);
            SetVatorInactive(SpriteNum);

            // set owner swith back to OFF
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
            SetVatorInactive(SpriteNum);
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

                if (bsp->statnum == STAT_ENEMY)
                {
                    if (labs(sectp->ceilingz - sectp->floorz) < SPRITEp_SIZE_Z(bsp))
                    {
                        InitBloodSpray(i, TRUE, -1);
                        UpdateSinglePlayKills(i);
                        KillSprite(i);
                        continue;
                    }
                }

                if (bu && TEST(bsp->cstat, CSTAT_SPRITE_BLOCK) && TEST(bsp->extra, SPRX_PLAYER_OR_ENEMY))
                {
                    // found something blocking so reverse to ON position
                    ReverseVator(SpriteNum);
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
                        ReverseVator(SpriteNum);

                        u->vel_rate = -u->vel_rate;
                        found = TRUE;
                    }
                }
            }
        }
        else
        {
            int i,nexti;
            SPRITEp bsp;
            USERp bu;
            SWBOOL found = FALSE;

            TRAVERSE_SPRITE_SECT(headspritesect[sp->sectnum], i, nexti)
            {
                bsp = &sprite[i];
                bu = User[i];

                if (bsp->statnum == STAT_ENEMY)
                {
                    if (labs(sectp->ceilingz - sectp->floorz) < SPRITEp_SIZE_Z(bsp))
                    {
                        InitBloodSpray(i, TRUE, -1);
                        UpdateSinglePlayKills(i);
                        KillSprite(i);
                        continue;
                    }
                }
            }
        }
    }



    return 0;
}

int DoVatorAuto(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SECTORp sectp = &sector[sp->sectnum];
    int zval;
    int *lptr;
    int amt;

    if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
    {
        lptr = &sectp->ceilingz;
        amt = DoVatorMove(SpriteNum, lptr);
        MoveSpritesWithSector(sp->sectnum, amt, 1); // ceiling
    }
    else
    {
        lptr = &sectp->floorz;
        amt = DoVatorMove(SpriteNum, lptr);
        MoveSpritesWithSector(sp->sectnum, amt, 0); // floor
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

static saveable_code saveable_vator_code[] =
{
    SAVE_CODE(ReverseVator),
    SAVE_CODE(VatorSwitch),
    SAVE_CODE(SetVatorActive),
    SAVE_CODE(SetVatorInactive),
    SAVE_CODE(DoVatorOperate),
    SAVE_CODE(DoVatorMatch),
    SAVE_CODE(TestVatorMatchActive),
    SAVE_CODE(InterpSectorSprites),
    SAVE_CODE(MoveSpritesWithSector),
    SAVE_CODE(DoVatorMove),
    SAVE_CODE(DoVator),
    SAVE_CODE(DoVatorAuto),
};

saveable_module saveable_vator =
{
    // code
    saveable_vator_code,
    SIZ(saveable_vator_code),

    // data
    NULL,0
};
