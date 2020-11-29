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
#include "razemenu.h"


BEGIN_SW_NS

void DoPlayerHorizon(PLAYERp pp, float const horz, double const scaleAdjust);
void DoPlayerTurn(PLAYERp pp, float const avel, double const scaleAdjust);
void DoPlayerTurnVehicle(PLAYERp pp, float avel, int z, int floor_dist);
void DoPlayerTurnTurret(PLAYERp pp, float avel);

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

#if 0
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
#endif

//---------------------------------------------------------------------------
//
// handles the input bits
//
//---------------------------------------------------------------------------

static void processInputBits(PLAYERp const pp, ControlInfo* const hidInput)
{
    ApplyGlobalInput(loc, hidInput);

    if (!CommEnabled)
    {
        // Go back to the source to set this - the old code here was catastrophically bad.
        // this needs to be fixed properly - as it is this can never be compatible with demo playback.

        if (!(loc.actions & SB_AIMMODE))
            SET(Player[myconnectindex].Flags, PF_MOUSE_AIMING_ON);
        else
            RESET(Player[myconnectindex].Flags, PF_MOUSE_AIMING_ON);

        if (Autoaim(myconnectindex))
            SET(Player[myconnectindex].Flags, PF_AUTO_AIM);
        else
            RESET(Player[myconnectindex].Flags, PF_AUTO_AIM);
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

void GameInterface::GetInput(InputPacket *packet, ControlInfo* const hidInput)
{
    if (paused || M_Active())
    {
        loc = {};
        return;
    }

    double const scaleAdjust = InputScale();
    InputPacket input {};
    PLAYERp pp = &Player[myconnectindex];

    processInputBits(pp, hidInput);
    processMovement(&input, &loc, hidInput, scaleAdjust, 0, !pp->sop, pp->sop_control ? 3. / 1.40625 : 1.);
    processWeapon(pp);

    if (!cl_syncinput)
    {
        if (TEST(pp->Flags2, PF2_INPUT_CAN_AIM))
        {
            DoPlayerHorizon(pp, input.horz, scaleAdjust);
        }

        if (TEST(pp->Flags2, PF2_INPUT_CAN_TURN_GENERAL))
        {
            DoPlayerTurn(pp, input.avel, scaleAdjust);
        }

        if (TEST(pp->Flags2, PF2_INPUT_CAN_TURN_VEHICLE))
        {
            DoPlayerTurnVehicle(pp, input.avel, pp->posz + Z(10), labs(pp->posz + Z(10) - pp->sop->floor_loz));
        }

        if (TEST(pp->Flags2, PF2_INPUT_CAN_TURN_TURRET))
        {
            DoPlayerTurnTurret(pp, input.avel);
        }

        pp->angle.processhelpers(scaleAdjust);
        pp->horizon.processhelpers(scaleAdjust);
    }

    if (packet)
    {
        auto const cos = pp->angle.ang.bcos();
        auto const sin = pp->angle.ang.bsin();

        *packet = loc;

        packet->fvel = mulscale9(loc.fvel, cos) + mulscale9(loc.svel, sin);
        packet->svel = mulscale9(loc.fvel, sin) - mulscale9(loc.svel, cos);

        loc = {};
    }
}

//---------------------------------------------------------------------------
//
// This is called from InputState::ClearAllInput and resets all static state being used here.
//
//---------------------------------------------------------------------------

void GameInterface::clearlocalinputstate()
{
    loc = {};
    turnheldtime = 0;
}

END_SW_NS
