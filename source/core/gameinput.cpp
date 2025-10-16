//-------------------------------------------------------------------------
/*
Copyright (C) 2019 Christoph Oelckers
Copyright (C) 2020 Mitchell Richters

This is free software; you can redistribute it and/or
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

*/
//------------------------------------------------------------------------- 

#include "menu.h"
#include "gameinput.h"
#include "g_input.h"

//---------------------------------------------------------------------------
//
// CVARs to control input.
//
//---------------------------------------------------------------------------

CVAR(Bool, cl_noturnscaling, false, CVAR_GLOBALCONFIG | CVAR_ARCHIVE);
CVAR(Float, m_pitch, 1.f, CVAR_GLOBALCONFIG | CVAR_ARCHIVE)
CVAR(Float, m_yaw, 1.f, CVAR_GLOBALCONFIG | CVAR_ARCHIVE)
CVAR(Float, m_forward, 1.f, CVAR_GLOBALCONFIG | CVAR_ARCHIVE)
CVAR(Float, m_side, 1.f, CVAR_GLOBALCONFIG | CVAR_ARCHIVE)


//---------------------------------------------------------------------------
//
// Input constants used throughout associated functions.
//
//---------------------------------------------------------------------------

/*
// Preamble.
Blood:   N/A;
Exhumed: N/A;
Duke:       5 * 2 * 30 = 300;
SW:   3 * 1.40625 * 40 = 168.75; // Precisely, ((((3 * 12) + ((3 * 12) / 4)) * 3) / 32) * 40
Average: 234.375;

// Normal speed.
Blood:     92 / 4 * 30 = 690;
Duke:      15 * 2 * 30 = 900;
SW:  18 * 1.40625 * 40 = 1012.5; // Precisely, (((((12 + 6) * 12) + (((12 + 6) * 12) / 4)) * 3) / 32) * 40
Exhumed:    8 * 4 * 30 = 960;
Average: 890.625;

// Running speed.
Blood: 92 / 4 * 2 * 30 = 1380;
Duke:  15 * 2 * 2 * 30 = 1800;
SW:  28 * 1.40625 * 40 = 1575;   // Precisely, ((((28 * 12) + ((28 * 12) / 4)) * 3) / 32) * 40
Exhumed:   12 * 4 * 30 = 1440;
Average: 1548.75;
*/
static constexpr double YAW_TURNSPEEDS[3] = {
	((mapangle(5. * 2.).Degrees() * 30.) + (mapangle((((3. * 12.) + ((3. * 12.) * (1. / 4.))) * 3.) * (1. / 32.)).Degrees() * 40.)) * 0.5,
	((mapangle(92. * (1. / 4.)).Degrees() * 30.) + (mapangle(15. * 2.).Degrees() * 30.) + (mapangle(((((12. + 6.) * 12.) + (((12. + 6.) * 12.) * (1. / 4.))) * 3.) * (1. / 32.)).Degrees() * 40.) + (mapangle(8. * 4.).Degrees() * 30.)) * 0.25,
	((mapangle(92. * (1. / 4.) * 2.).Degrees() * 30.) + (mapangle(15. * 2. * 2.).Degrees() * 30.) + (mapangle((((28. * 12.) + ((28. * 12.) * (1. / 4.))) * 3.) * (1. / 32.)).Degrees() * 40.) + (mapangle(12. * 4.).Degrees() * 30.)) * 0.25
};
static constexpr double PRETURBOTURNSCALE = YAW_TURNSPEEDS[0] / YAW_TURNSPEEDS[1];
static constexpr DVector3 MAXVEL[3] = { { 0., 0., 1. }, { 1., 1., 1. }, { 2., 2., 1. } };
static constexpr DRotator MAXANG = { DAngle90 - minAngle, DAngle180 - minAngle, DAngle180 - minAngle };
static constexpr DAngle MOUSE_SCALE = DAngle::fromDeg(1. / 16.);


//---------------------------------------------------------------------------
//
// Initialised variables.
//
//---------------------------------------------------------------------------

GameInput gameInput{};
bool crouch_toggle = false;


