//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#include "ns.h"	// Must come before everything else!

#include "duke3d.h"
#include "d_event.h"
#include "gamevar.h"

BEGIN_DUKE_NS

fix16_t GetDeltaQ16Angle(fix16_t ang1, fix16_t ang2);
void processCommonInput(input_t& input);
void processSelectWeapon(input_t& input);
int motoApplyTurn(player_struct* p, int turnl, int turnr, int bike_turn, bool goback, double factor);
int boatApplyTurn(player_struct* p, int turnl, int turnr, int bike_turn, double factor);


int32_t PHEIGHT = PHEIGHT_DUKE;

int32_t lastvisinc;

#define TURBOTURNTIME (TICRATE/8) // 7
#define NORMALTURN    15
#define PREAMBLETURN  5
#define NORMALKEYMOVE 40
#define MAXVEL        ((NORMALKEYMOVE*2)+10)
#define MAXSVEL       ((NORMALKEYMOVE*2)+10)
#define MAXANGVEL     1024
#define MAXHORIZVEL   256
#define ONEEIGHTYSCALE 4

#define MOTOTURN      20
#define MAXVELMOTO    120

int32_t g_myAimStat = 0, g_oldAimStat = 0;
int32_t mouseyaxismode = -1;

enum inputlock_t
{
    IL_NOANGLE = 0x1,
    IL_NOHORIZ = 0x2,
    IL_NOMOVE  = 0x4,

    IL_NOTHING = IL_NOANGLE|IL_NOHORIZ|IL_NOMOVE,
};

static int P_CheckLockedMovement(int const playerNum)
{
    auto const pPlayer = &ps[playerNum];

    if (pPlayer->on_crane >= 0)
        return IL_NOMOVE|IL_NOANGLE;

    if (pPlayer->newowner != -1)
        return IL_NOANGLE|IL_NOHORIZ;

    if (pPlayer->curr_weapon > 11) return 0;

    if (pPlayer->dead_flag || pPlayer->fist_incs || pPlayer->transporter_hold > 2 || pPlayer->hard_landing || pPlayer->access_incs > 0
        || pPlayer->knee_incs > 0
        || (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == TRIPBOMB_WEAPON && pPlayer->kickback_pic > 1
            && pPlayer->kickback_pic < PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay)))
        return IL_NOTHING;

    return 0;
}

static double elapsedInputTicks;

static double scaleAdjustmentToInterval(double x)
{
    return x * REALGAMETICSPERSEC / (1000.0 / elapsedInputTicks);
}

