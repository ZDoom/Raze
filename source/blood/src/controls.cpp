//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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

#include "compat.h"
#include "baselayer.h"
#include "mmulti.h"
#include "gamecontrol.h"
#include "common_game.h"
#include "blood.h"
#include "controls.h"
#include "globals.h"
#include "levels.h"
#include "map2d.h"
#include "view.h"
#include "d_event.h"
#include "gamestate.h"
#include "sound.h"

BEGIN_BLD_NS

GINPUT gInput, gNetInput;
bool bSilentAim = false;

int iTurnCount = 0;
static int WeaponToSend;
static KEYFLAGS BitsToSend;
static USEFLAGS UsesToSend;

void ctrlInit(void)
{
}

void ctrlTerm(void)
{
}

int32_t mouseyaxismode = -1;

int32_t GetTime(void)
{
    return gameclock;
}

fix16_t gViewLook, gViewAngle;
float gViewAngleAdjust;
float gViewLookAdjust;
int gViewLookRecenter;

void LocalKeys(void)
{
    if (buttonMap.ButtonDown(gamefunc_Third_Person_View))
    {
        buttonMap.ClearButton(gamefunc_Third_Person_View);
        if (gViewPos > VIEWPOS_0)
            gViewPos = VIEWPOS_0;
        else
            gViewPos = VIEWPOS_1;
    }
    if (buttonMap.ButtonDown(gamefunc_See_Coop_View))
    {
        buttonMap.ClearButton(gamefunc_See_Coop_View);
        if (gGameOptions.nGameType == 1)
        {
            gViewIndex = connectpoint2[gViewIndex];
            if (gViewIndex == -1)
                gViewIndex = connecthead;
            gView = &gPlayer[gViewIndex];
        }
        else if (gGameOptions.nGameType == 3)
        {
            int oldViewIndex = gViewIndex;
            do
            {
                gViewIndex = connectpoint2[gViewIndex];
                if (gViewIndex == -1)
                    gViewIndex = connecthead;
                if (oldViewIndex == gViewIndex || gMe->teamId == gPlayer[gViewIndex].teamId)
                    break;
            } while (oldViewIndex != gViewIndex);
            gView = &gPlayer[gViewIndex];
        }
    }
}



