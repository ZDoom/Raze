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
#include "gamestate.h"
#include "gameinput.h"

EXTERN_CVAR(Float, m_sensitivity_x)
EXTERN_CVAR(Float, m_yaw)


//---------------------------------------------------------------------------
//
// Static constants used throughout functions.
//
//---------------------------------------------------------------------------

enum
{
	BUILDTICRATE = 120,
	TURBOTURNBASE = 590,
};

static InputPacket inputBuffer{};
static double turnheldtime = 0;
static int WeaponToSend = 0;
static int dpad_lock = 0;

static constexpr double YAW_TURNSPEEDS[3] = { 41.1987304, 156.555175, 272.24121 };
static constexpr double YAW_PREAMBLESCALE = YAW_TURNSPEEDS[0] / YAW_TURNSPEEDS[1];
static constexpr double YAW_LOOKINGSPEED = 801.5625;
static constexpr double YAW_ROTATESPEED = 63.28125;
static constexpr double YAW_LOOKRETURN = 7.5;
static constexpr double YAW_SPINSTAND = 675.;
static constexpr double YAW_SPINCROUCH = YAW_SPINSTAND * 0.5;
static constexpr double PITCH_LOOKSPEED = 222.83185;
static constexpr double PITCH_AIMSPEED = PITCH_LOOKSPEED * 0.5;
static constexpr double PITCH_CENTERSPEED = 10.7375;
static constexpr double PITCH_HORIZOFFSPEED = 4.375;
static constexpr DAngle PITCH_CNTRSINEOFFSET = DAngle90 / 8.;
static constexpr DAngle PITCH_HORIZOFFCLIMB = DAngle::fromDeg(-38.);
static constexpr DAngle PITCH_HORIZOFFPUSH = DAngle::fromDeg(0.4476);


//---------------------------------------------------------------------------
//
// Input scale helper functions.
//
//---------------------------------------------------------------------------

static inline double getTicrateScale(const double value)
{
	return value / GameTicRate;
}

static inline DAngle getscaledangle(const DAngle angle, const double scale, const DAngle push)
{
	return (angle.Normalized180() * getTicrateScale(scale)) + push;
}

static inline bool scaletozero(DAngle& angle, const double scale, const DAngle push = DAngle::fromDeg(32. / 465.))
{
	const auto sgn = angle.Sgn();

	if (!sgn || sgn != (angle -= getscaledangle(angle, scale, push * sgn)).Sgn())
	{
		angle = nullAngle;
		return true;
	}
	return false;
}


//---------------------------------------------------------------------------
//
// Functions for determining whether its turbo turn time (turn key held for a number of tics).
//
//---------------------------------------------------------------------------

static inline void updateTurnHeldAmt(const double scaleAdjust)
{
	turnheldtime += getTicrateScale(BUILDTICRATE) * scaleAdjust;
}

static inline bool isTurboTurnTime()
{
	return turnheldtime >= getTicrateScale(TURBOTURNBASE);
}

static inline void resetTurnHeldAmt()
{
	turnheldtime = 0;
}


//---------------------------------------------------------------------------
//
// Player's movement function, called from game's ticker or from gi->GetInput() as required.
//
//---------------------------------------------------------------------------

