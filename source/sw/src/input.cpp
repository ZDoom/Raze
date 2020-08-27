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

SWBOOL MultiPlayQuitFlag = FALSE;

int BitsToSend = 0;
int inv_hotkey = 0;


void
FunctionKeys(PLAYERp pp)
{
    // F7 VIEW control
	if (buttonMap.ButtonDown(gamefunc_Third_Person_View))
    {
		buttonMap.ClearButton(gamefunc_Third_Person_View);

        if (inputState.ShiftPressed())
        {
            if (TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE))
                pp->view_outside_dang = NORM_ANGLE(pp->view_outside_dang + 256);
        }
        else
        {
            if (TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE))
            {
                RESET(pp->Flags, PF_VIEW_FROM_OUTSIDE);
            }
            else
            {
                SET(pp->Flags, PF_VIEW_FROM_OUTSIDE);
                pp->camera_dist = 0;
            }
        }
    }
}



double elapsedInputTicks;
double scaleAdjustmentToInterval(double x) { return x * (120 / synctics) / (1000.0 / elapsedInputTicks); }

void DoPlayerTurn(PLAYERp pp, fix16_t *pq16ang, fix16_t q16angvel);
void DoPlayerHorizon(PLAYERp pp, fix16_t *pq16horiz, fix16_t q16aimvel);


void GameInterface::ResetFollowPos(bool)
{
	auto pp = &Player[myconnectindex];
	Follow_posx = pp->posx;
	Follow_posy = pp->posy;
}