//---------------------------------------------------------------------------
//
// Default player movement function for the games. Can be overridden.
//
//---------------------------------------------------------------------------

void GameInterface::doPlayerMovement()
{
	gameInput.processMovement();
}


//---------------------------------------------------------------------------
//
// Clears crouch toggle state for new games.
//
//---------------------------------------------------------------------------

void GameInput::resetCrouchToggle()
{
	crouch_toggle = false;
}


//---------------------------------------------------------------------------
//
// Player's movement function, called from game's ticker or from gi->doPlayerMovement() as required.
//
//---------------------------------------------------------------------------

void GameInput::processMovement(const double turnscale, const bool allowstrafe, const int drink_amt)
{
	// set up variables.
	InputPacket thisInput{};
	keymove = 1 << int(!!(inputBuffer.actions & SB_RUN));
	const auto hidspeed = getTicrateAngle(YAW_TURNSPEEDS[2]);

	// get all input amounts.
	const auto turning = buttonMap.ButtonDown(gamefunc_Turn_Right) -
		buttonMap.ButtonDown(gamefunc_Turn_Left);

	const auto moving = buttonMap.ButtonDown(gamefunc_Move_Forward) -
		buttonMap.ButtonDown(gamefunc_Move_Backward) +
		joyAxes[JOYAXIS_Forward] * scaleAdjust;

	const auto strafing = buttonMap.ButtonDown(gamefunc_Strafe_Right) -
		buttonMap.ButtonDown(gamefunc_Strafe_Left) -
		joyAxes[JOYAXIS_Side] * scaleAdjust;

	const auto soaring = buttonMap.ButtonDown(gamefunc_Move_Up) -
		buttonMap.ButtonDown(gamefunc_Move_Down) +
		joyAxes[JOYAXIS_Up] * scaleAdjust;

	// process player yaw input.
	if (!(buttonMap.ButtonDown(gamefunc_Strafe) && allowstrafe))
	{
		const double turndir = clamp(turning + strafing * !allowstrafe, -1., 1.);
		const double tttscale = (cl_noturnscaling || isTurboTurnTime()) ? 1 : PRETURBOTURNSCALE;
		const DAngle turnspeed = getTicrateAngle(YAW_TURNSPEEDS[keymove] * tttscale);
		thisInput.ang.Yaw += MOUSE_SCALE * mouseInput.X * m_yaw;
		thisInput.ang.Yaw -= hidspeed * joyAxes[JOYAXIS_Yaw] * scaleAdjust;
		thisInput.ang.Yaw += turnspeed * turndir * scaleAdjust;
		thisInput.ang.Yaw *= turnscale;
		if (turndir) updateTurnHeldAmt(); else turnheldtime = 0;
	}
	else
	{
		thisInput.vel.Y += mouseInput.X * MOUSE_SCALE.Degrees() * m_side;
		thisInput.vel.Y -= joyAxes[JOYAXIS_Yaw] * keymove * scaleAdjust;
		thisInput.vel.Y += turning * keymove * scaleAdjust;
	}

	// process player pitch input.
	if (!(inputBuffer.actions & SB_AIMMODE))
	{
		thisInput.ang.Pitch -= MOUSE_SCALE * mouseInput.Y * m_pitch;
		thisInput.ang.Pitch -= hidspeed * joyAxes[JOYAXIS_Pitch] * scaleAdjust;
		thisInput.ang.Pitch *= turnscale;
	}
	else
	{
		thisInput.vel.X += mouseInput.Y * MOUSE_SCALE.Degrees() * m_forward;
		thisInput.vel.X += joyAxes[JOYAXIS_Pitch] * keymove * scaleAdjust;
	}

	// process movement input.
	thisInput.vel.X += moving * keymove;
	thisInput.vel.Y += strafing * keymove * allowstrafe;
	thisInput.vel.Z += soaring; // this isn't scaled by running.

	// process RR's drunk state.
	if (isRR() && drink_amt >= 66 && drink_amt <= 87)
	{
		thisInput.vel.Y += drink_amt & 1 ? -thisInput.vel.X : thisInput.vel.X;
	}

	// add collected input to game's local input accumulation packet.
	inputBuffer.vel += thisInput.vel;
	inputBuffer.ang += thisInput.ang;

	// directly update player angles if we can.
	if (scaleAdjust < 1)
	{
		PlayerArray[myconnectindex]->CameraAngles += thisInput.ang;
	}
}


