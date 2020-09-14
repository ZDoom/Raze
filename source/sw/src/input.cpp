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
#include "game.h"
#include "network.h"
#include "gamecontrol.h"
#include "player.h"
#include "menu.h"


BEGIN_SW_NS

void DoPlayerHorizon(PLAYERp pp, fixed_t const q16horz, double const scaleAdjust);
void DoPlayerTurn(PLAYERp pp, fixed_t const q16avel, double const scaleAdjust);
void DoPlayerTurnVehicle(PLAYERp pp, fixed_t q16avel, int z, int floor_dist);
void DoPlayerTurnTurret(PLAYERp pp, fixed_t q16avel);

static InputPacket loc;
static int32_t turnheldtime;

void
InitNetVars(void)
{
    loc = {};
}

void
InitTimingVars(void)
{
    PlayClock = 0;
    randomseed = 17L;
    MoveSkip8 = 2;
    MoveSkip2 = 0;
    MoveSkip4 = 1;                      // start slightly offset so these
}


enum
{
    TURBOTURNTIME = (120 / 8),
    NORMALTURN = (12 + 6),
    RUNTURN = (28),
    PREAMBLETURN = 3,
    NORMALKEYMOVE = 35,
    MAXFVEL = ((NORMALKEYMOVE * 2) + 10),
    MAXSVEL = ((NORMALKEYMOVE * 2) + 10),
    MAXANGVEL = 100,
    MAXHORIZVEL = 128
};

//---------------------------------------------------------------------------
//
// handles the input bits
//
//---------------------------------------------------------------------------

static void processInputBits(PLAYERp const pp, ControlInfo* const hidInput, bool* mouseaim)
{
    ApplyGlobalInput(loc, hidInput);
    *mouseaim = !(loc.actions & SB_AIMMODE);

    if (!CommEnabled)
    {
        // Go back to the source to set this - the old code here was catastrophically bad.
        // this needs to be fixed properly - as it is this can never be compatible with demo playback.

        if (mouseaim)
            SET(Player[myconnectindex].Flags, PF_MOUSE_AIMING_ON);
        else
            RESET(Player[myconnectindex].Flags, PF_MOUSE_AIMING_ON);

        if (cl_autoaim)
            SET(Player[myconnectindex].Flags, PF_AUTO_AIM);
        else
            RESET(Player[myconnectindex].Flags, PF_AUTO_AIM);
    }

    if (buttonMap.ButtonDown(gamefunc_Toggle_Crouch))
    {
        // this shares a bit with another function so cannot be in the common code.
        loc.actions |= SB_CROUCH_LOCK;
    }
}

//---------------------------------------------------------------------------
//
// handles movement
//
//---------------------------------------------------------------------------

static void processWeapon(PLAYERp const pp)
{
    USERp u = User[pp->PlayerSprite];
    int i;

    if (loc.getNewWeapon() == WeaponSel_Next)
    {
        short next_weapon = u->WeaponNum + 1;
        short start_weapon;

        start_weapon = u->WeaponNum + 1;

        if (u->WeaponNum == WPN_SWORD)
            start_weapon = WPN_STAR;

        if (u->WeaponNum == WPN_FIST)
        {
            next_weapon = 14;
        }
        else
        {
            next_weapon = -1;
            for (i = start_weapon; true; i++)
            {
                if (i >= MAX_WEAPONS_KEYS)
                {
                    next_weapon = 13;
                    break;
                }

                if (TEST(pp->WpnFlags, BIT(i)) && pp->WpnAmmo[i])
                {
                    next_weapon = i;
                    break;
                }
            }
        }

        loc.setNewWeapon(next_weapon + 1);
    }
    else if (loc.getNewWeapon() == WeaponSel_Prev)
    {
        USERp u = User[pp->PlayerSprite];
        short prev_weapon = u->WeaponNum - 1;
        short start_weapon;

        start_weapon = u->WeaponNum - 1;

        if (u->WeaponNum == WPN_SWORD)
        {
            prev_weapon = 13;
        }
        else if (u->WeaponNum == WPN_STAR)
        {
            prev_weapon = 14;
        }
        else
        {
            prev_weapon = -1;
            for (i = start_weapon; true; i--)
            {
                if (i <= -1)
                    i = WPN_HEART;

                if (TEST(pp->WpnFlags, BIT(i)) && pp->WpnAmmo[i])
                {
                    prev_weapon = i;
                    break;
                }
            }
        }
        loc.setNewWeapon(prev_weapon + 1);
    }
    else if (loc.getNewWeapon() == WeaponSel_Alt)
    {
        USERp u = User[pp->PlayerSprite];
        short const which_weapon = u->WeaponNum + 1;
        loc.setNewWeapon(which_weapon);
    }
}

//---------------------------------------------------------------------------
//
// handles movement
//
//---------------------------------------------------------------------------