void
getinput(InputPacket *loc, SWBOOL tied)
{
    int i;
    PLAYERp pp = Player + myconnectindex;
    PLAYERp newpp = Player + myconnectindex;

#define TURBOTURNTIME (120/8)
#define NORMALTURN   (12+6)
#define RUNTURN      (28)
#define PREAMBLETURN 3
#define NORMALKEYMOVE 35
#define MAXVEL       ((NORMALKEYMOVE*2)+10)
#define MAXSVEL      ((NORMALKEYMOVE*2)+10)
#define MAXANGVEL    100
#define MAXHORIZVEL  128
#define SET_LOC_KEY(loc, sync_num, key_test) SET(loc, ((!!(key_test)) << (sync_num)))

    static int32_t turnheldtime;
    int32_t momx, momy;

    extern SWBOOL MenuButtonAutoAim;

    if (Prediction && CommEnabled)
    {
        newpp = ppp;
    }

    static double lastInputTicks;

    auto const currentHiTicks = I_msTimeF();
    elapsedInputTicks = currentHiTicks - lastInputTicks;

    lastInputTicks = currentHiTicks;

    // MAKE SURE THIS WILL GET SET
    SET_LOC_KEY(loc->bits, SK_QUIT_GAME, MultiPlayQuitFlag);

    bool mouseaim = in_mousemode || buttonMap.ButtonDown(gamefunc_Mouse_Aiming);

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

    ControlInfo info;
    CONTROL_GetInput(&info);

    if (paused)
        return;

    // If in 2D follow mode, scroll around using glob vars
    // Tried calling this in domovethings, but key response it too poor, skips key presses
    // Note: this get called only during follow mode
    if (!tied && automapFollow && automapMode != am_off && pp == Player + myconnectindex && !Prediction)
        MoveScrollMode2D(Player + myconnectindex);

    // !JIM! Added M_Active() so that you don't move at all while using menus
    if (M_Active() || automapFollow)
        return;

    SET_LOC_KEY(loc->bits, SK_SPACE_BAR, buttonMap.ButtonDown(gamefunc_Open));

    int const running = G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run));
    int32_t turnamount;
    int32_t keymove;

    // The function DoPlayerTurn() scales the player's q16angvel by 1.40625, so store as constant
    // and use to scale back player's aim and ang values for a consistent feel between games.
    float const angvelScale = 1.40625f;
    float const aimvelScale = 1.203125f;

    // Shadow Warrior has a ticrate of 40, 25% more than the other games, so store below constant
    // for dividing controller input to match speed input speed of other games.
    float const ticrateScale = 0.75f;

    ApplyGlobalInput(*loc, &info);


    if (running)
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

    if (tied)
        keymove = 0;

    int32_t svel = 0, vel = 0;
    fix16_t q16aimvel = 0, q16angvel = 0;

    if (buttonMap.ButtonDown(gamefunc_Strafe) && !pp->sop)
    {
        svel -= (info.mousex * ticrateScale) * 4.f;
        svel -= info.dyaw * keymove;
    }
    else
    {
        q16angvel = fix16_sadd(q16angvel, fix16_from_float(info.mousex / angvelScale));
        q16angvel = fix16_sadd(q16angvel, fix16_from_dbl(scaleAdjustmentToInterval((info.dyaw * ticrateScale) / angvelScale)));
    }

    if (mouseaim)
        q16aimvel = fix16_ssub(q16aimvel, fix16_from_float(info.mousey / aimvelScale));
    else
        vel -= (info.mousey * ticrateScale) * 8.f;

    if (in_mouseflip)
        q16aimvel = -q16aimvel;

    q16aimvel -= fix16_from_dbl(scaleAdjustmentToInterval((info.dpitch * ticrateScale) / aimvelScale));
    svel -= info.dx * keymove;
    vel -= info.dz * keymove;

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
            if (PedanticMode)
            {
                if (turnheldtime >= TURBOTURNTIME)
                    q16angvel -= fix16_from_int(turnamount);
                else
                    q16angvel -= fix16_from_int(PREAMBLETURN);
            }
            else
                q16angvel = fix16_ssub(q16angvel, fix16_from_float(scaleAdjustmentToInterval((turnheldtime >= TURBOTURNTIME) ? turnamount : PREAMBLETURN)));
        }
        else if (buttonMap.ButtonDown(gamefunc_Turn_Right) || (buttonMap.ButtonDown(gamefunc_Strafe_Right) && pp->sop))
        {
            turnheldtime += synctics;
            if (PedanticMode)
            {
                if (turnheldtime >= TURBOTURNTIME)
                    q16angvel += fix16_from_int(turnamount);
                else
                    q16angvel += fix16_from_int(PREAMBLETURN);
            }
            else
                q16angvel = fix16_sadd(q16angvel, fix16_from_float(scaleAdjustmentToInterval((turnheldtime >= TURBOTURNTIME) ? turnamount : PREAMBLETURN)));
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

    q16angvel = fix16_clamp(q16angvel, -fix16_from_int(MAXANGVEL), fix16_from_int(MAXANGVEL));
    q16aimvel = fix16_clamp(q16aimvel, -fix16_from_int(MAXHORIZVEL), fix16_from_int(MAXHORIZVEL));

    void DoPlayerTeleportPause(PLAYERp pp);
    if (PedanticMode)
    {
        q16angvel = fix16_floor(q16angvel);
        q16aimvel = fix16_floor(q16aimvel);
    }
    else
    {
        fix16_t prevcamq16ang = pp->camq16ang, prevcamq16horiz = pp->camq16horiz;

        if (TEST(pp->Flags2, PF2_INPUT_CAN_TURN))
            DoPlayerTurn(pp, &pp->camq16ang, q16angvel);
        if (TEST(pp->Flags2, PF2_INPUT_CAN_AIM))
            DoPlayerHorizon(pp, &pp->camq16horiz, q16aimvel);
        pp->oq16ang += pp->camq16ang - prevcamq16ang;
        pp->oq16horiz += pp->camq16horiz - prevcamq16horiz;
    }

    loc->fvel += vel;
    loc->svel += svel;

    if (!tied)
    {
        vel = clamp(loc->fvel, -MAXVEL, MAXVEL);
        svel = clamp(loc->svel, -MAXSVEL, MAXSVEL);

        momx = mulscale9(vel, sintable[NORM_ANGLE(fix16_to_int(newpp->q16ang) + 512)]);
        momy = mulscale9(vel, sintable[NORM_ANGLE(fix16_to_int(newpp->q16ang))]);

        momx += mulscale9(svel, sintable[NORM_ANGLE(fix16_to_int(newpp->q16ang))]);
        momy += mulscale9(svel, sintable[NORM_ANGLE(fix16_to_int(newpp->q16ang) + 1536)]);

        loc->fvel = momx;
        loc->svel = momy;
    }

    loc->q16avel += q16angvel;
    loc->q16aimvel += q16aimvel;

    if (!CommEnabled)
    {
		// What a mess...:?
#if 0
        if (MenuButtonAutoAim)
        {
            MenuButtonAutoAim = FALSE;
            if ((!!TEST(pp->Flags, PF_AUTO_AIM)) != !!cl_autoaim)
                SET_LOC_KEY(loc->bits, SK_AUTO_AIM, TRUE);
        }
#endif
    }

    SET_LOC_KEY(loc->bits, SK_RUN, buttonMap.ButtonDown(gamefunc_Run));
    SET_LOC_KEY(loc->bits, SK_SHOOT, buttonMap.ButtonDown(gamefunc_Fire));

    // actually snap
    SET_LOC_KEY(loc->bits, SK_SNAP_UP, buttonMap.ButtonDown(gamefunc_Aim_Up));
    SET_LOC_KEY(loc->bits, SK_SNAP_DOWN, buttonMap.ButtonDown(gamefunc_Aim_Down));

    // actually just look
    SET_LOC_KEY(loc->bits, SK_LOOK_UP, buttonMap.ButtonDown(gamefunc_Look_Up));
    SET_LOC_KEY(loc->bits, SK_LOOK_DOWN, buttonMap.ButtonDown(gamefunc_Look_Down));

    if (loc->getNewWeapon() == WeaponSel_Next)
    {
        USERp u = User[pp->PlayerSprite];
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

        loc->setNewWeapon(next_weapon + 1);
    }
    else if (loc->getNewWeapon() == WeaponSel_Prev)
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
        loc->setNewWeapon(prev_weapon + 1);
    }
    else if (loc->getNewWeapon() == WeaponSel_Alt)
    {
        USERp u = User[pp->PlayerSprite];
        short const which_weapon = u->WeaponNum + 1;
        loc->setNewWeapon(which_weapon);
    }


    loc->bits |= BitsToSend;
    BitsToSend = 0;

    SET_LOC_KEY(loc->bits, SK_OPERATE, buttonMap.ButtonDown(gamefunc_Open));
    SET_LOC_KEY(loc->bits, SK_JUMP, buttonMap.ButtonDown(gamefunc_Jump));
    SET_LOC_KEY(loc->bits, SK_CRAWL, buttonMap.ButtonDown(gamefunc_Crouch));

    // need BUTTON
    SET_LOC_KEY(loc->bits, SK_CRAWL_LOCK, buttonMap.ButtonDown(gamefunc_Toggle_Crouch));

    if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
    {
        if (buttonMap.ButtonDown(gamefunc_See_Coop_View))
        {
            buttonMap.ClearButton(gamefunc_See_Coop_View);

            screenpeek = connectpoint2[screenpeek];

            if (screenpeek < 0)
                screenpeek = connecthead;

            if (screenpeek == myconnectindex)
            {
                // JBF: figure out what's going on here
                DoPlayerDivePalette(pp);  // Check Dive again
                DoPlayerNightVisionPalette(pp);  // Check Night Vision again
            }
            else
            {
                PLAYERp tp = Player+screenpeek;
                DoPlayerDivePalette(tp);
                DoPlayerNightVisionPalette(tp);
            }
        }
    }

    if (!tied)
        FunctionKeys(pp);
}