void P_GetInput(int const playerNum)
{
    auto      &thisPlayer = g_player[playerNum];
    auto const pPlayer = &ps[playerNum];
    auto const pSprite = &sprite[pPlayer->i];
    ControlInfo info;
    double scaleAdjust = elapsedInputTicks * REALGAMETICSPERSEC / 1000.0;

	bool mouseaim = in_mousemode || buttonMap.ButtonDown(gamefunc_Mouse_Aiming);


    CONTROL_GetInput(&info);


    // JBF: Run key behaviour is selectable
	
	int const     playerRunning    = G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run));
    int const     turnAmount       = playerRunning ? (NORMALTURN << 1) : NORMALTURN;
    constexpr int analogTurnAmount = (NORMALTURN << 1);
    int const     keyMove          = playerRunning ? (NORMALKEYMOVE << 1) : NORMALKEYMOVE;
    constexpr int analogExtent     = 32767; // KEEPINSYNC sdlayer.cpp

    input_t input {};

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        input.svel -= info.mousex * 4.f;
        input.svel -= scaleAdjustmentToInterval(info.dyaw * keyMove);
    }
    else
    {
        input.q16avel = fix16_sadd(input.q16avel, fix16_from_float(info.mousex));
        input.q16avel = fix16_sadd(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(info.dyaw)));
    }

    if (mouseaim)
        input.q16horz = fix16_sadd(input.q16horz, fix16_from_float(info.mousey));
    else
        input.fvel -= info.mousey * 8.f;

    if (!in_mouseflip) input.q16horz = -input.q16horz;

    input.q16horz = fix16_ssub(input.q16horz, fix16_from_dbl(scaleAdjustmentToInterval(info.dpitch)));
    input.svel -= scaleAdjustmentToInterval(info.dx * keyMove);
    input.fvel -= scaleAdjustmentToInterval(info.dz * keyMove);

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        if (!localInput.svel)
        {
            if (buttonMap.ButtonDown(gamefunc_Turn_Left) && !localInput.svel)
                input.svel = keyMove;

            if (buttonMap.ButtonDown(gamefunc_Turn_Right) && !localInput.svel)
                input.svel = -keyMove;
        }
    }
    else
    {
        static int32_t turnHeldTime;
        static int32_t lastInputClock;  // MED
        int32_t const  elapsedTics = (int32_t)totalclock - lastInputClock;

        lastInputClock = (int32_t) totalclock;

        if (buttonMap.ButtonDown(gamefunc_Turn_Left))
        {
            turnHeldTime += elapsedTics;
            input.q16avel = fix16_ssub(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval((turnHeldTime >= TURBOTURNTIME) ? (turnAmount << 1) : (PREAMBLETURN << 1))));
        }
        else if (buttonMap.ButtonDown(gamefunc_Turn_Right))
        {
            turnHeldTime += elapsedTics;
            input.q16avel = fix16_sadd(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval((turnHeldTime >= TURBOTURNTIME) ? (turnAmount << 1) : (PREAMBLETURN << 1))));
        }
        else
            turnHeldTime = 0;
    }

    if (localInput.svel < keyMove && localInput.svel > -keyMove)
    {
        if (buttonMap.ButtonDown(gamefunc_Strafe_Left))
            input.svel += keyMove;

        if (buttonMap.ButtonDown(gamefunc_Strafe_Right))
            input.svel += -keyMove;
    }

    if (localInput.fvel < keyMove && localInput.fvel > -keyMove)
    {
        if (isRR() && pPlayer->drink_amt >= 66 && pPlayer->drink_amt <= 87)
        {
            if (buttonMap.ButtonDown(gamefunc_Move_Forward))
            {
                input.fvel += keyMove;
                if (pPlayer->drink_amt & 1)
                    input.svel += keyMove;
                else
                    input.svel -= keyMove;
            }

            if (buttonMap.ButtonDown(gamefunc_Move_Backward))
            {
                input.fvel += -keyMove;
                if (pPlayer->drink_amt & 1)
                    input.svel -= keyMove;
                else
                    input.svel += keyMove;
            }
        }
        else
        {
            if (buttonMap.ButtonDown(gamefunc_Move_Forward))
                input.fvel += keyMove;

            if (buttonMap.ButtonDown(gamefunc_Move_Backward))
                input.fvel += -keyMove;
        }
    }

    processSelectWeapon(input);


    int const sectorLotag = pPlayer->cursectnum != -1 ? sector[pPlayer->cursectnum].lotag : 0;
    int const crouchable = sectorLotag != 2 && (sectorLotag != 1 || pPlayer->spritebridge);

    if (buttonMap.ButtonDown(gamefunc_Toggle_Crouch))
    {
        pPlayer->crouch_toggle = !pPlayer->crouch_toggle && crouchable;

        if (crouchable)
            buttonMap.ClearButton(gamefunc_Toggle_Crouch);
    }

    if (buttonMap.ButtonDown(gamefunc_Crouch) || buttonMap.ButtonDown(gamefunc_Jump) || pPlayer->jetpack_on || (!crouchable && pPlayer->on_ground))
        pPlayer->crouch_toggle = 0;

    processCommonInput(input);

    int const crouching = buttonMap.ButtonDown(gamefunc_Crouch) || buttonMap.ButtonDown(gamefunc_Toggle_Crouch) || pPlayer->crouch_toggle;

    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Jump) << SK_JUMP) | (crouching << SK_CROUCH);

    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Aim_Up) || (buttonMap.ButtonDown(gamefunc_Dpad_Aiming) && input.fvel > 0)) << SK_AIM_UP;
    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Aim_Down) || (buttonMap.ButtonDown(gamefunc_Dpad_Aiming) && input.fvel < 0)) << SK_AIM_DOWN;
    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Center_View) << SK_CENTER_VIEW);

    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Look_Left) << SK_LOOK_LEFT) | (buttonMap.ButtonDown(gamefunc_Look_Right) << SK_LOOK_RIGHT);
    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Look_Up) << SK_LOOK_UP) | (buttonMap.ButtonDown(gamefunc_Look_Down) << SK_LOOK_DOWN);

    localInput.bits |= (playerRunning << SK_RUN);

    localInput.bits |= buttonMap.ButtonDown(gamefunc_Holster_Weapon) << SK_HOLSTER;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Quick_Kick) << SK_QUICK_KICK;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_TurnAround) << SK_TURNAROUND;

    localInput.bits |= (mouseaim << SK_AIMMODE);

    if (isRR())
    {
        if (localInput.bits & SKB_CROUCH)
            localInput.bits &= ~SKB_JUMP;
        if (pPlayer->drink_amt > 88)
            localInput.bits |= SKB_LOOK_LEFT;
        if (pPlayer->drink_amt > 99)
            localInput.bits |= SKB_LOOK_DOWN;
    }

    if (buttonMap.ButtonDown(gamefunc_Dpad_Aiming))
        input.fvel = 0;

    int const movementLocked = P_CheckLockedMovement(playerNum);

    if ((ud.scrollmode && ud.overhead_on) || (movementLocked & IL_NOTHING) == IL_NOTHING)
    {
        if (ud.scrollmode && ud.overhead_on)
        {
            ud.folfvel = input.fvel;
            ud.folavel = fix16_to_int(input.q16avel);
        }

        localInput.fvel = localInput.svel = 0;
        localInput.q16avel = localInput.q16horz = 0;
    }
    else
    {
        if (!(movementLocked & IL_NOMOVE))
        {
            localInput.fvel = clamp(localInput.fvel + input.fvel, -MAXVEL, MAXVEL);
            localInput.svel = clamp(localInput.svel + input.svel, -MAXSVEL, MAXSVEL);
        }

        if (!(movementLocked & IL_NOANGLE))
        {
            localInput.q16avel = fix16_sadd(localInput.q16avel, input.q16avel);
            if (!synchronized_input)
            {
                pPlayer->q16ang = fix16_sadd(pPlayer->q16ang, input.q16avel) & 0x7FFFFFF;

                if (input.q16avel)
                {
                    pPlayer->one_eighty_count = 0;
                }
            }
        }

        if (!(movementLocked & IL_NOHORIZ))
        {
            localInput.q16horz = fix16_clamp(fix16_sadd(localInput.q16horz, input.q16horz), F16(-MAXHORIZVEL), F16(MAXHORIZVEL));
            if (!synchronized_input)
                pPlayer->q16horiz  = fix16_clamp(fix16_sadd(pPlayer->q16horiz, input.q16horz), F16(HORIZ_MIN), F16(HORIZ_MAX));
        }
    }
    if (synchronized_input) return;

    // don't adjust rotscrnang and look_ang if dead.
    if (pSprite->extra > 0)
    {
        pPlayer->q16rotscrnang = fix16_ssub(pPlayer->q16rotscrnang, fix16_from_dbl(scaleAdjustmentToInterval(fix16_to_dbl(fix16_sdiv(pPlayer->q16rotscrnang, fix16_from_int(2))))));

        if (pPlayer->q16rotscrnang && !fix16_sdiv(pPlayer->q16rotscrnang, fix16_from_dbl(scaleAdjustmentToInterval(2))))
            pPlayer->q16rotscrnang = fix16_ssub(pPlayer->q16rotscrnang, fix16_from_dbl(scaleAdjustmentToInterval(ksgn(fix16_to_int(pPlayer->q16rotscrnang)))));

        pPlayer->q16look_ang = fix16_ssub(pPlayer->q16look_ang, fix16_from_dbl(scaleAdjustmentToInterval(fix16_to_dbl(fix16_sdiv(pPlayer->q16look_ang, fix16_from_int(4))))));

        if (pPlayer->q16look_ang && !fix16_sdiv(pPlayer->q16look_ang, fix16_from_dbl(scaleAdjustmentToInterval(4))))
            pPlayer->q16look_ang = fix16_ssub(pPlayer->q16look_ang, fix16_from_dbl(scaleAdjustmentToInterval(ksgn(fix16_to_int(pPlayer->q16look_ang)))));

        if (thisPlayer.lookLeft)
        {
            pPlayer->q16look_ang = fix16_ssub(pPlayer->q16look_ang, fix16_from_dbl(scaleAdjustmentToInterval(152)));
            pPlayer->q16rotscrnang = fix16_sadd(pPlayer->q16rotscrnang, fix16_from_dbl(scaleAdjustmentToInterval(24)));
        }
        if (thisPlayer.lookRight)
        {
            pPlayer->q16look_ang = fix16_sadd(pPlayer->q16look_ang, fix16_from_dbl(scaleAdjustmentToInterval(152)));
            pPlayer->q16rotscrnang = fix16_ssub(pPlayer->q16rotscrnang, fix16_from_dbl(scaleAdjustmentToInterval(24)));
        }

        if (pPlayer->one_eighty_count < 0)
        {
            pPlayer->one_eighty_count = -fix16_to_int(fix16_abs(GetDeltaQ16Angle(pPlayer->one_eighty_target, pPlayer->q16ang)));
            pPlayer->q16ang = fix16_sadd(pPlayer->q16ang, fix16_max(fix16_one, fix16_from_dbl(scaleAdjustmentToInterval(-pPlayer->one_eighty_count / ONEEIGHTYSCALE)))) & 0x7FFFFFF;
        }

        apply_seasick(pPlayer, scaleAdjust);
    }
    // A horiz diff of 128 equal 45 degrees, so we convert horiz to 1024 angle units

    if (thisPlayer.horizAngleAdjust)
    {
        float const horizAngle
        = atan2f(pPlayer->q16horiz - F16(100), F16(128)) * (512.f / fPI) + scaleAdjustmentToInterval(thisPlayer.horizAngleAdjust);
        pPlayer->q16horiz = F16(100) + Blrintf(F16(128) * tanf(horizAngle * (fPI / 512.f)));
    }
    else if (pPlayer->return_to_center > 0)
    {
        pPlayer->q16horiz = fix16_sadd(pPlayer->q16horiz, fix16_from_dbl(scaleAdjustmentToInterval(fix16_to_dbl(fix16_from_dbl(66.535) - fix16_sdiv(pPlayer->q16horiz, fix16_from_dbl(1.505))))));

        if (pPlayer->q16horiz >= F16(99) && pPlayer->q16horiz <= F16(101))
        {
            pPlayer->q16horiz = F16(100);
            pPlayer->return_to_center = 0;
        }

        if (pPlayer->q16horizoff >= F16(-1) && pPlayer->q16horizoff <= F16(1))
            pPlayer->q16horizoff = 0;
    }
 
    // calculates automatic view angle for playing without a mouse
    if (!pPlayer->aim_mode && pPlayer->on_ground && sectorLotag != ST_2_UNDERWATER && (sector[pPlayer->cursectnum].floorstat & 2))
    {
        // this is some kind of horse shit approximation of where the player is looking, I guess?
        vec2_t const adjustedPosition = { pPlayer->posx + (sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047] >> 5),
                                          pPlayer->posy + (sintable[fix16_to_int(pPlayer->q16ang) & 2047] >> 5) };
        int16_t currentSector = pPlayer->cursectnum;
 
        updatesector(adjustedPosition.x, adjustedPosition.y, &currentSector);
 
        if (currentSector >= 0)
        {
            int const slopeZ = getflorzofslope(pPlayer->cursectnum, adjustedPosition.x, adjustedPosition.y);
            if ((pPlayer->cursectnum == currentSector) || (klabs(getflorzofslope(currentSector, adjustedPosition.x, adjustedPosition.y) - slopeZ) <= ZOFFSET6))
                pPlayer->q16horizoff = fix16_sadd(pPlayer->q16horizoff, fix16_from_dbl(scaleAdjustmentToInterval(mulscale16(pPlayer->truefz - slopeZ, 160))));
        }
    }
 
    if (pPlayer->q16horizoff > 0)
    {
        pPlayer->q16horizoff = fix16_ssub(pPlayer->q16horizoff, fix16_from_dbl(scaleAdjustmentToInterval(fix16_to_dbl((pPlayer->q16horizoff >> 3) + fix16_one))));
        pPlayer->q16horizoff = fix16_max(pPlayer->q16horizoff, 0);
    }
    else if (pPlayer->q16horizoff < 0)
    {
        pPlayer->q16horizoff = fix16_sadd(pPlayer->q16horizoff, fix16_from_dbl(scaleAdjustmentToInterval(fix16_to_dbl((-pPlayer->q16horizoff >> 3) + fix16_one))));
        pPlayer->q16horizoff = fix16_min(pPlayer->q16horizoff, 0);
    }
 
    if (thisPlayer.horizSkew)
        pPlayer->q16horiz = fix16_sadd(pPlayer->q16horiz, fix16_from_dbl(scaleAdjustmentToInterval(thisPlayer.horizSkew)));
 
    pPlayer->q16horiz = fix16_clamp(pPlayer->q16horiz, F16(HORIZ_MIN), F16(HORIZ_MAX));
}


