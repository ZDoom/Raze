//-------------------------------------------------------------------------
/*
Copyright (C) 2023 Christoph Oelckers
Copyright (C) 2023 Mitchell Richters

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

#include "gameinput.h"


//---------------------------------------------------------------------------
//
// Input constants used throughout associated functions.
//
//---------------------------------------------------------------------------

static constexpr double ROLL_TILTAVELSCALE = (1966426. / 12000000.);
static constexpr DAngle ROLL_TILTAVELMAX = DAngle::fromDeg(90. / 32. * 1.5);
static constexpr double ROLL_TILTRETURN = 15.;
static constexpr double YAW_LOOKINGSPEED = 801.5625;
static constexpr double YAW_ROTATESPEED = 63.28125;
static constexpr double YAW_LOOKRETURN = 7.5;
static constexpr double YAW_SPINSTAND = 675.;
static constexpr double YAW_SPINCROUCH = YAW_SPINSTAND * 0.5;
static constexpr double PITCH_LOOKSPEED = (269426662. / 1209103.);
static constexpr double PITCH_AIMSPEED = PITCH_LOOKSPEED * 0.5;
static constexpr double PITCH_CENTERSPEED = 10.7375;
static constexpr double PITCH_HORIZOFFSPEED = 4.375;
static constexpr DAngle PITCH_CNTRSINEOFFSET = DAngle90 / 8.;
static constexpr DAngle PITCH_HORIZOFFCLIMB = DAngle::fromDeg(-127076387. / 3344227.);
static constexpr DAngle PITCH_HORIZOFFPUSH = DAngle::fromDeg(14115687. / 31535389.);


//---------------------------------------------------------------------------
//
// CVARs to control view rolling.
//
//---------------------------------------------------------------------------

EXTERN_CVAR(Int, vr_mode)
CVAR(Float, cl_viewtiltscale, 1.f, CVAR_GLOBALCONFIG | CVAR_ARCHIVE);
CUSTOM_CVAR(Int, cl_viewtilting, 0, CVAR_GLOBALCONFIG | CVAR_ARCHIVE)
{
	if (self < 0) self = 0;
	else if (self > 3) self = 3;
}


//---------------------------------------------------------------------------
//
// Adjust player's pitch by way of keyboard input.
//
//---------------------------------------------------------------------------

void DCorePlayer::doPitchInput()
{
	// Add player's mouse/device input.
	if (cmd.ucmd.ang.Pitch.Degrees())
	{
		actor->spr.Angles.Pitch += cmd.ucmd.ang.Pitch * gameInput.SyncInput();
		cmd.ucmd.actions &= ~SB_CENTERVIEW;
	}

	// Set up a myriad of bools.
	const auto aimingUp = (cmd.ucmd.actions & SB_LOOK_UP) == SB_AIM_UP;
	const auto aimingDown = (cmd.ucmd.actions & SB_LOOK_DOWN) == SB_AIM_DOWN;
	const auto lookingUp = (cmd.ucmd.actions & SB_LOOK_UP) == SB_LOOK_UP;
	const auto lookingDown = (cmd.ucmd.actions & SB_LOOK_DOWN) == SB_LOOK_DOWN;

	// Process keyboard input.
	if (const auto aiming = aimingDown - aimingUp)
	{
		actor->spr.Angles.Pitch += getTicrateAngle(PITCH_AIMSPEED * aiming);
		cmd.ucmd.actions &= ~SB_CENTERVIEW;
	}
	if (const auto looking = lookingDown - lookingUp)
	{
		actor->spr.Angles.Pitch += getTicrateAngle(PITCH_LOOKSPEED * looking);
		cmd.ucmd.actions |= SB_CENTERVIEW;
	}

	// Do return to centre.
	if ((cmd.ucmd.actions & SB_CENTERVIEW) && !(lookingUp || lookingDown))
	{
		const auto pitch = abs(actor->spr.Angles.Pitch);
		const auto scale = pitch > PITCH_CNTRSINEOFFSET ? (pitch - PITCH_CNTRSINEOFFSET).Cos() : 1.;
		if (scaletozero(actor->spr.Angles.Pitch, PITCH_CENTERSPEED * scale))
			cmd.ucmd.actions &= ~SB_CENTERVIEW;
	}

	// clamp before we finish, factoring in the player's view pitch offset.
	const auto maximum = GetMaxPitch() - ViewAngles.Pitch * (ViewAngles.Pitch < nullAngle);
	const auto minimum = GetMinPitch() - ViewAngles.Pitch * (ViewAngles.Pitch > nullAngle);
	actor->spr.Angles.Pitch = clamp(actor->spr.Angles.Pitch, maximum, minimum);
}


//---------------------------------------------------------------------------
//
// Adjust player's yaw by way of keyboard input.
//
//---------------------------------------------------------------------------

void DCorePlayer::doYawInput()
{
	// Add player's mouse/device input.
	actor->spr.Angles.Yaw += cmd.ucmd.ang.Yaw * gameInput.SyncInput();

	if (cmd.ucmd.actions & SB_TURNAROUND)
	{
		if (YawSpin == nullAngle)
		{
			// currently not spinning, so start a spin
			YawSpin = -DAngle180;
		}
		cmd.ucmd.actions &= ~SB_TURNAROUND;
	}

	if (YawSpin < nullAngle)
	{
		// return spin to 0
		DAngle add = getTicrateAngle(!(cmd.ucmd.actions & SB_CROUCH) ? YAW_SPINSTAND : YAW_SPINCROUCH);
		YawSpin += add;
		if (YawSpin > nullAngle)
		{
			// Don't overshoot our target. With variable factor this is possible.
			add -= YawSpin;
			YawSpin = nullAngle;
		}
		actor->spr.Angles.Yaw += add;
	}
}


//---------------------------------------------------------------------------
//
// Player's slope tilt when playing without a mouse and on a slope.
//
//---------------------------------------------------------------------------

void DCorePlayer::doViewPitch(const bool climbing)
{
	if (cl_slopetilting && canSlopeTilt())
	{
		const auto actorsect = actor->sector();
		if (actorsect && (actorsect->floorstat & CSTAT_SECTOR_SLOPE)) // If the floor is sloped
		{
			// Get a point, 512 (64 for Blood) units ahead of player's position
			const auto rotpt = actor->spr.pos.XY() + actor->spr.Angles.Yaw.ToVector() * (!isBlood() ? 32 : 4);
			auto tempsect = actorsect;
			updatesector(rotpt, &tempsect);

			if (tempsect != nullptr) // If the new point is inside a valid sector...
			{
				// Get the floorz as if the new (x,y) point was still in
				// your sector, unless it's Blood.
				const double j = getflorzofslopeptr(actorsect, actor->spr.pos.XY());
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
}


//---------------------------------------------------------------------------
//
// Player's look left/right key angle handler.
//
//---------------------------------------------------------------------------

void DCorePlayer::doViewYaw()
{
	// Process angle return to zeros.
	scaletozero(ViewAngles.Yaw, YAW_LOOKRETURN);
	scaletozero(ViewAngles.Roll, YAW_LOOKRETURN);

	// Process keyboard input.
	if (const auto looking = !!(cmd.ucmd.actions & SB_LOOK_RIGHT) - !!(cmd.ucmd.actions & SB_LOOK_LEFT))
	{
		ViewAngles.Yaw += getTicrateAngle(YAW_LOOKINGSPEED * looking);
		ViewAngles.Roll += getTicrateAngle(YAW_ROTATESPEED * looking);
	}
}


//---------------------------------------------------------------------------
//
// View tilting effects, mostly for Exhumed to enhance its gameplay feel.
//
//---------------------------------------------------------------------------

void DCorePlayer::doRollInput(const bool bUnderwater)
{
	// Allow viewtilting if we're not in a VR mode.
	if (!vr_mode)
	{
		// Scale/attenuate tilting based on player actions.
		const auto nMaxVel = GetMaxInputVel();
		const auto rollAmp = cl_viewtiltscale / (1 + bUnderwater);
		const auto runScale = 1. / (1 + !(cmd.ucmd.actions & SB_RUN));
		const auto strafeScale = 1 + !!cmd.ucmd.vel.Y;

		if (cl_viewtilting == 1)
		{
			// Console-like yaw rolling. Adjustment == ~(90/32) for keyboard turning. Clamp is 1.5x this value.
			const auto rollAdj = cmd.ucmd.ang.Yaw * ROLL_TILTAVELSCALE * rollAmp;
			const auto rollMax = ROLL_TILTAVELMAX * cl_viewtiltscale;
			scaletozero(actor->spr.Angles.Roll, ROLL_TILTRETURN);
			actor->spr.Angles.Roll = clamp(actor->spr.Angles.Roll + rollAdj, -rollMax, rollMax);
		}
		else if (cl_viewtilting == 2)
		{
			// Quake-like strafe rolling. Adjustment == (90/48) for running keyboard strafe.
			const auto rollAdj = StrafeVel * strafeScale * rollAmp;
			const auto rollMax = nMaxVel * runScale * cl_viewtiltscale;
			actor->spr.Angles.Roll = DAngle::fromDeg(clamp(rollAdj, -rollMax, rollMax) * (1.875 / nMaxVel));
		}
		else if (cl_viewtilting == 3)
		{
			// Movement rolling from player's velocity. Adjustment == (90/48) for running keyboard strafe.
			const auto rollAdj = GetInputVelocity().Rotated(-actor->spr.Angles.Yaw).Y * strafeScale * rollAmp;
			const auto rollMax = nMaxVel * runScale * cl_viewtiltscale;
			actor->spr.Angles.Roll = DAngle::fromDeg(clamp(rollAdj, -rollMax, rollMax) * (1.875 / nMaxVel));
		}
		else
		{
			// Always reset roll if we're not tilting at all.
			actor->spr.Angles.Roll = nullAngle;
		}
	}
	else
	{
		// Add player's device input.
		actor->spr.Angles.Roll += cmd.ucmd.ang.Roll * gameInput.SyncInput();
	}
}
