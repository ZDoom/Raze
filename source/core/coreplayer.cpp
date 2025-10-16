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

// Looking speed (in Build angle).
// Duke:     152 * 30 = 4560;
static constexpr double YAW_LOOKINGSPEED = mapangle(152).Degrees() * 30.;

// Rotating speed (in Build angle).
// Duke:      24 * 30 = 720 (halved for a consistent YAW_LOOKRETURN value)
static constexpr double YAW_ROTATESPEED = mapangle(24).Degrees() * 0.5 * 30.;

// Look return speed (in Build angle).
// Duke: (1 / 4) * 30 = 7.5;
static constexpr double YAW_LOOKRETURN = 0.25 * 30.;

// Look return push (in Build angle).
// Duke:       1 * 30 = 30;
// SW:         1 * 40 = 40;
// Average: 35.;
static constexpr double YAW_LOOKRETURNPUSH = ((mapangle(1).Degrees() * 30.) + (mapangle(1).Degrees() * 40.)) * 0.5;

// Spin standing speed (in Build angle).
// Duke:     128 * 30 = 3840;
// Blood:    128 * 30 = 3840;
static constexpr double YAW_SPINSTAND = mapangle(128).Degrees() * 30.;
static constexpr double YAW_SPINCROUCH = YAW_SPINSTAND * 0.5;

// Look speed (in Build tangent).
// Duke:     12 * 30 = 360;
// SW:       16 * 40 = 640;
// Average: 500.;
static const double PITCH_LOOKSPEED = ((maphoriz(12).Degrees() * 30.) + (maphoriz(16).Degrees() * 40.)) * 0.5;
static const double PITCH_AIMSPEED = PITCH_LOOKSPEED * 0.5;

// Return to centre speed (in Build tangent).
// Duke: (1 / 3) * 30 = 10;
// SW:   (1 / 4) * 40 = 10;
// Average: 10.;
static constexpr double PITCH_CENTERSPEED = ((((1. / 3.) * 30.) + ((1. / 4.) * 40.)) * 0.5) + (3. / 4.); // Plus additional sine offset compensation.

// Horizoff centre speed (in Build tangent).
// Duke: (1 / 8) * 30 = 3.75;
// SW:   (1 / 8) * 40 = 5;
// Average: 4.375;
static constexpr double PITCH_HORIZOFFSPEED = (((1. / 8.) * 30.) + ((1. / 8.) * 40.)) * 0.5;

// Horizoff climb maximum (in Build tangent).
// SW:                = 100;
static const DAngle PITCH_HORIZOFFCLIMB = -maphoriz(100);

// Horizon offset push (in Build tangent).
// Duke:       1 * 30 = 30;
// SW:         1 * 40 = 40;
// Average: 35.;
static const double PITCH_HORIZOFFPUSH = ((maphoriz(1).Degrees() * 30.) + (maphoriz(1).Degrees() * 40.)) * 0.5;

// Constants for new input features, not in the original games.
static const double SCALETOZEROPUSH = (PITCH_HORIZOFFPUSH + YAW_LOOKRETURNPUSH) * 0.5 * (2. / 9.);
static constexpr DAngle PITCH_CNTRSINEOFFSET = DAngle90 * (1. / 8.);
static constexpr DAngle ROLL_TILTAVELMAX = DAngle::fromDeg((90. / 32.) * 1.5);
static constexpr double ROLL_TILTAVELSCALE = (1966426. / 12000000.);
static constexpr double ROLL_TILTRETURN = 15.;


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
		if (scaleAngleToZero(actor->spr.Angles.Pitch, PITCH_CENTERSPEED * scale))
		{
			cmd.ucmd.actions &= ~SB_CENTERVIEW;
		}
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
		{
			ViewAngles.Pitch += getScaledAngle(deltaangle(ViewAngles.Pitch, PITCH_HORIZOFFCLIMB), PITCH_HORIZOFFSPEED, PITCH_HORIZOFFPUSH);
		}
	}
	else
	{
		// Make horizoff grow towards 0 since horizoff is not modified when you're not on a slope.
		scaleAngleToZero(ViewAngles.Pitch, PITCH_HORIZOFFSPEED, PITCH_HORIZOFFPUSH);
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
	scaleAngleToZero(ViewAngles.Yaw, YAW_LOOKRETURN);
	scaleAngleToZero(ViewAngles.Roll, YAW_LOOKRETURN);

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
			scaleAngleToZero(actor->spr.Angles.Roll, ROLL_TILTRETURN);
			actor->spr.Angles.Roll = clamp(actor->spr.Angles.Roll + rollAdj, -rollMax, rollMax);
		}
		else if (cl_viewtilting == 2)
		{
			// Quake-like strafe rolling. Adjustment == (90/48) for running keyboard strafe.
			const auto rollAdj = RollVel * strafeScale * rollAmp;
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


//---------------------------------------------------------------------------
//
// Scales a referenced angle's value towards zero by the given scale.
//
//---------------------------------------------------------------------------

bool scaleAngleToZero(DAngle& angle, const double scale, const double push)
{
	const auto sgn = angle.Sgn();
	if (!sgn || sgn != (angle -= getScaledAngle(angle, scale, getTicrateAngle(push == DBL_MAX ? SCALETOZEROPUSH : push) * sgn)).Sgn())
	{
		angle = nullAngle;
		return true;
	}
	return false;
}


//---------------------------------------------------------------------------
//
// Stat for console player's position/angles.
//
//---------------------------------------------------------------------------

ADD_STAT(coord)
{
	FString out;
	const auto p = PlayerArray[myconnectindex];
	if (const auto pActor = p->GetActor())
	{
		out.AppendFormat("X: %.4f  ", pActor->spr.pos.X);
		out.AppendFormat("Y: %.4f  ", pActor->spr.pos.Y);
		out.AppendFormat("Z: %.4f\n", pActor->spr.pos.Z);
		out.AppendFormat("Yaw: %.4f  ", pActor->spr.Angles.Yaw.Degrees());
		out.AppendFormat("Pitch: %.4f  ", pActor->spr.Angles.Pitch.Degrees());
		out.AppendFormat("Roll: %.4f\n", pActor->spr.Angles.Roll.Degrees());
		out.AppendFormat("View Yaw: %.4f  ", p->ViewAngles.Yaw.Degrees());
		out.AppendFormat("View Pitch: %.4f  ", p->ViewAngles.Pitch.Degrees());
		out.AppendFormat("View Roll: %.4f\n", p->ViewAngles.Roll.Degrees());
	}
	return out;
}


//---------------------------------------------------------------------------
//
// CCMD to warp console player to the given coordinates.
//
//---------------------------------------------------------------------------

CCMD(warptocoords)
{
	if (netgame)
	{
		Printf("warptocoords cannot be used in multiplayer.\n");
		return;
	}
	if (argv.argc() < 4)
	{
		Printf("warptocoords [x] [y] [z] [yaw] (optional) [pitch] (optional): warps the player to the specified coordinates\n");
		return;
	}
	if (gamestate != GS_LEVEL)
	{
		Printf("warptocoords: must be in a level\n");
		return;
	}

	if (const auto pActor = PlayerArray[myconnectindex]->GetActor())
	{
		pActor->spr.pos = DVector3(atof(argv[1]), atof(argv[2]), atof(argv[3]));
		if (argv.argc() > 4) pActor->spr.Angles.Yaw = DAngle::fromDeg(atof(argv[4]));
		if (argv.argc() > 5) pActor->spr.Angles.Pitch = DAngle::fromDeg(atof(argv[5]));
		pActor->backuploc();
	}
}