void P_GetInputMotorcycle(int playerNum)
{
    auto      &thisPlayer = g_player[playerNum];
    auto const pPlayer = &ps[playerNum];
    auto const pSprite = &sprite[pPlayer->i]; 
    ControlInfo info;
    double scaleAdjust = elapsedInputTicks * REALGAMETICSPERSEC / 1000.0;

	bool mouseaim = in_mousemode || buttonMap.ButtonDown(gamefunc_Mouse_Aiming);

    CONTROL_GetInput(&info);

    // JBF: Run key behaviour is selectable
    int const     playerRunning    = G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run));
    int const     keyMove          = playerRunning ? (NORMALKEYMOVE << 1) : NORMALKEYMOVE;

    input_t input {};

    input.q16avel = fix16_sadd(input.q16avel, fix16_from_float(info.mousex));
    input.q16avel = fix16_sadd(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(info.dyaw)));

    input.svel -= scaleAdjustmentToInterval(info.dx * keyMove);
    input.fvel -= scaleAdjustmentToInterval(info.dz * keyMove);

    pPlayer->crouch_toggle = 0;
    processCommonInput(input);

    if (buttonMap.ButtonDown(gamefunc_Dpad_Aiming))
        input.fvel = 0;
    
    int const turn = input.q16avel / 32;
    int turnLeft = buttonMap.ButtonDown(gamefunc_Turn_Left) || buttonMap.ButtonDown(gamefunc_Strafe_Left);
    int turnRight = buttonMap.ButtonDown(gamefunc_Turn_Right) || buttonMap.ButtonDown(gamefunc_Strafe_Right);
    int avelScale = F16((turnLeft || turnRight) ? 1 : 0);
    if (turn)
    {
        avelScale = fix16_max(avelScale, fix16_clamp(fix16_mul(turn, turn),0,F16(1)));
        if (turn < 0)
            turnLeft = 1;
        else if (turn > 0)
            turnRight = 1;
    }

    input.svel = input.fvel = input.q16avel = 0;

    localInput.bits |= turnLeft << SK_AIM_DOWN;
    localInput.bits |= turnRight << SK_LOOK_LEFT;

    int const moveBack = buttonMap.ButtonDown(gamefunc_Move_Backward) && pPlayer->MotoSpeed <= 0;

    // turn is truncated to integer precision to avoid having micro-movement affect the result, which makes a significant difference here.
    int turnvel = motoApplyTurn(pPlayer, turnLeft, turnRight, turn >> FRACBITS, moveBack, scaleAdjust);
    input.q16avel += int(turnvel * scaleAdjust * FRACUNIT * 2);

    if (pPlayer->moto_underwater)
    {
        pPlayer->MotoSpeed = 0;
    }
    else
    {
        localInput.bits |= (buttonMap.ButtonDown(gamefunc_Move_Forward) || buttonMap.ButtonDown(gamefunc_Strafe)) << SK_JUMP;
        localInput.bits |= buttonMap.ButtonDown(gamefunc_Move_Backward) << SK_AIM_UP;
        localInput.bits |= buttonMap.ButtonDown(gamefunc_Run) << SK_CROUCH;
    }

    input.fvel += pPlayer->MotoSpeed;
    input.q16avel = fix16_mul(input.q16avel, avelScale);

    int const movementLocked = P_CheckLockedMovement(playerNum);

    if ((ud.scrollmode && ud.overhead_on) || (movementLocked & IL_NOTHING) == IL_NOTHING)
    {
        if (ud.scrollmode && ud.overhead_on)
        {
            ud.folfvel = input.fvel;
            ud.folavel = fix16_to_int(input.q16avel);
        }

        localInput.fvel = localInput.svel = 0;
        localInput.q16avel = localInput.q16horz = 0;
    }
    else
    {
        if (!(movementLocked & IL_NOMOVE))
        {
            localInput.fvel = clamp(input.fvel, -(MAXVELMOTO / 8), MAXVELMOTO);
        }

        if (!(movementLocked & IL_NOANGLE))
        {
            localInput.q16avel = fix16_sadd(localInput.q16avel, input.q16avel);
            if (!synchronized_input) pPlayer->q16ang    = fix16_sadd(pPlayer->q16ang, input.q16avel) & 0x7FFFFFF;
        }
    }

    // don't adjust rotscrnang and look_ang if dead.
    if (pSprite->extra > 0 && !synchronized_input)
    {
        apply_seasick(pPlayer, scaleAdjust);
    }
}