void processMovement(HIDInput* const hidInput, InputPacket* const currInput, const double scaleAdjust, const int drink_amt, const bool allowstrafe, const double turnscale)
{
	// set up variables.
	const int keymove = 1 << int(!!(inputBuffer.actions & SB_RUN));
	const float hidspeed = float(getTicrateScale(YAW_TURNSPEEDS[2]) * turnscale);
	const float scaleAdjustf = float(scaleAdjust);

	// determine player input.
	const auto turning = buttonMap.ButtonDown(gamefunc_Turn_Right) - buttonMap.ButtonDown(gamefunc_Turn_Left);
	const auto moving = buttonMap.ButtonDown(gamefunc_Move_Forward) - buttonMap.ButtonDown(gamefunc_Move_Backward) + hidInput->joyaxes[JOYAXIS_Forward] * scaleAdjustf;
	const auto strafing = buttonMap.ButtonDown(gamefunc_Strafe_Right) - buttonMap.ButtonDown(gamefunc_Strafe_Left) - hidInput->joyaxes[JOYAXIS_Side] * scaleAdjustf;

	// process player angle input.
	if (!(buttonMap.ButtonDown(gamefunc_Strafe) && allowstrafe))
	{
		const float turndir = clamp(turning + strafing * !allowstrafe, -1.f, 1.f);
		const float turnspeed = float(getTicrateScale(YAW_TURNSPEEDS[keymove]) * turnscale * (isTurboTurnTime() ? 1. : YAW_PREAMBLESCALE));
		currInput->avel += hidInput->mouseturnx - (hidInput->joyaxes[JOYAXIS_Yaw] * hidspeed - turndir * turnspeed) * scaleAdjustf;
		if (turndir) updateTurnHeldAmt(scaleAdjust); else resetTurnHeldAmt();
	}
	else
	{
		currInput->svel += hidInput->mousemovex - (hidInput->joyaxes[JOYAXIS_Yaw] - turning) * keymove * scaleAdjustf;
	}

	// process player pitch input.
	if (!(inputBuffer.actions & SB_AIMMODE))
		currInput->horz -= hidInput->mouseturny + hidInput->joyaxes[JOYAXIS_Pitch] * hidspeed * scaleAdjustf;
	else
		currInput->fvel += hidInput->mousemovey + hidInput->joyaxes[JOYAXIS_Pitch] * keymove * scaleAdjustf;

	// process movement input.
	currInput->fvel += moving * keymove;
	currInput->svel += strafing * keymove * allowstrafe;

	// process RR's drunk state.
	if (isRR() && drink_amt >= 66 && drink_amt <= 87)
		currInput->svel += drink_amt & 1 ? -currInput->fvel : currInput->fvel;

	// add collected input to game's local input accumulation packet.
	inputBuffer.fvel = clamp(inputBuffer.fvel + currInput->fvel, -(float)keymove, (float)keymove);
	inputBuffer.svel = clamp(inputBuffer.svel + currInput->svel, -(float)keymove, (float)keymove);
	inputBuffer.avel = clamp(inputBuffer.avel + currInput->avel, -179.f, 179.f);
	inputBuffer.horz = clamp(inputBuffer.horz + currInput->horz, -179.f, 179.f);
}


//---------------------------------------------------------------------------
//
// Player's vehicle movement function.
//
//---------------------------------------------------------------------------

void processVehicleInput(HIDInput* const hidInput, InputPacket* const currInput, const double scaleAdjust, const float baseVel, const float velScale, const bool canMove, const bool canTurn, const bool attenuate)
{
	// mask out all actions not compatible with vehicles.
	inputBuffer.actions &= ~(SB_WEAPONMASK_BITS | SB_TURNAROUND | SB_CENTERVIEW | SB_HOLSTER | SB_JUMP | SB_CROUCH | SB_RUN | 
		SB_AIM_UP | SB_AIM_DOWN | SB_AIMMODE | SB_LOOK_UP | SB_LOOK_DOWN | SB_LOOK_LEFT | SB_LOOK_RIGHT);

	// Cancel out micro-movement
	if (fabs(hidInput->mouseturnx) < (m_sensitivity_x * m_yaw * backendinputscale() * 2.f)) 
		hidInput->mouseturnx = 0;

	// Yes, we need all these bools...
	const auto kbdForwards = buttonMap.ButtonDown(gamefunc_Move_Forward) || buttonMap.ButtonDown(gamefunc_Strafe);
	const auto kbdBackward = buttonMap.ButtonDown(gamefunc_Move_Backward);
	const auto kbdLeft = buttonMap.ButtonDown(gamefunc_Turn_Left) || buttonMap.ButtonDown(gamefunc_Strafe_Left);
	const auto kbdRight = buttonMap.ButtonDown(gamefunc_Turn_Right) || buttonMap.ButtonDown(gamefunc_Strafe_Right);
	const auto hidLeft = hidInput->mouseturnx < 0 || hidInput->joyaxes[JOYAXIS_Yaw] > 0;
	const auto hidRight = hidInput->mouseturnx > 0 || hidInput->joyaxes[JOYAXIS_Yaw] < 0;
	const auto turnDir = (kbdRight || hidRight) - (kbdLeft || hidLeft);

	if (canMove)
	{
		currInput->fvel = kbdForwards - kbdBackward + hidInput->joyaxes[JOYAXIS_Forward];
		if (buttonMap.ButtonDown(gamefunc_Run)) inputBuffer.actions |= SB_CROUCH;
	}

	if (canTurn && turnDir)
	{
		const bool noattenuate = (isTurboTurnTime() || hidLeft || hidRight) && !attenuate;
		const auto vel = (noattenuate) ? (baseVel) : (baseVel * velScale);

		currInput->avel = vel * -hidInput->joyaxes[JOYAXIS_Yaw];

		if (const auto kbdDir = kbdRight - kbdLeft)
		{
			currInput->avel += vel * kbdDir;
			updateTurnHeldAmt(scaleAdjust);
		}
		else
		{
			resetTurnHeldAmt();
		}

		if (hidInput->mouseturnx)
		{
			currInput->avel += sqrtf(abs(vel * hidInput->mouseturnx / (float)scaleAdjust) * (7.f / 20.f)) * Sgn(vel) * Sgn(hidInput->mouseturnx);
		}

		currInput->avel *= (float)scaleAdjust;
	}
	else
	{
		resetTurnHeldAmt();
	}

	inputBuffer.fvel = clamp(inputBuffer.fvel + currInput->fvel, -1.00f, 1.00f);
	inputBuffer.avel = clamp(inputBuffer.avel + currInput->avel, -179.f, 179.f);
}