static void processMovement(PLAYERp const pp, ControlInfo* const hidInput, bool const mouseaim)
{
    double const scaleAdjust = InputScale();
    bool const strafeKey = buttonMap.ButtonDown(gamefunc_Strafe) && !pp->sop;
    int32_t turnamount, keymove;
    int32_t fvel = 0, svel = 0;
    fixed_t q16avel = 0, q16horz = 0;

    if (loc.actions & SB_RUN)
    {
        turnamount = pp->sop_control ? RUNTURN * 3 : RUNTURN;
        keymove = NORMALKEYMOVE << 1;
    }
    else
    {
        turnamount = pp->sop_control ? NORMALTURN * 3 : NORMALTURN;
        keymove = NORMALKEYMOVE;
    }

    if (strafeKey)
    {
        svel -= xs_CRoundToInt(hidInput->mousex * 4.);
        svel -= hidInput->dyaw * keymove;
    }
    else
    {
        q16avel += FloatToFixed(hidInput->mousex + (scaleAdjust * hidInput->dyaw));
    }

    if (mouseaim)
        q16horz -= FloatToFixed(hidInput->mousey);
    else
        fvel -= xs_CRoundToInt(hidInput->mousey * 8.);

    if (in_mouseflip)
        q16horz = -q16horz;

    q16horz -= FloatToFixed(scaleAdjust * hidInput->dpitch);
    svel -= hidInput->dx * keymove;
    fvel -= hidInput->dz * keymove;

    if (strafeKey)
    {
        if (buttonMap.ButtonDown(gamefunc_Turn_Left))
            svel += keymove;
        if (buttonMap.ButtonDown(gamefunc_Turn_Right))
            svel -= keymove;
    }
    else
    {
        if (buttonMap.ButtonDown(gamefunc_Turn_Left) || (buttonMap.ButtonDown(gamefunc_Strafe_Left) && pp->sop))
        {
            turnheldtime += synctics;
            q16avel -= FloatToFixed(scaleAdjust * (turnheldtime >= TURBOTURNTIME ? turnamount : PREAMBLETURN));
        }
        else if (buttonMap.ButtonDown(gamefunc_Turn_Right) || (buttonMap.ButtonDown(gamefunc_Strafe_Right) && pp->sop))
        {
            turnheldtime += synctics;
            q16avel += FloatToFixed(scaleAdjust * (turnheldtime >= TURBOTURNTIME ? turnamount : PREAMBLETURN));
        }
        else
        {
            turnheldtime = 0;
        }
    }

    if (abs(loc.svel) < keymove)
    {
        if (buttonMap.ButtonDown(gamefunc_Strafe_Left) && !pp->sop)
            svel += keymove;

        if (buttonMap.ButtonDown(gamefunc_Strafe_Right) && !pp->sop)
            svel -= keymove;
    }
    if (abs(loc.fvel) < keymove)
    {
        if (buttonMap.ButtonDown(gamefunc_Move_Forward))
            fvel += keymove;

        if (buttonMap.ButtonDown(gamefunc_Move_Backward))
            fvel -= keymove;
    }

    if (!cl_syncinput)
    {
        if (TEST(pp->Flags2, PF2_INPUT_CAN_AIM))
        {
            DoPlayerHorizon(pp, q16horz, scaleAdjust);
        }

        if (pp->horizTarget)
        {
            fixed_t horizDelta = pp->horizTarget - pp->q16horiz;
            pp->q16horiz += xs_CRoundToInt(scaleAdjust * horizDelta);

            if (abs(pp->q16horiz - pp->horizTarget) < FRACUNIT)
            {
                pp->q16horiz = pp->horizTarget;
                pp->horizTarget = 0;
            }
        }
        else if (pp->horizAdjust)
        {
            pp->q16horiz += FloatToFixed(scaleAdjust * pp->horizAdjust);
        }

        if (TEST(pp->Flags2, PF2_INPUT_CAN_TURN_GENERAL))
        {
            DoPlayerTurn(pp, q16avel, scaleAdjust);
        }

        if (pp->angTarget)
        {
            fixed_t angDelta = GetDeltaQ16Angle(pp->angTarget, pp->q16ang);
            pp->q16ang = (pp->q16ang + xs_CRoundToInt(scaleAdjust * angDelta));

            if (abs(pp->q16ang - pp->angTarget) < FRACUNIT)
            {
                pp->q16ang = pp->angTarget;
                pp->angTarget = 0;
            }
        }
        else if (pp->angAdjust)
        {
            pp->q16ang = (pp->q16ang + FloatToFixed(scaleAdjust * pp->angAdjust)) & 0x7FFFFFF;
        }

        if (TEST(pp->Flags2, PF2_INPUT_CAN_TURN_VEHICLE))
        {
            DoPlayerTurnVehicle(pp, q16avel, pp->posz + Z(10), labs(pp->posz + Z(10) - pp->sop->floor_loz));
        }

        if (TEST(pp->Flags2, PF2_INPUT_CAN_TURN_TURRET))
        {
            DoPlayerTurnTurret(pp, q16avel);
        }
    }

    loc.fvel = clamp(loc.fvel + fvel, -MAXFVEL, MAXFVEL);
    loc.svel = clamp(loc.svel + svel, -MAXSVEL, MAXSVEL);

    loc.q16avel = clamp(loc.q16avel + q16avel, -IntToFixed(MAXANGVEL), IntToFixed(MAXANGVEL));
    loc.q16horz = clamp(loc.q16horz + q16horz, -IntToFixed(MAXHORIZVEL), IntToFixed(MAXHORIZVEL));
}

void GameInterface::GetInput(InputPacket *packet, ControlInfo* const hidInput)
{
    if (paused || M_Active())
    {
        loc = {};
        return;
    }

    PLAYERp pp = &Player[myconnectindex];
    bool mouseaim;

    processInputBits(pp, hidInput, &mouseaim);
    processMovement(pp, hidInput, mouseaim);
    processWeapon(pp);

    if (packet)
    {
        auto const ang = FixedToInt(pp->q16ang);

        *packet = loc;

        packet->fvel = mulscale9(loc.fvel, sintable[NORM_ANGLE(ang + 512)]) + mulscale9(loc.svel, sintable[NORM_ANGLE(ang)]);
        packet->svel = mulscale9(loc.fvel, sintable[NORM_ANGLE(ang)]) + mulscale9(loc.svel, sintable[NORM_ANGLE(ang + 1536)]);

        loc = {};
    }
}

END_SW_NS