//---------------------------------------------------------------------------
//
// Player's vehicle movement function.
//
//---------------------------------------------------------------------------

void GameInput::processVehicle(const double baseVel, const double velScale, const unsigned flags)
{
	// open up input packet for this session.
	InputPacket thisInput{};

	// mask out all actions not compatible with vehicles.
	inputBuffer.actions &= ~(SB_WEAPONMASK_BITS | SB_TURNAROUND | SB_CENTERVIEW | SB_HOLSTER | SB_JUMP | SB_CROUCH | SB_RUN | 
		SB_AIM_UP | SB_AIM_DOWN | SB_AIMMODE | SB_LOOK_UP | SB_LOOK_DOWN | SB_LOOK_LEFT | SB_LOOK_RIGHT);

	if ((keymove = !!(flags & VEH_CANMOVE)))
	{
		const auto kbdForwards = buttonMap.ButtonDown(gamefunc_Move_Forward) || buttonMap.ButtonDown(gamefunc_Strafe);
		const auto kbdBackward = buttonMap.ButtonDown(gamefunc_Move_Backward);
		thisInput.vel.X = kbdForwards - kbdBackward + joyAxes[JOYAXIS_Forward];
		inputBuffer.vel.X += thisInput.vel.X;

		// This sync bit is the brake key.
		if (buttonMap.ButtonDown(gamefunc_Run)) inputBuffer.actions |= SB_CROUCH;
	}

	if (flags & VEH_CANTURN)
	{
		// Keyboard turning.
		const auto kbdLeft = buttonMap.ButtonDown(gamefunc_Turn_Left) || buttonMap.ButtonDown(gamefunc_Strafe_Left);
		const auto kbdRight = buttonMap.ButtonDown(gamefunc_Turn_Right) || buttonMap.ButtonDown(gamefunc_Strafe_Right);
		const auto kbdDir = kbdRight - kbdLeft;

		// Input device turning.
		const auto hidLeft = mouseInput.X < 0 || joyAxes[JOYAXIS_Yaw] > 0;
		const auto hidRight = mouseInput.X > 0 || joyAxes[JOYAXIS_Yaw] < 0;
		const auto hidDir = hidRight - hidLeft;

		// Velocity setup.
		const auto scaleVel = !(flags & VEH_SCALETURN) && (cl_noturnscaling || hidDir || isTurboTurnTime());
		const auto turnVel = scaleVel ? baseVel : baseVel * velScale;
		const auto mouseVel = abs(turnVel * mouseInput.X * m_yaw) * (45. / 2048.) / scaleAdjust;

		// Apply inputs.
		thisInput.ang.Yaw += DAngle::fromDeg(((mouseVel > 1) ? g_sqrt(mouseVel) : mouseVel) * Sgn(turnVel) * Sgn(mouseInput.X) * Sgn(m_yaw));
		thisInput.ang.Yaw -= DAngle::fromDeg(turnVel * joyAxes[JOYAXIS_Yaw] - turnVel * kbdDir);
		thisInput.ang.Yaw *= scaleAdjust;
		inputBuffer.ang.Yaw += thisInput.ang.Yaw;
		if (kbdDir) updateTurnHeldAmt(); else turnheldtime = 0;
	}
	else
	{
		turnheldtime = 0;
	}

	// directly update player angles if we can.
	if (scaleAdjust < 1)
	{
		PlayerArray[myconnectindex]->CameraAngles += thisInput.ang;
	}
}


//---------------------------------------------------------------------------
//
// Processes all the input bits.
//
//---------------------------------------------------------------------------