//---------------------------------------------------------------------------
//
// Processes all the input bits.
//
//---------------------------------------------------------------------------

static void ApplyGlobalInput(HIDInput* const hidInput)
{
	inputState.GetMouseDelta(hidInput);
	if (use_joystick) I_GetAxes(hidInput->joyaxes);

	if (WeaponToSend != 0) inputBuffer.setNewWeapon(WeaponToSend);
	WeaponToSend = 0;
	if (hidInput && buttonMap.ButtonDown(gamefunc_Dpad_Select))
	{
		// These buttons should not autorepeat. The game handlers are not really equipped for that.
		if (hidInput->joyaxes[JOYAXIS_Forward] > 0 && !(dpad_lock & 1)) { dpad_lock |= 1;  inputBuffer.setNewWeapon(WeaponSel_Prev); }
		else dpad_lock &= ~1;
		if (hidInput->joyaxes[JOYAXIS_Forward] < 0 && !(dpad_lock & 2)) { dpad_lock |= 2;  inputBuffer.setNewWeapon(WeaponSel_Next); }
		else dpad_lock &= ~2;
		if ((hidInput->joyaxes[JOYAXIS_Side] < 0 || hidInput->joyaxes[JOYAXIS_Yaw] > 0) && !(dpad_lock & 4)) { dpad_lock |= 4;  inputBuffer.actions |= SB_INVPREV; }
		else dpad_lock &= ~4;
		if ((hidInput->joyaxes[JOYAXIS_Side] > 0 || hidInput->joyaxes[JOYAXIS_Yaw] < 0) && !(dpad_lock & 8)) { dpad_lock |= 8;  inputBuffer.actions |= SB_INVNEXT; }
		else dpad_lock &= ~8;

		// This eats the controller input for regular use
		hidInput->joyaxes[JOYAXIS_Side] = 0;
		hidInput->joyaxes[JOYAXIS_Forward] = 0;
		hidInput->joyaxes[JOYAXIS_Yaw] = 0;
	}
	else dpad_lock = 0;

	gi->reapplyInputBits(&inputBuffer);

	inputBuffer.actions |= ActionsToSend;
	ActionsToSend = 0;

	if (buttonMap.ButtonDown(gamefunc_Aim_Up) || (buttonMap.ButtonDown(gamefunc_Dpad_Aiming) && hidInput->joyaxes[JOYAXIS_Forward] > 0)) 
	{
		inputBuffer.actions |= SB_AIM_UP;
		inputBuffer.actions &= ~SB_CENTERVIEW;
	}

	if ((buttonMap.ButtonDown(gamefunc_Aim_Down) || (buttonMap.ButtonDown(gamefunc_Dpad_Aiming) && hidInput->joyaxes[JOYAXIS_Forward] < 0))) 
	{
		inputBuffer.actions |= SB_AIM_DOWN;
		inputBuffer.actions &= ~SB_CENTERVIEW;
	}

	if (buttonMap.ButtonDown(gamefunc_Dpad_Aiming))
		hidInput->joyaxes[JOYAXIS_Forward] = 0;

	if (buttonMap.ButtonDown(gamefunc_Jump))
		inputBuffer.actions |= SB_JUMP;

	if (buttonMap.ButtonDown(gamefunc_Crouch) || buttonMap.ButtonDown(gamefunc_Toggle_Crouch) || crouch_toggle)
		inputBuffer.actions |= SB_CROUCH;

	if (buttonMap.ButtonDown(gamefunc_Toggle_Crouch))
	{
		crouch_toggle = !crouch_toggle;
		buttonMap.ClearButton(gamefunc_Toggle_Crouch);
	}

	if (buttonMap.ButtonDown(gamefunc_Crouch) || buttonMap.ButtonDown(gamefunc_Jump))
		crouch_toggle = false;

	if (buttonMap.ButtonDown(gamefunc_Fire))
		inputBuffer.actions |= SB_FIRE;

	if (buttonMap.ButtonDown(gamefunc_Alt_Fire))
		inputBuffer.actions |= SB_ALTFIRE;

	if (buttonMap.ButtonDown(gamefunc_Open))
	{
		if (isBlood()) buttonMap.ClearButton(gamefunc_Open);
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

void clearLocalInputBuffer()
{
	inputBuffer = {};
	WeaponToSend = 0;
	dpad_lock = 0;
	resetTurnHeldAmt();
}

void getInput(const double scaleAdjust, PlayerAngles* const plrAngles, InputPacket* packet)
{
	if (M_Active() || gamestate != GS_LEVEL || !plrAngles || !plrAngles->pActor)
	{
		inputBuffer = {};
		return;
	}

	InputPacket input{};
	HIDInput hidInput{};
	ApplyGlobalInput(&hidInput);

	// Directly update the camera angles if we're unsynchronised.
	if (!SyncInput())
	{
		gi->GetInput(&hidInput, &input, scaleAdjust);
		plrAngles->CameraAngles.Yaw += DAngle::fromDeg(input.avel);
		plrAngles->CameraAngles.Pitch += DAngle::fromDeg(input.horz);
	}
	else
	{
		gi->GetInput(&hidInput, &input, 1);
	}

	if (packet)
	{
		*packet = inputBuffer;
		inputBuffer = {};
	}
}


//---------------------------------------------------------------------------
//
// Adjust player's pitch by way of keyboard input.
//
//---------------------------------------------------------------------------

void PlayerAngles::doPitchKeys(InputPacket* const input)
{
	// Cancel return to center if conditions met.
	if (input->horz)
		input->actions &= ~SB_CENTERVIEW;

	// Set up a myriad of bools.
	const auto aimingUp = (input->actions & SB_LOOK_UP) == SB_AIM_UP;
	const auto aimingDown = (input->actions & SB_LOOK_DOWN) == SB_AIM_DOWN;
	const auto lookingUp = (input->actions & SB_LOOK_UP) == SB_LOOK_UP;
	const auto lookingDown = (input->actions & SB_LOOK_DOWN) == SB_LOOK_DOWN;

	// Process keyboard input.
	if (const auto aiming = aimingDown - aimingUp)
	{
		pActor->spr.Angles.Pitch += DAngle::fromDeg(getTicrateScale(PITCH_AIMSPEED) * aiming);
		input->actions &= ~SB_CENTERVIEW;
	}
	if (const auto looking = lookingDown - lookingUp)
	{
		pActor->spr.Angles.Pitch += DAngle::fromDeg(getTicrateScale(PITCH_LOOKSPEED) * looking);
		input->actions |= SB_CENTERVIEW;
	}

	// Do return to centre.
	if ((input->actions & SB_CENTERVIEW) && !(lookingUp || lookingDown))
	{
		const auto pitch = abs(pActor->spr.Angles.Pitch);
		const auto scale = pitch > PITCH_CNTRSINEOFFSET ? (pitch - PITCH_CNTRSINEOFFSET).Cos() : 1.;
		if (scaletozero(pActor->spr.Angles.Pitch, PITCH_CENTERSPEED * scale))
			input->actions &= ~SB_CENTERVIEW;
	}

	// clamp before we finish, factoring in the player's view pitch offset.
	const auto maximum = GetMaxPitch() - ViewAngles.Pitch * (ViewAngles.Pitch < nullAngle);
	const auto minimum = GetMinPitch() - ViewAngles.Pitch * (ViewAngles.Pitch > nullAngle);
	pActor->spr.Angles.Pitch = clamp(pActor->spr.Angles.Pitch, maximum, minimum);
}


//---------------------------------------------------------------------------
//
// Adjust player's yaw by way of keyboard input.
//
//---------------------------------------------------------------------------

void PlayerAngles::doYawKeys(InputPacket* const input)
{
	if (input->actions & SB_TURNAROUND)
	{
		if (YawSpin == nullAngle)
		{
			// currently not spinning, so start a spin
			YawSpin = -DAngle180;
		}
		input->actions &= ~SB_TURNAROUND;
	}

	if (YawSpin < nullAngle)
	{
		// return spin to 0
		DAngle add = DAngle::fromDeg(getTicrateScale(!(input->actions & SB_CROUCH) ? YAW_SPINSTAND : YAW_SPINCROUCH));
		YawSpin += add;
		if (YawSpin > nullAngle)
		{
			// Don't overshoot our target. With variable factor this is possible.
			add -= YawSpin;
			YawSpin = nullAngle;
		}
		pActor->spr.Angles.Yaw += add;
	}
}


//---------------------------------------------------------------------------
//
// Player's slope tilt when playing without a mouse and on a slope.
//
//---------------------------------------------------------------------------

void PlayerAngles::doViewPitch(const bool canslopetilt, const bool climbing)
{
	if (cl_slopetilting && canslopetilt)
	{
		const auto actorsect = pActor->sector();
		if (actorsect && (actorsect->floorstat & CSTAT_SECTOR_SLOPE)) // If the floor is sloped
		{
			// Get a point, 512 (64 for Blood) units ahead of player's position
			const auto rotpt = pActor->spr.pos.XY() + pActor->spr.Angles.Yaw.ToVector() * (!isBlood() ? 32 : 4);
			auto tempsect = actorsect;
			updatesector(rotpt, &tempsect);

			if (tempsect != nullptr) // If the new point is inside a valid sector...
			{
				// Get the floorz as if the new (x,y) point was still in
				// your sector, unless it's Blood.
				const double j = getflorzofslopeptr(actorsect, pActor->spr.pos.XY());
				const double k = getflorzofslopeptr(!isBlood() ? actorsect : tempsect, rotpt);

				// If extended point is in same sector as you or the slopes
				// of the sector of the extended point and your sector match
				// closely (to avoid accidently looking straight out when
				// you're at the edge of a sector line) then adjust horizon
				// accordingly
				if (actorsect == tempsect || (!isBlood() && abs(getflorzofslopeptr(tempsect, rotpt) - k) <= 4))
				{
					ViewAngles.Pitch -= maphoriz((j - k) * (!isBlood() ? 0.625 : 5.5));
				}
			}
		}
	}

	if (cl_slopetilting && climbing)
	{
		// tilt when climbing but you can't even really tell it.
		if (ViewAngles.Pitch > PITCH_HORIZOFFCLIMB)
			ViewAngles.Pitch += getscaledangle(deltaangle(ViewAngles.Pitch, PITCH_HORIZOFFCLIMB), PITCH_HORIZOFFSPEED, PITCH_HORIZOFFPUSH);
	}
	else
	{
		// Make horizoff grow towards 0 since horizoff is not modified when you're not on a slope.
		scaletozero(ViewAngles.Pitch, PITCH_HORIZOFFSPEED, PITCH_HORIZOFFPUSH);
	}

	// Clamp off against the maximum allowed pitch.
	ViewAngles.Pitch = ClampViewPitch(ViewAngles.Pitch);
}


//---------------------------------------------------------------------------
//
// Player's look left/right key angle handler.
//
//---------------------------------------------------------------------------

void PlayerAngles::doViewYaw(InputPacket* const input)
{
	// Process angle return to zeros.
	scaletozero(ViewAngles.Yaw, YAW_LOOKRETURN);
	scaletozero(ViewAngles.Roll, YAW_LOOKRETURN);

	// Process keyboard input.
	if (const auto looking = !!(input->actions & SB_LOOK_RIGHT) - !!(input->actions & SB_LOOK_LEFT))
	{
		ViewAngles.Yaw += DAngle::fromDeg(getTicrateScale(YAW_LOOKINGSPEED) * looking);
		ViewAngles.Roll += DAngle::fromDeg(getTicrateScale(YAW_ROTATESPEED) * looking);
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngles& w, PlayerAngles* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("viewangles", w.ViewAngles)
			("spin", w.YawSpin)
			("actor", w.pActor)
			.EndObject();

		if (arc.isReading())
		{
			w.resetCameraAngles();
		}
	}
	return arc;
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
	const int max = isExhumed() || isDukeShareware ? 7 : isBlood() ? 12 : 10;

	if (argv.argc() != 2)
	{
		Printf("slot <weaponslot>: select a weapon from the given slot (1-%d)", max);
		return;
	}

	const auto slot = atoi(argv[1]);

	if (slot >= 1 && slot <= max)
	{
		WeaponToSend = slot;
	}
}

CCMD(weapprev)
{
	WeaponToSend = WeaponSel_Prev;
}

CCMD(weapnext)
{
	WeaponToSend = WeaponSel_Next;
}

CCMD(weapalt)
{
	WeaponToSend = WeaponSel_Alt;	// Only used by SW - should also be made usable by Blood ans Duke which put multiple weapons in the same slot.
}