void ctrlGetInput(void)
{
    int prevPauseState = paused;
    ControlInfo info;

    static double lastInputTicks;
    auto const    currentHiTicks    = timerGetHiTicks();
    double const  elapsedInputTicks = currentHiTicks - lastInputTicks;

    lastInputTicks = currentHiTicks;

    auto scaleAdjustmentToInterval = [=](double x) { return x * kTicsPerSec / (1000.0 / elapsedInputTicks); };

    if (gamestate != GS_LEVEL || System_WantGuiCapture())
    {
        gInput = {};
        CONTROL_GetInput(&info);
        return;
    }

    if (paused != prevPauseState)
    {
        gInput.keyFlags.pause = 1;
    }

    if (paused)
        return;

    GINPUT input = {};

	bool mouseaim = in_mousemode || buttonMap.ButtonDown(gamefunc_Mouse_Aiming);
	if (!mouseaim) gInput.keyFlags.lookCenter = 1;

    if (numplayers == 1)
    {
        gProfile[myconnectindex].nAutoAim = cl_autoaim;
        gProfile[myconnectindex].nWeaponSwitch = cl_weaponswitch;
    }

    CONTROL_GetInput(&info);

    if (gQuitRequest)
        gInput.keyFlags.quit = 1;

    gInput.keyFlags.word |= BitsToSend.word;
    gInput.useFlags.byte |= UsesToSend.byte;
    if (WeaponToSend != 0) 
        gInput.newWeapon = WeaponToSend;

    BitsToSend.word = 0;
    UsesToSend.byte = 0;
    WeaponToSend = 0;

    if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
    {
        if (automapMode != am_off)
        {
            gZoom = ClipLow(gZoom - (gZoom >> 4), 64);
            gViewMap.nZoom = gZoom;
        }
    }

    if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
    {
        if (automapMode != am_off)
        {
            gZoom = ClipHigh(gZoom + (gZoom >> 4), 4096);
            gViewMap.nZoom = gZoom;
        }
    }

    if (gPlayer[myconnectindex].nextWeapon == 0)
    {
    }

    if (buttonMap.ButtonDown(gamefunc_Show_Opponents_Weapon))
    {
        buttonMap.ClearButton(gamefunc_Show_Opponents_Weapon);
        cl_showweapon = (cl_showweapon + 1) & 3;
    }

    if (buttonMap.ButtonDown(gamefunc_Jump))
        gInput.buttonFlags.jump = 1;

    if (buttonMap.ButtonDown(gamefunc_Crouch))
        gInput.buttonFlags.crouch = 1;

    if (buttonMap.ButtonDown(gamefunc_Fire))
        gInput.buttonFlags.shoot = 1;

    if (buttonMap.ButtonDown(gamefunc_Alt_Fire))
        gInput.buttonFlags.shoot2 = 1;

    if (buttonMap.ButtonDown(gamefunc_Open))
    {
        buttonMap.ClearButton(gamefunc_Open);
        gInput.keyFlags.action = 1;
    }

    gInput.buttonFlags.lookUp |= buttonMap.ButtonDown(gamefunc_Look_Up);
    gInput.buttonFlags.lookDown |= buttonMap.ButtonDown(gamefunc_Look_Down);

    if (buttonMap.ButtonDown(gamefunc_Look_Up) || buttonMap.ButtonDown(gamefunc_Look_Down))
        gInput.keyFlags.lookCenter = 1;
    else
    {
        gInput.buttonFlags.lookUp |= buttonMap.ButtonDown(gamefunc_Aim_Up);
        gInput.buttonFlags.lookDown |= buttonMap.ButtonDown(gamefunc_Aim_Down);
    }

    int const run = G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run));
    int const run2 = false; // What??? buttonMap.ButtonDown(gamefunc_Run);
    int const keyMove = (1 + run) << 10;

    gInput.syncFlags.run |= run;

    if (gInput.forward < keyMove && gInput.forward > -keyMove)
    {
        if (buttonMap.ButtonDown(gamefunc_Move_Forward))
            input.forward += keyMove;

        if (buttonMap.ButtonDown(gamefunc_Move_Backward))
            input.forward -= keyMove;
    }

    if (gInput.strafe < keyMove && gInput.strafe > -keyMove)
    {
        if (buttonMap.ButtonDown(gamefunc_Strafe_Left))
            input.strafe += keyMove;
        if (buttonMap.ButtonDown(gamefunc_Strafe_Right))
            input.strafe -= keyMove;
    }


    char turnLeft = 0, turnRight = 0;

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        if (gInput.strafe < keyMove && gInput.strafe > -keyMove)
        {
            if (buttonMap.ButtonDown(gamefunc_Turn_Left))
                input.strafe += keyMove;
            if (buttonMap.ButtonDown(gamefunc_Turn_Right))
                input.strafe -= keyMove;
        }
    }
    else
    {
        if (buttonMap.ButtonDown(gamefunc_Turn_Left))
            turnLeft = 1;
        if (buttonMap.ButtonDown(gamefunc_Turn_Right))
            turnRight = 1;
    }

    static int32_t turnHeldTime;
    static int32_t lastInputClock;  // MED
    int32_t const  elapsedTics = gameclock - lastInputClock;

    // Blood's q16mlook scaling is different from the other games, therefore use the below constant to attenuate
    // the speed to match the other games.
    float const mlookScale = 3.25f;

    lastInputClock = gameclock;

    if (turnLeft || turnRight)
        turnHeldTime += elapsedTics;
    else
        turnHeldTime = 0;

    if (turnLeft)
        input.q16turn = fix16_ssub(input.q16turn, fix16_from_dbl(scaleAdjustmentToInterval(ClipHigh(12 * turnHeldTime, gTurnSpeed)>>2)));
    if (turnRight)
        input.q16turn = fix16_sadd(input.q16turn, fix16_from_dbl(scaleAdjustmentToInterval(ClipHigh(12 * turnHeldTime, gTurnSpeed)>>2)));

    if ((run2 || run) && turnHeldTime > 24)
        input.q16turn <<= 1;

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        input.strafe -= info.mousex * 32.f;
        input.strafe -= scaleAdjustmentToInterval(info.dyaw * keyMove);
    }
    else
    {
        input.q16turn = fix16_sadd(input.q16turn, fix16_from_float(info.mousex));
        input.q16turn = fix16_sadd(input.q16turn, fix16_from_dbl(scaleAdjustmentToInterval(info.dyaw)));
    }

    input.strafe  -= scaleAdjustmentToInterval(info.dx * keyMove);
    input.forward -= scaleAdjustmentToInterval(info.dz * keyMove);

    if (mouseaim)
        input.q16mlook = fix16_sadd(input.q16mlook, fix16_from_float(info.mousey / mlookScale));
    else
        input.forward -= info.mousey * 64.f;
    if (!in_mouseflip)
        input.q16mlook = -input.q16mlook;

    input.q16mlook = fix16_ssub(input.q16mlook, fix16_from_dbl(scaleAdjustmentToInterval(info.dpitch / mlookScale)));

    if (!automapFollow && automapMode != am_off)
    {
        gViewMap.turn += input.q16turn<<2;
        gViewMap.forward += input.forward;
        gViewMap.strafe += input.strafe;
        input.q16turn = 0;
        input.forward = 0;
        input.strafe = 0;
    }
    gInput.forward = clamp(gInput.forward + input.forward, -2048, 2048);
    gInput.strafe = clamp(gInput.strafe + input.strafe, -2048, 2048);
    gInput.q16turn = fix16_sadd(gInput.q16turn, input.q16turn);
    gInput.q16mlook = fix16_clamp(fix16_sadd(gInput.q16mlook, input.q16mlook), fix16_from_int(-127)>>2, fix16_from_int(127)>>2);
    if (gMe && gMe->pXSprite->health != 0 && !paused)
    {
        int upAngle = 289;
        int downAngle = -347;
        double lookStepUp = 4.0*upAngle/60.0;
        double lookStepDown = -4.0*downAngle/60.0;
        gViewAngle = (gViewAngle + input.q16turn + fix16_from_dbl(scaleAdjustmentToInterval(gViewAngleAdjust))) & 0x7ffffff;
        if (gViewLookRecenter)
        {
            if (gViewLook < 0)
                gViewLook = fix16_min(gViewLook+fix16_from_dbl(scaleAdjustmentToInterval(lookStepDown)), fix16_from_int(0));
            if (gViewLook > 0)
                gViewLook = fix16_max(gViewLook-fix16_from_dbl(scaleAdjustmentToInterval(lookStepUp)), fix16_from_int(0));
        }
        else
        {
            gViewLook = fix16_clamp(gViewLook+fix16_from_dbl(scaleAdjustmentToInterval(gViewLookAdjust)), fix16_from_int(downAngle), fix16_from_int(upAngle));
        }
        gViewLook = fix16_clamp(gViewLook+(input.q16mlook << 3), fix16_from_int(downAngle), fix16_from_int(upAngle));
    }
}