void P_GetInputBoat(int playerNum)
{
    auto      &thisPlayer = g_player[playerNum];
    auto const pPlayer    = &ps[playerNum];
    auto const pSprite = &sprite[pPlayer->i];
    ControlInfo info;
    double scaleAdjust = elapsedInputTicks * REALGAMETICSPERSEC / 1000.0;

	bool mouseaim = in_mousemode || buttonMap.ButtonDown(gamefunc_Mouse_Aiming);

    CONTROL_GetInput(&info);

    // JBF: Run key behaviour is selectable
    int const     playerRunning    = G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run));
    constexpr int analogTurnAmount = (NORMALTURN << 1);
    int const     keyMove          = playerRunning ? (NORMALKEYMOVE << 1) : NORMALKEYMOVE;
    constexpr int analogExtent     = 32767; // KEEPINSYNC sdlayer.cpp

    input_t input {};

    input.q16avel = fix16_sadd(input.q16avel, fix16_from_float(info.mousex));
    input.q16avel = fix16_sadd(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(info.dyaw)));

    input.svel -= scaleAdjustmentToInterval(info.dx * keyMove);
    input.fvel -= scaleAdjustmentToInterval(info.dz * keyMove);

    pPlayer->crouch_toggle = 0;
    processCommonInput(input);

    if (buttonMap.ButtonDown(gamefunc_Dpad_Aiming))
        input.fvel = 0;
    
    int const turn = input.q16avel / 32;
    int turnLeft = buttonMap.ButtonDown(gamefunc_Turn_Left) || buttonMap.ButtonDown(gamefunc_Strafe_Left);
    int turnRight = buttonMap.ButtonDown(gamefunc_Turn_Right) || buttonMap.ButtonDown(gamefunc_Strafe_Right);
    int avelScale = F16((turnLeft || turnRight) ? 1 : 0);
    if (turn)
    {
        avelScale = fix16_max(avelScale, fix16_clamp(fix16_mul(turn, turn),0,F16(1)));
        if (turn < 0)
            turnLeft = 1;
        else if (turn > 0)
            turnRight = 1;
    }

    input.svel = input.fvel = input.q16avel = 0;

    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Move_Forward) || buttonMap.ButtonDown(gamefunc_Strafe)) << SK_JUMP;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Move_Backward) << SK_AIM_UP;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Run) << SK_CROUCH;

    localInput.bits |= turnLeft << SK_AIM_DOWN;
    localInput.bits |= turnRight << SK_LOOK_LEFT;

    static int32_t turnHeldTime;
    static int32_t lastInputClock;  // MED
    int32_t const  elapsedTics = (int32_t)totalclock - lastInputClock;

    // turn is truncated to integer precision to avoid having micro-movement affect the result, which makes a significant difference here.
    int turnvel = boatApplyTurn(pPlayer, turnLeft, turnRight, turn >> FRACBITS, scaleAdjust); 
    input.q16avel += int(turnvel * scaleAdjust * FRACUNIT * 2);

    input.fvel += pPlayer->MotoSpeed;
    input.q16avel = fix16_mul(input.q16avel, avelScale);

    int const movementLocked = P_CheckLockedMovement(playerNum);

    if ((ud.scrollmode && ud.overhead_on) || (movementLocked & IL_NOTHING) == IL_NOTHING)
    {
        if (ud.scrollmode && ud.overhead_on)
        {
            ud.folfvel = input.fvel;
            ud.folavel = fix16_to_int(input.q16avel);
        }

        localInput.fvel = localInput.svel = 0;
        localInput.q16avel = localInput.q16horz = 0;
    }
    else
    {
        if (!(movementLocked & IL_NOMOVE))
        {
            localInput.fvel = clamp(input.fvel, -(MAXVELMOTO / 8), MAXVELMOTO);
        }

        if (!(movementLocked & IL_NOANGLE))
        {
            localInput.q16avel = fix16_sadd(localInput.q16avel, input.q16avel);
            if (!synchronized_input) pPlayer->q16ang    = fix16_sadd(pPlayer->q16ang, input.q16avel) & 0x7FFFFFF;
        }
    }

    // don't adjust rotscrnang and look_ang if dead.
    if (pSprite->extra > 0 && !synchronized_input)
    {
        apply_seasick(pPlayer, scaleAdjust);
    }
}

void GetInput()
{
    static uint64_t lastCheck;

    auto const p = &ps[myconnectindex];
    updatePauseStatus();

    auto now = I_msTimeF();
    elapsedInputTicks = now - lastCheck;
    lastCheck = now;

    if (paused)
    {
        localInput = {};
        if (g_gameQuit) localInput.bits |= SKB_GAMEQUIT;
        return;
    }

    D_ProcessEvents();
    if (numplayers == 1)
    {
        setlocalplayerinput(p);
    }

    if (isRRRA() && p->OnMotorcycle)
        P_GetInputMotorcycle(myconnectindex);
    else if (isRRRA() && p->OnBoat)
        P_GetInputBoat(myconnectindex);
    else
        P_GetInput(myconnectindex);
}
END_DUKE_NS