void GameInput::processInputBits()
{
	if (WeaponToSend != 0) inputBuffer.setNewWeapon(WeaponToSend);
	WeaponToSend = 0;
	if (buttonMap.ButtonDown(gamefunc_Dpad_Select))
	{
		// These buttons should not autorepeat. The game handlers are not really equipped for that.
		if (joyAxes[JOYAXIS_Forward] > 0 && !(dpad_lock & 1)) { dpad_lock |= 1;  inputBuffer.setNewWeapon(WeaponSel_Prev); }
		else dpad_lock &= ~1;
		if (joyAxes[JOYAXIS_Forward] < 0 && !(dpad_lock & 2)) { dpad_lock |= 2;  inputBuffer.setNewWeapon(WeaponSel_Next); }
		else dpad_lock &= ~2;
		if ((joyAxes[JOYAXIS_Side] < 0 || joyAxes[JOYAXIS_Yaw] > 0) && !(dpad_lock & 4)) { dpad_lock |= 4;  inputBuffer.actions |= SB_INVPREV; }
		else dpad_lock &= ~4;
		if ((joyAxes[JOYAXIS_Side] > 0 || joyAxes[JOYAXIS_Yaw] < 0) && !(dpad_lock & 8)) { dpad_lock |= 8;  inputBuffer.actions |= SB_INVNEXT; }
		else dpad_lock &= ~8;

		// This eats the controller input for regular use
		joyAxes[JOYAXIS_Side] = 0;
		joyAxes[JOYAXIS_Forward] = 0;
		joyAxes[JOYAXIS_Yaw] = 0;
	}
	else dpad_lock = 0;

	const auto crouchFlags = PlayerArray[myconnectindex]->getCrouchFlags();
	inputBuffer.actions |= ActionsToSend;
	ActionsToSend = 0;

	if (buttonMap.ButtonDown(gamefunc_Aim_Up) || (buttonMap.ButtonDown(gamefunc_Dpad_Aiming) && joyAxes[JOYAXIS_Forward] > 0)) 
	{
		inputBuffer.actions |= SB_AIM_UP;
		inputBuffer.actions &= ~SB_CENTERVIEW;
	}

	if ((buttonMap.ButtonDown(gamefunc_Aim_Down) || (buttonMap.ButtonDown(gamefunc_Dpad_Aiming) && joyAxes[JOYAXIS_Forward] < 0))) 
	{
		inputBuffer.actions |= SB_AIM_DOWN;
		inputBuffer.actions &= ~SB_CENTERVIEW;
	}

	if (buttonMap.ButtonDown(gamefunc_Toggle_Crouch))
	{
		const bool canCrouch = crouchFlags & CS_CANCROUCH;
		crouch_toggle = !crouch_toggle && canCrouch;
		if (canCrouch) buttonMap.ClearButton(gamefunc_Toggle_Crouch);
	}

	if (buttonMap.ButtonDown(gamefunc_Crouch) || buttonMap.ButtonDown(gamefunc_Jump) || (crouchFlags & CS_DISABLETOGGLE))
		crouch_toggle = false;

	if (buttonMap.ButtonDown(gamefunc_Crouch) || buttonMap.ButtonDown(gamefunc_Toggle_Crouch) || crouch_toggle)
		inputBuffer.actions |= SB_CROUCH;

	if (buttonMap.ButtonDown(gamefunc_Dpad_Aiming))
		joyAxes[JOYAXIS_Forward] = 0;

	if (buttonMap.ButtonDown(gamefunc_Jump))
		inputBuffer.actions |= SB_JUMP;

	if (buttonMap.ButtonDown(gamefunc_Fire))
		inputBuffer.actions |= SB_FIRE;

	if (buttonMap.ButtonDown(gamefunc_Alt_Fire))
		inputBuffer.actions |= SB_ALTFIRE;

	if (buttonMap.ButtonDown(gamefunc_Open))
	{
		if (isBlood() || isExhumed()) buttonMap.ClearButton(gamefunc_Open);
		inputBuffer.actions |= SB_OPEN;
	}

	if (G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run)))
		inputBuffer.actions |= SB_RUN;

	if (!in_mousemode && !buttonMap.ButtonDown(gamefunc_Mouse_Aiming)) 
		inputBuffer.actions |= SB_AIMMODE;

	if (buttonMap.ButtonDown(gamefunc_Look_Up)) 
		inputBuffer.actions |= SB_LOOK_UP;

	if (buttonMap.ButtonDown(gamefunc_Look_Down)) 
		inputBuffer.actions |= SB_LOOK_DOWN;

	if (buttonMap.ButtonDown(gamefunc_Look_Left)) 
		inputBuffer.actions |= SB_LOOK_LEFT;

	if (buttonMap.ButtonDown(gamefunc_Look_Right)) 
		inputBuffer.actions |= SB_LOOK_RIGHT;

	if (buttonMap.ButtonDown(gamefunc_Quick_Kick))
		inputBuffer.actions |= SB_QUICK_KICK;
}


