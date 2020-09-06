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

double elapsedInputTicks;
double scaleAdjustmentToInterval(double x) { return x * (120 / synctics) / (1000.0 / elapsedInputTicks); }

void DoPlayerTurn(PLAYERp pp, fixed_t *pq16ang, fixed_t q16angvel);
void DoPlayerHorizon(PLAYERp pp, fixed_t *pq16horiz, fixed_t q16horz);

static InputPacket loc;
static int32_t turnheldtime;

// Constant used for scaling input down to match other games. May not last long after true re-factoring.
static constexpr double const inputScale = 263. / 360.;

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



void GameInterface::ResetFollowPos(bool)
{
	auto pp = &Player[myconnectindex];
	Follow_posx = pp->posx;
	Follow_posy = pp->posy;
}


enum
{
    TURBOTURNTIME = (120 / 8),
    NORMALTURN = (12 + 6),
    RUNTURN = (28),
    PREAMBLETURN = 3,
    NORMALKEYMOVE = 35,
    MAXVEL = ((NORMALKEYMOVE * 2) + 10),
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
            for (i = start_weapon; TRUE; i++)
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
            for (i = start_weapon; TRUE; i--)
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
    static double lastInputTicks;

    auto const currentHiTicks = I_msTimeF();
    elapsedInputTicks = currentHiTicks - lastInputTicks;

    lastInputTicks = currentHiTicks;

    // If in 2D follow mode, scroll around using glob vars
    // Tried calling this in domovethings, but key response it too poor, skips key presses
    // Note: this get called only during follow mode
    if (automapFollow && automapMode != am_off && pp == Player + myconnectindex && !Prediction)
        MoveScrollMode2D(Player + myconnectindex, hidInput);

    int32_t turnamount;
    int32_t keymove;

    if (loc.actions & SB_RUN)
    {
        if (pp->sop_control)
            turnamount = RUNTURN * 3;
        else
            turnamount = RUNTURN;

        keymove = NORMALKEYMOVE << 1;
    }
    else
    {
        if (pp->sop_control)
            turnamount = NORMALTURN * 3;
        else
            turnamount = NORMALTURN;

        keymove = NORMALKEYMOVE;
    }

    int32_t svel = 0, vel = 0;
    fixed_t q16horz = 0, q16angvel = 0;

    if (buttonMap.ButtonDown(gamefunc_Strafe) && !pp->sop)
    {
        svel -= (hidInput->mousex * inputScale) * 4.f;
        svel -= hidInput->dyaw * keymove;
    }
    else
    {
        q16angvel += FloatToFixed((hidInput->mousex + scaleAdjustmentToInterval(hidInput->dyaw)) * inputScale);
    }

    if (mouseaim)
        q16horz -= FloatToFixed(hidInput->mousey * inputScale);
    else
        vel -= (hidInput->mousey * inputScale) * 8.f;

    if (in_mouseflip)
        q16horz = -q16horz;

    q16horz -= FloatToFixed(scaleAdjustmentToInterval(hidInput->dpitch * inputScale));
    svel -= hidInput->dx * keymove;
    vel -= hidInput->dz * keymove;

    if (buttonMap.ButtonDown(gamefunc_Strafe) && !pp->sop)
    {
        if (buttonMap.ButtonDown(gamefunc_Turn_Left))
            svel -= -keymove;
        if (buttonMap.ButtonDown(gamefunc_Turn_Right))
            svel -= keymove;
    }
    else
    {
        if (buttonMap.ButtonDown(gamefunc_Turn_Left) || (buttonMap.ButtonDown(gamefunc_Strafe_Left) && pp->sop))
        {
            turnheldtime += synctics;
            if (cl_syncinput)
            {
                if (turnheldtime >= TURBOTURNTIME)
                    q16angvel -= IntToFixed(turnamount);
                else
                    q16angvel -= IntToFixed(PREAMBLETURN);
            }
            else
                q16angvel -= FloatToFixed(scaleAdjustmentToInterval((turnheldtime >= TURBOTURNTIME) ? turnamount : PREAMBLETURN));
        }
        else if (buttonMap.ButtonDown(gamefunc_Turn_Right) || (buttonMap.ButtonDown(gamefunc_Strafe_Right) && pp->sop))
        {
            turnheldtime += synctics;
            if (cl_syncinput)
            {
                if (turnheldtime >= TURBOTURNTIME)
                    q16angvel += IntToFixed(turnamount);
                else
                    q16angvel += IntToFixed(PREAMBLETURN);
            }
            else
                q16angvel += FloatToFixed(scaleAdjustmentToInterval((turnheldtime >= TURBOTURNTIME) ? turnamount : PREAMBLETURN));
        }
        else
        {
            turnheldtime = 0;
        }
    }

    if (buttonMap.ButtonDown(gamefunc_Strafe_Left) && !pp->sop)
        svel += keymove;

    if (buttonMap.ButtonDown(gamefunc_Strafe_Right) && !pp->sop)
        svel += -keymove;

    if (buttonMap.ButtonDown(gamefunc_Move_Forward))
    {
        vel += keymove;
    }

    if (buttonMap.ButtonDown(gamefunc_Move_Backward))
        vel += -keymove;

    q16angvel = clamp(q16angvel, -IntToFixed(MAXANGVEL), IntToFixed(MAXANGVEL));
    q16horz = clamp(q16horz, -IntToFixed(MAXHORIZVEL), IntToFixed(MAXHORIZVEL));

    void DoPlayerTeleportPause(PLAYERp pp);
    if (cl_syncinput)
    {
        q16angvel = q16angvel;
        q16horz = q16horz;
    }
    else
    {
        fixed_t prevcamq16ang = pp->camq16ang, prevcamq16horiz = pp->camq16horiz;

        if (TEST(pp->Flags2, PF2_INPUT_CAN_TURN))
            DoPlayerTurn(pp, &pp->camq16ang, q16angvel);
        if (TEST(pp->Flags2, PF2_INPUT_CAN_AIM))
            DoPlayerHorizon(pp, &pp->camq16horiz, q16horz);
        pp->oq16ang += pp->camq16ang - prevcamq16ang;
        pp->oq16horiz += pp->camq16horiz - prevcamq16horiz;
    }

    loc.fvel = clamp(loc.fvel + vel, -MAXVEL, MAXVEL);
    loc.svel = clamp(loc.svel + svel, -MAXSVEL, MAXSVEL);

    loc.q16avel += q16angvel;
    loc.q16horz += q16horz;
}

void GameInterface::GetInput(InputPacket *packet, ControlInfo* const hidInput)
{
    if (paused || M_Active() || (automapFollow && automapMode != am_off))
    {
        return;
    }

    PLAYERp pp = &Player[myconnectindex];
    bool mouseaim;

    processInputBits(pp, hidInput, &mouseaim);
    processMovement(pp, hidInput, mouseaim);
    processWeapon(pp);

    if (packet)
    {
        auto fvel = loc.fvel;
        auto svel = loc.svel;
        auto ang  = FixedToInt(pp->camq16ang);

        loc.fvel = mulscale9(fvel, sintable[NORM_ANGLE(ang + 512)]) + mulscale9(svel, sintable[NORM_ANGLE(ang)]);
        loc.svel = mulscale9(fvel, sintable[NORM_ANGLE(ang)]) + mulscale9(svel, sintable[NORM_ANGLE(ang + 1536)]);
        loc.q16ang = pp->camq16ang;
        loc.q16horiz = pp->camq16horiz;
        *packet = loc;
        loc = {};
    }
}

END_SW_NS