//---------------------------------------------------------------------------
//
// CCMD based input. The basics are from Randi's ZDuke but this uses dynamic
// registration to only have the commands active when this game module runs.
//
//---------------------------------------------------------------------------

static int ccmd_slot(CCmdFuncPtr parm)
{
    if (parm->numparms != 1) return CCMD_SHOWHELP;

    auto slot = atoi(parm->parms[0]);
    if (slot >= 1 && slot <= 10)
    {
        WeaponToSend = slot;
        return CCMD_OK;
    }
    return CCMD_SHOWHELP;
}

void registerinputcommands()
{
    C_RegisterFunction("slot", "slot <weaponslot>: select a weapon from the given slot (1-10)", ccmd_slot);
    C_RegisterFunction("weapprev", nullptr, [](CCmdFuncPtr)->int { if (gPlayer[myconnectindex].nextWeapon == 0) BitsToSend.prevWeapon = 1; return CCMD_OK; });
    C_RegisterFunction("weapnext", nullptr, [](CCmdFuncPtr)->int { if (gPlayer[myconnectindex].nextWeapon == 0) BitsToSend.nextWeapon = 1; return CCMD_OK; });
    C_RegisterFunction("pause", nullptr, [](CCmdFuncPtr)->int { BitsToSend.pause = 1; sendPause = true; return CCMD_OK; });
    C_RegisterFunction("proximitybombs", nullptr, [](CCmdFuncPtr)->int { WeaponToSend = 11; return CCMD_OK; });
    C_RegisterFunction("remotebombs", nullptr, [](CCmdFuncPtr)->int { WeaponToSend = 12; return CCMD_OK; });
    C_RegisterFunction("jumpboots", nullptr, [](CCmdFuncPtr)->int { UsesToSend.useJumpBoots = 1; return CCMD_OK; });
    C_RegisterFunction("medkit", nullptr, [](CCmdFuncPtr)->int { UsesToSend.useMedKit = 1; return CCMD_OK; });
    C_RegisterFunction("centerview", nullptr, [](CCmdFuncPtr)->int { BitsToSend.lookCenter = 1; return CCMD_OK; });
    C_RegisterFunction("holsterweapon", nullptr, [](CCmdFuncPtr)->int { BitsToSend.holsterWeapon = 1; return CCMD_OK; });
    C_RegisterFunction("invprev", nullptr, [](CCmdFuncPtr)->int { BitsToSend.prevItem = 1; return CCMD_OK; });
    C_RegisterFunction("invnext", nullptr, [](CCmdFuncPtr)->int { BitsToSend.nextItem = 1; return CCMD_OK; });
    C_RegisterFunction("crystalball", nullptr, [](CCmdFuncPtr)->int { UsesToSend.useCrystalBall = 1; return CCMD_OK; });
    C_RegisterFunction("beastvision", nullptr, [](CCmdFuncPtr)->int { UsesToSend.useBeastVision = 1; return CCMD_OK; });
    C_RegisterFunction("turnaround", nullptr, [](CCmdFuncPtr)->int { BitsToSend.spin180 = 1; return CCMD_OK; });
    C_RegisterFunction("invuse", nullptr, [](CCmdFuncPtr)->int { BitsToSend.useItem = 1; return CCMD_OK; });
}

// This is called from ImputState::ClearAllInput and resets all static state being used here.
void GameInterface::clearlocalinputstate()
{
    WeaponToSend = 0;
    BitsToSend.word = 0;
    UsesToSend.byte = 0;
}


END_BLD_NS