//---------------------------------------------------------------------------
//
// Processes input and returns a packet if provided.
//
//---------------------------------------------------------------------------

void GameInput::getInput(InputPacket* packet)
{
	I_GetEvent();
	SendAction(PlayerArray[myconnectindex]->cmd.ucmd.actions & SB_CENTERVIEW);

	if (M_Active() || gamestate != GS_LEVEL)
	{
		inputBuffer = {};
		return;
	}

	I_GetAxes(joyAxes);
	processInputBits();
	if (!paused) gi->doPlayerMovement();
	mouseInput.Zero();

	if (packet)
	{
		const DVector3& maxVel = MAXVEL[keymove];
		*packet = {	clamp(inputBuffer.vel, -maxVel, maxVel), clamp(inputBuffer.ang, -MAXANG, MAXANG), inputBuffer.actions };
		inputBuffer = {};
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CCMD(slot)
{
	// The max differs between games so we have to handle this here.
	const bool isDukeShareware = (g_gameType & (GAMEFLAG_DUKE | GAMEFLAG_SHAREWARE)) == (GAMEFLAG_DUKE | GAMEFLAG_SHAREWARE);
	const int max = isExhumed() || isDukeShareware ? WeaponSel_MaxExhumed : isBlood() ? WeaponSel_MaxBlood : WeaponSel_Max;

	if (argv.argc() != 2)
	{
		Printf("slot <weaponslot>: select a weapon from the given slot (1-%d)", max);
		return;
	}

	const auto slot = atoi(argv[1]);

	if (slot >= 1 && slot <= max)
	{
		gameInput.SendWeapon(slot);
	}
}

CCMD(weapprev)
{
	gameInput.SendWeapon(WeaponSel_Prev);
}

CCMD(weapnext)
{
	gameInput.SendWeapon(WeaponSel_Next);
}

CCMD(weapalt)
{
	gameInput.SendWeapon(WeaponSel_Alt);	// Only used by SW - should also be made usable by Blood ans Duke which put multiple weapons in the same slot.
}

CCMD(useitem)
{
	const int max = isExhumed() ? 6 : isSWALL() ? 7 : isBlood() ? 4 : 5;

	if (argv.argc() != 2)
	{
		Printf("useitem <itemnum>: activates an inventory item (1-%d)", max);
		return;
	}

	const auto slot = atoi(argv[1]);

	if (slot >= 1 && slot <= max)
	{
		gameInput.SendAction(ESyncBits::FromInt(SB_ITEM_BIT_1 << (slot - 1)));
	}
}

CCMD(invprev)
{
	gameInput.SendAction(SB_INVPREV);
}

CCMD(invnext)
{
	gameInput.SendAction(SB_INVNEXT);
}

CCMD(invuse)
{
	gameInput.SendAction(SB_INVUSE);
}

CCMD(centerview)
{
	gameInput.SendAction(SB_CENTERVIEW);
}

CCMD(turnaround)
{
	gameInput.SendAction(SB_TURNAROUND);
}

CCMD(holsterweapon)
{
	gameInput.SendAction(SB_HOLSTER);
}

CCMD(third_person_view)
{
	gi->ToggleThirdPerson();
}

CCMD(coop_view)
{
	gi->SwitchCoopView();
}

CCMD(show_weapon)
{
	gi->ToggleShowWeapon();
}