//---------------------------------------------------------------------------
//
// CCMD based input. The basics are from Randi's ZDuke but this uses dynamic
// registration to only have the commands active when this game module runs.
//
//---------------------------------------------------------------------------

void registerinputcommands()
{
    C_RegisterFunction("pause", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= BIT(SK_PAUSE); sendPause = true; return CCMD_OK; });
    C_RegisterFunction("centerview", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= BIT(SK_CENTER_VIEW); return CCMD_OK; });
    C_RegisterFunction("holsterweapon", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= BIT(SK_HIDE_WEAPON); return CCMD_OK; });
    C_RegisterFunction("invprev", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= BIT(SK_INV_LEFT); return CCMD_OK; });
    C_RegisterFunction("invnext", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= BIT(SK_INV_RIGHT); return CCMD_OK; });
    C_RegisterFunction("turnaround", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= BIT(SK_TURN_180); return CCMD_OK; });
    C_RegisterFunction("invuse", nullptr, [](CCmdFuncPtr)->int { BitsToSend |= BIT(SK_INV_USE); return CCMD_OK; });
}

// This is called from ImputState::ClearAllInput and resets all static state being used here.
void GameInterface::clearlocalinputstate()
{
    BitsToSend = 0;
    inv_hotkey = 0;

}

END_SW_NS
